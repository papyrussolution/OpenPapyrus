// ALBATROS.CPP
// Copyright (c) A.Starodub 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
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
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 2, PPAlbatrosCfgHdr::fUncondAcceptEdiIntrMov); // @v9.0.0
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 3, PPAlbatrosCfgHdr::fUseOwnEgaisObjects); // @v9.0.2
	AddClusterAssoc(CTL_ALBTRCFG_FLAGS, 4, PPAlbatrosCfgHdr::fUseDateInBillAnalog); // @v9.1.9
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

#define UHTT_PW_SIZE 20 // @attention изменение значения требует конвертации хранимого пароля

int PPAlbatrossConfig::SetPassword(int fld, const char * pPassword)
{
	int    ok = 1;
	SString temp_buf;
	if(oneof4(fld, ALBATROSEXSTR_UHTTPASSW, ALBATROSEXSTR_VETISPASSW, ALBATROSEXSTR_VETISDOCTPASSW, ALBATROSEXSTR_MQC_SECRET)) {
		Reference::Helper_EncodeOtherPw(0, pPassword, UHTT_PW_SIZE, temp_buf/*UhttPassword*/);
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
		Reference::Helper_DecodeOtherPw(0, temp_buf/*UhttPassword*/, UHTT_PW_SIZE, rPw);
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
					n_h.PutInner("virtual-host", virthost); // @v10.6.0
					n_h.PutInner("user", user);
					n_h.PutInner("secret", secret);
				}
			}
			xmlTextWriterFlush(p_x);
			temp_buf.CatN(reinterpret_cast<char *>(p_x_buf->content), p_x_buf->use);
		}
		//temp_buf.Z().Cat(host).CatChar(':').Cat(user).CatChar(':').Cat(secret);
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
				xmlNode * p_root = 0;
				THROW(p_ctx = xmlNewParserCtxt());
				THROW_LXML((p_doc = xmlCtxtReadMemory(p_ctx, cbuf2.cptr(), actual_size, 0, 0, XML_PARSE_NOENT)), p_ctx);
				THROW(p_root = xmlDocGetRootElement(p_doc));
				if(SXml::IsName(p_root, "CommonMqsConfig")) {
					if(pCfg) {
						for(xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
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
	// @v11.1.10 @ctr MEMSZERO(cfg);
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
void AlbatrosOrder::Init()
{
	MEMSZERO(Head);
	Items.freeAll();
}
#endif // } 0 @v9.6.3 @obsolete
#if 0 // @v9.3.8 @obsolete {
//
// AlbatrosTagParser
//
AlbatrosTagParser::AlbatrosTagParser()
{
	SymbNum = 0;
	P_TagValBuf = new char[ALBATROS_MAXTAGVALSIZE];
	PPLoadText(PPTXT_ALBATROSTAGNAMES, TagNamesStr);
	MEMSZERO(OrderItem);
}

AlbatrosTagParser::~AlbatrosTagParser()
{
	delete [] P_TagValBuf;
}

int AlbatrosTagParser::ProcessNext(AlbatrosOrder * pOrder, const char * pPath)
{
	P_Order = pOrder;
	return Run(pPath);
}

/*static*/int AlbatrosTagParser::ResolveClientID(PPID inID, PPID opID, AlbatrosOrderHeader * pHead, PPID * pOutID, int use_ta)
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

/*static*/int AlbatrosTagParser::ResolveArticleByPerson(PPID psnID, PPID opID, PPID * pOutID, int use_ta)
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

// @v10.5.3 Описание диалога перенесено сюда из ppw.rc дабы не заниматься не нужной языковой локализацией строк {
/*
DLG_CLINFO DIALOGEX 52, 0, 332, 181
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Будет добавлена персоналия"
FONT 8, "MS Sans Serif", 0, 0, 0x0
BEGIN
    LTEXT           "@name",4001,10,10,16,8
    EDITTEXT        CTL_CLINFO_CLNAME,45,10,130,13,ES_AUTOHSCROLL
    LTEXT           "@country",4005,10,30,28,8
    EDITTEXT        CTL_CLINFO_CLCOUNTRY,45,30,130,13,ES_AUTOHSCROLL | ES_READONLY
    PUSHBUTTON      "",CTLSEL_CLINFO_CLCOUNTRY,175,30,12,13,BS_BITMAP
    LTEXT           "@city",4006,10,45,24,8
    EDITTEXT        CTL_CLINFO_CLCITY,45,45,130,13,ES_AUTOHSCROLL | ES_READONLY
    PUSHBUTTON      "",CTLSEL_CLINFO_CLCITY,175,45,12,13,BS_BITMAP
    LTEXT           "@address",4007,10,60,24,8
    EDITTEXT        CTL_CLINFO_CLADDR,45,60,130,13,ES_AUTOHSCROLL
    LTEXT           "@phone",4008,10,75,30,8
    EDITTEXT        CTL_CLINFO_CLPHONE,45,75,130,13,ES_AUTOHSCROLL
    LTEXT           "@elink",4009,10,95,72,8
    EDITTEXT        CTL_CLINFO_CLMAIL,115,95,130,13,ES_AUTOHSCROLL
    LTEXT           "&ИНН клиента в базе данных",4002,195,25,100,8
    EDITTEXT        CTL_CLINFO_CLBASETPID,195,36,130,13,ES_AUTOHSCROLL
    LTEXT           "И&НН клиента в заказе",4003,195,50,84,8
    EDITTEXT        CTL_CLINFO_CLORDERTPID,195,60,130,13,ES_AUTOHSCROLL
    PUSHBUTTON      "&Заменить ИНН",4004,260,75,65,13
    LTEXT           "В принимаемом заказе, указан клиент, который не найден",CTL_CLINFO_TEXT1,115,110,205,8
    LTEXT           "в текущей базе данных. Кнопка ОК - добавить клиента и",CTL_CLINFO_TEXT2,115,120,205,8
    LTEXT           "принять заказ, кнопка Отмена - не добавлять клиента и",CTL_CLINFO_TEXT3,115,130,205,8
    LTEXT           "не принимать заказ.",CTL_CLINFO_TEXT4,115,140,205,8
    DEFPUSHBUTTON   "@but_ok",4010,220,160,50,13
    PUSHBUTTON      "@but_cancel",4011,275,160,50,13
END
*/
// } @v10.5.3

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

/*static*/int AlbatrosTagParser::ConfirmClientAdd(PPPersonPacket * pPack, const char * pClientINNInOrder, int add)
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

int AlbatrosTagParser::ProcessTag(const char * pTag, long)
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

int AlbatrosTagParser::LoadPersonPacket(AlbatrosOrderHeader * pHead, PPID albClID,
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

int AlbatrosTagParser::SaveTagVal(const char * pTag)
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
			case PPALTAGNAM_ORDDT: strtodate(buf, DATF_DMY, &P_Order->Head.OrderDate); break;
			case PPALTAGNAM_ORDCODE: STRNSCPY(P_Order->Head.OrderCode, buf); break;
			case PPALTAGNAM_CLID:
				if(!strtolong(buf, &P_Order->Head.ClientID))
					ok = 0;
				break;
			case PPALTAGNAM_CLNAM: STRNSCPY(P_Order->Head.ClientName, buf); break;
			case PPALTAGNAM_CLINN: STRNSCPY(P_Order->Head.ClientINN, buf); break;
			case PPALTAGNAM_CLCITY: STRNSCPY(P_Order->Head.ClientCity, buf); break;
			case PPALTAGNAM_CLADDR: STRNSCPY(P_Order->Head.ClientAddr, buf); break;
			case PPALTAGNAM_CLPHONE: STRNSCPY(P_Order->Head.ClientPhone, buf); break;
			case PPALTAGNAM_CLMAIL: STRNSCPY(P_Order->Head.ClientMail, buf); break;
			case PPALTAGNAM_CLBNKACCT: STRNSCPY(P_Order->Head.ClientBankAcc, buf); break;
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
			case PPALTAGNAM_GOODSNAM: STRNSCPY(OrderItem.GoodsName, buf); break;
			case PPALTAGNAM_GOODSCODE: STRNSCPY(OrderItem.GoodsCode, buf); break;
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

int ImportOrders()
{
	int    ok = -1;
	int    clean = 0; // Очистить приемник от старых файлов
	char   str_ord_count[18];
	uint   j = 0;
	long   ord_count = 0;
	PPFileNameArray fary;
	SString path_in, file_path;
	SDirec sdirec;
	PPAlbatrossConfig cfg;
	AlbatrosOrder al_order;
	AlbatrosTagParser tag_pars;
	PPObjGoods goods_obj;

	THROW(PPAlbatrosCfgMngr::Get(&cfg));
	THROW_PP(cfg.Hdr.OpID > 0, PPERR_INVALBORDOPID);
	PPGetPath(PPPATH_IN, path_in);
	if(CONFIRM(PPCFM_DELOUTFILES))
		clean = 1;
	PPWaitStart();
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
		PPWaitStop();
		THROW((r = tag_pars.ResolveClientID(al_order.Head.ClientID, cfg.Hdr.OpID, &al_order.Head, &pack.Rec.Object, 0)));
		PPWaitStart();
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
	PPWaitStop();
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
