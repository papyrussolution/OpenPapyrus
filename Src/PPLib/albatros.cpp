// ALBATROS.CPP
// Copyright (c) A.Starodub 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

class AlbatrosConfigDialog : public TDialog {
public:
	AlbatrosConfigDialog() : TDialog(DLG_ALBTRCFG2)
	{
	}
	int    setDTS(const PPAlbatrossConfig *);
	int    getDTS(PPAlbatrossConfig *);
private:
	DECL_HANDLE_EVENT;
	int    EditVetisConfig();
	int    EditMqcConfig();

	PPAlbatrossConfig Data;
};

int AlbatrosConfigDialog::EditMqcConfig()
{
	class MqcConfigDialog : public TDialog {
		DECL_DIALOG_DATA(PPAlbatrossConfig);
	public:
		MqcConfigDialog() : TDialog(DLG_MQCCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			SString temp_buf;
			Data.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
			setCtrlString(CTL_MQCCFG_HOST, temp_buf);
			// @v10.6.0 {
			Data.GetExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, temp_buf);
			setCtrlString(CTL_MQCCFG_VIRTHOST, temp_buf);
			// } @v10.6.0 
			Data.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
			setCtrlString(CTL_MQCCFG_USER, temp_buf);
			Data.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
			setCtrlString(CTL_MQCCFG_SECRET, temp_buf);
			Data.GetExtStrData(ALBATROSEXSTR_MQC_DATADOMAIN, temp_buf);
			setCtrlString(CTL_MQCCFG_DATADOMAIN, temp_buf);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlString(CTL_MQCCFG_HOST, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf.Strip());
			// @v10.6.0 {
			getCtrlString(CTL_MQCCFG_VIRTHOST, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, temp_buf.Strip());
			// } @v10.6.0
			getCtrlString(CTL_MQCCFG_USER, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf.Strip());
			getCtrlString(CTL_MQCCFG_SECRET, temp_buf);
			Data.SetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
			getCtrlString(CTL_MQCCFG_DATADOMAIN, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_MQC_DATADOMAIN, temp_buf);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmMqcGetCommonConfig)) {
				SString temp_buf;
				PPUhttClient uhtt_cli;
				PPAlbatrossConfig temp_alb_cfg;
				if(uhtt_cli.GetCommonMqsConfig(temp_alb_cfg) > 0) {
					temp_alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
					setCtrlString(CTL_MQCCFG_HOST, temp_buf);
					temp_alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, temp_buf);
					setCtrlString(CTL_MQCCFG_VIRTHOST, temp_buf);
					temp_alb_cfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
					setCtrlString(CTL_MQCCFG_USER, temp_buf);
					temp_alb_cfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
					setCtrlString(CTL_MQCCFG_SECRET, temp_buf);
				}
				clearEvent(event);
			}
			else
				return;
		}
	};
	int    ok = -1;
	MqcConfigDialog * dlg = new MqcConfigDialog();
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&Data);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getDTS(&Data);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

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
		// @v10.1.0 {
		Data.GetExtStrData(ALBATROSEXSTR_VETISDOCTUSER, temp_buf);
		dlg->setCtrlString(CTL_VETISCFG_DOCTUSER, temp_buf);
		Data.GetPassword(ALBATROSEXSTR_VETISDOCTPASSW, temp_buf);
		dlg->setCtrlString(CTL_VETISCFG_DOCTPASSW, temp_buf);
		dlg->setCtrlData(CTL_VETISCFG_TIMEOUT, &Data.Hdr.VetisTimeout);
		dlg->setCtrlData(CTL_VETISCFG_DOCCRTDELAY, &Data.Hdr.VetisCertDelay); // @v10.1.10
		// } @v10.1.0
		dlg->AddClusterAssoc(CTL_VETISCFG_FLAGS, 0, Data.Hdr.fVetisTestContour); // @v10.5.1
		dlg->SetClusterData(CTL_VETISCFG_FLAGS, Data.Hdr.Flags); // @v10.5.1
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_VETISCFG_USER, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_VETISUSER, temp_buf.Strip());
			dlg->getCtrlString(CTL_VETISCFG_PASSW, temp_buf);
			Data.SetPassword(ALBATROSEXSTR_VETISPASSW, temp_buf);
			dlg->getCtrlString(CTL_VETISCFG_APIKEY, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_VETISAPIKEY, temp_buf);
			// @v10.1.0 {
			dlg->getCtrlString(CTL_VETISCFG_DOCTUSER, temp_buf);
			Data.PutExtStrData(ALBATROSEXSTR_VETISDOCTUSER, temp_buf.Strip());
			dlg->getCtrlString(CTL_VETISCFG_DOCTPASSW, temp_buf);
			Data.SetPassword(ALBATROSEXSTR_VETISDOCTPASSW, temp_buf);
			dlg->getCtrlData(CTL_VETISCFG_TIMEOUT, &Data.Hdr.VetisTimeout);
			// } @v10.1.0
			dlg->getCtrlData(CTL_VETISCFG_DOCCRTDELAY, &Data.Hdr.VetisCertDelay); // @v10.1.10
			dlg->GetClusterData(CTL_VETISCFG_FLAGS, &Data.Hdr.Flags); // @v10.5.1
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
			SetupPPObjCombo(this, CTLSEL_ALBTRCFG_MAILACC, PPOBJ_INTERNETACCOUNT, mac_id, OLW_CANINSERT, reinterpret_cast<void *>(PPObjInternetAccount::filtfMail));
	}
	else if(event.isCmd(cmEditSmsAcc)) {
		PPObjSmsAccount mac_obj;
		PPID   mac_id = getCtrlLong(CTLSEL_ALBTRCFG_SMSACC);
		if(mac_obj.Edit(&mac_id, 0) == cmOK)
			SetupPPObjCombo(this, CTLSEL_ALBTRCFG_SMSACC, PPOBJ_SMSPRVACCOUNT, mac_id, OLW_CANINSERT, 0);
	}
	else if(event.isCmd(cmVetisConfig))
		EditVetisConfig();
	else if(event.isCmd(cmMqcConfig))
		EditMqcConfig();
	else
		return;
	clearEvent(event);
}

int AlbatrosConfigDialog::setDTS(const PPAlbatrossConfig * pCfg)
{
	int    ok = 1;
	SString temp_buf;
	PPIDArray op_type_list;
	op_type_list.addzlist(PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0);
	if(!RVALUEPTR(Data, pCfg))
		MEMSZERO(Data);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_OPKINDID, Data.Hdr.OpID, 0, &op_type_list, 0);
	SetupPPObjCombo(this, CTLSEL_ALBTRCFG_MAILACC, PPOBJ_INTERNETACCOUNT, Data.Hdr.MailAccID, OLW_CANINSERT, reinterpret_cast<void *>(PPObjInternetAccount::filtfMail));
	SetupPPObjCombo(this, CTLSEL_ALBTRCFG_SMSACC, PPOBJ_SMSPRVACCOUNT, Data.Hdr.SmsAccID, OLW_CANINSERT, 0);
	// @v10.5.12 @unused Data.GetExtStrData(ALBATROSEXSTR_UHTTURN, temp_buf);
	// @v10.5.12 @unused setCtrlString(CTL_ALBTRCFG_UHTTURN, temp_buf/*Data.UhttUrn*/);
	disableCtrl(CTL_ALBTRCFG_UHTTURN, 1); // @v10.5.12 @unused
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
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EGRCPTOP, Data.Hdr.EgaisRcptOpID, OLW_CANINSERT, &op_type_list, 0);
	op_type_list.Z().addzlist(PPOPT_DRAFTRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_ALBTRCFG_EGRETOP, Data.Hdr.EgaisRetOpID, OLW_CANINSERT, &op_type_list, 0);
	{
		ObjTagFilt tf(PPOBJ_BILL);
		SetupObjTagCombo(this, CTLSEL_ALBTRCFG_RCVTAG, Data.Hdr.RcptTagID, OLW_CANINSERT, &tf);
	}
	{
		ObjTagFilt tf(PPOBJ_BILL);
		SetupObjTagCombo(this, CTLSEL_ALBTRCFG_TTNTAG, Data.Hdr.TtnTagID, OLW_CANINSERT, &tf);
	}
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 0, PPAlbatrosCfgHdr::fSkipBillWithUnresolvedItems);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 1, PPAlbatrosCfgHdr::fRecadvEvalByCorrBill);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 2, PPAlbatrosCfgHdr::fUncondAcceptEdiIntrMov);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 3, PPAlbatrosCfgHdr::fUseOwnEgaisObjects);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 4, PPAlbatrosCfgHdr::fUseDateInBillAnalog);
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 5, PPAlbatrosCfgHdr::fStrictExpGtinCheck); // @v10.0.04
	SetClusterData(CTL_ALBTRCFG_FLAGS, Data.Hdr.Flags);
	return ok;
}

int AlbatrosConfigDialog::getDTS(PPAlbatrossConfig * pCfg)
{
	SString temp_buf;
	getCtrlData(CTLSEL_ALBTRCFG_OPKINDID, &Data.Hdr.OpID);
	getCtrlData(CTLSEL_ALBTRCFG_MAILACC,  &Data.Hdr.MailAccID);
	getCtrlData(CTLSEL_ALBTRCFG_SMSACC,  &Data.Hdr.SmsAccID);
	// @v10.5.12 @unused getCtrlString(CTL_ALBTRCFG_UHTTURN, temp_buf);
	// @v10.5.12 @unused Data.PutExtStrData(ALBATROSEXSTR_UHTTURN, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTURLPFX, temp_buf/*Data.UhttUrlPrefix*/);
	Data.PutExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTACCOUNT, temp_buf/*Data.UhttAccount*/);
	Data.PutExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
	getCtrlString(CTL_ALBTRCFG_UHTTPASSW, temp_buf);
	Data.SetPassword(ALBATROSEXSTR_UHTTPASSW, temp_buf);
	getCtrlString(CTL_ALBTRCFG_EGAISURL, temp_buf/*Data.EgaisServerURL*/);
	Data.PutExtStrData(ALBATROSEXSTR_EGAISSRVURL, temp_buf);

	getCtrlData(CTLSEL_ALBTRCFG_EDIORD, &Data.Hdr.EdiOrderOpID);
	getCtrlData(CTLSEL_ALBTRCFG_EDIORDSP, &Data.Hdr.EdiOrderSpOpID);
	getCtrlData(CTLSEL_ALBTRCFG_DESADV, &Data.Hdr.EdiDesadvOpID);
	getCtrlData(CTLSEL_ALBTRCFG_EGRCPTOP, &Data.Hdr.EgaisRcptOpID);
	getCtrlData(CTLSEL_ALBTRCFG_EGRETOP, &Data.Hdr.EgaisRetOpID);
	getCtrlData(CTLSEL_ALBTRCFG_RCVTAG, &Data.Hdr.RcptTagID);
	getCtrlData(CTLSEL_ALBTRCFG_TTNTAG, &Data.Hdr.TtnTagID);
	GetClusterData(CTL_ALBTRCFG_FLAGS, &Data.Hdr.Flags);

	ASSIGN_PTR(pCfg, Data);
	return 1;
}

PPAlbatrossConfig::PPAlbatrossConfig()
{
	MEMSZERO(Hdr);
}

PPAlbatrossConfig & PPAlbatrossConfig::Z()
{
	MEMSZERO(Hdr);
	ExtString.Z();
	return *this;
}

int PPAlbatrossConfig::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPAlbatrossConfig::PutExtStrData(int fldID, const char * pStr) { return PPPutExtStrData(fldID, ExtString, pStr); }

// @v11.8.11 (moved to PPConst) #define UHTT_PW_SIZE 20 // @attention изменение значения требует конвертации хранимого пароля

int PPAlbatrossConfig::SetPassword(int fld, const char * pPassword)
{
	int    ok = 1;
	SString temp_buf;
	if(oneof4(fld, ALBATROSEXSTR_UHTTPASSW, ALBATROSEXSTR_VETISPASSW, ALBATROSEXSTR_VETISDOCTPASSW, ALBATROSEXSTR_MQC_SECRET)) {
		Reference::Helper_EncodeOtherPw(0, pPassword, PPConst::PwSize_UHTT, temp_buf/*UhttPassword*/);
		PutExtStrData(fld, temp_buf);
	}
	else
		ok = -1;
	return ok;
}

int PPAlbatrossConfig::GetPassword(int fld, SString & rPw) const
{
	rPw.Z();
	int    ok = 1;
	SString temp_buf;
	if(oneof4(fld, ALBATROSEXSTR_UHTTPASSW, ALBATROSEXSTR_VETISPASSW, ALBATROSEXSTR_VETISDOCTPASSW, ALBATROSEXSTR_MQC_SECRET)) {
		GetExtStrData(fld, temp_buf);
		Reference::Helper_DecodeOtherPw(0, temp_buf/*UhttPassword*/, PPConst::PwSize_UHTT, rPw);
	}
	else
		ok = -1;
	return ok;
}

/*static*/int PPAlbatrosCfgMngr::MakeCommonMqsConfigPacket(const PPAlbatrossConfig & rCfg, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	xmlTextWriter * p_x = 0;
	xmlBuffer * p_x_buf = 0;
	SString temp_buf;
	SString host;
	SString virthost;
	SString user;
	SString secret;
	rCfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, host);
	rCfg.GetExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, virthost);
	rCfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, user);
	rCfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, secret);
	if(host.NotEmptyS() && user.NotEmptyS() && secret.NotEmptyS()) {
		{
			THROW(p_x_buf = xmlBufferCreate());
			THROW(p_x = xmlNewTextWriterMemory(p_x_buf, 0));
			{
				SXml::WDoc _doc(p_x, cpUTF8);
				{
					SXml::WNode n_h(_doc, "CommonMqsConfig");
					n_h.PutInner("host", host);
					n_h.PutInner("virtual-host", virthost);
					n_h.PutInner("user", user);
					n_h.PutInner("secret", secret);
				}
			}
			xmlTextWriterFlush(p_x);
			temp_buf.CatN(reinterpret_cast<char *>(p_x_buf->content), p_x_buf->use);
		}
		//temp_buf.Z().Cat(host).Colon().Cat(user).Colon().Cat(secret);
		size_t len = ALIGNSIZE(temp_buf.Len()+1, 4); // Размер блока AES 128 bit (16 bytes = 2^4)
		STempBuffer cbuf(len + 64); // 64 ensurance
		STempBuffer cbuf2(len + 64); // 64 ensurance
		memcpy(cbuf.vptr(), temp_buf.cptr(), temp_buf.Len());
		PTR8(cbuf.vptr())[temp_buf.Len()] = 0;
		SlCrypto cryp(SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
		SlCrypto::Key crypkey;
		size_t cryp_size = 0;
		uint8 key_buf[16]; //
		memzero(key_buf, sizeof(key_buf));
		key_buf[2] = 1;
		key_buf[7] = 17;
		THROW(cryp.SetupKey(crypkey, key_buf, sizeof(key_buf), 0, 0));
		THROW(cryp.Encrypt_(&crypkey, cbuf.vcptr(), len, cbuf2.vptr(), cbuf2.GetSize(), &cryp_size));
		temp_buf.Z().EncodeMime64(cbuf2.vcptr(), cryp_size);
		rBuf = temp_buf;
	}
	else
		ok = -1;
	CATCHZOK
	xmlFreeTextWriter(p_x);
	xmlBufferFree(p_x_buf);
	return ok;
}

/*static*/int PPAlbatrosCfgMngr::ParseCommonMqsConfigPacket(const char * pBuf, PPAlbatrossConfig * pCfg)
{
	int    ok = -1;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	SString src_buf(pBuf);
	STempBuffer cbuf(1024);
	SString temp_buf;
	SlCrypto cryp(SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
	SlCrypto::Key crypkey;
	size_t decryp_size = 0;
	uint8 key_buf[16]; //
	size_t actual_size = 0;
	THROW_SL(src_buf.DecodeMime64(cbuf, cbuf.GetSize(), &actual_size));
	{
		STempBuffer cbuf2(actual_size+64); // 64 ensurance
		memzero(key_buf, sizeof(key_buf));
		key_buf[2] = 1;
		key_buf[7] = 17;
		THROW(cryp.SetupKey(crypkey, key_buf, sizeof(key_buf), 0, 0));
		THROW(cryp.Decrypt_(&crypkey, cbuf.vcptr(), actual_size, cbuf2.vptr(), cbuf2.GetSize(), &decryp_size));
		if(pCfg) {
			SString result_buf;
			result_buf.CatN(cbuf2.cptr(), actual_size);
			{
				const xmlNode * p_root = 0;
				THROW(p_ctx = xmlNewParserCtxt());
				THROW_LXML((p_doc = xmlCtxtReadMemory(p_ctx, cbuf2.cptr(), actual_size, 0, 0, XML_PARSE_NOENT)), p_ctx);
				THROW(p_root = xmlDocGetRootElement(p_doc));
				if(SXml::IsName(p_root, "CommonMqsConfig")) {
					if(pCfg) {
						for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
							if(SXml::GetContentByName(p_c, "host", temp_buf))
								pCfg->PutExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
							else if(SXml::GetContentByName(p_c, "virtual-host", temp_buf)) // @v10.6.0
								pCfg->PutExtStrData(ALBATROSEXSTR_MQC_VIRTHOST, temp_buf);
							else if(SXml::GetContentByName(p_c, "user", temp_buf))
								pCfg->PutExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
							else if(SXml::GetContentByName(p_c, "secret", temp_buf))
								pCfg->SetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
						}
					}
					ok = 1;
				}
			}
			/*
			StringSet ss(':', result_buf);
			uint tn = 0;
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				tn++;
				if(tn == 1) {
					pCfg->PutExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
				}
				else if(tn == 2) {
					pCfg->PutExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
				}
				else if(tn == 3) {
					pCfg->SetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
				}
			}
			*/
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

static const int16 AlbatrossStrIdList[] = { ALBATROSEXSTR_UHTTURN_unused, ALBATROSEXSTR_UHTTURLPFX, ALBATROSEXSTR_UHTTACC, ALBATROSEXSTR_UHTTPASSW,
	ALBATROSEXSTR_EGAISSRVURL, ALBATROSEXSTR_VETISUSER, ALBATROSEXSTR_VETISPASSW, ALBATROSEXSTR_VETISAPIKEY,
	ALBATROSEXSTR_VETISDOCTUSER, ALBATROSEXSTR_VETISDOCTPASSW, ALBATROSEXSTR_MQC_HOST, ALBATROSEXSTR_MQC_USER, ALBATROSEXSTR_MQC_SECRET, ALBATROSEXSTR_MQC_DATADOMAIN };

/*static*/int PPAlbatrosCfgMngr::Helper_Put(Reference * pRef, PPAlbatrossConfig * pCfg, int use_ta)
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
		memcpy(buffer + p, tail.cptr(), tail.Len()+1);
		THROW(pRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG2, buffer, s, 0));
		DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_ALBATROS, 0, 0, 0);
	}
	THROW(tra.Commit());
	DS.FetchAlbatrosConfig(0); // dirty cache
	CATCHZOK
	return ok;
}

/*static*/int PPAlbatrosCfgMngr::Put(PPAlbatrossConfig * pCfg, int use_ta)
{
	return Helper_Put(PPRef, pCfg, use_ta);
}

int PPAlbatrosCfgMngr::Helper_Get(Reference * pRef, PPAlbatrossConfig * pCfg)
{
	int    ok = 1, r;
	SString tail;
	SString temp_buf;
	STempBuffer buffer(2048);
	pCfg->Z();
	if(pRef) {
		THROW(r = pRef->GetPropMainConfig(PPPRP_ALBATROSCFG2, buffer, buffer.GetSize()));
		if(r > 0) {
			size_t sz = pCfg->Hdr.Size;
			if(sz > buffer.GetSize()) {
				THROW_SL(buffer.Alloc(sz));
				THROW(pRef->GetPropMainConfig(PPPRP_ALBATROSCFG2, buffer, sz) > 0);
			}
			memcpy(&pCfg->Hdr, buffer, sizeof(pCfg->Hdr));
			tail = buffer.cptr()+sizeof(pCfg->Hdr);
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
				mac.SetPassword_(old_cfg.Password, MAEXSTR_RCVPASSWORD);
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

/*static*/int PPAlbatrosCfgMngr::Get(PPAlbatrossConfig * pCfg)
{
	return Helper_Get(PPRef, pCfg);
}

/*static*/int PPAlbatrosCfgMngr::Helper_Get(Reference * pRef, PPAlbatrosCfgHdr * pCfg)
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
				mac.SetPassword_(old_cfg.Password, MAEXSTR_RCVPASSWORD);
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

/*static*/int PPAlbatrosCfgMngr::Get(PPAlbatrosCfgHdr * pCfg)
{
	return Helper_Get(PPRef, pCfg);
}

/*static*/int PPAlbatrosCfgMngr::Put(const PPAlbatrosCfgHdr * pCfg, int use_ta)
{
	int    ok = 1;
	PPAlbatrosCfgHdr cfg = *pCfg;
	THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALBATROSCFG2, &cfg, sizeof(cfg), use_ta));
	CATCHZOK
	return ok;
}

/*static*/int PPAlbatrosCfgMngr::Edit()
{
	int    ok = -1;
	int    valid_data = 0;
	int    is_new = 0;
	AlbatrosConfigDialog * p_dlg = new AlbatrosConfigDialog();
	PPAlbatrossConfig cfg;
	THROW(CheckCfgRights(PPCFGOBJ_ALBATROS, PPR_READ, 0));
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
	UhttPriceMgr();
	~UhttGoodsValueMgr();
	int    Set(UhttGoodsValue * pVal, int use_ta);
	int    Remove(UhttGoodsValueFilt * pFilt, int use_ta);
	int    Get(UhttGoodsValueFilt * pFilt, UhttGoodsValueArray * pList);
private:
	int    GetRel(const UhttGoodsValue * pVal, PPID * pID, int createIfNExists, int use_ta);

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

UhttGoodsValueMgr::UhttGoodsValueMgr()
{
	TLP_OPEN(P_Tbl);
	TLP_OPEN(P_Rel);
}

UhttGoodsValueMgr::~UhttGoodsValueMgr()
{
	TLP_CLOSE(P_Tbl);
	TLP_CLOSE(P_Rel);
}

int UhttGoodsValueMgr::GetRel(const UhttGoodsValue * pVal, PPID * pID, int createIfNExists, int use_ta)
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

int UhttGoodsValueMgr::Set(const UhttGoodsValue * pRec, int use_ta)
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

int UhttGoodsValueMgr::Get(UhttGoodsValueFilt * pFilt, UhttGoodsValueArray * pList)
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
				const  PPID loc_id = seller_loc_list.get(i);
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
				for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
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
				const  PPID goods_id = goods_list.get(i);
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
				for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
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
