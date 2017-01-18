// PPSMS.CPP
// Copyright (c) V.Miller 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <slib.h>
#include <pp.h>
#pragma hdrstop
#include <idea.h>
#include <ppsms.h>
//
//
//
StConfig::StConfig()
{
	Clear();
}

void StConfig::Clear()
{
	Host = 0;
	Port = 0;
	SystemId = 0;
	Login = 0;
	Password = 0;
	SystemType = 0;
	SourceAddressTon = TON_UNKNOWN;
	SourceAddressNpi = NPI_UNKNOWN;
	DestAddressTon = TON_UNKNOWN;
	DestAddressNpi = NPI_UNKNOWN;
	DataCoding = SMSC_DEFAULT;
	AddressRange = 0;
	EsmClass = 0;
	From = 0;
	SplitLongMsg = 0;
	ResponseTimeout = 0;
	ResendMsgQueueTimeout = 0;
	ResendTriesNum = 0;
	ReconnectTimeout = 0;
	ReconnectTriesNum = 0;
}

int StConfig::SetConfig(const char * pHost, uint port, const char * pSystemId, const char * pLogin,
	const char * pPassword, const char * pSystemType, uint sourceAddressTon, uint sourceAddressNpi, uint destAddressTon, uint destAddressNpi, uint dataCoding, const char * pFrom, uint splitLongMsg)
{
	int    ok = 1;
	SString from;
	THROW_PP(!isempty(pHost), PPERR_SMS_HOSTNEEDED);
	THROW_PP(!isempty(pSystemId), PPERR_SMS_SYSTEMIDNEEDED);
	THROW_PP(!isempty(pPassword), PPERR_SMS_PASSWNEEDED);
	THROW_PP(port != 0, PPERR_SMS_INVPORT);
	THROW_PP(strlen(pSystemId) < MAX_SYSTEM_ID_LEN, PPERR_SMS_SYSTEMIDERRLEN);
	THROW_PP(strlen(pPassword) < MAX_PASSWORD_LEN, PPERR_SMS_PASSWERRLEN);
	THROW_PP(strlen(pSystemType) < MAX_SYSTEM_TYPE_LEN, PPERR_SMS_SYSTYPEERRLEN);
	// Параметры VALIDITY_PERIOD и SCHEDULE_DELIVERY_TIME могут иметь длину 1 или 17
	THROW_PP((strlen(VALIDITY_PERIOD) < MAX_DATE_LEN) && !((strlen(VALIDITY_PERIOD) > 0) && (strlen(VALIDITY_PERIOD) < MAX_DATE_LEN - 1)), PPERR_SMS_VALIDPERIODERRLEN);
	THROW_PP((strlen(SCHEDULE_DELIVERY_TIME) < MAX_DATE_LEN) && !((strlen(SCHEDULE_DELIVERY_TIME) > 0) && (strlen(SCHEDULE_DELIVERY_TIME) < MAX_DATE_LEN - 1)), PPERR_SMS_DELIVTIMEERRLEN);
	THROW_PP(AddressRange.Len() < MAX_ADDR_RANGE_LEN, PPERR_SMS_ADDRRANGEERRLEN);
	(Host = 0).Cat(pHost);
	Port = port;
	(SystemId = 0).Cat(pSystemId);
	(Login = 0).Cat(pLogin);
	(Password = 0).Cat(pPassword);
	(SystemType = 0).Cat(pSystemType);
	SourceAddressTon = sourceAddressTon;
	SourceAddressNpi = sourceAddressNpi;
	DestAddressTon = destAddressTon;
	DestAddressNpi = destAddressNpi;
	DataCoding = dataCoding;
	// @vmiller comment
	//if(isempty(pFrom)) {
	//	GetMainOrgName(From.Trim(MAX_ADDR_LEN - 1));
	//}
	//else
		(From = 0).Cat(pFrom);
	if(!PPObjSmsAccount::VerifyString(From, 0)) {
		From = "";
		THROW_PP(0, PPERR_SMS_FROMMUSTBELATIN);
	}
	SplitLongMsg = splitLongMsg;
	CATCHZOK;
	return ok;
}
//
//
//
StSMResults::StSMResults()
{
	BindResult = NO_STATUS;
	UnbindResult = NO_STATUS;
	SubmitResult = NO_STATUS;
	DataResult = NO_STATUS;
	EnquireLinkResult = NO_STATUS;
	GenericNackResult = NO_STATUS;
};

int StSMResults::GetResult(int kindOfResult)
{
	int    result = NO_STATUS;
	if(kindOfResult == NO_STATUS) {
		result = GenericNackResult;
		GenericNackResult = NO_STATUS;
	}
	else {
		result = kindOfResult;
		kindOfResult = NO_STATUS;
	}
	return result;
}
//
//
//static
int SLAPI PPObjSmsAccount::VerifyString(const SString & rStr, int options)
{
	int    ok = 1;
	for(size_t i = 0; ok && i < rStr.Len(); i++) {
		const char c = rStr.C(i);
		if(!((c >= ' ' && c <= '~') || c == 0xC || c == 0xD))
			ok = 0;
	}
	return ok;
}

SLAPI PPObjSmsAccount::PPObjSmsAccount(void * extraPtr) : PPObjReference(PPOBJ_SMSPRVACCOUNT, extraPtr)
{
}

int SLAPI PPObjSmsAccount::Browse(void * extraPtr)
{
	int    ok = -1;
	ObjViewDialog * p_dlg = 0;
	if(CheckRights(PPR_READ) && CheckDialogPtr(&(p_dlg = new ObjViewDialog(DLG_OBJVIEW, this, extraPtr)))) {
		ExecViewAndDestroy(p_dlg);
		ok = 1;
	}
	else
		ok = PPErrorZ();
	return ok;
}

SLAPI PPSmsAccount::PPSmsAccount()
{
	THISZERO();
}

int FASTCALL PPSmsAccount::IsEqual(const PPSmsAccount & rS) const
{
    int    yes = 1;
	if(Flags != rS.Flags)
		yes = 0;
	else if(ServerPort != rS.ServerPort)
		yes = 0;
	else if(PersonID != rS.PersonID)
		yes = 0;
	else if(ResponseTimeout != rS.ResponseTimeout)
		yes = 0;
	else if(ResendMsgQueueTimeout != rS.ResendMsgQueueTimeout)
		yes = 0;
	else if(ResendTriesNum != rS.ResendTriesNum)
		yes = 0;
	else if(ReconnectTimeout != rS.ReconnectTimeout)
		yes = 0;
	else if(ReconnectTriesNum != rS.ReconnectTriesNum)
		yes = 0;
    else if(strcmp(Name, rS.Name) != 0)
		yes = 0;
	else if(strcmp(Symb, rS.Symb) != 0)
		yes = 0;
    return yes;
}

SLAPI PPSmsAccPacket::PPSmsAccPacket()
{
}

void SLAPI PPSmsAccPacket::Init()
{
	ExtStr = 0;
	MEMSZERO(Rec);
}

int FASTCALL PPSmsAccPacket::IsEqual(const PPSmsAccPacket & rS) const
{
	int    yes = 1;
	if(!Rec.IsEqual(rS.Rec))
		yes = 0;
	else if(PPCmpExtStrData(SMEXTSTR_HOST, ExtStr, rS.ExtStr, 0) != 0)
		yes = 0;
	else if(PPCmpExtStrData(SMEXTSTR_SYSTEMID, ExtStr, rS.ExtStr, 0) != 0)
		yes = 0;
	else if(PPCmpExtStrData(SMEXTSTR_FROM, ExtStr, rS.ExtStr, 0) != 0)
		yes = 0;
	else if(PPCmpExtStrData(SMEXTSTR_PASSWORD, ExtStr, rS.ExtStr, 0) != 0)
		yes = 0;
	else if(PPCmpExtStrData(SMEXTSTR_SYSTYPE, ExtStr, rS.ExtStr, 0) != 0)
		yes = 0;
	return yes;
}

int SLAPI PPSmsAccPacket::Verify(long flags) const
{
	int    ok = 1;
	SString temp_buf;
	temp_buf = Rec.Name;
	THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);

	PPGetExtStrData(SMEXTSTR_HOST, ExtStr, temp_buf);
	if(!(flags & vfEditingOnly))
		THROW_PP(temp_buf.NotEmptyS(), PPERR_SMS_HOSTNEEDED);

	PPGetExtStrData(SMEXTSTR_SYSTEMID, ExtStr, temp_buf);
	if(!(flags & vfEditingOnly))
		THROW_PP(temp_buf.NotEmptyS(), PPERR_SMS_SYSTEMIDNEEDED);
	THROW_PP(temp_buf.Len() < MAX_SYSTEM_ID_LEN, PPERR_SMS_SYSTEMIDERRLEN);

	GetPassword(temp_buf);
	if(!(flags & vfEditingOnly))
		THROW_PP(temp_buf.NotEmpty(), PPERR_SMS_PASSWNEEDED);
	THROW_PP(temp_buf.Len() < MAX_PASSWORD_LEN, PPERR_SMS_PASSWERRLEN);
	if(!(flags & vfEditingOnly))
		THROW_PP(Rec.ServerPort > 0, PPERR_SMS_INVPORT);
	PPGetExtStrData(SMEXTSTR_SYSTYPE, ExtStr, temp_buf);
	THROW_PP(temp_buf.Len() < MAX_SYSTEM_TYPE_LEN, PPERR_SMS_SYSTYPEERRLEN);

	PPGetExtStrData(SMEXTSTR_FROM, ExtStr, temp_buf);
	THROW_PP(PPObjSmsAccount::VerifyString(temp_buf.Strip(), 0), PPERR_SMS_FROMMUSTBELATIN);
	CATCHZOK
	return ok;
}

int SLAPI PPSmsAccPacket::SetPassword(const char * pPassword)
{
	/*
	const  size_t temp_buf_len = 128;
	char   temp_pw[128], temp_buf[512];
	STRNSCPY(temp_pw, pPassword);
	size_t pw_len = strlen(temp_pw);
	IdeaEncrypt(0, temp_pw, MAX_PASSWORD_LEN);
	size_t p = 0;
	for(size_t i = 0; i < MAX_PASSWORD_LEN; i++) {
		sprintf(temp_buf+p, "%03u", (uint8)temp_pw[i]);
		p += 3;
	}
	temp_buf[p] = 0;
	*/
	SString temp_buf;
	Reference::Helper_EncodeOtherPw(0, pPassword, MAX_PASSWORD_LEN, temp_buf);
	return PPPutExtStrData(SMEXTSTR_PASSWORD, ExtStr, temp_buf);
}

int SLAPI PPSmsAccPacket::GetPassword(SString & rBuf) const
{
	rBuf = 0;
	int    ok = 1;
	SString temp_buf;
	PPGetExtStrData(SMEXTSTR_PASSWORD, ExtStr, temp_buf);
	Reference::Helper_DecodeOtherPw(0, temp_buf, MAX_PASSWORD_LEN, rBuf);
	/*
	char   temp_pw[128];
	memzero(temp_pw, sizeof(temp_pw));
	if(temp_buf.Len() == (MAX_PASSWORD_LEN*3)) {
		for(size_t i = 0, p = 0; i < MAX_PASSWORD_LEN; i++) {
			char   nmb[16];
			nmb[0] = temp_buf.C(p);
			nmb[1] = temp_buf.C(p+1);
			nmb[2] = temp_buf.C(p+2);
			nmb[3] = 0;
			temp_pw[i] = atoi(nmb);
			p += 3;
		}
		IdeaDecrypt(0, temp_pw, MAX_PASSWORD_LEN);
		rBuf = temp_pw;
		IdeaRandMem(temp_pw, sizeof(temp_pw));
	}
	else
		ok = 0;
	*/
	return ok;
}

PPAutoSmsConfig::PPAutoSmsConfig()
{
	MEMSZERO(Reserve);
	TddoPath = 0;
	AllowedWeekDays = 0;
	Reserve2 = 0;
	AllowedStartTm = ZEROTIME;
	AllowedEndTm = ZEROTIME;
}

int PPAutoSmsConfig::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		THROW_SL(rBuf.Write(TddoPath, TddoPath.Len() + 1))
		THROW_SL(pCtx->Serialize(dir, AllowedWeekDays, rBuf));
		THROW_SL(pCtx->Serialize(dir, AllowedStartTm, rBuf));
		THROW_SL(pCtx->Serialize(dir, AllowedEndTm, rBuf));
	}
	else if(dir < 0) {
		TddoPath.CopyFrom((const char *)rBuf.GetBuf());
		rBuf.SetRdOffs((size_t)(TddoPath.Len() + 1));
		THROW_SL(pCtx->Serialize(dir, AllowedWeekDays, rBuf));
		THROW_SL(pCtx->Serialize(dir, AllowedStartTm, rBuf));
		THROW_SL(pCtx->Serialize(dir, AllowedEndTm, rBuf));
	}
	CATCHZOK
	return ok;
}

SLAPI PPSendSms::PPSendSms()
{
	memzero(this, offsetof(PPSendSms, ExtStr)-0);
}

void SLAPI PPSendSms::Init()
{
	ExtStr = 0;
	memzero(this, offsetof(PPSendSms, ExtStr)-0);
}

int PPSendSms::NotEmpty()
{
	int    ok = 1;
	SString msg;
	PPGetExtStrData(SENDSMEXTSTR_TEXT, ExtStr, msg);
	THROW(msg.NotEmpty());
	CATCHZOK
	return ok;
}
//
//
//
class SendSmsDialog : public TDialog {
public:
	// @v8.5.4 static void FormatText(SString & rSrcMsg, SString & rDestMsg, PPID PersoneId);

	// Инициирует рассылку смс через диалог (AutoSms = 0)
	SendSmsDialog(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr);
	// Инициирует рассылку смс через tddo (то есть автоматическая рассылка, AutoSms = 1)
	SendSmsDialog(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr, PPID objTypeId, StrAssocArray & rObjIdArr);
	int    getDTS(PPSendSms * pData);
	int    GetSmsCount(const char * pMsg);
	int    AutoSmsSending();
	int    CheckSchedule();
protected:
	DECL_HANDLE_EVENT;

	PPSendSms Data;
	PPID AccID;
	PPID ObjTypeId;
	StrAssocArray PrsnIdArr;
	StrAssocArray PhoneArr;
	StrAssocArray ObjIdArr;
	uint SplitMsg;
	uint AutoSms;
private:
	// @v8.5.4 static void SubstVar(SString & src, SString & rDest, PPID personeId);
	// void   GetStrVar(size_t pos, SString & rVar);
	int    DrawList();
	int    SendSmsText();
	int    GetAutoSmsText(PPID prsn_id, PPID objId, SString & rText);
};

SendSmsDialog::SendSmsDialog(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr) : TDialog(DLG_SENDSMS)
{
	AccID = accID;
	SplitMsg = 0;
	PrsnIdArr.Copy(rPrsnIdArr);
	PhoneArr.Copy(rPhoneArr);
	AutoSms = 0;
	ObjTypeId = 0;
	PPSmsAccPacket acc_pack;
	PPObjSmsAccount mobj;
	if(mobj.GetPacket(accID, &acc_pack))
		if(acc_pack.Rec.Flags & PPSmsAccount::smacfSpliLongMsg)
			SplitMsg = 1;

	if(!SetupStrListBox(this, CTL_SENDSMS_INSERT))
		PPError();
	DrawList();
}

SendSmsDialog::SendSmsDialog(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr, PPID objTypeId, StrAssocArray & rObjIdArr) : TDialog(/*DLG_SENDSMS*/0)
{
	AccID = accID;
	SplitMsg = 0;
	PrsnIdArr.Copy(rPrsnIdArr);
	PhoneArr.Copy(rPhoneArr);
	AutoSms = 1;
	ObjTypeId = objTypeId;
	ObjIdArr.Copy(rObjIdArr);
	PPSmsAccPacket acc_pack;
	PPObjSmsAccount mobj;
	if(mobj.GetPacket(accID, &acc_pack))
		if(acc_pack.Rec.Flags & PPSmsAccount::smacfSpliLongMsg)
			SplitMsg = 1;
}

IMPL_HANDLE_EVENT(SendSmsDialog)
{
	TDialog::handleEvent(event);
	if(event.isCtlEvent(CTL_SENDSMS_INSERT) && event.isCmd(cmLBDblClk)) {
		SmartListBox * p_list = (SmartListBox *)getCtrlView(CTL_SENDSMS_INSERT);
		if(p_list) {
			SString var, text;
			long   c = 0;
			p_list->getCurID(&c);
			if(PPSmsSender::GetSubstVar(c, var)) {
				getCtrlString(CTL_SENDSMS_TEXT, text);
				text.Cat(var);
				setCtrlString(CTL_SENDSMS_TEXT, text);
			}
		}
	}
	if(event.isCtlEvent(CTL_SENDSMS_TEXT)) {
		PPSendSms send_sms;
		SString src_msg, dest_msg, prsn_id_str;
		long   msg_len = 0, sms_count = 0;
		char   temp_buf[256];
		getCtrlData(CTL_SENDSMS_TEXT, &temp_buf);
		if(*strip(temp_buf))
			msg_len = strlen(temp_buf);
		sms_count = GetSmsCount(temp_buf);
		setCtrlLong(CTL_SENDSMS_SYMBCOUNT, msg_len);
		setCtrlLong(CTL_SENDSMS_SMSCOUNT,  sms_count);
		getDTS(&send_sms);
		(src_msg = 0).Cat(send_sms.ExtStr.Excise(0, 3));
		PrsnIdArr.Get(0, prsn_id_str);
		{
			// @v8.5.4 {
			PPSmsSender::FormatMessageBlock fmb;
			fmb.PersonID = prsn_id_str.ToLong();
			PPSmsSender::FormatMessage(src_msg, dest_msg, &fmb);
			// } @v8.5.4
			// @v8.5.4 FormatText(src_msg, dest_msg = 0, prsn_id);
		}
		setCtrlString(CTL_SENDSMS_PREVTEXT, dest_msg);
		msg_len = dest_msg.Len();
		sms_count = GetSmsCount(dest_msg);
		setCtrlData(CTL_SENDSMS_SYMBCOUNT2, &msg_len);
		setCtrlData(CTL_SENDSMS_SMSCOUNT2, &sms_count);
	}
	if(event.isCmd(cmSendSms)) {
		SendSmsText();
	}
	clearEvent(event);
}

int SendSmsDialog::getDTS(PPSendSms * pData)
{
	int    ok = 1;
	uint   selctl = 0;
	SString temp_buf;
	getCtrlString(CTL_SENDSMS_TEXT, temp_buf);
	PPPutExtStrData(SENDSMEXTSTR_TEXT, Data.ExtStr, temp_buf);
	ASSIGN_PTR(pData, Data);
	return 1;
}
//int SendSmsDialog::setDTS(PPSendSms * pData)
//{
//	int    ok = 1;
//	uint   selctl = 0;
//	if(pData)
//		Data = *pData;
//	SetExtStrData(this, &Data, CTL_SENDSMS_TEXT, SENDSMEXTSTR_TEXT);
//	return 1;
//}

int SendSmsDialog::GetSmsCount(const char * pMsg)
{
	size_t max_msg_len = 0, msg_len = 0;
	SString msg_str(pMsg);

	// Если длинное сообщение разбиваем на короткие, то длина каждого сообщения для латиницы 160 символов, для криллицы - 70.
	if(SplitMsg) {
		if(PPObjSmsAccount::VerifyString(msg_str, 0))
			max_msg_len = MAX_MESSAGE_7BIT_LEN;
		else
			max_msg_len = MAX_MESSAGE_UCS2_LEN;
	}
	// Если длинное сообщение отправляем одним запросом, то длина каждой его части для латиницы 153 символа, для кириллицы - 67.
	else {
		if(!Data.SmsCount) {
			if(PPObjSmsAccount::VerifyString(msg_str, 0))
				max_msg_len = MAX_MESSAGE_7BIT_LEN;
			else
				max_msg_len = MAX_MESSAGE_UCS2_LEN;
		}
		else {
			if(PPObjSmsAccount::VerifyString(msg_str, 0))
				max_msg_len = MAX_MESSAGE_7BIT_NOTSPLIT_LEN;
			else
				max_msg_len = MAX_MESSAGE_UCS2_NOTSPLIT_LEN;
		}
	}
	msg_len = msg_str.Len();
	Data.SmsCount = (int)(msg_len / max_msg_len);
	// Если остаток от деления не 0, то дополнительная смс
	if((msg_len % max_msg_len) != 0)
		Data.SmsCount++;
	return Data.SmsCount;
}

int SendSmsDialog::DrawList()
{
	int    ok = 1;
	SString text, params;
	SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_SENDSMS_INSERT);
	if(p_list) {
		THROW(PPLoadText(PPTXT_SMS_FIELDS, params));
		for(int i = SMS_FIELDS_PERSNAME; i <= SMS_FIELDS_MAINORGADDR; i++) {
			PPGetSubStr(params, i, text);
			THROW_SL(p_list->addItem(i, text));
		}
	}
	p_list->draw();
	CATCHZOK;
	return ok;
}

int SendSmsDialog::AutoSmsSending()
{
	return(SendSmsText());
}

int SendSmsDialog::GetAutoSmsText(PPID prsn_id, PPID objId, SString & rText)
{
	int    ok = 1;
	Tddo   t;
	SString temp_buf, buf;
	SBuffer text;
	StringSet param_list;
	switch(ObjTypeId) {
		case PPOBJ_TSESSION:
			{
				// Прочитаем конфигурацию для автоматической рассылки
				PPObjTSession tsess_obj;
				PPObjGoods goods_obj;
				PPTSessConfig tsess_cfg;
				TSessionTbl::Rec tsess_rec;
				Goods2Tbl::Rec goods_rec;
				THROW(tsess_obj.ReadConfig(&tsess_cfg));
				THROW(Tddo::LoadFile(tsess_cfg.SmsConfig.TddoPath, temp_buf));
				t.SetInputFileName(tsess_cfg.SmsConfig.TddoPath);
				// Заполняем список дополнительных параметров
				param_list.setDelim(",");
				if(tsess_obj.Search(objId, &tsess_rec) > 0) {
					TechTbl::Rec tech_rec;
					tsess_obj.GetTech(tsess_rec.TechID, &tech_rec);
					goods_obj.Search(tech_rec.GoodsID, &goods_rec);
					(buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
					param_list.add(buf); // ${1} Имя товара в сессии (технологии)
					(buf = 0).Cat(tsess_rec.StDt);
					param_list.add(buf); // ${2}
					(buf = 0).Cat(tsess_rec.StTm);
					param_list.add(buf); // ${3}
				}
				{
					DlRtm::ExportParam ep;
					PPFilt _pf;
					_pf.ID = prsn_id;
					_pf.Ptr = 0;
					ep.P_F = &_pf;
					THROW(t.Process("Person", temp_buf, /*prsn_id, 0*/ep, &param_list, text));
					(rText = 0).CopyFromN((const char *)text.GetBuf(), text.GetAvailableSize()).ToOem();
				}
			}
			break;
		default:
			PPSetError(PPERR_OBJNFOUND);
			ok = 0;
			break;
	}
	CATCHZOK;
	return ok;
}

int SendSmsDialog::SendSmsText()
{
	int    ok = 1;
	SmsClient client;
	PPSendSms send_sms;
	PPPersonPacket pack;
	PPObjPerson person_obj;
	SString src_msg, dest_msg, phone, prsn_id_str, obj_id_str, str;
	size_t max_sms_len = 0;
	PPID prsn_id = 0, obj_id = 0;
	//PPLogger logger;

	getDTS(&send_sms);
	(src_msg = 0).Cat(send_sms.ExtStr.Excise(0, 3));
	if(src_msg.NotEmpty() || AutoSms) {
		GetSmsCount(src_msg);
		THROW(SmsInit(client, AccID, &client.Logger));
		PPLoadText(PPTXT_SMS_SENDINGSMS, str);
		client.Logger.Log(str);
		for(size_t i = 0; i < PhoneArr.getCount(); i++) {
			PrsnIdArr.Get(i, prsn_id_str);
			prsn_id = prsn_id_str.ToLong();
			PhoneArr.Get(i, phone);
			// Если через tddo
			if(AutoSms) {
				ObjIdArr.Get(i, obj_id_str);
				obj_id = obj_id_str.ToLong();
				THROW(GetAutoSmsText(prsn_id, obj_id, dest_msg));
				SendingSms(client, prsn_id, phone, dest_msg, &client.Logger);
			}
			else { // Иначе если интерактивно
				// @v8.5.4 {
                PPSmsSender::FormatMessageBlock fmb;
				fmb.PersonID = prsn_id;
				PPSmsSender::FormatMessage(src_msg, dest_msg, &fmb);
				// } @v8.5.4
				// @v8.5.4 FormatText(src_msg, dest_msg = 0, prsn_id);
				/*if(!SendingSms(client, prsn_id, phone, dest_msg, &logger))
					PPError();*/
				SendingSms(client, prsn_id, phone, dest_msg, &client.Logger);
			}
		}
		THROW(SmsRelease(client, &client.Logger));
	}
	CATCHZOKPPERR;
	//logger.Save("SmsLog.log");
	return ok;
}

int SLAPI PPObjSmsAccount::Edit(PPID * pID, void * extraPtr)
{
	class SmsAcctDialog : public TDialog {
	public:
		SmsAcctDialog() : TDialog(DLG_SMSACC)
		{
		}
		int    setDTS(const PPSmsAccPacket * pData)
		{
			if(pData)
				Data = *pData;
			setCtrlData(CTL_SMSACC_NAME, Data.Rec.Name);
			setCtrlData(CTL_SMSACC_SYMB, Data.Rec.Symb);
			setCtrlLong(CTL_SMSACC_ID, Data.Rec.ID);
			disableCtrl(CTL_SMSACC_ID, 1);
			SetExtStrData(&Data, CTL_SMSACC_HOST, SMEXTSTR_HOST);
			setCtrlLong(CTL_SMSACC_PORT, Data.Rec.ServerPort);
			SetExtStrData(&Data, CTL_SMSACC_SYSTEMID, SMEXTSTR_SYSTEMID);
			SetExtStrData(&Data, CTL_SMSACC_FROM,  SMEXTSTR_FROM);
			SetExtStrData(&Data, CTL_SMSACC_SYSTYPE,  SMEXTSTR_SYSTYPE);
			{
				SString temp_buf;
				Data.GetPassword(temp_buf);
				setCtrlString(CTL_SMSACC_PASSW, temp_buf);
				IdeaRandMem((char *)(const char *)temp_buf, temp_buf.Len());
			}
			AddClusterAssoc(CTL_SMSACC_FLAGS, 0, PPSmsAccount::smacfUseUHTT);
			AddClusterAssoc(CTL_SMSACC_FLAGS, 1, PPSmsAccount::smacfSpliLongMsg);
			SetClusterData(CTL_SMSACC_FLAGS, Data.Rec.Flags);
			return 1;
		}
		int    getDTS(PPSmsAccPacket * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			getCtrlData(sel = CTL_SMSACC_NAME, Data.Rec.Name);
			THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
			getCtrlData(CTL_SMSACC_SYMB, Data.Rec.Symb);
			GetClusterData(CTL_SMSACC_FLAGS, &Data.Rec.Flags);
			GetExtStrData(&Data, CTL_SMSACC_HOST, SMEXTSTR_HOST);
			getCtrlData(CTL_SMSACC_PORT, &Data.Rec.ServerPort);
			GetExtStrData(&Data, CTL_SMSACC_SYSTEMID, SMEXTSTR_SYSTEMID);
			GetExtStrData(&Data, CTL_SMSACC_FROM,   SMEXTSTR_FROM);
			GetExtStrData(&Data, CTL_SMSACC_SYSTYPE, SMEXTSTR_SYSTYPE);
			{
				getCtrlString(CTL_SMSACC_PASSW, temp_buf);
				Data.SetPassword(temp_buf);
				IdeaRandMem((char *)(const char *)temp_buf, temp_buf.Len());
			}
			sel = 0;
			THROW(Data.Verify(Data.vfEditingOnly));
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel, -1);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmSmsSys)) {
				editSysData();
			}
			else
				return;
			clearEvent(event);
		}
		int    editSysData()
		{
			int    ok = -1;
			TDialog * dlg = new TDialog(DLG_SMSSYS);
			if(CheckDialogPtr(&dlg, 1)) {
				dlg->setCtrlData(CTL_SMSSYS_RESPTIMEOUT, &Data.Rec.ResponseTimeout);
				dlg->setCtrlData(CTL_SMSSYS_RSNDTIMEOUT, &Data.Rec.ResendMsgQueueTimeout);
				dlg->setCtrlData(CTL_SMSSYS_RSNDNUM, &Data.Rec.ResendTriesNum);
				dlg->setCtrlData(CTL_SMSSYS_RECONNTIMEOUT, &Data.Rec.ReconnectTimeout);
				dlg->setCtrlData(CTL_SMSSYS_RECONNNUM, &Data.Rec.ReconnectTriesNum);
				if(ExecView(dlg) == cmOK) {
					dlg->getCtrlData(CTL_SMSSYS_RESPTIMEOUT, &Data.Rec.ResponseTimeout);
					dlg->getCtrlData(CTL_SMSSYS_RSNDTIMEOUT, &Data.Rec.ResendMsgQueueTimeout);
					dlg->getCtrlData(CTL_SMSSYS_RSNDNUM, &Data.Rec.ResendTriesNum);
					dlg->getCtrlData(CTL_SMSSYS_RECONNTIMEOUT, &Data.Rec.ReconnectTimeout);
					dlg->getCtrlData(CTL_SMSSYS_RECONNNUM, &Data.Rec.ReconnectTriesNum);
					ok = 1;
				}
			}
			else
				ok = 0;
			delete dlg;
			return ok;
		}
		void   SetExtStrData(PPSmsAccPacket * pData, uint ctlID, uint strID)
		{
			SString temp_buf;
			PPGetExtStrData(strID, pData->ExtStr, temp_buf);
			setCtrlString(ctlID, temp_buf);
		}
		void   GetExtStrData(PPSmsAccPacket * pData, uint ctlID, uint strID)
		{
			SString temp_buf;
			getCtrlString(ctlID, temp_buf);
			PPPutExtStrData(strID, pData->ExtStr, temp_buf);
		}
		PPSmsAccPacket Data;
	};
	int    r = cmCancel, ok = 1, valid_data = 0;
	PPSmsAccPacket pack;
	SmsAcctDialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	dlg = new SmsAcctDialog();
	THROW(CheckDialogPtr(&dlg));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(*pID)
				*pID = pack.Rec.ID;
			if(!ref->CheckUniqueSymb(Obj, *pID, pack.Rec.Name, offsetof(PPSmsAccount, Name)))
				PPError();
			else {
				if(PutPacket(pID, &pack, 1))
					valid_data = 1;
				else
					PPError();
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int SLAPI PPObjSmsAccount::PutPacket(PPID * pID, PPSmsAccPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			if(pPack) {
				THROW(ref->UpdateItem(Obj, *pID, &(pPack->Rec), 1, 0));
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(ref->AddItem(Obj, pID, &(pPack->Rec), 0));
		}
		if(*pID)
			THROW(ref->PutPropVlrString(Obj, *pID, SMACCPRP_EXTRA, pPack->ExtStr));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjSmsAccount::GetPacket(PPID id, PPSmsAccPacket * pPack)
{
	int    ok = 1;
	PPSmsAccPacket pack;
	THROW(Search(id, &pack.Rec) > 0);
	THROW(PPRef->GetPropVlrString(Obj, id, SMACCPRP_EXTRA, pack.ExtStr));
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}

int GetSmsConfig(PPSmsAccPacket & rPack, StConfig & rConfig)
{
	int    ok = 1;
	uint   split_long_msg = 0, use_uhtt = 0;
	SString str, msg;
	SString host, system_id, password, from, system_type;
	PPGetExtStrData(SMEXTSTR_HOST, rPack.ExtStr, host);
	PPGetExtStrData(SMEXTSTR_SYSTEMID, rPack.ExtStr, system_id);
	rPack.GetPassword(str);
	PPGetExtStrData(SMEXTSTR_SYSTYPE, rPack.ExtStr, system_type);
	PPGetExtStrData(SMEXTSTR_FROM,    rPack.ExtStr, from);
	if(rPack.Rec.Flags & PPSmsAccount::smacfSpliLongMsg)
		split_long_msg = 1;
	if(rPack.Rec.Flags & PPSmsAccount::smacfUseUHTT)
		use_uhtt = 1;
	(password = 0).Cat(str);
	THROW(rConfig.SetConfig(host, rPack.Rec.ServerPort, system_id, "", password, system_type, TON_ALPHANUMERIC/*INTERNATIONAL*/,
		NPI_UNKNOWN/*ISDN*/, TON_INTERNATIONAL, NPI_ISDN, SMSC_DEFAULT, from, split_long_msg));

	// Если какой-то системный параметр не установлен, то ставим значение по умолчанию
	rConfig.ResponseTimeout = rPack.Rec.ResponseTimeout ? rPack.Rec.ResponseTimeout : SUBMIT_RECEIVE_TIMEOUT;
	rConfig.ResendMsgQueueTimeout = rPack.Rec.ResendMsgQueueTimeout ? rPack.Rec.ResendMsgQueueTimeout : RESEND_QUEUE_MSG_TIMEOUT;
	rConfig.ResendTriesNum = rPack.Rec.ResendTriesNum ? rPack.Rec.ResendTriesNum : RESEND_MSG_COUNT;
	rConfig.ReconnectTimeout = rPack.Rec.ReconnectTimeout ? rPack.Rec.ReconnectTimeout : RECONNECT_TIMEOUT;
	rConfig.ReconnectTriesNum = rPack.Rec.ReconnectTriesNum ? rPack.Rec.ReconnectTriesNum : RECONNECTION_COUNT;
	CATCHZOK;
	return ok;
}
//
// Descr: Преобразует номер телефона так, чтобы номер представлял собой последовательность чисел. Лишние символы удаляет.
//
int FormatPhone(const char * pPhone, SString & rPhone, SString & rErrMsg)
{
	int   ok = 1;
	SString phone(pPhone), old_phone(pPhone);
	SString str, sub_str, fmt_buf, added_buf;
	if(phone.NotEmpty()) {
		if(!phone.IsDigit()) {
			for(size_t j = 0; j < phone.Len();) {
				if((phone.C(j) < '0') || (phone.C(j) > '9'))
					phone.Excise(j, 1);
				else
					j++;
			}
		}
	}
	// Добавим код страны (7), если его нет
	if(phone.Len() == 10)
		phone.PadLeft(1, '7');
	else if((phone.Len() == 11) && (phone.C(0) != '7')){
		phone.ShiftLeft(1);
		phone.PadLeft(1, '7');
	}
	if(phone.Len() != 11) {
		str = 0;
		if(phone.CmpNC(old_phone) != 0) {
			PPLoadText(PPTXT_SMS_PNONENUMCHNGD, fmt_buf);
			(added_buf = old_phone).Cat("->").Cat(phone).CR();
			str.Printf(fmt_buf, added_buf.cptr());
		}
		else
			str.Cat(phone).Space();
		PPLoadText(PPTXT_SMS_ELEVENNUMS, sub_str);
		str.ToOem().Cat(sub_str);
		rErrMsg = str;
		//PPSetError(0, str);
		ok = 0;
	}
	rPhone = phone;
	return ok;
}

static SString & GetString(const char * pInpString, size_t maxLen, const char * pDefValue, SString & rBuf)
{
    if(isempty(pInpString))
        rBuf = pDefValue;
    else if(strlen(pInpString) + 1 > maxLen)
		rBuf.CopyFromN(pInpString, maxLen);
	else
		rBuf = pInpString;
	return rBuf;
}

StSmscErrorText SmscErrorMsgs[] = {
	{ESME_RINVMSGLEN,		"Message Length is invalid"},
	{ESME_RINVCMDLEN,		"Command Length is invalid"},
	{ESME_RINVCMDID,		"Invalid Command ID"},
	{ESME_RINVBNDSTS,		"Incorrect BIND Status for given command"},
	{ESME_RALYBND,			"ESME Already in Bound State"},
	{ESME_RINVPRTFLG,		"Invalid Priority Flag"},
	{ESME_RINVREGDLVFLG,	"Invalid Registered Delivery Flag"},
	{ESME_RSYSERR,			"System Error"},
	{ESME_RINVSRCADR,		"Invalid Source Address"},
	{ESME_RINVDSTADR,		"Invalid Dest Addr"},
	{ESME_RINVMSGID,		"Message ID is invalid"},
	{ESME_RBINDFAIL,		"Bind Failed"},
	{ESME_RINVPASWD,		"Invalid Password"},
	{ESME_RINVSYSID,		"Invalid System ID"},
	{ESME_RCANCELFAIL,		"Cancel SM Failed"},
	{ESME_RREPLACEFAIL,		"Replace SM Failed"},
	{ESME_RMSGQFULL,		"Message Queue Full"},
	{ESME_RINVSERTYP,		"Invalid Service Type"},
	{ESME_RINVNUMDESTS,		"Invalid number of destinations"},
	{ESME_RINVDLNAME,		"Invalid Distribution List name"},
	{ESME_RINVDESTFLAG,		"Destination flag is invalid(submit_multi)"},
	{ESME_RINVSUBREP,		"Invalid 'submit with replace' request(i.e. submit_sm with replace_if_present_flag set)"},
	{ESME_RINVESMCLASS,		"Invalid esm_class field data"},
	{ESME_RCNTSUBDL,		"Cannot Submit to Distribution List"},
	{ESME_RSUBMITFAIL,		"Submit_sm or Submit_multi failed"},
	{ESME_RINVSRCTON,		"Invalid Source address TON"},
	{ESME_RINVSRCNPI,		"Invalid Source address NPI"},
	{ESME_RINVDSTTON,		"Invalid Destination address TON"},
	{ESME_RINVDSTNPI,		"Invalid Destination address NPI"},
	{ESME_RINVSYSTYP,		"Invalid system_type field"},
	{ESME_RINVREPFLAG,		"Invalid replace_if_present flag"},
	{ESME_RINVNUMMSGS,		"Invalid number of messages"},
	{ESME_RTHROTTLED,		"Throttling error (ESME has exceeded allowed message limits)"},
	{ESME_RINVSCHED,		"Invalid Scheduled Delivery Time"},
	{ESME_RINVEXPIRY,		"Invalid message validity period (Expiry time)"},
	{ESME_RINVDFTMSGID, 	"Predefined Message Invalid or Not Found"},
	{ESME_RX_T_APPN,		"ESME Receiver Temporary App Error Code"},
	{ESME_RX_P_APPN,		"ESME Receiver Permanent App Error Code"},
	{ESME_RX_R_APPN,		"ESME Receiver Reject Message Error Code"},
	{ESME_RQUERYFAIL,		"Query_sm request failed"},
	{ESME_RINVOPTPARSTREAM,	"Error in the optional part of the PDU Body"},
	{ESME_ROPTPARNOTALLWD,	"Optional Parameter not allowed"},
	{ESME_RINVPARLEN,		"Invalid Parameter Length"},
	{ESME_RMISSINGOPTPARAM,	"Expected Optional Parameter missing"},
	{ESME_RINVOPTPARAMVAL,	"Invalid Optional Parameter Value"},
	{ESME_RDELIVERYFAILURE,	"Delivery Failure (used for data_sm_resp)"},
	{ESME_RUNKNOWNERR,		"Unknown Error"}
};

StStatusText StatusTexts[] = {
	{SMS_DELIVERED,			"DELIVERED"},
	{SMS_EXPIRED,			"EXPIRED"},
	{SMS_DELETED,			"DELETED"},
	{SMS_UNDELIVERABLE,		"UNDELIVERABLE"},
	{SMS_ACCEPTED,			"ACCEPTED"},
	{SMS_UNKNOWN,			"UNKNOWN"},
	{SMS_REJECTED,			"REJECTED"}
};

SString & GetSmscErrorText(int error, SString & rErrorText)
{
	for(size_t i = 0; i < SIZEOFARRAY(SmscErrorMsgs); i++) {
		if(SmscErrorMsgs[i].Id == error) {
			rErrorText = SmscErrorMsgs[i].ErrorMsg;
			break;
		}
	}
	if(rErrorText.Empty())
		rErrorText.Cat(error);
	return rErrorText;
}

SString & GetStatusText(int status, SString & rStatusText)
{
	for(size_t i = 0; i < SIZEOFARRAY(StatusTexts); i++)
		if(StatusTexts[i].Id == status) {
			rStatusText = StatusTexts[i].Status;
			break;
		}
	return rStatusText;
}

class SmsProtocolBuf : public STempBuffer {
public:
	struct Hdr { // @persistent size=16
		uint32 PduLen;		// Длина PDU
		uint32 CommandId;	// ID команды
		uint32 CommandStatus;	// Статус запроса
		uint32 SequenceNum;	// Номер запроса
	};
	SmsProtocolBuf(uint cmdId, uint cmdStatus, uint seqNum);
	SmsProtocolBuf();
	int    Init();
	int    AddUInt8(uint8 c);
	int    AddUInt16(uint16 c);
	int    AddString(const char * pStr);
	int    AddString(const char * pStr, size_t strLen);
	int    AddUInt8Array(uint8 * pBuf, size_t bufSize);
	//
	// Descr: Подсчитывает полную длину команды и заполняет поле PduLen
	//
	int    Finish();
	Hdr    Header;
};

SmsProtocolBuf::SmsProtocolBuf(uint cmdId, uint cmdStatus, uint seqNum) : STempBuffer(0)
{
	assert(sizeof(Hdr) == 16);
	Header.PduLen = 0;
	Header.CommandId = cmdId;
	Header.CommandStatus = cmdStatus;
	Header.SequenceNum = seqNum;
}

SmsProtocolBuf::SmsProtocolBuf() : STempBuffer(0)
{
	assert(sizeof(Hdr) == 16);
	SmsProtocolBuf::Init();
}

int SmsProtocolBuf::Init()
{
	Alloc(0);
	Header.PduLen = 0;
	Header.CommandId = 0;
	Header.CommandStatus = 0;
	Header.SequenceNum = 0;
	return 1;
}

int SmsProtocolBuf::AddUInt8(uint8 c)
{
	int    ok = 1;
	size_t size = GetSize();
	THROW_MEM(Alloc(size + 1));
	memcpy((char *)*this + size, &c, 1);
	CATCHZOK;
	return ok;
}

int SmsProtocolBuf::AddUInt8Array(uint8 * pBuf, size_t bufSize)
{
	int   ok = 1;
	size_t size = GetSize();
	THROW_MEM(Alloc(size + bufSize));
	memcpy((char *)*this + size, pBuf, bufSize);
	CATCHZOK;
	return ok;
}

int SmsProtocolBuf::AddUInt16(uint16 c)
{
	int    ok = 1;
	size_t size = GetSize();
	THROW_MEM(Alloc(size + 2));
	memcpy((char *)*this + size, &c, 2);
	CATCHZOK;
	return ok;
}

int SmsProtocolBuf::AddString(const char * pStr)
{
	int    ok = 1;
	size_t size = GetSize();
	THROW_MEM(Alloc(size + strlen(pStr) + 1));
	memcpy((char *)*this + size, pStr, strlen(pStr) + 1);
	CATCHZOK;
	return ok;
}

int SmsProtocolBuf::AddString(const char * pStr, size_t strLen)
{
	int    ok = 1;
	size_t size = GetSize();
	THROW_MEM(Alloc(size + strLen));
	memcpy((char *)*this + size, pStr, strLen);
	CATCHZOK;
	return ok;
}

int SmsProtocolBuf::Finish()
{
	Header.PduLen = _byteswap_ulong(GetSize() + sizeof(Hdr));
	return 1;
}

SmsClient::StSubmitSMParam::StSubmitSMParam()
{
	Clear();
}

void SmsClient::StSubmitSMParam::Clear()
{
	SourceAddressTon = TON_UNKNOWN;
	SourceAddressNpi = NPI_UNKNOWN;
	SourceAddress = "";
	DestinationAddressTon = TON_UNKNOWN;
	DestinationAddressNpi = NPI_UNKNOWN;
	DestinationAddress = "";
	EsmClass = 3; // Store and Forward
	ProtocolId = 0;
	PriorityFlag = BULK;
	SheduleDeliveryTime = 0;
	ValidityPeriod = 0;
	ReplaceIfPresentFlag = 0;
	DataCoding = SMSC_DEFAULT;
	SmDefaultMsgId = 0;
}

SmsClient::SmsClient()
{
    ClientSocket = INVALID_SOCKET;
	StartTime = getcurdatetime_();
	ConnectionState = SMPP_SOCKET_DISCONNECTED;
	ResendErrLenMsg = 0;
	SequenceNumber = 0;
	ReSendQueueMsgTryNums = 0;
	MessageCount = 0;
	AddStatusCodeNum = 0;
	AddErrorSubmitNum = 0;
	UndeliverableMessages = 0;
	// @v8.5.4 RecvTimeout = WAIT_PACKET_RESPONSE;
}

SmsClient::~SmsClient()
{
	Logger.Save("SmsLog.log", 0);
}

int SmsClient::Disconnect()
{
	int    ok = 1;
	delay(500);
    /*THROW(*/Unbind()/*)*/; // @todo разобраться с ошибками в Unbind()
    DisconnectSocket();
	/*
	CATCHZOK;
	*/
	return ok;
}

int SmsClient::ReceiveToTimer()
{
	int    ok = 1;
	long   diff = 0;
	if(USE_ENQUIRELINK) {
		LDATETIME cur_dtm = getcurdatetime_();
		if(diffdatetime(cur_dtm, StartTime, 4, &diff) >= ENQUIRE_LINK_TIMEOUT) {
			THROW(SendEnquireLink(SequenceNumber++));
			StartTime = getcurdatetime_();
		}
	}
	CATCHZOK;
	return ok;
}

void SmsClient::AddErrorSubmit(const char * pDestNum, int errCode)
{
	SString err_code_text, err_msg ;
	GetSmscErrorText(errCode, err_code_text);
	(err_msg = 0).Cat(pDestNum).Semicol().Cat(err_code_text);
	ErrorSubmitArr.Add(AddErrorSubmitNum++, err_msg, 1);
}

int SmsClient::Receive(uint timeout)
{
	int    ok = 1, r = 0;
	size_t buf_recvd = 0;
	SmsProtocolBuf protocol_buf;
	if((r = ClientSocket.Select(TcpSocket::mRead, timeout, &buf_recvd)) == 1) { // @v8.5.4 while-->if
		protocol_buf.Init();
		THROW_SL(ClientSocket.Recv(&protocol_buf.Header, 16, &buf_recvd));
		if(buf_recvd > 0) {
			protocol_buf.Header.PduLen = _byteswap_ulong(protocol_buf.Header.PduLen);
			protocol_buf.Header.CommandId = _byteswap_ulong(protocol_buf.Header.CommandId);
			protocol_buf.Header.CommandStatus = _byteswap_ulong(protocol_buf.Header.CommandStatus);
			protocol_buf.Header.SequenceNum = _byteswap_ulong(protocol_buf.Header.SequenceNum);
			THROW(protocol_buf.Header.PduLen > 0);
			THROW(protocol_buf.Header.CommandId > 0);
			if(protocol_buf.Header.PduLen >= 16) {
				if(protocol_buf.Header.PduLen > 16) {
					protocol_buf.Alloc(protocol_buf.Header.PduLen - 16);
					THROW(ClientSocket.Select(TcpSocket::mRead, timeout, &buf_recvd) > 0);
					THROW_SL(ClientSocket.Recv((char *)protocol_buf, protocol_buf.Header.PduLen - 16, &buf_recvd));
				}
				// Смотрим, на какую команду пришел данный ответ
				switch(protocol_buf.Header.CommandId) {
					case BIND_TRANSCEIVER_RESP:
						SMResults.BindResult = protocol_buf.Header.CommandStatus;
						if(ConnectionState == SMPP_BIND_SENT) {
							if(protocol_buf.Header.CommandStatus == 0) {
								ConnectionState = SMPP_BINDED;
								StartTime = getcurtime_();
							}
						}
						break;
					case SUBMIT_SM_RESP:
						SMResults.SubmitResult = protocol_buf.Header.CommandStatus;
						break;
					case ENQUIRE_LINK_RESP:
						SMResults.EnquireLinkResult = protocol_buf.Header.CommandStatus;
						StartTime = getcurtime_();
						break;
					case ENQUIRE_LINK:
						StartTime = getcurtime_();
						THROW(SendEnquireLinkResp(protocol_buf.Header.SequenceNum));
						break;
					case UNBIND_RESP:
						SMResults.UnbindResult = protocol_buf.Header.CommandStatus;
						ConnectionState = SMPP_UNBINDED;
						break;
					case DELIVER_SM:
						DecodeDeliverSm(protocol_buf.Header.SequenceNum, (char *)protocol_buf, protocol_buf.Header.PduLen - 16);
						THROW(SendDeliverSmResp(protocol_buf.Header.SequenceNum));
						break;
					case GENERIC_NACK:
						SMResults.GenericNackResult = protocol_buf.Header.CommandStatus;
						break;
					default: // Если command_id незвестен
						THROW(SendGenericNack(protocol_buf.Header.SequenceNum, ESME_RINVCMDID));
						break;
				}
			}
			else {
				// Отправляем на СМСЦ сообщение о неверной длине ответа
				THROW(SendGenericNack(protocol_buf.Header.SequenceNum, ESME_RINVCMDLEN));
			}
		}
	}
	THROW(r != -1);
	CATCHZOK;
	return ok;
}

int SmsClient::Send(const void * data, size_t bufLen)
{
	int    ok = 1;
	size_t buf_sent = 0;
	THROW_SL(ClientSocket.Send((char *)data, bufLen, &buf_sent));
	CATCHZOK;
	return ok;
}

int SmsClient::ConnectToSMSC()
{
	const  int init_timeout_ms = 0;
	const  int retry_delay_ms  = 500;
	const  int bind_tries = 1;

	int    ok = 0;
	InetAddr inet_addr;
	DisconnectSocket();
	inet_addr.Set(Config.Host, Config.Port);
	THROW_SL(ClientSocket.Connect(inet_addr) > 0);
	ConnectionState = SMPP_SOCKET_CONNECTED;
	delay(init_timeout_ms);
	for(int i = 0; i < bind_tries && !ok; i++) {
		delay(retry_delay_ms);
		ok = Bind();
	}
	CATCHZOK;
	return ok;
}

int SmsClient::Send(const SmsProtocolBuf & rBuf, int tryReconnect)
{
	int    ok = 0;
	uint   rti = 0; // Номер итерации реконнекта
	const  uint rtn = tryReconnect ? Config.ReconnectTriesNum : 0;
	do {
		if(Send(&rBuf.Header, 16) && Send((const char *)rBuf, rBuf.GetSize()))
			ok = 1;
		else if(rti < rtn)
			TryToReconnect(rti);
		else
			break;
	} while(!ok);
	return ok;
}

int SmsClient::Bind()
{
	int    ok = 1, cmd_status = NO_STATUS;
	SString err_msg;
	struct BindPduBody {
		SString SystemId;     // ID запрашивающей системы (ID клиента SMPP)
		SString Password;     // Пароль
		SString SystemType;   // Тип запрашивающей системы
		uchar  InterfaceVer;  // Версия протокола SMPP
		uchar  AddrTon;       // Тип номера отправителя //
		uchar  AddrNpi;       // Схема присвоения номеров для отправителя //
		SString AddressRange; // Формат адреса отправителя //
		uint8  Reserve;       // @alignment
	};
	BindPduBody bind_pdu_body;
	SmsProtocolBuf protocol_buf(_byteswap_ulong(BIND_TRANSCEIVER), 0, _byteswap_ulong(SequenceNumber++));
	GetString(Config.SystemId, MAX_SYSTEM_ID_LEN, "", bind_pdu_body.SystemId);
	GetString(Config.Password, MAX_PASSWORD_LEN, "", bind_pdu_body.Password);
	GetString(Config.SystemType, MAX_SYSTEM_TYPE_LEN, "", bind_pdu_body.SystemType);
	bind_pdu_body.InterfaceVer = INTERFACE_VERSION;
    bind_pdu_body.AddrTon = (uchar)Config.SourceAddressTon;
    bind_pdu_body.AddrNpi = (uchar)Config.SourceAddressNpi;
	GetString(Config.AddressRange, MAX_ADDR_RANGE_LEN, "", bind_pdu_body.AddressRange);
	protocol_buf.AddString(bind_pdu_body.SystemId);
	protocol_buf.AddString(bind_pdu_body.Password);
	protocol_buf.AddString(bind_pdu_body.SystemType);
	protocol_buf.AddUInt8(bind_pdu_body.InterfaceVer);
	protocol_buf.AddUInt8(bind_pdu_body.AddrTon);
	protocol_buf.AddUInt8(bind_pdu_body.AddrNpi);
	protocol_buf.AddString(bind_pdu_body.AddressRange);
	protocol_buf.Finish();
	THROW(Send(protocol_buf, 0));
	ConnectionState = SMPP_BIND_SENT;
	THROW(Receive(BIND_RECEIVE_TIMEOUT));
	if((cmd_status = SMResults.GetResult(SMResults.BindResult)) != ESME_ROK) {
		GetSmscErrorText(cmd_status, err_msg);
		PPSetError(PPERR_SMS_BINDERROR, err_msg.ToOem());
		ok = 0;
	}
	if(ok == 1)
		ReceiveToTimer();
	CATCHZOK;
	return ok;
}

int SmsClient::TryToReconnect(uint & rReconnectionTryNums)
{
	int    ok = 1;
	SString msg_buf;
	PPLoadText(PPTXT_SMSSRVRECONNECTION, msg_buf);
	Logger.Log(msg_buf);
    DisconnectSocket();
	delay(Config.ReconnectTimeout);
	rReconnectionTryNums++;
	return ConnectToSMSC();
}

int SmsClient::Unbind()
{
	int    ok = 1, r = 0;
	SString err_msg;
    if(ConnectionState == SMPP_BINDED) {
		SmsProtocolBuf protocol_buf(_byteswap_ulong(UNBIND), 0, _byteswap_ulong(SequenceNumber++));
		protocol_buf.Finish();
		THROW(Send(protocol_buf, 1));
		r = Receive(BIND_RECEIVE_TIMEOUT);
		if(!r) {
			uint   reconnection_try_nums = 0;
			while(!(r = TryToReconnect(reconnection_try_nums)) && reconnection_try_nums < Config.ReconnectTriesNum)
				continue;
			THROW(r);
			r = Receive(BIND_RECEIVE_TIMEOUT);
		}
		THROW(r);
		{
			int    cmd_status = SMResults.GetResult(SMResults.UnbindResult);
			if(cmd_status != ESME_ROK) {
				GetSmscErrorText(cmd_status, err_msg);
				PPSetError(PPERR_SMS_UNBINDERROR, err_msg.ToOem());
				ok = 0;
			}
		}
    }
	CATCHZOK;
	return ok;
}

void SmsClient::DecodeDeliverSm(int sequenceNumber, void * pPduBody, size_t bodyLength)
{
	uint sm_length = 0, pos = 0, end_pos = 0;
	SString message, status, error, dest_addr;

	message = 0;
	status = 0;
	dest_addr = 0;
	// Считываем Service_type
    while(PTR8(pPduBody)[pos] != 0)
		pos++;
	pos++;
	// Пропускаем Source_addr_ton и Source_addr_npi
	pos += 2;
	// Считываем адрес источника (это есть номер абонента, которму было отправлено сообщение)
	dest_addr.Cat((const char *)pPduBody + pos);
	pos += dest_addr.Len() + 1;
	// Пропускаем dest_addr_ton и dest_addr_npi
	pos += 2;
	// Считываем адрес получател
	//pos += (strlen(pduBody+pos)+1);
	while(PTR8(pPduBody)[pos] != 0)
		pos++;
	pos++;
	// Пропускаем esm_class, protocol_id, priority_flag, schedule_delivery_time, validity_period, registered_delivery, replace_if_present_flag, data_coding, sm_default_msg_id
	pos+=9;
	// Считали длину сообщени
	sm_length = PTR8(pPduBody)[pos++];
	// Считываем соообщение
	// Формат сообщения:
	// id:IIIIIIIIII sub:SSS dlvrd:DDD submit date:YYMMDDhhmm done
	// date:YYMMDDhhmm stat:DDDDDDD err:E Text:. . . . . . .. .
	message.Cat((const char *)pPduBody + pos);
	pos += message.Len() + 1;
	message.Search("stat:", 0, 1, &pos);
	pos+=5; // Пропускаем "stat:"
	for(uint i = 0; i < 7; i++, pos++) {
		status.CatChar(message[pos]);
	}
	message.Search("err:", 0, 1, &pos);
	pos += 4; // Пропускаем "err:"
	message.Search("Text:", 0, 1, &end_pos);
	error = 0;
	while (pos < (end_pos - 1)) {
		error.CatChar(message[pos++]);
	}
	if(status.CmpNC(SMS_DELIVERED_STR) == 0)
		AddStatusCode(dest_addr, SMS_DELIVERED, error);
	else if(status.CmpNC(SMS_EXPIRED_STR) == 0)
		AddStatusCode(dest_addr, SMS_EXPIRED, error);
	else if(status.CmpNC(SMS_DELETED_STR) == 0)
		AddStatusCode(dest_addr, SMS_DELETED, error);
	else if(status.CmpNC(SMS_UNDELIVERABLE_STR) == 0)
		AddStatusCode(dest_addr, SMS_UNDELIVERABLE, error);
	else if(status.CmpNC(SMS_ACCEPTED_STR) == 0)
		AddStatusCode(dest_addr, SMS_ACCEPTED, error);
	else if(status.CmpNC(SMS_UNKNOWN_STR) == 0)
		AddStatusCode(dest_addr, SMS_UNKNOWN, error);
	else if(status.CmpNC(SMS_REJECTED_STR) == 0)
		AddStatusCode(dest_addr, SMS_REJECTED, error);
}

int SmsClient::SendDeliverSmResp(int sequenceNumber)
{
	SmsProtocolBuf protocol_buf(_byteswap_ulong(DELIVER_SM_RESP), 0, _byteswap_ulong(sequenceNumber));
	protocol_buf.AddUInt8(0);
	protocol_buf.Finish();
	return Send(protocol_buf, 1);
}

void SmsClient::DisconnectSocket()
{
	if(ClientSocket != INVALID_SOCKET) {
		ConnectionState = SMPP_SOCKET_DISCONNECTED;
		shutdown(ClientSocket, 2);
		closesocket(ClientSocket);
		ClientSocket = INVALID_SOCKET;
	}
}

int SmsClient::SendEnquireLink(int sequenceNumber)
{
	int    ok = 1, cmd_status = NO_STATUS, r = 0;
	SString err_msg;
	SmsProtocolBuf protocol_buf(_byteswap_ulong(ENQUIRE_LINK), 0, _byteswap_ulong(sequenceNumber));
	protocol_buf.Finish();
	THROW(Send(protocol_buf, 1));
	{
		uint   reconnection_try_nums = 0;
		if(!(r = Receive(ENQLINQ_RECEIVE_TIMEOUT))) {
			while(!(r = TryToReconnect(reconnection_try_nums)) && reconnection_try_nums < Config.ReconnectTriesNum)
				continue;
			THROW(r);
			r = Receive(ENQLINQ_RECEIVE_TIMEOUT);
		}
		THROW(r);
	}
	if((cmd_status = SMResults.GetResult(SMResults.EnquireLinkResult)) != ESME_ROK) {
		GetSmscErrorText(cmd_status, err_msg);
		PPSetError(PPERR_SMS_ENQLINKERROR, err_msg.ToOem());
		ok = 0;
	}
	CATCHZOK;
	return ok;
}

int SmsClient::SendGenericNack(int sequenceNumber, int commandStatus)
{
	int    ok = 1;
	int    cmd_status = NO_STATUS, r = 0;
	SmsProtocolBuf protocol_buf(_byteswap_ulong(GENERIC_NACK), _byteswap_ulong(commandStatus), _byteswap_ulong(sequenceNumber));
	protocol_buf.Finish();
	THROW(Send(protocol_buf, 1));
	{
		uint   reconnection_try_nums = 0;
		if(!(r = Receive(GENNAC_RECEIVE_TIMEOUT))) {
			while(!(r = TryToReconnect(reconnection_try_nums)) && reconnection_try_nums < Config.ReconnectTriesNum)
				continue;
			THROW(r);
			r = Receive(GENNAC_RECEIVE_TIMEOUT);
		}
		THROW(r);
		if((cmd_status = SMResults.GenericNackResult) != ESME_ROK) {
			PPError(cmd_status);
			ok = 0;
		}
		else {
			SMResults.GenericNackResult = -1;
			ReceiveToTimer();
		}
	}
	CATCHZOK;
	return ok;
}

void SmsClient::AddStatusCode(const char * pDestNum, int statusCode, const char * pError)
{
	SString status_text, status_msg;
	(status_msg = 0).Cat(pDestNum).Semicol();
	GetStatusText(statusCode, status_text);
	if(status_text.NotEmpty())
		status_msg.Cat(status_text);
	if(strcmp(pError, "000")) // Если 000 (ошибок нет), то ничего не выводим
		status_msg.Space().Cat("Error").Space().Cat(pError);
	StatusCodesArr.Add(AddStatusCodeNum++, status_msg, 1);
	if(statusCode == SMS_UNDELIVERABLE)
		UndeliverableMessages++;
}

int CopyUStrToStr(SStringU & rUStr, SString & rStr)
{
	int    ok = 1;
	char   buf[2];
	memzero(buf, 2);
	for(size_t i = 0; i < rUStr.Len() + 1; i++) {
		memcpy(buf, (&rUStr[i]), 2);
		if(buf[0] != 0 || buf[1] != 0) {
			rStr.CatChar(buf[1]);
			rStr.CatChar(buf[0]);
		}
	}
	return ok;
}

int SmsClient::SubmitSM(const StSubmitSMParam & rParam, const char * pMessage, bool payloadMessage)
{
	int    ok = 1;
    uint   message_len = 0, reconnection_try_nums = 0;
	int    cmd_status = NO_STATUS, r = 0;
	SString err_msg;
	struct SubmitSmPduBody {
		SString	ServiceType;	// Тип сервиса
		uchar	SourceAddrTon;	// Тип номера отправител
		uchar	SourceAddrNpi;	// Схема присвоения номеров для отправител
		SString	SourceAddr;		// Номер отправител
		uchar	DestAddrTon;	// Тип номера получател
		uchar	DestAddrNpi;	// Схема присвоения номеров для получател
		SString	DestAddr;		// Номер получател
		uchar	EsmClass;		// Тип сообщения и режим отправки
		uchar	ProtocolId;		// Версия протокола
		uchar	PriorityFlag;	// Флаг приоритета доставки сообщени
		SString	SchedDelivrTime;	// Время, когда сообщение должно быть доставлено
		SString	ValidityPeriod;	// Период допустимости сообщения (сколько оно будет храниться на СМСЦ)
		uchar	RegrDelivery;	// Сразу получить статус сообщени
		uchar	ReplaceIfPresent;	// Заменить существующее сообщение в очереди СМСЦ
		uchar	DataCoding;		// Тип кодировки сообщени
		uchar	SmDefaultMsgId;	// Номер сообщения из списка предопределенных
		uchar	MessageLen;		// Длина сообщени
		SString	ShortMessage;	// Сообщение
		uint8	Reserved[4];
	};
	struct SubmitExtraParams {
		uchar	Extra[43];	// Ряд дополнительных параметров
		ushort	PayloadTag; // Тег поля Payload
		ushort	PayloadLen;	// Длина сообщени
		SString	PayloadValue; // Сообщение
		uint8	Reserved[17];
	};

	SubmitSmPduBody sbmt_pdu_body;
	SubmitExtraParams sbmt_extra;
	SmsProtocolBuf protocol_buf(_byteswap_ulong(SUBMIT_SM), 0, _byteswap_ulong(SequenceNumber++));
	GetString("", MAX_SERVICE_TYPE_LEN - 1, DEFAULT_SERVICE_TYPE, sbmt_pdu_body.ServiceType);
	sbmt_pdu_body.SourceAddrTon = rParam.SourceAddressTon;
	sbmt_pdu_body.SourceAddrNpi = rParam.SourceAddressNpi;
	GetString(rParam.SourceAddress, MAX_ADDR_LEN - 1, "", sbmt_pdu_body.SourceAddr);
	sbmt_pdu_body.DestAddrTon = rParam.DestinationAddressTon;
	sbmt_pdu_body.DestAddrNpi = rParam.DestinationAddressNpi;
	GetString(rParam.DestinationAddress, MAX_ADDR_LEN - 1, "", sbmt_pdu_body.DestAddr);
	sbmt_pdu_body.EsmClass = rParam.EsmClass;
	sbmt_pdu_body.ProtocolId = rParam.ProtocolId;
	sbmt_pdu_body.PriorityFlag = rParam.PriorityFlag;
	GetString(rParam.SheduleDeliveryTime, MAX_DATE_LEN - 1, SCHEDULE_DELIVERY_TIME, sbmt_pdu_body.SchedDelivrTime);
	GetString(rParam.ValidityPeriod, MAX_DATE_LEN - 1, VALIDITY_PERIOD, sbmt_pdu_body.ValidityPeriod);
	sbmt_pdu_body.RegrDelivery = 1; // По получении этого запроса СМСЦ должен выслать ответ с описанием статуса сообщени
	sbmt_pdu_body.ReplaceIfPresent = rParam.ReplaceIfPresentFlag;
	sbmt_pdu_body.DataCoding = rParam.DataCoding;
	sbmt_pdu_body.SmDefaultMsgId = rParam.SmDefaultMsgId;
	if(!payloadMessage) { // Длинное сообщение было разбито на короткие
		if(rParam.DataCoding == UCS2) {
 			SString str;
			SStringU ustr;
			ustr.CopyFromUtf8((str = 0).Cat(pMessage).Transf(CTRANSF_INNER_TO_UTF8)); // Переводим сообщение в UTF-16
			CopyUStrToStr(ustr, sbmt_pdu_body.ShortMessage);
			// Длина сообщения (в байтах)
			sbmt_pdu_body.MessageLen = (uchar)sbmt_pdu_body.ShortMessage.Len() + 1;
		}
		else {
			// Длина сообщения (в байтах)
			sbmt_pdu_body.MessageLen = (strlen(pMessage) + 1) > MAX_SUBMIT_MESSAGE_LEN ? (uchar)MAX_SUBMIT_MESSAGE_LEN : (uchar)(strlen(pMessage) + 1); // Длина сообщени
			// Сообщение
			sbmt_pdu_body.ShortMessage.CopyFromN(pMessage, sbmt_pdu_body.MessageLen - 1);
		}
	}
	else { // Отправляем длинное сообщение одним запросом
		sbmt_pdu_body.MessageLen = 0; // Поле sm_length устанавливаем в ноль
		sbmt_pdu_body.ShortMessage = 0; // Поле short_message устанавливаем в ноль
		memzero(sbmt_extra.Extra, SIZEOFARRAY(sbmt_extra.Extra)); // Пропускаем остальные дополнительные пол
		// Значение тега  - 0x0424
		sbmt_extra.PayloadTag = _byteswap_ushort(0x0424);
		if(rParam.DataCoding == UCS2) {
			SString str;
			SStringU ustr;
			ustr.CopyFromUtf8((str = 0).Cat(pMessage).Transf(CTRANSF_INNER_TO_UTF8)); // Переводим сообщение в UTF-16
			CopyUStrToStr(ustr, sbmt_extra.PayloadValue);
			// Длина сообщения (в байтах)
			message_len = sbmt_extra.PayloadValue.Len() + 1;
			sbmt_extra.PayloadLen = _byteswap_ushort(message_len);
		}
		else {
			// Длина сообщенния (в байтах)
			message_len = (strlen(pMessage) + 1) > MAX_MESSAGE_7BIT_LONG ? (uint)MAX_MESSAGE_7BIT_LONG : (uint)(strlen(pMessage) + 1);
			sbmt_extra.PayloadLen = _byteswap_ushort(message_len);
			// Сообщение
			sbmt_extra.PayloadValue.CopyFromN(pMessage, message_len - 1);
		}
	}
	protocol_buf.AddString(sbmt_pdu_body.ServiceType);
	protocol_buf.AddUInt8(sbmt_pdu_body.SourceAddrTon);
	protocol_buf.AddUInt8(sbmt_pdu_body.SourceAddrNpi);
	protocol_buf.AddString(sbmt_pdu_body.SourceAddr);
	protocol_buf.AddUInt8(sbmt_pdu_body.DestAddrTon);
	protocol_buf.AddUInt8(sbmt_pdu_body.DestAddrNpi);
	protocol_buf.AddString(sbmt_pdu_body.DestAddr);
	protocol_buf.AddUInt8(sbmt_pdu_body.EsmClass);
	protocol_buf.AddUInt8(sbmt_pdu_body.ProtocolId);
    protocol_buf.AddUInt8(sbmt_pdu_body.PriorityFlag);
	protocol_buf.AddString(sbmt_pdu_body.SchedDelivrTime);
	protocol_buf.AddString(sbmt_pdu_body.ValidityPeriod);
	protocol_buf.AddUInt8(sbmt_pdu_body.RegrDelivery);
	protocol_buf.AddUInt8(sbmt_pdu_body.ReplaceIfPresent);
	protocol_buf.AddUInt8(sbmt_pdu_body.DataCoding);
	protocol_buf.AddUInt8(sbmt_pdu_body.SmDefaultMsgId);
	protocol_buf.AddUInt8(sbmt_pdu_body.MessageLen);
	if(!payloadMessage) {
		protocol_buf.AddString(sbmt_pdu_body.ShortMessage, sbmt_pdu_body.ShortMessage.Len() + 1);
	}
	if(payloadMessage) {
		protocol_buf.AddString(sbmt_pdu_body.ShortMessage);
		protocol_buf.AddUInt8Array(sbmt_extra.Extra, SIZEOFARRAY(sbmt_extra.Extra));
		protocol_buf.AddUInt16(sbmt_extra.PayloadTag);
		protocol_buf.AddUInt16(sbmt_extra.PayloadLen);
		protocol_buf.AddString(sbmt_extra.PayloadValue, sbmt_extra.PayloadValue.Len() + 1);
	}
	protocol_buf.Finish();
	THROW(Send(protocol_buf, 1));
	reconnection_try_nums = 0;
	if(!(r = Receive(Config.ResponseTimeout))) {
		while(!(r = TryToReconnect(reconnection_try_nums)) && reconnection_try_nums < Config.ReconnectTriesNum)
			continue;
		THROW(r);
		r = Receive(Config.ResponseTimeout);
	}
	THROW(r);
	if((cmd_status = SMResults.SubmitResult) != ESME_ROK) {
		if(cmd_status == NO_STATUS && SMResults.GenericNackResult != NO_STATUS) {
			cmd_status = SMResults.GenericNackResult;
			SMResults.GenericNackResult = NO_STATUS;
			ok = 0;
		}
		if(cmd_status == NO_STATUS) // Если не был получен статус команды
			ok = -1;
		else if(cmd_status == ESME_RMSGQFULL && (ReSendQueueMsgTryNums < Config.ResendTriesNum)) // Если ошибка переполнения очереди
			ok = -1;
		else if(cmd_status == ESME_RINVMSGLEN && (ResendErrLenMsg < 1)) // Если неверная длина сообщени
			ok = -1;
		else if(cmd_status == ESME_RINVDSTADR) // Если неверный адрес отправител
			ok = -1;
		else {
			GetSmscErrorText(cmd_status, err_msg);
			PPSetError(PPERR_SMS_SUBMITERROR, err_msg.ToOem());
			AddErrorSubmit(rParam.DestinationAddress, cmd_status);
			ok = 0;
		}
	}
	reconnection_try_nums = 0;
	if(ok == 1) { // Получаем DeliverSm_resp
		if(!(r = Receive(Config.ResponseTimeout))) {
			while(!(r = TryToReconnect(reconnection_try_nums)) && reconnection_try_nums < /*RECONNECTION_COUNT*/Config.ReconnectTriesNum)
				continue;
			THROW(r);
			r = Receive(Config.ResponseTimeout);
		}
		THROW(r);
	}
	ReceiveToTimer();
	CATCHZOK;
	return ok;
}

int SmsClient::SendSms_(const char * pFrom, const char * pTo, const char * pText)
{
    int    ok = 1, cmd_status = NO_STATUS;
	size_t max_length = 0;
    SString message;
    SString sms_text = pText;

	ResendErrLenMsg = 0;
	ReSendQueueMsgTryNums = 0;
	GetString(pFrom, MAX_ADDR_LEN - 1, "", SMParams.SourceAddress);
	if(SMParams.SourceAddress.NotEmptyS())
		SMParams.SourceAddressTon = TON_ALPHANUMERIC;
	else
		SMParams.SourceAddressTon = Config.SourceAddressTon;
	SMParams.SourceAddressNpi = Config.SourceAddressNpi;
	GetString(pTo, MAX_ADDR_LEN - 1, "", SMParams.DestinationAddress);
	SMParams.DestinationAddressTon = Config.DestAddressTon;
	SMParams.DestinationAddressNpi = Config.DestAddressNpi;
	if(Config.DataCoding == UCS2 || !PPObjSmsAccount::VerifyString(sms_text, 0)) { // Если текст написан кириллицей или кодировка UCS2(ISO/IEC-10646)
		max_length = MAX_MESSAGE_UCS2_LEN;
		SMParams.DataCoding = UCS2;
	}
	else
		max_length = MAX_MESSAGE_7BIT_LEN;
	SMParams.ProtocolId = 0;
	SMParams.PriorityFlag = VERY_URGENT;
	GetString(SCHEDULE_DELIVERY_TIME, MAX_DATE_LEN - 1, "", SMParams.SheduleDeliveryTime);
	GetString(VALIDITY_PERIOD, MAX_DATE_LEN - 1, "", SMParams.ValidityPeriod);
	SMParams.ReplaceIfPresentFlag = REPLACE_IF_PRESENT;
	SMParams.SmDefaultMsgId = 0;
	if((sms_text.Len() + 1 <= max_length) || Config.SplitLongMsg) {
		while(sms_text.Len() > 0) {
			sms_text.Sub(0, ((sms_text.Len() + 1) > max_length) ? (max_length - 1) : sms_text.Len(), message);
			sms_text = sms_text.Excise(0, ((sms_text.Len() + 1) > max_length) ? (max_length - 1) : sms_text.Len());
			do {
				SMResults.SubmitResult = -1;
				THROW(ok = SubmitSM(SMParams, message, 0));
				cmd_status = SMResults.SubmitResult;
				if(ok != 1) {
					if(cmd_status == ESME_RMSGQFULL) { // Если переполнена очередь смс
						delay(/*RESEND_QUEUE_MSG_TIMEOUT*/Config.ResendMsgQueueTimeout);
						ReSendQueueMsgTryNums++;
					}
					else if(cmd_status == ESME_RINVMSGLEN && SMParams.DataCoding != UCS2) { // Если неверная длина сообщения, то меняем длину сообщения и пытаемся один раз его переотправить
						ResendErrLenMsg++;
						sms_text.Insert(0, message);
						max_length = MAX_MESSAGE_8BIT_LEN;
						sms_text.Sub(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len(), message);
						sms_text = sms_text.Excise(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len());
						delay(500);
					}
				}
				else {
					MessageCount++;
					ReSendQueueMsgTryNums = 0;
				}
			} while(((cmd_status == ESME_RMSGQFULL) && (ReSendQueueMsgTryNums <= /*RESEND_MSG_COUNT*/Config.ResendTriesNum)) || ((cmd_status == ESME_RINVMSGLEN) && (ResendErrLenMsg <= 1)));
			if(cmd_status == ESME_RINVDSTADR) { // Если неверный адрес отправителя, то запоминаем ошибку и продолжаем слать смс на другие номера
				AddErrorSubmit(SMParams.DestinationAddress, cmd_status);
				sms_text = 0;
			}
		}
	}
	else {
		if(SMParams.DataCoding == UCS2)
			max_length = MAX_MESSAGE_UCS2_LONG;
		else
			max_length = MAX_MESSAGE_7BIT_LONG;
		while(sms_text.Len() > 0) {
			sms_text.Sub(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len(), message);
			sms_text = sms_text.Excise(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len());
			do {
				SMResults.SubmitResult = -1;
				THROW(ok = SubmitSM(SMParams, message, 1));
				cmd_status = SMResults.SubmitResult;
				if(ok != 1) {
					if(cmd_status == ESME_RMSGQFULL) {
						delay(Config.ResendMsgQueueTimeout);
						ReSendQueueMsgTryNums++;
					}
					else if(cmd_status == ESME_RINVMSGLEN) {
						ResendErrLenMsg++;
						sms_text.Insert(0, message);
						max_length = MAX_MESSAGE_8BIT_LEN;
						sms_text.Sub(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len(), message);
						sms_text = sms_text.Excise(0, sms_text.Len() + 1 > max_length ? (max_length - 1) : sms_text.Len());
						delay(500);
					}
				}
				else {
					MessageCount++;
					ReSendQueueMsgTryNums = 0;
				}
			} while(((cmd_status == ESME_RMSGQFULL) && (ReSendQueueMsgTryNums <= Config.ResendTriesNum)) || ((cmd_status == ESME_RINVMSGLEN) && (ResendErrLenMsg <= 1)));
			if(cmd_status == ESME_RINVDSTADR) {
				AddErrorSubmit(SMParams.DestinationAddress, cmd_status);
				sms_text = 0;
			}
		}
	}
	CATCHZOK;
	SMResults.SubmitResult = -1;
    return ok;
}

int SmsClient::SendEnquireLinkResp(int sequenceNumber)
{
	SmsProtocolBuf protocol_buf(_byteswap_ulong(ENQUIRE_LINK_RESP), 0, _byteswap_ulong(sequenceNumber));
	protocol_buf.Finish();
	return Send(protocol_buf, 1);
}

bool SmsClient::CanSend() const
{
	return ((ConnectionState == SMPP_BINDED) /*&& (UndeliverableMessages <= UNDELIVERABLE_MESSAGES)*/) ? true : false; // @replace поставлен комментарий
}

bool SmsClient::IsConnected() const
{
	return ((ConnectionState == SMPP_BINDED) ? true : false);
}

int SmsClient::Connect(const StConfig & rConfig)
{
	int    ok = 1;
	uint   reconnection_try_nums = 0;
	SString msg_buf;
	SetConfig(rConfig);
    if((ok = ConnectToSMSC()) != 1)
		while((ok != 1) && (reconnection_try_nums < Config.ReconnectTriesNum))
			ok = TryToReconnect(reconnection_try_nums);
	THROW(ok == 1);
	PPLoadText(PPTXT_SMSSRVCONNECTED, msg_buf);
	Logger.Log(msg_buf);
	CATCHZOK;
	return ok;
}

int SmsClient::GetStatusCode(SString & rDestNum, SString & rStatus, size_t pos) const
{
	int    ok = 0;
	SString status_msg;
	if((pos < StatusCodesArr.getCount()) && (StatusCodesArr.getCount() != 0)) {
		StatusCodesArr.Get(pos, status_msg);
		status_msg.Divide(';', rDestNum, rStatus);
		ok = 1;
	}
	return ok;
}

int SmsClient::GetErrorSubmit(SString & rDestNum, SString & rErrText, size_t pos) const
{
	int    ok = 0;
	SString err_msg;
	if(pos < ErrorSubmitArr.getCount()) {
		ErrorSubmitArr.Get(pos, err_msg);
		err_msg.Divide(';', rDestNum = 0, rErrText = 0);
		ok = 1;
	}
	return ok;
}

int SmsClient::GetRestOfReceives()
{
	int    ok = 1, count = 1;
	while((count++ <= REST_OF_RECEIVES_COUNT) && (AddStatusCodeNum < MessageCount))
		THROW(Receive(Config.ResponseTimeout));
    CATCH
		ok = 0;
	ENDCATCH;
	return ok;
}

// static
int SmsClient::SendSms(const char * pTo, const char * pText, SString & rStatus)
{
	int    ok = 1;
	size_t pos = 0;
	SString dest_num, status_text;
	THROW_PP(!isempty(pTo), PPERR_SMS_NOTRECIEVENUMBERS);
	THROW_PP(!isempty(pText), PPERR_SMS_SMSTEXTEMPTY);
	THROW_PP(strlen(Config.From) < MAX_ADDR_LEN, PPERR_SMS_SENDERADDRERRLEN);
	THROW_PP(strlen(pTo) < MAX_ADDR_LEN, PPERR_SMS_RECVRADDRERRLEN);
	if(CanSend())
		ok = SendSms_(Config.From, pTo, pText);
	GetRestOfReceives();
	rStatus = 0;
	while(GetStatusCode(dest_num, status_text, pos)) {
		if(rStatus.NotEmpty())
			rStatus.Comma();
		rStatus.Cat(status_text);
		pos++;
	}
	pos = 0;
	while(GetErrorSubmit(dest_num = 0, status_text = 0, pos)) {
		if(rStatus.NotEmpty())
			rStatus.Comma();
		rStatus.Cat(status_text);
		pos++;
	}
	CATCHZOK;
	StatusCodesArr.Clear();
	ErrorSubmitArr.Clear();
	AddStatusCodeNum = 0;
	AddErrorSubmitNum = 0;
	MessageCount = 0;
	return ok;
}
//
// Descr: Инициализирует смс-клиента и устанавливает соединение с СМСЦ для обычной рассылки через диалог
//
int BeginDelivery(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr)
{
	int    ok = 1;
	PPSendSms send_sms;
	SendSmsDialog * dlg = new SendSmsDialog(accID, rPrsnIdArr, rPhoneArr);
	if(CheckDialogPtr(&dlg, 1)) {
		if(ExecView(dlg) == cmOK)
			dlg->getDTS(&send_sms);
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}
//
// Descr: Инициализирует смс-клиента и устанавливает соединение с СМСЦ для автоматической рассылки через шаблон tddo
//
int BeginDelivery(PPID accID, StrAssocArray & rPrsnIdArr, StrAssocArray & rPhoneArr, PPID objTypeId, StrAssocArray & rObjIdArr)
{
	int    ok = 1;
	PPSendSms send_sms;
	SendSmsDialog * dlg = new SendSmsDialog(accID, rPrsnIdArr, rPhoneArr, objTypeId, rObjIdArr);
	//if(CheckDialogPtr(&dlg, 1)) {
		dlg->AutoSmsSending();
		delete dlg;
	//}
	//else
	//	ok = 0;
	return ok;
}

int SmsInit(SmsClient & rCli, PPID accID, PPLogger * pLogger)
{
	int    ok = 1;
	SString msg_buf;
	PPSmsAccPacket pack;
	PPObjSmsAccount mobj;
	StConfig config;

	// Получаем конфигурацию sms-аккаунта
	THROW(mobj.GetPacket(accID, &pack));
	THROW(pack.Verify(0));
	config.Clear();
	THROW(GetSmsConfig(pack, config));
	PPLoadText(PPTXT_SMSACCPARAMRECEIVED, msg_buf);
	msg_buf.CatDiv(':', 2).CatEq("Name", pack.Rec.Name).CatDiv(',', 2).CatEq("Host", config.Host).CatDiv(',', 2).
		CatEq("Port", (ulong)config.Port).CatDiv(',', 2).CatEq("SystemID", config.SystemId).CatDiv(',', 2).CatEq("From", config.From);
	pLogger->Log(msg_buf);
	// Устанавливаем соединение с СМСЦ
	PPLoadText(PPTXT_SMS_CONNECTING, msg_buf);
	pLogger->Log(msg_buf);
	THROW(rCli.Connect(config));
	CATCH
		ok = 0;
		PPLoadText(PPTXT_SMS_NOTCONNECTED, msg_buf);
		if(pLogger)
			pLogger->Log(msg_buf);
	ENDCATCH;
	return ok;
}

//
// Descr: Закрывает соединение с СМСЦ
//
int SmsRelease(SmsClient & rCli, PPLogger * pLogger)
{
	int    ok = 1;
	SString status_msg;
	if(rCli.IsConnected()) {
		ok = rCli.Disconnect();
		PPLoadText(PPTXT_SMS_ENDSENDING, status_msg);
		pLogger->Log(status_msg);
	}
	return ok;
}

int SendingSms(SmsClient & rCli, PPID personID, const char * pPhone, const char * pText, PPLogger * pLogger)
{
	int    ok = 1, skip = 0;
	SString temp_buf, msg_buf, result, err_msg;
	// @v8.0.6 {
	PPObjPerson psn_obj;
	PersonTbl::Rec psn_rec;
	if(psn_obj.Search(personID, &psn_rec) > 0 && !(psn_rec.Flags & PSNF_NONOTIFICATIONS)) {
		PPPersonConfig psn_cfg;
		if(psn_obj.FetchConfig(&psn_cfg) > 0 && psn_cfg.SendSmsSamePersonTimeout > 0) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj) {
				LDATETIME moment;
				PPIDArray acn_list;
				acn_list.add(PPACN_SMSSENDED);
				if(p_sj->GetLastObjEvent(PPOBJ_PERSON, personID, &acn_list, &moment) > 0) {
					const LDATETIME cdtm = getcurdatetime_();
					long  diff_sec = diffdatetimesec(cdtm, moment);
					if(diff_sec <= psn_cfg.SendSmsSamePersonTimeout) {
						if(pLogger) {
							PPLoadText(PPTXT_SMSNOTSENDED_TIMEOUT, temp_buf);
							PPFormat(temp_buf, &msg_buf, personID, psn_cfg.SendSmsSamePersonTimeout);
							pLogger->Log(msg_buf);
						}
						ok = -1;
						skip = 1;
					}
				}
			}
		}
		// } @v8.0.6
		if(!skip) {
			int    connected = 0;
			Tddo   t;
			SBuffer buf;
			StringSet ext_param_list;
			SPathStruc path_struct;
			SString old_phone(pPhone), new_phone;
			PPSendSms send_sms;

			// Отправляем смс
			THROW(rCli.IsConnected());
			if(FormatPhone(old_phone, new_phone, msg_buf = 0)) {
				THROW(rCli.SendSms(new_phone, pText, result = 0));
				(msg_buf = 0).Cat(old_phone).Space().Cat(result);
				if(pLogger)
					pLogger->Log(msg_buf);
				DS.LogAction(PPACN_SMSSENDED, PPOBJ_PERSON, personID, 0, 1); // @v8.0.0
			}
			else if(pLogger)
				pLogger->Log(msg_buf);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPSmsSender::PPSmsSender()
{
}

SLAPI PPSmsSender::~PPSmsSender()
{
}


/*
	Label format definition

@name			-	Имя клиента
@extname        -   Расширенное имя
@mainorgname	-	Название главной организации
@mainorgphone	-	Телефон главной организации
@mainorgaddr	-	Адрес главной организации

*/

enum SmsVarStr {
	smsvsName = 0,
	smsvsExtName,
	smsvsMainorgname,
	smsvsMainorgphone,
	smsvsMainorgaddr,
};

#define FIRSTSUBSTVAR	smsvsName
#define NUMSUBSTVARS	5

#if 0 // @obsolete {

// static
void SendSmsDialog::SubstVar(SString & src, SString & rDest, PPID personeId)
{
	SString s = src;
	SString d = rDest;
	char   temp[256];
	SString temp_str, sub_str;
	PPID org_id = 0;
	PPPersonPacket pack;
	PPObjPerson person_obj;
	PersonTbl::Rec psn_rec;

	if(s.C(0) == '@')
		s.Excise(0, 1);
	PPLoadText(PPTXT_SMSFIELDS_VARS, sub_str);
	for(int i = FIRSTSUBSTVAR; i < FIRSTSUBSTVAR + NUMSUBSTVARS; i++) {
		if(PPGetSubStr(sub_str, i, temp, sizeof(temp)) && strnicmp(s, temp, strlen(temp)) == 0) {
			size_t var_len = strlen(temp);
			switch(i) {
				case smsvsName:
					//temp_str = pack.Rec.Name;
					if(person_obj.GetPacket(personeId, &pack, 0))
						d.Cat(pack.Rec.Name);
					break;
				case smsvsExtName:
					if(person_obj.GetPacket(personeId, &pack, 0))
						pack.GetExtName(temp_str);
					d.Cat(temp_str);
					break;
				case smsvsMainorgname:
					GetMainOrgName(temp_str);
					d.Cat(temp_str);
					break;
				case smsvsMainorgphone:
					GetMainOrgID(&org_id);
					person_obj.Search(org_id, &psn_rec);
					if(person_obj.GetPacket(org_id, &pack, 0))
						pack.GetPhones(1, temp_str);
					d.Cat(temp_str);
					break;
				case smsvsMainorgaddr:
					GetMainOrgID(&org_id);
					person_obj.Search(org_id, &psn_rec);
					if(person_obj.GetPacket(org_id, &pack, 0))
						pack.GetAddress(psn_rec.MainLoc, temp_str);
					d.Cat(temp_str);
					break;
			}
			s.Excise(0, var_len);
			break;
		}
	}
	src = s;
	rDest = d;
}

// static
void SendSmsDialog::FormatText(SString & rSrcMsg, SString & rDestMsg, PPID personeId)
{
	SString d, s;
	d = 0;
	s = rSrcMsg;
	size_t i = 0;
	while(s.C(i) != 0) {
		if(s.C(i) != '@') {
			d.CatChar(s.C(i));
			i++;
		}
		else {
			s.Excise(0, i + 1);
			SubstVar(s, d, personeId);
			i = 0;
		}
	}
	(rDestMsg = 0).Cat(d);
}

#endif // } 0 @obsolete

//void SendSmsDialog::GetStrVar(size_t pos, SString & rVar)
//static
int SLAPI PPSmsSender::GetSubstVar(long p, SString & rBuf)
{
	rBuf = 0;
	int    ok = 1;
	SString sub_str;
	PPLoadText(PPTXT_SMSFIELDS_VARS, sub_str);
	if(PPGetSubStr(sub_str, p, rBuf)) {
		rBuf.PadLeft(1, '@');
	}
	else {
		rBuf = 0;
		ok = 0;
	}
	return ok;
}

//static
int SLAPI PPSmsSender::FormatMessage(const char * pTemplate, SString & rResult, PPSmsSender::FormatMessageBlock * pFmBlk)
{
	rResult = 0;
	int    ok = 1;
	SString sub_str;
	SString var_buf, temp_buf;
	PPObjPerson psn_obj;
	PersonTbl::Rec psn_rec;
	PPPersonPacket psn_pack;

	PPLoadText(PPTXT_SMSFIELDS_VARS, sub_str);
	for(const char * p = pTemplate; *p;) {
		if(*p == '@') {
			int    subst_var_success = 0;
			PPID   org_id = 0;
			temp_buf = 0;
			for(int i = FIRSTSUBSTVAR; i < FIRSTSUBSTVAR + NUMSUBSTVARS; i++) {
				if(PPGetSubStr(sub_str, i, var_buf) && var_buf.CmpL(p+1, 1) == 0) {
					switch(i) {
						case smsvsName:
							GetPersonName(pFmBlk ? pFmBlk->PersonID : 0, temp_buf);
							rResult.Cat(temp_buf);
							break;
						case smsvsExtName:
							if(pFmBlk && pFmBlk->PersonID && psn_obj.GetPacket(pFmBlk->PersonID, &psn_pack, 0) > 0)
								psn_pack.GetExtName(temp_buf);
							rResult.Cat(temp_buf);
							break;
						case smsvsMainorgname:
							GetMainOrgName(temp_buf);
							rResult.Cat(temp_buf);
							break;
						case smsvsMainorgphone:
							GetMainOrgID(&org_id);
							if(psn_obj.GetPacket(org_id, &psn_pack, 0) > 0)
								psn_pack.GetPhones(1, temp_buf);
							rResult.Cat(temp_buf);
							break;
						case smsvsMainorgaddr:
							GetMainOrgID(&org_id);
							if(psn_obj.Search(org_id, &psn_rec) > 0)
								psn_pack.GetAddress(psn_rec.MainLoc, temp_buf);
							rResult.Cat(temp_buf);
							break;
					}
					p += var_buf.Len()+1;
					subst_var_success = 1;
					break;
				}
			}
			if(!subst_var_success)
				rResult.CatChar(*p++);
		}
		else
			rResult.CatChar(*p++);
	}
	return ok;
}

