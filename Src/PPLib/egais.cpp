// EGAIS.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Интеграция с системой EGAIS
//
#include <pp.h>
#pragma hdrstop
//
// Специальное смещение для значений номеров строк, с помощью которого
// решается проблема одиозных входящих идентификаторов строк документов (0, guid, текст, значение большие чем RowIdentDivider)
//
/*
#define BEDIUS_DESADV_OUT_SENDED            1 // Документ отправлен получателю в виде DESADV
#define BEDIUS_DESADV_OUT_PROCESSED         2 // Документ, отправленный получателю в виде DESADV, принят EDI-сервером
#define BEDIUS_DESADV_OUT_RECADV_ACC        3 // На документ, отправленный получателю в виде DESADV, получен RECADV "принят"
#define BEDIUS_DESADV_OUT_RECADV_PACC       4 // На документ, отправленный получателю в виде DESADV, получен RECADV "частично принят"
#define BEDIUS_DESADV_OUT_RECADV_REJ        5 // На документ, отправленный получателю в виде DESADV, получен RECADV "отклонен"
#define BEDIUS_DESADV_OUT_RECADV_CONF_ACC   6 // В ответ на RECADV отправлено подтверждение "принято"
#define BEDIUS_DESADV_OUT_RECADV_CONF_REJ   7 // В ответ на RECADV отправлено подтверждение "отклонено"

#define BEDIUS_DESADV_IN_ACCEPTED           8 // Документ получен от отправителя в виде DESADV
#define BEDIUS_DESADV_IN_RECADV_ACC         9 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "принят"
#define BEDIUS_DESADV_IN_RECADV_PACC       10 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "частично принят"
#define BEDIUS_DESADV_IN_RECADV_REJ        11 // На документ, полученный от отправителя в виде DESADV, отправлен RECADV "отклонен"
*/

/*
BILL_EDI_USER_STATE

Пользовательские состояния отправленных документов:

- отправлен в УТМ                        yellow    тег PPTAG_BILL_EDIACK
- успешно обработан ЕГАИС                orange    теги PPTAG_BILL_EDIACK и PPTAG_BILL_EDIIDENT
- получено подтверждение от получателя
	-- полностью принят                  green          тег PPTAG_BILL_EDIIDENT и BillCore::GetRecadvStatus() == PPEDI_RECADV_STATUS_ACCEPT
	-- частично принят                   light green    тег PPTAG_BILL_EDIIDENT и BillCore::GetRecadvStatus() == PPEDI_RECADV_STATUS_PARTACCEPT
	-- отказ                             brown          тег PPTAG_BILL_EDIIDENT и BillCore::GetRecadvStatus() == PPEDI_RECADV_STATUS_REJECT
- отправлено подтверждение получателю
	-- принято                           blue           тег PPTAG_BILL_EDIIDENT и BillCore::GetRecadvStatus() != PPEDI_RECADV_STATUS_UNDEF и
															BillCore::GetRecadvConfStatus() == PPEDI_RECADVCONF_STATUS_ACCEPT
	-- отказано                          grey           тег PPTAG_BILL_EDIIDENT и BillCore::GetRecadvStatus() != PPEDI_RECADV_STATUS_UNDEF и
															BillCore::GetRecadvConfStatus() == PPEDI_RECADVCONF_STATUS_REJECT

Пользовательские состояния принятых документов:
- принят из ЕГАИС                        orange      EdiOp == PPEDIOP_EGAIS_WAYBILL
- отправлен RECADV
	-- полностью принят                  green       EdiOp == PPEDIOP_EGAIS_WAYBILL && BillCore::GetRecadvConfStatus() == PPEDI_RECADVCONF_STATUS_ACCEPT
	-- частично принят                   light green EdiOp == PPEDIOP_EGAIS_WAYBILL && BillCore::GetRecadvConfStatus() == PPEDI_RECADVCONF_STATUS_PARTACCEPT
	-- отказ                             brown       EdiOp == PPEDIOP_EGAIS_WAYBILL && BillCore::GetRecadvConfStatus() == PPEDI_RECADVCONF_STATUS_REJECT
*/
static const char * P_TempOutputDirName = "temp-query";

static SString & Egais_GetBillCode(const BillTbl::Rec & rRec, SString & rBuf)
{
	// Since v11.1.12 эта функция устарела. Теперь длина номера документа вполне достаточна, тем не менее, пока оставляем ее "в живых"
	// @v11.1.12 rBuf.Z();
	// @v11.1.12 BillCore::GetCode(rBuf = rRec.Code);
	rBuf = rRec.Code; // @v11.1.12 
	if(rBuf.Len() >= (sizeof(rRec.Code)-4)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		if(PPRef->Ot.GetTagStr(PPOBJ_BILL, rRec.ID, PPTAG_BILL_LONGCODE, r_temp_buf) > 0) {
			rBuf = r_temp_buf;
		}
	}
	return rBuf;
}

static SString & Egais_GetBillCode(const PPBillPacket & rPack, SString & rBuf)
{
	rBuf.Z();
	if(rPack.BTagL.GetItemStr(PPTAG_BILL_LONGCODE, rBuf) > 0) {
		;
	}
	else {
		// @v11.1.12 BillCore::GetCode(rBuf = rPack.Rec.Code);
		rBuf = rPack.Rec.Code; // @v11.1.12 
	}
	return rBuf;
}

PPEgaisProcessor::Ack::Ack() : Ver(0), SignSize(0), Status(0)
{
	Sign[0] = 0;
}

PPEgaisProcessor::Ack & PPEgaisProcessor::Ack::Z()
{
	Ver = 0;
	SignSize = 0;
	Sign[0] = 0;
	Id.Z();
	Status = 0;
	Url.Z();
	Message.Z();
	return *this;
}

PPEgaisProcessor::Reply::Reply() : Status(0)
{
}

PPEgaisProcessor::Ticket::Result::Result() : Type(0), Conclusion(-1), Special(spcNone), Time(ZERODATETIME)
{
}

PPEgaisProcessor::Ticket::Result & PPEgaisProcessor::Ticket::Result::Z()
{
    Conclusion = -1;
    Special = spcNone;
    Time.Z();
    OpName.Z();
    Comment.Z();
	return *this;
}

PPEgaisProcessor::Ticket::Ticket() : TicketTime(ZERODATETIME), DocType(0)
{
}

PPEgaisProcessor::Ticket & PPEgaisProcessor::Ticket::Z()
{
	TicketTime.Z();
	DocUUID.Z();
	TranspUUID.Z();
	DocType = 0;
	RegIdent.Z();
	R.Z();
	OpR.Z();
	return *this;
}

PPEgaisProcessor::ConfirmTicket::ConfirmTicket() : BillID(0), Conclusion(-1), Date(ZERODATE)
{
}

PPEgaisProcessor::InformAReg::InformAReg() : Qtty(0), ManufDate(ZERODATE), TTNDate(ZERODATE), EGAISDate(ZERODATE)
{
}

int PPEgaisProcessor::InformAReg::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Qtty, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ManufDate, rBuf));
	THROW_SL(pSCtx->Serialize(dir, TTNDate, rBuf));
	THROW_SL(pSCtx->Serialize(dir, TTNCode, rBuf));
	THROW_SL(pSCtx->Serialize(dir, EGAISDate, rBuf));
	THROW_SL(pSCtx->Serialize(dir, EGAISCode, rBuf));
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::InformAReg::ToStr(SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	SSerializeContext sctx;
	SBuffer sbuf;
	THROW(Serialize(+1, sbuf, &sctx));
	rBuf.EncodeMime64(sbuf.GetBuf(sbuf.GetRdOffs()), sbuf.GetAvailableSize());
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::InformAReg::FromStr(const SString & rBuf)
{
	int    ok = 1;
	uint8  mime_buf[512];
	size_t binary_size = 0;
	SSerializeContext sctx;
	SBuffer sbuf;
	THROW_SL(rBuf.DecodeMime64(mime_buf, sizeof(mime_buf), &binary_size));
	THROW_SL(sbuf.Write(mime_buf, binary_size));
	THROW(Serialize(-1, sbuf, &sctx));
	CATCHZOK
	return ok;
}

PPEgaisProcessor::InformBItem::InformBItem() : P(0), BottlingDate(ZERODATE)
{
	OrgRowIdent[0] = 0;
	Ident[0] = 0;
}

PPEgaisProcessor::InformBItem & PPEgaisProcessor::InformBItem::Z()
{
	THISZERO();
	return *this;
}

PPEgaisProcessor::InformB::InformB() : FixDate(ZERODATE), OuterDate(ZERODATE), ShipperPsnID(0), ConsigneePsnID(0), SupplPsnID(0)
{
}

PPEgaisProcessor::InformB & PPEgaisProcessor::InformB::Z()
{
	Id.Z();
	WBRegId.Z();
	FixNumber.Z();
	FixDate = ZERODATE;
	OuterCode.Z();
	OuterDate = ZERODATE;
	ShipperPsnID = 0;
	ConsigneePsnID = 0;
	SupplPsnID = 0;
	Items.clear();
	return *this;
}

PPEgaisProcessor::ActInformItem::ActInformItem() : P(0)
{
	AIdent[0] = 0;
}

PPEgaisProcessor::RepealWb::RepealWb() : BillID(0), ReqTime(ZERODATETIME), Confirm(0)
{
}

PPEgaisProcessor::QueryBarcode::QueryBarcode() : RowId(0), CodeType(0)
{
}

PPEgaisProcessor::ReplyRestBCode::ReplyRestBCode() : RestTime(ZERODATETIME)
{
}

PPEgaisProcessor::UtmEntry::UtmEntry() : MainOrgID(0), Flags(0)
{
    Url[0] = 0;
    FSRARID[0] = 0;
}

PPEgaisProcessor::Packet::Packet(int docType) : DocType(docType), Flags(0), IntrBillID(0), P_Data(0), SrcReplyPos(0)
{
	switch(DocType) {
		case PPEDIOP_EGAIS_QUERYCLIENTS:
		case PPEDIOP_EGAIS_QUERYAP: P_Data = new StrStrAssocArray; break;
		case PPEDIOP_EGAIS_QUERYRESENDDOC:
		case PPEDIOP_EGAIS_QUERYRESTBCODE:
		case PPEDIOP_EGAIS_QUERYFORMA:
		case PPEDIOP_EGAIS_QUERYFORMB: 
		case PPEDIOP_EGAIS_QUERYFORMF1: // @v11.2.12
		case PPEDIOP_EGAIS_QUERYFORMF2: // @v11.2.12
			P_Data = new SString; break;
		case PPEDIOP_EGAIS_TICKET: P_Data = new Ticket; break;
		case PPEDIOP_EGAIS_TTNINFORMBREG:
		case PPEDIOP_EGAIS_TTNINFORMF2REG: P_Data = new InformB; break;
		case PPEDIOP_EGAIS_REPLYCLIENT: P_Data = new TSCollection <PPPersonPacket>; break;
		case PPEDIOP_EGAIS_REPLYAP: P_Data = new TSCollection <PPGoodsPacket>; break;
		case PPEDIOP_EGAIS_REPLYFORMA:
		case PPEDIOP_EGAIS_REPLYFORM1: // @v11.3.4
			P_Data = new EgaisRefATbl::Rec;
			memzero(P_Data, sizeof(EgaisRefATbl::Rec));
			break;
		case PPEDIOP_EGAIS_WAYBILL:
		case PPEDIOP_EGAIS_WAYBILL_V2:
		case PPEDIOP_EGAIS_WAYBILL_V3:
		case PPEDIOP_EGAIS_WAYBILL_V4: // @v11.0.12
		case PPEDIOP_EGAIS_WAYBILLACT:
		case PPEDIOP_EGAIS_WAYBILLACT_V2:
		case PPEDIOP_EGAIS_WAYBILLACT_V3:
		case PPEDIOP_EGAIS_WAYBILLACT_V4:
		case PPEDIOP_EGAIS_ACTCHARGEON:
		case PPEDIOP_EGAIS_ACTCHARGEON_V2:
		case PPEDIOP_EGAIS_ACTCHARGEONSHOP:
		case PPEDIOP_EGAIS_REPLYRESTS:
		case PPEDIOP_EGAIS_REPLYRESTS_V2:
		case PPEDIOP_EGAIS_REPLYRESTSSHOP:
		case PPEDIOP_EGAIS_ACTWRITEOFF:
		case PPEDIOP_EGAIS_ACTWRITEOFF_V2:
		case PPEDIOP_EGAIS_ACTWRITEOFF_V3:
		case PPEDIOP_EGAIS_ACTWRITEOFFSHOP:
		case PPEDIOP_EGAIS_TRANSFERTOSHOP:
		case PPEDIOP_EGAIS_TRANSFERFROMSHOP:
		case PPEDIOP_EGAIS_ACTFIXBARCODE:
		case PPEDIOP_EGAIS_ACTUNFIXBARCODE:
			P_Data = new PPBillPacket; break;
		case PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG: P_Data = new ActInform; break;
		case PPEDIOP_EGAIS_CONFIRMTICKET: P_Data = new ConfirmTicket(); break;
		case PPEDIOP_EGAIS_REQUESTREPEALWB:
		case PPEDIOP_EGAIS_REQUESTREPEALAWO:
		case PPEDIOP_EGAIS_CONFIRMREPEALWB: P_Data = new RepealWb(); break;
        case PPEDIOP_EGAIS_QUERYBARCODE:
		case PPEDIOP_EGAIS_REPLYBARCODE: P_Data = new TSCollection <QueryBarcode>; break;
		case PPEDIOP_EGAIS_REPLYRESTBCODE: P_Data = new ReplyRestBCode; break;
	}
}

PPEgaisProcessor::Packet::~Packet()
{
	switch(DocType) {
		case PPEDIOP_EGAIS_QUERYCLIENTS:
		case PPEDIOP_EGAIS_QUERYAP: delete static_cast<StrStrAssocArray *>(P_Data); break;
		case PPEDIOP_EGAIS_QUERYRESENDDOC:
		case PPEDIOP_EGAIS_QUERYRESTBCODE:
		case PPEDIOP_EGAIS_QUERYFORMA:
		case PPEDIOP_EGAIS_QUERYFORMB: 
		case PPEDIOP_EGAIS_QUERYFORMF1: // @v11.2.12
		case PPEDIOP_EGAIS_QUERYFORMF2: // @v11.2.12
			delete static_cast<SString *>(P_Data); break;
		case PPEDIOP_EGAIS_TICKET: delete static_cast<Ticket *>(P_Data); break;
		case PPEDIOP_EGAIS_TTNINFORMBREG:
		case PPEDIOP_EGAIS_TTNINFORMF2REG: delete static_cast<InformB *>(P_Data); break;
		case PPEDIOP_EGAIS_REPLYCLIENT: delete static_cast<TSCollection <PPPersonPacket> *>(P_Data); break;
		case PPEDIOP_EGAIS_REPLYAP: delete static_cast<TSCollection <PPGoodsPacket> *>(P_Data); break;
		case PPEDIOP_EGAIS_REPLYFORMA: 
		case PPEDIOP_EGAIS_REPLYFORM1: // @v11.3.4
			delete static_cast<EgaisRefATbl::Rec *>(P_Data); break;
		case PPEDIOP_EGAIS_WAYBILL:
		case PPEDIOP_EGAIS_WAYBILL_V2:
		case PPEDIOP_EGAIS_WAYBILL_V3:
		case PPEDIOP_EGAIS_WAYBILL_V4: // @v11.0.12
		case PPEDIOP_EGAIS_WAYBILLACT:
		case PPEDIOP_EGAIS_WAYBILLACT_V2:
		case PPEDIOP_EGAIS_WAYBILLACT_V3:
		case PPEDIOP_EGAIS_WAYBILLACT_V4: // @v11.0.12
		case PPEDIOP_EGAIS_ACTCHARGEON:
		case PPEDIOP_EGAIS_ACTCHARGEON_V2:
		case PPEDIOP_EGAIS_ACTCHARGEONSHOP:
		case PPEDIOP_EGAIS_REPLYRESTS:
		case PPEDIOP_EGAIS_REPLYRESTS_V2:
		case PPEDIOP_EGAIS_REPLYRESTSSHOP:
		case PPEDIOP_EGAIS_ACTWRITEOFF:
		case PPEDIOP_EGAIS_ACTWRITEOFF_V2:
		case PPEDIOP_EGAIS_ACTWRITEOFF_V3:
		case PPEDIOP_EGAIS_ACTWRITEOFFSHOP:
		case PPEDIOP_EGAIS_TRANSFERTOSHOP:
		case PPEDIOP_EGAIS_TRANSFERFROMSHOP:
		case PPEDIOP_EGAIS_ACTFIXBARCODE:
		case PPEDIOP_EGAIS_ACTUNFIXBARCODE:
			delete static_cast<PPBillPacket *>(P_Data); break;
		case PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG: delete static_cast<ActInform *>(P_Data); break;
		case PPEDIOP_EGAIS_CONFIRMTICKET: delete static_cast<ConfirmTicket *>(P_Data); break;
		case PPEDIOP_EGAIS_REQUESTREPEALWB:
		case PPEDIOP_EGAIS_REQUESTREPEALAWO:
		case PPEDIOP_EGAIS_CONFIRMREPEALWB: delete static_cast<RepealWb *>(P_Data); break;
        case PPEDIOP_EGAIS_QUERYBARCODE:
		case PPEDIOP_EGAIS_REPLYBARCODE: delete static_cast<TSCollection <QueryBarcode> *>(P_Data); break;
		case PPEDIOP_EGAIS_REPLYRESTBCODE: delete static_cast<ReplyRestBCode *>(P_Data); break;
	}
}

LotExtCodeCore * PPEgaisProcessor::GetLecT() // @v12.0.11
{ 
	SETIFZQ(P_LecT, new LotExtCodeCore);
	return P_LecT; 
} 

int PPEgaisProcessor::LogSended(const Packet & rPack)
{
	int    ok = -1;
	SString msg_buf, temp_buf;
	switch(rPack.DocType) {
		case PPEDIOP_EGAIS_QUERYCLIENTS:
		case PPEDIOP_EGAIS_QUERYAP:
		case PPEDIOP_EGAIS_QUERYRESTS:
		case PPEDIOP_EGAIS_QUERYRESTS_V2:
			PPLoadText(PPTXT_EGAIS_QS_QUERY, msg_buf);
			PPEgaisProcessor::GetDocTypeTag(rPack.DocType, temp_buf);
			msg_buf.CatDiv(':', 2).Cat(temp_buf);
			if(rPack.P_Data) {
				const StrStrAssocArray * p_list = static_cast<const StrStrAssocArray *>(rPack.P_Data);
				if(p_list->getCount()) {
					msg_buf.CatDiv('-', 2);
					for(uint i = 0; i < p_list->getCount(); i++) {
						SStrToStrAssoc param = p_list->at(i);
						msg_buf.Space().CatChar('[').Cat(param.Key).CatDiv(';', 2).Cat(param.Val).CatChar(']');
					}
				}
			}
			Log(msg_buf);
			ok = 1;
			break;
		case PPEDIOP_EGAIS_WAYBILL:
		case PPEDIOP_EGAIS_WAYBILL_V2:
			if(msg_buf.IsEmpty())
				PPLoadText(PPTXT_EGAIS_QS_WAYBILL, msg_buf);
			// @fallthrough
		case PPEDIOP_EGAIS_WAYBILLACT:
		case PPEDIOP_EGAIS_WAYBILLACT_V2:
			if(msg_buf.IsEmpty())
				PPLoadText(PPTXT_EGAIS_QS_WAYBILLACT, msg_buf);
			// @fallthrough
		case PPEDIOP_EGAIS_ACTCHARGEON:
		case PPEDIOP_EGAIS_ACTCHARGEON_V2:
			if(msg_buf.IsEmpty())
				PPLoadText(PPTXT_EGAIS_QS_CHARGEON, msg_buf);
			{
				const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(rPack.P_Data);
				if(p_pack) {
					PPObjBill::MakeCodeString(&p_pack->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
					msg_buf.CatDiv(':', 2).Cat(temp_buf);
				}
			}
			Log(msg_buf);
			ok = 1;
			break;
		case PPEDIOP_EGAIS_CONFIRMTICKET:
			PPLoadText(PPTXT_EGAIS_QS_CFMTICKET, msg_buf);
			{
				const ConfirmTicket * p_ct = static_cast<const ConfirmTicket *>(rPack.P_Data);
				if(p_ct) {
					BillTbl::Rec bill_rec;
					if(p_ct->BillID && BillObj->Search(p_ct->BillID, &bill_rec) > 0) {
						PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
						msg_buf.CatDiv(':', 2).Cat(temp_buf);
					}
					msg_buf.CatDiv('-', 1);
					if(p_ct->Conclusion == 1)
						msg_buf.Cat("Accepted");
					else if(p_ct->Conclusion == 0)
						msg_buf.Cat("Rejected");
					else
						msg_buf.Cat("Conclusion undefined!");
				}
			}
			Log(msg_buf);
			ok = 1;
			break;
	}
	return ok;
}

int PPEgaisProcessor::GetReplyList(void * pCtx, PPID locID, int direction /* +1 out, -1 - in */, TSCollection <PPEgaisProcessor::Reply> & rList)
{
    int    ok = 1;
    xmlParserCtxt * p_ctx = static_cast<xmlParserCtxt *>(pCtx);
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	ScURL  c;
	SString url;
	SBuffer ack_buf;
	THROW(GetURL(locID, url));
	url.RmvLastSlash().Slash().Cat("opt").Slash().Cat((direction > 0) ? "out" : "in");
	{
		SFile wr_stream(ack_buf, SFile::mWrite);
		THROW_SL(c.HttpGet(url, 0, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				SString temp_buf;
				const int avl_size = static_cast<int>(p_ack_buf->GetAvailableSize());
				THROW_LXML((p_doc = xmlCtxtReadMemory(p_ctx, p_ack_buf->GetBufC(), avl_size, 0, 0, XML_PARSE_NOENT)), p_ctx);
				THROW(p_root = xmlDocGetRootElement(p_doc));
				if(SXml::IsName(p_root, "A")) {
					THROW(!SXml::IsContent(p_root, "error"))
					for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
						if(SXml::IsName(p_c, "url")) {
							Reply * p_new_reply = rList.CreateNewItem();
							THROW_SL(p_new_reply);
							SXml::GetContent(p_c, temp_buf);
							p_new_reply->Url = temp_buf;
							if(SXml::GetAttrib(p_c, "replyId", temp_buf) && temp_buf.NotEmptyS())
								p_new_reply->Id.FromStr(temp_buf);
						}
						else if(SXml::GetContentByName(p_c, "ver", temp_buf)) {
							// Фактически, здесь номер версии не используем
						}
					}
				}
			}
		}
	}
    CATCHZOK
	xmlFreeDoc(p_doc);
    return ok;
}

int PPEgaisProcessor::GetTemporaryFileName(const char * pPath, const char * pSubPath, const char * pPrefix, SString & rFn)
{
	int    ok = 1;
	const  int is_cc = sstreqi_ascii(pSubPath, "cc");
	rFn.Z();
	SString temp_path;
	if(!isempty(pPath)) {
		(temp_path = pPath).SetLastSlash();
		if(is_cc)
			temp_path.Cat("xml");
		else
			temp_path.Cat("opt").SetLastSlash().Cat(P_TempOutputDirName);
	}
	else
		PPGetPath(PPPATH_TEMP, temp_path);
	if(!is_cc && !isempty(pSubPath))
		temp_path.SetLastSlash().Cat(pSubPath);
	temp_path.RmvLastSlash();
	THROW_SL(SFile::CreateDir(temp_path));
	MakeTempFileName(temp_path.SetLastSlash(), pPrefix, "xml", 0, rFn);
	CATCHZOK
	return ok;
}

int TestReadXmlMem_EgaisAck()
{
	int    ok = 1;
    xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	PPEgaisProcessor::Ack ack;
	SString temp_buf;
	SString src_path;
	PPGetPath(PPPATH_TESTROOT, src_path);
	if(src_path.NotEmpty()) {
		src_path.SetLastSlash().Cat("data").SetLastSlash().Cat("egais-ack.xml");
		SFile f_in(src_path, SFile::mRead|SFile::mBinary);
		int64 f_in_size = 0;
		f_in.CalcSize(&f_in_size);
		STempBuffer inbuf(static_cast<size_t>(f_in_size + 1024));
		if(inbuf.IsValid()) {
			size_t actual_size = 0;
			if(f_in.Read(inbuf.vptr(), inbuf.GetSize(), &actual_size) > 0) {
				SBuffer test_buf;
				test_buf.Write(inbuf.vcptr(), actual_size);
				//
				const int avl_size = static_cast<int>(test_buf.GetAvailableSize());
				xmlNode * p_root = 0;
				THROW(p_ctx = xmlNewParserCtxt());
				THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, test_buf.GetBufC(), avl_size, 0, 0, XML_PARSE_NOENT), p_ctx);
				THROW(p_root = xmlDocGetRootElement(p_doc));
				if(SXml::IsName(p_root, "A")) {
					for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
						if(SXml::GetContentByName(p_c, "error", temp_buf)) {
							ack.Status |= PPEgaisProcessor::Ack::stError;
							ack.Message = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						else if(SXml::GetContentByName(p_c, "url", temp_buf)) {
							ack.Url = temp_buf;
							if(ack.Id.FromStr(temp_buf))
								ack.Status &= ~PPEgaisProcessor::Ack::stError;
							else {
								size_t id_pos = 0;
								if(temp_buf.Search("id=", 0, 1, &id_pos) && ack.Id.FromStr(temp_buf+id_pos+3))
									ack.Status &= ~PPEgaisProcessor::Ack::stError;
							}
						}
						else if(SXml::GetContentByName(p_c, "sign", temp_buf)) {
							strnzcpy(reinterpret_cast<char *>(ack.Sign), temp_buf, sizeof(ack.Sign));
							ack.SignSize = static_cast<uint8>(temp_buf.Len());
						}
						else if(SXml::GetContentByName(p_c, "ver", temp_buf)) {
							ack.Ver = temp_buf.ToLong();
						}
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int PPEgaisProcessor::ReadAck(const SBuffer * pBuf, PPEgaisProcessor::Ack & rAck)
{
	rAck.Z();
	int    ok = -1;
    xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	const int avl_size = pBuf ? static_cast<int>(pBuf->GetAvailableSize()) : 0;
	SString debug_log_buf;
	{
		// @debug {
		debug_log_buf.Cat("UTM reply 3").CatDiv(':', 2).CatEq("size", avl_size);
		if(avl_size) {
			debug_log_buf.CatN(pBuf->GetBufC(pBuf->GetRdOffs()), avl_size);
			PPLogMessage(PPFILNAM_DEBUG_LOG, debug_log_buf, LOGMSGF_DIRECTOUTP);
		}
		// } @debug
	}
	if(pBuf) {
		SString temp_buf;
		xmlNode * p_root = 0;
		THROW(p_ctx = xmlNewParserCtxt());
		THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, pBuf->GetBufC(), avl_size, 0, 0, XML_PARSE_NOENT), p_ctx); // note Здесь может произойти сбой сеанса по непонятным причинам (Win10)
		THROW(p_root = xmlDocGetRootElement(p_doc));
		if(SXml::IsName(p_root, "A")) {
			for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
				if(SXml::GetContentByName(p_c, "error", temp_buf)) {
					rAck.Status |= Ack::stError;
					rAck.Message = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_c, "url", temp_buf)) {
					size_t id_pos = 0;
					rAck.Url = temp_buf;
					if(rAck.Id.FromStr(temp_buf))
						rAck.Status &= ~Ack::stError;
					else if(temp_buf.Search("id=", 0, 1, &id_pos) && rAck.Id.FromStr(temp_buf+id_pos+3))
						rAck.Status &= ~PPEgaisProcessor::Ack::stError;
				}
				else if(SXml::GetContentByName(p_c, "sign", temp_buf)) {
					strnzcpy(reinterpret_cast<char *>(rAck.Sign), temp_buf, sizeof(rAck.Sign));
					rAck.SignSize = static_cast<uint8>(temp_buf.Len());
				}
				else if(SXml::GetContentByName(p_c, "ver", temp_buf)) {
					rAck.Ver = temp_buf.ToLong();
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int PPEgaisProcessor::PutCCheck(const CCheckPacket & rPack, PPID locID, bool horecaAutoWo, PPEgaisProcessor::Ack & rAck)
{
	const int omit_nonmarked_goods = 1;

    int    ok = -1;
	xmlTextWriter * p_x = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	SString file_name, temp_buf;
	SString temp_path;
	SString mark_buf;
	SBuffer ack_buf;
	LongArray marked_pos_list;    // Список позиций чека, содержащих маркированный алкоголь (номера позиций [1..])
	LongArray nonmarked_pos_list; // Список позиций чека, содержащих не маркированный алкоголь (номера позиций [1..])
	PrcssrAlcReport::GoodsItem agi;
	SString url_buf;
	{
		for(uint i = 0; i < rPack.GetCount(); i++) {
			const CCheckLineTbl::Rec & r_item = rPack.GetLineC(i);
			if(IsAlcGoods(r_item.GoodsID) && PreprocessGoodsItem(r_item.GoodsID, 0, 0, 0, agi) > 0) {
				if(agi.StatusFlags & agi.stMarkWanted) {
					marked_pos_list.add(static_cast<long>(i+1));
					rPack.GetLineTextExt(i+1, CCheckPacket::lnextEgaisMark, mark_buf);
					THROW_PP_S(IsEgaisMark(mark_buf, 0), PPERR_TEXTISNTEGAISMARK, mark_buf);
				}
				else
					nonmarked_pos_list.add(static_cast<long>(i+1));
			}
		}
	}
	if(marked_pos_list.getCount() || (!omit_nonmarked_goods && nonmarked_pos_list.getCount())) {
		SString cc_text;
		SString result_barcode;
		CCheckCore::MakeCodeString(&rPack.Rec, 0, cc_text);
		PPID   main_org_id = 0;
		SString fsrar_ident;
		PPObjCashNode cn_obj;
		PPSyncCashNode cn_pack;
        THROW(cn_obj.GetSync(rPack.Rec.PosNodeID, &cn_pack) > 0);
        const  PPID loc_id = NZOR(locID, cn_pack.LocID); // Переданный параметром locID имеет приоритет перед cn_pack.LocID
		THROW(GetURL(loc_id, url_buf));
		THROW(GetDebugPath(loc_id, temp_path));
		THROW(GetTemporaryFileName(temp_path, "cc", "EGH", file_name));
		THROW(GetFSRARID(loc_id, fsrar_ident, &main_org_id));
		THROW_SL(p_x = xmlNewTextWriterFilename(file_name, 0));
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			//
			SString inn, kpp, org_name, org_addr;
			RegisterTbl::Rec reg_rec;
			PrcssrAlcReport::EgaisMarkBlock emb;
			PsnObj.GetRegNumber(main_org_id, PPREGT_TPID, rPack.Rec.Dt, inn);
			if(GetWkrRegister(wkrKPP, main_org_id, loc_id, rPack.Rec.Dt, &reg_rec) > 0)
				kpp = reg_rec.Num;
			{
				org_addr.Z();
				LocationTbl::Rec loc_rec;
				PersonTbl::Rec psn_rec;
				if(loc_id && PsnObj.LocObj.Search(loc_id, &loc_rec) > 0) {
					PsnObj.LocObj.P_Tbl->GetAddress(loc_rec, 0, org_addr);
					org_name = loc_rec.Name;
				}
				if(!org_addr.NotEmptyS() || !org_name.NotEmptyS()) {
					if(PsnObj.Search(main_org_id, &psn_rec) > 0) {
						if(psn_rec.RLoc && PsnObj.LocObj.Search(psn_rec.RLoc, &loc_rec) > 0) {
							PsnObj.LocObj.P_Tbl->GetAddress(loc_rec, 0, org_addr);
						}
						if(!org_addr.NotEmptyS()) {
							if(psn_rec.MainLoc && PsnObj.LocObj.Search(psn_rec.MainLoc, &loc_rec) > 0) {
								PsnObj.LocObj.P_Tbl->GetAddress(loc_rec, 0, org_addr);
							}
						}
						if(!org_name.NotEmptyS()) {
							org_name = psn_rec.Name;
						}
					}
				}
			}
			LDATETIME _dtm;
			if(checkdate(rPack.Rec.Dt))
				_dtm.Set(rPack.Rec.Dt, rPack.Rec.Tm);
			else
				_dtm = getcurdatetime_();
			if(Cfg.E.Flags & Config::fEgaisVer4Fmt) {
				SXml::WNode n_docs(_doc, SXml::nst("ns", "Documents"));
				n_docs.PutAttrib(SXml::nst_xmlns("xsi"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
				n_docs.PutAttrib(SXml::nst_xmlns("ns"), InetUrl::MkHttp("fsrar.ru", "WEGAIS/WB_DOC_SINGLE_01"));
				if(horecaAutoWo) {
					n_docs.PutAttrib(SXml::nst_xmlns("ck"), InetUrl::MkHttp("fsrar.ru", "WEGAIS/ChequeV4")); // @v12.0.12 WEGAIS/ChequeV3-->WEGAIS/ChequeV4
				}
				else {
					n_docs.PutAttrib(SXml::nst_xmlns("ck"), InetUrl::MkHttp("fsrar.ru", "WEGAIS/ChequeV3"));
				}
				n_docs.PutAttrib(SXml::nst_xmlns("oref"), InetUrl::MkHttp("fsrar.ru", "WEGAIS/ClientRef_v2"));
				n_docs.PutAttrib(SXml::nst_xmlns("pref"), InetUrl::MkHttp("fsrar.ru", "WEGAIS/ProductRef_v2"));
				{
					{
						SXml::WNode n_ow(_doc, SXml::nst("ns", "Owner"));
						n_ow.PutInner(SXml::nst("ns", "FSRAR_ID"), fsrar_ident);
					}
					{
						SXml::WNode n_doc(_doc, SXml::nst("ns", "Document"));
						{
							SXml::WNode n_chk(_doc, SXml::nst("ns", horecaAutoWo ? "ChequeV4" : "ChequeV3")); // @v12.0.12 ChequeV3-->ChequeV4
							n_chk.PutInner(SXml::nst("ck", "Identity"), temp_buf.Z().Cat(rPack.Rec.ID));
							{
								SXml::WNode n_httn(_doc, SXml::nst("ck", "Header"));
								temp_buf.Z().Cat(_dtm, DATF_ISO8601CENT, 0);
								n_chk.PutInner(SXml::nst("ck", "Date"), temp_buf);
								{
									cn_pack.GetPropString(SCN_MANUFSERIAL, temp_buf);
									if(!temp_buf.NotEmptyS())
										temp_buf.CatLongZ(1, 6);
									n_chk.PutInner(SXml::nst("ck", "Kassa"), temp_buf); // Заводской номер кассы
								}
								n_chk.PutInner(SXml::nst("ck", "Shift"), temp_buf.Z().Cat(rPack.Rec.SessID)); // Номер смены @todo Вероятно, здесь нужен номер, а не идентификатор
								n_chk.PutInner(SXml::nst("ck", "Number"), temp_buf.Z().Cat(rPack.Rec.Code));
								PPLoadStringS(PPSTR_HASHTOKEN_C, (rPack.Rec.Flags & CCHKF_RETURN) ? PPHSC_RU_EGAIS_RETOFSALE : PPHSC_RU_EGAIS_SALE, temp_buf);
								n_chk.PutInner(SXml::nst("ck", "Type"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
							}
							{
								/*{
									SXml::WNode n_httn(_doc, SXml::nst("ck", "HeaderTTN"));
									n_httn.PutInner(SXml::nst("ck", "Date"), temp_buf.Z().Cat(rPack.Rec.Dt, DATF_ISO8601CENT));
									temp_buf.Z().Cat(rPack.Rec.Code);
									n_httn.PutInner(SXml::nst("ck", "BillNumber"), temp_buf);
									n_httn.PutInner(SXml::nst("ck", "TTNNumber"), temp_buf);
									PPLoadStringS(PPSTR_HASHTOKEN_C, (rPack.Rec.Flags & CCHKF_RETURN) ? PPSHC_RU_EGAIS_RETOFSALE : PPSHC_RU_EGAIS_SALE, temp_buf);
									n_httn.PutInner(SXml::nst("ck", "Type"), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
								}*/
								{
									SXml::WNode n_c(_doc, SXml::nst("ck", "Content"));
									for(uint i = 0; i < marked_pos_list.getCount(); i++) {
										const uint _pos = static_cast<uint>(marked_pos_list.get(i) - 1);
										assert(_pos >= 0 && _pos < rPack.GetCount()); // @paranoic
										if(_pos >= 0 && _pos < rPack.GetCount()) { // @paranoic
											const CCheckLineTbl::Rec & r_item = rPack.GetLineC(_pos);
											if(PreprocessGoodsItem(r_item.GoodsID, 0, 0, 0, agi) > 0) {
												assert(agi.StatusFlags & agi.stMarkWanted); // @paranoic
												rPack.GetLineTextExt(_pos+1, CCheckPacket::lnextEgaisMark, mark_buf);
												assert(IsEgaisMark(mark_buf, 0)); // @paranoic
												//
												if(horecaAutoWo) {
													SXml::WNode n_b(_doc, SXml::nst("ck", "Catering"));
													n_b.PutInner(SXml::nst("ck", "Barcode"), mark_buf);
													{
														GObj.GetSingleBarcode(r_item.GoodsID, 0, temp_buf);
														result_barcode.Z();
														int    dbr = 0; // Результат диагностики штрихкода
														if(temp_buf.NotEmptyS()) {
															int    diag = 0, std = 0;
															dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
														}
														if(dbr == 0 || !oneof4(result_barcode.Len(), 8, 12, 13, 14))
															result_barcode = SlConst::P_DummyEanCode;
														n_b.PutInner(SXml::nst("ck", "EAN"), result_barcode);
													}
													{
														double _p = fabs(intmnytodbl(r_item.Price) - r_item.Dscnt);
														if(_p <= 0.0)
															_p = 1000.0;
														temp_buf.Z().Cat(_p, MKSFMTD_020);
														n_b.PutInner(SXml::nst("ck", "Price"), temp_buf);
													}
													{
														double _qtty = r_item.Quantity;
														double _vol_ml = 0.0;
														if(agi.Volume > 0.0) {
															_vol_ml = (_qtty * agi.Volume) * 1000.0;
														}
														temp_buf.Z().Cat(R0i(_vol_ml));
														n_b.PutInner(SXml::nst("ck", "Volume"), temp_buf);
													}
												}
												else {
													SXml::WNode n_b(_doc, SXml::nst("ck", "Bottle"));
													n_b.PutInner(SXml::nst("ck", "Barcode"), mark_buf);
													{
														GObj.GetSingleBarcode(r_item.GoodsID, 0, temp_buf);
														result_barcode.Z();
														int    dbr = 0; // Результат диагностики штрихкода
														if(temp_buf.NotEmptyS()) {
															int    diag = 0, std = 0;
															dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
														}
														if(dbr == 0 || !oneof4(result_barcode.Len(), 8, 12, 13, 14))
															result_barcode = SlConst::P_DummyEanCode;
														n_b.PutInner(SXml::nst("ck", "EAN"), result_barcode);
													}
													{
														double _p = fabs(intmnytodbl(r_item.Price) - r_item.Dscnt);
														temp_buf.Z().Cat(_p, MKSFMTD_020);
														n_b.PutInner(SXml::nst("ck", "Price"), temp_buf);
													}
													//<ck:Barcode>22N000008XSG44YI5IC0P7T8091700102571956QFBDLJ8NXQXV7NU55ANSSPYCRAUET</ck:Barcode>
													//<ck:EAN>4607136136823</ck:EAN>
													//<ck:Price>543.22</ck:Price>
												}
											}
										}
									}
									if(!omit_nonmarked_goods) {
										for(uint i = 0; i < nonmarked_pos_list.getCount(); i++) {
											const uint _pos = static_cast<uint>(nonmarked_pos_list.get(i) - 1);
											assert(_pos >= 0 && _pos < rPack.GetCount()); // @paranoic
											if(_pos >= 0 && _pos < rPack.GetCount()) { // @paranoic
												const CCheckLineTbl::Rec & r_item = rPack.GetLineC(_pos);
												if(PreprocessGoodsItem(r_item.GoodsID, 0, 0, 0, agi) > 0) {
													assert((agi.StatusFlags & agi.stMarkWanted) == 0); // @paranoic
													//
													SXml::WNode n_nm(_doc, SXml::nst("ck", "Nomark"));
													{
														WriteProductInfo(_doc, SXml::nst("ck", "Product"), r_item.GoodsID, 0, wpifPutManufInfo|wpifVersion2, 0);
														temp_buf.Z().Cat(fabs(r_item.Quantity), MKSFMTD(0, 0, 0));
														n_nm.PutInner(SXml::nst("ck", "Quantity"), EncText(temp_buf));
														{
															GObj.GetSingleBarcode(r_item.GoodsID, 0, temp_buf);
															result_barcode.Z();
															int    dbr = 0; // Результат диагностики штрихкода
															if(temp_buf.NotEmptyS()) {
																int    diag = 0, std = 0;
																dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
															}
															if(dbr == 0 || !oneof4(result_barcode.Len(), 8, 12, 13, 14))
																result_barcode = SlConst::P_DummyEanCode;
															n_nm.PutInner(SXml::nst("ck", "EAN"), result_barcode);
														}
														{
															double _p = fabs(intmnytodbl(r_item.Price) - r_item.Dscnt);
															temp_buf.Z().Cat(_p, MKSFMTD_020);
															n_nm.PutInner(SXml::nst("ck", "Price"), temp_buf);
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
			else {
				SXml::WNode n_doc(_doc, "Cheque");
				n_doc.PutAttrib("inn", EncText(temp_buf = inn));
				n_doc.PutAttrib("kpp", EncText(temp_buf = kpp));
				{
					if(!org_addr.NotEmptyS())
						org_addr = "dummy address";
					n_doc.PutAttrib("address", EncText(org_addr));
				}
				{
					if(!org_name.NotEmptyS())
						org_name = "dummy org name";
					n_doc.PutAttrib("name", EncText(org_name));
				}
				{
					cn_pack.GetPropString(SCN_MANUFSERIAL, temp_buf);
					if(!temp_buf.NotEmptyS())
						temp_buf.CatLongZ(1, 6);
					n_doc.PutAttrib("kassa", EncText(temp_buf)); // Заводской номер кассы
				}
				n_doc.PutAttrib("shift",  temp_buf.Z().Cat(rPack.Rec.SessID)); // Номер смены @todo Вероятно, здесь нужен номер, а не идентификатор
				n_doc.PutAttrib("number", temp_buf.Z().Cat(rPack.Rec.Code));
				{
					temp_buf.Z().Cat(_dtm.d, DATF_DMY|DATF_NODIV).Cat(_dtm.t, TIMF_HM|TIMF_NODIV);
					n_doc.PutAttrib("datetime", temp_buf);
				}
				{
					for(uint i = 0; i < marked_pos_list.getCount(); i++) {
						const uint _pos = static_cast<uint>(marked_pos_list.get(i) - 1);
						assert(_pos >= 0 && _pos < rPack.GetCount()); // @paranoic
						if(_pos >= 0 && _pos < rPack.GetCount()) { // @paranoic
							const CCheckLineTbl::Rec & r_item = rPack.GetLineC(_pos);
							if(PreprocessGoodsItem(r_item.GoodsID, 0, 0, 0, agi) > 0) {
								assert(agi.StatusFlags & agi.stMarkWanted); // @paranoic
								rPack.GetLineTextExt(_pos+1, CCheckPacket::lnextEgaisMark, mark_buf);
								assert(IsEgaisMark(mark_buf, 0)); // @paranoic
								SXml::WNode n_item(_doc, "Bottle");
								{
									double _p = intmnytodbl(r_item.Price) - r_item.Dscnt;
									if(rPack.Rec.Flags & CCHKF_RETURN)
										_p = -_p;
									temp_buf.Z().Cat(_p, MKSFMTD_020);
									n_item.PutAttrib("price", EncText(temp_buf));
								}
								n_item.PutAttrib("barcode", EncText(mark_buf));
								{
									GObj.GetSingleBarcode(r_item.GoodsID, 0, temp_buf);
									result_barcode.Z();
									int    dbr = 0; // Результат диагностики штрихкода
									if(temp_buf.NotEmptyS()) {
										int    diag = 0, std = 0;
										dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
									}
									if(dbr == 0 || !oneof4(result_barcode.Len(), 8, 12, 13, 14))
										result_barcode = SlConst::P_DummyEanCode;
									n_item.PutAttrib("ean", result_barcode);
								}
								n_item.PutAttrib("volume", EncText(temp_buf.Z().Cat(agi.Volume, MKSFMTD_030)));
							}
						}
					}
				}
				if(!omit_nonmarked_goods) {
					for(uint i = 0; i < nonmarked_pos_list.getCount(); i++) {
						const uint _pos = static_cast<uint>(nonmarked_pos_list.get(i) - 1);
						assert(_pos >= 0 && _pos < rPack.GetCount()); // @paranoic
						if(_pos >= 0 && _pos < rPack.GetCount()) { // @paranoic
							const CCheckLineTbl::Rec & r_item = rPack.GetLineC(_pos);
							if(PreprocessGoodsItem(r_item.GoodsID, 0, 0, 0, agi) > 0) {
								assert((agi.StatusFlags & agi.stMarkWanted) == 0); // @paranoic
								SXml::WNode n_item(_doc, "nopdf");
								temp_buf = agi.CategoryCode;
								n_item.PutAttrib("code", EncText(temp_buf));
								GetGoodsName(r_item.GoodsID, temp_buf);
								n_item.PutAttrib("bname", EncText(temp_buf));
								n_item.PutAttrib("volume", EncText(temp_buf.Z().Cat(agi.Volume, MKSFMTD_030)));
								n_item.PutAttrib("alc", EncText(temp_buf.Z().Cat(agi.Proof, MKSFMTD(0, 1, 0))));
								{
									const double _p = intmnytodbl(r_item.Price) - r_item.Dscnt;
									temp_buf.Z().Cat(_p, MKSFMTD_020);
									n_item.PutAttrib("price", EncText(temp_buf));
								}
								{
									GObj.GetSingleBarcode(r_item.GoodsID, BarcodeArray::sifValidEanUpcOnly, temp_buf); // @v11.9.9 BarcodeArray::sifValidEanUpcOnly
									if(temp_buf.IsEmpty()) { // @v11.9.9 
										// @v11.9.9 if(!oneof4(temp_buf.Len(), 8, 12, 13, 14)) {
										temp_buf = SlConst::P_DummyEanCode;
									}
									n_item.PutAttrib("ean", temp_buf);
								}
								temp_buf.Z().Cat(fabs(r_item.Quantity), MKSFMTD(0, 0, 0));
								n_item.PutAttrib("count", EncText(temp_buf));
							}
						}
					}
				}
			}
		}
		xmlFreeTextWriter(p_x);
		p_x = 0;
		{
            PPLoadText(PPTXT_EGAIS_QS_CCHECK, temp_buf); // Серверу ЕГАИС отправлен кассовый чек '%s'
            temp_buf.Space().CatChar('\'').Cat(cc_text).CatChar('\'');
            if(!(State & stTestSendingMode))
				temp_buf.Space().CatChar('\'').Cat(file_name).CatChar('\'');
            Log(temp_buf);
		}
		if(State & stTestSendingMode) {
			Log(PPLoadTextS(PPTXT_EGAIS_TESTSEDNING, temp_buf).CatDiv(':', 2).Cat(file_name));
			rAck.Status = 0;
			rAck.Ver = 2;
			strnzcpy(reinterpret_cast<char *>(rAck.Sign), "91F2257133C70DC649E903F203ED461D91AC9E3D4E363DF95A48BCA416FC374E4926C22F3E7002B135819AC5A7AEBD681CB81688B0AD2389EE1777412089AF86", sizeof(rAck.Sign));
			rAck.SignSize = static_cast<uint8>(sstrlen(rAck.Sign));
			rAck.Id.Generate();
			rAck.Url = "http://check.egais.ru?id=a4a702b9-3f03-49f4-800c-e69874bf3302&dt=1906162019&cn=020000448086";
			rAck.Message = "EGAIS reply emulation";
		}
		else {
			ScURL c;
			SString req;
			url_buf.RmvLastSlash().Slash().Cat("xml");
			if(horecaAutoWo) { // @v12.1.1
				url_buf.CatChar('?').CatEq("type", "ChequeV4");
			}
			else if(Cfg.E.Flags & Config::fEgaisVer4Fmt) { // @v11.0.11
				url_buf.CatChar('?').CatEq("type", "ChequeV3"); // Это - не ошибка! Версия протокола 4, а версия чеков 3
			}
			SFile wr_stream(ack_buf, SFile::mWrite);
			ScURL::HttpForm hf(c);
			{
				SFileFormat::GetMime(SFileFormat::Xml, temp_buf);
				hf.AddContentFile(file_name, temp_buf, "xml_file");
			}
			THROW_SL(c.HttpPost(url_buf, 0, hf, &wr_stream));
			{
				//LogSended(rPack);
				SBuffer * p_wr_buffer = static_cast<SBuffer *>(wr_stream);
				THROW(p_wr_buffer);
				{
					//
					// Танец с бубном в стремлении решить непонятную проблему с аварийным завершением сеанса в функции ReadAck при чтении xml из буфера
					//
					uint32 dummy_bytes = 0;
					const size_t preserve_wr_offs = p_wr_buffer->GetWrOffs();
					p_wr_buffer->Write(dummy_bytes);
					p_wr_buffer->SetWrOffs(preserve_wr_offs);
				}
				THROW(ReadAck(p_wr_buffer, rAck));
				/*
				<?xml version="1.0" encoding="UTF-8" standalone="no"?>
				<A><url>https://146.120.90.148:1444?id=ab43d5e0-855c-4b54-b1d8-ddeb34d1d110&amp;dt=1503271510&amp;cn=00040218</url>
				<sign>32B7136A7BCEAFEBE4DA92D9510C196CC42DC(1.174)
				56260E1E2802B8C24A8C3EA75F3978EA6963F370F
				3165B89DBF98FD94F2F0C7F4803965B62A6616A12D8159A5D3</sign>
				<ver>2</ver></A>
				*/
				{
					/*
						PPTXT_CCACKOK                 "Отправка чека '@zstr' серверу ЕГАИС завершилась УСПЕШНО: @zstr"
						PPTXT_CCACKERROR              "Отправка чека '@zstr' серверу ЕГАИС завершилась С ОШИБКОЙ: @zstr"
					*/
					if(rAck.Status & rAck.stError) {
						PPFormatT(PPTXT_CCACKERROR, &temp_buf, cc_text.cptr(), rAck.Message.cptr());
						Log(temp_buf);
						CALLEXCEPT_PP_S(PPERR_EGAIS_CCSENDFAULT, rAck.Message.cptr());
					}
					else {
						PPFormatT(PPTXT_CCACKOK, &temp_buf, cc_text.cptr(), reinterpret_cast<const char *>(rAck.Sign));
						Log(temp_buf);
					}
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeTextWriter(p_x);
    return ok;
}

int PPEgaisProcessor::PutQuery(PPEgaisProcessor::Packet & rPack, PPID locID, const char * pUrlSuffix, PPEgaisProcessor::Ack & rAck)
{
	rAck.Z();
    int    ok = 1;
	SString temp_buf;
	SString file_name;
	SString temp_path;
	SBuffer ack_buf;
	SString url_buf;
	THROW(GetURL(locID, url_buf));
	THROW(GetDebugPath(locID, temp_path));
	THROW(GetTemporaryFileName(temp_path, pUrlSuffix, "EGQ", file_name));
	THROW(Write(rPack, locID, file_name));
	if(State & stTestSendingMode) {
		Log(PPLoadTextS(PPTXT_EGAIS_TESTSEDNING, temp_buf).CatDiv(':', 2).Cat(file_name));
	}
	else {
		ScURL c;
		SString req;
		url_buf.RmvLastSlash().Slash().Cat("opt").Slash().Cat("in");
		if(!isempty(pUrlSuffix))
			url_buf.Slash().Cat(pUrlSuffix);
		SFile wr_stream(ack_buf, SFile::mWrite);
		ScURL::HttpForm hf(c);
		{
			SFileFormat::GetMime(SFileFormat::Xml, temp_buf);
			hf.AddContentFile(file_name, temp_buf, "xml_file");
		}
		THROW_SL(c.HttpPost(url_buf, 0, hf, &wr_stream));
		LogSended(rPack);
		{
			SBuffer * p_wr_buffer = static_cast<SBuffer *>(wr_stream);
			THROW(p_wr_buffer);
			{
				//
				// Танец с бубном в стремлении решить непонятную проблему с аварийным завершением сеанса в функции ReadAck при чтении xml из буфера
				//
				uint32 dummy_bytes = 0;
				const size_t preserve_wr_offs = p_wr_buffer->GetWrOffs();
				p_wr_buffer->Write(dummy_bytes);
				p_wr_buffer->SetWrOffs(preserve_wr_offs);
			}
			THROW(ReadAck(p_wr_buffer, rAck));
			if(rAck.Status & Ack::stError) {
				PPLoadText(PPTXT_EGAIS_ERRONQUERY, temp_buf);
				Log(temp_buf.CatDiv(':', 2).Cat(rAck.Message));
			}
			else if(!!rAck.Id) {
				BillTbl::Rec bill_rec;
				ObjTagItem tag_item;
				PPIDArray edi_op_list;
				edi_op_list.addzlist(PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4,
					PPEDIOP_EGAIS_ACTCHARGEON, PPEDIOP_EGAIS_ACTCHARGEON_V2, PPEDIOP_EGAIS_ACTCHARGEONSHOP, PPEDIOP_EGAIS_ACTWRITEOFF,
					PPEDIOP_EGAIS_ACTWRITEOFF_V2, PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_TRANSFERTOSHOP, PPEDIOP_EGAIS_TRANSFERFROMSHOP,
					PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4,
					PPEDIOP_EGAIS_ACTFIXBARCODE, PPEDIOP_EGAIS_ACTUNFIXBARCODE, 0);
				{
					Reference * p_ref = PPRef;
					PPTransaction tra(1);
					THROW(tra);
					if(rPack.DocType == PPEDIOP_EGAIS_REQUESTREPEALWB) {
						RepealWb * p_req = static_cast<RepealWb *>(rPack.P_Data);
						if(p_req) {
							THROW(P_BObj->Search(p_req->BillID, &bill_rec) > 0);
							rAck.Id.ToStr(S_GUID::fmtIDL, temp_buf);
							THROW(tag_item.SetStr(PPTAG_BILL_EDIREPEALACK, temp_buf));
							THROW(p_ref->Ot.PutTag(PPOBJ_BILL, p_req->BillID, &tag_item, 0));
						}
					}
					else if(edi_op_list.lsearch(rPack.DocType)) {
						PPBillPacket * p_bp = static_cast<PPBillPacket *>(rPack.P_Data);
						if(p_bp) {
							THROW(P_BObj->Search(p_bp->Rec.ID, &bill_rec) > 0);
							{
								const long org_flags2 = bill_rec.Flags2;
								bill_rec.Flags2 |= BILLF2_ACKPENDING;
								THROW(P_BObj->P_Tbl->SetRecFlag2(p_bp->Rec.ID, BILLF2_ACKPENDING, 1, 0));
								if(oneof4(rPack.DocType, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4)) {
									const int recadv_status = BillCore::GetRecadvStatus(p_bp->Rec);
									BillCore::SetRecadvStatus(recadv_status, bill_rec);
								}
								if(bill_rec.Flags2 != org_flags2) {
									THROW_DB(P_BObj->P_Tbl->rereadForUpdate(0, 0));
									THROW_DB(P_BObj->P_Tbl->updateRecBuf(&bill_rec)); // @sfu
								}
							}
							{
								rAck.Id.ToStr(S_GUID::fmtIDL, temp_buf);
								if(oneof4(rPack.DocType, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4) &&
									!p_bp->Rec.EdiOp && p_bp->Rec.Flags2 & BILLF2_DECLINED) {
									//
									// Для отказа от собственного документа необходимо установить специальный тег квитанции
									//
									THROW(tag_item.SetStr(PPTAG_BILL_EDIREJECTACK, temp_buf));
								}
								else {
									THROW(tag_item.SetStr(PPTAG_BILL_EDIACK, temp_buf));
								}
								THROW(p_ref->Ot.PutTag(PPOBJ_BILL, p_bp->Rec.ID, &tag_item, 0));
							}
						}
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryClients(PPID locID, int queryby, const char * pQ)
{
	int    ok = -1;
	THROW_INVARG(oneof2(queryby, querybyINN, querybyCode) && !isempty(pQ));
	{
		Ack    ack;
		Packet qp(PPEDIOP_EGAIS_QUERYCLIENTS);
		if(qp.P_Data) {
           	// Запрос контрагента по идентификатору: "СИО"
           	// Запрос товара по коду ЕГАИС: "КОД"
			SString code_buf;
			int   code_txt_id = 0;
			if(queryby == querybyINN)
				code_txt_id = PPTXT_EGAIS_QP_INN; // "ИНН"
			else if(queryby == querybyCode)
				code_txt_id = PPTXT_EGAIS_QP_CIO; // "СИО"
			else
				assert(0);
			if(code_txt_id && PPLoadText(code_txt_id, code_buf)) {
				static_cast<StrStrAssocArray *>(qp.P_Data)->Add(code_buf, pQ);
			}
		}
		THROW(PutQuery(qp, locID, "QueryPartner", ack));
	}
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryRests(PPID locID, const char *)
{
	int    ok = -1;
	Ack    ack;
	const  int __v2 = BIN((Cfg.E.Flags & Cfg.fEgaisVer2Fmt) || (State & stUseEgaisVer3));
    Packet qp(__v2 ? PPEDIOP_EGAIS_QUERYRESTS_V2 : PPEDIOP_EGAIS_QUERYRESTS);
	THROW(PutQuery(qp, locID, __v2 ? "QueryRests_v2" : "QueryRests", ack));
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryRestsShop(PPID locID, const char *)
{
	int    ok = -1;
	Ack    ack;
    Packet qp(PPEDIOP_EGAIS_QUERYRESTSSHOP);
	THROW(PutQuery(qp, locID, "QueryRestsShop_v2", ack));
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryProducts(PPID locID, int queryby, const char * pQ)
{
	int    ok = -1;
	THROW_INVARG(oneof2(queryby, querybyINN, querybyCode) && !isempty(pQ));
	{
		Ack    ack;
		Packet qp(PPEDIOP_EGAIS_QUERYAP);
		if(qp.P_Data) {
			SString code_buf;
			int   code_txt_id = 0;
			if(queryby == querybyINN)
				code_txt_id = PPTXT_EGAIS_QP_INN; // "ИНН"
			else if(queryby == querybyCode)
				code_txt_id = PPTXT_EGAIS_QP_KOD; // "КОД"
			else
				assert(0);
			if(PPLoadText(code_txt_id, code_buf))
				static_cast<StrStrAssocArray *>(qp.P_Data)->Add(code_buf, pQ);
		}
		THROW(PutQuery(qp, locID, "QueryAP", ack));
	}
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryInfA(PPID locID, const char * pInfA)
{
	int    ok = -1;
	Ack    ack;
    Packet qp(/*PPEDIOP_EGAIS_QUERYFORMA*/PPEDIOP_EGAIS_QUERYFORMF1); // @v11.2.12 PPEDIOP_EGAIS_QUERYFORMA-->PPEDIOP_EGAIS_QUERYFORMF1
	if(qp.P_Data) {
        *static_cast<SString *>(qp.P_Data) = pInfA;
	}
	THROW(PutQuery(qp, locID, /*"QueryFormA"*/"QueryFormF1", ack)); // @v11.2.12 "QueryFormA"-->"QueryFormF1"
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::QueryInfB(PPID locID, const char * pInfB)
{
	int    ok = -1;
	Ack    ack;
    Packet qp(/*PPEDIOP_EGAIS_QUERYFORMB*/PPEDIOP_EGAIS_QUERYFORMF2); // @v11.2.12 PPEDIOP_EGAIS_QUERYFORMB-->PPEDIOP_EGAIS_QUERYFORMF2
	if(qp.P_Data)
		*static_cast<SString *>(qp.P_Data) = pInfB;
	THROW(PutQuery(qp, locID, /*"QueryFormB"*/"QueryFormF2", ack)); // @v11.2.12 "QueryFormB"-->"QueryFormF2"
	CATCHZOK
    return ok;
}

PPEgaisProcessor::PPEgaisProcessor(long cflags, PPLogger * pOuterLogger, int __reserve) : PrcssrAlcReport(),
	PPEmbeddedLogger((cflags & cfDirectFileLogging) ? PPEmbeddedLogger::ctrfDirectLogging : 0, pOuterLogger, PPFILNAM_EGAIS_LOG, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER),
	State(0), P_LecT(0), P_UtmEntry(0), P_Taw(0)
{
	{
		//
		// Инициализация процессора замены наименований входящих товаров
		//
		SString temp_buf;
		SString file_name;
		SString path;
		PPGetFileName(PPFILNAM_REPLACERRULEGOODS, file_name);
		PPGetPath(PPPATH_DD, path);
		(temp_buf = path).SetLastSlash().Cat("local").SetLastSlash().Cat(file_name);
		if(!fileExists(temp_buf))
			(temp_buf = path).SetLastSlash().Cat(file_name);
		if(fileExists(temp_buf)) {
			P_Taw = new PPTextAnalyzerWrapper;
			if(P_Taw && !P_Taw->Init(temp_buf, PPTextAnalyzerWrapper::fEncInner))
				ZDELETE(P_Taw);
		}
	}
	P_Las = new PPLocAddrStruc(0, 0);
	THROW(SetConfig(0));
	THROW(Init());
	if(cflags & cfDebugMode) {
		State |= stTestSendingMode;
	}
	if(cflags & cfUseVerByConfig) {
		SETFLAG(State, stUseEgaisVer3, BIN(Cfg.E.Flags & Cfg.fEgaisVer3Fmt));
		SETFLAG(State, stUseEgaisVer4, BIN(Cfg.E.Flags & Cfg.fEgaisVer4Fmt)); // @v11.0.12
	}
	else {
		SETFLAG(State, stUseEgaisVer3, BIN(cflags & cfVer3));
		SETFLAG(State, stUseEgaisVer4, BIN(cflags & cfVer4)); // @v11.0.12
	}
	if(DS.CheckExtFlag(ECF_OPENSOURCE))
		State |= stValidLic;
	else {
		PPLicData ld;
		if(PPGetLicData(&ld) > 0 && ld.ExtFunc & PPLicData::effEgais)
			State |= stValidLic;
	}
	CATCH
		State |= stError;
	ENDCATCH
}

PPEgaisProcessor::~PPEgaisProcessor()
{
	P_UtmEntry = 0;
	delete P_LecT;
	delete P_Las;
	delete P_Taw;
}

bool PPEgaisProcessor::operator !() const { return LOGIC(State & stError); }
void PPEgaisProcessor::SetTestSendingMode(bool set) { SETFLAG(State, stTestSendingMode, set); }
bool PPEgaisProcessor::GetTestSendingMode() const { return LOGIC(State & stTestSendingMode); }
void PPEgaisProcessor::SetNonRvmTagMode(bool set) { SETFLAG(State, stDontRemoveTags, set); }
int  PPEgaisProcessor::CheckLic() const { return (State & stValidLic) ? 1 : PPSetError(PPERR_EGAIS_NOLIC); }

const SString & FASTCALL PPEgaisProcessor::EncText(const char * pS)
{
	EncBuf = pS;
	EncBuf.ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(EncBuf, "&<>\'");
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

const SString & FASTCALL PPEgaisProcessor::EncText(const SString & rS)
{
	(EncBuf = rS).ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(EncBuf, "&<>\'");
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

static const SIntToSymbTabEntry _EgaisDocTypes[] = {
	{ PPEDIOP_EGAIS_TICKET,           "Ticket" },
	{ PPEDIOP_EGAIS_WAYBILLACT,       "WayBillAct" },
	{ PPEDIOP_EGAIS_WAYBILL,          "WayBill" },
	{ PPEDIOP_EGAIS_CONFIRMTICKET,    "ConfirmTicket" },
	{ PPEDIOP_EGAIS_TTNINFORMBREG,    "TTNInformBReg" },
	{ PPEDIOP_EGAIS_ACTREJECT,        "ActReject" },
	{ PPEDIOP_EGAIS_ACTUSING,         "ActUsing" },
	{ PPEDIOP_EGAIS_ACTINVENTORYPARTIAL,     "ActInventoryPartial" },
	{ PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG,  "ActInventoryInformBReg" },
	{ PPEDIOP_EGAIS_ACTINVENTORYINFORMF2REG, "ActInventoryInformF2Reg" },
	{ PPEDIOP_EGAIS_ACTINVENTORY,     "ActInventory" },
	{ PPEDIOP_EGAIS_QUERYAP,          "QueryAP" },
	{ PPEDIOP_EGAIS_QUERYSSP,         "QuerySSP" },
	{ PPEDIOP_EGAIS_QUERYSP,          "QuerySP" },
	{ PPEDIOP_EGAIS_QUERYCLIENTS,     "QueryClients" },
	{ PPEDIOP_EGAIS_QUERYRESTS,       "QueryRests" },
	{ PPEDIOP_EGAIS_QUERYRESTS_V2,    "QueryRests_v2" },
	{ PPEDIOP_EGAIS_QUERYRESTSSHOP,   "QueryRestsShop_v2" },
	{ PPEDIOP_EGAIS_QUERYFORMA,       "QueryFormA" },
	{ PPEDIOP_EGAIS_QUERYFORMB,       "QueryFormB" },
	{ PPEDIOP_EGAIS_QUERYFORMF1,      "QueryFormF1" }, // @v11.2.12
	{ PPEDIOP_EGAIS_QUERYFORMF2,      "QueryFormF2" }, // @v11.2.12
	{ PPEDIOP_EGAIS_REPLYSSP,         "ReplySSP" },
	{ PPEDIOP_EGAIS_REPLYSPIRIT,      "ReplySpirit" },
	{ PPEDIOP_EGAIS_REPLYCLIENT,      "ReplyClient" },
	{ PPEDIOP_EGAIS_REPLYAP,          "ReplyAP" },
	{ PPEDIOP_EGAIS_REPLYRESTS,       "ReplyRests" },
	{ PPEDIOP_EGAIS_REPLYRESTS_V2,    "ReplyRests_v2" },
	{ PPEDIOP_EGAIS_REPLYRESTSSHOP,   "ReplyRestsShop_v2" },
	{ PPEDIOP_EGAIS_REPLYFORMA,       "ReplyFormA" },
	{ PPEDIOP_EGAIS_REPLYFORMB,       "ReplyFormB" },
	{ PPEDIOP_EGAIS_REPLYFORM1,       "ReplyForm1" }, // @v11.3.4
	{ PPEDIOP_EGAIS_REPLYFORM2,       "ReplyForm2" }, // @v11.3.4
	{ PPEDIOP_EGAIS_ACTCHARGEON,      "ActChargeOn" },
	{ PPEDIOP_EGAIS_ACTCHARGEON_V2,   "ActChargeOn_v2" },
	{ PPEDIOP_EGAIS_ACTWRITEOFF,      "ActWriteOff" },
	{ PPEDIOP_EGAIS_ACTWRITEOFF_V2,   "ActWriteOff_v2" },
	{ PPEDIOP_EGAIS_REQUESTREPEALWB,  "RequestRepealWB" },
	{ PPEDIOP_EGAIS_REQUESTREPEALAWO, "RequestRepealAWO" },
	{ PPEDIOP_EGAIS_CONFIRMREPEALWB,  "ConfirmRepealWB" },
	{ PPEDIOP_EGAIS_QUERYBARCODE,     "QueryBarcode" },
	{ PPEDIOP_EGAIS_REPLYBARCODE,     "ReplyBarcode" },
	{ PPEDIOP_EGAIS_TRANSFERTOSHOP,   "TransferToShop" },
	{ PPEDIOP_EGAIS_TRANSFERFROMSHOP, "TransferFromShop" },
	{ PPEDIOP_EGAIS_ACTCHARGEONSHOP,  "ActChargeOnShop_v2" },
	{ PPEDIOP_EGAIS_ACTWRITEOFFSHOP,  "ActWriteOffShop_v2" },
	{ PPEDIOP_EGAIS_WAYBILL_V2,       "WayBill_v2" },
	{ PPEDIOP_EGAIS_TTNINFORMF2REG,   "TTNInformF2Reg" },
	{ PPEDIOP_EGAIS_WAYBILLACT_V2,    "WayBillAct_v2" },
	{ PPEDIOP_EGAIS_NOTIFY_WBVER2,    "InfoVersionTTN" },
	{ PPEDIOP_EGAIS_NOTIFY_WBVER3,    "InfoVersionTTN" },
	{ PPEDIOP_EGAIS_WAYBILL_V3,       "WayBill_v3" },
	{ PPEDIOP_EGAIS_WAYBILLACT_V3,    "WayBillAct_v3" },
	{ PPEDIOP_EGAIS_ACTWRITEOFF_V3,   "ActWriteOff_v3" },
	{ PPEDIOP_EGAIS_QUERYRESENDDOC,   "QueryResendDoc" },
	{ PPEDIOP_EGAIS_QUERYRESTBCODE,   "QueryRestBCode" },
	{ PPEDIOP_EGAIS_REPLYRESTBCODE,   "ReplyRestBCode" },
	{ PPEDIOP_EGAIS_ACTFIXBARCODE,    "ActFixBarCode"  },
	{ PPEDIOP_EGAIS_ACTUNFIXBARCODE,  "ActUnFixBarCode" },
	{ PPEDIOP_EGAIS_NOTIFY_WBVER4,    "InfoVersionTTN" }, // @v11.0.12
	{ PPEDIOP_EGAIS_WAYBILL_V4,       "WayBill_v4" }, // @v11.0.12
	{ PPEDIOP_EGAIS_WAYBILLACT_V4,    "WayBillAct_v4" }, // @v11.0.12
};


/*static*/int FASTCALL PPEgaisProcessor::GetDocTypeTag(int docType, SString & rTag)
{
	return SIntToSymbTab_GetSymb(_EgaisDocTypes, SIZEOFARRAY(_EgaisDocTypes), docType, rTag);
}

/*static*/int FASTCALL PPEgaisProcessor::RecognizeDocTypeTag(const char * pTag)
{
	int    doc_type = 0;
	if(!isempty(pTag)) {
		SString ns, ntag;
		SString tag;
		if((tag = pTag).Strip().Divide(':', ns, ntag) > 0) {
			tag = ntag;
		}
		if(tag.NotEmptyS()) {
			doc_type = SIntToSymbTab_GetId(_EgaisDocTypes, SIZEOFARRAY(_EgaisDocTypes), tag);
			if(!doc_type) {
				if(tag.IsEqiAscii("FORMBREGINFO"))
					doc_type = PPEDIOP_EGAIS_TTNINFORMBREG;
				else if(tag.IsEqiAscii("FORM2REGINFO"))
					doc_type = PPEDIOP_EGAIS_TTNINFORMF2REG;
				else if(tag.IsEqiAscii("ReplyPartner"))
					doc_type = PPEDIOP_EGAIS_REPLYCLIENT;
				else if(tag.IsEqiAscii("ReplyAP"))
					doc_type = PPEDIOP_EGAIS_REPLYAP;
				else if(tag.IsEqiAscii("INVENTORYREGINFO"))
					doc_type = PPEDIOP_EGAIS_ACTINVENTORYINFORMF2REG;
				else if(tag.IsEqiAscii("WayBillTicket"))
					doc_type = PPEDIOP_EGAIS_CONFIRMTICKET;
			}
		}
	}
    return doc_type;
}
//
//
//
static const SIntToSymbTabEntry _EgaisWayBillTypes[] = {
	{ PPEgaisProcessor::wbtInvcFromMe, "WBInvoiceFromMe" },
	{ PPEgaisProcessor::wbtInvcToMe, "WBInvoiceToMe" },
	{ PPEgaisProcessor::wbtRetFromMe, "WBReturnFromMe" },
	{ PPEgaisProcessor::wbtRetToMe, "WBReturnToMe" }
};

/*static*/bool FASTCALL PPEgaisProcessor::GetWayBillTypeText(int wbType, SString & rBuf)
	{ return SIntToSymbTab_GetSymb(_EgaisWayBillTypes, SIZEOFARRAY(_EgaisWayBillTypes), wbType, rBuf); }
int FASTCALL PPEgaisProcessor::RecognizeWayBillTypeText(const char * pText)
	{ return SIntToSymbTab_GetId(_EgaisWayBillTypes, SIZEOFARRAY(_EgaisWayBillTypes), pText); }

int PPEgaisProcessor::WriteOrgInfo(SXml::WDoc & rXmlDoc, const char * pScopeXmlTag, PPID personID, PPID addrLocID, LDATE actualDate, long flags)
{
	// woifDontSendWithoutFSRARID
	int   ok = 1;
	Reference * p_ref = PPRef;
	PPPersonPacket psn_pack;
	SString temp_buf;
	SString inn, kpp;
	SString rar_id;
	SString info_org_name; // Наименование организации для вывода информации
	RegisterTbl::Rec reg_rec;
	int   j_status = 0; // 1 - росс юр, 2 - росс ип, 3 - иностранец (не ТС), 4 - иностранец (таможенный союз)
	const char * p_j_scope = 0;
	THROW(PsnObj.GetPacket(personID, &psn_pack, PGETPCKF_USEINHERITENCE) > 0);
	info_org_name = psn_pack.Rec.Name;
	{
		ObjTagItem tag_item;
		if(addrLocID)
			p_ref->Ot.GetTagStr(PPOBJ_LOCATION, addrLocID, PPTAG_LOC_FSRARID, rar_id);
		if(!rar_id.NotEmptyS())
			p_ref->Ot.GetTagStr(PPOBJ_PERSON, personID, PPTAG_PERSON_FSRARID, rar_id);
		if(flags & woifStrict) {
			THROW_PP_S(rar_id.NotEmptyS(), PPERR_EGAIS_PERSONFSRARIDUNDEF, info_org_name);
		}
		else if(flags & woifDontSendWithoutFSRARID) {
			if(!rar_id.NotEmptyS()) {
				LogTextWithAddendum(PPTXT_EGAIS_PERSONHASNTRARID, info_org_name);
                ok = -1;
			}
		}
	}
	if(ok > 0) {
		EgaisPersonCore::Item epr_item;
		LocationTbl::Rec __loc_rec;
		PPID   __loc_id = 0;
		SString __full_addr_buf;
		if(P_RefC && rar_id.NotEmpty()) {
			TSVector <EgaisPersonTbl::Rec> epr_list;
			const int psc_idx = P_RefC->PsC.SearchByCode(rar_id, epr_list);
			if(psc_idx > 0) {
                const EgaisPersonTbl::Rec & r_rec = epr_list.at(psc_idx-1);
                P_RefC->PsC.RecToItem(r_rec, epr_item);
			}
		}
		if(epr_item.ID) {
			inn = epr_item.INN;
			kpp = epr_item.KPP;
			if(epr_item.CountryCode == 643 && inn.Len() == 10) {
				j_status = 1;
				p_j_scope = "UL";
			}
			else if(epr_item.CountryCode == 643 && inn.Len() == 12) {
				j_status = 2;
				p_j_scope = "FL";
			}
			else if(epr_item.RNN[0] || epr_item.UNP[0]) {
				j_status = 4;
				p_j_scope = "TS";
			}
			else {
				j_status = 3;
				p_j_scope = "FO";
			}
		}
		else {
			//
			if(psn_pack.Regs.GetRegister(PPREGT_TPID, actualDate, 0, &reg_rec) > 0)
				(inn = reg_rec.Num).Strip();
			if(GetWkrRegister(wkrKPP, personID, addrLocID, actualDate, &reg_rec) > 0)
				kpp = reg_rec.Num;
			if(inn.Len() == 12) {
				j_status = 2;
				p_j_scope = "FL";
			}
			else {
				j_status = 1;
				p_j_scope = "UL";
			}
		}
		if(flags & woifStrict && oneof2(j_status, 1, 2)) {
			THROW_PP_S(inn.NotEmptyS(), PPERR_PERSONINNUNDEF, info_org_name);
			THROW_PP_S(kpp.NotEmptyS() || inn.Len() == 12, PPERR_EGAIS_PERSONKPPUNDEF, info_org_name);
		}
		//
		if(addrLocID && PsnObj.LocObj.Search(addrLocID, &__loc_rec) > 0)
			__loc_id = addrLocID;
		else if(psn_pack.Rec.RLoc) {
			__loc_id = psn_pack.RLoc.ID;
			__loc_rec = psn_pack.RLoc;
		}
		else if(psn_pack.Rec.MainLoc) {
			__loc_id = psn_pack.Loc.ID;
			__loc_rec = psn_pack.Loc;
		}
		if(__loc_id)
			PsnObj.LocObj.P_Tbl->GetAddress(__loc_rec, 0, __full_addr_buf);
		else
			__full_addr_buf = 0;
		P_Las->Recognize((temp_buf = __full_addr_buf).Transf(CTRANSF_INNER_TO_OUTER));
		//
		SXml::WNode w_s(rXmlDoc, pScopeXmlTag);
		if(flags & woifVersion2) {
			SXml::WNode w_j(rXmlDoc, SXml::nst("oref", p_j_scope));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "ClientRegId"), EncText(rar_id));
			if(epr_item.ID) {
				w_s.PutInner(SXml::nst("oref", "FullName"), EncText(epr_item.FullName));
				w_s.PutInner(SXml::nst("oref", "ShortName"), EncText((temp_buf = epr_item.Name).Trim(64)));
				if(j_status == 1) {
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "KPP"), EncText(kpp));
				}
				else if(j_status == 2) {
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
				}
				else if(j_status == 3) {
					if(epr_item.RNN[0])
						w_s.PutInnerSkipEmpty(SXml::nst("oref", "TSNUM"), EncText(epr_item.RNN));
					else if(epr_item.UNP[0])
						w_s.PutInnerSkipEmpty(SXml::nst("oref", "TSNUM"), EncText(epr_item.UNP));
				}
				{
					SXml::WNode w_a(rXmlDoc, SXml::nst("oref", "address"));
					w_a.PutInner(SXml::nst("oref", "Country"), EncText(temp_buf.Z().CatLongZ(epr_item.CountryCode, 3)));
					if(epr_item.RegionCode)
						w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf.Z().CatLongZ(epr_item.RegionCode, 2)));
					else if(inn.Len() > 2) {
						inn.Sub(0, 2, temp_buf);
						w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf));
					}
					else if(oneof2(j_status, 1, 2)) {
						LogTextWithAddendum(PPTXT_EGAIS_PERSONHASNTREGCODE, temp_buf = epr_item.Name);
					}
					w_a.PutInnerSkipEmpty(SXml::nst("oref", "area"), "");
					w_a.PutInner(SXml::nst("oref", "description"), EncText(temp_buf = epr_item.AddressDescr));
				}
			}
			else {
				if(!psn_pack.GetExtName(temp_buf))
					temp_buf = psn_pack.Rec.Name;
				w_s.PutInner(SXml::nst("oref", "FullName"), EncText(temp_buf));
				w_s.PutInner(SXml::nst("oref", "ShortName"), EncText((temp_buf = psn_pack.Rec.Name).Trim(64)));
				//
				if(j_status == 1) {
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "KPP"), EncText(kpp));
				}
				else if(j_status == 2) {
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
				}
				{
					SXml::WNode w_a(rXmlDoc, SXml::nst("oref", "address"));
					{
						PPCountryBlock cntryb;
						PsnObj.LocObj.GetCountry(&__loc_rec, 0, &cntryb);
						{
							const long country_code = (temp_buf = cntryb.Code.NotEmptyS() ? cntryb.Code.cptr() : "643").ToLong();
							temp_buf.Z().CatLongZ(country_code, 3);
							w_a.PutInner(SXml::nst("oref", "Country"), EncText(temp_buf)); // По умолчанию 643 (Россия)
						}
					}
					{
						//
						// ИНН и КПП для России всегда содержать в виде префикса код региона.
						// Поэтому не будем пытаться получить этот год из адреса, а используем КПП (с приоритетом) или,
						// если КПП пустой - ИНН.
						//
						if(kpp.Len() > 2)
							kpp.Sub(0, 2, temp_buf);
						else if(inn.Len() > 2)
							inn.Sub(0, 2, temp_buf);
						else
							temp_buf.Z();
						w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf));
					}
					w_a.PutInnerSkipEmpty(SXml::nst("oref", "area"), "");
					w_a.PutInner(SXml::nst("oref", "description"), EncText(__full_addr_buf));
				}
			}
		}
		else {
			// oref:
			w_s.PutInner(SXml::nst("oref", "Identity"), temp_buf.Z().Cat(personID));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "ClientRegId"), EncText(rar_id));
			if(!psn_pack.GetExtName(temp_buf))
				temp_buf = psn_pack.Rec.Name;
			w_s.PutInner(SXml::nst("oref", "FullName"), EncText(temp_buf));
			w_s.PutInner(SXml::nst("oref", "ShortName"), EncText((temp_buf = psn_pack.Rec.Name).Trim(64)));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "KPP"), EncText(kpp));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "UNP"), ""); // Для Белоруси
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "RNN"), ""); // Для Казахстана
			{
				SXml::WNode w_a(rXmlDoc, SXml::nst("oref", "address"));
				PPCountryBlock cntryb;
				PsnObj.LocObj.GetCountry(&__loc_rec, 0, &cntryb);
				{
					long country_code = (temp_buf = cntryb.Code.NotEmptyS() ? cntryb.Code.cptr() : "643").ToLong();
					temp_buf.Z().CatLongZ(country_code, 3);
					w_a.PutInner(SXml::nst("oref", "Country"), EncText(temp_buf)); // По умолчанию 643 (Россия)
				}
				LocationCore::GetExField(&__loc_rec, LOCEXSTR_ZIP, temp_buf);
				if(!temp_buf.IsDec() || temp_buf.Len() != 6) // Проверка на "битый" почтовый индекс
					temp_buf.Z();
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "Index"), EncText(temp_buf));
				//
				// ИНН и КПП для России всегда содержать в виде префикса код региона.
				// Поэтому не будем пытаться получить этот год из адреса, а используем КПП (с приоритетом) или,
				// если КПП пустой - ИНН.
				//
				if(kpp.Len() > 2)
					kpp.Sub(0, 2, temp_buf);
				else if(inn.Len() > 2)
					inn.Sub(0, 2, temp_buf);
				else
					temp_buf.Z();
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf));
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "area"), "");
				PsnObj.LocObj.GetCity(__loc_id, 0, &temp_buf, 1);
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "city"), EncText(temp_buf));
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "place"), "");
				if(P_Las) {
					P_Las->GetText(P_Las->tStreet, temp_buf);
					temp_buf.Strip().Transf(CTRANSF_OUTER_TO_INNER);
				}
				else
					temp_buf.Z();
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "street"), EncText(temp_buf));
				if(P_Las) {
					P_Las->GetText(P_Las->tHouse, temp_buf);
					temp_buf.Strip().Transf(CTRANSF_OUTER_TO_INNER);
				}
				else
					temp_buf.Z();
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "house"), EncText(temp_buf));
				if(P_Las) {
					P_Las->GetText(P_Las->tHouseAddendum, temp_buf);
					temp_buf.Strip().Transf(CTRANSF_OUTER_TO_INNER);
				}
				else
					temp_buf.Z();
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "building"), EncText(temp_buf));
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "liter"), "");
				w_a.PutInner(SXml::nst("oref", "description"), EncText(__full_addr_buf));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::WriteOrgInfo(SXml::WDoc & rXmlDoc, const char * pScopeXmlTag, const EgaisPersonCore::Item & rRefcItem, long flags)
{
	// woifDontSendWithoutFSRARID
	int   ok = 1;
	SString temp_buf;
	SString inn(rRefcItem.INN);
	SString kpp(rRefcItem.KPP);
	SString rar_id(rRefcItem.RarIdent);
	int   j_status = 0; // 1 - росс юр, 2 - росс ип, 3 - иностранец (не ТС), 4 - иностранец (таможенный союз)
	const char * p_j_scope = 0;
	if(flags & woifStrict) {
		THROW_PP_S(rar_id.NotEmptyS(), PPERR_EGAIS_PERSONFSRARIDUNDEF, rRefcItem.Name);
	}
	else if(flags & woifDontSendWithoutFSRARID) {
		if(!rar_id.NotEmptyS()) {
			LogTextWithAddendum(PPTXT_EGAIS_PERSONHASNTRARID, temp_buf = rRefcItem.Name);
            ok = -1;
		}
	}
	if(ok > 0) {
		if(rRefcItem.CountryCode == 643 && inn.Len() == 10) {
			j_status = 1;
			p_j_scope = "UL";
		}
		else if(rRefcItem.CountryCode == 643 && inn.Len() == 12) {
			j_status = 2;
			p_j_scope = "FL";
		}
		else if(rRefcItem.RNN[0] || rRefcItem.UNP[0]) {
			j_status = 4;
			p_j_scope = "TS";
		}
		else {
			j_status = 3;
			p_j_scope = "FO";
		}
		SXml::WNode w_s(rXmlDoc, pScopeXmlTag);
		if(flags & woifVersion2) {
			SXml::WNode w_j(rXmlDoc, SXml::nst("oref", p_j_scope));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "ClientRegId"), EncText(rar_id));
			w_s.PutInner(SXml::nst("oref", "FullName"), EncText(rRefcItem.FullName));
			w_s.PutInner(SXml::nst("oref", "ShortName"), EncText((temp_buf = rRefcItem.Name).Trim(64)));
			if(j_status == 1) {
				THROW_PP_S(!(flags & woifStrict) || inn.NotEmptyS(), PPERR_PERSONINNUNDEF, rRefcItem.Name);
				w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
				THROW_PP_S(!(flags & woifStrict) || (kpp.NotEmptyS() || inn.Len() == 12), PPERR_EGAIS_PERSONKPPUNDEF, rRefcItem.Name);
				w_s.PutInnerSkipEmpty(SXml::nst("oref", "KPP"), EncText(kpp));
			}
			else if(j_status == 2) {
				THROW_PP_S(!(flags & woifStrict) || inn.NotEmptyS(), PPERR_PERSONINNUNDEF, rRefcItem.Name);
				w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
			}
			else if(j_status == 3) {
				if(rRefcItem.RNN[0])
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "TSNUM"), EncText(rRefcItem.RNN));
				else if(rRefcItem.UNP[0])
					w_s.PutInnerSkipEmpty(SXml::nst("oref", "TSNUM"), EncText(rRefcItem.UNP));
			}
			{
				SXml::WNode w_a(rXmlDoc, SXml::nst("oref", "address"));
				w_a.PutInner(SXml::nst("oref", "Country"), EncText(temp_buf.Z().CatLongZ(rRefcItem.CountryCode, 3)));
				if(rRefcItem.RegionCode)
					w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf.Z().CatLongZ(rRefcItem.RegionCode, 2)));
				else if(oneof2(j_status, 1, 2)) {
					LogTextWithAddendum(PPTXT_EGAIS_PERSONHASNTREGCODE, temp_buf = rRefcItem.Name);
				}
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "area"), "");
				w_a.PutInner(SXml::nst("oref", "description"), EncText(temp_buf = rRefcItem.AddressDescr));
				/*if(temp_buf.IsEmpty()) {
					LogTextWithAddendum(PPTXT_EGAIS_PERSONHASNTREGCODE, temp_buf = rRefcItem.Name);
				}*/
			}
		}
		else {
			// oref:
			w_s.PutInner(SXml::nst("oref", "Identity"), temp_buf.Z().Cat(rRefcItem.ID));
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "ClientRegId"), EncText(rar_id));
			w_s.PutInner(SXml::nst("oref", "FullName"), EncText(rRefcItem.FullName));
			w_s.PutInner(SXml::nst("oref", "ShortName"), EncText((temp_buf = rRefcItem.Name).Trim(64)));
			{
				THROW_PP_S(!(flags & woifStrict) || inn.NotEmptyS(), PPERR_PERSONINNUNDEF, rRefcItem.Name);
				w_s.PutInnerSkipEmpty(SXml::nst("oref", "INN"), EncText(inn));
			}
			{
				THROW_PP_S(!(flags & woifStrict) || (kpp.NotEmptyS() || inn.Len() == 12), PPERR_EGAIS_PERSONKPPUNDEF, rRefcItem.Name);
				w_s.PutInnerSkipEmpty(SXml::nst("oref", "KPP"), EncText(kpp));
			}
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "UNP"), EncText(rRefcItem.UNP)); // Для Белоруси
			w_s.PutInnerSkipEmpty(SXml::nst("oref", "RNN"), EncText(rRefcItem.RNN)); // Для Казахстана
			{
				SXml::WNode w_a(rXmlDoc, SXml::nst("oref", "address"));
				w_a.PutInner(SXml::nst("oref", "Country"), EncText(temp_buf.Z().CatLongZ(rRefcItem.CountryCode, 3)));
				if(rRefcItem.RegionCode)
					w_a.PutInnerSkipEmpty(SXml::nst("oref", "RegionCode"), EncText(temp_buf.Z().CatLongZ(rRefcItem.RegionCode, 2)));
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "area"), "");
				w_a.PutInnerSkipEmpty(SXml::nst("oref", "description"), EncText(temp_buf = rRefcItem.AddressDescr));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrAlcReport::GetEgaisPersonByCode(const char * pCode, EgaisPersonCore::Item & rItem)
{
	int    ok = -1;
	rItem.Clear();
	if(!isempty(pCode) && P_RefC) {
		TSVector <EgaisPersonTbl::Rec> epr_list;
		const int idx = P_RefC->PsC.SearchByCode(pCode, epr_list);
		if(idx > 0) {
            const EgaisPersonTbl::Rec & r_rec = epr_list.at(idx-1);
            P_RefC->PsC.RecToItem(r_rec, rItem);
			ok = 1;
		}
	}
	return ok;
}

int PPEgaisProcessor::WriteProductInfo(SXml::WDoc & rXmlDoc, const char * pScopeXmlTag, PPID goodsID, PPID lotID, long flags, const ObjTagList * pLotTagList)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	PrcssrAlcReport::GoodsItem agi;
	if(GObj.Fetch(goodsID, &goods_rec) > 0 && PreprocessGoodsItem(goodsID, lotID, pLotTagList, 0, agi) > 0) {
		const int use_refc_data = UseOwnEgaisObjects();
		SString temp_buf, added_msg_buf;
		PPID   manuf_id = 0;
		PPID   importer_id = 0;
		int    manuf_done = 0;
		SXml::WNode w_s(rXmlDoc, pScopeXmlTag /*"wb:Product"*/);
		if(!(flags & wpifVersion2))
			w_s.PutInner(SXml::nst("pref", "Identity"), temp_buf.Z().Cat(goodsID));
		if(flags & wpifVersion2)
			w_s.PutInner(SXml::nst("pref", "UnitType"), EncText((temp_buf = (agi.UnpackedVolume > 0.0) ? "Unpacked" : "Packed").Transf(CTRANSF_OUTER_TO_INNER)));
		{
			PPLoadText(PPTXT_EGAIS_AP, temp_buf);
			w_s.PutInner(SXml::nst("pref", "Type"), EncText(temp_buf)); // АП или что-то еще
		}
		{
			int    names_done = 0;
			if(use_refc_data && agi.RefcProductID) {
				EgaisProductCore::Item item;
                if(P_RefC->PrC.Search(agi.RefcProductID, item) > 0 && item.FullName.NotEmpty()) {
					w_s.PutInner(SXml::nst("pref", "FullName"), EncText(item.FullName));
					temp_buf = item.Name;
					if(!temp_buf.NotEmptyS())
						temp_buf = item.FullName;
					w_s.PutInner(SXml::nst("pref", "ShortName"), EncText(temp_buf.Trim(64)));
					names_done = 1;
                }
			}
			if(!names_done) {
				w_s.PutInner(SXml::nst("pref", "FullName"), EncText(goods_rec.Name));
				w_s.PutInner(SXml::nst("pref", "ShortName"), EncText((temp_buf = goods_rec.Name).Trim(64)));
			}
		}
		if(agi.EgaisCode.IsEmpty()) {
			added_msg_buf.Z().CatEq("Goods", goods_rec.Name).CatDiv(';', 2).CatEq("LotID", lotID);
			LogTextWithAddendum(PPTXT_EGAIS_NOEGAISCODE, added_msg_buf);
		}
		w_s.PutInner(SXml::nst("pref", "AlcCode"), EncText(agi.EgaisCode)); // Код продукта по версии ЕГАИС
		if(agi.UnpackedVolume > 0.0) {
		}
		else {
			const double volume = (use_refc_data && agi.RefcVolume) ? agi.RefcVolume : agi.Volume;
			w_s.PutInner(SXml::nst("pref", "Capacity"), temp_buf.Z().Cat(volume, MKSFMTD(0, 4, 0)));
		}
		{
			const double proof = (use_refc_data && agi.RefcProof) ? agi.RefcProof : agi.Proof;
			w_s.PutInner(SXml::nst("pref", "AlcVolume"), temp_buf.Z().Cat(proof, MKSFMTD_030));
		}
		{
			temp_buf = (use_refc_data && agi.RefcCategoryCode.NotEmpty()) ? agi.RefcCategoryCode : agi.CategoryCode;
			w_s.PutInner(SXml::nst("pref", "ProductVCode"), temp_buf);
		}
		if(flags & wpifPutManufInfo) {
			const  long woif = (flags & wpifVersion2) ? (woifDontSendWithoutFSRARID|woifVersion2) : woifDontSendWithoutFSRARID;
			if(use_refc_data && (agi.RefcImporterCode.NotEmpty() || agi.RefcManufCode.NotEmpty())) {
				EgaisPersonCore::Item epr_item;
				if(GetEgaisPersonByCode(agi.RefcManufCode, epr_item) > 0) {
					WriteOrgInfo(rXmlDoc, SXml::nst("pref", "Producer"), epr_item, woif);
					manuf_done = 1;
				}
				if(!(flags & wpifVersion2) && GetEgaisPersonByCode(agi.RefcImporterCode, epr_item) > 0) {
					WriteOrgInfo(rXmlDoc, SXml::nst("pref", "Importer"), epr_item, woif);
					manuf_done = 1;
				}
			}
			if(!manuf_done) {
				if(lotID) {
					if(Cfg.LotManufTagList.getCount()) {
						ObjTagItem tag_item;
						long   temp_id = 0;
						PersonTbl::Rec psn_rec;
						for(uint i = 0; !temp_id && i < Cfg.LotManufTagList.getCount(); i++) {
							const  PPID tag_id = Cfg.LotManufTagList.get(i);
							if(PPRef->Ot.GetTag(PPOBJ_LOT, lotID, tag_id, &tag_item) > 0)
								tag_item.GetInt(&temp_id);
						}
						if(temp_id && PsnObj.Fetch(temp_id, &psn_rec) > 0) {
							int manuf_type = GetManufPersonType(temp_id);
							if(manuf_type == 1)
								manuf_id = temp_id;
							else if(manuf_type == 2)
								importer_id = temp_id;
						}
					}
				}
				if(manuf_id) {
					WriteOrgInfo(rXmlDoc, SXml::nst("pref", "Producer"), manuf_id, 0, getcurdate_(), woif);
					manuf_done = 1;
				}
				if(importer_id && !(flags & wpifVersion2))
					WriteOrgInfo(rXmlDoc, SXml::nst("pref", "Importer"), importer_id, 0, getcurdate_(), woif);
			}
		}
		if((flags & wpifVersion2) && !manuf_done) {
			added_msg_buf.Z().Cat(agi.EgaisCode).CatDiv('-', 1).Cat(goods_rec.Name);
			LogTextWithAddendum(PPTXT_EGAIS_ALCWAREHASNTMANUF, added_msg_buf);
		}
		ok = 1;
	}
	return ok;
}

int PPEgaisProcessor::WriteInformCode(SXml::WDoc & rXmlDoc, const char * pNs, char informKind, SString & rCode, int docType)
{
	assert(oneof2(informKind, 'A', 'B'));
    int    ok = 1;
    if(oneof2(informKind, 'A', 'B')) {
		int    done = 0;
		SString temp_buf;
		if(oneof2(docType, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) { // @v11.0.12 PPEDIOP_EGAIS_WAYBILL_V4
			if(informKind == 'A') {
				(temp_buf = pNs).Colon().Cat("FARegId");
				SXml::WNode w_s(rXmlDoc, temp_buf, EncText(rCode));
				done = 1;
			}
			else if(informKind == 'B') {
				(temp_buf = pNs).Colon().Cat("Inform").Cat("F2");
			}
		}
		else {
			(temp_buf = pNs).Colon().Cat("Inform");
			if(docType == PPEDIOP_EGAIS_WAYBILL_V2) {
				if(informKind == 'A')
					temp_buf.Cat("F1");
				else if(informKind == 'B')
					temp_buf.Cat("F2");
			}
			else {
				temp_buf.CatChar(informKind);
			}
		}
		if(!done) {
			SXml::WNode w_s(rXmlDoc, temp_buf);
			if(informKind == 'A') {
				w_s.PutInner("pref:RegId", EncText(rCode));
			}
			else {
				if(oneof2(docType, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) { // @v11.0.12 PPEDIOP_EGAIS_WAYBILL_V4
					w_s.PutInner("ce:F2RegId", EncText(rCode));
				}
				else if(docType == PPEDIOP_EGAIS_WAYBILL_V2) {
					SXml::WNode w_i(rXmlDoc, "pref:InformF2Item");
					w_i.PutInner("pref:F2RegId", EncText(rCode));
				}
				else {
					SXml::WNode w_i(rXmlDoc, "pref:InformBItem");
					w_i.PutInner("pref:BRegId", EncText(rCode));
				}
			}
		}
    }
    else
		ok = 0;
    return ok;
}

void PPEgaisProcessor::SetUtmEntry(PPID locID, const UtmEntry * pEntry, const DateRange * pPeriod)
{
	P_UtmEntry = pEntry;
	if(P_UtmEntry) {
		PPID   main_org_id = 0;
		SString fsrar_id;
		SString url_buf;
		SString msg_buf, temp_buf;
		GetURL(locID, url_buf);
		GetFSRARID(locID, fsrar_id, &main_org_id);
		GetPersonName(main_org_id, temp_buf);
		msg_buf.Cat(temp_buf).CatDiv('-', 1);
		if(locID) {
			GetLocationName(locID, temp_buf);
			msg_buf.Cat(temp_buf).CatDiv('-', 1);
		}
		msg_buf.Cat(url_buf).CatDiv('-', 1);
		msg_buf.Cat(fsrar_id);
		if(pPeriod && !pPeriod->IsZero()) {
			msg_buf.CatDiv('-', 1).Cat(*pPeriod, 1);
		}
		Log(msg_buf);
	}
}

int PPEgaisProcessor::GetUtmList(PPID locID, TSVector <UtmEntry> & rList)
{
	rList.clear();
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString fsrar_id;
	SString url_buf;
	PPIDArray main_psn_list;
	PPID   main_org_id = GetMainOrgID();
	if(locID && main_org_id) {
		p_ref->Ot.GetTagStr(PPOBJ_LOCATION, locID, PPTAG_LOC_FSRARID, fsrar_id);
		p_ref->Ot.GetTagStr(PPOBJ_LOCATION, locID, PPTAG_LOC_EGAISSRVURL, url_buf);
		if(fsrar_id.NotEmptyS() && url_buf.NotEmptyS()) {
			UtmEntry new_entry;
			new_entry.Flags |= UtmEntry::fDefault;
			new_entry.MainOrgID = main_org_id;
			STRNSCPY(new_entry.Url, url_buf);
			STRNSCPY(new_entry.FSRARID, fsrar_id);
			THROW_SL(rList.insert(&new_entry));
			ok = 3;
		}
	}
	if(!rList.getCount()) {
		{
			PersonKindTbl * t = &PsnObj.P_Tbl->Kind;
			PersonKindTbl::Key0 k0;
			PersonTbl::Rec psn_rec;
			MEMSZERO(k0);
			k0.KindID = PPPRK_MAIN;
			BExtQuery q(t, 0, 128);
			q.select(t->PersonID, 0).where(t->KindID == PPPRK_MAIN);
			for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
				const  PPID psn_id = t->data.PersonID;
				if(PsnObj.Search(psn_id, &psn_rec) > 0)
					main_psn_list.add(psn_id);
			}
			main_psn_list.sortAndUndup();
			//
			for(uint i = 0; i < main_psn_list.getCount(); i++) {
				const  PPID psn_id = main_psn_list.get(i);
				p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_FSRARID, fsrar_id);
				p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_EGAISSRVURL, url_buf);
				if(fsrar_id.NotEmptyS() && url_buf.NotEmptyS()) {
					UtmEntry new_entry;
					new_entry.MainOrgID = psn_id;
					STRNSCPY(new_entry.Url, url_buf);
					STRNSCPY(new_entry.FSRARID, fsrar_id);
					THROW_SL(rList.insert(&new_entry));
					ok = 2;
				}
			}
		}
		if(!rList.getCount()) {
			fsrar_id = 0;
			url_buf.Z();
			THROW(GetURL(locID, url_buf));
			THROW(GetFSRARID(locID, fsrar_id, &main_org_id));
			{
				UtmEntry new_entry;
				new_entry.Flags |= UtmEntry::fDefault;
				new_entry.MainOrgID = main_org_id;
				STRNSCPY(new_entry.Url, url_buf);
				STRNSCPY(new_entry.FSRARID, fsrar_id);
				THROW_SL(rList.insert(&new_entry));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::GetURL(PPID locID, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(locID && (!P_UtmEntry || P_UtmEntry->Flags & UtmEntry::fDefault)) {
		if(PPRef->Ot.GetTagStr(PPOBJ_LOCATION, locID, PPTAG_LOC_EGAISSRVURL, rBuf) > 0)
			ok = 2;
	}
	if(!ok) {
		if(P_UtmEntry) {
			if(P_UtmEntry->Url[0]) {
				rBuf = P_UtmEntry->Url;
				ok = 3;
			}
		}
		else {
			ACfg.GetExtStrData(ALBATROSEXSTR_EGAISSRVURL, rBuf);
			if(rBuf.NotEmptyS())
				ok = 1;
		}
	}
	return NZOR(ok, PPSetError(PPERR_EGAIS_SRVURLUNDEF));
}

int PPEgaisProcessor::GetFSRARID(PPID locID, SString & rBuf, PPID * pMainOrgID)
{
	rBuf.Z();
	int    ok = 0;
	Reference * p_ref = PPRef;
	ObjTagItem rarid_tag;
	PPID   main_org_id = GetMainOrgID();
	if(locID && (!P_UtmEntry || P_UtmEntry->Flags & UtmEntry::fDefault)) {
		if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, locID, PPTAG_LOC_FSRARID, rBuf) > 0)
			ok = 2;
	}
	if(!ok) {
		if(P_UtmEntry) {
			main_org_id = P_UtmEntry->MainOrgID;
			if(P_UtmEntry->FSRARID[0]) {
				rBuf = P_UtmEntry->FSRARID;
				ok = 3;
			}
		}
		else if(p_ref->Ot.GetTagStr(PPOBJ_PERSON, main_org_id, PPTAG_PERSON_FSRARID, rBuf) > 0)
			ok = 1;
	}
	ASSIGN_PTR(pMainOrgID, main_org_id);
	if(!ok)
		PPSetError(PPERR_EGAIS_FSRARIDUNDEF);
	return ok;
}

int PPEgaisProcessor::Helper_Write(Packet & rPack, PPID locID, xmlTextWriter * pX)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  int doc_type = rPack.DocType;
	SString doc_type_tag;
	PPID   main_org_id = 0;
	SString fsrar_ident;
	SString temp_buf;
	SString bill_text;
	SString lot_text;
	StringSet ss;
	PPLotExtCodeContainer::MarkSet::Entry msentry;
	PPLotExtCodeContainer::MarkSet ext_codes_set;
	PrcssrAlcReport::GoodsItem agi;
	assert(pX);
	THROW_INVARG(pX);
	THROW(GetDocTypeTag(doc_type, doc_type_tag));
	THROW(GetFSRARID(locID, fsrar_ident, &main_org_id));
	if(rPack.P_Data || oneof6(doc_type, PPEDIOP_EGAIS_QUERYRESTS, PPEDIOP_EGAIS_QUERYRESTS_V2, PPEDIOP_EGAIS_QUERYRESTSSHOP,
		PPEDIOP_EGAIS_NOTIFY_WBVER2, PPEDIOP_EGAIS_NOTIFY_WBVER3, PPEDIOP_EGAIS_NOTIFY_WBVER4)) {
		SXml::WDoc _doc(pX, cpUTF8);
		{
			SXml::WNode n_docs(_doc, SXml::nst("ns", "Documents"));
			n_docs.PutAttrib("Version", "1.0");
			{
				static const struct EgaisNsEntry {
					int   Nn;
					const char * P_Ns;
					const char * P_Sub;
				} ns_entries[] = {
					{  1, "ns",   "WB_DOC_SINGLE_01"     },
					{  2, "c",    "Common"               },
					{  3, "oref", "ClientRef"            }, // ambiguity
					{  4, "pref", "ProductRef"           }, // ambiguity
					{  5, "wb",   "TTNSingle"            }, // ambiguity
					{  6, "qp",   "QueryParameters"      }, // ambiguity
					{  7, "qp",   "RequestRepealWB"      }, // ambiguity
					{  8, "wa",   "ActTTNSingle"         }, // ambiguity
					{  9, "ain",  "ActChargeOn"          },
					{ 10, "iab",  "ActInventoryABInfo"   }, // ambiguity
					{ 11, "wt",   "ConfirmTicket"        }, // ambiguity
					{ 12, "wt",   "ConfirmRepealWB"      }, // ambiguity
					{ 13, "awr",  "ActWriteOff"          }, // ambiguity
					{ 14, "qf",   "QueryFormF1F2"        }, // @v11.2.12 "QueryFormAB"-->"QueryFormF1F2"
					{ 15, "bk",   "QueryBarcode"         },
					{ 16, "ce",   "CommonEnum"           }, // ambiguity
					{ 17, "pref", "ProductRef_v2"        }, // ambiguity
					{ 18, "tts",  "TransferToShop"       },
					{ 19, "ainp", "ActChargeOnShop_v2"   }, // ambiguity
					{ 20, "oref", "ClientRef_v2"         }, // ambiguity
					{ 21, "tfs",  "TransferFromShop"     },
					{ 22, "ainp", "ActChargeOn_v2"       }, // ambiguity
					{ 23, "iab",  "ActInventoryF1F2Info" }, // ambiguity
					{ 24, "awr",  "ActWriteOffShop_v2"   }, // ambiguity
					{ 25, "wa",   "ActTTNSingle_v2"      }, // ambiguity
					{ 26, "wb",   "TTNSingle_v2"         }, // ambiguity
					{ 27, "qp",   "InfoVersionTTN"       }, // ambiguity
					{ 28, "awr",  "ActWriteOff_v2"       }, // ambiguity
					{ 29, "wa",   "ActTTNSingle_v3"      }, // ambiguity
					{ 30, "ce",   "CommonV3"             }, // ambiguity
					{ 31, "wb",   "TTNSingle_v3"         }, // ambiguity
					{ 32, "awr",  "ActWriteOff_v3"       }, // ambiguity
					{ 33, "awr",  "ActFixBarCode"        }, // ambiguity
					{ 34, "wa",   "ActTTNSingle_v4"      }, // ambiguity // @v11.0.12
					{ 35, "wb",   "TTNSingle_v4"         }, // ambiguity // @v11.0.12
				};
				const SString fsrar_url_prefix = InetUrl::MkHttp("fsrar.ru", "WEGAIS/"); // "http://fsrar.ru/WEGAIS/"
				n_docs.PutAttrib(SXml::nst_xmlns("xsi"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance")/*"http://www.w3.org/2001/XMLSchema-instance"*/);
				for(uint eidx = 0; eidx < SIZEOFARRAY(ns_entries); eidx++) {
					const EgaisNsEntry & r_entry = ns_entries[eidx];
					bool   skip = false;
					//
					// Разрешение неоднозначностей в namespace'ах
					//
					switch(r_entry.Nn) {
						case  3: skip = (oneof7(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4,
							PPEDIOP_EGAIS_ACTCHARGEONSHOP, PPEDIOP_EGAIS_ACTCHARGEON_V2, PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "oref" "ClientRef"
						case  4: skip = (oneof11(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_TRANSFERTOSHOP,
							PPEDIOP_EGAIS_ACTCHARGEONSHOP, PPEDIOP_EGAIS_TRANSFERFROMSHOP, PPEDIOP_EGAIS_ACTCHARGEON_V2,
							PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_ACTWRITEOFF_V2, PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "pref" "ProductRef"
						case  5: skip = (oneof4(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "wb"
						case  7: skip = (doc_type != PPEDIOP_EGAIS_REQUESTREPEALWB); break;
						case  6: skip = (oneof5(doc_type, PPEDIOP_EGAIS_REQUESTREPEALWB, 
							PPEDIOP_EGAIS_NOTIFY_WBVER2, PPEDIOP_EGAIS_NOTIFY_WBVER3, PPEDIOP_EGAIS_NOTIFY_WBVER4, PPEDIOP_EGAIS_ACTFIXBARCODE)); break;
						case  8: skip = (oneof4(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "wa"
						case  9: skip = (doc_type == PPEDIOP_EGAIS_ACTFIXBARCODE); break; // "ain"
						case 10: skip = (oneof2(doc_type, PPEDIOP_EGAIS_ACTCHARGEON_V2, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "iab"
						case 11: skip = (oneof2(doc_type, PPEDIOP_EGAIS_CONFIRMREPEALWB, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "wt"
						case 12: skip = (doc_type != PPEDIOP_EGAIS_CONFIRMREPEALWB); break; // "wt"
						case 13: skip = (oneof5(doc_type, PPEDIOP_EGAIS_CONFIRMREPEALWB, PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_ACTWRITEOFF_V2,
							PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "awr"
						case 14: skip = (doc_type == PPEDIOP_EGAIS_ACTFIXBARCODE); break; // "qf"
						case 15: skip = (doc_type == PPEDIOP_EGAIS_ACTFIXBARCODE); break; // "bk"
						case 16: skip = (oneof6(doc_type, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4, 
							PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "ce"
						case 17: skip = (!oneof11(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_TRANSFERTOSHOP,
							PPEDIOP_EGAIS_ACTCHARGEONSHOP, PPEDIOP_EGAIS_TRANSFERFROMSHOP, PPEDIOP_EGAIS_ACTCHARGEON_V2,
							PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_ACTWRITEOFF_V2, PPEDIOP_EGAIS_ACTWRITEOFF_V3, 
							PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "pref" "ProductRef_v2"
						case 19: skip = (doc_type == PPEDIOP_EGAIS_ACTCHARGEON_V2); break; // "ainp"
						case 20: skip = (!oneof6(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_ACTCHARGEONSHOP, 
							PPEDIOP_EGAIS_ACTCHARGEON_V2, PPEDIOP_EGAIS_ACTWRITEOFFSHOP)); break; // "oref" "ClientRef_v2"
						case 22: skip = (doc_type != PPEDIOP_EGAIS_ACTCHARGEON_V2); break; // "ainp"
						case 23: skip = (doc_type != PPEDIOP_EGAIS_ACTCHARGEON_V2); break; // "iab"
						case 24: skip = (doc_type != PPEDIOP_EGAIS_ACTWRITEOFFSHOP); break; // "awr"
						case 25: skip = (doc_type != PPEDIOP_EGAIS_WAYBILLACT_V2); break; // "wa"
						case 26: skip = (doc_type != PPEDIOP_EGAIS_WAYBILL_V2); break; // "wb"
						case 27: skip = (!oneof3(doc_type, PPEDIOP_EGAIS_NOTIFY_WBVER2, PPEDIOP_EGAIS_NOTIFY_WBVER3, PPEDIOP_EGAIS_NOTIFY_WBVER4)); break; // "qp"
						case 28: skip = (doc_type != PPEDIOP_EGAIS_ACTWRITEOFF_V2); break; // "awr"
						case 29: skip = (doc_type != PPEDIOP_EGAIS_WAYBILLACT_V3); break; // "wa"
						case 30: skip = (!oneof6(doc_type, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4, 
							PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_ACTFIXBARCODE)); break; // "ce"
						case 31: skip = (doc_type != PPEDIOP_EGAIS_WAYBILL_V3); break; // "wb"
						case 32: skip = (doc_type != PPEDIOP_EGAIS_ACTWRITEOFF_V3); break; // "awr"
						case 33: skip = (doc_type != PPEDIOP_EGAIS_ACTFIXBARCODE); break; // "awr"
						case 34: skip = (doc_type != PPEDIOP_EGAIS_WAYBILLACT_V4); break; // "wa"
						case 35: skip = (doc_type != PPEDIOP_EGAIS_WAYBILL_V4); break; // "wb"
					}
					if(!skip) {
						bill_text.Z().Cat("xmlns").Colon().Cat(r_entry.P_Ns); // bill_text as temporary buffer
						n_docs.PutAttrib(bill_text, (temp_buf = fsrar_url_prefix).Cat(r_entry.P_Sub));
					}
				}
			}
			{
				SXml::WNode n_owner(_doc, SXml::nst("ns", "Owner"));
				n_owner.PutInner(SXml::nst("ns", "FSRAR_ID"), EncText(fsrar_ident)); // РАР идент отправителя
			}
			{
				SXml::WNode n_d(_doc, SXml::nst("ns", "Document"));
				{
					(temp_buf = "ns").Colon().Cat(doc_type_tag);
					SXml::WNode n_dt(_doc, temp_buf);
					if(oneof4(doc_type, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
						const  PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						int    wb_type = 0;
						PPOprKind op_rec;
						PPOprKind link_op_rec;
						THROW(GetOpData(p_bp->Rec.OpID, &op_rec) > 0);
						PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
						if(op_rec.LinkOpID) {
							THROW(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0);
						}
						n_dt.PutInner(SXml::nst("wb", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
                        	SXml::WNode n_h(_doc, SXml::nst("wb", "Header"));
                        	temp_buf.Z();
                        	PPID   consignee_psn_id = 0;
                        	PPID   consignee_loc_id = 0;
                        	PPID   shipper_psn_id = 0;
                        	PPID   shipper_loc_id = 0;
                        	PPID   suppl_psn_id = 0;
                        	PPID   suppl_loc_id = 0;
                        	int    do_skip = 0;
                        	int    is_intrexpend = 0;
                        	if(rPack.Flags & PPEgaisProcessor::Packet::fReturnBill) {
								wb_type = wbtRetFromMe;
								consignee_psn_id = ObjectToPerson(p_bp->Rec.Object, 0);
								consignee_loc_id = p_bp->GetDlvrAddrID();
								shipper_psn_id = main_org_id;
								shipper_loc_id = p_bp->Rec.LocID;
                        	}
                        	else if(oneof2(p_bp->OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)) {
								wb_type = wbtInvcFromMe;
								PPID   ar2_main_org_id = 0;
								if(p_bp->Rec.Object2 && AcsObj.IsLinkedToMainOrg(op_rec.AccSheet2ID))
									ar2_main_org_id = ObjectToPerson(p_bp->Rec.Object2, 0);
								if(IsIntrExpndOp(p_bp->Rec.OpID)) {
									consignee_psn_id = main_org_id;
									consignee_loc_id = PPObjLocation::ObjToWarehouse_IgnoreRights(p_bp->Rec.Object);
									shipper_psn_id = main_org_id;
									shipper_loc_id = p_bp->Rec.LocID;
									is_intrexpend = 1;
									{
										SString ar_name;
										SString loc_name;
										GetArticleName(p_bp->Rec.Object, ar_name);
										GetLocationName(consignee_loc_id, loc_name);
										(temp_buf = bill_text).Space().Cat("-->").Space().Cat(ar_name).Space().CatChar('[').Cat(loc_name).CatChar(']');
										LogTextWithAddendum(PPTXT_EGAIS_DEBUG_INTREXPND, temp_buf);
									}
								}
								else {
									consignee_psn_id = ObjectToPerson(p_bp->Rec.Object, 0);
									consignee_loc_id = p_bp->GetDlvrAddrID();
									shipper_psn_id = main_org_id;
									shipper_loc_id = p_bp->Rec.LocID;
								}
								if(consignee_psn_id == shipper_psn_id) {
									RegisterTbl::Rec reg_rec;
									SString consignee_kpp;
									SString shipper_kpp;
									if(GetWkrRegister(wkrKPP, shipper_psn_id, shipper_loc_id, p_bp->Rec.Dt, &reg_rec) > 0)
										(shipper_kpp = reg_rec.Num).ToLower();
									if(GetWkrRegister(wkrKPP, consignee_psn_id, consignee_loc_id, p_bp->Rec.Dt, &reg_rec) > 0)
										(consignee_kpp = reg_rec.Num).ToLower();
                                    if(shipper_kpp == consignee_kpp) {
										LogTextWithAddendum(PPTXT_EGAIS_SELFTRANSFBILL, bill_text);
										do_skip = 1;
                                    }
								}
                        	}
                        	else if(oneof2(p_bp->OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
                        		wb_type = wbtInvcToMe;
								consignee_psn_id = main_org_id;
								consignee_loc_id = p_bp->Rec.LocID;
								shipper_psn_id = ObjectToPerson(p_bp->Rec.Object, 0);
								shipper_loc_id = p_bp->GetDlvrAddrID();
                        	}
                        	else if(p_bp->OpTypeID == PPOPT_GOODSRETURN && op_rec.LinkOpID) {
								if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
									wb_type = wbtRetFromMe;
									consignee_psn_id = ObjectToPerson(p_bp->Rec.Object, 0);
									consignee_loc_id = p_bp->GetDlvrAddrID();
									shipper_psn_id = main_org_id;
									shipper_loc_id = p_bp->Rec.LocID;
								}
								else if(link_op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
									wb_type = wbtRetToMe;
									consignee_psn_id = main_org_id;
									consignee_loc_id = p_bp->Rec.LocID;
									shipper_psn_id = ObjectToPerson(p_bp->Rec.Object, 0);
									shipper_loc_id = p_bp->GetDlvrAddrID();
								}
                        	}
                        	if(wb_type && !do_skip) {
								enum {
									gppfAlc      = 0x0001, // В документе есть алкогольные товары
									gppfPacked   = 0x0002, // В документе есть алкогольные упакованные товары
									gppfUnpacked = 0x0004  // В документе есть алкогольные неупакованные товары
								};
								long   gppf = 0;
								PrcssrAlcReport::GoodsItem agi;
								for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
									const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
									if(IsAlcGoods(r_ti.GoodsID)) {
										gppf |= gppfAlc;
										if(PreprocessGoodsItem(r_ti.GoodsID, r_ti.LotID, 0, 0, agi) > 0)
											gppf |= ((agi.UnpackedVolume > 0.0) ? gppfUnpacked : gppfPacked);
									}
								}
								THROW_PP_S(gppf & gppfAlc, PPERR_EGAIS_NOALCINBILL, bill_text);
								THROW_PP_S(consignee_psn_id, PPERR_EDI_UNBLRSLV_BILLOBJ, bill_text);
								// @v10.1.6 (moved down) @01 THROW_PP_S((gppf & (gppfPacked|gppfUnpacked)) != (gppfPacked|gppfUnpacked), PPERR_EGAIS_PKUPKMIXINBILL, bill_text);
								GetWayBillTypeText(wb_type, temp_buf);
								THROW(temp_buf.NotEmpty());
								n_h.PutInner(SXml::nst("wb", "Type"), temp_buf);
								// @v11.0.12 BillCore::GetCode(temp_buf = p_bp->Rec.Code);
								Egais_GetBillCode(*p_bp, temp_buf); // @v11.0.12
								n_h.PutInner(SXml::nst("wb", "NUMBER"), EncText(temp_buf));
								n_h.PutInner(SXml::nst("wb", "Date"), temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT));
								{
									// @v11.1.1 Если дата отгрузки определена и больше даты документа, то она и будет таковой, в противном случае
									// датой отгрузки считаем дату документа.
									LDATE shipping_date = ZERODATE;
									if(p_bp->P_Freight && checkdate(p_bp->P_Freight->IssueDate) && p_bp->P_Freight->IssueDate > p_bp->Rec.Dt)
										shipping_date = p_bp->P_Freight->IssueDate;
									else
										shipping_date = p_bp->Rec.Dt;
									n_h.PutInner(SXml::nst("wb", "ShippingDate"), temp_buf.Z().Cat(shipping_date, DATF_ISO8601CENT));
								}
								{
									long woi_flags = woifStrict|woifDontSendWithoutFSRARID;
									if(oneof3(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4))
										woi_flags |= woifVersion2;
									else {
										THROW_PP_S((gppf & (gppfPacked|gppfUnpacked)) != (gppfPacked|gppfUnpacked), PPERR_EGAIS_PKUPKMIXINBILL, bill_text); // @v10.1.6 (moved from @01)
										n_h.PutInner(SXml::nst("wb", "UnitType"), (gppf & gppfUnpacked) ? "Unpacked" : "Packed");
									}
									THROW(WriteOrgInfo(_doc, SXml::nst("wb", "Shipper"), shipper_psn_id, shipper_loc_id, p_bp->Rec.Dt, woi_flags));
									THROW(WriteOrgInfo(_doc, SXml::nst("wb", "Consignee"), consignee_psn_id, consignee_loc_id, p_bp->Rec.Dt, woi_flags));
									if(suppl_psn_id)
										THROW(WriteOrgInfo(_doc, SXml::nst("wb", "Supplier"), suppl_psn_id, suppl_loc_id, p_bp->Rec.Dt, woi_flags));
								}
								{
									SXml::WNode n_tr(_doc, SXml::nst("wb", "Transport"));
									n_tr.PutInner(SXml::nst("wb", "TRAN_TYPE"), "413"); // Пока не понятно. Во всех примерах только это значение.
									// @v11.0.12 {
									if(doc_type == PPEDIOP_EGAIS_WAYBILL_V4) {
										//<wb:ChangeOwnership>IsChange</wb:ChangeOwnership>
										n_tr.PutInner(SXml::nst("wb", "ChangeOwnership"), "IsChange"); // ???
										//<wb:TRANSPORT_TYPE>car</wb:TRANSPORT_TYPE>
										n_tr.PutInner(SXml::nst("wb", "TRANSPORT_TYPE"), "car");
									}
									// } @v11.0.12 
									temp_buf.Z();
									if(p_bp->P_Freight)
										GetPersonName(p_bp->P_Freight->AgentID, temp_buf);
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_COMPANY"), EncText(temp_buf));
									{
										temp_buf.Z();
										if(p_bp->P_Freight)
											GetObjectName(PPOBJ_TRANSPORT, p_bp->P_Freight->ShipID, temp_buf);
										n_tr.PutInnerSkipEmpty(SXml::nst("wb", (doc_type == PPEDIOP_EGAIS_WAYBILL_V4) ? "TRANSPORT_REGNUMBER" : "TRAN_CAR"), EncText(temp_buf));
									}
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_TRAILER"), "");
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_CUSTOMER"), "");
									temp_buf.Z();
									if(p_bp->P_Freight)
										GetPersonName(p_bp->P_Freight->CaptainID, temp_buf);
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_DRIVER"), EncText(temp_buf));
									temp_buf.Z();
									if(p_bp->P_Freight)
                                        GetObjectName(PPOBJ_WORLD, p_bp->P_Freight->PortOfLoading, temp_buf);
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_LOADPOINT"), EncText(temp_buf));
									temp_buf.Z();
									if(p_bp->P_Freight)
                                        GetObjectName(PPOBJ_WORLD, p_bp->P_Freight->PortOfDischarge, temp_buf);
									n_tr.PutInnerSkipEmpty(SXml::nst("wb", "TRAN_UNLOADPOINT"), EncText(temp_buf));
									n_tr.PutInner(SXml::nst("wb", "TRAN_REDIRECT"), ""); // @v11.7.10 PutInnerSkipEmpty-->PutInner
									n_tr.PutInner(SXml::nst("wb", "TRAN_FORWARDER"), ""); // @v11.7.10 PutInnerSkipEmpty-->PutInner
								}
								n_h.PutInnerSkipEmpty(SXml::nst("wb", "Base"), ""); // Основание
								{
									temp_buf.Z();
									if(is_intrexpend)
										temp_buf.CatEq(PPConst::P_BillNotePrefix_IntrExpnd, p_bp->Rec.ID);
									// @v8.9.10 (Поставщики не хотят передавать свои примечания в ЕГАИС) if(p_bp->Rec.Memo[0]) temp_buf.CatDiv('-', 1, 1).Cat(p_bp->Rec.Memo);
									n_h.PutInnerSkipEmpty(SXml::nst("wb", "Note"), EncText(temp_buf)); // Примечание
								}
                        	}
                        }
						{
                        	SXml::WNode w_c(_doc, SXml::nst("wb", "Content"));
                        	LongArray seen_pos_list;
                        	SString ref_a;
                        	SString ref_b;
                        	SString ref_b_fw;
                        	for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(!seen_pos_list.lsearch(tidx) && IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, r_ti.LotID, 0, 0, agi) > 0) {
									LongArray local_pos_list;
									double qtty = fabs(r_ti.Qtty());
									double price = 0.0;
									long   qtty_fmt = MKSFMTD(0, 0, NMBF_NOTRAILZ);
									ReceiptTbl::Rec lot_rec;
									if(wb_type == wbtRetFromMe)
										price = (op_rec.Flags & OPKF_SELLING) ? fabs(r_ti.NetPrice()) : r_ti.Cost;
									else
										price = fabs(r_ti.NetPrice());
									if(agi.UnpackedVolume > 0.0) {
										const double mult = agi.UnpackedVolume / 10.0;
										qtty = (qtty * mult); // Неупакованная продукция передается в декалитрах
										price = (price / mult);
										qtty_fmt = MKSFMTD(0, 6, 0); // @v11.2.10 MKSFMTD_030-->MKSFMTD(0, 6, 0)
									}
									P_BObj->trfr->Rcpt.Search(r_ti.LotID, &lot_rec);
									P_BObj->MakeLotText(&lot_rec, PPObjBill::ltfGoodsName, temp_buf);
									lot_text.Z().CatChar('[').Cat(r_ti.RByBill).CatChar(']').Space().Cat(temp_buf);
									THROW_PP_S(p_bp->LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARINFA, ref_a) > 0, PPERR_EGAIS_NOINFAIDINLOT, lot_text);
									THROW_PP_S(p_bp->LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARINFB, ref_b) > 0, PPERR_EGAIS_NOINFBIDINLOT, lot_text);
									//
									// Если товар и лот в двух строках совпадают, то придется сливать такие строки.
									// В равной степени это касается эквивалентности справок Б (ref_b).
									//
									for(uint fw_tidx = tidx+1; fw_tidx < p_bp->GetTCount(); fw_tidx++) {
										const PPTransferItem & r_fw_ti = p_bp->ConstTI(fw_tidx);
										if(r_fw_ti.GoodsID == r_ti.GoodsID && !seen_pos_list.lsearch(fw_tidx)) {
											int   do_merge = 0;
											if(r_fw_ti.LotID == r_ti.LocID)
												do_merge = 1;
											else if(p_bp->LTagL.GetTagStr(fw_tidx, PPTAG_LOT_FSRARINFB, ref_b_fw) > 0 && ref_b_fw.CmpNC(ref_b) == 0)
												do_merge = 1;
											if(do_merge) {
												double fw_qtty = fabs(r_fw_ti.Qtty());
												double fw_price = 0.0;
												if(wb_type == wbtRetFromMe)
													fw_price = (op_rec.Flags & OPKF_SELLING) ? fabs(r_fw_ti.NetPrice()) : r_fw_ti.Cost;
												else
													fw_price = fabs(r_fw_ti.NetPrice());
												if(agi.UnpackedVolume > 0.0) {
													const double mult = agi.UnpackedVolume / 10.0;
													fw_qtty = (fw_qtty * mult); // Неупакованная продукция передается в декалитрах
													fw_price = (fw_price / mult);
												}
												double total_qtty = qtty + fw_qtty;
												double total_price = ((qtty * price) + (fw_qtty * fw_price)) / total_qtty;
												qtty = total_qtty;
												price = total_price;
												seen_pos_list.add(fw_tidx);
												local_pos_list.add(fw_tidx);
											}
										}
									}
									seen_pos_list.add(tidx);
									local_pos_list.add(tidx);
									//
									SXml::WNode w_p(_doc, SXml::nst("wb", "Position"));
									w_p.PutInner(SXml::nst("wb", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
									long wpi_flags = (oneof3(doc_type, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) ?
										(wpifPutManufInfo|wpifVersion2) : wpifPutManufInfo;
									THROW(WriteProductInfo(_doc, SXml::nst("wb", "Product"), r_ti.GoodsID, r_ti.LotID, wpi_flags, 0))
									w_p.PutInnerSkipEmpty(SXml::nst("wb", "Pack_ID"), "");
									{
										w_p.PutInner(SXml::nst("wb", "Quantity"), EncText(temp_buf.Z().Cat(qtty, qtty_fmt)));
										w_p.PutInner(SXml::nst("wb", "Price"), EncText(temp_buf.Z().Cat(price, MKSFMTD_020)));
									}
									p_bp->LTagL.GetString(PPTAG_LOT_SN, tidx, temp_buf);
									w_p.PutInnerSkipEmpty(SXml::nst("wb", "Party"), EncText(temp_buf));
									{
										WriteInformCode(_doc, "wb", 'A', ref_a, doc_type);
										if(oneof2(doc_type, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
											SXml::WNode w_s(_doc, SXml::nst("wb", "InformF2"));
											w_s.PutInner(SXml::nst("ce", "F2RegId"), EncText(ref_b));
											//
											// Уточнение выгрузки марок для случая, если несколько строк документа объединяются в одну
											//
											int    is_there_ext_codes = 0;
											{
												for(uint lpidx = 0; !is_there_ext_codes && lpidx < local_pos_list.getCount(); lpidx++) {
													const int row_idx = local_pos_list.get(lpidx);
													if(p_bp->XcL.Get(row_idx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount())
														is_there_ext_codes = 1;
												}
											}
											if(is_there_ext_codes) {
												SXml::WNode w_m(_doc, SXml::nst("ce", "MarkInfo"));
												for(uint lpidx = 0; lpidx < local_pos_list.getCount(); lpidx++) {
													const int row_idx = local_pos_list.get(lpidx);
													if(p_bp->XcL.Get(row_idx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
														// @v10.7.12 (moved up to #1) SXml::WNode w_m(_doc, SXml::nst("ce", "MarkInfo"));
														for(uint boxidx = 0; boxidx < ext_codes_set.GetCount(); boxidx++) {
															if(ext_codes_set.GetByIdx(boxidx, msentry) && msentry.Flags & PPLotExtCodeContainer::fBox) {
																SXml::WNode w_box(_doc, SXml::nst("ce", "boxpos"));
																w_box.PutInner(SXml::nst("ce", "boxnumber"), EncText(msentry.Num));
																{
																	SXml::WNode w_amclist(_doc, SXml::nst("ce", "amclist"));
																	ext_codes_set.GetByBoxID(msentry.BoxID, ss);
																	for(uint ssp = 0; ss.get(&ssp, temp_buf);)
																		w_amclist.PutInner(SXml::nst("ce", "amc"), EncText(temp_buf));
																}
															}
														}
														{
															//
															// В конце вставляем марки, не привязанные к боксам
															//
															ext_codes_set.GetByBoxID(0, ss);
															if(ss.getCount()) {
																SXml::WNode w_box(_doc, SXml::nst("ce", "boxpos"));
																{
																	SXml::WNode w_amclist(_doc, SXml::nst("ce", "amclist"));
																	for(uint ssp = 0; ss.get(&ssp, temp_buf);)
																		w_amclist.PutInner(SXml::nst("ce", "amc"), EncText(temp_buf));
																}
															}
														}
													}
												}
											}
										}
										else
											WriteInformCode(_doc, "wb", 'B', ref_b, doc_type);
									}
								}
                        	}
                        }
					}
					else if(oneof7(doc_type, PPEDIOP_EGAIS_QUERYAP, PPEDIOP_EGAIS_QUERYCLIENTS, PPEDIOP_EGAIS_QUERYRESTS,
						PPEDIOP_EGAIS_QUERYRESTS_V2, PPEDIOP_EGAIS_QUERYSP, PPEDIOP_EGAIS_QUERYSSP, PPEDIOP_EGAIS_QUERYRESTSSHOP)) {
						const StrStrAssocArray * p_param_list = static_cast<const StrStrAssocArray *>(rPack.P_Data);
						if(p_param_list && p_param_list->getCount()) {
							SXml::WNode n_arglist(_doc, SXml::nst("qp", "Parameters"));
							for(uint i = 0; i < p_param_list->getCount(); i++) {
								const SStrToStrAssoc par = p_param_list->at(i);
								SXml::WNode n_p(_doc, SXml::nst("qp", "Parameter"));
								n_p.PutInner(SXml::nst("qp", "Name"), EncText(temp_buf = par.Key));
								n_p.PutInner(SXml::nst("qp", "Value"), EncText(temp_buf = par.Val));
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_QUERYRESENDDOC) {
						const SString * p_doc_id = static_cast<const SString *>(rPack.P_Data);
						if(p_doc_id && p_doc_id->NotEmpty()) {
							SXml::WNode n_arglist(_doc, SXml::nst("qp", "Parameters"));
							{
								SXml::WNode n_p(_doc, SXml::nst("qp", "Parameter"));
								n_p.PutInner(SXml::nst("qp", "Name"), EncText(temp_buf = "WBREGID"));
								n_p.PutInner(SXml::nst("qp", "Value"), EncText(temp_buf = *p_doc_id));
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_QUERYRESTBCODE) {
						const SString * p_param = static_cast<const SString *>(rPack.P_Data);
						if(p_param && p_param->NotEmpty()) {
							bool   is_horeca = false;
							SString ref_b;
							SString addendum;
							int   divider = 0;
							if(p_param->HasChr(' '))
								divider = ' ';
							else if(p_param->HasChr(';'))
								divider = ';';
							else if(p_param->HasChr(','))
								divider = ',';
							if(divider && p_param->Divide(divider, temp_buf, addendum) > 0) {
								temp_buf.Strip();
								addendum.Strip();
								if(temp_buf.IsEqiAscii("horeca") || temp_buf.IsEqiAscii("cafe")) {
									is_horeca = true;
									ref_b = addendum;
								}
								else if(addendum.IsEqiAscii("horeca") || addendum.IsEqiAscii("cafe")) {
									is_horeca = true;
									ref_b = temp_buf;
								}
								else
									ref_b = *p_param;
							}
							else
								ref_b = *p_param;
							SXml::WNode n_arglist(_doc, SXml::nst("qp", "Parameters"));
							//PPHSC_RU_EGAIS_FORM2_CAPS       "ФОРМА2"  // Строка используемая в качестве параметра запроса к егаис
							//PPHSC_RU_EGAIS_CAFE_CAPS       "ОБЩЕПИТ" // Строка используемая в качестве параметра запроса к егаис
							{
								SXml::WNode n_p(_doc, SXml::nst("qp", "Parameter"));
								PPLoadStringUtf8(PPSTR_HASHTOKEN_C, PPHSC_RU_EGAIS_FORM2_CAPS, temp_buf);
								n_p.PutInner(SXml::nst("qp", "Name"), temp_buf/*EncText((temp_buf = "ФОРМА2").Transf(CTRANSF_UTF8_TO_INNER))*/); // current srcfile is in UTF-8
								n_p.PutInner(SXml::nst("qp", "Value"), EncText(temp_buf = ref_b));
							}
							if(is_horeca) {
								SXml::WNode n_p(_doc, SXml::nst("qp", "Parameter"));
								PPLoadStringUtf8(PPSTR_HASHTOKEN_C, PPHSC_RU_EGAIS_CAFE_CAPS, temp_buf);
								n_p.PutInner(SXml::nst("qp", "Name"), temp_buf/*EncText((temp_buf = "ОБЩЕПИТ").Transf(CTRANSF_UTF8_TO_INNER))*/); // current srcfile is in UTF-8
								n_p.PutInner(SXml::nst("qp", "Value"), "true");									
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_NOTIFY_WBVER2) {
						n_dt.PutInner(SXml::nst("qp", "ClientId"), EncText(fsrar_ident));
						n_dt.PutInner(SXml::nst("qp", "WBTypeUsed"), EncText(temp_buf = "WayBill_v2"));
					}
					else if(doc_type == PPEDIOP_EGAIS_NOTIFY_WBVER3) {
						n_dt.PutInner(SXml::nst("qp", "ClientId"), EncText(fsrar_ident));
						n_dt.PutInner(SXml::nst("qp", "WBTypeUsed"), EncText(temp_buf = "WayBill_v3"));
					}
					else if(doc_type == PPEDIOP_EGAIS_NOTIFY_WBVER4) { // @v11.0.12
						n_dt.PutInner(SXml::nst("qp", "ClientId"), EncText(fsrar_ident));
						n_dt.PutInner(SXml::nst("qp", "WBTypeUsed"), EncText(temp_buf = "WayBill_v4"));
					}
					else if(oneof4(doc_type, PPEDIOP_EGAIS_QUERYFORMA, PPEDIOP_EGAIS_QUERYFORMB, PPEDIOP_EGAIS_QUERYFORMF1, PPEDIOP_EGAIS_QUERYFORMF2)) { 
						// @v11.2.12 PPEDIOP_EGAIS_QUERYFORMF1, PPEDIOP_EGAIS_QUERYFORMF2 // @v11.3.4 @fix PPEDIOP_EGAIS_REPLYFORMB-->PPEDIOP_EGAIS_QUERYFORMB
						const SString * p_formab_regid = static_cast<const SString *>(rPack.P_Data);
                        if(p_formab_regid->NotEmpty()) {
							n_dt.PutInner(SXml::nst("qf", "FormRegId"), EncText(temp_buf = *p_formab_regid));
                        }
                        else {
							PPSetError(PPERR_EGAIS_QUERYFORMREGIDEMPTY);
							LogLastError();
                        }
					}
					else if(doc_type == PPEDIOP_EGAIS_QUERYBARCODE) {
                        const TSCollection <QueryBarcode> * p_qbl = static_cast<const TSCollection <QueryBarcode> *>(rPack.P_Data);
                        if(p_qbl->getCount()) {
							const LDATETIME now_dtm = getcurdatetime_();
							n_dt.PutInner(SXml::nst("bk", "QueryNumber"), temp_buf.Z().Cat((now_dtm.d.v % 1000) * 1000 + (now_dtm.t.v % 1000)));
							n_dt.PutInner(SXml::nst("bk", "Date"), temp_buf.Z().Cat(now_dtm, DATF_ISO8601CENT, 0));
							{
								long   global_row_id = 0;
								LongArray row_id_list;
								SXml::WNode n_items(_doc, SXml::nst("bk", "Marks"));
								for(uint qbidx = 0; qbidx < p_qbl->getCount(); qbidx++) {
									const QueryBarcode * p_qb = p_qbl->at(qbidx);
									if(p_qb && p_qb->CodeType && p_qb->Rank.NotEmpty() && p_qb->Number.NotEmpty()) {
										SXml::WNode n_mark(_doc, SXml::nst("bk", "Mark"));
										{
											long  row_id = p_qb->RowId;
											if(!row_id)
												row_id = ++global_row_id;
											else
												global_row_id = row_id;
											while(row_id_list.lsearch(row_id)) {
												row_id = ++global_row_id;
											}
											n_mark.PutInner(SXml::nst("bk", "Identity"), temp_buf.Z().Cat(row_id));
											n_mark.PutInner(SXml::nst("bk", "Type"),   temp_buf.Z().CatLongZ(p_qb->CodeType, 3));
											n_mark.PutInner(SXml::nst("bk", "Rank"),   temp_buf.Z().Cat(p_qb->Rank));
											n_mark.PutInner(SXml::nst("bk", "Number"), temp_buf.Z().Cat(p_qb->Number));
											row_id_list.add(row_id);
										}
									}
								}
							}
                        }
					}
					else if(doc_type == PPEDIOP_EGAIS_CONFIRMTICKET) {
                        ConfirmTicket * p_ticket = static_cast<ConfirmTicket *>(rPack.P_Data);
						SXml::WNode n_h(_doc, SXml::nst("wt", "Header"));
						n_h.PutInner(SXml::nst("wt", "IsConfirm"), p_ticket->Conclusion ? "Accepted" : "Rejected");
						n_h.PutInner(SXml::nst("wt", "TicketNumber"), EncText(temp_buf = p_ticket->Code));
						n_h.PutInner(SXml::nst("wt", "TicketDate"), temp_buf.Z().Cat(p_ticket->Date, DATF_ISO8601CENT));
						n_h.PutInner(SXml::nst("wt", "WBRegId"), EncText(temp_buf = p_ticket->RegIdent));
						n_h.PutInnerSkipEmpty(SXml::nst("wt", "Note"), EncText(temp_buf = p_ticket->Comment));
					}
					else if(oneof4(doc_type, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4)) {
						PPBillPacket * p_bp = static_cast<PPBillPacket *>(rPack.P_Data);
						PPBillPacket * p_link_bp = 0;
						PPBillPacket _link_bp;
						int    is_status_suited = 0;
						SString edi_ident;
						SString bill_code;
						// @v11.0.12 BillCore::GetCode(bill_code = p_bp->Rec.Code);
						Egais_GetBillCode(*p_bp, bill_code); // @v11.0.12
						bill_code.Strip();
						PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
						p_bp->BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, edi_ident);
						THROW_PP_S(edi_ident.NotEmptyS(), PPERR_EGAIS_BILLHASNEDIIDENTTAG, bill_text);
						if(P_BObj->CheckStatusFlag(p_bp->Rec.StatusID, BILSTF_READYFOREDIACK))
							is_status_suited = 1;
						if(oneof4(p_bp->Rec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
							int    cmp_result = 0; // 0 - accepted, -1 - rejected,
								// & 0x01 - есть отличия в меньшую сторону по количеству
								// & 0x02 - есть отличия в большую сторону по количеству
							int    is_uncond_accept = 0;
							PPIDArray wroff_bill_list;
							BillTbl::Rec wroff_bill_rec;
							for(DateIter diter; P_BObj->P_Tbl->EnumLinks(p_bp->Rec.ID, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;)
								wroff_bill_list.add(wroff_bill_rec.ID);
							if(wroff_bill_list.getCount() > 1)
								LogTextWithAddendum(PPTXT_EGAIS_BILLWROFFCONFL, bill_text);
							else {
								if(wroff_bill_list.getCount() == 1) {
									THROW(P_BObj->ExtractPacket(wroff_bill_list.get(0), &_link_bp) > 0);
									p_link_bp = &_link_bp;
									if(P_BObj->CheckStatusFlag(_link_bp.Rec.StatusID, BILSTF_READYFOREDIACK)) {
										is_status_suited = 1;
                                        if(ACfg.Hdr.Flags & PPAlbatrosCfgHdr::fUncondAcceptEdiIntrMov && IsIntrOp(_link_bp.Rec.OpID))
											is_uncond_accept = 1;
									}
								}
								else if(p_bp->Rec.Flags2 & BILLF2_DECLINED)
									is_status_suited = 1;
								if(is_status_suited) {
									int    recadv_status = PPEDI_RECADV_STATUS_UNDEF;
									TSCollection <PPBillPacket::TiDifferenceItem> diff_list;
									if(p_link_bp) {
										if(ACfg.Hdr.Flags & PPAlbatrosCfgHdr::fRecadvEvalByCorrBill)
											p_link_bp->CompareTIByCorrection(PPBillPacket::tidfRByBillPrec|PPBillPacket::tidfQtty, 0, diff_list);
										else
											p_bp->CompareTI(*p_link_bp, PPBillPacket::tidfRByBillPrec|PPBillPacket::tidfQtty|
												PPBillPacket::tidfIgnoreGoods|PPBillPacket::tidfIgnoreSign, 0, diff_list);
									}
									int    all_absent = 0;
									if(diff_list.getCount()) {
										LongArray absent_pos_list;
										for(uint di = 0; di < diff_list.getCount(); di++) {
											const PPBillPacket::TiDifferenceItem * p_diff_item = diff_list.at(di);
											if(p_diff_item->Flags & PPBillPacket::tidfOtherAbsent) {
												cmp_result |= 0x01;
												absent_pos_list.add(&p_diff_item->ThisPList);
											}
											if(p_diff_item->Flags & PPBillPacket::tidfThisAbsent) {
												//
												// Если в нашем документе отсутствует позиция документа списания,
												// не являющаяся алкогольной, то игнорируем это рассогласование
												//
												int    is_other_alc = 0;
												for(uint opi = 0; !is_other_alc && opi < p_diff_item->OtherPList.getCount(); opi++) {
													const  PPID op_goods_id = labs(p_link_bp->ConstTI(p_diff_item->OtherPList.get(opi)).GoodsID);
													if(IsAlcGoods(op_goods_id))
														is_other_alc = 1;
												}
												if(is_other_alc)
													cmp_result |= 0x02;
											}
											if(p_diff_item->Flags & PPBillPacket::tidfQtty) {
												if(p_diff_item->OtherQtty < p_diff_item->ThisQtty)
													cmp_result |= 0x01;
												else if(p_diff_item->OtherQtty > p_diff_item->ThisQtty)
													cmp_result |= 0x02;
											}
										}
										absent_pos_list.sortAndUndup();
										if(absent_pos_list.getCount() >= p_bp->GetTCount())
											all_absent = 1;
									}
									if(p_bp->Rec.Flags2 & BILLF2_DECLINED) {
										LogTextWithAddendum(PPTXT_EGAIS_BILLDECLINED, bill_text);
										cmp_result = -1;
										recadv_status = PPEDI_RECADV_STATUS_REJECT;
									}
									else if(is_uncond_accept) {
										LogTextWithAddendum(PPTXT_EGAIS_BILLUNCNDACCEPTED, bill_text);
										cmp_result = 0;
										recadv_status = PPEDI_RECADV_STATUS_ACCEPT;
									}
									else if(all_absent) {
										LogTextWithAddendum(PPTXT_EGAIS_BILLFULLYREJECTED, bill_text);
										cmp_result = -1;
										recadv_status = PPEDI_RECADV_STATUS_REJECT;
									}
									else if(cmp_result & 0x01)
										recadv_status = PPEDI_RECADV_STATUS_PARTACCEPT;
									else if(cmp_result & 0x02)
										recadv_status = PPEDI_RECADV_STATUS_ACCEPT;
									else if(cmp_result == 0) {
										LogTextWithAddendum(PPTXT_EGAIS_BILLFULLYACCEPTED, bill_text);
										recadv_status = PPEDI_RECADV_STATUS_ACCEPT;
									}
									{
										SXml::WNode n_h(_doc, SXml::nst("wa", "Header"));
										if(oneof2(cmp_result, -1, 0)) {
											if(cmp_result == -1)
												temp_buf = "Rejected";
											else if(oneof3(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4))
												temp_buf = (cmp_result & 0x01) ? "Differences" : "Accepted";
											else
												temp_buf = "Accepted";
											n_h.PutInner(SXml::nst("wa", "IsAccept"), EncText(temp_buf));
										}
										else if(cmp_result & 0x01) {
											if(oneof3(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4))
												n_h.PutInner(SXml::nst("wa", "IsAccept"), EncText(temp_buf = "Differences"));
										}
										else {
											if(oneof3(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4))
												n_h.PutInner(SXml::nst("wa", "IsAccept"), EncText(temp_buf = "Accepted"));
										}
										(temp_buf = bill_code).CatChar('-').Cat("ACT");
										n_h.PutInner(SXml::nst("wa", "ACTNUMBER"), EncText(temp_buf));
										n_h.PutInner(SXml::nst("wa", "ActDate"), temp_buf.Z().Cat(getcurdate_(), DATF_ISO8601CENT));
										// @v11.0.12 (useless) BillCore::GetCode(temp_buf = p_bp->Rec.Code);
										n_h.PutInner(SXml::nst("wa", "WBRegId"), EncText(temp_buf = edi_ident));
										n_h.PutInner(SXml::nst("wa", "Note"), EncText(/*p_bp->Rec.Memo*/"")); // Не хотят передавать свои примечания в ЕГАИС
									}
									{
										SXml::WNode n_c(_doc, SXml::nst("wa", "Content"));
										if(cmp_result == -1) {
										}
										else {
											if(cmp_result & 0x01) {
												LogTextWithAddendum(PPTXT_EGAIS_BILLPARTACCEPTEDM, bill_text);
												for(uint bi = 0; bi < p_bp->GetTCount(); bi++) { // Итерация по строкам драфт-документа
													const PPTransferItem & r_ti = p_bp->ConstTI(bi);
													const PPTransferItem * p_lti = 0; // Указатель на строку в документе списания p_link_bp, соответствующую r_ti
													uint  lti_pos = 0; // Позиция строки документа списания p_link_bp, соответствующая bi
													double real_qtty = 0.0;
													double declared_qtty = fabs(r_ti.Quantity_);
													for(uint lbi = 0; !p_lti && lbi < p_link_bp->GetTCount(); lbi++) {
														const PPTransferItem & r_lti = p_link_bp->ConstTI(lbi);
														if(r_lti.RByBill == r_ti.RByBill) {
															p_lti = &p_link_bp->ConstTI(lbi);
															lti_pos = lbi;
														}
													}
													if(ACfg.Hdr.Flags & PPAlbatrosCfgHdr::fRecadvEvalByCorrBill) {
														double other_qtty = r_ti.Quantity_;
														if(p_lti) {
															for(uint di = 0; di < diff_list.getCount(); di++) {
																const PPBillPacket::TiDifferenceItem * p_diff_item = diff_list.at(di);
																if(p_diff_item && p_diff_item->ThisPList.lsearch(lti_pos)) {
																	other_qtty = p_diff_item->OtherQtty;
																	break;
																}
															}
														}
														real_qtty = MIN(fabs(r_ti.Quantity_), fabs(other_qtty));
													}
													else
														real_qtty = p_lti ? MIN(fabs(r_ti.Quantity_), fabs(p_lti->Quantity_)) : 0.0;
													{
														GoodsItem _agi;
														PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, _agi);
														if(_agi.UnpackedVolume > 0.0) {
															const double mult = _agi.UnpackedVolume / 10.0;
															real_qtty = (real_qtty * mult); // Неупакованная продукция передается в декалитрах
														}
													}
													{
														SXml::WNode n_pos(_doc, SXml::nst("wa", "Position"));
														if(p_bp->LTagL.GetTagStr(bi, PPTAG_LOT_ORGLINEIDENT, temp_buf) <= 0) {
															temp_buf.Z().Cat(r_ti.RByBill);
														}
														n_pos.PutInner(SXml::nst("wa", "Identity"), EncText(temp_buf));
														//
														p_bp->LTagL.GetTagStr(bi, PPTAG_LOT_FSRARINFB, temp_buf);
														if(oneof3(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4))
															n_pos.PutInner(SXml::nst("wa", "InformF2RegId"), EncText(temp_buf));
														else
															n_pos.PutInner(SXml::nst("wa", "InformBRegId"), EncText(temp_buf));
														//
														n_pos.PutInner(SXml::nst("wa", "RealQuantity"), temp_buf.Z().Cat(real_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
														if(oneof2(doc_type, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4) && declared_qtty > real_qtty) {
															const int do_send_with_waybillact_accepted_marks = 0;
															if(do_send_with_waybillact_accepted_marks) {
																uint mark_count = 0;
																if(p_lti && p_link_bp->XcL.Get(lti_pos+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
																	SXml::WNode w_m(_doc, SXml::nst("wa", "MarkInfo"));
																	for(uint boxidx = 0; boxidx < ext_codes_set.GetCount(); boxidx++) {
																		if(ext_codes_set.GetByIdx(boxidx, msentry) && !(msentry.Flags & PPLotExtCodeContainer::fBox)) {
																			w_m.PutInner(SXml::nst("ce", "amc"), EncText(msentry.Num));
																			mark_count++;
																		}
																	}
																	if(mark_count != static_cast<uint>(real_qtty)) {
																		//PPTXT_EGAIS_NEQMARKINACTROW         "Количество марок в строке документа списания не соответствует принятому количеству товара: %s"
																		(temp_buf = bill_text).CatDiv('-', 1).Cat(p_lti->RByBill).Space().
																			Cat(mark_count).Slash().Cat(fabs(p_lti->Quantity_), MKSFMTD(0, 3, NMBF_NOTRAILZ));
																		LogTextWithAddendum(PPTXT_EGAIS_NEQMARKINACTROW, temp_buf);
																	}
																}
															}
															else if(p_lti) {
																if(p_bp->XcL.Get(bi+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
																	SXml::WNode w_m(_doc, SXml::nst("wa", "MarkInfo"));
																	for(uint boxidx = 0; boxidx < ext_codes_set.GetCount(); boxidx++) {
																		if(ext_codes_set.GetByIdx(boxidx, msentry) && !(msentry.Flags & PPLotExtCodeContainer::fBox)) {
																			if(!p_link_bp->_VXcL.Search(msentry.Num, 0, 0)) {
																				w_m.PutInner(SXml::nst("ce", "amc"), EncText(msentry.Num));
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
											if(cmp_result & 0x02)
												LogTextWithAddendum(PPTXT_EGAIS_BILLPARTACCEPTEDP, bill_text);
										}
									}
									BillCore::SetRecadvStatus(recadv_status, p_bp->Rec);
								}
								else
									LogTextWithAddendum(PPTXT_EGAIS_BILLSTATUSNREADY, bill_text);
							}
						}
						else if(p_bp->Rec.Flags2 & BILLF2_DECLINED && is_status_suited) { // нет признака EdiOp - документ отправлялся нами и мы хотим отправить отмену проводки
							{
								SXml::WNode n_h(_doc, SXml::nst("wa", "Header"));
								n_h.PutInner(SXml::nst("wa", "IsAccept"), EncText(temp_buf = "Rejected"));
								(temp_buf = bill_code).CatChar('-').Cat("REJECT");
								n_h.PutInner(SXml::nst("wa", "ACTNUMBER"), EncText(temp_buf));
								n_h.PutInner(SXml::nst("wa", "ActDate"), temp_buf.Z().Cat(getcurdate_(), DATF_ISO8601CENT));
								n_h.PutInner(SXml::nst("wa", "WBRegId"), EncText(temp_buf = edi_ident));
								// @v8.9.10 (Не хотят передавать свои примечания в ЕГАИС) n_h.PutInnerSkipEmpty("wa:Note", EncText(p_bp->Rec.Memo));
							}
							{
								SXml::WNode n_c(_doc, SXml::nst("wa", "Content"));
							}
							LogTextWithAddendum(PPTXT_EGAIS_BILLSELFREJECTED, bill_text);
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("ainp", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("ainp", "Header"));
							n_h.PutInner(SXml::nst("ainp", "Number"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("ainp", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							temp_buf.Z();
							if(p_bp->BTagL.GetItemStr(PPTAG_BILL_FORMALREASON, temp_buf) <= 0) {
								PPLoadText(PPTXT_EGAIS_PRODRCVDBEFORE2016, temp_buf);
							}
							n_h.PutInner(SXml::nst("ainp", "TypeChargeOn"), EncText(temp_buf));
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("ainp", "Content"));
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
									double _qtty = fabs(r_ti.Quantity_);
									long   _fmt = 0;
									if(agi.UnpackedVolume > 0.0) {
										const double mult = agi.UnpackedVolume / 10.0;
										_qtty = R4(_qtty * mult);
										_fmt = MKSFMTD(0, 4, 0);
									}
									else {
										_qtty = R0(_qtty);
									}
									SXml::WNode w_p(_doc, SXml::nst("ainp", "Position"));
									w_p.PutInner(SXml::nst("ainp", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
									const ObjTagList * p_ti_tag_list = p_bp->LTagL.Get(tidx);
									THROW(WriteProductInfo(_doc, SXml::nst("ainp", "Product"), r_ti.GoodsID, 0, wpifPutManufInfo|wpifVersion2, p_ti_tag_list))
									w_p.PutInner(SXml::nst("ainp", "Quantity"), EncText(temp_buf.Z().Cat(_qtty, _fmt)));
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTWRITEOFFSHOP) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("awr", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("awr", "Header"));
							n_h.PutInner(SXml::nst("awr", "ActNumber"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("awr", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							if(p_bp->BTagL.GetItemStr(PPTAG_BILL_FORMALREASON, temp_buf) <= 0) {
								PPLoadText(/*PPTXT_EGAIS_LACK*/PPTXT_EGAIS_SALE, temp_buf); // @v11.3.5 PPTXT_EGAIS_LACK-->PPTXT_EGAIS_SALE
							}
							n_h.PutInner(SXml::nst("awr", "TypeWriteOff"), EncText(temp_buf));
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("awr", "Content"));
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
									double qtty = fabs(r_ti.Quantity_);
									long   qtty_fmt = MKSFMTD(0, 0, NMBF_NOTRAILZ);
									if(agi.UnpackedVolume > 0.0) {
										const double mult = agi.UnpackedVolume / 10.0;
										qtty = (qtty * mult); // Неупакованная продукция передается в декалитрах
										qtty_fmt = MKSFMTD_030;
									}
									SXml::WNode w_p(_doc, SXml::nst("awr", "Position"));
									w_p.PutInner(SXml::nst("awr", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
									const ObjTagList * p_ti_tag_list = p_bp->LTagL.Get(tidx);
									THROW(WriteProductInfo(_doc, SXml::nst("awr", "Product"), r_ti.GoodsID, 0, wpifPutManufInfo|wpifVersion2, p_ti_tag_list))
									w_p.PutInner(SXml::nst("awr", "Quantity"), EncText(temp_buf.Z().Cat(qtty, qtty_fmt)));
									if(p_bp->XcL.Get(tidx+1, 0, ext_codes_set) > 0) {
										SXml::WNode w_mc(_doc, SXml::nst("awr", "MarkCodeInfo"));
										//for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);) {
										//PPLotExtCodeContainer::MarkSet::Entry msentry;
										for(uint msidx = 0; msidx < ext_codes_set.GetCount(); msidx++) {
											if(ext_codes_set.GetByIdx(msidx, msentry) && !(msentry.Flags & PPLotExtCodeContainer::fBox)) {
												w_mc.PutInner("MarkCode", msentry.Num);
											}
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTCHARGEON_V2) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						{
							SXml::WNode n_h(_doc, SXml::nst("ainp", "Header"));
							n_h.PutInner(SXml::nst("ainp", "Number"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("ainp", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							temp_buf.Z();
							if(p_bp->BTagL.GetItemStr(PPTAG_BILL_FORMALREASON, temp_buf) <= 0) {
								PPLoadText(PPTXT_EGAIS_REGRADING, temp_buf);
							}
							// regrading
							n_h.PutInner(SXml::nst("ainp", "TypeChargeOn"), EncText(temp_buf));
							if(p_bp->BTagL.GetItemStr(PPTAG_BILL_COMPLEMENTARY, temp_buf) > 0) {
								n_h.PutInner(SXml::nst("ainp", "ActWriteOff"), EncText(temp_buf));
							}
						}
						THROW_MEM(SETIFZ(P_LecT, new LotExtCodeCore));
						{
							SXml::WNode n_c(_doc, SXml::nst("ainp", "Content"));
							SString infa_ident;
							SString infb_ident;
							StringSet ss_ext_codes;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								const  PPID lot_id = static_cast<PPID>(r_ti.QuotPrice);
								if(lot_id && IsAlcGoods(r_ti.GoodsID)) {
									ObjTagList tag_list;
									p_ref->Ot.GetList(PPOBJ_LOT, lot_id, &tag_list);
									uint   ext_code_count = 0;
									int    ext_code_exists = 0;
									if(tag_list.GetCount()) {
										const int mmlr = P_LecT->GetMarkListByLot(lot_id, &ExclChrgOnMarks, ss_ext_codes, &ext_code_count);
										THROW(mmlr);
										if(mmlr > 0)
											ext_code_exists = 1;
										SXml::WNode w_p(_doc, SXml::nst("ainp", "Position"));
										w_p.PutInner(SXml::nst("ainp", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
										THROW(WriteProductInfo(_doc, SXml::nst("ainp", "Product"), r_ti.GoodsID, lot_id, wpifPutManufInfo|wpifVersion2, 0));
										{
											//
											// Передаем количество, устанавливаемое на баланс.
											// Если в сопровождении с лотом идут коды марок, то в качестве количества применяем количество этих марок.
											// В противном случае берем количество из строки документа (остаток по лоту на момент формирования документа).
											//
											const long org_qtty = R0i(fabs(r_ti.Qtty()));
											const long sqtty = /*ext_code_count*/ext_code_exists ? static_cast<long>(ext_code_count) : org_qtty;
											w_p.PutInner(SXml::nst("ainp", "Quantity"), EncText(temp_buf.Z().Cat(sqtty)));
											if(/*ext_code_count*/ext_code_exists && sqtty != org_qtty) {
												temp_buf.Z().CatEq("LotID", lot_id).Space().CatEq("Qtty", org_qtty).Space().CatEq("Sended Qtty", sqtty);
												LogTextWithAddendum(PPTXT_EGAIS_MARKQTYNEQLOTQTY, temp_buf);
											}
										}
										{
											const ObjTagItem * p_infaid_tag = tag_list.GetItem(PPTAG_LOT_FSRARINFA);
											const ObjTagItem * p_infbid_tag = tag_list.GetItem(PPTAG_LOT_FSRARINFB);
											SXml::WNode n_infab(_doc, SXml::nst("ainp", "InformF1F2"));
											if(tag_list.GetItemStr(PPTAG_LOT_EGIASINFAREG, temp_buf) > 0) {
												InformAReg iar;
												if(iar.FromStr(temp_buf)) {
													SXml::WNode n_infabreg(_doc, SXml::nst("ainp", "InformF1F2Reg"));
													{
														SXml::WNode n_infa(_doc, SXml::nst("ainp", "InformF1"));
														n_infa.PutInner(SXml::nst("iab", "Quantity"), EncText(temp_buf.Z().Cat(iar.Qtty)));
														n_infa.PutInnerValidDate(SXml::nst("iab", "BottlingDate"), iar.ManufDate, DATF_ISO8601CENT);
														n_infa.PutInnerSkipEmpty(SXml::nst("iab", "TTNNumber"), EncText(temp_buf = iar.TTNCode));
														n_infa.PutInnerValidDate(SXml::nst("iab", "TTNDate"), iar.TTNDate, DATF_ISO8601CENT);
														n_infa.PutInnerSkipEmpty(SXml::nst("iab", "EGAISFixNumber"), EncText(temp_buf = iar.EGAISCode));
														n_infa.PutInnerValidDate(SXml::nst("iab", "EGAISFixDate"), iar.EGAISDate, DATF_ISO8601CENT);
													}
												}
												else
													LogLastError();
											}
										}
										if(ext_code_count) {
											SXml::WNode n_makr(_doc, SXml::nst("ainp", "MarkCodeInfo"));
											for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);)
												n_makr.PutInner(/*"ainp:MarkCode"*/"MarkCode", EncText(temp_buf));
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTCHARGEON) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						{
							SXml::WNode n_h(_doc, SXml::nst("ain", "Header"));
							n_h.PutInner(SXml::nst("ain", "Number"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("ain", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							// @v8.9.10 (Не хотят передавать свои примечания в ЕГАИС) n_h.PutInnerSkipEmpty("wb:Note", EncText(temp_buf = p_bp->Rec.Memo)); // Примечание
						}
						THROW_MEM(SETIFZ(P_LecT, new LotExtCodeCore));
						{
							SXml::WNode n_c(_doc, SXml::nst("ain", "Content"));
							SString infa_ident;
							SString infb_ident;
							StringSet ss_ext_codes;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								const  PPID lot_id = static_cast<PPID>(r_ti.QuotPrice);
								if(lot_id && IsAlcGoods(r_ti.GoodsID)) {
									ObjTagList tag_list;
									p_ref->Ot.GetList(PPOBJ_LOT, lot_id, &tag_list);
									uint   ext_code_count = 0;
									int    ext_code_exists = 0;
									if(tag_list.GetCount()) {
										const int mmlr = P_LecT->GetMarkListByLot(lot_id, &ExclChrgOnMarks, ss_ext_codes, &ext_code_count);
										THROW(mmlr);
										if(mmlr > 0)
											ext_code_exists = 1;
										SXml::WNode w_p(_doc, SXml::nst("ain", "Position"));
										w_p.PutInner(SXml::nst("ain", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
										THROW(WriteProductInfo(_doc, SXml::nst("ain", "Product"), r_ti.GoodsID, lot_id, 0, 0));
										{
											//
											// Передаем количество, устанавливаемое на баланс.
											// Если в сопровождении с лотом идут коды марок, то в качестве количества применяем количество этих марок.
											// В противном случае берем количество из строки документа (остаток по лоту на момент формирования документа).
											//
											const long org_qtty = R0i(fabs(r_ti.Qtty()));
											const long sqtty = /*ext_code_count*/ext_code_exists ? static_cast<long>(ext_code_count) : org_qtty;
											w_p.PutInner(SXml::nst("ain", "Quantity"), EncText(temp_buf.Z().Cat(sqtty)));
											if(/*ext_code_count*/ext_code_exists && sqtty != org_qtty) {
												temp_buf.Z().CatEq("LotID", lot_id).Space().CatEq("Qtty", org_qtty).Space().CatEq("Sended Qtty", sqtty);
												LogTextWithAddendum(PPTXT_EGAIS_MARKQTYNEQLOTQTY, temp_buf);
											}
										}
										{
											const ObjTagItem * p_infaid_tag = tag_list.GetItem(PPTAG_LOT_FSRARINFA);
											const ObjTagItem * p_infbid_tag = tag_list.GetItem(PPTAG_LOT_FSRARINFB);
											SXml::WNode n_infab(_doc, SXml::nst("ain", "InformAB"));
											if(p_infaid_tag && p_infbid_tag && p_infaid_tag->GetStr(infa_ident) && infa_ident.NotEmptyS() && p_infbid_tag->GetStr(infb_ident) && infb_ident.NotEmptyS()) {
												SXml::WNode n_infabkey(_doc, SXml::nst("ain", "InformABKey"));
												n_infabkey.PutInner(SXml::nst("ain", "FormA"), EncText(infa_ident));
												n_infabkey.PutInner(SXml::nst("ain", "LastFormB"), EncText(infb_ident));
											}
											else if(tag_list.GetItemStr(PPTAG_LOT_EGIASINFAREG, temp_buf) > 0) {
												InformAReg iar;
												if(iar.FromStr(temp_buf)) {
													SXml::WNode n_infabreg(_doc, SXml::nst("ain", "InformABReg"));
													{
														SXml::WNode n_infa(_doc, SXml::nst("ain", "InformA"));
														n_infa.PutInner(SXml::nst("iab", "Quantity"), EncText(temp_buf.Z().Cat(iar.Qtty)));
														n_infa.PutInnerValidDate(SXml::nst("iab", "BottlingDate"), iar.ManufDate, DATF_ISO8601CENT);
														n_infa.PutInnerSkipEmpty(SXml::nst("iab", "TTNNumber"), EncText(temp_buf = iar.TTNCode));
														n_infa.PutInnerValidDate(SXml::nst("iab", "TTNDate"), iar.TTNDate, DATF_ISO8601CENT);
														n_infa.PutInnerSkipEmpty(SXml::nst("iab", "EGAISFixNumber"), EncText(temp_buf = iar.EGAISCode));
														n_infa.PutInnerValidDate(SXml::nst("iab", "EGAISFixDate"), iar.EGAISDate, DATF_ISO8601CENT);
													}
												}
												else
													LogLastError();
											}
										}
										if(ext_code_count) {
											SXml::WNode n_makr(_doc, SXml::nst("ain", "MarkCodeInfo"));
											for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);)
												n_makr.PutInner(SXml::nst("ain", "MarkCode"), EncText(temp_buf));
										}
									}
								}
							}
						}
					}
					else if(oneof3(doc_type, PPEDIOP_EGAIS_ACTWRITEOFF, PPEDIOP_EGAIS_ACTWRITEOFF_V2, PPEDIOP_EGAIS_ACTWRITEOFF_V3)) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("awr", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("awr", "Header"));
							n_h.PutInner(SXml::nst("awr", "ActNumber"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("awr", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							if(p_bp->BTagL.GetItemStr(PPTAG_BILL_FORMALREASON, temp_buf) <= 0)
								PPLoadString("losses", temp_buf); // Потери
							n_h.PutInner(SXml::nst("awr", "TypeWriteOff"), EncText(temp_buf));
							// @v8.9.10 (Не хотят передавать свои примечания в ЕГАИС) n_h.PutInnerSkipEmpty("awr:Note", EncText(temp_buf = p_bp->Rec.Memo)); // Примечание
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("awr", "Content"));
							SString infa_ident;
							SString infb_ident;
							StringSet ss_ext_codes;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, r_ti.LotID, 0, 0, agi) > 0) {
									SXml::WNode w_p(_doc, SXml::nst("awr", "Position"));
									w_p.PutInner(SXml::nst("awr", "Identity"), EncText(temp_buf.Z().Cat(r_ti.RByBill)));
									{
										double qtty = fabs(r_ti.Quantity_);
										double item_amount = r_ti.NetPrice() * qtty;
										long   qtty_fmt = MKSFMTD(0, 0, NMBF_NOTRAILZ);
										if(agi.UnpackedVolume > 0.0) {
											const double mult = agi.UnpackedVolume / 10.0;
											qtty = (qtty * mult); // Неупакованная продукция передается в декалитрах
											qtty_fmt = MKSFMTD_030;
										}
										w_p.PutInner(SXml::nst("awr", "Quantity"), EncText(temp_buf.Z().Cat(qtty, qtty_fmt)));
										if(doc_type == PPEDIOP_EGAIS_ACTWRITEOFF_V3) {
											w_p.PutInner(SXml::nst("awr", "SumSale"), EncText(temp_buf.Z().Cat(item_amount, MKSFMTD_020)));
										}
									}
									{
										ReceiptTbl::Rec lot_rec;
										P_BObj->trfr->Rcpt.Search(r_ti.LotID, &lot_rec);
										P_BObj->MakeLotText(&lot_rec, PPObjBill::ltfGoodsName, temp_buf);
										lot_text.Z().CatChar('[').Cat(r_ti.RByBill).CatChar(']').Space().Cat(temp_buf);
										//
										THROW_PP_S(p_bp->LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARINFB, temp_buf) > 0, PPERR_EGAIS_NOINFBIDINLOT, lot_text);
										if(doc_type == PPEDIOP_EGAIS_ACTWRITEOFF) {
											SXml::WNode w_refb(_doc, SXml::nst("awr", "InformB"));
											w_refb.PutInner(SXml::nst("pref", "BRegId"), temp_buf);
										}
										else if(doc_type == PPEDIOP_EGAIS_ACTWRITEOFF_V2) {
											SXml::WNode n_infab(_doc, SXml::nst("awr", "InformF1F2"));
											{
												SXml::WNode w_refb(_doc, SXml::nst("awr", "InformF2"));
												w_refb.PutInner(SXml::nst("pref", "F2RegId"), EncText(temp_buf));
											}
										}
										else if(doc_type == PPEDIOP_EGAIS_ACTWRITEOFF_V3) {
											{ // (марки следуют за зоной InformF1F2
												SXml::WNode n_infab(_doc, SXml::nst("awr", "InformF1F2"));
												{
													SXml::WNode w_refb(_doc, SXml::nst("awr", "InformF2"));
													w_refb.PutInner(SXml::nst("pref", "F2RegId"), EncText(temp_buf));
												}
											}
											if(p_bp->XcL.Get(tidx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
												// В этом документе марки передаются без информации о боксах
												SXml::WNode n_mci(_doc, SXml::nst("awr", "MarkCodeInfo"));
												for(uint markidx = 0; markidx < ext_codes_set.GetCount(); markidx++) {
													if(ext_codes_set.GetByIdx(markidx, msentry) && !(msentry.Flags & PPLotExtCodeContainer::fBox)) {
														//SXml::WNode w_amclist(_doc, SXml::nst("ce", "amclist"));
														/*w_amclist*/n_mci.PutInner(SXml::nst("ce", "amc"), EncText(msentry.Num));
													}
												}
											}
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_TRANSFERTOSHOP) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("tts", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("tts", "Header"));
							n_h.PutInner(SXml::nst("tts", "TransferNumber"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("tts", "TransferDate"),   EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("tts", "Content"));
							SString infb_ident;
							SString rar_product_ident;
							SString qtty_buf;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(r_ti.Quantity_ > 0.0 && IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
									const ObjTagList * p_ti_tag_list = p_bp->LTagL.Get(tidx);
									infb_ident.Z();
									rar_product_ident.Z();
									CALLPTRMEMB(p_ti_tag_list, GetItemStr(PPTAG_LOT_FSRARINFB, infb_ident));
									CALLPTRMEMB(p_ti_tag_list, GetItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, rar_product_ident));
									rar_product_ident.SetIfEmpty(agi.EgaisCode);
									if(rar_product_ident.NotEmpty() && infb_ident.NotEmpty()) {
										double qtty = fabs(r_ti.Quantity_);
										long   qtty_fmt = MKSFMTD(0, 0, NMBF_NOTRAILZ);
										if(agi.UnpackedVolume > 0.0) {
											const double mult = agi.UnpackedVolume / 10.0;
											qtty = (qtty * mult); // Неупакованная продукция передается в декалитрах
											qtty_fmt = MKSFMTD(0, 3, NMBF_NOTRAILZ);
										}
										qtty_buf.Z().Cat(qtty, qtty_fmt);
										if(qtty_buf.NotEmptyS()) {
											SXml::WNode w_p(_doc, SXml::nst("tts", "Position"));
											w_p.PutInner(SXml::nst("tts", "Identity"),    EncText(temp_buf.Z().Cat(r_ti.RByBill)));
											w_p.PutInner(SXml::nst("tts", "ProductCode"), EncText(temp_buf = rar_product_ident));
											w_p.PutInner(SXml::nst("tts", "Quantity"), EncText(qtty_buf));
											{
												SXml::WNode w_refb(_doc, SXml::nst("tts", "InformF2"));
												w_refb.PutInner(SXml::nst("pref", "F2RegId"), EncText(temp_buf = infb_ident));
											}
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_TRANSFERFROMSHOP) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("tfs", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("tfs", "Header"));
							n_h.PutInner(SXml::nst("tfs", "TransferNumber"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("tfs", "TransferDate"),   EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("tfs", "Content"));
							SString infb_ident;
							SString rar_product_ident;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(r_ti.Quantity_ < 0.0 && IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
									const ObjTagList * p_ti_tag_list = p_bp->LTagL.Get(tidx);
									infb_ident.Z();
									rar_product_ident.Z();
									CALLPTRMEMB(p_ti_tag_list, GetItemStr(PPTAG_LOT_FSRARINFB, infb_ident));
									CALLPTRMEMB(p_ti_tag_list, GetItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, rar_product_ident));
									rar_product_ident.SetIfEmpty(agi.EgaisCode);
									if(rar_product_ident.NotEmpty() && infb_ident.NotEmpty()) {
										double qtty = fabs(r_ti.Quantity_);
										long   qtty_fmt = MKSFMTD(0, 0, NMBF_NOTRAILZ);
										if(agi.UnpackedVolume > 0.0) {
											const double mult = agi.UnpackedVolume / 10.0;
											qtty = (qtty * mult); // Неупакованная продукция передается в декалитрах
											qtty_fmt = MKSFMTD_030;
										}
										SXml::WNode w_p(_doc, SXml::nst("tfs", "Position"));
										w_p.PutInner(SXml::nst("tfs", "Identity"),    EncText(temp_buf.Z().Cat(r_ti.RByBill)));
										w_p.PutInner(SXml::nst("tfs", "ProductCode"), EncText(temp_buf = rar_product_ident));
										w_p.PutInner(SXml::nst("tfs", "Quantity"), EncText(temp_buf.Z().Cat(qtty, qtty_fmt)));
										{
											SXml::WNode w_refb(_doc, SXml::nst("tfs", "InformF2"));
											w_refb.PutInner(SXml::nst("pref", "F2RegId"), EncText(temp_buf = infb_ident));
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTFIXBARCODE) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
						n_dt.PutInner(SXml::nst("awr", "Identity"), temp_buf.Z().Cat(p_bp->Rec.ID));
						{
							SXml::WNode n_h(_doc, SXml::nst("awr", "Header"));
							n_h.PutInner(SXml::nst("awr", "Number"), EncText(temp_buf = p_bp->Rec.Code));
							n_h.PutInner(SXml::nst("awr", "ActDate"), EncText(temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT)));
							n_h.PutInner(SXml::nst("awr", "Note"), EncText(temp_buf.Z()));
						}
						{
							SXml::WNode n_c(_doc, SXml::nst("awr", "Content"));
							SString infb_ident;
							for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
								const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
								if(IsAlcGoods(r_ti.GoodsID) && PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
									const ObjTagList * p_ti_tag_list = p_bp->LTagL.Get(tidx);
									infb_ident.Z();
									CALLPTRMEMB(p_ti_tag_list, GetItemStr(PPTAG_LOT_FSRARINFB, infb_ident));
									if(infb_ident.NotEmpty() && (p_bp->XcL.Get(tidx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount())) {
										SXml::WNode w_p(_doc, SXml::nst("awr", "Position"));
										w_p.PutInner(SXml::nst("awr", "Identity"),    EncText(temp_buf.Z().Cat(r_ti.RByBill)));
										w_p.PutInner(SXml::nst("awr", "Inform2RegId"), EncText(temp_buf = infb_ident));
										{
											SXml::WNode n_mci(_doc, SXml::nst("awr", "MarkInfo"));
											for(uint markidx = 0; markidx < ext_codes_set.GetCount(); markidx++) {
												if(ext_codes_set.GetByIdx(markidx, msentry) && !(msentry.Flags & PPLotExtCodeContainer::fBox)) {
													n_mci.PutInner(SXml::nst("ce", "amc"), EncText(msentry.Num));
												}
											}
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTUNFIXBARCODE) {
						const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
					}
					else if(doc_type == PPEDIOP_EGAIS_REQUESTREPEALWB) {
						const RepealWb * p_rwb = static_cast<const RepealWb *>(rPack.P_Data);
						/*
						<qp:ClientId>030000194005</qp:ClientId>
						<qp:RequestNumber>011</qp:RequestNumber>
						<qp:RequestDate>2016-05-06T13:00:00</qp:RequestDate>
						<qp:WBRegId>TTN-0021795603</qp:WBRegId>
						*/
						n_dt.PutInner(SXml::nst("qp", "ClientId"), EncText(fsrar_ident));
						n_dt.PutInner(SXml::nst("qp", "RequestNumber"), EncText(temp_buf.Z().Cat(p_rwb->ReqNumber)));
						n_dt.PutInner(SXml::nst("qp", "RequestDate"), temp_buf.Z().CatCurDateTime(DATF_ISO8601CENT, 0));
						n_dt.PutInner(SXml::nst("qp", "WBRegId"), EncText(temp_buf.Z().Cat(p_rwb->TTNCode)));
					}
					else if(doc_type == PPEDIOP_EGAIS_REQUESTREPEALAWO) {
						const RepealWb * p_rwb = static_cast<const RepealWb *>(rPack.P_Data);
						/*
						<qp:ClientId>030000194005</qp:ClientId>
						<qp:RequestNumber>011</qp:RequestNumber>
						<qp:RequestDate>2016-05-06T13:00:00</qp:RequestDate>
						<qp:AWORegId>TTN-0021795603</qp:WBRegId>
						*/
						n_dt.PutInner(SXml::nst("qp", "ClientId"), EncText(fsrar_ident));
						n_dt.PutInner(SXml::nst("qp", "RequestNumber"), EncText(temp_buf.Z().Cat(p_rwb->ReqNumber)));
						n_dt.PutInner(SXml::nst("qp", "RequestDate"), temp_buf.Z().CatCurDateTime(DATF_ISO8601CENT, 0));
						n_dt.PutInner(SXml::nst("qp", "AWORegId"), EncText(temp_buf.Z().Cat(p_rwb->TTNCode)));
					}
					else if(doc_type == PPEDIOP_EGAIS_CONFIRMREPEALWB) {
						const RepealWb * p_rwb = static_cast<const RepealWb *>(rPack.P_Data);
						/*
							<wt:Header>
							<wt:IsConfirm>Accepted</wt:IsConfirm>
							<wt:ConfirmNumber>0001</wt:ConfirmNumber>
							<wt:ConfirmDate>2014-12-17</wt:ConfirmDate>
							<wt:WBRegId>TTN-0021795603</wt:WBRegId>
							<wt:Note>Подтверждаем отмену проведения ТТН</wt:Note>
							</wt:Header>
						*/
						SXml::WNode n_c(_doc, SXml::nst("wt", "Header"));
						n_c.PutInner(SXml::nst("wt", "IsConfirm"), p_rwb->Confirm ? "Accepted" : "Rejected");
						n_c.PutInner(SXml::nst("wt", "ConfirmNumber"), EncText(temp_buf.Z().Cat(p_rwb->ReqNumber)));
						n_c.PutInner(SXml::nst("wt", "ConfirmDate"), temp_buf.Z().Cat(getcurdate_(), DATF_ISO8601CENT));
						n_c.PutInner(SXml::nst("wt", "WBRegId"), EncText(temp_buf.Z().Cat(p_rwb->TTNCode)));
						n_c.PutInnerSkipEmpty(SXml::nst("wt", "Note"), EncText(p_rwb->Memo));
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTUSING) {
					}
					else if(doc_type == PPEDIOP_EGAIS_ACTREJECT) {
					}
					else {
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

struct EgaisGoodsNameReplacement {
	const char * P_Pattern;
	const char * P_Subst;
};

SString & FASTCALL PPEgaisProcessor::PreprocessGoodsName(SString & rName) const
{
	rName.Strip();
	if(P_Taw) {
		SString result_buf;
        rName.Transf(CTRANSF_OUTER_TO_INNER);
		if(P_Taw->ReplaceString(rName, result_buf) > 0) {
			rName = result_buf.Strip();
		}
		rName.Transf(CTRANSF_INNER_TO_OUTER);
	}
	return rName;
}

int PPEgaisProcessor::Read_ProductInfo(xmlNode * pFirstNode, PPGoodsPacket * pPack,
	PrcssrAlcReport::GoodsItem * pExt, PrcssrAlcReport::RefCollection * pRefC, SFile * pOutFile)
{
	int    ok = 1;
	PPID   goods_id = 0;
	PrcssrAlcReport::GoodsItem alc_goods_item;
	SString temp_buf, temp_buf2;
	SString full_name, short_name;
	//SString rar_ident;
	//SString ap_code; // Код алкогольной продукции (не путать с rar_ident - кодом конкретного наименования)
	const  PPID manuf_tag_id = Cfg.LotManufTagList.getCount() ? Cfg.LotManufTagList.get(0) : 0;
	PPID   manuf_psn_kind_id = PPPRK_MANUF;
	int    manuf_refc_pos = -1;
	int    imp_refc_pos = -1;
	int    is_unpacked = 0;
	PPPersonPacket psn_manuf;
	PPPersonPacket psn_imp;
	if(pRefC)
		pRefC->LastProductP = -1;
	if(manuf_tag_id) {
		PPObjectTag tag_rec;
        if(TagObj.Fetch(manuf_tag_id, &tag_rec) > 0) {
			if(tag_rec.TagDataType == OTTYP_OBJLINK && tag_rec.TagEnumID == PPOBJ_PERSON && tag_rec.LinkObjGrp)
				manuf_psn_kind_id = tag_rec.LinkObjGrp;
        }
	}
	for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "Identity"))
			;
		else if(SXml::IsName(p_n, "Type"))
			; // Не используем - здесь всегда "АП"
		else if(SXml::GetContentByName(p_n, "UnitType", temp_buf)) {
			if(temp_buf.IsEqiAscii("Unpacked")) {
				is_unpacked = 1;
			}
		}
		else if(SXml::GetContentByName(p_n, "FullName", full_name))
			full_name.Utf8ToChar();
		else if(SXml::GetContentByName(p_n, "ShortName", short_name))
			short_name.Utf8ToChar();
		else if(SXml::GetContentByName(p_n, "AlcCode", alc_goods_item.EgaisCode))
			;
		else if(SXml::GetContentByName(p_n, "ProductVCode", alc_goods_item.CategoryCode))
			;
		else if(SXml::GetContentByName(p_n, "Capacity", temp_buf))
			alc_goods_item.Volume = temp_buf.ToReal();
		else if(SXml::GetContentByName(p_n, "AlcVolume", temp_buf))
			alc_goods_item.Proof = temp_buf.ToReal();
		else if(SXml::IsName(p_n, "Producer")) {
			Read_OrgInfo(p_n->children, manuf_psn_kind_id, EgaisPersonCore::rolefManuf, pPack ? &psn_manuf : 0, pRefC, 0);
			if(pRefC && pRefC->LastPersonP >= 0)
				manuf_refc_pos = pRefC->LastPersonP;
		}
		else if(SXml::IsName(p_n, "Importer")) {
			Read_OrgInfo(p_n->children, manuf_psn_kind_id, EgaisPersonCore::rolefImporter, pPack ? &psn_imp : 0, pRefC, 0);
			if(pRefC && pRefC->LastPersonP >= 0)
				imp_refc_pos = pRefC->LastPersonP;
		}
	}
	if(pOutFile) {
		SString out_line_buf;
        out_line_buf.Cat(alc_goods_item.EgaisCode).Tab().Cat(alc_goods_item.CategoryCode).Tab().Cat(alc_goods_item.Volume, MKSFMTD_030).Tab().
			Cat(alc_goods_item.Proof, MKSFMTD(0, 1, 0)).Tab().Cat(full_name).Tab().Cat(short_name).CR();
		pOutFile->WriteLine(out_line_buf);
	}
	{
		EgaisProductCore::Item pi;
		int16   country_code = 0;
		STRNSCPY(pi.AlcoCode, alc_goods_item.EgaisCode);
		STRNSCPY(pi.CategoryCode, alc_goods_item.CategoryCode);
		pi.Proof = alc_goods_item.Proof;
		pi.Volume = alc_goods_item.Volume;
		pi.Flags  = EgaisProductCore::fVerified;
		(pi.Name = short_name).Transf(CTRANSF_OUTER_TO_INNER);
		(pi.FullName = full_name).Transf(CTRANSF_OUTER_TO_INNER);
		if(pRefC) {
			if(manuf_refc_pos >= 0) {
				const EgaisPersonCore::Item * p_person = pRefC->PersonList.at(manuf_refc_pos);
				if(p_person) {
					STRNSCPY(pi.ManufRarIdent, p_person->RarIdent);
					country_code = p_person->CountryCode;
				}
			}
			if(imp_refc_pos >= 0) {
				const EgaisPersonCore::Item * p_person = pRefC->PersonList.at(imp_refc_pos);
				if(p_person)
					STRNSCPY(pi.ImporterRarIdent, p_person->RarIdent);
			}
			THROW(pRefC->SetProduct(pi)); // 0178140000002304670
		}
		if(pExt) {
			if(psn_imp.Rec.ID)
				pExt->MnfOrImpPsnID = psn_imp.Rec.ID;
			else if(psn_manuf.Rec.ID)
				pExt->MnfOrImpPsnID = psn_manuf.Rec.ID;
			pExt->EgaisCode = alc_goods_item.EgaisCode;
			pExt->CategoryCode = pi.CategoryCode;
			pExt->RefcManufCode = pi.ManufRarIdent;
			pExt->RefcImporterCode = pi.ImporterRarIdent;
			pExt->Proof = pi.Proof;
			pExt->Volume = pi.Volume;
			pExt->CountryCode = country_code;
			pExt->OuterUnpackedTag = is_unpacked;
		}
		if(pPack) {
			pPack->Z();
			if(alc_goods_item.EgaisCode.NotEmptyS()) {
				int    found_by_rarcode = 0;
				SString goods_name;
				{
					//
					// Формируем наименование товара
					//
					if(full_name.NotEmptyS())
						goods_name = full_name;
					else if(short_name.NotEmptyS())
						goods_name = short_name;
					PreprocessGoodsName(goods_name);
					if(alc_goods_item.Proof > 0.0) {
						temp_buf.Z().Cat(alc_goods_item.Proof, MKSFMTD(0, 1, NMBF_NOTRAILZ)).CatChar('%');
						if(!goods_name.Search(temp_buf, 0, 1, 0))
							goods_name.Space().Cat(temp_buf);
					}
					if(alc_goods_item.Volume > 0.0) {
						PPLoadString("@munit_l", temp_buf2);
						temp_buf.Z().Cat(alc_goods_item.Volume, MKSFMTD(0, 3, NMBF_NOTRAILZ)).Cat(temp_buf2);
						if(!goods_name.Search(temp_buf, 0, 1, 0))
							goods_name.Space().Cat(temp_buf);
					}
					{
						temp_buf = alc_goods_item.EgaisCode;
						if(!goods_name.Search(temp_buf, 0, 1, 0))
							goods_name.Space().Cat(temp_buf);
					}
					goods_name.Transf(CTRANSF_OUTER_TO_INNER);
				}
				if(SearchGoodsByRarCode(alc_goods_item.EgaisCode, &goods_id) > 0) {
					THROW(GObj.GetPacket(goods_id, pPack, 0) > 0);
					found_by_rarcode = 1;
				}
				else if(ACfg.Hdr.Flags & ACfg.Hdr.fSkipBillWithUnresolvedItems) {
					LogTextWithAddendum(PPTXT_EGAIS_GOODSREJECTEDCFG, goods_name);
					ok = -1;
				}
				else {
					goods_id = 0; // @paranoic
					pPack->Rec.Kind = PPGDSK_GOODS;
					STRNSCPY(pPack->Rec.Name, goods_name);
					STRNSCPY(pPack->Rec.Abbr, goods_name);
					pPack->Rec.Flags |= GF_AUTOCREATE;
				}
				if(ok > 0) {
					SETIFZ(pPack->Rec.GdsClsID, Cfg.E.AlcGoodsClsID);
					if(pPack->Rec.GdsClsID) {
						PPGdsClsPacket gc_pack;
						THROW(GcObj.Fetch(pPack->Rec.GdsClsID, &gc_pack) > 0);
						SETIFZ(pPack->Rec.ParentID, gc_pack.Rec.DefGrpID);
						SETIFZ(pPack->Rec.UnitID, gc_pack.Rec.DefUnitID);
						if(Cfg.VolumeClsDim && alc_goods_item.Volume > 0.0) {
							double org_val = 0.0;
							gc_pack.GetExtDim(&pPack->ExtRec, Cfg.VolumeClsDim, &org_val);
							if(org_val <= 0.0)
								gc_pack.RealToExtDim(alc_goods_item.Volume, Cfg.VolumeClsDim, pPack->ExtRec);
						}
						if(Cfg.E.ProofClsDim && alc_goods_item.Proof > 0.0) {
							double org_val = 0.0;
							gc_pack.GetExtDim(&pPack->ExtRec, Cfg.E.ProofClsDim, &org_val);
							if(org_val <= 0.0)
								gc_pack.RealToExtDim(alc_goods_item.Proof, Cfg.E.ProofClsDim, pPack->ExtRec);
						}
						if(Cfg.CategoryClsDim && alc_goods_item.CategoryCode.ToLong() > 0) {
							if(oneof4(Cfg.CategoryClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW)) {
								double org_val = 0.0;
								gc_pack.GetExtDim(&pPack->ExtRec, Cfg.CategoryClsDim, &org_val);
								if(org_val <= 0.0)
									gc_pack.RealToExtDim(alc_goods_item.CategoryCode.ToLong(), Cfg.CategoryClsDim, pPack->ExtRec);
							}
						}
					}
					if(Cfg.CategoryTagID && alc_goods_item.CategoryCode.NotEmptyS()) {
						const ObjTagItem * p_tag_item = pPack->TagL.GetItem(Cfg.CategoryTagID);
						if(!p_tag_item) {
							THROW(pPack->TagL.PutItemStr(Cfg.CategoryTagID, alc_goods_item.CategoryCode));
						}
					}
					if(!found_by_rarcode) {
						THROW(pPack->AddCode(alc_goods_item.EgaisCode, 0, 1.0));
					}
					THROW(GObj.PutPacket(&goods_id, pPack, 1));
					if(!found_by_rarcode)
						LogTextWithAddendum(/*PPTXT_EGAIS_GOODSCREATED*/PPTXT_WAREAUTOCREATED, goods_name);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Read_ActInventoryInformBReg(xmlNode * pFirstNode, PPEgaisProcessor::Packet * pPack)
{
	int   ok = 1;
	SString temp_buf;
	ActInformItem * p_item = 0;
	if(pPack && pPack->P_Data) {
		PPEgaisProcessor::ActInform * p_pack = static_cast<PPEgaisProcessor::ActInform *>(pPack->P_Data);
		for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "Header")) {
				for(const xmlNode * p_a = p_n->children; p_a; p_a = p_a->next) {
					if(SXml::GetContentByName(p_a, "ActRegId", temp_buf))
						(p_pack->ActRegId = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
					else if(SXml::GetContentByName(p_a, "Number", temp_buf))
						(p_pack->ActNumber = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
			else if(SXml::IsName(p_n, "Content")) {
				for(const xmlNode * p_a = p_n->children; p_a; p_a = p_a->next) {
					if(SXml::IsName(p_a, "Position")) {
						THROW_MEM(p_item = new ActInformItem);
						for(const xmlNode * p_p = p_a->children; p_p; p_p = p_p->next) {
							if(SXml::GetContentByName(p_p, "Identity", temp_buf))
								p_item->P = temp_buf.ToLong();
							else if(SXml::GetContentByName(p_p, "InformARegId", temp_buf) || SXml::GetContentByName(p_p, "InformF1RegId", temp_buf)) {
								temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_item->AIdent, sizeof(p_item->AIdent));
							}
							else if(SXml::IsName(p_p, "InformB") || SXml::IsName(p_p, "InformF2")) {
								for(const xmlNode * p_b = p_p->children; p_b; p_b = p_b->next) {
									if(SXml::IsName(p_b, "InformBItem") || SXml::IsName(p_b, "InformF2Item")) {
										InformBItem bitem;
										for(const xmlNode * p_i = p_b->children; p_i; p_i = p_i->next) {
											if(SXml::GetContentByName(p_i, "Identity", temp_buf))
												bitem.P = temp_buf.ToLong();
											else if(SXml::GetContentByName(p_i, "BRegId", temp_buf) || SXml::GetContentByName(p_i, "F2RegId", temp_buf))
												STRNSCPY(bitem.Ident, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
										}
										THROW_SL(p_item->BItems.insert(&bitem));
									}
								}
							}
						}
						p_pack->Items.insert(p_item);
						p_item = 0;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_item;
	return ok;
}

int PPEgaisProcessor::AssignManufTypeToPersonPacket(PPPersonPacket & rPack, int roleFlag)
{
	int    ok = -1;
	int    manuf_type = -1;
	if(roleFlag & EgaisPersonCore::rolefManuf)
		manuf_type = 1;
	else if(roleFlag & EgaisPersonCore::rolefImporter)
		manuf_type = 2;
	if(oneof2(manuf_type, 1, 2)) {
		SString temp_buf;
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
        if(Cfg.ManufImpTagID && tag_obj.Fetch(Cfg.ManufImpTagID, &tag_rec) > 0) {
			const ObjTagItem * p_item = rPack.TagL.GetItem(Cfg.ManufImpTagID);
			if(!p_item) {
				ObjTagItem new_tag_item;
				int    is_new_tag_inited = 0;
				if(tag_rec.TagDataType == OTTYP_ENUM) {
					PPTagEnumList enl(tag_rec.TagEnumID);
					if(enl.Read(tag_rec.TagEnumID)) {
                        for(uint i = 0; !is_new_tag_inited && i < enl.getCount(); i++) {
							StrAssocArray::Item item = enl.at_WithoutParent(i);
							temp_buf = item.Txt;
							const long temp_val = temp_buf.ToLong();
							if(temp_val == 1 && manuf_type == 1) {
								if(new_tag_item.SetInt(Cfg.ManufImpTagID, item.Id))
									is_new_tag_inited = 1;
							}
							else if(temp_val == 2 && manuf_type == 2) {
								if(new_tag_item.SetInt(Cfg.ManufImpTagID, item.Id))
									is_new_tag_inited = 1;
							}
                        }
					}
				}
				else if(tag_rec.TagDataType == OTTYP_INT) {
					if(new_tag_item.SetInt(Cfg.ManufImpTagID, manuf_type))
						is_new_tag_inited = 1;
				}
				else if(tag_rec.TagDataType == OTTYP_STRING) {
					if(new_tag_item.SetStr(Cfg.ManufImpTagID, temp_buf.Z().Cat(manuf_type)))
						is_new_tag_inited = 1;
				}
				if(is_new_tag_inited) {
					THROW(rPack.TagL.PutItem(Cfg.ManufImpTagID, &new_tag_item));
					ok = 1;
				}
			}
        }
        if(ok < 0 && Cfg.E.ImporterPersonKindID) {
			if(manuf_type == 2) {
				rPack.Kinds.addUnique(Cfg.E.ImporterPersonKindID);
				ok = 1;
			}
			else if(manuf_type == 1) {
				rPack.Kinds.removeByID(Cfg.E.ImporterPersonKindID);
				ok = 1;
			}
        }
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Read_OrgInfo(xmlNode * pFirstNode, PPID personKindID, int roleFlag, PPPersonPacket * pPack,
	PrcssrAlcReport::RefCollection * pRefC, SFile * pOutFile)
{
	int   ok = 1;
	Reference * p_ref = PPRef;
	PPID  psn_id = 0;
	PPID  loc_id = 0;
	SString temp_buf;
	SString rar_ident;
	SString inn, kpp, unp, rnn;
	SString full_name, short_name;
	SString addr_descr;
	SString city;
	int    country_code = 0;
	int    region_code = 0;
	ObjTagItem tag_item;
	RegisterTbl::Rec new_reg_rec;
	SString zip;
	if(pRefC)
		pRefC->LastPersonP = -1;
	int    j_status = 0;
	for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		//
		// Для 2-й версии протокола есть дополнительная обрамляющая зона идентифицирующая юридический статус персоналии
		//
		int    is_v2_js = 0;
		if(!j_status) {
			if(SXml::IsName(p_n, "UL")) {
				j_status = 1;
				is_v2_js = 1;
			}
			else if(SXml::IsName(p_n, "FL")) {
				j_status = 2;
				is_v2_js = 1;
			}
			else if(SXml::IsName(p_n, "TS")) {
				j_status = 4;
				is_v2_js = 1;
			}
			else if(SXml::IsName(p_n, "FO")) {
				j_status = 3;
				is_v2_js = 1;
			}
			if(is_v2_js)
				p_n = p_n->children; // Переключаемся на внутреннюю область - она идентична в 1 и 2 версиях протокола
		}
		//
		if(SXml::IsName(p_n, "Identity")) {
		}
		else if(SXml::GetContentByName(p_n, "ClientRegId", rar_ident))
			;
		else if(SXml::GetContentByName(p_n, "FullName", full_name))
			full_name.Utf8ToChar();
		else if(SXml::GetContentByName(p_n, "ShortName", short_name))
			short_name.Utf8ToChar();
		else if(SXml::GetContentByName(p_n, "INN", inn))
			;
		else if(SXml::GetContentByName(p_n, "KPP", kpp))
			;
		else if(SXml::GetContentByName(p_n, "UNP", unp))
			;
		else if(SXml::GetContentByName(p_n, "RNN", rnn))
			;
		else if(SXml::IsName(p_n, "address")) {
			for(const xmlNode * p_a = p_n->children; p_a; p_a = p_a->next) {
				if(SXml::GetContentByName(p_a, "Country", temp_buf))
					country_code = temp_buf.ToLong();
				else if(SXml::GetContentByName(p_a, "Index", zip))
					;
				else if(SXml::GetContentByName(p_a, "RegionCode", temp_buf))
					region_code = temp_buf.ToLong();
				else if(SXml::IsName(p_a, "area"))
					;
				else if(SXml::GetContentByName(p_a, "city", city))
					city.Utf8ToChar();
				else if(SXml::IsName(p_a, "place"))
					;
				else if(SXml::IsName(p_a, "street"))
					;
				else if(SXml::IsName(p_a, "house"))
					;
				else if(SXml::IsName(p_a, "building"))
					;
				else if(SXml::IsName(p_a, "liter"))
					;
				else if(SXml::GetContentByName(p_a, "description", addr_descr))
					addr_descr.Utf8ToChar();
			}
		}
	}
	if(pOutFile) {
        SString out_line_buf;
        out_line_buf.Cat(rar_ident).Tab().Cat(inn).Tab().Cat(kpp).Tab().Cat(unp).Tab().Cat(rnn).Cat(full_name).Tab().Cat(short_name).Tab().Cat(country_code).Tab().Cat(city).CR();
        pOutFile->WriteLine(out_line_buf);
	}
	if(rar_ident.NotEmptyS()) {
		if(pRefC) {
			EgaisPersonCore::Item pi;
			STRNSCPY(pi.RarIdent, rar_ident);
			STRNSCPY(pi.INN, inn);
			STRNSCPY(pi.KPP, kpp);
			STRNSCPY(pi.UNP, unp);
			STRNSCPY(pi.RNN, rnn);
			pi.CountryCode = static_cast<int16>(country_code);
			pi.RegionCode = static_cast<int16>(region_code);
			(pi.Name = short_name).Transf(CTRANSF_OUTER_TO_INNER);
			(pi.FullName = full_name).Transf(CTRANSF_OUTER_TO_INNER);
			(pi.AddressDescr = addr_descr).Transf(CTRANSF_OUTER_TO_INNER);
			pi.Flags = roleFlag;
			THROW(pRefC->SetPerson(pi));
		}
		if(pPack) {
			int    pack_extracted = 0; // Если !0 то пакет pPacket идентифицирован (в той или иной
				// степени однозначности) и извлечен из базы данных.
			int    rarid_assigned = 0; // 1 - ид РАР присвоен пакету персоналии, 2 - ид РАР присвоен одному из адресов
			PPLocationPacket loc_pack;
			if(SearchPersonByRarCode(rar_ident, &psn_id, &loc_id) > 0) {
				THROW(PsnObj.GetPacket(psn_id, pPack, 0) > 0);
				pack_extracted = 1;
				rarid_assigned = 1;
				if(loc_id) {
					for(uint dlp = 0; pPack->EnumDlvrLoc(&dlp, &loc_pack);) {
						if(loc_pack.ID == loc_id) {
							pPack->SelectedLocPos = dlp;
							break;
						}
					}
				}
			}
			else {
				kpp.Strip();
				PersonTbl::Rec psn_rec;
				PPIDArray by_inn_psn_list;
				if(inn.NotEmptyS()) {
					PPIDArray id_list;
					PsnObj.GetListByRegNumber(PPREGT_TPID, 0, inn, id_list);
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID _id = id_list.get(i);
						if(PsnObj.Fetch(_id, &psn_rec) > 0)
							by_inn_psn_list.add(_id);
					}
				}
				if(kpp.NotEmptyS()) {
					PPIDArray by_kpp_psn_list;
					LAssocArray by_kpp_loc_list;
					RegisterArray kpp_reg_list;
					for(uint i = 0; i < by_inn_psn_list.getCount(); i++) {
						const  PPID _id = by_inn_psn_list.get(i);
						PPPersonPacket _pack;
						if(PsnObj.GetPacket(_id, &_pack, 0) > 0) {
							if(_pack.Regs.GetListByType(PPREGT_KPP, ZERODATE, &kpp_reg_list) > 0) {
								for(uint j = 0; j < kpp_reg_list.getCount(); j++) {
									(temp_buf = kpp_reg_list.at(j).Num).Strip();
									if(kpp == temp_buf) {
										by_kpp_psn_list.add(_id);
										break;
									}
								}
							}
							if(!(roleFlag & (EgaisPersonCore::rolefManuf|EgaisPersonCore::rolefImporter))) { // Для производителей не сопоставляем КПП с адресами доставки
								for(uint dlp = 0; _pack.EnumDlvrLoc(&dlp, &loc_pack) > 0;) {
									if(loc_pack.Regs.GetListByType(PPREGT_KPP, ZERODATE, &kpp_reg_list) > 0) {
										for(uint j = 0; j < kpp_reg_list.getCount(); j++) {
											(temp_buf = kpp_reg_list.at(j).Num).Strip();
											if(kpp == temp_buf) {
												by_kpp_loc_list.Add(_id, loc_pack.ID, 0);
												break;
											}
										}
									}
								}
							}
						}
					}
					if(by_kpp_loc_list.getCount()) {
						if(by_kpp_loc_list.getCount() > 1) {
							// @todo log Неоднозначность в разрешении склада по КПП
							// Прежде всего рассматриваем те локации, у которых не определен ФСРАР-ИД
							PPIDArray wo_rarid_loc_list;
							for(uint i = 0; i < by_kpp_loc_list.getCount(); i++) {
								const  PPID loc_id = by_kpp_loc_list.at(i).Val;
								if(loc_id && p_ref->Ot.GetTagStr(PPOBJ_LOCATION, loc_id, PPTAG_LOC_FSRARID, temp_buf) <= 0)
									wo_rarid_loc_list.add(loc_id);
							}
							if(wo_rarid_loc_list.getCount()) {
								wo_rarid_loc_list.sortAndUndup();
								uint j = by_kpp_loc_list.getCount();
								do {
									const  PPID loc_id = by_kpp_loc_list.at(--j).Val;
									if(!wo_rarid_loc_list.bsearch(loc_id))
										by_kpp_loc_list.atFree(j);
								} while(j);
							}
						}
						const LAssoc & r_pl = by_kpp_loc_list.at(0);
						THROW(PsnObj.GetPacket(r_pl.Key, pPack, 0) > 0);
						pack_extracted = 1;
						for(uint dlp = 0; pPack->EnumDlvrLoc(&dlp, &loc_pack) > 0;) {
							if(loc_pack.ID == r_pl.Val) {
								pPack->SelectedLocPos = dlp;
								break;
							}
						}
					}
					else if(by_kpp_psn_list.getCount()) {
						if(by_kpp_psn_list.getCount() > 1) {
							PPIDArray wo_rarid_psn_list;
							// @todo log Неоднозначность в разрешении персоналии по КПП
							// Прежде всего рассматриваем те персоналии, у которых не определен ФСРАР-ИД
							for(uint i = 0; i < by_kpp_psn_list.getCount(); i++) {
								const  PPID psn_id = by_kpp_psn_list.get(i);
								if(p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_FSRARID, temp_buf) <= 0)
									wo_rarid_psn_list.add(psn_id);
							}
							if(wo_rarid_psn_list.getCount())
								by_kpp_psn_list = wo_rarid_psn_list;
						}
						THROW(PsnObj.GetPacket(by_kpp_psn_list.get(0), pPack, 0) > 0);
						pack_extracted = 1;
					}
				}
				//
				// По поводу условия (manufType < 0):
				// Для производителей другой КПП - новая персоналия
				//
				if(!pack_extracted && by_inn_psn_list.getCount() && !(roleFlag & (EgaisPersonCore::rolefManuf|EgaisPersonCore::rolefImporter))) {
					if(by_inn_psn_list.getCount() > 1) {
						// @todo log Неоднозначность в разрешении персоналии по ИНН
						// Прежде всего рассматриваем те персоналии, у которых не определен ФСРАР-ИД
						PPIDArray wo_rarid_psn_list;
						for(uint i = 0; i < by_inn_psn_list.getCount(); i++) {
							const  PPID psn_id = by_inn_psn_list.get(i);
							if(p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_FSRARID, temp_buf) <= 0)
								wo_rarid_psn_list.add(psn_id);
						}
						if(wo_rarid_psn_list.getCount())
							by_inn_psn_list = wo_rarid_psn_list;
					}
					THROW(PsnObj.GetPacket(by_inn_psn_list.get(0), pPack, 0) > 0);
					pack_extracted = 1;
					if(pPack->Regs.GetListByType(PPREGT_KPP, ZERODATE, 0) > 0) {
						//
						// КПП у персоналии есть, по заданному КПП ни склад ни персоналию разрешить не удалось:
						//     создаем новый адрес доставки в найденной по ИНН персоналии
						//
						loc_pack.destroy();
						loc_pack.Type = LOCTYP_ADDRESS;
						PsnObj.LocObj.InitCode(&loc_pack);
						if(zip.NotEmptyS())
							LocationCore::SetExField(&loc_pack, LOCEXSTR_ZIP, zip);
						if(addr_descr.NotEmptyS()) {
							LocationCore::SetExField(&loc_pack, LOCEXSTR_FULLADDR, (temp_buf = addr_descr).Transf(CTRANSF_OUTER_TO_INNER));
							loc_pack.Flags |= LOCF_MANUALADDR;
						}
						{
							PPObjRegister::InitPacket(&new_reg_rec, PPREGT_KPP, PPObjID(PPOBJ_LOCATION, 0), kpp);
							loc_pack.Regs.insert(&new_reg_rec);
						}
						{
							const ObjTagItem * p_tag_item = loc_pack.TagL.GetItem(PPTAG_LOC_FSRARID);
							if(!p_tag_item || (p_tag_item->GetStr(temp_buf) <= 0 || temp_buf.IsEmpty())) {
								loc_pack.TagL.PutItemStr(PPTAG_LOC_FSRARID, rar_ident);
								rarid_assigned = 2;
							}
						}
						pPack->AddDlvrLoc(loc_pack);
					}
					else {
						PPObjRegister::InitPacket(&new_reg_rec, PPREGT_KPP, PPObjID(PPOBJ_PERSON, 0), kpp);
						pPack->Regs.insert(&new_reg_rec);
					}
				}
			}
			if(pack_extracted) {
				PPLocationPacket loc_pack;
				psn_id = pPack->Rec.ID;
				assert(psn_id != 0);
				if(personKindID)
					pPack->Kinds.addUnique(personKindID);
				AssignManufTypeToPersonPacket(*pPack, roleFlag);
				if(pPack->SelectedLocPos > 0 && pPack->GetDlvrLocByPos(pPack->SelectedLocPos-1, &loc_pack)) {
					const ObjTagItem * p_tag_item = loc_pack.TagL.GetItem(PPTAG_LOC_FSRARID);
					if(!p_tag_item || (p_tag_item->GetStr(temp_buf) <= 0 || temp_buf.IsEmpty())) {
						loc_pack.TagL.PutItemStr(PPTAG_LOC_FSRARID, rar_ident);
						rarid_assigned = 2;
					}
				}
			}
			else {
				pPack->Z();
				pPack->Rec.Status = PPPRS_LEGAL;
				pPack->Kinds.addUnique(personKindID);
				AssignManufTypeToPersonPacket(*pPack, roleFlag);
				temp_buf.Z();
				if(short_name.NotEmptyS())
					temp_buf = short_name;
				else if(full_name.NotEmptyS())
					temp_buf = full_name;
				temp_buf.Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(pPack->Rec.Name, temp_buf);
				if(inn.NotEmptyS()) {
					PPObjRegister::InitPacket(&new_reg_rec, PPREGT_TPID, PPObjID(PPOBJ_PERSON, 0), inn);
					pPack->Regs.insert(&new_reg_rec);
				}
				if(kpp.NotEmptyS()) {
					PPObjRegister::InitPacket(&new_reg_rec, PPREGT_KPP, PPObjID(PPOBJ_PERSON, 0), kpp);
					pPack->Regs.insert(&new_reg_rec);
				}
				{
					pPack->Loc.destroy();
					pPack->Loc.Type = LOCTYP_ADDRESS;
					PsnObj.LocObj.InitCode(&pPack->Loc);
					if(zip.NotEmptyS())
						LocationCore::SetExField(&pPack->Loc, LOCEXSTR_ZIP, zip);
					if(addr_descr.NotEmptyS()) {
						LocationCore::SetExField(&pPack->Loc, LOCEXSTR_FULLADDR, (temp_buf = addr_descr).Transf(CTRANSF_OUTER_TO_INNER));
						pPack->Loc.Flags |= LOCF_MANUALADDR;
					}
				}
			}
			if(!rarid_assigned) {
				//
				// Не заменяем уже установленный ФСРАР-ИД
				//
				const ObjTagItem * p_tag_item = pPack->TagL.GetItem(PPTAG_PERSON_FSRARID);
				if(!p_tag_item || (p_tag_item->GetStr(temp_buf) <= 0 || temp_buf.IsEmpty())) {
					pPack->TagL.PutItemStr(PPTAG_PERSON_FSRARID, rar_ident);
					rarid_assigned = 1;
				}
			}
			THROW(PsnObj.PutPacket(&psn_id, pPack, 1));
		}
	}
	CATCH
		LogLastError();
	ENDCATCH
	return ok;
}

int PPEgaisProcessor::Read_TicketResult(xmlNode * pFirstNode, int ticketType, PPEgaisProcessor::Ticket::Result & rResult)
{
	assert(oneof2(ticketType, 1, 2));
	int    ok = -1;
	SString temp_buf;
	rResult.Type = ticketType;
	for(const xmlNode * p_r = pFirstNode; p_r; p_r = p_r->next) {
		if(ticketType == 1) {
			if(SXml::GetContentByName(p_r, "Conclusion", temp_buf)) {
				rResult.Conclusion = temp_buf.IsEqiAscii("Accepted") ? 1 : (temp_buf.IsEqiAscii("Rejected") ? 0 : -1);
				if(rResult.Conclusion >= 0)
					ok = 1;
			}
			else if(SXml::GetContentByName(p_r, "ConclusionDate", temp_buf))
				strtodatetime(temp_buf,  &rResult.Time, DATF_ISO8601, TIMF_HMS);
			else if(SXml::GetContentByName(p_r, "Comments", temp_buf))
				rResult.Comment = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(ticketType == 2) {
			if(SXml::GetContentByName(p_r, "OperationResult", temp_buf)) {
				rResult.Conclusion = temp_buf.IsEqiAscii("Accepted") ? 1 : (temp_buf.IsEqiAscii("Rejected") ? 0 : -1);
				if(rResult.Conclusion >= 0)
					ok = 1;
			}
			else if(SXml::GetContentByName(p_r, "OperationName", temp_buf))
				rResult.OpName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_r, "OperationDate", temp_buf))
				strtodatetime(temp_buf,  &rResult.Time, DATF_ISO8601, TIMF_HMS);
			else if(SXml::GetContentByName(p_r, "OperationComment", temp_buf))
				rResult.Comment = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
	}
	if(rResult.Conclusion == 0) {
		//<tc:Comments>Зафиксирована попытка подачи недостоверных данных. Накладная с номером "29622оа" от 25.10.2017 00:00:00 уже зарегистрирована в системе. Регистрационный номер TTN-0154151096, дата регистрации 25.10.2017 08:31:04. Владелец ["010060693450"], накладная "5978692".
		PPLoadText(PPTXT_EGAIS_TKT_UNREL_ATTEMPT, temp_buf); // Зафиксирована попытка подачи недостоверных данных
		if(rResult.Comment.Search(temp_buf, 0, 1, 0)) {
			PPLoadText(PPTXT_EGAIS_TKT_ALRREGISTERED, temp_buf); // уже зарегистрирована в системе
			if(rResult.Comment.Search(temp_buf, 0, 1, 0))
				rResult.Special = rResult.spcDup;
		}
	}
	return ok;
}

int PPEgaisProcessor::Read_Ticket(xmlNode * pFirstNode, Packet * pPack)
{
	int    ok = 1;
	SString temp_buf;
	if(pPack && pPack->P_Data) {
		Ticket * p_ticket = static_cast<Ticket *>(pPack->P_Data);
		for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "Identity"))
				;
			else if(SXml::GetContentByName(p_n, "TicketDate", temp_buf))
				strtodatetime(temp_buf,  &p_ticket->TicketTime, DATF_ISO8601, TIMF_HMS);
			else if(SXml::GetContentByName(p_n, "DocId", temp_buf))
				p_ticket->DocUUID.FromStr(temp_buf);
			else if(SXml::GetContentByName(p_n, "TransportId", temp_buf))
				p_ticket->TranspUUID.FromStr(temp_buf);
			else if(SXml::GetContentByName(p_n, "RegID", temp_buf))
				p_ticket->RegIdent = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			else if(SXml::GetContentByName(p_n, "DocType", temp_buf))
				p_ticket->DocType = PPEgaisProcessor::RecognizeDocTypeTag(temp_buf);
			else if(SXml::IsName(p_n, "Result"))
				Read_TicketResult(p_n->children, 1, p_ticket->R);
			else if(SXml::IsName(p_n, "OperationResult"))
				Read_TicketResult(p_n->children, 2, p_ticket->OpR);
		}
	}
	//CATCHZOK
	return ok;
}

int PPEgaisProcessor::Read_IformA(xmlNode * pFirstNode, Packet * pPack, PrcssrAlcReport::RefCollection * pRefC)
{
    int    ok = 1;
	int    shipper_refc_pos = -1;
	int    consignee_refc_pos = -1;
	PPPersonPacket psn_shipper;
	PPPersonPacket psn_consignee;
	PPPersonPacket psn_org_client; // @v11.3.4
    SString temp_buf;
    EgaisRefATbl::Rec * p_data = static_cast<EgaisRefATbl::Rec *>(pPack->P_Data);
    for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "InformARegId", temp_buf) || SXml::GetContentByName(p_n, "InformF1RegId", temp_buf)) {
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			STRNSCPY(p_data->RefACode, temp_buf);
		}
		else if(SXml::GetContentByName(p_n, "BottlingDate", temp_buf))
			strtodate(temp_buf, DATF_ISO8601, &p_data->BottlingDate);
		else if(SXml::GetContentByName(p_n, "TTNNumber", temp_buf)) {
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			STRNSCPY(p_data->TTNNumber, temp_buf);
		}
		else if(SXml::GetContentByName(p_n, "TTNDate", temp_buf)) {
			strtodate(temp_buf, DATF_ISO8601, &p_data->TTNDate);
		}
		else if(SXml::GetContentByName(p_n, "EGAISNumber", temp_buf)) {
			temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			STRNSCPY(p_data->EgaisNumber, temp_buf);
		}
		else if(SXml::GetContentByName(p_n, "EGAISDate", temp_buf)) {
			strtodate(temp_buf, DATF_ISO8601, &p_data->ActualDate);
			strtodate(temp_buf, DATF_ISO8601, &p_data->EgaisDate); // дубликат
		}
		else if(SXml::IsName(p_n, "Shipper")) {
			Read_OrgInfo(p_n->children, PPPRK_MANUF, EgaisPersonCore::rolefManuf, pPack ? &psn_shipper : 0, pRefC, 0);
			if(pRefC && pRefC->LastPersonP >= 0) {
				const EgaisPersonCore::Item * p_person = pRefC->PersonList.at(pRefC->LastPersonP);
				if(p_person)
					STRNSCPY(p_data->ShipperRarIdent, p_person->RarIdent);
			}
		}
		else if(SXml::IsName(p_n, "Consignee")) {
			Read_OrgInfo(p_n->children, PPPRK_MANUF, EgaisPersonCore::rolefManuf, pPack ? &psn_consignee : 0, pRefC, 0);
			if(pRefC && pRefC->LastPersonP >= 0) {
				const EgaisPersonCore::Item * p_person = pRefC->PersonList.at(pRefC->LastPersonP);
				if(p_person)
					STRNSCPY(p_data->ConsigneeRarIdent, p_person->RarIdent);
			}
		}
		else if(SXml::IsName(p_n, "OriginalClient")) { // @v11.3.4
			Read_OrgInfo(p_n->children, PPPRK_MANUF, EgaisPersonCore::rolefManuf, pPack ? &psn_org_client : 0, pRefC, 0);
			if(pRefC && pRefC->LastPersonP >= 0) {
				const EgaisPersonCore::Item * p_person = pRefC->PersonList.at(pRefC->LastPersonP);
				if(p_person)
					STRNSCPY(p_data->ImporterRarIdent, p_person->RarIdent);
			}
		}
		else if(SXml::GetContentByName(p_n, "OriginalDocNumber", temp_buf)) { // @v11.3.4
		}
		else if(SXml::GetContentByName(p_n, "OriginalDocDate", temp_buf)) { // @v11.3.4
		}
		else if(SXml::GetContentByName(p_n, "GTDNUMBER", temp_buf)) { // @v11.3.4
		}
		else if(SXml::GetContentByName(p_n, "GTDDate", temp_buf)) { // @v11.3.4
		}
		else if(SXml::IsName(p_n, "Product")) {
			PrcssrAlcReport::GoodsItem agi;
			Read_ProductInfo(p_n->children, 0, &agi, pRefC, 0);
			STRNSCPY(p_data->AlcCode, agi.EgaisCode);
			STRNSCPY(p_data->ManufRarIdent, agi.RefcManufCode);
			// @v11.3.4 STRNSCPY(p_data->ImporterRarIdent, agi.RefcImporterCode);
			p_data->Volume = static_cast<long>(agi.Volume * 100000);
			p_data->CountryCode = agi.CountryCode;
			p_data->Flags |= EgaisRefACore::fVerified;
		}
    }
    if(pRefC) {
		pRefC->SetRefA(*p_data);
    }
    return ok;
}

int PPEgaisProcessor::Read_TTNIformBReg(xmlNode * pFirstNode, Packet * pPack)
{
    int    ok = 1;
    SString temp_buf;
    SString bill_ident;
    InformB * p_data = static_cast<InformB *>(pPack->P_Data);
    for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "Header")) {
            for(const xmlNode * p_h = p_n->children; p_h; p_h = p_h->next) {
				/*
            	if(SXml::GetContentByName(p_h, "Identity", temp_buf))
					p_data->Id = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				*/
            	if(SXml::GetContentByName(p_h, "Identity", bill_ident)) {
					bill_ident.Transf(CTRANSF_UTF8_TO_INNER);
            	}
            	else if(SXml::GetContentByName(p_h, "WBRegId", temp_buf))
            		p_data->WBRegId = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
            	else if(SXml::GetContentByName(p_h, "EGAISFixNumber", temp_buf))
            		p_data->FixNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
            	else if(SXml::GetContentByName(p_h, "EGAISFixDate", temp_buf))
            		strtodate(temp_buf, DATF_ISO8601, &p_data->FixDate);
            	else if(SXml::GetContentByName(p_h, "WBNUMBER", temp_buf))
					p_data->OuterCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
            	else if(SXml::GetContentByName(p_h, "WBDate", temp_buf))
					strtodate(temp_buf, DATF_ISO8601, &p_data->OuterDate);
				/*
            	else if(SXml::IsName(p_h, "Shipper"))
					;
            	else if(SXml::IsName(p_h, "Consignee"))
					;
            	else if(SXml::IsName(p_h, "Supplier"))
					;
				*/
            }
        }
		else {
			if(SXml::IsName(p_n, "Content")) {
				int    surrogate_line_ident = PPConst::EgaisInRowIdentDivider;
				for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
                    InformBItem item;
					if(SXml::IsName(p_c, "Position")) {
						for(const xmlNode * p_pos = p_c->children; p_pos; p_pos = p_pos->next) {
							if(SXml::GetContentByName(p_pos, "Identity", temp_buf)) {
								STRNSCPY(item.OrgRowIdent, temp_buf);
								if(temp_buf.IsDec()) {
									if(temp_buf == "0")
										item.P = PPConst::EgaisInRowIdentDivider;
									else
										item.P = (temp_buf.ToLong() % PPConst::EgaisInRowIdentDivider);
								}
								else {
									item.P = ++surrogate_line_ident;
								}
							}
							else if(SXml::GetContentByName(p_pos, "InformBRegId", temp_buf))
                                STRNSCPY(item.Ident, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
							else if(SXml::GetContentByName(p_pos, "InformF2RegId", temp_buf)) // PPEDIOP_EGAIS_TTNINFORMF2REG
								STRNSCPY(item.Ident, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
							else if(SXml::GetContentByName(p_pos, "BottlingDate", temp_buf))
								strtodate(temp_buf, DATF_ISO8601, &item.BottlingDate);
						}
						p_data->Items.insert(&item);
					}
				}
			}
        }
    }
    if(!bill_ident.NotEmptyS())
		bill_ident = p_data->OuterCode;
	p_data->Id = bill_ident;
    return ok;
}

struct WayBillActRecadvItem { // @flat
	WayBillActRecadvItem() : P(0), RealQtty(0.0)
	{
		PTR32(InformBIdent)[0] = 0;
	}
    long   P;
    char   InformBIdent[24];
    double RealQtty;
};

int PPEgaisProcessor::Read_WayBillAct(xmlNode * pFirstNode, PPID locID, Packet * pPack)
{
    int    ok = 1;
    int    was_header_accepted = 0;
    int    is_pack_inited = 0;
    int    wb_type = 0;
    int    is_accepted = -1;
    LDATE  act_date = ZERODATE;
    SString act_code;
    PPBillPacket * p_bp = static_cast<PPBillPacket *>(pPack->P_Data);
    SString bill_ident;
	SString bill_text;
    SString memo_note; // <note>
    SString temp_buf;
    PPObjOprKind op_obj;
    BillTbl::Rec bhdr;
    TSVector <WayBillActRecadvItem> items;
    for(const xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "Header")) {
			was_header_accepted = 1;
            for(const xmlNode * p_h = p_n->children; p_h; p_h = p_h->next) {
				if(SXml::GetContentByName(p_h, "IsAccept", temp_buf)) {
					if(temp_buf.IsEqiAscii("Accepted"))
						is_accepted = 1;
					else if(temp_buf.IsEqiAscii("Rejected"))
						is_accepted = 0;
				}
                else if(SXml::GetContentByName(p_h, "ACTNUMBER", temp_buf))
					(act_code = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
                else if(SXml::GetContentByName(p_h, "ActDate", temp_buf))
                    strtodate(temp_buf, DATF_ISO8601, &act_date);
                else if(SXml::GetContentByName(p_h, "WBRegId", temp_buf))
					(bill_ident = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
                else if(SXml::GetContentByName(p_h, "Note", temp_buf))
					(memo_note = temp_buf).Utf8ToChar();
            }
        }
		else if(SXml::IsName(p_n, "Content")) {
			for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
				WayBillActRecadvItem item;
				if(SXml::IsName(p_c, "Position")) {
					for(const xmlNode * p_pos = p_c->children; p_pos; p_pos = p_pos->next) {
						if(SXml::GetContentByName(p_pos, "Identity", temp_buf))
							item.P = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_pos, "InformBRegId", temp_buf))
							temp_buf.Transf(CTRANSF_UTF8_TO_INNER).CopyTo(item.InformBIdent, sizeof(item.InformBIdent));
						else if(SXml::GetContentByName(p_pos, "RealQuantity", temp_buf))
							item.RealQtty = temp_buf.ToReal();
					}
					THROW_SL(items.insert(&item));
				}
			}
        }
    }
	if(ok < 0) {
		PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
		LogTextWithAddendum(PPTXT_EGAIS_BILLREJECTEDUNR, bill_text);
	}
	else {
		int    do_skip = 0;
		if(bill_ident.NotEmptyS()) {
			PPIDArray ex_bill_id_list;
			PPRef->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, bill_ident, &ex_bill_id_list);
			for(uint i = 0; !do_skip && i < ex_bill_id_list.getCount(); i++) {
				const  PPID ex_bill_id = ex_bill_id_list.get(i);
				BillTbl::Rec ex_bill_rec;
				if(P_BObj->Fetch(ex_bill_id, &ex_bill_rec) > 0) {
                    PPBillPacket bp;
					PPBillPacket link_pack;
                    int    do_turn_link_pack = 0;
                    int    recadv_status = PPEDI_RECADV_STATUS_UNDEF;
					THROW(P_BObj->ExtractPacket(ex_bill_id, &bp) > 0);
                    if(is_accepted != 0) {
						if(items.getCount() == 0 && is_accepted > 0)
							recadv_status = PPEDI_RECADV_STATUS_ACCEPT;
						else {
                            PPID   op_id = 0;
                            THROW(op_obj.GetEdiRecadvOp(&op_id, 1));
							{
								int    edirecadv_found = 0;
                                BillTbl::Rec link_rec;
								for(DateIter di; !do_skip && P_BObj->P_Tbl->EnumLinks(ex_bill_id, &di, BLNK_EDIRECADV, &link_rec) > 0;) {
									do_skip = 1;
									edirecadv_found = 1;
								}
								recadv_status = PPEDI_RECADV_STATUS_ACCEPT;
								if(edirecadv_found) {
									for(uint k = 0; k < bp.GetTCount(); k++) {
										const PPTransferItem & r_ti = bp.ConstTI(k);
										uint item_idx = 0;
										if(items.lsearch(&r_ti.RByBill, &item_idx, CMPF_LONG)) {
											const double real_qtty = fabs(items.at(item_idx).RealQtty);
											if(!feqeps(real_qtty, fabs(r_ti.Quantity_), 1e-06))
												recadv_status = PPEDI_RECADV_STATUS_PARTACCEPT;
										}
									}
								}
								else if(!do_skip) {
									THROW(link_pack.CreateBlank_WithoutCode(op_id, ex_bill_id, bp.Rec.LocID, 1));
									STRNSCPY(link_pack.Rec.Code, act_code);
									link_pack.Rec.Dt = ValidDateOr(act_date, getcurdate_());
									link_pack.Rec.EdiOp = PPEDIOP_RECADV;
									for(uint k = 0; k < bp.GetTCount(); k++) {
										const PPTransferItem & r_ti = bp.ConstTI(k);
										PPTransferItem ti;
										uint item_idx = 0;
										THROW(ti.Init(&link_pack.Rec));
										ti.GoodsID = r_ti.GoodsID;
										if(items.lsearch(&r_ti.RByBill, &item_idx, CMPF_LONG)) {
											const double real_qtty = fabs(items.at(item_idx).RealQtty);
											ti.Quantity_ = real_qtty;
											if(!feqeps(real_qtty, fabs(r_ti.Quantity_), 1e-06))
												recadv_status = PPEDI_RECADV_STATUS_PARTACCEPT;
										}
										else
											ti.Quantity_ = r_ti.Quantity_;
										{
											GoodsItem _agi;
											PreprocessGoodsItem(ti.GoodsID, 0, 0, 0, _agi);
											if(_agi.UnpackedVolume > 0.0) {
												const double mult = _agi.UnpackedVolume / 10.0;
												ti.Quantity_ = (ti.Quantity_ / mult); // Неупакованная продукция передается в декалитрах
											}
										}
										ti.Cost = r_ti.Cost;
										ti.Price = r_ti.NetPrice();
										THROW(link_pack.LoadTItem(&ti, 0, 0));
									}
									do_turn_link_pack = 1;
								}
                            }
						}
                    }
					else if(is_accepted == 0)
						recadv_status = PPEDI_RECADV_STATUS_REJECT;
					{
						const int srsr = BillCore::SetRecadvStatus(recadv_status, bp.Rec);
						if(do_turn_link_pack || srsr > 0) {
							PPTransaction tra(1);
							THROW(tra);
							if(do_turn_link_pack) {
								link_pack.InitAmounts();
								THROW(P_BObj->TurnPacket(&link_pack, 0));
							}
							if(srsr > 0) {
								THROW(SearchByID_ForUpdate(P_BObj->P_Tbl, PPOBJ_BILL, bp.Rec.ID, &ex_bill_rec) > 0);
								ex_bill_rec.Flags2 = bp.Rec.Flags2;
								THROW_DB(P_BObj->P_Tbl->updateRecBuf(&ex_bill_rec)); // @sfu
							}
							THROW(tra.Commit());
							{
								PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
								LogTextWithAddendum(PPTXT_EDI_BILLRECADVACCEPTED, bill_text);
							}
						}
					}
					break;
				}
			}
		}
		pPack->Flags |= Packet::fDoDelete;
	}
	CATCHZOK
    return ok;
}

struct EgaisWayBillRowTags { // @flat
	int     RByBill;
	char    GoodsCode[24];
	char    InformA[24];
	char    InformB[24];
	char    Serial[24];
	char    OrgLineIdent[64]; // 
	char    GoodsCategory[8]; // @v11.3.5 Код категории алкогольной продукции
	PPID    ManufID;
};

struct ExtCodeSetEntry {
	ExtCodeSetEntry() : RByBill(0)
	{
	}
	int    RByBill;
	PPLotExtCodeContainer::MarkSet Set;
};

int PPEgaisProcessor::Read_WayBill(xmlNode * pFirstNode, PPID locID, const DateRange * pPeriod, Packet * pPack, PrcssrAlcReport::RefCollection * pRefC)
{
	//
	// Descr: Причины отказа от приема документа
	//
	enum RejectReason {
		rrNone = 0,
		rrUnresolvedGoods,    // В документе есть неразрешенные товары
		rrDateOutOfRange,     // Дата документа выходит за пределы заданного периода
		rrIntrBillNotFound,   // Не найден соответствующий документ внутреннего перемещения
		rrIntrBillIdUnrecogn, // Принятый документ имеет примечание с префиксом внутреннего перемещения, но не удалось распознать идентификатор
		rrRByBillDup          // Дублирование идентификаторов строк в документе
	};
    int    ok = 1;
    RejectReason rr = rrNone;
    int    was_header_accepted = 0;
    int    is_pack_inited = 0;
    int    wb_type = 0;
    int    unpacked = 0; // Признак того, что документ содержит неупакованную продукцию (количество в декалитрах)
    PPID   intr_expend_bill_id = 0; // Идент документа внутренней передачи, дубликат которого мы получили по ЕГАИС (установлен в поле Note)
	// (Замещено на pPack->IntrBillID) PPID   local_intr_bill_id = 0; // Локальный для нашей БД идентификатор документа внутренней передачи, соответствующий intr_expend_bill_id
    const  PPID loc_id = NZOR(locID, LConfig.Location);
    PPPersonPacket psn_shipper;
    PPPersonPacket psn_suppl;
    PPPersonPacket psn_consignee;
	PPLocationPacket loc_pack;
	PPBillPacket * p_bp = pPack ? static_cast<PPBillPacket *>(pPack->P_Data) : 0;
    PPFreight freight;
    SString bill_ident;
    SString memo_base; // <base>
    SString memo_note; // <note>
    SString temp_buf;
	SString bill_code;
	LDATE  bill_date = ZERODATE;
	TSVector <EgaisWayBillRowTags> row_tags;
	const  PPID manuf_tag_id = Cfg.LotManufTagList.getCount() ? Cfg.LotManufTagList.get(0) : 0;
	TSCollection <ExtCodeSetEntry> ext_code_set_list;
	if(pPack)
		pPack->Flags |= Packet::fFaultObj; // Иницилизируем флаг. Когда убедимся, что документ OK, флаг снимим.
    for(const xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
        if(SXml::GetContentByName(p_n, "Identity", temp_buf)) {
			bill_ident = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::IsName(p_n, "Header")) {
			was_header_accepted = 1;
            for(const xmlNode * p_h = p_n->children; ok > 0 && p_h; p_h = p_h->next) {
                if(SXml::GetContentByName(p_h, "NUMBER", temp_buf)) {
					bill_code = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
                }
                else if(SXml::GetContentByName(p_h, "Date", temp_buf))
                    strtodate(temp_buf, DATF_ISO8601, &bill_date);
                else if(SXml::GetContentByName(p_h, "ShippingDate", temp_buf)) {
					LDATE shp_dt;
					strtodate(temp_buf, DATF_ISO8601, &shp_dt);
					if(checkdate(shp_dt))
						freight.IssueDate = shp_dt;
                }
                else if(SXml::GetContentByName(p_h, "Type", temp_buf))
					wb_type = RecognizeWayBillTypeText(temp_buf.Utf8ToChar());
                else if(SXml::GetContentByName(p_h, "UnitType", temp_buf)) {
					if(temp_buf.IsEqiAscii("Unpacked"))
						unpacked = 1;
                }
				else if(SXml::IsName(p_h, "Shipper")) {
					// pRefC-->0: Считанные из накладной данные о персоналии не вносим во внутреннюю БД (они могут быть не достоверны)
					Read_OrgInfo(p_h->children, PPPRK_SUPPL, EgaisPersonCore::rolefShipper, p_bp ? &psn_shipper : 0, /*pRefC*/0, 0);
				}
                else if(SXml::IsName(p_h, "Consignee")) {
                	PPID   psn_kind = 0;
					if(oneof2(wb_type, wbtInvcFromMe, wbtRetFromMe)) // От поставщика ко мне
						psn_kind = PPPRK_MAIN;
					else
						psn_kind = PPPRK_SUPPL; // Технически, мы здесь не должны оказаться ибо импортируются только документы ToMe.
					// @todo В этом блоке необходимо правильно идентифицировать склад, на который должен попасть документ
					// pRefC-->0: Считанные из накладной данные о персоналии не вносим во внутреннюю БД (они могут быть не достоверны)
					Read_OrgInfo(p_h->children, psn_kind, EgaisPersonCore::rolefConsignee, p_bp ? &psn_consignee : 0, /*pRefC*/0, 0);
                }
				else if(SXml::IsName(p_h, "Supplier")) {
					// pRefC-->0: Считанные из накладной данные о персоналии не вносим во внутреннюю БД (они могут быть не достоверны)
					Read_OrgInfo(p_h->children, PPPRK_SUPPL, EgaisPersonCore::rolefSupplier, p_bp ? &psn_suppl : 0, /*pRefC*/0, 0);
				}
                else if(SXml::IsName(p_h, "Transport"))
					;
                else if(SXml::GetContentByName(p_h, "Base", memo_base))
					memo_base.Transf(CTRANSF_UTF8_TO_INNER);
                else if(SXml::GetContentByName(p_h, "Note", memo_note)) {
                	memo_note.Transf(CTRANSF_UTF8_TO_INNER);
                	if(memo_note.HasPrefixNC(PPConst::P_BillNotePrefix_IntrExpnd)) {
						(temp_buf = memo_note).ShiftLeft(sstrlen(PPConst::P_BillNotePrefix_IntrExpnd)).Strip().ShiftLeftChr('=').Strip();
						intr_expend_bill_id = temp_buf.ToLong();
						SETIFZ(intr_expend_bill_id, -1);
                	}
                }
            }
        }
		else if(was_header_accepted && !is_pack_inited) {
			PPID   ar_id = 0;
			PPID   dlvr_loc_id = 0;
			PPOprKind op_rec;
			PPBillPacket::SetupObjectBlock sob;
			if(!pPack) {
				; // @skip next block
			}
			else if(intr_expend_bill_id > 0) {
				BillTbl::Rec intr_bill_rec;
				if(P_BObj->Fetch(intr_expend_bill_id, &intr_bill_rec) > 0 && IsIntrExpndOp(intr_bill_rec.OpID)) {
					if(Egais_GetBillCode(intr_bill_rec, temp_buf) == bill_code)
						pPack->IntrBillID = intr_expend_bill_id;
				}
				if(!pPack->IntrBillID) {
					ObjSyncCore * p_osc = DS.GetTLA().P_ObjSync;
					PPIDArray private_id_list;
					if(p_osc && p_osc->GetPrivateObjectsByForeignID(PPOBJ_BILL, intr_expend_bill_id, &private_id_list) > 0) {
						for(uint i = 0; !pPack->IntrBillID && i < private_id_list.getCount(); i++) {
							const  PPID test_bill_id = private_id_list.get(i);
							if(P_BObj->Fetch(test_bill_id, &intr_bill_rec) > 0) {
								const int intr = IsIntrOp(intr_bill_rec.OpID);
								if(oneof2(intr, INTREXPND, INTRRCPT)) {
									if(Egais_GetBillCode(intr_bill_rec, temp_buf) == bill_code)
										pPack->IntrBillID = test_bill_id;
								}
							}
						}
					}
				}
				if(pPack->IntrBillID > 0) {
					//PPTXT_EGAIS_INTRBILLFOUND
				}
				else {
					pPack->IntrBillID = -1;
					rr = rrIntrBillNotFound;
					ok = -1;
				}
			}
			else if(intr_expend_bill_id < 0) {
				pPack->IntrBillID = -1;
				rr = rrIntrBillIdUnrecogn;
				ok = -1;
			}
			if(wb_type == wbtInvcFromMe) { // От поставщика ко мне
				if(p_bp) {
					const  PPID op_id = ACfg.Hdr.EgaisRcptOpID;
					THROW_PP(op_id, PPERR_EGAIS_RCPTOPUNDEF);
					GetOpData(op_id, &op_rec);
					THROW(p_bp->CreateBlank_WithoutCode(op_id, 0, loc_id, 1));
					p_bp->Rec.Dt = bill_date;
					STRNSCPY(p_bp->Rec.Code, bill_code);
					if(bill_code.Len() > (sizeof(p_bp->Rec.Code)-1)) {
						p_bp->BTagL.PutItemStr(PPTAG_BILL_LONGCODE, bill_code);
					}
					if(memo_note.NotEmptyS()) {
						p_bp->SMemo = memo_note;
					}
					else if(memo_base.NotEmptyS()) {
						p_bp->SMemo = memo_base;
					}
					//
					// Приоритет применения контрагента в качестве поставщика:
					// ранее приоритет был у psn_suppl, однако выяснилось что некоторые поставщики
					// гонят в этом теге бредовые значения (типа собственного поставщика).
					//
					if(psn_shipper.Rec.ID)
						ArObj.P_Tbl->PersonToArticle(psn_shipper.Rec.ID, op_rec.AccSheetID, &ar_id);
					else if(psn_suppl.Rec.ID)
						ArObj.P_Tbl->PersonToArticle(psn_suppl.Rec.ID, op_rec.AccSheetID, &ar_id);
					if(ar_id)
						p_bp->SetupObject(ar_id, sob);
					if(P_UtmEntry && P_UtmEntry->MainOrgID && AcsObj.IsLinkedToMainOrg(op_rec.AccSheet2ID)) {
						PPID   ar2_id = 0;
						ArObj.P_Tbl->PersonToArticle(P_UtmEntry->MainOrgID, op_rec.AccSheet2ID, &ar2_id);
						p_bp->SetupObject2(ar2_id);
					}
					is_pack_inited = 1;
				}
			}
			else if(wb_type == wbtRetFromMe) { // Возврат от покупателя ко мне
				if(p_bp) {
					const  PPID op_id = ACfg.Hdr.EgaisRetOpID;
					THROW_PP(op_id, PPERR_EGAIS_RETOPUNDEF);
					GetOpData(op_id, &op_rec);
					THROW(p_bp->CreateBlank_WithoutCode(op_id, 0, loc_id, 1));
					p_bp->Rec.Dt = bill_date;
					STRNSCPY(p_bp->Rec.Code, bill_code);
					if(bill_code.Len() > (sizeof(p_bp->Rec.Code)-1)) {
						p_bp->BTagL.PutItemStr(PPTAG_BILL_LONGCODE, bill_code);
					}
					if(memo_note.NotEmptyS()) {
						p_bp->SMemo = memo_note;
					}
					else if(memo_base.NotEmptyS()) {
						p_bp->SMemo = memo_base;
					}
					//
					// @v9.2.10 Изменен приоритет идентификации контрагента: сначала по грузоотправителю, потом - по поставщику,
					// поскольку некоторые покупатели отгружают возврат с указанием получателя как поставщика
					//
					if(psn_shipper.Rec.ID) {
						ArObj.P_Tbl->PersonToArticle(psn_shipper.Rec.ID, op_rec.AccSheetID, &ar_id);
						if(ar_id && psn_shipper.SelectedLocPos > 0) {
							if(psn_shipper.GetDlvrLocByPos(psn_shipper.SelectedLocPos-1, &loc_pack) > 0)
								dlvr_loc_id = loc_pack.ID;
						}
					}
					else if(psn_suppl.Rec.ID) {
						ArObj.P_Tbl->PersonToArticle(psn_suppl.Rec.ID, op_rec.AccSheetID, &ar_id);
						if(ar_id && psn_suppl.SelectedLocPos > 0) {
							if(psn_suppl.GetDlvrLocByPos(psn_suppl.SelectedLocPos-1, &loc_pack) > 0)
								dlvr_loc_id = loc_pack.ID;
						}
					}
					if(ar_id) {
						p_bp->SetupObject(ar_id, sob);
						freight.SetupDlvrAddr(dlvr_loc_id);
					}
					if(P_UtmEntry && P_UtmEntry->MainOrgID && AcsObj.IsLinkedToMainOrg(op_rec.AccSheet2ID)) {
						PPID   ar2_id = 0;
						ArObj.P_Tbl->PersonToArticle(P_UtmEntry->MainOrgID, op_rec.AccSheet2ID, &ar2_id);
						p_bp->SetupObject2(ar2_id);
					}
					is_pack_inited = 1;
				}
			}
		}
		if(SXml::IsName(p_n, "Content")) {
			if(p_bp && pPeriod && !pPeriod->CheckDate(p_bp->Rec.Dt)) {
				rr = rrDateOutOfRange;
				ok = -1;
			}
			else {
				SString serial;
				SString box_number;
				int    surrogate_line_ident = PPConst::EgaisInRowIdentDivider;
				SString org_line_ident;
				for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
					PPTransferItem ti;
					serial.Z();
					if(p_bp) {
						THROW(ti.Init(&p_bp->Rec, 0, 0));
					}
					if(SXml::IsName(p_c, "Position")) {
						GoodsItem alc_ext;
						int    product_refc_pos = -1;
						int    local_unpacket_tag = 0;
						double _src_qtty = 0.0;
						double _src_cost = 0.0;
						ExtCodeSetEntry * p_ecs_entry = 0;
						org_line_ident.Z();
						for(const xmlNode * p_pos = p_c->children; p_pos; p_pos = p_pos->next) {
							if(SXml::GetContentByName(p_pos, "Identity", temp_buf)) {
								if(temp_buf.IsDec()) {
									const long dec_id = temp_buf.ToLong();
									if(temp_buf == "0") {
										ti.RByBill = PPConst::EgaisInRowIdentDivider;
										org_line_ident = temp_buf;
									}
									else {
										ti.RByBill = static_cast<int16>(dec_id % PPConst::EgaisInRowIdentDivider);
										if(dec_id != ti.RByBill)
											org_line_ident = temp_buf;
									}
								}
								else {
									ti.RByBill = ++surrogate_line_ident;
									org_line_ident = temp_buf;
								}
							}
							else if(SXml::IsName(p_pos, "Product")) {
								int   rs = 0;
								PPGoodsPacket goods_pack;
								// pRefC-->0: Считанные из накладной данные о товаре не вносим во внутреннюю БД (они могут быть не достоверны)
								THROW(rs = Read_ProductInfo(p_pos->children, p_bp ? &goods_pack : 0, &alc_ext, /*pRefC*/0, 0));
								if(alc_ext.OuterUnpackedTag)
									local_unpacket_tag = 1;
								if(pRefC && pRefC->LastProductP > 0) {
                                    product_refc_pos = pRefC->LastProductP;
								}
								if(p_bp) {
									if(rs > 0) {
										THROW(ti.SetupGoods(goods_pack.Rec.ID, 0));
									}
									else {
										rr = rrUnresolvedGoods;
										ok = -1; // Отказ акцептировать товар воспринимаем как директиву к выходу из процесса чтения документа
									}
								}
							}
							else if(SXml::GetContentByName(p_pos, "Quantity", temp_buf))
								_src_qtty = fabs(temp_buf.ToReal());
							else if(SXml::GetContentByName(p_pos, "Price", temp_buf))
								_src_cost = fabs(temp_buf.ToReal());
							else if(SXml::IsName(p_pos, "Pack_ID")) {
							}
							else if(SXml::GetContentByName(p_pos, "Party", serial))
								serial.Strip().Transf(CTRANSF_UTF8_TO_INNER);
							else if(SXml::GetContentByName(p_pos, "FARegId", temp_buf))
								alc_ext.InformA = temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
							else if(SXml::IsName(p_pos, "InformA") || SXml::IsName(p_pos, "InformF1")) {
								for(const xmlNode * p_inf = p_pos->children; p_inf; p_inf = p_inf->next) {
									if(SXml::GetContentByName(p_inf, "RegId", alc_ext.InformA)) {
										alc_ext.InformA.Strip().Transf(CTRANSF_UTF8_TO_INNER);
										break;
									}
								}
							}
							else if(SXml::IsName(p_pos, "InformB") || SXml::IsName(p_pos, "InformF2")) {
								for(const xmlNode * p_inf = p_pos->children; p_inf; p_inf = p_inf->next) {
									if(SXml::GetContentByName(p_inf, "F2RegId", temp_buf))
										alc_ext.InformB = temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER);
									else if(SXml::IsName(p_inf, "MarkInfo")) {
										THROW_MEM(SETIFZ(p_ecs_entry, ext_code_set_list.CreateNewItem()));
										for(const xmlNode * p_boxpos = p_inf->children; p_boxpos; p_boxpos = p_boxpos->next) {
											if(SXml::IsName(p_boxpos, "boxpos")) {
												box_number.Z();
												long   box_id = 0;
												for(const xmlNode * p_box = p_boxpos->children; p_box; p_box = p_box->next) {
													if(SXml::GetContentByName(p_box, "boxnumber", temp_buf)) {
														if(temp_buf.NotEmpty())
															box_id = p_ecs_entry->Set.AddBox(0, temp_buf, 0);
													}
													else if(SXml::IsName(p_box, "amclist")) {
														for(const xmlNode * p_amc = p_box->children; p_amc; p_amc = p_amc->next) {
															if(SXml::GetContentByName(p_amc, "amc", temp_buf) > 0)
																p_ecs_entry->Set.AddNum(box_id, temp_buf, 0);
														}
													}
												}
											}
										}
									}
									else if(SXml::IsName(p_inf, "InformBItem") || SXml::IsName(p_inf, "InformF2Item")) {
										for(const xmlNode * p_inf_item = p_inf->children; p_inf_item; p_inf_item = p_inf_item->next) {
											if(SXml::GetContentByName(p_inf_item, "BRegId", alc_ext.InformB) || SXml::GetContentByName(p_inf_item, "F2RegId", alc_ext.InformB))
												alc_ext.InformB.Strip().Transf(CTRANSF_UTF8_TO_INNER);
										}
									}
								}
							}
						}
						if(pRefC) {
							EgaisRefATbl::Rec refai;
							STRNSCPY(refai.RefACode, alc_ext.InformA);
							if(product_refc_pos >= 0) {
								const EgaisProductCore::Item * p_product = pRefC->ProductList.at(product_refc_pos);
								if(p_product) {
									STRNSCPY(refai.AlcCode, p_product->AlcoCode);
									STRNSCPY(refai.ManufRarIdent, p_product->ManufRarIdent);
									STRNSCPY(refai.ImporterRarIdent, p_product->ImporterRarIdent);
									refai.Volume = static_cast<int32>(p_product->Volume * 100000.0);
									THROW(pRefC->SetRefA(refai));
								}
							}
						}
						if(is_pack_inited && ti.GoodsID && _src_qtty != 0.0) {
							if(unpacked || local_unpacket_tag) {
								GoodsItem _agi;
								PreprocessGoodsItem(ti.GoodsID, 0, 0, 0, _agi);
								if(_agi.UnpackedVolume > 0.0) {
									const double mult = _agi.UnpackedVolume / 10.0;
									ti.Quantity_ = R6(_src_qtty / mult);
									ti.Cost = R5(_src_cost * mult);
								}
								else {
									ti.Quantity_ = _src_qtty;
									ti.Cost = _src_cost;
								}
							}
							else {
								ti.Quantity_ = _src_qtty;
								ti.Cost = _src_cost;
							}
							uint   new_pos = p_bp->GetTCount();
							THROW(p_bp->LoadTItem(&ti, 0, 0/*serial*/));
							if(p_ecs_entry) {
								p_ecs_entry->RByBill = ti.RByBill;
							}
							{
								EgaisWayBillRowTags rt;
								MEMSZERO(rt);
								rt.RByBill = ti.RByBill;
								if(alc_ext.EgaisCode.NotEmptyS())
									STRNSCPY(rt.GoodsCode, alc_ext.EgaisCode);
								if(alc_ext.InformA.NotEmptyS())
									STRNSCPY(rt.InformA, alc_ext.InformA);
								if(alc_ext.InformB.NotEmptyS())
									STRNSCPY(rt.InformB, alc_ext.InformB);
								rt.ManufID = alc_ext.MnfOrImpPsnID;
								STRNSCPY(rt.Serial, serial.Strip());
								STRNSCPY(rt.OrgLineIdent, org_line_ident);
								STRNSCPY(rt.GoodsCategory, alc_ext.CategoryCode); // @v11.3.5
								row_tags.insert(&rt);
							}
						}
					}
				}
			}
		}
    }
    if(is_pack_inited) {
		SString bill_text;
		PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
		p_bp->SortTI(); // Обязательно отсортировать строки по RByBill (могут прийти в перепутанном порядке)
		if(ext_code_set_list.getCount()) {
			for(uint ecsidx = 0; ecsidx < ext_code_set_list.getCount(); ecsidx++) {
				const ExtCodeSetEntry * p_ecs_entry = ext_code_set_list.at(ecsidx);
				uint tipos = 0;
				if(p_ecs_entry && p_ecs_entry->Set.GetCount() && p_bp->SearchTI(p_ecs_entry->RByBill, &tipos))
					p_bp->XcL.Set_2(tipos+1, &p_ecs_entry->Set);
			}
		}
		{
			//
			// Проверка на наличие дубликатов в номерах строк документа
			//
			LongArray rbb_list;
			for(uint i = 0; ok > 0 && i < p_bp->GetTCount(); i++) {
				const PPTransferItem & r_ti = p_bp->ConstTI(i);
				if(r_ti.RByBill && rbb_list.lsearch(r_ti.RByBill)) {
					rr = rrRByBillDup;
					ok = -1;
				}
			}
		}
		if(ok < 0) {
			int    msg_id = 0;
			switch(rr) {
				case rrUnresolvedGoods: msg_id = PPTXT_EGAIS_BILLREJECTEDUNR; break;
				case rrDateOutOfRange: /* (сообщение мешает) msg_id = PPTXT_EGAIS_BILLREJECTOOD; */ break;
				case rrIntrBillIdUnrecogn: break;
				case rrIntrBillNotFound: msg_id = PPTXT_EGAIS_INTRBILLNFOUND; break;
				case rrRByBillDup: msg_id = PPTXT_EGAIS_RBYBILLDUP; break;
				default: msg_id = PPTXT_EGAIS_BILLREJECT; break;
			}
			LogTextWithAddendum(msg_id, bill_text);
		}
		else {
			int    do_skip = 0;
			if(!bill_ident.NotEmptyS()) {
				bill_ident = bill_code;
			}
			{
				ObjTagItem tag_item;
				for(uint i = 0; i < p_bp->GetTCount(); i++) {
					const PPTransferItem & r_ti = p_bp->ConstTI(i);
					for(uint j = 0; j < row_tags.getCount(); j++) {
						const EgaisWayBillRowTags & r_rt = row_tags.at(j);
						if(r_rt.RByBill == r_ti.RByBill) {
							ObjTagList tag_list;
							tag_list.PutItemStrNE(PPTAG_LOT_FSRARLOTGOODSCODE, r_rt.GoodsCode);
							tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, r_rt.InformA);
							tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFB, r_rt.InformB);
							tag_list.PutItemStrNE(PPTAG_LOT_ORGLINEIDENT, r_rt.OrgLineIdent);
							// @v11.3.5 {
							if(Cfg.CategoryTagID && r_rt.GoodsCategory[0]) {
								tag_list.PutItemStrNE(Cfg.CategoryTagID, r_rt.GoodsCategory);
							}
							// } @v11.3.5
							if(r_rt.ManufID && manuf_tag_id) {
								tag_item.SetInt(manuf_tag_id, r_rt.ManufID);
								tag_list.PutItem(manuf_tag_id, &tag_item);
							}
							THROW(p_bp->LTagL.Set(i, &tag_list));
						}
					}
				}
				p_bp->ProcessFlags |= PPBillPacket::pfForceRByBill;
				p_bp->Rec.EdiOp = pPack->DocType;
				if(!freight.IsEmpty())
					p_bp->SetFreight(&freight);
				p_bp->BTagL.PutItemStrNE(PPTAG_BILL_OUTERCODE, bill_ident);
				{
					int    is_valuation_modif = 0;
					if(CheckOpFlags(p_bp->Rec.OpID, OPKF_NEEDVALUATION))
						P_BObj->AutoCalcPrices(p_bp, 0, &is_valuation_modif);
					if(!is_valuation_modif)
						p_bp->InitAmounts();
				}
			}
			pPack->Flags &= ~Packet::fFaultObj; // Пакет документа готов к акцепту
		}
    }
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::Helper_AreArticlesEq(PPID ar1ID, PPID ar2ID)
{
	int    yes = 0;
	if(ar1ID == ar2ID)
		yes = 1;
	else if(ar1ID && ar2ID) {
		const  PPID psn1_id = ObjectToPerson(ar1ID);
		const  PPID psn2_id = ObjectToPerson(ar2ID);
		Reference * p_ref = PPRef;
		if(psn1_id && psn2_id) {
			SString code1, code2;
			p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn1_id, PPTAG_PERSON_FSRARID, code1);
			p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn2_id, PPTAG_PERSON_FSRARID, code2);
			if(code1.NotEmptyS() && code2.NotEmptyS() && code1 == code2)
				yes = 1;
			else {
				PsnObj.GetRegNumber(psn1_id, PPREGT_TPID, getcurdate_(), code1);
				PsnObj.GetRegNumber(psn2_id, PPREGT_TPID, getcurdate_(), code2);
				if(code1.NotEmptyS() && code2.NotEmptyS() && code1 == code2)
					yes = 1;
			}
		}
	}
	return yes;
}

int PPEgaisProcessor::Helper_AcceptBillPacket(Packet * pPack, const TSCollection <PPEgaisProcessor::Packet> * pPackList, uint packIdx)
{
	int    ok = -1;
	PPBillPacket * p_bp = pPack ? static_cast<PPBillPacket *>(pPack->P_Data) : 0;
	if(p_bp && !(pPack->Flags & (Packet::fAcceptedBill|Packet::fFaultObj))) {
		const  int use_dt_in_bill_analog = BIN(ACfg.Hdr.Flags & PPAlbatrosCfgHdr::fUseDateInBillAnalog);
		int    do_skip = 0;
		SString temp_buf;
		SString msg_buf, fmt_buf;
		SString bill_text;
		SString bill_ident;
		SString bill_code;
		Egais_GetBillCode(*p_bp, bill_code); // @v11.0.12
		p_bp->BTagL.GetItemStr(PPTAG_BILL_OUTERCODE, bill_ident);
		if(!bill_ident.NotEmptyS()) {
			// @v11.0.12 bill_ident = p_bp->Rec.Code;
			bill_ident = bill_code; // @v11.0.12
		}
		if(bill_ident.NotEmptyS()) {
			uint   last_analog_pos = 0; // idx+1
			if(pPackList) {
				for(uint pi = packIdx+1; pi < pPackList->getCount(); pi++) {
                    const Packet * p_pack = pPackList->at(pi);
                    if(p_pack && p_pack->P_Data && oneof4(p_pack->DocType, PPEDIOP_EGAIS_WAYBILL, 
						PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4) && !(p_pack->Flags & Packet::fFaultObj)) {
						const PPBillPacket * p_other_bp = static_cast<const PPBillPacket *>(p_pack->P_Data);
						if((!use_dt_in_bill_analog || p_other_bp->Rec.Dt == p_bp->Rec.Dt) && sstreq(p_other_bp->Rec.Code, p_bp->Rec.Code)) {
                            if(p_other_bp->BTagL.GetItemStr(PPTAG_BILL_OUTERCODE, temp_buf) > 0 && temp_buf == bill_ident) {
								if(Helper_AreArticlesEq(p_other_bp->Rec.Object, p_bp->Rec.Object))
									last_analog_pos = pi+1;
                            }
                        }
                    }
				}
			}
			if(last_analog_pos) {
				do_skip = 1;
				PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
				Log(msg_buf.Printf(PPLoadTextS(PPTXT_EGAIS_BILLFORWARDFOUND, fmt_buf), bill_text.cptr()));
			}
			else {
				PPIDArray ex_bill_id_list;
				PPRef->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_OUTERCODE, bill_ident, &ex_bill_id_list);
				for(uint i = 0; !do_skip && i < ex_bill_id_list.getCount(); i++) {
					const  PPID ex_bill_id = ex_bill_id_list.get(i);
					BillTbl::Rec ex_bill_rec;
					if(P_BObj->Fetch(ex_bill_id, &ex_bill_rec) > 0 && ex_bill_rec.OpID == p_bp->Rec.OpID) {
						if(!use_dt_in_bill_analog || ex_bill_rec.Dt == p_bp->Rec.Dt) {
							// @v11.0.12 BillCore::GetCode(temp_buf = ex_bill_rec.Code);
							Egais_GetBillCode(ex_bill_rec, temp_buf); // @v11.0.12
							// @v11.0.12 if(temp_buf.CmpNC(p_bp->Rec.Code) == 0 && Helper_AreArticlesEq(ex_bill_rec.Object, p_bp->Rec.Object)) {
							if(temp_buf.IsEqNC(bill_code) && Helper_AreArticlesEq(ex_bill_rec.Object, p_bp->Rec.Object)) { // @v11.0.12
								do_skip = 1;
							}
							if(do_skip) {
								/* Избыточное сообщение - мешает
								PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
								PPLoadText(PPTXT_EGAIS_BILLTOOACCEPTED, fmt_buf);
								Log(msg_buf.Printf(fmt_buf, (const char *)bill_text));
								*/
								if(diffdate(getcurdate_(), ex_bill_rec.Dt) > 14)
									pPack->Flags |= Packet::fDoDelete;
								//
								// Аварийный блок, восстанавливающий марки в принятом ранее документе если они не были сохранены
								// (не стояла галка CCFLG2_USELOTXCODE в PPCommConfig) или что-то еще пошло не так.
								//
								if(DS.CheckExtFlag(ECF_AVERAGE) && PPMaster && p_bp->XcL.GetCount() && P_BObj->P_LotXcT) {
									SBuffer vxcl_buf;
									PPLotExtCodeContainer ex_xcl;
									if(P_BObj->P_LotXcT->GetContainer(ex_bill_rec.ID, ex_xcl)) {
										if(ex_xcl.GetCount() == 0) {
											PPBillPacket ex_bpack;
											if(P_BObj->ExtractPacket(ex_bill_rec.ID, &ex_bpack) > 0) {
												int   not_eq_packets = 0;
												if(ex_bpack.GetTCount() == p_bp->GetTCount()) {
													for(uint tidx = 0; tidx < p_bp->GetTCount(); tidx++) {
														const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
														const PPTransferItem & r_ex_ti = ex_bpack.ConstTI(tidx);
														if(r_ti.RByBill != r_ex_ti.RByBill)
															not_eq_packets = 1;
													}
												}
												else
													not_eq_packets = 1;
												if(!not_eq_packets) {
													ex_bpack.XcL = p_bp->XcL;
													THROW(P_BObj->UpdatePacket(&ex_bpack, 1));
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
		if(!do_skip) {
			THROW(P_BObj->TurnPacket(p_bp, 1));
			pPack->Flags |= Packet::fAcceptedBill;
			ok = 1;
			PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
			LogTextWithAddendum(PPTXT_EGAIS_BILLACCEPTED, bill_text);
			if(pPack->IntrBillID > 0) {
				LogTextWithAddendum(PPTXT_EGAIS_INTRBILLFOUND, bill_text);
				PPBillPacket wroff_pack;
				THROW(P_BObj->ExtractPacket(pPack->IntrBillID, &wroff_pack) > 0);
				{
					const int intr = IsIntrOp(wroff_pack.Rec.OpID);
					int   do_update_wroff_pack = 0;
					if(wroff_pack.Rec.LinkBillID != p_bp->Rec.ID) {
						wroff_pack.Rec.LinkBillID = p_bp->Rec.ID;
						do_update_wroff_pack = 1;
					}
					for(uint i = 0; i < wroff_pack.GetTCount(); i++) {
						PPTransferItem & r_wo_ti = wroff_pack.TI(i);
						for(uint j = 0; j < p_bp->GetTCount(); j++) {
							const PPTransferItem & r_ti = p_bp->ConstTI(j);
							if(r_ti.RByBill == r_wo_ti.RByBill) {
								const ObjTagList * p_src_tag_list = p_bp->LTagL.Get(j);
								if(p_src_tag_list) {
									const ObjTagItem * p_infa_tag = p_src_tag_list->GetItem(PPTAG_LOT_FSRARINFA);
									const ObjTagItem * p_infb_tag = p_src_tag_list->GetItem(PPTAG_LOT_FSRARINFB);
									const ObjTagItem * p_egaiscode_tag = p_src_tag_list->GetItem(PPTAG_LOT_FSRARLOTGOODSCODE);
									if(p_infa_tag || p_infb_tag || p_egaiscode_tag) { // p_egaiscode_tag
										ObjTagList dest_tag_list;
										const ObjTagList * p_dest_tag_list = 0;
										switch(intr) {
											case INTREXPND:
												THROW_MEM(SETIFZ(wroff_pack.P_MirrorLTagL, new PPLotTagContainer));
												p_dest_tag_list = wroff_pack.P_MirrorLTagL->Get(i);
												RVALUEPTR(dest_tag_list, p_dest_tag_list);
												if(dest_tag_list.PutItemNZ(p_infa_tag, 0) > 0)
													do_update_wroff_pack = 1;
												if(dest_tag_list.PutItemNZ(p_infb_tag, 0) > 0)
													do_update_wroff_pack = 1;
												if(dest_tag_list.PutItemNZ(p_egaiscode_tag, 0) > 0)
													do_update_wroff_pack = 1;
												wroff_pack.P_MirrorLTagL->Set(i, &dest_tag_list);
												break;
											case INTRRCPT:
												p_dest_tag_list = wroff_pack.LTagL.Get(i);
												RVALUEPTR(dest_tag_list, p_dest_tag_list);
												if(dest_tag_list.PutItemNZ(p_infa_tag, 0) > 0)
													do_update_wroff_pack = 1;
												if(dest_tag_list.PutItemNZ(p_infb_tag, 0) > 0)
													do_update_wroff_pack = 1;
												if(dest_tag_list.PutItemNZ(p_egaiscode_tag, 0) > 0)
													do_update_wroff_pack = 1;
												wroff_pack.LTagL.Set(i, &dest_tag_list);
												break;
                                        }
									}
								}
							}
						}
					}
					if(do_update_wroff_pack) {
						wroff_pack.ProcessFlags |= PPBillPacket::pfIgnoreStatusRestr; // @v11.0.10
						PPTransaction tra(1);
						THROW(tra);
                        THROW(P_BObj->UpdatePacket(&wroff_pack, 0));
						THROW(P_BObj->P_Tbl->SetRecFlag(p_bp->Rec.ID, BILLF_WRITEDOFF, 1, 0));
						THROW(tra.Commit());
					}
				}
			}
		}
	}
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::Helper_AcceptTtnRefB(const Packet * pPack, const TSCollection <PPEgaisProcessor::Packet> * pPackList, const uint packIdx, LongArray & rSkipPackIdxList)
{
	int    ok = -1;
	const InformB * p_inf = static_cast<const InformB *>(pPack->P_Data);
	SString temp_buf;
	int   do_skip = 0;
	//
	// Пытаемся избежать повторного разбора двух и более PPEDIOP_EGAIS_TTNINFORMBREG
	// относящихся к одному документу (разные версии посылок). Нам нужна только последняя версия.
	//
	for(uint fwpackidx = packIdx+1; fwpackidx < pPackList->getCount(); fwpackidx++) {
		const Packet * p_fw_pack = pPackList->at(fwpackidx);
		if(p_fw_pack && p_fw_pack->P_Data && p_fw_pack->DocType == pPack->DocType) {
			const InformB * p_fw_inf = static_cast<const InformB *>(p_fw_pack->P_Data);
			if(p_fw_inf->Id.CmpNC(p_inf->Id) == 0 && p_fw_inf->OuterCode.CmpNC(p_inf->OuterCode) == 0 && p_fw_inf->OuterDate == p_inf->OuterDate) {
				if(p_fw_inf->FixDate > p_inf->FixDate)
                    do_skip = 1;
				else if(p_fw_inf->FixDate < p_inf->FixDate)
					rSkipPackIdxList.add(static_cast<long>(fwpackidx));
				else {
                    int64 this_fix_number = 0;
                    int64 fw_fix_number = 0;
					temp_buf = p_inf->FixNumber;
					while(temp_buf.Len() && !isdec(temp_buf.C(0)))
                        temp_buf.ShiftLeft();
					this_fix_number = temp_buf.ToInt64();
					temp_buf = p_fw_inf->FixNumber;
					while(temp_buf.Len() && !isdec(temp_buf.C(0)))
                        temp_buf.ShiftLeft();
					fw_fix_number = temp_buf.ToInt64();
					if(fw_fix_number > this_fix_number) {
						do_skip = 1;
					}
					else if(fw_fix_number < this_fix_number)
						rSkipPackIdxList.add(static_cast<long>(fwpackidx));
				}
			}
		}
	}
	if(!do_skip) {
		Reference * p_ref = PPRef;
		const SString edi_ident = p_inf->WBRegId;
		SString bill_text;
		BillTbl::Rec ex_bill_rec;
		PPIDArray bill_id_list;
		p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_OUTERCODE, p_inf->Id, &bill_id_list);
		if(p_inf->Id.IsDec()) {
			int64 id_64 = p_inf->Id.ToInt64();
			if(id_64 > 0 && id_64 < LONG_MAX) {
				PPID   id_32 = static_cast<PPID>(id_64);
				if(!bill_id_list.lsearch(id_32) && P_BObj->Fetch(id_32, &ex_bill_rec) > 0) {
					if(IsOpBelongTo(ex_bill_rec.OpID, Cfg.ExpndOpID) || IsOpBelongTo(ex_bill_rec.OpID, Cfg.IntrExpndOpID)) {
						// @v11.0.12 BillCore::GetCode(temp_buf = ex_bill_rec.Code);
						Egais_GetBillCode(ex_bill_rec, temp_buf); // @v11.0.12
						if(temp_buf.NotEmptyS() && temp_buf.CmpNC(p_inf->OuterCode) == 0) {
							long    _dd = diffdate(p_inf->FixDate, ex_bill_rec.Dt);
							if(_dd >= 0 && _dd <= 30)
								bill_id_list.add(id_32);
						}
					}
				}
			}
		}
		int    _bill_found = 0;
		int    _err = 0; // Признак того, что произошла ошибка. Если !0, то удалять документ из УТМ нельзя
		for(uint j = 0; !_bill_found && j < bill_id_list.getCount(); j++) {
			const  PPID bill_id = bill_id_list.get(j);
			BillTbl::Rec ex_bill_rec;
			PPBillPacket bp, _link_bp;
			PPBillPacket * p_intr_link_bp = 0;
			int   do_update = 0;
			int   do_update_wroff_pack = 0;
			//
			// Значения do_process:
			// 0 - не обрабатывать,
			// 1 - приход от поставщика,
			// 2 - возврат от покупателя
			// 3 - расходные операции (по ним сейчас ничего не делаем, но временно оставляем так)
			int   do_process = 0;
			if(P_BObj->Fetch(bill_id, &ex_bill_rec) > 0) {
				if(IsOpBelongTo(ex_bill_rec.OpID, ACfg.Hdr.EgaisRcptOpID))
					do_process = 1;
				else if(IsOpBelongTo(ex_bill_rec.OpID, ACfg.Hdr.EgaisRetOpID))
					do_process = 2;
				if(IsOpBelongTo(ex_bill_rec.OpID, Cfg.ExpndOpID) || IsOpBelongTo(ex_bill_rec.OpID, Cfg.IntrExpndOpID))
					do_process = 3;
			}
			if(do_process == 3) {
				// Ничего не делаем по справке Б для собственного документа
			}
			else if(oneof2(do_process, 1, 2)) {
				// @v11.0.12 BillCore::GetCode(temp_buf = ex_bill_rec.Code);
				Egais_GetBillCode(ex_bill_rec, temp_buf); // @v11.0.12
				if(temp_buf.NotEmptyS() && temp_buf.CmpNC(p_inf->OuterCode) == 0 && P_BObj->ExtractPacket(bill_id, &bp) > 0) {
					_bill_found = 1;
					PPObjBill::MakeCodeString(&bp.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
					ObjTagItem tag_item;
					int    intr = 0;
					const  ObjTagItem * p_ex_tag_item = bp.BTagL.GetItem(PPTAG_BILL_EDIIDENT);
					if(!p_ex_tag_item) {
						if(tag_item.SetStr(PPTAG_BILL_EDIIDENT, edi_ident)) {
							bp.BTagL.PutItem(PPTAG_BILL_EDIIDENT, &tag_item);
							LogTextWithAddendum(PPTXT_EGAIS_BILLEDIASSGNINFB, bill_text);
							do_update = 1;
						}
					}
					else {
						p_ex_tag_item->GetStr(temp_buf);
						if(temp_buf.CmpNC(edi_ident) != 0) {
							//LogTextWithAddendum(PPTXT_EGAIS_BILLEDIIDNINFB, bill_text);
							_bill_found = 0;
						}
					}
					if(_bill_found) {
						LongArray potential_row_n_list;
						for(uint bi = 0; bi < p_inf->Items.getCount(); bi++) {
							const  InformBItem & r_bitem = p_inf->Items.at(bi);
							int    row_idx = -1;
							potential_row_n_list.Z();
							if(r_bitem.OrgRowIdent[0])
								bp.LTagL.SearchString(r_bitem.OrgRowIdent, PPTAG_LOT_ORGLINEIDENT, 0, potential_row_n_list);
							if(potential_row_n_list.getCount() == 1) {
								const long temp_row_idx = potential_row_n_list.get(0);
								if(temp_row_idx >= 0 && temp_row_idx < bp.GetTCountI())
									row_idx = temp_row_idx;
							}
							if(row_idx < 0 && r_bitem.P && r_bitem.Ident[0]) {
								for(uint t = 0; t < bp.GetTCount(); t++) {
									if(bp.TI(t).RByBill == r_bitem.P)
										row_idx = static_cast<int>(t);
								}
							}
							if(row_idx >= 0) {
								assert(row_idx >= 0 && row_idx < bp.GetTCountI()); // @paranoic
								PPTransferItem & r_ti = bp.TI(row_idx);
								if(r_ti.RByBill == r_bitem.P) {
									const ObjTagList * p_ex_tag_list = bp.LTagL.Get(row_idx);
									ObjTagList tag_list;
									int    do_update_informb = 1;
									if(p_ex_tag_list) {
										p_ex_tag_item = p_ex_tag_list->GetItem(PPTAG_LOT_FSRARINFB);
										if(p_ex_tag_item && p_ex_tag_item->GetStr(temp_buf) > 0 && temp_buf.CmpNC(r_bitem.Ident) == 0)
											do_update_informb = 0;
										else
											tag_list = *p_ex_tag_list;
									}
									if(do_update_informb) {
										tag_list.PutItemStr(PPTAG_LOT_FSRARINFB, r_bitem.Ident);
										THROW(bp.LTagL.Set(row_idx, &tag_list));
										do_update = 1;
									}
								}
							}
						}
						{
							//
							// Если драфт-документ уже имеет привязанный документ внутреннего перемещения,
							// то нам понадобится пакет этого привязанного документа для изменения
							// в нем справок А и Б.
							//
							PPIDArray wroff_bill_list;
							BillTbl::Rec wroff_bill_rec;
							for(DateIter diter; P_BObj->P_Tbl->EnumLinks(bp.Rec.ID, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;) {
								const int local_intr = IsIntrOp(wroff_bill_rec.OpID);
								if(oneof2(local_intr, INTREXPND, INTRRCPT))
									wroff_bill_list.add(wroff_bill_rec.ID);
							}
							if(wroff_bill_list.getCount() > 1)
								LogTextWithAddendum(PPTXT_EGAIS_BILLWROFFCONFL, bill_text);
							else if(wroff_bill_list.getCount() == 1) {
								THROW(P_BObj->ExtractPacket(wroff_bill_list.get(0), &_link_bp) > 0);
								intr = IsIntrOp(_link_bp.Rec.OpID);
								if(oneof2(intr, INTREXPND, INTRRCPT)) {
									p_intr_link_bp = &_link_bp;
									for(uint lbi = 0; lbi < _link_bp.GetTCount(); lbi++) {
										PPTransferItem & r_wo_ti = _link_bp.TI(lbi);
										for(uint t = 0; t < bp.GetTCount(); t++) {
											const PPTransferItem & r_ti = bp.ConstTI(t);
											if(r_ti.RByBill == r_wo_ti.RByBill) {
												const ObjTagList * p_src_tag_list = bp.LTagL.Get(t);
												if(p_src_tag_list) {
													const ObjTagItem * p_infa_tag = p_src_tag_list->GetItem(PPTAG_LOT_FSRARINFA);
													const ObjTagItem * p_infb_tag = p_src_tag_list->GetItem(PPTAG_LOT_FSRARINFB);
													if(p_infa_tag || p_infb_tag) {
														ObjTagList dest_tag_list;
														if(intr == INTREXPND) {
															THROW_MEM(SETIFZ(_link_bp.P_MirrorLTagL, new PPLotTagContainer));
															const ObjTagList * p_dest_tag_list = _link_bp.P_MirrorLTagL->Get(lbi);
															RVALUEPTR(dest_tag_list, p_dest_tag_list);
															if(dest_tag_list.PutItemNZ(p_infa_tag, 0) > 0)
																do_update_wroff_pack = 1;
															if(dest_tag_list.PutItemNZ(p_infb_tag, 0) > 0)
																do_update_wroff_pack = 1;
															_link_bp.P_MirrorLTagL->Set(lbi, &dest_tag_list);
														}
														else if(intr == INTRRCPT) {
															const ObjTagList * p_dest_tag_list = _link_bp.LTagL.Get(lbi);
															RVALUEPTR(dest_tag_list, p_dest_tag_list);
															if(dest_tag_list.PutItemNZ(p_infa_tag, 0) > 0)
																do_update_wroff_pack = 1;
															if(dest_tag_list.PutItemNZ(p_infb_tag, 0) > 0)
																do_update_wroff_pack = 1;
															_link_bp.LTagL.Set(lbi, &dest_tag_list);
														}
													}
												}
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			if(do_update) {
				if(!P_BObj->UpdatePacket(&bp, 1)) {
					_err = 1;
					LogLastError();
				}
			}
			if(do_update_wroff_pack && p_intr_link_bp) {
				if(!P_BObj->UpdatePacket(p_intr_link_bp, 1)) {
					_err = 1;
					LogLastError();
				}
			}
		}
		if(/*!(flags & rifOffline) && */_bill_found && !_err && diffdate(getcurdate_(), p_inf->OuterDate) >= 14) {
			ok = 2; // сигнал для удаления пакета с УТМ
			//DeleteSrcPacket(pPack, reply_list);
		}
	}
	CATCHZOK
	return ok;
}

struct EgaisRestItem { // @flat
	EgaisRestItem() : GoodsID(0), MnfOrImpPsnID(0), Rest(0.0)
	{
		PTR32(InformAIdent)[0] = 0;
		PTR32(InformBIdent)[0] = 0;
		PTR32(EgaisCode)[0] = 0;
	}
	PPID   GoodsID;
	PPID   MnfOrImpPsnID;
	char   InformAIdent[24];
    char   InformBIdent[24];
    char   EgaisCode[24];
    double Rest;
};

int PPEgaisProcessor::Helper_CreateWriteOff_ByCCheck(const PPBillPacket * pCurrentRestPack, const DateRange * pPeriod) 
{
	int    ok = 1;
	return ok;
}

int PPEgaisProcessor::Helper_CreateWriteOffShop(int v3markMode, const PPBillPacket * pCurrentRestPack, const DateRange * pPeriod) // @indevelopment
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  PPID loc_id = pCurrentRestPack ? pCurrentRestPack->Rec.LocID : 0;
	const  bool use_lotxcode = LOGIC(CConfig.Flags2 & CCFLG2_USELOTXCODE);
	PPID   op_id = 0;
	SString temp_buf;
	SString fmt_buf;
	SString bill_text;
	PPObjOprKind op_obj;
	ObjTagItem tag_item;
	PPBillPacket * p_wroff_bp = 0;
	PPBillPacket * p_wroff_wo_marks_bp = 0; // @v11.8.3
	//
	// Формируем документ передачи в торговый зал (регистр 2) лотов, которых
	// нет в отчете ЕГАИС об остатках в торговом зале.
	//
	const LDATE _cur_date = getcurdate_();
	PPIDArray cc_id_list_to_mark; // @v11.8.2 Список чеков, которые необходимо пометить тегом extssEgaisProcessingTag
    PPIDArray alco_goods_list;
    PPID   wos_op_id = 0;
    if((!v3markMode || use_lotxcode) && oneof3(Cfg.E.WrOffShopWay, Cfg.woswBalanceWithLots, Cfg.woswByBills, Cfg.woswByCChecks)) {
		if(v3markMode) {
			if(!op_obj.GetEdiWrOffWithMarksOp(&wos_op_id, 1))
				LogLastError();
		}
		else {
			if(!op_obj.GetEdiWrOffShopOp(&wos_op_id, 1))
				LogLastError();
		}
		if(wos_op_id) {
			PPID   ex_today_bill_id = 0;
			{
				DateRange test_period;
				test_period.SetDate(_cur_date);
				BillTbl::Rec sbill_rec;
				SString edi_ack;
				for(SEnum en = P_BObj->P_Tbl->EnumByOp(wos_op_id, &test_period, 0); !ex_today_bill_id && en.Next(&sbill_rec) > 0;) {
					if(sbill_rec.LocID == loc_id) {
						ex_today_bill_id = sbill_rec.ID;
						PPObjBill::MakeCodeString(&sbill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
					}
				}
			}
			if(ex_today_bill_id)
				LogTextWithAddendum(PPTXT_EGAIS_TODAYYETWROFFR2, bill_text);
			else if(GetAlcGoodsList(alco_goods_list) > 0) {
				SString egais_code;
				SString egais_code_by_restdoc; // @v11.8.3
				SString ref_a;
				SString ref_b;
				SString egais_code_by_mark;
				ReceiptTbl::Rec lot_rec;
				PPIDArray lot_id_list;
				PPViewCCheck cc_view;
				CCheckCore * p_cc = cc_view.GetCc();
				bool do_balance_with_lots_unmarked_only = false; // @v11.7.12 Балансировать с текущими остатками только немаркированную продукцию
				if(Cfg.E.WrOffShopWay == Cfg.woswByCChecks) {
					// @v11.7.12 {
					if(Cfg.E.Flags & Cfg.fNMarkedBalance)
						do_balance_with_lots_unmarked_only = true;
					// } @v11.7.12
					int    skip = 0;
					CCheckFilt cc_filt;
					if(!Cfg.P_CcFilt) {
						LogTextWithAddendum(PPTXT_EGAIS_WROFFR2CCFUNDEF, temp_buf.Z());
						skip = 1;
					}
					else {
						cc_filt = *Cfg.P_CcFilt;
						if(cc_filt.Period.IsZero()) {
							if(pPeriod && !pPeriod->IsZero())
								cc_filt.Period = *pPeriod;
							else
								cc_filt.Period.SetDate(plusdate(getcurdate_(), -1));
						}
						if(!cc_filt.NodeList.GetCount() && loc_id) {
							PPObjCashNode cn_obj;
							PPCashNode cn_rec;
							skip = 1;
							for(SEnum en = cn_obj.P_Ref->Enum(PPOBJ_CASHNODE, 0); en.Next(&cn_rec) > 0;) {
								if(cn_rec.LocID == loc_id) {
									cc_filt.NodeList.Add(cn_rec.ID);
									skip = 0;
								}
							}
							if(skip)
								LogTextWithAddendum(PPTXT_EGAIS_WROFFR2CCFNONODES, temp_buf.Z());
						}
					}
					if(!skip) {
						CCheckViewItem cc_item;
						THROW(cc_view.Init_(&cc_filt));
						if(p_cc) {
							CCheckPacket cc_pack;
							EgaisMarkBlock emb;
							SString egais_mark;
							BarcodeArray bc_list;
							LongArray crest_row_list;
							LongArray ex_row_list;
							PPIDArray lot_list_by_refb; // @v11.8.3
							PPIDArray cc_list;
							TSVector <LotExtCodeTbl::Rec> lec_rec_list;
							const  PPID draft_rcpt_op_id = ACfg.Hdr.EgaisRcptOpID;
							THROW_MEM(SETIFZ(P_LecT, new LotExtCodeCore));
							for(cc_view.InitIteration(0); cc_view.NextIteration(&cc_item) > 0;) {
								cc_list.add(cc_item.ID);
							}
							cc_list.sortAndUndup();
							for(uint ccidx = 0; ccidx < cc_list.getCount(); ccidx++) {
								const  PPID cc_id = cc_list.get(ccidx);
								bool  do_mark_ccheck = false; // @v11.8.2 Признак того, что на чек необходимо установить тег extssEgaisProcessingTag
								SString processing_tag; // @v11.8.2
								p_cc->LoadPacket(cc_id, 0, &cc_pack);
								cc_pack.GetExtStrData(CCheckPacket::extssEgaisProcessingTag, processing_tag); // @v11.8.2
								PPID   new_processing_wroff_bill_id = 0;
								PPID   ex_processing_wroff_bill_id = processing_tag.ToLong();
								BillTbl::Rec processing_wroff_bill_rec;
								if(ex_processing_wroff_bill_id) {
									if(P_BObj->Fetch(ex_processing_wroff_bill_id, &processing_wroff_bill_rec) > 0 && processing_wroff_bill_rec.OpID == wos_op_id) {
										;
									}
									else 
										ex_processing_wroff_bill_id = 0;
								}
								for(uint clidx = 0; clidx < cc_pack.GetCount(); clidx++) {
									const CCheckLineTbl::Rec & r_ccl = cc_pack.GetLineC(clidx);
									const  PPID goods_id = labs(r_ccl.GoodsID);
									if(alco_goods_list.bsearch(goods_id)) {
										cc_pack.GetLineTextExt(clidx+1, CCheckPacket::lnextEgaisMark, /*temp_buf*/egais_mark);
										bc_list.clear();
										if(egais_mark.NotEmpty() && ParseEgaisMark(egais_mark, emb) > 0) {
											egais_code = emb.EgaisCode;
											bc_list.Add(egais_code, 0, 1.0);
										}
										else
											GetEgaisCodeList(goods_id, bc_list);
										const double ccl_qtty = r_ccl.Quantity;
										double ccl_rest = ccl_qtty;
										if(v3markMode) {
											if(egais_mark.Len() == 150) {
												ref_b.Z();
												egais_code_by_mark.Z();
												if(P_LecT->GetRecListByMark(egais_mark, lec_rec_list) > 0) {
													enum {
														emrfAlreadyWrittenOff = 0x0001,
														emrfInitBillFound     = 0x0002
													};
													int    ex_mark_results = 0;
													double ref_cost = 0.0;  // Цена поступления из строки приходного (драфт) документа
													double ref_price = 0.0; // Цена реализации из строки приходного (драфт) документа
													const  bool use_last_init_op = true; // @v12.0.2 До этого релиза применялась первая инициирующая операция. Это, вероятно, не правильно - надо последнюю использовать.
													uint   init_op_idx = 0; // @v12.02 (индекс +1) инициирующей операции в списке lec_rec_list
													for(uint mridx = 0; mridx < lec_rec_list.getCount(); mridx++) {
														const LotExtCodeTbl::Rec & r_lec_rec = lec_rec_list.at(mridx);
														BillTbl::Rec lec_bill_rec;
														if(P_BObj->Fetch(r_lec_rec.BillID, &lec_bill_rec) > 0) {
															if(lec_bill_rec.OpID == wos_op_id) {
																// Уже есть документ со списанием этой марки // @todo Выдать сообщение об этом
																ex_mark_results |= emrfAlreadyWrittenOff;
															}
															else if(lec_bill_rec.OpID == draft_rcpt_op_id) {
																// @v12.0.1 {
																if(use_last_init_op) {
																	init_op_idx = mridx+1;
																}
																else if(!init_op_idx) {
																	init_op_idx = mridx+1;
																}
																// } @v12.0.1 
																/* @v12.0.2																	
																if(!(ex_mark_results & emrfInitBillFound)) {
																	int16 ln = 0;
																	PPTransferItem lec_ti;
																	for(int lec_rbb = 0; P_BObj->P_CpTrfr->EnumItems(lec_bill_rec.ID, &lec_rbb, &lec_ti, 0) > 0;) {
																		if(++ln == r_lec_rec.RByBill)
																			break;
																	}
																	if(ln == r_lec_rec.RByBill) {
																		PPLotTagContainer ltc;
																		P_BObj->LoadRowTagListForDraft(lec_bill_rec.ID, ltc);
																		ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARINFB, ref_b);
																		ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_mark);
																		ref_cost = lec_ti.Cost;
																		ref_price = lec_ti.Price;
																	}
																	ex_mark_results |= emrfInitBillFound;
																}*/
															}
														}
													}
													// @v12.0.2 {
													if(init_op_idx) {
														assert(init_op_idx <= lec_rec_list.getCount());
														const LotExtCodeTbl::Rec & r_lec_rec = lec_rec_list.at(init_op_idx-1);
														//
														int16 ln = 0;
														PPTransferItem lec_ti;
														for(int lec_rbb = 0; P_BObj->P_CpTrfr->EnumItems(r_lec_rec.BillID, &lec_rbb, &lec_ti, 0) > 0;) {
															if(++ln == r_lec_rec.RByBill)
																break;
														}
														if(ln == r_lec_rec.RByBill) {
															PPLotTagContainer ltc;
															P_BObj->LoadRowTagListForDraft(r_lec_rec.BillID, ltc);
															ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARINFB, ref_b);
															ltc.GetTagStr(ln-1, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_mark);
															ref_cost = lec_ti.Cost;
															ref_price = lec_ti.Price;
														}
														ex_mark_results |= emrfInitBillFound;
													}
													// } @v12.0.2 
													if(!(ex_mark_results & emrfAlreadyWrittenOff) && ref_b.NotEmpty() && egais_code_by_mark.NotEmpty()) {
														double ex_row_qtty = 0.0;
														double crest = 0.0;
														if(p_wroff_bp) {
															p_wroff_bp->LTagL.SearchString(ref_b, PPTAG_LOT_FSRARINFB, 0, ex_row_list);
															for(uint eridx = 0; eridx < ex_row_list.getCount(); eridx++) {
																const PPTransferItem & r_ex_ti = p_wroff_bp->TI(ex_row_list.get(eridx));
																ex_row_qtty += r_ex_ti.Quantity_;
															}
														}
														else
															ex_row_list.clear();
														{
															pCurrentRestPack->LTagL.SearchString(ref_b, PPTAG_LOT_FSRARINFB, 0, crest_row_list);
															for(uint cri = 0; cri < crest_row_list.getCount(); cri++) {
																const PPTransferItem & r_cr_ti = pCurrentRestPack->ConstTI(crest_row_list.get(cri));
																crest += r_cr_ti.Quantity_;
															}
															crest += ex_row_qtty;
														}
														if(ccl_rest != 0.0 && fsign(ccl_rest) == fsign(ccl_qtty)) {
															double wroff_qtty = 0.0;
															if(ccl_rest < 0.0)
																wroff_qtty = ccl_rest;
															// ЕГАИС не разрешает списание в минус else if(bci == (bc_list.getCount()-1)) wroff_qtty = ccl_rest;
															else if(crest > 0.0)
																wroff_qtty = MIN(ccl_rest, crest);
															if(wroff_qtty != 0.0) {
																if(!p_wroff_bp) {
																	THROW_MEM(p_wroff_bp = new PPBillPacket);
																	THROW(p_wroff_bp->CreateBlank2(wos_op_id, _cur_date, loc_id, 1));
																	p_wroff_bp->BTagL.PutItemStr(PPTAG_BILL_FORMALREASON, PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_SELLING, temp_buf)); // "Реализация"
																}
																if(ex_row_list.getCount()) {
																	const uint ex_pos = ex_row_list.get(0);
																	PPTransferItem & r_ex_ti = p_wroff_bp->TI(ex_pos);
																	r_ex_ti.Quantity_ -= wroff_qtty;
																	if(use_lotxcode && egais_mark.NotEmpty())
																		p_wroff_bp->XcL.Add(ex_pos+1, 0, 0, egais_mark, 0);
																}
																else {
																	PPTransferItem ti;
																	uint   new_pos = p_wroff_bp->GetTCount();
																	THROW(ti.Init(&p_wroff_bp->Rec, 1));
																	THROW(ti.SetupGoods(goods_id, 0));
																	ti.Quantity_ = -wroff_qtty;
																	ti.Cost  = ref_cost;
																	ti.Price = ref_price;
																	THROW(p_wroff_bp->LoadTItem(&ti, 0, 0));
																	{
																		ObjTagList tag_list;
																		tag_list.PutItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_mark);
																		tag_list.PutItemStr(PPTAG_LOT_FSRARINFB, ref_b);
																		THROW(p_wroff_bp->LTagL.Set(new_pos, &tag_list));
																	}
																	if(use_lotxcode && egais_mark.NotEmpty())
																		p_wroff_bp->XcL.Add(new_pos+1, 0, 0, egais_mark, 0);
																}
																ccl_rest -= wroff_qtty;
															}
														}
													}
												}
											}
											else if(egais_mark.IsEmpty()) { // no mark
												// @v11.8.2 @construction Списание немаркированной продукции со склада по
												// чекам вскрытия тары.
												double wroff_qtty = ccl_qtty;
												if(wroff_qtty > 0.0 && !ex_processing_wroff_bill_id) {
													for(uint crpidx = 0; wroff_qtty > 0.0 && pCurrentRestPack->SearchGoods(goods_id, &crpidx); crpidx++) {
														const PPTransferItem & r_crp_ti = pCurrentRestPack->ConstTI(crpidx);
														double ex_wroff_qtty = 0.0;
														pCurrentRestPack->LTagL.GetTagStr(crpidx, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_restdoc); // @v11.8.3
														pCurrentRestPack->LTagL.GetTagStr(crpidx, PPTAG_LOT_FSRARINFA, ref_a);
														pCurrentRestPack->LTagL.GetTagStr(crpidx, PPTAG_LOT_FSRARINFB, ref_b);
														if(ref_b.NotEmpty()) {
															uint   last_ex_row_idx_1b = 0;
															if(!p_wroff_wo_marks_bp) {
																THROW_MEM(p_wroff_wo_marks_bp = new PPBillPacket);
																THROW(p_wroff_wo_marks_bp->CreateBlank2(wos_op_id, _cur_date, loc_id, 1));
																p_wroff_wo_marks_bp->BTagL.PutItemStr(PPTAG_BILL_FORMALREASON, PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_SELLING, temp_buf)); // "Реализация"
															}
															else {
																LongArray ex_row_idx_list;
																p_wroff_wo_marks_bp->LTagL.SearchString(ref_b, PPTAG_LOT_FSRARINFB, 0, ex_row_idx_list);
																for(uint i = 0; i < ex_row_idx_list.getCount(); i++) {
																	const uint ex_row_idx = static_cast<uint>(ex_row_idx_list.get(i));
																	const PPTransferItem & r_wroff_ti = p_wroff_wo_marks_bp->ConstTI(ex_row_idx);
																	ex_wroff_qtty += abs(r_wroff_ti.Quantity_);
																	last_ex_row_idx_1b = ex_row_idx+1;
																}
															}
															const double rest_qtty = r_crp_ti.Quantity_ - ex_wroff_qtty;
															const double qtty_to_apply = MIN(rest_qtty, wroff_qtty);
															if(qtty_to_apply > 0.0) {
																if(last_ex_row_idx_1b) {
																	PPTransferItem & r_ti = p_wroff_wo_marks_bp->TI(last_ex_row_idx_1b-1);
																	r_ti.Quantity_ -= qtty_to_apply;
																}
																else {
																	double cost_by_last_lot = 0.0;
																	double price_by_last_lot = 0.0;
																	{
																		lot_list_by_refb.Z();
																		p_ref->Ot.SearchObjectsByStr(PPOBJ_LOT, PPTAG_LOT_FSRARINFB, ref_b, &lot_list_by_refb);
																		lot_list_by_refb.sortAndUndup();
																		uint lotidx = lot_list_by_refb.getCount();
																		if(lotidx) do {
																			const  PPID last_lot_id = lot_list_by_refb.get(--lotidx);
																			if(P_BObj->trfr->Rcpt.Search(last_lot_id, &lot_rec) > 0) {
																				cost_by_last_lot = lot_rec.Cost;
																				price_by_last_lot = lot_rec.Price;
																				break;
																			}
																		} while(lotidx);
																	}
																	PPTransferItem ti;
																	uint   new_pos = p_wroff_wo_marks_bp->GetTCount();
																	THROW(ti.Init(&p_wroff_wo_marks_bp->Rec, 1));
																	THROW(ti.SetupGoods(goods_id, 0));
																	ti.Quantity_ = -qtty_to_apply;
																	ti.Cost = cost_by_last_lot;
																	ti.Price = price_by_last_lot;
																	THROW(p_wroff_wo_marks_bp->LoadTItem(&ti, 0, 0));
																	{
																		ObjTagList tag_list;
																		tag_list.PutItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code_by_restdoc);
																		tag_list.PutItemStr(PPTAG_LOT_FSRARINFB, ref_b);
																		THROW(p_wroff_wo_marks_bp->LTagL.Set(new_pos, &tag_list));
																	}
																}
																cc_id_list_to_mark.add(cc_pack.Rec.ID);
																wroff_qtty -= qtty_to_apply;
															}
														}
													}
												}
											}
										}
										else if(!v3markMode) {
											for(uint bci = 0; bci < bc_list.getCount(); bci++) {
												egais_code = bc_list.at(bci).Code;
												double ex_row_qtty = 0.0;
												double crest = 0.0;
												if(p_wroff_bp) {
													p_wroff_bp->LTagL.SearchString(egais_code, PPTAG_LOT_FSRARLOTGOODSCODE, 0, ex_row_list);
													for(uint eridx = 0; eridx < ex_row_list.getCount(); eridx++) {
														const PPTransferItem & r_ex_ti = p_wroff_bp->TI(ex_row_list.get(eridx));
														ex_row_qtty += r_ex_ti.Quantity_;
													}
												}
												else
													ex_row_list.Z();
												{
													pCurrentRestPack->LTagL.SearchString(egais_code, PPTAG_LOT_FSRARLOTGOODSCODE, 0, crest_row_list);
													for(uint cri = 0; cri < crest_row_list.getCount(); cri++) {
														const PPTransferItem & r_cr_ti = pCurrentRestPack->ConstTI(crest_row_list.get(cri));
														crest += r_cr_ti.Quantity_;
													}
													crest += ex_row_qtty;
												}
												if(ccl_rest != 0.0 && fsign(ccl_rest) == fsign(ccl_qtty)) {
													double wroff_qtty = 0.0;
													if(ccl_rest < 0.0)
														wroff_qtty = ccl_rest;
													// ЕГАИС не разрешает списание в минус else if(bci == (bc_list.getCount()-1)) wroff_qtty = ccl_rest;
													else if(crest > 0.0)
														wroff_qtty = MIN(ccl_rest, crest);
													if(wroff_qtty != 0.0) {
														if(!p_wroff_bp) {
															THROW_MEM(p_wroff_bp = new PPBillPacket);
															THROW(p_wroff_bp->CreateBlank2(wos_op_id, _cur_date, loc_id, 1));
															p_wroff_bp->BTagL.PutItemStr(PPTAG_BILL_FORMALREASON, PPLoadStringS(PPSTR_HASHTOKEN_C, PPHSC_RU_SELLING, temp_buf)); // "Реализация"
														}
														if(ex_row_list.getCount()) {
															const uint ex_pos = ex_row_list.get(0);
															PPTransferItem & r_ex_ti = p_wroff_bp->TI(ex_pos);
															r_ex_ti.Quantity_ -= wroff_qtty;
															if(use_lotxcode && egais_mark.NotEmpty())
																p_wroff_bp->XcL.Add(ex_pos+1, 0, 0, egais_mark, 0);
														}
														else {
															PPTransferItem ti;
															uint   new_pos = p_wroff_bp->GetTCount();
															THROW(ti.Init(&p_wroff_bp->Rec, 1));
															THROW(ti.SetupGoods(goods_id, 0));
															ti.Quantity_ = -wroff_qtty;
															THROW(p_wroff_bp->LoadTItem(&ti, 0, 0));
															{
																ObjTagList tag_list;
																tag_list.PutItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
																THROW(p_wroff_bp->LTagL.Set(new_pos, &tag_list));
															}
															if(use_lotxcode && egais_mark.NotEmpty())
																p_wroff_bp->XcL.Add(new_pos+1, 0, 0, egais_mark, 0);
														}
														ccl_rest -= wroff_qtty;
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
				if(Cfg.E.WrOffShopWay == Cfg.woswBalanceWithLots || do_balance_with_lots_unmarked_only) {
					// @v11.7.2 if(!v3markMode) {
						SString ref_a; // @v11.7.3 Справка А из отчета об остатках
						SString ref_b; // @v11.7.3 Справка Б из отчета об остатках
						SString lot_ref_b; // @v11.7.7 Справка Б из лота (для обора только тех лотов, которые соответствуют строке остатка)
						for(uint i = 0; i < pCurrentRestPack->GetTCount(); i++) {
							const PPTransferItem & r_ti = pCurrentRestPack->ConstTI(i);
							if(r_ti.Quantity_ > 0.0 && pCurrentRestPack->LTagL.GetTagStr(i, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code) > 0) {
								bool is_beer = false;
								assert(egais_code.NotEmpty());
								p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code, &lot_id_list);
								double current_lot_rest = 0.0;
								PrcssrAlcReport::GoodsItem agi;
								// @v11.7.2 {
								bool do_process = false;
								if(!v3markMode && !do_balance_with_lots_unmarked_only) { // @v11.7.12 (&& !do_balance_with_lots_unmarked_only)
									do_process = true;
								}
								else if(PreprocessGoodsItem(labs(r_ti.GoodsID), /*org_lot_id*/0, 0, 0, agi) > 0) {
									is_beer = LOGIC(PrcssrAlcReport::IsBeerCategoryCode(agi.CategoryCode));
									if(is_beer)
										do_process = true;
								}
								// } @v11.7.2 
								if(do_process) {
									pCurrentRestPack->LTagL.GetTagStr(i, PPTAG_LOT_FSRARINFA, ref_a); // @v11.7.3
									pCurrentRestPack->LTagL.GetTagStr(i, PPTAG_LOT_FSRARINFB, ref_b); // @v11.7.3
									double _last_cost = 0.0; // @v11.7.4
									double _last_price = 0.0; // @v11.7.4
									for(uint j = 0; j < lot_id_list.getCount(); j++) {
										const  PPID lot_id = lot_id_list.get(j);
										if(P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0 && lot_rec.LocID == loc_id) {
											// @v11.7.8 {
											bool do_reckon_lot_rest = false;
											if(v3markMode)
												do_reckon_lot_rest = (p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_FSRARINFB, lot_ref_b) && lot_ref_b.IsEqiAscii(ref_b));
											else
												do_reckon_lot_rest = true;
											// } @v11.7.8 
											if(do_reckon_lot_rest) { // @v11.7.7
												double _rest = 0.0;
												P_BObj->trfr->GetRest(lot_id, _cur_date, MAXLONG, &_rest, 0);
												current_lot_rest += _rest;
												//
												_last_cost = lot_rec.Cost; // @v11.7.4
												_last_price = lot_rec.Price; // @v11.7.4
											}
										}
									}
									const double wroff_qtty = (r_ti.Quantity_ - current_lot_rest);
									if(wroff_qtty > 0.0) {
										if(!p_wroff_bp) {
											THROW_MEM(p_wroff_bp = new PPBillPacket);
											THROW(p_wroff_bp->CreateBlank2(wos_op_id, _cur_date, loc_id, 1));
										}
										{
											PPTransferItem ti;
											const uint new_pos = p_wroff_bp->GetTCount();
											THROW(ti.Init(&p_wroff_bp->Rec, 1));
											THROW(ti.SetupGoods(r_ti.GoodsID, 0));
											ti.Quantity_ = wroff_qtty;
											ti.Cost  = _last_cost; // @v11.7.4
											ti.Price = _last_price; // @v11.7.4
											THROW(p_wroff_bp->LoadTItem(&ti, 0, 0));
											{
												ObjTagList tag_list;
												tag_list.PutItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
												// @v11.7.3 {
												if(is_beer) {
													tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, ref_a);
													tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFB, ref_b);
												}
												// } @v11.7.3 
												THROW(p_wroff_bp->LTagL.Set(new_pos, &tag_list));
											}
										}
									}
								}
							}
						}
					// @v11.7.2 }
				}
				{
					//p_wroff_wo_marks_bp
					PPBillPacket * pp_wroff_pack_list[2] = { p_wroff_bp, p_wroff_wo_marks_bp };
					for(uint wopidx = 0; wopidx < SIZEOFARRAY(pp_wroff_pack_list); wopidx++) {
						PPBillPacket * p_wop = pp_wroff_pack_list[wopidx];
						if(p_wop && p_wop->GetTCount()) {
							p_wop->InitAmounts();
							{
								PPTransaction tra(1);
								THROW(tra);
								THROW(P_BObj->TurnPacket(p_wop, 0));
								// @v11.8.2 {
								if(cc_id_list_to_mark.getCount()) {
									assert(p_wop->Rec.ID); // P_BObj->TurnPacket успешно отработала: стало быть проверяемое значение не нулевое
									assert(p_cc); // Иначе как мы cc_id_list_to_mark вычислили?
									if(p_cc && p_wop->Rec.ID) {
										cc_id_list_to_mark.sortAndUndup();
										temp_buf.Z().Cat(p_wop->Rec.ID);
										for(uint ccidx = 0; ccidx < cc_id_list_to_mark.getCount(); ccidx++) {
											const  PPID cc_id = cc_id_list_to_mark.get(ccidx);
											p_cc->UpdateExtText(cc_id, CCheckPacket::extssEgaisProcessingTag, temp_buf, 0);
										}
									}
								}
								// } @v11.8.2 
								THROW(tra.Commit());
							}
							PPObjBill::MakeCodeString(&p_wop->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
							LogTextWithAddendum(v3markMode ? PPTXT_EGAIS_WROFFMARKSCREATED : PPTXT_EGAIS_REG2WBCREATED, bill_text);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_wroff_bp);
	return ok;
}

int PPEgaisProcessor::Helper_CreateTransferToShop(const PPBillPacket * pCurrentRestPack)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  PPID loc_id = pCurrentRestPack ? pCurrentRestPack->Rec.LocID : 0;
	PPID   op_id = 0;
	SString temp_buf;
	SString fmt_buf;
	SString bill_text;
	PPObjOprKind op_obj;
	ObjTagItem tag_item;
	PPBillPacket * p_shop_rest_bp = 0;
	//
	// Формируем документ передачи в торговый зал (регистр 2) лотов, которых
	// нет в отчете ЕГАИС об остатках в торговом зале.
	//
	const LDATE _cur_date = getcurdate_();
    PPIDArray alco_goods_list;
    PPID   sco_op_id = 0;
    PPBillPacket current_wh_rest_bp;
    if(!op_obj.GetEdiShopChargeOnOp(&sco_op_id, 1))
		LogLastError();
	else if(!op_id && !op_obj.GetEdiStockOp(&op_id, 1))
		LogLastError();
	else if(op_id) {
		BillTbl::Rec bill_rec;
		DateRange period;
		period.Set(plusdate(_cur_date, -2), plusdate(_cur_date, +1));
		PPID   last_wh_rest_bill_id = 0;
		PPOprKind op_rec_r1;
		PPOprKind op_rec_r2;
		int    r1_can_have_main_org_ar = 0;
		PPID   target_ar2_id = 0; // Дополнительная статья, которая, если !0, должна совпадать у документа остатков по R2 и по R1.
		GetOpData(op_id, &op_rec_r1);
		GetOpData(pCurrentRestPack->Rec.OpID, &op_rec_r2);
		if(AcsObj.IsLinkedToMainOrg(op_rec_r1.AccSheet2ID))
			r1_can_have_main_org_ar = 1;
		if(r1_can_have_main_org_ar && pCurrentRestPack->Rec.Object2 && AcsObj.IsLinkedToMainOrg(op_rec_r2.AccSheet2ID))
			target_ar2_id = pCurrentRestPack->Rec.Object2;
		for(DateIter di(&period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
			if(oneof2(bill_rec.EdiOp, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2)) {
				if(!target_ar2_id || !bill_rec.Object2 || bill_rec.Object2 == target_ar2_id)
					last_wh_rest_bill_id = bill_rec.ID;
			}
		}
		if(last_wh_rest_bill_id) {
			if(P_BObj->ExtractPacketWithFlags(last_wh_rest_bill_id, &current_wh_rest_bp, BPLD_FORCESERIALS) > 0) {
				//PPTXT_EGAIS_LASTSTOCKBILLFOUND "Последний документ регистрации складских остатков в @{brand_egais}: '%s'"
				PPObjBill::MakeCodeString(&current_wh_rest_bp.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
				LogTextWithAddendum(PPTXT_EGAIS_LASTSTOCKBILLFOUND, temp_buf);
			}
			else {
				current_wh_rest_bp.Rec.ID = 0;
				LogLastError();
			}
		}
		else {
			temp_buf.Z().Cat(period, 1);
			LogTextWithAddendum(PPTXT_EGAIS_STOCKBILLNFOUNT, temp_buf);
		}
	}
    if(current_wh_rest_bp.Rec.ID && sco_op_id && GetAlcGoodsList(alco_goods_list) > 0) {
		int    dont_create_transfer = 0; // Если есть еще не принятый ЕГАИС документ передачи, то новый документ не заводим
		PPIDArray settled_bill_list; // Список документов передачи, принятых в ЕГАИС
		SString settled_edi_ack;
		BillTbl::Rec sbill_rec;
		{
			DateRange settled_period;
			settled_period.Set(encodedate(1, 9, 2016), ZERODATE);
			for(SEnum en = P_BObj->P_Tbl->EnumByOp(sco_op_id, &settled_period, 0); en.Next(&sbill_rec) > 0;) {
				if(p_ref->Ot.GetTagStr(PPOBJ_BILL, sbill_rec.ID, PPTAG_BILL_EDIACK, settled_edi_ack) > 0) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, sbill_rec.ID, PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
						THROW_SL(settled_bill_list.add(sbill_rec.ID));
					}
					else {
						PPObjBill::MakeCodeString(&sbill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
						LogTextWithAddendum(PPTXT_EGAIS_UNSETTLEDSHOPTRFR, temp_buf);
						dont_create_transfer = 1;
					}
				}
			}
		}
		if(!dont_create_transfer) {
			StringSet refb_charged_list; // @v11.0.3 Список справок Б, соответствующих маркам, поставленным на баланс операцией PPOPK_EDI_CHARGEONWITHMARKS
			settled_bill_list.sortAndUndup();
			SString ref_a, ref_b, egais_code;
			SString goods_name;
			LongArray row_idx_list;
			THROW_MEM(SETIFZ(P_LecT, new LotExtCodeCore));
			// @v11.0.3 {
			{
				DateRange mark_charge_list_period;	
				mark_charge_list_period.Set(encodedate(1, 9, 2020), ZERODATE); // Дата соответствует дате ввода вида операции PPOPK_EDI_CHARGEONWITHMARKS в код системы
				PPBillPacket mark_charge_bpack;
				for(SEnum en = P_BObj->P_Tbl->EnumByOp(PPOPK_EDI_CHARGEONWITHMARKS, &mark_charge_list_period, 0); en.Next(&sbill_rec) > 0;) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, sbill_rec.ID, PPTAG_BILL_EDIACK, settled_edi_ack) > 0) {
						if(p_ref->Ot.GetTagStr(PPOBJ_BILL, sbill_rec.ID, PPTAG_BILL_EDIIDENT, temp_buf) > 0) {					
							if(P_BObj->ExtractPacket(sbill_rec.ID, &mark_charge_bpack) > 0) {
								for(uint rn = 0; rn < mark_charge_bpack.GetTCount(); rn++) {
									if(mark_charge_bpack.LTagL.GetString(PPTAG_LOT_FSRARINFB, rn, temp_buf) > 0)
										refb_charged_list.add(temp_buf);
								}
							}
						}
					}
				}
				refb_charged_list.sortAndUndup();
			}
			// } @v11.0.3 
			{
				PPIDArray ref_b_lot_list; // Список лотов со справкой Б той же, что и в строке текущих остатков по складу
				LongArray processed_csr_rows; // Список индексов строк pCurrentRestPack, которые были затронуты созданным документом
				PrcssrAlcReport::GoodsItem agi;
				long   checked_counter = 0;
				StrAssocArray checked_list; // Список егаис-кодов, которые уже встречались
				RAssocArray checked_qtty_list; // Список учтенных остатков по егаис кодам
				//
				PPTransaction tra(1);
				THROW(tra);
				for(uint cwridx = 0; cwridx < current_wh_rest_bp.GetTCount(); cwridx++) {
					const PPTransferItem & r_cwr_item = current_wh_rest_bp.ConstTI(cwridx);
					current_wh_rest_bp.LTagL.GetTagStr(cwridx, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
					current_wh_rest_bp.LTagL.GetTagStr(cwridx, PPTAG_LOT_FSRARINFB, ref_b);
					if(egais_code.NotEmpty() && ref_b.NotEmpty() /*&& PreprocessGoodsItem(labs(r_cwr_item.GoodsID), 0, 0, 0, agi) > 0*/) {
						if(refb_charged_list.searchNcAscii(ref_b, 0)) { // @v11.0.3
							// Если справка Б найдена среди строк документа постановки марок на баланс, но 
							// строку с этой справкой на регистр 2 отправлять нельзя.
							// PPTXT_EGAIS_WHRESTINMCHARGE "Строка остатка по складу @{brand_egais} '%s' имеет справку Б, поставленную на баланс с марками: на регистр 2 не передается."
							temp_buf.Z().Cat(egais_code).CatDiv('-', 1).Cat(ref_b);
							LogTextWithAddendum(PPTXT_EGAIS_WHRESTINMCHARGE, temp_buf);
						}
						else {
							Transfer * p_trfr = P_BObj->trfr;
							const double cwr_rest = r_cwr_item.Quantity_;
							double shop_rest = 0.0;
							PPID   lot_id = 0;
							bool   is_lot_in_3format = false;
							bool   is_beer = false; // @v11.2.0
							ReceiptTbl::Rec lot_rec;
							ref_b_lot_list.clear();
							p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_FSRARINFB, ref_b, &ref_b_lot_list);
							for(uint llidx = 0; llidx < ref_b_lot_list.getCount(); llidx++) {
								const  PPID temp_lot_id = ref_b_lot_list.get(llidx);
								PPID   org_lot_id = 0;
								// @todo Вероятно, надо искать лот по любому складу
								if(p_trfr->Rcpt.Search(temp_lot_id, &lot_rec) > 0 && lot_rec.LocID == loc_id && IsAlcGoods(lot_rec.GoodsID)) {
									if(p_trfr->Rcpt.SearchOrigin(lot_rec.ID, &org_lot_id, 0, 0) && PreprocessGoodsItem(lot_rec.GoodsID, org_lot_id, 0, 0, agi) > 0) {
										is_beer = LOGIC(PrcssrAlcReport::IsBeerCategoryCode(agi.CategoryCode)); // @v11.2.0
										const  PPID lot_bill_id = lot_rec.BillID;
										TransferTbl::Rec trfr_rec;
										for(DateIter di; p_trfr->EnumByLot(temp_lot_id, &di, &trfr_rec) > 0;) {
											if(trfr_rec.BillID == lot_bill_id) {
												int16 row_idx = 0;
												int   row_is_found = 0;
												for(int   rbb_iter = 0; !row_is_found && p_trfr->EnumItems(lot_bill_id, &rbb_iter, 0) > 0;) {
													row_idx++;
													if(rbb_iter == trfr_rec.RByBill)
														row_is_found = 1;
												}
												if(row_is_found) {
													LotExtCodeTbl::Key2 k2;
													MEMSZERO(k2);
													k2.BillID = lot_bill_id;
													k2.RByBill = /*rbb*/row_idx;
													if(P_LecT->search(2, &k2, spGe) && P_LecT->data.BillID == lot_bill_id && P_LecT->data.RByBill == row_idx) do {
														if(P_LecT->data.Code[0])
															is_lot_in_3format = 1;
													} while(!is_lot_in_3format && P_LecT->search(2, &k2, spNext) && P_LecT->data.BillID == lot_bill_id && P_LecT->data.RByBill == row_idx);
													break;
												}
											}
										}
										lot_id = lot_rec.ID;
										break;
									}
								}
							}
							{
								LongArray csr_row_idx_list;
								const bool cscr = pCurrentRestPack->LTagL.SearchString(egais_code, PPTAG_LOT_FSRARLOTGOODSCODE, 0, csr_row_idx_list);
								if(cscr) {
									for(uint s = 0; s < csr_row_idx_list.getCount(); s++) {
										const long csr_idx = csr_row_idx_list.get(s);
										if(csr_idx >= 0 && csr_idx < pCurrentRestPack->GetTCountI()) {
											const PPTransferItem & r_csr_ti = pCurrentRestPack->ConstTI(csr_idx);
											THROW_SL(processed_csr_rows.add(csr_idx));
											shop_rest += r_csr_ti.Quantity_;
										}
									}
								}
								{
									uint   clp = 0;
									if(checked_list.SearchByText(egais_code, PTR_CMPFUNC(PcharNoCase), &clp)) {
										const long clid = checked_list.Get(clp).Id;
										const double checked_qtty = checked_qtty_list.Get(clid, 0);
										shop_rest += checked_qtty;
									}
								}
							}
							//
							if(lot_id == 0) {
								// PPTXT_EGAIS_NOLOTSFORWHREST    "Для остатка по складу ЕГАИС '%s' не найдено ни одного соответствия в лотах"
								temp_buf.Z().Cat(egais_code).CatDiv('-', 1).Cat(ref_b).Space().Eq().Cat(cwr_rest, MKSFMTD(0, 1, 0));
								LogTextWithAddendum(PPTXT_EGAIS_NOLOTSFORWHREST, temp_buf);
							}
							else if(!is_beer) { // @v11.2.0 Начиная с этого релиза передается только пиво
								//PPTXT_EGAIS_CHARGEONSHOPBEERONLY
								temp_buf.Z().Cat(egais_code).CatDiv('-', 1).Cat(ref_b).Space().Eq().Cat(cwr_rest, MKSFMTD(0, 1, 0));
								LogTextWithAddendum(PPTXT_EGAIS_CHARGEONSHOPBEERONLY, temp_buf);
							}
							else if(is_lot_in_3format) {
								//PPTXT_EGAIS_LOTFORWHRESTIN3F
								temp_buf.Z().Cat(egais_code).CatDiv('-', 1).Cat(ref_b).Space().Eq().Cat(cwr_rest, MKSFMTD(0, 1, 0));
								LogTextWithAddendum(PPTXT_EGAIS_LOTFORWHRESTIN3F, temp_buf);
							}
							else if(cwr_rest > 0.0) {
								double transfer_qtty = 0.0;
								if(Cfg.E.Flags & PrcssrAlcReport::Config::fWhToReg2ByLacks) {
									if(shop_rest < 0.0)
										transfer_qtty = MIN(-shop_rest, cwr_rest);
								}
								else
									transfer_qtty = cwr_rest;
								if(transfer_qtty >= 0.01) {
									if(p_shop_rest_bp && p_shop_rest_bp->GetTCount() >= 100) {
										p_shop_rest_bp->InitAmounts();
										THROW(P_BObj->TurnPacket(p_shop_rest_bp, 0));
										PPObjBill::MakeCodeString(&p_shop_rest_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
										LogTextWithAddendum(PPTXT_EGAIS_REG2TBCREATED, bill_text);
										ZDELETE(p_shop_rest_bp);
									}
									if(!p_shop_rest_bp) {
										THROW_MEM(p_shop_rest_bp = new PPBillPacket);
										THROW(p_shop_rest_bp->CreateBlank2(sco_op_id, _cur_date, loc_id, 0));
									}
									{
										PPTransferItem ti;
										const uint new_pos = p_shop_rest_bp->GetTCount();
										THROW(ti.Init(&p_shop_rest_bp->Rec, 1));
										THROW(ti.SetupGoods(lot_rec.GoodsID, 0));
										ti.Quantity_ = transfer_qtty;
										ti.Cost = lot_rec.Cost;
										ti.Price = lot_rec.Price;
										THROW(p_shop_rest_bp->LoadTItem(&ti, 0, 0));
										{
											uint   clp = 0;
											if(!checked_list.SearchByText(egais_code, PTR_CMPFUNC(PcharNoCase), &clp)) {
												checked_counter++;
												THROW_SL(checked_list.Add(checked_counter, egais_code, 0));
												clp = 0;
												THROW_SL(checked_list.SearchByText(egais_code, PTR_CMPFUNC(PcharNoCase), &clp));
											}
											const long clid = checked_list.Get(clp).Id;
											THROW_SL(checked_qtty_list.Add(clid, transfer_qtty, 1, 0));
										}
										{
											ObjTagList tag_list;
											tag_list.PutItemStrNE(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
											assert(ref_b.NotEmpty()); // @paranoic
											tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFB, ref_b);
											tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, ref_a);
											THROW(p_shop_rest_bp->LTagL.Set(new_pos, &tag_list));
										}
									}
								}
							}
							else if(shop_rest < 0.0) {
								//
								// PPTXT_EGAIS_NOWHRESTFORR2DFCT  "Для отрицательного остатка по регистру 2 @{brand_egais} '%s' нет доступного остатка по складу @{brand_egais}"
								temp_buf.Z().Cat(egais_code).CatDiv('-', 1).Cat(ref_b).Space().Eq().Cat(shop_rest, MKSFMTD(0, 1, 0));
								LogTextWithAddendum(PPTXT_EGAIS_NOWHRESTFORR2DFCT, temp_buf);
							}
						}
					}
				}
				for(uint csridx = 0; csridx < pCurrentRestPack->GetTCount(); csridx++) {
					if(!processed_csr_rows.lsearch(static_cast<long>(csridx))) {
						const PPTransferItem & r_csr_ti = pCurrentRestPack->ConstTI(csridx);
						if(r_csr_ti.Quantity_ < 0.0) {
							pCurrentRestPack->LTagL.GetTagStr(csridx, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
							// PPTXT_EGAIS_NOWHRESTFORR2DFCT  "Для отрицательного остатка по регистру 2 @{brand_egais} '%s' нет доступного остатка по складу @{brand_egais}"
							temp_buf.Z().Cat(egais_code).Space().Eq().Cat(r_csr_ti.Quantity_, MKSFMTD(0, 1, 0));
							LogTextWithAddendum(PPTXT_EGAIS_NOWHRESTFORR2DFCT, temp_buf);
						}
					}
				}
				if(p_shop_rest_bp && p_shop_rest_bp->GetTCount()) {
					p_shop_rest_bp->InitAmounts();
					THROW(P_BObj->TurnPacket(p_shop_rest_bp, 0));
					PPObjBill::MakeCodeString(&p_shop_rest_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
					LogTextWithAddendum(PPTXT_EGAIS_REG2TBCREATED, bill_text);
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	ZDELETE(p_shop_rest_bp);
	return ok;
}

int PPEgaisProcessor::Read_Rests(xmlNode * pFirstNode, PPID locID, const DateRange * pPeriod, Packet * pPack, PrcssrAlcReport::RefCollection * pRefC)
{
    int    ok = 1;
    int    was_header_accepted = 0;
    int    is_pack_inited = 0;
    int    wb_type = 0;
    PPID   op_id = 0;
    const  PPID   loc_id = NZOR(locID, LConfig.Location);
	DateRange period;
	BillTbl::Rec bill_rec;
    LDATETIME rest_dtm = ZERODATETIME;
	PPBillPacket * p_bp = pPack ? static_cast<PPBillPacket *>(pPack->P_Data) : 0;
	PPBillPacket * p_shop_rest_bp = 0;
    SString temp_buf;
	PPGoodsPacket goods_pack;
	GoodsItem alc_ext;
    BillTbl::Rec bhdr;
    TSVector <EgaisRestItem> items;
	const  PPID manuf_tag_id = Cfg.LotManufTagList.getCount() ? Cfg.LotManufTagList.get(0) : 0;
    for(const xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
        if(SXml::GetContentByName(p_n, "RestsDate", temp_buf)) {
			strtodatetime(temp_buf, &rest_dtm, DATF_ISO8601, TIMF_HMS);
		}
		else if(SXml::IsName(p_n, "Products")) {
			for(const xmlNode * p_c = p_n->children; ok > 0 && p_c; p_c = p_c->next) {
				if(SXml::IsName(p_c, "StockPosition") || SXml::IsName(p_c, "ShopPosition")) {
					alc_ext.Z();
					EgaisRestItem rest_item;
					int    product_refc_pos = -1;
					for(const xmlNode * p_pos = p_c->children; ok > 0 && p_pos; p_pos = p_pos->next) {
						if(SXml::GetContentByName(p_pos, "Quantity", temp_buf))
							rest_item.Rest = temp_buf.ToReal();
						else if(SXml::GetContentByName(p_pos, "InformARegId", temp_buf) || SXml::GetContentByName(p_pos, "InformF1RegId", temp_buf))
							temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(rest_item.InformAIdent, sizeof(rest_item.InformAIdent));
						else if(SXml::GetContentByName(p_pos, "InformBRegId", temp_buf) || SXml::GetContentByName(p_pos, "InformF2RegId", temp_buf))
							temp_buf.Strip().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(rest_item.InformBIdent, sizeof(rest_item.InformBIdent));
						else if(SXml::IsName(p_pos, "Product")) {
							int   rs = 0;
							THROW(rs = Read_ProductInfo(p_pos->children, p_bp ? &goods_pack : 0, &alc_ext, pRefC, 0));
							if(pRefC && pRefC->LastProductP > 0) {
								product_refc_pos = pRefC->LastProductP;
							}
							if(rs > 0) {
								rest_item.GoodsID = goods_pack.Rec.ID;
								rest_item.MnfOrImpPsnID = alc_ext.MnfOrImpPsnID;
								STRNSCPY(rest_item.EgaisCode, alc_ext.EgaisCode);
							}
						}
					}
					if(pRefC) {
						EgaisRefATbl::Rec refai;
						STRNSCPY(refai.RefACode, rest_item.InformAIdent);
						if(product_refc_pos >= 0) {
							const EgaisProductCore::Item * p_product = pRefC->ProductList.at(product_refc_pos);
							if(p_product) {
								STRNSCPY(refai.AlcCode, p_product->AlcoCode);
								STRNSCPY(refai.ManufRarIdent, p_product->ManufRarIdent);
								STRNSCPY(refai.ImporterRarIdent, p_product->ImporterRarIdent);
								refai.Volume = static_cast<int32>(p_product->Volume * 100000.0);
								THROW(pRefC->SetRefA(refai));
							}
						}
					}
					THROW_SL(items.insert(&rest_item));
				}
			}
		}
    }
    if(p_bp && checkdate(rest_dtm.d)) {
		SString bill_text;
		int    do_skip = 0;
		PPID   op_id = 0;
		PPObjOprKind op_obj;
		PPOprKind op_rec;
		ObjTagItem tag_item;
		const char * p_code_suffix = 0;
		if(pPack->DocType == PPEDIOP_EGAIS_REPLYRESTSSHOP)
			p_code_suffix = "-R2";
		else if(oneof2(pPack->DocType, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2))
			p_code_suffix = "-R1";
		THROW(op_obj.GetEdiStockOp(&op_id, 1));
		THROW(GetOpData(op_id, &op_rec) > 0);
        {
			period.SetDate(rest_dtm.d);
        	for(DateIter di(&period); !do_skip && P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if(bill_rec.LocID == loc_id && (!p_code_suffix || (temp_buf = bill_rec.Code).CmpSuffix(p_code_suffix, 0) == 0)) { // @v11.7.7 (bill_rec.LocID == loc_id &&)
					LDATETIME ts;
					if(PPRef->Ot.GetTag(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_CREATEDTM, &tag_item) > 0) {
						if(tag_item.GetTimestamp(&ts) > 0 && diffdatetimesec(ts, rest_dtm) >= 0)
							do_skip = 1;
					}
				}
			}
        }
        if(!do_skip) {
			PPTransaction tra(1);
			THROW(tra);
			THROW(p_bp->CreateBlank2(op_id, rest_dtm.d, loc_id, 0));
			p_bp->Rec.EdiOp = pPack->DocType;
			if(p_code_suffix) {
				(temp_buf = p_bp->Rec.Code).Cat(p_code_suffix);
				STRNSCPY(p_bp->Rec.Code, temp_buf);
			}
			if(P_UtmEntry && P_UtmEntry->MainOrgID && AcsObj.IsLinkedToMainOrg(op_rec.AccSheet2ID)) {
				PPID   ar2_id = 0;
				ArObj.P_Tbl->PersonToArticle(P_UtmEntry->MainOrgID, op_rec.AccSheet2ID, &ar2_id);
				p_bp->SetupObject2(ar2_id);
			}
			{
				tag_item.SetTimestamp(PPTAG_BILL_CREATEDTM, rest_dtm);
				p_bp->BTagL.PutItem(PPTAG_BILL_CREATEDTM, &tag_item);
			}
			for(uint i = 0; i < items.getCount(); i++) {
				const EgaisRestItem & r_item = items.at(i);
				if(r_item.GoodsID) {
					PPTransferItem ti;
					const uint new_pos = p_bp->GetTCount();
					THROW(ti.Init(&p_bp->Rec, 1));
					THROW(ti.SetupGoods(r_item.GoodsID, 0));
					{
						GoodsItem _agi;
						PreprocessGoodsItem(ti.GoodsID, 0, 0, 0, _agi);
						if(_agi.UnpackedVolume > 0.0) {
							const double mult = _agi.UnpackedVolume / 10.0;
							ti.Quantity_ = R6(r_item.Rest / mult);
						}
						else {
							ti.Quantity_ = r_item.Rest;
						}
					}
					THROW(p_bp->LoadTItem(&ti, 0, 0));
					{
						ObjTagList tag_list;
						tag_list.PutItemStrNE(PPTAG_LOT_FSRARLOTGOODSCODE, r_item.EgaisCode);
						tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, r_item.InformAIdent);
						tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFB, r_item.InformBIdent);
						if(r_item.MnfOrImpPsnID && manuf_tag_id) {
							tag_item.SetInt(manuf_tag_id, r_item.MnfOrImpPsnID);
							tag_list.PutItem(manuf_tag_id, &tag_item);
						}
						THROW(p_bp->LTagL.Set(new_pos, &tag_list));
					}
				}
			}
			p_bp->InitAmounts();
			THROW(P_BObj->TurnPacket(p_bp, 0));
			pPack->Flags |= Packet::fAcceptedBill;
			THROW(tra.Commit());
			PPObjBill::MakeCodeString(&p_bp->Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
			LogTextWithAddendum(PPTXT_EGAIS_RESTSACCEPTED, bill_text);
		}
    }
	CATCHZOK
	ZDELETE(p_shop_rest_bp);
    return ok;
}

int PPEgaisProcessor::Write(Packet & rPack, PPID locID, const char * pFileName)
{
	int    ok = -1;
    xmlTextWriter * p_x = xmlNewTextWriterFilename(pFileName, 0);
    THROW_SL(p_x);
    THROW(ok = Helper_Write(rPack, locID, p_x));
    CATCHZOK
    xmlFreeTextWriter(p_x);
    return ok;
}

int PPEgaisProcessor::Write(Packet & rPack, PPID locID, SBuffer & rBuffer)
{
	int    ok = -1;
	xmlTextWriter * p_x = 0;
	xmlBuffer * p_x_buf = xmlBufferCreate();
	THROW(p_x_buf);
    THROW(p_x = xmlNewTextWriterMemory(p_x_buf, 0));
    THROW(ok = Helper_Write(rPack, locID, p_x));
	xmlTextWriterFlush(p_x);
	THROW_SL(rBuffer.Write(p_x_buf->content, p_x_buf->use));
    CATCHZOK
    xmlFreeTextWriter(p_x);
    xmlBufferFree(p_x_buf);
    return ok;
}

int PPEgaisProcessor::AcceptDoc(PPEgaisProcessor::Reply & rR, const char * pFileName)
{
    int    ok = 1;
    SString file_name;
    if(!isempty(pFileName))
		file_name = pFileName;
	else {
		THROW(GetTemporaryFileName(0, 0, "EQD", file_name));
	}
	{
		const  int max_try = 3;
		int    try_no = 0;
		int    try_ok = 0;
		do {
			if(try_no)
				SDelay(1000);
			SFile wr_stream(file_name, SFile::mWrite);
			ScURL c;
			try_ok = c.HttpGet(rR.Url, 0, &wr_stream);
			try_no++;
		} while(!try_ok && (try_no < max_try));
		THROW_SL(try_ok);
	}
	rR.AcceptedFileName = file_name;
    CATCHZOK
    return ok;
}

int PPEgaisProcessor::DeleteDoc(PPEgaisProcessor::Reply & rR)
{
    int    ok = 1;
    if(!(rR.Status & Reply::stOffline)) {
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		ScURL c;
		THROW_SL(c.HttpDelete(rR.Url, 0, &wr_stream));
    }
	rR.Status |= Reply::stDeleted;
	// (лишнее сообщение - мешает) LogTextWithAddendum(PPTXT_EGAIS_DOCDELETED, rR.Url);
    CATCHZOK
    return ok;
}

int PPEgaisProcessor::Helper_InitNewPack(const int docType, TSCollection <PPEgaisProcessor::Packet> * pPackList, PPEgaisProcessor::Packet ** ppPack)
{
	int    ok = 1;
	PPEgaisProcessor::Packet * p_new_pack = 0;
	if(pPackList) {
		THROW_MEM(p_new_pack = new Packet(docType));
	}
	ASSIGN_PTR(ppPack, p_new_pack);
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Helper_FinalizeNewPack(PPEgaisProcessor::Packet ** ppNewPack, uint srcReplyPos, TSCollection <PPEgaisProcessor::Packet> * pPackList)
{
	int    ok = 1;
	if(pPackList && ppNewPack && *ppNewPack) {
		(*ppNewPack)->SrcReplyPos = srcReplyPos;
		THROW_SL(pPackList->insert(*ppNewPack));
		(*ppNewPack) = 0;
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Helper_Read(void * pCtx, const char * pFileName, long flags, PPID locID, const DateRange * pPeriod,
	uint srcReplyPos, TSCollection <PPEgaisProcessor::Packet> * pPackList, PrcssrAlcReport::RefCollection * pRefC)
{
	long   file_no = 0;
	{
		SFsPath ps(pFileName);
		file_no = ps.Nam.ToLong();
	}
	int    ok = -1;
	xmlParserCtxt * p_ctx = static_cast<xmlParserCtxt *>(pCtx);
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	Packet * p_new_pack = 0;
	SString line_buf;
	SString out_file_name;
	SString temp_buf;
	SString fsrar_id;
	GetFSRARID(locID, fsrar_id, 0);
	THROW_SL(fileExists(pFileName));
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "Documents")) {
		for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "Owner")) {

			}
			else if(SXml::IsName(p_n, "Document")) {
				for(const xmlNode * p_nd = p_n->children; p_nd; p_nd = p_nd->next) {
					const int doc_type = RecognizeDocTypeTag(reinterpret_cast<const char *>(p_nd->name));
					int    rs = 0;
					if(oneof4(doc_type, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_WayBill(p_nd->children, locID, pPeriod, p_new_pack, pRefC));
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(oneof4(doc_type, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4)) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_WayBillAct(p_nd->children, locID, p_new_pack));
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_REQUESTREPEALWB) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						{
							RepealWb * p_rwb = static_cast<RepealWb *>(p_new_pack->P_Data);
							for(const xmlNode * p_n = p_nd->children; p_n; p_n = p_n->next) {
								if(SXml::GetContentByName(p_n, "ClientId", temp_buf) > 0)
									p_rwb->ContragentCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								else if(SXml::GetContentByName(p_n, "RequestNumber", temp_buf) > 0)
									p_rwb->ReqNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								else if(SXml::GetContentByName(p_n, "RequestDate", temp_buf) > 0)
									strtodatetime(temp_buf, &p_rwb->ReqTime, DATF_ISO8601, TIMF_HMS);
								else if(SXml::GetContentByName(p_n, "WBRegId", temp_buf) > 0)
									p_rwb->TTNCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							}
							rs = 1;
						}
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(oneof3(doc_type, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2, PPEDIOP_EGAIS_REPLYRESTSSHOP)) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_Rests(p_nd->children, locID, pPeriod, p_new_pack, pRefC));
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(oneof2(doc_type, PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG, PPEDIOP_EGAIS_ACTINVENTORYINFORMF2REG)) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_ActInventoryInformBReg(p_nd->children, p_new_pack));
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(oneof2(doc_type, PPEDIOP_EGAIS_TTNINFORMBREG, PPEDIOP_EGAIS_TTNINFORMF2REG)) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_TTNIformBReg(p_nd->children, p_new_pack));
						if(rs > 0) {
							const InformB * p_data = static_cast<const InformB *>(p_new_pack->P_Data);
							if(!pPeriod || pPeriod->CheckDate(p_data->OuterDate)) {
								THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							}
							else {
								ZDELETE(p_new_pack);
							}
							ok = 1;
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_TICKET) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(rs = Read_Ticket(p_nd->children, p_new_pack));
						if(rs > 0) {
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(oneof2(doc_type, PPEDIOP_EGAIS_REPLYFORMA, PPEDIOP_EGAIS_REPLYFORM1)) { // @v11.3.4 PPEDIOP_EGAIS_REPLYFORM1
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						THROW(Read_IformA(p_nd->children, p_new_pack, pRefC));
						THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
					}
					else if(doc_type == PPEDIOP_EGAIS_REPLYCLIENT) {
						const  PPID sell_pk_id = GetSellPersonKind();
						if(sell_pk_id) {
							THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
							{
								for(const xmlNode * p_n = p_nd->children; p_n; p_n = p_n->next) {
									if(SXml::IsName(p_n, "Clients")) {
										PPGetFilePath(PPPATH_OUT, "egais-clients", out_file_name);
										SFile out_file(out_file_name, SFile::mAppend);
										for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
											if(SXml::IsName(p_c, "Client")) {
												PPPersonPacket * p_new_psn_pack = 0;
												if(p_new_pack) {
													THROW_MEM(p_new_psn_pack = new PPPersonPacket);
												}
												Read_OrgInfo(p_c->children, sell_pk_id, EgaisPersonCore::rolefConsignee|EgaisPersonCore::rolefVerified,
													p_new_psn_pack, pRefC, out_file.IsValid() ? &out_file : 0);
												if(p_new_psn_pack)
													static_cast<TSCollection <PPPersonPacket> *>(p_new_pack->P_Data)->insert(p_new_psn_pack);
											}
										}
									}
								}
							}
							THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
							ok = 1;
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_REPLYAP) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						{
							for(const xmlNode * p_n = p_nd->children; p_n; p_n = p_n->next) {
								if(SXml::IsName(p_n, "Products")) {
									PPGetFilePath(PPPATH_OUT, "egais-products", out_file_name);
									SFile out_file(out_file_name, SFile::mAppend);
									for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
										if(SXml::IsName(p_c, "Product")) {
											PrcssrAlcReport::GoodsItem ext;
											PPGoodsPacket * p_new_goods_pack = 0;
											if(p_new_pack) {
												THROW_MEM(p_new_goods_pack = new PPGoodsPacket);
											}
											Read_ProductInfo(p_c->children, p_new_goods_pack, &ext, pRefC, out_file.IsValid() ? &out_file : 0);
											if(p_new_goods_pack)
												static_cast<TSCollection <PPGoodsPacket> *>(p_new_pack->P_Data)->insert(p_new_goods_pack);
										}
									}
								}
							}
						}
						THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
						ok = 1;
					}
					else if(doc_type == PPEDIOP_EGAIS_REPLYRESTBCODE) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						{
							ReplyRestBCode * p_rrbc = static_cast<ReplyRestBCode *>(p_new_pack->P_Data);
							for(const xmlNode * p_n = p_nd->children; p_n; p_n = p_n->next) {
								if(SXml::GetContentByName(p_n, "RestsDate", temp_buf)) {
									strtodatetime(temp_buf, &p_rrbc->RestTime, DATF_ISO8601CENT, TIMF_HMS);
								}
								else if(SXml::GetContentByName(p_n, "Inform2RegId", temp_buf)) {
									p_rrbc->Inform2RegId = temp_buf;
								}
								else if(SXml::IsName(p_n, "MarkInfo")) {
									for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
										if(SXml::GetContentByName(p_c, "amc", temp_buf)) {
											p_rrbc->MarkSet.add(temp_buf);
										}
									}
								}
							}
						}
					}
					else if(doc_type == PPEDIOP_EGAIS_REPLYBARCODE) {
						THROW(Helper_InitNewPack(doc_type, pPackList, &p_new_pack));
						{
                        	TSCollection <QueryBarcode> * p_qbl = static_cast<TSCollection <QueryBarcode> *>(p_new_pack->P_Data);
                        	for(const xmlNode * p_n = p_nd->children; p_n; p_n = p_n->next) {
                        		if(SXml::IsName(p_n, "Marks")) {
									PPGetFilePath(PPPATH_OUT, "egais-marks", out_file_name);
									const bool is_out_empty = !fileExists(out_file_name);
									SFile out_file(out_file_name, SFile::mAppend);
									if(is_out_empty) {
										line_buf.Z().Cat("Type").Tab().Cat("Rank").Tab().Cat("Number").Tab().Cat("Barcode").CR();
										out_file.WriteLine(line_buf);
									}
                        			for(const xmlNode * p_c = p_n->children; p_c; p_c = p_c->next) {
										if(SXml::IsName(p_c, "Mark")) {
											QueryBarcode * p_qb = p_qbl->CreateNewItem();
											THROW_SL(p_qb);
											for(const xmlNode * p_m = p_c->children; p_m; p_m = p_m->next) {
												if(SXml::GetContentByName(p_m, "Type", temp_buf))
													p_qb->CodeType = temp_buf.ToLong();
												else if(SXml::GetContentByName(p_m, "Rank", temp_buf))
													p_qb->Rank = temp_buf;
												else if(SXml::GetContentByName(p_m, "Number", temp_buf))
													p_qb->Number = temp_buf;
												else if(SXml::GetContentByName(p_m, "Barcode", temp_buf))
													p_qb->Result = temp_buf;
											}
											line_buf.Z().Cat(p_qb->CodeType).Tab().Cat(p_qb->Rank).Tab().Cat(p_qb->Number).Tab().Cat(p_qb->Result).CR();
											out_file.WriteLine(line_buf);
											{
												PPBarcode::BarcodeImageParam bip;
												bip.Code = p_qb->Result;
												bip.Std = BARCSTD_PDF417;
												bip.OutputFormat = SFileFormat::Png;
												temp_buf.Z();
                                                if(p_qb->Rank.NotEmpty())
													temp_buf.CatDivIfNotEmpty('-', 0).Cat(p_qb->Rank);
												if(p_qb->Number.NotEmpty())
													temp_buf.CatDivIfNotEmpty('-', 0).Cat(p_qb->Number);
												temp_buf.CatDivIfNotEmpty('-', 0).Cat(p_qb->Result);
                                                PPGetFilePath(PPPATH_OUT, temp_buf, bip.OutputFileName);
												PPBarcode::CreateImage(bip);
											}
										}
                        			}
                        		}
                        	}
                        }
						THROW(Helper_FinalizeNewPack(&p_new_pack, srcReplyPos, pPackList));
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	delete p_new_pack;
	return ok;
}

int PPEgaisProcessor::GetDebugPath(PPID locID, SString & rPath)
{
	rPath.Z();
    int    ok = 1;
    SString temp_buf;
    SString temp_path;
	THROW(GetFSRARID(locID, temp_buf, 0));
    PPGetPath(PPPATH_TEMP, temp_path);
    temp_path.SetLastSlash().Cat("EGAIS").CatChar('-').Cat(temp_buf);
    THROW_SL(SFile::CreateDir(temp_path));
	rPath = temp_path;
    CATCHZOK
    return ok;
}

int PPEgaisProcessor::MakeOutputFileName(const Reply * pReply, const SString & rTempPath, SString & rFileName)
{
	rFileName.Z();
	int    ok = 1;
	SString temp_buf;
	InetUrl _up(pReply->Url);
	if(_up.GetProtocol() != InetUrl::protFile) { // @v11.7.7
		_up.GetComponent(InetUrl::cPath, 0, temp_buf);
	}
	(rFileName = rTempPath).SetLastSlash().Cat(temp_buf);
	SFsPath ps(rFileName);
	(temp_buf = ps.Nam).DotCat("xml");
	ps.Merge(SFsPath::fDrv|SFsPath::fDir, rFileName);
	THROW_SL(SFile::CreateDir(rFileName));
	rFileName.SetLastSlash().Cat(temp_buf);
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::DebugReadInput(PPID locID)
{
    int    ok = 1;
    xmlParserCtxt * p_ctx = 0;
    SString temp_buf;
    SString temp_path;
	SString line_buf;
	SString url;
	THROW(GetURL(locID, url));
	Log((line_buf = "URL").CatDiv(':', 2).Cat(url));
	THROW(GetDebugPath(locID, temp_path));
    {
		TSCollection <PPEgaisProcessor::Reply> reply_list;
		THROW(p_ctx = xmlNewParserCtxt());
		THROW(GetReplyList(p_ctx, locID, +1, reply_list));
		for(uint i = 0; i < reply_list.getCount(); i++) {
			Reply * p_reply = reply_list.at(i);
			if(p_reply) {
				p_reply->Id.ToStr(S_GUID::fmtIDL, temp_buf);
				(line_buf = "Reply").CatDiv(':', 2).Cat(p_reply->Url).CatChar(' ').Cat(temp_buf);
				Log(line_buf);
				{
					THROW(MakeOutputFileName(p_reply, temp_path, line_buf));
					if(!AcceptDoc(*p_reply, line_buf))
						LogLastError();
					else
						Log((temp_buf = "Accepted").CatDiv(':', 2).Cat(line_buf));
				}
			}
		}
    }
	CATCH
		LogLastError();
		ok = 0;
	ENDCATCH
	xmlFreeParserCtxt(p_ctx);
	PPEmbeddedLogger::Save(PPFILNAM_EGAIS_LOG, 0);
    return ok;
}

void PPEgaisProcessor::LogTicketResult(const Ticket * pTicket, const BillTbl::Rec * pBillRec)
{
	SString temp_buf;
	GetDocTypeTag(pTicket->DocType, temp_buf);
	if(pBillRec) {
		SString bill_text;
		PPObjBill::MakeCodeString(pBillRec, PPObjBill::mcsAddOpName, bill_text);
		temp_buf.Space().Cat("bill").Space().CatChar('\'').Cat(bill_text).CatChar('\'');
	}
	if(pTicket->R.Type == 1) {
		temp_buf.Space().Cat("RESULT");
		if(pTicket->R.Conclusion == 1)
			temp_buf.CatDiv(':', 2).Cat("Accepted");
		else if(pTicket->R.Conclusion == 0)
			temp_buf.CatDiv(':', 2).Cat("Rejected");
		if(pTicket->R.Comment.NotEmpty())
			temp_buf.CatDiv('-', 1).Cat(pTicket->R.Comment);
	}
	if(pTicket->OpR.Type == 2) {
		temp_buf.Space().Cat("OPERATION-RESULT");
		if(pTicket->OpR.Conclusion == 1)
			temp_buf.CatDiv(':', 2).Cat("Accepted");
		else if(pTicket->OpR.Conclusion == 0)
			temp_buf.CatDiv(':', 2).Cat("Rejected");
		if(pTicket->OpR.Comment.NotEmpty())
			temp_buf.CatDiv('-', 1).Cat(pTicket->OpR.Comment);
	}
	LogTextWithAddendum(PPTXT_EGAIS_TICKETREGARRVD, temp_buf);
}

int PPEgaisProcessor::Helper_FinishBillProcessingByTicket(int ticketType, const BillTbl::Rec & rRec,
	const SString & rBillText, const PPEgaisProcessor::Ticket * pT, int conclusion, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    selfreject_ticket = 0;
	if(ticketType > 1000) {
		ticketType = ticketType - 1000;
		selfreject_ticket = 1000;
	}
	const  PPID bill_id = rRec.ID;
	SString temp_buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pT->RegIdent.NotEmpty()) {
			if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
				if(temp_buf.CmpNC(pT->RegIdent) != 0)
					LogTextWithAddendum(PPTXT_EGAIS_BILLEDIIDNTICKET, rBillText);
			}
			else if(conclusion != 0 && !selfreject_ticket) {
				if(ticketType == 2) {
					ObjTagItem tag_item;
					THROW(tag_item.SetStr(PPTAG_BILL_EDIIDENT, pT->RegIdent));
					THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
				}
			}
		}
		if(conclusion == 1) {
			int    msg_id = (ticketType == 2) ? PPTXT_EGAIS_BILLFPROCESSED_S : PPTXT_EGAIS_BILLFPROCESSED_U;
			THROW(P_BObj->P_Tbl->SetRecFlag2(bill_id, BILLF2_ACKPENDING, 0, 0));
			LogTextWithAddendum(msg_id, rBillText);
			ok = 1;
		}
		else if(conclusion == 0) {
			const int msg_id = (ticketType == 2) ? PPTXT_EGAIS_BILLFUNPROCESSED_S : PPTXT_EGAIS_BILLFUNPROCESSED_U;
			//
			// @v9.8.6 Обнаружилась не понятная ситуация с ЕГАИС - возникают сообщения о попытке отправить
			// дублированную накладную иногда без видимых оснований. Как попытка обойти проблему
			// мы идентифицируем текст сообщения и если, он соответствующий, то не будем удалять теги
			// и менять флаги.
			//
			// (!Отменяем - оказалось, стало хуже) if(pT->R.Special != Ticket::Result::spcDup && pT->OpR.Special != Ticket::Result::spcDup)
			THROW(P_BObj->P_Tbl->SetRecFlag2(bill_id, BILLF2_ACKPENDING, 0, 0));
			if(!(State & stDontRemoveTags)) {
				if(!!pT->TranspUUID) {
					const  PPID uuid_tag_id = selfreject_ticket ? PPTAG_BILL_EDIREJECTACK : PPTAG_BILL_EDIACK;
					ObjTagItem uuid_tag;
					if(p_ref->Ot.GetTag(PPOBJ_BILL, bill_id, uuid_tag_id, &uuid_tag) > 0) {
						S_GUID ex_uuid;
						uuid_tag.GetGuid(&ex_uuid);
						if(ex_uuid == pT->TranspUUID) {
							THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, bill_id, uuid_tag_id, 0));
						}
					}
				}
				if((!oneof4(rRec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4) && !selfreject_ticket) &&
					(rRec.EdiOp || oneof4(pT->DocType, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4))) {
					if(pT->RegIdent.NotEmpty()) {
						SString ex_edi_ident;
						if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, ex_edi_ident) > 0) {
							if(ex_edi_ident.CmpNC(pT->RegIdent) == 0)
								THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, 0));
						}
					}
				}
			}
			LogTextWithAddendum(msg_id, rBillText);
			ok = 1;
		}
		{
			SString memo_msg;
			SString prefix_buf;
			StringSet ss_prefix;
			if(conclusion == 0) {
				ss_prefix.add((temp_buf = PPConst::P_ObjMemo_UtmRejPfx).Colon());
				ss_prefix.add((temp_buf = PPConst::P_ObjMemo_EgaisRejPfx).Colon());
				if(ticketType == 1 && pT->R.Comment.NotEmpty()) {
					temp_buf.Z().Cat(PPConst::P_ObjMemo_UtmRejPfx).Space().Cat(pT->R.Time, DATF_ISO8601, TIMF_HMS).Colon();
					ss_prefix.add(temp_buf);
					memo_msg.Space().Cat(temp_buf).Space().Cat(pT->R.Comment);
				}
				else if(ticketType == 2 && pT->OpR.Comment.NotEmpty()) {
					temp_buf.Z().Cat(PPConst::P_ObjMemo_EgaisRejPfx).Space().Cat(pT->OpR.Time, DATF_ISO8601, TIMF_HMS).Colon();
					ss_prefix.add(temp_buf);
					memo_msg.Space().Cat(temp_buf).Space().Cat(pT->OpR.Comment);
				}
			}
			else {
				ss_prefix.add(PPConst::P_ObjMemo_UtmRejPfx);
				ss_prefix.add(PPConst::P_ObjMemo_EgaisRejPfx);
			}
			{
				int   do_update_memos = 0;
				SString memos;
				StringSet ss_memo(PPConst::P_ObjMemoDelim);
				StringSet ss_memo_new(PPConst::P_ObjMemoDelim);
				p_ref->GetPropVlrString(PPOBJ_BILL, bill_id, PPPRP_BILLMEMO, memos);
				ss_memo.setBuf(memos);
				for(uint ssp = 0; ss_memo.get(&ssp, temp_buf);) {
					if(!temp_buf.NotEmptyS())
						do_update_memos = 1;
					else {
						for(uint pssp = 0; ss_prefix.get(&pssp, prefix_buf);) {
							if(temp_buf.Search(prefix_buf, 0, 1, 0)) {
								temp_buf.Z();
								do_update_memos = 1;
								break;
							}
						}
						if(temp_buf.NotEmptyS())
							ss_memo_new.add(temp_buf);
					}
				}
				if(memo_msg.NotEmpty()) {
					ss_memo_new.add(memo_msg);
					do_update_memos = 1;
				}
				if(do_update_memos) {
					memos = ss_memo_new.getBuf();
					PutObjMemos(PPOBJ_BILL, PPPRP_BILLMEMO, bill_id, memos, 0);
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Helper_FinishConfirmProcessingByTicket(const BillTbl::Rec & rRec, const SString & rBillText, const S_GUID & rUuid, int conclusion, int use_ta)
{
	int    ok = -1;
	const  PPID bill_id = rRec.ID;
	SString temp_buf;
	{
		Reference * p_ref = PPRef;
		int    log_msg_code = 0;
		PPTransaction tra(1);
		THROW(tra);
		if(conclusion == 1) {
			ObjTagItem tag_item;
			THROW(tag_item.SetGuid(PPTAG_BILL_EDIRECADVCONFIRM, &rUuid));
			THROW(p_ref->Ot.PutTag(PPOBJ_BILL, rRec.ID, &tag_item, 0));
			log_msg_code = PPTXT_EGAIS_RECADVCFMACCEPTED;
			ok = 1;
		}
		else if(conclusion == 0) {
			THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, rRec.ID, PPTAG_BILL_EDIRECADVCONFIRM, 0));
			log_msg_code = PPTXT_EGAIS_RECADVCFMREJECTED;
			ok = 1;
		}
		LogTextWithAddendum(log_msg_code, rBillText);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::FinishBillProcessingByTicket(const PPEgaisProcessor::Ticket * pT, int use_ta)
{
	int    ok = -1;
	if(pT) {
		Reference * p_ref = PPRef;
		SString fmt_buf, bill_text, temp_buf;
		PPIDArray bill_id_list;
		ObjTagItem tag_item;
		PPIDArray ediop_list;
		ediop_list.addzlist(PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_ACTCHARGEON, PPEDIOP_EGAIS_ACTCHARGEON_V2,
			PPEDIOP_EGAIS_ACTWRITEOFF, PPEDIOP_EGAIS_ACTWRITEOFF_V2, PPEDIOP_EGAIS_ACTWRITEOFF_V3, PPEDIOP_EGAIS_TRANSFERTOSHOP, PPEDIOP_EGAIS_TRANSFERFROMSHOP,
			PPEDIOP_EGAIS_ACTCHARGEONSHOP, PPEDIOP_EGAIS_ACTWRITEOFFSHOP, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4,
			PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4, PPEDIOP_EGAIS_ACTFIXBARCODE, PPEDIOP_EGAIS_ACTUNFIXBARCODE, 0);
			// @v11.1.1 PPEDIOP_EGAIS_ACTWRITEOFF_V4
		if(pT->DocType == PPEDIOP_EGAIS_CONFIRMTICKET) {
			temp_buf = pT->RegIdent;
			p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, temp_buf, &bill_id_list);
			bill_id_list.sortAndUndup();
			for(uint j = 0; j < bill_id_list.getCount(); j++) {
				const  PPID bill_id = bill_id_list.get(j);
				BillTbl::Rec bill_rec;
				if(P_BObj->Search(bill_id, &bill_rec) > 0) {
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
					if(pT->R.Type == 1) {
						THROW(ok = Helper_FinishConfirmProcessingByTicket(bill_rec, bill_text, pT->TranspUUID, pT->R.Conclusion, use_ta));
						LogTicketResult(pT, &bill_rec);
					}
					if(pT->OpR.Type == 2) {
						THROW(ok = Helper_FinishConfirmProcessingByTicket(bill_rec, bill_text, pT->TranspUUID, pT->OpR.Conclusion, use_ta));
						LogTicketResult(pT, &bill_rec);
					}
					break;
				}
			}
		}
		else if(ediop_list.lsearch(pT->DocType)) {
			int    selfreject_ticket = 0;
			SString guid;
			if(!pT->TranspUUID.IsZero())
				pT->TranspUUID.ToStr(S_GUID::fmtIDL, guid);
			if(oneof4(pT->DocType, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4)) {
				p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIREJECTACK, guid, &bill_id_list);
				if(bill_id_list.getCount())
					selfreject_ticket = 1000;
			}
			if(!selfreject_ticket) {
				if(oneof4(pT->DocType, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
					//
					// Порядок идентификации документа для тикетов на документы отгрузки:
					// сначала ищем по PPTAG_BILL_EDIACK потом по PPTAG_BILL_EDIIDENT. Это связано с тем,
					// что в противном случае при отправке документов в пределах одной базы между родственными организациями
					// драфт-документ прихода может перехватить тикет, предназначенный отправленному документу.
					//
					if(guid.NotEmpty())
						p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIACK, guid, &bill_id_list);
					if(!bill_id_list.getCount() && pT->RegIdent.NotEmpty())
						p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, pT->RegIdent, &bill_id_list);
				}
				else {
					if(pT->RegIdent.NotEmpty())
						p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, pT->RegIdent, &bill_id_list);
					if(!bill_id_list.getCount() && guid.NotEmpty())
						p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIACK, guid, &bill_id_list);
				}
			}
			bill_id_list.sortAndUndup();
			for(uint j = 0; j < bill_id_list.getCount(); j++) {
				const  PPID bill_id = bill_id_list.get(j);
				BillTbl::Rec bill_rec;
				if(P_BObj->Search(bill_id, &bill_rec) > 0) {
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
					if(pT->R.Type == 1) {
						THROW(ok = Helper_FinishBillProcessingByTicket(1+selfreject_ticket, bill_rec, bill_text, pT, pT->R.Conclusion, use_ta));
						LogTicketResult(pT, &bill_rec);
					}
					if(pT->OpR.Type == 2) {
						THROW(ok = Helper_FinishBillProcessingByTicket(2+selfreject_ticket, bill_rec, bill_text, pT, pT->OpR.Conclusion, use_ta));
						LogTicketResult(pT, &bill_rec);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::Helper_ReadFilesOffline(const char * pPath, TSCollection <PPEgaisProcessor::Reply> & rList)
{
    int    ok = 1;
	SString temp_buf;
	SString code_buf;
	SString file_name;
	SFsPath ps;
	(temp_buf = pPath).SetLastSlash().Cat("*.*");
	SDirEntry de;
	for(SDirec direc(temp_buf); direc.Next(&de) > 0;) {
		if(de.IsFolder()) {
			de.GetNameA(file_name);
			if(!de.IsSelf() && !de.IsUpFolder() && !sstreqi_ascii(file_name, P_TempOutputDirName)) {
				de.GetNameA(pPath, temp_buf);
				THROW(Helper_ReadFilesOffline(temp_buf, rList)); // @recursion
			}
		}
		else {
			de.GetNameA(pPath, temp_buf);
			ps.Split(temp_buf);
			if(ps.Ext.IsEqiAscii("xml")) {
				PPEgaisProcessor::Reply * p_new_reply = rList.CreateNewItem();
				THROW_SL(p_new_reply);
				p_new_reply->Status |= Reply::stOffline;
				p_new_reply->AcceptedFileName = temp_buf;
				SFsPath::NormalizePath(pPath, SFsPath::npfSlash, p_new_reply->Url);
			}
		}
	}
	CATCHZOK
    return ok;
}

int PPEgaisProcessor::SearchActChargeByActInform(const PPEgaisProcessor::ActInform & rInf, PPID * pBillID)
{
	int    ok = -1;
	PPID   bill_id = 0;
    if(rInf.ActNumber.NotEmpty()) {
		PPID   op_id = 0;
		SString temp_buf;
		BillTbl::Rec bill_rec;
		THROW(GetActChargeOnOp(&op_id, PPEDIOP_EGAIS_ACTCHARGEON, 1));
		for(DateIter di(encodedate(1, 12, 2015), ZERODATE); ok < 0 && P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
			// @v11.0.12 BillCore::GetCode(temp_buf = bill_rec.Code);
			Egais_GetBillCode(bill_rec, temp_buf); // @v11.0.12
			if(temp_buf.CmpNC(rInf.ActNumber) == 0) {
				bill_id = bill_rec.ID;
				ok = 1;
			}
		}
    }
    CATCHZOK
    ASSIGN_PTR(pBillID, bill_id);
	return ok;
}

int PPEgaisProcessor::DeleteSrcPacket(const Packet * pPack, TSCollection <PPEgaisProcessor::Reply> & rReplyList)
{
	int    ok = -1;
	if(pPack && pPack->SrcReplyPos > 0 && pPack->SrcReplyPos <= rReplyList.getCount()) {
		Reply * p_reply = rReplyList.at(pPack->SrcReplyPos-1);
		THROW(DeleteDoc(*p_reply));
		ok = 1;
	}
	CATCH
		LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPEgaisProcessor::Helper_CollectRefs(void * pCtx, TSCollection <PPEgaisProcessor::Reply> & rReplyList, RefCollection & rRefC)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = static_cast<xmlParserCtxt *>(pCtx);
	SString temp_buf;
	SString temp_path;
	StringSet ss_url_tok;
    for(uint i = 0; i < rReplyList.getCount(); i++) {
		PPEgaisProcessor::Reply * p_reply = rReplyList.at(i);
		int    doc_type = 0;
		(temp_buf = p_reply->Url).Strip().RmvLastSlash();
		ss_url_tok.Z();
		temp_buf.Tokenize("/", ss_url_tok);
		ss_url_tok.reverse();
		for(uint sp = 0; !doc_type && ss_url_tok.get(&sp, temp_buf);) {
			if(temp_buf.NotEmptyS())
				doc_type = PPEgaisProcessor::RecognizeDocTypeTag(temp_buf);
		}
		if(oneof9(doc_type, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, 
			PPEDIOP_EGAIS_REPLYCLIENT, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2, PPEDIOP_EGAIS_REPLYRESTSSHOP, PPEDIOP_EGAIS_REPLYAP)) {
			int    adr = 1; // AcceptDoc result
			if(!(p_reply->Status & Reply::stOffline)) {
				THROW(MakeOutputFileName(p_reply, temp_path, temp_buf));
				adr = AcceptDoc(*p_reply, temp_buf);
			}
			if(!adr || !Helper_Read(p_ctx, p_reply->AcceptedFileName, 0, 0, 0, (i+1), 0, &rRefC))
				LogLastError();
			ok = 1;
		}
    }
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::CollectRefs()
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	{
		SString temp_buf;
		SString temp_path;
		SString url;
		StringSet ss_egais_paths;
		PPGetPath(PPPATH_TEMP, temp_path);
		SFsPath ps;
		(temp_buf = temp_path).SetLastSlash().Cat("*.*");
		SDirEntry de;
		for(SDirec direc(temp_buf, 1); direc.Next(&de) > 0;) {
			de.GetNameA(temp_buf);
			if(temp_buf.Len() == 18 && temp_buf.HasPrefixIAscii("EGAIS-")) {
				de.GetNameA(temp_path, temp_buf);
				ss_egais_paths.add(temp_buf);
			}
		}
		if(ss_egais_paths.getCount()) {
			THROW_MEM(SETIFZ(P_RefC, new RefCollection));
			THROW(p_ctx = xmlNewParserCtxt());
			for(uint pp = 0; ss_egais_paths.get(&pp, temp_buf);) {
				TSCollection <PPEgaisProcessor::Reply> reply_list;
				if(Helper_ReadFilesOffline(temp_buf, reply_list)) {
					Helper_CollectRefs(p_ctx, reply_list, *P_RefC);
				}
			}
			THROW(P_RefC->Store(1));
		}
	}
	CATCH
		LogLastError();
		ok = 0;
	ENDCATCH
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int PPEgaisProcessor::RemoveOutputMessages(PPID locID, bool debugMode)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	TSCollection <PPEgaisProcessor::Reply> reply_list;
	THROW(p_ctx = xmlNewParserCtxt());
	THROW(GetReplyList(p_ctx, locID, -1 /* ! -1 - исходящие сообщения */, reply_list));
	const uint reply_count = reply_list.getCount();
	if(reply_count) {
		SString msg_buf;
		PPLoadText(PPTXT_EGAIS_REMOVEOUTPUT, msg_buf);
		for(uint i = 0; i < reply_count; i++) {
			Reply * p_reply = reply_list.at(i);
			if(p_reply) {
				if(debugMode) {
					Log(p_reply->Url);
				}
				else {
					THROW(DeleteDoc(*p_reply));
				}
			}
			PPWaitPercent(i+1, reply_count, msg_buf);
		}
	}
	CATCH
		ok = 0;
		LogLastError();
	ENDCATCH
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int PPEgaisProcessor::ReadInput(PPID locID, const DateRange * pPeriod, long flags)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString temp_path;
	xmlParserCtxt * p_ctx = 0;
	TSCollection <PPEgaisProcessor::Reply> reply_list;
	TSCollection <PPEgaisProcessor::Packet> pack_list;
	StringSet ss_url_tok;
	PPWaitMsg(PPSTR_TEXT, PPTXT_EGAIS_READINGDATA, 0);
	THROW(p_ctx = xmlNewParserCtxt());
	THROW(GetDebugPath(locID, temp_path));
	if(flags & rifOffline) {
		THROW(Helper_ReadFilesOffline(temp_path, reply_list));
	}
	else {
		THROW(GetReplyList(p_ctx, locID, +1, reply_list));
	}
	const uint reply_count = reply_list.getCount();
	if(reply_count) {
		LongArray ediop_list;
		ediop_list.addzlist(PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4, PPEDIOP_EGAIS_TTNINFORMBREG, PPEDIOP_EGAIS_TTNINFORMF2REG,
			PPEDIOP_EGAIS_TICKET, PPEDIOP_EGAIS_REPLYCLIENT, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2, PPEDIOP_EGAIS_REPLYRESTSSHOP,
			PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG, PPEDIOP_EGAIS_WAYBILLACT, PPEDIOP_EGAIS_REPLYAP, PPEDIOP_EGAIS_REPLYFORMA, PPEDIOP_EGAIS_REPLYFORM1,
			PPEDIOP_EGAIS_REPLYRESTSSHOP, PPEDIOP_EGAIS_REPLYBARCODE, PPEDIOP_EGAIS_WAYBILLACT_V2, PPEDIOP_EGAIS_WAYBILLACT_V3, PPEDIOP_EGAIS_WAYBILLACT_V4,
			PPEDIOP_EGAIS_REQUESTREPEALWB, PPEDIOP_EGAIS_REPLYRESTBCODE, 0);
		ediop_list.sortAndUndup();
		THROW_MEM(SETIFZ(P_RefC, new RefCollection));
		for(uint i = 0; i < reply_count; i++) {
			PPEgaisProcessor::Reply * p_reply = reply_list.at(i);
			PPWaitPercent(i+1, reply_count, p_reply->Url);
			int    doc_type = 0;
			temp_buf = p_reply->Url;
			temp_buf.Strip().RmvLastSlash();
			ss_url_tok.Z();
			temp_buf.Tokenize("/", ss_url_tok);
			ss_url_tok.reverse();
			{
				//ReplyRests_v2
				int debug_mark = 0;
				if(temp_buf.Search("ReplyRests_v2", 0, 1, 0)) {
					debug_mark = 1;
				}
				else if(temp_buf.Search("ReplyRestsShop_v2", 0, 1, 0)) {
					debug_mark = 2;
				}
			}
			for(uint sp = 0; !doc_type && ss_url_tok.get(&sp, temp_buf);) {
				if(temp_buf.NotEmptyS())
					doc_type = PPEgaisProcessor::RecognizeDocTypeTag(temp_buf);
			}
			if(ediop_list.bsearch(doc_type)) {
				int    adr = 1; // AcceptDoc result
				if(!(p_reply->Status & Reply::stOffline)) {
					THROW(MakeOutputFileName(p_reply, temp_path, temp_buf));
					adr = AcceptDoc(*p_reply, temp_buf);
				}
				if(adr && Helper_Read(p_ctx, p_reply->AcceptedFileName, 0, locID, pPeriod, (i+1), &pack_list, P_RefC)) {
					if(oneof6(doc_type, PPEDIOP_EGAIS_REPLYCLIENT, PPEDIOP_EGAIS_REPLYAP, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2, 
						PPEDIOP_EGAIS_REPLYFORMA, PPEDIOP_EGAIS_REPLYFORM1)) { // @v11.3.4 PPEDIOP_EGAIS_REPLYFORM1
						if(!DeleteDoc(*p_reply))
							LogLastError();
					}
					else {
						uint plc = pack_list.getCount();
						if(plc) do {
							const Packet * p_pack = pack_list.at(--plc);
							if(p_pack && p_pack->SrcReplyPos == (i+1)) {
								if(p_pack->Flags & Packet::fDoDelete) {
									if(!DeleteDoc(*p_reply))
										LogLastError();
									break;
								}
							}
							else
								break;
						} while(plc);
					}
				}
				else
					LogLastError();
				ok = 1;
			}
			else {
				if(!(p_reply->Status & Reply::stOffline)) {
					if(MakeOutputFileName(p_reply, temp_path, temp_buf))
						AcceptDoc(*p_reply, temp_buf);
					else
						LogLastError();
				}
				GetDocTypeTag(doc_type, temp_buf);
				if(temp_buf.IsEmpty())
					temp_buf.Cat(doc_type);
				LogTextWithAddendum(PPTXT_EGAIS_GOTUNPROCESSEDDOC, temp_buf);
			}
			PPWaitPercent(i+1, reply_count);
		}
		if(!P_RefC->Store(1))
			LogLastError();
	}
	{
		PPIDArray bill_id_list;
		SString edi_ident;
		SString bill_text;
		const LDATETIME now_dtm = getcurdatetime_();
		LongArray skip_packidx_list;
		int    last_rest_pos = -1;
		int    last_restshop_pos = -1;
		DateIter last_rest_di;
		DateIter last_restshop_di;
		//
		// Акцептируем данные в 2 прохода. На втором проходе акцептируются те объекты,
		// которые могут зависеть от того, что принято на 1-м проходе.
		// На текущий момент это - справки Б.
		//
		uint   packidx = 0;
		//
		// 1-й проход
		//
		for(packidx = 0; packidx < pack_list.getCount(); packidx++) {
			Packet * p_pack = pack_list.at(packidx);
			if(p_pack && p_pack->P_Data && !skip_packidx_list.lsearch(static_cast<long>(packidx))) {
				if(oneof3(p_pack->DocType, PPEDIOP_EGAIS_REPLYRESTS, PPEDIOP_EGAIS_REPLYRESTS_V2, PPEDIOP_EGAIS_REPLYRESTSSHOP)) {
					const PPBillPacket * p_rbp = static_cast<const PPBillPacket *>(p_pack->P_Data);
					if(p_rbp->Rec.ID) {
						DateIter tdi;
						tdi.dt = p_rbp->Rec.Dt;
						tdi.oprno = p_rbp->Rec.BillNo;
						if(p_pack->DocType == PPEDIOP_EGAIS_REPLYRESTSSHOP) {
							if(tdi.Cmp(last_restshop_di) > 0) {
								last_restshop_di = tdi;
								last_restshop_pos = static_cast<int>(packidx);
							}
						}
						else {
							if(tdi.Cmp(last_rest_di) > 0) {
								last_rest_di = tdi;
								last_rest_pos = static_cast<int>(packidx);
							}
						}
					}
				}
				else if(oneof4(p_pack->DocType, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
					if(!Helper_AcceptBillPacket(p_pack, &pack_list, packidx))
						LogLastError();
				}
				else if(p_pack->DocType == PPEDIOP_EGAIS_REQUESTREPEALWB) {
                    const RepealWb * p_rwb = static_cast<const RepealWb *>(p_pack->P_Data);
					p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, p_rwb->TTNCode, &bill_id_list);
					PPIDArray candid_bill_list;
					BillTbl::Rec bill_rec;
					for(uint i = 0; i < bill_id_list.getCount(); i++) {
						const  PPID bill_id = bill_id_list.get(i);
						PPOprKind op_rec;
						PPFreight freight;
						if(P_BObj->Search(bill_id, &bill_rec) > 0) {
                            if(bill_rec.Object && GetOpData(bill_rec.OpID, &op_rec) > 0) {
                                const  PPID psn_id = ObjectToPerson(bill_rec.Object, 0);
								if(psn_id) {
									if(p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_FSRARID, temp_buf) > 0 && temp_buf.CmpNC(p_rwb->ContragentCode) == 0)
										candid_bill_list.add(bill_id);
									else if(P_BObj->FetchFreight(bill_id, &freight) > 0 && freight.DlvrAddrID__) {
										if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, freight.DlvrAddrID__, PPTAG_LOC_FSRARID, temp_buf) > 0 && temp_buf.CmpNC(p_rwb->ContragentCode) == 0)
											candid_bill_list.add(bill_id);
									}
								}
                            }
						}
					}
					if(candid_bill_list.getCount() == 1) {
						const  PPID bill_id = candid_bill_list.get(0);
						ObjTagItem tag_item;
						THROW(P_BObj->Search(bill_id, &bill_rec) > 0);
						if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIREPEALREQ, temp_buf) > 0) {
							;
						}
						else if(tag_item.SetStr(PPTAG_BILL_EDIREPEALREQ, p_rwb->ReqNumber) && p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 1)) {
							PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_text);
							LogTextWithAddendum(PPTXT_EGAIS_WBRPLREQACCEPTED, bill_text);
						}
						else
							LogLastError();
					}
					else if(candid_bill_list.getCount() > 1) {
						temp_buf.Z().Cat(p_rwb->TTNCode).CatDiv('-', 1).Cat(p_rwb->ReqNumber);
						LogTextWithAddendum(PPTXT_EGAIS_WBRPLREQMANYDOCS, temp_buf);
					}
					else {
						// Запрос на отмену проведения через ЕГАИС '%s' не может быть акцептирован поскольку ему не соответствует ни одного документа в БД
						temp_buf.Z().Cat(p_rwb->TTNCode).CatDiv('-', 1).Cat(p_rwb->ReqNumber);
						LogTextWithAddendum(PPTXT_EGAIS_WBRPLREQNODOCS, temp_buf);
					}
				}
				else if(p_pack->DocType == PPEDIOP_EGAIS_TICKET) {
					Ticket * p_tick = static_cast<Ticket *>(p_pack->P_Data);
					//
					// Note: Следующий вызов правильнее разместить в точке @1, однако пока
					// механизмы не будут окончательно отлажены будем финишировать цикл
					// обработки документа не зависимо от инфраструктуры хранения данных об
					// отправленных и ожищающих ответа запросов (DGQ).
					//
					THROW(FinishBillProcessingByTicket(p_tick, 1));
					if(!(flags & rifOffline) && diffdate(now_dtm.d, p_tick->TicketTime.d) > 1)
						DeleteSrcPacket(p_pack, reply_list);
				}
				else if(oneof2(p_pack->DocType, PPEDIOP_EGAIS_REPLYFORMA, PPEDIOP_EGAIS_REPLYFORM1)) { // @v11.3.4 PPEDIOP_EGAIS_REPLYFORM1
					const EgaisRefATbl::Rec * p_ref_a = static_cast<const EgaisRefATbl::Rec *>(p_pack->P_Data);
					if(P_RefC) {
						EgaisRefATbl::Rec refai;
						STRNSCPY(refai.RefACode, p_ref_a->RefACode);
						STRNSCPY(refai.AlcCode,  p_ref_a->AlcCode);
						STRNSCPY(refai.ManufRarIdent, p_ref_a->ManufRarIdent);
						STRNSCPY(refai.ImporterRarIdent, p_ref_a->ImporterRarIdent);
						refai.Volume = p_ref_a->Volume;
						refai.ActualDate = p_ref_a->ActualDate;
						refai.BottlingDate = p_ref_a->BottlingDate;
						THROW(P_RefC->SetRefA(refai));
					}
				}
				else if(p_pack->DocType == PPEDIOP_EGAIS_REPLYRESTBCODE) {
					const ReplyRestBCode * p_rrbc = static_cast<const ReplyRestBCode *>(p_pack->P_Data);
					if(p_rrbc) {
						SString out_file_name;
						PPGetFilePath(PPPATH_OUT, "egais-replyrestbcode.txt", out_file_name);
						SFile f_out(out_file_name, SFile::mAppend);
						if(f_out.IsValid()) {
							temp_buf.Z().Cat(p_rrbc->RestTime, DATF_ISO8601CENT, 0).Space().Cat(p_rrbc->Inform2RegId).CR();
							f_out.WriteLine(temp_buf);
							for(uint ssp = 0; p_rrbc->MarkSet.get(&ssp, temp_buf);) {
								temp_buf.Strip().CR();
								f_out.WriteLine(temp_buf);
							}
						}
					}
				}
			}
		}
		//
		// 2-й проход
		//
		for(packidx = 0; packidx < pack_list.getCount(); packidx++) {
			Packet * p_pack = pack_list.at(packidx);
			if(p_pack && p_pack->P_Data && !skip_packidx_list.lsearch(static_cast<long>(packidx))) {
				if(oneof2(p_pack->DocType, PPEDIOP_EGAIS_TTNINFORMBREG, PPEDIOP_EGAIS_TTNINFORMF2REG)) {
					const int __r = Helper_AcceptTtnRefB(p_pack, &pack_list, packidx, skip_packidx_list);
					if(!__r)
						LogLastError();
					else if(__r == 2 && !(flags & rifOffline) && p_pack->Flags & p_pack->fDoDelete)
						DeleteSrcPacket(p_pack, reply_list);
				}
				else if(p_pack->DocType == PPEDIOP_EGAIS_ACTINVENTORYINFORMBREG) {
					const ActInform * p_inf = static_cast<const ActInform *>(p_pack->P_Data);
					PPID   bill_id = 0;
					if(flags & rifRepairInventoryMark) {
						SString ref_b;
						PPTransaction tra(1);
						THROW(tra);
						for(uint j = 0; j < p_inf->Items.getCount(); j++) {
							const ActInformItem * p_item = p_inf->Items.at(j);
							if(p_item) {
								ref_b.Z();
								if(p_item->BItems.getCount()) {
									for(uint n = 0; n < p_item->BItems.getCount(); n++) {
										const InformBItem & r_bitem = p_item->BItems.at(n);
										if(r_bitem.P == p_item->P && r_bitem.Ident[0]) {
											(ref_b = r_bitem.Ident).Strip();
											break;
										}
									}
								}
								if(p_item->AIdent[0] && ref_b.NotEmpty()) {
									PPIDArray lot_A_list;
									PPIDArray lot_B_list;
									p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_FSRARINFA, p_item->AIdent, &lot_A_list);
									p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOT, PPTAG_LOT_FSRARINFB, ref_b, &lot_B_list);
									if(lot_A_list.getCount() == 0 && lot_B_list.getCount() == 1) {
										const  PPID lot_id = lot_B_list.get(0);
										ObjTagItem tag_item;
										THROW(tag_item.SetStr(PPTAG_LOT_FSRARINFA, p_item->AIdent));
										THROW(p_ref->Ot.PutTag(PPOBJ_LOT, lot_id, &tag_item, 0));
									}
								}
							}
						}
						THROW(tra.Commit());
					}
					else if(SearchActChargeByActInform(*p_inf, &bill_id) > 0) {
						PPBillPacket bp;
						THROW(P_BObj->ExtractPacket(bill_id, &bp) > 0);
						{
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < bp.GetTCount(); i++) {
								const long   row_id = static_cast<long>(i+1);
								for(uint j = 0; j < p_inf->Items.getCount(); j++) {
									const ActInformItem * p_item = p_inf->Items.at(j);
									if(p_item && p_item->P == row_id) {
										const PPTransferItem & r_ti = bp.ConstTI(i);
										const  PPID lot_id = static_cast<PPID>(r_ti.QuotPrice);
										if(lot_id) {
											ObjTagItem tag_item;
											if(p_item->AIdent[0]) {
												THROW(tag_item.SetStr(PPTAG_LOT_FSRARINFA, p_item->AIdent));
												THROW(p_ref->Ot.PutTag(PPOBJ_LOT, lot_id, &tag_item, 0));
											}
											if(p_item->BItems.getCount()) {
												for(uint n = 0; n < p_item->BItems.getCount(); n++) {
													const InformBItem & r_bitem = p_item->BItems.at(n);
													if(r_bitem.P == p_item->P && r_bitem.Ident[0]) {
														THROW(tag_item.SetStr(PPTAG_LOT_FSRARINFB, r_bitem.Ident));
														THROW(p_ref->Ot.PutTag(PPOBJ_LOT, lot_id, &tag_item, 0));
														break;
													}
												}
											}
										}
										break;
									}
								}
							}
							THROW(tra.Commit());
						}
					}
				}
			}
		}
		{
			if(last_rest_pos >= 0) {
				// Списание продаж по маркам
				const Packet * p_pack = pack_list.at(last_rest_pos);
				const PPBillPacket * p_bp = p_pack ? static_cast<const PPBillPacket *>(p_pack->P_Data) : 0;
				THROW(Helper_CreateWriteOffShop(1, p_bp, pPeriod));
			}
			if(last_restshop_pos >= 0 && last_restshop_pos < pack_list.getCountI()) {
				const Packet * p_pack = pack_list.at(last_restshop_pos);
				const PPBillPacket * p_bp = p_pack ? static_cast<const PPBillPacket *>(p_pack->P_Data) : 0;
				THROW(Helper_CreateTransferToShop(p_bp));
				THROW(Helper_CreateWriteOffShop(0, p_bp, pPeriod));
			}
		}
	}
	CATCH
		LogLastError();
		ok = 0; // Сообщение об ошибке должно выводиться блоком, который вызывает PPEgaisProcessor::ReadInput
	ENDCATCH
	xmlFreeParserCtxt(p_ctx);
	PPEmbeddedLogger::Save(PPFILNAM_EGAIS_LOG, 0);
	return ok;
}

int PPEgaisProcessor::GetActChargeOnOp(PPID * pID, int egaisOp, int use_ta)
{
	int    ok = 1;
	PPID   reserved_id = 0;
	PPID   op_type_id = 0;
	long   op_flags = 0;
	uint   name_id = 0;
	const  char * p_symb = 0;
	const  char * p_cntr_template = 0;

	PPID   op_id = 0;
	SString temp_buf;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	if(egaisOp == PPEDIOP_EGAIS_ACTCHARGEON) {
		reserved_id = PPOPK_EDI_ACTCHARGEON;
		name_id = PPTXT_OPK_EGAIS_ACTCHARGEON;
		p_symb = "EGAISACTCHARGEON";
		op_type_id = PPOPT_GOODSEXPEND;
		op_flags = (OPKF_NOUPDLOTREST|OPKF_PASSIVE);
		p_cntr_template = "EGACT%05";
	}
	else if(egaisOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
		reserved_id = PPOPK_EDI_ACTCHARGEONSHOP;
		name_id = PPTXT_OPK_EGAIS_ACTCHARGEONSHOP;
		p_symb = "EGAISACTCHARGEON-R2";
		op_type_id = PPOPT_DRAFTRECEIPT;
		op_flags = OPKF_PASSIVE;
		p_cntr_template = "EGACTS%05";
	}
	assert(reserved_id);
	THROW_INVARG(reserved_id);
	if(op_obj.Search(reserved_id, &op_rec) > 0) {
		op_id = op_rec.ID;
	}
	else {
		PPOprKindPacket op_pack;
		op_pack.Rec.ID = reserved_id;
        PPLoadText(name_id, temp_buf);
        THROW(temp_buf.NotEmptyS());
        STRNSCPY(op_pack.Rec.Name, temp_buf);
        STRNSCPY(op_pack.Rec.Symb, p_symb);
        op_pack.Rec.OpTypeID = op_type_id;
		op_pack.Rec.Flags |= op_flags;
		op_pack.OpCntrPack.Init(0);
		STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, p_cntr_template);
		op_pack.OpCntrPack.Head.Counter = 0;
		THROW(op_obj.PutPacket(&op_id, &op_pack, use_ta));
	}
	CATCHZOK
	ASSIGN_PTR(pID, op_id);
	return ok;
}

int PPEgaisProcessor::CreateActChargeOnBill(PPID * pBillID, int ediOp, PPID locID, LDATE restDate, const PPIDArray & rLotList, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   op_id = 0;
	PPID   bill_id = 0;
	SString ref_a, ref_b, egais_code;
	PrcssrAlcReport::GoodsItem agi;
	PPBillPacket pack;
	PPIDArray ex_lot_list;
	const LDATE rest_date = NZOR(restDate, getcurdate_());
	assert(oneof2(ediOp, PPEDIOP_EGAIS_ACTCHARGEON, PPEDIOP_EGAIS_ACTCHARGEONSHOP));
	THROW_INVARG(oneof2(ediOp, PPEDIOP_EGAIS_ACTCHARGEON, PPEDIOP_EGAIS_ACTCHARGEONSHOP));
	THROW(GetActChargeOnOp(&op_id, ediOp, 1));
	if(ediOp == PPEDIOP_EGAIS_ACTCHARGEON) {
		THROW_PP(CheckOpFlags(op_id, OPKF_NOUPDLOTREST, 0), PPERR_EGAIS_INVACTOP);
	}
	else if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
		THROW_PP(GetOpType(op_id) == PPOPT_DRAFTRECEIPT, PPERR_EGAIS_INVACTOP);
	}
	{
		BillTbl::Rec bill_rec;
		for(DateIter di; P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
			PPTransferItem ti;
			if(ediOp == PPEDIOP_EGAIS_ACTCHARGEON) {
				for(int rbb = 0; P_BObj->trfr->EnumItems(bill_rec.ID, &rbb, &ti) > 0;) {
					ex_lot_list.addnz(ti.LotID);
				}
			}
			else if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
				PPBillPacket ex_bp;
				if(P_BObj->ExtractPacketWithFlags(bill_rec.ID, &ex_bp, BPLD_FORCESERIALS) > 0) {
					for(uint i = 0; i < ex_bp.GetTCount(); i++) {
						const PPTransferItem & r_ex_ti = ex_bp.ConstTI(i);
						if(r_ex_ti.Quantity_ > 0.0)
							ex_lot_list.addnz(r_ex_ti.OrdLotID);
					}
				}
			}
		}
		ex_lot_list.sortAndUndup();
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(pack.CreateBlank2(op_id, rest_date, locID, 0));
        for(uint i = 0; i < rLotList.getCount(); i++) {
			const  PPID lot_id = rLotList.get(i);
			if(!ex_lot_list.bsearch(lot_id)) {
				ReceiptTbl::Rec lot_rec;
				if(P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					const  PPID goods_id = lot_rec.GoodsID;
					if(goods_id > 0 && IsAlcGoods(goods_id) && PreprocessGoodsItem(goods_id, 0, 0, 0, agi) > 0) { // Исключаем попадание лота заказа
						PPTransferItem ti;
						int   skip = 0;
						double rest = 0.0;
						p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_FSRARINFA, ref_a);
						p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_FSRARINFB, ref_b);
						p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_id, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
						if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
							if(ref_b.NotEmptyS() || egais_code.IsEmpty())
								skip = 1;
						}
						if(!skip) {
							if(ediOp == PPEDIOP_EGAIS_ACTCHARGEON) {
								P_BObj->trfr->GetRest(lot_id, pack.Rec.Dt, MAXLONG, &rest);
							}
							else if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
								P_BObj->trfr->GetRest(lot_id, pack.Rec.Dt, encodedate(30, 9, 2016), &rest);
								if(agi.UnpackedVolume > 0.0) {
									const double mult = agi.UnpackedVolume / 10.0;
									rest = (rest * mult); // Неупакованная продукция передается в декалитрах
								}
							}
							if(rest > 0.0) {
								THROW(ti.Init(&pack.Rec));
								THROW(ti.SetupGoods(goods_id));
								THROW(ti.SetupLot(lot_id, &lot_rec, 0));
								ti.QuotPrice = static_cast<double>(lot_id);
								ti.Cost = 0.0;
								ti.Price = 0.0;
								ti.Discount = 0.0;
								if(ediOp == PPEDIOP_EGAIS_ACTCHARGEON) {
									ti.Quantity_ = -rest;
								}
								else if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
									ti.Quantity_ = rest;
									ti.OrdLotID = lot_id;
								}
								{
									const uint tipos = pack.GetTCount();
									THROW(pack.LoadTItem(&ti, 0, 0));
									if(ediOp == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
										ObjTagList tag_list;
										ObjTagItem tag_item;
										tag_list.PutItemStrNE(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
										tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, ref_a);
										tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFB, ref_b);
										THROW(pack.LTagL.Set(tipos, &tag_list));
									}
								}
							}
						}
					}
				}
			}
        }
        if(pack.GetTCount()) {
			THROW(P_BObj->__TurnPacket(&pack, 0, 1, 0));
			bill_id = pack.Rec.ID;
        }
		else
			ok = -1;
		THROW(tra.Commit());
	}
    CATCHZOK
	ASSIGN_PTR(pBillID, bill_id);
	return ok;
}

int PPEgaisProcessor::GetAcceptedBillList(const PPBillIterchangeFilt & rP, long flags, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	Reference * p_ref = PPRef;
	uint   i;
	SString temp_buf;
	PPIDArray temp_bill_list;
	PPIDArray base_op_list;
	PPIDArray op_list;
	BillTbl::Rec bill_rec;
	ObjTagItem tag_item;
	base_op_list.add(ACfg.Hdr.EgaisRcptOpID);
	if(!(flags & bilstfRepeal))
		base_op_list.add(ACfg.Hdr.EgaisRetOpID);
	PPObjOprKind::ExpandOpList(base_op_list, op_list);
	for(i = 0; i < op_list.getCount(); i++) {
		const  PPID op_id = op_list.get(i);
		if(op_id) {
			PPOprKind op_rec;
			GetOpData(op_id, &op_rec);
			if(rP.IdList.getCount()) {
				for(uint j = 0; j < rP.IdList.getCount(); j++) {
					const  PPID bill_id = rP.IdList.get(j);
                    if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
						if(oneof4(bill_rec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4) && (!rP.LocID || bill_rec.LocID == rP.LocID)) {
							if(!(flags & bilstfWritedOff) || (bill_rec.Flags & BILLF_WRITEDOFF)) {
								if(CheckBillForMainOrgID(bill_rec, op_rec)) {
									temp_bill_list.add(bill_rec.ID);
								}
							}
						}
                    }
				}
			}
			else {
				for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
					if(oneof4(bill_rec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4) && (!rP.LocID || bill_rec.LocID == rP.LocID)) {
						if(!(flags & bilstfWritedOff) || (bill_rec.Flags & BILLF_WRITEDOFF)) {
							if(CheckBillForMainOrgID(bill_rec, op_rec))
								temp_bill_list.add(bill_rec.ID);
						}
					}
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const  PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0) {
			int    suited = 1;
			if(flags & bilstfReadyForAck) {
				if(!P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
					int    wroff_bill_is_ready = -1;
					BillTbl::Rec wroff_bill_rec;
					for(DateIter diter; P_BObj->P_Tbl->EnumLinks(bill_rec.ID, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;) {
						if(!P_BObj->CheckStatusFlag(wroff_bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
							wroff_bill_is_ready = 0;
							break;
						}
						else
							wroff_bill_is_ready = 1;
					}
					if(wroff_bill_is_ready <= 0)
						suited = 0;
				}
			}
			if(suited) {
				if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIACK, temp_buf) > 0) {
					if(flags & bilstfRepeal) {
						if(temp_buf.C(0) != '!')
							suited = 0;
						else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIREPEALACK, temp_buf) > 0) {
							suited = 0; // Запрос на отмену проведения уже отправлен
						}
					}
					else {
						suited = 0; // По документам с установленным тегом PPTAG_BILL_EDIACK уже отправлены подтверждения
					}
				}
				else if(flags & bilstfRepeal) {
					suited = 0;
				}
			}
			if(suited) {
				rList.add(bill_rec.ID);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPEgaisProcessor::SendBillActs(const PPBillIterchangeFilt & rP)
{
	int    ok = -1;
	PPIDArray accepted_bill_list;
	GetAcceptedBillList(rP, PPEgaisProcessor::bilstfReadyForAck, accepted_bill_list);
	for(uint i = 0; i < accepted_bill_list.getCount(); i++) {
		const  PPID bill_id = accepted_bill_list.get(i);
		PPEgaisProcessor::Packet pack(PPEDIOP_EGAIS_WAYBILLACT);
		PPBillPacket * p_bp = static_cast<PPBillPacket *>(pack.P_Data);
		if(P_BObj->ExtractPacket(bill_id, p_bp) > 0) {
            Ack ack;
			const char * p_suffix = 0;
			if(p_bp->Rec.EdiOp == PPEDIOP_EGAIS_WAYBILL_V4) { // @v11.0.12
				p_suffix = "WayBillAct_v4";
				pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V4;
			}
			else if(p_bp->Rec.EdiOp == PPEDIOP_EGAIS_WAYBILL_V3) {
				p_suffix = "WayBillAct_v3";
				pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V3;
			}
			else if(p_bp->Rec.EdiOp == PPEDIOP_EGAIS_WAYBILL_V2) {
				p_suffix = "WayBillAct_v2";
				pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V2;
			}
			else {
				p_suffix = "WayBillAct";
			}
            const int r = PutQuery(pack, rP.LocID, p_suffix, ack);
			if(r > 0) {
				ok = 1;
			}
			else if(!r)
				LogLastError();
		}
	}
	//CATCHZOK
	return ok;
}

int PPEgaisProcessor::SendBillRepeals(const PPBillIterchangeFilt & rP)
{
	int    ok = -1;
	PPIDArray accepted_bill_list;
	GetAcceptedBillList(rP, PPEgaisProcessor::bilstfReadyForAck|PPEgaisProcessor::bilstfRepeal, accepted_bill_list);
	for(uint i = 0; i < accepted_bill_list.getCount(); i++) {
		const  PPID bill_id = accepted_bill_list.get(i);
		PPBillPacket bp;
		if(P_BObj->ExtractPacket(bill_id, &bp) > 0) {
			// @v11.0.7 PPEDIOP_EGAIS_REQUESTREPEALAWO
			const int egais_doc_type = (bp.Rec.OpID == PPOPK_EDI_WROFFWITHMARKS) ? PPEDIOP_EGAIS_REQUESTREPEALAWO : PPEDIOP_EGAIS_REQUESTREPEALWB;
			SString url_suffix;
			const int gdttr = PPEgaisProcessor::GetDocTypeTag(egais_doc_type, url_suffix);
			assert(gdttr);
			if(url_suffix.NotEmpty()) {
				PPEgaisProcessor::Packet pack(egais_doc_type);
				Ack ack;
				RepealWb * p_rwb = static_cast<RepealWb *>(pack.P_Data);
				p_rwb->BillID = bp.Rec.ID;
				// @v11.0.12 BillCore::GetCode(p_rwb->ReqNumber = bp.Rec.Code);
				Egais_GetBillCode(bp, p_rwb->ReqNumber); // @v11.0.12
				p_rwb->ReqNumber.CatChar('-').Cat("repeal");
				if(bp.BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, p_rwb->TTNCode) > 0) {
					const int r = PutQuery(pack, rP.LocID, url_suffix, ack);
					if(r > 0) {
						ok = 1;
					}
					else if(!r)
						LogLastError();
				}
			}
		}
	}
	return ok;
}

int PPEgaisProcessor::GetBillListForTransmission(const PPBillIterchangeFilt & rP, long flags, PPIDArray & rList, PPIDArray * pRejectList)
{
	rList.clear();
	int    ok = -1;
	Reference * p_ref = PPRef;
	uint   i;
	SString edi_ack;
	SString edi_rejectack;
	SString edi_ident;
	ObjTagItem tag_item;
	PPIDArray op_list;
	PPObjOprKind op_obj;
	BillTbl::Rec bill_rec;
	PPIDArray temp_bill_list;
	{
		PPIDArray base_op_list;
		if(flags & bilstfChargeOn)
			base_op_list.add(PPOPK_EDI_ACTCHARGEON);
		if(flags & bilstfChargeOnShop)
			base_op_list.add(PPOPK_EDI_ACTCHARGEONSHOP);
		if(flags & (bilstfTransferToShop|bilstfTransferFromShop))
			base_op_list.add(PPOPK_EDI_SHOPCHARGEON);
		if(flags & bilstfWriteOffShop)
			base_op_list.add(PPOPK_EDI_WRITEOFFSHOP);
		if(flags & bilstfExpend)
			base_op_list.add(Cfg.ExpndOpID);
		if(flags & bilstfIntrExpend)
			base_op_list.add(Cfg.IntrExpndOpID);
		if(flags & bilstfReturnToSuppl)
			base_op_list.add(Cfg.SupplRetOpID);
		if(flags & bilstfFixBarcode)
			base_op_list.add(PPOPK_EDI_CHARGEONWITHMARKS);
		if(flags & bilstfLosses) {
			base_op_list.add(Cfg.ExpndEtcOpID);
			if(flags & bilstfV3)
				base_op_list.add(PPOPK_EDI_WROFFWITHMARKS);
		}
		if(flags & bilstfWbRepealConf) {
			base_op_list.add(Cfg.ExpndOpID);
			base_op_list.add(Cfg.SupplRetOpID);
		}
		PPObjOprKind::ExpandOpList(base_op_list, op_list);
	}
	for(i = 0; i < op_list.getCount(); i++) {
		const  PPID op_id = op_list.get(i);
		PPOprKind op_rec;
		GetOpData(op_id, &op_rec);
		if(rP.IdList.getCount()) {
			for(uint j = 0; j < rP.IdList.getCount(); j++) {
				const  PPID bill_id = rP.IdList.get(j);
				if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
					if(!rP.LocID || bill_rec.LocID == rP.LocID) {
						int    suited = 1;
						if(flags & bilstfReadyForAck && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
							suited = 0;
						else if(CheckBillForMainOrgID(bill_rec, op_rec))
							temp_bill_list.add(bill_rec.ID);
					}
				}
			}
		}
		else {
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if(!rP.LocID || bill_rec.LocID == rP.LocID) {
					int    suited = 1;
					if(flags & bilstfReadyForAck && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
						suited = 0;
					else if(CheckBillForMainOrgID(bill_rec, op_rec))
						temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const  PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0) {
			int    suited = 1;
			int    for_reject = 0;
			p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIACK, edi_ack);
			p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, edi_ident);
			p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIREJECTACK, edi_rejectack);
			if(edi_ack.NotEmpty() && edi_ident.NotEmpty() && bill_rec.Flags2 & BILLF2_DECLINED && edi_rejectack.IsEmpty()) {
				for_reject = 1; // По документу необходимо отправить отказ от проведения //
			}
			else if(suited) {
				if(edi_ack.NotEmpty())
					suited = 0; // По документам с установленным тегом PPTAG_BILL_EDIACK уже отправлены подтверждения
				else if((flags & (bilstfExpend|bilstfIntrExpend)) && edi_ident.NotEmpty())
					suited = 0; // Документ уже отправлен
			}
			if(suited) {
				PPBillPacket pack;
				suited = 0;
				if(P_BObj->ExtractPacket(bill_id, &pack) > 0) {
					int    has_plus = 0;
					int    has_minus = 0;
					for(uint tidx = 0; !suited && tidx < pack.GetTCount(); tidx++) {
						const PPTransferItem & r_ti = pack.ConstTI(tidx);
						if(IsAlcGoods(r_ti.GoodsID)) {
							if(r_ti.Quantity_ > 0.0)
								has_plus = 1;
							else if(r_ti.Quantity_ < 0.0)
								has_minus = 1;
							suited = 1;
						}
					}
					if(suited) {
						if((flags & bilstfTransferToShop) && !has_plus)
							suited = 0;
						else if((flags & bilstfTransferFromShop) && !has_minus)
							suited = 0;
					}
				}
				if(suited) {
					if(for_reject) {
						CALLPTRMEMB(pRejectList, add(bill_rec.ID));
					}
					else
						rList.add(bill_id);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPEgaisProcessor::CheckBillForMainOrgID(const BillTbl::Rec & rRec, const PPOprKind & rOpRec)
{
	int    ok = 1;
	const  PPID utm_main_org_id = P_UtmEntry ? P_UtmEntry->MainOrgID : 0;
	if(utm_main_org_id && AcsObj.IsLinkedToMainOrg(rOpRec.AccSheet2ID)) {
		if(rRec.Object2 && ObjectToPerson(rRec.Object2, 0) != utm_main_org_id)
			ok = 0;
	}
	return ok;
}

int PPEgaisProcessor::GetBillListForConfirmTicket(const PPBillIterchangeFilt & rP, long flags, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	Reference * p_ref = PPRef;
	uint   i;
	BillTbl::Rec bill_rec;
	SString temp_buf;
	ObjTagItem tag_item;
	PPIDArray op_list;
	PPObjOprKind op_obj;
	{
		PPIDArray base_op_list;
		if(flags & bilstfExpend)
			base_op_list.add(Cfg.ExpndOpID);
		if(flags & bilstfIntrExpend)
			base_op_list.add(Cfg.IntrExpndOpID);
		if(flags & bilstfReturnToSuppl)
			base_op_list.add(Cfg.SupplRetOpID);
		PPObjOprKind::ExpandOpList(base_op_list, op_list);
	}
    for(i = 0; i < op_list.getCount(); i++) {
		const  PPID op_id = op_list.get(i);
		PPOprKind op_rec;
		GetOpData(op_id, &op_rec);
		if(rP.IdList.getCount()) {
			for(uint j = 0; j < rP.IdList.getCount(); j++) {
				const  PPID bill_id = rP.IdList.get(j);
				if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
					if(!rP.LocID || bill_rec.LocID == rP.LocID) {
						int    suited = 1;
						const  int recadv_status = BillCore::GetRecadvStatus(bill_rec);
						const  int recadv_conf_status = BillCore::GetRecadvConfStatus(bill_rec);
						if(!recadv_status && !recadv_conf_status)
							suited = 0;
						else if(flags & bilstfReadyForAck && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
							suited = 0;
						else if(!CheckBillForMainOrgID(bill_rec, op_rec))
							suited = 0;
						else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIIDENT, temp_buf) <= 0)
							suited = 0;
						else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIACK, temp_buf) <= 0)
							suited = 0;
						else if(p_ref->Ot.GetTag(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIRECADVCONFIRM, &tag_item) > 0) {
							S_GUID uuid;
							if(tag_item.GetGuid(&uuid) && !!uuid)
								suited = 0;
						}
						if(suited) {
							rList.add(bill_rec.ID);
							ok = 1;
						}
					}
				}
			}
		}
		else {
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if(!rP.LocID || bill_rec.LocID == rP.LocID) {
					int    suited = 1;
					const  int recadv_status = BillCore::GetRecadvStatus(bill_rec);
					const  int recadv_conf_status = BillCore::GetRecadvConfStatus(bill_rec);
					if(!recadv_status && !recadv_conf_status)
						suited = 0;
					else if(flags & bilstfReadyForAck && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
						suited = 0;
					else if(!CheckBillForMainOrgID(bill_rec, op_rec))
						suited = 0;
					else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIIDENT, temp_buf) <= 0)
						suited = 0;
					else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIACK, temp_buf) <= 0)
						suited = 0;
					else if(p_ref->Ot.GetTag(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIRECADVCONFIRM, &tag_item) > 0) {
						S_GUID uuid;
						if(tag_item.GetGuid(&uuid) && !!uuid)
							suited = 0;
					}
					if(suited) {
						rList.add(bill_rec.ID);
						ok = 1;
					}
				}
			}
		}
    }
	return ok;
}

int PPEgaisProcessor::Helper_SendBills(PPID billID, int ediOp, PPID locID, const char * pUrlSuffix)
{
	int    ok = -1;
	PPEgaisProcessor::Packet pack(ediOp);
	if(P_BObj->ExtractPacketWithFlags(billID, static_cast<PPBillPacket *>(pack.P_Data), BPLD_FORCESERIALS) > 0) {
		Ack ack;
		const int r = PutQuery(pack, locID, pUrlSuffix, ack);
		if(r > 0)
			ok = 1;
		else if(!r)
			LogLastError();
	}
	return ok;
}

int PPEgaisProcessor::Helper_SendBillsByPattern(const PPBillIterchangeFilt & rP, const PPEgaisProcessor::BillTransmissionPattern & rPattern)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString file_name;
	SString temp_buf;
	PPIDArray totransm_bill_list;
	if(rPattern.EdiOp == PPEDIOP_EGAIS_CONFIRMREPEALWB) {
		PPOprKind op_rec;
		SString edi_ident;
		SString repeal_tag;
		StrAssocArray repealreq_bill_list;
		BillTbl::Rec bill_rec;
		p_ref->Ot.GetObjTextList(PPOBJ_BILL, PPTAG_BILL_EDIREPEALREQ, repealreq_bill_list);
		for(uint i = 0; i < repealreq_bill_list.getCount(); i++) {
            StrAssocArray::Item item = repealreq_bill_list.at_WithoutParent(i);
			(repeal_tag = item.Txt).Strip();
            if(repeal_tag.NotEmpty() && oneof2(repeal_tag.C(0), '!', '-')) {
				const  PPID bill_id = item.Id;
                if(P_BObj->Search(bill_id, &bill_rec) > 0 && p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, edi_ident) > 0) {
					GetOpData(bill_rec.OpID, &op_rec);
					if((!rP.LocID || bill_rec.LocID == rP.LocID) && CheckBillForMainOrgID(bill_rec, op_rec)) {
						PPEgaisProcessor::Packet pack(rPattern.EdiOp);
						RepealWb * p_rwb = static_cast<RepealWb *>(pack.P_Data);
						if(p_rwb) {
							p_rwb->BillID = bill_id;
							if(repeal_tag.C(0) == '!')
								p_rwb->Confirm = 1;
							else if(repeal_tag.C(0) == '-')
								p_rwb->Confirm = 0;
							else
								p_rwb->Confirm = -1;
							if(oneof2(p_rwb->Confirm, 0, 1)) {
								p_rwb->TTNCode = edi_ident;
								p_rwb->ReqTime = getcurdatetime_();
								p_rwb->ReqNumber = item.Txt+1;
								//
								{
									Ack ack;
									const int r = PutQuery(pack, rP.LocID, rPattern.P_UrlSuffix, ack);
									if(r > 0) {
										ObjTagItem tag_item;
										repeal_tag.ShiftLeft();
										repeal_tag.Insert(0, "*");
										if(tag_item.SetStr(PPTAG_BILL_EDIREPEALREQ, repeal_tag) && p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 1))
											ok = 1;
										else
											LogLastError();
									}
									else if(!r)
										LogLastError();
								}
							}
						}
					}
                }
            }
		}
	}
	else {
		GetBillListForTransmission(rP, rPattern.Flags, totransm_bill_list, 0);
		if(totransm_bill_list.getCount()) {
			if(rPattern.EdiOp == PPEDIOP_EGAIS_ACTCHARGEON) {
				ExclChrgOnMarks.Z();
				PPGetFilePath(PPPATH_IN, "egais-exclude-chargeon-marks.txt", file_name);
				if(fileExists(file_name)) {
					SFile f_in(file_name, SFile::mRead);
					while(f_in.ReadLine(temp_buf, SFile::rlfChomp)) {
						if(PrcssrAlcReport::IsEgaisMark(temp_buf, 0))
							ExclChrgOnMarks.add(temp_buf);
					}
				}
			}
			for(uint i = 0; i < totransm_bill_list.getCount(); i++) {
				if(Helper_SendBills(totransm_bill_list.get(i), rPattern.EdiOp, rP.LocID, rPattern.P_UrlSuffix) > 0)
					ok = 1;
			}
		}
	}
	return ok;
}

int PPEgaisProcessor::SendBills(const PPBillIterchangeFilt & rP)
{
	int    ok = -1;
	const  int __v2 = BIN(Cfg.E.Flags & Cfg.fEgaisVer2Fmt);
	const  int __v3 = BIN(State & stUseEgaisVer3);
	const  int __v4 = BIN(State & stUseEgaisVer4); // @v11.0.12
	SString file_name;
	SString temp_buf;
	PPIDArray totransm_bill_list, reject_bill_list;
	{
		static const PPEgaisProcessor::BillTransmissionPattern _BillTransmPatterns[] = {
			{ bilstfReadyForAck|bilstfChargeOn|bilstfV1, PPEDIOP_EGAIS_ACTCHARGEON,       "ActChargeOn" },
			{ bilstfReadyForAck|bilstfChargeOn|bilstfV2|bilstfV3, PPEDIOP_EGAIS_ACTCHARGEON_V2, "ActChargeOn_v2" },
			{ bilstfReadyForAck|bilstfChargeOnShop,      PPEDIOP_EGAIS_ACTCHARGEONSHOP,   "ActChargeOnShop_v2" },
			{ bilstfReadyForAck|bilstfWriteOffShop,      PPEDIOP_EGAIS_ACTWRITEOFFSHOP,   "ActWriteOffShop_v2" },
			{ bilstfReadyForAck|bilstfTransferToShop,    PPEDIOP_EGAIS_TRANSFERTOSHOP,    "TransferToShop" },
			{ bilstfReadyForAck|bilstfTransferFromShop,  PPEDIOP_EGAIS_TRANSFERFROMSHOP,  "TransferFromShop" },
			{ bilstfReadyForAck|bilstfLosses|bilstfV1,   PPEDIOP_EGAIS_ACTWRITEOFF,       "ActWriteOff" },
			{ bilstfReadyForAck|bilstfLosses|bilstfV2,   PPEDIOP_EGAIS_ACTWRITEOFF_V2,    "ActWriteOff_v2" },
			{ bilstfReadyForAck|bilstfLosses|bilstfV3,   PPEDIOP_EGAIS_ACTWRITEOFF_V3,    "ActWriteOff_v3" }, 
			{ bilstfReadyForAck|bilstfWbRepealConf,      PPEDIOP_EGAIS_CONFIRMREPEALWB,   "ConfirmRepealWB" },
			{ bilstfReadyForAck|bilstfFixBarcode,        PPEDIOP_EGAIS_ACTFIXBARCODE,     "ActFixBarCode" }
		};
		for(uint i = 0; i < SIZEOFARRAY(_BillTransmPatterns); i++) {
			const PPEgaisProcessor::BillTransmissionPattern & r_pattern = _BillTransmPatterns[i];
			int __do = 0;
			if(!(r_pattern.Flags & (bilstfV1|bilstfV2|bilstfV3)))
				__do = 1;
			else if((r_pattern.Flags & bilstfV3) && (__v3 || __v4))
				__do = 1;
			else if((r_pattern.Flags & bilstfV2) && __v2 && !(__v3 || __v4))
				__do = 1;
			else if((r_pattern.Flags & bilstfV1) && !__v2 && !__v3 && !__v4)
				__do = 1;
			if(__do) {
				if(Helper_SendBillsByPattern(rP, r_pattern) > 0)
					ok = 1;
			}
		}
	}
	{
		//
		// Передача документов отгрузки
		//
		totransm_bill_list.clear();
		reject_bill_list.clear();
		GetBillListForTransmission(rP, bilstfReadyForAck|bilstfExpend|bilstfIntrExpend, totransm_bill_list, &reject_bill_list);
		{
			const int edi_op = __v4 ? PPEDIOP_EGAIS_WAYBILL_V4 : (__v3 ? PPEDIOP_EGAIS_WAYBILL_V3 : (__v2 ? PPEDIOP_EGAIS_WAYBILL_V2 : PPEDIOP_EGAIS_WAYBILL));
			const char * p_url_sfx = __v4 ? "WayBill_v4" : (__v3 ? "WayBill_v3" : (__v2 ? "WayBill_v2" : "WayBill"));
			for(uint i = 0; i < totransm_bill_list.getCount(); i++) {
				if(Helper_SendBills(totransm_bill_list.get(i), edi_op, rP.LocID, p_url_sfx) > 0)
					ok = 1;
			}
		}
		{
			for(uint i = 0; i < reject_bill_list.getCount(); i++) {
				const  PPID bill_id = reject_bill_list.get(i);
				PPEgaisProcessor::Packet pack(PPEDIOP_EGAIS_WAYBILLACT);
				PPBillPacket * p_bp = static_cast<PPBillPacket *>(pack.P_Data);
				if(P_BObj->ExtractPacketWithFlags(bill_id, p_bp, BPLD_FORCESERIALS) > 0) {
					if(p_bp->Rec.Flags2 & BILLF2_DECLINED && p_bp->BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
						Ack ack;
						const char * p_suffix = 0;
						if(__v4) { // @v11.0.12
							p_suffix = "WayBillAct_v4";
							pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V4;
						}
						else if(__v3) {
							p_suffix = "WayBillAct_v3";
							pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V3;
						}
						else if(__v2) {
							p_suffix = "WayBillAct_v2";
							pack.DocType = PPEDIOP_EGAIS_WAYBILLACT_V2;
						}
						else {
							p_suffix = "WayBillAct";
						}
						const int r = PutQuery(pack, rP.LocID, p_suffix, ack);
						if(r > 0)
							ok = 1;
						else if(!r)
							LogLastError();
					}
				}
			}
		}
	}
	{
		//
		// Передача документов возврата поставщику
		//
		totransm_bill_list.clear();
		reject_bill_list.clear();
		GetBillListForTransmission(/*locID, period*/rP, bilstfReadyForAck|bilstfReturnToSuppl, totransm_bill_list, &reject_bill_list);
		{
			for(uint i = 0; i < totransm_bill_list.getCount(); i++) {
				const  PPID bill_id = totransm_bill_list.get(i);
				const int edi_op = __v4 ? PPEDIOP_EGAIS_WAYBILL_V4 : (__v3 ? PPEDIOP_EGAIS_WAYBILL_V3 : (__v2 ? PPEDIOP_EGAIS_WAYBILL_V2 : PPEDIOP_EGAIS_WAYBILL));
				const char * p_url_sfx = __v4 ? "WayBill_v4" : (__v3 ? "WayBill_v3" : (__v2 ? "WayBill_v2" : "WayBill"));
				PPEgaisProcessor::Packet pack(edi_op);
				pack.Flags |= PPEgaisProcessor::Packet::fReturnBill;
				PPBillPacket * p_bp = static_cast<PPBillPacket *>(pack.P_Data);
				if(P_BObj->ExtractPacketWithFlags(bill_id, p_bp, BPLD_FORCESERIALS) > 0) {
					Ack ack;
					const int r = PutQuery(pack, rP.LocID, p_url_sfx, ack);
					if(r > 0)
						ok = 1;
					else if(!r)
						LogLastError();
				}
			}
		}
		{
			for(uint i = 0; i < reject_bill_list.getCount(); i++) {
				const  PPID bill_id = reject_bill_list.get(i);
				const int edi_op = __v4 ? PPEDIOP_EGAIS_WAYBILLACT_V4 : (__v3 ? PPEDIOP_EGAIS_WAYBILLACT_V3 : (__v2 ? PPEDIOP_EGAIS_WAYBILLACT_V2 : PPEDIOP_EGAIS_WAYBILLACT));
				PPEgaisProcessor::Packet pack(edi_op);
				pack.Flags |= PPEgaisProcessor::Packet::fReturnBill;
				PPBillPacket * p_bp = static_cast<PPBillPacket *>(pack.P_Data);
				if(P_BObj->ExtractPacketWithFlags(bill_id, p_bp, BPLD_FORCESERIALS) > 0) {
					if(p_bp->Rec.Flags2 & BILLF2_DECLINED && p_bp->BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
						Ack ack;
						const char * p_suffix = (edi_op == PPEDIOP_EGAIS_WAYBILLACT_V4) ? "WayBillAct_v4" : 
							((edi_op == PPEDIOP_EGAIS_WAYBILLACT_V3) ? "WayBillAct_v3" : ((edi_op == PPEDIOP_EGAIS_WAYBILLACT_V2) ? "WayBillAct_v2" : "WayBillAct"));
						const int r = PutQuery(pack, rP.LocID, p_suffix, ack);
						if(r > 0)
							ok = 1;
						else if(!r)
							LogLastError();
					}
				}
			}
		}
	}
	{
		//
		// Внутренние перемещения
		//
		totransm_bill_list.clear();
		GetBillListForConfirmTicket(rP, bilstfReadyForAck|bilstfExpend|bilstfIntrExpend|bilstfReturnToSuppl, totransm_bill_list);
		ObjTagItem tag_item;
		SString reg_ident;
		for(uint i = 0; i < totransm_bill_list.getCount(); i++) {
			const  PPID bill_id = totransm_bill_list.get(i);
			BillTbl::Rec bill_rec;
			if(P_BObj->Search(bill_id, &bill_rec) > 0) {
				if(PPRef->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIIDENT, reg_ident) > 0) {
					PPEgaisProcessor::Packet pack(PPEDIOP_EGAIS_CONFIRMTICKET);
					ConfirmTicket * p_ticket = static_cast<ConfirmTicket *>(pack.P_Data);
					p_ticket->BillID = bill_rec.ID;
					p_ticket->Conclusion = -1;
					const int recadv_conf_status = BillCore::GetRecadvConfStatus(bill_rec);
					if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_ACCEPT)
						p_ticket->Conclusion = 1;
					else if(recadv_conf_status == PPEDI_RECADVCONF_STATUS_REJECT)
						p_ticket->Conclusion = 0;
					if(oneof2(p_ticket->Conclusion, 0, 1)) {
						Ack ack;
						// @v11.0.12 p_ticket->Code = bill_rec.Code;
                        // @v11.0.12 BillCore::GetCode(p_ticket->Code);
						Egais_GetBillCode(bill_rec, p_ticket->Code); // @v11.0.12
                        p_ticket->Code.CatChar('-').Cat("RECADVCFM");
						p_ticket->Date = getcurdate_();
						p_ticket->RegIdent = reg_ident;
						const int r = PutQuery(pack, rP.LocID, "WayBillTicket", ack);
						if(r > 0)
							ok = 1;
						else if(!r)
							LogLastError();
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}

PPEgaisProcessor::QueryParam::QueryParam() : DocType(0), DbActualizeFlags(0), Flags(0), MainOrgID(0), LocID(0), P_LotFilt(0)
{
}

PPEgaisProcessor::QueryParam::~QueryParam()
{
	ZDELETE(P_LotFilt);
}

PPEgaisProcessor::QueryParam::QueryParam(const QueryParam & rS) : DocType(rS.DocType), DbActualizeFlags(rS.DbActualizeFlags), Flags(rS.Flags),
	MainOrgID(rS.MainOrgID), LocID(rS.LocID), P_LotFilt(0), ParamString(rS.ParamString), InfoText(rS.InfoText)
{
	if(rS.P_LotFilt)
		P_LotFilt = new LotFilt(*rS.P_LotFilt);
}

PPEgaisProcessor::QueryParam & PPEgaisProcessor::QueryParam::operator = (const QueryParam & rS)
{
	DocType = rS.DocType;
	DbActualizeFlags = rS.DbActualizeFlags;
	Flags = rS.Flags;
	MainOrgID = rS.MainOrgID;
	LocID = rS.LocID;
	ParamString = rS.ParamString;
	InfoText = rS.InfoText;
	if(rS.P_LotFilt) {
		if(P_LotFilt) {
			*P_LotFilt = *rS.P_LotFilt;
		}
		else
			P_LotFilt = new LotFilt(*rS.P_LotFilt);
	}
	else {
		ZDELETE(P_LotFilt);
	}
	return *this;
}

int PPEgaisProcessor::QueryParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, DocType, rBuf));
	THROW(pSCtx->Serialize(dir, DbActualizeFlags, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, MainOrgID, rBuf));
	THROW(pSCtx->Serialize(dir, LocID, rBuf));
	THROW(pSCtx->Serialize(dir, ParamString, rBuf));
	CATCHZOK
	return ok;
}

int PPEgaisProcessor::EditQueryParam(PPEgaisProcessor::QueryParam * pData)
{
	class EgaisQDialog : public TDialog {
		DECL_DIALOG_DATA(PPEgaisProcessor::QueryParam);
	public:
		EgaisQDialog(PPEgaisProcessor * pPrc) : TDialog(DLG_EGAISQ), P_Prc(pPrc), Prev_afClearInnerEgaisDb_State(0)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_EGAISQ_WHAT,  0, PPEDIOP_EGAIS_QUERYCLIENTS);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  1, PPEDIOP_EGAIS_QUERYCLIENTS+1000);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  2, PPEDIOP_EGAIS_QUERYAP);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  3, PPEDIOP_EGAIS_QUERYFORMA);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  4, PPEDIOP_EGAIS_QUERYFORMB);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  5, PPEDIOP_EGAIS_QUERYRESTS);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  6, PPEDIOP_EGAIS_QUERYRESTSSHOP);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  7, PPEDIOP_EGAIS_QUERYBARCODE);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  8, PPEDIOP_EGAIS_QUERYCLIENTS+2000);
			AddClusterAssoc(CTL_EGAISQ_WHAT,  9, PPEDIOP_EGAIS_QUERYCLIENTS+3000);
			AddClusterAssoc(CTL_EGAISQ_WHAT, 10, PPEDIOP_EGAIS_NOTIFY_WBVER2);
			AddClusterAssoc(CTL_EGAISQ_WHAT, 11, PPEDIOP_EGAIS_NOTIFY_WBVER3);
			AddClusterAssoc(CTL_EGAISQ_WHAT, 12, PPEDIOP_EGAIS_QUERYRESENDDOC);
			AddClusterAssoc(CTL_EGAISQ_WHAT, 13, PPEDIOP_EGAIS_QUERYRESTBCODE);
			AddClusterAssoc(CTL_EGAISQ_WHAT, 14, PPEDIOP_EGAIS_NOTIFY_WBVER4); // @v11.0.12
			SetClusterData(CTL_EGAISQ_WHAT, Data.DocType);
			setCtrlString(CTL_EGAISQ_QADD, Data.ParamString);
			SetupPersonCombo(this, CTLSEL_EGAISQ_MAINORG, Data.MainOrgID, 0, PPPRK_MAIN, 1);
			SetupLocationCombo(this, CTLSEL_EGAISQ_LOC, Data.LocID, 0, LOCTYP_WAREHOUSE, 0);

			AddClusterAssoc(CTL_EGAISQ_ACTLZVAR, 0, Data._afQueryRefA);
			AddClusterAssoc(CTL_EGAISQ_ACTLZVAR, 1, Data._afQueryPerson);
			AddClusterAssoc(CTL_EGAISQ_ACTLZVAR, 2, Data._afQueryGoods);
			AddClusterAssoc(CTL_EGAISQ_ACTLZVAR, 3, Data._afQueryByChargeOn);
			AddClusterAssoc(CTL_EGAISQ_ACTLZVAR, 4, Data._afClearInnerEgaisDb);
			SetClusterData(CTL_EGAISQ_ACTLZVAR, Data.DbActualizeFlags);
			Prev_afClearInnerEgaisDb_State = BIN(Data.DbActualizeFlags & Data._afClearInnerEgaisDb);

			AddClusterAssoc(CTL_EGAISQ_FLAGS, 0, stTestSendingMode);
			SetClusterData(CTL_EGAISQ_FLAGS, Data.Flags);

			setStaticText(CTL_EGAISQ_INFO, Data.InfoText);

			disableCtrl(CTL_EGAISQ_ACTLZVAR, Data.DocType != (PPEDIOP_EGAIS_QUERYCLIENTS+2000));
			SetupStartUpInfo();
			DisplayInfo(0);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Data.DocType = GetClusterData(CTL_EGAISQ_WHAT);
			getCtrlString(CTL_EGAISQ_QADD, Data.ParamString);
			getCtrlData(CTLSEL_EGAISQ_MAINORG, &Data.MainOrgID);
			getCtrlData(CTLSEL_EGAISQ_LOC, &Data.LocID);
			GetClusterData(CTL_EGAISQ_FLAGS, &Data.Flags);
			GetClusterData(CTL_EGAISQ_ACTLZVAR, &Data.DbActualizeFlags);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
        void   DisplayInfo(const char * pInfo)
        {
			SString msg_buf;
			if(StartUpInfo.NotEmpty())
				msg_buf.Cat(StartUpInfo).CR();
			msg_buf.Cat(pInfo);
			setStaticText(CTL_EGAISQ_INFO, msg_buf);
        }
        void   SetupStartUpInfo()
        {
        	StartUpInfo = 0;
			if(P_Prc) {
				PPID   main_org_id = 0;
				PPID   loc_id = 0;
				SString url, temp_buf;
				getCtrlData(CTLSEL_EGAISQ_MAINORG, &main_org_id);
				getCtrlData(CTLSEL_EGAISQ_LOC, &loc_id);
				{
					TSVector <UtmEntry> utm_list;
					const  PPID preserve_main_org_id = GetMainOrgID();
					if(main_org_id)
						DS.SetMainOrgID(main_org_id, 0);
					{
						P_Prc->GetUtmList(loc_id, utm_list);
						if(utm_list.getCount()) {
							const UtmEntry & r_utm_entry = utm_list.at(0);
							StartUpInfo.Cat(r_utm_entry.FSRARID).Space().Cat(r_utm_entry.Url);
							if(utm_list.getCount() > 1)
								StartUpInfo.Space().Cat("multi");
						}
						else {
							if(P_Prc->GetFSRARID(loc_id, temp_buf.Z(), 0))
								StartUpInfo.Cat(temp_buf);
							else
								StartUpInfo.Cat("fasrarid-undef");
							StartUpInfo.Space();
							if(P_Prc->GetURL(loc_id, url.Z()))
								StartUpInfo.Cat(url);
							else
								StartUpInfo.Cat("url-undef");
						}
					}
					DS.SetMainOrgID(preserve_main_org_id, 0);
				}
			}
        }
	private:
        DECL_HANDLE_EVENT
        {
        	TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_EGAISQ_WHAT)) {
				long doc_type = GetClusterData(CTL_EGAISQ_WHAT);
				SString msg_buf;
				int   info_text_id = 0;
				if(doc_type == (PPEDIOP_EGAIS_QUERYCLIENTS+2000))
					disableCtrl(CTL_EGAISQ_ACTLZVAR, false);
				else {
					disableCtrl(CTL_EGAISQ_ACTLZVAR, true);
					switch(doc_type) {
						case PPEDIOP_EGAIS_QUERYBARCODE: info_text_id = PPTXT_HINT_EGAIS_QBARCODE; break;
						case PPEDIOP_EGAIS_QUERYCLIENTS: info_text_id = PPTXT_HINT_EGAIS_QCLIENTS; break;
						case PPEDIOP_EGAIS_QUERYAP: info_text_id = PPTXT_HINT_EGAIS_QPRODUCTS; break;
						case PPEDIOP_EGAIS_QUERYRESTS: info_text_id = PPTXT_HINT_EGAIS_QRESTS; break;
						case PPEDIOP_EGAIS_QUERYRESTSSHOP: info_text_id = PPTXT_HINT_EGAIS_QRESTSSHOP; break;
						case PPEDIOP_EGAIS_NOTIFY_WBVER2: info_text_id = PPTXT_HINT_EGAIS_NOTIFY_WBVER2; break;
						case PPEDIOP_EGAIS_NOTIFY_WBVER3: info_text_id = PPTXT_HINT_EGAIS_NOTIFY_WBVER3; break;
						case PPEDIOP_EGAIS_NOTIFY_WBVER4: info_text_id = PPTXT_HINT_EGAIS_NOTIFY_WBVER4; break; // @v11.0.12
						case PPEDIOP_EGAIS_QUERYRESENDDOC: info_text_id = PPTXT_HINT_EGAIS_QUERYRESENDDOC; break;
						case PPEDIOP_EGAIS_QUERYRESTBCODE: info_text_id = PPTXT_HINT_EGAIS_QUERYRESTBCODE; break;
					}
				}
				DisplayInfo(info_text_id ? PPLoadTextS(info_text_id, msg_buf).cptr() : 0);
			}
			else if(event.isClusterClk(CTL_EGAISQ_ACTLZVAR)) {
				long _af = 0;
				GetClusterData(CTL_EGAISQ_ACTLZVAR, &_af);
				if(!(_af & PPEgaisProcessor::QueryParam::_afClearInnerEgaisDb)) {
					Prev_afClearInnerEgaisDb_State = 0;
				}
				else if(!Prev_afClearInnerEgaisDb_State) {
					if(CONFIRM(PPCFM_CLEARINNEREGAISDB)) {
						Prev_afClearInnerEgaisDb_State = 1;
					}
					else {
						_af &= ~PPEgaisProcessor::QueryParam::_afClearInnerEgaisDb;
						SetClusterData(CTL_EGAISQ_ACTLZVAR, _af);
					}
				}
			}
			else if(event.isCbSelected(CTLSEL_EGAISQ_LOC) || event.isCbSelected(CTLSEL_EGAISQ_MAINORG)) {
				SetupStartUpInfo();
				DisplayInfo(0);
			}
			else if(event.isCmd(cmLotFilt)) {
				LotFilt temp_filt;
				RVALUEPTR(temp_filt, Data.P_LotFilt);
				PPViewLot lot_view;
				if(lot_view.EditBaseFilt(&temp_filt) > 0) {
					if(Data.P_LotFilt) {
						*Data.P_LotFilt = temp_filt;
					}
					else
						Data.P_LotFilt = new LotFilt(temp_filt);
				}
			}
			else
				return;
			clearEvent(event);
        }
        SString StartUpInfo;
        PPEgaisProcessor * P_Prc;
		long   Prev_afClearInnerEgaisDb_State;
	};
	int    ok = -1;
	QueryParam _param;
	RVALUEPTR(_param, pData);
	EgaisQDialog * dlg = new EgaisQDialog(this);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&_param);
		if(ExecView(dlg) == cmOK) {
			dlg->getDTS(&_param);
			ASSIGN_PTR(pData, _param);
            ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPEgaisProcessor::Helper_ExtractGoodsCodesFromBills(PPID opID, StringSet & rSs)
{
	int    ok = -1;
	SString temp_buf;
	BillTbl::Rec cobill_rec;
	DateRange coperiod;
	coperiod.Set(encodedate(1, 9, 2016), ZERODATE);
	for(SEnum en = P_BObj->P_Tbl->EnumByOp(opID, &coperiod, 0); en.Next(&cobill_rec) > 0;) {
		const  PPID sobill_id = cobill_rec.ID;
		PPBillPacket sobill_pack;
		if(P_BObj->ExtractPacketWithFlags(sobill_id, &sobill_pack, BPLD_FORCESERIALS) > 0) {
			for(uint k = 0; k < sobill_pack.GetTCount(); k++) {
				const PPTransferItem & r_so_ti = sobill_pack.ConstTI(k);
				if(sobill_pack.LTagL.GetTagStr(k, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0) {
					if(temp_buf.Len() > 8 && P_RefC->PrC.IsVerifiedCode(temp_buf) <= 0) {
						rSs.add(temp_buf);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

int PPEgaisProcessor::ImplementQuery(PPEgaisProcessor::QueryParam & rParam)
{
	int    ok = 1;
	int    query_sended = 0;
	int    do_report_error = 0;
	SString mark_buf;
	SString temp_buf;
	TSVector <UtmEntry> utm_list;
	const  PPID preserve_main_org_id = GetMainOrgID();
	if(rParam.MainOrgID)
		DS.SetMainOrgID(rParam.MainOrgID, 0);
	{
		GetUtmList(rParam.LocID, utm_list);
		if(utm_list.getCount()) {
			const UtmEntry & r_utm_entry = utm_list.at(0);
			SetUtmEntry(rParam.LocID, &r_utm_entry, 0);
		}
	}
	//
	if(PrcssrAlcReport::IsEgaisMark(rParam.ParamString, &mark_buf)) {
		PrcssrAlcReport::EgaisMarkBlock emb;
		PrcssrAlcReport::ParseEgaisMark(mark_buf, emb);
		rParam.InfoText.Z().Cat(emb.EgaisCode);
	}
	else {
		Ack    ack;
		switch(rParam.DocType) {
			case PPEDIOP_EGAIS_QUERYCLIENTS:
				if(QueryClients(rParam.LocID, querybyINN, rParam.ParamString))
					query_sended = 1;
				else
					do_report_error = 1;
				break;
			case (PPEDIOP_EGAIS_QUERYCLIENTS+1000):
				if(Cfg.AlcLicRegTypeID) {
					const LDATE _cd = getcurdate_();
					StringSet inn_list;
					SString temp_buf;
					PPViewPerson psn_view;
					PersonFilt psn_filt;
					PersonViewItem psn_item;
					psn_filt.SetAttribType(PPPSNATTR_REGISTER);
					psn_filt.RegTypeID = Cfg.AlcLicRegTypeID;
					psn_filt.EmptyAttrib = EA_NOEMPTY;
					PPWaitStart();
					THROW(psn_view.Init_(&psn_filt));
					for(psn_view.InitIteration(); psn_view.NextIteration(&psn_item) > 0;) {
						RegisterTbl::Rec reg_rec;
						if(PsnObj.GetRegister(psn_item.ID, PPREGT_TPID, _cd, &reg_rec) > 0) {
							(temp_buf = reg_rec.Num).Strip().ToUpper();
							if(!inn_list.search(temp_buf, 0, 0))
								inn_list.add(temp_buf);
						}
						PPWaitPercent(psn_view.GetCounter(), "Get contragent INN");
					}
					{
						const uint innc = inn_list.getCount();
						uint   inni = 0;
						for(uint innp = 0; inn_list.get(&innp, temp_buf);) {
							if(QueryClients(rParam.LocID, querybyINN, temp_buf))
								query_sended = 1;
							inni++;
							PPWaitPercent(inni, innc, "Query EGAIS contragent");
						}
					}
					PPWaitStop();
				}
				break;
			case (PPEDIOP_EGAIS_QUERYCLIENTS+2000):
				{
					THROW_MEM(SETIFZ(P_RefC, new RefCollection));
					{
						//
						// Запрос контрагента по идентификатору: "СИО"
						// Запрос товара по коду ЕГАИС: "КОД"
						//
						const uint _delay_ms = 100;
						int   db_was_cleared = 0;
						SString msg_buf;
						StringSet refa_codes;
						StringSet manuf_codes_for_products; // Идентификаторы
						StringSet codes_for_products;
						StringSet codes_for_contragents;
						StringSet inn_for_contragents;
						PPWaitStart();
						if(rParam.DbActualizeFlags & rParam._afClearInnerEgaisDb) {
							PPTransaction tra(1);
							THROW(tra);
							THROW(P_RefC->PrC.Clear(0))
							THROW(P_RefC->PsC.Clear(0));
							THROW(P_RefC->RaC.Clear(0));
							THROW(tra.Commit());
							db_was_cleared = 1;
						}
						PPWaitMsg(PPSTR_TEXT, PPTXT_EGAIS_PREPAREQFORIDB, 0);
						if(rParam.DbActualizeFlags & rParam._afQueryByChargeOn) {
							Helper_ExtractGoodsCodesFromBills(PPOPK_EDI_ACTCHARGEONSHOP, codes_for_products);
							Helper_ExtractGoodsCodesFromBills(PPOPK_EDI_SHOPCHARGEON, codes_for_products);
							Helper_ExtractGoodsCodesFromBills(PPOPK_EDI_WRITEOFFSHOP, codes_for_products);
						}
						if(!db_was_cleared && rParam.DbActualizeFlags & (rParam._afQueryRefA|rParam._afQueryPerson|rParam._afQueryGoods)) {
							if(rParam.P_LotFilt && rParam.DbActualizeFlags & rParam._afQueryRefA) {
								Reference * p_ref = PPRef;
								PPViewLot lot_view;
								LotFilt l;
								if(lot_view.Init_(rParam.P_LotFilt)) {
									LotViewItem lot_view_item;
									for(lot_view.InitIteration(); lot_view.NextIteration(&lot_view_item) > 0;) {
										if(p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_view_item.ID, PPTAG_LOT_FSRARINFA, temp_buf) > 0) {
											refa_codes.add(temp_buf);
										}
									}
								}
							}
							else {
								BExtQuery q(&P_RefC->RaC, 1);
								EgaisRefATbl::Key1 k1;
								MEMSZERO(k1);
								q.selectAll();
								for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
									EgaisRefATbl::Rec item;
									P_RefC->RaC.copyBufTo(&item);
									if(item.RefACode[0]) {
										if(rParam.DbActualizeFlags & rParam._afQueryRefA) {
											if(!(item.Flags & EgaisRefACore::fVerified))
												refa_codes.add(item.RefACode);
										}
										if(rParam.DbActualizeFlags & rParam._afQueryPerson) {
											int    is_importer = 0;
											if(sstrlen(item.ImporterRarIdent) >= 8) { // Код контрагента 12 символов, но страхуемся значением 8
												is_importer = 1;
												if(P_RefC->PsC.IsVerifiedCode(item.ImporterRarIdent) <= 0) {
													codes_for_contragents.add(item.ImporterRarIdent);
													manuf_codes_for_products.add(item.ImporterRarIdent);
												}
											}
											if(sstrlen(item.ManufRarIdent) >= 8) {
												if(P_RefC->PsC.IsVerifiedCode(item.ManufRarIdent) <= 0) {
													codes_for_contragents.add(item.ManufRarIdent);
													if(!is_importer)
														manuf_codes_for_products.add(item.ManufRarIdent);
												}
											}
										}
										if(rParam.DbActualizeFlags & rParam._afQueryGoods) {
											if(sstrlen(item.AlcCode) > 8) {
												if(P_RefC->PrC.IsVerifiedCode(item.AlcCode) <= 0)
													codes_for_products.add(item.AlcCode);
											}
										}
									}
								}
							}
						}
						refa_codes.sortAndUndup();
						codes_for_contragents.sortAndUndup();
						manuf_codes_for_products.sortAndUndup();
						codes_for_products.sortAndUndup();
						/*
						{
							BExtQuery q(&P_RefC->PsC, 1);
							EgaisPersonTbl::Key1 k1;
							MEMSZERO(k1);
							q.selectAll();
							EgaisPersonCore::Item item;
							for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
								if(P_RefC->PsC.RecToItem(P_RefC->PsC.data, item)) {
									if(sstrlen(item.INN) >= 8)
										inn_for_contragents.add(item.INN);
								}
							}
							inn_for_contragents.sortAndUndup();
						}
						*/
						PPLoadText(PPTXT_EGAIS_REQUESTFORIDB, msg_buf);
						PPWaitMsg(msg_buf);
						{
							long   done_q = 0;
							const  long total_q = refa_codes.getCount() + codes_for_contragents.getCount() + codes_for_products.getCount();
							uint  qc = 0;
							for(uint ssp = 0; refa_codes.get(&ssp, temp_buf);) {
								if(qc)
									SDelay(_delay_ms);
								if(QueryInfA(rParam.LocID, temp_buf))
									qc++;
								else
									LogLastError();
								done_q++;
								PPWaitPercent(done_q, total_q, msg_buf);
							}
							if(codes_for_contragents.getCount()) {
								for(uint ssp = 0; codes_for_contragents.get(&ssp, temp_buf);) {
									if(qc)
										SDelay(_delay_ms);
									if(QueryClients(rParam.LocID, querybyCode, temp_buf))
										qc++;
									else
										LogLastError();
									done_q++;
									PPWaitPercent(done_q, total_q, msg_buf);
								}
							}
							if(codes_for_products.getCount()) {
								for(uint ssp = 0; codes_for_products.get(&ssp, temp_buf);) {
									if(qc)
										SDelay(_delay_ms);
									if(QueryProducts(rParam.LocID, querybyCode, temp_buf))
										qc++;
									else
										LogLastError();
									done_q++;
									PPWaitPercent(done_q, total_q, msg_buf);
								}
							}
						}
						PPWaitStop();
					}
				}
				break;
			case (PPEDIOP_EGAIS_QUERYCLIENTS+3000):
				{
					const bool rmv_debug_mode = !rParam.ParamString.IsEqiAscii("yes");
					THROW(GetUtmList(rParam.LocID, utm_list));
					PPWaitStart();
					for(uint i = 0; i < utm_list.getCount(); i++) {
						SetUtmEntry(rParam.LocID, &utm_list.at(i), 0);
						RemoveOutputMessages(rParam.LocID, rmv_debug_mode);
					}
					PPWaitStop();
				}
				break;
			case PPEDIOP_EGAIS_NOTIFY_WBVER2:
				{
					Packet qp(rParam.DocType);
					if(PutQuery(qp, rParam.LocID, "InfoVersionTTN", ack))
						query_sended = 1;
					else
						do_report_error = 1;
				}
				break;
			case PPEDIOP_EGAIS_NOTIFY_WBVER3:
				{
					Packet qp(rParam.DocType);
					if(PutQuery(qp, rParam.LocID, "InfoVersionTTN", ack))
						query_sended = 1;
					else
						do_report_error = 1;
				}
				break;
			case PPEDIOP_EGAIS_NOTIFY_WBVER4:
				{
					Packet qp(rParam.DocType);
					if(PutQuery(qp, rParam.LocID, "InfoVersionTTN", ack))
						query_sended = 1;
					else
						do_report_error = 1;
				}
				break;
			case PPEDIOP_EGAIS_QUERYRESENDDOC:
				{
					Packet qp(rParam.DocType);
					if(qp.P_Data) {
						*static_cast<SString *>(qp.P_Data) = rParam.ParamString;
						if(PutQuery(qp, rParam.LocID, "QueryResendDoc", ack))
							query_sended = 1;
						else
							do_report_error = 1;
					}
				}
				break;
			case PPEDIOP_EGAIS_QUERYRESTBCODE:
				{
					Packet qp(rParam.DocType);
					if(qp.P_Data) {
						*static_cast<SString *>(qp.P_Data) = rParam.ParamString;
						if(PutQuery(qp, rParam.LocID, "QueryRestBCode", ack))
							query_sended = 1;
						else
							do_report_error = 1;
					}
				}
				break;
			case PPEDIOP_EGAIS_QUERYFORMA:
				if(QueryInfA(rParam.LocID, rParam.ParamString))
					query_sended = 1;
				else
					do_report_error = 1;
				break;
			case PPEDIOP_EGAIS_QUERYFORMB:
				if(QueryInfB(rParam.LocID, rParam.ParamString))
					query_sended = 1;
				else
					do_report_error = 1;
				break;
			case PPEDIOP_EGAIS_QUERYAP:
				if(QueryProducts(rParam.LocID, querybyINN, rParam.ParamString))
					query_sended = 1;
				else
					do_report_error = 1;
				break;
			case PPEDIOP_EGAIS_QUERYRESTS:
				if(QueryRests(rParam.LocID, 0))
					query_sended = 1;
				else
					do_report_error = 1;
				break;
			case PPEDIOP_EGAIS_QUERYRESTSSHOP:
				//
				// Непосредственно перед запросом остатков по регистру 2 запрашиваем остатки по складу. Для того, чтобы при создании
				// автоматического документа передачи со склада на регистр 2 проверять наличие заданной справки Б на складе.
				//
				if(QueryRests(rParam.LocID, 0)) {
					query_sended = 1;
					SDelay(100);
					if(QueryRestsShop(rParam.LocID, 0))
						query_sended = 1;
					else
						do_report_error = 1;
				}
				else
					do_report_error = 1;
				break;
			case PPEDIOP_EGAIS_QUERYBARCODE:
				if(rParam.ParamString.NotEmptyS()) {
					StringSet ss(' ', rParam.ParamString);
					Packet qp(PPEDIOP_EGAIS_QUERYBARCODE);
					TSCollection <QueryBarcode> * p_qbl = static_cast<TSCollection <QueryBarcode> *>(qp.P_Data);
					if(p_qbl) {
						if(ss.getCount() == 3) {
							QueryBarcode qb;
							uint   ssp = 0;
							ss.get(&ssp, temp_buf.Z());
							qb.CodeType = temp_buf.ToLong();
							ss.get(&ssp, temp_buf.Z());
							qb.Rank = temp_buf;
							ss.get(&ssp, temp_buf.Z());
							qb.Number = temp_buf;
							QueryBarcode * p_qb = p_qbl->CreateNewItem();
							THROW_SL(p_qb);
							*p_qb = qb;
						}
						else {
							SFsPath ps(rParam.ParamString);
							if(ps.Drv.IsEmpty() && ps.Dir.IsEmpty())
								PPGetFilePath(PPPATH_IN, rParam.ParamString, temp_buf);
							else
								temp_buf = rParam.ParamString;
							if(fileExists(temp_buf)) {
								SFile f_in(temp_buf, SFile::mRead);
								if(f_in.IsValid()) {
									while(f_in.ReadLine(temp_buf, SFile::rlfChomp)) {
										ss.setBuf(temp_buf);
										if(ss.getCount() == 3) {
											QueryBarcode qb;
											uint   ssp = 0;
											ss.get(&ssp, temp_buf.Z());
											qb.CodeType = temp_buf.ToLong();
											ss.get(&ssp, temp_buf.Z());
											qb.Rank = temp_buf;
											ss.get(&ssp, temp_buf.Z());
											qb.Number = temp_buf;
											QueryBarcode * p_qb = p_qbl->CreateNewItem();
											THROW_SL(p_qb);
											*p_qb = qb;
										}
									}
								}
							}
						}
						if(p_qbl->getCount()) {
							if(!PutQuery(qp, rParam.LocID, "QueryBarcode", ack))
								do_report_error = 1;
						}
					}
				}
				break;
		}
	}
	if(do_report_error) {
		PPGetLastErrorMessage(1, temp_buf);
		rParam.InfoText = temp_buf;
		LogLastError();
	}
	else if(query_sended) {

	}
	CATCHZOK
	DS.SetMainOrgID(preserve_main_org_id, 0);
	return ok;
}

int PPEgaisProcessor::InteractiveQuery()
{
	int    ok = -1;
	const  long  preserve_state = State;
	QueryParam _param;
	SETFLAG(_param.Flags, stTestSendingMode, GetTestSendingMode());
	_param.LocID = LConfig.Location;
	while(EditQueryParam(&_param) > 0) {
		SetTestSendingMode(LOGIC(_param.Flags & stTestSendingMode));
		THROW(ImplementQuery(_param));
		ok = 1;
	}
	CATCHZOKPPERR
	State = preserve_state;
	return ok;
}

/*static*/int PPEgaisProcessor::EditInformAReg(InformAReg & rData)
{
    int    ok = -1;
    uint   sel = 0;
    TDialog * dlg = new TDialog(DLG_EGAISREGA);
    THROW(CheckDialogPtr(&dlg));
    dlg->setCtrlLong(CTL_EGAISREGA_QTTY, rData.Qtty);
    dlg->setCtrlDate(CTL_EGAISREGA_MANUFDATE, rData.ManufDate);
    dlg->setCtrlDate(CTL_EGAISREGA_TTNDATE, rData.TTNDate);
    dlg->setCtrlString(CTL_EGAISREGA_TTNCODE, rData.TTNCode);
    dlg->setCtrlDate(CTL_EGAISREGA_EGAISD, rData.EGAISDate);
    dlg->setCtrlString(CTL_EGAISREGA_EGAISN, rData.EGAISCode);
    while(ok < 0 && ExecView(dlg) == cmOK) {
		rData.Qtty = dlg->getCtrlLong(sel = CTL_EGAISREGA_QTTY);
		if(rData.Qtty <= 0 || rData.Qtty > 1000000)
			PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
		else {
			rData.ManufDate = dlg->getCtrlDate(sel = CTL_EGAISREGA_MANUFDATE);
			if(!checkdate(rData.ManufDate))
				PPErrorByDialog(dlg, sel, PPERR_SLIB);
			else {
				rData.TTNDate = dlg->getCtrlDate(sel = CTL_EGAISREGA_TTNDATE);
				if(!checkdate(rData.TTNDate))
					PPErrorByDialog(dlg, sel, PPERR_SLIB);
				else {
					dlg->getCtrlString(sel = CTL_EGAISREGA_TTNCODE, rData.TTNCode);
					rData.EGAISDate = dlg->getCtrlDate(sel = CTL_EGAISREGA_EGAISD);
					if(!checkdate(rData.EGAISDate, 1))
						PPErrorByDialog(dlg, sel, PPERR_SLIB);
					else {
						dlg->getCtrlString(sel = CTL_EGAISREGA_EGAISN, rData.EGAISCode);
						ok = 1;
					}
				}
			}
		}
    }
    CATCHZOKPPERR
    return ok;
}

/*static*/int PPEgaisProcessor::InputMark(const PrcssrAlcReport::GoodsItem * pAgi, SString & rMark)
{
	class EgaisMarkDialog : public TDialog {
	public:
		EgaisMarkDialog(const PrcssrAlcReport::GoodsItem * pAgi) : TDialog(DLG_EGAISMARK), P_Agi(pAgi)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_EGAISMARK_INPUT)) {
				getCtrlString(CTL_EGAISMARK_INPUT, CodeBuf.Z());
				SString msg_buf, mark_buf;
				if(PrcssrAlcReport::IsEgaisMark(CodeBuf, &mark_buf) && PrcssrAlcReport::ParseEgaisMark(mark_buf, Mb) > 0)
					PPLoadTextS(PPTXT_EGAISMARKVALID, msg_buf).CR().Cat(Mb.EgaisCode);
				else
					PPLoadError(PPERR_TEXTISNTEGAISMARK, msg_buf, CodeBuf);
				setStaticText(CTL_EGAISMARK_INFO, msg_buf);
			}
		}
		SString CodeBuf;
		const PrcssrAlcReport::GoodsItem * P_Agi;
		PrcssrAlcReport::EgaisMarkBlock Mb;
	};

	rMark.Z();

    int    ok = -1;
	SString temp_buf;
    PrcssrAlcReport::EgaisMarkBlock mb;
    EgaisMarkDialog * dlg = new EgaisMarkDialog(pAgi);
    THROW(CheckDialogPtr(&dlg));
	if(pAgi) {
		SString line_buf;
		GetGoodsName(pAgi->GoodsID, temp_buf);
		line_buf = temp_buf;
		if(pAgi->CategoryCode.NotEmpty())
			line_buf.CR().Cat(pAgi->CategoryCode);
		if(pAgi->CategoryName.NotEmpty()) {
			(temp_buf = pAgi->CategoryName).Transf(CTRANSF_OUTER_TO_INNER);
			line_buf.CR().Cat(temp_buf);
		}
		dlg->setStaticText(CTL_EGAISMARK_AGI, line_buf);
	}
    while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_EGAISMARK_INPUT, temp_buf);
		if(PrcssrAlcReport::IsEgaisMark(temp_buf, &rMark) && PrcssrAlcReport::ParseEgaisMark(rMark, mb) > 0) {
			ok = 1;
		}
		else {
			PPSetError(PPERR_TEXTISNTEGAISMARK, temp_buf);
			PPErrorByDialog(dlg, CTL_EGAISMARK_INPUT);
			TInputLine * p_il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_EGAISMARK_INPUT));
			CALLPTRMEMB(p_il, selectAll(1));
			rMark.Z();
		}
    }
    CATCHZOKPPERR
    delete dlg;
    return ok;
}

int TestEGAIS(const PPEgaisProcessor::TestParam * pParam)
{
	int    ok = 1;
	TDialog * dlg = 0;
	SString temp_buf, file_name;
	PPEgaisProcessor::TestParam param;
	if(!RVALUEPTR(param, pParam)) {
		MEMSZERO(param);
		dlg = new TDialog(DLG_TESTEGAIS);
		THROW(CheckDialogPtr(&dlg));
		param.Flags |= PPEgaisProcessor::TestParam::fReceiveDataToTempCatalog;
		dlg->AddClusterAssoc(CTL_TESTEGAIS_FLAGS, 0, PPEgaisProcessor::TestParam::fReceiveDataToTempCatalog);
		dlg->AddClusterAssoc(CTL_TESTEGAIS_FLAGS, 1, PPEgaisProcessor::TestParam::fAcceptDataFromTempCatalog);
		dlg->SetClusterData(CTL_TESTEGAIS_FLAGS, param.Flags);
		SetupLocationCombo(dlg, CTLSEL_TESTEGAIS_LOC, param.LocID, 0, LOCTYP_WAREHOUSE, 0);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_TESTEGAIS_LOC, &param.LocID);
			dlg->GetClusterData(CTL_TESTEGAIS_FLAGS, &param.Flags);
		}
		else
			ok = -1;
		ZDELETE(dlg);
	}
	if(ok > 0) {
		PPEgaisProcessor ep(PPEgaisProcessor::cfUseVerByConfig, 0, 0); // @instantiation(PPEgaisProcessor)
		THROW(ep);
		PPWaitStart();
		{
			TSVector <PPEgaisProcessor::UtmEntry> utm_list;
			THROW(ep.GetUtmList(param.LocID, utm_list));
			for(uint i = 0; i < utm_list.getCount(); i++) {
				ep.SetUtmEntry(param.LocID, &utm_list.at(i), 0);
				if(param.Flags & PPEgaisProcessor::TestParam::fReceiveDataToTempCatalog) {
					THROW(ep.DebugReadInput(param.LocID));
				}
				if(param.Flags & PPEgaisProcessor::TestParam::fAcceptDataFromTempCatalog) {
					THROW(ep.ReadInput(param.LocID, 0, PPEgaisProcessor::rifOffline));
				}
				ep.SetUtmEntry(0, 0, 0);
			}
		}
		if(!(param.Flags & (PPEgaisProcessor::TestParam::fAcceptDataFromTempCatalog|PPEgaisProcessor::TestParam::fReceiveDataToTempCatalog))) {
			ep.CollectRefs();
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

EgaisPersonCore::Item::Item()
{
	Clear();
}

void EgaisPersonCore::Item::Clear()
{
	ID = 0;
	PTR32(RarIdent)[0] = 0;
	PTR32(INN)[0] = 0;
	PTR32(KPP)[0] = 0;
	PTR32(UNP)[0] = 0;
	PTR32(RNN)[0] = 0;
	CountryCode = 0;
	RegionCode = 0;
	Name.Z();
	FullName.Z();
	AddressDescr.Z();
	Flags = 0;
	ActualDate = ZERODATE;
}

int EgaisPersonCore::Item::IsEq(const EgaisPersonCore::Item & rS, long cmpflags) const
{
    int    yes = 1;
    if(!(cmpflags & cmpfExceptID) && ID != rS.ID)
		yes = 0;
	else if(!sstreq(RarIdent, rS.RarIdent))
		yes = 0;
	else if(!sstreq(INN, rS.INN))
		yes = 0;
	else if(!sstreq(KPP, rS.KPP))
		yes = 0;
	else if(!sstreq(UNP, rS.UNP))
		yes = 0;
	else if(!sstreq(RNN, rS.RNN))
		yes = 0;
	else if(CountryCode != rS.CountryCode)
		yes = 0;
	else if(RegionCode != rS.RegionCode)
		yes = 0;
	else if(Name != rS.Name)
		yes = 0;
	else if(FullName != rS.FullName)
		yes = 0;
	else if(AddressDescr != rS.AddressDescr)
		yes = 0;
	else if(!(cmpflags & cmpfExceptFlags) && Flags != rS.Flags)
		yes = 0;
	else if(ActualDate != rS.ActualDate)
		yes = 0;
	return yes;
}

EgaisPersonCore::EgaisPersonCore() : EgaisPersonTbl()
{
}

int EgaisPersonCore::RecToItem(const EgaisPersonTbl::Rec & rRec, EgaisPersonCore::Item & rItem)
{
	int    ok = 1;
	SString temp_buf;
	rItem.ID = rRec.ID;
	STRNSCPY(rItem.RarIdent, rRec.RarIdent);
	STRNSCPY(rItem.INN, rRec.INN);
	STRNSCPY(rItem.KPP, rRec.KPP);
	STRNSCPY(rItem.UNP, rRec.UNP);
	rItem.CountryCode = rRec.CountryCode;
	rItem.RegionCode = rRec.RegionCode;
	rItem.Flags = rRec.Flags;
	rItem.ActualDate = rRec.ActualDate;
	{
		Reference * p_ref = PPRef;
		THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rRec.ID, PPTRPROP_NAME), rItem.Name));
		rItem.Name.Transf(CTRANSF_UTF8_TO_INNER);
		THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rRec.ID, PPTRPROP_LONGNAME), rItem.FullName));
		rItem.FullName.Transf(CTRANSF_UTF8_TO_INNER);
		THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rRec.ID, txtprpAddressDescr), rItem.AddressDescr));
		rItem.AddressDescr.Transf(CTRANSF_UTF8_TO_INNER);
	}
	CATCHZOK
	return ok;
}

int EgaisPersonCore::Search(PPID id, EgaisPersonCore::Item & rItem)
{
	rItem.Clear();
	int    ok = -1;
	EgaisPersonTbl::Rec rec;
	int    r = SearchByID(this, PPOBJ_EGAISPERSON, id, &rec);
	if(r > 0) {
		RecToItem(rec, rItem);
		ok = 1;
	}
	return ok;
}

int EgaisPersonCore::IsVerifiedCode(const char * pRarCode)
{
	int    ok = -1;
	TSVector <EgaisPersonTbl::Rec> list;
	const int ap = SearchByCode(pRarCode, list);
	if(ap > 0) {
		for(uint i = 0; ok < 0 && i < list.getCount(); i++)
			if(list.at(i).Flags & rolefVerified)
				ok = 1;
	}
	else if(ap == 0)
		ok = 0;
	return ok;
}

int EgaisPersonCore::SearchByCode(const char * pRarCode, TSVector <EgaisPersonTbl::Rec> & rList)
{
	rList.clear();
	int    ok = -1;
	EgaisPersonTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.RarIdent, pRarCode);
	if(search(1, &k1, spEq)) do {
		THROW_SL(rList.insert(&data));
		ok = 1;
	} while(search(1, &k1, spNext) && sstreq(data.RarIdent, pRarCode));
	if(ok > 0) {
		const uint _c = rList.getCount();
		if(_c == 1)
			ok = 1;
		else {
			uint   actual_pos = 0; // +1
			LDATE  last_dt = ZERODATE;
			for(uint i = 0; i < _c; i++) {
				const EgaisPersonTbl::Rec & r_item = rList.at(i);
				if(r_item.ActualDate > last_dt)
					actual_pos = i+1;
			}
			ok = static_cast<int>(actual_pos);
		}
	}
	CATCHZOK
	return ok;
}

int EgaisPersonCore::Put(PPID * pID, EgaisPersonCore::Item * pItem, long * pConflictFlags, int use_ta)
{
	int    ok = 1;
	long   conflict_flags = 0;
	SString temp_buf;
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			Item org_item;
			THROW(Search(*pID, org_item) > 0);
			if(!pItem) {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(deleteRec());
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_NAME), static_cast<const wchar_t *>(0), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_LONGNAME), static_cast<const wchar_t *>(0), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, txtprpAddressDescr), static_cast<const wchar_t *>(0), 0));
			}
			else if(pItem->IsEq(org_item, 0)) {
				ok = -1;
			}
			else {
				EgaisPersonTbl::Rec rec;
				rec.ID = *pID;
				STRNSCPY(rec.RarIdent, pItem->RarIdent);
				STRNSCPY(rec.INN, pItem->INN);
				STRNSCPY(rec.KPP, pItem->KPP);
				STRNSCPY(rec.UNP, pItem->UNP);
				STRNSCPY(rec.RNN, pItem->RNN);
				rec.CountryCode = pItem->CountryCode;
				rec.RegionCode = pItem->RegionCode;
				rec.Flags = pItem->Flags;
				rec.ActualDate = pItem->ActualDate;
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(updateRecBuf(&rec));
				//
				// Пустые текстовые поля не вносим на случай, если в БД есть не пустое сооте
				//
				if(pItem->Name.NotEmptyS()) {
					(temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8);
					THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_NAME), temp_buf, 0));
				}
				if(pItem->FullName.NotEmptyS()) {
					(temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8);
					THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_LONGNAME), temp_buf, 0));
				}
				if(pItem->AddressDescr.NotEmptyS()) {
					(temp_buf = pItem->AddressDescr).Transf(CTRANSF_INNER_TO_UTF8);
					THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, txtprpAddressDescr), temp_buf, 0));
				}
			}
		}
		else {
			EgaisPersonTbl::Rec rec;
			EgaisPersonTbl::Key1 k1;
			MEMSZERO(k1);
			STRNSCPY(k1.RarIdent, pItem->RarIdent);
			if(search(1, &k1, spEq)) {
				ok = -1;
                int   do_update = 0;
                copyBufTo(&rec);
                if(pItem->INN[0]) {
					if(rec.INN[0] == 0) {
						STRNSCPY(rec.INN, pItem->INN);
						do_update = 1;
					}
					else if(!sstreq(rec.INN, pItem->INN)) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							STRNSCPY(rec.INN, pItem->INN);
							do_update = 1;
						}
						conflict_flags |= 0x0001;
					}
                }
                if(pItem->KPP[0]) {
					if(rec.KPP[0] == 0) {
						STRNSCPY(rec.KPP, pItem->KPP);
						do_update = 1;
					}
					else if(!sstreq(rec.KPP, pItem->KPP)) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							STRNSCPY(rec.KPP, pItem->KPP);
							do_update = 1;
						}
						conflict_flags |= 0x0002;
					}
                }
                if(pItem->UNP[0]) {
					if(rec.UNP[0] == 0) {
						STRNSCPY(rec.UNP, pItem->UNP);
						do_update = 1;
					}
					else if(!sstreq(rec.UNP, pItem->UNP)) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							STRNSCPY(rec.UNP, pItem->UNP);
							do_update = 1;
						}
						conflict_flags |= 0x0004;
					}
                }
                if(pItem->RNN[0]) {
					if(rec.RNN[0] == 0) {
						STRNSCPY(rec.RNN, pItem->RNN);
						do_update = 1;
					}
					else if(!sstreq(rec.RNN, pItem->RNN)) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							STRNSCPY(rec.RNN, pItem->RNN);
							do_update = 1;
						}
						conflict_flags |= 0x0008;
					}
                }
                if(pItem->CountryCode) {
					if(!rec.CountryCode) {
						rec.CountryCode = pItem->CountryCode;
						do_update = 1;
					}
					else if(rec.CountryCode != pItem->CountryCode) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							rec.CountryCode = pItem->CountryCode;
							do_update = 1;
						}
						conflict_flags |= 0x0010;
					}
                }
                if(pItem->RegionCode) {
					if(!rec.RegionCode) {
						rec.RegionCode = pItem->RegionCode;
						do_update = 1;
					}
					else if(rec.RegionCode != pItem->RegionCode) {
						if(pItem->Flags & EgaisPersonCore::rolefVerified) {
							rec.RegionCode = pItem->RegionCode;
							do_update = 1;
						}
						conflict_flags |= 0x0020;
					}
                }
				if(rec.Flags != pItem->Flags) {
					rec.Flags |= pItem->Flags;
					do_update = 1;
				}
				if(pItem->ActualDate > rec.ActualDate) {
					rec.ActualDate = pItem->ActualDate;
					do_update = 1;
				}
				{
					SString name, full_name, address_descr;
					THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, PPTRPROP_NAME), name));
					name.Transf(CTRANSF_UTF8_TO_INNER);
					THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, PPTRPROP_LONGNAME), full_name));
					full_name.Transf(CTRANSF_UTF8_TO_INNER);
					THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, txtprpAddressDescr), address_descr));
					address_descr.Transf(CTRANSF_UTF8_TO_INNER);
					if(pItem->Name.Len() > name.Len()) {
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, PPTRPROP_NAME), (temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8), 0));
						ok = 1;
					}
					if(pItem->FullName.Len() > full_name.Len()) {
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, PPTRPROP_LONGNAME), (temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8), 0));
						ok = 1;
					}
					if(pItem->AddressDescr.Len() > address_descr.Len()) {
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, rec.ID, txtprpAddressDescr), (temp_buf = pItem->AddressDescr).Transf(CTRANSF_INNER_TO_UTF8), 0));
						ok = 1;
					}
				}
				if(do_update) {
					THROW_DB(rereadForUpdate(0, 0));
					THROW_DB(updateRecBuf(&rec));
					ok = 1;
				}
			}
			else {
				STRNSCPY(rec.RarIdent, pItem->RarIdent);
				STRNSCPY(rec.INN, pItem->INN);
				STRNSCPY(rec.KPP, pItem->KPP);
				STRNSCPY(rec.UNP, pItem->UNP);
				STRNSCPY(rec.RNN, pItem->RNN);
				rec.CountryCode = pItem->CountryCode;
				rec.RegionCode = pItem->RegionCode;
				rec.Flags = pItem->Flags;
				rec.ActualDate = pItem->ActualDate;
				THROW_DB(insertRecBuf(&rec, 0, pID));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_NAME), (temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, PPTRPROP_LONGNAME), (temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, *pID, txtprpAddressDescr), (temp_buf = pItem->AddressDescr).Transf(CTRANSF_INNER_TO_UTF8), 0));
			}
		}
		THROW(tra.Commit());
	}
    CATCHZOK
	ASSIGN_PTR(pConflictFlags, conflict_flags);
    return ok;
}

int EgaisPersonCore::Clear(int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	IterCounter cntr;
	SString msg_buf;
	EgaisPersonTbl::Key0 k0;
	MEMSZERO(k0);
	PPInitIterCounter(cntr, this);
	PPLoadText(PPTXT_EGAIS_CLEARINNEREGAISDB, msg_buf);
	msg_buf.CatDiv(':', 2).Cat(this->GetTableName());
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(search(0, &k0, spFirst)) do {
			const  PPID id = data.ID;
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(deleteRec());
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, id, PPTRPROP_NAME), static_cast<const wchar_t *>(0), 0));
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, id, PPTRPROP_LONGNAME), static_cast<const wchar_t *>(0), 0));
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPERSON, id, txtprpAddressDescr), static_cast<const wchar_t *>(0), 0));
			PPWaitPercent(cntr.Increment(), msg_buf);
			ok = 1;
		} while(search(0, &k0, spNext));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int EgaisPersonCore::Export(long fmt, const char * pFileName)
{
	int    ok = 1;
	SFile  f_out(pFileName, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	{
		ulong  rec_no = 0;
		SString line_buf;
		BExtQuery q(this, 1);
		EgaisPersonTbl::Key1 k1;
		MEMSZERO(k1);
		q.selectAll();
		Item item;
		for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
			if(RecToItem(data, item)) {
				if(rec_no == 0) {
					line_buf.Z().Cat("ID").Tab().Cat("RarIdent").Tab().
						Cat("INN").Tab().Cat("KPP").Tab().Cat("UNP").Tab().Cat("RNN").Tab().
						Cat("CountryCode").Tab().Cat("RegionCode").Tab().Cat("Flags").Tab().
						Cat("Name").Tab().Cat("FullName").Tab().Cat("AddrDescr").CR();
					f_out.WriteLine(line_buf);
				}
				line_buf.Z().Cat(item.ID).Tab().Cat(item.RarIdent).Tab().Cat(item.INN).Tab().Cat(item.KPP).Tab().
					Cat(item.UNP).Tab().Cat(item.RNN).Tab().Cat(item.CountryCode).Tab().Cat(item.RegionCode).Tab().
					CatHex(item.Flags).Tab().Cat(item.Name.Transf(CTRANSF_INNER_TO_OUTER)).Tab().Cat(item.FullName.Transf(CTRANSF_INNER_TO_OUTER)).Tab().Cat(item.AddressDescr.Transf(CTRANSF_INNER_TO_OUTER)).CR();
				f_out.WriteLine(line_buf);
				rec_no++;
			}
		}
	}
    CATCHZOK
	return ok;
}

EgaisProductCore::Item::Item()
{
	Clear();
}

void EgaisProductCore::Item::Clear()
{
	ID = 0;
	Flags = 0;
	AlcoCode[0] = 0;
	ManufRarIdent[0] = 0;
	ImporterRarIdent[0] = 0;
	CategoryCode[0] = 0;
	Proof = 0.0;
	Volume = 0.0;
	Name.Z();
	FullName.Z();
	ActualDate = ZERODATE;
}

int EgaisProductCore::Item::IsEq(const Item & rS, long cmpflags) const
{
	int    yes = 1;
	if(!(cmpflags & cmpfExceptID) && ID != rS.ID)
		yes = 0;
	else if(Flags != rS.Flags)
		yes = 0;
	else if(Proof != rS.Proof)
		yes = 0;
	else if(Volume != rS.Volume)
		yes = 0;
	else if(!sstreq(AlcoCode, rS.AlcoCode))
		yes = 0;
	else if(!sstreq(ManufRarIdent, rS.ManufRarIdent))
		yes = 0;
	else if(!sstreq(ImporterRarIdent, rS.ImporterRarIdent))
		yes = 0;
	else if(!sstreq(CategoryCode, rS.CategoryCode))
		yes = 0;
	else if(Name != rS.Name)
		yes = 0;
	else if(FullName != rS.FullName)
		yes = 0;
	else if(ActualDate != rS.ActualDate)
		yes = 0;
	return yes;
}

EgaisProductCore::EgaisProductCore() : EgaisProductTbl()
{
}

int EgaisProductCore::RecToItem(const EgaisProductTbl::Rec & rRec, EgaisProductCore::Item & rItem)
{
	int    ok = 1;
	rItem.ID = rRec.ID;
	rItem.Proof = fdiv1000i(rRec.Proof);
	rItem.Volume = static_cast<double>(rRec.Volume) / 100000.0;
	rItem.ActualDate = rRec.ActualDate;
	rItem.Flags = rRec.Flags;
	STRNSCPY(rItem.AlcoCode, rRec.AlcCode);
	STRNSCPY(rItem.ManufRarIdent, rRec.ManufRarIdent);
	STRNSCPY(rItem.ImporterRarIdent, rRec.ImporterRarIdent);
	STRNSCPY(rItem.CategoryCode, rRec.CategoryCode);
	{
		Reference * p_ref = PPRef;
		THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rRec.ID, PPTRPROP_NAME), rItem.Name));
		rItem.Name.Transf(CTRANSF_UTF8_TO_INNER);
		THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rRec.ID, PPTRPROP_LONGNAME), rItem.FullName));
		rItem.FullName.Transf(CTRANSF_UTF8_TO_INNER);
	}
	CATCHZOK
	return ok;
}

int EgaisProductCore::Search(PPID id, EgaisProductCore::Item & rItem)
{
	rItem.Clear();
	int    ok = -1;
	EgaisProductTbl::Rec rec;
	int    r = SearchByID(this, PPOBJ_EGAISPRODUCT, id, &rec);
	if(r > 0) {
		RecToItem(rec, rItem);
		ok = 1;
	}
	return ok;
}

int EgaisProductCore::IsVerifiedCode(const char * pAlcoCode)
{
	int    ok = -1;
	TSVector <EgaisProductTbl::Rec> list;
	const int ap = SearchByCode(pAlcoCode, list);
	if(ap > 0) {
		for(uint i = 0; ok < 0 && i < list.getCount(); i++) {
			const EgaisProductTbl::Rec & r_rec = list.at(i);
			if(r_rec.Flags & fVerified)
				ok = 1;
		}
	}
	else if(ap == 0)
		ok = 0;
	return ok;
}

int EgaisProductCore::SearchByCode(const char * pAlcoCode, TSVector <EgaisProductTbl::Rec> & rList)
{
	rList.clear();
	int    ok = -1;
	EgaisProductTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.AlcCode, pAlcoCode);
	if(search(1, &k1, spEq)) do {
		THROW_SL(rList.insert(&data));
		ok = 1;
	} while(search(1, &k1, spNext) && sstreq(data.AlcCode, pAlcoCode));
	if(ok > 0) {
		const uint _c = rList.getCount();
		if(_c == 1)
			ok = 1;
		else {
			uint   actual_pos = 0; // +1
			LDATE  last_dt = ZERODATE;
			for(uint i = 0; i < _c; i++) {
				const EgaisProductTbl::Rec & r_item = rList.at(i);
				if(r_item.ActualDate > last_dt)
					actual_pos = i+1;
			}
			ok = static_cast<int>(actual_pos);
		}
	}
	CATCHZOK
	return ok;
}

int EgaisProductCore::Put(PPID * pID, const EgaisProductCore::Item * pItem, long * pConflictFlags, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	long   conflict_flags = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			Item org_item;
			THROW(Search(*pID, org_item) > 0);
			if(!pItem) {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(deleteRec());
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_NAME), static_cast<const wchar_t *>(0), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_LONGNAME), static_cast<const wchar_t *>(0), 0));
			}
			else if(pItem->IsEq(org_item, 0)) {
				ok = -1;
			}
			else {
				EgaisProductTbl::Rec rec;
				rec.ID = *pID;
				rec.Proof = static_cast<long>(pItem->Proof * 1000.0);
				rec.Volume = static_cast<long>(pItem->Volume * 100000.0);
				STRNSCPY(rec.AlcCode, pItem->AlcoCode);
				STRNSCPY(rec.ManufRarIdent, pItem->ManufRarIdent);
				STRNSCPY(rec.ImporterRarIdent, pItem->ImporterRarIdent);
				STRNSCPY(rec.CategoryCode, pItem->CategoryCode);
				rec.ActualDate = pItem->ActualDate;
				rec.Flags = pItem->Flags;
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(updateRecBuf(&rec));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_NAME), (temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_LONGNAME), (temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8), 0));
			}
		}
		else {
			const int32 new_proof  = static_cast<long>(pItem->Proof * 1000.0);
			const int32 new_volume = static_cast<long>(pItem->Volume * 100000.0);
			EgaisProductTbl::Key1 k1;
			MEMSZERO(k1);
			STRNSCPY(k1.AlcCode, pItem->AlcoCode);
			if(search(1, &k1, spEq)) {
				ok = -1;
				int    do_update = 0;
				EgaisProductTbl::Rec rec;
				copyBufTo(&rec);
				if(pItem->AlcoCode[0]) {
					if(rec.AlcCode[0] == 0) {
						STRNSCPY(rec.AlcCode, pItem->AlcoCode);
						do_update = 1;
					}
					else if(!sstreq(rec.AlcCode, pItem->AlcoCode)) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							STRNSCPY(rec.AlcCode, pItem->AlcoCode);
							do_update = 1;
						}
						conflict_flags |= 0x0001;
					}
				}
				if(pItem->CategoryCode[0]) {
					if(rec.CategoryCode[0] == 0) {
						STRNSCPY(rec.CategoryCode, pItem->CategoryCode);
						do_update = 1;
					}
					else if(!sstreq(rec.CategoryCode, pItem->CategoryCode)) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							STRNSCPY(rec.CategoryCode, pItem->CategoryCode);
							do_update = 1;
						}
						conflict_flags |= 0x0002;
					}
				}
				if(pItem->ManufRarIdent[0]) {
					if(rec.ManufRarIdent[0] == 0) {
						STRNSCPY(rec.ManufRarIdent, pItem->ManufRarIdent);
						do_update = 1;
					}
					else if(!sstreq(rec.ManufRarIdent, pItem->ManufRarIdent)) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							STRNSCPY(rec.ManufRarIdent, pItem->ManufRarIdent);
							do_update = 1;
						}
						conflict_flags |= 0x0004;
					}
				}
				if(pItem->ImporterRarIdent[0]) {
					if(rec.ImporterRarIdent[0] == 0) {
						STRNSCPY(rec.ImporterRarIdent, pItem->ImporterRarIdent);
						do_update = 1;
					}
					else if(!sstreq(rec.ImporterRarIdent, pItem->ImporterRarIdent)) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							STRNSCPY(rec.ImporterRarIdent, pItem->ImporterRarIdent);
							do_update = 1;
						}
						conflict_flags |= 0x0008;
					}
				}
				if(new_proof > 0) {
					if(rec.Proof <= 0) {
						rec.Proof = new_proof;
						do_update = 1;
					}
					else if(new_proof != rec.Proof) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							rec.Proof = new_proof;
							do_update = 1;
						}
						conflict_flags |= 0x0010;
					}
				}
				if(new_volume > 0) {
					if(rec.Volume <= 0) {
						rec.Volume = new_volume;
						do_update = 1;
					}
					else if(new_volume != rec.Volume) {
						if(pItem->Flags & EgaisProductCore::fVerified) {
							rec.Volume = new_volume;
							do_update = 1;
						}
						conflict_flags |= 0x0020;
					}
				}
				{
					SString name, full_name;
					THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rec.ID, PPTRPROP_NAME), name));
					name.Transf(CTRANSF_UTF8_TO_INNER);
					THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rec.ID, PPTRPROP_LONGNAME), full_name));
					full_name.Transf(CTRANSF_UTF8_TO_INNER);
					if(pItem->Name.Len() > name.Len()) {
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rec.ID, PPTRPROP_NAME), (temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8), 0));
						ok = 1;
					}
					if(pItem->FullName.Len() > full_name.Len()) {
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, rec.ID, PPTRPROP_LONGNAME), (temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8), 0));
						ok = 1;
					}
				}
				if(pItem->Flags & EgaisProductCore::fVerified && !(rec.Flags & EgaisProductCore::fVerified)) {
					rec.Flags |= EgaisProductCore::fVerified;
					do_update = 1;
				}
				if(do_update) {
					THROW_DB(rereadForUpdate(0, 0));
					THROW_DB(updateRecBuf(&rec));
					ok = 1;
				}
			}
			else {
				EgaisProductTbl::Rec rec;
				rec.Proof = new_proof;
				rec.Volume = new_volume;
				STRNSCPY(rec.AlcCode, pItem->AlcoCode);
				STRNSCPY(rec.ManufRarIdent, pItem->ManufRarIdent);
				STRNSCPY(rec.ImporterRarIdent, pItem->ImporterRarIdent);
				STRNSCPY(rec.CategoryCode, pItem->CategoryCode);
				rec.ActualDate = pItem->ActualDate;
				rec.Flags = pItem->Flags;
				THROW_DB(insertRecBuf(&rec, 0, pID));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_NAME), (temp_buf = pItem->Name).Transf(CTRANSF_INNER_TO_UTF8), 0));
				THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, *pID, PPTRPROP_LONGNAME), (temp_buf = pItem->FullName).Transf(CTRANSF_INNER_TO_UTF8), 0));
			}
		}
		THROW(tra.Commit());
	}
    CATCHZOK
    ASSIGN_PTR(pConflictFlags, conflict_flags);
    return ok;
}

int EgaisProductCore::Clear(int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	IterCounter cntr;
	SString msg_buf;
	EgaisProductTbl::Key0 k0;
	MEMSZERO(k0);
	PPInitIterCounter(cntr, this);
	PPLoadText(PPTXT_EGAIS_CLEARINNEREGAISDB, msg_buf);
	msg_buf.CatDiv(':', 2).Cat(this->GetTableName());
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(search(0, &k0, spFirst)) do {
			const  PPID id = data.ID;
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(deleteRec());
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, id, PPTRPROP_NAME), static_cast<const wchar_t *>(0), 0));
			THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EGAISPRODUCT, id, PPTRPROP_LONGNAME), static_cast<const wchar_t *>(0), 0));
			PPWaitPercent(cntr.Increment(), msg_buf);
			ok = 1;
		} while(search(0, &k0, spNext));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int EgaisProductCore::Export(long fmt, const char * pFileName)
{
	int    ok = 1;
	SFile  f_out(pFileName, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	{
		ulong  rec_no = 0;
		SString line_buf;
		BExtQuery q(this, 1);
		EgaisProductTbl::Key1 k1;
		MEMSZERO(k1);
		q.selectAll();
		Item item;
		for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
			if(RecToItem(data, item)) {
				if(rec_no == 0) {
					line_buf.Z().Cat("ID").Tab().Cat("AlcoCode").Tab().
						Cat("ManufRarIdent").Tab().Cat("ImporterRarIdent").Tab().Cat("CategoryCode").Tab().Cat("Proof").Tab().
						Cat("Volume").Tab().Cat("Name").Tab().Cat("FullName").CR();
					f_out.WriteLine(line_buf);
				}
				line_buf.Z().Cat(item.ID).Tab().Cat(item.AlcoCode).Tab().Cat(item.ManufRarIdent).Tab().Cat(item.ImporterRarIdent).Tab().
					Cat(item.CategoryCode).Tab().Cat(item.Proof, MKSFMTD_020).Tab().Cat(item.Volume, MKSFMTD_030).Tab().
					Cat(item.Name.Transf(CTRANSF_INNER_TO_OUTER)).Tab().Cat(item.FullName.Transf(CTRANSF_INNER_TO_OUTER)).CR();
				f_out.WriteLine(line_buf);
				rec_no++;
			}
		}
	}
    CATCHZOK
	return ok;
}

/*static*/int FASTCALL EgaisRefACore::IsRecEq(const EgaisRefATbl::Rec & rR1, const EgaisRefATbl::Rec & rR2)
{
#define CMP_MEMB(m)  if(rR1.m != rR2.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rR1.m, rR2.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMBS(RefACode);
	CMP_MEMBS(AlcCode);
	CMP_MEMBS(ManufRarIdent);
	CMP_MEMBS(ImporterRarIdent);
	CMP_MEMBS(ShipperRarIdent);
	CMP_MEMBS(ConsigneeRarIdent);
	CMP_MEMBS(EgaisNumber);
	CMP_MEMBS(TTNNumber);
	CMP_MEMB(EgaisDate);
	CMP_MEMB(TTNDate);
	CMP_MEMB(CountryCode);
	CMP_MEMB(Volume);
	CMP_MEMB(BottlingDate);
	CMP_MEMB(ActualDate);
#undef CMP_MEMBS
#undef CMP_MEMB
	return 1;
}

EgaisRefACore::EgaisRefACore() : EgaisRefATbl()
{
}

int EgaisRefACore::Search(PPID id, EgaisRefATbl::Rec * pRec) { return SearchByID(this, PPOBJ_EGAISREFA, id, pRec); }

int EgaisRefACore::SearchByCode(const char * pRefACode, TSVector <EgaisRefATbl::Rec> & rList)
{
	rList.clear();
	int    ok = -1;
	EgaisRefATbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.RefACode, pRefACode);
	if(search(1, &k1, spEq)) do {
		THROW_SL(rList.insert(&data));
		ok = 1;
	} while(search(1, &k1, spNext) && sstreq(data.RefACode, pRefACode));
	if(ok > 0) {
		const uint _c = rList.getCount();
		if(_c == 1)
			ok = 1;
		else {
			uint   actual_pos = 0; // +1
			LDATE  last_dt = ZERODATE;
			for(uint i = 0; i < _c; i++) {
				const EgaisRefATbl::Rec & r_item = rList.at(i);
				if(r_item.ActualDate > last_dt)
					actual_pos = i+1;
			}
			ok = static_cast<int>(actual_pos);
		}
	}
	CATCHZOK
	return ok;
}

int EgaisRefACore::SearchByProductCode(const char * pAlcoCode, TSVector <EgaisRefATbl::Rec> & rList)
{
	rList.clear();
	int    ok = -1;
	EgaisRefATbl::Key2 k2;
	MEMSZERO(k2);
	STRNSCPY(k2.AlcCode, pAlcoCode);
	if(search(2, &k2, spEq)) do {
		THROW_SL(rList.insert(&data));
		ok = 1;
	} while(search(2, &k2, spNext) && sstreq(data.AlcCode, pAlcoCode));
	CATCHZOK
	return ok;
}

static int OnPut_CopyTextField(const char * pSrc, char * pDest, size_t destBufSize, bool isSrcVerified, long cf, long * pConflictFlags)
{
	int    do_update = 0;
	if(!isempty(pSrc)) {
		if(isempty(pDest)) {
			strnzcpy(pDest, pSrc, destBufSize);
			do_update = 1;
		}
		else if(!sstreq(pSrc, pDest)) {
			if(isSrcVerified) {
				strnzcpy(pDest, pSrc, destBufSize);
				do_update = 1;
			}
			if(pConflictFlags)
				*pConflictFlags |= cf;
		}
	}
	return do_update;
}

static int OnPut_CopyDateField(LDATE srcDate, LDATE * pDestDate, bool isSrcVerified, long cf, long * pConflictFlags)
{
	int    do_update = 0;
	if(checkdate(srcDate)) {
		if(!checkdate(*pDestDate)) {
			*pDestDate = srcDate;
			do_update = 1;
		}
		else if(srcDate != *pDestDate) {
			if(isSrcVerified) {
				*pDestDate = srcDate;
				do_update = 1;
			}
			if(pConflictFlags)
				*pConflictFlags |= cf;
		}
	}
	return do_update;
}

int EgaisRefACore::Put(PPID * pID, const EgaisRefATbl::Rec * pRec, long * pConflictFlags, int use_ta)
{
	int    ok = 1;
	long   conflict_flags = 0;
		// 0x0001 AlcCode
		// 0x0002 ManufRarIdent
		// 0x0004 ImporterRarIdent
		// 0x0008 Volume
		// 0x0010 BottlingDate
		// 0x0020 CountryCode
		// 0x0040 ShipperRarIdent
		// 0x0080 ConsigneeRarIdent
		// 0x0100 TTNNumber
		// 0x0200 EgaisNumber
		// 0x0400 EgaisDate
		// 0x0800 TTNDate
		// 0x1000 ActualDate
    if(*pID) {
		EgaisRefATbl::Rec org_rec;
		THROW(Search(*pID, &org_rec) > 0);
		if(!pRec) {
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(deleteRec());
		}
		else if(IsRecEq(*pRec, org_rec)) {
			ok = -1;
		}
		else {
			EgaisRefATbl::Rec rec;
			rec.ID = *pID;
			STRNSCPY(rec.RefACode, pRec->RefACode);
			STRNSCPY(rec.AlcCode, pRec->AlcCode);
			STRNSCPY(rec.ManufRarIdent, pRec->ManufRarIdent);
			STRNSCPY(rec.ImporterRarIdent, pRec->ImporterRarIdent);
			STRNSCPY(rec.ShipperRarIdent, pRec->ShipperRarIdent);
			STRNSCPY(rec.ConsigneeRarIdent, pRec->ConsigneeRarIdent);
			STRNSCPY(rec.EgaisNumber, pRec->EgaisNumber);
			STRNSCPY(rec.TTNNumber, pRec->TTNNumber);
			rec.CountryCode = pRec->CountryCode;
			rec.Volume = pRec->Volume;
			rec.BottlingDate = pRec->BottlingDate;
			rec.TTNDate = pRec->TTNDate;
			rec.EgaisDate = pRec->EgaisDate;
			rec.ActualDate = pRec->ActualDate;
			rec.Flags = pRec->Flags;
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(updateRecBuf(&rec));
		}
    }
    else {
		EgaisRefATbl::Key1 k1;
		MEMSZERO(k1);
		STRNSCPY(k1.RefACode, pRec->RefACode);
		if(search(1, &k1, spEq)) {
			ok = -1;
			int    do_update = 0;
			EgaisRefATbl::Rec rec;
			copyBufTo(&rec);
			if(OnPut_CopyTextField(pRec->AlcCode, rec.AlcCode, sizeof(rec.AlcCode), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x01, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->ManufRarIdent, rec.ManufRarIdent, sizeof(rec.ManufRarIdent), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x02, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->ImporterRarIdent, rec.ImporterRarIdent, sizeof(rec.ImporterRarIdent), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x04, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->ShipperRarIdent, rec.ShipperRarIdent, sizeof(rec.ShipperRarIdent), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x40, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->ConsigneeRarIdent, rec.ConsigneeRarIdent, sizeof(rec.ConsigneeRarIdent), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x80, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->TTNNumber, rec.TTNNumber, sizeof(rec.TTNNumber), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x100, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyTextField(pRec->EgaisNumber, rec.EgaisNumber, sizeof(rec.EgaisNumber), LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x200, &conflict_flags))
				do_update = 1;
			if(pRec->Volume > 0) {
                if(rec.Volume <= 0) {
					rec.Volume = pRec->Volume;
					do_update = 1;
                }
                else if(rec.Volume != pRec->Volume) {
					if(pRec->Flags & EgaisRefACore::fVerified) {
						rec.Volume = pRec->Volume;
						do_update = 1;
					}
					conflict_flags |= 0x0008;
				}
			}
			if(OnPut_CopyDateField(pRec->BottlingDate, &rec.BottlingDate, LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x10, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyDateField(pRec->EgaisDate, &rec.EgaisDate, LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x400, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyDateField(pRec->TTNDate, &rec.TTNDate, LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x800, &conflict_flags))
				do_update = 1;
			if(OnPut_CopyDateField(pRec->ActualDate, &rec.ActualDate, LOGIC(pRec->Flags & EgaisRefACore::fVerified), 0x1000, &conflict_flags))
				do_update = 1;
			if(pRec->CountryCode > 0) {
				if(rec.CountryCode <= 0) {
					rec.CountryCode = pRec->CountryCode;
					do_update = 1;
				}
				else if(pRec->CountryCode != rec.CountryCode) {
					if(pRec->Flags & EgaisRefACore::fVerified) {
						rec.CountryCode = pRec->CountryCode;
						do_update = 1;
					}
					conflict_flags |= 0x0020;
				}
			}
			if(pRec->Flags & EgaisRefACore::fVerified && !(rec.Flags & EgaisRefACore::fVerified)) {
				rec.Flags |= EgaisRefACore::fVerified;
				do_update = 1;
			}
			if(do_update) {
				THROW_DB(rereadForUpdate(0, 0));
				THROW_DB(updateRecBuf(&rec));
			}
		}
		else {
			EgaisRefATbl::Rec rec;
			STRNSCPY(rec.RefACode, pRec->RefACode);
			STRNSCPY(rec.AlcCode, pRec->AlcCode);
			STRNSCPY(rec.EgaisNumber, pRec->EgaisNumber);
			STRNSCPY(rec.TTNNumber, pRec->TTNNumber);
			STRNSCPY(rec.ManufRarIdent, pRec->ManufRarIdent);
			STRNSCPY(rec.ImporterRarIdent, pRec->ImporterRarIdent);
			STRNSCPY(rec.ShipperRarIdent, pRec->ShipperRarIdent);
			STRNSCPY(rec.ConsigneeRarIdent, pRec->ConsigneeRarIdent);
			rec.CountryCode = pRec->CountryCode;
			rec.EgaisDate = pRec->EgaisDate;
			rec.ActualDate = pRec->ActualDate;
			rec.TTNDate = pRec->TTNDate;
			rec.Volume = pRec->Volume;
			rec.BottlingDate = pRec->BottlingDate;
			rec.Flags = pRec->Flags;
			THROW_DB(insertRecBuf(&rec, 0, pID));
		}
    }
    CATCHZOK
    ASSIGN_PTR(pConflictFlags, conflict_flags);
	return ok;
}

int EgaisRefACore::Clear(int use_ta)
{
	int    ok = -1;
	IterCounter cntr;
	SString msg_buf;
	EgaisRefATbl::Key0 k0;
	MEMSZERO(k0);
	PPInitIterCounter(cntr, this);
	PPLoadText(PPTXT_EGAIS_CLEARINNEREGAISDB, msg_buf);
	msg_buf.CatDiv(':', 2).Cat(this->GetTableName());
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(search(0, &k0, spFirst)) do {
			const  PPID id = data.ID;
			THROW_DB(rereadForUpdate(0, 0));
			THROW_DB(deleteRec());
			PPWaitPercent(cntr.Increment(), msg_buf);
			ok = 1;
		} while(search(0, &k0, spNext));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int EgaisRefACore::Export(long fmt, const char * pFileName)
{
	int    ok = 1;
	SFile  f_out(pFileName, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	{
		ulong  rec_no = 0;
		SString line_buf;
		BExtQuery q(this, 1);
		EgaisRefATbl::Key1 k1;
		MEMSZERO(k1);
		q.selectAll();
		for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0;) {
			EgaisRefATbl::Rec item;
			copyBufTo(&item);
			if(rec_no == 0) {
				line_buf.Z().Cat("ID").Tab().Cat("RefACode").Tab().
					Cat("AlcCode").Tab().Cat("ManufRarIdent").Tab().Cat("ImporterRarIdent").Tab().Cat("CountryCode").Tab().
					Cat("Volume").Tab().Cat("BottlingDate").CR();
				f_out.WriteLine(line_buf);
			}
			line_buf.Z().Cat(item.ID).Tab().Cat(item.RefACode).Tab().Cat(item.AlcCode).Tab().Cat(item.ManufRarIdent).Tab().
				Cat(item.ImporterRarIdent).Tab().Cat(item.CountryCode).Tab().Cat(static_cast<double>(item.Volume)/100000.0, MKSFMTD_030).Tab().
				Cat(item.BottlingDate, DATF_DMY|DATF_CENTURY).CR();
			f_out.WriteLine(line_buf);
			rec_no++;
		}
	}
    CATCHZOK
	return ok;
}
//
//
//
PrcssrAlcReport::RefCollection::RefCollection() : LastPersonP(-1), LastProductP(-1), LastRefAP(-1)
{
}

int FASTCALL PrcssrAlcReport::RefCollection::SetPerson(const EgaisPersonCore::Item & rItem)
{
	int    ok = 1;
	int    result_pos = -1;
	for(uint pos = 0; result_pos < 0 && PersonList.lsearch(rItem.RarIdent, &pos, PTR_CMPFUNC(Pchar), offsetof(EgaisPersonCore::Item, RarIdent));) {
		EgaisPersonCore::Item * p_item = PersonList.at(pos);
		if(p_item && p_item->IsEq(rItem, EgaisPersonCore::Item::cmpfExceptFlags)) {
			p_item->Flags |= rItem.Flags;
			result_pos = static_cast<int>(pos);
		}
		pos++;
	}
	if(result_pos < 0) {
		EgaisPersonCore::Item * p_new_item = new EgaisPersonCore::Item(rItem);
		THROW_MEM(p_new_item);
		THROW_SL(PersonList.insert(p_new_item));
		result_pos = PersonList.getCount()-1;
	}
	LastPersonP = result_pos;
	CATCH
		ok = 0;
		LastPersonP = -1;
	ENDCATCH
	return ok;
}

int FASTCALL PrcssrAlcReport::RefCollection::SetProduct(const EgaisProductCore::Item & rItem)
{
	int    ok = 1;
	int    result_pos = -1;
	for(uint pos = 0; result_pos < 0 && ProductList.lsearch(rItem.AlcoCode, &pos, PTR_CMPFUNC(Pchar), offsetof(EgaisProductCore::Item, AlcoCode));) {
		const EgaisProductCore::Item * p_item = ProductList.at(pos);
		if(p_item && p_item->IsEq(rItem, 0)) {
			result_pos = static_cast<int>(pos);
		}
		pos++;
	}
	if(result_pos < 0) {
		EgaisProductCore::Item * p_new_item = new EgaisProductCore::Item(rItem);
		THROW_MEM(p_new_item);
		THROW_SL(ProductList.insert(p_new_item));
		result_pos = ProductList.getCount()-1;
	}
	LastProductP = result_pos;
	CATCH
		ok = 0;
		LastProductP = -1;
	ENDCATCH
	return ok;
}

int FASTCALL PrcssrAlcReport::RefCollection::SetRefA(EgaisRefATbl::Rec & rItem)
{
	int    ok = 1;
	int    result_pos = -1;
	for(uint pos = 0; result_pos < 0 && RefAList.lsearch(rItem.RefACode, &pos, PTR_CMPFUNC(Pchar), offsetof(EgaisRefATbl::Rec, RefACode));) {
		const EgaisRefATbl::Rec & r_item = RefAList.at(pos);
		if(EgaisRefACore::IsRecEq(r_item, rItem)) {
			result_pos = static_cast<int>(pos);
		}
		pos++;
	}
	if(result_pos < 0) {
		THROW_SL(RefAList.insert(&rItem));
		result_pos = RefAList.getCount()-1;
	}
	LastRefAP = result_pos;
	CATCH
		ok = 0;
		LastRefAP = -1;
	ENDCATCH
	return ok;
}

int PrcssrAlcReport::RefCollection::Store(int use_ta)
{
	int    ok = 1;
	long   conflict_flags = 0;
	uint   i;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < PersonList.getCount(); i++) {
			EgaisPersonCore::Item * p_item = PersonList.at(i);
			if(p_item)
				THROW(PsC.Put(&p_item->ID, p_item, &conflict_flags, 0));
		}
		for(i = 0; i < ProductList.getCount(); i++) {
			EgaisProductCore::Item * p_item = ProductList.at(i);
			if(p_item)
				THROW(PrC.Put(&p_item->ID, p_item, &conflict_flags, 0));
		}
		for(i = 0; i < RefAList.getCount(); i++) {
			EgaisRefATbl::Rec & r_item = RefAList.at(i);
			THROW(RaC.Put(&r_item.ID, &r_item, &conflict_flags, 0));
		}
		THROW(tra.Commit());
	}
	{
        SString dump_filename;
        THROW(PsC.Export(0, PPGetFilePathS(PPPATH_OUT, PPFILNAM_EGAIS_PERSON_TXT, dump_filename)));
        THROW(PrC.Export(0, PPGetFilePathS(PPPATH_OUT, PPFILNAM_EGAIS_PRODUCT_TXT, dump_filename)));
        THROW(RaC.Export(0, PPGetFilePathS(PPPATH_OUT, PPFILNAM_EGAIS_REFA_TXT, dump_filename)));
	}
    CATCHZOK
	return ok;
}
//
//
//
EgaisMarkAutoSelector::_MarkEntry::_MarkEntry() : Rest(0.0)
{
}

int EgaisMarkAutoSelector::_MarkEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v12.2.11
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Mark, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Rest, rBuf));
	CATCHZOK
	return ok;
}

EgaisMarkAutoSelector::_TerminalEntry::_TerminalEntry() : GoodsID(0), LotID(0), LotDate(ZERODATE), Qtty(0.0)
{
}

EgaisMarkAutoSelector::_TerminalEntry::_TerminalEntry(const _TerminalEntry & rS)
{
	Copy(rS);
}

EgaisMarkAutoSelector::_TerminalEntry & FASTCALL EgaisMarkAutoSelector::_TerminalEntry::operator = (const _TerminalEntry & rS)
{
	Copy(rS);
	return *this;
}
		
bool FASTCALL EgaisMarkAutoSelector::_TerminalEntry::Copy(const EgaisMarkAutoSelector::_TerminalEntry & rS)
{
	GoodsID = rS.GoodsID;
	LotID = rS.LotID;
	LotDate = rS.LotDate;
	Qtty = rS.Qtty;
	TSCollection_Copy(ML, rS.ML);
	return true;
}

uint EgaisMarkAutoSelector::_TerminalEntry::GetTerminalEntryCount() const
{
	uint   result = 0;
	for(uint i = 0; i < ML.getCount(); i++) {
		const _MarkEntry * p_entry = ML.at(i);
		if(p_entry)
			result++;
	}
	return result;
}

int EgaisMarkAutoSelector::_TerminalEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v12.2.11
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, GoodsID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, LotID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, LotDate, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Qtty, rBuf));
	THROW(TSCollection_Serialize(ML, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

EgaisMarkAutoSelector::Entry::Entry() : GsID(0)
{
}
		
EgaisMarkAutoSelector::Entry::Entry(const Entry & rS)
{
	Copy(rS);
}

EgaisMarkAutoSelector::Entry & FASTCALL EgaisMarkAutoSelector::Entry::operator = (const Entry & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL EgaisMarkAutoSelector::Entry::Copy(const Entry & rS)
{
	GsID = rS.GsID;
	TSCollection_Copy(Te, rS.Te);
	return true;
}

int EgaisMarkAutoSelector::Entry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v12.2.11
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, GsID, rBuf));
	THROW(TSCollection_Serialize(Te, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

uint EgaisMarkAutoSelector::Entry::GetTerminalEntryCount() const
{
	uint   result = 0;
	for(uint i = 0; i < Te.getCount(); i++) {
		const _TerminalEntry * p_entry = Te.at(i);
		if(p_entry)
			result += p_entry->GetTerminalEntryCount();
	}
	return result;
}

const EgaisMarkAutoSelector::_TerminalEntry * EgaisMarkAutoSelector::Entry::SelectMark(uint * pMarkEntryIdx) const
{
	const EgaisMarkAutoSelector::_TerminalEntry * p_result = 0;
	for(uint i = 0; !p_result && i < Te.getCount(); i++) {
		const _TerminalEntry * p_te = Te.at(i);
		if(p_te) {
			for(uint j = 0; !p_result && j < p_te->ML.getCount(); j++) {
				const _MarkEntry * p_me = p_te->ML.at(j);
				if(p_me && p_me->Rest >= p_te->Qtty) {
					ASSIGN_PTR(pMarkEntryIdx, j);
					p_result = p_te;
				}
			}
		}
	}
	return p_result;
}

EgaisMarkAutoSelector::DocItem::DocItem() : ItemId(0), GoodsID(0), Qtty(0.0), VolumeQtty(0.0)
{
}

EgaisMarkAutoSelector::DocItem::DocItem(long itemId, PPID goodsID, double qtty) : ItemId(itemId), GoodsID(goodsID), Qtty(qtty), VolumeQtty(0.0)
{
}

EgaisMarkAutoSelector::DocItem::DocItem(const DocItem & rS)
{
	Copy(rS);
}

EgaisMarkAutoSelector::DocItem & FASTCALL EgaisMarkAutoSelector::DocItem::operator = (const DocItem & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL EgaisMarkAutoSelector::DocItem::Copy(const DocItem & rS)
{
	ItemId = rS.ItemId;
	GoodsID = rS.GoodsID;
	Qtty = rS.Qtty;
	VolumeQtty = rS.VolumeQtty;
	TSCollection_Copy(*this, rS);
	return true;
}

uint EgaisMarkAutoSelector::DocItem::GetTerminalEntryCount() const
{
	uint   result = 0;
	for(uint i = 0; i < getCount(); i++) {
		const Entry * p_entry = at(i);
		if(p_entry)
			result += p_entry->GetTerminalEntryCount();
	}
	return result;
}

int EgaisMarkAutoSelector::DocItem::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v12.2.11
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, ItemId, rBuf));
	THROW_SL(pSCtx->Serialize(dir, GoodsID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Qtty, rBuf));
	THROW_SL(pSCtx->Serialize(dir, VolumeQtty, rBuf));
	THROW(TSCollection_Serialize(*this, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

EgaisMarkAutoSelector::ResultBlock::ResultBlock() : RetCode(-1)
{
}
		
EgaisMarkAutoSelector::ResultBlock::ResultBlock(const ResultBlock & rS)
{
	Copy(rS);
}
		
EgaisMarkAutoSelector::ResultBlock & FASTCALL EgaisMarkAutoSelector::ResultBlock::operator = (const ResultBlock & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL EgaisMarkAutoSelector::ResultBlock::Copy(const ResultBlock & rS)
{
	RetCode = rS.RetCode;
	TSCollection_Copy(*this, rS);
	return true;
}

uint EgaisMarkAutoSelector::ResultBlock::GetTerminalEntryCount() const
{
	uint result = 0;
	for(uint i = 0; i < getCount(); i++) {
		const DocItem * p_item = at(i);
		if(p_item)
			result += p_item->GetTerminalEntryCount();	
	}
	return result;
}

int EgaisMarkAutoSelector::ResultBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v12.2.11
{
	// PPConst::Signature_EgaisMarkAutoSelector_ResultBlock
	int    ok = 1;
	uint32 signature = 0;
	if(dir > 0) {
		signature = PPConst::Signature_EgaisMarkAutoSelector_ResultBlock;
		pSCtx->Serialize(dir, signature, rBuf);
	}
	else if(dir < 0) {
		pSCtx->Serialize(dir, signature, rBuf);
		THROW_PP_S(signature == PPConst::Signature_EgaisMarkAutoSelector_ResultBlock, PPERR_INVSIGNONOBJSRLZRD, "EgaisMarkAutoSelector::ResultBlock");
	}
	THROW(pSCtx->Serialize(dir, RetCode, rBuf));
	THROW(TSCollection_Serialize(*this, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

EgaisMarkAutoSelector::EgaisMarkAutoSelector(/*PPEgaisProcessor * pEgPrc*/) : /*P_EgPrc(pEgPrc),*/IsInitialized(false)
{
}

EgaisMarkAutoSelector::~EgaisMarkAutoSelector()
{
}

int EgaisMarkAutoSelector::Helper_ProcessLot(PPID goodsStrucID, const ReceiptTbl::Rec & rLotRec, double qtty, Entry ** ppEntry, DocItem & rResult)
{
	assert(ppEntry);
	int    ok = -1;
	SString temp_buf;
	ObjTagCore & r_tagc = PPRef->Ot;
	if(r_tagc.GetTagStr(PPOBJ_LOT, rLotRec.ID, PPTAG_LOT_FSRARINFB, temp_buf) > 0) {
		uint  refb_idx = 0;
		if(SearchRefB(RecentEgaisStock, temp_buf, &refb_idx)) {
			PPObjBill * p_bobj = BillObj;
			_TerminalEntry * p_te = 0;
			SString mark_buf;
			StringSet ss_ext_codes;
			if(p_bobj->GetMarkListByLot(rLotRec.ID, ss_ext_codes) > 0) {
				for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);) {
					const bool iemr = PrcssrAlcReport::IsEgaisMark(temp_buf, &mark_buf);
					if(iemr) {
						assert(mark_buf.NotEmpty());
						if(!(*ppEntry)) {
							(*ppEntry) = rResult.CreateNewItem();
							THROW_SL(*ppEntry);
							(*ppEntry)->GsID = goodsStrucID;
						}
						if(!p_te) {
							p_te = (*ppEntry)->Te.CreateNewItem();
							THROW_SL(p_te);
							p_te->GoodsID = rLotRec.GoodsID;
							p_te->LotID = rLotRec.ID;
							p_te->LotDate = rLotRec.Dt;
							p_te->Qtty = qtty;
						}
						_MarkEntry * p_me = p_te->ML.CreateNewItem();
						THROW_SL(p_me);
						p_me->Mark = mark_buf;
						p_me->Rest = 1.0; // @?
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int EgaisMarkAutoSelector::Helper(PPID goodsID, double qtty, DocItem & rResult)
{
	int    ok = -1;
	if(qtty != 0.0) {
		PPEgaisProcessor * p_egprc = DS.GetTLA().GetEgaisProcessor();
		if(p_egprc) {
			PPObjBill * p_bobj = BillObj;
			Transfer * p_trfr = p_bobj->trfr;
			SString temp_buf;
			SString mark_buf;
			StringSet ss_ext_codes;
			Goods2Tbl::Rec goods_rec; // запись основного товара (который непосредственно продается)
			Goods2Tbl::Rec org_goods_rec; // запись оригинального товара (из которого был произведен основной товар)
			PPGoodsType gt_rec;
			PPObjUnit unit_obj;
			double main_goods_liter_ratio = 0.0;
			double inner_goods_liter_ratio = 0.0;
			if(GObj.Fetch(goodsID, &goods_rec) > 0) {
				if(goods_rec.PhUnitID && goods_rec.PhUPerU > 0.0) {
					unit_obj.TranslateToBase(goods_rec.PhUnitID, SUOM_LITER, &main_goods_liter_ratio);
					main_goods_liter_ratio *= goods_rec.PhUPerU;
				}
				else {
					unit_obj.TranslateToBase(goods_rec.UnitID, SUOM_LITER, &main_goods_liter_ratio);
				}
				if(main_goods_liter_ratio > 0.0) {
					ObjTagCore & r_tagc = PPRef->Ot;
					PPGoodsStruc::Ident gsi(goodsID, GSF_COMPL, GSF_PARTITIAL);
					TSCollection <PPGoodsStruc> gs_list;
					LotArray lot_list;
					GObj.LoadGoodsStruc(gsi, gs_list);
					for(uint gsidx = 0; gsidx < gs_list.getCount(); gsidx++) {
						const PPGoodsStruc * p_gs = gs_list.at(gsidx);
						if(p_gs) {
							PPGoodsStrucItem gs_item;
							double item_qtty = 0.0;
							for(uint gs_pos = 0; p_gs->EnumItemsExt(&gs_pos, &gs_item, 0, qtty, &item_qtty) > 0;) {
								Goods2Tbl::Rec inner_goods_rec;
								if(GObj.Fetch(gs_item.GoodsID, &inner_goods_rec) > 0) {
									Entry * p_entry_component = 0;
									if(p_egprc->IsAlcGoods(gs_item.GoodsID)) {
										if(inner_goods_rec.PhUnitID && inner_goods_rec.PhUPerU > 0.0) {
											unit_obj.TranslateToBase(inner_goods_rec.PhUnitID, SUOM_LITER, &inner_goods_liter_ratio);
											inner_goods_liter_ratio *= inner_goods_rec.PhUPerU;
										}
										else
											unit_obj.TranslateToBase(inner_goods_rec.UnitID, SUOM_LITER, &inner_goods_liter_ratio);
										if(inner_goods_liter_ratio > 0.0) {
											lot_list.clear();
											p_trfr->Rcpt.GetList(gs_item.GoodsID, 0, 0, ZERODATE, ReceiptCore::glfWithExtCodeOnly, &lot_list);
											if(lot_list.getCount()) {
												for(uint lotidx = 0; lotidx < lot_list.getCount(); lotidx++) {
													const ReceiptTbl::Rec & r_lot_rec = lot_list.at(lotidx);
													const int plr = Helper_ProcessLot(p_gs->Rec.ID, r_lot_rec, item_qtty, &p_entry_component, rResult);
													THROW(plr);
													if(plr > 0)
														ok = 1;
												}
											}
										}
									}
									else {
										PPGoodsStruc::Ident gsi_inner(gs_item.GoodsID, GSF_SUBST, 0);
										TSCollection <PPGoodsStruc> gs_list_inner;
										GObj.LoadGoodsStruc(gsi_inner, gs_list_inner);
										for(uint gsinneridx = 0; gsinneridx < gs_list_inner.getCount(); gsinneridx++) {
											const PPGoodsStruc * p_gs_inner = gs_list_inner.at(gsinneridx);
											if(p_gs_inner) {
												PPGoodsStrucItem gs_inner_item;
												double inner_item_qtty = 0.0;
												for(uint gs_inner_pos = 0; p_gs_inner->EnumItemsExt(&gs_inner_pos, &gs_inner_item, 0, item_qtty, &inner_item_qtty) > 0;) {
													Goods2Tbl::Rec subst_goods_rec;
													if(GObj.Fetch(gs_inner_item.GoodsID, &subst_goods_rec) > 0 && p_egprc->IsAlcGoods(gs_inner_item.GoodsID)) {
														if(subst_goods_rec.PhUnitID && subst_goods_rec.PhUPerU > 0.0) {
															unit_obj.TranslateToBase(subst_goods_rec.PhUnitID, SUOM_LITER, &inner_goods_liter_ratio);
															inner_goods_liter_ratio *= subst_goods_rec.PhUPerU;
														}
														else
															unit_obj.TranslateToBase(subst_goods_rec.UnitID, SUOM_LITER, &inner_goods_liter_ratio);
														if(inner_goods_liter_ratio > 0.0) {
															lot_list.clear();
															p_trfr->Rcpt.GetList(gs_inner_item.GoodsID, 0, 0, ZERODATE, ReceiptCore::glfWithExtCodeOnly, &lot_list);
															if(lot_list.getCount()) {
																for(uint lotidx = 0; lotidx < lot_list.getCount(); lotidx++) {
																	const ReceiptTbl::Rec & r_lot_rec = lot_list.at(lotidx);
																	const int plr = Helper_ProcessLot(p_gs_inner->Rec.ID, r_lot_rec, inner_item_qtty, &p_entry_component, rResult);
																	THROW(plr);
																	if(plr > 0)
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
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/bool EgaisMarkAutoSelector::SearchRefBEntry(const TSVector <RefBEntry> & rList, const RefBEntry & rKey, uint * pPos)
{
	bool result = false;
	uint idx = 0;
	if(rList.getCount() && !isempty(rKey.RefB)) {
		for(uint i = 0; !result && i < rList.getCount(); i++) {
			const RefBEntry & r_item = rList.at(i);
			if(sstreqi_ascii(r_item.RefB, rKey.RefB)) {
				idx = i;
				result = true;
			}
		}
	}
	ASSIGN_PTR(pPos, idx);
	return result;
}

/*static*/bool EgaisMarkAutoSelector::SearchRefB(const TSVector <RefBEntry> & rList, const char * pRefB, uint * pPos)
{
	bool result = false;
	uint idx = 0;
	if(rList.getCount() && !isempty(pRefB)) {
		for(uint i = 0; !result && i < rList.getCount(); i++) {
			const RefBEntry & r_item = rList.at(i);
			if(sstreqi_ascii(r_item.RefB, pRefB)) {
				idx = i;
				result = true;
			}
		}
	}
	ASSIGN_PTR(pPos, idx);
	return result;
}

int EgaisMarkAutoSelector::GetRecentEgaisStock(TSVector <RefBEntry> & rResultList)
{
	int    ok = -1;
	SString temp_buf;
	PPID   stock_op_id = 0; // Вид операции фиксации остатков егаис (драфт-документ, чаще всего это - PPOPK_EDI_STOCK)
	PPObjOprKind op_obj;
	{
		PPID   op_id = 0;
		if(op_obj.GetEdiStockOp(&op_id, 1)) {
			stock_op_id = op_id;
		}
	}
	if(stock_op_id) {
		PPObjBill * p_bobj = BillObj;
		BillCore * p_billc = p_bobj->P_Tbl;
		ObjTagCore & r_tagc = PPRef->Ot;
		const long back_days = SlDebugMode::CT() ? 60 : 1;
		const char * p_code_suffix = "-R1";
		PPIDArray loc_list;
		LAssocArray loc_to_date_list;
		LDATE recent_date = ZERODATE;
		BillTbl::Rec bill_rec;
		{
			BillTbl::Key2 k2;
			k2.OpID = stock_op_id;
			k2.Dt   = MAXDATE;
			k2.BillNo = MAXLONG;
			const long max_days_for_survey = 30;
			if(p_billc->search(2, &k2, spLe) && k2.OpID == stock_op_id) do {
				p_billc->copyBufTo(&bill_rec);
				if(!p_code_suffix || (temp_buf = bill_rec.Code).CmpSuffix(p_code_suffix, 0) == 0) {
					if(!recent_date)
						recent_date = bill_rec.Dt;
					else if(diffdate(recent_date, bill_rec.Dt) > max_days_for_survey) {
						break;
					}
					{
						uint _p = 0;
						if(loc_to_date_list.Search(bill_rec.LocID, &_p)) {
							;
						}
						else {
							loc_to_date_list.Add(bill_rec.LocID, static_cast<long>(static_cast<ulong>(bill_rec.Dt)));
						}
					}
				}
			} while(p_billc->search(2, &k2, spPrev) && k2.OpID == stock_op_id); // @v12.1.2 @fix spNext-->spPrev
		}
		for(uint i = 0; i < loc_to_date_list.getCount(); i++) {
			const PPID loc_id = loc_to_date_list.at(i).Key;
			LDATE dt;
			dt.v = static_cast<ulong>(loc_to_date_list.at(i).Val);
			DateRange period;
			period.Set(dt, dt);
			for(DateIter di(&period); p_billc->EnumByOpr(stock_op_id, &di, &bill_rec) > 0;) {
				if(bill_rec.LocID == loc_id && (!p_code_suffix || (temp_buf = bill_rec.Code).CmpSuffix(p_code_suffix, 0) == 0)) {
					PPBillPacket bpack;
					if(p_bobj->ExtractPacket(bill_rec.ID, &bpack) > 0) {
						for(uint tidx = 0; tidx < bpack.GetTCount(); tidx++) {
							const PPTransferItem & r_ti = bpack.ConstTI(tidx);
							bpack.LTagL.GetString(PPTAG_LOT_FSRARINFB, tidx, temp_buf);
							if(temp_buf.NotEmptyS()) {
								RefBEntry rbentry;
								STRNSCPY(rbentry.RefB, temp_buf);
								rbentry.GoodsID = r_ti.GoodsID;
								rbentry.LocID = bpack.Rec.LocID;
								rbentry.StockBillID = bpack.Rec.ID;
								rbentry.StockBillDt = bpack.Rec.Dt;
								rbentry.StockBillRbb = r_ti.RByBill;
								rbentry.Rest = fabs(r_ti.Quantity_);
								uint found_idx = 0;
								if(SearchRefBEntry(rResultList, rbentry, &found_idx)) {
									rResultList.at(found_idx).Rest = rbentry.Rest;
								}
								else {
									rResultList.insert(&rbentry);
								}
								ok = 1;
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int EgaisMarkAutoSelector::Run(ResultBlock & rResult)
{
	int    ok = -1;
	if(/*P_EgPrc*/rResult.getCount()) {
		PPEgaisProcessor * p_egprc = DS.GetTLA().GetEgaisProcessor();
		if(p_egprc) {
			Goods2Tbl::Rec goods_rec; // запись основного товара (который непосредственно продается)
			for(uint itemidx = 0; itemidx < rResult.getCount(); itemidx++) {
				DocItem * p_item = rResult.at(itemidx);
				if(p_item && p_item->Qtty != 0.0 && GObj.Fetch(p_item->GoodsID, &goods_rec) > 0 && goods_rec.GoodsTypeID) {
					PPGoodsType gt_rec;
					if(GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.Flags & GTF_EGAISAUTOWO) {
						if(!IsInitialized) {
							GetRecentEgaisStock(RecentEgaisStock);
						}
						const int r = Helper(p_item->GoodsID, p_item->Qtty, *p_item);
						if(r > 0) {
							assert(rResult.getCount());
							LotExtCodeCore * p_lotxct = p_egprc->GetLecT();
							if(p_lotxct) {
								TSCollection <CCheckCore::ListByMarkEntry> lbm;
								for(uint i = 0; i < p_item->getCount(); i++) {
									const EgaisMarkAutoSelector::Entry * p_entry = p_item->at(i);
									if(p_entry && p_entry->Te.getCount()) {
										for(uint eidx = 0; eidx < p_entry->Te.getCount(); eidx++) {
											const EgaisMarkAutoSelector::_TerminalEntry * p_te = p_entry->Te.at(eidx);
											if(p_te && p_te->ML.getCount()) {
												for(uint mlidx = 0; mlidx < p_te->ML.getCount(); mlidx++) {
													EgaisMarkAutoSelector::_MarkEntry * p_me = p_te->ML.at(mlidx);
													if(p_me) {
														CCheckCore::ListByMarkEntry * p_cclbm_entry = lbm.CreateNewItem();
														THROW_SL(p_cclbm_entry);
														p_cclbm_entry->OrgLotID = p_te->LotID;
														p_cclbm_entry->OrgLotDate = p_te->LotDate;
														p_cclbm_entry->OrgLotQtty = p_te->Qtty;
														p_cclbm_entry->P_Extra = p_me;
														STRNSCPY(p_cclbm_entry->Mark, p_me->Mark);
													}
												}
											}
										}
									}
								}
								if(lbm.getCount()) {
									PPObjCSession csobj;
									const uint back_days = PPObjCSession::GetCcListByMarkBackDays(lbm);
									LAssocArray index;
									LAssocArray * p_index = csobj.FetchCcDate2MaxIdIndex(index) ? &index : 0;
									csobj.P_Cc->Helper_GetListByMark2(lbm, CCheckPacket::lnextEgaisMark, p_index, back_days, 0);
									for(uint lbmidx = 0; lbmidx < lbm.getCount(); lbmidx++) {
										const CCheckCore::ListByMarkEntry * p_cclbm_entry = lbm.at(lbmidx);
										if(p_cclbm_entry && p_cclbm_entry->P_Extra) {
											EgaisMarkAutoSelector::_MarkEntry * p_me = static_cast<EgaisMarkAutoSelector::_MarkEntry *>(p_cclbm_entry->P_Extra);
											p_me->Rest -= p_cclbm_entry->TotalOpQtty;
										}
									}
								}
							}
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	rResult.RetCode = ok;
	return ok;
}
