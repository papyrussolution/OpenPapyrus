// DECLSTRU.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2005, 2007, 2008, 2010, 2011, 2012, 2015, 2016, 2017, 2019, 2020, 2021, 2023
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppds.h>
#include <charry.h>
/*
	Формат представления декларации данных в ресурсах:

	Resource Type: PP_RCDECLSTRUC
	Resource ID:   PPDS_XXX

	Declaration name: "Some_Name\0"
	Items count:      uint
	{
		Item ID:          uint
		Item Name:        "Some_Item_Name\0"
		Item Flags:       uint
		Item Type Name:   "Type_Name\0"
	} Items Count Times
*/
/*static*/PPDeclStruc * PPDeclStruc::CreateInstance(long typeID, void * /*extraPtr*/, const PPDeclStruc * pOuter, PPLogger * pLogger)
{
	PPDeclStruc * p_decl = 0;
	switch(typeID) {
		case PPDS_CRRACCTENTRY:        p_decl = new PPDS_CrrAcctEntry;        break;
		case PPDS_CRRACCTURNTEMPL:     p_decl = new PPDS_CrrAccturnTempl;     break;
		case PPDS_CRROPRKIND:          p_decl = new PPDS_CrrOprKind;          break;
		case PPDS_CRRADDRESS:          p_decl = new PPDS_CrrAddress;          break;
		case PPDS_CRRBNKACCT:          p_decl = new PPDS_CrrBnkAcct;          break;
		case PPDS_ELINKADDR:           p_decl = new PPDS_ELinkAddr;           break;
		case PPDS_CRRPERSON:           p_decl = new PPDS_CrrPerson;           break;
		case PPDS_BARCODE:             p_decl = new PPDS_Barcode;             break;
		case PPDS_CRRGOODS:            p_decl = new PPDS_CrrGoods;            break;
		case PPDS_CRRQCERT:            p_decl = new PPDS_CrrQCert;            break;
		case PPDS_CRRBILLITEM:         p_decl = new PPDS_CrrBillItem;         break;
		case PPDS_CRRBILL:             p_decl = new PPDS_CrrBill;             break;
		case PPDS_CRRAMOUNTTYPE:       p_decl = new PPDS_CrrAmountType;       break;
		case PPDS_CRRSALCHARGE:        p_decl = new PPDS_CrrSalCharge;        break;
		case PPDS_CRRSALCHARGEGROUP:   p_decl = new PPDS_CrrSalChargeGroup;   break;
		case PPDS_CRRSTAFFCAL:         p_decl = new PPDS_CrrStaffCal;         break;
		case PPDS_CRRSTAFFCALENTRY:    p_decl = new PPDS_CrrStaffCalEntry;    break;
		case PPDS_CRRDBDIV:            p_decl = new PPDS_CrrDbDiv;            break;
		case PPDS_CRRBARCODESTRUC:     p_decl = new PPDS_CrrBarcodeStruc;     break;
		case PPDS_CRRGOODSTYPE:        p_decl = new PPDS_CrrGoodsType;        break;
		case PPDS_CRRFORMULA:          p_decl = new PPDS_CrrFormula;          break;
		case PPDS_CRRSCALE:            p_decl = new PPDS_CrrScale;            break;
		case PPDS_CRRREGISTERTYPE:     p_decl = new PPDS_CrrRegisterType;     break;
		case PPDS_CRRQUOTKIND:         p_decl = new PPDS_CrrQuotKind;         break;
		case PPDS_CRRPERSONKIND:       p_decl = new PPDS_CrrPersonKind;       break;
		case PPDS_CRRCURRENCY:         p_decl = new PPDS_CrrCurrency;         break;
		case PPDS_CRRCURRATETYPE:      p_decl = new PPDS_CrrCurRateType;      break;
		case PPDS_CRRASSETWROFFGRP:    p_decl = new PPDS_CrrAssetWrOffGrp;    break;
		case PPDS_CRRMAILACCOUNT:      p_decl = new PPDS_CrrMailAccount;      break;
		case PPDS_CRRPERSONRELTYPE:    p_decl = new PPDS_CrrPersonRelType;    break;
		case PPDS_CRROBJTAG:           p_decl = new PPDS_CrrObjTag;           break;
		case PPDS_CRRDRAFTWROFFENTRY:  p_decl = new PPDS_CrrDraftWrOffEntry;  break;
		case PPDS_CRRDRAFTWROFF:       p_decl = new PPDS_CrrDraftWrOff;       break;
		case PPDS_CRRACCOUNT:          p_decl = new PPDS_CrrAccount;          break;
		case PPDS_CRRLOCATION:         p_decl = new PPDS_CrrLocation;         break;
		case PPDS_CRRACCSHEET:         p_decl = new PPDS_CrrAccSheet;         break;
		case PPDS_CRRARTICLE:          p_decl = new PPDS_CrrArticle;          break;
		case PPDS_CRROPRKINDENTRY:     p_decl = new PPDS_CrrOprKindEntry;     break;
		case PPDS_CRRINVOPEXENTRY:     p_decl = new PPDS_CrrInvOpExEntry;     break;
		case PPDS_CRRRECKONOPEXENTRY:  p_decl = new PPDS_CrrReckonOpExEntry;  break;
		case PPDS_CRRDRAFTOPEXENTRY:   p_decl = new PPDS_CrrDraftOpExEntry;   break;
		case PPDS_CRRBILLPOOLOPEXENTRY:   p_decl = new PPDS_CrrBillPoolOpExEntry;   break;
		case PPDS_CRRBILLSTATUS:       p_decl = new PPDS_CrrBillStatus;       break;
		default: PPSetError(PPERR_UNDEFCHARRYSTRUCID); break;
	}
	if(p_decl) {
		p_decl->Id = typeID;
		p_decl->P_Outer = pOuter;
		p_decl->P_Logger = pLogger;
		if(!p_decl->LoadFromResource(typeID))
			ZDELETE(p_decl);
	}
	return p_decl;
}

//
int SendCharryObject(PPID strucID, const PPIDArray & rObjIdList)
{
	int    ok = 1;
	PPDeclStruc * p_decl = 0;
	SString path, fname;
	PPDBXchgConfig cfg;
	FILE * stream = 0;
	if(rObjIdList.getCount()) {
		PPWaitStart();
		PPWaitMsg(PPSTR_TEXT, PPTXT_SENDCHARRYOBJ, 0);
		PPObjectTransmit::ReadConfig(&cfg);
		long   counter = cfg.CharryOutCounter+1;
		PPGetPath(PPPATH_OUT, path);
		MakeTempFileName(path, 0, PPConst::FnExt_CHARRY, &counter, fname);
		THROW_PP(stream = fopen(fname, "w"), PPERR_EXPFOPENFAULT);
		for(uint i = 0; i < rObjIdList.getCount(); i++) {
			THROW(p_decl = PPDeclStruc::CreateInstance(strucID, 0, 0, 0));
			if(p_decl->InitData(PPDeclStruc::idoExtract, 0, rObjIdList.get(i)) > 0)
				THROW(p_decl->WriteDataToStream(stream, 0));
			ZDELETE(p_decl);
		}
		PPObjectTransmit::IncrementCharryOutCounter();
	}
	else
		ok = -1;
	CATCHZOK
	delete p_decl;
	SFile::ZClose(&stream);
	if(ok == 0 && fname.NotEmpty())
		SFile::Remove(fname);
	PPWaitStop();
	return ok;
}
//
//
//
class PrcssrMailCharry {
public:
	struct Param {
		Param() : MailAccID(0), Flags(0), DestPersonID(0)
		{
			DestAddr[0] = 0;
		}
		enum {
			fRemoveSrcFiles = 0x0001 // После успешной передачи удалить исходные файлы
		};
		PPID   MailAccID;
		long   Flags;
		PPID   DestPersonID;
		char   DestAddr[64];
	};
	PrcssrMailCharry()
	{
	}
	int    InitParam(Param * pParam)
	{
		RestoreParam(pParam);
		return 1;
	}
	int    EditParam(Param *);
	int    Init(const Param * pParam)
	{
		RVALUEPTR(P, pParam);
		return 1;
	}
	int    Run();
private:
	int    SaveParam(const Param *);
	int    RestoreParam(Param *);

	Param  P;
};

int PrcssrMailCharry::EditParam(Param * pParam)
{
	class MailCharryParamDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrMailCharry::Param);
	public:
		enum {
			ctlgroupMailAcc = 1
		};
		MailCharryParamDialog() : TDialog(DLG_MAILCHRY)
		{
			MailAccCtrlGroup * p_grp = new MailAccCtrlGroup(CTLSEL_MAILCHRY_MAILACC, cmEditMailAcc);
			addGroup(ctlgroupMailAcc, p_grp);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			ushort v = 0;
			MailAccCtrlGroup::Rec mac_rec;
			mac_rec.MailAccID = Data.MailAccID;
			mac_rec.ExtraPtr = reinterpret_cast<void *>(PPObjInternetAccount::filtfMail); // INETACCT_ONLYMAIL;
			setGroupData(ctlgroupMailAcc, &mac_rec);
			setCtrlData(CTL_MAILCHRY_DESTADDR, Data.DestAddr);
			SETFLAG(v, 0x01, Data.Flags & PrcssrMailCharry::Param::fRemoveSrcFiles);
			setCtrlData(CTL_MAILCHRY_FLAGS, &v);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v = 0;
			MailAccCtrlGroup::Rec mac_rec;
			sel = CTLSEL_MAILCHRY_MAILACC;
			getGroupData(ctlgroupMailAcc, &mac_rec);
			Data.MailAccID = mac_rec.MailAccID;
			THROW_PP(Data.MailAccID, PPERR_MAILACCNEEDED);
			getCtrlData(sel = CTL_MAILCHRY_DESTADDR, Data.DestAddr);
			{
				STokenRecognizer tr;
				SNaturalTokenArray nta;
				tr.Run(reinterpret_cast<const uchar *>(Data.DestAddr), -1, nta, 0);
				THROW_PP_S(nta.Has(SNTOK_EMAIL) > 0.0f, PPERR_INVEMAILADDR, Data.DestAddr);
			}
			getCtrlData(CTL_MAILCHRY_FLAGS, &(v = 0));
			SETFLAG(Data.Flags, PrcssrMailCharry::Param::fRemoveSrcFiles, v & 0x01);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmAddressBook)) {
				PPID   person_id = 0;
				SString addr;
				if(SelectAddressFromBook(&person_id, addr) > 0) {
					Data.DestPersonID = person_id;
					setCtrlString(CTL_MAILCHRY_DESTADDR, addr);
				}
			}
			else
				return;
			clearEvent(event);
		}
	};
	DIALOG_PROC_BODY(MailCharryParamDialog, pParam);
}

struct __MailCharryParam {
	PPID   Tag;            // Const PPOBJ_CONFIG
	PPID   ID;             // Const PPCFG_MAIN
	PPID   Prop;           // Const PPPRP_MAILCHARRYCFG
	char   Reserve[60];
	long   Flags;
	PPID   DestPersonID;   // -> Person.ID              Персоналия-получатель сообщения //
	PPID   MailAccID;      // -> Ref(PPOBJ_INTERNETACCOUNT) ИД учетной записи электронной почты
	char   DestAddr[64];
};

int PrcssrMailCharry::SaveParam(const Param * pParam)
{
	int    ok = 1;
	__MailCharryParam strg;
	MEMSZERO(strg);
	strg.MailAccID = pParam->MailAccID;
	strg.DestPersonID = pParam->DestPersonID;
	strg.Flags = pParam->Flags;
	strip(STRNSCPY(strg.DestAddr, pParam->DestAddr));
	THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MAILCHARRYCFG, &strg, sizeof(strg), 1));
	CATCHZOK
	return ok;
}

int PrcssrMailCharry::RestoreParam(Param * pParam)
{
	int    ok = 1;
	__MailCharryParam strg;
	memzero(pParam, sizeof(*pParam));
	int r = PPRef->GetPropMainConfig(PPPRP_MAILCHARRYCFG, &strg, sizeof(strg));
	if(!r)
		ok = 0;
	else if(r < 0) {
		PPAlbatrossConfig alb_cfg;
		if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0)
			pParam->MailAccID = alb_cfg.Hdr.MailAccID;
		ok = -1;
	}
	else {
		pParam->MailAccID = strg.MailAccID;
		pParam->Flags = strg.Flags;
		pParam->DestPersonID = strg.DestPersonID;
		STRNSCPY(pParam->DestAddr, strg.DestAddr);
	}
	return ok;
}

int PrcssrMailCharry::Run()
{
	int    ok = 1;
	SString src_path;
	SaveParam(&P);
	PPGetPath(PPPATH_OUT, src_path);
	SFileEntryPool fep;
	SString pattern;
	pattern.CatChar('*').Cat(PPConst::FnExt_CHARRY);
	THROW(fep.Scan(src_path, pattern, 0));
	THROW(PutFilesToEmail(/*&fary*/&fep, P.MailAccID, P.DestAddr, PPConst::P_SubjectCharry, 0));
	CATCHZOK
	return ok;
}

int SendCharryFiles()
{
	int    ok = -1;
	PrcssrMailCharry prcssr;
	PrcssrMailCharry::Param param;
	prcssr.InitParam(&param);
	while(ok <= 0 && prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}
//
//
//
RcvCharryParam::RcvCharryParam() : MailAccID(0), Action(0), Flags(0)
{
}

int RcvCharryParam::Edit()
{
	enum {
		ctlgroupMailAcc = 1
	};
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_RCVCHRY);
	if(CheckDialogPtrErr(&dlg)) {
		long   action = 0;
		MailAccCtrlGroup::Rec mac_rec;
		MailAccCtrlGroup * p_grp = new MailAccCtrlGroup(CTLSEL_RCVCHRY_MAILACC, cmEditMailAcc);
		dlg->addGroup(ctlgroupMailAcc, p_grp);
		mac_rec.MailAccID = MailAccID;
		mac_rec.ExtraPtr = reinterpret_cast<void *>(PPObjInternetAccount::filtfMail); // INETACCT_ONLYMAIL;
		dlg->setGroupData(ctlgroupMailAcc, &mac_rec);
		dlg->AddClusterAssoc(CTL_RCVCHRY_ACTION, 0, RcvCharryParam::aRcvFromMail);
		dlg->AddClusterAssoc(CTL_RCVCHRY_ACTION, 1, RcvCharryParam::aGetFromInPath);
		dlg->AddClusterAssoc(CTL_RCVCHRY_ACTION, 2, RcvCharryParam::aRcvFromFile);
		dlg->SetClusterData(CTL_RCVCHRY_ACTION, (action = (long)Action));
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->GetClusterData(CTL_RCVCHRY_ACTION, &action);
			Action = (int)action;
			dlg->getGroupData(ctlgroupMailAcc, &mac_rec);
			MailAccID = mac_rec.MailAccID;
			if(Action == RcvCharryParam::aRcvFromMail && MailAccID == 0)
				PPErrorByDialog(dlg, CTLSEL_RCVCHRY_MAILACC, PPERR_MAILACCNEEDED);
			else
				ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int ReceiveCharryObjects(const RcvCharryParam * pParam)
{
	int    ok = -1, r;
	RcvCharryParam rcp;
	rcp.Action = RcvCharryParam::aRcvFromMail;
	PPAlbatrossConfig alb_cfg;
	if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0)
		rcp.MailAccID = alb_cfg.Hdr.MailAccID;
	if(pParam || rcp.Edit() > 0) {
		uint   p;
		SString path, file_path;
		//PPFileNameArray fary;
		SFileEntryPool fep;
		RVALUEPTR(rcp, pParam);
		if(rcp.Action == RcvCharryParam::aRcvFromFile) {
			if(PPOpenFile(PPTXT_FILPAT_CHARRY, path, 0, APPL->H_MainWnd) > 0) {
				PPDeclStrucProcessor dsp;
				PPWaitStart();
				if(dsp.InitDataParsing(path) > 0) {
					PPDeclStruc * p_decl = 0;
					while((r = dsp.NextDecl(&p_decl)) > 0) {
						PPDeclStrucItem item;
						THROW(p_decl->GetItem(0, &item));
					}
					THROW(r);
				}
			}
		}
		else {
			SString pattern;
			pattern.CatChar('*').Cat(PPConst::FnExt_CHARRY);
			PPWaitStart();
			THROW(PPGetPath(PPPATH_IN, path));
			if(rcp.Action == RcvCharryParam::aRcvFromMail) {
				THROW(GetFilesFromMailServer2(rcp.MailAccID, path, SMailMessage::fPpyCharry, 0 /*don't clean*/, 1 /*dele msg*/));
			}
			THROW(fep.Scan(path.SetLastSlash(), pattern, 0));
			//for(p = 0; fary.Enum(&p, 0, &file_path);) {
			for(p = 0; p < fep.GetCount(); p++) {
				if(fep.Get(p, 0, &file_path)) {
					PPDeclStrucProcessor dsp;
					if(dsp.InitDataParsing(file_path) > 0) {
						PPDeclStruc * p_decl = 0;
						while((r = dsp.NextDecl(&p_decl)) > 0) {
							PPDeclStrucItem item;
							THROW(p_decl->GetItem(0, &item));
						}
						THROW(r);
					}
				}
			}
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

#if 0 // {

int TestDeclDefinitionParsing()
{
	PPDeclStrucProcessor dsp;
	if(!dsp.ParseDefinition("pp.ds", "ppdsdefs.rc", "ppdsdefs.h")) {
		char msg_buf[256];
		dsp.MakeErrMsgString(msg_buf, sizeof(msg_buf));
		PPOutputMessage(msg_buf, mfError | mfOK);
	}
	return 1;
}

int TestOprKindOutput()
{
	int    ok = 1;
	PPID   op_id;
	PPOprKind op_rec;
	PPDeclStruc * p_decl = 0;
	FILE * out = fopen("oprkind.", "w");
	THROW(out);
	THROW(p_decl = PPDeclStruc::CreateInstance(PPDS_OPRKIND, 0, 0, 0));
	for(op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
		if(p_decl->InitData(PPDeclStruc::idoExtract, 0, op_id) > 0) {
			THROW(p_decl->WriteDataToStream(out, 0));
		}
	}
	CATCHZOKPPERR
	delete p_decl;
	SFile::ZClose(&out);
	return ok;
}

int TestOprKindInput()
{
	int    ok = 1, r;
	PPDeclStrucProcessor dsp;
	PPObjOprKind op_obj;
	if(dsp.InitDataParsing("oprkind.") > 0) {
		PPDeclStruc * p_decl = 0;
		while((r = dsp.NextDecl(&p_decl)) > 0) {
			PPDeclStrucItem item;
			p_decl->GetItem(0, &item);
			if(item.ID == PPDS_OPRKIND) {
				op_obj.EditPacket(&((PPDS_OprKind *)p_decl)->Data);
			}
		}
		if(!r) {
			char msg_buf[256];
			dsp.MakeErrMsgString(msg_buf, sizeof(msg_buf));
			PPOutputMessage(msg_buf, mfError | mfOK);
		}
	}
	return ok;
}

#endif // } 0
