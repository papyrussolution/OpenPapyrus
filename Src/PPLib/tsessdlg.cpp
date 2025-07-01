// TSESSDLG.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

static constexpr int _TSesStatusTab[] = { TSESST_PLANNED, TSESST_PENDING, TSESST_INPROCESS, TSESST_CLOSED, TSESST_CANCELED };

PrcTechCtrlGroup::Rec::Rec() : PrcID(0), PrcParentID(0), TechID(0), ArID(0), Ar2ID(0), IdleStatus(false)
{
}

PrcTechCtrlGroup::PrcTechCtrlGroup(uint ctlSelPrc, uint ctlSelTech, uint ctlStGoods,
	uint ctlSelAr, uint ctlSelAr2, uint cmdSelTechByGoods, uint cmdCreateGoods) : CtrlGroup(), CtlselPrc(ctlSelPrc), CtlselTech(ctlSelTech), 
	CtlStGoods(ctlStGoods), CtlselAr(ctlSelAr), CtlselAr2(ctlSelAr2), CmdSelTechByGoods(cmdSelTechByGoods), CmdCreateGoods(cmdCreateGoods),
	SelGoodsID(0), AutoGoodsGrpID(0), IdleStatus(false), Flags(0)
{
	Flags |= fEnablePrcSelUpLevel;
}

void PrcTechCtrlGroup::setIdleStatus(TDialog * pDlg, bool s)
{
	if(IdleStatus != s) {
		IdleStatus = s;
		pDlg->disableCtrl(CtlselTech, IdleStatus);
		pDlg->setCtrlLong(CtlselTech, 0);
		pDlg->enableCommand(CmdSelTechByGoods, !IdleStatus);
		onPrcSelection(pDlg, 1);
	}
}

void PrcTechCtrlGroup::enablePrcSelUpLevel(bool enbl) { SETFLAG(Flags, fEnablePrcSelUpLevel, enbl); }
void PrcTechCtrlGroup::enableTechSelUpLevel(bool enbl) { SETFLAG(Flags, fEnableTechSelUpLevel, enbl); }
void PrcTechCtrlGroup::enablePrcInsert(bool enbl) { SETFLAG(Flags, fEnablePrcInsert, enbl); }

void PrcTechCtrlGroup::setupArticle(TDialog * pDlg, const ProcessorTbl::Rec * pPrcRec)
{
	PPOprKind op_rec;
	GetOpData(pPrcRec->WrOffOpID, &op_rec);
	SetupArCombo(pDlg, CtlselAr, Data.ArID, OLW_LOADDEFONOPEN|OLW_CANINSERT, op_rec.AccSheetID, sacfDisableIfZeroSheet);
	bool   was_idle_sheet_set = false;
	if(IdleStatus) {
		PPTSessConfig cfg;
		PPObjTSession::ReadConfig(&cfg);
		if(cfg.IdleAccSheetID) {
			SetupArCombo(pDlg, CtlselAr2, Data.Ar2ID, OLW_LOADDEFONOPEN|OLW_CANINSERT, cfg.IdleAccSheetID, sacfDisableIfZeroSheet);
			was_idle_sheet_set = true;
		}
	}
	if(!was_idle_sheet_set) {
		PPID   acs_id = op_rec.AccSheet2ID;
		if(!acs_id) {
			PPObjProcessor prc_obj;
			ProcessorTbl::Rec prc_rec;
			if(prc_obj.GetRecWithInheritance(Data.PrcID, &prc_rec, 1) > 0 && prc_rec.Flags & PRCF_ADDEDOBJASAGENT)
				acs_id = GetAgentAccSheet();
		}
		SetupArCombo(pDlg, CtlselAr2, Data.Ar2ID, OLW_LOADDEFONOPEN|OLW_CANINSERT, acs_id, sacfDisableIfZeroSheet);
	}
}

void PrcTechCtrlGroup::onPrcSelection(TDialog * pDlg, int onIdleStatus)
{
	PPObjProcessor prc_obj;
	ProcessorTbl::Rec prev_prc_rec;
	if(prc_obj.GetRecWithInheritance(Data.PrcID, &prev_prc_rec, 1) <= 0)
		prev_prc_rec.Clear();
	if(onIdleStatus) {
		setupArticle(pDlg, &prev_prc_rec);
	}
	else {
		pDlg->getCtrlData(CtlselPrc, &Data.PrcID);
		if(Data.PrcID != prev_prc_rec.ID) {
			ProcessorTbl::Rec prc_rec;
			if(prc_obj.GetRecWithInheritance(Data.PrcID, &prc_rec, 1) <= 0)
				prc_rec.Clear();
			if(SelGoodsID) {
				TecObj.CreateAutoTech(Data.PrcID, SelGoodsID, 0, 1);
				PPObjTech::SetupCombo(pDlg, CtlselTech, Data.TechID, GetTechComboOlwFlags(), Data.PrcID, SelGoodsID);
			}
			else
				SetupPPObjCombo(pDlg, CtlselTech, PPOBJ_TECH, Data.TechID, GetTechComboOlwFlags() | OLW_SETUPSINGLE, reinterpret_cast<void *>(Data.PrcID));
			setupGoodsName(pDlg);
			if(prc_rec.WrOffOpID != prev_prc_rec.WrOffOpID)
				setupArticle(pDlg, &prc_rec);
		}
	}
	setupCreateGoodsButton(pDlg);
}

void PrcTechCtrlGroup::selTechByGoods(TDialog * pDlg)
{
	const  PPID prc_id = pDlg->getCtrlLong(CtlselPrc);
	if(prc_id) {
		const long egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
		ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, 0, egsd_flags);
		if(CheckDialogPtrErr(&dlg)) {
			PPIDArray goods_id_list;
			TIDlgInitData tidi;
			TecObj.GetGoodsListByPrc(prc_id, &goods_id_list);
			dlg->setSelectionByGoodsList(&goods_id_list);
			if(goods_id_list.lsearch(SelGoodsID))
				tidi.GoodsID = SelGoodsID;
			dlg->setDTS(&tidi);
			if(ExecView(dlg) == cmOK)
				if(dlg->getDTS(&tidi) > 0) {
					SelGoodsID = tidi.GoodsID;
					if(SelGoodsID)
						PPObjTech::SetupCombo(pDlg, CtlselTech, /*Data.TechID*/0, GetTechComboOlwFlags(), Data.PrcID, SelGoodsID);
					else
						SetupPPObjCombo(pDlg, CtlselTech, PPOBJ_TECH, Data.TechID, GetTechComboOlwFlags(), reinterpret_cast<void *>(Data.PrcID));
					setupGoodsName(pDlg);
					TView::messageCommand(pDlg, cmCBSelected, pDlg->getCtrlView(CtlselTech));
				}
			delete dlg;
		}
	}
}

int PrcTechCtrlGroup::getGoodsID(TDialog * pDlg, PPID * pGoodsID)
{
	int    ok = -1;
	PPID   goods_id = 0;
	TechTbl::Rec tec_rec;
	pDlg->getCtrlData(CtlselTech, &Data.TechID);
	if(TecObj.Search(Data.TechID, &tec_rec) > 0) {
		goods_id = tec_rec.GoodsID;
		if(goods_id > 0)
			ok = 1;
	}
	ASSIGN_PTR(pGoodsID, goods_id);
	return ok;
}

void PrcTechCtrlGroup::setupGoodsName(TDialog * pDlg)
{
	TechTbl::Rec tec_rec;
	pDlg->getCtrlData(CtlselTech, &Data.TechID);
	SString goods_name;
	if(TecObj.Search(Data.TechID, &tec_rec) > 0)
		if(GetGoodsNameR(tec_rec.GoodsID, goods_name) <= 0) {
			SetupPPObjCombo(pDlg, CtlselTech, PPOBJ_TECH, Data.TechID, GetTechComboOlwFlags(), reinterpret_cast<void *>(Data.PrcID));
			SelGoodsID = 0;
		}
	pDlg->setStaticText(CtlStGoods, goods_name);
}

void PrcTechCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCbSelected(CtlselPrc))
		onPrcSelection(pDlg);
	else if(event.isCbSelected(CtlselTech))
		setupGoodsName(pDlg);
	else if(event.isCmd(CmdSelTechByGoods))
		selTechByGoods(pDlg);
	else if(event.isCmd(CmdCreateGoods) && AutoGoodsGrpID) {
		PPID   goods_id = 0;
		if(GObj.Edit(&goods_id, reinterpret_cast<void *>(AutoGoodsGrpID)) == cmOK && goods_id) {
			if(!Data.TechID && pDlg->getCtrlLong(CtlselTech) == 0) {
				SelGoodsID = goods_id;
				PPObjTech::SetupCombo(pDlg, CtlselTech, 0, GetTechComboOlwFlags(), Data.PrcID, SelGoodsID);
				setupGoodsName(pDlg);
				TView::messageCommand(pDlg, cmCBSelected, pDlg->getCtrlView(CtlselTech));
			}
		}
	}
}

void PrcTechCtrlGroup::setupCreateGoodsButton(TDialog * pDlg)
{
	AutoGoodsGrpID = 0;
	if(CmdCreateGoods) {
		if(Data.PrcID && !IdleStatus && !Data.TechID) {
			PPID   goods_grp_id = 0;
			if(TecObj.SearchAutoForGoodsCreation(Data.PrcID, &goods_grp_id) > 0) {
				Goods2Tbl::Rec goods_rec;
				if(GObj.Fetch(goods_grp_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GROUP) {
					AutoGoodsGrpID = goods_grp_id;
				}
			}
		}
		pDlg->showButton(CmdCreateGoods, BIN(AutoGoodsGrpID));
		pDlg->enableCommand(CmdCreateGoods, BIN(AutoGoodsGrpID));
	}
}

long PrcTechCtrlGroup::GetTechComboOlwFlags() const // @v11.7.6
{
	long   olw = 0;
	if(Flags & fEnableTechSelUpLevel)
		olw |= OLW_CANSELUPLEVEL;
	return olw;
}

int PrcTechCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = 1;
	PPObjProcessor prc_obj;
	if(pData) {
		Data = *static_cast<Rec *>(pData);
		IdleStatus = Data.IdleStatus;
	}
	{
		long   olw = 0;
		if(Flags & fEnablePrcSelUpLevel)
			olw |= OLW_CANSELUPLEVEL;
		if(Flags & fEnablePrcInsert)
			olw |= OLW_CANINSERT;
		SetupPPObjCombo(pDlg, CtlselPrc, PPOBJ_PROCESSOR, Data.PrcID, olw, reinterpret_cast<void *>(Data.PrcParentID));
	}
	pDlg->disableCtrl(CtlselTech, IdleStatus);
	if(IdleStatus)
		Data.TechID = 0;
	pDlg->enableCommand(CmdSelTechByGoods, !IdleStatus);
	ProcessorTbl::Rec prc_rec;
	long   prc_ext_param = Data.PrcParentID;
	int    r = prc_obj.GetRecWithInheritance(Data.PrcID, &prc_rec);
	if(r > 0) {
		SetupPPObjCombo(pDlg, CtlselTech, PPOBJ_TECH, Data.TechID, GetTechComboOlwFlags() | OLW_SETUPSINGLE, reinterpret_cast<void *>(Data.PrcID));
		setupGoodsName(pDlg);
		setupArticle(pDlg, &prc_rec);
	}
	else {
		if(!r)
			ok = 0;
	}
	setupCreateGoodsButton(pDlg);
	return ok;
}

int PrcTechCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	pDlg->getCtrlData(CtlselPrc, &Data.PrcID);
	pDlg->getCtrlData(CtlselTech, &Data.TechID);
	pDlg->getCtrlData(CtlselAr, &Data.ArID);
	pDlg->getCtrlData(CtlselAr2, &Data.Ar2ID);
	if(pData)
		*static_cast<Rec *>(pData) = Data;
	return 1;
}
//
//
//
class TSessionDialog : public TDialog {
	enum {
		ctlgroupPrcTech = 2
	};
public:
	explicit TSessionDialog(uint dlgID) : TDialog(dlgID), SessUpdated(false), InnerGetDTS(false), InpUpdLock(0), P_BObj(BillObj)
	{
		SetupCalDate(CTLCAL_TSESS_STDT, CTL_TSESS_STDT);
		SetupCalDate(CTLCAL_TSESS_FNDT, CTL_TSESS_FNDT);
		SetupCalPeriod(CTLCAL_TSESS_PLANPERIOD, CTL_TSESS_PLANPERIOD);
		SetupTimePicker(this, CTL_TSESS_STTM, CTLTM_TSESS_STTM);
		SetupTimePicker(this, CTL_TSESS_FNTM, CTLTM_TSESS_FNTM);
		addGroup(ctlgroupPrcTech, new PrcTechCtrlGroup(CTLSEL_TSESS_PRC, CTLSEL_TSESS_TECH, CTL_TSESS_ST_GOODS,
			CTLSEL_TSESS_OBJ, CTLSEL_TSESS_OBJ2, cmSelTechByGoods, cmCreateGoods));
		if(!(TSesObj.GetConfig().Flags & PPTSessConfig::fUsePricing))
			showCtrl(CTL_TSESS_AMOUNT, false);
		MEMSZERO(TB);
		TB.ToolingTime = -1;
	}
	int    setDTS(const TSessionPacket *);
	int    getDTS(TSessionPacket *);
	bool   GetUpdatedStatus(PPID * pID) const
	{
		ASSIGN_PTR(pID, Data.Rec.ID);
		return SessUpdated;
	}
	PPID   GetCCheckID() const { return Data.Rec.CCheckID_; }
private:
	DECL_HANDLE_EVENT;
	void   SetupSCard();
	//
	// ARG(master IN): 0 - ведущее значение - продолжительность, 1 - ведущее значение - время окончания //
	//
	int    SetupTiming(int master);
	void   SetupCapacity();
	void   setupOrder();
	void   SetupTech(int force);
	void   selectFreeEntry();
	long   getToolingTiming();
	void   Detail();
	void   SetupCCheckButton();
	void   SetupCipAndRepButton();
	void   SetupAutoFillButton();
	void   SetPlannedTiming(long sec);
	long   GetPlannedTiming();
	int    SetupPayment();
	int    AddPayment();
	void   More();
	int    IsAddCompletionAvailable();
	int    AddCompletion();

	TSessionPacket Data;
	int    OrgStatus;
	bool   SessUpdated; // Получает значение !0, если сессия была сохранена в процессе редактирования (переход в режим чека, либо переход в строки).
	bool   InnerGetDTS;
	uint8  Reserve[2];  // @alignment
	long   InpUpdLock;
	//
	// Блок параметров перенстройки процессора.
	// Из-за того, что извлечение параметров перенастройки - достаточно дорогая с точки зрения //
	// производительности операция, мы сохраним в этом блоке те параметры сессии, от которых зависят
	// характеристики перенастройки, дабы извлекать их только при изменении указанных параметров.
	//
	struct ToolingBlock {
		LDATETIME Start;
		PPID   PrcID;
		PPID   TechID;
		long   ToolingTime; // Если -1, то перенастройка не определена
	} TB;
	PPObjTSession TSesObj;
	PPObjSCard ScObj;
	PPObjBill * P_BObj;
};

int TSessionDialog::SetupPayment()
{
	int    ok = -1;
	SString text_buf, temp_buf;
	if(Data.Rec.ID) {
		if(Data.Rec.PrcID) {
			ProcessorTbl::Rec prc_rec;
			double debt, paym_bill, cc_paym;
			double debt_netto = 0.0;
			TSesObj.CalcBalance(Data.Rec.ID, &debt, &paym_bill, &cc_paym);
			debt_netto = debt - paym_bill - cc_paym;
			if(paym_bill != 0.0) {
				text_buf.Cat(PPLoadStringS("payment", temp_buf)).CatDiv(':', 2).Cat(paym_bill, SFMT_MONEY).CRB();
			}
			if(cc_paym != 0.0) {
				text_buf.Cat(PPLoadStringS("paymentcc", temp_buf)).CatDiv(':', 2).Cat(cc_paym, SFMT_MONEY).CRB();
			}
			if(debt_netto != 0.0) {
				text_buf.Cat(PPLoadStringS("debt", temp_buf)).CatDiv(':', 2).Cat(debt_netto, SFMT_MONEY).CRB();
			}
			if(TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 1, 1) > 0 && prc_rec.WrOffOpID) {
				PPOprKind op_rec;
				if(GetOpData(prc_rec.WrOffOpID, &op_rec) > 0 && op_rec.Flags & OPKF_NEEDPAYMENT) {
					ReckonOpArList op_list;
					PPIDArray finish_op_list;
					P_BObj->GetPaymentOpListByDebtOp(prc_rec.WrOffOpID, Data.Rec.ArID, &op_list);
					for(uint i = 0; i < op_list.getCount(); i++) {
						const ReckonOpArItem & r_item = op_list.at(i);
						if(r_item.PayableOpID)
							finish_op_list.addUnique(r_item.PayableOpID);
					}
					if(finish_op_list.getCount()) {
						ok = 1;
					}
				}
			}
		}
	}
	setCtrlString(CTL_TSESS_PAYMINFO, text_buf);
	showCtrl(CTL_TSESS_ADDPAYMBUTT, (ok > 0));
	showCtrl(CTL_TSESS_PAYMINFO, (ok > 0 || text_buf.NotEmpty()));
	enableCommand(cmAddPaym, (ok > 0));
	showCtrl(STDCTL_BILLSBUTTON, LOGIC(Data.Rec.ID));
	enableCommand(cmBills, Data.Rec.ID);
	return ok;
}

int TSessionDialog::AddPayment()
{
	int    ok = -1;
	const  PPID preserve_loc_id = LConfig.Location;
	if(Data.Rec.ID && Data.Rec.PrcID) {
		ProcessorTbl::Rec prc_rec;
		if(TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 1, 1) > 0 && prc_rec.WrOffOpID) {
			PPOprKind op_rec;
			if(GetOpData(prc_rec.WrOffOpID, &op_rec) > 0 && op_rec.Flags & OPKF_NEEDPAYMENT) {
				ReckonOpArList op_list;
				PPIDArray finish_op_list; // Список зачетных операций, приемлемых для ввода документа оплаты
				P_BObj->GetPaymentOpListByDebtOp(prc_rec.WrOffOpID, Data.Rec.ArID, &op_list);
				for(uint i = 0; i < op_list.getCount(); i++) {
					const ReckonOpArItem & r_item = op_list.at(i);
					if(r_item.PayableOpID)
						finish_op_list.addUnique(r_item.PayableOpID);
				}
				if(finish_op_list.getCount()) {
					PPID   loc_id = prc_rec.LocID;
					PPID   op_id = 0;
					if(finish_op_list.getCount() == 1)
						op_id = finish_op_list.get(0);
					else if(BillPrelude(&finish_op_list, OPKLF_OPLIST, 0, &op_id, &loc_id) > 0) {
					}
					else
						op_id = 0;
					if(op_id && loc_id) {
						DS.SetLocation(loc_id);
						PPObjBill::AddBlock ab;
						ab.OpID = op_id;
						ab.ObjectID = Data.Rec.ArID;
						ab.PoolID = Data.Rec.ID;
						ab.Pk = PPBillPacket::bpkTSessPaym;
						PPID   bill_id = 0;
						int    r = P_BObj->AddGoodsBill(&bill_id, &ab);
						if(r > 0) {
							SetupPayment();
						}
					}
					ok = 1;
				}
			}
		}
	}
	DS.SetLocation(preserve_loc_id);
	return ok;
}

long TSessionDialog::getToolingTiming()
{
	getCtrlData(CTL_TSESS_STDT, &Data.Rec.StDt);
	getCtrlData(CTL_TSESS_STTM, &Data.Rec.StTm);
	if(TB.ToolingTime < 0 || TB.Start.d != Data.Rec.StDt || TB.Start.t != Data.Rec.StTm || TB.PrcID != Data.Rec.PrcID || TB.TechID != Data.Rec.TechID) {
		TB.Start.d = Data.Rec.StDt;
		TB.Start.t = Data.Rec.StTm;
		TB.PrcID  = Data.Rec.PrcID;
		TB.TechID = Data.Rec.TechID;
		TB.ToolingTime = 0;
		TSessionTbl::Rec prev_sess_rec;
		TechTbl::Rec tec_rec, prev_tec_rec;
		if(TSesObj.GetPrevSession(Data.Rec, &prev_sess_rec) > 0 && TSesObj.GetTech(Data.Rec.TechID, &tec_rec) > 0 &&
			TSesObj.GetTech(prev_sess_rec.TechID, &prev_tec_rec, 1) > 0) {
			//if(tec_rec.GoodsID != prev_tec_rec.GoodsID) {
				TSVector <TechTbl::Rec> t_list;
				PPObjTech tec_obj;
				if(tec_obj.SelectTooling(Data.Rec.PrcID, tec_rec.GoodsID, prev_tec_rec.GoodsID, &t_list) > 0)
					for(uint i = 0; i < t_list.getCount(); i++)
						TB.ToolingTime += t_list.at(i).Duration;
			//}
		}
	}
	return TB.ToolingTime;
}

void TSessionDialog::selectFreeEntry()
{
	class PrcFreeListDialog : public PPListDialog {
	public:
		PrcFreeListDialog() : PPListDialog(DLG_PRCFREELIST, CTL_PRCFREELIST_LIST), Capacity(0)
		{
		}
		int    setDTS(const PrcBusyArray * pBusyList, double capacity)
		{
			PrcBusyArray temp_list;
			temp_list.copy(*pBusyList);
			temp_list.Sort();
			FreeList.freeAll();
			temp_list.GetFreeList(&FreeList);
			FreeList.Sort(1);
			Capacity = capacity;
			updateList(-1);
			return 1;
		}
		int    getSelectedPeriod(LDATETIME * pStart, LDATETIME * pFinish)
		{
			long   pos = 0, id = 0;
			if(getCurItem(&pos, &id) && id > 0 && id <= FreeList.getCountI()) {
				const PrcBusy & entry = *static_cast<const PrcBusy *>(FreeList.at(id-1));
				ASSIGN_PTR(pStart, entry.Start);
				ASSIGN_PTR(pFinish, entry.Finish);
				return 1;
			}
			return 0;
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmLBDblClk)) {
				if(IsInState(sfModal))
					endModal(cmOK);
				clearEvent(event);
			}
			PPListDialog::handleEvent(event);
		}
		virtual int setupList()
		{
			int    ok = 1;
			StringSet  ss(SLBColumnDelim);
			SString sub;
			for(uint i = 0; i < FreeList.getCount(); i++) {
				const PrcBusy & entry = *static_cast<const PrcBusy *>(FreeList.at(i));
				ss.Z();
				ss.add(entry.ToStr(0, sub.Z()));
				const long dur = entry.GetDuration();
				sub.Z();
				if(dur >= 0) {
					LTIME tm_dur;
					tm_dur.settotalsec(dur);
					sub.Cat(tm_dur);
				}
				ss.add(sub);
				sub.Z();
				if(Capacity > 0 && dur > 0)
					sub.Cat(Capacity * dur, MKSFMTD(0, 3, NMBF_NOTRAILZ | NMBF_NOZERO));
				ss.add(sub);
				THROW(addStringToList(i+1, ss.getBuf()));
			}
			CATCHZOK
			return ok;
		}

		PrcBusyArray FreeList;
		double Capacity;
	};
	PrcFreeListDialog * dlg = 0;
	getCtrlData(CTLSEL_TSESS_PRC, &Data.Rec.PrcID);
	if(Data.Rec.PrcID) {
		if(CheckDialogPtrErr(&(dlg = new PrcFreeListDialog))) {
			TechTbl::Rec tec_rec;
			getCtrlData(CTLSEL_TSESS_TECH, &Data.Rec.TechID);
			double capacity = (TSesObj.GetTech(Data.Rec.TechID, &tec_rec, 1) > 0) ? tec_rec.Capacity : 0;
			PrcBusyArray busy_list;
			int    kind = 0;
			if(Data.Rec.Flags & TSESF_IDLE)
				kind = TSESK_IDLE;
			else if(Data.Rec.Flags & TSESF_PLAN)
				kind = TSESK_PLAN;
			else if(Data.Rec.Flags & TSESF_SUPERSESS)
				kind = TSESK_SUPERSESS;
			else if(Data.Rec.Flags & TSESF_SUBSESS)
				kind = TSESK_SUBSESS;
			else
				kind = TSESK_SESSION;
			TSesObj.P_Tbl->LoadBusyArray(Data.Rec.PrcID, Data.Rec.ID, kind, 0, &busy_list);
			dlg->setDTS(&busy_list, capacity);
			if(ExecView(dlg) == cmOK) {
				LDATETIME start, finish;
				if(dlg->getSelectedPeriod(&start, &finish) > 0) {
					setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, start);
					if(finish.IsFar())
						finish.Z();
					setCtrlDatetime(CTL_TSESS_FNDT, CTL_TSESS_FNTM, finish);
					SetupTiming(1);
				}
			}
		}
	}
	delete dlg;
}

void TSessionDialog::setupOrder()
{
	SString order_buf;
	ReceiptTbl::Rec lot_rec;
	if(Data.Rec.OrderLotID)
		if(P_BObj->trfr->Rcpt.Search(Data.Rec.OrderLotID, &lot_rec) > 0) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			const  PPID prev_ar_id = Data.Rec.ArID;
			const  PPID prev_ar2_id = Data.Rec.Ar2ID;
			SString temp_buf, serial;
			P_BObj->GetSerialNumberByLot(lot_rec.ID, serial, 0);
			if(ar_obj.Fetch(lot_rec.SupplID, &ar_rec) > 0) {
				ProcessorTbl::Rec prc_rec;
				PPOprKind op_rec;
				if(TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 1, 1) > 0 && GetOpData(prc_rec.WrOffOpID, &op_rec) > 0)
					if(op_rec.AccSheetID == ar_rec.AccSheetID) {
						if(!Data.Rec.ArID || !Data.Rec.ID)
							Data.Rec.ArID = lot_rec.SupplID;
					}
					else if(op_rec.AccSheet2ID == ar_rec.AccSheetID) {
						if(!Data.Rec.Ar2ID || !Data.Rec.ID)
							Data.Rec.Ar2ID = lot_rec.SupplID;
					}
				temp_buf = ar_rec.Name;
			}
			else
				ideqvalstr(lot_rec.SupplID, temp_buf);
			order_buf.Cat(serial).CatDivIfNotEmpty('-', 1).Cat(lot_rec.Dt).CatDiv('-', 1).Cat(temp_buf).
				CatDiv('-', 1).Cat(lot_rec.Rest, MKSFMTD(0, 3, NMBF_NOTRAILZ));
			if(Data.Rec.ArID != prev_ar_id || Data.Rec.Ar2ID != prev_ar2_id) {
				setCtrlLong(CTLSEL_TSESS_OBJ,  Data.Rec.ArID);
				setCtrlLong(CTLSEL_TSESS_OBJ2, Data.Rec.Ar2ID);
			}
		}
		else
			ideqvalstr(Data.Rec.OrderLotID, order_buf);
	setCtrlString(CTL_TSESS_ORDER, order_buf);
}

void TSessionDialog::SetupTech(int force)
{
	PPID   tech_id = Data.Rec.TechID;
	getCtrlData(CTLSEL_TSESS_TECH, &tech_id);
	if(force || tech_id != Data.Rec.TechID) {
		TechTbl::Rec prev_tec_rec, new_tec_rec;
		int    r = 1;
		if(TSesObj.GetTech(tech_id, &new_tec_rec, 1) > 0) {
			//
			// Если сессия не новая и изменился товар, то проверяем, не следует ли изменить товар
			// во всех строках сессии
			//
			if(Data.Rec.ID && TSesObj.GetTech(Data.Rec.TechID, &prev_tec_rec, 1) > 0) {
				if(new_tec_rec.GoodsID && prev_tec_rec.GoodsID && new_tec_rec.GoodsID != prev_tec_rec.GoodsID) {
					if(TSesObj.ReplaceGoodsInLines(Data.Rec.ID, prev_tec_rec.GoodsID, new_tec_rec.GoodsID, 0x0001, 0) > 0) {
						if(CONFIRM(PPCFM_REPLCTSESSGOODS)) {
							if(!TSesObj.ReplaceGoodsInLines(Data.Rec.ID, prev_tec_rec.GoodsID, new_tec_rec.GoodsID, 0, 1)) {
								r = PPErrorZ();
								setCtrlLong(CTLSEL_TSESS_TECH, tech_id = Data.Rec.TechID);
							}
						}
					}
				}
			}
			if(r && new_tec_rec.InitQtty > 0.0) {
				const double prev_qtty = getCtrlReal(CTL_TSESS_PLANQTTY);
				if(!force || prev_qtty == 0.0) {
					setCtrlReal(CTL_TSESS_PLANQTTY, new_tec_rec.InitQtty);
					SetupCapacity();
				}
			}
		}
		else
			Data.Rec.TechID = 0;
		SetupAutoFillButton(); // @v10.8.12
	}
	Data.Rec.TechID = tech_id;
}

void TSessionDialog::Detail()
{
	TSessionPacket temp_pack;
	if(getDTS(&temp_pack)) {
		PPID   temp_id = temp_pack.Rec.ID;
		if(TSesObj.PutPacket(&temp_id, &temp_pack, 1)) {
			SessUpdated = true;
			Data.Rec.ID = temp_id;
			TSessLineFilt filt(temp_id);
			ViewTSessLine(&filt);
			if(!TSesObj.Search(temp_id, &temp_pack.Rec)) {
				PPError();
				endModal(cmCancel);
			}
			else {
				setDTS(&temp_pack);
			}
		}
		else
			PPError();
	}
}

class TSessMoreDialog : public PPListDialog {
	DECL_DIALOG_DATA(TSessionPacket);
	enum {
		ctlgroupIbg = 1
	};
public:
	TSessMoreDialog() : PPListDialog(DLG_TSESSEXT, CTL_TSESSEXT_PLACELIST)
	{
		addGroup(ctlgroupIbg, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_TSESSEXT_IMAGE, cmAddImage, cmDelImage, 1));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		{
			ImageBrowseCtrlGroup::Rec rec;
			Data.LinkFiles.Init(PPOBJ_TSESSION);
			if(!Data.LinkFiles.GetCount() && Data.Rec.Flags & TSESF_HASIMAGES)
				Data.LinkFiles.Load(Data.Rec.ID, 0L);
			Data.LinkFiles.At(0, rec.Path);
			setGroupData(ctlgroupIbg, &rec);
		}
		SetInlineTags();
		//
		Data.Ext.GetExtStrData(PRCEXSTR_DETAILDESCR, temp_buf);
		SetupInputLine(CTL_TSESSEXT_DETAIL, MKSTYPE(S_ZSTRING, 4000), MKSFMT(4000, 0));
		setCtrlString(CTL_TSESSEXT_DETAIL, temp_buf);
		//
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   ctl_sel = 0;
		SString temp_buf;
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(getGroupData(ctlgroupIbg, &rec))
				if(rec.Path.Len()) {
					THROW(Data.LinkFiles.Replace(0, rec.Path));
				}
				else
					Data.LinkFiles.Remove(0);
			SETFLAG(Data.Rec.Flags, TSESF_HASIMAGES, Data.LinkFiles.GetCount());
		}
		GetInlineTags();
		getCtrlString(CTL_TSESSEXT_DETAIL, temp_buf);
		Data.Ext.PutExtStrData(PRCEXSTR_DETAILDESCR, temp_buf);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, ctl_sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			GetInlineTags();
			Data.TagL.Oid.Obj = PPOBJ_TSESSION;
			EditObjTagValList(&Data.TagL, 0);
			SetInlineTags();
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList()
	{
		int    ok = 1;
		SString temp_buf;
		StringSet ss(SLBColumnDelim);
		PPProcessorPacket::PlaceDescription item;
		for(uint i = 0; i < Data.Ext.GetPlaceDescriptionCount(); i++) {
			if(!Data.Ext.GetPlaceDescription(i, item)) {
				item.Z().Range = "#ERROR";
			}
			ss.Z();
			ss.add(item.Range);
			ss.add(GetGoodsName(item.GoodsID, temp_buf.Z()));
			ss.add(item.Descr);
			THROW(addStringToList(i+1, ss.getBuf()));
		}
		CATCHZOKPPERR
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		PPProcessorPacket::PlaceDescription new_item;
		if(PPObjProcessor::EditPrcPlaceItem(&new_item) > 0) {
			uint   new_pos = Data.Ext.GetPlaceDescriptionCount();
			if(!Data.Ext.PutPlaceDescription(new_pos, &new_item))
				ok = PPErrorZ();
			else {
				ASSIGN_PTR(pPos, new_pos);
				ASSIGN_PTR(pID, new_pos+1);
				ok = 1;
			}
		}
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		const uint c = Data.Ext.GetPlaceDescriptionCount();
		if(pos < static_cast<long>(c)) {
			PPProcessorPacket::PlaceDescription item;
			if(Data.Ext.GetPlaceDescription(pos, item)) {
				if(PPObjProcessor::EditPrcPlaceItem(&item) > 0) {
					if(!Data.Ext.PutPlaceDescription(pos, &item))
						ok = PPErrorZ();
					else
						ok = 1;
				}
			}
		}
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		const uint c = Data.Ext.GetPlaceDescriptionCount();
		if(pos < static_cast<long>(c)) {
			Data.Ext.PutPlaceDescription(pos, 0);
			ok = 1;
		}
		return ok;
	}
	int    SetInlineTags()
	{
		int    ok = 1;
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		SString temp_buf;
		if(getCtrlView(CTL_TSESSEXT_DESCR) && tag_obj.Fetch(PPTAG_TSESS_DESCR, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_STRING) {
			const ObjTagItem * p_tag = Data.TagL.GetItem(PPTAG_TSESS_DESCR);
			if(p_tag && p_tag->GetStr(temp_buf))
				setCtrlString(CTL_TSESSEXT_DESCR, temp_buf);
		}
		return ok;
	}
	int    GetInlineTags()
	{
		int    ok = 1;
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		SString temp_buf;
		if(getCtrlView(CTL_TSESSEXT_DESCR) && tag_obj.Fetch(PPTAG_TSESS_DESCR, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_STRING) {
			getCtrlString(CTL_TSESSEXT_DESCR, temp_buf);
			Data.TagL.PutItemStr(PPTAG_TSESS_DESCR, temp_buf);
		}
		return ok;
	}
};

void TSessionDialog::More()
{
    TSessionPacket pack;
    if(getDTS(&pack)) {
		if(PPDialogProcBody <TSessMoreDialog, TSessionPacket> (&pack) > 0) {
			setDTS(&pack);
		}
    }
}

void TSessionDialog::SetupSCard()
{
	int    ok = -1;
	SString scard_no;
	SCardTbl::Rec scard_rec;
	if(InputStringDialog(0, scard_no) > 0)
		if(scard_no.NotEmptyS())
			if(ScObj.SearchCode(0, scard_no, &scard_rec) > 0) {
				if(TSesObj.SetSCardID(&Data.Rec, &scard_rec) == 2) {
					setCtrlLong(CTLSEL_TSESS_OBJ,  Data.Rec.ArID);
					setCtrlLong(CTLSEL_TSESS_OBJ2, Data.Rec.Ar2ID);
				}
				setCtrlString(CTL_TSESS_SCARD, scard_no);
				ok = 1;
			}
			else {
				ok = PPErrorZ();
				scard_no = 0;
			}
		else {
			if(TSesObj.SetSCardID(&Data.Rec, 0) == 2) {
				setCtrlLong(CTLSEL_TSESS_OBJ,  Data.Rec.ArID);
				setCtrlLong(CTLSEL_TSESS_OBJ2, Data.Rec.Ar2ID);
			}
			setCtrlString(CTL_TSESS_SCARD, scard_no);
			ok = 1;
		}
	if(ok > 0) {
		TSessionPacket temp_pack;
		if(getDTS(&temp_pack) && temp_pack.Rec.ID) {
			int    r = TSesObj.SetupDiscount(temp_pack.Rec.ID, 1, fdiv100i(scard_rec.PDis), 1);
			if(r > 0) {
				TSessionTbl::Rec rec;
				if(TSesObj.Search(temp_pack.Rec.ID, &rec) > 0)
					setCtrlReal(CTL_TSESS_AMOUNT, Data.Rec.Amount = rec.Amount);
			}
			else if(!r)
				PPError();
		}
	}
}

static int PPCheckInPersonTurnProc_TSess(const PPCheckInPersonConfig * pCfg, PPCheckInPersonArray * pList, uint itemPos, void * pExtPtr)
{
	assert(pList != 0);
	int    ok = -1;
	TSessionPacket * p_pack = static_cast<TSessionPacket *>(pExtPtr);
	if(p_pack) {
		if(itemPos < pList->GetCount()) {
			//PPCheckInPersonItem & r_item = pList->at(itemPos);
			PPObjTSession tses_obj;
			if(pList != &p_pack->CiList) {
				p_pack->CiList = *pList;
			}
			PPID   tses_id = p_pack->Rec.ID;
			THROW(tses_obj.PutPacket(&tses_id, p_pack, 1));
			if(pList != &p_pack->CiList) {
				*pList = p_pack->CiList;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

void TSessionDialog::SetPlannedTiming(long sec)
{
	const  long   as_ = labs(sec);
	LTIME  tm = ZEROTIME;
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v10.0.07
	if((as_ / 3600) < 3) {
		tm.settotalsec(as_);
		r_temp_buf.Cat(tm, TIMF_HMS);
	}
	else {
		const long nd = as_ / SlConst::SecsPerDay;
		if(nd)
			r_temp_buf.Cat(nd).CatChar('d').Space();
		tm.settotalsec(as_ % SlConst::SecsPerDay);
		r_temp_buf.Cat(tm, TIMF_HMS);
	}
	setCtrlString(CTL_TSESS_PLANTIMING, r_temp_buf);
}

long TSessionDialog::GetPlannedTiming()
{
	long   sec = 0;
	long   nd = -1;
	SString line_buf, temp_buf;
	getCtrlString(CTL_TSESS_PLANTIMING, line_buf);
	line_buf.Transf(CTRANSF_INNER_TO_UTF8); // @v11.3.1 CTRANSF_INNER_TO_OUTER-->CTRANSF_INNER_TO_UTF8 (модуль переведен в кодировку UTF-8)
	SStrScan scan(line_buf);
	scan.Skip();
	if(scan.GetDigits(temp_buf)) {
		long   n1 = temp_buf.ToLong();
		scan.Skip();
		const char * p_days_word[] = { "days", "day", "d", "дней", "день", "д", "суток", "сутки", "сут" };
		for(uint i = 0; i < SIZEOFARRAY(p_days_word); i++) {
			const char * p = p_days_word[i];
			if(scan.Is(p)) {
				scan.Incr(sstrlen(p));
				nd = n1;
			}
		}
		scan.Skip();
	}
	LTIME tm;
	if(nd >= 0) {
		strtotime(scan, TIMF_HMS, &tm);
		sec = tm.totalsec() + (nd * SlConst::SecsPerDay);
	}
	else {
		strtotime(line_buf, TIMF_HMS, &tm);
		sec = tm.totalsec();
	}
	return sec;
}

int TSessionDialog::IsAddCompletionAvailable()
{
	int    yes = 0;
	long   h_lnenum = -1;
	if(Data.Rec.PrcID && Data.Rec.TechID) {
		const  double planned_qtty = getCtrlReal(CTL_TSESS_PLANQTTY);
		PPID   sess_id = Data.Rec.ID;
		if(!(Data.Rec.Flags & (TSESF_SUPERSESS|TSESF_WRITEDOFF)) && TSesObj.GetSessionKind(Data.Rec) == TSESK_SESSION && planned_qtty != 0.0) {
			uint   ln_count = 0;
			TSessLineTbl::Rec line_rec;
			for(!sess_id || TSesObj.P_Tbl->InitLineEnum(sess_id, &h_lnenum); TSesObj.P_Tbl->NextLineEnum(h_lnenum, &line_rec) > 0;) {
				ln_count++;
				break;
			}
			if(ln_count == 0) {
				ProcessorTbl::Rec prc_rec;
				TechTbl::Rec tec_rec;
				if(TSesObj.PrcObj.GetRecWithInheritance(Data.Rec.PrcID, &prc_rec, 1) > 0 && TSesObj.TecObj.Fetch(Data.Rec.TechID, &tec_rec) > 0) {
					if(tec_rec.GoodsID && tec_rec.GStrucID)
						yes = 1;
				}
			}
		}
	}
	TSesObj.P_Tbl->DestroyIter(h_lnenum);
	return yes;
}

int TSessionDialog::AddCompletion() // @v10.8.12
{
	int    ok = -1;
	PPID   sess_id = Data.Rec.ID;
	const  double planned_qtty = getCtrlReal(CTL_TSESS_PLANQTTY);
	if(!(Data.Rec.Flags & (TSESF_SUPERSESS|TSESF_WRITEDOFF)) && TSesObj.GetSessionKind(Data.Rec) == TSESK_SESSION && planned_qtty != 0.0) {
		uint   ln_count = 0;
		if(sess_id) {
			TSessLineTbl::Rec line_rec;
			long   h_lnenum = -1;
			for(TSesObj.P_Tbl->InitLineEnum(sess_id, &h_lnenum); TSesObj.P_Tbl->NextLineEnum(h_lnenum, &line_rec) > 0;) {
				ln_count++;
			}
			TSesObj.P_Tbl->DestroyIter(h_lnenum);
		}
		if(ln_count == 0) {
			ProcessorTbl::Rec prc_rec;
			TechTbl::Rec tec_rec;
			if(TSesObj.PrcObj.GetRecWithInheritance(Data.Rec.PrcID, &prc_rec) > 0 && TSesObj.TecObj.Fetch(Data.Rec.TechID, &tec_rec) > 0) {
				const  PPID tec_goods_id = tec_rec.GoodsID;
				const  PPID tec_struc_id = tec_rec.GStrucID;
				const int  main_item_sign = tec_rec.Sign;
				if(tec_goods_id && tec_struc_id) {
					TSessionPacket temp_pack;
					if(getDTS(&temp_pack)) {
						THROW(TSesObj.CheckPossibilityToInsertLine(temp_pack.Rec)); // @v10.9.0
						if(CONFIRM(PPCFM_TSESSAUTOCOMPLETE)) {
							TSessionTbl::Rec updated_tses_rec;
							PPTransaction tra(1);
							THROW(tra);
							sess_id = temp_pack.Rec.ID;
							THROW(TSesObj.PutPacket(&sess_id, &temp_pack, 0));
							{
								{
									TSessLineTbl::Rec line_rec;
									long   oprno = 0;
									THROW(TSesObj.InitLinePacket(&line_rec, sess_id) > 0);
									THROW(TSesObj.SetupLineGoods(&line_rec, tec_goods_id, 0, 0));
									line_rec.Sign = static_cast<int16>(main_item_sign);
									line_rec.Qtty = planned_qtty;
									THROW(TSesObj.PutLine(sess_id, &oprno, &line_rec, 0));
								}
								THROW(TSesObj.CompleteSession(sess_id, 0));
							}
							THROW(TSesObj.Search(sess_id, &updated_tses_rec) > 0);
							THROW(tra.Commit());
							assert(updated_tses_rec.ID == sess_id);
							Data.Rec.ID = sess_id;
							Data.Rec.ActQtty = updated_tses_rec.ActQtty;
							Data.Rec.Amount = updated_tses_rec.Amount;
							Data.Rec.FinDt = updated_tses_rec.FinDt;
							Data.Rec.FinTm = updated_tses_rec.FinTm;
							setCtrlReal(CTL_TSESS_ACTQTTY, Data.Rec.ActQtty);
							setCtrlReal(CTL_TSESS_AMOUNT, Data.Rec.Amount);
							setCtrlData(CTL_TSESS_FNDT, &Data.Rec.FinDt);
							setCtrlData(CTL_TSESS_FNTM, &Data.Rec.FinTm);
							SetupAutoFillButton();
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

IMPL_HANDLE_EVENT(TSessionDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(event.isCmd(cmTSessDetail))
			Detail();
		else if(event.isCmd(cmSelOrder)) {
			PPID   goods_id = 0;
			PrcTechCtrlGroup * p_grp = static_cast<PrcTechCtrlGroup *>(getGroup(ctlgroupPrcTech));
			if(p_grp && p_grp->getGoodsID(this, &goods_id) > 0) {
				PPObjBill::SelectLotParam slp(-labs(goods_id), 0, 0, PPObjBill::SelectLotParam::fFillLotRec);
				slp.RetLotID = Data.Rec.OrderLotID;
				if(P_BObj->SelectLot2(slp) > 0) {
					Data.Rec.OrderLotID = slp.RetLotID;
					setupOrder();
				}
			}
		}
		else if(event.isCmd(cmFreeList))
			selectFreeEntry();
		else if(event.isCmd(cmSelSCard))
			SetupSCard();
		else if(event.isCmd(cmBrowseTSessByAr)) {
			PPID   ar_id = getCtrlLong(CTLSEL_TSESS_OBJ);
			if(ar_id) {
				TSessionFilt ts_filt;
				ts_filt.ArID = ar_id;
				ViewTSession(&ts_filt);
			}
		}
		else if(event.isCmd(cmCCheck)) {
			if(Data.Rec.CCheckID_) {
				PPID   cn_id = 0;
				//CC.GetNodeID(id, &cn_id);
				CCheckPane(cn_id, Data.Rec.CCheckID_);
			}
			else {
				TSessionPacket temp_pack;
				InnerGetDTS = true;
				if(getDTS(&temp_pack)) {
					PPID   temp_id = temp_pack.Rec.ID;
					if(TSesObj.PutPacket(&temp_id, &temp_pack, 1)) {
						SessUpdated = true;
						Data.Rec.ID = temp_id;
						TSesObj.CallCheckPaneBySess(temp_id, 0);
						if(TSesObj.Search(temp_id, &temp_pack.Rec) > 0 && temp_pack.Rec.CCheckID_) {
							Data.Rec.CCheckID_ = temp_pack.Rec.CCheckID_;
							SetupCCheckButton();
						}
					}
					else
						PPError();
				}
				InnerGetDTS = false;
			}
		}
		else if(event.isCmd(cmChkInP)) {
			TSessionPacket temp_pack;
			PPCheckInPersonConfig cip_cfg(TSesObj, Data);
			if(cip_cfg.Flags & PPCheckInPersonConfig::fInitPrc) {
				if(cip_cfg.Flags & PPCheckInPersonConfig::fInitTech) {
					if(getDTS(&temp_pack)) {
						cip_cfg.TurnProc = PPCheckInPersonTurnProc_TSess;
						cip_cfg.P_TurnProcExt = &temp_pack;
					}
				}
				EditCheckInPersonList(&cip_cfg, &Data.CiList);
				InnerGetDTS = false;
			}
		}
		else if(event.isCmd(cmAddPaym)) {
			AddPayment();
		}
		else if(event.isCmd(cmBills)) {
			if(Data.Rec.ID) {
				ViewBillsByPool(PPASS_TSESSBILLPOOL, Data.Rec.ID);
			}
		}
		// @v11.0.4 {
		else if(event.isCmd(cmRepeating)) { 
			STATIC_ASSERT(sizeof(Data.Rec.Repeating) == sizeof(DateRepeating));
			DateRepeating dr = *reinterpret_cast<const DateRepeating *>(&Data.Rec.Repeating);
			RepeatingDialog * dlg = new RepeatingDialog(RepeatingDialog::fEditRepeatAfterItem);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->setDTS(&dr);
				while(ExecView(dlg) == cmOK)
					if(dlg->getDTS(&dr)) {
						*reinterpret_cast<DateRepeating *>(&Data.Rec.Repeating) = dr;
						break;
					}
			}
			delete dlg;
		}
		// } @v11.0.4 
		else if(event.isCmd(cmTags)) {
			Data.TagL.Oid.Obj = PPOBJ_TSESSION;
			EditObjTagValList(&Data.TagL, 0);
		}
		else if(event.isCmd(cmaMore)) {
			More();
		}
		// @v10.8.12 {
		else if(event.isCmd(cmAutoFill)) {
			AddCompletion();
		}
		// } @v10.8.12 
		else if(event.isClusterClk(CTL_TSESS_STATUS)) {
			long   new_status = 0;
			GetClusterData(CTL_TSESS_STATUS, &new_status);
			Data.Rec.Status = OrgStatus;
			TSessionTbl::Rec prev_rec = Data.Rec;
			if(TSesObj.SetSessionState(&Data.Rec, new_status, 0)) {
				if(prev_rec.StDt != Data.Rec.StDt)
					setCtrlData(CTL_TSESS_STDT, &Data.Rec.StDt);
				if(prev_rec.StTm != Data.Rec.StTm)
					setCtrlData(CTL_TSESS_STTM, &Data.Rec.StTm);
				if(prev_rec.FinDt != Data.Rec.FinDt)
					setCtrlData(CTL_TSESS_FNDT, &Data.Rec.FinDt);
				if(prev_rec.StTm != Data.Rec.FinTm)
					setCtrlData(CTL_TSESS_FNTM, &Data.Rec.FinTm);
				SetupCCheckButton();
			}
			else
				PPError();
		}
		else if(event.isClusterClk(CTL_TSESS_IDLE)) {
			GetClusterData(CTL_TSESS_IDLE, &Data.Rec.Flags);
			PrcTechCtrlGroup * p_grp = static_cast<PrcTechCtrlGroup *>(getGroup(ctlgroupPrcTech));
			CALLPTRMEMB(p_grp, setIdleStatus(this, LOGIC(Data.Rec.Flags & TSESF_IDLE)));
			SetupCCheckButton();
		}
		else if(event.isCbSelected(CTLSEL_TSESS_PRC)) {
			int    r = 1;
			const  PPID prev_prc_id = Data.Rec.PrcID;
			const  long prev_status = Data.Rec.Status;
			long   new_status = 0;
			GetClusterData(CTL_TSESS_STATUS, &new_status);
			PPID   prc_id = getCtrlLong(CTLSEL_TSESS_PRC);
			if(prc_id != Data.Rec.PrcID) {
				Data.Rec.PrcID = prc_id;
				if(prc_id) {
					Data.Rec.Status = 1;
					if(!TSesObj.SetSessionState(&Data.Rec, new_status, 1)) {
						PPError();
						Data.Rec.Status = static_cast<int16>(prev_status);
						Data.Rec.PrcID  = prev_prc_id;
						setCtrlData(CTLSEL_TSESS_PRC, &Data.Rec.PrcID);
						r = 0;
					}
					else if(Data.Rec.Flags & TSESF_SUPERSESS) {
						ProcessorTbl::Rec prc_rec;
						if(TSesObj.GetPrc(prc_id, &prc_rec, 1, 1) > 0 && prc_rec.SuperSessTiming) {
							Data.Rec.PlannedTiming = prc_rec.SuperSessTiming;
							SetPlannedTiming(Data.Rec.PlannedTiming);
							SetupTiming(0);
						}
					}
					else if(Data.Rec.ParentID) {
						if(TSesObj.InductSuperSess(&Data.Rec) > 0) {
							setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, Data.Rec.StDt, Data.Rec.StTm);
							setCtrlData(CTLSEL_TSESS_OBJ, &Data.Rec.ArID);
						}
					}
					for(uint i = 0; i < SIZEOFARRAY(_TSesStatusTab); i++) {
						int    item_pos = 0;
						const  int status = _TSesStatusTab[i];
						if(GetClusterItemByAssoc(CTL_TSESS_STATUS, status, &item_pos)) {
							DisableClusterItem(CTL_TSESS_STATUS, item_pos, !TSesObj.SetSessionState(&Data.Rec, status, 1));
						}
					}
				}
				if(r) {
					getCtrlData(CTLSEL_TSESS_SUPERSES, &Data.Rec.ParentID);
					void * extra_ptr = PPObjTSession::MakeExtraParam(0, prc_id, 1);
					SetupPPObjCombo(this, CTLSEL_TSESS_SUPERSES, PPOBJ_TSESSION, Data.Rec.ParentID, OLW_LOADDEFONOPEN, extra_ptr);
					SetupCCheckButton();
					SetupCipAndRepButton();
				}
				SetupPayment();
			}
			else
				return;
		}
		else if(event.isCbSelected(CTLSEL_TSESS_SUPERSES)) {
			PPID   super_sess_id = getCtrlLong(CTLSEL_TSESS_SUPERSES);
			if(TSesObj.CheckSuperSessLink(&Data.Rec, super_sess_id, 0)) {
				Data.Rec.ParentID = super_sess_id;
				if(TSesObj.InductSuperSess(&Data.Rec) > 0) {
					setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, Data.Rec.StDt, Data.Rec.StTm);
					setCtrlLong(CTLSEL_TSESS_OBJ, Data.Rec.ArID);
				}
			}
			else {
				PPError();
				setCtrlLong(CTLSEL_TSESS_SUPERSES, Data.Rec.ParentID);
			}
		}
		else if(event.isCbSelected(CTLSEL_TSESS_TECH))
			SetupTech(0);
		else if(event.isCbSelected(CTLSEL_TSESS_OBJ)) {
			Data.Rec.ArID = getCtrlLong(CTLSEL_TSESS_OBJ);
			enableCommand(cmBrowseTSessByAr, Data.Rec.ArID);
			SetupPayment();
		}
		else if(event.isCbSelected(CTLSEL_TSESS_OBJ2)) {
			Data.Rec.Ar2ID = getCtrlLong(CTLSEL_TSESS_OBJ2);
		}
		else if(TVCMD == cmInputUpdated) {
			uint i = TVINFOVIEW->GetId();
			if(oneof3(i, CTL_TSESS_PLANTIMING, CTL_TSESS_STDT, CTL_TSESS_STTM)) {
				if(!InpUpdLock) {
					TInputLine * il = static_cast<TInputLine *>(getCtrlView(i));
					CALLPTRMEMB(il, disableDeleteSelection(1));
					SetupTiming(0);
					CALLPTRMEMB(il, disableDeleteSelection(0));
				}
			}
			if(oneof2(i, CTL_TSESS_FNDT, CTL_TSESS_FNTM)) {
				if(!InpUpdLock) {
					TInputLine * il = static_cast<TInputLine *>(getCtrlView(i));
					CALLPTRMEMB(il, disableDeleteSelection(1));
					SetupTiming(1);
					CALLPTRMEMB(il, disableDeleteSelection(0));
				}
			}
			else if(i == CTL_TSESS_PLANQTTY) {
				if(!InpUpdLock) {
					TInputLine * il = static_cast<TInputLine *>(getCtrlView(i));
					CALLPTRMEMB(il, disableDeleteSelection(1));
					SetupCapacity();
					SetupAutoFillButton(); // @v10.8.12
					CALLPTRMEMB(il, disableDeleteSelection(0));
				}
			}
		}
		else
			return;
	}
	else if(TVBROADCAST) {
		if(TVCMD == cmReleasedFocus || TVCMD == cmCommitInput) {
			uint i = TVINFOVIEW->GetId();
			if(oneof3(i, CTL_TSESS_PLANTIMING, CTL_TSESS_STDT, CTL_TSESS_STTM)) {
				if(!InpUpdLock)
					SetupTiming(0);
			}
			else if(i == CTL_TSESS_PLANQTTY) {
				if(!InpUpdLock)
					SetupCapacity();
			}
			else
				return;
		}
		else
			return;
	}
	else if(TVKEYDOWN) {
		if(TVKEY == kbF2 && P_Current) {
			LDATETIME dtm = getcurdatetime_();
			if(isCurrCtlID(CTL_TSESS_STDT)) {
				setCtrlData(CTL_TSESS_STDT, &dtm.d);
				SetupTiming(0);
			}
			else if(isCurrCtlID(CTL_TSESS_STTM)) {
				setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, dtm);
				SetupTiming(0);
			}
			else if(isCurrCtlID(CTL_TSESS_FNDT))
				setCtrlData(CTL_TSESS_FNDT, &dtm.d);
			else if(isCurrCtlID(CTL_TSESS_FNTM))
				setCtrlDatetime(CTL_TSESS_FNDT, CTL_TSESS_FNTM, dtm);
		}
		else if(TVKEY == kbF11)
			Detail();
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void TSessionDialog::SetupCapacity()
{
	const double planned_qtty = getCtrlReal(CTL_TSESS_PLANQTTY);
	if(planned_qtty != Data.Rec.PlannedQtty || Data.Rec.PlannedTiming <= 0) {
		InpUpdLock++;
		Data.Rec.PlannedQtty = planned_qtty;
		getCtrlData(CTLSEL_TSESS_TECH, &Data.Rec.TechID); // getCtrlLong использовать нельзя, ибо поле может отсутствовать
		TSesObj.SetPlannedTiming(&Data.Rec);
		SetPlannedTiming(Data.Rec.PlannedTiming);
		SetupTiming(0);
		InpUpdLock--;
	}
}

int TSessionDialog::SetupTiming(int master)
{
	int    ok = 1;
	uint   sel = 0;
	LTIME  tm = ZEROTIME;
	LDATETIME start, finish;
	long   timing = 0;
	long   force_timing = -1;
	getCtrlData(sel = CTL_TSESS_STDT, &Data.Rec.StDt);
	THROW_SL(checkdate(Data.Rec.StDt, 1));
	getCtrlData(CTL_TSESS_STTM, &Data.Rec.StTm);
	start.Set(Data.Rec.StDt, Data.Rec.StTm);
	if(master == 0) {
		Data.Rec.PlannedTiming = GetPlannedTiming();
		force_timing = timing = Data.Rec.PlannedTiming /*+ getToolingTiming()*/;
	}
	else {
		getCtrlDatetime(CTL_TSESS_FNDT, CTL_TSESS_FNTM, finish);
		timing = diffdatetimesec(finish, start);
		if(timing >= 0) {
			Data.Rec.FinDt = finish.d;
			Data.Rec.FinTm = finish.t;
		}
	}
	if((oneof3(Data.Rec.Status, TSESST_PLANNED, TSESST_PENDING, TSESST_INPROCESS) ||
		(Data.Rec.Status == TSESST_CLOSED && !Data.Rec.ID)) && timing >= 0) { // @v8.2.9 || (Data.Rec.Status == TSESST_CLOSED && !Data.Rec.ID)
		InpUpdLock++;
		if(master == 0) {
			finish = plusdatetime(start, timing, 3);
			if(!start.d)
				finish.d = ZERODATE;
			Data.Rec.FinDt = finish.d;
			Data.Rec.FinTm = finish.t;
			setCtrlDatetime(CTL_TSESS_FNDT, CTL_TSESS_FNTM, finish);
		}
		else {
			Data.Rec.PlannedTiming = timing;
			SetPlannedTiming(Data.Rec.PlannedTiming);
		}
		{
			double qtty = 0.0;
			getCtrlData(CTLSEL_TSESS_TECH, &Data.Rec.TechID); // getCtrlLong использовать нельзя, ибо поле может отсутствовать
			if(TSesObj.CalcPlannedQtty(&Data.Rec, force_timing, &qtty) > 0) {
				setCtrlReal(CTL_TSESS_PLANQTTY, R6(qtty));
			}
		}
		InpUpdLock--;
	}
	CATCH
		selectCtrl(sel);
		ok = 0;
	ENDCATCH
	return ok;
}

void TSessionDialog::SetupAutoFillButton()
{
	enableCommand(cmAutoFill, IsAddCompletionAvailable());
}

void TSessionDialog::SetupCCheckButton()
{
	bool   allow = false;
	ProcessorTbl::Rec prc_rec;
	if(Data.Rec.CCheckID_)
		allow = true;
	else if(Data.Rec.Status == TSESST_CLOSED && !(Data.Rec.Flags & TSESF_IDLE) && TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 1, 1) > 0)
		allow = (PPObjCashNode::Select(prc_rec.LocID, 1, 0, 1) > 0);
	enableCommand(cmCCheck, allow);
	showButton(cmCCheck, allow);
}

void TSessionDialog::SetupCipAndRepButton()
{
	bool   allow_cip = false;
	bool   allow_rep = false;
	ProcessorTbl::Rec prc_rec;
	if(TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 1, 1) > 0) {
		allow_cip = LOGIC(prc_rec.Flags & PRCF_ALLOWCIP);
		allow_rep = LOGIC(prc_rec.Flags & PRCF_ALLOWREPEATING);
	}
	enableCommand(cmChkInP, allow_cip);
	enableCommand(cmRepeating, allow_rep);
	showButton(cmChkInP, allow_cip);
	showButton(cmRepeating, allow_rep);
}

int TSessionDialog::setDTS(const TSessionPacket * pData)
{
	RVALUEPTR(Data, pData);
	OrgStatus = Data.Rec.Status;
	int    ok = 1;
	SString temp_buf;
	InpUpdLock++;
	setCtrlLong(CTL_TSESS_NUMBER, Data.Rec.Num);
	setCtrlLong(CTL_TSESS_ID, Data.Rec.ID);
	PrcTechCtrlGroup::Rec ptcg_rec;
	ptcg_rec.PrcID = Data.Rec.PrcID;
	if(Data.Rec.Flags & TSESF_PLAN) {
		if(Data.Rec.PrcID == 0)
			ptcg_rec.PrcParentID = PRCEXDF_GROUP;
		DateRange period;
		period.Set(Data.Rec.StDt, Data.Rec.FinDt);
		SetPeriodInput(this, CTL_TSESS_PLANPERIOD, period);
	}
	else {
		ptcg_rec.TechID = Data.Rec.TechID;
		ptcg_rec.ArID   = Data.Rec.ArID;
		ptcg_rec.Ar2ID  = Data.Rec.Ar2ID;
		ptcg_rec.IdleStatus = LOGIC(Data.Rec.Flags & TSESF_IDLE);
	}
	{
		PrcTechCtrlGroup * p_grp = static_cast<PrcTechCtrlGroup *>(getGroup(ctlgroupPrcTech));
		if(p_grp) {
			if(!(Data.Rec.Flags & (TSESF_SUPERSESS|TSESF_PLAN)))
				p_grp->enablePrcSelUpLevel(0);
			p_grp->enablePrcInsert(1);
		}
	}
	if(setGroupData(ctlgroupPrcTech, &ptcg_rec)) {
		//
		// Предварительно устанавливаем дату и время начала в диалоге из-за того,
		// что вызов SetupTech (косвенно через события) приводит к тому,
		// что из этих полей извлекается время.
		//
		setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, Data.Rec.StDt, Data.Rec.StTm);
		//
		// Группа выбора процессора может изменить TechID, установив
		// единственную доступную для данного процессора технологию.
		//
		SetupTech(Data.Rec.ID ? 0 : 1);
	}
	else
		PPError();
	void * extra_ptr = PPObjTSession::MakeExtraParam(0, Data.Rec.PrcID, TSESK_SUPERSESS);
	SetupPPObjCombo(this, CTLSEL_TSESS_SUPERSES, PPOBJ_TSESSION, Data.Rec.ParentID, OLW_LOADDEFONOPEN, extra_ptr);
	AddClusterAssoc(CTL_TSESS_IDLE, 0, TSESF_IDLE);
	SetClusterData(CTL_TSESS_IDLE, Data.Rec.Flags);
	AddClusterAssoc(CTL_TSESS_PLANFLAGS, 0, TSESF_PLAN_PHUNIT);
	SetClusterData(CTL_TSESS_PLANFLAGS, Data.Rec.Flags);
	{
		int    first_status = 0;
		uint   pos = 0;
		if(!oneof2(resourceID, DLG_TSESSSIMPLE, DLG_TSESSTM)) {
			pos = 1;
			AddClusterAssoc(CTL_TSESS_STATUS,  0, TSESST_PLANNED);
		}
		else
			first_status = 1;
		AddClusterAssocDef(CTL_TSESS_STATUS,  pos++, TSESST_PENDING);
		AddClusterAssoc(CTL_TSESS_STATUS,  pos++, TSESST_INPROCESS);
		AddClusterAssoc(CTL_TSESS_STATUS,  pos++, TSESST_CLOSED);
		AddClusterAssoc(CTL_TSESS_STATUS,  pos++, TSESST_CANCELED);
		SetClusterData(CTL_TSESS_STATUS, Data.Rec.Status);
		for(uint i = 0; i < SIZEOFARRAY(_TSesStatusTab); i++) {
			int    item_pos = 0;
			const  int status = _TSesStatusTab[i];
			if(GetClusterItemByAssoc(CTL_TSESS_STATUS, status, &item_pos))
				DisableClusterItem(CTL_TSESS_STATUS, item_pos, !TSesObj.SetSessionState(&Data.Rec, status, 1));
		}
	}
	setCtrlDatetime(CTL_TSESS_STDT, CTL_TSESS_STTM, Data.Rec.StDt, Data.Rec.StTm);
	setCtrlDatetime(CTL_TSESS_FNDT, CTL_TSESS_FNTM, Data.Rec.FinDt, Data.Rec.FinTm);
	setCtrlReal(CTL_TSESS_PLANQTTY,   Data.Rec.PlannedQtty);
	setCtrlReal(CTL_TSESS_AMOUNT,     Data.Rec.Amount);

	SetPlannedTiming(Data.Rec.PlannedTiming);
	setCtrlData(CTL_TSESS_ACTQTTY,    &Data.Rec.ActQtty);
	setCtrlData(CTL_TSESS_INCOMPL,    &Data.Rec.Incomplete);
	disableCtrls(1, CTL_TSESS_ID, CTL_TSESS_ACTQTTY, CTL_TSESS_ACTTIMING, CTL_TSESS_INCOMPL, 0);
	{
		SCardTbl::Rec scard_rec;
		SString scard_no;
		if(ScObj.Search(Data.Rec.SCardID, &scard_rec) > 0)
			scard_no = scard_rec.Code;
		setCtrlString(CTL_TSESS_SCARD, scard_no);
	}
	setupOrder();
	// @v11.0.4 setCtrlData(CTL_TSESS_MEMO, Data.Rec.Memo);
	// @v11.0.4 {
	Data.Ext.GetExtStrData(PRCEXSTR_MEMO, temp_buf.Z());
	setCtrlString(CTL_TSESS_MEMO, temp_buf); 
	// } @v11.0.4 
	disableCtrl(CTL_TSESS_IDLE, Data.Rec.ID);
	disableCtrl(CTLSEL_TSESS_TECH, Data.Rec.Flags & (TSESF_IDLE | TSESF_PLAN));
	enableCommand(cmSelTechByGoods, !(Data.Rec.Flags & (TSESF_IDLE | TSESF_PLAN)));
	enableCommand(cmBrowseTSessByAr, Data.Rec.ArID);
	SetupCCheckButton();
	SetupCipAndRepButton();
	SetupAutoFillButton(); // @v10.8.12
	SetupPayment();
	InpUpdLock--;
	return ok;
}

int TSessionDialog::getDTS(TSessionPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	SString temp_buf;
	getCtrlData(CTL_TSESS_NUMBER, &Data.Rec.Num);
	GetClusterData(CTL_TSESS_IDLE,      &Data.Rec.Flags);
	GetClusterData(CTL_TSESS_PLANFLAGS, &Data.Rec.Flags);
	GetClusterData(CTL_TSESS_STATUS, &Data.Rec.Status);
	if(Data.Rec.Flags & TSESF_PLAN) {
		DateRange period;
		THROW(GetPeriodInput(this, sel = CTL_TSESS_PLANPERIOD, &period));
		Data.Rec.StDt = period.low;
		Data.Rec.StTm = ZEROTIME;
		Data.Rec.FinDt = period.upp;
		Data.Rec.FinTm = MAXDAYTIMESEC;
	}
	else {
		PrcTechCtrlGroup::Rec ptcg_rec;
		if(getGroupData(ctlgroupPrcTech, &ptcg_rec)) {
			Data.Rec.PrcID = ptcg_rec.PrcID;
			sel = CTLSEL_TSESS_PRC;
			THROW_PP(Data.Rec.PrcID, PPERR_PRCNEEDED);
			Data.Rec.TechID = ptcg_rec.TechID;
			if(!(Data.Rec.Flags & TSESF_SUPERSESS) && !(Data.Rec.Flags & TSESF_IDLE)) {
				sel = CTLSEL_TSESS_TECH;
				THROW_PP(Data.Rec.TechID, PPERR_TECHNEEDED);
			}
			Data.Rec.ArID   = ptcg_rec.ArID;
			Data.Rec.Ar2ID  = ptcg_rec.Ar2ID;
		}
		if(!InnerGetDTS && Data.Rec.Status == TSESST_CLOSED) {
			//
			// Проверка на необходимость ввода чека по закрытой тех сессии
			//
			ProcessorTbl::Rec prc_rec;
			if(TSesObj.GetPrc(Data.Rec.PrcID, &prc_rec, 0, 1) > 0) {
				THROW(Data.Rec.CCheckID_ || !(prc_rec.Flags & PRCF_NEEDCCHECK) || TSesObj.CheckRights(TSESRT_CLOSEWOCC));
			}
		}
	}
	// @v11.0.4 getCtrlData(CTL_TSESS_MEMO, Data.Rec.Memo);
	// @v11.0.4 {
	{
		getCtrlString(CTL_TSESS_MEMO, temp_buf.Z()); 
		Data.Ext.PutExtStrData(PRCEXSTR_MEMO, temp_buf);
	}
	// } @v11.0.4
	getCtrlData(sel = CTL_TSESS_FNDT, &Data.Rec.FinDt);
	THROW_SL(checkdate(Data.Rec.FinDt, 1));
	getCtrlData(CTL_TSESS_FNTM, &Data.Rec.FinTm);
	getCtrlData(CTL_TSESS_PLANQTTY,   &Data.Rec.PlannedQtty);
	sel = 0;
	THROW(SetupTiming(0));
	THROW(TSesObj.CheckSessionTime(Data.Rec));
	PPID   parent_id = getCtrlLong(sel = CTLSEL_TSESS_SUPERSES);
	THROW(TSesObj.CheckSuperSessLink(&Data.Rec, parent_id, 0));
	if(PPObjTSession::IsIdleInsignificant(&Data.Rec, OrgStatus) && CONFIRM(PPCFM_TSESSIDLEINSIGNIF)) {
		sel = CTL_TSESS_FNTM;
		CALLEXCEPT();
	}
	{
		SCardTbl::Rec scard_rec;
		SString scard_no;
		getCtrlString(sel = CTL_TSESS_SCARD, scard_no);
		if(scard_no.NotEmptyS()) {
			THROW_PP_S(ScObj.SearchCode(0, scard_no, &scard_rec) > 0, PPERR_SCARDNOTFOUND, scard_no);
			Data.Rec.SCardID = scard_rec.ID;
		}
		else
			Data.Rec.SCardID = 0;
	}
	if(ok)
		ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}
//
//
//
class TSessLineDialog : public TDialog {
	DECL_DIALOG_DATA(TSessLineTbl::Rec);
	enum {
		ctlgroupGoods = 1
	};
public:
	explicit TSessLineDialog(uint dlgId) : TDialog(dlgId/*DLG_TSESSLN*/), PctDis(0.0)
	{
		SetupCalDate(CTLCAL_TSESSLN_DT, CTL_TSESSLN_DT);
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_TSESSLN_GGRP, CTLSEL_TSESSLN_GOODS));
		showCtrl(CTL_TSESSLN_ORGQTTY, false); // @v11.0.7
		if(!(TSesObj.GetConfig().Flags & PPTSessConfig::fUsePricing)) {
			showCtrl(CTL_TSESSLN_PRICE, false);
			showCtrl(CTL_TSESSLN_DSCNT, false);
			showCtrl(CTL_TSESSLN_NETPRICE, false);
		}
		else if(!TSesObj.CheckRights(TSESRT_MODPRICE))
			disableCtrls(1, CTL_TSESSLN_PRICE, CTL_TSESSLN_DSCNT, 0);
		PPSetupCtrlMenu(this, CTL_TSESSLN_QTTY, CTLMNU_TSESSLN_QTTY, CTRLMENU_TSESSLINEQTTY);
		PPSetupCtrlMenu(this, CTL_TSESSLN_INDEPPHQTTY, CTLMNU_TSESSLN_PHQTTY, CTRLMENU_TSESSLINEPHQTTY);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		TSessLocID = 0;
		int    ok = 1;
		TSessionTbl::Rec tses_rec;
		ProcessorTbl::Rec prc_rec;
		SString sess_title;
		THROW(TSesObj.Search(Data.TSessID, &tses_rec) > 0);
		THROW(Data.OprNo || TSesObj.CheckPossibilityToInsertLine(tses_rec));
		sess_title.Cat(tses_rec.Num);
		{
			SCardTbl::Rec sc_rec;
			if(SearchObject(PPOBJ_SCARD, tses_rec.SCardID, &sc_rec) > 0)
				PctDis = fdiv100i(sc_rec.PDis);
		}
		if(TSesObj.PrcObj.Search(tses_rec.PrcID, &prc_rec) > 0) {
			sess_title.Space().CatChar('-').Space().Cat(prc_rec.Name);
			TSessLocID = prc_rec.LocID;
		}
		setStaticText(CTL_TSESSLN_SESSTITLE, sess_title);
		{
			GoodsCtrlGroup::Rec gcg_rec(0, Data.GoodsID, prc_rec.LocID,
				GoodsCtrlGroup::disableEmptyGoods|GoodsCtrlGroup::activateGoodsListOnGroupSelection|GoodsCtrlGroup::enableInsertGoods);
			setGroupData(ctlgroupGoods, &gcg_rec);
		}
		setCtrlLong(CTL_TSESSLN_SESSID, Data.TSessID);
		setCtrlLong(CTL_TSESSLN_OPRNO,  Data.OprNo);
		AddClusterAssocDef(CTL_TSESSLN_SIGN, 0, -1);
		AddClusterAssoc(CTL_TSESSLN_SIGN, 1, +1);
		AddClusterAssoc(CTL_TSESSLN_SIGN, 2,  0);
		SetClusterData(CTL_TSESSLN_SIGN, Data.Sign);
		AddClusterAssoc(CTL_TSESSLN_FLAG_REST, 0, TSESLF_REST);
		SetClusterData(CTL_TSESSLN_FLAG_REST, Data.Flags);
		AddClusterAssoc(CTL_TSESSLN_PLANFLAGS, 0, TSESLF_PLAN_PHUNIT);
		SetClusterData(CTL_TSESSLN_PLANFLAGS, Data.Flags);
		setCtrlData(CTL_TSESSLN_DT, &Data.Dt);
		setCtrlData(CTL_TSESSLN_TM, &Data.Tm);
		setCtrlReal(CTL_TSESSLN_QTTY,        Data.Qtty);
		setCtrlReal(CTL_TSESSLN_INDEPPHQTTY, Data.WtQtty);
		SetupPricing(0);
		setCtrlData(CTL_TSESSLN_SERIAL, Data.Serial);
		disableCtrl(CTL_TSESSLN_SERIAL, true);
		disableCtrl(CTL_TSESSLN_SIGN, Data.OprNo != 0);
		disableCtrl(CTL_TSESSLN_FLAG_REST, Data.OprNo != 0);
		if(Data.GoodsID)
			selectCtrl(CTL_TSESSLN_QTTY);
		disableCtrl(CTL_TSESSLN_SERIAL, BIN(Data.Sign < 0));
		// @v11.0.4 {
		{
			uint lda = GetLotDimAllowence(0);
			if(lda & 0x01)
				setCtrlReal(CTL_TSESSLN_LOTDIMX, Data.LotDimX);
			if(lda & 0x02)
				setCtrlReal(CTL_TSESSLN_LOTDIMY, Data.LotDimY);
			if(lda & 0x04)
				setCtrlReal(CTL_TSESSLN_LOTDIMZ, Data.LotDimZ);
		}
		// } @v11.0.4 
		enableCommand(cmSelSerial, BIN(Data.Sign < 0));
		SetupCtrlsOnGoodsSelection();
		CATCHZOKPPERR
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GoodsCtrlGroup::Rec gcg_rec;
		THROW(getGroupData(ctlgroupGoods, &gcg_rec));
		Data.GoodsID = gcg_rec.GoodsID;
		Data.Sign = static_cast<int16>(GetClusterData(CTL_TSESSLN_SIGN));
		GetClusterData(CTL_TSESSLN_FLAG_REST, &Data.Flags);
		GetClusterData(CTL_TSESSLN_PLANFLAGS, &Data.Flags);
		getCtrlData(sel = CTL_TSESSLN_QTTY, &Data.Qtty);
		THROW_PP(Data.Qtty > 0, PPERR_QTTYMUSTBEGTZ);
		if(Data.Flags & TSESLF_INDEPPHQTTY)
			getCtrlData(sel = CTL_TSESSLN_INDEPPHQTTY, &Data.WtQtty); // float WtQtty
		else
			Data.WtQtty = 0.0;
		Data.Price    = getCtrlReal(CTL_TSESSLN_PRICE);
		Data.Discount = getCtrlReal(CTL_TSESSLN_DSCNT);
		getCtrlData(sel = CTL_TSESSLN_DT, &Data.Dt);
		THROW_SL(checkdate(Data.Dt));
		getCtrlData(CTL_TSESSLN_TM, &Data.Tm);
		getCtrlData(CTL_TSESSLN_SERIAL, Data.Serial);
		// @v11.0.4 {
		{
			uint lda = GetLotDimAllowence(0);
			if(lda & 0x01)
				getCtrlData(CTL_TSESSLN_LOTDIMX, &Data.LotDimX);
			else
				Data.LotDimX = 0.0;
			if(lda & 0x02)
				getCtrlData(CTL_TSESSLN_LOTDIMY, &Data.LotDimY);
			else
				Data.LotDimY = 0.0;
			if(lda & 0x04)
				getCtrlData(CTL_TSESSLN_LOTDIMZ, &Data.LotDimZ);
			else
				Data.LotDimZ = 0.0;
		}
		// } @v11.0.4 
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrlsOnGoodsSelection();
	void   SetupQtty(int readFromField, double qtty);
	void   SetupPricing(int recalcDiscount);
	void   SetupLotDimention();
	uint   GetLotDimAllowence(SString * pQttyFormula);

	PPObjTSession TSesObj;
	PPObjGoods GObj;
	PPID   TSessLocID; // Склад из технологической сессии
	double PctDis; // Процентная скидка (извлекается из дисконтой карты, привязанной к сессии)
};

void TSessLineDialog::SetupCtrlsOnGoodsSelection()
{
	Goods2Tbl::Rec goods_rec;
	PPUnit unit_rec;
	setStaticText(CTL_TSESSLN_ST_PHQTTY, 0);
	if(Data.GoodsID && GObj.Fetch(Data.GoodsID, &goods_rec) > 0 && GObj.FetchUnit(goods_rec.PhUnitID, &unit_rec) > 0)
		setStaticText(CTL_TSESSLN_ST_PHQTTY, unit_rec.Name);
	showCtrl(CTL_TSESSLN_PHQTTY, !(Data.Flags & TSESLF_INDEPPHQTTY));
	showCtrl(CTL_TSESSLN_INDEPPHQTTY, LOGIC(Data.Flags & TSESLF_INDEPPHQTTY));
	showCtrl(CTLMNU_TSESSLN_PHQTTY, LOGIC(Data.Flags & TSESLF_INDEPPHQTTY));
	disableCtrls(Data.Flags & TSESLF_RECOMPL, CTL_TSESSLN_QTTY, CTL_TSESSLN_SIGN, CTL_TSESSLN_ORGQTTY, 0); // @v11.0.7 CTL_TSESSLN_QTTY
	SetupLotDimention(); // @v11.0.4
}

uint TSessLineDialog::GetLotDimAllowence(SString * pQttyFormula)
{
	uint   result = 0; // bit-mask X: 0x01, Y: 0x02, Z: 0x04
	CALLPTRMEMB(pQttyFormula, Z());
	if(Data.Sign > 0 && Data.GoodsID) {
		Goods2Tbl::Rec goods_rec;
		PPGdsClsPacket gcls_pack;
		if(GObj.FetchCls(Data.GoodsID, &goods_rec, &gcls_pack) > 0) {
			switch(gcls_pack.Rec.LotDimCount) {
				case 3: result = 0x01 | 0x02 | 0x04; break;
				case 2: result = 0x01 | 0x02; break;
				case 1: result = 0x01; break;
			}
			if(result) {
				ASSIGN_PTR(pQttyFormula, gcls_pack.LotDimQtty_Formula);
			}
		}
	}
	return result;
}

void TSessLineDialog::SetupLotDimention()
{
	SString formula;
	uint   dims_allowed = GetLotDimAllowence(&formula); // bit-mask X: 0x01, Y: 0x02, Z: 0x04
	showCtrl(CTL_TSESSLN_LOTDIMX, LOGIC(dims_allowed & 0x01));
	showCtrl(CTL_TSESSLN_LOTDIMY, LOGIC(dims_allowed & 0x02));
	showCtrl(CTL_TSESSLN_LOTDIMZ, LOGIC(dims_allowed & 0x04));
	if(formula.NotEmptyS()) {
		if(dims_allowed & 0x01)
			selectCtrl(CTL_TSESSLN_LOTDIMX);
		else if(dims_allowed & 0x02)
			selectCtrl(CTL_TSESSLN_LOTDIMY);
		else if(dims_allowed & 0x04)
			selectCtrl(CTL_TSESSLN_LOTDIMZ);
	}
}

void TSessLineDialog::SetupQtty(int readFromField, double qtty)
{
	SString phq_txt;
	double phuperu;
	GetClusterData(CTL_TSESSLN_PLANFLAGS, &Data.Flags);
	if(readFromField)
		Data.Qtty = getCtrlReal(CTL_TSESSLN_QTTY);
	else
		setCtrlReal(CTL_TSESSLN_QTTY, Data.Qtty = qtty);
	if(Data.Flags & TSESLF_INDEPPHQTTY)
		setCtrlReal(CTL_TSESSLN_INDEPPHQTTY, R6(Data.WtQtty));
	else if(Data.Flags & TSESLF_PLAN_PHUNIT)
		phq_txt.Cat(R6(Data.Qtty), MKSFMTD(0, 6, NMBF_NOZERO|NMBF_NOTRAILZ));
	else if(GObj.GetPhUPerU(Data.GoodsID, 0, &phuperu) > 0)
		phq_txt.Cat(R6(Data.Qtty * phuperu), MKSFMTD(0, 6, NMBF_NOZERO|NMBF_NOTRAILZ));
	setStaticText(CTL_TSESSLN_PHQTTY, phq_txt);
}

IMPL_HANDLE_EVENT(TSessLineDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(event.isCmd(cmSelSerial)) {
			const  PPID   goods_id = getCtrlLong(CTLSEL_TSESSLN_GOODS);
			if(goods_id) {
				long   temp_long = 0;
				if(GetClusterData(CTL_TSESSLN_SIGN, &temp_long))
					Data.Sign = static_cast<int16>(temp_long);
				if(Data.Sign < 0) {
					PPObjTSession::SerialByGoodsListItem si;
					if(TSesObj.SelectSerialByGoods(goods_id, TSessLocID, &si) > 0 && si.Serial[0])
						setCtrlData(CTL_TSESSLN_SERIAL, si.Serial);
				}
			}
		}
		else if(event.isCbSelected(CTLSEL_TSESSLN_GOODS)) {
			const  PPID goods_id = getCtrlLong(CTLSEL_TSESSLN_GOODS);
			const  int  r = TSesObj.SetupLineGoods(&Data, goods_id, 0, 0);
			if(!r) {
				setCtrlLong(CTLSEL_TSESSLN_GOODS, Data.GoodsID);
				PPError();
			}
			else {
				Data.GoodsID = goods_id;
				if(r > 0) {
					SetClusterData(CTL_TSESSLN_SIGN, Data.Sign);
					SetupPricing(1);
				}
				SetupCtrlsOnGoodsSelection();
			}
		}
		else if(event.isClusterClk(CTL_TSESSLN_PLANFLAGS))
			SetupQtty(1, 0);
		else if(event.isClusterClk(CTL_TSESSLN_SIGN)) { // @v11.0.4 @fix CTL_TSESSLN_PLANFLAGS-->CTL_TSESSLN_SIGN
			long   temp_long = 0;
			if(GetClusterData(CTL_TSESSLN_SIGN, &temp_long)) {
				Data.Sign = static_cast<int16>(temp_long);
				disableCtrl(CTL_TSESSLN_SERIAL, BIN(Data.Sign < 0));
				enableCommand(cmSelSerial, BIN(Data.Sign < 0));
				SetupLotDimention(); // @v11.0.4
			}
		}
		else if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_TSESSLN_PRICE)) {
				Data.Price = getCtrlReal(CTL_TSESSLN_PRICE);
				if(PctDis) {
					Data.Discount = Data.Price * fdiv100r(PctDis);
					setCtrlReal(CTL_TSESSLN_DSCNT, Data.Discount);
				}
				setCtrlReal(CTL_TSESSLN_NETPRICE, Data.Price - Data.Discount);
			}
			else if(event.isCtlEvent(CTL_TSESSLN_DSCNT)) {
				Data.Price = getCtrlReal(CTL_TSESSLN_PRICE);
				Data.Discount = getCtrlReal(CTL_TSESSLN_DSCNT);
				setCtrlReal(CTL_TSESSLN_NETPRICE, Data.Price - Data.Discount);
			}
			else if(event.isCtlEvent(CTL_TSESSLN_QTTY) && !(Data.Flags & TSESLF_INDEPPHQTTY))
				SetupQtty(1, 0);
			// @v11.0.4 {
			else if(event.isCtlEvent(CTL_TSESSLN_LOTDIMX) || event.isCtlEvent(CTL_TSESSLN_LOTDIMY) || event.isCtlEvent(CTL_TSESSLN_LOTDIMZ)) {
				SString formula;
				uint dims_allowed = GetLotDimAllowence(&formula); // bit-mask X: 0x01, Y: 0x02, Z: 0x04
				if(dims_allowed && formula.NotEmptyS()) {
					SString temp_buf;
					GoodsContext::Param gcp;
					gcp.GoodsID = Data.GoodsID;
					gcp.LotDim.X = getCtrlReal(CTL_TSESSLN_LOTDIMX);
					gcp.LotDim.Y = getCtrlReal(CTL_TSESSLN_LOTDIMY);
					gcp.LotDim.Z = getCtrlReal(CTL_TSESSLN_LOTDIMZ);
					GoodsContext gctx(gcp);
					//GoodsContext::Param gcp = gctx.GetParam();
					double qtty = 0.0;
					if(PPExprParser::CalcExpression(formula, &qtty, 0, &gctx) && qtty > 0.0) {
						//setCtrlReal(CTL_LOT_QUANTITY, R6(qtty));
						//setupQuantity(CTL_LOT_QUANTITY, 1);
						setCtrlReal(CTL_TSESSLN_QTTY, qtty);
					}
				}
			}
			// } @v11.0.4 
			else
				return;
		}
		else
			return;
	}
	else if(TVKEYDOWN) {
		const  uint curr_ctl_id = GetCurrId();
		if(TVKEY == kbF2) {
			if(curr_ctl_id == CTL_TSESSLN_QTTY) {
				const double qtty = getCtrlReal(CTL_TSESSLN_QTTY);
				if(qtty > 0.0) {
					PPID   goods_id = getCtrlLong(CTLSEL_TSESSLN_GOODS);
					GoodsStockExt gse;
					if(GObj.GetStockExt(Data.GoodsID, &gse) > 0 && gse.Package > 0)
						SetupQtty(0, R0(qtty * gse.Package));
				}
			}
			else
				return;
		}
		else if(TVKEY == kbF4) {
			if(curr_ctl_id == CTL_TSESSLN_QTTY) {
				double phuperu;
				double qtty = 0.0;
				GetClusterData(CTL_TSESSLN_PLANFLAGS, &Data.Flags);
				if(!(Data.Flags & TSESLF_PLAN_PHUNIT) && getCtrlData(CTL_TSESSLN_QTTY, &qtty) && qtty > 0)
					if(Data.GoodsID && GObj.GetPhUPerU(Data.GoodsID, 0, &phuperu) > 0)
						SetupQtty(0, R6(qtty / phuperu));
			}
			else
				return;
		}
		else if(TVKEY == kbShiftF6) {
			if(curr_ctl_id == CTL_TSESSLN_QTTY) {
				double qtty = getCtrlReal(CTL_TSESSLN_QTTY);
				if(qtty > 0.0 && Data.GoodsID) {
					PPGoodsStruc gs;
					if(TSesObj.GetGoodsStruc(Data.TSessID, &gs) > 0 && gs.RecalcQttyByMainItemPh(&qtty) > 0)
						SetupQtty(0, qtty);
				}
			}
			else
				return;
		}
		else if(TVKEY == kbF9) {
			if(oneof2(curr_ctl_id, CTL_TSESSLN_QTTY, CTL_TSESSLN_INDEPPHQTTY)) {
				double result = 0.0;
				const  PPID   goods_id = getCtrlLong(CTLSEL_TSESSLN_GOODS);
				const  double _arg = getCtrlReal(curr_ctl_id);
				if(PPGoodsCalculator(goods_id, Data.TSessID, 1, _arg, &result) > 0 && result > 0.0) {
					setCtrlReal(curr_ctl_id, R6(result));
					if(curr_ctl_id == CTL_LOT_QUANTITY)
						SetupQtty(1, 0);
				}
			}
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

void TSessLineDialog::SetupPricing(int recalcDiscount)
{
	setCtrlReal(CTL_TSESSLN_PRICE, Data.Price);
	if(recalcDiscount)
		Data.Discount = Data.Price * fdiv100r(PctDis);
	setCtrlReal(CTL_TSESSLN_DSCNT, Data.Discount);
	setCtrlReal(CTL_TSESSLN_NETPRICE, Data.Price - Data.Discount);
}
//
//
//
int PPObjTSession::EditDialog(TSessionPacket * pData)
{
	int    ok = -1;
	uint   dlg_id = 0;
	ProcessorTbl::Rec prc_rec;
	if(pData->Rec.Flags & TSESF_SUPERSESS)
		dlg_id = DLG_TSES_SUP;
	else if(pData->Rec.Flags & TSESF_PLAN)
		dlg_id = DLG_TSES_PLAN;
	else if(PrcObj.Search(pData->Rec.PrcID, &prc_rec) > 0 && (prc_rec.Flags & PRCF_USETSESSSIMPLEDLG)) {
		TechTbl::Rec tec_rec;
		PPID   tec_id = pData->Rec.TechID;
		if(!tec_id) {
			PPIDArray tec_list;
			TecObj.GetListByPrcGoods(pData->Rec.PrcID, 0, &tec_list);
			if(tec_list.getCount() == 1)
				tec_id = pData->Rec.TechID = tec_list.get(0);
		}
		if(tec_id && TecObj.Fetch(tec_id, &tec_rec) > 0 && IsTimingTech(&tec_rec, 0) > 0) {
			//
			// Специальный случай: процессор имеет единственную повременную технологию, либо
			// повременная технология уже установлена в сессии.
			//
			dlg_id = DLG_TSESSTM;
		}
		else
			dlg_id = DLG_TSESSSIMPLE;
	}
	else
		dlg_id = DLG_TSESS;
	TSessionDialog * dlg = new TSessionDialog(dlg_id);
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(pData)) {
		int    r = 0;
		while(ok <= 0 && (r = ExecView(dlg)) == cmOK) {
			if(dlg->getDTS(pData))
				ok = 1;
		}
		if(r == cmCancel) {
			PPID   temp_id = 0;
			if(dlg->GetUpdatedStatus(&temp_id)) {
				// @v8.6.4 Изменения в блоке с целью избежать потери связи с чеком при отмене редактирования
				if(pData) {
					pData->Rec.ID = temp_id;
					SETIFZ(pData->Rec.CCheckID_, dlg->GetCCheckID());
					ok = 2;
				}
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjTSession::EditLineDialog(TSessLineTbl::Rec * pData, int asPlanLine)
{
	uint   dlg_id = asPlanLine ? DLG_TSESSPLANLN : DLG_TSESSLN;
	DIALOG_PROC_BODY_P1(TSessLineDialog, dlg_id, pData);
}
