// CHECKFMT.CPP
// Copyright (c) V.Nasonov 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

SLAPI SlipLineParam::SlipLineParam()
{
	Init();
}

void SLAPI SlipLineParam::Init()
{
	Font = 0;
	Kind = 0;
	Flags = 0;
	Qtty = 0.0;
	Price = 0.0;
	VatRate = 0.0;
	DivID = 0;
	FontSize = 0;
	MEMSZERO(PictCoord);
	FontName = 0;
	PictPath = 0;
	BarcodeStd = 0;
	BarcodeWd = 0;
	BarcodeHt = 0;
	Text = 0; // @v9.5.7
	Code = 0; // @v9.5.7
}

class PPSlipFormatZone;

struct PPSlipFormatEntry {
	PPSlipFormatEntry();
	~PPSlipFormatEntry();
	enum {
		fSpace       = 0x0001,
		fAdjRight    = 0x0002,
		fAdjLeft     = 0x0004,
		fAdjCenter   = 0x0008,
		fIfElse      = 0x0010, // Условный оператор. Условие определяется выражением {left(Text) Condition right(Text)}
			// Здесь left(Text) и right(Text) - результаты разбиения текста на две части функцией SString::Divide(';')
		fRegRibbon   = 0x0020, // Печать на регулярную ленту
		fJRibbon     = 0x0040, // Печать на журнальную ленту
		fFiscal      = 0x0080, // Фискальная строка отчета
		fWrap        = 0x0100, // Признак разбиения строки (кол-во символов - в Condition)
		fSignBarcode = 0x0200, // Элемент представляет отображение штрихкода подписи чека (BarcodeId != 0)
		fItemBarcode = 0x0400, // Элемент представляет отображение штрихкода товара (BarcodeId != 0)
			// @#{fSignBarcode^fItemBarcode}
		fFakeCcQr    = 0x0800  // @v9.6.11 Элемент представляет фейковый QR-код ОФД по чеку
	};
	enum {
		cEq = 1,
		cNEq,
		cGt,
		cGe,
		cLt,
		cLe
	};
	long   Flags;
	int    Condition;
	int    FontId;
	int    PictId;
	int    BarcodeId;
	SString Text;
	PPSlipFormatZone * P_TrueZone;
	PPSlipFormatZone * P_FalseZone;
};

class PPSlipFormatZone : public TSCollection <PPSlipFormatEntry> {
public:
	enum {
		kInner = 0, // Внутренняя зона (вложенные операторы)
		kHeader,
		kFooter,
		kDetail,
		kPaymDetail
	};
	PPSlipFormatZone(int kind);

	int    Kind;
};
//
//
//
PPSlipFormatEntry::PPSlipFormatEntry()
{
	Flags = 0;
	Condition = 0;
	FontId = 0;
	PictId = 0;
	BarcodeId = 0;
	P_TrueZone = 0;
	P_FalseZone = 0;
}

PPSlipFormatEntry::~PPSlipFormatEntry()
{
	delete P_TrueZone;
	delete P_FalseZone;
}
//
//
//
PPSlipFormatZone::PPSlipFormatZone(int kind) : TSCollection <PPSlipFormatEntry> ()
{
	assert(oneof5(kind, kInner, kHeader, kFooter, kDetail, kPaymDetail)); // @v8.4.1 kPaymDetail
	Kind = kind;
}

class PPSlipFormat {
public:
	struct Iter {
		Iter()
		{
			memzero(this, offsetof(Iter, Stack));
		}
		Iter & Reset()
		{
			memzero(this, offsetof(Iter, Stack));
			Stack.freeAll();
			return *this;
		}
		int    GetOuterZoneKind() const
		{
			int    zone_kind = -1;
			if(P_Zone) {
				zone_kind = P_Zone->Kind;
				for(uint stk_ptr = Stack.getPointer(); stk_ptr && zone_kind == PPSlipFormatZone::kInner;) {
					const SI * p_si = (const SI *)Stack.at(--stk_ptr);
					zone_kind = (p_si && p_si->P_Zone) ? p_si->P_Zone->Kind : -1;
				}
			}
			return zone_kind;
		}

		long   SrcItemNo;
		long   EntryNo;
		//
		long   SrcItemsCount;
		long   PrintedLineNo; // Номер строки на листе
		uint   PageWidth;     // ==PPSlipFormat::PageWidth  Справочное поле
		uint   PageLength;    // ==PPSlipFormat::PageLength Справочное поле
		uint   RegTo;         // ==PPSlipFormat::RegTo      Справочное поле
		double Qtty;          // Для строки чека (документа) - абсолютное количество, в остальных случаях - 0
		double Price;         // Для строки чека (документа) - чистая цена (с учетом скидки), в остальных случаях - 0
		double Amount;        // Для строки чека (документа) - чистая сумма по строке, для "дна" чека (документа) -
			// сумма к оплате, для кассовой сессии - общая сумма выручки (с учетом возвратов).
		double VatRate;       // @v9.7.1 Для строки чека (документа) - ставка НДС (в процентах)
		int16  DivID;         // Для строки чека (документа) - ИД отдела, в остальных случаях - 0
		uint16 Reserve;       // @alignment
		long   GoodsID;       // @v9.5.7
		// @v9.5.7 char   PictPath[256];
		char   Text[256];     // @v9.5.7
		char   Code[32];      // @v9.5.7
		RECT   PictCoord;

		const  PPSlipFormatZone  * P_Zone;
		const  PPSlipFormatEntry * P_Entry;
		struct SI {
			const PPSlipFormatZone * P_Zone;
			long  EntryNo;
		};
		TSStack <SI> Stack; // @anchor
	};

	PPSlipFormat();
	~PPSlipFormat();
	int    Parse(const char * pFileName, const char * pFormatName);
	int    GetFormList(const char * pFileName, StrAssocArray * pList, int getSlipDocForms);

	int    SetSource(const CCheckPacket *);
	int    SetSource(const PPBillPacket *);
	int    SetSource(const CSessInfo * pInfo);

	int    InitIteration(int zoneKind, Iter *);
	int    NextIteration(Iter *, SString & rBuf);

	int    Init(const char * pFileName, const char * pFormatName, SlipDocCommonParam * pParam);
	int    InitIteration(const CCheckPacket *);
	int    InitIteration(const PPBillPacket *);
	int    InitIteration(const CSessInfo * pInfo);
	int    NextIteration(SString & rBuf, SlipLineParam * pParam);
private:
	friend class SLTEST_CLS(PPSlipFormatLexer);

	void   Helper_InitIteration();
	int    NextToken(SFile & rFile, SString & rResult);
	int    ParseZone(SFile & rFile, SString & rTokResult, int prec, PPSlipFormatZone * pZone);
	int    AddZone(PPSlipFormatZone * pZone);
	int    ResolveString(const Iter * pIter, const char * pExpr, SString & rResult, int * pSplitPos, int * pSplitChr);
	int    CheckCondition(const Iter * pIter, const SString & rText, int condition);
	int    GetCurCheckItem(const Iter * pIter, CCheckLineTbl::Rec * pRec, CCheckPacket::LineExt * pExtRec = 0);
	int    GetCurBillItem(const Iter * pIter, PPTransferItem * pRec);
	int    GetCurCheckPaymItem(const Iter * pIter, CcAmountEntry * pPaymEntry) const;
	PPID   GetSCardID(const Iter * pIter, double * pAdjSum = 0) const;
	int    NextOuterIter(Iter * pIter);
	int    WrapText(const char * pText, uint maxLen, SString & rHead, SString & rTail, int * pWrapPos);

	enum {
		tokEOL = 1,         // Конец строки
		tokEOF,             // Конец файла
		tokComment,         // Комментарий (//)
		tokIdent,           // Символьный идентификатор (abc, _xyz, q200 и т.д.)
		tokString,          // Строковая константа ("...")
		tokNumber,          // Числовая константа
		tokZone,            // Начало зоны (.name)
		tokLBrace,          // {
		tokRBrace,          // }
		tokLPar,            // (
		tokRPar,            // )
		tokComma,           // ,
		tokMetavar,         // Метапеременная (@var)
		tokEq,              // =
		tokEq2,             // ==
		tokNEq,             // !=,<>
		tokGt,              // >
		tokGe,              // >=
		tokLt,              // <
		tokLe,              // <=
		_tokFirstWordToken, // @anchor
		tokSlip,            // slip
		tokIf,              // if
		tokElse,            // else
		tokEndif,           // endif
		tokTitle,           // title
		tokDocNo,           // number
		tokPageWidth,       // pagewidth
		tokPageLength,      // pagelength
		tokSpace,           // space
		tokAlign,           // align
		tokLeft,            // left
		tokRight,           // right
		tokCenter,          // center
		tokPaper,           // paper
		tokRegular,         // regular
		tokJournal,         // journal
		tokFont,            // font
		tokFiscal,          // fiscal
		tokHeadLines,       // headlines
		tokWrap,            // wrap
		tokPicture,         // picture
		tokTextOutput,      // textoutput // @vmiller
		tokBarcode,         // barcode width height [textabove|textbelow|textnone]
		tokSignBarcode,     // signbarcode std width height [textabove|textbelow|textnone]
		tokFakeCcQrCode,    // fakeccqrcode // @v9.6.11
		_tokLastWordToken   // @anchor
	};

	uint     LineNo;
	SString  LineBuf;
	SStrScan Scan;
	SString  Name;
	SString  Title;
	SString  DocNumber;
	uint     PageWidth;
	uint     PageLength;
	uint     HeadLines;
	uint     RegTo; // SlipLineParam::regtoXXX
	uint	 TextOutput; // @vmiller
	int      IsWrap;
	long     LastPictId;
	TSCollection <PPSlipFormatZone> ZoneList;
	SString  VarString;
	SString  MetavarList;
	enum {
		srcCCheck = 1,   // CCheckPacket
		srcGoodsBill,    // PPBillPacket
		srcCSession      // CSessionTbl::Rec
	};
	int    Src; // Источник данных
	union {
		const CCheckPacket * P_CcPack;
		const PPBillPacket * P_BillPack;
		const CSessInfo    * P_SessInfo;
	};
	struct OnLoginData {
		PPObjPerson   PsnObj;
		PPObjGoods    GObj;
		PPObjSCard    ScObj;
		PPObjCSession CsObj;
	};
	struct FontBlock {
		int    Id;
		int    Size;
		int    PageWidth;
		char   Face[64];
	};
	struct PictBlock {
		int    Id;
		char   Path[256];
		RECT   Coord;
	};
	struct BarcodeBlock {
		enum {
			fTextAbove = 0x0001,
			fTextBelow = 0x0002
		};
        int    Id;
        long   Flags;
        int    BcStd;
        int    Width;
        int    Height;
        char   Code[256];
	};
	TSArray <FontBlock> FontList;
	TSArray <PictBlock> PictList;
	TSArray <BarcodeBlock> BcList;
	OnLoginData * P_Od;
	uint    CurZone;
	Iter    CurIter;
	double  RunningTotal; // Специальная накопительная сумма, используемая для правильного округления строк чека
	SString LastFileName;
	SString LastFormatName;
};

PPSlipFormat::PPSlipFormat()
{
	LineNo     = 0;
	PageWidth  = 0;
	PageLength = 0;
	LastPictId = 0;
	Src = 0;
	P_CcPack = 0;
	P_Od   = 0;
	RegTo  = 0;
	IsWrap = 0;
	RunningTotal = 0.0;
	PPLoadText(PPTXT_SLIPFMT_KEYW, VarString);
	PPLoadText(PPTXT_SLIPFMT_METAVAR, MetavarList);
	CurZone = 0;
	TextOutput = 0;
}

PPSlipFormat::~PPSlipFormat()
{
	delete P_Od;
}

int PPSlipFormat::SetSource(const CCheckPacket * pCcPack)
{
	P_CcPack = pCcPack;
	Src = pCcPack ? srcCCheck : 0;
	return 1;
}

int PPSlipFormat::SetSource(const PPBillPacket * pBillPack)
{
	P_BillPack = pBillPack;
	Src = pBillPack ? srcGoodsBill : 0;
	return 1;
}

int PPSlipFormat::SetSource(const CSessInfo * pInfo)
{
	P_SessInfo = pInfo;
	Src = pInfo ? srcCSession : 0;
	return 1;
}

int PPSlipFormat::InitIteration(int zoneKind, Iter * pIter)
{
	uint   i;
	SETIFZ(P_Od, new OnLoginData);
	pIter->SrcItemNo = 0;
	pIter->EntryNo = 0;
	pIter->SrcItemsCount = -1;
	pIter->PrintedLineNo = 0;
	pIter->PageWidth  = PageWidth;
	pIter->PageLength = PageLength;
	pIter->RegTo = RegTo;
	pIter->Qtty = pIter->Price = pIter->Amount = 0.0;
	pIter->P_Zone = 0;
	pIter->P_Entry = 0;
	pIter->Stack.freeAll();
	for(i = 0; !pIter->P_Zone && i < ZoneList.getCount(); i++) {
		if(ZoneList.at(i)->Kind == zoneKind) {
			pIter->P_Zone = ZoneList.at(i);
			if(zoneKind == PPSlipFormatZone::kDetail) {
				if(P_CcPack)
					if(Src == srcCCheck)
						pIter->SrcItemsCount = P_CcPack->GetCount();
					else if(Src == srcGoodsBill)
						pIter->SrcItemsCount = P_BillPack->GetTCount();
			}
			else if(zoneKind == PPSlipFormatZone::kPaymDetail) {
				if(P_CcPack)
					if(Src == srcCCheck)
						pIter->SrcItemsCount = P_CcPack->AL_Const().getCount();
			}
		}
	}
	if(P_CcPack)
		if(Src == srcCCheck)
			pIter->Amount = MONEYTOLDBL(P_CcPack->Rec.Amount);
		else if(Src == srcGoodsBill)
			pIter->Amount = P_BillPack->Rec.Amount;
		else if(Src == srcCSession)
			pIter->Amount = P_SessInfo->Rec.Amount;
	return BIN(pIter->P_Zone);
}



int PPSlipFormat::GetCurCheckItem(const Iter * pIter, CCheckLineTbl::Rec * pRec, CCheckPacket::LineExt * pExtItem)
{
	int    ok = 0;
	if(pIter && pIter->P_Zone) {
		const int zone_kind = pIter->GetOuterZoneKind();
		if(zone_kind == PPSlipFormatZone::kDetail) {
			if(pIter->SrcItemNo < (long)P_CcPack->GetCount()) {
				ASSIGN_PTR(pRec, P_CcPack->GetLine(pIter->SrcItemNo));
				if(pExtItem)
					P_CcPack->GetLineExt(pIter->SrcItemNo+1, *pExtItem);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPSlipFormat::GetCurCheckPaymItem(const Iter * pIter, CcAmountEntry * pPaymEntry) const
{
	int    ok = 0;
	if(pIter && pIter->P_Zone) {
		const int zone_kind = pIter->GetOuterZoneKind();
		if(zone_kind == PPSlipFormatZone::kPaymDetail) {
			const CcAmountList & r_al = P_CcPack->AL_Const();
			if(pIter->SrcItemNo < (long)r_al.getCount()) {
				ASSIGN_PTR(pPaymEntry, r_al.at(pIter->SrcItemNo));
				ok = 1;
			}
		}
	}
	return ok;
}

int PPSlipFormat::GetCurBillItem(const Iter * pIter, PPTransferItem * pRec)
{
	if(pIter->SrcItemNo < (long)P_BillPack->GetTCount()) {
		ASSIGN_PTR(pRec, P_BillPack->TI(pIter->SrcItemNo));
		return 1;
	}
	else
		return 0;
}

enum {
	symbCurDate = 1,       // DATE
	symbCurTime,           // TIME
	symbMainOrg,           // MAINORG
	symbDate,              // DATE
	symbTime,              // TIME
	symbBillNo,            // BILLNO
	symbClient,            // CLIENT
	symbAgent,             // AGENT
	symbCashier,           // CASHIER
	symbTable,             // TABLE
	symbAmountWoDis,       // AMOUNTWODIS
	symbAmount,            // AMOUNT
	symbDiscount,          // DISCOUNT
	symbIsCreditSCard,     // ISCREDITSCARD 1 если карта является кредитной, 0 - в противном случае
	symbSCardRest,         // SCARDREST     Остаток на кредитной карте
	symbSCardOwner,        // SCARDOWNER    Наименование владельца карты
	symbSCardType,         // SCARDTYPE     Тип карты: 0 - дисконтная, 1 - кредитная, 2 - бонусная //
	symbSCard,             // SCARD         Номер карты
	symbPctDis,            // PCTDIS
	symbWrOffAmount,       // WROFFAMOUNT
	symbDeficit,           // DEFICIT
	symbGoodsGroup,        // GOODSGRP
	symbGoods,             // GOODS
	symbQtty,              // QTTY
	symbPriceWoDis,        // PRICEWODIS
	symbPrice,             // PRICE
	symbItemDisAmt,        // ITEMDISAMT
	symbItemDisPct,        // ITEMDISPCT    Процент скидки по строке
	symbItemDis,           // ITEMDIS
	symbItemAmtWoDis,      // ITEMAMTWODIS
	symbItemAmt,           // ITEMAMT
	symbMemo,              // MEMO
	symbPosNode,           // POSNODE Имя кассового узла
	symbPos,               // POS     Имя кассового аппарата
	symbIsRet,             // ISRET
	symbReturn,            // RETURN
	symbRetCashAmount,     // RETCASHAMOUNT
	symbRetBankAmount,     // RETBANKAMOUNT
	symbCashAmount,        // CASHAMOUNT
	symbBankAmount,        // BANKAMOUNT
	symbCheckCount,        // CHECKCOUNT
	symbCashCount,         // CHECKCASHCOUNT
	symbBankCount,         // CHECKBANKCOUNT
	symbRetCheckCount,     // RETCHECKCOUNT
	symbRetCashCount,      // RETCASHCOUNT
	symbRetBankCount,      // RETBANKCOUNT
	symbDivision,          // DIVISION
	symbChecksAmountWoDis, // CAMOUNTWODIS
	symbChecksAmount,      // CAMOUNT
	symbChecksDiscount,    // CDISCOUNT
	symbItemNo,            // ITEMNO
	symbBringAmount,       // BRINGAMOUNT
	symbDeliveryAmount,    // DELIVERYAMOUNT
	symbFiscalAmount,      // FISCALAMOUNT
	symbNonFiscalAmount,   // NONFISCALAMOUNT
	symbIsOrder,           // ISORDER
	symbOrderDate,         // ORDERDATE
	symbOrderTimeStart,    // ORDERTIMESTART
	symbOrderTimeEnd,      // ORDERTIMEEND
	symbOrderDuration,     // ORDERDURATION
	symbLinkOrderNo,       // LINKORDERNO          Номер чека заказа, к которому привязан данный чек
	symbLinkOrderDate,     // LINKORDERDATE        Дата чека заказа, к которому привязан данный чек
	symbDlvrCity,          // DLVRCITY             Город доставки
	symbDlvrAddr,          // DLVRADDR             Адрес доставки
	symbDlvrPhone,         // DLVRPHONE            Телефон доставки
	symbInitTime,          // INITTIME             Дата/время создания чека
	symbIsGiftQuot,        // ISGIFTQUOT           Цена на позицию установлена по подарочной котировке
	symbUhttScHash,        // UHTTSCHASH           Код для просмотра информации о бонусной карте Universe-HTT
	symbPhQtty,            // PHQTTY               Количество в физических единицах
	symbPhUnit,            // PHUNIT               Наименование физической единицы
	symbPaymType,          // PAYMTYPE             @v8.4.1 Тип оплаты (для итератора по оплатам)
	symbPaymAmt,           // PAYMAMT              @v8.4.1 Сумма оплаты (для итератора по оплатам)
	symbTableN,            // TABLN                @v8.4.1 Номер стола. В отличии от symbTable, значение которого может
		// быть как номером, так и именем стола, symbTableN - всегда номер
	symbIsEgais,           // ISEGAIS
	symbEgaisUrl,          // EGAISURL
	symbEgaisSign,         // EGAISSIGN
	symbManufSerial,       // MANUFSERIAL
	symbDirector,          // DIRECTOR   @v9.7.6 Директор
	symbAccountant         // ACCOUNTANT @v9.7.6 Главный бухгалтер
};

PPID PPSlipFormat::GetSCardID(const Iter * pIter, double * pAdjSum) const
{
	PPID   sc_id = 0;
	double adj_amt = 0.0;
	if(Src == srcCCheck) {
		if(P_CcPack) {
			CcAmountEntry paym_entry;
			if(GetCurCheckPaymItem(pIter, &paym_entry)) {
				if(paym_entry.Type == CCAMTTYP_CRDCARD) {
					sc_id = paym_entry.AddedID;
					if(!P_CcPack->Rec.ID)
						adj_amt = fabs(paym_entry.Amount);
				}
			}
			else {
				sc_id = P_CcPack->Rec.SCardID;
				if(!P_CcPack->Rec.ID)
					adj_amt = fabs(MONEYTOLDBL(P_CcPack->Rec.Amount));
			}
		}
	}
	else if(Src == srcGoodsBill) {
		if(P_BillPack) {
			sc_id = P_BillPack->Rec.SCardID;
			if(!P_BillPack->Rec.ID)
				adj_amt = P_BillPack->Rec.Amount;
		}
	}
	ASSIGN_PTR(pAdjSum, adj_amt);
	return sc_id;
}

int PPSlipFormat::ResolveString(const Iter * pIter, const char * pExpr, SString & rResult, int * pSplitPos, int * pSplitChr)
{
	const CCheckPacket * p_ccp = P_CcPack;
	const PPBillPacket * p_bp = P_BillPack;

	int    ok = -1;
	CCheckLineTbl::Rec cc_item;
	CCheckPacket::LineExt ccext_item;
	PersonTbl::Rec psn_rec;
	PPTransferItem ti;
	StringSet ss(';', MetavarList);
	rResult = 0;
	SString temp_buf;
	SStrScan scan(pExpr);
	const long cc_flags = p_ccp ? p_ccp->Rec.Flags : 0;
	while(*scan) {
		if(*scan == '@') {
			int  var_idx = 0;
			PPID temp_id = 0;
			scan.Incr();
			for(uint i = 0, j = 0; !var_idx && ss.get(&i, temp_buf); j++) {
				if(strnicmp(temp_buf, scan, temp_buf.Len()) == 0) {
					var_idx = j+1;
					scan.Incr(temp_buf.Len());
				}
			}
			switch(var_idx) {
				case symbCurDate: rResult.Cat(getcurdate_()); break;
				case symbCurTime: rResult.Cat(getcurtime_()); break;
				case symbMainOrg: rResult.Cat(GetMainOrgName(temp_buf)); break;
				case symbDate:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Rec.Dt);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Dt);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Dt);
					break;
				case symbTime:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Rec.Tm);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Tm);
					break;
				case symbBillNo:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Rec.Code);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Code);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.SessNumber);
					break;
				case symbClient:
					if(Src == srcGoodsBill) {
						GetArticleName(p_bp->Rec.Object, temp_buf);
						rResult.Cat(temp_buf);
					}
					break;
				case symbAgent:
					if(Src == srcCCheck)
						temp_id = p_ccp->Ext.SalerID;
					else if(Src == srcGoodsBill)
						temp_id = p_bp->Ext.AgentID;
					if(temp_id && GetArticleName(temp_id, temp_buf) > 0)
						rResult.Cat(temp_buf);
					break;
				case symbCashier:
					if(Src == srcCCheck && p_ccp->Rec.UserID) {
						if(P_Od->PsnObj.Fetch(p_ccp->Rec.UserID, &psn_rec) > 0)
							rResult.Cat(psn_rec.Name);
					}
					else if(oneof3(Src, srcCCheck, srcGoodsBill, srcCSession)) {
						PPID   psn_id = 0;
						if(GetCurUserPerson(&psn_id, &temp_buf) > 0)
							rResult.Cat(temp_buf);
						else {
							GetCurUserName(temp_buf);
							rResult.Cat(temp_buf);
						}
					}
					break;
				case symbDirector: // @v9.7.6
					{
						DS.GetTLA().InitMainOrgData(0);
						if(P_Od->PsnObj.Fetch(CConfig.MainOrgDirector_, &psn_rec) > 0)
							rResult.Cat(psn_rec.Name);
					}
					break;
				case symbAccountant: // @v9.7.6
					{
						DS.GetTLA().InitMainOrgData(0);
						if(P_Od->PsnObj.Fetch(CConfig.MainOrgAccountant_, &psn_rec) > 0)
							rResult.Cat(psn_rec.Name);
					}
					break;
				case symbTable:
					if(Src == srcCCheck) {
                        PPObjCashNode::GetCafeTableName(p_ccp->Ext.TableNo, temp_buf);
						rResult.Cat(temp_buf);
					}
					break;
				case symbTableN:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Ext.TableNo);
					break;
				case symbAmount:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Amount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Amount, SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Amount, SFMT_MONEY);
					break;
				case symbChecksAmount:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Amount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Amount, SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.Amount, SFMT_MONEY);
					break;
				case symbDiscount:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Discount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)                               // PPBillPacket AmtList PPBill
						rResult.Cat(p_bp->Amounts.Get(PPAMT_DISCOUNT, 0), SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Discount, SFMT_MONEY);
					break;
				case symbChecksDiscount:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Discount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)                               // PPBillPacket AmtList PPBill
						rResult.Cat(p_bp->Amounts.Get(PPAMT_DISCOUNT, 0), SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.Discount, SFMT_MONEY);
					break;
				case symbAmountWoDis:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Amount) + MONEYTOLDBL(p_ccp->Rec.Discount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Amount + p_bp->Amounts.Get(PPAMT_DISCOUNT, 0), SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Amount + P_SessInfo->Rec.Discount, SFMT_MONEY);
					break;
				case symbChecksAmountWoDis:
					if(Src == srcCCheck)
						rResult.Cat(fabs(MONEYTOLDBL(p_ccp->Rec.Amount) + MONEYTOLDBL(p_ccp->Rec.Discount)), SFMT_MONEY);
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Amount + p_bp->Amounts.Get(PPAMT_DISCOUNT, 0), SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.Amount + P_SessInfo->Total.Discount, SFMT_MONEY);
					break;
				case symbReturn:         // RETURN
					if(Src == srcCCheck) {
						rResult.Cat(((cc_flags & CCHKF_RETURN) ? fabs(MONEYTOLDBL(p_ccp->Rec.Amount)) : 0.0), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetOpType(p_bp->Rec.OpID) == PPOPT_GOODSRETURN)
							rResult.Cat(p_bp->Rec.Amount, SFMT_MONEY);
						else
							rResult.Cat(0.0, SFMT_MONEY);
					}
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.WORetAmount - P_SessInfo->Rec.Amount, SFMT_MONEY);
					break;
				case symbCashAmount:     // CASHAMOUNT
					if(Src == srcCCheck) {
						rResult.Cat(((cc_flags & CCHKF_BANKING) ? 0.0 : fabs(MONEYTOLDBL(p_ccp->Rec.Amount))), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(p_bp->Rec.Amount, SFMT_MONEY);
					}
					else {
						rResult.Cat(P_SessInfo->Total.Amount - P_SessInfo->Total.BnkAmount, SFMT_MONEY);
					}
					break;
				case symbRetCashAmount: // RETCASHAMOUNT
					if(Src == srcCCheck) {
						rResult.Cat(((!(cc_flags & CCHKF_BANKING) && (cc_flags & CCHKF_RETURN)) ? fabs(MONEYTOLDBL(p_ccp->Rec.Amount)) : 0.0), SFMT_MONEY);
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.RetAmount - P_SessInfo->Total.RetBnkAmount, SFMT_MONEY);
					}
					break;
				case symbRetBankAmount: // RETBANKAMOUNT
					if(Src == srcCCheck) {
						rResult.Cat((((cc_flags & CCHKF_BANKING) && (cc_flags & CCHKF_RETURN)) ? fabs(MONEYTOLDBL(p_ccp->Rec.Amount)) : 0.0), SFMT_MONEY);
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.RetBnkAmount, SFMT_MONEY);
					}
					break;
				case symbCashCount:     // CHECKCASHCOUNT
					if(Src == srcCCheck) {
						rResult.Cat(BIN(!(cc_flags & CCHKF_BANKING) && !(cc_flags & CCHKF_RETURN)));
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.SaleCheckCount - P_SessInfo->Total.SaleBnkCount);
					}
					break;
				case symbBankCount:     // CHECKBANKCOUNT
					if(Src == srcCCheck) {
						rResult.Cat(BIN((cc_flags & CCHKF_BANKING) && !(cc_flags & CCHKF_RETURN)));
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.SaleBnkCount);
					}
					break;
				case symbRetCashCount:  // RETCASHCOUNT
					if(Src == srcCCheck) {
						rResult.Cat(BIN(!(cc_flags & CCHKF_BANKING) && (cc_flags & CCHKF_RETURN)));
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.RetCheckCount - P_SessInfo->Total.RetBnkCount);
					}
					break;
				case symbRetBankCount:  // RETBANKCOUNT
					if(Src == srcCCheck) {
						rResult.Cat(BIN((cc_flags & CCHKF_BANKING) && (cc_flags & CCHKF_RETURN)));
					}
					else if(Src == srcCSession) {
						rResult.Cat(P_SessInfo->Total.RetBnkCount);
					}
					break;
				case symbBankAmount:     // BANKAMOUNT
					if(Src == srcCCheck) {
						rResult.Cat(((p_ccp->Rec.Flags & CCHKF_BANKING) ? fabs(MONEYTOLDBL(p_ccp->Rec.Amount)) : 0.0), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(0.0, SFMT_MONEY);
					}
					else
						rResult.Cat(P_SessInfo->Total.BnkAmount, SFMT_MONEY);
					break;
				case symbFiscalAmount: // FISCALAMOUNT
					if(Src == srcCCheck) {
						double fiscal, nonfiscal;
						p_ccp->HasNonFiscalAmount(&fiscal, &nonfiscal);
						rResult.Cat(fiscal, SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(p_bp->Rec.Amount, SFMT_MONEY);
					}
					else
						rResult.Cat(P_SessInfo->Total.FiscalAmount, SFMT_MONEY); // @v7.5.8 WORetAmount-->FiscalAmount
					break;
				case symbNonFiscalAmount: // NONFISCALAMOUNT
					if(Src == srcCCheck) {
						double fiscal, nonfiscal;
						p_ccp->HasNonFiscalAmount(&fiscal, &nonfiscal);
						rResult.Cat(nonfiscal, SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(0.0, SFMT_MONEY);
					}
					else
						rResult.Cat(P_SessInfo->Total.Amount-P_SessInfo->Total.FiscalAmount, SFMT_MONEY); // @v7.5.8 WORetAmount-->Amount-FiscalAmount
					break;
				case symbBringAmount:
					if(Src == srcCCheck) {
						rResult.Cat(((p_ccp->Rec.Flags & CCHKF_BANKING) ? fabs(MONEYTOLDBL(p_ccp->Rec.Amount)) : p_ccp->_Cash), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(0.0);
					}
					else
						rResult.Cat(0.0);
					break;
				case symbDeliveryAmount:
					if(Src == srcCCheck) {
						rResult.Cat(((p_ccp->Rec.Flags & CCHKF_BANKING) ? 0.0 : p_ccp->_Cash - fabs(MONEYTOLDBL(p_ccp->Rec.Amount))), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						rResult.Cat(0.0);
					}
					else
						rResult.Cat(0.0);
					break;
				case symbSCard:
					temp_id = GetSCardID(pIter);
					if(temp_id) {
						SCardTbl::Rec sc_rec;
						if(P_Od->ScObj.Search(temp_id, &sc_rec) > 0)
							rResult.Cat(sc_rec.Code);
					}
					break;
				case symbSCardOwner:
					temp_id = GetSCardID(pIter);
					if(temp_id) {
						SCardTbl::Rec sc_rec;
						if(P_Od->ScObj.Search(temp_id, &sc_rec) > 0) {
							PersonTbl::Rec psn_rec;
							if(sc_rec.PersonID && P_Od->PsnObj.Fetch(sc_rec.PersonID, &psn_rec) > 0)
								rResult.Cat(psn_rec.Name);
						}
					}
					break;
				case symbIsCreditSCard:
					temp_id = GetSCardID(pIter);
					{
						int    is_credit = BIN(temp_id && P_Od->ScObj.IsCreditCard(temp_id));
						rResult.Cat(is_credit);
					}
					break;
				case symbSCardType:
					temp_id = GetSCardID(pIter);
					{
						int    sc_type = temp_id ? P_Od->ScObj.GetCardType(temp_id) : 0;
						rResult.Cat(sc_type);
					}
					break;
				case symbSCardRest:
					{
						//
						// Если чек (документ) не проведен (id==0) тогда остаток по карте
						// корректируем на величину номинальной суммы чека (документа).
						//
						double adj_amt = 0.0;
						temp_id = GetSCardID(pIter, &adj_amt);
						{
							double rest = 0.0;
							if(temp_id && P_Od->ScObj.IsCreditCard(temp_id)) {
								P_Od->ScObj.P_Tbl->GetRest(temp_id, ZERODATE, &rest);
								rest -= adj_amt;
							}
							rResult.Cat(rest, SFMT_MONEY);
						}
					}
					break;
				case symbUhttScHash:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->UhttScHash);
					break;
				case symbPctDis:
					if(Src == srcCCheck)
						rResult.Cat(MONEYTOLDBL(p_ccp->Rec.Discount) * 100.0 /
							(MONEYTOLDBL(p_ccp->Rec.Amount) + MONEYTOLDBL(p_ccp->Rec.Discount)), MKSFMTD(0, 1, 0));
					else if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Amounts.Get(PPAMT_DISCOUNT, 0) * 100.0 /
							(p_bp->Rec.Amount + p_bp->Amounts.Get(PPAMT_DISCOUNT, 0)), SFMT_MONEY);
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.Discount * 100.0 / (P_SessInfo->Rec.Amount + P_SessInfo->Rec.Discount), SFMT_MONEY);
					break;
				case symbWrOffAmount:
					if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.WrOffAmount, SFMT_MONEY);
					break;
				case symbDeficit:
					if(Src == srcCSession)                                 // CSessionTbl
						rResult.Cat(P_SessInfo->Rec.Amount - P_SessInfo->Rec.WrOffAmount, SFMT_MONEY);
					break;
				case symbItemNo:
					if(oneof2(Src, srcCCheck, srcGoodsBill) && pIter)
						rResult.Cat(pIter->SrcItemNo+1);
					break;
				case symbGoodsGroup:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							temp_id = cc_item.GoodsID;
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							temp_id = ti.GoodsID;
					}
					if(temp_id) {
						PPObjGoods goods_obj;
						Goods2Tbl::Rec goods_rec;
						if(goods_obj.Fetch(temp_id, &goods_rec) > 0) {
							if(GetGoodsNameR(goods_rec.ParentID, temp_buf) > 0)
								rResult.Cat(temp_buf);
						}
					}
					break;
				case symbGoods:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							temp_id = cc_item.GoodsID;
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							temp_id = ti.GoodsID;
					}
					if(temp_id && GetGoodsNameR(temp_id, temp_buf) > 0)
						rResult.Cat(temp_buf);
					break;
				case symbQtty:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(fabs(cc_item.Quantity), MKSFMTD(0, 3, NMBF_NOTRAILZ));
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(fabs(ti.Quantity_), MKSFMTD(0, 3, NMBF_NOTRAILZ));
					}
					break;
				case symbPhQtty:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item)) {
							PPObjGoods goods_obj;
							PPID   ph_unit_id = 0;
							double phuperu = 0.0;
							if(goods_obj.GetPhUPerU(cc_item.GoodsID, &ph_unit_id, &phuperu) > 0 && phuperu > 0.0) {
								rResult.Cat(fabs(cc_item.Quantity * phuperu), MKSFMTD(0, 3, NMBF_NOTRAILZ));
							}
						}
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti)) {
							PPObjGoods goods_obj;
							PPID   ph_unit_id = 0;
							double phuperu = 0.0;
							if(goods_obj.GetPhUPerU(labs(ti.GoodsID), &ph_unit_id, &phuperu) > 0 && phuperu > 0.0) {
								rResult.Cat(fabs(ti.Quantity_ * phuperu), MKSFMTD(0, 3, NMBF_NOTRAILZ));
							}
						}
					}
					break;
				case symbPhUnit:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item)) {
							PPObjGoods goods_obj;
							PPUnit u_rec;
							PPID   ph_unit_id = 0;
							double phuperu = 0.0;
							if(goods_obj.GetPhUPerU(cc_item.GoodsID, &ph_unit_id, &phuperu) > 0 && ph_unit_id) {
								if(goods_obj.FetchUnit(ph_unit_id, &u_rec) > 0)
									rResult.Cat(u_rec.Name);
							}
						}
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti)) {
							PPObjGoods goods_obj;
							PPUnit u_rec;
							PPID   ph_unit_id = 0;
							double phuperu = 0.0;
							if(goods_obj.GetPhUPerU(labs(ti.GoodsID), &ph_unit_id, &phuperu) > 0 && ph_unit_id) {
								if(goods_obj.FetchUnit(ph_unit_id, &u_rec) > 0)
									rResult.Cat(u_rec.Name);
							}
						}
					}
					break;
				// @v8.4.1 {
				case symbPaymType:
					if(Src == srcCCheck) {
						CcAmountEntry paym_entry;
						if(Src == srcCCheck && GetCurCheckPaymItem(pIter, &paym_entry)) {
							paym_entry.GetTypeText(temp_buf);
							rResult.Cat(temp_buf);
						}
					}
					break;
				case symbPaymAmt:
					if(Src == srcCCheck) {
						CcAmountEntry paym_entry;
						if(Src == srcCCheck && GetCurCheckPaymItem(pIter, &paym_entry)) {
							rResult.Cat(paym_entry.Amount, SFMT_MONEY);
						}
					}
					break;
				// } @v8.4.1
				case symbPrice:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(fabs(fdiv100i(cc_item.Price) - cc_item.Dscnt), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(fabs(ti.NetPrice()), SFMT_MONEY);
					}
					break;
				case symbPriceWoDis:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(fabs(fdiv100i(cc_item.Price)), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(fabs(ti.Price), SFMT_MONEY);
					}
					break;
				case symbItemDis:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(cc_item.Dscnt, SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(ti.Discount, SFMT_MONEY);
					}
					break;
				case symbItemDisAmt:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(cc_item.Dscnt * cc_item.Quantity, SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(ti.Discount * ti.Quantity_, SFMT_MONEY);
					}
					break;
				case symbItemDisPct:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item)) {
							double amt = fabs(fdiv100i(cc_item.Price) * cc_item.Quantity);
							if(amt != 0.0)
								rResult.Cat(100.0 * fabs(cc_item.Dscnt * cc_item.Quantity) / amt, MKSFMTD(0, 0, 0));
						}
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti)) {
							double amt = fabs(ti.NetPrice() * ti.Quantity_);
							if(amt != 0.0)
								rResult.Cat(100.0 * fabs(ti.Discount * ti.Quantity_) / amt, MKSFMTD(0, 1, 0));
						}
					}
					break;
				case symbItemAmt:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(fabs((fdiv100i(cc_item.Price) - cc_item.Dscnt) * cc_item.Quantity), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(fabs(ti.NetPrice() * ti.Quantity_), SFMT_MONEY);
					}
					break;
				case symbItemAmtWoDis:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item))
							rResult.Cat(fabs(fdiv100i(cc_item.Price) * cc_item.Quantity), SFMT_MONEY);
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti))
							rResult.Cat(fabs(ti.Price * ti.Quantity_), SFMT_MONEY);
					}
					break;
				case symbIsGiftQuot:
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item, &ccext_item))
							rResult.CatChar((ccext_item.Flags & ccext_item.fQuotedByGift) ? '1' : '0');
						else
							rResult.CatChar('0');
					}
					break;
				case symbMemo:
					if(Src == srcGoodsBill)
						rResult.Cat(p_bp->Rec.Memo);
					else if(Src == srcCCheck)
						rResult.Cat(p_ccp->Ext.Memo);
					break;
				case symbPos:
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Rec.CashID);
					/*
					else if(Src == srcGoodsBill) {
					}
					*/
					else if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Rec.CashNumber);
					break;
				case symbPosNode:
					if(Src == srcCCheck) {
						if(p_ccp->Rec.SessID) {
							CSessionTbl::Rec ses_rec;
							if(P_Od->CsObj.Search(p_ccp->Rec.SessID, &ses_rec) > 0) {
								PPObjCashNode cn_obj;
								PPCashNode cn_rec;
								if(cn_obj.Fetch(ses_rec.CashNodeID, &cn_rec) > 0)
									rResult.Cat(cn_rec.Name);
							}
						}
					}
					/*
					else if(Src == srcGoodsBill) {
					}
					*/
					else if(Src == srcCSession) {
						PPObjCashNode cn_obj;
						PPCashNode cn_rec;
						if(cn_obj.Fetch(P_SessInfo->Rec.CashNodeID, &cn_rec) > 0) {
							rResult.Cat(cn_rec.Name);
						}
					}
					break;
				// @v9.6.11 {
				case symbManufSerial:
					if(Src == srcCCheck) {
						if(p_ccp->Rec.SessID) {
							CSessionTbl::Rec ses_rec;
							if(P_Od->CsObj.Search(p_ccp->Rec.SessID, &ses_rec) > 0) {
								PPObjCashNode cn_obj;
								PPSyncCashNode scn_pack;
								if(cn_obj.GetSync(P_SessInfo->Rec.CashNodeID, &scn_pack) > 0) {
									scn_pack.GetPropString(SCN_MANUFSERIAL, temp_buf);
									rResult.Cat(temp_buf);
								}
							}
						}
					}
					break;
				// } @v9.6.11
				case symbIsRet:
					if(Src == srcCCheck) {
						rResult.Cat((long)BIN(p_ccp->Rec.Flags & CCHKF_RETURN));
					}
					else if(Src == srcGoodsBill) {
						long is_ret = 0;
						if(GetOpType(p_bp->Rec.OpID) == PPOPT_GOODSRETURN)
							is_ret = 1;
						rResult.Cat(is_ret);
					}
					else
						rResult.Cat(0L);
					break;
				case symbCheckCount:
					if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.SaleCheckCount);
					break;
				case symbRetCheckCount:
					if(Src == srcCSession)
						rResult.Cat(P_SessInfo->Total.RetCheckCount);
					break;
				case symbDivision:
					if(Src == srcCCheck && GetCurCheckItem(pIter, &cc_item))
						rResult.Cat((long)((cc_item.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? cc_item.DivID - CHECK_LINE_IS_PRINTED_BIAS : cc_item.DivID));
				case symbInitTime:          // INITTIME
					if(Src == srcCCheck)
						rResult.Cat(p_ccp->Ext.CreationDtm, DATF_DMY|DATF_NOZERO, TIMF_HMS|TIMF_NOZERO);
					break;
				case symbIsOrder:           // ISORDER
					if(Src == srcCCheck)
						rResult.Cat((long)(BIN(p_ccp->Rec.Flags & CCHKF_ORDER)));
					break;
				case symbOrderDate:         // ORDERDATE
					if(Src == srcCCheck && p_ccp->Rec.Flags & CCHKF_ORDER)
						rResult.Cat(p_ccp->Ext.StartOrdDtm.d);
					break;
				case symbOrderTimeStart:    // ORDERTIMESTART
					if(Src == srcCCheck && p_ccp->Rec.Flags & CCHKF_ORDER)
						rResult.Cat(p_ccp->Ext.StartOrdDtm.t, TIMF_HM);
					break;
				case symbOrderTimeEnd:      // ORDERTIMEEND
					if(Src == srcCCheck && p_ccp->Rec.Flags & CCHKF_ORDER)
						rResult.Cat(p_ccp->Ext.EndOrdDtm.t, TIMF_HM);
					break;
				case symbOrderDuration:     // ORDERDURATION
					if(Src == srcCCheck && p_ccp->Rec.Flags & CCHKF_ORDER) {
						long sec = diffdatetimesec(p_ccp->Ext.EndOrdDtm, p_ccp->Ext.StartOrdDtm);
						LTIME duration;
						duration.settotalsec(labs(sec));
						rResult.Cat(duration, TIMF_HM);
					}
					break;
				case symbLinkOrderNo:       // LINKORDERNO          Номер чека заказа, к которому привязан данный чек
					if(Src == srcCCheck && P_Od && p_ccp->Ext.LinkCheckID) {
						CCheckTbl::Rec cc_rec;
						if(P_Od->ScObj.P_CcTbl->Search(p_ccp->Ext.LinkCheckID, &cc_rec) > 0)
							rResult.Cat(cc_rec.Code);
					}
					break;
				case symbLinkOrderDate:     // LINKORDERDATE        Дата чека заказа, к которому привязан данный чек
					if(Src == srcCCheck && P_Od && p_ccp->Ext.LinkCheckID) {
						CCheckTbl::Rec cc_rec;
						if(P_Od->ScObj.P_CcTbl->Search(p_ccp->Ext.LinkCheckID, &cc_rec) > 0)
							rResult.Cat(cc_rec.Dt);
					}
					break;
				case symbDlvrCity:
					if(Src == srcCCheck) {
						if(p_ccp->Rec.Flags & CCHKF_DELIVERY) {
							if(P_Od) {
								temp_buf.Z();
								P_Od->PsnObj.LocObj.GetCity(p_ccp->Ext.AddrID, 0, &temp_buf, 1);
								rResult.Cat(temp_buf);
							}
						}
					}
					break;
				case symbDlvrAddr:
					if(Src == srcCCheck) {
						if(p_ccp->Rec.Flags & CCHKF_DELIVERY) {
							if(P_Od) {
								LocationTbl::Rec loc_rec;
								PPID   addr_id = p_ccp->Ext.AddrID;
								if(P_Od->PsnObj.LocObj.Search(p_ccp->Ext.AddrID, &loc_rec) > 0) {
									LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf.Z());
									rResult.Cat(temp_buf);
								}
							}
						}
					}
					break;
				case symbDlvrPhone:
					if(Src == srcCCheck) {
						if(p_ccp->Rec.Flags & CCHKF_DELIVERY) {
							if(P_Od) {
								LocationTbl::Rec loc_rec;
								PPID   addr_id = p_ccp->Ext.AddrID;
								if(P_Od->PsnObj.LocObj.Search(p_ccp->Ext.AddrID, &loc_rec) > 0) {
									LocationCore::GetExField(&loc_rec, LOCEXSTR_PHONE, temp_buf.Z());
									rResult.Cat(temp_buf);
								}
							}
						}
					}
					break;
				case symbIsEgais:           // ISEGAIS
					{
						long   _yes = 0;
						if(Src == srcCCheck && p_ccp && p_ccp->GetExtStrData(CCheckPacket::extssEgaisUrl, temp_buf) && temp_buf.NotEmptyS())
							_yes = 1;
						rResult.Cat(_yes);
					}
					break;
				case symbEgaisUrl:          // EGAISURL
					if(Src == srcCCheck) {
						if(p_ccp) {
							p_ccp->GetExtStrData(CCheckPacket::extssEgaisUrl, temp_buf);
							rResult = temp_buf;
						}
					}
					break;
				case symbEgaisSign:          // EGAISSIGN
					if(Src == srcCCheck) {
						if(p_ccp) {
							p_ccp->GetExtStrData(CCheckPacket::extssSign, temp_buf);
							if(temp_buf.Len() > 32) {
								rResult = 0;
								for(uint p = 0; p < temp_buf.Len();) {
									if(p)
										rResult.Space();
									const uint part = MIN(32, temp_buf.Len()-p);
									rResult.CatN(temp_buf+p, part);
									p += part;
								}
							}
							else
								rResult = temp_buf;
						}
					}
					break;
			}
		}
		else {
			rResult.CatChar(*scan);
			scan.Incr();
		}
	}
	{
		size_t split_pos = 0;
		int    split_chr = ' ';
		if(rResult.Search("\\t", 0, 0, &split_pos)) {
			rResult.ReplaceStr("\\t", " ", 1);
			if(rResult[split_pos+1] == '\\' && rResult[split_pos+2] != 0) {
				split_chr = rResult[split_pos+2];
				rResult.Excise(split_pos+1, 2);
			}
			ASSIGN_PTR(pSplitPos, (int)split_pos);
			ASSIGN_PTR(pSplitChr, split_chr);
			ok = 2;
		}
		else
			ASSIGN_PTR(pSplitPos, -1);
	}
	return ok;
}

int PPSlipFormat::CheckCondition(const Iter * pIter, const SString & rText, int condition)
{
	int    c = 0;
	SString left, right, result;
	double left_num = 0, right_num = 0;
	int    is_left_num = 0, is_right_num = 0;
	rText.Divide(';', left, right);
	{
		ResolveString(pIter, left, result, 0, 0);
		Scan.Set(result, 0);
		Scan.Skip();
		is_left_num = Scan.IsNumber();
		if(is_left_num) {
			Scan.GetNumber(left);
			left_num = left.ToReal();
		}
		else
			left = result;
	}
	{
		ResolveString(pIter, right, result, 0, 0);
		Scan.Set(result, 0);
		Scan.Skip();
		is_right_num = Scan.IsNumber();
		if(is_right_num) {
			Scan.GetNumber(right);
			right_num = right.ToReal();
		}
		else
			right = result;
	}
	switch(condition) {
		case PPSlipFormatEntry::cEq:
			if(is_left_num && is_right_num)
				c = BIN(left_num == right_num);
			else if(is_right_num && right_num == 0)
				c = BIN(left.Empty());
			else if(is_left_num && left_num == 0)
				c = BIN(right.Empty());
			else
				c = BIN(left.CmpNC(right) == 0);
			break;
		case PPSlipFormatEntry::cNEq:
			if(is_left_num && is_right_num)
				c = BIN(left_num != right_num);
			else if(is_right_num && right_num == 0)
				c = BIN(left.NotEmpty());
			else if(is_left_num && left_num == 0)
				c = BIN(right.NotEmpty());
			else
				c = BIN(left.CmpNC(right) != 0);
			break;
		case PPSlipFormatEntry::cGt:
			if(is_left_num && is_right_num)
				c = BIN(left_num > right_num);
			else
				c = BIN(left.CmpNC(right) > 0);
			break;
		case PPSlipFormatEntry::cGe:
			if(is_left_num && is_right_num)
				c = BIN(left_num >= right_num);
			else
				c = BIN(left.CmpNC(right) >= 0);
			break;
		case PPSlipFormatEntry::cLt:
			if(is_left_num && is_right_num)
				c = BIN(left_num < right_num);
			else
				c = BIN(left.CmpNC(right) < 0);
			break;
		case PPSlipFormatEntry::cLe:
			if(is_left_num && is_right_num)
				c = BIN(left_num <= right_num);
			else
				c = BIN(left.CmpNC(right) <= 0);
			break;
	}
	return c;
}

int SLAPI PPSlipFormat::WrapText(const char * pText, uint maxLen, SString & rHead, SString & rTail, int * pWrapPos)
{
	int    ok = 1;
	rHead = 0;
	rTail = 0;
	if(pText) {
		const size_t len = sstrlen(pText);
		size_t p = maxLen;
		if(p > 0 && p < len) {
			size_t temp_pos = p;
			size_t next_pos = p;
			const char * p_wraps = " .,;:()=?!&-";
			int    is_space = 0;
			//while(pText[temp_pos] != ' ')
			while(!strchr(p_wraps, pText[temp_pos])) {
				if(pText[temp_pos] == ' ')
					is_space = 1;
				if(temp_pos)
					temp_pos--;
				else
					break;
			}
			if(temp_pos) {
				p = temp_pos;
				next_pos = temp_pos;
				if(is_space)
					next_pos++;
			}
			rHead.CopyFromN(pText, p);
			rTail.CopyFrom(pText+next_pos);
			ASSIGN_PTR(pWrapPos, (int)p);
		}
		else {
			rHead = pText;
			ASSIGN_PTR(pWrapPos, -1);
			ok = -1;
		}
	}
	else {
		ASSIGN_PTR(pWrapPos, -1);
		ok = 0;
	}
	return ok;
}

int PPSlipFormat::NextOuterIter(Iter * pIter)
{
	++pIter->SrcItemNo;
	if(pIter->SrcItemNo < pIter->SrcItemsCount) {
		pIter->EntryNo = 0;
		return 1;
	}
	else
		return -1;
}

int PPSlipFormat::NextIteration(Iter * pIter, SString & rBuf)
{
	int    ok = -1;
	SString result;
	rBuf.Z();
	if(pIter && pIter->P_Zone) {
		SString temp_buf;
		Goods2Tbl::Rec goods_rec;
		do {
			pIter->DivID = 0;
			pIter->Qtty = pIter->Price = 0.0;
			const PPSlipFormatZone * p_zone = pIter->P_Zone;
			if(pIter->EntryNo < (long)p_zone->getCount()) {
				const PPSlipFormatEntry * p_entry = pIter->P_Entry = p_zone->at(pIter->EntryNo);
				PPGoodsTaxEntry tax_entry;
				if(P_CcPack) {
					CCheckLineTbl::Rec cc_item;
					PPTransferItem ti;
					if(Src == srcCCheck) {
						if(GetCurCheckItem(pIter, &cc_item)) {
							const double s  = intmnytodbl(cc_item.Price) * cc_item.Quantity;
							const double ds = cc_item.Dscnt * cc_item.Quantity;
							const double prev_rt = RunningTotal;
							RunningTotal = R2(RunningTotal + s - ds);
							const double is = RunningTotal - prev_rt;
							pIter->Qtty  = fabs(cc_item.Quantity);
							pIter->Price = is / pIter->Qtty;
							pIter->VatRate = 0.0; // @v9.7.1
							pIter->DivID = (cc_item.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? (cc_item.DivID - CHECK_LINE_IS_PRINTED_BIAS) : cc_item.DivID;
							// @v9.5.7 {
							pIter->GoodsID = cc_item.GoodsID;
							if(P_Od && P_Od->GObj.Fetch(pIter->GoodsID, &goods_rec) > 0) {
								STRNSCPY(pIter->Text, goods_rec.Name);
								P_Od->GObj.GetSingleBarcode(pIter->GoodsID, temp_buf);
								STRNSCPY(pIter->Code, temp_buf);
								if(P_Od->GObj.FetchTax(pIter->GoodsID, P_CcPack->Rec.Dt, 0, &tax_entry) > 0)
									pIter->VatRate = tax_entry.GetVatRate();
							}
							// } @v9.5.7
						}
					}
					else if(Src == srcGoodsBill) {
						if(GetCurBillItem(pIter, &ti)) {
							pIter->Qtty  = R3(fabs(ti.Quantity_));
							pIter->Price = R2(fabs(ti.NetPrice()));
							pIter->VatRate = 0.0; // @v9.7.1
							// @v9.5.7 {
							pIter->GoodsID = labs(ti.GoodsID);
							if(P_Od && P_Od->GObj.Fetch(pIter->GoodsID, &goods_rec) > 0) {
								STRNSCPY(pIter->Text, goods_rec.Name);
								P_Od->GObj.GetSingleBarcode(pIter->GoodsID, temp_buf);
								STRNSCPY(pIter->Code, temp_buf);
								// @v9.7.1 {
								if(P_Od->GObj.FetchTax(pIter->GoodsID, ti.Date, 0, &tax_entry) > 0)
									pIter->VatRate = tax_entry.GetVatRate();
								// } @v9.7.1
							}
							// } @v9.5.7
						}
					}
				}
				if(p_entry->Flags & PPSlipFormatEntry::fIfElse) {
					int    c = CheckCondition(pIter, p_entry->Text, p_entry->Condition);
					const  PPSlipFormatZone * p_branch_zone = c ? p_entry->P_TrueZone : p_entry->P_FalseZone;
					if(p_branch_zone) {
						Iter::SI si;
						si.P_Zone = p_zone;
						si.EntryNo = pIter->EntryNo;
						pIter->Stack.push(si);
						pIter->P_Zone = p_branch_zone;
						pIter->EntryNo = 0;
					}
					else
						pIter->EntryNo++;
					ok = NextIteration(pIter, rBuf); // @recursion
				}
				else if(p_entry->Flags & PPSlipFormatEntry::fSpace) {
					if(p_entry->Text.Empty())
						result.CatCharN(' ', PageWidth);
					else {
						for(uint i = 0; i < PageWidth / p_entry->Text.Len(); i++)
							result.Cat(p_entry->Text);
					}
					rBuf = result;
					pIter->EntryNo++;
					ok = 1;
				}
				else if(p_entry->Flags & PPSlipFormatEntry::fSignBarcode) {
					if(P_CcPack && P_CcPack->GetExtStrData(CCheckPacket::extssEgaisUrl, result) && result.NotEmptyS()) { // @v9.1.8 extssSign-->extssEgaisUrl
					}
					else {
						result = 0;
					}
					rBuf = result;
					pIter->EntryNo++;
					ok = 1;
				}
				else if(p_entry->Flags & PPSlipFormatEntry::fFakeCcQr) {
					int    split_pos = -1;
					int    split_chr = ' ';
					ResolveString(pIter, p_entry->Text, temp_buf, &split_pos, &split_chr);
					{
						long   pict_id = p_entry->PictId;
						uint   pict_pos = 0;
						if(PictList.lsearch(&pict_id, &pict_pos, CMPF_LONG)) {
							PictBlock & r_pb = PictList.at(pict_pos);
							PPBarcode::BarcodeImageParam bip;
							bip.Std = BARCSTD_QR;
							bip.Code = temp_buf;
							bip.OutputFormat = SFileFormat::Png;
							PPMakeTempFileName("fccqr", "png", 0, bip.OutputFileName);
							if(PPBarcode::CreateImage(bip)) {
								STRNSCPY(r_pb.Path, bip.OutputFileName);
								ok = 1;
							}
						}
						rBuf.Z();
						pIter->EntryNo++;
					}
				}
				else {
					int    split_pos = -1;
					int    split_chr = ' ';
					int    alignment = -1;
					ResolveString(pIter, p_entry->Text, result, &split_pos, &split_chr);
					if(p_entry->Flags & PPSlipFormatEntry::fWrap) {
						int    wrap_pos = -1;
						SString buf = result;
						if(IsWrap) {
							if(WrapText(buf, p_entry->Condition, temp_buf, result, &wrap_pos) > 0) {
								IsWrap = 0;
								if(split_pos >= 0) {
									if(split_pos > wrap_pos)
										split_pos -= wrap_pos;
									else if(split_pos == wrap_pos)
										alignment = ADJ_RIGHT;
								}
							}
						}
						else {
							if(WrapText(buf, p_entry->Condition, result, temp_buf, &wrap_pos) > 0) {
								IsWrap = 1;
								if(split_pos >= 0) {
									if(split_pos <= wrap_pos)
										alignment = ADJ_LEFT;
								}
							}
						}
					}
					{
						result.Transf(CTRANSF_INNER_TO_OUTER);
						int    page_width = PageWidth;
						FontBlock fb;
						long   font_id = p_entry->FontId;
						uint   font_pos = 0;
						if(FontList.lsearch(&font_id, &font_pos, CMPF_LONG)) {
							fb = FontList.at(font_pos);
							if(fb.PageWidth > 0)
								page_width = fb.PageWidth;
						}
						else if(font_id == 2)
							page_width = PageWidth / 2;
						if(split_pos >= 0) {
							SString left, right;
							(left = result).Trim(split_pos);
							(right = result).ShiftLeft(split_pos).Strip();
							int    diff = (int)(page_width - right.Len() - 1);
							if(diff <= 0)
								result = right;
							else {
								result = left;
								int    ad = (int)result.Strip().Len();
								if(diff > ad)
									result.CatCharN(split_chr, diff - ad);
								result.Trim(diff).Space().Cat(right);
							}
							result.Trim(page_width);
						}
						else {
							int    adj = -1;
							if(p_entry) {
								if(alignment < 0) {
									if(p_entry->Flags & PPSlipFormatEntry::fAdjLeft)
										adj = ADJ_LEFT;
									else if(p_entry->Flags & PPSlipFormatEntry::fAdjRight)
										adj = ADJ_RIGHT;
									else if(p_entry->Flags & PPSlipFormatEntry::fAdjCenter)
										adj = ADJ_CENTER;
								}
								else
									adj = alignment;
							}
							if(adj >= 0)
								result.Align(page_width, adj);
							else
								result.Trim(page_width);
						}
					}
					rBuf = result;
					if(!IsWrap)
						pIter->EntryNo++;
					ok = 1;
				}
			}
			else if(pIter->Stack.getPointer()) {
				Iter::SI si;
				pIter->Stack.pop(si);
				pIter->P_Zone = si.P_Zone;
				pIter->EntryNo = si.EntryNo+1;
				ok = NextIteration(pIter, rBuf); // @recursion
			}
		} while(ok < 0 && NextOuterIter(pIter) > 0);
	}
	return ok;
}

int PPSlipFormat::AddZone(PPSlipFormatZone * pZone)
{
	ZoneList.insert(pZone);
	return 1;
}

int PPSlipFormat::NextToken(SFile & rFile, SString & rResult)
{
	rResult = 0;
	int    token = 0;
	if(Scan.P_Buf == 0) {
		THROW_SL(rFile.ReadLine(LineBuf));
		LineNo++;
		Scan.Set(LineBuf.Chomp(), 0);
	}
	Scan.Skip();
	switch(Scan[0]) {
		case    0:
			token = tokEOL;
			if(rFile.ReadLine(LineBuf)) {
				LineNo++;
				Scan.Set(LineBuf.Chomp(), 0);
			}
			else
				token = tokEOF;
			break;
		case  '{': token = tokLBrace; Scan.Incr(); break;
		case  '}': token = tokRBrace; Scan.Incr(); break;
		case  '(': token = tokLPar; Scan.Incr(); break;
		case  ')': token = tokRPar; Scan.Incr(); break;
		case  ',': token = tokComma; Scan.Incr(); break;
		case  '=':
			if(Scan[1] == '=') {
				token = tokEq2;
				Scan.Incr(2);
			}
			else {
				token = tokEq;
				Scan.Incr();
			}
			break;
		case '<':
			if(Scan[1] == '=') {
				token = tokLe;
				Scan.Incr(2);
			}
			else if(Scan[1] == '>') {
				token = tokNEq;
				Scan.Incr(2);
			}
			else {
				token = tokLt;
				Scan.Incr();
			}
			break;
		case '>':
			if(Scan[1] == '=') {
				token = tokGe;
				Scan.Incr(2);
			}
			else {
				token = tokGt;
				Scan.Incr();
			}
			break;
		case '!':
			if(Scan[1] == '=') {
				token = tokNEq;
				Scan.Incr(2);
			}
			break;
		case '\"':
			token = Scan.GetQuotedString(rResult) ? tokString : 0;
			rResult.Transf(CTRANSF_OUTER_TO_INNER);
			break;
		case  '/':
			if(Scan[1] == '/') {
				token = tokComment;
				Scan.Incr(2);
				Scan.Skip();
				rResult = Scan;
				Scan.Incr(strlen(Scan));
			}
			break;
		case '.':
			Scan.Incr();
			token = Scan.GetIdent(rResult) ? tokZone : 0;
			break;
		case '@':
			Scan.Incr();
			token = Scan.GetIdent(rResult) ? tokMetavar : 0;
			rResult.Insert(0, "@");
			break;
		default:
			if(Scan.IsNumber()) {
				Scan.GetNumber(rResult);
				token = tokNumber;
			}
			else {
				/* @v8.4.5
				int    var_idx = -1;
				token = Scan.GetIdent(rResult) ? tokIdent : 0;
				THROW_PP_S(token == tokIdent,  PPERR_TOKENEXPECTED, "ident"); // @v8.4.1
				if(PPSearchSubStr(VarString, &var_idx, rResult, 1) &&
					(var_idx >= 0 && var_idx < (_tokLastWordToken-_tokFirstWordToken-1))) {
					token = var_idx + _tokFirstWordToken + 1;
				}
				*/
				// @v8.4.5 Возвращет код до v8.4.1 {
				token = Scan.GetIdent(rResult) ? tokIdent : 0;
				if(token == tokIdent) {
					int    var_idx = -1;
					if(PPSearchSubStr(VarString, &var_idx, rResult, 1) && (var_idx >= 0 && var_idx < (_tokLastWordToken-_tokFirstWordToken-1))) {
						token = var_idx + _tokFirstWordToken + 1;
					}
				}
				// } @v8.4.5 Возвращет код до v8.4.1
			}
			break;
	}
	CATCH
		token = 0;
	ENDCATCH
	return token;
}

int PPSlipFormat::ParseZone(SFile & rFile, SString & rTokResult, int prec, PPSlipFormatZone * pZone)
{
	int    token = 0;
	PPSlipFormatZone * p_zone = 0; // inner zone
	assert(pZone);
	PPSlipFormatEntry * p_entry = 0;
	do {
		token = NextToken(rFile, rTokResult);
		THROW(token);
		if(token == tokString) {
			THROW_MEM(p_entry = new PPSlipFormatEntry);
			p_entry->Text   = rTokResult;
			while((token = NextToken(rFile, rTokResult)) != tokEOL && token != tokEOF) {
				THROW(token);
				switch(token) {
					case tokLeft:    p_entry->Flags |= PPSlipFormatEntry::fAdjLeft; break;
					case tokRight:   p_entry->Flags |= PPSlipFormatEntry::fAdjRight; break;
					case tokCenter:  p_entry->Flags |= PPSlipFormatEntry::fAdjCenter; break;
					case tokFiscal:  p_entry->Flags |= PPSlipFormatEntry::fFiscal; break;
					case tokRegular: p_entry->Flags |= PPSlipFormatEntry::fRegRibbon; break;
					case tokJournal: p_entry->Flags |= PPSlipFormatEntry::fJRibbon; break;
					case tokFont:
						token = NextToken(rFile, rTokResult);
						THROW_PP_S(token == tokEq, PPERR_TOKENEXPECTED, "=");
						token = NextToken(rFile, rTokResult);
						THROW_PP_S(token == tokNumber, PPERR_TOKENEXPECTED, "number");
						p_entry->FontId = (uint16)rTokResult.ToLong();
						break;
					case tokWrap:
						p_entry->Flags |= PPSlipFormatEntry::fWrap;
						token = NextToken(rFile, rTokResult);
						THROW_PP_S(token == tokEq, PPERR_TOKENEXPECTED, "=");
						token = NextToken(rFile, rTokResult);
						THROW_PP_S(token == tokNumber, PPERR_TOKENEXPECTED, "number");
						p_entry->Condition = (uint16)rTokResult.ToLong();
						break;
					default:
						CALLEXCEPT_PP_S(PPERR_SLIPFMT_INVTOKEN, rTokResult);
				}
			}
			pZone->insert(p_entry);
			p_entry = 0;
		}
		else if(token == tokSpace) {
			THROW_MEM(p_entry = new PPSlipFormatEntry);
			p_entry->Flags |= PPSlipFormatEntry::fSpace;
			THROW(token = NextToken(rFile, rTokResult));
			if(token == tokEq) {
				THROW(token = NextToken(rFile, rTokResult));
				THROW_PP_S(token == tokString, PPERR_TOKENEXPECTED, "string");
				p_entry->Text = rTokResult;
				THROW(token = NextToken(rFile, rTokResult));
			}
			THROW_PP_S(token == tokEOL, PPERR_TOKENEXPECTED, "EOL");
			pZone->insert(p_entry);
			p_entry = 0;
		}
		else if(token == tokIf) {
			assert(p_zone == 0);
			ZDELETE(p_zone);
			THROW_MEM(p_entry = new PPSlipFormatEntry);
			p_entry->Flags |= PPSlipFormatEntry::fIfElse;
			{
				// Разбираем конструкцию: "( left_part condition right_part )"
				THROW(token = NextToken(rFile, rTokResult));
				THROW_PP_S(token == tokLPar, PPERR_TOKENEXPECTED, "(");

				THROW(token = NextToken(rFile, rTokResult));
				if(token == tokString || token == tokNumber)
					p_entry->Text.Cat(rTokResult);
				else if(token == tokMetavar)
					p_entry->Text.Cat(rTokResult);
				else
					CALLEXCEPT_PP_S(PPERR_TOKENEXPECTED, "string or number or metavar");
				THROW(token = NextToken(rFile, rTokResult));
				switch(token) {
					case tokEq:  p_entry->Condition = PPSlipFormatEntry::cEq; break;
					case tokEq2: p_entry->Condition = PPSlipFormatEntry::cEq; break;
					case tokNEq: p_entry->Condition = PPSlipFormatEntry::cNEq; break;
					case tokLt:  p_entry->Condition = PPSlipFormatEntry::cLt; break;
					case tokLe:  p_entry->Condition = PPSlipFormatEntry::cLe; break;
					case tokGt:  p_entry->Condition = PPSlipFormatEntry::cGt; break;
					case tokGe:  p_entry->Condition = PPSlipFormatEntry::cGe; break;
					default:
						CALLEXCEPT_PP_S(PPERR_TOKENEXPECTED, "condition operator");
				}
				THROW(token = NextToken(rFile, rTokResult));
				p_entry->Text.Semicol();
				if(token == tokString || token == tokNumber)
					p_entry->Text.Cat(rTokResult);
				else if(token == tokMetavar)
					p_entry->Text.Cat(rTokResult);
				else
					CALLEXCEPT_PP_S(PPERR_TOKENEXPECTED, "string or number or metavar");
			}
			p_zone = new PPSlipFormatZone(PPSlipFormatZone::kInner);
			THROW(token = ParseZone(rFile, rTokResult, tokIf, p_zone)); // @recursion
			p_entry->P_TrueZone = p_zone;
			p_zone = 0;
			if(token == tokElse) {
				p_zone = new PPSlipFormatZone(PPSlipFormatZone::kInner);
				THROW(token = ParseZone(rFile, rTokResult, tokElse, p_zone)); // @recursion
				p_entry->P_FalseZone = p_zone;
				p_zone = 0;
			}
			pZone->insert(p_entry);
			p_entry = 0;
		}
		else if(token == tokElse) {
			THROW_PP(prec == tokIf, PPERR_MISPLACEDELSE);
			break; // @end_of_loop
		}
		else if(token == tokEndif) {
			THROW_PP(oneof2(prec, tokIf, tokElse), PPERR_MISPLACEDENDIF);
			break; // @end_of_loop
		}
		else if(token == tokZone) {
			THROW_PP(prec == 0, PPERR_SLIPFMT_UNEXPENDOFZONE);
			break; // @end_of_loop
		}
		else if(token == tokRBrace) {
			THROW_PP(prec == 0, PPERR_SLIPFMT_UNEXPENDOFZONE);
			break; // @end_of_loop
		}
		else if(token == tokPicture) {
			THROW_MEM(p_entry = new PPSlipFormatEntry);
			p_entry->Text = rTokResult;
			THROW_PP_S(NextToken(rFile, rTokResult) == tokEq, PPERR_TOKENEXPECTED, "=");
			THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
			p_entry->PictId = (uint16)rTokResult.ToLong();
			SETMAX(LastPictId, p_entry->PictId);
			pZone->insert(p_entry);
			p_entry = 0;
		}
		else if(token == tokFakeCcQrCode) {
			//https://receipt.taxcom.ru/AE2AAFC4/v01/show/?rnm={RN}&fn={FN}&i={DN}&fp={DH}
			//fakeccqrcode="https://receipt.taxcom.ru/AE2AAFC4/v01/show/?rnm={RN}&fn={FN}&i={DN}&fp={DH}" 10, 10, 15, 15
			THROW_MEM(p_entry = new PPSlipFormatEntry);
			THROW_PP_S(NextToken(rFile, rTokResult) == tokEq, PPERR_TOKENEXPECTED, "=");
			THROW_PP_S(NextToken(rFile, rTokResult) == tokString, PPERR_TOKENEXPECTED, "string");
			p_entry->Text = rTokResult;
			p_entry->Flags |= PPSlipFormatEntry::fFakeCcQr;
			p_entry->PictId = ++LastPictId;
			{
				PictBlock pb;
				MEMSZERO(pb);
				pb.Id = p_entry->PictId;
				THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
				pb.Coord.top = rTokResult.ToLong();
				THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
				pb.Coord.left = rTokResult.ToLong();
				THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
				pb.Coord.right = rTokResult.ToLong();
				THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
				pb.Coord.bottom = rTokResult.ToLong();
				THROW_PP_S(NextToken(rFile, rTokResult) == tokEOL, PPERR_TOKENEXPECTED, "EOL");
				THROW_PP_S(PictList.lsearch(&pb.Id, 0, PTR_CMPFUNC(long)) == 0, PPERR_SLIPFMT_DUPPICTID, pb.Id);
				THROW_SL(PictList.insert(&pb));
			}
			THROW_SL(pZone->insert(p_entry));
			p_entry = 0;
		}
		else if(token == tokBarcode) {
			//tokBarcode,    // barcode = width height [textabove|textbelow|textnone]
			THROW_MEM(p_entry = new PPSlipFormatEntry);
		}
		else if(token == tokSignBarcode) {
			//tokSignBarcode // signbarcode = std width height [textabove|textbelow|textnone]
			BarcodeBlock bb;
			MEMSZERO(bb);
			THROW_PP_S(NextToken(rFile, rTokResult) == tokIdent, PPERR_TOKENEXPECTED, "barcode standard");
			THROW(bb.BcStd = PPBarcode::RecognizeStdName(rTokResult));
			THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
			bb.Width = rTokResult.ToLong();
			THROW_PP_S(bb.Width > 0 && bb.Width < 1000, PPERR_INVWIDTH, rTokResult);
			THROW_PP_S(NextToken(rFile, rTokResult) == tokNumber, PPERR_TOKENEXPECTED, "number");
			bb.Height = rTokResult.ToLong();
			THROW_PP_S(bb.Height > 0 && bb.Height < 1000, PPERR_INVHEIGHT, rTokResult);
			{
				const int next_tok = NextToken(rFile, rTokResult);
				if(next_tok == tokIdent) {
					rTokResult.ToLower();
					if(rTokResult == "textabove")
						bb.Flags |= bb.fTextAbove;
					else if(rTokResult == "textbelow")
						bb.Flags |= bb.fTextBelow;
					else if(rTokResult == "textnone" || rTokResult == "notext")
						bb.Flags &= ~(bb.fTextBelow|bb.fTextAbove);
				}
				else {
					THROW_PP_S(next_tok == tokEOL, PPERR_SLIPFMT_INVTOKEN, rTokResult);
				}
			}
			bb.Id = (int)(BcList.getCount()+1);
			THROW_SL(BcList.insert(&bb));
			{
				THROW_MEM(p_entry = new PPSlipFormatEntry);
				p_entry->Flags |= PPSlipFormatEntry::fSignBarcode;
				p_entry->BarcodeId = bb.Id;
				pZone->insert(p_entry);
				p_entry = 0;
			}
		}
		else {
			THROW_PP(token != tokEOF, PPERR_UNEXPEOF);
		}
	} while(1);
	CATCH
		token = 0;
	ENDCATCH
	delete p_entry;
	delete p_zone;
	return token;
}

int PPSlipFormat::GetFormList(const char * pFileName, StrAssocArray * pList, int getSlipDocForms)
{
	int    ok = -1;
	int    token = 0;
	long   counter = 0;
	SFile  file;
	SString tok_result, form_name;
	Scan.Set(0, 0);
	THROW_SL(file.Open(pFileName, SFile::mRead));
	do {
		if(!token)
			THROW(token = NextToken(file, tok_result));
		if(token == tokEOF)
			ok = 1; // @end_of_loop
		else if(token == tokSlip) {
			THROW_PP_S(NextToken(file, form_name) == tokIdent,  PPERR_TOKENEXPECTED, "ident");
			THROW_PP_S(NextToken(file, tok_result) == tokLBrace, PPERR_TOKENEXPECTED, "{");
			do {
				THROW(token = NextToken(file, tok_result));
				if(token == tokPaper) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					do {
						token = NextToken(file, tok_result);
						if(token == tokRegular) {
							RegTo |= SlipLineParam::fRegRegular;
							token = NextToken(file, tok_result);
						}
						else if(token == tokJournal) {
							RegTo |= SlipLineParam::fRegJournal;
							token = NextToken(file, tok_result);
						}
						else if(token == tokSlip) {
							RegTo |= SlipLineParam::fRegSlip;
							token = NextToken(file, tok_result);
						}
					} while(token == tokComma);
					THROW_PP(!(RegTo & SlipLineParam::fRegSlip) ||
						!(RegTo & (SlipLineParam::fRegRegular | SlipLineParam::fRegJournal)), PPERR_SLIPFMT_UNCOMPPAPER);
					if(((RegTo & SlipLineParam::fRegSlip) && getSlipDocForms) || (!(RegTo & SlipLineParam::fRegSlip) && !getSlipDocForms))
						pList->Add(++counter, form_name);
				}
			} while(!oneof2(token, tokRBrace, tokEOF));
			THROW_PP_S(token == tokRBrace, PPERR_TOKENEXPECTED, "}");
			RegTo = 0;
			token = 0;
		}
		else
			token = 0;
	} while(ok < 0);
	CATCHZOK
	return ok;
}

int PPSlipFormat::Parse(const char * pFileName, const char * pFormatName)
{
	int    ok = -1;
	PPSlipFormatZone * p_zone = 0;
	if(LastFileName.CmpNC(pFileName) == 0 && LastFormatName.CmpNC(pFormatName) == 0)
		ok = 1;
	else {
		RegTo = 0;
		Title = 0;
		PageWidth  = 0;
		PageLength = 0;
		HeadLines  = 0;
		DocNumber  = 0;
		LineNo = 0;
		ZoneList.freeAll();
		FontList.freeAll();
		PictList.freeAll();
		Scan.Set(0, 0);

		int    token = 0;
		int    zone_kind = -1;
		int    this_slip = 0; // Устанавливается в !0 когда обрабатывается требуемый формат (по имени pFormatName)
		SFile  file;
		SString tok_result;
		THROW_SL(file.Open(pFileName, SFile::mRead));
		do {
			if(!token)
				THROW(token = NextToken(file, tok_result));
			if(token == tokEOF)
				break; // @end_of_loop
			else if(token == tokSlip) {
				THROW_PP_S(NextToken(file, tok_result) == tokIdent,  PPERR_TOKENEXPECTED, "ident");
				if(tok_result.CmpNC(pFormatName) == 0) {
					ok = this_slip = 1;
					Name = tok_result;
					THROW_PP_S(NextToken(file, tok_result) == tokLBrace, PPERR_TOKENEXPECTED, "{");
					token = 0;
				}
				else {
					this_slip = 0;
					do {
						THROW(token = NextToken(file, tok_result));
					} while(!oneof2(token, tokRBrace, tokEOF));
					THROW_PP_S(token == tokRBrace, PPERR_TOKENEXPECTED, "}");
					token = 0;
				}
			}
			else if(this_slip) {
				if(token == tokZone) {
					do {
						//
						// В цикле забираем все подряд идущие зоны (ParseZone вернет нам
						// идентификатор следующей лексемы)
						//
						if(tok_result.CmpNC("header") == 0)
							zone_kind = PPSlipFormatZone::kHeader;
						else if(tok_result.CmpNC("footer") == 0)
							zone_kind = PPSlipFormatZone::kFooter;
						else if(tok_result.CmpNC("detail") == 0)
							zone_kind = PPSlipFormatZone::kDetail;
						else if(tok_result.CmpNC("paymdetail") == 0)
							zone_kind = PPSlipFormatZone::kPaymDetail; // @v8.4.1
						else {
							CALLEXCEPT_PP_S(PPERR_SLIPFMT_INVZONE, tok_result);
						}
						THROW_MEM(p_zone = new PPSlipFormatZone(zone_kind));
						THROW(token = ParseZone(file, tok_result, 0, p_zone));
						THROW(AddZone(p_zone));
						p_zone = 0;
					} while(token == tokZone);
					THROW_PP_S(token == tokRBrace, PPERR_TOKENEXPECTED, "}");
					token = 0;
				}
				else if(token == tokTitle) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(token = tokString, PPERR_TOKENEXPECTED, "string");
					Title = tok_result.Transf(CTRANSF_INNER_TO_OUTER);
					token = 0;
				}
				else if(token == tokHeadLines) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(token = tokNumber, PPERR_TOKENEXPECTED, "number");
					HeadLines = tok_result.ToLong();
					token = 0;
				}
				else if(token == tokDocNo) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(oneof2(token, tokString, tokNumber), PPERR_TOKENEXPECTED, "string or number");
					DocNumber = tok_result;
					token = 0;
				}
				else if(token == tokPageWidth) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(token = tokNumber, PPERR_TOKENEXPECTED, "number");
					PageWidth = tok_result.ToLong();
					token = 0;
				}
				else if(token == tokPageLength) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(token = tokNumber, PPERR_TOKENEXPECTED, "number");
					PageLength = tok_result.ToLong();
					token = 0;
				}
				else if(token == tokPaper) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					do {
						token = NextToken(file, tok_result);
						if(token == tokRegular) {
							RegTo |= SlipLineParam::fRegRegular;
							token = NextToken(file, tok_result);
						}
						else if(token == tokJournal) {
							RegTo |= SlipLineParam::fRegJournal;
							token = NextToken(file, tok_result);
						}
						else if(token == tokSlip) {
							RegTo |= SlipLineParam::fRegSlip;
							token = NextToken(file, tok_result);
						}
					} while(token == tokComma);
					// Не очищаем token чтобы он использовался на следующей итерации цикла
					THROW_PP(!(RegTo & SlipLineParam::fRegSlip) ||
						!(RegTo & (SlipLineParam::fRegRegular|SlipLineParam::fRegJournal)), PPERR_SLIPFMT_UNCOMPPAPER);
				}
				else if(token == tokFont) {
					//
					// font 1 = "Courier New" 10
					// font 2 = "Times New Roman" 14
					//
					FontBlock fb;
					MEMSZERO(fb);

					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					fb.Id = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					THROW_PP_S(NextToken(file, tok_result) == tokString, PPERR_TOKENEXPECTED, "string");
					tok_result.CopyTo(fb.Face, sizeof(fb.Face));
					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					fb.Size = tok_result.ToLong();
					token = NextToken(file, tok_result);
					if(token == tokNumber) {
						fb.PageWidth = tok_result.ToLong();
						token = NextToken(file, tok_result);
					}
					THROW_PP_S(token == tokEOL, PPERR_TOKENEXPECTED, "EOL");
					THROW_PP_S(FontList.lsearch(&fb.Id, 0, CMPF_LONG) == 0, PPERR_SLIPFMT_DUPFONTID, fb.Id);
					THROW_SL(FontList.insert(&fb));
					token = 0;
				}
				else if(token == tokPicture) {
					//
					// picture 1 = "c:\temp\image1.jpg" 10, 10, 15, 15
					// picture 2 = "c:\temp\image2.bmp" 20, 20, 10, 10
					//
					PictBlock pb;
					MEMSZERO(pb);

					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					pb.Id = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					THROW_PP_S(NextToken(file, tok_result) == tokString, PPERR_TOKENEXPECTED, "string");
					tok_result.CopyTo(pb.Path, sizeof(pb.Path));
					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					pb.Coord.top = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					pb.Coord.left = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					pb.Coord.right = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokNumber, PPERR_TOKENEXPECTED, "number");
					pb.Coord.bottom = tok_result.ToLong();
					THROW_PP_S(NextToken(file, tok_result) == tokEOL, PPERR_TOKENEXPECTED, "EOL");
					THROW_PP_S(PictList.lsearch(&pb.Id, 0, PTR_CMPFUNC(long)) == 0, PPERR_SLIPFMT_DUPPICTID, pb.Id);
					THROW_SL(PictList.insert(&pb));
					token = 0;
				}
				// @vmiller {
				else if(token == tokTextOutput) {
					THROW_PP_S(NextToken(file, tok_result) == tokEq, PPERR_TOKENEXPECTED, "=");
					token = NextToken(file, tok_result);
					THROW_PP_S(token = tokNumber, PPERR_TOKENEXPECTED, "number");
					TextOutput = tok_result.ToLong();
					token = 0;
				}
				// } @vmiller
				else {
					THROW_PP(token != tokIf,    PPERR_MISPLACEDIF);
					THROW_PP(token != tokElse,  PPERR_MISPLACEDELSE);
					THROW_PP(token != tokEndif, PPERR_MISPLACEDENDIF);
					token = 0;
				}
			}
			else
				token = 0;
		} while(1);
		if(ok > 0) {
			LastFileName   = pFileName;
			LastFormatName = pFormatName;
		}
		else {
			PPSetError(PPERR_SLIPFMT_NOTFOUND, pFormatName);
			LastFileName   = 0;
			LastFormatName = 0;
		}
	}
	CATCH
		{
			SString msg_buf, temp_buf;
			PPGetLastErrorMessage(0, msg_buf);
			(temp_buf = pFileName).CatChar('(').Cat(LineNo).CatChar(')').CatDiv(':', 0).Cat(msg_buf);
			PPSetError(PPERR_SLIPFMT_PARSE, temp_buf);
		}
		LastFileName   = 0;
		LastFormatName = 0;
		ok = 0;
	ENDCATCH
	delete p_zone;
	return ok;
}

int PPSlipFormat::Init(const char * pFileName, const char * pFormatName, SlipDocCommonParam * pParam)
{
	SlipDocCommonParam  sdc_param;
	MEMSZERO(sdc_param);
	int    ok = Parse(pFileName, pFormatName);
	if(ok > 0) {
		sdc_param.Title      = Title;
		sdc_param.DocNumber  = DocNumber;
		sdc_param.PageWidth  = PageWidth;
		sdc_param.PageLength = PageLength;
		sdc_param.HeadLines  = HeadLines;
		sdc_param.RegTo      = RegTo;
		sdc_param.TextOutput = TextOutput; // @vmiller
	}
	ASSIGN_PTR(pParam, sdc_param);
	return ok;
}

void PPSlipFormat::Helper_InitIteration()
{
	CurZone = 0;
	RunningTotal = 0.0;
	CurIter.Reset();
}

int PPSlipFormat::InitIteration(const CCheckPacket * pPack)
{
	Helper_InitIteration();
	return SetSource(pPack);
}

int PPSlipFormat::InitIteration(const PPBillPacket * pPack)
{
	Helper_InitIteration();
	return SetSource(pPack);
}

int PPSlipFormat::InitIteration(const CSessInfo * pInfo)
{
	Helper_InitIteration();
	return SetSource(pInfo);
}

int PPSlipFormat::NextIteration(SString & rBuf, SlipLineParam * pParam)
{
	int    ok = -1;
	SlipLineParam sl_param;
	while(ok < 0) {
		ok = NextIteration(&CurIter, rBuf);
		if(ok > 0) {
			const long flags = CurIter.P_Entry->Flags;
			sl_param.Font = CurIter.P_Entry->FontId;
			if(flags & PPSlipFormatEntry::fItemBarcode)
				sl_param.Kind = sl_param.lkBarcode;
			else if(flags & PPSlipFormatEntry::fSignBarcode)
				sl_param.Kind = sl_param.lkSignBarcode;
			sl_param.Text = CurIter.Text; // @v9.5.7
			sl_param.Code = CurIter.Code; // @v9.5.7
			{
				long   font_id = sl_param.Font;
				uint   font_pos = 0;
				if(FontList.lsearch(&font_id, &font_pos, CMPF_LONG)) {
					const FontBlock & r_fb = FontList.at(font_pos);
					sl_param.FontName = r_fb.Face;
					sl_param.FontSize = r_fb.Size;
				}
			}
			{
				long   pict_id = CurIter.P_Entry->PictId;
				uint   pict_pos = 0;
				if(PictList.lsearch(&pict_id, &pict_pos, CMPF_LONG)) {
					const PictBlock & r_pb = PictList.at(pict_pos);
					sl_param.PictPath = r_pb.Path;
					sl_param.PictCoord = r_pb.Coord;
				}
			}
			{
				long   bc_id = CurIter.P_Entry->BarcodeId;
				uint   bc_pos = 0;
				if(BcList.lsearch(&bc_id, &bc_pos, CMPF_LONG)) {
					const BarcodeBlock & r_bcb = BcList.at(bc_pos);
                    sl_param.BarcodeStd = r_bcb.BcStd;
                    sl_param.BarcodeWd = r_bcb.Width;
                    sl_param.BarcodeHt = r_bcb.Height;
                    SETFLAG(sl_param.Flags, SlipLineParam::fBcTextAbove, r_bcb.fTextAbove);
                    SETFLAG(sl_param.Flags, SlipLineParam::fBcTextBelow, r_bcb.fTextBelow);
				}
			}
			SETFLAG(sl_param.Flags, SlipLineParam::fRegRegular, flags & (PPSlipFormatEntry::fRegRibbon|PPSlipFormatEntry::fFiscal));
			SETFLAG(sl_param.Flags, SlipLineParam::fRegJournal, flags & (PPSlipFormatEntry::fJRibbon|PPSlipFormatEntry::fFiscal));
			SETFLAG(sl_param.Flags, SlipLineParam::fRegFiscal,  flags & PPSlipFormatEntry::fFiscal);
			sl_param.Qtty  = CurIter.Qtty;
			sl_param.Price = CurIter.Price;
			sl_param.VatRate = CurIter.VatRate; // @v9.7.1
			sl_param.DivID = CurIter.DivID;
		}
		else if(ok < 0) {
			if(CurZone == 0)
				InitIteration(CurZone = PPSlipFormatZone::kHeader, &CurIter);
			else if(CurZone == PPSlipFormatZone::kHeader)
				InitIteration(CurZone = PPSlipFormatZone::kDetail, &CurIter);
			else if(CurZone == PPSlipFormatZone::kDetail) {
				if(Src == srcCCheck) {
					InitIteration(CurZone = PPSlipFormatZone::kPaymDetail, &CurIter);
				}
				else {
					InitIteration(CurZone = PPSlipFormatZone::kFooter, &CurIter);
				}
			}
			else if(CurZone == PPSlipFormatZone::kPaymDetail) {
				InitIteration(CurZone = PPSlipFormatZone::kFooter, &CurIter);
			}
			else {
				CurZone = 0;
				break;
			}
		}
	}
	ASSIGN_PTR(pParam, sl_param);
	return ok;
}

#if SLTEST_RUNNING // {

SLTEST_R(PPSlipFormatLexer)
{
	int    ok = 1;
	int    token;
	SString result, out_buf;
	SString in_file_name = MakeInputFilePath("slip.fmt");
	PPSlipFormat fmt;
	SFile f_out(MakeOutputFilePath("slip.lex"), SFile::mWrite);
	THROW(SLTEST_CHECK_NZ(f_out.IsValid()));
	{
		SFile f_in(in_file_name, SFile::mRead);
		THROW(SLTEST_CHECK_NZ(f_in.IsValid()));
		do {
			THROW(SLTEST_CHECK_NZ(token = fmt.NextToken(f_in, result)));
			f_out.WriteLine((out_buf = 0).Cat(token).Space().Cat(result).CR());
		} while(token != PPSlipFormat::tokEOF);
	}
	THROW(SLTEST_CHECK_NZ(fmt.Parse(in_file_name, "Form10")));
	//
	// Print PPSlipFormat
	//
	(result = 0).CR();
#define _CATFLD(f) CatEq(#f, f).CR()
	result._CATFLD(fmt.Name);
	result._CATFLD(fmt.Title);
	result._CATFLD(fmt.DocNumber);
	result._CATFLD((long)fmt.PageWidth);
	result._CATFLD((long)fmt.PageLength);
	//result._CATFLD((long)fmt.Paper);
	result.Cat("ZoneList").Space().CatChar('{').CR();
	for(uint i = 0; i < fmt.ZoneList.getCount(); i++) {
		PPSlipFormatZone * p_zone = fmt.ZoneList.at(i);
		result._CATFLD((long)p_zone->Kind);
		result.Cat("EntryList").Space().CatChar('{').CR();
		for(uint j = 0; j < p_zone->getCount(); j++) {
			PPSlipFormatEntry * p_entry = p_zone->at(j);
			result._CATFLD((long)p_entry->Flags);
			result._CATFLD((long)p_entry->Condition);
			result._CATFLD((long)p_entry->FontId);
			result._CATFLD(p_entry->Text);
			if(p_entry->P_TrueZone) {
				result.Cat("TrueZone").CR();
			}
			if(p_entry->P_FalseZone) {
				result.Cat("FalseZone").CR();
			}
		}
		result.CatChar('}').CR();
	}
	result.CatChar('}').CR();
	f_out.WriteLine(result);
	//
	// GetFormList
	//
	{
		StrAssocArray list;
		SLTEST_CHECK_NZ(fmt.GetFormList(in_file_name, &list, 0));
		(result = 0).CR();
		for(uint i = 0; i < list.getCount(); i++) {
			result.Cat(list.at(i).Txt).CR();
		}
		f_out.WriteLine(result);
	}
	//
	//
	//
	CATCH
		{
			SString msg_buf;
			PPGetLastErrorMessage(1, msg_buf);
			SetInfo(msg_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
		}
		ok = 0;
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(PPSlipFormatOutput)
{
	int    ok = 1;
	long   c_max = 20;
	long   c = 0;  // Количество выведенных чеков
	long   cz = 0; // Количество выведенных чеков с нулевым количестом строк
	PPSlipFormat fmt;
	CCheckFilt cc_filt;
	CCheckViewItem cc_item;
	PPViewCCheck cc_view;
	SFile f_out;

	SString line_buf;
	THROW(SLTEST_CHECK_NZ(f_out.Open(MakeOutputFilePath("slip-output.lex"), SFile::mWrite)));
	THROW(SLTEST_CHECK_NZ(fmt.Parse(MakeInputFilePath("slip.fmt"), "Form1")));
	cc_filt.Flags |= CCheckFilt::fDontCount;
	THROW(SLTEST_CHECK_NZ(cc_view.Init_(&cc_filt)));
	for(cc_view.InitIteration(CCheckFilt::ordByDef); c < c_max && cc_view.NextIteration(&cc_item) > 0;) {
		CCheckPacket pack;
		cc_view.GetPacket(cc_item.ID, &pack);
		if(pack.GetCount() == 0)
			if(cz < 2)
				cz++;
			else
				continue;
		fmt.SetSource(&pack);
		PPSlipFormat::Iter iter;
		if(fmt.InitIteration(PPSlipFormatZone::kHeader, &iter))
			while(fmt.NextIteration(&iter, line_buf) > 0)
				f_out.WriteLine(line_buf.CR());
		if(fmt.InitIteration(PPSlipFormatZone::kDetail, &iter))
			while(fmt.NextIteration(&iter, line_buf) > 0)
				f_out.WriteLine(line_buf.CR());
		if(fmt.InitIteration(PPSlipFormatZone::kFooter, &iter))
			while(fmt.NextIteration(&iter, line_buf) > 0)
				f_out.WriteLine(line_buf.CR());
		line_buf.Z().CatCharN('*', 32).CR();
		f_out.WriteLine(line_buf);
		c++;
	}
	CATCH
		ok = 0;
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
//
//   PPSlipFormatter
//
PPSlipFormatter::PPSlipFormatter(const char * pFmtFileName)
{
	SString file_name;
	file_name = pFmtFileName;
	if(file_name.NotEmptyS() && fileExists(file_name))
		SlipFmtPath = file_name;
	else
		PPGetFilePath(PPPATH_BIN, PPFILNAM_STDSLIP_FMT, SlipFmtPath);
	P_SlipFormat = fileExists(SlipFmtPath) ? (new PPSlipFormat) : 0;
}

PPSlipFormatter::~PPSlipFormatter()
{
	delete ((PPSlipFormat *)P_SlipFormat);
}

int PPSlipFormatter::Init(const char * pFormatName, SlipDocCommonParam * pParam)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->Init(SlipFmtPath, pFormatName, pParam) : -1;
}

int PPSlipFormatter::InitIteration(const CCheckPacket * pPack)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->InitIteration(pPack) : -1;
}

int PPSlipFormatter::InitIteration(const PPBillPacket * pPack)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->InitIteration(pPack) : -1;
}

int PPSlipFormatter::InitIteration(const CSessInfo * pInfo)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->InitIteration(pInfo) : -1;
}

int PPSlipFormatter::NextIteration(SString & rBuf, SlipLineParam * pParam)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->NextIteration(rBuf, pParam) : -1;
}

int PPSlipFormatter::GetFormList(StrAssocArray * pList, int getSlipDocForms /* = 0*/)
{
	return P_SlipFormat ? ((PPSlipFormat *)P_SlipFormat)->GetFormList(SlipFmtPath, pList, getSlipDocForms) : -1;
}
