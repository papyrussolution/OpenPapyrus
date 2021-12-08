// DREAMKAS.CPP
// Copyright (c) A.Sobolev 2018, 2019, 2020, 2021
// @codepage UTF-8
// Интерфейс с кассовым порталом DreamKas
//
#include <pp.h>
#pragma hdrstop

class ACS_DREAMKAS : public PPAsyncCashSession {
public:
	ACS_DREAMKAS(PPID id) : PPAsyncCashSession(id)
	{
		GetNodeData(&Acn);
	}
	virtual int ExportData(int updOnly);
	virtual int GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int ImportSession(int);
	virtual int FinishImportSession(PPIDArray *);
	virtual int SetGoodsRestLoadFlag(int updOnly);
	virtual int InteractiveQuery();
protected:
	PPID   StatID;
private:
	struct CashierEntry {
		CashierEntry() : TabNumber(0)
		{
			PTR32(Name)[0] = 0;
			PTR32(INN)[0] = 0;
		}
		long   TabNumber;
		char   Name[128];
		char   INN[16];
	};
	struct SessEntry {
		SessEntry()
		{
			THISZERO();
		}
		enum {
			fClosed = 0x0001 // Призна закрытой сессии
		};
		char    Ident[64]; // ид сессии в дрим-кас (24 шестнадцатиричных символов)
		long    N;  // номер сессии
		long    DeviceN; // номер устройства
		LDATETIME OpenedTime;
		LDATETIME ClosedTime;
		double CashOnOpen;
		double CashOnClose;
		long   Flags;
		CashierEntry Cashier;
	};
	int    ParseGoods(const SJson * pJsonObj, S_GUID & rUuid, SString & rName);
	int    ParseSess(const SJson * pJsonObj, SessEntry & rEntry);
	int    AcceptCheck(const SJson * pJsonObj);
	int    ImportGoodsList(UUIDAssocArray & rList);
	int    SendGoods(SJson ** ppJson, uint & rCount, int update, int force);
	int    ExportGoods(AsyncCashGoodsIterator & rIter, PPID gcAlcID);
	int    PrepareHtmlFields(StrStrAssocArray & rHdrFlds);

	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	PPIDArray SessAry;
	SString PathRpt;
	SString PathFlag;
	SString PathGoods;
	SString PathGoodsFlag;
	int    UseAltImport;
	int    CrdCardAsDsc;
	int    SkipExportingDiscountSchemes;
	int    ImpExpTimeout;
	int    ImportDelay;
	PPObjGoods GObj;
	StringSet ImpPaths;
	StringSet ExpPaths;
	PPAsyncCashNode Acn;
	SString   ImportedFiles;
	struct SessCloseBlock {;
		void   Reset()
		{
			PeriodToCheckQuery.Z();
			SessList.clear();
		}
		STimeChunk PeriodToCheckQuery;
		TSArray <SessEntry> SessList;
	};

	SessCloseBlock Scb;
};

class CM_DREAMKAS : public PPCashMachine {
public:
	CM_DREAMKAS(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * AsyncInterface() { return new ACS_DREAMKAS(NodeID); }
};

REGISTER_CMT(DREAMKAS, 0, 1);

int ACS_DREAMKAS::ParseGoods(const SJson * pJsonObj, S_GUID & rUuid, SString & rName)
{
	rUuid.Z();
	rName.Z();
	int    ok = 1;
	SString temp_buf;
	for(const SJson * p_cur = pJsonObj; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Text.IsEqiAscii("id")) {
			if(p_cur->P_Child) {
				rUuid.FromStr(p_cur->P_Child->Text);
			}
		}
		else if(p_cur->Text.IsEqiAscii("name")) {
			if(p_cur->P_Child)
				rName = (temp_buf = p_cur->P_Child->Text).Unescape();
		}
	}
	if(rName.IsEmpty() || rUuid.IsZero())
		ok = -1;
	return ok;
}

int ACS_DREAMKAS::ParseSess(const SJson * pJsonObj, SessEntry & rEntry)
{
	int    ok = 1;
	for(const SJson * p_cur = pJsonObj; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->P_Child) {
			if(p_cur->Text.IsEqiAscii("id"))
				STRNSCPY(rEntry.Ident, p_cur->P_Child->Text);
			else if(p_cur->Text.IsEqiAscii("shiftId"))
				rEntry.N = p_cur->P_Child->Text.ToLong();
			else if(p_cur->Text.IsEqiAscii("deviceId"))
				rEntry.DeviceN = p_cur->P_Child->Text.ToLong();
			else if(p_cur->Text.IsEqiAscii("openedAt")) // openedAtUTC
				strtodatetime(p_cur->P_Child->Text, &rEntry.OpenedTime, DATF_ISO8601, TIMF_HMS);
			else if(p_cur->Text.IsEqiAscii("closedAt")) // closedAtUTC
				strtodatetime(p_cur->P_Child->Text, &rEntry.ClosedTime, DATF_ISO8601, TIMF_HMS);
			else if(p_cur->Text.IsEqiAscii("cashOnOpen"))
				rEntry.CashOnOpen = p_cur->P_Child->Text.ToReal();
			else if(p_cur->Text.IsEqiAscii("cashOnClose"))
				rEntry.CashOnClose = p_cur->P_Child->Text.ToReal();
			else if(p_cur->Text.IsEqiAscii("isClosed"))
				SETFLAG(rEntry.Flags, rEntry.fClosed, p_cur->P_Child->Text.IsEqiAscii("true"));
		}
	}
	return ok;
}

static const char * P_DreamKasUrlBase = "https://kabinet.dreamkas.ru/api";
static const char * P_Dk_DebugFileName = "dreamkas-debug.log";

int ACS_DREAMKAS::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
{
	int    ok = 1;
	SString temp_buf;
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrContentType, temp_buf);
	{
		int    auth_detected = 0;
		for(uint tagidx = 0; tagidx < Acn.TagL.GetCount(); tagidx++) {
			const ObjTagItem * p_tag_item = Acn.TagL.GetItemByPos(tagidx);
			if(p_tag_item && p_tag_item->TagDataType == OTTYP_OBJLINK && p_tag_item->TagEnumID == PPOBJ_GLOBALUSERACC) {
				PPID   gua_id = p_tag_item->Val.IntVal;
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				if(gua_obj.Search(gua_id, &gua_rec) > 0) { // Fetch использовать нельзя - пароль не извлечется!
					SString pwd;
					SString login;
					Reference::Decrypt(Reference::crymRef2, gua_rec.Password, sstrlen(gua_rec.Password), pwd);
					if(PPRef->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_LOGIN, login) > 0) {
						;
					}
					else
						login = gua_rec.Name;
					if(login.NotEmptyS()) {
						login.Colon().Cat(pwd).Transf(CTRANSF_INNER_TO_UTF8);
						temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
						SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthorization, login.Z().Cat("Basic").Space().Cat(temp_buf));
						pwd.Obfuscate();
						login.Obfuscate();
						auth_detected = 1;
					}
				}
				break;
			}
		}
		THROW_PP(auth_detected, PPERR_UNABLEGETAUTH);
	}
	CATCHZOK
	return ok;
}

int ACS_DREAMKAS::ImportGoodsList(UUIDAssocArray & rList)
{
	rList.clear();
	int    ok = -1;
	SJson * p_json_doc = 0;
	SString temp_buf;
	SString goods_name;
	SString wait_msg_buf;
	SString log_buf;
	ScURL c;
	InetUrl url((temp_buf = P_DreamKasUrlBase).SetLastDSlash().Cat("v2").SetLastDSlash().Cat("products"));
	StrStrAssocArray hdr_flds;
	const  long max_chunk_count = 100;
	long   query_offs = 0;
	long   ret_count = 0;
	PPGetFilePath(PPPATH_LOG, P_Dk_DebugFileName, temp_buf);
	SFile f_out_test(temp_buf, SFile::mAppend);
	THROW(PrepareHtmlFields(hdr_flds));
	PPLoadText(PPTXT_IMPGOODS, wait_msg_buf);
	do {
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		ret_count = 0;
		url.SetComponent(InetUrl::cQuery, temp_buf.Z().CatEq("limit", max_chunk_count).CatChar('&').CatEq("offset", query_offs));
		f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(temp_buf).CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		//
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
				if(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK) {
					long   seq_id = 0;
					S_GUID goods_uuid;
					SJson * p_next = 0;
					for(SJson * p_cur = p_json_doc; p_cur; p_cur = p_next) {
						p_next = p_cur->P_Next;
						switch(p_cur->Type) {
							case SJson::tARRAY:
								p_next = p_cur->P_Child;
								break;
							case SJson::tOBJECT:
								ret_count++;
								if(ParseGoods(p_cur->P_Child, goods_uuid, goods_name) > 0) {
									rList.Add(++seq_id, goods_uuid, 0);
									ok = 1;
								}
								break;
						}
					}
				}
				ZDELETE(p_json_doc);
			}
		}
		query_offs += ret_count;
		PPWaitMsg((temp_buf = wait_msg_buf).Space().Cat(query_offs));
	} while(ret_count > 0 && ret_count == max_chunk_count);
	CATCHZOK
	delete p_json_doc;
	return ok;
}

int ACS_DREAMKAS::SendGoods(SJson ** ppJson, uint & rCount, int update, int force)
{
	int    ok = -1;
	if(rCount && (force || rCount >= 100)) {
		SString temp_buf;
		SString json_buf;
		ScURL c;
		StrStrAssocArray hdr_flds;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW(PrepareHtmlFields(hdr_flds));
		{
			InetUrl url((temp_buf = P_DreamKasUrlBase).SetLastDSlash().Cat("v2").SetLastDSlash().Cat("products"));
			const char * p_debug_file_name = update ? "dreamkas-export-upd-debug.json" : "dreamkas-export-add-debug.json";
			PPGetFilePath(PPPATH_OUT, p_debug_file_name, temp_buf);
			SFile f_out_test(temp_buf, SFile::mAppend);
			THROW_SL((*ppJson)->ToStr(json_buf));
			if(!update) {
				THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			}
			else {
				THROW_SL(c.HttpPatch(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			}
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					f_out_test.WriteLine(temp_buf.CR().CR());
				}
			}
			json_format_string(json_buf, temp_buf.Z());
			f_out_test.WriteLine(temp_buf);
			//
			json_free_value(ppJson);
			if(!force) {
				THROW_MEM(*ppJson = new SJson(SJson::tARRAY));
			}
			rCount = 0;
		}
	}
	CATCHZOK
	return ok;
}

int ACS_DREAMKAS::ExportGoods(AsyncCashGoodsIterator & rIter, PPID gcAlcID)
{
	int    ok = 1;
	SString temp_buf;
	PPObjGoods goods_obj;
	AsyncCashGoodsInfo gds_info;
	PPIDArray rmv_goods_list;
	PrcssrAlcReport::GoodsItem agi;
	UUIDAssocArray ex_goods_list;
	LongArray normal_bc_pos_list;
	LongArray etc_bc_pos_list;
	uint   items_to_create_count = 0;
	uint   items_to_update_count = 0;
	char * p_json_buf = 0;
	SJson * p_iter_ary_to_create = new SJson(SJson::tARRAY);
	SJson * p_iter_ary_to_update = new SJson(SJson::tARRAY);
	THROW_SL(p_iter_ary_to_create);
	THROW_SL(p_iter_ary_to_update);
	THROW(ImportGoodsList(ex_goods_list));
	Acn.GetLogNumList(LogNumList);
	while(rIter.Next(&gds_info) > 0) {
		if(gds_info.GoodsFlags & GF_PASSIV && Acn.ExtFlags & CASHFX_RMVPASSIVEGOODS && gds_info.Rest <= 0.0) {
			rmv_goods_list.addUnique(gds_info.ID);
		}
		else {
			long   level = 0;
			PPID   dscnt_scheme_id = 0;
			/*if(gcAlcID && gc_alc_code.NotEmpty() && gds_info.GdsClsID == gcAlcID) {
				alc_goods_list.add(gds_info.ID);
			}*/
			if(rIter.GetAlcoGoodsExtension(gds_info.ID, 0, agi) > 0) {
			}
			else {
			}
			{
				int    is_wght = 0; // @v10.8.4 Признак весового товара
				SJson * p_iter_obj = new SJson(SJson::tOBJECT);
				THROW_SL(p_iter_obj);
				THROW_SL(p_iter_obj->InsertString("id", temp_buf.Z().Cat(gds_info.Uuid, S_GUID::fmtIDL)));
				THROW_SL(p_iter_obj->InsertString("name", temp_buf.Z().Cat(gds_info.Name).Escape().Transf(CTRANSF_INNER_TO_UTF8)));
				// @v10.8.4 {
				{
					const PPGoodsConfig & gcfg = GetGoodsCfg();
					if(gds_info.P_CodeList) {
						for(uint i = 0; !is_wght && i < gds_info.P_CodeList->getCount(); i++) {
							const BarcodeTbl::Rec & r_bc_item = gds_info.P_CodeList->at(i);
							if(strlen(r_bc_item.Code) > 3 && gcfg.IsWghtPrefix(r_bc_item.Code))
								is_wght = 1;
						}
					}
				}
				// } @v10.8.4 
				THROW_SL(p_iter_obj->InsertString("type", is_wght ? "SCALABLE" : "COUNTABLE"));
				THROW_SL(p_iter_obj->Insert("departmentId", /*json_new_number(temp_buf.Z().Cat(gds_info.DivN))*/new SJson(SJson::tNULL)));
				THROW_SL(p_iter_obj->Insert("quantity", json_new_number(temp_buf.Z().Cat(1000))));
				THROW_SL(p_iter_obj->Insert("price", json_new_number(temp_buf.Z().Cat((long)(gds_info.Price * 100.0)))));
				if(LogNumList.getCount()) {
					SJson * p_price_ary = new SJson(SJson::tARRAY);
					THROW_SL(p_price_ary);
					for(uint i = 0; i < LogNumList.getCount(); i++) {
						SJson * p_price_obj = new SJson(SJson::tOBJECT);
						THROW_SL(p_price_obj);
						THROW_SL(p_price_obj->Insert("deviceId", json_new_number(temp_buf.Z().Cat(LogNumList.get(i)))));
						THROW_SL(p_price_obj->Insert("value", json_new_number(temp_buf.Z().Cat((long)(gds_info.Price * 100.0)))));
						THROW_SL(json_insert_child(p_price_ary, p_price_obj));
					}
					THROW_SL(p_iter_obj->Insert("prices", p_price_ary));
				}
				if(gds_info.P_CodeList) {
					normal_bc_pos_list.clear();
					etc_bc_pos_list.clear();
					for(uint i = 0; i < gds_info.P_CodeList->getCount(); i++) {
						const BarcodeTbl::Rec & r_bc_item = gds_info.P_CodeList->at(i);
						int    d = 0;
						int    std = 0;
						if(goods_obj.DiagBarcode(r_bc_item.Code, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE)) {
							normal_bc_pos_list.add(i+1);
						}
						else {
							etc_bc_pos_list.add(i+1);
						}
					}
					if(normal_bc_pos_list.getCount()) {
						SJson * p_array = new SJson(SJson::tARRAY);
						for(uint j = 0; j < normal_bc_pos_list.getCount(); j++) {
							const char * p_code = gds_info.P_CodeList->at(normal_bc_pos_list.get(j)-1).Code;
							THROW_SL(json_insert_child(p_array, json_new_string(p_code)));
						}
						p_iter_obj->Insert("barcodes", p_array);
					}
					{
						SJson * p_array = new SJson(SJson::tARRAY);
						THROW_SL(json_insert_child(p_array, json_new_string(temp_buf.Z().Cat(gds_info.ID))));
						if(etc_bc_pos_list.getCount()) {
							for(uint j = 0; j < etc_bc_pos_list.getCount(); j++) { // @v10.7.1 @fix normal_bc_pos_list-->etc_bc_pos_list
								(temp_buf = gds_info.P_CodeList->at(etc_bc_pos_list.get(j)-1).Code).Escape().Transf(CTRANSF_INNER_TO_UTF8);
								THROW_SL(json_insert_child(p_array, json_new_string(temp_buf)));
							}
						}
						THROW_SL(p_iter_obj->Insert("vendorCodes", p_array));
					}
				}
				{
					long   seq_id = 0;
					if(ex_goods_list.SearchVal(gds_info.Uuid, &seq_id, 0)) {
						THROW_SL(json_insert_child(p_iter_ary_to_update, p_iter_obj));
						items_to_update_count++;
						THROW(SendGoods(&p_iter_ary_to_update, items_to_update_count, 1, 0));
					}
					else {
						THROW_SL(json_insert_child(p_iter_ary_to_create, p_iter_obj));
						items_to_create_count++;
						THROW(SendGoods(&p_iter_ary_to_create, items_to_create_count, 0, 0));
					}
				}
			}
		}
		PPWaitPercent(rIter.GetIterCounter());
	}
	THROW(SendGoods(&p_iter_ary_to_update, items_to_update_count, 1, 1));
	THROW(SendGoods(&p_iter_ary_to_create, items_to_create_count, 0, 1));
	CATCHZOK
	delete p_iter_ary_to_create;
	delete p_iter_ary_to_update;
	ZFREE(p_json_buf);
	return ok;
}

/*virtual*/int ACS_DREAMKAS::ExportData(int updOnly)
{
	int    ok = 1;
	//
	ScURL c;
	const char * p_user = "";
	const char * p_password = "";

	int    next_barcode = 0;
	uint   i;
	LAssocArray  grp_n_level_ary;
	SString   f_str;
	SString   tail;
	SString   temp_buf;
	SString   email_subj;
	SString   path_goods;
	SString   path_flag;
	//
	PPID      gc_alc_id = 0;
	SString   gc_alc_code; // Код класса товаров, относящихся к алкоголю
	PPIDArray alc_goods_list;
	//
	PPObjQuotKind qk_obj;
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPSCardSerPacket scs_pack;
	PPObjGoods goods_obj;
	PPObjGoodsClass gc_obj;
	LAssocArray  scard_quot_list;
	PPIDArray retail_quot_list;
	PPIDArray scard_series_list;
	BitArray used_retail_quot;
	PPIniFile ini_file;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	//
	// Извлечем информацию о классе алкогольного товара
	//
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSALC, temp_buf) > 0 && temp_buf.NotEmptyS()) {
		StringSet ss(',', temp_buf);
		ss.get(&(i = 0), temp_buf.Z());
		if(gc_obj.SearchBySymb(temp_buf, &gc_alc_id) > 0) {
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(gc_alc_id, &gc_pack) > 0) {
				gc_alc_id = gc_pack.Rec.ID;
				(temp_buf = gc_pack.Rec.Symb).Strip();
				//
				// Если код товарного класса числовой, то используем для загрузки его, иначе - идентификатор
				// (фронтол требует цифрового кода, но использование идентификатора не желательно из-за возможного разнобоя между разделами БД).
				//
				if(temp_buf.ToLong() > 0)
					gc_alc_code = temp_buf;
				else
					gc_alc_code.Z().Cat(gc_alc_id);
			}
		}
	}
	// }
	THROW_MEM(SETIFZ(P_Dls, new DeviceLoadingStat));
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	PPWaitStart();
	{
		qk_obj.GetRetailQuotList(ZERODATETIME, &retail_quot_list, 0);
		used_retail_quot.insertN(0, retail_quot_list.getCount());
	}
	if(updOnly || (Flags & PPACSF_LOADRESTWOSALES)) {
	}
	else {
	}
	{
		SJson jdoc(SJson::tOBJECT);
		long   acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		if(Acn.ExtFlags & CASHFX_EXPLOCPRNASSOC)
			acgif |= ACGIF_INITLOCPRN;
		acgif |= (ACGIF_ALLCODESPERITER|ACGIF_ENSUREUUID);
		AsyncCashGoodsIterator goods_iter(NodeID, acgif, SinceDlsID, P_Dls);
		{
			AsyncCashGoodsInfo gds_info;
			PPIDArray rmv_goods_list;
			PrcssrAlcReport::GoodsItem agi;
			for(i = 0; i < retail_quot_list.getCount(); i++) {
				gds_info.QuotList.Add(retail_quot_list.get(i), 0, 1);
			}
			THROW(ExportGoods(goods_iter, gc_alc_id));
			//
			// Список товаров на удаление.
			//
			if(rmv_goods_list.getCount()) {
				for(i = 0; i < rmv_goods_list.getCount(); i++) {
					const PPID goods_id_to_remove = rmv_goods_list.get(i);
				}
			}
			//
			// Список алкогольных товаров
			//
			if(alc_goods_list.getCount()) {
				alc_goods_list.sortAndUndup();
				for(i = 0; i < alc_goods_list.getCount(); i++) {
					;
				}
			}
		}
	}
	PPWaitStop();
	PPWaitStart();
	//
	// Здесь отправить данные на сервер
	//
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCHZOK
	PPWaitStop();
	return ok;
}

/*virtual*/int ACS_DREAMKAS::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd)
{ 
	Scb.Reset();
	int    ok = -1;
	SJson * p_json_doc = 0;
	SString temp_buf;
	SString qbuf;
	SString enc_buf;
	SString wait_msg_buf;
	SString log_buf;
	ScURL c;
	InetUrl url((temp_buf = P_DreamKasUrlBase).SetLastDSlash().Cat("shifts"));
	StrStrAssocArray hdr_flds;
	const  long max_chunk_count = 100;
	long   query_offs = 0;
	long   ret_count = 0;
	PPGetFilePath(PPPATH_LOG, P_Dk_DebugFileName, temp_buf);
	SFile f_out_test(temp_buf, SFile::mAppend);
	Acn.GetLogNumList(LogNumList);
	THROW(PrepareHtmlFields(hdr_flds));
	do {
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		ret_count = 0;
		qbuf.Z();
		{
			LDATETIME dtm;
			dtm.Set(((pPrd && pPrd->low) ? pPrd->low : encodedate(1, 4, 2018)), ZEROTIME);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0).Cat(".000Z");
			qbuf.CatEq("from", enc_buf.EncodeUrl(temp_buf, 1));
		}
		{
			LDATETIME dtm;
			dtm.Set(((pPrd && pPrd->upp) ? pPrd->upp : getcurdate_()), MAXDAYTIME);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0).Cat(".000Z");
			qbuf.CatChar('&').CatEq("to", enc_buf.EncodeUrl(temp_buf, 1));
		}
		/*
		if(LogNumList.getCount()) {
			temp_buf.Z();
			for(uint i = 0; i < LogNumList.getCount(); i++) {
				if(i)
					temp_buf.CatChar(',');
				temp_buf.Cat(LogNumList.get(i));
			}
			qbuf.CatChar('&').CatEq("devices", enc_buf.EncodeUrl(temp_buf, 1));
		}
		*/
		//qbuf.CatChar('&').CatEq("is_closed", "true");
		qbuf.CatChar('&').CatEq("limit", max_chunk_count).CatChar('&').CatEq("offset", query_offs);
		url.SetComponent(InetUrl::cQuery, qbuf);
		f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(qbuf).CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		//
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
				if(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK) {
					SJson * p_next = 0;
					for(SJson * p_cur = p_json_doc; p_cur; p_cur = p_next) {
						p_next = p_cur->P_Next;
						switch(p_cur->Type) {
							case SJson::tARRAY:
								p_next = p_cur->P_Child;
								break;
							case SJson::tOBJECT:
								ret_count++;
								{
									SessEntry sess_entry;
									if(ParseSess(p_cur->P_Child, sess_entry)) {
										Scb.SessList.insert(&sess_entry);
										LDATETIME open_dtm = sess_entry.ClosedTime; // @v10.7.3
										LDATETIME close_dtm = sess_entry.OpenedTime; // @v10.7.3
										if(!!open_dtm) {
											if(!Scb.PeriodToCheckQuery.Start || cmp(Scb.PeriodToCheckQuery.Start, open_dtm) > 0)
												Scb.PeriodToCheckQuery.Start = open_dtm;
										}
										if(!!close_dtm) {
											if(!Scb.PeriodToCheckQuery.Finish || cmp(Scb.PeriodToCheckQuery.Finish, close_dtm) < 0)
												Scb.PeriodToCheckQuery.Finish = close_dtm;
										}
										ok = 1;
									}
								}
								break;
						}
					}
				}
				ZDELETE(p_json_doc);
			}
		}
		query_offs += ret_count;
		PPWaitMsg((temp_buf = wait_msg_buf).Space().Cat(query_offs));
	} while(ret_count > 0 && ret_count == max_chunk_count);
	if(Scb.SessList.getCount())
		ok = 1;
	ASSIGN_PTR(pSessCount, Scb.SessList.getCountI());
	ASSIGN_PTR(pIsForwardSess, 0);
	CATCHZOK
	delete p_json_doc;
	return ok;
}

int ACS_DREAMKAS::AcceptCheck(const SJson * pJsonObj)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPID   cc_id = 0;
	long   cc_flags = 0;
	long   foreign_cc_id = 0;
	int    cc_type = 0; // 1 - sale, 2 - REFUND, 3 - OUTFLOW, 4 - OUTFLOW_REFUND
	long   cc_number = 0;
	LDATETIME cc_dtm = ZERODATETIME;
	double cc_amount = 0.0;
	double cc_discount = 0.0;
	long   device_id = 0;
	long   shop_id = 0;
	long   sess_n = 0;
	PPID   cashier_id = 0;
	PPID   sc_id = 0;
	SString operation_ident;
	S_GUID goods_uuid;
	SString goods_name;
	SString barcode;
	SString excise_barcode;
	SString vendor_code;
	PPIDArray goods_by_uuid_list;
	const SJson * p_positions_ary = 0;
	const SJson * p_payms_ary = 0;
	for(const SJson * p_cur = pJsonObj; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tARRAY) {
		}
		else if(p_cur->Type == SJson::tOBJECT) {
		}
		else {
			if(p_cur->P_Child) {
				if(p_cur->Text.IsEqiAscii("id"))
					foreign_cc_id = p_cur->P_Child->Text.ToLong();
				else if(p_cur->Text.IsEqiAscii("type")) {
					if(p_cur->P_Child->Text.IsEqiAscii("SALE"))
						cc_type = 1;
					else if(p_cur->P_Child->Text.IsEqiAscii("REFUND")) {
						cc_type = 2;
						cc_flags |= CCHKF_RETURN;
					}
					else if(p_cur->P_Child->Text.IsEqiAscii("OUTFLOW"))
						cc_type = 3;
					else if(p_cur->P_Child->Text.IsEqiAscii("OUTFLOW_REFUND"))
						cc_type = 4;
				}
				else if(p_cur->Text.IsEqiAscii("amount"))
					cc_amount = fdiv100i(p_cur->P_Child->Text.ToLong());
				else if(p_cur->Text.IsEqiAscii("discount"))
					cc_discount = fdiv100i(p_cur->P_Child->Text.ToLong());
				else if(p_cur->Text.IsEqiAscii("deviceId"))
					device_id = p_cur->P_Child->Text.ToLong();
				else if(p_cur->Text.IsEqiAscii("shopId"))
					shop_id = p_cur->P_Child->Text.ToLong();
				else if(p_cur->Text.IsEqiAscii("operationId"))
					operation_ident = p_cur->P_Child->Text;
				else if(p_cur->Text.IsEqiAscii("shiftId"))
					sess_n = p_cur->P_Child->Text.ToLong();
				else if(p_cur->Text.IsEqiAscii("number"))
					cc_number = p_cur->P_Child->Text.ToLong();
				else if(p_cur->Text.IsEqiAscii("localDate"))
					strtodatetime(p_cur->P_Child->Text, &cc_dtm, DATF_ISO8601, TIMF_HMS);
				else if(p_cur->Text.IsEqiAscii("date"))
					;
				else if(p_cur->Text.IsEqiAscii("positions")) {
					const SJson * p_ary = p_cur->P_Child;
					if(p_ary->Type == SJson::tARRAY)
						p_positions_ary = p_ary;
				}
				else if(p_cur->Text.IsEqiAscii("payments")) {
					const SJson * p_ary = p_cur->P_Child;
					if(p_ary->Type == SJson::tARRAY)
						p_payms_ary = p_ary;
				}
			}
		}
	}
	if(Scb.SessList.lsearch(&sess_n, 0, CMPF_LONG, offsetof(SessEntry, N))) {
		int    ccr = 0;
		// @v10.7.3 {
		if(cc_flags & CCHKF_RETURN) {
			cc_amount = -cc_amount;
			cc_discount = -cc_discount;
		}
		// } @v10.7.3 
		THROW(ccr = AddTempCheck(&cc_id, sess_n, cc_flags, device_id, cc_number, cashier_id, sc_id, cc_dtm, cc_amount, cc_discount));
		if(ccr > 0) {
			assert(cc_id);
			if(p_positions_ary) {
				for(const SJson * p_pos = p_positions_ary->P_Child; p_pos; p_pos = p_pos->P_Next) {
					if(p_pos->Type == SJson::tOBJECT) {
						goods_uuid.Z();
						goods_name.Z();
						barcode.Z();
						excise_barcode.Z();
						vendor_code.Z();
						PPID   goods_id = 0;
						long   depart_no = 0;
						double qtty = 0.0;
						double price = 0.0;
						double dscnt = 0.0;
						for(const SJson * p_obj = p_pos->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->P_Child) {
								if(p_obj->Text.IsEqiAscii("id"))
									goods_uuid.FromStr(p_obj->P_Child->Text);
								else if(p_obj->Text.IsEqiAscii("name"))
									(goods_name = p_obj->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
								else if(p_obj->Text.IsEqiAscii("type")) 
									;
								else if(p_obj->Text.IsEqiAscii("quantity"))
									qtty = fdiv1000i(p_obj->P_Child->Text.ToLong());
								else if(p_obj->Text.IsEqiAscii("price"))
									price = fdiv100i(p_obj->P_Child->Text.ToLong());
								else if(p_obj->Text.IsEqiAscii("discount"))
									dscnt = fdiv100i(p_obj->P_Child->Text.ToLong());
								else if(p_obj->Text.IsEqiAscii("barcode"))
									(barcode = p_obj->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
								else if(p_obj->Text.IsEqiAscii("exciseBarcode"))
									(excise_barcode = p_obj->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
								else if(p_obj->Text.IsEqiAscii("vendorCode"))
									(vendor_code = p_obj->P_Child->Text).Transf(CTRANSF_UTF8_TO_INNER);
								else if(p_obj->Text.IsEqiAscii("tax")) {
								}
								else if(p_obj->Text.IsEqiAscii("departmentId"))
									depart_no = p_obj->P_Child->Text.ToLong();
							}
						}
						if(!!goods_uuid) {
							ObjTagItem tag;
							PPIDArray id_list;
							if(tag.SetGuid(PPTAG_BILL_UUID, &goods_uuid)) {
								if(p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_GOODS, PPTAG_GOODS_UUID, tag.Val.PStr, &goods_by_uuid_list) > 0) {
									if(goods_by_uuid_list.getCount()) {
										if(goods_by_uuid_list.getCount() == 1) {
											goods_id = goods_by_uuid_list.get(0);
										}
										else {
											; // @todo message(ambiguity)
											goods_id = goods_by_uuid_list.get(0);
										}
									}
								}
							}
						}
						else {
							; // @todo message
						}
						//rCcPack.InsertItem(goods_id, qtty, price, discount, depart_no, 0);
						qtty = (cc_flags & CCHKF_RETURN) ? -fabs(qtty) : fabs(qtty); // @v10.7.3
						SetupTempCcLineRec(0, cc_id, cc_number, cc_dtm.d, 0/*div_n*/, goods_id);
						// @v10.7.3 SetTempCcLineValues(0, qtty, price, dscnt, 0/*pLnExtStrings*/);
						// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
						THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, qtty, price, dscnt, 0/*pLnExtStrings*/)); // @v10.7.3
					}
				}
			}
			if(p_payms_ary) {
				for(const SJson * p_pos = p_payms_ary->P_Child; p_pos; p_pos = p_pos->P_Next) {
					if(p_pos->Type == SJson::tOBJECT) {
						int    paym_type = 0;
						double paym_amt = 0.0;
						PPID   paym_sc_id = 0;
						for(const SJson * p_obj = p_pos->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->P_Child) {
								if(p_obj->Text.IsEqiAscii("type")) {
									if(p_obj->P_Child->Text.IsEqiAscii("CASH"))
										paym_type = CCAMTTYP_CASH;
									else if(p_obj->P_Child->Text.IsEqiAscii("CASHLESS"))
										paym_type = CCAMTTYP_BANK;
									else
										paym_type = CCAMTTYP_CASH; // @default=cash
								}
								else if(p_obj->Text.IsEqiAscii("amount")) {
									paym_amt = fdiv100i(p_obj->P_Child->Text.ToLong());
								}
							}
						}
						// @v10.7.3 {
						if(cc_flags & CCHKF_RETURN)
							paym_amt = -paym_amt;
						// } @v10.7.3 
						THROW(AddTempCheckPaym(cc_id, paym_type, paym_amt, paym_sc_id));
					}
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int ACS_DREAMKAS::InteractiveQuery()
{
	int    ok = -1;
	/*
	TSVector <PPPosProtocol::QueryBlock> qb_list;
	if(PPPosProtocol::EditPosQuery(qb_list) > 0) {
		for(uint i = 0; i < qb_list.getCount(); i++) {
			THROW(Pp.SendQuery(NodeID, qb_list.at(i)));
		}
	}
	CATCHZOKPPERR
	*/
	return ok;
}

/*virtual*/int ACS_DREAMKAS::ImportSession(int sessIdx)
{ 
	int    ok = -1;
	SJson * p_json_doc = 0;
	SString temp_buf;
	SString qbuf;
	SString enc_buf;
	SString log_buf;
	ScURL c;
	InetUrl url((temp_buf = P_DreamKasUrlBase).SetLastDSlash().Cat("receipts"));
	StrStrAssocArray hdr_flds;
	const  long max_chunk_count = 1000;
	long   query_offs = 0;
	long   ret_count = 0;
	if(sessIdx < Scb.SessList.getCountI() && sessIdx == 0) {
		PPGetFilePath(PPPATH_LOG, P_Dk_DebugFileName, temp_buf);
		SFile f_out_test(temp_buf, SFile::mAppend);
		THROW(CreateTables());
		Acn.GetLogNumList(LogNumList);
		THROW(PrepareHtmlFields(hdr_flds));
		do {
			SBuffer ack_buf;
			SBuffer * p_ack_buf = 0;
			SFile wr_stream(ack_buf, SFile::mWrite);
			LDATETIME dtm;
			ret_count = 0;
			qbuf.Z();
			if(!!Scb.PeriodToCheckQuery.Start)
				dtm = Scb.PeriodToCheckQuery.Start;
			else
				dtm.Set(encodedate(1, 4, 2018), ZEROTIME);
			qbuf.CatEq("from", enc_buf.EncodeUrl(temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0), 1));
			/*if(!!Scb.PeriodToCheckQuery.Finish)
				dtm = Scb.PeriodToCheckQuery.Finish;
			else
				dtm.Set(getcurdate_(), MAXDAYTIME);*/
			dtm.Set(getcurdate_(), MAXDAYTIME);
			qbuf.CatChar('&').CatEq("to", enc_buf.EncodeUrl(temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0), 1));
			/*if(LogNumList.getCount()) {
				temp_buf.Z();
				for(uint i = 0; i < LogNumList.getCount(); i++) {
					if(i)
						temp_buf.CatChar(',');
					temp_buf.Cat(LogNumList.get(i));
				}
				qbuf.CatChar('&').CatEq("devices", enc_buf.EncodeUrl(temp_buf, 1));
			}*/
			qbuf.CatChar('&').CatEq("limit", max_chunk_count).CatChar('&').CatEq("offset", query_offs);
			url.SetComponent(InetUrl::cQuery, qbuf);
			f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(qbuf).CR());
			THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			//
			p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSizeI());
				f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
				THROW(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
				{
					SJson * p_next = 0;
					PPTransaction tra(1);
					THROW(tra);
					for(SJson * p_cur = p_json_doc->P_Child; p_cur; p_cur = p_next) {
						p_next = p_cur->P_Next;
						if(p_cur->Type == SJson::tSTRING) {
							if(p_cur->Text.IsEqiAscii("data"))
								p_next = p_cur->P_Child;
						}
						else if(p_cur->Type == SJson::tARRAY)
							p_next = p_cur->P_Child;
						else if(p_cur->Type == SJson::tOBJECT) {
							ret_count++;
							AcceptCheck(p_cur->P_Child);
						}
					}
					THROW(tra.Commit());
					ok = 1;
				}
				ZDELETE(p_json_doc);
			}
			query_offs += ret_count;
		} while(ret_count > 0 && ret_count == max_chunk_count);
	}
	CATCHZOK
	delete p_json_doc;
	return ok;
}

/*virtual*/int ACS_DREAMKAS::FinishImportSession(PPIDArray *) { return -1; }
/*virtual*/int ACS_DREAMKAS::SetGoodsRestLoadFlag(int updOnly) { return -1; }
