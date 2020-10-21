// SCSPCTRT.CPP
// Copyright (c) A.Sobolev 2018, 2020
// @codepage UTF-8
// Реализация специальных интерпретаций поведения персональных карт
//
#include <pp.h>
#pragma hdrstop

SCardSpecialTreatment::CardBlock::CardBlock() : SpecialTreatment(0), PosNodeID(0)
{
}

SCardSpecialTreatment::DiscountBlock::DiscountBlock() : RowN(0), GoodsID(0), Flags(0), Qtty(0.0), InPrice(0.0), ResultPrice(0.0)
{
	PTR32(TaIdent)[0] = 0;
}

SCardSpecialTreatment::IdentifyReplyBlock::IdentifyReplyBlock() : SpecialTreatment(0), Flags(0), ScID(0), InCodeType(0), Discount(0.0), Rest(0.0)
{
	PTR32(InCode)[0] = 0;
	PTR32(OperationCode)[0] = 0;
	PTR32(Hash)[0] = 0;
}

SCardSpecialTreatment::IdentifyReplyBlock & SCardSpecialTreatment::IdentifyReplyBlock::Z()
{
	SpecialTreatment = 0;
	Flags = 0;
	ScID = 0;
	InCodeType = 0;
	Discount = 0.0;
	Rest = 0.0;
	PTR32(InCode)[0] = 0;
	PTR32(OperationCode)[0] = 0;
	PTR32(Hash)[0] = 0;
	return *this;
}

SCardSpecialTreatment::SCardSpecialTreatment(uint capability) : Capability(capability)
{
}

SCardSpecialTreatment::~SCardSpecialTreatment()
{
}

int SCardSpecialTreatment::IdentifyCode(IdentifyReplyBlock & rB, PPID seriesID, int use_ta)
	{ return ictUndef; }
int SCardSpecialTreatment::VerifyOwner(const CardBlock * pScBlk)
	{ return -1; }
int SCardSpecialTreatment::DoesWareBelongToScope(PPID goodsID)
	{ return 0; }
int SCardSpecialTreatment::QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList)
	{ return -1; }
int SCardSpecialTreatment::CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, TransactionResult * pResult)
	{ return -1; }

/*static*/int FASTCALL SCardSpecialTreatment::InitSpecialCardBlock(PPID scID, PPID posNodeID, SCardSpecialTreatment::CardBlock & rBlk)
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
				PPRef->Ot.GetTagStr(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_UUID, rBlk.PosNodeCode);
				ok = 1;
			}
		}
	}
	return ok;
}

class SCardSpecialTreatment_AstraZeneca : public SCardSpecialTreatment {
public:
	SCardSpecialTreatment_AstraZeneca() : SCardSpecialTreatment(capfVerifyPhone|capfItemDiscount)
	{
	}
	virtual ~SCardSpecialTreatment_AstraZeneca()
	{
	}
	virtual int IdentifyCode(IdentifyReplyBlock & rB, PPID seriesID, int use_ta);
	virtual int VerifyOwner(const CardBlock * pScBlk);
	virtual int DoesWareBelongToScope(PPID goodsID);
	virtual int QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList);
	virtual int CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, TransactionResult * pResult);
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
	int PrepareHtmlFields(StrStrAssocArray & rHdrFlds);
};

static const char * P_Az_DebugFileName = "astrazeneca-debug.log";

// Токен O3SYowZft14FaJ84SSotNk3JbGkkpXpiKUSjMVBS
// Сикрет Y3Xga2A2XcKrsrQjJRk9RuyQjr0JiOUEdlshabnc

int SCardSpecialTreatment_AstraZeneca::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
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

/*virtual*/int SCardSpecialTreatment_AstraZeneca::IdentifyCode(IdentifyReplyBlock & rB, PPID seriesID, int use_ta)
{
	int    ret = ictUndef;
	PPObjSCard sc_obj;
	SCardTbl::Rec sc_rec;
	if(sc_obj.SearchCode(seriesID, rB.InCode, &sc_rec) > 0) {
		rB.ScID = sc_rec.ID;
		rB.InCodeType = ictSCardCode;
		rB.SpecialTreatment = SCRDSSPCTRT_AZ;
		ret = ictSCardCode;
	}
	else
		rB.Z();
	return ret;
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
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
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
			url.Z().Parse(temp_buf);
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
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
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

int SCardSpecialTreatment_AstraZeneca::CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, TransactionResult * pResult)
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
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					SString item_any_data;
					SString item_ta;
					SString item_msg;
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					f_out_test.WriteLine((log_buf = "R").CatDiv(':', 2).Cat(temp_buf).CR());
					THROW_SL(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK);
					for(json_t * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
						if(json_t::IsObject(p_cur)) {
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

int SCardSpecialTreatment_AstraZeneca::DoesWareBelongToScope(PPID goodsID)
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

int SCardSpecialTreatment_AstraZeneca::QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList)
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
				p_array->AssignText(temp_buf = "orders");
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
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					SString item_any_data;
					SString item_ta;
					SString item_msg;
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
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
class UdsGameInterface {
public:
	struct InitBlock {
		InitBlock() : GuaID(0)
		{
		}
		InitBlock & Z()
		{
			GuaID = 0;
			GuaPack.Z();
			CliIdent.Z();
			CliAccsKey.Z();
			EndPoint.Z();
			return *this;
		}
		PPID   GuaID;
		PPGlobalUserAccPacket GuaPack;
		SString CliIdent;
		SString CliAccsKey;
		SString EndPoint; // URL для запросов
	};
	struct Error {
		Error() : Code(0)
		{
		}
		Error & Z()
		{
			Code = 0;
			ErrCode.Z();
			Message.Z();
			return *this;
		}
		int    Code; // -1 error but numeric code is undefined
		SString ErrCode; // text code
		SString Message; // text message
	};
	struct MembershipTier {
		struct Conditions {
			Conditions() : TotalCashSpent(0.0), EffInvitedCount(0.0)
			{
			}
			double TotalCashSpent;
			double EffInvitedCount;
		};
		MembershipTier() : Rate(0.0)
		{
		}
		S_GUID Uid;
		SString Name;
		double Rate;
		Conditions C;
	};
	struct Settings {
		enum {
			fPurchaseByPhone = 0x0001,
			fWriteInvoice    = 0x0002,
			fBurnPointsOnPurchase  = 0x0004,
			fBurnPointsOnPricelist = 0x0008
		};
		Settings() : ID(0), Flags(0), MaxScoresDiscount(0.0), CashierAward(0.0), ReferralReward(0.0), ReceiptLimit(0.0), DeferPointsForDays(0.0)
		{
		}
		int64  ID;
		uint   Flags;
		double MaxScoresDiscount;
		double CashierAward;
		double ReferralReward;
		double ReceiptLimit;
		double DeferPointsForDays;
		SString Name;
		SString PromoCode;
		SString Currency;
		SString BaseDiscountPolicy;
		SString Slug;
		MembershipTier BaseMt;
		TSCollection <MembershipTier> MtList;
	};
	struct Participant {
		Participant() : ID(0), InviterID(0), PointCount(0.0), DiscountRate(0.0), CashbackRate(0.0), DOB(ZERODATE),
			DtmCreated(ZERODATETIME), DtmLastTransaction(ZERODATETIME)
		{
		}
		int64 ID;
		int64 InviterID;
		double PointCount;
		double DiscountRate;
		double CashbackRate;
		LDATE DOB;
		LDATETIME DtmCreated;
		LDATETIME DtmLastTransaction;
		MembershipTier Mt;
	};
	struct Customer {
		Customer() : Gender(GENDER_UNDEF), DOB(ZERODATE)
		{
		}
		int    Gender; // GENDER_XXX
		LDATE  DOB;
		S_GUID Uid;
		SString Avatar;
		SString DisplayName;
		SString Phone;
		Participant P;
	};
	struct Purchase {
		double MaxPoints;
		double Total;
		double SkipLoyaltyTotal;
		double DiscountAmount;
		double DiscountPercent;
		double Points;
		double PointsPercent;
		double NetDiscount;
		double NetDiscountPercent;
		double Cash;
		double CashBack;
	};
	struct Ref {
		Ref() : ID(0)
		{
		}
		int64  ID;
		SString Name;
	};

	enum {
		tactUndef = 0,
		tactPurchase = 1,
	};
	enum {
		tstUndef = 0,
		tstNormal,   // NORMAL
		tstCanceled, // CANCELED
		tstReversal  // REVERSAL
	};
	struct Transaction {
		Transaction() : ID(0), Dtm(ZERODATETIME), Action(tactUndef), State(tstUndef), Points(0.0), Cash(0.0), Total(0.0), SkipLoyaltyTotal(0.0)
		{
		}
		int64  ID;
		LDATETIME Dtm;
		int    Action; // tactXXX
		int    State;  // tstXXX
		SString Code;       // При создании транзакции: код, сканируемый с телефона 
		SString BillNumber; // При создании транзакции: номер чека или документа
		Customer Cust;
		Ref    Cashier;
		Ref    Branch;
		double Points;
		double Cash;
		double Total;
		double SkipLoyaltyTotal; // При создании транзакции: A part of the bill amount for which cashback is not credited and to which the discount does not apply (in currency units).
	};
	struct FindCustomerParam {
		FindCustomerParam() : Total(0.0), SkipLoyaltyTotal(0.0), Flags(0)
		{
		}
		enum {
			fExchangeCode = 0x0001 // Exchange existing payment promo code (if present in query) to a new long-term one. Old promo code will be deactivated.
		};
		S_GUID Uid;    // Customer UID
		double Total;  // Total bill amount (in currency units).
		double SkipLoyaltyTotal; // A part of the bill amount for which cashback is not credited and to which the discount does not apply (in currency units).
		long   Flags;
		SString Code;  // Payment code
		SString Phone; // Phone number in E164 format, for example, +79876543210.
	};
	UdsGameInterface();
	~UdsGameInterface();
	int    Setup(PPID guaID);
	//
	// Descr: Возвращает !0 если последний метод обращения к серверу завершился ошибкой.
	//   По ссылке rErr возвращает состояние последней ошибки.
	//
	int    IsError(Error & rErr) const
	{
		rErr = LastErr;
		return BIN(LastErr.Code);
	}
	int    GetSettings(Settings & rResult);        // GET https://api.uds.app/partner/v2/settings
	int    GetTransactionList(); // GET  https://api.uds.app/partner/v2/operations
	int    GetTransactionInformation(); // GET  https://api.uds.app/partner/v2/operations
	int    GetTransactionInformation2(); // POST https://api.uds.app/partner/v2/operations/calc
	int    CreateTransaction(const Transaction & rT, Transaction & pReplyT);  // POST https://api.uds.app/partner/v2/operations
	int    RefundTransaction();  // POST https://api.uds.app/partner/v2/operations/<id>/refund
	int    RewardingUsersWithPoints(); // POST https://api.uds.app/partner/v2/operations/reward
	int    GetCustomerList(TSCollection <Customer> & rResult); // GET https://api.uds.app/partner/v2/customers
	int    FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase);  // GET https://api.uds.app/partner/v2/customers/find
	int    GetCustomerInformation(int64 id, Customer & rC); // GET https://api.uds.app/partner/v2/customers/<id>
	int    CreatePriceItem();  // POST https://api.uds.app/partner/v2/goods
	int    UpdatePriceItem();  // PUT https://api.uds.app/partner/v2/goods/<id>
	int    DeletePriceItem();  // DELETE -s https://api.uds.app/partner/v2/goods/<id>
	int    GetPriceItemList(); // GET -s https://api.uds.app/partner/v2/goods
	int    GetPriceItemInformation(); // GET -s https://api.uds.app/partner/v2/goods/<id>
private:
	void   PrepareHtmlFields(StrStrAssocArray & rHdrFlds);
	int    ReadError(const json_t * pJs, Error & rErr) const;
	int    ReadMembershipTier(const json_t * pJs, MembershipTier & rT) const;
	int    ReadCustomer(const json_t * pJs, Customer & rC) const;
	int    ReadParticipant(const json_t * pJs, Participant & rP) const;
	int    ReadPurchase(const json_t * pJs, Purchase & rP) const;
	InitBlock Ib;
	Error LastErr;
};

UdsGameInterface::UdsGameInterface()
{
}

UdsGameInterface::~UdsGameInterface()
{
}

int UdsGameInterface::Setup(PPID guaID)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	if(!guaID) {
		PPID single_gua_id = 0;
		PPGlobalUserAcc gua_rec;
		for(SEnum en = gua_obj.Enum(0); en.Next(&gua_rec) > 0;) {
			if(gua_rec.ServiceIdent == PPGLS_UDS) {
				if(!single_gua_id)
					single_gua_id = gua_rec.ID;
				else {
					single_gua_id = 0;
					break;
				}
			}
		}
		if(single_gua_id)
			guaID = single_gua_id;
	}
	THROW(guaID);
	Ib.GuaID = guaID;
	THROW(gua_obj.GetPacket(guaID, &Ib.GuaPack) > 0);
	THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_LOGIN, Ib.CliIdent) > 0);
	THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, Ib.CliAccsKey) > 0);
	Ib.EndPoint = InetUrl::MkHttps("api.uds.app", "partner/v2");
	CATCHZOK
	return ok;
}

void UdsGameInterface::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
{
	SString temp_buf;
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAccept, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAcceptCharset, "utf-8");
	{
		SString login;
		login.Z().Cat(Ib.CliIdent).CatChar(':').Cat(Ib.CliAccsKey).Transf(CTRANSF_INNER_TO_UTF8);
		temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthorization, login.Z().Cat("Basic").Space().Cat(temp_buf));
	}
	{
		S_GUID guid;
		guid.Generate();
		guid.ToStr(S_GUID::fmtIDL, temp_buf);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXOriginRequestId, temp_buf);
	}
	{
		const LDATETIME now_dtm = getcurdatetime_();
		temp_buf.Z().Cat(now_dtm.d, DATF_ISO8601|DATF_CENTURY).CatChar('T').Cat(now_dtm.t, TIMF_HMS|TIMF_MSEC|TIMF_TIMEZONE);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXTimestamp, temp_buf);
	}
}

int UdsGameInterface::ReadError(const json_t * pJs, Error & rErr) const
{
	int    ok = -1;
	const  json_t * p_cur = pJs;
	if(json_t::IsObject(p_cur)) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("errorCode")) {
				rErr.ErrCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
				if(rErr.ErrCode.IsEqiAscii("notFound")) {
					rErr.Code = 404;
				}
				else if(rErr.ErrCode.IsEqiAscii("badRequest")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("invalidChecksum")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("withdrawNotPermitted")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("insufficientFunds")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("priceListOnly")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("purchaseByPhoneDisabled")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("cashierNotFound")) {
					rErr.Code = 404;
				}
				else if(rErr.ErrCode.IsEqiAscii("receiptExists")) {
					rErr.Code = 409;
				}
				else {
					rErr.Code = 1; // undefined
				}
				ok = 1;
			}
			else if(p_cur->Text.IsEqiAscii("message")) {
				rErr.Message = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadMembershipTier(const json_t * pJs, MembershipTier & rT) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(json_t::IsString(p_cur->P_Child))
					rT.Uid.FromStr(p_cur->P_Child->Text);
				else
					rT.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				rT.Name = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("rate")) {
				rT.Rate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("conditions")) {
				if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tOBJECT) {
					for(const json_t * p_c = p_cur->P_Child->P_Child; p_c; p_c = p_c->P_Next) {
						if(p_c->Text.IsEqiAscii("totalCashSpent")) {
							; // @todo
						}
						else if(p_c->Text.IsEqiAscii("effectiveInvitedCount")) {
							; // @todo
						}
					}
				}
			}
		}
	}
	return ok;
}

int UdsGameInterface::GetSettings(Settings & rResult)
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("settings"));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							ok = 1;
							if(p_cur->Text.IsEqiAscii("id")) {
								rResult.ID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
							}
							else if(p_cur->Text.IsEqiAscii("name")) {
								rResult.Name = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("promoCode")) {
								rResult.PromoCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("baseDiscountPolicy")) {
								rResult.BaseDiscountPolicy = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("purchaseByPhone")) {
								SETFLAG(rResult.Flags, rResult.fPurchaseByPhone, json_t::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("writeInvoice")) {
								SETFLAG(rResult.Flags, rResult.fWriteInvoice, json_t::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("currency")) {
								rResult.Currency = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("slug")) {
								rResult.Slug = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("loyaltyProgramSettings")) {
								if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tOBJECT) {
									for(json_t * p_lps_item = p_cur->P_Child->P_Child; p_lps_item; p_lps_item = p_lps_item->P_Next) {
										if(p_lps_item->Text.IsEqiAscii("membershipTiers")) {
											if(p_lps_item->P_Child && p_lps_item->P_Child->Type == json_t::tARRAY) {
												for(json_t * p_mt_item = p_lps_item->P_Child->P_Child; p_mt_item; p_mt_item = p_mt_item->P_Next) {
													MembershipTier * p_new_mt = rResult.MtList.CreateNewItem();
													if(p_new_mt) {
														ReadMembershipTier(p_mt_item, *p_new_mt);
													}
												}
											}
										}
										else if(p_lps_item->Text.IsEqiAscii("baseMembershipTier")) {
											ReadMembershipTier(p_lps_item->P_Child, rResult.BaseMt);
										}
										else if(p_lps_item->Text.IsEqiAscii("deferPointsForDays"))
											rResult.DeferPointsForDays = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralCashbackRates")) {
											; // @todo
										}
										else if(p_lps_item->Text.IsEqiAscii("receiptLimit"))
											rResult.ReceiptLimit = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("cashierAward"))
											rResult.CashierAward = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralReward"))
											rResult.ReferralReward = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("maxScoresDiscount"))
											rResult.MaxScoresDiscount = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
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
	delete p_js_doc;
	return ok;
}

int UdsGameInterface::ReadCustomer(const json_t * pJs, Customer & rC) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(json_t::IsString(p_cur->P_Child))
					rC.Uid.FromStr(p_cur->P_Child->Text);
				else
					rC.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("id")) {
				if(json_t::IsNumber(p_cur->P_Child))
					rC.P.ID = p_cur->P_Child->Text.ToInt64();
			}
			else if(p_cur->Text.IsEqiAscii("phone")) {
				rC.Phone = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("gender")) {
				if(json_t::IsString(p_cur->P_Child)) {
					if(p_cur->P_Child->Text.IsEqiAscii("male"))
						rC.Gender = GENDER_MALE;
					else if(p_cur->P_Child->Text.IsEqiAscii("female"))
						rC.Gender = GENDER_FEMALE;
					else
						rC.Gender = GENDER_UNDEF;
				}
				else
					rC.Gender = GENDER_UNDEF;
			}
			else if(p_cur->Text.IsEqiAscii("birthDate")) {
				if(json_t::IsString(p_cur->P_Child))
					rC.DOB = strtodate_(p_cur->P_Child->Text, DATF_ISO8601);
				else
					rC.DOB = ZERODATE;
			}
			else if(p_cur->Text.IsEqiAscii("avatar")) {
				rC.Avatar = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("displayName")) {
				rC.DisplayName = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("participant")) {
				ReadParticipant(p_cur->P_Child, rC.P);
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadParticipant(const json_t * pJs, Participant & rP) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id")) {
				rP.ID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("cashbackRate")) {
				rP.CashbackRate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("dateCreated")) {
				if(json_t::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmCreated, DATF_ISO8601, 0);
				else
					rP.DtmCreated.Z();
			}
			else if(p_cur->Text.IsEqiAscii("discountRate")) {
				rP.DiscountRate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("inviterId")) {
				rP.InviterID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("lastTransactionTime")) {
				if(json_t::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmLastTransaction, DATF_ISO8601, 0);
				else
					rP.DtmLastTransaction.Z();
			}
			else if(p_cur->Text.IsEqiAscii("points")) {
				rP.PointCount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("membershipTier")) {
				ReadMembershipTier(p_cur->P_Child, rP.Mt);
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadPurchase(const json_t * pJs, Purchase & rP) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("maxPoints"))
				rP.MaxPoints = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("total"))
				rP.Total = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("skipLoyaltyTotal"))
				rP.SkipLoyaltyTotal = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("discountAmount"))
				rP.DiscountAmount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("discountPercent"))
				rP.DiscountPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("points"))
				rP.Points = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("pointsPercent"))
				rP.PointsPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("netDiscount"))
				rP.NetDiscount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("netDiscountPercent"))
				rP.NetDiscountPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("cash"))
				rP.Cash = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			else if(p_cur->Text.IsEqiAscii("cashBack"))
				rP.CashBack = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
		}
	}
	return ok;
}

int UdsGameInterface::GetCustomerInformation(int64 id, Customer & rC) // GET https://api.uds.app/partner/v2/customers/<id>
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers").SetLastDSlash().Cat(id));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					ReadCustomer(p_js_doc, rC);
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	return ok;
}

int UdsGameInterface::GetCustomerList(TSCollection <Customer> & rResult) // GET https://api.uds.app/partner/v2/customers
{
	LastErr.Z();
	int    ok = -1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers"));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("rows")) {
								ok = 1;
								if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tARRAY) {
									for(json_t * p_item = p_cur->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										Customer * p_new_cust = rResult.CreateNewItem();
										if(p_new_cust) {
											ReadCustomer(p_item, *p_new_cust);
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
	delete p_js_doc;
	return ok;
}

int UdsGameInterface::FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase)  // GET https://api.uds.app/partner/v2/customers/find
{
	LastErr.Z();
	int    ok = 1;
	/*
		curl -H 'Accept: application/json' \
			 -H "X-Origin-Request-Id: $(uuidgen)" \
			 -H "X-Timestamp: $(date --iso-8601=seconds --utc)" \
			 -u "<companyId>:<api_key>"
			 -X GET -s https://api.uds.app/partner/v2/customers/find?code=456123&phone=+71234567898&uid=23684cea-ca50-4c5c-b399-eb473e85b5ad
	*/
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		uint   arg_count = 0;
		(url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers").SetLastDSlash().Cat("find");
		if(rP.Code.NotEmpty())
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("code", rP.Code);
		if(rP.Phone.NotEmpty())
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("phone", rP.Phone);
		if(!rP.Uid.IsZero()) {
			rP.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("uid", temp_buf);
		}
		InetUrl url(url_buf);
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("user")) {
								ReadCustomer(p_cur->P_Child, rC);
							}
							else if(p_cur->Text.IsEqiAscii("code")) {
								rCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("purchase")) {
								ReadPurchase(p_cur->P_Child, rPurchase);
							}
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	return ok;
}

int UdsGameInterface::CreateTransaction(const Transaction & rT, Transaction & rReplyT)  // POST https://api.uds.app/partner/v2/operations
{
	/*
	{
		"code": "string",
		"participant": {
			"uid": "string",
			"phone": "string"
		},
		"nonce": "string",
		"cashier": {
			"externalId": "string",
			"name": "string"
		},
		"receipt": {
			"total": 0,
			"cash": 0,
			"points": 0,
			"number": "string",
			"skipLoyaltyTotal": 0
		}
	}
	*/
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	json_t * p_json_req = 0;
	ScURL c;
	S_GUID tra_guid;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	{
		SFileFormat::GetMime(SFileFormat::Json, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		PrepareHtmlFields(hdr_flds);
	}
	{
		p_json_req = new json_t(json_t::tOBJECT);
		if(rT.Code.NotEmpty())
			p_json_req->InsertString("code", rT.Code);
		if(rT.Cust.Phone.NotEmpty() || !!rT.Cust.Uid) {
			json_t * p_js_participant = new json_t(json_t::tOBJECT);
			if(!!rT.Cust.Uid) {
				rT.Cust.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				p_js_participant->InsertString("uid", temp_buf);
			}
			else
				p_js_participant->InsertNull("uid");
			if(rT.Cust.Phone.NotEmpty())
				p_js_participant->InsertString("phone", rT.Cust.Phone);
			else
				p_js_participant->InsertNull("phone");
			p_json_req->Insert("participant", p_js_participant);
		}
		else
			p_json_req->InsertNull("participant");
		tra_guid.Generate();
		tra_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		p_json_req->InsertString("nonce", temp_buf);
		if(rT.Cashier.ID || rT.Cashier.Name.NotEmpty()) {
			json_t * p_js_cashier = new json_t(json_t::tOBJECT);
			if(rT.Cashier.ID) {
				temp_buf.Z().Cat(rT.Cashier.ID);
				p_js_cashier->InsertString("externalId", temp_buf);
			}
			if(rT.Cashier.Name.NotEmpty()) {
				p_js_cashier->InsertString("name", rT.Cashier.Name);
			}
			p_json_req->Insert("cashier", p_js_cashier);
		}
		{
			json_t * p_js_receipt = new json_t(json_t::tOBJECT);
			p_js_receipt->InsertDouble("total", rT.Total, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertDouble("cash", rT.Cash, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertDouble("points", rT.Points, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertString("number", rT.BillNumber);
			p_js_receipt->InsertDouble("skipLoyaltyTotal", rT.SkipLoyaltyTotal, MKSFMTD(0, 2, 0));
			p_json_req->Insert("receipt", p_js_receipt);
		}
		THROW_SL(json_tree_to_string(p_json_req, json_buf));
	}
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("operations"));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf).Space().Cat(json_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							ok = 1;
							if(p_cur->Text.IsEqiAscii("id")) {
								rReplyT.ID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
							}
							else if(p_cur->Text.IsEqiAscii("dateCreated")) {
								if(json_t::IsString(p_cur->P_Child)) {
									strtodatetime(p_cur->P_Child->Text, &rReplyT.Dtm, DATF_ISO8601, 0);
								}
							}
							else if(p_cur->Text.IsEqiAscii("action")) {
								if(json_t::IsString(p_cur->P_Child)) {
									if(p_cur->P_Child->Text.IsEqiAscii("PURCHASE")) {
										rReplyT.Action = tactPurchase;
									}
								}
							}
							else if(p_cur->Text.IsEqiAscii("state")) {
								if(json_t::IsString(p_cur->P_Child)) {
									if(p_cur->P_Child->Text.IsEqiAscii("NORMAL"))
										rReplyT.State = tstNormal;
									else if(p_cur->P_Child->Text.IsEqiAscii("CANCELED"))
										rReplyT.State = tstCanceled;
									else if(p_cur->P_Child->Text.IsEqiAscii("REVERSAL"))
										rReplyT.State = tstReversal;
								}
							}
							else if(p_cur->Text.IsEqiAscii("points")) {
								rReplyT.Points = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
							}
							else if(p_cur->Text.IsEqiAscii("cash")) {
								rReplyT.Cash = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
							}
							else if(p_cur->Text.IsEqiAscii("total")) {
								rReplyT.Total = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
							}
							else if(p_cur->Text.IsEqiAscii("customer")) {
								ReadCustomer(p_cur->P_Child, rReplyT.Cust);
							}
							else if(p_cur->Text.IsEqiAscii("cashier")) {
							}
							else if(p_cur->Text.IsEqiAscii("branch")) {
							}
							else if(p_cur->Text.IsEqiAscii("origin")) {
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	delete p_json_req;
	return ok;
}

class SCardSpecialTreatment_UDS : public SCardSpecialTreatment, public UdsGameInterface {
public:
	SCardSpecialTreatment_UDS() : SCardSpecialTreatment(capfBonus|capfTotalDiscount), UdsGameInterface()
	{
	}
	virtual ~SCardSpecialTreatment_UDS()
	{
	}
	virtual int IdentifyCode(IdentifyReplyBlock & rB, PPID seriesID, int use_ta);
	virtual int VerifyOwner(const CardBlock * pScBlk);
	virtual int DoesWareBelongToScope(PPID goodsID);
	virtual int QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList);
	virtual int CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, TransactionResult * pResult);
};

/*virtual*/int SCardSpecialTreatment_UDS::IdentifyCode(IdentifyReplyBlock & rB, PPID seriesID, int use_ta)
{
	assert(seriesID != 0);
	int    ret = ictUndef;
	PPID   found_sc_id = 0;
	SString temp_buf;
	SString code_buf(rB.InCode);
	code_buf.Strip();
	STokenRecognizer tr;
	SNaturalTokenStat nts;
	SNaturalTokenArray nta;
	tr.Run(code_buf.ucptr(), code_buf.Len(), nta.Z(), &nts); 
	if(code_buf.Len() == 6 && nta.Has(SNTOK_DIGITCODE)) {
		Reference * p_ref = PPRef;
		PPObjSCard sc_obj;
		PPObjPerson psn_obj;
		SCardTbl::Rec sc_rec;
		UdsGameInterface::FindCustomerParam fcp;
		fcp.Code = code_buf;
		SString cust_code;
		UdsGameInterface::Customer cust;
		UdsGameInterface::Purchase cust_purch;
		//fcp.Phone = "+79142706592";
		//fcp.Uid.FromStr("4f678ec3-3888-4650-af52-efa30db5699a");
		int fcr = UdsGameInterface::Setup(0) ? FindCustomer(fcp, cust, cust_code, cust_purch) : 0;
		if(fcr > 0) {
			cust.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
			PPIDArray ex_sc_list;  // Список карт, найденных по cust.Uid
			PPIDArray ex_psn_list; // Список персоналий, найденных по cust.Uid
			PPID ex_psn_id = 0;
			PPSCardSerPacket scs_pack;
			PPObjSCardSeries scs_obj;
			PPSCardPacket sc_pack;
			THROW(!seriesID || scs_obj.GetPacket(seriesID, &scs_pack) > 0);
			{
				p_ref->Ot.SearchObjectsByGuid(PPOBJ_PERSON, PPTAG_PERSON_UUID, cust.Uid, &ex_psn_list);
				for(uint i = 0; !ex_psn_id && i < ex_psn_list.getCount(); i++) {
					const PPID temp_id = ex_psn_list.get(i);
					if(temp_id && (!scs_pack.Rec.PersonKindID || psn_obj.P_Tbl->IsBelongToKind(temp_id, scs_pack.Rec.PersonKindID)))
						ex_psn_id = temp_id;
				}
			}
			if(ex_psn_id) {
				sc_obj.P_Tbl->GetListByPerson(ex_psn_id, seriesID, &ex_sc_list);
				for(uint i = 0; !found_sc_id && i < ex_sc_list.getCount(); i++) {
					if(sc_obj.GetPacket(ex_sc_list.get(i), &sc_pack) > 0) {
						sc_pack.GetExtStrData(PPSCardPacket::extssOuterId, temp_buf);
						S_GUID sc_uuid;
						if(sc_uuid.FromStr(temp_buf) && sc_uuid == cust.Uid)
							found_sc_id = sc_pack.Rec.ID;
					}
				}
			}
			if(!found_sc_id && sc_obj.P_Tbl->GetListByText(PPSCardPacket::extssOuterId, temp_buf, &ex_sc_list) > 0) {
				for(uint i = 0; !found_sc_id && i < ex_sc_list.getCount(); i++) {
					const PPID ex_sc_id = ex_sc_list.get(i);
					if(sc_obj.Search(ex_sc_id, &sc_rec) > 0 && (!seriesID || sc_rec.SeriesID == seriesID))
						found_sc_id = ex_sc_id;
				}
			}
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				{
					PPPersonPacket psn_pack;
					if(cust.DisplayName.NotEmptyS()) {
						if(ex_psn_id) {
							THROW(psn_obj.GetPacket(ex_psn_id, &psn_pack, 0) > 0);
						}
						{
							ObjTagItem tag_item;
							if(tag_item.SetGuid(PPTAG_PERSON_UUID, &cust.Uid))
								psn_pack.TagL.PutItem(PPTAG_PERSON_UUID, &tag_item);
							(temp_buf = cust.DisplayName).Transf(CTRANSF_UTF8_TO_INNER);
							STRNSCPY(psn_pack.Rec.Name, temp_buf);
							PersonCore::SetGender(psn_pack.Rec, cust.Gender);
							if(checkdate(cust.DOB) && tag_item.SetDate(PPTAG_PERSON_DOB, cust.DOB)) {
								psn_pack.TagL.PutItem(PPTAG_PERSON_DOB, &tag_item);
							}
							psn_pack.Kinds.addUnique(NZOR(scs_pack.Rec.PersonKindID, PPPRK_UNKNOWN));
							if(cust.Phone.NotEmpty()) {
								for(uint i = 0; i < psn_pack.ELA.getCount(); i++) {
								}
								//psn_pack.ELA.AddItem()
							}
							SETIFZ(psn_pack.Rec.Status, PPPRS_PRIVATE);
						}
						THROW(psn_obj.PutPacket(&ex_psn_id, &psn_pack, 0));
					}
					{
						if(found_sc_id) {
							THROW(sc_obj.GetPacket(found_sc_id, &sc_pack) > 0);
						}
						else {
							sc_pack.Z();
						}
						sc_pack.Rec.SeriesID = seriesID;
						sc_pack.Rec.PersonID = ex_psn_id;
						if(cust.P.ID > 0) {
							temp_buf.Z().Cat(cust.P.ID).Transf(CTRANSF_UTF8_TO_INNER);
							STRNSCPY(sc_pack.Rec.Code, temp_buf);
						}
						{
							cust.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
							sc_pack.PutExtStrData(PPSCardPacket::extssOuterId, temp_buf);
						}
						THROW(sc_obj.PutPacket(&found_sc_id, &sc_pack, 0));
					}
					{
						double current_rest = 0.0;
						sc_obj.P_Tbl->GetRest(found_sc_id, ZERODATE, &current_rest);
						if(!feqeps(current_rest, cust.P.PointCount, 1E-4)) {
							// Текущий остаток по карте в базе данных не соответствует остатку на сервере - надо выровнять
							TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
							SCardCore::OpBlock ob;
							ob.SCardID = found_sc_id;
							ob.Amount = (cust.P.PointCount - current_rest);
							ob.Flags |= SCardCore::OpBlock::fLeveling;
							THROW(sc_obj.P_Tbl->PutOpBlk(ob, &urn_list, 0));
						}
					}
				}
				//
				THROW(tra.Commit());
				rB.SpecialTreatment = SCRDSSPCTRT_UDS;
				rB.ScID = found_sc_id;
				rB.InCodeType = ictHash;
				rB.Discount = cust.P.DiscountRate;
				if(rB.Discount > 0.0)
					rB.Flags |= rB.fDefinedDiscount;
				rB.Rest = cust.P.PointCount;
				rB.Flags |= rB.fDefinedRest;
				ret = ictHash;
			}
		}
	}
	else if(nta.Has(SNTOK_PHONE)) { // @notimplemented
	}
	CATCH
		ret = ictUndef;
	ENDCATCH
	if(ret == ictUndef)
		rB.Z();
	return ret;
}

/*virtual*/int SCardSpecialTreatment_UDS::VerifyOwner(const CardBlock * pScBlk)
{
	return -1;
}

/*virtual*/int SCardSpecialTreatment_UDS::DoesWareBelongToScope(PPID goodsID)
{
	return -1;
}

/*virtual*/int SCardSpecialTreatment_UDS::QueryDiscount(const CardBlock * pScBlk, TSVector <DiscountBlock> & rDL, long * pRetFlags, StringSet * pRetMsgList)
{
	return -1;
}

/*virtual*/int SCardSpecialTreatment_UDS::CommitCheck(const CardBlock * pScBlk, const CCheckPacket * pCcPack, TransactionResult * pResult)
{
	int    ok = -1;
	if(pCcPack) {
		IdentifyReplyBlock irb;
		if(pCcPack->GetSCardSpecialTreatmentIdentifyReplyBlock(&irb)) {
			SString temp_buf;
			THROW(UdsGameInterface::Setup(0));
			if(pCcPack->Rec.Flags & CCHKF_RETURN) {
				; // @todo refund
			}
			else {
				UdsGameInterface::Transaction t;
				UdsGameInterface::Transaction reply_t;
				int    is_withdraw_permitted = 0;
				if(irb.InCodeType == SCardSpecialTreatment::ictHash) {
					is_withdraw_permitted = 1;
					t.Code = irb.InCode;
				}
				else {
					if(irb.InCodeType == SCardSpecialTreatment::ictPhone)
						t.Cust.Phone = irb.InCode;
					if(pScBlk && pScBlk->ScPack.GetExtStrData(PPSCardPacket::extssOuterId, temp_buf)) {
						S_GUID uid;
						if(uid.FromStr(temp_buf))
							t.Cust.Uid = uid;
					}
				}
				temp_buf.Z().Cat("CCUDS").CatChar('-').Cat(pCcPack->Rec.Code);
				t.BillNumber = temp_buf;
				if(pCcPack->Rec.UserID) {
					PPObjPerson psn_obj;
					PersonTbl::Rec psn_rec;
					if(psn_obj.Fetch(pCcPack->Rec.UserID, &psn_rec) > 0) {
						t.Cashier.ID = psn_rec.ID;
						(temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
						t.Cashier.Name = temp_buf;
					}
				}
				{
					t.Total = 0.0;
					t.Cash = 0.0;
					t.Points = 0.0;
					t.SkipLoyaltyTotal = 0.01;
					double used_bonus = 0.0;
					const CcAmountList & r_al = pCcPack->AL_Const();
					if(r_al.getCount()) {
						for(uint alidx = 0; alidx < r_al.getCount(); alidx++) {
							const CcAmountEntry r_ale = r_al.at(alidx);
							if(oneof2(r_ale.Type, CCAMTTYP_CASH, CCAMTTYP_BANK)) {
								t.Total += r_ale.Amount;
								t.Cash += r_ale.Amount;
							}
							else if(r_ale.Type == CCAMTTYP_CRDCARD && is_withdraw_permitted) {
								if(r_ale.AddedID == irb.ScID) {
									t.Total += r_ale.Amount;
									t.Points += r_ale.Amount;
								}
							}
						}
					}
					else {
						double cca = 0.0;
						double ccd = 0.0;
						pCcPack->CalcAmount(&cca, &ccd);
						t.Total = cca;
						t.Cash = cca;
					}
				}
				int ctr = UdsGameInterface::CreateTransaction(t, reply_t);
				if(pResult) {
					if(ctr > 0) {
						pResult->Status = TransactionResult::stAccepted;
						if(reply_t.ID) {
							temp_buf.Z().Cat("UDS").Cat(reply_t.ID);
							STRNSCPY(pResult->TaIdent, temp_buf);
						}
						pResult->CustomerUid = reply_t.Cust.Uid;
						if(reply_t.Cust.P.ID) {
							temp_buf.Z().Cat(reply_t.Cust.P.ID);
							STRNSCPY(pResult->CustomerIdent, temp_buf);
						}
						ok = 1;
					}
					else {
						pResult->Status = TransactionResult::stRejected;
						Error err;
						if(IsError(err)) {
							STRNSCPY(pResult->ErrMessage, err.Message);
						}
						ok = 0;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
/*static*/SCardSpecialTreatment * FASTCALL SCardSpecialTreatment::CreateInstance(int spcTrtID)
{
	if(spcTrtID == SCRDSSPCTRT_AZ)
		return new SCardSpecialTreatment_AstraZeneca;
	else if(spcTrtID == SCRDSSPCTRT_UDS)
		return new SCardSpecialTreatment_UDS;
	else
		return 0;
}
//
//
//
int TestUdsInterface()
{
	int    ok = -1;
	SString temp_buf;
	UdsGameInterface ifc;
	if(ifc.Setup(0)) {
		UdsGameInterface::Settings s;
		TSCollection <UdsGameInterface::Customer> cust_list;
		ifc.GetSettings(s);
		PPInputStringDialogParam isdp("Input client code");
		SString cli_code;
		if(InputStringDialog(&isdp, cli_code) > 0) {
			int fcr = 0;
			SString cust_code;
			UdsGameInterface::Customer cust;
			UdsGameInterface::Purchase cust_purch;
			{
				// 1099540994296
				// +79142706592 
				// 4f678ec3-3888-4650-af52-efa30db5699a
				UdsGameInterface::FindCustomerParam fcp;
				fcp.Code = cli_code;
				//fcp.Phone = "+79142706592";
				//fcp.Uid.FromStr("4f678ec3-3888-4650-af52-efa30db5699a");
				fcr = ifc.FindCustomer(fcp, cust, cust_code, cust_purch);
			}
			if(fcr > 0) {
				UdsGameInterface::Transaction t;
				UdsGameInterface::Transaction reply_t;
				t.Code = cli_code;
				t.Cust.Uid = cust.Uid;
				t.BillNumber = "CC-TEST-401";
				t.Cashier.ID = 102;
				t.Cashier.Name = "Nicole 2";
				t.Total = 400.0;
				t.Cash = 400.0;
				t.Points = 0.0;
				t.SkipLoyaltyTotal = 0.01;
				ifc.CreateTransaction(t, reply_t);
			}
		}
		ifc.GetCustomerList(cust_list);
		{
			UdsGameInterface::Customer cust;
			ifc.GetCustomerInformation(1099541301566, cust);
		}
		ok = 1;
	}
	return ok;
}
//
//
//
struct SetupGlobalServiceUDS_Param {
	SetupGlobalServiceUDS_Param() : SCardSerID(0), GuaID(0)
	{
	}
	SString Login;
	SString ApiKey;
	PPID   SCardSerID;
	PPID   GuaID;
};

static int Setup_GlobalService_UDS_InitParam(SetupGlobalServiceUDS_Param & rP, int force)
{
	int    ok = 1;
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	PPObjSCardSeries scs_obj;
	PPGlobalUserAcc gua_rec;
	PPSCardSeries2 scs_rec;
	PPIDArray gua_candidate_list;
	PPIDArray scs_candidate_list;
	
	PPSCardConfig sc_cfg;
	PPObjSCard::ReadConfig(&sc_cfg);

	if(!rP.GuaID) {
		for(SEnum en = gua_obj.Enum(0); en.Next(&gua_rec) > 0;) {
			if(gua_rec.ServiceIdent == PPGLS_UDS)
				gua_candidate_list.add(gua_rec.ID);
		}
		if(gua_candidate_list.getCount() == 1)
			rP.GuaID = gua_candidate_list.get(0);
	}
	if(rP.GuaID) {
		PPGlobalUserAccPacket gua_pack;
		if(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0) {
			if(force) {
			}
			else {
				SString db_value;
				gua_pack.TagL.GetItemStr(PPTAG_GUA_LOGIN, db_value);
				if(db_value.NotEmptyS() && !rP.Login.NotEmptyS())
					rP.Login = db_value;
				gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, db_value.Z());
				if(db_value.NotEmptyS() && !rP.ApiKey.NotEmptyS()) {
					rP.ApiKey = db_value;
				}
			}
		}
	}
	if(!rP.SCardSerID) {
		for(SEnum en = scs_obj.Enum(0); en.Next(&scs_rec) > 0;) {
			if(scs_rec.SpecialTreatment == SCRDSSPCTRT_UDS) {
				scs_candidate_list.add(scs_rec.ID);
			}
		}
		if(scs_candidate_list.getCount() == 1)
			rP.SCardSerID = scs_candidate_list.get(0);
	}
	if(force) {
		PPGlobalUserAccPacket gua_pack;
		PPSCardSerPacket scs_pack;
		THROW_PP(rP.Login.NotEmptyS(), PPERR_LOGINNEEDED);
		THROW_PP(rP.ApiKey.NotEmptyS(), PPERR_APIKEYNEEDED);
		{
			PPTransaction tra(1);
			THROW(tra);
			if(rP.GuaID) {
				PPID   temp_id = gua_pack.Rec.ID;
				THROW(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0);
				THROW_PP_S(gua_pack.Rec.ServiceIdent == PPGLS_UDS, PPERR_INVGUASERVICEIDENT, temp_buf.Z());
				gua_pack.TagL.PutItemStr(PPTAG_GUA_LOGIN, rP.Login);
				gua_pack.TagL.PutItemStr(PPTAG_GUA_ACCESSKEY, rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 1));
			}
			else {
				PPID   temp_id = 0;
				gua_pack.Rec.ServiceIdent = PPGLS_UDS;
				STRNSCPY(gua_pack.Rec.Name, "UDS");
				STRNSCPY(gua_pack.Rec.Symb, "UDS");
				gua_pack.TagL.PutItemStr(PPTAG_GUA_LOGIN, rP.Login);
				gua_pack.TagL.PutItemStr(PPTAG_GUA_ACCESSKEY, rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
			}
			if(rP.SCardSerID) {
				THROW(scs_obj.GetPacket(rP.SCardSerID, &scs_pack) > 0);
				THROW_PP_S(scs_pack.Rec.SpecialTreatment == SCRDSSPCTRT_UDS, PPERR_INVSCSSPECIALTREATMENT, temp_buf.Z());
				THROW_PP_S(scs_pack.Rec.PersonKindID != 0, PPERR_UNDEFSCSPERSONKIND, scs_pack.Rec.Name);
				THROW_PP_S(scs_pack.Rec.Flags & SCRDSF_BONUS, PPERR_SCARDSERMUSTBEBONUS, scs_pack.Rec.Name);
			}
			else {
				PPID   temp_id = 0;
				scs_pack.Rec.SpecialTreatment = SCRDSSPCTRT_UDS;
				scs_pack.Rec.Flags |= SCRDSF_BONUS;
				scs_pack.Rec.PersonKindID = sc_cfg.PersonKindID;
				STRNSCPY(scs_pack.Rec.Name, "UDS");
				STRNSCPY(scs_pack.Rec.Symb, "UDS");
				STRNSCPY(scs_pack.Eb.CodeTempl, "UDS%07[1..100000]");
				THROW(scs_obj.PutPacket(&temp_id, &scs_pack, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

class SetupGlobalServiceUDS_Dialog : public TDialog {
	DECL_DIALOG_DATA(SetupGlobalServiceUDS_Param);
public:
	SetupGlobalServiceUDS_Dialog(DlgDataType & rData) : TDialog(DLG_SU_UDS), Data(rData), State(0)
	{
		Setup_GlobalService_UDS_InitParam(Data, 0);
		setCtrlString(CTL_SUUDS_LOGIN, Data.Login);
		setCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
		SetupPPObjCombo(this, CTLSEL_SUUDS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0);
		{
			SCardSeriesFilt scs_filt;
			scs_filt.Flags = scs_filt.fOnlySeries;
			scs_filt.SpecialTreatment = SCRDSSPCTRT_UDS;
			SetupPPObjCombo(this, CTLSEL_SUUDS_SCARDSER, PPOBJ_SCARDSERIES, Data.SCardSerID, 0);
		}
		{
			SString temp_buf;
			selectCtrl(CTL_SUUDS_LOGIN);
			PPLoadStringDescription("setupglbsvc_uds_login", temp_buf);
			setStaticText(CTL_SUUDS_HINT, temp_buf);
		}
	}
	int    IsSettled() const
	{
		return BIN(State & stSettled);
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmConfigure)) {
			getCtrlString(CTL_SUUDS_LOGIN, Data.Login);
			getCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
			getCtrlData(CTLSEL_SUUDS_GUA, &Data.GuaID);
			getCtrlData(CTLSEL_SUUDS_SCARDSER, &Data.SCardSerID);
			if(!Setup_GlobalService_UDS_InitParam(Data, 1)) {
				PPError();
			}
			else {
				setCtrlString(CTL_SUUDS_LOGIN, Data.Login);
				setCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
				//
				// Комбо-боксы надо перестроить - у нас появились новые объекты, которых не было в списках
				//
				SetupPPObjCombo(this, CTLSEL_SUUDS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0);
				{
					SCardSeriesFilt scs_filt;
					scs_filt.Flags = scs_filt.fOnlySeries;
					scs_filt.SpecialTreatment = SCRDSSPCTRT_UDS;
					SetupPPObjCombo(this, CTLSEL_SUUDS_SCARDSER, PPOBJ_SCARDSERIES, Data.SCardSerID, 0);
				}
				State |= stSettled;
				//enableCommand(cmConfigure, 0);
			}
		}
		else if(TVBROADCAST && TVCMD == cmReceivedFocus) {
			SString temp_buf;
			if(event.isCtlEvent(CTL_SUUDS_LOGIN)) {
				PPLoadStringDescription("setupglbsvc_uds_login", temp_buf);
			}
			else if(event.isCtlEvent(CTL_SUUDS_APIKEY)) {
				PPLoadStringDescription("setupglbsvc_uds_apikey", temp_buf);
			}
			setStaticText(CTL_SUUDS_HINT, temp_buf);
		}
		else
			return;
		clearEvent(event);
	}
	enum {
		stSettled = 0x0001
	};
	long   State;
};

int PPSetup_GlobalService_UDS()
{
	int    ok = -1;
	SetupGlobalServiceUDS_Param param;
	SetupGlobalServiceUDS_Dialog * dlg = new SetupGlobalServiceUDS_Dialog(param);
	if(CheckDialogPtrErr(&dlg)) {
		ExecView(dlg);
		if(dlg->IsSettled())
			ok = 1;
	}
	delete dlg;
	return ok;
}
