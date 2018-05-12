// DREAMKAS.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Интерфейс с кассовым порталом DreamKas
//
#include <pp.h>
#pragma hdrstop

class ACS_DREAMKAS : public PPAsyncCashSession {
public:
	SLAPI  ACS_DREAMKAS(PPID id) : PPAsyncCashSession(id)
	{
		GetNodeData(&Acn);
	}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);
	virtual int SLAPI SetGoodsRestLoadFlag(int updOnly);
protected:
	PPID   StatID;
private:
	int    SLAPI ConvertWareList(const char * pImpPath);
	int    SLAPI ExportGoods(AsyncCashGoodsIterator & rIter, const PPAsyncCashNode & rCnData, PPID gcAlcID);
	int    SLAPI MakeAuthField(SString & rBuf);

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
	StringSet ImpPaths;
	StringSet ExpPaths;
	PPAsyncCashNode Acn;
	SString   ImportedFiles;
};

class CM_DREAMKAS : public PPCashMachine {
public:
	SLAPI CM_DREAMKAS(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_DREAMKAS(NodeID); }
};

REGISTER_CMT(DREAMKAS, 0, 1);

int SLAPI ACS_DREAMKAS::MakeAuthField(SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	for(uint tagidx = 0; tagidx < Acn.TagL.GetCount(); tagidx++) {
		const ObjTagItem * p_tag_item = Acn.TagL.GetItemByPos(tagidx);
		if(p_tag_item && p_tag_item->TagDataType == OTTYP_OBJLINK && p_tag_item->TagEnumID == PPOBJ_GLOBALUSERACC) {
			PPID gua_id = p_tag_item->Val.IntVal;
			PPObjGlobalUserAcc gua_obj;

			PPGlobalUserAcc gua_rec;
			if(gua_obj.Search(gua_id, &gua_rec) > 0) { // Fetch использовать нельзя - пароль не извлечется!
				SString pwd;
				SString login;
				SString temp_buf;
				Reference::Decrypt(Reference::crymRef2, gua_rec.Password, sstrlen(gua_rec.Password), pwd);
				if(PPRef->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_LOGIN, login) > 0) {
					;
				}
				else {
					login = gua_rec.Name;
				}
				if(login.NotEmptyS()) {
					login.CatChar(':').Cat(pwd).Transf(CTRANSF_INNER_TO_UTF8);
					temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
					rBuf.Cat("Basic").Space().Cat(temp_buf);
					pwd.Obfuscate();
					login.Obfuscate();
					ok = 1;
				}
			}
			break;
		}
	}
	return ok;
}

int SLAPI ACS_DREAMKAS::ExportGoods(AsyncCashGoodsIterator & rIter, const PPAsyncCashNode & rCnData, PPID gcAlcID)
{
	int    ok = 1;
	SString temp_buf;
	PPObjGoods goods_obj;
	AsyncCashGoodsInfo gds_info;
	PPIDArray rmv_goods_list;
	PrcssrAlcReport::GoodsItem agi;
	json_t * p_iter_ary = new json_t(json_t::tARRAY);
	THROW_SL(p_iter_ary);
	while(rIter.Next(&gds_info) > 0) {
		if(gds_info.GoodsFlags & GF_PASSIV && rCnData.ExtFlags & CASHFX_RMVPASSIVEGOODS && gds_info.Rest <= 0.0) {
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
				json_t * p_iter_obj = new json_t(json_t::tOBJECT);
				THROW_SL(p_iter_obj);
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "id", json_new_string(temp_buf.Z().Cat(gds_info.Uuid, S_GUID::fmtIDL))));
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "name", json_new_string(temp_buf.Z().Cat(gds_info.Name).Escape().Transf(CTRANSF_INNER_TO_UTF8))));
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "type", json_new_string("COUNTABLE")));
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "departmentId", json_new_number(temp_buf.Z().Cat(gds_info.DivN))));
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "quantity", json_new_number(temp_buf.Z().Cat(1000))));
				THROW_SL(json_insert_pair_into_object(p_iter_obj, "price", json_new_number(temp_buf.Z().Cat((long)(gds_info.Price * 100.0)))));
				if(gds_info.P_CodeList) {
					LongArray normal_bc_pos_list;
					LongArray etc_bc_pos_list;
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
						json_t * p_array = new json_t(json_t::tARRAY);
						for(uint j = 0; j < normal_bc_pos_list.getCount(); j++) {
							const char * p_code = gds_info.P_CodeList->at(normal_bc_pos_list.get(j)-1).Code;
							THROW_SL(json_insert_child(p_array, json_new_string(p_code)));
						}
						json_insert_pair_into_object(p_iter_obj, "barcodes", p_array);
					}
					{
						json_t * p_array = new json_t(json_t::tARRAY);
						THROW_SL(json_insert_child(p_array, json_new_string(temp_buf.Z().Cat(gds_info.ID))));
						if(etc_bc_pos_list.getCount()) {
							for(uint j = 0; j < normal_bc_pos_list.getCount(); j++) {
								(temp_buf = gds_info.P_CodeList->at(normal_bc_pos_list.get(j)-1).Code).Escape().Transf(CTRANSF_INNER_TO_UTF8);
								THROW_SL(json_insert_child(p_array, json_new_string(temp_buf)));
							}
						}
						THROW_SL(json_insert_pair_into_object(p_iter_obj, "vendorCodes", p_array));
					}
				}
				THROW_SL(json_insert_child(p_iter_ary, p_iter_obj));
			}
		}
		PPWaitPercent(rIter.GetIterCounter());
	}
	{
		PPGetFilePath(PPPATH_OUT, "dreamkas-export-debug.json", temp_buf);
		SFile f_out_test(temp_buf, SFile::mWrite);
		char * p_json_buf = 0;
		json_tree_to_string(p_iter_ary, &p_json_buf);
		{
			ScURL c;
			SBuffer ack_buf;
			InetUrl url("https://kabinet.dreamkas.ru/api/v2/products/id");
			SFile wr_stream(ack_buf, SFile::mWrite);
			ScURL::HttpForm hf;
			StrStrAssocArray hdr_flds;
			
			{
				SFileFormat::GetMime(SFileFormat::Json, temp_buf);
				hdr_flds.Add("Content-Type", temp_buf);
				//
				THROW(MakeAuthField(temp_buf));
				hdr_flds.Add("Authorization", temp_buf);
			}
			THROW_SL(c.HttpPatch(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, p_json_buf, &wr_stream));
			//
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					f_out_test.WriteLine(temp_buf.CR().CR());
				}
			}
		}
		json_format_string(p_json_buf, temp_buf.Z());
		f_out_test.WriteLine(temp_buf);
		SAlloc::F(p_json_buf);
	}
	CATCHZOK
	json_free_value(&p_iter_ary);
	return ok;
}

//virtual 
int SLAPI ACS_DREAMKAS::ExportData(int updOnly)
{
	int    ok = 1;
	//
	ScURL c;
	const char * p_url_base = "https://kabinet.dreamkas.ru/api";
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
	//PPUnit    unit_rec;
	//PPObjUnit unit_obj;
	PPObjQuotKind qk_obj;
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPSCardSerPacket scs_pack;
	PPObjGoods goods_obj;
	PPObjGoodsClass gc_obj;
	PPAsyncCashNode cn_data;
	LAssocArray  scard_quot_list;
	PPIDArray retail_quot_list;
	PPIDArray scard_series_list;
	BitArray used_retail_quot;
	PPIniFile ini_file;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	THROW(GetNodeData(&cn_data) > 0);
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
	PPWait(1);
	{
		qk_obj.GetRetailQuotList(ZERODATETIME, &retail_quot_list, 0);
		used_retail_quot.insertN(0, retail_quot_list.getCount());
	}
	if(updOnly || (Flags & PPACSF_LOADRESTWOSALES)) {
	}
	else {
	}
	{
		json_t jdoc(json_t::tOBJECT);
		long   acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		if(cn_data.ExtFlags & CASHFX_EXPLOCPRNASSOC)
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
			THROW(ExportGoods(goods_iter, cn_data, gc_alc_id));
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
	PPWait(0);
	PPWait(1);
	//
	// Здесь отправить данные на сервер
	//
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCHZOK
	PPWait(0);
	return ok;
}

//virtual 
int SLAPI ACS_DREAMKAS::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::ImportSession(int)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::FinishImportSession(PPIDArray *)
{
	return -1;
}
//virtual 
int SLAPI ACS_DREAMKAS::SetGoodsRestLoadFlag(int updOnly)
{
	return -1;
}
