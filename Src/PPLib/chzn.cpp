// CHZN.CPP
// Copyright (c) A.Sobolev 2019, 2020, 2021, 2022
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
/*

0104601669010315 21unXxjJoRse3rl 91EE0692ZIMgcIl2oboNlVDMuI9DYM3bLWSLQ20IZ0FWTTYug6M=
0104601669010315217m2CUceKxx41w91EE0692rUIIFK4Vds1rR0GLNvvo43aAM7CwwSh1twdFV8Y0heQ=

Оборудование по сериализации формирует специальные коды:
	AI 01 (GTIN) 14 симв. + идентификатор применения 2 симв. = 16 символов (обяз.)
	AI 21 (S/N) 13 симв.+ идентификатор применения 2 симв + FNC1 (29hASCII) 1 симв = 16 символов (обяз.)
	Регистратор эмиссии добавляет к ним специальный ключ и криптоподпись в формате:
		AI 91 (ключ проверки) 4 симв. + идентификатор применения 2 симв. + FNC1 (29h ASCII) 1 симв = 7 симв.
		AI 92 (криптоподпись) 44 симв. + идентификатор применения 2 симв. = 46 символов.
		Так же может использоваться завершающий FNC1, который добавляется только в случае наличия в средстве идентификации
		дополнительной информации, кроме КМ. Группы данных должны располагаться последовательно - от первой к четвертой. (в ред. Постановления Правительства РФ от 30.08.2019 N 1118)
*/
//
// 46 bytes
//
/*static*/int PPChZnPrcssr::Encode1162(int productType, const char * pGTIN, const char * pSerial, void * pResultBuf, size_t resultBufSize)
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

/*static*/int PPChZnPrcssr::ReconstructOriginalChZnCode(const GtinStruc & rS, SString & rBuf)
{
	int    ok = 0;
	SString temp_buf;
	rBuf.Z();
	if(rS.GetToken(GtinStruc::fldOriginalText, &temp_buf)) {
		/*if(temp_buf.Len() == 83)*/{
			// CHZN-0121GS91GS92
			SString _01;
			SString _21;
			SString _91;
			SString _92;
			if(rS.GetToken(GtinStruc::fldGTIN14, &_01) && rS.GetToken(GtinStruc::fldSerial, &_21) &&
				rS.GetToken(GtinStruc::fldUSPS, &_91) && rS.GetToken(GtinStruc::fldInner1, &_92)) {
				if(_01.Len() == 14 && _21.Len() == 13 && _91.Len() == 4/*&& _92.Len() == 44*/) {
					rBuf./*CatChar(232).*/Cat("01").Cat(_01).Cat("21").Cat(_21).CatChar('\x1D').Cat("91").Cat(_91).CatChar('\x1D').Cat("92").Cat(_92);
					ok = 2;
				}
			}
		}
		if(!ok) {
			rBuf = temp_buf;
			ok = 1;
		}
	}
	return ok;
}

/*static*/int PPChZnPrcssr::ParseChZnCode(const char * pCode, GtinStruc & rS, long flags)
{
	// Это, на самом деле, один из вариантов. Здесь текст приведен для того, чтобы придумать как
	// искусственно вставлять спецсимволы 
	//
	// Признак  символики  Data  Matrix – символ,  имеющий  код  «232» в  таблице 
	// символов ASCII. Признак символики Data Matrix является «невидимым» символом. 
	// Отображение при сканировании 2D сканером зависит от настроек сканера и средства 
	// просмотра информации; 
	// • Первая  группа  данных – глобальный  идентификационный  номер  торговой 
	// единицы,  состоящий  из  14  цифровых  символов,  которому  предшествует 
	// идентификатор применения (01); 
	// • Вторая группа данных  –  индивидуальный  серийный  номер  торговой  единицы, 
	// состоящий  из  13  символов  цифровой  или  буквенно-цифровой 
	// последовательности  (латинского  алфавита),  которому  предшествует 
	// идентификатор  применения (21).  Завершающим  символом  для  этой  группы 
	// данных является специальный символ-разделитель, имеющий код «29» в таблице 
	// символов ASCII (GS)  или символ «ФУНКЦИЯ 1»  (FNC1).  Символ-разделитель «GS» и символ «ФУНКЦИЯ 1» 
	// являются «невидимыми» символами. Отображение при сканировании 2D сканером зависит от настроек сканера и средства просмотра информации; 
	// • Третья  группа  данных  –  идентификатор  (индивидуальный  порядковый  номер) 
	// ключа проверки, предоставляемый эмитентам средств идентификации оператором 
	// системы мониторинга в составе кода проверки, состоящий из 4 символов  (цифр, 
	// строчных  и  прописных  букв  латинского  алфавита),  которому  предшествует 
	// идентификатор  применения (91).  Завершающим  символом  для  этой  группы данных является специальный символ-разделитель, 
	// имеющий код «29» в таблице символов ASCII  (GS) или символ «ФУНКЦИЯ 1»  (FNC1).  Символ-разделитель 7 «GS» и символ «ФУНКЦИЯ 1» 
	// являются «невидимыми» символами. Отображение при сканировании 2D  сканером зависит от настроек сканера и средства просмотра информации; 
	// • Четвертая группа данных – значение кода проверки, предоставляемое эмитентам 
	// средств идентификации оператором системы мониторинга в составе кода проверки, 
	// которому  предшествует  идентификатор  применения  (92),  и  состоящее  из  44 
	// символов  (цифр, строчных и прописных букв латинского алфавита, а также специальных символов).
	//
	// Назовем такую структуру CHZN-0121GS91GS92 без спецсимволов она имеет в точности 83 символа 
	// 01 [14] 21 [13] \x29 91 [4] \x29 92 [44]
	// example:
	// 01 04603182002518 21 0100007852382 91 EE06 92 uy1H5DQr89ewuV4W/ssuZKTxmcX7r0A8/1KZU3tMLSY=
	// 
	int    ok = 0;
	rS.Z();
	rS.AddSpecialStopChar(0x1D); // @v10.9.9
	rS.AddSpecialStopChar(0xE8); // @v10.9.9
	rS.AddOnlyToken(GtinStruc::fldGTIN14);
	rS.AddOnlyToken(GtinStruc::fldSerial);
	rS.SetSpecialFixedToken(GtinStruc::fldSerial, 13);
	rS.AddOnlyToken(GtinStruc::fldPart);
	rS.AddOnlyToken(GtinStruc::fldAddendumId);
	rS.AddOnlyToken(GtinStruc::fldUSPS); //
	rS.SetSpecialFixedToken(GtinStruc::fldUSPS, 4); // @v10.9.10
	rS.AddOnlyToken(GtinStruc::fldInner1);
	rS.SetSpecialFixedToken(GtinStruc::fldInner1, 1000/*UNTIL EOL*/); // @v10.9.0
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
			temp_buf.ShiftLeftChr('\xE8'); // @v10.9.9 Специальный символ. Может присутствовать в начале кода 
			// "]C1"
			// @v11.0.1 {
			if(temp_buf.HasPrefixIAscii("]C1")) { // Выяснилось, что и такие служебные префиксы встречаются //
				temp_buf.ShiftLeft(3);
				temp_buf.ShiftLeftChr('\xE8'); // Черт его знает: на всякий случай снова проверим этого обдолбыша
			}
			// } @v11.0.1 
			// @v11.2.5 {
			TranslateLocaleKeyboardTextToLatin(temp_buf, raw_buf);
			pCode = raw_buf;
			// } @v11.2.5 
			/* @v11.2.5 if(!temp_buf.IsAscii()) {
				// Попытка транслировать латинский символ из локальной раскладки клавиатуры
				SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
				r_temp_buf_u.CopyFromMb_INNER(temp_buf, temp_buf.Len());
				for(size_t i = 0; i < r_temp_buf_u.Len(); i++) {
					const wchar_t c = r_temp_buf_u.C(i);
					KeyDownCommand kd;
					uint   tc = kd.SetCharU(c) ? kd.GetChar() : 0; 
					raw_buf.CatChar(static_cast<char>(tc));
				}
				pCode = raw_buf.cptr();
			}*/
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

/*static*/int FASTCALL PPChZnPrcssr::IsChZnCode(const char * pCode)
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
					// @v10.9.8 if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || oneof14(c, '!', '\"', '%', '&', '\'', '(', ')', '*', '+', '-', ',', '.', '/', '_') || isdec(c)))
					if(!(isasciialnum(c) || oneof14(c, '!', '\"', '%', '&', '\'', '(', ')', '*', '+', '-', ',', '.', '/', '_'))) // @v10.9.8
						result = 0;
				}
			}
		}
	}
	return result;
}

/*static*/int PPChZnPrcssr::InputMark(SString & rMark, SString * pReconstructedOriginal, const char * pExtraInfoText)
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
	if(!isempty(pExtraInfoText)) {
		dlg->setStaticText(CTL_CHZNMARK_INFO, pExtraInfoText);
	}
    while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_CHZNMARK_INPUT, temp_buf);
		GtinStruc gts;
		const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
		if(pczcr) {
			gts.GetToken(GtinStruc::fldOriginalText, &rMark);
			if(pReconstructedOriginal) {
				ReconstructOriginalChZnCode(gts, *pReconstructedOriginal);
			}
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
	{ 252, "refusal_receiver" }, // @v10.9.1 
	{ 415, "move_order" }, // @v10.9.2
	{ 416, "receive_order" },
	{ 431, "move_place" }, // @v10.8.7 doctypMovePlace
	{ 441, "move_unregistered_order" }, // @v11.2.0 move_unregistered_order
	{ 511, "retail_sale" }, // @v11.0.1 
	{ 602, "receive_order_notification" },
	{ 701, "accept" },
	{ 702, "posting" }, // @v10.9.7
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
	ChZnInterface() : Lth(PPFILNAM_CHZNTALK_LOG)
	{
	}
	~ChZnInterface()
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
		Document() : Dtm(ZERODATETIME), ReceivedDtm(ZERODATETIME), Type(doctypUnkn), 
			Format(SFileFormat::Unkn), Status(docstUnkn), DownloadStatus(0), Flags(0)
		{
		}
		Document(const Document & rS)
		{
			Copy(rS);
		}
		Document & FASTCALL operator = (const Document & rS)
		{
			return Copy(rS);
		}
		int    Parse(const char * pBuffer);
		int    GetTransactionPartyCode(PPID psnID, PPID locID, SString & rCode);
		int    Make(SXml::WDoc & rX, const ChZnInterface::InitBlock & rIb, const ChZnInterface::Packet * pPack);
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
		doctypUnkn                       = 0,
		doctypMdlpResult                   		 = 200,
		doctypMdlpQueryKizInfo             		 = 210,
		doctypMdlpKizInfo                  		 = 211,
		doctypMdlpRefusalReceiver          		 = 252, // @v10.9.1
		doctypMdlpMoveOrder                		 = 415, // @v10.9.2
		doctypMdlpReceiveOrder             		 = 416,
		doctypMdlpMovePlace                		 = 431, // @v10.8.7 
		doctypMdlpMoveUnregisteredOrder  = 441, // @v11.2.0
		doctypMdlpRetailSale             = 511, // @v11.0.1
		doctypMdlpReceiveOrderNotification 		 = 602,
		doctypMdlpAccept                 = 701,
		doctypMdlpPosting                = 702, // @v10.9.7
		//
		// Следующие типы определены для интерфейса ГИС МТ. Для них нет заданных внешним сервисом числовых значений
		//
		doctGisMt_OstDescription         = 1001,
		doctGisMt_Aggregation            = 1002,
		doctGisMt_Disaggregation         = 1003,
		doctGisMt_Reaggregation          = 1004,
		doctGisMt_LpIntroduceGoods       = 1005,
		doctGisMt_LkIndiCommissioning    = 1006,
		doctGisMt_LpGoodsImport          = 1007,
		doctGisMt_Crossborder            = 1008,
		doctGisMt_LpIntroduceGoodsCrossborderCSD = 1009,
		doctGisMt_IntroduceOST           = 1010,
		doctGisMt_LkContractCommissioning        = 1011,
		doctGisMt_LpReturn               = 1012,
		doctGisMt_LpShipGoods            = 1013,
		doctGisMt_LpShipReceipt          = 1014,
		doctGisMt_LpCancelShipment       = 1015,
		doctGisMt_LpAcceptGoods          = 1016,
		doctGisMt_LkReceipt              = 1017,
		doctGisMt_LkRemark               = 1018,
		doctGisMt_KmCancellation         = 1019,
		doctGisMt_AppliedKmCancellation  = 1020
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
		explicit Packet(int docType) : DocType(docType), Flags(0), P_Data(0), ChZnProdType(0)
		{
			switch(DocType) {
				case doctypMdlpResult: P_Data = new OperationResult(); break;
				case doctypMdlpQueryKizInfo: P_Data = new QueryKizInfo(); break;
				case doctypMdlpMoveOrder: // @v10.9.2
				case doctypMdlpReceiveOrder:
				case doctypMdlpMovePlace: // @v10.8.7 
				case doctypMdlpRefusalReceiver: // @v10.9.1
				case doctypMdlpPosting: // @v10.9.7
				case doctGisMt_LkReceipt: // @v10.9.10
				case doctGisMt_LpShipReceipt: // @v10.9.10
					P_Data = new PPBillPacket; 
					break; 
				case doctypMdlpRetailSale: // @v11.0.1
					P_Data = new CCheckPacket;
					break;
			}
		}
		~Packet()
		{
			switch(DocType) {
				case doctypMdlpResult: delete static_cast<OperationResult *>(P_Data); break;
				case doctypMdlpQueryKizInfo: delete static_cast<QueryKizInfo *>(P_Data); break;
				case doctypMdlpMoveOrder: // @v10.9.2
				case doctypMdlpReceiveOrder:
				case doctypMdlpMovePlace: // @v10.8.7 
				case doctypMdlpRefusalReceiver: // @v10.9.1
				case doctypMdlpPosting: // @v10.9.7
				case doctGisMt_LkReceipt: // @v10.9.10
				case doctGisMt_LpShipReceipt: // @v10.9.10
					delete static_cast<PPBillPacket *>(P_Data); 
					break;
				case doctypMdlpRetailSale: // @v11.0.1
					delete static_cast<CCheckPacket *>(P_Data);
					break;					
			}
		}
		const  int DocType;
		long   Flags;
		long   ChZnProdType; // @v11.1.11 GTCHZNPT_XXX
		void * P_Data;
	};
	
	int    ParseDocument(const SJson * pJsonObj, Document & rItem);
	int    ParseDocumentList(const char * pJsonInput, TSCollection <Document> & rList);
	int    SetupInitBlock(PPID guaID, const char * pEndPoint, InitBlock & rBlk);
	int    GetSign(const InitBlock & rIb, const void * pData, size_t dataLen, SString & rResultBuf);
	SString & MakeTargetUrl_(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const;
	enum {
		mhffTokenOnly  = 0x0001,
		mhffAuthBearer = 0x0002 // auth: Bearer else auth: Token
	};
	SString & MakeHeaderFields(const char * pToken, uint flags, StrStrAssocArray * pHdrFlds, SString & rBuf);
	const CERT_CONTEXT * GetClientSslCertificate(InitBlock & rIb);
	int    MakeAuthRequest(InitBlock & rBlk, SString & rBuf);
	int    MakeTokenRequest(InitBlock & rIb, const char * pAuthCode, SString & rBuf);
	int    MakeAuthRequest2(InitBlock & rBlk, SString & rBuf);
	int    MakeTokenRequest2(InitBlock & rIb, const char * pAuthCode, SString & rBuf);
	int    MakeDocumentRequest(const InitBlock & rIb, const ChZnInterface::Packet & rPack, const void * pData, size_t dataLen, S_GUID & rReqId, SString & rBuf);
	uint   GetLastWinInternetResponse(SString & rMsgBuf);
	uint   ReadReply(HINTERNET hReq, SString & rBuf);
	int    GetUserInfo2(InitBlock & rIb);
	int    GetIncomeDocList2_temp(InitBlock & rIb);
	enum {
		docfoldDocs     = 0,
		docfoldArc      = 1,
		docfoldBin      = 2,
		docfoldToSign   = 3,
		docfoldRejected = 4
	};
	struct DocumentFilt {
		DocumentFilt() : Flags(0), Folder(docfoldDocs), CountLimit(0), CountOffset(0)
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
	int    GetDocumentList(InitBlock & rIb, const DocumentFilt * pFilt, TSCollection <Document> & rList);
	int    GetDocument(const InitBlock & rIb, const S_GUID * pUuid, const InetUrl * pUrl, Document & rDoc);
	int    ReadJsonReplyForSingleItem(const char * pReply, const char * pTarget, SString & rResult);
	int    TransmitDocument2(const InitBlock & rIb, const ChZnInterface::Packet & rPack, SString & rReply);
	int    GetDocumentTicket(const InitBlock & rIb, const char * pDocIdent, SString & rTicket);
	int    Connect(InitBlock & rIb);
	// @v10.8.0 int    Connect2(InitBlock & rIb);
	int    GetToken2(const char * pAuthCode, InitBlock & rIb);
	int    GetPendingIdentList(const InitBlock & rIb, StringSet & rResult);
	int    CommitTicket(const char * pPath, const char * pIdent, const char * pTicket);
	int    ParseTicket(const char * pTicket, Packet ** ppP);
	int    GetDebugPath(const InitBlock & rIb, SString & rPath);
private:
	//int    LogTalking(const char * pPrefix, const char * pTargetUrl, const SString & rMsg);
	int    GetTemporaryFileName(const char * pPath, const char * pSubPath, const char * pPrefix, SString & rFn);
	int    CreatePendingFile(const char * pPath, const char * pIdent);

	PPGlobalServiceLogTalkingHelper Lth;
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
	{ ChZnInterface::doctGisMt_OstDescription, "OST_DESCRIPTION" }, // OST_DESCRIPTION; Описание остатков товара;json;Описание остатков товара - JSON
		//OST_DESCRIPTION_CSV;csv;Описание остатков товара - CSV
		//OST_DESCRIPTION_XML;xml;Описание остатков товара - XML
	{ ChZnInterface::doctGisMt_Aggregation, "AGGREGATION_DOCUMENT" },//AGGREGATION_DOCUMENT;Агрегация;json;Агрегация - JSON
		//AGGREGATION_DOCUMENT_CSV;csv;Агрегация - CSV
		//AGGREGATION_DOCUMENT_XML;xml;Агрегация - XML
	{ ChZnInterface::doctGisMt_Disaggregation, "DISAGGREGATION_DOCUMENT" }, //DISAGGREGATION_DOCUMENT;Расформирование агрегата;json;Расформирование агрегата - JSON
		//DISAGGREGATION_DOCUMENT_CSV;csv;Расформирование агрегата - CSV
		//DISAGGREGATION_DOCUMENT_XML;xml;Расформирование агрегата - XML
	{ ChZnInterface::doctGisMt_Reaggregation, "REAGGREGATION_DOCUMENT" }, //REAGGREGATION_DOCUMENT;Трансформация агрегата;json;Трансформация агрегата - JSON
		//REAGGREGATION_DOCUMENT_XML;xml;Трансформация агрегата - XML
		//REAGGREGATION_DOCUMENT_CSV;csv;Трансформация агрегата - CSV
	{ ChZnInterface::doctGisMt_LpIntroduceGoods, "LP_INTRODUCE_GOODS" }, //LP_INTRODUCE_GOODS;Ввод в оборот. Производство РФ;json;Ввод в оборот. Производство РФ - JSON
		//LP_INTRODUCE_GOODS_CSV;csv;Ввод в оборот. Производство РФ - CSV
		//LP_INTRODUCE_GOODS_XML;xml;Ввод в оборот. Производство РФ - XML
	{ ChZnInterface::doctGisMt_LkIndiCommissioning, "LK_INDI_COMMISSIONING" }, //LK_INDI_COMMISSIONING;Ввод в оборот. Полученных от физических лиц;json;Ввод в оборот. Полученных от физических лиц - JSON
		//LK_INDI_COMMISSIONING_CSV;csv;Ввод в оборот. Полученных от физических лиц - CSV
		//LK_INDI_COMMISSIONING_XML;xml;Ввод в оборот. Полученных от физических лиц - XML
	{ ChZnInterface::doctGisMt_LpGoodsImport, "LP_GOODS_IMPORT" }, //LP_GOODS_IMPORT;json;Ввод в оборот. Производство вне ЕАЭС - JSON
		//LP_GOODS_IMPORT_CSV;Ввод в оборот. Производство вне ЕАЭС;csv;Ввод в оборот. Производство вне ЕАЭС - CSV
		//LP_GOODS_IMPORT_XML;xml;Ввод в оборот. Производство вне ЕАЭС - XML
	{ ChZnInterface::doctGisMt_Crossborder, "CROSSBORDER" }, //CROSSBORDER;Ввод в оборот. Трансграничная торговля.;json;Ввод в оборот. Трансграничная торговля. JSON (MANUAL)
		//CROSSBORDER_CSV;csv;Ввод в оборот. Трансграничная торговля. CSV
		//CROSSBORDER_XML;xml;Ввод в оборот. Трансграничная торговля. XML
	{ ChZnInterface::doctGisMt_LpIntroduceGoodsCrossborderCSD, "LP_INTRODUCE_GOODS_CROSSBORDER_CSD" }, //LP_INTRODUCE_GOODS_CROSSBORDER_CSD_JSON;Ввод в оборот. На территории стран ЕАЭС (контрактное производство);json;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). JSON (MANUAL)
		//LP_INTRODUCE_GOODS_CROSSBORDER_CSD_XML;xml;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). CSV
		//LP_INTRODUCE_GOODS_CROSSBORDER_CSD_CSV;csv;Ввод в оборот. На территории стран ЕАЭС (контрактное производство). XML
	{ ChZnInterface::doctGisMt_IntroduceOST, "LP_INTRODUCE_OST" }, //LP_INTRODUCE_OST;Ввод в оборот. Маркировка остатков;json;Ввод в оборот. Маркировка остатков - JSON
		//LP_INTRODUCE_OST_CSV;csv;Ввод в оборот. Маркировка остатков - CSV
		//LP_INTRODUCE_OST_XML;xml;Ввод в оборот. Маркировка остатков - XML
	{ ChZnInterface::doctGisMt_LkContractCommissioning, "LK_CONTRACT_COMMISSIONING" }, //LK_CONTRACT_COMMISSIONING;Ввод в оборот. Контрактное производство РФ;json;Ввод в оборот. Контрактное производство РФ - JSON
		//LK_CONTRACT_COMMISSIONING_CSV;csv;Ввод в оборот. Контрактное производство РФ - CSV
		//LK_CONTRACT_COMMISSIONING_XML;xml;Ввод в оборот. Контрактное производство РФ - XML
	{ ChZnInterface::doctGisMt_LpReturn, "LP_RETURN" }, //LP_RETURN;Возврат в оборот;json;Возврат в оборот. JSON (MANUAL)
		//LP_RETURN_CSV;xml;Возврат в оборот. CSV
		//LP_RETURN_XML;csv;Возврат в оборот. XML
	{ ChZnInterface::doctGisMt_LpShipGoods, "LP_SHIP_GOODS" }, //LP_SHIP_GOODS;Отгрузка;json;Отгрузка - JSON
		//LP_SHIP_GOODS_CSV;csv;Отгрузка - CSV
		//LP_SHIP_GOODS_XML;xml;Отгрузка - XML
	{ ChZnInterface::doctGisMt_LpShipReceipt, "LP_SHIP_RECEIPT" }, //LP_SHIP_RECEIPT;Отгрузка с выводом из оборота.;json;Отгрузка с выводом из оборота. JSON (MANUAL)
		//LP_SHIP_RECEIPT_CSV;csv;Отгрузка с выводом из оборота. CSV
		//LP_SHIP_RECEIPT_XML;xml;Отгрузка с выводом из оборота. XML
	{ ChZnInterface::doctGisMt_LpCancelShipment, "LP_CANCEL_SHIPMENT" }, //LP_CANCEL_SHIPMENT;Отмена отгрузки;json;Отмена отгрузки. JSON (MANUAL)
	{ ChZnInterface::doctGisMt_LpAcceptGoods, "LP_ACCEPT_GOODS" }, //LP_ACCEPT_GOODS;Приемка;json;Приемка - JSON
		//LP_ACCEPT_GOODS_XML;xml;Приемка - XML
	{ ChZnInterface::doctGisMt_LkReceipt, "LK_RECEIPT" }, //LK_RECEIPT;Вывод товара из оборота;json;Вывод товара из оборота - JSON
		//LK_RECEIPT_CSV;csv;Вывод товара из оборота - CSV
		//LK_RECEIPT_XML;xml;Вывод товара из оборота - XML
	{ ChZnInterface::doctGisMt_LkRemark, "LK_REMARK" }, //LK_REMARK;Перемаркировка;json;Перемаркировка - JSON
		//LK_REMARK_CSV;csv;Перемаркировка - CSV
		//LK_REMARK_XML;xml;Перемаркировка - XML
	{ ChZnInterface::doctGisMt_KmCancellation, "LK_KM_CANCELLATION" }, //LK_KM_CANCELLATION;Списание ненанесенных КМ;json;Списание ненанесенных КМ - JSON
		//LK_KM_CANCELLATION_XML;xml;Списание ненанесенных КМ - XML
		//LK_KM_CANCELLATION_XSD;xsd;Списание ненанесенных КМ - XSD
	{ ChZnInterface::doctGisMt_AppliedKmCancellation, "LK_APPLIED_KM_CANCELLATION" },//LK_APPLIED_KM_CANCELLATION;Списание нанесенных КМ;json;Списание нанесенных КМ - JSON
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

static int ChZnGetDocTypeSymb(int docType, SString & rSymb)
{
	return SIntToSymbTab_GetSymb(ChZnDocTypeList, SIZEOFARRAY(ChZnDocTypeList), docType, rSymb);
}

int ChZnInterface::ParseDocument(const SJson * pJsonObj, Document & rItem)
{
	int    ok = -1;
	if(pJsonObj && pJsonObj->Type == SJson::tOBJECT) {
		const SJson * p_next = pJsonObj->P_Next;
		const SJson * p_fld_next = 0;
		for(const SJson * p_fld = pJsonObj->P_Child; p_fld; p_fld = p_fld_next) {
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
					if(p_fld->P_Child->Type == SJson::tTRUE)
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

int  ChZnInterface::ParseDocumentList(const char * pJsonInput, TSCollection <Document> & rList)
{
	int    ok = -1;
	SJson * p_json_doc = 0;
	if(json_parse_document(&p_json_doc, pJsonInput) == JSON_OK) {
		SString temp_buf;
		SJson * p_next = 0;
		SJson * p_fld_next = 0;
		if(p_json_doc) {
			if(p_json_doc->Type == SJson::tOBJECT) {
				SJson * p_child = p_json_doc->P_Child;
				if(p_child && p_child->Type == SJson::tSTRING && sstreqi_ascii(p_child->Text, "results")) {
					ok = 1;
					p_child = p_child->P_Child;
					if(p_child && p_child->Type == SJson::tARRAY) {
						for(const SJson * p_cur = p_child->P_Child; p_cur; p_cur = p_next) {
							if(p_cur->Type == SJson::tOBJECT) {
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

int ChZnInterface::Document::Parse(const char * pBuffer)
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

int ChZnInterface::Document::GetTransactionPartyCode(PPID psnID, PPID locID, SString & rCode)
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

static void _PutOperationDate(SXml::WNode & rN, SString & rTempBuf)
{
	rTempBuf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, 0);
	TimeZoneFmt(0, tzfmtConcat|tzfmtColon|tzfmtCurrent, rTempBuf);
	rN.PutInner("operation_date", rTempBuf);
}

static void _PutDocDateAndNum(const BillTbl::Rec & rBillRec, SXml::WNode & rN, SString & rTempBuf)
{
	rTempBuf = rBillRec.Code;
	// @v11.1.12 BillCore::GetCode(rTempBuf);
	rTempBuf.Transf(CTRANSF_INNER_TO_UTF8);
	rN.PutInner("doc_num", rTempBuf);
	rN.PutInner("doc_date", rTempBuf.Z().Cat(rBillRec.Dt, DATF_GERMAN|DATF_CENTURY));
}

int ChZnInterface::Document::Make(SXml::WDoc & rX, const ChZnInterface::InitBlock & rIb, const ChZnInterface::Packet * pPack)
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
	PPObjPerson psn_obj;
	PPObjGoods goods_obj;
	PPID   main_org_id = GetMainOrgID();
	if(pPack->DocType == doctGisMt_LkReceipt) {
		const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
		if(p_bp) {
			/*
				<withdrawal action_id="15" version="4"> 
					<trade_participant_inn>7714897741</trade_participant_inn>
					<withdrawal_type>RETAIL</withdrawal_type>
					<withdrawal_date>08.10.2020</withdrawal_date>
					<primary_document_type>OTHER</primary_document_type>
					<primary_document_number>PDN</primary_document_number> 
					<primary_document_date>08.10.2020</primary_document_date>
					<primary_document_custom_name>custom</primary_document_custom_name>
					<kkt_number>234</kkt_number>
					<products_list> 
						<product> 
							<ki>0000000000000000000FFFFFFFFFFFFFFFFFFF</ki>
							<cost>1000</cost>
							<primary_document_type>OTHER</primary_document_type>
							<primary_document_number>PDN</primary_document_number>
							<primary_document_date>08.10.2020</primary_document_date> 
							<primary_document_custom_name>custom</primary_document_custom_name> 
						</product>
					</products_list> 
				</withdrawal>
			*/
			// Далее все не верно (по ошибке сделано в соответствии с doctGisMt_LpShipReceipt)
			const PPID   rcvr_ar_id = p_bp->Rec.Object;
			const PPID   rcvr_psn_id = ObjectToPerson(rcvr_ar_id, 0);
			SString sender_inn;
			SString receiver_inn;
			SString doc_date_text;
			PPID   subj_psn_id = main_org_id;
			doc_date_text.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY);
			psn_obj.GetRegNumber(subj_psn_id, PPREGT_TPID, sender_inn);
			if(rcvr_psn_id)
				psn_obj.GetRegNumber(rcvr_psn_id, PPREGT_TPID, receiver_inn);
			SXml::WNode nh(rX, "withdrawal");
			nh.PutAttrib("action_id", "15");
			nh.PutAttrib("version", "4");
			nh.PutInner("trade_participant_inn", sender_inn);
			nh.PutInner("withdrawal_type", "RETAIL");
			nh.PutInner("withdrawal_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
			nh.PutInner("primary_document_type", "OTHER");
			// @v11.1.12 BillCore::GetCode(temp_buf = p_bp->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8);
			(temp_buf = p_bp->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8); // @v11.1.12 
			nh.PutInner("primary_document_number", temp_buf);
			nh.PutInner("primary_document_date", doc_date_text);
			// @v11.1.10 {
			PPLoadString("document_upd_s", temp_buf); // УПД
			nh.PutInner("primary_document_custom_name", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
			// } @v11.1.10 
			//nh.PutInner("primary_document_custom_name", "custom");
			//nh.PutInner("kkt_number", "234");
			//nh.PutInner("st_contract_id", ""); // optional
			{
				SXml::WNode npl(rX, "products_list");
				for(uint i = 0; i < p_bp->GetTCount(); i++) {
					const PPTransferItem & r_ti = p_bp->ConstTI(i);
					const  double cost = fabs(r_ti.NetPrice());
					double vat_in_cost = 0.0;
					{
						GTaxVect vect;
						vect.CalcTI(r_ti, p_bp->Rec.OpID, TIAMT_PRICE);
						vat_in_cost = vect.GetValue(GTAXVF_VAT) / fabs(r_ti.Quantity_);
					}
					p_bp->XcL.Get(i+1, 0, lotxcode_set);
					lotxcode_set.GetByBoxID(0, ss);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
						if(pczcr > 0) {
							mark_buf.Z();
							if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
								mark_buf.Cat("01").Cat(temp_buf);
								if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
									mark_buf.Cat("21").Cat(temp_buf);

									SXml::WNode np(rX, "product");
									XMLReplaceSpecSymb(mark_buf, "&<>\'");
									np.PutInner("ki", mark_buf); // Возможно, дожно быть "cis" вместо "ki"
									np.PutInner("cost", temp_buf.Z().Cat(R0i(cost * 100.0)));
									//
									// @v11.1.11 np.PutInner("primary_document_type", "OTHER");
									// @v11.1.11 np.PutInner("primary_document_number", "PDN");
									// @v11.1.11 np.PutInner("primary_document_date", doc_date_text);
									// @v11.1.11 PPLoadString("document_upd_s", temp_buf); // @v11.1.10 УПД
									// @v11.1.11 np.PutInner("primary_document_custom_name", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
								}
							}
						}
					}
				}
			}
		}
	}
	else if(pPack->DocType == doctGisMt_LpShipReceipt) {
		const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
		if(p_bp) {
			/*
				<?xml version="1.0" encoding="UTF-8"?>
				<shipment action_id="10" version="5">
					<trade_participant_inn_sender>0000000000</trade_participant_inn_sender>
					<trade_participant_inn_receiver>0000000000</trade_participant_inn_receiver>
					<transfer_date>01.01.2020</transfer_date>
					<move_document_number>12345678901234567890</move_document_number>
					<move_document_date>01.01.2020</move_document_date>
					<turnover_type>SELLING</turnover_type>
					<!--Optional:-->
					<withdrawal_type>STATE_ENTERPRISE</withdrawal_type>
					<!--Optional:-->
					<withdrawal_date>01.01.2020</withdrawal_date>
					<!--Optional:-->
					<st_contract_id>string</st_contract_id>
					<products_list>
						<!--1 or more repetitions:-->
						<product>
							<!--Optional:-->
							<ki>00000000000000FFFFFFFFFFFFFFF</ki>
							<!--Optional:-->
							<cost>100000</cost>
							<!--Optional:-->
							<vat_value>100000</vat_value>
						</product>
					</products_list>
				</shipment>
			*/
			const PPID   rcvr_ar_id = p_bp->Rec.Object;
			const PPID   rcvr_psn_id = ObjectToPerson(rcvr_ar_id, 0);
			SString sender_inn;
			SString receiver_inn;
			PPID   subj_psn_id = main_org_id;
			psn_obj.GetRegNumber(subj_psn_id, PPREGT_TPID, sender_inn);
			if(rcvr_psn_id)
				psn_obj.GetRegNumber(rcvr_psn_id, PPREGT_TPID, receiver_inn);
			SXml::WNode nh(rX, "shipment");
			nh.PutAttrib("action_id", "10");
			nh.PutAttrib("version", "5");
			nh.PutInner("trade_participant_inn_sender", sender_inn);
			nh.PutInner("trade_participant_inn_receiver", receiver_inn);
			nh.PutInner("transfer_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
			// @v11.1.12 BillCore::GetCode(temp_buf = p_bp->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8);
			(temp_buf = p_bp->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8); // @v11.1.12 
			nh.PutInner("move_document_number", temp_buf);
			nh.PutInner("move_document_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
			nh.PutInner("turnover_type", "SELLING");
			nh.PutInner("to_not_participant", "true"); // @v10.9.11 optional признак того, что отгружаем не участнику оборота чезн
			nh.PutInner("withdrawal_type", "NO_RETAIL_USE"); // optional
			nh.PutInner("withdrawal_date", temp_buf.Z().Cat(p_bp->Rec.Dt, DATF_GERMAN|DATF_CENTURY)); // optional
			// nh.PutInner("st_contract_id", ""); // optional
			{
				SXml::WNode npl(rX, "products_list");
				for(uint i = 0; i < p_bp->GetTCount(); i++) {
					const PPTransferItem & r_ti = p_bp->ConstTI(i);
					const  double cost = fabs(r_ti.NetPrice());
					double vat_in_cost = 0.0;
					{
						GTaxVect vect;
						vect.CalcTI(r_ti, p_bp->Rec.OpID, TIAMT_PRICE);
						vat_in_cost = vect.GetValue(GTAXVF_VAT) / fabs(r_ti.Quantity_);
					}
					p_bp->XcL.Get(i+1, 0, lotxcode_set);
					lotxcode_set.GetByBoxID(0, ss);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
						if(pczcr > 0) {
							mark_buf.Z();
							if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
								mark_buf.Cat("01").Cat(temp_buf);
								if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
									mark_buf.Cat("21").Cat(temp_buf);

									SXml::WNode np(rX, "product");
									XMLReplaceSpecSymb(mark_buf, "&<>\'");
									np.PutInner("ki", mark_buf);
									np.PutInner("cost", temp_buf.Z().Cat(cost, MKSFMTD(0, 2, 0)));
									np.PutInner("vat_value", temp_buf.Z().Cat(vat_in_cost, MKSFMTD(0, 2, 0)));
								}
							}
						}
					}
				}
			}
		}		
	}
	else {
		SXml::WNode wdocs(rX, "documents");
		wdocs.PutAttrib("session_ui", rIb.Token);
		wdocs.PutAttrib("version", "1.35"); // @v10.9.9 "1.34"-->"1.35"
		wdocs.PutAttrib(SXml::nst("xmlns", "xsi"), InetUrl::MkHttp("www.w3.org", "2001/XMLSchema-instance"));
		{
			SIntToSymbTab_GetSymb(CzDocType_SymbTab, SIZEOFARRAY(CzDocType_SymbTab), pPack->DocType, temp_buf);
			SXml::WNode wd(rX, temp_buf);
			wd.PutAttrib("action_id", temp_buf.Z().Cat(pPack->DocType));
			//
			if(pPack->DocType == doctypMdlpMovePlace) {
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   rcvr_ar_id = p_bp->Rec.Object;
					const PPID   rcvr_loc_id = PPObjLocation::ObjToWarehouse(rcvr_ar_id);
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					PPID   subj_psn_id = main_org_id;
					GetTransactionPartyCode(0, rcvr_loc_id, receiver_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					wd.PutInner("receiver_id", receiver_ident);
					_PutOperationDate(wd, temp_buf);
					_PutDocDateAndNum(p_bp->Rec, wd, temp_buf);
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
			else if(pPack->DocType == doctypMdlpRetailSale) { // @v11.0.1
				const CCheckPacket * p_ccp = static_cast<const CCheckPacket *>(pPack->P_Data);
				if(p_ccp) {
					/*
						<documents session_ui="4Aa246a6-D7e2-2465-a056-0234554369a3" version="1.34">
							<retail_sale action_id="511">
								<subject_id>19527400000042</subject_id>
								<operation_date>2017-04-09T15:08:00+05:00</operation_date>
								<sales>
									<union>
										<detail>
											<sgtin>11670012610151BBM13L07G86DQ</sgtin>
											<cost>17.0</cost>
											<vat_value>300.0</vat_value>
										</detail>
										<sale_docs>
											<doc_type>1</doc_type>
											<doc_name>cheque</doc_name>
											<doc_number>1</doc_number>
											<doc_date>04.04.2017</doc_date>
										</sale_docs>
									</union>
								</sales>
							</retail_sale>
						</documents>
					*/
					PPObjCashNode cnobj;
					PPCashNode cn_rec;
					if(cnobj.Fetch(p_ccp->Rec.CashID, &cn_rec) > 0 && cn_rec.LocID) {
						//p_ccp->Rec.CashID
						const PPID   subj_loc_id = cn_rec.LocID;
						PPID  subj_psn_id = main_org_id;
						GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
						wd.PutInner("subject_id", subj_ident);
						_PutOperationDate(wd, temp_buf);
						{
							SXml::WNode sn(rX, "sales");
							CCheckItem ccitem;
							for(uint i = 0; p_ccp->EnumLines(&i, &ccitem) > 0;) {
								p_ccp->GetLineTextExt(i, CCheckPacket::lnextChZnMark, temp_buf);
								if(temp_buf.NotEmptyS()) {
									const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
									if(pczcr > 0) {
										mark_buf.Z();
										if(gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
											mark_buf.Cat(temp_buf);
											if(gts.GetToken(GtinStruc::fldSerial, &temp_buf)) {
												mark_buf.Cat(temp_buf);
												double cost = R2(ccitem.Price - ccitem.Discount);
												double vat_in_cost = 0.0;
												PPGoodsTaxEntry gtx;
												if(goods_obj.FetchTax(ccitem.GoodsID, p_ccp->Rec.Dt, 0, &gtx) > 0) {
													GTaxVect vect;
													vect.Calc_(&gtx, cost, 1.0, GTAXVF_BEFORETAXES, 0);
													vat_in_cost = vect.GetValue(GTAXVF_VAT);
												}
												SXml::WNode un(rX, "union");
												{
													SXml::WNode dn(rX, "detail");
													dn.PutInner("sgtin", mark_buf);
													dn.PutInner("cost", temp_buf.Z().Cat(cost, MKSFMTD(0, 2, 0)));
													dn.PutInner("vat_value", temp_buf.Z().Cat(vat_in_cost, MKSFMTD(0, 2, 0)));
												}
												{
													SXml::WNode sdn(rX, "sale_docs");
													sdn.PutInner("doc_type", "1");
													sdn.PutInner("doc_name", "cheque");
													sdn.PutInner("doc_number", temp_buf.Z().Cat(p_ccp->Rec.Code));
													sdn.PutInner("doc_date", temp_buf.Z().Cat(p_ccp->Rec.Dt, DATF_GERMAN|DATF_CENTURY));
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
			else if(pPack->DocType == doctypMdlpMoveOrder) { // @v10.9.2
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   rcvr_ar_id = p_bp->Rec.Object;
					const PPID   rcvr_psn_id = ObjectToPerson(rcvr_ar_id, 0);
					const PPID   rcvr_loc_id = p_bp->GetDlvrAddrID();
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					//
					PPID   subj_psn_id = main_org_id;
					GetTransactionPartyCode(rcvr_psn_id, rcvr_loc_id, receiver_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					wd.PutInner("receiver_id", receiver_ident);
					_PutOperationDate(wd, temp_buf);
					_PutDocDateAndNum(p_bp->Rec, wd, temp_buf);
					wd.PutInner("turnover_type", temp_buf.Z().Cat(2L));
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
			else if(oneof2(pPack->DocType, doctypMdlpReceiveOrder, doctypMdlpPosting)) {
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   dlvr_ar_id = p_bp->Rec.Object;
					const PPID   dlvr_psn_id = ObjectToPerson(dlvr_ar_id, 0);
					const PPID   dlvr_loc_id = p_bp->GetDlvrAddrID();
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					PPID   subj_psn_id = main_org_id;
					GetTransactionPartyCode(dlvr_psn_id, dlvr_loc_id, shipper_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					if(pPack->DocType == doctypMdlpReceiveOrder) {
						wd.PutInner("shipper_id", shipper_ident);
					}
					else {
						SXml::WNode si(rX, "shipper_info");
						psn_obj.GetRegNumber(dlvr_psn_id, PPREGT_TPID, temp_buf);
						si.PutInner("inn", temp_buf.Transf(CTRANSF_INNER_TO_UTF8)); // @todo
						psn_obj.GetRegNumber(dlvr_psn_id, PPREGT_KPP, temp_buf);
						si.PutInner("kpp", temp_buf.Transf(CTRANSF_INNER_TO_UTF8)); // @todo
					}
					_PutOperationDate(wd, temp_buf);
					_PutDocDateAndNum(p_bp->Rec, wd, temp_buf);
					if(pPack->DocType == doctypMdlpReceiveOrder) {
						wd.PutInner("receive_type", temp_buf.Z().Cat(1L));
						wd.PutInner("source", temp_buf.Z().Cat(1L));
						wd.PutInner("contract_type", temp_buf.Z().Cat(1L));
					}
					else {
						wd.PutInner("contract_type", temp_buf.Z().Cat(1L));
						wd.PutInner("source", temp_buf.Z().Cat(1L));
					}
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
			else if(pPack->DocType == doctypMdlpRefusalReceiver) { // @v10.9.1
				const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(pPack->P_Data);
				if(p_bp) {
					const PPID   dlvr_ar_id = p_bp->Rec.Object;
					const PPID   dlvr_psn_id = ObjectToPerson(dlvr_ar_id, 0);
					const PPID   dlvr_loc_id = p_bp->GetDlvrAddrID();
					const PPID   subj_loc_id = p_bp->Rec.LocID;
					PPID   subj_psn_id = main_org_id;
					GetTransactionPartyCode(dlvr_psn_id, dlvr_loc_id, shipper_ident);
					GetTransactionPartyCode(subj_psn_id, subj_loc_id, subj_ident);
					wd.PutInner("subject_id", subj_ident);
					_PutOperationDate(wd, temp_buf);
					wd.PutInner("shipper_id", shipper_ident);
					// @v11.1.12 if(!isempty(p_bp->Rec.Memo)) {
					if(p_bp->SMemo.NotEmpty()) {
						// @v11.1.12 (temp_buf = p_bp->Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8);
						(temp_buf = p_bp->SMemo).Transf(CTRANSF_INNER_TO_UTF8); // @v11.1.12
					}
					else
						temp_buf = "refusal-receiver";
					wd.PutInner("reason", temp_buf);
					{
						SXml::WNode dtl(rX, "order_details");
						for(uint i = 0; i < p_bp->GetTCount(); i++) {
							const PPTransferItem & r_ti = p_bp->ConstTI(i);
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
												dtl.PutInner("sgtin", mark_buf);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else if(pPack->DocType == doctypMdlpQueryKizInfo) {
				const Packet::QueryKizInfo * p_bp = static_cast<const Packet::QueryKizInfo *>(pPack->P_Data);
				if(p_bp) {
					subj_ident = p_bp->SubjectIdent;
					if(subj_ident.IsEmpty()) {
						PPID   dlvr_psn_id = ObjectToPerson(p_bp->ArID, 0);
						PPID   subj_psn_id = main_org_id;
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

int ChZnInterface::SetupInitBlock(PPID guaID, const char * pEndPoint, InitBlock & rBlk)
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
	
int ChZnInterface::GetSign(const InitBlock & rIb, const void * pData, size_t dataLen, SString & rResultBuf)
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
			Lth.Log("crypto-pro-cmdline", 0, temp_buf);
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

SString & ChZnInterface::MakeTargetUrl_(int query, const char * pAddendum, const InitBlock & rIb, SString & rResult) const
{
	if(rIb.EndPoint.NotEmpty())
		rResult = rIb.EndPoint;
	else {
		switch(rIb.ProtocolId) {
			case InitBlock::protidEdoLtMdlp:
				(rResult = "https").Cat("://").Cat("mdlp").DotCat("edo").DotCat("crpt").DotCat("tech");
				break;
			case InitBlock::protidEdoLtInt:
				(rResult = "https").Cat("://").Cat("int").DotCat("edo").DotCat("crpt").DotCat("tech");
				break;
			case InitBlock::protidEdoLtElk:
				(rResult = "https").Cat("://").Cat("elk").DotCat("edo").DotCat("crpt").DotCat("tech");
				break;
			case InitBlock::protidGisMt:
				if(rIb.GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)
					(rResult = "https").Cat("://").Cat("demo").DotCat("lp").DotCat("crpt").DotCat("tech");
				else 
					(rResult = "https").Cat("://").Cat("ismp").DotCat("crpt").DotCat("ru");
				break;
			case InitBlock::protidMdlp:
			default:
				//rResult = (query == qDocumentSend) ? "https" : "http"; // @test
				// https://api.mdlp.crpt.ru:443/api/v1/documents/send
				if(oneof3(query, qDocumentSend, qGetDoc, qGetTicket)) 
					rResult = "https";
				else if(oneof2(query, qAuth, qToken)) // @v11.3.8
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
			else if(rIb.ProtocolId == InitBlock::protidGisMt) { // @v10.9.9
				rResult.Cat("api/v3/lk/documents/create");
			}
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
			if(rIb.ProtocolId == InitBlock::protidMdlp)
				rResult.Cat("documents/send"); 
			else if(rIb.ProtocolId == InitBlock::protidGisMt) { // @v10.9.10
				rResult.Cat("api/v3/lk/documents/create");
			}
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
	
int ChZnInterface::MakeAuthRequest2(InitBlock & rBlk, SString & rBuf)
{
	// GET https://ismp.crpt.ru/api/v3/auth/cert/key
	int    ok = 0;
	return ok;
}

int ChZnInterface::MakeTokenRequest2(InitBlock & rIb, const char * pAuthCode, SString & rBuf)
{
	// POST https://ismp.crpt.ru/api/v3/auth/cert
	int    ok = 0;
	return ok;
}

int ChZnInterface::MakeAuthRequest(InitBlock & rBlk, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SJson * p_json_req = 0;
	{
		p_json_req = new SJson(SJson::tOBJECT);
		p_json_req->InsertString("client_id", rBlk.CliAccsKey);
		p_json_req->InsertString("client_secret", rBlk.CliSecret);
		p_json_req->InsertString("user_id", rBlk.CliIdent);
		p_json_req->InsertString("auth_type", "SIGNED_CODE");
		THROW_SL(p_json_req->ToStr(rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int ChZnInterface::MakeTokenRequest(InitBlock & rIb, const char * pAuthCode, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString code_sign;
	SJson * p_json_req = 0;
	THROW(GetSign(rIb, pAuthCode, sstrlen(pAuthCode), code_sign));
	{
		p_json_req = new SJson(SJson::tOBJECT);
		p_json_req->InsertString("code", pAuthCode);
		p_json_req->InsertString("signature", code_sign);
		THROW_SL(p_json_req->ToStr(rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int ChZnInterface::MakeDocumentRequest(const InitBlock & rIb, const ChZnInterface::Packet & rPack, const void * pData, size_t dataLen, S_GUID & rReqId, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString code_sign;
	SString temp_buf;
	SString data_base64_buf;
	SJson * p_json_req = 0;
	THROW(GetSign(rIb, pData, dataLen, code_sign));
	data_base64_buf.EncodeMime64(pData, dataLen);
	if(rIb.ProtocolId == rIb.protidGisMt) { // @v10.9.9
		/*
			{ 
				"document_format": "string", 
				"product_document": "<Документ формата Base64>", 
				"type": "string", 
				"signature": "<Открепленная УКЭП формата Base64>" 
			}
		*/
		if(ChZnGetDocTypeSymb(rPack.DocType, temp_buf)) {
			p_json_req = new SJson(SJson::tOBJECT);
			p_json_req->InsertString("document_format", "XML");
			p_json_req->InsertString("product_document", data_base64_buf);
			p_json_req->InsertString("type", temp_buf.CatChar('_').Cat("XML"));
			p_json_req->InsertString("signature", code_sign);
			THROW_SL(p_json_req->ToStr(rBuf));
		}
	}
	else {
		p_json_req = new SJson(SJson::tOBJECT);
		p_json_req->InsertString("document", data_base64_buf);
		p_json_req->InsertString("sign", code_sign);
		rReqId.Generate();
		rReqId.ToStr(S_GUID::fmtIDL, temp_buf);
		p_json_req->InsertString("request_id", temp_buf);
		THROW_SL(p_json_req->ToStr(rBuf));
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

const CERT_CONTEXT * ChZnInterface::GetClientSslCertificate(InitBlock & rIb)
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
			if(cnu.IsEq(cert_text)) {
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

uint ChZnInterface::GetLastWinInternetResponse(SString & rMsgBuf)
{
	rMsgBuf.Z();
	DWORD last_err = 0;
	TCHAR last_err_text[1024];
	DWORD last_err_text_len = SIZEOFARRAY(last_err_text);
	InternetGetLastResponseInfo(&last_err, last_err_text, &last_err_text_len);
	rMsgBuf.CopyUtf8FromUnicode(last_err_text, last_err_text_len, 1);
	return last_err;
}

SString & ChZnInterface::MakeHeaderFields(const char * pToken, uint flags, StrStrAssocArray * pHdrFlds, SString & rBuf)
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

int ChZnInterface::GetDocumentTicket(const InitBlock & rIb, const char * pDocIdent, SString & rTicket)
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
				Lth.Log("req", url_buf, temp_buf.Z());
				if(HttpSendRequest(h_req, SUcSwitch(hdr_buf), -1, const_cast<char *>(req_buf.cptr())/*optional data*/, req_buf.Len()/*optional data length*/)) {
					SString wi_msg;
					uint  wi_code = GetLastWinInternetResponse(wi_msg);
					ReadReply(h_req, reply_buf);
					Lth.Log("rep", 0, reply_buf);
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
			Lth.Log("req", url_buf, temp_buf.Z());
			THROW_SL(c.HttpGet(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					if(ReadJsonReplyForSingleItem(reply_buf, "link", link_buf) > 0) {
						wr_stream.Open(ack_buf.Z(), SFile::mWrite);
						url.Parse(link_buf);
						Lth.Log("req", link_buf, temp_buf.Z());
						THROW_SL(c.HttpGet(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, &wr_stream));
						p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							rTicket.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, rTicket);
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

int ChZnInterface::GetPendingIdentList(const InitBlock & rIb, StringSet & rResult)
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

int ChZnInterface::GetDebugPath(const InitBlock & rIb, SString & rPath)
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

int ChZnInterface::GetTemporaryFileName(const char * pPath, const char * pSubPath, const char * pPrefix, SString & rFn)
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

int ChZnInterface::CreatePendingFile(const char * pPath, const char * pIdent)
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

int ChZnInterface::ParseTicket(const char * pTicket, Packet ** ppP)
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
					THROW(p_pack = new Packet(doctypMdlpResult));
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

int ChZnInterface::CommitTicket(const char * pPath, const char * pIdent, const char * pTicket)
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

int ChZnInterface::TransmitDocument2(const InitBlock & rIb, const ChZnInterface::Packet & rPack, SString & rReply)
{
	/* 
	Товарные группы:
		Код в БД  Наименование  Описание 
		1  lp  Предметы одежды, бельё постельное, столовое, туалетное и кухонное 
		2  shoes  Обувные товары 
		3  tobacco  Табачная продукция 
		4  perfumery  Духи и туалетная вода 
		5  tires  Шины и покрышки пневматические резиновые новые 
		6  electronics  Фотокамеры (кроме кинокамер), фотовспышки и лампы-вспышки 
		8  milk  Молочная продукция 
		9  bicycle  Велосипеды и велосипедные рамы 
		10  wheelchairs  Кресла-коляски 
		12  otp  Альтернативная табачная продукция 
		13  water  Упакованная вода 
		14  furs  Товары из натурального меха 
		15  beer  Пиво, напитки, изготавливаемые на основе пива, слабоалкогольные напитки 
		16  ncp  Никотиносодержащая продукция 
		17  bio  Биологические активные добавки к пище
	*/
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
						Lth.Log("req-rawdoc", 0, data_buf);
					}
					THROW(MakeDocumentRequest(rIb, rPack, data_buf.cptr(), data_buf.Len(), req_id, req_buf));
					Lth.Log("req", url_buf, req_buf);
					if(rIb.ProtocolId == rIb.protidMdlp) {
						const  int mdlp_use_curl = 0; // не работает mdlp через curl: ебаное гост-шифрование. желаю всем авторам честного знака и всему фсб сдохнуть!
						if(mdlp_use_curl) {
							ScURL c;
							MakeHeaderFields(rIb.Token, 0, &hdr_flds, temp_buf);
							SBuffer ack_buf;
							SFile wr_stream(ack_buf.Z(), SFile::mWrite);
							THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
							{
								SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
								if(p_ack_buf)
									reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							}
						}
						else {
							int    wininet_err = 0;
							WinInternetHandleStack hstk;
							HINTERNET h_inet_sess = 0;
							HINTERNET h_connection = 0;
							HINTERNET h_req = 0;
							THROW(h_inet_sess = hstk.Push(InternetOpen(_T("Papyrus"), /*INTERNET_OPEN_TYPE_PRECONFIG*/INTERNET_OPEN_TYPE_DIRECT, 0/*lpszProxy*/, 0/*lpszProxyBypass*/, 0/*dwFlags*/)));
							THROW(h_connection = hstk.PushConnection(url, h_inet_sess));
							{
								const size_t req_len = req_buf.Len();
								SStringU hdr_flds_u;
								STempBuffer temp_req_buf(req_len + 1024);
								THROW_SL(temp_req_buf.IsValid());
								memcpy(temp_req_buf, req_buf.cptr(), req_len+1);
								THROW(h_req = hstk.PushHttpRequestPost(h_connection, url));
								MakeHeaderFields(rIb.Token, 0, &hdr_flds, temp_buf);
								hdr_flds_u.CopyFromMb_OUTER(temp_buf, temp_buf.Len());
								if(HttpSendRequest(h_req, hdr_flds_u, hdr_flds_u.Len(), temp_req_buf, req_len)) {
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
					}
					else if(rIb.ProtocolId == rIb.protidGisMt) { // @v10.9.9
						ScURL c;
						MakeHeaderFields(rIb.Token, mhffAuthBearer, &hdr_flds, temp_buf); // @v11.1.11
						/*
								document_format:
									MANUAL - json
									XML - xml
									CSV - csv
								product_document:
									MIME64 document-body
								product_group:
									clothes – Предметы одежды, белье постельное, столовое, туалетное и кухонное
									shoes – Обувные товары
									tobacco – Табачная продукция
									perfumery – Духи и туалетная вода
									tires – Шины и покрышки пневматические резиновые новые
									electronics – Фотокамеры (кроме кинокамер), фотовспышки и лампы-вспышки
									pharma – Лекарственные препараты для медицинского применения
									milk – Молочная продукция
									bicycle – Велосипеды и велосипедные рамы
									wheelchairs – Кресла-коляски
								type: ChZnDocTypeList
									
								{
									"document_format": "string",
									"product_document": "string",
									"product_group": "string",
									"signature": "string",
									"type": "string"
								}
						*/
						{
							// @v11.1.11 {
							if(rPack.ChZnProdType > 0) {
								const char * p_chzn_prodtype_symb = 0;
								switch(rPack.ChZnProdType) {
									case GTCHZNPT_FUR: p_chzn_prodtype_symb = ""; break;
									case GTCHZNPT_TOBACCO: p_chzn_prodtype_symb = "tobacco"; break;
									case GTCHZNPT_SHOE: p_chzn_prodtype_symb = "shoes"; break;
									case GTCHZNPT_MEDICINE: p_chzn_prodtype_symb = 0; break;
									case GTCHZNPT_CARTIRE: p_chzn_prodtype_symb = "tires"; break;
									case GTCHZNPT_TEXTILE: p_chzn_prodtype_symb = "lp"; break;
									case GTCHZNPT_PERFUMERY: p_chzn_prodtype_symb = "perfumery"; break;
									case GTCHZNPT_MILK: p_chzn_prodtype_symb = "milk"; break;
								}
								if(!isempty(p_chzn_prodtype_symb)) {
									temp_buf.Z().CatEq("pg", p_chzn_prodtype_symb);
									url.SetComponent(InetUrl::cQuery, temp_buf);
								}
							}
							// } @v11.1.11 
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
					Lth.Log("rep", 0, reply_buf);
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
	xmlBufferFree(p_xml_buf);
	return ok;
}

uint ChZnInterface::ReadReply(HINTERNET hReq, SString & rBuf)
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

int ChZnInterface::GetUserInfo2(InitBlock & rIb)
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

int ChZnInterface::GetToken2(const char * pAuthCode, InitBlock & rIb)
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

int ChZnInterface::ReadJsonReplyForSingleItem(const char * pReply, const char * pTarget, SString & rResult)
{
	rResult.Z();
	int    ok = -1;
	SJson * p_json_doc = 0;
	if(json_parse_document(&p_json_doc, pReply) == JSON_OK) {
		SString temp_buf;
		const SJson * p_next = 0;
		for(const SJson * p_cur = p_json_doc; rResult.IsEmpty() && p_cur; p_cur = p_next) {
			p_next = p_cur->P_Next;
			switch(p_cur->Type) {
				case SJson::tOBJECT: p_next = p_cur->P_Child; break;
				case SJson::tSTRING:
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

int ChZnInterface::GetDocument(const InitBlock & rIb, const S_GUID * pUuid, const InetUrl * pUrl, Document & rDoc)
{
	int    ok = -1;
	SJson * p_json_req = 0;
	SJson * p_json_doc = 0;
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
			Lth.Log("req", 0, req_buf);
			THROW_SL(c.HttpGet(*pUrl, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
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
				Lth.Log("req", 0, req_buf);
				if(HttpSendRequest(h_req, SUcSwitch(hdr_buf), -1, const_cast<char *>(req_buf.cptr()), req_buf.Len())) {
					SString wi_msg;
					uint  wi_code = GetLastWinInternetResponse(wi_msg);
					ReadReply(h_req, reply_buf);
					Lth.Log("rep", 0, reply_buf);
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

int ChZnInterface::GetDocumentList(InitBlock & rIb, const DocumentFilt * pFilt, TSCollection <Document> & rList)
{
	int    ok = -1;
	SJson * p_json_req = 0;
	SString temp_buf;
	SString req_buf;
	SString url_buf;
	SBuffer ack_buf;
	InetUrl url(MakeTargetUrl_(qGetDocList, 0, rIb, url_buf));
	{
		req_buf.Z();
		{
			// { "filter": { "doc_status": "PROCESSED_DOCUMENT" }, "start_from": 0, "count": 100 }
			p_json_req = new SJson(SJson::tOBJECT);
			{
				SJson * p_json_filt = new SJson(SJson::tOBJECT);
				p_json_filt->InsertString("doc_status", "PROCESSED_DOCUMENT");
				p_json_req->Insert("filter", p_json_filt);
			}
			p_json_req->Insert("start_from", json_new_number("0"));
			p_json_req->Insert("count", json_new_number("100"));
			THROW_SL(p_json_req->ToStr(req_buf));
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
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, temp_buf);
					ParseDocumentList(temp_buf, rList);
				}
			}
		}
	}
	CATCHZOK
	delete p_json_req;
	return ok;
}

int ChZnInterface::GetIncomeDocList2_temp(InitBlock & rIb)
{
	int    ok = -1;
	int    wininet_err = 0;
	int    win_err = 0;
	SJson * p_json_req = 0;
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
			p_json_req = new SJson(SJson::tOBJECT);
			{
				SJson * p_json_filt = new SJson(SJson::tOBJECT);
				p_json_filt->InsertString("doc_status", "PROCESSED_DOCUMENT");
				p_json_req->Insert("filter", p_json_filt);
			}
			p_json_req->Insert("start_from", json_new_number("0"));
			p_json_req->Insert("count", json_new_number("100"));
			THROW_SL(p_json_req->ToStr(req_buf));
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

int ChZnInterface::Connect(InitBlock & rIb)
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
			// @v11.3.8 {
			{
				HINTERNET h_inet_sess = 0;
				HINTERNET h_connection = 0;
				HINTERNET h_req = 0;
				WinInternetHandleStack hstk;
				int    wininet_err = 0;
				int    win_err = 0;
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
						if(ReadJsonReplyForSingleItem(temp_buf, "code", result_code) > 0)
							ok = 1;
					}
					else
						wininet_err = GetLastError();
				}
			}
			// } @v11.3.8 
			/*{
				ScURL c;
				StrStrAssocArray hdr_flds;
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream)); // ? ScURL::mfDontVerifySslPeer
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, temp_buf);
							if(ReadJsonReplyForSingleItem(temp_buf, "code", result_code) > 0)
								ok = 1;
						}
					}
				}
			}*/
		}
		if(ok > 0) {
			ok = -1;
			InetUrl url(MakeTargetUrl_(qToken, 0, rIb, url_buf));
			THROW(MakeTokenRequest(rIb, result_code, req_buf));
			// @v11.3.8 {
			{
				HINTERNET h_inet_sess = 0;
				HINTERNET h_connection = 0;
				HINTERNET h_req = 0;
				WinInternetHandleStack hstk;
				int    wininet_err = 0;
				int    win_err = 0;
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
						if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
							ok = 1;
					}
					else
						wininet_err = GetLastError();
				}
			}
			// } @v11.3.8 
			/*{
				ScURL c;
				StrStrAssocArray hdr_flds;
				SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream)); // ? ScURL::mfDontVerifySslPeer
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, temp_buf);
							if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
								ok = 1;
						}
					}
				}
			}*/
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
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpGet(url_buf, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, temp_buf);
							{
								SJson * p_json_doc = 0;
								if(json_parse_document(&p_json_doc, temp_buf) == JSON_OK) {
									SString temp_buf;
									const SJson * p_next = 0;
									for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_next) {
										p_next = p_cur->P_Next;
										switch(p_cur->Type) {
											case SJson::tOBJECT: p_next = p_cur->P_Child; break;
											case SJson::tSTRING:
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
				SJson json_req(SJson::tOBJECT);
				temp_buf.Z().Cat(result_uuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				json_req.InsertString("uuid", temp_buf);
				json_req.InsertString("data", signed_data);
				THROW_SL(json_req.ToStr(req_buf));
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
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, temp_buf);
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
PPChZnPrcssr::Param::Param() : GuaID(0), LocID(0)
{
	Period.Z();
}

PPChZnPrcssr::QueryParam::QueryParam() : DocType(0), Flags(0), GuaID(0), LocID(0)
{
}

PPChZnPrcssr::PPChZnPrcssr(PPLogger * pOuterLogger) : PPEmbeddedLogger(0, pOuterLogger, PPFILNAM_CHZN_LOG, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER), P_Ib(new ChZnInterface::InitBlock)
{
}

PPChZnPrcssr::~PPChZnPrcssr()
{
	delete static_cast<ChZnInterface::InitBlock *>(P_Ib);
}

int PPChZnPrcssr::EditParam(Param * pParam)
{
	class ChZnPrcssrParamDialog : public TDialog {
		DECL_DIALOG_DATA(PPChZnPrcssr::Param);
	public:
		ChZnPrcssrParamDialog() : TDialog(DLG_CHZNPARAM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_CHZNPARAM_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, 0);
			SetupLocationCombo(this, CTLSEL_CHZNPARAM_LOC, Data.LocID, 0, LOCTYP_WAREHOUSE, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_CHZNPARAM_GUA, &Data.GuaID);
			getCtrlData(CTLSEL_CHZNPARAM_LOC, &Data.LocID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(ChZnPrcssrParamDialog, pParam);
}

int PPChZnPrcssr::EditQueryParam(PPChZnPrcssr::QueryParam * pData)
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

int PPChZnPrcssr::InteractiveQuery()
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
				ChZnInterface::Packet pack(ChZnInterface::doctypMdlpQueryKizInfo);
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

int PPChZnPrcssr::PrepareBillPacketForSending(PPID billID, void * pChZnPacket)
{
	int    suited = 0;
	ChZnInterface::Packet * p_chzn_packet = static_cast<ChZnInterface::Packet *>(pChZnPacket);
	PPObjBill * p_bobj = BillObj;
	PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_chzn_packet->P_Data);
	long chzn_prod_type = 0; // @v11.1.11
	PPObjGoods goods_obj; // @v11.1.11
	Goods2Tbl::Rec goods_rec; // @v11.1.11
	PPGoodsType2 gt_rec; // @v11.1.11
	if(p_bobj->ExtractPacket(billID, p_bp) > 0) {
		PPLotExtCodeContainer::MarkSet lotxcode_set;
		PPLotExtCodeContainer::MarkSet::Entry msentry;
		for(uint tidx = 0; !suited && tidx < p_bp->GetTCount(); tidx++) {
			const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
			// @v11.1.11 {
			if(goods_obj.Fetch(r_ti.GoodsID, &goods_rec) > 0 && goods_rec.GoodsTypeID && goods_obj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0) {
				if(gt_rec.ChZnProdType) {
					if(!chzn_prod_type)
						chzn_prod_type = gt_rec.ChZnProdType;
					else if(chzn_prod_type != gt_rec.ChZnProdType)
						chzn_prod_type = -1;
				}
			}
			// } @v11.1.11 
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
	p_chzn_packet->ChZnProdType = chzn_prod_type; // @v11.1.11
	return suited ? 1 : -1;
}

int PPChZnPrcssr::TransmitCcList(const Param & rP, const TSCollection <CCheckPacket> & rList)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	ChZnInterface::Packet * p_pack = 0;
	SString temp_buf;
	SString fmt_buf;
	SString msg_buf;
	SString result_doc_ident;
	SString edi_ident;
	CCheckCore cc_core;
	TSCollection <ChZnInterface::Packet> pack_list;
	ChZnInterface ifc;
	ChZnInterface::InitBlock * p_ib = static_cast<ChZnInterface::InitBlock *>(P_Ib);
	p_ib->ProtocolId = ChZnInterface::InitBlock::protidMdlp;
	THROW(ifc.SetupInitBlock(rP.GuaID, 0, *p_ib));
	{
		for(uint i = 0; i < rList.getCount(); i++) {
			const CCheckPacket * p_src_cc_pack = rList.at(i);
			if(p_src_cc_pack) {
				p_pack = new ChZnInterface::Packet(ChZnInterface::doctypMdlpRetailSale);
				if(p_pack->P_Data) {
					*static_cast<CCheckPacket *>(p_pack->P_Data) = *p_src_cc_pack;
					pack_list.insert(p_pack);
					p_pack = 0;
				}
				else
					ZDELETE(p_pack);
			}
		}
	}
	if(ifc.Connect(*p_ib) > 0) {
		for(uint bpidx = 0; bpidx < pack_list.getCount(); bpidx++) {
			const ChZnInterface::Packet * p_inner_pack = pack_list.at(bpidx);
			const CCheckPacket * p_cc_pack = static_cast<const CCheckPacket *>(p_inner_pack->P_Data);
			int tdr = ifc.TransmitDocument2(*p_ib, *p_inner_pack, result_doc_ident);
			if(tdr > 0) {
				if(result_doc_ident.NotEmpty()) {
					cc_core.UpdateExtText(p_cc_pack->Rec.ID, CCheckPacket::extssChZnProcessingTag, result_doc_ident, 1);
				}
				//
				CCheckCore::MakeCodeString(&p_cc_pack->Rec, temp_buf);
				//p_cc_pack
				//PPTXT_CCPACKSENTTOCHZN              "Кассовый чек %s успешно отправлен на сервер честный знак. Тикет: %s" 
				PPLoadText(PPTXT_CCPACKSENTTOCHZN, fmt_buf);
				msg_buf.Printf(fmt_buf, temp_buf.cptr(), result_doc_ident.cptr());
				Log(msg_buf);
			}
			else if(tdr == 0)
				LogLastError();
		}
	}
	CATCH
		LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPChZnPrcssr::Run(const Param & rP)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	Reference * p_ref = PPRef;
	ChZnInterface::Packet * p_pack = 0;
	SString temp_buf;
	SString result_doc_ident;
	SString edi_ident;
	PPID   chzn_252_op_id = 0;
	LAssocArray op_assoc_list;
	StringSet pending_list;
	//
	TSCollection <ChZnInterface::Packet> pack_list;
	ChZnInterface ifc;
	ChZnInterface::InitBlock * p_ib = static_cast<ChZnInterface::InitBlock *>(P_Ib);
	p_ib->ProtocolId = ChZnInterface::InitBlock::protidMdlp;
	THROW(ifc.SetupInitBlock(rP.GuaID, 0, *p_ib));
	THROW(ifc.GetPendingIdentList(*p_ib, pending_list));
	//
	{
		PrcssrAlcReport::Config alcr_cfg;
		PrcssrAlcReport::ReadConfig(&alcr_cfg);
		PPIDArray base_op_list;
		PPIDArray op_list;
		if(base_op_list.Z().addnz(alcr_cfg.RcptOpID) > 0) {
			PPObjOprKind::ExpandOpList(base_op_list, op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				const PPID op_id = op_list.get(i);
				if(!op_assoc_list.Search(op_id, 0, 0)) {
					if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp)
						op_assoc_list.Add(op_id, ChZnInterface::doctypMdlpReceiveOrder);
				}
			}
		}
		if(base_op_list.Z().addnz(alcr_cfg.IntrExpndOpID) > 0) {
			PPObjOprKind::ExpandOpList(base_op_list, op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				const PPID op_id = op_list.get(i);
				if(IsIntrExpndOp(op_id) && !op_assoc_list.Search(op_id, 0, 0)) {
					if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp)
						op_assoc_list.Add(op_id, ChZnInterface::doctypMdlpMovePlace);
				}
			}
		}
		if(base_op_list.Z().addnz(alcr_cfg.SupplRetOpID) > 0) {
			PPObjOprKind::ExpandOpList(base_op_list, op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				const PPID op_id = op_list.get(i);
				if(!op_assoc_list.Search(op_id, 0, 0)) {
					if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp)
						op_assoc_list.Add(op_id, ChZnInterface::doctypMdlpMoveOrder);
				}
			}
		}
		// @v10.9.10 {
		if(base_op_list.Z().addnz(alcr_cfg.ExpndEtcOpID) > 0) {
			PPObjOprKind::ExpandOpList(base_op_list, op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				const PPID op_id = op_list.get(i);
				if(!op_assoc_list.Search(op_id, 0, 0)) {
					if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidGisMt)
						op_assoc_list.Add(op_id, ChZnInterface::doctGisMt_LkReceipt);
				}
			}
		}
		if(base_op_list.Z().addnz(alcr_cfg.ExpndOpID) > 0) {
			PPObjOprKind::ExpandOpList(base_op_list, op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				const PPID op_id = op_list.get(i);
				if(!op_assoc_list.Search(op_id, 0, 0)) {
					if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidGisMt)
						op_assoc_list.Add(op_id, ChZnInterface::doctGisMt_LpShipReceipt);
					else if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp) // @v11.2.0
						op_assoc_list.Add(op_id, ChZnInterface::doctypMdlpMoveUnregisteredOrder);
				}
			}
		}
		// } @v10.9.10 
	}
	// @v10.9.1 {
	if(p_ib->ProtocolId == ChZnInterface::InitBlock::protidMdlp) {
		PPObjOprKind op_obj;
		PPOprKind en_op_rec;
		for(SEnum en = op_obj.Enum(0); en.Next(&en_op_rec) > 0;) {
			if(sstreqi_ascii(en_op_rec.Symb, "chzn-252") && en_op_rec.OpTypeID == PPOPT_DRAFTEXPEND) {
				if(!chzn_252_op_id && !op_assoc_list.Search(chzn_252_op_id, 0, 0))
					chzn_252_op_id = en_op_rec.ID;
			}
		}
		if(chzn_252_op_id) {
			op_assoc_list.Add(chzn_252_op_id, ChZnInterface::doctypMdlpRefusalReceiver);
		}
	}
	// } @v10.9.1
	if(op_assoc_list.getCount()) {
		for(uint opidx = 0; opidx < op_assoc_list.getCount(); opidx++) {
			const PPID op_id = op_assoc_list.at(opidx).Key;
			const PPID chzn_op_id = op_assoc_list.at(opidx).Val;
			PPOprKind op_rec;
			BillTbl::Rec bill_rec;
			GetOpData(op_id, &op_rec);
			for(DateIter di(&rP.Period); p_bobj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && p_bobj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIIDENT, edi_ident) <= 0) { // если тег установлен, то док уже был передан
						int    suited = 0;
						if(oneof2(chzn_op_id, ChZnInterface::doctGisMt_LkReceipt, ChZnInterface::doctGisMt_LpShipReceipt)) { // @v10.9.10
							assert(bill_rec.OpID == op_id);
							THROW_SL(p_pack = new ChZnInterface::Packet(chzn_op_id));
							if(PrepareBillPacketForSending(bill_rec.ID, p_pack) > 0)
								suited = 1;
						}
						else if(chzn_op_id == ChZnInterface::doctypMdlpMovePlace) {
							assert(bill_rec.OpID == op_id);
							assert(IsIntrExpndOp(bill_rec.OpID));
							const PPID dest_loc_id = bill_rec.Object ? PPObjLocation::ObjToWarehouse(bill_rec.Object) : 0;
							if(dest_loc_id && p_ref->Ot.GetTagStr(PPOBJ_LOCATION, dest_loc_id, PPTAG_LOC_CHZNCODE, temp_buf) > 0) {
								THROW_SL(p_pack = new ChZnInterface::Packet(chzn_op_id));
								if(PrepareBillPacketForSending(bill_rec.ID, p_pack) > 0)
									suited = 1;
							}
						}
						else if(chzn_op_id == ChZnInterface::doctypMdlpMoveUnregisteredOrder) { // @v11.2.0 @construction
							assert(bill_rec.OpID == op_id);
							const PPID dest_psn_id = bill_rec.Object ? ObjectToPerson(bill_rec.Object, 0) : 0;
							if(dest_psn_id) {
								; // @todo	
							}
						}
						if(oneof3(chzn_op_id, ChZnInterface::doctypMdlpReceiveOrder, ChZnInterface::doctypMdlpRefusalReceiver, ChZnInterface::doctypMdlpMoveOrder)) {
							const PPID psn_id = bill_rec.Object ? ObjectToPerson(bill_rec.Object, 0) : 0;
							if(psn_id && p_ref->Ot.GetTagStr(PPOBJ_PERSON, psn_id, PPTAG_PERSON_CHZNCODE, temp_buf) > 0) {
								PPID   local_chzn_op_id = chzn_op_id;
								// @v10.9.7 {
								if(chzn_op_id == ChZnInterface::doctypMdlpReceiveOrder) {
									if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_KEYWORDS, temp_buf) > 0) {
										if(temp_buf.Search("chzn-702", 0, 1, 0) || temp_buf.Search("chzn702", 0, 1, 0)) {
											local_chzn_op_id = ChZnInterface::doctypMdlpPosting;
										}
									}
								}
								// } @v10.9.7 
								THROW_SL(p_pack = new ChZnInterface::Packet(local_chzn_op_id));
								if(PrepareBillPacketForSending(bill_rec.ID, p_pack) > 0) {
									suited = 1;
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

/*static*/int PPChZnPrcssr::Test()
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
