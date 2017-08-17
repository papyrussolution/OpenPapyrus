// COMMCFG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017
// @codepage windows-1251
//
// Диалог редактирования общих параметров настройки
//
#include <pp.h>
#pragma hdrstop

class CommCfgDialog : public TDialog {
public:
	CommCfgDialog() : TDialog(DLG_COMMCFG)
	{
	}
	void   setDTS(const PPCommConfig *);
	int    getDTS(PPCommConfig *);
private:
	DECL_HANDLE_EVENT;
	int    editSysParams();
	int    getAcc(uint ctl, Acct *);
	PPCommConfig cfg;
};

void CommCfgDialog::setDTS(const PPCommConfig * pData)
{
	RVALUEPTR(cfg, pData);
	setCtrlData(CTL_COMMCFG_SUPPLACC,   &cfg.SupplAcct);
	setCtrlData(CTL_COMMCFG_SELLACC,    &cfg.SellAcct);
	setCtrlData(CTL_COMMCFG_CASHACC,    &cfg.CashAcct);
	setCtrlData(CTL_COMMCFG_IMPRESTACC, &cfg.ImprestAcct);
	setCtrlData(CTL_COMMCFG_ASSETACC,   &cfg.AssetAcct);

	AddClusterAssocDef(CTL_COMMCFG_INCM,  0, INCM_DEFAULT);
	AddClusterAssoc(CTL_COMMCFG_INCM,  1, INCM_BYSHIPMENT);
	AddClusterAssoc(CTL_COMMCFG_INCM,  2, INCM_BYPAYMENT);
	SetClusterData(CTL_COMMCFG_INCM, cfg.IncomeCalcMethod);

	AddClusterAssoc(CTL_COMMCFG_FLAGS, 0, CCFLG_PRICEWOEXCISE);
	AddClusterAssoc(CTL_COMMCFG_FLAGS, 1, CCFLG_TGGLEXCSNPRICE);
	AddClusterAssoc(CTL_COMMCFG_FLAGS, 2, CCFLG_SETWLONLINK);
	AddClusterAssoc(CTL_COMMCFG_FLAGS, 3, CCFLG_COSTWOVATBYDEF);
	AddClusterAssoc(CTL_COMMCFG_FLAGS, 4, CCFLG_COSTWOVATBYSUM);
	SetClusterData(CTL_COMMCFG_FLAGS, cfg.Flags);
}

int CommCfgDialog::getAcc(uint ctl, Acct * pAcc)
{
	getCtrlData(ctl, pAcc);
	pAcc->ar = 0;
	PPObjAccount acc_obj;
	return ((!pAcc->ac && !pAcc->sb) || acc_obj.SearchNum(pAcc->ac, pAcc->sb, 0L /* @curID */) > 0) ? 1 : (selectCtrl(ctl), 0);
}

int CommCfgDialog::getDTS(PPCommConfig * pData)
{
	if(!getAcc(CTL_COMMCFG_SUPPLACC, &cfg.SupplAcct) ||
		!getAcc(CTL_COMMCFG_SELLACC, &cfg.SellAcct) ||
		!getAcc(CTL_COMMCFG_CASHACC, &cfg.CashAcct) ||
		!getAcc(CTL_COMMCFG_IMPRESTACC, &cfg.ImprestAcct) ||
		!getAcc(CTL_COMMCFG_ASSETACC, &cfg.AssetAcct))
		return 0;
	GetClusterData(CTL_COMMCFG_FLAGS, &cfg.Flags);
	GetClusterData(CTL_COMMCFG_INCM,  &cfg.IncomeCalcMethod);
	cfg.Reserve2 = 0;
	ASSIGN_PTR(pData, cfg);
	return 1;
}

int CommCfgDialog::editSysParams()
{
	class SysParamsDialog : public TDialog {
	public:
		SysParamsDialog() : TDialog(DLG_CCFGSYS)
		{
		}
		int    setDTS(const PPCommConfig * pData)
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_CCFGSYS_MAXGBLINES, &Data.MaxGoodsBillLines);
			setCtrlData(CTL_CCFGSYS_FRRLDAYS,   &Data.FRRL_Days);
			setCtrlData(CTL_CCFGSYS_GRESTTHR,   &Data.GRestCalcThreshold);
			setCtrlData(CTL_CCFGSYS_ICQTTYEPS,  &Data.IltiCvtQttyEpsilon);
			setCtrlData(CTL_CCFGSYS_CSUNITPRD,  &Data.CSessUnitingPeriod);
			setCtrlData(CTL_CCFGSYS_CSFWPRD,    &Data.CSessFwAsscPeriod);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  0, CCFLG_USEGDSCLS);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  1, CCFLG_USEGOODSPCKG);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  2, CCFLG_USEDRAFTBILL);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  3, CCFLG_USEHISTBILL);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  4, CCFLG_CHECKSPOILAGE);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  5, CCFLG_THROUGHSCARDUNIQ);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  6, CCFLG_USEHISTGOODS);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  7, CCFLG_USECCHECKEXT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  8, CCFLG_USECCHECKLINEEXT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  9, CCFLG_GENLOTONUNLIMORDER);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 10, CCFLG_USEARGOODSCODE);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 11, CCFLG_INDIVIDBILLEXTFILT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 12, CCFLG_USEDISTRIBCOST);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 13, CCFLG_RSRVSJ);
			SetClusterData(CTL_CCFGSYS_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_CCFGSYS_LCRUSAGE, 0, 0);
			AddClusterAssoc(CTL_CCFGSYS_LCRUSAGE, 1, 1);
			AddClusterAssoc(CTL_CCFGSYS_LCRUSAGE, 2, 2);
			SetClusterData(CTL_CCFGSYS_LCRUSAGE, Data.LcrUsage);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 0, CCFLG2_QUOT2);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 1, CCFLG2_INDEXEADDR);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 2, CCFLG2_SYNCLOT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 3, CCFLG2_USECCHECKEXTPAYM); // @v8.0.0
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 4, CCFLG2_USEOMTPAYMAMT); // @v8.5.8
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 5, CCFLG2_USESARTREDB); // @v9.7.11
			SetClusterData(CTL_CCFGSYS_FLAGS2, Data.Flags2);
			{
				SString db_path;
				PPGetPath(PPPATH_SARTREDB, db_path);
				DisableClusterItem(CTL_CCFGSYS_FLAGS2, 5, !isDir(db_path.RmvLastSlash()));
			}
			return 1;
		}
		int    getDTS(PPCommConfig * pData)
		{
			getCtrlData(CTL_CCFGSYS_MAXGBLINES, &Data.MaxGoodsBillLines);
			getCtrlData(CTL_CCFGSYS_FRRLDAYS,   &Data.FRRL_Days);
			getCtrlData(CTL_CCFGSYS_GRESTTHR,   &Data.GRestCalcThreshold);
			getCtrlData(CTL_CCFGSYS_ICQTTYEPS,  &Data.IltiCvtQttyEpsilon);
			getCtrlData(CTL_CCFGSYS_CSUNITPRD,  &Data.CSessUnitingPeriod);
			getCtrlData(CTL_CCFGSYS_CSFWPRD,    &Data.CSessFwAsscPeriod);
			GetClusterData(CTL_CCFGSYS_FLAGS, &Data.Flags);
			Data.LcrUsage = (int16)GetClusterData(CTL_CCFGSYS_LCRUSAGE);
			GetClusterData(CTL_CCFGSYS_FLAGS2, &Data.Flags2);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_CCFGSYS_FLAGS)) {
				long   flags = 0;
				GetClusterData(CTL_CCFGSYS_FLAGS, &flags);
				if(flags & CCFLG_THROUGHSCARDUNIQ) {
					int    r = 0;
					PPObjSCard obj_scard;
					if(!(r = obj_scard.CheckUniq())) {
						flags &= ~CCFLG_THROUGHSCARDUNIQ;
						SetClusterData(CTL_CCFGSYS_FLAGS, flags);
						PPError(PPERR_DUPLSCARDFOUND);
					}
				}
				clearEvent(event);
			}
		}
		PPCommConfig Data;
	};
	DIALOG_PROC_BODY(SysParamsDialog, &cfg);
}

IMPL_HANDLE_EVENT(CommCfgDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCommCfgMainOrg:
				{
					TDialog * dlg = new TDialog(DLG_MAINORG);
					if(CheckDialogPtrErr(&dlg)) {
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_DEF,      PPOBJ_PERSON, cfg.MainOrgID,          OLW_CANINSERT, (void *)PPPRK_MAIN);
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_DIRECTOR, PPOBJ_PERSON, cfg.MainOrgDirector_,   OLW_CANINSERT, (void *)PPPRK_EMPL);
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_ACCTNT,   PPOBJ_PERSON, cfg.MainOrgAccountant_, OLW_CANINSERT, (void *)PPPRK_EMPL);
						if(ExecView(dlg) == cmOK) {
							dlg->getCtrlData(CTLSEL_MAINORG_DEF,      &cfg.MainOrgID);
							dlg->getCtrlData(CTLSEL_MAINORG_DIRECTOR, &cfg.MainOrgDirector_);
							dlg->getCtrlData(CTLSEL_MAINORG_ACCTNT,   &cfg.MainOrgAccountant_);
						}
					}
					delete dlg;
				}
				break;
			case cmCommCfgOp:
				{
					TDialog * dlg = new TDialog(DLG_CCFGOP);
					if(CheckDialogPtrErr(&dlg)) {
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_SUPPLSH,    PPOBJ_ACCSHEET,  cfg.SupplAccSheet, OLW_CANINSERT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_SELLSH,     PPOBJ_ACCSHEET,  cfg.SellAccSheet,  OLW_CANINSERT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RCPTOP,     PPOBJ_OPRKIND,   cfg.ReceiptOp,     OLW_CANINSERT, (void *)PPOPT_GOODSRECEIPT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RETAILOP,   PPOBJ_OPRKIND,   cfg.RetailOp,      OLW_CANINSERT, (void *)PPOPT_GOODSEXPEND);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RETAILRET,  PPOBJ_OPRKIND,   cfg.RetailRetOp,   OLW_CANINSERT, (void *)PPOPT_GOODSRETURN);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_INTRRCPT,   PPOBJ_OPRKIND,   cfg.IntrReceiptOp, OLW_CANINSERT, (void *)PPOPT_GOODSRECEIPT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_ACOP,       PPOBJ_OPRKIND,   cfg.AutoComplOp,   OLW_CANINSERT, (void *)PPOPT_GOODSMODIF);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_DRAFTRCPT,  PPOBJ_OPRKIND,   cfg.DraftRcptOp,   OLW_CANINSERT, (void *)PPOPT_DRAFTRECEIPT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_DYNGTYPESA, PPOBJ_GOODSTYPE, cfg.DynGoodsTypeForSupplAgent, OLW_CANINSERT);
						// @v9.7.0 dlg->setCtrlData(CTL_CCFGOP_PREPAYGOODS, cfg.PrepayInvoiceGoodsCode);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_PREPAYWARE, PPOBJ_GOODS, cfg.PrepayInvoiceGoodsID, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0); // @v9.7.0
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 0, CCFLG_USENOUPDRESTOPFLAG);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 1, CCFLG_DISABLEACCTURN);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 2, CCFLG_IGNOREORDERSDEBT);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 3, CCFLG_NOADJPRWROFFDRAFT); // @v9.3.4
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 4, CCFLG_DONTUNDOOPCNTRONESC); // @v9.5.1
						dlg->SetClusterData(CTL_CCFGOP_FLAGS, cfg.Flags);
						if(ExecView(dlg) == cmOK) {
							dlg->getCtrlData(CTLSEL_CCFGOP_SUPPLSH,   &cfg.SupplAccSheet);
							dlg->getCtrlData(CTLSEL_CCFGOP_SELLSH,    &cfg.SellAccSheet);
							dlg->getCtrlData(CTLSEL_CCFGOP_RCPTOP,    &cfg.ReceiptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_RETAILOP,  &cfg.RetailOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_RETAILRET, &cfg.RetailRetOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_INTRRCPT,  &cfg.IntrReceiptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_ACOP,      &cfg.AutoComplOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_DRAFTRCPT, &cfg.DraftRcptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_DYNGTYPESA, &cfg.DynGoodsTypeForSupplAgent);
							// @v9.7.0 dlg->getCtrlData(CTL_CCFGOP_PREPAYGOODS,  cfg.PrepayInvoiceGoodsCode);
							dlg->getCtrlData(CTLSEL_CCFGOP_PREPAYWARE, &cfg.PrepayInvoiceGoodsID); // @v9.7.0
							dlg->GetClusterData(CTL_CCFGOP_FLAGS, &cfg.Flags);
						}
					}
					delete dlg;
				}
				break;
			case cmCommCfgSys:
				editSysParams();
				break;
			default:
				return;
		}
		clearEvent(event);
	}
}

int SLAPI EditCommConfig()
{
	int    r, valid_data = 0;
	PPCommConfig cfg;
	CommCfgDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_COMMCONFIG, PPR_MOD, 0));
	THROW(GetCommConfig(&cfg));
	THROW(CheckDialogPtr(&(dlg = new CommCfgDialog)));
	dlg->setDTS(&cfg);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&cfg)) {
			valid_data = 1;
			THROW(SetCommConfig(&cfg, 1));
		}
		else
			PPError();
	}
	CATCH
		r = PPErrorZ();
	ENDCATCH
	delete dlg;
	return r;
}
//
// Редактирование текущей конфигурации
//
int SLAPI EditCurrConfig()
{
	int    ok = -1;
	PPID   main_org_id = 0;
	PPID   curr_loc = 0;
	TDialog * dlg = new TDialog(DLG_CURRCFG);
	if(CheckDialogPtrErr(&dlg)) {
		GetMainOrgID(&main_org_id);
		curr_loc = LConfig.Location;
		SetupPPObjCombo(dlg, CTLSEL_CURRCFG_MAINORG, PPOBJ_PERSON, main_org_id, 0, (void *)PPPRK_MAIN);
		SetupPPObjCombo(dlg, CTLSEL_CURRCFG_LOC, PPOBJ_LOCATION, curr_loc, 0);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_CURRCFG_MAINORG, &main_org_id);
			dlg->getCtrlData(CTLSEL_CURRCFG_LOC,     &curr_loc);
			if(main_org_id)
				DS.SetMainOrgID(main_org_id, 1);
			if(curr_loc)
				DS.SetLocation(curr_loc);
			ok = 1;
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}
