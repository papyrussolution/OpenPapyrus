// PPAPPO.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <pp.h>
#pragma hdrstop
#include <Lm.h>
#include <strstore.h>
//
// @prototype {
//
int SLAPI ExportDialogs(const char * pFileName);
int TestAddressRecognition();
int SLAPI SelfBuild();
int SLAPI ConvertCipher(const char * pDbSymb, const char * pMasterPassword, const char * pSrcIniFileName, const char * pDestIniFileName);
int SLAPI RemoveZeroLinkCCLines();
int SLAPI ReceiptPacking();
int SLAPI PPTex2Html();
int SLAPI PPVer2Html();
int SLAPI Test_KeywordListGenerator(); // txtanlz.cpp
int SLAPI TestFias(); // objloctn.cpp
int SLAPI TestCURL(); // pputil.cpp
int SLAPI TestEGAIS(const PPEgaisProcessor::TestParam *); // egais.cpp
int SLAPI BuildLocalDL600(); // ppbuild.cpp
int SLAPI TestLogWindow(); // pplog.cpp
int SLAPI TestTddo2(); // tddo2.cpp
int SLAPI TestVetis(); // vetis.cpp
int SLAPI TestMqc(); // ppmqc.cpp
int SLAPI RecoverAbsenceTrfr();
int SLAPI PPValidateXml();
int SLAPI OpenPhonePane();
int SLAPI UniFindObj();
//int SLAPI ImportSartre();
// } @prototype
//
//
/*static*/ const char * PPRegKeys::PrefSettings          = "Software\\Papyrus\\Pref";   // @declared(pp.h)
/*static*/ const char * PPRegKeys::PrefBasketSelSettings = "Software\\Papyrus\\Pref\\BasketSel"; // @declared(pp.h)
/*static*/ const char * PPRegKeys::SysSettings           = "Software\\Papyrus\\System";   // @declared(pp.h)
/*static*/ const char * PPRegKeys::Sessions              = "Software\\Papyrus\\Sessions"; // @declared(pp.h)
//
//
//
int PPHandlePrintError(int)
{
	int    reply = 0;
	TDialog * dlg = new TDialog(DLG_PRNERR);
	if(CheckDialogPtrErr(&dlg)) {
		SString text;
		if(PPLoadString(PPERR_SLIB, SLibError, text)) {
			text.Insert(0, "\003");
			dlg->setStaticText(CTL_PRNERR_TEXT, text);
			reply = BIN(ExecView(dlg) == cmOK);
		}
	}
	delete dlg;
	return reply;
}
//
// PPApp
//
PPApp::PPApp(HINSTANCE hInst, const char * pAppSymb, const char * pAppTitle) : TProgram(hInst, pAppSymb, pAppTitle), LocalState(0)
{
	SPrinter::HandlePrintError = PPHandlePrintError;
	STextBrowser::RegWindowClass(hInst); // @v9.1.3
	InitUiToolBox(); // @v9.5.6
	InitDeskTop();
	TEvent event;
	event.setCmd(cmInit, 0);
	handleEvent(event);
}

int PPApp::InitStatusBar()
{
	int    ok = TProgram::InitStatusBar();
	if(ok) {
		SString copyright_buf;
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetCopyrightText(copyright_buf);
		P_Stw->RemoveItem(-1);
		P_Stw->AddItem(copyright_buf);
		P_Stw->Update();
	}
	return 1;
}

/*virtual*/int PPApp::LoadVectorTools(TWhatmanToolArray * pT)
{
	int    ok = -1;
	SString src_file_name;
	if(PPGetFilePath(PPPATH_BIN, PPFILNAM_PPDV_WTA, src_file_name)) {
		if(fileExists(src_file_name)) {
			if(pT) {
                THROW(pT->Load(src_file_name));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

IMPL_HANDLE_EVENT(PPApp)
{
	if(TVCOMMAND) {
		if(TVCMD == cmQuit) {
			if(!TVINFOPTR || *static_cast<const long *>(TVINFOPTR) == -1 || CONFIRM(PPCFM_QUIT)) {
				UserInterfaceSettings ui_cfg;
				ui_cfg.Restore();
				SETFLAG(ui_cfg.Flags, ui_cfg.fShowLeftTree, IsTreeVisible());
				ui_cfg.Save();
				CALLPTRMEMB(P_Toolbar, SaveUserSettings(TV_GLBTOOLBAR));
				CloseAllBrowsers();
				DS.Logout();
			}
			else {
				clearEvent(event);
				return;
			}
		}
		else if(TVCMD == cmDBExit) {
			UserInterfaceSettings ui_cfg;
			ui_cfg.Restore();
			SETFLAG(ui_cfg.Flags, ui_cfg.fShowLeftTree, IsTreeVisible());
			ui_cfg.Save();
			CALLPTRMEMB(P_Toolbar, SaveUserSettings(TV_GLBTOOLBAR));
			CloseAllBrowsers();
			DS.Logout();
			login(1);
		}
	}
	TProgram::handleEvent(event);
	if(TVBROADCAST) {
		if(TVCMD == cmIdle)
			DS.ProcessIdle();
	}
	else if(TVCOMMAND) {
		if(TVCMD == cmTimeChange) {
			SetDatetimeByServer(0, 1);
			clearEvent(event);
		}
		else if(processCommand(TVCMD))
			clearEvent(event);
		else
			return;
	}
}

#if 0 // {
//
// јварийна€ функци€ восстановлени€ товаров
//
int SLAPI RestoreGoods2()
{
	char   fname[MAXPATH], out_fname[MAXPATH];
	GoodsCore goods_tbl;
	PPObjUnit unit_obj;
	PPObjPerson psn_obj;
	Goods2Tbl::Rec rec, temp_rec;
	long recs_count = 0, update_count = 0;
	long pos = 0;

	STRNSCPY(fname, goods_tbl.fileName);
	replaceExt(fname, ".BAD", 1);
	replaceExt(STRNSCPY(out_fname, fname), ".OUT", 1);
	FILE * in = fopen(fname, "rb");
	if(!in)
		return 0;
	FILE * out = fopen(out_fname, "wb");
	if(!out)
		return 0;

	MEMSZERO(rec);
	pos = ftell(in);
	while(fread(&rec, sizeof(rec), 1, in) == 1) {
		int c = 1;
		int bad_group = 0;
		uint i;
		if(rec.Kind == PPGDSK_GROUP && oneof9(rec.ID, 1, 5, 247, 49, 262, 271, 258, 248, 51)) {
			bad_group = 1;
		}
		if(!(rec.ID > 0 && rec.ID < 1000))
			c = 0;
		else if(rec.Kind != PPGDSK_GROUP && rec.Kind != PPGDSK_GOODS)
			c = 0;
		else if(!IsLetter866(rec.Name[0]) || !IsLetter866(rec.Name[1]) || !IsLetter866(rec.Name[2]))
			c = 0;
		else
			for(i = 0; c && i < sizeof(rec.Name); i++) {
				char * t = rec.Name+i;
				if(*t == 3 && t[1] == 0)
					*t = 0;
				if(*t == 10)
					*t = ' ';
				if(!IsLetter866(*t) && !isprint(*t) && *t != ' ' && *t != 0)
					c = 0;
			}
		if(c)
			for(i = 0; c && i < sizeof(rec.Name); i++) {
				char * t = rec.Abbr+i;
				if(*t == 3 && t[1] == 0)
					*t = 0;
				if(*t == 10)
					*t = ' ';
				if(!IsLetter866(*t) && !isprint(*t) && *t != ' ' && *t != 0)
					c = 0;
			}
		if(!bad_group && c) {
			PPID valid_parent_id = 0;
			if(goods_tbl.Search(rec.ParentID, &temp_rec) > 0 && temp_rec.Kind == PPGDSK_GROUP)
				valid_parent_id = rec.ParentID;
			if(goods_tbl.Search(rec.ID, &temp_rec) > 0) {
				if(strncmp(temp_rec.Name, "Goods Stub", sstrlen("Goods Stub")) == 0) {
					STRNSCPY(temp_rec.Name, rec.Name);
					STRNSCPY(temp_rec.Abbr, rec.Abbr);
					if(valid_parent_id)
						temp_rec.ParentID = valid_parent_id;
					if(goods_tbl.updateRecBuf(&temp_rec))
						update_count++;
				}
				c = 0;
			}
			if(c && !(rec.ParentID >= 0 && rec.ParentID < 1000))
				c = 0;
			if(c && rec.UnitID && unit_obj.Search(rec.UnitID) <= 0)
				c = 0;
			if(c && rec.PhUnitID && unit_obj.Search(rec.PhUnitID) <= 0)
				c = 0;
			/*
			else if(!(rec.UnitID >= 0 && rec.UnitID <= 1999))
				c = 0;
			else if(!(rec.PhUnitID >= 0 && rec.PhUnitID <= 1999))
				c = 0;
			*/
			if(c && !(rec.PhUPerU >= 0.0 && rec.PhUPerU <= 2.0))
				c = 0;
			if(c && rec.ManufID && psn_obj.Search(rec.ManufID) <= 0) {
				//c = 0;
				rec.ManufID = 0;
			}
			/*
			else if(!(rec.ManufID >= 0 && rec.ManufID <= 2000))
				c = 0;
			*/
			if(c && rec.StrucID != 0) {
				//c = 0;
				rec.StrucID = 0;
			}
			if(c && !(rec.TaxGrpID >= 0 && rec.TaxGrpID <= 1999)) {
				//c = 0;
				rec.TaxGrpID = 0;
			}
			if(c && rec.WrOffGrpID != 0) {
				//c = 0;
				rec.WrOffGrpID = 0;
			}
			if(c && rec.Flags & 0xffffe000L) {
				//c = 0;
				rec.Flags &= ~0xffffe000L;
			}
			if(c && rec.GdsClsID != 0) {
				//c = 0;
				rec.GdsClsID = 0;
			}
			if(c && rec.Reserve != 0) {
				//c = 0;
				rec.Reserve = 0;
			}
		}
		if(c || bad_group) {
			/*
			fprintf(out, "%d,", sizeof(rec));
			fwrite(&rec, sizeof(rec), 1, out);
			fwrite("\xD\xA", 2, 1, out);
			*/
			if(bad_group) {
				temp_rec = rec;
				MEMSZERO(rec);
				rec.ID = temp_rec.ID;
				rec.Kind = temp_rec.Kind;
				STRNSCPY(rec.Name, temp_rec.Name);
				if(temp_rec.UnitID && unit_obj.Search(temp_rec.UnitID) > 0)
					rec.UnitID = temp_rec.UnitID;
			}
			fprintf(out, "%6ld %2ld %-48s %-48s %6ld %4ld %4ld %4ld %8.4lf %6ld %4ld %4ld %4ld 0x%08lx %4ld %8ld",
				rec.ID, rec.Kind, rec.Name, rec.Abbr, rec.ParentID, rec.GoodsTypeID,
				rec.UnitID, rec.PhUnitID, rec.PhUPerU, rec.ManufID, rec.StrucID, rec.TaxGrpID,
				rec.WrOffGrpID, rec.Flags, rec.GdsClsID, rec.Reserve);
			if(goods_tbl.insertRecBuf(&rec)) {
				fprintf(out, " OK\n");
				recs_count++;
			}
			else {
				fprintf(out, " Error %d\n", BtrError);
			}
		}
		else {
			fseek(in, pos+1, SEEK_SET);
		}
		pos = ftell(in);
	}
	fclose(out);
	fclose(in);
	return 1;
}

#endif // } 0
//
static int SLAPI PPAbout()
{
	int    ok = 1;
	TDialog * dlg = new TDialog(DLG_ABOUT);
	if(CheckDialogPtrErr(&dlg)) {
		const int is_open = DS.CheckExtFlag(ECF_OPENSOURCE);
		char  c_buf[512];
		SString fmt, text;
		SString temp_buf;
		PPLicData lic;
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetVersionText(c_buf, sizeof(c_buf));
		PPGetLicData(&lic);
		{
			// @v9.3.6 if(dlg->getStaticText(CTL_ABOUT_VERSION, fmt) > 0)
			// @v9.3.6 	dlg->setStaticText(CTL_ABOUT_VERSION, text.Printf(fmt, c_buf));
			// @v9.3.6 {
			PPLoadString("entmanagementsys", temp_buf);
			text = temp_buf;
			vi.GetProductName(temp_buf);
			text.Space().Cat(temp_buf);
			PPLoadString("version", temp_buf);
			text.CatDiv('.', 2).Cat(temp_buf).Space().Cat(c_buf);
			dlg->setStaticText(CTL_ABOUT_VERSION, text);
			// } @v9.3.6
		}
		{
			uint   start_year = 0;
			uint   end_year = 0;
			vi.GetTeam(c_buf, sizeof(c_buf));
			vi.GetDevYears(&start_year, &end_year);
			text.Z().CatParStr("c").Space().Cat(c_buf).Space().Cat(start_year).CatChar('-').Cat(end_year);
			dlg->setStaticText(CTL_ABOUT_TEAM, text);
		}
		text = is_open ? 0 : lic.RegName;
		dlg->setCtrlString(CTL_ABOUT_LICNAME, text);
		text = is_open ? 0 : lic.RegNumber;
		dlg->setCtrlString(CTL_ABOUT_LICREG, text);
		dlg->setCtrlLong(CTL_ABOUT_LICCOUNT, is_open ? 1000 : lic.LicCount);
		dlg->setCtrlLong(CTL_ABOUT_FREELICCOUNT, lic.FreeLicCount);
		{
			text = 0;
			if(is_open)
				text = "OpenSource";
			else {
				if(lic.ExtFunc & PPLicData::effJobServer)
					text.Cat("JobServer");
				if(lic.ExtFunc & PPLicData::effEgais)
					text.CatDiv(' ', 1).Cat("EGAIS");
				if(lic.ExtFunc & PPLicData::effDebtorStat)
					text.CatDiv(' ', 1).Cat("DebtorStat");
			}
			dlg->setStaticText(CTL_ABOUT_LICADDON, text);
		}
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}

static void ShowTags_(PPID objType)
{
	ObjTagFilt ot_filt(objType, (objType ? 0 : ObjTagFilt::fObjTypeRoots|ObjTagFilt::fAnyObjects));
	ShowObjects(PPOBJ_TAG, &ot_filt);
}

TWindow * PPApp::FindPhonePaneDialog()
{
	const long res_id = DLG_PHNCPANE;
	for(TView * p = P_DeskTop->GetFirstView(); p != 0; p = p->nextView()) {
		if(p->IsConsistent() && p->GetSubSign() == TV_SUBSIGN_DIALOG && static_cast<const TDialog *>(p)->resourceID == res_id)
			return static_cast<TWindow *>(p);
	}
	return 0;
}

int PPApp::processCommand(uint cmd)
{
	LastCmd = cmd;
	PPObjBill * p_bobj = BillObj;
	switch(cmd) {
		case cmTextBrowserTest:      PPViewTextBrowser(0, 0, 0, -1);      break;
		case cmOpenTextFile:         PPEditTextFile(0);                   break;
		case cmQuit:                 TView::messageCommand(this, cmQuit); break;
		case cmInit:                 login(1);                            break;
		case cmAbout:                PPAbout();                           break;
		case cmaCalculate:           PPCalculator(0, 0);                  break;
		case cmEditConfig:           EditMainConfig();                    break;
		case cmGroups_n_Users:
			{
				PPObjSecur::ExtraParam param;
				param.Flags |= PPObjSecur::ExtraParam::fShowAll;
				ShowObjects(PPOBJ_USR, &param);
			}
			break;
		case cmUnit:                 ShowObjects(PPOBJ_UNIT, 0);              break;
		case cmCityStatus:           ShowObjects(PPOBJ_CITYSTATUS, 0);        break;
		case cmPersonKind:           ShowObjects(PPOBJ_PRSNKIND, 0);          break;
		case cmPersonStatus:         ShowObjects(PPOBJ_PRSNSTATUS, 0);        break;
		case cmPersonCategory:       ShowObjects(PPOBJ_PRSNCATEGORY, 0);      break;
		case cmBnkAccType:           ShowObjects(PPOBJ_BNKACCTYPE, 0);        break;
		case cmAccSheet:             ShowObjects(PPOBJ_ACCSHEET, 0);          break;
		case cmOprType:              ShowObjects(PPOBJ_OPRTYPE, 0);           break;
		case cmOprKind:              ShowObjects(PPOBJ_OPRKIND, 0);           break;
		case cmOpCounter:            ShowObjects(PPOBJ_OPCOUNTER, 0);         break;
		case cmBillStatus:           ShowObjects(PPOBJ_BILLSTATUS, 0);        break;
		case cmCountry:              ShowObjects(PPOBJ_COUNTRY, 0);           break;
		case cmEditWorld:            ShowObjects(PPOBJ_WORLD, 0);             break;
		case cmGoodsGroup:           ShowObjects(PPOBJ_GOODSGROUP, 0);        break;
		case cmGoods:                ShowObjects(PPOBJ_GOODS, 0);             break;
		case cmBrand:                ShowObjects(PPOBJ_BRAND, 0);             break;
		case cmPersonRelType:        ShowObjects(PPOBJ_PERSONRELTYPE, 0);     break;
		case cmGoodsType:            ShowObjects(PPOBJ_GOODSTYPE, 0);         break;
		case cmNamedGoodsStruc:      ShowObjects(PPOBJ_GOODSSTRUC, 0);        break;
		case cmAction:               ShowObjects(PPOBJ_ACTION, 0);            break;
		case cmViewQCert:            ShowObjects(PPOBJ_QCERT, 0);             break;
		case cmBarCodeStruc:         ShowObjects(PPOBJ_BCODESTRUC, 0);        break;
		case cmSmsAccount:           ShowObjects(PPOBJ_SMSPRVACCOUNT, 0);	  break;
		case cmVATBook:              ShowObjects(PPOBJ_VATBOOK, 0);           break;
		case cmFormula:              ShowObjects(PPOBJ_FORMULA, 0);           break;
		case cmELinkKind:            ShowObjects(PPOBJ_ELINKKIND, 0);         break;
		case cmQuotKind:             ShowObjects(PPOBJ_QUOTKIND, reinterpret_cast<void *>(QuotKindFilt::fAll)); break;
		case cmPersonOpKind:         ShowObjects(PPOBJ_PERSONOPKIND, 0);      break;
		case cmPersonEvent:          ShowObjects(PPOBJ_PERSONEVENT, 0);       break;
		case cmStaffList:            ShowObjects(PPOBJ_STAFFLIST2, 0);        break;
		case cmGoodsTax:             ShowObjects(PPOBJ_GOODSTAX, 0);          break;
		case cmBarcodePrinter:       ShowObjects(PPOBJ_BCODEPRINTER, 0);      break;
		case cmCurrency:             ShowObjects(PPOBJ_CURRENCY, 0);          break;
		case cmCurRateType:          ShowObjects(PPOBJ_CURRATETYPE, 0);       break;
		case cmGoodsClass:           ShowObjects(PPOBJ_GOODSCLASS, 0);        break;
		case cmWoodBreed:            ShowObjects(PPOBJ_WOODBREED, 0);         break;
		case cmPckgType:             ShowObjects(PPOBJ_PCKGTYPE, 0);          break;
		case cmGoodsBasket:          ShowObjects(PPOBJ_GOODSBASKET, 0);	      break;
		case cmBht:                  ShowObjects(PPOBJ_BHT, 0);               break;
		case cmSCardSeries:
			{
				SCardSeriesFilt scs_filt;
				scs_filt.Flags |= SCardSeriesFilt::fShowPassive;
				ShowObjects(PPOBJ_SCARDSERIES, &scs_filt);
			}
			break;
		case cmDraftWrOff:           ShowObjects(PPOBJ_DRAFTWROFF, 0);        break;
		case cmAssetWrOffGrp:        ShowObjects(PPOBJ_ASSTWROFFGRP, 0);      break;
		case cmAdvBillKind:          ShowObjects(PPOBJ_ADVBILLKIND, 0);       break;
		case cmAccounts:             ShowObjects(PPOBJ_ACCOUNT2, 0);           break;
		case cmAccPlan:              ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_BAL));     break;
		case cmAccRegister:          ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_REGISTER)); break;
		case cmAccOutbal:            ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_OBAL));    break;
		case cmAccAggr:              ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_AGGR));    break;
		case cmAccAlias:             ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_ALIAS));   break;
		case cmAccBudget:            ShowObjects(PPOBJ_ACCOUNT2, reinterpret_cast<void *>(ACY_BUDGET));  break;
		case cmDraftCreateRules:     ShowObjects(PPOBJ_DFCREATERULE, 0);      break;
		case cmEditGoodsInfo:        ShowObjects(PPOBJ_GOODSINFO, 0);         break;
		case cmDynamicObjs:          ShowObjects(PPOBJ_DYNAMICOBJS, 0);       break;
		case cmTouchScreen:          ShowObjects(PPOBJ_TOUCHSCREEN, 0);       break;
		case cmDateTimeRep:          ShowObjects(PPOBJ_DATETIMEREP, 0);       break;
		case cmDutySched:            ShowObjects(PPOBJ_DUTYSCHED, 0);         break;
		case cmLocPrinter:           ShowObjects(PPOBJ_LOCPRINTER, 0);        break;
		case cmSalCharge:            ShowObjects(PPOBJ_SALCHARGE, 0);         break;
		case cmStaffCalendar:        ShowObjects(PPOBJ_STAFFCAL, 0);          break;
		case cmFTPAcct:              ShowObjects(PPOBJ_INTERNETACCOUNT, reinterpret_cast<void *>(PPObjInternetAccount::filtfFtp)); break;
		case cmMailAcct:             ShowObjects(PPOBJ_INTERNETACCOUNT, 0/*interpret_cast<void *>(PPObjInternetAccount::filtfMail)*/); break;
		case cmMailAccount:          ShowObjects(PPOBJ_INTERNETACCOUNT, reinterpret_cast<void *>(PPObjInternetAccount::filtfMail)); break;
		case cmProcessor:            ShowObjects(PPOBJ_PROCESSOR, 0);         break;
		case cmNamedObjAssoc:        ShowObjects(PPOBJ_NAMEDOBJASSOC, 0);     break;
		case cmBizScore:             ShowObjects(PPOBJ_BIZSCORE, 0);          break;
		case cmUhttStore:            ShowObjects(PPOBJ_UHTTSTORE, 0);         break;
		case cmWorkbook:             ShowObjects(PPOBJ_WORKBOOK, 0);          break;
		case cmGoodsValRestr:        ShowObjects(PPOBJ_GOODSVALRESTR, 0);     break;
		case cmPallet:               ShowObjects(PPOBJ_PALLET, 0);            break;
		case cmRFIDDevice:           ShowObjects(PPOBJ_RFIDDEVICE, 0);        break;
		case cmDebtDim:              ShowObjects(PPOBJ_DEBTDIM, 0);           break;
		case cmEventToken:           ShowObjects(PPOBJ_EVENTTOKEN, 0);        break;
		case cmEventSubscription:    ShowObjects(PPOBJ_EVENTSUBSCRIPTION, 0); break; // @v10.8.9
		case cmPhoneService:         ShowObjects(PPOBJ_PHONESERVICE, 0);      break;
		case cmGenericDevice:        ShowObjects(PPOBJ_GENERICDEVICE, 0);     break;
		case cmCafeTable:            ShowObjects(PPOBJ_CAFETABLE, 0);         break;
		case cmEdiProvider:          ShowObjects(PPOBJ_EDIPROVIDER, 0);       break;
		case cmTimeSeries:           ShowObjects(PPOBJ_TIMESERIES,  0);       break; // @v10.2.3
		case cmTssModel:             ShowObjects(PPOBJ_TSSMODEL,    0);       break; // @v10.7.4
		case cmFreightPackageType:   ShowObjects(PPOBJ_FREIGHTPACKAGETYPE, 0); break; // @v10.4.1
		case cmTaxSystemKind:        ShowObjects(PPOBJ_TAXSYSTEMKIND, 0);     break; // @v10.6.0
		// @v9.6.1 case cmAccArticle:           ViewArticle(0);                          break;
		// @v9.6.1 case cmPerson:               ViewPerson(0);                           break;
		case cmObjTag:               ShowTags_(PPOBJ_PERSON);                 break;
		case cmLotTag:               ShowTags_(PPOBJ_LOT);                    break;
		case cmBillTag:              ShowTags_(PPOBJ_BILL);                   break;
		case cmGoodsTag:             ShowTags_(PPOBJ_GOODS);                  break;
		case cmLocationTag:          ShowTags_(PPOBJ_LOCATION);               break;
		case cmGlobalUserAccTag:     ShowTags_(PPOBJ_GLOBALUSERACC);          break;
		case cmUhttStoreTag:         ShowTags_(PPOBJ_UHTTSTORE);              break;
		case cmObjTagAllObj:         ShowTags_(0);                            break;
		case cmAccArticle:           PPView::Execute(PPVIEW_ARTICLE,         0, PPView::exefModeless, 0); break; // @v9.6.1
		case cmPerson:               PPView::Execute(PPVIEW_PERSON,          0, PPView::exefModeless, 0); break; // @v9.6.1
		case cmPersonRel:            PPView::Execute(PPVIEW_PERSONREL,       0, PPView::exefModeless, 0); break;
		case cmStaff:                PPView::Execute(PPVIEW_STAFFPOST,       0, PPView::exefModeless, 0); break;
		case cmViewSalary:           PPView::Execute(PPVIEW_SALARY,          0, PPView::exefModeless, 0); break;
		case cmPrcBusy:              PPView::Execute(PPVIEW_PRCBUSY,         0, PPView::exefModeless, 0); break;
		case cmViewStaffCal:         PPView::Execute(PPVIEW_STAFFCAL,        0, PPView::exefModeless, 0); break;
		case cmViewObjSync:          PPView::Execute(PPVIEW_OBJSYNC,         0, PPView::exefModeless, 0); break;
		case cmViewObjSyncQueue:     PPView::Execute(PPVIEW_OBJSYNCQUEUE,    0, PPView::exefModeless, 0); break;
		case cmSStat:                PPView::Execute(PPVIEW_SSTAT,           0, PPView::exefModeless, 0); break;
		case cmSStatSupplOrder:      PPView::Execute(PPVIEW_SSTAT,           0, PPView::exefModeless, reinterpret_cast<void *>(1)); break;
		case cmFreight:              PPView::Execute(PPVIEW_FREIGHT,         0, PPView::exefModeless, 0); break;
		case cmViewCSessDeficit:     PPView::Execute(PPVIEW_CSESSEXC,        0, PPView::exefModeless, 0); break;
		case cmSellTrnovr:           PPView::Execute(PPVIEW_DEBTTRNOVR,      0, PPView::exefModeless, 0); break;
		case cmDebtSupplTrnovr:      PPView::Execute(PPVIEW_DEBTTRNOVR,      0, PPView::exefModeless, reinterpret_cast<void *>(1)); break;
		case cmProject:              PPView::Execute(PPVIEW_PROJECT,         0, PPView::exefModeless, 0); break;
		case cmViewGoodsStruc:       PPView::Execute(PPVIEW_GOODSSTRUC,      0, PPView::exefModeless, 0); break;
		case cmViewCurRates:         PPView::Execute(PPVIEW_CURRATE,         0, PPView::exefModeless, 0); break;
		case cmObjLikeness:          PPView::Execute(PPVIEW_OBJLIKENESS,     0, PPView::exefModeless, reinterpret_cast<void *>(1)); break;
		case cmPriceList:            PPView::Execute(PPVIEW_PRICELIST,       0, PPView::exefModeless, 0); break;
		case cmViewCSess:            PPView::Execute(PPVIEW_CSESS,           0, PPView::exefModeless, 0); break;
		case cmPriceAnlz:            PPView::Execute(PPVIEW_PRICEANLZ,       0, PPView::exefModeless, 0); break;
		case cmReport:               PPView::Execute(PPVIEW_REPORT,          0, PPView::exefModeless, 0); break;
		case cmBizScoreVal:          PPView::Execute(PPVIEW_BIZSCOREVAL,     0, PPView::exefModeless, 0); break;
		case cmDebtorStat:           PPView::Execute(PPVIEW_DEBTORSTAT,      0, PPView::exefModeless, 0); break;
		case cmStyloPalm:            PPView::Execute(PPVIEW_PALM,            0, PPView::exefModeless, 0); break;
		case cmLocTransf:            PPView::Execute(PPVIEW_LOCTRANSF,       0, PPView::exefModeless, 0); break;
		case cmViewBBoard:           PPView::Execute(PPVIEW_SERVERSTAT,      0, PPView::exefModeless, 0); break;
		case cmViewAsset:            PPView::Execute(PPVIEW_ASSET,           0, PPView::exefModeless, 0); break;
		case cmTransport:            PPView::Execute(PPVIEW_TRANSPORT,       0, PPView::exefModeless, 0); break;
		case cmPaymPlan:             PPView::Execute(PPVIEW_PAYMPLAN,        0, PPView::exefModeless, 0); break;
		case cmAmountType:           PPView::Execute(PPVIEW_AMOUNTTYPE,      0, PPView::exefModeless, 0); break;
		case cmRegisterType:         PPView::Execute(PPVIEW_REGISTERTYPE,    0, PPView::exefModeless, 0); break;
		case cmStockOpt:             PPView::Execute(PPVIEW_STOCKOPT,        0, PPView::exefModeless, 0); break;
		case cmViewLogsMon:          PPView::Execute(PPVIEW_LOGSMONITOR,     0, PPView::exefModeless, 0); break;
		case cmBudgeting:            PPView::Execute(PPVIEW_BUDGET,          0, PPView::exefModeless, 0); break;
		case cmViewBizScTempl:       PPView::Execute(PPVIEW_BIZSCTEMPL,      0, PPView::exefModeless, 0); break;
		case cmBizScValByTempl:      PPView::Execute(PPVIEW_BIZSCVALBYTEMPL, 0, PPView::exefModeless, 0); break;
		case cmViewDLStat:	 	     PPView::Execute(PPVIEW_DVCLOADINGSTAT,  0, PPView::exefModeless, 0); break;
		case cmSpecSeries:           PPView::Execute(PPVIEW_SPECSERIES,      0, PPView::exefModeless, 0); break;
		case cmGtaJournal:           PPView::Execute(PPVIEW_GTAJOURNAL,      0, PPView::exefModeless, 0); break;
		case cmSCardOp:              PPView::Execute(PPVIEW_SCARDOP,         0, PPView::exefModeless, 0); break;
		case cmSuprWare:             PPView::Execute(PPVIEW_SUPRWARE,        0, PPView::exefModeless, 0); break;
		case cmUserProfile:		     PPView::Execute(PPVIEW_USERPROFILE,     0, PPView::exefModeless, 0); break;
		case cmAccTurnList:          PPView::Execute(PPVIEW_ACCTURN,         0, PPView::exefModeless, 0); break;
		case cmBalance:              PPView::Execute(PPVIEW_BALANCE,         0, PPView::exefModeless, 0); break;
		case cmShipmAnalyze:         PPView::Execute(PPVIEW_SHIPMANALYZE,    0, PPView::exefModeless, 0); break;
		case cmGoodsGrpRlz:          PPView::Execute(PPVIEW_GOODSTAXANALYZE, 0, PPView::exefModeless, 0); break;
		case cmViewCheckOpJ:         PPView::Execute(PPVIEW_CHECKOPJRNL,     0, PPView::exefModeless, 0); break;
		case cmJobPool:              PPView::Execute(PPVIEW_JOB,             0, PPView::exefModeless, 0); break;
		case cmGlobalUserAcc:        PPView::Execute(PPVIEW_GLOBALUSERACC,   0, PPView::exefModeless, 0); break;
		case cmViewGeoTracking:      PPView::Execute(PPVIEW_GEOTRACKING,     0, PPView::exefModeless, 0); break;
		case cmCashNode:             PPView::Execute(PPVIEW_CASHNODE,        0, PPView::exefModeless, 0); break;
		case cmScale:                PPView::Execute(PPVIEW_SCALE,           0, PPView::exefModeless, 0); break;
		case cmDBDiv:                PPView::Execute(PPVIEW_DBDIV,           0, PPView::exefModeless, 0); break;
		case cmGoodsRest:            PPView::Execute(PPVIEW_GOODSREST,       0, PPView::exefModeless, 0); break;
		case cmGetPrognosis:         PPView::Execute(PPVIEW_GOODSREST,       0, PPView::exefModeless, reinterpret_cast<void *>(1)); break;
		case cmViewPhnSvcMonitor:    PPView::Execute(PPVIEW_PHNSVCMONITOR,   0, PPView::exefModeless, 0); break; // @v9.9.10
		case cmVetisDocument:        PPView::Execute(PPVIEW_VETISDOCUMENT,   0, PPView::exefModeless, 0); break; // @v10.0.12
		case cmQuoteReqAnalyze:      PPView::Execute(PPVIEW_QUOTEREQANALYZE, 0, PPView::exefModeless, 0); break; // @v10.5.7
		case cmVetisInterchange:     PPViewVetisDocument::RunInterchangeProcess(0); break; // @v10.1.2
		case cmEvents:               PPView::Execute(PPVIEW_EVENT,           0, PPView::exefModeless, 0); break; // @v10.8.9
		case cmTimeSeriesSA: // @construction
			break;
		case cmCTblOrdTimeGraph:
			{
				CTableOrder::Param param;
				CTableOrder cto;
				param.Flags |= param.fShowTimeGraph;
				if(cto.EditParam(&param) > 0) {
					CTableOrder::ShowTimeGraph(param.PosNodeID, 1);
				}
			}
			break;
		case cmCreateDraftBySupplOrder: PrcssrBillAutoCreate::CreateDraftBySupplOrders(0); break;
		case cmCreateObjLikeness:    PPViewObjLikeness::CreateLikenessTable();  break;
		case cmChargeSalary:         DoChargeSalary();                        break;
		case cmBizScoreCalc:         DoBizScore(0);                           break;
		case cmFastGrp_n_Users:      FastEditRightsDialog();                  break;
		case cmMainOrg2:             EditMainOrg();                           break;
		case cmDefaultCliAgt:        PPObjArticle::DefaultClientAgreement();  break;
		case cmDefaultSupplAgt:      PPObjArticle::DefaultSupplAgreement();   break;
		case cmGoods2LocAssoc:       ViewGoodsToLocAssoc(0, PPASS_GOODS2LOC, 0, 0); break;
		case cmGoods2SupplAssoc:     ViewGoodsToObjAssoc(PPOBJ_ARTICLE,  0, PPASS_GOODS2SUPPL, reinterpret_cast<void *>(GetSupplAccSheet())); break;
		case cmGoods2CashNodeAssoc:  ViewGoodsToObjAssoc(PPOBJ_CASHNODE, 0, PPASS_GOODS2CASHNODE, 0); break;
		case cmAccAnalyze:           ViewAccAnlz(0, aakndGeneric);            break;
		case cmCashBook:             ViewAccAnlz(0, aakndCashBook);           break;
		case cmSupplTrnovr:          ViewAccAnlz(0, aakndSupplTrnovr);        break;
		case cmLocation:             PPObjLocation::ViewWarehouse();          break;
		case cmDivision:             PPObjLocation::ViewDivision();           break;
		case cmSumByDivision:        FastEditSumByDivDialog();            break;
		case cmDivisionBySum:        FastViewDivBySumDialog();            break;
		case cmSelectLocation:       EditCurrConfig();                    break;
		case cmGoodsBillList:        BrowseBills(bbtGoodsBills);          break;
		case cmGoodsOrder:           BrowseBills(bbtOrderBills);          break;
		case cmAccturnBillList:      BrowseBills(bbtAccturnBills);        break;
		case cmInventory:            BrowseBills(bbtInventoryBills);      break;
		case cmPoolBillList:         BrowseBills(bbtPoolBills);           break;
		case cmDraftBillList:        BrowseBills(bbtDraftBills);          break;
		case cmWmsBillList:          BrowseBills(bbtWmsBills);            break;
		case cmAutoCharge:           p_bobj->AutoCharge();               break;
		case cmCorrectBillPaym:      p_bobj->GatherPayments();           break;
		case cmPaymWOLinkBill:       p_bobj->SearchPaymWOLinkBill();     break;
		case cmCorrectTrfrRvrs:      p_bobj->trfr->CorrectReverse();     break;
		case cmCorrectLotTaxGrp:     p_bobj->trfr->CorrectLotTaxGrp();   break;
		case cmRecalcLotCurRest:     p_bobj->trfr->RecalcLcr();          break;
		case cmCorrectTrfrIntrUnite: p_bobj->trfr->CorrectIntrUnite();   break;
		case cmCorrectPckgCloseTags: p_bobj->CorrectPckgCloseTag();      break;
		case cmCorrectBalance:       p_bobj->atobj->CorrectBalance();    break;
		case cmCorrectAccrelArRefs:  p_bobj->atobj->CorrectRelsArRefs(); break;
		case cmCorrectGoodsGroups:   RecoverGoodsGroups(0);               break;
		case cmCorrectGoodsGroupsNIA: RecoverGoodsGroupsNIA();            break;
		case cmCorrectWorldObjNIA:   PPObjWorld::Recover();               break; // @v10.8.0
		case cmCheckGStrucRecursion: PPObjGoodsStruc::CheckRecursion();   break;
		case cmWriteOffDrafts:       WriteOffDrafts(0);                 break;
		case cmViewLots:             ViewLots(0, 0, 1);                 break;
		case cmViewOrdLots:          ViewLots(0, 1, 1);                 break;
		case cmTrfrAnalyze:          ViewTrfrAnlz(0);                   break;
		case cmGoodsTurnover:        ViewGoodsTurnover(0);              break;
		case cmGoodsRestTest:        GoodsRestTest();                   break;
		case cmPredictSales:         ViewPredictSales(0);               break;
		case cmFillPredictSalesTbl:  FillPredictSales();                break;
		case cmTestPredictSalesTbl:  TestPredictSales();                break;
		case cmTestReconnect:        TestReconnect();                   break;
		case cmGoodsMov:             ViewGoodsMov(1);                   break;
		case cmGoodsOprGrouping:     ViewOpGrouping(0);                 break;
		case cmGoodsOper:            ViewGoodsOpAnalyze(0);             break;
		case cmCashBillList:         ViewCashBills(0);                  break;
		case cmCloseCashDay:         CloseCashDay();                    break;
		case cmViewSysJ:             ViewSysJournal(0, 1);              break;
		case cmSCard:                ViewSCard(0, 1);                   break;
		case cmSetSCardsByRule:      PPObjSCardSeries::SetSCardsByRule(0); break;
		case cmMrpTab:               ViewMrpTab(0);                   break;
		case cmPrjTask:              ViewPrjTask(0);                  break;
		case cmPrjTask_ByStatus:     ViewPrjTask_ByStatus();          break;
		case cmPrjTask_ByReminder:   ViewPrjTask_ByReminder();        break;
		case cmMaintainPrjTask:      MaintainPrjTask();               break;
		case cmGotoSite:             TProgram::GotoSite();            break;
		//case cmGotoVK:               GotoVK();                        break; //@erikTEMP v10.7.7 		case cmOpenPhonePane:        OpenPhonePane();                 break; // @v10.5.8
		case cmViewNewVersionList:   PPUhttClient::ViewNewVerList(1); break;
		case cmViewQuots:            ViewQuot(0);                     break;
		case cmTech:                 ViewTech(0);                     break;
		case cmTSession:             ViewTSession(0);                 break;
		case cmManufPlan:            ViewManufPlan(0);                break;
		case cmTSessWrOffOrder:      PPObjTSession::EditWrOffOrder(); break;
		case cmUnifyGoodsPrice:      UnifyGoodsPrice();               break;
		case cmUpdateQuots:          UpdateQuots(0);                  break;
		case cmRollbackQuots:        RollbackQuots(0);                break; // v10.5.8 @erik
		case cmCommConfig:           EditCommConfig();                break;
		case cmCashNodePanel:        ExecCSPanel(0);                  break;
		case cmRemoveEmptyAcctRels:  RemoveEmptyAcctRels();           break;
		case cmSelfSync:             SynchronizeObjects(0);           break;
		case cmSendBills:            PPObjectTransmit::TransmitBillsByDBDivList(0);   break;
		case cmSendBillsByFilt:      PPViewBill::TransmitByFilt(0, 0); break;
		case cmTransmitModifications: PPObjectTransmit::TransmitModificationsByDBDivList(0); break;
		case cmReceivePackets:       PPObjectTransmit::ReceivePackets(0);  break;
		case cmReplacePerson:        PPObjPerson::ReplacePerson();      break;
		case cmReplaceGoods:
			{
				PPObjGoods::ExtUniteBlock eub;
				PPObjGoods::ReplaceGoods(eub);
			}
			break;
		case cmReplaceGoodsTaxGrp:   PPObjGoodsTax::ReplaceGoodsTaxGrp(); break;
		case cmReplaceUnit:          PPObject::ReplaceObjInteractive(PPOBJ_UNIT, 0);  break;
		case cmReplaceWorld:         PPObject::ReplaceObjInteractive(PPOBJ_WORLD, 0); break;
		case cmReplacePersonCat:     PPObject::ReplaceObjInteractive(PPOBJ_PRSNCATEGORY, 0); break;
		case cmReplaceLikeWorld:     PPObjWorld::UniteMaxLike();        break;
		case cmReplaceDynamicObj:    PPObject::ReplaceObjInteractive(PPOBJ_DYNAMICOBJS, 0);   break;
		case cmReplaceBrand:         PPObject::ReplaceObjInteractive(PPOBJ_BRAND, 0); break;
		case cmUniFindObj:           UniFindObj();                      break; // @v10.6.1 @construction
		case cmPrinterCfg:           EditDefaultPrinterCfg();           break;
		case cmGoodsCfg:             PPObjGoods::EditConfig();          break;
		case cmPersonCfg:            PPObjPerson::EditConfig();         break;
		case cmStyloPalmCfg:         PPObjStyloPalm::EditConfig();      break;
		case cmPredictCfg:           PrcssrPrediction::EditPredictCfg(); break;
		case cmEditHolidays:         EditHolidays();                    break;
		case cmUserInterfaceCfg:	 UISettingsDialog();                break;
		case cmBillCfg:              PPObjBill::EditConfig();           break;
		case cmProjectCfg:           PPObjProject::EditConfig();        break;
		case cmTSessionCfg:          PPObjTSession::EditConfig();       break;
		case cmSCardCfg:             PPObjSCard::EditConfig();          break;
		case cmPriceListCfg:         EditPriceListConfig();             break;
		case cmDBXchgCfg:            PPObjectTransmit::EditConfig();    break;
		case cmTransportCfg:         PPObjTransport::EditConfig();      break;
		case cmVATBookSellCfg:       PPObjVATBook::EditConfig(PPVTB_SELL, 0); break;
		case cmVATBookBuyCfg:        PPObjVATBook::EditConfig(PPVTB_BUY,  0); break;
		case cmSimpleLedgerCfg:      PPObjVATBook::EditConfig(PPVTB_SIMPLELEDGER, 0);  break;
		case cmInetConnCfg:          PpyInetDataPrcssr::EditCfg();      break;
		case cmEquipCfg:             EditEquipConfig();                 break;
		case cmDebtorStatCfg:        PPDebtorStatConfig::Edit();        break;
		case cmLocationCfg:          PPObjLocation::EditConfig();       break;
		case cmStockOptCfg:          PPStockOpt::EditConfig();          break;
		case cmGlobalAccCfg:         PPObjGlobalUserAcc::EditConfig();  break;
		case cmWorkbookCfg:          PPObjWorkbook::EditConfig();       break;
		case cmAlcReportCfg:         PrcssrAlcReport::EditConfig();     break;
		case cmTimeSeriesConfig:     PPObjTimeSeries::EditConfig(0);    break; // @v10.3.1
		case cmResetOpCounters:      PPObjOpCounter::ResetAll();        break;
		case cmCorrectLotsCloseTags: CorrectLotsCloseTags();            break;
		case cmCorrectLotSuppl:      CorrectLotSuppl();                 break;
		case cmCreateReservedObjs:   PPObject::CreateReservedObjects(0);  break;
		case cmCheckPane:            CCheckPane(0, 0);                    break;
		case cmViewCCheck:           ViewCCheck(0, PPView::exefModeless); break;
		case cmSCardPane:            ViewSCardInfo(0, 0, 0);            break;
		case cmScalePrepareData:     PPObjScale::PrepareData(0);        break;
		case cmScaleTransmitData:    PPObjScale::TransmitData(0);       break;
		case cmUpdatePassword:       UpdatePassword();                  break;
		case cmUpdateLicense:        PPLicUpdate();                     break;
		case cmRegistration:         PPLicRegister();                   break;
		case cmViewStatus:           ViewStatus();                      break;
		case cmDatabaseCutting:      DatabaseCutting();                 break;
		case cmCurReval:             CurReval();                        break;
		case cmSaveBalance:          SaveBalance();                     break;
		case cmRestoreBalance:       RestoreBalance();                  break;
		case cmMakeCRptFiles:        MakeCRptDataFiles();               break;
		case cmVerifyAllCRpt:        MakeCRptDataFiles(1);              break;
		case cmCorrectAccTurnRest:   CorrectAccTurnRest();              break;
		case cmCorrectCurRest:       CorrectCurRest();                  break;
		case cmRecalcBillTurns:      RecalcBillTurns(0);                break;
		case cmCheckBillAmounts:     RecalcBillTurns(1);                break;
		case cmCorrectZeroQCertRefs: CorrectZeroQCertRefs();            break;
		case cmCorrectZeroDebtDimRefs: CorrectZeroDebtDimRefs();        break; // @v10.2.8
		case cmCorrectAbsenceLots:   RecoverAbsenceLots();              break;
		case cmCorrectAbsenceGoods:  RecoverAbsenceGoods();             break;
		case cmRecoverAbsenceBills:  RecoverAbsenceBills();             break;
		case cmRecoverAbsenceAccounts: RecoverAbsenceAccounts();        break;
		case cmRecoverAbsencePrjTaskPerson: PPObjPrjTask::RecoverAbsencePerson(); break;
		case cmCorrectAccturn:        CorrectAccturn();                  break;
		case cmRecover_v242:          ViewOpersByLot(0, 1);              break;
		case cmRemoveBadReckons:      RemoveBadReckons();                break;
		case cmImportGoodsGrp:        Import(PPOBJ_GOODSGROUP);          break;
		case cmImportGoodsOld:        Import(PPOBJ_GOODS, 1);            break;
		case cmImportGoods:           Import(PPOBJ_GOODS);               break;
		case cmImportQuot:            Import(PPOBJ_QUOT, 1);             break;
		case cmImportQuot2:           Import(PPOBJ_QUOT);                break;
		case cmImportPerson:          Import(PPOBJ_PERSON, 0);           break;
		case cmImportPerson2:         PPObjPerson().Import();            break;
		case cmImportSuppl:           Import(PPOBJ_PERSON, PPPRK_SUPPL); break;
		case cmImportCountry:         Import(PPOBJ_COUNTRY);             break;
		case cmImportSCard:           Import(PPOBJ_SCARD);               break;
		case cmImportSCard2:          ImportSCard();                     break;
		case cmImportBanks:           ImportBanks();                     break;
		case cmImportKLADR:           ImportKLADR();                     break;
		case cmImportInventory:       ImportInventory();                 break;
		case cmImportSR25:            ImportSR25();                      break; // @vmiller
		case cmImportSuprWare:        ImportCompGS();                    break; // @vmiller
		case cmCfgImpSpecSeries:      EditSpecSeriesFormatDescription(); break;
		case cmImportSpecSeries:      ImportSpecSeries();                break;
		case cmImportTodoFromVCal:    PPObjPrjTask::ImportFromVCal();    break;
		case cmImportPhoneList:       ImportPhoneList();                 break;
		case cmImportWorkbook:        ImportWorkbook();                  break;
		//case cmImportFias:            ImportFias();                      break;
		case cmImportTSessUHTT:       PPObjTSession().ImportUHTT();      break;
		case cmImportPosRefs:
			{
				PPPosProtocol pp;
				PPPosProtocol::ProcessInputBlock pib;
				pib.Flags |= (pib.fProcessRefs|pib.fBackupProcessed|pib.fRemoveProcessed|pib.fProcessQueries);
				pp.ProcessInput(pib);
			}
			break; // @v9.0.8
		case cmProcessSartre:         DoProcessSartre(0);                break;
		case cmChainDatabase:         SetDatabaseChain();                break;
		case cmDeleteTmpFiles:        DeleteTmpFiles();                  break;
		case cmCreateNewFiles:        MakeDatabase();                    break;
		case cmViewGoodsInfo:         ViewGoodsInfo(0);                  break;
		case cmBhtTransmit:           PPObjBHT::TransmitData();          break;
		case cmBhtTransmitProgram:    PPObjBHT::TransmitProgram();       break;
		case cmBhtReceive:            PPObjBHT::ReceiveData();           break;
		case cmStyloPalmImpExp:       PPObjStyloPalm::ImpExp(0);         break;
		case cmRevalAssets:           RevalAssets();                     break;
		case cmActiveUsersList:       ActiveUsersListDialog();           break;
		// @v9.3.8 @obsolete case cmImportOrders:          ImportOrders();                    break;
		case cmAlbatrosCfg:           PPAlbatrosCfgMngr::Edit();         break;
		case cmProcessDL200:          ProcessDL200();                    break;
		case cmReceiveCharry:         ReceiveCharryObjects(0);           break;
		case cmMailCharryFiles:       SendCharryFiles();                 break;
		case cmAnalyzeObjSyncCmp:     AnalyzeObjSyncCmp(1);              break;
		case cmCalcGoodsSaldo:        ProcessGoodsSaldo();               break;
		// @v9.7.10 @obsolete case cmImportCurrList:        ImportCurrencyList();              break;
		case cmSupplGoodsImport:      SupplGoodsImport();                break;
		case cmSupplInterchange:      DoSupplInterchange(0);             break;
		case cmAddressBook:           ViewAddressBook();                 break;
		case cmCliBnkFmtCfg:		  SetupCliBnkFormats();		         break;
		case cmCliBnkAsCfg:		 	  SetupCliBnkAssoc();	             break;
		case cmCliBnkImport:	 	  CliBnkImport();			         break;
		case cmDBMaintain:            DoDBMaintain(0);                   break;
		case cmDBMaintainCfg:         EditDBMaintainCfg();               break;
		case cmViewLogs:              ViewLogs();                        break;
		case cmPrcPane:               ExecPrcPane(0);                    break;
		case cmRemoveZeroLinkCCLines: RemoveZeroLinkCCLines();           break;
		case cmEditDesktops:          EditMenus(0, 0, 1);                break;
		case cmSelDesktop:
			if(SelectMenu(&DS.GetTLA().Lc.DesktopID, 0, SELTYPE_DESKTOP, 0) > 0)
				PPDesktop::Open(LConfig.DesktopID);
			break;
		case cmEditMenus:             EditMenus(0, 0, 0);              break;
		case cmEditMenusFromFile:     EditMenusFromFile();             break;
		case cmEditWhatman:           PPWhatmanWindow::Edit(0, 0);     break;
		case cmBillExpImpParams:      EditBillImpExpParams(1);         break;
		case cmBRowExpImpParams:      EditBillImpExpParams(0);         break;
		case cmPriceListExpImpParams: EditPriceListImpExpParams();     break;
		case cmCashSessExpImpParams:
			// @v9.1.3 EditCSessImpExpParams();
			break;
		case cmExpImpParamsInventory: EditInventoryImpExpParams();     break;
		case cmGoodsExpImpParams:     EditGoodsImpExpParams();         break;
		case cmImpExpListConfig:      EditImpExpConfigs();             break;
		case cmImportBills:           ImportBills(0, 0, 0, 0);         break;
		case cmImportEmailAccts:      ImportEmailAccts();              break;
		case cmLoadFonts:             BarcodeLabelPrinter::LoadFonts(0, 0);  break;
		case cmRegisterComServer:     RegisterComServer();             break;
		case cmCreateGlobalUserAcc:   CreateBizScGlblUserAcct();       break;
		case cmKeyboardWKeyCfg:       EditDueToKeyboardRights();       break;
		case cmGenerateGoodsBills:    GenerateGoodsBills();            break;
		case cmGenerateCliBnkImport:  GenerateCliBnkImpData();         break;
		case cmCorrectTransfer:       RecoverTransfer();               break;
		case cmDebtRate:              DoDebtRate();                    break;
		case cmUpdatePpIni:
			{
				SString src_file_name;
				PPGetFilePath(PPPATH_IN, PPFILNAM_PPUPD_INI, src_file_name);
				PPIniFile ini_file(0, 0, 0, 1);
				int    r = ini_file.UpdateFromFile(src_file_name);
				if(r > 0) {
					ini_file.FlashIniBuf();
					PPMessage(mfInfo|mfOK, PPINF_PPINIUPDATED);
				}
				else if(r < 0)
					PPMessage(mfInfo|mfOK, PPINF_PPININOTUPDATED, src_file_name);
				else
					PPError();
			}
			break;
		case cmSearchDlvrAddr: SearchDlvrAddr(); break;
		case cmLogin:          login(0); break;
		case cmDBMaintenance:  DBMaintenance(0, 0); break;
		case cmPrivateBasket:
			if(DS.GetPrivateBasket()) {
				PPBasketCombine bc;
				bc.Pack = *DS.GetPrivateBasket();
				bc.BasketID = bc.Pack.Head.ID;
				GoodsBasketDialog(bc, 1);
			}
			break;
		case cmSetupPreferredBarcodeTags: PPObjGoods().SetupPreferredBarcodeTags(); break;
		case cmQueryEGAIS: PPEgaisProcessor(PPEgaisProcessor::cfUseVerByConfig, 0, 0).InteractiveQuery(); break;
		case cmQueryCHZN: PPChZnPrcssr(0).InteractiveQuery(); break;
		case cmFiltPool: ViewFiltPool(); break; // @paul pentaho
		case cmIndexPhones: PPObject::IndexPhones(); break;
		case cmIndexTextObjects:
			{
				MemLeakTracer mlt;
				PPTextAnalyzer taz;
				taz.Test();
			}
			break;
		case cmWorkbookInterchange: PPObjWorkbook().InterchangeUhtt(); break; // @v9.3.10
		case cmProcessObjText:     DoProcessObjText(0); break;
		case cmGenerateKeywordSeq: Test_KeywordListGenerator(); break;
		case cmLotPacking:         ReceiptPacking();        break;
		case cmCvtQCert:           ConvertLandQCertToLotTag(); break;
		case cmCorrectCCheckLine01:     CCheckCore().CorrectCCLineProblem01(); break;
		case cmRecoverLostUnifiedLinks: PPObjTag::RecoverLostUnifiedLinks(); break;
		case cmBuildLocalDL600:         BuildLocalDL600(); break;
		case cmValidateXml:             PPValidateXml(); break;
		case cmDumpData:                DoDbDump(0); break; // @v10.0.05
		case cmConstructionTest:        DoConstructionTest(); break;
		// @v10.5.12 case cmTestUhttGetLocByPhone:   PPUhttClient::TestUi_GetLocationListByPhone(); break;
		// @v10.5.12 case cmTestUhttGetQuotByLoc:    PPUhttClient::TestUi_GetQuotByLoc(); break;
		case cmTestFias:                TestFias(); break;
		case cmTestCURL:                TestCURL(); break;
		case cmTestEGAIS:               TestEGAIS(0); break;
		case cmTestSearchEmail:         PPObjPerson::TestSearchEmail(); break;
		case cmTestLogWindow:           TestLogWindow(); break; // @v9.7.0
		case cmTestTddo2:               TestTddo2(); break; // @v9.8.3
		case cmTestVetis:               TestVetis(); break; // @v9.8.9
		case cmTestMqc:                 TestMqc();   break; // @v10.5.7
		case cmConvertLatexToHtml:      PPTex2Html(); break;
		case cmConvertVersionToHtml:    PPVer2Html(); break;
		// @underconstruction case cmGenerateGoodsBills:   GenerateGoodsBills(); break;
		// @v3.7.2 case cmConvert229:           Convert229();         break;
		// @v3.7.2 case cmConvert253:           Convert253();         break;
		// @v3.7.2 case cmConvert270:           Convert270();         break;
		// @v3.7.2 case cmConvert300:           Convert300();         break;
		// @v4.4.2 case cmConvert301:           Convert301();         break;
		// @v4.4.2 case cmConvert329:           Convert329();         break;
		// @v4.4.5 case cmConvert3512:          Convert3512();        break;
		// @v9.0.4 case cmConvert372:           Convert372();                    break;
		// @v9.0.4 case cmConvert400:           Convert400();                    break;
		// @v9.0.4 case cmConvert31102:         Convert31102();                  break;
		case cmConvert4402:          Convert4402();                   break;
		case cmConvert4405:          Convert4405();                   break;
		case cmConvert4707:          Convert4707();                   break;
		case cmConvert4805:          Convert4805();                   break;
		case cmConvertQuot720:       ConvertQuot720();                break;
		case cmConvertBillUUID7601:  CALLPTRMEMB(p_bobj, ConvertUuid7601()); break;
		case cmConvertWorkbook813:   ConvertWorkbook813();            break;
		case cmConvert10703:         Convert10703();                  break;
		case cmConvert1:
#ifdef __CONVERT__
#endif
			break;
		default: return 0;
	}
	LastCmd = 0;
	return 1;
}

static int SLAPI GetStartUpPath(PPID pi, SString & rArgBuf)
{
	int    ok = 0;
	if(pi == PPPATH_IN)
		ok = PPSession::GetStartUpOption(PPSession::cmdlIn, rArgBuf);
	else if(pi == PPPATH_OUT)
		ok = PPSession::GetStartUpOption(PPSession::cmdlOut, rArgBuf);
	else
		rArgBuf.Z();
	return ok;
}
//
// Prototype
//
int SLAPI ProcessExportJob(const char *);
int SLAPI ProcessImportJob(const char *);

int SLAPI PPLoginDialog(int * protect, SString & rUserName, char * pPassw,
	LDATE * pDate, PPDbEntrySet2 * pDbes, int forcePrompt, int sysAccLogin, int retryLogin)
{
	class LoginDialog : public TDialog {
	public:
		LoginDialog(int rezID, int sysAccLogin, PPDbEntrySet2 * pDbes) : TDialog(rezID), SystemAccountLogin(sysAccLogin), P_Dbes(pDbes)
		{
			HelpCtx = HELP_DLG_LOGIN;
			SString temp_buf;
			uint   mj, mn, rv;
			PPVersionInfo vi = DS.GetVersionInfo();
			vi.GetVersion(&mj, &mn, &rv);
			PPLoadString("version", temp_buf);
			temp_buf.Space().CatDotTriplet(mj, mn, rv).Space().CatChar('(').Cat(vi.GetAssemblyN()).CatChar(')');
			setStaticText(CTL_LOGIN_ST_INFO, temp_buf);
		}
		int    CheckSystemAccount(int getDBEntryFromDlg)
		{
			int    ok = 0;
			if(P_Dbes) {
				const PPID dbid = getDBEntryFromDlg ? getCtrlLong(CTLSEL_LOGIN_DB) : P_Dbes->GetSelection();
				if(dbid) {
					PPID   dbentry_id = 0;
					DbLoginBlock dlb;
					PPSecur usr_rec;
					P_Dbes->SetSelection(dbid);
					if(P_Dbes->GetByID(dbid, &dlb) && DS.CheckSystemAccount(&dlb, &usr_rec) > 0) {
						char   pw[128], user_name[128];
						memzero(pw, sizeof(pw));
						memzero(user_name, sizeof(user_name));
						STRNSCPY(user_name, usr_rec.Name);
						Reference::GetPassword(&usr_rec, pw, sizeof(pw));
						setCtrlData(CTL_LOGIN_NAME,     user_name);
						setCtrlData(CTL_LOGIN_PASSWORD, pw);
						ok = 1;
					}
				}
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmUtil)) {
				endModal(TVCMD);
				return; // ѕосле endModal не следует обращатьс€ к this
			}
			else if(event.isCbSelected(CTLSEL_LOGIN_DB) && P_Dbes && SystemAccountLogin) {
				if(CheckSystemAccount(1)) {
					endModal(cmOK);
					return; // ѕосле endModal не следует обращатьс€ к this
				}
			}
			else
				return;
			clearEvent(event);
		}
		int    SystemAccountLogin;
		PPDbEntrySet2 * P_Dbes;
	};
	int    ok = -1, r;
	int    valid_data = 0;
	int    usedbsel  = 0;
	int    nologinprompt = 0;
	PPID   dbid = 0;
	if(pDbes && pDbes->GetCount())
		if(pDbes->GetCount() > 1) {
			dbid = pDbes->GetSelection();
			usedbsel = dbid ? 0 : 1;
		}
		else
			dbid = pDbes->SetSelection(dbid = 1L);
	uint   rezID = protect ? DLG_PROTECT : (usedbsel ? DLG_LOGIND : DLG_LOGIN);
	LDATE  curdate = getcurdate_();
	if(!forcePrompt && !usedbsel && rUserName.NotEmpty()) {
		PPIniFile ini_file;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_NOLOGINPROMPT, &nologinprompt);
	}
	if(nologinprompt) {
		*pDate = curdate;
		ok = 2;
	}
	else {
		int    skip_edit_dlg = 0;
		SString title_buf;
		SString temp_buf;
		LoginDialog * dlg = new LoginDialog(rezID, sysAccLogin, pDbes);
		if(CheckDialogPtrErr(&dlg)) {
			// @v9.4.3 {
			if(!protect) {
				PPLoadText(PPTXT_GREETING, title_buf);
				PPVersionInfo vi = DS.GetVersionInfo();
				vi.GetProductName(temp_buf);
				title_buf.Space().Cat(temp_buf);
				dlg->setTitle(title_buf);
			}
			// } @v9.4.3
			skip_edit_dlg = BIN(!retryLogin && !usedbsel && sysAccLogin);
			decodedate(&r, &DefaultMonth, &DefaultYear, &curdate);
			r = 0;
			dlg->setCtrlString(CTL_LOGIN_NAME, rUserName);
			dlg->setCtrlData(CTL_LOGIN_PASSWORD, pPassw);
			dlg->setCtrlData(CTL_LOGIN_DATE,     pDate);
			if(usedbsel)
				SetupDBEntryComboBox(dlg, CTLSEL_LOGIN_DB, pDbes);
			if(rUserName.NotEmpty() && !usedbsel)
				dlg->selectCtrl(CTL_LOGIN_PASSWORD);
			skip_edit_dlg = BIN(skip_edit_dlg && dlg->CheckSystemAccount(0));
			while(!valid_data && (skip_edit_dlg || (r = ExecView(dlg)) == cmOK || r == cmUtil)) {
				if(usedbsel)
					pDbes->SetSelection(dlg->getCtrlLong(CTLSEL_LOGIN_DB));
				if(r == cmUtil) {
					DBMaintenance(pDbes, 0);
					// @v9.4.6 break;
				}
				else {
					dlg->getCtrlString(CTL_LOGIN_NAME,   rUserName);
					dlg->getCtrlData(CTL_LOGIN_PASSWORD, pPassw);
					dlg->getCtrlData(CTL_LOGIN_DATE,     pDate);
					if(!checkdate(pDate))
						PPError(PPERR_SLIB, 0);
					else
						ok = valid_data = 1;
				}
			}
			delete dlg;
		}
	}
	return ok;
}

static int SLAPI StoreLastBinPath()
{
	// @v10.7.10 static const char * WrParam_BinPath = "BinPath"; // @global @threadsafe
	SString path, param;
	SString temp_buf;
	PPGetPath(PPPATH_BIN, path);
	path.RmvLastSlash();
	WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
	WinRegValue reg_val;
	long   last_path_idx = 0;
	for(uint i = 0; reg_key.EnumValues(&i, &param, &reg_val) > 0;) {
		const size_t prfx_len = sstrlen(_PPConst.WrParam_BinPath);
		if(strnicmp(param, _PPConst.WrParam_BinPath, prfx_len) == 0) {
			last_path_idx = MAX(atoi(param+prfx_len), last_path_idx);
			if(reg_val.GetString(temp_buf) && path.CmpNC(temp_buf) == 0)
				return -1;
		}
	}
	(param = _PPConst.WrParam_BinPath).Cat(last_path_idx+1);
	return BIN(reg_key.PutString(param, path));
}

static int SLAPI GetCashNodeIDByName(PPID * pID, const char * pName)
{
	int    ok = 1;
	PPID   id = 0;
	PPObjCashNode cn_obj;
	if(cn_obj.SearchByName(pName, &id) <= 0) {
		id = 0;
		ok = PPSetError(PPERR_UNKNOWNCASHNAME, pName);
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

static int SLAPI GetPrcIDByName(PPID * pID, const char * pName)
{
	int    ok = 1;
	PPID   id = 0;
	if(pName) {
		PPObjProcessor prc_obj;
		if(prc_obj.SearchByName(PPPRCK_PROCESSOR, pName, &id, 0) <= 0) {
			SString oem_name(pName);
			oem_name.Transf(CTRANSF_OUTER_TO_INNER);
			if(prc_obj.SearchByName(PPPRCK_PROCESSOR, oem_name, &id, 0) <= 0) {
				id = atol(pName);
				if(prc_obj.Search(id, 0) <= 0) {
					id = 0;
					ok = PPSetError(PPERR_UNKNOWNPRCNAME, pName);
				}
			}
		}
	}
	else
		ok = PPSetError(PPERR_UNKNOWNPRCNAME);
	ASSIGN_PTR(pID, id);
	return ok;
}

static int SLAPI GetGoodsInfoIDByName(PPID * pID, const char * pName)
{
	int    ok = 1;
	PPID   id = 0;
	PPGoodsInfo rec;
	PPObjGoodsInfo gi_obj;
	for(id = 0; !id && gi_obj.EnumItems(&id, &rec) > 0;)
		if(stricmp866(rec.Name, pName) == 0)
			id = rec.ObjID;
	if(!id)
		ok = PPSetError(PPERR_UNKNOWNGOODSINFONAME, pName);
	ASSIGN_PTR(pID, id);
	return ok;
}

SLAPI PPVerHistory::Info::Info()
{
}

SLAPI PPVerHistory::PPVerHistory()
{
}

int SLAPI PPVerHistory::Read(const char * pDataPath, Info * pInfo)
{
	int    ok = -1;
	if(pDataPath) {
		size_t len = 0;
		size_t hdr_size = sizeof(PPVerHistory::Header);
		SString fname, path;
		PPGetFileName(PPFILNAM_VERHIST, fname);
		(path = pDataPath).SetLastSlash().Cat(fname);
		if(fileExists(path) != 0) {
			SFile  f;
			uint32 crc  = 0;
			Header hdr;
			MEMSZERO(hdr);
			THROW_SL(f.Open(path, SFile::mRead|SFile::mBinary) > 0);
			THROW_PP_S(f.Read(&hdr, hdr_size, &len) > 0 && len == hdr_size, PPERR_INVHDRSIZE, path); // @v10.6.9 THROW_PP-->THROW_PP_S(..., path)
			THROW_PP_S(hdr.Signature == _PPConst.Signature_VerHist, PPERR_VERHISTINVSIGN, path);
			THROW_SL(f.CalcCRC(hdr_size, &crc));
			SLibError = SLERR_INVALIDCRC;
			THROW_SL(crc == hdr.CRC);
			if(pInfo) {
				pInfo->MinVer = hdr.MinVer;
				pInfo->CurVer = hdr.CurVer;
				pInfo->DbUUID = hdr.DbUUID;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPVerHistory::Write(const char * pDataPath, const Info * pInfo)
{
	int    ok = -1;
	uint32 crc = 0;
	SString path, temp_buf;
	SVerT hdr_minv;
	(path = pDataPath).SetLastSlash().Cat(PPGetFileName(PPFILNAM_VERHIST, temp_buf));
	SFile  f;
	Header hdr;
	Record rec;
	THROW_INVARG(pInfo);
	PPSetAddedMsgString(path);
	if(fileExists(path)) {
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mReadWrite|SFile::mBinary) > 0);
	}
	else {
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mWrite|SFile::mBinary) > 0);
		f.Close();
		THROW_SL(f.Open(path, SFile::mDenyWrite|SFile::mReadWriteTrunc|SFile::mBinary) > 0);
	}
	MEMSZERO(hdr);
	MEMSZERO(rec);
	f.Seek(0);
	hdr.Signature = _PPConst.Signature_VerHist;
	hdr.CRC       = crc;
	hdr.CurVer    = pInfo->CurVer;
	hdr.MinVer    = pInfo->MinVer;
	hdr.DbUUID    = pInfo->DbUUID;
	THROW_SL(f.Write(&hdr, sizeof(hdr)) > 0);
	{
		MACAddr addr;
		rec.Dtm = getcurdatetime_();
		rec.Ver = pInfo->CurVer;
		DS.GetMachineID(&addr, 0);
		addr.ToStr(temp_buf.Z()).CopyTo(rec.Machine, sizeof(rec.Machine));
		f.Seek(0, SEEK_END);
		THROW_SL(f.Write(&rec, sizeof(rec)) > 0);
		THROW_SL(f.CalcCRC(sizeof(hdr), &crc) > 0);
		f.Seek(0);
		hdr.CRC = crc;
		THROW_SL(f.Write(&hdr, sizeof(hdr)) > 0);
	}
	CATCHZOK
	return ok;
}

/*static*/int SLAPI PPVerHistory::Log(const char * pDataPath, const char * pLogPath)
{
	int    ok = 0;
	SString path, fname;
	PPGetFileName(PPFILNAM_VERHIST, fname);
	path.CopyFrom(pDataPath).SetLastSlash().Cat(fname);
	if(fileExists(path)) {
		int    j = 0, m = 0, r = 0;
		uint32 crc = 0;
		const char * p_rectitle = "date time;mac address;version\n";
		const char * p_hdrtitle = "current version;minimum version\n";
		SString buf;
		Header hdr;
		Record rec;
		SFile  f, logf;
		MEMSZERO(hdr);
		MEMSZERO(rec);
		THROW_SL(f.Open(path, SFile::mRead|SFile::mBinary) > 0);
		THROW_SL(f.Read(&hdr, sizeof(hdr)) > 0);
		THROW_SL(f.CalcCRC(sizeof(hdr), &crc));
		SLibError = SLERR_INVALIDCRC;
		THROW_SL(crc == hdr.CRC);
		PPGetFileName(PPFILNAM_VERHISTLONG, (fname = 0));
		(path = 0).CopyFrom(pLogPath).SetLastSlash().Cat(fname);
		THROW_SL(logf.Open(path, SFile::mWrite) > 0);
		hdr.CurVer.Get(&j, &m, &r);
		buf.CatDotTriplet(j, m, r).Semicol();
		hdr.MinVer.Get(&j, &m, &r);
		buf.CatDotTriplet(j, m, r).CR();
		THROW_SL(logf.WriteLine(p_hdrtitle) > 0);
		THROW_SL(logf.WriteLine(buf));
		THROW_SL(logf.WriteLine(p_rectitle) > 0);
		while(f.Read(&rec, sizeof(rec)) > 0) {
			rec.Ver.Get(&(j = 0), &(m = 0), &(r = 0));
			buf.Z().Cat(rec.Dtm).Semicol().Cat(rec.Machine).Semicol().CatDotTriplet(j, m, r).CR();
			THROW_SL(logf.WriteLine(buf) > 0);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*static*/int SLAPI PPVerHistory::Log()
{
	int    ok = -1;
	SString arg_buf;
	if(PPSession::GetStartUpOption(PPSession::cmdlVerHist, arg_buf) && arg_buf.NotEmptyS()) {
		SString data_path, log_path;
		PPIniFile  ini_file;
		PPID   dbentry_id = 0;
		PPDbEntrySet2 dbes;
		DbLoginBlock dlb;
		PPVerHistory verh;
		dbes.ReadFromProfile(&ini_file);
		THROW_SL(dbentry_id = dbes.GetBySymb(arg_buf, &dlb));
		dlb.GetAttr(DbLoginBlock::attrDbPath, data_path);
		PPGetPath(PPPATH_LOG, log_path);
		THROW(verh.Log(data_path, log_path) > 0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPApp::SetDatetimeByServer(PPIniFile * pIniFile, int force)
{
	int    ok = -1;
	PPIniFile * p_ini_file = 0;
	//
	// ≈сли определен сервер синхронизации времени, то пытаемс€ установить
	// системное врем€ на рабочей станции в соответствии с сервером.
	// ¬рем€ устанавливаетс€ только в том случае, если текущее врем€ на
	// станции отличаетс€ от сервера более чем на 1 секунду
	//
	if(!(LocalState & lsLockSettingTimeByServer)) {
		SString temp_buf;
		if(pIniFile == 0) {
			THROW_MEM(p_ini_file = new PPIniFile);
			pIniFile = p_ini_file;
		}
		if(pIniFile->Get(PPINISECT_CONFIG, PPINIPARAM_TIMESERVER, temp_buf) && temp_buf.Len()) {
			SString fmt_buf, msg_buf, server_name, enforce_param;
			LDATETIME dtm;
			temp_buf.Divide(',', server_name, enforce_param);
			if(!force || enforce_param.ToLong() != 0) {
				if(force)
					PPLogMessage(PPFILNAM_INFO_LOG, PPSTR_TEXT, PPTXT_LOG_TIMECHANGE_FORCE, LOGMSGF_TIME|LOGMSGF_COMP);
				int r = SGetTimeFromRemoteServer(server_name, &dtm);
				if(r == 0) {
					PPSetErrorSLib();
					PPFormatT(PPTXT_LOG_TIMESERVERERR, &msg_buf, server_name.cptr(), PPErrCode);
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
					ok = 0;
				}
				else if(r > 0) {
					const  LDATETIME curdtm = getcurdatetime_();
					const  long   diff = diffdatetimesec(dtm, curdtm);
					if(labs(diff) >= 2) {
						LocalState |= lsLockSettingTimeByServer;
						if(setcurdatetime(dtm)) {
							PPFormatT(PPTXT_LOG_TIMESERVERSET, &msg_buf, server_name.cptr(), diff);
							PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
							ok = 1;
						}
						else {
							PPSetErrorSLib();
							PPFormatT(PPTXT_LOG_TIMESERVERSETERR, &msg_buf, server_name.cptr(), PPErrCode);
							PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_COMP);
							ok = 0;
						}
						LocalState &= ~lsLockSettingTimeByServer;
					}
					else
						ok = -2;
				}
			}
			else if(force) {
				PPLogMessage(PPFILNAM_INFO_LOG, PPSTR_TEXT, PPTXT_LOG_TIMECHANGE_NOFORCE, LOGMSGF_TIME|LOGMSGF_COMP);
				ok = -3;
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_LASTERR);
		ok = 0;
	ENDCATCH
	delete p_ini_file;
	return ok;
}

#if 0 // @v10.1.5 {
int PPBackupSystemFiles()
{
	int    ok = -1;
	SString bin_dir;
	SString backup_dir;
	SString fname;
	SString lock_path;
	PPGetPath(PPPATH_BIN, bin_dir);
	(backup_dir = bin_dir).SetLastSlash().Cat("backup").SetLastSlash();
	PPGetFileName(PPFILNAM_BULOCK, fname);
	(lock_path = backup_dir).SetLastSlash().Cat(fname);
	createDir(backup_dir);
	if(!fileExists(lock_path)) {
		LDATE last_copy_dt, cur_dt;
		SString buf, last_copy_path;
		SFile f(lock_path, SFile::mWrite);

		f.Close();
		PPGetFileName(PPFILNAM_LASTBACKUP, fname);
		(last_copy_path = backup_dir).SetLastSlash().Cat(fname);
		f.Open(last_copy_path, SFile::mRead);
		f.ReadLine(buf);
		f.Close();
		strtodate(buf, DATF_DMY, &last_copy_dt);
		getcurdate(&cur_dt);
		if(last_copy_dt < cur_dt) {
			long start = 1;
			SString path;
			SString dest_dir;
			SString dest_path;
			dest_dir = MakeTempFileName(backup_dir, 0, 0, &start, dest_dir);
			if(start > 6) {
				StrAssocArray temp_dir_list;
				SDirEntry de;
				SDirec sdir((path = backup_dir).CatChar('*').Dot().CatChar('*'), 1);
				for(long i = 1; sdir.Next(&de) > 0; i++)
					if(de.FileName[0] != '.' && de.FileName[1] != 0)
						temp_dir_list.Add(i, 0, de.FileName);
				if(temp_dir_list.getCount() > 0) {
					(path = backup_dir).SetLastSlash().Cat(temp_dir_list.Get(0).Txt);
					RemoveDir(path);
					temp_dir_list.AtFree(0);
				}
				for(uint j = 0; j < temp_dir_list.getCount(); j++) {
					(buf = backup_dir).SetLastSlash().Cat(temp_dir_list.Get(j).Txt);
					SFile::Rename(buf, path);
					path = buf;
				}
				dest_dir = MakeTempFileName(backup_dir, 0, 0, &(start = 5), dest_dir);
			}

			dest_dir.TrimRightChr('.');
			createDir(dest_dir);

			fname = "PP.INI";
			(path = bin_dir).SetLastSlash().Cat(fname);
			(dest_path = dest_dir).SetLastSlash().Cat(fname);
			SCopyFile(path, dest_path, 0, FILE_SHARE_READ, 0);
			{
				static const uint _bin_file_id_list[] = {
					PPFILNAM_PPCMD_BIN,
					PPFILNAM_PPDESK_BIN,
					PPFILNAM_JOBPOOL,
					PPFILNAM_REPORT_INI,
					PPFILNAM_IMPEXP_INI,
					PPFILNAM_BARLABEL_LBL, // @v10.1.5
					PPFILNAM_REGDATA,      // @v10.1.5
					PPFILNAM_STDSLIP_FMT   // @v10.1.5
				};
				for(uint i = 0; i < SIZEOFARRAY(_bin_file_id_list); i++) {
					PPGetFileName(_bin_file_id_list[i], fname);
					(path = bin_dir).SetLastSlash().Cat(fname);
					if(fileExists(path)) {
						(dest_path = dest_dir).SetLastSlash().Cat(fname);
						SCopyFile(path, dest_path, 0, FILE_SHARE_READ, 0);
					}
				}
			}
			PPGetFileName(PPFILNAM_LOCAL_DL6, fname);
			PPGetPath(PPPATH_DD, path);
			path.SetLastSlash().Cat(fname);
			(dest_path = dest_dir).SetLastSlash().Cat(fname);
			SCopyFile(path, dest_path, 0, FILE_SHARE_READ, 0);
			{
				f.Open(last_copy_path, SFile::mWrite);
				buf.Z().Cat(cur_dt, DATF_DMY);
				f.WriteLine(buf);
				ok = 1;
			}
		}
		SFile::Remove(lock_path);
	}
	return ok;
}
#endif // } 0 @v10.1.5

int PPBackupSystemFiles_Arc()
{
	int    ok = -1;
	SString bin_dir;
	SString backup_dir;
	SString fname;
	SString lock_path;
	PPGetPath(PPPATH_BIN, bin_dir);
	(backup_dir = bin_dir).SetLastSlash().Cat("backup").SetLastSlash();
	PPGetFileName(PPFILNAM_BULOCK, fname);
	(lock_path = backup_dir).SetLastSlash().Cat(fname);
	createDir(backup_dir);
	const int  is_locked = BIN(fileExists(lock_path) && SFile::IsOpenedForWriting(lock_path)); // @v10.7.5 дополнительна€ проверка на то, чтоб файл был открыт дл€ записи
	if(!is_locked) {
		const LDATETIME cur_dtm = getcurdatetime_();
		LDATE last_copy_dt;
		SString buf, last_copy_path;
		SFile f_lck(lock_path, SFile::mWrite);
		// @v10.7.6 @fix f_lck.Close();
		PPGetFileName(PPFILNAM_LASTBACKUP, fname);
		(last_copy_path = backup_dir).SetLastSlash().Cat(fname);
		{
			SFile f(last_copy_path, SFile::mRead);
			f.ReadLine(buf);
			f.Close();
			strtodate(buf, DATF_DMY, &last_copy_dt);
			if(last_copy_dt < cur_dtm.d) {
				SArchive arc;
				SString arc_file_name;
				// NB: archive file name will be created without extention (this is a protection against crypto-viruses)
				(arc_file_name = backup_dir).SetLastSlash().Cat("bu").CatChar('-').Cat(cur_dtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(cur_dtm.t, TIMF_HMS|TIMF_NODIV);
				if(arc.Open(SArchive::tZip, arc_file_name, SFile::mReadWrite, 0)) {
					long start = 1;
					SString path;
					SString dest_dir;
					SString dest_path;
					fname = "PP.INI";
					(path = bin_dir).SetLastSlash().Cat(fname);
					(dest_path = dest_dir).SetLastSlash().Cat(fname);
					arc.AddEntry(path, fname, 0);
					{
						static const uint _bin_file_id_list[] = {
							PPFILNAM_PPCMD_BIN,
							PPFILNAM_PPDESK_BIN,
							PPFILNAM_JOBPOOL,
							PPFILNAM_REPORT_INI,
							PPFILNAM_IMPEXP_INI,
							PPFILNAM_BARLABEL_LBL, // @v10.1.5
							PPFILNAM_REGDATA,      // @v10.1.5
							PPFILNAM_STDSLIP_FMT   // @v10.1.5
						};
						for(uint i = 0; i < SIZEOFARRAY(_bin_file_id_list); i++) {
							PPGetFileName(_bin_file_id_list[i], fname);
							(path = bin_dir).SetLastSlash().Cat(fname);
							if(fileExists(path)) {
								(dest_path = dest_dir).SetLastSlash().Cat(fname);
								arc.AddEntry(path, fname, 0);
							}
						}
					}
					PPGetFileName(PPFILNAM_LOCAL_DL6, fname);
					PPGetPath(PPPATH_DD, path);
					path.SetLastSlash().Cat(fname);
					(dest_path = dest_dir).SetLastSlash().Cat(fname);
					arc.AddEntry(path, fname, 0);
	//@erik v10.6.13 {
					PPGetPath(PPPATH_WORKSPACE, path);
					arc.AddEntries(path, SArchive::aefRecursive);
	// } @erik
					// @v10.7.5 {
					{
						//
						// Ћокальные отчеты
						//
						PPIniFile ifile;
						ifile.Get(PPINISECT_PATH, PPINIPARAM_WINLOCALRPT, path);
						if(path.Empty()) {
							PPGetPath(PPPATH_BIN, path);
							path.SetLastSlash().Cat("RPT").SetLastSlash().Cat("LOCAL");
						}
						arc.AddEntries(path, SArchive::aefRecursive);
					}
					// @v10.7.5 {
					arc.Close();
					{
						const uint max_arc = 5;
						//
						// ≈сли количество архивов больше max_arc, то лишние самые старые удал€ем
						//
						SFileEntryPool fep;
						fep.Scan(backup_dir, path.Z().Cat("bu").CatChar('-').CatCharN('*', 14), 0);
						fep.Sort(SFileEntryPool::scByWrTime);
						if(fep.GetCount() > max_arc) {
							uint to_remove_count = (fep.GetCount() - max_arc);
							SFileEntryPool::Entry fep_entry;
							for(uint i = 0; i < to_remove_count; i++) {
								if(fep.Get(i, &fep_entry, &path)) {
									SFile::Remove(path);
								}
							}
						}
					}
					{
						f.Open(last_copy_path, SFile::mWrite);
						buf.Z().Cat(cur_dtm.d, DATF_DMY|DATF_CENTURY);
						f.WriteLine(buf);
						ok = 1;
					}
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
				}
			}
		}
		f_lck.Close(); // @v10.7.6
		SFile::Remove(lock_path);
	}
	return ok;
}

#if 0 // {

int SLAPI PreprocessCmdLine()
{
	int    ok = 1;
	char   buf[512];
	SString msg_buf, temp_buf;
	if(GetStartUpOption(cmdlVerHist, 0)) {
		int    r = 0;
		if((r = PPVerHistory::Log()) <= 0) {
			if(r == -1)
				msg_buf.Cat("Usage").CatDiv(':', 2).Cat("ppw.exe /verhist:dbsymb");
			else
				PPGetLastErrorMessage(0, msg_buf);
			puts(msg_buf);
		}
		ok = -1;
	}
	// else if(GetStartUpOption(cmdlSartrTest, 0)) { SartTest(); }
	else if(GetStartUpOption(cmdlPPOS, buf)) {
		if(buf[0] == 0) {
			puts(msg_buf.Cat("Usage").CatDiv(':', 2).Cat("ppw.exe /ppos:file_name").CR());
		}
		else if(fileExists(buf)) {
			PPObjectTransmit::PacketStat stat;
			if(PPObjectTransmit::ReadFileStat(buf, stat)) {
				int    major, minor, rel;
				stat.Hdr.SwVer.Get(&major, &minor, &rel);
				msg_buf.Z();
				msg_buf.Cat("File").CatDiv(':', 2).Cat(buf).CR();
				//
				msg_buf.Cat("Packet type").CatDiv(':', 2);
				if(stat.Hdr.PacketType == PPOT_OBJ)
					msg_buf.Cat("PPOT_OBJ");
				else if(stat.Hdr.PacketType == PPOT_ACK)
					msg_buf.Cat("PPOT_ACK");
				else if(stat.Hdr.PacketType == PPOT_SYNCCMP)
					msg_buf.Cat("PPOT_SYNCCMP");
				else
					msg_buf.Cat("Unknown");
				msg_buf.CR();
				//
				msg_buf.Cat("Source").CatDiv(':', 2).Cat(stat.Hdr.DBID);
				if(stat.Hdr.SwVer.IsGt(6, 2, 1)) {
					stat.Hdr.SrcDivUuid.ToStr(S_GUID::fmtIDL, temp_buf);
					msg_buf.Space().Cat(temp_buf).CR();
					msg_buf.Cat("UserID").CatDiv(':', 2).Cat(stat.Hdr.UserID).CR();
				}
				else
					msg_buf.CR();
				//
				msg_buf.Cat("Destination").CatDiv(':', 2).Cat(stat.Hdr.DestDBID);
				if(stat.Hdr.SwVer.IsGt(6, 2, 1)) {
					stat.Hdr.DestDivUuid.ToStr(S_GUID::fmtIDL, temp_buf);
					msg_buf.Space().Cat(temp_buf);
				}
				msg_buf.CR();
				//
				msg_buf.Cat("Used").CatDiv(':', 2).Cat((stat.Hdr.Flags & PPOTF_ACK) ? "Yes" : "No").CR();
				msg_buf.Cat("Packed").CatDiv(':', 2).Cat((stat.Hdr.Flags & PPOTF_ARC) ? "Yes" : "No").CR();
				//
				msg_buf.Cat("Source Version").CatDiv(':', 2).CatDotTriplet(major, minor, rel).CR();
				stat.Hdr.MinDestVer.Get(&major, &minor, &rel);
				msg_buf.Cat("Min Destination Version").CatDiv(':', 2).CatDotTriplet(major, minor, rel).CR();

				puts(msg_buf);
			}
		}
		else {
			PPGetMessage(mfError, PPERR_SLIB, 0, 0, msg_buf);
			puts(msg_buf.CR());
		}
		ok = -1;
	}
	return 1;
}

#endif // } 0

int SLAPI ExecuteLuaScript(const char * pScriptText);
int SLAPI ExecuteLuaScriptFile(const char * pFileName);
int SLAPI ExportUhttForGitHub();
int SLAPI ParseWinRcForNativeText();

void PPApp::login(int processCmdLine)
{
	int    do_quit = 0;
	SString arg_buf;
	SString msg_buf, temp_buf;
	const  int no_login = PPSession::GetStartUpOption(PPSession::cmdlNoLogin, arg_buf);
	const  int is_batch = PPSession::GetStartUpOption(PPSession::cmdlBatch, arg_buf);
	//Test_KeywordListGenerator(); // @debug
	{
		// Test VERSEL.DLL
		/*
		SDynLibrary versel_mod("../../src/build/versel.dll");
		if(versel_mod.IsValid()) {
			// extern "C" __declspec(dllexport) int SelectVersion(char * pPath, long flags)
			// typedef SString * (*ISALESPUTBILLS_PROC)(PPSoapClientSession & rSess, const char * pUser, const char * pPassw, const TSCollection <iSalesBillPacket> * pItems, uint maxItems);
			int (*versel_proc)(HWND, char *, long) = (int (*)(HWND, char *, long))versel_mod.GetProcAddr("SelectVersion");
			if(versel_proc) {
				char   path_buf[512];
				memzero(path_buf, sizeof(path_buf));
				versel_proc(0, path_buf, 0);
			}
		}
		*/
	}
	if(processCmdLine && no_login) {
		DS.SetMenu(MENU_NOLOGIN);
		if(!InitMenuBar()) {
			PPError();
			TView::messageCommand(this, cmQuit);
		}
		else {
			PPIniFile ini_file;
			InitStatusBar();
			StoreLastBinPath();
			//
			// ≈сли определен сервер синхронизации времени, то пытаемс€ установить
			// системное врем€ на рабочей станции в соответствии с сервером.
			// ¬рем€ устанавливаетс€ только в том случае, если текущее врем€ на
			// станции отличаетс€ от сервера более чем на 1 секунду
			//
			SetDatetimeByServer(&ini_file, 0);
			const LDATE dt = getcurdate_();
			if(DS.CheckStateFlag(CFGST_DEMOMODE)) {
				if(CONFIRM(PPCFM_REGISTRATION) && PPLicRegister() > 0)
					DS.SetDemoMode(0);
				else
					PPMessage(mfInfo|mfOK, PPINF_DEMO);
			}
			DS.SetOperDate(dt);
			StatusWinChange(1);
		}
	}
	else if(PPSession::GetStartUpOption(PPSession::cmdlConvertRbcBnk, arg_buf)) {
		if(!ConvertRbcBnk(arg_buf))
			PPError();
		TView::messageCommand(this, cmQuit);
	}
	else if(PPSession::GetStartUpOption(PPSession::cmdlExportDialogs, arg_buf)) {
		PPGetFilePath(PPPATH_BIN, "ppdlg.dl6", temp_buf);
		ExportDialogs(temp_buf);
		TView::messageCommand(this, cmQuit);
	}
	else if(PPSession::GetStartUpOption(PPSession::cmdlAutoTranslate, arg_buf)) {
		SString src_file_path, auto_transl_lang;
		arg_buf.Divide(';', src_file_path, auto_transl_lang);
		src_file_path.Strip();
		auto_transl_lang.Strip();
		long   flags = StringStore2::cfTranslateOnly;
		{
			StringStore2 s;
			s.Compile(src_file_path, flags, auto_transl_lang);
		}
		TView::messageCommand(this, cmQuit);
	}
	else if(PPSession::GetStartUpOption(PPSession::cmdlConvertCipher, arg_buf)) {
		class CvtCipherDialog : public TDialog {
		public:
			CvtCipherDialog() : TDialog(DLG_CVTCIPHER)
			{
				FileBrowseCtrlGroup::Setup(this, CTLBRW_CVTCIPHER_INISRC,  CTL_CVTCIPHER_INISRC,  1, 0, PPTXT_FILPAT_DAT, FileBrowseCtrlGroup::fbcgfFile);
				FileBrowseCtrlGroup::Setup(this, CTLBRW_CVTCIPHER_INIDEST, CTL_CVTCIPHER_INIDEST, 2, 0, PPTXT_FILPAT_DAT, FileBrowseCtrlGroup::fbcgfFile);
			}
		};
		SString db_symb;
		SString pw;
		SString ini_src, ini_dest;
		CvtCipherDialog * dlg = new CvtCipherDialog();
		if(CheckDialogPtrErr(&dlg)) {
			PPIniFile ini_file;
			PPDbEntrySet2 dbes;
			dbes.ReadFromProfile(&ini_file);
			SetupDBEntryComboBox(dlg, CTLSEL_CVTCIPHER_DB, &dbes);
            dlg->setCtrlString(CTL_CVTCIPHER_PASSWORD, pw);
            dlg->setCtrlString(CTL_CVTCIPHER_INISRC, ini_src);
            dlg->setCtrlString(CTL_CVTCIPHER_INIDEST, ini_dest);
            for(int r = -1; r < 0 && ExecView(dlg) == cmOK;) {
				dlg->getCtrlString(CTL_CVTCIPHER_PASSWORD, pw);
				dlg->getCtrlString(CTL_CVTCIPHER_INISRC, ini_src);
				dlg->getCtrlString(CTL_CVTCIPHER_INIDEST, ini_dest);
				long    db_sel = dlg->getCtrlLong(CTLSEL_CVTCIPHER_DB);
				DbLoginBlock dblb;
				if(dbes.GetByID(db_sel, &dblb) > 0) {
					dblb.GetAttr(dblb.attrDbSymb, db_symb);
					if(ConvertCipher(db_symb, pw, ini_src, ini_dest)) {
						r = 1;
					}
					else
						PPError();
				}
            }
		}
		ZDELETE(dlg);
		TView::messageCommand(this, cmQuit);
	}
	// @v9.7.8 else if(GetStartUpOption(cmdlSartrTest, arg_buf)) { SartTest(); }
	else {
		int    valid = 0;
		int    force_prompt = 0;
		int    retry_login = 0;
		int    sys_acc_login = 0;
		int    prot = 0;
		int    recover = 0;
		char   pw[20];
		LDATE  oper_date = getcurdate_();
		SString lua_script; // @v10.0.1 @experimental
		SString usr_;
		PPID   dbentry_id = 0;
		PPDbEntrySet2 dbes; // @
		if(_argc > 1 && *_argv[1] != '/' && *_argv[1] != '-') {
			SPathStruc ps(_argv[1]);
			if(ps.Nam.NotEmpty() && ps.Ext.NotEmpty() && ps.Ext.IsEqiAscii("lua")) {
				lua_script = _argv[1];
			}
			else {
				usr_ = _argv[1];
				usr_.Transf(CTRANSF_OUTER_TO_INNER);
			}
		}
		PTR32(pw)[0] = 0;
		{ // @v10.5.7 «она видимости ini_file. ≈е необходимо изолировать - PPLoginDialog может затребовать эксклюзивный доступ к pp.ini дл€ редактировани€.
			PPIniFile  ini_file;
			dbes.ReadFromProfile(&ini_file);
			if(PPSession::GetStartUpOption(PPSession::cmdlDB, arg_buf)) {
				dbentry_id = dbes.GetBySymb(arg_buf, 0);
				if(dbentry_id <= 0) {
					PPError();
					TView::messageCommand(this, cmQuit);
					return;
				}
			}
			else if(dbes.GetCount() == 1)
				dbentry_id = 1;
			dbes.SetSelection(dbentry_id);
			InitStatusBar();
			StoreLastBinPath();
			ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_SYSAUTH, &sys_acc_login);
			//
			// ≈сли определен сервер синхронизации времени, то пытаемс€ установить
			// системное врем€ на рабочей станции в соответствии с сервером.
			// ¬рем€ устанавливаетс€ только в том случае, если текущее врем€ на
			// станции отличаетс€ от сервера более чем на 1 секунду
			//
			SetDatetimeByServer(&ini_file, 0);
			PPSession::EnsureExtCfgDb(); // @v10.7.7
			// @v10.1.5 PPBackupSystemFiles();
			PPBackupSystemFiles_Arc(); // @v10.1.5
			// @v10.0.1 @experimental {
			/*if(lua_script.NotEmpty()) {
				ExecuteLuaScriptFile(lua_script);
			}*/
			// } @v10.0.1
		}
		while(!valid && PPLoginDialog(prot ? &prot : 0, usr_, pw, &oper_date, &dbes, force_prompt, sys_acc_login, retry_login) > 0) {
			SString db_symb;
			PPID   db_id = NZOR(dbes.GetSelection(), dbes.SetDefaultSelection());
			DbLoginBlock dlb;
			if(db_id && dbes.GetByID(db_id, &dlb)) {
				PPWait(1);
				dlb.GetAttr(DbLoginBlock::attrDbSymb, db_symb);
				if(DS.Login(db_symb, usr_, pw, 0) && InitMenuBar()) {
					if(LConfig.Flags & CFGFLG_SEC_DSBLNSYSDATE && oper_date != getcurdate_()) {
						DS.Logout();
						PPSetError(PPERR_OPERDATEDIFSYSDATE);
					}
					else
						valid = 1;
				}
				PPWait(0);
			}
			if(!valid) {
				PPError();
				retry_login  = 1;
				force_prompt = 1;
			}
			else if(DS.CheckStateFlag(CFGST_DEMOMODE)) {
				if(CONFIRM(PPCFM_REGISTRATION) && PPLicRegister() > 0)
					DS.SetDemoMode(0);
				else
					PPMessage(mfInfo|mfOK, PPINF_DEMO);
			}
		}
		if(valid && !prot && !recover) {
			DS.SetOperDate(oper_date);
			StatusWinChange(1);
			PPDesktop::RegWindowClass(TProgram::GetInst());
			PPDesktop::Open(LConfig.DesktopID, 1); // @todo должна вызыватьс€ после того, как будут созданы основные окна.
			if(!no_login)
				RemoveMenu(GetMenu(H_MainWnd), cmDBExit, MF_BYCOMMAND);
			if(processCmdLine) {
				if(GetStartUpPath(PPPATH_IN, arg_buf))
					DS.SetPath(PPPATH_IN, arg_buf, 0, 1);
				if(GetStartUpPath(PPPATH_OUT, arg_buf))
					DS.SetPath(PPPATH_OUT, arg_buf, 0, 1);
				if(PPSession::GetStartUpOption(PPSession::cmdlImport, arg_buf))
					ProcessImportJob(arg_buf);
				if(PPSession::GetStartUpOption(PPSession::cmdlExport, arg_buf))
					ProcessExportJob(arg_buf);
				if(PPSession::GetStartUpOption(PPSession::cmdlSyncGet, arg_buf))
					PPObjectTransmit::ReceivePackets(0);
				if(PPSession::GetStartUpOption(PPSession::cmdlSyncPut, arg_buf))
					PPObjectTransmit::TransmitBillsByDBDivList(0);
				if(PPSession::GetStartUpOption(PPSession::cmdlUhttGoodsToGitHubExport, arg_buf)) { // @v10.1.6
					ExportUhttForGitHub();
				}
				{
					PPID   _id  = 0;
					if(PPSession::GetStartUpOption(PPSession::cmdlSelfBuild, arg_buf)) {
						if(arg_buf.IsEqiAscii("parsewinrcfornativetext")) {
							ParseWinRcForNativeText();
						}
						else
							SelfBuild();
					}
					else if(PPSession::GetStartUpOption(PPSession::cmdlCash, arg_buf)) {
						arg_buf.Transf(CTRANSF_OUTER_TO_INNER);
						if(GetCashNodeIDByName(&_id, arg_buf))
							ViewCashBills(_id);
						else
							PPError();
					}
					else if(PPSession::GetStartUpOption(PPSession::cmdlPrc, arg_buf)) {
						if(GetPrcIDByName(&_id, arg_buf))
							ExecPrcPane(_id);
						else
							PPError();
					}
					else if(PPSession::GetStartUpOption(PPSession::cmdlTSess, arg_buf)) {
						if(GetPrcIDByName(&_id, arg_buf)) {
							TSessionFilt filt;
							filt.PrcID = _id;
							filt.Flags |= TSessionFilt::fCurrent;
							::ViewTSession(&filt);
						}
						else
							PPError();
					}
					else if(PPSession::GetStartUpOption(PPSession::cmdlGoodsInfo, arg_buf)) {
						if(GetGoodsInfoIDByName(&_id, arg_buf)) {
							InfoKioskPaneFilt ik_filt;
							ik_filt.InfoKioskID = _id;
							ViewGoodsInfo(&ik_filt);
						}
					}
					if(PPSession::GetStartUpOption(PPSession::cmdlBillCash, arg_buf)) {
						if(GetCashNodeIDByName(&_id, arg_buf))
							DS.SetDefBillCashID(_id);
						else
							PPError();
					}
					// @v5.6.7 @temp {
					/* @v9.0.3 if(GetStartUpOption(cmdlRecoverTransfer, arg_buf)) {
						RecoverAbsenceTrfr();
					} */
					// } @v5.6.7 @temp
				}
				TestLogin();
				if(is_batch)
					TView::messageCommand(this, cmQuit);
			}
		}
		else if(!no_login)
			TView::messageCommand(this, cmQuit);
	}
	PPSession::GetStartUpOption(-1, arg_buf); // @v9.0.2 ќчищаем внутренний буфер функции
}

int PPApp::InitDeskTop()
{
	return 1;
}

int PPApp::InitMenuBar()
{
	const  PPConfig & r_cfg = LConfig;
	const  int is_master = BIN(PPMaster);
	const  int from_rc = (r_cfg.MenuID && !is_master) ? 0 : 1;
	if(r_cfg.Menu || r_cfg.MenuID || is_master) {
		int not_found = 0;
		HMENU  m = PPLoadMenu(P_SlRez, from_rc ? r_cfg.Menu : r_cfg.MenuID, from_rc, &not_found);
		if(!m || not_found) {
			if(is_master) {
				DS.GetTLA().Lc.Menu = 1;
				if(m)
					::DestroyMenu(m);
				m = PPLoadMenu(P_SlRez, r_cfg.Menu, 1, &not_found);
				if(!m || not_found) {
					if(m) {
						::DestroyMenu(m);
						m = 0; // @v10.4.5
					}
					DS.SetMenu(MENU_MASTER);
					// @v10.4.5 (¬строенные в windows-ресурс меню упразднены) m = ::LoadMenu(TProgram::GetInst(), MAKEINTRESOURCE(r_cfg.Menu));
				}
			}
			if(!m)
				return PPSetError(PPERR_MENULOADFAULT);
		}
		DestroyMenu(GetMenu(H_MainWnd));
		SetMenu(H_MainWnd, m);
	}
	::PostMessage(H_MainWnd, WM_USER, 0, 0);
	return 1;
}

int PPApp::RegisterComServer()
{
	class RegComSvrDialog : public TDialog {
	public:
		struct Param {
			Param() : Action(aRegister), Flags(0)
			{
			}
			/*void Init()
			{
				Action = aRegister;
				Flags = 0;
				ExcelRegKeyDir.Z();
				DllPath.Z();
				DllPath_XpCompat.Z();
			}*/
			enum {
				aRegister   = 0x0001,
				aDeregister = 0x0002
			};
			enum {
				fDl200Install = 0x0001,
				fXpCompat     = 0x0002
			};
			long   Action;
			long   Flags;
			SString ExcelRegKeyDir;
			SString DllPath;
			SString DllPath_XpCompat;
		};

		DECL_DIALOG_DATA(Param);

		RegComSvrDialog() : TDialog(DLG_REGCOMSVR)
		{
			SString reg_path;
			for(uint i = 14; XLARegKeyDir.Len() == 0 && i > 8; i--) {
				(reg_path = "Software\\Microsoft\\Office\\").Cat(i).Dot().Cat("0").Cat("\\Excel");
				WinRegKey reg_key;
				if(reg_key.Open(HKEY_CURRENT_USER, reg_path, 1, 1) > 0)
					XLARegKeyDir = reg_path;
			}
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_REGCOMSVR_ACTION,  0, Param::aRegister);
			AddClusterAssoc(CTL_REGCOMSVR_ACTION,  1, Param::aDeregister);
			SetClusterData(CTL_REGCOMSVR_ACTION, Data.Action);
			AddClusterAssoc(CTL_REGCOMSVR_FLAGS, 0, Param::fDl200Install);
			AddClusterAssoc(CTL_REGCOMSVR_FLAGS, 1, Param::fXpCompat);
			SetClusterData(CTL_REGCOMSVR_DL200INST, Data.Flags);
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetClusterData(CTL_REGCOMSVR_ACTION, &Data.Action);
			GetClusterData(CTL_REGCOMSVR_FLAGS, &Data.Flags);
			Data.ExcelRegKeyDir = XLARegKeyDir;
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_REGCOMSVR_ACTION) || event.isClusterClk(CTL_REGCOMSVR_FLAGS)) {
				GetClusterData(CTL_REGCOMSVR_ACTION, &Data.Action);
				GetClusterData(CTL_REGCOMSVR_FLAGS, &Data.Flags);
				SetupCtrls();
				clearEvent(event);
			}
		}
		void   SetupCtrls()
		{
			const int disable = BIN(XLARegKeyDir.Len() == 0 || Data.Action == Param::aDeregister);
			DisableClusterItem(CTL_REGCOMSVR_FLAGS, 0, disable);
			DisableClusterItem(CTL_REGCOMSVR_FLAGS, 1, disable || Data.DllPath_XpCompat.Empty());
			if(Data.Flags & Param::fXpCompat)
				setCtrlString(CTL_REGCOMSVR_INFO, Data.DllPath_XpCompat);
			else
				setCtrlString(CTL_REGCOMSVR_INFO, Data.DllPath);
		}
		SString XLARegKeyDir;
	};
	int    ok = -1;
	int    is_xpcompat_exists = 0;
	SString proc_name;
	SString param_buf;
	RegComSvrDialog::Param param;
	RegComSvrDialog * p_dlg = new RegComSvrDialog;
	PPGetFilePath(PPPATH_BIN, "ppwmt.dll", param.DllPath);
	PPGetFilePath(PPPATH_BIN, "ppwmt-xp.dll", param.DllPath_XpCompat);
	THROW_SL(::fileExists(param.DllPath));
	if(!::fileExists(param.DllPath_XpCompat))
		param.DllPath_XpCompat.Z();
	else
		is_xpcompat_exists = 1;
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&param);
	if(ExecView(p_dlg) == cmOK)	{
		p_dlg->getDTS(&param);
		HRESULT hr;
		const char * p_dll_path = (param.Flags & param.fXpCompat && ::fileExists(param.DllPath_XpCompat)) ? param.DllPath_XpCompat : param.DllPath;
		SDynLibrary lib(p_dll_path);
		THROW_SL(lib.IsValid());
		typedef HRESULT (STDAPICALLTYPE *procDllRegisterServer)();
		proc_name = (param.Action == RegComSvrDialog::Param::aDeregister) ? "DllUnregisterServer" : "DllRegisterServer";
		procDllRegisterServer proc = reinterpret_cast<procDllRegisterServer>(lib.GetProcAddr(proc_name));
		THROW_SL(proc);
		hr = proc();
		THROW(hr == S_OK);
		PPMessage(mfInfo|mfOK, (param.Action == RegComSvrDialog::Param::aDeregister) ? PPINF_COMSRVUNREGISTERED : PPINF_COMSRVREGISTERED, p_dll_path);
		if(param.Flags & RegComSvrDialog::Param::fDl200Install) {
			const char * p_dl200_name = "dl200.xla";
			const char * p_open_name = "OPEN";
			SString   src_path, subkey, opt_param;
			LongArray delreg_list;
			WinRegKey reg_key;
			PPGetFilePath(PPPATH_DD, p_dl200_name, src_path);
			(subkey = param.ExcelRegKeyDir).SetLastSlash().Cat("Options").SetLastSlash();
			opt_param = "OPEN";
			if(reg_key.Open(HKEY_CURRENT_USER, subkey, 0) > 0) {
				SString subkey2;
				WinRegValue reg_val;
				for(uint idx = 0; reg_key.EnumValues(&idx, &subkey2.Z(), &reg_val) > 0;) {
					if(subkey2.HasPrefixNC(p_open_name)) {
						SString open_idx_str;
						long open_idx = (open_idx_str = subkey2).ShiftLeft(4).ToLong();
						if(reg_key.GetString(subkey2, param_buf) > 0 && src_path.CmpNC(param_buf) == 0)
							delreg_list.add(open_idx);
						(opt_param = p_open_name).Cat(open_idx + 1);
					}
				}
			}
			reg_key.Close();
			for(uint i = 0; i < delreg_list.getCount(); i++) {
				opt_param = p_open_name;
				if(delreg_list.at(i) != 0)
					opt_param.Cat(delreg_list.at(i));
				reg_key.DeleteValue(HKEY_CURRENT_USER, subkey, opt_param);
			}
			delreg_list.freeAll();
			if(reg_key.Open(HKEY_CURRENT_USER, subkey, 0) > 0)
				reg_key.PutString(opt_param, src_path);
			reg_key.Close();
			(subkey = param.ExcelRegKeyDir).SetLastSlash().Cat("Add-in Manager");
			if(reg_key.Open(HKEY_CURRENT_USER, subkey, 0) > 0) {
				WinRegValue reg_val;
				for(uint idx = 0; reg_key.EnumValues(&idx, &param_buf, &reg_val) > 0;) {
					if(src_path.CmpNC(param_buf) == 0)
						delreg_list.add(idx);
				}
			}
			reg_key.Close();
			for(uint i = 0; i < delreg_list.getCount(); i++)
				reg_key.DeleteValue(HKEY_CURRENT_USER, subkey, src_path);
			if(reg_key.Open(HKEY_CURRENT_USER, subkey, 0) > 0)
				reg_key.PutString(src_path, "");
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
