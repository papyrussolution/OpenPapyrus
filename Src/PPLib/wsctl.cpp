// WSCTL.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <wsctl.h>
//
//
//
WsCtl_SelfIdentityBlock::WsCtl_SelfIdentityBlock() : PrcID(0)
{
}
	
int WsCtl_SelfIdentityBlock::GetOwnUuid()
{
	int    ok = 1;
	bool   found = false;
	S_GUID _uuid;
	{
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 1);
		if(reg_key.GetBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid)) > 0) {
			Uuid = _uuid;
			found = true;
		}
	}
	if(!found) {
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 0);
		_uuid.Generate();
		if(reg_key.PutBinary(PPConst::WrParam_WsCtl_MachineUUID, &_uuid, sizeof(_uuid))) {
			Uuid = _uuid;
			ok = 2;
		}
		else
			ok = 0;
	}
	if(ok > 0) {
		SString msg_buf;
		msg_buf.Z().Cat("WSCTL own-uuid").CatDiv(':', 2).Cat(Uuid, S_GUID::fmtIDL);
		PPLogMessage("wsctl-debug.log", msg_buf, LOGMSGF_TIME|LOGMSGF_COMP|LOGMSGF_SLSSESSGUID);
	}
	return ok;
}
//
//
//
WsCtl_Config::WsCtl_Config() : Port(0), Timeout(0)
{
}
	
WsCtl_Config & WsCtl_Config::Z()
{
	Server.Z();
	Port = 0;
	Timeout = 0;
	DbSymb.Z();
	User.Z();
	Password.Z();
	return *this;
}
//
// Descr: Считывает конфигурацию из win-реестра
//
int WsCtl_Config::Read()
{
	int    ok = 1;
	SJson * p_js = 0;
	SSecretTagPool pool;
	{
		SBuffer sbuf;
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 1);
		int    r = reg_key.GetBinary(PPConst::WrParam_WsCtl_Config, sbuf);
		THROW_SL(r);
		if(r > 0) {
			SString temp_buf;
			SSecretTagPool pool;
			THROW_SL(pool.Serialize(-1, sbuf, 0));
			p_js = pool.GetJson(SSecretTagPool::tagRawData);
			THROW(FromJsonObj(p_js));
		}
		else
			ok = -1;
	}
	CATCHZOK
	delete p_js;
	return ok;
}
//
// Descr: Записывает конфигурацию в win-реестр
//
int WsCtl_Config::Write()
{
	int    ok = 1;
	SString temp_buf;
	SSecretTagPool pool;
	SBuffer sbuf;
	SJson * p_js = ToJsonObj();
	THROW(p_js);
	p_js->ToStr(temp_buf);
	THROW_SL(pool.Put(SSecretTagPool::tagRawData, temp_buf, temp_buf.Len(), 0));
	THROW_SL(pool.Serialize(+1, sbuf, 0));
	{
		WinRegKey reg_key(HKEY_LOCAL_MACHINE, PPConst::WrKey_WsCtl, 0);
		THROW_SL(reg_key.PutBinary(PPConst::WrParam_WsCtl_Config, sbuf.GetBufC(), sbuf.GetAvailableSize()));
		ok = 1;
	}
	CATCHZOK
	delete p_js;
	return ok;
}

SJson * WsCtl_Config::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertStringNe("server", (temp_buf = Server).Escape());
	if(Port > 0) {
		p_result->InsertInt("port", Port);
	}
	if(Timeout > 0) {
		p_result->InsertInt("timeout", Timeout);
	}
	p_result->InsertStringNe("dbsymb", (temp_buf = DbSymb).Escape());
	p_result->InsertStringNe("user", (temp_buf = User).Escape());
	p_result->InsertStringNe("password", (temp_buf = Password).Escape());
	return p_result;
}

int WsCtl_Config::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 0;
	if(SJson::IsObject(pJsObj)) {
		Z();
		const SJson * p_c = pJsObj->FindChildByKey("server");
		if(SJson::IsString(p_c))
			(Server = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("port");
		if(SJson::IsNumber(p_c))
			Port = p_c->Text.ToLong();
		p_c = pJsObj->FindChildByKey("timeout");
		if(SJson::IsNumber(p_c))
			Timeout = p_c->Text.ToLong();
		p_c = pJsObj->FindChildByKey("dbsymb");
		if(SJson::IsString(p_c))
			(DbSymb = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("user");
		if(SJson::IsString(p_c))
			(User = p_c->Text).Unescape();
		p_c = pJsObj->FindChildByKey("password");
		if(SJson::IsString(p_c))
			(Password = p_c->Text).Unescape();
		ok = 1;
	}
	return ok;
}
//
//
//
WsCtl_ClientPolicy::WsCtl_ClientPolicy()
{
}
	
WsCtl_ClientPolicy & WsCtl_ClientPolicy::Z()
{
	SysUser.Z();
	SysPassword.Z();
	SsAppEnabled.Z();
	SsAppDisabled.Z();
	return *this;
}

bool FASTCALL WsCtl_ClientPolicy::IsEq(const WsCtl_ClientPolicy & rS) const
{
	bool   eq = true;
	if(SysUser != rS.SysUser)
		eq = false;
	else if(SysPassword != rS.SysPassword)
		eq = false;
	else {
		if(!SsAppEnabled.IsEqPermutation(rS.SsAppEnabled))
			eq = false;
		else if(!SsAppDisabled.IsEqPermutation(rS.SsAppDisabled))
			eq = false;
	}
	return eq;
}
	
SJson * WsCtl_ClientPolicy::ToJsonObj() const
{
	SJson * p_js = SJson::CreateObj();
	SString temp_buf;
	if(SysUser.NotEmpty() || SysPassword.NotEmpty()) {
		SJson * p_js_acc = SJson::CreateObj();
		if(SysUser.NotEmpty()) {
			(temp_buf = SysUser).Escape();
			p_js_acc->InsertString("login", temp_buf);
		}
		if(SysPassword.NotEmpty()) {
			(temp_buf = SysPassword).Escape();
			p_js_acc->InsertString("password", temp_buf);
		}
		p_js->Insert("account", p_js_acc);
	}
	if(SsAppEnabled.getCount() || SsAppDisabled.getCount()) {
		SJson * p_js_app = SJson::CreateObj();
		if(SsAppEnabled.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint ssp = 0; SsAppEnabled.get(&ssp, temp_buf);) {
				if(temp_buf.NotEmptyS()) {
					p_js_arr->InsertChild(SJson::CreateString(temp_buf));
				}
			}
			p_js_app->Insert("enable", p_js_arr);
		}
		if(SsAppDisabled.getCount()) {
			SJson * p_js_arr = SJson::CreateArr();
			for(uint ssp = 0; SsAppDisabled.get(&ssp, temp_buf);) {
				if(temp_buf.NotEmptyS()) {
					p_js_arr->InsertChild(SJson::CreateString(temp_buf));
				}
			}
			p_js_app->Insert("disable", p_js_arr);
		}
		p_js->Insert("app", p_js_app);
	}
	return p_js;
}
	
int WsCtl_ClientPolicy::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	if(SJson::IsObject(pJsObj)) {
		Z();
		SString temp_buf;
		const SJson * p_c = pJsObj->FindChildByKey("account");
		if(SJson::IsObject(p_c)) {
			//(Server = p_c->Text).Unescape();
			const SJson * p_ac = p_c->FindChildByKey("login");
			if(SJson::IsString(p_ac)) {
				(SysUser = p_ac->Text).Unescape();
			}
			p_ac = p_c->FindChildByKey("password");
			if(SJson::IsString(p_ac)) {
				(SysPassword = p_ac->Text).Unescape();
			}
		}
		p_c = pJsObj->FindChildByKey("app");
		if(SJson::IsObject(p_c)) {
			const SJson * p_ac = p_c->FindChildByKey("enable");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsString(p_js_item)) {
						(temp_buf = p_js_item->Text).Unescape();
						if(temp_buf.NotEmptyS())
							SsAppEnabled.add(temp_buf);
					}
				}
			}
			p_ac = p_c->FindChildByKey("disable");
			if(SJson::IsArray(p_ac)) {
				for(const SJson * p_js_item = p_ac->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
					if(SJson::IsString(p_js_item)) {
						(temp_buf = p_js_item->Text).Unescape();
						if(temp_buf.NotEmptyS())
							SsAppDisabled.add(temp_buf);
					}
				}
			}
		}
		ok = 1;			
	}
	return ok;
}

int WsCtl_ClientPolicy::Apply()
{
	// HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion
	// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun
	// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\RestrictRun
	int    ok = 1;
	if(SsAppDisabled.getCount()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\DisallowRun", 0);
		THROW(reg_key.PutEnumeratedStrings(SsAppDisabled, 0));
	}
	if(SsAppEnabled.getCount()) {
		WinRegKey reg_key(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\RestrictRun", 0);
		THROW(reg_key.PutEnumeratedStrings(SsAppEnabled, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
WsCtlSrvBlock::AuthBlock::AuthBlock()
{
}
		
WsCtlSrvBlock::AuthBlock & WsCtlSrvBlock::AuthBlock::Z()
{
	WsCtlUuid.Z();
	LoginText.Z();
	Pw.Z();
	return *this;
}
//
// Descr: Возвращает true если входящие параметры инициализированы в достаточной степени для запуска исполнения команды
//
bool WsCtlSrvBlock::AuthBlock::IsEmpty() const
{
	return LoginText.IsEmpty();//(Phone.NotEmpty() || Name.NotEmpty() || ScCode.NotEmpty());
}

bool WsCtlSrvBlock::AuthBlock::FromJsonObj(const SJson * pJs)
{
	Z();
	if(pJs) {
		const SJson * p_c = pJs->FindChildByKey("login");
		if(SJson::IsString(p_c))
			LoginText = p_c->Text;
		p_c = pJs->FindChildByKey("pw");
		if(SJson::IsString(p_c))
			Pw = p_c->Text;
		p_c = pJs->FindChildByKey("wsctluuid");
		if(SJson::IsString(p_c)) {
			WsCtlUuid.FromStr(p_c->Text);
		}
	}
	return !IsEmpty();
}

WsCtlSrvBlock::StartSessBlock::StartSessBlock() : SCardID(0), GoodsID(0), TSessID(0), TechID(0), Amount(0.0), RetTechID(0), RetSCardID(0), RetGoodsID(0), ScOpDtm(ZERODATETIME), WrOffAmount(0.0)
{
}
		
WsCtlSrvBlock::StartSessBlock & WsCtlSrvBlock::StartSessBlock::Z()
{
	WsCtlUuid.Z();
	SCardID = 0;
	GoodsID = 0;
	TSessID = 0;
	TechID = 0;
	Amount = 0.0;
	RetTechID = 0;
	RetSCardID = 0;
	RetGoodsID = 0;
	ScOpDtm.Z();
	WrOffAmount = 0.0;
	SessTimeRange.Z();
	return *this;
}

bool WsCtlSrvBlock::StartSessBlock::FromJsonObj(const SJson * pJs)
{
	Z();
	if(pJs) {
		const SJson * p_c = pJs->FindChildByKey("scardid");
		if(SJson::IsNumber(p_c))
			SCardID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("goodsid");
		if(SJson::IsNumber(p_c))
			GoodsID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("techid");
		if(SJson::IsNumber(p_c))
			TechID = p_c->Text.ToLong();
		p_c = pJs->FindChildByKey("amt");
		if(SJson::IsNumber(p_c))
			Amount = p_c->Text.ToReal();
		p_c = pJs->FindChildByKey("wsctluuid");
		if(SJson::IsString(p_c)) {
			WsCtlUuid.FromStr(p_c->Text);
		}
	}
	return !IsEmpty();
}

WsCtlSrvBlock::WsCtlSrvBlock() : PrcID(0), ScSerID(0), PsnKindID(0)
{
	ScObj.FetchConfig(&ScCfg);
	if(ScCfg.DefCreditSerID) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(ScCfg.DefCreditSerID, &scs_rec) > 0) {
			ScSerID = ScCfg.DefCreditSerID;
			PsnKindID = scs_rec.PersonKindID;
		}
	}
}
	
int WsCtlSrvBlock::SearchPrcByWsCtlUuid(const S_GUID & rWsCtlUuid, PPID * pPrcID, SString * pPrcNameUtf8)
{
	int    ok = -1;
	ASSIGN_PTR(pPrcID, 0);
	CALLPTRMEMB(pPrcNameUtf8, Z());
	if(!!rWsCtlUuid) {
		Reference * p_ref = PPRef;
		PPIDArray prc_list;
		p_ref->Ot.SearchObjectsByGuid(PPOBJ_PROCESSOR, PPTAG_PRC_UUID, rWsCtlUuid, &prc_list);
		THROW_PP_S(prc_list.getCount(), PPERR_WSCTL_PRCBYUUIDNFOUND, SLS.AcquireRvlStr().Cat(rWsCtlUuid, S_GUID::fmtIDL));
		THROW_PP_S(prc_list.getCount() == 1, PPERR_WSCTL_DUPPRCUUID, SLS.AcquireRvlStr().Cat(rWsCtlUuid, S_GUID::fmtIDL));
		{
			const PPID prc_id = prc_list.get(0);
			ProcessorTbl::Rec prc_rec;
			THROW(TSesObj.PrcObj.Fetch(prc_id, &prc_rec) > 0);
			ASSIGN_PTR(pPrcID, prc_id);
			//WsUUID = rUuid;
			if(pPrcNameUtf8)
				(*pPrcNameUtf8 = prc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
	
int WsCtlSrvBlock::Init(const S_GUID & rUuid)
{
	int    ok = SearchPrcByWsCtlUuid(rUuid, &PrcID, &PrcNameUtf8);
	if(ok > 0)
		WsUUID = rUuid;
	return ok;
}
//
// Descr: Возвращает ранжированный список видов котировок, которые могут быть применены для установки цен для технологических сессий.
//
int WsCtlSrvBlock::GetRawQuotKindList(PPIDArray & rList) 
{
	rList.Z();
	int    ok = 1;
	StrAssocArray qk_sa_list;
	QuotKindFilt qk_filt;
	qk_filt.MaxItems = -1;
	qk_filt.Flags |= (QuotKindFilt::fIgnoreRights|QuotKindFilt::fAddBase|QuotKindFilt::fSortByRankName);
	QkObj.MakeList(&qk_filt, &qk_sa_list);
	{
		for(uint i = 0; i < qk_sa_list.getCount(); i++) {
			rList.add(qk_sa_list.at_WithoutParent(i).Id);
		}
		QkObj.ArrangeList(getcurdatetime_(), rList, RTLPF_USEQUOTWTIME);
	}
	return ok;
}

int WsCtlSrvBlock::GetQuotList(PPID goodsID, PPID locID, const PPIDArray & rRawQkList, PPQuotArray & rQList)
{
	rQList.clear();
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(goodsID, &goods_rec) > 0) {
		PPQuotKind qk_rec;
		PPIDArray qk_list_intersection;
		GObj.GetQuotList(goodsID, locID, rQList);
		rQList.GetQuotKindIdList(qk_list_intersection);
		qk_list_intersection.intersect(&rRawQkList, 0);
		if(qk_list_intersection.getCount()) {
			uint i = rQList.getCount();
			assert(i);
			if(i) {
				do {
					const PPQuot & r_q = rQList.at(--i);
					if(qk_list_intersection.lsearch(r_q.Kind)) {
						const int qkfr = QkObj.Fetch(r_q.Kind, &qk_rec);
						assert(qkfr > 0);
						if(qkfr <= 0) {
							rQList.atFree(i);
						}
					}
					else {
						rQList.atFree(i);
					}
				} while(i);
				if(rQList.getCount())
					ok = 1;
			}
		}
	}
	return ok;
}
	
int WsCtlSrvBlock::StartSess(StartSessBlock & rBlk)
{
	int    ok = 1;
	const  LDATETIME now_dtm = getcurdatetime_();
	PPID   tses_id = 0;
	double screst = 0.0; // Остаток на счете клиента на момент начала сессии
	SCardTbl::Rec sc_rec;
	Goods2Tbl::Rec goods_rec;
	THROW(!rBlk.IsEmpty());
	THROW(rBlk.WsCtlUuid);
	THROW(ScSerID); // @todo @err (не удалось определить серию карт, с которыми ассоциированы аккаунты клиентов)
	THROW(ScObj.Search(rBlk.SCardID, &sc_rec) > 0);
	THROW(ScObj.P_Tbl->GetRest(rBlk.SCardID, ZERODATE, &screst));
	THROW(GObj.Search(rBlk.GoodsID, &goods_rec) > 0);
	{
		PPID   prc_id = 0;
		PPID   tec_id = 0;
		double amount_to_wroff = 0.0;
		PPIDArray tec_id_list;
		SString prc_name_utf8;
		ProcessorTbl::Rec prc_rec;
		TechTbl::Rec tec_rec;
		THROW(SearchPrcByWsCtlUuid(rBlk.WsCtlUuid, &prc_id, &prc_name_utf8) > 0);
		THROW(TSesObj.GetPrc(prc_id, &prc_rec, 1, 1) > 0);
		TSesObj.TecObj.GetListByPrcGoods(prc_id, rBlk.GoodsID, &tec_id_list);
		THROW(tec_id_list.getCount());
		tec_id = tec_id_list.get(0);
		THROW(TSesObj.TecObj.Fetch(tec_id, &tec_rec) > 0);
		{
			double price = 0.0;
			PPIDArray raw_qk_id_list;
			PPQuotArray qlist;
			GetRawQuotKindList(raw_qk_id_list);
			if(GetQuotList(rBlk.GoodsID, prc_rec.LocID, raw_qk_id_list, qlist) > 0) {
				assert(qlist.getCount());
				price = qlist.at(0).Quot;
			}
			if(price > 0.0) {
				if(rBlk.Amount > 0) {
					if(feqeps(rBlk.Amount, price, 1.0E-6)) {
						amount_to_wroff = price; // ok
					}
					else {
						; // @todo @err
					}
				}
			}
			else {
				; // @todo @err
			}
		}
		if(amount_to_wroff > 0.0) {
			THROW_PP_S(amount_to_wroff <= screst, PPERR_SCARDRESTNOTENOUGH, sc_rec.Code);
		}
		{
			TSessionPacket pack;
			ProcessorTbl::Rec inh_prc_rec;
			TSesObj.InitPacket(&pack, TSESK_SESSION, prc_id, 0, TSESST_INPROCESS);
			pack.OuterTimingPrice = amount_to_wroff;
			pack.Rec.TechID = tec_id;
			pack.Rec.SCardID = rBlk.SCardID; 
			pack.Rec.StDt = now_dtm.d;
			pack.Rec.StTm = now_dtm.t;
			pack.Rec.PlannedQtty = (tec_rec.InitQtty > 0.0f) ? tec_rec.InitQtty : 1.0;
			if(tec_rec.Capacity > 0.0 && pack.Rec.PlannedQtty > 0.0) {
				long duration = R0i(pack.Rec.PlannedQtty / tec_rec.Capacity);
				if(duration > 0) {
					pack.Rec.PlannedTiming = duration;
					LDATETIME finish_dtm;
					(finish_dtm = now_dtm).addsec(duration);
					pack.Rec.FinDt = finish_dtm.d;
					pack.Rec.FinTm = finish_dtm.t;
				}
			}
			THROW(TSesObj.CheckSessionTime(pack.Rec));
			THROW(TSesObj.GetPrc(prc_id, &inh_prc_rec, 1, 1) > 0);
			if(inh_prc_rec.WrOffOpID) {
				PPOprKind op_rec;
				if(GetOpData(inh_prc_rec.WrOffOpID, &op_rec) > 0 && op_rec.AccSheetID && sc_rec.PersonID) {
					PPID   ar_id = 0;
					if(ArObj.P_Tbl->PersonToArticle(sc_rec.PersonID, op_rec.AccSheetID, &ar_id) > 0) {
						pack.Rec.ArID = ar_id;
					}
				}
			}
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(TSesObj.PutPacket(&tses_id, &pack, 0));
				pack.GetTimeRange(rBlk.SessTimeRange);
				if(amount_to_wroff > 0.0) {
					SCardCore::OpBlock op;
					op.LinkOi.Set(PPOBJ_TSESSION, tses_id);
					op.SCardID = rBlk.SCardID;
					op.Amount = -amount_to_wroff;
					op.Dtm = getcurdatetime_();
					THROW(ScObj.P_Tbl->PutOpBlk(op, 0, 0));
					rBlk.RetSCardID = op.SCardID;
					rBlk.ScOpDtm = op.Dtm;
					rBlk.WrOffAmount = op.Amount;
				}
				THROW(tra.Commit());
			}
			{
				/*
					PPID   TSessID;
					PPID   TechID;
					PPID   RetSCardID;
					PPID   RetGoodsID;
					LDATETIME ScOpDtm; // Время операции списания денег //
					double WrOffAmount; // Списанная сумма //
					STimeChunk SessTimeRange;
				*/
				rBlk.TSessID = tses_id;
				rBlk.RetTechID = pack.Rec.TechID;
				rBlk.RetSCardID = rBlk.SCardID;
				rBlk.RetGoodsID = rBlk.GoodsID;
				// @todo rBlk.ScOpDtm
				// @todo rBlk.WrOffAmount
				rBlk.SessTimeRange.Start.Set(pack.Rec.StDt, pack.Rec.StTm);
				rBlk.SessTimeRange.Finish.Set(pack.Rec.FinDt, pack.Rec.FinTm);
			}
		}
	}
	CATCHZOK
	return ok;
}
	
int WsCtlSrvBlock::Auth(AuthBlock & rBlk)
{
	int    ok = 0;
	SString temp_buf;
	SCardTbl::Rec sc_rec;
	PersonTbl::Rec psn_rec;
	THROW(!rBlk.IsEmpty()); // @todo @err (не определены параметры для авторизации)
	THROW(ScSerID); // @todo @err (не удалось определить серию карт, с которыми ассоциированы аккаунты клиентов)
	assert(rBlk.LoginText.NotEmpty()); // Вызов rBlk.IsEmpty() выше должен гарантировать этот инвариант
	{
		bool    text_identification_done = false;
		SNaturalTokenArray nta;
		STokenRecognizer tr;
		tr.Run(rBlk.LoginText.ucptr(), rBlk.LoginText.Len(), nta, 0);
		if(nta.Has(SNTOK_PHONE)) {
			PPObjIDArray oid_list_by_phone;
			PPEAddr::Phone::NormalizeStr(rBlk.LoginText, 0, temp_buf);
			PsnObj.LocObj.P_Tbl->SearchPhoneObjList(temp_buf, 0, oid_list_by_phone);
			if(oid_list_by_phone.getCount()) {
				PPID   single_sc_id = 0;
				PPID   single_psn_id = 0;
				bool   mult_sc_id_by_phone = false;
				bool   mult_psn_id_by_phone = false;
				for(uint i = 0; i < oid_list_by_phone.getCount(); i++) {
					const PPObjID oid = oid_list_by_phone.at(i);
					if(oid.Obj == PPOBJ_SCARD && ScObj.Fetch(oid.Id, &sc_rec) > 0 && sc_rec.SeriesID == ScSerID) {
						if(!single_sc_id) {
							if(!mult_sc_id_by_phone)
								single_sc_id = oid.Id;
						}
						else {
							single_sc_id = 0;
							mult_sc_id_by_phone = true;
						}
					}
					else if(oid.Obj == PPOBJ_PERSON && PsnObj.Fetch(oid.Id, &psn_rec) > 0) {
						if(!single_psn_id) {
							if(!mult_psn_id_by_phone)
								single_psn_id = oid.Id;
						}
						else {
							single_psn_id = 0;
							mult_psn_id_by_phone = true;
						}
					}
				}
				if(single_sc_id) {
					THROW(ScObj.Search(single_sc_id, &sc_rec) > 0); // @todo @err // Выше мы проверили single_sc_id на "невисячесть"
					THROW(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0); // @todo @err
					rBlk.SCardID = single_sc_id;
					rBlk.PsnID = psn_rec.ID;
					// (еще пароль проверить надо) ok = 1;
				}
				else if(single_psn_id) {
					PPIDArray potential_sc_list;
					THROW(PsnObj.Search(single_psn_id, &psn_rec) > 0); // @todo @err // Выше мы проверили single_psn_id на "невисячесть"
					ScObj.P_Tbl->GetListByPerson(single_psn_id, ScSerID, &potential_sc_list);
					THROW(potential_sc_list.getCount()); // @todo @err
					rBlk.SCardID = potential_sc_list.get(0);
					rBlk.PsnID = single_psn_id;
					text_identification_done = true;
					// (еще пароль проверить надо) ok = 1;
				}
			}
		}
		if(!text_identification_done) {
			if(ScObj.SearchCode(ScSerID, rBlk.LoginText, &sc_rec) > 0) {
				THROW(sc_rec.SeriesID == ScSerID); // @todo @err
				THROW(sc_rec.PersonID && PsnObj.Search(sc_rec.PersonID, &psn_rec) > 0); // @todo @err
				rBlk.SCardID = sc_rec.ID;
				rBlk.PsnID = sc_rec.PersonID;
				text_identification_done = true;
			}
		}
		if(!text_identification_done) {
			PPIDArray kind_list;
			PPIDArray potential_sc_list;
			THROW_PP(PsnKindID, PPERR_WSCTL_ACCPSNKINDUNDEF); // не удалось определить вид персоналий-клиентов для идентификации по имени
			kind_list.add(PsnKindID);
			(temp_buf = rBlk.LoginText).Transf(CTRANSF_UTF8_TO_INNER); // Строки в rBlk в кодировке utf-8
			THROW_PP(PsnObj.SearchFirstByName(temp_buf, &kind_list, 0, &psn_rec) > 0, PPERR_WSCTL_UNABLERECOGNLOGIN);
			ScObj.P_Tbl->GetListByPerson(psn_rec.ID, ScSerID, &potential_sc_list);
			THROW_PP(potential_sc_list.getCount(), PPERR_WSCTL_NAMENOTLINKEDWITHSC);
			rBlk.SCardID = potential_sc_list.get(0);
			rBlk.PsnID = psn_rec.ID;
			text_identification_done = true;
		}
	}
	if(rBlk.SCardID) {
		PPSCardPacket sc_pack;
		SString pw_buf;
		THROW(ScObj.GetPacket(rBlk.SCardID, &sc_pack) > 0); // @todo @err // Выше мы проверили rBlk.SCardID на "невисячесть"
		sc_pack.GetExtStrData(PPSCardPacket::extssPassword, pw_buf);
		if(rBlk.Pw.NotEmpty()) {
			(temp_buf = rBlk.Pw).Transf(CTRANSF_UTF8_TO_INNER);
			THROW_PP(temp_buf == pw_buf, PPERR_INVUSERORPASSW);
		}
		else {
			THROW_PP(pw_buf.IsEmpty(), PPERR_INVUSERORPASSW);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
	
int WsCtlSrvBlock::SendClientPolicy(SString & rResult)
{
	rResult.Z();
	int    ok = 1;
	SJson * p_js = 0;
	SString temp_buf;
	PPGetPath(PPPATH_WORKSPACE, temp_buf);
	temp_buf.SetLastSlash().Cat("wsctl").SetLastSlash().Cat("wsctl-policy.json");
	p_js = SJson::ParseFile(temp_buf);
	THROW(p_js);
	{
		WsCtl_ClientPolicy policy;
		THROW(policy.FromJsonObj(p_js));
		p_js->ToStr(rResult);
	}
	CATCHZOK
	return ok;
}
