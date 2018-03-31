// PPEDI.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop

SLAPI PPEdiProcessor::ProviderImplementation::ProviderImplementation(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags) : 
	Epp(rEpp), MainOrgID(mainOrgID), Flags(flags)
{
}
		
SLAPI PPEdiProcessor::ProviderImplementation::~ProviderImplementation()
{
}

int SLAPI PPEdiProcessor::ProviderImplementation::ValidateGLN(const SString & rGLN)
{
	int    ok = 0;
	if(rGLN.NotEmpty()) {
		SNaturalTokenArray ntl;
		SNaturalTokenStat nts;
		TR.Run(rGLN.ucptr(), rGLN.Len(), ntl, &nts);
		if(nts.Seq & SNTOKSEQ_DEC)
			ok = 1;
	}
	return ok;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetArticleGLN(PPID arID, SString & rGLN)
{
	int    ok = 0;
	rGLN.Z();
	PPID   psn_id = ObjectToPerson(arID, 0);
	if(psn_id) {
		PsnObj.GetRegNumber(psn_id, PPREGT_GLN, rGLN);
	}
	if(ValidateGLN(rGLN)) {
		ok = 1;
	}
	else {
		SString temp_buf;
		GetObjectName(PPOBJ_ARTICLE, arID, temp_buf);
		PPSetError(PPERR_EDI_ARHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetMainOrgGLN(SString & rGLN)
{
	int    ok = 0;
	rGLN.Z();
	PPID   psn_id = MainOrgID;
	if(psn_id) {
		PsnObj.GetRegNumber(psn_id, PPREGT_GLN, rGLN);
	}
	if(ValidateGLN(rGLN)) {
		ok = 1;
	}
	else {
		SString temp_buf;
		GetObjectName(PPOBJ_PERSON, psn_id, temp_buf);
		PPSetError(PPERR_EDI_MAINORGHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetLocGLN(PPID locID, SString & rGLN)
{
	int    ok = 0;
	rGLN.Z();
	if(locID) {
		RegisterTbl::Rec reg_rec;
		if(PsnObj.LocObj.GetRegister(locID, PPREGT_GLN, ZERODATE, 1, &reg_rec) > 0) {
			rGLN = reg_rec.Num;
		}
	}
	if(ValidateGLN(rGLN)) {
		ok = 1;
	}
	else {
		SString temp_buf;
		GetObjectName(PPOBJ_LOCATION, locID, temp_buf);
		PPSetError(PPERR_EDI_LOCHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetTempOutputPath(int docType, SString & rBuf)
{
	rBuf.Z();
	PPGetPath(PPPATH_TEMP, rBuf);
	rBuf.SetLastSlash().Cat("EDI");
	if(Epp.Rec.Symb[0])
		rBuf.SetLastSlash().Cat(Epp.Rec.Symb);
	if(docType) {
		SString temp_buf;
		PPGetSubStrById(PPTXT_EDIOP, docType, temp_buf);
		if(temp_buf.NotEmpty())
			rBuf.SetLastSlash().Cat(temp_buf);
	}
	rBuf.SetLastSlash();
	return 1;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetTempInputPath(int docType, SString & rBuf)
{
	rBuf.Z();
	return -1;
}

class PPEanComDocument {
public:
	static int FASTCALL GetMsgSymbByType(int msgType, SString & rSymb);
	static int FASTCALL GetMsgTypeBySymb(const char * pSymb);
	static int FASTCALL GetRefqSymb(int refq, SString & rSymb);
	static int FASTCALL GetRefqBySymb(const char * pSymb);
	SLAPI  PPEanComDocument(PPEdiProcessor::ProviderImplementation * pPi);
	SLAPI ~PPEanComDocument();
	int    SLAPI Write_MessageHeader(SXml::WDoc & rDoc, int msgType, const char * pMsgId);
	int    SLAPI Read_MessageHeader(SXml::WDoc & rDoc, SString & rMsgType, SString & rMsgId); // @notimplemented
	//
	// Descr: Коды функций сообщения
	//
	enum {
		fmsgcodeReplace      =  5, // Replace 
		fmsgcodeConfirmation =  6, // Confirmation 
		fmsgcodeDuplicate    =  7, // Duplicate 
		fmsgcodeOriginal     =  9, // Original 
		fmsgcodeProposal     = 16, // Proposal 
		fmsgcodeAcceptedWOA  = 29, // Accepted without amendment 
		fmsgcodeCopy         = 31, // Copy 
		fmsgcodeConfirmationVieSpcMeans = 42, // Confirmation via specific means 
		fmsgcodeProvisional  = 46  // Provisional
	};
	//
	// Коды документов в BeginningOfMessage
	//
	// 220 = Order 
	// 221 = Blanket order 
	// 224 = Rush order 
	// 225 = Repair order 
	// 226 = Call off order 
	// 227 = Consignment order 
	// 22E = Manufacturer raised order (GS1 Temporary Code) 
	// 23E = Manufacturer raised consignment order (GS1 Temporary Code) 
	// 258 = Standing order 
	// 237 = Cross docking services order 
	// 400 = Exceptional order 
	// 401 = Transshipment order 
	// 402 = Cross docking order
	// 
	int    SLAPI Write_BeginningOfMessage(SXml::WDoc & rDoc, const char * pDocCode, const char * pDocIdent, int funcMsgCode);
	int    SLAPI Read_BeginningOfMessage(SXml::WDoc & rDoc, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode); // @notimplemented
	//
	//
	//
	enum {
		dtmDelivery             = 2, // Delivery date/time, requested 
		dtmShipment             = 10, // Shipment date/time, requested 
		dtmDespatch             = 11, // Despatch date and/or time 
		dtmPromotionStart       = 15, // Promotion start date/time 
		dtmDeliveryEstimated    = 17, // Delivery date/time, estimated
		dtmShipNotBefore        = 37, // Ship not before date/time 
		dtmShipNotLater         = 38, // Ship not later than date/time 
		dtmCancelIfNotDelivered = 61, // Cancel if not delivered by this date 
		dtmDeliveryLatest       = 63, // Delivery date/time, latest 
		dtmDeliveryEarliest     = 64, // Delivery date/time, earliest 
		dtmDeliveryPromisedFor  = 69, // Delivery date/time, promised for 
		dtmDeliveryScheduledFor = 76, // Delivery date/time, scheduled for 
		//X14 = Requested for delivery week commencing (GS1 Temporary Code) 
		dtmDocument             = 137, // Document/message date/time 
		dtmReference            = 171, // Reference date/time 
		dtmStart                = 194, // Start date/time 
		dtmCollectionOfCargo    = 200, // Pick-up/collection date/time of cargo 
		dtmEnd                  = 206, // End date/time 
		dtmCollectionLatest     = 235, // Collection date/time, latest 
		dtmInvoicingPeriod      = 263, // Invoicing period 
		dtmValidityPeriod       = 273, // Validity period 
		dtmConfirmation         = 282, // Confirmation date lead time 
		dtmCancelIfNotShipped   = 383, // Cancel if not shipped by this date
		//54E = Stuffing date/time (GS1 Temporary Code)
	};
	enum {
		dtmfmtCCYYMMDD          = 102, // CCYYMMDD 
		dtmfmtCCYYMMDDHHMM      = 203, // CCYYMMDDHHMM 
		dtmfmtCCYYWW            = 616, // CCYYWW 
		dtmfmtCCYYMMDD_CCYYMMDD = 718, // CCYYMMDD-CCYYMMDD
	};
	//
	int    SLAPI Write_DTM(SXml::WDoc & rDoc, int dtmKind, int dtmFmt, const LDATETIME & rDtm, const LDATETIME * pFinish);
	int    SLAPI Read_DTM(SXml::WDoc & rDoc, int * pDtmKind, int * pDtmFmt, LDATETIME * pDtm, LDATETIME * pFinish); // @notimplemented
	enum {
		refqUndef = 0,
		refqAAB = 1, // Proforma invoice number
		refqAAJ, // Delivery order number 
		refqAAK, // Despatch advice number
		refqAAN, // Delivery schedule number 
		refqAAU, // Despatch note number 
		refqAFO, // Beneficiary's reference
		refqAIZ, // Consolidated invoice number
		refqALL, // Message batch number
		refqAMT, // Goods and Services Tax identification number
		refqAPQ, // Commercial account summary reference number
		refqAWT, // Administrative Reference Code 
		refqCD, // Credit note number
		refqCR, // Customer reference number 
		refqCT, // Contract number 
		refqDL, // Debit note number
		refqDQ, // Delivery note number
		refqFC, // Fiscal number Tax payer's number. Number assigned to individual persons as well as to corporates by a public institution; 
			// this number is different from the VAT registration number.
		refqIP, // Import licence number 
		refqIV, // Invoice number
		refqON, // Order number (buyer) 
		refqPK, // Packing list number 
		refqPL, // Price list number
		refqPOR, // Purchase order response number 
		refqPP, // Purchase order change number 
		refqRF, // Export reference number
		refqVN, // Order number (supplier)
		refqXA, // Company/place registration number. Company registration and place as legally required.
	};
	int    SLAPI Write_RFF(SXml::WDoc & rDoc, int refQ, const char * pRef); // reference
	int    SLAPI Read_RFF(SXml::WDoc & rDoc, int * pRefQ, SString & rRef); // reference
	int    SLAPI Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN);
	int    SLAPI Read_NAD(SXml::WDoc & rDoc, int partyQ, SString & rGLN); // @notimplemented
	int    SLAPI Write_CUX(SXml::WDoc & rDoc, const char * pCurrencyCode3);
	int    SLAPI Read_CUX(SXml::WDoc & rDoc, SString & rCurrencyCode3); // @notimplemented
	enum { // values significat!
		amtqAmtDue                =   9, // Amount due/amount payable 
		amtqCashDiscount          =  21, // Cash discount 
		amtqCashOnDeliveryAmt     =  22, // Cash on delivery amount 
		amtqChargeAmt             =  23, // Charge amount 
		amtqInvoiceItemAmt        =  38, // Invoice item amount 
		amtqInvoiceTotalAmt       =  39, // Invoice total amount 
		amtqCustomsValue          =  40, // Customs value 
		amtqFreightCharge         =  64, // Freight charge 
		amtqTotalLnItemsAmt       =  79, // Total line items amount 
		amtqLoadAndHandlCost      =  81, // Loading and handling cost 
		amtqMsgTotalMonetaryAmt   =  86, // Message total monetary amount 
		amtqOriginalAmt           =  98, // Original amount 
		amtqTaxAmt                = 124, // Tax amount 
		amtqTaxableAmt            = 125, // Taxable amount
		amtqTotalAmt              = 128, // Total amount (The amount specified is the total amount)
		amtqTotalServiceCharge    = 140, // Total service charge 
		amtqUnitPrice             = 146, // Unit price (5110) Reporting monetary amount is a "per unit" amount. (цена без НДС)
		amtqMsgTotalDutyTaxFeeAmt = 176, // Message total duty/tax/fee amount 
		amtqExactAmt              = 178, // Exact amount 
		amtqLnItemAmt             = 203, // Line item amount 
		amtqAllowanceAmt          = 204, // Allowance amount
		amtqTotalCharges          = 259, // Total charges 
		amtqTotalAllowances       = 260, // Total allowances
		amtqInstalmentAmt         = 262, // Instalment amount
		amtGoodsAndServicesTax    = 369, // Goods and services tax 
		amtqTotalAmtInclVAT       = 388, // Total amount including Value Added Tax (VAT)
		amtqCalcBasisExclAllTaxes = 528, // Calculation basis excluding all taxes 
		amtqUnloadAndHandlCost    = 542, // Unloading and handling cost
	};
	int    SLAPI Write_MOA(SXml::WDoc & rDoc, int amtQ, double amount);
	int    SLAPI Read_MOA(SXml::WDoc & rDoc, int * pAmtQ, double * pAmt); // @notimplemented
	// 
	// 1 = Discrete quantity 
	// 11 = Split quantity
	// 12 = Despatch quantity 
	// 21 = Ordered quantity 
	// 52 = Quantity per pack 
	// 59 = Number of consumer units in the traded unit 
	// 61 = Return quantity 
	// 121 = Over shipped 
	// 164 = Delivery batch 
	// 192 = Free goods quantity 
	// 193 = Free quantity included 
	// 17E = Number of units in lower packaging or configuration level (GS1 Temporary Code) 
	// 246 = Returns replacement quantity 
	// 45E = Number of units in higher packaging or configuration level (GS1 Temporary Code)
	// 
	int    SLAPI Write_QTY(SXml::WDoc & rDoc, PPID goodsID, int qtyQ, double qtty);
	int    SLAPI Read_QTY(SXml::WDoc & rDoc, int * pAmtQ, double * pAmt); // @notimplemented
	enum {
		cntqQuantity       = 1, // Algebraic total of the quantity values in line items in a message
		cntqNumOfLn        = 2, // Number of line items in message
		cntqNumOfLnAndSub  = 3, // Number of line and sub items in message
		cntqNumOfInvcLn    = 4, // Number of invoice lines
	};
	int    SLAPI Write_CNT(SXml::WDoc & rDoc, int countQ, double value);
	int    SLAPI Read_CNT(SXml::WDoc & rDoc, int * pCountQ, double * pValue);
	enum {
		// Use the codes AAH, AAQ, ABL, ABM when dealing with CSA (customer specific articles).
		priceqAAA = 1, // Calculation net. The price stated is the net price including all allowances and charges and excluding taxes. 
			// Allowances and charges may be stated for information purposes only. 
		priceqAAE,     // Information price, excluding allowances or charges, including taxes 
		priceqAAF,     // Information price, excluding allowances or charges and taxes 
		priceqAAH,     // Subject to escalation and price adjustment 
		priceqAAQ,     // Firm price  
		priceqABL,     // Base price  
		priceqABM      // Base price difference 
	};
	int    SLAPI Write_PRI(SXml::WDoc & rDoc, int priceQ, double amount);
	int    SLAPI Read_PRI(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt); // @notimplemented
	enum {
		taxqCustomDuty = 5,
		taxqTax        = 7
	};
	enum {
		taxtGST = 1, // GST = Goods and services tax
		taxtIMP,     // IMP = Import tax 
		taxtVAT      // VAT = Value added tax
	};
	//
	// Descr: Записывает сегмент определения налога.
	// Note: Пока функция заточена только на НДС.
	//
	int    SLAPI Write_TAX(SXml::WDoc & rDoc, int taxQ, int taxT, double value);
	int    SLAPI Read_TAX(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt); // @notimplemented
	int    SLAPI Write_LIN(SXml::WDoc & rDoc, int lineN, const char * pGoodsCode);
	int    SLAPI Read_LIN(SXml::WDoc & rDoc, int * pLineN, SString & rGoodsCode);
	int    SLAPI Write_PIA(SXml::WDoc & rDoc, const char * pGoodsCode);
	int    SLAPI Read_PIA(SXml::WDoc & rDoc, SString & rGoodsCode);
	int    SLAPI Write_IMD(SXml::WDoc & rDoc, const char * pDescription);
	int    SLAPI Read_IMD(SXml::WDoc & rDoc, SString & rDescription);

	struct BillGoodsItemsTotal {
		BillGoodsItemsTotal() : Count(0), SegCount(0), Quantity(0.0), AmountWoTax(0.0), AmountWithTax(0.0)
		{
		}
		uint   Count; // Количество строк
		uint   SegCount; // Количество сегментов сообщения
		double Quantity;
		double AmountWoTax;
		double AmountWithTax;
	};
	int    SLAPI Write_DesadvGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal);
	int    SLAPI Read_DesadvGoodsItem(SXml::WDoc & rDoc, PPTransferItem & rTi); // @notimplemented
	int    SLAPI Write_OrderGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal);
	int    SLAPI Read_OrderGoodsItem(SXml::WDoc & rDoc, PPTransferItem & rTi); // @notimplemented
	//
	//
	//
	int    SLAPI Write_DESADV(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    SLAPI Write_ORDERS(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    SLAPI Read_DESADV(const PPBillPacket & rPack);
	int    SLAPI Read_Document();
private:
	PPEdiProcessor::ProviderImplementation * P_Pi;
};

SLAPI PPEanComDocument::PPEanComDocument(PPEdiProcessor::ProviderImplementation * pPi) : P_Pi(pPi)
{
}

SLAPI PPEanComDocument::~PPEanComDocument()
{
}

int SLAPI PPEanComDocument::Write_MessageHeader(SXml::WDoc & rDoc, int msgType, const char * pMsgId)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_unh(rDoc, "UNH"); // Message header
	n_unh.PutInner("E0062", temp_buf.Z().Cat(pMsgId)); // ИД сообщения
	{
		SXml::WNode n_i(rDoc, "S009");
		THROW(GetMsgSymbByType(msgType, temp_buf));
		n_i.PutInner("E0065", temp_buf); // Тип сообщения
		n_i.PutInner("E0052", "D"); // Версия сообщения
		n_i.PutInner("E0054", "01B"); // Версия выпуска
		{
			const char * p_e0057_code = 0;
			if(msgType == PPEDIOP_ORDER)
				p_e0057_code = "EAN010";
			else if(msgType == PPEDIOP_DESADV)
				p_e0057_code = "EAN007";
			if(p_e0057_code) {
				n_i.PutInner("E0051", "UN"); // Код ведущей организации
				n_i.PutInner("E0057", p_e0057_code); // Код, присвоенный ведущей организацией
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_MessageHeader(SXml::WDoc & rDoc, SString & rMsgType, SString & rMsgId) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_BeginningOfMessage(SXml::WDoc & rDoc, const char * pDocCode, const char * pDocIdent, int funcMsgCode)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_bgm(rDoc, "BGM"); // Beginning of message
	{
		{
			SXml::WNode n_i(rDoc, "C002"); // Имя документа/сообщения
			(temp_buf = pDocCode).Transf(CTRANSF_INNER_TO_UTF8);
			n_i.PutInner("E1001", temp_buf); // Код документа 
		}
		{
			SXml::WNode n_i(rDoc, "C106"); // Идентификация документа/сообщения
			temp_buf = pDocIdent;
			THROW_PP_S(temp_buf.Len() > 0 && temp_buf.Len() <= 17, PPERR_EDI_DOCIDENTLEN, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			n_i.PutInner("E1004", temp_buf); // Номер документа (Должно быть максимум 17 символов)
		}
		n_bgm.PutInner("E1225", temp_buf.Z().Cat(funcMsgCode)); // Код функции сообщения
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_BeginningOfMessage(SXml::WDoc & rDoc, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_DTM(SXml::WDoc & rDoc, int dtmKind, int dtmFmt, const LDATETIME & rDtm, const LDATETIME * pFinish)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_dtm(rDoc, "DTM"); // Дата документа
	THROW(oneof3(dtmFmt, dtmfmtCCYYMMDD, dtmfmtCCYYMMDDHHMM, dtmfmtCCYYMMDD_CCYYMMDD));
	THROW(dtmFmt != dtmfmtCCYYMMDD_CCYYMMDD || pFinish);
	{
		SXml::WNode n_i(rDoc, "C507");
		n_i.PutInner("E2005", temp_buf.Z().Cat(dtmKind)); // Квалификатор функции даты-времени (Дата/время документа/сообщения)
		temp_buf.Z();
		if(dtmFmt == dtmfmtCCYYMMDD) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV);
		}
		else if(dtmFmt == dtmfmtCCYYMMDDHHMM) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(rDtm.t, TIMF_HM);
		}
		else if(dtmFmt == dtmfmtCCYYMMDD_CCYYMMDD) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).CatChar('-').Cat(pFinish->d, DATF_YMD|DATF_CENTURY|DATF_NODIV);
		}
		n_i.PutInner("E2380", temp_buf); // Дата или время, или период
		n_i.PutInner("E2379", temp_buf.Z().Cat(dtmFmt)); // Формат даты/времени (CCYYMMDD)
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_DTM(SXml::WDoc & rDoc, int * pDtmKind, int * pDtmFmt, LDATETIME * pDtm, LDATETIME * pFinish) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_RFF(SXml::WDoc & rDoc, int refQ, const char * pRef) // reference
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode n_rff(rDoc, "RFF");
		{
			SXml::WNode n_c506(rDoc, "C506");
			THROW(GetRefqSymb(refQ, temp_buf));
			n_c506.PutInner("E1153", temp_buf);
			(temp_buf = pRef).Transf(CTRANSF_INNER_TO_UTF8);
			n_c506.PutInner("E1154", temp_buf);
		}
	}
	CATCHZOK
	return ok;
}
int SLAPI PPEanComDocument::Read_RFF(SXml::WDoc & rDoc, int * pRefQ, SString & rRef) // reference
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_nad(rDoc, "NAD"); // Наименование и адрес
	THROW_PP(!isempty(pGLN), PPERR_EDI_GLNISEMPTY);
	{
		/*	 
			Party function code qualifier:
			BY = Buyer 
			CO = Corporate office 
			DP = Delivery party 
			IV = Invoicee 
			SN = Store number 
			SR = Supplier's agent/representative 
			SU = Supplier 
			WH = Warehouse keeper

			EDIPARTYQ_XXX
		*/
		switch(partyQ) {
			case EDIPARTYQ_BUYER: temp_buf = "BY"; break;
			case EDIPARTYQ_CORPOFFICE: temp_buf = "CO"; break;
			case EDIPARTYQ_DELIVERY: temp_buf = "DP"; break;
			case EDIPARTYQ_INVOICEE: temp_buf = "IV"; break;
			case EDIPARTYQ_STORENUMBER: temp_buf = "SN"; break;
			case EDIPARTYQ_SUPPLAGENT: temp_buf = "SR"; break;
			case EDIPARTYQ_SUPPLIER: temp_buf = "SU"; break;
			case EDIPARTYQ_WAREHOUSEKEEPER: temp_buf = "WH"; break;
			case EDIPARTYQ_CONSIGNOR: temp_buf = "CZ"; break;
			default:
				CALLEXCEPT_PP_S(PPERR_EDI_INVPARTYQ, (long)partyQ);

		}
		n_nad.PutInner("E3035", temp_buf);
		{
			SXml::WNode n_i(rDoc, "C082"); // Детали стороны
			n_i.PutInner("E3039", pGLN); // GLN стороны
			n_i.PutInner("E3055", "9"); // Код ведущей организации - EAN (Международная ассоциация товарной нумерации)
		}
	}
	CATCHZOK
	return ok;
}
int SLAPI PPEanComDocument::Read_NAD(SXml::WDoc & rDoc, int partyQ, SString & rGLN) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_CUX(SXml::WDoc & rDoc, const char * pCurrencyCode3)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_cux(rDoc, "CUX"); 
	THROW_PP(!isempty(pCurrencyCode3), PPERR_EDI_CURRCODEISEMPTY);
	{
		SXml::WNode n_i(rDoc, "C504");
		n_i.PutInner("E6347", "2");
		n_i.PutInner("E6345", pCurrencyCode3);
		n_i.PutInner("E3055", "9");
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_CUX(SXml::WDoc & rDoc, SString & rCurrencyCode3) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_MOA(SXml::WDoc & rDoc, int amtQ, double amount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_moa(rDoc, "MOA"); 
	{
		SXml::WNode n_i(rDoc, "C516");
		n_i.PutInner("E5025", temp_buf.Z().Cat(amtQ)); // Квалификатор суммы товарной позиции
		n_i.PutInner("E5004", temp_buf.Z().Cat(amount, MKSFMTD(0, 2, 0))); // Сумма (Число знаков после запятой - не больше 2)
	}
	return ok;
}
	
int SLAPI PPEanComDocument::Read_MOA(SXml::WDoc & rDoc, int * pAmtQ, double * pAmt) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_QTY(SXml::WDoc & rDoc, PPID goodsID, int qtyQ, double qtty)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_qty(rDoc, "QTY"); 
	{
		SXml::WNode n_i(rDoc, "C186");
		n_i.PutInner("E6063", temp_buf.Z().Cat(qtyQ)); // Квалификатор количества
		SString unit_buf = "PCE";
		double unit_scale = 1.0;
		Goods2Tbl::Rec goods_rec;
		if(goodsID && P_Pi->GObj.Fetch(goodsID, &goods_rec) > 0) {
			PPUnit u_rec;
			if(P_Pi->GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
				if(u_rec.ID == PPUNT_KILOGRAM)
					unit_buf = "KGM";
				else if(u_rec.BaseUnitID == PPUNT_KILOGRAM && u_rec.BaseRatio > 0.0) {
					unit_buf = "KGM";
					unit_scale = u_rec.BaseRatio;
				}
			}
		}
		n_i.PutInner("E6060", temp_buf.Z().Cat(qtty * unit_scale, MKSFMTD(0, 6, NMBF_NOTRAILZ))); 
		n_i.PutInner("E6411", unit_buf); 
	}
	return ok;
}

int SLAPI PPEanComDocument::Read_QTY(SXml::WDoc & rDoc, int * pAmtQ, double * pAmt) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_CNT(SXml::WDoc & rDoc, int countQ, double value)
{
	int    ok = 1;
	SXml::WNode n_cnt(rDoc, "CNT");
	{
		SString temp_buf;
		SXml::WNode n_c270(rDoc, "C270");
		n_c270.PutInner("E6069", temp_buf.Z().Cat(countQ));
		n_c270.PutInner("E6066", temp_buf.Z().Cat(value, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
	}
	return ok;
}
	
int SLAPI PPEanComDocument::Read_CNT(SXml::WDoc & rDoc, int * pCountQ, double * pValue)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_PRI(SXml::WDoc & rDoc, int priceQ, double amount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_pri(rDoc, "PRI"); 
	{
		SXml::WNode n_i(rDoc, "C509");
		switch(priceQ) {
			case priceqAAA: temp_buf = "AAA"; break;
			case priceqAAE: temp_buf = "AAE"; break;
			case priceqAAF: temp_buf = "AAF"; break;
			case priceqAAH: temp_buf = "AAH"; break;
			case priceqAAQ: temp_buf = "AAQ"; break;
			case priceqABL: temp_buf = "ABL"; break;
			case priceqABM: temp_buf = "ABM"; break;
			default:
				CALLEXCEPT_PP_S(PPERR_EDI_INVPRICEQ, (long)priceQ);
		}
		n_i.PutInner("E5125", temp_buf); // Квалификатор цены
		n_i.PutInner("E5118", temp_buf.Z().Cat(amount, MKSFMTD(0, 2, 0))); 
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_PRI(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_TAX(SXml::WDoc & rDoc, int taxQ, int taxT, double value)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_tax(rDoc, "TAX"); 
	THROW_PP_S(oneof2(taxQ, taxqCustomDuty, taxqTax), PPERR_EDI_INVTAXQ, (long)taxQ);
	n_tax.PutInner("E5283", temp_buf.Z().Cat(taxQ)); // Квалификатор налога
	{
		// Тип налога
		switch(taxT) {
			case taxtGST: temp_buf = "GST"; break;
			case taxtIMP: temp_buf = "IMP"; break;
			case taxtVAT: temp_buf = "VAT"; break;
			default:
				CALLEXCEPT_PP_S(PPERR_EDI_INVTAXTYPE, (long)taxT);
		}
		SXml::WNode n_c241(rDoc, "C241");
		n_c241.PutInner("E5153", temp_buf); 
	}
	{
		SXml::WNode n_c243(rDoc, "C243");
		n_c243.PutInner("E5278", temp_buf.Z().Cat(value)); // Ставка 
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_TAX(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_LIN(SXml::WDoc & rDoc, int lineN, const char * pGoodsCode)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_lin(rDoc, "LIN");
	n_lin.PutInner("E1082", temp_buf.Z().Cat(lineN));
	{
		SXml::WNode n_c212(rDoc, "C212");
		n_c212.PutInner("E7140", pGoodsCode); // Штрих-код товара
		n_c212.PutInner("E7143", "SRV"); // Тип штрихкода EAN.UCC
	}
	return ok;
}

int SLAPI PPEanComDocument::Read_LIN(SXml::WDoc & rDoc, int * pLineN, SString & rGoodsCode)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_PIA(SXml::WDoc & rDoc, const char * pGoodsCode)
{
	int    ok = 1;
	if(!isempty(pGoodsCode)) {
		SString temp_buf;
		SXml::WNode n_pia(rDoc, "PIA"); // Дополнительный идентификатор товара
		n_pia.PutInner("E4347", "1"); // Дополнительный идентификатор
		{
			SXml::WNode n_c212(rDoc, "C212");
			(temp_buf = pGoodsCode).Transf(CTRANSF_INNER_TO_UTF8);
			n_c212.PutInner("E7140", temp_buf); // Артикул
			n_c212.PutInner("E7143", "SA"); // Идентификатор артикула поставщика
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPEanComDocument::Read_PIA(SXml::WDoc & rDoc, SString & rGoodsCode)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_IMD(SXml::WDoc & rDoc, const char * pDescription)
{
	int    ok = 1;
	if(!isempty(pDescription)) {
		SXml::WNode n_imd(rDoc, "IMD"); // Описание товара
		n_imd.PutInner("E7077", "F"); // Код формата описания (текст)
		{
			SString temp_buf;
			SXml::WNode n_c273(rDoc, "C273"); // Описание
			n_c273.PutInner("E7008", SXml::WNode::CDATA((temp_buf = pDescription).Transf(CTRANSF_INNER_TO_UTF8)));
		}
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPEanComDocument::Read_IMD(SXml::WDoc & rDoc, SString & rDescription)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_DesadvGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal)
{
	int    ok = 1;
	const  double qtty = fabs(rTi.Qtty());
	SString temp_buf;
	SString goods_code;
	SString goods_ar_code;
	Goods2Tbl::Rec goods_rec;
	BarcodeArray bc_list;
	/*
		AC = HIBC (Health Industry Bar Code) 
		DW = Drawing 
		IB = ISBN (International Standard Book Number)
		IN = Buyer's item number 
		SA = Supplier's article number 
		SRV = GS1 Global Trade Item Number 
		EWC = European Waste Catalogue (GS1 Temporary Code) 
		UA = Ultimate customer's article number
	*/
	THROW(qtty > 0.0); // @todo error (бессмысленная строка с нулевым количеством, но пока не ясно, что с ней делать - возможно лучше просто пропустить с замечанием).
	THROW(P_Pi->GObj.Search(rTi.GoodsID, &goods_rec) > 0);
	P_Pi->GObj.P_Tbl->ReadBarcodes(rTi.GoodsID, bc_list);
	for(uint bcidx = 0; goods_code.Empty() && bcidx < bc_list.getCount(); bcidx++) {
		int    d = 0;
		int    std = 0;
		const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
		if(P_Pi->GObj.DiagBarcode(r_bc_item.Code, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE)) {
			goods_code = r_bc_item.Code;
		}
	}
	THROW_PP_S(goods_code.NotEmpty(), PPERR_EDI_WAREHASNTVALIDCODE, goods_rec.Name);
	{
		THROW(Write_LIN(rDoc, rTi.RByBill, goods_code));
		rTotal.SegCount++;
		if(goods_ar_code.NotEmpty()) {
			THROW(Write_PIA(rDoc, goods_ar_code));
			rTotal.SegCount++;
		}
		THROW(Write_IMD(rDoc, goods_rec.Name));
		rTotal.SegCount++;
		THROW(Write_QTY(rDoc, rTi.GoodsID, 21, qtty));
		rTotal.SegCount++;
		rTotal.Quantity += qtty;
		{
			GTaxVect vect;
			vect.CalcTI(&rTi, 0 /*opID*/, tiamt);
			const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
			const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
			const double vat_rate = vect.GetTaxRate(GTAXVF_VAT, 0);
			const double price_with_vat = R5(amount_with_vat / qtty);
			const double price_without_vat = R5(amount_without_vat / qtty);
			rTotal.AmountWithTax += amount_with_vat;
			rTotal.AmountWoTax += amount_without_vat;
			THROW(Write_MOA(rDoc, amtqUnitPrice/*146*/, price_without_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqTotalLnItemsAmt/*79*/, amount_with_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqLnItemAmt/*203*/, amount_without_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqTaxAmt/*124*/, amount_with_vat - amount_without_vat));
			rTotal.SegCount++;
		}
	}
	rTotal.Count++;
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_DesadvGoodsItem(SXml::WDoc & rDoc, PPTransferItem & rTi) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_OrderGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal)
{
	int    ok = 1;
	const  double qtty = fabs(rTi.Qtty());
	SString temp_buf;
	SString goods_code;
	SString goods_ar_code;
	Goods2Tbl::Rec goods_rec;
	BarcodeArray bc_list;
	/*
		AC = HIBC (Health Industry Bar Code) 
		DW = Drawing 
		IB = ISBN (International Standard Book Number)
		IN = Buyer's item number 
		SA = Supplier's article number 
		SRV = GS1 Global Trade Item Number 
		EWC = European Waste Catalogue (GS1 Temporary Code) 
		UA = Ultimate customer's article number
	*/
	THROW(qtty > 0.0); // @todo error (бессмысленная строка с нулевым количеством, но пока не ясно, что с ней делать - возможно лучше просто пропустить с замечанием).
	THROW(P_Pi->GObj.Search(rTi.GoodsID, &goods_rec) > 0);
	P_Pi->GObj.P_Tbl->ReadBarcodes(rTi.GoodsID, bc_list);
	for(uint bcidx = 0; goods_code.Empty() && bcidx < bc_list.getCount(); bcidx++) {
		int    d = 0;
		int    std = 0;
		const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
		if(P_Pi->GObj.DiagBarcode(r_bc_item.Code, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE)) {
			goods_code = r_bc_item.Code;
		}
	}
	THROW_PP_S(goods_code.NotEmpty(), PPERR_EDI_WAREHASNTVALIDCODE, goods_rec.Name);
	{
		THROW(Write_LIN(rDoc, rTi.RByBill, goods_code));
		rTotal.SegCount++;
		if(goods_ar_code.NotEmpty()) {
			THROW(Write_PIA(rDoc, goods_ar_code));
			rTotal.SegCount++;
		}
		THROW(Write_IMD(rDoc, goods_rec.Name));
		rTotal.SegCount++;
		THROW(Write_QTY(rDoc, rTi.GoodsID, 21, qtty));
		rTotal.SegCount++;
		{
			GTaxVect vect;
			vect.CalcTI(&rTi, 0 /*opID*/, tiamt);
			const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
			const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
			const double vat_rate = vect.GetTaxRate(GTAXVF_VAT, 0);
			const double price_with_vat = R5(amount_with_vat / qtty);
			const double price_without_vat = R5(amount_without_vat / qtty);
			rTotal.AmountWithTax += amount_with_vat;
			rTotal.AmountWoTax += amount_without_vat;
			THROW(Write_MOA(rDoc, amtqTotalLnItemsAmt, amount_with_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqLnItemAmt, amount_without_vat));
			rTotal.SegCount++;
			{
				SXml::WNode n_sg32(rDoc, "SG32"); // Цена товара с НДС
				THROW(Write_PRI(rDoc, priceqAAE, price_with_vat));
				rTotal.SegCount++;
			}
			{
				SXml::WNode n_sg32(rDoc, "SG32"); // Цена товара без НДС
				THROW(Write_PRI(rDoc, priceqAAA, price_without_vat));
				rTotal.SegCount++;
			}
			{
				SXml::WNode n_sg38(rDoc, "SG38"); // Ставка НДС
				THROW(Write_TAX(rDoc, taxqTax, taxtVAT, vat_rate));
				rTotal.SegCount++;
			}
		}
	}
	rTotal.Count++;
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Read_OrderGoodsItem(SXml::WDoc & rDoc, PPTransferItem & rTi) // @notimplemented
{
	int    ok = 0;
	return ok;
}

/*int SLAPI PPEanComDocument::Helper_Read(void * pCtx, const char * pFileName, )
{
	int    ok = 0;
	return ok;
}*/

int SLAPI PPEanComDocument::Read_DESADV(const PPBillPacket & rPack)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_DESADV(xmlTextWriter * pX, const PPBillPacket & rPack)
{
	int    ok = 1;
	const  int edi_op = PPEDIOP_DESADV;
	uint   seg_count = 0;
	SString temp_buf;
	SString bill_ident;
	LDATETIME dtm;
	BillGoodsItemsTotal items_total;
	if(rPack.BTagL.GetItemStr(PPTAG_BILL_UUID, temp_buf) > 0)
		bill_ident = temp_buf;
	else
		bill_ident.Z().Cat(rPack.Rec.ID);
	{
		SXml::WDoc _doc(pX, cpUTF8);
		SXml::WNode n_docs(_doc, "DESADV");
		n_docs.PutAttrib("version", "1.07");
		{
			THROW(Write_MessageHeader(_doc, edi_op, bill_ident)); // "UNH" Message header
			seg_count++;
			BillCore::GetCode(temp_buf = rPack.Rec.Code);
			THROW(Write_BeginningOfMessage(_doc, "351", temp_buf, fmsgcodeOriginal)); // "BGM" Beginning of message
			seg_count++;
			dtm.Set(rPack.Rec.Dt, ZEROTIME);
			THROW(Write_DTM(_doc, dtmDocument, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
			seg_count++;
			if(rPack.P_Freight) {
				if(checkdate(rPack.P_Freight->IssueDate, 0)) {
					dtm.Set(rPack.P_Freight->IssueDate, ZEROTIME);
					THROW(Write_DTM(_doc, dtmDespatch, dtmfmtCCYYMMDD, dtm, 0));
					seg_count++;
				}
				if(checkdate(rPack.P_Freight->ArrivalDate, 0)) {
					dtm.Set(rPack.P_Freight->ArrivalDate, ZEROTIME);
					THROW(Write_DTM(_doc, dtmDeliveryEstimated, dtmfmtCCYYMMDD, dtm, 0));
					seg_count++;
				}
			}
			// amtqAmtDue amtqOriginalAmt amtqTaxableAmt amtqTaxAmt
			THROW(Write_MOA(_doc, amtqAmtDue, rPack.Rec.Amount));
			seg_count++;
			THROW(Write_MOA(_doc, amtqOriginalAmt, 0.0));
			seg_count++;
			THROW(Write_MOA(_doc, amtqTaxableAmt, 0.0));
			seg_count++;
			THROW(Write_MOA(_doc, amtqTaxAmt, 0.0));
			seg_count++;
			{
				PPIDArray order_id_list;
				rPack.GetOrderList(order_id_list);
				for(uint ordidx = 0; ordidx < order_id_list.getCount(); ordidx++) {
					const PPID ord_id = order_id_list.get(ordidx);
					BillTbl::Rec ord_rec;
					if(ord_id && BillObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
						BillCore::GetCode(temp_buf = ord_rec.Code);
						LDATETIME ord_dtm;
						ord_dtm.Set(ord_rec.Dt, ZEROTIME);
						{
							SXml::WNode n_sg1(_doc, "SG1");
							THROW(Write_RFF(_doc, refqON, temp_buf));
							if(checkdate(ord_dtm.d, 0)) {
								THROW(Write_DTM(_doc, dtmReference, dtmfmtCCYYMMDD, ord_dtm, 0));
							}
						}
					}
				}
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				THROW(P_Pi->GetMainOrgGLN(temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_SUPPLIER, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_BUYER, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				if(rPack.P_Freight && rPack.P_Freight->DlvrAddrID) {
					THROW(P_Pi->GetLocGLN(rPack.P_Freight->DlvrAddrID, temp_buf));
				}
				else {
					THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				}
				THROW(Write_NAD(_doc, EDIPARTYQ_DELIVERY, temp_buf));
			}
			{
				SXml::WNode n_sg2(_doc, "SG2"); 
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_INVOICEE, temp_buf));
			}
			{
				SXml::WNode n_sg10(_doc, "SG10"); 
				{
					SXml::WNode n_cps(_doc, "CPS");
					n_cps.PutInner("E7164", "1"); // Номер иерархии по умолчанию - 1
				}
				{	
					for(uint i = 0; i < rPack.GetTCount(); i++) {
						const PPTransferItem & r_ti = rPack.ConstTI(i);
						SXml::WNode n_sg17(_doc, "SG17"); // maxOccurs="200000" LIN-PIA-IMD-MEA-QTY-ALI-DTM-MOA-GIN-QVR-FTX-SG32-SG33-SG34-SG37-SG38-SG39-SG43-SG49 
						// A group of segments providing details of the individual ordered items. This Segment group may be repeated to give sub-line details.
						THROW(Write_DesadvGoodsItem(_doc, edi_op, r_ti, TIAMT_PRICE, items_total));
					}
					seg_count += items_total.SegCount;
				}
			}
			THROW(Write_CNT(_doc, cntqQuantity, items_total.Quantity));
			seg_count++;
			THROW(Write_CNT(_doc, cntqNumOfLn, items_total.Count));
			seg_count++;
			{
				seg_count++;
				SXml::WNode n_unt(_doc, "UNT"); // Окончание сообщения
				n_unt.PutInner("E0074", temp_buf.Z().Cat(seg_count)); // Общее число сегментов в сообщении
				n_unt.PutInner("E0062", bill_ident); // Номер электронного сообщения (совпадает с указанным в заголовке)
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEanComDocument::Write_ORDERS(xmlTextWriter * pX, const PPBillPacket & rPack)
{
	int    ok = 1;
	const  int edi_op = PPEDIOP_ORDER;
	uint   seg_count = 0;
	SString temp_buf;
	SString fmt;
	SString bill_ident;
	LDATETIME dtm;
	//THROW_PP(pX, IEERR_NULLWRIEXMLPTR);
	BillGoodsItemsTotal items_total;
	if(rPack.BTagL.GetItemStr(PPTAG_BILL_UUID, temp_buf) > 0)
		bill_ident = temp_buf;
	else
		bill_ident.Z().Cat(rPack.Rec.ID);
	{
		SXml::WDoc _doc(pX, cpUTF8);
		SXml::WNode n_docs(_doc, "ORDERS");
		n_docs.PutAttrib("version", "1.07");
		{
			THROW(Write_MessageHeader(_doc, edi_op, bill_ident)); // "UNH" Message header
			seg_count++;
			BillCore::GetCode(temp_buf = rPack.Rec.Code);
			THROW(Write_BeginningOfMessage(_doc, "220", temp_buf, fmsgcodeOriginal)); // "BGM" Beginning of message
			seg_count++;
			dtm.Set(rPack.Rec.Dt, ZEROTIME);
			THROW(Write_DTM(_doc, dtmDocument, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
			seg_count++;
			if(checkdate(rPack.Rec.DueDate, 0)) {
				dtm.Set(rPack.Rec.DueDate, ZEROTIME);
				THROW(Write_DTM(_doc, dtmDelivery, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
				seg_count++;
			}
			//SXml::WNode n_sg1(_doc, "SG1"); // RFF-DTM // minOccurs="0" maxOccurs="9999"
			{ // Поставщик
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_SUPPLIER, temp_buf));
				seg_count++;
			}
			{ // Грузоотправитель
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_CONSIGNOR, temp_buf));
				seg_count++;
			}
			{ // Покупатель
				THROW(P_Pi->GetMainOrgGLN(temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_BUYER, temp_buf));
				seg_count++;
			}
			{ // Адрес доставки
				THROW(P_Pi->GetLocGLN(rPack.Rec.LocID, temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_DELIVERY, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg7(_doc, "SG7"); // CUX-DTM // minOccurs="0" maxOccurs="5"
				THROW(Write_CUX(_doc, "RUB"));
				seg_count++;
			}
			{	
				for(uint i = 0; i < rPack.GetTCount(); i++) {
					const PPTransferItem & r_ti = rPack.ConstTI(i);
					SXml::WNode n_sg28(_doc, "SG28"); // maxOccurs="200000" LIN-PIA-IMD-MEA-QTY-ALI-DTM-MOA-GIN-QVR-FTX-SG32-SG33-SG34-SG37-SG38-SG39-SG43-SG49 
					// A group of segments providing details of the individual ordered items. This Segment group may be repeated to give sub-line details.
					THROW(Write_OrderGoodsItem(_doc, edi_op, r_ti, TIAMT_COST, items_total));
				}
				seg_count += items_total.SegCount;
			}
			{
				SXml::WNode n_uns(_doc, "UNS"); // Разделитель зон
				n_uns.PutInner("E0081", "S"); // Идентификатор секции (Зона итоговой информации)
				seg_count++;
			}
			THROW(Write_MOA(_doc, amtqTotalAmt, items_total.AmountWithTax));
			seg_count++;
			THROW(Write_MOA(_doc, amtqOriginalAmt, items_total.AmountWoTax));
			seg_count++;
			THROW(Write_CNT(_doc, cntqNumOfLn, items_total.Count));
			seg_count++;
			{
				seg_count++;
				SXml::WNode n_unt(_doc, "UNT"); // Окончание сообщения
				n_unt.PutInner("E0074", temp_buf.Z().Cat(seg_count)); // Общее число сегментов в сообщении
				n_unt.PutInner("E0062", bill_ident); // Номер электронного сообщения (совпадает с указанным в заголовке)
			}
		}
	}
	CATCHZOK
	return ok;
}

struct EdiIdSymb {
	int    Id;
	const  char * P_Symb;	
};

static int FASTCALL __EdiGetSymbById(const EdiIdSymb * pTab, size_t tabSize, int msgType, SString & rSymb)
{
	rSymb.Z();
	int    ok = 0;
	for(uint i = 0; !ok && i < tabSize; i++) {
		if(pTab[i].Id == msgType) {
			rSymb = pTab[i].P_Symb;
			ok = 1;
		}
	}
	return ok;
}

static int FASTCALL __EdiGetIdBySymb(const EdiIdSymb * pTab, size_t tabSize, const char * pSymb)
{
	int    id = 0;
	for(uint i = 0; !id && i < tabSize; i++) {
		if(sstreqi_ascii(pSymb, pTab[i].P_Symb))
			id = pTab[i].Id;
	}
	return id;
}

static const EdiIdSymb EdiMsgTypeSymbols_EanCom[] = {
	{ PPEDIOP_ORDER,        "ORDERS" },
	{ PPEDIOP_ORDERRSP,     "ORDRSP" },
	{ PPEDIOP_APERAK,       "APERAK" },
	{ PPEDIOP_DESADV,       "DESADV" },
	{ PPEDIOP_DECLINEORDER, "DECLNORDER" },
	{ PPEDIOP_RECADV,       "RECADV" },
	{ PPEDIOP_ALCODESADV,   "ALCODESADV" }
};

//static 
int FASTCALL PPEanComDocument::GetMsgSymbByType(int msgType, SString & rSymb)
	{ return __EdiGetSymbById(EdiMsgTypeSymbols_EanCom, SIZEOFARRAY(EdiMsgTypeSymbols_EanCom), msgType, rSymb); }
//static 
int FASTCALL PPEanComDocument::GetMsgTypeBySymb(const char * pSymb)
	{ return __EdiGetIdBySymb(EdiMsgTypeSymbols_EanCom, SIZEOFARRAY(EdiMsgTypeSymbols_EanCom), pSymb); }

static const EdiIdSymb EanComRefQSymbList[] = {
	{ PPEanComDocument::refqAAB, "AAB" },
	{ PPEanComDocument::refqAAJ, "AAJ" },
	{ PPEanComDocument::refqAAK, "AAK" },
	{ PPEanComDocument::refqAAN, "AAN" },
	{ PPEanComDocument::refqAAU, "AAU" },
	{ PPEanComDocument::refqAFO, "AFO" },
	{ PPEanComDocument::refqAIZ, "AIZ" },
	{ PPEanComDocument::refqALL, "ALL" },
	{ PPEanComDocument::refqAMT, "AMT" },
	{ PPEanComDocument::refqAPQ, "APQ" },
	{ PPEanComDocument::refqAWT, "AWT" },
	{ PPEanComDocument::refqCD,  "CD"  },
	{ PPEanComDocument::refqCR,  "CR"  },
	{ PPEanComDocument::refqCT,  "CT"  },
	{ PPEanComDocument::refqDL,  "DL"  },
	{ PPEanComDocument::refqDQ,  "DQ"  },
	{ PPEanComDocument::refqFC,  "FC"  },
	{ PPEanComDocument::refqIP,  "IP"  },
	{ PPEanComDocument::refqIV,  "IV"  },
	{ PPEanComDocument::refqON,  "ON"  },
	{ PPEanComDocument::refqPK,  "PK"  },
	{ PPEanComDocument::refqPL,  "PL"  },
	{ PPEanComDocument::refqPOR, "POR" },
	{ PPEanComDocument::refqPP,  "PP"  },
	{ PPEanComDocument::refqRF,  "RF"  },
	{ PPEanComDocument::refqVN,  "VN"  },
	{ PPEanComDocument::refqXA,  "XA"  },
};

//static 
int FASTCALL PPEanComDocument::GetRefqSymb(int refq, SString & rSymb)
	{ return __EdiGetSymbById(EanComRefQSymbList, SIZEOFARRAY(EanComRefQSymbList), refq, rSymb); }
//static 
int FASTCALL PPEanComDocument::GetRefqBySymb(const char * pSymb)
	{ return __EdiGetIdBySymb(EanComRefQSymbList, SIZEOFARRAY(EanComRefQSymbList), pSymb); }

class EdiProviderImplementation_Kontur : public PPEdiProcessor::ProviderImplementation {
public:
	SLAPI  EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags);
	virtual SLAPI ~EdiProviderImplementation_Kontur();
	virtual int    SLAPI  GetDocumentList(PPEdiProcessor::DocumentInfoList & rList);
	virtual int    SLAPI  ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
	virtual int    SLAPI  SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
};

SLAPI PPEdiProcessor::DocumentInfo::DocumentInfo() : ID(0), EdiOp(0), Time(ZERODATETIME), Status(0), Flags(0), PrvFlags(0)
{
	Uuid.SetZero();
}

SLAPI PPEdiProcessor::DocumentInfoList::DocumentInfoList()
{
}

uint SLAPI PPEdiProcessor::DocumentInfoList::GetCount() const { return L.getCount(); }

int SLAPI PPEdiProcessor::DocumentInfoList::GetByIdx(uint idx, DocumentInfo & rItem) const
{
	int    ok = 1;
	if(idx < L.getCount()) {
		const Entry & r_entry = L.at(idx);
		rItem.ID = r_entry.ID;
		rItem.EdiOp = r_entry.EdiOp;
		rItem.Time = r_entry.Dtm;
		rItem.Uuid = r_entry.Uuid;
		rItem.Status = r_entry.Status;
		rItem.Flags = r_entry.Flags;
		rItem.PrvFlags = r_entry.PrvFlags;
		GetS(r_entry.CodeP, rItem.Code);
		GetS(r_entry.SenderCodeP, rItem.SenderCode);
		GetS(r_entry.RcvrCodeP, rItem.RcvrCode);
		GetS(r_entry.BoxP, rItem.Box);
		GetS(r_entry.SIdP, rItem.SId);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPEdiProcessor::DocumentInfoList::Add(const DocumentInfo & rItem, uint * pIdx)
{
	int    ok = 1;
	Entry  new_entry;
	MEMSZERO(new_entry);
	new_entry.ID = rItem.ID;
	new_entry.EdiOp = rItem.EdiOp;
	new_entry.Dtm = rItem.Time;
	new_entry.Uuid = rItem.Uuid;
	new_entry.Status = rItem.Status;
	new_entry.Flags = rItem.Flags;
	new_entry.PrvFlags = rItem.PrvFlags;
	AddS(rItem.Code, &new_entry.CodeP);
	AddS(rItem.SenderCode, &new_entry.SenderCodeP);
	AddS(rItem.RcvrCode, &new_entry.RcvrCodeP);
	AddS(rItem.Box, &new_entry.BoxP);
	AddS(rItem.SId, &new_entry.SIdP);
	L.insert(&new_entry);
	return ok;
}

PPEdiProcessor::Packet::Packet(int docType) : DocType(docType), Flags(0), P_Data(0)
{
	switch(DocType) {
		case PPEDIOP_ORDER:
		case PPEDIOP_DESADV:
			P_Data = new PPBillPacket;
			break;
	}
}

PPEdiProcessor::Packet::~Packet()
{
	switch(DocType) {
		case PPEDIOP_ORDER:
		case PPEDIOP_DESADV:
			delete (PPBillPacket *)P_Data;
			break;
	}
	P_Data = 0;
}

//static 
PPEdiProcessor::ProviderImplementation * SLAPI PPEdiProcessor::CreateProviderImplementation(PPID ediPrvID, PPID mainOrgID, long flags)
{
	ProviderImplementation * p_imp = 0;
	PPObjEdiProvider ep_obj;
	PPEdiProviderPacket ep_pack;
	THROW(ep_obj.GetPacket(ediPrvID, &ep_pack) > 0);
	if(sstreqi_ascii(ep_pack.Rec.Symb, "KONTUR") || sstreqi_ascii(ep_pack.Rec.Symb, "KONTUR-T")) {
		p_imp = new EdiProviderImplementation_Kontur(ep_pack, mainOrgID, flags);
	}
	else {
		CALLEXCEPT_PP_S(PPERR_EDI_THEREISNTPRVIMP, ep_pack.Rec.Symb);
	}
	CATCH
		ZDELETE(p_imp);
	ENDCATCH
	return p_imp;
}

SLAPI PPEdiProcessor::PPEdiProcessor(ProviderImplementation * pImp, PPLogger * pLogger) : P_Prv(pImp), P_Logger(pLogger), P_BObj(BillObj)
{
}

SLAPI PPEdiProcessor::~PPEdiProcessor()
{
}

int SLAPI PPEdiProcessor::SendDocument(DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->SendDocument(pIdent, rPack));
	CATCH
		ok = 0;
		CALLPTRMEMB(P_Logger, LogLastError());
	ENDCATCH
	return ok;
}

int SLAPI PPEdiProcessor::ReceiveDocument(const DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->ReceiveDocument(pIdent, rPack));
	CATCHZOK
	return ok;
}

int SLAPI PPEdiProcessor::GetDocumentList(DocumentInfoList & rList)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->GetDocumentList(rList))
	CATCHZOK
	return ok;
}

int SLAPI PPEdiProcessor::SendBills(const PPBillExportFilt & rP)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	CATCHZOK
	return ok;
}

int SLAPI PPEdiProcessor::SendOrders(const PPBillExportFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	BillTbl::Rec bill_rec;
	PPIDArray temp_bill_list;
	PPIDArray op_list;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	{
		PPPredictConfig cfg;
		PrcssrPrediction::GetPredictCfg(&cfg);
		op_list.addnz(cfg.PurchaseOpID);
		op_list.addnz(CConfig.DraftRcptOp);
		op_list.sortAndUndup();
	}
	for(uint i = 0; i < op_list.getCount(); i++) {
		const PPID op_id = op_list.get(i);
		PPOprKind op_rec;
		GetOpData(op_id, &op_rec);
		if(rP.IdList.getCount()) {
			for(uint j = 0; j < rP.IdList.getCount(); j++) {
				const PPID bill_id = rP.IdList.get(j);
				if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
					if(!rP.LocID || bill_rec.LocID == rP.LocID) {
						if(rArList.lsearch(bill_rec.Object)) {
							temp_bill_list.add(bill_rec.ID);
						}
					}
				}
			}
		}
		else {
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if(!rP.LocID || bill_rec.LocID == rP.LocID) {
					if(rArList.lsearch(bill_rec.Object)) {
						temp_bill_list.add(bill_rec.ID);
					}
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0) {
			PPEdiProcessor::Packet pack(PPEDIOP_ORDER);
			if(P_BObj->ExtractPacket(bill_id, (PPBillPacket *)pack.P_Data) > 0) {
				DocumentInfo di;
				SendDocument(&di, pack);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPEdiProcessor::SendDESADV(const PPBillExportFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	BillTbl::Rec bill_rec;
	PPIDArray temp_bill_list;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	if(rP.IdList.getCount()) {
		for(uint j = 0; j < rP.IdList.getCount(); j++) {
			const PPID bill_id = rP.IdList.get(j);
			if(P_BObj->Search(bill_id, &bill_rec) > 0 && GetOpType(bill_rec.OpID) == PPOPT_GOODSEXPEND) {
				if(!rP.LocID || bill_rec.LocID == rP.LocID) {
					if(rArList.lsearch(bill_rec.Object)) {
						temp_bill_list.add(bill_rec.ID);
					}
				}
			}
		}
	}
	else {
		for(uint i = 0; i < rArList.getCount(); i++) {
			const PPID ar_id = rArList.get(i);
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && GetOpType(bill_rec.OpID) == PPOPT_GOODSEXPEND) {
					temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0) {
			PPEdiProcessor::Packet pack(PPEDIOP_DESADV);
			if(P_BObj->ExtractPacket(bill_id, (PPBillPacket *)pack.P_Data) > 0) {
				DocumentInfo di;
				SendDocument(&di, pack);
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI EdiProviderImplementation_Kontur::EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags) : 
	PPEdiProcessor::ProviderImplementation(rEpp, mainOrgID, flags)
{
}

SLAPI EdiProviderImplementation_Kontur::~EdiProviderImplementation_Kontur()
{
}

/*
int ImportCls::ParseFileName(const char * pFileName, PPEdiMessageEntry * pEntry) const
{
	int    ok = 0;
	SString left, right;
    SPathStruc ps(pFileName);
    pEntry->EdiOp = 0;
	if(ps.Nam.Divide('_', left, right) > 0) {
		if(left.CmpNC("OrdRsp") == 0)
			pEntry->EdiOp = PPEDIOP_ORDERRSP;
		else if(left.CmpNC("Desadv") == 0)
			pEntry->EdiOp = PPEDIOP_DESADV;
		else if(left.CmpNC("Aperak") == 0)
			pEntry->EdiOp = PPEDIOP_APERAK;
		else if(left.CmpNC("alcodesadv") == 0 || left.CmpNC("alcrpt") == 0 || left.CmpNC("alcdes") == 0)
			pEntry->EdiOp = PPEDIOP_ALCODESADV;
		pEntry->Uuid.FromStr(right);
		STRNSCPY(pEntry->SId, pFileName);
	}
	else {
		if(left.CmpPrefix("ordrsp", 1) == 0)
			pEntry->EdiOp = PPEDIOP_ORDERRSP;
		else if(left.CmpPrefix("Desadv", 1) == 0)
			pEntry->EdiOp = PPEDIOP_DESADV;
		else if(left.CmpPrefix("Aperak", 1) == 0)
			pEntry->EdiOp = PPEDIOP_APERAK;
		else if(left.CmpPrefix("alcodesadv", 1) == 0 || left.CmpPrefix("alcrpt", 1) == 0 || left.CmpPrefix("alcdes", 1) == 0)
			pEntry->EdiOp = PPEDIOP_ALCODESADV;
	}
	if(pEntry->EdiOp) {
		ok = 1;
	}
	return ok;
}
*/

int SLAPI EdiProviderImplementation_Kontur::GetDocumentList(PPEdiProcessor::DocumentInfoList & rList)
{
	int    ok = -1;
	SString temp_buf;
	SString left, right;
	SPathStruc ps;
	ScURL  curl;
	InetUrl url;
	if(Epp.MakeUrl(0, url)) {
		int    prot = url.GetProtocol();
		if(prot == InetUrl::protUnkn) {
			url.SetProtocol(InetUrl::protFtp);
		}
		if(prot == InetUrl::protFtp) {
			const char * p_box = "Inbox";
			int    last_id = 0;
			ScURL  curl;
			url.SetComponent(url.cPath, p_box);
			SFileEntryPool fp;
			SFileEntryPool::Entry fpe;
			THROW_SL(curl.FtpList(url, ScURL::mfVerbose, fp));
			for(uint i = 0; i < fp.GetCount(); i++) {
				if(fp.Get(i, &fpe, 0) > 0) {
					ps.Split(fpe.Name);
					if(ps.Ext.IsEqiAscii("xml") && ps.Nam.Divide('_', left, right) > 0) {
						PPEdiProcessor::DocumentInfo entry;
						entry.Uuid.FromStr(right);
						if(left.IsEqiAscii("Desadv")) {
							entry.EdiOp = PPEDIOP_DESADV;
						}
						else if(left.IsEqiAscii("alcodesadv") || left.IsEqiAscii("alcrpt") || left.IsEqiAscii("alcdes")) {
							entry.EdiOp = PPEDIOP_ALCODESADV;
						}
						else if(left.IsEqiAscii("Aperak")) {
							entry.EdiOp = PPEDIOP_APERAK;
						}
						else if(left.IsEqiAscii("Recadv")) {
							entry.EdiOp = PPEDIOP_RECADV;
						}
						else if(left.IsEqiAscii("OrdRsp")) {
							entry.EdiOp = PPEDIOP_ORDERRSP;
						}
						else if(left.IsEqiAscii("Orders") || left.IsEqiAscii("Order")) {
							entry.EdiOp = PPEDIOP_ORDER;
						}
						entry.Box = p_box;
						entry.ID = ++last_id;
						entry.SId = fpe.Name;
						entry.Time = fpe.WriteTime;
						THROW(rList.Add(entry, 0));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI EdiProviderImplementation_Kontur::ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	return -1;
}

int SLAPI EdiProviderImplementation_Kontur::SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = 1;
	xmlTextWriter * p_x = 0;
	SString path;
	if(oneof2(rPack.DocType, PPEDIOP_ORDER, PPEDIOP_DESADV)) {
		SString temp_buf;
		PPEanComDocument s_doc(this);
		GetTempOutputPath(rPack.DocType, path);
		THROW_SL(::createDir(path.RmvLastSlash()));
		MakeTempFileName(path.SetLastSlash(), "export_", "xml", 0, temp_buf);
		path = temp_buf;
		THROW(p_x = xmlNewTextWriterFilename(path, 0));
		if(rPack.DocType == PPEDIOP_ORDER) {
			THROW(s_doc.Write_ORDERS(p_x, *(const PPBillPacket *)rPack.P_Data));
		}
		else if(rPack.DocType == PPEDIOP_DESADV) {
			THROW(s_doc.Write_DESADV(p_x, *(const PPBillPacket *)rPack.P_Data));
		}
		if(!(Flags & ctrfTestMode)) {
			InetUrl url;
			if(Epp.MakeUrl(0, url)) {
				int    prot = url.GetProtocol();
				if(prot == InetUrl::protUnkn) {
					url.SetProtocol(InetUrl::protFtp);
				}
				if(prot == InetUrl::protFtp) {
					ScURL curl;
					url.SetComponent(url.cPath, "Outbox");
					THROW(curl.FtpPut(url, ScURL::mfVerbose, path, 0));
				}
			}
		}
	}
    CATCHZOK
	xmlFreeTextWriter(p_x);
	if(!ok && path.NotEmpty())
		SFile::Remove(path);
    return ok;
}
