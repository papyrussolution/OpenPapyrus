// SCSPCTRT.CPP
// Copyright (c) A.Sobolev 2018
// @codepage UTF-8
// Реализация специальных интерпретаций поведения персоанльных карт
//
#include <pp.h>
#pragma hdrstop

class SCardSpecialTreatment {
public:
	struct CardBlock {
		CardBlock() : PosNodeID(0)
		{
		}
		PPSCardPacket ScPack;
		PPID   PosNodeID;
		SString PosNodeCode;
	};
	SCardSpecialTreatment()
	{
	}
	virtual ~SCardSpecialTreatment()
	{
	}
	virtual int VerifyOwner(const CardBlock * pScBlk)
	{
		return -1;
	}
	virtual int QueryDiscount(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags)
	{
		return -1;
	}
	virtual int CommitCheck(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags)
	{
		return -1;
	}
};

class SCardSpecialTreatment_AstraZeneca : public SCardSpecialTreatment {
public:
	SCardSpecialTreatment_AstraZeneca() : SCardSpecialTreatment()
	{
	}
	virtual ~SCardSpecialTreatment_AstraZeneca()
	{
	}
	virtual int VerifyOwner(const CardBlock * pScBlk);
	virtual int QueryDiscount(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags);
	virtual int CommitCheck(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags);
private:
	void MakeUrl(const char * pSuffix, SString & rBuf)
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

int SCardSpecialTreatment_AstraZeneca::VerifyOwner(const CardBlock * pScBlk)
{
	int    ok = 1;
	json_t * p_query = 0;
	json_t * p_reply = 0;
	ScURL  c;
	SString temp_buf;
	SString json_buf;
	SString reply_code;
	SString phone_buf;
	int    last_query_status = -1; // 1 - success, 0 - error, -1 - undefined
	int    last_query_errcode = 0;
	SString last_query_message;
	MakeUrl("register", temp_buf);
	InetUrl url(temp_buf);
	StrStrAssocArray hdr_flds;
	THROW(PrepareHtmlFields(hdr_flds));
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
		THROW_SL(p_query->Insert("trust_key", json_new_string("")));
		THROW_SL(json_tree_to_string(p_query, json_buf));
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = (SBuffer *)wr_stream;
			if(p_ack_buf) {
				const int avl_size = (int)p_ack_buf->GetAvailableSize();
				temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
				//f_out_test.WriteLine(temp_buf.CR().CR());
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
	}
	json_free_value(&p_query);
	if(last_query_status > 0 && reply_code.NotEmpty()) {
		//
		MakeUrl("confirm_code", temp_buf);
		url.Clear();
		url.Parse(temp_buf);
		//
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(p_query = new json_t(json_t::tOBJECT));
		THROW_SL(p_query->Insert("pos_id", json_new_string(pScBlk->PosNodeCode)));
		THROW_SL(p_query->Insert("code", json_new_string(reply_code)));
		THROW_SL(json_tree_to_string(p_query, json_buf));
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = (SBuffer *)wr_stream;
			if(p_ack_buf) {
				const int avl_size = (int)p_ack_buf->GetAvailableSize();
				temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
				//f_out_test.WriteLine(temp_buf.CR().CR());
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
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

int SCardSpecialTreatment_AstraZeneca::CommitCheck(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags)
{
	return -1;
}

int SCardSpecialTreatment_AstraZeneca::QueryDiscount(const CardBlock * pScBlk, CCheckPacket * pCcPack, long * pRetFlags)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	json_t * p_query = 0;
	json_t * p_reply = 0;
	ScURL  c;
	SString temp_buf;
	SString barcode;
	SString json_buf;
	SString reply_code;
	SString phone_buf;
	PPObjGoods goods_obj;
	BarcodeArray bc_list;
	int    last_query_status = -1; // 1 - success, 0 - error, -1 - undefined
	int    last_query_errcode = 0;
	int    is_cc_suitable = 0;
	SString last_query_message;
	PPObjTag tag_obj;
	ObjTagItem tag_item;
	PPID   tag_id = 0;
	MakeUrl("get_discount", temp_buf);
	InetUrl url(temp_buf);
	StrStrAssocArray hdr_flds;
	const char * p_tag_symb = "ASTRAZENECAGOODS";
	THROW_PP_S(tag_obj.FetchBySymb(p_tag_symb, &tag_id) > 0, PPERR_SPCGOODSTAGNDEF, p_tag_symb);
	{
		for(uint i = 0; i < pCcPack->GetCount(); i++) {
			const CCheckLineTbl::Rec & r_line = pCcPack->GetLine(i);
			if(r_line.Quantity != 0 && p_ref->Ot.GetTag(PPOBJ_GOODS, r_line.GoodsID, tag_id, 0) > 0) {
				goods_obj.P_Tbl->ReadBarcodes(r_line.GoodsID, bc_list);
				for(uint bcidx = 0; !is_cc_suitable && bcidx < bc_list.getCount(); bcidx++) {
					int    d = 0;
					int    std = 0;
					const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
					(temp_buf = r_bc_item.Code).Strip();
					if(goods_obj.DiagBarcode(temp_buf, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE))
						is_cc_suitable = 1;
				}
			}
		}
	}
	if(is_cc_suitable) {
		THROW(PrepareHtmlFields(hdr_flds));
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
			THROW_SL(p_query->Insert("any_data", json_new_string("")));
			{
				json_t * p_array = new json_t(json_t::tARRAY);
				THROW_SL(p_array);
				p_array->AssignText("orders");
				for(uint i = 0; i < pCcPack->GetCount(); i++) {
					barcode.Z();
					const CCheckLineTbl::Rec & r_line = pCcPack->GetLine(i);
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
							THROW_SL(p_item->Insert("count", json_new_number(temp_buf.Z().Cat(r_line.Quantity, MKSFMTD(0, 6, NMBF_NOTRAILZ)))));
							const double net_price = fdiv100i(r_line.Price) - r_line.Dscnt;
							THROW_SL(p_item->Insert("price", json_new_number(temp_buf.Z().Cat(net_price, MKSFMTD(0, 2, 0)))));
							THROW_SL(p_item->Insert("any_data", json_new_string(temp_buf.Z().Cat(i+1))));
							THROW_SL(json_insert_child(p_array, p_item));
						}
					}
				}
				THROW_SL(json_insert_child(p_query, p_array));
			}
			THROW_SL(json_tree_to_string(p_query, json_buf));
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = (SBuffer *)wr_stream;
				if(p_ack_buf) {
					const int avl_size = (int)p_ack_buf->GetAvailableSize();
					temp_buf.Z().CatN((const char *)p_ack_buf->GetBuf(), avl_size);
					//f_out_test.WriteLine(temp_buf.CR().CR());
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
									else if(p_obj->Text.IsEqiAscii("card_number")) {
									}
									else if(p_obj->Text.IsEqiAscii("phone_number")) {
									}
									else if(p_obj->Text.IsEqiAscii("any_data")) {
									}
									else if(p_obj->Text.IsEqiAscii("orders")) {
										for(const json_t * p_item = p_obj->P_Child; p_item; p_item = p_item->P_Next) {
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
	else
		ok = -1;
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}