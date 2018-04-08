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

int SLAPI PPEdiProcessor::ProviderImplementation::GetIntermediatePath(const char * pSub, int docType, SString & rBuf)
{
	rBuf.Z();
	PPGetPath(PPPATH_TEMP, rBuf);
	rBuf.SetLastSlash().Cat("EDI");
	if(Epp.Rec.Symb[0])
		rBuf.SetLastSlash().Cat(Epp.Rec.Symb);
	if(!isempty(pSub))
		rBuf.SetLastSlash().Cat(pSub);
	if(docType) {
		SString temp_buf;
		PPGetSubStrById(PPTXT_EDIOP, docType, temp_buf);
		if(temp_buf.NotEmpty())
			rBuf.SetLastSlash().Cat(temp_buf);
	}
	rBuf.SetLastSlash();
	::createDir(rBuf);
	return 1;
}

int SLAPI PPEdiProcessor::ProviderImplementation::GetTempOutputPath(int docType, SString & rBuf)
	{ return GetIntermediatePath("OUT", docType, rBuf); }
int SLAPI PPEdiProcessor::ProviderImplementation::GetTempInputPath(int docType, SString & rBuf)
	{ return GetIntermediatePath("IN", docType, rBuf); }

class PPEanComDocument {
public:

	struct QValue {
		QValue() : Q(0), Unit(0), Currency(0), Value(0.0)
		{
		}
		int    Q;    // Квалификатор 
		int    Unit; // UNIT_XXX
		int    Currency; // Валюта (для денежных величин)
		double Value; // Значение
	};
	struct DtmValue {
		DtmValue() : Q(0), Dtm(ZERODATETIME), DtmFinish(ZERODATETIME)
		{
		}
		int    Q;
		LDATETIME Dtm;
		LDATETIME DtmFinish;
	};

	static int FASTCALL GetMsgSymbByType(int msgType, SString & rSymb);
	static int FASTCALL GetMsgTypeBySymb(const char * pSymb);
	static int FASTCALL GetRefqSymb(int refq, SString & rSymb);
	static int FASTCALL GetRefqBySymb(const char * pSymb);
	static int FASTCALL GetPartyqSymb(int refq, SString & rSymb);
	static int FASTCALL GetPartyqBySymb(const char * pSymb);
	SLAPI  PPEanComDocument(PPEdiProcessor::ProviderImplementation * pPi);
	SLAPI ~PPEanComDocument();
	int    SLAPI Write_MessageHeader(SXml::WDoc & rDoc, int msgType, const char * pMsgId);
	int    SLAPI Read_MessageHeader(xmlNode * pFirstNode, SString & rMsgType, SString & rMsgId); // @notimplemented
	//
	// Descr: Коды функций сообщения
	// Message function code:
	// 1 Cancellation. Message cancelling a previous transmission for a given transaction.
	// 2 Addition. Message containing items to be added.
	// 3 Deletion. Message containing items to be deleted.
	// 4 Change. Message containing items to be changed.
	// 5 Replace. Message replacing a previous message.
	// 6 Confirmation. Message confirming the details of a previous transmission where such confirmation is required or
	//	recommended under the terms of a trading partner agreement.
	// 7 Duplicate. The message is a duplicate of a previously generated message.
	// 8 Status. Code indicating that the referenced message is a status.
	// 9 Original. Initial transmission related to a given transaction.
	// 10 Not found. Message whose reference number is not filed.
	// 11 Response. Message responding to a previous message or document.
	// 12 Not processed. Message indicating that the referenced message was received but not yet processed.
	// 13 Request. Code indicating that the referenced message is a request.
	// 14 Advance notification. Code indicating that the information contained in the
	//	message is an advance notification of information to follow.
	// 15 Reminder. Repeated message transmission for reminding purposes.
	// 16 Proposal. Message content is a proposal.
	// 17 Cancel, to be reissued. Referenced transaction cancelled, reissued message will follow.
	// 18 Reissue. New issue of a previous message (maybe cancelled).
	// 19 Seller initiated change. Change information submitted by buyer but initiated by seller.
	// 20 Replace heading section only. Message to replace the heading of a previous message.
	// 21 Replace item detail and summary only. Message to replace item detail and summary of a previous message.
	// 22 Final transmission. Final message in a related series of messages together
	//	making up a commercial, administrative or transport transaction.
	// 23 Transaction on hold. Message not to be processed until further release information.
	// 24 Delivery instruction. Delivery schedule message only used to transmit short-term delivery instructions.
	// 25 Forecast. Delivery schedule message only used to transmit long- term schedule information.
	// 26 Delivery instruction and forecast. Combination of codes '24' and '25'.
	// 27 Not accepted. Message to inform that the referenced message is not accepted by the recipient.
	// 28 Accepted, with amendment in heading section. Message accepted but amended in heading section.
	// 29 Accepted without amendment. Referenced message is entirely accepted.
	// 30 Accepted, with amendment in detail section. Referenced message is accepted but amended in detail section.
	// 31 Copy. Indicates that the message is a copy of an original message that has been sent, e.g. for action or information.
	// 32 Approval. A message releasing an existing referenced message for action to the receiver.
	// 33 Change in heading section. Message changing the referenced message heading section.
	// 34 Accepted with amendment. The referenced message is accepted but amended.
	// 35 Retransmission. Change-free transmission of a message previously sent.
	// 36 Change in detail section. Message changing referenced detail section.
	// 37 Reversal of a debit. Reversal of a previously posted debit.
	// 38 Reversal of a credit. Reversal of a previously posted credit.
	// 39 Reversal for cancellation. Code indicating that the referenced message is reversing
	//	a cancellation of a previous transmission for a given transaction.
	// 40 Request for deletion. The message is given to inform the recipient to delete the referenced transaction.
	// 41 Finishing/closing order. Last of series of call-offs.
	// 42 Confirmation via specific means. Message confirming a transaction previously agreed via other means (e.g. phone).
	// 43 Additional transmission. Message already transmitted via another communication
	//	channel. This transmission is to provide electronically processable data only.
	// 44 Accepted without reserves. Message accepted without reserves.
	// 45 Accepted with reserves. Message accepted with reserves.
	// 46 Provisional. Message content is provisional.
	// 47 Definitive. Message content is definitive.
	// 48 Accepted, contents rejected. Message to inform that the previous message is received, but it cannot be processed due to regulations, laws, etc.
	// 49 Settled dispute. The reported dispute is settled.
	// 50 Withdraw. Message withdrawing a previously approved message.
	// 51 Authorisation. Message authorising a message or transaction(s).
	// 52 Proposed amendment. A code used to indicate an amendment suggested by the sender.
	// 53 Test. Code indicating the message is to be considered as a test.
	// 54 Extract. A subset of the original.
	// 55 Notification only. The receiver may use the notification information for analysis only.
	// 56 Advice of ledger booked items. An advice that items have been booked in the ledger.
	// 57 Advice of items pending to be booked in the ledger. An advice that items are pending to be booked in the ledger.
	// 58 Pre-advice of items requiring further information. A pre-advice that items require further information.
	// 59 Pre-adviced items. A pre-advice of items.
	// 60 No action since last message. Code indicating the fact that no action has taken place since the last message.
	// 61 Complete schedule. The message function is a complete schedule.
	// 62 Update schedule. The message function is an update to a schedule.
	// 63 Not accepted, provisional. Not accepted, subject to confirmation.
	// +64 Verification. The message is transmitted to verify information.
	// +65 Unsettled dispute. To report an unsettled dispute.
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
	int    SLAPI Read_BeginningOfMessage(xmlNode * pFirstNode, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode);
	int    SLAPI Write_UNT(SXml::WDoc & rDoc, const char * pDocCode, uint segCount);
	int    SLAPI Read_UNT(xmlNode * pFirstNode, SString & rDocCode, uint * pSegCount);
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
	/*
		2           DDMMYY Calendar date: D = Day; M = Month; Y = Year. 	
		101         YYMMDD Calendar date: Y = Year; M = Month; D = Day. 	
		102 !       CCYYMMDD Calendar date: C = Century ; Y = Year ; M = Month ; D = Day. 	
		104         MMWW-MMWW A period of time specified by giving the start week of a month followed by the end week of a month. Data is to be transmitted as consecutive characters without hyphen. 	
		107         DDD Day's number within a specific year: D = Day. 	
		108         WW Week's number within a specific year: W = Week. 	
		109         MM Month's number within a specific year: M = Month. 	
		110         DD Day's number within is a specific month. 	
		201         YYMMDDHHMM 	Calendar date including time without seconds: Y = Year; M = Month; D = Day; H = Hour; M = Minute. 	
		203  !      CCYYMMDDHHMM 	Calendar date including time with minutes: C=Century; Y=Year; M=Month; D=Day; H=Hour; M=Minutes. 	
		204         CCYYMMDDHHMMSS 	Calendar date including time with seconds: C=Century;Y=Year; M=Month;D=Day;H=Hour;M=Minute;S=Second. 	
		401         HHMM 	Time without seconds: H = Hour; m = Minute. 	
		501         HHMMHHMM 	Time span without seconds: H = Hour; m = Minute;. 	
		502         HHMMSS-HHMMSS 	Format of period to be given without hyphen. 	
		602         CCYY 	Calendar year including century: C = Century; Y = Year. 	
		609         YYMM 	Month within a calendar year: Y = Year; M = Month. 	
		610         CCYYMM 	Month within a calendar year: CC = Century; Y = Year; M = Month. 	
		615         YYWW 	Week within a calendar year: Y = Year; W = Week 1st week of January = week 01. 	
		616  !      CCYYWW 	Week within a calendar year: CC = Century; Y = Year; W = Week (1st week of January = week 01). 	
		713         YYMMDDHHMM-YYMMDDHHMM 	Format of period to be given in actual message without hyphen. 	
		715         YYWW-YYWW 	A period of time specified by giving the start week of a year followed by the end week of year (both not including century). Data is to be transmitted as consecutive characters without hyphen. 	
		717         YYMMDD-YYMMDD 	Format of period to be given in actual message without hyphen. 	
		718  !      CCYYMMDD-CCYYMMDD 	Format of period to be given without hyphen. 	
		719         CCYYMMDDHHMM-CCYYMMDDHHMM 	A period of time which includes the century, year, month, day, hour and minute. Format of period to be given in actual message without hyphen. 	
		720         DHHMM-DHHMM 	Format of period to be given without hyphen (D=day of the week, 1=Monday; 2=Tuesday; ... 7=Sunday). 	
		801         Year 	To indicate a quantity of years. 	
		802         Month 	To indicate a quantity of months. 	
		803         Week 	To indicate a quantity of weeks. 	
		804         Day 	To indicate a quantity of days. 	
		805         Hour 	To indicate a quantity of hours. 	
		806         Minute 	To indicate a quantity of minutes. 	
		810         Trimester 	To indicate a quantity of trimesters (three months). 	
		811         Half month 	To indicate a quantity of half months. 	
		21E         DDHHMM-DDHHMM (GS1 Temporary Code) 	Format of period to be given in actual message without hyphen.
	*/
	enum {
		dtmfmtCCYYMMDD          = 102, // CCYYMMDD 
		dtmfmtCCYYMMDDHHMM      = 203, // CCYYMMDDHHMM 
		dtmfmtCCYYWW            = 616, // CCYYWW 
		dtmfmtCCYYMMDD_CCYYMMDD = 718, // CCYYMMDD-CCYYMMDD
	};
	//
	int    SLAPI Write_DTM(SXml::WDoc & rDoc, int dtmKind, int dtmFmt, const LDATETIME & rDtm, const LDATETIME * pFinish);
	int    SLAPI Read_DTM(xmlNode * pFirstNode, TSVector <DtmValue> & rList);
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
	int    SLAPI Read_RFF(xmlNode * pFirstNode, int * pRefQ, SString & rRef); // reference
	int    SLAPI Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN);
	int    SLAPI Read_NAD(xmlNode * pFirstNode, int * pPartyQ, SString & rGLN);
	int    SLAPI Write_CUX(SXml::WDoc & rDoc, const char * pCurrencyCode3);
	int    SLAPI Read_CUX(xmlNode * pFirstNode, SString & rCurrencyCode3); // @notimplemented
	int    SLAPI Write_CPS(SXml::WDoc & rDoc);
	int    SLAPI Read_CPS(xmlNode * pFirstNode);
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
	//
	// Descr: Считывает суммовую величину и заносит ее в вектор rList.
	//
	int    SLAPI Read_MOA(xmlNode * pFirstNode, TSVector <QValue> & rList);
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
	int    SLAPI Read_QTY(xmlNode * pFirstNode, TSVector <QValue> & rList);
	enum {
		cntqQuantity       = 1, // Algebraic total of the quantity values in line items in a message
		cntqNumOfLn        = 2, // Number of line items in message
		cntqNumOfLnAndSub  = 3, // Number of line and sub items in message
		cntqNumOfInvcLn    = 4, // Number of invoice lines
	};
	int    SLAPI Write_CNT(SXml::WDoc & rDoc, int countQ, double value);
	int    SLAPI Read_CNT(xmlNode * pFirstNode, int * pCountQ, double * pValue);
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
	int    SLAPI Read_LIN(xmlNode * pFirstNode, int * pLineN, SString & rGoodsCode);
	int    SLAPI Write_PIA(SXml::WDoc & rDoc, const char * pGoodsCode);
	int    SLAPI Read_PIA(xmlNode * pFirstNode, SString & rGoodsCode);
	//
	// Descr: IMD Description format code
	//
	enum {
		imdqFreeFormLongDescr = 1, // A = Free-form long description 
		imdqCode,                  // C = Code (from industry code list) 
		imdqFreeFormShortDescr,    // E = Free-form short description 
		imdqFreeForm,              // F = Free-form 
		imdqStructured,            // S = Structured (from industry code list) 
		imdqCodeAndText            // B = Code and text
	};
	int    SLAPI Write_IMD(SXml::WDoc & rDoc, int imdq, const char * pDescription);
	int    SLAPI Read_IMD(xmlNode * pFirstNode, int * pImdQ, SString & rDescription);

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
	int    SLAPI Write_OrderGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal);
	//
	//
	//
	int    SLAPI Write_DESADV(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    SLAPI Write_ORDERS(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    SLAPI Read_DESADV(void * pCtx, const char * pFileName, const PPBillPacket & rPack);
	int    SLAPI Read_Document(void * pCtx, const char * pFileName, TSCollection <PPEdiProcessor::Packet> & rList);
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

int SLAPI PPEanComDocument::Read_MessageHeader(xmlNode * pFirstNode, SString & rMsgType, SString & rMsgId) // @notimplemented
{
	rMsgType.Z();
	rMsgId.Z();
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E0062", temp_buf)) {
			rMsgId = temp_buf;
		}
		else if(SXml::IsName(p_n, "S009")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E0065", temp_buf)) {
					rMsgType = temp_buf;
				}
			}
		}
	}
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

int SLAPI PPEanComDocument::Read_BeginningOfMessage(xmlNode * pFirstNode, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode)
{
	rDocCode.Z();
	rDocIdent.Z();
	int    ok = 1;
	int    func_msg_code = 0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C002")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1001", temp_buf)) {
					rDocCode = temp_buf;
				}
			}
		}
		else if(SXml::IsName(p_n, "C106")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1004", temp_buf)) {
					rDocIdent = temp_buf;
				}
			}
		}
		else if(SXml::GetContentByName(p_n, "E1225", temp_buf)) {
			func_msg_code = temp_buf.ToLong();
		}
	}
	ASSIGN_PTR(pFuncMsgCode, func_msg_code);
	return ok;
}

int SLAPI PPEanComDocument::Write_UNT(SXml::WDoc & rDoc, const char * pDocCode, uint segCount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_unt(rDoc, "UNT"); // Окончание сообщения
	n_unt.PutInner("E0074", temp_buf.Z().Cat(segCount)); // Общее число сегментов в сообщении
	n_unt.PutInner("E0062", pDocCode); // Номер электронного сообщения (совпадает с указанным в заголовке)
	return ok;
}

int SLAPI PPEanComDocument::Read_UNT(xmlNode * pFirstNode, SString & rDocCode, uint * pSegCount)
{
	int    ok = 1;
	uint   seg_count = 0;
	rDocCode.Z();
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E0074", temp_buf)) {
			seg_count = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_n, "E0062", temp_buf)) {
			rDocCode = temp_buf;
		}
	}
	ASSIGN_PTR(pSegCount, seg_count);
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

	/*
		2           DDMMYY Calendar date: D = Day; M = Month; Y = Year. 	
		101         YYMMDD Calendar date: Y = Year; M = Month; D = Day. 	
		102 !       CCYYMMDD Calendar date: C = Century ; Y = Year ; M = Month ; D = Day. 	
		104         MMWW-MMWW A period of time specified by giving the start week of a month followed by the end week of a month. Data is to be transmitted as consecutive characters without hyphen. 	
		107         DDD Day's number within a specific year: D = Day. 	
		108         WW Week's number within a specific year: W = Week. 	
		109         MM Month's number within a specific year: M = Month. 	
		110         DD Day's number within is a specific month. 	
		201         YYMMDDHHMM 	Calendar date including time without seconds: Y = Year; M = Month; D = Day; H = Hour; M = Minute. 	
		203  !      CCYYMMDDHHMM 	Calendar date including time with minutes: C=Century; Y=Year; M=Month; D=Day; H=Hour; M=Minutes. 	
		204         CCYYMMDDHHMMSS 	Calendar date including time with seconds: C=Century;Y=Year; M=Month;D=Day;H=Hour;M=Minute;S=Second. 	
		401         HHMM 	Time without seconds: H = Hour; m = Minute. 	
		501         HHMMHHMM 	Time span without seconds: H = Hour; m = Minute;. 	
		502         HHMMSS-HHMMSS 	Format of period to be given without hyphen. 	
		602         CCYY 	Calendar year including century: C = Century; Y = Year. 	
		609         YYMM 	Month within a calendar year: Y = Year; M = Month. 	
		610         CCYYMM 	Month within a calendar year: CC = Century; Y = Year; M = Month. 	
		615         YYWW 	Week within a calendar year: Y = Year; W = Week 1st week of January = week 01. 	
		616  !      CCYYWW 	Week within a calendar year: CC = Century; Y = Year; W = Week (1st week of January = week 01). 	
		713         YYMMDDHHMM-YYMMDDHHMM 	Format of period to be given in actual message without hyphen. 	
		715         YYWW-YYWW 	A period of time specified by giving the start week of a year followed by the end week of year (both not including century). Data is to be transmitted as consecutive characters without hyphen. 	
		717         YYMMDD-YYMMDD 	Format of period to be given in actual message without hyphen. 	
		718  !      CCYYMMDD-CCYYMMDD 	Format of period to be given without hyphen. 	
		719         CCYYMMDDHHMM-CCYYMMDDHHMM 	A period of time which includes the century, year, month, day, hour and minute. Format of period to be given in actual message without hyphen. 	
		720         DHHMM-DHHMM 	Format of period to be given without hyphen (D=day of the week, 1=Monday; 2=Tuesday; ... 7=Sunday). 	
		801         Year 	To indicate a quantity of years. 	
		802         Month 	To indicate a quantity of months. 	
		803         Week 	To indicate a quantity of weeks. 	
		804         Day 	To indicate a quantity of days. 	
		805         Hour 	To indicate a quantity of hours. 	
		806         Minute 	To indicate a quantity of minutes. 	
		810         Trimester 	To indicate a quantity of trimesters (three months). 	
		811         Half month 	To indicate a quantity of half months. 	
		21E         DDHHMM-DDHHMM (GS1 Temporary Code) 	Format of period to be given in actual message without hyphen.
	*/

static int ParseDTM(int fmt, const SString & rBuf, LDATETIME & rDtm, LDATETIME & rDtmFinish)
{
	int    ok = 0;
	SString temp_buf;
	rDtm.SetZero();
	rDtmFinish.SetZero();
	switch(fmt) {
		case 102:
			if(rBuf.Len() == 8) {
				ok = strtodate(rBuf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
			}
			break;
		case 203:
			if(rBuf.Len() == 12) {
				(temp_buf = rBuf).Trim(8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
				(temp_buf = rBuf).Excise(7, 4);
				strtotime(temp_buf, TIMF_HM|TIMF_NODIV, &rDtm.t);
				ok = 1;
			}
			break;
		case 616:
			break;
		case 718:
			if(rBuf.Len() == 16) {
				(temp_buf = rBuf).Trim(8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
				(temp_buf = rBuf).Excise(7, 8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtmFinish.d);
				ok = 1;
			}
			break;
	}
	return ok;
}

int SLAPI PPEanComDocument::Read_DTM(xmlNode * pFirstNode, TSVector <DtmValue> & rList)
{
	int    ok = 1;
	int    dtm_fmt = 0;
	DtmValue dtm_val;
	SString dtm_text;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C507")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E2005", temp_buf)) {
					dtm_val.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E2379", temp_buf)) {
					dtm_fmt = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E2380", temp_buf)) {
					dtm_text = temp_buf;
				}
			}
		}
	}
	ParseDTM(dtm_fmt, dtm_text, dtm_val.Dtm, dtm_val.DtmFinish);
	if(dtm_val.Q) {
		rList.insert(&dtm_val);
	}
	else
		ok = 0;
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

int SLAPI PPEanComDocument::Read_RFF(xmlNode * pFirstNode, int * pRefQ, SString & rRef) // reference
{
	int    ok = 1;
	int    ref_q = 0;
	rRef.Z();
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C506")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1153", temp_buf))
					ref_q = GetRefqBySymb(temp_buf);
				else if(SXml::GetContentByName(p_n2, "E1154", temp_buf))
					rRef = temp_buf;
			}
		}
	}
	ASSIGN_PTR(pRefQ, ref_q);
	return ok;
}

int SLAPI PPEanComDocument::Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_nad(rDoc, "NAD"); // Наименование и адрес
	THROW_PP(!isempty(pGLN), PPERR_EDI_GLNISEMPTY);
	{
		THROW_PP_S(GetPartyqSymb(partyQ, temp_buf), PPERR_EDI_INVPARTYQ, (long)partyQ);
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

int SLAPI PPEanComDocument::Read_NAD(xmlNode * pFirstNode, int * pPartyQ, SString & rGLN)
{
	int    ok = 1;
	int    party_q = 0;
	rGLN.Z();
	SString temp_buf;
	SString ident_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E3035", temp_buf)) { // Party function code qualifier
			party_q = GetPartyqBySymb(temp_buf);
		}
		else if(SXml::IsName(p_n, "C082")) { // PARTY IDENTIFICATION DETAILS
			int    agency_code = 0;
			ident_buf.Z();
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E3039", temp_buf)) { // Party identifier
					ident_buf = temp_buf;
				}
				else if(SXml::GetContentByName(p_n2, "E3055", temp_buf)) { // Code list responsible agency code (9 = GS1)
					agency_code = temp_buf.ToLong();
				}
			}
			rGLN = ident_buf;
		}
		else if(SXml::IsName(p_n, "C080")) { // PARTY NAME
		}
		else if(SXml::IsName(p_n, "C058")) { // NAME AND ADDRESS
		}
		else if(SXml::IsName(p_n, "C059")) { // STREET
		}
		else if(SXml::IsName(p_n, "C819")) { // COUNTRY SUB-ENTITY DETAILS
		}
		else if(SXml::GetContentByName(p_n, "E3251", temp_buf)) { // Postal identification code
		}
		else if(SXml::GetContentByName(p_n, "E3207", temp_buf)) { // Country name code
		}
	}
	ASSIGN_PTR(pPartyQ, party_q);
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

int SLAPI PPEanComDocument::Read_CUX(xmlNode * pFirstNode, SString & rCurrencyCode3)
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
	
int SLAPI PPEanComDocument::Read_MOA(xmlNode * pFirstNode, TSVector <QValue> & rList)
{
	int    ok = 1;
	SString temp_buf;
	QValue qv;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C516")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E5025", temp_buf)) {
					qv.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E5004", temp_buf)) {
					qv.Value = temp_buf.ToReal();
				}
			}
		}
	}
	if(qv.Q) {
		rList.insert(&qv);
	}
	else
		ok = 0;
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

int SLAPI PPEanComDocument::Read_QTY(xmlNode * pFirstNode, TSVector <QValue> & rList)
{
	int    ok = 1;
	SString temp_buf;
	QValue qv;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C186")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E6063", temp_buf)) {
					qv.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6060", temp_buf)) {
					qv.Value = temp_buf.ToReal();
				}
				else if(SXml::GetContentByName(p_n2, "E6411", temp_buf)) {
					if(temp_buf.IsEqiAscii("KGM"))
						qv.Unit = UNIT_KILOGRAM;
					else if(temp_buf.IsEqiAscii("PCE"))
						qv.Unit = UNIT_ITEM;
				}
			}
		}
	}
	if(qv.Q) {
		rList.insert(&qv);
	}
	else
		ok = 0;
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
	
int SLAPI PPEanComDocument::Read_CNT(xmlNode * pFirstNode, int * pCountQ, double * pValue)
{
	int    ok = 1;
	int    count_q = 0;
	double value = 0.0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C270")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E6069", temp_buf)) {
					count_q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6066", temp_buf)) {
					value = temp_buf.ToReal();
				}
			}
		}
	}
	ASSIGN_PTR(pCountQ, count_q);
	ASSIGN_PTR(pValue, value);
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

int SLAPI PPEanComDocument::Read_LIN(xmlNode * pFirstNode, int * pLineN, SString & rGoodsCode)
{
	rGoodsCode.Z();
	int    ok = 1;
	int    line_n = 0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E1082", temp_buf)) {
			line_n = temp_buf.ToLong();
		}
		else if(SXml::IsName(p_n, "C212")) {
			int   is_srv = 0;
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E7140", temp_buf)) {
					rGoodsCode = temp_buf;
				}
				else if(SXml::GetContentByName(p_n2, "E7143", temp_buf)) {
					if(temp_buf.IsEqiAscii("SRV"))
						is_srv = 1;
				}
			}
			if(!is_srv) {
				rGoodsCode.Z();
			}
		}
	}
	ASSIGN_PTR(pLineN, line_n);
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

int SLAPI PPEanComDocument::Read_PIA(xmlNode * pFirstNode, SString & rGoodsCode)
{
	int    ok = 0;
	return ok;
}

int SLAPI PPEanComDocument::Write_IMD(SXml::WDoc & rDoc, int imdQ, const char * pDescription)
{
	int    ok = 1;
	if(!isempty(pDescription)) {
		SString temp_buf;
		SXml::WNode n_imd(rDoc, "IMD"); // Описание товара
		switch(imdQ) {
			case imdqFreeFormLongDescr: temp_buf = "A"; break;
			case imdqCode: temp_buf = "C"; break;
			case imdqFreeFormShortDescr: temp_buf = "E"; break;
			case imdqFreeForm:    temp_buf = "F"; break;
			case imdqStructured:  temp_buf = "S"; break;
			case imdqCodeAndText: temp_buf = "B"; break;
			default: temp_buf.Z(); break;
		}
		if(temp_buf.NotEmpty()) {
			n_imd.PutInner("E7077", temp_buf); // Код формата описания (текст)
			{
				SXml::WNode n_c273(rDoc, "C273"); // Описание
				n_c273.PutInner("E7008", SXml::WNode::CDATA((temp_buf = pDescription).Transf(CTRANSF_INNER_TO_UTF8)));
			}
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPEanComDocument::Read_IMD(xmlNode * pFirstNode, int * pImdQ, SString & rDescription)
{
	rDescription.Z();
	int    ok = 1;
	int    imd_q = 0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E7077", temp_buf)) {
			if(temp_buf.Len() == 1) {
				switch(temp_buf.C(0)) {
					case 'A': imd_q = imdqFreeFormLongDescr; break;
					case 'C': imd_q = imdqCode; break;
					case 'E': imd_q = imdqFreeFormShortDescr; break;
					case 'F': imd_q = imdqFreeForm; break;
					case 'S': imd_q = imdqStructured; break;
					case 'B': imd_q = imdqCodeAndText; break;
				}
			}
		}
		else if(SXml::IsName(p_n, "C273")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E7008", temp_buf)) {
					rDescription = temp_buf;
				}
			}
		}
	}
	ASSIGN_PTR(pImdQ, imd_q);
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
		THROW(Write_IMD(rDoc, imdqFreeForm, goods_rec.Name));
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
		THROW(Write_IMD(rDoc, imdqFreeForm, goods_rec.Name));
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

/*int SLAPI PPEanComDocument::Helper_Read(void * pCtx, const char * pFileName, )
{
	int    ok = 0;
	return ok;
}*/

int SLAPI PPEanComDocument::Read_Document(void * pCtx, const char * pFileName, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = 0;
	SString temp_buf;
	SString ident_buf;
	SString ref_buf;
	SString gln_buf;
	SString goods_code_buf;
	SString goods_descr;
	int    func_msg_code = 0;
	xmlParserCtxt * p_ctx = (xmlParserCtxt *)pCtx;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	PPEdiProcessor::Packet * p_pack = 0;
	PPBillPacket * p_bpack = 0; // is owned by p_pack
	TSVector <DtmValue> dtm_list;
	TSVector <DtmValue> dtm_temp_list;
	TSVector <QValue> moa_list;
	TSVector <QValue> moa_byitem_list;
	TSVector <QValue> qty_byitem_list;
	THROW_SL(fileExists(pFileName));
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "DESADV")) {
		THROW_MEM(p_pack = new PPEdiProcessor::Packet(PPEDIOP_DESADV));
		p_bpack = (PPBillPacket *)p_pack->P_Data;
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "UNH")) {
				THROW(Read_MessageHeader(p_n->children, temp_buf, ident_buf));
			}
			else if(SXml::IsName(p_n, "BGM")) {
				THROW(Read_BeginningOfMessage(p_n->children, temp_buf, ident_buf, &func_msg_code));
			}
			else if(SXml::IsName(p_n, "DTM")) {
				THROW(Read_DTM(p_n->children, dtm_list));
			}
			else if(SXml::IsName(p_n, "MOA")) {
				THROW(Read_MOA(p_n->children, moa_list));
			}
			else if(SXml::IsName(p_n, "SG1")) {
				dtm_temp_list.clear();
				int    ref_q = 0;
				ref_buf.Z();
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, "RFF")) {
						THROW(Read_RFF(p_n2->children, &ref_q, ref_buf));
					}
					else if(SXml::IsName(p_n, "DTM")) {
						THROW(Read_DTM(p_n2->children, dtm_temp_list));
					}
				}
			}
			else if(SXml::IsName(p_n, "SG2")) {
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					int    party_q = 0;
					gln_buf.Z();
					if(SXml::IsName(p_n2, "NAD")) {
						PPID   ar_id = 0;
						int    rgr = 0;
						THROW(Read_NAD(p_n2->children, &party_q, gln_buf));
						/*
							{ EDIPARTYQ_BUYER, "BY" },
							{ EDIPARTYQ_CORPOFFICE, "CO" },
							{ EDIPARTYQ_DELIVERY, "DP" },
							{ EDIPARTYQ_INVOICEE, "IV" },
							{ EDIPARTYQ_STORENUMBER, "SN" },
							{ EDIPARTYQ_SUPPLAGENT, "SR" },
							{ EDIPARTYQ_SUPPLIER, "SU" },
							{ EDIPARTYQ_WAREHOUSEKEEPER, "WH" },
							{ EDIPARTYQ_CONSIGNOR, "CZ" },
						*/
						if(party_q == EDIPARTYQ_SUPPLIER) {
							rgr = P_Pi->PsnObj.ResolveGLN(gln_buf, GetSupplAccSheet(), &ar_id);
						}
						else if(party_q == EDIPARTYQ_BUYER) {
						}
						else if(party_q == EDIPARTYQ_CONSIGNER) {
						}
						else if(party_q == EDIPARTYQ_CONSIGNER) {
						}
						else if(party_q == EDIPARTYQ_DELIVERY) {
						}
					}
					else if(SXml::IsName(p_n2, "LOC")) {
					}
					else if(SXml::IsName(p_n2, "SG3")) {
						int    ref_q = 0;
						ref_buf.Z();
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "RFF")) {
								THROW(Read_RFF(p_n2->children, &ref_q, ref_buf));
							}
						}
					}
					else if(SXml::IsName(p_n2, "SG4")) {
					}
				}
			}
			else if(SXml::IsName(p_n, "SG10")) {
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, "CPS")) {
					}
					else if(SXml::IsName(p_n2, "FTX")) {
					}
					else if(SXml::IsName(p_n2, "SG11")) { //PAC-MEA-QTY-SG12-SG13
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "PAC")) {
							}
							else if(SXml::IsName(p_n3, "MEA")) {
							}
							else if(SXml::IsName(p_n3, "QTY")) {
								// This segment is used to specify the quantity per package specified in the PAC segment.
							}
							else if(SXml::IsName(p_n3, "SG12")) {
							}
							else if(SXml::IsName(p_n3, "SG13")) {
							}
						}
					}
					else if(SXml::IsName(p_n2, "SG17")) { // LIN-PIA-IMD-MEA-QTY-ALI-DLM-DTM-FTX-MOA-SG18-SG20-SG22-SG25
						goods_code_buf.Z();
						goods_descr.Z();
						int    goods_descr_q = 0;
						int    line_n = 0;
						moa_byitem_list.clear();
						qty_byitem_list.clear();
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "LIN")) {
								THROW(Read_LIN(p_n3->children, &line_n, goods_code_buf));
							}
							else if(SXml::IsName(p_n3, "PIA")) {
							}
							else if(SXml::IsName(p_n3, "IMD")) {
								THROW(Read_IMD(p_n3->children, &goods_descr_q, goods_descr))
							}
							else if(SXml::IsName(p_n3, "QTY")) {
								THROW(Read_QTY(p_n3->children, qty_byitem_list));
							}
							else if(SXml::IsName(p_n3, "ALI")) {
							}
							else if(SXml::IsName(p_n3, "DLM")) {
							}
							else if(SXml::IsName(p_n3, "DTM")) {
							}
							else if(SXml::IsName(p_n3, "FTX")) {
							}
							else if(SXml::IsName(p_n3, "MOA")) {
								THROW(Read_MOA(p_n2->children, moa_byitem_list));
							}
							else if(SXml::IsName(p_n3, "SG18")) {
							}
							else if(SXml::IsName(p_n3, "SG20")) {
							}
							else if(SXml::IsName(p_n3, "SG22")) {
							}
							else if(SXml::IsName(p_n3, "SG25")) {
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "CNT")) {
				int    cnt_q = 0;
				double cnt_value = 0.0;
				THROW(Read_CNT(p_n->children, &cnt_q, &cnt_value));
			}
			else if(SXml::IsName(p_n, "UNT")) {
				uint   seg_count = 0;
				THROW(Read_UNT(p_n->children, temp_buf, &seg_count));
			}
		}
	}
	else if(SXml::IsName(p_root, "ORDERS")) {
		THROW_MEM(p_pack = new PPEdiProcessor::Packet(PPEDIOP_ORDER));
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
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
			THROW(Write_UNT(_doc, bill_ident, ++seg_count));
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
			THROW(Write_UNT(_doc, bill_ident, ++seg_count));
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

static const EdiIdSymb EanComPartyQSymbList[] = {
	{ EDIPARTYQ_BUYER, "BY" },
	{ EDIPARTYQ_CORPOFFICE, "CO" },
	{ EDIPARTYQ_DELIVERY, "DP" },
	{ EDIPARTYQ_INVOICEE, "IV" },
	{ EDIPARTYQ_STORENUMBER, "SN" },
	{ EDIPARTYQ_SUPPLAGENT, "SR" },
	{ EDIPARTYQ_SUPPLIER, "SU" },
	{ EDIPARTYQ_WAREHOUSEKEEPER, "WH" },
	{ EDIPARTYQ_CONSIGNOR, "CZ" },
};

//static 
int FASTCALL PPEanComDocument::GetPartyqSymb(int refq, SString & rSymb)
	{ return __EdiGetSymbById(EanComPartyQSymbList, SIZEOFARRAY(EanComPartyQSymbList), refq, rSymb); }
//static 
int FASTCALL PPEanComDocument::GetPartyqBySymb(const char * pSymb)
	{ return __EdiGetIdBySymb(EanComPartyQSymbList, SIZEOFARRAY(EanComPartyQSymbList), pSymb); }

class EdiProviderImplementation_Kontur : public PPEdiProcessor::ProviderImplementation {
public:
	SLAPI  EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags);
	virtual SLAPI ~EdiProviderImplementation_Kontur();
	virtual int    SLAPI  GetDocumentList(PPEdiProcessor::DocumentInfoList & rList);
	virtual int    SLAPI  ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList);
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

int SLAPI PPEdiProcessor::ReceiveDocument(const DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->ReceiveDocument(pIdent, rList));
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
						if(left.IsEqiAscii("Desadv"))
							entry.EdiOp = PPEDIOP_DESADV;
						else if(left.IsEqiAscii("alcodesadv") || left.IsEqiAscii("alcrpt") || left.IsEqiAscii("alcdes"))
							entry.EdiOp = PPEDIOP_ALCODESADV;
						else if(left.IsEqiAscii("Aperak"))
							entry.EdiOp = PPEDIOP_APERAK;
						else if(left.IsEqiAscii("Recadv"))
							entry.EdiOp = PPEDIOP_RECADV;
						else if(left.IsEqiAscii("OrdRsp"))
							entry.EdiOp = PPEDIOP_ORDERRSP;
						else if(left.IsEqiAscii("Orders") || left.IsEqiAscii("Order"))
							entry.EdiOp = PPEDIOP_ORDER;
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

int SLAPI EdiProviderImplementation_Kontur::ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	if(pIdent && pIdent->Box.NotEmpty() && pIdent->SId.NotEmpty()) {
		InetUrl url;
		if(Epp.MakeUrl(0, url)) {
			int    prot = url.GetProtocol();
			if(prot == InetUrl::protUnkn) {
				url.SetProtocol(InetUrl::protFtp);
			}
			if(prot == InetUrl::protFtp) {
				const char * p_box = pIdent->Box;
				SString temp_buf;
				ScURL  curl;
				THROW(p_ctx = xmlNewParserCtxt());
				(temp_buf = p_box).SetLastDSlash().Cat(pIdent->SId);
				url.SetComponent(url.cPath, temp_buf);

				GetTempInputPath(pIdent->EdiOp, temp_buf);
				temp_buf.SetLastSlash().Cat(pIdent->SId);
				if(!fileExists(temp_buf)) {
					THROW_SL(curl.FtpGet(url, ScURL::mfVerbose, temp_buf, 0, 0));
				}
				{
					PPEanComDocument s_doc(this);
					s_doc.Read_Document(p_ctx, temp_buf, rList);
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	xmlFreeParserCtxt(p_ctx);
	return ok;
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
