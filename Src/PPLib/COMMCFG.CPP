// COMMCFG.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
// Диалог редактирования общих параметров настройки
//
#include <pp.h>
#pragma hdrstop

class CommCfgDialog : public TDialog {
	DECL_DIALOG_DATA(PPCommConfig);
public:
	CommCfgDialog() : TDialog(DLG_COMMCFG)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_COMMCFG_SUPPLACC,   &Data.SupplAcct);
		setCtrlData(CTL_COMMCFG_SELLACC,    &Data.SellAcct);
		setCtrlData(CTL_COMMCFG_CASHACC,    &Data.CashAcct);
		setCtrlData(CTL_COMMCFG_IMPRESTACC, &Data.ImprestAcct);
		setCtrlData(CTL_COMMCFG_ASSETACC,   &Data.AssetAcct);
		AddClusterAssocDef(CTL_COMMCFG_INCM,  0, INCM_DEFAULT);
		AddClusterAssoc(CTL_COMMCFG_INCM,  1, INCM_BYSHIPMENT);
		AddClusterAssoc(CTL_COMMCFG_INCM,  2, INCM_BYPAYMENT);
		SetClusterData(CTL_COMMCFG_INCM, Data.IncomeCalcMethod);
		AddClusterAssoc(CTL_COMMCFG_FLAGS, 0, CCFLG_PRICEWOEXCISE);
		AddClusterAssoc(CTL_COMMCFG_FLAGS, 1, CCFLG_TGGLEXCSNPRICE);
		AddClusterAssoc(CTL_COMMCFG_FLAGS, 2, CCFLG_SETWLONLINK);
		AddClusterAssoc(CTL_COMMCFG_FLAGS, 3, CCFLG_COSTWOVATBYDEF);
		AddClusterAssoc(CTL_COMMCFG_FLAGS, 4, CCFLG_COSTWOVATBYSUM);
		SetClusterData(CTL_COMMCFG_FLAGS, Data.Flags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		THROW(getAcc(sel = CTL_COMMCFG_SUPPLACC, &Data.SupplAcct));
		THROW(getAcc(sel = CTL_COMMCFG_SELLACC, &Data.SellAcct));
		THROW(getAcc(sel = CTL_COMMCFG_CASHACC, &Data.CashAcct));
		THROW(getAcc(sel = CTL_COMMCFG_IMPRESTACC, &Data.ImprestAcct));
		THROW(getAcc(sel = CTL_COMMCFG_ASSETACC, &Data.AssetAcct));
		GetClusterData(CTL_COMMCFG_FLAGS, &Data.Flags);
		GetClusterData(CTL_COMMCFG_INCM,  &Data.IncomeCalcMethod);
		// @v12.0.5 Data.Reserve2 = 0;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    editSysParams();
	int    getAcc(uint ctl, Acct *);
	PPObjAccount AccObj;
};

int CommCfgDialog::getAcc(uint ctl, Acct * pAcc)
{
	getCtrlData(ctl, pAcc);
	pAcc->ar = 0;
	return BIN((!pAcc->ac && !pAcc->sb) || AccObj.SearchNum(pAcc->ac, pAcc->sb, 0L/*@curID*/) > 0);
}

int CommCfgDialog::editSysParams()
{
	class SysParamsDialog : public TDialog {
		DECL_DIALOG_DATA(PPCommConfig);
	public:
		enum {
			usehistobjBill   = 0x0001,
			usehistobjGoods  = 0x0002,
			usehistobjPerson = 0x0004,
			usehistobjSCard  = 0x0008,
		};
		SysParamsDialog() : TDialog(DLG_CCFGSYS)
		{
		}
		DECL_DIALOG_SETDTS()
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
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  2, CCFLG_USEDRAFTBILL); // @obsolete
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  3, CCFLG_CHECKSPOILAGE);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  4, CCFLG_THROUGHSCARDUNIQ);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  5, CCFLG_USECCHECKEXT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  6, CCFLG_USECCHECKLINEEXT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  7, CCFLG_GENLOTONUNLIMORDER);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  8, CCFLG_USEARGOODSCODE);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS,  9, CCFLG_INDIVIDBILLEXTFILT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 10, CCFLG_USEDISTRIBCOST);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS, 11, CCFLG_RSRVSJ);
			SetClusterData(CTL_CCFGSYS_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_CCFGSYS_LCRUSAGE, 0, 0);
			AddClusterAssoc(CTL_CCFGSYS_LCRUSAGE, 1, 1);
			AddClusterAssoc(CTL_CCFGSYS_LCRUSAGE, 2, 2);
			SetClusterData(CTL_CCFGSYS_LCRUSAGE, Data.LcrUsage);
			AddClusterAssoc(CTL_CCFGSYS_LCRV2, 0, CCFLG2_USELCR2);
			SetClusterData(CTL_CCFGSYS_LCRV2, Data.Flags2);
			setCtrlData(CTL_CCFGSYS_LCRSINCE, &Data.LcrUsageSince);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 0, CCFLG2_QUOT2);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 1, CCFLG2_INDEXEADDR);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 2, CCFLG2_SYNCLOT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 3, CCFLG2_USECCHECKEXTPAYM);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 4, CCFLG2_USEOMTPAYMAMT);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 5, CCFLG2_USESARTREDB);
			AddClusterAssoc(CTL_CCFGSYS_FLAGS2, 6, CCFLG2_USELOTXCODE);
			SetClusterData(CTL_CCFGSYS_FLAGS2, Data.Flags2);
			{
				long   usehistobj = 0;
				SETFLAG(usehistobj, usehistobjBill,   Data.Flags & CCFLG_USEHISTBILL);
				SETFLAG(usehistobj, usehistobjGoods,  Data.Flags & CCFLG_USEHISTGOODS);
				SETFLAG(usehistobj, usehistobjPerson, Data.Flags2 & CCFLG2_USEHISTPERSON);
				SETFLAG(usehistobj, usehistobjSCard,  Data.Flags2 & CCFLG2_USEHISTSCARD);
				AddClusterAssoc(CTL_CCFGSYS_OBJHISTF, 0, usehistobjBill);
				AddClusterAssoc(CTL_CCFGSYS_OBJHISTF, 1, usehistobjGoods);
				AddClusterAssoc(CTL_CCFGSYS_OBJHISTF, 2, usehistobjPerson);
				AddClusterAssoc(CTL_CCFGSYS_OBJHISTF, 3, usehistobjSCard);
				SetClusterData(CTL_CCFGSYS_OBJHISTF, usehistobj);
			}
			{
				SString db_path;
				PPGetPath(PPPATH_SARTREDB, db_path);
				DisableClusterItem(CTL_CCFGSYS_FLAGS2, 5, !SFile::IsDir(db_path.RmvLastSlash()));
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			getCtrlData(CTL_CCFGSYS_MAXGBLINES, &Data.MaxGoodsBillLines);
			getCtrlData(CTL_CCFGSYS_FRRLDAYS,   &Data.FRRL_Days);
			getCtrlData(CTL_CCFGSYS_GRESTTHR,   &Data.GRestCalcThreshold);
			getCtrlData(CTL_CCFGSYS_ICQTTYEPS,  &Data.IltiCvtQttyEpsilon);
			getCtrlData(CTL_CCFGSYS_CSUNITPRD,  &Data.CSessUnitingPeriod);
			getCtrlData(CTL_CCFGSYS_CSFWPRD,    &Data.CSessFwAsscPeriod);
			GetClusterData(CTL_CCFGSYS_FLAGS, &Data.Flags);
			Data.LcrUsage = (int16)GetClusterData(CTL_CCFGSYS_LCRUSAGE);
			GetClusterData(CTL_CCFGSYS_LCRV2, &Data.Flags2);
			getCtrlData(CTL_CCFGSYS_LCRSINCE, &Data.LcrUsageSince);
			GetClusterData(CTL_CCFGSYS_FLAGS2, &Data.Flags2);
			{
				long   usehistobj = GetClusterData(CTL_CCFGSYS_OBJHISTF);
				SETFLAG(Data.Flags, CCFLG_USEHISTBILL, usehistobj & usehistobjBill);
				SETFLAG(Data.Flags, CCFLG_USEHISTGOODS, usehistobj & usehistobjGoods);
				SETFLAG(Data.Flags2, CCFLG2_USEHISTPERSON, usehistobj & usehistobjPerson);
				SETFLAG(Data.Flags2, CCFLG2_USEHISTSCARD, usehistobj & usehistobjSCard);
			}
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
						PPError(PPERR_DUPLSCARDFOUND, "many");
					}
				}
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY(SysParamsDialog, &Data);
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
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_DEF,      PPOBJ_PERSON, Data.MainOrgID,          OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_MAIN));
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_DIRECTOR, PPOBJ_PERSON, Data.MainOrgDirector_,   OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
						SetupPPObjCombo(dlg, CTLSEL_MAINORG_ACCTNT,   PPOBJ_PERSON, Data.MainOrgAccountant_, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_EMPL));
						if(ExecView(dlg) == cmOK) {
							dlg->getCtrlData(CTLSEL_MAINORG_DEF,      &Data.MainOrgID);
							dlg->getCtrlData(CTLSEL_MAINORG_DIRECTOR, &Data.MainOrgDirector_);
							dlg->getCtrlData(CTLSEL_MAINORG_ACCTNT,   &Data.MainOrgAccountant_);
						}
					}
					delete dlg;
				}
				break;
			case cmCommCfgOp:
				{
					TDialog * dlg = new TDialog(DLG_CCFGOP);
					if(CheckDialogPtrErr(&dlg)) {
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_SUPPLSH,    PPOBJ_ACCSHEET,  Data.SupplAccSheet, OLW_CANINSERT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_SELLSH,     PPOBJ_ACCSHEET,  Data.SellAccSheet,  OLW_CANINSERT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RCPTOP,     PPOBJ_OPRKIND,   Data.ReceiptOp,     OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RETAILOP,   PPOBJ_OPRKIND,   Data.RetailOp,      OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSEXPEND));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_RETAILRET,  PPOBJ_OPRKIND,   Data.RetailRetOp,   OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSRETURN));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_INTRRCPT,   PPOBJ_OPRKIND,   Data.IntrReceiptOp, OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_ACOP,       PPOBJ_OPRKIND,   Data.AutoComplOp,   OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_GOODSMODIF));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_DRAFTRCPT,  PPOBJ_OPRKIND,   Data.DraftRcptOp,   OLW_CANINSERT, reinterpret_cast<void *>(PPOPT_DRAFTRECEIPT));
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_DYNGTYPESA, PPOBJ_GOODSTYPE, Data.DynGoodsTypeForSupplAgent, OLW_CANINSERT);
						SetupPPObjCombo(dlg, CTLSEL_CCFGOP_PREPAYWARE, PPOBJ_GOODS, Data.PrepayInvoiceGoodsID, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 0, CCFLG_USENOUPDRESTOPFLAG);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 1, CCFLG_DISABLEACCTURN);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 2, CCFLG_IGNOREORDERSDEBT);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 3, CCFLG_NOADJPRWROFFDRAFT);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 4, CCFLG_DONTUNDOOPCNTRONESC);
						dlg->AddClusterAssoc(CTL_CCFGOP_FLAGS, 5, CCFLG_CHECKUNIQBILLCODE); // @v11.7.8
						dlg->SetClusterData(CTL_CCFGOP_FLAGS, Data.Flags);
						if(ExecView(dlg) == cmOK) {
							dlg->getCtrlData(CTLSEL_CCFGOP_SUPPLSH,   &Data.SupplAccSheet);
							dlg->getCtrlData(CTLSEL_CCFGOP_SELLSH,    &Data.SellAccSheet);
							dlg->getCtrlData(CTLSEL_CCFGOP_RCPTOP,    &Data.ReceiptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_RETAILOP,  &Data.RetailOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_RETAILRET, &Data.RetailRetOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_INTRRCPT,  &Data.IntrReceiptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_ACOP,      &Data.AutoComplOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_DRAFTRCPT, &Data.DraftRcptOp);
							dlg->getCtrlData(CTLSEL_CCFGOP_DYNGTYPESA, &Data.DynGoodsTypeForSupplAgent);
							dlg->getCtrlData(CTLSEL_CCFGOP_PREPAYWARE, &Data.PrepayInvoiceGoodsID);
							dlg->GetClusterData(CTL_CCFGOP_FLAGS, &Data.Flags);
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

int EditCommConfig()
{
	int    r;
	int    valid_data = 0;
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
int EditCurrConfig()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_CURRCFG);
	if(CheckDialogPtrErr(&dlg)) {
		PPID   main_org_id = GetMainOrgID();
		PPID   curr_loc = LConfig.Location;
		SetupPPObjCombo(dlg, CTLSEL_CURRCFG_MAINORG, PPOBJ_PERSON, main_org_id, 0, reinterpret_cast<void *>(PPPRK_MAIN));
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
