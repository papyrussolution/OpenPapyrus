// CHZN.CPP
// Copyright (c) A.Sobolev 2019, 2020
// @codepage UTF-8
// Реализация интерфейса к сервисам честный знак
//
#include <pp.h>
#pragma hdrstop
//#include <wininet.h>

/*
С блока сигарет считали марку 0104600266011725212095134931209513424010067290
Разбираем ее по правилам https://www.garant.ru/products/ipo/prime/doc/72089916/:
01 ИД GTIN, далее 14 знаков GTIN: 04600266011725.
21 ИД Serial, далее 7 знаков serial: 2095134.

Data Matrix для табачной продукции и фармацевтики и обуви состоит из 4-х частей.
Для табака:	01 - GTIN: код товара; 21 - индивидуальный серийный номер единицы товара; 8005 – МРЦ; 93 - код проверки.
Для обуви: 01 - GTIN: код товара; 21 - индивидуальный серийный номер единицы товара; 91 - ключ проверки; 92 - код проверки.
Для лекарств: 21 - серийный номер; 01 - GTIN; 91 - ключ проверки; 92 - код проверки. В код также могут быть включены (необязательно) 
  следующие поля: дата истечения срока годности, номер серии, в установленных законодательством форматах 
*/
//
// 46 bytes
//
#if 0 // @v10.6.9 {
SLAPI ChZnCodeStruc::ChZnCodeStruc() : SStrGroup(), GtinPrefixP(0), GtinP(0), SerialPrefixP(0), SerialP(0), SkuP(0), TailP(0)
{
}
	
ChZnCodeStruc & SLAPI ChZnCodeStruc::Z()
{
	GtinPrefixP = 0;
	GtinP = 0;
	SerialPrefixP = 0;
	SerialP = 0;
	SkuP = 0;
	TailP = 0;
	ClearS();
	return *this;
}

int SLAPI ChZnCodeStruc::Parse(const char * pRawCode)
{
	int    ok = 0;
	Z();
	const size_t raw_len = sstrlen(pRawCode);
	if(raw_len >= 25) {
		SString temp_buf;
		SString raw_buf;
		uint   forward_dig_count = 0;
		{
			int   non_dec = 0;
			for(size_t i = 0; i < raw_len; i++) {
				const char c = pRawCode[i];
				if(isdec(c)) {
					if(!non_dec)
						forward_dig_count++;
				}
				else
					non_dec = 1;
				if(!isdec(c) && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z') && !oneof4(c, '=', '/', '+', '-')) {
					temp_buf.Z().CatChar(c).Transf(CTRANSF_INNER_TO_OUTER);
					KeyDownCommand kd;
					uint   tc = kd.SetChar((uchar)temp_buf.C(0)) ? kd.GetChar() : 0; // Попытка транслировать латинский символ из локальной раскладки клавиатуры
					if((tc >= 'A' && tc <= 'Z') || (tc >= 'a' && tc <= 'z'))
						raw_buf.CatChar((char)tc);
					else
						raw_buf.CatChar(c);
				}
				else {
					raw_buf.CatChar(c);
				}
			}
		}
		pRawCode = raw_buf.cptr();
		size_t p = 0;
		temp_buf.Z().CatN(pRawCode+p, 2);
		p += 2;
		AddS(temp_buf, &GtinPrefixP);
		if(temp_buf == "01") {
			temp_buf.Z().CatN(pRawCode+p, 14);
			p += 14;
			AddS(temp_buf, &GtinP);
			while(temp_buf.C(0) == '0')
				temp_buf.ShiftLeft();
			//
			temp_buf.Z().CatN(pRawCode+p, 2);
			p += 2;
			AddS(temp_buf, &SerialPrefixP);
			if(temp_buf == "21") {
				temp_buf.Z();
				while(pRawCode[p] && strncmp(pRawCode+p, "240", 3) != 0) {
					temp_buf.CatChar(pRawCode[p++]);
				}
				AddS(temp_buf, &SerialP);
				/*if(strncmp(pRawCode, "240", 3) == 0) {

				}*/
				temp_buf.Z().Cat(pRawCode+p);
				AddS(temp_buf, &TailP);
				ok = 1;
			}
		}
	}
	return ok;
}
#endif // } 0 @v10.6.9

/*static*/int SLAPI PPChZnPrcssr::Encode1162(int productType, const char * pGTIN, const char * pSerial, void * pResultBuf, size_t resultBufSize)
{
	uint16 product_type_bytes = 0;
	switch(productType) {
		case ptFur: product_type_bytes = 0x0002; break;
		case ptTobacco: product_type_bytes = 0x0005; break;
		case ptShoe: product_type_bytes = 0x1520; break;
		case ptMedicine: product_type_bytes = 0x0003; break;
	}
	return product_type_bytes ? STokenRecognizer::EncodeChZn1162(product_type_bytes, pGTIN, pSerial, pResultBuf, resultBufSize) : 0;
}

/*static*/int SLAPI PPChZnPrcssr::ParseChZnCode(const char * pCode, GtinStruc & rS, long flags)
{
	int    ok = 0;
	rS.Z();
	rS.AddOnlyToken(GtinStruc::fldGTIN14);
	rS.AddOnlyToken(GtinStruc::fldSerial);
	rS.SetSpecialFixedToken(GtinStruc::fldSerial, 13);
	rS.AddOnlyToken(GtinStruc::fldPart);
	rS.AddOnlyToken(GtinStruc::fldAddendumId);
	rS.AddOnlyToken(GtinStruc::fldUSPS);
	rS.AddOnlyToken(GtinStruc::fldInner1);
	rS.AddOnlyToken(GtinStruc::fldInner2);
	rS.AddOnlyToken(GtinStruc::fldSscc18);
	rS.AddOnlyToken(GtinStruc::fldExpiryDate);
	rS.AddOnlyToken(GtinStruc::fldManufDate); // @v10.8.2
	rS.AddOnlyToken(GtinStruc::fldVariant);
	//rS.AddOnlyToken(GtinStruc::fldPriceRuTobacco);
	//rS.AddOnlyToken(GtinStruc::fldPrice);
	int   pr = 0;
	{
		SString raw_buf;
		SString temp_buf;
		{
			temp_buf = pCode;
			if(!temp_buf.IsAscii()) {
				// Попытка транслировать латинский символ из локальной раскладки клавиатуры
				SStringU temp_buf_u;
				temp_buf_u.CopyFromMb_INNER(temp_buf, temp_buf.Len());
				for(size_t i = 0; i < temp_buf_u.Len(); i++) {
					const wchar_t c = temp_buf_u.C(i);
					KeyDownCommand kd;
					uint   tc = kd.SetCharU(c) ? kd.GetChar() : 0; 
					raw_buf.CatChar(static_cast<char>(tc));
				}
				pCode = raw_buf.cptr();
			}
		}
		pr = rS.Parse(pCode);
		if(pr != 1 && rS.GetToken(GtinStruc::fldGTIN14, 0)) {
			rS.SetSpecialFixedToken(GtinStruc::fldSerial, 12);
			pr = rS.Parse(pCode);
			if(pr != 1 && rS.GetToken(GtinStruc::fldGTIN14, 0)) {
				rS.SetSpecialFixedToken(GtinStruc::fldSerial, 11);
				pr = rS.Parse(pCode);
				// @v10.8.2 {
				/*if(pr != 1 && rS.GetToken(GtinStruc::fldGTIN14, 0)) {
					rS.SetSpecialFixedToken(GtinStruc::fldSerial, 8);
					pr = rS.Parse(temp_buf);
				}*/
				// } @v10.8.2 
			}
		}
		if(pr == 1) {
			if(rS.GetToken(GtinStruc::fldGTIN14, 0) && rS.GetToken(GtinStruc::fldSerial, &temp_buf)) {
				if(rS.GetSpecialNaturalToken() == SNTOK_CHZN_CIGITEM)
					ok = SNTOK_CHZN_CIGITEM;
				else if(rS.GetSpecialNaturalToken() == SNTOK_CHZN_CIGBLOCK)
					ok = SNTOK_CHZN_CIGBLOCK;
				else if(temp_buf.Len() == 13)
					ok = SNTOK_CHZN_SIGN_SGTIN;
				else
					ok = SNTOK_CHZN_GS1_GTIN;
			}
		}
	}
	// @v10.8.2 {
	if(!ok && flags & pchzncfPretendEverythingIsOk) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		SNaturalTokenStat nts;
		uint tokn = 0;
		tr.Run(reinterpret_cast<const uchar *>(pCode), sstrlen(pCode), nta, &nts);
		if(nts.Seq & SNTOKSEQ_ASCII && nts.Len >= 25)
			ok = 100000;
	}
	// } @v10.8.2
	return ok;
}

//static 
int FASTCALL PPChZnPrcssr::IsChZnCode(const char * pCode)
{
	//#define SNTOK_CHZN_GS1_GTIN    28 // честный знак Идентификационный номер GS1 для идентификации товаров regexp: "[0-9]{14}"
	//#define SNTOK_CHZN_SIGN_SGTIN  29 // честный знак Индивидуальный серийный номер вторичной упаковки regexp: "[0-9]{14}[&#x21;-&#x22;&#x25;-&#x2F;&#x30;-&#x39;&#x41;-&#x5A;&#x5F;&#x61;-&#x7A;]{13}"
	//#define SNTOK_CHZN_SSCC        30 // честный знак Индивидуальный серийный номер третичной/транспортной упаковки regexp: "[0-9]{18}"
	int    result = 0;
	const size_t len = sstrlen(pCode);
	if(oneof3(len, 14, 27, 18)) {
		/*
			gs1_gtin_type: Идентификационный номер GS1 для идентификации товаров (допускаются только цифры общей длины 14 символов)
				length value: 14
				pattern value: "[0-9]{14}"

			sign_sgtin_type: Индивидуальный серийный номер вторичной упаковки
				length value: 27
				pattern value: "[0-9]{14}[&#x21;-&#x22;&#x25;-&#x2F;&#x30;-&#x39;&#x41;-&#x5A;&#x5F;&#x61;-&#x7A;]{13}"

			sscc_type: Индивидуальный серийный номер третичной/транспортной упаковки
				pattern value: "[0-9]{18}"
		*/
		if(oneof2(len, 14, 18)) {
			if(len == 14)
				result = SNTOK_CHZN_GS1_GTIN;
			else // len == 18
				result = SNTOK_CHZN_SSCC;
			for(const char * p = pCode; result && *p; p++) {
				if(!isdec(*p))
					result = 0;
			}
		}
		else if(len == 27) {
			result = SNTOK_CHZN_SIGN_SGTIN;
			size_t n = 0;
			const char * p = pCode;
			for(; result && n < 14 && *p; p++, n++) {
				if(!isdec(*p))
					result = 0;
			}
			if(result) {
				for(; result && n < 27 && *p; p++, n++) {
					const char c = *p;
					if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || oneof14(c, '!', '\"', '%', '&', '\'', '(', ')', '*', '+', '-', ',', '.', '/', '_') || isdec(c)))
						result = 0;
				}
			}
		}
	}
	return result;
}

//static 
int SLAPI PPChZnPrcssr::InputMark(SString & rMark)
{
	class ChZnMarkDialog : public TDialog {
	public:
		ChZnMarkDialog() : TDialog(DLG_CHZNMARK)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			/* @v10.8.0
			if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_CHZNMARK_INPUT)) {
				static int __lock = 0;
				if(!__lock) {
					__lock = 1;
					int   is_auto_input = 0;
					getCtrlString(CTL_CHZNMARK_INPUT, CodeBuf.Z());
					TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_CHZNMARK_INPUT));
					if(p_il) {
						TInputLine::Statistics stat;
						p_il->GetStatistics(&stat);
						if(stat.Flags & stat.fSerialized && !(stat.Flags & stat.fPaste) && stat.SymbCount && stat.IntervalMean <= 100.0) // @v10.7.2 (<=5.0)-->(<=50.0) // @v10.7.12 (<=50.0)-->(<=100.0)
							is_auto_input = 1;
					}
					if(!is_auto_input) {
						SString msg_buf;
						SString temp_buf;
						GtinStruc gts;
						const int pczcr = PPChZnPrcssr::ParseChZnCode(CodeBuf, gts);
						if(pczcr) {
							if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf))
								msg_buf.Space().CatEq("GTIN14", temp_buf);
							if(gts.GetToken(GtinStruc::fldSerial, &temp_buf))
								msg_buf.Space().CatEq("SER", temp_buf);
							if(gts.GetToken(GtinStruc::fldPart, &temp_buf))
								msg_buf.Space().CatEq("PART", temp_buf);
							if(gts.GetToken(GtinStruc::fldSscc18, &temp_buf))
								msg_buf.Space().CatEq("SSCC18", temp_buf);
							if(gts.GetToken(GtinStruc::fldPriceRuTobacco, &temp_buf))
								msg_buf.Space().CatEq("PRICE", temp_buf);
							//PPLoadTextS(PPTXT_CHZNMARKVALID, msg_buf).CR().Cat(CodeBuf);
							// @v10.7.1 {
							if(gts.GetToken(GtinStruc::fldOriginalText, &temp_buf)) {
								CodeBuf = temp_buf;
								setCtrlString(CTL_CHZNMARK_INPUT, temp_buf);
							}
							// } @v10.7.1 
						}
						else
							PPLoadError(PPERR_TEXTISNTCHZNMARK, msg_buf, CodeBuf);
						setStaticText(CTL_CHZNMARK_INFO, msg_buf);
					}
					__lock = 0;
				}
			}*/
		}
		SString CodeBuf;
	};

	rMark.Z();

    int    ok = -1;
	SString temp_buf;
    PrcssrAlcReport::EgaisMarkBlock mb;
    ChZnMarkDialog * dlg = new ChZnMarkDialog();
    THROW(CheckDialogPtr(&dlg));
	/*if(pAgi) {
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
	}*/
    while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_CHZNMARK_INPUT, temp_buf);
		GtinStruc gts;
		const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
		if(pczcr) {
			gts.GetToken(GtinStruc::fldOriginalText, &rMark);
			ok = 1;
		}
		else {
			PPSetError(PPERR_TEXTISNTCHZNMARK, temp_buf);
			PPErrorByDialog(dlg, CTL_CHZNMARK_INPUT);
			TInputLine * p_il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_CHZNMARK_INPUT));
			CALLPTRMEMB(p_il, selectAll(1));
			rMark.Z();
		}
    }
    CATCHZOKPPERR
    delete dlg;
    return ok;
}

static const SIntToSymbTabEntry CzDocType_SymbTab[] = {
	{ 200, "result" },
	{ 210, "query_kiz_info" },
	{ 211, "kiz_info" },
	{ 416, "receive_order" },
	{ 431, "move_place" }, // @v10.8.7 doctypMovePlace
	{ 602, "receive_order_notification" },
	{ 701, "accept" },
};

class ChZnInterface {
public:
	enum {
		qAuth = 1,
		qToken,
		qDocOutcome,
		qCurrentUserInfo,
		qGetDocList,
		qGetIncomingDocList,
		qGetOutcomingDocList,
		qGetDoc,
		qDocumentSend,
		qGetTicket
	};
	SLAPI  ChZnInterface()
	{
	}
	SLAPI ~ChZnInterface()
	{
	}
	enum {
		urloptHttps   = 0x0001,
		urloptSandBox = 0x0002
	};

	struct InitBlock {
		InitBlock() : GuaID(0), ProtocolVer(0), ProtocolId(protidUnkn)
		{
		}
		enum {
			protidUnkn      = 0,
			protidMdlp      = 1, // ИС Маркировка. МДЛП. Протокол обмена интерфейсного уровня
			protidEdoLtMdlp = 2, // API ЭДО лайт МДЛП
			protidEdoLtInt  = 3, // API ЭДО лайт Интеграционный стенд ГИС МТ
			protidEdoLtElk  = 4, // API ЭДО лайт Продуктивый стенд ГИС МТ
			protidGisMt     = 5, // API ГИС МТ
		};
		PPID   GuaID;
		int    ProtocolVer;
		int    ProtocolId;
		PPGlobalUserAccPacket GuaPack;
		SString CliAccsKey;
		SString CliSecret;
		SString CliIdent;
		SString Cn; // CN субъекта сертификата электронной подписи (PPTAG_GUA_CERTSUBJCN)
		SString CryptoProPath;
		SString EndPoint; // URL для запросов
		//
		SString Token;
	};

	class WinInternetHandleStack : private TSStack <HINTERNET> {
	public:
		WinInternetHandleStack()
		{
		}
		~WinInternetHandleStack()
		{
			Destroy();
		}
		HINTERNET Push(HINTERNET h)
		{
			if(h) {
				push(h);
			}
			return h;
		}
		void Destroy()
		{
			HINTERNET h = 0;
			while(pop(h)) {
				::InternetCloseHandle(h);
			}
		}
		HINTERNET PushConnection(const InetUrl & rUrl, HINTERNET hInetSess)
		{
			SString temp_buf;
			rUrl.GetComponent(InetUrl::cHost, 0, temp_buf);
			int   port = rUrl.GetDefProtocolPort(rUrl.GetProtocol());
			return Push(InternetConnect(hInetSess, SUcSwitch(temp_buf), port, _T("")/*lpszUserName*/, _T("")/*lpszPassword*/, INTERNET_SERVICE_HTTP, 0, 0));
		}
		HINTERNET PushHttpRequestGet(HINTERNET hConn, const InetUrl & rUrl)
		{
			const TCHAR * p_types[] = { _T("text/*"), _T("application/json"), 0 };
			SString qbuf;
			SString temp_buf;
			rUrl.GetComponent(InetUrl::cPath, 0, temp_buf);
			qbuf = temp_buf;
			rUrl.GetComponent(InetUrl::cQuery, 0, temp_buf);
			if(temp_buf.NotEmptyS())
				qbuf.CatChar('?').Cat(temp_buf);
			return Push(HttpOpenRequest(hConn, _T("GET"), SUcSwitch(qbuf), _T("HTTP/1.1"), NULL, p_types, INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_SECURE, 1));
		}
		HINTERNET PushHttpRequestPost(HINTERNET hConn, const InetUrl & rUrl)
		{
			const TCHAR * p_types[] = { _T("text/*"), _T("application/json"), 0 };
			SString qbuf;
			SString temp_buf;
			rUrl.GetComponent(InetUrl::cPath, 0, temp_buf);
			qbuf = temp_buf;
			rUrl.GetComponent(InetUrl::cQuery, 0, temp_buf);
			if(temp_buf.NotEmptyS())
				qbuf.CatChar('?').Cat(temp_buf);
			return Push(HttpOpenRequest(hConn, _T("POST"), SUcSwitch(qbuf), _T("HTTP/1.1"), NULL, p_types, INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_SECURE, 1));
		}
	};
	/*enum {
		codetypeSgtin = 1,
		codetypeSscc  = 2
	};*/

	enum {
		doctypUnkn = 0,
		doctOstDescription,
		doctAggregation,
		doctDisaggregation,
		doctReaggregation,
		doctLpIntroduceGoods,
		doctLkIndiCommissioning,
		doctLpGoodsImport,
		doctCrossborder,
		doctLpIntroduceGoodsCrossborderCSD,
		doctIntroduceOST,
		doctLkContractCommissioning,
		doctLpReturn,
		doctLpShipGoods,
		doctLpShipReceipt,
		doctLpCancelShipment,
		doctLpAcceptGoods,
		doctLkReceipt,
		doctLkRemark,
		doctKmCancellation,
		doctAppliedKmCancellation
	};

	struct Packet;
	//
	// Descr: Статус документа
	//
	enum {
		docstUnkn = 0,
		docstInProgress, // IN_PROGRESS – Проверяется
		docstCheckedOk,  // CHECKED_OK – Обработан
		docstCheckedNoOk, // CHECKED_NOT_OK – Обработан с ошибками
		docstProcessingError, // PROCESSING_ERROR – Техническая ошибка
		docstUndefined, // UNDEFINED – не определен
		docstCancelled, // CANCELLED – Аннулирован. Только для документа 'Отмена отгрузки'
		docstAccepted, // ACCEPTED – Принят. Только для документа 'Отгрузка'
		docstWaitAcceptance, // WAIT_ACCEPTANCE – Ожидание приемку. Только для документа 'Отгрузка'. Устанавливается при успешной обработке документа 'Отгрузка товара'
		docstParticipantRegistration // WAIT_PARTICIPANT_REGISTRATION -Ожидает регистрации участника в ГИС МТ. Только для документа 'Отгрузка'. Устанавливается при успешной обработке документа 'Отгрузка товара' в сторону незарегистрированного участника
	};
	class Document {
	public:
		SLAPI  Document() : Dtm(ZERODATETIME), ReceivedDtm(ZERODATETIME), Type(doctypUnkn), 
			Format(SFileFormat::Unkn), Status(docstUnkn), DownloadStatus(0), Flags(0)
		{
		}
		SLAPI  Document(const Document & rS)
		{
			Copy(rS);
		}
		Document & FASTCALL operator = (const Document & rS)
		{
			return Copy(rS);
		}
		int    SLAPI Parse(const char * pBuffer);
		int    SLAPI GetTransactionPartyCode(PPID psnID, PPID locID, SString & rCode);
		int    SLAPI Make(SXml::WDoc & rX, const ChZnInterface::InitBlock & rIb, const ChZnInterface::Packet * pPack);
		Document & FASTCALL Copy(const Document & rS)
		{
			Uuid = rS.Uuid;
			Dtm = rS.Dtm;
			ReceivedDtm = rS.ReceivedDtm;
			Type = rS.Type;
			Format = rS.Format;
			Status = rS.Status;
			DownloadStatus = rS.DownloadStatus;
			Flags = rS.Flags;
			SenderName = rS.SenderName;
			ReceiverName = rS.ReceiverName;
			Body = rS.Body;
			Content = rS.Content;
			return *this;
		}
		enum {
			fInput = 0x0001 // Входящий документ. Иначе - исходящий
		};
		S_GUID Uuid;
		LDATETIME Dtm;
		LDATETIME ReceivedDtm;
		int  Type;
		int  Format; // SFileFormat::xxx
		int  Status;
		int  DownloadStatus;
		int  Flags;
		SString SenderName;
		SString ReceiverName;
		SString Body;
		SString Content;
	};
	//
	// Для идентификации типов кодов используются константы, определенные в slib.h
	//   SNTOK_CHZN_GS1_GTIN SNTOK_CHZN_SIGN_SGTIN SNTOK_CHZN_SSCC
	//
	enum {
		doctypResult                   = 200,
		doctypQueryKizInfo             = 210,
		doctypKizInfo                  = 211,
		doctypReceiveOrder             = 416,
		doctypMovePlace                = 431, // @v10.8.7 
		doctypReceiveOrderNotification = 602,
		doctypAccept                   = 701,
	};
	struct Packet {
		struct ErrorItem {
			int    Code;
			SString Descr;
			SString Ident;
		};
		struct OperationResult {
			OperationResult() : Op(0), Status(0)
			{
			}
			int    Op; // doctypXXX Ид операции в ответ на которую пришел тикет
			int    Status; // -1 - rejected, 1 - accepted
			SString OpIdent; 
			SString OpComment;
			TSCollection <ErrorItem> Errors;
		};
		struct QueryKizInfo {
			QueryKizInfo() : CodeType(0), Modifier(0), ArID(0)
			{
			}
			int   CodeType; // SNTOK_CHZN_XXX
			int   Modifier; // 0 - none, 1 - down, 2 - up
			PPID  ArID;
			SString SubjectIdent;
			SString Code;
		};
		explicit Packet(int docType) : DocType(docType), Flags(0), P_Data(0)
		{
			switch(DocType) {
				case doctypResult: P_Data = new OperationResult(); break;
				case doctypQueryKizInfo: P_Data = new QueryKizInfo(); break;
				case doctypReceiveOrder: P_Data = new PPBillPacket(); break;
				case doctypMovePlace: P_Data = new PPBillPacket; break; // @v10.8.7 
			}
		}
		~Packet()
		{
			switch(DocType) {
				case doctypResult: delete static_cast<OperationResult *>(P_Data); break;
				case doctypQueryKizInfo: delete static_cast<QueryKizInfo *>(P_Data); break;
				case doctypReceiveOrder: delete static_cast<PPBillPacket *>(P_Data); break;
				case doctypMovePlace: delete static_cast<PPBillPacket *>(P_Data); break; // @v10.8.7 
			}
		}
		const  int DocType;
		long   Flags;
		void * P_Data;
	};
	
	int    SLAPI ParseDocument(const json_t * pJsonObj, Document & rItem);
	int    SLAPI ParseDocumentList(const char * pJsonInput, TSCollection <Document> & rList);
	int    SLAPI SetupInitBlock(PPID guaID, const char * pEndPoint, InitBlock & rBlk);
	int    SLAPI GetSign(const InitBlock & rIb, const void * pData, size_t dataLen, SString & rResultBuf);
	//SString & SLAPI MakeTargetUrl(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const;
	//SString & SLAPI MakeTargetUrl2(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const;
	SString & SLAPI MakeTargetUrl_(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const;
	enum {
		mhffTokenOnly  = 0x0001,
		mhffAuthBearer = 0x0002 // auth: Bearer else auth: Token
	};
	SString & SLAPI MakeHeaderFields(const char * pToken, uint flags, StrStrAssocArray * pHdrFlds, SString & rBuf);
	const CERT_CONTEXT * SLAPI GetClientSslCertificate(InitBlock & rIb);
	int    SLAPI MakeAuthRequest(InitBlock & rBlk, SString & rBuf);
	int    SLAPI MakeTokenRequest(InitBlock & rIb, const char * pAuthCode, SString & rBuf);
	int    SLAPI MakeAuthRequest2(InitBlock & rBlk, SString & rBuf);
	int    SLAPI MakeTokenRequest2(InitBlock & rIb, const char * pAuthCode, SString & rBuf);
	int    SLAPI MakeDocumentRequest(const InitBlock & rIb, const void * pData, size_t dataLen, S_GUID & rReqId, SString & rBuf);
	uint   SLAPI GetLastWinInternetResponse(SString & rMsgBuf);
	uint   SLAPI ReadReply(HINTERNET hReq, SString & rBuf);
	int    SLAPI GetUserInfo2(InitBlock & rIb);
	int    SLAPI GetIncomeDocList2_temp(InitBlock & rIb);
	enum {
		docfoldDocs     = 0,
		docfoldArc      = 1,
		docfoldBin      = 2,
		docfoldToSign   = 3,
		docfoldRejected = 4
	};
	struct DocumentFilt {
		SLAPI  DocumentFilt() : Flags(0), Folder(docfoldDocs), CountLimit(0), CountOffset(0)
		{
			Period.Z();
		}
		enum {
			fIncoming  = 0x0001,
			fOutcoming = 0x0002
		};
		int    Flags;
		int    Folder;
		uint   CountOffset;
		uint   CountLimit;
		DateRange Period;
	};
	int    SLAPI GetDocumentList(InitBlock & rIb, const DocumentFilt * pFilt, TSCollection <Document> & rList);
	int    SLAPI GetDocument(const InitBlock & rIb, const S_GUID * pUuid, const InetUrl * pUrl, Document & rDoc);
	int    SLAPI ReadJsonReplyForSingleItem(const char * pReply, const char * pTarget, SString & rResult);
	int    SLAPI TransmitDocument2(const InitBlock & rIb, const ChZnInterface::Packet & rPack, SString & rReply);
	int    SLAPI GetDocumentTicket(const InitBlock & rIb, const char * pDocIdent, SString & rTicket);
	int    SLAPI Connect(InitBlock & rIb);
	// @v10.8.0 int    SLAPI Connect2(InitBlock & rIb);
	int    SLAPI GetToken2(const char * pAuthCode, InitBlock & rIb);
	int    SLAPI GetPendingIdentList(const InitBlock & rIb, StringSet & rResult);
	int    SLAPI CommitTicket(const char * pPath, const char * pIdent, const char * pTicket);
	int    SLAPI ParseTicket(const char * pTicket, Packet ** ppP);
	int    SLAPI GetDebugPath(const InitBlock & rIb, SString & rPath);
private:
	int    SLAPI LogTalking(const char * pPrefix, const char * pTargetUrl, const SString & rMsg);
	int    SLAPI GetTemporaryFileName(const char * pPath, const char * pSubPath, const char * pPrefix, SString & rFn);
	int    SLAPI CreatePendingFile(const char * pPath, const char * pIdent);
};

static const SIntToSymbTabEntry ChZnDocStatusList[] = {
	{ ChZnInterface::docstInProgress, "IN_PROGRESS" },// IN_PROGRESS – Проверяется
	{ ChZnInterface::docstCheckedOk, "CHECKED_OK" }, // CHECKED_OK – Обработан
	{ ChZnInterface::docstCheckedNoOk, "CHECKED_NOT_OK" },// CHECKED_NOT_OK – Обработан с ошибками
	{ ChZnInterface::docstProcessingError, "PROCESSING_ERROR" }, // PROCESSING_ERROR – Техническая ошибка
	{ ChZnInterface::docstUndefined, "UNDEFINED" },// UNDEFINED – не определен
	{ ChZnInterface::docstCancelled, "CANCELLED" },// CANCELLED – Аннулирован. Только для документа 'Отмена отгрузки'
	{ ChZnInterface::docstAccepted, "ACCEPTED" },// ACCEPTED – Принят. Только для документа 'Отгрузка'
	{ ChZnInterface::docstWaitAcceptance, "WAIT_ACCEPTANCE" },// WAIT_ACCEPTANCE – Ожидание приемку. Только для документа 'Отгрузка'. Устанавливается при успешной обработке документа 'Отгрузка товара'
	{ ChZnInterface::docstParticipantRegistration,  "WAIT_PARTICIPANT_REGISTRATION" }// WAIT_PARTICIPANT_REGISTRATION -Ожидает регистрации участника в ГИС МТ. Только для документа 'Отгрузка'. Устанавливается при успешной обработке документа 'Отгрузка товара' в сторону незарегистрированного участника
};

static const SIntToSymbTabEntry ChZnDocTypeList[] = {
	{ ChZnInterface::doctOstDescription, "OST_DESCRIPTION" }, // OST_DESCRIPTION; Описание остатков товара;json;Описание остатков товара - JSON
		//OST_DESCRIPTION_CSV;csv;Описание остатков товара - CSV
		//OST_DESCRIPTION_XML;xml;Описание остатков товара - XML
	{ ChZnInterface::doctAggregation, "AGGREGATION_DOCUMENT" },//AGGREGATION_DOCUMENT;Агрегация;json;Агрегация - JSON
		//AGGREGATION_DOCUMENT_CSV;csv;Агрегация - CSV
		//AGGREGATION_DOCUMENT_XML;xml;Агрегация - XML
	{ ChZnInterface::doctDisaggregation, "DISAGGREGATION_DOCUMENT" }, //DISAGGREGATION_DOCUMENT;Расформирование агрегата;json;Расформирование агрегата - JSON
		//DISAGGREGATION_DOCUMENT_CSV;csv;Расформирование агрегата - CSV
		//DISAGGREGATION_DOCUMENT_XML;xml;Расформирование агрегата - XML
	{ ChZnInterface::doctReaggregation, "REAGGREGATION_DOCUMENT" }, //REAGGREGATION_DOCUMENT;Трансформация агрегата;json;Трансформация агрегата - JSON
		//REAGGREGATION_DOCUMENT_XML;xml;Трансформация агрегата - XML
		//REAGGREGATION_DOCUMENT_CSV;csv;Трансформация агрегата - CSV
	{ ChZnInterface::doctLpIntroduceGoods, "LP_INTRODUCE_GOODS" }, //LP_INTRODUCE_GOODS;Ввод в оборот. Производство РФ;json;Ввод в оборот. Производство РФ - JSON
		//LP_INTRODUCE_GOODS_CSV;csv;Ввод в оборот. Производство РФ - CSV
		//LP_INTRODUCE_GOODS_XML;xml;Ввод в оборот. Производство РФ - XML
	{ ChZnInterface::doctLkIndiCommissioning, "LK_INDI_COMMISSIONING" }, //LK_INDI_COMMISSIONING;Ввод в оборот. Полученных от физических лиц;json;Ввод в оборот. Полученных от физических лиц - JSON
		//LK_INDI_COMMISSIONING_CSV;csv;Ввод в оборот. Полученных от физических лиц - CSV
		//LK_INDI_COMMISSIONING_XML;xml;Ввод в оборот. Полученных от физических лиц - XML
	{ ChZnInterface::doctLpGoodsImport, "LP_GOODS_IMPORT" }, //LP_GOODS_IMPORT;json;Ввод в оборот. Производство вне ЕАЭС - JSON
		//LP_GOODS_IMPORT_CSV;Ввод в оборот. Производство вне ЕАЭС;csv;Ввод в оборот. Производство вне ЕАЭС - CSV
		//LP_GOODS_IMPORT_XML;xml;Ввод в оборот. Производство вне ЕАЭС - XML
	{ ChZnInterface::doctCrossborder, "CROSSBORDER" }, //CROSSBORDER;Ввод в оборот. Трансграничная торговля.;json;Ввод в оборот. Трансграничная торговля. JSON (MANUAL)
		//CROSSBORDER_CSV;csv;Ввод в оборот. Трансграничная торговля. CSV
		//CROSSBORDER_XML;xml;Ввод в оборот. Трансграничная торговля. XML
	{ ChZnInterface::doctLpIntroduceGoodsCrossborderCSD, "LP_INTRODUCE_GOODS_CROSSBORDER_CSD" }, //LP_INTRODUCE_GOODS_CROSSBORDER_CSD_JSON;Ввод в оборот. На территории стран ЕАЭС (контрактное производство);json;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). JSON (MANUAL)
		//LP_INTRODUCE_GOODS_CROSSBORDER_CSD_XML;xml;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). CSV
		//LP_INTRODUCE_GOODS_CROSSBORDER_CSD_CSV;csv;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). XML
	{ ChZnInterface::doctIntroduceOST, "LP_INTRODUCE_OST" }, //LP_INTRODUCE_OST;Ввод в оборот. Маркировка остатков;json;Ввод в оборот. Маркировка остатков - JSON
		//LP_INTRODUCE_OST_CSV;csv;Ввод в оборот. Маркировка остатков - CSV
		//LP_INTRODUCE_OST_XML;xml;Ввод в оборот. Маркировка остатков - XML
	{ ChZnInterface::doctLkContractCommissioning, "LK_CONTRACT_COMMISSIONING" }, //LK_CONTRACT_COMMISSIONING;Ввод в оборот. Контрактное производство РФ;json;Ввод в оборот. Контрактное производство РФ - JSON
		//LK_CONTRACT_COMMISSIONING_CSV;csv;Ввод в оборот. Контрактное производство РФ - CSV
		//LK_CONTRACT_COMMISSIONING_XML;xml;Ввод в оборот. Контрактное производство РФ - XML
	{ ChZnInterface::doctLpReturn, "LP_RETURN" }, //LP_RETURN;Возврат в оборот;json;Возврат в оборот. JSON (MANUAL)
		//LP_RETURN_CSV;xml;Возврат в оборот. CSV
		//LP_RETURN_XML;csv;Возврат в оборот. XML
	{ ChZnInterface::doctLpShipGoods, "LP_SHIP_GOODS" }, //LP_SHIP_GOODS;Отгрузка;json;Отгрузка - JSON
		//LP_SHIP_GOODS_CSV;csv;Отгрузка - CSV
		//LP_SHIP_GOODS_XML;xml;Отгрузка - XML
	{ ChZnInterface::doctLpShipReceipt, "LP_SHIP_RECEIPT" }, //LP_SHIP_RECEIPT;Отгрузка с выводом из оборота.;json;Отгрузка с выводом из оборота. JSON (MANUAL)
		//LP_SHIP_RECEIPT_CSV;csv;Отгрузка с выводом из оборота. CSV
		//LP_SHIP_RECEIPT_XML;xml;Отгрузка с выводом из оборота. XML
	{ ChZnInterface::doctLpCancelShipment, "LP_CANCEL_SHIPMENT" }, //LP_CANCEL_SHIPMENT;Отмена отгрузки;json;Отмена отгрузки. JSON (MANUAL)
	{ ChZnInterface::doctLpAcceptGoods, "LP_ACCEPT_GOODS" }, //LP_ACCEPT_GOODS;Приемка;json;Приемка - JSON
		//LP_ACCEPT_GOODS_XML;xml;Приемка - XML
	{ ChZnInterface::doctLkReceipt, "LK_RECEIPT" }, //LK_RECEIPT;Вывод товара из оборота;json;Вывод товара из оборота - JSON
		//LK_RECEIPT_CSV;csv;Вывод товара из оборота - CSV
		//LK_RECEIPT_XML;xml;Вывод товара из оборота - XML
	{ ChZnInterface::doctLkRemark, "LK_REMARK" }, //LK_REMARK;Перемаркировка;json;Перемаркировка - JSON
		//LK_REMARK_CSV;csv;Перемаркировка - CSV
		//LK_REMARK_XML;xml;Перемаркировка - XML
	{ ChZnInterface::doctKmCancellation, "LK_KM_CANCELLATION" }, //LK_KM_CANCELLATION;Списание ненанесенных КМ;json;Списание ненанесенных КМ - JSON
		//LK_KM_CANCELLATION_XML;xml;Списание ненанесенных КМ - XML
		//LK_KM_CANCELLATION_XSD;xsd;Списание ненанесенных КМ - XSD
	{ ChZnInterface::doctAppliedKmCancellation, "LK_APPLIED_KM_CANCELLATION" },//LK_APPLIED_KM_CANCELLATION;Списание нанесенных КМ;json;Списание нанесенных КМ - JSON
		//LK_APPLIED_KM_CANCELLATION_XML;xml;Списание нанесенных КМ - XML
		//LK_APPLIED_KM_CANCELLATION_XSD;xsd;Списание нанесенных КМ - XSD
};

static int ChZnDocStatusFromStr(const char * pText)
{
	return SIntToSymbTab_GetId(ChZnDocStatusList, SIZEOFARRAY(ChZnDocStatusList), pText);
}

static int ChZnDocTypeFromStr(const char * pText, int * pType, int * pFormat)
{
	SString temp_buf(pText);
	int    format = 0;
	if(temp_buf.CmpSuffix("_CSV", 1) == 0) {
		format = SFileFormat::Csv;
		temp_buf.Trim(temp_buf.Len()-4);
	}
	else if(temp_buf.CmpSuffix("_XML", 1) == 0) {
		format = SFileFormat::Xml;
		temp_buf.Trim(temp_buf.Len()-4);
	}
	else if(temp_buf.CmpSuffix("_XSD", 1) == 0) {
		format = SFileFormat::Xsd;
		temp_buf.Trim(temp_buf.Len()-4);
	}
	int type = SIntToSymbTab_GetId(ChZnDocTypeList, SIZEOFARRAY(ChZnDocTypeList), temp_buf);
	ASSIGN_PTR(pType, type);
	ASSIGN_PTR(pFormat, format);
	return BIN(type);
}

int SLAPI ChZnInterface::ParseDocument(const json_t * pJsonObj, Document & rItem)
{
	int    ok = -1;
	if(pJsonObj && pJsonObj->Type == json_t::tOBJECT) {
		const json_t * p_next = pJsonObj->P_Next;
		const json_t * p_fld_next = 0;
		for(const json_t * p_fld = pJsonObj->P_Child; p_fld; p_fld = p_fld_next) {
			p_fld_next = p_fld->P_Next;
			if(p_fld->P_Child) {
				ok = 1;
				if(sstreqi_ascii(p_fld->Text, "number")) {
					rItem.Uuid.FromStr(p_fld->P_Child->Text);
				}
				else if(sstreqi_ascii(p_fld->Text, "docDate")) {
					strtodatetime(p_fld->P_Child->Text, &rItem.Dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS|TIMF_MSEC);
				}
				else if(sstreqi_ascii(p_fld->Text, "receivedAt")) {
					strtodatetime(p_fld->P_Child->Text, &rItem.ReceivedDtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS|TIMF_MSEC);
				}
				else if(sstreqi_ascii(p_fld->Text, "type")) {
					ChZnDocTypeFromStr(p_fld->P_Child->Text, &rItem.Type, &rItem.Format);
				}
				else if(sstreqi_ascii(p_fld->Text, "status")) {
					rItem.Status = ChZnDocStatusFromStr(p_fld->P_Child->Text);
				}
				else if(sstreqi_ascii(p_fld->Text, "senderName")) {
					rItem.SenderName = p_fld->P_Child->Text;
				}
				else if(sstreqi_ascii(p_fld->Text, "receiverName")) {
					rItem.ReceiverName = p_fld->P_Child->Text;
				}
				else if(sstreqi_ascii(p_fld->Text, "downloadStatus")) {
					rItem.DownloadStatus = ChZnDocStatusFromStr(p_fld->P_Child->Text);
				}
				else if(sstreqi_ascii(p_fld->Text, "downloadDesc")) {
				}
				else if(sstreqi_ascii(p_fld->Text, "input")) { // bool
					if(p_fld->P_Child->Type == json_t::tTRUE)
						rItem.Flags |= rItem.fInput;
					else
						rItem.Flags &= ~rItem.fInput;
				}
				else if(sstreqi_ascii(p_fld->Text, "pdfFile")) { 
				}
				else if(sstreqi_ascii(p_fld->Text, "docErrors")) { // []
				}
				else if(sstreqi_ascii(p_fld->Text, "pdfFile")) { 
				}
				else if(sstreqi_ascii(p_fld->Text, "documentDataDto")) { 
				}
				else if(sstreqi_ascii(p_fld->Text, "body")) { 
					rItem.Body = p_fld->P_Child->Text;
				}
				else if(sstreqi_ascii(p_fld->Text, "content")) { 
					rItem.Content = p_fld->P_Child->Text;
				}
				/*else if(sstreqi_ascii(p_fld->Text, "total")) { 
				}*/
			}
		}
	}
	return ok;
}

int  SLAPI ChZnInterface::ParseDocumentList(const char * pJsonInput, TSCollection <Document> & rList)
{
	int    ok = -1;
	json_t * p_json_doc = 0;
	if(json_parse_document(&p_json_doc, pJsonInput) == JSON_OK) {
		SString temp_buf;
		json_t * p_next = 0;
		json_t * p_fld_next = 0;
		if(p_json_doc) {
			if(p_json_doc->Type == json_t::tOBJECT) {
				json_t * p_child = p_json_doc->P_Child;
				if(p_child && p_child->Type == json_t::tSTRING && sstreqi_ascii(p_child->Text, "results")) {
					ok = 1;
					p_child = p_child->P_Child;
					if(p_child && p_child->Type == json_t::tARRAY) {
						for(json_t * p_cur = p_child->P_Child; p_cur; p_cur = p_next) {
							if(p_cur->Type == json_t::tOBJECT) {
								p_next = p_cur->P_Next;
								Document * p_new_entry = rList.CreateNewItem();
								ParseDocument(p_cur, *p_new_entry);
							}
							else
								break;
						}
					}
				}

			}
		}
	}
	delete p_json_doc;
	return ok;
}

int SLAPI ChZnInterface::Document::Parse(const char * pBuffer)
{
	int    ok = 0;
	if(!isempty(pBuffer)) {
		const char * p_src = pBuffer;
		while(oneof2(*p_src, ' ', '\t'))
			p_src++;
		if(sstrneq(p_src, "<xml", 4)) { // xml

		}
		else if(sstrneq(p_src, "{", 1)) { // json
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI ChZnInterface::Document::GetTransactionPartyCode(PPID psnID, PPID locID, SString & rCode)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	rCode.Z();
	if(locID && p_ref->Ot.GetTagStr(PPOBJ_LOCATION, locID, PPTAG_LOC_CHZNCODE, rCode) > 0)
		ok = 1;
	else if(psnID && p_ref->Ot.GetTagStr(PPOBJ_PERSON, psnID, PPTAG_PERSON_CHZNCODE, rCode) > 0)
		ok = 1;
	return ok;
}

int SLAPI ChZnInterface::Document::Make(SXml::WDoc & rX, const ChZnInterface::InitBlock & rIb, const ChZnInterface::Packet * pPack)
{
	int    ok = 1;
	SString temp_buf;
	SString mark_buf;
	SString subj_ident;
	SString shipper_ident;
	SString receiver_ident;
	StringSet ss;
	PPLotExtCodeContainer::MarkSet lotxcode_set;
	GtinStruc gts;
	{
		SXml::WNode wdocs(rX, "documents");
		wdocs.PutAttrib("session_ui", rIb.Token);
		wdocs.PutAttrib("version", "1.34");
		wdocs.PutAttrib(SXml::nst("xmlns", "xsi"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
		{
			SIntToSymbTab_GetSymb(CzDocType_SymbTab, SIZEOFARRAY(CzDocType_SymbTab), pPack->DocType, temp_buf);
			SXml::WNode wd(rX, temp_buf);
			wd.PutAttrib("action_id", temp_buf.Z().Cat(pPack->DocType));
			//
			if(pPack->DocType == doctypMovePlace) {
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   rcvr_ar_id = p_bp->Rec.Object;
					const PPID   rcvr_loc_id = PPObjLocation::ObjToWarehouse(rcvr_ar_id);
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					PPID   subj_psn_id = 0;
					GetMainOrgID(&subj_psn_id);
					GetTransactionPartyCode(0, rcvr_loc_id, receiver_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					wd.PutInner("receiver_id", receiver_ident);
					temp_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
					TimeZoneFmt(0, tzfmtConcat|tzfmtColon|tzfmtCurrent, temp_buf);
					wd.PutInner("operation_date", temp_buf);
					temp_buf = p_bp->Rec.Code;
					BillCore::GetCode(temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
					wd.PutInner("doc_num", temp_buf);
					wd.PutInner("doc_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
					{
						SXml::WNode dtl(rX, "order_details");
						for(uint i = 0; i < p_bp->GetTCount(); i++) {
							const PPTransferItem & r_ti = p_bp->ConstTI(i);
							p_bp->XcL.Get(i+1, 0, lotxcode_set);
							lotxcode_set.GetByBoxID(0, ss);
							for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
								const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
								if(pczcr > 0) {
									mark_buf.Z();
									if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
										mark_buf.Cat(temp_buf);
										if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
											mark_buf.Cat(temp_buf);
											dtl.PutInner("sgtin", mark_buf);
										}
									}
								}
							}
						}
					}
				}
			}
			else if(pPack->DocType == doctypReceiveOrder) {
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   dlvr_ar_id = p_bp->Rec.Object;
					const PPID   dlvr_psn_id = ObjectToPerson(dlvr_ar_id, 0);
					const PPID   dlvr_loc_id = p_bp->GetDlvrAddrID();
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					PPID   subj_psn_id = 0;
					GetMainOrgID(&subj_psn_id);
					GetTransactionPartyCode(dlvr_psn_id, dlvr_loc_id, shipper_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					wd.PutInner("shipper_id", shipper_ident);
					temp_buf.Z().Cat(getcurdatetime_(), DATF_ISO8601|DATF_CENTURY, 0);
					TimeZoneFmt(0, tzfmtConcat|tzfmtColon|tzfmtCurrent, temp_buf);
					wd.PutInner("operation_date", temp_buf);
					temp_buf = p_bp->Rec.Code;
					BillCore::GetCode(temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
					wd.PutInner("doc_num", temp_buf);
					wd.PutInner("doc_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
					wd.PutInner("receive_type", temp_buf.Z().Cat(1L));
					wd.PutInner("source", temp_buf.Z().Cat(1L));
					wd.PutInner("contract_type", temp_buf.Z().Cat(1L));
					{
						SXml::WNode dtl(rX, "order_details");
						for(uint i = 0; i < p_bp->GetTCount(); i++) {
							const PPTransferItem & r_ti = p_bp->ConstTI(i);
							double cost = r_ti.Cost;
							double vat_in_cost = 0.0;
							{
								GTaxVect vect;
								vect.CalcTI(r_ti, p_bp->Rec.OpID, TIAMT_COST);
								vat_in_cost = vect.GetValue(GTAXVF_VAT) / fabs(r_ti.Quantity_);
							}
							p_bp->XcL.Get(i+1, 0, lotxcode_set);
							{
								lotxcode_set.GetByBoxID(0, ss);
								for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
									const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
									if(pczcr > 0) {
										mark_buf.Z();
										if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
											mark_buf.Cat(temp_buf);
											if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
												mark_buf.Cat(temp_buf);
												SXml::WNode un(rX, "union");
												un.PutInner("sgtin", mark_buf);
												un.PutInner("cost", temp_buf.Z().Cat(cost, MKSFMTD(0, 2, 0)));
												un.PutInner(/*"vat_in_cost"*/"vat_value", temp_buf.Z().Cat(vat_in_cost, MKSFMTD(0, 2, 0)));
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else if(pPack->DocType == doctypQueryKizInfo) {
				const Packet::QueryKizInfo * p_bp = static_cast<const Packet::QueryKizInfo *>(pPack->P_Data);
				if(p_bp) {
					subj_ident = p_bp->SubjectIdent;
					if(subj_ident.Empty()) {
						PPID   dlvr_psn_id = ObjectToPerson(p_bp->ArID, 0);
						PPID   subj_psn_id = 0;
						GetMainOrgID(&subj_psn_id);
						GetTransactionPartyCode(/*dlvr_psn_id*/subj_psn_id, 0, subj_ident);
					}
					wd.PutInnerSkipEmpty("subject_id", subj_ident);
					//int codetype = PPChZnPrcssr::IsChZnCode(p_bp->Code);
					GtinStruc gts;
					const int pczcr = PPChZnPrcssr::ParseChZnCode(p_bp->Code, gts, 0);
					if(pczcr > 0) {
						wd.PutInner("sgtin", p_bp->Code);
					}
					//SETIFZ(codetype, p_bp->CodeType);
					/*if(codetype == SNTOK_CHZN_GS1_GTIN) {
						wd.PutInner("sgtin", p_bp->Code);
					}
					else if(codetype == SNTOK_CHZN_SIGN_SGTIN) {
						wd.PutInner("sgtin", p_bp->Code);
					}
					else if(codetype == SNTOK_CHZN_SSCC) {
						if(p_bp->Modifier == 1) // down
							wd.PutInner("sscc_down", p_bp->Code);
						else if(p_bp->Modifier == 2) // up
							wd.PutInner("sscc_up", p_bp->Code);
						else // default - down
							wd.PutInner("sscc_down", p_bp->Code);
					}*/
				}
			}
		}
	}
	return ok;
}

int SLAPI ChZnInterface::SetupInitBlock(PPID guaID, const char * pEndPoint, InitBlock & rBlk)
{
	int    ok = 1;
	int    protocol_id = 0;
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	THROW(gua_obj.GetPacket(guaID, &rBlk.GuaPack) > 0);
	rBlk.EndPoint = pEndPoint;
	rBlk.GuaPack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, rBlk.CliAccsKey);
	rBlk.GuaPack.TagL.GetItemStr(PPTAG_GUA_SECRET, rBlk.CliSecret);
	rBlk.GuaPack.TagL.GetItemStr(PPTAG_GUA_LOGIN, rBlk.CliIdent); // Отпечаток открытого ключа 
		// Сертификат в реестре находится в "сертификаты-текущий пользователь/личное/реестр/сертификаты". 
		// Требуется в доверенные еще внести сертификат Крипто-Про (в инструкции по быстрому старту про это есть). 
	rBlk.GuaPack.TagL.GetItemStr(PPTAG_GUA_CERTSUBJCN, rBlk.Cn);
	if(rBlk.Cn.NotEmptyS()) {
		//rBlk.Cn.ReplaceStr("\"", " ", 0);
		//rBlk.Cn.ReplaceStr("  ", " ", 0);
		rBlk.Cn.Transf(CTRANSF_INNER_TO_OUTER);
	}
	if(rBlk.GuaPack.TagL.GetItemStr(PPTAG_GUA_PROTOCOL, temp_buf) > 0) {
		StringSet ss;
		temp_buf.Tokenize(" ,;", ss);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			temp_buf.Strip();
			if(temp_buf.IsEqiAscii("v1") || temp_buf == "1") {
				if(!rBlk.ProtocolVer)
					rBlk.ProtocolVer = 1;
			}
			else if(temp_buf.IsEqiAscii("v3") || temp_buf == "3") {
				if(!rBlk.ProtocolVer)
					rBlk.ProtocolVer = 3;
			}
			else if(temp_buf.IsEqiAscii("mdlp"))
				protocol_id = InitBlock::protidMdlp;
			else if(temp_buf.IsEqiAscii("gismt") || temp_buf.IsEqiAscii("gis-mt"))
				protocol_id = InitBlock::protidGisMt;
			else if(temp_buf.IsEqiAscii("edoltmdlp") || temp_buf.IsEqiAscii("edo-lite-mdlp"))
				protocol_id = InitBlock::protidEdoLtMdlp;
			else if(temp_buf.IsEqiAscii("edoltint") || temp_buf.IsEqiAscii("edo-lite-int"))
				protocol_id = InitBlock::protidEdoLtInt;
			else if(temp_buf.IsEqiAscii("edoltelk") || temp_buf.IsEqiAscii("edo-lite-elk"))
				protocol_id = InitBlock::protidEdoLtElk;
		}
	}
	THROW_PP(protocol_id, PPERR_CHZN_PROTOCOLTAGNDEF);
	rBlk.ProtocolId = protocol_id;
	if(protocol_id == InitBlock::protidMdlp) {
		THROW_PP(rBlk.CliAccsKey.NotEmpty(), PPERR_CHZN_MDLPKEYSNDEF);
		THROW_PP(rBlk.CliSecret.NotEmpty(), PPERR_CHZN_MDLPKEYSNDEF);
		THROW_PP(rBlk.CliIdent.NotEmpty(), PPERR_CHZN_MDLPKEYSNDEF);
	}
	rBlk.GuaID = guaID;
	{
		PPIniFile ini_file;
		if(ini_file.Get(PPINISECT_PATH, PPINIPARAM_PATH_CRYPTOPRO, rBlk.CryptoProPath) <= 0)
			ini_file.Get(PPINISECT_PATH, PPINIPARAM_PATH_CRYPTOPRO_BAD, rBlk.CryptoProPath);
	}
	CATCHZOK
	return ok;
}
	
int SLAPI ChZnInterface::GetSign(const InitBlock & rIb, const void * pData, size_t dataLen, SString & rResultBuf)
{
	int    ok = -1;
	rResultBuf.Z();
	if(pData && dataLen) {
		SString temp_buf;
		THROW_PP(rIb.CryptoProPath.NotEmpty(), PPERR_CHZN_CRYPTOPROPATHNDEF);
		THROW_PP(IsDirectory(rIb.CryptoProPath), PPERR_CHZN_CRYPTOPROPATHINV);
		THROW_PP(rIb.Cn.NotEmpty(), PPERR_CHZN_CERTSUBJNDEF);
		//"C:\Program Files\Crypto Pro\CSP\csptest" -sfsign -sign -in barcode-tobacco.7z –out barcode-tobacco.sign -my "ООО ЛУИЗА ПЛЮС" -detached -base64 –add -silent
		temp_buf.Cat(rIb.CryptoProPath).SetLastSlash().Cat("csptest.exe");
		THROW_SL(fileExists(temp_buf));
		{
			SString in_file_name;
			SString out_file_name;
			SString cn;
			PPMakeTempFileName("chzn", "st", 0, in_file_name);
			PPMakeTempFileName("chzn", "sf", 0, out_file_name);
			{
				SFile f_in(in_file_name, SFile::mWrite|SFile::mBinary);
				THROW_SL(f_in.IsValid());
				THROW_SL(f_in.Write(pData, dataLen));
			}
			{
				cn = rIb.Cn;
				cn.ReplaceStr("\"", " ", 0);
				cn.ReplaceStr("  ", " ", 0);
				cn.Strip();
			}
			temp_buf.Space().Cat("-sfsign").Space().Cat("-sign").Space().Cat("-in").Space().Cat(in_file_name).Space().
				Cat("-out").Space().Cat(out_file_name).Space().Cat("-my").Space().CatQStr(cn);
			//temp_buf.Space().Cat("-detached");
			temp_buf.Space();
			temp_buf.Cat("-base64").Space().Cat("-add");
			LogTalking("crypto-pro-cmdline", 0, temp_buf);
			STempBuffer cmd_line((temp_buf.Len() + 32) * sizeof(TCHAR));
			STARTUPINFO si;
			DWORD exit_code = 0;
			PROCESS_INFORMATION pi;
			MEMSZERO(si);
			si.cb = sizeof(si);
			MEMSZERO(pi);
			strnzcpy(static_cast<TCHAR *>(cmd_line.vptr()), SUcSwitch(temp_buf), cmd_line.GetSize() / sizeof(TCHAR));
			int    r = ::CreateProcess(0, static_cast<TCHAR *>(cmd_line.vptr()), 0, 0, FALSE, 0, 0, 0, &si, &pi);
			if(!r) {
				SLS.SetOsError(0);
				CALLEXCEPT_PP(PPERR_SLIB);
			}
			WaitForSingleObject(pi.hProcess, INFINITE); // Wait until child process exits.
			{
				GetExitCodeProcess(pi.hProcess, &exit_code);
				if(exit_code == 0) {
					SFile f_result(out_file_name, SFile::mRead);
					THROW_SL(f_result.IsValid());
					while(f_result.ReadLine(temp_buf)) {
						temp_buf.Chomp();
						rResultBuf.Cat(temp_buf);
					}
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

SString & SLAPI ChZnInterface::MakeTargetUrl_(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const
{
	if(rIb.EndPoint.NotEmpty())
		rResult = rIb.EndPoint;
	else {
		switch(rIb.ProtocolId) {
			case InitBlock::protidEdoLtMdlp:
				(rResult = "https").Cat("://").Cat("mdlp").Dot().Cat("edo").Dot().Cat("crpt").Dot().Cat("tech");
				break;
			case InitBlock::protidEdoLtInt:
				(rResult = "https").Cat("://").Cat("int").Dot().Cat("edo").Dot().Cat("crpt").Dot().Cat("tech");
				break;
			case InitBlock::protidEdoLtElk:
				(rResult = "https").Cat("://").Cat("elk").Dot().Cat("edo").Dot().Cat("crpt").Dot().Cat("tech");
				break;
			case InitBlock::protidGisMt:
				if(rIb.GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)
					(rResult = "https").Cat("://").Cat("demo").Dot().Cat("lp").Dot().Cat("crpt").Dot().Cat("tech");
				else 
					(rResult = "https").Cat("://").Cat("ismp").Dot().Cat("crpt").Dot().Cat("ru");
				break;
			case InitBlock::protidMdlp:
			default:
				//rResult = (query == qDocumentSend) ? "https" : "http"; // @test
				// https://api.mdlp.crpt.ru:443/api/v1/documents/send
				if(oneof3(query, qDocumentSend, qGetDoc, qGetTicket)) 
					rResult = "https";
				else
					rResult = "http";
				rResult.Cat("://").Cat("api").Dot();
				if(rIb.GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)
					rResult.Cat("sb").Dot();
				rResult.Cat("mdlp.crpt.ru");
				if(query == qDocumentSend) 
					rResult.Cat(":443");
				rResult.SetLastDSlash().Cat("api/v1");
				break;
		}
	}
	rResult.SetLastDSlash();
	switch(query) {
		case qAuth:  
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("auth"); 
			else if(rIb.ProtocolId == InitBlock::protidGisMt) {
				rResult.Cat("api/v3/auth/cert/key"); 
			}
			else if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
				rResult.Cat("api/v1/session"); 
			}
			break;
		case qToken: 
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("token"); 
			else if(rIb.ProtocolId == InitBlock::protidGisMt) {
				rResult.Cat("api/v3/auth/cert/"/*"facade/auth"*/); 
			}
			else if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
				rResult.Cat("api/v1/session"); 
			}
			break;
		case qDocOutcome: 
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("documents/outcome"); 
			else {
			}
			break;
		case qCurrentUserInfo: 
			rResult.Cat("users/current"); 
			break;
		case qGetDoc: 
			if(rIb.ProtocolId == InitBlock::protidGisMt) {
				//api/v3/facade/doc/{docId}/body
				rResult.Cat("api/v3/facade/doc/");
				if(pAddendum) {
					rResult.Cat(pAddendum).CatChar('/');
				}
				rResult.Cat("body");
			}
			break;
		case qGetDocList: 
		case qGetIncomingDocList: 
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("documents/income"); 
			else if(rIb.ProtocolId == InitBlock::protidGisMt) {
				rResult.Cat("api/v3/facade/doc/listV2");
			}
			else if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
				rResult.Cat("api/v1/incoming-documents");
			}
			break;
		case qGetOutcomingDocList: 
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("documents/income"); 
			else if(rIb.ProtocolId == InitBlock::protidGisMt) {
				rResult.Cat("api/v3/facade/doc/listV2");
			}
			else if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
				rResult.Cat("api/v1/outgoing-documents");
			}
			break;
		case qDocumentSend: 
			rResult.Cat("documents/send"); 
			break;
		case qGetTicket: 
			rResult.Cat("documents");
			if(!isempty(pAddendum))
				rResult.CatChar('/').Cat(pAddendum);
			rResult.CatChar('/').Cat("ticket");
			break;
	}
	return rResult;
}
	
#if 0 // {
SString & SLAPI ChZnInterface::MakeTargetUrl(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const
{
	//rResult = "http://api.sb.mdlp.crpt.ru";
	//(rResult = (oneof2(query, qAuth, qToken) ? "http" : "https")).Cat("://");
	if(rIb.EndPoint.NotEmpty())
		rResult = rIb.EndPoint;
	else {
		(rResult = (oneof2(query, qAuth, qToken) ? "http" : "http")).Cat("://").Cat("api").Dot();
		if(rIb.GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)
			rResult.Cat("sb").Dot();
		rResult.Cat("mdlp.crpt.ru").SetLastDSlash().Cat("api/v1");
	}
	rResult.SetLastDSlash();
	switch(query) {
		case qAuth:  rResult.Cat("auth"); break;
		case qToken: rResult.Cat("token"); break;
		case qDocOutcome: rResult.Cat("documents/outcome"); break;
		case qCurrentUserInfo: rResult.Cat("users/current"); break;
		case qGetDocList: rResult.Cat("documents/income"); break;
		case qDocumentSend: rResult.Cat("documents/send"); break;
		case qGetTicket: 
			rResult.Cat("documents");
			if(!isempty(pAddendum))
				rResult.CatChar('/').Cat(pAddendum);
			rResult.CatChar('/').Cat("ticket");
			break;
	}
	return rResult;
}

SString & SLAPI ChZnInterface::MakeTargetUrl2(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const
{
	//rResult = "http://api.sb.mdlp.crpt.ru";
	//(rResult = (oneof2(query, qAuth, qToken) ? "http" : "https")).Cat("://");
	//https://ismp.crpt.ru/api/v3/auth/cert/key
	//https://demo.lp.crpt.tech
	// /api/v3/facade/auth"
	if(rIb.EndPoint.NotEmpty())
		rResult = rIb.EndPoint;
	else if(rIb.GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)
		(rResult = "https").Cat("://").Cat("demo").Dot().Cat("lp").Dot().Cat("crpt").Dot().Cat("tech");
	else 
		(rResult = "https").Cat("://").Cat("ismp").Dot().Cat("crpt").Dot().Cat("ru");
	rResult.SetLastDSlash().Cat("api/v3").SetLastDSlash();
	switch(query) {
		case qAuth:  rResult.Cat("auth/cert/key"); break;
		case qToken: rResult.Cat("auth/cert/"/*"facade/auth"*/); break;
		/*
		case qDocOutcome: rResult.Cat("documents/outcome"); break;
		case qCurrentUserInfo: rResult.Cat("users/current"); break;
		case qGetDocList: rResult.Cat("documents/income"); break;
		case qDocumentSend: rResult.Cat("documents/send"); break;
		case qGetTicket: 
			rResult.Cat("documents");
			if(!isempty(pAddendum))
				rResult.CatChar('/').Cat(pAddendum);
			rResult.CatChar('/').Cat("ticket");
			break;
		*/
	}
	return rResult;
}
#endif // } 0

int SLAPI ChZnInterface::MakeAuthRequest2(InitBlock & rBlk, SString & rBuf)
{
	// GET https://ismp.crpt.ru/api/v3/auth/cert/key
	int    ok = 0;
	return ok;
}

int SLAPI ChZnInterface::MakeTokenRequest2(InitBlock & rIb, const char * pAuthCode, SString & rBuf)
{
	// POST https://ismp.crpt.ru/api/v3/auth/cert
	int    ok = 0;
	return ok;
}

int SLAPI ChZnInterface::MakeAuthRequest(InitBlock & rBlk, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	json_t * p_json_req = 0;
	{
		p_json_req = new json_t(json_t::tOBJECT);
		p_json_req->InsertString("client_id", rBlk.CliAccsKey);
		p_json_req->InsertString("client_secret", rBlk.CliSecret);
		p_json_req->InsertString("user_id", rBlk.CliIdent);
		p_json_req->InsertString("auth_type", "SIGNED_CODE");
		THROW_SL(json_tree_to_string(p_json_req, rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int SLAPI ChZnInterface::MakeTokenRequest(InitBlock & rIb, const char * pAuthCode, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString code_sign;
	json_t * p_json_req = 0;
	THROW(GetSign(rIb, pAuthCode, sstrlen(pAuthCode), code_sign));
	{
		p_json_req = new json_t(json_t::tOBJECT);
		p_json_req->InsertString("code", pAuthCode);
		p_json_req->InsertString("signature", code_sign);
		THROW_SL(json_tree_to_string(p_json_req, rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int SLAPI ChZnInterface::MakeDocumentRequest(const InitBlock & rIb, const void * pData, size_t dataLen, S_GUID & rReqId, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString code_sign;
	SString temp_buf;
	SString data_base64_buf;
	json_t * p_json_req = 0;
	THROW(GetSign(rIb, pData, dataLen, code_sign));
	data_base64_buf.EncodeMime64(pData, dataLen);
	{
		p_json_req = new json_t(json_t::tOBJECT);
		p_json_req->InsertString("document", data_base64_buf);
		p_json_req->InsertString("sign", code_sign);
		rReqId.Generate();
		rReqId.ToStr(S_GUID::fmtIDL, temp_buf);
		p_json_req->InsertString("request_id", temp_buf);
		THROW_SL(json_tree_to_string(p_json_req, rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

const CERT_CONTEXT * SLAPI ChZnInterface::GetClientSslCertificate(InitBlock & rIb)
{
	uint  sys_err = 0;
	const CERT_CONTEXT * p_cert_context = NULL;
	HCERTSTORE h_store = CertOpenStore(CERT_STORE_PROV_SYSTEM, /*X509_ASN_ENCODING*/0, 0, /*CERT_SYSTEM_STORE_LOCAL_MACHINE*/CERT_SYSTEM_STORE_CURRENT_USER, L"MY");
	if(h_store) {
		SStringU cnu;
		SString temp_buf = rIb.Cn;
		//temp_buf.ReplaceStr("\"", " ", 0);
		//temp_buf.ReplaceStr("  ", " ", 0);
		temp_buf.Strip();
		cnu.CopyFromMb_OUTER(temp_buf, temp_buf.Len());

		//const CERT_CONTEXT * p_cert_next = CertFindCertificateInStore(h_store, X509_ASN_ENCODING|PKCS_7_ASN_ENCODING, 0, 
		CERT_INFO * p_cert_info = 0; // @debug
		wchar_t cert_text[256];
		for(const CERT_CONTEXT * p_cert_next = CertEnumCertificatesInStore(h_store, 0); p_cert_next; p_cert_next = CertEnumCertificatesInStore(h_store, p_cert_next)) {
			p_cert_info = p_cert_next->pCertInfo;
			DWORD para_type = 0;
			CertGetNameString(p_cert_next, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, &para_type, cert_text, SIZEOFARRAY(cert_text));
			if(cnu.IsEqual(cert_text)) {
				p_cert_context = CertDuplicateCertificateContext(p_cert_next);
				break;
			}
		}

		//p_cert_context = CertFindCertificateInStore(h_store, X509_ASN_ENCODING|PKCS_7_ASN_ENCODING, 
			//0, CERT_FIND_SUBJECT_STR_W, static_cast<const wchar_t *>(cnu)/*use appropriate subject name*/, NULL);
		CertCloseStore(h_store, CERT_CLOSE_STORE_CHECK_FLAG);
	}
	sys_err = ::GetLastError();
	return p_cert_context;
}

uint SLAPI ChZnInterface::GetLastWinInternetResponse(SString & rMsgBuf)
{
	rMsgBuf.Z();
	DWORD last_err = 0;
	TCHAR last_err_text[1024];
	DWORD last_err_text_len = SIZEOFARRAY(last_err_text);
	InternetGetLastResponseInfo(&last_err, last_err_text, &last_err_text_len);
	rMsgBuf.CopyUtf8FromUnicode(last_err_text, last_err_text_len, 1);
	return last_err;
}

SString & SLAPI ChZnInterface::MakeHeaderFields(const char * pToken, uint flags, StrStrAssocArray * pHdrFlds, SString & rBuf)
{
	StrStrAssocArray hdr_flds;
	SETIFZ(pHdrFlds, &hdr_flds);
	if(!(flags & mhffTokenOnly)) {
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrCacheControl, "no-cache");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAcceptLang, "ru");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAccept, "application/json");
	}
	if(!isempty(pToken)) {
		SString temp_buf;
		if(flags & mhffAuthBearer)
			(temp_buf = "Bearer").Space().Cat(pToken);
		else
			(temp_buf = "token").Space().Cat(pToken);
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAuthorization, temp_buf);
	}
	SHttpProtocol::PutHeaderFieldsIntoString(*pHdrFlds, rBuf);
	return rBuf;
}

int SLAPI ChZnInterface::GetDocumentTicket(const InitBlock & rIb, const char * pDocIdent, SString & rTicket)
{
	rTicket.Z();
	int    ok = -1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString reply_buf;
	SString hdr_buf;
	SString link_buf;
	InetUrl url(MakeTargetUrl_(qGetTicket, pDocIdent, rIb, url_buf));
	{
		ScURL c;
		StrStrAssocArray hdr_flds;
		MakeHeaderFields(rIb.Token, mhffTokenOnly, &hdr_flds, hdr_buf);
		if(rIb.ProtocolId == rIb.protidMdlp) {
			int    wininet_err = 0;
			WinInternetHandleStack hstk;
			HINTERNET h_inet_sess = 0;
			HINTERNET h_connection = 0;
			HINTERNET h_req = 0;
			THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_PRECONFIG, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
			THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
			{
				THROW(h_req = hstk.PushHttpRequestGet(h_connection, url));
				LogTalking("req", url_buf, temp_buf.Z());
				if(HttpSendRequest(h_req, SUcSwitch(hdr_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
					SString wi_msg;
					uint  wi_code = GetLastWinInternetResponse(wi_msg);
					ReadReply(h_req, reply_buf);
					LogTalking("rep", 0, reply_buf);
					if(ReadJsonReplyForSingleItem(reply_buf, "link", link_buf) > 0) {
						url.Parse(link_buf);
						Document reply_doc;
						GetDocument(rIb, 0, &url, reply_doc);
						rTicket = reply_doc.Content;
						ok = 1;
					}
				}
				else {
					wininet_err = GetLastError();
				}
			}
		}
		else {
			SBuffer ack_buf;
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			LogTalking("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					LogTalking("rep", 0, reply_buf);
					if(ReadJsonReplyForSingleItem(reply_buf, "link", link_buf) > 0) {
						wr_stream.Open(ack_buf.Z(), SFile::mWrite);
						url.Parse(link_buf);
						LogTalking("req", link_buf, temp_buf.Z());
						THROW_SL(c.HttpGet(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, &wr_stream));
						p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							rTicket.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							LogTalking("rep", 0, rTicket);
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::GetPendingIdentList(const InitBlock & rIb, StringSet & rResult)
{
	int    ok = -1;
	SString path;
	THROW(GetDebugPath(rIb, path));
	path.SetLastSlash().Cat("pending").SetLastSlash().Cat("*");
	{
		SDirEntry de;
		for(SDirec sd(path, 0); sd.Next(&de) > 0;) {
			if(de.IsFile()) {
				rResult.add(de.FileName);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::GetDebugPath(const InitBlock & rIb, SString & rPath)
{
	rPath.Z();
    int    ok = 1;
    SString temp_path;
    PPGetPath(PPPATH_TEMP, temp_path);
    temp_path.SetLastSlash().Cat("CHZN").CatChar('-').Cat(rIb.CliIdent);
    THROW_SL(::createDir(temp_path));
	rPath = temp_path;
    CATCHZOK
    return ok;
}

int SLAPI ChZnInterface::GetTemporaryFileName(const char * pPath, const char * pSubPath, const char * pPrefix, SString & rFn)
{
	int    ok = 1;
	rFn.Z();
	SString temp_path;
	if(!isempty(pPath))
		temp_path = pPath;
	else
		PPGetPath(PPPATH_TEMP, temp_path);
	if(!isempty(pSubPath))
		temp_path.SetLastSlash().Cat(pSubPath);
	temp_path.RmvLastSlash();
	THROW_SL(::createDir(temp_path));
	MakeTempFileName(temp_path.SetLastSlash(), pPrefix, "xml", 0, rFn);
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::CreatePendingFile(const char * pPath, const char * pIdent)
{
	int    ok = 1;
	SString temp_path;
	if(!isempty(pPath))
		temp_path = pPath;
	else
		PPGetPath(PPPATH_TEMP, temp_path);
	temp_path.SetLastSlash().Cat("pending");
	THROW_SL(::createDir(temp_path));
	{
		temp_path.SetLastSlash().Cat(pIdent);
		SFile f(temp_path, SFile::mWrite|SFile::mBinary);
		THROW_SL(f.IsValid());
	}
	//MakeTempFileName(temp_path.SetLastSlash(), pPrefix, "xml", 0, rFn);
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::ParseTicket(const char * pTicket, Packet ** ppP)
{
	ASSIGN_PTR(ppP, 0);
	int    ok = -1;
	SString temp_buf;
	Packet * p_pack = 0;
    xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	if(!isempty(pTicket)) {
		xmlNode * p_root = 0;
		THROW(p_ctx = xmlNewParserCtxt());
		THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, pTicket, sstrlen(pTicket), 0, 0, XML_PARSE_NOENT), p_ctx);
		THROW(p_root = xmlDocGetRootElement(p_doc));
		if(SXml::IsName(p_root, "documents")) {
			for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
				if(SXml::IsName(p_c, "result")) {
					THROW(p_pack = new Packet(doctypResult));
					ok = 1;
					Packet::OperationResult * p_opr = static_cast<Packet::OperationResult *>(p_pack->P_Data);
					for(const xmlNode * p_c2 = p_c->children; p_c2; p_c2 = p_c2->next) {
						if(SXml::GetContentByName(p_c2, "operation", temp_buf)) {
							p_opr->Op = temp_buf.ToLong();
						}
						else if(SXml::GetContentByName(p_c2, "operation_id", temp_buf)) {
							p_opr->OpIdent = temp_buf;
						}
						else if(SXml::GetContentByName(p_c2, "operation_result", temp_buf)) {
							if(temp_buf.IsEqiAscii("Rejected"))
								p_opr->Status = -1;
							else if(temp_buf.IsEqiAscii("Accepted"))
								p_opr->Status = 1;
						}
						else if(SXml::GetContentByName(p_c2, "operation_comment", temp_buf)) {
							p_opr->OpComment = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						else if(SXml::IsName(p_c2, "errors")) {
							Packet::ErrorItem * p_err_item = p_opr->Errors.CreateNewItem();
							THROW_SL(p_err_item);
							for(const xmlNode * p_c3 = p_c2->children; p_c3; p_c3 = p_c3->next) {
								if(SXml::GetContentByName(p_c3, "error_code", temp_buf))
									p_err_item->Code = temp_buf.ToLong();
								else if(SXml::GetContentByName(p_c3, "error_desc", temp_buf))
									p_err_item->Descr = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								else if(SXml::GetContentByName(p_c3, "object_id", temp_buf))
									p_err_item->Ident = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(ppP, p_pack);
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int SLAPI ChZnInterface::CommitTicket(const char * pPath, const char * pIdent, const char * pTicket)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString temp_path;
	SString pending_path;
	SString ticket_path;
	Packet * p_result_pack = 0;
	if(ParseTicket(pTicket, &p_result_pack) > 0) {
		const Packet::OperationResult * p_opr = static_cast<Packet::OperationResult *>(p_result_pack->P_Data);
		if(p_opr) {
			PPIDArray bill_id_list;
			p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, pIdent, &bill_id_list);
			for(uint i = 0; i < bill_id_list.getCount(); i++) {
				const PPID bill_id = bill_id_list.get(i);
				if(p_opr->Status < 0) {
					int   do_update_memos = 0;
					SString memo_msg;
					SString memos;
					StringSet ss_memo(_PPConst.P_ObjMemoDelim);
					StringSet ss_memo_new(_PPConst.P_ObjMemoDelim);
					p_ref->GetPropVlrString(PPOBJ_BILL, bill_id, PPPRP_BILLMEMO, memos);
					ss_memo.setBuf(memos);
					for(uint ssp = 0; ss_memo.get(&ssp, temp_buf);) {
						if(!temp_buf.NotEmptyS())
							do_update_memos = 1;
						else {
							if(temp_buf.Search(_PPConst.P_ObjMemo_ChznRejPfx, 0, 1, 0)) {
								temp_buf.Z();
								do_update_memos = 1;
								break;
							}
							if(temp_buf.NotEmptyS())
								ss_memo_new.add(temp_buf);
						}
					}
					{
						if(p_opr->OpComment.NotEmpty()) {
							memo_msg.Z().Space().Cat(_PPConst.P_ObjMemo_ChznRejPfx).CatDiv(':', 2).Cat(p_opr->OpComment);
							ss_memo_new.add(memo_msg);
							do_update_memos = 1;
						}
						if(p_opr->Errors.getCount()) {
							for(uint eridx = 0; eridx < p_opr->Errors.getCount(); eridx++) {
								const ChZnInterface::Packet::ErrorItem * p_er = p_opr->Errors.at(eridx);
								if(p_er) {
									memo_msg.Z().Space().Cat(_PPConst.P_ObjMemo_ChznRejPfx).CatDiv(':', 2).Cat(p_er->Code).Space().Cat(p_er->Ident).Space().Cat(p_er->Descr);
									ss_memo_new.add(memo_msg);
									do_update_memos = 1;
								}
							}
						}
					}
					{
						PPTransaction tra(1);
						THROW(tra);
						if(do_update_memos) {
							memos = ss_memo_new.getBuf();
							PutObjMemos(PPOBJ_BILL, PPPRP_BILLMEMO, bill_id, memos, 0);
						}
						THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIIDENT, 0/*ta*/));
						THROW(tra.Commit());
					}
				}
			}
		}
	}
	if(!isempty(pPath))
		temp_path = pPath;
	else
		PPGetPath(PPPATH_TEMP, temp_path);
	(pending_path = temp_path).SetLastSlash().Cat("pending");
	(ticket_path = temp_path).SetLastSlash().Cat("ticket");
	THROW_SL(::createDir(pending_path));
	THROW_SL(::createDir(ticket_path));
	{
		ticket_path.SetLastSlash().Cat(pIdent);
		SFile f(ticket_path, SFile::mWrite);
		THROW_SL(f.IsValid());
		THROW_SL(f.Write(pTicket, sstrlen(pTicket)));
	}
	{
		pending_path.SetLastSlash().Cat(pIdent);
		SFile::Remove(pending_path);
	}
	CATCHZOK
	delete p_result_pack;
	return ok;
}

int SLAPI ChZnInterface::TransmitDocument2(const InitBlock & rIb, const ChZnInterface::Packet & rPack, SString & rReply)
{
	rReply.Z();
	int    ok = -1;
	SString data_buf;
	xmlTextWriter * p_writer = 0;
	ChZnInterface::Document czd;
	xmlBuffer * p_xml_buf = xmlBufferCreate();
	if(p_xml_buf) {
		p_writer = xmlNewTextWriterMemory(p_xml_buf, 0);
		if(p_writer) {
			SXml::WDoc _doc(p_writer, cpUTF8);
			if(czd.Make(_doc, rIb, &rPack)) {
				xmlTextWriterFlush(p_writer);
				data_buf.CopyFromN(reinterpret_cast<const char *>(p_xml_buf->content), p_xml_buf->use);
				data_buf.ReplaceStr("\n", "", 0).ReplaceStr("\xD\xA", "", 0);
				{
					SString temp_buf;
					SString url_buf;
					SString req_buf;
					SString reply_buf;
					SString sign;
					SString data_base64_buf;
					S_GUID req_id;
					StrStrAssocArray hdr_flds;
					InetUrl url(MakeTargetUrl_(qDocumentSend, 0, rIb, url_buf));
					{
						//temp_buf.Z().CatN(static_cast<const char *>(pData), dataLen);
						LogTalking("req-rawdoc", 0, data_buf);
					}
					THROW(MakeDocumentRequest(rIb, data_buf.cptr(), data_buf.Len(), req_id, req_buf));
					LogTalking("req", url_buf, req_buf);
					if(rIb.ProtocolId == rIb.protidMdlp) {
						int    wininet_err = 0;
						WinInternetHandleStack hstk;
						HINTERNET h_inet_sess = 0;
						HINTERNET h_connection = 0;
						HINTERNET h_req = 0;
						THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_PRECONFIG, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
						THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
						{
							THROW(h_req = hstk.PushHttpRequestPost(h_connection, url));
							MakeHeaderFields(rIb.Token, 0, &hdr_flds, temp_buf);
							if(HttpSendRequest(h_req, SUcSwitch(temp_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
								SString wi_msg;
								uint  wi_code = GetLastWinInternetResponse(wi_msg);
								ReadReply(h_req, reply_buf);
							}
							else {
								wininet_err = GetLastError();
								SLS.SetError(SLERR_WINDOWS);
								PPSetErrorSLib();
							}
						}
					}
					else {
						ScURL c;
						MakeHeaderFields(rIb.Token, 0, &hdr_flds, temp_buf);
						{
							SBuffer ack_buf;
							SFile wr_stream(ack_buf.Z(), SFile::mWrite);
							THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
							{
								SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
								if(p_ack_buf)
									reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							}
						}
					}
					LogTalking("rep", 0, reply_buf);
					if(reply_buf.NotEmpty() && ReadJsonReplyForSingleItem(reply_buf, "document_id", rReply) > 0) {
						GetDebugPath(rIb, temp_buf);
						THROW(CreatePendingFile(temp_buf, rReply))
						ok = 1;
					}
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	p_writer = 0;
	xmlBufferFree(p_xml_buf);
	p_xml_buf = 0;
	return ok;
}

uint SLAPI ChZnInterface::ReadReply(HINTERNET hReq, SString & rBuf)
{
	rBuf.Z();
	DWORD  read_bytes = 0;
	SBuffer ret_data_buf;
	do {
		uint8 buf[1024];
		InternetReadFile(hReq, buf, sizeof(buf), &read_bytes);
		if(read_bytes) {
			ret_data_buf.Write(buf, read_bytes);
		}
	} while(read_bytes > 0);
	rBuf.CatN(ret_data_buf.GetBufC(), ret_data_buf.GetAvailableSize());
	return read_bytes;
}

int SLAPI ChZnInterface::GetUserInfo2(InitBlock & rIb)
{
	int    ok = -1;
	int    wininet_err = 0;
	int    win_err = 0;
	WinInternetHandleStack hstk;
	HINTERNET h_inet_sess = 0;
	HINTERNET h_connection = 0;
	HINTERNET h_req = 0;
	SString temp_buf;
	SString req_buf;
	InetUrl url(MakeTargetUrl_(qCurrentUserInfo, 0, rIb, temp_buf));
	THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_DIRECT, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
	THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
	{
		THROW(h_req = hstk.PushHttpRequestGet(h_connection, url));
		int  isor = 0;
		uint iresp = 0;
		{
			const CERT_CONTEXT * p_cert = GetClientSslCertificate(rIb);
			if(p_cert) {
				isor = InternetSetOption(h_req, INTERNET_OPTION_CLIENT_CERT_CONTEXT, (LPVOID)(p_cert), sizeof(*p_cert));
				if(!isor) {
					win_err = GetLastError();
					iresp = GetLastWinInternetResponse(temp_buf);
				}
			}
		}
		//
		MakeHeaderFields(rIb.Token, 0, 0, temp_buf);
		if(HttpSendRequest(h_req, SUcSwitch(temp_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
			SString wi_msg;
			uint  wi_code = GetLastWinInternetResponse(wi_msg);
			ReadReply(h_req, temp_buf);
		}
		else {
			wininet_err = GetLastError();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::GetToken2(const char * pAuthCode, InitBlock & rIb)
{
	int    ok = -1;
	int    wininet_err = 0;
	WinInternetHandleStack hstk;
	HINTERNET h_inet_sess = 0;
	HINTERNET h_connection = 0;
	HINTERNET h_req = 0;
	SString temp_buf;
	SString req_buf;
	InetUrl url(MakeTargetUrl_(qToken, 0, rIb, temp_buf));
	THROW(MakeTokenRequest(rIb, pAuthCode, req_buf));
	THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_PRECONFIG, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
	THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
	{
		THROW(h_req = hstk.PushHttpRequestPost(h_connection, url));
		MakeHeaderFields(0, 0, 0, temp_buf);
		if(HttpSendRequest(h_req, SUcSwitch(temp_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
			SString wi_msg;
			uint  wi_code = GetLastWinInternetResponse(wi_msg);
			ReadReply(h_req, temp_buf);
			if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0) {
				ok = 1;
			}
		}
		else {
			wininet_err = GetLastError();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ChZnInterface::ReadJsonReplyForSingleItem(const char * pReply, const char * pTarget, SString & rResult)
{
	rResult.Z();
	int    ok = -1;
	json_t * p_json_doc = 0;
	if(json_parse_document(&p_json_doc, pReply) == JSON_OK) {
		SString temp_buf;
		json_t * p_next = 0;
		for(json_t * p_cur = p_json_doc; rResult.Empty() && p_cur; p_cur = p_next) {
			p_next = p_cur->P_Next;
			switch(p_cur->Type) {
				case json_t::tOBJECT: p_next = p_cur->P_Child; break;
				case json_t::tSTRING:
					if(p_cur->P_Child) {
						if(sstreqi_ascii(p_cur->Text, pTarget)) {
							rResult = (temp_buf = p_cur->P_Child->Text).Unescape();
							ok = 1;
						}
					}
					break;
			}
		}
	}
	delete p_json_doc;
	return ok;
}

int SLAPI ChZnInterface::GetDocument(const InitBlock & rIb, const S_GUID * pUuid, const InetUrl * pUrl, Document & rDoc)
{
	int    ok = -1;
	json_t * p_json_req = 0;
	json_t * p_json_doc = 0;
	SString temp_buf;
	SString hdr_buf;
	SString req_buf;
	SString reply_buf;
	SString uuid_buf;
	SBuffer ack_buf;
	InetUrl url;
	if(pUrl) {
	}
	else if(pUuid && !!*pUuid) {
		uuid_buf.Z().Cat(*pUuid, S_GUID::fmtLower|S_GUID::fmtIDL);
		if(url.Parse(MakeTargetUrl_(qGetDoc, uuid_buf.cptr(), rIb, temp_buf)))
			pUrl = &url;
	}
	if(pUrl) {
		//uuid_buf.Z().Cat(rUuid, S_GUID::fmtLower|S_GUID::fmtIDL);
		//InetUrl url(MakeTargetUrl_(qGetDoc, uuid_buf.cptr(), rIb, temp_buf));
		StrStrAssocArray hdr_flds;
		if(rIb.ProtocolId == InitBlock::protidGisMt) {
			MakeHeaderFields(rIb.Token, mhffAuthBearer, &hdr_flds, hdr_buf);
			ScURL c;
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			LogTalking("req", 0, req_buf);
			THROW_SL(c.HttpGet(*pUrl, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					LogTalking("rep", 0, reply_buf);
					if(json_parse_document(&p_json_doc, reply_buf) == JSON_OK) {
						if(ParseDocument(p_json_doc, rDoc) > 0)
							ok = 1;
					}
				}
			}
		}
		else if(rIb.ProtocolId == InitBlock::protidMdlp) {
			MakeHeaderFields(rIb.Token, mhffTokenOnly, &hdr_flds, hdr_buf);
			int    wininet_err = 0;
			WinInternetHandleStack hstk;
			HINTERNET h_inet_sess = 0;
			HINTERNET h_connection = 0;
			HINTERNET h_req = 0;
			THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_PRECONFIG, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
			THROW(h_connection = hstk.PushConnection(*pUrl, h_inet_sess));
			{
				THROW(h_req = hstk.PushHttpRequestGet(h_connection, *pUrl));
				LogTalking("req", 0, req_buf);
				if(HttpSendRequest(h_req, SUcSwitch(hdr_buf), -1, const_cast<char *>(req_buf.cptr()), req_buf.Len())) {
					SString wi_msg;
					uint  wi_code = GetLastWinInternetResponse(wi_msg);
					ReadReply(h_req, reply_buf);
					LogTalking("rep", 0, reply_buf);
					rDoc.Content = reply_buf;
				}
				else {
					wininet_err = GetLastError();
				}
			}
		}
	}
	CATCHZOK
	delete p_json_doc;
	return ok;
}

int SLAPI ChZnInterface::GetDocumentList(InitBlock & rIb, const DocumentFilt * pFilt, TSCollection <Document> & rList)
{
	int    ok = -1;
	json_t * p_json_req = 0;
	SString temp_buf;
	SString req_buf;
	SString url_buf;
	SBuffer ack_buf;
	InetUrl url(MakeTargetUrl_(qGetDocList, 0, rIb, url_buf));
	{
		req_buf.Z();
		{
			// { "filter": { "doc_status": "PROCESSED_DOCUMENT" }, "start_from": 0, "count": 100 }
			p_json_req = new json_t(json_t::tOBJECT);
			{
				json_t * p_json_filt = new json_t(json_t::tOBJECT);
				p_json_filt->InsertString("doc_status", "PROCESSED_DOCUMENT");
				p_json_req->Insert("filter", p_json_filt);
			}
			p_json_req->Insert("start_from", json_new_number("0"));
			p_json_req->Insert("count", json_new_number("100"));
			THROW_SL(json_tree_to_string(p_json_req, req_buf));
		}
		ZDELETE(p_json_req);
	}
	{
		ScURL c;
		StrStrAssocArray hdr_flds;
		if(rIb.ProtocolId == InitBlock::protidGisMt || oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
			if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidEdoLtMdlp)) {
				long   doc_fold_id = 0;
				long   count_offset = 0;
				long   count_limit = 100;
				int64  created_from = 0; // time_t
				int64  created_to = 0; // time_t
				if(pFilt) {
					//url.
					switch(pFilt->Folder) {
						case docfoldDocs: doc_fold_id = 0; break;
						case docfoldArc: doc_fold_id = 1; break;
						case docfoldBin: doc_fold_id = 2; break;
						case docfoldToSign: doc_fold_id = 3; break;
						case docfoldRejected: doc_fold_id = 4; break;
						default: doc_fold_id = 0; break;
					}
					if(pFilt->CountLimit > 0)
						count_limit = pFilt->CountLimit;
					count_offset = pFilt->CountOffset;
				}
				created_from = (pFilt && pFilt->Period.low) ? pFilt->Period.low.GetTimeT() : encodedate(1, 1, 2020).GetTimeT();
				created_to = (pFilt && pFilt->Period.upp) ? pFilt->Period.upp.GetTimeT() : getcurdate_().GetTimeT();
				temp_buf.Z();
				temp_buf.CatEq("limit", count_limit);
				temp_buf.CatChar('&').CatEq("offset", count_offset);
				temp_buf.CatChar('&').CatEq("folder", doc_fold_id);
				temp_buf.CatChar('&').CatEq("created_from", created_from);
				temp_buf.CatChar('&').CatEq("created_to", created_to);
				//temp_buf.CatChar('&').CatEq("partner_inn", "5003052454"); // @debug
				url.SetComponent(InetUrl::cQuery, temp_buf);
			}
			MakeHeaderFields(rIb.Token, mhffAuthBearer, &hdr_flds, temp_buf);
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			//PPGetFilePath(PPPATH_BIN, "cacerts-mcs.pem", temp_buf);
			//THROW_SL(c.SetupDefaultSslOptions(temp_buf, SSystem::sslTLS_v10, 0)); //CURLOPT_SSLVERSION значением CURL_SSLVERSION_TLSv1_0
			LogTalking("req", url_buf, req_buf);
			THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					LogTalking("rep", 0, temp_buf);
					ParseDocumentList(temp_buf, rList);
				}
			}
		}
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int SLAPI ChZnInterface::GetIncomeDocList2_temp(InitBlock & rIb)
{
	int    ok = -1;
	int    wininet_err = 0;
	int    win_err = 0;
	json_t * p_json_req = 0;
	WinInternetHandleStack hstk;
	HINTERNET h_inet_sess = 0;
	HINTERNET h_connection = 0;
	HINTERNET h_req = 0;
	SString temp_buf;
	SString req_buf;
	InetUrl url(MakeTargetUrl_(qGetDocList, 0, rIb, temp_buf));
	{
		req_buf.Z();
		{
			// { "filter": { "doc_status": "PROCESSED_DOCUMENT" }, "start_from": 0, "count": 100 }
			p_json_req = new json_t(json_t::tOBJECT);
			{
				json_t * p_json_filt = new json_t(json_t::tOBJECT);
				p_json_filt->InsertString("doc_status", "PROCESSED_DOCUMENT");
				p_json_req->Insert("filter", p_json_filt);
			}
			p_json_req->Insert("start_from", json_new_number("0"));
			p_json_req->Insert("count", json_new_number("100"));
			THROW_SL(json_tree_to_string(p_json_req, req_buf));
		}
		ZDELETE(p_json_req);
	}
	THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_DIRECT, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
	THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
	{
		THROW(h_req = hstk.PushHttpRequestPost(h_connection, url));
		//
		int  isor = 0;
		uint iresp = 0;
		{
			const CERT_CONTEXT * p_cert = GetClientSslCertificate(rIb);
			if(p_cert) {
				isor = InternetSetOption(h_req, INTERNET_OPTION_CLIENT_CERT_CONTEXT, (LPVOID)(p_cert), sizeof(*p_cert));
				if(!isor) {
					win_err = GetLastError();
					iresp = GetLastWinInternetResponse(temp_buf);
				}
			}
		}
		//
		MakeHeaderFields(rIb.Token, 0, 0, temp_buf);
		if(HttpSendRequest(h_req, SUcSwitch(temp_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
			SString wi_msg;
			uint  wi_code = GetLastWinInternetResponse(wi_msg);
			ReadReply(h_req, temp_buf);
		}
		else {
			wininet_err = GetLastError();
		}
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

#if 0 // @v10.8.0 {
int SLAPI ChZnInterface::Connect2(InitBlock & rIb)
{
	int    ok = -1;
	int    wininet_err = 0;
	WinInternetHandleStack hstk;
	HINTERNET h_inet_sess = 0;
	HINTERNET h_connection = 0;
	HINTERNET h_req = 0;
	SString temp_buf;
	SString result_code;
	SString req_buf;
	InetUrl url(MakeTargetUrl_(qAuth, 0, rIb, temp_buf));
	THROW(MakeAuthRequest(rIb, req_buf));
	THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), INTERNET_OPEN_TYPE_PRECONFIG, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
	THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
	{
		const TCHAR * p_types[] = { _T("text/*"), _T("application/json"), 0 };
		THROW(h_req = hstk.PushHttpRequestPost(h_connection, url, p_types));
		MakeHeaderFields(0, 0, 0, temp_buf);
		if(HttpSendRequest(h_req, SUcSwitch(temp_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
			SString wi_msg;
			uint  wi_code = GetLastWinInternetResponse(wi_msg);
			ReadReply(h_req, temp_buf);
			if(ReadJsonReplyForSingleItem(temp_buf, "code", result_code) > 0) {
				ok = 1;
			}
		}
		else {
			wininet_err = GetLastError();
		}
	}
	if(ok > 0) {
		hstk.Destroy();
		THROW(GetToken2(result_code, rIb));
	}
	CATCHZOK
	return ok;
}
#endif // } 0 @v10.8.0

int SLAPI ChZnInterface::LogTalking(const char * pPrefix, const char * pTargetUrl, const SString & rMsg)
{
	int    ok = 1;
	SString file_name;
	PPGetFilePath(PPPATH_LOG, PPFILNAM_CHZNTALK_LOG, file_name);
	SFile  f_log(file_name, SFile::mAppend);
	if(f_log.IsValid()) {
		if(!isempty(pPrefix)) {
			SString temp_buf;
			temp_buf.Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS|TIMF_MSEC).Space().Cat(pPrefix).CatDiv(':', 2);
			if(!isempty(pTargetUrl)) {
				temp_buf.Space().CatEq("url", pTargetUrl);
			}
			f_log.WriteLine(temp_buf.CR());
		}
		if(rMsg.NotEmpty())
			f_log.WriteLine(rMsg);
		f_log.WriteLine(0);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI ChZnInterface::Connect(InitBlock & rIb)
{
	int    ok = -1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString result_code;
	SBuffer ack_buf;
	if(rIb.ProtocolId == InitBlock::protidMdlp) {
		{
			InetUrl url(MakeTargetUrl_(qAuth, 0, rIb, url_buf));
			THROW(MakeAuthRequest(rIb, req_buf));
			{
				ScURL c;
				StrStrAssocArray hdr_flds;
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					LogTalking("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							LogTalking("rep", 0, temp_buf);
							if(ReadJsonReplyForSingleItem(temp_buf, "code", result_code) > 0)
								ok = 1;
						}
					}
				}
			}
		}
		if(ok > 0) {
			ok = -1;
			InetUrl url(MakeTargetUrl_(qToken, 0, rIb, url_buf));
			THROW(MakeTokenRequest(rIb, result_code, req_buf));
			{
				ScURL c;
				StrStrAssocArray hdr_flds;
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					LogTalking("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							LogTalking("rep", 0, temp_buf);
							if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
								ok = 1;
						}
					}
				}
			}
		}
	}
	else if(oneof3(rIb.ProtocolId, InitBlock::protidEdoLtElk, InitBlock::protidEdoLtInt, InitBlock::protidGisMt)) {
		S_GUID result_uuid;
		SString result_data;
		{
			InetUrl url(MakeTargetUrl_(qAuth, 0, rIb, url_buf));
			{
				ScURL c;
				StrStrAssocArray hdr_flds;
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					LogTalking("req", url_buf, req_buf);
					THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							LogTalking("rep", 0, temp_buf);
							{
								json_t * p_json_doc = 0;
								if(json_parse_document(&p_json_doc, temp_buf) == JSON_OK) {
									SString temp_buf;
									json_t * p_next = 0;
									for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_next) {
										p_next = p_cur->P_Next;
										switch(p_cur->Type) {
											case json_t::tOBJECT: p_next = p_cur->P_Child; break;
											case json_t::tSTRING:
												if(p_cur->P_Child) {
													if(sstreqi_ascii(p_cur->Text, "uuid"))
														result_uuid.FromStr((temp_buf = p_cur->P_Child->Text).Unescape());
													else if(sstreqi_ascii(p_cur->Text, "data"))
														(result_data = p_cur->P_Child->Text).Unescape();
												}
												break;
										}
									}
								}
								ZDELETE(p_json_doc);
							}
						}
					}
				}
			}
		}
		if(!!result_uuid && result_data.NotEmpty()) {
			req_buf.Z();
			SString signed_data;
			THROW(GetSign(rIb, result_data.cptr(), result_data.Len(), signed_data));
			{
				json_t * p_json_req = new json_t(json_t::tOBJECT);
				temp_buf.Z().Cat(result_uuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				p_json_req->InsertString("uuid", temp_buf);
				p_json_req->InsertString("data", signed_data);
				THROW_SL(json_tree_to_string(p_json_req, req_buf));
				ZDELETE(p_json_req);
			}
			{
				InetUrl url(MakeTargetUrl_(qToken, 0, rIb, url_buf));
				ScURL c;
				StrStrAssocArray hdr_flds;
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, "application/json");
				//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAuthorization, temp_buf.Z().Cat("Bearer").Space().Cat(""));
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					LogTalking("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							LogTalking("rep", 0, temp_buf);
							if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
								ok = 1;
						}
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
SLAPI PPChZnPrcssr::Param::Param() : GuaID(0), LocID(0)
{
	Period.Z();
}

SLAPI  PPChZnPrcssr::QueryParam::QueryParam() : DocType(0), Flags(0), GuaID(0), LocID(0)
{
}

SLAPI PPChZnPrcssr::PPChZnPrcssr(PPLogger * pOuterLogger) : PPEmbeddedLogger(0, pOuterLogger, PPFILNAM_CHZN_LOG, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER), P_Ib(new ChZnInterface::InitBlock)
{
}

SLAPI PPChZnPrcssr::~PPChZnPrcssr()
{
	delete static_cast<ChZnInterface::InitBlock *>(P_Ib);
}

int SLAPI PPChZnPrcssr::EditParam(Param * pParam)
{
	class ChZnPrcssrParamDialog : public TDialog {
		DECL_DIALOG_DATA(PPChZnPrcssr::Param);
	public:
		ChZnPrcssrParamDialog() : TDialog(DLG_CHZNIX)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_CHZNIX_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_CHZNIX_GUA, &Data.GuaID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(ChZnPrcssrParamDialog, pParam);
}

int SLAPI PPChZnPrcssr::EditQueryParam(PPChZnPrcssr::QueryParam * pData)
{
	class EditChZnQueryParamDialog : public TDialog {
		DECL_DIALOG_DATA(PPChZnPrcssr::QueryParam);
	public:
		EditChZnQueryParamDialog() : TDialog(DLG_CHZNIX)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_CHZNIX_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, 0);
			AddClusterAssocDef(CTL_CHZNIX_WHAT, 0, Data._afQueryTicket);
			AddClusterAssoc(CTL_CHZNIX_WHAT, 1, Data._afQueryKizInfo);
			SetClusterData(CTL_CHZNIX_WHAT, Data.DocType);
			setCtrlString(CTL_CHZNIX_PARAM, Data.ParamString);
			SetupArCombo(this, CTLSEL_CHZNIX_SUPPL, Data.ArID, 0, GetSupplAccSheet(), 0);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_CHZNIX_GUA, &Data.GuaID);
			GetClusterData(CTL_CHZNIX_WHAT, &Data.DocType);
			getCtrlString(CTL_CHZNIX_PARAM, Data.ParamString);
			getCtrlData(CTLSEL_CHZNIX_SUPPL, &Data.ArID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(EditChZnQueryParamDialog, pData);
}

int SLAPI PPChZnPrcssr::InteractiveQuery()
{
	int    ok = -1;
	SString temp_buf;
	SString result_buf;
	QueryParam _param;
	_param.LocID = LConfig.Location;
	while(EditQueryParam(&_param) > 0) {
		if(oneof2(_param.DocType, QueryParam::_afQueryTicket, QueryParam::_afQueryKizInfo)) {
			ChZnInterface ifc;
			ChZnInterface::InitBlock * p_ib = static_cast<ChZnInterface::InitBlock *>(P_Ib);
			p_ib->ProtocolId = ChZnInterface::InitBlock::protidMdlp;
			THROW(ifc.SetupInitBlock(_param.GuaID, 0, *p_ib));
			THROW(ifc.Connect(*p_ib) > 0);
			//SString doc_ident = "e8b6b8e2-6135-4153-804d-7a676cbfc0de";
			//ifc.GetDocumentTicket(*p_ib, doc_ident, temp_buf);
			//ifc.GetIncomeDocList_(*p_ib);
			if(_param.DocType == QueryParam::_afQueryTicket) {
				ifc.GetDocumentTicket(*p_ib, _param.ParamString, temp_buf);
			}
			else if(_param.DocType == QueryParam::_afQueryKizInfo) {
				ChZnInterface::Packet pack(ChZnInterface::doctypQueryKizInfo);
				ChZnInterface::Packet::QueryKizInfo * p_cq = static_cast<ChZnInterface::Packet::QueryKizInfo *>(pack.P_Data);
				p_cq->Code = _param.ParamString;
				p_cq->ArID = _param.ArID;
				if(!ifc.TransmitDocument2(*p_ib, pack, result_buf))
					LogLastError();
			}
		}
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPChZnPrcssr::Run(const Param & rP)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	Reference * p_ref = PPRef;
	ChZnInterface::Packet * p_pack = 0;
	SString temp_buf;
	SString result_doc_ident;
	SString edi_ident;
	PPIDArray base_op_list;
	PPIDArray op_list;
	StringSet pending_list;
	PrcssrAlcReport::Config alcr_cfg;
	PrcssrAlcReport::ReadConfig(&alcr_cfg);
	base_op_list.addnz(alcr_cfg.RcptOpID);
	base_op_list.addnz(alcr_cfg.IntrExpndOpID);
	TSCollection <ChZnInterface::Packet> pack_list;
	ChZnInterface ifc;
	ChZnInterface::InitBlock * p_ib = static_cast<ChZnInterface::InitBlock *>(P_Ib);
	p_ib->ProtocolId = ChZnInterface::InitBlock::protidMdlp;
	THROW(ifc.SetupInitBlock(rP.GuaID, 0, *p_ib));
	THROW(ifc.GetPendingIdentList(*p_ib, pending_list));
	if(PPObjOprKind::ExpandOpList(base_op_list, op_list) > 0) {
		PPLotExtCodeContainer::MarkSet lotxcode_set;
		PPLotExtCodeContainer::MarkSet::Entry msentry;
		for(uint opidx = 0; opidx < op_list.getCount(); opidx++) {
			const PPID op_id = op_list.get(opidx);
			PPOprKind op_rec;
			BillTbl::Rec bill_rec;
			GetOpData(op_id, &op_rec);
			for(DateIter di(&rP.Period); p_bobj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && p_bobj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIIDENT, edi_ident) <= 0) { // если тег установлен, то док уже был передан
						int    suited = 0;
						if(IsIntrExpndOp(bill_rec.OpID)) {
							const PPID dest_loc_id = bill_rec.Object ? PPObjLocation::ObjToWarehouse(bill_rec.Object) : 0;
							if(dest_loc_id && p_ref->Ot.GetTagStr(PPOBJ_LOCATION, dest_loc_id, PPTAG_LOC_CHZNCODE, temp_buf) > 0) {
								THROW_SL(p_pack = new ChZnInterface::Packet(ChZnInterface::doctypMovePlace));
								{
									PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_pack->P_Data);
									if(p_bobj->ExtractPacket(bill_rec.ID, p_bp) > 0) {
										for(uint tidx = 0; !suited && tidx < p_bp->GetTCount(); tidx++) {
											const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
											p_bp->XcL.Get(tidx+1, 0, lotxcode_set);
											for(uint j = 0; !suited && j < lotxcode_set.GetCount(); j++) {
												if(lotxcode_set.GetByIdx(j, msentry) /*&& !(msentry.Flags & PPLotExtCodeContainer::fBox)*/) {
													GtinStruc gts;
													const int pczcr = PPChZnPrcssr::ParseChZnCode(msentry.Num, gts, 0);
													if(pczcr > 0)
														suited = 1;
												}
											}
										}
									}
								}
							}
						}
						else {
							const PPID psn_id = bill_rec.Object ? ObjectToPerson(bill_rec.Object, 0) : 0;
							if(psn_id && p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_CHZNCODE, temp_buf) > 0) {
								THROW_SL(p_pack = new ChZnInterface::Packet(ChZnInterface::doctypReceiveOrder));
								{
									PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_pack->P_Data);
									if(p_bobj->ExtractPacket(bill_rec.ID, p_bp) > 0) {
										for(uint tidx = 0; !suited && tidx < p_bp->GetTCount(); tidx++) {
											const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
											p_bp->XcL.Get(tidx+1, 0, lotxcode_set);
											for(uint j = 0; !suited && j < lotxcode_set.GetCount(); j++) {
												if(lotxcode_set.GetByIdx(j, msentry) /*&& !(msentry.Flags & PPLotExtCodeContainer::fBox)*/) {
													GtinStruc gts;
													const int pczcr = PPChZnPrcssr::ParseChZnCode(msentry.Num, gts, 0);
													if(pczcr > 0)
														suited = 1;
												}
											}
										}
									}
								}
							}
						}
						if(suited) {
							pack_list.insert(p_pack);
							p_pack = 0; // to prevent destruction at the end of this function
						}
						else {
							ZDELETE(p_pack);
						}
					}
				}
			}
		}
	}
	if(pending_list.getCount() || pack_list.getCount()) {
		if(ifc.Connect(*p_ib) > 0) {
			{
				SString pending_ident;
				SString ticket_buf;
				SString path;
				PPIDArray bill_id_list;
				ifc.GetDebugPath(*p_ib, path);
				for(uint ssp = 0; pending_list.get(&ssp, pending_ident);) {
					if(ifc.GetDocumentTicket(*p_ib, pending_ident, ticket_buf) > 0) {
						if(!ifc.CommitTicket(path, pending_ident, ticket_buf))
							LogLastError();
					}
				}
			}
			for(uint bpidx = 0; bpidx < pack_list.getCount(); bpidx++) {
				ChZnInterface::Packet * p_inner_pack = pack_list.at(bpidx);
				int tdr = ifc.TransmitDocument2(*p_ib, *p_inner_pack, result_doc_ident);
				if(tdr > 0) {
					if(result_doc_ident.NotEmpty()) {
						PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_inner_pack->P_Data);
						ObjTagItem tag_item;
						if(tag_item.SetStr(PPTAG_BILL_EDIIDENT, result_doc_ident)) {
							// @v10.8.5 @construction p_bobj->P_Tbl->SetRecFlag2(p_bp->Rec.ID, BILLF2_ACKPENDING, 1, 1);
							if(!p_ref->Ot.PutTag(PPOBJ_BILL, p_bp->Rec.ID, &tag_item, 1))
								LogLastError();
						}
					}
				}
				else if(tdr == 0)
					LogLastError();
			}
		}
	}
	CATCH
		LogLastError();
		ok = 0;
	ENDCATCH
	ZDELETE(p_pack);
	return ok;
}

//static
int SLAPI PPChZnPrcssr::Test()
{
	int    ok = 1;
	SString temp_buf;
	PPChZnPrcssr prcssr(0);
	PPChZnPrcssr::Param param;
	TSCollection <ChZnInterface::Document> doc_list;
	ChZnInterface::Document single_doc;
	if(prcssr.EditParam(&param) > 0) {
		ChZnInterface ifc;
		ChZnInterface::InitBlock * p_ib = static_cast<ChZnInterface::InitBlock *>(prcssr.P_Ib);
		THROW(ifc.SetupInitBlock(param.GuaID, 0, *p_ib));
		//SETIFZ(p_ib->ProtocolId = ChZnInterface::InitBlock::protidGisMt;
		{
			//const CERT_CONTEXT * p_cert = ifc.GetClientSslCertificate(prcssr.Ib);
			THROW(ifc.Connect(*p_ib) > 0);
			if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp) {
				//ifc.GetUserInfo2(prcssr.Ib);
				//ifc.GetIncomeDocList2(*p_ib);
			}
			else if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidGisMt) {
				//SString doc_ident = "e8b6b8e2-6135-4153-804d-7a676cbfc0de";
				//ifc.GetDocumentTicket(*p_ib, doc_ident, temp_buf);
				ChZnInterface::DocumentFilt df;
				df.Flags |= df.fIncoming;
				df.Folder = ChZnInterface::docfoldDocs;
				ifc.GetDocumentList(*p_ib, &df, doc_list);
				for(uint i = 0; i < doc_list.getCount(); i++) {
					if(doc_list.at(i)) {
						const S_GUID doc_uuid = doc_list.at(i)->Uuid;
						ifc.GetDocument(*p_ib, &doc_uuid, 0, single_doc);
					}
				}
			}
			else if(oneof3(p_ib->ProtocolId, ChZnInterface::InitBlock::protidEdoLtElk, ChZnInterface::InitBlock::protidEdoLtInt, ChZnInterface::InitBlock::protidEdoLtMdlp)) {
				//SString doc_ident = "e8b6b8e2-6135-4153-804d-7a676cbfc0de";
				//ifc.GetDocumentTicket(*p_ib, doc_ident, temp_buf);
				ChZnInterface::DocumentFilt df;
				df.Flags |= df.fIncoming;
				df.Folder = ChZnInterface::docfoldDocs;
				ifc.GetDocumentList(*p_ib, &df, doc_list);
				for(uint i = 0; i < doc_list.getCount(); i++) {
					if(doc_list.at(i)) {
						const S_GUID doc_uuid = doc_list.at(i)->Uuid;
						ifc.GetDocument(*p_ib, &doc_uuid, 0, single_doc);
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}