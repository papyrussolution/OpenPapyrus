// BITMBROW.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
// Модуль, отвечающий за броузер строк товарных документов.
//
// Весьма запутанный модуль. Основная сложность в том, что здесь одним махом реализован
// как собственно броузер для просмотра и редактирования документов, так и броузер, работающий в качестве
// селектора строк для добавления строк в документ по связанному документу (возвраты, продажа по заказу).
//
#include <pp.h>
#pragma hdrstop

class SBrowserSortIndex {
public:
	SBrowserSortIndex() : State(0)
	{
	}
	long   FASTCALL GetOriginalPosition(long rawPos) const
	{
		return (State & stUsed) ? ((rawPos >= 0 && rawPos < PositionIndex.getCountI()) ? PositionIndex.at(rawPos) : 0) : rawPos;
	}
	enum {
		stUsed = 0x0001
	};
	long   State;
	LongArray PositionIndex;
};

class BillItemBrowser : public BrowserWindow {
public:
	friend int SLAPI ViewBillDetails(PPBillPacket *, long, PPObjBill *);
	struct TotalData {
		void   Init()
		{
			memzero(PTR8(this)+sizeof(Text), sizeof(*this)-sizeof(Text));
		}
		enum {
			fHasIndepPhQtty = 0x0001, // По крайней мере одна строка имеет признак PPTFR_INDEPPHQTTY
			fHasVetisGuid   = 0x0002  // @v10.1.8 По крайней мере одна строка имеет сертификат ВЕТИС
		};

		SString Text;
		long   Count;
		long   Flags;
		double Qtty;
		double PhQtty;
		double LinkQtty;
		double ShippedQtty;
		double OrderRest;
		double Rest;
		double Cost;
		double Price;
		double Discount;
		double Amount;
		double VatSum;
		double CurAmount;
		double OldPrice;
		double ExtCost;
		double PckgCount;
		double OrderQtty; // @v9.1.1 Заказанное количество, соответсвтующее данному документу отгрузки
	};
	BillItemBrowser(uint rezID, PPObjBill * pBObj, PPBillPacket *, PPBillPacket * pMainPack, int pckgPos, int asSelector, int editMode);
	~BillItemBrowser();
	const  PPBillPacket * GetPacket() const { return P_Pack; }
	void   GetTotal(TotalData * pTotal) const { ASSIGN_PTR(pTotal, Total); }
	const  LongArray & SLAPI GetPriceDevList() const;
	const  StrAssocArray & SLAPI GetProblemsList() const;

	struct ColumnPosBlock {
		SLAPI  ColumnPosBlock() : QttyPos(-2), CostPos(-2), PricePos(-2), SerialPos(-2), QuotInfoPos(-2), CodePos(-2), LinkQttyPos(-2), 
			OrdQttyPos(-2), ShippedQttyPos(-2), VetisCertPos(-2)
		{
		}
		int    SLAPI IsEmpty() const
		{
			return BIN(QttyPos < 0 && CostPos < 0 && PricePos < 0 && SerialPos < 0 &&
				QuotInfoPos < 0 && CodePos < 0 && LinkQttyPos < 0 && OrdQttyPos < 0 && ShippedQttyPos < 0 && VetisCertPos < 0);
		}
		long   QttyPos;
		long   CostPos;
		long   PricePos;
		long   SerialPos;
		long   QuotInfoPos;
		long   CodePos;
		long   LinkQttyPos;
		long   OrdQttyPos;
		long   ShippedQttyPos;
		long   VetisCertPos; // @v10.5.11
	};
	int    GetColPos(ColumnPosBlock & rBlk);
	int    HasLinkPack() const { return BIN(P_LinkPack); }
	//
	// Descr: Возвращает количество из связанного документа, соответствующее строке rTi документа P_Pack
	//
	double FASTCALL GetLinkQtty(const PPTransferItem & rTi) const;
	double FASTCALL GetOrderedQtty(const PPTransferItem & rTi) const;
	int    SLAPI CalcShippedQtty(const BillGoodsBrwItem * pItem, const BillGoodsBrwItemArray * pList, double * pVal);
	int    SLAPI CmpSortIndexItems(const BillGoodsBrwItem * pItem1, const BillGoodsBrwItem * pItem2);
private:
	static int PriceDevColorFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr);
	static int SortFunc(const LongArray * pSortColIdxList, void * extraPtr);
	DECL_HANDLE_EVENT;
	void   addItem(int fromOrder = 0, TIDlgInitData * = 0, int sign = 0);
	//
	// Descr: реализует расширенное добавление строки в товарный документ.
	// ARG(mode IN):
	//   0 - вызывается диалог выбора группы и товара
	//   1 - вызывается диалог, предлагающий пользователю ввести строку, которая содержится в
	//       наименовании товара, а после этого появляется диалог со списком товаров, содержащих
	//       введенную пользователем строку.
	//
	void   addItemExt(int mode);
	void   addItemBySerial();
	void   addNewPackage();
	int    addModifItem(int * pSign, TIDlgInitData * pInitData);
	void   editItem();
	void   delItem();
	enum { // Параметр функции update
		pos_top = -1,
		pos_cur = -2,
		pos_bottom = -3
	};
	void   update(int);
	int    _moveItem(int srcRowIdx);
	int    _moveItem2(int srcRowIdx);
	int    selectOrder();
	int    addItemByOrder(const PPBillPacket * pOrderPack, int line);
	int    ConvertBasketToBill();
	int    ConvertBillToBasket();
	void   GetMinMaxQtty(uint itemPos, RealRange & rRange) const;
	int    subtractRetsFromLinkPack();
	// Ненулевой параметр только при вызове из конструктора
	SArray * MakeList(PPBillPacket * = 0, int pckgPos = -1);
	int    ConvertSupplRetLink(PPID locID);
	int    checkForward(const PPTransferItem * pTi, LDATE, int reverse);
	int    editPackageData(LPackage *);
	void   viewPckgItems(int activateNewRow);
	int    getCurItemPos();
	void   selectPckg(PPID goodsID);
	int    isAllGoodsInPckg(PPID goodsID);
	static int FASTCALL GetDataForBrowser(SBrowserDataProcBlock * pBlk);
	int    SLAPI _GetDataForBrowser(SBrowserDataProcBlock * pBlk);
	enum {
		cpdifRestrOnly = 0x0001
	};
	long   FASTCALL CalcPriceDevItem(long pos, long flags);
	int    SLAPI GetPriceRestrictions(int itemPos, const PPTransferItem & rTi, RealRange * pRange);
	int    SLAPI UpdatePriceDevList(long pos, int op);
	int    SLAPI CheckRows(); // Проверяет список строк документа. Если есть проблемы, то они добавляются в список ProblemsList, который отображается при наведении на соотвествующую строку (колонка 1)
	int    SLAPI EditExtCodeList(int rowIdx);
	int    SLAPI ValidateExtCodeList();
	int    SLAPI Sort(const LongArray * pSortColIdxList);

	enum {
		stOrderSelector    = 0x0002, // Броузер используется как селектор из заказа
		stAltView          = 0x0004, // Альтернативный просмотр строк товарного документа
		stExpndOnReturn    = 0x0008, // Флаг устанавливается при расходном возврате
		stUseLinkSelection = 0x0010, // При выборе товара всегда обращаться к строкам связанного документа
		stShowLinkQtty     = 0x0020, // Показывать количество из связанного документа
		stCtrlX            = 0x0040,
		stAccsCost         = 0x0080, // @*BillItemBrowser::BillItemBrowser
		stActivateNewRow   = 0x0100, // If !0 && !EditMode then execute() calls addItem()
		stIsModified       = 0x0400
	};
	long   State;
	int    AsSelector;
	int    EditMode;
	int    CurLine; // Отслеживает номер текущей строки в функции BillItemBrowser::_GetDataForBrowser
	PPID   OrderBillID;
	PPID   NewGoodsGrpID;
	PPID   AlcoGoodsClsID;
	PPObjBill * P_BObj;
	Transfer  * P_T;
	VetisEntityCore * P_Ec; // @v10.5.11
	PPBillPacket * P_Pack;
	PPBillPacket * P_LinkPack;
	LPackage * P_Pckg;
	TotalData Total;
	PPObjGoods GObj;
	Goods2Tbl::Rec ClGoodsRec;
	LongArray PriceDevList;
	RAssocArray OrdQttyList; // Список величин заказанного количества, сопоставленных с соответствующими лотами заказов
		// используется для ускорения выборки
	LAssocArray VetisExpiryList; // @v10.5.11 Список дат срока годности сертификатов ВЕТИС, сопоставленных с лотами
	StrAssocArray ProblemsList; // Список проблем каждой из строк документа
	SpecSeriesCore * P_SpcCore;
};

#define BROWSER_ID(nam) BROWSER_##nam##2

struct BillGoodsBrwItem {
	enum {
		fHasCode       = 0x0001,
		fRestInited    = 0x0002,
		fOrdRestInited = 0x0004,
		fSerialBad     = 0x0008,
		fSerialOk      = 0x0010,
		fQuotProblem   = 0x0020, // Строки имеет проблемную котировку, выявленную при расценке или назначении цен по котировкам
		fCodeWarn      = 0x0040  // Строка имеет товар с проблемой в штрихкоде
	};
	long   Pos;
	uint   CodePos;
	double Rest;        // Остаток заказанного товара (определяется в контексте связанного товарного документа) //
	double OrderRest;   // Не отгруженное по заказу количество товара                                           //
	double LinkQtty;    // Количество товара в связанном документе
	double VatRate;     // Ставка НДС
	double VatSum;      // Сумма НДС
	double UnitPerPack; //
	uint16 Flags;       //
	int16  RByBill;     //
};

class BillGoodsBrwItemArray : public SArray, public SStrGroup {
public:
	BillGoodsBrwItemArray() : SArray(sizeof(BillGoodsBrwItem)), P_Item(0), HasUpp(0)
	{
	}
	void   SetRest(long itemPos, double val) const
	{
		if(GetItemByPos(itemPos)) {
			P_Item->Rest = val;
			P_Item->Flags |= BillGoodsBrwItem::fRestInited;
		}
	}
	int    GetRest(long itemPos, double * pVal) const
	{
		int    ok = 1;
		if(GetItemByPos(itemPos))
			if(P_Item->Flags & BillGoodsBrwItem::fRestInited) {
				ASSIGN_PTR(pVal, P_Item->Rest);
			}
			else
				ok = -1;
		else
			ok = 0;
		return ok;
	}
	void   SetOrderRest(long itemPos, double val) const
	{
		if(GetItemByPos(itemPos)) {
			P_Item->OrderRest = val;
			P_Item->Flags |= BillGoodsBrwItem::fOrdRestInited;
		}
	}
	int    GetOrderRest(long itemPos, double * pVal) const
	{
		int    ok = 1;
		if(GetItemByPos(itemPos))
			if(P_Item->Flags & BillGoodsBrwItem::fOrdRestInited) {
				ASSIGN_PTR(pVal, P_Item->OrderRest);
			}
			else
				ok = -1;
		else
			ok = 0;
		return ok;
	}
	int    AddCode(long itemPos, const char * pCode)
	{
		int    ok = 1;
		if(GetItemByPos(itemPos)) {
			SStrGroup::AddS(pCode, &P_Item->CodePos);
			P_Item->Flags |= BillGoodsBrwItem::fHasCode;
		}
		else
			ok = 0;
		return ok;
	}
	int    GetCode(long itemPos, SString & rBuf) const
	{
		int    ok = 0;
		if(GetItemByPos(itemPos))
			if(P_Item->Flags & BillGoodsBrwItem::fHasCode) {
				SStrGroup::GetS(P_Item->CodePos, rBuf);
				ok = 1;
			}
			else
				ok = -1;
		return ok;
	}
	int    GetVat(long itemPos, double * pVatRate, double * pVatSum) const
	{
		int    ok = 1;
		if(GetItemByPos(itemPos)) {
			ASSIGN_PTR(pVatRate, P_Item->VatRate);
			ASSIGN_PTR(pVatSum, P_Item->VatSum);
		}
		else
			ok = 0;
		return ok;
	}
	int    HasUpp; // По крайней мере одна строка имеет не нулевую емкость упаковки
private:
	int    FASTCALL GetItemByPos(long itemPos) const
	{
		uint   pos = 0;
		P_Item = lsearch(&itemPos, &pos, CMPF_LONG) ? static_cast<BillGoodsBrwItem *>(at(pos)) : 0;
		return BIN(P_Item);
	}
	mutable BillGoodsBrwItem * P_Item; // Временный указатель
	LAssocArray CodeStatusList;
};

int SLAPI PPObjBill::ViewPckgDetail(PPID pckgID)
{
	int    ok = 1;
	uint   res_id = (LConfig.Flags & CFGFLG_SHOWPHQTTY) ? BROWSER_ID(GOODSITEMPH_W) : BROWSER_ID(GOODSITEM_W);
	if(trfr->Rcpt.Search(pckgID) > 0) {
		PPBillPacket pack;
		// @v9.5.3 (excess) pack.destroy();
		pack.Rec.Dt    = MAXDATE;
		pack.Rec.LocID = trfr->Rcpt.data.LocID;
		if(AddPckgToBillPacket(pckgID, &pack)) {
			BillItemBrowser * brw = new BillItemBrowser(res_id, this, &pack, 0, 0, 0, 2);
			if(brw == 0)
				ok = (PPError(PPERR_NOMEM, 0), 0);
			else
				ExecViewAndDestroy(brw);
		}
	}
	return ok;
}
//
//
//
int SLAPI ViewBillDetails(PPBillPacket * pack, long options, PPObjBill * pBObj)
{
	uint   res_id;
	const PPConfig & r_cfg = LConfig;
	if(pack->Rec.Flags & BILLF_CASH)
		res_id = BROWSER_ID(CHECKITEM_W);
	else if(pack->OpTypeID == PPOPT_GOODSORDER)
		res_id = BROWSER_ID(ORDERITEM_W);
	else if(pack->OpTypeID == PPOPT_GOODSREVAL)
		res_id = BROWSER_ID(GOODSITEM_REVAL_W);
	else if((r_cfg.Flags & CFGFLG_SHOWPHQTTY) && pack->Rec.CurID)
		res_id = BROWSER_ID(GOODSITEMPH_CUR_W);
	else if(r_cfg.Flags & CFGFLG_SHOWPHQTTY)
		res_id = BROWSER_ID(GOODSITEMPH_W);
	else if(pack->Rec.CurID)
		res_id = BROWSER_ID(GOODSITEM_CUR_W);
	else
		res_id = BROWSER_ID(GOODSITEM_W);
	int    r = -1;
	if(!oneof2(pack->OpTypeID, PPOPT_ACCTURN, PPOPT_PAYMENT)) {
		BillItemBrowser * p_brw = new BillItemBrowser(res_id, pBObj, pack, 0, -1, 0, options);
		if(p_brw == 0)
			r = (PPError(PPERR_NOMEM), 0);
		else {
			if(pack->Rec.Flags & BILLF_CASH) {
				PPObjCashNode cn_obj;
				PPCashNode cn_rec;
				if(cn_obj.Fetch(r_cfg.Cash, &cn_rec) > 0 && !(cn_rec.Flags & CASHF_NAFCL)) {
					//ActivateNewRow = 1;
					p_brw->State |= BillItemBrowser::stActivateNewRow;
				}
			}
			ExecView(p_brw);
			//r = p_brw->IsModified ? 1 : -1;
			r = (p_brw->State & BillItemBrowser::stIsModified) ? 1 : -1;
			delete p_brw;
		}
	}
	return r;
}

static int test_lot(const ReceiptTbl::Rec * pLotRec, void * extraPtr)
{
	const PPID loc_id = reinterpret_cast<const PPID>(extraPtr);
	return (pLotRec->LocID == loc_id && !pLotRec->Closed);
}
//
// Эта функция конвертирует товарные строки пакета P_LinkPack таким образом,
// чтобы товар, оприходованный от поставщика на одну локацию, можно было
// вернуть с локации loc, на которой он (товар) оказался в результате
// межскладских перемещений.
//
int BillItemBrowser::ConvertSupplRetLink(PPID locID)
{
	int    ok = 1;
	uint   i, j;
	PPID * p_lot_id;
	PPTrfrArray temp;
	PPIDArray childs;
	PPTransferItem * p_ti, t;
	for(i = 0; P_LinkPack->EnumTItems(&i, &p_ti);) {
		if(p_ti->LocID == locID)
			THROW_SL(temp.insert(p_ti));
		THROW(P_T->Rcpt.GatherChilds(p_ti->LotID, &childs, test_lot, reinterpret_cast<void *>(locID)));
		for(j = 0; childs.enumItems(&j, reinterpret_cast<void **>(&p_lot_id));) {
			t       = *p_ti;
			t.LocID = locID;
			t.LotID = *p_lot_id;
			THROW_SL(temp.insert(&t));
		}
		childs.clear();
		P_LinkPack->RemoveRow(--i);
	}
	P_LinkPack->RemoveRows(0);
	for(i = 0; temp.enumItems(&i, reinterpret_cast<void **>(&p_ti));)
		THROW(P_LinkPack->InsertRow(p_ti, 0));
	CATCHZOK
	return ok;
}

int BillItemBrowser::subtractRetsFromLinkPack()
{
	int    ok = 1;
	BillTbl::Rec   bill_rec;
	PPTransferItem ti;
	if(P_LinkPack && P_LinkPack->Rec.ID && !(State & stExpndOnReturn)) {
		for(DateIter di; P_BObj->P_Tbl->EnumLinks(P_LinkPack->Rec.ID, &di, BLNK_RETURN, &bill_rec) > 0;) {
			if(bill_rec.ID != P_Pack->Rec.ID) {
				for(int rbybill = 0; P_BObj->trfr->EnumItems(bill_rec.ID, &rbybill, &ti) > 0;) {
					double sub_qtty = ti.Quantity_;
					for(uint pos = 0; sub_qtty > 0.0 && P_LinkPack->SearchLot(ti.LotID, &pos) > 0; pos++) {
						PPTransferItem & r_ti = P_LinkPack->TI(pos);
						const double decr = MIN(-r_ti.Quantity_, sub_qtty);
						r_ti.Quantity_ += decr;
						sub_qtty -= decr;
					}
				}
			}
		}
	}
	return ok;
}

int BillItemBrowser::GetColPos(ColumnPosBlock & rBlk)
{
	int    ok = -1;
	if(oneof5(RezID, BROWSER_ID(GOODSITEM_W), BROWSER_ID(GOODSITEMPH_W), BROWSER_ID(GOODSITEMPH_CUR_W),
		BROWSER_ID(GOODSITEM_CUR_W), BROWSER_ID(ORDERITEM_W))) {
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(view->getDef());
		if(p_def) {
			for(uint i = 0; i < p_def->getCount(); i++) {
				const BroColumn & r_col = p_def->at(i);
				switch(r_col.Offs) {
					case  2: rBlk.QttyPos = static_cast<long>(i);  break;
					case  3: rBlk.CostPos = static_cast<long>(i);  break;
					case  4: rBlk.PricePos = static_cast<long>(i); break;
					//case 23: link_qtty_col = static_cast<long>(i); break;
					case 27: rBlk.CodePos = static_cast<long>(i); break;
					case 28: rBlk.SerialPos = static_cast<long>(i); break;
					case 30: rBlk.QuotInfoPos = static_cast<long>(i); break;
					case 31: rBlk.OrdQttyPos = static_cast<long>(i); break;
					case 19: rBlk.ShippedQttyPos = static_cast<long>(i); break;
					case 32: rBlk.VetisCertPos = static_cast<long>(i); break; // @v10.5.11
				}
			}
			ok = rBlk.IsEmpty() ? -1 : 1;
		}
	}
	return ok;
}

//static 
int BillItemBrowser::SortFunc(const LongArray * pSortColIdxList, void * extraPtr)
{
	int    ok = -1;
	BillItemBrowser * p_brw = static_cast<BillItemBrowser *>(extraPtr);
	if(p_brw) {
	}
	return ok;
}

//static 
int BillItemBrowser::PriceDevColorFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	BillItemBrowser * p_brw = static_cast<BillItemBrowser *>(extraPtr);
	if(p_brw && pData && pStyle) {
		BillItemBrowser::ColumnPosBlock posblk;
		const  BillGoodsBrwItem * p_item = static_cast<const BillGoodsBrwItem *>(pData);
		const  long pos = p_item->Pos;
		const  LongArray & r_price_dev_list = p_brw->GetPriceDevList();
		const PPBillPacket * p_pack = p_brw->GetPacket();
		if(p_pack) {
			if(p_brw->GetColPos(posblk) > 0) {
				if(col == posblk.QttyPos && pos >= 0) {
					if(p_pack && pos < static_cast<int>(p_pack->GetTCount())) {
						const PPTransferItem & r_ti = p_pack->ConstTI(pos);
						if(r_ti.Flags & PPTFR_LOTSYNC) {
							pStyle->Color2 = GetColorRef(SClrIndigo);
							pStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
							ok = 1;
						}
						else if(r_ti.Quantity_ < 0.0 && oneof2(p_pack->Rec.OpID, PPOPK_EDI_STOCK, PPOPK_EDI_SHOPCHARGEON)) {
							pStyle->Color2 = GetColorRef(SClrRed);
							pStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
							ok = 1;
						}
					}
				}
				else if(col == posblk.OrdQttyPos && pos >= 0) {
					if(p_pack && pos < static_cast<int>(p_pack->GetTCount())) {
						const PPTransferItem & r_ti = p_pack->ConstTI(pos);
						double ord_qtty = p_brw->GetOrderedQtty(r_ti);
						if((ord_qtty - fabs(r_ti.Qtty())) > 1E-6) {
							pStyle->Color = GetColorRef(SClrOrange);
							ok = 1;
						}
					}
				}
				else if(col == posblk.ShippedQttyPos && pos >= 0) { // @v10.1.5
					if(p_pack && pos < static_cast<int>(p_pack->GetTCount())) {
						const PPTransferItem & r_ti = p_pack->ConstTI(pos);
						const AryBrowserDef * p_def = static_cast<const AryBrowserDef *>(p_brw->getDef());
						const BillGoodsBrwItemArray * p_list = p_def ? static_cast<const BillGoodsBrwItemArray *>(p_def->getArray()) : 0;
						double shp_qtty = 0.0;
						p_brw->CalcShippedQtty(p_item, p_list, &shp_qtty);
						if(fabs(shp_qtty - fabs(r_ti.Quantity_)) > 1E-6) {
							pStyle->Color = GetColorRef(SClrOrange);
							ok = 1;
						}
					}
				}
				else if(col == posblk.VetisCertPos && pos >= 0) { // @v10.5.11
					if(p_pack && pos < static_cast<int>(p_pack->GetTCount())) {
						long   expiry_val = 0;
						if(p_brw->VetisExpiryList.Search(p_item->Pos, &expiry_val, 0)) {
							LDATE expiry_dt;
							expiry_dt.v = static_cast<ulong>(expiry_val);
							if(checkdate(expiry_dt) && expiry_dt <= p_pack->Rec.Dt) {
								pStyle->Color = GetColorRef(SClrCrimson);
								ok = 1;
							}
						}
					}
				}
				/* @construction
				else if(col == link_qtty_pos && pos >= 0) {
					if(p_brw->HasLinkPack()) {
						if(p_pack && pos < (int)p_pack->GetTCount()) {
							const PPTransferItem & r_ti = p_pack->ConstTI(pos);
							double link_qtty = p_brw->GetLinkQtty(r_ti);
							if(!feqeps(link_qtty, fabs(r_ti.Qtty()), 1E-3)) {
								pStyle->Color = GetColorRef(SClrOrchid);
								ok = 1;
							}
						}
					}
				}
				*/
				else if(col == posblk.QuotInfoPos && pos >= 0) {
					uint   qsip = 0;
					if(p_pack->P_QuotSetupInfoList && p_pack->P_QuotSetupInfoList->lsearch(&pos, &qsip, CMPF_LONG)) {
						const PPBillPacket::QuotSetupInfoItem & r_qsi = p_pack->P_QuotSetupInfoList->at(qsip);
						if(r_qsi.Flags & r_qsi.fInvalidQuot)
							pStyle->Color = GetColorRef(SClrRed);
						else if(r_qsi.Flags & r_qsi.fMissingQuot)
							pStyle->Color = GetColorRef(SClrOrange);
						else
							pStyle->Color = GetColorRef(SClrGreen);
					}
					else
						pStyle->Color = GetColorRef(SClrYellow);
					ok = 1;
				}
				if(pos >= 0 && pos < static_cast<long>(r_price_dev_list.getCount())) {
					long   price_flags = r_price_dev_list.at(pos);
					if(price_flags && oneof3(col, posblk.QttyPos, posblk.CostPos, posblk.PricePos)) {
						if(col == posblk.QttyPos && price_flags & LOTSF_FIRST) {
							pStyle->Color = GetColorRef(SClrBlue);
							pStyle->Flags = BrowserWindow::CellStyle::fCorner;
							ok = 1;
						}
						else if(col == posblk.CostPos) {
							if(price_flags & LOTSF_COSTUP) {
								pStyle->Color = GetColorRef(SClrGreen);
								pStyle->Flags = BrowserWindow::CellStyle::fCorner;
								ok = 1;
							}
							else if(price_flags & LOTSF_COSTDOWN) {
								pStyle->Color = GetColorRef(SClrRed);
								pStyle->Flags = BrowserWindow::CellStyle::fCorner;
								ok = 1;
							}
							if(price_flags & LOTSF_LINKCOSTUP) {
								pStyle->Color2 = GetColorRef(SClrGreen);
								pStyle->Flags = BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
							else if(price_flags & LOTSF_LINKCOSTDN) {
								pStyle->Color2 = GetColorRef(SClrRed);
								pStyle->Flags = BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
						}
						else if(col == posblk.PricePos) {
							if(price_flags & LOTSF_PRICEUP) {
								pStyle->Color = GetColorRef(SClrGreen);
								pStyle->Flags = BrowserWindow::CellStyle::fCorner;
								ok = 1;
							}
							else if(price_flags & LOTSF_PRICEDOWN) {
								pStyle->Color = GetColorRef(SClrRed);
								pStyle->Flags = BrowserWindow::CellStyle::fCorner;
								ok = 1;
							}
							if(price_flags & LOTSF_RESTRBOUNDS) {
								pStyle->Color = GetColorRef(SClrGrey);
								pStyle->Flags = BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
						}
					}
				}
				if(posblk.SerialPos >= 0 && col == posblk.SerialPos) {
					if(p_item->Flags & BillGoodsBrwItem::fSerialBad) {
						pStyle->Color = GetColorRef(SClrOrange);
						pStyle->Flags = BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
				if(posblk.CodePos >= 0 && col == posblk.CodePos) {
					if(p_item->Flags & BillGoodsBrwItem::fCodeWarn) {
						pStyle->Color = GetColorRef(SClrOrange);
						pStyle->Flags = BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
			}
			if(col == 0) {
				const StrAssocArray & r_problems_list = p_brw->GetProblemsList();
				if(r_problems_list.Search(pos) > 0) {
					pStyle->Color2 = GetColorRef(SClrRed);
					pStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
					ok = 1;
				}
				if(p_pack && pos >= 0 && pos < static_cast<int>(p_pack->GetTCount()) && p_pack->TI(pos).TFlags & PPTransferItem::tfForceRemove) {
					pStyle->Color = GetColorRef(SClrGrey);
					pStyle->Flags = BrowserWindow::CellStyle::fCorner;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

const LongArray & SLAPI BillItemBrowser::GetPriceDevList() const { return PriceDevList; }
const StrAssocArray & SLAPI BillItemBrowser::GetProblemsList() const { return ProblemsList; }
int SLAPI BillItemBrowser::GetPriceRestrictions(int itemPos, const PPTransferItem & rTi, RealRange * pRange)
	{ return P_BObj->GetPriceRestrictions(*P_Pack, rTi, itemPos, pRange); }

long FASTCALL BillItemBrowser::CalcPriceDevItem(long pos, long flags)
{
	long   price_flags = 0;
	if(P_Pack && pos >= 0 && pos < static_cast<long>(P_Pack->GetTCount())) {
		const PPTransferItem & r_ti = P_Pack->ConstTI(pos);
		RealRange restr_bounds;
		if(!(flags & cpdifRestrOnly)) {
			if(!GObj.CheckFlag(r_ti.GoodsID, GF_UNLIM)) {
				int   r = 0;
				ReceiptTbl::Rec prev_rec, rec;
				if(r_ti.LotID == 0) {
					THROW(r = P_T->Rcpt.GetLastLot(r_ti.GoodsID, r_ti.LocID, r_ti.Date, &prev_rec));
				}
				else if(P_T->Rcpt.Search(r_ti.LotID, &rec) > 0) {
					THROW(r = P_T->Rcpt.GetPreviousLot(rec.GoodsID, rec.LocID, rec.Dt, rec.OprNo, &prev_rec));
				}
				if(r > 0) {
					if(r_ti.Cost > prev_rec.Cost)
						price_flags |= LOTSF_COSTUP;
					else if(r_ti.Cost < prev_rec.Cost)
						price_flags |= LOTSF_COSTDOWN;
					if(r_ti.Price > prev_rec.Price)
						price_flags |= LOTSF_PRICEUP;
					else if(r_ti.Price < prev_rec.Price)
						price_flags |= LOTSF_PRICEDOWN;
				}
				else
					price_flags |= LOTSF_FIRST;
			}
		}
		if(GetPriceRestrictions(pos, r_ti, &restr_bounds) > 0) {
			if(!restr_bounds.CheckValEps(r_ti.NetPrice(), 1E-7)) { // @v10.2.12
				price_flags |= LOTSF_RESTRBOUNDS;
			}
		}
		if(P_LinkPack && !(flags & cpdifRestrOnly)) {
			uint _p = 0;
			if(P_LinkPack->SearchGoods(labs(r_ti.GoodsID), &_p)) {
				double link_cost = P_LinkPack->ConstTI(_p).Cost;
				if(r_ti.Cost > link_cost)
					price_flags |= LOTSF_LINKCOSTUP;
				else if(r_ti.Cost < link_cost)
					price_flags |= LOTSF_LINKCOSTDN;
			}
		}
	}
	CATCH
		price_flags = 0x80000000;
	ENDCATCH
	return price_flags;
}

int SLAPI BillItemBrowser::UpdatePriceDevList(long pos, int op)
{
	int    ok = 1;
	if(P_Pack) {
		long cpdif = -1;
		if(P_Pack->OpTypeID == PPOPT_GOODSRECEIPT)
			cpdif = 0;
		else if(P_Pack->OpTypeID == PPOPT_GOODSEXPEND)
			cpdif = cpdifRestrOnly;
		if(cpdif != -1) {
			const long ti_count = P_Pack->GetTCount();
			if(pos < 0) {
				PriceDevList.clear();
				for(long i = 0; i < ti_count; i++) {
					THROW_SL(PriceDevList.add(CalcPriceDevItem(i, cpdif)));
				}
			}
			else {
				if(op == 0) {
					if(pos < static_cast<long>(PriceDevList.getCount()))
						PriceDevList.at(pos) = CalcPriceDevItem(pos, cpdif);
				}
				else if(op > 0) {
					if(pos < static_cast<long>(PriceDevList.getCount())) { // @v10.3.2 @fix (<=)--(<)
						long   price_flags = CalcPriceDevItem(pos, cpdif);
						PriceDevList.atInsert(static_cast<uint>(pos), &price_flags);
					}
				}
				else { // if(op < 0) {
					if(pos < static_cast<long>(PriceDevList.getCount()))
						PriceDevList.atFree(static_cast<uint>(pos));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI BillItemBrowser::CheckRows()
{
	int    ok = 1;
	SString buf;
	QuotIdent qi(QIDATE(P_Pack->Rec.Dt), P_Pack->Rec.LocID, DS.GetConstTLA().SupplDealQuotKindID, 0, P_Pack->Rec.Object);
	PPTransferItem * p_ti = 0;
	ProblemsList.Z();
	for(uint idx = 0; P_Pack->EnumTItems(&idx, &p_ti) > 0;) {
		ReceiptTbl::Rec lot_rec;
		PPSupplDeal supl_deal;
		if(p_ti->LotID && P_T->Rcpt.Search(p_ti->LotID, &lot_rec) <= 0) {
			PPGetLastErrorMessage(DS.CheckExtFlag(ECF_SYSSERVICE), buf);
			ProblemsList.Add(idx - 1, 0, buf);
		}
		if(GObj.GetSupplDeal(p_ti->GoodsID, qi, &supl_deal) > 0) {
			if(supl_deal.CheckCost(p_ti->Cost) <= 0) {
				PPGetMessage(mfError, PPERR_SUPPLDEALVIOLATION, 0, DS.CheckExtFlag(ECF_SYSSERVICE), buf);
				ProblemsList.Add(idx - 1, 0, buf);
			}
		}
	}
	return ok;
}

BillItemBrowser::BillItemBrowser(uint rezID, PPObjBill * pBObj, PPBillPacket * p,
	PPBillPacket * pMainPack /* Продажа по ордеру */, int pckgPos, int asSelector, int editMode) :
	AsSelector(asSelector), EditMode(editMode), BrowserWindow(rezID, static_cast<SArray *>(0)),
	P_BObj(pBObj), P_T(P_BObj->trfr), P_SpcCore(0), P_Pack(p), P_Pckg(0), State(0), OrderBillID(0),
	NewGoodsGrpID(0), CurLine(-1000), P_LinkPack(pMainPack), P_Ec(0)
{
	SETFLAG(State, stOrderSelector, P_LinkPack);
	SETFLAG(State, stAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
	SETFLAG(State, stAltView, rezID == BROWSER_ID(GOODSITEM_ALTVIEW));
	{
		PrcssrAlcReport::Config parc;
		AlcoGoodsClsID = (PrcssrAlcReport::ReadConfig(&parc) > 0) ? parc.E.AlcGoodsClsID : 0;
	}
	SString temp_buf;
	uint   show_barcode_or_serial = 0;
	if((!(State & stAltView) || ((P_BObj->Cfg.Flags & BCF_SHOWBARCODESINGBLINES) && (CConfig.Flags & CCFLG_USEARGOODSCODE)))) {
		uint   _brw_pos = 2;
		if(P_BObj->Cfg.Flags & BCF_SHOWBARCODESINGBLINES) {
			if((State & stAltView) && (CConfig.Flags & CCFLG_USEARGOODSCODE))
				PPGetWord(PPWORD_ARGOODSCODE, 0, temp_buf);
			else
				PPLoadString("barcode", temp_buf);
			insertColumn(_brw_pos++, temp_buf, 27, MKSTYPE(S_ZSTRING, 20), 0, BCO_USERPROC);
			show_barcode_or_serial |= 0x01;
		}
		if(P_BObj->Cfg.Flags & BCF_SHOWSERIALSINGBLINES) {
			insertColumn(_brw_pos++, PPLoadStringS("serial", temp_buf), 28, MKSTYPE(S_ZSTRING, 20), 0, BCO_USERPROC);
			show_barcode_or_serial |= 0x02;
		}
	}
	uint   i, pos;
	temp_buf.Z();
	if(pckgPos >= 0) {
		P_Pckg = P_Pack->P_PckgList->GetByIdx(pckgPos);
		if(P_Pckg) {
			if(P_Pckg->PckgTypeID) {
				PPObjPckgType pt_obj;
				PPGdsPckgType pt_rec;
				if(pt_obj.Get(P_Pckg->PckgTypeID, &pt_rec) > 0)
					NewGoodsGrpID = pt_rec.GoodsGrpID;
			}
			setTitle(PPLoadTextS(PPTXT_PCKGLINESTITLE, temp_buf));
			setSubTitle(P_Pckg->Code);
		}
	}
	else
		setSubTitle(PPObjBill::MakeCodeString(&P_Pack->Rec, 0, temp_buf));
	if(oneof2(P_Pack->OpTypeID, PPOPT_GOODSRETURN, PPOPT_CORRECTION) && P_Pack->Rec.LinkBillID) {
		PPTransferItem * p_link_ti;
		THROW_MEM(P_LinkPack = new PPBillPacket);
		THROW(P_BObj->ExtractPacket(P_Pack->Rec.LinkBillID, P_LinkPack));
		State |= stUseLinkSelection;
		if(P_Pack->OpTypeID == PPOPT_CORRECTION) {
			for(i = 0; P_LinkPack->EnumTItems(&i, &p_link_ti);) {
				if(P_LinkPack->OpTypeID == PPOPT_GOODSEXPEND) {
					uint    _pos = 0;
					if(P_Pack->SearchTI(p_link_ti->RByBill, &_pos))
						p_link_ti->Flags |= 0x80000000L;
				}
				else {
					if(p_link_ti->LotID) {
						int    used = P_Pack->SearchLot(p_link_ti->LotID, 0);
						if(used)
							p_link_ti->Flags |= 0x80000000L;
					}
					else if(p_link_ti->GoodsID && P_Pack->SearchGoods(p_link_ti->GoodsID, 0))
						p_link_ti->Flags |= 0x80000000L;
				}
			}
		}
		else if(P_Pack->OpTypeID == PPOPT_GOODSRETURN) {
			if(P_Pack->Rec.Flags & BILLF_GEXPEND) {
				State |= stExpndOnReturn;
				THROW(ConvertSupplRetLink(P_Pack->Rec.LocID));
				for(i = 0; P_LinkPack->EnumTItems(&i, &p_link_ti);) {
					if(p_link_ti->LotID) {
						ReceiptTbl::Rec rr;
						int    used = P_Pack->SearchLot(p_link_ti->LotID, 0);
						THROW(P_T->Rcpt.Search(p_link_ti->LotID, &rr) > 0);
						if(rr.Closed && !used)
							P_LinkPack->RemoveRow(--i);
						else {
							double rest = 0.0;
							THROW(P_Pack->BoundsByLot(p_link_ti->LotID, 0, -1, &rest, 0));
							if(rest != 0.0) {
								//
								// В случае возврата поставщику цены устанавливаем с учетом переоценки
								//
								if(P_LinkPack->OpTypeID == PPOPT_GOODSRECEIPT) {
									THROW(P_T->GetLotPrices(&rr, P_Pack->Rec.Dt));
									p_link_ti->Cost  = R5(rr.Cost);
									p_link_ti->Price = R5(rr.Price);
								}
								p_link_ti->Quantity_ = rest;
							}
							else if(used)
								p_link_ti->Flags |= 0x80000000L;
							else
								P_LinkPack->RemoveRow(--i);
						}
					}
					else if(p_link_ti->GoodsID && P_Pack->SearchGoods(p_link_ti->GoodsID, 0))
						p_link_ti->Flags |= 0x80000000L;
				}
			}
			else {
				subtractRetsFromLinkPack();
				for(i = 0; P_LinkPack->EnumTItems(&i, &p_link_ti);)
					if(p_link_ti->LotID) {
						if(p_link_ti->Quantity_ >= 0.0 || P_Pack->SearchLot(p_link_ti->LotID, &(pos = 0)))
							p_link_ti->Flags |= 0x80000000L;
					}
					else if(p_link_ti->GoodsID && P_Pack->SearchGoods(p_link_ti->GoodsID, &(pos = 0)))
						p_link_ti->Flags |= 0x80000000L;
			}
		}
	}
	else if(!(State & stOrderSelector)) {
		P_LinkPack = 0;
		//
		// Загружаем пакет связанного документа для того, чтобы показать
		// количество товара в связанном документе отдельной колонкой
		//
		if(P_Pack->Rec.LinkBillID) {
			BillTbl::Rec link_rec;
			if(P_BObj->Search(P_Pack->Rec.LinkBillID, &link_rec) > 0) {
				const PPID op_type_id = GetOpType(link_rec.OpID);
				if(P_Pack->OpTypeID == PPOPT_GOODSACK || oneof2(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT)) {
					THROW_MEM(P_LinkPack = new PPBillPacket);
					THROW(P_BObj->ExtractPacket(P_Pack->Rec.LinkBillID, P_LinkPack));
					State |= stShowLinkQtty;
					{
						int    at_pos = 1;
						if(show_barcode_or_serial & 0x01)
							at_pos++;
						if(show_barcode_or_serial & 0x02)
							at_pos++;
						// @v10.5.8 PPGetWord(PPWORD_LINKQTTY, 0, temp_buf);
						PPLoadString("linkedqtty", temp_buf); // @v10.5.8
						insertColumn(at_pos+1, temp_buf, 23, MKSTYPE(S_FLOAT, 8), MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_NOZERO), BCO_USERPROC);
					}
				}
			}
		}
	}
	State &= ~stIsModified;
	update(pos_top);
	GetDefScaleData();
	if(P_Pack) {
		THROW(UpdatePriceDevList(-1, 0));
		SetCellStyleFunc(PriceDevColorFunc, this);
	}
	CATCH
		if(P_LinkPack && !(State & stOrderSelector))
			ZDELETE(P_LinkPack);
		PPError();
	ENDCATCH
}

inline BillItemBrowser::~BillItemBrowser()
{
	if(!(State & stOrderSelector))
		delete P_LinkPack;
	delete P_SpcCore;
	delete P_Ec; // @v10.5.11
}

int BillItemBrowser::getCurItemPos()
{
	int    rp = 0; // @v10.6.3 int16-->int
	const AryBrowserDef * p_def = static_cast<const AryBrowserDef *>(view->getDefC());
	if(p_def) {
		int    c_ = getDefC()->_curItem(); // @v10.6.3 int16-->int
		const BillGoodsBrwItemArray * p_list = static_cast<const BillGoodsBrwItemArray *>(p_def->getArray());
		if(p_list && c_ >= 0 && c_ < p_list->getCountI()) {
			int    cp = static_cast<const BillGoodsBrwItem *>(p_list->at(c_))->Pos;
			if(cp >= 0) {
				PPTransferItem * p_ti;
				for(uint i = 0; P_Pack->EnumTItems(&i, &p_ti);) {
					if(P_Pckg) {
						if(P_Pckg->SearchByIdx(i-1, 0) <= 0)
							continue;
					}
					else {
						if((AsSelector && p_ti->Flags & 0x80000000L) || (p_ti->Flags & PPTFR_PCKGGEN))
							continue;
					}
					if(rp == cp)
						return (i-1);
					rp++;
				}
			}
		}
	}
	return -1;
}

SArray * BillItemBrowser::MakeList(PPBillPacket * pPack, int pckgPos)
{
	PPID   id = 0;
	BillGoodsBrwItemArray * p_packed_list = 0;
	PPBillPacket * p_pack = NZOR(pPack, P_Pack);
	LPackage * p_pckg = 0;
	PPTransferItem * p_ti;
	SString temp_buf, lines;
	uint   i, lines_count = 0;
	BillGoodsBrwItem item;
	Goods2Tbl::Rec goods_rec;
	GoodsStockExt gse;
	BarcodeArray bc_list;
	const int check_spoil = BIN(CConfig.Flags & CCFLG_CHECKSPOILAGE && P_BObj->Cfg.Flags & BCF_SHOWSERIALSINGBLINES);
	Total.Init();
	OrdQttyList.clear();
	THROW_MEM(p_packed_list = new BillGoodsBrwItemArray);
	p_pckg = pPack ? ((pckgPos >= 0 && p_pack->P_PckgList) ? p_pack->P_PckgList->GetByIdx(pckgPos) : 0) : P_Pckg;
	for(i = 0; p_pack->EnumTItems(&i, &p_ti);) {
		if(p_pckg) {
			if(p_pckg->SearchByIdx(i-1, 0) <= 0)
				continue;
		}
		else {
			if((AsSelector == 1 && p_ti->Flags & 0x80000000L) || (p_ti->Flags & PPTFR_PCKGGEN))
				continue;
		}
		MEMSZERO(item);
		const double sqtty = p_ti->SQtty(p_pack->Rec.OpID);
		const double qtty  = p_ti->Qtty();
		const double __q = (p_pack->OpTypeID == PPOPT_GOODSMODIF) ? sqtty : qtty;
		if(GObj.Fetch(p_ti->GoodsID, &goods_rec) <= 0)
			MEMSZERO(goods_rec);
		if(p_ti->Flags & PPTFR_INDEPPHQTTY) {
			Total.PhQtty += p_ti->WtQtty;
			Total.Flags |= Total.fHasIndepPhQtty; // @v10.0.07
		}
		else {
			double phuperu;
			// @v10.1.8 {
			const int gphupur = goods_rec.ID ? GObj.GetPhUPerU(&goods_rec, 0, &phuperu) : GObj.GetPhUPerU(static_cast<const Goods2Tbl::Rec *>(0), 0, &phuperu);
			if(gphupur > 0)
				Total.PhQtty += __q * phuperu;
			// } @v10.1.8
			/* @v10.1.8 if(GObj.GetPhUPerU(p_ti->GoodsID, 0, &phuperu) > 0)
				Total.PhQtty += __q * phuperu;*/
		}
		Total.Qtty  += __q;
		Total.Price += R2(p_ti->Price * __q); // (см. комментарии ниже)
		if(State & stAccsCost) {
			Total.Cost += R2(p_ti->Cost * __q); // CalcAmount использует такое же округление, следовательно,
				// если здесь нет округления, то возникнет разница между номинальной суммой и этим значением)
			Total.ExtCost += R2(p_ti->ExtCost * __q);
		}
		if(p_ti->Flags & PPTFR_REVAL) {
			const double op = p_ti->IsRecomplete() ? (p_ti->CurID ? p_ti->CurPrice : p_ti->Price) : p_ti->Discount;
			Total.OldPrice += (op * qtty);
		}
		else {
			Total.Discount += (p_ti->Discount * qtty);
			if(p_ti->CurID)
				Total.CurAmount += p_ti->CurPrice * sqtty;
			// @v9.1.1 {
			if(p_ti->Flags & PPTFR_ONORDER) {
				uint sh_lot_pos = 0;
				if(p_pack->SearchShLot(p_ti->OrdLotID, &sh_lot_pos)) {
                    double ord_qtty = 0.0;
                    ReceiptTbl::Rec lot_rec;
                    if(P_T->Rcpt.Search(p_ti->OrdLotID, &lot_rec) > 0)
						ord_qtty = fabs(lot_rec.Quantity);
                    OrdQttyList.Add(p_ti->OrdLotID, ord_qtty, 0, 0);
					Total.OrderQtty += ord_qtty;
				}
			}
			// } @v9.1.1
		}
		Total.Amount += p_ti->CurID ? p_ti->CalcCurAmount() : p_ti->CalcAmount(!(State & stAccsCost));
		if(P_LinkPack)
			for(uint pos = 0; P_LinkPack->SearchGoods(labs(p_ti->GoodsID), &pos); pos++)
				Total.LinkQtty += fabs(P_LinkPack->ConstTI(pos).Qtty());
		{
			GTaxVect vect;
			vect.CalcTI(*p_ti, P_Pack->Rec.OpID, TIAMT_AMOUNT);
			item.VatSum  = vect.GetValue(GTAXVF_VAT);
			item.VatRate = vect.GetTaxRate(GTAX_VAT, 0);
			Total.VatSum += item.VatSum;
		}
		{
			double upp = p_ti->UnitPerPack;
			if(upp <= 0.0) {
				if(GObj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
					upp = gse.Package;
			}
			if(upp <= 0.0 && P_Pack->IsDraft()) {
				ReceiptTbl::Rec lot_rec;
				if(P_T->Rcpt.GetLastLot(p_ti->GoodsID, P_Pack->Rec.LocID, P_Pack->Rec.Dt, &lot_rec) > 0)
					upp = lot_rec.UnitPerPack;
			}
			if(upp > 0.0) {
				item.UnitPerPack = upp;
				p_packed_list->HasUpp = 1;
				Total.PckgCount += static_cast<long>(fabs(qtty) / upp);
			}
		}
		item.Pos = i-1;
		item.RByBill = p_ti->RByBill;
		if(check_spoil) {
			// @v9.8.11 p_pack->SnL.GetNumber(item.Pos, &temp_buf);
			p_pack->LTagL.GetNumber(PPTAG_LOT_SN, item.Pos, temp_buf); // @v9.8.11
			if(SETIFZ(P_SpcCore, new SpecSeriesCore)) {
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				SpecSeries2Tbl::Rec spc_rec;
				if(P_SpcCore->SearchBySerial(SPCSERIK_SPOILAGE, temp_buf, &spc_rec) > 0) {
					item.Flags &= ~BillGoodsBrwItem::fSerialOk;
					item.Flags |= BillGoodsBrwItem::fSerialBad;
				}
				else {
					item.Flags |= BillGoodsBrwItem::fSerialOk;
					item.Flags &= ~BillGoodsBrwItem::fSerialBad;
				}
			}
		}
		// @v10.1.8 {
		if(!(Total.Flags & Total.fHasVetisGuid)) {
			p_pack->LTagL.GetNumber(PPTAG_LOT_VETIS_UUID, item.Pos, temp_buf);
			if(temp_buf.NotEmpty())
				Total.Flags |= Total.fHasVetisGuid;
			else if(goods_rec.Flags & GF_WANTVETISCERT)
				Total.Flags |= Total.fHasVetisGuid;
		}
		// } @v10.1.8
		if(AlcoGoodsClsID && goods_rec.GdsClsID == AlcoGoodsClsID) {
			int    has_egais_code = 0;
			GObj.P_Tbl->ReadBarcodes(labs(p_ti->GoodsID), bc_list);
			for(uint bcidx = 0; bcidx < bc_list.getCount(); bcidx++) {
				const BarcodeTbl::Rec & r_bc_rec = bc_list.at(bcidx);
				if(sstrlen(r_bc_rec.Code) == 19)
					has_egais_code = 1;
			}
			SETFLAG(item.Flags, BillGoodsBrwItem::fCodeWarn, !has_egais_code);
		}
		THROW_SL(p_packed_list->insert(&item));
		lines_count++;
	}
	if(p_packed_list && (State & stOrderSelector || P_Pack->OpTypeID == PPOPT_GOODSORDER)) {
		BillGoodsBrwItem * p_item;
		Total.ShippedQtty = 0.0;
		Total.OrderRest = 0.0;
		double q;
		for(i = 0; p_packed_list->enumItems(&i, reinterpret_cast<void **>(&p_item));)
			if(CalcShippedQtty(p_item, p_packed_list, &q)) {
				Total.ShippedQtty += q;
				p_packed_list->GetOrderRest(p_item->Pos, &(q = 0.0));
				Total.OrderRest += q;
			}
	}
	PPGetWord(PPWORD_TOTAL, 0, Total.Text).Space().CatChar('(').Cat(PPGetWord(PPWORD_LINES, 0, lines)).
		Space().Cat(static_cast<long>(lines_count)).CatChar(')');
	MEMSZERO(item);
	item.Pos = -1;
	THROW_SL(p_packed_list->insert(&item)); // Total row
	CATCH
		ZDELETE(p_packed_list);
		PPError();
	ENDCATCH
	return p_packed_list;
}

int SLAPI BillItemBrowser::CalcShippedQtty(const BillGoodsBrwItem * pItem, const BillGoodsBrwItemArray * pList, double * pVal)
{
	int    ok = 1;
	double real_val = 0.0;
	if(pList && pItem) {
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(getDef());
		const PPTransferItem & r_ti = P_Pack->ConstTI(pItem->Pos);
		if(pList->GetOrderRest(pItem->Pos, &real_val) < 0) {
			if(State & stOrderSelector) {
				if(GObj.CheckFlag(r_ti.GoodsID, GF_UNLIM))
					real_val = fabs(r_ti.Quantity_);
				else if(P_LinkPack)
					P_LinkPack->RestByOrderLot(r_ti.LotID, 0, -1, &real_val);
				else
					real_val = 0.0;
				pList->SetOrderRest(pItem->Pos, real_val);
			}
			else if(P_Pack->OpTypeID == PPOPT_GOODSORDER)
				if(r_ti.LotID) {
					double rest = 0.0;
					P_BObj->trfr->GetRest(r_ti.LotID, MAXDATE, &rest);
					real_val = fabs(r_ti.Quantity_) - rest;
				}
				else
					real_val = 0.0; //r_ti.SQtty(P_Pack->Rec.OpID);
			else
				ok = 0;
		}
	}
	else
		ok = 0;
	ASSIGN_PTR(pVal, real_val);
	return ok;
}
//
// Порядок следования полей броузера товарных строк документа:
//  0 - Номер строки (в порядке, в котором строки расположены в документе 1..) *
//  1 - Наименование товара                            *
//  2 - Количество                                     *
//  3 - Цена поступления //                            *
//  4 - Цена реализации  //                            *
//  5 - Скидка                                         *
//  6 - Сумма по строке                                *
//  7 - Физическое количество                          *
//  8 - Старая цена поступления (для переоценки)       *
//  9 - Старая цена реализации (для переоценки)        *
// 10 - Валютная цена                                  *
// 11 - Валютная сумма                                 *
// 12 - Налоговая группа                               *
// 13 - Ставка НДС                                     *
// 14 - Сумма НДС                                      *
// 15 - Срок годности                                  *
// 16 - Штрихкод (серийный номер или артикул)          *
// 17 - Признак "Цена поступления без НДС"             *
// 18 - Признак "Цена реализации без налогов"          *
// 19 - Отгружено по заказу                            *
// 20 - Остаток                                        *
// 21 - Масса брутто (на все количество)
// 22 - Объем (на все количество)
// 23 - Связанное количество                           *
// 24 - Остаток по заказу
// 25 - Распределенная себестоимость (ExtCost)
// 26 - Складская ячейка
// 27 - Штрихкод       (поле 16 показывает ЛИБО штрихкод, ЛИБО серию в зависимости от настройки)
// 28 - Серийный номер (поле 16 показывает ЛИБО штрихкод, ЛИБО серию в зависимости от настройки)
// 29 - Количество в упаковках
// 30 - Информация об установленной котировке @v8.2.0
// 31 - Заказанное количество (для документов отгрузки) @v9.1.1
// 32 - GUID сертификата VETIS @v10.1.8
//
int SLAPI BillItemBrowser::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData && P_Pack) {
		SString temp_buf;
		const BillGoodsBrwItem * p_item = static_cast<const BillGoodsBrwItem *>(pBlk->P_SrcData);
		const  int is_total = (p_item->Pos == -1);
		if(is_total || (p_item->Pos >= 0 && p_item->Pos < (PPID)P_Pack->GetTCount())) {
			ok = 1;
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(getDef());
			BillGoodsBrwItemArray * p_list = p_def ? (BillGoodsBrwItemArray *)(p_def->getArray()) : 0; // @badcast
			const PPTransferItem * p_ti = 0;
			if(is_total) {
				CurLine = -1;
				MEMSZERO(ClGoodsRec);
			}
			else {
				p_ti = &P_Pack->ConstTI(p_item->Pos);
				if(p_item->Pos != CurLine) {
					if(GObj.Fetch(p_ti->GoodsID, &ClGoodsRec) <= 0) {
						MEMSZERO(ClGoodsRec);
						ideqvalstr(p_ti->GoodsID, ClGoodsRec.Name, sizeof(ClGoodsRec.Name));
					}
					CurLine = p_item->Pos;
				}
			}
			double real_val = 0.0;
			switch(pBlk->ColumnN) {
				case 0: // Номер строки
					if(is_total)
						ok = 0;
					else if(/*PPMaster*/ 1 /* @v8.9.8 && (CConfig.Flags & CCFLG_DEBUG)*/) // @v9.0.0 PPMaster-->1
						pBlk->Set((int32)p_item->RByBill);
					else
						pBlk->Set((int32)(p_item->Pos+1));
					break;
				case 1: // Наименование товара
					if(is_total)
						pBlk->Set(Total.Text);
					else
						pBlk->Set(ClGoodsRec.Name);
					break;
				case 2: // Количество по строке
					if(is_total)
						pBlk->Set(Total.Qtty);
					else if(p_ti->IsCorrectionExp())
						pBlk->Set(p_ti->Quantity_);
					else
						pBlk->Set(p_ti->SQtty(P_Pack->Rec.OpID));
					break;
				case 3: // Учетная цена поступления по строке (на одну единицу)
					if(State & stAccsCost)
						pBlk->Set(is_total ? Total.Cost : p_ti->Cost);
					else
						ok = 0;
					break;
				case 4: pBlk->Set(is_total ? Total.Price : p_ti->Price); break; // Учетная цена реализации по строке (на одну единицу)
				case 5: pBlk->Set(is_total ? Total.Discount : p_ti->Discount); break; // Скидка по строке (на одну единицу)
				case 6: // Сумма по строке
					if(is_total)
						real_val = Total.Amount;
					else
						real_val = p_ti->CurID ? p_ti->CalcCurAmount() : p_ti->CalcAmount(!(State & stAccsCost));
					pBlk->Set(real_val);
					break;
				case 7: // Физическое количество
					if(is_total)
						real_val = Total.PhQtty;
					else if(p_ti->Flags & PPTFR_INDEPPHQTTY)
						real_val = p_ti->WtQtty;
					else if(GObj.GetPhUPerU(&ClGoodsRec, 0, &real_val) > 0)
						real_val *= p_ti->SQtty(P_Pack->Rec.OpID);
					pBlk->Set(real_val);
					break;
				case 8:  // Старая цена поступления (для переоценки) //
					if(p_ti->Flags & PPTFR_REVAL)
						pBlk->Set(p_ti->Quantity_);
					else
						ok = 0;
					break;
				case 9:  // Старая цена реализации (для переоценки)  //
					if(is_total)
						pBlk->Set(Total.OldPrice);
					else if(p_ti->Flags & PPTFR_REVAL) {
						real_val = p_ti->IsRecomplete() ? (p_ti->CurID ? p_ti->CurPrice : p_ti->Price) : p_ti->Discount;
						pBlk->Set(real_val);
					}
					else
						ok = 0;
					break;
				case 10: // Валютная цена
					if(is_total)
						ok = 0;
					else {
						if(!(p_ti->Flags & PPTFR_REVAL) && p_ti->CurID)
							real_val = p_ti->CurPrice;
						pBlk->Set(real_val);
					}
					break;
				case 11: // Валютная сумма
					if(is_total)
						real_val = Total.CurAmount;
					else if(!(p_ti->Flags & PPTFR_REVAL) && p_ti->CurID)
						real_val = p_ti->CurPrice * p_ti->SQtty(P_Pack->Rec.OpID);
					pBlk->Set(real_val);
					break;
				case 12: // Налоговая группа
					if(is_total)
						ok = 0;
					else {
						GetObjectName(PPOBJ_GOODSTAX, ClGoodsRec.TaxGrpID, pBlk->TempBuf);
						pBlk->TempBuf.CopyTo(static_cast<char *>(pBlk->P_DestData), stsize(pBlk->TypeID));
					}
					break;
				case 13: // Ставка НДС
					if(!is_total && p_list) {
						int    r = p_list->GetVat(p_item->Pos, &real_val, 0);
						if(r > 0)
							pBlk->Set(real_val);
						else
							ok = 0;
					}
					else
						ok = 0;
					break;
				case 14: // Сумма НДС
					if(is_total)
						pBlk->Set(Total.VatSum);
					else if(p_list) {
						int    r = p_list->GetVat(p_item->Pos, 0, &real_val);
						if(r > 0)
							pBlk->Set(real_val);
						else
							ok = 0;
					}
					else
						ok = 0;
					break;
				case 15: // Срок годности
					if(!is_total)
						pBlk->Set(p_ti->Expiry);
					else
						ok = 0;
					break;
				case 16: // Штрихкод или серийный номер или артикул
					if(!is_total && p_list) {
						if(P_BObj->Cfg.Flags & BCF_SHOWBARCODESINGBLINES) {
							if(p_list->GetCode(p_item->Pos, pBlk->TempBuf) < 0) {
								if(State & stAltView && (CConfig.Flags & CCFLG_USEARGOODSCODE)) {
									if(P_Pack && P_Pack->Rec.Object)
										GObj.P_Tbl->GetArCode(P_Pack->Rec.Object, p_ti->GoodsID, temp_buf, 0);
								}
								else
									GObj.FetchSingleBarcode(p_ti->GoodsID, temp_buf);
								p_list->AddCode(p_item->Pos, temp_buf);
								pBlk->Set(temp_buf);
							}
						}
						else {
							// @v9.8.11 P_Pack->SnL.GetNumber(p_item->Pos, &temp_buf);
							P_Pack->LTagL.GetNumber(PPTAG_LOT_SN, p_item->Pos, temp_buf); // @v9.8.11
							pBlk->Set(temp_buf);
						}
					}
					else
						ok = 0;
					break;
				case 17:
					if(!is_total && p_ti->Flags & PPTFR_COSTWOVAT)
						pBlk->Set("Y");
					else
						ok = 0;
					break;
				case 18:
					if(!is_total && p_ti->Flags & PPTFR_PRICEWOTAXES)
						pBlk->Set("Y");
					else
						ok = 0;
					break;
				case 19: // Отгружено по заказу
					if(is_total)
						pBlk->Set(Total.ShippedQtty);
					else {
						ok = CalcShippedQtty(p_item, p_list, &real_val);
						pBlk->Set(real_val);
					}
					break;
				case 20: // Остаток
					if(is_total)
						ok = 0;
					else {
						if(p_list->GetRest(p_item->Pos, &real_val) < 0) {
							if(State & stOrderSelector) {
								if(!(ClGoodsRec.Flags & GF_UNLIM) && P_LinkPack)
									P_LinkPack->GoodsRest(labs(p_ti->GoodsID), 0, -1, &real_val);
								else
									real_val = 0.0;
								p_list->SetRest(p_item->Pos, real_val);
							}
							else
								real_val = 0.0;
						}
						pBlk->Set(real_val);
					}
					break;
				case 23: // Связанное количество
					if(!is_total) {
						real_val = GetLinkQtty(*p_ti);
						pBlk->Set(real_val);
					}
					else
						pBlk->Set(Total.LinkQtty);
					break;
				case 24: // Остаток по заказу
					if(is_total)
						pBlk->Set(Total.OrderRest);
					else {
						ok = CalcShippedQtty(p_item, p_list, 0);
						p_list->GetOrderRest(p_item->Pos, &real_val);
						pBlk->Set(real_val);
					}
					break;
				case 25: // Распределенная себестоимость
					if(is_total)
						pBlk->Set(Total.ExtCost);
					else
						pBlk->Set(p_ti->ExtCost);
					break;
				case 26: // Складская ячейка
					if(is_total)
						pBlk->SetZero();
					else {
						GetLocationName(p_ti->LocID, temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				case 27: // Штрихкод
					if(is_total)
						pBlk->SetZero();
					else {
						GObj.FetchSingleBarcode(p_ti->GoodsID, temp_buf);
						pBlk->Set(temp_buf);
					}
					break;
				case 28: // Серийный номер
					if(is_total)
						pBlk->SetZero();
					else {
						// @v9.8.11 P_Pack->SnL.GetNumber(p_item->Pos, &temp_buf);
						P_Pack->LTagL.GetNumber(PPTAG_LOT_SN, p_item->Pos, temp_buf); // @v9.8.11
						pBlk->Set(temp_buf);
					}
					break;
				case 29:
					if(is_total) {
						temp_buf.Z().Cat(Total.PckgCount, MKSFMTD(0, 0, NMBF_NOZERO));
						pBlk->Set(temp_buf);
					}
					else if(p_item->UnitPerPack > 0.0) {
						char   temp_[256];
						QttyToStr(p_ti->SQtty(P_Pack->Rec.OpID), p_item->UnitPerPack, QTTYF_COMPLPACK|QTTYF_FRACTION, temp_);
						pBlk->Set(temp_);
					}
					else
						pBlk->SetZero();
					break;
				case 30: // Информация об установленной котировке
					if(is_total)
						pBlk->SetZero();
					else {
						uint   qsip = 0;
						if(P_Pack->P_QuotSetupInfoList && P_Pack->P_QuotSetupInfoList->lsearch(&p_item->Pos, &qsip, CMPF_LONG)) {
							const PPBillPacket::QuotSetupInfoItem & r_qsi = P_Pack->P_QuotSetupInfoList->at(qsip);
							if(r_qsi.Flags & r_qsi.fInvalidQuot)
								pBlk->Set("!");
							else if(r_qsi.Flags & r_qsi.fMissingQuot)
								pBlk->Set("x");
							else
								pBlk->Set("*");
						}
						else
							pBlk->SetZero();
					}
					break;
				case 31: // Заказанное количество (для документов отгрузки)
					if(is_total) {
						pBlk->Set(Total.OrderQtty);
					}
					else {
						double ord_qtty = (p_ti->Flags & PPTFR_ONORDER) ? OrdQttyList.Get(p_ti->OrdLotID, 0) : 0.0;
						pBlk->Set(ord_qtty);
					}
					break;
				case 32: // GUID сертификата VETIS @v10.1.8
					if(is_total)
						pBlk->SetZero();
					else {
						P_Pack->LTagL.GetNumber(PPTAG_LOT_VETIS_UUID, p_item->Pos, temp_buf);
						if(temp_buf.Empty()) {
							Goods2Tbl::Rec goods_rec;
							if(GObj.Fetch(p_ti->GoodsID, &goods_rec) > 0 && goods_rec.Flags & GF_WANTVETISCERT)
								temp_buf = "none";
						}
						// @v10.5.11 {
						else {
							long  expiry_val = 0;
							if(!VetisExpiryList.Search(p_item->Pos, &expiry_val, 0)) {
								S_GUID uuid;
								if(uuid.FromStr(temp_buf) && !!uuid) {
									if(SETIFZ(P_Ec, new VetisEntityCore)) {
										VetisEntityCore::Entity vetis_entity;
										if(P_Ec->GetEntityByUuid(uuid, vetis_entity) > 0 && vetis_entity.Kind == VetisEntityCore::kVetDocument) {
											VetisDocumentTbl::Rec vr;
											if(P_Ec->SearchDocument(vetis_entity.ID, &vr) > 0 && (vr.ExpiryTo || vr.ExpiryFrom)) {
												SUniTime ut;
												LDATETIME expiry_dtm;
												ut.FromInt64(NZOR(vr.ExpiryTo, vr.ExpiryFrom));
												if(ut.Get(expiry_dtm))
													expiry_val = static_cast<long>(expiry_dtm.d);
											}
										}
									}
								}
								VetisExpiryList.Add(p_item->Pos, expiry_val);
							}
						}
						// } @v10.5.11
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

static IMPL_CMPFUNC(BillGoodsBrwItem, i1, i2)
{
	BillItemBrowser * p_obj = static_cast<BillItemBrowser *>(pExtraData);
	if(p_obj) {
		const BillGoodsBrwItem * p_item1 = static_cast<const BillGoodsBrwItem *>(i1);
		const BillGoodsBrwItem * p_item2 = static_cast<const BillGoodsBrwItem *>(i2);
		return p_obj->CmpSortIndexItems(p_item1, p_item2);
	}
	else
		return 0;
}

int SLAPI BillItemBrowser::CmpSortIndexItems(const BillGoodsBrwItem * pItem1, const BillGoodsBrwItem * pItem2)
{
	int    sn = 0;
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(getDef());
	if(p_def) {
		for(uint i = 0; !sn && i < GetSettledOrderList().getCount(); i++) {
			if(pItem1->Pos < 0 && pItem2->Pos >= 0) // TOTAL-item is always greater than non-TOTAL one
				sn = +1;
			else if(pItem2->Pos < 0 && pItem1->Pos >= 0) // TOTAL-item is always greater than non-TOTAL one
				sn = -1;
			else {
				int    col = GetSettledOrderList().get(i);
				TYPEID typ1 = 0;
				TYPEID typ2 = 0;
				uint8  dest_data1[512];
				uint8  dest_data2[512];
				if(p_def->GetCellData(pItem1, labs(col)-1, &typ1, &dest_data1, sizeof(dest_data1)) && p_def->GetCellData(pItem2, labs(col)-1, &typ2, &dest_data2, sizeof(dest_data2))) {
					assert(typ1 == typ2);
					if(typ1 == typ2) {
						sn = stcomp(typ1, dest_data1, dest_data2);
						if(sn && col < 0)
							sn = -sn;
					}
				}
			}
		}
	}
	return sn;
}

double FASTCALL BillItemBrowser::GetLinkQtty(const PPTransferItem & rTi) const
{
	double result = 0.0;
	if(P_LinkPack) {
		const PPID goods_id = labs(rTi.GoodsID);
		for(uint pos = 0; P_LinkPack->SearchGoods(goods_id, &pos); pos++)
			result += fabs(P_LinkPack->ConstTI(pos).Qtty());
	}
	return result;
}

double FASTCALL BillItemBrowser::GetOrderedQtty(const PPTransferItem & rTi) const
	{ return (rTi.Flags & PPTFR_ONORDER) ? OrdQttyList.Get(rTi.OrdLotID, 0) : 0.0; }

//static
int FASTCALL BillItemBrowser::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	BillItemBrowser * p_brw = static_cast<BillItemBrowser *>(pBlk->ExtraPtr);
	return p_brw ? p_brw->_GetDataForBrowser(pBlk) : 0;
}

void BillItemBrowser::update(int pos)
{
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(view->getDef());
	if(p_def) {
		long   org_current_pos = p_def->_curItem();
		long   org_current_pos_in_bill = -1;
		const  int is_sorting_needed = BIN(GetSettledOrderList().getCount());
		if(is_sorting_needed && org_current_pos >= 0) {
			const BillGoodsBrwItemArray * p_org_list = static_cast<const BillGoodsBrwItemArray *>(p_def->getArray());
			if(SVectorBase::GetCount(p_org_list) && org_current_pos < p_org_list->getCountI()) {
				const BillGoodsBrwItem * p_org_cur_item = static_cast<const BillGoodsBrwItem *>(p_org_list->at(org_current_pos));
				org_current_pos_in_bill = p_org_cur_item->Pos;
			}
		}
		p_def->setArray(0, 0, 1);
		BillGoodsBrwItemArray * p_list = 0;
		PROFILE(p_list = static_cast<BillGoodsBrwItemArray *>(MakeList()));
		if(p_list) {
			// @v10.6.3 {
			if(is_sorting_needed) {
				p_list->sort(PTR_CMPFUNC(BillGoodsBrwItem), this);
				if(org_current_pos_in_bill >= 0) {
					uint  lp = 0;
					org_current_pos = p_list->lsearch(&org_current_pos_in_bill, &lp, CMPF_LONG) ? static_cast<long>(lp) : -1;
				}
			}
			// } @v10.6.3 
			p_def->SetUserProc(BillItemBrowser::GetDataForBrowser, this);
			// {
            p_def->setArray(p_list, 0, 1);
			view->setRange(p_list->getCount());
			{
				int    setup_quot_info_col = -1;
				int    ext_cost_col = -1;
				int    cost_col = -1;
				int    qtty_col = -1;
				int    phqtty_col = -1;
				int    upp_col = -1;
				int    ordqtty_col = -1;
				int    vetis_uuid_col = -1;
				for(uint i = 0; i < p_def->getCount(); i++) {
					const BroColumn & r_col = p_def->at(i);
					switch(r_col.Offs) {
						case 25: ext_cost_col = static_cast<int>(i); break;
						case  3: cost_col = static_cast<int>(i); break;
						case  2: qtty_col = static_cast<int>(i); break;
						case  7: phqtty_col = static_cast<int>(i); break;
						case 29: upp_col = static_cast<int>(i); break;
						case 30: setup_quot_info_col = static_cast<int>(i); break;
						case 31: ordqtty_col = static_cast<int>(i); break;
						case 32: vetis_uuid_col = static_cast<int>(i); break;
					}
				}
				if(Total.ExtCost != 0.0) {
					if(ext_cost_col < 0 && cost_col >= 0) {
						view->insertColumn(cost_col+1, "ExtCost", 25, T_DOUBLE, MKSFMTD(0, 2, NMBF_NOZERO), BCO_USERPROC);
					}
				}
				else {
					/* @v9.2.5 Опасная операция - может повлечь вылет сеанса - надо доработать removeColumn
					if(ext_cost_col >= 0) {
						view->removeColumn(ext_cost_col);
					}
					*/
				}
				if(qtty_col >= 0) {
					if(p_list->HasUpp && upp_col < 0)
						view->insertColumn(++qtty_col, "Package", 29, MKSTYPE(S_ZSTRING, 32), ALIGN_RIGHT, BCO_USERPROC|BCO_CAPRIGHT);
					if(phqtty_col < 0 && Total.Flags & Total.fHasIndepPhQtty)
						view->insertColumn(++qtty_col, "@phqtty", 7, T_DOUBLE, MKSFMTD(0, 6, NMBF_NOZERO|NMBF_NOTRAILZ), BCO_USERPROC|BCO_CAPRIGHT);
				}
				// @v9.1.1 {
				if(Total.OrderQtty != 0.0) {
					if(ordqtty_col < 0) {
						int   prev_col = (upp_col >= 0) ? upp_col : ((qtty_col >= 0) ? qtty_col : -1);
						if(prev_col)
							view->insertColumn(prev_col+1, "@ordered", 31, T_DOUBLE, MKSFMTD(0, 3, NMBF_NOZERO|NMBF_NOTRAILZ), BCO_USERPROC|BCO_CAPRIGHT);
					}
				}
				else {
					/* @v9.2.5 Опасная операция - может повлечь вылет сеанса - надо доработать removeColumn
					if(ordqtty_col >= 0) {
						view->removeColumn(ordqtty_col);
					}*/
				}
				// } @v9.1.1
				if(P_Pack->P_QuotSetupInfoList) {
					if(setup_quot_info_col < 0) {
						view->insertColumn(-1, "SetupQuotInfo", 30, MKSTYPE(S_ZSTRING, 16), ALIGN_CENTER, BCO_USERPROC|BCO_CAPLEFT);
					}
				}
				else {
					/* @v9.2.5 Опасная операция - может повлечь вылет сеанса - надо доработать removeColumn
					if(setup_quot_info_col >= 0) {
						view->removeColumn(setup_quot_info_col);
					}*/
				}
				// @v10.1.8 {
				if(Total.Flags & Total.fHasVetisGuid && vetis_uuid_col < 0)
					view->insertColumn(-1, "VetisCert", 32, MKSTYPE(S_ZSTRING, 48), ALIGN_LEFT, BCO_USERPROC|BCO_CAPLEFT);
				// } @v10.1.8
			}
			// @v10.6.3 {
			{
				for(uint cidx = 0; cidx < view->getDef()->getCount(); cidx++) {
					view->getDef()->at(cidx).Options |= BCO_SORTABLE;
				}
			}
			// } @v10.6.3 
			if(pos == pos_cur && org_current_pos >= 0)
				view->go(org_current_pos);
			else if(pos == pos_bottom)
				view->go(p_list->getCountI() - 2);
			else if(pos >= 0 && pos < p_list->getCountI())
				view->go(pos);
			// }
		}
	}
}

int BillItemBrowser::_moveItem(int srcRowIdx)
{
	int    ok = 1;
	uint   i = 0;
	int    s = 0;
	ReceiptTbl::Rec rr;
	PPTransferItem newitem;
	PPTransferItem * p_ti = &P_LinkPack->TI(srcRowIdx);
	if(p_ti->LotID)
		s = P_Pack->SearchLot(p_ti->LotID, &i) ? 0 : 1;
	else if(p_ti->GoodsID)
		s = P_Pack->SearchGoods(p_ti->GoodsID, &i) ? 0 : 1;
	else
		s = 0;
	if(s && !(p_ti->Flags & 0x80000000L)) {
		double qtty = 0.0, price = 0.0;
		if(!(State & stExpndOnReturn))
			for(uint j = 0, r = 1; r; j++) {
				if(p_ti->LotID)
					r = P_LinkPack->SearchLot(p_ti->LotID, &j);
				else if(p_ti->GoodsID)
					r = P_LinkPack->SearchGoods(p_ti->GoodsID, &j);
				else
					r = 0;
				if(r) {
					PPTransferItem & r_temp_ti = P_LinkPack->TI(j);
					double temp_q = fabs(r_temp_ti.Quantity_);
					if(temp_q != 0.0) {
						price = (r_temp_ti.NetPrice() * temp_q + price * qtty) / (temp_q + qtty);
						qtty += temp_q;
						r_temp_ti.Flags |= 0x80000000L;
					}
				}
			}
		newitem = *p_ti;
		THROW(newitem.Init(&P_Pack->Rec));
		if(p_ti->LotID && P_T->Rcpt.Search(p_ti->LotID, &rr) > 0 && P_T->GetLotPrices(&rr, P_Pack->Rec.Dt)) {
			newitem.Cost = R5(rr.Cost);
			newitem.Discount = R5(rr.Price) - ((price != 0.0) ? price : newitem.NetPrice());
			newitem.Price = R5(rr.Price);
		}
		newitem.Quantity_ = (qtty != 0.0) ? qtty : fabs(newitem.Quantity_);
		newitem.Flags   &= ~PPTFR_RECEIPT;
		THROW(P_Pack->InsertRow(&newitem, 0));
		if(State & stExpndOnReturn)
			p_ti->Quantity_ = 0.0;
		p_ti->Flags |= 0x80000000L;
	}
	CATCHZOK
	return ok;
}

int BillItemBrowser::_moveItem2(int srcRowIdx)
{
	int    ok = 1;
	// Признак корректировки расходного документа
	const  int is_exp_correction = BIN(P_Pack->OpTypeID == PPOPT_CORRECTION && P_LinkPack->OpTypeID == PPOPT_GOODSEXPEND);
	uint   i = 0;
	int    s = 0;
	ReceiptTbl::Rec rr;
	PPTransferItem new_ti;
	PPTransferItem & r_ti = P_LinkPack->TI(srcRowIdx);
	if(is_exp_correction) {
		uint   ex_ti_pos = 0;
		if(!P_Pack->SearchTI(r_ti.RByBill, &ex_ti_pos))
			s = 1;
	}
	else {
		if(r_ti.LotID)
			s = P_Pack->SearchLot(r_ti.LotID, &i) ? 0 : 1;
		else if(r_ti.GoodsID)
			s = P_Pack->SearchGoods(r_ti.GoodsID, &i) ? 0 : 1;
		else
			s = 0;
	}
	if(s && !(r_ti.Flags & 0x80000000L)) {
		double qtty = 0.0, price = 0.0;
		if(!(State & stExpndOnReturn) && !is_exp_correction) {
			for(uint j = 0, r = 1; r; j++) {
				if(r_ti.LotID)
					r = P_LinkPack->SearchLot(r_ti.LotID, &j);
				else if(r_ti.GoodsID)
					r = P_LinkPack->SearchGoods(r_ti.GoodsID, &j);
				else
					r = 0;
				if(r) {
					PPTransferItem & r_temp_ti = P_LinkPack->TI(j);
					const double temp_q = fabs(r_temp_ti.Quantity_);
					if(temp_q != 0.0) {
						price = (r_temp_ti.NetPrice() * temp_q + price * qtty) / (temp_q + qtty);
						qtty += temp_q;
						r_temp_ti.Flags |= 0x80000000L;
					}
				}
			}
		}
		new_ti = r_ti;
		THROW(new_ti.Init(&P_Pack->Rec, 0 /*!zeroRByBill*/));
		// @v9.4.3 {
		if(new_ti.IsCorrectionExp()) {
			PPIDArray chain_list;
			if(P_BObj->GetCorrectionBackChain(P_Pack->Rec, chain_list) > 0) {
				double org_qtty = 0.0;
				double org_price = 0.0;
				if(P_BObj->trfr->GetOriginalValuesForCorrection(new_ti, chain_list, Transfer::govcoVerifyGoods, &org_qtty, &org_price) > 0) {
					new_ti.RevalCost = org_price;
					new_ti.QuotPrice = fabs(org_qtty);
				}
				else {
					// @todo Здесь надо как-то отреагировать (в лог что-то написать или что-то в этом роде)
				}
			}
			// @v9.5.9 new_ti.TFlags |= PPTransferItem::tfForceNew;
		}
		new_ti.TFlags |= PPTransferItem::tfForceNew; // @v9.5.9 // Так как в вызове PPTransferItem::Init парам zeroRByBill == 0, данный флаг должен быть безусловно
		// } @v9.4.3
		if(r_ti.LotID && P_T->Rcpt.Search(r_ti.LotID, &rr) > 0 && P_T->GetLotPrices(&rr, P_Pack->Rec.Dt)) {
			new_ti.Cost = R5(rr.Cost);
			new_ti.Discount = R5(rr.Price) - ((price != 0.0) ? price : new_ti.NetPrice());
			new_ti.Price = R5(rr.Price);
		}
		new_ti.Quantity_ = (qtty != 0.0) ? qtty : fabs(new_ti.Quantity_);
		new_ti.Flags   &= ~PPTFR_RECEIPT;
		THROW(P_Pack->InsertRow(&new_ti, 0));
		if(State & stExpndOnReturn)
			r_ti.Quantity_ = 0.0;
		r_ti.Flags |= 0x80000000L;
	}
	CATCHZOK
	return ok;
}

int BillItemBrowser::addItemByOrder(const PPBillPacket * pOrderPack, int line)
{
	int    ok = P_BObj->InsertShipmentItemByOrder(P_Pack, pOrderPack, line, 0/*srcLotID*/, 1);
	if(ok > 0 && !P_Pack->Rec.SCardID && pOrderPack->Rec.SCardID > 0)
		P_Pack->Rec.SCardID = pOrderPack->Rec.SCardID;
	return ok;
}

int BillItemBrowser::addModifItem(int * pSign, TIDlgInitData * pInitData)
{
	int    ok = -1;
	int    sign = pSign ? *pSign : TISIGN_UNDEF;
	TDialog * dlg = 0;
	if(sign == TISIGN_UNDEF) {
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SELMODIF))));
		dlg->setCtrlUInt16(CTL_SELMODIF_WHAT, 0);
		dlg->DisableClusterItem(CTL_SELMODIF_WHAT, 2, BIN(P_Pack->Rec.Flags & BILLF_RECOMPLETE));
		if(ExecView(dlg) == cmOK) {
			switch(dlg->getCtrlUInt16(CTL_SELMODIF_WHAT)) {
				case 0: sign = TISIGN_MINUS; break;
				case 1: sign = TISIGN_PLUS; break;
				case 2: sign = TISIGN_RECOMPLETE; break;
			}
		}
	}
	if(oneof3(sign, TISIGN_MINUS, TISIGN_PLUS, TISIGN_RECOMPLETE))
		ok = 1;
	if(P_Pack->Rec.Flags & BILLF_RECOMPLETE && sign == TISIGN_PLUS) {
		PPID  recompl_lot_id = 0, lot_id = 0;
		PPTransferItem * p_ti;
		CompleteArray compl_list;
		for(uint i = 0; !recompl_lot_id && P_Pack->EnumTItems(&i, &p_ti);)
			if(p_ti->IsRecomplete())
				recompl_lot_id = p_ti->LotID;
		if(recompl_lot_id && P_BObj->GetComplete(recompl_lot_id, PPObjBill::gcfGatherSources, &compl_list) > 0) {
			CompleteItem item;
			if(pInitData->LotID) {
				THROW_PP(compl_list.SearchLotID(pInitData->LotID, 0, &item), PPERR_NONCOMPLCOMPONENT);
				pInitData->Quantity = item.Qtty;
				ok = 2;
			}
			else if(pInitData->GoodsID) {
				THROW_PP(compl_list.SearchGoodsID(pInitData->GoodsID, 0, &item), PPERR_NONCOMPLCOMPONENT);
				pInitData->LotID = item.LotID;
				pInitData->Quantity = item.Qtty;
				ok = 2;
			}
			else if(P_BObj->ViewLotComplete(recompl_lot_id, &lot_id) > 0) {
				pInitData->LotID = lot_id;
				THROW_PP(compl_list.SearchLotID(pInitData->LotID, 0, &item), PPERR_NONCOMPLCOMPONENT);
				pInitData->GoodsID = item.GoodsID;
				pInitData->Quantity = item.Qtty;
				ok = 2;
			}
			else
				ok = -1;
		}
	}
	CATCHZOK
	delete dlg;
	ASSIGN_PTR(pSign, sign);
	return ok;
}

void BillItemBrowser::addItem(int fromOrder, TIDlgInitData * pInitData, int sign)
{
	int    r, lc, i;
	BillItemBrowser * brw = 0;
	if(GetOpType(P_Pack->Rec.OpID) == PPOPT_GOODSEXPEND && CheckOpFlags(P_Pack->Rec.OpID, OPKF_PCKGMOUNTING) && !P_Pckg)
		selectPckg(0);
	else if(fromOrder) {
		if(CheckOpFlags(P_Pack->Rec.OpID, OPKF_ONORDER)) {
			THROW(r = selectOrder());
			if(r > 0 && OrderBillID) {
				uint   res_id = BROWSER_ID(ORDGOODSITEM_W);
				THROW_MEM(P_LinkPack = new PPBillPacket);
				THROW(P_BObj->ExtractPacket(OrderBillID, P_LinkPack));
				THROW_MEM(brw = new BillItemBrowser(res_id, P_BObj, P_LinkPack, P_Pack, -1, 1, 0));
				brw->changeBounds(TRect(0, 0, 80, 10));
				brw->State |= BillItemBrowser::stOrderSelector;
				while(ExecView(brw) == cmOK) {
					r  = brw->AsSelector;
					brw->AsSelector = 1;
					lc = P_LinkPack->GetTCount();
					int    gr = 0;
					if(r >= 0 && r < lc) {
						THROW(r = addItemByOrder(P_LinkPack, r));
						if(r > 0)
							gr = 1;
					}
					else if(r == -1) {
						for(i = 0; i < lc; i++) {
							THROW(r = addItemByOrder(P_LinkPack, i));
							if(r > 0)
								gr = 1;
						}
					}
					if(gr) {
						brw->update(pos_cur);
						brw->State |= BillItemBrowser::stIsModified;
						update(pos_bottom);
					}
				}
				ZDELETE(P_LinkPack);
			}
		}
	}
	//
	// Возврат товара
	//
	else if(State & stUseLinkSelection && P_LinkPack) {
		uint   res_id = (LConfig.Flags & CFGFLG_SHOWPHQTTY) ? BROWSER_ID(GOODSITEMPH_W) : BROWSER_ID(GOODSITEM_W);
		THROW_MEM(brw = new BillItemBrowser(res_id, P_BObj, P_LinkPack, 0, -1, 1, 0));
		brw->changeBounds(TRect(0, 0, 80, 10));
		while(ExecView(brw) == cmOK) {
			r = brw->AsSelector;
			brw->AsSelector = 1;
			lc = P_LinkPack->GetTCount();
			if(r >= 0 && r < lc) {
				THROW(_moveItem2(r));
				brw->update(pos_cur);
				State |= stIsModified;
				update(pos_bottom);
			}
			else if(r == -1) {
				for(r = 0; r < lc; r++)
					THROW(_moveItem2(r));
				State |= stIsModified;
				update(pos_bottom);
				break;
			}
		}
	}
	else if(!AsSelector) {
		int    skip = 0;
		TIDlgInitData temp_tidi;
		if(NewGoodsGrpID && !pInitData) {
			temp_tidi.GoodsGrpID = NewGoodsGrpID;
			pInitData = &temp_tidi;
		}
		if(P_Pack->OpTypeID == PPOPT_GOODSMODIF && sign == 0) {
			RVALUEPTR(temp_tidi, pInitData);
			sign = temp_tidi.GetTiSign();
			THROW(r = addModifItem(&sign, &temp_tidi));
			if(r < 0)
				skip = 1;
			else if(r == 2) {
				if(pInitData)
					*pInitData = temp_tidi;
				else
					pInitData = &temp_tidi;
			}
		}
		if(!skip && EditTransferItem(P_Pack, -1, pInitData, 0, sign) == cmOK) {
			const  uint pos = P_Pack->GetTCount() - 1;
			PPTransferItem * p_ti = &P_Pack->TI(pos);
			if(P_Pckg) {
				p_ti->Flags |= PPTFR_PCKGGEN;
		        if(P_Pckg->Flags & PCKGF_MOUNTED && p_ti->Flags & PPTFR_MINUS)
		            p_ti->Flags |= PPTFR_UNITEINTR;
				P_Pckg->AddItem(0, pos);
			}
			if(P_Pack->OpTypeID == PPOPT_GOODSMODIF) {
				if(!p_ti->IsRecomplete()) {
					PPGoodsStruc::Ident gs_ident(p_ti->GoodsID, -1L, GSF_PARTITIAL, P_Pack->Rec.Dt);
					if(sign > 0)
						gs_ident.AndFlags = (GSF_COMPL);
					else if(sign < 0)
						gs_ident.AndFlags = (GSF_DECOMPL);
					TSCollection <PPGoodsStruc> gs_list;
					if(GObj.LoadGoodsStruc(&gs_ident, gs_list) > 0) {
						if(gs_list.getCount() == 1) {
							const uint gs_pos = 0;
							PPGoodsStruc * p_gs = gs_list.at(gs_pos);
							THROW(P_Pack->InsertComplete(gs_list.at(gs_pos), pos, 0, 0));
						}
						else if(gs_list.getCount() > 1) {
							int    do_select = 1;
							SString source_serial;
							P_Pack->LTagL.GetTagStr(pos, PPTAG_LOT_SOURCESERIAL, source_serial);
							if(source_serial.NotEmptyS()) {
								PPIDArray src_lot_list;
								LongArray potential_gs_pos_list;
								P_BObj->SearchLotsBySerialExactly(source_serial, &src_lot_list);
								for(uint llidx = 0; llidx < src_lot_list.getCount(); llidx++) {
									const PPID src_lot_id = src_lot_list.get(llidx);
									ReceiptTbl::Rec src_lot_rec;
									if(P_BObj->trfr->Rcpt.Search(src_lot_id, &src_lot_rec) > 0 && src_lot_rec.Rest > 0.0) {
										for(uint sidx = 0; sidx < gs_list.getCount(); sidx++) {
											const PPGoodsStruc * p_gs = gs_list.at(sidx);
											if(p_gs->HasGoods(src_lot_rec.GoodsID)) {
												potential_gs_pos_list.add(sidx+1);
											}
										}
									}
								}
								potential_gs_pos_list.sortAndUndup();
								if(potential_gs_pos_list.getCount() == 1) {
									const uint gs_pos = (uint)(potential_gs_pos_list.get(0)-1);
									THROW(P_Pack->InsertComplete(gs_list.at(gs_pos), pos, 0, 0));
									do_select = 0;
								}
							}
							if(do_select) {
								PPObjGoodsStruc gs_obj;
								uint   gs_pos = 0;
								if(gs_obj.SelectorDialog(gs_list, &gs_pos) > 0) {
									if(gs_pos < gs_list.getCount()) {
										THROW(P_Pack->InsertComplete(gs_list.at(gs_pos), pos, 0, 0));
									}
								}
							}
						}
					}
				}
				P_Pack->CalcModifCost();
			}
			if(P_Pack->ProcessFlags & PPBillPacket::pfHasExtCost)
				P_Pack->InitAmounts(0);
			else if(P_Pack->UsesDistribCost()) {
				Goods2Tbl::Rec goods_rec;
				if(GObj.Fetch(p_ti->GoodsID, &goods_rec) > 0 && goods_rec.Flags & GF_ODD) {
					P_Pack->InitAmounts(0);
				}
			}
			State |= stIsModified;
			update(pos_bottom);
			UpdatePriceDevList(static_cast<long>(pos), +1);
		}
	}
	CATCH
		if(fromOrder)
			ZDELETE(P_LinkPack);
		PPError();
	ENDCATCH
	delete brw;
}

int BillItemBrowser::ConvertBillToBasket()
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	param.SelPrice = IsSellingOp(P_Pack->Rec.OpID) ? 2 : 1;
	THROW(r = GetBasketByDialog(&param, "BillItemBrowser"));
	if(r > 0) {
		PPTransferItem * p_item = 0;
		PPWait(1);
		for(uint i = 0; P_Pack->EnumTItems(&i, &p_item);) {
			ILTI   ilti(p_item);
			ilti.GoodsID = labs(ilti.GoodsID);
			if(param.SelPrice == 1)
				ilti.Price = p_item->Cost;
			else if(param.SelPrice == 2)
				ilti.Price = p_item->Price;
			else if(param.SelPrice == 3)
				ilti.Price = p_item->NetPrice();
			ilti.Quantity = fabs(ilti.Quantity);
			THROW(param.Pack.AddItem(&ilti, 0, param.SelReplace));
		}
		PPWait(0);
		THROW(GoodsBasketDialog(param, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int BillItemBrowser::ConvertBasketToBill()
{
	int    ok = -1, r = 0;
	PPObjGoodsBasket gb_obj;
	PPBasketCombine basket;
	THROW(r = gb_obj.SelectBasket(basket));
	if(r > 0) {
		THROW(r = P_BObj->ConvertBasket(&basket.Pack, P_Pack));
		if(r > 0) {
			State |= stIsModified;
			update(pos_bottom);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

void BillItemBrowser::viewPckgItems(int activateNewRow)
{
	int    c = getCurItemPos(); // @v10.6.3 int16-->int
	const  int sav_brw_pos = view->getDef()->_curItem(); // @v10.6.3 int16-->int
	if(c >= 0) {
		const PPTransferItem & r_ti = P_Pack->ConstTI(c);
		if(r_ti.Flags & PPTFR_PCKG && P_Pack->P_PckgList && P_Pack->P_PckgList->GetByIdx(c)) {
			uint   res_id = (LConfig.Flags & CFGFLG_SHOWPHQTTY) ? BROWSER_ID(GOODSITEMPH_W) : BROWSER_ID(GOODSITEM_W);
			int    edit_mode = (r_ti.Flags & PPTFR_RECEIPT) ? EditMode : 2;
			BillItemBrowser * brw = new BillItemBrowser(res_id, P_BObj, P_Pack, 0, c, 0, edit_mode);
			if(activateNewRow)
				brw->State |= BillItemBrowser::stActivateNewRow;
			ExecViewAndDestroy(brw);
			brw = 0;
			if(r_ti.Flags & PPTFR_RECEIPT) {
				P_Pack->CalcPckgTotals();
				update(sav_brw_pos);
			}
		}
	}
}

void BillItemBrowser::GetMinMaxQtty(uint itemPos, RealRange & rBounds) const
{
	rBounds.Z();
	const PPTransferItem & r_ti = P_Pack->ConstTI(itemPos);
	if(P_Pack->OpTypeID == PPOPT_GOODSEXPEND) {
		if(P_Pack->Rec.ID) {
			BillTbl::Rec bill_rec;
			for(DateIter di; P_BObj->P_Tbl->EnumLinks(P_Pack->Rec.ID, &di, BLNK_RETURN, &bill_rec) > 0;)
				P_T->SubtractBillQtty(bill_rec.ID, r_ti.LotID, &rBounds.low);
		}
	}
	else if(P_Pack->OpTypeID == PPOPT_GOODSRETURN && P_LinkPack)
		for(uint i = 0; P_LinkPack->SearchLot(r_ti.LotID, &i); i++)
			if(!(State & stExpndOnReturn))
				rBounds.upp += P_LinkPack->ConstTI(i).Quantity_;
}

void BillItemBrowser::editItem()
{
	int    c = getCurItemPos(); // @v10.6.3 int16-->int
	uint   i;
	double rest;
	int    valid_data = 0;
	PPTransferItem * p_ti;
	if(c >= 0) {
		p_ti = &P_Pack->TI(static_cast<uint>(c));
		if(p_ti->Flags & PPTFR_PCKG) {
			if(p_ti->Flags & PPTFR_RECEIPT || IsIntrExpndOp(P_Pack->Rec.OpID)) {
				LPackage * p_pckg = P_Pack->P_PckgList ? P_Pack->P_PckgList->GetByIdx(c) : 0;
				if(p_pckg) {
					if(editPackageData(p_pckg) > 0)
						update(pos_cur);
				}
			}
			else
				viewPckgItems(0);
		}
		else if(!AsSelector) {
			TIDlgInitData tidi;
			GetMinMaxQtty((uint)c, tidi.QttyBounds);
			while(!valid_data && EditTransferItem(P_Pack, static_cast<int>(c), &tidi, 0) == cmOK) {
				valid_data = 1;
				//
				// Проверка на то, чтобы возврат не превышал взятое количество
				//
				if(P_Pack->OpTypeID == PPOPT_GOODSRETURN && P_LinkPack) {
					double expend = 0.0;
					for(i = 0, p_ti = &P_Pack->TI(static_cast<uint>(c)); P_LinkPack->SearchLot(p_ti->LotID, &i); i++)
						if(State & stExpndOnReturn)
							if(!P_Pack->BoundsByLot(p_ti->LotID, 0, -1, &rest, 0))
								valid_data = PPErrorZ();
							else if(rest < 0)
								valid_data = (PPError(PPERR_RETQTTY, 0), 0);
							else {
								p_ti = &P_LinkPack->TI(i);
								p_ti->Quantity_ = rest;
								SETFLAG(p_ti->Flags, 0x80000000L, rest == 0.0);
							}
						else
							expend += P_LinkPack->ConstTI(i).Quantity_;
					if(!(State & stExpndOnReturn) && fabs(expend) < fabs(p_ti->Quantity_))
						valid_data = (PPError(PPERR_RETQTTY, 0), 0);
				}
				if(valid_data) {
					State |= stIsModified;
					if(P_Pack->OpTypeID == PPOPT_GOODSMODIF)
						P_Pack->CalcModifCost();
					if(P_Pack->ProcessFlags & PPBillPacket::pfHasExtCost)
						P_Pack->InitAmounts(0);
					update(pos_cur);
					UpdatePriceDevList(c, 0);
				}
			}
		}
		//
		// Если окно работает как селектор и является модальным,
		// то переменной AsSelector присваиваем номер выбранного
		// элемента и закрываем окно.
		//
		else if(IsInState(sfModal)) {
			AsSelector = c;
			endModal(cmOK);
			return;
		}
	}
}

int BillItemBrowser::checkForward(const PPTransferItem * pTI, LDATE dt, int reverse)
{
	int    ok = 0;
	if(P_T->SearchByBill(pTI->BillID, reverse, pTI->RByBill, 0) > 0) {
		double neck    = -fabs(pTI->Quantity_);
		double ph_neck = -fabs(R6(pTI->WtQtty));
		if(P_T->UpdateForward(P_T->data.LotID, dt, P_T->data.OprNo, 1, &neck, &ph_neck) > 0)
			ok = 1;
	}
	else
		ok = 1;
	return ok;
}

void BillItemBrowser::delItem()
{
	PPID   id, goods_id;
	uint   i;
	double tmp;
	if(!AsSelector) {
		int    c = getCurItemPos(); // @v10.6.3 int16-->int
		int    cur_view_pos = view->getDef()->_curItem(); // @v10.6.3 int16-->int
		if(c >= 0 && (!(LConfig.Flags & CFGFLG_CONFGBROWRMV) || CONFIRM(PPCFM_DELETE))) {
			//
			// Проверяем, можно ли будет провести документ с заявленным удалением
			//
			PPTransferItem * p_ti = &P_Pack->TI(static_cast<uint>(c));
			double qtty = fabs(p_ti->Quantity_);
			if(P_Pack->OpTypeID != PPOPT_GOODSRETURN && P_Pack->Rec.ID && p_ti->LotID) {
				BillTbl::Rec bill_rec;
				for(DateIter diter; P_BObj->P_Tbl->EnumLinks(P_Pack->Rec.ID, &diter, BLNK_RETURN, &bill_rec) > 0;) {
					PPBillPacket ret_pack;
					THROW(P_BObj->ExtractPacket(bill_rec.ID, &ret_pack) > 0);
					THROW_PP(ret_pack.SearchLot(p_ti->LotID, &(i = 0)) <= 0, PPERR_DELLOTWRET);
				}
			}
			if(p_ti->BillID && p_ti->RByBill) {
				if(P_Pack->Rec.Flags & BILLF_GRECEIPT && !(p_ti->Flags & PPTFR_UNLIM)) {
					THROW(checkForward(p_ti, P_Pack->Rec.Dt, 0));
				}
				//
				// Если документ передачи по межскладу, то проверяем лот,
				// созданный зеркальной проводкой.
				//
				else if(IsIntrExpndOp(P_Pack->Rec.OpID)) {
					THROW(checkForward(p_ti, P_Pack->Rec.Dt, 1));
				}
			}
			//
			// Если есть связанный документ то отмечаем в нем удаление
			//
			if(P_LinkPack) {
				goods_id = p_ti->GoodsID;
				for(i = 0, id = p_ti->LotID; P_LinkPack->EnumTItems(&i, &p_ti);)
					if((id && p_ti->LotID == id) || (!id && p_ti->GoodsID == goods_id)) {
						p_ti->Flags &= ~0x80000000L;
						if(State & stExpndOnReturn) {
							THROW(P_Pack->BoundsByLot(id, 0, -1, &tmp, 0));
							p_ti->Quantity_ += qtty;
							break;
						}
					}
			}
			//
			// Если удаляемая строка имеет теневую строку, на которую не ссылается //
			// больше ни одна строка, то теневую строку удаляем
			//
			p_ti = &P_Pack->TI(static_cast<uint>(c));
			id = (p_ti->Flags & PPTFR_ONORDER && P_Pack->P_ShLots) ? p_ti->OrdLotID : 0; // @ordlotid
			if(p_ti->Flags & PPTFR_AUTOCOMPL)
				P_Pack->RemoveAutoComplRow((uint)c);
			P_Pack->RemoveRow((int)c);
			if(id && P_Pack->SearchShLot(id, &(i = 0))) {
				P_Pack->CalcShadowQuantity(id, &tmp);
				if(tmp == 0.0)
					P_Pack->P_ShLots->atFree(i);
				else
					P_Pack->P_ShLots->at(i).Quantity_ = tmp;
			}
			State |= stIsModified;
			if(P_Pack->OpTypeID == PPOPT_GOODSMODIF)
				P_Pack->CalcModifCost();
			if(P_Pack->ProcessFlags & PPBillPacket::pfHasExtCost)
				P_Pack->InitAmounts(0);
			update(cur_view_pos);
			THROW(UpdatePriceDevList(c, -1));
			ProblemsList.Remove(c);
		}
	}
	CATCH
		PPError();
	ENDCATCH
}

void BillItemBrowser::selectPckg(PPID goodsID)
{
	int    r;
	if(GetOpType(P_Pack->Rec.OpID) == PPOPT_GOODSEXPEND && CheckOpFlags(P_Pack->Rec.OpID, OPKF_PCKGMOUNTING)) {
		LPackage pckg;
		P_BObj->InitPckg(&pckg);
		pckg.Flags |= PCKGF_MOUNTED;
		if((r = editPackageData(&pckg)) > 0)
			if(P_Pack->AddPckg(&pckg)) {
				update(pos_bottom);
				if(oneof2(r, 2, 3))
					viewPckgItems(BIN(r == 3));
			}
			else
				PPError();
	}
	else {
		PckgFilt flt;
		MEMSZERO(flt);
		flt.GoodsID = goodsID;
		flt.LocID   = P_Pack->Rec.LocID;
		for(PPID pckg_id = 0; P_BObj->SelectPckg(&flt, &pckg_id) > 0;) {
			if(P_BObj->AddPckgToBillPacket(pckg_id, P_Pack)) {
				update(pos_bottom);
				if(flt.GoodsID)
					break;
			}
			else
				PPError();
		}
	}
}

class LotXCodeListDialog_Base : public PPListDialog {
public:
	LotXCodeListDialog_Base(uint dlgId, uint listCtlId, const PPBillPacket * pPack, int rowIdx, long flags) : PPListDialog(dlgId, listCtlId, flags),
		P_Pack(pPack), P_LotXcT(BillObj->P_LotXcT), RowIdx(rowIdx)
	{
		SetCtrlFont(CTL_LOTXCLIST_LIST, "Courier New", 14);
	}
	int    setDTS(const PPLotExtCodeContainer * pData)
	{
		RVALUEPTR(Data, pData);
		updateList(-1);
		return 1;
	}
	int    getDTS(PPLotExtCodeContainer * pData)
	{
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	virtual int  pasteFromClipboardAll(){return -1;} // @erik v10.5.2
protected:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmCopyToClipboard)) {
			int   row_idx = 0;
			uint  inner_idx = 0;
			long  cur_pos = 0;
			long  cur_id = 0;
			SString text_buf;
			if(getCurItem(&cur_pos, &cur_id)) {
				if(getText(cur_pos, text_buf)) {
					SClipboard::Copy_Text(text_buf, text_buf.Len());
				}
				/*if(GetItem(cur_pos, &row_idx, &inner_idx)) {
					PPLotExtCodeContainer::Item2 item;
					if(Data.GetByIdx(inner_idx, item)) {
						SClipboard::Copy_Text(item.Num, item.Num.Len());
					}
				}*/
			}
		}
		else if(event.isCmd(cmCopyToClipboardAll)) {
			SString buf_to_copy;
			SString temp_buf;
			StringSet ss;
			PPLotExtCodeContainer::MarkSet ms;
			PPLotExtCodeContainer::MarkSet::Entry msentry;
			Data.Get(RowIdx, 0, ms);
			for(uint boxidx = 0; boxidx < ms.GetCount(); boxidx++) {
				if(ms.GetByIdx(boxidx, msentry) && msentry.Flags & PPLotExtCodeContainer::fBox) {
					temp_buf.Z().Cat("box").CatDiv(':', 2).Cat(msentry.Num);
					buf_to_copy.Cat(temp_buf).CRB();
					ms.GetByBoxID(msentry.BoxID, ss);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						//temp_buf.Insert(0, " "); @erik	 v10.4.9
						buf_to_copy.Cat(temp_buf).CRB();
					}
				}
			}
			ms.GetByBoxID(0, ss);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				buf_to_copy.Cat(temp_buf).CRB();
			}
			if(buf_to_copy.NotEmpty()) {
				SClipboard::Copy_Text(buf_to_copy, buf_to_copy.Len());
			}
		}
		// @erik v10.5.2 {
		else if(event.isCmd(cmPasteFromClipboardAll)){
			pasteFromClipboardAll();
		}
		// } @erik
		else
			return;
		clearEvent(event);
	}
	int    GetItem(long pos, int * pRowIdx, uint * pInnerIdx)
	{
		int    ok = 0;
		int   row_idx = 0;
		uint  inner_idx = 0;
		SString text_buf;
		if(getText(pos, text_buf)) {
			if(text_buf.CmpPrefix("box:", 1) == 0)
				text_buf.ShiftLeft(4);
			text_buf.Strip();
			if(Data.Search(text_buf, &row_idx, &inner_idx)) {
				ASSIGN_PTR(pRowIdx, row_idx);
				ASSIGN_PTR(pInnerIdx, inner_idx);
				ok = 1;
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		int    row_idx = 0;
		uint   inner_idx = 0;
		if(GetItem(pos, &row_idx, &inner_idx)) {
			if(Data.Delete(row_idx, inner_idx))
				ok = 1;
		}
		return ok;
	}
	const  int RowIdx;
	const  PPBillPacket * P_Pack;
	PPLotExtCodeContainer Data;
	LotExtCodeCore * P_LotXcT;
};

class ValidateLotXCodeListDialog : public LotXCodeListDialog_Base {
public:
	ValidateLotXCodeListDialog(const PPBillPacket * pPack) : LotXCodeListDialog_Base(DLG_LOTXCCKLIST, CTL_LOTXCCKLIST_LIST, pPack, -1, fOmitSearchByFirstChar|fOwnerDraw),
		FontId(0), CStyleId(0), ViewFlags(0)
	{
		AddClusterAssoc(CTL_LOTXCCKLIST_FLAGS, 0, vfShowUncheckedItems);
		SetClusterData(CTL_LOTXCCKLIST_FLAGS, 0);
	}
private:
	DECL_HANDLE_EVENT
	{
		if(TVKEYDOWN) {
			uchar  c = TVCHR;
			if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || isdec(c)) {
				LotExtCodeTbl::Rec rec;
				PPLotExtCodeContainer::MarkSet set;
				if(EditItemDialog(rec, c, set) > 0) {
					if(Data.AddValidation(set))
						updateList(-1);
					else
						PPError();
				}
				clearEvent(event);
			}
		}
		LotXCodeListDialog_Base::handleEvent(event);
		if(event.isClusterClk(CTL_LOTXCCKLIST_FLAGS)) {
			const long preserve_view_flags = ViewFlags;
			GetClusterData(CTL_LOTXCCKLIST_FLAGS, &ViewFlags);
			if(ViewFlags != preserve_view_flags) {
				updateList(-1);
			}
		}
		else if(event.isCmd(cmDrawItem)) {
			TDrawItemData * p_draw_item = static_cast<TDrawItemData *>(TVINFOPTR);
			if(p_draw_item && p_draw_item->P_View) {
				PPID   list_ctrl_id = p_draw_item->P_View->GetId();
				if(list_ctrl_id == ctlList) {
					SmartListBox * p_lbx = static_cast<SmartListBox *>(p_draw_item->P_View);
					const FRect  rect_elem = p_draw_item->ItemRect;
					SPaintToolBox & r_tb = APPL->GetUiToolBox();
					TCanvas2 canv(r_tb, p_draw_item->H_DC);
					if(p_draw_item->ItemAction & TDrawItemData::iaBackground) {
						canv.Rect(rect_elem, 0, TProgram::tbiListBkgBrush);
						p_draw_item->ItemAction = 0; // Мы перерисовали фон
					}
					else {
						SString code_buf;
						SString box_code;
						p_lbx->getText(static_cast<long>(p_draw_item->ItemData), code_buf);
						int    err = 0;
						int    row_idx = -1;
						int    brush_id = TProgram::tbiListBkgBrush;
						if(code_buf.NotEmpty()) {
							uint  inner_idx = 0;
							if(!(ViewFlags & vfShowUncheckedItems) || Data.Search(code_buf, &row_idx, &inner_idx)) {
								int    vcr = P_Pack->XcL.ValidateCode(code_buf, 0, &err, &row_idx, &box_code);
								if(!vcr) {
									if(err == 2) // марка не найдена
										brush_id = TProgram::tbiInvalInpBrush;
									else if(err == 3) // не та коробка
										brush_id = TProgram::tbiInvalInp2Brush;
									else
										brush_id = TProgram::tbiInvalInp3Brush;
								}
							}
							else {
								brush_id = TProgram::tbiInvalInp3Brush;
							}
						}
						if(brush_id)
							canv.Rect(rect_elem, TProgram::tbiListBkgPen, brush_id);
						{
							int local_pen_id = 0;
							if(p_draw_item->ItemState & ODS_FOCUS)
								local_pen_id = TProgram::tbiListFocPen;
							else if(p_draw_item->ItemState & ODS_SELECTED)
								local_pen_id = TProgram::tbiListSelPen;
							/*else
								local_pen_id = TProgram::tbiListBkgPen;*/
							if(local_pen_id) {
								FRect rect_elem_f(rect_elem);
								rect_elem_f.Grow(-1.0f, -1.0f);
								canv.Rect(rect_elem_f, local_pen_id, 0/*brush*/);
							}
						}
						if(code_buf.NotEmpty()) {
							if(FontId <= 0) {
								HFONT  hf = reinterpret_cast<HFONT>(SendMessage(p_draw_item->H_Item, WM_GETFONT, 0, 0));
								LOGFONT f;
								if(hf && ::GetObject(hf, sizeof(f), &f)) {
									SFontDescr fd(0, 0, 0);
									f.lfHeight = 12;
									fd.SetLogFont(&f);
									//fd.Size = (int16)MulDiv(fd.Size, 72, GetDeviceCaps(canv, LOGPIXELSY));
									FontId = r_tb.CreateFont_(0, fd.Face, fd.Size, fd.Flags);
								}
							}
							if(FontId) {
								SDrawContext dctx = canv;
								if(CStyleId <= 0) {
									int    tool_text_brush_id = 0; //SPaintToolBox::rbr3DFace;
									CStyleId = r_tb.CreateCStyle(0, FontId, TProgram::tbiBlackPen, tool_text_brush_id);
								}
								SParaDescr pd;
								int    tid_para = r_tb.CreateParagraph(0, &pd);
								STextLayout tlo;
								tlo.SetText(code_buf);
								tlo.SetOptions(STextLayout::fOneLine|STextLayout::fVCenter, tid_para, CStyleId);
								tlo.SetBounds(rect_elem);
								tlo.Arrange(dctx, r_tb);
								canv.DrawTextLayout(&tlo);
							}
						}
					}
				}
				else
					p_draw_item->ItemAction = 0; // Список не активен - строку не рисуем
			}
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList()
	{
		int    ok = 1;
		uint   mark_count = 0;
		uint   box_count = 0;
		uint   org_box_count = 0;
		uint   org_mark_count = 0;
		SString temp_buf;
		SString box_num;
		StringSet ss;
		PPLotExtCodeContainer::MarkSet ms;
		PPLotExtCodeContainer::MarkSet::Entry msentry;
		LongArray idx_list;
		Data.Get(RowIdx, &idx_list, ms);
		long list_pos_idx = 0;
		for(uint boxidx = 0; boxidx < ms.GetCount(); boxidx++) {
			if(ms.GetByIdx(boxidx, msentry)) {
				if(msentry.Flags & PPLotExtCodeContainer::fBox) {
					box_count++;
					temp_buf.Z().Cat("box").CatDiv(':', 2).Cat(msentry.Num);
					++list_pos_idx;
					THROW(addStringToList(list_pos_idx, temp_buf));
					ms.GetByBoxID(msentry.BoxID, ss);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						temp_buf.Insert(0, " ");
						++list_pos_idx;
						THROW(addStringToList(list_pos_idx, temp_buf));
					}
				}
				else
					mark_count++;
			}
		}
		{
			ms.GetByBoxID(0, ss);
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				++list_pos_idx;
				THROW(addStringToList(list_pos_idx, temp_buf));
			}
		}
		{
			uint oc = P_Pack->XcL.GetCount();
			PPLotExtCodeContainer::Item2 oi;
			for(uint i = 0; i < oc; i++) {
				if(P_Pack->XcL.GetByIdx(i, oi)) {
					if(oi.Flags & PPLotExtCodeContainer::fBox)
						org_box_count++;
					else
						org_mark_count++;
					if(ViewFlags & vfShowUncheckedItems) {
						int   row_idx = 0;
						uint  inner_idx = 0;
						if(!Data.Search(oi.Num, &row_idx, &inner_idx)) {
							++list_pos_idx;
							THROW(addStringToList(list_pos_idx, oi.Num));
						}
					}
				}
			}
		}
		{
			temp_buf.Z();
			if(mark_count)
				temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("Marks", mark_count);
			if(box_count)
				temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("Boxes", box_count);
			if(org_mark_count || org_box_count)
				temp_buf.CatDivIfNotEmpty('-', 1);
			if(org_mark_count)
				temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("OriginalMarks", org_mark_count);
			if(org_box_count)
				temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("OriginalBoxes", org_box_count);
			setStaticText(CTL_LOTXCLIST_INFO, temp_buf);
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		PPLotExtCodeContainer::MarkSet set;
		LotExtCodeTbl::Rec rec;
		MEMSZERO(rec);
		rec.BillID = P_Pack->Rec.ID;
		rec.RByBill = RowIdx;
		while(ok < 0 && EditItemDialog(rec, 0, set) > 0) {
			if(Data.AddValidation(set)) {
				ok = 1;
			}
			else
				PPError();
		}
		return ok;
	}
	int    SLAPI EditItemDialog(LotExtCodeTbl::Rec & rRec, char firstChar, PPLotExtCodeContainer::MarkSet & rSet)
	{
		int    ok = -1;
		uint   sel = 0;
		TDialog * dlg = new TDialog(DLG_LOTEXTCODE);
		SString temp_buf, info_buf;
		SString mark_buf;
		const PPTransferItem * p_ti = (RowIdx > 0 && RowIdx <= (int)P_Pack->GetTCount()) ? &P_Pack->ConstTI(RowIdx-1) : 0;
		THROW(CheckDialogPtr(&dlg));
		if(firstChar)
			temp_buf.Z().CatChar(firstChar);
		else
			(temp_buf = rRec.Code).Strip();
		dlg->setCtrlString(CTL_LOTEXTCODE_CODE, temp_buf);
		if(firstChar) {
			TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_LOTEXTCODE_CODE));
			CALLPTRMEMB(il, disableDeleteSelection(1));
		}
		dlg->setStaticText(CTL_LOTEXTCODE_INFO, info_buf);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlString(sel = CTL_LOTEXTCODE_CODE, temp_buf);
			if(!temp_buf.NotEmptyS())
				PPErrorByDialog(dlg, sel, PPERR_CODENEEDED);
			else if(temp_buf.Len() >= sizeof(rRec.Code)) {
				PPSetError(PPERR_CODETOOLONG, (long)(sizeof(rRec.Code)-1));
				PPErrorByDialog(dlg, sel);
			}
			else if(!PrcssrAlcReport::IsEgaisMark(temp_buf, &mark_buf)) {
				if(P_LotXcT) {
					//if(P_LotXcT->FindMarkToTransfer(temp_buf, goods_id, lot_id, rSet) > 0)
						ok = 1;
					//else
						//PPErrorByDialog(dlg, sel);
				}
				else {
					PPSetError(PPERR_TEXTISNTEGAISMARK, temp_buf);
					PPErrorByDialog(dlg, sel);
				}
			}
			else {
				rSet.AddNum(0, mark_buf, 1);
				STRNSCPY(rRec.Code, mark_buf);
				ok = 1;
			}
		}
		CATCHZOKPPERR
		delete dlg;
		return ok;
	}
	//
	// функция вставки всех марок из буфера(если они там есть)
	//
	virtual int pasteFromClipboardAll()
	{
		int ok = 0;
		uint   sel = 0;
		//TDialog * dlg = new TDialog(DLG_LOTEXTCODE);
		SString temp_buf;
		StringSet ss;
		PPLotExtCodeContainer::MarkSet set;
		LotExtCodeTbl::Rec rec;
		MEMSZERO(rec);
		rec.BillID = P_Pack->Rec.ID;
		rec.RByBill = RowIdx;
		const PPTransferItem * p_ti = (RowIdx > 0 && RowIdx <= (int)P_Pack->GetTCount()) ? &P_Pack->ConstTI(RowIdx - 1) : 0;
		const int  do_check = (P_Pack->IsDraft() || (!p_ti || p_ti->Flags & PPTFR_RECEIPT)) ? 0 : 1;
		const PPID goods_id = (do_check && p_ti) ? labs(p_ti->GoodsID) : 0;
		const PPID lot_id = (do_check && p_ti) ? p_ti->LotID : 0;
		SStringU buf_from_copy;
		SClipboard::Past_Text(buf_from_copy);
		buf_from_copy.CopyToUtf8(temp_buf, 0);
		if(temp_buf.Tokenize("\xD\xA", ss)) {
			temp_buf.Z();
			for(uint ssp = 0; ss.get(&ssp, temp_buf); temp_buf.Z(), set.Clear()) {
				if(!temp_buf.NotEmptyS()) {
					//PPErrorByDialog(dlg, sel, PPERR_CODENEEDED);
					continue;
				}
				else if(temp_buf.Len() >= sizeof(rec.Code)) {
					//PPSetError(PPERR_CODETOOLONG, (long)(sizeof(rec.Code) - 1));
					//PPErrorByDialog(dlg, sel);
					continue;
				}
				else {
					SString mark_buf = temp_buf;
					const int iemr = PrcssrAlcReport::IsEgaisMark(mark_buf, 0);
					if(!iemr) {
						if(P_LotXcT) {
							if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, set) <= 0) {
								//PPErrorByDialog(dlg, sel);
								continue;
							}
						}
						else {
							//PPSetError(PPERR_TEXTISNTEGAISMARK, mark_buf);
							//PPErrorByDialog(dlg, sel);
							continue;
						}
					}
					else if(do_check && P_LotXcT) {
						if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, set) > 0) {
							STRNSCPY(rec.Code, mark_buf);
						}
						else {
							//PPErrorByDialog(dlg, sel);
							continue;
						}
					}
					else {
						set.AddNum(0, mark_buf, 1);
						STRNSCPY(rec.Code, mark_buf);
					}
				}
				if(Data.AddValidation(set)) {
					ok++;
				}
				//else {
				//	PPError();
				//}
			}
		}
		updateList(0);
		return ok;
	}

	int    FontId;
	int    CStyleId;
	enum {
		vfShowUncheckedItems = 0x0001
	};
	long   ViewFlags;
};

int SLAPI BillItemBrowser::ValidateExtCodeList()
{
	int    ok = -1;
	ValidateLotXCodeListDialog * dlg = (P_Pack && P_Pack->XcL.GetCount()) ? new ValidateLotXCodeListDialog(P_Pack) : 0;
	if(dlg && CheckDialogPtr(&dlg)) {
		dlg->setDTS(&P_Pack->_VXcL);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&P_Pack->_VXcL);
			ok = 1;
		}
	}
	delete dlg;
	return ok;
}

int SLAPI BillItemBrowser::EditExtCodeList(int rowIdx)
{
	class LotXCodeListDialog : public LotXCodeListDialog_Base {
	public:
		LotXCodeListDialog(const PPBillPacket * pPack, int rowIdx) : LotXCodeListDialog_Base(DLG_LOTXCLIST, CTL_LOTXCLIST_LIST, pPack, rowIdx, fOmitSearchByFirstChar)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(TVKEYDOWN) {
				uchar  c = TVCHR;
				if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || isdec(c)) {
					LotExtCodeTbl::Rec rec;
					PPLotExtCodeContainer::MarkSet set;
					if(EditItemDialog(rec, c, set) > 0) {
						if(Data.Add(RowIdx, set))
							updateList(-1);
						else
							PPError();
					}
					clearEvent(event);
				}
			}
			LotXCodeListDialog_Base::handleEvent(event);
		}
		virtual int setupList()
		{
			int    ok = 1;
			uint   mark_count = 0	;
			uint   box_count = 0;
			SString temp_buf;
			SString box_num;
			StringSet ss;
			PPLotExtCodeContainer::MarkSet ms;
			PPLotExtCodeContainer::MarkSet::Entry msentry;
			LongArray idx_list;
			Data.Get(RowIdx, &idx_list, ms);
			long list_pos_idx = 0;
			for(uint boxidx = 0; boxidx < ms.GetCount(); boxidx++) {
				if(ms.GetByIdx(boxidx, msentry)) {
					if(msentry.Flags & PPLotExtCodeContainer::fBox) {
						box_count++;
						temp_buf.Z().Cat("box").CatDiv(':', 2).Cat(msentry.Num);
						++list_pos_idx;
						THROW(addStringToList(list_pos_idx, temp_buf));
						ms.GetByBoxID(msentry.BoxID, ss);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							temp_buf.Insert(0, " ");
							++list_pos_idx;
							THROW(addStringToList(list_pos_idx, temp_buf));
						}
					}
					else
						mark_count++;
				}
			}
			{
				ms.GetByBoxID(0, ss);
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					++list_pos_idx;
					THROW(addStringToList(list_pos_idx, temp_buf));
				}
			}
			{
				temp_buf.Z();
				if(mark_count)
					temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("Marks", mark_count);
				if(box_count)
					temp_buf.CatDivIfNotEmpty(' ', 0).CatEq("Boxes", box_count);
				if(RowIdx > 0 && RowIdx <= (int)P_Pack->GetTCount()) {
					SString name_buf;
					const PPTransferItem & r_ti = P_Pack->ConstTI(RowIdx-1);
					GetGoodsName(r_ti.GoodsID, name_buf);
					temp_buf.CatDiv('-', 1).Cat(name_buf).Space().CatChar('[').Cat(fabs(r_ti.Quantity_), MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatChar(']');
				}
				setStaticText(CTL_LOTXCLIST_INFO, temp_buf);
			}
			CATCHZOK
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			PPLotExtCodeContainer::MarkSet set;
			LotExtCodeTbl::Rec rec;
			MEMSZERO(rec);
			rec.BillID = P_Pack->Rec.ID;
			rec.RByBill = RowIdx;
			while(ok < 0 && EditItemDialog(rec, 0, set) > 0) {
				if(Data.Add(RowIdx, set)) {
					ok = 1;
				}
				else
					PPError();
			}
			return ok;
		}
		int    SLAPI EditItemDialog(LotExtCodeTbl::Rec & rRec, char firstChar, PPLotExtCodeContainer::MarkSet & rSet)
		{
			int    ok = -1;
			uint   sel = 0;
			TDialog * dlg = new TDialog(DLG_LOTEXTCODE);
			SString temp_buf, info_buf;
			ReceiptTbl::Rec lot_rec;
			ReceiptCore & r_rcpt = BillObj->trfr->Rcpt;
			const PPTransferItem * p_ti = (RowIdx > 0 && RowIdx <= (int)P_Pack->GetTCount()) ? &P_Pack->ConstTI(RowIdx-1) : 0;
			const int  do_check = (P_Pack->IsDraft() || (!p_ti || p_ti->Flags & PPTFR_RECEIPT)) ? 0 : 1;
			const PPID goods_id = (do_check && p_ti) ? labs(p_ti->GoodsID) : 0;
			const PPID lot_id = (do_check && p_ti) ? p_ti->LotID : 0;
			THROW(CheckDialogPtr(&dlg));
			if(r_rcpt.Search(rRec.LotID, &lot_rec) <= 0)
				MEMSZERO(lot_rec);
			if(firstChar)
				temp_buf.Z().CatChar(firstChar);
			else
				(temp_buf = rRec.Code).Strip();
			dlg->setCtrlString(CTL_LOTEXTCODE_CODE, temp_buf);
			if(firstChar) {
				TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_LOTEXTCODE_CODE));
				CALLPTRMEMB(il, disableDeleteSelection(1));
			}
			if(lot_rec.GoodsID) {
				GetGoodsName(lot_rec.GoodsID, temp_buf);
				info_buf.Z().CatEq("LotID", lot_rec.ID).Space().Cat(lot_rec.Dt, DATF_DMY|DATF_CENTURY).CR().Cat(temp_buf);
			}
			dlg->setStaticText(CTL_LOTEXTCODE_INFO, info_buf);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->getCtrlString(sel = CTL_LOTEXTCODE_CODE, temp_buf);
				if(!temp_buf.NotEmptyS())
					PPErrorByDialog(dlg, sel, PPERR_CODENEEDED);
				else if(temp_buf.Len() >= sizeof(rRec.Code)) {
					PPSetError(PPERR_CODETOOLONG, (long)(sizeof(rRec.Code)-1));
					PPErrorByDialog(dlg, sel);
				}
				else {
					SString mark_buf = temp_buf;
					const int iemr = PrcssrAlcReport::IsEgaisMark(mark_buf, 0);
					if(!iemr) {
						if(P_LotXcT) {
							if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, rSet) > 0)
								ok = 1;
							else
								PPErrorByDialog(dlg, sel);
						}
						else {
							PPSetError(PPERR_TEXTISNTEGAISMARK, mark_buf);
							PPErrorByDialog(dlg, sel);
						}
					}
					else if(do_check && P_LotXcT) {
						if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, rSet) > 0) {
							STRNSCPY(rRec.Code, mark_buf);
							ok = 1;
						}
						else
							PPErrorByDialog(dlg, sel);
					}
					else {
						rSet.AddNum(0, mark_buf, 1);
						STRNSCPY(rRec.Code, mark_buf);
						ok = 1;
					}
				}
			}
			CATCHZOKPPERR
			delete dlg;
			return ok;
		}
		//
		//  @erik v10.5.2
		//  функция вставки всех марок из буфера(если они там есть)
		//
		virtual int pasteFromClipboardAll()
		{
			int    ok = 0;
			uint   sel = 0;
			//TDialog * dlg = new TDialog(DLG_LOTEXTCODE);
			SString temp_buf;
			StringSet ss;
			PPLotExtCodeContainer::MarkSet set;
			LotExtCodeTbl::Rec rec;
			MEMSZERO(rec);
			rec.BillID = P_Pack->Rec.ID;
			rec.RByBill = RowIdx;
			const PPTransferItem * p_ti = (RowIdx > 0 && RowIdx <= (int)P_Pack->GetTCount()) ? &P_Pack->ConstTI(RowIdx - 1) : 0;
			const int  do_check = (P_Pack->IsDraft() || (!p_ti || p_ti->Flags & PPTFR_RECEIPT)) ? 0 : 1;
			const PPID goods_id = (do_check && p_ti) ? labs(p_ti->GoodsID) : 0;
			const PPID lot_id = (do_check && p_ti) ? p_ti->LotID : 0;
			SStringU buf_from_copy;
			SClipboard::Past_Text(buf_from_copy);
			buf_from_copy.CopyToUtf8(temp_buf, 0);
			if(temp_buf.Tokenize("\xD\xA", ss)) {
				temp_buf.Z();
				for(uint ssp = 0; ss.get(&ssp, temp_buf); temp_buf.Z(), set.Clear()) {
					if(!temp_buf.NotEmptyS()) {
						//PPErrorByDialog(dlg, sel, PPERR_CODENEEDED);
						continue;
					}
					else if(temp_buf.Len() >= sizeof(rec.Code)) {
						//PPSetError(PPERR_CODETOOLONG, (long)(sizeof(rec.Code) - 1));
						//PPErrorByDialog(dlg, sel);
						continue;
					}
					else {
						SString mark_buf = temp_buf;
						const int iemr = PrcssrAlcReport::IsEgaisMark(mark_buf, 0);
						if(!iemr) {
							if(P_LotXcT) {
								if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, set) <= 0) {
									//PPErrorByDialog(dlg, sel);
									continue;
								}
							}
							else {
								//PPSetError(PPERR_TEXTISNTEGAISMARK, mark_buf);
								//PPErrorByDialog(dlg, sel);
								continue;
							}
						}
						else if(do_check && P_LotXcT) {
							if(P_LotXcT->FindMarkToTransfer(mark_buf, goods_id, lot_id, set) > 0) {
								STRNSCPY(rec.Code, mark_buf);
							}
							else {
								//PPErrorByDialog(dlg, sel);
								continue;
							}
						}
						else {
							set.AddNum(0, mark_buf, 1);
							STRNSCPY(rec.Code, mark_buf);
						}
					}
					if(Data.Add(RowIdx, set)) {
						ok++;
					}
					else {
						//PPError();
					}
				}
			}
			updateList(0);
			return ok;
		}
	};
	int    ok = -1;
	LotXCodeListDialog * dlg = new LotXCodeListDialog(P_Pack, rowIdx);
	if(CheckDialogPtr(&dlg)) {
		dlg->setDTS(&P_Pack->XcL);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&P_Pack->XcL);
		}
	}
	delete dlg;
	return ok;
}

IMPL_HANDLE_EVENT(BillItemBrowser)
{
	int    c;
	if(TVKEYDOWN && TVCHR == kbCtrlX)
		State |= stCtrlX;
	else if(TVCOMMAND || TVKEYDOWN) {
		if(TVCHR != kbCtrlR && TVCHR != kbCtrlZ && TVCHR != kbCtrlD)
			State &= ~stCtrlX;
	}
	if(TVKEYDOWN && (TVKEY == kbF3 || isalnum(c = TVCHR) || c == '*') && !AsSelector && EditMode < 2) {
		PPID   pckg_id = 0;
		SString code;
		Goods2Tbl::Rec grec;
		ReceiptTbl::Rec lot_rec;
		double qtty = 0.0;
		const  int  init_chr = (TVKEY == kbF3) ? 0 : c;
		const  long cfg_flags = LConfig.Flags;
		if(GObj.SelectGoodsByBarcode(init_chr, P_Pack->Rec.Object, &grec, &qtty, &code) > 0) {
			uint   pos = 0;
			if(!(P_BObj->Cfg.Flags & BCF_DONTWARNDUPGOODS) && P_Pack->SearchGoods(grec.ID, &pos) && !CONFIRM(PPCFM_SAMEGOODSINPACK))
				view->go(pos);
			else {
				const  PPID op_type = GetOpType(P_Pack->Rec.OpID);
				const  int  skip_dlg = BIN((P_BObj->GetConfig().Flags & BCF_ADDAUTOQTTYBYBRCODE) && oneof2(op_type, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT));
				TIDlgInitData tidi;
				SETFLAG(tidi.Flags, TIDIF_AUTOQTTY, skip_dlg);
				tidi.GoodsID  = grec.ID;
				tidi.Quantity = (tidi.Flags & TIDIF_AUTOQTTY) ? 1.0 : ((cfg_flags & CFGFLG_USEPACKAGE) ? 0.0 : qtty);
				addItem(0, &tidi);
			}
		}
		else if(P_BObj->SelectPckgByCode(code, P_Pack->Rec.LocID, &pckg_id) > 0) {
			if(P_BObj->AddPckgToBillPacket(pckg_id, P_Pack))
				update(pos_bottom);
			else
				PPError();
		}
		else if(P_BObj->SelectLotBySerial(code, 0, P_Pack->Rec.LocID, &lot_rec) > 0) {
			TIDlgInitData tidi;
			tidi.GoodsID  = labs(lot_rec.GoodsID);
			tidi.LotID    = lot_rec.ID;
			tidi.Quantity = (cfg_flags & CFGFLG_USEPACKAGE) ? 0.0 : qtty;
			addItem(0, &tidi);
		}
	}
	else {
		if(event.isCmd(cmExecute)) {
			if((State & stActivateNewRow) && !EditMode)
				addItem();
			// Далее управление передается базовому классу
		}
		BrowserWindow::handleEvent(event);
		if(event.isCmd(cmSort)) { // @v10.6.3
			update(pos_cur);
		}
		else if(TVCOMMAND && EditMode < 2) {
			switch(TVCMD) {
				case cmaEdit:
					if(!EventBarrier()) {
						editItem();
						EventBarrier(1);
					}
					break;
				case cmaInsert:
					if(!EventBarrier()) {
						TIDlgInitData tidi;
						GetDefScaleData(&tidi);
						addItem(0, &tidi);
						EventBarrier(1);
					}
					break;
				case cmaDelete:
					EVENT_BARRIER(delItem());
					break;
				default:
					return;
			}
		}
		else if(TVBROADCAST) {
			if(TVCMD == cmMouseHover) {
				long   v = 0;
				SString buf;
				TPoint point = *static_cast<TPoint *>(event.message.infoPtr);
				if(ItemByPoint(point, 0, &v)) {
					if(ProblemsList.GetText(v, buf) > 0)
						PPTooltipMessage(buf, 0, H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
							SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
				}
			}
		}
		else if(TVKEYDOWN) {
			switch(TVKEY) {
				case KB_CTRLENTER:
					if(AsSelector && IsInState(sfModal)) {
						AsSelector = -1;
						endModal(cmOK);
						return; // После endModal не следует обращаться к this
					}
					break;
				case kbF2:
					if(EditMode < 2) {
						EVENT_BARRIER(addItemExt(0));
					}
					break;
				case kbShiftF2:
					if(!EventBarrier()) {
						c = getCurItemPos();
						if(c >= 0 && c < (int)P_Pack->GetTCount()) {
							PPID   goods_id = labs(P_Pack->ConstTI(static_cast<uint>(c)).GoodsID);
							if(goods_id && GObj.Edit(&goods_id, 0) == cmOK)
								update(pos_cur);
						}
						EventBarrier(1);
					}
					break;
				case kbShiftF3:
					if(!EventBarrier()) {
						c = getCurItemPos();
						if(c >= 0 && c < (int)P_Pack->GetTCount()) {
							const PPTransferItem & r_ti = P_Pack->ConstTI(static_cast<uint>(c));
							if(oneof4(P_Pack->Rec.OpID, PPOPK_EDI_STOCK, PPOPK_EDI_ACTCHARGEON, PPOPK_EDI_ACTCHARGEONSHOP, PPOPK_EDI_SHOPCHARGEON)) {
								Reference * p_ref = PPRef;
								uint   i;
								SString ref_b, egais_code;
								StringSet ss_egais_codes;
                                BarcodeArray bc_list;
                                PPIDArray temp_list;
                                PPObjBill::SelectLotParam slp(labs(r_ti.GoodsID), r_ti.LocID, 0, PPObjBill::SelectLotParam::fShowEgaisTags|PPObjBill::SelectLotParam::fShowManufTime);
                                slp.GoodsList.add(labs(r_ti.GoodsID));
                                if(P_Pack->LTagL.GetTagStr(c, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code) > 0) {
									temp_list.clear();
									p_ref->Ot.SearchObjectsByStr(PPOBJ_LOT, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code, &temp_list);
                                    slp.AddendumLotList.add(&temp_list);
									ss_egais_codes.add(egais_code);
                                }
                                if(P_Pack->LTagL.GetTagStr(c, PPTAG_LOT_FSRARINFB, ref_b) > 0) {
									temp_list.clear();
									p_ref->Ot.SearchObjectsByStr(PPOBJ_LOT, PPTAG_LOT_FSRARINFB, ref_b, &temp_list);
                                    slp.AddendumLotList.add(&temp_list);
                                }
                                slp.AddendumLotList.sortAndUndup();
								GObj.ReadBarcodes(r_ti.GoodsID, bc_list);
								for(i = 0; i < bc_list.getCount(); i++) {
									const BarcodeTbl::Rec & r_bc = bc_list.at(i);
									if(sstrlen(r_bc.Code) == 19)
										ss_egais_codes.add(r_bc.Code);
								}
								ss_egais_codes.sortAndUndup();
								for(uint ssp = 0; ss_egais_codes.get(&ssp, egais_code);) {
									BarcodeTbl::Rec bc_rec;
									if(GObj.SearchByBarcode(egais_code, &bc_rec, 0, 0) > 0)
										slp.GoodsList.add(bc_rec.GoodsID);
								}
								slp.GoodsList.sortAndUndup();
								P_BObj->SelectLot2(slp);
							}
							else {
								// @v9.3.6 PPID   lot_id = r_ti.LotID;
								// @v9.3.6 SelectLot(r_ti.LocID, labs(r_ti.GoodsID), 0, &lot_id, 0);
								PPObjBill::SelectLotParam slp(labs(r_ti.GoodsID), r_ti.LocID, 0, PPObjBill::SelectLotParam::fShowManufTime); // @v9.3.6
								slp.RetLotID = r_ti.LotID; // @v9.3.6
								P_BObj->SelectLot2(slp);
							}
						}
						EventBarrier(1);
					}
					break;
				case kbF4:
					if(EditMode < 2) {
						EVENT_BARRIER(addItemExt(1));
					}
					break;
				case kbCtrlF4:
					if(EditMode < 2) {
                        EVENT_BARRIER(addItemBySerial());
					}
					break;
				case kbF5: // Подбор товара по цене
					if(EditMode < 2) {
                        EVENT_BARRIER(addItemExt(2));
					}
					break;
				case kbShiftF5:
					if(EditMode < 2) {
						EVENT_BARRIER(addNewPackage());
					}
					break;
				case kbCtrlF5:
					EVENT_BARRIER(viewPckgItems(0));
					break;
				case kbF6:
					if(!AsSelector && EditMode < 2) {
						EVENT_BARRIER(addItem(1));
					}
					break;
				case kbF8:
					EVENT_BARRIER(ConvertBasketToBill());
					break;
				case kbF9:
					if(!AsSelector && EditMode < 2) {
						if(!EventBarrier()) {
							int    r = P_Pack->InsertPartitialStruc();
							if(r >= 0) {
								if(r == 0)
									PPError();
								State |= stIsModified;
								update(pos_bottom);
							}
							EventBarrier(1);
						}
					}
					break;
				case kbCtrlF8:
					EVENT_BARRIER(ConvertBillToBasket());
					break;
				case kbShiftF4:
					if(CheckRows() > 0)
						update(0);
					break;
				default:
					if(TVCHR == kbCtrlT) {
						if(!EventBarrier()) {
							BillTotalData btd;
							P_Pack->InitAmounts(0);
							P_Pack->CalcTotal(&btd, 0);
							btd.Amounts.copy(P_Pack->Amounts);
							if(!P_BObj->CheckRights(BILLRT_ACCSCOST))
								btd.Amounts.Put(PPAMT_BUYING, 0L /* @curID */, 0, 0, 1);
							AmtListDialog * dlg = new AmtListDialog(DLG_GOODSBILLTOTAL, CTL_GBILLTOTAL_AMTLIST, 1, &btd.Amounts, 0, 0, 0);
							if(CheckDialogPtrErr(&dlg)) {
								dlg->setCtrlLong(CTL_GBILLTOTAL_LINES,  btd.LinesCount);
								dlg->setCtrlLong(CTL_GBILLTOTAL_GCOUNT, btd.GoodsCount);
								dlg->setCtrlReal(CTL_GBILLTOTAL_BRUTTO, btd.Brutto);
								dlg->setCtrlReal(CTL_GBILLTOTAL_VOLUME, btd.Volume);
								dlg->setCtrlReal(CTL_GBILLTOTAL_QTTY,   btd.UnitsCount);
								dlg->setCtrlReal(CTL_GBILLTOTAL_PHQTTY, btd.PhUnitsCount);
								ExecViewAndDestroy(dlg);
							}
							EventBarrier(1);
						}
					}
					else if(TVCHR == kbCtrlE) {
						int    c = getCurItemPos();
						if(c >= 0) {
							EditExtCodeList(c+1);
						}
					}
					else if(TVCHR == kbCtrlD) {
						ValidateExtCodeList();
					}
					else if(TVCHR == kbCtrlL) {
						if(!EventBarrier()) {
							int    c = getCurItemPos();
							if(c >= 0)
								BarcodeLabelPrinter::PrintLabelByBill2(P_Pack, (uint)c);
							EventBarrier(1);
						}
					}
					else if(TVCHR == kbCtrlB) {
						if(EditMode < 2 && P_Pack) {
							if(!EventBarrier()) {
								if(oneof2(P_Pack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT) && !GetOpSubType(P_Pack->Rec.OpID)) {
									int    is_mod = BIN(State & stIsModified);
									if(P_BObj->AutoCalcPrices(P_Pack, 1, &is_mod) > 0)
										update(0);
									if(is_mod)
										State |= stIsModified;
								}
								else {
									if(P_BObj->SetupQuot(P_Pack, 0) > 0)
										update(0);
								}
								EventBarrier(1);
							}
						}
					}
					else if(TVCHR == kbCtrlA) {
						if(!(State & stAltView) && P_Pack && IsSellingOp(P_Pack->Rec.OpID) >= 0 && P_Pack->OpTypeID != PPOPT_GOODSORDER) {
							if(!EventBarrier()) {
								int    __c = 0; // @v10.6.3 int16-->int
								BillItemBrowser * p_brw = new BillItemBrowser(BROWSER_ID(GOODSITEM_ALTVIEW), P_BObj, P_Pack, 0, -1, 0, EditMode);
								if(p_brw) {
									if(getDefC()) {
										// @todo Здесь необходимо учесть сортировку 
										__c = getDefC()->_curItem();
										p_brw->go(__c);
									}
									ExecViewAndDestroy(p_brw);
								}
								update(__c);
								EventBarrier(1);
							}
						}
					}
					else if(TVCHR == kbCtrlS) {
						SString temp_buf, templt;
						if(P_Pack && oneof2(P_Pack->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT) && CONFIRM(PPCFM_GENSERIALFORBILL)) {
							if(!EventBarrier()) {
								PPTransferItem * p_ti = 0;
								int    upd = 0;
								for(uint i = 0; P_Pack->EnumTItems(&i, &p_ti);) {
									// @v9.8.11 if(P_Pack->SnL.GetNumber(i-1, &temp_buf) <= 0 || !temp_buf.NotEmptyS()) {
									if(P_Pack->LTagL.GetNumber(PPTAG_LOT_SN, i-1, temp_buf) <= 0 || !temp_buf.NotEmptyS()) { // @v9.8.11
										templt = (GObj.IsAsset(p_ti->GoodsID) > 0) ? P_BObj->Cfg.InvSnTemplt : P_BObj->Cfg.SnTemplt;
										if(P_BObj->GetSnByTemplate(P_Pack->Rec.Code, labs(p_ti->GoodsID), &P_Pack->LTagL/*SnL*/, templt, temp_buf) > 0) {
											if(temp_buf.NotEmptyS()) {
												// @v9.8.11 P_Pack->SnL.AddNumber(i-1, temp_buf);
												P_Pack->LTagL.AddNumber(PPTAG_LOT_SN, i-1, temp_buf); // @v9.8.11
												upd = 1;
											}
										}
									}
								}
								if(upd)
									update(pos_cur);
								EventBarrier(1);
							}
						}
					}
					else if(TVCHR == kbCtrlU) {
						if(!EventBarrier()) {
							c = getCurItemPos();
							if(c >= 0 && c < (int)P_Pack->GetTCount()) {
								const PPTransferItem & r_ti = P_Pack->ConstTI(static_cast<uint>(c));
								Goods2Tbl::Rec goods_rec;
								const PPID goods_id = labs(r_ti.GoodsID);
								if(GObj.Fetch(goods_id, &goods_rec) > 0) {
									PPObjGoods::ExtUniteBlock eub;
									PrcssrAlcReport::Config alcr_cfg;
									if(PrcssrAlcReport::ReadConfig(&alcr_cfg) > 0 && alcr_cfg.E.AlcGoodsClsID) {
										if(goods_rec.GdsClsID == alcr_cfg.E.AlcGoodsClsID)
											eub.Flags |= eub.fUseSpcFormEgais;
									}
									//eub.Flags |= (eub.fReverseOnStart|eub.fOnce);
									eub.DestList.add(labs(r_ti.GoodsID));
									if(GObj.ReplaceGoods(/*labs(r_ti.GoodsID), &eub*/eub) > 0) {
										for(uint i = 0; i < P_Pack->GetTCount(); i++) {
											PPTransferItem & r_item = P_Pack->TI(i);
											const int gsign = (r_item.GoodsID < 0) ? -1 : 1;
											if(eub.DestList.lsearch(labs(r_item.GoodsID)))
                                                r_item.SetupGoods(eub.ResultID * gsign);
										}
										update(pos_cur);
									}
								}
							}
							EventBarrier(1);
						}
					}
					else if(TVCHR == kbCtrlD && State & stCtrlX) {
						if(!EventBarrier()) {
							State &= ~stCtrlX;
							if(DS.CheckExtFlag(ECF_AVERAGE) && PPMaster) {
								const int c = getCurItemPos(); // @v10.6.3 int16-->int
								if(c >= 0) {
									PPTransferItem * p_ti = &P_Pack->TI(static_cast<uint>(c));
									INVERSEFLAG(p_ti->TFlags, PPTransferItem::tfForceRemove);
									update(c);
								}
							}
							EventBarrier(1);
						}
					}
					else if(TVCHR == kbCtrlR && State & stCtrlX) {
						if(!EventBarrier()) {
							State &= ~stCtrlX;
							if(CConfig.Flags & CCFLG_DEBUG && PPMaster) {
								const int  c = getCurItemPos(); // @v10.6.3 int16-->int
								if(c >= 0) {
									PPTransferItem * p_ti = &P_Pack->TI(static_cast<uint>(c));
									SString temp_buf;
									temp_buf.CatHex(p_ti->Flags);
									PPInputStringDialogParam isd_param;
									isd_param.Flags |= PPInputStringDialogParam::fDisableSelection;
									if(InputStringDialog(&isd_param, temp_buf) > 0) {
										sscanf(temp_buf.Strip(), "%lx", &p_ti->Flags);
										update(-1);
									}
								}
							}
							EventBarrier(1);
						}
					}
					else if(TVCHR == kbCtrlZ && State & stCtrlX) {
						if(!EventBarrier()) {
							State &= ~stCtrlX;
							if(P_Pack->Rec.ID && P_Pack->GetTCount() == 0) {
								SysJournal * p_sj = DS.GetTLA().P_SysJ;
								if(p_sj) {
									HistBillCore hb_core;
									SysJournalTbl::Key1 k1;
									MEMSZERO(k1);
									k1.ObjType = PPOBJ_BILL;
									k1.ObjID = P_Pack->Rec.ID;
									k1.Dt = MAXDATE;
									k1.Tm = MAXTIME;
									if(p_sj->search(1, &k1, spLt) && k1.ObjType == PPOBJ_BILL && k1.ObjID == P_Pack->Rec.ID) {
										do {
											if(p_sj->data.Action == PPACN_UPDBILL) {
												PPHistBillPacket hb_pack;
												if(hb_core.GetPacket(p_sj->data.Extra, &hb_pack) > 0 && hb_pack.GetCount()) {
													HistTrfrTbl::Rec * p_h_item;
													for(uint i = 0; hb_pack.EnumItems(&i, &p_h_item); ) {
														PPTransferItem ti(&P_Pack->Rec, TISIGN_UNDEF);
														ti.SetupGoods(p_h_item->GoodsID);
														ti.Quantity_ = p_h_item->Quantity;
														ti.Cost     = p_h_item->Cost;
														ti.Price    = p_h_item->Price;
														ti.Discount = p_h_item->Discount;
														ti.QCert    = p_h_item->QCertID;
														ti.Expiry   = p_h_item->Expiry;
														ti.Flags    = p_h_item->Flags;
														ILTI ilti(&ti);
														if(!P_BObj->ConvertILTI(&ilti, P_Pack, 0, CILTIF_ABSQTTY, 0, 0))
															PPError();
													}
													update(0);
													break;
												}
											}
										} while(p_sj->search(1, &k1, spPrev) && k1.ObjType == PPOBJ_BILL && k1.ObjID == P_Pack->Rec.ID);
									}
								}
							}
							EventBarrier(1);
						}
					}
					else
						return;
					break;
			}
		}
		else
			return;
	}
	clearEvent(event);
}

int BillItemBrowser::selectOrder()
{
	OrderBillID = 0;
	int    r = -1;
	if(!P_Pack->SampleBillID) {
		BillFilt flt;
		PPOprKind op_rec;
		flt.Period.upp = P_Pack->Rec.Dt;
		PPID   op_id = 0;
		for(PPID id = 0; (r = EnumOperations(PPOPT_GOODSORDER, &id, &op_rec)) > 0;)
			if(op_rec.AccSheetID == P_Pack->AccSheetID)
				if(op_id == 0)
					op_id = id;
				else {
					op_id = 0;
					break;
				}
		flt.OpID = op_id;
		flt.ObjectID = P_Pack->Rec.Object;
		//flt.LocID  = P_Pack->Rec.Location;
		flt.Ft_ClosedOrder = -1;
		flt.Flags   |= (BillFilt::fAsSelector | BillFilt::fOrderOnly);
		if((r = ViewGoodsBills(&flt, 0)) > 0)
			return ((OrderBillID = flt.Sel), 1);
		else
			return r ? -1 : 0;
	}
	else
		return ((OrderBillID = P_Pack->SampleBillID), 1);
}

int BillItemBrowser::isAllGoodsInPckg(PPID goodsID)
{
	int    all_lots_in_pckg = 0;
	//
	// Проверка на то, чтобы хотя бы один лот товара был не в пакете
	// Если весь товар в пакетах, то предлагаем выбрать пакет
	//
	if(goodsID && (CConfig.Flags & CCFLG_USEGOODSPCKG) && !oneof2(P_Pack->OpTypeID, PPOPT_GOODSORDER, PPOPT_GOODSRECEIPT)) {
		LotArray lot_list;
		P_T->Rcpt.GetListOfOpenedLots(1, goodsID, P_Pack->Rec.LocID, P_Pack->Rec.Dt, &lot_list);
		for(uint i = 0; i < lot_list.getCount(); i++)
			if(P_BObj->IsLotInPckg(lot_list.at(i).ID) > 0)
				all_lots_in_pckg = 1;
			else {
				all_lots_in_pckg = 0;
				break;
			}
	}
	return all_lots_in_pckg;
}

void BillItemBrowser::addItemBySerial()
{
	const PPID op_id = P_Pack->Rec.OpID;
	const PPID op_type_id = GetOpType(op_id);
	const int opened_only = BIN(IsExpendOp(op_id) > 0 || op_type_id == PPOPT_GOODSREVAL || (op_type_id == PPOPT_GOODSORDER && CheckOpFlags(op_id, OPKF_ORDEXSTONLY)));
	SString serial;
	PPInputStringDialogParam isd_param;
	PPLoadText(PPTXT_SELGOODSBYSERIAL, isd_param.Title);
	if(opened_only && P_BObj->GetConfig().Flags & BCF_PICKLOTS) {
		if(InputStringDialog(&isd_param, serial) > 0 && serial.NotEmptyS()) {
			PickLotParam plp(0);
			plp.LocID = P_Pack->Rec.LocID;
			plp.Mode = plp.mBySerial;
			plp.Text = serial;
			PPLoadText(PPTXT_PICKLOT_BYSERIAL, plp.SubTitle);
			plp.SubTitle.Space().CatQStr(serial);
			TIDlgInitData tidi;
			if(PickLot(plp, &tidi) > 0) {
				addItem(0, &tidi);
			}
		}
	}
	else {
		isd_param.P_Wse = new ObjTagSelExtra(PPOBJ_LOT, PPTAG_LOT_SN);
		{
			const long _flags = opened_only ? ObjTagSelExtra::lfOpenedSerialsOnly : 0;
			static_cast<ObjTagSelExtra *>(isd_param.P_Wse)->SetupLotSerialParam(P_Pack->Rec.LocID, _flags);
		}
		if(InputStringDialog(&isd_param, serial) > 0 && serial.NotEmptyS()) {
			ReceiptTbl::Rec lot_rec;
			if(P_BObj->SelectLotBySerial(serial, 0, P_Pack->Rec.LocID, &lot_rec) > 0) {
				TIDlgInitData tidi;
				tidi.GoodsID  = labs(lot_rec.GoodsID);
				tidi.LotID    = lot_rec.ID;
				tidi.Quantity = (LConfig.Flags & CFGFLG_USEPACKAGE) ? 0.0 : 1.0;
				addItem(0, &tidi);
			}
		}
	}
}

void BillItemBrowser::addItemExt(int mode)
{
	const  PPID op_id = P_Pack->Rec.OpID;
	const  PPID op_type_id = P_Pack->OpTypeID;
	int    do_exit = 0;
	double sel_price = 0.0;
	SString sub;
	ExtGoodsSelDialog * dlg = 0;
	if(State & stUseLinkSelection)
		addItem();
	else if(op_type_id == PPOPT_GOODSEXPEND && CheckOpFlags(op_id, OPKF_PCKGMOUNTING) && !P_Pckg)
		selectPckg(0);
	else {
		StrAssocArray goods_list;
		const int opened_only = BIN(IsExpendOp(op_id) > 0 || op_type_id == PPOPT_GOODSREVAL || (op_type_id == PPOPT_GOODSORDER && CheckOpFlags(op_id, OPKF_ORDEXSTONLY)));
		if(mode == 1) {
			PPInputStringDialogParam isd_param;
			PPLoadText(PPTXT_SELGOODSBYNAME, isd_param.Title);
			if(InputStringDialog(&isd_param, sub) > 0 && sub.NotEmptyS()) {
				THROW(GObj.P_Tbl->GetListBySubstring(sub, &goods_list, -1));
			}
			else {
				mode = 0;
				do_exit = 1;
			}
		}
		else if(mode == 2) {
			SString title, label;
			if(InputNumberDialog(PPLoadTextS(PPTXT_SELGOODSBYPRICE, title), PPLoadTextS(PPTXT_INPUTPRICE, label), sel_price) > 0 && sel_price > 0.0) {
			}
			else {
				mode = 0;
				do_exit = 1;
			}
		}
		else if(NewGoodsGrpID == 0) {
			const  int op_subt = GetOpSubType(op_id);
			if(oneof2(op_subt, OPSUBT_ASSETEXPL, OPSUBT_ASSETRCV))
				NewGoodsGrpID = GObj.GetConfig().AssetGrpID;
		}
		if(!do_exit) {
			TIDlgInitData tidi;
			if(opened_only && P_BObj->GetConfig().Flags & BCF_PICKLOTS && oneof2(mode, 1, 2)) {
				PickLotParam plp(0);
				plp.LocID = P_Pack->Rec.LocID;
                if(mode == 2) {
                	plp.Mode = plp.mByPrice;
                	plp.Price = sel_price;
					PPLoadText(PPTXT_PICKLOT_BYPRICE, plp.SubTitle);
					plp.SubTitle.Space().Cat(sel_price, SFMT_MONEY);
                }
                else if(mode == 1) {
                	plp.Mode = plp.mByGoodsList;
					plp.GoodsList = goods_list;
					PPLoadText(PPTXT_PICKLOT_BYSUBNAME, plp.SubTitle);
					plp.SubTitle.Space().CatQStr(sub);
                }
				if(PickLot(plp, &tidi) > 0) {
					addItem(0, &tidi);
				}
			}
			else {
				long   egsd_flags = 0;
				UserInterfaceSettings uis;
				const int uis_r = uis.Restore();
				if(uis_r > 0 && uis.Flags & UserInterfaceSettings::fExtGoodsSelMainName)
					egsd_flags |= ExtGoodsSelDialog::fByName;
				if(op_type_id == PPOPT_GOODSMODIF) {
					if(uis_r <= 0 || !(uis.Flags & UserInterfaceSettings::fOldModifSignSelection))
						egsd_flags |= ExtGoodsSelDialog::fSelectModifMode;
				}
				THROW(CheckDialogPtr(&(dlg = new ExtGoodsSelDialog(op_id, NewGoodsGrpID, egsd_flags))));
				dlg->RestoreUserSettings();
				if(mode == 1) {
					const  int  code_prefix = BIN(DS.CheckExtFlag(ECF_CODEPREFIXEDLIST));
					if(code_prefix) {
						StrAssocArray temp_array;
						SString text_buf;
						for(uint i = 0; i < goods_list.getCount(); i++) {
							StrAssocArray::Item item = goods_list.at_WithoutParent(i);
							if(GObj.FetchSingleBarcode(item.Id, text_buf.Z()) > 0)
								text_buf.CatCharN(' ', 3).Cat(item.Txt);
							else
								text_buf = item.Txt;
							temp_array.AddFast(item.Id, text_buf);
						}
						goods_list.Z();
						dlg->setSelectionByGoodsList(&temp_array);
					}
					else
						dlg->setSelectionByGoodsList(&goods_list);
				}
				else if(mode == 2) {
					dlg->setSelectionByPrice(sel_price);
				}
				else {
					if(NewGoodsGrpID == 0)
						GetDefScaleData(&tidi);
					tidi.ArID = P_Pack->Rec.Object;
					dlg->setDTS(&tidi);
					dlg->setLocation(P_Pack->Rec.LocID);
				}
				while(ExecView(dlg) == cmOK) {
					dlg->SaveUserSettings();
					if(dlg->getDTS(&tidi) > 0) {
						uint   pos = 0;
						if(!(P_BObj->Cfg.Flags & BCF_DONTWARNDUPGOODS) && P_Pack->SearchGoods(tidi.GoodsID, &pos) && !CONFIRM(PPCFM_SAMEGOODSINPACK))
							view->go(pos);
						else if(isAllGoodsInPckg(tidi.GoodsID))
							selectPckg(tidi.GoodsID);
						else
							addItem(0, &tidi);
					}
				}
				dlg->SaveUserSettings();
			}
		}
	}
	CATCH
		PPError();
	ENDCATCH
	delete dlg;
}

int BillItemBrowser::editPackageData(LPackage * pPckg)
{
	class PckgDialog : public TDialog {
		DECL_DIALOG_DATA(LPackage);
	public:
		PckgDialog(PPBillPacket * pPack, PPObjBill * pBObj) : TDialog(DLG_PCKG), P_Pack(pPack), P_BObj(pBObj)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_PCKG_TYPE, PPOBJ_PCKGTYPE, Data.PckgTypeID, 0, 0);
			setCtrlData(CTL_PCKG_CODE, Data.Code);
			setCtrlData(CTL_PCKG_ID, &Data.ID);
			setCtrlData(CTL_PCKG_COST, &Data.Cost);
			setCtrlData(CTL_PCKG_PRICE, &Data.Price);
			disableCtrls(1, CTL_PCKG_ID, CTL_PCKG_COST, CTL_PCKG_PRICE, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			PPObjPckgType pt_obj;
			PPGdsPckgType pt_rec;
			getCtrlData(CTLSEL_PCKG_TYPE, &Data.PckgTypeID);
			getCtrlData(CTL_PCKG_CODE, Data.Code);
			//getCtrlData(CTL_PCKG_ID, &Data.ID);
			//getCtrlData(CTL_PCKG_COST, &Data.Cost);
			//getCtrlData(CTL_PCKG_PRICE, &Data.Price);
			if(!P_BObj->CheckPckgCodeUnique(&Data, P_Pack))
				ok = (PPError(PPERR_DUPPCKGCODE, Data.Code), 0);
			else {
				if(Data.PckgTypeID && pt_obj.Get(Data.PckgTypeID, &pt_rec) > 0)
					if(pt_rec.Flags & GF_PCKG_AROWS)
						if(pt_rec.Flags & GF_PCKG_ANEWROW)
							ok = 3;
						else
							ok = 2;
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmPckgMakeCode) || (event.isKeyDown(kbF2) && isCurrCtlID(CTL_PCKG_CODE))) {
				makeCode();
				clearEvent(event);
			}
		}
		void   makeCode()
		{
			char   code[32];
			PTR32(code)[0] = 0;
			const  PPID pt_id = getCtrlLong(CTLSEL_PCKG_TYPE);
			P_BObj->GenPckgCode(pt_id, code, sizeof(code));
			if(*strip(code))
				setCtrlData(CTL_PCKG_CODE, code);
		}
		PPBillPacket * P_Pack;
		PPObjBill * P_BObj;
	};
	DIALOG_PROC_BODY_P2(PckgDialog, P_Pack, P_BObj, pPckg);
}

void BillItemBrowser::addNewPackage()
{
	if(CConfig.Flags & CCFLG_USEGOODSPCKG && !P_Pckg)
		if(IsExpendOp(P_Pack->Rec.OpID) > 0)
			selectPckg(0);
		else if(P_Pack->OpTypeID == PPOPT_GOODSRECEIPT) {
			int    r;
			LPackage pckg;
			P_BObj->InitPckg(&pckg);
			if((r = editPackageData(&pckg)) > 0)
				if(P_Pack->AddPckg(&pckg)) {
					update(pos_bottom);
					if(oneof2(r, 2, 3))
						viewPckgItems(BIN(r == 3));
				}
				else
					PPError();
		}
}

int SLAPI PPObjBill::SelectPckgByCode(const char * pCode, PPID locID, PPID * pPckgID)
{
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		PPIDArray pckg_id_list;
		if(P_PckgT->SearchByCode(-1, pCode, &pckg_id_list) > 0) {
			for(int i = pckg_id_list.getCount()-1; i >= 0; i--) {
				if(trfr->Rcpt.Search(pckg_id_list.at(i)) > 0) {
					if(trfr->Rcpt.data.Rest <= 0 || trfr->Rcpt.data.LocID != locID)
						pckg_id_list.atFree(i);
				}
				else
					pckg_id_list.atFree(i);
			}
			if(pckg_id_list.getCount()) {
				pckg_id_list.sort();
				ASSIGN_PTR(pPckgID, pckg_id_list.at(pckg_id_list.getCount()-1));
				return 1;
			}
		}
	}
	return -1;
}

int SLAPI PPObjBill::AddPckgToBillPacket(PPID pckgID, PPBillPacket * pPack)
{
	int    ok = 1;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		uint   j;
		int    idx;
		PPID   lot_id;
		double rest;
		LPackage pckg;
		ReceiptTbl::Rec lot_rec;
		if(pPack->P_PckgList)
			THROW_PP(!pPack->P_PckgList->GetByID(pckgID), PPERR_PCKGINBILL);
		THROW(P_PckgT->GetPckg(pckgID, &pckg) > 0);
		if(trfr->Rcpt.Search(pckgID, &lot_rec) > 0) {
			THROW_PP(pPack->Rec.LocID == 0 || lot_rec.LocID == pPack->Rec.LocID, PPERR_MISSPCKGLOC);
			{
				PPTransferItem ti(&pPack->Rec, 0);
				THROW(ti.SetupGoods(lot_rec.GoodsID));
	   	    	ti.SetupLot(pckgID, &lot_rec, 0);
				THROW(pPack->BoundsByLot(pckgID, 0, -1, &rest, 0));
				ti.Quantity_ = fabs(rest);
				ti.Flags |= PPTFR_PCKG;
				THROW(pPack->InsertRow(&ti, 0));
				pckg.PckgIdx = pPack->GetTCount()-1;
			}
		}
		for(j = 0; pckg.EnumItems(&j, &idx, &lot_id);)
			if(trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
				PPTransferItem ti(&pPack->Rec, 0);
				THROW(ti.SetupGoods(lot_rec.GoodsID));
				ti.SetupLot(lot_id, &lot_rec, 0);
				THROW(pPack->BoundsByLot(lot_id, 0, -1, &rest, 0));
				ti.Quantity_ = fabs(rest);
				ti.Flags |= PPTFR_PCKGGEN;
				THROW(pPack->InsertRow(&ti, 0));
				pckg.UpdateItem(j - 1, pPack->GetTCount()-1, lot_id);
			}
		THROW(pPack->AddPckg(&pckg));
		pPack->CalcPckgTotals();
	}
	CATCHZOK
	return ok;
}
//
//
//
class CompleteBrowser : public BrowserWindow {
public:
	CompleteBrowser(PPObjBill * pBObj, const CompleteArray * s, int asSelector) :
		AsSelector(asSelector), BrowserWindow(BROWSER_COMPLETE, (SArray *)0), Data(*s), P_BObj(pBObj), CmplAryPos(0), SelectedPos(-1)
	{
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(view->getDef());
		if(p_def) {
			p_def->setArray(0, 0, 1);
			SArray * p_list = MakeList();
			if(p_list) {
				p_def->setArray(p_list, 0, 1);
				view->setRange(p_list->getCount());
			}
		}
		SetCellStyleFunc(CompleteBrowser::CellStyleFunc, this);
	}
	int    GetSelectedItem(CompleteItem *);
	int    InitIteration()
	{
		CmplAryPos = 0;
		return 1;
	}
	int    FASTCALL NextIteration(CompleteItem * pItem);
	PPID   GetParentLot() const { return Data.LotID; }
	PPID   GetBillID() const { return Data.BillID; }
private:
	struct _Entry {
		PPID   GoodsID;
		PPID   LotID;
		LDATE  Dt;
		char   GoodsName[128];
		char   Serial[24];
		double Qtty;
		double Cost;
		double Price;
		LDATE  Expiry;
		char   ArName[48];
		char   Memo[128];
		long   Flags;
	};
	DECL_HANDLE_EVENT;
	SArray * MakeList();
	enum { // Параметр функции update
		pos_top = -1,
		pos_cur = -2,
		pos_bottom = -3
	};
	void   update(int pos);
	int    Print();
	static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr);

	PPObjBill * P_BObj;
	CompleteArray Data;
	uint   CmplAryPos;
	int    AsSelector;
	int    SelectedPos;
};

//static
int CompleteBrowser::CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	CompleteBrowser * p_brw = static_cast<CompleteBrowser *>(extraPtr);
	if(p_brw && pData && pStyle) {
		const _Entry * p_entry = static_cast<const _Entry *>(pData);
		const long goods_col = 1;
		if(col == goods_col) {
			if(p_entry->Flags & CompleteItem::fSource) {
				pStyle->Color = GetColorRef(SClrCoral);
				pStyle->Flags = BrowserWindow::CellStyle::fCorner;
				ok = 1;
			}
			else if(p_entry->Flags & CompleteItem::fBranch) {
				pStyle->Color = GetColorRef(SClrLightgreen);
				pStyle->Flags = BrowserWindow::CellStyle::fCorner;
				ok = 1;
			}
		}
	}
	return ok;
}

void CompleteBrowser::update(int pos)
{
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(view->getDef());
	if(p_def) {
		const int c = p_def->_curItem(); // @v10.6.3 int16-->int
		p_def->setArray(0, 0, 1);
		CompleteArray compl_list;
		Data.freeAll();
		int    r = P_BObj->GetComplete(Data.LotID, PPObjBill::gcfGatherSources, &Data);
		SArray * a = MakeList();
		if(a) {
            p_def->setArray(a, 0, 1);
			view->setRange(a->getCount());
			if(pos == pos_cur && c >= 0)
				view->go(c);
			else if(pos == pos_bottom)
				view->go(a->getCount() - 2);
			else if(pos >= 0 && pos < static_cast<int>(a->getCount()))
				view->go(pos);
		}
	}
}

IMPL_HANDLE_EVENT(CompleteBrowser)
{
	BrowserWindow::handleEvent(event);
	if(event.isCmd(cmaEdit)) {
		if(AsSelector) {
			if(IsInState(sfModal)) {
				SelectedPos = view->getDefC()->_curItem();
				endModal(cmOK);
				return; // После endModal не следует обращаться к this
			}
		}
		else {
			long   pos = view->getDefC()->_curItem();
			PPID   bill_id = Data.at(pos).BillID;
			BillTbl::Rec bill_rec;
			if(P_BObj->Search(bill_id, &bill_rec) > 0) {
				int    r = P_BObj->Edit(&bill_id, 0);
				if(r == 0)
					PPError();
				else if(r == cmOK)
					update(-1);
			}
		}
	}
	else if(event.isCmd(cmaInsert)) {
		ReceiptTbl::Rec lot_rec;
		if(Data.LotID && P_BObj->trfr->Rcpt.Search(Data.LotID, &lot_rec) > 0) {
			PPIDArray op_list;
			GetOpList(PPOPT_GOODSMODIF, &op_list);
			if(op_list.getCount()) {
				PPObjBill::AddBlock ab;
				ab.LocID = lot_rec.LocID;
				ab.OpID = op_list.getSingle();
				if(ab.OpID || BillPrelude(&op_list, OPKLF_OPLIST, 0, &ab.OpID, &ab.LocID) > 0) {
					PPID   new_bill_id = 0;
					ab.FirstItemSign = TISIGN_RECOMPLETE;
					ab.FirstItemLotID = lot_rec.ID;
					int r = P_BObj->AddGoodsBill(&new_bill_id, &ab);
					if(r == cmOK) {
						update(-1);
					}
					else if(r == 0)
						PPError();
					//if(P_Pack->BoundsByLot(Item.LotID, &Item, ItemNo, &Rest, 0));
				}
			}
		}
	}
	else if(event.isKeyDown(kbF7))
		Print();
	else
		return;
	clearEvent(event);
}

int CompleteBrowser::GetSelectedItem(CompleteItem * pItem)
{
	int    ok = 1;
	if(AsSelector && SelectedPos >= 0 && SelectedPos < static_cast<int>(Data.getCount())) {
		ASSIGN_PTR(pItem, Data.at(SelectedPos));
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL CompleteBrowser::NextIteration(CompleteItem * pItem)
{
	CompleteItem * p_item = 0;
	int    ok = Data.enumItems(&CmplAryPos, reinterpret_cast<void **>(&p_item));
	if(ok)
		ASSIGN_PTR(pItem, *p_item);
	return ok;
}

SArray * CompleteBrowser::MakeList()
{
	SString temp_buf;
	PPObjGoods goods_obj;
	PPObjArticle ar_obj;
	CompleteItem * p_item;
	SArray * p_list = new SArray(sizeof(_Entry));
	THROW_MEM(p_list);
	for(uint i = 0; Data.enumItems(&i, reinterpret_cast<void **>(&p_item));) {
		BillTbl::Rec bill_rec;
		_Entry  entry;
		MEMSZERO(entry);
		entry.GoodsID = p_item->GoodsID;
		entry.LotID   = p_item->LotID;
		entry.Dt      = p_item->Dt;
		entry.Expiry  = p_item->Expiry;
		// @v9.5.5 STRNSCPY(entry.GoodsName, GetGoodsName(p_item->GoodsID, temp_buf));
		goods_obj.FetchNameR(p_item->GoodsID, temp_buf); // @v9.5.5
		STRNSCPY(entry.GoodsName, temp_buf); // @v9.5.5
		GetArticleName(p_item->ArID, temp_buf);
		STRNSCPY(entry.ArName, temp_buf);
		STRNSCPY(entry.Serial, p_item->Serial);
		entry.Qtty = p_item->Qtty;
		entry.Cost = p_item->Cost;
		entry.Price = p_item->Price;
		entry.Flags = p_item->Flags; // @v9.0.4
		if(P_BObj->Search(p_item->BillID, &bill_rec) > 0)
			STRNSCPY(entry.Memo, bill_rec.Memo);
		THROW_SL(p_list->insert(&entry));
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int CompleteBrowser::Print()
{
	PView  pf(this);
	return PPAlddPrint(REPORT_COMPLETE, &pf);
}

int SLAPI PPObjBill::ViewLotComplete(PPID lotID, PPID * pSelectedLotID)
{
	int    ok = -1;
	MemLeakTracer mlt;
	{
		CompleteArray compl_list;
		int    r = GetComplete(lotID, PPObjBill::gcfGatherSources|PPObjBill::gcfGatherBranches, &compl_list);
		if(r > 0) {
			CompleteBrowser * p_brw = new CompleteBrowser(this, &compl_list, BIN(pSelectedLotID));
			if(p_brw == 0)
				ok = (PPError(PPERR_NOMEM, 0), 0);
			else {
				ReceiptTbl::Rec lot_rec;
				SString title_buf;
				if(trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
					// @v9.5.5 GetGoodsName(lot_rec.GoodsID, title_buf);
					GObj.FetchNameR(lot_rec.GoodsID, title_buf); // @v9.5.5
					GetObjectName(PPOBJ_LOCATION, lot_rec.LocID, title_buf.CatDivIfNotEmpty(':', 1), 1);
					title_buf.CatDivIfNotEmpty(':', 1).Cat(lot_rec.Dt);
				}
				p_brw->setSubTitle(title_buf);
				int   cmd = ExecView(p_brw);
				CompleteItem compl_item;
				if(cmd == cmOK && pSelectedLotID && p_brw->GetSelectedItem(&compl_item) > 0) {
					*pSelectedLotID = compl_item.LotID;
					ok = 1;
				}
				delete p_brw;
			}
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Complete
//
PPALDD_CONSTRUCTOR(Complete)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Complete) { Destroy(); }

int PPALDD_Complete::InitData(PPFilt & rFilt, long rsrv)
{
	CompleteBrowser * p_cb = 0;
	if(rsrv)
		Extra[1].Ptr = p_cb = static_cast<CompleteBrowser *>(rFilt.Ptr);
	else
		Extra[0].Ptr = p_cb = new CompleteBrowser(BillObj, static_cast<const CompleteArray *>(rFilt.Ptr), 0);
	H.ParentLotID = p_cb->GetParentLot();
	H.BillID = p_cb->GetBillID();
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Complete::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	CompleteBrowser * p_cb = static_cast<CompleteBrowser *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	return BIN(p_cb->InitIteration());
}

int PPALDD_Complete::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	CompleteBrowser * p_cb = static_cast<CompleteBrowser *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	CompleteItem      item;
	if(p_cb->NextIteration(&item) > 0) {
		I.GoodsID = item.GoodsID;
		I.LotID   = item.LotID;
		STRNSCPY(I.Serial, item.Serial);
		I.Qtty    = item.Qtty;
		I.Cost    = item.Cost;
		I.Price   = item.Price;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_Complete::Destroy()
{
	if(Extra[0].Ptr) {
		delete static_cast<CompleteBrowser *>(Extra[0].Ptr);
		Extra[0].Ptr = 0;
	}
	Extra[1].Ptr = 0;
}
