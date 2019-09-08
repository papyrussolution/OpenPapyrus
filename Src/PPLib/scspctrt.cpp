// SCSPCTRT.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Реализация специальных интерпретаций поведения персональных карт
//
#include <pp.h>
#pragma hdrstop

SLAPI SCardSpecialTreatment::CardBlock::CardBlock() : SpecialTreatment(0), PosNodeID(0)
{
}

SLAPI SCardSpecialTreatment::DiscountBlock::DiscountBlock() : RowN(0), GoodsID(0), Flags(0), Qtty(0.0), InPrice(0.0), ResultPrice(0.0)
{
	PTR32(TaIdent)[0] = 0;
}

SLAPI SCardSpecialTreatment::SCardSpecialTreatment()
{
}

SLAPI SCardSpecialTreatment::~SCardSpecialTreatment()
{
}

int SLAPI SCardSpecialTreatment::VerifyOwner(const CardBlock * pScBlk)
	{ return -1; }
int SLAPI SCardSpecialTreatment::DoesWareBelongToScope(PPID goodsID)
	{ return 0; }
int SLAPI SCardSpecialTreatment::QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList)
	{ return -1; }
int SLAPI SCardSpecialTreatment::CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, long * pRetFlags)
	{ return -1; }

//static
int FASTCALL SCardSpecialTreatment::InitSpecialCardBlock(PPID scID, PPID posNodeID, SCardSpecialTreatment::CardBlock & rBlk)
{
	int    ok = -1;
	rBlk.SpecialTreatment = 0;
	rBlk.PosNodeID = 0;
	rBlk.PosNodeCode.Z();
	rBlk.ScPack.Z();
	PPObjSCard sc_obj;
	if(sc_obj.GetPacket(scID, &rBlk.ScPack) > 0) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Search(rBlk.ScPack.Rec.SeriesID, &scs_rec) > 0 && scs_rec.SpecialTreatment) {
			rBlk.SpecialTreatment = scs_rec.SpecialTreatment;
			rBlk.PosNodeID = posNodeID;
			if(posNodeID) {
				if(PPRef->Ot.GetTagStr(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_UUID, rBlk.PosNodeCode) > 0) { // @test
					ok = 1;
				}
				/* @real
				S_GUID pos_uuid;
				if(PPRef->Ot.GetTagGuid(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_UUID, pos_uuid) > 0) {
					pos_uuid.ToStr(S_GUID::fmtIDL, cb.PosNodeCode);
					ok = 1;
				}
				*/
			}
		}
	}
	return ok;
}

class SCardSpecialTreatment_AstraZeneca : public SCardSpecialTreatment {
public:
	SLAPI SCardSpecialTreatment_AstraZeneca() : SCardSpecialTreatment()
	{
	}
	virtual SLAPI ~SCardSpecialTreatment_AstraZeneca()
	{
	}
	virtual int SLAPI VerifyOwner(const CardBlock * pScBlk);
	virtual int SLAPI DoesWareBelongToScope(PPID goodsID);
	virtual int SLAPI QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList);
	virtual int SLAPI CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, long * pRetFlags);
private:
	void SLAPI MakeUrl(const char * pSuffix, SString & rBuf)
	{
		//https://astrazeneca.like-pharma.com/api/1.0/register/
		SString sfx;
		sfx = "api/1.0/";
		if(!isempty(pSuffix))
			sfx.Cat(pSuffix).SetLastDSlash();
		rBuf = InetUrl::MkHttps("astrazeneca.like-pharma.com", sfx);
	}
	int SLAPI PrepareHtmlFields(StrStrAssocArray & rHdrFlds);
};

static const char * P_Az_DebugFileName = "astrazeneca-debug.log";

// Токен O3SYowZft14FaJ84SSotNk3JbGkkpXpiKUSjMVBS
// Сикрет Y3Xga2A2XcKrsrQjJRk9RuyQjr0JiOUEdlshabnc

int SLAPI SCardSpecialTreatment_AstraZeneca::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString login;
	SString secret;
	SString temp_buf;
	{
		PPObjGlobalUserAcc gua_obj;
		PPGlobalUserAcc gua_rec;
		PPID   gua_id = 0;
		if(gua_obj.SearchBySymb("ASTRAZENECAAPI", &gua_id, &gua_rec) > 0) {
			p_ref->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_LOGIN, login);
			p_ref->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_SECRET, secret);
		}
	}
	THROW_PP(login.NotEmpty(), PPERR_UNABLEGETAUTH);
	THROW_PP(secret.NotEmpty(), PPERR_UNABLEGETAUTH);
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrContentType, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthToken, login);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthSecret, secret);
	CATCHZOK
	return ok;
}

int SLAPI SCardSpecialTreatment_AstraZeneca::VerifyOwner(const CardBlock * pScBlk)
{
	int    ok = 1;
	json_t * p_query = 0;
	json_t * p_reply = 0;
	ScURL  c;
	SString temp_buf;
	SString json_buf;
	SString reply_code;
	SString phone_buf;
	SString log_buf;
	uint   check_code = 0; // Код подтверждения, полученный владельцем - этот код надо будет переправить провайдеру
	int    last_query_status = -1; // 1 - success, 0 - error, -1 - undefined
	int    last_query_errcode = 0;
	SString last_query_message;
	MakeUrl("register", temp_buf);
	InetUrl url(temp_buf);
	StrStrAssocArray hdr_flds;
	THROW(PrepareHtmlFields(hdr_flds));
	{
		PPGetFilePath(PPPATH_LOG, P_Az_DebugFileName, temp_buf);
		SFile f_out_test(temp_buf, SFile::mAppend);
		{
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(p_query = new json_t(json_t::tOBJECT));
			THROW_SL(p_query->Insert("pos_id", json_new_string(pScBlk->PosNodeCode)));
			THROW_SL(p_query->Insert("card_number", json_new_string(pScBlk->ScPack.Rec.Code)));
			pScBlk->ScPack.GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
			THROW(temp_buf.NotEmptyS());
			PPEAddr::Phone::NormalizeStr(temp_buf, PPEAddr::Phone::nsfPlus, phone_buf);
			THROW_SL(p_query->Insert("phone_number", json_new_string(phone_buf)));
			//THROW_SL(p_query->Insert("trust_key", json_new_string("")));
			THROW_SL(json_tree_to_string(p_query, temp_buf));
			json_buf.EncodeUrl(temp_buf, 0);
			f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(json_buf).CR());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
					if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
						for(json_t * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Type == json_t::tOBJECT) {
								for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
									if(p_obj->Text.IsEqiAscii("status")) {
										if(p_obj->P_Child->Text.IsEqiAscii("success"))
											last_query_status = 1;
										else if(p_obj->P_Child->Text.IsEqiAscii("error"))
											last_query_status = 0;
									}
									else if(p_obj->Text.IsEqiAscii("error_code")) {
										last_query_errcode = p_obj->P_Child->Text.ToLong();
									}
									else if(p_obj->Text.IsEqiAscii("code")) {
										reply_code = p_obj->P_Child->Text;
									}
									else if(p_obj->Text.IsEqiAscii("message")) {
										last_query_message = p_obj->P_Child->Text;
									}
								}
							}
						}
					}
				}
			}
			if(!last_query_status) {
				temp_buf.Z().Cat(last_query_errcode).CatDiv('-', 1).Cat(last_query_message.Transf(CTRANSF_UTF8_TO_INNER));
				CALLEXCEPT_PP_S(PPERR_SCSPCTRT_AZ, temp_buf);
			}
		}
		json_free_value(&p_query);
		if(last_query_status > 0 && VerifyPhoneNumberBySms(phone_buf, 0, &check_code, 1) > 0) {
			MakeUrl("confirm_code", temp_buf);
			url.Clear();
			url.Parse(temp_buf);
			//
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(p_query = new json_t(json_t::tOBJECT));
			THROW_SL(p_query->Insert("pos_id", json_new_string(pScBlk->PosNodeCode)));
			THROW_SL(p_query->Insert("code", json_new_string(temp_buf.Z().Cat(check_code))));
			THROW_SL(json_tree_to_string(p_query, temp_buf));
			json_buf.EncodeUrl(temp_buf, 0);
			f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(json_buf).CR());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
					json_free_value(&p_reply);
					if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
						for(json_t * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Type == json_t::tOBJECT) {
								for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
									if(p_obj->Text.IsEqiAscii("status")) {
										if(p_obj->P_Child->Text.IsEqiAscii("success"))
											last_query_status = 1;
										else if(p_obj->P_Child->Text.IsEqiAscii("error"))
											last_query_status = 0;
									}
									else if(p_obj->Text.IsEqiAscii("error_code")) {
										last_query_errcode = p_obj->P_Child->Text.ToLong();
									}
									else if(p_obj->Text.IsEqiAscii("message")) {
										last_query_message = p_obj->P_Child->Text;
									}
								}
							}
						}
					}
				}
			}
			if(!last_query_status) {
				temp_buf.Z().Cat(last_query_errcode).CatDiv('-', 1).Cat(last_query_message.Transf(CTRANSF_UTF8_TO_INNER));
				CALLEXCEPT_PP_S(PPERR_SCSPCTRT_AZ, temp_buf);
			}
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

int SLAPI SCardSpecialTreatment_AstraZeneca::CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, long * pRetFlags)
{
	int    ok = -1;
	json_t * p_query = 0;
	json_t * p_reply = 0;
	SString temp_buf;
	SString ta_ident;
	int    last_query_status = -1; // 1 - success, 0 - error, -1 - undefined
	int    last_query_errcode = 0;
	int    is_cc_suitable = 0;
	{
		for(uint lp = 0; !is_cc_suitable && lp < pCcPack->GetCount(); lp++) {
			const CCheckLineTbl::Rec & r_line = pCcPack->GetLine(lp);
			pCcPack->GetLineTextExt(lp+1, CCheckPacket::lnextRemoteProcessingTa, ta_ident);
			if(ta_ident.NotEmpty())
				is_cc_suitable = 1;
		}
	}
	if(is_cc_suitable) {
		SString json_buf;
		SString reply_code;
		SString phone_buf;
		SString log_buf;
		SString last_query_message;
		MakeUrl("confirm_purchase", temp_buf);
		InetUrl url(temp_buf);
		StrStrAssocArray hdr_flds;
		THROW(PrepareHtmlFields(hdr_flds));
		{
			ScURL  c;
			PPGetFilePath(PPPATH_LOG, P_Az_DebugFileName, temp_buf);
			SFile f_out_test(temp_buf, SFile::mAppend);
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(p_query = new json_t(json_t::tOBJECT));
			THROW_SL(p_query->Insert("pos_id", json_new_string(pScBlk->PosNodeCode)));
			THROW_SL(p_query->Insert("card_number", json_new_string(pScBlk->ScPack.Rec.Code)));
			pScBlk->ScPack.GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
			THROW(temp_buf.NotEmptyS());
			PPEAddr::Phone::NormalizeStr(temp_buf, PPEAddr::Phone::nsfPlus, phone_buf);
			THROW_SL(p_query->Insert("phone_number", json_new_string(phone_buf)));
			{
				json_t * p_array = new json_t(json_t::tARRAY);
				THROW_SL(p_array);
				for(uint lp = 0; lp < pCcPack->GetCount(); lp++) {
					const CCheckLineTbl::Rec & r_line = pCcPack->GetLine(lp);
					pCcPack->GetLineTextExt(lp+1, CCheckPacket::lnextRemoteProcessingTa, ta_ident);
					if(ta_ident.NotEmpty()) {
						json_t * p_item = new json_t(json_t::tSTRING);
						THROW_SL(p_item);
						p_item->AssignText(ta_ident);
						THROW_SL(json_insert_child(p_array, p_item));
					}
				}
				THROW_SL(p_query->Insert("transactions", p_array));
			}
			THROW_SL(json_tree_to_string(p_query, temp_buf));
			json_buf.EncodeUrl(temp_buf, 0);
			f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(json_buf).CR());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					SString item_any_data;
					SString item_ta;
					SString item_msg;
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
					THROW_SL(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK);
					for(json_t * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Type == json_t::tOBJECT) {
							for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
								if(p_obj->Text.IsEqiAscii("status")) {
									if(p_obj->P_Child->Text.IsEqiAscii("success"))
										last_query_status = 1;
									else if(p_obj->P_Child->Text.IsEqiAscii("error"))
										last_query_status = 0;
								}
								else if(p_obj->Text.IsEqiAscii("error_code")) {
									last_query_errcode = p_obj->P_Child->Text.ToLong();
								}
								else if(p_obj->Text.IsEqiAscii("message")) {
									last_query_message = p_obj->P_Child->Text;
								}
							}
						}
					}
					if(last_query_status == 1)
						ok = 1;
				}
			}
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

static const char * /*p_tag_symb*/P_AstraZenecaGoodsTagSymb = "ASTRAZENECAGOODS";

int SLAPI SCardSpecialTreatment_AstraZeneca::DoesWareBelongToScope(PPID goodsID)
{
	int    ok = 0;
	PPID   tag_id = 0;
	PPObjTag tag_obj;
	Reference * p_ref = PPRef;
	THROW_PP_S(tag_obj.FetchBySymb(P_AstraZenecaGoodsTagSymb, &tag_id) > 0, PPERR_SPCGOODSTAGNDEF, P_AstraZenecaGoodsTagSymb);
	if(p_ref->Ot.GetTag(PPOBJ_GOODS, goodsID, tag_id, 0) > 0) {
		PPObjGoods goods_obj;
		BarcodeArray bc_list;
		SString temp_buf;
		goods_obj.P_Tbl->ReadBarcodes(goodsID, bc_list);
		for(uint bcidx = 0; !ok && bcidx < bc_list.getCount(); bcidx++) {
			int    d = 0;
			int    std = 0;
			const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
			(temp_buf = r_bc_item.Code).Strip();
			if(goods_obj.DiagBarcode(temp_buf, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE))
				ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SCardSpecialTreatment_AstraZeneca::QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	json_t * p_query = 0;
	json_t * p_reply = 0;
	SString temp_buf;
	PPObjGoods goods_obj;
	BarcodeArray bc_list;
	int    is_cc_suitable = 0;
	PPObjTag tag_obj;
	PPID   tag_id = 0;
	//const char * p_tag_symb = "ASTRAZENECAGOODS";
	THROW_PP_S(tag_obj.FetchBySymb(P_AstraZenecaGoodsTagSymb, &tag_id) > 0, PPERR_SPCGOODSTAGNDEF, P_AstraZenecaGoodsTagSymb);
	for(uint i = 0; i < rDL.getCount(); i++) {
		const DiscountBlock & r_line = rDL.at(i);
		if(r_line.Qtty != 0 && DoesWareBelongToScope(r_line.GoodsID) > 0) {
			is_cc_suitable = 1;
		}
		/*if(r_line.Qtty != 0 && p_ref->Ot.GetTag(PPOBJ_GOODS, r_line.GoodsID, tag_id, 0) > 0) {
			goods_obj.P_Tbl->ReadBarcodes(r_line.GoodsID, bc_list);
			for(uint bcidx = 0; !is_cc_suitable && bcidx < bc_list.getCount(); bcidx++) {
				int    d = 0;
				int    std = 0;
				const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
				(temp_buf = r_bc_item.Code).Strip();
				if(goods_obj.DiagBarcode(temp_buf, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE))
					is_cc_suitable = 1;
			}
		}*/
	}
	if(is_cc_suitable) {
		int    last_query_status = -1; // 1 - success, 0 - error, -1 - undefined
		int    last_query_errcode = 0;
		SString barcode;
		SString json_buf;
		SString reply_code;
		SString phone_buf;
		SString log_buf;
		SString last_query_message;
		MakeUrl("get_discount", temp_buf);
		InetUrl url(temp_buf);
		StrStrAssocArray hdr_flds;
		THROW(PrepareHtmlFields(hdr_flds));
		{
			ScURL  c;
			PPGetFilePath(PPPATH_LOG, P_Az_DebugFileName, temp_buf);
			SFile f_out_test(temp_buf, SFile::mAppend);
			SBuffer ack_buf;
			SFile wr_stream(ack_buf, SFile::mWrite);
			THROW_SL(p_query = new json_t(json_t::tOBJECT));
			THROW_SL(p_query->Insert("pos_id", json_new_string(pScBlk->PosNodeCode)));
			THROW_SL(p_query->Insert("card_number", json_new_string(pScBlk->ScPack.Rec.Code)));
			pScBlk->ScPack.GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
			THROW(temp_buf.NotEmptyS());
			PPEAddr::Phone::NormalizeStr(temp_buf, PPEAddr::Phone::nsfPlus, phone_buf);
			THROW_SL(p_query->Insert("phone_number", json_new_string(phone_buf)));
			THROW_SL(p_query->Insert("any_data", json_new_string("")));
			{
				json_t * p_array = new json_t(json_t::tARRAY);
				THROW_SL(p_array);
				p_array->AssignText("orders");
				for(uint i = 0; i < rDL.getCount(); i++) {
					barcode.Z();
					DiscountBlock & r_line = rDL.at(i);
					r_line.TaIdent[0] = 0;
					r_line.ResultPrice = r_line.InPrice;
					if(p_ref->Ot.GetTag(PPOBJ_GOODS, r_line.GoodsID, tag_id, 0) > 0) {
						goods_obj.P_Tbl->ReadBarcodes(r_line.GoodsID, bc_list);
						for(uint bcidx = 0; bcidx < bc_list.getCount(); bcidx++) {
							int    d = 0;
							int    std = 0;
							const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
							(temp_buf = r_bc_item.Code).Strip();
							if(goods_obj.DiagBarcode(temp_buf, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE)) {
								barcode = temp_buf;
								break;
							}
						}
						if(barcode.NotEmpty()) {
							json_t * p_item = new json_t(json_t::tOBJECT);
							THROW_SL(p_item);
							THROW_SL(p_item->Insert("barcode", json_new_string(barcode)));
							THROW_SL(p_item->Insert("count", json_new_number(temp_buf.Z().Cat(r_line.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)))));
							const double net_price = r_line.InPrice; //fdiv100i(r_line.Price) - r_line.Dscnt;
							THROW_SL(p_item->Insert("price", json_new_number(temp_buf.Z().Cat(net_price, MKSFMTD(0, 2, 0)))));
							THROW_SL(p_item->Insert("any_data", json_new_string(temp_buf.Z().Cat(i+1))));
							THROW_SL(json_insert_child(p_array, p_item));
						}
					}
				}
				THROW_SL(p_query->Insert("orders", p_array));
			}
			THROW_SL(json_tree_to_string(p_query, temp_buf));
			json_buf.EncodeUrl(temp_buf, 0);
			f_out_test.WriteLine((log_buf = "Q").CatDiv(':', 2).Cat(json_buf).CR());
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					SString item_any_data;
					SString item_ta;
					SString item_msg;
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
					THROW_SL(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK);
					for(json_t * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Type == json_t::tOBJECT) {
							for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
								if(p_obj->Text.IsEqiAscii("status")) {
									if(p_obj->P_Child->Text.IsEqiAscii("success"))
										last_query_status = 1;
									else if(p_obj->P_Child->Text.IsEqiAscii("error"))
										last_query_status = 0;
								}
								else if(p_obj->Text.IsEqiAscii("error_code")) {
									last_query_errcode = p_obj->P_Child->Text.ToLong();
								}
								else if(p_obj->Text.IsEqiAscii("code")) {
									reply_code = p_obj->P_Child->Text;
								}
								else if(p_obj->Text.IsEqiAscii("message")) {
									last_query_message = p_obj->P_Child->Text;
								}
								else if(p_obj->Text.IsEqiAscii("description")) {
									(temp_buf = p_obj->P_Child->Text).Strip().Chomp().Strip().Transf(CTRANSF_UTF8_TO_INNER);
									CALLPTRMEMB(pRetMsgList, add(temp_buf));
								}
								else if(p_obj->Text.IsEqiAscii("pos_id")) {
								}
								else if(p_obj->Text.IsEqiAscii("card_number")) {
								}
								else if(p_obj->Text.IsEqiAscii("phone_number")) {
								}
								else if(p_obj->Text.IsEqiAscii("any_data")) {
								}
								else if(p_obj->Text.IsEqiAscii("orders")) {
									if(p_obj->P_Child) {
										for(const json_t * p_ary_item = p_obj->P_Child->P_Child; p_ary_item; p_ary_item = p_ary_item->P_Next) {
											item_any_data.Z();
											item_ta.Z();
											item_msg.Z();
											int   item_discount_type = 0; // 1 - cash, 2 - percent
											int   item_err_code = 0;
											double item_count = 0.0;
											double item_value = 0.0;
											double item_value_per_item = 0.0;
											double item_discount = 0.0;
											double item_price = 0.0;
											for(const json_t * p_item = p_ary_item->P_Child; p_item; p_item = p_item->P_Next) {
												if(p_item->Text.IsEqiAscii("barcode"))
													;
												else if(p_item->Text.IsEqiAscii("count"))
													item_count = p_item->P_Child->Text.ToReal();
												else if(p_item->Text.IsEqiAscii("discount"))
													item_discount = p_item->P_Child->Text.ToReal();
												else if(p_item->Text.IsEqiAscii("type")) {
													if(p_item->P_Child->Text.IsEqiAscii("cash"))
														item_discount_type = 1;
													else if(p_item->P_Child->Text.IsEqiAscii("percent"))
														item_discount_type = 2;
												}
												else if(p_item->Text.IsEqiAscii("value"))
													item_value = p_item->P_Child->Text.ToReal();
												else if(p_item->Text.IsEqiAscii("value_per_item"))
													item_value_per_item = p_item->P_Child->Text.ToReal();
												else if(p_item->Text.IsEqiAscii("price"))
													item_price = p_item->P_Child->Text.ToReal();
												else if(p_item->Text.IsEqiAscii("transaction"))
													item_ta = p_item->P_Child->Text;
												else if(p_item->Text.IsEqiAscii("descrption"))
													;
												else if(p_item->Text.IsEqiAscii("error_code"))
													item_err_code = p_item->P_Child->Text.ToLong();
												else if(p_item->Text.IsEqiAscii("message"))
													item_msg = p_item->P_Child->Text;
												else if(p_item->Text.IsEqiAscii("any_data"))
													item_any_data = p_item->P_Child->Text;
											}
											{
												const long row_no = item_any_data.ToLong();
												if(row_no > 0 && row_no <= (long)rDL.getCount() && item_ta.NotEmptyS()) {
													DiscountBlock & r_line = rDL.at(row_no-1);
													if(!feqeps(item_value_per_item, r_line.InPrice, 1E-2)) {
														r_line.ResultPrice = item_value_per_item;
														STRNSCPY(r_line.TaIdent, item_ta);
														ok = 1;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}
//
//
//
//static 
SCardSpecialTreatment * FASTCALL SCardSpecialTreatment::CreateInstance(int spcTrtID)
{
	if(spcTrtID == SCRDSSPCTRT_AZ)
		return new SCardSpecialTreatment_AstraZeneca;
	else
		return 0;
}