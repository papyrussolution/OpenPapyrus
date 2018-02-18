// ALBATROS.CPP
// Copyright (c) A.Starodub 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 @obsolete #include <albatros.h>
// @v9.6.3 #include <idea.h>

class AlbatrosConfigDialog : public TDialog {
public:
	AlbatrosConfigDialog() : TDialog(DLG_ALBTRCFG2)
	{
	}
	int    setDTS(const PPAlbatrosConfig *);
	int    getDTS(PPAlbatrosConfig *);
private:
	DECL_HANDLE_EVENT;
	int    EditVetisConfig();

	PPAlbatrosConfig Data;
};

int AlbatrosConfigDialog::EditVetisConfig()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_VETISCFG);
	if(CheckDialogPtrErr(&dlg)) {
		SString temp_buf;
		Data.GetExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf);
		dlg->setCtrlString(CTL_VETISCFG_USER, temp_buf);
		Data.GetPassword(ALBATROSEXSTR_VETISPASSW, temp_buf);
		dlg->setCtrlString(CTL_VETISCFG_PASSW, temp_buf);
		Data.GetExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf);
		dlg->setCtrlString(CTL_VETISCFG_APIKEY, temp_buf);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_VETISCFG_USER, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf.Strip());
			dlg->getCtrlString(CTL_VETISCFG_PASSW, temp_buf);
			Data.SetPassword(ALBATROSEXSTR_VETISPASSW, temp_buf);
			dlg->getCtrlString(CTL_VETISCFG_APIKEY, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

IMPL_HANDLE_EVENT(AlbatrosConfigDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmEditMailAcc)) {
		PPObjInternetAccount mac_obj;
		PPID   mac_id = getCtrlLong(CTLSEL_ALBTRCFG_MAILACC);
		if(mac_obj.Edit(&mac_id, 0) == cmOK)
			SetupPPObjCombo(this, CTLSEL_ALBTRCFG_MAILACC, PPOBJ_INTERNETACCOUNT, mac_id, OLW_CANINSERT, (void *)PPObjInternetAccount::filtfMail);
	}
	else if(event.isCmd(cmEditSmsAcc)) {
		PPObjSmsAccount mac_obj;
		PPID   mac_id = getCtrlLong(CTLSEL_ALBTRCFG_SMSACC);
		if(mac_obj.Edit(&mac_id, 0) == cmOK)
			SetupPPObjCombo(this, CTLSEL_ALBTRCFG_SMSACC, PPOBJ_SMSPRVACCOUNT, mac_id, OLW_CANINSERT, 0);
	}
	else if(event.isCmd(cmVetisConfig)) {
		EditVetisConfig();
	}
	else
		return;
	clearEvent(event);
}

int AlbatrosConfigDialog::setDTS(const PPAlbatrosConfig * pCfg)
{
	int    ok = 1;
	SString temp_buf;
	PPIDArray op_type_list;
	op_type_list.addzlist(PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0);
	if(!RVALUEPTR(Data, pCfg))
		MEMSZERO(Data);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_OPKINDID, Data.Hdr.OpID, 0, &op_type_list, 0);
	SetupPPObjCombo(this, CTLSEL_ALBTRCFG_MAILACC, PPOBJ_INTERNETACCOUNT, Data.Hdr.MailAccID, OLW_CANINSERT, (void *)PPObjInternetAccount::filtfMail);
	SetupPPObjCombo(this, CTLSEL_ALBTRCFG_SMSACC, PPOBJ_SMSPRVACCOUNT, Data.Hdr.SmsAccID, OLW_CANINSERT, 0);
	Data.GetExtStrData(ALBATROSEXSTR_UHTTURN, temp_buf);
	setCtrlString(CTL_ALBTRCFG_UHTTURN, temp_buf/*Data.UhttUrn*/);
	Data.GetExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
	setCtrlString(CTL_ALBTRCFG_UHTTURLPFX, temp_buf/*Data.UhttUrlPrefix*/);
	Data.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
	setCtrlString(CTL_ALBTRCFG_UHTTACCOUNT, temp_buf/*Data.UhttAccount*/);
	Data.GetPassword(ALBATROSEXSTR_UHTTPASSW, temp_buf);
	setCtrlString(CTL_ALBTRCFG_UHTTPASSW, temp_buf);
	Data.GetExtStrData(ALBATROSEXSTR_EGAISSRVURL, temp_buf);
	setCtrlString(CTL_ALBTRCFG_EGAISURL, temp_buf/*Data.EgaisServerURL*/);

	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EDIORD, Data.Hdr.EdiOrderOpID, OLW_CANINSERT, &op_type_list, 0);
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EDIORDSP, Data.Hdr.EdiOrderSpOpID, OLW_CANINSERT, &op_type_list, 0);
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_GOODSRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_DESADV, Data.Hdr.EdiDesadvOpID, OLW_CANINSERT, &op_type_list, 0);
	// @v8.8.0 {
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EGRCPTOP, Data.Hdr.EgaisRcptOpID, OLW_CANINSERT, &op_type_list, 0);
	// } @v8.8.0
	// @v8.9.6 {
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EGRETOP, Data.Hdr.EgaisRetOpID, OLW_CANINSERT, &op_type_list, 0);
	// } @v8.9.6
	{
		ObjTagFilt tf(PPOBJ_BILL);
		SetupObjTagCombo(this, CTLSEL_ALBTRCFG_RCVTAG, Data.Hdr.RcptTagID, OLW_CANINSERT, &tf);
	}
	{
		ObjTagFilt tf(PPOBJ_BILL);
		SetupObjTagCombo(this, CTLSEL_ALBTRCFG_TTNTAG, Data.Hdr.TtnTagID, OLW_CANINSERT, &tf);
	}
	// @v8.8.3 {
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 0, PPAlbatrosCfgHdr::fSkipBillWithUnresolvedItems);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 1, PPAlbatrosCfgHdr::fRecadvEvalByCorrBill);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 2, PPAlbatrosCfgHdr::fUncondAcceptEdiIntrMov); // @v9.0.0
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 3, PPAlbatrosCfgHdr::fUseOwnEgaisObjects); // @v9.0.2
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 4, PPAlbatrosCfgHdr::fUseDateInBillAnalog); // @v9.1.9
	SetClusterData(CTL_ALBTRCFG_FLAGS, Data.Hdr.Flags);
	// } @v8.8.3
	return ok;
}

int AlbatrosConfigDialog::getDTS(PPAlbatrosConfig * pCfg)
{
	SString temp_buf;
	getCtrlData(CTLSEL_ALBTRCFG_OPKINDID, &Data.Hdr.OpID);
	getCtrlData(CTLSEL_ALBTRCFG_MAILACC,  &Data.Hdr.MailAccID);
	getCtrlData(CTLSEL_ALBTRCFG_SMSACC,  &Data.Hdr.SmsAccID);
	getCtrlString(CTL_ALBTRCFG_UHTTURN, temp_buf);
	Data.PutExtStrData(ALBATROSEXSTR_UHTTURN, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTURLPFX, temp_buf/*Data.UhttUrlPrefix*/);
	Data.PutExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTACCOUNT, temp_buf/*Data.UhttAccount*/);
	Data.PutExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTPASSW, temp_buf);
	Data.SetPassword(ALBATROSEXSTR_UHTTPASSW, temp_buf);
	getCtrlString(CTL_ALBTRCFG_EGAISURL, temp_buf/*Data.EgaisServerURL*/); // @v8.8.0
	Data.PutExtStrData(ALBATROSEXSTR_EGAISSRVURL, temp_buf);

	getCtrlData(CTLSEL_ALBTRCFG_EDIORD, &Data.Hdr.EdiOrderOpID);
	getCtrlData(CTLSEL_ALBTRCFG_EDIORDSP, &Data.Hdr.EdiOrderSpOpID);
	getCtrlData(CTLSEL_ALBTRCFG_DESADV, &Data.Hdr.EdiDesadvOpID);
	getCtrlData(CTLSEL_ALBTRCFG_EGRCPTOP, &Data.Hdr.EgaisRcptOpID); // @v8.8.0
	getCtrlData(CTLSEL_ALBTRCFG_EGRETOP, &Data.Hdr.EgaisRetOpID); // @v8.9.6
	getCtrlData(CTLSEL_ALBTRCFG_RCVTAG, &Data.Hdr.RcptTagID);
	getCtrlData(CTLSEL_ALBTRCFG_TTNTAG, &Data.Hdr.TtnTagID);
	GetClusterData(CTL_ALBTRCFG_FLAGS, &Data.Hdr.Flags); // @v8.8.3

	ASSIGN_PTR(pCfg, Data);
	return 1;
}

SLAPI PPAlbatrosConfig::PPAlbatrosConfig()
{
	MEMSZERO(Hdr);
}

PPAlbatrosConfig & SLAPI PPAlbatrosConfig::Clear()
{
	MEMSZERO(Hdr);
	ExtString.Z();
	//UhttUrn = 0;
	//UhttUrlPrefix = 0;
	//UhttAccount = 0;
	//UhttPassword = 0;
	return *this;
}

int SLAPI PPAlbatrosConfig::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int SLAPI PPAlbatrosConfig::PutExtStrData(int fldID, const char * pStr) { return PPPutExtStrData(fldID, ExtString, pStr); }

#define UHTT_PW_SIZE 20 // @attention изменение значения требует конвертации хранимого пароля

int SLAPI PPAlbatrosConfig::SetPassword(int fld, const char * pPassword)
{
	int    ok = 1;
	SString temp_buf;
	if(oneof2(fld, ALBATROSEXSTR_UHTTPASSW, ALBATROSEXSTR_VETISPASSW)) {
		Reference::Helper_EncodeOtherPw(0, pPassword, UHTT_PW_SIZE, temp_buf/*UhttPassword*/);
		PutExtStrData(fld, temp_buf);
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPAlbatrosConfig::GetPassword(int fld, SString & rPw)
{
	rPw.Z();
	int    ok = 1;
	SString temp_buf;
	if(oneof2(fld, ALBATROSEXSTR_UHTTPASSW, ALBATROSEXSTR_VETISPASSW)) {
		GetExtStrData(fld, temp_buf);
		Reference::Helper_DecodeOtherPw(0, temp_buf/*UhttPassword*/, UHTT_PW_SIZE, rPw);
	}
	else
		ok = -1;
	return ok;
}

static const int16 AlbatrossStrIdList[] = { ALBATROSEXSTR_UHTTURN, ALBATROSEXSTR_UHTTURLPFX, ALBATROSEXSTR_UHTTACC, ALBATROSEXSTR_UHTTPASSW,
	ALBATROSEXSTR_EGAISSRVURL, ALBATROSEXSTR_VETISUSER, ALBATROSEXSTR_VETISPASSW, ALBATROSEXSTR_VETISAPIKEY };

//static
int SLAPI PPAlbatrosCfgMngr::Helper_Put(Reference * pRef, PPAlbatrosConfig * pCfg, int use_ta)
{
	int    ok = 1;
	size_t p = 0;
	uint   s = sizeof(pCfg->Hdr);
	SString temp_buf;
	SString tail, pw;
	PPTransaction tra(use_ta);
	THROW(tra);
	tail.Space().Z();
	for(uint i = 0; i < SIZEOFARRAY(AlbatrossStrIdList); i++) {
		const int str_id = AlbatrossStrIdList[i];
		pCfg->GetExtStrData(str_id, temp_buf);
		PPPutExtStrData(str_id, tail, temp_buf);
	}
	s += (uint)(tail.Len()+1);
	{
		STempBuffer buffer(s);
		THROW_SL(buffer.IsValid());
		pCfg->Hdr.Size = s;
		memcpy(buffer + p, &pCfg->Hdr, sizeof(pCfg->Hdr));
		p += sizeof(pCfg->Hdr);
		memcpy(buffer + p, (const char *)tail, tail.Len()+1);
		THROW(pRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG2, buffer, s, 0));
		DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_ALBATROS, 0, 0, 0);
	}
	THROW(tra.Commit());
	DS.FetchAlbatrosConfig(0); // dirty cache
	CATCHZOK
	return ok;
}

//static
int SLAPI PPAlbatrosCfgMngr::Put(PPAlbatrosConfig * pCfg, int use_ta)
{
	return Helper_Put(PPRef, pCfg, use_ta);
}

int SLAPI PPAlbatrosCfgMngr::Helper_Get(Reference * pRef, PPAlbatrosConfig * pCfg)
{
	int    ok = 1, r;
	SString tail;
	SString temp_buf;
	STempBuffer buffer(2048);
	pCfg->Clear();
	if(pRef) {
		THROW(r = pRef->GetPropMainConfig(PPPRP_ALBATROSCFG2, buffer, buffer.GetSize()));
		if(r > 0) {
			size_t sz = pCfg->Hdr.Size;
			if(sz > buffer.GetSize()) {
				THROW_SL(buffer.Alloc(sz));
				THROW(pRef->GetPropMainConfig(PPPRP_ALBATROSCFG2, buffer, sz) > 0);
			}
			memcpy(&pCfg->Hdr, buffer, sizeof(pCfg->Hdr));
			tail = ((const char *)buffer)+sizeof(pCfg->Hdr);
			for(uint i = 0; i < SIZEOFARRAY(AlbatrossStrIdList); i++) {
				const int str_id = AlbatrossStrIdList[i];
				PPGetExtStrData(str_id, tail, temp_buf);
				pCfg->PutExtStrData(str_id, temp_buf);
			}
		}
		else {
			//
			// Пытаемся найти запись в старом формате и конвертировать в новый
			//
			struct OldConfig {
				PPID   Tag;            // Const PPOBJ_CONFIG
				PPID   ID;             // Const PPCFG_MAIN
				PPID   Prop;           // Const PPPRP_ALBATROSCFG
				PPID   OpKindID;
				char   Login[20];
				char   Password[20];
				char   MailServer[48];
				char   MailAddr[48];
			};
			OldConfig old_cfg;
			if(pRef->GetPropMainConfig(PPPRP_ALBATROSCFG, &old_cfg, sizeof(old_cfg)) > 0) {
				PPID   mac_id = 0;
				PPAlbatrosCfgHdr cfg;
				PPObjInternetAccount mac_obj;
				PPInternetAccount mac;
				PPTransaction tra(1);
				THROW(tra);
				MEMSZERO(cfg);
				cfg.Tag = old_cfg.Tag;
				cfg.ID = old_cfg.ID;
				cfg.Prop = PPPRP_ALBATROSCFG2;
				cfg.OpID = old_cfg.OpKindID;

				STRNSCPY(mac.Name, "Albatros account");
				mac.SetExtField(MAEXSTR_SENDSERVER,  old_cfg.MailServer);
				mac.SetExtField(MAEXSTR_RCVSERVER,   old_cfg.MailServer);
				mac.SetExtField(MAEXSTR_RCVNAME,     old_cfg.Login);
				IdeaDecrypt(0, old_cfg.Password, sizeof(old_cfg.Password));
				mac.SetPassword(old_cfg.Password);
				mac.SetExtField(MAEXSTR_FROMADDRESS, old_cfg.MailAddr);

				THROW(mac_obj.Put(&mac_id, &mac, 0));
				cfg.MailAccID = mac_id;
				THROW(PPAlbatrosCfgMngr::Put(&cfg, 0));
				// Удаляем старую запись
				THROW(pRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG, 0, 0));
				DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_ALBATROS, 0, 0, 0);
				THROW(tra.Commit());
				THROW(PPAlbatrosCfgMngr::Helper_Get(pRef, &pCfg->Hdr)); // @recursion
			}
			else {
				ok = (PPErrCode = PPERR_UNDEFALBATROCONFIG, -1);
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_UNDEFALBATROCONFIG, -1);
	CATCHZOK
	return ok;
}

//static
int SLAPI PPAlbatrosCfgMngr::Get(PPAlbatrosConfig * pCfg)
{
	return Helper_Get(PPRef, pCfg);
}

//static
int SLAPI PPAlbatrosCfgMngr::Helper_Get(Reference * pRef, PPAlbatrosCfgHdr * pCfg)
{
	int    ok = 1, r;
	PPAlbatrosCfgHdr cfg;
	THROW(r = pRef->GetPropMainConfig(PPPRP_ALBATROSCFG2, &cfg, sizeof(cfg)));
	if(r < 0) {
		//
		// Пытаемся найти запись в старом формате и конвертировать в новый
		//
		struct OldConfig {
			PPID   Tag;            // Const PPOBJ_CONFIG
			PPID   ID;             // Const PPCFG_MAIN
			PPID   Prop;           // Const PPPRP_ALBATROSCFG
			PPID   OpKindID;
			char   Login[20];
			char   Password[20];
			char   MailServer[48];
			char   MailAddr[48];
		};
		OldConfig old_cfg;
		if(pRef->GetPropMainConfig(PPPRP_ALBATROSCFG, &old_cfg, sizeof(old_cfg)) > 0) {
			PPID   mac_id = 0;
			PPObjInternetAccount mac_obj;
			PPInternetAccount mac;
			{
				PPTransaction tra(1);
				THROW(tra);
				MEMSZERO(cfg);
				cfg.Tag = old_cfg.Tag;
				cfg.ID = old_cfg.ID;
				cfg.Prop = PPPRP_ALBATROSCFG2;
				cfg.OpID = old_cfg.OpKindID;

				STRNSCPY(mac.Name, "Albatros account");
				mac.SetExtField(MAEXSTR_SENDSERVER,  old_cfg.MailServer);
				mac.SetExtField(MAEXSTR_RCVSERVER,   old_cfg.MailServer);
				mac.SetExtField(MAEXSTR_RCVNAME,     old_cfg.Login);
				IdeaDecrypt(0, old_cfg.Password, sizeof(old_cfg.Password));
				mac.SetPassword(old_cfg.Password);
				mac.SetExtField(MAEXSTR_FROMADDRESS, old_cfg.MailAddr);

				THROW(mac_obj.Put(&mac_id, &mac, 0));
				cfg.MailAccID = mac_id;
				THROW(PPAlbatrosCfgMngr::Put(&cfg, 0));
				// Удаляем старую запись
				THROW(pRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG, 0, 0));
				DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_ALBATROS, 0, 0, 0);
				THROW(tra.Commit());
			}
			THROW(PPAlbatrosCfgMngr::Helper_Get(pRef, &cfg));
		}
		else {
			MEMSZERO(cfg);
			ok = (PPErrCode = PPERR_UNDEFALBATROCONFIG, -1);
		}
	}
	ASSIGN_PTR(pCfg, cfg);
	CATCHZOK
	return ok;
}

//static
int SLAPI PPAlbatrosCfgMngr::Get(PPAlbatrosCfgHdr * pCfg)
{
	return Helper_Get(PPRef, pCfg);
}

//static
int SLAPI PPAlbatrosCfgMngr::Put(const PPAlbatrosCfgHdr * pCfg, int use_ta)
{
	int    ok = 1;
	PPAlbatrosCfgHdr cfg = *pCfg;
	THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG2, &cfg, sizeof(cfg), use_ta));
	CATCHZOK
	return ok;
}

//static
int SLAPI PPAlbatrosCfgMngr::Edit()
{
	int    ok = -1, valid_data = 0, is_new = 0;
	AlbatrosConfigDialog * p_dlg = new AlbatrosConfigDialog();
	PPAlbatrosConfig cfg;

	THROW(CheckCfgRights(PPCFGOBJ_ALBATROS, PPR_READ, 0));
	MEMSZERO(cfg);
	THROW(is_new = PPAlbatrosCfgMngr::Get(&cfg));
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&cfg);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_ALBATROS, PPR_MOD, 0));
		if(p_dlg->getDTS(&cfg) > 0 && PPAlbatrosCfgMngr::Put(&cfg, 1)) {
			ok = valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

#if 0 // @v9.6.3 @obsolete {
void SLAPI AlbatrosOrder::Init()
{
	MEMSZERO(Head);
	Items.freeAll();
}
#endif // } 0 @v9.6.3 @obsolete
#if 0 // @v9.3.8 @obsolete {
//
// AlbatrosTagParser
//
SLAPI AlbatrosTagParser::AlbatrosTagParser()
{
	SymbNum = 0;
	P_TagValBuf = new char[ALBATROS_MAXTAGVALSIZE];
	PPLoadText(PPTXT_ALBATROSTAGNAMES, TagNamesStr);
	MEMSZERO(OrderItem);
}

SLAPI AlbatrosTagParser::~AlbatrosTagParser()
{
	delete [] P_TagValBuf;
}

int SLAPI AlbatrosTagParser::ProcessNext(AlbatrosOrder * pOrder, const char * pPath)
{
	P_Order = pOrder;
	return Run(pPath);
}

// static
int SLAPI AlbatrosTagParser::ResolveClientID(PPID inID, PPID opID, AlbatrosOrderHeader * pHead, PPID * pOutID, int use_ta)
{
	int    ok = -1, ta = 0;
	if(pHead && inID > 0) {
		int    found = 0;
		SString s_clid, tmp_name;
		uint   pos = 0;
		PPID   psn_id = 0;
		RegisterTbl::Rec reg_rec;
		PPPersonPacket packet, tmp_packet;
		s_clid.Cat(inID);
		if(RegObj.SearchByNumber(0, PPREGT_ALBATROSCLID, 0, s_clid, &reg_rec) > 0 && oneof2(reg_rec.ObjType, 0, PPOBJ_PERSON)) {
			found = 1;
			psn_id = reg_rec.ObjID;
		}
		if(RegObj.SearchByNumber(0, PPREGT_TPID, 0, pHead->ClientINN, &reg_rec) > 0 && oneof2(reg_rec.ObjType, 0, PPOBJ_PERSON)) {
			if(found && psn_id != reg_rec.ObjID) {
				THROW(PsnObj.GetPacket(psn_id, &packet, 0) > 0);
				if(packet.GetRegister(PPREGT_ALBATROSCLID, &pos) > 0 && pos > 0)
					packet.Regs.atFree(pos-1);
				THROW(PsnObj.PutPacket(&psn_id, &packet, use_ta) > 0);
				packet.destroy();
				found = 0;
			}
			psn_id = reg_rec.ObjID;
			if(found)
				ok = 1;
			else {
				THROW(PsnObj.GetPacket(psn_id, &packet, 0) > 0);
				tmp_name = packet.Rec.Name;
				THROW(LoadPersonPacket(pHead, inID, &packet, 1, use_ta) > 0);
				THROW((ok = ConfirmClientAdd(&packet, pHead->ClientINN, -1)));
				tmp_name.CopyTo(packet.Rec.Name, sizeof(packet.Rec.Name));
				if(ok > 0)
					THROW(PsnObj.PutPacket(&psn_id, &packet, use_ta) > 0);
			}
			if(ok > 0)
				THROW((ok = ResolveArticleByPerson(psn_id, opID, pOutID, use_ta)));
		}
		else // add person and article if user wish
			if(!found) {
				THROW(LoadPersonPacket(pHead, inID, &packet, 0, use_ta) > 0)
				THROW((ok = ConfirmClientAdd(&packet, pHead->ClientINN, 1)));
				if(ok > 0) {
					THROW(PsnObj.PutPacket(&(psn_id = 0), &packet, use_ta) > 0);
					if(RegObj.SearchByNumber(0, PPREGT_ALBATROSCLID, 0, s_clid, &reg_rec) > 0 && oneof2(reg_rec.ObjType, 0, PPOBJ_PERSON))
						THROW((ok = ResolveArticleByPerson(reg_rec.ObjID, opID, pOutID, use_ta)));
				}
			}
			else {
				psn_id = reg_rec.ObjID;
				THROW(PsnObj.GetPacket(psn_id, &packet, 0) > 0);
				tmp_name = packet.Rec.Name;
				THROW(LoadPersonPacket(pHead, 0, &packet, 1, use_ta) > 0);
				THROW((ok = ConfirmClientAdd(&packet, pHead->ClientINN, 0)));
				tmp_name.CopyTo(packet.Rec.Name, sizeof(packet.Rec.Name));
				if(ok > 0) {
					THROW(PsnObj.PutPacket(&psn_id, &packet, use_ta) > 0);
					if(RegObj.SearchByNumber(0, PPREGT_ALBATROSCLID, 0, s_clid, &reg_rec) > 0 && oneof2(reg_rec.ObjType, 0, PPOBJ_PERSON))
						THROW((ok = ResolveArticleByPerson(reg_rec.ObjID, opID, pOutID, use_ta)) > 0);
				}
			}
	}
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

//static
int SLAPI AlbatrosTagParser::ResolveArticleByPerson(PPID psnID, PPID opID, PPID * pOutID, int use_ta)
{
	int    ok = -1;
	if(psnID > 0) {
		PPOprKind opk_data;
		ArticleTbl::Rec ar_rec;
		THROW(GetOpData(opID, &opk_data) > 0);
		if(ArObj.P_Tbl->SearchObjRef(opk_data.AccSheetID, psnID, &ar_rec) > 0)
			ok = 1;
		else {
			PPAccSheet shtr;
			// add article
			THROW(SearchObject(PPOBJ_ACCSHEET, opk_data.AccSheetID, &shtr) > 0 && shtr.Assoc == PPOBJ_PERSON);
			if(PsnObj.P_Tbl->IsBelongToKind(psnID, shtr.ObjGroup) <= 0)
				THROW(PsnObj.P_Tbl->AddKind(psnID, shtr.ObjGroup, use_ta) > 0);
			THROW(ArObj.CreateObjRef(&ar_rec.ID, opk_data.AccSheetID, psnID, 0, use_ta) > 0);
			ok = 1;
		}
		ASSIGN_PTR(pOutID, ar_rec.ID);
	}
	CATCHZOK
	return ok;
}

class ClientAddDialog : public TDialog {
public:
	ClientAddDialog(int add, const char * pClientINNInOrder);
	int    setDTS(PPPersonPacket *);
	int    getDTS(PPPersonPacket *);
private:
	DECL_HANDLE_EVENT;
	char   ClientINNInOrder[48];
	int    ClientAdd;
};

ClientAddDialog::ClientAddDialog(int add, const char * pClientINNInOrder) : TDialog(DLG_CLINFO)
{
	ClientAdd = add;
	STRNSCPY(ClientINNInOrder, pClientINNInOrder);
	disableCtrls(1, CTL_CLINFO_CLNAME, CTL_CLINFO_CLORDERTPID, CTLSEL_CLINFO_CLCOUNTRY, CTLSEL_CLINFO_CLCITY,
		CTL_CLINFO_CLADDR, CTL_CLINFO_CLPHONE, CTL_CLINFO_CLMAIL, 0);
	disableCtrl(CTL_CLINFO_CLBASETPID, ClientAdd);
	enableCommand(cmChangeINN, !ClientAdd);
	if(ClientAdd <= 0) {
		SString buf;
		PPLoadText(ClientAdd ? PPTXT_CLCHANGEPSNTITLE : PPTXT_CLCHANGEINNTITLE, buf);
		setTitle(buf);
		PPGetSubStr(PPTXT_CLINFOTXTSTR, ClientAdd ? PPCLINFO_STR5 : PPCLINFO_STR1, buf);
		setStaticText(CTL_CLINFO_TEXT1, buf);
		PPGetSubStr(PPTXT_CLINFOTXTSTR, ClientAdd ? PPCLINFO_STR6 : PPCLINFO_STR2, buf);
		setStaticText(CTL_CLINFO_TEXT2, buf);
		PPGetSubStr(PPTXT_CLINFOTXTSTR, PPCLINFO_STR3, buf);
		setStaticText(CTL_CLINFO_TEXT3, buf);
		PPGetSubStr(PPTXT_CLINFOTXTSTR, PPCLINFO_STR4, buf);
		setStaticText(CTL_CLINFO_TEXT4, buf);
	}
}

int ClientAddDialog::setDTS(PPPersonPacket * pPacket)
{
	SString temp_buf;
	if(pPacket) {
		// set client info
		setCtrlData(CTL_CLINFO_CLNAME, pPacket->Rec.Name);
		pPacket->GetRegNumber(PPREGT_TPID, temp_buf);
		setCtrlString(CTL_CLINFO_CLBASETPID, temp_buf);
		setCtrlString(CTL_CLINFO_CLORDERTPID, temp_buf = ClientINNInOrder);
		SetupPPObjCombo(this, CTLSEL_CLINFO_CLCOUNTRY, PPOBJ_WORLD, pPacket->Loc.ParentID, 0, PPObjWorld::MakeExtraParam(WORLDOBJ_COUNTRY, 0, 0));
		SetupPPObjCombo(this, CTLSEL_CLINFO_CLCITY, PPOBJ_WORLD, pPacket->Loc.CityID, 0, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
		LocationCore::GetExField(&pPacket->Loc, LOCEXSTR_SHORTADDR, temp_buf);
		setCtrlString(CTL_CLINFO_CLADDR, temp_buf);
		if(pPacket->ELA.GetItem(PPELK_WORKPHONE, temp_buf) > 0)
			setCtrlString(CTL_CLINFO_CLPHONE, temp_buf);
		if(pPacket->ELA.GetItem(PPELK_EMAIL, temp_buf) > 0)
			setCtrlString(CTL_CLINFO_CLMAIL, temp_buf);
	}
	// disable all ctrls
	return 1;
}

int ClientAddDialog::getDTS(PPPersonPacket * pPacket)
{
	int    ok = -1;
	char   buf[64];
	uint   pos = 0;
	if(pPacket) {
		RegisterTbl::Rec reg_rec;
		PPObjRegister reg_obj;
		getCtrlData(CTL_CLINFO_CLBASETPID, buf);
		if(*strip(buf)) {
			MEMSZERO(reg_rec);
			reg_rec.RegTypeID = PPREGT_TPID;
			STRNSCPY(reg_rec.Num, buf);
			pPacket->GetRegister(PPREGT_TPID, &pos);
			if(reg_obj.CheckUniqueNumber(&reg_rec, &pPacket->Regs, PPOBJ_PERSON, pPacket->Rec.ID)) {
				if((ok = pPacket->Regs.insert(&reg_rec) ? 1 : PPSetErrorSLib()) > 0 && pos > 0)
					pPacket->Regs.atFree(pos-1);
			}
		}
		else
			ok = PPSetError(PPERR_USERINPUT);
	}
	if(ok == 0)
		selectCtrl(CTL_CLINFO_CLBASETPID);
	return ok;
}

IMPL_HANDLE_EVENT(ClientAddDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmChangeINN)) {
		char   buf[64];
		getCtrlData(CTL_CLINFO_CLORDERTPID, buf);
		setCtrlData(CTL_CLINFO_CLBASETPID, buf);
		clearEvent(event);
	}
}

//static
int SLAPI AlbatrosTagParser::ConfirmClientAdd(PPPersonPacket * pPack, const char * pClientINNInOrder, int add)
{
	int    ok = -1, valid_data = 0;
	ClientAddDialog *p_dlg = new ClientAddDialog(add, pClientINNInOrder);
	THROW(pPack && CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(pPack);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(pPack) > 0) {
			valid_data = 1;
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int SLAPI AlbatrosTagParser::ProcessTag(const char * pTag, long)
{
	int    tok = tokErr;
	char   tag_buf[64];
	while((tok = GetToken(pTag, tag_buf, sizeof(tag_buf))) != tokEOF && tok != tokEndTag && tok != tokErr) {
		if(tok == tokTag) {
			SymbNum = 0;
			if(ProcessTag(tag_buf, 0) == tokErr) {
				tok = tokErr;
				break;
			}
		}
		else if(SymbNum < (long) ALBATROS_MAXTAGVALSIZE && !oneof3(tag_buf[0], '\n', '\r', '\0')) {
			P_TagValBuf[SymbNum] = tag_buf[0];
			SymbNum++;
		}
	}
	if(tok != tokErr) {
		if(SymbNum < (long) ALBATROS_MAXTAGVALSIZE)
			P_TagValBuf[SymbNum] = '\0';
		if(!SaveTagVal(pTag))
			tok = tokErr;
	}
	return tok;
}

int SLAPI AlbatrosTagParser::LoadPersonPacket(AlbatrosOrderHeader * pHead, PPID albClID,
	PPPersonPacket * pPacket, int update, int use_ta)
{
	int    ok = -1, ta = 0;
	//BankAccountTbl::Rec bnk_rec;
	PPObjWorld w_obj;
	if(pHead && pPacket) {
		if(!update)
			pPacket->destroy();
		STRNSCPY(pPacket->Rec.Name, pHead->ClientName);
		pPacket->Rec.Status = PPPRS_LEGAL;
		if(albClID > 0) {
			uint   pos = 0;
			SString s_clid;
			s_clid.Cat(albClID);
			if(update)
				pPacket->GetRegister(PPREGT_ALBATROSCLID, &pos);
			THROW(pPacket->AddRegister(PPREGT_ALBATROSCLID, s_clid) > 0);
			if(update && pos > 0)
				pPacket->Regs.atFree(pos-1);
		}
		if(!update)
			pPacket->AddRegister(PPREGT_TPID, pHead->ClientINN);
		pPacket->ELA.AddItem(PPELK_WORKPHONE, pHead->ClientPhone);
		pPacket->ELA.AddItem(PPELK_EMAIL, pHead->ClientMail);
		LocationCore::SetExField(&pPacket->Loc, LOCEXSTR_SHORTADDR, pHead->ClientAddr);
		/* debug {
		MEMSZERO(bnk_rec);
		packet.BAA.insert(bnk_rec);
		} debug*/
		THROW(w_obj.AddSimple(&pPacket->Loc.CityID, WORLDOBJ_CITY, pHead->ClientCity, 0, 0));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI AlbatrosTagParser::SaveTagVal(const char * pTag)
{
	int    ok = 1;
	int    tag_idx = 0;
	char   buf[ALBATROS_MAXTAGVALSIZE];
	memzero(buf, sizeof(buf));
	if(PPSearchSubStr(TagNamesStr, &tag_idx, pTag, ALBATROS_MAXTAGSIZE) > 0) {
		if(P_TagValBuf && strip(P_TagValBuf)[0] != 0)
			decode64(P_TagValBuf, sstrlen(P_TagValBuf), buf, 0);
		SCharToOem(buf);
		switch(tag_idx) {
			// if get head items
			case PPALTAGNAM_ORDDT:
				strtodate(buf, DATF_DMY, &P_Order->Head.OrderDate);
				break;
			case PPALTAGNAM_ORDCODE:
				STRNSCPY(P_Order->Head.OrderCode, buf);
				break;
			case PPALTAGNAM_CLID:
				if(!strtolong(buf, &P_Order->Head.ClientID))
					ok = 0;
				break;
			case PPALTAGNAM_CLNAM:
				STRNSCPY(P_Order->Head.ClientName, buf);
				break;
			case PPALTAGNAM_CLINN:
				STRNSCPY(P_Order->Head.ClientINN, buf);
				break;
			case PPALTAGNAM_CLCITY:
				STRNSCPY(P_Order->Head.ClientCity, buf);
				break;
			case PPALTAGNAM_CLADDR:
				STRNSCPY(P_Order->Head.ClientAddr, buf);
				break;
			case PPALTAGNAM_CLPHONE:
				STRNSCPY(P_Order->Head.ClientPhone, buf);
				break;
			case PPALTAGNAM_CLMAIL:
				STRNSCPY(P_Order->Head.ClientMail, buf);
				break;
			case PPALTAGNAM_CLBNKACCT:
				STRNSCPY(P_Order->Head.ClientBankAcc, buf);
				break;
			case PPALTAGNAM_ORDAMT:
				if(!strtodoub(buf, &P_Order->Head.OrderAmount))
					ok = 0;
				break;
			case PPALTAGNAM_PCTDIS:
				if(!strtodoub(buf, &P_Order->Head.PctDis))
					ok = 0;
				break;
			// if get tag AlbatrosOrderItem
			case PPALTAGNAM_ALBORDITEM:
				P_Order->Items.insert(&OrderItem);
				MEMSZERO(OrderItem);
				break;
			// if get item item
			case PPALTAGNAM_GOODSID:
				if(!strtolong(buf, &OrderItem.GoodsID))
					ok = 0;
				break;
			case PPALTAGNAM_GOODSNAM:
				STRNSCPY(OrderItem.GoodsName, buf);
				break;
			case PPALTAGNAM_GOODSCODE:
				STRNSCPY(OrderItem.GoodsCode, buf);
				break;
			case PPALTAGNAM_UPP:
				if(!strtodoub(buf, &OrderItem.UnitsPerPack))
					ok = 0;
				break;
			case PPALTAGNAM_QTTY:
				if(!strtodoub(buf, &OrderItem.Qtty))
					ok = 0;
				break;
			case PPALTAGNAM_PRICE:
				if(!strtodoub(buf, &OrderItem.Price))
					ok = 0;
				break;
			case PPALTAGNAM_DISC:
				if(!strtodoub(buf, &OrderItem.Discount))
					ok = 0;
				break;
			case PPALTAGNAM_AMOUNT:
				if(!strtodoub(buf, &OrderItem.Amount))
					ok = 0;
				break;
			default:
				ok = -1;
		}
	}
	else
		ok = 0;
	return ok ? ok : (SLibError = SLERR_INVFORMAT, ok);
}

int SLAPI ImportOrders()
{
	int    ok = -1;
	int    clean = 0; // Очистить приемник от старых файлов
	char   str_ord_count[18];
	uint   j = 0;
	long   ord_count = 0;
	PPFileNameArray fary;
	SString path_in, file_path;
	SDirec sdirec;
	PPAlbatrosConfig cfg;
	AlbatrosOrder al_order;
	AlbatrosTagParser tag_pars;
	PPObjGoods goods_obj;

	THROW(PPAlbatrosCfgMngr::Get(&cfg));
	THROW_PP(cfg.Hdr.OpID > 0, PPERR_INVALBORDOPID);
	PPGetPath(PPPATH_IN, path_in);
	if(CONFIRM(PPCFM_DELOUTFILES))
		clean = 1;
	PPWait(1);
	// @v9.8.11 GetFilesFromMailServer-->GetFilesFromMailServer2
	THROW(GetFilesFromMailServer2(cfg.Hdr.MailAccID, path_in, SMailMessage::fPpyOrder, clean, 1 /* dele msg */));
	THROW(fary.Scan(path_in.SetLastSlash(), "*" ORDEXT));
	for(j = 0; fary.Enum(&j, 0, &file_path);) {
		int    r;
		uint   i;
		PPBillPacket pack;

		al_order.Init();
		THROW_SL(tag_pars.ProcessNext(&al_order, file_path) > 0);
		THROW(pack.CreateBlank(cfg.Hdr.OpID, 0L, 0, 1));
		STRNSCPY(pack.Rec.Code, al_order.Head.OrderCode);
		pack.Rec.Dt = al_order.Head.OrderDate;
		PPWait(0);
		THROW((r = tag_pars.ResolveClientID(al_order.Head.ClientID, cfg.Hdr.OpID, &al_order.Head, &pack.Rec.Object, 0)));
		PPWait(1);
		if(r > 0) {
			for(i = 0; i < al_order.Items.getCount(); i++) {
				PPTransferItem ti;
				Goods2Tbl::Rec goods_rec;
				THROW(ti.Init(&pack.Rec));
				if(goods_obj.Search(al_order.Items.at(i).GoodsID, &goods_rec) > 0) {
					ti.SetupGoods(al_order.Items.at(i).GoodsID);
					ti.Quantity_   = al_order.Items.at(i).Qtty;
					ti.Price       = al_order.Items.at(i).Price;
					ti.UnitPerPack = al_order.Items.at(i).UnitsPerPack;
					THROW(pack.InsertRow(&ti, 0));
				}
			}
			if(pack.GetTCount()) {
				double diff;
				BillTbl::Rec same_rec;
				pack.InitAmounts();
				r = BillObj->tbl->SearchAnalog(&pack.Rec, 0, &same_rec);
				if(r > 0)
					diff = R6(R2(same_rec.Amount) - R2(pack.Rec.Amount));
				if(r < 0 || diff != 0) {
					THROW(BillObj->FillTurnList(&pack));
					THROW(BillObj->TurnPacket(&pack, 1));
					ord_count++;
					ok = 1;
				}
			}
		}
	}
	PPWait(0);
	if(CONFIRM(PPCFM_DELINFILES))
		PPRemoveFiles(&fary);
	CATCHZOKPPERR
	ltoa(ord_count, str_ord_count, 10);
	PPMessage(mfInfo | mfOK, PPINF_RCVORDERSCOUNT, str_ord_count);
	return ok;
}
#endif // } 0 @v9.3.8 @obsolete
//
//
//
#if 0 // @construction {
//
// Управление публикацией цен для проекта Universe-HTT
//
/*
table UhttGoodsValueRel {
	autolong ID;
	long   Kind;
	long   SellerID;    // ->Person.ID   Продавец (избыточное поле: его значение может быть извлечено из Location по SellerLocID)
	long   SellerLocID; // ->Location.ID Локация продавца
	long   BuyerID;     // ->Person.ID   Если цена предлагается для конкретного контрагента, то BuyerID - ид персоналии-контрагента
index:
	ID (unique);
	Kind, SellerLocID, BuyerID (unique mod);
	Kind, BuyerID, SellerLocID (unique mod);
file:
	"uhttgvalrel.btr";
	balanced;
}

table UhttGoodsValue {
	long   RelID;          // UhttPriceRel.ID
	long   GoodsID;        // ->Goods2.ID
	long   CurID;          // ->Ref(PPOBJ_CURRENCY)
	double Value;          // Значение, сопоставленное с товаром и набором атрибутов RelID
	date   Dt;
	time   Tm;
	note   Memo[128];
index:
	GoodsID, RelID (unique mod);
	RelID, GoodsID (unique mod);
file:
	"uhttgval.btr";
	vlr;
	balanced;
}
*/

struct UhttGoodsValue {
	long   RelID;       // internal use
	long   Kind;
	PPID   SellerID;
	PPID   SellerLocID;
	PPID   BuyerID;
	PPID   GoodsID;
	PPID   CurID;
	LDATETIME Dtm;
	double Value;
};

struct UhttGoodsValueFilt {
	long   Kind;
	PPID   SellerID;
	PPID   SellerLocID;
	PPID   SellerLocWorldID;
	PPID   BuyerID;
	PPID   GoodsID;
	PPID   BrandID;
	PPID   CurID;
	long   Flags;
};

class UhttGoodsValueArray : public TSArray <UhttGoodsValue> {
public:
	UhttGoodsValueArray();
};

class UhttGoodsValueMgr {
public:
	SLAPI  UhttPriceMgr();
	SLAPI ~UhttGoodsValueMgr();
	int    SLAPI Set(UhttGoodsValue * pVal, int use_ta);
	int    SLAPI Remove(UhttGoodsValueFilt * pFilt, int use_ta);
	int    SLAPI Get(UhttGoodsValueFilt * pFilt, UhttGoodsValueArray * pList);
private:
	int    SLAPI GetRel(const UhttGoodsValue * pVal, PPID * pID, int createIfNExists, int use_ta);

	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjWorld WObj;
	TLP_MEMB(UhttGoodsValueTbl, P_Tbl);
	TLP_MEMB(UhttGoodsValueRelTbl, P_Rel);
};

UhttGoodsValueArray::UhttGoodsValueArray() : TSArray <UhttGoodsValue>()
{
}

TLP_IMPL(UhttGoodsValueMgr, UhttGoodsValueTbl, P_Tbl);
TLP_IMPL(UhttGoodsValueMgr, UhttGoodsValueRelTbl, P_Rel);

SLAPI  UhttGoodsValueMgr::UhttGoodsValueMgr()
{
	TLP_OPEN(P_Tbl);
	TLP_OPEN(P_Rel);
}

SLAPI UhttGoodsValueMgr::~UhttGoodsValueMgr()
{
	TLP_CLOSE(P_Tbl);
	TLP_CLOSE(P_Rel);
}

int SLAPI UhttGoodsValueMgr::GetRel(const UhttGoodsValue * pVal, PPID * pID, int createIfNExists, int use_ta)
{
	int    ok = -1;
	PPID   rel_id = 0;
	UhttGoodsValueRelTbl::Key1 k1;
	MEMSZERO(k1);
	k1.Kind = pVal->Kind;
	k1.SellerLocID = pVal->SellerLocID;
	k1.BuyerID = pVal->BuyerID;
	if(P_Rel->search(1, &k1, spEq)) {
		rel_id = P_Rel->data.ID;
		ok = 1;
	}
	else if(createIfNExists) {
		UhttGoodsValueRelTbl::Rec rec;
		MEMSZERO(rec);
		rec.Kind = pVal->Kind;
		rec.SellerID = pVal->SellerID;
		rec.SellerLocID = pVal->SellerLocID;
		rec.BuyerID = pVal->BuyerID;
		THROW(AddByID(P_Rel, &rel_id, &rec, use_ta));
		ok = 2;
	}
	CATCHZOK
	ASSIGN_PTR(pID, rel_id);
	return ok;
}

int SLAPI UhttGoodsValueMgr::Set(const UhttGoodsValue * pRec, int use_ta)
{
	int    ok = 1;
	SString temp_buf;
	LDATETIME dtm;
	UhttPriceTbl::Key0 k0;
	THROW_INVARG(pRec);
	THROW_PP_S(pRec->SellerLocID, PPERR_UHTTPRICE_INVSELLERLOCID, pRec->SellerLocID);
	THROW_PP_S(pRec->Price >= 0.0 && pRec->Price < 1.e9, PPERR_UHTTPRICE_INVPRICE, temp_buf.Z().Cat(pRec->Price, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		dtm = getcurdatetime_();
		pRec->Dt = dtm.d;
		pRec->Tm = dtm.t;
		MEMSZERO(k0);
		k0.SellerLocID = pRec->SellerLocID;
		k0.BuyerID = pRec->BuyerID;
		k0.GoodsID = pRec->GoodsID;
		if(P_Tbl->searchForUpdate(0, &k0, spEq)) {
			if(pRec->Price == 0.0) {
				THROW_DB(P_Tbl->deleteRec());
				ok = 3;
			}
			else {
				if(pRec->CurID != P_Tbl->data.CurID || pRec->Price != P_Tbl->data.Price || strcmp(pRec->Memo, P_Tbl->data.Memo) != 0) {
					THROW_DB(P_Tbl->updateRecBuf(pRec));
					ok = 2;
				}
				else
					ok = -1;
			}
		}
		else {
			THROW_DB(P_Tbl->insertRecBuf(pRec));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI UhttGoodsValueMgr::Get(UhttGoodsValueFilt * pFilt, UhttGoodsValueArray * pList)
{
	int    ok = -1, empty = 0;
	PPIDArray goods_list;
	PPIDArray seller_loc_list;
	if(pFilt->SellerLocID) {
		seller_loc_list.add(pFilt->SellerLocID);
	}
	else if(pFilt->SellerID) {
		PsnObj.GetDlvrLocList(pFilt->SellerID, &seller_loc_list);
	}
	seller_loc_list.sort();
	if(pFilt->GoodsID) {
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(pFilt->GoodsID, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC)) {
			goods_list.add(goods_rec.ID);
		}
		else {
			GoodsFilt gf;
			gf.GrpIDList.Add(pFilt->GoodsID);
			gf.BrandList.Add(pFilt->BrandID);
			GoodsIterator::GetListByFilt(&gf, &goods_list);
		}
		goods_list.sort();
		if(!goods_list.getCount())
			empty = 1;
	}
	if(!empty) {
		UhttPriceTbl::Key0 k0;
		UhttPriceTbl::Key1 k1;
		if(seller_loc_list.getCount()) {
			for(uint i = 0; i < seller_loc_list.getCount(); i++) {
				const PPID loc_id = seller_loc_list.get(i);
				BExtQuery q(P_Tbl, 1);
				DBQ * dbq = 0;
				MEMSZERO(k1);
				k1.SellerLocID = loc_id;
				dbq = &(*dbq && P_Tbl->SellerLocID == loc_id);
				if(goods_list.getCount() == 1) {
					k1.GoodsID = goods_list.get(0);
					dbq = &(*dbq && P_Tbl->GoodsID == k1.GoodsID);
				}
				dbq = ppcheckfiltid(dbq, P_Tbl->CurID, pFilt->CurID);
				q.selectAll().where(*dbq);
				for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
					if(pFilt->BuyerID && P_Tbl->data.BuyerID && P_Tbl->data.BuyerID != pFilt->BuyerID)
						continue;
					else if(goods_list.getCount() && !goods_list.bsearch(P_Tbl->data.GoodsID))
						continue;
					else if(pFilt->SellerLocWorldID) {
						LocationTbl::Rec loc_rec;
						if(P_Tbl->data.SellerLocID == 0)
							continue;
						else if(PsnObj.LocObj.Fetch(P_Tbl->data.SellerLocID, &loc_rec) > 0) {
							if(!WObj.IsChildOf(loc_rec.CityID, pFilt->SellerLocWorldID))
								continue;
						}
						else
							continue;
					}
					THROW_SL(pList->insert(&P_Tbl->data));
					ok = 1;
				}
			}
		}
		else if(goods_list.getCount()) {
			for(uint i = 0; i < goods_list.getCount(); i++) {
				const PPID goods_id = goods_list.get(i);
				BExtQuery q(P_Tbl, 0);
				DBQ * dbq = 0;
				MEMSZERO(k0);
				k0.GoodsID = goods_id;
				dbq = &(*dbq && P_Tbl->GoodsID == goods_id);
				if(seller_loc_list.getCount() == 1) {
					k0.SellerLocID = seller_loc_list.get(0);
					dbq = &(*dbq && P_Tbl->SellerLocID == k0.SellerLocID);
				}
				dbq = ppcheckfiltid(dbq, P_Tbl->CurID, pFilt->CurID);
				q.selectAll().where(*dbq);
				for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
					if(pFilt->BuyerID && P_Tbl->data.BuyerID && P_Tbl->data.BuyerID != pFilt->BuyerID)
						continue;
					else if(seller_loc_list.getCount() && !seller_loc_list.bsearch(P_Tbl->data.SellerLocID))
						continue;
					else if(pFilt->SellerLocWorldID) {
						LocationTbl::Rec loc_rec;
						if(P_Tbl->data.SellerLocID == 0)
							continue;
						else if(PsnObj.LocObj.Fetch(P_Tbl->data.SellerLocID, &loc_rec) > 0) {
							if(!WObj.IsChildOf(loc_rec.CityID, pFilt->SellerLocWorldID))
								continue;
						}
						else
							continue;
					}
					THROW_SL(pList->insert(&P_Tbl->data));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

#endif // } 0 @construction
