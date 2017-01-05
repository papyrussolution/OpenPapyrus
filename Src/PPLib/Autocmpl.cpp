// AUTOCMPL.CPP
// Copyright (c) A.Sobolev 1998-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2016
// @codepage windows-1251
// Автоматическая комплектация //
//
#include <pp.h>
#pragma hdrstop

SLAPI PUGI::PUGI()
{
	THISZERO();
}

PUGL::SupplSubstItem::SupplSubstItem(uint pos)
{
	Position = pos;
	SupplID = 0;
	Unit = uAbs;
	Qtty = 0.0;
}

SString & FASTCALL PUGL::SupplSubstItem::QttyToStr(SString & rBuf) const
{
	rBuf = 0;
	if(Qtty > 0.0) {
		rBuf.Cat(Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
		if(Unit == uPct)
			rBuf.CatChar('%');
	}
	return rBuf;
}

//static
int PUGL::BalanceSupplSubstList(TSArray <SupplSubstItem> & rList, double neededeQtty)
{
	int    ok = -1;
	if(neededeQtty > 0.0 && rList.getCount()) {
		RAssocArray abs_list;
		uint i;
		for(i = 0; i < rList.getCount(); i++) {
			const SupplSubstItem & r_item = rList.at(i);
			double val = 0.0;
			if(r_item.Unit == r_item.uAbs) {
				val = r_item.Qtty;
			}
			else if(r_item.Unit == r_item.uPct) {
				val = r_item.Qtty * neededeQtty / 100.0;
			}
			if(val > 0.0) {
				abs_list.Add(i+1, val, 0, 0);
			}
		}
		double sum = abs_list.GetTotal();
		double delta = neededeQtty - sum;
		if((fabs(delta) / neededeQtty) <= 0.01) {
			uint max_pos = 0;
			double max_val = 0.0;
			for(i = 0; i < rList.getCount(); i++) {
				SupplSubstItem & r_item = rList.at(i);
				for(uint j = 0; j < abs_list.getCount(); j++) {
					RAssoc & r_abs_item = abs_list.at(j);
					if(r_abs_item.Key == (i+1)) {
						r_item.Qtty = r_abs_item.Val;
						break;
					}
				}
				if(r_item.Qtty > max_val) {
					max_val = r_item.Qtty;
					max_pos = i;
				}
			}
			if(delta != 0.0) {
				rList.at(max_pos).Qtty += delta;
			}
			ok = 1;
		}
	}
	return ok;
}

SLAPI PUGL::PUGL() : SArray(sizeof(PUGI))
{
	OPcug = 0; //PCUG_CANCEL;
	LocID = 0;
	SupplAccSheetForSubstID = 0; // @v9.2.1
	Dt = ZERODATE;
	ClearActions();
	CostByCalc  = 0;
	CalcCostPct = 0;
}

PUGL & FASTCALL PUGL::operator = (const PUGL & rS)
{
	OPcug = rS.OPcug;
	LocID = rS.LocID;
	SupplAccSheetForSubstID = rS.SupplAccSheetForSubstID; // @v9.2.1
	Dt    = rS.Dt;
	ActionsCount = rS.ActionsCount;
	memcpy(Actions, rS.Actions, sizeof(Actions));
	CostByCalc   = rS.CostByCalc;
	CalcCostPct  = rS.CalcCostPct;
	SArray::copy(rS);
	SupplSubstList = rS.SupplSubstList;
	return *this;
}

int FASTCALL PUGL::SetHeader(const BillTbl::Rec * pBillRec)
{
	LocID = pBillRec ? pBillRec->LocID : 0;
	Dt    = pBillRec ? pBillRec->Dt : ZERODATE;
	return 1;
}

int SLAPI PUGL::SearchGoods(PPID goodsID, uint * pPos, PUGI * pItem) const
{
	PUGI * p_item;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(p_item->GoodsID == goodsID) {
			ASSIGN_PTR(pItem, *p_item);
			ASSIGN_PTR(pPos, i-1);
			return 1;
		}
	return 0;
}

int SLAPI PUGL::Add(const PUGL * pList)
{
	PUGI * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);)
		if(!Add(p_item, pList->Dt))
			return 0;
	return 1;
}

int SLAPI PUGL::Add(const PUGI * pItem, LDATE dt)
{
	PUGI   item, * p_item;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(p_item->GoodsID == pItem->GoodsID && p_item->LocID == pItem->LocID) {
			p_item->NeededQty  += fabs(pItem->NeededQty);
			p_item->DeficitQty += fabs(pItem->DeficitQty);
			return 1;
		}
	item.Pos        = pItem->Pos;
	item.GoodsID    = pItem->GoodsID;
	item.LocID      = pItem->LocID;
	item.Flags      = pItem->Flags;
	item.NeededQty  = fabs(pItem->NeededQty);
	item.DeficitQty = fabs(pItem->DeficitQty);
	item.Cost       = pItem->Cost;
	item.Price      = pItem->Price;
	if(dt && (Dt == 0 || Dt > dt))
		Dt = dt;
	return insert(&item) ? 1 : PPSetErrorSLib();
}

int SLAPI PUGL::Add(const ILTI * pItem, PPID locID, uint itemPos, LDATE dt /* = ZERODATE */)
{
	PUGI   item;
	item.Pos = itemPos;
	item.GoodsID    = pItem->GoodsID;
	item.LocID      = locID;
	item.NeededQty  = fabs(pItem->Quantity);
	item.DeficitQty = fabs(pItem->Rest);
	item.Price      = pItem->Price;
	item.Cost       = pItem->Cost;
	return Add(&item, dt);
}

int SLAPI PUGL::Log(PPLogger * pLogger) const
{
	int    ok = -1;
	if(pLogger) {
		PUGI * p_di;
		SString msg_buf, goods_name;
		for(uint n = 0; enumItems(&n, (void **)&p_di);) {
			GetGoodsName(p_di->GoodsID, goods_name);
			(msg_buf = 0).CatCharN(' ', 4).Cat(goods_name);
			pLogger->Log(msg_buf.CatCharN(' ', 4).Cat(p_di->DeficitQty, MKSFMTD(0, 6, 0)));
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PUGL::GetItemsLocList(PPIDArray * pList) const
{
	PUGI * p_item;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(p_item->LocID)
			pList->addUnique(p_item->LocID);
	return 1;
}

int SLAPI PUGL::GetSupplSubstList(uint pos /*[1..]*/, TSArray <PUGL::SupplSubstItem> & rList) const
{
	int    ok = -1;
	rList.clear();
	for(uint i = 0; i < SupplSubstList.getCount(); i++) {
		const SupplSubstItem & r_item = SupplSubstList.at(i);
		if(r_item.Position == pos) {
			THROW_SL(rList.insert(&r_item));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PUGL::IsTerminal() const
{
	PUGI * p_item;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(!(p_item->Flags & PUGI::fTerminal))
			return 0;
	return 1;
}

int SLAPI PUGL::AddAction(int16 a)
{
	if(ActionsCount < SIZEOFARRAY(Actions)) {
		Actions[ActionsCount++] = a;
		return 1;
	}
	else
		return 0;
}

void SLAPI PUGL::Clear()
{
	SupplAccSheetForSubstID = 0; // @v9.2.1
	CostByCalc  = 0;
	CalcCostPct = 0;
    freeAll();
    SupplSubstList.freeAll();
}

void SLAPI PUGL::ClearActions()
{
	ActionsCount = 0;
	memzero(Actions, sizeof(Actions));
}
//
//
//
class PuglSupplAssocDialog : public PPListDialog {
public:
	PuglSupplAssocDialog(PPID accSheetID, uint puglPos) : PPListDialog(DLG_PUGLSPLL, CTL_PUGLSPLL_LIST), AcsID(accSheetID), PuglPos(puglPos)
	{
	}
	int    setDTS(const PUGL * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		if(PuglPos <= Data.getCount()) {
			updateList(-1);
		}
		else
			ok = 0;
		return ok;
	}
	int    getDTS(PUGL * pData)
	{
		int    ok = 1;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		const  long fmt = MKSFMTD(0, 6, NMBF_NOTRAILZ);
		PUGI * p_item = 0;
		StringSet ss(SLBColumnDelim);
		SString sub;
		for(uint i = 0; i < Data.SupplSubstList.getCount(); i++) {
			const PUGL::SupplSubstItem & r_item = Data.SupplSubstList.at(i);
			if(r_item.Position == PuglPos) {
				ss.clear();
				GetArticleName(r_item.SupplID, sub);
				ss.add(sub);
				ss.add(r_item.QttyToStr(sub));
				THROW(addStringToList(i+1, ss.getBuf()));
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		int    _pos = -1;
		if(EditItem(&_pos) > 0) {
			long   _id = _pos+1;
			if(pPos) {
				long    inner_pos = 0;
				for(uint i = 0; i < Data.SupplSubstList.getCount(); i++) {
					const PUGL::SupplSubstItem & r_item = Data.SupplSubstList.at(i);
					if(r_item.Position == PuglPos) {
						inner_pos++;
						if((int)i == _pos) {
							*pPos = inner_pos;
							break;
						}
					}
				}
			}
			ASSIGN_PTR(pID, _id);
			ok = 1;
		}
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= (long)Data.SupplSubstList.getCount()) {
			int    _pos = id-1;
			if(EditItem(&_pos) > 0) {
				ok = 1;
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= (long)Data.SupplSubstList.getCount()) {
			Data.SupplSubstList.atFree(id-1);
			ok = 1;
		}
		return ok;
	}
	int    EditItem(int * pSupplSubstPos)
	{
		assert(pSupplSubstPos);
		int    ok = -1;
		uint   sel = 0;
		TDialog * dlg = 0;
		PUGL::SupplSubstItem item(PuglPos);
        if(*pSupplSubstPos >= 0) {
			THROW(*pSupplSubstPos < (int)Data.SupplSubstList.getCount());
			item = Data.SupplSubstList.at(*pSupplSubstPos);
			assert(item.Position == PuglPos);
        }
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PUGLSPLI)), 0));
		SetupArCombo(dlg, CTLSEL_PUGLSPLI_SUPPL, item.SupplID, 0, AcsID, sacfDisableIfZeroSheet);
		dlg->setCtrlReal(CTL_PUGLSPLI_QTTY, item.Qtty);
		dlg->AddClusterAssoc(CTL_PUGLSPLI_UNIT,  0, item.uAbs);
		dlg->AddClusterAssoc(CTL_PUGLSPLI_UNIT, -1, item.uAbs);
		dlg->AddClusterAssoc(CTL_PUGLSPLI_UNIT,  1, item.uPct);
		dlg->SetClusterData(CTL_PUGLSPLI_UNIT, item.Unit);
		while(ok < 0 && ExecView(dlg) == cmOK) {
            item.SupplID = dlg->getCtrlLong(sel = CTLSEL_PUGLSPLI_SUPPL);
            if(!item.SupplID)
				PPErrorByDialog(dlg, sel, PPERR_ARNEEDED);
			else {
				item.Unit = dlg->GetClusterData(sel = CTL_PUGLSPLI_UNIT);
				if(!oneof2(item.Unit, item.uAbs, item.uPct))
					PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
				else {
					item.Qtty = dlg->getCtrlReal(sel = CTL_PUGLSPLI_QTTY);
					if(item.Qtty <= 0.0) {
						PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
					}
					else if(item.Unit == item.uPct && item.Qtty > 100.0) {
						PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
					}
					else {
						if(*pSupplSubstPos >= 0) {
							Data.SupplSubstList.at(*pSupplSubstPos) = item;
						}
						else {
							THROW_SL(Data.SupplSubstList.insert(&item));
							*pSupplSubstPos = (int)(Data.SupplSubstList.getCount()-1);
						}
						ok = 1;
					}
				}
			}
		}
		CATCHZOKPPERR
		delete dlg;
		return ok;
	}
	const  uint PuglPos;
	const  PPID AcsID;
	PUGL   Data;
};

class PuglDialog : public PPListDialog {
public:
	PuglDialog(uint dlgID) : PPListDialog(dlgID, CTL_MSGNCMPL_LIST)
	{
		SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_MSGNCMPL_LIST);
		if(!SetupStrListBox(p_list))
			PPError();
		updateList(-1);
	}
	int    setDTS(const PUGL *);
	int    getDTS(PUGL *);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	void    setupCtrls();
	int     EditSupplAssoc(long selId)
	{
		int    ok = -1;
		PuglSupplAssocDialog * dlg = 0;
		if(Data.SupplAccSheetForSubstID && selId > 0 && selId <= (long)Data.getCount()) {
			dlg = new PuglSupplAssocDialog(Data.SupplAccSheetForSubstID, selId);
			if(CheckDialogPtr(&dlg, 1)) {
				if(dlg->setDTS(&Data)) {
					if(ExecView(dlg) == cmOK) {
						dlg->getDTS(&Data);
						ok = 1;
					}
				}
			}
		}
		delete dlg;
		return ok;
	}
	PUGL   Data;
};

IMPL_HANDLE_EVENT(PuglDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmMsgNCmplPrint)) {
		PPFilt pf;
		pf.Ptr = &Data;
		PPAlddPrint(REPORT_PUGL, &pf);
	}
	else if(event.isCmd(cmLBItemSelected)) {
		long   sel_id = 0;
		enableCommand(cmPuglSupplAssoc, (Data.SupplAccSheetForSubstID && getSelection(&sel_id) && sel_id > 0 && sel_id <= (long)Data.getCount()));
	}
	else if(event.isCmd(cmPuglSupplAssoc)) {
		long   sel_id = 0;
		if(getSelection(&sel_id) && sel_id > 0 && sel_id <= (long)Data.getCount()) {
			EditSupplAssoc(sel_id);
		}
	}
	else if(event.isClusterClk(CTL_MSGNCMPL_ACTION) || event.isClusterClk(CTL_MSGNCMPL_COSTALG))
		setupCtrls();
	else
		return;
	clearEvent(event);
}

void PuglDialog::setupCtrls()
{
	ushort v1 = 999, v2 = 0;
	if(Data.ActionsCount)
		getCtrlData(CTL_MSGNCMPL_ACTION, &v1);
	if(v1 != 0)
		setCtrlData(CTL_MSGNCMPL_COSTALG, &v2);
	else
		getCtrlData(CTL_MSGNCMPL_COSTALG, &v2);
	disableCtrl(CTL_MSGNCMPL_COSTALG, v1);
	disableCtrl(CTL_MSGNCMPL_CPCTVAL, !v2);
}

int PuglDialog::setDTS(const PUGL * pData)
{
	RVALUEPTR(Data, pData);
	ushort v = 0;
	for(uint i = 0; i < Data.ActionsCount; i++)
		if(Data.OPcug == Data.Actions[i]) {
			v = i;
			break;
		}
	setCtrlUInt16(CTL_MSGNCMPL_ACTION, v);
	disableCtrl(CTL_MSGNCMPL_ACTION, !Data.ActionsCount);
	setCtrlUInt16(CTL_MSGNCMPL_COSTALG, BIN(Data.CostByCalc));
	setCtrlData(CTL_MSGNCMPL_CPCTVAL, &Data.CalcCostPct);
	setupCtrls();
	updateList(-1);
	{
		long   sel_id = 0;
		enableCommand(cmPuglSupplAssoc, (Data.SupplAccSheetForSubstID && getSelection(&sel_id) && sel_id > 0 && sel_id <= (long)Data.getCount()));
	}
	return 1;
}

int PuglDialog::getDTS(PUGL * pData)
{
	int    ok = 1;
	ushort v = 999;
	Data.OPcug = (getCtrlData(CTL_MSGNCMPL_ACTION, &v) && v < Data.ActionsCount) ? Data.Actions[v] : PCUG_CANCEL;
	v = 0;
	if(Data.OPcug == PCUG_BALANCE)
		getCtrlData(CTL_MSGNCMPL_COSTALG, &v);
	Data.CostByCalc = v;
	if(Data.CostByCalc)
		getCtrlData(CTL_MSGNCMPL_CPCTVAL, &Data.CalcCostPct);
	else
		Data.CalcCostPct = 0.0;
	if(Data.CalcCostPct < 0.0 || Data.CalcCostPct > 100.0)
		ok = PPErrorByDialog(this, CTL_MSGNCMPL_CPCTVAL, PPERR_PERCENTINPUT);
	else
		ASSIGN_PTR(pData, Data);
	return ok;
}

int PuglDialog::setupList()
{
	int    ok = 1;
	const  long fmt = MKSFMTD(0, 6, NMBF_NOTRAILZ);
	PUGI * p_item = 0;
	StringSet ss(SLBColumnDelim);
	SString sub;
	for(uint i = 0; Data.enumItems(&i, (void**)&p_item);) {
		ss.clear();
		GetGoodsName(p_item->GoodsID, sub);
		ss.add(sub);
		ss.add((sub = 0).Cat(p_item->NeededQty,  fmt));
		ss.add((sub = 0).Cat(p_item->DeficitQty, fmt));
		THROW(addStringToList(i, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int SLAPI ProcessUnsuffisientList(uint dlgID, PUGL * pList)
{
	int    ret = PCUG_CANCEL, valid_data = 0;
	PuglDialog * dlg = new PuglDialog(NZOR(dlgID, DLG_MSGNCMPL3));
	if(CheckDialogPtr(&dlg, 1)) {
		dlg->setDTS(pList);
		while(!valid_data && ExecView(dlg) == cmOK)
			if(dlg->getDTS(pList)) {
				ret = pList->OPcug;
				valid_data = 1;
			}
	}
	delete dlg;
	return ret;
}

int SLAPI ProcessUnsuffisientGoods(PPID goods, PUGP param)
{
	uint   dlg_id;
	if(param == pugpFull)
		dlg_id = DLG_MSGNCMPL;
	else if(param == pugpZero)
		dlg_id = DLG_MSGZCMPL;
	else
		dlg_id = DLG_MSGNCMPL2;
	TDialog * dlg = new TDialog(dlg_id);
	if(CheckDialogPtr(&dlg, 1)) {
		ushort v = 0;
		SString fmt, buf, goods_name;
		if(dlg->getStaticText(CTL_MSGNCMPL_LINE1, fmt) > 0) {
			GetGoodsName(goods, goods_name);
			dlg->setStaticText(CTL_MSGNCMPL_LINE1, buf.Printf(fmt, (const char *)goods_name));
		}
		dlg->setCtrlData(CTL_MSGNCMPL_ACTION, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_MSGNCMPL_ACTION, &v);
			if(param == pugpZero) {
				if(v == 0)      v = PCUG_EXCLUDE;
				else if(v == 1) v = PCUG_CANCEL;
			}
			else if(param == pugpFull) {
				if(v == 0)      v = PCUG_BALANCE;
				else if(v == 1) v = PCUG_ASGOODSAS;
				else if(v == 2) v = PCUG_EXCLUDE;
				else            v = PCUG_CANCEL;
			}
			else {
				if(v == 0)      v = PCUG_ASGOODSAS;
				else if(v == 1) v = PCUG_EXCLUDE;
				else            v = PCUG_CANCEL;
			}
		}
		else
			v = PCUG_CANCEL;
		delete dlg;
		return (int)v;
	}
	return PCUG_CANCEL;
}

// @v9.4.0
int SLAPI PPGoodsStruc::InitCompleteData(PPID goodsID, double needQty, const PPBillPacket * pBillPack, PPComplBlock & rData)
{
	int    ok = 1;
	int    r;
	PPObjGoods     goods_obj;
	Goods2Tbl::Rec goods_rec;
	PPGoodsStrucItem gsi;
	rData.Head.GoodsID = goodsID;
	rData.Head.PartQty = 1.0;
	rData.Head.NeedQty = needQty;
	rData.Head.FreeQty = 0.0;
	// @v9.4.0 rData.Head.Cost    = 0.0;
	rData.Head.GoodsFlags = (goods_obj.Fetch(goodsID, &goods_rec) > 0) ? goods_rec.Flags : 0;
	double sqtty = 0.0;
	int    is_there_formula = 0;
	for(uint i = 0; (r = EnumItemsExt(&i, &gsi, goodsID, rData.Head.NeedQty, &sqtty)) > 0;) {
		ComplItem s;
		double qtty = 0.0;
		s.SrcGsPos = i;
		s.GsiFlags = gsi.Flags; // @v9.0.4
		// @v8.8.3 {
		// Строго говоря, условие sqtty==0.0 лишнее: факт присутствия формулы обязательно должен
		// влечь за собой расчет количества именно по формуле. Но для минимизации последствий
		// от ввода этого участка кода для существующих клиентов все-таки ограничение важно.
		//
		if(sqtty == 0.0 && gsi.Formula[0]) {
			s.GsiFlags |= GSIF_FORMULA;
			is_there_formula = 1;
			qtty = sqtty;
		}
		else {
			s.GsiFlags &= ~GSIF_FORMULA;
			qtty = sqtty;
		}
		// } @v8.8.3
		s.GoodsID = gsi.GoodsID;
		s.GoodsFlags = (goods_obj.Fetch(s.GoodsID, &goods_rec) > 0) ? goods_rec.Flags : 0;
		s.NeedQty = qtty;
		s.PartQty = fdivnz(qtty, rData.Head.NeedQty);
		s.FreeQty = 0.0;
		// @v9.4.0 s.Cost    = 0.0;
		THROW_SL(rData.insert(&s));
	}
	THROW(r);
	if(is_there_formula) {
		P_Cb = &rData;
		for(uint j = 0; j < rData.getCount(); j++) {
			ComplItem & r_item = rData.at(j);
			if(r_item.GsiFlags & GSIF_FORMULA && r_item.SrcGsPos > 0 && r_item.SrcGsPos <= Items.getCount()) {
				double v = 0.0;
				const  PPGoodsStrucItem & r_gsi = Items.at(r_item.SrcGsPos-1);
				GdsClsCalcExprContext ctx(this, pBillPack);
				THROW(PPCalcExpression(r_gsi.Formula, &v, &ctx));
				r_item.NeedQty = v;
				r_item.PartQty = fdivnz(v, rData.Head.NeedQty);
			}
		}
		P_Cb = 0;
	}
	CATCHZOK
	return ok;
}

// @v9.4.0
int SLAPI PPGoodsStruc::InitCompleteData2(PPID goodsID, double needQty, PPComplBlock & rData)
{
	int    ok = 1;
	const double need_qtty = needQty; //rTi.Quantity_;
	// @v9.4.0 {
	PPComplBlock temp_blk;
	THROW(InitCompleteData(goodsID, needQty, /*(const PPBillPacket *)*/0, temp_blk));
	for(uint j = 0; j < temp_blk.getCount(); j++) {
		const ComplItem & r_src_item = temp_blk.at(j);

		uint   pos = 0;
		ComplItem s;
		s.GoodsID = r_src_item.GoodsID;
		s.GsiFlags = r_src_item.GsiFlags;
		if(rData.lsearch(&s, &pos, CMPF_LONG)) {
			ComplItem & r_item = rData.at(pos);
			const double _tq = s.NeedQty + r_src_item.NeedQty;
			if(r_src_item.PartQty != 0.0) {
				const double _d = need_qtty + (r_src_item.NeedQty / r_src_item.PartQty);
				if(_d != 0.0)
					s.PartQty = _tq / _d;
			}
			s.NeedQty = _tq;
		}
		else {
			s.GoodsFlags = r_src_item.GoodsFlags;
			s.NeedQty    = r_src_item.NeedQty;
			s.PartQty    = fdivnz(s.NeedQty, need_qtty);
			s.FreeQty    = 0.0;
			THROW_SL(rData.insert(&s));
		}
	}
	// } @v9.4.0
#if 0 // @v9.4.0 {
	{
		PPObjGoods goods_obj;
		PPGoodsStrucItem gsi;
		int    r = 0;
		double qtty = 0.0;
		for(uint i = 0; (r = EnumItemsExt(&i, &gsi, labs(/*rTi.GoodsID*/goodsID), need_qtty, &qtty)) > 0;) {
			uint   pos = 0;
			ComplItem s;
			s.GoodsID = gsi.GoodsID;
			s.GsiFlags = gsi.Flags; // @v9.0.4
			if(rData.lsearch(&s, &pos, CMPF_LONG)) {
				ComplItem & r_item = rData.at(pos);
				r_item.NeedQty += (need_qtty * r_item.PartQty);
			}
			else {
				Goods2Tbl::Rec goods_rec;
				s.GoodsFlags = (goods_obj.Fetch(s.GoodsID, &goods_rec) > 0) ? goods_rec.Flags : 0;
				s.NeedQty    = qtty;
				s.PartQty    = fdivnz(qtty, need_qtty);
				s.FreeQty    = 0.0;
				// @v9.4.0 s.Cost       = 0.0;
				THROW_SL(rData.insert(&s));
			}
		}
		THROW(r);
	}
#endif // } 0 @v9.4.0
	CATCHZOK
	return ok;
}

int SLAPI PPBillPacket::InsertComplete(PPGoodsStruc * pGS, uint pos, PUGL * pDfctList,
	int processUnsuffisientQtty, const GoodsReplacementArray * pGra)
{
	int    ok = 1;
	int    user_cancel = 0; // Признак того, что ошибка возникла по причине отказа пользователя.
		// В этом случае функция возвращает -1
	uint   i;
	PPObjGoods goods_obj;
	IntArray positions;
	const  PPTransferItem * p_ti = & ConstTI(pos);
	int    sign = (p_ti->Flags & PPTFR_PLUS) ? -1 : 1;
	const  int out_price_by_quot = BIN(P_BObj->GetConfig().Flags & BCF_AUTOCOMPLOUTBYQUOT);
	double need_qty = p_ti->Quantity_;
	//
	// Расходная позиция может быть введена несколькими строками (автоматическое
	// использование несколькоих лотов). Поэтому, применяем трюк, благодаря которому,
	// автоматическая разукомплектация учтет все введенное израсходованное количество.
	//
	if(p_ti->Flags & PPTFR_MINUS) {
		if((i = pos) != 0) do {
			const PPTransferItem * p_prev_ti = & ConstTI(--i);
			if(p_prev_ti->GoodsID == p_ti->GoodsID && p_prev_ti->Flags & PPTFR_MINUS)
				need_qty += p_prev_ti->Quantity_;
			else
				i = 0;
		} while(i);
	}
	for(int try_again = 1; try_again == 1;) {
		try_again = 0;
		int    lim  = -1/*, zero = 0*/;
		double limq = SMathConst::Max;
		PPComplBlock ary;
		SString serial;
		SString src_serial; // @v9.1.4 Серийный номер, который используем для формирования серии новой позиции
		PUGL   deficit_list;
		PUGL * p_deficit_list = NZOR(pDfctList, &deficit_list);
		//ComplItem S, * ps;
		ComplItem * ps;
		THROW(pGS->InitCompleteData(p_ti->GoodsID, need_qty, this, /*&S,*/ ary));
		for(i = 0; ary.enumItems(&i, (void**)&ps);) {
			if(ps->PartQty != 0.0 && ps->NeedQty != 0.0) {
				ILTI   ilti;
				long   convert_ilti_flags = CILTIF_OPTMZLOTS|CILTIF_USESUBST;
				serial = 0;
				double out_quot  = 0.0;
				double out_price = 0.0;
				ilti.GoodsID = ps->GoodsID;
				if(sign > 0) {
					ilti.SetQtty(ps->NeedQty, 0, PPTFR_RECEIPT | PPTFR_PLUS);
					SETFLAG(ilti.Flags, PPTFR_COSTWOVAT, pGS->Rec.Flags & GSF_OUTPWOVAT);
					if(ps->GoodsFlags & GF_UNLIM) {
						QuotIdent qi(Rec.LocID, PPQUOTK_BASE, 0L /* @curID */);
						if(goods_obj.GetQuot(ilti.GoodsID, qi, 0, 0, &out_quot) <= 0)
							out_quot = 0.0;
					}
					else {
						if(out_price_by_quot) {
							QuotIdent qi(Rec.LocID, PPQUOTK_BASE, 0L /* @curID */);
							if(goods_obj.GetQuot(ilti.GoodsID, qi, 0, 0, &out_quot) <= 0)
								out_quot = 0.0;
						}
						ReceiptTbl::Rec lot_rec;
						THROW(::GetCurGoodsPrice(ilti.GoodsID, Rec.LocID, GPRET_MOSTRECENT, &out_price, &lot_rec));
						ilti.UnitPerPack = lot_rec.UnitPerPack;
					}
					ilti.Price = NZOR(out_quot, out_price);
				}
				else {
					// @v8.8.6 {
					if(ps->GoodsFlags & GF_UNLIM) {
						QuotIdent qi(Rec.LocID, PPQUOTK_BASE, 0L /* @curID */);
						if(goods_obj.GetQuot(ilti.GoodsID, qi, 0, 0, &out_quot) <= 0)
							out_quot = 0.0;
						ilti.Cost = out_quot;
						ilti.Price = out_quot;
					} // } @v8.8.6
					else if(ps->GsiFlags & GSIF_QUERYEXPLOT) {
						// @v9.0.4 {
						int    r = 0;
						ReceiptTbl::Rec lot_rec;
						// @v9.3.6 {
						if(LTagL.GetTagStr(pos, PPTAG_LOT_SOURCESERIAL, serial) > 0 && P_BObj->SelectLotBySerial(serial, ps->GoodsID, Rec.LocID, &lot_rec) > 0) {
							src_serial = serial;
							convert_ilti_flags = 0;
							r = 1;
						}
						else {
							PPObjBill::SelectLotParam slp(ps->GoodsID, Rec.LocID, 0, PPObjBill::SelectLotParam::fNotEmptySerial);
							slp.Period.Set(ZERODATE, Rec.Dt);
							if(P_BObj->SelectLot2(slp) > 0) {
								serial = slp.RetLotSerial;
								src_serial = serial;
								convert_ilti_flags = 0;
								r = 1;
							}
						}
						// } @v9.3.6 
						/* @v9.3.6
						PPID   lot_id = 0;
						{
							uint   s = 0;
							ReceiptCore & r_rcpt = P_BObj->trfr->Rcpt;
							SArray * p_ary = SelLotBrowser::CreateArray();
							for(DateIter diter(0, Rec.Dt); r_rcpt.EnumLots(ps->GoodsID, Rec.LocID, &diter, &lot_rec) > 0;) {
								double rest = lot_rec.Rest;
								P_BObj->GetSerialNumberByLot(lot_rec.ID, serial = 0, 1);
								if(serial.NotEmpty())
									SelLotBrowser::AddItemToArray(p_ary, &lot_rec, Rec.Dt, rest, 1);
							}
							serial = 0;
							if(p_ary->getCount()) {
								SelLotBrowser::Entry * p_sel = 0;
								SelLotBrowser * p_brw = new SelLotBrowser(P_BObj, p_ary, s, ps->GoodsID, 0);
								if(ExecView(p_brw) == cmOK && (p_sel = (SelLotBrowser::Entry *)p_brw->view->getCurItem()) != 0) {
									if(strip(p_sel->Serial)[0] != 0) {
										serial = p_sel->Serial;
										src_serial = serial; // @v9.1.4
										convert_ilti_flags = 0;
										r = 1;
									}
								}
								ZDELETE(p_brw);
							}
							else
								ZDELETE(p_ary);
						}
						*/
						if(r <= 0) {
							user_cancel = 1;
							CALLEXCEPT();
						}
						// } @v9.0.4
					}
					ilti.SetQtty(ps->NeedQty, 0.0, PPTFR_MINUS);
				}
				THROW(P_BObj->ConvertILTI(&ilti, this, &positions, convert_ilti_flags, serial, pGra));
				ps->FreeQty = ps->NeedQty - fabs(ilti.Rest);
				if(ilti.HasDeficit()) {
					THROW(p_deficit_list->Add(&ilti, Rec.LocID, i-1));
					if((ps->FreeQty / ps->PartQty) < limq) {
						lim  = i-1;
						limq = ps->FreeQty / ps->PartQty;
					}
				}
			}
		}
		if(lim != -1) {
			ComplItem & t = ary.at(lim);
			int    r = processUnsuffisientQtty;
			if(r == 0) {
				p_deficit_list->ClearActions();
				p_deficit_list->AddAction(PCUG_BALANCE);
				p_deficit_list->AddAction(PCUG_ASGOODSAS);
				p_deficit_list->AddAction(PCUG_EXCLUDE);
				p_deficit_list->AddAction(PCUG_CANCEL);
				p_deficit_list->OPcug = PCUG_CANCEL;
				r = ProcessUnsuffisientList(0, p_deficit_list);
			}
			if(oneof2(r, PCUG_BALANCE, PCUG_EXCLUDE)) {
				if(r == PCUG_BALANCE) {
					ary.Head.NeedQty = t.FreeQty / t.PartQty;
					if(ary.Head.GoodsFlags & GF_INTVAL)
						ary.Head.NeedQty = fint(ary.Head.NeedQty);
					TI(pos).Quantity_ = fabs(ary.Head.NeedQty);
				}
				else
					pGS->Items.at(lim).Median = 0;
				RemoveRows(&positions);
				if(ary.Head.NeedQty == 0) {
					RemoveRow(pos);
					ok = (PPSetError(PPERR_AUTOCOMPLREST), -1);
				}
				else {
					need_qty = ary.Head.NeedQty;
					try_again = 1;
				}
			}
			else if(r == PCUG_CANCEL) {
				RemoveRows(&positions);
				RemoveRow(pos);
				ok = (PPSetError(PPERR_AUTOCOMPLREST), -1);
			}
			/*
			else if(r == PCUG_ASGOODSAS)
				;
			*/
		}
		// @v9.1.4 {
		if(ok > 0 && src_serial.NotEmpty()) {
			SnL.GetNumber(pos, &serial);
			if(serial.Empty()) {
				// @todo Следует формировать новую серию по какому-либо шаблону
				(serial = src_serial).CatChar('-').Cat("???");
				SnL.AddNumber(pos, serial);
			}
		}
		// } @v9.1.4
	}
	CATCH
		ok = user_cancel ? -1 : 0;
		RemoveRows(&positions);
		RemoveRow(pos);
	ENDCATCH
	SetQuantitySign(-1);
	return ok;
}

int SLAPI PPBillPacket::InsertPartitialStruc()
{
	int    ok = -1;
	if(oneof6(OprType, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_GOODSMODIF)) {
		uint   i, j;
		PPTransferItem * p_ti;
		RAssocArray psr_array;
		RAssocArray cost_array; // Список себестоимостей отдельных позиций
		RAssocArray sub_list;
		RAssocArray sum_array;  // В этом массиве собираются суммы по тем компонентам, которые
			// рассчитываются по правилу "количество - 1, цена - процент" (GSIF_QTTYASPRICE)
		LongArray positions, local_pos_list;
		const  int sign = (oneof2(OprType, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) ? 1 : -1;
		PPObjGoods goods_obj;
		for(i = 0; EnumTItems(&i, &p_ti);) {
			PPGoodsStruc gs;
			PPGoodsStrucItem gsi;
			//
			// Локальный список позиций необходим для случая, если количество рассчитывается по
			// формуле, заданной в компоненте структуры. При этом каждая строка документа должна
			// обсчитываться независимо (нельзя суммировать строки с одинаковыми товарами).
			//
			local_pos_list.clear();
			if(!positions.lsearch(i) && !(p_ti->Flags & PPTFR_PARTSTRUSED)) {
				PPGoodsStruc::Ident gs_ident(p_ti->GoodsID, GSF_PARTITIAL, 0, Rec.Dt);
				if(LoadGoodsStruc(&gs_ident, &gs) > 0) {
					RAssocArray local_qtty_list;
					double qtty = p_ti->Qtty();
					double src_qtty  = qtty;
					double sum_cost = p_ti->Cost * qtty;
					double sum_price = p_ti->NetPrice() * qtty;
					local_qtty_list.Add(i, qtty);
					positions.add(i); // Отметим, что строка j уже нами рассмотрена
					local_pos_list.add(i);
					//
					// Собираем все строки документа по одному товару
					// и получаем общее количество. Это необходимо для //
					// расчета количества элементов структуры, которые заданы
					// с флагом отбрасывания дробной части.
					//
					PPTransferItem * p_ti2 = 0;
					for(j = i; EnumTItems(&j, &p_ti2);) {
						if(p_ti2->GoodsID == p_ti->GoodsID) {
							qtty = p_ti2->Qtty();
							src_qtty  += qtty;
							sum_cost += p_ti2->Cost;
							sum_price += p_ti2->NetPrice() * qtty;
							local_qtty_list.Add(j, qtty);
							positions.add(j); // Отметим, что строка j уже нами рассмотрена
							local_pos_list.add(j);
						}
					}
					//
					int    r = 0;
					qtty = 0.0;
					for(j = 0; (r = gs.EnumItemsExt(&j, &gsi, p_ti->GoodsID, fabs(src_qtty), &qtty)) > 0;) {
						if(gsi.Flags & GSIF_QTTYASPRICE) {
							double price = 0.0;
							if(gsi.GetQttyAsPrice(sum_price, &price) > 0)
								THROW_SL(sum_array.Add(gsi.GoodsID, price, 1));
						}
						else {
							if(gsi.Formula[0]) {
								double v = 0.0;
								qtty = 0.0;
								for(uint k = 0; k < local_pos_list.getCount(); k++) {
									GdsClsCalcExprContext ctx(&TI(local_pos_list.get(k)-1), this);
									THROW(PPCalcExpression(gsi.Formula, &v, &ctx));
									qtty += v;
								}
							}
							if(qtty != 0.0) {
								THROW_SL(psr_array.Add(gsi.GoodsID, qtty, 1));
								if(gsi.Flags & GSIF_SUBPARTSTR) {
									RAssocArray local_sub_list;
									local_qtty_list.Distribute(qtty, RAssocArray::dfRound|RAssocArray::dfReset, 6, local_sub_list);
									sub_list.Add(local_sub_list, 0, 0);
									cost_array.Add(gsi.GoodsID, local_sub_list.GetTotal() * sum_cost / src_qtty);
								}
							}
						}
					}
					THROW(r);
				}
			}
		}
		{
			IntArray pos_list;
			const uint psr_count = psr_array.getCount();
			for(i = 0; i < psr_count + sum_array.getCount(); i++) {
				const  int is_sum_val = (i < psr_count) ? 0 : 1;
				const  RAssoc & r_item = is_sum_val ? sum_array.at(i - psr_count) : psr_array.at(i);
				const  int    last_pos = pos_list.getCount();
				const  double qtty = is_sum_val ? 1.0 : r_item.Val;
				if(qtty != 0.0) {
					ILTI   ilti;
					ilti.GoodsID = r_item.Key;
					if(sign > 0) {
						ReceiptTbl::Rec lot_rec;
						THROW(::GetCurGoodsPrice(ilti.GoodsID, Rec.LocID, 0, &ilti.Price, &lot_rec));
						if(cost_array.Get(ilti.GoodsID) > 0.0 && !(ProcessFlags & pfSubCostOnSubPartStr))
							ilti.Cost = cost_array.Get(ilti.GoodsID) / qtty;
						else
							ilti.Cost = is_sum_val ? r_item.Val : R5(lot_rec.Cost);
					}
					else if(is_sum_val)
						ilti.Price = r_item.Val;
					if(!(is_sum_val && ilti.Price > 0.0) && (goods_obj.CheckFlag(ilti.GoodsID, GF_UNLIM) || ilti.Price == 0.0)) {
						QuotIdent qi(Rec.LocID, PPQUOTK_BASE, Rec.CurID);
						goods_obj.GetQuotExt(ilti.GoodsID, qi, 0, 0, &ilti.Price, 1);
					}
					ilti.SetQtty(qtty, 0, (sign > 0) ? (PPTFR_RECEIPT|PPTFR_PLUS) : PPTFR_MINUS);
					THROW(P_BObj->ConvertILTI(&ilti, this, &pos_list, CILTIF_OPTMZLOTS | CILTIF_ABSQTTY, 0));
					ok = 1;
					if(ilti.Rest != 0.0) {
						int    reply = ProcessUnsuffisientGoods(ilti.GoodsID, pugpNoBalance);
						if(reply == PCUG_EXCLUDE)
							RemoveRows(&pos_list, last_pos);
						else if(reply == PCUG_CANCEL) {
							RemoveRows(&pos_list);
							sub_list.freeAll();
							positions.freeAll();
							break;
						}
						/*
						else if(reply == PCUG_ASGOODSAS)
							;
						*/
					}
				}
			}
			if(sub_list.getCount()) {
				//
				// Если какие-то компоненты структуры должны вычитаться из количества
				// исходного товара, то осуществляем вычитание, предварительно проверив
				// невозможность перехода начального количества через ноль.
				//
				int too_big = 0;
				for(i = 0; i < sub_list.getCount(); i++) {
					const RAssoc & r_item = sub_list.at(i);
					if(r_item.Val > ConstTI(r_item.Key-1).Qtty()) {
						too_big = 1;
						break;
					}
				}
				if(!too_big) {
					for(i = 0; i < sub_list.getCount(); i++) {
						const RAssoc & r_item = sub_list.at(i);
						TI(r_item.Key-1).Quantity_ -= r_item.Val;
					}
				}
			}
			for(i = 0; i < positions.getCount(); i++) {
				TI(positions.get(i)-1).Flags |= PPTFR_PARTSTRUSED;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillPacket::RemoveAutoComplRow(uint pos)
{
	uint   acpos = 0;
	if(pos < GetTCount()) {
		PPTransferItem * p_ti = & TI(pos);
		PPID   ac_lot_id = p_ti->LotID;
		if(ac_lot_id < 0 && p_ti->Flags & PPTFR_AUTOCOMPL) {
			if(P_ACPack->SearchLot(ac_lot_id, &acpos)) do {
				P_ACPack->RemoveRow(acpos);
			} while(acpos < P_ACPack->GetTCount() && P_ACPack->TI(acpos).LotID >= 0);
			P_ACPack->CalcModifCost();
			p_ti->LotID = 0;
		}
	}
	return 1;
}

int SLAPI PPBillPacket::UpdateAutoComplRow(uint pos, int pcug)
{
	int    ok = 1;
	uint   acpos = 0;
	if(pos < GetTCount()) {
		const  PPTransferItem & r_ti = ConstTI(pos);
		PPID   aclot_id = r_ti.LotID;
		if(r_ti.Flags & PPTFR_AUTOCOMPL && aclot_id < 0 && P_ACPack->SearchLot(aclot_id, &acpos)) {
			const PPTransferItem & r_acti = P_ACPack->ConstTI(acpos);
			if(fabs(r_acti.Quantity_) != fabs(r_ti.Quantity_) || r_acti.Price != r_ti.Price) {
				THROW(RemoveAutoComplRow(pos));
				THROW(InsertAutoComplRow(pos, pcug));
				P_ACPack->CalcModifCost();
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillPacket::InsertAutoComplRow(uint pos, int pcug)
{
	int    ok = 1;
	uint   acpos;
	PPID   ac_lot_id;
	PPGoodsStruc   gs;
	PPTransferItem loti;
	PPTransferItem & r_ti = TI(pos);
	if(r_ti.Flags & PPTFR_AUTOCOMPL) {
		PPGoodsStruc::Ident gs_ident(r_ti.GoodsID, 0, GSF_PARTITIAL, Rec.Dt);
		THROW(LoadGoodsStruc(&gs_ident, &gs) > 0);
		if(!P_ACPack)
			THROW(InitACPacket());
		THROW(loti.Init(&P_ACPack->Rec, 1, TISIGN_PLUS));
		THROW(loti.SetupGoods(r_ti.GoodsID));
		loti.UnitPerPack = r_ti.UnitPerPack;
		if(!(P_BObj->GetConfig().Flags & BCF_DONTINHQCERT)) // @v8.2.5
			P_BObj->trfr->Rcpt.GetLastQCert(labs(r_ti.GoodsID), &loti.QCert);
		loti.Quantity_ = fabs(r_ti.Quantity_);
		loti.Cost     = 0.0;
		loti.Price    = r_ti.Price;
		for(ac_lot_id = -1; P_ACPack->SearchLot(ac_lot_id, 0);)
			ac_lot_id--;
		r_ti.LotID = ac_lot_id;
		loti.LotID = ac_lot_id;
		acpos = P_ACPack->GetTCount();
		THROW(P_ACPack->InsertRow(&loti, 0));
		THROW(P_ACPack->InsertComplete(&gs, acpos, 0, pcug) > 0);
		r_ti.Quantity_ = P_ACPack->ConstTI(acpos).Quantity_;
		P_ACPack->CalcModifCost();
	}
	CATCH
		RemoveRow(pos);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPBillPacket::InitACPacket()
{
	int    ok = 1;
	PPOprKind op_rec;
	THROW_PP(CConfig.AutoComplOp, PPERR_UNDEFAUTOCOMPLOP);
	THROW_PP(GetOpData(CConfig.AutoComplOp, &op_rec) > 0, PPERR_UNDEFAUTOCOMPLOP);
	THROW_PP(op_rec.OpTypeID == PPOPT_GOODSMODIF, PPERR_INVAUTOCOMPLOP);
	ZDELETE(P_ACPack);
	THROW_MEM(P_ACPack = new PPBillPacket);
	THROW(P_ACPack->CreateBlank(CConfig.AutoComplOp, 0, Rec.LocID, 1));
	P_ACPack->Rec.Dt    = Rec.Dt;
	P_ACPack->Rec.LocID = Rec.LocID;
	P_ACPack->P_Outer   = this;
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_PUGL
//
PPALDD_CONSTRUCTOR(PUGL)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PUGL)
{
	Destroy();
}

int PPALDD_PUGL::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PUGL::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	if(sortId >= 0)
		SortIdx = sortId;
	IterProlog(iterId, 1);
	I.nn = 0;
	return 1;
}

int PPALDD_PUGL::NextIteration(PPIterID iterId, long rsrv)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PUGL * p_list = (PUGL *)Extra[0].Ptr;
	PUGI * p_item = 0;
	uint   nn = (uint)I.nn;
	if(p_list->enumItems(&nn, (void**)&p_item)) {
		I.nn       = nn;
		I.GoodsID  = p_item->GoodsID;
		I.LocID    = p_item->LocID;
		I.NeedQtty = p_item->NeededQty;
		I.Deficit  = p_item->DeficitQty;
		ok = DlRtm::NextIteration(iterId, rsrv);
	}
	return ok;
}
