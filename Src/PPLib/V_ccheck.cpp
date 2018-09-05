// V_CCHECK.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_PPFILT_FACTORY(CCheck); SLAPI CCheckFilt::CCheckFilt() : PPBaseFilt(PPFILT_CCHECK, 0, 4)
{
	SetFlatChunk(offsetof(CCheckFilt, ReserveStart), offsetof(CCheckFilt, SessIDList)-offsetof(CCheckFilt, ReserveStart));
	SetBranchSVector(offsetof(CCheckFilt, SessIDList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
	SetBranchObjIdListFilt(offsetof(CCheckFilt, NodeList));
	SetBranchObjIdListFilt(offsetof(CCheckFilt, CorrGoodsList));
	SetBranchObjIdListFilt(offsetof(CCheckFilt, CtValList));
	SetBranchObjIdListFilt(offsetof(CCheckFilt, ScsList));
	Init(1, 0);
}

CCheckFilt & FASTCALL CCheckFilt::operator = (const CCheckFilt & src)
{
	Copy(&src, 1);
	return *this;
}

//virtual
int SLAPI CCheckFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 3) {
		class CCheckFilt_v3 : public PPBaseFilt {
		public:
			SLAPI  CCheckFilt_v3() : PPBaseFilt(PPFILT_CCHECK, 0, 3)
			{
				SetFlatChunk(offsetof(CCheckFilt_v3, ReserveStart), offsetof(CCheckFilt_v3, SessIDList)-offsetof(CCheckFilt_v3, ReserveStart));
				SetBranchSVector(offsetof(CCheckFilt_v3, SessIDList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
				SetBranchObjIdListFilt(offsetof(CCheckFilt_v3, NodeList));
				SetBranchObjIdListFilt(offsetof(CCheckFilt_v3, CorrGoodsList));
				SetBranchObjIdListFilt(offsetof(CCheckFilt_v3, CtValList));
				Init(1, 0);
			}
			uint8  ReserveStart[24];
			PPID   DlvrAddrID;
			uint16 LowLinesCount;
			uint16 UppLinesCount;
			DateRange Period;
			TimeRange TimePeriod;
			int8   HourBefore;
			uint8  WeekDays;
			int16  GuestCount;
			long   CashNumber;
			Grouping Grp;
			long   Flags;
			PPID   GoodsGrpID;
			PPID   GoodsID;
			int16  Div;
			long   CtKind;
			char   Reserve[2];
			PPID   SCardSerID;
			PPID   SCardID;
			PPID   CashierID;
			PPID   AgentID;
			long   TableCode;
			IntRange  CodeR;
			RealRange AmtR;
			RealRange QttyR;
			RealRange PcntR;
			double    AmountQuant;
			long   SortOrder;        // Сортировка
			long   GcoMinCount;      // Параметры расчета попарных включений товаров в один чек
			SubstGrpGoods Sgg;       // Подстановка товара для попарных товаров
			PPIDArray SessIDList;    // @anchor
			ObjIdListFilt NodeList;  // Список узлов.
			ObjIdListFilt CorrGoodsList; // Список кореллирующих товаров
			ObjIdListFilt CtValList;     // CCheckFilt::ctvXXX Показатель, вычисляемый в кросстаб-отчете
		};
		CCheckFilt_v3 fv3;
		THROW(fv3.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
		Reserve = 0;
#define CPYFLD(f) f = fv3.f
		CPYFLD(DlvrAddrID);
		CPYFLD(LowLinesCount);
		CPYFLD(UppLinesCount);
		CPYFLD(Period);
		CPYFLD(TimePeriod);
		CPYFLD(HourBefore);
		CPYFLD(WeekDays);
		CPYFLD(GuestCount);
		CPYFLD(CashNumber);
		CPYFLD(Grp);
		CPYFLD(Flags);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(Div);
		CPYFLD(CtKind);
		CPYFLD(SCardSerID);
		CPYFLD(SCardID);
		CPYFLD(CashierID);
		CPYFLD(AgentID);
		CPYFLD(TableCode);
		CPYFLD(CodeR);
		CPYFLD(AmtR);
		CPYFLD(QttyR);
		CPYFLD(PcntR);
		CPYFLD(AmountQuant);
		CPYFLD(SortOrder);
		CPYFLD(GcoMinCount);
		CPYFLD(Sgg);

		CPYFLD(SessIDList);
		CPYFLD(NodeList);
		CPYFLD(CorrGoodsList);
		CPYFLD(CtValList);
#undef CPYFLD
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI CCheckFilt::SetLocList(const PPIDArray * pLocList)
{
	int    ok = -1;
	if(pLocList && pLocList->getCount()) {
		PPObjLocation loc_obj;
		PPObjCashNode cn_obj;
		PPIDArray loc_list;
		PPIDArray cn_list;
		loc_obj.ResolveWarehouseList(pLocList, loc_list);
		for(uint i = 0; i < loc_list.getCount(); i++) {
			if(cn_obj.GetListByLoc(loc_list.get(i), cn_list) > 0)
				ok = 1;
		}
		NodeList.Set(&cn_list);
	}
	else
		NodeList.Set(0);
	return ok;
}
//
//
//
int SLAPI PPViewCCheck::Helper_Construct()
{
	ImplementFlags |= implDontSetupCtColumnsOnChgFilt;
	ImplementFlags |= implUseServer;
	P_CC = 0;
	P_TmpTbl = 0;
	P_TmpGrpTbl = 0;
	P_TmpGdsCorrTbl = 0;
	CurLine = 0;
	State = 0;
	P_InOutVATList = 0;
	return 1;
}

SLAPI PPViewCCheck::PPViewCCheck() : PPView(0, &Filt, PPVIEW_CCHECK)
{
	Helper_Construct();
	P_CC = new CCheckCore;
	SETFLAG(State, stHasExt, P_CC->HasExt());
}

SLAPI PPViewCCheck::PPViewCCheck(CCheckCore & rOuterCc) : PPView(0, &Filt, PPVIEW_CCHECK)
{
	Helper_Construct();
	P_CC = &rOuterCc;
	State |= stOuterCc;
	SETFLAG(State, stHasExt, P_CC->HasExt());
}

SLAPI PPViewCCheck::~PPViewCCheck()
{
	delete P_TmpGrpTbl;
	delete P_TmpTbl;
	delete P_TmpGdsCorrTbl;
	delete P_InOutVATList;
	if(!(BaseState & bsServerInst))
		DBRemoveTempFiles();
	if(!(State & stOuterCc))
		delete P_CC;
}

class CCheckCrosstab : public Crosstab {
public:
	SLAPI  CCheckCrosstab(PPViewCCheck * pV) : Crosstab(), P_V(pV)
	{
	}
	virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
		{ return new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner); }
	virtual int  SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
		{ return (pVal && P_V) ? P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0; }
protected:
	PPViewCCheck * P_V;
};

int SLAPI PPViewCCheck::GetTabTitle(long tabID, SString & rBuf) const
{
	rBuf.Z();
	if(Filt.CtKind == CCheckFilt::ctDate) {
		LDATE dt = ZERODATE;
		dt.v = tabID;
		rBuf.Cat(dt);
	}
	return 1;
}

const BVATAccmArray * PPViewCCheck::GetInOutVATList() const { return P_InOutVATList; }
CCheckCore * SLAPI PPViewCCheck::GetCc() { return P_CC; }

int SLAPI PPViewCCheck::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, State, rBuf));
	THROW(CcIdList.Serialize(dir, rBuf, pCtx));
	THROW(NodeIdList.Serialize(dir, rBuf, pCtx));
	THROW(SessIdList.Serialize(dir, rBuf, pCtx));
	THROW(SCardList.Serialize(dir, rBuf, pCtx));
	THROW(GoodsList.Serialize(dir, rBuf, pCtx));
	THROW(pCtx->Serialize(dir, &SessCnList, rBuf));
	THROW(Gsl.Serialize(dir, rBuf, pCtx));

	THROW(SerializeDbTableByFileName <TempCCheckQttyTbl> (dir, &P_TmpTbl, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempCCheckGrpTbl>  (dir, &P_TmpGrpTbl, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempCCheckGdsCorrTbl> (dir, &P_TmpGdsCorrTbl, rBuf, pCtx));
	if(dir > 0) {
		THROW(pCtx->Serialize(dir, P_InOutVATList, rBuf));
	}
	else if(dir < 0) {
		THROW_MEM(P_InOutVATList = new BVATAccmArray(BVATF_SUMZEROVAT));
		THROW(pCtx->Serialize(dir, P_InOutVATList, rBuf));
	}
	CATCHZOK
	return ok;
}

PPBaseFilt * SLAPI PPViewCCheck::CreateFilt(void * extraPtr) const
{
	CCheckFilt * p_filt = new CCheckFilt;
	if(p_filt) {
		PPObjCashNode cn_obj;
		p_filt->NodeList.Add(cn_obj.GetSingle());
		p_filt->Flags |= CCheckFilt::fWithoutSkipTag;
	}
	else
		PPSetErrorNoMem();
	return p_filt;
}
//
//   CCheckFiltDialog
//
#define SHOW_CTVAL 0x00000001L

class CCheckFiltCtDialog : public PPListDialog {
public:
	CCheckFiltCtDialog() : PPListDialog(DLG_CROSST, CTL_CROSST_VALLIST)
	{
		PPLoadText(PPTXT_CCHECKCTVALNAMES, CtValNames);
		setSmartListBoxOption(CTL_CROSST_VALLIST, lbtSelNotify);
	}
	int    setDTS(const CCheckFilt * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		if(Data.Grp != CCheckFilt::gGoodsDate) {
			DisableClusterItem(CTL_CROSST_KIND, 1, 1);
			Data.CtKind = TrfrAnlzFilt::ctNone;
		}
		AddClusterAssocDef(CTL_CROSST_KIND, 0, TrfrAnlzFilt::ctNone);
		AddClusterAssoc(CTL_CROSST_KIND,  1, TrfrAnlzFilt::ctDate);
		SetClusterData(CTL_CROSST_KIND, Data.CtKind);
		if(!Data.CtValList.IsExists())
			Data.CtValList.InitEmpty();
		updateList(-1);
		return 1;
	}
	int    getDTS(CCheckFilt * pData)
	{
		int    ok = 1;
		GetClusterData(CTL_CROSST_KIND, &Data.CtKind);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isKeyDown(kbSpace)) {
			long sel_id = 0;
			if(getSelection(&sel_id) && sel_id) {
				ToggleFlag(sel_id);
				updateList(-1);
			}
			clearEvent(event);
		}
	}
	virtual int setupList();
	virtual int editItem(long pos, long id)
	{
		return ToggleFlag(id);
	}
	int    ToggleFlag(long itemId);
	SString CtValNames;
	CCheckFilt Data;
};

int CCheckFiltCtDialog::setupList()
{
	int    ok = 1;
	SString buf;
	StringSet text_list(';', CtValNames);
	StringSet ss(SLBColumnDelim);
	for(uint i = 0, j = 1; ok && text_list.get(&i, buf) > 0; j++) {
		ss.clear();
		ss.add(buf);
		buf.Z().CatChar(Data.CtValList.CheckID(j) ? 'v' : ' ');
		ss.add(buf);
		ok = addStringToList(j, ss.getBuf());
	}
	return ok;
}

int CCheckFiltCtDialog::ToggleFlag(long itemId)
{
	if(itemId) {
		if(Data.CtValList.CheckID(itemId))
			Data.CtValList.Remove(itemId);
		else
			Data.CtValList.Add(itemId);
		return 1;
	}
	else
		return -1;
}

class CCheckFiltDialog : public TDialog {
public:
	enum {
		ctlgroupGoodsFilt = 1,
		ctlgroupPosNode   = 2,
		ctlgroupSCard     = 3
	};
	CCheckFiltDialog(int hasExt) : TDialog(DLG_CCHECKFLT), HasExt(hasExt)
	{
		addGroup(ctlgroupPosNode, new PosNodeCtrlGroup(CTLSEL_CCHECKFLT_NODE, cmPosNodeList));
		addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(CTLSEL_CCHECKFLT_GOODS, CTLSEL_GOODSREST_GGRP, cmGoodsFilt)); // @fix CTLSEL_GOODSREST_GGRP-->CTLSEL_CCHECKFLT_GGRP
		addGroup(ctlgroupSCard,  new SCardCtrlGroup(CTLSEL_CCHECKFLT_SCSER, CTL_CCHECKFLT_SCARD, cmSCardSerList));
	}
	int    setDTS(const CCheckFilt *);
	int    getDTS(CCheckFilt *);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_CCHECKFLT_GRP)) {
			SetupCtrls();
			clearEvent(event);
		}
		else if(event.isClusterClk(CTL_CCHECKFLT_FLAGS)) {
			long preserve_flags = Data.Flags;
			GetClusterData(CTL_CCHECKFLT_FLAGS, &Data.Flags);
			if(Data.Flags & CCheckFilt::fSuspendedOnly && !(preserve_flags & CCheckFilt::fSuspendedOnly))
				Data.Flags |= CCheckFilt::fShowSuspended;
			if(!(Data.Flags & CCheckFilt::fShowSuspended) && preserve_flags & CCheckFilt::fShowSuspended)
				Data.Flags &= ~CCheckFilt::fSuspendedOnly;
			if(Data.Flags & CCheckFilt::fOrderOnly && !(preserve_flags & CCheckFilt::fOrderOnly))
				Data.Flags &= ~(CCheckFilt::fDlvrOnly | CCheckFilt::fDlvrOutstandOnly);
			if(Data.Flags & CCheckFilt::fDlvrOnly && !(preserve_flags & CCheckFilt::fDlvrOnly))
				Data.Flags &= ~CCheckFilt::fOrderOnly;
			else if(!(Data.Flags & CCheckFilt::fDlvrOnly) && preserve_flags & CCheckFilt::fDlvrOnly)
				Data.Flags &= ~CCheckFilt::fDlvrOutstandOnly;
			if(Data.Flags & CCheckFilt::fDlvrOutstandOnly && !(preserve_flags & CCheckFilt::fDlvrOutstandOnly)) {
				Data.Flags |= CCheckFilt::fDlvrOnly;
				Data.Flags &= ~CCheckFilt::fOrderOnly;
			}
			if(Data.Flags != preserve_flags)
				SetClusterData(CTL_CCHECKFLT_FLAGS, Data.Flags);
			clearEvent(event);
		}
		else if(event.isCmd(cmCrosstab))
			EditCrosstab();
	}
	int    EditCrosstab()
	{
		Data.Grp = (CCheckFilt::Grouping)getCtrlLong(CTLSEL_CCHECKFLT_GRP);
		DIALOG_PROC_BODYERR(CCheckFiltCtDialog, &Data);
	}
	void   SetupCtrls();

	const int HasExt;
	CCheckFilt Data;
};

void CCheckFiltDialog::SetupCtrls()
{
	CCheckFilt::Grouping  grp = (CCheckFilt::Grouping)getCtrlLong(CTLSEL_CCHECKFLT_GRP);
	if(!oneof3(grp, CCheckFilt::gAmount, CCheckFilt::gQtty, CCheckFilt::gAmountNGoods)) {
		Data.AmountQuant = 0.0;
		setCtrlData(CTL_CCHECKFLT_AMTQUANT, &Data.AmountQuant);
		disableCtrl(CTL_CCHECKFLT_AMTQUANT, 1);
	}
	else
		disableCtrl(CTL_CCHECKFLT_AMTQUANT, 0);
	disableCtrl(CTLSEL_CCHECKFLT_SUBST, !CCheckFilt::HasGoodsGrouping(grp));
	if(grp == CCheckFilt::gNone) {
		SetClusterData(CTL_CCHECKFLT_ORDER, Data.SortOrder = CCheckFilt::ordByDef);
		disableCtrl(CTL_CCHECKFLT_ORDER, 1);
	}
	else {
		disableCtrl(CTL_CCHECKFLT_ORDER, 0);
		if(oneof2(grp, CCheckFilt::gQtty, CCheckFilt::gGoods)) {
			DisableClusterItem(CTL_CCHECKFLT_ORDER, 2, 0);
		}
		else {
			GetClusterData(CTL_CCHECKFLT_ORDER, &Data.SortOrder);
			if(Data.SortOrder == CCheckFilt::ordByQtty)
				SetClusterData(CTL_CCHECKFLT_ORDER, Data.SortOrder = CCheckFilt::ordByDef);
			DisableClusterItem(CTL_CCHECKFLT_ORDER, 2, 1);
		}
	}
	if(!CCheckFilt::HasGoodsGrouping(grp))
		setCtrlData(CTLSEL_CCHECKFLT_SUBST, 0);
	enableCommand(cmCrosstab, BIN(grp == CCheckFilt::gGoodsDate));
	if(grp != CCheckFilt::gGoodsDate) {
		Data.CtKind = CCheckFilt::ctNone;
		Data.CtValList.FreeAll();
	}
}

int CCheckFiltDialog::setDTS(const CCheckFilt * pFilt)
{
	int    ok = -1;
	if(pFilt) {
		Data = *pFilt;
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		GoodsFiltCtrlGroup::Rec grp_rec(Data.GoodsGrpID, Data.GoodsID);
		SetupCalPeriod(CTLCAL_CCHECKFLT_PERIOD, CTL_CCHECKFLT_PERIOD);
		disableCtrls(Data.SessIDList.getCount(), CTL_CCHECKFLT_PERIOD, /*CTL_CCHECKFLT_FLAGS,*/ 0);
		SetPeriodInput(this, CTL_CCHECKFLT_PERIOD, &Data.Period);
		if(HasExt)
			setCtrlUInt16(CTL_CCHECKFLT_STARTSRVCP, BIN(Data.Flags & CCheckFilt::fStartOrderPeriod));
		else {
			Data.Flags &= ~CCheckFilt::fStartOrderPeriod;
			disableCtrl(CTL_CCHECKFLT_STARTSRVCP, 1);
		}
		{
			if(Data.NodeList.GetCount() == 0) {
				Data.NodeList.Add(eq_cfg.DefCashNodeID);
			}
			PosNodeCtrlGroup::Rec cn_rec(&Data.NodeList);
			setGroupData(ctlgroupPosNode, &cn_rec);
		}
		setCtrlData(CTL_CCHECKFLT_CASHN, &Data.CashNumber);
		SetIntRangeInput(this, CTL_CCHECKFLT_CODERANGE, &Data.CodeR);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  0, CCheckFilt::fZeroSess);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  1, CCheckFilt::fShowSuspended);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  2, CCheckFilt::fSuspendedOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  3, CCheckFilt::fRetOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  4, CCheckFilt::fJunkOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  5, CCheckFilt::fNotPrintedOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  6, CCheckFilt::fGiftOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  7, CCheckFilt::fOrderOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  8, CCheckFilt::fDlvrOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS,  9, CCheckFilt::fDlvrOutstandOnly);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS, 10, CCheckFilt::fCalcSkuStat);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS, 11, CCheckFilt::fWithoutSkipTag);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS, 12, CCheckFilt::fPrintDetail);
		AddClusterAssoc(CTL_CCHECKFLT_FLAGS, 13, CCheckFilt::fNotSpFinished); // @v9.7.5
		SetClusterData(CTL_CCHECKFLT_FLAGS, Data.Flags);
		grp_rec.Flags |= GoodsCtrlGroup::enableSelUpLevel;
		setGroupData(ctlgroupGoodsFilt, &grp_rec);
		{
			SCardCtrlGroup::Rec screc;
			const PPIDArray * p_temp_list = &Data.ScsList.Get();
			RVALUEPTR(screc.SCardSerList, p_temp_list);
			screc.SCardID = Data.SCardID;
			setGroupData(ctlgroupSCard, &screc);
		}
		SetupArCombo(this, CTLSEL_CCHECKFLT_AGENT, Data.AgentID, OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet);
		setCtrlData(CTL_CCHECKFLT_TABLECODE, &Data.TableCode);
		SetupStringCombo(this, CTLSEL_CCHECKFLT_GRP, PPTXT_CCHECKGROUPING, Data.Grp);
		if(oneof3(Data.Grp, CCheckFilt::gAmount, CCheckFilt::gQtty, CCheckFilt::gAmountNGoods))
			setCtrlData(CTL_CCHECKFLT_AMTQUANT, &Data.AmountQuant);
		AddClusterAssoc(CTL_CCHECKFLT_ORDER, 0, CCheckFilt::ordByDef);
		AddClusterAssoc(CTL_CCHECKFLT_ORDER, 1, CCheckFilt::ordByCount);
		AddClusterAssoc(CTL_CCHECKFLT_ORDER, 2, CCheckFilt::ordByQtty);
		AddClusterAssoc(CTL_CCHECKFLT_ORDER, 3, CCheckFilt::ordByAmt);
		SetClusterData(CTL_CCHECKFLT_ORDER, Data.SortOrder);
		long cob = 0;
		if(Data.Flags & CCheckFilt::fCashOnly)
			cob = 1;
		else if(Data.Flags & CCheckFilt::fBankingOnly)
			cob = 2;
		AddClusterAssocDef(CTL_CCHECKFLT_CASHORBANK,  0, 0);
		AddClusterAssoc(CTL_CCHECKFLT_CASHORBANK,  1, 1);
		AddClusterAssoc(CTL_CCHECKFLT_CASHORBANK,  2, 2);
		SetClusterData(CTL_CCHECKFLT_CASHORBANK, cob);
		// @v10.0.02 {
		AddClusterAssocDef(CTL_CCHECKFLT_ALTREG, 0, 0);
		AddClusterAssoc(CTL_CCHECKFLT_ALTREG, 1, +1);
		AddClusterAssoc(CTL_CCHECKFLT_ALTREG, 2, -1);
		SetClusterData(CTL_CCHECKFLT_ALTREG, Data.AltRegF);
		// } @v10.0.02
		SetupSubstGoodsCombo(this, CTLSEL_CCHECKFLT_SUBST, Data.Sgg);
		SetupPersonCombo(this, CTLSEL_CCHECKFLT_CSHR, Data.CashierID, 0, eq_cfg.CshrsPsnKindID, 1); // @v10.1.8
		SetupCtrls();
		selectCtrl(CTL_CCHECKFLT_PERIOD);
		ok = 1;
	}
	return ok;
}

int CCheckFiltDialog::getDTS(CCheckFilt * pFilt)
{
	int    ok = 1;
	GoodsFiltCtrlGroup::Rec grp_rec;
	GetPeriodInput(this, CTL_CCHECKFLT_PERIOD, &Data.Period);
	if(HasExt) {
		SETFLAG(Data.Flags, CCheckFilt::fStartOrderPeriod, getCtrlUInt16(CTL_CCHECKFLT_STARTSRVCP));
	}
	else
		Data.Flags &= ~CCheckFilt::fStartOrderPeriod;
	{
		PosNodeCtrlGroup::Rec cn_rec;
		getGroupData(ctlgroupPosNode, &cn_rec);
		Data.NodeList = cn_rec.List;
	}
	getCtrlData(CTL_CCHECKFLT_CASHN, &Data.CashNumber);
	Data.CodeR.Set(0); // @v9.6.8 при пустой строке диапазон не меняется
	GetIntRangeInput(this, CTL_CCHECKFLT_CODERANGE, &Data.CodeR);
	GetClusterData(CTL_CCHECKFLT_FLAGS, &Data.Flags);
	getGroupData(ctlgroupGoodsFilt, &grp_rec);
	Data.GoodsGrpID = grp_rec.GoodsGrpID;
	Data.GoodsID = grp_rec.GoodsID;
	{
		SCardCtrlGroup::Rec screc;
		getGroupData(ctlgroupSCard, &screc);
		Data.ScsList.Set(&screc.SCardSerList);
		Data.SCardID = screc.SCardID;
	}
	getCtrlData(CTLSEL_CCHECKFLT_AGENT, &Data.AgentID);
	getCtrlData(CTLSEL_CCHECKFLT_CSHR, &Data.CashierID); // @v10.1.8
	getCtrlData(CTL_CCHECKFLT_TABLECODE, &Data.TableCode);
	Data.Grp = (CCheckFilt::Grouping)getCtrlLong(CTLSEL_CCHECKFLT_GRP);
	if(oneof3(Data.Grp, CCheckFilt::gAmount, CCheckFilt::gQtty, CCheckFilt::gAmountNGoods)) {
		const double min_amt_qtty = (Data.Grp == CCheckFilt::gQtty) ? 0.01 : 0.1;
		getCtrlData(CTL_CCHECKFLT_AMTQUANT, &Data.AmountQuant);
		if(Data.AmountQuant < min_amt_qtty)
			ok = PPErrorByDialog(this, CTL_CCHECKFLT_AMTQUANT, PPERR_USERINPUT);
	}
	else
		Data.AmountQuant = (Data.Grp == CCheckFilt::gNone) ? 0.0 : -1.0;
	GetClusterData(CTL_CCHECKFLT_ORDER, &Data.SortOrder);
	long   temp_long = 0;
	GetClusterData(CTL_CCHECKFLT_CASHORBANK, &temp_long);
	Data.Flags &= ~(CCheckFilt::fCashOnly | CCheckFilt::fBankingOnly);
	if(temp_long == 1)
		Data.Flags |= CCheckFilt::fCashOnly;
	else if(temp_long == 2)
		Data.Flags |= CCheckFilt::fBankingOnly;
	Data.AltRegF = (int8)GetClusterData(CTL_CCHECKFLT_ALTREG); // @v10.0.02
	getCtrlData(CTLSEL_CCHECKFLT_SUBST, &Data.Sgg);
	if(ok)
		ASSIGN_PTR(pFilt, Data);
	return ok;
}
//
//
//
int SLAPI PPViewCCheck::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	CCheckFilt * p_filt = (CCheckFilt *)pBaseFilt;
	DIALOG_PROC_BODY_P1(CCheckFiltDialog, BIN(State & stHasExt), p_filt);
}

void SLAPI PPViewCCheck::PreprocessCheckRec(const CCheckTbl::Rec * pRec, CCheckTbl::Rec & rResultRec, CCheckExtTbl::Rec & rExtRec)
{
	rResultRec = *pRec;
	MEMSZERO(rExtRec);
	const long ff_ = Filt.Flags;
	if(pRec->Flags & CCHKF_EXT && P_CC->GetExt(pRec->ID, &rExtRec) > 0) {
		if(ff_ & CCheckFilt::fStartOrderPeriod) {
			rResultRec.Dt = rExtRec.CreationDtm.d;
			rResultRec.Tm = rExtRec.CreationDtm.t;
		}
	}
}

int FASTCALL PPViewCCheck::CheckForFilt(const CCheckTbl::Rec * pRec, const CCheckExtTbl::Rec * pExtRec)
{
	if(Filt.CashNumber && pRec->CashID != Filt.CashNumber)
		return 0;
	else if(Filt.CashierID && pRec->UserID != Filt.CashierID)
		return 0;
	else {
		const long ff_ = Filt.Flags;
		const long f = pRec->Flags;
		if(!(ff_ & CCheckFilt::fStartOrderPeriod) && !Filt.Period.CheckDate(pRec->Dt))
			return 0;
		else if(!Filt.CodeR.CheckVal(pRec->Code))
			return 0;
		else {
			if(Filt.SCardID) {
				if(pRec->SCardID != Filt.SCardID) {
					if(pRec->Flags & CCHKF_PAYMLIST) {
						CCheckPaymTbl::Key1 pk1;
						pk1.SCardID = Filt.SCardID;
						pk1.CheckID = pRec->ID;
						if(!P_CC->PaymT.search(1, &pk1, spEq))
							return 0;
					}
					else
						return 0;
				}
			}
			else if(ff_ & CCheckFilt::fCashOnly && (f & CCHKF_BANKING))
				return 0;
			else if(ff_ & CCheckFilt::fBankingOnly && !(f & CCHKF_BANKING))
				return 0;
			else if(!(ff_ & (CCheckFilt::fShowSuspended|CCheckFilt::fCTableStatus|CCheckFilt::fJunkOnly)) && f & CCHKF_SUSPENDED)
				return 0;
			else if(ff_ & CCheckFilt::fSuspendedOnly && !(f & CCHKF_SUSPENDED))
				return 0;
			else if(ff_ & CCheckFilt::fRetOnly && !(f & CCHKF_RETURN))
				return 0;
			else if(ff_ & CCheckFilt::fJunkOnly) {
				if(!(f & CCHKF_JUNK))
					return 0;
			}
			else {
				if(f & CCHKF_JUNK) {
					int    suit_ = 0;
					if(ff_ & CCheckFilt::fLostJunkAsSusp && ff_ & CCheckFilt::fShowSuspended) {
						PPSession::RegSessData rsd;
						if(P_CC->IsLostJunkCheck(pRec->ID, &Filt.LostJunkUUID, &rsd) > 0)
							suit_ = 1;
					}
					if(!suit_)
						return 0;
				}
			}
			if(ff_ & CCheckFilt::fNotPrintedOnly && f & CCHKF_PRINTED)
				return 0;
			else if(ff_ & CCheckFilt::fGiftOnly && !(f & CCHKF_HASGIFT))
				return 0;
			else if(ff_ & CCheckFilt::fOrderOnly && !(f & CCHKF_ORDER))
				return 0;
			else if(ff_ & CCheckFilt::fDlvrOnly && !(f & CCHKF_DELIVERY))
				return 0;
			else if(ff_ & CCheckFilt::fDlvrOutstandOnly && (!(f & CCHKF_DELIVERY) || (f & CCHKF_CLOSEDORDER)))
				return 0;
			else if(ff_ & CCheckFilt::fCTableStatus && !(f & (CCHKF_SUSPENDED|CCHKF_ORDER)))
				return 0;
			// @v9.7.11 дополнение по состоянию stSkipUnprinted
			else if(ff_ & CCheckFilt::fWithoutSkipTag && ((f & CCHKF_SKIP) || (State & stSkipUnprinted && !(f & CCHKF_PRINTED))))
				return 0;
			else if(ff_ & CCheckFilt::fNotSpFinished && f & CCHKF_SPFINISHED) // @v9.7.5
				return 0;
			else if(Filt.WeekDays && !(Filt.WeekDays & (1 << dayofweek(&pRec->Dt, 0))))
				return 0;
			// @v10.0.02 {
			else if(Filt.AltRegF > 0 && !(f & CCHKF_ALTREG))
				return 0;
			else if(Filt.AltRegF < 0 && (f & CCHKF_ALTREG))
				return 0;
			// } @v10.0.02
			else if(Filt.HourBefore) {
				LTIME  hour_low = encodetime(Filt.HourBefore-1, 0, 0, 0);
				LTIME  hour_upp = encodetime(Filt.HourBefore,   0, 0, 0);
				if(pRec->Tm < hour_low || pRec->Tm >= hour_upp)
					return 0;
			}
			if(!SessIdList.CheckID(pRec->SessID))
				return 0;
			else if(!SCardList.CheckID(pRec->SCardID)) {
				if(pRec->Flags & CCHKF_PAYMLIST) {
					CcAmountList cp_list;
					P_CC->GetPaymList(pRec->ID, cp_list);
					int    _f = 0;
					for(uint i = 0; !_f && i < SCardList.GetCount(); i++) {
						const PPID sc_id = SCardList.Get().get(i);
						if(cp_list.SearchAddedID(sc_id, 0))
							_f = 1;
					}
					if(!_f)
						return 0;
				}
				else
					return 0;
			}
			if(!!CcIdList && !CcIdList.Search(pRec->ID, 0, 1))
				return 0;
			else if(Filt.HasExtFiltering() || (ff_ & CCheckFilt::fCTableStatus)) {
				if(pRec->Flags & CCHKF_EXT && pExtRec) {
					if(Filt.AgentID && pExtRec->SalerID != Filt.AgentID)
						return 0;
					else if(Filt.TableCode && pExtRec->TableNo != Filt.TableCode)
						return 0;
					else if(Filt.GuestCount && pExtRec->GuestCount != Filt.GuestCount)
						return 0;
					else if(ff_ & CCheckFilt::fCTableStatus && pExtRec->TableNo == 0)
						return 0;
					else if(ff_ & CCheckFilt::fZeroDlvrAddr && pExtRec->AddrID)
						return 0;
					else if(Filt.DlvrAddrID && pExtRec->AddrID != Filt.DlvrAddrID)
						return 0;
					else if(ff_ & CCheckFilt::fStartOrderPeriod && !Filt.Period.CheckDate(pExtRec->CreationDtm.d))
						return 0;
				}
				else
					return 0;
			}
			if(NodeIdList.GetCount()) {
				long   cn_id = 0;
				//
				// Если условия фильтрации требуют отображения junk или потерянных junk-чеков,
				// то для junk-чека игнорируем фильтр по сессии, а кассовый узел проверяем по CCheckTbl::Rec::CashID
				// (это всегда синхронные кассы)
				//
				if(f & CCHKF_JUNK && (ff_ & CCheckFilt::fLostJunkAsSusp && ff_ & CCheckFilt::fShowSuspended)) {
					if(!NodeIdList.CheckID(pRec->CashID))
						return 0;
				}
				else if(SessCnList.Search(pRec->SessID, &cn_id, 0)) {
					if(!NodeIdList.CheckID(cn_id))
						return 0;
				}
				else {
					//
					// Так как сессий может быть очень много, причем чеки по ним
					// просматриваются достаточно локально, то не будем зря забивать память
					// и время от времени станем очищать таблицу ассоциаций.
					//
					const uint max_sescnlist_size = 512;
					if(SessCnList.getCount() >= max_sescnlist_size)
						SessCnList.clear();
					//
					CSessionTbl::Rec cs_rec;
					if(CsObj.Fetch(pRec->SessID, &cs_rec) > 0) {
						SessCnList.Add(cs_rec.ID, cs_rec.CashNodeID, 0);
						if(!NodeIdList.CheckID(cs_rec.CashNodeID))
							return 0;
					}
					else {
						//
						// Висячий ид сессии. Добавляем его в таблицу ассоциаций с нулевым
						// идентификатором кассового узла и считаем, что проверка не выполнена.
						//
						SessCnList.Add(pRec->SessID, 0, 0);
						return 0;
					}
				}
			}
			if(!(State & stUseGoodsList) || ff_ & CCheckFilt::fFiltByCheck) {
				const double amt = MONEYTOLDBL(pRec->Amount);
				if(!Filt.AmtR.CheckVal(fabs(amt)))
					return 0;
				else if(!Filt.PcntR.IsZero()) {
					double dis = MONEYTOLDBL(pRec->Discount);
					if(!Filt.PcntR.CheckVal(fabs(fdivnz(dis, amt+dis)) * 100.0)) // @pctdis
						return 0;
				}
			}
			return 1;
		}
	}
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempCCheckQtty);
PP_CREATE_TEMP_FILE_PROC(CreateTempGrpFile, TempCCheckGrp);
PP_CREATE_TEMP_FILE_PROC(CreateTempGdsCorrFile, TempCCheckGdsCorr);

int SLAPI PPViewCCheck::DoProcessLines() const
{
	return BIN((State & stUseGoodsList) || !Filt.QttyR.IsZero() ||
		(!(Filt.Flags & CCheckFilt::fFiltByCheck) && (!Filt.AmtR.IsZero() || !Filt.PcntR.IsZero())) ||
		Filt.LowLinesCount > 0 || Filt.UppLinesCount > 0 || Filt.Div);
}

int FASTCALL PPViewCCheck::CheckLineForFilt(const CCheckLineTbl::Rec & rLnRec)
{
	int    ok = 0;
	if(!(State & stUseGoodsList) || GoodsList.Has((uint)rLnRec.GoodsID)) {
		const double qtty = rLnRec.Quantity;
		if(Filt.Flags & CCheckFilt::fFiltByCheck || (
			Filt.QttyR.CheckVal(fabs(qtty)) &&
			Filt.AmtR.CheckVal(qtty * intmnytodbl(rLnRec.Price)) &&
			Filt.PcntR.CheckVal(qtty * rLnRec.Dscnt)) && (!Filt.Div || rLnRec.DivID == Filt.Div)) {
			ok = 1;
		}
	}
	return ok;
}

static TempCCheckQttyTbl::Rec & FASTCALL CCheckRec_To_TempCCheckQttyRec(const CCheckTbl::Rec & rSrc, TempCCheckQttyTbl::Rec & rDest)
{
	rDest.ID = rSrc.ID;
	rDest.Code = rSrc.Code;
	rDest.CashID = rSrc.CashID;
	rDest.UserID = rSrc.UserID;
	rDest.SessID = rSrc.SessID;
	rDest.Dt = rSrc.Dt;
	rDest.Tm = rSrc.Tm;
	rDest.Amount = MONEYTOLDBL(rSrc.Amount);
	rDest.Discount = MONEYTOLDBL(rSrc.Discount);
	rDest.SCardID = rSrc.SCardID;
	rDest.Flags = rSrc.Flags;
	return rDest;
}

static CCheckTbl::Rec & FASTCALL TempCCheckQttyRec_To_CCheckRec(const TempCCheckQttyTbl::Rec & rSrc, CCheckTbl::Rec & rDest)
{
	rDest.ID = rSrc.ID;
	rDest.Code = rSrc.Code;
	rDest.CashID = rSrc.CashID;
	rDest.UserID = rSrc.UserID;
	rDest.SessID = rSrc.SessID;
	rDest.Dt = rSrc.Dt;
	rDest.Tm = rSrc.Tm;
	LDBLTOMONEY(rSrc.Amount, rDest.Amount);
	LDBLTOMONEY(rSrc.Discount, rDest.Discount);
	rDest.SCardID = rSrc.SCardID;
	rDest.Flags = rSrc.Flags;
	return rDest;
}

int SLAPI PPViewCCheck::ProcessCheckRec(const CCheckTbl::Rec * pRec, BExtInsert * pBei)
{
	int    ok = -1;
	CCheckLineTbl::Rec ln_rec;
	CCheckTbl::Rec _rec;
	CCheckExtTbl::Rec _ext_rec;
	PreprocessCheckRec(pRec, _rec, _ext_rec);
	if(CheckForFilt(&_rec, &_ext_rec)) {
		if(DoProcessLines()) {
			TempCCheckQttyTbl::Rec rec;
			MEMSZERO(rec);
			CCheckRec_To_TempCCheckQttyRec(_rec, rec);
			double qtty = 0.0, amt = 0.0, dscnt = 0.0, t_dscnt = 0.0, pcnt = 0.0;
			BVATAccmArray temp_bva_ary;
			CcPack.ClearLines();
			if(P_CC->LoadLines(rec.ID, 0, &CcPack)) {
				PPIDArray goods_id_list;
				const uint c = CcPack.GetCount();
				if(c && ((Filt.LowLinesCount <= 0 && Filt.UppLinesCount <= 0) || (c >= Filt.LowLinesCount && c <= Filt.UppLinesCount))) {
					for(uint i = 0; CcPack.EnumLines(&i, &ln_rec);) {
						const double ln_q = ln_rec.Quantity;
						if(CheckLineForFilt(ln_rec)) {
							const double ln_p = intmnytodbl(ln_rec.Price);
							qtty  += ln_q;
							amt   += ln_q * ln_p;
							dscnt += ln_q * ln_rec.Dscnt;
							{
								BVATAccm  bva_item;
								PPGoodsTaxEntry gtx;
								if(GdsObj.FetchTax(ln_rec.GoodsID, LConfig.OperDate, 0L, &gtx) > 0)
									bva_item.PRate = gtx.GetVatRate();
								bva_item.PTrnovr  += ln_q * (ln_p - ln_rec.Dscnt);
								bva_item.Discount += ln_q * ln_rec.Dscnt;
								THROW(temp_bva_ary.Add(&bva_item));
							}
							if(Filt.Flags & CCheckFilt::fCalcSkuStat)
								goods_id_list.add(ln_rec.GoodsID);
							ok = 1;
						}
						t_dscnt += ln_q * ln_rec.Dscnt;
					}
					goods_id_list.sortAndUndup();
				}
				if(ok > 0 && Filt.Flags & CCheckFilt::fFiltByCheck && Filt.QttyR.CheckVal(fabs(qtty)))
					ok = -1;
				if(ok > 0) {
					rec.LinesCount = (long)c;
					rec.SkuCount = (long)goods_id_list.getCount();
					if(State & stUseGoodsList) {
						double  chk_dscnt = rec.Discount - t_dscnt;
						if(chk_dscnt != 0.0)
							dscnt += fdivnz(chk_dscnt * amt, rec.Amount + rec.Discount);
						rec.Amount = amt - dscnt;
						rec.Discount = dscnt;
					}
					if(!(Filt.Flags & CCheckFilt::fFiltByCheck)) {
						pcnt = fabs(fdivnz(dscnt, amt + dscnt)) * 100.0; // @pctdis
						ok = (Filt.AmtR.CheckVal(fabs(amt - dscnt)) && (Filt.PcntR.IsZero() || Filt.PcntR.CheckVal(pcnt))) ? 1 : -1;
					}
				}
				if(ok > 0) {
					for(uint c = 0; c < temp_bva_ary.getCount(); c++)
						THROW(P_InOutVATList->Add(&temp_bva_ary.at(c)));
					if(pBei) {
						rec.Qtty = qtty;
						THROW_DB(pBei->insert(&rec));
					}
				}
			}
		}
		else {
			if(pBei) {
				TempCCheckQttyTbl::Rec rec;
				MEMSZERO(rec);
				CCheckRec_To_TempCCheckQttyRec(_rec, rec);
				if(Filt.Flags & CCheckFilt::fCalcSkuStat) {
					PPIDArray goods_id_list;
					CcPack.ClearLines();
					if(P_CC->LoadLines(rec.ID, 0, &CcPack))
						for(uint i = 0; CcPack.EnumLines(&i, &ln_rec);)
							goods_id_list.add(ln_rec.GoodsID);
					goods_id_list.sortAndUndup();
					rec.LinesCount = (long)CcPack.GetCount();
					rec.SkuCount = (long)goods_id_list.getCount();
				}
				THROW_DB(pBei->insert(&rec));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

struct CCheckGrpItem { // @flat @size=72
	LDATE  Dt;
	LTIME  Tm;
	long   CashID;
	long   SCardID;
	long   GoodsID;
	long   Count;
	long   LinesCount;
	long   SkuCount;
	double Amount;
	double Discount;
	double BnkAmt;
	double CrdCardAmt;
	double Qtty;
};

IMPL_CMPFUNC(CCheckGrpItem, p1, p2) { RET_CMPCASCADE5((const CCheckGrpItem *)p1, (const CCheckGrpItem *)p2, Dt, Tm, CashID, SCardID, GoodsID); }

class CCheckGrpCache : SVector { // @v9.8.4 SArray-->SVector
public:
	SLAPI  CCheckGrpCache(size_t maxItems, TempCCheckGrpTbl * pTbl) : SVector(sizeof(CCheckGrpItem)), P_Tbl(pTbl) // @v9.8.4 SArray-->SVector
	{
		MaxItems = (maxItems > 0) ? maxItems : 1024;
	}
	int    FASTCALL AddItem(const CCheckGrpItem *);
	int    SLAPI Flash()
	{
		CCheckGrpItem * p_item;
		for(uint i = 0; enumItems(&i, (void **)&p_item);)
			if(!FlashItem(p_item))
				return 0;
		return 1;
	}
private:
	int    SLAPI SearchItem(const CCheckGrpItem * pKey, CCheckGrpItem * pItem);
	int    SLAPI FlashItem(const CCheckGrpItem *);

	size_t MaxItems;
	TempCCheckGrpTbl * P_Tbl;
};

int SLAPI CCheckGrpCache::SearchItem(const CCheckGrpItem * pKey, CCheckGrpItem * pItem)
{
	TempCCheckGrpTbl::Key1 k1;
	k1.Dt = pKey->Dt;
	k1.Tm = pKey->Tm;
	k1.CashID  = pKey->CashID;
	k1.SCardID = pKey->SCardID;
	k1.GoodsID = pKey->GoodsID;
	if(P_Tbl->search(1, &k1, spEq)) {
		if(pItem) {
			pItem->Dt       = P_Tbl->data.Dt;
			pItem->Tm       = P_Tbl->data.Tm;
			pItem->CashID   = P_Tbl->data.CashID;
			pItem->SCardID  = P_Tbl->data.SCardID;
			pItem->GoodsID  = P_Tbl->data.GoodsID;
			pItem->Count    = P_Tbl->data.Count;
			pItem->LinesCount = P_Tbl->data.LinesCount;
			pItem->SkuCount   = P_Tbl->data.SkuCount;
			pItem->Amount   = P_Tbl->data.Amount;
			pItem->Discount = P_Tbl->data.Discount;
			pItem->BnkAmt   = P_Tbl->data.BnkAmt;
			pItem->CrdCardAmt = P_Tbl->data.CrdCardAmt;
			pItem->Qtty     = P_Tbl->data.Qtty;
		}
		return 1;
	}
	return -1;
}

int SLAPI CCheckGrpCache::FlashItem(const CCheckGrpItem * pItem)
{
	int    ok = 1;
	TempCCheckGrpTbl::Rec rec;
	MEMSZERO(rec);
	rec.Dt       = pItem->Dt;
	rec.Tm       = pItem->Tm;
	rec.CashID   = pItem->CashID;
	rec.SCardID  = pItem->SCardID;
	rec.GoodsID  = pItem->GoodsID;
	rec.Count    = pItem->Count;
	rec.LinesCount = pItem->LinesCount;
	rec.SkuCount = pItem->SkuCount;
	rec.Amount   = pItem->Amount;
	rec.Discount = pItem->Discount;
	rec.BnkAmt   = pItem->BnkAmt;
	rec.CrdCardAmt = pItem->CrdCardAmt;
	rec.Qtty     = pItem->Qtty;
	if(SearchItem(pItem, 0) > 0) {
		rec.ID__ = P_Tbl->data.ID__;
		ok = P_Tbl->updateRecBuf(&rec) ? 1 : PPSetErrorDB();
	}
	else {
		ok = P_Tbl->insertRecBuf(&rec) ? 1 : PPSetErrorDB();
	}
	return ok;
}

int FASTCALL CCheckGrpCache::AddItem(const CCheckGrpItem * pItem)
{
	int    ok = 1;
	uint   pos = 0;
	CCheckGrpItem * p_item, item;
	if(bsearch(pItem, &pos, PTR_CMPFUNC(CCheckGrpItem))) {
		p_item = (CCheckGrpItem *)at(pos);
		p_item->Count    += pItem->Count;
		p_item->LinesCount += pItem->LinesCount;
		p_item->SkuCount  += pItem->SkuCount;
		p_item->Amount   += pItem->Amount;
		p_item->Discount += pItem->Discount;
		p_item->BnkAmt   += pItem->BnkAmt;
		p_item->CrdCardAmt += pItem->CrdCardAmt;
		p_item->Qtty     += pItem->Qtty;
	}
	else {
		while(getCount() >= MaxItems) {
			uint min_pos = 0;
			long min_count = MAXLONG;
			for(uint i = 0; enumItems(&i, (void **)&p_item);)
				if(p_item->Count <= min_count) {
					min_count = p_item->Count;
					min_pos = i;
				}
			THROW(min_pos > 0); // Mystik error
			THROW(FlashItem((CCheckGrpItem *)at(min_pos-1)));
			atFree(min_pos-1);
		}
		if(SearchItem(pItem, &item) > 0) {
			item.Count    += pItem->Count;
			item.LinesCount += pItem->LinesCount;
			item.SkuCount  += pItem->SkuCount;
			item.Amount   += pItem->Amount;
			item.Discount += pItem->Discount;
			item.BnkAmt   += pItem->BnkAmt;
			item.CrdCardAmt += pItem->CrdCardAmt;
			item.Qtty     += pItem->Qtty;
		}
		else
			item = *pItem;
		THROW_SL(ordInsert(&item, 0, PTR_CMPFUNC(CCheckGrpItem)));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewCCheck::IsTempTblNeeded() const
{
	return BIN((State & stUseGoodsList) || SessIdList.GetCount() > 1 ||
		Filt.SCardSerID || Filt.SCardID || Filt.ScsList.GetCount() || Filt.HourBefore || Filt.WeekDays || !Filt.QttyR.IsZero() ||
		(Filt.LowLinesCount > 0 || Filt.UppLinesCount > 0) ||
		(Filt.Flags & (CCheckFilt::fCalcSkuStat|CCheckFilt::fStartOrderPeriod)) || !!CcIdList);
}

int SLAPI PPViewCCheck::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, r;
	SString temp_buf, name_buf, goods_name;
	PPIDArray temp_list;
	CCheckCore * p_cct = P_CC;
	PPObjLocation loc_obj;
	PPObjCashNode cn_obj;
	PPObjSCardSeries sc_obj;
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	THROW(ObjRts.AdjustCSessPeriod(Filt.Period, 0)); // @v9.3.0
	ZDELETE(P_TmpGrpTbl);
	CcIdList.Set(0);
	SCardList.Set(0);
	SessIdList.Set(0);
	NodeIdList.Set(0);
	GoodsList.Clear();
	State &= ~(stUseGoodsList|stSkipUnprinted); // @v9.7.11 stSkipUnprinted
	Gsl.Init(1, 0);
	ZDELETE(P_InOutVATList);
	THROW_MEM(P_InOutVATList = new BVATAccmArray(BVATF_SUMZEROVAT));
	if(oneof2(Filt.Grp, CCheckFilt::gAmount, CCheckFilt::gQtty) && Filt.AmountQuant <= 0.0)
		Filt.AmountQuant = (Filt.Grp == CCheckFilt::gQtty) ? 1.0 : 100.0;
	uint   i;
	PPObjSCardSeries * p_serobj = 0;
	{
		PPIDArray node_id_list;
		//
		// Идентифицируем терминальный список кассовых узлов
		//
		if(Filt.NodeList.GetCount()) {
			temp_list.clear();
			if(cn_obj.ResolveList(&Filt.NodeList.Get(), temp_list) > 0) {
				node_id_list = temp_list;
				NodeIdList.Set(&node_id_list);
			}
		}
		{
			//
			// Идентифицируем терминальный список кассовых сессий
			//
			temp_list.clear();
			if(Filt.Flags & CCheckFilt::fZeroSess)
				temp_list.add((long)0);
			else {
				for(i = 0; i < Filt.SessIDList.getCount(); i++) {
					const PPID sess_id = Filt.SessIDList.get (i);
					THROW(r = CsObj.P_Tbl->GetSubSessList(sess_id, &temp_list));
					if(r < 0)
						THROW(temp_list.add(sess_id));
				}
			}
			//
			// Из каждой кассовой сессии извлекаем ид кассового узла чтобы включить в общий список узлов
			//
			if(temp_list.getCount()) {
				SessIdList.Set(&temp_list);
				for(uint i = 0; i < temp_list.getCount(); i++) {
					const PPID sess_id = temp_list.get(i);
					CSessionTbl::Rec cs_rec;
					if(CsObj.Search(sess_id, &cs_rec) > 0) {
                        node_id_list.add(cs_rec.CashNodeID);
					}
				}
			}
		}
		// @v9.7.11 {
		if(node_id_list.getCount()) {
			//
			// Теперь, имея исчерпывающий список кассовых узлов можно
			// определить нужно показывать не отпечатанные чеки или нет.
			// Note: на самом деле, это не совсем корректно - чеки будут показаны по ПЕРЕСЕЧЕНИЮ
			//   списка узлов и списка сессий, а здесь решение мы принимаем по ОБЪЕДИНЕНИЮ.
			//   Пока оставим это противоречие (оно может сказаться в очень редких случаях, но
			//   не забываем, что здесь есть проблема).
			//
			node_id_list.sortAndUndup();
			State |= stSkipUnprinted;
			for(i = 0; State & stSkipUnprinted && i < node_id_list.getCount(); i++) {
				PPCashNode cn_rec;
				if(cn_obj.Fetch(node_id_list.get(i), &cn_rec) > 0 && !(cn_rec.Flags & CASHF_SKIPUNPRINTEDCHECKS))
					State &= ~stSkipUnprinted;
			}
		}
		// } @v9.7.11
	}
	if(Filt.SCardID)
		SCardList.Add(Filt.SCardID);
	else if(Filt.ScsList.GetCount() || Filt.SCardSerID) {
		PPIDArray scs_list;
		temp_list.clear();
		if(Filt.ScsList.GetCount()) {
			scs_list = Filt.ScsList.Get();
			scs_list.sortAndUndup();
		}
		else
			scs_list.add(Filt.SCardSerID);
		SCardCore & r_ct = P_CC->Cards;
		for(i = 0; i < scs_list.getCount(); i++) {
			const PPID scs_id = scs_list.get(i);
			SCardTbl::Key2 k2;
			BExtQuery q(&r_ct, 2);
			q.select(r_ct.ID, 0L).where(r_ct.SeriesID == scs_id);
			MEMSZERO(k2);
			k2.SeriesID = scs_id;
			for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
				temp_list.add(r_ct.data.ID);
			}
			temp_list.sortAndUndup();
			SCardList.Set(&temp_list);
		}
	}
	if(Filt.GoodsID) {
		GoodsList.Add((uint)Filt.GoodsID);
		State |= stUseGoodsList;
	}
	else if(Filt.GoodsGrpID) {
		temp_list.clear();
		GoodsIterator::GetListByGroup(Filt.GoodsGrpID, &temp_list);
		for(uint i = 0; i < temp_list.getCount(); i++)
			GoodsList.Add((uint)temp_list.get(i));
		State |= stUseGoodsList;
	}
	if(!Filt.CorrGoodsList.IsEmpty()) {
		const PPIDArray & r_goods_corr_list = Filt.CorrGoodsList.Get();
		for(uint i = 0; i < r_goods_corr_list.getCount(); i++)
			GoodsList.Add((uint)r_goods_corr_list.get(i));
		State |= stUseGoodsList;
	}
	if(Filt.Grp) {
		THROW(P_TmpGrpTbl = CreateTempGrpFile());
		{
			CCheckGrpCache ccache(20480, P_TmpGrpTbl);
			PPObjGoods::SubstBlock sgg_blk;
			sgg_blk.ExclParentID = Filt.GoodsGrpID;
			double total_amount = 0.0, total_qtty = 0.0;
			PPID   last_chk_id = 0;
			PPIDArray  gds_id_ary;
			CCheckViewItem item;
			PPViewCCheck   temp_view;
			CCheckFilt     temp_flt = Filt;
			temp_flt.Grp = CCheckFilt::gNone;
			temp_flt.Flags |= (CCheckFilt::fInitLinesCount | CCheckFilt::fInner);
			SETFLAG(temp_flt.Flags, CCheckFilt::fFillCashNodeID, BIN(Filt.Grp == CCheckFilt::gCashNode));
			if(Filt.Grp != CCheckFilt::gQtty)
				temp_flt.AmountQuant = 0.0;
			else if(!temp_flt.GoodsGrpID && !temp_flt.GoodsID && temp_flt.QttyR.IsZero())
				temp_flt.QttyR.Set(-1.0, 0.0);
			SETFLAG(temp_flt.Flags, CCheckFilt::fCheckLines, Filt.HasGoodsGrouping());
			//
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			THROW(temp_view.Init_(&temp_flt));
			for(temp_view.InitIteration(0); temp_view.NextIteration(&item) > 0;) {
				CCheckGrpItem ccgitem;
				MEMSZERO(ccgitem);
				ccgitem.Count = 1;
				ccgitem.Amount   = MONEYTOLDBL(item.Amount);
				ccgitem.Discount = MONEYTOLDBL(item.Discount);
				ccgitem.BnkAmt   = item.BnkAmt;
				ccgitem.CrdCardAmt = item.CrdCardAmt;
				ccgitem.Qtty     = item.G_Qtty;
				ccgitem.LinesCount = (long)item.G_LinesCount;
				ccgitem.SkuCount = item.G_SkuCount;
				total_amount += ccgitem.Amount;
				total_qtty   += ccgitem.Qtty;
				switch(Filt.Grp) {
					case CCheckFilt::gTime:       ccgitem.Tm = encodetime(item.Tm.hour(), 0, 0, 0); break;
					case CCheckFilt::gDate:       ccgitem.Dt = item.Dt; break;
					case CCheckFilt::gDayOfWeek:  ccgitem.Dt.setday((uint)dayofweek(&item.Dt, 1)); break;
					case CCheckFilt::gCash:       ccgitem.CashID = item.CashID; break;
					case CCheckFilt::gCashNode:   ccgitem.CashID = item.CashNodeID; break;
					case CCheckFilt::gCard:       ccgitem.SCardID = item.SCardID; break;
					case CCheckFilt::gGuestCount: ccgitem.CashID = item.GuestCount; break;
					case CCheckFilt::gTableNo:    ccgitem.CashID = item.TableCode; break;
					case CCheckFilt::gDiv:        ccgitem.CashID = item.Div; break;
					case CCheckFilt::gDlvrAddr:   ccgitem.CashID = item.AddrID; break;
					case CCheckFilt::gCashiers:   ccgitem.CashID = item.UserID; break;
					case CCheckFilt::gAgents:     ccgitem.CashID = item.AgentID; break;
					case CCheckFilt::gLinesCount: ccgitem.CashID = item.LinesCount; break;
					case CCheckFilt::gDowNTime:
						ccgitem.Dt.setday((uint)dayofweek(&item.Dt, 1));
						ccgitem.Tm = encodetime(item.Tm.hour(), 0, 0, 0);
						break;
					case CCheckFilt::gDscntPct:
						ccgitem.CashID = (long)ceil(fabs(fdivnz(ccgitem.Discount, ccgitem.Amount+ccgitem.Discount)) * 400); // @pctdis
						break;
					case CCheckFilt::gAmount:
						if(Filt.AmountQuant > 0.0)
							ccgitem.CashID = (long)fabs(ccgitem.Amount / Filt.AmountQuant);
						break;
					case CCheckFilt::gQtty:
						if(Filt.AmountQuant > 0.0)
							ccgitem.CashID = (long)fabs(ccgitem.Qtty / Filt.AmountQuant);
						break;
					case CCheckFilt::gGoods:
					case CCheckFilt::gGoodsDate:
					case CCheckFilt::gAgentsNGoods:
					case CCheckFilt::gCashiersNGoods:
					case CCheckFilt::gGoodsSCSer:
					case CCheckFilt::gAmountNGoods:
					case CCheckFilt::gAgentGoodsSCSer: // @v9.6.6
						if(!(Filt.Flags & CCheckFilt::fGoodsCorr))
							THROW(GdsObj.SubstGoods(item.G_GoodsID, &ccgitem.GoodsID, Filt.Sgg, &sgg_blk, &Gsl));
						if(last_chk_id != item.ID) {
							last_chk_id = item.ID;
							gds_id_ary.clear();
						}
						if(gds_id_ary.addUnique(ccgitem.GoodsID) < 0)
							ccgitem.Count = 0;
						if(Filt.Grp == CCheckFilt::gGoodsDate)
							ccgitem.Dt = item.Dt;
						else if(Filt.Grp == CCheckFilt::gAgentsNGoods)
							ccgitem.CashID = item.AgentID;
						// @v9.6.6 {
						else if(Filt.Grp == CCheckFilt::gAgentGoodsSCSer) {
							ccgitem.CashID = item.AgentID;
							SCardTbl::Rec sc_rec;
							if(item.SCardID && ScObj.Fetch(item.SCardID, &sc_rec) > 0)
								ccgitem.SCardID = sc_rec.SeriesID;
							else
								ccgitem.SCardID = 0;
						}
						// } @v9.6.6
						else if(Filt.Grp == CCheckFilt::gCashiersNGoods)
							ccgitem.CashID = item.UserID;
						else if(Filt.Grp == CCheckFilt::gGoodsSCSer) {
							SCardTbl::Rec sc_rec;
							if(item.SCardID && ScObj.Fetch(item.SCardID, &sc_rec) > 0)
								ccgitem.CashID = sc_rec.SeriesID;
							else
								ccgitem.CashID = 0;
						}
						else if(Filt.Grp == CCheckFilt::gAmountNGoods) {
							if(Filt.AmountQuant > 0.0)
								ccgitem.CashID = (long)fabs(ccgitem.Amount / Filt.AmountQuant);
						}
						break;
					case CCheckFilt::gAgentsNHour:
						ccgitem.CashID = item.AgentID;
						ccgitem.Tm = encodetime(item.Tm.hour(), 0, 0, 0);
						break;
				}
				THROW(ccache.AddItem(&ccgitem));
				PPWaitPercent(temp_view.GetCounter());
			}
			THROW(ccache.Flash());
			if(total_amount != 0.0 || total_qtty != 0.0) {
				TempCCheckGrpTbl::Key0 k0;
				MEMSZERO(k0);
				if(P_TmpGrpTbl->searchForUpdate(0, &k0, spFirst)) do {
					TempCCheckGrpTbl::Rec rec;
					P_TmpGrpTbl->copyBufTo(&rec);
					temp_buf.Z();
					switch(Filt.Grp) {
						case CCheckFilt::gTime:
							temp_buf.CatLongZ(rec.Tm.hour(), 2).CatCharN('.', 2).CatLongZ(rec.Tm.hour()+1, 2);
							break;
						case CCheckFilt::gDate:
							temp_buf.Cat(rec.Dt);
							break;
						case CCheckFilt::gDayOfWeek:
							GetDayOfWeekText(dowtRuFull, rec.Dt.day(), temp_buf);
							temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
							break;
						case CCheckFilt::gDowNTime:
							GetDayOfWeekText(dowtRuFull, rec.Dt.day(), temp_buf);
							temp_buf.Transf(CTRANSF_OUTER_TO_INNER).CatDiv('-', 1).CatLongZ(rec.Tm.hour(), 2).CatCharN('.', 2).CatLongZ(rec.Tm.hour()+1, 2);
							break;
						case CCheckFilt::gCash:
							temp_buf.Cat(rec.CashID);
		   	            	break;
						case CCheckFilt::gCashNode:
							{
								PPCashNode cn_rec;
								if(cn_obj.Fetch(rec.CashID, &cn_rec) > 0)
									temp_buf = cn_rec.Name;
							}
		   	            	break;
						case CCheckFilt::gCard:
							{
								SCardTbl::Rec crd_rec;
								if(ScObj.Fetch(rec.SCardID, &crd_rec) > 0) {
									PPSCardSeries ser_rec;
									if(!p_serobj)
										THROW_MEM(p_serobj = new PPObjSCardSeries);
									if(p_serobj->Fetch(crd_rec.SeriesID, &ser_rec) > 0)
										temp_buf.Cat(ser_rec.Name).CatChar(':');
									temp_buf.Cat(crd_rec.Code);
									if(crd_rec.PersonID) {
										GetPersonName(crd_rec.PersonID, name_buf);
										temp_buf.CatChar(':').Cat(name_buf);
									}
								}
							}
							break;
						case CCheckFilt::gDscntPct:
							if(rec.CashID) {
								temp_buf.Cat((rec.CashID - 1) * 0.25 , SFMT_MONEY).CatCharN('.', 2);
								temp_buf.Cat(rec.CashID * 0.25 , SFMT_MONEY);
							}
							else
								temp_buf.Cat(0L);
							break;
						case CCheckFilt::gAmount:
						case CCheckFilt::gQtty:
							{
								int    prec = (Filt.Grp == CCheckFilt::gQtty) ? 3 : 2;
								double low = round(rec.CashID * Filt.AmountQuant, prec);
								double upp = low + Filt.AmountQuant - fpow10i(-prec);
								temp_buf.Cat(low, MKSFMTD(8, prec, ALIGN_RIGHT | NMBF_NOTRAILZ));
								temp_buf.CatCharN('.', 2);
								temp_buf.Cat(upp, MKSFMTD(0, prec, NMBF_NOTRAILZ));
							}
							break;
						case CCheckFilt::gGoods:
						case CCheckFilt::gGoodsDate:
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, temp_buf));
							}
							else {
								// @v9.5.5 GetGoodsName(rec.GoodsID, temp_buf);
								GdsObj.FetchNameR(rec.GoodsID, temp_buf); // @v9.5.5
							}
							break;
						case CCheckFilt::gGuestCount: temp_buf.CatLongZ(rec.CashID, 3); break;
						case CCheckFilt::gTableNo: temp_buf.CatLongZ(rec.CashID, 3); break;
						case CCheckFilt::gDiv: temp_buf.CatLongZ(rec.CashID, 3); break;
						case CCheckFilt::gLinesCount: temp_buf.CatLongZ(rec.CashID, 4); break;
						case CCheckFilt::gCashiers:
							if(rec.CashID)
								GetPersonName(rec.CashID, temp_buf);
		   	            	break;
						case CCheckFilt::gAgents:
							if(rec.CashID)
								GetArticleName(rec.CashID, temp_buf);
		   	            	break;
						case CCheckFilt::gDlvrAddr:
							if(rec.CashID) {
								LocationTbl::Rec loc_rec;
								if(loc_obj.Search(rec.CashID, &loc_rec) > 0) {
									LocationCore::GetExField(&loc_rec, LOCEXSTR_PHONE, name_buf);
									temp_buf.Cat(name_buf);
									LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, name_buf);
									if(name_buf.NotEmptyS())
										temp_buf.CatDivIfNotEmpty(',', 2).Cat(name_buf);
									LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, name_buf);
									if(name_buf.NotEmptyS())
										temp_buf.CatDivIfNotEmpty(',', 2).Cat(name_buf);
								}
							}
							break;
						case CCheckFilt::gAgentsNHour:
							if(rec.CashID)
								GetArticleName(rec.CashID, temp_buf);
							else
								PPGetWord(PPWORD_AGENTNOTDEF, 0, temp_buf);
							temp_buf.CatDiv('-', 1).CatLongZ(rec.Tm.hour(), 2).CatCharN('.', 2).CatLongZ(rec.Tm.hour()+1, 2);
							break;
						case CCheckFilt::gAgentsNGoods:
							if(rec.CashID)
								GetArticleName(rec.CashID, temp_buf);
							else
								PPGetWord(PPWORD_AGENTNOTDEF, 0, temp_buf);
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, goods_name));
							}
							else {
								// @v9.5.5 GetGoodsName(rec.GoodsID, goods_name);
								GdsObj.FetchNameR(rec.GoodsID, goods_name); // @v9.5.5
							}
							temp_buf.CatDiv('-', 1).Cat(goods_name);
							break;
						case CCheckFilt::gAgentGoodsSCSer: // @v9.6.6
							if(rec.CashID)
								GetArticleName(rec.CashID, temp_buf);
							else
								PPGetWord(PPWORD_AGENTNOTDEF, 0, temp_buf);
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, goods_name));
							}
							else
								GdsObj.FetchNameR(rec.GoodsID, goods_name);
							temp_buf.CatDiv('-', 1).Cat(goods_name);
							if(rec.SCardID) {
								PPSCardSeries sc_rec;
								if(sc_obj.Fetch(rec.SCardID, &sc_rec) > 0)
									temp_buf.CatDivIfNotEmpty('-', 1).Cat(sc_rec.Name);
							}
							break;
						case CCheckFilt::gCashiersNGoods:
							if(rec.CashID)
								GetPersonName(rec.CashID, temp_buf);
							else
								temp_buf.Z();
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, goods_name));
							}
							else {
								// @v9.5.5 GetGoodsName(rec.GoodsID, goods_name);
								GdsObj.FetchNameR(rec.GoodsID, goods_name); // @v9.5.5
							}
							temp_buf.CatDivIfNotEmpty('-', 1).Cat(goods_name);
							break;
						case CCheckFilt::gGoodsSCSer:
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, goods_name));
							}
							else {
								// @v9.5.5 GetGoodsName(rec.GoodsID, goods_name);
								GdsObj.FetchNameR(rec.GoodsID, goods_name); // @v9.5.5
							}
							temp_buf = goods_name;
							if(rec.CashID) {
								PPSCardSeries sc_rec;
								if(sc_obj.Fetch(rec.CashID, &sc_rec) > 0)
									temp_buf.CatDivIfNotEmpty('-', 1).Cat(sc_rec.Name);
							}
							break;
						case CCheckFilt::gAmountNGoods:
							{
								int    prec = 2;
								double low = round(rec.CashID * Filt.AmountQuant, prec);
								double upp = low + Filt.AmountQuant - fpow10i(-prec);
								temp_buf.Cat(low, MKSFMTD(8, prec, ALIGN_RIGHT | NMBF_NOTRAILZ));
								temp_buf.CatCharN('.', 2);
								temp_buf.Cat(upp, MKSFMTD(0, prec, NMBF_NOTRAILZ));
							}
							if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
								THROW(GdsObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, goods_name));
							}
							else {
								// @v9.5.5 GetGoodsName(rec.GoodsID, goods_name);
								GdsObj.FetchNameR(rec.GoodsID, goods_name); // @v9.5.5
							}
							temp_buf.CatDivIfNotEmpty('-', 1).Cat(goods_name);
							break;
					}
					temp_buf.CopyTo(rec.Text, sizeof(rec.Text));
					if(Filt.Grp == CCheckFilt::gQtty)
						rec.PctPart = fdivnz(100.0 * rec.Qtty, total_qtty);
					else
						rec.PctPart = fdivnz(100.0 * rec.Amount, total_amount);
					THROW_DB(P_TmpGrpTbl->updateRecBuf(&rec)); // @sfu
				} while(P_TmpGrpTbl->searchForUpdate(0, &k0, spNext));
			}
			THROW(tra.Commit());
		}
	}
	else {
		if(Filt.Flags & CCheckFilt::fSuspendedOnly && Filt.Period.low) {
			temp_list.clear();
			CCheckTbl::Key1 k1;
			MEMSZERO(k1);
			k1.Dt = Filt.Period.low;
			DBQ  * dbq = 0;
			BExtQuery q(p_cct, 1);
			dbq = & (*dbq && daterange(p_cct->Dt, &Filt.Period));
			dbq = ppcheckfiltid(dbq, p_cct->CashID, Filt.CashNumber);
			dbq = ppcheckfiltid(dbq, p_cct->UserID, Filt.CashierID);
			dbq = &(*dbq && intrange(p_cct->Code, Filt.CodeR));
			q.select(p_cct->ID, p_cct->Flags, 0L).where(*dbq);
			for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
				if(p_cct->data.Flags & CCHKF_SUSPENDED)
					temp_list.add(p_cct->data.ID);
			}
			temp_list.sortAndUndup();
			CcIdList.Set(&temp_list);
		}
		else
			P_CC->GetListByExtFilt(Filt, CcIdList);
		if(!(Filt.Flags & CCheckFilt::fCheckLines)){
			if(P_TmpTbl && (!IsTempTblNeeded() || !(Filt.Flags & CCheckFilt::fInner)))
				ZDELETE(P_TmpTbl);
			if(!P_TmpTbl && IsTempTblNeeded()) { // @todo Из-за того что при вызове ChangeFilt не удаляется таблица P_TmpTbl, возможны артефакты
				THROW(P_TmpTbl = CreateTempFile());
				{
					BExtInsert bei(P_TmpTbl);
					PPTransaction tra(ppDbDependTransaction, 1);
					THROW(tra);
					if(SessIdList.IsExists()) {
						for(uint i = 0; i < SessIdList.GetCount(); i++) {
							CCheckTbl::Key3 k;
							PPID   sess_id = SessIdList.Get().get(i);
							DBQ  * dbq = 0;
							BExtQuery q(p_cct, 3);
							q.selectAll();
							dbq = & (p_cct->SessID == sess_id);
							dbq = ppcheckfiltid(dbq, p_cct->CashID, Filt.CashNumber);
							dbq = ppcheckfiltid(dbq, p_cct->UserID, Filt.CashierID);
							dbq = ppcheckfiltid(dbq, p_cct->SCardID, Filt.SCardID);
							dbq = &(*dbq && intrange(p_cct->Code, Filt.CodeR));
							q.where(*dbq);
							MEMSZERO(k);
							k.SessID = sess_id;
							for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
								THROW(ProcessCheckRec(&p_cct->data, &bei));
							}
						}
					}
					else if(SCardList.IsExists()) {
						uint i;
						PPIDArray cc_id_list;
						for(i = 0; i < SCardList.GetCount(); i++) {
							const PPID card_id = SCardList.Get().get(i);
							{
								CCheckTbl::Key4 k;
								DBQ  * dbq = 0;
								BExtQuery q(p_cct, 4);
								dbq = & (p_cct->SCardID == card_id);
								dbq = & (*dbq && daterange(p_cct->Dt, &Filt.Period));
								dbq = ppcheckfiltid(dbq, p_cct->CashID, Filt.CashNumber);
								dbq = ppcheckfiltid(dbq, p_cct->UserID, Filt.CashierID);
								dbq = &(*dbq && intrange(p_cct->Code, Filt.CodeR));
								q.select(p_cct->ID, 0L).where(*dbq);
								MEMSZERO(k);
								k.SCardID = card_id;
								k.Dt = Filt.Period.low;
								for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
									cc_id_list.add(p_cct->data.ID);
								}
							}
							{
								CCheckPaymTbl * p_ccp = &p_cct->PaymT;
								CCheckPaymTbl::Key1 pk1;
								MEMSZERO(pk1);
								pk1.SCardID = card_id;
								BExtQuery q(p_ccp, 1);
								q.select(p_ccp->CheckID, 0L).where(p_ccp->SCardID == card_id);
								for(q.initIteration(0, &pk1, spGe); q.nextIteration() > 0;) {
									cc_id_list.add(p_ccp->data.CheckID);
								}
							}
						}
						cc_id_list.sortAndUndup();
						for(i = 0; i < cc_id_list.getCount(); i++) {
							CCheckTbl::Rec cc_rec;
							if(p_cct->Search(cc_id_list.get(i), &cc_rec) > 0) {
								THROW(ProcessCheckRec(&cc_rec, &bei));
							}
						}
					}
					else if(CcIdList.IsExists()) {
						for(uint i = 0; i < CcIdList.GetCount(); i++) {
							const PPID cc_id = CcIdList.Get().get(i);
							if(p_cct->Search(cc_id, 0) > 0) {
								THROW(ProcessCheckRec(&p_cct->data, &bei));
							}
						}
					}
					else {
						IterCounter cntr;
						CCheckTbl::Key1 k1, k1_;
						DBQ  * dbq = 0;
						BExtQuery q(p_cct, 1);
						dbq = & (*dbq && daterange(p_cct->Dt, &Filt.Period));
						dbq = ppcheckfiltid(dbq, p_cct->CashID, Filt.CashNumber);
						dbq = ppcheckfiltid(dbq, p_cct->UserID, Filt.CashierID);
						dbq = &(*dbq && intrange(p_cct->Code, Filt.CodeR));
						q.selectAll().where(*dbq);
						MEMSZERO(k1);
						k1.Dt     = Filt.Period.low;
						k1.CashID = Filt.CashNumber;
						k1_ = k1;
						cntr.Init(q.countIterations(0, &k1_, spGe));
						for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
							THROW(ProcessCheckRec(&p_cct->data, &bei));
							PPWaitPercent(cntr.Increment());
						}
					}
					THROW_DB(bei.flash());
					THROW(tra.Commit());
				}
			}
		}
	}
	ZDELETE(P_Ct);
	if(Filt.Flags & CCheckFilt::fGoodsCorr) {
		Gsl.Init(1, 0);
		THROW(CreateGoodsCorrTbl());
		ImplementFlags |= implChangeFilt;
	}
	else if(P_TmpGrpTbl && Filt.CtKind != TrfrAnlzFilt::ctNone) {
		int    setup_total = 0;
		DBFieldList total_list;
		StringSet total_title_list;

		THROW_MEM(P_Ct = new CCheckCrosstab(this));
		if(Filt.CtKind == CCheckFilt::ctDate)
			P_Ct->SetTable(P_TmpGrpTbl, P_TmpGrpTbl->Dt);
		P_Ct->AddIdxField(P_TmpGrpTbl->GoodsID);
		P_Ct->AddInheritedFixField(P_TmpGrpTbl->Text);
		if(Filt.CtValList.CheckID(CCheckFilt::ctvChecksSum) > 0) {
			P_Ct->AddAggrField(P_TmpGrpTbl->Amount);
			total_list.Add(P_TmpGrpTbl->Amount);
			total_title_list.add(GetCtColumnTitle(CCheckFilt::ctvChecksSum, temp_buf));
		}
		if(Filt.CtValList.CheckID(CCheckFilt::ctvChecksCount) > 0) {
			P_Ct->AddAggrField(P_TmpGrpTbl->Count);
			total_list.Add(P_TmpGrpTbl->Count);
			total_title_list.add(GetCtColumnTitle(CCheckFilt::ctvChecksCount, temp_buf));
		}
		if(Filt.CtValList.CheckID(CCheckFilt::ctvSKUCount) > 0) {
			P_Ct->AddAggrField(P_TmpGrpTbl->SkuCount);
			total_list.Add(P_TmpGrpTbl->SkuCount);
			total_title_list.add(GetCtColumnTitle(CCheckFilt::ctvSKUCount, temp_buf));
		}
		P_Ct->SetSortIdx("Text", 0L);
		if(total_list.GetCount()) {
			PPGetWord(PPWORD_TOTAL, 0, temp_buf);
			P_Ct->AddTotalRow(total_list, 0, temp_buf);
			uint ss_pos = 0;
			for(uint i = 0; i < total_list.GetCount(); i++) {
				total_title_list.get(&ss_pos, temp_buf.Z());
				P_Ct->AddTotalColumn(total_list.Get(i), 0, temp_buf);
			}
		}
		THROW(P_Ct->Create(1));
	}
	DefReportId = GetReportId();
	CATCH
		ZDELETE(P_TmpTbl);
		ZDELETE(P_TmpGrpTbl);
		ZDELETE(P_TmpGdsCorrTbl);
		ok = 0;
	ENDCATCH
	delete p_serobj;
	return ok;
}

int SLAPI PPViewCCheck::InitIteration(int order)
{
	BExtQuery::ZDelete(&P_IterQuery);

	SETFLAG(State, stIterLines, (order & CCheckFilt::ordIterLines) && !(Filt.Flags & CCheckFilt::fCheckLines));
	order &= ~CCheckFilt::ordIterLines;
	SETIFZ(order, (Filt.SortOrder & ~CCheckFilt::ordIterLines));
	int    ok = 1, idx = 0, sp = spGe;
	RECORDNUMBER num_recs = 0;
	DBQ  * dbq = 0;
	BExtQuery * p_q = 0;
	if(P_TmpGdsCorrTbl) {
		union {
			TempCCheckGdsCorrTbl::Key3 k3;
			TempCCheckGdsCorrTbl::Key4 k4;
		} k;
		PPInitIterCounter(Counter, P_TmpGdsCorrTbl);
		if(order == CCheckFilt::ordByName)
			idx = 3;
		else
			idx = 4;
		THROW_MEM(p_q = new BExtQuery(P_TmpGdsCorrTbl, idx));
		p_q->selectAll();
		MEMSZERO(k);
		p_q->initIteration(0, &k, spFirst);
		P_IterQuery = p_q;
	}
	else if(P_TmpGrpTbl) {
		union {
			TempCCheckGrpTbl::Key1 k1;
			TempCCheckGrpTbl::Key2 k2;
			TempCCheckGrpTbl::Key3 k3;
			TempCCheckGrpTbl::Key4 k4;
			TempCCheckGrpTbl::Key5 k5;
		} k;
		PPInitIterCounter(Counter, P_TmpGrpTbl);
		switch(order) {
			case CCheckFilt::ordByCount: idx = 3; break;
			case CCheckFilt::ordByQtty:  idx = 4; break;
			case CCheckFilt::ordByAmt:   idx = 5; break;
			default:
				if(oneof8(Filt.Grp, CCheckFilt::gDate, CCheckFilt::gDayOfWeek, CCheckFilt::gDowNTime,
					CCheckFilt::gCash, CCheckFilt::gCashNode, CCheckFilt::gDscntPct, CCheckFilt::gAmount, CCheckFilt::gQtty))
					idx = 1;
				else
					idx = 2;
		}
		THROW_MEM(p_q = new BExtQuery(P_TmpGrpTbl, idx));
		p_q->selectAll();
		MEMSZERO(k);
		p_q->initIteration(0, &k, spFirst);
		P_IterQuery = p_q;
	}
	else {
		union {
			CCheckTbl::Key1 k1;
			CCheckTbl::Key3 k3;
		} k, k_;
		if(P_TmpTbl) {
			PPInitIterCounter(Counter, P_TmpTbl);
			THROW_MEM(p_q = new BExtQuery(P_TmpTbl, 1, 16));
			idx = 1;
			sp = spFirst;
			MEMSZERO(k);
			p_q->selectAll();
		}
		else {
			CCheckCore * p_cct = P_CC;
			if(SessIdList.GetCount() == 1) { // Здесь нельзя использовать GetSingle ибо единственный элементы может быть 0
				idx = 3;
				sp = spGe;
				MEMSZERO(k);
				k.k3.SessID = SessIdList.GetSingle();
				k.k3.CashID = Filt.CashNumber;
				dbq = &(*dbq && p_cct->SessID == SessIdList.GetSingle());
			}
			else {
				idx = 1;
				sp = spGe;
				MEMSZERO(k);
				k.k1.Dt = Filt.Period.low;
			}
			dbq = ppcheckfiltid(dbq, p_cct->CashID, Filt.CashNumber);
			dbq = ppcheckfiltid(dbq, p_cct->UserID, Filt.CashierID);
			dbq = & (*dbq && daterange(p_cct->Dt, &Filt.Period));
			dbq = ppcheckfiltid(dbq, p_cct->SCardID, Filt.SCardID);
			THROW_MEM(p_q = new BExtQuery(p_cct, idx));
			p_q->selectAll().where(*dbq);
			if(!(Filt.Flags & CCheckFilt::fDontCount))
				Counter.Init(p_q->countIterations(0, &(k_ = k), sp));
		}
		p_q->initIteration(0, &k, sp);
		P_IterQuery = p_q;
	}
	CATCH
		ok = 0;
		BExtQuery::ZDelete(&p_q);
	ENDCATCH
	return ok;
}

int FASTCALL PPViewCCheck::NextIteration(CCheckViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		while(ok < 0 && (CurLine != 0 || P_IterQuery && P_IterQuery->nextIteration() > 0)) {
			PPID   cc_id = 0;
			memzero(pItem, sizeof(CCheckViewItem));
			if(P_TmpGdsCorrTbl) {
				TempCCheckGdsCorrTbl::Rec rec;
				P_TmpGdsCorrTbl->copyBufTo(&rec);
				pItem->ID = rec.ID__;
				pItem->G_GoodsID  = rec.Goods1ID;
				pItem->G_Goods2ID = rec.Goods2ID;
				pItem->G_Count    = rec.Count;
				ok = 1;
			}
			else if(Filt.Grp) {
				TempCCheckGrpTbl::Rec rec;
				P_TmpGrpTbl->copyBufTo(&rec);
				pItem->ID = rec.ID__;
				pItem->Dt = rec.Dt;
				pItem->Tm = rec.Tm;
				pItem->CashID  = rec.CashID;
				pItem->SCardID = rec.SCardID;
				pItem->G_GoodsID = rec.GoodsID;
				LDBLTOMONEY(rec.Amount,   pItem->Amount);
				LDBLTOMONEY(rec.Discount, pItem->Discount);
				pItem->BnkAmt = rec.BnkAmt;
				pItem->CrdCardAmt = rec.CrdCardAmt;
				STRNSCPY(pItem->G_Text, rec.Text);
				pItem->G_Count    = rec.Count;
				pItem->G_LinesCount = rec.LinesCount;
				pItem->G_SkuCount = rec.SkuCount;
				pItem->G_Amount   = rec.Amount;
				pItem->G_Discount = rec.Discount;
				pItem->G_PctPart  = rec.PctPart;
				pItem->G_Qtty     = rec.Qtty;
				// @v9.3.0 {
				if(Filt.Grp == CCheckFilt::gAgents)
					pItem->AgentID = rec.CashID;
				else if(Filt.Grp == CCheckFilt::gAgentsNGoods)
					pItem->AgentID = rec.CashID;
				else if(Filt.Grp == CCheckFilt::gAgentsNHour)
					pItem->AgentID = rec.CashID;
				// } @v9.3.0
				// @v9.6.6 {
				else if(Filt.Grp == CCheckFilt::gAgentGoodsSCSer) {
					pItem->AgentID = rec.CashID;
				}
				// } @v9.6.6
				ok = 1;
			}
			else if(P_TmpTbl) {
				TempCCheckQttyRec_To_CCheckRec(P_TmpTbl->data, *pItem);
				cc_id = pItem->ID;
				//*((CCheckTbl::Rec*)pItem) = *(CCheckTbl::Rec *)&P_TmpTbl->data;
				pItem->G_LinesCount = P_TmpTbl->data.LinesCount;
				pItem->G_SkuCount = P_TmpTbl->data.SkuCount;
				pItem->G_Qtty = P_TmpTbl->data.Qtty;
				pItem->BnkAmt = P_TmpTbl->data.BnkAmt;
				pItem->CrdCardAmt = P_TmpTbl->data.CrdCardAmt;
				if(pItem->Flags & CCHKF_EXT) {
					CCheckExtTbl::Rec ext_rec;
					if(P_CC->GetExt(pItem->ID, &ext_rec) > 0) {
						pItem->TableCode = ext_rec.TableNo;
						pItem->GuestCount = ext_rec.GuestCount;
						pItem->AgentID   = ext_rec.SalerID;
						pItem->LinkCheckID = ext_rec.LinkCheckID;
						pItem->CreationDtm = ext_rec.CreationDtm;
						if(pItem->Flags & CCHKF_ORDER)
							pItem->OrderTime.Init(ext_rec.StartOrdDtm, ext_rec.EndOrdDtm);
					}
				}
				if(!pItem->CashNodeID && Filt.Flags & CCheckFilt::fFillCashNodeID) {
					CSessionTbl::Rec cs_rec;
					if(CsObj.Fetch(pItem->SessID, &cs_rec) > 0)
						pItem->CashNodeID = cs_rec.CashNodeID;
				}
				ok = 1;
			}
			else {
				CCheckTbl::Rec _rec;
				CCheckExtTbl::Rec _ext_rec;
				PreprocessCheckRec(&P_CC->data, _rec, _ext_rec);
				const long _cc_flags = _rec.Flags;
				const double _cc_amt = MONEYTOLDBL(_rec.Amount);
				if(CheckForFilt(&_rec, &_ext_rec)) {
					CCheckCore * p_cct = P_CC;
					*((CCheckTbl::Rec*)pItem) = _rec;
					cc_id = pItem->ID;
					if(Filt.Flags & CCheckFilt::fCheckLines) {
						int    suitable = 1;
						double qtty = 0.0, amt = 0.0, dscnt = 0.0, t_dscnt = 0.0, pcnt = 0.0;
						CCheckLineTbl::Rec ln_rec;
						if(CurLine == 0) {
							CcPack.ClearLines();
							p_cct->LoadLines(p_cct->data.ID, 0 /*Filt.GoodsID*/, &CcPack);
							if((State & stUseGoodsList) || !Filt.QttyR.IsZero()) {
								suitable = 0;
								for(uint i = 0; CcPack.EnumLines(&i, &ln_rec);) {
									if(!(State & stUseGoodsList) || GoodsList.Has((uint)ln_rec.GoodsID))
										suitable = 1;
									if(suitable || (Filt.Flags & CCheckFilt::fFiltByCheck)) {
										qtty  += ln_rec.Quantity;
										amt   += ln_rec.Quantity * intmnytodbl(ln_rec.Price);
										dscnt += ln_rec.Quantity * ln_rec.Dscnt;
									}
									t_dscnt += ln_rec.Quantity * ln_rec.Dscnt;
								}
								if(suitable && Filt.QttyR.CheckVal(fabs(qtty))) {
									if(State & stUseGoodsList) {
										double  chk_dscnt = MONEYTOLDBL(p_cct->data.Discount) - t_dscnt;
										if(chk_dscnt != 0.0)
											dscnt += fdivnz(chk_dscnt * amt, MONEYTOLDBL(p_cct->data.Amount) + MONEYTOLDBL(p_cct->data.Discount));
										LDBLTOMONEY(amt - dscnt, p_cct->data.Amount);
										LDBLTOMONEY(dscnt, p_cct->data.Discount);
									}
									if(!(Filt.Flags & CCheckFilt::fFiltByCheck)) {
										pcnt = fabs(fdivnz(dscnt, amt+dscnt)) * 100; // @pctdis
										suitable = BIN(Filt.AmtR.CheckVal(fabs(amt - dscnt)) && Filt.PcntR.CheckVal(pcnt));
									}
								}
							}
						}
						if(suitable) {
							while(CcPack.EnumLines(&CurLine, &ln_rec)) {
								if(!(State & stUseGoodsList) || GoodsList.Has((uint)ln_rec.GoodsID)) {
									const double price_ = intmnytodbl(ln_rec.Price);
									amt   = price_ * ln_rec.Quantity;
									dscnt = ln_rec.Dscnt * ln_rec.Quantity;
									amt  -= dscnt;
									LDBLTOMONEY(amt,   pItem->Amount);
									LDBLTOMONEY(dscnt, pItem->Discount);
									pItem->Div       = ln_rec.DivID;
									pItem->G_Qtty    = ln_rec.Quantity;
									pItem->G_Price   = price_;
									pItem->G_GoodsID = ln_rec.GoodsID;
									pItem->LinesCount = CcPack.GetCount();
									ok = 1;
									break;
								}
							}
						}
						if(CurLine >= CcPack.GetCount() || !suitable)
							CurLine = 0;
					}
					else {
						if(Filt.Flags & CCheckFilt::fInitLinesCount) {
							CCheckPacket cc_pack;
							P_CC->LoadLines(P_CC->data.ID, 0, &cc_pack);
							pItem->LinesCount = cc_pack.GetCount();
						}
						ok = 1;
					}
					if(ok > 0) {
						if(pItem->Flags & CCHKF_EXT) {
							pItem->TableCode  = _ext_rec.TableNo;
							pItem->GuestCount = _ext_rec.GuestCount;
							pItem->AgentID    = _ext_rec.SalerID;
							pItem->LinkCheckID = _ext_rec.LinkCheckID;
							pItem->AddrID      = _ext_rec.AddrID;
							pItem->CreationDtm = _ext_rec.CreationDtm;
							if(pItem->Flags & CCHKF_ORDER)
								pItem->OrderTime.Init(_ext_rec.StartOrdDtm, _ext_rec.EndOrdDtm);
						}
						if(Filt.Flags & CCheckFilt::fFillCashNodeID) {
							CSessionTbl::Rec cs_rec;
							if(CsObj.Fetch(pItem->SessID, &cs_rec) > 0)
								pItem->CashNodeID = cs_rec.CashNodeID;
						}
						{
							int    pmr = 0;
							CcAmountList _pl;
							if(_cc_flags & CCHKF_PAYMLIST) {
								pmr = P_CC->GetPaymList(pItem->ID, _pl);
							}
							if(pmr > 0) {
								pItem->BnkAmt = _pl.Get(CCAMTTYP_BANK);
								pItem->CrdCardAmt = _pl.Get(CCAMTTYP_CRDCARD);
							}
							else {
								if(_cc_flags & CCHKF_INCORPCRD) {
									pItem->CrdCardAmt = _cc_amt;
								}
								else if(_cc_flags & CCHKF_BANKING) {
									pItem->BnkAmt = _cc_amt;
								}
							}
							if(Filt.Flags & CCheckFilt::fCheckLines) {
								const double item_amt = MONEYTOLDBL(pItem->Amount);
								if(_cc_amt != 0.0) {
									if(pItem->BnkAmt != 0.0)
										pItem->BnkAmt = (pItem->BnkAmt * item_amt) / _cc_amt;
									if(pItem->CrdCardAmt != 0.0)
										pItem->CrdCardAmt = (pItem->CrdCardAmt * item_amt) / _cc_amt;
								}
							}
						}
					}
				}
			}
			if(cc_id && (State & stIterLines) && !(Filt.Flags & CCheckFilt::fCheckLines)) {
				double qtty = 0.0, amt = 0.0, dscnt = 0.0, t_dscnt = 0.0, pcnt = 0.0;
				CCheckLineTbl::Rec ln_rec;
				if(CurLine == 0) {
					CcPack.ClearLines();
					P_CC->LoadLines(cc_id, 0 /*Filt.GoodsID*/, &CcPack);
				}
				if(CcPack.EnumLines(&CurLine, &ln_rec)) {
					CCheckPacket::LineExt le;
					SString temp_buf;
					if(CcPack.GetLineExt(CurLine, le) > 0) {
						pItem->LineFlags = le.Flags;
						pItem->LineQueue = le.Queue;
					}
					if(CcPack.GetLineTextExt(CurLine, CCheckPacket::lnextSerial, temp_buf) > 0) {
						temp_buf.CopyTo(pItem->G_Text, sizeof(pItem->G_Text));
					}
					const double price_ = intmnytodbl(ln_rec.Price);
					amt   = price_ * ln_rec.Quantity;
					dscnt = ln_rec.Dscnt * ln_rec.Quantity;
					amt  -= dscnt;
					pItem->RByCheck   = ln_rec.RByCheck;
					pItem->G_GoodsID  = ln_rec.GoodsID;
					pItem->Div        = ln_rec.DivID;
					pItem->G_Qtty     = ln_rec.Quantity;
					pItem->G_Price    = price_;
					pItem->G_Discount = ln_rec.Dscnt;
					pItem->G_Amount   = amt;
					pItem->LinesCount = CcPack.GetCount();
					pItem->G_Count    = 1;
					ok = 1;
				}
				if(CurLine >= CcPack.GetCount())
					CurLine = 0;
			}
			if(CurLine == 0)
				Counter.Increment();
		}
	}
	return ok;
}

int PPViewCCheck::DynFuncPosText = 0;

static IMPL_DBE_PROC(dbqf_ccheck_postext_ii) // @construction
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		buf[0] = 0;
		PPID   sess_id = params[0].lval;
		PPID   pos_id = params[1].lval;
		SString temp_buf;
		PPObjCSession cs_obj;
		CSessionTbl::Rec cs_rec;
		if(cs_obj.Fetch(params[0].lval, &cs_rec) > 0) {
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(cn_obj.Fetch(cs_rec.CashNodeID, &cn_rec) > 0) {
				temp_buf = cn_rec.Name;
			}
		}
		temp_buf.CatDivIfNotEmpty('-', 1).Cat(pos_id);
		STRNSCPY(buf, temp_buf);
		result->init(buf);
	}
}

DBQuery * SLAPI PPViewCCheck::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncPosText, BTS_STRING, dbqf_ccheck_postext_ii, 2, BTS_INT, BTS_INT);

	DBQuery * p_q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_psn;
	DBE    dbe_saler;
	DBE    dbe_pctdis;
	DBE    dbe_addr_city;
	DBE    dbe_addr_phone;
	DBE    dbe_sc_code;
	DBE    dbe_scowner_name;
	DBE    dbe_addr;
	DBE    dbe_checkposnode;
	DBE    dbe_posnode; // @v10.1.10
	DBE  * p_add_paym = 0;
	CCheckTbl * t = 0;
	CCheckExtTbl * p_ext = 0;
	CSessionTbl * cs = 0;
	TempCCheckGrpTbl  * g = 0;
	TempCCheckQttyTbl * cq = 0;
	TempCCheckGdsCorrTbl * p_gc = 0;
	uint   brw_id = BROWSER_CCHECK;
	if(P_Ct) {
		brw_id = BROWSER_CCHECK_CT;
		p_q = PPView::CrosstabDbQueryStub;
	}
	else {
		switch(Filt.Grp) {
			case CCheckFilt::gTime:      brw_id = BROWSER_CCHECKGRP_TM;        break;
			case CCheckFilt::gDate:      brw_id = BROWSER_CCHECKGRP_DT;        break;
			case CCheckFilt::gDayOfWeek: brw_id = BROWSER_CCHECKGRP_DAYOFWEEK; break;
			case CCheckFilt::gDowNTime:  brw_id = BROWSER_CCHECKGRP_DOWNTIME;  break;
			case CCheckFilt::gCash:      brw_id = BROWSER_CCHECKGRP_CASH;      break;
			case CCheckFilt::gCashNode:  brw_id = BROWSER_CCHECKGRP_CASHNODE;  break;
			case CCheckFilt::gCard:      brw_id = BROWSER_CCHECKGRP_CARD;      break;
			case CCheckFilt::gDscntPct:  brw_id = BROWSER_CCHECKGRP_DSCNTPCT;  break;
			case CCheckFilt::gAmount:    brw_id = BROWSER_CCHECKGRP_AMOUNT;    break;
			case CCheckFilt::gQtty:      brw_id = BROWSER_CCHECKGRP_QTTY;      break;
			case CCheckFilt::gGoods:
			case CCheckFilt::gGoodsDate:
			case CCheckFilt::gAmountNGoods: brw_id = BROWSER_CCHECKGRP_GOODS;  break;
			case CCheckFilt::gCashiers:  brw_id = BROWSER_CCHECKGRP_CASHIERS;  break;
			case CCheckFilt::gAgents:    brw_id = BROWSER_CCHECKGRP_AGENTS;    break;
			case CCheckFilt::gDlvrAddr:  brw_id = BROWSER_CCHECKGRP_ADDR;      break;
			case CCheckFilt::gLinesCount: brw_id = BROWSER_CCHECKGRP_LNCOUNT;  break;
			case CCheckFilt::gGuestCount: brw_id = BROWSER_CCHECKGRP_GUESTCOUNT;  break;
			case CCheckFilt::gTableNo:   brw_id = BROWSER_CCHECKGRP_TABLENO;      break;
			case CCheckFilt::gDiv:       brw_id = BROWSER_CCHECKGRP_DIV;       break;
			case CCheckFilt::gAgentsNHour:    brw_id = BROWSER_CCHECKGRP_AGENTSNHOUR; break;
			case CCheckFilt::gAgentsNGoods:   brw_id = BROWSER_CCHECKGRP_AGENTSNHOUR; break;
			case CCheckFilt::gCashiersNGoods: brw_id = BROWSER_CCHECKGRP_AGENTSNHOUR; break;
			case CCheckFilt::gGoodsSCSer:     brw_id = BROWSER_CCHECKGRP_AGENTSNHOUR; break;
			case CCheckFilt::gAgentGoodsSCSer: brw_id = BROWSER_CCHECKGRP_AGENTSNHOUR; break; // @v9.6.6
			default: brw_id = (Filt.Flags & CCheckFilt::fGoodsCorr) ? BROWSER_CCHECK_GOODSCORR : BROWSER_CCHECK;
		}
		if(P_TmpGdsCorrTbl) {
			p_gc = new TempCCheckGdsCorrTbl(P_TmpGdsCorrTbl->GetName());
			p_q = & select(
				p_gc->ID__,           // #0
				p_gc->Goods1ID,       // #1
				p_gc->Goods2ID,       // #2
				p_gc->GoodsName1,     // #3
				p_gc->GoodsName2,     // #4
				p_gc->Count,          // #5
				p_gc->ChecksCount,    // #6
				p_gc->ChecksCountPct, // #7
				0L).from(p_gc, 0L);
			if(Filt.SortOrder == CCheckFilt::ordByName)
				p_q->orderBy(p_gc->GoodsName1, p_gc->Count, p_gc->GoodsName2, 0L);
			else
				p_q->orderBy(p_gc->Count, p_gc->GoodsName1, p_gc->GoodsName2, 0L);
		}
		else if(P_TmpGrpTbl) {
			DBE * p_dbe_avrg = 0;
			DBE * p_dbe_lc_avg = 0; // Среднее количество строк
			DBE * p_dbe_sc_avg = 0; // Среднее количество товаров
			g = new TempCCheckGrpTbl(P_TmpGrpTbl->GetName());
			p_dbe_avrg = & (g->Amount / g->Count);
			p_dbe_lc_avg = & (g->LinesCount / g->Count);
			p_dbe_sc_avg = & (g->SkuCount / g->Count);
			p_q = & select(
				g->ID__,        // #0
				g->Dt,          // #1
				g->Tm,          // #2
				g->Text,        // #3
				g->Count,       // #4
				g->Amount,      // #5
				g->Discount,    // #6
				g->PctPart,     // #7
				g->Qtty,        // #8
				*p_dbe_avrg,    // #9
				g->LinesCount,  // #10
				g->SkuCount,    // #11
				*p_dbe_lc_avg,  // #12
				*p_dbe_sc_avg,  // #13
				0L).from(g, 0L);
			delete p_dbe_avrg;
			delete p_dbe_lc_avg;
			delete p_dbe_sc_avg;
			switch(Filt.SortOrder) {
				case CCheckFilt::ordByCount: p_q->orderBy(g->Count, 0L); break;
				case CCheckFilt::ordByQtty:  p_q->orderBy(g->Qtty, 0L); break;
				case CCheckFilt::ordByAmt:   p_q->orderBy(g->Amount, 0L); break;
				default:
					if(oneof7(Filt.Grp, CCheckFilt::gDate, CCheckFilt::gDayOfWeek, CCheckFilt::gDowNTime,
						CCheckFilt::gCash, CCheckFilt::gDscntPct, CCheckFilt::gAmount, CCheckFilt::gQtty))
						p_q->orderBy(g->Dt, 0L);
					else if(oneof2(Filt.Grp, CCheckFilt::gGoodsDate, CCheckFilt::gCashNode))
						p_q->orderBy(g->Text, g->Dt, 0L);
					else
						p_q->orderBy(g->Text, 0L);
			}
		}
		else {
			if(P_TmpTbl) {
				cq = new TempCCheckQttyTbl(P_TmpTbl->GetName());
				PPDbqFuncPool::InitObjNameFunc(dbe_sc_code, PPDbqFuncPool::IdObjCodeSCard, cq->SCardID);
				PPDbqFuncPool::InitObjNameFunc(dbe_scowner_name, PPDbqFuncPool::IdSCardOwnerName, cq->SCardID);
				{
					dbe_posnode.init();
					dbe_posnode.push(cq->SessID);
					dbe_posnode.push(cq->CashID);
					dbe_posnode.push((DBFunc)DynFuncPosText);
				}
				p_q = & select(
					cq->ID,           // #0
					cq->Dt,           // #1
					cq->Tm,           // #2
					// @v10.1.10 cq->CashID,       // #3
					dbe_posnode,      // #3 // @v10.1.10
					cq->Flags,        // #4
					cq->Code,         // #5
					cq->Amount,       // #6
					dbe_sc_code,      // #7
					cq->Discount,     // #8
					cq->Qtty,         // #9
					cq->LinesCount,   // #10
					cq->SkuCount,     // #11
					dbe_scowner_name, // #12
					0L);
				if(State & stHasExt) {
					SETIFZ(p_ext, new CCheckExtTbl);
					PPDbqFuncPool::InitObjNameFunc(dbe_saler, PPDbqFuncPool::IdObjNameAr, p_ext->SalerID);
					p_add_paym = &(p_ext->AddPaym_unused / 100.0);
					p_q->addField(dbe_saler);           // #13
					p_q->addField(p_ext->TableNo);      // #14
					p_q->addField(p_ext->GuestCount);   // #15
					p_q->addField(*p_add_paym);         // #16
					p_q->addField(p_ext->Memo);    // #17
					if(Filt.Flags & CCheckFilt::fDlvrOnly) {
						{
							dbe_addr_city.init();
							dbe_addr_city.push(p_ext->AddrID);
							dbe_addr_city.push((DBFunc)PPDbqFuncPool::IdAddrCityName);
						}
						PPDbqFuncPool::InitFunc2Arg(dbe_addr_phone, PPDbqFuncPool::IdAddrExField, p_ext->AddrID, dbconst((long)LOCEXSTR_PHONE));
						PPDbqFuncPool::InitFunc2Arg(dbe_addr, PPDbqFuncPool::IdAddrExField, p_ext->AddrID, dbconst((long)LOCEXSTR_SHORTADDR));
						p_q->addField(p_ext->StartOrdDtm); // #18
						p_q->addField(dbe_addr_phone);     // #19
						p_q->addField(dbe_addr_city);      // #20
						p_q->addField(dbe_addr);           // #21
					}
					else if(Filt.Flags & CCheckFilt::fOrderOnly) {
						p_q->addField(p_ext->StartOrdDtm); // #18
						p_q->addField(p_ext->EndOrdDtm);   // #19
					}
					dbq = & (*dbq && (p_ext->CheckID += cq->ID));
					p_q->from(cq, p_ext, 0L);
				}
				else {
					p_q->from(cq, 0L);
				}
				p_q->where(*dbq).orderBy(cq->Dt, cq->Tm, 0L);
			}
			else {
				t = new CCheckTbl;
				PPDbqFuncPool::InitObjNameFunc(dbe_psn, PPDbqFuncPool::IdObjNamePerson, t->UserID);
				PPDbqFuncPool::InitObjNameFunc(dbe_sc_code, PPDbqFuncPool::IdObjCodeSCard, t->SCardID);
				PPDbqFuncPool::InitObjNameFunc(dbe_scowner_name, PPDbqFuncPool::IdSCardOwnerName, t->SCardID);
				{
					dbe_posnode.init();
					dbe_posnode.push(t->SessID);
					dbe_posnode.push(t->CashID);
					dbe_posnode.push((DBFunc)DynFuncPosText);
				}
				p_q = & select(
					t->ID,              // #0
					t->Dt,              // #1
					t->Tm,              // #2
					// @v10.1.10 t->CashID,          // #3
					dbe_posnode,        // #3 @v10.1.10
					t->Flags,           // #4
					t->Code,            // #5
					t->Amount,          // #6
					dbe_sc_code,        // #7
					t->Discount,        // #8
					dbe_psn,            // #9
					dbe_scowner_name,   // #10
					0L);
				if(State & stHasExt) {
					SETIFZ(p_ext, new CCheckExtTbl);
					PPDbqFuncPool::InitObjNameFunc(dbe_saler, PPDbqFuncPool::IdObjNameAr, p_ext->SalerID);
					p_add_paym = &(p_ext->AddPaym_unused / 100.0);
					p_q->addField(dbe_saler);         // #11
					p_q->addField(p_ext->TableNo);    // #12
					p_q->addField(p_ext->GuestCount); // #13
					p_q->addField(*p_add_paym);       // #14
					p_q->addField(p_ext->Memo);       // #15
					if(Filt.Flags & CCheckFilt::fDlvrOnly || Filt.DlvrAddrID) {
						{
							dbe_addr_city.init();
							dbe_addr_city.push(p_ext->AddrID);
							dbe_addr_city.push((DBFunc)PPDbqFuncPool::IdAddrCityName);
						}
						PPDbqFuncPool::InitFunc2Arg(dbe_addr_phone, PPDbqFuncPool::IdAddrExField, p_ext->AddrID, dbconst((long)LOCEXSTR_PHONE));
						PPDbqFuncPool::InitFunc2Arg(dbe_addr, PPDbqFuncPool::IdAddrExField, p_ext->AddrID, dbconst((long)LOCEXSTR_SHORTADDR));
						p_q->addField(p_ext->StartOrdDtm); // #16
						p_q->addField(dbe_addr_phone);     // #17
						p_q->addField(dbe_addr_city);      // #18
						p_q->addField(dbe_addr);           // #19
					}
					else if(Filt.Flags & CCheckFilt::fOrderOnly) {
						p_q->addField(p_ext->StartOrdDtm); // #16
						p_q->addField(p_ext->EndOrdDtm);   // #17
					}
				}
				if(Filt.Flags & (CCheckFilt::fCashOnly|CCheckFilt::fBankingOnly))
					dbq = ppcheckflag(dbq, t->Flags, CCHKF_BANKING, (Filt.Flags & CCheckFilt::fBankingOnly) ? 1 : -1);
				// @v10.0.02 {
				if(Filt.AltRegF)
					dbq = ppcheckflag(dbq, t->Flags, CCHKF_ALTREG, Filt.AltRegF);
				// } @v10.0.02
				dbq = ppcheckfiltid(dbq, t->CashID, Filt.CashNumber);
				dbq = ppcheckfiltid(dbq, t->UserID, Filt.CashierID);
				dbq = & (*dbq && daterange(t->Dt, &Filt.Period));
				int    cf = 0;
				if(Filt.Flags & CCheckFilt::fSuspendedOnly)
					cf = +1;
				else if(!(Filt.Flags & CCheckFilt::fShowSuspended) && !(Filt.Flags & CCheckFilt::fJunkOnly))
					cf = -1;
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_SUSPENDED, cf);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_RETURN,   (Filt.Flags & CCheckFilt::fRetOnly)  ? 1 : 0);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_JUNK,     (Filt.Flags & CCheckFilt::fJunkOnly) ? 1 : -1);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_PRINTED,  (Filt.Flags & CCheckFilt::fNotPrintedOnly) ? -1 : 0);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_HASGIFT,  (Filt.Flags & CCheckFilt::fGiftOnly)  ? 1 : 0);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_ORDER,    (Filt.Flags & CCheckFilt::fOrderOnly) ? 1 : 0);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_DELIVERY, (Filt.Flags & (CCheckFilt::fDlvrOnly|CCheckFilt::fDlvrOutstandOnly)) ? 1 : 0);
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_CLOSEDORDER, (Filt.Flags & CCheckFilt::fDlvrOutstandOnly) ? -1 : 0);
				{
					dbq = ppcheckflag(dbq, t->Flags, CCHKF_SKIP, (Filt.Flags & CCheckFilt::fWithoutSkipTag) ? -1 : 0);
					// @v9.7.11 {
					if(State & stSkipUnprinted)
						dbq = ppcheckflag(dbq, t->Flags, CCHKF_PRINTED, (Filt.Flags & CCheckFilt::fWithoutSkipTag) ? 1 : 0);
					// } @v9.7.11
				}
				dbq = ppcheckflag(dbq, t->Flags, CCHKF_SPFINISHED, (Filt.Flags & CCheckFilt::fNotSpFinished) ? -1 : 0); // @v9.7.5
				dbq = &(*dbq && intrange(t->Code, Filt.CodeR));
				if(!Filt.PcntR.IsZero()) {
					PPDbqFuncPool::InitPctFunc(dbe_pctdis, t->Discount, t->Amount, 1); // @pctdis
					dbq = &(*dbq && realrange(dbe_pctdis, Filt.PcntR.low, Filt.PcntR.upp));
				}
				if(!Filt.AmtR.IsZero()) {
					dbq = &(*dbq && realrange(t->Amount, Filt.AmtR.low, Filt.AmtR.upp));
				}
				if(SessIdList.GetCount() == 1) { // Здесь нельзя использовать GetSingle ибо единственный элементы может быть 0
					dbq = &(*dbq && t->SessID == SessIdList.GetSingle());
				}
				else if(!Filt.CashNumber && NodeIdList.GetCount()) {
					{
						dbe_checkposnode.init();
						dbe_checkposnode.push(t->SessID);
						if(NodeIdList.GetSingle()) {
							DBConst dbc_long;
							dbc_long.init(NodeIdList.GetSingle());
							dbe_checkposnode.push(dbc_long);
							dbe_checkposnode.push((DBFunc)PPDbqFuncPool::IdCheckCsPosNode);
							dbq = & (*dbq && dbe_checkposnode == (long)1);
						}
						else {
							DBConst dbc_ptr;
							dbc_ptr.init(&NodeIdList.Get());
							dbe_checkposnode.push(dbc_ptr);
							dbe_checkposnode.push((DBFunc)PPDbqFuncPool::IdCheckCsPosNodeList);
							dbq = & (*dbq && dbe_checkposnode == (long)1);
						}
					}
				}
				if(State & stHasExt) {
					if(Filt.AgentID || Filt.TableCode || Filt.GuestCount > 0 || Filt.DlvrAddrID || (Filt.Flags & CCheckFilt::fZeroDlvrAddr)) {
						dbq = &(*dbq && p_ext->CheckID == t->ID);
						dbq = ppcheckfiltid(dbq, p_ext->SalerID, Filt.AgentID);
						dbq = ppcheckfiltid(dbq, p_ext->TableNo, Filt.TableCode);
						dbq = ppcheckfiltid(dbq, p_ext->GuestCount, Filt.GuestCount);
						if(Filt.Flags & CCheckFilt::fZeroDlvrAddr) {
							dbq = &(*dbq && p_ext->AddrID == 0L);
						}
						else if(Filt.DlvrAddrID) {
							dbq = &(*dbq && p_ext->AddrID == Filt.DlvrAddrID);
						}
					}
					else
						dbq = & (*dbq && (p_ext->CheckID += t->ID));
					p_q->from(t, p_ext, cs, 0L);
				}
				else
					p_q->from(t, cs, 0L);
				p_q->where(*dbq);
				if(SessIdList.GetCount() == 1) // Здесь нельзя использовать GetSingle ибо единственный элементы может быть 0
					p_q->orderBy(t->SessID, 0L);
				else
					p_q->orderBy(t->Dt, t->Tm, 0L);
			}
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	delete p_add_paym;
	return p_q;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw && pData && pStyle) {
		const  BrowserDef * p_def = p_brw->getDef();
		if(col == 3) {
			struct _E {
				long  ID;
				LDATE Dt;
				LTIME Tm;
				long  CashID;
				long  Flags;
			};
			_E * p_row = (_E*)pData;
			if(p_row) {
				ok = 1;
				pStyle->Flags = BrowserWindow::CellStyle::fCorner;
				if(p_row->Flags & CCHKF_ORDER)
					pStyle->Color = (p_row->Flags & CCHKF_SKIP) ? DarkenColor(GetColorRef(SClrBlue), 0.1f) : LightenColor(GetColorRef(SClrBlue), 0.3f);
				else if(p_row->Flags & CCHKF_ZCHECK)
					pStyle->Color = LightenColor(GetColorRef(SClrYellow), 0.3f);
				else if(p_row->Flags & CCHKF_JUNK)
					pStyle->Color = GetColorRef(SClrBrown);
				else if(p_row->Flags & CCHKF_SUSPENDED)
					pStyle->Color = GetColorRef(SClrOrange);
				else if(p_row->Flags & CCHKF_RETURN)
					pStyle->Color = GetColorRef(SClrRed);
				else if(!(p_row->Flags & CCHKF_PRINTED))
					pStyle->Color = GetColorRef(SClrGrey);
				else
					ok = -1;
				if(p_row->Flags & CCHKF_DELIVERY) {
					pStyle->Color2 = (p_row->Flags & CCHKF_CLOSEDORDER) ? GetColorRef(SClrYellow) : GetColorRef(SClrCoral);
					pStyle->Flags &= ~BrowserWindow::CellStyle::fCorner;
					pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewCCheck::GetBrwHdr(const void * pRow, BrwHdr * pHdr) const
{
	int    ok = 1;
	memzero(pHdr, sizeof(*pHdr));
	if(pRow) {
		if(P_Ct) {
			struct CtHdr {
				long   ID;
				long   GoodsID;
			};
			const CtHdr * p_ct_hdr = (CtHdr *)pRow;
			pHdr->ID = p_ct_hdr->ID;
			pHdr->GoodsID = (p_ct_hdr->GoodsID == MAXLONG) ? 0 : p_ct_hdr->GoodsID;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

void SLAPI PPViewCCheck::PreprocessBrowser(PPViewBrowser * pBrw)
{
	SString title, buf;
	if(!(Filt.Flags & CCheckFilt::fGoodsCorr)) {
		if(pBrw && P_Ct) {
			BroCrosstab ct_col;
			BrowserDef * p_def = pBrw->view->getDef();
			if(p_def) {
				uint   col_width = 20;
	#define ADDCTCOLUMN(type, caption, format, options, width) \
				ct_col.P_Text  = newStr(caption); \
				ct_col.Type    = type; \
				ct_col.Format  = format; \
				ct_col.Options = options;  \
				ct_col.Width   = width; \
				p_def->AddCrosstab(&ct_col);

				p_def->FreeAllCrosstab();
				const long dfmt = MKSFMTD(col_width, 2, NMBF_NOZERO);
				if(Filt.CtValList.CheckID(CCheckFilt::ctvChecksSum) > 0) {
					GetCtColumnTitle(CCheckFilt::ctvChecksSum, title);
					ADDCTCOLUMN(T_DOUBLE, title, dfmt, 0, col_width);
				}
				if(Filt.CtValList.CheckID(CCheckFilt::ctvChecksCount) > 0) {
					GetCtColumnTitle(CCheckFilt::ctvChecksCount, title);
					ADDCTCOLUMN(T_DOUBLE, title, dfmt, 0, col_width);
				}
				if(Filt.CtValList.CheckID(CCheckFilt::ctvSKUCount) > 0) {
					GetCtColumnTitle(CCheckFilt::ctvSKUCount, title);
					ADDCTCOLUMN(T_DOUBLE, title, dfmt, 0, col_width);
				}
				P_Ct->SetupBrowserCtColumns(pBrw);
			}
		}
		else {
			if(Filt.Grp == 0) {
				PPLoadString("scardowner", buf);
				pBrw->insertColumn(-1, buf, P_TmpTbl ? 12 : 10, 0, MKSFMT(20, 0), 0);
			}
			if(DoProcessLines() && Filt.Grp == 0) {
				pBrw->InsColumn(4, "@qtty", 9, 0, MKSFMTD(12, 3, 0), 0);
			}
			else if(!P_TmpGrpTbl && !P_TmpTbl) {
				pBrw->InsColumn(6, "@cashier", 9, 0, MKSFMT(12, 0), 0);
			}
			if(Filt.Grp == CCheckFilt::gGoodsDate)
				pBrw->InsColumn(1, "@date", 1, 0, DATF_DMY, 0);
			if(!P_TmpGrpTbl && (State & stHasExt)) {
				int    pos = P_TmpTbl ? 13 : 11;
				pBrw->InsColumn    (-1, "@seller",          pos++, 0, MKSFMT(24, 0), 0);
				pBrw->InsColumnWord(-1, PPWORD_DINNERTABLE, pos++, 0, MKSFMT(5, NMBF_NOZERO), 0);
				pBrw->InsColumn    (-1, "@guestcount",      pos++, 0, MKSFMT(5, NMBF_NOZERO), 0);
				pBrw->InsColumnWord(-1, PPWORD_ADDPAYMENT,  pos++, 0, MKSFMTD(12, 2, NMBF_NOZERO), 0);
				pBrw->InsColumn    (-1, "@memo",            pos++, 0, MKSFMT(40, 0), 0);
				if(Filt.Flags & CCheckFilt::fDlvrOnly) {
					pBrw->InsColumn(-1, "@duetime",  pos++, 0, MKSFMT(10, 0), 0);
					pBrw->InsColumn(-1, "@phone",    pos++, 0, MKSFMT(10, 0), 0);
					pBrw->InsColumn(-1, "@city",     pos++, 0, MKSFMT(10, 0), 0);
					pBrw->InsColumn(-1, "@daddress", pos++, 0, MKSFMT(20, 0), 0);
				}
				else if(Filt.Flags & CCheckFilt::fOrderOnly) {
					pBrw->insertColumn(-1, "", pos++, 0, MKSFMT(10, 0), 0);
					pBrw->insertColumn(-1, "", pos++, 0, MKSFMT(10, 0), 0);
				}
			}
			if(Filt.Flags & CCheckFilt::fCalcSkuStat) {
				if(P_TmpTbl) {
					pBrw->InsColumnWord(-1, PPWORD_LINESCOUNT, 10, 0, MKSFMT(6, NMBF_NOZERO), 0);
					pBrw->InsColumnWord(-1, PPWORD_SKUCOUNT,   11, 0, MKSFMT(6, NMBF_NOZERO), 0);
				}
				else if(P_TmpGrpTbl) {
					pBrw->InsColumnWord(-1, PPWORD_LINESCOUNT,    10, 0, MKSFMT(6, NMBF_NOZERO), 0);
					pBrw->InsColumnWord(-1, PPWORD_AVGLINESCOUNT, 12, 0, MKSFMT(6, NMBF_NOZERO), 0);
					pBrw->InsColumnWord(-1, PPWORD_SKUCOUNT,      11, 0, MKSFMT(6, NMBF_NOZERO), 0);
					pBrw->InsColumnWord(-1, PPWORD_AVGSKUCOUNT,   13, 0, MKSFMT(6, NMBF_NOZERO), 0);
				}
			}
			if(Filt.Grp == 0)
				pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		}
	}
}

int SLAPI PPViewCCheck::OnExecBrowser(PPViewBrowser * pBrw)
{
	if(Filt.Flags & CCheckFilt::fImmOpenPanel && Filt.Flags & CCheckFilt::fActiveSess) {
		int    r = AddItem();
		if(r == 1) {
			pBrw->Update();
			return -1;
		}
		else if(r == 2) {
			// Опция кассового узла CASHF_NOMODALCHECKVIEW предписывает после кассовой панели
			// не входить в таблицу чеков
			return cmCancel;
		}
	}
	return -1;
}

int SLAPI PPViewCCheck::PosPrint(PPID checkID, long)
{
	return -1;
}

static int SLAPI PutGdsCorr(BExtInsert * pBei, PPID goods1ID, PPID goods2ID, SString & rG1Name, SString & rG2Name, long intersectChkCount, long goods1ChkCount, long goods2ChkCount, long totalChkCount)
{
	int    ok = 1;
	TempCCheckGdsCorrTbl::Rec gc_rec;
	THROW_INVARG(pBei);
	MEMSZERO(gc_rec);
	gc_rec.Goods1ID = goods1ID;
	gc_rec.Goods2ID = goods2ID;
	rG1Name.CopyTo(gc_rec.GoodsName1, sizeof(gc_rec.GoodsName1));
	rG2Name.CopyTo(gc_rec.GoodsName2, sizeof(gc_rec.GoodsName2));
	gc_rec.Count       = intersectChkCount;
	gc_rec.ChecksCount = goods1ChkCount;
	gc_rec.ChecksCountPct = (totalChkCount) ? ((double)gc_rec.ChecksCount / (double)totalChkCount) * 100 : 0;
	THROW_DB(pBei->insert(&gc_rec));
	MEMSZERO(gc_rec);
	gc_rec.Goods1ID = goods2ID;
	gc_rec.Goods2ID = goods1ID;
	rG1Name.CopyTo(gc_rec.GoodsName2, sizeof(gc_rec.GoodsName2));
	rG2Name.CopyTo(gc_rec.GoodsName1, sizeof(gc_rec.GoodsName1));
	gc_rec.Count       = intersectChkCount;
	gc_rec.ChecksCount = goods2ChkCount;
	gc_rec.ChecksCountPct = (totalChkCount) ? ((double)gc_rec.ChecksCount / (double)totalChkCount) * 100 : 0;
	THROW_DB(pBei->insert(&gc_rec));
	CATCHZOK
	return ok;
}

int SLAPI PPViewCCheck::CreateGoodsCorrTbl()
{
	int    ok = 1, c;
	uint   pos, pos1, goods_chk_pos = 0;
	LAssocArray * p_la_ary = 0, * p_goods_ary = 0, goods_chk_ary;
	CCheckViewItem  item;
	IterCounter cntr;
	PPIDArray   excl_goods_list;
	// Кроме ary_count все остальные переменные используются только для тестирования //
	ulong  corr_count = 0, ins_count = 0, chk_count = 0;
	ulong  min_p_la = 0xffffffff;
	ulong  max_p_la = 0;
	ulong  min_at_ary = 0xffffffff;
	ulong  max_at_ary = 0;
	ulong  max_corr = 0;
	long   ary_count = 0, la_count = 0;
	//
	long   laa_ary[1024];
	MEMSZERO(laa_ary);
	SString  goods1_name, goods2_name;
	RAssocArray gds_qtty_ary;
	PPObjGoods::SubstBlock sgg_blk;
	sgg_blk.ExclParentID = Filt.GoodsGrpID;
	if(Filt.GcoExcludeGrpID) {
		GoodsIterator::GetListByGroup(Filt.GcoExcludeGrpID, &excl_goods_list);
		excl_goods_list.sortAndUndup();
	}
	PPWait(1);
	ZDELETE(P_TmpGdsCorrTbl);
	for(InitIteration(0); NextIteration(&item) > 0;) {
		RAssoc * p_gds_qtty1 = 0, * p_gds_qtty2 = 0;
		CCheckLineTbl::Rec  cl_rec;
		gds_qtty_ary.clear();
		for(int p = 0; P_CC->EnumLines(item.ID, &p, &cl_rec) > 0;) {
			PPID goods_id = cl_rec.GoodsID;
			if(!excl_goods_list.bsearch(goods_id)) {
				if(Filt.Sgg) {
					THROW(GdsObj.SubstGoods(cl_rec.GoodsID, &cl_rec.GoodsID, Filt.Sgg, &sgg_blk, &Gsl));
				}
				THROW_SL(gds_qtty_ary.Add(cl_rec.GoodsID, cl_rec.Quantity, 1, 1));
			}
		}
		chk_count++;
		for(pos = 0; gds_qtty_ary.enumItems(&pos, (void **)&p_gds_qtty1);) {
			for(pos1 = pos; gds_qtty_ary.enumItems(&pos1, (void **)&p_gds_qtty2);) {
				PPID  goods1 = p_gds_qtty1->Key, goods2 = p_gds_qtty2->Key;
				int   ary_no = goods1 % 1024, ary_is_found = 0;
				uint  goods2_pos = 0;
				p_la_ary = (LAssocArray *)laa_ary[ary_no];
				p_goods_ary = 0;
				if(p_la_ary) {
					if(p_la_ary->BSearch(goods1, (long *)&p_goods_ary, 0))
						ary_is_found = 1;
				}
				else {
					THROW_MEM(p_la_ary = new LAssocArray);
					laa_ary[ary_no] = (long)p_la_ary;
					la_count++;
				}
				if(!ary_is_found) {
					THROW_MEM(p_goods_ary = new LAssocArray);
					THROW_SL(p_la_ary->Add(goods1, (long)p_goods_ary, 0, 1));
					ary_count++;
				}
				if(p_goods_ary->BSearch(goods2, 0, &goods2_pos)) {
					ulong  corr = ++p_goods_ary->at(goods2_pos).Val;
					max_corr = MAX(max_corr, corr);
				}
				else {
					THROW_SL(p_goods_ary->Add(goods2, 1, 0, 1));
					corr_count++;
				}
				ins_count++;
			}
			if(goods_chk_ary.BSearch(p_gds_qtty1->Key, 0, &(goods_chk_pos = 0)) > 0) // Количество чеков в которых встречается ведомый товар, будем хранить в отдельном массиве
				goods_chk_ary.at(goods_chk_pos).Val++;
			else {
				THROW_SL(goods_chk_ary.Add(p_gds_qtty1->Key, 1, 0, 1));
			}
		}
		PPWaitPercent(GetCounter());
	}
	THROW(P_TmpGdsCorrTbl = CreateTempGdsCorrFile());
	cntr.Init(ary_count);
	{
		BExtInsert bei(P_TmpGdsCorrTbl);
		PPTransaction tra(ppDbDependTransaction, 1);

		THROW(tra);
		for(c = 0; c < 1024; c++) {
			p_la_ary = (LAssocArray *)laa_ary[c];
			if(p_la_ary) {
				const uint la_count = p_la_ary->getCount();
				min_p_la = MIN(min_p_la, la_count);
				max_p_la = MAX(max_p_la, la_count);
				for(pos = 0; pos < la_count; pos++) {
					long     goods1_chk_count = 0;
					PPID     goods1_id = p_la_ary->at(pos).Key;
					p_goods_ary = (LAssocArray *)p_la_ary->at(pos).Val;
					const uint term_count = p_goods_ary->getCount();
					TempCCheckGdsCorrTbl::Rec gc_rec;

					min_at_ary = MIN(min_at_ary, term_count);
					max_at_ary = MAX(max_at_ary, term_count);
					if(Filt.Sgg)
						GdsObj.GetSubstText(goods1_id, Filt.Sgg, &Gsl, goods1_name);
					else {
						// @v9.5.5 GetGoodsName(goods1_id, goods1_name);
						GdsObj.FetchNameR(goods1_id, goods1_name); // @v9.5.5
					}
					goods_chk_ary.BSearch(goods1_id, &goods1_chk_count, 0);
					for(uint pos1 = 0; pos1 < term_count; pos1++) {
						long  count = p_goods_ary->at(pos1).Val;
						if(count >= Filt.GcoMinCount) {
							PPID     goods2_id = p_goods_ary->at(pos1).Key;
							long goods2_chk_count = 0;
							if(Filt.Sgg)
								GdsObj.GetSubstText(goods2_id, Filt.Sgg, &Gsl, goods2_name);
							else {
								// @v9.5.5 GetGoodsName(goods2_id, goods2_name);
								GdsObj.FetchNameR(goods2_id, goods2_name); // @v9.5.5
							}
							MEMSZERO(gc_rec);
							gc_rec.Goods1ID = goods1_id;
							gc_rec.Goods2ID = goods2_id;
							goods1_name.CopyTo(gc_rec.GoodsName1, sizeof(gc_rec.GoodsName1));
							goods2_name.CopyTo(gc_rec.GoodsName2, sizeof(gc_rec.GoodsName2));
							gc_rec.Count       = count;
							gc_rec.ChecksCount = goods1_chk_count;
							gc_rec.ChecksCountPct = (chk_count) ? ((double)count / (double)goods1_chk_count) * 100 : 0;
							THROW_DB(bei.insert(&gc_rec));

							goods_chk_ary.BSearch(goods2_id, &goods2_chk_count, 0);
							goods2_chk_count = MAX(goods2_chk_count, 1);
							MEMSZERO(gc_rec);
							gc_rec.Goods1ID = goods2_id;
							gc_rec.Goods2ID = goods1_id;
							goods1_name.CopyTo(gc_rec.GoodsName2, sizeof(gc_rec.GoodsName2));
							goods2_name.CopyTo(gc_rec.GoodsName1, sizeof(gc_rec.GoodsName1));
							gc_rec.Count       = count;
							gc_rec.ChecksCount = goods2_chk_count;
							gc_rec.ChecksCountPct = (chk_count) ? ((double)count / (double)goods2_chk_count) * 100 : 0;
							THROW_DB(bei.insert(&gc_rec));
						}
					}
					delete p_goods_ary;
					p_la_ary->at(pos).Val = 0;
					PPWaitPercent(cntr.Increment());
				}
				delete p_la_ary;
				laa_ary[c] = 0;
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		for(c = 0; c < 1024; c++) {
			p_la_ary = (LAssocArray *)laa_ary[c];
			if(p_la_ary) {
				for(pos = 0; pos < p_la_ary->getCount(); pos++) {
					p_goods_ary = (LAssocArray *)p_la_ary->at(pos).Val;
					delete p_goods_ary;
				}
				delete p_la_ary;
			}
		}
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

static int SetGoodsCorr(CCheckFilt * pFilt)
{
	int    ok = -1;
	if(pFilt) {
		TDialog * p_dlg = new TDialog(DLG_GOODSCORR);
		if(CheckDialogPtrErr(&p_dlg)) {
			p_dlg->setCtrlLong(CTL_GOODSCORR_MINQTTY, pFilt->GcoMinCount);
			p_dlg->AddClusterAssoc(CTL_GOODSCORR_SORT, 0, CCheckFilt::ordByCount);
			p_dlg->AddClusterAssoc(CTL_GOODSCORR_SORT, 1, CCheckFilt::ordByName);
			p_dlg->SetClusterData(CTL_GOODSCORR_SORT, pFilt->SortOrder);
			SetupPPObjCombo(p_dlg, CTLSEL_GOODSCORR_EXCLGG, PPOBJ_GOODSGROUP, pFilt->GcoExcludeGrpID,
				OLW_LOADDEFONOPEN|OLW_CANSELUPLEVEL, 0);
			SetupSubstGoodsCombo(p_dlg, CTLSEL_GOODSCORR_SUBST, pFilt->Sgg);
			if(pFilt->Grp != CCheckFilt::gNone)
				p_dlg->disableCtrl(CTLSEL_GOODSCORR_SUBST, 1);
			if(ExecView(p_dlg) == cmOK) {
				pFilt->GcoMinCount = p_dlg->getCtrlLong(CTL_GOODSCORR_MINQTTY);
				p_dlg->GetClusterData(CTL_GOODSCORR_SORT, &pFilt->SortOrder);
				p_dlg->getCtrlData(CTLSEL_GOODSCORR_EXCLGG, &pFilt->GcoExcludeGrpID);
				p_dlg->getCtrlData(CTLSEL_GOODSCORR_SUBST, &pFilt->Sgg);
				ok = 1;
			}
			delete p_dlg;
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI PPViewCCheck::ViewGoodsCorr()
{
	int    ok = -1;
	CCheckFilt temp_flt;
	temp_flt.Copy(&Filt, 1);
	if(Filt.Grp == CCheckFilt::gNone && SetGoodsCorr(&temp_flt) > 0) {
		temp_flt.Flags |= CCheckFilt::fGoodsCorr;
		ok = ViewCCheck(&temp_flt, 0);
	}
	return ok;
}

int SLAPI PPViewCCheck::ChangeFilt(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && Filt.Flags & CCheckFilt::fGoodsCorr) {
		long   prev_gco_order = Filt.SortOrder;
		long   prev_gco_min   = Filt.GcoMinCount;
		if(SetGoodsCorr(&Filt) > 0 && (Filt.GcoMinCount != prev_gco_min || Filt.SortOrder != prev_gco_order)) {
			uint   brw_id = 0;
			DBQuery * p_q = 0;
			if(Filt.GcoMinCount != prev_gco_min)
				THROW(CreateGoodsCorrTbl());
			THROW(p_q = CreateBrowserQuery(&brw_id, 0));
			ok = pBrw->ChangeResource(brw_id, p_q);
		}
	}
	else
		ok = PPView::ChangeFilt(0, pBrw);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewCCheck::EditGoods(const void * pHdr, int goodsNo)
{
	int    ok = -1;
	if(pHdr) {
		TempCCheckGdsCorrTbl::Rec * p_gc_rec = (TempCCheckGdsCorrTbl::Rec *)pHdr;
		PPID    goods_id   = (goodsNo == 1) ? p_gc_rec->Goods1ID : p_gc_rec->Goods2ID;
		SString goods_name = (goodsNo == 1) ? p_gc_rec->GoodsName1 : p_gc_rec->GoodsName2;
		if(GdsObj.Edit(&goods_id, 0) == cmOK) {
			Goods2Tbl::Rec  gds_rec;
			if(GdsObj.Fetch(goods_id, &gds_rec) > 0 && goods_name.Cmp(gds_rec.Name, 0)) {
				TempCCheckGdsCorrTbl::Key1  k1;
				TempCCheckGdsCorrTbl::Key2  k2;
				MEMSZERO(k1);
				k1.Goods1ID = goods_id;
				while(P_TmpGdsCorrTbl->search(1, &k1, spGt) && k1.Goods1ID == goods_id) {
					STRNSCPY(P_TmpGdsCorrTbl->data.GoodsName1, gds_rec.Name);
					THROW_DB(P_TmpGdsCorrTbl->updateRec());
				}
				MEMSZERO(k2);
				k2.Goods2ID = goods_id;
				while(P_TmpGdsCorrTbl->search(2, &k2, spGt) && k2.Goods2ID == goods_id) {
					STRNSCPY(P_TmpGdsCorrTbl->data.GoodsName2, gds_rec.Name);
					THROW_DB(P_TmpGdsCorrTbl->updateRec());
				}
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewCCheck::ToggleDlvrTag(PPID checkID)
{
	int    ok = -1;
	TDialog * dlg = 0;
	CCheckPacket cc_pack; // CCheckCore CCheckExtTbl CCheckTbl
	if(P_CC->LoadPacket(checkID, 0, &cc_pack) > 0 && cc_pack.Rec.Flags & CCHKF_DELIVERY) {
		if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_TOGGLECCDLVR)))) {
			SString temp_buf;
			dlg->SetupCalDate(CTLCAL_TOGGLECCDLVR_DT, CTL_TOGGLECCDLVR_DT);
			SetupTimePicker(dlg, CTL_TOGGLECCDLVR_TM, CTLTM_TOGGLECCDLVR_TM);
			CCheckCore::MakeCodeString(&cc_pack.Rec, temp_buf);
			dlg->setStaticText(CTL_TOGGLECCDLVR_INFO, temp_buf);
			dlg->setCtrlUInt16(CTL_TOGGLECCDLVR_FLAG, (cc_pack.Rec.Flags & CCHKF_CLOSEDORDER) ? 1 : 0);
			dlg->setCtrlDatetime(CTL_TOGGLECCDLVR_DT, CTL_TOGGLECCDLVR_TM, cc_pack.Ext.EndOrdDtm);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				LDATETIME dtm;
				long   c = dlg->getCtrlUInt16(CTL_TOGGLECCDLVR_FLAG) ? CCHKF_CLOSEDORDER : 0;
				if(c && !dlg->getCtrlDatetime(CTL_TOGGLECCDLVR_DT, CTL_TOGGLECCDLVR_TM, dtm))
					PPErrorByDialog(dlg, CTL_TOGGLECCDLVR_DT, PPERR_SLIB);
				else {
					cc_pack.Ext.EndOrdDtm = dtm;
					SETFLAG(cc_pack.Rec.Flags, CCHKF_CLOSEDORDER, dlg->getCtrlUInt16(CTL_TOGGLECCDLVR_FLAG) & 0x01);
					if(!P_CC->ToggleDeliveryCloseTag(checkID, dtm, c, 1))
						PPError();
					else
						ok = 1;
				}
			}
		}
	}
	delete dlg;
	return ok;
}

int SLAPI PPViewCCheck::ViewGraph()
{
	int    ok = -1;
	if(Filt.Grp) {
		SString temp_buf;
		Generator_GnuPlot plot(0);
		Generator_GnuPlot::PlotParam param;
		Generator_GnuPlot::StyleTics xtics;
		if(Filt.Grp == CCheckFilt::gDate)
			param.Flags |= Generator_GnuPlot::PlotParam::fLines;
		else
			param.Flags |= Generator_GnuPlot::PlotParam::fHistogram;
		param.Legend.Add(2, PPGetWord(PPWORD_SALES, 1, temp_buf));
		param.Legend.Add(3, PPGetWord(PPWORD_DISCOUNT, 1, temp_buf));
		plot.Preamble();
		if(Filt.Grp == CCheckFilt::gDate)
			plot.SetDateTimeFormat(Generator_GnuPlot::axX);
		plot.SetGrid();
		plot.SetStyleFill("solid");

		xtics.Rotate = 90;
		xtics.Font.Size = 8;
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, xtics.Font.Face);
		plot.SetTics(Generator_GnuPlot::axX, &xtics);
		if(Filt.Grp == CCheckFilt::gDate) {
			{
				PPGetWord(PPWORD_SALES, 1, temp_buf);
				PPGpPlotItem plot_item(plot.GetDataFileName(), temp_buf, PPGpPlotItem::sLines);
				plot_item.Style.SetLine(RGB(0xFF, 0x8C, 0x69), 3);
				plot_item.AddDataIndex(1);
				plot_item.AddDataIndex(2);
				plot.AddPlotItem(plot_item);
			}
			{
				PPGetWord(PPWORD_DISCOUNT, 1, temp_buf);
				PPGpPlotItem plot_item(plot.GetDataFileName(), temp_buf, PPGpPlotItem::sLines);
				plot_item.Style.SetLine(RGB(0x76, 0xEE, 0x00), 3);
				plot_item.AddDataIndex(1);
				plot_item.AddDataIndex(3);
				plot.AddPlotItem(plot_item);
			}
		}
		plot.Plot(&param);
		plot.StartData(1);
		if(Filt.Grp != CCheckFilt::gDate) {
			//
			// Заголовки столбцов нужны только для гистрограмм
			//
			plot.PutData(PPGetWord(PPWORD_GROUP, 1, temp_buf), 1);
			plot.PutData(PPGetWord(PPWORD_SALES, 1, temp_buf), 1);
			plot.PutData(PPGetWord(PPWORD_DISCOUNT, 1, temp_buf), 1);
			plot.PutEOR();
		}
		CCheckViewItem item;
		for(InitIteration(0); NextIteration(&item) > 0;) {
			if(Filt.Grp == CCheckFilt::gDate)
				plot.PutData(item.Dt);                           // #1
			else {
				(temp_buf = item.G_Text).Strip();
				if(temp_buf.NotEmpty())
					temp_buf.ReplaceChar(' ', '_').Transf(CTRANSF_INNER_TO_OUTER);
				else
					temp_buf.CatChar('-');
				plot.PutData(temp_buf, 1);                       // #1
			}
			plot.PutData(item.G_Amount);                         // #2
			plot.PutData(item.G_Discount);                       // #3
			plot.PutEOR();
		}
		plot.PutEndOfData();
		ok = plot.Run();
	}
	return ok;
}

int SLAPI PPViewCCheck::AddGoodsToBasket(PPID checkID)
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	if(Filt.Grp == CCheckFilt::gGoods) {
		param.SelPrice = 3;
		param.Flags = SelBasketParam::fNotSelPrice;
		THROW(r = GetBasketByDialog(&param, 0));
		if(r > 0) {
			CCheckViewItem item;
			PPWait(1);
			for(InitIteration(0); NextIteration(&item) > 0;) {
				ILTI   ilti;
				ilti.GoodsID  = item.G_GoodsID;
				ilti.Quantity = fabs(item.G_Qtty);
				ilti.Price    = MONEYTOLDBL(item.Amount) / ilti.Quantity;
				THROW(param.Pack.AddItem(&ilti, 0, param.SelReplace));
			}
			PPWait(0);
			THROW(GoodsBasketDialog(param, 1));
			ok = 1;
		}
	}
	else if(checkID) {
		CCheckPacket pack;
		THROW(P_CC->LoadPacket(checkID, 0, &pack) > 0);
		param.SelPrice = 3;
		param.Flags = SelBasketParam::fNotSelPrice;
		THROW(r = GetBasketByDialog(&param, 0));
		if(r > 0) {
			CCheckLineTbl::Rec ccl_rec;
			for(uint pos = 0; pack.EnumLines(&pos, &ccl_rec, 0) > 0;) {
				ILTI   ilti;
				ilti.GoodsID  = ccl_rec.GoodsID;
				ilti.Quantity = fabs(ccl_rec.Quantity);
				if(ilti.Quantity)
					ilti.Price = pack.GetItemAmount(ccl_rec) / ilti.Quantity;
				THROW(param.Pack.AddItem(&ilti, 0, param.SelReplace));
			}
			THROW(GoodsBasketDialog(param, 1));
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

SString & SLAPI PPViewCCheck::GetCtColumnTitle(int ct, SString & rBuf)
{
	rBuf.Space() = 0;
	SString temp_buf;
	if(ct == CCheckFilt::ctvChecksSum) {
		PPLoadText(PPTXT_CCHECKAMOUNT, rBuf);
	}
	else if(ct == CCheckFilt::ctvChecksCount) {
		PPLoadText(PPTXT_CCHECKCOUNT, rBuf);
	}
	else if(ct == CCheckFilt::ctvSKUCount)
		PPGetWord(PPWORD_SKUCOUNT, 0, rBuf);
	return rBuf;
}

struct CcDupEntry {
	int    FASTCALL IsDup(const CcDupEntry & rS) const
	{
		int    yes = 0;
		if(Code == rS.Code && PosId == rS.PosId) {
			long dif_days = 0;
			long dif = diffdatetime(Dtm, rS.Dtm, 4, &dif_days);
			if(dif_days == 0 && labs(dif) <= 1000 && feqeps(Amount, rS.Amount, 1E-5))
				yes = 1;
		}
		return yes;
	}
	long   ID;
	long   PosId;
	long   Code;
	LDATETIME Dtm;
	double Amount;
};

IMPL_CMPCFUNC(CcDupEntry, p1, p2)
{
	const CcDupEntry * i1 = (const CcDupEntry *)p1;
	const CcDupEntry * i2 = (const CcDupEntry *)p2;
	int   si = 0;
	CMPCASCADE2(si, i1, i2, PosId, Code);
	return NZOR(si, cmp(i1->Dtm, i2->Dtm));
}

static int DetectCcDups(TSVector <CcDupEntry> & rList, TSVector <CcDupEntry> & rDestList)
{
	rDestList.clear();
	int    ok = -1;
	const  uint _c = rList.getCount();
	uint   last_dup_entry_pos = 0;
	rList.sort(PTR_CMPCFUNC(CcDupEntry));
	for(uint i = 0; i < _c; i++) {
		const CcDupEntry & r_cur_item = rList.at(i);
		if(i < (_c-1)) {
			const CcDupEntry & r_next_item = rList.at(i+1);
			if(r_cur_item.IsDup(r_next_item)) {
				if(i != last_dup_entry_pos)
					rDestList.insert(&r_cur_item);
				rDestList.insert(&r_next_item);
				last_dup_entry_pos = i+1;
			}
		}
	}
	return ok;
}

int SLAPI PPViewCCheck::Recover()
{
	int    ok = -1;
	if(Filt.Grp == Filt.gNone) { // @v10.0.04
		PPLogger logger;
		SString log_fname;
		long   flags = 0;
		CCheckViewItem item;
		TSVector <CcDupEntry> full_list;
		TSVector <CcDupEntry> dup_list;
		int   do_cancel = 1;
		{
			TDialog * dlg = new TDialog(DLG_CORCC);
			THROW(CheckDialogPtr(&dlg));
			FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORCC_LOG, CTL_CORCC_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
			PPGetFileName(PPFILNAM_CCVERR_LOG, log_fname);
			dlg->setCtrlString(CTL_CORCC_LOG, log_fname);
			dlg->AddClusterAssoc(CTL_CORCC_FLAGS, 0, 0x01);
			dlg->SetClusterData(CTL_CORCC_FLAGS, flags);
			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlString(CTL_CORCC_LOG, log_fname);
				dlg->GetClusterData(CTL_CORCC_FLAGS, &flags);
				do_cancel = 0;
			}
			delete dlg;
		}
		if(!do_cancel) {
			PPWait(1);
			for(InitIteration(0); PPCheckUserBreak() && NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
				P_CC->ValidateCheck(item.ID, 0.001, logger);
				CcDupEntry dup_entry;
				dup_entry.ID = item.ID;
				dup_entry.Code = item.Code;
				dup_entry.PosId = item.CashID;
				dup_entry.Dtm.Set(item.Dt, item.Tm);
				dup_entry.Amount = MONEYTOLDBL(item.Amount);
				full_list.insert(&dup_entry);
			}
			DetectCcDups(full_list, dup_list);
			if(dup_list.getCount()) {
				SString fmt_buf, msg_buf, cc_buf;
				PPIDArray list_to_remove;
				//PPTXT_CCHKERR_DUP            "Обнаружены дублированные чеки: %s"
				PPLoadText(PPTXT_CCHKERR_DUP, fmt_buf);
				for(uint i = 0; i < dup_list.getCount(); i++) {
					const CcDupEntry & r_entry = dup_list.at(i);
					cc_buf.Z();
					for(uint inner_idx = i+1; inner_idx < dup_list.getCount(); inner_idx++) {
						const CcDupEntry & r_inner_entry = dup_list.at(inner_idx);
						if(r_inner_entry.IsDup(r_entry)) {
							if(inner_idx == (i+1)) {
								cc_buf.CatChar('[').Cat(r_entry.ID).CatDiv('-', 1).Cat(r_entry.PosId).CatDiv('-', 1).
									Cat(r_entry.Code).CatDiv('-', 1).Cat(r_entry.Dtm, DATF_DMY, TIMF_HMS).CatDiv('-', 1).
									Cat(r_entry.Amount, MKSFMTD(0, 2, 0)).CatChar(']');
							}
							cc_buf.CatDiv(':', 1);
							cc_buf.CatChar('[').Cat(r_inner_entry.ID).CatDiv('-', 1).Cat(r_inner_entry.PosId).CatDiv('-', 1).
								Cat(r_inner_entry.Code).CatDiv('-', 1).Cat(r_inner_entry.Dtm, DATF_DMY, TIMF_HMS).CatDiv('-', 1).
								Cat(r_inner_entry.Amount, MKSFMTD(0, 2, 0)).CatChar(']');
							list_to_remove.add(r_inner_entry.ID);
						}
					}
					if(cc_buf.NotEmpty()) {
						msg_buf.Printf(fmt_buf, cc_buf.cptr());
						logger.Log(msg_buf);
					}
				}
				if(list_to_remove.getCount()) {
					list_to_remove.sortAndUndup();
					//PPTXT_CCHKERR_DUPTOTAL       "Всего обнаружено %ld дубликитов чеков, которые следует удалить"
					PPLoadText(PPTXT_CCHKERR_DUPTOTAL, fmt_buf);
					logger.Log(msg_buf.Printf(fmt_buf, (long)list_to_remove.getCount()));
					if(flags & 0x01) { // Исправлять ошибки
						PPTransaction tra(1);
						THROW(tra);
						for(uint tridx = 0; tridx < list_to_remove.getCount(); tridx++) {
							const PPID id_to_remove = list_to_remove.get(tridx);
							THROW(P_CC->RemovePacket(id_to_remove, 0));
						}
						THROW(tra.Commit());
					}
				}
			}
			PPWait(0);
			logger.Save(log_fname, 0);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewCCheck::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int   ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				ok = -1;
				if(id)
					if(Filt.Grp || (Filt.Flags & CCheckFilt::fGoodsCorr))
						Detail(pHdr, pBrw);
					else {
						PPID   cn_id = 0;
						P_CC->GetNodeID(id, &cn_id);
						CCheckPane(cn_id, id);
						ok = 1;
					}
				break;
			case PPVCMD_TOGGLE:
				ok = -1;
				if(id) {
					if(ToggleDlvrTag(id) > 0)
						ok = 1;
				}
				break;
			case PPVCMD_ADDITEM:
				if(Filt.Flags & CCheckFilt::fGoodsCorr)
					ok = EditGoods(pHdr, 1);
				else
					ok = AddItem();
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(!Filt.Grp)
					if(CsObj.CheckRights(CSESSRT_RMVCHECK)) {
						if(id && PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) == cmYes) {
							ok = P_CC->RemovePacket(id, 1);
							if(ok) {
								DS.LogAction(PPACN_CCHECKDELETED, PPOBJ_CCHECK, id, 0, 1);
								if(P_TmpTbl)
									deleteFrom(P_TmpTbl, 1, P_TmpTbl->ID == id);
							}
						}
					}
					else
						ok = 0;
				if(!ok)
					PPError();
				break;
			case PPVCMD_ADDBYSAMPLE:
				if(Filt.Flags & CCheckFilt::fGoodsCorr)
					ok = EditGoods(pHdr, 2);
				else {
					ok = -1;
					PPViewCCheck::CreateDraftBySuspCheck(this, id);
				}
				break;
			case PPVCMD_SYSTEMINFO:
				ok = EditItemInfo(id);
				break;
			case PPVCMD_DORECOVER:
				ok = Recover();
				break;
			case PPVCMD_DELETEALL:
				ok = RemoveAll(); // Удаляет только чеки с неопределенной сессией
				break;
			case PPVCMD_POSPRINT:
				ok = PosPrint(id, 0);
				break;
			case PPVCMD_GOODSCORR:
				ok = -1;
				ViewGoodsCorr();
				break;
			case PPVCMD_ADDALLTOBASKET:
				ok = -1;
				AddGoodsToBasket(0);
				break;
			case PPVCMD_ADDTOBASKET:
				ok = -1;
				if(!Filt.Grp)
					AddGoodsToBasket(id);
				break;
			case PPVCMD_GRAPH:
				ok = -1;
				ViewGraph();
				break;
			case PPVCMD_CHANGEFILT:
				ok = ChangeFilt(pBrw);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewCCheck::CalcTotal(CCheckTotal * pTotal)
{
	int    ok = 1;
	CCheckViewItem item;
	if(pTotal) {
		CSessTotal cs_total;
		PPWait(1);
		memzero(pTotal, sizeof(CCheckTotal));
		pTotal->MinCheckSum = MAXLONG;
		for(InitIteration(0); NextIteration(&item) > 0;) {
			const double amount = MONEYTOLDBL(item.Amount);
			if(Filt.Grp != CCheckFilt::gNone) {
				cs_total.Amount += amount;
				cs_total.BnkAmount += item.BnkAmt;
				cs_total.CSCardAmount += item.CrdCardAmt;
				pTotal->Count += item.G_Count;
				pTotal->Qtty  += item.G_Qtty;
				pTotal->GuestCount += item.GuestCount;
			}
			else {
				P_CC->AddRecToSessTotal(&item, &cs_total);
				pTotal->Count  += 1;
				pTotal->Qtty   += item.G_Qtty;
				{
					SETMIN(pTotal->MinCheckSum, fabs(amount));
					SETMAX(pTotal->MaxCheckSum, fabs(amount));
				}
				pTotal->GuestCount += item.GuestCount;
			}
			PPWaitPercent(GetCounter());
		}
		pTotal->Amount = cs_total.Amount;
		pTotal->Discount = cs_total.Discount;
		pTotal->AmtBank = cs_total.BnkAmount;
		pTotal->AmtSCard = cs_total.CSCardAmount;
		pTotal->AmtCash = (cs_total.Amount - cs_total.BnkAmount - cs_total.CSCardAmount);
		pTotal->AmtReturn = cs_total.RetAmount;
		pTotal->AmtAltReg = cs_total.AltRegAmount; // @v10.0.02
		pTotal->CountAltReg = cs_total.AltRegCount; // @v10.0.02
		if(Filt.Grp == CCheckFilt::gNone && pTotal->Count)
			pTotal->AvrgCheckSum = pTotal->Amount / pTotal->Count;
		else
			pTotal->MinCheckSum = 0.0;
		PPWait(0);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int SLAPI PPViewCCheck::RemoveAll()
{
	int    ok = -1;
	if(Filt.Flags & CCheckFilt::fZeroSess && CONFIRMCRIT(PPCFM_DELETE)) {
		CCheckViewItem item;
		PPIDArray id_list;
		for(InitIteration(0); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			THROW(id_list.add(item.ID));
		id_list.sortAndUndup();
		{
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < id_list.getCount(); i++) {
				THROW(P_CC->RemovePacket(id_list.at(i), 0));
				ok = 1;
				PPWaitPercent(i, id_list.getCount());
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewCCheck::ViewTotal()
{
	int    ok = 1;
	TDialog * dlg = 0;
	CCheckTotal total;
	CalcTotal(&total);
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_CCHECKTOTAL)))) {
		dlg->setCtrlLong(CTL_CCHECKTOTAL_COUNT,    total.Count);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_QTTY,     total.Qtty);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_AMOUNT,   total.Amount);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_DISCOUNT, total.Discount);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_ACASH,    total.AmtCash);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_ABANK,    total.AmtBank);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_ASCARD,   total.AmtSCard);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_MINSUM,   total.MinCheckSum);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_AVRGSUM,  total.AvrgCheckSum);
		dlg->setCtrlReal(CTL_CCHECKTOTAL_MAXSUM,   total.MaxCheckSum);
		dlg->setCtrlLong(CTL_CCHECKTOTAL_GUESTC,   total.GuestCount);
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewCCheck::AddItem()
{
	if(!Filt.Grp) {
		int  to_view = 0, close_imm = 0;
		if(CsObj.CheckRights(CSESSRT_ADDCHECK))
			to_view = 1;
		// Допускается работа с кассовой панелью (в ограниченом режиме: можно создать отложенный чек, но нельзя чек провести)
		else if(Filt.Flags & CCheckFilt::fImmOpenPanel) {
			close_imm = 1;
			to_view = 1;
		}
		if(to_view) {
			const PPID cn_id = NodeIdList.GetSingle();
			if(Filt.Flags & CCheckFilt::fActiveSess && cn_id) {
				int    r = cmOK;
				PPCashMachine * p_cm = PPCashMachine::CreateInstance(cn_id), * p_cm_ext = 0;
				if(p_cm) {
					PPSyncCashNode  scn;
					PPObjCashNode   cn_obj;
					if(cn_obj.GetSync(cn_id, &scn) > 0) {
						LDATE  dt = ZERODATE;
						if(scn.Flags & CASHF_NOMODALCHECKVIEW)
							close_imm = 1;
						if(scn.Flags & CASHF_DAYCLOSED) {
							r = cmNo;
							if(PPMessage(mfConf|mfYesNo, PPCFM_PREVCASHDAYCLOSED) == cmYes && p_cm->SyncOpenSession(&dt) > 0)
								r = cmOK;
						}
						if(r == cmOK && scn.ExtCashNodeID) {
							p_cm_ext = PPCashMachine::CreateInstance(scn.ExtCashNodeID);
							if(p_cm_ext) {
								const PPCashNode & r_cn = p_cm_ext->GetNodeData();
								if(r_cn.Flags & CASHF_DAYCLOSED) {
									r = cmNo;
									if((dt || PPMessage(mfConf|mfYesNo, PPCFM_PREVCASHDAYCLOSED) == cmYes) && p_cm_ext->SyncOpenSession(&dt) > 0)
										r = cmOK;
								}
							}
						}
					}
				}
				delete p_cm;
				delete p_cm_ext;
				if(r == cmOK) {
					CCheckPane(cn_id, 0);
					return close_imm ? 2 : 1;
				}
			}
		}
		else
			return PPErrorZ();
	}
	return -1;
}

// @<<PPViewCSess::CreateDraft
int SLAPI PPViewCCheck::GetPacket(PPID id, CCheckPacket * pPack)
{
	return P_CC->LoadPacket(id, 0, pPack);
}

class CCheckInfoDialog : public TDialog {
public:
	CCheckInfoDialog() : TDialog(DLG_CCHECKINFO), CanModif(0)
	{
	}
	int    setDTS(const CCheckPacket * pData)
	{
		int    ok = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		CSessionTbl::Rec csess_rec;
		SCardTbl::Rec sc_rec;
		char   scard_no[32];
		setCtrlData(CTL_CCHECKINFO_ID,       &Data.Rec.ID);
		setCtrlData(CTL_CCHECKINFO_CODE,     &Data.Rec.Code);
		setCtrlData(CTL_CCHECKINFO_CASHCODE, &Data.Rec.CashID);
		setCtrlData(CTL_CCHECKINFO_USERID,   &Data.Rec.UserID);
		setCtrlData(CTL_CCHECKINFO_SESSID,   &Data.Rec.SessID);
		setCtrlData(CTL_CCHECKINFO_DATE,     &Data.Rec.Dt);
		setCtrlData(CTL_CCHECKINFO_TIME,     &Data.Rec.Tm);
		setCtrlData(CTL_CCHECKINFO_AMOUNT,   Data.Rec.Amount);
		setCtrlData(CTL_CCHECKINFO_DSCNT,    Data.Rec.Discount);
		setCtrlReal(CTL_CCHECKINFO_ADDPAYM,  0.0); // @v9.0.4 fdiv100i(Data.Ext.AddPaym)-->0.0
		if((Data.Rec.SessID == 0 || CsObj.Search(Data.Rec.SessID, &csess_rec) <= 0) && PPMaster) {
			//
			// Если ИД сессии равен нулю или сессия не найдена и работает master,
			// то следующие поля не блокируем
			//
			// CTL_CCHECKINFO_CODE
			// CTL_CCHECKINFO_CASHCODE
			// CTL_CCHECKINFO_SESSID
			// CTL_CCHECKINFO_DATE
			// CTL_CCHECKINFO_TIME
			//
			disableCtrls(1, CTL_CCHECKINFO_ID, CTL_CCHECKINFO_USERID, CTL_CCHECKINFO_AMOUNT, CTL_CCHECKINFO_DSCNT, 0);
			CanModif = 1;
		}
		else if((Data.Rec.Flags & CCHKF_NOTUSED) && oneof2(csess_rec.Incomplete, CSESSINCMPL_GLINES, CSESSINCMPL_COMPLETE) && PPMaster) {
			disableCtrls(1, CTL_CCHECKINFO_ID, CTL_CCHECKINFO_USERID, CTL_CCHECKINFO_AMOUNT, CTL_CCHECKINFO_DSCNT, 0);
			CanModif = 1;
		}
		else {
			disableCtrls(1, CTL_CCHECKINFO_ID, CTL_CCHECKINFO_CODE, CTL_CCHECKINFO_CASHCODE,
				CTL_CCHECKINFO_USERID, CTL_CCHECKINFO_SESSID, CTL_CCHECKINFO_DATE,
				CTL_CCHECKINFO_TIME, CTL_CCHECKINFO_AMOUNT, CTL_CCHECKINFO_DSCNT, 0);
		}
		enableCommand(cmPrint, FiscalPrintintgEnabled());
		if(ScObj.Search(Data.Rec.SCardID, &sc_rec) > 0)
			STRNSCPY(scard_no, sc_rec.Code);
		else
			scard_no[0] = 0;
		setCtrlData(CTL_CCHECKINFO_CARDNO, scard_no);
		disableCtrl(CTL_CCHECKINFO_CARDNO, !ScObj.CheckRights(SCRDRT_BINDING));
		SetupArCombo(this, CTLSEL_CCHECKINFO_AGENT, Data.Ext.SalerID, OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet);
		setCtrlLong(CTL_CCHECKINFO_TABLE, Data.Ext.TableNo);
		setCtrlLong(CTL_CCHECKINFO_LINKID, Data.Ext.LinkCheckID);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  0, CCHKF_SYNC);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  1, CCHKF_NOTUSED);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  2, CCHKF_PRINTED);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  3, CCHKF_RETURN);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  4, CCHKF_ZCHECK);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  5, CCHKF_TRANSMIT);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  6, CCHKF_BANKING);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  7, CCHKF_INCORPCRD);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  8, CCHKF_SKIP);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS,  9, CCHKF_SUSPENDED);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS, 10, CCHKF_JUNK);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS, 11, CCHKF_EXT);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS, 12, CCHKF_LINEEXT);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS, 13, CCHKF_HASGIFT);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS, 14, CCHKF_BONUSCARD);
		SetClusterData(CTL_CCHECKINFO_FLAGS, Data.Rec.Flags);

		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 0, CCHKF_ORDER);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 1, CCHKF_CLOSEDORDER);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 2, CCHKF_DELIVERY);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 3, CCHKF_FIXEDPRICE);
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 4, CCHKF_SPFINISHED); // @v9.7.5
		AddClusterAssoc(CTL_CCHECKINFO_FLAGS2, 5, CCHKF_ALTREG); // @v9.7.8
		SetClusterData(CTL_CCHECKINFO_FLAGS2, Data.Rec.Flags);
		if(Data.AL_Const().getCount()) {
			SmartListBox * p_box = (SmartListBox*)getCtrlView(CTL_CCHECKINFO_PAYMLIST);
			if(p_box && SetupStrListBox(p_box)) {
				StringSet ss(SLBColumnDelim);
				for(uint i = 0; i < Data.AL_Const().getCount(); i++) {
					const CcAmountEntry & r_entry = Data.AL_Const().at(i);
					ss.clear();
					r_entry.GetTypeText(temp_buf);
					ss.add(temp_buf);
					temp_buf.Z().Cat(r_entry.Amount, SFMT_MONEY);
					ss.add(temp_buf);
					temp_buf.Z();
					if(r_entry.AddedID) {
						if(ScObj.Fetch(r_entry.AddedID, &sc_rec) > 0)
							temp_buf = sc_rec.Code;
						else
							ideqvalstr(r_entry.AddedID, temp_buf);
					}
					ss.add(temp_buf);
					p_box->addItem(i+1, ss.getBuf());
				}
				p_box->Draw_();
				p_box->focusItem(0);
			}
		}
		{
			temp_buf.Z();
			ObjTagItem tag_item;
			if(PPRef->Ot.GetTag(PPOBJ_CCHECK, Data.Rec.ID, PPTAG_CCHECK_JS_UUID, &tag_item) > 0) {
				S_GUID sess_uuid;
				if(tag_item.GetGuid(&sess_uuid) > 0)
					sess_uuid.ToStr(S_GUID::fmtIDL, temp_buf);
			}
			setCtrlString(CTL_CCHECKINFO_SESSUUID, temp_buf);
		}
		return ok;
	}
	int    getDTS(CCheckPacket * pData)
	{
		int    ok = 1;
		long   flags = Data.Rec.Flags;
		PPID   card_id = Data.Rec.SCardID;
		char   scard_no[32];
		SCardTbl::Rec sc_rec;
		GetClusterData(CTL_CCHECKINFO_FLAGS, &flags);
		GetClusterData(CTL_CCHECKINFO_FLAGS2, &flags);
		if(ScObj.CheckRights(SCRDRT_BINDING)) {
			getCtrlData(CTL_CCHECKINFO_CARDNO, scard_no);
			if(*strip(scard_no)) {
				if(ScObj.SearchCode(0, scard_no, &sc_rec) > 0)
					card_id = sc_rec.ID;
				else {
					ok = PPSetError(PPERR_SCARDNOTFOUND, scard_no);
					PPErrorByDialog(this, CTL_CCHECKINFO_CARDNO);
				}
			}
			else
				card_id = 0;
		}
		if(ok) {
			ok = 0;
			if(CanModif) {
				getCtrlData(CTL_CCHECKINFO_CODE,     &Data.Rec.Code);
				getCtrlData(CTL_CCHECKINFO_CASHCODE, &Data.Rec.CashID);
				getCtrlData(CTL_CCHECKINFO_SESSID,   &Data.Rec.SessID);
				getCtrlData(CTL_CCHECKINFO_DATE,     &Data.Rec.Dt);
				getCtrlData(CTL_CCHECKINFO_TIME,     &Data.Rec.Tm);
				ok |= 0x0002;
			}
			if(flags != Data.Rec.Flags) {
				Data.Rec.Flags = flags;
				ok |= 0x0004;
			}
			if(card_id != Data.Rec.SCardID) {
				Data.Rec.SCardID = card_id;
				ok |= 0x0008;
			}
			if(ok) {
				ASSIGN_PTR(pData, Data);
			}
			else
				ok = -1;
		}
		return ok;
	}
	int    GetCanModifStatus() const { return CanModif; }
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmPrint)) {
			if(FiscalPrintintgEnabled()) {
				CheckPaneDialog * cc_dlg = new CheckPaneDialog(Data.Rec.CashID, Data.Rec.ID, &Data);
                if(cc_dlg) {
					int r = cc_dlg->AcceptCheck(0, 0, 0.0, CPosProcessor::accmAveragePrinting);
					ZDELETE(cc_dlg);
					if(r > 0) {
						CCheckPacket new_pack;
						if(ScObj.P_CcTbl->LoadPacket(Data.Rec.ID, 0, &new_pack) > 0)
							setDTS(&new_pack);
					}
                }

			}
		}
	}
	int    SLAPI FiscalPrintintgEnabled() const
	{
		return (!(Data.Rec.Flags & (CCHKF_PRINTED|CCHKF_SUSPENDED)) && (Data.Rec.Flags & CCHKF_SYNC) && Data.Rec.CashID && Data.Rec.SessID);
	}
	CCheckPacket Data;
	PPObjCSession CsObj;
	PPObjSCard ScObj;
	int    CanModif;
};

// static
int SLAPI PPViewCCheck::EditCCheckSystemInfo(CCheckPacket & rPack)
{
	int    ok = -1, valid_data = 0;
	CCheckInfoDialog * dlg = 0;
	int    can_modif = 0;
	PPObjSCard sc_obj;
	PPObjCSession cs_obj;
	SCardTbl::Rec sc_rec;
	char   scard_no[32];
	THROW(cs_obj.CheckRights(CSESSRT_CHECKINFO));
	THROW(CheckDialogPtr(&(dlg = new CCheckInfoDialog())));
	dlg->setDTS(&rPack);
	while(!valid_data && ExecView(dlg) == cmOK) {
		valid_data = 1;
		can_modif = dlg->GetCanModifStatus();
		long   flags = rPack.Rec.Flags;
		PPID   card_id = rPack.Rec.SCardID;
		dlg->GetClusterData(CTL_CCHECKINFO_FLAGS, &flags);
		dlg->GetClusterData(CTL_CCHECKINFO_FLAGS2, &flags);
		if(sc_obj.CheckRights(SCRDRT_BINDING)) {
			dlg->getCtrlData(CTL_CCHECKINFO_CARDNO, scard_no);
			if(*strip(scard_no)) {
				if(sc_obj.SearchCode(0, scard_no, &sc_rec) > 0)
					card_id = sc_rec.ID;
				else {
					PPError(PPERR_SCARDNOTFOUND, scard_no);
					dlg->selectCtrl(CTL_CCHECKINFO_CARDNO);
					valid_data = 0;
				}
			}
			else
				card_id = 0;
		}
		if(valid_data) {
			ok = 0;
			if(can_modif) {
				dlg->getCtrlData(CTL_CCHECKINFO_CODE,     &rPack.Rec.Code);
				dlg->getCtrlData(CTL_CCHECKINFO_CASHCODE, &rPack.Rec.CashID);
				dlg->getCtrlData(CTL_CCHECKINFO_SESSID,   &rPack.Rec.SessID);
				dlg->getCtrlData(CTL_CCHECKINFO_DATE,     &rPack.Rec.Dt);
				dlg->getCtrlData(CTL_CCHECKINFO_TIME,     &rPack.Rec.Tm);
				ok |= 0x0002;
			}
			if(flags != rPack.Rec.Flags) {
				rPack.Rec.Flags = flags;
				ok |= 0x0004;
			}
			if(card_id != rPack.Rec.SCardID) {
				rPack.Rec.SCardID = card_id;
				ok |= 0x0008;
			}
			if(!ok)
				ok = -1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewCCheck::EditItemInfo(PPID id)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!Filt.Grp) {
		CCheckPacket pack, org_pack;
		THROW(CsObj.CheckRights(CSESSRT_CHECKINFO));
		THROW(P_CC->LoadPacket(id, 0, &org_pack) > 0);
		pack = org_pack;
		{
			const int dr = PPViewCCheck::EditCCheckSystemInfo(pack);
			if(dr > 0) {
				PPTransaction tra(1);
				THROW(tra);
				if(dr & 0x0002) {
					org_pack.Rec.Code = pack.Rec.Code;
					org_pack.Rec.CashID = pack.Rec.CashID;
					org_pack.Rec.SessID = pack.Rec.SessID;
					org_pack.Rec.Dt = pack.Rec.Dt;
					org_pack.Rec.Tm = pack.Rec.Tm;
					THROW(P_CC->UpdateRec(id, &org_pack.Rec, 0));
				}
				if(dr & 0x0004) {
					THROW(P_CC->UpdateFlags(id, pack.Rec.Flags, 0));
				}
				if(dr & 0x0008) {
					THROW(P_CC->UpdateSCard(id, pack.Rec.SCardID, 0));
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewCCheck::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pHdr) {
		if(P_Ct) {
			BrwHdr hdr;
			DateRange ct_period;
			MEMSZERO(hdr);
			if(GetBrwHdr(pHdr, &hdr)) {
				uint   tab_idx = pBrw ? pBrw->GetCurColumn() : 0;
				PPID   tab_id = 0;
				// @v9.8.7 DBFieldList fld_list; // realy const, do not modify
				int    r = 0;
				if(Filt.Grp == CCheckFilt::gGoodsDate)
					r = (tab_idx > 0) ? P_Ct->GetTab((tab_idx-1) / P_Ct->GetAggrCount(), &tab_id) : 1;
				ct_period = Filt.Period;
				if(r > 0) {
					if(Filt.CtKind == CCheckFilt::ctDate) {
						LDATE dt;
						dt.v = (ulong)tab_id;
						if(!checkdate(dt, 0))
							ct_period = Filt.Period;
						else
							ct_period.SetDate(dt);
					}
					CCheckFilt tmp_filt = Filt;
					PPIDArray  goods_list;

					tmp_filt.Grp = CCheckFilt::gNone;
					tmp_filt.CorrGoodsList.InitEmpty();
					tmp_filt.Period  = ct_period;
					if(Filt.Sgg != sggNone) {
						Gsl.GetGoodsBySubstID(hdr.GoodsID, &goods_list);
						tmp_filt.CorrGoodsList.Set(&goods_list);
					}
					else
						tmp_filt.GoodsID = hdr.GoodsID;
					if(!ViewCCheck(&tmp_filt, 0))
						ok = 0;
				}
			}
		}
		else if(Filt.Flags & CCheckFilt::fGoodsCorr) {
			TempCCheckGdsCorrTbl::Rec * p_gc_rec = (TempCCheckGdsCorrTbl::Rec *)pHdr;
			CCheckFilt  temp_flt;
			temp_flt.Copy(&Filt, 1);
			if(Filt.Sgg) {
				PPIDArray list1, list2;
				Gsl.GetGoodsBySubstID(p_gc_rec->Goods1ID, &list1);
				Gsl.GetGoodsBySubstID(p_gc_rec->Goods2ID, &list2);
				for(uint i = 0; i < list2.getCount(); i++)
					if(!list1.lsearch(list2.at(i)))
						list1.add(list2.at(i));
				temp_flt.CorrGoodsList.Set(&list1);
			}
			else {
				temp_flt.CorrGoodsList.Add(p_gc_rec->Goods1ID);
				temp_flt.CorrGoodsList.Add(p_gc_rec->Goods2ID);
			}
			temp_flt.Flags &= ~CCheckFilt::fGoodsCorr;
			if(temp_flt.CorrGoodsList.GetCount())
				ok = ViewCCheck(&temp_flt, 0);
		}
		else {
			const CCheckFilt::Grouping grp = Filt.Grp;
			PPID  temp_id = *(PPID *)pHdr;
			if(temp_id && P_TmpGrpTbl && oneof4(grp, CCheckFilt::gTime, CCheckFilt::gDate, CCheckFilt::gDayOfWeek, CCheckFilt::gDowNTime) ||
				oneof7(grp, CCheckFilt::gCash, CCheckFilt::gCashNode, CCheckFilt::gCard, CCheckFilt::gDscntPct, CCheckFilt::gDiv, CCheckFilt::gGuestCount, CCheckFilt::gTableNo) ||
				oneof6(grp, CCheckFilt::gGoods, CCheckFilt::gAmount, CCheckFilt::gQtty, CCheckFilt::gCashiers, CCheckFilt::gAgents, CCheckFilt::gLinesCount) ||
				oneof7(grp, CCheckFilt::gAgentsNHour, CCheckFilt::gGoodsDate, CCheckFilt::gAgentsNGoods, CCheckFilt::gCashiersNGoods,
					CCheckFilt::gDlvrAddr, CCheckFilt::gGoodsSCSer, CCheckFilt::gAgentGoodsSCSer)) {
				PPID   k = temp_id;
				if(P_TmpGrpTbl->search(0, &k, spEq)) {
					int  to_view = 1;
					TempCCheckGrpTbl::Rec cur_rec;
					P_TmpGrpTbl->copyBufTo(&cur_rec);
					CCheckFilt tmp_filt = Filt;
					tmp_filt.Grp = CCheckFilt::gNone;
					tmp_filt.CorrGoodsList.InitEmpty();
					if(Filt.Grp == CCheckFilt::gTime)
						tmp_filt.HourBefore = cur_rec.Tm.hour() + 1;
					else if(Filt.Grp == CCheckFilt::gDate)
						tmp_filt.Period.SetDate(cur_rec.Dt);
					else if(Filt.Grp == CCheckFilt::gDayOfWeek)
						tmp_filt.WeekDays = 1 << (cur_rec.Dt.day() % 7);
					else if(Filt.Grp == CCheckFilt::gDowNTime) {
						tmp_filt.WeekDays = 1 << (cur_rec.Dt.day() % 7);
						tmp_filt.HourBefore = cur_rec.Tm.hour() + 1;
					}
					else if(Filt.Grp == CCheckFilt::gCash)
						tmp_filt.CashNumber = cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gCashNode)
						tmp_filt.NodeList.Add(cur_rec.CashID);
					else if(Filt.Grp == CCheckFilt::gCard)
						tmp_filt.SCardID = cur_rec.SCardID;
					else if(Filt.Grp == CCheckFilt::gDscntPct) {
						tmp_filt.PcntR.SetDelta(R2((cur_rec.CashID - 1) * 0.25), 0.25);
						tmp_filt.PcntR.low += 0.00001;
					}
					else if(Filt.Grp == CCheckFilt::gAmount)
						tmp_filt.AmtR.SetDelta(R2(cur_rec.CashID * Filt.AmountQuant), Filt.AmountQuant-0.01);
					else if(Filt.Grp == CCheckFilt::gQtty)
						tmp_filt.QttyR.SetDelta(R3(cur_rec.CashID * Filt.AmountQuant), Filt.AmountQuant-0.001);
					else if(Filt.HasGoodsGrouping()) {
						if(Filt.Sgg != sggNone) {
							PPIDArray goods_list;
							Gsl.GetGoodsBySubstID(cur_rec.GoodsID, &goods_list);
							tmp_filt.CorrGoodsList.Set(&goods_list);
						}
						else {
							if(!(State & stUseGoodsList) && (!oneof3(Filt.Grp, CCheckFilt::gGoodsDate,
								CCheckFilt::gAgentsNGoods, CCheckFilt::gCashiersNGoods)) &&
								(!Filt.AmtR.IsZero() || !Filt.QttyR.IsZero() || !Filt.PcntR.IsZero()))
								tmp_filt.Flags |= CCheckFilt::fFiltByCheck;
							tmp_filt.GoodsID = cur_rec.GoodsID;
						}
						if(Filt.Grp == CCheckFilt::gGoodsDate) {
							BrwHdr * p_hdr = (BrwHdr*)pHdr;
							tmp_filt.Period.SetDate(p_hdr->Dt);
						}
						else if(Filt.Grp == CCheckFilt::gAgentsNGoods)
							tmp_filt.AgentID = cur_rec.CashID;
						else if(Filt.Grp == CCheckFilt::gCashiersNGoods)
							tmp_filt.CashierID = cur_rec.CashID;
						else if(Filt.Grp == CCheckFilt::gGoodsSCSer)
							tmp_filt.SCardSerID = cur_rec.CashID;
						// @v9.6.6 {
						else if(Filt.Grp == CCheckFilt::gAgentGoodsSCSer) {
							tmp_filt.SCardSerID = cur_rec.SCardID;
							tmp_filt.AgentID = cur_rec.CashID;
						}
						// } @v9.6.6
					}
					else if(Filt.Grp == CCheckFilt::gGuestCount)
						tmp_filt.GuestCount = (int16)cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gTableNo)
						tmp_filt.TableCode = (int16)cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gDiv)
						tmp_filt.Div = (int16)cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gCashiers)
						tmp_filt.CashierID = cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gAgents)
						tmp_filt.AgentID = cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gLinesCount)
						tmp_filt.LowLinesCount = tmp_filt.UppLinesCount = (uint16)cur_rec.CashID;
					else if(Filt.Grp == CCheckFilt::gAgentsNHour) {
						tmp_filt.AgentID = cur_rec.CashID;
						tmp_filt.HourBefore = cur_rec.Tm.hour() + 1;
					}
					else if(Filt.Grp == CCheckFilt::gDlvrAddr) {
						if(cur_rec.CashID) {
							tmp_filt.DlvrAddrID = cur_rec.CashID;
							tmp_filt.Flags &= ~CCheckFilt::fZeroDlvrAddr;
						}
						else {
							tmp_filt.DlvrAddrID = 0;
							tmp_filt.Flags |= CCheckFilt::fZeroDlvrAddr;
						}
					}
					if(to_view && !ViewCCheck(&tmp_filt, 0))
						ok = 0;
				}
			}
		}
	}
	return ok;
}

// static
int SLAPI PPViewCCheck::CreateDraftBySuspCheck(PPViewCCheck * pV, PPID chkID)
{
	int    ok = -1;
	const PPEquipConfig & r_eq_cfg = pV->CsObj.GetEqCfg();
	if(chkID && !pV->Filt.Grp && r_eq_cfg.OpOnTempSess && GetOpType(r_eq_cfg.OpOnTempSess) == PPOPT_DRAFTEXPEND) {
		uint  all_selection = chkID ? 0 : 1;
		if(!chkID || SelectorDialog(DLG_SELDRAFTBYCHK, CTL_SELDRAFTBYCHK_SEL, &all_selection) > 0) {
			PPID    loc_id = LConfig.Location;
			PPID    ar_id = 0;
			SString bill_memo, temp_buf;
			CCheckViewItem chk_rec;
			PPBillPacket  pack;
			BillFilt    b_filt;
			PPObjBill * p_bobj = BillObj;
			THROW(p_bobj->CheckRights(PPR_INS));
			if(all_selection) {
				PPLoadText(PPTXT_CCHECKSELFORPERIOD, temp_buf);
				b_filt.Period.SetDate(pV->Filt.Period.low);
				bill_memo.Cat(temp_buf).Space().Cat(pV->Filt.Period);
				pV->InitIteration(0);
			}
			else {
				CSessionTbl::Rec cs_rec;
				THROW(pV->P_CC->Search(chkID, &chk_rec) > 0);
				b_filt.Period.SetDate(chk_rec.Dt);
				if(pV->CsObj.Search(chk_rec.SessID, &cs_rec) > 0 && cs_rec.CashNodeID) {
					PPObjCashNode cn_obj;
					PPCashNode cn_rec;
					if(cn_obj.Search(cs_rec.CashNodeID, &cn_rec) > 0 && cn_rec.LocID)
						loc_id = cn_rec.LocID;
				}
				if(chk_rec.SCardID) {
					SCardTbl::Rec  sc_rec;
					if(pV->P_CC->Cards.Search(chk_rec.SCardID, &sc_rec) > 0 && sc_rec.PersonID) {
						PPOprKind  op_rec;
						if(GetOpData(r_eq_cfg.OpOnTempSess, &op_rec) > 0) {
							PPObjArticle ar_obj;
							ar_obj.GetByPerson(op_rec.AccSheetID, sc_rec.PersonID, &ar_id);
						}
					}
				}
				pV->P_CC->MakeCodeString(&chk_rec, bill_memo);
			}
			b_filt.LocList.Add(loc_id);
			THROW(pack.CreateBlankByFilt(r_eq_cfg.OpOnTempSess, &b_filt, 1));
			while(!all_selection || pV->NextIteration(&chk_rec) > 0) {
				CCheckLineTbl::Rec cc_line;
				MEMSZERO(cc_line);
				if(diffdate(chk_rec.Dt, pack.Rec.Dt) > 0)
					pack.Rec.Dt = chk_rec.Dt;
				for(int i = 0; pV->P_CC->EnumLines(chk_rec.ID, &i, &cc_line) > 0;) {
					ReceiptTbl::Rec lot_rec;
					PPTransferItem ti;
					THROW(ti.Init(&pack.Rec));
					THROW(ti.SetupGoods(cc_line.GoodsID));
					ti.SetupLot(0, 0, 0);
					ti.Quantity_ = cc_line.Quantity;
					if(p_bobj->trfr->Rcpt.GetLastLot(ti.GoodsID, 0L, pack.Rec.Dt, &lot_rec) > 0) {
						ti.Cost  = R5(lot_rec.Cost);
						ti.QCert = lot_rec.QCertID;
						ti.UnitPerPack = lot_rec.UnitPerPack;
					}
					ti.Price = TR5(intmnytodbl(cc_line.Price) - cc_line.Dscnt);
					THROW(pack.InsertRow(&ti, 0));
				}
				if(!all_selection) {
					pack.Rec.Object  = ar_id;
					pack.Rec.SCardID = chk_rec.SCardID;
					break;
				}
			}
			pack.InitAmounts();
			bill_memo.CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
			THROW(p_bobj->FillTurnList(&pack));
			if(p_bobj->TurnPacket(&pack, 1) <= 0)
				p_bobj->DiagGoodsTurnError(&pack);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewCCheck::CalcVATData()
{
	int    ok = -1;
	SString msg_buf;
	CCheckViewItem view_item;
	ZDELETE(P_InOutVATList);
	THROW_MEM(P_InOutVATList = new BVATAccmArray(BVATF_SUMZEROVAT));
	PPLoadText(PPTXT_CALCVATBYCCHECKLIST, msg_buf);
	for(InitIteration(0); NextIteration(&view_item) > 0;) {
		CCheckLineTbl::Rec ln_rec;
		CcPack.ClearLines();
		P_CC->LoadLines(view_item.ID, 0, &CcPack);
		for(uint i = 0; CcPack.EnumLines(&i, &ln_rec);) {
			BVATAccm bva_item;
			PPGoodsTaxEntry gtx;
			bva_item.PTrnovr  = ln_rec.Quantity * (intmnytodbl(ln_rec.Price) - ln_rec.Dscnt);
			bva_item.Discount = ln_rec.Quantity * ln_rec.Dscnt;
			bva_item.PRate = (GdsObj.FetchTax(ln_rec.GoodsID, view_item.Dt, 0L, &gtx) > 0) ? gtx.GetVatRate() : 0.0;
			THROW(P_InOutVATList->Add(&bva_item, 1));
		}
		PPWaitPercent(GetCounter(), msg_buf);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewCCheck::GetReportId() const
{
	int    rpt_id = 0;
	if(P_TmpGdsCorrTbl)
		rpt_id = REPORT_CCHECKGDSCORR;
	else if(Filt.Grp == CCheckFilt::gNone) {
		if(DoProcessLines())
			rpt_id = REPORT_CCHECKVIEW_Q;
		else if(Filt.Flags & CCheckFilt::fPrintDetail)
			rpt_id = REPORT_CCHECKVIEWDETAIL;
		else
			rpt_id = REPORT_CCHECKVIEW;
	}
	else if(Filt.Grp == CCheckFilt::gDate)
		rpt_id = REPORT_CCHECKVIEW_G_DT;
	else if(oneof2(Filt.Grp, CCheckFilt::gQtty, CCheckFilt::gGoods))
		rpt_id = REPORT_CCHECKVIEW_G_Q;
	else if(Filt.Grp == CCheckFilt::gGoodsDate) {
		if(P_Ct)
			;// rpt_id = REPORT_CCHECKVIEW_G_CT;
		else
			rpt_id = REPORT_CCHECKVIEW_G_GDT;
	}
	else
		rpt_id = REPORT_CCHECKVIEW_G;
	return rpt_id;
}

int SLAPI PPViewCCheck::Print(const void *)
{
	uint   rpt_id = GetReportId();
	if(!P_TmpGdsCorrTbl && Filt.Grp == CCheckFilt::gNone) {
		PPWait(1);
		CalcVATData();
		PPWait(0);
	}
	return Helper_Print(rpt_id, 0);
}

int SLAPI ViewCCheck(const CCheckFilt * pFilt, int exeFlags) { return PPView::Execute(PPVIEW_CCHECK, pFilt, exeFlags, 0); }
//
// Implementation of PPALDD_CCheck
//
PPALDD_CONSTRUCTOR(CCheck)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new CCheckCore;
	}
}

PPALDD_DESTRUCTOR(CCheck)
{
	Destroy();
	delete (CCheckCore *)Extra[0].Ptr;
}

int PPALDD_CCheck::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		PPObjSCard scard_obj;
		SCardTbl::Rec scard_rec;
		MEMSZERO(scard_rec);
		MEMSZERO(H);
		H.ID = rFilt.ID;
		CCheckPacket pack;
		CCheckCore & r_cc = *(CCheckCore*)Extra[0].Ptr;
		if(r_cc.LoadPacket(rFilt.ID, CCheckCore::lpfNoLines, &pack) > 0) {
			H.ID       = pack.Rec.ID;
			H.Code     = pack.Rec.Code;
			H.CashID   = pack.Rec.CashID;
			H.UserID   = pack.Rec.UserID;
			H.SessID   = pack.Rec.SessID;
			H.Flags    = pack.Rec.Flags;
			H.fPrinted     = BIN(pack.Rec.Flags & CCHKF_PRINTED);
			H.fBanking     = BIN(pack.Rec.Flags & CCHKF_BANKING);
			H.fIncorpCard  = BIN(pack.Rec.Flags & CCHKF_INCORPCRD);
			H.fSuspended   = BIN(pack.Rec.Flags & CCHKF_SUSPENDED);
			H.fOrder       = BIN(pack.Rec.Flags & CCHKF_ORDER);
			H.fDelivery    = BIN(pack.Rec.Flags & CCHKF_DELIVERY);
			H.fClosedOrder = BIN(pack.Rec.Flags & CCHKF_CLOSEDORDER);
			H.fReturn      = BIN(pack.Rec.Flags & CCHKF_RETURN);
			H.Dt       = pack.Rec.Dt;
			H.Tm       = pack.Rec.Tm;
			H.Amount   = MONEYTOLDBL(pack.Rec.Amount);
			H.Discount = MONEYTOLDBL(pack.Rec.Discount);
			if(pack.AL_Const().getCount()) {
				H.CashAmount = pack.AL_Const().Get(CCAMTTYP_CASH);
				H.BnkAmount  = pack.AL_Const().Get(CCAMTTYP_BANK);
				H.CCrdAmount = pack.AL_Const().Get(CCAMTTYP_CRDCARD);
			}
			else {
				if(pack.Rec.Flags & CCHKF_BANKING) {
					H.BnkAmount = H.Amount;
				}
				else if(pack.Rec.Flags & CCHKF_INCORPCRD) {
					H.CCrdAmount = H.Amount;
				}
				else
					H.CashAmount = H.Amount;
			}
			H.SCardID  = pack.Rec.SCardID;
			scard_obj.Search(pack.Rec.SCardID, &scard_rec);
			STRNSCPY(H.SCardCode, scard_rec.Code);
			if(pack.Rec.Flags & CCHKF_EXT) {
				SString temp_buf;
				H.AgentID = pack.Ext.SalerID;
				H.TableNo = pack.Ext.TableNo;
				H.GuestCount = pack.Ext.GuestCount;
				// @v9.0.4 H.AddCrdCardID = pack.Ext.AddCrdCardID;
				// @v9.0.4 H.AddCrdCardPaym = pack.Ext.AddCrdCardPaym;
				H.CreationDt   = pack.Ext.CreationDtm.d;
				H.CreationTm   = pack.Ext.CreationDtm.t;
				H.OrderStartDt = pack.Ext.StartOrdDtm.d;
				H.OrderStartTm = pack.Ext.StartOrdDtm.t;
				H.OrderEndDt = pack.Ext.EndOrdDtm.d;
				H.OrderEndTm = pack.Ext.EndOrdDtm.t;
				H.LinkCheckID = pack.Ext.LinkCheckID;
				H.DlvrLocID   = pack.Ext.AddrID;
				temp_buf.Z();
				if(H.OrderStartDt)
					temp_buf.Cat(H.OrderStartDt, DATF_DMY);
				if(H.OrderStartTm)
					temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.OrderStartTm, TIMF_HM);
				temp_buf.CopyTo(H.OrderStartTxt, sizeof(H.OrderStartTxt));
				//
				temp_buf.Z();
				if(H.OrderEndDt)
					temp_buf.Cat(H.OrderEndDt, DATF_DMY);
				if(H.OrderEndTm)
					temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.OrderEndTm, TIMF_HM);
				temp_buf.CopyTo(H.OrderEndTxt, sizeof(H.OrderEndTxt));
				temp_buf.Z();
				if(H.CreationDt)
					temp_buf.Cat(H.CreationDt, DATF_DMY);
				if(H.CreationTm)
					temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.CreationTm, TIMF_HM);
				temp_buf.CopyTo(H.CreationTxt, sizeof(H.CreationTxt));
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_CCheckView
//
PPALDD_CONSTRUCTOR(CCheckView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(CCheckView) { Destroy(); }

static void SetVATData(double * pVATRate, double * pPTrnovr, double * pDscnt, const BVATAccmArray * pInOutList, uint i)
{
	double  vat_rate = 0.0, trnovr = 0.0, dscnt = 0.0;
	if(pInOutList && i >= 0 && pInOutList->getCount() > i) {
		const BVATAccm & r_item = pInOutList->at(i);
		vat_rate = r_item.PRate;
		trnovr = r_item.PTrnovr;
		dscnt  = r_item.Discount;
	}
	ASSIGN_PTR(pVATRate, vat_rate);
	ASSIGN_PTR(pPTrnovr, trnovr);
	ASSIGN_PTR(pDscnt,   dscnt);
}

int PPALDD_CCheckView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(CCheck, rsrv);
	H.FltCashNumber  = p_filt->CashNumber;
	H.FltCashNodeID  = p_filt->NodeList.GetSingle();
	H.FltGoodsGrpID  = p_filt->GoodsGrpID;
	H.FltGoodsID     = p_filt->GoodsID;
	H.FltSCardID     = p_filt->SCardID;
	H.FltGrp         = p_filt->Grp;
	H.FltTableNo     = (int16)p_filt->TableCode;
	H.FltAgentID     = p_filt->AgentID;
	H.AmountQuant    = p_filt->AmountQuant;
	H.FltBeg  = p_filt->Period.low;
	H.FltEnd  = p_filt->Period.upp;
	H.FltFlags = p_filt->Flags;
	H.fSuspendedOnly = BIN(p_filt->Flags & CCheckFilt::fSuspendedOnly);
	H.fGiftOnly      = BIN(p_filt->Flags & CCheckFilt::fGiftOnly);
	H.fOrderOnly     = BIN(p_filt->Flags & CCheckFilt::fOrderOnly);
	H.fDeliveryOnly  = BIN(p_filt->Flags & CCheckFilt::fDlvrOnly);
	const BVATAccmArray * p_inout_vat_list = p_v->GetInOutVATList();
	SetVATData(&H.VATRate1, &H.PTrnovr1, &H.Discount1, p_inout_vat_list, 0);
	SetVATData(&H.VATRate2, &H.PTrnovr2, &H.Discount2, p_inout_vat_list, 1);
	SetVATData(&H.VATRate3, &H.PTrnovr3, &H.Discount3, p_inout_vat_list, 2);
	SetVATData(&H.VATRate4, &H.PTrnovr4, &H.Discount4, p_inout_vat_list, 3);
	SetVATData(&H.VATRate5, &H.PTrnovr5, &H.Discount5, p_inout_vat_list, 4);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CCheckView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER_ORD(CCheck, 0);
}

int PPALDD_CCheckView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(CCheck);
	I.CheckID   = item.ID;
	I.Amount    = MONEYTOLDBL(item.Amount);
	I.Discount  = MONEYTOLDBL(item.Discount);
	I.CashAmount = I.Amount - item.BnkAmt - item.CrdCardAmt;
	I.BnkAmount = item.BnkAmt;
	I.CCrdAmount = item.CrdCardAmt;
	I.Goods1ID  = item.G_GoodsID;
	I.Goods2ID  = item.G_Goods2ID;
	I.GCount    = item.G_Count;
	I.GDate     = item.Dt;
	I.GPctPart  = item.G_PctPart;
	I.GAmount   = item.G_Amount;
	I.GDiscount = item.G_Discount;
	I.GQtty     = item.G_Qtty;
	I.GSCardID  = 0;
	I.CcAgentID = item.AgentID; // @v9.3.0
	const int _grp = ((CCheckFilt *)p_v->GetBaseFilt())->Grp;
	if(_grp == CCheckFilt::gCard)
		I.GSCardID = item.SCardID;
	else if(_grp == CCheckFilt::gAgentGoodsSCSer) // @v9.6.6
		I.GSCardID = item.SCardID;
	STRNSCPY(I.GrpngItemText, item.G_Text);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_CCheckView::Destroy() { DESTROY_PPVIEW_ALDD(CCheck); }
//
// Implementation of PPALDD_CCheckViewDetail
//
PPALDD_CONSTRUCTOR(CCheckViewDetail)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(CCheckViewDetail) { Destroy(); }
void PPALDD_CCheckViewDetail::Destroy() { DESTROY_PPVIEW_ALDD(CCheck); }

int PPALDD_CCheckViewDetail::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(CCheck, rsrv);
	H.FltBeg  = p_filt->Period.low;
	H.FltEnd  = p_filt->Period.upp;
	H.FltFlags = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CCheckViewDetail::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER_ORD(CCheck, CCheckFilt::ordIterLines);
}

int PPALDD_CCheckViewDetail::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(CCheck);
    I.CheckID = item.ID;
    I.Code    = item.Code;
    I.UserID  = item.UserID;
    I.SessID  = item.SessID;
    I.SCardID = item.SCardID;
    I.Dt      = item.Dt;
    I.Tm      = item.Tm;
    I.MachineN = item.CashID;
    I.CcFlags = item.Flags;
    I.LinesCount = item.LinesCount;
    I.CcAmount = MONEYTOLDBL(item.Amount);
    I.CcDiscount = MONEYTOLDBL(item.Discount);

	I.GoodsID = item.G_GoodsID;
    I.RByCheck = (int16)item.RByCheck;
    I.LineQueue = (int16)item.LineQueue;
	I.lfPrinted       = BIN(item.LineFlags & cifIsPrinted);
	I.lfGift          = BIN(item.LineFlags & cifGift);
	I.lfUsedByGift    = BIN(item.LineFlags & cifUsedByGift);
	I.lfQuotedByGift  = BIN(item.LineFlags & cifQuotedByGift);
	I.lfPartOfComplex = BIN(item.LineFlags & cifPartOfComplex);
	I.lfModifier      = BIN(item.LineFlags & cifModifier);
	I.LnFlags     = item.LineFlags;
	I.LnQuantity  = item.G_Qtty;
	I.LnPrice     = item.G_Price;
	I.LnDiscount  = item.G_Discount;
	I.LnAmount    = item.G_Amount;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}
//
// Implementation of PPALDD_CCheckDetail
//
PPALDD_CONSTRUCTOR(CCheckDetail)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(CCheckDetail) { Destroy(); }

int PPALDD_CCheckDetail::InitData(PPFilt & rFilt, long rsrv)
{
	SString temp_buf;
	CPosProcessor * p_cpp = (CPosProcessor *)rFilt.Ptr;
	Extra[1].Ptr = p_cpp;
	CCheckPacket pack;
	p_cpp->GetCheckInfo(&pack);
	H.CashNodeID    = pack.Rec.CashID;
	H.CheckID       = pack.Rec.ID;
	H.LinkCheckID   = pack.Ext.LinkCheckID;
	H.DlvrLocID     = pack.Ext.AddrID;
	H.CheckNumber   = pack.Rec.Code;
	H.CheckDt       = pack.Rec.Dt;
	H.CheckTm       = pack.Rec.Tm;
	H.CheckAmount   = MONEYTOLDBL(pack.Rec.Amount);
	H.CheckDiscount = MONEYTOLDBL(pack.Rec.Discount);
	H.OrderPrepay   = pack._OrdPrepay;
	{
		PosPaymentBlock ppb(0, 0);
		p_cpp->CalculatePaymentList(ppb, 0);
		H.UsableBonus = ppb.GetUsableBonus();
	}
	GetObjectName(PPOBJ_SCARD, pack.Rec.SCardID, H.CheckSCardCode, sizeof(H.CheckSCardCode));
	H.AgentID       = pack.Ext.SalerID;
	H.TableNo       = pack.Ext.TableNo;
	H.GuestCount    = pack.Ext.GuestCount;
	H.fPrinted      = BIN(pack.Rec.Flags & CCHKF_PRINTED);
	H.fBanking      = BIN(pack.Rec.Flags & CCHKF_BANKING);
	H.fIncorpCard   = BIN(pack.Rec.Flags & CCHKF_INCORPCRD);
	H.fSuspended    = BIN(pack.Rec.Flags & CCHKF_SUSPENDED);
	H.fOrder        = BIN(pack.Rec.Flags & CCHKF_ORDER);
	H.fDelivery     = BIN(pack.Rec.Flags & CCHKF_DELIVERY);
	H.fClosedOrder  = BIN(pack.Rec.Flags & CCHKF_CLOSEDORDER);
	H.fReturn       = BIN(pack.Rec.Flags & CCHKF_RETURN);

	H.CreationDt = pack.Ext.CreationDtm.d;
	H.CreationTm = pack.Ext.CreationDtm.t;

	H.OrderStartDt  = pack.Ext.StartOrdDtm.d;
	H.OrderStartTm  = pack.Ext.StartOrdDtm.t;
	H.OrderEndDt    = pack.Ext.EndOrdDtm.d;
	H.OrderEndTm    = pack.Ext.EndOrdDtm.t;

	temp_buf.Z();
	if(H.OrderStartDt)
		temp_buf.Cat(H.OrderStartDt, DATF_DMY);
	if(H.OrderStartTm)
		temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.OrderStartTm, TIMF_HM);
	temp_buf.CopyTo(H.OrderStartTxt, sizeof(H.OrderStartTxt));
	//
	temp_buf.Z();
	if(H.OrderEndDt)
		temp_buf.Cat(H.OrderEndDt, DATF_DMY);
	if(H.OrderEndTm)
		temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.OrderEndTm, TIMF_HM);
	temp_buf.CopyTo(H.OrderEndTxt, sizeof(H.OrderEndTxt));
	temp_buf.Z();
	if(H.CreationDt)
		temp_buf.Cat(H.CreationDt, DATF_DMY);
	if(H.CreationTm)
		temp_buf.CatDivIfNotEmpty(0, 1).Cat(H.CreationTm, TIMF_HM);
	temp_buf.CopyTo(H.CreationTxt, sizeof(H.CreationTxt));
	if(pack.Rec.Flags & CCHKF_DELIVERY) {
		LocationTbl::Rec loc_rec;
		const LocationTbl::Rec * p_addr = pack.GetDlvrAddr();
		if(!p_addr && pack.Ext.AddrID) {
			PPObjLocation loc_obj;
			if(loc_obj.Search(pack.Ext.AddrID, &loc_rec) > 0)
				p_addr = &loc_rec;
		}
		if(p_addr) {
			LocationCore::GetExField(p_addr, LOCEXSTR_SHORTADDR, temp_buf);
			temp_buf.CopyTo(H.DlvrAddr, sizeof(H.DlvrAddr));
			LocationCore::GetExField(p_addr, LOCEXSTR_PHONE, temp_buf);
			temp_buf.CopyTo(H.DlvrPhone, sizeof(H.DlvrPhone));
			LocationCore::GetExField(p_addr, LOCEXSTR_CONTACT, temp_buf);
			temp_buf.CopyTo(H.DlvrContact, sizeof(H.DlvrContact));
		}
	}
	pack.MakeBarcodeIdent(temp_buf).CopyTo(H.OrderCode, sizeof(H.OrderCode));
	STRNSCPY(H.Memo, pack.Ext.Memo);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CCheckDetail::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	CPosProcessor * p_cpp = (CPosProcessor *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return p_cpp->InitIteration();
}

int PPALDD_CCheckDetail::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	CPosProcessor * p_cpp = (CPosProcessor *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	CCheckItem item;
	if(p_cpp->NextIteration(&item) > 0) {
		I.LineGrpN = item.LineGrpN;
		I.LineQueue = item.Queue;
		I.fPrinted       = BIN(item.Flags & cifIsPrinted);
		I.fGift          = BIN(item.Flags & cifGift);
		I.fUsedByGift    = BIN(item.Flags & cifUsedByGift);
		I.fQuotedByGift  = BIN(item.Flags & cifQuotedByGift);
		I.fPartOfComplex = BIN(item.Flags & cifPartOfComplex);
		I.fModifier      = BIN(item.Flags & cifModifier);
		I.GoodsID  = item.GoodsID;
		I.Price    = item.Price;
		I.Discount = item.Discount;
		I.Quantity = item.Quantity;
		I.Amount   = item.Quantity * item.Price;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_CCheckDetail::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))
	#define _RET_STR     (**(SString **)rS.GetPtr(pApl->Get(0)))

	SString temp_buf;
	if(pF->Name == "?GetPrefixedCode") {
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		_RET_STR.Z().Cat(eq_cfg.SuspCcPrefix).CatLongZ(H.CheckNumber, 6);
	}
	else if(pF->Name == "?GetPrintablePrefixedCode") {
		char   code_buf[128];
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		temp_buf.Cat(eq_cfg.SuspCcPrefix).CatLongZ(H.CheckNumber, 6);
		CreatePrintableBarcode(temp_buf, 39, code_buf, sizeof(code_buf));
		_RET_STR = code_buf;
	}
	else if(pF->Name == "?CTableName") {
		PPObjCashNode::GetCafeTableName(H.TableNo, temp_buf);
		_RET_STR = temp_buf;
	}
}

void PPALDD_CCheckDetail::Destroy()
{
 	delete (CheckPaneDialog *)Extra[0].Ptr;
 	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_CCheckPacket
//
PPALDD_CONSTRUCTOR(CCheckPacket)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(CCheckPacket) { Destroy(); }

int PPALDD_CCheckPacket::InitData(PPFilt & rFilt, long rsrv)
{
	SString temp_buf;
	CCheckPacket * p_pack = (CCheckPacket *)rFilt.Ptr;
	Extra[1].Ptr = p_pack;
	H.PosNodeID     = p_pack->Rec.CashID;
	H.CheckID       = p_pack->Rec.ID;
	H.LinkCheckID   = p_pack->Ext.LinkCheckID;
	H.SCardID       = p_pack->Rec.SCardID;
	H.CheckNumber   = p_pack->Rec.Code;
	H.CheckDt       = p_pack->Rec.Dt;
	H.CheckTm       = p_pack->Rec.Tm;
	H.CheckAmount   = MONEYTOLDBL(p_pack->Rec.Amount);
	H.CheckDiscount = MONEYTOLDBL(p_pack->Rec.Discount);
	H.AgentID       = p_pack->Ext.SalerID;
	H.TableNo       = p_pack->Ext.TableNo;
	H.GuestCount    = p_pack->Ext.GuestCount;
	H.fOrder        = BIN(p_pack->Rec.Flags & CCHKF_ORDER);
	H.OrderStartDt  = p_pack->Ext.StartOrdDtm.d;
	H.OrderStartTm  = p_pack->Ext.StartOrdDtm.t;
	H.OrderEndDt    = p_pack->Ext.EndOrdDtm.d;
	H.OrderEndTm    = p_pack->Ext.EndOrdDtm.t;
	temp_buf.Z().Cat(p_pack->Ext.StartOrdDtm.t).CopyTo(H.OrderStartTmText, sizeof(H.OrderStartTmText));
	temp_buf.Z().Cat(p_pack->Ext.EndOrdDtm.t).CopyTo(H.OrderEndTmText, sizeof(H.OrderEndTmText));
	STRNSCPY(H.Memo, p_pack->Ext.Memo);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CCheckPacket::InitIteration(long iterId, int sortId, long rsrv)
{
	CCheckPacket * p_pack = (CCheckPacket *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(p_pack) {
		if(sortId >= 0)
			SortIdx = sortId;
		p_pack->InitLineIteration();
		return 1;
	}
	else
		return 0;
}

int PPALDD_CCheckPacket::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	CCheckPacket * p_pack = (CCheckPacket *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	if(p_pack) {
		CCheckLineTbl::Rec item;
		SString serial;
		if(p_pack->NextLineIteration(&item, &serial) > 0) {
			I.GoodsID  = item.GoodsID;
			I.Quantity = item.Quantity;
			I.Price    = intmnytodbl(item.Price);
			I.Discount = item.Dscnt;
			I.Amount   = intmnytodbl(item.Price) * item.Quantity;
			serial.CopyTo(I.Serial, sizeof(I.Serial));
			ok = DlRtm::NextIteration(iterId);
		}
	}
	return ok;
}

void PPALDD_CCheckPacket::Destroy()
{
}
