// QUOT.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop

int SLAPI ViewQuotValueInfo(const PPQuot & rQuot)
{
	int    ok = -1;
	SString temp_buf;
	TDialog * dlg = new TDialog(DLG_QUOT);
	THROW(CheckDialogPtr(&dlg, 0));
	GetObjectName(PPOBJ_QUOTKIND, rQuot.Kind, temp_buf);
	dlg->setCtrlString(CTL_QUOT_KIND, temp_buf);
	dlg->setCtrlString(CTL_QUOT_GOODS, GetGoodsName(rQuot.GoodsID, temp_buf));

	GetLocationName(rQuot.LocID, temp_buf);
	dlg->setCtrlString(CTL_QUOT_LOC, temp_buf);

	GetArticleName(rQuot.ArID, temp_buf);
	dlg->setCtrlString(CTL_QUOT_AR, temp_buf);

	PPFormatPeriod(&rQuot.Period, temp_buf);
	dlg->setCtrlString(CTL_QUOT_PERIOD, temp_buf);

	(temp_buf = 0).Cat(rQuot.Dtm, DATF_DMY, TIMF_HMS);
	dlg->setCtrlString(CTL_QUOT_DATETIME, temp_buf);

	(temp_buf = 0).Cat(rQuot.Quot, MKSFMTD(0, 6, NMBF_NOTRAILZ));
	dlg->setCtrlString(CTL_QUOT_VALUE, temp_buf);

	temp_buf = 0;
	if(rQuot.Flags & PPQuot::fPctOnCost)
		temp_buf.CatChar('C');
	if(rQuot.Flags & PPQuot::fPctOnPrice)
		temp_buf.CatChar('P');
	if(rQuot.Flags & PPQuot::fPctOnAddition)
		temp_buf.CatChar('D');
	if(rQuot.Flags & PPQuot::fPctDisabled)
		temp_buf.CatChar('X');
	if(rQuot.Flags & PPQuot::fPctOnBase)
		temp_buf.CatChar('Q');
	if(rQuot.Flags & PPQuot::fWithoutTaxes)
		temp_buf.CatChar('F');
	if(rQuot.Flags & PPQuot::fZero)
		temp_buf.CatChar('Z');
	dlg->setCtrlString(CTL_QUOT_FLAGS, temp_buf);
	ExecViewAndDestroy(dlg);
	CATCHZOKPPERR
	return ok;
}

#define GRP_LOC       1
#define GRP_ARTICLE   2
#define GRP_GOODS     3
#define GRP_GOODSFILT 4

class QuotUpdDialog : public TDialog {
public:
	QuotUpdDialog() : TDialog(DLG_QUOTUPD)
	{
		QuotCls = PPQuot::clsGeneral;
		PPObjQuotKind::GetSpecialKinds(&QkSpc, 1);
		addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_QUOTUPD_GGRP, cmGoodsFilt)); // @v7.9.11
		addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_QUOTUPD_GGRP, CTLSEL_QUOTUPD_GOODS));
		{
			PPIDArray ext_loc_list, * p_ext_loc_list = 0;
			if(CConfig.Flags2 & CCFLG2_QUOT2) {
				Quotation2Core qc2;
				if(qc2.GetAddressLocList(ext_loc_list) > 0) {
					p_ext_loc_list = &ext_loc_list;
				}
			}
			addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_QUOTUPD_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, p_ext_loc_list));
		}
		addGroup(GRP_ARTICLE, new ArticleCtrlGroup(0, 0, CTLSEL_QUOTUPD_AR, cmArList, GetSellAccSheet()));
	}
	int    setDTS(const QuotUpdFilt * pFilt);
	int    getDTS(QuotUpdFilt * pFilt);
private:
	DECL_HANDLE_EVENT;
	int    editAdvOptions();
	void   setupQuot();
	QuotUpdFilt Data;
	int    QuotCls; // PPQuot::clsXXX  (obsolete: 1 - котировки, 2 - контрактные цены, 3 - матрица)
	PPObjQuotKind::Special QkSpc;
	PPObjQuotKind QkObj;
	PPObjGoods GObj;
};

int QuotUpdDialog::setDTS(const QuotUpdFilt * pFilt)
{
	ushort v;
	PPID   acc_sheet_id = 0;
	long   qk_sel_extra = 1;
	PPID   new_qk_id = 0;
	SString temp_buf;

	Data = *pFilt;
	QkObj.Classify(Data.QuotKindID, &QuotCls);
	if(QuotCls == PPQuot::clsMtx) {
		qk_sel_extra = QuotKindFilt::fGoodsMatrix;
		new_qk_id = QkSpc.MtxID;
	}
	else if(QuotCls == PPQuot::clsSupplDeal) {
		acc_sheet_id = GetSupplAccSheet();
		qk_sel_extra = QuotKindFilt::fSupplDeal;
		new_qk_id = QkSpc.SupplDealID;
	}
	else if(QuotCls == PPQuot::clsPredictCoeff) {
		qk_sel_extra = QuotKindFilt::fPredictCoeff;
		new_qk_id = QkSpc.PredictCoeffID;
	}
	else {
		acc_sheet_id = GetSellAccSheet();
		qk_sel_extra = 1;
		new_qk_id = 0;
	}
	SETIFZ(Data.QuotKindID, new_qk_id);
	AddClusterAssoc(CTL_QUOTUPD_WHAT, 0, PPQuot::clsGeneral);
	AddClusterAssoc(CTL_QUOTUPD_WHAT, -1, PPQuot::clsGeneral);
	AddClusterAssoc(CTL_QUOTUPD_WHAT, 1, PPQuot::clsSupplDeal);
	AddClusterAssoc(CTL_QUOTUPD_WHAT, 2, PPQuot::clsMtx);
	AddClusterAssoc(CTL_QUOTUPD_WHAT, 3, PPQuot::clsPredictCoeff);
	SetClusterData(CTL_QUOTUPD_WHAT, QuotCls);
	SetupPPObjCombo(this, CTLSEL_QUOTUPD_KIND, PPOBJ_QUOTKIND, Data.QuotKindID, OLW_LOADDEFONOPEN, (void *)qk_sel_extra);
	{
		LocationCtrlGroup::Rec grp_rec(&Data.LocList);
		setGroupData(GRP_LOC, &grp_rec);
	}
	{
		GoodsCtrlGroup::Rec ggrp_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODS, &ggrp_rec);
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODSFILT, &gf_rec);
	}
	{
		ArticleCtrlGroup::Rec ar_grp_rec(0, 0, &Data.ArList);
		ArticleCtrlGroup * p_ar_grp = (ArticleCtrlGroup *)getGroup(GRP_ARTICLE);
		p_ar_grp->SetAccSheet(acc_sheet_id);
		setGroupData(GRP_ARTICLE, &ar_grp_rec);
	}
	AddClusterAssoc(CTL_QUOTUPD_HOW, 0, QuotUpdFilt::byLots);
	AddClusterAssoc(CTL_QUOTUPD_HOW, -1, QuotUpdFilt::byLots);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 1, QuotUpdFilt::byLastReval);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 2, QuotUpdFilt::byPctVal);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 3, QuotUpdFilt::byAbsVal);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 4, QuotUpdFilt::byFormula);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 5, QuotUpdFilt::byAddedQuot);
	AddClusterAssoc(CTL_QUOTUPD_HOW, 6, QuotUpdFilt::byDelete);
	SetClusterData(CTL_QUOTUPD_HOW, Data.ByWhat);

	disableCtrl(CTL_QUOTUPD_PCT, !oneof3(Data.ByWhat, QuotUpdFilt::byAbsVal, QuotUpdFilt::byPctVal, QuotUpdFilt::byFormula));
	enableCommand(cmQuotUpdSetQuot, Data.ByWhat == QuotUpdFilt::byAbsVal);
	v = 0;
	if(Data.Flags & QuotUpdFilt::fExistOnly)
		v = 0;
	else if(Data.Flags & QuotUpdFilt::fSetupIfNotExists)
		v = 1;
	else if(Data.Flags & QuotUpdFilt::fNonExistOnly)
		v = 2;
	setCtrlData(CTL_QUOTUPD_NEXS, &v);
	AddClusterAssoc(CTL_QUOTUPD_WARN, 0, QuotUpdFilt::fWarnExistsAbsQuot);
	AddClusterAssoc(CTL_QUOTUPD_WARN, 1, QuotUpdFilt::fSkipNoDisGoods);
	AddClusterAssoc(CTL_QUOTUPD_WARN, 2, QuotUpdFilt::fSkipDatedQuot);
	SetClusterData(CTL_QUOTUPD_WARN, Data.Flags);
	if(Data.ByWhat == QuotUpdFilt::byFormula) {
		temp_buf = Data.Formula;
	}
	else {
		PPQuot quot;
		quot.Flags = Data.QuotFlags;
		quot.Quot = Data.QuotVal;
		quot.PutValToStr(temp_buf);
	}
	setCtrlString(CTL_QUOTUPD_PCT, temp_buf);
	// @v7.3.5 {
	(temp_buf = 0).Cat(Data.QuotValPeriod, 1);
	setStaticText(CTL_QUOTUPD_ST_VALEXT, temp_buf);
	// } @v7.3.5
	return 1;
}

int QuotUpdDialog::getDTS(QuotUpdFilt * pFilt)
{
	int    ok = 1;
	uint   sel = 0;
	ushort v = 0;
	SString temp_buf;
	QuotCls = GetClusterData(CTL_QUOTUPD_WHAT);
	getCtrlData(CTLSEL_QUOTUPD_KIND, &Data.QuotKindID);
	{
		LocationCtrlGroup::Rec grp_rec;
		getGroupData(GRP_LOC, &grp_rec);
		Data.LocList = grp_rec.LocList;
		// getCtrlData(CTLSEL_QUOTUPD_LOC,  &Data.LocID);
	}
	{
		// @v7.9.11 {
		GoodsFiltCtrlGroup::Rec gf_rec;
		THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		// } @v7.9.11
		{
			// @v7.3.8 getCtrlData(CTLSEL_QUOTUPD_GGRP, &Data.GoodsGrpID);
			GoodsCtrlGroup::Rec ggrp_rec;
			getGroupData(GRP_GOODS, &ggrp_rec);
			SETIFZ(Data.GoodsGrpID, ggrp_rec.GrpID); // @v7.9.11 Если была выбрана динамическая группа выше, то не переопределяем ее.
			Data.GoodsID    = ggrp_rec.GoodsID;
		}
	}
	{
		// @v7.3.5 {
		ArticleCtrlGroup::Rec ar_grp_rec;
		getGroupData(GRP_ARTICLE, &ar_grp_rec);
		Data.ArList = ar_grp_rec.ArList;
		Data.ArticleID = Data.ArList.GetSingle();
		// } @v7.3.5
		// @v7.3.5 getCtrlData(CTLSEL_QUOTUPD_AR,   &Data.ArticleID);
	}
	Data.ByWhat = GetClusterData(CTL_QUOTUPD_HOW);
	getCtrlData(CTL_QUOTUPD_NEXS, &(v = 0));
	Data.Flags &= ~(QuotUpdFilt::fExistOnly | QuotUpdFilt::fSetupIfNotExists | QuotUpdFilt::fNonExistOnly);
	if(v == 0)
		Data.Flags |= QuotUpdFilt::fExistOnly;
	else if(v == 1)
		Data.Flags |= QuotUpdFilt::fSetupIfNotExists;
	else if(v == 2)
		Data.Flags |= QuotUpdFilt::fNonExistOnly;
	else
		Data.Flags |= QuotUpdFilt::fExistOnly;
	GetClusterData(CTL_QUOTUPD_WARN, &Data.Flags);
	getCtrlString(sel = CTL_QUOTUPD_PCT, temp_buf = 0);
	if(Data.ByWhat == QuotUpdFilt::byFormula) {
		Data.Formula = temp_buf;
		{
			//
			// Тест формулы
			//
			double _val = 0.0;
			GoodsContext::Param gcp;
			gcp.GoodsID = 1;
			gcp.LocID = 1;
			gcp.ArID = 0;
			GoodsContext gctx(gcp);
			THROW(PPExprParser::CalcExpression(temp_buf, &_val, 0, &gctx));
		}
	}
	else {
		PPQuot quot;
		quot.GetValFromStr(temp_buf);
		Data.QuotFlags = quot.Flags;
		Data.QuotVal = quot.Quot;
	}
	if(Data.QuotKindID == 0 && !CONFIRM(PPCFM_UPDQUOT_NOQUOTKIND))
		ok = 0;
	if(Data.ByWhat == QuotUpdFilt::byDelete) {
		Data.Flags &= ~QuotUpdFilt::fWarnExistsAbsQuot;
		SETFLAG(Data.Flags, QuotUpdFilt::fExistOnly, 1);
		SETFLAG(Data.Flags, QuotUpdFilt::fSetupIfNotExists, 0);
		SETFLAG(Data.Flags, QuotUpdFilt::fNonExistOnly, 0);
		Data.QuotVal = 0;
	}
	if(ok)
		ASSIGN_PTR(pFilt, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

void QuotUpdDialog::setupQuot()
{
	Data.ByWhat = GetClusterData(CTL_QUOTUPD_HOW);
	if(Data.ByWhat == QuotUpdFilt::byAbsVal) {
		PPQuot quot;
		SString temp_buf;
		getCtrlString(CTL_QUOTUPD_PCT, temp_buf);
		quot.GetValFromStr(temp_buf);
		quot.Period = Data.QuotValPeriod;
		if(EditQuotVal(&quot, QuotCls) > 0) {
			Data.QuotValPeriod = quot.Period;
			setCtrlString(CTL_QUOTUPD_PCT, quot.PutValToStr(temp_buf));
			(temp_buf = 0).Cat(Data.QuotValPeriod, 1);
			setStaticText(CTL_QUOTUPD_ST_VALEXT, temp_buf);
		}
	}
}

IMPL_HANDLE_EVENT(QuotUpdDialog)
{
	if(event.isCmd(cmOK)) {
		Data.ByWhat = GetClusterData(CTL_QUOTUPD_HOW);
		if(Data.ByWhat == QuotUpdFilt::byDelete) {
			SString msg;
			if(!CONFIRMCRIT(PPCFM_DELQUOTSBYFILT))
				clearEvent(event);
		}
	}
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_QUOTUPD_WHAT)) {
		long   prev_cls = QuotCls;
		QuotCls = GetClusterData(CTL_QUOTUPD_WHAT);
		PPID   acc_sheet_id = 0;
		long   qk_sel_extra = 1;
		PPID   new_qk_id = 0;
		if(QuotCls == PPQuot::clsSupplDeal) {
			acc_sheet_id = GetSupplAccSheet();
			qk_sel_extra = QuotKindFilt::fSupplDeal;
			new_qk_id = QkSpc.SupplDealID;
		}
		else if(QuotCls == PPQuot::clsMtx) {
			qk_sel_extra = QuotKindFilt::fGoodsMatrix;
			new_qk_id = QkSpc.MtxID;
		}
		else if(QuotCls == PPQuot::clsPredictCoeff) {
			qk_sel_extra = QuotKindFilt::fPredictCoeff;
			new_qk_id = QkSpc.PredictCoeffID;
		}
		else {
			acc_sheet_id = GetSellAccSheet();
			qk_sel_extra = 1;
			new_qk_id = 0;
		}
		if(QuotCls != prev_cls) {
			Data.ArticleID = 0;
			Data.ArList.Set(0);
			Data.QuotKindID = new_qk_id;
			SetupPPObjCombo(this, CTLSEL_QUOTUPD_KIND, PPOBJ_QUOTKIND, Data.QuotKindID, 0, (void *)qk_sel_extra);
			{
				ArticleCtrlGroup::Rec ar_grp_rec(0, 0, &Data.ArList);
				ArticleCtrlGroup * p_ar_grp = (ArticleCtrlGroup *)getGroup(GRP_ARTICLE);
				p_ar_grp->SetAccSheet(acc_sheet_id);
				setGroupData(GRP_ARTICLE, &ar_grp_rec);
			}
		}
	}
	else if(event.isClusterClk(CTL_QUOTUPD_HOW)) {
		Data.ByWhat = GetClusterData(CTL_QUOTUPD_HOW);
		disableCtrl(CTL_QUOTUPD_PCT, !oneof3(Data.ByWhat, QuotUpdFilt::byAbsVal, QuotUpdFilt::byPctVal, QuotUpdFilt::byFormula));
		enableCommand(cmQuotUpdSetQuot, Data.ByWhat == QuotUpdFilt::byAbsVal);
	}
	else if(event.isCmd(cmAdvOptions)) {
		if(!editAdvOptions())
			PPError();
	}
	else if(event.isCmd(cmQuotUpdSetQuot))
		setupQuot();
	else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_QUOTUPD_PCT))
		setupQuot();
	else
		return;
	clearEvent(event);
}

int QuotUpdDialog::editAdvOptions()
{
	class QuotUpdAdvDialog : public TDialog {
	public:
		QuotUpdAdvDialog() : TDialog(DLG_QUOTUPDA)
		{
		}
		int setDTS(const QuotUpdFilt * pData)
		{
			Data = *pData;
			SetupPPObjCombo(this, CTLSEL_QUOTUPDA_KIND, PPOBJ_QUOTKIND, Data.AdvOptQuotKindID, 0, (void *)1);
			SetupPPObjCombo(this, CTLSEL_QUOTUPDA_LOC, PPOBJ_LOCATION, Data.AdvOptLocID, OLW_CANSELUPLEVEL);
			{
				PPIDArray op_type_list;
				op_type_list.add(PPOPT_DRAFTEXPEND);
				SetupOprKindCombo(this, CTLSEL_QUOTUPDA_OP, Data.RegisterOpID, 0, &op_type_list, 0);
			}
			setCtrlData(CTL_QUOTUPDA_QUOT,    &Data.AdvOptQuot);
			// @v9.1.0 {
			AddClusterAssoc(CTL_QUOTUPDA_FLAGS, 0, QuotUpdFilt::fSetupDatedSamples);
			SetClusterData(CTL_QUOTUPDA_FLAGS, Data.Flags);
			// } @v9.1.0
			SetupPPObjCombo(this, CTLSEL_QUOTUPDA_ACTION, PPOBJ_ACTION, 0, 0);
			if(Data.EventList.GetCount() > 1) {
				SetComboBoxListText(this, CTLSEL_QUOTUPDA_ACTION);
				disableCtrl(CTLSEL_QUOTUPDA_ACTION, 1);
			}
			else {
				setCtrlLong(CTLSEL_QUOTUPDA_ACTION, Data.EventList.GetSingle());
				disableCtrl(CTLSEL_QUOTUPDA_ACTION, 0);
			}
			SetupPPObjCombo(this, CTLSEL_QUOTUPDA_TOKEN, PPOBJ_EVENTTOKEN, Data.EvTokID, OLW_CANINSERT);
			return 1;
		}
		int getDTS(QuotUpdFilt * pData)
		{
			int    ok = 1;
			getCtrlData(CTL_QUOTUPDA_KIND, &Data.AdvOptQuotKindID);
			getCtrlData(CTL_QUOTUPDA_LOC,  &Data.AdvOptLocID);
			getCtrlData(CTL_QUOTUPDA_QUOT, &Data.AdvOptQuot);
			GetClusterData(CTL_QUOTUPDA_FLAGS, &Data.Flags); // @v9.1.0
			getCtrlData(CTLSEL_QUOTUPDA_OP, &Data.RegisterOpID);
			getCtrlData(CTLSEL_QUOTUPDA_TOKEN, &Data.EvTokID);
			if(Data.EventList.GetCount() <= 1) {
				PPID  _id = getCtrlLong(CTLSEL_QUOTUPDA_ACTION);
				Data.EventList.Set(0);
				if(_id)
					Data.EventList.Add(_id);
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmSysJActionList)) {
				PPIDArray temp_list;
				if(Data.EventList.IsExists())
					temp_list = Data.EventList.Get();
				ListToListData l2l_data(PPOBJ_ACTION, (void *)-1, &temp_list);
				l2l_data.TitleStrID = 0; // PPTXT_XXX;
				if(ListToListDialog(&l2l_data) > 0) {
					if(temp_list.getCount())
						Data.EventList.Set(&temp_list);
					else
						Data.EventList.Set(0);
					if(Data.EventList.GetCount() > 1) {
						SetComboBoxListText(this, CTLSEL_QUOTUPDA_ACTION);
						disableCtrl(CTLSEL_QUOTUPDA_ACTION, 1);
					}
					else {
						setCtrlLong(CTLSEL_QUOTUPDA_ACTION, temp_list.getSingle());
						disableCtrl(CTLSEL_QUOTUPDA_ACTION, 0);
					}
				}
				clearEvent(event);
			}
		}
		QuotUpdFilt Data;
	};
	DIALOG_PROC_BODY(QuotUpdAdvDialog, &Data);
}

int SLAPI EditQuotUpdDialog(QuotUpdFilt * pFilt)
{
	DIALOG_PROC_BODY(QuotUpdDialog, pFilt);
}
//
//
//
class QuotUpdFilt_v1 : public PPBaseFilt {
public:
	SLAPI  QuotUpdFilt_v1();
	char   ReserveStart[32]; // @anchor
	PPID   QuotKindID;       //
	PPID   GoodsGrpID;       //
	PPID   ArticleID;        //
	PPID   RegisterOpID;     //
	int    ByWhat;           //
	long   Flags;            //
	long   EvTokID;          //
	long   QuotFlags;        //
	double QuotVal;          //
	PPID   AdvOptQuotKindID; //
	PPID   AdvOptLocID;      //
	double AdvOptQuot;       //
	long   Reserve;          // @anchor Заглушка для отмера "плоского" участка фильтра
	ObjIdListFilt LocList;   //
	ObjIdListFilt EventList; //
	SString Formula;         //
};

SLAPI QuotUpdFilt_v1::QuotUpdFilt_v1() : PPBaseFilt(PPFILT_QUOTUPD, 0, 1)
{
	SetFlatChunk(offsetof(QuotUpdFilt_v1, ReserveStart),
		offsetof(QuotUpdFilt_v1, Reserve)-offsetof(QuotUpdFilt_v1, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(QuotUpdFilt_v1, LocList));
	SetBranchObjIdListFilt(offsetof(QuotUpdFilt_v1, EventList));
	SetBranchSString(offsetof(QuotUpdFilt_v1, Formula));
	Init(1, 0);
}


IMPLEMENT_PPFILT_FACTORY(QuotUpd); SLAPI QuotUpdFilt::QuotUpdFilt() : PPBaseFilt(PPFILT_QUOTUPD, 0, 2) // @v7.3.5 ver 1-->2
{
	SetFlatChunk(offsetof(QuotUpdFilt, ReserveStart),
		offsetof(QuotUpdFilt, Reserve)-offsetof(QuotUpdFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(QuotUpdFilt, LocList));
	SetBranchObjIdListFilt(offsetof(QuotUpdFilt, EventList));
	SetBranchObjIdListFilt(offsetof(QuotUpdFilt, ArList)); // @v7.3.5
	SetBranchSString(offsetof(QuotUpdFilt, Formula));
	Init(1, 0);
}

QuotUpdFilt & FASTCALL QuotUpdFilt::operator = (const QuotUpdFilt & s)
{
	Copy(&s, 1);
	return *this;
}

int FASTCALL QuotUpdFilt::IsQuotByAdvOptExists(const PPQuotArray * pList) const
{
	int    ok = -1;
	PPQuot * p_q = 0;
	for(uint i = 0; ok <= 0 && pList->enumItems(&i, (void**)&p_q);)
		if(oneof2(AdvOptQuotKindID, 0, p_q->Kind) && oneof2(AdvOptLocID, 0, p_q->LocID) &&
			(p_q->Quot == AdvOptQuot && !p_q->IsRelative()))
			ok = 1;
	return ok;
}

int FASTCALL QuotUpdFilt::IsQuotSuitesToAdvOpt(const PPQuot & rQ) const
{
	if(!AdvOptQuot || (oneof2(AdvOptQuotKindID, 0, rQ.Kind) &&
		oneof2(AdvOptLocID, 0, rQ.LocID) && (rQ.Quot == AdvOptQuot && !rQ.IsRelative())))
		return 1;
	else
		return 0;
}

/*
int SLAPI QuotUpdFilt::Write(SBuffer & rBuf, long) const
{
	PPIDArray loc_list;
	LocList.CopyTo(&loc_list);
	if(rBuf.Write(QuotKindID) &&
		rBuf.Write(GoodsGrpID) &&
		rBuf.Write(ArticleID) &&
		rBuf.Write(&ByWhat, sizeof(ByWhat)) &&
		rBuf.Write(Flags) &&
		rBuf.Write(QuotFlags) &&
		rBuf.Write(QuotVal) &&
		rBuf.Write((SArray*)&loc_list))
		return 1;
	else
		return PPSetErrorSLib();
}
*/

int SLAPI QuotUpdFilt::Read_Pre720(SBuffer & rBuf, long)
{
	PPIDArray loc_list;
	if(rBuf.Read(QuotKindID) &&
		rBuf.Read(GoodsGrpID) &&
		rBuf.Read(ArticleID) &&
		rBuf.Read(&ByWhat, sizeof(ByWhat)) &&
		rBuf.Read(Flags) &&
		rBuf.Read(QuotFlags) &&
		rBuf.Read(QuotVal) &&
		rBuf.Read((SArray*)&loc_list, 0)) {
		LocList.Set(&loc_list);
		return 1;
	}
	else
		return PPSetErrorSLib();
}

int SLAPI QuotUpdFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 1) {
		QuotUpdFilt_v1 fv1;
		THROW(fv1.Read(rBuf, 0));
#define CPYFLD(f) f = fv1.f
		CPYFLD(QuotKindID);
		CPYFLD(GoodsGrpID);
		CPYFLD(ArticleID);
		CPYFLD(RegisterOpID);
		CPYFLD(ByWhat);
		CPYFLD(Flags);
		CPYFLD(EvTokID);
		CPYFLD(QuotFlags);
		CPYFLD(QuotVal);
		CPYFLD(AdvOptQuotKindID);
		CPYFLD(AdvOptLocID);
		CPYFLD(AdvOptQuot);
		CPYFLD(Reserve);
		CPYFLD(LocList);
		CPYFLD(EventList);
		CPYFLD(Formula);
#undef CPYFLD
		ok = 1;
	}
	CATCHZOK
	return ok;
}

static int SLAPI WarnUpdQuot(PPID goodsID, const PPQuot * pOldQuot, const PPQuot * pNewQuot)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_W_UPDQUOT);
	if(CheckDialogPtr(&dlg, 1)) {
		ushort v = 0;
		SString temp_buf;
		dlg->setCtrlString(CTL_W_UPDQUOT_GOODS, GetGoodsName(goodsID, temp_buf));
		dlg->setCtrlString(CTL_W_UPDQUOT_OLDQUOT, pOldQuot->PutValToStr(temp_buf));
		dlg->setCtrlString(CTL_W_UPDQUOT_NEWQUOT, pNewQuot->PutValToStr(temp_buf));
		dlg->setCtrlData(CTL_W_UPDQUOT_ALLFLAGS, &(v = 0));
		int    r = ExecView(dlg);
		if(r == cmYes)
			ok = 1;
		else if(r == cmNo)
			ok = 2;
		else if(r == cmCancel)
			ok = -1;
		dlg->getCtrlData(CTL_W_UPDQUOT_ALLFLAGS, &(v = 0));
		if(ok > 0 && v & 0x01)
			ok += 100;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

static int SLAPI _SetQuot(PPQuotArray * pList, const QuotIdent & rQi, double v, long flags, long minQtty, const DateRange * pPeriod)
{
	SString prev_q_buf, new_q_buf;
    const  PPQuotArray preserve_list(*pList);
    int    ret = pList->SetQuot(rQi, v, flags, minQtty, pPeriod);
    if(ret > 0) {
    	SString fmt_buf, log_msg_buf;
        if(ret == (int)(preserve_list.getCount()+1)) {
            // "Добавлено новое значение котировки"
            const PPQuot & r_new_q = pList->at(ret-1);
            PPQuot::PutValToStr(r_new_q.Quot, r_new_q.Flags, r_new_q.Period, r_new_q.MinQtty, new_q_buf);
            PPFormatT(PPTXT_LOG_QUOTUPD_QINSERTED, &log_msg_buf, pList->GoodsID, new_q_buf.cptr());
        }
        else {
			// "Изменено значение котировки"
            const PPQuot & r_new_q = pList->at(ret-1);
            const PPQuot & r_prev_q = preserve_list.at(ret-1);
            PPQuot::PutValToStr(r_prev_q.Quot, r_prev_q.Flags, r_prev_q.Period, r_prev_q.MinQtty, prev_q_buf);
            PPQuot::PutValToStr(r_new_q.Quot, r_new_q.Flags, r_new_q.Period, r_new_q.MinQtty, new_q_buf);
			PPFormatT(PPTXT_LOG_QUOTUPD_QUPDATED, &log_msg_buf, pList->GoodsID, prev_q_buf.cptr(), new_q_buf.cptr());
        }
		PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
    }
    else {
		PPLogMessage(PPFILNAM_QUOTUPD_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
    }
    return ret;
}

static int SLAPI SetupQuotList(const QuotUpdFilt & rFilt, PPID locID, PPID goodsID, PPIDArray * pQuotKindList, PPQuotArray * pList, PPQuotArray * pUpdList, int * pForAll)
{
	int    ok = -1, r = 0;
	int    for_all = *pForAll;
	uint   i;
	PPQuot * p_quot;
	PPID * p_qk_id = 0;
	double price = 0.0;
	PPQuotArray * p_dest_list = NZOR(pUpdList, pList);
	LongArray qp_list;
	// @v7.3.5 {
	PPIDArray ar_list;
	for(i = 0; i < rFilt.ArList.GetCount(); i++) {
		ar_list.addUnique(rFilt.ArList.Get(i));
	}
	if(!ar_list.getCount())
		ar_list.add(0L);
	//
	DateRange _period = rFilt.QuotValPeriod;
	_period.Actualize(ZERODATE);
	const DateRange * p_period = _period.IsZero() ? 0 : &_period;
	// } @v7.3.5
	switch(rFilt.ByWhat) {
		case QuotUpdFilt::byLots: // Установка котировок по цене реализации в последнем лоте
			THROW(r = ::GetCurGoodsPrice(goodsID, locID, GPRET_MOSTRECENT, &price));
			if(oneof3(r, GPRET_PRESENT, GPRET_CLOSEDLOTS, GPRET_OTHERLOC)) {
				for(i = 0; pQuotKindList->enumItems(&i, (void **)&p_qk_id);) {
					for(uint j = 0; j < ar_list.getCount(); j++) {
						const  PPID    ar_id = ar_list.get(j);
						const  QuotIdent qi(locID, *p_qk_id, 0L /* @curID */, ar_id);
						const  int is_exist = BIN(pList->SearchQiList(qi, qp_list) > 0);
						assert(!is_exist || qp_list.getCount()); // Уверенность в корректности SearchQiList
						if(is_exist) {
							if(!(rFilt.Flags & QuotUpdFilt::fNonExistOnly)) {
								for(uint p = 0; p < qp_list.getCount(); p++) {
									const uint pos = (uint)qp_list.get(p);
									const PPQuot & r_ex_q = pList->at(pos);
									assert(r_ex_q.ArID == ar_id); // Тест инварианта
									if(rFilt.IsQuotSuitesToAdvOpt(r_ex_q) && !(rFilt.Flags & QuotUpdFilt::fSkipDatedQuot) || r_ex_q.Period.IsZero()) {
										QuotIdent temp_qi(r_ex_q);
										THROW(_SetQuot(p_dest_list, temp_qi, price, 0, r_ex_q.MinQtty, &r_ex_q.Period));
										ok = 1;
									}
								}
							}
						}
						else if(rFilt.Flags & (QuotUpdFilt::fExistOnly|QuotUpdFilt::fSetupIfNotExists)) {
							THROW(_SetQuot(p_dest_list, qi, price, 0, 0, p_period));
							ok = 1;
						}
					}
				}
			}
			break;
		case QuotUpdFilt::byFormula: // Установка котировок по формуле
		case QuotUpdFilt::byAbsVal: // Установка котировок по абсолютной величине
			for(i = 0; pQuotKindList->enumItems(&i, (void **)&p_qk_id);) {
				for(uint j = 0; j < ar_list.getCount(); j++) {
					//uint   pos = 0;
					const  PPID    ar_id = ar_list.get(j);
					QuotIdent qi(locID, *p_qk_id, 0L /* @curID */, ar_id);
					qi.SetIdentPeriod(p_period); // @v8.5.3
					const  int is_exist = BIN(pList->SearchQiList(qi, qp_list) > 0);
					assert(!is_exist || qp_list.getCount()); // Уверенность в корректности SearchQiList

					double qval = 0.0;
					long   qf = 0;
					if(rFilt.ByWhat == QuotUpdFilt::byFormula) {
						GoodsContext::Param gcp;
						gcp.GoodsID = goodsID;
						gcp.LocID = locID;
						gcp.ArID = ar_id;
						GoodsContext gctx(gcp);
						THROW(PPExprParser::CalcExpression(rFilt.Formula, &qval, 0, &gctx));
					}
					else {
						qval = rFilt.QuotVal;
						qf = rFilt.QuotFlags;
					}
					if(is_exist) {
						if(!(rFilt.Flags & QuotUpdFilt::fNonExistOnly)) {
							for(uint p = 0; p < qp_list.getCount(); p++) {
								const uint pos = (uint)qp_list.get(p);
								const PPQuot & r_ex_q = pList->at(pos);
								assert(r_ex_q.ArID == ar_id); // Тест инварианта
								if(rFilt.IsQuotSuitesToAdvOpt(r_ex_q)) {
									if(!(rFilt.Flags & QuotUpdFilt::fSkipDatedQuot) || r_ex_q.Period.IsZero() || r_ex_q.Period == _period) {
										int    skip = 0;
										if(rFilt.Flags & QuotUpdFilt::fWarnExistsAbsQuot && !r_ex_q.IsRelative()) {
											PPQuot new_quot(goodsID);
											new_quot.Quot  = qval;
											new_quot.Flags = qf;
											if(new_quot.IsRelative()) {
												int    r = NZOR(for_all, WarnUpdQuot(goodsID, &r_ex_q, &new_quot));
												THROW(r);
												THROW_PP(r > 0, PPERR_USERBREAK);
												if(r == 2 || r == 102) {
													if(r == 102)
														for_all = 2;
													skip = 1;
												}
												else if(r == 1 || r == 101) {
													if(r == 101)
														for_all = 1;
												}
											}
										}
										if(!skip) {
											QuotIdent temp_qi(r_ex_q);
											THROW(_SetQuot(p_dest_list, temp_qi, qval, qf, r_ex_q.MinQtty, &r_ex_q.Period));
											ok = 1;
										}
									}
								}
							}
						}
					}
					else {
						if(rFilt.Flags & (QuotUpdFilt::fExistOnly|QuotUpdFilt::fSetupIfNotExists)) {
							THROW(_SetQuot(p_dest_list, qi, qval, qf, 0, p_period));
							ok = 1;
						}
					}
					ok = 1;
				}
			}
			break;
		case QuotUpdFilt::byPctVal: // Установка котировок по относительному изменению (только для существующих)
			if(rFilt.QuotVal) {
				for(i = 0; pList->enumItems(&i, (void**)&p_quot);) {
					for(uint j = 0; j < ar_list.getCount(); j++) {
						const PPID ar_id = ar_list.get(j);
						if((!rFilt.QuotKindID || p_quot->Kind == rFilt.QuotKindID) && p_quot->LocID == locID &&
							p_quot->ArID == ar_id && !p_quot->IsRelative() && rFilt.IsQuotSuitesToAdvOpt(*p_quot)) {
							if(!(rFilt.Flags & QuotUpdFilt::fSkipDatedQuot) || p_quot->Period.IsZero()) {
								if(p_dest_list == pList) {
									p_quot->Quot = RoundUpPrice(p_quot->Quot * (1L + rFilt.QuotVal / 100L));
								}
								else {
									PPQuot upd_q = *p_quot;
									upd_q.Quot = RoundUpPrice(upd_q.Quot * (1L + rFilt.QuotVal / 100L));
									THROW_SL(p_dest_list->insert(&upd_q));
								}
								ok = 1;
							}
						}
					}
				}
			}
			break;
	}
	CATCHZOK
	ASSIGN_PTR(pForAll, for_all);
	return ok;
}

static int SLAPI SetupQuotUpdRegBill(const QuotUpdFilt & rF, const PPQuotArray & rQList, TSCollection <PPBillPacket> & rPackList)
{
	int    ok = -1;
	if(rF.RegisterOpID) {
		for(uint k = 0; k < rQList.getCount(); k++) {
			const PPQuot & r_q = rQList.at(k);
			if(r_q.Flags & PPQuot::fDbUpd && r_q.Kind == rF.QuotKindID && r_q.LocID) {
				PPBillPacket * p_reg_pack = 0;
				for(uint j = 0; !p_reg_pack && j < rPackList.getCount(); j++) {
					if(rPackList.at(j)->Rec.LocID == r_q.LocID)
						p_reg_pack = rPackList.at(j);
				}
				if(!p_reg_pack) {
					THROW_MEM(p_reg_pack = new PPBillPacket);
					THROW(p_reg_pack->CreateBlank2(rF.RegisterOpID, ZERODATE, r_q.LocID, 0));
					rPackList.insert(p_reg_pack);
				}
				PPTransferItem reg_ti(&p_reg_pack->Rec, TISIGN_UNDEF);
				reg_ti.GoodsID = r_q.GoodsID;
				reg_ti.Price = r_q.Quot;
				reg_ti.Quantity_ = 1.0; // @v7.3.12
				THROW(p_reg_pack->InsertRow(&reg_ti, 0, 0));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

static int SLAPI FinishUpdateQuots(TSCollection <PPBillPacket> & rRegPackList, int use_ta)
{
	const uint max_tries = 3;

	int    ok = 1;
	const uint _c = rRegPackList.getCount();
	for(uint j = 0; j < _c; j++) {
		PPBillPacket * p_pack = rRegPackList.at(j);
		int    local_ok = 0;
		uint   t = 0;
		while(!local_ok && t < max_tries) {
			local_ok = BillObj->__TurnPacket(p_pack, 0, 1, use_ta);
			t++;
		}
		if(!local_ok) {
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			ok = 100;
		}
		else if(t > 1) {
		}
	}
	return ok;
}

int SLAPI UpdateQuots(const QuotUpdFilt * pFilt)
{
	int    ok = -1, r = 0, for_all = 0;
	//PPLogger logger;
	SString log_msg_buf, temp_buf;
	PPObjQuotKind qk_obj;
	PPObjGoods goods_obj;
	PPObjLocation loc_obj;
	QuotUpdFilt flt;
	PPIDArray loc_list;
	THROW(qk_obj.CheckRights(QUOTRT_UPDQUOTS, 0) && goods_obj.CheckRights(GOODSRT_UPDQUOTS, 0));
	if(!RVALUEPTR(flt, pFilt)) {
		flt.LocList.Add(LConfig.Location);
		flt.Flags |= QuotUpdFilt::fExistOnly;
	}
	while(pFilt || EditQuotUpdDialog(&flt) > 0) {
		TSCollection <PPBillPacket> reg_pack_list;
		flt.ByWhat = (flt.ByWhat == QuotUpdFilt::byDelete) ? QuotUpdFilt::byAbsVal : flt.ByWhat;
		PPWait(1);
		flt.LocList.CopyTo(&loc_list);
		loc_list.sort();
		if(flt.ByWhat == QuotUpdFilt::byLastReval) {
			PPLogMessage(PPFILNAM_QUOTUPD_LOG, PPSTR_TEXT, PPTXT_LOG_QUOTUPD_BYLASTREVAL, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
			PPID   kind = flt.QuotKindID;
			PPBillPacket  pack;
			PPTransferItem * p_ti;
			if(loc_list.getCount()) {
				PPID   bill_id = 0;
				{
					//
					// @todo Оптимизировать поиск по видам операций переоценки
					//
					BillTbl::Key1 k;
					k.Dt     = MAXDATE;
					k.BillNo = MAXLONG;
					while(!bill_id && BillObj->P_Tbl->search(1, &k, spLt)) {
						BillTbl::Rec bill_rec;
						BillObj->P_Tbl->copyBufTo(&bill_rec);
						if(loc_list.bsearch(bill_rec.LocID, 0) && GetOpType(bill_rec.OpID) == PPOPT_GOODSREVAL)
							bill_id = bill_rec.ID;
					}
				}
				if(bill_id) {
					THROW(BillObj->ExtractPacket(bill_id, &pack) > 0);
					{
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; pack.EnumTItems(&i, &p_ti);) {
							const PPID goods_id = labs(p_ti->GoodsID);
							if(!flt.GoodsID || (goods_id == flt.GoodsID) && goods_obj.BelongToGroup(goods_id, flt.GoodsGrpID)) {
								if(flt.GoodsID || !(flt.Flags & flt.fSkipNoDisGoods) || !goods_obj.CheckFlag(goods_id, GF_NODISCOUNT)) {
									double price = 0.0;
									THROW(r = ::GetCurGoodsPrice(goods_id, p_ti->LocID, GPRET_MOSTRECENT, &price));
									if(oneof2(r, GPRET_PRESENT, GPRET_CLOSEDLOTS)) {
										QuotIdent qi(p_ti->LocID, kind, 0 /* @curID */, 0 /* ArID */);
										PPQuotArray qary(goods_id);
										if(flt.AdvOptQuot == 0 || flt.IsQuotByAdvOptExists(&qary)) {
											int    to_process = 1;
											uint   pos = 0;
											THROW(goods_obj.GetQuotList(goods_id, 0, qary));
											int    is_exist = BIN(qary.SearchQi(qi, &pos) > 0);
											if(is_exist && qary.at(pos).IsRelative())
												to_process = 0;
											else if(flt.Flags & QuotUpdFilt::fExistOnly) {
												if(!is_exist)
													to_process = 0;
											}
											else if(flt.Flags & QuotUpdFilt::fNonExistOnly) {
												if(is_exist)
													to_process = 0;
											}
											if(to_process) {
												THROW(_SetQuot(&qary, qi, price, 0, 0, 0 /* period */));
												THROW(goods_obj.P_Tbl->SetQuotList(qary, 0));
												THROW(SetupQuotUpdRegBill(flt, qary, reg_pack_list));
											}
										}
									}
								}
							}
   	    					PPWaitPercent(i, pack.GetTCount(), 0);
						}
						if(flt.EvTokID)
							DS.LogAction(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, flt.EvTokID, 0, 0);
						THROW(tra.Commit());
						THROW(FinishUpdateQuots(reg_pack_list, 1));
					}
				}
			}
		}
		else {
			PPIDArray kind_list;
			if(!flt.QuotKindID) {
				for(PPID qk_id = 0; qk_obj.EnumItems(&qk_id) > 0;)
					kind_list.add(qk_id);
				kind_list.addUnique(PPQUOTK_BASE);
			}
			else
				kind_list.add(flt.QuotKindID);
			if(flt.ByWhat == QuotUpdFilt::byAddedQuot) {
				PPLogMessage(PPFILNAM_QUOTUPD_LOG, PPSTR_TEXT, PPTXT_LOG_QUOTUPD_BYSAMPLE, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
				PPViewQuot q_view;
				QuotFilt q_filt;
				QuotViewItem q_item;
				q_filt.QuotKindID = flt.AdvOptQuotKindID;
				q_filt.LocList.Add(flt.AdvOptLocID, 1 /* ignore zero */);
				q_filt.GoodsGrpID = flt.GoodsGrpID;
				q_filt.GoodsID    = flt.GoodsID; // @v7.3.8
				THROW_PP(flt.AdvOptQuotKindID, PPERR_QUOTKINDNEEDED);
				THROW(q_view.Init_(&q_filt));
				{
					PPWait(1); // q_view.Init_ убрал с экрана окно ожидания //
					PPTransaction tra(1);
					THROW(tra);
					for(q_view.InitIteration(); q_view.NextIteration(&q_item) > 0;) {
						PROFILE_START
						const PPID goods_id = q_item.GoodsID;
						if(!flt.GoodsID || goods_id == flt.GoodsID) {
							if(flt.GoodsID || !(flt.Flags & flt.fSkipNoDisGoods) || !goods_obj.CheckFlag(goods_id, GF_NODISCOUNT)) {

								//PPTXT_LOG_QUOTUPD_SAMPLEFOUND "Для товара '@goods' шаблонная котировка"
								PPFormatT(PPTXT_LOG_QUOTUPD_SAMPLEFOUND, &log_msg_buf, goods_id);

								QuotIdent qi(q_item.LocID, flt.AdvOptQuotKindID, 0, q_item.ArticleID);
								uint   qpos = 0;
								PPQuotArray qary(goods_id);
								LongArray qpos_list;
								PROFILE(THROW(goods_obj.GetQuotList(goods_id, 0, qary)));
								if(flt.Flags & QuotUpdFilt::fSetupDatedSamples) {
									const  uint qlc = qary.getCount();
									for(uint ql_idx = 0; ql_idx < qlc; ql_idx++) {
										const PPQuot & rq = qary.at(ql_idx);
                                        if(rq.Kind == flt.AdvOptQuotKindID && rq.LocID == q_item.LocID && rq.ArID == q_item.ArticleID) {
                                            qpos_list.add((long)ql_idx);
                                        }
									}
								}
								else {
									if(qary.SearchNearest(qi, &qpos) > 0) {
										qpos_list.add((long)qpos);
									}
								}
								if(qpos_list.getCount()) {
									const PPQuotArray qtemplate = qary;
									for(uint qp_idx = 0; qp_idx < qpos_list.getCount(); qp_idx++) {
										qpos = (uint)qpos_list.get(qp_idx);
										//if(qary.SearchNearest(qi, &qpos) > 0) {
										log_msg_buf.Space().Cat("FOUNDED");
										PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);

										PPQuot q;
										q = qary.at(qpos);
										int    upd = 0;
										//
										// Временный фильтр temp_filt будет использоваться для установки нового
										// значения котировки функцией SetupQuotList (в этом варианте работа функции
										// аналогична опции QuotUpdFilt::byAbsVal).
										//
										QuotUpdFilt temp_filt;
										temp_filt = flt;
										temp_filt.ByWhat = QuotUpdFilt::byAbsVal;
										temp_filt.QuotVal = q.Quot;
										temp_filt.QuotFlags = q.Flags;
										temp_filt.AdvOptQuotKindID = 0;
										temp_filt.AdvOptLocID = 0;
										temp_filt.AdvOptQuot = 0.0;
										if(flt.Flags & QuotUpdFilt::fSetupDatedSamples) {
											temp_filt.QuotValPeriod = q.Period;
										}
										PPQuotArray upd_list(goods_id);
										for(uint loc_idx = 0; loc_idx < loc_list.getCount(); loc_idx++) {
											const PPID dest_loc_id = loc_list.get(loc_idx);
											THROW(r = SetupQuotList(temp_filt, dest_loc_id, goods_id, &kind_list, &qary, &upd_list, &for_all));
											if(r > 0)
												upd = 1;
										}
										if(upd) {
											PROFILE_START
											THROW(goods_obj.P_Tbl->SetQuotListQ(upd_list, &qtemplate, 1, 0));
											THROW(SetupQuotUpdRegBill(flt, upd_list, reg_pack_list));
											PROFILE_END
										}
									}
								}
								else {
									log_msg_buf.Space().Cat("NOT FOUNDED");
									PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
								}
							}
							else {
								PPFormatT(PPTXT_LOG_QUOTUPD_IGNGNODIS, &log_msg_buf, goods_id);
								PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
							}
						}
						PPWaitPercent(q_view.GetCounter());
						PROFILE_END
					}
					if(flt.EvTokID)
						DS.LogAction(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, flt.EvTokID, 0, 0);
					THROW(tra.Commit());
					THROW(FinishUpdateQuots(reg_pack_list, 1));
				}
			}
			else {
				PPLogMessage(PPFILNAM_QUOTUPD_LOG, PPSTR_TEXT, PPTXT_LOG_QUOTUPD_GENERIC, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
				PPIDArray goods_list;
				if(flt.GoodsID) {
					// @v8.8.0 {
					Goods2Tbl::Rec goods_rec;
					THROW(goods_obj.Fetch(flt.GoodsID, &goods_rec) > 0);
					if(goods_rec.Flags & GF_GENERIC)
						goods_obj.GetGenericList(flt.GoodsID, &goods_list);
					else
					// } @v8.8.0
						goods_list.add(flt.GoodsID);
				}
				else {
					GoodsFilt g_filt;
					g_filt.GrpID = flt.GoodsGrpID;
					if(g_filt.GrpID) {
						PPFormatT(PPTXT_LOG_QUOTUPD_GOODSGRP, &log_msg_buf, g_filt.GrpID);
						PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
					}
					if(flt.EventList.GetCount() && flt.EvTokID) {
						SString ev_msg_buf;
						SysJournal * p_sj = DS.GetTLA().P_SysJ;
						if(p_sj) {
							LDATETIME moment;
							PPIDArray acn_list;
							acn_list.add(PPACN_EVENTTOKEN);
							if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, flt.EvTokID, &acn_list, &moment) > 0) {
								THROW_MEM(g_filt.P_SjF = new SysJournalFilt);
								g_filt.P_SjF->ActionIDList = flt.EventList.Get();
								g_filt.P_SjF->Period.low = moment.d;
								g_filt.P_SjF->BegTm = moment.t;
								(ev_msg_buf = 0).Cat(moment, DATF_DMY, TIMF_HMS).Space().CatEq("EvTokID", flt.EvTokID);
							}
							else
								ev_msg_buf = "event not found";
						}
						else
							ev_msg_buf = "error";
						PPFormatT(PPTXT_LOG_QUOTUPD_GLISTBYEV, &log_msg_buf, ev_msg_buf.cptr());
						PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
					}
					GoodsIterator::GetListByFilt(&g_filt, &goods_list, 0);
                    PPLoadText(PPTXT_LOG_QUOTUPD_GOODSLIST, log_msg_buf);
                    log_msg_buf.CatDiv(':', 2).CatEq("count", (long)goods_list.getCount()).Space();
                    for(uint i = 0; i < goods_list.getCount(); i++) {
						if(i)
							log_msg_buf.CatDiv(',', 2);
						log_msg_buf.Cat(goods_list.get(i));
                    }
                    PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
				}
				PPTransaction tra(1);
				THROW(tra);
				for(uint m = 0; m < goods_list.getCount(); m++) {
					const PPID goods_id = goods_list.get(m);
					if(flt.GoodsID || !(flt.Flags & flt.fSkipNoDisGoods) || !goods_obj.CheckFlag(goods_id, GF_NODISCOUNT)) {
						int    upd = 0;
						PPQuotArray qary(goods_id);
						THROW(goods_obj.GetQuotList(goods_id, 0, qary));
						if(flt.AdvOptQuot == 0 || flt.IsQuotByAdvOptExists(&qary)) {
							if(flt.LocList.IsEmpty()) {
								THROW(r = SetupQuotList(flt, 0, goods_id, &kind_list, &qary, 0, &for_all));
								if(r > 0)
									upd = 1;
							}
							else {
								for(uint i = 0; i < loc_list.getCount(); i++) {
									THROW(r = SetupQuotList(flt, loc_list.get(i), goods_id, &kind_list, &qary, 0, &for_all));
									if(r > 0)
										upd = 1;
								}
							}
							if(upd) {
								THROW(goods_obj.P_Tbl->SetQuotList(qary, 0));
								THROW(SetupQuotUpdRegBill(flt, qary, reg_pack_list));
							}
						}
					}
					else {
						PPFormatT(PPTXT_LOG_QUOTUPD_IGNGNODIS, &log_msg_buf, goods_id);
						PPLogMessage(PPFILNAM_QUOTUPD_LOG, log_msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
					}
					PPWaitPercent(m+1, goods_list.getCount());
				}
				if(flt.EvTokID)
					DS.LogAction(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, flt.EvTokID, 0, 0);
				THROW(tra.Commit());
				THROW(FinishUpdateQuots(reg_pack_list, 1));
			}
		}
		PPWait(0);
		if(pFilt || !CONFIRM(PPCFM_QUOTUPD_AGAIN))
			break;
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
#if SLTEST_RUNNING // {

class PrcssrQuotTester {
public:
	PrcssrQuotTester();
	~PrcssrQuotTester();
	int    Init();
	int    Run();
private:
	int    GenerateQuot(PPQuot * pQuot);
	int    Test_SetQuot(const PPQuot & rQuot, int use_ta);

	PPID   SupplDealQkID;  // Вид котировки, определяющий контрактные цены поставщиков
	PPID   SupplDevUpQkID; // Вид котировки, определяющий верхние отклонения от контрактных цен поставщиков
	PPID   SupplDevDnQkID; // Вид котировки, определяющий нижние отклонения от контрактных цен поставщиков
	PPID   MtxQkID;        // Вид котировки для товарной матрицы
	PPID   MtxRestrQkID;   // Вид котировки ограничений по товарной матрице
	PPObjGoods GObj;       //
	QuotationCore Qc;
	PPIDArray GoodsList;   // Список всех товаров
	PPIDArray GoodsGrpList; // Список всех обыкновенных товарных групп и групп верхнего уровня //
	PPIDArray LocList;     // Список всех складов
	PPIDArray QkList;      // Список видов котировок

	SRng * P_RngCount;     //
	SRng * P_RngGoods;     // Генератор товаров
	SRng * P_RngGoodsGrp;  // Генератор товарных групп
	SRng * P_RngLoc;       // Генератор складов
	SRng * P_RngQ;         // Генератор значения котировки
};

PrcssrQuotTester::PrcssrQuotTester()
{
	const PPThreadLocalArea & r_tla = DS.GetConstTLA();
	SupplDealQkID  = r_tla.SupplDealQuotKindID;
	SupplDevUpQkID = r_tla.SupplDevUpQuotKindID;
	SupplDevDnQkID = r_tla.SupplDevDnQuotKindID;
	MtxQkID        = GObj.GetConfig().MtxQkID;
	MtxRestrQkID   = GObj.GetConfig().MtxRestrQkID;

	P_RngCount = 0;
	P_RngGoods = 0;
	P_RngGoodsGrp = 0;
	P_RngLoc = 0;
	P_RngQ = 0;
}

PrcssrQuotTester::~PrcssrQuotTester()
{
	delete P_RngCount;
	delete P_RngGoods;
	delete P_RngGoodsGrp;
	delete P_RngLoc;
	delete P_RngQ;
}

int PrcssrQuotTester::Init()
{
	int    ok = 1;
	GoodsList.freeAll();
	LocList.freeAll();
	GoodsGrpList.freeAll();
	QkList.freeAll();

	ZDELETE(P_RngCount);
	ZDELETE(P_RngGoods);
	ZDELETE(P_RngGoodsGrp);
	ZDELETE(P_RngLoc);
	ZDELETE(P_RngQ);

	PPObjLocation loc_obj;
	PPObjQuotKind qk_obj;

	const  int  dont_sel_passive = BIN(GObj.GetConfig().Flags & GCF_DONTSELPASSIVE);
	GoodsFilt gf;
	if(dont_sel_passive)
		gf.Flags |= GoodsFilt::fHidePassive;
	THROW(GoodsIterator::GetListByFilt(&gf, &GoodsList));
	THROW(GoodsList.getCount());
	{
		PPID   gg_id = 0;
		SString gg_name;
		for(GoodsGroupIterator ggiter(0, 0); ggiter.Next(&gg_id, gg_name) > 0;) {
			GoodsGrpList.add(gg_id);
		}
	}
	loc_obj.GetWarehouseList(&LocList);
	{
		LTIME ct = getcurtime_();
		THROW(P_RngGoods = SRng::CreateInstance(SRng::algMT, 0));
		P_RngGoods->Set(ct.v);
		THROW(P_RngGoodsGrp= SRng::CreateInstance(SRng::algMT, 0));
		P_RngGoodsGrp->Set(ct.v + 17);
		THROW(P_RngLoc = SRng::CreateInstance(SRng::algMT, 0));
		P_RngLoc->Set(ct.v + 23);
		THROW(P_RngQ  = SRng::CreateInstance(SRng::algMT, 0));
		P_RngQ->Set(ct.v + 37);
		THROW(P_RngCount = SRng::CreateInstance(SRng::algMT, 0));
		P_RngCount->Set(ct.v + 101);
	}
	{
		QuotKindFilt qk_filt;
		qk_filt.Flags |= QuotKindFilt::fAll;
		StrAssocArray qk_list;
		qk_obj.MakeList(&qk_filt, &qk_list);
		for(uint i = 0; i < qk_list.getCount(); i++)
			QkList.add(qk_list.at(i).Id);
	}
	CATCHZOK
	return ok;
}

int PrcssrQuotTester::GenerateQuot(PPQuot * pQuot)
{
	int    ok = -1;
	while(ok < 0) {
		int    is_group = BIN((P_RngCount->Get() % 10) == 0);
		uint   pos = 0;
		PPID   id = 0;
		PPID   loc_id = 0;
		PPID   qk_id = 0;
		Goods2Tbl::Rec goods_rec;
		if(!is_group) {
			pos = P_RngCount->GetUniformInt(GoodsList.getCount());
			if(pos < GoodsList.getCount())
				id = GoodsList.get(pos);
		}
		else {
			pos = P_RngCount->GetUniformInt(GoodsGrpList.getCount());
			if(pos < GoodsGrpList.getCount())
				id = GoodsGrpList.get(pos);
		}
		if(id && GObj.Fetch(id, &goods_rec) > 0) {
			int    skip = 0;
			ReceiptTbl::Rec lot_rec;
			pos = (P_RngCount->Get() % (LocList.getCount() + 1));
			loc_id = (pos < LocList.getCount()) ? LocList.get(pos) : 0;
			pos = P_RngCount->GetUniformInt(QkList.getCount());
			qk_id = QkList.get(pos);
			if(qk_id) {
				PPQuot q;
				q.LocID = loc_id;
				q.GoodsID = id;
				q.Flags = 0;
				q.Quot = 0.0;
				if(qk_id == MtxQkID) {
					uint temp = P_RngCount->Get() % 3;
					if(temp == 0)
						q.Quot = 0.0;
					else if(temp == 1)
						q.Quot = 1.0;
					else if(temp == 2)
						q.Quot = -1.0;
				}
				else if(qk_id == MtxRestrQkID) {
					if(is_group) {
						uint temp = P_RngCount->GetUniformInt(200);
						q.Quot = temp * 10;
					}
					else
						skip = 1;
				}
				else if(qk_id == SupplDealQkID) {
					if(!is_group) {
						double price = 0.0;
						GetCurGoodsPrice(id, loc_id, 0, &price, &lot_rec);
						if(lot_rec.Cost > 0.0) {
							double dev = 2.0;
							while(fabs(dev) > 0.5)
								dev = P_RngCount->GetGaussian(0.4);
							q.Quot = lot_rec.Cost * (1.0 + dev);
						}
						else
							skip = 1;
					}
					else
						skip = 1;
				}
				else if(oneof2(qk_id, SupplDevDnQkID, SupplDevUpQkID)) {
					if(is_group) {
						double dev = 2.0;
						while(fabs(dev) > 0.3)
							dev = P_RngCount->GetGaussian(0.4);
						q.Quot = R0(fabs(dev * 100));
					}
					else
						skip = 1;
				}
				else {
					int    is_rel = 0;
					uint   d = P_RngCount->Get();
					if(is_group) {
						if((d % 2) == 0) { // Относительная по COST
							double dev = 2.0;
							while(fabs(dev) > 0.9)
								dev = P_RngCount->GetGaussian(1.0);
							q.Quot = round(fabs(dev), 1);
							q.Flags |= PPQuot::fPctOnCost;
						}
						else { // Относительная по PRICE
							double dev = 2.0;
							while(fabs(dev) > 0.2)
								dev = P_RngCount->GetGaussian(0.4);
							q.Quot = round(dev, 1);
							q.Flags |= PPQuot::fPctOnPrice;
						}
					}
					else if((d % 3) == 0) { // Абсолютное значение
						double price = 0.0;
						GetCurGoodsPrice(id, loc_id, 0, &price, &lot_rec);
						if(price > 0.0) {
							double dev = 2.0;
							while(fabs(dev) > 0.2)
								dev = P_RngCount->GetGaussian(0.4);
							q.Quot = price * (1.0 + dev);
						}
						else
							skip = 1;
					}
					else if((d % 13) == 0) {
						q.Quot = 0.0;
						q.Flags |= PPQuot::fPctDisabled;
					}
					else if((d % 3) == 1) { // Относительная по COST
						double dev = 2.0;
						while(fabs(dev) > 0.9)
							dev = P_RngCount->GetGaussian(1.0);
						q.Quot = round(fabs(dev), 1);
						q.Flags |= PPQuot::fPctOnCost;
					}
					else {
						double dev = 2.0;
						while(fabs(dev) > 0.2)
							dev = P_RngCount->GetGaussian(0.4);
						q.Quot = round(dev, 1);
						q.Flags |= PPQuot::fPctOnPrice;
					}
				}
				if(!skip) {
					*pQuot = q;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PrcssrQuotTester::Test_SetQuot(const PPQuot & rQuot, int use_ta)
{
	int    ok = 1;
	PPID   q_id = 0;
	PPID   goods_id = rQuot.GoodsID;
	PPQuotArray q_list, q_list2;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Qc.GetCurrList(goods_id, 0, 0, q_list));
		THROW(Qc.SetCurr(&q_id, &rQuot, 1, 0));
		THROW(Qc.GetCurrList(goods_id, 0, 0, q_list2));
		{
			int diff_count = q_list.getCount() - q_list2.getCount();
			if(diff_count != 0 && diff_count != -1 && diff_count != 1) {
				// @error Удалено либо создано более одной котировки
				ok = -1;
			}
			else {
				uint i, j;
				for(i = 0; i < q_list.getCount(); i++) {
					const PPQuot & r_item = q_list.at(i);
					int found = 0;
					for(j = 0; j < q_list2.getCount(); j++) {
						const PPQuot & r_item2 = q_list2.at(i);
						if(r_item.ID == r_item2.ID) {
							found = 1;
							if(r_item.ID != rQuot.ID) {
								if(!r_item.IsEqual(r_item2)) {
									// @error Исказилась котировка, отличная от rQuot
									ok = -1;
								}
							}
							else {
								if(!r_item.IsEqual(rQuot)) {
									// @error Исказаласть котировка rQuot
									ok = -1;
								}
							}
						}
					}
					if(!found) {
						//
						// После записи в списке не найден элемент r_item.
						// Это может означать, что rQuot.IsEmpty() (тогда правильно),
						// или элемент исчез (не правильно).
						//
						if(!rQuot.IsEmpty()) {
							// @error Котировка r_item исчезла
							ok = -1;
						}
					}
				}
				for(i = 0; i < q_list2.getCount(); i++) {
					const PPQuot & r_item2 = q_list2.at(i);
					int found = 0;
					for(j = 0; j < q_list.getCount(); j++) {
						const PPQuot & r_item = q_list.at(i);
						if(r_item.ID == r_item2.ID) {
							found = 1;
							if(r_item.ID != rQuot.ID) {
								if(!r_item.IsEqual(r_item2)) {
									// @error Исказилась котировка, отличная от rQuot
									ok = -1;
								}
							}
							else {
								if(!r_item.IsEqual(rQuot)) {
									// @error Исказаласть котировка rQuot
									ok = -1;
								}
							}
						}
					}
					if(!found) {
						//
						// После записи в списке появился новый элемент r_item.
						//
						if(!rQuot.IsEqual(r_item2, PPQuot::cmpNoID)) {
							// @error Появилась котировка не эквивалентная той, что мы передавали
						}
						else if(rQuot.IsEmpty()) {
							// @error Пустая котировка rQuot была записана
							ok = -1;
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PrcssrQuotTester::Run()
{
	int    ok = 1;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(uint i = 0; i < 1000; i++) {
			PPQuot quot;
			if(GenerateQuot(&quot) > 0) {
				int r;
				THROW(r = Test_SetQuot(quot, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING

