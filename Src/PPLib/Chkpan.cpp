// CHKPAN.CPP
// Copyright (c) A.Sobolev 1998-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Панель ввода кассовых чеков
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

int RunInputProcessThread(PPID posNodeID); // @prototype(PPPosProtocol.cpp)
//
// Команда для вызова звонка на принтере с кухонным звонком
//
// ESC 'p' m n1 n2
//
// Параметры:
//    m = 0 или 1 - не знаю что значит
//    n1 [0..255] - не знаю, что значит (возможно, длительность)
//    n2 [0..255] - не знаю, что значит (возможно, длительность)
//
// Example: 1B 70 0 5 5
// "1B70000505"
//
#define DEFAULT_TS_FONTSIZE        24
#define INSTVSRCH_THRESHOLD        50000
#define TSGGROUPSASITEMS_FONTDELTA 4
#define UNDEF_CHARGEGOODSID        -1000000000L

//#define GRP_IBG   1
//#define GRP_SCARD 2
//PPCustDisp * GetCustDisp(PPID cashNodeID); // Prototype (CUSTDISP.CPP)
//PPBnkTerminal * GetBnkTerm(PPID bnkTermID, const char * pPort, const char * pPath); // Prototype (BNKTERM.CPP)
int showInputLineCalc(TDialog *, uint);    // Prototype (VTBUTTON.CPP)
//
// Состояния панели ввода кассовых чеков и допустимые действия //
//
// - (1) sEMPTYLIST_EMPTYBUF Пустой список строк, пустой буфер ввода
//   ! 1 Выбрать товар по коду                  Код + Enter
//   ! 2 Выбрать товар по наименованию          F2
//   ! 3 Выбрать товар по цене                  Цена + F4
//   ! 4 Включить режим возврата товара         Ctrl-F5
//    11 Дисконтая карта                        Код карты + F3
//   ! 5 Закрыть панель                         Escape
//    13 Выбрать отложенный чек                 F8
//
// - (2) sEMPTYLIST_BUF      Пустой список строк, в буфере ввода есть товар
//   ! 1 Выбрать товар по коду                  Код + Enter
//   ! 2 Выбрать товар по наименованию          F2
//   ! 3 Выбрать товар по цене                  Цена + F4
//   ! 6 Внести количество                      Количество + F6 or "* количество" or "количество *"
//    11 Дисконтая карта                        Код карты + F3
//   ! 7 Внести товар из буфера ввода в список  Enter
//   ! 8 Очистить буфер ввода                   Escape
//
// - (3) sLIST_EMPTYBUF      Непустой список строк, пустой буфер ввода
//   ! 1 Выбрать товар по коду                  Код + Enter
//   ! 2 Выбрать товар по наименованию          F2
//   ! 3 Выбрать товар по цене                  Цена + F4
//    11 Дисконтая карта                        Код карты + F3
//   ! 9 Очистить список строк                  Escape
//   !10 Провести чек                           Enter
//    12 Отложить чек                           F8
//
// - (4) sLIST_BUF           Непустой список строк, в буфере ввода есть товар
//   ! 1 Выбрать товар по коду                  Код + Enter
//   ! 2 Выбрать товар по наименованию          F2
//   ! 3 Выбрать товар по цене                  Цена + F4
//   ! 6 Внести количество                      Количество + F6 or "* количество" or "количество *"
//    11 Дисконтая карта                        Код карты + F3
//   ! 7 Внести товар из буфера ввода в список  Enter
//   ! 8 Очистить буфер ввода                   Escape
//
// - (5) sLISTSEL_EMPTYBUF   Режим выбора строк из чека продажи, пустой буфер ввода
//   ! 2 Выбрать товар из чека продажи          F2
//   ! 9 Очистить список строк                  Escape
//   !10 Провести чек                           Enter
//
// - (6) sLISTSEL_BUF        Режим выбора строк из чека продажи, в буфере ввода есть товар
//   ! 2 Выбрать товар из чека продажи          F2
//   ! 6 Внести количество                      Количество + F6 or "* количество" or "количество *"
//   ! 7 Внести товар из буфера ввода в список  Enter
//   ! 8 Очистить буфер ввода                   Escape
//
struct SaComplexEntry {
	SaComplexEntry(PPID goodsID, double qtty) : GoodsID(goodsID), FinalGoodsID(0), Qtty(qtty), OrgPrice(0.0), FinalPrice(0.0), Flags(0)
	{
	}
	bool   IsComplete() const { return (GoodsID && (!(Flags & fGeneric) || FinalGoodsID)); }
	int    Subst(uint genListIdx);
	enum {
		fGeneric = 0x0001 // Товар GoodsID является обобщенным
	};
	PPID   GoodsID;
	PPID   FinalGoodsID;
	long   Flags;
	double Qtty;
	double OrgPrice;
	double FinalPrice;
	RAssocArray GenericList; // Key - goodsID, Val - price
};

class SaComplex : public TSArray <SaComplexEntry> {
public:
	SaComplex() : TSArray <SaComplexEntry> (), GoodsID(0), StrucID(0), Qtty(0.0), Price(0.0)
	{
	}
	void   Init(PPID goodsID, PPID strucID, double qtty);
	void   SetQuantity(double qtty);
	bool   RecalcFinalPrice();
	bool   Subst(uint itemIdx, uint entryItemIdx) { return (itemIdx < getCount() && at(itemIdx).Subst(entryItemIdx) > 0) ? RecalcFinalPrice() : false; }
	bool   IsComplete() const;

	PPID   GoodsID;
	PPID   StrucID;
	double Qtty;
	double Price;
private:
	virtual void FASTCALL freeItem(void *);
};

int SaComplexEntry::Subst(uint genListIdx)
{
	int    ok = 1;
	if(genListIdx < GenericList.getCount()) {
		FinalGoodsID = GenericList.at(genListIdx).Key;
		OrgPrice = GenericList.at(genListIdx).Val;
	}
	else
		ok = 0;
	return ok;
}

void SaComplex::Init(PPID goodsID, PPID strucID, double qtty)
{
	GoodsID = goodsID;
	StrucID = strucID;
	Qtty = qtty;
	Price = 0.0;
	freeAll();
}

bool SaComplex::IsComplete() const
{
	SForEachVectorItem(*this, i) { if(!at(i).IsComplete()) return false; }
	return true;
}

void SaComplex::SetQuantity(double qtty)
{
	if(qtty > 0.0) {
		const uint c = getCount();
		if(c) {
			if(Qtty > 0.0) {
				const double rel = qtty / Qtty;
				for(uint i = 0; i < c; i++)
					at(i).Qtty *= rel;
			}
			else {
				for(uint i = 0; i < c; i++)
					at(i).Qtty = qtty;
			}
		}
		Qtty = qtty;
	}
}

bool SaComplex::RecalcFinalPrice()
{
	bool   ok = true;
	RAssocArray template_list, list;
	for(uint i = 0; i < getCount(); i++) {
		const SaComplexEntry & r_entry = at(i);
		template_list.Add(r_entry.GoodsID, r_entry.OrgPrice * r_entry.Qtty);
	}
	if(template_list.Distribute(Price * Qtty, RAssocArray::dfRound|RAssocArray::dfReset, 0, list) > 0) {
		for(uint i = 0; i < getCount(); i++) {
			SaComplexEntry & r_entry = at(i);
			r_entry.FinalPrice = (r_entry.Qtty > 0.0) ? (list.Get(r_entry.GoodsID) / r_entry.Qtty) : r_entry.OrgPrice;
		}
	}
	else
		ok = false;
	return ok;
}

/*virtual*/void FASTCALL SaComplex::freeItem(void * pItem) { static_cast<SaComplexEntry *>(pItem)->GenericList.freeAll(); }
//
//
//
CPosProcessor::GrpListItem::GrpListItem() : ID(0), ParentID(0), Flags(0), Level(0)
{
}

CPosProcessor::GroupArray::GroupArray() : TSVector <CheckPaneDialog::GrpListItem> (), TopID(0)
{
}

CPosProcessor::GrpListItem * CPosProcessor::GroupArray::Get(PPID id, uint * pPos) const
{
	uint   pos = 0;
	if(lsearch(&id, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		return &at(pos);
	}
	else {
		ASSIGN_PTR(pPos, 0);
		return 0;
	}
}
//
//
//
CPosProcessor::Packet::Packet()
{
	Z();
}

CPosProcessor::Packet & FASTCALL CPosProcessor::Packet::operator = (const CCheckItemArray & rS)
{
	*static_cast<CCheckItemArray *>(this) = rS;
	return *this;
}

CPosProcessor::Packet & CPosProcessor::Packet::Z()
{
	TableCode = 0;
	GuestCount = 0;
	Reserve = 0;
	AgentID__ = 0;
	OrgAgentID = 0;
	OrderCheckID = 0;
	OrgUserID = 0;
	SVector::clear();
	ClearCur();
	IterIdx = 0;
	Eccd.Z();
	GiftAssoc.clear();
	// @v11.8.11 Prescr.Z();
	PPExtStrContainer::Z(); // @v11.8.11
	Lb_.Z(); // @v12.2.9
	return *this;
}

void CPosProcessor::Packet::ClearCur()
{
	CurCcItemPos = -1;
	Cur.Z();
	CurModifList.clear();
	Rest = 0.0;
}

int CPosProcessor::Packet::ClearGift()
{
	int    ok = -1;
	uint   c = getCount();
	if(c) do {
		--c;
		CCheckItem & r_item = at(c);
		if(r_item.Flags & cifGift) {
			atFree(c);
			ok = 1;
		}
		else {
			r_item.ResetGiftQuot();
			r_item.Flags &= ~(cifUsedByGift|cifMainGiftItem);
			ok = 1;
		}
	} while(c);
	GiftAssoc.clear();
	return 1;
}

PPID FASTCALL CPosProcessor::Packet::GetAgentID(int actual) const { return actual ? AgentID__ : NZOR(OrgAgentID, AgentID__); }
bool CPosProcessor::Packet::HasCur() const { return (CurCcItemPos >= 0); }
bool CPosProcessor::Packet::IsCurValid() const { return (CurCcItemPos >= 0 && CurCcItemPos < getCountI()); }
void CPosProcessor::Packet::SetRest(double rest) { Rest = rest; }

double CPosProcessor::Packet::GetGoodsQtty(PPID goodsID) const
{
	double qtty = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const CCheckItem & r_item = at(i);
		if(r_item.GoodsID == goodsID)
			qtty += r_item.Quantity;
	}
	return qtty;
}

static void FASTCALL CatCharByFlag(long val, long flag, int chr, SString & rBuf, int inverse)
{
	if((!inverse && val & flag) || (inverse && !(val & flag)))
		rBuf.CatChar(chr);
}

int CheckPaneDialog::LoadCheck(const CCheckPacket * pPack, bool makeRetCheck, bool dontShow)
{
	if(pPack) {
		Goods2Tbl::Rec goods_rec;
		SString temp_buf;
		CCheckItem cc_item;
		for(uint i = 0; pPack->EnumLines(&i, &cc_item);) {
			cc_item.Quantity  = makeRetCheck ? -fabs(cc_item.Quantity) : cc_item.Quantity;
			cc_item.Flags    |= cc_item.Quantity ? 0 : cifGift;
			if(GObj.Fetch(cc_item.GoodsID, &goods_rec) > 0) {
				STRNSCPY(cc_item.GoodsName, goods_rec.Name);
				GObj.FetchSingleBarcode(cc_item.GoodsID, temp_buf.Z());
				temp_buf.CopyTo(cc_item.BarCode, sizeof(cc_item.BarCode));
			}
			if(!P.insert(&cc_item)) {
				MessageError(PPERR_SLIB, 0, eomMsgWindow);
				break;
			}
		}
		SetupExt(pPack); // @v11.8.8
		if(!dontShow) {
			if(P_ChkPack) {
				setStaticText(CTL_CHKPAN_CHKID,   temp_buf.Z().Cat(P_ChkPack->Rec.ID));
				setStaticText(CTL_CHKPAN_CHKDTTM, temp_buf.Z().Cat(P_ChkPack->Rec.Dt).Space().Cat(P_ChkPack->Rec.Tm));
				setStaticText(CTL_CHKPAN_CHKNUM,  temp_buf.Z().Cat(P_ChkPack->Rec.Code));
				setStaticText(CTL_CHKPAN_CASHNUM, temp_buf.Z().Cat(P_ChkPack->Rec.PosNodeID));
				setStaticText(CTL_CHKPAN_INITDTM, temp_buf.Z().Cat(P_ChkPack->Ext.CreationDtm, DATF_DMY|DATF_NOZERO, TIMF_HMS|TIMF_NOZERO));
				if(P_ChkPack->Ext.CreationUserID)
					GetObjectName(PPOBJ_USR, P_ChkPack->Ext.CreationUserID, temp_buf);
				else
					temp_buf.Z();
				setStaticText(CTL_CHKPAN_INITUSER, temp_buf);
				temp_buf.Z();
				const long f = P_ChkPack->Rec.Flags;
				CatCharByFlag(f, CCHKF_NOTUSED,   'G', temp_buf, 1);
				CatCharByFlag(f, CCHKF_PRINTED,   'P', temp_buf, 0);
				CatCharByFlag(f, CCHKF_RETURN,    'R', temp_buf, 0);
				CatCharByFlag(f, CCHKF_ZCHECK,    'Z', temp_buf, 0);
				CatCharByFlag(f, CCHKF_TRANSMIT,  'T', temp_buf, 0);
				CatCharByFlag(f, CCHKF_BANKING,   'B', temp_buf, 0);
				CatCharByFlag(f, CCHKF_INCORPCRD, 'I', temp_buf, 0);
				CatCharByFlag(f, CCHKF_SUSPENDED, 'S', temp_buf, 0);
				CatCharByFlag(f, CCHKF_JUNK,      'J', temp_buf, 0);
				setStaticText(CTL_CHKPAN_FLAGS, temp_buf);
				if(P_ChkPack->Rec.SCardID) {
					SCardTbl::Rec sc_rec;
					if(ScObj.Search(P_ChkPack->Rec.SCardID, &sc_rec) > 0) {
						temp_buf.Z().Cat(sc_rec.Code);
						setStaticText(CTL_CHKPAN_SCARDCODE, temp_buf);
					}
				}
			}
			OnUpdateList(0);
			ClearRow();
		}
	}
	return 1;
}

void FASTCALL CPosProcessor::Packet::SetupCCheckPacket(CCheckPacket * pPack, const CardState & rCSt, bool isExtCc) const
{
	if(pPack) {
		SString temp_buf;
		pPack->Rec.SCardID = rCSt.GetID();
		pPack->SetSCardSpecialTreatmentIdentifyReplyBlock(&rCSt.CSTRB);
		if(OrgUserID) {
			pPack->Rec.UserID = OrgUserID;
		}
		else {
			PPObjPerson::GetCurUserPerson(&pPack->Rec.UserID, 0);
		}
		pPack->Ext.SalerID = GetAgentID(0);
		pPack->Ext.TableNo = TableCode;
		pPack->Ext.GuestCount = GuestCount;
		// @v12.2.9 pPack->Ext.LinkCheckID = (pPack->Rec.Flags & CCHKF_SKIP) ? 0 : OrderCheckID;
		// @v12.2.9 {
		if(pPack->Rec.Flags & (CCHKF_RETURN|CCHKF_CORRECTION)) {
			pPack->Ext.LinkCheckID = Lb_._CcID;
		}
		else if(pPack->Rec.Flags & CCHKF_SKIP)
			pPack->Ext.LinkCheckID = 0;
		else 
			pPack->Ext.LinkCheckID = OrderCheckID;
		// } @v12.2.9 
		pPack->Ext.CreationDtm = Eccd.InitDtm;
		pPack->Ext.CreationUserID = Eccd.InitUserID;
		if(!isExtCc)
			CCheckPacket::CopyExtStrContainer(*pPack, *this, 0); // @v11.8.11
		// @v11.8.11 pPack->SetGuid(&Eccd.Uuid); // @v11.5.8
		Eccd.Memo.CopyTo(pPack->Ext.Memo, sizeof(pPack->Ext.Memo));
		SETFLAG(pPack->Rec.Flags, CCHKF_DELIVERY,   Eccd.Flags & Eccd.fDelivery);
		SETFLAG(pPack->Rec.Flags, CCHKF_FIXEDPRICE, Eccd.Flags & Eccd.fFixedPrice);
		SETFLAG(pPack->Rec.Flags, CCHKF_SPFINISHED, Eccd.Flags & Eccd.fSpFinished);
		// @v11.3.6 {
		// @v11.9.8 (похоже, я погорячился, закомментировав этот участок при перестройке работы блока строк расширения чека) /* @v11.8.11 
		if(!EAddr.IsEmpty()) {
			if(EAddr.AddrType == SNTOK_EMAIL) {
				pPack->PutExtStrData(CCheckPacket::extssBuyerEMail, EAddr.EAddr);
				pPack->PutExtStrData(CCheckPacket::extssBuyerPhone, 0);
			}
			else if(EAddr.AddrType == SNTOK_PHONE) {
				pPack->PutExtStrData(CCheckPacket::extssBuyerEMail, 0);
				pPack->PutExtStrData(CCheckPacket::extssBuyerPhone, EAddr.EAddr);
			}
			SETFLAG(pPack->Rec.Flags, CCHKF_PAPERLESS, Paperless);
		}
		else {
			pPack->PutExtStrData(CCheckPacket::extssBuyerEMail, 0);
			pPack->PutExtStrData(CCheckPacket::extssBuyerPhone, 0);
			SETFLAG(pPack->Rec.Flags, CCHKF_PAPERLESS, 0);
		}
		// @v11.9.8 */
		// } @v11.3.6 
		// @v11.8.11 pPack->SetPrescription(Prescr); // @v11.7.12 
		// @v11.8.8 {
		/* @v11.8.11 {
			if(!!Eccd.LinkBillUuid) {
				Eccd.LinkBillUuid.ToStr(S_GUID::fmtIDL, temp_buf);
				pPack->PutExtStrData(CCheckPacket::extssLinkBillUuid, temp_buf);
			}
		}*/
		// } @v11.8.8
		if(Eccd.Flags & Eccd.fDelivery) {
			pPack->SetDlvrAddr(&Eccd.Addr_);
			pPack->Ext.StartOrdDtm = Eccd.DlvrDtm;
		}
	}
}

void CPosProcessor::Packet::SetupInfo(SString & rBuf)
{
	SString temp_buf;
	rBuf.Z();
	if(GetAgentID(1)) {
		PPLoadStringS("seller", rBuf).CatDiv(':', 2);
		GetArticleName(GetAgentID(1), temp_buf);
		if(rBuf.NotEmpty())
			rBuf.CatCharN(' ', 4);
		rBuf.Cat(temp_buf);
	}
	if(TableCode) {
		if(rBuf.NotEmpty())
			rBuf.CatCharN(' ', 4);
		rBuf.Cat(PPLoadStringS("ftable", temp_buf)).CatDiv(':', 2).Cat(TableCode);
		if(GuestCount) {
			rBuf.CatCharN(' ', 4).Cat(PPLoadStringS("guestcount", temp_buf)).CatDiv(':', 2).Cat(GuestCount);
		}
	}
	if(GetCur().Division) {
		if(rBuf.NotEmpty())
			rBuf.CatCharN(' ', 4);
		rBuf.Cat(PPLoadStringS("department", temp_buf)).CatDiv(':', 2).Cat(GetCur().Division);
	}
}

int CPosProcessor::Packet::MoveUp(uint itemIdx)
{
	if(itemIdx < getCount() && itemIdx > 0) {
		swap(itemIdx, itemIdx-1);
		return 1;
	}
	else
		return -1;
}

int CPosProcessor::Packet::MoveDown(uint itemIdx)
{
	if((itemIdx+1) < getCount()) {
		swap(itemIdx, itemIdx+1);
		return 1;
	}
	else
		return -1;
}

int CPosProcessor::Packet::Grouping(uint itemIdx)
{
	if(itemIdx > 0 && itemIdx < getCount()) {
		if(at(itemIdx).Flags & cifGrouped) {
			at(itemIdx).Flags &= ~cifGrouped;
		}
		else {
			at(itemIdx).Flags |= cifGrouped;
			//
			// Для данного элемента устанавливается такой же номер очереди, как и у
			// предыдущего элемента (предшествующий элемент имеет приоритет по причине здравого
			// смысла - группируют обычно менее значительный элемент с более значительным).
			//
			int8   prev_queue = at(itemIdx-1).Queue;
			if(at(itemIdx).Queue != prev_queue)
				SetQueue(itemIdx, prev_queue);
		}
		return 1;
	}
	else
		return -1;
}

int CPosProcessor::Packet::SetQueue(uint itemIdx, int8 queue)
{
	int    ok = 1;
	const  uint c = getCount();
	uint   i = itemIdx;
	if(i < c) {
		at(i++).Queue = queue;
		//
		// Для всех элементов, сгруппированных с данным, необходимо выставить тот же самый номер очереди.
		//
		while(i < c && at(i).Flags & (cifGrouped|cifModifier)) {
			at(i++).Queue = queue;
		}
		for(i = itemIdx; at(i).Flags & cifGrouped && i > 0;)
			at(--i).Queue = queue;
	}
	else
		ok = 0;
	return ok;
}

int CPosProcessor::Packet::InitIteration()
{
	IterIdx = 0;
	//
	// Расставляем нумерацию групп строк по признаку cifGrouped
	//
	int    grp_n = 0;
	bool   is_grp = false;
	for(uint i = 0; i < getCount(); i++) {
		CCheckItem & r_item = at(i);
		if(r_item.Flags & cifGrouped) {
			if(!is_grp) {
				grp_n++;
				is_grp = true;
				if(i > 0)
					at(i-1).LineGrpN = grp_n;
			}
			r_item.LineGrpN = grp_n;
		}
		else
			is_grp = false;
	}
	return 1;
}

int FASTCALL CPosProcessor::Packet::NextIteration(CCheckItem * pItem)
{
	int    ok = -1;
	CCheckItem * p_item;
	if(pItem && enumItems(&IterIdx, (void **)&p_item)) {
		ASSIGN_PTR(pItem, *p_item);
		ok = 1;
	}
	return ok;
}
//
//
//
CPosProcessor::PgsBlock::PgsBlock(double qtty) : Flags(0), Qtty((qtty != 0.0) ? qtty : 1.0), PriceBySerial(0.0), AbstractPrice(0.0), ChZnPm_ReqTimestamp(0)
{
	AllowedPriceRange.Z(); // @v12.2.2
}
//
//
//
CPosProcessor::AcceptCheckProcessBlock::AcceptCheckProcessBlock() : R(1), SyncPrnErr(0), RExt(1), ExtSyncPrnErr(0), Flags(0)
{
}
//
//
//
CPosProcessor::ExtCcData::ExtCcData() : Flags(0), SCardID_(0), InitUserID(0), DlvrDtm(ZERODATETIME), InitDtm(ZERODATETIME)
{
}

CPosProcessor::ExtCcData & CPosProcessor::ExtCcData::Z()
{
	Flags = 0;
	SCardID_ = 0;
	InitUserID = 0;
	DlvrDtm.Z();
	InitDtm.Z();
	Addr_.Clear();
	Memo.Z();
	return *this;
}
//
//
//
CPosProcessor::LinkBlock::LinkBlock() : _Op(CCOP_GENERAL), _CcID(0), _CcAmount(0.0), _CcCredit(0.0)
{
}

CPosProcessor::LinkBlock & CPosProcessor::LinkBlock::Z()
{
	_Op = CCOP_GENERAL;
	_CcID = 0;
	_CcAmount = 0.0;
	_CcCredit = 0.0;
	AmL.Z();
	FiscalTag.Z();
	return *this;
}
//
//
//
CPosProcessor::ManualDiscount::ManualDiscount() : Flags(0), Discount(0.0), SettledAbsolutDiscount(0.0)
{
}

CPosProcessor::ManualDiscount & CPosProcessor::ManualDiscount::Z()
{
	Flags = 0;
	Discount = 0.0;
	SettledAbsolutDiscount = 0.0;
	return *this;
}

CPosProcessor::CardState::CardState() : CSTRB(), Flags(0), OwnerID(0), Discount(0.0), SettledDiscount(0.0), RestByCrdCard(0.0), UsableBonus(0.0),
	MaxCreditByCrdCard(0.0), AdditionalPayment(0.0), SCardID(0), P_DisByAmtRule(0), P_Eqb(0)
{
	Code[0] = 0;
}

CPosProcessor::CardState::~CardState()
{
	delete P_DisByAmtRule;
	delete P_Eqb;
}

CPosProcessor::CardState & CPosProcessor::CardState::Z()
{
	ZDELETE(P_DisByAmtRule);
	ZDELETE(P_Eqb);
	CSTRB.Z();
	Flags = 0;
	OwnerID = 0;
	Discount = 0.0;
	SettledDiscount = 0.0;
	RestByCrdCard = 0.0;
	UsableBonus = 0.0;
	MaxCreditByCrdCard = 0.0;
	AdditionalPayment = 0.0;
	SCardID = 0;
	Code[0] = 0;
	return *this;
}

void CPosProcessor::CardState::SetID(PPID id, const char * pCode)
{
	SCardID = id;
	STRNSCPY(Code, pCode);
}

double CPosProcessor::CardState::GetDiscount(double ccAmount) const
{
	double ret = 0.0;
	if(P_DisByAmtRule && ccAmount > 0.0) {
		const TrnovrRngDis * p_item = P_DisByAmtRule->SearchItem(ccAmount);
		ret = (p_item && !(p_item->Flags & TrnovrRngDis::fBonusAbsoluteValue)) ? p_item->Value : Discount;
	}
	else
		ret = Discount;
	return ret;
}

const RetailPriceExtractor::ExtQuotBlock * CPosProcessor::GetCStEqb(PPID goodsID, bool * pNoDiscount)
{
  	const  bool cfg_dsbl_no_dis = LOGIC(CsObj.GetEqCfg().Flags & PPEquipConfig::fIgnoreNoDisGoodsTag);
	const  bool nodis = (!cfg_dsbl_no_dis && GObj.CheckFlag(goodsID, GF_NODISCOUNT));
	ASSIGN_PTR(pNoDiscount, nodis);
	return nodis ? 0 : CSt.P_Eqb;
}

const RetailPriceExtractor::ExtQuotBlock * CPosProcessor::GetCStEqbND(bool nodiscount) const { return nodiscount ? 0 : CSt.P_Eqb; }
void  CPosProcessor::MsgToDisp_Clear() { MsgToDisp.Z(); }
CCheckCore & CPosProcessor::GetCc() { return *ScObj.P_CcTbl; }
PPObjSCard & CPosProcessor::GetScObj() { return ScObj; }
bool   CPosProcessor::InitCcView() { return SETIFZ(P_CcView, new PPViewCCheck(GetCc())); }
bool   FASTCALL CPosProcessor::IsState(int s) const { return (State_p == s); }
bool   FASTCALL CPosProcessor::F(long f) const { return LOGIC(Flags & f); }
void   CPosProcessor::SetupSessUuid(const S_GUID_Base & rUuid) { SessUUID = rUuid; }
PPID   CPosProcessor::GetPosNodeID() const { return PNP.NodeID; }
long   CPosProcessor::GetTableCode() const { return P.TableCode; }
int    CPosProcessor::GetGuestCount() const { return P.GuestCount; }
PPID   CPosProcessor::GetCnLocID(PPID goodsID) const { return (ExtCashNodeID && PNP.ExtCnLocID && BelongToExtCashNode(goodsID)) ? PNP.ExtCnLocID : PNP.CnLocID; }
int    CPosProcessor::InitIteration() { return P.InitIteration(); }
int    FASTCALL CPosProcessor::NextIteration(CCheckItem * pItem) { return P.NextIteration(pItem); }
/*virtual*/int  CPosProcessor::MsgToDisp_Show() { return -1; }
/*virtual*/int  CPosProcessor::ConfirmMessage(int msgId, const char * pAddedMsg, int defaultResponse) { return defaultResponse; }
/*virtual*/int  CPosProcessor::CDispCommand(int cmd, int iVal, double rv1, double rv2) { return -1; }
/*virtual*/int  CPosProcessor::Implement_AcceptCheckOnEquipment(const CcAmountList * pPl, AcceptCheckProcessBlock & rB) { return 1; }
/*virtual*/void CPosProcessor::NotifyGift(PPID giftID, const SaGiftArray::Gift * pGift) {}
/*virtual*/void CPosProcessor::SetPrintedFlag(int set) { SETFLAG(Flags, fPrinted, set); }
/*virtual*/void CPosProcessor::SetupInfo(const char * pErrMsg) {}
/*virtual*/void CPosProcessor::OnUpdateList(int goBottom) {}

int CPosProcessor::MsgToDisp_Add(const char * pMsg)
{
	int    ok = -1;
	if(!isempty(pMsg)) {
		bool   dup = false;
		SString temp_buf;
		SString msg_buf(pMsg);
		msg_buf.Chomp().Strip();
		for(uint sp = 0; !dup && MsgToDisp.get(&sp, temp_buf);) {
			if(temp_buf.CmpNC(msg_buf) == 0)
				dup = true;
		}
		if(!dup)
			ok = MsgToDisp.add(pMsg) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

class CPosProcessor_MsgToDisp_Frame {
public:
	CPosProcessor_MsgToDisp_Frame(CPosProcessor * pCls) : P_Cls(pCls)
	{
		CALLPTRMEMB(P_Cls, MsgToDisp_Clear());
	}
	~CPosProcessor_MsgToDisp_Frame()
	{
		CALLPTRMEMB(P_Cls, MsgToDisp_Show());
	}
private:
	CPosProcessor * P_Cls;
};

int CPosProcessor::LoadModifiers(PPID goodsID, SaModif & rModif)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	Goods2Tbl::Rec item_goods_rec;
	rModif.clear();
	if(GObj.Fetch(goodsID, &goods_rec) > 0) {
		int    r = 0;
		PPGoodsStruc gs;
		PPID   gen_goods_id = 0;
		THROW(r = GObj.LoadGoodsStruc(PPGoodsStruc::Ident(goodsID, GSF_PARTITIAL|GSF_POSMODIFIER, 0, getcurdate_()), &gs));
		if(r < 0 && GObj.BelongToGen(goodsID, &gen_goods_id, 0) > 0) {
			THROW(r = GObj.LoadGoodsStruc(PPGoodsStruc::Ident(gen_goods_id, GSF_PARTITIAL|GSF_POSMODIFIER, 0, getcurdate_()), &gs));
		}
		if(r > 0) {
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			double item_qtty = 0.0;
			PPIDArray gen_list;
			PPGoodsStrucItem gs_item;
			for(uint p = 0; gs.EnumItemsExt(&p, &gs_item, 0, 1.0, &item_qtty) > 0;) {
				if(GObj.Fetch(gs_item.GoodsID, &item_goods_rec) > 0 && !(item_goods_rec.Flags & GF_GENERIC)) {
					SaModifEntry entry;
					MEMSZERO(entry);
					entry.GoodsID = gs_item.GoodsID;
					entry.Qtty = fabs(item_qtty);
					{
						RetailGoodsInfo rgi;
						GetRgi(entry.GoodsID, 0.0, 0, rgi);
						entry.Price = rgi.Price;
					}
					THROW_SL(rModif.insert(&entry));
				}
			}
			if(rModif.getCount())
				ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

bool CPosProcessor::Backend_SetZeroAgentRestriction(bool set) // @v11.8.8
{
	const bool preserve_value = LOGIC(PNP.CnFlags & CASHF_DISABLEZEROAGENT);
	SETFLAG(PNP.CnFlags, CASHF_DISABLEZEROAGENT, set);
	SETFLAG(PNP.CnExtFlags, CASHF_DISABLEZEROAGENT, set);
	return preserve_value;
}

int CPosProcessor::Backend_GetGoodsList(PPIDArray & rList) // @v11.4.5
{
	rList.Z();
	int    ok = 1;
	GoodsFilt gf;
	if(!(PNP.CnFlags & CASHF_SELALLGOODS)) {
		gf.Flags |= GoodsFilt::fActualOnly;
		gf.LocList.Add(PNP.CnLocID);
	}
	if(PNP.CnExtFlags & CASHFX_USEGOODSMATRIX)
		gf.Flags |= GoodsFilt::fRestrictByMatrix;
	GoodsIterator::GetListByFilt(&gf, &rList, 0);
	return ok;
}

int CPosProcessor::Backend_GetCCheckList(const DateRange * pPeriod, long ctblId, TSVector <CCheckViewItem> & rList)
{
	rList.clear();
	int    ok = 1;
	CCheckViewItem item;
	CCheckFilt flt;
	THROW(InitCcView());
	flt.Flags = CCheckFilt::fShowSuspended|CCheckFilt::fSuspendedOnly;
	flt.Flags |= CCheckFilt::fLostJunkAsSusp;
	if(ctblId)
		flt.Flags |= CCheckFilt::fCTableStatus;
    if(!SessUUID.IsZero()) {
        flt.LostJunkUUID = SessUUID;
    }
	flt.TableCode = ctblId;
	flt.AgentID = P.GetAgentID();
	flt.NodeList.Add(GetPosNodeID());
	if(pPeriod)
		flt.Period = *pPeriod;
	else {
		flt.Period.upp = getcurdate_();
		flt.Period.low = plusdate(flt.Period.upp, -PNP.Scf.DaysPeriod);
	}
	P_CcView->Init_(&flt);
	for(P_CcView->InitIteration(0); P_CcView->NextIteration(&item) > 0;) {
		THROW_SL(rList.insert(&item));
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::ExportCTblList(SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	int    use_def_ctbl = 0;
	SString temp_buf;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	TSVector <CCheckViewItem> cc_list;
	LongArray ctbl_list;
	if(PNP.CTblList.getCount()) {
		SForEachVectorItem(PNP.CTblList, i) { ctbl_list.add(PNP.CTblList.get(i)); }
	}
	else {
		ctbl_list.add(PPObjCashNode::SubstCTblID);
		use_def_ctbl = 1;
	}
	THROW(Backend_GetCCheckList(0, 0, cc_list));
	//
	THROW(p_xml_buf = xmlBufferCreate());
	THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
	{
		SXml::WDoc _doc(p_writer, cpUTF8);
		xmlTextWriterStartDTD(p_writer, (temp_buf = "CPosProcessorCTableList").ucptr(), 0, 0);
		XMLWriteSpecSymbEntities(p_writer);
		xmlTextWriterEndDTD(p_writer);
		{
			SXml::WNode n_list(p_writer, "CPosProcessorCTableList");
			for(uint i = 0; i < ctbl_list.getCount(); i++) {
				const long ctbl_id = ctbl_list.get(i);
				SXml::WNode n_item(p_writer, "CPosProcessorCTable");
				n_item.PutInner("ID", temp_buf.Z().Cat(ctbl_id));
				PPObjCashNode::GetCafeTableName(ctbl_id, temp_buf.Z());
				n_item.PutInner("Name", temp_buf);
				n_item.PutInner("State", temp_buf.Z().Cat(0L));
				{
					int    cc_count = 0;
					int    cc_guest_count = 0;
					double cc_amount = 0.0;
					if(ctbl_id == PPObjCashNode::SubstCTblID) {
						for(uint p = 0; p < cc_list.getCount(); p++) {
							cc_count++;
							cc_guest_count += cc_list.at(p).GuestCount;
							cc_amount += MONEYTOLDBL(cc_list.at(p).Amount);
						}
					}
					else {
						for(uint p = 0; cc_list.lsearch(&ctbl_id, &p, CMPF_LONG, offsetof(CCheckViewItem, TableCode)); p++) {
							cc_count++;
							cc_guest_count += cc_list.at(p).GuestCount;
							cc_amount += MONEYTOLDBL(cc_list.at(p).Amount);
						}
					}
					n_item.PutInner("CCCount", temp_buf.Z().Cat(cc_count));
					n_item.PutInner("CCAmount", temp_buf.Z().Cat(cc_amount, MKSFMTD_020));
					n_item.PutInner("CCGuestCount", temp_buf.Z().Cat(cc_guest_count));
				}
			}
		}
		xmlTextWriterFlush(p_writer);
		rBuf.CopyFromN(PTRCHRC_(p_xml_buf->content), p_xml_buf->use)/*.UTF8ToChar()*/;
		rBuf.Transf(CTRANSF_INNER_TO_UTF8);
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
    return ok;
}

int CPosProcessor::ExportCCheckList(long ctblId, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	uint   i;
	SString temp_buf;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	TSVector <CCheckViewItem> cc_list;
	THROW(Backend_GetCCheckList(0, ctblId, cc_list));
	THROW(p_xml_buf = xmlBufferCreate());
	THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
	{
		SXml::WDoc _doc(p_writer, cpUTF8);
		xmlTextWriterStartDTD(p_writer, (temp_buf = "CPosProcessorCCheckList").ucptr(), 0, 0);
		XMLWriteSpecSymbEntities(p_writer);
		xmlTextWriterEndDTD(p_writer);
		{
			SXml::WNode n_list(p_writer, "CPosProcessorCCheckList");
			for(i = 0; i < cc_list.getCount(); i++) {
				const CCheckViewItem & r_item = cc_list.at(i);
				SXml::WNode n_item(p_writer, "CPosProcessorCCheck");
				n_item.PutInner("ID", temp_buf.Z().Cat(r_item.ID));
				n_item.PutInner("Code", temp_buf.Z().Cat(r_item.Code));
				n_item.PutInner("Flags", temp_buf.Z().Cat(r_item.Flags));
				n_item.PutInner("AgentID", temp_buf.Z().Cat(r_item.AgentID));
				temp_buf.Z();
				if(r_item.AgentID)
					GetArticleName(r_item.AgentID, temp_buf);
				n_item.PutInner("AgentName", temp_buf);
				n_item.PutInner("SCardID", temp_buf.Z().Cat(r_item.SCardID));
				{
					temp_buf.Z();
					SCardTbl::Rec sc_rec;
					if(r_item.SCardID && ScObj.Fetch(r_item.SCardID, &sc_rec) > 0)
						temp_buf = sc_rec.Code;
					n_item.PutInner("SCardCode", temp_buf);
				}
				n_item.PutInner("CTableID", temp_buf.Z().Cat(r_item.TableCode));
				PPObjCashNode::GetCafeTableName(r_item.TableCode, temp_buf.Z());
				n_item.PutInner("CTableName", temp_buf);
				n_item.PutInner("GuestCount", temp_buf.Z().Cat(r_item.GuestCount));
				n_item.PutInner("Amount", temp_buf.Z().Cat(MONEYTOLDBL(r_item.Amount), MKSFMTD_020));
				n_item.PutInner("Discount", temp_buf.Z().Cat(MONEYTOLDBL(r_item.Discount), MKSFMTD_020));
				n_item.PutInner("CreationTime", temp_buf.Z().Cat(r_item.CreationDtm, DATF_ISO8601CENT, 0));
				n_item.PutInner("CreationUserID", temp_buf.Z().Cat(r_item.CreationUserID));
			}
		}
		xmlTextWriterFlush(p_writer);
		rBuf.CopyFromN(reinterpret_cast<const char *>(p_xml_buf->content), p_xml_buf->use);
		rBuf.Transf(CTRANSF_INNER_TO_UTF8);
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
    return ok;
}

int CPosProcessor::ExportCurrentState(SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	rBuf.Z();
	THROW(p_xml_buf = xmlBufferCreate());
	THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
	{
		SXml::WDoc _doc(p_writer, cpUTF8);
		xmlTextWriterStartDTD(p_writer, (temp_buf = "CPosProcessorState").ucptr(), 0, 0);
		XMLWriteSpecSymbEntities(p_writer);
		xmlTextWriterEndDTD(p_writer);
		{
			SXml::WNode n_state(p_writer, "CPosProcessorState");
			{
				#define PUTNODE_F(f) SXml::WNode(p_writer, #f, temp_buf.Z().Cat(f))
				#define PUTNODE_TF(t, f) SXml::WNode(p_writer, #t, temp_buf.Z().Cat(f))
				PUTNODE_F(AuthAgentID);
				PUTNODE_F(CheckID);
				PUTNODE_F(SuspCheckID);
				PUTNODE_TF(State, State_p);
				PUTNODE_F(Flags);
				PUTNODE_F(OperRightsFlags);
				PUTNODE_F(PNP.CnFlags);
				PUTNODE_F(PNP.CnExtFlags);
				#undef PUTNODE_TF
				#undef PUTNODE_F
				{
					SXml::WNode n_cardstate(p_writer, "CardState");
					{
						SXml::WNode(p_writer, "ID", temp_buf.Z().Cat(CSt.GetID()));
						SXml::WNode(p_writer, "Code", temp_buf.Z().Cat(CSt.GetCode()));
						#define PUTNODE_F(f) SXml::WNode(p_writer, #f, temp_buf.Z().Cat(CSt.f))
						PUTNODE_F(Flags);
						PUTNODE_F(Discount);
						PUTNODE_F(SettledDiscount);
						PUTNODE_F(CSTRB.Rest/*OuterSvcRest*/);
						PUTNODE_F(RestByCrdCard);
						PUTNODE_F(UsableBonus);
						PUTNODE_F(MaxCreditByCrdCard);
						PUTNODE_F(CSTRB.OperationCode/*UhttCode*/);
						PUTNODE_F(CSTRB.Hash/*UhttHash*/);
						#undef PUTNODE_F
					}
				}
				{
					SXml::WNode n_packet(p_writer, "Packet");
					{
						#define PUTNODE_F(f) SXml::WNode(p_writer, #f, temp_buf.Z().Cat(P.f))
						SXml::WNode(p_writer, "CTableID", temp_buf.Z().Cat(P.TableCode));
						PUTNODE_F(GuestCount);
						PUTNODE_F(OrderCheckID);
						PUTNODE_F(CurCcItemPos);
						SXml::WNode(p_writer, "Rest", temp_buf.Z().Cat(P.GetRest()));
						SXml::WNode(p_writer, "AgentID", temp_buf.Z().Cat(P.GetAgentID()));
						#undef PUTNODE_F
						for(uint i = 0; i < P.getCount(); i++) {
							SXml::WNode n_row(p_writer, "CCRow");
							{
								const CCheckItem & r_item = P.at(i);
								#define PUTNODE_F(f) SXml::WNode(p_writer, #f, temp_buf.Z().Cat(r_item.f))
								PUTNODE_F(GoodsID);
								PUTNODE_F(Quantity);
								PUTNODE_F(PhQtty);
								PUTNODE_F(Price);
								PUTNODE_F(Discount);
								PUTNODE_F(BeforeGiftPrice);
								PUTNODE_F(GiftID);
								PUTNODE_F(Flags);
								PUTNODE_F(Division);
								PUTNODE_F(LineGrpN);
								PUTNODE_F(Queue);
								PUTNODE_F(BarCode);
								PUTNODE_F(GoodsName);
								PUTNODE_F(Serial);
								#undef PUTNODE_F
							}
						}
						if(P.HasCur()) {
							SXml::WNode n_row(p_writer, "CCRowCurrent");
							{
								const CCheckItem & r_item = P.GetCurC();
								#define PUTNODE_F(f) SXml::WNode(p_writer, #f, temp_buf.Z().Cat(r_item.f))
								PUTNODE_F(GoodsID);
								PUTNODE_F(Quantity);
								PUTNODE_F(PhQtty);
								PUTNODE_F(Price);
								PUTNODE_F(Discount);
								PUTNODE_F(BeforeGiftPrice);
								PUTNODE_F(GiftID);
								PUTNODE_F(Flags);
								PUTNODE_F(Division);
								PUTNODE_F(LineGrpN);
								PUTNODE_F(Queue);
								PUTNODE_F(BarCode);
								PUTNODE_F(GoodsName);
								PUTNODE_F(Serial);
								#undef PUTNODE_F
							}
						}
					}
				}
			}
		}
	}
	xmlTextWriterFlush(p_writer);
	rBuf.CopyFromN(reinterpret_cast<const char *>(p_xml_buf->content), p_xml_buf->use)/*.UTF8ToChar()*/;
	rBuf.Transf(CTRANSF_INNER_TO_UTF8);
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
	return ok;
}

int CPosProcessor::ExportModifList(PPID goodsID, SString & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	rBuf.Z();
	THROW(p_xml_buf = xmlBufferCreate());
	THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
	{
		SXml::WDoc _doc(p_writer, cpUTF8);
		xmlTextWriterStartDTD(p_writer, (temp_buf = "CPosProcessorModifList").ucptr(), 0, 0);
		XMLWriteSpecSymbEntities(p_writer);
		xmlTextWriterEndDTD(p_writer);
		{
			SXml::WNode n_list(p_writer, "CPosProcessorModifList");
			SaModif mlist;
			n_list.PutInner("GoodsID", temp_buf.Z().Cat(goodsID));
			if(LoadModifiers(goodsID, mlist) > 0) {
				Goods2Tbl::Rec goods_rec;
				for(uint i = 0; i < mlist.getCount(); i++) {
					const SaModifEntry & r_item = mlist.at(i);
					if(GObj.Fetch(r_item.GoodsID, &goods_rec) > 0) {
						SXml::WNode n_item(p_writer, "Item");
						n_item.PutInner("GoodsID", temp_buf.Z().Cat(r_item.GoodsID));
						n_item.PutInner("GoodsName", (temp_buf = goods_rec.Name));
						n_item.PutInner("Flags", temp_buf.Z().Cat(r_item.Flags));
						n_item.PutInner("Price", temp_buf.Z().Cat(r_item.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
						n_item.PutInner("Qtty",  temp_buf.Z().Cat(r_item.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
					}
				}
			}
		}
	}
	xmlTextWriterFlush(p_writer);
	rBuf.CopyFromN(PTRCHRC_(p_xml_buf->content), p_xml_buf->use)/*.UTF8ToChar()*/;
	rBuf.Transf(CTRANSF_INNER_TO_UTF8);
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
	return ok;
}

void CPosProcessor::GetTblOrderList(LDATE lastDate, TSVector <CCheckViewItem> & rList)
{
	rList.clear();
	CCheckFilt cc_filt;
	cc_filt.Period.low = plusdate(getcurdate_(), -7);
	cc_filt.Flags |= CCheckFilt::fOrderOnly;
	//cc_filt.CashNodeID = PNP.NodeID;
	//cc_filt.TableCode = (P_AddParam) ? P_AddParam->TableCode : 0;
	//cc_filt.AgentID = (P_AddParam) ? P_AddParam->AgentID : 0;
	if(InitCcView() && P_CcView->Init_(&cc_filt)) {
		CCheckViewItem item;
		for(P_CcView->InitIteration(0); P_CcView->NextIteration(&item) > 0;) {
			if(!(item.Flags & (CCHKF_SKIP|CCHKF_CLOSEDORDER))) {
				if(!checkdate(lastDate) || item.OrderTime.Start.d == lastDate || item.OrderTime.Finish.d == lastDate)
					rList.insert(&item);
			}
		}
	}
}
//
//
//
CPosProcessor::PosNodeParam::PosNodeParam(PPID posNodeID) : NodeID(posNodeID), CnPhnSvcID(0), CnFlags(0), CnExtFlags(0), CnSpeciality(0), CnLocID(0), 
	ExtCnLocID(0), EgaisMode(0), ChZnPermissiveMode(0), ChZnGuaID(0), AbstractGoodsID(0), AllowedPaymentTypes(0), StatusFlags(0)
{
	AllowedPaymentTypes = ((1 << cpmCash)|(1 << cpmBank));
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	if(cn_obj.Search(NodeID, &cn_rec) > 0) {
		CnName = cn_rec.Name;
		CnSymb = cn_rec.Symb;
		CnFlags = cn_rec.Flags & (CASHF_SELALLGOODS|CASHF_USEQUOT|CASHF_NOASKPAYMTYPE|CASHF_SHOWREST|CASHF_KEYBOARDWKEY|CASHF_WORKWHENLOCK|CASHF_DISABLEZEROAGENT|
			CASHF_UNIFYGDSATCHECK|CASHF_UNIFYGDSTOPRINT|CASHF_CHECKFORPRESENT|CASHF_ABOVEZEROSALE|CASHF_SYNC|CASHF_SKIPUNPRINTEDCHECKS);
		CnExtFlags = cn_rec.ExtFlags;
		CnSpeciality = static_cast<long>(cn_rec.Speciality);
		CnLocID = cn_rec.LocID;
		if(cn_rec.Flags & CASHF_SYNC) {
			PPSyncCashNode cn_pack;
			if(cn_obj.GetSync(NodeID, &cn_pack) > 0) {
				CTblList = cn_pack.CTblList;
				Scf      = cn_pack.Scf;
				ChZnPermissiveMode = cn_pack.ChZnPermissiveMode; // @v12.0.12
				ChZnGuaID = cn_pack.ChZnGuaID; // @v12.0.12
				if(oneof4(cn_pack.EgaisMode, 0, 1, 2, 3)) {
					EgaisMode = cn_pack.EgaisMode;
				}
				{
					assert(AllowedPaymentTypes == ((1 << cpmCash)|(1 << cpmBank))); // see above
					if(oneof2(cn_pack.AllowedPaymentTypes, 0, -1)) {
						if(CnFlags & CASHF_NOASKPAYMTYPE) {
							AllowedPaymentTypes = (1 << cpmCash);
						}
					}
					else {
						if(!(cn_pack.AllowedPaymentTypes & (1 << cpmCash)))
							AllowedPaymentTypes &= ~(1 << cpmCash);
						if(!(cn_pack.AllowedPaymentTypes & (1 << cpmBank)))
							AllowedPaymentTypes &= ~(1 << cpmBank);
						if(AllowedPaymentTypes == 0) { // Какой-то тип оплаты должен быть доступен
							AllowedPaymentTypes = (1 << cpmCash);
						}
					}
				}
			}
			else 
				StatusFlags |= stError;
		}
		if(CnExtFlags & CASHFX_ABSTRGOODSALLOWED) {
			PPObjGoods goods_obj;
			const  PPID def_goods_id = goods_obj.GetConfig().DefGoodsID;
			Goods2Tbl::Rec goods_rec;
			if(def_goods_id && goods_obj.Fetch(def_goods_id, &goods_rec) > 0)
				AbstractGoodsID = def_goods_id;
		}
	}
	else
		StatusFlags |= stError;
}

CPosProcessor::CPosProcessor(PPID cashNodeID, PPID checkID, CCheckPacket * pOuterPack, uint ctrFlags, void * pDummy) : PNP(cashNodeID),
	P_CcView(0), P_TSesObj(0), /*@v12.2.11 P_EgPrc_ToEliminate(0),*/ P_EgMas(0), P_CM(0), P_CM_EXT(0), P_CM_ALT(0), P_GTOA(0), P_ChkPack(pOuterPack), P_DivGrpList(0),
	Flags(0), BonusMaxPart(1.0), OperRightsFlags(0), OrgOperRights(0), SuspCheckID(0), CheckID(checkID), AuthAgentID(0),
	ExtCashNodeID(0), AltRegisterID(0), TouchScreenID(0), ScaleID(0), UiFlags(0),
	State_p(0), LastGrpListUpdTime(ZERODATETIME)
{
	OuterOi.Z();
	MEMSZERO(R__);
	SETFLAG(Flags, fNoEdit, (P_ChkPack || !PNP.NodeID));
	//PPCashNode cn_rec;
	//CnObj.Search(PNP.NodeID, &cn_rec);
	//PNP.CnName = cn_rec.Name;
	//PNP.CnSymb = cn_rec.Symb;
	//PNP.CnFlags = cn_rec.Flags & (CASHF_SELALLGOODS|CASHF_USEQUOT|CASHF_NOASKPAYMTYPE|CASHF_SHOWREST|CASHF_KEYBOARDWKEY|CASHF_WORKWHENLOCK|CASHF_DISABLEZEROAGENT|
		//CASHF_UNIFYGDSATCHECK|CASHF_UNIFYGDSTOPRINT|CASHF_CHECKFORPRESENT|CASHF_ABOVEZEROSALE|CASHF_SYNC|CASHF_SKIPUNPRINTEDCHECKS);
	//PNP.CnExtFlags = cn_rec.ExtFlags;
	//PNP.CnSpeciality = static_cast<long>(cn_rec.Speciality);
	//PNP.CnLocID = cn_rec.LocID;
	//if(cn_rec.Flags & CASHF_SYNC) {
	if(PNP.CnFlags & CASHF_SYNC) {
		PPSyncCashNode cn_pack;
		if(CnObj.GetSync(PNP.NodeID, &cn_pack) > 0) {
			//PNP.CTblList = cn_pack.CTblList;
			//PNP.Scf      = cn_pack.Scf;
			//PNP.ChZnPermissiveMode = cn_pack.ChZnPermissiveMode; // @v12.0.12
			//PNP.ChZnGuaID = cn_pack.ChZnGuaID; // @v12.0.12
			cn_pack.GetPropString(SCN_RPTPRNPORT, RptPrnPort);
			RptPrnPort.Strip();
			if(oneof4(cn_pack.EgaisMode, 0, 1, 2, 3)) {
				//PNP.EgaisMode = cn_pack.EgaisMode;
				if(oneof3(PNP.EgaisMode, 1, 2, 3) && !(Flags & fNoEdit)) {
					/* @v12.2.11
					long   egcf = PPEgaisProcessor::cfDirectFileLogging|PPEgaisProcessor::cfUseVerByConfig;
					if(EgaisMode == 2)
						egcf |= PPEgaisProcessor::cfDebugMode;
					P_EgPrc_ToEliminate = new PPEgaisProcessor(egcf, 0, 0); // @instantiation(PPEgaisProcessor)
					*/
					PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor(); // @v12.2.11 fake call in order to create instance of PPEgaisProcessor
					// @v12.0.12 {
					if(PNP.CnSpeciality == PPCashNode::spCafe) {
						PPObjGoodsType gt_obj;
						PPGoodsType gt_rec;
						bool is_there_egais_auto_wo_flags = false;
						for(SEnum en = gt_obj.Enum(0); !is_there_egais_auto_wo_flags && en.Next(&gt_rec) > 0;) {
							if(gt_rec.Flags & GTF_EGAISAUTOWO) {
								is_there_egais_auto_wo_flags = true;
							}
						}
						if(is_there_egais_auto_wo_flags)
							P_EgMas = new EgaisMarkAutoSelector(/*P_EgPrc*/); 
					}
					// } @v12.0.12 
				}
			}
			// @v11.4.5 {
			if(ctrFlags & ctrfForceInitGroupList && cn_pack.TouchScreenID) {
				PPTouchScreenPacket ts_pack;
				PPObjTouchScreen    ts_obj;
				if(ts_obj.GetPacket(TouchScreenID, &ts_pack) > 0)
					InitGroupList(ts_pack);
			}
			// } @v11.4.5 
		}
	}
	{
		SVector temp_list(sizeof(PPGenCashNode::DivGrpAssc));
		if(PPRef->GetPropArray(PPOBJ_CASHNODE, PNP.NodeID, CNPRP_DIVGRPASSC, &temp_list) > 0 && temp_list.getCount())
			P_DivGrpList = new SArray(temp_list);
	}
	SETFLAG(Flags, fAsSelector, (P_ChkPack && !PNP.NodeID && !CheckID));
	// @v11.4.5 SETFLAG(Flags, fTouchScreen, isTouchScreen);
	SETFLAG(Flags, fTouchScreen, (ctrFlags & ctrfTouchScreen)); // @v11.4.5
	SETFLAG(Flags, fCashNodeIsLocked, CnObj.IsLocked(PNP.NodeID) > 0);
	/* @v12.3.8 if(PNP.CnExtFlags & CASHFX_ABSTRGOODSALLOWED) {
		const  PPID def_goods_id = GObj.GetConfig().DefGoodsID;
		Goods2Tbl::Rec goods_rec;
		if(def_goods_id && GObj.Fetch(def_goods_id, &goods_rec) > 0)
			AbstractGoodsID = def_goods_id;
	}*/
	{
		PPObjLocPrinter lp_obj;
		SETFLAG(Flags, fLocPrinters, lp_obj.IsPrinter());
	}
	{
		struct RtTabEntry { long Orf; long CsR; int8 IsOprRt; };
		static const RtTabEntry rt_tab[] = {
			{ orfReturns,                 CSESSOPRT_RETCHECK,       1 },
			{ orfEscCheck,                CSESSRT_ESCCHECK,         0 },
			{ orfEscChkLine,              CSESSOPRT_ESCCLINE,       1 },
			{ orfBanking,                 CSESSOPRT_BANKING,        1 },
			{ orfZReport,                 CSESSRT_CLOSE,            0 },
			{ orfPreCheck,                CSESSOPRT_PREPRT,         1 },
			{ orfSuspCheck,               CSESSOPRT_SUSPCHECK,      1 },
			{ orfCopyCheck,               CSESSOPRT_COPYCHECK,      1 },
			{ orfCopyZReport,             CSESSOPRT_COPYZREPT,      1 },
			{ orfPrintCheck,              CSESSRT_ADDCHECK,         0 },
			{ orfRowDiscount,             CSESSOPRT_ROWDISCOUNT,    1 },
			{ orfXReport,                 CSESSOPRT_XREP,           1 },
			{ orfSplitCheck,              CSESSOPRT_SPLITCHK,       1 },
			{ orfMergeChecks,             CSESSOPRT_MERGECHK,       1 },
			{ orfChgPrintedCheck,         CSESSOPRT_CHGPRINTEDCHK,  1 },
			{ orfRestoreSuspWithoutAgent, CSESSOPRT_RESTORESUSPWOA, 1 },
			{ orfChgAgentInCheck,         CSESSOPRT_CHGCCAGENT,     1 },
			{ orfEscChkLineBeforeOrder,   CSESSOPRT_ESCCLINEBORD,   1 },
			{ orfReprnUnfCc,              CSESSOPRT_REPRNUNFCC,     1 },
			{ orfArbitraryDiscount,       CSESSOPRT_ARBITRARYDISC,  1 }, // @v11.0.9
		};
		for(uint i = 0; i < SIZEOFARRAY(rt_tab); i++) {
			SETFLAG(OperRightsFlags, rt_tab[i].Orf, CsObj.CheckRights(rt_tab[i].CsR, rt_tab[i].IsOprRt));
		}
		OrgOperRights = OperRightsFlags;
	}
	{
		PPObjSCardSeries scs_obj;
		scs_obj.GetSeriesWithSpecialTreatment(SpcTrtScsList);
	}
}

CPosProcessor::~CPosProcessor()
{
	delete P_CM;
	delete P_CM_EXT;
	delete P_CM_ALT;
	delete P_GTOA;
	delete P_DivGrpList;
	delete P_TSesObj;
	delete P_CcView;
	delete P_EgMas; // @v12.0.12 зависит от P_EgPrc по этому удаляем до P_EgPrc
	// @v12.2.11 delete P_EgPrc_ToEliminate;
}

int CPosProcessor::InitCashMachine()
{
	int    ok = 1;
	THROW(P_CM || (P_CM = PPCashMachine::CreateInstance(PNP.NodeID)) != 0);
	THROW(!ExtCashNodeID || P_CM_EXT || (P_CM_EXT = PPCashMachine::CreateInstance(ExtCashNodeID)) != 0);
	THROW(!AltRegisterID || P_CM_ALT || (P_CM_ALT = PPCashMachine::CreateInstance(AltRegisterID)) != 0);
	CATCHZOK
	return ok;
}

/*virtual*/int CPosProcessor::MessageError(int errCode, const char * pAddedMsg, long outputMode)
{
	SString err_msg;
	if(errCode < 0)
		errCode = PPErrCode;
	else if(errCode > 0)
		PPSetError(errCode, pAddedMsg);
	PPGetMessage(mfError, errCode, pAddedMsg, 1, err_msg);
	PPLogMessage(PPFILNAM_ERRMSG_LOG, err_msg, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
	return 0;
}

int CPosProcessor::MakeGroupEntryList(StrAssocArray * pTreeList, PPID parentID, uint level)
{
	int    ok = -1;
	for(uint i = 0; i < pTreeList->getCount(); i++) {
		StrAssocArray::Item item = pTreeList->Get(i);
		if(item.ParentId == parentID) {
			uint   pos = 0;
			int    is_opened = -1;
			GrpListItem gli;
			if(GroupList.Get(item.Id, &pos)) {
				is_opened = BIN(GroupList.at(pos).Flags & GrpListItem::fOpened);
				GroupList.atFree(pos);
			}
			gli.ID = item.Id;
			gli.ParentID = item.ParentId;
			gli.Level = static_cast<uint16>(level);
			pos = GroupList.getCount();
			GroupList.insert(&gli);
			if(MakeGroupEntryList(pTreeList, item.Id, level + 1) > 0) { // @recursion
				GroupList.at(pos).Flags |= GrpListItem::fFolder;
				if(is_opened > 0)
					GroupList.at(pos).Flags |= GrpListItem::fOpened;
			}
			ok = 1;
		}
	}
	return ok;
}
//
// Инициализация списка групп, которые можно выбирать
//
int CPosProcessor::InitGroupList(const PPTouchScreenPacket & rTsPack)
{
	int    ok = 1;
	SString   temp_buf;
	Goods2Tbl::Rec goods_rec;
	PPIDArray grp_id_list(rTsPack.GrpIDList);
	PPID   grp_id = 0;
	GroupList.clear();
	if(grp_id_list.getCount() == 0) {
		for(GoodsGroupIterator gg_iter(0); gg_iter.Next(&grp_id, temp_buf) > 0;) {
			while(grp_id && GObj.Fetch(grp_id, &goods_rec) > 0) {
				grp_id_list.add(grp_id);
				grp_id = goods_rec.ParentID;
			}
		}
	}
	grp_id_list.sortAndUndup();
	{
		StrAssocArray temp_list;
		for(uint p = 0; p < grp_id_list.getCount(); p++) {
			if(GObj.Fetch(grp_id_list.get(p), &goods_rec) > 0) {
				const  PPID par_id = goods_rec.ParentID;
				temp_list.AddFast(goods_rec.ID, (par_id && grp_id_list.bsearch(par_id)) ? par_id : 0, goods_rec.Name);
			}
		}
		temp_list.SortByText();
		THROW(MakeGroupEntryList(&temp_list, 0, 0));
	}
	LastGrpListUpdTime = getcurdatetime_(); // @v11.4.5
	CATCHZOK
	return ok;
}

int CPosProcessor::GetNewCheckCode(PPID cashNodeID, long * pCode)
{
	int   ok = 1;
	long  code = 1;
	CCheckTbl::Rec chk_rec;
	if(GetCc().GetLastCheckByCode(cashNodeID, &chk_rec) > 0)
		code = chk_rec.Code + 1;
	ASSIGN_PTR(pCode, code);
	return ok;
}

int CPosProcessor::GetCheckInfo(CCheckPacket * pPack)
{
	int    ok = 1;
	CCheckTbl::Rec rec;
	pPack->Rec.PosNodeID = PNP.NodeID;
	CCheckCore & r_cc = GetCc();
	if(CheckID && r_cc.Search(CheckID, &pPack->Rec) > 0) {
		CCheckExtTbl::Rec ext_rec;
		if(r_cc.GetExt(CheckID, &ext_rec) > 0)
			pPack->Ext = ext_rec;
	}
	else {
		if(SuspCheckID && r_cc.Search(SuspCheckID, &rec) > 0) {
			pPack->Rec.Code = rec.Code;
			pPack->Rec.ID   = SuspCheckID;
		}
		else
			GetNewCheckCode(PNP.NodeID, &pPack->Rec.Code);
		THROW(Helper_InitCcPacket(pPack, 0, 0, iccpSetCurTime | iccpDontFillLines));
	}
	if(pPack->Rec.SCardID) {
		SCardOpTbl::Rec scop_rec;
		const  PPID prepay_cc_id = NZOR(pPack->Ext.LinkCheckID, ((pPack->Rec.Flags & CCHKF_ORDER) ? pPack->Rec.ID : 0));
		if(prepay_cc_id && ScObj.P_Tbl->SearchOpByLinkObj(PPOBJ_CCHECK, prepay_cc_id, &scop_rec) > 0 && scop_rec.Amount > 0.0)
			pPack->_OrdPrepay = scop_rec.Amount;
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::SetupState(int st)
{
	int    ok = -1;
	if(State_p != st) {
		State_p = st;
		ok = 1;
	}
	return ok;
}

void CPosProcessor::SetupExt(const CCheckPacket * pPack)
{
	SString temp_buf;
	SetupAgent((pPack ? pPack->Ext.SalerID : 0), 0);
	P.TableCode = pPack ? pPack->Ext.TableNo : 0;
	P.GuestCount = pPack ? pPack->Ext.GuestCount : 0;
	P.OrderCheckID = pPack ? pPack->Ext.LinkCheckID : 0;
	P.Eccd.Z();
	if(pPack) {
		P.Eccd.Memo = pPack->Ext.Memo;
		SETFLAG(P.Eccd.Flags, P.Eccd.fDelivery,   pPack->Rec.Flags & CCHKF_DELIVERY);
		SETFLAG(P.Eccd.Flags, P.Eccd.fFixedPrice, pPack->Rec.Flags & CCHKF_FIXEDPRICE);
		SETFLAG(P.Eccd.Flags, P.Eccd.fSpFinished, pPack->Rec.Flags & CCHKF_SPFINISHED);
		SETFLAG(P.Eccd.Flags, P.Eccd.fImported,   pPack->Rec.Flags & CCHKF_IMPORTED); // @v11.8.11
		if(pPack->Ext.AddrID) {
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Search(pPack->Ext.AddrID, &loc_rec) > 0) {
				P.Eccd.Addr_ = loc_rec;
				PPIDArray sc_list;
				if(ScObj.P_Tbl->GetListByLoc(loc_rec.ID, 0, &sc_list) > 0) {
					if(sc_list.getCount()) {
						P.Eccd.SCardID_ = sc_list.get(0);
						// @todo Что-то надо сделать в случае неоднозначности (sc_list.getCount() > 1)
					}
				}
			}
		}
		P.Eccd.InitDtm = pPack->Ext.CreationDtm;
		P.Eccd.InitUserID = pPack->Ext.CreationUserID;
		// @v11.8.11 pPack->GetGuid(P.Eccd.Uuid); // @v11.5.8
		if(P.Eccd.Flags & P.Eccd.fDelivery) {
			P.Eccd.DlvrDtm = pPack->Ext.StartOrdDtm;
			// @v11.8.0 {
			const LocationTbl::Rec * p_dlvr_addr = pPack->GetDlvrAddr();
			RVALUEPTR(P.Eccd.Addr_, p_dlvr_addr);
			// } @v11.8.0 
		}
		P.AmL = pPack->AL_Const();
		// @v11.8.0 {
		{
			P.Paperless = LOGIC(pPack->Rec.Flags & CCHKF_PAPERLESS);
			if(pPack->GetExtStrData(CCheckPacket::extssBuyerEMail, temp_buf) > 0 && temp_buf.NotEmptyS())
				P.EAddr.SetEMail(temp_buf);
			else if(pPack->GetExtStrData(CCheckPacket::extssBuyerPhone, temp_buf) > 0 && temp_buf.NotEmptyS())
				P.EAddr.SetPhone(temp_buf);
			// @v11.8.11 pPack->GetPrescription(P.Prescr);
		}
		// } @v11.8.0 
		// @v11.8.8 {
		/* @v11.8.11 {
			if(pPack->GetExtStrData(CCheckPacket::extssLinkBillUuid, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				P.Eccd.LinkBillUuid.FromStr(temp_buf);
			}
		}*/
		// } @v11.8.8 
		P.PPExtStrContainer::Copy(*pPack); // @v11.9.10
	}
}

int CPosProcessor::SetupAgent(PPID agentID, int asAuthAgent)
{
	int    ok = 1;
	ArticleTbl::Rec agent_ar_rec;
	THROW(!agentID || ArObj.Search(agentID, &agent_ar_rec) > 0);
	Flags &= ~fUsedRighsByAgent;
	if(agentID) {
		ObjTagItem tag;
		PPID   psn_id = ObjectToPerson(agentID, 0);
		if(psn_id && PPRef->Ot.GetTag(PPOBJ_PERSON, psn_id, PPTAG_PERSON_POSRIGHTS, &tag) > 0) {
			SString rt_buf;
			long   rt = 0, ort = 0, f = 0;
			tag.GetStr(rt_buf);
			PPObjCSession::StringToRights(rt_buf, &rt, &ort);
			f = OrgOperRights;
			//
			// Флаги помеченные как //x закомментированы по причине того,
			// что в диалоге радектирования тега прав персоналии к кассовым операциям
			// данные признаки не присутствуют, а значит не могут быть отключены.
			//
			SETFLAG(f, orfReturns,     ort & CSESSOPRT_RETCHECK);
			SETFLAG(f, orfEscCheck,    rt & CSESSRT_ESCCHECK);
			SETFLAG(f, orfEscChkLine,  ort & CSESSOPRT_ESCCLINE);
			SETFLAG(f, orfBanking,     ort & CSESSOPRT_BANKING);
			SETFLAG(f, orfZReport,     rt & CSESSRT_CLOSE);
			//x SETFLAG(f, orfPreCheck,    ort & CSESSOPRT_PREPRT);
			//x SETFLAG(f, orfSuspCheck,   ort & CSESSOPRT_SUSPCHECK);
			SETFLAG(f, orfCopyCheck,   ort & CSESSOPRT_COPYCHECK);
			SETFLAG(f, orfCopyZReport, ort & CSESSOPRT_COPYZREPT);
			SETFLAG(f, orfPrintCheck,  rt & CSESSRT_ADDCHECK);
			SETFLAG(f, orfRowDiscount, ort & CSESSOPRT_ROWDISCOUNT);
			SETFLAG(f, orfXReport,     ort & CSESSOPRT_XREP);
			//x SETFLAG(f, orfCTblOrd,         ort & CSESSOPRT_CTBLORD);
			SETFLAG(f, orfSplitCheck,      ort & CSESSOPRT_SPLITCHK);
			SETFLAG(f, orfMergeChecks,     ort & CSESSOPRT_MERGECHK);
			SETFLAG(f, orfChgPrintedCheck, ort & CSESSOPRT_CHGPRINTEDCHK);
			SETFLAG(f, orfChgAgentInCheck, ort & CSESSOPRT_CHGCCAGENT);
			SETFLAG(f, orfEscChkLineBeforeOrder, ort & CSESSOPRT_ESCCLINEBORD);
			SETFLAG(f, orfReprnUnfCc,      ort & CSESSOPRT_REPRNUNFCC);
			SETFLAG(f, orfArbitraryDiscount, ort & CSESSOPRT_ARBITRARYDISC); // @v11.0.9

			OperRightsFlags = f;
			Flags |= fUsedRighsByAgent;
			{
				SString added_msg_str;
				GetPersonName(psn_id, added_msg_str);
				DS.GetTLA().AddedMsgStrNoRights = added_msg_str;
			}
		}
		else {
			OperRightsFlags = OrgOperRights;
			DS.GetTLA().AddedMsgStrNoRights.Z();
		}
	}
	else {
		OperRightsFlags = OrgOperRights;
		DS.GetTLA().AddedMsgStrNoRights.Z();
	}
	P.AgentID__ = agentID;
	if(asAuthAgent)
		AuthAgentID = agentID;
	if(SuspCheckID) {
		SETIFZ(P.OrgAgentID, agentID);
		if(OperRightsFlags & orfChgAgentInCheck && agentID && P.OrgAgentID != agentID) {
			SString added_msg_buf;
			GetArticleName(P.OrgAgentID, added_msg_buf);
			if(ConfirmMessage(PPCFM_CHANGECCAGENT, added_msg_buf, 1)) {
				{
					CCheckTbl::Rec org_cc_rec;
					CCheckPacket pack;
					InitCashMachine();
					pack.Rec.SessID = P_CM ? P_CM->GetCurSessID() : 0;
					pack.Rec.PosNodeID = PNP.NodeID;
					pack.Rec.Flags |= (CCHKF_SYNC | CCHKF_NOTUSED);
					Helper_InitCcPacket(&pack, 0, 0, 0);
					if(SuspCheckID && GetCc().Search(SuspCheckID, &org_cc_rec) > 0) {
						pack.Rec.Code = org_cc_rec.Code;
						pack.Rec.ID   = SuspCheckID;
					}
					GetCc().WriteCCheckLogFile(&pack, 0, CCheckCore::logAgentChanged, 0);
				}
				P.OrgAgentID = agentID;
			}
		}
	}
	else
		P.OrgAgentID = agentID;
	CATCHZOK
	return ok;
}

double CPosProcessor::CalcCurrentRest(PPID goodsID, bool checkInputBuffer)
{
	double rest = 0.0;
	if(GetCc().CalcGoodsRest(goodsID, getcurdate_(), GetCnLocID(goodsID), &rest)) {
		for(uint pos = 0; P.lsearch(&goodsID, &pos, CMPF_LONG); pos++)
			rest -= P.at(pos).Quantity;
		if(checkInputBuffer && P.HasCur()) {
			const CCheckItem & r_buf_item = P.GetCurC();
			if(r_buf_item.GoodsID == goodsID)
				rest -= r_buf_item.Quantity;
		}
	}
	return rest;
}

/*virtual*/void CPosProcessor::SetupRowData(bool doCalcRest)
{
	const CCheckItem & r_item = P.GetCur();
	SetupState(oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF) ? sLISTSEL_BUF : (P.getCount() ? sLIST_BUF : sEMPTYLIST_BUF));
	if(doCalcRest) {
		// @v11.8.3 Если опции кассового узла не предписывают отображение остатка, то (наверное) и расчитывать не стоит. 
		// Расчет остатков иногда может быть долгим.
		P.SetRest((PNP.CnFlags & CASHF_SHOWREST) ? CalcCurrentRest(r_item.GoodsID, false/*checkInputBuffer*/) : 0.0);
	}
	SetupInfo(0);
}

bool FASTCALL CPosProcessor::BelongToExtCashNode(PPID goodsID) const
{
	PPID   assoc_id = 0;
	return (P_GTOA && P_GTOA->Get(goodsID, &assoc_id) > 0 && assoc_id && assoc_id == (DS.CheckExtFlag(ECF_CHKPAN_USEGDSLOCASSOC) ? PNP.ExtCnLocID : ExtCashNodeID));
}

PPID FASTCALL CPosProcessor::GetChargeGoodsID(PPID scardID)
{
	const PPID id = ScObj.GetChargeGoodsID(scardID);
	return NZOR(id, UNDEF_CHARGEGOODSID);
}

PPID CPosProcessor::GetAuthAgentID() const
{
	if(!AuthAgentID)
		PPSetError(PPERR_CPOS_UNDEFAUTHAGENT);
	return AuthAgentID;
}

double CPosProcessor::GetUsableBonus() const 
{ 
	double result = 0.0;
	if(Flags & fSCardBonus) {
		if(CSt.CSTRB.SpecialTreatment && CSt.CSTRB.Flags & CSt.CSTRB.fBonusDisabled)
			result = 0.0;
		else
			result = CSt.UsableBonus;
	}
	return result;
}

double CPosProcessor::GetBonusMaxPart() const { return BonusMaxPart; }
double CPosProcessor::RoundDis(double d) const { return PPRound(d, R__.DisRoundPrec, R__.DisRoundDir); }

int CPosProcessor::SetupCTable(int tableNo, int guestCount)
{
	int    ok = -1;
	if(tableNo >= 0 && (P.TableCode != tableNo || (guestCount && P.GuestCount != guestCount))) {
		P.TableCode  = tableNo;
		P.GuestCount = guestCount;
		SetupInfo(0);
		ok = 1;
	}
	return ok;
}

int CPosProcessor::SetupUuid(const S_GUID & rUuid)
{
	int    ok = -1;
	SString temp_buf; // @v11.8.11
	if(!!rUuid) {
		temp_buf.Cat(rUuid, S_GUID::fmtIDL); // @v11.8.11 
		// @v11.8.11 P.Eccd.Uuid = rUuid;
		ok = 1;
	}
	P.PutExtStrData(CCheckPacket::extssUuid, temp_buf); // @v11.8.11
	return ok;
}
/*
// @test
int CPosProcessor::SetupItem(PPID goodsID, double qtty, double price)
{
	CCheckItem & r_item = P.GetCur();
	r_item.GoodsID = goodsID;
	r_item.Quantity = fabs(qtty);
	r_item.Price    = 10;
	r_item.Discount = 0.0;
	P.insert(&r_item);
	P.CurCcItemPos = P.getCount();
	return 1;
}
*/
int CPosProcessor::OpenSession(LDATE * pDt, int ifClosed)
{
	int    ok = 0;
	bool   is_openend = false;
	THROW(InitCashMachine());
	if(ifClosed) {
		PPCashNode cn_rec;
		CnObj.Search(PNP.NodeID, &cn_rec);
		if(cn_rec.Flags & CASHF_DAYCLOSED || !cn_rec.CurDate || ((cn_rec.Flags & CASHF_CHKPAN) && !cn_rec.CurSessID))
			is_openend = true;
	}
	else
		is_openend = true;
	if(is_openend) {
		int    r_ext = 0;
		const  int r = P_CM ? P_CM->SyncOpenSession(pDt) : 0;
		if(P_CM_EXT) {
			if(P_CM_EXT->GetNodeData().CashType == PPCMT_PAPYRUS) {
				if(r > 0) {
					P_CM_EXT->SetParentNode(PNP.NodeID);
					P_CM_EXT->AsyncOpenSession(0, 0);
				}
			}
			else
				r_ext = P_CM_EXT->SyncOpenSession(pDt);
		}
		if(r > 0 || r_ext > 0) {
			Flags &= ~fOnlyReports;
			ok = 1;
		}
	}
	else
		ok = 1;
	CATCHZOK
	return ok;
}
//
//
//
int CPosProcessor::CalcRestByCrdCard_(int checkCurItem)
{
	int    ok = 1;
	SCardTbl::Rec sc_rec;
	PPSCardSeries scs_rec;
	PPObjSCardSeries scs_obj;
	CSt.MaxCreditByCrdCard = 0.0;
	CSt.RestByCrdCard = 0.0;
	CSt.UsableBonus = 0.0;
	CSt.AdditionalPayment = 0.0;
	Flags &= ~(fSCardCredit|fSCardBonus|fSCardBonusReal);
	if(ScObj.Search(CSt.GetID(), &sc_rec) > 0 && scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && (scs_rec.Flags & (SCRDSF_CREDIT|SCRDSF_BONUS))) {
		const int scst = scs_rec.GetType();
		if(IsCurrentOp(CCOP_RETURN)) {
			if(Flags & fRetByCredit) {
				const CcTotal cct = CalcTotal();
				const double credit_part = fdivnz(P.Lb_._CcCredit, P.Lb_._CcAmount);
				const double ret_by_credit = cct.Amount * credit_part;
				if(ret_by_credit < 0.0 && ret_by_credit > cct.Amount)
					CSt.AdditionalPayment = R2(cct.Amount - ret_by_credit);
			}
		}
		else {
			int    skip_crd_processing = 0;
			double init_rest = R2((scs_rec.Flags & SCRDSF_UHTTSYNC) ? CSt.CSTRB.Rest/*OuterSvcRest*/ : sc_rec.Rest);
			if(init_rest < 0.0 && scst == scstBonus)
				init_rest = 0.0;
			double rest = 0.0;
			const double fixed_bonus = fdiv100i(sc_rec.FixedBonus);
			CSt.RestByCrdCard = (fixed_bonus > 0.0) ? MIN(fixed_bonus, init_rest) : init_rest; // @todo FixedBonus надо доработать
			Flags |= fSCardCredit;
			if(scst == scstBonus) {
				PPSCardConfig sc_cfg;
				ScObj.FetchConfig(&sc_cfg);
				//
				// Необходимо различать случаи использования бонусов и начисления бонусов.
				// Флаг fSCardBonusReal позволяет начислить на бонусную карту деньги, но не позволяет
				// списывать, флаг же fSCardBonus позволяет списывать (остаток не нулевой).
				if(!(sc_cfg.Flags & sc_cfg.fDontUseBonusCards))
					Flags |= fSCardBonusReal;
				if(!(sc_cfg.Flags & sc_cfg.fDontUseBonusCards) && init_rest > 0.0)
					Flags |= fSCardBonus;
				else {
					Flags &= ~fSCardCredit;
					CSt.RestByCrdCard = 0.0;
					skip_crd_processing = 1;
				}
			}
			else if(scst == scstCredit) {
				CSt.MaxCreditByCrdCard = sc_rec.MaxCredit;
			}
			if(!skip_crd_processing) {
				double non_crd_amt = 0.0;
				double add_paym = 0.0;
				double cc = -CalcCreditCharge(0, 0, checkCurItem ? &P.GetCur() : 0, &non_crd_amt, 0);
				const  CcTotal cct = CalcTotal();
				if(Flags & fSCardBonus && BonusMaxPart < 1.0 && cc > 0.0) {
					double cc_ = R2(MIN(cc, cct.Amount * BonusMaxPart));
					cc = MIN(cc_, CSt.RestByCrdCard);
					add_paym = cct.Amount-cc;
				}
				else if(scst == scstCredit && CSt.MaxCreditByCrdCard > 0.0 && cc > (CSt.RestByCrdCard + CSt.MaxCreditByCrdCard)) {
					//
					// Если по карте установлен положительный кредитный лимит и он исчерпан, то перестаем воспринимать
					// эту карту как кредитную.
 					//
					Flags &= ~fSCardCredit;
					CSt.RestByCrdCard = 0.0;
					skip_crd_processing = 1;
					MessageError(PPERR_CHKPAN_SCOUTOFCRDLIMIT, sc_rec.Code, /*eomMsgWindow*/eomPopup|eomBeep);
				}
				else {
					// @v11.1.10 (возвращаем назад) add_paym = non_crd_amt - (cc + CSt.RestByCrdCard + CSt.MaxCreditByCrdCard);
					add_paym = cc - (CSt.RestByCrdCard + CSt.MaxCreditByCrdCard); // @v11.1.10 (возвращаем назад)
				}
				if(!skip_crd_processing) {
					if(Flags & fSCardBonus)
						CSt.UsableBonus = MIN(MAX(cc, 0.0), init_rest);
					else /*if(!(Flags & fSCardBonus))*/ { // Для бонусных карт запрос доплаты не выводится и сумма дебета не корректируется //
						if(add_paym > 0.0) {
							if(R2(CSt.AdditionalPayment) == 0.0) { 
								if(scs_rec.Flags & SCRDSF_DISABLEADDPAYM)
									ok = MessageError(PPERR_UNABLEADDPAYMONCRDCARD, 0, /*eomMsgWindow*/eomPopup|eomBeep);
								else {
									SString  buf;
									buf.Cat(add_paym, SFMT_MONEY);
									if(!ConfirmMessage(PPCFM_MAXCRD_OVERDRAFT, buf, 1)) {
										//
										// @? Значение доплаты все равно устанавливаем
										//
										CSt.AdditionalPayment = R2(add_paym);
										//
										ok = 0;
									}
								}
							}
							if(ok) {
								CSt.AdditionalPayment = R2(add_paym);
								if((CSt.RestByCrdCard + cc) < -CSt.MaxCreditByCrdCard) // @v10.9.0 @fix (- cc)-->(+ cc)
									CSt.RestByCrdCard = -CSt.MaxCreditByCrdCard;
								else
									CSt.RestByCrdCard += cc; // @v10.9.0 @fix (-=)-->(+=)
							}
						}
						else {
							CSt.RestByCrdCard -= cc; // @v10.9.0 @fix (-=)-->(+=) // @v11.0.0 @fix-again (+=)-->(-=)
							CSt.AdditionalPayment = (add_paym < 0.0) ? 0.0 : R2(add_paym);
						}
					}
					CSt.RestByCrdCard = R2(CSt.RestByCrdCard);
					CSt.UsableBonus = R2(CSt.UsableBonus);
				}
			}
		}
		SetupInfo(0);
	}
	return ok;
}

CPosProcessor::CcTotal::CcTotal() : Amount(0.0), Discount(0.0)
{
}

CPosProcessor::CcTotal CPosProcessor::CalcTotal() const
{
	CcTotal _t;
	SForEachVectorItem(P, i) {
		const CCheckItem & r_item = P.at(i);
		if(r_item.Flags & cifGift) {
			_t.Amount   = R2(_t.Amount - r_item.Quantity * r_item.Discount);
			_t.Discount = R2(_t.Discount + r_item.Quantity * r_item.Discount);
		}
		else {
			_t.Amount   = R2(_t.Amount + r_item.GetAmount()); // @R2
			_t.Discount = R2(_t.Discount + r_item.Quantity * r_item.Discount); // @R2
		}
	}
	_t.Amount = R2(_t.Amount);
	return _t;
}

struct CPosProcessor_SetupDiscontBlock {
	CPosProcessor_SetupDiscontBlock(double roundingDiscount, int distributeGiftDiscount) : IsRounding(BIN(roundingDiscount != 0.0)),
		DistributeGiftDiscount(distributeGiftDiscount), LastIndex(0), Amount(0.0), P_Scst(0)
	{
	}
	~CPosProcessor_SetupDiscontBlock()
	{
		delete P_Scst;
	}
	const int IsRounding;
	const int DistributeGiftDiscount;
	uint   LastIndex;
	double Amount;
	RAssocArray GiftDisList;
	LongArray WoDisPosList;
	SCardSpecialTreatment * P_Scst;
	SCardSpecialTreatment::CardBlock ScstCb;
};

int CPosProcessor::Helper_PreprocessDiscountLoop(int mode, void * pBlk)
{
	int    result = 1;
	CPosProcessor_SetupDiscontBlock & r_blk = *static_cast<CPosProcessor_SetupDiscontBlock *>(pBlk);
	if(mode == 0 || (mode == 1 && r_blk.P_Scst)) {
		PPObjBill * p_bobj(BillObj);
		CCheckItem * p_item;
		double min_qtty  = SMathConst::Max;
		double max_price = 0.0;
		//const  long rpe_flags = ((CSt.Flags & CardState::fUseMinQuotVal) ? RTLPF_USEMINEXTQVAL : 0) | RTLPF_USEQUOTWTIME;
		uint    i;
		PPIDArray lot_list;
		StringSet addendum_msg_list;
		SString temp_buf;
		int     is_there_scst_goods_to_query = 0;
		TSVector <SCardSpecialTreatment::DiscountBlock> scst_dbl;
		for(i = 0; P.enumItems(&i, (void **)&p_item);) {
			double qtty = fabs(p_item->Quantity);
			double gift_item_dis = 0.0; // Подарочная суммовая скидка по строке
			double item_price = p_item->Price;
			double item_discount = p_item->Discount;
			int    no_discount = 0;
			int    no_calcprice = 0;
			RetailGoodsInfo rgi;
			long   ext_rgi_flags = 0;
			const  PPID goods_id = p_item->GoodsID;
			{
				//
				// Рассчитываем цену {
				//
				double price_by_serial = 0.0;
				lot_list.clear();
				if(p_item->Serial[0] && p_bobj->SearchLotsBySerialExactly(p_item->Serial, &lot_list) > 0) { // @v11.4.2 SearchLotsBySerial-->SearchLotsBySerialExactly
					ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
					LDATE  last_date = ZERODATE;
					ReceiptTbl::Rec lot_rec;
					const  PPID  loc_id = GetCnLocID(goods_id);
					for(uint j = 0; j < lot_list.getCount(); j++) {
						if(r_rcpt.Search(lot_list.get(j), &lot_rec) > 0 && lot_rec.GoodsID == goods_id && lot_rec.LocID == loc_id && last_date < lot_rec.Dt) {
							price_by_serial = lot_rec.Price;
							last_date = lot_rec.Dt;
						}
					}
				}
				if(price_by_serial > 0.0) {
					ext_rgi_flags |= PPObjGoods::rgifUseOuterPrice;
					rgi.OuterPrice = price_by_serial;
				}
				const int _gpr = GetRgi(goods_id, qtty, ext_rgi_flags, rgi);
				if(_gpr > 0)
					item_price = rgi.Price;
				if(rgi.Flags & RetailGoodsInfo::fNoDiscount || item_price == 0.0) // @v11.5.8 (|| item_price == 0.0)
					no_discount = 1;
				//
				if(_gpr > 0 && (rgi.QuotKindUsedForExtPrice && rgi.ExtPrice >= 0.0)) {
					if(rgi.Flags & rgi.fDisabledQuot) { // Исключительная ситуация: ExtPrice перебивает по приоритету блокированную котировку
						item_price = rgi.ExtPrice;
						item_discount = 0.0;
					}
					else
						item_discount = (item_price - rgi.ExtPrice);
					no_discount = 1;
				}
				else {
					const RetailPriceExtractor::ExtQuotBlock * p_eqb = GetCStEqbND(no_discount);
					if(p_eqb && p_eqb->QkList.getCount() && !(CSt.Flags & CardState::fUseDscntIfNQuot))
						no_discount = 1;
					item_discount = 0.0;
				}
				// }
			}
			if(mode == 1) {
				assert(r_blk.P_Scst);
				if(r_blk.P_Scst && r_blk.P_Scst->DoesWareBelongToScope(goods_id) > 0) {
					SCardSpecialTreatment::DiscountBlock db;
					db.RowN = i;
					db.GoodsID = goods_id;
					SETFLAG(db.Flags, db.fProcessed, p_item->RemoteProcessingTa[0]);
					db.InPrice = item_price-item_discount;
					db.ResultPrice = item_price-item_discount;
					db.Qtty = fabs(p_item->Quantity);
					STRNSCPY(db.TaIdent, p_item->RemoteProcessingTa);
					//scst_dbl.clear();
					scst_dbl.insert(&db);
				}
			}
			else {
				if(p_item->Flags & cifGiftDiscount && r_blk.DistributeGiftDiscount) {
					gift_item_dis = qtty * p_item->Discount;
					if(gift_item_dis != 0.0) {
						r_blk.GiftDisList.Add((long)(i-1), gift_item_dis);
						if(!r_blk.IsRounding)
							r_blk.Amount = R2(r_blk.Amount - gift_item_dis); // Сумму подарочной скидки необходимо вычесть из базы для расчета общей скидки
					}
					assert(p_item->Price == 0.0);
					p_item->Discount = 0.0;
				}
				if(p_item->Flags & (cifGift|cifQuotedByGift|cifPartOfComplex)) {
					no_calcprice = 1;
					no_discount = 1;
				}
				if(r_blk.IsRounding || (p_item->Flags & cifFixedPrice) || (P.Eccd.Flags & P.Eccd.fFixedPrice)) {
					no_calcprice = 1;
					if(p_item->RemoteProcessingTa[0])
						no_discount = 1;
				}
				if(!no_calcprice) {
					p_item->Price = item_price;
					p_item->Discount = item_discount;
				}
				else if(!no_discount) { // Дополнительный блок для правильной идентификации блокировки скидки
					const int _gpr = GetRgi(goods_id, qtty, ext_rgi_flags, rgi);
					if(rgi.Flags & RetailGoodsInfo::fNoDiscount)
						no_discount = 1;
				}
				if(no_discount) {
					r_blk.WoDisPosList.addUnique(i);
				}
				else {
					const double p = R2(r_blk.IsRounding ? p_item->NetPrice() : p_item->Price);
					r_blk.Amount = R2(r_blk.Amount + p * qtty);
					if(qtty > 0.0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
						r_blk.LastIndex = i;
						min_qtty = qtty;
						max_price = p;
					}
				}
			}
		}
		if(scst_dbl.getCount()) {
			assert(mode == 1);
			long    qdrf = 0;
			if(r_blk.P_Scst->QueryDiscount(&r_blk.ScstCb, scst_dbl, &qdrf, &addendum_msg_list.Z()) > 0) {
				for(i = 0; i < scst_dbl.getCount(); i++) {
					const SCardSpecialTreatment::DiscountBlock & r_db = scst_dbl.at(i);
					p_item = &P.at(r_db.RowN-1);
					if(isempty(p_item->RemoteProcessingTa)) {
						p_item->Discount = 0.0;
						p_item->Price = r_db.ResultPrice;
						p_item->Flags |= cifFixedPrice;
						STRNSCPY(p_item->RemoteProcessingTa, r_db.TaIdent);
					}
				}
				for(uint sp = 0; addendum_msg_list.get(&sp, temp_buf);)
					MsgToDisp_Add(temp_buf);
			}
		}
		r_blk.Amount = R2(r_blk.Amount);
	}
	return result;
}

void CPosProcessor::Helper_SetupDiscount(double roundingDiscount, int distributeGiftDiscount)
{
	//
	// Если товар не имеет котировки, то на него распространяется общая скидка.
	//
	CPosProcessor_SetupDiscontBlock sdb(roundingDiscount, distributeGiftDiscount);
	if(CSt.GetID() && CSt.CSTRB.SpecialTreatment) {
		SCardSpecialTreatment * p_st = SCardSpecialTreatment::CreateInstance(CSt.CSTRB.SpecialTreatment);
		if(p_st && p_st->GetCapability() & SCardSpecialTreatment::capfItemDiscount)
			sdb.P_Scst = p_st;
		else
			delete p_st;
	}
	if(sdb.P_Scst) {
		if(SCardSpecialTreatment::InitSpecialCardBlock(CSt.GetID(), PNP.NodeID, sdb.ScstCb) > 0) {
			Helper_PreprocessDiscountLoop(1/*mode*/, &sdb); // Первый цикл (mode = 1) для запроса к сторонним сервисам
		}
		else {
			ZDELETE(sdb.P_Scst);
		}
	}
	Helper_PreprocessDiscountLoop(0/*mode*/, &sdb); // Основной цикл (mode = 0)
	{
		//
		// Специальный признак, индицирующий то, что финишная скидка
		// на элемент (last_index-1) должна прибавляться к существующей скидке,
		// а не замещать ее. Чаще всего этот признак просто равен is_rounding,
		// однако, если происходит распределение подарочной скидки по позиции
		// (last_index-1), то finish_addendum становится равным 1.
		//
		int    finish_addendum = sdb.IsRounding;
		//
		CCheckItem * p_item;
		double discount = 0.0;
		if(sdb.IsRounding)
			discount = roundingDiscount;
		else {
			const double _dis = CSt.GetDiscount(sdb.Amount);
			discount = _dis * fdiv100r(sdb.Amount);
			CSt.SettledDiscount = _dis;
			// @v11.0.9 {
			if(discount < sdb.Amount && ManDis.Discount > 0.0) {
				double mandiscount = 0.0;
				if(ManDis.Flags & ManualDiscount::fPct) {
					const double _mpctdis = MIN(ManDis.Discount, 100.0);
					mandiscount = _mpctdis * (sdb.Amount - discount) / 100.0;
				}
				else {
					mandiscount = discount + ManDis.Discount;
					SETMIN(mandiscount, sdb.Amount);
				}
				ManDis.SettledAbsolutDiscount = mandiscount;
				discount = mandiscount;
			}
			// } @v11.0.9 
		}
		double part_dis = 0.0;
		double part_amount = 0.0;
		if(!sdb.IsRounding) {
			if(discount != 0.0) {
				const double temp_dis = this->RoundDis(discount);
				if(temp_dis < sdb.Amount)
					discount = temp_dis;
			}
			for(uint i = 0; P.enumItems(&i, (void **)&p_item);)
				if(i != sdb.LastIndex && !sdb.WoDisPosList.lsearch(i)) {
					const double qtty = fabs(p_item->Quantity);
					const double p    = R2(sdb.IsRounding ? p_item->NetPrice() : p_item->Price); // @R2
					double d = this->RoundDis(fdivnz(p * (discount - part_dis), (sdb.Amount - part_amount)));
					SETMIN(d, p); // Гарантируем то, что скидка не превысит цену
					p_item->Discount = d;
					part_dis    += (d * qtty);
					part_amount += (p * qtty);
				}
			//
			// Распределяем специальную подарочную скидку по тем позициям, на основании
			// которых она была предоставлена.
			// {
			for(uint j = 0; j < sdb.GiftDisList.getCount(); j++) {
				uint   i;
				const  uint main_pos = sdb.GiftDisList.at(j).Key;
				const  double dis = sdb.GiftDisList.at(j).Val;
				double gift_part_dis = 0.0;
				double gift_part_amt = 0.0;
				double gift_amt = 0.0;
				LongArray pos_list;
				LongArray main_pos_list;
				P.GiftAssoc.GetListByKey(main_pos, pos_list);
				uint   plc = pos_list.getCount();
				for(i = 0; i < plc; i++) {
					const uint pos = pos_list.get(i);
					assert(pos < P.getCount() && pos != main_pos);
					p_item = &P.at(pos);
					const double item_amt = p_item->NetPrice() * fabs(p_item->Quantity);
					gift_amt = R2(gift_amt + R2(item_amt));
					if(p_item->Flags & cifMainGiftItem /*&& dis < item_amt*/)
						main_pos_list.addUnique(static_cast<int>(pos));
				}
				if(main_pos_list.getCount()) {
					//
					// Специальный случай: подарочная скидка распределяется только на основной компонент подарочной структуры
					//
					pos_list = main_pos_list; // Подменяем pos_list на main_pos_list
					gift_amt = 0.0;
					//
					plc = pos_list.getCount();
					for(i = 0; i < plc; i++) {
						const uint pos = pos_list.get(i);
						assert(pos < P.getCount() && pos != main_pos);
						p_item = &P.at(pos);
						const double item_amt = p_item->NetPrice() * fabs(p_item->Quantity);
						gift_amt = R2(gift_amt + R2(item_amt));
					}
				}
				for(i = 0; i < plc; i++) {
					const uint _pos = pos_list.get(i);
					if(sdb.LastIndex && _pos == (sdb.LastIndex-1))
						finish_addendum = 1;
					p_item = &P.at(_pos);
					const double qtty = fabs(p_item->Quantity);
					const double p    = R2(p_item->NetPrice()); // @R2
					double d = this->RoundDis((i == (plc-1)) ? ((dis - gift_part_dis) / qtty) : (fdivnz(p * (dis - gift_part_dis), (gift_amt - gift_part_amt))));
					SETMIN(d, p); // Гарантируем то, что скидка не превысит цену
					p_item->Discount = this->RoundDis(p_item->Discount + d);
					gift_part_dis += (d * qtty);
					gift_part_amt += (p * qtty);
				}
			}
		}
		if(sdb.LastIndex) {
			p_item = &P.at(sdb.LastIndex-1);
			const double qtty = fabs(p_item->Quantity);
			const double org_p = R2(p_item->Price);
			const double p    = R2(sdb.IsRounding ? p_item->NetPrice() : p_item->Price); // @R2
			double d = (discount - part_dis) / qtty;
			if(!sdb.IsRounding)
				d = this->RoundDis(d);
			if(finish_addendum) {
				if((p_item->Discount + d) > org_p)
					d = org_p;
				p_item->Discount = sdb.IsRounding ? (p_item->Discount + d) : this->RoundDis(p_item->Discount + d);
			}
			else {
				SETMIN(d, org_p); // Гарантируем то, что скидка не превысит цену
				p_item->Discount = d;
			}
			part_dis    += (d * qtty); // @debug
			part_amount += (p * qtty); // @debug
		}
	}
}

void CPosProcessor::SetupDiscount(int distributeGiftDiscount /*=0*/)
{
	Helper_SetupDiscount(0.0, distributeGiftDiscount);
	const CcTotal cct = CalcTotal();
	double new_amt = (R__.AmtRoundPrec != 0.0) ? PPRound(cct.Amount, R__.AmtRoundPrec, R__.AmtRoundDir) : R2(cct.Amount);
	double diff = R2(cct.Amount - new_amt);
	if(!feqeps(diff, 0.0, 1E-6)) {
		Helper_SetupDiscount(diff, 0);
	}
}

int CPosProcessor::VerifyPrices()
{
	int    ok = 1;
	if(P.getCount()) {
		for(uint i = 0; ok && i < P.getCount(); i++) {
			const CCheckItem & r_item = P.at(i);
			RealRange pr;
			int r = CheckPriceRestrictions(labs(r_item.GoodsID), r_item, r_item.NetPrice(), &pr);
			if(!r) {
				ok = 0;
			}
		}
	}
	return ok;
}

void CPosProcessor::ResetSCard()
{
	CSt.Z();
	Flags &= ~(fSCardCredit|fSCardBonus|fSCardBonusReal|fPctDis); // @scard
	SetupDiscount(0);
}

int CPosProcessor::GetRgi(PPID goodsID, double qtty, long extRgiFlags, RetailGoodsInfo & rRgi)
{
	const  long rgi_flags = (PNP.CnFlags & CASHF_USEQUOT) ? (PPObjGoods::rgifUseQuotWTimePeriod|PPObjGoods::rgifUseBaseQuotAsPrice) : PPObjGoods::rgifUseQuotWTimePeriod;
	bool   nodis = false;
	const LDATETIME actual_dtm = P.Eccd.InitDtm;
	PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor(); // @v12.2.11
	int    r = GObj.GetRetailGoodsInfo(goodsID, GetCnLocID(goodsID), GetCStEqb(goodsID, &nodis), p_eg_prc, P.GetAgentID(), actual_dtm, fabs(qtty), &rRgi, rgi_flags|extRgiFlags);
	SETFLAG(rRgi.Flags, RetailGoodsInfo::fNoDiscount, nodis);
	return r;
}

int CPosProcessor::LoadPartialStruc(PPID goodsID, PPGoodsStruc & rGs)
{
	return (GObj.LoadGoodsStruc(PPGoodsStruc::Ident(goodsID, GSF_PARTITIAL, GSF_PRESENT|GSF_COMPLEX|GSF_SUBST, getcurdate_()), &rGs) > 0 && !rGs.IsEmpty()) ? 1 : -1;
}

int CPosProcessor::LoadComplex(PPID goodsID, SaComplex & rComplex)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec, item_goods_rec;
	rComplex.Init(goodsID, 0, 1.0);
	if(GObj.Fetch(goodsID, &goods_rec) > 0 && goods_rec.Flags & GF_GENERIC) {
		PPGoodsStruc gs;
		if(GObj.LoadGoodsStruc(PPGoodsStruc::Ident(goodsID, GSF_COMPLEX, 0, getcurdate_()), &gs) > 0) {
			rComplex.Init(goodsID, gs.Rec.ID, 1.0);
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			double item_qtty = 0.0;
			PPIDArray gen_list;
			PPGoodsStrucItem gs_item;
			{
				RetailGoodsInfo rgi;
				THROW(GetRgi(goodsID, 0.0, 0, rgi));
				rComplex.Price = rgi.Price;
			}
			for(uint p = 0; gs.EnumItemsExt(&p, &gs_item, 0, rComplex.Qtty, &item_qtty) > 0;) {
				if(GObj.Fetch(gs_item.GoodsID, &item_goods_rec) > 0) {
					{
						SaComplexEntry entry(gs_item.GoodsID, fabs(item_qtty));
						THROW_SL(rComplex.insert(&entry));
					}
					SaComplexEntry & r_entry = rComplex.at(rComplex.getCount()-1);
					if(item_goods_rec.Flags & GF_GENERIC) {
						gen_list.clear();
						r_entry.Flags |= r_entry.fGeneric;
						if(GObj.GetGenericList(r_entry.GoodsID, &gen_list) > 0 && gen_list.getCount()) {
							double sum = 0.0;
							for(uint i = 0; i < gen_list.getCount(); i++) {
								const  PPID gen_goods_id = gen_list.get(i);
								RetailGoodsInfo rgi;
								THROW(GetRgi(gen_goods_id, 0.0, PPObjGoods::rgifUseBaseQuotAsPrice, rgi));
								if(rgi.Price > 0.0) {
									r_entry.GenericList.Add(gen_goods_id, rgi.Price, 0);
									sum += (rgi.Price * r_entry.Qtty);
								}
							}
							if(r_entry.Qtty > 0.0)
								r_entry.OrgPrice = (sum / r_entry.Qtty);
						}
					}
					else {
						RetailGoodsInfo rgi;
						THROW(GetRgi(r_entry.GoodsID, 0.0, 0, rgi));
						r_entry.OrgPrice = rgi.Price;
					}
				}
			}
			if(rComplex.getCount())
				ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
double CPosProcessor::CalcSCardOpAmount(const CCheckLineTbl::Rec & rItem, PPID chargeGoodsID, PPID crdGoodsGrpID, double * pNonCrdAmt)
{
	double charge = 0.0;
	double non_crd_amt = 0.0;
	const double s  = intmnytodbl(rItem.Price) * rItem.Quantity;
	const double ds = rItem.Dscnt * rItem.Quantity;
	if(crdGoodsGrpID) {
		if(GObj.BelongToGroup(rItem.GoodsID, crdGoodsGrpID, 0) > 0)
			charge = -R3(rItem.Quantity);
		else if(!oneof2(chargeGoodsID, 0, UNDEF_CHARGEGOODSID) && rItem.GoodsID == chargeGoodsID) {
			if(crdGoodsGrpID)
				charge = R3(rItem.Quantity);
			else
				charge = R2(s - ds);
		}
		else
			non_crd_amt = R2(s - ds);
	}
	else {
		if(!oneof2(chargeGoodsID, 0, UNDEF_CHARGEGOODSID) && rItem.GoodsID == chargeGoodsID)
			charge = R2(s - ds);
		else
			charge = -R2(s - ds);
	}
	ASSIGN_PTR(pNonCrdAmt, non_crd_amt);
	return charge;
}

/*static*/double CPosProcessor::Helper_CalcSCardOpBonusAmount(const CCheckLineTbl::Rec & rItem, PPObjGoods & rGObj, PPID bonusGoodsGrpID, double * pNonCrdAmt)
{
	double charge = 0.0;
	double non_crd_amt = 0.0;
	const double s  = intmnytodbl(rItem.Price) * rItem.Quantity;
	const double ds = rItem.Dscnt * rItem.Quantity;
	if(bonusGoodsGrpID) {
		if(rGObj.BelongToGroup(rItem.GoodsID, bonusGoodsGrpID, 0) > 0)
			charge = -R2(s - ds);
		else
			non_crd_amt = R2(s - ds);
	}
	else {
		charge = -R2(s - ds);
	}
	ASSIGN_PTR(pNonCrdAmt, non_crd_amt);
	return charge;
}

double CPosProcessor::CalcSCardOpBonusAmount(const CCheckLineTbl::Rec & rItem, PPID bonusGoodsGrpID, double * pNonCrdAmt)
{
	return Helper_CalcSCardOpBonusAmount(rItem, GObj, bonusGoodsGrpID, pNonCrdAmt);
}

double CPosProcessor::CalcCreditCharge(const CCheckPacket * pPack, const CCheckPacket * pExtPack,
	const CCheckItem * pCurItem, double * pNonCrdAmt, double * pBonusChargeAmt)
{
	double charge = 0.0;
	double non_crd_amt = 0.0;
	double bonus_charge_amt = 0.0;
	const  PPID scard_id = pPack ? pPack->Rec.SCardID : CSt.GetID();
	uint   i;
	SCardTbl::Rec sc_rec;
	if(scard_id && ScObj.Search(scard_id, &sc_rec) > 0) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) {
			double nca = 0.0;
			CCheckLineTbl::Rec ccl_rec;
			CCheckItem * p_item;
			if(scs_rec.GetType() == scstBonus) {
				PPSCardConfig sc_cfg;
				ScObj.FetchConfig(&sc_cfg);
				if(!(sc_cfg.Flags & sc_cfg.fDontUseBonusCards)) {
					if(!F(fRetCheck)) { // По бонусной карте в возврате бонус не учитываем (пока)
						const  PPID bonus_goods_grp_id  = scs_rec.BonusGrpID;
						const  PPID bonus_charge_grp_id = scs_rec.BonusChrgGrpID;
						if(pPack) {
							for(i = 0; i < pPack->GetCount(); i++) {
								charge += CalcSCardOpBonusAmount(pPack->GetLineC(i), bonus_goods_grp_id, &nca);
								non_crd_amt += nca;
								bonus_charge_amt += CalcSCardOpBonusAmount(pPack->GetLineC(i), bonus_charge_grp_id, 0);
							}
							if(pExtPack)
								for(i = 0; i < pExtPack->GetCount(); i++) {
									charge += CalcSCardOpBonusAmount(pExtPack->GetLineC(i), bonus_goods_grp_id, &nca);
									non_crd_amt += nca;
									bonus_charge_amt += CalcSCardOpBonusAmount(pExtPack->GetLineC(i), bonus_charge_grp_id, 0);
								}
						}
						else {
							for(i = 0; P.enumItems(&i, (void **)&p_item);) {
								p_item->GetRec(ccl_rec, IsCurrentOp(CCOP_RETURN));
								charge += CalcSCardOpBonusAmount(ccl_rec, bonus_goods_grp_id, &nca);
								non_crd_amt += nca;
								bonus_charge_amt += CalcSCardOpBonusAmount(ccl_rec, bonus_charge_grp_id, 0);
							}
						}
						if(pCurItem) {
							pCurItem->GetRec(ccl_rec, IsCurrentOp(CCOP_RETURN));
							charge += CalcSCardOpBonusAmount(ccl_rec, bonus_goods_grp_id, &nca);
							non_crd_amt += nca;
							bonus_charge_amt += CalcSCardOpBonusAmount(ccl_rec, bonus_charge_grp_id, 0);
						}
					}
				}
			}
			else if(scs_rec.GetType() == scstCredit) {
				const  PPID crd_goods_grp_id = scs_rec.CrdGoodsGrpID;
				const  PPID charge_goods_id = GetChargeGoodsID(scard_id);
				if(charge_goods_id != UNDEF_CHARGEGOODSID || crd_goods_grp_id) {
					if(pPack) {
						for(i = 0; i < pPack->GetCount(); i++) {
							charge += CalcSCardOpAmount(pPack->GetLineC(i), charge_goods_id, crd_goods_grp_id, &nca);
							non_crd_amt += nca;
						}
						if(pExtPack) {
							for(i = 0; i < pExtPack->GetCount(); i++) {
								charge += CalcSCardOpAmount(pExtPack->GetLineC(i), charge_goods_id, crd_goods_grp_id, &nca);
								non_crd_amt += nca;
							}
						}
					}
					else {
						for(i = 0; P.enumItems(&i, (void **)&p_item);) {
							p_item->GetRec(ccl_rec, IsCurrentOp(CCOP_RETURN));
							charge += CalcSCardOpAmount(ccl_rec, charge_goods_id, crd_goods_grp_id, &nca);
							non_crd_amt += nca;
						}
					}
					if(pCurItem) {
						pCurItem->GetRec(ccl_rec, IsCurrentOp(CCOP_RETURN));
						charge += CalcSCardOpAmount(ccl_rec, charge_goods_id, crd_goods_grp_id, &nca);
						non_crd_amt += nca;
					}
				}
				else if(pPack) {
					charge -= MONEYTOLDBL(pPack->Rec.Amount);
					if(pExtPack)
						charge -= MONEYTOLDBL(pExtPack->Rec.Amount);
				}
				else {
					for(i = 0; P.enumItems(&i, (void **)&p_item);)
						charge -= p_item->GetAmount();
					if(pCurItem)
						charge -= pCurItem->GetAmount();
				}
			}
		}
	}
	ASSIGN_PTR(pNonCrdAmt, non_crd_amt);
	ASSIGN_PTR(pBonusChargeAmt, bonus_charge_amt);
	return charge;
}

int CPosProcessor::Helper_InitCcPacket(CCheckPacket * pPack, CCheckPacket * pExtPack, const CcAmountList * pCcPl, long options)
{
	int    ok = 1;
	{
		long   sf = 0;
		if(IsCurrentOp(CCOP_RETURN)) {
			sf = CCHKF_RETURN;
			pPack->Ext.Op_ = GetCurrentOp(); // @v12.2.11
		}
		else if(IsCurrentOpCorrection()) {
			sf = CCHKF_CORRECTION;
			pPack->Ext.Op_ = GetCurrentOp(); // @v12.2.11
		}
		if(sf) {
			if(pPack)
				pPack->Rec.Flags |= sf;
			if(pExtPack)
				pExtPack->Rec.Flags |= sf;
		}
	}
	const CcTotal cct_debug = CalcTotal(); // @debug
	CcTotal cct;
	if(options & iccpDontFillLines) {
		cct = CalcTotal();
	}
	else {
		const bool to_fill_ext_pack = (!(pPack->Rec.Flags & CCHKF_SUSPENDED) && pExtPack && ExtCashNodeID);
		int   has_gift = 0;
		uint  i;
		CCheckItem * p_item = 0;
		CCheckItemArray to_fill_items;
		if(/*CnFlags & CASHF_UNIFYGDSATCHECK*/false) { // @v12.2.12 Блокируем возможность объединения строк чека из-за возможных проблем с макрировкой строк
			for(i = 0; P.enumItems(&i, (void **)&p_item);) {
				bool to_insert = true;
				for(uint p = 0; to_fill_items.lsearch(&p_item->GoodsID, &p, CMPF_LONG); p++) {
					CCheckItem & r_dest_item = to_fill_items.at(p);
					if(p_item->CanMerge(pPack, r_dest_item)) {
						r_dest_item.Quantity += p_item->Quantity;
						to_insert = false;
						break;
					}
				}
				if(to_insert)
					to_fill_items.insert(p_item);
			}
		}
		else
			to_fill_items = static_cast<CCheckItemArray &>(P);
		{
			// @v12.1.3 Строки, содержащие разливное пиво с упрощенным порядком учета в chzn сдвигаем вниз чека
			// это связано с проблемой регистрации чека в кассовом регистраторе, если в чеке после таких строк есть строки
			// с обычными марками chzn
			do {
				uint last_chzn_idx = 0; // [1..] последняя позиция с "нормальной" маркой chzn
				uint first_sdb_idx = 0; // [1..] первая позиция с упрощенным разливным пивом
				for(i = 0; i < to_fill_items.getCount(); i++) {
					CCheckItem & r_item = to_fill_items.at(i);
					const bool is_simplified_draftbeer = PPSyncCashSession::IsSimplifiedDraftBeerPosition(PNP.NodeID, r_item.GoodsID);
					if(is_simplified_draftbeer) {
						if(!first_sdb_idx)
							first_sdb_idx = i+1;
					}
					else if(r_item.ChZnMark[0]) {
						last_chzn_idx = i+1;
					}
				}
				if(first_sdb_idx && first_sdb_idx < last_chzn_idx) {
					to_fill_items.moveItemTo(first_sdb_idx-1, last_chzn_idx-1);
				}
				else
					break; // exit-loop
			} while(true);
		}
		for(i = 0; to_fill_items.enumItems(&i, (void **)&p_item);) {
			CCheckPacket * p_pack = (to_fill_ext_pack && BelongToExtCashNode(p_item->GoodsID)) ? pExtPack : pPack;
			if(p_item->Flags & cifUsedByGift)
				has_gift = 1;
			{
				short  division = p_item->Division;
				if(!division && P_DivGrpList) {
					int    use_default_div = 1;
					short  default_div = 0;
					PPGenCashNode::DivGrpAssc * p_dg_item;
					for(uint p = 0; !division && P_DivGrpList->enumItems(&p, (void **)&p_dg_item);) {
						if(p_dg_item->GrpID == 0)
							default_div = p_dg_item->DivN;
						else if(GObj.BelongToGroup(p_item->GoodsID, p_dg_item->GrpID, 0) > 0) {
							division = p_dg_item->DivN;
							use_default_div = 0;
						}
					}
					if(use_default_div)
						division = default_div;
					p_item->Division = division;
				}
			}
			THROW(p_pack->InsertCcl(*p_item));
		}
		SETFLAG(pPack->Rec.Flags, CCHKF_HASGIFT, has_gift);
		pPack->CalcAmount(&cct.Amount, &cct.Discount);
	}
	LDBLTOMONEY(cct.Amount, pPack->Rec.Amount);
	LDBLTOMONEY(cct.Discount, pPack->Rec.Discount);
	// @v11.9.0 P.SetupCCheckPacket(pPack, CSt, false);
	// @v11.9.0 {
	if(pExtPack && pExtPack->GetCount() && pPack->GetCount() == 0) {
		P.SetupCCheckPacket(pExtPack, CSt, false); // isExtCc == false ибо в данном случае спаренный чек ведет себя как основной чек (основного - просто нет)
		pExtPack->SetupPaymList(pCcPl);
		ok = 2; // @v11.9.0
	}	
	// } @v11.9.0 
	else {
		P.SetupCCheckPacket(pPack, CSt, false);
		pPack->SetupPaymList(pCcPl);
	}
	// @v11.0.9 {
	// @todo Здесь надо в пакете чека сохранить информацию о ручной скидке.
	// Это - важно, иначе не останется следа от того факта, что кассир предоставил произвольную скидку
	/*if(ManDis.Discount > 0.0) {
		if(ManDis.Flags & ManDis.fPct) {
		}
		else {
		}
	}*/
	// } @v11.0.9 
	if(options & iccpSetCurTime) {
		const LDATETIME now_dtm = getcurdatetime_();
		pPack->Rec.Dt = now_dtm.d;
		pPack->Rec.Tm = now_dtm.t;
		if(pExtPack) {
			pExtPack->Rec.Dt = now_dtm.d;
			pExtPack->Rec.Tm = now_dtm.t;
		}
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::AcceptCheckToBeCleared()
{
	int    ok = 1;
	CCheckPacket pack;
	GetNewCheckCode(PNP.NodeID, &pack.Rec.Code);
	pack.Rec.SessID = 0;
	pack.Rec.PosNodeID = 0;
	pack.Rec.Flags |= (CCHKF_SYNC|CCHKF_NOTUSED|CCHKF_SKIP);
	THROW(Helper_InitCcPacket(&pack, 0, 0, iccpSetCurTime));
	{
		PPTransaction tra(1);
		THROW(tra);
		if(SuspCheckID) {
			pack.Rec.ID = SuspCheckID;
			THROW(GetCc().UpdateCheck(&pack, 0));
			SuspCheckID = 0;
		}
		else {
			THROW(GetCc().TurnCheck(&pack, 0));
		}
		DS.LogAction(PPACN_CLEARCHECK, PPOBJ_CCHECK, pack.Rec.ID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

int CPosProcessor::AutosaveCheck()
{
	PPID   cc_id = 0;
	return (CsObj.GetEqCfg().Flags & PPEquipConfig::fAutosaveSyncChecks) ? AcceptCheck(&cc_id, 0, 0, 0.0, accmJunk) : -1;
}

int CPosProcessor::TurnCorrectionStorno(PPID * pCcID, int ccOp /*CCOP_XXX*/, PPID ccToStornoID) // @v12.2.12
{
	int    ok = -1;
	Reference * p_ref(PPRef);
	CCheckCore & r_cc = GetCc();
	assert(oneof2(ccOp, CCOP_CORRECTION_SELLSTORNO, CCOP_CORRECTION_RETSTORNO));
	if(oneof2(ccOp, CCOP_CORRECTION_SELLSTORNO, CCOP_CORRECTION_RETSTORNO)) {
		const  LDATETIME now_dtm = getcurdatetime_();
		SString temp_buf;
		CCheckPacket pack;
		if(GetCc().LoadPacket(ccToStornoID, 0, &pack) > 0) {
			SString fiscal_tag;
			if(pack.GetExtStrData(CCheckPacket::extssFiscalSign, fiscal_tag) > 0 && fiscal_tag.NotEmpty()) {
				AcceptCheckProcessBlock blk; 
				//CCheckPacket pack_storno(pack);
				blk.Pack = pack;
				blk.Flags |= AcceptCheckProcessBlock::fIsPack;
				blk.Pack.PutExtStrData(CCheckPacket::extssFiscalSign, 0);
				blk.Pack.Ext.LinkCheckID = pack.Rec.ID;
				blk.Pack.Ext.Op_ = ccOp;
				blk.Pack.Rec.ID = 0;
				blk.Pack.Rec.Flags |= (CCHKF_CORRECTION|CCHKF_EXT);
				blk.Pack.Rec.Flags &= ~CCHKF_PRINTED;
				blk.Pack.Rec.Dt = now_dtm.d;
				blk.Pack.Rec.Tm = now_dtm.t;
				GetNewCheckCode(PNP.NodeID, &blk.Pack.Rec.Code);
				//
				const long org_code = blk.Pack.Rec.Code;
				const long org_flags = blk.Pack.Rec.Flags;
				const long org_sess_id = blk.Pack.Rec.SessID;
				SString org_cctext;
				SString org_ext_cctext;
				SString new_cctext;
				blk.Pack.PackTextExt(org_cctext);
				blk.ExtPack.PackTextExt(org_ext_cctext);
				//
				if(r_cc.TurnCheck(&blk.Pack, 1)) {
					if(Implement_AcceptCheckOnEquipment(0, blk)) {
						{
							//
							// При проведении чека перед печатью может возникнуть ситуация, когда
							// печать изменила некоторые поля чека. В этом случае необходимо в БД изменить значения этих полей.
							//
							PPTransaction tra(1);
							THROW(tra);
							if(org_code != blk.Pack.Rec.Code || org_flags != blk.Pack.Rec.Flags || org_sess_id != blk.Pack.Rec.SessID) {
								THROW_DB(updateFor(&r_cc, 0, r_cc.ID == blk.Pack.Rec.ID,
									set(r_cc.Code, dbconst(blk.Pack.Rec.Code)).set(r_cc.Flags, dbconst(blk.Pack.Rec.Flags)).set(r_cc.SessID, dbconst(blk.Pack.Rec.SessID))));
							}
							{
								blk.Pack.PackTextExt(new_cctext);
								if(new_cctext != org_cctext) {
									THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_CCHECK, blk.Pack.Rec.ID, PPTRPROP_CC_LNEXT), new_cctext.Transf(CTRANSF_INNER_TO_UTF8), 0));
								}
							}
							THROW(tra.Commit());
						}
						ok = 1;
					}
					else {
						ok = 0;
					}
				}
			}
			else {
				// Не провести сторно-коррекцию чека, у которого нет фискального тега
			}
		}
	}
	CATCHZOK
	return ok;
}

void _CorrectProblem_v12304()
{
	TDialog * dlg = new TDialog(DLG_CORRECT12304);
	DateRange period;
	PPID   pos_node_id = 0;
	enum {
		fTestMode = 0x0001
	};
	long   flags = fTestMode;
	period.Set(encodedate(1, 5, 2025), ZERODATE);
	if(CheckDialogPtrErr(&dlg)) {
		bool    do_process = false;
		SetupPPObjCombo(dlg, CTLSEL_CORRECT12304_POSNODE, PPOBJ_CASHNODE, pos_node_id, 0);
		SetPeriodInput(dlg, CTL_CORRECT12304_PERIOD, period);
		dlg->AddClusterAssoc(CTL_CORRECT12304_FLAGS, 0, fTestMode);
		dlg->SetClusterData(CTL_CORRECT12304_FLAGS, flags);
		while(!do_process && ExecView(dlg) == cmOK) {
			uint   sel = 0;
			dlg->getCtrlData(sel = CTLSEL_CORRECT12304_POSNODE, &pos_node_id);
			if(pos_node_id) {
				GetPeriodInput(dlg, CTL_CORRECT12304_PERIOD, &period);
				dlg->GetClusterData(CTL_CORRECT12304_FLAGS, &flags);
				do_process = true;
			}
			else {
				PPErrorByDialog(dlg, sel, PPERR_CASHNODENEEDED);
			}
		}
		if(do_process) {
			assert(pos_node_id);
			CPosProcessor p(pos_node_id, 0, 0, 0, 0);
			period.Actualize(ZERODATE);
			p.CorrectProblem_v12304(&period, LOGIC(flags & fTestMode));
		}
	}
	delete dlg;
}

int CPosProcessor::CorrectProblem_v12304(const DateRange * pPeriod, bool testMode) // @v12.4.10 // @construction
{
	// Дата возникновения проблемы май-июнь 2025
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString msg_buf;
	//const char * p_log_file_name = "CorrectProblem_v12304.log";
	SString log_file_name;
	PPGetFilePath(PPPATH_LOG, "CorrectProblem_v12304.log", log_file_name);
	PPWait(1);
	if(P_EgMas) {
		PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor();
		if(p_eg_prc) {
			DateRange period;
			const LDATE __start_problem_date = encodedate(1, 5, 2025);
			if(pPeriod) {
				if(checkdate(pPeriod->low) && pPeriod->low >= __start_problem_date)
					period.low = pPeriod->low;
				else
					period.low = __start_problem_date;
				if(!checkdate(pPeriod->upp) || pPeriod->upp < period.low)
					period.upp = ZERODATE;
				else
					period.upp = pPeriod->upp;
			}
			else {
				period.Set(__start_problem_date, ZERODATE);
			}
			const LDATE start_date = period.low;
			CCheckCore & r_cc = GetCc();
			PPIDArray shadow_cc_id_list;
			LAssocArray processed_shadow_code_list; // Список теневых чеков, которые уже были созданы для решения проблемы. Key - ID, Val - Code
			LAssocArray egais_marked_code_list; // @v12.5.0 Список теневых чеков, у которых установлен признак того, что они уже отправлены в ЕГАИС
			LAssocArray target_cc_id_list; // key - main_cc_id, val - shadow_cc_id
			TSVector <CCheckTbl::Rec> temp_cc_rec_list;
			CCheckPacket cc_pack;
			CCheckTbl::Key2 k2;
			MEMSZERO(k2);
			k2.PosNodeID = PPPOSN_SHADOW;
			/*
				p_ref->UtrC.SearchUtf8(TextRefIdent(PPOBJ_CCHECK, r_rec.ID, PPTRPROP_CC_LNEXT), temp_buf);
				temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				CCheckPacket::Helper_UnpackTextExt(temp_buf, &sc, &cc_fld_list);
				if(sc.GetExtStrData(CCheckPacket::extssFiscalSign, temp_buf) > 0) {
				}
			*/ 
			if(r_cc.search(2, &k2, spGe) && r_cc.data.PosNodeID == PPPOSN_SHADOW) do {
				PPExtStrContainer sc;
				StrAssocArray cc_fld_list;
				if(r_cc.data.PosNodeID == PPPOSN_SHADOW) {
					const PPID cc_id = r_cc.data.ID;
					const long cc_code = r_cc.data.Code;
					const bool _is_in_period = period.CheckDate(r_cc.data.Dt);
					if(_is_in_period) {
						shadow_cc_id_list.add(cc_id);
					}
					{
						p_ref->UtrC.SearchUtf8(TextRefIdent(PPOBJ_CCHECK, cc_id, PPTRPROP_CC_LNEXT), temp_buf);
						CCheckPacket::Helper_UnpackTextExt(temp_buf, &sc, &cc_fld_list);
						if(sc.GetExtStrData(CCheckPacket::extssCorrect12304, temp_buf) > 0 && temp_buf.IsEqiAscii("true")) {
							processed_shadow_code_list.Add(cc_id, cc_code);
						}
						// @v12.5.0 {
						// ? extssEgaisUrl
						if(sc.GetExtStrData(CCheckPacket::extssSign, temp_buf) > 0 && temp_buf.NotEmpty()) {
							egais_marked_code_list.Add(cc_id, cc_code);
						}
						// } @v12.5.0 
					}
				}
			} while(r_cc.search(2, &k2, spNext) && r_cc.data.PosNodeID == PPPOSN_SHADOW);
			shadow_cc_id_list.sortAndUndup();
			uint   iter_idx = shadow_cc_id_list.getCount();
			if(iter_idx) {
				uint   shadow_cc_counter = 0;
				do {
					PPID   iter_cc_id = shadow_cc_id_list.at(--iter_idx);
					if(r_cc.LoadPacket(iter_cc_id, 0, &cc_pack) > 0 && cc_pack.GetCount()) {
						bool    all_zero = true;
						for(uint clidx = 0; clidx < cc_pack.GetCount(); clidx++) {
							const CCheckLineTbl::Rec & r_ccl = cc_pack.GetLineC(clidx);
							if(r_ccl.Quantity != 0.0) {
								all_zero = false;
							}
						}
						if(all_zero) {
							LDATETIME shadow_cc_dtm;
							shadow_cc_dtm.Set(cc_pack.Rec.Dt, cc_pack.Rec.Tm);
							temp_cc_rec_list.clear();
							r_cc.SearchByDateAndCode(cc_pack.Rec.Code, shadow_cc_dtm.d, false, &temp_cc_rec_list);
							PPID    main_cc_id = 0;
							bool    ambiguity = false;
							for(uint i = 0; i < temp_cc_rec_list.getCount(); i++) {
								const CCheckTbl::Rec & r_iter_cc_rec = temp_cc_rec_list.at(i);
								if(r_iter_cc_rec.ID != cc_pack.Rec.ID) {
									LDATETIME iter_cc_dtm;	
									iter_cc_dtm.Set(r_iter_cc_rec.Dt, r_iter_cc_rec.Tm);
									const long _s = abs(diffdatetimesec(iter_cc_dtm, shadow_cc_dtm));
									if(_s <= 60) {
										if(main_cc_id) {
											ambiguity = true;
											break;
										}
										else {
											main_cc_id = r_iter_cc_rec.ID;
										}
									}
								}
							}
							if(main_cc_id && !ambiguity) {
								target_cc_id_list.Add(main_cc_id, cc_pack.Rec.ID);
							}
						}
					}
				} while(iter_idx);
				if(target_cc_id_list.getCount()) {
					target_cc_id_list.SortByKeyVal();
					for(uint tli = 0; tli < target_cc_id_list.getCount(); tli++) {
						const PPID main_cc_id = target_cc_id_list.at(tli).Key;
						const PPID shadow_cc_id = target_cc_id_list.at(tli).Val;
						CCheckPacket main_cc_pack;
						if(r_cc.LoadPacket(main_cc_id, 0, &main_cc_pack) > 0) {
							uint   processed_shadow_code_pos = 0;
							// @v12.5.0 if(processed_shadow_code_list.SearchByVal(main_cc_pack.Rec.Code, &processed_shadow_code_pos)) {
							if(egais_marked_code_list.SearchByVal(main_cc_pack.Rec.Code, &processed_shadow_code_pos)) { // @v12.5.0
								// @v12.5.0 LAssoc pi = processed_shadow_code_list.at(processed_shadow_code_pos);
								LAssoc pi = egais_marked_code_list.at(processed_shadow_code_pos); // @v12.5.0 
								CCheckCore::MakeCodeString(&main_cc_pack.Rec, CCheckCore::mcsID, temp_buf);
								temp_buf.CatDiv(':', 2).Cat("corrected").CatParStr(pi.Key);
								PPLogMessage(log_file_name, temp_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
							}
							else {
								{
									CCheckCore::MakeCodeString(&main_cc_pack.Rec, CCheckCore::mcsID, temp_buf);
									PPLogMessage(log_file_name, temp_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
								}
								EgaisMarkAutoSelector::ResultBlock rb;
								PPGoodsType gt_rec;
								Goods2Tbl::Rec goods_rec;
								{
									for(uint i = 0; i < main_cc_pack.GetCount(); i++) {
										const CCheckLineTbl::Rec & r_ccl = main_cc_pack.GetLineC(i);
										P_EgMas->AddSourceItemToResultBlock(rb, i+1, r_ccl.GoodsID, r_ccl.Quantity);
									}
								}
								if(rb.getCount()) {
									{
										for(uint i = 0; i < rb.getCount(); i++) {
											const EgaisMarkAutoSelector::DocItem * p_src_item = rb.at(i);
											if(p_src_item) {
												msg_buf.Z();
												GetObjectName(PPOBJ_GOODS, p_src_item->GoodsID, temp_buf);
												msg_buf.Tab().Cat(temp_buf).Space().CatEq("qtty", p_src_item->Qtty, MKSFMTD(0, 3, 0)).Space().
													CatEq("volume-qtty", p_src_item->VolumeQtty, MKSFMTD(0, 3, 0));
												PPLogMessage(log_file_name, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
											}
										}
									}
									int r = P_EgMas->Run(rb, /*main_cc_pack.Rec.Dt*/ZERODATE);
									if(r > 0) {
										{
											msg_buf.Z().Tab_(2).Cat("EgaisMarkAutoSelector").Space().Cat("process completed successfully");
											PPLogMessage(log_file_name, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
										}
										CCheckPacket cc_shadow_egais;
										if(P_EgMas->MakeShadowCcPacket(&rb, main_cc_pack, cc_shadow_egais) > 0) {
											{
												for(uint i = 0; i < cc_shadow_egais.GetCount(); i++) {
													const CCheckLineTbl::Rec & r_ccl = cc_shadow_egais.GetLineC(i);
													//
													msg_buf.Z();
													GetObjectName(PPOBJ_GOODS, r_ccl.GoodsID, temp_buf);
													msg_buf.Tab_(2).Cat(temp_buf).Space().CatEq("qtty", r_ccl.Quantity, MKSFMTD(0, 3, 0));
													PPLogMessage(log_file_name, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
												}
											}
											if(!testMode) {
												const  LDATETIME now_dtm = getcurdatetime_();
												cc_shadow_egais.Rec.Dt = now_dtm.d;
												cc_shadow_egais.Rec.Tm = now_dtm.t;
												cc_shadow_egais.PutExtStrData(CCheckPacket::extssCorrect12304, "true");
												r_cc.AdjustRecTime(cc_shadow_egais.Rec);
												const int tsemapr = TurnShadowEgaisMarkAutoselectionCcPacket(main_cc_pack, cc_shadow_egais, 1);
												if(!tsemapr) {
													PPLogMessage(log_file_name, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_DBINFO);
												}
											}
										}
									}
									else {
										msg_buf.Z().Tab_(2).Cat("EgaisMarkAutoSelector").Space().Cat("process failed");
										PPLogMessage(log_file_name, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
									}
								}
							}
						}
						PPWaitPercent(tli+1, target_cc_id_list.getCount());
					}
				}
			}
		}
	}
	PPWait(0);
	return ok;
}

int CPosProcessor::TurnShadowEgaisMarkAutoselectionCcPacket(const CCheckPacket & rMainCcPack, CCheckPacket & rCcPack, int use_ta) // @v12.4.10 @construction
{
	int   ok = -1;
	CCheckCore & r_cc = GetCc();
	PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor(); // @v12.2.11
	SString fmt_buf;
	SString msg_buf;
	int    eg_prc_pccr = 0; // Результат отправки чека в егаис (утм)
	{
		//
		// Если мы проводим аварийный исправительный чек (CorrectProblem_v12304), то можем нарушить уникальность идекса 2 {PosNodeID, Code, Dt, Tm}.
		// На этот случай сделаем поправку времени.
		//
		CCheckTbl::Key2 k2;
		ulong hs_incr = 0;
		k2.PosNodeID = rCcPack.Rec.PosNodeID;
		k2.Code = rCcPack.Rec.Code;
		k2.Dt = rCcPack.Rec.Dt;
		k2.Tm = rCcPack.Rec.Tm;
		if(r_cc.search(2, &k2, spEq)) do {
			k2.PosNodeID = rCcPack.Rec.PosNodeID;
			k2.Code = rCcPack.Rec.Code;
			k2.Dt = rCcPack.Rec.Dt;
			k2.Tm.v = rCcPack.Rec.Tm.v + (++hs_incr);
		} while(r_cc.search(2, &k2, spEq));
		if(hs_incr)
			rCcPack.Rec.Tm.v += hs_incr;
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r_cc.TurnCheck(&rCcPack, 0));
		{
			// Для проведения чека через егаис нам нужен реальный кассовый узел, а не фейковый, каковым является PPPOSN_SHADOW
			bool   local_do_update = false;
			PPEgaisProcessor::Ack eg_ack;
			const  PPID preserve_cn_id = rCcPack.Rec.PosNodeID;
			rCcPack.Rec.PosNodeID = rMainCcPack.Rec.PosNodeID;
			rCcPack.Rec.SessID = rMainCcPack.Rec.SessID; // @v12.1.2
			PPLogMessage(PPFILNAM_DEBUG_LOG, "P_EgPrc->PutCCheck shadow autoselection", LOGMSGF_DIRECTOUTP);
			{
				// @v12.2.11 {
				assert(p_eg_prc);
				if(p_eg_prc) {
					const bool org_eg_test_mode = p_eg_prc->GetTestSendingMode();
					p_eg_prc->SetTestSendingMode((PNP.EgaisMode == 2));
					eg_prc_pccr = p_eg_prc->PutCCheck(rCcPack, PNP.CnLocID, true/*horecaAutoWo*/, eg_ack);
					p_eg_prc->SetTestSendingMode(org_eg_test_mode);
					// @v12.3.4 THROW(eg_prc_pccr);
					// @v12.3.4 {
					if(!eg_prc_pccr) {
						PPGetLastErrorMessage(1/*rmvSpcChrs*/, msg_buf);
						PPLoadText(PPTXT_ERR_EGAIS_PUTCCSHADOW, fmt_buf);
						fmt_buf.CatDiv(':', 2).Cat(msg_buf);
						PPLogMessage(PPFILNAM_ERR_LOG, fmt_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					// } @v12.3.4 
				}
				// } @v12.2.11 
				// @v12.2.11 THROW(P_EgPrc_ToEliminate->PutCCheck(cc_shadow_egais, CnLocID, true/*horecaAutoWo*/, eg_ack));
			}
			rCcPack.Rec.PosNodeID = preserve_cn_id;
			if(eg_prc_pccr) {
				if(eg_ack.Sign[0] && eg_ack.SignSize) {
					msg_buf.Z().CatN(reinterpret_cast<const char *>(eg_ack.Sign), eg_ack.SignSize);
					rCcPack.PutExtStrData(CCheckPacket::extssSign, msg_buf);
					local_do_update = true;
				}
				if(eg_ack.Url.NotEmpty()) {
					rCcPack.PutExtStrData(CCheckPacket::extssEgaisUrl, eg_ack.Url);
					local_do_update = true;
				}
			}
			if(local_do_update) {
				THROW(r_cc.UpdateCheck(&rCcPack, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
		ok = 0;
	ENDCATCH
	return ok;
}

/*virtual*/int CPosProcessor::AcceptCheck(PPID * pCcID, const CcAmountList * pPl, PPID altPosNodeID, double cash, int mode /* accmXXX */)
{
	int    ok = 1;
	Reference * p_ref(PPRef);
	const  bool turn_check_before_printing = true;
	const  bool reprint_regular = (mode == accmAveragePrinting && Flags & fReprinting);
	int    was_turned_before_printing = 0;
	SString before_printing_check_text;
	SString msg_buf;
	SString fmt_buf;
	PPEgaisProcessor * p_eg_prc = 0; // @v12.2.11
	THROW_INVARG((mode != accmAveragePrinting || reprint_regular) || P_ChkPack);
	if(PNP.NodeID) {
		AcceptCheckProcessBlock epb;
		if(!(Flags & fNoEdit)) {
			THROW_PP(!(PNP.CnFlags & CASHF_DISABLEZEROAGENT) || P.GetAgentID(), PPERR_CHKPAN_SALERNEEDED);
			THROW_PP(!(PNP.CnExtFlags & CASHFX_DISABLEZEROSCARD) || CSt.GetID(), PPERR_CHKPAN_SCARDNEEDED);
		}
		THROW(InitCashMachine());
		if(mode == accmAveragePrinting) {
			THROW_PP(OperRightsFlags & orfPrintCheck, PPERR_NORIGHTS);
			if(reprint_regular) {
				epb.Pack = SelPack;
				//epb.Pack.Rec.Flags |= CCHKF_PRINTED;
			}
			else
				epb.Pack = *P_ChkPack;
		}
		else {
			// @v12.2.9 {
			if(IsCurrentOpCorrection()) {
				THROW_PP(OperRightsFlags & orfReturns, PPERR_NORIGHTS); // @todo Возможно, здесь нужен новый флаг прав доступа
				epb.Pack.Rec.Flags |= CCHKF_CORRECTION;
			}
			else { // } @v12.2.9 
				if(IsCurrentOp(CCOP_RETURN)) {
					THROW_PP(OperRightsFlags & orfReturns, PPERR_NORIGHTS);
					epb.Pack.Rec.Flags |= CCHKF_RETURN;
				}
			}
			if(pPl && pPl->Get(CCAMTTYP_BANK) != 0.0) {
				THROW_PP(OperRightsFlags & orfBanking, PPERR_NORIGHTS);
				epb.Pack.Rec.Flags |= CCHKF_BANKING;
			}
			if(SuspCheckID && GetCc().Search(SuspCheckID, &epb.LastChkRec) > 0) {
				// @debug {
				if((epb.LastChkRec.Flags & (CCHKF_JUNK|CCHKF_SUSPENDED)) != (CCHKF_JUNK|CCHKF_SUSPENDED) && !reprint_regular) {
					PPSetError(PPERR_CCHKMUSTBEJUNK, CCheckCore::MakeCodeString(&epb.LastChkRec, 0, msg_buf));
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				}
				// } @debug
				epb.Pack.Rec.Code = epb.LastChkRec.Code;
				if(reprint_regular) {
					epb.Pack.Rec.Dt = epb.LastChkRec.Dt;
					epb.Pack.Rec.Tm = epb.LastChkRec.Tm;
				}
				else {
					//
					// Следующие 2 строки следует раскомментировать, если дата и время отложенного чека должны
					// соответствовать последнему изменению (в противном случае они будут соответствовать времени создания чека).
					//
					//pack.Rec.Dt = last_chk_rec.Dt;
					//pack.Rec.Tm = last_chk_rec.Tm;
				}
			}
			else
				GetNewCheckCode(PNP.NodeID, &epb.Pack.Rec.Code);
			if(mode == accmSuspended) {
				THROW_PP(OuterOi.Id == 0, PPERR_UNABLESUSPTSESSCHECK);
				//
				// Перед тем, как сохранить отложенный чек, удаляем строки подарков.
				// Делается это потому, что при восстановлении могут измениться условия начисления //
				// подарков, кроме того, у нас нет механизма сохранения признака cifGift в строке чека.
				// При восстановлении подарки будут начислены снова.
				//
				P.ClearGift();
			}
			else if(mode != accmJunk) {
				THROW_PP(OperRightsFlags & orfPrintCheck, PPERR_NORIGHTS);
			}
			epb.Pack.Rec.SessID = P_CM->GetCurSessID();
			epb.Pack.Rec.PosNodeID = PNP.NodeID;
			epb.Pack.Rec.Flags |= (CCHKF_SYNC|CCHKF_NOTUSED);
			SETFLAG(epb.Pack.Rec.Flags, CCHKF_INCORPCRD, CSt.GetID() && Flags & fSCardCredit);
			epb.Pack.Rec.Flags &= ~CCHKF_BONUSCARD;
			SETFLAG(epb.Pack.Rec.Flags, CCHKF_SUSPENDED|CCHKF_SKIP, oneof2(mode, accmSuspended, accmJunk));
			SETFLAG(epb.Pack.Rec.Flags, CCHKF_JUNK, mode == accmJunk);
			SETFLAG(epb.Pack.Rec.Flags, CCHKF_PREPRINT, Flags & fPrinted);
			//
			// Перед окончательным проведением чека необходимо распределить подарочную скидку (если она есть) по строкам чека.
			//
			if(mode == accmRegular) {
				for(uint i = 0; i < P.getCount(); i++) {
					if(P.at(i).Flags & cifGiftDiscount) {
						SetupDiscount(1);
						break;
					}
				}
			}
			const int iccpr = Helper_InitCcPacket(&epb.Pack, (((mode == accmRegular || reprint_regular) && !altPosNodeID) ? &epb.ExtPack : 0), pPl, 0);
			THROW(iccpr);
			if(mode == accmRegular && P_CM_EXT) {
				epb.Pack._Cash = MONEYTOLDBL(epb.Pack.Rec.Amount);
				SETFLAG(epb.Flags, epb.fIsExtPack, epb.ExtPack.GetCount());
				if(epb.Flags & epb.fIsExtPack) {
					double amt;
					double dscnt;
					const  long preserve_ext_pack_flags = epb.ExtPack.Rec.Flags;
					GetNewCheckCode(ExtCashNodeID, &epb.ExtPack.Rec.Code);
					epb.ExtPack.Rec.SessID = P_CM_EXT->GetCurSessID(); // @!
					epb.ExtPack.Rec.PosNodeID = ExtCashNodeID;
					epb.ExtPack.Rec.Flags  = epb.Pack.Rec.Flags;
					epb.ExtPack.SetupAmount(&amt, &dscnt);
					if(iccpr == 2) {
						SETFLAGBYSAMPLE(epb.ExtPack.Rec.Flags, preserve_ext_pack_flags, (CCHKF_PAYMLIST|CCHKF_BANKING|CCHKF_RETURN)); // @v11.9.1 @fix (CCHKF_BANKING|CCHKF_RETURN)
					}
					else {
						P.SetupCCheckPacket(&epb.ExtPack, CSt, true);
					}
					epb.ExtPack._Cash = amt;
				}
			}
			else {
				epb.Pack._Cash = cash;
			}
		}
		SETFLAG(epb.Flags, epb.fIsPack, epb.Pack.GetCount());
		if(mode == accmJunk) {
			THROW(StoreCheck(&epb.Pack, 0, mode));
		}
		else {
			const long org_code = epb.Pack.Rec.Code;
			const long org_flags = epb.Pack.Rec.Flags;
			const long org_sess_id = epb.Pack.Rec.SessID;
			const long org_ext_code = epb.ExtPack.Rec.Code;
			const long org_ext_flags = epb.ExtPack.Rec.Flags;
			const long org_ext_sess_id = epb.ExtPack.Rec.SessID;
			SString org_cctext;
			SString org_ext_cctext;
			epb.Pack.PackTextExt(org_cctext);
			epb.ExtPack.PackTextExt(org_ext_cctext);
			bool   dont_accept_ccode_from_printer = false;
			if(mode == accmRegular) {
				if(oneof2(PNP.EgaisMode, 1, 2)) {
					p_eg_prc = DS.GetTLA().GetEgaisProcessor(); // @v12.2.11
					// @v12.0.12 {
					if(p_eg_prc && P_EgMas) {
						EgaisMarkAutoSelector::ResultBlock rb;
						EgaisMarkAutoSelector::ResultBlock rb_server_reply;
						EgaisMarkAutoSelector::ResultBlock * p_rb_result = 0; // В зависимости от того локально мы вызывали обработку или посредством сервера
							// этот указатель может ссылаться либо на rb либо на rb_server_reply
						PPGoodsType gt_rec;
						Goods2Tbl::Rec goods_rec;
						{
							for(uint i = 0; i < P.getCount(); i++) {
								const CCheckItem & r_cc_item = P.at(i);
								P_EgMas->AddSourceItemToResultBlock(rb, i+1, r_cc_item.GoodsID, r_cc_item.Quantity);
							}
						}
						if(rb.getCount()) {
							// @v12.2.11
							PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
							if(p_cli) {
								PROFILE_START
								{
									PPUserFuncProfiler ufp(PPUPRF_EGAISMARKAUTOSELECTOR_SVR);
									PPJobSrvCmd cmd;
									PPJobSrvReply reply;
									if(cmd.StartWriting(PPSCMD_EGAISMARKAUTOSELECTION)) {
										SSerializeContext sctx;
										if(rb.Serialize(+1, cmd, &sctx)) {
											cmd.FinishWriting();
											if(p_cli->ExecSrvCmd(cmd, reply)) {
												reply.StartReading(0);
												if(reply.CheckRepError()) {
													int rsr = rb_server_reply.Serialize(-1, reply, &sctx);
													if(rsr)
														p_rb_result = &rb_server_reply;
												}
											}
										}
									}
									ufp.SetFactor(0, rb.getCount());
									ufp.SetFactor(1, rb_server_reply.GetTerminalEntryCount());
								}
								PROFILE_END
								(msg_buf = "EgaisMarkAutoSelector server call").CatDiv(':', 2).Cat(p_rb_result ? "succsess" : "failure");
								PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DIRECTOUTP); // @debug
							}
							// } @v12.2.11
							int r = 0;
							if(!p_rb_result) {
								PROFILE_START
								{
									PPUserFuncProfiler ufp(PPUPRF_EGAISMARKAUTOSELECTOR);
									r = P_EgMas->Run(rb, ZERODATE);
									ufp.SetFactor(0, rb.getCount());
									ufp.SetFactor(1, rb.GetTerminalEntryCount());
								}
								PROFILE_END
								p_rb_result = &rb;
							}
							else
								r = 1;
							assert(p_rb_result);
							if(r > 0) {
								CCheckPacket cc_shadow_egais;
								if(P_EgMas->MakeShadowCcPacket(p_rb_result, epb.Pack, cc_shadow_egais) > 0) {
									const int tsemapr = TurnShadowEgaisMarkAutoselectionCcPacket(epb.Pack, cc_shadow_egais, 1); // @v12.4.10
								}
							}
						}
					}
					// } @v12.0.12 
					if(PNP.EgaisMode == 1) {
						dont_accept_ccode_from_printer = true;
					}
					if(p_eg_prc) {
						//
						// Перед передачей в ЕГАИС необходимо предварительное проведение чеков
						// для того, что бы сформировались значения номеров чеков.
						// Однако, это не исключает коллизию, возникающую в случае, если после
						// успешного проведения через ЕГАИС фискальный регистратор при печати
						// присвоит чеку собственный номер. При этом получится так, что в БД и
						// фискальной памяти будут хранится не те номера, которые были переданы в ЕГАИС.
						//
						if(turn_check_before_printing && !was_turned_before_printing && (mode != accmAveragePrinting || reprint_regular)) {
							THROW(StoreCheck(&epb.Pack, (epb.Flags & epb.fIsExtPack) ? &epb.ExtPack : 0, mode));
							CCheckCore::MakeCodeString(&epb.Pack.Rec, 0, before_printing_check_text);
							was_turned_before_printing = 1;
						}
						{
							PPEgaisProcessor::Ack eg_ack;
							PPLogMessage(PPFILNAM_DEBUG_LOG, "P_EgPrc->PutCCheck before", LOGMSGF_DIRECTOUTP); // @debug
							{
								// @v12.2.11 {
								const bool org_eg_test_mode = p_eg_prc->GetTestSendingMode();
								p_eg_prc->SetTestSendingMode((PNP.EgaisMode == 2));
								const int pccr = p_eg_prc->PutCCheck(epb.Pack, PNP.CnLocID, false/*horecaAutoWo*/, eg_ack);
								p_eg_prc->SetTestSendingMode(org_eg_test_mode);
								THROW(pccr);
								// } @v12.2.11 
								// @v12.2.11 THROW(P_EgPrc_ToEliminate->PutCCheck(epb.Pack, CnLocID, false/*horecaAutoWo*/, eg_ack));
							}
							if(eg_ack.Sign[0] && eg_ack.SignSize) {
								msg_buf.Z().CatN(reinterpret_cast<const char *>(eg_ack.Sign), eg_ack.SignSize);
								epb.Pack.PutExtStrData(CCheckPacket::extssSign, msg_buf);
							}
							if(eg_ack.Url.NotEmpty()) {
								epb.Pack.PutExtStrData(CCheckPacket::extssEgaisUrl, eg_ack.Url);
							}
						}
						if(epb.Flags & epb.fIsExtPack) {
							PPEgaisProcessor::Ack eg_ack;
							{
								// @v12.2.11 {
								assert(p_eg_prc);
								if(p_eg_prc) {
									const bool org_eg_test_mode = p_eg_prc->GetTestSendingMode();
									p_eg_prc->SetTestSendingMode((PNP.EgaisMode == 2));
									const int pccr = p_eg_prc->PutCCheck(epb.ExtPack, PNP.ExtCnLocID, false/*horecaAutoWo*/, eg_ack);
									p_eg_prc->SetTestSendingMode(org_eg_test_mode);
									THROW(pccr);
								}
								// } @v12.2.11 
								// @v12.2.11 THROW(P_EgPrc_ToEliminate->PutCCheck(epb.ExtPack, ExtCnLocID, false/*horecaAutoWo*/, eg_ack));
							}
							if(eg_ack.Sign[0] && eg_ack.SignSize) {
								msg_buf.Z().CatN(reinterpret_cast<const char *>(eg_ack.Sign), eg_ack.SignSize);
								epb.ExtPack.PutExtStrData(CCheckPacket::extssSign, msg_buf);
							}
							if(eg_ack.Url.NotEmpty()) {
								epb.ExtPack.PutExtStrData(CCheckPacket::extssEgaisUrl, eg_ack.Url);
							}
						}
					}
				}
			}
			if(turn_check_before_printing && !was_turned_before_printing && (mode != accmAveragePrinting || reprint_regular)) {
				THROW(StoreCheck(&epb.Pack, (epb.Flags & epb.fIsExtPack) ? &epb.ExtPack : 0, mode));
				CCheckCore::MakeCodeString(&epb.Pack.Rec, 0, before_printing_check_text);
				was_turned_before_printing = 1;
			}
			if(mode != accmSuspended) {
				SETFLAG(epb.Flags, epb.fAltReg, altPosNodeID && P_CM_ALT);
				if(!Implement_AcceptCheckOnEquipment(pPl, epb))
					ok = 0;
			}
			if(dont_accept_ccode_from_printer) {
				epb.Pack.Rec.Code = org_code;
				if(epb.Flags & epb.fIsExtPack)
					epb.ExtPack.Rec.Code = org_ext_code;
			}
			{
				// @v9.5.10 {
				// По непонятным причинам Viki Print перестал возвращать ненулевой номер чека прежним образом.
				// Пока, до решения проблемы, будем замещать нулевой номер нашим собственным.
				SETIFZ(epb.Pack.Rec.Code, org_code);
				if(epb.Flags & epb.fIsExtPack) {
					SETIFZ(epb.ExtPack.Rec.Code, org_ext_code);
				}
				// } @v9.5.10
			}
			if(was_turned_before_printing) {
				//
				// При проведении чека перед печатью может возникнуть ситуация, когда
				// печать изменила некоторые поля чека. В этом случае необходимо в БД изменить значения этих полей.
				//
				CCheckCore & r_cc = GetCc();
				PPTransaction tra(1);
				THROW(tra);
				if(org_code != epb.Pack.Rec.Code || org_flags != epb.Pack.Rec.Flags || org_sess_id != epb.Pack.Rec.SessID) {
					THROW_DB(updateFor(&r_cc, 0, r_cc.ID == epb.Pack.Rec.ID,
						set(r_cc.Code, dbconst(epb.Pack.Rec.Code)).set(r_cc.Flags, dbconst(epb.Pack.Rec.Flags)).set(r_cc.SessID, dbconst(epb.Pack.Rec.SessID))));
				}
				if(org_ext_code != epb.ExtPack.Rec.Code || org_ext_flags != epb.ExtPack.Rec.Flags || org_ext_sess_id != epb.ExtPack.Rec.SessID) {
					THROW_DB(updateFor(&r_cc, 0, r_cc.ID == epb.ExtPack.Rec.ID,
						set(r_cc.Code, dbconst(epb.ExtPack.Rec.Code)).set(r_cc.Flags, dbconst(epb.ExtPack.Rec.Flags)).set(r_cc.SessID, dbconst(epb.ExtPack.Rec.SessID))));
				}
				{
					SString new_cctext;
					SString new_ext_cctext;
					epb.Pack.PackTextExt(new_cctext);
					epb.ExtPack.PackTextExt(new_ext_cctext);
					if(new_cctext != org_cctext) {
						THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_CCHECK, epb.Pack.Rec.ID, PPTRPROP_CC_LNEXT), new_cctext.Transf(CTRANSF_INNER_TO_UTF8), 0));
					}
					if(new_ext_cctext != org_ext_cctext) {
						THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_CCHECK, epb.ExtPack.Rec.ID, PPTRPROP_CC_LNEXT), new_ext_cctext.Transf(CTRANSF_INNER_TO_UTF8), 0));
					}
				}
				THROW(tra.Commit());
			}
			else if(mode != accmAveragePrinting || reprint_regular) {
				THROW(StoreCheck(&epb.Pack, (epb.Flags & epb.fIsExtPack) ? &epb.ExtPack : 0, mode));
			}
			//
			// На ошибки печати чека и др., возникшие в Implement_AcceptCheckOnEquipment(), реагируем уже после
			// завершения транзакций в базе данных.
			//
			if((epb.R == 0 && epb.SyncPrnErr != 3) || (epb.RExt == 0 && epb.ExtSyncPrnErr != 3)) // ???
				PPError();
			ASSIGN_PTR(pCcID, epb.Pack.Rec.ID); // @v11.4.6
		}
	}
	if(mode != accmJunk && (mode != accmAveragePrinting || reprint_regular))
		ClearCheck();
	CATCH
		ok = PPErrorZ();
		if(was_turned_before_printing) {
			ClearCheck();
			MessageError(PPERR_CHKPAN_CHKTURNEDBEFOREPRNERR, before_printing_check_text, eomPopup);
		}
	ENDCATCH
	return ok;
}

/*virtual*/void CPosProcessor::ClearCheck()
{
	if(SuspCheckID) {
		if(!GetCc().RemovePacket(SuspCheckID, 1))
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
		SuspCheckID = 0;
	}
	SetupAgent(0, 0);
	P.OrderCheckID = 0;
	P.Z();
	OuterOi.Z();
	if(oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF))
		SetupState(sEMPTYLIST_EMPTYBUF);
	ResetSCard();
	ManDis.Z(); // @v11.0.9
	Flags &= ~(fBankingPayment|fReprinting);
	OnUpdateList(0);
	SetPrintedFlag(0);
}

/*virtual*/void CPosProcessor::ClearRow()
{
	P.ClearCur();
	SetupRowData(false/*doCalcRest*/);
	SetupState(oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF) ? sLISTSEL_EMPTYBUF : (P.getCount() ? sLIST_EMPTYBUF : sEMPTYLIST_EMPTYBUF));
}

int CPosProcessor::Helper_SetupSessUuidForCheck(PPID checkID)
{
	int    ok = 1;
	if(checkID) {
		ObjTagItem tag_item;
		const S_GUID uuid(SessUUID.IsZero() ? SLS.GetSessUuid() : SessUUID);
		if(tag_item.SetGuid(PPTAG_CCHECK_JS_UUID, &uuid))
			PPRef->Ot.PutTag(PPOBJ_CCHECK, checkID, &tag_item, 0);
	}
	else
		ok = -1;
	return ok;
}

bool CPosProcessor::IsOnlyChargeGoodsInPacket(PPID scID, const CCheckPacket * pPack)
{
	bool   only_charge_goods = true;
	const  PPID charge_goods_id = GetChargeGoodsID(scID);
	if(pPack) {
		for(uint i = 0; only_charge_goods && i < pPack->GetCount(); i++) {
			const CCheckLineTbl::Rec & r_line = pPack->GetLineC(i);
			if(r_line.GoodsID != charge_goods_id)
				only_charge_goods = false;
		}
	}
	else {
		for(uint i = 0; only_charge_goods && i < P.getCount(); i++) {
			const CCheckItem & r_line = P.at(i);
			if(r_line.GoodsID != charge_goods_id)
				only_charge_goods = false;
		}
	}
	return only_charge_goods;
}

int CPosProcessor::StoreCheck(CCheckPacket * pPack, CCheckPacket * pExtPack, int mode)
{
 	int    ok = 1;
	CCheckCore & r_cc = GetCc();
 	int    _turn_done = 0; // Признак того, что была выполнена функция CCheckCore::TurnCheck или CCheckCore::UpdateCheck
	int    do_clear_junk_attrs = 0;
	SString temp_buf;
	SString uhtt_err_added_msg;
	PPCheckInPersonMngr cip_mgr;
	TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
	const LDATETIME now_dtm = getcurdatetime_();
	assert(pPack);
	THROW_INVARG(pPack != 0);
	const  PPID  preserve_csess_id = pPack->Rec.SessID;
	{
		PPTransaction tra(1);
		THROW(tra);
		double temp_val = 0.0;
		double non_crd_amt = 0.0;
		const double _charge = (mode != accmJunk) ? CalcCreditCharge(pPack, pExtPack, 0, &non_crd_amt, &temp_val) : 0.0;
		const double bonus_charge_amt = (mode != accmJunk && temp_val < 0.0) ? -temp_val : 0.0;
		if(pPack->GetCount()) {
			if(P.Eccd.Flags & P.Eccd.fDelivery) {
				pPack->Rec.Flags |= CCHKF_DELIVERY;
				pPack->Ext.StartOrdDtm = P.Eccd.DlvrDtm;
				if(P.Eccd.Addr_.Tail[0]) {
					if(!P.Eccd.Addr_.ID) {
						if(LocationCore::GetExFieldS(&P.Eccd.Addr_, LOCEXSTR_PHONE, temp_buf).NotEmptyS()) {
							//
							// Если у адреса есть телефон, то объявляем такой адрес автономным,
							// дабы при повторном обращении этого клиента можно было бы повторно использовать этот адрес.
							//
							P.Eccd.Addr_.Flags |= LOCF_STANDALONE;
						}
						SETIFZ(P.Eccd.Addr_.Type, LOCTYP_ADDRESS);
						THROW(PsnObj.LocObj.PutRecord(&P.Eccd.Addr_.ID, &P.Eccd.Addr_, 0));
					}
					pPack->Ext.AddrID = P.Eccd.Addr_.ID;
				}
			}
			SETFLAG(pPack->Rec.Flags, CCHKF_FIXEDPRICE, P.Eccd.Flags & P.Eccd.fFixedPrice);
			SETFLAG(pPack->Rec.Flags, CCHKF_SPFINISHED, P.Eccd.Flags & P.Eccd.fSpFinished);
			SETFLAG(pPack->Rec.Flags, CCHKF_IMPORTED, P.Eccd.Flags & P.Eccd.fImported); // @v11.8.11
			pPack->Ext.CreationDtm = P.Eccd.InitDtm;
			pPack->Ext.CreationUserID = P.Eccd.InitUserID;
			SETFLAG(pPack->Rec.Flags, CCHKF_EXT, pPack->HasExt());
			SETIFZ(pPack->Rec.Dt, now_dtm.d);
			SETIFZ(pPack->Rec.Tm, now_dtm.t);
			{
				if(mode == accmJunk) {
					pPack->Rec.Flags |= (CCHKF_SUSPENDED|CCHKF_JUNK);
					pPack->Rec.SessID = 0;
					do_clear_junk_attrs = 1;
				}
				if(SuspCheckID) {
					pPack->Rec.ID = SuspCheckID;
					THROW(r_cc.UpdateCheck(pPack, 0));
					SuspCheckID = 0;
				}
				else {
					THROW(r_cc.TurnCheck(pPack, 0));
				}
				_turn_done = 1;
				// @v11.8.8 {
				{
					S_GUID cc_uuid;
					if(pPack->GetGuid(cc_uuid) && pPack->GetExtStrData(CCheckPacket::extssLinkBillUuid, temp_buf) > 0) {
						S_GUID bill_uuid;
						if(bill_uuid.FromStr(temp_buf)) {
							BillTbl::Rec bill_rec;
							if(BillObj->SearchByGuid(bill_uuid, &bill_rec) > 0) {
								ObjTagItem tag;
								if(tag.SetGuid(PPTAG_BILL_LINKCCHECKUUID, &cc_uuid) && PPRef->Ot.PutTag(PPOBJ_BILL, bill_rec.ID, &tag, 0)) {
									;
								}
								else {
									; // @todo @logerr
								}								
							}
						}
					}
				}
				// } @v11.8.8 
				if(mode == accmJunk) {
					Helper_SetupSessUuidForCheck(pPack->Rec.ID);
					SuspCheckID = pPack->Rec.ID;
				}
			}
		}
		if(mode != accmJunk) {
			if(pExtPack) {
				SETIFZ(pExtPack->Rec.Dt, now_dtm.d);
				SETIFZ(pExtPack->Rec.Tm, now_dtm.t);
				THROW(r_cc.TurnCheck(pExtPack, 0));
			}
			if(OuterOi.Obj == PPOBJ_TSESSION && OuterOi.Id && P_TSesObj) {
				TSessionTbl::Rec tses_rec;
				if(P_TSesObj->Search(OuterOi.Id, &tses_rec) > 0) {
					tses_rec.CCheckID_ = pPack->Rec.ID;
					PPID   tsess_id = OuterOi.Id;
					THROW(P_TSesObj->PutRec(&tsess_id, &tses_rec, 0));
				}
			}
			else if(OuterOi.Obj == PPOBJ_CHKINP && OuterOi.Id) {
				PPCheckInPersonItem cip_item;
				if(cip_mgr.Search(OuterOi.Id, &cip_item) > 0) {
					cip_item.CCheckID = pPack->Rec.ID;
					THROW(cip_mgr.Put(cip_item, 0));
				}
			}
			if(mode == accmRegular && pPack && !(pPack->Rec.Flags & (CCHKF_JUNK|CCHKF_SKIP))) {
				int    scst = scstUnkn;
				SCardTbl::Rec sc_rec;
				if(CSt.GetID()) {
					THROW(ScObj.Search(CSt.GetID(), &sc_rec) > 0);
					scst = ScObj.GetSeriesType(sc_rec.SeriesID);
					//
					// Автоактивация карты
					//
					const long scf_mask = SCRDF_NEEDACTIVATION|SCRDF_AUTOACTIVATION|SCRDF_CLOSED;
					if((sc_rec.Flags & scf_mask) == scf_mask) {
						//
						// Начисление на карту не считаем операцией, которая может активировать карту
						//
						const bool only_charge_goods = IsOnlyChargeGoodsInPacket(sc_rec.ID, pPack);
						if(!only_charge_goods && ScObj.ActivateRec(&sc_rec) > 0)
							THROW(ScObj.P_Tbl->Update(sc_rec.ID, &sc_rec, 0));
					}
					if(CSt.CSTRB.SpecialTreatment) {
						SCardSpecialTreatment * p_scst = SCardSpecialTreatment::CreateInstance(CSt.CSTRB.SpecialTreatment);
						SCardSpecialTreatment::CardBlock scst_cb;
						TSVector <SCardSpecialTreatment::DiscountBlock> scst_dbl;
						if(p_scst && SCardSpecialTreatment::InitSpecialCardBlock(CSt.GetID(), PNP.NodeID, scst_cb) > 0) {
							SCardSpecialTreatment::TransactionResult pack_ta_result;
							SCardSpecialTreatment::TransactionResult extpack_ta_result;
							int    ccr = p_scst->CommitCheck(&scst_cb, pPack, &pack_ta_result);
							int    ccer = pExtPack ? p_scst->CommitCheck(&scst_cb, pExtPack, &extpack_ta_result) : -1;
							if(ccr > 0 || ccer > 0) {
								if(ccr > 0 && !isempty(pack_ta_result.TaIdent)) {
									//pPack->PutExtStrData(CCheckPacket::extssRemoteProcessingTa, pack_ta_result.TaIdent);
									r_cc.UpdateExtText(pPack->Rec.ID, CCheckPacket::extssRemoteProcessingTa, pack_ta_result.TaIdent, 0);
								}
								if(ccer > 0 && !isempty(extpack_ta_result.TaIdent)) {
									//pExtPack->PutExtStrData(CCheckPacket::extssRemoteProcessingTa, extpack_ta_result.TaIdent);
									r_cc.UpdateExtText(pExtPack->Rec.ID, CCheckPacket::extssRemoteProcessingTa, extpack_ta_result.TaIdent, 0);
								}
							}
							else if(!ccr || !ccer) {
								temp_buf = (!ccr) ? pack_ta_result.ErrMessage : extpack_ta_result.ErrMessage;
								MessageError(PPERR_SCSPCTRT_TURNCCTRA, temp_buf, eomPopup);
							}
						}
						ZDELETE(p_scst);
					}
				}
				if(pPack->AL_Const().getCount()) {
					//
					// Если в чеке установлен расширенный список оплат, то все списания произведены функцией CCheckCore::TurnCheck
					// Здесь нам остается только начислить деньги на карту (если необходимо)
					//
					if(!F(fRetCheck)) {
						if(CSt.GetID() && CSt.Flags & CSt.fUhtt && F(fSCardBonusReal|fSCardCredit) && bonus_charge_amt != 0.0) {
							if(CSt.CSTRB.OperationCode[0]) {
								int    is_error = 0;
								PPUhttClient uhtt_cli;
								if(uhtt_cli.Auth()) {
									if(F(fSCardBonusReal) && bonus_charge_amt > 0.0) {
										UhttCheckPacket uhtt_cc_pack;
										double finish_bonus_charge_amt = bonus_charge_amt;
										{
											PPObjSCardSeries scs_obj;
											PPSCardSeries scs_rec;
											if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.BonusChrgExtRule) {
												if(scs_rec.Flags & SCRDSF_BONUSER_ONBNK) {
													if(pPack->AL_Const().Get(CCAMTTYP_BANK) != 0.0) {
														const double _coeff = 1.0 + (static_cast<double>(scs_rec.BonusChrgExtRule) / (10.0 * 100.0));
														finish_bonus_charge_amt *= _coeff;
													}
												}
											}
										}
										uhtt_cc_pack.Amount = finish_bonus_charge_amt;
										LocationTbl::Rec loc_rec;
										if(PsnObj.LocObj.Fetch(PNP.CnLocID, &loc_rec) > 0 && !isempty(loc_rec.Code)) {
											if(!uhtt_cli.CreateSCardCheck(loc_rec.Code, CSt.CSTRB.OperationCode, uhtt_cc_pack)) {
												PPSetError(PPERR_UHTT_SCBONUSREG, uhtt_cli.GetLastMessage());
												is_error = 1;
											}
										}
										else {
											PPGetMessage(mfError, PPERR_LOCSYMBUNDEF, loc_rec.Name, 0, uhtt_err_added_msg);
											PPSetError(PPERR_UHTT_SCBONUSREG, uhtt_err_added_msg);
											is_error = 1;
										}
									}
								}
								else {
									is_error = 1;
								}
								if(is_error)
									MessageError(-1, 0, eomPopup);
							}
						}
					}
				}
				else {
					if(IsCurrentOp(CCOP_RETURN)) {
						//
						// Одновременная проверка на fRetCheck и fRetByCredit - параноидальная, но береженого - Бог бережет.
						//
						if(F(fRetByCredit) && CSt.GetID() && oneof2(scst, scstCredit, scstBonus)) {
							double total = MONEYTOLDBL(pPack->Rec.Amount);
							//
							// Для возвратов суммы отрицательные, но начисление должно быть положительным
							//
							double charge_amount = R2(-(total - CSt.AdditionalPayment));
							if(charge_amount >= 0.01) {
								if(CSt.Flags & CSt.fUhtt && CSt.CSTRB.OperationCode[0]) {
									if(!ScObj.PutUhttOp(CSt.GetID(), charge_amount)) {
										MessageError(-1, 0, eomPopup);
									}
								}
								SCardCore::OpBlock blk;
								blk.Dtm.Set(pPack->Rec.Dt, pPack->Rec.Tm);
								blk.SCardID = CSt.GetID();
								blk.LinkOi.Set(PPOBJ_CCHECK, pPack->Rec.ID);
								blk.Amount = charge_amount;
								THROW(ScObj.P_Tbl->PutOpBlk(blk, &urn_list, 0));
							}
						}
					}
					else {
						double bonus_withdraw_amt = 0.0;
						if(_charge != 0.0) {
						}
						if(CSt.GetID() && (CSt.Flags & CSt.fUhtt) && ((F(fSCardBonusReal) && bonus_charge_amt != 0.0) || (F(fSCardBonus|fSCardCredit) && bonus_withdraw_amt != 0.0))) {
							if(CSt.CSTRB.OperationCode[0]) {
								int    is_error = 0;
								PPUhttClient uhtt_cli;
								if(uhtt_cli.Auth()) {
									if(bonus_withdraw_amt > 0.01 && F(fSCardBonus|fSCardCredit)) {
										if(!uhtt_cli.WithdrawSCardAmount(CSt.CSTRB.OperationCode, bonus_withdraw_amt)) {
											PPSetError(PPERR_UHTT_SCWITHDRAW, uhtt_cli.GetLastMessage());
											is_error = 1;
										}
									}
									if(F(fSCardBonusReal) && bonus_charge_amt > 0.0) {
										UhttCheckPacket uhtt_cc_pack;
										double finish_bonus_charge_amt = bonus_charge_amt;
										{
											PPObjSCardSeries scs_obj;
											PPSCardSeries scs_rec;
											if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.BonusChrgExtRule) {
												if(scs_rec.Flags & SCRDSF_BONUSER_ONBNK) {
													if(pPack->Rec.Flags & CCHKF_BANKING) {
														double _coeff = 1.0 + (static_cast<double>(scs_rec.BonusChrgExtRule) / (10.0 * 100.0));
														finish_bonus_charge_amt *= _coeff;
													}
												}
											}
										}
										uhtt_cc_pack.Amount = finish_bonus_charge_amt;
										LocationTbl::Rec loc_rec;
										if(PsnObj.LocObj.Fetch(PNP.CnLocID, &loc_rec) > 0 && !isempty(loc_rec.Code)) {
											if(!uhtt_cli.CreateSCardCheck(loc_rec.Code, CSt.CSTRB.OperationCode, uhtt_cc_pack)) {
												PPSetError(PPERR_UHTT_SCBONUSREG, uhtt_cli.GetLastMessage());
												is_error = 1;
											}
										}
										else {
											PPGetMessage(mfError, PPERR_LOCSYMBUNDEF, loc_rec.Name, 0, uhtt_err_added_msg);
											PPSetError(PPERR_UHTT_SCBONUSREG, uhtt_err_added_msg);
											is_error = 1;
										}
									}
								}
								else {
									is_error = 1;
								}
								if(is_error)
									MessageError(-1, 0, eomPopup);
							}
						}
					}
				}
			}
		}
		if(!_turn_done && SuspCheckID) {
			THROW(r_cc.RemovePacket(SuspCheckID, 0));
			SuspCheckID = 0;
		}
		r_cc.WriteCCheckLogFile(pPack, 0, (mode == accmRegular) ? CCheckCore::logWrited : CCheckCore::logSuspended, 0);
		THROW(tra.Commit());
	}
	ScObj.FinishSCardUpdNotifyList(urn_list);
	CATCHZOK
	if(do_clear_junk_attrs) {
		pPack->Rec.Flags &= ~(CCHKF_SUSPENDED|CCHKF_JUNK);
		pPack->Rec.SessID = preserve_csess_id;
	}
	return ok;
}

/*static*/int CheckPaneDialog::PalmImport(PalmBillPacket * pPack, void * extraPtr)
{
	int    ok = PIPR_ERROR_BREAK;
	CheckPaneDialog * dlg = static_cast<CheckPaneDialog *>(extraPtr); // This func don't ownes by dlg
	if(pPack && dlg->IsState(sEMPTYLIST_EMPTYBUF)) {
		SString palm_name;
		GetObjectName(PPOBJ_STYLOPALM, pPack->Hdr.PalmID, palm_name);
		int    r = PPMessage(mfConf|mfYes|mfNo|mfCancel|mfDefaultYes, PPCFM_ACCEPTCHECKFROMPALM, palm_name);
		if(r == cmYes) {
			PalmBillItem item;
			for(uint i = 0; pPack->EnumItems(&i, &item) > 0;) {
				RetailGoodsInfo rgi;
				if(dlg->GetRgi(item.GoodsID, item.Qtty, 0, rgi)) {
					const double price = rgi.Price;
					CCheckItem chk_item;
					chk_item.GoodsID = item.GoodsID;
					STRNSCPY(chk_item.GoodsName, rgi.Name);
					STRNSCPY(chk_item.BarCode,   rgi.BarCode);
					chk_item.Quantity = R6((dlg->IsCurrentOp(CCOP_RETURN)) ? -fabs(item.Qtty) : fabs(item.Qtty));
					chk_item.Price    = price;
					chk_item.Discount = 0.0;
					dlg->P.insert(&chk_item);
				}
			}
			dlg->SetupDiscount(0);
			dlg->OnUpdateList(1);
			dlg->ClearRow();
			ok = PIPR_OK_BREAK;
		}
		else if(r == cmNo)
			ok = PIPR_OK_DESTROY;
		else if(r == cmCancel)
			ok = PIPR_OK_DESTROY;
		else // Default
			ok = PIPR_ERROR_BREAK;
	}
	return ok;
}

/*static*/int CheckPaneDialog::SetLbxItemHight(TDialog *, void * extraPtr) // DialogPreProcFunc
{
	int    ok = -1;
	PPSyncCashNode  scn;
	PPObjCashNode   cn_obj;
	if(cn_obj.GetSync(reinterpret_cast<PPID>(extraPtr), &scn) > 0) {
		const  PPID ts_id = NZOR(scn.LocalTouchScrID, scn.TouchScreenID);
		if(ts_id) {
			PPObjTouchScreen ts_obj;
			PPTouchScreenPacket ts_pack;
			if(ts_obj.GetPacket(ts_id, &ts_pack) > 0) {
				int     height  = 0;
				static const  PPID ctrls[] = { CTL_CHKPAN_GDSLIST, CTL_CHKPAN_GRPLIST };
				if(ts_pack.Rec.GdsListFontHight && ts_pack.Rec.GdsListFontName[0]) {
					HDC  dc = GetDC(0);
					const int cy = GetDeviceCaps(dc, LOGPIXELSY);
					height  = labs(ts_pack.Rec.GdsListFontHight) * 72 / cy;
					ReleaseDC(0, dc);
				}
				else
					height = DEFAULT_TS_FONTSIZE;
				for(int i = 0, j = 0; i < 32; i++)
					if(OwnerDrawCtrls[i].CtrlType == 0 && OwnerDrawCtrls[i].CtrlID == 0) {
						OwnerDrawCtrls[i].CtrlType = ctListBox;
						OwnerDrawCtrls[i].CtrlID   = ctrls[j];
						OwnerDrawCtrls[i].ExtraParam = height + ts_pack.Rec.GdsListEntryGap;
						ok = 1;
						if(++j == SIZEOFARRAY(ctrls))
							break;
					}
			}
		}
	}
	return ok;
}

int CheckPaneDialog::SelectGroup(PPID * pGrpID)
{
	int    ok = -1;
	SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GRPLIST));
	if(SmartListBox::IsValidS(p_box)) {
		PPID   grp_id = 0;
		p_box->P_Def->getCurID(&grp_id);
		GrpListItem * p_item = GroupList.Get(grp_id, 0);
		if(p_item) {
			if(p_item->Flags & GrpListItem::fFolder) {
				if(UiFlags & uifOneGroupLevel) {
					GroupList.TopID = p_item->ID;
				}
				else {
					INVERSEFLAG(p_item->Flags, GrpListItem::fOpened);
				}
				ok = 1;
			}
			else
				ok = 2;
			ASSIGN_PTR(pGrpID, p_item->ID);
		}
	}
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(CashNodePane); CashNodePaneFilt::CashNodePaneFilt() : PPBaseFilt(PPFILT_CASHNODEPANE, 0, 1)
{
	SetFlatChunk(offsetof(CashNodePaneFilt, ReserveStart),
		offsetof(CashNodePaneFilt, ReserveEnd) - offsetof(CashNodePaneFilt, ReserveStart));
	Init(1, 0);
}

int CheckPaneDialog::PhnSvcConnect()
{
	int    ok = -1;
	ZDELETE(P_PhnSvcClient);
	PhnSvcLocalChannelSymb.Z();
	if(PNP.CnPhnSvcID) {
		PPObjPhoneService ps_obj(0);
		PPPhoneServicePacket ps_pack;
		if(ps_obj.GetPacket(PNP.CnPhnSvcID, &ps_pack) > 0) {
			P_PhnSvcClient = ps_obj.InitAsteriskAmiClient(PNP.CnPhnSvcID);
			if(P_PhnSvcClient) {
				PhnSvcLocalChannelSymb = ps_pack.LocalChannelSymb;
			}
		}
	}
	return ok;
}

CheckPaneDialog::CheckPaneDialog(PPID cashNodeID, PPID checkID, CCheckPacket * pOuterPack, uint ctrFlags/*int isTouchScreen*/) :
	TDialog(pOuterPack ? ((ctrFlags & ctrfTouchScreen) ? DLG_CHKPANV_L : DLG_CHKPANV) : ((ctrFlags & ctrfTouchScreen) ? DLG_CHKPAN_TS : DLG_CHKPAN),
		(ctrFlags & ctrfTouchScreen) ? CheckPaneDialog::SetLbxItemHight : 0, reinterpret_cast<void *>(cashNodeID)),
	CPosProcessor(cashNodeID, checkID, pOuterPack, ctrFlags, 0/*pDummy*/),
	PhnSvcTimer(1000), UhttImportTimer(180000), BarrierViolationCounter(0)/*@debug*/, ActiveListID(0), SelGoodsGrpID(0), AltGoodsGrpID(0),
	GoodsListFontHeight(0), GoodsListEntryGap(0), P_PalmWaiter(0), P_UhttImporter(0), P_CDY(0), P_BNKTERM(0), P_PhnSvcClient(0)
{
	SetupState(sEMPTYLIST_EMPTYBUF);
	SETFLAG(DlgFlags, fLarge, /*isTouchScreen*/(ctrFlags & ctrfTouchScreen));
	Ptb.SetColor(clrFocus,  RGB(0x20, 0xAC, 0x90));
	Ptb.SetColor(clrEven,   RGB(0xD3, 0xEF, 0xF4));
	Ptb.SetColor(clrOdd,    RGB(0xDC, 0xED, 0xD5));
	Ptb.SetColor(clrGrp,    RGB(0xDA, 0xD7, 0xD0));
	Ptb.SetColor(clrParent, RGB(0x80, 0xB9, 0xE8));
	Ptb.SetBrush(brSel,  SPaintObj::psSolid, Ptb.GetColor(clrFocus), 0);
	Ptb.SetBrush(brEven, SPaintObj::psSolid, Ptb.GetColor(clrEven), 0);
	Ptb.SetBrush(brOdd,  SPaintObj::psSolid, Ptb.GetColor(clrOdd), 0);
	Ptb.SetBrush(brErrorBkg,   SPaintObj::psSolid, GetColorRef(SClrRed), 0);
	Ptb.SetBrush(brPresentBkg, SPaintObj::psSolid, GetColorRef(SClrGreen), 0);
	Ptb.SetBrush(brGrpSel,     SPaintObj::psSolid, Ptb.GetColor(clrFocus), 0);
	Ptb.SetBrush(brGrp,        SPaintObj::psSolid, Ptb.GetColor(clrGrp), 0);
	Ptb.SetBrush(brGrpParent,  SPaintObj::psSolid, Ptb.GetColor(clrParent), 0);
	Ptb.SetPen(penSel, SPaintObj::psSolid, 1, RGB(0x66, 0x33, 0xFF));
	Ptb.SetBrush(brTotalGift,    SPaintObj::psSolid, GetColorRef(SClrSalmon), 0);
	Ptb.SetBrush(brDiscountGift, SPaintObj::psSolid, GetColorRef(SClrSeagreen), 0);
	Ptb.SetBrush(brOrderBkg,     SPaintObj::psSolid, GetColorRef(SClrLightsteelblue), 0);
	//
	// @v11.4.5 LastGrpListUpdTime = getcurdatetime_();
	CnSleepTimeout = 0;
	AutoInputTolerance = 5;
	IdleClock = clock();
	PrintCheckClock = 0;
	ClearCDYTimeout = 0;
	SString temp_buf;
	SString font_face;
	if(!(Flags & fNoEdit)) {
		CsObj.BuildCcDate2MaxIdIndex(PPObjCSession::buildccdate2maxidindexMode_SkipIfCached); // @v11.7.4
		P_PalmWaiter = new PalmImportWaiter(CheckPaneDialog::PalmImport, this); // @newok
		{
			if(APPL->GetUiSettings().Flags & UserInterfaceSettings::fDisableBeep)
				EnableBeep(0);
		}
		if(PNP.CnFlags & CASHF_SYNC) {
			PPIniFile ini_file;
			PPSyncCashNode  scn;
			if(CnObj.GetSync(PNP.NodeID, &scn) > 0) {
				P_CDY = GetCustDisp(scn.CustDispType, scn.CustDispPort, scn.CustDispFlags);
				ClearCDYTimeout = scn.ClearCDYTimeout * CLOCKS_PER_SEC;
				CnSleepTimeout  = scn.SleepTimeout * CLOCKS_PER_SEC;
				if(!GetBnkTerm(scn.BnkTermType, scn.BnkTermLogNum, scn.BnkTermPort, scn.BnkTermPath, &P_BNKTERM))
					Flags |= fLockBankPaym;
				TouchScreenID   = NZOR(scn.LocalTouchScrID, scn.TouchScreenID);
				AltRegisterID   = scn.AlternateRegID;
				PNP.CnPhnSvcID  = scn.PhnSvcID;
				if(scn.ExtCashNodeID) {
					if(scn.ExtFlags & CASHFX_EXTNODEASALT && !AltRegisterID)
						AltRegisterID = scn.ExtCashNodeID;
					else
						ExtCashNodeID = scn.ExtCashNodeID;
				}
				else
					ExtCashNodeID = 0;
				if(AltRegisterID) {
					ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_ALTERNATEREGPASS, temp_buf.Z());
					if(!temp_buf.IsEqiAscii("yes"))
						AltRegisterID = 0;
				}
				ScaleID = scn.ScaleID;
				BonusMaxPart    = (scn.BonusMaxPart > 0 && scn.BonusMaxPart <= 1000) ? R3(((double)scn.BonusMaxPart) / 1000.0) : 1.0;
				scn.GetRoundParam(&R__);
				SETFLAG(Flags, fSelSerial, scn.ExtFlags & CASHFX_SELSERIALBYGOODS);
				SETFLAG(Flags, fForceDivision, scn.ExtFlags & CASHFX_FORCEDIVISION);
				if(ExtCashNodeID) {
					if(CnObj.GetSync(ExtCashNodeID, &scn) > 0)
						PNP.ExtCnLocID = scn.LocID;
					P_GTOA = DS.CheckExtFlag(ECF_CHKPAN_USEGDSLOCASSOC) ?
						new GoodsToObjAssoc(PPASS_GOODS2LOC, PPOBJ_LOCATION) : new GoodsToObjAssoc(PPASS_GOODS2CASHNODE, PPOBJ_CASHNODE);
					CALLPTRMEMB(P_GTOA, Load());
				}
				TableSelWhatman = scn.TableSelWhatman;
				{
					ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_KITCHENBELLCMD, temp_buf);
					KitchenBellCmd = temp_buf.NotEmptyS() ? temp_buf : "1B70000505";
					bool   is_err = false;
					if(KitchenBellCmd.Len() % 2 != 0)
						is_err = true;
					else {
						for(uint i = 0; i < KitchenBellCmd.Len(); i++) {
							if(!ishex(KitchenBellCmd.C(i))) {
								is_err = true;
								break; // @error
							}
						}
					}
					if(is_err) {
						KitchenBellCmd.Z();
						PPSetError(PPERR_INVKITCHENBELLCMD, KitchenBellCmd);
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					}
				}
				{
					int    ait = 0;
					ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_AUTOINPUTTOLERANCE, &ait);
					if(ait > 0 && ait <= 100)
						AutoInputTolerance = ait;
				}
				PhnSvcConnect();
				if(scn.ExtFlags & CASHFX_UHTTORDIMPORT) {
					PPAlbatrossConfig acfg;
					PPAlbatrosCfgMngr::Get(&acfg);
					acfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, temp_buf);
					//if(acfg.UhttAccount.NotEmpty() && acfg.Hdr.OpID) {
					if(temp_buf.NotEmpty() && acfg.Hdr.OpID) {
						P_UhttImporter = new PPBillImporter;
					}
				}
				RunInputProcessThread(PNP.NodeID);
			}
			CDispCommand(cdispcmdText, cdisptxtOpened, 0.0, 0.0);
			if(Flags & fTouchScreen && TouchScreenID) {
				PPTouchScreenPacket ts_pack;
				PPObjTouchScreen    ts_obj;
				if(ts_obj.GetPacket(TouchScreenID, &ts_pack) > 0) {
					LOGFONT log_font;
					int    r = 1; // 0-->1
					ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_TSGOODSGROUPSASBUTTONS, &r);
					SETFLAG(UiFlags, uifTSGGroupsAsButtons, r);
					ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_TSGGROUPLISTFLAT, &(r = 0));
					SETFLAG(UiFlags, uifOneGroupLevel, r);
					SelGoodsGrpID = AltGoodsGrpID = ts_pack.Rec.AltGdsGrpID;
					InitGroupList(ts_pack);
					MEMSZERO(log_font);
					log_font.lfCharSet = DEFAULT_CHARSET;
					GoodsListEntryGap = ts_pack.Rec.GdsListEntryGap;
					if(ts_pack.Rec.GdsListFontName[0]) {
						HDC    dc = ::GetDC(0);
						int    cy = ::GetDeviceCaps(dc, LOGPIXELSY);
						const  int height = labs(ts_pack.Rec.GdsListFontHight) * 72 / cy - ((UiFlags & uifTSGGroupsAsButtons) ? TSGGROUPSASITEMS_FONTDELTA : 0);
						::ReleaseDC(0, dc);
						log_font.lfHeight = height;
						STRNSCPY(log_font.lfFaceName, SUcSwitch(ts_pack.Rec.GdsListFontName));
					}
					else {
						PPGetSubStr(PPTXT_FONTFACE, PPFONT_MSSANSSERIF, temp_buf);
						STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf));
						log_font.lfHeight = (UiFlags & uifTSGGroupsAsButtons) ? (DEFAULT_TS_FONTSIZE - TSGGROUPSASITEMS_FONTDELTA) : DEFAULT_TS_FONTSIZE;
					}
					GoodsListFontHeight = log_font.lfHeight + GoodsListEntryGap + TSGGROUPSASITEMS_FONTDELTA;
					Ptb.SetFont(fontGoodsList, ::CreateFontIndirect(&log_font));
					SETFLAG(Flags, fPrintSlipDoc, ts_pack.Rec.Flags & TSF_PRINTSLIPDOC);
				}
			}
			setTitle(PPLoadStringS("posnode", temp_buf).CatDiv(':', 2).Cat(PNP.CnName));
		}
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_IMPACT, font_face);
		SetCtrlFont(CTL_CHKPAN_TOTAL, font_face, 54);
		SetCtrlFont(CTL_CHKPAN_INFO,  font_face, 20);
		SetCtrlFont(CTL_CHKPAN_CAFE_STATUS, font_face, 20);
		DefInputLine = CTL_CHKPAN_INPUT;
	}
	SetupExt(P_ChkPack);
	P_EGSDlg = 0;
	disableCtrl(CTL_CHKPAN_INPUT, LOGIC(Flags & fNoEdit));
	showCtrl(STDCTL_ALLBUTTON, LOGIC(Flags & fAsSelector));
	showCtrl(STDCTL_SYSINFOBUTTON, !LOGIC(Flags & fAsSelector));
	showCtrl(STDCTL_OKBUTTON,  LOGIC(Flags & fAsSelector));
	showCtrl(STDCTL_PRINT,    !LOGIC(Flags & fAsSelector));
	setupHint();
	SetupInfo(0);
	if(!SetupStrListBox(this, CTL_CHKPAN_LIST) || !LoadCheck(P_ChkPack, 0, false))
		PPError();
	if(Flags & fTouchScreen) {
		// @v12.3.8 SETFLAG(PNP.CnFlags, CASHF_NOASKPAYMTYPE, !(CsObj.GetEqCfg().Flags & PPEquipConfig::fUnifiedPayment));
		// @v12.3.8 {
		if(!(CsObj.GetEqCfg().Flags & PPEquipConfig::fUnifiedPayment)) {
			//
			// Оставим единственный тип допустимой оплаты
			//
			const bool paym_allowed_cash = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmCash));
			const bool paym_allowed_bank = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmBank));
			if(paym_allowed_cash && paym_allowed_bank) {
				PNP.AllowedPaymentTypes &= ~(1 << cpmBank);
			}
			else if(!paym_allowed_cash && !paym_allowed_bank) {
				PNP.AllowedPaymentTypes |= (1 << cpmCash);
			}
		}
		// } @v12.3.8 
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GDSLIST));
		CALLPTRMEMB(p_list, SetOwnerDrawState());
		SmartListBox * p_grp_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GRPLIST));
		if(p_grp_list) {
			p_grp_list->SetOwnerDrawState();
			SetupStrListBox(p_grp_list);
		}
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_MSSANSSERIF, font_face);
		SetCtrlFont(CTL_CHKPAN_INPUT, font_face, 20);
		enableCommand(cmaAltSelect, BIN(AltGoodsGrpID));
		selectCtrl((Flags & fNoEdit) ? CTL_CHKPAN_GRPLIST : CTL_CHKPAN_INPUT);
		SLS.SetUiFlag(sluifUseLargeDialogs, 1);
	}
	else
		selectCtrl((Flags & fNoEdit) ? CTL_CHKPAN_LIST : CTL_CHKPAN_INPUT);
	TView::messageCommand(this, cmSetupTooltip);
	if(PNP.CnSpeciality == PPCashNode::spCafe) {
		setButtonBitmap(cmChkPanF2, IDB_GUESTS);
		setButtonBitmap(cmChkPanF1, IDB_TABLE_ORDERS);
	}
	else if(PNP.CnSpeciality == PPCashNode::spDelivery) {
		setButtonBitmap(cmChkPanF1, IDB_DELIVERY);
	}
	else if(PNP.CnSpeciality == PPCashNode::spApteka) {
		showButton(cmSelTable, 0);
		if(!(Flags & fNoEdit))
			showButton(cmChkPanPrint, 0);
		showButton(cmToLocPrinters, 0);
		//
		const PPEquipConfig & r_cfg = CsObj.GetEqCfg();
		if(r_cfg.ChkPanImpOpID && r_cfg.ChkPanImpBillTagID) {
			setButtonBitmap(cmChkPanF1, PPDV_IMPORT_DOC);
		}
	}
	else if(PNP.CnSpeciality == PPCashNode::spShop) {
		showButton(cmSelTable, 0);
		if(!(Flags & fNoEdit))
			showButton(cmChkPanPrint, 0);
		showButton(cmToLocPrinters, 0);
	}
	if(Flags & fLockBankPaym || !(OperRightsFlags & orfBanking)) {
		setButtonBitmap(cmBanking, IDB_BANKINGDISABLED);
	}
	LastCtrlID = 0;
}

CheckPaneDialog::~CheckPaneDialog()
{
	SLS.SetUiFlag(sluifUseLargeDialogs, 0);
	DS.GetTLA().AddedMsgStrNoRights.Z();
	for(int i = 0; i < 32; i++)
		MEMSZERO(OwnerDrawCtrls[i]);
	delete P_EGSDlg;
	{
		CDispCommand(cdispcmdText, cdisptxtClosed, 0.0, 0.0);
		ZDELETE(P_CDY);
	}
	delete P_BNKTERM;
	delete P_PalmWaiter;
	delete P_PhnSvcClient;
	delete P_UhttImporter;
}

int CheckPaneDialog::SetupState(int st)
{
	int    ok = CPosProcessor::SetupState(st);
	if(ok > 0)
		showButton(cmSelModifier, oneof3(State_p, sEMPTYLIST_BUF, sLIST_BUF, sLISTSEL_BUF));
	return ok;
}

void CheckPaneDialog::EnableBeep(int enbl) { SETFLAG(Flags, fDisableBeep, !enbl); }

/*virtual*/int CheckPaneDialog::AcceptCheck(PPID * pCcID, const CcAmountList * pPl, PPID altPosNodeID, double cash, int mode /* accmXXX */)
{
	int    ok = CPosProcessor::AcceptCheck(pCcID, pPl, altPosNodeID, cash, mode /*suspended*/);
	if(!oneof2(mode, accmJunk, accmAveragePrinting)) {
		if(ClearCDYTimeout)
			PrintCheckClock = clock();
		else
			CDispCommand(cdispcmdText, cdisptxtOpened, 0.0, 0.0);
		if(ok > 0 && UiFlags & uifOnce)
			TView::messageCommand(this, cmCancel);
	}
	return ok;
}

int CheckPaneDialog::SuspendCheck()
{
	int    ok = -1;
	const  int  prev_state = GetState();
	const  PPID prev_agent_id = P.GetAgentID(1);
	if(IsState(sEMPTYLIST_EMPTYBUF)) {
		SelectSuspendedCheck();
	}
	else if(IsState(sLIST_EMPTYBUF)) {
		if(OperRightsFlags & orfSuspCheck) {
			PPID   cc_id = 0;
			const CcTotal cct = CalcTotal();
			CDispCommand(cdispcmdClear, 0, 0.0, 0.0);
			CDispCommand(cdispcmdTotal, 0, cct.Amount, 0.0);
			if(cct.Discount != 0.0)
				CDispCommand(cdispcmdTotalDiscount, 0, (cct.Discount * 100.0) / (cct.Amount + cct.Discount), cct.Discount);
			AcceptCheck(&cc_id, 0, 0, cct.Amount, accmSuspended);
			ok = 1;
		}
	}
	if(GetState() != prev_state || P.GetAgentID(1) != prev_agent_id)
		setupHint();
	return ok;
}

int CheckPaneDialog::IsCode(const SString & rInput, SString & rPfx, int asterix, SString & rCode) const
{
	if(asterix)
		rPfx.PadLeft(1, '*');
	if(rInput.CmpPrefix(rPfx, 1) == 0) {
		(rCode = rInput).ShiftLeft(rPfx.Len());
		if(asterix)
			rCode.TrimRightChr('*');
		return 1;
	}
	else
		return 0;
}

int CheckPaneDialog::IsSalCode(const SString & rInput, SString & rCode)
{
	int    ok = 0;
	int    asterix = 0;
	SString pfx;
	const PPEquipConfig & r_cfg = CsObj.GetEqCfg();
	if(r_cfg.AgentPrefix[0] && sstrlen(r_cfg.AgentPrefix) < (uint)r_cfg.AgentCodeLen) {
		pfx = r_cfg.AgentPrefix;
		do {
			if(IsCode(rInput, pfx, asterix, rCode))
				ok = 1;
		} while(!ok && ++asterix < 2);
	}
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr();;
		TranslateLocaleKeyboardTextToLatin(rInput, r_temp_buf); // @v11.2.5
		do {
			// @v11.2.5 {
			if(IsCode(r_temp_buf, pfx = "SAL", asterix, rCode))
				ok = 1;
			// } @v11.2.5 
			/* @v11.2.5
			if(IsCode(rInput, pfx = "SAL", asterix, rCode))
				ok = 1;
			else if(IsCode(rInput, pfx = "ЫФД", asterix, rCode))
				ok = 1;
			else if(IsCode(rInput, (pfx = "ЫФД").ToOem(), asterix, rCode))
				ok = 1;
			*/
		} while(!ok && ++asterix < 2);
	}
	return ok;
}

int CheckPaneDialog::SetupSalByCode(const SString & rInput)
{
	int    ok = -1;
	SString code;
	if(IsSalCode(rInput, code)) {
		if(code.ToLong() == 0) {
			SetupAgent(0, 0);
			SetupInfo(0);
			ok = 2;
		}
		else {
			const  PPID acs_id = GetAgentAccSheet();
			PPID   ar_id = 0;
			PPID   reg_type_id = 0;
			if(acs_id && code.NotEmpty() && PPObjArticle::GetSearchingRegTypeID(acs_id, 0, 0, &reg_type_id) > 0)
				ArObj.SearchByRegCode(acs_id, reg_type_id, code, &ar_id, 0);
			if(ar_id) {
				SetupAgent(ar_id, 0);
				SetupInfo(0);
				ok = 1;
			}
			else
				ok = MessageError(PPERR_ARCODENFOUND, Input, eomMsgWindow);
		}
	}
	return ok;
}

void CheckPaneDialog::AddFromBasket()
{
	int    is_locked = 0;
	PPBasketCombine bc;
	int    ok = GoodsBasketDialog(bc, 2);
	if(ok > 0 && bc.BasketID) {
		ILTI * p_item;
		for(uint i = 0; bc.Pack.Lots.enumItems(&i, (void **)&p_item);) {
			PgsBlock pgsb(p_item->Quantity);
			SetupNewRow(p_item->GoodsID, pgsb);
		}
		AcceptRow();
	}
}

void CheckPaneDialog::ViewStoragePlaces(PPID goodsId)
{
	if(CheckID <= 0/* @v11.7.11 && CnSpeciality == PPCashNode::spApteka*/) {
		const PPID goods_id = NZOR(goodsId, (P.HasCur() ? P.GetCur().GoodsID : 0));
		if(goods_id) {
			const PPID assc = PPASS_GOODS2WAREPLACE;
			SString out_msg;
			SString loc_name;
			SString tag_name;
			SString tag_value; //@erik v10.4.9
			{
				GoodsToObjAssoc gtoa(assc, PPOBJ_LOCATION);
				if(gtoa.IsValid() && gtoa.Load()) {
					PPID   loc_id = 0;
					if(gtoa.Get(goods_id, &loc_id) > 0 && loc_id) {
						LocationTbl::Rec loc_rec;
						if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0)
							loc_name = loc_rec.Name;
					}
				}
			}
			{
				const ObjTagItem * p_item = 0;
				bool  found = false;
				ObjTagList tag_list;
				PPObjTag obj_tag;
				GObj.GetTagList(goods_id, &tag_list);
				for(uint pos = 0; !found && (p_item = tag_list.EnumItems(&pos));) {
					PPObjTagPacket tag_pack;
					if(obj_tag.GetPacket(p_item->TagID, &tag_pack) > 0 && tag_pack.Rec.Flags & OTF_NOTICEINCASHPANE) {
						p_item->GetStr(tag_value); //@erik v10.4.10
						tag_name = tag_pack.Rec.Name;
						found = true;
					}
				}
			}
			//@erik v10.4.9{
			if(tag_name.Len()) {
				if(tag_value.Len()) {
					(out_msg = (tag_name.Colon())).CR();
					(out_msg.Cat(tag_value)).CR().CR();
				}
				else {
					(out_msg = tag_name).CR().CR();
				}
			}//}@erik
			if(loc_name.Len()) {
				SString buf;
				PPLoadStringS("storageplace", buf).Colon();
				out_msg.Cat(buf).CR();
				out_msg.Cat(loc_name);
			}
			if(out_msg.Len()) {
				PPTooltipMessage(out_msg, 0, H(), 20000, GetColorRef(SClrCyan),
					SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
			}
			else
				SMessageWindow::DestroyByParent(H());
		}
	}
}

int CheckPaneDialog::SetupOrder(PPID ordCheckID)
{
	int    ok = 1;
	if(ordCheckID) {
		SString cc_text;
		CCheckPacket cc_pack;
		THROW(GetCc().LoadPacket(ordCheckID, 0, &cc_pack) > 0);
		CCheckCore::MakeCodeString(&cc_pack.Rec, 0, cc_text);
		THROW_PP_S(cc_pack.Rec.Flags & CCHKF_ORDER, PPERR_CCHKNORDER, cc_text);
		THROW_PP_S(!(cc_pack.Rec.Flags & CCHKF_SKIP), PPERR_CCHKORDCANCELED, cc_text);
		THROW_PP_S(!(cc_pack.Rec.Flags & CCHKF_CLOSEDORDER), PPERR_CCHKORDCLOSED, cc_text);
		P.OrderCheckID = cc_pack.Rec.ID;
		P.TableCode = cc_pack.Ext.TableNo;
		if(cc_pack.Rec.SCardID)
			AcceptSCard(cc_pack.Rec.SCardID, 0, ascfIgnoreRights);
	}
	else {
		P.OrderCheckID = 0;
	}
	SetupInfo(0);
	CATCH
		ok = MessageError(PPErrCode, 0, eomStatusLine|eomBeep);
	ENDCATCH
	return ok;
}

int CPosProcessor::CalculatePaymentList(PosPaymentBlock & rBlk, int interactive)
{
	int    ok = 1;
	const  double add_paym_epsilon = 0.0099;
	const  bool   paym_allowed_cash = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmCash));
	const  bool   paym_allowed_bank = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmBank));
	const  bool   bank_paym_strictly_disabled = (!(OperRightsFlags & orfBanking) || (Flags & fLockBankPaym));
	const  bool   unified_paym_interface = LOGIC(CsObj.GetEqCfg().Flags & PPEquipConfig::fUnifiedPayment);
	const  bool   prefer_banking_payment = (!bank_paym_strictly_disabled && ((CsObj.GetEqCfg().Flags & PPEquipConfig::fPreferBankingPayment) || !paym_allowed_cash)); // @v12.1.10
	double non_crd_amt = 0.0;
	const  double credit_charge = CalcCreditCharge(0, 0, 0, &non_crd_amt, 0);
	double addpaym_r2 = R2(CSt.AdditionalPayment);
	uint   v = 0;
	double diff = 0.0;
	rBlk.Init(this);
	if(Flags & fLockBankPaym || !(PNP.AllowedPaymentTypes & (1 << cpmBank))) // @v12.3.8 (|| !(PNP.AllowedPaymentTypes & (1 << cpmBank)))
		rBlk.DisabledKinds |= (1 << cpmBank);
	// @v12.3.8 {
	if(!(PNP.AllowedPaymentTypes & (1 << cpmCash)))
		rBlk.DisabledKinds |= (1 << cpmCash);
	// } @v12.3.8 
	if(Flags & fSCardCredit && !(Flags & fSCardBonus) && CSt.GetID() && addpaym_r2 <= add_paym_epsilon) {
		if(unified_paym_interface && ((credit_charge > 0.0) || non_crd_amt >= rBlk.GetTotal())) 
			// @v11.1.10 (credit_charge > 0.0)-->(feqeps(credit_charge, 0.0, add_paym_epsilon))
			// @v11.7.9 !feqeps(credit_charge, 0.0, add_paym_epsilon)-->(credit_charge > 0.0)
			// @v11.5.11 (non_crd_amt >= rBlk.GetTotal())
			if(Flags & fBankingPayment)
				rBlk.Kind = cpmBank;
			else
				rBlk.Kind = cpmUndef;
		else
			rBlk.Kind = cpmIncorpCrd;
	}
	else if(Flags & fBankingPayment && !bank_paym_strictly_disabled) {
		rBlk.Kind = cpmBank;
	}
	// @v12.3.8 else if((Flags & fLockBankPaym) || !(OperRightsFlags & orfBanking) || (PNP.CnFlags & CASHF_NOASKPAYMTYPE)) { rBlk.Kind = cpmCash; }
	else if(bank_paym_strictly_disabled || !(PNP.AllowedPaymentTypes & (1 << cpmBank))) { rBlk.Kind = cpmCash; } // @v12.3.8 
	else if(unified_paym_interface)
		rBlk.Kind = cpmUndef;
	else if(interactive) {
		if(SelectorDialog(DLG_CHKPAYM, CTL_CHKPAYM_METHOD, &v) > 0) {
			if(v == 0)
				rBlk.Kind = cpmCash;
			else if(v == 1)
				rBlk.Kind = cpmBank;
		}
		else
			ok = -1;
	}
	else
		rBlk.Kind = cpmCash;
	if(ok > 0) {
		assert(oneof4(rBlk.Kind, cpmUndef, cpmCash, cpmBank, cpmIncorpCrd));
		SETFLAG(rBlk.Flags, PosPaymentBlock::fPreferCashlessPayment, prefer_banking_payment); // @v12.1.10
		if(IsCurrentOp(CCOP_RETURN) && P.Lb_.AmL.GetTotal() != 0.0) {
			//
			// Если возврат осуществляется по чеку со сложными суммами, то масштабируем список
			// до суммы возврата и (изменив флаг) используем для расчета.
			//
			rBlk.CcPl = P.Lb_.AmL;
			rBlk.CcPl.InvertSign();
			rBlk.CcPl.ScaleTo(-fabs(rBlk.GetTotal()));
			rBlk.Kind = cpmUndef;
		}
		else if(F(fRetByCredit)) {
			if(addpaym_r2 > add_paym_epsilon) {
				rBlk.AmtToPaym = R2(rBlk.GetTotal());
				assert(CSt.GetID() != 0); // Если карта не установлена, то попасть в эту ветку кода невозможно
				if(rBlk.Kind == cpmBank) {
					rBlk.CcPl.Add(CCAMTTYP_BANK, rBlk.AmtToPaym);
				}
				else if(rBlk.Kind == cpmIncorpCrd) {
					// При предустановленном типе оплаты cmpIncorpCrd мы не должны попасть в эту ветку
					assert(rBlk.Kind != cpmIncorpCrd);
				}
				else if(rBlk.Kind == cpmCash) { // @v12.1.10
					rBlk.CcPl.Add(CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				else {
					assert(rBlk.Kind == cpmUndef); // Другие варианты исключены
					rBlk.CcPl.Add(prefer_banking_payment ? CCAMTTYP_BANK : CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetTotal() - addpaym_r2, CSt.GetID());
			}
			else {
				rBlk.AmtToPaym = addpaym_r2;
				if(rBlk.Kind == cpmIncorpCrd) {
					assert(Flags & fSCardCredit && !(Flags & fSCardBonus) && CSt.GetID());
					rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetTotal(), CSt.GetID());
				}
				else {
					if(rBlk.Kind == cpmBank) {
						rBlk.CcPl.Add(CCAMTTYP_BANK, rBlk.AmtToPaym);
					}
					if(rBlk.Kind == cpmCash) { // @v12.1.10
						rBlk.CcPl.Add(CCAMTTYP_CASH, rBlk.AmtToPaym);
					}
					else {
						assert(rBlk.Kind == cpmUndef); // Другие варианты исключены
						rBlk.CcPl.Add(prefer_banking_payment ? CCAMTTYP_BANK : CCAMTTYP_CASH, rBlk.AmtToPaym);
					}
					rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetTotal() - rBlk.AmtToPaym, CSt.GetID());
				}
			}
		}
		else {
			if(fabs(addpaym_r2) > add_paym_epsilon) {
				if(addpaym_r2 > fabs(rBlk.GetTotal())) {
					MessageError(PPERR_CHKPAN_ADDPAYMABOVEAMT, 0, eomBeep|eomPopup);
					addpaym_r2 = rBlk.GetTotal() - rBlk.GetUsableBonus();
				}
				rBlk.AmtToPaym = addpaym_r2;
				assert(CSt.GetID() != 0); // Если карта не установлена, то попасть в эту ветку кода невозможно
				if(rBlk.Kind == cpmBank) {
					rBlk.CcPl.Add(CCAMTTYP_BANK, rBlk.AmtToPaym);
				}
				else if(rBlk.Kind == cpmIncorpCrd) {
					// При предустановленном типе оплаты cmpIncorpCrd мы не должны попасть в эту ветку
					assert(rBlk.Kind != cpmIncorpCrd);
				}
				else if(rBlk.Kind == cpmCash) { // @v12.1.10
					rBlk.CcPl.Add(CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				else {
					assert(rBlk.Kind == cpmUndef); // Другие варианты исключены
					rBlk.CcPl.Add(prefer_banking_payment ? CCAMTTYP_BANK : CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetTotal() - rBlk.AmtToPaym, CSt.GetID());
			}
			else {
				rBlk.AmtToPaym = R2(rBlk.GetTotal() - rBlk.GetUsableBonus());
				if(rBlk.Kind == cpmBank) {
					rBlk.CcPl.Add(CCAMTTYP_BANK, rBlk.AmtToPaym);
				}
				else if(rBlk.Kind == cpmIncorpCrd) {
					assert(Flags & fSCardCredit && !(Flags & fSCardBonus) && CSt.GetID());
					rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetTotal(), CSt.GetID());
				}
				else if(rBlk.Kind == cpmCash) { // @v12.1.10
					rBlk.CcPl.Add(CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				else {
					assert(rBlk.Kind == cpmUndef); // Другие варианты исключены (see above)
					rBlk.CcPl.Add(prefer_banking_payment ? CCAMTTYP_BANK : CCAMTTYP_CASH, rBlk.AmtToPaym);
				}
				if(rBlk.GetUsableBonus() != 0.0) {
					rBlk.CcPl.Add(CCAMTTYP_CRDCARD, rBlk.GetUsableBonus(), CSt.GetID());
				}
			}
			{
				const double ccpl_total = rBlk.CcPl.GetTotal();
				const double blk_total = rBlk.GetTotal();
				assert(feqeps(ccpl_total, blk_total, 1E-5));
			}
		}
	}
	return ok;
}

int CheckPaneDialog::ConfirmPosPaymBank(PosPaymentBlock & rPpl)
{
	class ConfirmPosPaymBankDialog : public TDialog {
		DECL_DIALOG_DATA(PosPaymentBlock);
		enum {
			dummyFirst = 1,
			brushInvalid,
			brushEAddrPhone,
			brushEAddrEmail,
		};
	public:
		ConfirmPosPaymBankDialog() : TDialog(DLG_POSPAYMBNK), Data(0, 0.0), EAddrInputState(0)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			{
				const UiDescription * p_uid = SLS.GetUiDescription();
				const SColorSet * p_cs = p_uid ? p_uid->GetColorSetC("papyrus_style") : 0;
				{
					SColor _color;
					if(!p_cs || !p_cs->Get("invalid_value_input_bg", &p_uid->ClrList, _color))
						_color = SClrCoral; 
					Ptb.SetBrush(brushInvalid, SPaintObj::bsSolid, _color, 0);
				}
				//"eaddr_phone_input_bg": "#aqua",
				//"eaddr_email_input_bg": "#cadetblue"
				{
					SColor _color;
					if(!p_cs || !p_cs->Get("eaddr_phone_input_bg", &p_uid->ClrList, _color))
						_color = SClrAqua; 
					Ptb.SetBrush(brushEAddrPhone, SPaintObj::bsSolid, _color,  0);
				}
				{
					SColor _color;
					if(!p_cs || !p_cs->Get("eaddr_email_input_bg", &p_uid->ClrList, _color))
						_color = SClrCadetblue; 
					Ptb.SetBrush(brushEAddrEmail, SPaintObj::bsSolid, _color,  0);
				}
			}
			if(!DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) {
				showCtrl(CTL_POSPAYMBNK_EADDR, false);
				showCtrl(CTL_POSPAYMBNK_EADDRINF, false);
				showCtrl(CTL_POSPAYMBNK_PAPERLESS, false);
				showCtrl(CTLFRAME_POSPAYMBNK_PAPERLESS, false);
			}
			setCtrlReal(CTL_POSPAYMBNK_AMOUNT, Data.AmtToPaym);
			// @v12.0.6 {
			AddClusterAssoc(CTL_POSPAYMBNK_CLBPEQ, 0, PosPaymentBlock::fCashlessBypassEq);
			SetClusterData(CTL_POSPAYMBNK_CLBPEQ, Data.Flags);
			showCtrl(CTL_POSPAYMBNK_CLBPEQ, LOGIC(Data.Flags & PosPaymentBlock::fCashlessBypassEqEnabled));
			// } @v12.0.6 
			if(DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) { // @v11.3.7
				if(Data.EAddr.IsEmpty())
					Data.EAddr.SetEMail(DS.GetConstTLA().PaperlessCheque_FakeEAddr);
				setCtrlString(CTL_POSPAYMBNK_EADDR, Data.EAddr.EAddr);
				setCtrlUInt16(CTL_POSPAYMBNK_PAPERLESS, BIN(Data.Flags & PosPaymentBlock::fPaperless));
			}
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint16 v = 0;
			Data.SetBuyersEAddr(0, 0);
			if(DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) {
				SString eaddr_buf;
				getCtrlString(CTL_POSPAYMBNK_EADDR, eaddr_buf);
				const int eaddr_status = GetEAddrStatus(eaddr_buf);
				if(oneof2(eaddr_status, SNTOK_EMAIL, SNTOK_PHONE)) {
					if(eaddr_status == SNTOK_PHONE) {
						SString normal_phone;
						eaddr_buf = PPEAddr::Phone::NormalizeStr(eaddr_buf, 0, normal_phone);
					}
					Data.SetBuyersEAddr(eaddr_status, eaddr_buf);
					v = getCtrlUInt16(CTL_POSPAYMBNK_PAPERLESS);
					SETFLAG(Data.Flags, PosPaymentBlock::fPaperless, v);
				}
				else
					Data.Flags &= ~PosPaymentBlock::fPaperless;
			}
			else
				Data.Flags &= ~PosPaymentBlock::fPaperless;
			// @v12.0.6 {
			if(Data.Flags & PosPaymentBlock::fCashlessBypassEqEnabled)
				GetClusterData(CTL_POSPAYMBNK_CLBPEQ, &Data.Flags);
			else
				SETFLAG(Data.Flags, PosPaymentBlock::fCashlessBypassEq, 0);
			// } @v12.0.6 
			ASSIGN_PTR(pData, Data);
			return ok;

		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCMD == cmCtlColor) {
				TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
				if(p_dc && getCtrlHandle(CTL_POSPAYMBNK_EADDR) == p_dc->H_Ctl) {
					int brush_ident = 0;
					if(EAddrInputState == SNTOK_PHONE)
						brush_ident = brushEAddrPhone;
					else if(EAddrInputState == SNTOK_EMAIL)
						brush_ident = brushEAddrEmail;
					else if(EAddrInputState < 0)
						brush_ident = brushInvalid;
					if(brush_ident) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brush_ident));
					}
				}
				else
					return;
			}
			else if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_POSPAYMBNK_EADDR)) {
					SString eaddr_buf;
					getCtrlString(CTL_POSPAYMBNK_EADDR, eaddr_buf);
					EAddrInputState = GetEAddrStatus(eaddr_buf);
					drawCtrl(CTL_POSPAYMBNK_EADDR);
				}
			}
		}
		int    GetEAddrStatus(SString & rBuf)
		{
			int    status = 0;
			if(rBuf.NotEmptyS()) {
				SNaturalTokenArray nta;
				Trgn.Run(rBuf.ucptr(), rBuf.Len(), nta, 0);
				if(nta.Has(SNTOK_PHONE))
					status = SNTOK_PHONE;
				else if(nta.Has(SNTOK_EMAIL))
					status = SNTOK_EMAIL;
				else
					status = -1;
			}
			else
				status = 0;
			return status;
		}
		SPaintToolBox Ptb;
		PPTokenRecognizer Trgn;
		int    EAddrInputState; // 0 - empty, -1 - invalid, SNTOK_PHONE, SNTOK_EMAIL
	};
	int    yes = 1;
	if(/*amount*/rPpl.AmtToPaym != 0.0) {
		ConfirmPosPaymBankDialog * dlg = new ConfirmPosPaymBankDialog();
		if(CheckDialogPtrErr(&dlg)) {
			// @v11.9.8 {
			/*if(!DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) {
				dlg->showCtrl(CTL_POSPAYMBNK_EADDR, false);
				dlg->showCtrl(CTL_POSPAYMBNK_EADDRINF, false);
				dlg->showCtrl(CTL_POSPAYMBNK_PAPERLESS, false);
				dlg->showCtrl(CTLFRAME_POSPAYMBNK_PAPERLESS, false);
			}*/
			// } @v11.9.8
			//dlg->setCtrlReal(CTL_POSPAYMBNK_AMOUNT, /*amount*/rPpl.AmtToPaym);
			dlg->setDTS(&rPpl);
			if(ExecView(dlg) == cmOK) {
				if(!dlg->getDTS(&rPpl)) {
					yes = 0;
				}
			}
			else
				yes = 0;
		}
		delete dlg;
	}
	return yes;
}

/*static*/SCardSpecialTreatment::IdentifyReplyBlock * CPosProcessor::GetSpecialTreatmentBlock(const TSVector <SCardSpecialTreatment::IdentifyReplyBlock> & rScpRbList, PPID scID)
{
	SCardSpecialTreatment::IdentifyReplyBlock * p_ret = 0;
	if(scID) {
		for(uint i = 0; !p_ret && i < rScpRbList.getCount(); i++) {
			if(rScpRbList.at(i).ScID == scID)
				p_ret = &rScpRbList.at(i);
		}
	}
	return p_ret;
}

int CPosProcessor::RecognizeCode(int mode, const char * pCode, int autoInput)
{
	int    ok = -1;
	if(!isempty(pCode)) {
		int    try_next = 1;
		PPID   ar_id = 0;
		PPID   reg_type_id = 0;
		const  PPID acs_id = GetAgentAccSheet();
		SString ss_code;
		if(oneof2(mode, crmodeAuto, crmodeAgent)) {
			if(acs_id && PPObjArticle::GetSearchingRegTypeID(acs_id, 0, 0, &reg_type_id) > 0)
				ArObj.SearchByRegCode(acs_id, reg_type_id, pCode, &ar_id, 0);
			if(ar_id) {
				SetupAgent(ar_id, BIN(mode == crmodeAgent)); // asAuthAgent=1 под вопросом. Сейчас установлено потому,
					// что функция вызывается только мобильным агентом и в этом случае так правильно.
				SetupInfo(0);
				try_next = 0;
				ok = 1;
			}
			else if(mode == crmodeAgent) {
				try_next = 0;
				ok = 0; // @error
			}
		}
		if(try_next && oneof2(mode, crmodeAuto, crmodeSCard)) {
			//
			// Выбор карты
			//
			if(PPObjSCard::PreprocessSCardCode(ss_code = pCode) > 0) {
				char   card_code[64];
				ss_code.CopyTo(card_code, sizeof(card_code));
				if(card_code[0]) {
					SCardTbl::Rec sc_rec;
					if(ScObj.SearchCode(0, card_code, &sc_rec) > 0) {
						if(autoInput || !(CsObj.GetEqCfg().Flags & PPEquipConfig::fDisableManualSCardInput))
							ok = Backend_AcceptSCard(sc_rec.ID, 0/*const SCardSpecialTreatment::IdentifyReplyBlock* */, 0);
						else
							ok = MessageError(PPERR_MANUALSCARDINPUTDISABLED, 0, eomBeep|eomStatusLine);
						try_next = 0;
					}
					else if(mode == crmodeSCard) {
						try_next = 0;
						ok = MessageError(PPERR_SCARDNOTFOUND, pCode, eomBeep|eomStatusLine);
					}
				}
			}
			else
				ok = MessageError(PPERR_GDSBYBARCODENFOUND, pCode, eomBeep|eomMsgWindow);
		}
		if(try_next && oneof2(mode, crmodeAuto, crmodeGoods)) {
			ok = 0;
		}
	}
	return ok;
}

static void FASTCALL InformCashNoteAndDelivery(HWND hParentWnd, const PosPaymentBlock & rBlk)
{
	SMessageWindow * p_win = new SMessageWindow;
	if(p_win) {
		double cash_amount = rBlk.CcPl.Get(CCAMTTYP_CASH);
		if(cash_amount == 0.0)
			cash_amount = rBlk.CashAmt;
		if(cash_amount == 0.0)
			cash_amount = rBlk.AmtToPaym;
		SString temp_buf;
		SString msg_buf;
		SString words;
		PPLoadText(PPTXT_CUSTDISP_WORDS, words);
		PPGetSubStr(words, PPCDY_TOTAL, temp_buf);
		msg_buf.Cat(temp_buf).Space().Cat(cash_amount, SFMT_MONEY).CR();
		PPGetSubStr(words, PPCDY_CASH, temp_buf);
		msg_buf.Cat(temp_buf).CatDiv(':', 2).Cat(rBlk.NoteAmt, SFMT_MONEY).CR();
		PPGetSubStr(words, PPCDY_CHANGE, temp_buf);
		msg_buf.Cat(temp_buf).CatDiv(':', 2).Cat(rBlk.DeliveryAmt, SFMT_MONEY).CR();
		p_win->Open(msg_buf, 0, hParentWnd, 0, 10000, GetColorRef(SClrCyan),
			SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText, 0);
	}
}

void CheckPaneDialog::PrintBankingSlip(int afterReceipt, const SString & rSlipBuf)
{
	if((afterReceipt && (PNP.CnExtFlags & CASHFX_BNKSLIPAFTERRCPT)) || (!afterReceipt && !(PNP.CnExtFlags & CASHFX_BNKSLIPAFTERRCPT))) {
		if(rSlipBuf.NotEmpty() && InitCashMachine() && P_CM)
			P_CM->SyncPrintBnkTermReport(rSlipBuf);
	}
}

void CheckPaneDialog::ProcessEnter(int selectInput)
{
	const int  prev_state = GetState();
	const PPID prev_agent_id = P.GetAgentID(1);
	SString temp_buf;
	if(selectInput)
		selectCtrl(CTL_CHKPAN_INPUT);
	if(isCurrCtlID(CTL_CHKPAN_INPUT)) {
		if(Flags & fWaitOnSCard)
			AcceptSCard(0, 0, ascfFromInput);
		else if(Flags & fSelByPrice)
			SelectGoods__(sgmByPrice);
		else if(GetInput()) {
			const bool auto_input = LOGIC(UiFlags & uifAutoInput);
			SString ss_code;
			CCheckPacket::BarcodeIdentStruc bis;
			if(Input.HasPrefixIAscii("TBL")) {
				const  int table_no = Input.ShiftLeft(3).ToLong();
				long   guest_count = 0;
				if(PNP.CnExtFlags & CASHFX_INPGUESTCFTBL)
					SelectGuestCount(table_no, &guest_count);
				SetupCTable(table_no, guest_count);
				ClearInput(0);
			}
			else if(SetupSalByCode(Input) >= 0)
				ClearInput(0);
			else if(Input.IsEqiAscii("SUS00"))
				SuspendCheck();
			else if(Input.IsEqiAscii("SUS01"))
				SelectSuspendedCheck();
			else if(CCheckPacket::ParseBarcodeIdent(Input, &bis)) {
				if(IsState(sEMPTYLIST_EMPTYBUF)) {
					if(OperRightsFlags & orfRestoreSuspWithoutAgent || P.GetAgentID()) {
						if(bis.PosId == PNP.NodeID) {
							uint   candid_count = 0;
							LDATETIME max_dtm = ZERODATETIME;
							uint   _pos = 0;
							TSVector <CCheckTbl::Rec> cc_list;
							if(GetCc().GetListByCode(bis.PosId, bis.CcCode, &cc_list) > 0) {
                                for(uint i = 0; i < cc_list.getCount(); i++) {
                                	const CCheckTbl::Rec & r_rec = cc_list.at(i);
                                	assert(r_rec.PosNodeID == PNP.NodeID && r_rec.Code == bis.CcCode);
                                    if(r_rec.PosNodeID == PNP.NodeID && r_rec.Code == bis.CcCode) { // @paranoic
                                        if(r_rec.Flags & CCHKF_SUSPENDED) {
                                        	candid_count++;
											LDATETIME cc_dtm;
											cc_dtm.Set(r_rec.Dt, r_rec.Tm);
											if(cmp(cc_dtm, max_dtm) > 0) {
												max_dtm = cc_dtm;
												_pos = i+1;
											}
                                        }
                                    }
                                }
                                if(_pos > 0) {
									assert(_pos <= cc_list.getCount());
									assert(candid_count > 0);
									if(candid_count > 1) {
									}
									if(!RestoreSuspendedCheck(cc_list.at(_pos-1).ID, 0/*pPack*/, 0/*unfinishedForReprinting*/)) {
										MessageError(-1, 0, eomBeep|eomStatusLine);
									}
                                }
                                else {
									temp_buf.Z().Cat(bis.PosId).Space().Cat(bis.CcCode);
									MessageError(PPERR_CHKPAN_SUSPCHKNFOUND, temp_buf, eomBeep|eomStatusLine);
                                }
							}
						}
					}
					else
						MessageError(PPERR_NORIGHTSELSUSPCHECK, 0, eomBeep|eomStatusLine);
				}
				else
					MessageError(PPERR_CHKPAN_CANTRESTORESUSPNE, 0, eomBeep|eomStatusLine);
			}
			else if(Input.HasPrefixIAscii("DIV")) {
				const int div = Input.ShiftLeft(3).ToLong();
				if(P.HasCur() && P.GetCur().GoodsID && (div > 0 && div < 1000)) {
					P.GetCur().Division = static_cast<int16>(div);
					SetupInfo(0);
				}
				ClearInput(0);
			}
			else if(Input.HasPrefixIAscii("TSN")) {
				const PPID sess_id = Input.ShiftLeft(3).ToLong();
				LoadTSession(sess_id);
				ClearInput(0);
			}
			else if(Input.HasPrefixIAscii("CHKINP")) {
				PPID   cip_id = 0;
				PPID   goods_id = 0;
				double qtty = 0.0;
				StringSet ss;
				temp_buf = Input;
				temp_buf.Tokenize(":", ss);
				uint p = 0;
				if(ss.get(&p, temp_buf)) {
					cip_id = temp_buf.ShiftLeft(6).ToLong();
					if(ss.get(&p, temp_buf)) {
						goods_id = temp_buf.ToLong();
						if(ss.get(&p, temp_buf))
							qtty = temp_buf.ToReal();
					}
				}
				if(qtty <= 0.0)
					qtty = 1.0;
				if(cip_id && goods_id) {
					LoadChkInP(cip_id, goods_id, qtty);
				}
				ClearInput(0);
			}
			else if(Input == "99999") {
				Sleep();
				ClearInput(0);
			}
			else if(Input.C(0) == '*' || Input.Last() == '*')
				AcceptQuantity();
			else if(Input.IsEqiAscii("$GENERATOR$"))
				GenerateChecks();
			else if(Input.IsEqiAscii("BASKET")) {
				AddFromBasket();
				ClearInput(0);
			}
			else if(Input.IsEqiAscii("TEST")) {
				TestCheck(cpmBank);
				ClearInput(0);
			}
			else if(!oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
				Flags |= fSuspSleepTimeout;
				if(Flags & fPrinted && !(OperRightsFlags & orfChgPrintedCheck))
					MessageError(PPERR_NORIGHTS, 0, eomBeep|eomStatusLine);
				else {
					int    r = -1; // r == 1000 - операция невозможна из-за несоблюдения какого-то условия //
					char   code[256];
					bool   is_serial = false; // !0 если code является подходящим серийным номером
					double qtty = 1.0;
					double price = 0.0;
					PPID   goods_id = 0;
					PPID   loc_id = 0;
					GoodsCodeSrchBlock gcsb;
					Input.CopyTo(gcsb.Code, sizeof(gcsb.Code));
					Input.CopyTo(code, sizeof(code));
					gcsb.Flags |= (GoodsCodeSrchBlock::fAdoptSearch|GoodsCodeSrchBlock::fUse2dTempl);
					if(GObj.SearchByCodeExt(&gcsb) > 0) {
						if(PNP.CnFlags & CASHF_DISABLEZEROAGENT && !P.GetAgentID()) {
							r = 1000;
							MessageError(PPERR_CHKPAN_SALERNEEDED, 0, eomBeep|eomStatusLine);
						}
						else {
							goods_id = gcsb.GoodsID;
							qtty = gcsb.Qtty;
							r = 1;
						}
					}
					else {
						PPObjBill * p_bobj(BillObj);
						PPID   lot_id = 0;
						ReceiptTbl::Rec lot_rec;
						PPIDArray  lot_list;
						if(p_bobj->SearchLotsBySerialExactly(code, &lot_list) > 0) { // @v11.4.2 SearchLotsBySerial-->SearchLotsBySerialExactly
							if(PNP.CnFlags & CASHF_DISABLEZEROAGENT && !P.GetAgentID()) {
								r = 1000;
								MessageError(PPERR_CHKPAN_SALERNEEDED, 0, eomBeep|eomStatusLine);
							}
							else {
								if(ExtCashNodeID && PNP.ExtCnLocID) {
									if(p_bobj->SelectLotFromSerialList(&lot_list, PNP.ExtCnLocID, false/*closedAllowed*/, &lot_id, &lot_rec) > 0 && 
										BelongToExtCashNode(labs(lot_rec.GoodsID))) {
										goods_id = labs(lot_rec.GoodsID);
										price  = lot_rec.Price;
										loc_id = PNP.ExtCnLocID;
										is_serial = true;
										r = 1;
									}
								}
								if(!goods_id && p_bobj->SelectLotFromSerialList(&lot_list, PNP.CnLocID, false/*closedAllowed*/, &lot_id, &lot_rec) > 0) {
									goods_id = labs(lot_rec.GoodsID);
									price  = lot_rec.Price;
									loc_id = PNP.CnLocID;
									is_serial = true;
									r = 1;
								}
							}
						}
					}
					if(r > 0 && r != 1000) {
						PgsBlock pgsb(qtty);
						pgsb.PriceBySerial = price;
						pgsb.Serial = is_serial ? code : 0;
						if(gcsb.Flags & (gcsb.fMarkedCode | gcsb.fChZnCode))
							pgsb.Flags |= PgsBlock::fMarkedBarcode;
						if(gcsb.Flags & gcsb.fChZnCode) {
							if(gcsb.ChZnSerial[0])
								pgsb.ChZnSerial = gcsb.ChZnSerial;
							pgsb.ChZnMark = gcsb.Code;
						}
						if(PreprocessGoodsSelection(goods_id, loc_id, pgsb) > 0)
							SetupNewRow(goods_id, pgsb);
					}
					else if(CsObj.GetEqCfg().Flags & PPEquipConfig::fRecognizeCode) {
						const  PPID acs_id = GetAgentAccSheet();
						PPID   ar_id = 0;
						PPID   reg_type_id = 0;
						if(acs_id && code[0] && PPObjArticle::GetSearchingRegTypeID(acs_id, 0, 0, &reg_type_id) > 0)
							ArObj.SearchByRegCode(acs_id, reg_type_id, code, &ar_id, 0);
						if(ar_id) {
							SetupAgent(ar_id, 0);
							SetupInfo(0);
						}
						//
						// Выбор карты
						//
						else {
							PPIDArray sc_id_list;
							TSVector <SCardSpecialTreatment::IdentifyReplyBlock> scp_rb_list;
							if(ScObj.SearchCodeExt(code, &SpcTrtScsList, sc_id_list, scp_rb_list) > 0) {
								assert(sc_id_list.getCount());
								if(auto_input || !(CsObj.GetEqCfg().Flags & PPEquipConfig::fDisableManualSCardInput)) {
									const  PPID local_sc_id = sc_id_list.get(0);
									AcceptSCard(local_sc_id, GetSpecialTreatmentBlock(scp_rb_list, local_sc_id), 0);
								}
								else
									MessageError(PPERR_MANUALSCARDINPUTDISABLED, 0, eomBeep|eomStatusLine);
							}
							else
								MessageError(PPERR_GDSBYBARCODENFOUND, code, eomBeep|eomMsgWindow);
						}
					}
					else
						MessageError(PPERR_GDSBYBARCODENFOUND, code, eomBeep|eomMsgWindow);
				}
				Flags &= ~fSuspSleepTimeout;
				ClearInput(0);
			}
			else
				ClearInput(0);
		}
		else if(P.HasCur())
			AcceptRow();
		else if(P.getCount()) {
			//
			// Проведение и печать чека
			//
			PosPaymentBlock paym_blk2(0, BonusMaxPart);
			if(PNP.CnExtFlags & CASHFX_DISABLEZEROSCARD && !CSt.GetID())
				MessageError(PPERR_CHKPAN_SCARDNEEDED, 0, eomBeep|eomStatusLine);
			else if(!(OperRightsFlags & orfPrintCheck))
				MessageError(PPERR_NORIGHTS, 0, eomBeep|eomStatusLine);
			else if(!VerifyPrices())
				MessageError(-1, 0, eomBeep|eomStatusLine);
			else if(CalculatePaymentList(paym_blk2, 1) > 0) {
				// Следующий оператор должен следовать после CalculatePaymentList поскольку эта функция обнуляет флаги
				SETFLAG(paym_blk2.Flags, PosPaymentBlock::fCashlessBypassEqEnabled, (PNP.CnExtFlags & CASHFX_ENABLECASHLESSBPEQ)); // @v12.0.6 
				PPID   cc_id = 0;
				CDispCommand(cdispcmdClear, 0, 0.0, 0.0);
				CDispCommand(cdispcmdTotal, 0, paym_blk2.GetTotal(), 0.0);
				if(paym_blk2.GetDiscount() != 0.0)
					CDispCommand(cdispcmdTotalDiscount, 0, paym_blk2.GetPctDiscount(), paym_blk2.GetDiscount());
				switch(paym_blk2.Kind) {
					case cpmCash:
						if(CalcDiff(paym_blk2.AmtToPaym, &paym_blk2.DeliveryAmt) > 0) {
							paym_blk2.NoteAmt = paym_blk2.AmtToPaym + paym_blk2.DeliveryAmt;
							InformCashNoteAndDelivery(H(), paym_blk2);
							CDispCommand(cdispcmdChange, 0, paym_blk2.NoteAmt, paym_blk2.DeliveryAmt);
							AcceptCheck(&cc_id, &paym_blk2.CcPl, 0, paym_blk2.NoteAmt, accmRegular);
						}
						break;
					case cpmBank:
						if(ConfirmPosPaymBank(paym_blk2)) {
							SString bnk_slip_buf;
							int    bnk_paym_result = 1;
							// @v11.9.8 {
							if(!paym_blk2.EAddr.IsEmpty()) {
								P.EAddr = paym_blk2.EAddr;
								P.Paperless = LOGIC(paym_blk2.Flags & PosPaymentBlock::fPaperless);
							}
							else {
								P.EAddr.Z();
								P.Paperless = false;
							}
							// } @v11.9.8 
							if(P_BNKTERM && !((paym_blk2.Flags & PosPaymentBlock::fCashlessBypassEq) && (paym_blk2.Flags & PosPaymentBlock::fCashlessBypassEqEnabled))) {
								const int r = (paym_blk2.AmtToPaym < 0) ? P_BNKTERM->Refund(-paym_blk2.AmtToPaym, bnk_slip_buf) : P_BNKTERM->Pay(paym_blk2.AmtToPaym, bnk_slip_buf);
								if(!r) {
									PPError();
									bnk_paym_result = 0;
								}
							}
							if(bnk_paym_result) {
								PrintBankingSlip(0/*beforeReceipt*/, bnk_slip_buf); // Печатать банковский слип до чека
								AcceptCheck(&cc_id, &paym_blk2.CcPl, 0, paym_blk2.AmtToPaym + paym_blk2.DeliveryAmt, accmRegular);
								PrintBankingSlip(1/*afterReceipt*/, bnk_slip_buf); // Печатать банковский слип после чека
							}
						}
						break;
					case cpmIncorpCrd:
						AcceptCheck(&cc_id, &paym_blk2.CcPl, 0, paym_blk2.AmtToPaym + paym_blk2.DeliveryAmt, accmRegular);
						break;
					case cpmUndef:
						{
							paym_blk2.ExclSCardID = CSt.GetID();
							const double ccpl_total = paym_blk2.CcPl.GetTotal();
							// @v11.3.6 paym_blk2.AltCashReg = AltRegisterID ? 0 : -1;
							SETFLAG(paym_blk2.Flags, PosPaymentBlock::fAltCashRegEnabled, AltRegisterID); // @v11.3.6 
							paym_blk2.Flags &= ~PosPaymentBlock::fAltCashRegUse; // @v11.3.6 
							// @v11.3.6 {
							if(CSt.GetID()) {
								PPSCardPacket sc_pack;
								if(ScObj.GetPacket(CSt.GetID(), &sc_pack) > 0) {
									SString _phone;
									SString _email;
									if(sc_pack.GetExtStrData(PPSCardPacket::extssPhone, _phone) > 0) {
										assert(_phone.NotEmpty());
									}
									if(sc_pack.Rec.PersonID) {
										PPELinkArray ela;
										StringSet ss;
										PersonCore::GetELinks(sc_pack.Rec.PersonID, ela);
										if(_phone.IsEmpty())
											ela.GetSinglePhone(_phone, 0);
										if(ela.GetListByType(ELNKRT_EMAIL, ss) > 0) {
											assert(ss.IsCountGreaterThan(0));
											ss.get(0U, _email);
										}
									}
									if(_email.NotEmpty())
										paym_blk2.SetBuyersEAddr(SNTOK_EMAIL, _email);
									else if(_phone.NotEmpty())
										paym_blk2.SetBuyersEAddr(SNTOK_PHONE, _phone);
								}
							}
							// } @v11.3.6
							for(int _again = 1; _again && paym_blk2.EditDialog2() > 0;) {
								assert(feqeps(paym_blk2.CcPl.GetTotal(), ccpl_total, 0.00001));
								assert(oneof3(paym_blk2.Kind, cpmCash, cpmBank, cpmIncorpCrd));
								// @v11.3.6 {
								if(!paym_blk2.EAddr.IsEmpty()) {
									P.EAddr = paym_blk2.EAddr;
									P.Paperless = LOGIC(paym_blk2.Flags & PosPaymentBlock::fPaperless);
								}
								else {
									P.EAddr.Z();
									P.Paperless = false;
								}
								// } @v11.3.6 
								if(CsObj.GetEqCfg().Flags & PPEquipConfig::fUnifiedPaymentCfmBank &&
									paym_blk2.CcPl.Get(CCAMTTYP_CASH) == 0.0 && !ConfirmPosPaymBank(paym_blk2)) {
									_again = 1;
								}
								else {
									int    bnk_paym_result = 1;
									SString bnk_slip_buf;
									_again = 0;
									// @vmiller {
									if(P_BNKTERM && !((paym_blk2.Flags & PosPaymentBlock::fCashlessBypassEq) && (paym_blk2.Flags & PosPaymentBlock::fCashlessBypassEqEnabled))) { 
										// Здесь не проверяю тип операции, потому что при смешанной оплате в paym_blk2.Kind будет стоять cpmCash
										for(uint i = 0; i < paym_blk2.CcPl.getCount(); i++) {
											if(paym_blk2.CcPl.at(i).Type == CCAMTTYP_BANK) {
												const double bank_amt = paym_blk2.CcPl.at(i).Amount;
												bnk_paym_result = (bank_amt > 0.0) ? P_BNKTERM->Pay(bank_amt, bnk_slip_buf) : P_BNKTERM->Refund(-bank_amt, bnk_slip_buf);
												if(!bnk_paym_result)
													PPError();
												break;
											}
										}
									}
									// } @vmiller
									if(paym_blk2.NoteAmt > 0.0 && paym_blk2.DeliveryAmt > 0.0) {
										CDispCommand(cdispcmdChange, 0, paym_blk2.NoteAmt, paym_blk2.DeliveryAmt);
										InformCashNoteAndDelivery(H(), paym_blk2);
									}
									else
										CDispCommand(cdispcmdChange, 0, paym_blk2.Amount, 0.0);
									if(bnk_paym_result) {
										PrintBankingSlip(0/*beforeReceipt*/, bnk_slip_buf); // Печатать банковский слип до чека
										// @v11.3.6 const  PPID alt_reg_id = (paym_blk2.AltCashReg > 0) ? AltRegisterID : 0;
										const  PPID alt_reg_id = ((paym_blk2.Flags & PosPaymentBlock::fAltCashRegUse) && (paym_blk2.Flags & PosPaymentBlock::fAltCashRegEnabled)) ? AltRegisterID : 0; // @v11.3.6
										AcceptCheck(&cc_id, &paym_blk2.CcPl, alt_reg_id, paym_blk2.NoteAmt, accmRegular);
										PrintBankingSlip(1/*afterReceipt*/, bnk_slip_buf); // Печатать банковский слип после чека
									}
								}
							}
						}
						break;
				}
			}
		}
	}
	if(GetState() != prev_state || P.GetAgentID(1) != prev_agent_id)
		setupHint();
}

int CheckPaneDialog::Sleep()
{
	int    ok = -1;
	if(!(Flags & fSleepMode)) {
		Flags |= fSleepMode;
		TDialog * dlg = new TDialog(DLG_CHKPANSLEEP); // @newok
		if(CheckDialogPtrErr(&dlg)) {
			SString code;
			while(1) {
				if(ExecView(dlg) == cmOK) {
					dlg->getCtrlString(CTL_CHKPANSLEEP_CODE, code);
					int    r = SetupSalByCode(code);
					if(r != 1 && CsObj.GetEqCfg().Flags & PPEquipConfig::fRecognizeCode) {
						PPID   ar_id = 0;
						PPID   reg_type_id = 0;
						const  PPID agent_acs_id = GetAgentAccSheet();
						if(agent_acs_id && code[0] && PPObjArticle::GetSearchingRegTypeID(agent_acs_id, 0, 0, &reg_type_id) > 0)
							ArObj.SearchByRegCode(agent_acs_id, reg_type_id, code, &ar_id, 0);
						if(ar_id) {
							SetupAgent(ar_id, 0);
							SetupInfo(0);
							r = 1;
						}
					}
					if(r == 1 || code.Cmp("99990", 0) == 0) {
						Flags &= ~fSleepMode;
						IdleClock = clock();
						break;
					}
				}
			}
		}
		delete dlg;
		ok = 1;
	}
	return ok;
}

int FASTCALL CheckPaneDialog::Barrier(int rmv)
{
	if(Flags & fBarrier) {
		if(rmv)
			Flags &= ~fBarrier;
		else
			BarrierViolationCounter++;
		return 1;
	}
	else {
		if(!rmv)
			Flags |= fBarrier;
		return 0;
	}
}

class ComplexDinnerDialog : public PPListDialog {
	DECL_DIALOG_DATA(SaComplex);
public:
	ComplexDinnerDialog(PPID locID) : PPListDialog(DLG_COMPLDIN, CTL_COMPLDIN_ELEMENTS), LocID(locID)
	{
		{
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_COMPLDIN_ALTLIST));
			if(!SetupStrListBox(p_list))
				PPError();
			setSmartListBoxOption(CTL_COMPLDIN_ALTLIST,  lbtSelNotify);
			setSmartListBoxOption(CTL_COMPLDIN_ELEMENTS, lbtFocNotify);
		}
		Ptb.SetColor(clrFocus,  RGB(0x20, 0xAC, 0x90));
		Ptb.SetColor(clrUnsel,  RGB(0xDA, 0xD7, 0xD0));
		Ptb.SetBrush(brSel,     SPaintObj::psSolid, Ptb.GetColor(clrFocus), 0);
		Ptb.SetBrush(brOdd,     SPaintObj::psSolid, Ptb.GetColor(clrOdd), 0);
		Ptb.SetBrush(brUnsel,   SPaintObj::psSolid, Ptb.GetColor(clrUnsel), 0);
		{
		 	SString temp_buf;
			LOGFONT log_font;
			MEMSZERO(log_font);
			log_font.lfCharSet = DEFAULT_CHARSET;
			ListEntryGap = 5;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIAL, temp_buf);
			STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf)); // @unicodeproblem
			log_font.lfHeight = (DEFAULT_TS_FONTSIZE - TSGGROUPSASITEMS_FONTDELTA);
			Ptb.SetFont(fontList, ::CreateFontIndirect(&log_font));
		}
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		Data.RecalcFinalPrice();
		updateList(-1);
		enableCommand(cmOK, Data.IsComplete());
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = -1;
		if(Data.IsComplete()) {
			ASSIGN_PTR(pData, Data);
			ok = 1;
		}
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	void   DrawListItem(TDrawItemData *);

	enum {
		dummyFirst = 1,
		fontList,
		brSel,
		brOdd,
		brUnsel,
		brGrp,
		clrFocus,
		clrOdd,
		clrUnsel
	};
	long   ListEntryGap;
	PPID   LocID;
	SPaintToolBox Ptb;
	PPObjGoods GObj;
};

IMPL_HANDLE_EVENT(ComplexDinnerDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmDrawItem))
		DrawListItem(static_cast<TDrawItemData *>(TVINFOPTR));
	else if(event.isCmd(cmSetupResizeParams)) {
		PPID   sb_id_altlist  = MAKE_BUTTON_ID(CTL_COMPLDIN_ALTLIST, 1);
		PPID   sb_id_elements = MAKE_BUTTON_ID(CTL_COMPLDIN_ELEMENTS, 1);
		SString font_face;

		PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIAL, font_face);
		SetCtrlFont(CTL_COMPLDIN_ELEMENTS, font_face, 26);
		SetCtrlFont(CTL_COMPLDIN_ALTLIST, font_face, 26);
		SetCtrlFont(CTL_COMPLDIN_TOTAL, font_face, 26);

		SetCtrlResizeParam(CTL_COMPLDIN_ELEMENTS, 0, 0, 0, CTL_COMPLDIN_TOTAL, crfResizeable);
		//SetCtrlResizeParam(sb_id_elements, CTL_COMPLDIN_ELEMENTS, 0, 0, CTL_COMPLDIN_TOTAL, crfResizeable);
		SetCtrlResizeParam(CTL_COMPLDIN_TOTAL, 0, CTL_COMPLDIN_ELEMENTS, 0, CTL_COMPLDIN_ALTLIST, crfResizeable);
		SetCtrlResizeParam(CTL_COMPLDIN_ALTLIST, 0, CTL_COMPLDIN_TOTAL, 0, 0, crfResizeable);
		//SetCtrlResizeParam(sb_id_altlist, CTL_COMPLDIN_ALTLIST, CTL_COMPLDIN_TOTAL, 0, BTN_COMPLDIN_INPUTQTTY, crfResizeable);
		SetCtrlResizeParam(BTN_COMPLDIN_INPUTQTTY, 0, CTL_COMPLDIN_ALTLIST, 0, 0, crfResizeable);
		SetCtrlResizeParam(STDCTL_OKBUTTON, BTN_COMPLDIN_INPUTQTTY, CTL_COMPLDIN_ALTLIST, 0, 0, crfResizeable);
		SetCtrlResizeParam(STDCTL_CANCELBUTTON, STDCTL_OKBUTTON, CTL_COMPLDIN_ALTLIST, 0, 0, crfResizeable);
		ResizeDlgToFullScreen();
	}
	else if(event.isCmd(cmInputQtty)) {
		double qtty = 1.0;
		if(InputQttyDialog(0, 0, &qtty) > 0) {
			Data.SetQuantity(qtty);
			updateList(-1);
		}
	}
	else if(event.isCmd(cmLBItemFocused) && event.isCtlEvent(CTL_COMPLDIN_ELEMENTS)) {
		SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_COMPLDIN_ALTLIST));
		if(p_box) {
			p_box->freeAll();
			long   pos = 0;
			getSelection(&pos);
			if(pos > 0 && pos <= Data.getCountI()) {
				SaComplexEntry & r_entry = Data.at(pos-1);
				if(r_entry.Flags & SaComplexEntry::fGeneric) {
					SString temp_buf;
					StringSet ss(SLBColumnDelim);
					long   focus_pos = 0;
					for(uint i = 0; i < r_entry.GenericList.getCount(); i++) {
						const  PPID goods_id = r_entry.GenericList.at(i).Key;
						ss.Z();
						ss.add(GetGoodsName(goods_id, temp_buf));
						ss.add(temp_buf.Z().Cat(r_entry.GenericList.at(i).Val, SFMT_MONEY));
						p_box->addItem(i+1, ss.getBuf());
						if(r_entry.FinalGoodsID == goods_id)
							focus_pos = static_cast<long>(i);
					}
					p_box->focusItem(focus_pos);
				}
				p_box->Draw_();
			}
		}
	}
	else if(event.isCmd(cmLBItemSelected) && event.isCtlEvent(CTL_COMPLDIN_ALTLIST)) {
		SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_COMPLDIN_ALTLIST));
		if(p_box) {
			long   main_pos = 0;
			long   subst_pos = 0;
			getSelection(&main_pos);
			p_box->getCurID(&subst_pos);
			if(main_pos > 0 && subst_pos > 0 && Data.Subst((uint)(main_pos-1), (uint)(subst_pos-1)))
				updateList(main_pos-1);
		}
		enableCommand(cmOK, Data.IsComplete());
	}
	else
		return;
	clearEvent(event);
}

void ComplexDinnerDialog::DrawListItem(TDrawItemData * pDrawItem)
{
	if(pDrawItem && pDrawItem->P_View) {
		const PPID list_ctrl_id = pDrawItem->P_View->GetId();
		if(list_ctrl_id == CTL_COMPLDIN_ELEMENTS) {
			HDC    h_dc = pDrawItem->H_DC;
			HFONT  h_fnt_def  = 0;
			HBRUSH h_br_def   = 0;
			HPEN   h_pen_def  = 0;
			COLORREF clr_prev = 0;
			SmartListBox * p_lbx = static_cast<SmartListBox *>(pDrawItem->P_View);
			RECT   rc = pDrawItem->ItemRect;
			SString temp_buf;
			if(pDrawItem->ItemAction & TDrawItemData::iaBackground) {
				::FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(brOdd)));
				pDrawItem->ItemAction = 0; // Мы перерисовали фон
			}
			else if(pDrawItem->ItemID != 0xffffffff) {
				h_fnt_def = static_cast<HFONT>(::SelectObject(h_dc, static_cast<HFONT>(Ptb.Get(fontList))));
				p_lbx->getText((long)pDrawItem->ItemData, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				if(pDrawItem->ItemState & (ODS_FOCUS|ODS_SELECTED)) {
					h_br_def = static_cast<HBRUSH>(SelectObject(h_dc, Ptb.Get(brSel)));
					clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrFocus));
					SInflateRect(rc, -1, -(1 + ListEntryGap / 4));
					RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
					rc.left += 4;
				}
				else {
					int  draw_odd = pDrawItem->ItemID % 2;
					clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrUnsel));
					h_br_def = static_cast<HBRUSH>(::SelectObject(h_dc, Ptb.Get(brUnsel)));
					SInflateRect(rc, -1, -(1 + ListEntryGap / 4));
					RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
					rc.left += 4;
				}
				::DrawText(h_dc, SUcSwitch(temp_buf), (int)temp_buf.Len(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE); // @unicodeproblem
			}
		}
		else
			pDrawItem->ItemAction = 0; // Список не активен - строку не рисуем
	}
}

int ComplexDinnerDialog::setupList()
{
	int    ok = 1;
	double total = 0.0;
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; i < Data.getCount(); i++) {
		const SaComplexEntry & r_entry = Data.at(i);
		const  PPID goods_id = NZOR(r_entry.FinalGoodsID, r_entry.GoodsID);
		ss.Z();
		ss.add(GetGoodsName(goods_id, temp_buf));
		ss.add(temp_buf.Z().Cat(r_entry.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)));
		ss.add(temp_buf.Z().Cat(r_entry.OrgPrice, SFMT_MONEY));
		ss.add(temp_buf.Z().Cat(r_entry.FinalPrice, SFMT_MONEY));
		total += (r_entry.FinalPrice * r_entry.Qtty);
		addStringToList(i+1, ss.getBuf());
	}
	setCtrlReal(CTL_COMPLDIN_TOTAL, total);
	return ok;
}

int CheckPaneDialog::InputComplexDinner(SaComplex & rComplex) { DIALOG_PROC_BODY_P1(ComplexDinnerDialog, PNP.CnLocID, &rComplex); }
//
// SelCheckListDialog
//
struct _SelCheck {
	_SelCheck() : CheckID(0), BillID(0), Flags(0), CcOp(CCOP_GENERAL)
	{
	}
	enum {
		fUnfinished = 0x0001, // Структура возвращает неотпечатанный чек
		// @v12.2.11 fCorrection = 0x0002, // @v12.2.8 Была выбрана операция коррекции чека (из пары возврат/коррекция)
	};
	PPID    CheckID;
	PPID    BillID; // @v11.8.8
	long    Flags;
	int     CcOp;   // @v12.2.11 CCOP_XXX
	SString SelFormat;
};

class SelCheckListDialog : public PPListDialog {
public:
	struct AddedParam {
		AddedParam(PPID nodeID, long tableCode, PPID agentID, long rights) : NodeID(nodeID), TableCode(tableCode), AgentID(agentID), Rights(rights), Flags(0)
		{
		}
		enum {
			fAllowReturns          = 0x0001,
			fUnfinished            = 0x0002, // Выбор неотпечатанных чеков
			fRetOrCorrSelection    = 0x0004, // @v12.2.8 Допускается переключение установки операции (возврат или коррекция) по выбранному чеку
		};
		PPID   NodeID;
		long   TableCode;
		PPID   AgentID;
		long   Rights;       // Права доступа из кассовой панели
		long   Flags;        //
		SString FormatName;
	};
public:
	SelCheckListDialog(uint dlgId, int selectFormat, PPCashMachine * pCm, CPosProcessor * pSrv, const AddedParam * pAddParam = 0);
	SelCheckListDialog(uint dlgId, const TSVector <CCheckViewItem> * pChkList, int selToUnite, CPosProcessor * pSrv, const AddedParam * pAddParam = 0);
	//
	// Descr: Специализированный конструктор, предназначенный для формирования списка выбора документа
	//   для превращения в чек.
	//
	SelCheckListDialog(uint dlgId, const PPIDArray & rBillIdList, CPosProcessor * pSrv, const AddedParam * pAddParam = 0);
	~SelCheckListDialog();
	int    getDTS(_SelCheck * pSelCheck);
	void   SetList(const TSVector <CCheckViewItem> & rChkList);
private:
	void   Helper_Constructor(CPosProcessor * pSrv, const AddedParam * pAddParam);
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	int    SetupItemList();
	int    SplitCheck();
	int    UniteChecks();
	void   Init(PPCashMachine * pCm);

	CPosProcessor * P_Srv;
	PPObjGoods GObj;
	PPObjLocation LocObj;
	CTableOrder * P_Cto;
	StrAssocArray FmtList;
	TSVector <CCheckViewItem> ChkList;
	TSVector <CCheckViewItem> PreserveOuterChkList;
	PPBillPacketCollection BPackList; // @v11.8.7 key: Rec.ID
	PPID   LastTopID/*LastChkID*/; // Последний активный идентификатор объекта (чека или документа) в основном списке
	long   LastChkNo;
	LDATE  LastDate;
	long   LastOp; // @v12.2.8 CPosProcessor::opXXX
	long   Op; // @v12.2.8 CPosProcessor::opXXX

	enum {
		stTblOrders        = 0x0001,
		stInputUpdated     = 0x0002,
		stListUpdated      = 0x0004,
		stOuterList        = 0x0008,
		stSelectFormat     = 0x0010,
		stSelToUnite       = 0x0020,
		stSelectSlipFormat = 0x0040,
		stSelectUnfinished = 0x0080, // Режим выбора неотпечатанных чеков
		stSelectBill       = 0x0100, // @v11.8.7 Режим выбора документа 
	};
	long   State;
	const  AddedParam * P_AddParam; // @notowned
};

void SelCheckListDialog::Helper_Constructor(CPosProcessor * pSrv, const AddedParam * pAddParam)
{
	Op = CCOP_GENERAL;
	P_Srv = pSrv;
	assert(P_Srv);
	P_Cto = 0;
	P_AddParam  = pAddParam;
	State = 0;
	setSmartListBoxOption(CTL_SELCHECK_LIST, lbtSelNotify);
	setSmartListBoxOption(CTL_SELCHECK_LIST, lbtFocNotify);
	if(oneof2(this->Id, DLG_ORDERCHECKS, DLG_ORDERCHECKS_L)) {
		State |= stTblOrders;
		P_Cto = new CTableOrder;
		enableCommand(cmaInsert, P_Cto && P_Cto->HasRight(PPR_INS));
		enableCommand(cmaEdit,   P_Cto && P_Cto->HasRight(PPR_MOD));
		enableCommand(cmaDelete, P_Cto && P_Cto->HasRight(PPR_DEL));
	}
	else if(P_AddParam && P_AddParam->Flags & AddedParam::fUnfinished) {
		State |= stSelectUnfinished;
	}
	// @v12.2.8 {
	if(P_AddParam && (P_AddParam->Flags & AddedParam::fRetOrCorrSelection)) {
		const bool correction_enabled = SlDebugMode::CT(); // @v12.2.10 Пока блокируем возможность выбора коррекции - надо еще поработать над механизмами.
		if(correction_enabled)
			Op = CCOP_RETURN;
		else {
			if(Op == CCOP_GENERAL)
				Op = CCOP_RETURN;
		}
		AddClusterAssocDef(CTL_SELCHECK_OP, 0, CCOP_RETURN);
		AddClusterAssocDef(CTL_SELCHECK_OP, 1, CCOP_CORRECTION_SELL);
		AddClusterAssocDef(CTL_SELCHECK_OP, 2, CCOP_CORRECTION_SELLSTORNO); // @v12.2.11
		AddClusterAssocDef(CTL_SELCHECK_OP, 3, CCOP_CORRECTION_RET); // @v12.2.11
		AddClusterAssocDef(CTL_SELCHECK_OP, 4, CCOP_CORRECTION_RETSTORNO); // @v12.2.11
		SetClusterData(CTL_SELCHECK_OP, Op);
		DisableClusterItem(CTL_SELCHECK_OP, 1, !correction_enabled);
		DisableClusterItem(CTL_SELCHECK_OP, 2, !correction_enabled);
		DisableClusterItem(CTL_SELCHECK_OP, 3, !correction_enabled);
		DisableClusterItem(CTL_SELCHECK_OP, 4, !correction_enabled);
	}
	else {
		showCtrl(CTL_SELCHECK_OP, false);
	}
	// } @v12.2.8 
	AddClusterAssoc(CTL_SELCHECK_UNFC, 0, stSelectUnfinished);
	SetClusterData(CTL_SELCHECK_UNFC, State);
}

SelCheckListDialog::SelCheckListDialog(uint dlgId, int selectFormat, PPCashMachine * pCm, CPosProcessor * pSrv, const AddedParam * pAddParam/*=0*/) :
	PPListDialog(dlgId, CTL_SELCHECK_LIST)
{
	Helper_Constructor(pSrv, pAddParam);
	SETFLAG(State, stSelectFormat, selectFormat);
	SETFLAG(State, stSelectSlipFormat, (selectFormat > 0));
	Init(pCm);
}
	
SelCheckListDialog::SelCheckListDialog(uint dlgId, const TSVector <CCheckViewItem> * pChkList, int selToUnite, CPosProcessor * pSrv, const AddedParam * pAddParam/*=0*/) :
	PPListDialog(dlgId, CTL_SELCHECK_LIST)
{
	Helper_Constructor(pSrv, pAddParam);
	State |= stOuterList;
	SETFLAG(State, stSelToUnite, selToUnite);
	if(pChkList) {
		assert(ChkList.getItemSize() == pChkList->getItemSize());
		PreserveOuterChkList = *pChkList;
		ChkList = PreserveOuterChkList;
	}
	Init(0);
}

// @v11.8.7
SelCheckListDialog::SelCheckListDialog(uint dlgId, const PPIDArray & rBillIdList, CPosProcessor * pSrv, const AddedParam * pAddParam/*=0*/) :
	PPListDialog(dlgId, CTL_SELCHECK_LIST)
{
	Helper_Constructor(pSrv, pAddParam);
	State |= stSelectBill;
	if(rBillIdList.getCount()) {
		PPObjBill * p_bobj(BillObj);
		for(uint i = 0; i < rBillIdList.getCount(); i++) {
			const  PPID bill_id = rBillIdList.get(i);
			BillTbl::Rec bill_rec;
			if(p_bobj->Fetch(bill_id, &bill_rec) > 0) {
				uint   new_bpack_idx = 0;
				PPBillPacket * p_new_bpack = BPackList.CreateNewItem(&new_bpack_idx);
				if(p_new_bpack) {
					if(p_bobj->ExtractPacket(bill_id, p_new_bpack) > 0) {
						;
					}
					else
						BPackList.atFree(new_bpack_idx);
				}
			}
		}
	}
	Init(0);
}
	
SelCheckListDialog::~SelCheckListDialog()
{
	delete P_Cto;
}

void SelCheckListDialog::Init(PPCashMachine * pCm)
{
	enableCommand(cmSplitCheck, 0);
	enableCommand(cmUniteChecks, 0);
	DisableClusterItem(CTL_SELCHECK_UNFC, 0, P_Srv ? !P_Srv->CheckRights(CPosProcessor::orfReprnUnfCc) : 1);
	LastDate = getcurdate_();
	SetupCalDate(CTLCAL_SELCHECK_DATE, CTL_SELCHECK_DATE);
	setCtrlData(CTL_SELCHECK_DATE, &LastDate);
	if(!SetupStrListBox(this, CTL_SELCHECK_ITEMLIST))
		PPError();
	FmtList.Z();
	LastChkNo = -1;
	LastTopID = 0;
	LastOp = CCOP_GENERAL; // @v12.2.8
	State &= ~(stInputUpdated | stListUpdated);
	if(State & stSelectBill) {
		if(P_Box) {
			P_Box->RemoveColumns();
			P_Box->AddColumn(-1, "@date",       10, 0, 0);
			P_Box->AddColumn(-1, "@code",       14, 0, 0);
			P_Box->AddColumn(-1, "@contractor", 20, 0, 0);
			P_Box->AddColumn(-1, "@amount",      8, ALIGN_RIGHT, 0);
			P_Box->AddColumn(-1, "@memo",       30, 0, 0);
		}
	}
	updateList(-1);
	SetupItemList();
	if(State & stSelectFormat) {
		ListWindow * p_lw = 0;
		ComboBox   * p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_SELCHECK_FORMAT));
		if(p_cb && pCm && pCm->GetSlipFormatList(&FmtList, BIN(State & stSelectSlipFormat)) > 0) {
			ListWindow * p_lw = new ListWindow(new StrAssocListBoxDef(&FmtList, /*lbtDisposeData |*/ lbtDblClkNotify));
			long   fmt_id = 0;
			if(FmtList.getCount() == 1)
				fmt_id = FmtList.Get(0).Id;
			else if(FmtList.getCount() > 1 && P_AddParam && P_AddParam->FormatName.NotEmpty()) {
				for(uint i = 0; !fmt_id && i < FmtList.getCount(); i++) {
					StrAssocArray::Item fmt_list_item = FmtList.Get(i);
					if(P_AddParam->FormatName.CmpNC(fmt_list_item.Txt) == 0)
						fmt_id = fmt_list_item.Id;
				}
			}
			p_cb->setListWindow(p_lw, fmt_id);
		}
	}
	else {
		showCtrl(CTL_SELCHECK_FORMAT, false);
		showCtrl(CTLSEL_SELCHECK_FORMAT, false);
	}
}

void SelCheckListDialog::SetList(const TSVector <CCheckViewItem> & rChkList)
{
	if(State & stOuterList) {
		PreserveOuterChkList = rChkList;
		ChkList = rChkList;
		Init(0);
	}
}

int SelCheckListDialog::getDTS(_SelCheck * pSelCheck)
{
	int    ok = 1;
	ushort sel = CTL_SELCHECK_CODE;
	_SelCheck sel_chk;
	if(State & stSelectBill) {
		THROW_PP(BPackList.getCount(), PPERR_CHECKNOTFOUND);
		{
			PPID bill_id = 0;
			getCurItem(0, &bill_id);
			BillTbl::Rec bill_rec;
			if(BillObj->Search(bill_id, &bill_rec) > 0) {
				sel_chk.BillID = bill_id;
			}
		}
	}
	else {
		if(!(State & stOuterList)) {
			LDATE  dt = ZERODATE;
			long   fmt_id = getCtrlLong(CTL_SELCHECK_FORMAT);
			getCtrlData(sel = CTL_SELCHECK_DATE, &dt);
			THROW_PP(dt || (State & stSelectFormat), PPERR_CHKDATENEEDED);
			THROW_SL(checkdate(dt));
			if(fmt_id) {
				StrAssocArray::Item fmt_item = FmtList.Get(static_cast<uint>(fmt_id - 1));
				if(fmt_item.Txt[0])
					sel_chk.SelFormat = fmt_item.Txt;
			}
		}
		sel = 0;
		THROW_PP(ChkList.getCount() || (State & stSelectFormat), PPERR_CHECKNOTFOUND);
		getCurItem(0, &sel_chk.CheckID);
		if(sel_chk.CheckID) {
			// @v12.2.8 {
			/*@v12.2.11 if(Op == CCOP_CORRECTION_SELL) {
				sel_chk.Flags |= _SelCheck::fCorrection;
			}*/
			// } @v12.2.8
			sel_chk.CcOp = Op; // @v12.2.11
			if(State & stSelectUnfinished) {
				CCheckTbl::Rec cc_rec;
				if(P_Srv->GetCc().Search(sel_chk.CheckID, &cc_rec) > 0)
					sel_chk.Flags |= _SelCheck::fUnfinished;
			}
		}
	}
	CATCHZOKPPERRBYDLG
	ASSIGN_PTR(pSelCheck, sel_chk);
	return ok;
}

/*virtual*/int SelCheckListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	if(State & stTblOrders) {
		if(SETIFZ(P_Cto, new CTableOrder)) {
			CTableOrder::Param param;
			if(P_AddParam)
				param.PosNodeID = P_AddParam->NodeID;
			if(param.PosNodeID && P_Cto->Create(&param) > 0) {
				ASSIGN_PTR(pPos, -1);
				ASSIGN_PTR(pID, -1);
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/int SelCheckListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if((State & stTblOrders) && pos >= 0 && pos < ChkList.getCountI()) {
		PPID   check_id = ChkList.at(pos).ID;
		if(check_id) {
			if(SETIFZ(P_Cto, new CTableOrder)) {
				CTableOrder::Packet pack;
				if(P_Cto->GetCheck(check_id, &pack) > 0 && P_Cto->Edit(&pack) > 0) {
					ok = P_Cto->Update(&pack, 1);
					if(!ok)
						PPError();
				}
			}
		}
	}
	return ok;
}

/*virtual*/int SelCheckListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if((State & stTblOrders) && pos >= 0 && pos < ChkList.getCountI() && CONFIRM(PPCFM_CANCELCTBLORD)) {
		if(SETIFZ(P_Cto, new CTableOrder)) {
			if(!P_Cto->Cancel(ChkList.at(pos).ID))
				ok = PPErrorZ();
			else {
				ChkList.atFree(pos);
				ok = 1;
			}
		}
	}
	return ok;
}

/*virtual*/int SelCheckListDialog::setupList()
{
	int    ok = -1;
	CCheckCore & r_cc = P_Srv->GetCc();
	SString temp_buf;
	StringSet ss(SLBColumnDelim);
	if(State & stSelectBill) {
		PPObjBill * p_bobj(BillObj);
		for(uint i = 0; i < BPackList.getCount(); i++) {
			const PPBillPacket * p_bpack = BPackList.at(i);
			if(p_bpack) {
				/*
				P_Box->AddColumn(-1, "@date",       10, 0, 1);
				P_Box->AddColumn(-1, "@code",       14, 0, 2);
				P_Box->AddColumn(-1, "@contractor", 20, 0, 3);
				P_Box->AddColumn(-1, "@amount",      8, ALIGN_RIGHT, 4);
				P_Box->AddColumn(-1, "@memo",       30, 0, 5);				
				*/
				ss.Z();
				ss.add(temp_buf.Z().Cat(p_bpack->Rec.Dt, DATF_DMY));
				ss.add(temp_buf.Z().Cat(p_bpack->Rec.Code));
				temp_buf.Z();
				if(p_bpack->Rec.Object) {
					GetArticleName(p_bpack->Rec.Object, temp_buf);
				}
				ss.add(temp_buf);
				{
					// @v11.8.10 Сумму в списке лучше видеть в ценах реализации, нежели номинал (который может оказаться, например, суммой в ценах поступления)
					const double _ap = p_bpack->Amounts.Get(PPAMT_SELLING, p_bpack->Rec.CurID);
					const double _ad = p_bpack->Amounts.Get(PPAMT_DISCOUNT, p_bpack->Rec.CurID);
					const double _apd = (_ap - _ad);
					ss.add(temp_buf.Z().Cat((_apd > 0.0) ? _apd : p_bpack->Rec.Amount, MKSFMTD_020));
				}
				ss.add(p_bpack->SMemo);
				//
				THROW(addStringToList(p_bpack->Rec.ID, ss.getBuf()));
			}
		}
		if((State & stListUpdated) || BPackList.getCount())
			ok = 1;
	}
	else {
		if(State & stTblOrders)
			P_Srv->GetTblOrderList(LastDate, ChkList);
		else {
			const  bool unprinted_only = LOGIC(State & stSelectUnfinished);
			if(!(State & stOuterList) || unprinted_only) {
				State &= ~stListUpdated;
				long   chk_no = getCtrlLong(CTL_SELCHECK_CODE);
				LDATE  dt = getCtrlView(CTL_SELCHECK_DATE) ? getCtrlDate(CTL_SELCHECK_DATE) : ZERODATE;
				if(!checkdate(dt))
					dt = unprinted_only ? getcurdate_() : ZERODATE;
				if(chk_no || dt) {
					if(chk_no != LastChkNo || diffdate(dt, LastDate) || Op != LastOp) {
						TSVector <CCheckTbl::Rec> temp_list;
						StrAssocArray cc_fld_list;
						PPExtStrContainer sc;
						THROW(r_cc.SearchByDateAndCode(chk_no, dt, unprinted_only, &temp_list));
						{
							uint i = temp_list.getCount();
							if(i) do {
								CCheckTbl::Rec & r_rec = temp_list.at(--i);
								const double cc_amt = MONEYTOLDBL(r_rec.Amount);
								bool  do_remove = false;
								if(r_rec.Flags & CCHKF_SKIP)
									do_remove = true;
								else if(P_AddParam && P_AddParam->NodeID && r_rec.PosNodeID != P_AddParam->NodeID)
									do_remove = true;
								else if(cc_amt == 0.0)
									do_remove = true;
								else if(cc_amt < 0.0) {
									do_remove = !(P_AddParam && P_AddParam->Flags & P_AddParam->fAllowReturns);
								}
								if(!do_remove && CPosProcessor::IsCorrectionOp(Op)) { 
									PPRef->UtrC.SearchUtf8(TextRefIdent(PPOBJ_CCHECK, r_rec.ID, PPTRPROP_CC_LNEXT), temp_buf);
									temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									CCheckPacket::Helper_UnpackTextExt(temp_buf, &sc, &cc_fld_list);
									if(sc.GetExtStrData(CCheckPacket::extssFiscalSign, temp_buf) > 0) {
										if(oneof2(Op, CCOP_CORRECTION_RET, CCOP_CORRECTION_RETSTORNO) && !(r_rec.Flags & CCHKF_RETURN)) {
											do_remove = true;
										}
										else if(oneof2(Op, CCOP_CORRECTION_SELL, CCOP_CORRECTION_SELLSTORNO) && (r_rec.Flags & CCHKF_RETURN)) {
											do_remove = true;
										}
									}
									else
										do_remove = true;
								}
								if(do_remove)
									temp_list.atFree(i);
							} while(i);
						}
						{
							ChkList.clear();
							for(uint i = 0; i < temp_list.getCount(); i++) {
								const CCheckTbl::Rec & r_rec = temp_list.at(i);
								CCheckViewItem item;
								*static_cast<CCheckTbl::Rec *>(&item) = r_rec;
								if(item.Flags & CCHKF_EXT) {
									CCheckExtTbl::Rec ext_rec;
									if(r_cc.GetExt(r_rec.ID, &ext_rec) > 0) {
										item.TableCode  = ext_rec.TableNo;
										item.GuestCount = ext_rec.GuestCount;
										item.AgentID    = ext_rec.SalerID;
										item.LinkCheckID = ext_rec.LinkCheckID;
										if(item.Flags & CCHKF_ORDER)
											item.OrderTime.Init(ext_rec.StartOrdDtm, ext_rec.EndOrdDtm);
									}
								}
								ChkList.insert(&item);
							}
						}
						State |= stListUpdated;
					}
				}
				else if(ChkList.getCount()) {
					ChkList.freeAll();
					State |= stListUpdated;
				}
				LastChkNo = chk_no;
				LastDate  = dt;
				LastOp = Op; // @v12.2.8
			}
		}
		if(ChkList.getCount()) {
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SELCHECK_LIST));
			SString scard_psn;
			SString scard_no;
			for(uint i = 0; i < ChkList.getCount(); i++) {
				const CCheckViewItem & r_chk_rec = ChkList.at(i);
				ss.Z();
				LDATETIME dtm;
				dtm.Set(r_chk_rec.Dt, r_chk_rec.Tm);
				if(!(State & stTblOrders)) {
					ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS));
					/*
					if(p_item->Flags & cifGift)
						p_list->def->SetItemColor(i, SClrBlack, SClrGreen);
					*/
					if(r_chk_rec.Flags & CCHKF_JUNK)
						p_list->P_Def->SetItemColor(r_chk_rec.ID, SClrBlack, SClrOrange);
					else if(r_chk_rec.Flags & CCHKF_IMPORTED) // @v11.8.6
						p_list->P_Def->SetItemColor(r_chk_rec.ID, SClrBlack, SClrPalegreen);
					else if(!(r_chk_rec.Flags & CCHKF_SUSPENDED))
						p_list->P_Def->SetItemColor(r_chk_rec.ID, SClrBlack, SClrYellow);
					{
						temp_buf.Z();
						if(r_chk_rec.Flags & CCHKF_JUNK)
							temp_buf.CatDiv('*', 2);
						ss.add(temp_buf.Cat(r_chk_rec.Code));
					}
					ss.add(temp_buf.Z().Cat(MONEYTOLDBL(r_chk_rec.Amount), SFMT_MONEY));
					{
						temp_buf.Z();
						if(r_chk_rec.TableCode)
							temp_buf.Cat(r_chk_rec.TableCode);
						/* @v11.8.8 else if(r_chk_rec.Flags & CCHKF_IMPORTED)
							temp_buf.Cat("IMP");*/ // @v11.8.6 "UHTT"-->"IMP"
						ss.add(temp_buf);
					}
					GetArticleName(r_chk_rec.AgentID, temp_buf);
					ss.add(temp_buf);
				}
				else {
					// @lbt_tblordlist        "10,R,Стол;26,R,Время заказа;10,L,Карта;19,L,Владелец карты;14,R,Предоплата;18,L,Дата/время;10,L,№ чека"
					// @lbt_tblordlist_l      "20,R,Стол;35,R,Время заказа;20,L,Карта;30,L,Владелец карты;25,R,Предоплата;36,L,Дата/время;20,L,№ чека"
					STimeChunk tm_chunk;
					CCheckExtTbl::Rec ext_chk_rec;
					SCardTbl::Rec sc_rec;
					scard_no.Z();
					scard_psn.Z();
					r_cc.GetExt(r_chk_rec.ID, &ext_chk_rec);
					{
						temp_buf.Z();
						if(r_chk_rec.TableCode)
							temp_buf.Cat(r_chk_rec.TableCode);
						else if(r_chk_rec.Flags & CCHKF_IMPORTED)
							temp_buf.Cat("IMP"); // @v11.8.7 "UHTT"-->"IMP"
						ss.add(temp_buf);
					}
					tm_chunk.Init(ext_chk_rec.StartOrdDtm, ext_chk_rec.EndOrdDtm);
					ss.add(tm_chunk.ToStr(STimeChunk::fmtOmitSec, temp_buf.Z()));
					if(r_chk_rec.SCardID && P_Srv->GetScObj().Fetch(r_chk_rec.SCardID, &sc_rec) > 0) {
						scard_no = sc_rec.Code;
						if(sc_rec.PersonID)
							GetPersonName(sc_rec.PersonID, scard_psn);
					}
					ss.add(scard_no);
					ss.add(scard_psn);
					{
						CCheckPacket chk_pack;
						temp_buf.Z();
						if(r_cc.LoadPacket(r_chk_rec.ID, 0, &chk_pack) > 0) {
							if(chk_pack.GetCount()) {
								const CCheckLineTbl::Rec & cclr = chk_pack.GetLineC(0);
								temp_buf.Cat(intmnytodbl(cclr.Price));
							}
						}
						ss.add(temp_buf);
					}
					ss.add(temp_buf.Z().Cat(dtm, DATF_DMY, TIMF_HMS));
					ss.add(temp_buf.Z().Cat(r_chk_rec.Code));
				}
				THROW(addStringToList(r_chk_rec.ID, ss.getBuf()));
			}
		}
		if((State & stListUpdated) || ChkList.getCount())
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int SelCheckListDialog::SetupItemList()
{
	int    ok = -1;
	SString sub;
	SString memo_buf;
	int    memo_has_addr = 0;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SELCHECK_ITEMLIST));
	if(p_list) {
		PPID _top_id = 0;
		getCurItem(0, &_top_id);
		if(_top_id != LastTopID) {
			bool do_finish = false;
			StringSet ss(SLBColumnDelim);
			if(State & stSelectBill && (BPackList.getCount() || (State & stInputUpdated))) {
				const PPBillPacket * p_bpack = 0;
				{
					for(uint i = 0; !p_bpack && i < BPackList.getCount(); i++) {
						const PPBillPacket * p_bpack_local = BPackList.at(i);
						if(p_bpack_local && p_bpack_local->Rec.ID == _top_id)
							p_bpack = p_bpack_local;
					}
				}
				if(p_bpack) {
					p_list->freeAll();
					for(uint i = 0; i < p_bpack->GetTCount(); i++) {
						ss.Z();
						const PPTransferItem & r_ti = p_bpack->ConstTI(i);
						double price = r_ti.NetPrice();
						double qtty = abs(r_ti.Qtty());
						double sum = R2(qtty * r_ti.NetPrice());
						GetGoodsName(r_ti.GoodsID, sub);
						ss.add(sub);
						ss.add(sub.Z().Cat(price, SFMT_MONEY));
						ss.add(sub.Z().Cat(qtty, SFMT_QTTY));
						ss.add(sub.Z().Cat(sum, SFMT_MONEY));
						THROW(p_list->addItem(i, ss.getBuf()));
					}
					enableCommand(cmSplitCheck, false);
					enableCommand(cmUniteChecks, false);
					do_finish = true;
				}
			}
			else if(!(State & stTblOrders) && (ChkList.getCount() || (State & stInputUpdated))) {
				CCheckPacket pack;
				p_list->freeAll();
				if(_top_id && P_Srv->GetCc().LoadPacket(_top_id, 0, &pack) > 0) {
					memo_buf.Z();
					if(pack.Ext.AddrID) {
						LocationTbl::Rec loc_rec;
						if(LocObj.Search(pack.Ext.AddrID, &loc_rec) > 0) {
							memo_buf.Cat(LocationCore::GetExFieldS(&loc_rec, LOCEXSTR_PHONE, sub));
							if(LocationCore::GetExFieldS(&loc_rec, LOCEXSTR_SHORTADDR, sub).NotEmptyS())
								memo_buf.CatDivIfNotEmpty(',', 2).Cat(sub);
							if(LocationCore::GetExFieldS(&loc_rec, LOCEXSTR_CONTACT, sub).NotEmptyS())
								memo_buf.CatDivIfNotEmpty(',', 2).Cat(sub);
						}
					}
					if(memo_buf.NotEmptyS())
						memo_has_addr = 1;
					else
						memo_buf = pack.Ext.Memo;
					for(uint i = 0; i < pack.GetCount(); i++) {
						ss.Z();
						const CCheckLineTbl::Rec & cclr = pack.GetLineC(i);
						const double price = intmnytodbl(cclr.Price);
						const double sum = R2(cclr.Quantity * (price - cclr.Dscnt));
						GetGoodsName(cclr.GoodsID, sub);
						ss.add(sub);
						ss.add(sub.Z().Cat(price, SFMT_MONEY));
						ss.add(sub.Z().Cat(cclr.Quantity, SFMT_QTTY));
						ss.add(sub.Z().Cat(sum, SFMT_MONEY));
						THROW(p_list->addItem(i, ss.getBuf()));
					}
				}
				{
					const bool to_disable = (!(State & stSelToUnite) && (pack.Rec.Flags & CCHKF_SUSPENDED) && !(pack.Rec.Flags & CCHKF_JUNK));
					enableCommand(cmSplitCheck, to_disable);
					enableCommand(cmUniteChecks, to_disable);
				}
				{
					setLabelText(CTL_SELCHECK_MEMO, PPLoadStringS(memo_has_addr ? "address" : "memo", sub));
					setCtrlString(CTL_SELCHECK_MEMO, memo_buf);
				}
				do_finish = true;
			}
			if(do_finish) {
				p_list->focusItem(0);
				p_list->Draw_();
				LastTopID = _top_id;
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// SplitSuspCheckDialog
//
class SplitSuspCheckDialog : public Lst2LstAryDialog {
public:
	struct ListItem {
		long   LineNo;
		long   GoodsID;
		long   DivID;
		double Quantity;
		double Price;
		double Discount;
		char   Serial[64];
	};
	SplitSuspCheckDialog(uint dlgId, CCheckPacket * pPack, ListToListUIData * pData, SArray * pLeft, SArray * pRight) : Lst2LstAryDialog(dlgId, pData, pLeft, pRight), P_Pack(pPack)
	{
		SString agent_name;
		const double amount = MONEYTOLDBL(P_Pack->Rec.Amount);
		GetArticleName(P_Pack->Ext.SalerID, agent_name);
		setCtrlString(CTL_SPLITSUSCHK_AGENT, agent_name);
		setCtrlReal(CTL_SPLITSUSCHK_AMOUNT, amount);
		setCtrlData(CTL_SPLITSUSCHK_TABLE,  &P_Pack->Ext.TableNo);
		disableCtrls(1, CTL_SPLITSUSCHK_AGENT, CTL_SPLITSUSCHK_AMOUNT, CTL_SPLITSUSCHK_TABLE, 0L);
		SetupStrListBox(this, CTL_SPLITSUSCHK_LIST1);
		SetupStrListBox(this, CTL_SPLITSUSCHK_LIST2);
		setupLeftList();
		setupRightList();
	}
	int    getDTS(SArray * pLeftList, SArray * pRightList)
	{
		int    ok = 1;
		if(pLeftList) {
			pLeftList->copy(*GetLeft());
			THROW_PP(pLeftList->getCount(), PPERR_ONECHECKEMPTY);
		}
		if(pRightList) {
			pRightList->copy(*GetRight());
			THROW_PP(pRightList->getCount(), PPERR_ONECHECKEMPTY);
		}
		CATCHZOK
		return ok;
	}
private:
	virtual int addItem();
	virtual int removeItem();
	virtual int addAll() { return -1; }
	virtual int removeAll() { return -1; }
	virtual int SetupList(SArray *, SmartListBox *);

	CCheckPacket * P_Pack;
};

/*virtual*/int SplitSuspCheckDialog::addItem()
{
	int    ok  = 1;
	long   id = 0;
	uint   pos = 0;
	SmartListBox * p_view = GetLeftList();
	SArray * p_rl = GetRight(), * p_ll = GetLeft();
	if(p_view->getCurID(&id) && id && p_ll->lsearch(&id, &pos, CMPF_LONG, 0)) {
		double qtty = 0.0;
		ListItem * p_litem = static_cast<ListItem *>(p_ll->at(pos));
		if(InputQttyDialog(0, 0, &(qtty = p_litem->Quantity)) > 0 && qtty > 0.0) {
			qtty = (qtty > p_litem->Quantity) ? p_litem->Quantity : qtty;
			if(p_rl->lsearch(p_litem, &pos, CMPF_LONG, 0)) {
				ListItem * p_ritem = static_cast<ListItem *>(p_rl->at(pos));
				p_ritem->Quantity += qtty;
			}
			else {
				ListItem item;
				item = *p_litem;
				item.Quantity = qtty;
				THROW_SL(p_rl->insert(&item));
			}
			p_litem->Quantity -= qtty;
			if(p_litem->Quantity == 0.0)
				THROW_SL(p_ll->atFree(pos));
			THROW(setupLeftList());
			THROW(setupRightList());
		}
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int SplitSuspCheckDialog::removeItem()
{
	int    ok  = 1;
	uint   pos = 0;
	long   id = 0;
	SmartListBox * p_lb = GetRightList();
	SArray * p_rl = GetRight();
	SArray * p_ll = GetLeft();
	if(p_lb && p_lb->getCurID(&id) && id && p_rl->lsearch(&id, &pos, CMPF_LONG, 0)) {
		uint lpos = 0;
		ListItem * p_ritem = static_cast<ListItem *>(p_rl->at(pos));
		if(p_ll->lsearch(p_ritem, &lpos, CMPF_LONG, 0)) {
			ListItem * p_litem = static_cast<ListItem *>(p_ll->at(lpos));
			p_litem->Quantity += p_ritem->Quantity;
		}
		else {
			ListItem item(*p_ritem);
			THROW_SL(p_ll->insert(&item));
		}
		THROW_SL(p_rl->atFree(pos) > 0);
        THROW(setupRightList());
		THROW(setupLeftList());
	}
	CATCHZOKPPERR
	return ok;
}

/*virtual*/int SplitSuspCheckDialog::SetupList(SArray * pList, SmartListBox * pListBox)
{
	int    ok = 1;
	if(pList && SmartListBox::IsValidS(pListBox)) {
		const long preserve_pos = pListBox->P_Def->_curItem();
		SString temp_buf;
		StringSet ss(SLBColumnDelim);
		ListItem * p_item = 0;
		pListBox->freeAll();
		for(uint i = 0; pList->enumItems(&i, (void **)&p_item) > 0;) {
			ss.Z();
			GetGoodsName(p_item->GoodsID, temp_buf);
			ss.add(temp_buf);
			ss.add(temp_buf.Z().Cat(p_item->Price));
			ss.add(temp_buf.Z().Cat(p_item->Quantity));
			if(!pListBox->addItem(p_item->LineNo, ss.getBuf()))
				ok = (PPSetErrorSLib(), 0);
		}
		pListBox->P_Def->go(preserve_pos);
		pListBox->Draw_();
	}
	return ok;
}

int SelCheckListDialog::SplitCheck()
{
	int    ok = -1;
	SplitSuspCheckDialog * dlg = 0;
	if(oneof2(resourceID, DLG_SELSUSCHECK_L, DLG_SELSUSCHECK)) {
		long   chk_id = 0;
		uint   dlg_id = (resourceID == DLG_SELSUSCHECK_L) ? DLG_SPLITSUSCHK_L : DLG_SPLITSUSCHK;
		SArray left_list(sizeof(SplitSuspCheckDialog::ListItem));
		SArray right_list(sizeof(SplitSuspCheckDialog::ListItem));
		CCheckPacket pack;
		THROW_PP(!P_AddParam || P_AddParam->Rights & CheckPaneDialog::orfSplitCheck, PPERR_NORIGHTS);
		{
			getCurItem(0, &chk_id);
			if(chk_id && P_Srv->GetCc().LoadPacket(chk_id, 0, &pack) > 0) {
				SString serial;
				CCheckLineTbl::Rec chk_item;
				THROW_PP(!P_AddParam || !(pack.Rec.Flags & CCHKF_PREPRINT) || P_AddParam->Rights & CheckPaneDialog::orfChgPrintedCheck, PPERR_NORIGHTS);
				for(uint pos = 0; pack.EnumLines(&pos, &chk_item, &serial);) {
					SplitSuspCheckDialog::ListItem item;
					MEMSZERO(item);
					item.LineNo   = pos;
					item.GoodsID  = chk_item.GoodsID;
					item.DivID    = static_cast<long>(chk_item.DivID);
					item.Discount = chk_item.Dscnt;
					item.Price    = intmnytodbl(chk_item.Price);
					item.Quantity = chk_item.Quantity;
					serial.CopyTo(item.Serial, sizeof(item.Serial));
					THROW_SL(left_list.insert(&item));
				}
			}
		}
		if(left_list.getCount()) {
			ListToListUIData ui_data;
			ui_data.LeftCtlId  = CTL_SPLITSUSCHK_LIST1;
			ui_data.RightCtlId = CTL_SPLITSUSCHK_LIST2;
			THROW(CheckDialogPtr(&(dlg = new SplitSuspCheckDialog(dlg_id, &pack, &ui_data, &left_list, &right_list))));
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(!dlg->getDTS(&left_list, &right_list))
					PPError();
				else {
					CCheckPacket add_pack;
					CCheckCore & r_cc = P_Srv->GetCc();
					{
						PPTransaction tra(1);
						THROW(tra);
						{
							SplitSuspCheckDialog::ListItem * p_item = 0;
							pack.Rec.ID       = 0;
							pack.Ext.CheckID  = 0;
							pack.Ext.AddPaym_unused = 0;
							pack._Cash        = 0;
							pack.PctDis       = 0;
							add_pack.Rec = pack.Rec;
							add_pack.Ext = pack.Ext;
							add_pack.Ext.GuestCount = 0;
							{
								CCheckTbl::Rec chk_rec;
								add_pack.Rec.Code = 1 + ((r_cc.GetLastCheckByCode(pack.Rec.PosNodeID, &chk_rec) > 0) ? chk_rec.Code : pack.Rec.Code);
							}
							getcurdatetime(&add_pack.Rec.Dt, &add_pack.Rec.Tm);
							pack.ClearLines();
							for(uint pos = 0; left_list.enumItems(&pos, (void **)&p_item) > 0;) {
								CCheckLineTbl::Rec chk_item;
								chk_item.GoodsID  = p_item->GoodsID;
								chk_item.DivID    = static_cast<int16>(p_item->DivID);
								chk_item.Price    = dbltointmny(p_item->Price);
								chk_item.Dscnt    = p_item->Discount;
								chk_item.Quantity = p_item->Quantity;
								THROW(pack.InsertCclRec(&chk_item, p_item->Serial));
							}
							for(uint pos = 0; right_list.enumItems(&pos, (void **)&p_item) > 0;) {
								CCheckLineTbl::Rec chk_item;
								chk_item.GoodsID  = p_item->GoodsID;
								chk_item.DivID    = static_cast<int16>(p_item->DivID);
								chk_item.Price    = dbltointmny(p_item->Price);
								chk_item.Dscnt    = p_item->Discount;
								chk_item.Quantity = p_item->Quantity;
								THROW(add_pack.InsertCclRec(&chk_item, p_item->Serial));
							}
							pack.SetupAmount(0, 0);
							add_pack.SetupAmount(0, 0);
						}
						pack.Rec.ID = chk_id;
						THROW(r_cc.UpdateCheck(&pack, 0));
						THROW(r_cc.TurnCheck(&add_pack, 0));
						THROW(tra.Commit());
					}
					if(ChkList.getCount()) {
						uint pos = 0;
						CCheckViewItem v_item;
						*static_cast<CCheckTbl::Rec *>(&v_item) = pack.Rec;
						v_item.TableCode = pack.Ext.TableNo;
						v_item.AgentID   = pack.Ext.SalerID;
						THROW_SL(ChkList.lsearch(&chk_id, &pos, CMPF_LONG, 0));
						THROW_SL(ChkList.atFree(pos));
						THROW_SL(ChkList.insert(&v_item));
						MEMSZERO(v_item);
						*static_cast<CCheckTbl::Rec *>(&v_item) = add_pack.Rec;
						v_item.TableCode = pack.Ext.TableNo;
						v_item.AgentID   = pack.Ext.SalerID;
						THROW_SL(ChkList.insert(&v_item));
					}
					ok = valid_data = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SelCheckListDialog::UniteChecks()
{
	int    ok = -1;
	long   chk1_id = 0L;
	SelCheckListDialog * dlg = 0;
	getCurItem(0, &chk1_id);
	if(chk1_id && ChkList.getCount() > 1 && oneof2(resourceID, DLG_SELSUSCHECK_L, DLG_SELSUSCHECK)) {
		uint   pos = 0;
		TSVector <CCheckViewItem> list;
		THROW_PP(!P_AddParam || P_AddParam->Rights & CheckPaneDialog::orfMergeChecks, PPERR_NORIGHTS);
		list.copy(ChkList);
		THROW_SL(list.lsearch(&chk1_id, &pos, CMPF_LONG));
		list.atFree(pos);
		uint dlg_id = (DlgFlags & fLarge) ? DLG_SELSUSCHECK_L : DLG_SELSUSCHECK;
		THROW(CheckDialogPtr(&(dlg = new SelCheckListDialog(dlg_id, &list, 1, P_Srv, P_AddParam))));
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			_SelCheck check2;
			if(!dlg->getDTS(&check2))
				PPError();
			else {
				dlg_id = (resourceID == DLG_SELSUSCHECK_L) ? DLG_SPLITSUSCHK_L : DLG_SPLITSUSCHK;
				SString serial;
				CCheckLineTbl::Rec chk_item;
				CCheckPacket pack1, pack2;
				CCheckCore & r_cc = P_Srv->GetCc();
				{
					PPTransaction tra(1);
					THROW(tra);
					THROW(r_cc.LoadPacket(chk1_id, 0, &pack1));
					THROW(r_cc.LoadPacket(check2.CheckID, 0, &pack2));
					pack1._Cash        += pack2._Cash;
					SETIFZ(pack1.PctDis, pack2.PctDis);
					for(pos = 0; pack2.EnumLines(&pos, &chk_item, &serial);) {
						THROW(pack1.InsertCclRec(&chk_item, serial));
					}
					THROW(r_cc.RemovePacket(check2.CheckID, 0));
					pack1.SetupAmount(0, 0);
					pack1.Rec.ID = chk1_id;
					THROW(r_cc.UpdateCheck(&pack1, 0));
					DS.LogAction(PPACN_UNITECCHECK, PPOBJ_CCHECK, pack1.Rec.ID, pack2.Rec.ID, 0);
					THROW(tra.Commit());
				}
				if(ChkList.getCount()) {
					uint   new_pos = 0;
					CCheckViewItem v_item;
					*static_cast<CCheckTbl::Rec *>(&v_item) = pack1.Rec;
					v_item.TableCode = pack1.Ext.TableNo;
					v_item.AgentID   = pack1.Ext.SalerID;
					THROW_SL(ChkList.lsearch(&chk1_id, &(pos = 0), CMPF_LONG, 0));
					THROW_SL(ChkList.atInsert(pos, &v_item));
					THROW_SL(ChkList.atFree(pos + 1));
					THROW_SL(ChkList.lsearch(&check2.CheckID, &(pos = 0), CMPF_LONG, 0));
					THROW_SL(ChkList.atFree(pos));
					LastTopID = 0;
					if(P_Box) {
						THROW_SL(ChkList.lsearch(&pack1.Rec.ID, &(pos = 0), CMPF_LONG, 0));
						P_Box->focusItem(pos);
					}
				}
				ok = valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPL_HANDLE_EVENT(SelCheckListDialog)
{
	if(event.isCmd(cmOK)) {
		updateList(-1);
		if(State & stListUpdated) {
			selectCtrl(CTL_SELCHECK_LIST);
			State &= ~stListUpdated;
			clearEvent(event);
		}
	}
	else if(event.isCmd(cmLBDblClk))
		TVCMD = cmOK;
	else if(event.isCmd(cmNewCheck) && State & stTblOrders) {
		if(IsInState(sfModal)) {
			endModal(cmNewCheck);
			return; // После endModal не следует обращаться к this
		}
	}
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		const uint ev_ctl_id = event.getCtlID();
		if(TVCMD == cmInputUpdated) {
			State |= stInputUpdated;
			if(event.isCtlEvent(CTL_SELCHECK_DATE)) {
				if(State & stTblOrders) {
					const LDATE  dt = getCtrlDate(CTL_SELCHECK_DATE);
					if(dt != LastDate && checkdate(dt, 1)) {
						LastDate = dt;
						State |= stListUpdated;
						updateList(-1);
					}
				}
			}
		}
		else if(oneof2(TVCMD, cmLBItemSelected, cmLBItemFocused) && ev_ctl_id == CTL_SELCHECK_LIST)
			SetupItemList();
		else if(event.isClusterClk(CTL_SELCHECK_UNFC)) {
			const int org_unfc = BIN(State & stSelectUnfinished);
			GetClusterData(CTL_SELCHECK_UNFC, &State);
			const int unfc = BIN(State & stSelectUnfinished);
			if(unfc != org_unfc) {
				LastChkNo = -1;
				LastDate = ZERODATE;
				LastOp = CCOP_GENERAL; // @v12.2.8
				if(!unfc && State & stOuterList)
					ChkList = PreserveOuterChkList;
				updateList(-1);
			}
		}
		else if(event.isClusterClk(CTL_SELCHECK_OP)) { // @v12.2.8
			const long preserve_op = Op;
			GetClusterData(CTL_SELCHECK_OP, &Op);
			if(Op != preserve_op) {
				updateList(-1);
			}
		}
		else if(TVCMD == cmSplitCheck) {
			if(SplitCheck() > 0) {
				State |= stListUpdated;
				updateList(-1);
			}
		}
		else if(TVCMD == cmUniteChecks) {
			if(UniteChecks() > 0) {
				State |= stListUpdated;
				updateList(-1);
			}
		}
		else if(TVCMD == cmaInsert) {
			if(addItem(0, 0) > 0) {
				P_Srv->GetTblOrderList(LastDate, ChkList);
				updateList(-1);
			}
		}
		else if(TVCMD == cmaEdit) {
			long pos = -1;
			long id = -1;
			getCurItem(&pos, &id);
			if(editItem(pos, id) > 0) {
				P_Srv->GetTblOrderList(LastDate, ChkList);
				updateList(-1);
			}
		}
		else if(TVCMD == cmaDelete) {
			long pos = -1;
			long id = -1;
			getCurItem(&pos, &id);
			if(delItem(pos, id) > 0)
				updateList(-1);
		}
		else
			return;
	}
	else if(TVBROADCAST && oneof2(TVCMD, cmReceivedFocus, cmCommitInput) && (State & stInputUpdated)) {
		updateList(-1);
		State &= ~stInputUpdated;
	}
	else
		return;
	clearEvent(event);
}

struct AddrByPhoneItem { // @flat
	PPID   ObjType;
	PPID   ObjID;
	long   ObjFlags;
	PPID   CityID;
	PPID   AddrID;     // Если ObjType == PPOBJ_PERSON, то AddrID уточняет адрес из списка адресов, принадлежащих персоналии
	char   Phone[32];
	char   Addr[128];
	char   Contact[128];
};

class CheckDlvrDialog : public TDialog {
	DECL_DIALOG_DATA(CheckPaneDialog::ExtCcData);
	enum {
		ctlgrouSCard = 1
	};
public:
	CheckDlvrDialog(PPID scardID, const char * pDlvrPhone, const char * pChannel) : TDialog(DLG_CCHKDLVR), ScsRsrvPoolID(0), Channel(pChannel),
		LockAddrModChecking(0), PersonID(0), DefCityID(0), DlvrPhone(pDlvrPhone)
	{
		addGroup(ctlgrouSCard, new SCardCtrlGroup(0, CTL_CCHKDLVR_SCARD, 0));
		Data.SCardID_ = scardID;
		SetupCalDate(CTLCAL_CCHKDLVR_DT, CTL_CCHKDLVR_DT);
		SetupTimePicker(this, CTL_CCHKDLVR_TM, CTLTM_CCHKDLVR_TM);
		setStaticText(CTL_CCHKDLVR_ST_SIP, Channel);
		GetMainCityID(&DefCityID);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		{
			const  PPID preserve_sc_id = Data.SCardID_;
			Data = *pData;
			SETIFZ(Data.SCardID_, preserve_sc_id);
		}
		setCtrlString(CTL_CCHKDLVR_MEMO, Data.Memo);
		AddClusterAssoc(CTL_CCHKDLVR_FLAGS, 0, Data.fDelivery);
		AddClusterAssoc(CTL_CCHKDLVR_FLAGS, 1, Data.fSpFinished);
		SetClusterData(CTL_CCHKDLVR_FLAGS, Data.Flags);
		SetupPPObjCombo(this, CTLSEL_CCHKDLVR_CITY, PPOBJ_WORLD, NZOR(Data.Addr_.CityID, 0/*DefCityID*/), OLW_WORDSELECTOR, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
		SetupDeliveryCtrls(0);
		if(DlvrPhone.NotEmpty()) {
			Data.Flags |= Data.fDelivery;
			SetClusterData(CTL_CCHKDLVR_FLAGS, Data.Flags);
			SetupDeliveryCtrls(DlvrPhone);
			ReplyPhone(1);
		}
		if(Data.Flags & Data.fDelivery)
			selectCtrl(CTL_CCHKDLVR_ADDR);
		{
			SCardCtrlGroup::Rec screc;
			screc.SCardID = Data.SCardID_;
			setGroupData(ctlgrouSCard, &screc);
		}
		{
			AddClusterAssoc(CTL_CCHKDLVR_LPHTOCRD, 0, Data.fAttachPhoneToSCard);
			AddClusterAssoc(CTL_CCHKDLVR_LPHTOCRD, 1, Data.fCreateCardByPhone);
			SetClusterData(CTL_CCHKDLVR_LPHTOCRD, Data.Flags);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		const  PPID preserve_loc_id = CheckAddrModif(0) ? 0 : Data.Addr_.ID;
		const  PPID preserve_init_user_id = Data.InitUserID;
		const LDATETIME preserve_init_dtm = Data.InitDtm;
		Data.Z();
		Data.Addr_.ID = preserve_loc_id;
		Data.InitUserID = preserve_init_user_id;
		Data.InitDtm = preserve_init_dtm;
		getCtrlString(CTL_CCHKDLVR_MEMO, Data.Memo);
		GetClusterData(CTL_CCHKDLVR_FLAGS, &Data.Flags);
		if(Data.Flags & Data.fDelivery) {
			Data.Addr_.Type = LOCTYP_ADDRESS;
			Data.Addr_.CityID = getCtrlLong(CTLSEL_CCHKDLVR_CITY);
			getCtrlString(CTL_CCHKDLVR_ADDR, temp_buf);
			LocationCore::SetExField(&Data.Addr_, LOCEXSTR_SHORTADDR, temp_buf);
			getCtrlString(CTL_CCHKDLVR_PHONE, temp_buf);
			LocationCore::SetExField(&Data.Addr_, LOCEXSTR_PHONE, temp_buf);
			getCtrlString(CTL_CCHKDLVR_CONTACT, temp_buf);
			LocationCore::SetExField(&Data.Addr_, LOCEXSTR_CONTACT, temp_buf);
			Data.DlvrDtm.d = getCtrlDate(CTL_CCHKDLVR_DT);
			Data.DlvrDtm.t = getCtrlTime(CTL_CCHKDLVR_TM);
			GetClusterData(CTL_CCHKDLVR_LPHTOCRD, &Data.Flags);
		}
		{
			SCardCtrlGroup::Rec screc;
			getGroupData(ctlgrouSCard, &screc);
			Data.SCardID_ = screc.SCardID;
		}
		ASSIGN_PTR(pData, Data);
		return ok;
	}
	int    getSCardID(PPID * pScID) const
	{
		if(Data.SCardID_) {
			ASSIGN_PTR(pScID, Data.SCardID_);
			return 1;
		}
		else
			return 0;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_CCHKDLVR_FLAGS)) {
			GetClusterData(CTL_CCHKDLVR_FLAGS, &Data.Flags);
			SetupDeliveryCtrls(0);
		}
		else if(event.isClusterClk(CTL_CCHKDLVR_LPHTOCRD)) {
			const long preserve_flags = Data.Flags;
			SString temp_buf;
			SString phone_buf;
			GetClusterData(CTL_CCHKDLVR_LPHTOCRD, &Data.Flags);
			if(Data.Flags & Data.fCreateCardByPhone) {
				if(Data.SCardID_ == 0) {
					getCtrlString(CTL_CCHKDLVR_PHONE, temp_buf);
					if(temp_buf.NotEmptyS()) {
						temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
						PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
						PPID   scs_pool_id = 0;
						PPID   sc_id = 0;
						int    scr = ScObj.SelectCardFromReservePool(&scs_pool_id, 0, &sc_id, 1);
						if(scr > 0) {
						}
						else {
							PPError();
							Data.Flags &= ~Data.fCreateCardByPhone;
							SetClusterData(CTL_CCHKDLVR_LPHTOCRD, Data.Flags);
						}
					}
				}
			}
			else {
				if(Data.SCardID_) {
					if(ScsRsrvPoolID) {
					}
				}
			}
		}
		else if(event.isCmd(cmSelAddrByPhone)) {
			const  uint c = AddrByPhoneList.getCount();
			if(Data.Flags & Data.fDelivery && c) {
				if(c == 1)
					SetupAddr(&AddrByPhoneList.at(0));
				else
					SelectAddrByPhone();
			}
		}
		else if(TVCMD == cmInputUpdated) {
			if(event.isCtlEvent(CTL_CCHKDLVR_PHONE)) {
				UI_LOCAL_LOCK_ENTER
					ReplyPhone(0);
					CheckAddrModif(1);
				UI_LOCAL_LOCK_LEAVE
			}
			else if(event.isCtlEvent(CTL_CCHKDLVR_ADDR) || event.isCtlEvent(CTL_CCHKDLVR_CONTACT))
				CheckAddrModif(1);
			else
				return;
		}
		else if(TVCMD == cmWSSelected) {
			if(event.isCtlEvent(CTL_CCHKDLVR_SCARD)) {
				SCardCtrlGroup::Rec scgrec;
				getGroupData(ctlgrouSCard, &scgrec);
				if(scgrec.SCardID) {
					int    local_ok = 0;
					SCardTbl::Rec sc_rec;
					if(ScObj.Fetch(scgrec.SCardID, &sc_rec) > 0) {
						if((!sc_rec.LocID || sc_rec.LocID == Data.Addr_.ID) && (!sc_rec.PersonID || sc_rec.PersonID == Data.Addr_.OwnerID)) {
							Data.SCardID_ = scgrec.SCardID;
							local_ok = 1;
						}
					}
					if(!local_ok && scgrec.SCardID) {
						scgrec.SCardID = 0;
						setGroupData(ctlgrouSCard, &scgrec);
					}
				}
				else
					Data.SCardID_ = 0;
				SetupDeliveryCtrls(0);
			}
		}
		else
			return;
		clearEvent(event);
	}
	bool   CheckAddrModif(int doSetupCtrls)
	{
		bool   is_mod = false;
		SString org_buf;
		SString now_buf;
		if(!LockAddrModChecking && Data.Addr_.ID && Data.Addr_.ID == OrgLocRec.ID) {
			if(Data.Addr_.CityID != OrgLocRec.CityID)
				is_mod = true;
			else {
				getCtrlString(CTL_CCHKDLVR_ADDR, now_buf);
				if(now_buf.CmpNC(LocationCore::GetExFieldS(&OrgLocRec, LOCEXSTR_SHORTADDR, org_buf)) != 0)
					is_mod = true;
				else {
					getCtrlString(CTL_CCHKDLVR_PHONE, now_buf);
					if(now_buf.CmpNC(LocationCore::GetExFieldS(&OrgLocRec, LOCEXSTR_PHONE, org_buf)) != 0)
						is_mod = true;
					else {
						getCtrlString(CTL_CCHKDLVR_CONTACT, now_buf);
						if(now_buf.CmpNC(LocationCore::GetExFieldS(&OrgLocRec, LOCEXSTR_CONTACT, org_buf)) != 0)
							is_mod = true;
					}
				}
			}
		}
		if(doSetupCtrls) {
			if(is_mod)
				PPLoadText(PPTXT_DLVRADDRMODIFIED, now_buf);
			else
				now_buf.Z();
			setStaticText(CTL_CCHKDLVR_ST_ADDRMOD, now_buf);
		}
		return is_mod;
	}
	void SetupAddr(const AddrByPhoneItem * pEntry)
	{
		if(pEntry) {
			SString temp_buf;
			PersonID = 0;
			if(pEntry->ObjType == PPOBJ_LOCATION && (pEntry->ObjFlags & LOCF_STANDALONE) && PsnObj.LocObj.Search(pEntry->ObjID, &Data.Addr_) > 0) {
				OrgLocRec = Data.Addr_;
			}
			else if(pEntry->ObjType == PPOBJ_PERSON) {
				PersonID = pEntry->ObjID;
				if(pEntry->Phone[0])
					DlvrPhone = pEntry->Phone;
				if(pEntry->AddrID && PsnObj.LocObj.Search(pEntry->AddrID, &Data.Addr_) > 0)
					OrgLocRec = Data.Addr_;
			}
			else {
				OrgLocRec.Clear();
				if(pEntry->CityID)
					Data.Addr_.CityID = pEntry->CityID;
				LocationCore::SetExField(&Data.Addr_, LOCEXSTR_SHORTADDR, pEntry->Addr);
				LocationCore::SetExField(&Data.Addr_, LOCEXSTR_PHONE, pEntry->Phone);
				LocationCore::SetExField(&Data.Addr_, LOCEXSTR_CONTACT, pEntry->Contact);
				Data.Addr_.Flags |= LOCF_STANDALONE;
				SETIFZ(Data.Addr_.Type, LOCTYP_ADDRESS);
			}
			setStaticText(CTL_CCHKDLVR_ST_ADDRID,  temp_buf.Z().Cat(Data.Addr_.ID));
			setStaticText(CTL_CCHKDLVR_ST_ADDRMOD, temp_buf.Z());
			{
				PPIDArray sc_list;
				PPID   sc_id = 0;
				if(pEntry->Phone[0]) {
					PPEAddr::Phone::NormalizeStr(pEntry->Phone, 0, temp_buf);
					if(temp_buf.NotEmptyS()) {
						PPIDArray phone_id_list;
						PsnObj.LocObj.P_Tbl->SearchPhoneIndex(temp_buf, 0, phone_id_list);
						for(uint i = 0; i < phone_id_list.getCount(); i++) {
							const  PPID ea_id = phone_id_list.get(i);
							EAddrTbl::Rec ea_rec;
							if(PsnObj.LocObj.P_Tbl->GetEAddr(ea_id, &ea_rec) > 0 && ea_rec.LinkObjType == PPOBJ_SCARD)
								sc_list.add(ea_rec.LinkObjID);
						}
					}
				}
				if(!sc_list.getCount()) {
					if(PersonID)
						ScObj.P_Tbl->GetListByPerson(PersonID, 0, &sc_list);
					else if(Data.Addr_.ID)
						ScObj.P_Tbl->GetListByLoc(Data.Addr_.ID, 0, &sc_list);
				}
				if(sc_list.getCount()) {
					sc_id = sc_list.get(0);
				}
				setCtrlLong(CTL_CCHKDLVR_SCARD, sc_id);
			}
			SetupDeliveryCtrls(0);
		}
	}
	void SelectAddrByPhone()
	{
		class SelAddrByPhoneDialog : public PPListDialog {
		public:
			SelAddrByPhoneDialog(const TSVector <AddrByPhoneItem> * pData) : PPListDialog(DLG_SELADDRBYPH, CTL_SELADDRBYPH_LIST)
			{
				RVALUEPTR(Data, pData);
				updateList(-1);
			}
			const AddrByPhoneItem * getSelectedItem()
			{
				long   sel_id = 0;
				return (getSelection(&sel_id) && sel_id > 0 && sel_id <= Data.getCountI()) ? &Data.at(sel_id-1) : 0;
			}
		private:
			DECL_HANDLE_EVENT
			{
				if(event.isCmd(cmLBDblClk)) {
					long   idx = 0;
					if(getSelection(&idx) && idx > 0 && idx <= Data.getCountI())
						TVCMD = cmOK;
				}
				PPListDialog::handleEvent(event);
			}
			virtual int setupList()
			{
				int    ok = 1;
				SString temp_buf;
				PPIDArray sc_list;
				StringSet ss(SLBColumnDelim);
				for(uint i = 0; i < Data.getCount(); i++) {
					const AddrByPhoneItem & r_entry = Data.at(i);
					ss.Z();
					sc_list.clear();
					if(r_entry.ObjType == PPOBJ_LOCATION) {
						PPLoadStringS("address", temp_buf);
						ScObj.P_Tbl->GetListByLoc(r_entry.ObjID, 0, &sc_list);
					}
					else if(r_entry.ObjType == PPOBJ_PERSON) {
						PPLoadString("person", temp_buf);
						ScObj.P_Tbl->GetListByPerson(r_entry.ObjID, 0, &sc_list);
					}
					else
						temp_buf.Z().Cat(r_entry.ObjType);
					ss.add(temp_buf);
					ss.add(temp_buf.Z().Cat(r_entry.ObjID));
					ss.add(temp_buf = r_entry.Contact);
					ss.add(temp_buf = r_entry.Addr);
					{
						temp_buf.Z();
						if(sc_list.getCount()) {
							PPID   sc_id = sc_list.get(0);
							SCardTbl::Rec sc_rec;
							if(sc_list.getCount() > 1)
								temp_buf.Cat("...");
							if(ScObj.Fetch(sc_id, &sc_rec) > 0)
								temp_buf.Cat(sc_rec.Code);
						}
						ss.add(temp_buf);
					}
					addStringToList(i+1, ss.getBuf());
				}
				return ok;
			}
			TSVector <AddrByPhoneItem> Data;
			PPObjSCard ScObj;
		};
		{
			UI_LOCAL_LOCK_ENTER
				SelAddrByPhoneDialog * dlg = new SelAddrByPhoneDialog(&AddrByPhoneList);
				if(CheckDialogPtrErr(&dlg)) {
					if(ExecView(dlg) == cmOK)
						SetupAddr(dlg->getSelectedItem());
				}
				delete dlg;
			UI_LOCAL_LOCK_LEAVE
		}
	}
	int    ReplyPhone(int immSelect)
	{
		AddrByPhoneList.clear();
		SString temp_buf;
		SString phone_buf;
		getCtrlString(CTL_CCHKDLVR_PHONE, temp_buf);
		if(temp_buf.NotEmptyS()) {
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
			PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf);
			if(Data.Flags & Data.fDelivery && CConfig.Flags2 & CCFLG2_INDEXEADDR) {
				if(phone_buf.NotEmptyS()) {
					PPIDArray addr_list;
					PPIDArray dlvr_addr_list;
					PPIDArray sc_list;
					PPIDArray phone_id_list;
					PsnObj.LocObj.P_Tbl->SearchPhoneIndex(phone_buf, 0, phone_id_list);
					for(uint i = 0; i < phone_id_list.getCount(); i++) {
						EAddrTbl::Rec ea_rec;
						LocationTbl::Rec loc_rec;
						PersonTbl::Rec psn_rec;
						SCardTbl::Rec sc_rec;
						if(PsnObj.LocObj.P_Tbl->GetEAddr(phone_id_list.get(i), &ea_rec) > 0) {
							switch(ea_rec.LinkObjType) {
								case PPOBJ_SCARD:
									if(ScObj.Fetch(ea_rec.LinkObjID, &sc_rec) > 0) {
										sc_list.addUnique(sc_rec.ID);
									}
									break;
								case PPOBJ_LOCATION:
									if(PsnObj.LocObj.Search(ea_rec.LinkObjID, &loc_rec) > 0) {
										AddrByPhoneItem ap_item;
										MEMSZERO(ap_item);
										ap_item.ObjType = ea_rec.LinkObjType;
										ap_item.ObjID = ea_rec.LinkObjID;
										ap_item.ObjFlags = loc_rec.Flags;
										ap_item.CityID = loc_rec.CityID;
										phone_buf.CopyTo(ap_item.Phone, sizeof(ap_item.Phone));
										LocationCore::GetExFieldS(&loc_rec, LOCEXSTR_CONTACT, temp_buf).CopyTo(ap_item.Contact, sizeof(ap_item.Contact));
										LocationCore::GetExFieldS(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf).CopyTo(ap_item.Addr, sizeof(ap_item.Addr));
										AddrByPhoneList.insert(&ap_item);
									}
									break;
								case PPOBJ_PERSON:
									if(PsnObj.Search(ea_rec.LinkObjID, &psn_rec) > 0) {
										uint j;
										addr_list.clear();
										addr_list.addnz(psn_rec.RLoc);
										PsnObj.GetDlvrLocList(psn_rec.ID, &dlvr_addr_list);
										for(j = 0; j < dlvr_addr_list.getCount(); j++)
											addr_list.addnz(dlvr_addr_list.get(j));
										addr_list.addnz(psn_rec.MainLoc);
										if(addr_list.getCount() == 0) {
											//
											// Если у персоналии нет ни одного адреса, то все равно необходимо
											// отобразить эту персоналию в списке с пустым адресом, дабы
											// пользователь мог ее выбрать и ввести адрес, который ему назовет клиент.
											//
											addr_list.add(0L);
										}
										for(j = 0; j < addr_list.getCount(); j++) {
											const  PPID addr_id = addr_list.get(j);
											AddrByPhoneItem ap_item;
											MEMSZERO(ap_item);
											ap_item.ObjType = ea_rec.LinkObjType;
											ap_item.ObjID = ea_rec.LinkObjID;
											phone_buf.CopyTo(ap_item.Phone, sizeof(ap_item.Phone));
											STRNSCPY(ap_item.Contact, psn_rec.Name);
											ap_item.AddrID = addr_id;
											if(addr_id) {
												PsnObj.LocObj.GetAddress(addr_id, 0, temp_buf);
												temp_buf.CopyTo(ap_item.Addr, sizeof(ap_item.Addr));
											}
											AddrByPhoneList.insert(&ap_item);
										}
									}
									break;
							}
						}
					}
					if(sc_list.getCount()) {
						SCardCtrlGroup::Rec scgrec;
						getGroupData(ctlgrouSCard, &scgrec);
						if(!scgrec.SCardID) {
							scgrec.SCardID = sc_list.get(0);
							setGroupData(ctlgrouSCard, &scgrec);
						}
					}
				}
			}
		}
		DisableClusterItem(CTL_CCHKDLVR_LPHTOCRD, 1, phone_buf.IsEmpty());
		{
			const  uint c = AddrByPhoneList.getCount();
			enableCommand(cmSelAddrByPhone, BIN(c));
			if(c && immSelect)
				if(c == 1)
					SetupAddr(&AddrByPhoneList.at(0));
				else
					SelectAddrByPhone();
			return BIN(c);
		}
	}
	void   SetupDeliveryCtrls(const char * pPhone)
	{
		SString temp_buf;
		SString loc_phone;
		SString sc_phone;
		SCardTbl::Rec sc_rec;
		LockAddrModChecking = 1;
		if(Data.Flags & Data.fDelivery) {
			if(LocationCore::IsEmptyAddressRec(Data.Addr_)) {
				if(!isempty(pPhone)) {
					LocationCore::SetExField(&Data.Addr_, LOCEXSTR_PHONE, pPhone);
				}
				else if(Data.SCardID_) {
					PPPersonPacket pack;
					if(ScObj.Search(Data.SCardID_, &sc_rec) > 0 && sc_rec.PersonID && PsnObj.GetPacket(sc_rec.PersonID, &pack, 0) > 0) {
						PersonID = sc_rec.PersonID;
						LocationTbl::Rec loc_rec;
						loc_rec.ID = 0;
						if(!pack.RLoc.IsEmptyAddress())
							loc_rec = pack.RLoc;
						else if(!pack.Loc.IsEmptyAddress())
							loc_rec = pack.Loc;
						if(loc_rec.ID) {
							pack.GetPhones(1, temp_buf);
							if(temp_buf.NotEmptyS())
								LocationCore::SetExField(&loc_rec, LOCEXSTR_PHONE, temp_buf);
							//
							// Обнуляем идентификатор адреса поскольку эта запись станет собственностью чека.
							//
							loc_rec.ID = 0;
							Data.Addr_ = loc_rec;
						}
					}
				}
			}
			setCtrlLong(CTL_CCHKDLVR_ADDRID, Data.Addr_.ID);
			setCtrlLong(CTLSEL_CCHKDLVR_CITY, NZOR(Data.Addr_.CityID, DefCityID));
			setCtrlString(CTL_CCHKDLVR_ADDR, LocationCore::GetExFieldS(&Data.Addr_, LOCEXSTR_SHORTADDR, temp_buf));
			getCtrlString(CTL_CCHKDLVR_PHONE, temp_buf);
			if(temp_buf.IsEmpty()) {
				if(LocationCore::GetExFieldS(&Data.Addr_, LOCEXSTR_PHONE, loc_phone).IsEmpty() && PersonID)
					loc_phone = DlvrPhone;
				setCtrlString(CTL_CCHKDLVR_PHONE, loc_phone);
			}
			if(LocationCore::GetExFieldS(&Data.Addr_, LOCEXSTR_CONTACT, temp_buf).IsEmpty() && PersonID)
				GetPersonName(PersonID, temp_buf);
			setCtrlString(CTL_CCHKDLVR_CONTACT, temp_buf);
			SETIFZ(Data.DlvrDtm.d, getcurdate_());
			setCtrlDate(CTL_CCHKDLVR_DT, Data.DlvrDtm.d);
			setCtrlTime(CTL_CCHKDLVR_TM, Data.DlvrDtm.t);
			ComboBox * p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_CCHKDLVR_SCARD));
			if(PersonID && p_cb) {
				PPIDArray sc_list;
				ScObj.P_Tbl->GetListByPerson(PersonID, 0, &sc_list);
				if(sc_list.getCount()) {
					StrAssocArray * p_list = new StrAssocArray;
					if(p_list) {
						for(uint i = 0; i < sc_list.getCount(); i++) {
							if(ScObj.Fetch(sc_list.get(i), &sc_rec) > 0)
								p_list->Add(sc_rec.ID, sc_rec.Code);
						}
						ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData | lbtDblClkNotify);
						if(p_lw) {
							p_cb->setListWindow(p_lw);
							if(Data.SCardID_)
								p_cb->TransmitData(+1, &Data.SCardID_);
							else {
								p_cb->setInputLineText(0);
								p_cb->setUndefTag(1);
							}
						}
					}
				}
			}
			{
				int    is_attachm_phn_to_sc_allowed = 0;
				if(Data.SCardID_ && loc_phone.NotEmpty() && ScObj.Fetch(Data.SCardID_, &sc_rec) > 0) {
					is_attachm_phn_to_sc_allowed = 1;
					if(sc_rec.PersonID || sc_rec.LocID)
						is_attachm_phn_to_sc_allowed = 0;
					else {
						PPSCardPacket sc_pack;
						if(ScObj.GetPacket(Data.SCardID_, &sc_pack) > 0) {
							if(sc_pack.GetExtStrData(PPSCardPacket::extssPhone, sc_phone) > 0 && sc_phone.NotEmptyS())
								is_attachm_phn_to_sc_allowed = 0;
						}
						else
							is_attachm_phn_to_sc_allowed = 0;
					}
				}
				DisableClusterItem(CTL_CCHKDLVR_LPHTOCRD, 0, !is_attachm_phn_to_sc_allowed);
				if(!is_attachm_phn_to_sc_allowed) {
                    Data.Flags &= ~Data.fAttachPhoneToSCard;
					SetClusterData(CTL_CCHKDLVR_LPHTOCRD, Data.Flags);
				}
			}
		}
		disableCtrls(!(Data.Flags & Data.fDelivery), CTL_CCHKDLVR_ADDRID, CTLSEL_CCHKDLVR_CITY,
			CTL_CCHKDLVR_ADDR, CTL_CCHKDLVR_PHONE, CTL_CCHKDLVR_CONTACT, CTL_CCHKDLVR_DT, CTL_CCHKDLVR_TM, 0);
		LockAddrModChecking = 0;
	}

	PPID   ScsRsrvPoolID; // Персональная карта была акцептирована из резервного пула ScsRsrvPoolID.
	LocationTbl::Rec OrgLocRec;
	PPID   DefCityID;
	PPID   PersonID;
	int    LockAddrModChecking;
	SString DlvrPhone;
	const SString Channel;
	TSVector <AddrByPhoneItem> AddrByPhoneList;
	PPObjPerson PsnObj;
	PPObjSCard ScObj;
};

int CheckPaneDialog::EditMemo(const char * pDlvrPhone, const char * pChannel)
{
	int    ok = -1;
	PPID   sc_id = 0;
	const  bool preserve_delivery_flag = LOGIC(P.Eccd.Flags & P.Eccd.fDelivery);
	if(LocationCore::IsEmptyAddressRec(P.Eccd.Addr_)) {
		if(P.Eccd.Memo.IsEmpty() && PNP.CnSpeciality == PPCashNode::spDelivery)
			P.Eccd.Flags |= P.Eccd.fDelivery;
		sc_id = CSt.GetID();
	}
	CheckDlvrDialog * dlg = new CheckDlvrDialog(sc_id, pDlvrPhone, pChannel);
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(&P.Eccd)) {
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&P.Eccd)) {
				if(dlg->getSCardID(&sc_id)) {
					assert(P.Eccd.SCardID_ == sc_id);
					if(sc_id && P.Eccd.Flags & P.Eccd.fAttachPhoneToSCard) {
						SString loc_phone;
						SString ex_sc_phone;
						if(LocationCore::GetExFieldS(&P.Eccd.Addr_, LOCEXSTR_PHONE, loc_phone).NotEmptyS()) {
							PPSCardPacket sc_pack;
							if(ScObj.GetPacket(P.Eccd.SCardID_, &sc_pack) > 0) {
								sc_pack.GetExtStrData(PPSCardPacket::extssPhone, ex_sc_phone);
								if(sc_pack.Rec.LocID == 0 && sc_pack.Rec.PersonID == 0 && ex_sc_phone.IsEmpty()) {
									sc_pack.PutExtStrData(PPSCardPacket::extssPhone, loc_phone);
									if(!ScObj.PutPacket(&sc_id, &sc_pack, 1))
										PPError();
								}
							}
						}
					}
					if(sc_id != CSt.GetID())
						AcceptSCard(sc_id, 0, 0);
				}
				ok = 1;
			}
		}
		if(ok < 0) {
			SETFLAG(P.Eccd.Flags, P.Eccd.fDelivery, preserve_delivery_flag);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int CheckPaneDialog::EditPrescription()
{
	int   ok = -1;
	CCheckPacket::Prescription data/* @v11.8.11 (P.Prescr)*/;
	CCheckPacket::GetPrescription(P, data);
	TDialog * dlg = new TDialog(DLG_CCPRESCR);
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlDate(CTL_CCPRESCR_DT, data.Dt);
	dlg->setCtrlString(CTL_CCPRESCR_SERIAL, data.Serial);
	dlg->setCtrlString(CTL_CCPRESCR_NUMBER, data.Number);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		data.Dt = dlg->getCtrlDate(CTL_CCPRESCR_DT);
		dlg->getCtrlString(CTL_CCPRESCR_SERIAL, data.Serial);
		dlg->getCtrlString(CTL_CCPRESCR_NUMBER, data.Number);
		if(data.IsValid()) {
			// @v11.8.11 P.Prescr = data;
			CCheckPacket::SetPrescription(P, data); // @v11.8.11
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOK
	delete dlg;
	return ok;
}

#undef BARRIER
#define BARRIER(f) if(!Barrier()) { f; Barrier(1); }

int CheckPaneDialog::ProcessPhnSvc(int mode)
{
	int    ok = 1;
	int    pop_dlvr_pane = 0;
	SString phone_buf, channel_buf, caller_buf;
	if(P_PhnSvcClient) {
		if(mode == 1) {
			PhnSvcChannelStatusPool status_list;
			PhnSvcChannelStatus cnl_status;
			SString ringing_line;
			int   gcsr = P_PhnSvcClient->GetChannelStatus(0, status_list);
			if(!gcsr) {
				if(PhnSvcConnect() > 0)
					gcsr = P_PhnSvcClient->GetChannelStatus(0, status_list);
				else
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER|LOGMSGF_COMP);
			}
			if(gcsr) {
				if(status_list.GetCount()) {
					PPEAddrArray phn_list; // Список телефонов, находящихся в списке. Необходим для устранения дублируемых строк.
					PPIDArray ea_id_list;
					SString contact_buf;
					for(uint i = 0; !pop_dlvr_pane && i < status_list.GetCount(); i++) {
						status_list.Get(i, cnl_status);
						if(cnl_status.State == PhnSvcChannelStatus::stUp) {
							if(cnl_status.Channel.HasPrefixIAscii("SIP")) {
								if(cnl_status.ConnectedLineNum.IsEmpty() || cnl_status.ConnectedLineNum.ToLong() != 0) {
									if(PPObjPhoneService::IsPhnChannelAcceptable(PhnSvcLocalChannelSymb, cnl_status.Channel)) {
										if(PNP.CnSpeciality == PPCashNode::spDelivery && !(P.Eccd.Flags & P.Eccd.fDelivery) && IsState(sEMPTYLIST_EMPTYBUF) && !(Flags & fBarrier)) {
											pop_dlvr_pane = 1;
											if(cnl_status.ConnectedLineNum.Len() > cnl_status.CallerId.Len())
												phone_buf = cnl_status.ConnectedLineNum;
											else
												phone_buf = cnl_status.CallerId;
											channel_buf = cnl_status.Channel;
										}
									}
								}
							}
						}
						else if(cnl_status.State == PhnSvcChannelStatus::stRinging) {
							caller_buf = (cnl_status.ConnectedLineNum.Len() > cnl_status.CallerId.Len()) ?
								cnl_status.ConnectedLineNum : cnl_status.CallerId;
							if(caller_buf.Len() && !phn_list.SearchPhone(caller_buf, 0, 0)) {
								if(ringing_line.NotEmpty())
									ringing_line.CR();
								ringing_line.Cat(cnl_status.Channel).CatDiv(':', 2).Cat(caller_buf);
								phn_list.AddPhone(caller_buf);
								PsnObj.LocObj.P_Tbl->SearchPhoneIndex(caller_buf, 0, ea_id_list);
								contact_buf.Z();
								for(uint j = 0; !contact_buf.NotEmpty() && j < ea_id_list.getCount(); j++) {
									EAddrTbl::Rec ea_rec;
									if(PsnObj.LocObj.P_Tbl->GetEAddr(ea_id_list.get(j), &ea_rec) > 0) {
										if(ea_rec.LinkObjType == PPOBJ_PERSON) {
											GetPersonName(ea_rec.LinkObjID, contact_buf);
										}
										else if(ea_rec.LinkObjType == PPOBJ_LOCATION) {
											LocationTbl::Rec loc_rec;
											if(PsnObj.LocObj.Search(ea_rec.LinkObjID, &loc_rec) > 0)
												LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, contact_buf);
										}
									}
								}
								ringing_line.CatDiv(';', 2).Cat(contact_buf.SetIfEmpty("UNKNOWN"));
							}
						}
					}
				}
				if(ringing_line.NotEmpty()) {
					SMessageWindow * p_win = new SMessageWindow;
					if(p_win) {
						p_win->Open(ringing_line, 0, H(), 0, 10000, GetColorRef(SClrCadetblue),
							SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus, 0);
					}
				}
			}
		}
	}
	else
		ok = -1;
	if(pop_dlvr_pane) {
		BARRIER(EditMemo(phone_buf, channel_buf));
	}
	return ok;
}

int CheckPaneDialog::SelectGuestCount(int tableCode, long * pGuestCount)
{
	class SelectGuestCountDialog : public TDialog {
	public:
		struct Param {
			explicit Param(int tableNo = 0) : TableNo(tableNo), GuestCount(0)
			{
			}
			int    TableNo;
			int    GuestCount;
		};
		DECL_DIALOG_DATA(Param);

		SelectGuestCountDialog() : TDialog(DLG_SELGUESTCOUNT)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCMD >= cmSelGuestCount01 && TVCMD <= cmSelGuestCount15) {
				Data.GuestCount = (TVCMD - cmSelGuestCount01 + 1);
				endModal(cmOK);
			}
		}
	};
	int    ok = -1;
	SelectGuestCountDialog * dlg = 0;
	SelectGuestCountDialog::Param param(tableCode);
	if(param.TableNo) {
		if(CheckDialogPtrErr(&(dlg = new SelectGuestCountDialog))) {
			dlg->setDTS(&param);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->getDTS(&param);
				if(param.GuestCount > 0 && param.GuestCount < 1000)
					ok = 1;
				else
					param.GuestCount = 0;
			}
		}
	}
	ASSIGN_PTR(pGuestCount, param.GuestCount);
	delete dlg;
	return ok;
}

//int FASTCALL CheckPaneDialog::valid(ushort command) // @cmValidateCommand
bool CheckPaneDialog::ValidateCommand(TEvent & rEv)
{
	bool   r = true;
	assert(rEv.isCmd(cmValidateCommand));
	if(rEv.isCmd(cmValidateCommand)) {
		const  long cmd = rEv.message.infoLong;
		Flags |= fSuspSleepTimeout;
		const  int prev_state = GetState();
		const  PPID prev_agent_id = P.GetAgentID(1);
		if(cmd == cmCancel && !(Flags & fNoEdit)) {
			r = false;
			if(GetInput())
				ClearInput(0);
			else if(ResetCurrentLine() > 0)
				;
			else if((Flags & fTouchScreen) && LastCtrlID == CTL_CHKPAN_LIST)
				RemoveRow();
			else if(P.getCount()) {
				if(OperRightsFlags & orfEscCheck) {
					if(PPMessage(mfConf|mfYesNo, PPCFM_CLEARCHECK) == cmYes) {
						AcceptCheckToBeCleared();
						ClearCheck();
						if(CConfig.Flags & CCFLG_DEBUG) {
							CCheckPacket pack;
							if(SuspCheckID)
								GetCc().LoadPacket(SuspCheckID, 0, &pack);
							else {
								InitCashMachine();
								pack.Rec.SessID = P_CM ? P_CM->GetCurSessID() : 0;
								pack.Rec.PosNodeID = PNP.NodeID;
								pack.Rec.Flags |= (CCHKF_SYNC|CCHKF_NOTUSED);
								Helper_InitCcPacket(&pack, 0, 0, 0);
							}
							GetCc().WriteCCheckLogFile(&pack, 0, CCheckCore::logCleared, 1);
						}
					}
				}
				else
					PPMessage(mfInfo|mfOK, PPINF_CHKPAN_TURNCHECK);
			}
			else if(P.OrderCheckID) {
				ClearCheck();
			}
			else if((UiFlags & uifCloseWOAsk) || PPMessage(mfConf|mfYesNo, PPCFM_CLOSECCHKPANE) == cmYes)
				r = true;
			// @v12.2.9 @fix {
			if(!r)
				NegativeReplyOnValidateCommand(rEv);
			// } @v12.2.9
		}
		else {
			//r = TDialog::valid(cmd);
			TDialog::handleEvent(rEv);
			if(rEv.isCommandValidationFailed())
				r = false;
		}
		if(GetState() != prev_state || P.GetAgentID(1) != prev_agent_id)
			setupHint();
		Flags &= ~fSuspSleepTimeout;
		IdleClock = clock();
	}
	return r;
}

IMPL_HANDLE_EVENT(CheckPaneDialog)
{
	const  int  prev_state = GetState();
	const  PPID prev_agent_id = P.GetAgentID(1);
	SString temp_buf;
	if(TVCOMMAND) {
		if(TVCMD == cmValidateCommand) { // @v12.2.6
			if(!ValidateCommand(event)) {
				if(!event.isCommandValidationFailed())
					clearEvent(event);
			}
			return; // Функция ValidateCommand все сделала (включая обработку классами высшей иерархии)
		}
		else if(TVCMD == cmInputUpdated)
			IdleClock = clock(); // Не обрабатываем это сообщение, а лишь прерываем таймаут засыпания //
		else if(TVCMD == cmModalPostCreate) {
			if(!(Flags & fNoEdit) && PNP.CnExtFlags & CASHFX_NOTIFYEQPTIMEMISM) {
				if(InitCashMachine() && P_CM) {
					SString fmt_buf;
					SString msg_buf;
					LDATETIME device_dtm = ZERODATETIME;
					const int gdtr = P_CM->SyncGetDeviceTime(&device_dtm);
					/* @debug {
					(msg_buf = "GetDeviceTime").CatDiv(':', 2).Cat(gdtr).Space().Cat(device_dtm, DATF_ISO8601CENT, 0);
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					@debug */
					if(gdtr > 0) {
						const LDATETIME now_dtm = getcurdatetime_();							
						if(checkdate(device_dtm.d) && labs(diffdatetimesec(now_dtm, device_dtm)) >= 60) {
							PPLoadText(PPTXT_DEVICETIMEDIFFROMSYS, fmt_buf);
							PPFormat(fmt_buf, &msg_buf, device_dtm);
							PPTooltipMessage(msg_buf, 0, H(), 20000, GetColorRef(SClrTomato),
								SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
						}
					}
				}
			}
		}
		else if(TVCMD == cmCtlColor) {
			TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
			if(p_dc) {
				if(p_dc->Src == TDrawCtrlData::cScrollBar) {
					int ctl_id = GetDlgCtrlID(p_dc->H_Ctl);
					if(ctl_id && oneof2(ctl_id, MAKE_BUTTON_ID(CTL_CHKPAN_GRPLIST, 1), MAKE_BUTTON_ID(CTL_CHKPAN_GDSLIST, 1))) {
						::SendMessage(::GetDlgItem(H(), CTL_CHKPAN_ARROW_UP),   BM_SETSTYLE, BS_BITMAP, TRUE);
						::SendMessage(::GetDlgItem(H(), CTL_CHKPAN_ARROW_DOWN), BM_SETSTYLE, BS_BITMAP, TRUE);
					}
				}
				else {
					if(p_dc->H_Ctl == getCtrlHandle(CTL_CHKPAN_INFO)) {
						TCanvas canv(p_dc->H_DC);
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						if(Flags & fError) {
							canv.SetTextColor(GetColorRef(SClrWhite));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brErrorBkg));
							clearEvent(event);
						}
						else if(Flags & fPresent) {
							canv.SetTextColor(GetColorRef(SClrWhite));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brPresentBkg));
							clearEvent(event);
						}
					}
					else if(p_dc->H_Ctl == getCtrlHandle(CTL_CHKPAN_CAFE_STATUS)) {
						if(P.OrderCheckID) {
							TCanvas canv(p_dc->H_DC);
							::SetBkMode(p_dc->H_DC, TRANSPARENT);
							//canv.SetTextColor(GetColorRef(SClrWhite));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brOrderBkg));
							clearEvent(event);
						}
					}
					else if(p_dc->H_Ctl == getCtrlHandle(CTL_CHKPAN_TOTAL)) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						for(uint i = 0; i < P.getCount(); i++) {
							if(P.at(i).Flags & cifGift) {
								p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brTotalGift));
								clearEvent(event);
								break;
							}
						}
					}
					else if(p_dc->H_Ctl == getCtrlHandle(CTL_CHKPAN_DISCOUNT)) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						for(uint i = 0; i < P.getCount(); i++) {
							if(P.at(i).Flags & cifGift && P.at(i).Discount != 0.0) {
								p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brDiscountGift));
								clearEvent(event);
								break;
							}
						}
					}
				}
			}
			return;
		}
		else if(Flags & fAsSelector) {
			if(TVCMD == cmLBDblClk)
				TVCMD = cmOK;
			if(TVCMD == cmOK) {
				if(P_ChkPack) {
					const CCheckLineTbl::Rec * p_line = 0;
					CCheckLineTbl::Rec chk_line;
					SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
					const int cclext_id_list[] = {
						CCheckPacket::lnextSerial, 
						CCheckPacket::lnextEgaisMark, 
						CCheckPacket::lnextChZnMark,
						CCheckPacket::lnextChZnPm_ReqId, 
						CCheckPacket::lnextChZnPm_ReqTimestamp, 
						CCheckPacket::lnextChZnPm_LocalModuleInstance, 
						CCheckPacket::lnextChZnPm_LocalModuleDbVer
					};
					StrAssocArray cclext_list;
					if(SmartListBox::IsValidS(p_list)) {
						const long cur = p_list->P_Def->_curItem();
						if(cur >= 0 && cur < static_cast<long>(P_ChkPack->GetCount())) {
							chk_line = P_ChkPack->GetLineC(static_cast<uint>(cur));
							for(uint i = 0; i < SIZEOFARRAY(cclext_id_list); i++) {
								P_ChkPack->GetLineTextExt(cur+1, cclext_id_list[i], temp_buf);
								if(temp_buf.NotEmpty())
									cclext_list.Add(cclext_id_list[i], temp_buf);
							}
							p_line   = &chk_line;
						}
					}
					P_ChkPack->Z();
					if(p_line) {
						if(!P_ChkPack->InsertCclSimple(p_line->GoodsID, p_line->Quantity, intmnytodbl(p_line->Price), p_line->Dscnt))
							PPErrorZ();
						else {
							const uint ccl_idx = P_ChkPack->GetCount();
							for(uint i = 0; i < cclext_list.getCount(); i++) {
								StrAssocArray::Item cclext_list_item = cclext_list.at_WithoutParent(i);
								if(cclext_list_item.Id && !isempty(cclext_list_item.Txt)) {
									P_ChkPack->SetLineTextExt(ccl_idx, cclext_list_item.Id, cclext_list_item.Txt);
								}
							}
							P_ChkPack->Discount = p_line->Dscnt;
						}
					}
					else
						P_ChkPack->Discount = 0.0;
				}
			}
		}
		else if(TVCMD == cmOK) {
			BARRIER(ProcessEnter(0));
			clearEvent(event);
		}
	}
	TDialog::handleEvent(event);
	if(TVBROADCAST) {
		if(TVCMD == cmIdle) {
			clock_t diff = 0;
			if(P_PalmWaiter && IsState(sEMPTYLIST_EMPTYBUF))
				P_PalmWaiter->Activate();
			if(CnSleepTimeout && !(Flags & fSuspSleepTimeout)) {
				diff = clock() - IdleClock;
				if(diff >= CnSleepTimeout)
					Sleep();
			}
			if(PrintCheckClock) {
				diff = clock() - PrintCheckClock;
				if(diff >= ClearCDYTimeout) {
					PrintCheckClock = 0;
					CDispCommand(cdispcmdText, cdisptxtOpened, 0.0, 0.0);
				}
			}
			if(PhnSvcTimer.Check(0)) {
				ProcessPhnSvc(1);
			}
			if(UhttImportTimer.Check(0) && P_UhttImporter) {
				PPAlbatrossConfig acfg;
				PPAlbatrosCfgMngr::Get(&acfg);
				P_UhttImporter->InitUhttImport(acfg.Hdr.OpID, PNP.CnLocID, PNP.NodeID);
				if(P_UhttImporter->Run() > 0) {
					SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления
					SMessageWindow * p_win = new SMessageWindow;
					if(p_win) {
						SString msg_buf;
						PPLoadText(PPTXT_CHKPAN_UHTTORDER, msg_buf);
						p_win->Open(msg_buf, 0, H(), 0, 10000, GetColorRef(SClrCornsilk),
							SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus, 0);
					}
				}
			}
			return;
		}
		else if(TVCMD == cmDefault && isCurrCtlID(CTL_CHKPAN_INPUT)) {
			BARRIER(ProcessEnter(0));
		}
		else if(TVCMD == cmReleasedFocus && (Flags & fTouchScreen))
			LastCtrlID = event.message.infoView->GetId();
		else
			return;
	}
	else if(TVCOMMAND) {
		const uint ev_ctl_id = event.getCtlID();
		switch(TVCMD) {
			case cmSetupResizeParams:
				SetDlgResizeParams();
				break;
			case cmSetupTooltip:
				{
					SString tt_names;
					SString name;
					if(PPLoadTextWin(PPTXT_CHKPAN_TOOLTIPS, tt_names)) {
						for(uint idx = 0; idx < CTL_CHKPAN_NUMBUTTONS; idx++) {
							if(PPGetSubStr(tt_names, idx, name))
								SetCtrlToolTip(CTL_CHKPAN_STARTBUTTON + idx, name);
						}
					}
					if(PNP.CnSpeciality == PPCashNode::spApteka) { // @v11.8.7
						SetCtrlToolTip(CTL_CHKPAN_DIVISION, PPLoadStringS("selection_bill", name).Transf(CTRANSF_INNER_TO_OUTER));
					} 
					else if(PNP.CnSpeciality == PPCashNode::spShop) { // @v12.3.3
						//SetCtrlToolTip(CTL_CHKPAN_DIVISION, PPLoadStringS("selection_bill", name).Transf(CTRANSF_INNER_TO_OUTER)); // Так
					}
					else if(PNP.CnSpeciality == PPCashNode::spCafe) {
						SetCtrlToolTip(CTL_CHKPAN_BYPRICE, PPLoadStringS("guestcount", name).Transf(CTRANSF_INNER_TO_OUTER));
						SetCtrlToolTip(CTL_CHKPAN_DIVISION, PPLoadStringS("ftableorders", name).Transf(CTRANSF_INNER_TO_OUTER));
					}
					else if(PNP.CnSpeciality == PPCashNode::spDelivery) {
						SetCtrlToolTip(CTL_CHKPAN_DIVISION, PPLoadStringS("delivery", name).Transf(CTRANSF_INNER_TO_OUTER));
					}
				}
				break;
			case cmDrawItem:
				DrawListItem(static_cast<TDrawItemData *>(TVINFOPTR));
				break;
			case cmInputDblClk:
				if(!Barrier()) {
					if(TVINFOVIEW && TVINFOVIEW->GetId() == CTL_CHKPAN_INPUT) {
						// @v11.7.12 Для аптек по двойному клику в строке ввода теперь будет редактироваться рецепт 
						// (это не очень хорошо - если кто-то скажет, что им нужна доставка и(или) примечание, то придется пересматривать подход)
						if(PNP.CnSpeciality == PPCashNode::spApteka) { 
							EditPrescription();
						}
						else {
							EditMemo(0, 0);
						}
					}
					Barrier(1);
				}
				break;
			case cmaInsert:
				if(!Barrier()) {
					if(GetInput())
						ProcessEnter(1);
					else if(LastCtrlID == CTL_CHKPAN_GRPLIST)
						UpdateGList(1, 0);
					else if(LastCtrlID == CTL_CHKPAN_GDSLIST) {
						SelectGoods__(sgmInnerGoodsList);
					}
					else if(oneof3(GetState(), sEMPTYLIST_BUF, sLIST_BUF, sLISTSEL_BUF) && LastCtrlID != CTL_CHKPAN_LIST)
						ProcessEnter(1);
					Barrier(1);
				}
				break;
			case cmaDelete:
				if(ev_ctl_id == CTL_CHKPAN_LIST) {
					BARRIER(RemoveRow());
				}
				break;
			case cmaEdit:
				if(ev_ctl_id == CTL_CHKPAN_LIST) {
					int  no_edit = 1;
				}
				break;
			case cmaSelect:
				if(!Barrier()) {
					GroupList.TopID = 0;
					UpdateGList(0, 0);
					Barrier(1);
				}
				break;
			case cmaLevelUp:
				if(!Barrier()) {
					PPID   _sel_id = GroupList.TopID;
					if(UiFlags & uifOneGroupLevel && GroupList.TopID) {
						if(ActiveListID == CTL_CHKPAN_GDSLIST) {
							_sel_id = 0;
						}
						else {
							const GrpListItem * p_item = GroupList.Get(GroupList.TopID, 0);
							GroupList.TopID = p_item ? p_item->ParentID : 0;
						}
					}
					UpdateGList(0, _sel_id);
					Barrier(1);
				}
				break;
			case cmaAltSelect:
				BARRIER(UpdateGList(-1, AltGoodsGrpID));
				break;
			case cmLBItemSelected:
			case cmLBDblClk:
				if(!Barrier()) {
					if(ev_ctl_id == CTL_CHKPAN_GRPLIST) {
						PPID   grp_id = 0;
						int    r = SelectGroup(&grp_id);
						if(r == 2)
							UpdateGList(-1, grp_id);
						else if(r == 1)
							UpdateGList(0, grp_id);
					}
					else if(ev_ctl_id == CTL_CHKPAN_GDSLIST) {
						SelectGoods__(sgmInnerGoodsList);
					}
					else if(ev_ctl_id == CTL_CHKPAN_LIST) {
						if(!(Flags & fNoEdit)) {
							SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
							const long cur = SmartListBox::IsValidS(p_list) ? p_list->P_Def->_curItem() : -1;
							if(cur >= 0 && cur < P.getCountI()) {
								enum {
									rowopNone = 0,
									rowopUp,
									rowopDown,
									rowopDoGroup,
									rowopDelete,
									rowopPrintLabel // @v11.4.7
								};
								int    r = -1;
								const  CCheckItem & r_cur_item = P.at(cur);
								long   verb = rowopNone;
								int8   queue = r_cur_item.Queue;
								TDialog * dlg = new TDialog(DLG_CHKPANROWOP);
								if(CheckDialogPtrErr(&dlg)) {
									dlg->AddClusterAssocDef(CTL_CHKPANROWOP_VERB, 0, rowopNone);
									dlg->AddClusterAssoc(CTL_CHKPANROWOP_VERB, 1, rowopUp);
									dlg->AddClusterAssoc(CTL_CHKPANROWOP_VERB, 2, rowopDown);
									dlg->AddClusterAssoc(CTL_CHKPANROWOP_VERB, 3, rowopDoGroup);
									dlg->AddClusterAssoc(CTL_CHKPANROWOP_VERB, 4, rowopDelete);
									dlg->AddClusterAssoc(CTL_CHKPANROWOP_VERB, 5, rowopPrintLabel); // @v11.4.7
									dlg->SetClusterData(CTL_CHKPANROWOP_VERB, verb);
									dlg->SetupSpin(CTLSPIN_CHKPANROWOP_QUEUE, CTL_CHKPANROWOP_QUEUE, 0, 20, queue);
									dlg->setCtrlData(CTL_CHKPANROWOP_QUEUE, &queue);
									if(cur == 0) {
										dlg->DisableClusterItem(CTL_CHKPANROWOP_VERB, 1, 1);
										dlg->DisableClusterItem(CTL_CHKPANROWOP_VERB, 3, 1);
									}
									if((cur+1) >= static_cast<long>(P.getCount())) {
										dlg->DisableClusterItem(CTL_CHKPANROWOP_VERB, 2, 1);
									}
									if(!(OperRightsFlags & orfEscChkLine) || cur < 0 || cur >= static_cast<long>(P.getCount())) {
										dlg->DisableClusterItem(CTL_CHKPANROWOP_VERB, 4, 1);
									}
									while(r < 0 && ExecView(dlg) == cmOK) {
										verb = dlg->GetClusterData(CTL_CHKPANROWOP_VERB);
										dlg->getCtrlData(CTL_CHKPANROWOP_QUEUE, &queue);
										r = 1;
										if(queue != r_cur_item.Queue) {
											if(P.SetQueue(cur, queue) > 0)
												r = 2;
										}
										switch(verb) {
											case rowopUp:
												if(P.MoveUp(cur) > 0)
													r = 2;
												break;
											case rowopDown:
												if(P.MoveDown(cur) > 0)
													r = 2;
												break;
											case rowopDoGroup:
												if(P.Grouping(cur) > 0)
													r = 2;
												break;
											case rowopDelete:
												RemoveRow();
												break;
											case rowopPrintLabel: // @v11.4.7
												{
													RetailGoodsInfo rgi;
													rgi.Qtty = fabs(r_cur_item.Quantity);
													if(GObj.GetRetailGoodsInfo(r_cur_item.GoodsID, 0, 0, 0, 0.0, &rgi, PPObjGoods::rgifConcatQttyToCode) > 0)
														BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, /*Rec.LabelPrinterID*/0, 0/*silent*/);
												}
												break;
										}
									}
								}
								if(r == 2)
									OnUpdateList(0);
								delete dlg;
								/*
								TMenuPopup menu;
								if(cur > 0) {
									menu.Add("@moveup",   cmUp);
								}
								if((cur+1) < static_cast<long>(P.getCount()))
									menu.Add("@movedown", cmDown);
								if(cur > 0)
									menu.Add("@dogroup",  cmGrouping);
								if(OperRightsFlags & orfEscChkLine && cur >= 0 && cur < static_cast<long>(P.getCount())) {
									menu.AddSeparator();
									menu.Add("@delete",   cmaDelete);
								}
								if(menu.GetCount()) {
									int    cmd = menu.Execute(*this, TMenuPopup::efRet);
									switch(cmd) {
										case cmUp:
											if(P.MoveUp(cur) > 0)
												updateList();
											break;
										case cmDown:
											if(P.MoveDown(cur) > 0)
												updateList();
											break;
										case cmGrouping:
											if(P.Grouping(cur) > 0)
												updateList();
											break;
										case cmaDelete:
											RemoveRow();
											break;
									}
								}
								*/
							}
						}
					}
					Barrier(1);
				}
				break;
			case cmLBItemFocused:
				if(ActiveListID == CTL_CHKPAN_GRPLIST) {
					SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GRPLIST));
					CALLPTRMEMB(p_list, getCurID(&SelGoodsGrpID));
				}
				if(event.isCtlEvent(CTL_CHKPAN_LIST)) {
					SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
					const long pos = SmartListBox::IsValidS(p_list) ? p_list->P_Def->_curItem() : -1;
					if(pos < P.getCountI()) {
						ViewStoragePlaces(P.at(pos).GoodsID);
						if(Flags & fNoEdit)
							SetupInfo(0);
					}
				}
				break;
			case cmPrev:
			case cmNext:
				if(ActiveListID) {
					if(!Barrier()) {
						SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView((ushort)ActiveListID));
						CALLPTRMEMB(p_list, Scroll((TVCMD == cmPrev) ? SB_LINEUP : SB_LINEDOWN, 0));
						Barrier(1);
					}
				}
				break;
			case cmCash:
				if(!Barrier()) {
					if(PNP.AllowedPaymentTypes & (1 << cpmCash)) { // @v12.3.12 (condition)
						ClearInput(0);
						Flags &= ~fBankingPayment; // @bank
						ProcessEnter(0);
						Barrier(1);
					}
				}
				break;
			case cmBanking:
				if(!Barrier()) {
					if(PNP.AllowedPaymentTypes & (1 << cpmBank)) { // @v12.3.12 (condition)
						ClearInput(0);
						Flags |= fBankingPayment; // @bank
						ProcessEnter(0);
						Barrier(1);
					}
				}
				break;
			case cmChkPanPrint:
				if(!Barrier()) {
					if(Flags & fTouchScreen) {
						if(Flags & fPrintSlipDoc)
							PrintSlipDocument();
						else
							PrintToLocalPrinters(0, false/*ignoreNonZeroAgentReq*/);
					}
					else if(Flags & fNoEdit)
						Print(0, 0, 0);
					else
						PrintToLocalPrinters(0, false/*ignoreNonZeroAgentReq*/);
					Barrier(1);
				}
				break;
			case cmQuantity:      BARRIER(AcceptQuantity()); break;
			case cmRetCheck:      
				// @v12.2.9 BARRIER(SetupRetCheck(!F(fRetCheck))); 
				BARRIER(SetupRetCheck(!(IsCurrentOp(CCOP_RETURN) || IsCurrentOpCorrection()))); // @v12.2.9
				break;
			case cmSelSCard:      BARRIER(AcceptSCard(0, 0, (Flags & fWaitOnSCard) ? ascfFromInput : ascfExtPane)); break;
			case cmChkPanSuspend: BARRIER(SuspendCheck()); break;
			case cmToLocPrinters: BARRIER(PrintToLocalPrinters(-1, false/*ignoreNonZeroAgentReq*/)); break;
			case cmSelTable:      BARRIER(SelectTable()); break;
			case cmSelModifier:   BARRIER(SelectGoods__(sgmModifier)); break;
			case cmSelGoods:      BARRIER(SelectGoods__(sgmNormal)); break;
			case cmCashOper:
				if(!Barrier() || (Flags & fOnlyReports))
					Barrier((PrintCashReports() < 0) ? 0 : 1);
				break;
			case cmByPrice: /* cmChkPanF2 */
				if(!Barrier()) {
					if(PNP.CnSpeciality == PPCashNode::spCafe) {
						//
						// Выбор количества гостей за столом (P.GuestCount)
						//
						if(P.TableCode) {
							long   guest_count = 0;
							const  bool is_input = GetInput();
							if(!is_input) {
								if(SelectGuestCount(P.TableCode, &guest_count) > 0) {
									P.GuestCount = (uint16)guest_count;
									SetupInfo(0);
								}
							}
							else {
								if(Input.IsDec()) {
									guest_count = Input.ToLong();
									if(guest_count > 0 && guest_count < 1000) {
										P.GuestCount = (uint16)guest_count;
										SetupInfo(0);
									}
								}
								ClearInput(0);
							}
						}
					}
					else if(Flags & fTouchScreen) {
						if(PNP.AbstractGoodsID)
							SelectGoods__(sgmAbstractSale);
						else
							UpdateGList(-2, 0);
					}
					else {
						SelectGoods__(PNP.AbstractGoodsID ? sgmAbstractSale : sgmByPrice);
					}
					Barrier(1);
				}
				break;
			case cmGoodsDiv: /* cmChkPanF1 */
				if(!Barrier()) {
					const PPEquipConfig & r_cfg = CsObj.GetEqCfg();
					if(oneof2(PNP.CnSpeciality, PPCashNode::spApteka, PPCashNode::spShop) && (r_cfg.ChkPanImpOpID && r_cfg.ChkPanImpBillTagID)) { // @v11.8.7 // @v12.3.3 spShop
						if(IsState(sEMPTYLIST_EMPTYBUF)) {
							PPID   bill_id = 0;
							PPLoadString("selection_bill_forimport", temp_buf);
							if(SelectBill(&bill_id, temp_buf) > 0 && bill_id) {
								PPBillPacket bpack;
								if(BillObj->ExtractPacket(bill_id, &bpack) > 0) {
									CCheckPacket ccpack;
									PPBillPacket::ConvertToCCheckParam param;
									param.PosNodeID = PNP.NodeID;
									param.LocID = PNP.CnLocID;
									const int ctcr = bpack.ConvertToCheck2(param, &ccpack, 0);
									if(ctcr > 0) {
										const S_GUID cc_uuid(SCtrGenerate_);
										S_GUID bill_uuid;
										//bool bill_uuid_tag_settled = false;
										if(!bpack.GetGuid(bill_uuid)) {
											bpack.GenerateGuid(&bill_uuid);
											ObjTagItem tag;
											if(tag.SetGuid(PPTAG_BILL_UUID, &bill_uuid) && PPRef->Ot.PutTag(PPOBJ_BILL, bill_id, &tag, 1)) {
												;//bill_uuid_tag_settled = true;
											}
											else {
												bill_uuid.Z();
												PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_COMP);
											}
										}
										ccpack.SetGuid(&cc_uuid);
										if(/*bill_uuid_tag_settled*/!!bill_uuid) {
											bill_uuid.ToStr(S_GUID::fmtIDL, temp_buf);
											ccpack.PutExtStrData(CCheckPacket::extssLinkBillUuid, temp_buf);
										}
										LoadCheck(&ccpack, false/*makeRetCheck*/, false);
									}
								}
							}
						}
					}
					else if(PNP.CnSpeciality == PPCashNode::spCafe) {
						if(IsState(sEMPTYLIST_EMPTYBUF)) {
							SelCheckListDialog::AddedParam param(0, P.TableCode, P.GetAgentID(), OperRightsFlags);
							const uint dlg_id = (DlgFlags & fLarge) ? DLG_ORDERCHECKS_L : DLG_ORDERCHECKS;
							SelCheckListDialog * dlg = new SelCheckListDialog(dlg_id, static_cast<TSVector <CCheckViewItem> *>(0), 0, this, &param);
							if(CheckDialogPtrErr(&dlg)) {
								if(ExecView(dlg) == cmNewCheck) {
									_SelCheck sc;
									if(dlg->getDTS(&sc)) {
										SetupOrder(sc.CheckID);
									}
								}
								delete dlg;
							}
						}
					}
					else if(PNP.CnSpeciality == PPCashNode::spDelivery) {
						EditMemo(0, 0);
					}
					else
						AcceptDivision();
					Barrier(1);
				}
				break;
			case cmMemo: BARRIER(EditMemo(0, 0)); break;
			case cmSysInfo:
				if(P_ChkPack) {
					CCheckPacket pack(*P_ChkPack);
					PPViewCCheck::EditCCheckSystemInfo(pack);
				}
				break;
			case cmCcExtList: // @v12.4.1
				if(P_ChkPack) {
					uint   tab_count = 0;
					const  SIntToSymbTabEntry * p_tab = CCheckPacket::GetExtssNameSymbTab(&tab_count);
					ExtStrContainerListDialog * dlg = new ExtStrContainerListDialog(DLG_CCEXTLIST, CTL_CCEXTLIST_LIST, 0, true, p_tab, tab_count);
					if(CheckDialogPtrErr(&dlg)) {
						dlg->setDTS(P_ChkPack);
						ExecViewAndDestroy(dlg);
					}
				}
				break;
			case cmCcLnExtList: // @v12.4.1
				if(P_ChkPack) {
					SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
					const long pos = SmartListBox::IsValidS(p_list) ? p_list->P_Def->_curItem() : -1;
					if(pos >= 0 && pos < static_cast<long>(P_ChkPack->GetCount())) {
						PPExtStrContainer es_container;
						uint   tab_count = 0;
						const  SIntToSymbTabEntry * p_tab = CCheckPacket::GetLnExtssNameSymbTab(&tab_count);
						if(tab_count && p_tab) {
							for(uint i = 0; i < tab_count; i++) {
								PPLoadString(p_tab[i].P_Symb, temp_buf);
								if(temp_buf.NotEmpty()) {
									const int cclnext_id = p_tab[i].Id;
									P_ChkPack->GetLineTextExt(pos+1, cclnext_id, temp_buf);
									es_container.PutExtStrData(cclnext_id, temp_buf);
								}
							}
							ExtStrContainerListDialog * dlg = new ExtStrContainerListDialog(DLG_CCEXTLIST, CTL_CCEXTLIST_LIST, "@cclnextlist", true, p_tab, tab_count);
							if(CheckDialogPtrErr(&dlg)) {
								dlg->setDTS(&es_container);
								ExecViewAndDestroy(dlg);
							}
						}
					}
				}
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF12:
				if(SlDebugMode::CT()) {
					SString file_name;
					if(ExportCurrentState(temp_buf)) {
						PPGetFilePath(PPPATH_OUT, "CPosProcessorState.xml", file_name);
						SFile f_out(file_name, SFile::mWrite);
						f_out.WriteLine(temp_buf);
					}
					else
						PPError();
					if(ExportCTblList(temp_buf)) {
						PPGetFilePath(PPPATH_OUT, "CPosCTableList.xml", file_name);
						SFile f_out(file_name, SFile::mWrite);
						f_out.WriteLine(temp_buf);
					}
					else
						PPError();
					if(ExportCCheckList(0, temp_buf)) {
						PPGetFilePath(PPPATH_OUT, "CPosCCheckList.xml", file_name);
						SFile f_out(file_name, SFile::mWrite);
						f_out.WriteLine(temp_buf);
					}
					else
						PPError();
				}
				break;
			case kbCtrlF12:
				{
					SString mark;
					SString reconstructed_original;
					if(PPChZnPrcssr::InputMark(mark, &reconstructed_original, 0) > 0) {
						CCheckPacket::PreprocessChZnCodeResult chzn_result;
						if(InitCashMachine() && P_CM) {
							SString msg_buf;
							//reconstructed_original.Insert(0, "\xE8"); // @debug
							//PPChZnPrcssr::ReconstructOriginalChZnCode(const GtinStruc & rS, SString & rBuf)
							if(P_CM->SyncPreprocessChZnCode(PPSyncCashSession::ppchzcopInit, /*mark*/reconstructed_original, 1.0, 0/*uomId*/, 0, chzn_result)) {
								int r1 = P_CM->SyncPreprocessChZnCode(PPSyncCashSession::ppchzcopVerify, /*mark*/reconstructed_original, 1.0, 0/*uomId*/, 0, chzn_result);
								// (Оказалось, что не надо) P_CM->SyncPreprocessChZnCode(PPSyncCashSession::ppchzcopCancel, /*mark*/reconstructed_original, 1.0, 0/*uomId*/, 0, chzn_result);
								msg_buf.CatEq("SyncPreprocessChZnCode-result", r1).CR();
								msg_buf.Tab().CatEq("check-result", chzn_result.CheckResult).CR();
								msg_buf.Tab().CatEq("reason", chzn_result.Reason).CR();
								msg_buf.Tab().CatEq("processing-result", chzn_result.ProcessingResult).CR();
								msg_buf.Tab().CatEq("processing-code", chzn_result.ProcessingCode).CR();
								msg_buf.Tab().CatEq("status", chzn_result.Status).CR();
								// @v12.4.1 {
								chzn_result.Z();
								int r2 = P_CM->SyncPreprocessChZnCode(PPSyncCashSession::ppchzcopVerifyOffline, /*mark*/reconstructed_original, 1.0, 0/*uomId*/, 0, chzn_result);
								msg_buf.CR();
								msg_buf.CatEq("SyncPreprocessChZnCode(offline)-result", r2).CR();
								msg_buf.Tab().CatEq("check-result", chzn_result.CheckResult).CR();
								msg_buf.Tab().CatEq("reason", chzn_result.Reason).CR();
								msg_buf.Tab().CatEq("processing-result", chzn_result.ProcessingResult).CR();
								msg_buf.Tab().CatEq("processing-code", chzn_result.ProcessingCode).CR();
								msg_buf.Tab().CatEq("status", chzn_result.Status).CR();
								// } @v12.4.1 
							}
							PPChZnPrcssr::InputMark(mark, &reconstructed_original, msg_buf);
						}						
					}
				}
				break;
			case kbF2:      BARRIER(SelectGoods__(sgmNormal)); break;
			case kbShiftF2: BARRIER(SelectGoods__(sgmAllByName)); break;
			case kbF3:      BARRIER(AcceptSCard(0, 0, (Flags & fWaitOnSCard) ? ascfFromInput : ascfExtPane)); break;
			case kbF4:      BARRIER(SelectGoods__(PNP.AbstractGoodsID ? sgmAbstractSale : sgmByPrice)); break;
			case kbF5:      BARRIER(SetupRowByScale()); break;
			case kbCtrlF5:  
				// @v12.2.9 BARRIER(SetupRetCheck(!F(fRetCheck))); 
				BARRIER(SetupRetCheck(!(IsCurrentOp(CCOP_RETURN) || IsCurrentOpCorrection()))); // @v12.2.9
				break;
			case kbF6:      BARRIER(AcceptQuantity()); break;
			case kbF7:      BARRIER(PrintCheckCopy()); break;
			case kbCtrlF7:  BARRIER(PrintSlipDocument()); break;
			case kbAltF7:   BARRIER(PrintToLocalPrinters(1, false/*ignoreNonZeroAgentReq*/)); break;
			case kbShiftF7: BARRIER(PrintToLocalPrinters(0, false/*ignoreNonZeroAgentReq*/)); break;
			case kbF8:      BARRIER(SuspendCheck()); break;
			case kbF9:      BARRIER(AcceptDivision()); break;
			case kbF10:     
				// @v11.7.12 Для аптек по двойному клику в строке ввода теперь будет редактироваться рецепт 
				// (это не очень хорошо - если кто-то скажет, что им нужна доставка и(или) примечание, то придется пересматривать подход)
				if(PNP.CnSpeciality == PPCashNode::spApteka) { 
					BARRIER(EditPrescription());
				}
				else {
					BARRIER(EditMemo(0, 0));
				}
				break;
			case kbF11:     BARRIER(ResetOperRightsByKey()); break;
			case kbCtrlF3:
				if(InitCashMachine() && P_CM->GetNodeData().Flags & CASHF_OPENBOX)
					P_CM->SyncOpenBox();
				break;
			case kbCtrlF4: // @v11.0.9
				BARRIER(AcceptManualDiscount());
				break;
			case kbCtrlF6:
				if(!Barrier()) {
					if(P.HasCur() && P.GetCur().GoodsID) {
						const  PPID goods_id = P.GetCur().GoodsID;
						GObj.ViewUhttGoodsRestList(goods_id);
					}
					Barrier(1);
				}
				break;
			case kbCtrlF8: // Просмотр информации о персональной карте
				if(!Barrier()) {
					PPID   scard_id = CSt.GetID();
					ViewSCardInfo(&scard_id, PNP.NodeID, 1);
					Barrier(1);
				}
				break;
			case kbCtrlF9:
				if(!Barrier() || (Flags & fOnlyReports))
					Barrier((PrintCashReports() < 0) ? 0 : 1);
				break;
			case kbDown:
			case kbPgDn:
			case kbUp:
			case kbPgUp:
				if(ActiveListID) {
					SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView((ushort)ActiveListID));
					if(p_list) {
						int   scroll_code = -1;
						switch(TVKEY) {
							case kbUp:    scroll_code = SB_LINEUP;   break;
							case kbDown:  scroll_code = SB_LINEDOWN; break;
							case kbPgDn:  scroll_code = SB_PAGEDOWN; break;
							case kbPgUp:  scroll_code = SB_PAGEUP;   break;
						}
						if(scroll_code != -1)
							p_list->Scroll(scroll_code, 0);
					}
				}
				break;
			default:
				return;
		}
	}
	else
		return;
	if(!(Flags & fBarrier) && (GetState() != prev_state || P.GetAgentID(1) != prev_agent_id))
		setupHint();
	clearEvent(event);
	IdleClock = clock();
}

#undef BARRIER

void CheckPaneDialog::DrawListItem(TDrawItemData * pDrawItem)
{
	if(pDrawItem && pDrawItem->P_View) {
		PPID   list_ctrl_id = pDrawItem->P_View->GetId();
		if(list_ctrl_id == ActiveListID) {
			HDC    h_dc = pDrawItem->H_DC;
			HFONT  h_fnt_def  = 0;
			HBRUSH h_br_def   = 0;
			HPEN   h_pen_def  = 0;
			COLORREF clr_prev = 0;
			SmartListBox * p_lbx = static_cast<SmartListBox *>(pDrawItem->P_View);
			RECT   rc = pDrawItem->ItemRect;
			SString temp_buf;
			if(list_ctrl_id == CTL_CHKPAN_GDSLIST) {
				if(pDrawItem->ItemAction & TDrawItemData::iaBackground) {
					FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(brOdd)));
					pDrawItem->ItemAction = 0; // Мы перерисовали фон
				}
				else if(pDrawItem->ItemID != 0xffffffff) {
					h_fnt_def = static_cast<HFONT>(::SelectObject(h_dc, static_cast<HFONT>(Ptb.Get(fontGoodsList))));
					p_lbx->getText((long)pDrawItem->ItemData, temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
					if(pDrawItem->ItemState & (ODS_FOCUS|ODS_SELECTED)) {
						h_br_def = static_cast<HBRUSH>(::SelectObject(h_dc, Ptb.Get(brSel)));
						clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrFocus));
						if(UiFlags & uifTSGGroupsAsButtons) {
							SInflateRect(rc, -1, -(1 + GoodsListEntryGap / 4));
							::RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
							rc.left += 4;
						}
						else
							::FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(brSel)));
					}
					else {
						int  draw_odd = pDrawItem->ItemID % 2;
						if(UiFlags & uifTSGGroupsAsButtons) {
							clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrGrp));
							h_br_def = static_cast<HBRUSH>(::SelectObject(h_dc, Ptb.Get(brGrp)));
							SInflateRect(rc, -1, -(1 + GoodsListEntryGap / 4));
							::RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
							rc.left += 4;
						}
						else {
							h_br_def = static_cast<HBRUSH>(::SelectObject(h_dc, Ptb.Get(draw_odd ? brOdd : brEven)));
							clr_prev = SetBkColor(h_dc, Ptb.GetColor(draw_odd ? clrOdd : clrEven));
							::FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(draw_odd ? brOdd : brEven)));
						}
					}
					::DrawText(h_dc, SUcSwitch(temp_buf), temp_buf.Len(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE); // @unicodeproblem
				}
			}
			else if(list_ctrl_id == CTL_CHKPAN_GRPLIST) {
				uint   level = 0;
				if(pDrawItem->ItemAction & TDrawItemData::iaBackground) {
					clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrGrp));
					::FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(brGrp)));
					pDrawItem->ItemAction = 0; // Мы перерисовали фон
				}
				else if(pDrawItem->ItemID != 0xffffffff) {
					GrpListItem gli;
					if(SmartListBox::IsValidS(p_lbx)) {
						uint   pos = 0;
						const  void * p_row_data = p_lbx->P_Def->getRow_(static_cast<long>(pDrawItem->ItemData));
						PPID   grp_id = p_row_data ? *static_cast<const long *>(p_row_data) : 0;
						if(GroupList.Get(grp_id, &pos))
							gli = GroupList.at(pos);
					}
					h_fnt_def = static_cast<HFONT>(SelectObject(h_dc, Ptb.Get(fontGoodsList)));
					p_lbx->getText(static_cast<long>(pDrawItem->ItemData), temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
					if(pDrawItem->ItemState & (ODS_FOCUS | ODS_SELECTED)) {
						clr_prev = SetBkColor(h_dc, Ptb.GetColor(clrFocus));
						h_br_def = static_cast<HBRUSH>(SelectObject(h_dc, Ptb.Get(brGrpSel)));
						if(UiFlags & uifTSGGroupsAsButtons) {
							SInflateRect(rc, -1, -(1 + GoodsListEntryGap / 4));
							RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
						}
						else
							FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get(brGrpSel)));
					}
					else {
						clr_prev = SetBkColor(h_dc, Ptb.GetColor((gli.Flags & GrpListItem::fFolder) ? clrParent : clrGrp));
						h_br_def = static_cast<HBRUSH>(SelectObject(h_dc, Ptb.Get((gli.Flags & GrpListItem::fFolder) ? brGrpParent : brGrp)));
						if(UiFlags & uifTSGGroupsAsButtons) {
							SInflateRect(rc, -1, -(1 + GoodsListEntryGap / 4));
							RoundRect(h_dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
						}
						else
							FillRect(h_dc, &rc, static_cast<HBRUSH>(Ptb.Get((gli.Flags & GrpListItem::fFolder) ? brGrpParent : brGrp)));
					}
					rc.left += gli.Level * 24 + ((UiFlags & uifTSGGroupsAsButtons) ? 4 : 0);
					::DrawText(h_dc, SUcSwitch(temp_buf), temp_buf.Len(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
				}
			}
			if(h_fnt_def)
				::SelectObject(h_dc, h_fnt_def);
			if(h_br_def)
				::SelectObject(h_dc, h_br_def);
			if(h_pen_def)
				::SelectObject(h_dc, h_pen_def);
			if(clr_prev)
				::SetBkColor(h_dc, clr_prev);
		}
		else
			pDrawItem->ItemAction = 0; // Список не активен - строку не рисуем
	}
}

void CheckPaneDialog::ResetListWindows(int listCtrlID)
{
	const int   sx  = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
	const int   sy  = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME);
	const int   cy  = GetSystemMetrics(SM_CYCAPTION);
	const int   vsx = GetSystemMetrics(SM_CXVSCROLL);
	const int   vsy = GetSystemMetrics(SM_CYVSCROLL);
	RECT  dlg_rect, cr;
	HWND  ctrl_wnd = ::GetDlgItem(H(), listCtrlID);
	::GetWindowRect(H(), &dlg_rect);
	const int adj_left = (dlg_rect.left + sx);
	const int adj_top  = (dlg_rect.top + sy + cy);
	GetWindowRect(ctrl_wnd, &cr);
	cr.right -= vsx;
	MoveWindow(ctrl_wnd, cr.left - adj_left, cr.top - adj_top, cr.right - cr.left, cr.bottom - cr.top, 1);
	ctrl_wnd = GetDlgItem(H(), MAKE_BUTTON_ID(listCtrlID, 1));
	GetWindowRect(ctrl_wnd, &cr);
	cr.left -= vsx;
	MoveWindow(ctrl_wnd, cr.left - adj_left, cr.top + vsy * 2 - adj_top, cr.right - cr.left, cr.bottom - cr.top - vsy * 4, 1);
	ctrl_wnd = GetDlgItem(H(), CTL_CHKPAN_ARROW_UP);
	MoveWindow(ctrl_wnd, cr.left - adj_left, cr.top - adj_top, cr.right - cr.left, vsy * 3, 1);
	ctrl_wnd = GetDlgItem(H(), CTL_CHKPAN_ARROW_DOWN);
	MoveWindow(ctrl_wnd, cr.left - adj_left, cr.bottom - vsy * 3 - adj_top, cr.right - cr.left, vsy * 3, 1);
}

int CheckPaneDialog::SetDlgResizeParams()
{
	int    ok = -1;
	if(!(Flags & fNoEdit)) {
		if(Flags & fTouchScreen) {
			PPID   sb_id = MAKE_BUTTON_ID(CTL_CHKPAN_GRPLIST, 1);
			SString font_face;
			PPGetSubStr(PPTXT_FONTFACE, /*PPFONT_MSSANSSERIF*/PPFONT_ARIAL, font_face);
			SetCtrlFont(CTL_CHKPAN_LIST, font_face, /*16*//*22*/18);
			ResetListWindows(CTL_CHKPAN_GDSLIST);
			ResetListWindows(CTL_CHKPAN_GRPLIST);
			SetCtrlResizeParam(CTL_CHKPAN_LIST,        0, 0, CTL_CHKPAN_GDSLIST, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GDSLIST,     CTL_CHKPAN_LIST, 0, 0, 0, crfResizeable);
			SetCtrlResizeParam(MAKE_BUTTON_ID(CTL_CHKPAN_GDSLIST, 1), -1, 0, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPLIST,     CTL_CHKPAN_GDSLIST, 0, 0, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(sb_id,                  -1, 0, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_ARROW_UP,    sb_id,  0, sb_id, -1, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_ARROW_DOWN,  sb_id, -1, sb_id,  0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SELMODIFIER, CTL_CHKPAN_GRPNAME,     0, CTL_CHKPAN_LEVELUP,  -1, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_LEVELUP,     CTL_CHKPAN_SELMODIFIER, 0, CTL_CHKPAN_ARROW_UP, -1, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPNAME,     CTL_CHKPAN_GDSLIST,   0, 0, -1, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX1,     0,                    -1, CTL_CHKPAN_LIST,  0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GOODS,       0,                    -1, CTL_CHKPAN_LIST,  0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_QTTY,        0,                    -1, CTL_CHKPAN_INPUT, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_PRICE,       CTL_CHKPAN_INFO,      -1, CTL_CHKPAN_SUM, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SUM,         CTL_CHKPAN_PRICE,     -1, CTL_CHKPAN_GOODS, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_TOTAL,       CTL_CHKPAN_TOLOCPRN,  -1, CTL_CHKPAN_LIST, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_DISCOUNT,    CTL_CHKPAN_TOLOCPRN,  -1, CTL_CHKPAN_LIST, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SETQTTY,     CTL_CHKPAN_ENTER,     -1, CTL_CHKPAN_ENTER, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SCARD,       CTL_CHKPAN_ENTER,     -1, CTL_CHKPAN_ENTER, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_BYPRICE,     CTL_CHKPAN_CANCEL,    -1, CTL_CHKPAN_CANCEL, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SELTABLE,    CTL_CHKPAN_CANCEL,    -1, CTL_CHKPAN_CANCEL, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_ENTER,       CTL_CHKPAN_LIST,      -1, CTL_CHKPAN_CANCEL, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(STDCTL_OKBUTTON,        CTL_CHKPAN_ENTER,     CTL_CHKPAN_ENTER, CTL_CHKPAN_ENTER, CTL_CHKPAN_ENTER, CRF_LINK_ALL | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CANCEL,      CTL_CHKPAN_ENTER,     -1, CTL_CHKPAN_CASH, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_RETCHECK,    CTL_CHKPAN_CASH,      -1, CTL_CHKPAN_CASH, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SUSPCHECK,   CTL_CHKPAN_CASH,      -1, CTL_CHKPAN_CASH, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_DIVISION,    CTL_CHKPAN_BANKING,   -1, CTL_CHKPAN_BANKING, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CASHOPER,    CTL_CHKPAN_BANKING,   -1, CTL_CHKPAN_BANKING, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CASH,        CTL_CHKPAN_CANCEL,    -1, CTL_CHKPAN_BANKING, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_BANKING,     CTL_CHKPAN_CASH,      -1, CTL_CHKPAN_TOLOCPRN, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_TOLOCPRN,    CTL_CHKPAN_BANKING,   -1, CTL_CHKPAN_TODEFPRN, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_TODEFPRN,    CTL_CHKPAN_TOLOCPRN,  -1, CTL_CHKPAN_SELGDSGRP, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SELGDSGRP,   CTL_CHKPAN_TODEFPRN,  -1, CTL_CHKPAN_GRPBYDEF, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBYDEF,    CTL_CHKPAN_SELGDSGRP, -1, CTL_CHKPAN_LIST, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX2,     CTL_CHKPAN_CASH,      -1, CTL_CHKPAN_TODEFPRN, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX3,     CTL_CHKPAN_SELGDSGRP, -1, CTL_CHKPAN_LIST, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX4,     0,                    -1, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_INPUT,       0,                    -1, CTL_CHKPAN_CASH, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_INFO,        CTL_CHKPAN_BANKING,   -1, CTL_CHKPAN_LIST, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CAFE_STATUS, CTL_CHKPAN_GDSLIST,   -1, 0, 0, crfLinkLeft | crfResizeable);
		}
		else {
			SetCtrlResizeParam(CTL_CHKPAN_LIST, 0, 0, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX1, 0, -1, CTL_CHKPAN_TOTAL, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_TOTAL, CTL_CHKPAN_GRPBOX1, -1, 0, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_DISCOUNT, CTL_CHKPAN_TOTAL, -1, CTL_CHKPAN_TOTAL, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GOODS, CTL_CHKPAN_GRPBOX1, -1, CTL_CHKPAN_GRPBOX1, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_QTTY, CTL_CHKPAN_GRPBOX1, -1, CTL_CHKPAN_PRICE, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_PRICE, CTL_CHKPAN_QTTY, -1, CTL_CHKPAN_SUM, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SUM, CTL_CHKPAN_PRICE, -1, CTL_CHKPAN_GRPBOX1, 0, crfLinkRight | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CAFE_STATUS, CTL_CHKPAN_GRPBOX1, -1, CTL_CHKPAN_GRPBOX1, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX4, 0, -1, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX5, 0, -1, 0, 0, crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_INPUT, CTL_CHKPAN_GRPBOX4, -1, CTL_CHKPAN_INFO, 0, crfLinkLeft | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_INFO, CTL_CHKPAN_INPUT, -1, CTL_CHKPAN_GRPBOX4, 0, crfLinkRight | crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_SELGOODS,    CTL_CHKPAN_INPUT,     CTL_CHKPAN_GRPBOX5, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable); // @anchor
			SetCtrlResizeParam(CTL_CHKPAN_BYPRICE,     CTL_CHKPAN_SELGOODS,  CTL_CHKPAN_GRPBOX5, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SETQTTY,     CTL_CHKPAN_BYPRICE,   CTL_CHKPAN_GRPBOX5, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_ENTER,       CTL_CHKPAN_INPUT,     CTL_CHKPAN_BYPRICE, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(STDCTL_OKBUTTON,        CTL_CHKPAN_ENTER,     CTL_CHKPAN_ENTER, CTL_CHKPAN_ENTER, CTL_CHKPAN_ENTER, CRF_LINK_ALL | crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CANCEL,      CTL_CHKPAN_ENTER,     CTL_CHKPAN_BYPRICE, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX2,     CTL_CHKPAN_GRPBOX1,   CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_INFO, 0, crfLinkRight|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_CASH,        CTL_CHKPAN_GRPBOX2,   CTL_CHKPAN_GRPBOX2, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_BANKING,     CTL_CHKPAN_CASH,      CTL_CHKPAN_GRPBOX2, -1, 0, crfLinkLeft|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_CASHOPER,    -1, CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_GRPBOX5, 0, crfLinkRight|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SELTABLE,    CTL_CHKPAN_CASH, CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_CASH, 0, crfLinkLeft|crfLinkRight|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_TODEFPRN,    -1, CTL_CHKPAN_GRPBOX2, CTL_CHKPAN_GRPBOX2,  0, crfLinkRight|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_TOLOCPRN,    -1, CTL_CHKPAN_GRPBOX2, CTL_CHKPAN_TODEFPRN, 0, crfLinkRight|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_DIVISION,    -1, CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_SELTABLE, 0, crfLinkRight|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SCARD,       -1, CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_DIVISION, 0, crfLinkRight|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_RETCHECK,    -1, CTL_CHKPAN_GRPBOX2, CTL_CHKPAN_CASH,     0, crfLinkRight|crfLinkTop|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_SUSPCHECK,   -1, CTL_CHKPAN_GRPBOX2, CTL_CHKPAN_RETCHECK, 0, crfLinkRight|crfLinkTop|crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_GRPBOX4,     0,                    -1, 0, 0, crfResizeable);

			SetCtrlResizeParam(CTL_CHKPAN_BIGHINT,    CTL_CHKPAN_SETQTTY, CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_BIGHINT_KB, 0, crfLinkLeft|crfLinkTop|crfLinkRight|crfResizeable);
			SetCtrlResizeParam(CTL_CHKPAN_BIGHINT_KB, CTL_CHKPAN_INFO,    CTL_CHKPAN_GRPBOX5, CTL_CHKPAN_SCARD, 0, crfLinkLeft|crfLinkTop|crfLinkRight|crfResizeable);
			/*
			LinkCtrlsToDlgBorders(CRF_LINK_LEFTBOTTOM, CTL_CHKPAN_HINT1, CTL_CHKPAN_KBHINT1,
				CTL_CHKPAN_HINT2, CTL_CHKPAN_KBHINT2, CTL_CHKPAN_HINT3, CTL_CHKPAN_KBHINT3,
				CTL_CHKPAN_HINT4, CTL_CHKPAN_KBHINT4, CTL_CHKPAN_HINT5, CTL_CHKPAN_KBHINT5,
				CTL_CHKPAN_HINT6, CTL_CHKPAN_KBHINT6, CTL_CHKPAN_HINT7, CTL_CHKPAN_KBHINT7, 0L);
			*/
		}
//#ifdef NDEBUG
		ResizeDlgToFullScreen();
//#endif
		UpdateGList(0, 0);  // Формируем список товарных групп
		ok = 1;
	}
	return ok;
}

int CheckPaneDialog::UpdateGList(int updGoodsList, PPID selGroupID)
{
	int    ok = 1;
	if(Flags & fTouchScreen && !oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
		SString temp_buf;
		SString grp_name;
		if(updGoodsList) {
			selGroupID = NZOR(selGroupID, SelGoodsGrpID);
			ListBoxDef   * p_def = 0;
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GDSLIST));
			StrAssocArray * p_ts_ary = 0;
			if(updGoodsList == -2) {
				double price = 0.0;
				PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELBYPRICE, grp_name);
				bool   is_input = GetInput();
				if(!is_input) {
					showInputLineCalc(this, CTL_CHKPAN_INPUT);
					is_input = GetInput();
				}
				if(is_input) {
					price = Input.ToReal();
					PPWaitStart();
					if(price != 0.0) {
						p_ts_ary = GObj.CreateListByPrice(LConfig.Location, R2(price));
						grp_name.Space().Cat(price, SFMT_MONEY);
					}
					else {
						if(Input.Len() >= INSTVSRCH_THRESHOLD)
							temp_buf.Z().CatChar('!').Cat(Input);
						else
							temp_buf = Input;
						p_ts_ary = new StrAssocArray;
						GObj.P_Tbl->GetListBySubstring(temp_buf, p_ts_ary, -1, true);
						grp_name.Space().CatQStr(temp_buf);
					}
					PPWaitStop();
					ClearInput(0);
				}
				SETIFZ(p_ts_ary, new StrAssocArray); // empty list // @newok
				p_def = new StrAssocListBoxDef(p_ts_ary, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
			}
			else {
				Goods2Tbl::Rec grp_rec;
				PPWaitStart();
				if(GObj.Fetch(selGroupID, &grp_rec) > 0) {
					PPLoadStringS("group", grp_name).CatDiv(':', 2).Cat(grp_rec.Name);
				}
				else
					grp_name.Z();
				p_def = GObj.Selector(0, 0, reinterpret_cast<void *>(selGroupID));
				PPWaitStop();
			}
			if(!(Flags & fNoEdit)) {
				RECT   list_rect;
				GetClientRect(p_list->getHandle(), &list_rect);
				if(p_def) {
					p_def->setViewHight((list_rect.bottom - list_rect.top) / GoodsListFontHeight);
					p_def->SetOption(lbtHSizeAlreadyDef, 1);
				}
			}
			p_list->setDef(p_def);
			CALLPTRMEMB(p_list->P_Def, SetOption(lbtSelNotify, 1));
			ActiveListID = CTL_CHKPAN_GDSLIST;
			p_list->Draw_();
		}
		else {
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELGROUP, grp_name);
			//
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			PPIDArray act_list, obj_list;
			act_list.addzlist(PPACN_OBJADD, PPACN_OBJUPD, PPACN_OBJRMV, PPACN_OBJUNIFY, 0);
			if(!LastGrpListUpdTime || p_sj->GetObjListByEventSince(PPOBJ_GOODSGROUP, &act_list, LastGrpListUpdTime, obj_list, 0) > 0) // @v11.4.5 (!LastGrpListUpdTime ||)
				if(TouchScreenID) {
					PPTouchScreenPacket ts_pack;
					PPObjTouchScreen    ts_obj;
					if(ts_obj.GetPacket(TouchScreenID, &ts_pack) > 0)
						InitGroupList(ts_pack);
				}
			//
			SmartListBox * p_grp_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GRPLIST));
			ListBoxDef * p_def = SmartListBox::IsValidS(p_grp_list) ? static_cast<ListBoxDef *>(p_grp_list->P_Def) : 0;
			if(p_def) {
				const  int sav_pos = p_def->_curItem();
				bool   focus_item_found = false;
				p_grp_list->freeAll();
				GrpListItem * p_item = 0;
				for(uint i = 0; GroupList.enumItems(&i, (void **)&p_item);) {
					uint   p = 0;
					Goods2Tbl::Rec goods_rec;
					int    do_insert = 0;
					if(UiFlags & uifOneGroupLevel) {
						if(p_item->ParentID == GroupList.TopID)
							do_insert = 1;
					}
					else {
						if(!p_item->ParentID || (p_def->search(&p_item->ParentID, 0, lbSrchByID) &&
							GroupList.Get(p_item->ParentID, &p) && GroupList.at(p).Flags & GrpListItem::fOpened))
							do_insert = 1;
					}
					if(do_insert && GObj.Fetch(p_item->ID, &goods_rec) > 0) {
						p_grp_list->addItem(goods_rec.ID, goods_rec.Name);
						if(goods_rec.ID == selGroupID)
							focus_item_found = true;
					}
				}
				RECT   list_rect;
				GetClientRect(p_grp_list->getHandle(), &list_rect);
				p_def->setViewHight((list_rect.bottom - list_rect.top) / GoodsListFontHeight);
				p_def->SetOption(lbtHSizeAlreadyDef, 1);
				p_def->SetOption(lbtSelNotify, 1);
				p_def->SetOption(lbtFocNotify, 1);
				ActiveListID = CTL_CHKPAN_GRPLIST;
				if(selGroupID) {
					if(focus_item_found)
						p_grp_list->TransmitData(+1, &selGroupID);
					else
						p_grp_list->focusItem(0);
				}
				else
					p_grp_list->focusItem(sav_pos);
				p_grp_list->Draw_();
				p_def->getCurID(&SelGoodsGrpID);
			}
			// @v11.4.5 (moved to InitGroupList) LastGrpListUpdTime = getcurdatetime_();
		}
		showCtrl(CTL_CHKPAN_GRPLIST,    !updGoodsList);
		disableCtrl(CTL_CHKPAN_GRPLIST,  LOGIC(updGoodsList));
		ShowWindow(GetDlgItem(H(), MAKE_BUTTON_ID(CTL_CHKPAN_GRPLIST, 1)), updGoodsList ? SW_HIDE : SW_SHOW);
		showCtrl(CTL_CHKPAN_GDSLIST,     LOGIC(updGoodsList));
		disableCtrl(CTL_CHKPAN_GDSLIST, !updGoodsList);
		ShowWindow(GetDlgItem(H(), MAKE_BUTTON_ID(CTL_CHKPAN_GDSLIST, 1)), updGoodsList ? SW_SHOW : SW_HIDE);
		enableCommand(cmaSelect, LOGIC(updGoodsList));
		LastCtrlID = ActiveListID;
		setStaticText(CTL_CHKPAN_GRPNAME, grp_name);
	}
	return ok;
}

void CheckPaneDialog::setupHint()
{
	const  int _state = GetState();
	uint   hint_count = 0, hint_list[32];
	switch(_state) {
		case sEMPTYLIST_EMPTYBUF:
			hint_list[hint_count++] =  1;
			hint_list[hint_count++] =  2;
			if(PNP.AbstractGoodsID)
				hint_list[hint_count++] =  14;
			else
				hint_list[hint_count++] =  3;
			hint_list[hint_count++] =  4;
			hint_list[hint_count++] = 11;
			hint_list[hint_count++] =  5;
			hint_list[hint_count++] = 13;
			break;
		case sEMPTYLIST_BUF:
			hint_list[hint_count++] =  1;
			hint_list[hint_count++] =  2;
			if(PNP.AbstractGoodsID)
				hint_list[hint_count++] =  14;
			else
				hint_list[hint_count++] =  3;
			hint_list[hint_count++] =  6;
			hint_list[hint_count++] = 11;
			hint_list[hint_count++] =  7;
			hint_list[hint_count++] =  8;
			break;
		case sLIST_EMPTYBUF:
			hint_list[hint_count++] =  1;
			hint_list[hint_count++] =  2;
			if(PNP.AbstractGoodsID)
				hint_list[hint_count++] =  14;
			else
				hint_list[hint_count++] =  3;
			hint_list[hint_count++] = 11;
			hint_list[hint_count++] =  9;
			hint_list[hint_count++] = 10;
			hint_list[hint_count++] = 12;
			break;
		case sLIST_BUF:
			hint_list[hint_count++] =  1;
			hint_list[hint_count++] =  2;
			if(PNP.AbstractGoodsID)
				hint_list[hint_count++] =  14;
			else
				hint_list[hint_count++] =  3;
			hint_list[hint_count++] =  6;
			hint_list[hint_count++] = 11;
			hint_list[hint_count++] =  7;
			hint_list[hint_count++] =  8;
			break;
		case sLISTSEL_EMPTYBUF:
			hint_list[hint_count++] =  2;
			hint_list[hint_count++] =  9;
			hint_list[hint_count++] = 10;
			break;
		case sLISTSEL_BUF:
			hint_list[hint_count++] = 2;
			hint_list[hint_count++] = 6;
			hint_list[hint_count++] = 7;
			hint_list[hint_count++] = 8;
			break;
		default:
			memzero(hint_list, sizeof(hint_list));
			break;
	}
	{
		const bool is_locked = false;
		const bool paym_allowed_cash = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmCash)); // @v12.3.8
		const bool paym_allowed_bank = LOGIC(PNP.AllowedPaymentTypes & (1 << cpmBank)); // @v12.3.8
		enableCommand(cmRetCheck, oneof3(_state, sEMPTYLIST_EMPTYBUF, sLISTSEL_EMPTYBUF, sLISTSEL_BUF) && (OperRightsFlags & orfReturns));
		enableCommand(cmQuantity, oneof3(_state, sEMPTYLIST_BUF, sLIST_BUF, sLISTSEL_BUF));
		enableCommand(cmChkPanSuspend, oneof2(_state, sEMPTYLIST_EMPTYBUF, sLIST_EMPTYBUF));
		enableCommand(cmCash,    oneof2(_state, sLIST_EMPTYBUF, sLISTSEL_EMPTYBUF) && !is_locked && (OperRightsFlags & orfPrintCheck) && paym_allowed_cash);
		enableCommand(cmBanking, oneof2(_state, sLIST_EMPTYBUF, sLISTSEL_EMPTYBUF) && !is_locked && TESTMULTFLAG(OperRightsFlags, (orfBanking|orfPrintCheck)) && paym_allowed_bank);
		enableCommand(cmChkPanPrint,   (oneof2(_state, sEMPTYLIST_EMPTYBUF, sLIST_EMPTYBUF) || (Flags & fNoEdit)) && (OperRightsFlags & orfPreCheck));
		enableCommand(cmToLocPrinters, oneof2(_state, sEMPTYLIST_EMPTYBUF, sLIST_EMPTYBUF) && (Flags & fLocPrinters));
	}
	static const uint hint_text_list[] = {
		PPTXT_CHKPAN_HINT01,
		PPTXT_CHKPAN_HINT02,
		PPTXT_CHKPAN_HINT03,
		PPTXT_CHKPAN_HINT04,
		PPTXT_CHKPAN_HINT05,
		PPTXT_CHKPAN_HINT06,
		PPTXT_CHKPAN_HINT07,
		PPTXT_CHKPAN_HINT08,
		PPTXT_CHKPAN_HINT09,
		PPTXT_CHKPAN_HINT10,
		PPTXT_CHKPAN_HINT11,
		PPTXT_CHKPAN_HINT12,
		PPTXT_CHKPAN_HINT13,
		PPTXT_CHKPAN_HINT14
	};
	if(getCtrlView(CTL_CHKPAN_BIGHINT)) {
		SString temp_buf, hint, keyb, hint_buf, hint_kb_buf;
		for(uint i = 0; i < CTL_CHKPAN_NUMHINTS; i++) {
			const uint idx = hint_list[i];
			if(i < hint_count && idx > 0 && idx <= SIZEOFARRAY(hint_text_list) && PPLoadText(hint_text_list[idx-1], temp_buf) > 0) {
				if(temp_buf.Strip().Divide('=', hint, keyb) > 0) {
					hint_buf.Space().Cat(hint.Strip()).CR();
					hint_kb_buf.Space().Cat(keyb.Strip());
				}
				else {
					hint_buf.Space().Cat(hint.Strip()).CR();
				}
				hint_kb_buf.CR();
			}
		}
		setStaticText(CTL_CHKPAN_BIGHINT, hint_buf);
		setStaticText(CTL_CHKPAN_BIGHINT_KB, hint_kb_buf);
	}
	if(!(Flags & fTouchScreen)) {
		SString temp_buf, hint, keyb;
		for(uint i = 0; i < CTL_CHKPAN_NUMHINTS; i++) {
			const uint idx = hint_list[i];
			if(i < hint_count && idx > 0 && idx <= SIZEOFARRAY(hint_text_list) && PPLoadText(hint_text_list[idx-1], temp_buf) > 0) {
				if(temp_buf.Divide('=', hint, keyb) > 0)
					setStaticText(CTL_CHKPAN_HINT1 + i + CTL_CHKPAN_KBHINTBIAS, keyb);
				setStaticText(CTL_CHKPAN_HINT1 + i, hint);
			}
			else {
				temp_buf.Z();
				setStaticText(CTL_CHKPAN_HINT1 + i, temp_buf);
				setStaticText(CTL_CHKPAN_HINT1 + i + CTL_CHKPAN_KBHINTBIAS, temp_buf);
			}
		}
	}
}
//
//
//
int CheckPaneDialog::SelectBill(PPID * pBillID, const char * pTitle) // @v11.8.7
{
	int    ok = -1;
	SelCheckListDialog * dlg = 0;
	const uint dlg_id = (DlgFlags & fLarge) ? DLG_SELCHECK_L : DLG_SELCHECK;
	const PPEquipConfig & r_cfg = CsObj.GetEqCfg();
	if(r_cfg.ChkPanImpOpID && r_cfg.ChkPanImpBillTagID) {
		PPObjBill * p_bobj(BillObj);
		Reference * p_ref(PPRef);
		const LDATE now_dt = getcurdate_();
		PPViewBill v_bill;
		BillFilt f_bill;
		f_bill.OpID = r_cfg.ChkPanImpOpID;
		f_bill.LocList.Add(PNP.CnLocID); // @v11.8.10
		f_bill.P_TagF = new TagFilt;
		f_bill.P_TagF->TagsRestrict.Add(r_cfg.ChkPanImpBillTagID, PPConst::P_TagValRestrict_Exist, 0);
		{
			// @v11.8.9 Настраиваемый параметр количества дней для обзора документов
			int lbbp = r_cfg.LookBackBillPeriod;
			if(lbbp <= 0)
				lbbp = 30;
			f_bill.Period.low = plusdate(now_dt, -lbbp);
		}
		f_bill.SortOrder = BillFilt::ordByDate;
		f_bill.Flags |= (BillFilt::fDescOrder|BillFilt::fNoTempTable); // @v11.8.10 BillFilt::fNoTempTable
		if(v_bill.Init_(&f_bill)) {
			PPIDArray bill_id_list;
			v_bill.GetBillIDList(&bill_id_list);
			if(bill_id_list.getCount()) {
				CCheckCore & r_cc = GetCc();
				PPIDArray temp_cc_list;
				uint blidx = bill_id_list.getCount();
				do {
					const PPID bill_id = bill_id_list.get(--blidx);
					S_GUID cc_uuid;
					// Если документ уже ссылается на чек и нам удастся найти этот чек, то документ убираем из списка выбора.
					if(p_ref->Ot.GetTagGuid(PPOBJ_BILL, bill_id, PPTAG_BILL_LINKCCHECKUUID, cc_uuid) > 0 && !!cc_uuid) {
						// (проверка сильно замедляет работу) if(CsObj.GetListByUuid(cc_uuid, 30, temp_cc_list) > 0) {
							//assert(temp_cc_list.getCount());
							bill_id_list.atFree(blidx); 
						//}
					}
				} while(blidx);
				SelCheckListDialog::AddedParam param(PNP.NodeID, P.TableCode, P.GetAgentID(), OperRightsFlags);
				SelCheckListDialog * dlg = new SelCheckListDialog(dlg_id, bill_id_list, this, &param);
				if(CheckDialogPtrErr(&dlg)) {
					if(!isempty(pTitle))
						dlg->setTitle(pTitle);
					dlg->showCtrl(CTL_SELCHECK_CODE, false);
					dlg->showCtrl(CTL_SELCHECK_DATE, false);
					dlg->showCtrl(CTLCAL_SELCHECK_DATE, false);
					while(ok < 0 && ExecView(dlg) == cmOK) {
						_SelCheck sc;
						if(dlg->getDTS(&sc)) {
							ASSIGN_PTR(pBillID, sc.BillID);
							ok = 1;
						}
					}
				}
			}
		}
	}
	delete dlg;
	return ok;
}

//int CheckPaneDialog::SelectCheck(PPID * pChkID, const char * pTitle, long flags, SString * pSelFormat, int * pSelectedOp)
int CheckPaneDialog::SelectCheck(const char * pTitle, long flags, SelectCheckResult & rResult)
{
	// Если flags == 0, то полагаем, что выбираем чек для возврата или (начиная с релиза @v12.2.8) чек, который необходимо скорректировать (отпечатать чек коррекции)
	int    ok = -1;
	_SelCheck  sch;
	Flags |= fSuspSleepTimeout;
	SelCheckListDialog::AddedParam param(((flags & scfThisNodeOnly) ? PNP.NodeID : 0), P.TableCode, P.GetAgentID(), OperRightsFlags);
	if(flags == 0) {
		param.Flags |= param.fRetOrCorrSelection;
	}
	else if(flags & scfAllowReturns)
		param.Flags |= param.fAllowReturns;
	param.FormatName = rResult.Format;
	InitCashMachine();
	const uint dlg_id = (DlgFlags & fLarge) ? DLG_SELCHECK_L : DLG_SELCHECK;
	const int  select_format_arg = (flags & scfSelSlipDocFormat) ? 1 : ((flags & scfSelFormat) ? -1 : 0);
	SelCheckListDialog * dlg = new SelCheckListDialog(dlg_id, select_format_arg, P_CM, this, &param);
	if(CheckDialogPtrErr(&dlg)) {
		if(!isempty(pTitle))
			dlg->setTitle(pTitle);
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&sch))
				ok = 1;
	}
	else
		ok = 0;
	rResult.CcID = sch.CheckID;
	rResult.Format = sch.SelFormat;
	if(param.Flags & param.fRetOrCorrSelection) {
		rResult.Op = sch.CcOp; // (sch.Flags & _SelCheck::fCorrection) ? 2 : 1;
	}
	delete dlg;
	Flags &= ~fSuspSleepTimeout;
	return ok;
}

int CPosProcessor::RestoreSuspendedCheck(PPID ccID, CCheckPacket * pPack, int unfinishedForReprinting)
{
	int    ok = 1;
	CCheckCore & r_cc = GetCc();
	SString temp_buf;
	SString msg_buf;
	CCheckPacket cc_pack;
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(r_cc.LoadPacket(ccID, 0, &cc_pack) > 0);
		CCheckCore::MakeCodeString(&cc_pack.Rec, 0, msg_buf);
		const long _ccf = cc_pack.Rec.Flags;
		long  _ccf_to_update = _ccf;
		if(unfinishedForReprinting) {
			THROW_PP_S(!(_ccf & (CCHKF_SUSPENDED|CCHKF_PRINTED|CCHKF_JUNK)), PPERR_CCHKPRINTEDORSUSPENDED, msg_buf);
			_ccf_to_update |= (CCHKF_TOREPRINT|CCHKF_JUNK);
			SelPack = cc_pack;
		}
		else {
			THROW_PP_S(_ccf & CCHKF_SUSPENDED && (!(_ccf & CCHKF_JUNK) || r_cc.IsLostJunkCheck(ccID, &SessUUID, 0)), PPERR_CCHKNOMORESUSPENDED, msg_buf);
			_ccf_to_update |= (CCHKF_JUNK);
		}
		THROW(r_cc.UpdateFlags(ccID, _ccf_to_update, 0));
		Helper_SetupSessUuidForCheck(ccID);
		THROW(tra.Commit());
	}
	P.Z(); // @v11.9.11
	// @v11.8.11 P.freeAll();
	// @v11.8.11 P.OrgUserID = 0;
	{
		Goods2Tbl::Rec goods_rec;
		CCheckItem chk_item;
		for(uint i = 0; cc_pack.EnumLines(&i, &chk_item);) {
			if(GObj.Fetch(chk_item.GoodsID, &goods_rec) > 0) {
				STRNSCPY(chk_item.GoodsName, goods_rec.Name);
				GObj.GetSingleBarcode(chk_item.GoodsID, 0, temp_buf);
				STRNSCPY(chk_item.BarCode, temp_buf);
			}
			P.insert(&chk_item);
		}
	}
	CCheckPacket::CopyExtStrContainer(P, cc_pack, 0); // @v11.8.11
	SetupExt(&cc_pack);
	if(P.getCount()) {
		if(PNP.CnExtFlags & CASHFX_KEEPORGCCUSER)
			P.OrgUserID = cc_pack.Rec.UserID;
		SetupState(sLIST_EMPTYBUF);
	}
	// @v12.2.9 SETFLAG(Flags, fRetCheck, cc_pack.Rec.Flags & CCHKF_RETURN);
	// @v12.2.9 {
	if(cc_pack.Rec.Flags & CCHKF_CORRECTION)
		SetCurrentOp(CCOP_CORRECTION_SELL);
	else if(cc_pack.Rec.Flags & CCHKF_RETURN)
		SetCurrentOp(CCOP_RETURN);
	// } @v12.2.9 
	SetPrintedFlag(cc_pack.Rec.Flags & CCHKF_PREPRINT);
	if(!unfinishedForReprinting) {
		Flags  |= fWaitOnSCard;
		SetupSCard(cc_pack.Rec.SCardID, 0);
		Flags &= ~fWaitOnSCard;
	}
	SetupInfo(0);
	SuspCheckID = ccID;
	if(!unfinishedForReprinting) {
		ProcessGift(); // Добавляем в чек подарочные позиции (если не репринт)
	}
	GetCc().WriteCCheckLogFile(&cc_pack, 0, CCheckCore::logRestored, 1);
	ASSIGN_PTR(pPack, cc_pack); // @v11.5.8
	CATCHZOK
	return ok;
}

bool CPosProcessor::CheckRights(long rights) const { return ((OperRightsFlags & rights) ==  rights) ? true : PPSetError(PPERR_NORIGHTS); }

int CheckPaneDialog::SelectSuspendedCheck()
{
	int    ok = -1;
	PPID   chk_id = 0;
	SString msg_buf;
	SelCheckListDialog * dlg = 0;
	const  PPID single_agent_id = P.GetAgentID();
	THROW_PP(OperRightsFlags & orfRestoreSuspWithoutAgent || single_agent_id, PPERR_NORIGHTSELSUSPCHECK);
	if(IsState(sEMPTYLIST_EMPTYBUF)) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		TSVector <CCheckViewItem> list;
		SelCheckListDialog::AddedParam param(0, P.TableCode, single_agent_id, OperRightsFlags);
		THROW(InitCcView());
		const uint dlg_id = (DlgFlags & fLarge) ? DLG_SELSUSCHECK_L : DLG_SELSUSCHECK;
		dlg = new SelCheckListDialog(dlg_id, &list, 0, this, &param);
		Flags |= fSuspSleepTimeout;
		THROW(CheckDialogPtr(&dlg));
		do {
			const int do_filter_by_node = 0;
			list.clear();
			{
				CCheckFilt cc_filt;
				cc_filt.Period.low = plusdate(getcurdate_(), (PNP.Scf.DaysPeriod > 0) ? -PNP.Scf.DaysPeriod : -7);
				cc_filt.Flags |= (CCheckFilt::fSuspendedOnly | CCheckFilt::fShowSuspended | CCheckFilt::fLostJunkAsSusp);
				if(do_filter_by_node)
					cc_filt.NodeList.Add(PNP.NodeID);
				cc_filt.TableCode = P.TableCode;
				cc_filt.AgentID = single_agent_id;
				if(P_CcView->Init_(&cc_filt)) {
					CCheckViewItem item;
					for(P_CcView->InitIteration(0); P_CcView->NextIteration(&item) > 0;) {
						if(PNP.Scf.DlvrItemsShowTag < 0) {
							if(item.Flags & CCHKF_DELIVERY)
								continue;
						}
						else if(PNP.Scf.DlvrItemsShowTag > 0) {
							if(!(item.Flags & CCHKF_DELIVERY))
								continue;
						}
						if(!single_agent_id && item.AgentID && ar_obj.Fetch(item.AgentID, &ar_rec) > 0 && (ar_rec.Flags & ARTRF_STOPBILL))
							continue;
						list.insert(&item);
					}
				}
			}
			if(PNP.Scf.Flags & PPSyncCashNode::SuspCheckFilt::fNotSpFinished) {
				CCheckFilt cc_filt;
				cc_filt.Period.low = plusdate(getcurdate_(), (PNP.Scf.DaysPeriod > 0) ? -PNP.Scf.DaysPeriod : -7);
				cc_filt.Flags |= (CCheckFilt::fNotSpFinished|CCheckFilt::fLostJunkAsSusp);
				if(do_filter_by_node)
					cc_filt.NodeList.Add(PNP.NodeID);
				cc_filt.TableCode = P.TableCode;
				cc_filt.AgentID = single_agent_id;
				if(P_CcView->Init_(&cc_filt)) {
					CCheckViewItem item;
					for(P_CcView->InitIteration(0); P_CcView->NextIteration(&item) > 0;) {
						if(!(item.Flags & CCHKF_SKIP)) {
							if(PNP.Scf.DlvrItemsShowTag < 0) {
								if(item.Flags & CCHKF_DELIVERY)
									continue;
							}
							else if(PNP.Scf.DlvrItemsShowTag > 0) {
								if(!(item.Flags & CCHKF_DELIVERY))
									continue;
							}
							if(!single_agent_id && item.AgentID && ar_obj.Fetch(item.AgentID, &ar_rec) > 0 && (ar_rec.Flags & ARTRF_STOPBILL))
								continue;
							list.insert(&item);
						}
					}
				}
			}
			dlg->SetList(list);
			if(ExecView(dlg) == cmOK) {
				_SelCheck  sel_chk;
				selectCtrl(CTL_CHKPAN_INPUT);
				if(dlg->getDTS(&sel_chk) && sel_chk.CheckID) {
					CCheckTbl::Rec cc_rec;
					if(GetCc().Search(sel_chk.CheckID, &cc_rec) > 0) {
						if(sel_chk.Flags & _SelCheck::fUnfinished) {
							// ReprintCheck
							PPID   cc_id = 0;
							Flags |= fReprinting;
							int r1 = RestoreSuspendedCheck(sel_chk.CheckID, 0/*pPack*/, 1/*unfinishedForReprinting*/);
							if(r1)
								r1 = AcceptCheck(&cc_id, &SelPack.AL_Const(), 0, 0.0, CPosProcessor::accmAveragePrinting);
							Flags &= ~fReprinting;
							THROW(r1);
						}
						else if(cc_rec.Flags & CCHKF_SUSPENDED && (!(cc_rec.Flags & CCHKF_JUNK) || GetCc().IsLostJunkCheck(sel_chk.CheckID, &SessUUID, 0))) {
							chk_id = sel_chk.CheckID;
							ok = 1;
						}
						else if(!(cc_rec.Flags & CCHKF_SPFINISHED)) {
							CCheckCore::MakeCodeString(&cc_rec, 0, msg_buf);
							if(ConfirmMessage(PPCFM_SETCCASSPFINISHED, msg_buf, 1)) {
								CCheckTbl::Rec cc_rec_to_upd;
								if(GetCc().Search(cc_rec.ID, &cc_rec_to_upd) > 0) {
									if(!(cc_rec_to_upd.Flags & CCHKF_SPFINISHED)) {
										cc_rec_to_upd.Flags |= CCHKF_SPFINISHED;
										if(!GetCc().UpdateFlags(cc_rec_to_upd.ID, cc_rec_to_upd.Flags, 1)) {
											ok = (MessageError(-1, 0, eomMsgWindow), 2);
										}
									}
								}
							}
							else
								ok = 2;
						}
						else {
							CCheckCore::MakeCodeString(&cc_rec, 0, msg_buf);
							ok = (MessageError(PPERR_CCHKNOMORESUSPENDED, msg_buf, eomMsgWindow), 2);
						}
					}
					else
						ok = (MessageError(-1, 0, eomMsgWindow), 2);
				}
			}
			else
				ok = -1;
		} while(ok == 2);
		Flags &= ~fSuspSleepTimeout;
		if(ok == 1) {
			THROW(RestoreSuspendedCheck(chk_id, 0/*pPack*/, 0/*unfinishedForReprinting*/));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int CheckPaneDialog::SelectTable()
{
	int    ok = -1;
	if(!SuspCheckID || !P.TableCode || ConfirmMessage(PPCFM_CHKPAN_CHNGTBL, 0, 1)) {
		if(TableSelWhatman.NotEmpty() && fileExists(TableSelWhatman)) {
			TWhatmanObject::SelectObjRetBlock sel_blk;
			if(PPWhatmanWindow::Launch(TableSelWhatman, 0, &sel_blk) > 0) {
				if(sel_blk.Val1 == PPOBJ_CAFETABLE && sel_blk.Val2 > 0) {
					long   guest_count = 0;
					if(PNP.CnExtFlags & CASHFX_INPGUESTCFTBL)
						SelectGuestCount(sel_blk.Val2, &guest_count);
					SetupCTable(sel_blk.Val2, guest_count);
					ok = 1;
				}
			}
		}
		else {
			bool   is_input = GetInput();
			if(!is_input) {
				showInputLineCalc(this, CTL_CHKPAN_INPUT);
				is_input = GetInput();
			}
			if(is_input) {
				const  int table_no = Input.ToLong();
				long   guest_count = 0;
				if(PNP.CnExtFlags & CASHFX_INPGUESTCFTBL)
					SelectGuestCount(table_no, &guest_count);
				SetupCTable(table_no, guest_count);
				ClearInput(0);
				ok = 1;
			}
		}
	}
	return ok;
}
//
// Descr: Устанвливает либо сбрасывает состояние ввода возвратного или корректирующего чека
//
void CheckPaneDialog::SetupRetCheck(bool ret)
{
	SString temp_buf;
	Flags |= fSuspSleepTimeout;
	{
		const bool is_pane_empty = (!P.HasCur() && !P.getCount());
		Flags &= ~fRetByCredit;
		if(!ret) {
			if(is_pane_empty) {
				P.Lb_.Z();
				SetCurrentOp(CCOP_GENERAL);
				//
				SelPack.ClearLines();
				SelLines.clear();
				SetupState(sEMPTYLIST_EMPTYBUF);
				CSt.Z();
				SetupInfo(0);
			}
		}
		else {
			if(is_pane_empty || oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
				if(!(OperRightsFlags & orfReturns)) {
					PPError(PPERR_NORIGHTS);
				}
				else {
					int    selected_op = CCOP_GENERAL;
					PPID   selected_cc_id = 0;
					/*if(is_pane_empty) {
						// @v12.2.9 SETFLAG(Flags, fRetCheck, ret);
						// @v12.2.9 {
						if(!ret) {
							SetCurrentOp(CCOP_GENERAL);
						}
						// } @v12.2.9 
					}
					else if(IsCurrentOp(CCOP_RETURN) || IsCurrentOp(opCorrection)) {
						ret = true;
					}
					if(ret)*/
					{
						selected_op = CCOP_RETURN;
						CCheckPacket chk_pack;
						CCheckLineTbl::Rec line_rec;
						if(!oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
							SelectCheckResult sccr;
							if(SelectCheck(PPLoadStringS("selectccheck_forrefundorcorr", temp_buf), 0, sccr) > 0) {
								selected_op = sccr.Op;
								selected_cc_id = sccr.CcID;
								GetCc().LoadPacket(sccr.CcID, 0, &SelPack);
								chk_pack.CopyLines(SelPack);
								if(SelPack.Rec.SCardID)
									AcceptSCard(SelPack.Rec.SCardID, 0, ascfIgnoreRights);
								else
									CSt.SetID(0, 0);
								Flags &= ~fWaitOnSCard;
							}
						}
						else {
							for(uint i = 0; SelPack.EnumLines(&i, &line_rec);) {
								double sel_qtty = 0.0;
								const double rest_qtty_ = SelLines.Search(i, &sel_qtty, 0) ? (line_rec.Quantity-sel_qtty) : line_rec.Quantity;
								if(rest_qtty_ != 0.0)
									chk_pack.InsertCclSimple(line_rec.GoodsID, rest_qtty_, intmnytodbl(line_rec.Price), line_rec.Dscnt);
							}
						}
						// @v12.2.12 {
						if(oneof2(selected_op, CCOP_CORRECTION_SELLSTORNO, CCOP_CORRECTION_RETSTORNO)) {
							// Сторно-коррекцию проводим немедленно	
							//
							CCheckPacket cc_to_storno;
							if(selected_cc_id && GetCc().LoadPacket(selected_cc_id, 0, &cc_to_storno) > 0) {
								CCheckCore::MakeCodeString(&cc_to_storno.Rec, CCheckCore::mcsID, temp_buf);
								if(ConfirmMessage(PPCFG_CCCORRECTIONSTORNO, temp_buf, 1)) {
									// Здесь - процедура проводки сторно-коррекции
									PPID   result_cc_id = 0;
									if(!TurnCorrectionStorno(&result_cc_id, selected_op, selected_cc_id)) {
										
									}
									
								}
							}
							ClearCheck();
						}
						else /*} @v12.2.12*/ {
							SetCurrentOp(selected_op); // @v12.2.9
							if(chk_pack.GetCount()) {
								const long lc_flags = DS.GetTLA().Lc.Flags;
								if(selected_op == CCOP_RETURN) {
									chk_pack.Rec = SelPack.Rec;
									ushort r = CheckExecAndDestroyDialog(new CheckPaneDialog(0, 0, &chk_pack, (Flags & fTouchScreen)), 1, 0);
									if(r) {
										int    crcc_arg = -1; // Аргумент последующего вызова функции CalcRestByCrdCard_ (-1 - не вызывать)
										if(r == cmaAll) {
											LoadCheck(&chk_pack, true/*makeRetCheck*/, false);
											SelLines.freeAll();
											for(uint i = 0; SelPack.EnumLines(&i, &line_rec);)
												SelLines.Add(i, line_rec.Quantity);
											SetupState(sLISTSEL_EMPTYBUF);
											crcc_arg = 0;
										}
										else if(r == cmOK) {
											Goods2Tbl::Rec goods_rec;
											const CCheckLineTbl::Rec & cclr = chk_pack.GetLineC(0);
											CCheckItem & r_cur_item = P.GetCur();
											r_cur_item.GoodsID = cclr.GoodsID;
											if(GObj.Fetch(r_cur_item.GoodsID, &goods_rec) > 0) {
												STRNSCPY(r_cur_item.GoodsName, goods_rec.Name);
												GObj.FetchSingleBarcode(r_cur_item.GoodsID, temp_buf);
												STRNSCPY(r_cur_item.BarCode, temp_buf);
											}
											else
												r_cur_item.GoodsName[0] = r_cur_item.BarCode[0] = 0;
											r_cur_item.Quantity = -fabs(cclr.Quantity);
											r_cur_item.Price    = intmnytodbl(cclr.Price);
											r_cur_item.Discount = cclr.Dscnt;
											chk_pack.GetLineTextExt(1, CCheckPacket::lnextEgaisMark, temp_buf);
											STRNSCPY(r_cur_item.EgaisMark, temp_buf);
											chk_pack.GetLineTextExt(1, CCheckPacket::lnextSerial, temp_buf);
											STRNSCPY(r_cur_item.Serial, temp_buf);
											// @v11.3.6 {
											chk_pack.GetLineTextExt(1, CCheckPacket::lnextChZnSerial, temp_buf);
											STRNSCPY(r_cur_item.ChZnSerial, temp_buf);
											chk_pack.GetLineTextExt(1, CCheckPacket::lnextChZnGtin, temp_buf);
											STRNSCPY(r_cur_item.ChZnGtin, temp_buf);
											chk_pack.GetLineTextExt(1, CCheckPacket::lnextChZnMark, temp_buf);
											STRNSCPY(r_cur_item.ChZnMark, temp_buf);
											// } @v11.3.6 
											P.CurCcItemPos = P.getCount();
											SetupRowData(true/*doCalcRest*/);
											SetupState(sLISTSEL_BUF);
											crcc_arg = 1;
										}
										{
											P.Lb_._Op = selected_op;
											P.Lb_.AmL = SelPack.AL();
											P.Lb_._CcID = chk_pack.Rec.ID;
											if(P.Lb_.AmL.getCount()) {
												P.Lb_._CcAmount = P.Lb_.AmL.GetTotal();
												P.Lb_._CcCredit = P.Lb_.AmL.Get(CCAMTTYP_CRDCARD);
												assert(MONEYTOLDBL(SelPack.Rec.Amount) == P.Lb_._CcAmount); // @paranoic
											}
											else {
												P.Lb_._CcAmount = MONEYTOLDBL(chk_pack.Rec.Amount);
												P.Lb_._CcCredit = 0.0;
											}
											if(P.Lb_._CcCredit > 0.0 && P.Lb_._CcAmount > 0.0)
												Flags |= fRetByCredit;
										}
										if(crcc_arg >= 0)
											CalcRestByCrdCard_(crcc_arg);
									}
								}
								// @v12.2.9 {
								else if(CPosProcessor::IsCorrectionOp(selected_op)) {
									LoadCheck(&chk_pack, false/*makeRetCheck*/, false);
									SelLines.freeAll();
									for(uint i = 0; SelPack.EnumLines(&i, &line_rec);)
										SelLines.Add(i, line_rec.Quantity);
									SetupState(sLISTSEL_EMPTYBUF);
									{
										P.Lb_._Op = selected_op;
										P.Lb_.AmL = SelPack.AL();
										P.Lb_._CcID = selected_cc_id;
										if(P.Lb_.AmL.getCount()) {
											P.Lb_._CcAmount = P.Lb_.AmL.GetTotal();
											P.Lb_._CcCredit = P.Lb_.AmL.Get(CCAMTTYP_CRDCARD);
											assert(MONEYTOLDBL(SelPack.Rec.Amount) == P.Lb_._CcAmount); // @paranoic
										}
										else {
											P.Lb_._CcAmount = MONEYTOLDBL(chk_pack.Rec.Amount);
											P.Lb_._CcCredit = 0.0;
										}
										/*
										if(P.Lb_._CcCredit > 0.0 && P.Lb_._CcAmount > 0.0)
											Flags |= fRetByCredit;
										*/
									}
								}
								// } @v12.2.9 
								DS.SetLCfgFlags(lc_flags);
							}
						}
					}
					/*else if(!F(fRetCheck)) {
						SelPack.ClearLines();
						SelLines.clear();
						SetupState(sEMPTYLIST_EMPTYBUF);
						CSt.Z();
					}*/
					SetupInfo(0);
					ClearInput(0);
				}
			}
		}
	}
	Flags &= ~fSuspSleepTimeout;
}

void CheckPaneDialog::SetupInfo(const char * pErrMsg)
{
	double rest = 0.0;
	SString buf;
	SString temp_buf;
	if(pErrMsg) {
		Flags |= fError;
		buf = pErrMsg;
	}
	else {
		Flags &= ~fError;
		if(F(fOnlyReports))
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SESSISCLOSED, buf);
		// @v12.2.9 {
		else if(IsCurrentOpCorrection()) {
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_CORRECTION, buf);
			if(P.Lb_._CcID) {
				CCheckCore & r_cc = GetCc();
				CCheckTbl::Rec cc_rec;
				if(r_cc.Search(P.Lb_._CcID, &cc_rec) > 0) {
					CCheckCore::MakeCodeString(&cc_rec, 0/*options*/, temp_buf);
					buf.Space().Cat("->").Space().Cat(temp_buf);
				}
			}
		}
		// } @v12.2.9 
		else if(IsCurrentOp(CCOP_RETURN))
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_RETCHECK, buf);
		else if(F(fSelByPrice))
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELBYPRICE, buf);
		else if(F(fWaitOnSCard))
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_WAITINGONSCARD, buf);
		else if(P_ChkPack && P_ChkPack->Rec.Flags & CCHKF_ORDER) {
			if(P_ChkPack->Rec.Flags & CCHKF_SKIP)
				PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_CTBLORDERCANCELED, buf);
			else
				PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_CTBLORDER, buf);
		}
		else
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELLING, buf);
		if(CSt.GetID()) {
			SCardTbl::Rec sc_rec;
			if(ScObj.Fetch(CSt.GetID(), &sc_rec) > 0) {
				if(buf.NotEmpty())
					buf.Space();
				buf.Cat(PPLoadStringS("card", temp_buf)).Space().Cat(sc_rec.Code).Space();
				if(CSt.Flags & CSt.fUhtt)
					buf.CatParStr("UHTT").Space();
				if(IsCurrentOp(CCOP_RETURN)) {
					if(F(fRetByCredit) && CSt.AdditionalPayment) {
						PPLoadString("addpayment", temp_buf);
						buf.Cat(temp_buf).Space().Cat(CSt.AdditionalPayment, SFMT_MONEY);
					}
				}
				else {
					buf.Cat(PPLoadStringS("discount", temp_buf)).Space();
					buf.Cat(CSt.SettledDiscount, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
					if(Flags & (fBankingPayment|fSCardCredit|fSCardBonus)) { // @bank_or_scard
						if(CSt.AdditionalPayment > 0.0 && !(Flags & fSCardBonus)) {
							PPLoadString("addpayment", temp_buf);
							buf.Space().Cat(temp_buf).Space().Cat(CSt.AdditionalPayment, SFMT_MONEY);
						}
						else {
							PPLoadString((Flags & fSCardBonus) ? "bonus" : "rest", temp_buf);
							buf.Space().Cat(temp_buf).Space().Cat(CSt.RestByCrdCard, SFMT_MONEY);
						}
					}
				}
				if(CSt.P_Eqb && CSt.P_Eqb->QkList.getCount()) {
					const  PPID single_qk_id = CSt.P_Eqb->QkList.getSingle();
					if(single_qk_id) {
						PPObjQuotKind qk_obj;
						PPQuotKindPacket qk_pack;
						buf.Space().Cat(PPLoadStringS("quote", temp_buf)).CatDiv(':', 2);
						if(qk_obj.Fetch(single_qk_id, &qk_pack) > 0)
							buf.Cat(qk_pack.Rec.Name);
						else
							ideqvalstr(single_qk_id, buf);
					}
					else
						buf.Cat("QL");
				}
			}
		}
		// @v11.0.9 {
		if(ManDis.Discount > 0.0) {
			buf.Space().CatChar('[').CatChar('-');
			if(ManDis.Flags & ManDis.fPct)
				buf.Cat(ManDis.Discount, MKSFMTD(0, 1, NMBF_NOTRAILZ)).CatChar('%');
			else
				buf.Cat(ManDis.Discount, MKSFMTD_020);
			buf.CatChar(']');
		}
		// } @v11.0.9 
	}
	setCtrlString(CTL_CHKPAN_INFO, buf);
	//
	P.SetupInfo(buf);
	if(PNP.CnFlags & CASHF_SHOWREST && P.GetCur().GoodsID) {
		if(buf.NotEmpty())
			buf.CatCharN(' ', 4);
		buf.Cat(PPLoadStringS("rest", temp_buf)).CatDiv(':', 2).Cat(P.GetRest(), MKSFMTD(0, 3, NMBF_NOTRAILZ));
	}
	if(Flags & fNoEdit) {
		if(P_ChkPack) {
			P_ChkPack->GetExtStrData(CCheckPacket::extssSign, temp_buf);
			if(temp_buf.NotEmptyS())
				buf.CatDivIfNotEmpty(' ', 0).Cat("UTM");
			// @v11.0.1 {
			P_ChkPack->GetExtStrData(CCheckPacket::extssChZnProcessingTag, temp_buf);
			if(temp_buf.NotEmptyS())
				buf.CatDivIfNotEmpty(' ', 0).Cat("CHZN");
			P_ChkPack->GetExtStrData(CCheckPacket::extssRemoteProcessingTa, temp_buf);
			if(temp_buf.NotEmptyS())
				buf.CatDivIfNotEmpty(' ', 0).Cat("RPTA");
			// } @v11.0.1 
		}
	}
	setStaticText(CTL_CHKPAN_CAFE_STATUS, buf);
	if(Flags & fNoEdit) {
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
		const long cur = SmartListBox::IsValidS(p_list) ? p_list->P_Def->_curItem() : -1;
		if(cur >= 0 && cur < P.getCountI()) {
			const CCheckItem & r_item = P.at(cur);
			if(r_item.EgaisMark[0])
				setCtrlString(CTL_CHKPAN_INFO, buf = r_item.EgaisMark);
			else if(r_item.ChZnMark[0])
				setCtrlString(CTL_CHKPAN_INFO, buf = r_item.ChZnMark);
		}
	}
}

int CPosProcessor::PreprocessRowBeforeRemoving(/*IN*/long rowNo, /*OUT*/double * pResultQtty)
{
	int    ok = -1;
	double qtty = 0.0;
	if(rowNo >= 0 && rowNo < static_cast<long>(P.getCount())) {
		const  CCheckItem item = P.at(static_cast<uint>(rowNo)); // @note Переменная не должна быть ссылкой (оригинал может измениться)
		qtty = fabs(item.Quantity);
		int    is_rights = 1;
		if((Flags & fPrinted) && !(OperRightsFlags & orfChgPrintedCheck))
			is_rights = 0;
		else if(!(OperRightsFlags & orfEscChkLine) && (!(OperRightsFlags & orfEscChkLineBeforeOrder) || (item.Flags & cifIsPrinted)))
			is_rights = 0;
		if(is_rights) {
			CCheckLineTbl::Rec line;
			for(uint i = 0; qtty > 0.0 && SelPack.EnumLines(&i, &line);) {
				if(line.GoodsID == item.GoodsID) {
					double  sel_qtty = 0.0;
					if(SelLines.Search(i, &sel_qtty, 0)) {
						if(qtty < sel_qtty)
							SelLines.Add(i, -qtty);
						else
							SelLines.Remove(i);
						qtty -= sel_qtty;
					}
				}
			}
			ok = 1;
		}
		else {
			Flags |= fSuspSleepTimeout;
			ok = (PPError(PPERR_NORIGHTS), 0);
			Flags &= ~fSuspSleepTimeout;
		}
	}
	ASSIGN_PTR(pResultQtty, qtty);
	return ok;
}

int CPosProcessor::Helper_PrintRemovedRow(const CCheckItem & rItem)
{
	int   ok = -1;
	if(rItem.Flags & cifIsPrinted) {
		const bool do_debug_log = LOGIC(CConfig.Flags & CCFLG_DEBUG);
		SStrCollection debug_rep_list;
		SString msg_buf;
		SString chk_code;
		SString prn_name;
		SString buf_prn;
		SString buf_ulp;
		SString buf_errprn;
		if(do_debug_log) {
			PPLoadText(PPTXT_LOG_CHKPAN_PRINTING, buf_prn);
			PPLoadText(PPTXT_LOG_CHKPAN_UNDEFPRN, buf_ulp);
			PPLoadText(PPTXT_LOG_CHKPAN_ERRPRINTING, buf_errprn);
			CCheckPacket cc_pack;
			GetCheckInfo(&cc_pack);
			CCheckCore::MakeCodeString(&cc_pack.Rec, 0, chk_code);
		}
		PPObjLocPrinter lp_obj;
		PPLocPrinter loc_prn_rec;
		DS.GetTLA().PrintDevice.Z();
		//
		// Инициализация gtoa по ассоциации, установленной в кассовом узле
		//
		InitCashMachine();
		GoodsToObjAssoc gtoa(NZOR(P_CM->GetNodeData().GoodsLocAssocID, PPASS_GOODS2LOC), PPOBJ_LOCATION);
		if(gtoa.IsValid() && gtoa.Load()) {
			PPID   loc_id = 0;
			gtoa.Get(rItem.GoodsID, &loc_id);
			{
				//
				// В этой области содержимое пакета P полностью очищается, а затем восстанавливается (P = saved_check).
				//
				CCheckItemArray saved_check(P);
				P.freeAll();
				if(lp_obj.GetPrinterByLocation(loc_id, prn_name, &loc_prn_rec) > 0) {
					P.insert(&rItem);
					if(Print(1, &loc_prn_rec, REPORT_CCHECKDETAILLOCROWCANCEL) > 0)
						MakeDbgPrintLogList(0, buf_prn, chk_code, prn_name, debug_rep_list);
					else
						MakeDbgPrintLogList(2, buf_errprn, chk_code, prn_name, debug_rep_list);
				}
				else
					MakeDbgPrintLogList(1, buf_ulp, chk_code, prn_name, debug_rep_list);
				P = saved_check;
			}
			if(do_debug_log)
				PPLogMessageList(PPFILNAM_DEBUG_LOG, debug_rep_list, LOGMSGF_TIME|LOGMSGF_USER);
			ok = 1;
		}
		else
			ok = MessageError(PPErrCode, 0, eomBeep|eomStatusLine);
	}
	return ok;
}

int CPosProcessor::Helper_RemoveRow(long rowNo, const CCheckItem & rItem)
{
	int    ok = 1;
	P.atFree(static_cast<uint>(rowNo));
	Helper_PrintRemovedRow(rItem);
	ProcessGift();
	//ReplyOnRemoveItem();
	AutosaveCheck();
	DS.LogAction(PPACN_RMVCHKLINE, PPOBJ_CCHECK, 0, rItem.GoodsID, 1);
	if(!oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF))
		SetupDiscount(0);
	CalcRestByCrdCard_(0);
	if(CConfig.Flags & CCFLG_DEBUG) {
		CCheckPacket pack;
		CCheckLineTbl::Rec line_rec;
		if(SuspCheckID) {
			GetCc().Search(SuspCheckID, &pack.Rec);
		}
		else {
			InitCashMachine();
			pack.Rec.SessID = P_CM ? P_CM->GetCurSessID() : 0;
			pack.Rec.PosNodeID = PNP.NodeID;
			pack.Rec.Flags |= (CCHKF_SYNC | CCHKF_NOTUSED);
		}
		Helper_InitCcPacket(&pack, 0, 0, 0);
		rItem.GetRec(line_rec, LOGIC(pack.Rec.Flags & CCHKF_RETURN));
		line_rec.CheckID = pack.Rec.ID;
		GetCc().WriteCCheckLogFile(&pack, &line_rec, CCheckCore::logRowCleared, 1);
	}
	OnUpdateList(0);
	return ok;
}

int CPosProcessor::ResetCurrentLine()
{
	int    ok = -1;
	if(P.HasCur()) {
		ClearRow();
		ok = 1;
	}
	else
		ok = -1;
	return ok;
}

const CCheckItem * CPosProcessor::FindLine(int rbycheck) const
{
	const CCheckItem * p_result = 0;
	if(rbycheck > 0) {
		for(uint i = 0; !p_result && i < P.getCount(); i++) {
			if(P.at(i).RByCheck == rbycheck)
				p_result = &P.at(i);
		}
	}
	return p_result;
}

int CPosProcessor::UpdateLine(const CCheckItem * pItem)
{
	int    ok = 0;
	if(pItem && pItem->RByCheck > 0) {
		for(uint i = 0; i < P.getCount(); i++) {
			CCheckItem & r_item = P.at(i);
			if(r_item.RByCheck == pItem->RByCheck) {
				r_item.Quantity = pItem->Quantity;
				ok = 1;
			}
		}		
	}
	return ok;
}

int CPosProcessor::Backend_Release()
{
	int    ok = 1;
	ResetCurrentLine();
	if(P.getCount()) {
		PPID   cc_id = 0;
		THROW(AcceptCheck(&cc_id, 0, 0, 0, accmSuspended) > 0);
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::Backend_SetModifList(const SaModif & rList)
{
    int    ok = -1;
    THROW_PP(P.HasCur() && !(P.GetCur().Flags & cifModifier), PPERR_CPOS_UNABLESEMODIFLIST);
    if(rList.getCount()) {
		for(uint i = 0; i < rList.getCount(); i++) {
			const SaModifEntry & r_entry = rList.at(i);
			THROW_SL(P.CurModifList.insert(&r_entry));
		}
		ok = 1;
    }
    CATCHZOK
    return ok;
}

int CPosProcessor::Backend_SetRowQueue(int rowNo, int queue)
{
	int    ok = -1;
	if(!(Flags & fNoEdit)) {
		const long cur = rowNo;
		THROW_PP_S(cur >= 0 && cur < static_cast<long>(P.getCount()), PPERR_CPOS_INVCCROWINDEX, rowNo);
		THROW_PP_S(queue >= 0 && queue <= 100, PPERR_CPOS_INVCCROWQUEUE, queue);
		{
			CCheckItem & r_item = P.at((uint)cur);
			r_item.Queue = queue;
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::Backend_RemoveRow(int rowNo)
{
	int    ok = -1;
	if(!(Flags & fNoEdit)) {
		const long cur = rowNo;
		THROW_PP_S(cur >= 0 && cur < static_cast<long>(P.getCount()), PPERR_CPOS_INVCCROWINDEX, rowNo);
		if(PreprocessRowBeforeRemoving(cur, 0) > 0) {
			const  CCheckItem item = P.at(static_cast<uint>(cur));
			if(!(item.Flags & cifIsPrinted) || ConfirmMessage(PPCFM_PRINTCANCELEDCCROW, 0, 0)) {
				Helper_RemoveRow(cur, item);
				// @interactive selectCtrl(CTL_CHKPAN_INPUT);
				ok = 1;
			}
			if(!P.getCount()) {
				const int  prev_state = GetState();
				if(IsState(sLIST_EMPTYBUF)) {
					SetupState(sEMPTYLIST_EMPTYBUF);
					P.CurCcItemPos = -1;
				}
				else if(IsState(sLIST_BUF)) {
					SetupState(sEMPTYLIST_BUF);
					P.CurCcItemPos = 0;
				}
				/* @interactive
				if(GetState() != prev_state)
					setupHint();
				LastCtrlID = 0;
				*/
			}
		}
	}
	CATCHZOK
	return ok;
}

int CheckPaneDialog::RemoveRow()
{
	int    ok = -1;
	if(!(Flags & fNoEdit)) {
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
		if(SmartListBox::IsValidS(p_list)) {
			const long cur = p_list->P_Def->_curItem();
			if(PreprocessRowBeforeRemoving(cur, 0) > 0) {
				const  CCheckItem item = P.at((uint)cur);
				//
				// @v8.1.11 Структура следующего ниже блока изменена таким образом, что если
				// строка была ранее отпечатана и пользователь отклонил запрос на печать отмены,
				// то удаляться строка не будет.
				//
				if(!(item.Flags & cifIsPrinted) || ConfirmMessage(PPCFM_PRINTCANCELEDCCROW, 0, 1)) {
					Helper_RemoveRow(cur, item);
					selectCtrl(CTL_CHKPAN_INPUT);
					ok = 1;
				}
				if(!P.getCount()) {
					const int  prev_state = GetState();
					if(IsState(sLIST_EMPTYBUF)) {
						SetupState(sEMPTYLIST_EMPTYBUF);
						P.CurCcItemPos = -1;
					}
					else if(IsState(sLIST_BUF)) {
						SetupState(sEMPTYLIST_BUF);
						P.CurCcItemPos = 0;
					}
					if(GetState() != prev_state)
						setupHint();
					LastCtrlID = 0;
				}
			}
		}
	}
	return ok;
}

/*virtual*/void CheckPaneDialog::OnUpdateList(int goBottom)
{
//@lbt_chkpan    "3,R,#;3,C,;70,L,Товар;16,L,Штрихкод;11,R,Цена;10,R,Кол-во;11,R,Сумма;12,L,Серия;9,R,Отдел;4,C,Q" // DLG_CHKPAN
//@lbt_chkpan_ts "4,R,#;3,C,;60,L,Товар;12,R,Цена;12,R,Кол-во;12,R,Сумма;10,R,Отдел;4,C,Q"                         // DLG_CHKPAN_TS
//@lbt_chkpanv   "3,R,#;3,C,;40,L,Товар;16,L,Штрихкод;9,R,Цена;8,R,Скидка;8,R,Кол-во;9,R,Сумма;12,L,Серия"         // DLG_CHKPANV, DLG_CHKPANV_L
// since @v12.2.4 @lbt_chkpanv "3,R,#;3,C,;44,L,@ware;16,L,@barcode;9,R,@price;8,R,@discount;8,R,@qtty;9,R,@amount;6,R,@vatrate;15,L,@series" // DLG_CHKPANV, DLG_CHKPANV_L // @v12.2.4 vatrate
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_LIST));
	if(SmartListBox::IsValidS(p_list)) {
		const long column_egais_ident = 100;
		CCheckItem * p_item;
		ListBoxDef * p_def_ = p_list->P_Def;
		long   cur = p_def_->_curItem();
		uint   i;
		bool   do_show_egaismark = false;
		p_list->freeAll();
		StringSet ss(SLBColumnDelim);
		/*
		for(i = 0; !do_show_egaismark && P.enumItems(&i, (void **)&p_item);) {
			if(p_item->EgaisMark[0] != 0)
				do_show_egaismark = true;
		}
		if(do_show_egaismark && !p_list->SearchColumnByIdent(column_egais_ident, 0)) {
			p_list->AddColumn(-1, "@egaismark", 20, 0, column_egais_ident);
		}
		*/
		SString temp_buf;
		for(i = 0; P.enumItems(&i, (void **)&p_item);) {
			ss.Z();
			//char   sub[256];
			ss.add(temp_buf.Z().Cat(i));
			ss.add(temp_buf.Z().CatChar((p_item->Flags & cifIsPrinted) ? 'v' : ' '));
			{
				temp_buf.Z();
				if(p_item->Flags & cifModifier) {
					temp_buf.CatChar('>').CatCharN(' ', 7);
				}
				temp_buf.Cat(p_item->GoodsName);
				ss.add(temp_buf);
			}
			if(!(Flags & fTouchScreen) || (Flags & fNoEdit))
				ss.add(p_item->BarCode);
			ss.add(temp_buf.Z().Cat(p_item->Price, SFMT_MONEY));
			if(Flags & fNoEdit) {
				ss.add(temp_buf.Z().Cat(p_item->Discount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
			}
			ss.add(temp_buf.Z().Cat(p_item->Quantity, SFMT_QTTY));
			ss.add(temp_buf.Z().Cat(p_item->GetAmount(), SFMT_MONEY));
			// @v12.2.4 {
			if(Flags & fNoEdit) {
				CCheckLineTbl::Rec ccl_rec;
				p_item->GetRec(ccl_rec, LOGIC(P_ChkPack->Rec.Flags & CCHKF_RETURN));
				GTaxVect::EvalBlock eb(*P_ChkPack, ccl_rec, 0/*exclFlags*/);
				GTaxVect gtv;
				gtv.EvaluateTaxes(eb);
				double vat_rate = gtv.GetTaxRate(GTAX_VAT, 0);				
				ss.add(temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 1, NMBF_NOTRAILZ)).CatChar('%'));
			}
			// } @v12.2.4 
			if(!(Flags & fTouchScreen) || (Flags & fNoEdit))
				ss.add(p_item->Serial);
			temp_buf.Z();
			if(p_item->Division)
				temp_buf.Cat(p_item->Division);
			ss.add(temp_buf);
			temp_buf.Z();
			if(p_item->Queue)
				temp_buf.Cat(p_item->Queue);
			ss.add(temp_buf);
			if(do_show_egaismark)
				ss.add(p_item->EgaisMark);
			if(!p_list->addItem(i, ss.getBuf())) {
				Flags |= fSuspSleepTimeout;
				PPError(PPERR_SLIB, 0);
				Flags &= ~fSuspSleepTimeout;
				break;
			}
			else {
				if(p_item->Flags & cifGift)
					p_def_->SetItemColor(i, SClrBlack, SClrGreen);
				else if(p_item->Flags & cifGrouped || (i < P.getCount() && P.at(i).Flags & cifGrouped))
					p_def_->SetItemColor(i, SClrBlack, SClrLightgrey);
				else if(p_item->Flags & cifIsPrinted)
					p_def_->SetItemColor(i, SClrBlack, SColor(0xFC, 0xD5, 0xB4));
				else if(p_item->Flags & cifPartOfComplex)
					p_def_->SetItemColor(i, SClrBlack, SColor(0xD7, 0xE4, 0xBC));
				else if(p_item->RemoteProcessingTa[0] && p_item->Flags & cifFixedPrice)
					p_def_->SetItemColor(i, SClrBlack, SClrCadetblue);
				else
					p_def_->ResetItemColor(i);
			}
		}
		if(goBottom)
			cur = P.getCount()-1;
		p_list->focusItem(cur);
		p_list->Draw_();
		{
			const CcTotal cct = CalcTotal();
			temp_buf.Z();
			setStaticText(CTL_CHKPAN_TOTAL, temp_buf.Cat(cct.Amount, MKSFMTD(0, 2, NMBF_NOZERO)));
			if(cct.Discount != 0.0) {
				PPLoadStringS("discount", temp_buf).Colon().Cat(cct.Discount, SFMT_MONEY);
			}
			else
				temp_buf.Z();
			setStaticText(CTL_CHKPAN_DISCOUNT, temp_buf);
		}
	}
	SetupInfo(0);
}

/*virtual*/int CheckPaneDialog::MessageError(int errCode, const char * pAddedMsg, long outputMode)
{
	const int dest = (outputMode & 0xff);
	SString err_msg;
	if(outputMode & eomBeep && !(Flags & fDisableBeep)) {
		for(int i = 0; i < 2; i++) {
			Beep(500,  200);
			Beep(1500, 200);
		}
	}
	if(dest == eomMsgWindow) {
		Flags |= fSuspSleepTimeout;
		PPError(errCode, pAddedMsg, mfNoFocus);
		Flags &= ~fSuspSleepTimeout;
	}
	else if(dest == eomStatusLine) {
		PPGetMessage(mfError, (errCode < 0) ? PPErrCode : errCode, pAddedMsg, 1, err_msg);
		SetupInfo(err_msg);
	}
	else if(dest == eomPopup) {
		PPGetMessage(mfError, (errCode < 0) ? PPErrCode : errCode, pAddedMsg, 1, err_msg);
		SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления //
		PPTooltipMessage(err_msg, 0, H(), 20000, GetColorRef(SClrRed),
			SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
	}
	return 0;
}

/*virtual*/int CheckPaneDialog::MsgToDisp_Show()
{
	if(MsgToDisp.getCount()) {
		SString temp_buf;
		SString msg_buf;
		for(uint sp = 0; MsgToDisp.get(&sp, temp_buf);) {
			msg_buf.Cat(temp_buf.Chomp().Strip());
		}
		if(msg_buf.NotEmpty()) {
			SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления //
			PPTooltipMessage(msg_buf, 0, H(), 20000, GetColorRef(SClrOrange),
				SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fOpaque);
		}
	}
	return 1;
}

/*virtual*/int CheckPaneDialog::ConfirmMessage(int msgId, const char * pAddedMsg, int defaultResponse)
{
	return CONFIRM_S(msgId, pAddedMsg);
}

/*virtual*/int CheckPaneDialog::CDispCommand(int cmd, int iVal, double rv1, double rv2)
{
	int    ok = 1;
	if(P_CDY) {
		switch(cmd) {
			case cdispcmdClear: ok = P_CDY->ClearDisplay(); break;
			case cdispcmdText:
				if(iVal == cdisptxtOpened)
					ok = P_CDY->OpenedCash();
				else if(iVal == cdisptxtClosed)
					ok = P_CDY->ClosedCash();
				else
					ok = -1;
				break;
			case cdispcmdTotal: ok = P_CDY->SetTotal(rv1); break;
			case cdispcmdTotalDiscount: ok = P_CDY->SetDiscount(rv1, rv2); break;
			case cdispcmdChange: ok = (P_CDY->ClearDisplay() && P_CDY->SetChange(rv1, rv2)); break;
			case cdispcmdCurrentItem:
				if(P.IsCurValid()) {
					P_CDY->ClearDisplay();
					if(P_CDY->SetGoodsName(P.GetCur().GoodsName)) {
						SDelay(50);
						ok = P_CDY->SetAmt(P.GetCur().NetPrice(), P.GetCur().Quantity);
					}
					else
						ok = 0;
				}
				else
					ok = -1;
				break;
			case cdispcmdCurrentGiftItem:
				if(P.IsCurValid()) {
					P_CDY->ClearDisplay();
					if(P_CDY->SetGoodsName(P.GetCur().GoodsName)) {
						SDelay(50);
						ok = P_CDY->SetPresent();
					}
					else
						ok = 0;
				}
				else
					ok = -1;
				break;
		}
	}
	else
		ok = -1;
	return ok;
}
//
//
//
int CheckPaneDialog::SelectSerial(PPID goodsID, SString & rSerial, double * pPrice)
{
	rSerial.Z();
	int    ok = -1;
	PPObjBill * p_bobj(BillObj);
	const  SelLotBrowser::Entry * p_sel = 0;
	int    r;
	uint   s = 0;
	const  LDATE curdt = getcurdate_();
	double total_exp = 0.0; // Общий расход товара goodsID активными сессиями
	SString serial;
	DateIter diter;
	SArray * p_ary = 0;
	SelLotBrowser * p_brw = 0;
	ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
	ReceiptTbl::Rec lot_rec;
	StringSet seek_serial_list;
	const  PPID loc_id = GetCnLocID(goodsID);
	THROW(p_ary = SelLotBrowser::CreateArray());
	diter.Init(0, curdt);
	THROW(GetCc().CalcActiveExpendByGoods(goodsID, loc_id, 0, &total_exp));
	while((r = r_rcpt.EnumLots(goodsID, loc_id, &diter, &lot_rec)) > 0) {
		double exp = 0.0;
		double rest = lot_rec.Rest;
		p_bobj->GetSerialNumberByLot(lot_rec.ID, serial, 1);
		if(serial.NotEmpty()) {
			//
			// Защита от повторного учета текущих продаж на разных лотах, имеющих одинаковые серии
			//
			if(!seek_serial_list.search(serial, 0, 0)) {
				THROW(GetCc().CalcActiveExpendByGoods(goodsID, loc_id, serial, &exp));
				{
					CCheckItem * p_item;
					for(uint i = 0; P.enumItems(&i, (void **)&p_item);)
						if(p_item->GoodsID == goodsID && serial.CmpNC(p_item->Serial) == 0) {
							exp += p_item->Quantity;
							total_exp += p_item->Quantity;
						}
				}
				seek_serial_list.add(serial);
			}
			rest -= exp;
			total_exp -= exp;
			if(rest > 0.0 || !(PNP.CnFlags & CASHF_ABOVEZEROSALE))
				THROW(SelLotBrowser::AddItemToArray(p_ary, &lot_rec, curdt, rest, 1));
		}
	}
	THROW(r);
	if(p_ary->getCount()) {
		long   slbf = 0;
		// @v11.1.8 {
		if(CsObj.GetEqCfg().Flags & PPEquipConfig::fDisableSellSpoiledSeries)
			slbf |= SelLotBrowser::fDisableSelectionSpoiledSeries;
		// } @v11.1.8 
		THROW_MEM(p_brw = new SelLotBrowser(p_bobj, p_ary, s, slbf)); // @newok
		if(ExecView(p_brw) == cmOK && (p_sel = static_cast<const SelLotBrowser::Entry *>(p_brw->getCurItem())) != 0) {
			if((serial = p_sel->Serial).NotEmptyS()) {
				ASSIGN_PTR(pPrice, p_sel->Price);
				rSerial = serial;
				ok = 1;
			}
		}
	}
	else
		ok = -2;
	CATCH
		if(p_brw == 0)
			delete p_ary;
		PPError();
	ENDCATCH
	delete p_brw;
	return ok;
}
//
//
//
int CheckPaneDialog::ChZnMarkAutoSelect(PPID goodsID, double qtty, SString & rChZnBuf)
{
	rChZnBuf.Z();
	int    ok = -1;
	SString temp_buf;
	Goods2Tbl::Rec goods_rec; // запись основного товара (который непосредственно продается)
	Goods2Tbl::Rec org_goods_rec; // запись оригинального товара (из которого был произведен основной товар)
	if(qtty != 0.0 && GObj.Fetch(goodsID, &goods_rec) > 0 && goods_rec.GoodsTypeID)	{
		PPGoodsType gt_rec;
		if(GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.ChZnProdType == GTCHZNPT_DRAFTBEER_AWR) {
			PPID   org_goods_id = 0;
			PPObjUnit unit_obj;
			double main_goods_liter_ratio = 0.0;
			double org_goods_liter_ratio = 0.0;
			double main_goods_kg_ratio = 0.0;
			double org_goods_kg_ratio = 0.0;
			if(goods_rec.PhUnitID && goods_rec.PhUPerU > 0.0) {
				unit_obj.TranslateToBase(goods_rec.PhUnitID, SUOM_LITER, &main_goods_liter_ratio);
				unit_obj.TranslateToBase(goods_rec.PhUnitID, SUOM_KILOGRAM, &main_goods_kg_ratio);
				assert(!((main_goods_liter_ratio != 0.0) && (main_goods_kg_ratio != 0.0))); // не может единица измерения одновременно соотносится с килограммами и литрами
				main_goods_liter_ratio *= goods_rec.PhUPerU;
				main_goods_kg_ratio *= goods_rec.PhUPerU;
			}
			else {
				unit_obj.TranslateToBase(goods_rec.UnitID, SUOM_LITER, &main_goods_liter_ratio);
				unit_obj.TranslateToBase(goods_rec.UnitID, SUOM_KILOGRAM, &main_goods_kg_ratio);
				assert(!((main_goods_liter_ratio != 0.0) && (main_goods_kg_ratio != 0.0))); // не может единица измерения одновременно соотносится с килограммами и литрами
			}
			if(main_goods_liter_ratio <= 0.0 && main_goods_kg_ratio <= 0.0) {
				// @todo Тут надо что-то в лог вывести
			}
			else if(!GObj.GetOriginalRawGoodsByStruc(goodsID, &org_goods_id) || GObj.Fetch(org_goods_id, &org_goods_rec) <= 0) {
				// @todo Тут надо что-то в лог вывести
			}
			else {
				if(org_goods_rec.PhUnitID && org_goods_rec.PhUPerU > 0.0) {
					unit_obj.TranslateToBase(org_goods_rec.PhUnitID, SUOM_LITER, &org_goods_liter_ratio);
					unit_obj.TranslateToBase(org_goods_rec.PhUnitID, SUOM_KILOGRAM, &org_goods_kg_ratio);
					assert(!((org_goods_liter_ratio != 0.0) && (org_goods_kg_ratio != 0.0))); // не может единица измерения одновременно соотносится с килограммами и литрами
					org_goods_liter_ratio *= org_goods_rec.PhUPerU;
					org_goods_kg_ratio *= org_goods_rec.PhUPerU;
				}
				else {
					unit_obj.TranslateToBase(org_goods_rec.UnitID, SUOM_LITER, &org_goods_liter_ratio);
					unit_obj.TranslateToBase(org_goods_rec.UnitID, SUOM_KILOGRAM, &org_goods_kg_ratio);
				}
				PPID   base_unit_id = 0;
				if(main_goods_liter_ratio > 0.0 && org_goods_liter_ratio > 0.0) {
					base_unit_id = SUOM_LITER;
				}
				else if(main_goods_kg_ratio > 0.0 && org_goods_kg_ratio > 0.0) {
					base_unit_id = SUOM_KILOGRAM;
				}
				if(!base_unit_id) {
					// @todo Тут надо что-то в лог вывести
				}
				else {
					PPObjBill * p_bobj(BillObj);
					LotExtCodeCore * p_lotxct = p_bobj->P_LotXcT;
					if(!p_lotxct) {
						// @todo Тут надо что-то в лог вывести
					}
					else {
						Transfer * p_trfr = p_bobj->trfr;
						LotArray lot_list;
						StringSet ss_ext_codes;
						TSCollection <CCheckCore::ListByMarkEntry> lbm;
						p_trfr->Rcpt.GetList(org_goods_id, 0, 0, ZERODATE, ReceiptCore::glfWithExtCodeOnly, &lot_list);
						if(!lot_list.getCount()) {
							// @todo Тут надо что-то в лог вывести
						}
						else {
							for(uint i = 0; i < lot_list.getCount(); i++) {
								const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
								if(p_bobj->GetMarkListByLot(r_lot_rec.ID, ss_ext_codes) > 0) {
									for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);) {
										//int    Helper_GetListByMark2(TSCollection <ListByMarkEntry> & rList, int markLnextTextId, const LAssocArray * pCcDate2MaxIdIndex, uint backDays, int sentLnextTextId); // @v12.0.5
										//Cc.Helper_GetListByMark2()
										CCheckCore::ListByMarkEntry * p_lbm_entry = lbm.CreateNewItem();
										p_lbm_entry->OrgLotID = r_lot_rec.ID;
										p_lbm_entry->OrgLotDate = r_lot_rec.Dt;
										p_lbm_entry->OrgLotQtty = r_lot_rec.Quantity;
										STRNSCPY(p_lbm_entry->Mark, temp_buf);
									}
								}
							}
							const uint back_days = PPObjCSession::GetCcListByMarkBackDays(lbm);
							LAssocArray index;
							LAssocArray * p_index = CsObj.FetchCcDate2MaxIdIndex(index) ? &index : 0;
							GetCc().Helper_GetListByMark2(lbm, CCheckPacket::lnextChZnMark, p_index, back_days, 0);
							{
								double _local_qtty = 0.0;
								if(base_unit_id == SUOM_LITER) {
									_local_qtty = qtty * main_goods_liter_ratio;
								}
								else if(base_unit_id == SUOM_KILOGRAM) {
									_local_qtty = qtty * main_goods_kg_ratio;
								}
								assert(_local_qtty != 0.0); // Выше мы проверили что qtty != 0.0 && (main_goods_liter_ratio > 0.0 || main_goods_kg_ratio > 0)
								for(uint i = 0; ok < 0 && i < lbm.getCount(); i++) {
									const CCheckCore::ListByMarkEntry * p_lbm_entry = lbm.at(i);
									if(p_lbm_entry) {
										double rest = 1.0; // Независимо от того сколько пришло единиц по лоту, одна марка представляет одну торговую единицу!
										if(base_unit_id == SUOM_LITER) {
											rest = (rest * org_goods_liter_ratio) - (p_lbm_entry->TotalOpQtty * main_goods_liter_ratio);
										}
										else if(base_unit_id = SUOM_KILOGRAM) {
											rest = (rest * org_goods_kg_ratio) - (p_lbm_entry->TotalOpQtty * main_goods_kg_ratio);
										}
										assert(ok < 0); // мы не должны сюда попасть если ok > 0 (это - на случай, если ok инициализирована не верно либо еще где-то мы что-то не так сделали)
										if(rest >= _local_qtty) {
											// SUCCESS! Мы нашли марку, остаток по которой в физических единицах достаточен для продажи. Из цикла мы выскочим по условию (ok < 0)
											rChZnBuf = p_lbm_entry->Mark; 
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
	return ok;
}

int CheckPaneDialog::PreprocessGoodsSelection(const PPID goodsID, PPID locID, PgsBlock & rBlk)
{
	int    ok = -1;
	const  LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(goodsID, &goods_rec) > 0) { // @v11.2.8 CheckMatrix
		const  PPID sc_id = CSt.GetID();
		if(PNP.CnExtFlags & CASHFX_USEGOODSMATRIX && !GObj.CheckMatrix(goodsID, GetCnLocID(goodsID), 0, 0)) { // @v11.2.8
			ok = MessageError(-1, 0, eomBeep | eomPopup/*eomStatusLine*/);
		}
		else if(goodsID == GetChargeGoodsID(sc_id)) {
			// @todo Здесь надо проверить что бы товар не был равен ChargeGoodsID из любой кредитной серии карт
			ok = (sc_id && ScObj.IsCreditCard(sc_id)) ? 1 : MessageError(PPERR_INVUSAGECHARGEGOODS, 0, eomStatusLine);
		}
		else if(sc_id && IsOnlyChargeGoodsInPacket(sc_id, 0)) {
			SCardTbl::Rec sc_rec;
			if(ScObj.Search(sc_id, &sc_rec) > 0 && !ScObj.CheckRestrictions(&sc_rec, 0, now_dtm))
				ok = MessageError(PPERR_CHKPAN_SCINVONGOODS, sc_rec.Code, eomBeep | eomPopup/*eomStatusLine*/);
		}
		if(ok) {
			SaComplex complex;
			PPObjGoodsType gt_obj;
			PPGoodsType gt_rec;
			if(goods_rec.GoodsTypeID)
				gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec);
			//PPGoodsStruc partial_struc;
			/*if(LoadPartialStruc(goodsID, partial_struc) > 0) {
				AcceptRow(0);
				PPGoodsStrucItem gs_item;
				for(uint i = 0; i < partial_struc.EnumItemsExt(&i, &gs_item, PPID parentGoodsID, double srcQtty, double * pQtty) const;
			}
			else*/if(LoadComplex(goodsID, complex) > 0 && InputComplexDinner(complex) > 0 && complex.IsComplete() && complex.getCount()) {
				//
				// @todo В этом блоке осуществляется агрегированная автоматическая вставка нескольких позиций из комплекса.
				// Необходимо для каждой из этих позиций проверить валидность параметров (что-то вроде рекурсивного вызова PreprocessGoodsSelection()
				//
				AcceptRow(0);
				for(uint i = 0; i < complex.getCount(); i++) {
					const SaComplexEntry & r_entry = complex.at(i);
					CCheckItem chk_item;
					chk_item.GoodsID = NZOR(r_entry.FinalGoodsID, r_entry.GoodsID);
					GetGoodsName(chk_item.GoodsID, temp_buf);
					temp_buf.CopyTo(chk_item.GoodsName, sizeof(chk_item.GoodsName));
					GObj.FetchSingleBarcode(chk_item.GoodsID, temp_buf);
					temp_buf.CopyTo(chk_item.BarCode, sizeof(chk_item.BarCode));
					chk_item.Quantity = r_entry.Qtty;
					chk_item.Price = r_entry.FinalPrice;
					chk_item.Flags |= cifPartOfComplex;
					P.insert(&chk_item);
					SetupState(sLIST_EMPTYBUF);
				}
				OnUpdateList(1);
				ClearInput(0);
				ok = -1;
			}
			else if(GObj.CheckFlag(goodsID, GF_GENERIC)) {
				PPObject::SetLastErrObj(PPOBJ_GOODS, labs(goodsID));
				ok = MessageError(PPERR_INVGENGOODSCCOP, 0, eomBeep | eomStatusLine);
			}
			else {
				ok = CheckPaneDialog::VerifyQuantity(goodsID, rBlk.Qtty, 0, true/*adjustQtty*/, true/*checkInputBuffer*/);
				if(ok > 0) {
					// @v12.0.12 {
					/*if(rBlk.ChZnMark.NotEmpty()) {
						PPObjBill * p_bobj(BillObj);
						LotExtCodeCore * p_lotxct = p_bobj->P_LotXcT;
						if(p_lotxct) {
							TSVector <LotExtCodeTbl::Rec> rec_list;
							int   parity = 0;
							PPID  first_bill_id = 0;
							int   first_rbb = 0;
							long  box_no = 0;
							if(p_lotxct->GetRecListByMark(rBlk.ChZnMark, rec_list) > 0) {
								const  PPID loc_id = GetCnLocID(goodsID);
								ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
								ReceiptTbl::Rec lot_rec;
								for(uint lotxcidx = 0; lotxcidx < rec_list.getCount(); lotxcidx++) {
									const PPID lot_id = rec_list.at(lotxcidx).LotID;
									if(r_rcpt.Search(lot_id, &lot_rec) > 0 && lot_rec.LocID == loc_id) {
										double exp = 0.0;
										double rest = lot_rec.Rest;
										p_bobj->GetSerialNumberByLot(lot_rec.ID, temp_buf, 1);
										if(temp_buf.NotEmpty()) {
											
										}
									}
								}
							}
						}

					}*/
					// } @v12.0.12 
					const SString preserve_chzn_mark(rBlk.ChZnMark);
					const SString preserve_egais_mark(rBlk.EgaisMark);
					if(Flags & fSelSerial && rBlk.Serial.IsEmpty() /*&& rBlk.ChZnMark.IsEmpty()*/) { // @v12.0.12 Если указана марка то смысла выбирать серию нет // @v12.1.1 это была плохая идея :(
						const int r = SelectSerial(goodsID, rBlk.Serial, &rBlk.PriceBySerial);
						ok = (r > 0 || r == -2) ? 1 : -1;
					}
					else {
						const long lbpp = CsObj.GetEqCfg().LookBackPricePeriod;
						if(lbpp > 0 && goods_rec.GoodsTypeID && gt_rec.Flags & GTF_LOOKBACKPRICES) {
							RetailGoodsInfo rgi;
							long   ext_rgi_flags = 0;
							GetRgi(goodsID, rBlk.Qtty, ext_rgi_flags, rgi);
							if(rgi.Price > 0.0) {
								RealArray lbpl;
								AsyncCashGoodsIterator::__GetDifferentPricesForLookBackPeriod(goodsID, locID, rgi.Price, lbpp, lbpl);
								if(lbpl.getCount()) {
									PPID   sel_id = 0;
									lbpl.atInsert(0, &rgi.Price);

									class LbpListDialog : public PPListDialog {
									public:
										LbpListDialog(const RealArray & rList, const SString * pInfoText) :
											PPListDialog(DLG_SELLKBKPRICE, CTL_SELLKBKPRICE_LIST, fOnDblClkOk), R_List(rList), Id(0)
										{
											if(pInfoText)
												setCtrlString(CTL_SELLKBKPRICE_INFO, *pInfoText);
											updateList(0);
										}
									protected:
										virtual int setupList()
										{
											SString temp_buf;
											SForEachVectorItem(R_List, i) { addStringToList(i+1, temp_buf.Z().Cat(R_List.at(i), MKSFMTD_020)); }
											return 1;
										}
										const RealArray & R_List;
										long   Id;
									};
									temp_buf = goods_rec.Name;
									LbpListDialog * p_dlg = new LbpListDialog(lbpl, &temp_buf);
									if(CheckDialogPtrErr(&p_dlg) && ExecView(p_dlg) == cmOK) {
										long   sel_pos = 0;
										p_dlg->getSelection(&sel_pos);
										if(sel_pos > 1 && sel_pos <= lbpl.getCountI()) {
											rBlk.PriceBySerial = lbpl.at(sel_pos-1);
											rBlk.Serial.Z();
										}
									}
									delete p_dlg;
								}
							}
						}
					}
					if(ok > 0) {
						const  bool skip_unprinted_checks = LOGIC(PNP.CnFlags & CASHF_SKIPUNPRINTEDCHECKS); // @v12.0.11
						bool   is_mark_processed = false;
						if(oneof3(PNP.EgaisMode, 1, 2, 3)) {
							PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor();
							if(p_eg_prc && p_eg_prc->IsAlcGoods(goodsID)) {
								PrcssrAlcReport::GoodsItem agi;
								if(p_eg_prc->PreprocessGoodsItem(goodsID, 0, 0, 0, agi) && agi.StatusFlags & agi.stMarkWanted) {
									is_mark_processed = true;
									const TimeRange & r_rsat = p_eg_prc->GetConfig().E.RtlSaleAllwTime;
									if(!r_rsat.IsZero() && !r_rsat.Check(getcurtime_())) {
										ok = MessageError(PPERR_ALCRETAILPROHIBTIME, r_rsat.ToStr(TIMF_HM, temp_buf), eomBeep|eomStatusLine);
									}
									else {
										SString egais_mark;
										rBlk.Qtty = 1.0; // Маркированная алкогольная продукция - строго по одной штуке на строку чека
										if(PPEgaisProcessor::InputMark(&agi, egais_mark) > 0) {
											bool   dup_mark = false;
											for(uint i = 0; !dup_mark && i < P.getCount(); i++) {
												if(egais_mark.IsEq(P.at(i).EgaisMark))
													dup_mark = true;
											}
											if(!dup_mark && egais_mark.IsEq(P.GetCur().EgaisMark))
												dup_mark = true;
											if(!dup_mark) {
												if(PNP.CnExtFlags & CASHFX_CHECKEGAISMUNIQ) {
													TSCollection <CCheckCore::ListByMarkEntry> lbm;
													CCheckCore & r_cc = GetCc();
													int    cc_even = 0;
													temp_buf.Z();
													CCheckCore::ListByMarkEntry * p_lbm_entry = lbm.CreateNewItem();
													STRNSCPY(p_lbm_entry->Mark, egais_mark);
													if(CsObj.GetListByEgaisMark(lbm) > 0) {
														SString debug_msg_buf;
														for(uint j = 0; j < p_lbm_entry->CcList.getCount(); j++) {
															const CCheckCore::CcMarkedEntry & r_ccm_entry = p_lbm_entry->CcList.at(j);
															const  PPID cc_id = r_ccm_entry.CcID;
															CCheckTbl::Rec cc_rec;
															if(r_cc.Search(cc_id, &cc_rec) > 0) {
																debug_msg_buf.Z().Cat("egais-mark").Colon().Cat(egais_mark).Space().
																	Cat("is found at").Space().Cat(cc_rec.Dt, DATF_YMD).Space().Cat(cc_rec.Tm, TIMF_HMS).Space().Cat(cc_rec.Code).Space().
																	CatHex(cc_rec.Flags);
																PPLogMessage(PPFILNAM_DEBUG_LOG, debug_msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
																// @v12.0.11 проверка на неотпечатанный чек если таковой считается неучетным
																if(!(cc_rec.Flags & CCHKF_JUNK) && (!skip_unprinted_checks || (cc_rec.Flags & CCHKF_PRINTED))) { 
																	int   take_in_attention = 0;
																	CSessionTbl::Rec cs_rec;
																	PPCashNode cn_rec;
																	if(r_ccm_entry.Flags & CCheckCore::CcMarkedEntry::fSent)
																		take_in_attention = 1;
																	else if(cc_rec.SessID) {
																		//
																		// Специальный случай: рестораны - вскрытие тары. Формируют отложенные чеки с отсканированными
																		// марками с бутылок. Повтор однозначно блокируется невзирая на продажи-возвраты (cc_even = 1)
																		//
																		if(CsObj.Fetch(cc_rec.SessID, &cs_rec) > 0 && CnObj.Fetch(cs_rec.CashNodeID, &cn_rec) > 0 && cn_rec.Speciality == cn_rec.spCafe) {
																			take_in_attention = 1;
																			cc_even = 1;
																			break;
																		}
																	}
																	if(take_in_attention) {
																		CCheckCore::MakeCodeString(&cc_rec, 0, temp_buf);
																		if(cc_rec.Flags & CCHKF_RETURN)
																			cc_even--;
																		else
																			cc_even++;
																	}
																}
															}
														}
													}
													if(cc_even & 1 && !F(fRetCheck))
														ok = MessageError(PPERR_DUPEGAISMARKINOTHRCC, temp_buf.Space().Cat(egais_mark), eomBeep|eomStatusLine);
													else if(!(cc_even & 1) && F(fRetCheck))
														ok = MessageError(PPERR_DUPEGAISMARKINOTHRCC, temp_buf.Space().Cat(egais_mark), eomBeep|eomStatusLine);
													else
														rBlk.EgaisMark = egais_mark;
												}
												else
													rBlk.EgaisMark = egais_mark;
											}
											else
												ok = MessageError(PPERR_DUPEGAISMARKINCC, egais_mark, eomBeep|eomStatusLine);
										}
										else
											ok = -1;
										selectCtrl(CTL_CHKPAN_INPUT);
									}
								}
							}
						}
						if(!is_mark_processed) {
							const bool is_simplified_draftbeer = PPSyncCashSession::IsSimplifiedDraftBeerPosition(PNP.NodeID, goodsID); // @v11.9.4
							if((gt_rec.Flags & GTF_GMARKED || (rBlk.Flags & PgsBlock::fMarkedBarcode)) && !is_simplified_draftbeer) {
								const int disable_chzn_mark_backtest = 0; // Проблемы с сигаретами - слишком много продаж и идентификация дубликатов занимает много времени // @v11.7.4 1-->0
								if(!(PNP.CnSpeciality == PPCashNode::spApteka && (rBlk.Qtty > 0.0 && rBlk.Qtty < 1.0))) // @v11.6.9
									rBlk.Qtty = 1.0; // Маркированная продукция - строго по одной штуке на строку чека (исключение: аптека и остаток менее 1)
								SString chzn_mark(rBlk.ChZnMark);
								int imr = -1000; // Result of the function PPChZnPrcssr::InputMark() (-1000 - wasn't called)
								// @v12.0.5 {
								if(ChZnMarkAutoSelect(goodsID, rBlk.Qtty, chzn_mark) > 0) {
									rBlk.ChZnMark = chzn_mark;
									imr  = 1;
								}
								else {
									chzn_mark = rBlk.ChZnMark; // @v12.0.12 функция ChZnMarkAutoSelect() обнулила буфер chzn_mark: придется восстановить
									// } @v12.0.5 
									if(chzn_mark.NotEmpty() || (imr = PPChZnPrcssr::InputMark(chzn_mark, 0, 0)) > 0) {
										bool   dup_mark = chzn_mark.IsEq(P.GetCur().ChZnMark);
										for(uint i = 0; !dup_mark && i < P.getCount(); i++) {
											if(chzn_mark.IsEq(P.at(i).ChZnMark))
												dup_mark = true;
										}
										if(!dup_mark) {
											if(!disable_chzn_mark_backtest && (PNP.CnExtFlags & CASHFX_CHECKEGAISMUNIQ)) {
												TSCollection <CCheckCore::ListByMarkEntry> lbm;
												CCheckCore & r_cc = GetCc();
												int    cc_even = 0;
												temp_buf.Z();
												CCheckCore::ListByMarkEntry * p_lbm_entry = lbm.CreateNewItem();
												STRNSCPY(p_lbm_entry->Mark, chzn_mark);
												if(CsObj.GetListByChZnMark(lbm) > 0) {
													for(uint j = 0; j < p_lbm_entry->CcList.getCount(); j++) {
														const  CCheckCore::CcMarkedEntry & r_ccm_entry = p_lbm_entry->CcList.at(j);
														const  PPID cc_id = r_ccm_entry.CcID;
														CCheckTbl::Rec cc_rec;
														// @v12.0.11 проверка на неотпечатанный чек если таковой считается неучетным
														if(r_cc.Search(cc_id, &cc_rec) > 0 && !(cc_rec.Flags & CCHKF_JUNK) && (!skip_unprinted_checks || (cc_rec.Flags & CCHKF_PRINTED))) {
															CCheckCore::MakeCodeString(&cc_rec, 0, temp_buf);
															if(cc_rec.Flags & CCHKF_RETURN)
																cc_even--;
															else
																cc_even++;
														}
													}
												}
												if(cc_even & 1 && !F(fRetCheck))
													ok = MessageError(PPERR_DUPCHZNMARKINOTHRCC, temp_buf.Space().Cat(chzn_mark), eomBeep|eomStatusLine);
												else if(!(cc_even & 1) && F(fRetCheck))
													ok = MessageError(PPERR_DUPCHZNMARKINOTHRCC, temp_buf.Space().Cat(chzn_mark), eomBeep|eomStatusLine);
												else
													rBlk.ChZnMark = chzn_mark;
											}
											else
												rBlk.ChZnMark = chzn_mark;
										}
										else
											ok = MessageError(PPERR_DUPCHZNMARKINCC, chzn_mark, eomBeep|eomStatusLine);
									}
									else if(PNP.CnSpeciality != PPCashNode::spApteka)
										ok = -1;
								}
								// @v12.0.12 {
								if(rBlk.ChZnMark.NotEmpty() && PNP.ChZnPermissiveMode == PPSyncCashNode::chznpmStrict && PNP.ChZnGuaID) {
									PPChZnPrcssr::PermissiveModeInterface::CodeStatusCollection pm_code_list;
									pm_code_list.AddCodeEntry(rBlk.ChZnMark, 0, 0);
									if(pm_code_list.getCount()) {
										PPChZnPrcssr::PmCheck(PNP.ChZnGuaID, 0, 2/*regular online/offline mode*/, pm_code_list);
										for(uint i = 0; i < pm_code_list.getCount(); i++) {
											const PPChZnPrcssr::PermissiveModeInterface::CodeStatus * p_cle = pm_code_list.at(i);
											if(p_cle) {
												if(CConfig.Flags2 & CCFLG2_RESTRICTCHZNPMPRICE) { // @v12.2.5
													// @v12.2.2 {
													const double _mrp = R2(p_cle->Mrp / 100.0);
													const double _smp = R2(p_cle->Smp / 100.0);
													if(gt_rec.ChZnProdType != GTCHZNPT_ALTTOBACCO) { // @v12.2.4 Для альтернативной табачной продукции ценовое ограничение не проверяем.
														if(_mrp > 0.0) {
															rBlk.AllowedPriceRange.upp = _mrp;
														}
														if(_smp > 0.0) {
															rBlk.AllowedPriceRange.low = _smp;
														}
														// @v12.2.4 {
														if(gt_rec.ChZnProdType == GTCHZNPT_TOBACCO) { // @v12.2.5 @fix (!=)-->(==)
															if(CConfig.Flags2 & CCFLG2_RESTRICTCHZNCIGPRICEASMRC) {
																if(_mrp > 0.0) {
																	rBlk.AllowedPriceRange.SetVal(_mrp);
																}
															}
														}
														// } @v12.2.4 
													}
													// } @v12.2.2 
												}
												{
													int    local_err_code = 0;
													if(p_cle->ErrorCode != 0)
														local_err_code = PPERR_CHZNMARKPMFAULT;
													else if(p_cle->Flags & PPChZnPrcssr::PermissiveModeInterface::CodeStatus::fSold)
														local_err_code = PPERR_CHZNMARKPMFAULT_SOLD;
													else if(checkdate(p_cle->ExpiryDtm.d) && now_dtm.d >= p_cle->ExpiryDtm.d) // @v12.1.1
														local_err_code = PPERR_CHZNMARKPMFAULT_EXPIRY;
													if(local_err_code) {
														ok = MessageError(local_err_code, chzn_mark, eomBeep|eomStatusLine);
													}
													else { // @v12.1.1
														// OK
														rBlk.ChZnPm_ReqId = pm_code_list.ReqId;
														rBlk.ChZnPm_ReqTimestamp = pm_code_list.ReqTimestamp;
														rBlk.ChZnPm_LocalModuleInstance = pm_code_list.LocalModuleInstance; // @v12.3.12
														rBlk.ChZnPm_LocalModuleDbVer    = pm_code_list.LocalModuleDbVer;    // @v12.3.12
													}
												}
											}
										}
									}
								}
								// } @v12.0.12
								if(imr != -1000)
									selectCtrl(CTL_CHKPAN_INPUT);
							}
						}
					}
				}
			}
		}
	}
	/* @v12.2.3 @fix (это была тяжелая ошибка) if(!ok) P.ClearCur();*/ // @v12.2.2
	return ok;
}

void FASTCALL CheckPaneDialog::SelectGoods__(int mode)
{
	int    r = 1;
	Flags |= fSuspSleepTimeout;
	if(PNP.CnFlags & CASHF_DISABLEZEROAGENT && !P.GetAgentID())
		r = MessageError(PPERR_CHKPAN_SALERNEEDED, 0, eomBeep | eomStatusLine);
	else if(Flags & fPrinted && !(OperRightsFlags & orfChgPrintedCheck))
		MessageError(PPERR_NORIGHTS, 0, eomBeep | eomStatusLine);
	else if(mode == sgmInnerGoodsList) {
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_CHKPAN_GDSLIST));
		if(SmartListBox::IsValidS(p_list)) {
			PPID   goods_id = 0;
			p_list->P_Def->getCurID(&goods_id);
			PgsBlock pgsb(1.0);
			r = PreprocessGoodsSelection(goods_id, 0, pgsb);
			if(r > 0) {
				r = SetupNewRow(goods_id, pgsb);
				// @v12.2.2 {
				if(!r)
					P.ClearCur();
				// } @v12.2.2 
			}
			ClearInput(0);
		}
	}
	else if(mode == sgmAbstractSale) {
        if(PNP.AbstractGoodsID && GetInput()) {
			PgsBlock pgsb(1.0);
			pgsb.AbstractPrice = R2(Input.ToReal());
			if(pgsb.AbstractPrice > 0.0) {
				r = SetupNewRow(PNP.AbstractGoodsID, pgsb);
			}
			ClearInput(0);
        }
	}
	else if(mode == sgmModifier) {
		if(P.HasCur() && !(P.GetCur().Flags & cifModifier)) {
			const  PPID main_goods_id = P.GetCur().GoodsID;
			if(main_goods_id) {
				SaModif mlist;
				if(LoadModifiers(main_goods_id, mlist) > 0) {
					class CpSelModDialog : public PPListDialog {
					public:
						enum {
							dummyFirst = 1,
							fontList,
							brSel,
							brOdd,
							brUnsel,
							brGrp,
							clrFocus,
							clrOdd,
							clrUnsel
						};

						CpSelModDialog(SaModif & rList) : PPListDialog(DLG_CPSELMOD, CTL_CPSELMOD_ELEMENTS), List(rList), ListEntryGap(5)
						{
							setSmartListBoxOption(CTL_CPSELMOD_ELEMENTS, lbtFocNotify);
							setSmartListBoxOption(CTL_CPSELMOD_ELEMENTS, lbtDblClkNotify);
							Ptb.SetColor(clrFocus,  RGB(0x20, 0xAC, 0x90));
							Ptb.SetColor(clrUnsel,  RGB(0xDA, 0xD7, 0xD0));
							Ptb.SetBrush(brSel,     SPaintObj::psSolid, Ptb.GetColor(clrFocus), 0);
							Ptb.SetBrush(brOdd,     SPaintObj::psSolid, Ptb.GetColor(clrOdd), 0);
							Ptb.SetBrush(brUnsel,   SPaintObj::psSolid, Ptb.GetColor(clrUnsel), 0);
							{
								SString temp_buf;
								LOGFONT log_font;
								MEMSZERO(log_font);
								log_font.lfCharSet = DEFAULT_CHARSET;
								PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIAL, temp_buf);
								STRNSCPY(log_font.lfFaceName, SUcSwitch(temp_buf));
								log_font.lfHeight = (DEFAULT_TS_FONTSIZE - TSGGROUPSASITEMS_FONTDELTA);
								Ptb.SetFont(fontList, ::CreateFontIndirect(&log_font));
							}
							updateList(-1);
						}
					private:
						DECL_HANDLE_EVENT
						{
							if(event.isCmd(cmLBDblClk)) {
								long   idx = 0;
								if(getSelection(&idx) && idx > 0 && idx <= List.getCountI())
									TVCMD = cmOK;
							}
							else if(event.isCmd(cmClear)) {
								if(IsInState(sfModal)) {
									endModal(cmClear);
									return; // После endModal не следует обращаться к this
								}
							}
							PPListDialog::handleEvent(event);
						}
						virtual int setupList()
						{
							int    ok = 1;
							SString sub;
							StringSet ss(SLBColumnDelim);
							for(uint i = 0; i < List.getCount(); i++) {
								ss.Z();
								const SaModifEntry & r_entry = List.at(i);
								GetGoodsName(r_entry.GoodsID, sub);
								ss.add(sub);
								ss.add(sub.Z().Cat(r_entry.Qtty, MKSFMTD_030));
								ss.add(sub.Z().Cat(r_entry.Price, SFMT_MONEY));
								addStringToList(i+1, ss.getBuf());
							}
							return ok;
						}
						SaModif List;
						SPaintToolBox Ptb;
						long   ListEntryGap;
					};
					CpSelModDialog * dlg = new CpSelModDialog(mlist);
					if(CheckDialogPtr(&dlg)) {
						{
							SString label_text, goods_name, new_label_text;
							GetGoodsName(main_goods_id, goods_name);
							dlg->getLabelText(CTL_CPSELMOD_ELEMENTS, label_text);
							new_label_text.Printf(label_text, goods_name.cptr());
							dlg->setLabelText(CTL_CPSELMOD_ELEMENTS, new_label_text);
						}
						dlg->enableCommand(cmClear, BIN(P.CurModifList.getCount()));
						int    cmd = ExecView(dlg);
						if(cmd == cmOK) {
							long   idx = -1;
							if(dlg->getSelection(&idx) && idx > 0 && idx <= mlist.getCountI()) {
								const SaModifEntry & r_entry = mlist.at(idx-1);
								P.CurModifList.insert(&r_entry);
								SetupRowData(true/*doCalcRest*/);
							}
						}
						else if(cmd == cmClear) {
							P.CurModifList.freeAll();
							SetupRowData(true/*doCalcRest*/);
						}
					}
					ZDELETE(dlg);
				}
			}
		}
	}
	else if(oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
		if(mode == sgmNormal)
			SetupRetCheck(true);
		else
			ClearInput(0);
	}
	else if(mode == sgmRandom) {
		PPIDArray  rand_gds_ary;
		if(GObj.GetRandomIdsAry(100, &rand_gds_ary) > 0) {
			for(uint i = 0; i < rand_gds_ary.getCount(); i++) {
				PgsBlock pgsb(1.0);
				SetupNewRow(rand_gds_ary.at(i), pgsb);
			}
			AcceptRow();
		}
	}
	else if(Flags & fTouchScreen) {
		if(mode == sgmByPrice)
			UpdateGList(-2, 0);
	}
	else if(mode == sgmByPrice && !GetInput()) {
		INVERSEFLAG(Flags, fSelByPrice);
	}
	else {
		Flags &= ~fSelByPrice;
		long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
		if(PNP.CnFlags & CASHF_SELALLGOODS)
			egsd_flags |= ExtGoodsSelDialog::fForceExhausted;
		// @v11.2.8 {
		if(PNP.CnExtFlags & CASHFX_USEGOODSMATRIX)
			egsd_flags |= ExtGoodsSelDialog::fForceMatrixUsage;
		// } @v11.2.8 
		SETIFZ(P_EGSDlg, new ExtGoodsSelDialog(GetCashOp(), 0, egsd_flags));
		if(CheckDialogPtrErr(&P_EGSDlg)) {
			PPWaitStart();
			if(GetInput()) {
				SString temp_buf(Input);
				if(mode == sgmByPrice && temp_buf.ToReal() != 0.0)
					P_EGSDlg->setSelectionByPrice(R2(temp_buf.ToReal()));
				else {
					StrAssocArray goods_list;
					if(temp_buf.Len() >= INSTVSRCH_THRESHOLD && temp_buf.C(0) != '!')
						temp_buf.Insert(0, "!");
					{
						GoodsFilt gf;
						gf.PutExtssData(GoodsFilt::extssNameText, temp_buf);
						if(temp_buf.IsEmpty() || mode != sgmAllByName) {
							if(!(PNP.CnFlags & CASHF_SELALLGOODS) && (temp_buf.NotEmpty() && mode != sgmAllByName)) {
								gf.Flags |= GoodsFilt::fActualOnly;
								gf.LocList.Add(PNP.CnLocID);
							}
						}
						// @v11.3.7 {
						if(PNP.CnExtFlags & CASHFX_USEGOODSMATRIX)
							gf.Flags |= GoodsFilt::fRestrictByMatrix;
						// } @v11.3.7 
						GoodsIterator::GetListByFilt(&gf, &goods_list, 1);
						if(goods_list.getCount())
							P_EGSDlg->setSelectionByGoodsList(&goods_list);
					}
				}
				ClearInput(0);
			}
			else {
				P_EGSDlg->setSelectionByGroup();
				SetupInfo(0);
			}
			PPWaitStop();
			if(ExecView(P_EGSDlg) == cmOK) {
				TIDlgInitData tidi;
				if(P_EGSDlg->getDTS(&tidi) > 0) {
					PgsBlock pgsb(tidi.Quantity);
					if(PreprocessGoodsSelection(tidi.GoodsID, 0, pgsb) > 0)
						r = SetupNewRow(tidi.GoodsID, pgsb);
					else
						r = 0;
				}
			}
		}
	}
	Flags &= ~fSuspSleepTimeout;
	if(r)
		SetupInfo(0);
}

int CheckPaneDialog::VerifyQuantity(PPID goodsID, double & rQtty, const CCheckItem * pCurItem, bool adjustQtty, bool checkInputBuffer)
{
	int    ok = 1;
	if(goodsID) {
		const  bool restr_qtty_by_unit = LOGIC(CsObj.GetEqCfg().Flags & PPEquipConfig::fRestrictQttyByUnitRnd);
		const  bool is_unlim = GObj.CheckFlag(goodsID, GF_UNLIM);
		if(rQtty <= 0.0 && adjustQtty) {
			rQtty = 1.0;
		}
		if(rQtty != 0.0) {
			SString temp_buf;
			Goods2Tbl::Rec goods_rec;
			PPUnit2 u_rec;
			if(GObj.Fetch(goodsID, &goods_rec) > 0 && GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
				;
			}
			else {
				goods_rec.Clear();
				MEMSZERO(u_rec);
			}
			//
			// Проверка на кратность единицы измерения //
			//
			if(restr_qtty_by_unit && !u_rec.ValidateQuantityFraction(rQtty)) {
				ok = MessageError(-1, 0, eomStatusLine|eomBeep);							
			}
			//
			// Маркированная алкогольная продукция - строго по одной штуке на строку чека
			//
			if(oneof3(PNP.EgaisMode, 1, 2, 3)) {
				PPEgaisProcessor * p_eg_prc = DS.GetTLA().GetEgaisProcessor(); // @v12.2.11
				if(p_eg_prc && p_eg_prc->IsAlcGoods(goodsID)) {
					PrcssrAlcReport::GoodsItem agi;
					if(p_eg_prc->PreprocessGoodsItem(goodsID, 0, 0, 0, agi) && agi.StatusFlags & agi.stMarkWanted) {
						if(rQtty != 1.0) {
							if(adjustQtty)
								rQtty = 1.0;
							else
								ok = MessageError(PPERR_EGAIS_MARKEDQTTY, 0, eomBeep|eomStatusLine);
						}
					}
				}
			}
			if(pCurItem && !isempty(pCurItem->ChZnMark)) { 
				PPGoodsType gt_rec;
				// @v11.2.5 if(CnSpeciality == PPCashNode::spApteka) {
				if(GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.ChZnProdType == GTCHZNPT_MEDICINE) { // @v11.2.5 
					if(rQtty < 1.0) {
						if(!u_rec.ValidateQuantityFraction(rQtty)) {
							ok = MessageError(-1, 0, eomStatusLine|eomBeep);							
						}
					}
					else if(rQtty != 1.0) {
						if(adjustQtty)
							rQtty = 1.0;
						else
							ok = MessageError(PPERR_CHZN_MARKEDQTTY, 0, eomBeep|eomStatusLine);
					}
				}
				else if(gt_rec.ChZnProdType == GTCHZNPT_TOBACCO && rQtty == 10.0) { // @v11.2.8
					// ok: блок сигарет (это - очевидный hardcoded-костыль, но пока ничего умнее нет)
				}
				else {
					if(rQtty != 1.0) {
						if(adjustQtty)
							rQtty = 1.0;
						else
							ok = MessageError(PPERR_CHZN_MARKEDQTTY, 0, eomBeep|eomStatusLine);
					}
				}
			}
			//
			// Проверка на непревышение текущего остатка (при установленном флаге CASHF_ABOVEZEROSALE)
			//
			if(ok) {
				int    rest_check_wanted = 0;
				if(!is_unlim) {
					if(PNP.CnFlags & CASHF_ABOVEZEROSALE)
						rest_check_wanted = 1;
					else if(adjustQtty && !(PNP.CnFlags & CASHF_SELALLGOODS))
						rest_check_wanted = 1;
				}
				if(rest_check_wanted) {
					const double rest = CalcCurrentRest(goodsID, checkInputBuffer); // @v11.0.3 @fix checkInputBuffer 0-->checkInputBuffer
					if(rest < rQtty) {
						const double __prec = (u_rec.Rounding_ != 0.0) ? u_rec.Rounding_ : 0.001;
						if(adjustQtty && rQtty == 1.0 && rest >= __prec) {
							rQtty = round(rest, __prec, -1);
							ok = 1;
						}
						else if(PNP.CnFlags & CASHF_ABOVEZEROSALE) {
							PPObject::SetLastErrObj(PPOBJ_GOODS, labs(goodsID));
							ok = MessageError(PPERR_LOTRESTBOUND, 0, eomBeep|eomStatusLine);
						}
						else if(adjustQtty) {
							GetGoodsName(goodsID, temp_buf);
							ok = ConfirmMessage(PPCFM_GOODSRESTNOTENOUGH, temp_buf, 1) ? 1 : -2;
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	return ok;
}

void CheckPaneDialog::AcceptQuantity()
{
	int    ok = -1;
	bool   is_input = false;
	const  PPID goods_id = P.HasCur() ? P.GetCur().GoodsID : 0;
	if(goods_id) {
		SString temp_buf;
		CCheckItem & r_cur = P.GetCur();
		const CCheckItem preserve_item = r_cur;
		double prev_qtty = r_cur.Quantity;
		double qtty = 0.0;
		is_input = GetInput();
		if(!is_input && ScaleID) {
			int  r = 0;
			while(r == 0 && !(Flags & fNotUseScale)) {
				r = GetDataFromScale(0, &qtty);
				if(r > 0)
					is_input = true;
				else if(!r && PPMessage(mfConf|mfYesNo, PPCFM_SCALENOTREADY) != cmYes)
					Flags |= fNotUseScale;
			}
		}
		if(!is_input) {
			showInputLineCalc(this, CTL_CHKPAN_INPUT);
			is_input = GetInput();
		}
		if(is_input) {
			int    last_slash = 0;
			Input.ShiftLeftChr('*').TrimRightChr('*');
			if(Input.Last() == '/') {
				last_slash = 1;
				Input.TrimRight();
			}
			if(qtty == 0.0) {
				double dr = 0.0, dd = 1.0;
				SString dr_buf, dd_buf;
				if(Input.Divide('/', dr_buf, dd_buf) > 0) {
					dr = dr_buf.ToReal();
					dd = dd_buf.ToReal();
					qtty = R6((dd != 0.0) ? (dr / dd) : dr);
				}
				else
					qtty = R6(Input.ToReal());
			}
			if(qtty > 0.0) {
				r_cur.PhQtty = 0.0;
				if(last_slash) {
					double phuperu;
					if(GObj.GetPhUPerU(goods_id, 0, &phuperu) > 0) {
						r_cur.PhQtty = qtty;
						qtty = R6(qtty / phuperu);
					}
				}
				ok = VerifyQuantity(goods_id, qtty, &r_cur, false/*adjustQtty*/, false/*checkInputBuffer*/);
				if(ok) {
					r_cur.Quantity = (goods_id == GetChargeGoodsID(CSt.GetID())) ? fabs(R3(qtty)) : qtty;
					//
					// Так как некоторые котировки могут зависеть от количества, при изменении количества необходимо
					// снова определить цену.
					//
					if(fabs(qtty) != fabs(prev_qtty) && !(r_cur.Flags & cifPriceBySerial)) {
						RetailGoodsInfo rgi;
						GetRgi(goods_id, qtty, 0, rgi);
						if(rgi.Price != 0.0 && r_cur.Price != 0 && rgi.Price != r_cur.Price)
							r_cur.Price = rgi.Price;
					}
					//
					if(IsCurrentOp(CCOP_RETURN)) {
						r_cur.Quantity = -r_cur.Quantity;
						r_cur.PhQtty   = -r_cur.PhQtty;
						SetupRowData(false/*doCalcRest*/);
						ok = 1;
					}
					else if(!CalcRestByCrdCard_(1)) {
						r_cur = preserve_item;
						ok = 0;
					}
					else {
						SetupRowData(false/*doCalcRest*/);
						ok = 1;
					}
				}
			}
		}
	}
	{
		// @v12.2.8 (судя по коду is_input никогда не принимает значение 2) if(is_input != 2) // При установке внешнего значение ввод очищать не следует
		{
			ClearInput(0);
		}
	}
}

void CheckPaneDialog::AcceptDivision()
{
	bool   is_input = GetInput();
	if(!is_input) {
		showInputLineCalc(this, CTL_CHKPAN_INPUT);
		is_input = GetInput();
	}
	if(is_input && P.HasCur() && P.GetCur().GoodsID && Input.IsDec()) {
		const long div = Input.ToLong();
		if(div > 0 && div < 1000) {
			P.GetCur().Division = static_cast<int16>(div);
			SetupRowData(false/*doCalcRest*/);
		}
	}
	ClearInput(0);
}

int CheckPaneDialog::AcceptRowDiscount()
{
	int    ok = -1;
	if(oneof3(GetState(), sEMPTYLIST_BUF, sLIST_BUF, sLISTSEL_BUF) && GetInput()) {
		char   prefx = Input[0];
		char   postfx = (sstrlen(Input) > 0) ? Input[sstrlen(Input) - 1] : 0;
		double pct_dis = 0.0;
		int    is_row_dis = 1;
		if(oneof3(prefx, '%', '/', '\\'))
			pct_dis = satof(Input + 1);
		else if(oneof3(postfx, '%', '/', '\\'))
			pct_dis = satof(Input);
		else
			is_row_dis = 0;
		if(is_row_dis) {
			if(pct_dis >= 0.0 && pct_dis <= 100.0) {
				if(OperRightsFlags & orfRowDiscount) {
					const  double price = P.GetCur().Price;
					double discount = round((price / 100.0) * pct_dis, 2);
					P.GetCur().Price = price - discount;
					P.GetCur().Flags |= cifFixedPrice;
					Flags &= ~fWaitOnSCard;
					SetupRowData(true/*doCalcRest*/);
					{
						CCheckLineTbl::Rec row;
						row.CheckID  = SelPack.Rec.ID;
						row.GoodsID  = P.GetCur().GoodsID;
						row.Quantity = P.GetCur().Quantity;
						row.Price    = dbltointmny(price);
						row.Dscnt    = discount;
						GetCc().WriteCCheckLogFile(&SelPack, &row, CCheckCore::logRowDiscount, 1);
					}
					ok = 1;
				}
				else
					ok = MessageError(PPERR_NORIGHTS, 0, eomBeep|eomStatusLine);
			}
			else
				ok = MessageError(PPERR_PERCENTINPUT, 0, eomBeep|eomMsgWindow);
		}
	}
	return ok;
}

class SCardInfoDialog : public PPListDialog {
	enum {
		ctlgroupIBG = 1
	};
public:
	SCardInfoDialog(PPID posNodeID, int asSelector) : PPListDialog(DLG_SCARDVIEW, CTL_SCARDVIEW_LIST),
		PosNodeID(posNodeID), LocalState(0), SCardID(0), OwnerID(0)
	{
		if(asSelector)
			LocalState |= stAsSelector;
		{
			PPObjSCardSeries scs_obj;
			scs_obj.GetSeriesWithSpecialTreatment(SpcTrtScsList);
		}
		addGroup(ctlgroupIBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_SCARDVIEW_IMAGE, cmAddImage, cmDelImage,
			PsnObj.CheckRights(PSNRT_UPDIMAGE), ImageBrowseCtrlGroup::fUseExtOpenDlg));
		selectCtrl(CTL_SCARDVIEW_INPUT);
		Ptb.SetColor(clrRed,   GetColorRef(SClrRed));
		Ptb.SetColor(clrGreen, GetColorRef(SClrGreen));
		Ptb.SetColor(clrYellow, GetColorRef(SClrYellow));
		Ptb.SetBrush(brRed,    SPaintObj::psSolid, Ptb.GetColor(clrRed),   0);
		Ptb.SetBrush(brGreen,  SPaintObj::psSolid, Ptb.GetColor(clrGreen), 0);
		Ptb.SetBrush(brYellow, SPaintObj::psSolid, Ptb.GetColor(clrYellow), 0);
		Ptb.SetBrush(brOrange, SPaintObj::psSolid, GetColorRef(SClrOrange), 0);
		Ptb.SetBrush(brMovCrdRest, SPaintObj::psSolid, SClrDarkviolet,  0);

		PPLoadStringS("check_pl", ChecksText).Transf(CTRANSF_INNER_TO_OUTER);
		PPLoadStringS("op_pl", OperationsText).Transf(CTRANSF_INNER_TO_OUTER);
		if(!(LocalState & stAsSelector))
			showCtrl(STDCTL_OKBUTTON, false);
		showButton(cmActivate, false);
		showButton(cmVerify, false);
		SetupMode(modeCheckView, 1);
	}
	int    setDTS(const  PPID * pData)
	{
		return SetupCard(DEREFPTRORZ(pData), 0);
	}
	int    getDTS(PPID * pData)
	{
		int    ok = 1;
		ImageBrowseCtrlGroup::Rec rec;
		if(OwnerID && PsnObj.CheckRights(PSNRT_UPDIMAGE) && getGroupData(ctlgroupIBG, &rec) && rec.Flags & ImageBrowseCtrlGroup::Rec::fUpdated) {
			long   set_f = 0;
			long   reset_f = 0;
			ObjLinkFiles _lf(PPOBJ_PERSON);
			_lf.Load(OwnerID, 0L);
			if(rec.Path.NotEmptyS() && fileExists(rec.Path)) {
				_lf.Replace(0, rec.Path);
				set_f = PSNF_HASIMAGES;
			}
			else {
				_lf.Remove(0);
				reset_f = PSNF_HASIMAGES;
			}
			THROW(_lf.Save(OwnerID, 0L));
			THROW(PsnObj.P_Tbl->UpdateFlags(OwnerID, set_f, reset_f, 1));
		}
		ASSIGN_PTR(pData, SCardID);
		CATCH
			ok = PPErrorZ();
		ENDCATCH
		return ok;
	}
	int GetStirb(SCardSpecialTreatment::IdentifyReplyBlock * pStirb)
	{
		if(Stirb.ScID && Stirb.SpecialTreatment) {
			ASSIGN_PTR(pStirb, Stirb);
			return 1;
		}
		else {
			CALLPTRMEMB(pStirb, Z());
			return 0;
		}
	}
private:
	enum {
		dummyFirst = 1,
		brRed,             // Красная кисть
		brGreen,           // Зеленая кисть
		brYellow,          // Желтая кисть
		brOrange,          // Оранжевая кисть
		brMovCrdRest,      // Цвет поля остатка по карте в режиме переноса остатков с других карт
		clrRed,
		clrGreen,
		clrYellow,
	};

	DECL_HANDLE_EVENT;
	virtual int editItem(long pos, long id);
	virtual int setupList();
	int    SetupMode(long mode, int force);
	int    SetupCard(PPID scardID, SCardSpecialTreatment::IdentifyReplyBlock * pStirb);
	void   SetupMovCrd();
	void   CommitMovCrd();
	enum {
		modeCheckView = 1,   // Режим просмотра чеков по карте
		modeOpView,          // Режим просмотра операций начисления/списания по кредитной карте
		modeSelectByOwner,   // Режим выбора карты по владельцу
		modeMovCrd,          // Режим переноса кредитных остатков с других карт на выбранную карту
		modeSelectByMultCode // Режим выбора одной из карт, имеющих один и тот же код
	};
	enum {
		stCreditCard     = 0x0002,
		stAsSelector     = 0x0004,
		stWarnCardInfo   = 0x0008, // Информация о карте должна окрашиваться для привлечения внимания к просроченности карты
		stNeedActivation = 0x0010, // Карта требует активации. Информация окрашивается в желтый цвет.
		stAutoActivation = 0x0020  // Автоактивация карты //
	};
	struct SpcListItem { // @flat
		SpcListItem()
		{
			THISZERO();
		}
		PPID   PersonID;
		PPID   SCardID;
		PPID   SCardSerID;
		uint   PersonNamePos;
		uint   SCardCodePos;
		LDATE  SCardExpiry;
		double Rest;
		double Amount;
	};
	class  SpcArray : public TSVector <SpcListItem>, public SStrGroup {
	public:
		SpcArray() : TSVector <SpcListItem> ()
		{
		}
		void   Clear()
		{
			freeAll();
			ClearS();
		}
		int    Add(PPID cardID, PPID cardSerID, const char * pCardCode, double rest, double amount)
		{
			int    ok = 1;
			SString temp_buf;
			SpcListItem item;
			item.SCardID = cardID;
			item.SCardSerID = cardSerID;
			item.Rest = rest;
			item.Amount = amount;
			temp_buf = pCardCode;
			if(!temp_buf.NotEmptyS())
				ideqvalstr(cardID, temp_buf);
			AddS(temp_buf, &item.SCardCodePos);
			insert(&item);
			return ok;
		}
		PPID   Get(uint pos, SString & rCardCode, SString & rCardSerName, double * pRest, double * pAmount)
		{
			rCardCode.Z();
			rCardSerName.Z();
			if(pos < getCount()) {
				const SpcListItem & r_item = at(pos);
				PPSCardSeries scs_rec;
				GetS(r_item.SCardCodePos, rCardCode);
				if(r_item.SCardSerID && ScsObj.Fetch(r_item.SCardSerID, &scs_rec) > 0)
					rCardSerName = scs_rec.Name;
				ASSIGN_PTR(pRest, r_item.Rest);
				ASSIGN_PTR(pAmount, r_item.Amount);
				return r_item.SCardID;
			}
			else
				return 0;
		}
		int    Add(PPID personID, PPID cardID, PPID cardSerID, const char * pPersonName, const char * pCardCode, LDATE expiry)
		{
			int    ok = 1;
			SString temp_buf;
			SpcListItem item;
			item.PersonID = personID;
			item.SCardID = cardID;
			item.SCardSerID = cardSerID;
			temp_buf = pPersonName;
			if(!temp_buf.NotEmptyS())
				ideqvalstr(personID, temp_buf);
			AddS(temp_buf, &item.PersonNamePos);
			temp_buf = pCardCode;
			if(!temp_buf.NotEmptyS())
				ideqvalstr(cardID, temp_buf);
			AddS(temp_buf, &item.SCardCodePos);
			item.SCardExpiry = expiry;
			insert(&item);
			return ok;
		}
		PPID   Get(uint pos, SString & rPersonName, SString & rCardCode, SString & rCardSerName, LDATE & rExpiry)
		{
			rPersonName.Z();
			rCardCode.Z();
			rCardSerName.Z();
			rExpiry = ZERODATE;
			if(pos < getCount()) {
				const SpcListItem & r_item = at(pos);
				rExpiry = r_item.SCardExpiry;
				GetS(r_item.PersonNamePos, rPersonName);
				GetS(r_item.SCardCodePos, rCardCode);
				if(r_item.SCardSerID) {
					PPSCardSeries scs_rec;
					if(ScsObj.Fetch(r_item.SCardSerID, &scs_rec) > 0)
						rCardSerName = scs_rec.Name;
				}
				return r_item.SCardID;
			}
			else
				return 0;
		}
	private:
		PPObjSCardSeries ScsObj;
	};

	const  PPID PosNodeID;
	long   Mode;
	long   LocalState;
	PPID   SCardID;
	PPID   OwnerID; // Персоналия-владелец карты
	SString ChecksText;
	SString OperationsText;
	SPaintToolBox Ptb;
	PPObjSCard  ScObj;
	PPObjPerson PsnObj;
	SpcArray OwnerList;
	LAssocArray SpcTrtScsList;
	SCardSpecialTreatment::IdentifyReplyBlock Stirb;
};

/*virtual*/int SCardInfoDialog::editItem(long pos, long id)
{
	if(id) {
		if(Mode == modeCheckView) {
			PPID   cn_id = 0;
			ScObj.P_CcTbl->GetNodeID(id, &cn_id);
			CCheckPane(cn_id, id);
		}
		else if(oneof2(Mode, modeSelectByOwner, modeSelectByMultCode)) {
			SetupCard(id, 0);
		}
	}
	return -1;
}

int SCardInfoDialog::SetupCard(PPID scardID, SCardSpecialTreatment::IdentifyReplyBlock * pStirb)
{
	const LDATETIME now_dtm = getcurdatetime_();
	SString temp_buf;
	SString card;
	SString info_buf;
	SString psn_name;
	SString sc_phone;
	SString psn_phone;
	SString series_name;
	int    uhtt_error = -1;
	double uhtt_saldo = 0.0;
	double local_saldo = 0.0;
	ImageBrowseCtrlGroup::Rec ibg_rec;
	PPObjSCardSeries scs_obj; // @erik v10.6.4
	PPSCardPacket sc_pack;
	SCardID = scardID;
	OwnerID = 0;
	LocalState &= ~(stCreditCard|stWarnCardInfo|stNeedActivation|stAutoActivation);
	setStaticText(CTL_SCARDVIEW_SCINFO, info_buf.Z());
	setStaticText(CTL_SCARDVIEW_OWNERINFO, info_buf);
	if(ScObj.GetPacket(SCardID, &sc_pack) > 0) {
		const SCardTbl::Rec & r_sc_rec = sc_pack.Rec;
		SETFLAG(LocalState, stCreditCard, ScObj.IsCreditSeries(r_sc_rec.SeriesID));
		local_saldo = sc_pack.Rec.Rest;
		card = r_sc_rec.Code;
		// @erik v10.6.4 {
		PPSCardSeries scs_rec;
		if(scs_obj.Fetch(r_sc_rec.SeriesID, &scs_rec) > 0) {
			series_name = scs_rec.Name;
			if(scs_rec.Flags & SCRDSF_UHTTSYNC) {
				PPUhttClient uhtt_cli;
				if(uhtt_cli.Auth()) {
					UhttSCardPacket scp;
					double uhtt_rest = 0.0;
					if(uhtt_cli.GetSCardByNumber(sc_pack.Rec.Code, scp))
						if(uhtt_cli.GetSCardRest(scp.Code, 0, uhtt_rest)) {
							uhtt_saldo = R2(uhtt_rest);
							uhtt_error = 0;
						}
						else
							uhtt_error = 1;
				}
				else
					uhtt_error = 1;
			}
		}
		// } v10.6.4
		{
			info_buf.Z();
			sc_pack.GetExtStrData(PPSCardPacket::extssPhone, sc_phone);
			// @erik v10.6.4 {
			if(series_name.NotEmptyS())
				info_buf.CatDivIfNotEmpty(' ', 0).Cat(PPLoadStringS("series", temp_buf)).CatDiv(':', 2).Cat(series_name);
			// } v10.6.4
			if(sc_phone.NotEmptyS())
				info_buf.CatDivIfNotEmpty(' ', 0).Cat(PPLoadStringS("phone", temp_buf)).CatDiv(':', 2).Cat(sc_phone);
			if(r_sc_rec.Expiry) {
				info_buf.CatDivIfNotEmpty(' ', 0).Cat(PPLoadStringS("validuntil_fem", temp_buf)).CatDiv(':', 2).Cat(r_sc_rec.Expiry, DATF_DMY);
				if(r_sc_rec.Expiry < now_dtm.d)
					LocalState |= stWarnCardInfo;
			}
			{
				SString added_msg_buf;
				TSVector <SCardCore::OpBlock> frz_op_list;
				if(ScObj.P_Tbl->GetFreezingOpList(SCardID, frz_op_list) > 0) {
					uint   info_pos = 0;
					for(uint i = 0; i < frz_op_list.getCount(); i++) {
						const SCardCore::OpBlock & r_ob = frz_op_list.at(i);
						if(r_ob.CheckFreezingPeriod(ZERODATE)) {
							if(r_ob.FreezingPeriod.CheckDate(now_dtm.d)) {
								LocalState |= stWarnCardInfo;
								info_pos = i+1;
								break;
							}
							else if(!info_pos && r_ob.FreezingPeriod.low > now_dtm.d)
								info_pos = i+1;
						}
					}
					if(info_pos) {
						const SCardCore::OpBlock & r_ob = frz_op_list.at(info_pos-1);
						info_buf.CatDivIfNotEmpty(' ', 0).CatChar('<').Cat(r_ob.FreezingPeriod).CatChar('>');
					}
				}
			}
			if(r_sc_rec.UsageTmStart || r_sc_rec.UsageTmEnd) {
				info_buf.CatDivIfNotEmpty(' ', 0).Cat(PPLoadStringS("time", temp_buf)).CatDiv(':', 2);
				if(r_sc_rec.UsageTmStart) {
					info_buf.Cat(r_sc_rec.UsageTmStart, TIMF_HM);
					if(r_sc_rec.UsageTmStart > now_dtm.t)
						LocalState |= stWarnCardInfo;
				}
				if(r_sc_rec.UsageTmEnd) {
					info_buf.Dot().Dot().Cat(r_sc_rec.UsageTmEnd, TIMF_HM);
					if(r_sc_rec.UsageTmEnd < now_dtm.t)
						LocalState |= stWarnCardInfo;
				}
			}
			if(r_sc_rec.Flags & SCRDF_CLOSED) {
				if(r_sc_rec.Flags & SCRDF_NEEDACTIVATION) {
					LocalState |= stNeedActivation;
					if(r_sc_rec.Flags & SCRDF_AUTOACTIVATION)
						LocalState |= stAutoActivation;
				}
				else
					LocalState |= stWarnCardInfo;
			}
			if(r_sc_rec.PDis) {
				PPLoadString("discount", temp_buf);
				info_buf.CatDivIfNotEmpty(' ', 0).Cat(temp_buf).CatDiv(':', 2).Cat(fdiv100i(r_sc_rec.PDis), MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar('%');
			}
			setStaticText(CTL_SCARDVIEW_SCINFO, info_buf);
		}
		info_buf.Z();
		PPPersonPacket psn_pack;
		if(PsnObj.GetPacket(r_sc_rec.PersonID, &psn_pack, 0) > 0) {
			OwnerID = r_sc_rec.PersonID;
			psn_name = psn_pack.Rec.Name;
			psn_pack.LinkFiles.Init(PPOBJ_PERSON);
			if(psn_pack.Rec.Flags & PSNF_HASIMAGES) {
				psn_pack.LinkFiles.Load(psn_pack.Rec.ID, 0L);
				psn_pack.LinkFiles.At(0, ibg_rec.Path);
			}
			{
				PPObjPersonStatus ps_obj;
				PPPersonStatus ps_rec;
				if(ps_obj.Fetch(psn_pack.Rec.Status, &ps_rec) > 0 && ps_rec.Flags & PSNSTF_PRIVATE) {
					const ObjTagItem * p_dob_tag = psn_pack.TagL.GetItem(PPTAG_PERSON_DOB);
					if(p_dob_tag) {
						LDATE  dob = ZERODATE;
						p_dob_tag->GetDate(&dob);
						if(checkdate(dob)) {
							LDATE curdt = now_dtm.d;
							int years = curdt.year() - dob.year();
							curdt.setyear(dob.year());
							if(curdt < dob)
								years--;
							PPLoadString("age", temp_buf);
							info_buf.Cat(temp_buf).CatDiv(':', 2).Cat(years).Space().CatChar('(').Cat(dob, DATF_DMY|DATF_CENTURY).CatChar(')');
						}
					}
				}
			}
			if(psn_pack.ELA.GetSinglePhone(psn_phone, 0) > 0) {
				info_buf.Space().Space().Cat(PPLoadStringS("phone", temp_buf)).CatDiv(':', 2).Cat(psn_phone);
			}
		}
		else
			info_buf.Z();
		setStaticText(CTL_SCARDVIEW_OWNERINFO, info_buf);
		setGroupData(ctlgroupIBG, &ibg_rec);
		{
			const bool enbl_psn = (OwnerID && PsnObj.CheckRights(PPR_MOD));
			const bool enbl_pic = PsnObj.CheckRights(PSNRT_UPDIMAGE);
			enableCommand(cmAddImage, enbl_pic);
			enableCommand(cmDelImage, enbl_pic);
			enableCommand(cmPasteImage, enbl_pic);
			enableCommand(cmEditPerson, enbl_psn);
		}
		showButton(cmActivate, (LocalState & stNeedActivation));
		showButton(cmVerify, sc_phone.NotEmpty() && !(sc_pack.Rec.Flags & SCRDF_OWNERVERIFIED));
		SetButtonText(cmCreateSCard, PPLoadStringS("but_edit", temp_buf).Transf(CTRANSF_INNER_TO_OUTER));
		OwnerList.Clear();
		updateList(-1);
	}
	else {
		SCardID = 0;
		setGroupData(ctlgroupIBG, &ibg_rec);
		SetButtonText(cmCreateSCard, PPLoadStringS("new_fem", temp_buf).Transf(CTRANSF_INNER_TO_OUTER));
	}
	if(pStirb && SCardID && pStirb->ScID == SCardID && pStirb->SpecialTreatment) {
		Stirb = *pStirb;
	}
	else
		Stirb.Z();
	{
		const  PPID charge_goods_id = (SCardID && (LocalState & stAsSelector)) ? ScObj.GetChargeGoodsID(SCardID) : 0;
		showButton(cmCharge, charge_goods_id);
	}
	setCtrlReal(CTL_SCARDVIEW_SALDO, (uhtt_error == 0) ? uhtt_saldo : local_saldo);
	setCtrlString(CTL_SCARDVIEW_OWNER, psn_name);
	setCtrlString(CTL_SCARDVIEW_CARD, card);
	enableCommand(cmCheckOpSwitch, (LocalState & stCreditCard) && ScObj.CheckRights(SCRDRT_VIEWOPS));
	enableCommand(cmSCardMovCrd,   (LocalState & stCreditCard) && ScObj.CheckRights(SCRDRT_ADDOPS));
	SetupMode((LocalState & stCreditCard) ? modeOpView : modeCheckView, 0);
	return 1;
}

int SCardInfoDialog::setupList()
{
	int    ok = -1;
	SString person_name, card_code, ser_name, temp_buf;
	StringSet ss(SLBColumnDelim);
	if(Mode == modeCheckView) {
		if(SCardID) {
			CCheckFilt flt;
			CCheckViewItem item;
			PPViewCCheck view;
			flt.Flags |= CCheckFilt::fAvoidExt;
			flt.CountOfLastItems = 1000;
			flt.SCardID = SCardID;
			THROW(view.Init_(&flt));
			for(view.InitIteration(0); view.NextIteration(&item) > 0;) {
				if(!(item.Flags & CCHKF_SKIP)) {
					ss.Z();
					ss.add(temp_buf.Z().Cat(item.Dt));                                // Дата
					ss.add(temp_buf.Z().Cat(item.Tm));                                // Время //
					ss.add(temp_buf.Z().Cat(item.PosNodeID));                         // Касса
					ss.add(temp_buf.Z().Cat(item.Code));                              // Номер чека
					ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Amount), SFMT_MONEY));   // Сумма
					ss.add(temp_buf.Z().Cat(MONEYTOLDBL(item.Discount), SFMT_MONEY)); // Скидка
					THROW(addStringToList(item.ID, ss.getBuf()));
				}
			}
			ok = 1;
		}
	}
	else if(Mode == modeOpView) {
		if(SCardID) {
			SCardOpFilt flt;
			SCardOpViewItem item;
			PPViewSCardOp   view;
			flt.SCardID = SCardID;
			THROW(view.Init_(&flt));
			view.InitIteration();
			for(uint i = 1; view.NextIteration(&item) > 0; i++) {
				ss.Z();
				ss.add(temp_buf.Z().Cat(item.Dt)); // Дата
				ss.add(temp_buf.Z().Cat(item.Tm)); // Время //
				if(item.Flags & SCARDOPF_FREEZING) {
					DateRange frz_prd;
					frz_prd.Set(item.FreezingStart, item.FreezingEnd);
					ss.add(temp_buf.Z().Cat(frz_prd));
					ss.add(temp_buf.Z());
				}
				else {
					ss.add(temp_buf.Z().Cat(item.Amount, SFMT_MONEY|NMBF_NOZERO)); // Сумма
					ss.add(temp_buf.Z().Cat(item.Rest, SFMT_MONEY|NMBF_NOZERO));   // Остаток
				}
				THROW(addStringToList(i, ss.getBuf()));
			}
			ok = 1;
		}
	}
	else if(oneof2(Mode, modeSelectByOwner, modeSelectByMultCode)) {
		for(uint i = 0; i < OwnerList.getCount(); i++) {
			LDATE  expiry;
			PPID   card_id = OwnerList.Get(i, person_name, card_code, ser_name, expiry);
			ss.Z();
			ss.add(person_name);
			ss.add(card_code);
			ss.add(ser_name);
			temp_buf.Z();
			if(checkdate(expiry))
				temp_buf.Cat(expiry);
			ss.add(temp_buf);
			THROW(addStringToList(card_id, ss.getBuf()));
		}
	}
	else if(Mode == modeMovCrd) {
		for(uint i = 0; i < OwnerList.getCount(); i++) {
			double rest = 0.0, amount = 0.0;
			PPID card_id = OwnerList.Get(i, card_code, ser_name, &rest, &amount);
			ss.Z();
			ss.add(card_code);
			ss.add(ser_name);
			ss.add(temp_buf.Z().Cat(rest, MKSFMTD(0, 2, NMBF_NOZERO)));
			ss.add(temp_buf.Z().Cat(amount, MKSFMTD(0, 2, NMBF_NOZERO)));
			THROW(addStringToList(card_id, ss.getBuf()));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SCardInfoDialog::SetupMode(long mode, int force)
{
	long   prev_mode = Mode;
	if(oneof5(mode, modeCheckView, modeOpView, modeSelectByOwner, modeMovCrd, modeSelectByMultCode) && (force || mode != Mode)) {
		int    skip = 0;
		if(mode == modeMovCrd && !ScObj.CheckRights(SCRDRT_ADDOPS))
			skip = 1;
		else if(mode == modeOpView && !ScObj.CheckRights(SCRDRT_VIEWOPS))
			skip = 1;
		if(!skip) {
			Mode = mode;
			enableCommand(cmaEdit, Mode == modeCheckView);
			enableCommand(cmSelectByOwner, Mode != modeMovCrd);
			enableCommand(cmCommit, Mode == modeMovCrd && OwnerList.getCount());
			showButton(cmCommit, Mode == modeMovCrd && OwnerList.getCount());
			if(P_Box) {
				SString columns_buf, text, temp_buf;
				if(Mode == modeCheckView) {
					SetButtonText(cmCheckOpSwitch, OperationsText);
					setLabelText(CTL_SCARDVIEW_LIST, (text = ChecksText).Transf(CTRANSF_OUTER_TO_INNER));
					columns_buf = "@lbt_scardcheck";
				}
				else if(Mode == modeOpView) {
					SetButtonText(cmCheckOpSwitch, ChecksText);
					setLabelText(CTL_SCARDVIEW_LIST, (text = OperationsText).Transf(CTRANSF_OUTER_TO_INNER));
					columns_buf = "@lbt_scardop";
				}
				else if(Mode == modeSelectByOwner) {
					PPLoadString("person", temp_buf);
					setLabelText(CTL_SCARDVIEW_LIST, temp_buf);
					columns_buf = "@lbt_selscardbyowner";
				}
				else if(Mode == modeSelectByMultCode) {
					PPLoadString("code", temp_buf);
					setLabelText(CTL_SCARDVIEW_LIST, temp_buf);
					columns_buf = "@lbt_selscardbyowner";
				}
				else if(Mode == modeMovCrd) {
					setLabelText(CTL_SCARDVIEW_LIST, PPLoadTextS(PPTXT_MOVSCARDREST, temp_buf));
					columns_buf = "@lbt_scardmovlist";
				}
				enableCommand(cmCheckOpSwitch, !oneof2(Mode, modeSelectByOwner, modeSelectByMultCode));
				P_Box->SetupColumns(columns_buf);
				updateList(-1);
			}
		}
	}
	return prev_mode;
}

void SCardInfoDialog::SetupMovCrd()
{
	double rest = 0.0;
	ScObj.P_Tbl->GetRest(SCardID, MAXDATE, &rest);
	for(uint i = 0; i < OwnerList.getCount(); i++) {
		double v = 0.0;
		double amt = OwnerList.at(i).Amount;
		ScObj.P_Tbl->GetRest(OwnerList.at(i).SCardID, MAXDATE, &v);
		if(amt > 0.0 && amt <= v)
			rest += amt;
	}
	setCtrlReal(CTL_SCARDVIEW_SALDO, rest);
	enableCommand(cmCommit, OwnerList.getCount());
	showButton(cmCommit, OwnerList.getCount());
	updateList(-1);
}

void SCardInfoDialog::CommitMovCrd()
{
	int    ok = 1;
	const  uint c = OwnerList.getCount();
	if(Mode == modeMovCrd && c) {
		TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
		THROW(ScObj.CheckRights(SCRDRT_ADDOPS));
		PPTransaction tra(1);
		THROW(tra);
		for(uint i = 0; i < c; i++) {
			SpcListItem & r_item = OwnerList.at(i);
			if(r_item.SCardID && r_item.Amount > 0.0) {
				SCardCore::OpBlock op;
				op.SCardID = r_item.SCardID;
				op.DestSCardID = SCardID;
				op.Amount = r_item.Amount;
				op.Dtm = getcurdatetime_();
				THROW(ScObj.P_Tbl->PutOpBlk(op, &urn_list, 0));
			}
		}
		THROW(tra.Commit());
		ScObj.FinishSCardUpdNotifyList(urn_list);
		SetupMode(modeOpView, 1);
	}
	CATCH
		PPError();
	ENDCATCH
}

IMPL_HANDLE_EVENT(SCardInfoDialog)
{
	SCardTbl::Rec sc_rec;
	if(event.isCmd(cmCharge)) {
		SString code;
		getCtrlString(CTL_SCARDVIEW_INPUT, code);
		if(!code.NotEmptyS() && IsInState(sfModal)) {
			PPID   charge_goods_id = (SCardID && LocalState & stAsSelector) ? ScObj.GetChargeGoodsID(SCardID) : 0;
			if(charge_goods_id) {
				endModal(cmCharge);
				return; // После endModal не следует обращаться к this
			}
		}
	}
	if(event.isCmd(cmOK)) {
		if(oneof2(Mode, modeSelectByOwner, modeSelectByMultCode)) {
			PPID   sc_id = 0;
			if(getSelection(&sc_id) && sc_id)
				SetupCard(sc_id, 0);
		}
		else {
			SString code;
			getCtrlString(CTL_SCARDVIEW_INPUT, code);
			if(LocalState & stAsSelector && !code.NotEmptyS() && IsInState(sfModal)) {
				endModal(cmOK);
				return; // После endModal не следует обращаться к this
			}
			else {
				PPIDArray sc_id_list;
				TSVector <SCardSpecialTreatment::IdentifyReplyBlock> scp_rb_list;
				if(ScObj.SearchCodeExt(code, &SpcTrtScsList, sc_id_list, scp_rb_list) > 0) {
					assert(sc_id_list.getCount());
					if(sc_id_list.getCount()) {
						PPID   single_sc_id = sc_id_list.get(0);
						//const int mc = ScObj.P_Tbl->GetListByCode(code, &mult_list);
						if(sc_id_list.getCount() > 1) {
							OwnerList.Clear();
							SString psn_name;
							for(uint i = 0; i < sc_id_list.getCount(); i++) {
								const  PPID sc_id = sc_id_list.get(i);
								if(ScObj.Search(sc_id, &sc_rec) > 0) {
									psn_name = 0;
									if(sc_rec.PersonID)
										GetPersonName(sc_rec.PersonID, psn_name);
									OwnerList.Add(sc_rec.PersonID, sc_id, sc_rec.SeriesID, psn_name, sc_rec.Code, sc_rec.Expiry);
								}
							}
							if(OwnerList.getCount()) {
								SetupMode(modeSelectByMultCode, 1);
							}
						}
						else if(ScObj.Search(sc_id_list.get(0), &sc_rec) > 0) {
							assert(sc_id_list.getCount() == 1);
							if(Mode == modeMovCrd) {
								if(sc_rec.ID != SCardID && !OwnerList.lsearch(&sc_rec.ID, 0, CMPF_LONG, offsetof(SpcListItem, SCardID))) {
									if(ScObj.IsCreditSeries(sc_rec.SeriesID) && sc_rec.Rest > 0.0) {
										OwnerList.Add(sc_rec.ID, sc_rec.SeriesID, sc_rec.Code, sc_rec.Rest, sc_rec.Rest);
										SetupMovCrd();
									}
								}
							}
							else {
								SCardSpecialTreatment::IdentifyReplyBlock * p_stirb = CPosProcessor::GetSpecialTreatmentBlock(scp_rb_list, sc_rec.ID);
								SetupCard(sc_rec.ID, p_stirb);
							}
						}
					}
				}
				setCtrlString(CTL_SCARDVIEW_INPUT, code.Z());
			}
		}
		clearEvent(event);
	}
	else
		PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmClear:
				SetupCard(0, 0);
				selectCtrl(CTL_SCARDVIEW_INPUT);
				break;
			case cmEditPerson: 
				if((OwnerID && PsnObj.CheckRights(PPR_MOD)) && PsnObj.Edit(&OwnerID, 0) > 0)
              			SetupCard(SCardID, 0);
				break;
			case cmCreateSCard:
				if(SCardID) {
					PPID   temp_sc_id = SCardID;
					if(ScObj.Edit(&temp_sc_id, 0) > 0) {
						assert(temp_sc_id == SCardID);
						SetupCard(SCardID, 0);
					}
				}
				else {
					const  int preserve_slui_flag = SLS.CheckUiFlag(sluifUseLargeDialogs);
					bool   do_create = false;
					PPSCardConfig sc_cfg;
					PPObjSCardSeries scs_obj;
					PPSCardSeries scs_rec;
					PPObjPerson::EditBlock peb;
					ScObj.FetchConfig(&sc_cfg);
					if(scs_obj.Fetch(sc_cfg.DefCreditSerID, &scs_rec) > 0) {
						PsnObj.InitEditBlock(NZOR(scs_rec.PersonKindID, NZOR(sc_cfg.PersonKindID, PPPRK_CLIENT)), peb);
						peb.SCardSeriesID = sc_cfg.DefCreditSerID;
						do_create = true;
					}
					else if(scs_obj.Fetch(sc_cfg.DefSerID, &scs_rec) > 0) {
						PsnObj.InitEditBlock(NZOR(scs_rec.PersonKindID, NZOR(sc_cfg.PersonKindID, PPPRK_CLIENT)), peb);
						peb.SCardSeriesID = sc_cfg.DefSerID;
						do_create = true;
					}
					if(do_create) {
						peb.ShortDialog = 1;
						PPID   psn_id = 0;
						if(PsnObj.Edit_(&psn_id, peb) == cmOK)
							SetupCard(peb.RetSCardID, 0);
					}
					selectCtrl(CTL_SCARDVIEW_INPUT);
					SLS.SetUiFlag(sluifUseLargeDialogs, preserve_slui_flag);
				}
				break;
			case cmSelectByOwner: 
				{
					SString text;
					getCtrlString(CTL_SCARDVIEW_INPUT, text);
					if(text.Strip().Len() > 0/*2*/) {
						OwnerList.Clear();
						PersonTbl::Rec psn_rec;
						PPIDArray psn_list;
						PPIDArray sc_list;
						PPObjPerson::SrchAnalogPattern sap(text, 0);
						PsnObj.GetListByPattern(&sap, &psn_list);
						for(uint i = 0; i < psn_list.getCount(); i++) {
							const  PPID psn_id = psn_list.get(i);
							sc_list.clear();
							if(ScObj.P_Tbl->GetListByPerson(psn_id, 0, &sc_list) > 0) {
								if(PsnObj.Fetch(psn_id, &psn_rec) > 0) {
									for(uint j = 0; j < sc_list.getCount(); j++) {
										const  PPID sc_id = sc_list.get(j);
										if(ScObj.Search(sc_id, &sc_rec) > 0) {
											OwnerList.Add(psn_id, sc_id, sc_rec.SeriesID, psn_rec.Name, sc_rec.Code, sc_rec.Expiry);
										}
									}
								}
							}
						}
						if(OwnerList.getCount())
							SetupMode(modeSelectByOwner, 1);
					}
					setCtrlString(CTL_SCARDVIEW_INPUT, text.Z());
				}
				break;
			case cmSCardMovCrd:
				if(SCardID && LocalState & stCreditCard) {
					if(Mode != modeMovCrd)
						OwnerList.Clear();
					SetupMode(modeMovCrd, 0);
					selectCtrl(CTL_SCARDVIEW_INPUT);
				}
				break;
			case cmCommit:
				CommitMovCrd();
				selectCtrl(CTL_SCARDVIEW_INPUT);
				break;
			case cmCheckOpSwitch:
				if(SCardID) {
					if(Mode == modeCheckView)
						SetupMode(modeOpView, 0);
					else if(Mode == modeOpView)
						SetupMode(modeCheckView, 0);
				}
				break;
			case cmActivate:
				if(LocalState & stNeedActivation && ScObj.Search(SCardID, &sc_rec) > 0) {
					if(ScObj.ActivateRec(&sc_rec) > 0) {
						if(ScObj.P_Tbl->Update(sc_rec.ID, &sc_rec, 1))
							SetupCard(sc_rec.ID, 0);
						else
							PPError();
					}
				}
				break;
			case cmVerify:
				if(SCardID) {
					PPSCardPacket sc_pack;
					if(ScObj.GetPacket(SCardID, &sc_pack) > 0) {
						const int r = ScObj.VerifyOwner(sc_pack, PosNodeID, 1 /*updatImmediately*/);
						if(r > 0)
							SetupCard(sc_rec.ID, 0);
						else if(!r)
							PPError();
					}
				}
				break;
			case cmCtlColor:
				{
					TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
					if(p_dc && getCtrlHandle(CTL_SCARDVIEW_SALDO) == p_dc->H_Ctl) {
						const double saldo = getCtrlReal(CTL_SCARDVIEW_SALDO);
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
						int  br_ident = brRed; // default color
						if(Mode == modeMovCrd)
							br_ident = brMovCrdRest;
						else if(!(LocalState & stCreditCard) || saldo > 0.0)
							br_ident = brGreen;
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(br_ident));
	 				}
					else if(p_dc && getCtrlHandle(CTL_SCARDVIEW_SCINFO) == p_dc->H_Ctl) {
						if(LocalState & stWarnCardInfo) {
							::SetBkMode(p_dc->H_DC, TRANSPARENT);
							::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brRed));
						}
						else if(LocalState & stNeedActivation) {
							::SetBkMode(p_dc->H_DC, TRANSPARENT);
							::SetTextColor(p_dc->H_DC, GetColorRef(SClrBlack));
							p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get((LocalState & stAutoActivation) ? brOrange : brYellow));
						}
						else
							return;
					}
					else
						return;
				}
				break;
			default:
				return;
		}
		clearEvent(event);
	}
}

static int Helper_ViewSCardInfo(PPID * pSCardID, PPID posNodeID, int asSelector, SCardSpecialTreatment::IdentifyReplyBlock * pStirb)
{
	//DIALOG_PROC_BODY_P1(SCardInfoDialog, asSelector, pSCardID);
	int    ok = -1;
	SCardInfoDialog * dlg = new SCardInfoDialog(posNodeID, asSelector);
	if(CheckDialogPtrErr(&dlg) && dlg->setDTS(pSCardID)) {
		int    cm = 0;
		while(ok <= 0 && ((cm = ExecView(dlg)) == cmOK || cm == cmCharge)) {
			if(dlg->getDTS(pSCardID)) {
				dlg->GetStirb(pStirb);
				if(cm == cmCharge)
					ok = 2;
				else
					ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int STDCALL ViewSCardInfo(PPID * pSCardID, PPID posNodeID, int asSelector)
{
	const int preserve_use_large_dialogs_flags = SLS.CheckUiFlag(sluifUseLargeDialogs);
	SLS.SetUiFlag(sluifUseLargeDialogs, 0);
	int    ok = Helper_ViewSCardInfo(pSCardID, posNodeID, asSelector, 0);
	SLS.SetUiFlag(sluifUseLargeDialogs, preserve_use_large_dialogs_flags);
	return ok;
}

int CPosProcessor::Implement_AcceptSCard(const SCardTbl::Rec & rScRec, const SCardSpecialTreatment::IdentifyReplyBlock * pStirb)
{
	//
	// Реализует окончательную не интерактивную установку найденной в БД карты
	//
	// @todo необходимо уточнить несколько последующих после вызовов этой функции операторов -
	//  вероятно, их следует внести в тело этой функции.
	//
	int    ok = 1;
	CSt.SetID(rScRec.ID, rScRec.Code);
	ZDELETE(CSt.P_DisByAmtRule);
	{
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		PPSCardSerPacket scs_pack;
		if(scs_obj.GetPacket(rScRec.SeriesID, &scs_pack) > 0) {
			ZDELETE(CSt.P_Eqb);
			RetailPriceExtractor::ExtQuotBlock temp_eqb(scs_pack);
			if(temp_eqb.QkList.getCount()) {
				CSt.P_Eqb = new RetailPriceExtractor::ExtQuotBlock(scs_pack);
			}
			SETFLAG(CSt.Flags, CardState::fUseDscntIfNQuot, scs_pack.Rec.Flags & SCRDSF_USEDSCNTIFNQUOT || !scs_pack.Rec.QuotKindID_s);
			SETFLAG(CSt.Flags, CardState::fUseMinQuotVal,   scs_pack.Rec.Flags & SCRDSF_MINQUOTVAL);
			if(scs_pack.Rec.Flags & SCRDSF_UHTTSYNC) {
				int   uhtt_err = 1;
				PPUhttClient uhtt_cli;
				if(uhtt_cli.Auth()) {
					UhttSCardPacket scp;
					if(uhtt_cli.GetSCardByNumber(rScRec.Code, scp)) {
						CSt.Flags |= CardState::fUhtt;
						scp.Code.CopyTo(CSt.CSTRB.OperationCode, sizeof(CSt.CSTRB.OperationCode));
						scp.Hash.CopyTo(CSt.CSTRB.Hash, sizeof(CSt.CSTRB.Hash));
						double uhtt_rest = 0.0;
						if(uhtt_cli.GetSCardRest(CSt.CSTRB.OperationCode, 0, uhtt_rest)) {
							CSt.CSTRB.Flags |= SCardSpecialTreatment::IdentifyReplyBlock::fDefinedRest;
							CSt.CSTRB.Rest = R2(uhtt_rest);
							uhtt_err = 0;
						}
					}
				}
				if(uhtt_err) {
					CSt.Flags &= ~CardState::fUhtt;
					CSt.CSTRB.Rest = 0.0;
				}
			}
			else if(pStirb) {
				CSt.CSTRB = *pStirb;
			}
			if(rScRec.Flags & SCRDF_INHERITED && scs_pack.CcAmtDisRule.getCount()) {
				CSt.P_DisByAmtRule = new PPSCardSerRule;
				ASSIGN_PTR(CSt.P_DisByAmtRule, scs_pack.CcAmtDisRule);
			}
			CSt.CSTRB.SpecialTreatment = scs_pack.Rec.SpecialTreatment;
		}
		else {
			ZDELETE(CSt.P_Eqb);
			CSt.Flags &= ~(CardState::fUseDscntIfNQuot|CardState::fUseMinQuotVal);
		}
	}
	SETFLAG(CSt.Flags, CardState::fNoGift, rScRec.Flags & SCRDF_NOGIFT);
	CSt.OwnerID = rScRec.PersonID;
	if(CSt.OwnerID) {
		// Детектируем день рождения владельца карты
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(PPOBJ_PERSON, CSt.OwnerID, PPTAG_PERSON_DOB, &tag_item) > 0) {
			LDATE   dob_dt = ZERODATE;
			if(tag_item.GetDate(&dob_dt)) {
				const LDATE cdt = getcurdate_();
				if(dob_dt.day() == cdt.day() && dob_dt.month() == cdt.month())
					CSt.Flags |= CardState::fBirthday;
			}
		}
	}
	return ok;
}

int CPosProcessor::SetupSCard(PPID scID, const SCardTbl::Rec * pScRec)
{
	int    ok = 1;
	SCardTbl::Rec sc_rec;
	if(scID) {
		if(pScRec == 0) {
			THROW(ScObj.Search(scID, &sc_rec) > 0);
			pScRec = &sc_rec;
		}
		assert(pScRec);
		THROW(Implement_AcceptSCard(*pScRec, 0));
		if(pScRec->AutoGoodsID) {
			PgsBlock pgsb(1.0);
			SetupNewRow(pScRec->AutoGoodsID, pgsb);
		}
		CSt.Discount = fdiv100i(pScRec->PDis);
		Flags |= fPctDis;
		SetupDiscount(0); // Вызов SetupDiscount перед CalcRestByCrdCard_ необходим для корректного расчета остатка по кредитной (бонусной) карте
		if(!CalcRestByCrdCard_(0))
			ResetSCard();
	}
	else
		ResetSCard();
	OnUpdateList(0);
	CATCHZOK
	return ok;
}

int CPosProcessor::Backend_AcceptSCard(PPID scardID, const SCardSpecialTreatment::IdentifyReplyBlock * pStirb, uint ascf)
{
	int    ok = 1;
	SString temp_buf;
	if(Flags & fPrinted && !(OperRightsFlags & orfChgPrintedCheck) && !(ascf & ascfIgnoreRights)) {
		ok = MessageError(PPERR_NORIGHTS, 0, eomBeep | eomStatusLine);
		Flags &= ~fWaitOnSCard;
	}
	else {
		Flags |= fSuspSleepTimeout;
		const  bool prev_no_gift_status = LOGIC(CSt.Flags & CardState::fNoGift);
		//
		// Признак того, что скидка устанавливается только на текущую строку чека (1 скидка по строке, 0 - error, -1 - скидка по карте)
		//
		int    row_dscnt = -1;
		CSt.SetID(scardID, 0);
		//
		if(row_dscnt < 0 && !oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
			SCardTbl::Rec sc_rec;
			const  PPID sc_id = CSt.GetID();
			//
			// Если мы в состоянии начисления на кредитную карту, то менять карту уже нельзя //
			//
			if(sc_id) {
				const  PPID charge_goods_id = GetChargeGoodsID(sc_id);
				if(charge_goods_id != UNDEF_CHARGEGOODSID) {
					for(uint i = 0; ok && i < P.getCount(); i++)
						if(P.at(i).GoodsID == charge_goods_id)
							ok = MessageError(PPERR_UNABLECHNGCHARGESCARD, 0, eomStatusLine);
				}
			}
			if(ok) {
				if(sc_id && ScObj.Search(sc_id, &sc_rec) > 0) {
					const bool only_charge_goods = IsOnlyChargeGoodsInPacket(sc_id, 0);
					const int cr = ScObj.CheckRestrictions(&sc_rec, (only_charge_goods ? (PPObjSCard::chkrfIgnoreUsageTime|PPObjSCard::chkrfIgnoreUsageDow) : 0), getcurdatetime_());
					if(!cr) {
						ok = MessageError(-1, 0, eomPopup | eomBeep);
						CSt.SetID(0, 0);
					}
					else if(cr == 2) {
						/*
						SString msg_buf;
						PPLoadText(PPTXT_SCARDISAUTOACTIVATED, temp_buf.Z());
						msg_buf.Printf(temp_buf, sc_rec.Code);
						PPTooltipMessage(msg_buf, 0, hWnd, 10000, GetColorRef(SClrOrange),
							SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
						*/
					}
					else {
						THROW(Implement_AcceptSCard(sc_rec, pStirb));
						if(sc_rec.AutoGoodsID) {
							PgsBlock pgsb(1.0);
							SetupNewRow(sc_rec.AutoGoodsID, pgsb);
						}
						CSt.Discount = fdiv100i(sc_rec.PDis);
						Flags |= fPctDis;
						SetupDiscount(0); // Вызов SetupDiscount перед CalcRestByCrdCard_ необходим для корректного расчета остатка по кредитной (бонусной) карте
						if(CalcRestByCrdCard_(0)) {
							AutosaveCheck();
						}
						else {
							ResetSCard();
							ok = 0;
						}
						OnUpdateList(0);
					}
					Flags &= ~fWaitOnSCard;
				}
				else {
					if(Flags & fWaitOnSCard) {
						ResetSCard();
						OnUpdateList(0);
					}
					INVERSEFLAG(Flags, fWaitOnSCard);
				}
				SetupInfo(0);
				/*
				if(IsState(sLIST_EMPTYBUF))
					enableCommand(cmCash, !(Flags & fSCardCredit) || Flags & fWaitOnSCard); // @scard
				*/
			}
		}
		if(LOGIC(CSt.Flags & CardState::fNoGift) != prev_no_gift_status)
			ProcessGift();
		Flags &= ~fSuspSleepTimeout;
	}
	CATCHZOK
	return ok;
}

void CheckPaneDialog::AcceptManualDiscount()
{
	// @construction
	if(CheckRights(CPosProcessor::orfArbitraryDiscount) && oneof4(GetState(), sEMPTYLIST_BUF, sLIST_BUF, sLISTSEL_BUF, sLIST_EMPTYBUF)) {
		if(GetInput()) {
			char   prefx = Input[0];
			char   postfx = (sstrlen(Input) > 0) ? Input[sstrlen(Input) - 1] : 0;
			double pct_dis = 0.0;
			double abs_dis = 0.0;
			if(oneof3(prefx, '%', '/', '\\'))
				pct_dis = satof(Input + 1);
			else if(oneof3(postfx, '%', '/', '\\'))
				pct_dis = satof(Input);
			else {
				abs_dis = satof(Input);
			}
			if(pct_dis > 0.0 && pct_dis <= 100.0) {
				ManDis.Discount = pct_dis;
				ManDis.Flags |= ManualDiscount::fPct;
			}
			else if(abs_dis > 0.0) {
				ManDis.Discount = abs_dis;
				ManDis.Flags &= ~ManualDiscount::fPct;
			}
			else
				ManDis.Z();
		}
		else {
			ManDis.Z();
		}
		ClearInput(0);
		SetupDiscount(0);
		OnUpdateList(0);
	}
}

void CheckPaneDialog::AcceptSCard(PPID scardID, const SCardSpecialTreatment::IdentifyReplyBlock * pStirb, uint ascf)
{
	//assert(oneof3(fromInput, 0, 1, 100));
	int    ok = 1;
	SString temp_buf;
	CPosProcessor_MsgToDisp_Frame mdf(this);
	SCardSpecialTreatment::IdentifyReplyBlock local_stirb;
	if(Flags & fPrinted && !(OperRightsFlags & orfChgPrintedCheck) && !(ascf & ascfIgnoreRights)) {
		MessageError(PPERR_NORIGHTS, 0, eomBeep | eomStatusLine);
		Flags &= ~fWaitOnSCard;
	}
	else {
		Flags |= fSuspSleepTimeout;
		const  bool prev_no_gift_status = LOGIC(CSt.Flags & CardState::fNoGift);
		//
		// Признак того, что скидка устанавливается только на текущую строку чека (1 скидка по строке, 0 - error, -1 - скидка по карте)
		//
		int    row_dscnt = (ascf & (ascfFromInput|ascfExtPane)) ? AcceptRowDiscount() : -1;
		if(!(ascf & (ascfFromInput|ascfExtPane)))
			CSt.SetID(scardID, 0);
		//
		if(row_dscnt < 0 && !oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
			int    is_found = 0;
			SCardTbl::Rec sc_rec;
			//
			// Если мы в состоянии начисления на кредитную карту, то менять карту уже нельзя //
			//
			if(CSt.GetID()) {
				const  PPID charge_goods_id = GetChargeGoodsID(CSt.GetID());
				if(charge_goods_id != UNDEF_CHARGEGOODSID) {
					for(uint i = 0; ok && i < P.getCount(); i++)
						if(P.at(i).GoodsID == charge_goods_id)
							ok = MessageError(PPERR_UNABLECHNGCHARGESCARD, 0, eomStatusLine);
				}
			}
			if(ok) {
				int    ext_cancel = 0; // Признак отмены расширенного выбора карты
				int    auto_charge = 0; // Признак автоматического выбора товара для начисления (пользователь до этого в диалоге выбора карты нажал соответствующую кнопку).
				TSVector <SCardSpecialTreatment::IdentifyReplyBlock> scp_rb_list;
				const SCardSpecialTreatment::IdentifyReplyBlock * p_stirb = 0;
				if((ascf & ascfExtPane) && PNP.CnExtFlags & CASHFX_EXTSCARDSEL) {
					//
					// Расширенный выбор карты
					//
					PPID   scard_id = CSt.GetID();
					if(!scard_id) {
						GetInput();
						temp_buf = Input;
						if(temp_buf.NotEmptyS() && ScObj.SearchCode(0, temp_buf, &sc_rec) > 0)
							scard_id = sc_rec.ID;
					}
					{
						int cm = 0;//ViewSCardInfo(&scard_id, PNP.NodeID, 1);
						//int FASTCALL ViewSCardInfo(PPID * pSCardID, PPID posNodeID, int asSelector)
						{
							const int preserve_use_large_dialogs_flags = SLS.CheckUiFlag(sluifUseLargeDialogs);
							SLS.SetUiFlag(sluifUseLargeDialogs, 0);
							cm = Helper_ViewSCardInfo(&scard_id, PNP.NodeID, 1/*asSelector*/, &local_stirb);
							SLS.SetUiFlag(sluifUseLargeDialogs, preserve_use_large_dialogs_flags);
						}
						if(cm > 0) {
							if(scard_id && ScObj.Search(scard_id, &sc_rec) > 0) {
								CSt.SetID(scard_id, sc_rec.Code);
								if(local_stirb.ScID && local_stirb.SpecialTreatment)
									p_stirb = &local_stirb;
								if(cm == 2) // Начисление на карту
									auto_charge = 1;
								is_found = 1;
							}
							else {
								Flags |= fWaitOnSCard; // Ниже по этому флагу произойдет сброс выбранной карты.
							}
						}
						else
							ext_cancel = 1;
					}
				}
				else if(ascf & (ascfFromInput|ascfExtPane)) {
					//
					// Выбор карты по коду, введенному в строке ввода
					//
					if(GetInput()) {
						PPIDArray sc_id_list;
						int    err_code = 0;
						if(ScObj.SearchCodeExt(Input, &SpcTrtScsList, sc_id_list, scp_rb_list) > 0) {
							assert(sc_id_list.getCount());
							if(UiFlags & uifAutoInput || !(CsObj.GetEqCfg().Flags & PPEquipConfig::fDisableManualSCardInput)) {
								if(ScObj.Search(sc_id_list.get(0), &sc_rec) > 0) {
									p_stirb = GetSpecialTreatmentBlock(scp_rb_list, sc_rec.ID);
									is_found = 1;
								}
								else
									err_code = PPERR_SCARDNOTFOUND; // Это невозможно! Функция ScObj.SearchCodeExt уже нашла карту по этому идентификатору
							}
							else
								err_code = PPERR_MANUALSCARDINPUTDISABLED;
						}
						else
							err_code = PPERR_SCARDNOTFOUND;
						if(err_code)
							MessageError(err_code, Input, eomStatusLine | eomBeep);
					}
				}
				else if(CSt.GetID() && ScObj.Search(CSt.GetID(), &sc_rec) > 0) {
					//
					// Внешняя установка карты
					//
					p_stirb = pStirb;
					is_found = 1;
				}
				if(!ext_cancel) { // Если расширенный выбор был отменен, то это блок следует пропустить
					if(is_found) {
						const bool only_charge_goods = IsOnlyChargeGoodsInPacket(CSt.GetID(), 0);
						int    cr = ScObj.CheckRestrictions(&sc_rec, (only_charge_goods ? (PPObjSCard::chkrfIgnoreUsageTime|PPObjSCard::chkrfIgnoreUsageDow) : 0), getcurdatetime_());
						if(!cr) {
							MessageError(-1, 0, eomPopup | eomBeep);
							CSt.SetID(0, 0);
						}
						else {
							//
							// Автоактивируемая карта начиная с 9.1.8 на первом чеке ведет себя так же, как и обычная
							//
							{
								Implement_AcceptSCard(sc_rec, p_stirb);
								{
									PPID   auto_goods_id = 0;
									if(auto_charge) {
										const  PPID charge_goods_id = GetChargeGoodsID(CSt.GetID());
										if(charge_goods_id != UNDEF_CHARGEGOODSID)
											auto_goods_id = charge_goods_id;
									}
									else if(sc_rec.AutoGoodsID) {
										auto_goods_id = sc_rec.AutoGoodsID;
									}
									if(auto_goods_id) {
										PgsBlock pgsb(1.0);
										if(PreprocessGoodsSelection(auto_goods_id, 0, pgsb) > 0)
											SetupNewRow(auto_goods_id, pgsb);
									}
								}
								CSt.Discount = fdiv100i(sc_rec.PDis);
								Flags |= fPctDis;
								SetupDiscount(0); // Вызов SetupDiscount перед CalcRestByCrdCard_ необходим для корректного расчета остатка по кредитной (бонусной) карте
								if(CalcRestByCrdCard_(0))
									AutosaveCheck();
								else
									ResetSCard();
								OnUpdateList(0);
							}
							{
								SString msg_buf;
								if(cr == 2) {
									msg_buf.Printf(PPLoadTextS(PPTXT_SCARDISAUTOACTIVATED, temp_buf), sc_rec.Code);
									PPTooltipMessage(msg_buf, 0, H(), 10000, GetColorRef(SClrOrange),
										SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
								}
								else if(CSt.Flags & CSt.fBirthday) {
									if(CSt.OwnerID) {
										PPObjPerson psn_obj;
										PersonTbl::Rec psn_rec;
										if(psn_obj.Search(CSt.OwnerID, &psn_rec) > 0) {
											ObjTagItem tag_item;
											if(PPRef->Ot.GetTag(PPOBJ_PERSON, CSt.OwnerID, PPTAG_PERSON_DOB, &tag_item) > 0) {
												LDATE   dob_dt = ZERODATE;
												if(tag_item.GetDate(&dob_dt)) {
													PPLoadText(PPTXT_CLIBIRTHDAY, temp_buf);
													PPFormat(temp_buf, &msg_buf, psn_rec.Name, (getcurdate_().year() - dob_dt.year()));
													PPTooltipMessage(msg_buf, 0, H(), 10000, GetColorRef(SClrPink),
														SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
												}
											}
										}
									}
								}
							}
						}
						Flags &= ~fWaitOnSCard;
					}
					else {
						if(Flags & fWaitOnSCard) {
							ResetSCard();
							OnUpdateList(0);
						}
						INVERSEFLAG(Flags, fWaitOnSCard);
					}
					SetupInfo(0);
				}
			}
		}
		ClearInput(0);
		if(LOGIC(CSt.Flags & CardState::fNoGift) != prev_no_gift_status)
			ProcessGift();
		Flags &= ~fSuspSleepTimeout;
	}
}

/*virtual*/void CheckPaneDialog::SetupRowData(bool doCalcRest)
{
	const CCheckItem & r_item = P.GetCurC();
	SString temp_buf;
	double sum = R2(r_item.Quantity * (r_item.Price - r_item.Discount));
	SString buf(r_item.GoodsName);
	if(P.CurModifList.getCount()) {
		buf.Space().CatChar('{').CatChar('+');
		for(uint i = 0; i < P.CurModifList.getCount(); i++) {
			GetGoodsName(P.CurModifList.at(i).GoodsID, temp_buf);
			buf.CatDivConditionally(';', 2, i > 0).Cat(temp_buf);
		}
		buf.CatChar('}');
	}
	setCtrlString(CTL_CHKPAN_GOODS, buf);
	buf.Z().Cat(r_item.Quantity);
	if(r_item.PhQtty)
		buf.CatChar('(').Cat(r_item.PhQtty).CatChar(')');
	setCtrlString(CTL_CHKPAN_QTTY, buf);
	setCtrlReal(CTL_CHKPAN_PRICE, r_item.Price);
	setCtrlReal(CTL_CHKPAN_SUM,   sum);
	CPosProcessor::SetupRowData(doCalcRest);
	ViewStoragePlaces(0);
}

int CPosProcessor::GetCurrentOp() const // @v12.2.9 
{
	int   op = CCOP_GENERAL;
	assert(!(F(fCorrection) && F(fRetCheck))); // Если одновременно установлены флаги fCorrection и fRetCheck значит мы где-то ошились в определении состояния //
	if(F(fCorrection))
		op = CCOP_CORRECTION_SELL;
	else if(F(fRetCheck))
		op = CCOP_RETURN;
	else
		op = CCOP_GENERAL;
	return op;
}

bool CPosProcessor::IsCurrentOp(int op) const { return (GetCurrentOp() == op); } // @v12.2.9 
bool CPosProcessor::IsCurrentOpCorrection() const { return CPosProcessor::IsCorrectionOp(GetCurrentOp()); } // @v12.2.11

int CPosProcessor::SetCurrentOp(int op) // @v12.2.9
{
	assert(IsValidOp(op));
	if(IsCorrectionOp(op)) {
		SETFLAG(Flags, fCorrection, true);
		SETFLAG(Flags, fRetCheck, false);
	}
	else if(op == CCOP_RETURN) {
		SETFLAG(Flags, fCorrection, false);
		SETFLAG(Flags, fRetCheck, true);
	}
	else {
		SETFLAG(Flags, fCorrection, false);
		SETFLAG(Flags, fRetCheck, false);
	}
	return 1;
}

int CPosProcessor::Helper_GetPriceRestrictions_ByFormula(SString & rFormula, const CCheckItem & rCi, double & rBound) const
{
	int    ok = -1;
	rBound = 0.0;
	if(rFormula.NotEmptyS()) {
		double bound = 0.0;
		//CALLPTRMEMB(pPack, SetTPointer(itemPos));
		GdsClsCalcExprContext ctx(&rCi);
		if(PPCalcExpression(rFormula, &bound, &ctx) && bound > 0.0) {
			rBound = bound;
			ok = 1;
		}
	}
	return ok;
}

int CPosProcessor::CheckPriceRestrictions(PPID goodsID, const CCheckItem & rCi, double price, RealRange * pRange)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(goodsID, &goods_rec) > 0 && goods_rec.GoodsTypeID) {
		PPObjGoodsType gt_obj;
		PPGoodsType gt_rec;
		if(gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.PriceRestrID) {
			PPObjGoodsValRestr gvr_obj;
			PPGoodsValRestrPacket gvr_pack;
			RealRange range;
			int   pr = 0;
			if(gvr_obj.Fetch(gt_rec.PriceRestrID, &gvr_pack) > 0) {
				if(Helper_GetPriceRestrictions_ByFormula(gvr_pack.LowBoundFormula, rCi, range.low) > 0)
					pr = 1;
				if(Helper_GetPriceRestrictions_ByFormula(gvr_pack.UppBoundFormula, rCi, range.upp) > 0)
					pr = 1;
			}
			if(pr) {
				ok = 1;
				if(!range.CheckValEps(price, 1E-7)) {
					SString temp_buf;
					if(range.low > 0.0 && price < range.low) {
						SString & r_nam_buf = SLS.AcquireRvlStr();
						temp_buf.Z().Cat(range.low, SFMT_MONEY).Space().Cat(GetGoodsName(goodsID, r_nam_buf));
						ok = PPSetError(PPERR_PRICERESTRLOW, temp_buf);
					}
					else if(range.upp > 0.0 && price > range.upp) {
						SString & r_nam_buf = SLS.AcquireRvlStr();
						temp_buf.Z().Cat(range.upp, SFMT_MONEY).Space().Cat(GetGoodsName(goodsID, r_nam_buf));
						ok = PPSetError(PPERR_PRICERESTRUPP, temp_buf);
					}
				}
			}
		}
	}
	return ok;
}

int CPosProcessor::SetupNewRow(PPID goodsID, PgsBlock & rBlk, PPID giftID/*=0*/)
{
	int    ok = 1;
	int    r = 0;
	SString temp_buf;
	if(Flags & fPrinted && !(OperRightsFlags & orfChgPrintedCheck)) {
		ok = MessageError(PPERR_NORIGHTS, 0, eomBeep|eomStatusLine);
	}
	else {
		const bool gift_money = (giftID && rBlk.PriceBySerial > 0.0);
		if(AcceptRow(giftID)) {
			RetailGoodsInfo rgi;
			Goods2Tbl::Rec goods_rec;
			long   ext_rgi_flags = 0;
			int    is_abstract = 0;
			if(gift_money)
				ext_rgi_flags |= PPObjGoods::rgifAllowUnlimWoQuot;
			else if(rBlk.PriceBySerial > 0.0) {
				ext_rgi_flags |= PPObjGoods::rgifUseOuterPrice;
				rgi.OuterPrice = rBlk.PriceBySerial;
			}
			r = GetRgi(goodsID, rBlk.Qtty, ext_rgi_flags, rgi);
			if(goodsID == PNP.AbstractGoodsID && rBlk.AbstractPrice > 0.0) {
                rgi.Price = rBlk.AbstractPrice;
                is_abstract = 1;
                r = 1;
			}
			else if(goodsID == GetChargeGoodsID(CSt.GetID())) {
				//
				// Начисление на кредитную карту
				//
				rgi.ID = goodsID;
				if(GObj.Fetch(goodsID, &goods_rec) > 0) {
					STRNSCPY(rgi.Name, goods_rec.Name);
					GObj.FetchSingleBarcode(goodsID, temp_buf);
					temp_buf.CopyTo(rgi.BarCode, sizeof(rgi.BarCode));
				}
				rBlk.Qtty = 0.0;
				if(r <= 0)
					rgi.Price = 1.0;
				r = 2000;
			}
			if(r > 0 || (r == -2 && (PNP.CnFlags & CASHF_USEQUOT || gift_money))) {
				double price = 0.0;
				bool   use_ext_price = false;
				if(rgi.QuotKindUsedForExtPrice && rgi.ExtPrice >= 0.0 && !(rgi.Flags & rgi.fDisabledExtQuot))
					use_ext_price = true;
				else if(rgi.Flags & rgi.fDisabledQuot) {
					GetGoodsName(goodsID, temp_buf);
					ok = MessageError(PPERR_DISABLEDGOODSQUOT, temp_buf, eomStatusLine|eomBeep);
				}
				if(ok) {
					int    serial_price_tag = 0;
					int    look_back_price = 0;
					if(!giftID) {
						if(rBlk.PriceBySerial > 0.0) {
							if(rBlk.Serial.IsEmpty())
								look_back_price = 1;
							else
								serial_price_tag = 1;
						}
						price = use_ext_price ? rgi.ExtPrice : rgi.Price;
					}
					if(price > 0.0 || (price == 0.0 && rgi.QuotKindUsedForPrice) || giftID || r == 2000) {
						CCheckItem & r_item = P.GetCur();
						r_item.GoodsID = rgi.ID;
						STRNSCPY(r_item.GoodsName, rgi.Name);
						STRNSCPY(r_item.BarCode,   rgi.BarCode);
						r_item.Quantity = R6((IsCurrentOp(CCOP_RETURN)) ? -fabs(rBlk.Qtty) : fabs(rBlk.Qtty));
						r_item.Price    = price;
						r_item.Discount = 0.0;
						if(is_abstract || look_back_price)
							r_item.Flags |= cifFixedPrice;
						SETFLAG(r_item.Flags, cifPriceBySerial, serial_price_tag);
						STRNSCPY(r_item.Serial, rBlk.Serial);
						STRNSCPY(r_item.EgaisMark, rBlk.EgaisMark);
						STRNSCPY(r_item.ChZnSerial, rBlk.ChZnSerial);
						STRNSCPY(r_item.ChZnMark, rBlk.ChZnMark);
						r_item.ChZnPm_ReqId = rBlk.ChZnPm_ReqId; // @v12.1.1
						r_item.ChZnPm_ReqTimestamp = rBlk.ChZnPm_ReqTimestamp; // @v12.1.1
						r_item.ChZnPm_LocalModuleInstance = rBlk.ChZnPm_LocalModuleInstance; // @v12.3.12
						r_item.ChZnPm_LocalModuleDbVer = rBlk.ChZnPm_LocalModuleDbVer; // @v12.3.12
						if(giftID) {
							r_item.Flags |= cifGift;
							r_item.GiftID = giftID;
							if(gift_money) {
								r_item.Price = 0.0;
								r_item.Discount = fabs(rBlk.PriceBySerial);
								r_item.Flags |= cifGiftDiscount;
							}
						}
						// @v12.2.2 {
						if(!rBlk.AllowedPriceRange.CheckValEps(r_item.Price, 1E-7)) {
							const RealRange & r_range = rBlk.AllowedPriceRange;
							if(r_range.low > 0.0 && r_item.Price < r_range.low) {
								temp_buf.Z().Cat(r_range.low, SFMT_MONEY).Space().Cat(GetGoodsName(goodsID, SLS.AcquireRvlStr()));
								ok = MessageError(PPERR_PRICERESTRLOW, temp_buf, eomStatusLine|eomBeep);
							}
							else if(r_range.upp > 0.0 && r_item.Price > r_range.upp) {
								temp_buf.Z().Cat(r_range.upp, SFMT_MONEY).Space().Cat(GetGoodsName(goodsID, SLS.AcquireRvlStr()));
								ok = MessageError(PPERR_PRICERESTRUPP, temp_buf, eomStatusLine|eomBeep);
							}
						} // } @v12.2.2 
						else if(!CheckPriceRestrictions(goodsID, r_item, r_item.Price, 0)) {
							ok = MessageError(-1, 0, eomStatusLine|eomBeep);
						}
						if(ok) {
							P.CurCcItemPos = P.getCount();
							if(CalcRestByCrdCard_(1)) {
								SetupRowData(true/*doCalcRest*/);
							}
							else {
								ClearRow();
								ok = 0;
							}
						}
					}
					else {
						PPSetError(PPERR_INVGOODSPRICE, GetGoodsName(goodsID, temp_buf));
						ok = MessageError(PPERR_INVGOODSPRICE, temp_buf, eomStatusLine|eomBeep);
					}
				}
			}
			else if(!r)
				ok = MessageError(PPErrCode, 0, eomStatusLine|eomBeep);
		}
		else
			ok = 0;
	}
	return ok;
}

void CheckPaneDialog::ClearInput(int selectOnly)
{
	if(selectOnly) {
		TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_CHKPAN_INPUT));
		CALLPTRMEMB(p_il, selectAll(true));
	}
	else {
		setCtrlString(CTL_CHKPAN_INPUT, Input.Z());
		selectCtrl(CTL_CHKPAN_INPUT);
	}
}

void CheckPaneDialog::SetSCard(const char * pStr)
{
	Flags |= fWaitOnSCard;
	SString temp_buf(pStr);
	setCtrlString(CTL_CHKPAN_INPUT, temp_buf);
	UiFlags |= uifAutoInput; // @trick AcceptSCard(1, ) может отклонить номер карты из-за отсутствия данного флага.
	AcceptSCard(0, 0, ascfFromInput);
	UiFlags &= ~uifAutoInput; // @trick (see above)
}

bool CheckPaneDialog::GetInput()
{
	UiFlags &= ~uifAutoInput;
	getCtrlString(CTL_CHKPAN_INPUT, Input);
	TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_CHKPAN_INPUT));
	if(p_il) {
		TInputLine::Statistics stat;
		p_il->GetStatistics(&stat);
		if(stat.Flags & stat.fSerialized && !(stat.Flags & stat.fPaste)) {
			if(stat.SymbCount && stat.IntervalMean <= static_cast<double>(AutoInputTolerance))
				UiFlags |= uifAutoInput;
		}
	}
	return Input.NotEmptyS();
}

void CheckPaneDialog::SetInput(const char * pStr)
{
	Input = pStr;
	setCtrlString(CTL_CHKPAN_INPUT, Input);
	ProcessEnter(1);
}

/*virtual*/void CheckPaneDialog::NotifyGift(PPID giftID, const SaGiftArray::Gift * pGift)
{
	SString msg_buf;
	SString goods_name;
	if(giftID > 0) {
		Flags |= fPresent;
		PPLoadString("gift", msg_buf);
		GetGoodsName(giftID, goods_name);
		setCtrlString(CTL_CHKPAN_INFO, msg_buf.Space().Cat(goods_name.Quot(39, 39)));
		if(!(Flags & fDisableBeep))
			Beep(500, 200);
		Flags &= ~fPresent;
	}
	else if(pGift) {
		SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления о возможных подарках
		if(pGift->Pot.Name.NotEmpty()) {
			SMessageWindow * p_win = new SMessageWindow;
			if(p_win) {
				SString fmt_buf;
				GetGoodsName(pGift->Pot.GoodsID, goods_name);
				// PPTXT_CHKPAN_GIFTNOT    "Для получения подарка '%s'\nосталось купить товар '%s'";
				// PPTXT_CHKPAN_GIFTNOTAMT "Для получения подарка '%s'\nосталось купить товар '%s' на сумму %.2lf";
				PPLoadText((pGift->Pot.Amount > 0.0) ? PPTXT_CHKPAN_GIFTNOTEAMT : PPTXT_CHKPAN_GIFTNOTE, fmt_buf);
				msg_buf.Printf(fmt_buf, pGift->Pot.Name.cptr(), goods_name.cptr(), pGift->Pot.Deficit);
				p_win->Open(msg_buf, 0, H(), 0, 30000, GetColorRef(SClrAquamarine),
					SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus, 0);
			}
		}
	}
}

int CPosProcessor::AddGiftSaleItem(TSVector <SaSaleItem> & rList, const CCheckItem & rItem) const
{
	int    ok = 1;
	SaSaleItem sa_item;
	sa_item.GoodsID = rItem.GoodsID;
	sa_item.Qtty = fabs(rItem.Quantity);
	sa_item.Price = fabs(rItem.Price);
	uint pos = 0;
	if(rList.lsearch(&rItem.GoodsID, &pos, CMPF_LONG)) {
		SaSaleItem & r_sa_item = rList.at(pos);
		double qtty = r_sa_item.Qtty + fabs(rItem.Quantity);
		r_sa_item.Price = ((r_sa_item.Price * r_sa_item.Qtty) + (sa_item.Price * sa_item.Qtty)) / qtty;
		r_sa_item.Qtty = qtty;
	}
	else
		rList.insert(&sa_item);
	return ok;
}

int CPosProcessor::ProcessGift()
{
	int    ok = -1;
	uint   i;
	PPIDArray last_item_by_giftq_goods_list; // Список товаров, по которым уже предоставлена
		// подарочная скидка по опции GSGIFTQ_LASTITEMBYGIFTQ
	if(PNP.CnFlags & CASHF_CHECKFORPRESENT) {
		if(CSt.GetID() && CSt.Flags & CardState::fNoGift) {
			if(P.ClearGift() > 0)
				OnUpdateList(0);
		}
		else {
			PPID   last_gift_id = 0;
			int    is_gift = 0;
			SaGiftArray::Gift gift;
			SString buf, goods_name;
			TSVector <SaSaleItem> sale_list, full_sale_list;
			LAssocArray ex_gift_list;    // Список подарочных товаров, уже находящихся в чеке
			RAssocArray ex_gift_id_list; // Список подарков, уже находящихся в чеке
			LAssocArray preserve_gift_assoc = P.GiftAssoc;
			TSCollection <SaGiftArray::Gift> _gift_list; // Список обработанных схем подарков - необходим для избежания зацикливания
			P.GiftAssoc.clear();
			for(i = 0; i < P.getCount(); i++) {
				CCheckItem & r_item = P.at(i);
				if(r_item.Flags & cifGift) {
					ex_gift_list.Add(r_item.GoodsID, i, 0);
					ex_gift_id_list.Add(r_item.GiftID, fabs(r_item.Quantity), 0);
				}
				else {
					r_item.Flags &= ~(cifUsedByGift|cifMainGiftItem);
					r_item.ResetGiftQuot();
					AddGiftSaleItem(full_sale_list, r_item);
				}
			}
			sale_list = full_sale_list;
			if(sale_list.getCount()) {
				int    overlap = 0; // Признак обработки перекрывающих подарков
				PPObjGoodsStruc gs_obj;
				SaGiftArray sa_gift_list;
				THROW(gs_obj.FetchGiftList(&sa_gift_list));
				while(1) {
					{
						//
						// Блок управления циклом
						//
						int  do_process = 0;
						if(!overlap) {
							if(sa_gift_list.SelectGift(sale_list, ex_gift_id_list, 0 /* overlap */, gift) > 0)
								do_process = 1;
							else {
								overlap = 1;
								SaGiftArray::Potential ppot;
								gift.PreservePotential(ppot);
								if(sa_gift_list.SelectGift(full_sale_list, ex_gift_id_list, 1 /* overlap */, gift) > 0)
									do_process = 1;
								if(gift.Pot.Name.IsEmpty())
									gift.RestorePotential(ppot);
							}
						}
						for(uint gi = 0; do_process && gi < _gift_list.getCount(); gi++) {
							const SaGiftArray::Gift * p_gift = _gift_list.at(gi);
							if(p_gift && p_gift->IsEqualForResult(gift))
								do_process = 0;
						}
						if(do_process) {
							SaGiftArray::Gift * p_new_item = new SaGiftArray::Gift(gift);
							_gift_list.insert(p_new_item);
						}
						else
							break;
					}
					PPID   gift_id = 0;
					gift.Qtty = fint(gift.Qtty);
					if(gift.Qtty > 0.0) {
						int    skip = 0;
						int    manual_gift = 0;
						gift_id = gift.List.get(0);
						if(gift_id) {
							is_gift = 1;
							Flags |= fPresent;
							if(gift.QuotKindID) {
								const  uint pcnt = P.getCount();
								int    is_there_gift_quot = 0;
								if(gift.QuotKindID == GSGIFTQ_CHEAPESTITEMFREE) {
									if(pcnt > 0) {
										double min_price = SMathConst::Max;
										uint   min_pos = UINT_MAX;
										for(i = 0; i < pcnt; i++) {
											CCheckItem & r_item = P.at(i);
											if(!(r_item.Flags & cifGift) && gift.CheckList.Has(r_item.GoodsID)) {
												if(r_item.NetPrice() <= min_price) {
													min_price = r_item.NetPrice();
													min_pos = i;
												}
												r_item.Flags |= cifUsedByGift;
											}
										}
										if(min_pos < pcnt) {
											assert(min_price < SMathConst::Max);
											CCheckItem & r_item = P.at(min_pos);
											const double qtty = fabs(r_item.Quantity);
											const double net_price = r_item.NetPrice();
											double gift_price = round((net_price * qtty - net_price) / qtty, 2);
											is_there_gift_quot = r_item.SetupGiftQuot(gift_price, 1);
										}
									}
								}
								if(gift.QuotKindID == GSGIFTQ_CHEAPESTITEMBYGIFTQ) {
									if(pcnt > 0) {
										LongArray _pos_list;
										for(double _rest = gift.Qtty; _rest > 0.0;) {
											double min_price = SMathConst::Max;
											uint   min_pos = UINT_MAX;
											for(i = 0; i < pcnt; i++) {
												CCheckItem & r_item = P.at(i);
												if(!(r_item.Flags & cifGift) && !_pos_list.lsearch(i) && gift.CheckList.Has(r_item.GoodsID)) {
													if(r_item.NetPrice() <= min_price) {
														min_price = r_item.NetPrice();
														min_pos = i;
													}
													r_item.Flags |= cifUsedByGift;
												}
											}
											if(min_pos < pcnt) {
												assert(min_price < SMathConst::Max);
												_pos_list.add(min_pos);
												CCheckItem & r_item = P.at(min_pos);
												const double qtty = fabs(r_item.Quantity);
												const double gift_qtty = MIN(_rest, fint(qtty));
												const double net_price = r_item.NetPrice();
												double qprice = 0.0;
												QuotIdent qi(QIDATE(getcurdate_()), PNP.CnLocID, PPQUOTK_GIFT);
												if(GObj.GetQuotExt(r_item.GoodsID, qi, 0.0, r_item.Price, &qprice, 1) > 0) {
													if(qprice < net_price) {
														double gift_price = round((net_price * qtty - (gift_qtty * (net_price - qprice))) / qtty, 2);
														is_there_gift_quot = r_item.SetupGiftQuot(gift_price, 1);
														_rest -= gift_qtty;
													}
												}
											}
											else
												break;
										}
									}
								}
								else if(gift.QuotKindID == GSGIFTQ_LASTITEMBYGIFTQ) {
									const uint clc = gift.CheckList.getCount();
									const  PPID last_goods_id = clc ? gift.CheckList.at(clc-1).Key : 0;
									if(last_goods_id && !last_item_by_giftq_goods_list.lsearch(last_goods_id)) {
										int    single_pos = -1;
										int    max_qtty_pos = -1;
										double max_qtty = 0.0;
										i = pcnt;
										if(i) do {
											CCheckItem & r_item = P.at(--i);
											if(!(r_item.Flags & (cifGift|cifUsedByGift)) && r_item.GoodsID == last_goods_id) {
												if(r_item.Quantity == 1.0) {
													// Если встретили 1 штуку - на нее подарочную котировку и повесим - выходим.
													single_pos = i;
												}
												else if(r_item.Quantity > 1.0 && r_item.Quantity > max_qtty) {
													// Если количество больше 1, то позиция - потенциально может быть разбита на две
													// с целью получения единичной строки, на которую можно повесить подарочную котировку.
													max_qtty_pos = i;
													max_qtty = r_item.Quantity;
												}
											}
										} while(i && single_pos < 0);
										if(single_pos < 0 && max_qtty_pos >= 0) {
											//
											// Единичную позицию не нашли - придется разбивать строку с максимальным количеством.
											//
											CCheckItem & r_item = P.at(max_qtty_pos);
											CCheckItem new_item;
											if(r_item.SplitByQtty(1.0, new_item) > 0) {
												single_pos = static_cast<int>(P.getCount());
												new_item.Flags |= cifUsedByGift;
												P.insert(&new_item);
											}
										}
										if(single_pos >= 0) {
											CCheckItem & r_item = P.at(single_pos);
											double gift_price = 0.0;
											const QuotIdent qi(QIDATE(getcurdate_()), PNP.CnLocID, PPQUOTK_GIFT);
											if(GObj.GetQuotExt(r_item.GoodsID, qi, 0.0, r_item.Price, &gift_price, 1) > 0)
												is_there_gift_quot = r_item.SetupGiftQuot(gift_price, 1);
											for(i = 0; i < pcnt; i++) {
												CCheckItem & r_item = P.at(i);
												if(!(r_item.Flags & cifGift) && gift.CheckList.Has(r_item.GoodsID))
													r_item.Flags |= cifUsedByGift;
											}
											last_item_by_giftq_goods_list.addUnique(last_goods_id);
										}
									}
								}
								else {
									const  uint clc = gift.CheckList.getCount();
									double total_gift_qtty = gift.Qtty;
									for(uint m = 0; m < clc && total_gift_qtty > 0.0; m++) {
										const RAssoc & r_check_item = gift.CheckList.at(m);
										//double total_gift_qtty = r_check_item.Val * gift.Qtty;
										for(i = 0; i < pcnt && total_gift_qtty > 0.0; i++) {
											CCheckItem & r_item = P.at(i);
											if(!(r_item.Flags & cifGift) && r_item.GoodsID == r_check_item.Key) {
												double qprice = 0.0;
												QuotIdent qi(QIDATE(getcurdate_()), PNP.CnLocID, gift.QuotKindID);
												if(GObj.GetQuotExt(r_item.GoodsID, qi, 0.0, r_item.Price, &qprice, 1) > 0) {
													const double qtty = fabs(r_item.Quantity);
													const double gift_qtty = MIN(total_gift_qtty, qtty);
													const double net_price = r_item.NetPrice();
													double gift_price = round((net_price * qtty - (gift_qtty * (net_price - qprice))) / qtty, 2);
													is_there_gift_quot = r_item.SetupGiftQuot(gift_price, 1);
													total_gift_qtty -= gift_qtty;
												}
												r_item.Flags |= cifUsedByGift;
											}
										}
									}
								}
								if(is_there_gift_quot)
									last_gift_id = gift_id;
							}
							else {
								if(GObj.IsGeneric(gift_id)) {
									PPIDArray gen_list;
									GObj.GetGenericList(gift_id, &gen_list);
									i = gen_list.getCount();
									if(i)
										do {
											const  PPID goods_id = gen_list.get(--i);
											Goods2Tbl::Rec goods_rec;
											if(GObj.Fetch(goods_id, &goods_rec) <= 0 || goods_rec.Flags & GF_GENERIC)
												gen_list.atFree(i);
										} while(i);
									if(gen_list.getCount()) {
										//
										// Далее следует сложный цикл, необходимый для определения факта предшествующего ручного выбора
										// этого же подарка. Если мы нашли такой подарок, то не будем просить пользователя выбрать его снова.
										//
										for(i = 0; !manual_gift && i < ex_gift_list.getCount(); i++) {
											uint ex_pos = static_cast<uint>(ex_gift_list.at(i).Val);
											CCheckItem & r_item = P.at(static_cast<uint>(ex_gift_list.at(i).Val));
											assert(r_item.Flags & cifGift);
											if(r_item.Flags & cifManualGift && gen_list.lsearch(r_item.GoodsID)) {
												LongArray ex_pos_list;
												preserve_gift_assoc.GetListByKey(ex_pos, ex_pos_list);
												if(ex_pos_list.getCount()) {
													int    is_same = 1;
													for(uint j = 0; is_same && j < ex_pos_list.getCount(); j++) {
														uint _p = ex_pos_list.get(j);
														if(_p < P.getCount()) {
															CCheckItem & r_item2 = P.at(_p);
															uint   gift_pos = 0;
															if((r_item2.Flags & cifGift) || !gift.CheckList.Search(r_item2.GoodsID, 0, &gift_pos))
																is_same = 0;
														}
													}
													if(is_same) {
														gift_id = r_item.GoodsID;
														manual_gift = 1;
													}
												}
											}
										}
										//
										if(!manual_gift) {
											long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
											if(PNP.CnFlags & CASHF_SELALLGOODS)
												egsd_flags |= ExtGoodsSelDialog::fForceExhausted;
											ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(GetCashOp(), 0, egsd_flags);
											THROW(CheckDialogPtrErr(&dlg));
											dlg->setSelectionByGoodsList(&gen_list);
											if(ExecView(dlg) == cmOK) {
												TIDlgInitData tidi;
												dlg->getDTS(&tidi);
												if(tidi.GoodsID) {
													gift_id = tidi.GoodsID;
													manual_gift = 1;
												}
												else
													skip = 1;
											}
											else
												skip = 1;
											delete dlg;
										}
									}
								}
								if(!skip) {
									int    gift_accepted = 0;
									double gift_discount = 0.0;
									long   lpos = 0; // Индекс подарочной позиции в контейнере P
									uint   egl_pos = 0;
									if(ex_gift_list.Search(gift_id, &lpos, &egl_pos)) {
										CCheckItem & r_item = P.at(static_cast<uint>(lpos));
										assert(r_item.Flags & cifGift);
										r_item.Quantity = gift.Qtty;
										ex_gift_list.atFree(egl_pos);
										gift_accepted = 1;
									}
									else {
										double gift_quot = 0.0;
										const  QuotIdent qi(QIDATE(getcurdate_()), PNP.CnLocID, PPQUOTK_GIFT);
										if(GObj.GetQuotExt(gift_id, qi, &gift_quot, 1) > 0)
											gift_discount = -gift_quot;
										PgsBlock pgsb(gift.Qtty);
										pgsb.PriceBySerial = gift_discount;
										if(SetupNewRow(gift_id, /*gift.Qtty, gift_discount, 0*/pgsb, 1 /*gift*/)) {
											if(AcceptRow(1 /*gift*/)) {
												assert(P.getCount());
												lpos = P.getCount()-1;
												if(manual_gift)
													P.at(lpos).Flags |= cifManualGift;
												last_gift_id = gift_id;
												gift_accepted = 1;
											}
										}
									}
									if(gift_accepted) {
										assert(lpos < static_cast<long>(P.getCount()));
										for(i = 0; i < P.getCount(); i++) {
											CCheckItem & r_item = P.at(i);
											uint   gift_pos = 0;
											if(!(r_item.Flags & (cifGift|cifUsedByGift)) && gift.CheckList.Search(r_item.GoodsID, 0, &gift_pos)) {
												r_item.Flags |= cifUsedByGift;
												if(gift.MainPosList.lsearch(static_cast<long>(gift_pos)))
													r_item.Flags |= cifMainGiftItem;
												P.GiftAssoc.Add(lpos, static_cast<long>(i), 0);
											}
										}
									}
								}
							}
							if(!skip)
								ok = 1;
							Flags &= ~fPresent;
							//
							// Переходим к следующей итерации, дабы учесть несколько подарков
							//
							sale_list.clear();
							for(i = 0; i < P.getCount(); i++)
								if(!(P.at(i).Flags & (cifGift | cifUsedByGift)))
									AddGiftSaleItem(sale_list, P.at(i));
						}
						else
							break;
					}
					else
						break;
				}
			}
			if(ex_gift_list.getCount()) {
				//
				// Удалять надо начиная с самой нижней позиции - вверх.
				//
				LongArray pos_list;
				for(i = 0; i < ex_gift_list.getCount(); i++)
					pos_list.add(ex_gift_list.at(i).Val);
				pos_list.sort();
				i = pos_list.getCount();
				if(i) do {
					const uint pos = pos_list.get(--i);
					assert(P.at(pos).Flags & cifGift);
					P.atFree(static_cast<uint>(pos));
				} while(i);
			}
			OnUpdateList(0);
			NotifyGift(0, is_gift ? 0 : &gift);
			if(last_gift_id)
				NotifyGift(last_gift_id, 0);
		}
	}
	CATCHZOK
	return ok;
}

int CPosProcessor::AcceptRow(PPID giftID)
{
	// GSIF_SUBPARTSTR
	int    ok = 1;
	CPosProcessor_MsgToDisp_Frame mdf(this);
	Flags |= fSuspSleepTimeout;
	if(P.CurCcItemPos == P.getCountI()) {
		RAssocArray sl_ary(SelLines);
		if(Flags & fForceDivision && P.GetCur().Division == 0) {
			ok = MessageError(PPERR_POSDIVISIONNEEDED, 0, eomBeep|eomStatusLine);
		}
		else if(oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF)) {
			double   qtty = fabs(P.GetCur().Quantity);
			CCheckLineTbl::Rec line;
			for(uint i = 0; qtty > 0.0 && SelPack.EnumLines(&i, &line);) {
				if(line.GoodsID == P.GetCur().GoodsID) {
					double sel_qtty = 0.0;
					double rest_qtty = line.Quantity;
					if(sl_ary.Search(i, &sel_qtty, 0))
						rest_qtty -= sel_qtty;
					if(rest_qtty) {
						sl_ary.Add(i, MIN(qtty, rest_qtty));
						qtty -= rest_qtty;
					}
				}
			}
			if(qtty > 0.0)
				ok = (PPError(PPERR_TIMAXQTTY), 0);
		}
		if(ok) {
			ok = CalcRestByCrdCard_(1); // @01
			if(ok) {
				const CCheckItem & r_cur = P.GetCurC();
				P.insert(&r_cur);
				SString temp_buf;
				for(uint i = 0; i < P.CurModifList.getCount(); i++) {
					const SaModifEntry & r_entry = P.CurModifList.at(i);
					CCheckItem item;
					item.GoodsID = r_entry.GoodsID;
					item.Quantity = (r_entry.Qtty != 0.0) ? fabs(r_entry.Qtty * r_cur.Quantity) : 1.0;
					item.Price = r_entry.Price;
					item.Division = r_cur.Division;
					item.Queue = r_cur.Queue;
					item.Flags |= cifModifier;
					GetGoodsName(item.GoodsID, temp_buf);
					temp_buf.CopyTo(item.GoodsName, sizeof(item.GoodsName));
					P.insert(&item);
				}
				if(!SuspCheckID) {
					SETIFZ(P.Eccd.InitDtm, getcurdatetime_());
					SETIFZ(P.Eccd.InitUserID, LConfig.UserID);
				}
				if(!oneof2(GetState(), sLISTSEL_EMPTYBUF, sLISTSEL_BUF))
					SetupDiscount(0);
				CalcRestByCrdCard_(0); // Предыдущий вызов (@01) не учел скидку при расчете суммы доплаты
				AutosaveCheck();
				OnUpdateList(1);
				SelLines = sl_ary;
			}
		}
	}
	// Это условие никогда не выполняется //
	else if(P.IsCurValid()) {
		assert(0);
		P.at(P.CurCcItemPos) = P.GetCurC();
		SetupDiscount(0);
		OnUpdateList(0);
	}
	if(ok) {
		CDispCommand(giftID ? cdispcmdCurrentGiftItem : cdispcmdCurrentItem, 0, 0.0, 0.0);
		ClearRow();
		if(!IsCurrentOp(CCOP_RETURN) && !IsCurrentOpCorrection() && !giftID) {
			if(ProcessGift() > 0) {
				CalcRestByCrdCard_(0); // Предыдущий вызов (@01) не учел подарочную скидку
			}
		}
	}
	Flags &= ~fSuspSleepTimeout;
	return ok;
}

/*virtual*/void CheckPaneDialog::ClearRow()
{
	CPosProcessor::ClearRow();
	ViewStoragePlaces(0);
}

/*virtual*/void CheckPaneDialog::SetPrintedFlag(int set)
{
	CPosProcessor::SetPrintedFlag(set);
	setButtonBitmap(cmChkPanPrint, (Flags & fPrinted) ? IDB_PRINTDEFMARK : IDB_PRINTDEF);
}

/*virtual*/void CheckPaneDialog::ClearCheck()
{
	CPosProcessor::ClearCheck();
	ClearRow();
	SetupRetCheck(false);
}

int CheckPaneDialog::TestCheck(CheckPaymMethod paymMethod)
{
	int    ok = 1;
	Packet preserve_packet = P;
	if(PNP.NodeID) {
		int    r = 1;
		int    sync_prn_err = 0;
		int    r_ext = 1;
		int    ext_sync_prn_err = 0;
		int    is_pack = 0;
		int    is_ext_pack = 0;
		CCheckTbl::Rec last_chk_rec;
		CCheckPacket pack, ext_pack;
		THROW(InitCashMachine());
		if(paymMethod == cpmBank) {
			pack.Rec.Flags |= CCHKF_BANKING;
		}
		if(SuspCheckID && GetCc().Search(SuspCheckID, &last_chk_rec) > 0) {
			pack.Rec.Code = last_chk_rec.Code;
			//
			// Следующие 2 строки следует раскомментировать, если дата и время отложенного чека должны
			// соответствовать последнему изменению (в противном случае они будут соответствовать времени создания чека).
			//
			//pack.Rec.Dt = last_chk_rec.Dt;
			//pack.Rec.Tm = last_chk_rec.Tm;
		}
		else
			GetNewCheckCode(PNP.NodeID, &pack.Rec.Code);
		pack.Rec.SessID = P_CM->GetCurSessID();
		pack.Rec.PosNodeID = PNP.NodeID;
		pack.Rec.Flags |= (CCHKF_SYNC | CCHKF_NOTUSED);
		SETFLAG(pack.Rec.Flags, CCHKF_INCORPCRD, CSt.GetID() && Flags & fSCardCredit);
		pack.Rec.Flags &= ~CCHKF_BONUSCARD;
		SETFLAG(pack.Rec.Flags, CCHKF_SUSPENDED | CCHKF_SKIP, 0);
		SETFLAG(pack.Rec.Flags, CCHKF_PREPRINT, Flags & fPrinted);
		{
			//
			// Перед окончательным проведением чека необходимо распределить подарочную скидку (если она есть) по строкам чека.
			//
			CCheckItem * p_item;
			for(uint i = 0; P.enumItems(&i, (void **)&p_item);) {
				if(p_item->Flags & cifGiftDiscount) {
					SetupDiscount(1);
					break;
				}
			}
		}
		THROW(Helper_InitCcPacket(&pack, &ext_pack, 0, 0));
		is_pack = BIN(pack.GetCount());
		if(P_CM_EXT) {
			pack._Cash = MONEYTOLDBL(pack.Rec.Amount);
			is_ext_pack = BIN(ext_pack.GetCount());
			if(is_ext_pack) {
				double amt, dscnt;
				GetNewCheckCode(ExtCashNodeID, &ext_pack.Rec.Code);
				ext_pack.Rec.SessID = P_CM_EXT->GetCurSessID();
				ext_pack.Rec.PosNodeID = ExtCashNodeID;
				ext_pack.Rec.Flags  = pack.Rec.Flags;
				ext_pack.SetupAmount(&amt, &dscnt);
				P.SetupCCheckPacket(&ext_pack, CSt, true);
				ext_pack._Cash = amt;
			}
		}
		else {
			pack._Cash = 0.0;
		}
		if(is_pack) {
			pack.Rec.SessID = P_CM->GetCurSessID();
			THROW(P_CM->TestPrintCheck(&pack));
		}
		if(is_ext_pack && P_CM_EXT) {
			ext_pack.Rec.SessID = P_CM_EXT->GetCurSessID();
			THROW(P_CM_EXT->TestPrintCheck(&ext_pack));
		}
	}
	CATCHZOKPPERR
	P = preserve_packet;
	return ok;
}

/*virtual*/int CheckPaneDialog::Implement_AcceptCheckOnEquipment(const CcAmountList * pPl, AcceptCheckProcessBlock & rB)
{
	int    ok = 1;
	CCheckCore & r_cc = GetCc();
	if(rB.Flags & rB.fAltReg && P_CM_ALT) {
		if(rB.Flags & rB.fIsPack) {
			rB.Pack.Rec.SessID = P_CM->GetCurSessID();
			rB.R = P_CM_ALT->SyncPrintCheck(&rB.Pack, 1);
			if(rB.R == 0)
				rB.SyncPrnErr = P_CM_ALT->SyncGetPrintErrCode();
			THROW(rB.R > 0 || rB.SyncPrnErr == 1);
			rB.Pack.Rec.Flags |= (CCHKF_PRINTED|CCHKF_ALTREG);
			r_cc.WriteCCheckLogFile(&rB.Pack, 0, CCheckCore::logPrinted, 1);
		}
	}
	else {
		if(pPl && pPl->Get(CCAMTTYP_CASH) != 0.0) {
			if(P_CM->GetNodeData().Flags & CASHF_OPENBOX)
				P_CM->SyncOpenBox();
		}
		if(rB.Flags & rB.fIsPack) {
			THROW(rB.R = P_CM->SyncCheckForSessionOver());
			if(rB.R > 0) {
				rB.Pack.Rec.SessID = P_CM->GetCurSessID();
				rB.R = P_CM->SyncPrintCheck(&rB.Pack, 1);
				if(rB.R == 0)
					rB.SyncPrnErr = P_CM->SyncGetPrintErrCode();
			}
		}
		THROW(rB.R > 0 || rB.SyncPrnErr == 1);
		if(rB.Flags & rB.fIsPack) {
			rB.Pack.Rec.Flags |= CCHKF_PRINTED;
			r_cc.WriteCCheckLogFile(&rB.Pack, 0, CCheckCore::logPrinted, 1);
		}
		if(rB.Flags & rB.fIsExtPack) {
			THROW(rB.RExt = P_CM_EXT->SyncCheckForSessionOver());
			if(rB.RExt > 0) {
				rB.ExtPack.Rec.SessID = P_CM_EXT->GetCurSessID();
				rB.RExt = P_CM_EXT->SyncPrintCheck(&rB.ExtPack, 1);
				if(rB.RExt == 0)
					rB.ExtSyncPrnErr = P_CM_EXT->SyncGetPrintErrCode();
			}
			THROW(rB.RExt > 0 || rB.ExtSyncPrnErr == 1);
			rB.ExtPack.Rec.Flags |= CCHKF_PRINTED;
			r_cc.WriteCCheckLogFile(&rB.ExtPack, 0, CCheckCore::logPrinted, 1);
		}
	}
	CATCHZOK
	return ok;
}

int CheckPaneDialog::GetLastCheckPacket(PPID nodeID, PPID sessID, CCheckPacket * pPack)
{
	int    ok = -1;
	CCheckTbl::Rec chk_rec;
	if(pPack && GetCc().GetLastCheck(sessID, nodeID, &chk_rec) > 0 && GetCc().LoadPacket(chk_rec.ID, 0, pPack) > 0) {
		if(/*CnFlags & CASHF_UNIFYGDSATCHECK*/false) { // @v12.2.12 Блокируем возможность объединения строк чека из-за возможных проблем с макрировкой строк
			pPack->MergeLines(0);
		}
		ok = 1;
	}
	return ok;
}

int CheckPaneDialog::PrintCheckCopy()
{
	int     ok = -1;
	//PPID    chk_id = 0;
	SString title_buf;
	SelectCheckResult sccr;
	sccr.Format = "CCheckCopy";
	THROW_PP(OperRightsFlags & orfCopyCheck, PPERR_NORIGHTS);
	PPLoadString("selectccheck_forcopy", title_buf);
	if(IsState(sEMPTYLIST_EMPTYBUF) && SelectCheck(title_buf, scfSelFormat|scfThisNodeOnly|scfAllowReturns, sccr) > 0) {
		CCheckPacket   pack, ext_pack;
		THROW(InitCashMachine());
		if(sccr.CcID) {
			int  r = -1;
			THROW(r = GetCc().LoadPacket(sccr.CcID, 0, &pack));
			if(r > 0) {
				PPCashNode     cn_rec;
				if(P_CM_EXT && pack.Rec.PosNodeID && CnObj.Fetch(pack.Rec.PosNodeID, &cn_rec) > 0 && cn_rec.LocID == PNP.ExtCnLocID) {
					THROW(P_CM_EXT->SyncPrintCheckCopy(&pack, sccr.Format));
				}
				else {
					THROW(P_CM->SyncPrintCheckCopy(&pack, sccr.Format));
				}
				ok = 1;
			}
		}
		else {
			int r1 = GetLastCheckPacket(P_CM->GetNodeData().ID, P_CM->GetCurSessID(), &pack);
			int r2 = P_CM_EXT ? GetLastCheckPacket(P_CM_EXT->GetNodeData().ID, P_CM_EXT->GetCurSessID(), &ext_pack) : 1;
			if(r1 > 0 && r2 > 0) {
				LDATETIME  pack_dttm;
				LDATETIME  ext_pack_dttm;
				pack_dttm.Set(pack.Rec.Dt, pack.Rec.Tm);
				ext_pack_dttm.Set(ext_pack.Rec.Dt, ext_pack.Rec.Tm);
				if(cmp(pack_dttm, ext_pack_dttm) >= 0) {
					THROW(P_CM->SyncPrintCheckCopy(&pack, sccr.Format));
				}
				if(cmp(pack_dttm, ext_pack_dttm) <= 0) {
					THROW(P_CM_EXT->SyncPrintCheckCopy(&ext_pack, sccr.Format));
				}
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int CheckPaneDialog::PrintSlipDocument()
{
	int     ok = -1;
	SString title_buf;
	if(oneof2(GetState(), sEMPTYLIST_EMPTYBUF, sLIST_EMPTYBUF) && PNP.NodeID) {
		SelectCheckResult sccr;
		if(IsState(sLIST_EMPTYBUF)) {
			StrAssocArray fmt_list;
			THROW(InitCashMachine());
			if(P_CM->GetSlipFormatList(&fmt_list, 1) > 0) {
				if(fmt_list.getCount() == 1)
					sccr.Format = fmt_list.Get(0).Txt;
				//
				// @todo Вероятно, необходимо вызвать ошибку, если fmt_list.getCount() == 0
				//
			}
		}
		PPLoadString("selectccheck_forslip", title_buf);
		if(sccr.Format.NotEmpty() || SelectCheck(title_buf, scfSelSlipDocFormat|scfThisNodeOnly|scfAllowReturns, sccr) > 0) {
			int   r = -1;
			CCheckPacket  pack;
			THROW(InitCashMachine());
			if(IsState(sLIST_EMPTYBUF)) {
				CCheckTbl::Rec  last_chk_rec;
				if(SuspCheckID && GetCc().Search(SuspCheckID, &last_chk_rec) > 0)
					pack.Rec.Code = last_chk_rec.Code;
				else
					GetNewCheckCode(PNP.NodeID, &pack.Rec.Code);
				THROW(Helper_InitCcPacket(&pack, 0, 0, iccpSetCurTime));
				r = 1; // Если значение оставается меньше нуля документ не печатается
			}
			else {
				if(sccr.CcID) {
					THROW(r = GetCc().LoadPacket(sccr.CcID, 0, &pack));
				}
				if(r < 0)
					r = GetLastCheckPacket(P_CM->GetNodeData().ID, P_CM->GetCurSessID(), &pack);
			}
			if(r > 0) {
				THROW(ok = P_CM->SyncPrintSlipDocument(&pack, sccr.Format));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int CPosProcessor::Print(int noAsk, const PPLocPrinter2 * pLocPrn, uint rptId)
{
	int    ok = 1;
	bool   is_print_dvc_set = false;
	if(!pLocPrn && Flags & fPrinted && !(Flags & fNoEdit) && !(OperRightsFlags & orfChgPrintedCheck) && (PNP.CnSpeciality != PPCashNode::spDelivery)) {
		ok = MessageError(PPERR_NORIGHTS, 0, eomBeep | eomStatusLine);
	}
	else {
		const uint   rpt_id = rptId ? rptId : (pLocPrn ? REPORT_CCHECKDETAILVIEWLOC : REPORT_CCHECKDETAILVIEW);
		SString loc_prn_port(pLocPrn ? pLocPrn->Port : 0);
		loc_prn_port.Strip();
		CCheckItemArray saved_items(P);
		PPReportEnv env(noAsk ? SReport::PrintingNoAsk : 0, 0);
		env.ContextSymb = PNP.CnSymb;
		if(PNP.CnFlags & CASHF_UNIFYGDSTOPRINT) {
			CCheckItem * p_item = 0;
			CCheckItemArray to_print_items;
			for(uint i = 0; P.enumItems(&i, (void **)&p_item);) {
				// Позиции, имеющие модификаторы, объединять нельзя //
				const  bool has_modifier = (i < P.getCount() && P.at(i).Flags & cifModifier);
				bool   _found = false;
				if(!has_modifier) {
					for(uint p = 0; !_found && to_print_items.lsearch(&p_item->GoodsID, &p, CMPF_LONG); p++) {
						CCheckItem & r_ci = to_print_items.at(p);
						if(!(r_ci.Flags & cifHasModifier)) {
							r_ci.Quantity += p_item->Quantity;
							_found = true;
						}
					}
				}
				if(!_found) {
					to_print_items.insert(p_item);
					CCheckItem & r_ci = to_print_items.at(to_print_items.getCount()-1);
					SETFLAG(r_ci.Flags, cifHasModifier, has_modifier);
				}
			}
			P = to_print_items;
		}
		if(pLocPrn) {
			if(loc_prn_port.NotEmpty()) {
				DS.GetTLA().PrintDevice = loc_prn_port;
				env.PrnPort = loc_prn_port; // @v11.8.8
				is_print_dvc_set = true;
			}
		}
		else if(RptPrnPort.NotEmpty()) {
			DS.GetTLA().PrintDevice = RptPrnPort;
			is_print_dvc_set = true;
		}
		PPAlddPrint(rpt_id, PView(this), &env);
		P = saved_items;
		if(pLocPrn) {
			if(pLocPrn->Flags & PPLocPrinter::fHasKitchenBell && KitchenBellCmd.NotEmpty()) {
				SString kitchen_bell_port(KitchenBellPort.NotEmpty() ? KitchenBellPort : loc_prn_port);
				if(kitchen_bell_port.NotEmptyS()) {
					size_t out_size = 0;
					char   out_buf[64];
					const char * p = KitchenBellCmd;
					while(p[0] && p[1]) {
						const uint8 byte = (hex(p[0]) << 4) | hex(p[1]);
						out_buf[out_size++] = byte;
						p += 2;
					}
					SString file_name;
					FILE * f = fopen(loc_prn_port, "w");
					if(f) {
						fwrite(out_buf, out_size, 1, f);
						SFile::ZClose(&f);
					}
				}
			}
		}
		else if(!(Flags & fNoEdit))
			SetPrintedFlag(1);
	}
	if(is_print_dvc_set)
		DS.GetTLA().PrintDevice.Z();
	return ok;
}
//
// ARG(event IN):
//   0 - Строки успешно отпечатаны на принтер rPrnName
//   1 - Для строк не определен принтер
//   2 - Не удалось отпечатать строки на принтер rPrnName
//
int CPosProcessor::MakeDbgPrintLogList(int event, const SString & rFmtBuf, const SString & rChkBuf, const SString & rPrnName, SStrCollection & rList)
{
	int    ok = -1;
	if((CConfig.Flags & CCFLG_DEBUG) && oneof3(event, 0, 1, 2)) {
		SString msg_buf;
		SForEachVectorItem(P, j) {
			const CCheckItem & r_item = P.at(j);
			if(event == 0)
				PPFormat(rFmtBuf, &msg_buf, rChkBuf.cptr(), rPrnName.cptr(), r_item.GoodsID, r_item.Quantity);
			else if(event == 1)
				PPFormat(rFmtBuf, &msg_buf, rChkBuf.cptr(), r_item.GoodsID, r_item.Quantity);
			else if(event == 2)
				PPFormat(rFmtBuf, &msg_buf, rChkBuf.cptr(), rPrnName.cptr(), r_item.GoodsID, r_item.Quantity);
			rList.insert(newStr(msg_buf));
			ok = 1;
		}
	}
	return ok;
}

int CPosProcessor::PrintToLocalPrinters(int selPrnType, bool ignoreNonZeroAgentReq)
{
	int    ok = -1;
	PPID   cur_chk_id = CheckID;
	if(oneof2(GetState(), sEMPTYLIST_EMPTYBUF, sLIST_EMPTYBUF)) {
		if(P.getCount()) {
			int    to_local_prn = -1;
			THROW_PP(ignoreNonZeroAgentReq || !(PNP.CnFlags & CASHF_DISABLEZEROAGENT) || P.GetAgentID(), PPERR_CHKPAN_SALERNEEDED);
			if(selPrnType && (Flags & fLocPrinters)) {
				if(selPrnType > 0) {
					if(DS.IsThreadInteractive()) { // @v11.8.6
						TDialog * dlg = 0;
						THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SELLOCPRN))));
						dlg->setCtrlUInt16(CTL_SELLOCPRN_PRNTYPE, 0);
						if(ExecView(dlg) == cmOK) {
							ushort v = dlg->getCtrlUInt16(CTL_SELLOCPRN_PRNTYPE);
							if(v == 0)
								to_local_prn = 1;
							else if(v == 1)
								to_local_prn = 0;
						}
						delete dlg;
					}
				}
				else
					to_local_prn = 1;
			}
			else
				to_local_prn = 0;
			if(to_local_prn > 0) {
				const bool do_debug_log = LOGIC(CConfig.Flags & CCFLG_DEBUG);
				SStrCollection debug_rep_list;
				SString msg_buf;
				SString chk_code;
				SString prn_name;
				SString buf_ap;
				SString buf_prn;
				SString buf_ulp;
				SString buf_errprn;
				if(do_debug_log) {
					PPLoadText(PPTXT_LOG_CHKPAN_ALLREADYPRN, buf_ap);
					PPLoadText(PPTXT_LOG_CHKPAN_PRINTING, buf_prn);
					PPLoadText(PPTXT_LOG_CHKPAN_UNDEFPRN, buf_ulp);
					PPLoadText(PPTXT_LOG_CHKPAN_ERRPRINTING, buf_errprn);
					CCheckPacket cc_pack;
					GetCheckInfo(&cc_pack);
					CCheckCore::MakeCodeString(&cc_pack.Rec, 0, chk_code);
				}
				PPObjLocPrinter lp_obj;
				PPLocPrinter loc_prn_rec;
				CCheckItem * p_item = 0;
				DS.GetTLA().PrintDevice.Z();
				CCheckItemArray saved_check(P);
				uint   i;
				uint   j;
				uint   first_loc_assoc_pos = 0;
				PPID   loc_id;
				LAssocArray pos_assoc_list;
				PPIDArray printed_pos_list;
				//
				// Инициализация gtoa по ассоциации, установленной в кассовом узле
				//
				InitCashMachine();
				GoodsToObjAssoc gtoa(NZOR(P_CM->GetNodeData().GoodsLocAssocID, PPASS_GOODS2LOC), PPOBJ_LOCATION);
				THROW(gtoa.IsValid());
				THROW(gtoa.Load());
				for(i = 0; saved_check.enumItems(&i, (void **)&p_item);) {
					if(!(p_item->Flags & cifIsPrinted)) {
						gtoa.Get(p_item->GoodsID, &(loc_id = 0));
						pos_assoc_list.Add(loc_id, i-1, 0);
					}
					else if(do_debug_log) {
						PPFormat(buf_ap, &msg_buf, chk_code.cptr(), p_item->GoodsID, p_item->Quantity);
						debug_rep_list.insert(newStr(msg_buf));
					}
				}
				pos_assoc_list.sort(PTR_CMPFUNC(_2long));
				P.freeAll();
				loc_id = 0;
				for(i = 0; i < pos_assoc_list.getCount(); i++) {
					const LAssoc & r_assoc = pos_assoc_list.at(i);
					if(r_assoc.Key != loc_id) {
						if(P.getCount()) {
							if(lp_obj.GetPrinterByLocation(loc_id, prn_name, &loc_prn_rec) > 0) {
								if(Print(1, &loc_prn_rec, 0) > 0) {
									for(j = first_loc_assoc_pos; j < i; j++)
										printed_pos_list.addUnique(pos_assoc_list.at(j).Val);
									MakeDbgPrintLogList(0, buf_prn, chk_code, prn_name, debug_rep_list);
								}
								else
									MakeDbgPrintLogList(2, buf_errprn, chk_code, prn_name, debug_rep_list);
							}
							else
								MakeDbgPrintLogList(1, buf_ulp, chk_code, prn_name, debug_rep_list);
						}
						P.freeAll();
						first_loc_assoc_pos = i;
						loc_id = r_assoc.Key;
					}
					P.insert(&saved_check.at(r_assoc.Val));
				}
				if(P.getCount()) {
					if(lp_obj.GetPrinterByLocation(loc_id, prn_name, &loc_prn_rec) > 0) {
						if(Print(1, &loc_prn_rec, 0) > 0) {
							for(j = first_loc_assoc_pos; j < i; j++)
								printed_pos_list.addUnique(pos_assoc_list.at(j).Val);
							MakeDbgPrintLogList(0, buf_prn, chk_code, prn_name, debug_rep_list);
						}
						else
							MakeDbgPrintLogList(2, buf_errprn, chk_code, prn_name, debug_rep_list);
					}
					else
						MakeDbgPrintLogList(1, buf_ulp, chk_code, prn_name, debug_rep_list);
				}
				for(i = 0; i < printed_pos_list.getCount(); i++)
					saved_check.at(printed_pos_list.at(i)).Flags |= cifIsPrinted;
				P = saved_check;
				AutosaveCheck();
				OnUpdateList(0);
				if(do_debug_log)
					PPLogMessageList(PPFILNAM_DEBUG_LOG, debug_rep_list, LOGMSGF_TIME|LOGMSGF_USER);
			}
			else if(to_local_prn == 0) {
				Print((selPrnType ? 0 : 1), 0, 0);
				AutosaveCheck();
			}
			ok = 1;
		}
		if(IsState(sEMPTYLIST_EMPTYBUF)) {
			P.freeAll();
			CheckID = cur_chk_id;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int CheckPaneDialog::ResetOperRightsByKey()
{
	int    ok = -1;
	if((PNP.CnFlags & CASHF_KEYBOARDWKEY) && GetInput() && Input.IsDec()) {
		long   key_pos = Input.ToLong();
		PPIDArray oper_rights_ary;
		if(GetOperRightsByKeyPos(key_pos, &oper_rights_ary) > 0) {
			long   f = OrgOperRights;
			int    esc_chk = oper_rights_ary.lsearch(CSESSRT_ESCCHECK);
			SETFLAG(f, orfReturns,     oper_rights_ary.lsearch(CSESSOPRT_RETCHECK));
			SETFLAG(f, orfEscCheck,    esc_chk);
			SETFLAG(f, orfEscChkLine, /*esc_chk && @?*/oper_rights_ary.lsearch(CSESSOPRT_ESCCLINE));
			SETFLAG(f, orfBanking,     oper_rights_ary.lsearch(CSESSOPRT_BANKING));
			SETFLAG(f, orfZReport,     oper_rights_ary.lsearch(CSESSRT_CLOSE));
			SETFLAG(f, orfCopyCheck,   oper_rights_ary.lsearch(CSESSOPRT_COPYCHECK));
			SETFLAG(f, orfCopyZReport, oper_rights_ary.lsearch(CSESSOPRT_COPYZREPT));
			SETFLAG(f, orfRowDiscount, oper_rights_ary.lsearch(CSESSOPRT_ROWDISCOUNT));
			SETFLAG(f, orfXReport,     oper_rights_ary.lsearch(CSESSOPRT_XREP << 16));
			SETFLAG(f, orfSplitCheck,      oper_rights_ary.lsearch(CSESSOPRT_SPLITCHK));
			SETFLAG(f, orfMergeChecks,     oper_rights_ary.lsearch(CSESSOPRT_MERGECHK));
			SETFLAG(f, orfChgPrintedCheck, oper_rights_ary.lsearch(CSESSOPRT_CHGPRINTEDCHK));
			SETFLAG(f, orfChgAgentInCheck, oper_rights_ary.lsearch(CSESSOPRT_CHGCCAGENT));
			SETFLAG(f, orfEscChkLineBeforeOrder, oper_rights_ary.lsearch(CSESSOPRT_ESCCLINEBORD));
			SETFLAG(f, orfReprnUnfCc,      oper_rights_ary.lsearch(CSESSOPRT_REPRNUNFCC));
			SETFLAG(f, orfArbitraryDiscount, oper_rights_ary.lsearch(CSESSOPRT_ARBITRARYDISC)); // @v11.0.9
			if(!(Flags & fUsedRighsByAgent))
				OrgOperRights = OperRightsFlags = f;
			else
				OrgOperRights = f;
			{
				SString added_msg_str;
				(added_msg_str = "Key").Space().Cat(key_pos);
				DS.GetTLA().AddedMsgStrNoRights = added_msg_str;
			}
			ok = 1;
		}
		setupHint();
	}
	ClearInput(0);
	return ok;
}

int CheckPaneDialog::PrintCashReports()
{
	int    ok = 1;
	int    r = -1;
	int    r_ext = -1;
	int    c = cmCancel;
	int    zreport_printed = 0;
	SString temp_buf;
	CSPanel * dlg = 0;
	if(!(Flags & fNoEdit) && IsState(sEMPTYLIST_EMPTYBUF)) {
		int    csp_flags = 0;
		// 
		// Снять X-отчет;Открыть сессию;Копия Z-отчета;Инкассация;Блокировать;Разблокировать;Открыть денежный ящик;Режим ввода чека-метки
		// 
		{
#if 0 // @construction {
			if(PPLoadTextWin(PPTXT_MENU_CHKPAN, temp_buf)) {
				TMenuPopup menu;
				menu.AddSubstr(temp_buf, 0, cmSCSXReport);
				menu.AddSubstr(temp_buf, 1, cmCSOpen);
				menu.AddSubstr(temp_buf, 2, cmSCSZReportCopy);
				menu.AddSubstr(temp_buf, 3, cmSCSIncasso);
				menu.AddSubstr(temp_buf, 4, cmSCSLock);
				menu.AddSubstr(temp_buf, 5, cmSCSUnlock);
				menu.AddSubstr(temp_buf, 6, /*Открыть денежный ящик*/0);
				menu.AddSubstr(temp_buf, 7, cmSCSLock);
				int    cmd = menu.Execute(H(), TMenuPopup::efRet);
				/*
				if(cmd > 0)
					TView::messageCommand(this, cmd, 0);
				*/
			}
#endif // } 0 @construction
		}
		LDATE  dt;
		SETFLAG(csp_flags, CSPanel::fcspZReport,   OperRightsFlags & orfZReport);
		SETFLAG(csp_flags, CSPanel::fcspZRepCopy,  OperRightsFlags & orfCopyZReport);
		SETFLAG(csp_flags, CSPanel::fcspXReport,   OperRightsFlags & orfXReport);
		SETFLAG(csp_flags, CSPanel::fcspCheckCopy, (OperRightsFlags & orfCopyCheck) && IsState(sEMPTYLIST_EMPTYBUF));
		dlg = new CSPanel((DlgFlags & fLarge) ? DLG_CASHREPORTS_L : DLG_CASHREPORTS, PNP.NodeID, 0, csp_flags); // @newok
		THROW(CheckDialogPtr(&dlg));
		THROW(InitCashMachine());
		dlg->showCtrl(CTL_CSPANEL_CSESSOPEN,  LOGIC(Flags & fOnlyReports));
		dlg->showCtrl(CTL_CSPANEL_CSESSCLOSE, !LOGIC(Flags & fOnlyReports));
		dlg->showCtrl(CTL_CSPANEL_RESETCHZNPMSVRADDR, PNP.ChZnGuaID); // @v12.4.1
		while((c = ExecView(dlg)) != cmCancel) {
			switch(c) {
				case cmCSOpen:
					r = P_CM->SyncOpenSession(&(dt = ZERODATE));
					if(P_CM_EXT)
						if(P_CM_EXT->GetNodeData().CashType == PPCMT_PAPYRUS) {
							if(r > 0) {
								P_CM_EXT->SetParentNode(PNP.NodeID);
								P_CM_EXT->AsyncOpenSession(0, 0);
							}
						}
						else
							r_ext = P_CM_EXT->SyncOpenSession(&dt);
					if(r > 0 || r_ext > 0)
						Flags &= ~fOnlyReports;
					break;
				case cmCSClose:
					{
						SString zcheck;
						/* @v11.0.9 Экспериментально перемещаем печать сверки по банку после печати z-отчета
						if(P_BNKTERM) {
							if(P_BNKTERM->GetSessReport(zcheck))
								P_CM->SyncPrintBnkTermReport(1, zcheck);
							else 
								PPError();
						}
						*/
						r = P_CM->SyncCloseSession();
						if(P_CM_EXT) {
							if(P_CM_EXT->GetNodeData().CashType == PPCMT_PAPYRUS) {
								if(r > 0) {
									P_CM_EXT->SetParentNode(PNP.NodeID);
									P_CM_EXT->AsyncCloseSession(0, 0);
								}
							}
							else
								r_ext = P_CM_EXT->SyncCloseSession();
						}
						// @v11.0.9 (Экспериментально перемещаем печать сверки по банку после печати z-отчета) {
						if(P_BNKTERM) {
							if(P_BNKTERM->GetSessReport(zcheck)) {
								if(CConfig.Flags & CCFLG_DEBUG) {
									//
									// В одном из магазинов при снятии Z-отчета после печати банковского слипа загибается касса viki-print
									// Данный дамп призван помочь идентифицировать проблему.
									//
									PPGetFilePath(PPPATH_LOG, "bnkterm_zrep_dump.txt", temp_buf);
									SFile f_debug(temp_buf, SFile::mAppend|SFile::mBinary);
									if(f_debug.IsValid()) {
										temp_buf.Z().CR().CatCurDateTime(DATF_ISO8601, 0).CR();
										f_debug.WriteLine(temp_buf);
										f_debug.Write(zcheck.cptr(), zcheck.Len());
									}
								}
								P_CM->SyncPrintBnkTermReport(1, zcheck);
							}
							else 
								PPError();
						}
						// }
						if(r > 0 || r_ext > 0)
							zreport_printed = 1;
					}
					break;
				case cmSCSXReport:
					r = P_CM->SyncPrintXReport();
					if(P_CM_EXT)
						r_ext = P_CM_EXT->SyncPrintXReport();
					break;
				case cmSCSZReportCopy:
					if(OperRightsFlags & orfCopyZReport) {
						CSessInfo cs_info;
						r = SelectCSession(PNP.NodeID, ExtCashNodeID, &cs_info);
						if(r == 1)
							r = P_CM->SyncPrintZReportCopy(&cs_info);
						else if(r == 2 && P_CM_EXT)
							r_ext = P_CM_EXT->SyncPrintZReportCopy(&cs_info);
					}
					else
						r = 0;
					break;
				case cmSCSLock:
					r = Sleep();
					break;
				case cmSCSIncasso:
					r = P_CM->SyncPrintIncasso();
					break;
				case cmSCSPrintCheckCopy:
					PrintCheckCopy();
					break;
				case cmSCSDrawerOpen: // @v11.6.9
					P_CM->SyncOpenBox();
					break;
				case cmCSResetChZnPmSvrAddr: // @v12.4.1
					{
						bool done = false;
						bool err = false;
						Reference * p_ref(PPRef);
						if(PNP.ChZnGuaID) {
							PPObjGlobalUserAcc gua_obj;
							PPGlobalUserAcc gua_rec;
							if(gua_obj.Fetch(PNP.ChZnGuaID, &gua_rec) > 0 && p_ref->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, PNP.ChZnGuaID, PPTAG_GUA_CHZN_PM_HOST, temp_buf) > 0) {
								if(p_ref->Ot.RemoveTag(PPOBJ_GLOBALUSERACC, PNP.ChZnGuaID, PPTAG_GUA_CHZN_PM_HOST, 1)) {
									done = true;
								}
								else
									err = true;
							}
						}
						if(done) {
							//PPTXT_CHZNPMSVRADDRHASBEENRESET        "Адрес сервера разрешительного режима чзн был успешно сброшен"
							PPLoadText(PPTXT_CHZNPMSVRADDRHASBEENRESET, temp_buf);
							PPTooltipMessage(temp_buf, 0, H(), 10000, GetColorRef(SClrGreen),
								SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
						}
						else if(err) {
							MessageError(-1, 0, eomPopup);
						}
						else {
							//PPTXT_CHZNPMSVRADDRRESET_NOTDONE       "Адрес сервера разрешительного режима чзн либо не задан"
							PPLoadText(PPTXT_CHZNPMSVRADDRRESET_NOTDONE, temp_buf);
							PPTooltipMessage(temp_buf, 0, H(), 10000, GetColorRef(SClrCoral),
								SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
						}
					}
					break;
			}
			if(r == 0 || r_ext == 0)
				PPError();
			else
				break;
		}
		ZDELETE(dlg);
		if(zreport_printed)
			if(PPMessage(mfConf|mfYesNo, PPCFM_PREVCASHDAYCLOSED) == cmYes) {
				dt = ZERODATE;
				if(r > 0)
					THROW(ok = P_CM->SyncOpenSession(&dt));
				if(ok > 0 && r_ext > 0)
					THROW(ok = P_CM_EXT->SyncOpenSession(&dt));
				if(ok > 0)
					Flags &= ~fOnlyReports;
			}
			else {
				Flags |= fOnlyReports;
				SetupInfo(0);
			}
	}
	ok = (Flags & fOnlyReports) ? -1 : 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int CheckPaneDialog::GetDataFromScale(PPID * pGoodsID, double * pWeight)
{
	TIDlgInitData  tidi;
	int  ok = GetScaleData(ScaleID, &tidi);
	ASSIGN_PTR(pGoodsID, tidi.GoodsID);
	ASSIGN_PTR(pWeight, tidi.Quantity);
	return ok;
}

int CheckPaneDialog::SetupRowByScale()
{
	int    ok = -1;
	if(ScaleID) {
		PPID   goods_id = 0;
		double weight = 0.0;
		int    r = GetDataFromScale(&goods_id, &weight);
		if(r > 0 && goods_id) {
			PgsBlock pgsb(weight);
			ok = SetupNewRow(goods_id, pgsb);
			Flags &= ~fNotUseScale;
		}
		else if(r != -2)
			ok = PPErrorZ();
	}
	return ok;
}

int CheckPaneDialog::LoadTSession(PPID tsessID)
{
	int    ok = 1;
	SString temp_buf;
	TSessionTbl::Rec tses_rec;
	TSessLineTbl::Rec ln_rec;
	THROW_MEM(SETIFZ(P_TSesObj, new PPObjTSession));
	THROW(P_TSesObj->Search(tsessID, &tses_rec) > 0);
	if(tses_rec.CCheckID_) {
		//
		// По сессии уже был сформирован чек
		//
		CCheckTbl::Rec cc_rec;
		if(GetCc().Search(tses_rec.CCheckID_, &cc_rec) > 0)
			CCheckCore::MakeCodeString(&cc_rec, 0, temp_buf);
		else
			temp_buf.Z();
		CALLEXCEPT_PP_S(PPERR_TSESSALREADYCCHECKED, temp_buf);
	}
	//
	// Формирование чека возможно только по закрытой сессии
	//
	THROW_PP(tses_rec.Status == TSESST_CLOSED, PPERR_TSESSCCHECKNOTCLOSED);
	{
		PPObjTSession::WrOffAttrib attrib;
		THROW(P_TSesObj->GetWrOffAttrib(&tses_rec, &attrib));
		SetupAgent(attrib.AgentID, 0);
		if(attrib.SCardID)
			AcceptSCard(attrib.SCardID, 0, 0);
		const bool do_setup_price = LOGIC(P_TSesObj->GetConfig().Flags & PPTSessConfig::fSetupCcPricesInCPane);
		for(SEnum en = P_TSesObj->P_Tbl->EnumLines(tsessID); en.Next(&ln_rec) > 0;) {
			if(ln_rec.Sign < 0) {
				PgsBlock pgsb(fabs(ln_rec.Qtty));
				if(!do_setup_price)
					pgsb.PriceBySerial = ln_rec.Price;
				/*THROW(*/SetupNewRow(ln_rec.GoodsID, /*fabs(ln_rec.Qtty), ln_rec.Price, 0*/pgsb)/*)*/;
			}
		}
		AcceptRow();
		OuterOi.Set(PPOBJ_TSESSION, tsessID);
		SetupInfo(0);
		ClearInput(0);
	}
	CATCH
		ok = MessageError(PPErrCode, 0, eomBeep | eomStatusLine);
	ENDCATCH
	return ok;
}

int CheckPaneDialog::LoadChkInP(PPID chkinpID, PPID goodsID, double qtty)
{
	int    ok = 1;
	PPObjTSession::WrOffAttrib wroff_attrib;
	SString temp_buf;
	PPCheckInPersonMngr cip_mgr;
	PPCheckInPersonItem cip_item;
	MEMSZERO(wroff_attrib);
	THROW(cip_mgr.Search(chkinpID, &cip_item) > 0);
	if(cip_item.CCheckID) {
		//
		// По записи уже был сформирован чек
		//
		CCheckTbl::Rec cc_rec;
		if(GetCc().Search(cip_item.CCheckID, &cc_rec) > 0)
			CCheckCore::MakeCodeString(&cc_rec, 0, temp_buf);
		else
			temp_buf.Z();
		CALLEXCEPT_PP_S(PPERR_CHKINPALREADYCCHECKED, temp_buf);
	}
	//
	// Формирование чека возможно только по закрытой сессии
	//
	THROW_PP(cip_item.GetStatus() == cip_item.statusCheckedIn, PPERR_CHKINPCCHECKNOTCLOSED);
	if(cip_item.Kind == PPCheckInPersonItem::kTSession && cip_item.PrmrID) {
		TSessionTbl::Rec tses_rec;
		THROW_MEM(SETIFZ(P_TSesObj, new PPObjTSession));
		if(P_TSesObj->Search(cip_item.PrmrID, &tses_rec) > 0) {
			THROW(P_TSesObj->GetWrOffAttrib(&tses_rec, &wroff_attrib));
		}
	}
	{
		PgsBlock pgsb(fabs(qtty));
		/*THROW(*/SetupNewRow(goodsID, /*fabs(qtty), 0.0, 0*/pgsb)/*)*/;
		AcceptRow();
		SetupAgent(wroff_attrib.AgentID, 0);
		if(cip_item.SCardID)
			AcceptSCard(cip_item.SCardID, 0, 0);
		OuterOi.Set(PPOBJ_CHKINP, chkinpID);
		SetupInfo(0);
		ClearInput(0);
	}
	CATCH
		ok = MessageError(PPErrCode, 0, eomBeep | eomStatusLine);
	ENDCATCH
	return ok;
}
//
//
//
int CCheckPane(PPID cashNodeID, PPID chkID, const char * pInitLine, long flags)
{
	int    ok = 1;
	bool   is_touch_screen = false;
	const  PPID sav_loc_id = LConfig.Location;
	CCheckPacket pack;
	CheckPaneDialog * dlg = 0;
	PPObjSCard sc_obj;
  	if(cashNodeID) {
		PPCashNode cn_rec;
		PPObjCashNode cn_obj;
		THROW(cn_obj.Search(cashNodeID, &cn_rec) > 0);
		if(cn_rec.CashType != PPCMT_DISTRIB)
			THROW(DS.SetLocation(cn_rec.LocID));
		if(!chkID && cn_rec.Flags & CASHF_SYNC) {
			PPSyncCashNode  scn;
			if(cn_obj.GetSync(cashNodeID, &scn) > 0) {
				const  PPID ts_id = NZOR(scn.LocalTouchScrID, scn.TouchScreenID);
				if(ts_id)
					is_touch_screen = true;
			}
		}
	}
	if(chkID) {
		THROW(sc_obj.P_CcTbl->LoadPacket(chkID, 0, &pack));
	}
	THROW(CheckDialogPtr(&(dlg = new CheckPaneDialog(cashNodeID, chkID, chkID ? &pack : 0, (is_touch_screen ? CheckPaneDialog::ctrfTouchScreen : 0)))));
	if(pInitLine)
		dlg->SetInput(pInitLine);
	if(flags & cchkpanfOnce)
		dlg->UiFlags |= (CheckPaneDialog::uifOnce | CheckPaneDialog::uifCloseWOAsk);
	ExecViewAndDestroy(dlg);
	CATCHZOKPPERR
	DS.SetLocation(sav_loc_id);
	return ok;
}
//
// InfoKiosk
//
class InfoKioskDialog : public TDialog {
public:
	InfoKioskDialog(const PPGoodsInfo * pRec, PPID defGoodsGrpID) : TDialog(pRec && pRec->TouchScreenID ? DLG_INFKIOSK_TS : DLG_INFKIOSK)
	{
		SString font_face;
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_LOTS));
		if(!RVALUEPTR(Rec, pRec))
			MEMSZERO(Rec);
		SelGoodsGrpID = AltGoodsGrpID = defGoodsGrpID;
		LastCtrlID = 0;
		if(p_list)
			SetupStrListBox(p_list);
		SetupLots(0);
		Flags = (pRec && pRec->TouchScreenID) ? fTouchScreen : 0;
		if(Flags & fTouchScreen) {
			int  set_tool_tips = 1;
			PPTouchScreenPacket ts_pack;
			PPObjTouchScreen    ts_obj;
			if(ts_obj.GetPacket(pRec->TouchScreenID, &ts_pack) > 0) {
				if(!defGoodsGrpID)
					SelGoodsGrpID = AltGoodsGrpID = ts_pack.Rec.AltGdsGrpID;
				if(ts_pack.Rec.Flags & TSF_TXTSTYLEBTN) {
					ResetButtonToTextStyle(CTL_INFKIOSK_SCARD);
					ResetButtonToTextStyle(CTL_INFKIOSK_BYPRICE);
					ResetButtonToTextStyle(CTL_INFKIOSK_BYNAME);
					ResetButtonToTextStyle(CTL_INFKIOSK_PRINTLBL);
					ResetButtonToTextStyle(CTL_INFKIOSK_ENTER);
					ResetButtonToTextStyle(CTL_INFKIOSK_SELGDSGRP);
					ResetButtonToTextStyle(CTL_INFKIOSK_GRPBYDEF);
					set_tool_tips = 0;
				}
			}
			if(!SetupStrListBox(this, CTL_INFKIOSK_GRPLIST))
				PPError();
			SmartListBox * p_tree_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_GRPLIST));
			if(p_tree_list) {
				PPObjGoodsGroup gg_obj;
				p_tree_list->setDef(gg_obj.Selector(0, 0, 0));
				p_tree_list->Draw_();
				p_tree_list->P_Def->SetOption(lbtSelNotify, 1);
			}
			UpdateGList(-1);
			if(set_tool_tips)
				TView::messageCommand(this, cmSetupTooltip);
			enableCommand(cmaAltSelect, AltGoodsGrpID ? 1 : 0);
			DlgFlags |= fLarge;
		}
		selectCtrl(CTL_INFKIOSK_INPUT);
		DefInputLine = CTL_INFKIOSK_INPUT;
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_TIMESNEWROMAN, font_face);
		SetCtrlsFont(font_face, (Flags & fTouchScreen) ? 32 : 24, CTL_INFKIOSK_GOODS, CTL_INFKIOSK_CODE,
			CTL_INFKIOSK_PRICE, CTL_INFKIOSK_DSCNTPRICE, CTL_INFKIOSK_STATUS, 0);
		SetCtrlsFont(font_face, 20, CTL_INFKIOSK_LOTS, CTL_INFKIOSK_ADDINF1, CTL_INFKIOSK_ADDINF2,
			CTL_INFKIOSK_ADDINF3, CTL_INFKIOSK_ADDINF4, CTL_INFKIOSK_ADDINF5, 0);
		SetCtrlFont(CTL_INFKIOSK_INPUT,      font_face, (Flags & fTouchScreen) ? 20 : 16);
	}
private:
	DECL_HANDLE_EVENT;

	enum SearchParam {
		srchByNone    = 1,
		srchByBarcode,
		srchBySerial,
		srchByPrice,
		srchByName
	};

	int    SelectSCard();
	int    SelectGoods(SearchParam srch);
	int    SetupGoods(PPID goodsID, double qtty);
	int    SetupLots(PPID goodsID);
	int    SetupInfo();
	int    PrintLabel();
	bool   GetInput();
	void   ClearInput();
	void   UpdateGList(int updGdsList);
	int    ProcessGoodsSelection();
	void   ResetListWindows();
	int    SetDlgResizeParams();
	void   ResetButtonToTextStyle(uint ctrlID);
	int    ProcessEnter();
	enum {
		fWaitOnSCard  = 0x0001, // Ожидание ввода дисконтной карты
		fTouchScreen  = 0x0002  // Используется TouchScreen
	};
	long   Flags;
	PPID   LastCtrlID;
	PPID   AltGoodsGrpID;
	PPID   SelGoodsGrpID;
	SString Input;
	struct State {
		State() : GoodsID(0), Price(0.0), Rest(0.0), Qtty(0.0)
		{
		}
		State & Z()
		{
			THISZERO();
			return *this;
		}
		PPID   GoodsID;
		double Price;
		double Rest;
		double Qtty;
	};
	State  St;
    PPGoodsInfo Rec;
	PPObjGoods  GObj;
	PPObjQCert  QCObj;
	PPObjSCard  SCObj;
	PPObjSCardSeries SCSerObj;
};

int InfoKioskDialog::SetupInfo()
{
	int    ok = 1;
	SString status_buf, word;
	const  long fmt = MKSFMTD(0, 3, NMBF_NOTRAILZ);
	if(St.Rest > 0.0)
		status_buf.Space().Cat(PPLoadStringS("rest", word)).CatDiv(':', 2).Cat(St.Rest, fmt).Space();
	if(St.Qtty > 0.0)
		status_buf.Space().Cat(PPLoadStringS("qtty", word)).CatDiv(':', 2).Cat(St.Qtty, fmt);
	setStaticText(CTL_INFKIOSK_STATUS, status_buf);
	return ok;
}

int InfoKioskDialog::ProcessEnter()
{
	int    ok = 1;
	if(GetInput()) {
		if(Input.C(0) == '*' || Input.Last() == '*') {
			if(St.GoodsID) {
				St.Qtty = R6(Input.ShiftLeftChr('*').ToReal());
				if(St.Qtty > 0.0) {
					if(Input.TrimRightChr('*').Last() == '/') {
						double phuperu;
						if(GObj.GetPhUPerU(St.GoodsID, 0, &phuperu) > 0)
							St.Qtty = R6(St.Qtty / phuperu);
					}
					SetupInfo();
					ClearInput();
				}
			}
		}
		else if(Flags & fWaitOnSCard)
			SelectSCard();
		else
			SelectGoods(srchByBarcode);
	}
	else
		ok = -1;
	return ok;
}

IMPL_HANDLE_EVENT(InfoKioskDialog)
{
	if(event.isCmd(cmOK)) {
		ProcessEnter();
		clearEvent(event);
	}
	TDialog::handleEvent(event);
	if(TVBROADCAST)
		if(TVCMD == cmReleasedFocus && (Flags & fTouchScreen))
			LastCtrlID = event.message.infoView->GetId();
		else
			return;
	else if(TVCOMMAND) {
		const uint ev_ctl_id = event.getCtlID();
		switch(TVCMD) {
			case cmSetupResizeParams: SetDlgResizeParams(); break;
			case cmaSelect:  UpdateGList(0); break;
			case cmSelSCard: SelectSCard(); break;
			case cmByPrice:  UpdateGList(-2); break;
			case cmByName:   UpdateGList(-3); break;
			case cmPrint:    PrintLabel(); break;
			case cmSetupTooltip:
				if(Flags & fTouchScreen) {
					SString tt_names;
					SString name;
					if(PPLoadTextWin(PPTXT_INFKIOSK_TOOLTIPS, tt_names))
						for(uint idx = 0; idx < CTL_INFKIOSK_NUMBUTTONS; idx++)
							if(PPGetSubStr(tt_names, idx, name))
								SetCtrlToolTip(CTL_INFKIOSK_STARTBUTTON + idx, name);
				}
				break;
			case cmaInsert:
				if(ProcessEnter() < 0) {
					if(LastCtrlID == CTL_INFKIOSK_GRPLIST)
						UpdateGList(1);
					else if(LastCtrlID == CTL_INFKIOSK_GDSLIST)
						ProcessGoodsSelection();
				}
				break;
			case cmaAltSelect:
				SelGoodsGrpID = AltGoodsGrpID;
				UpdateGList(-1);
				break;
			case cmLBItemSelected:
			case cmLBDblClk:
				if(ev_ctl_id == CTL_INFKIOSK_GRPLIST) {
					UpdateGList(1);
					if(TVCMD == cmLBItemSelected)
						selectCtrl(CTL_INFKIOSK_INPUT);
				}
				else if(ev_ctl_id == CTL_INFKIOSK_GDSLIST)
					ProcessGoodsSelection();
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF2: SelectGoods(srchByNone); break;
			case kbF3: SelectSCard(); break;
			case kbF7: PrintLabel(); break;
			case kbF4:
				if(Flags & fTouchScreen)
					UpdateGList(-3);
				else
					SelectGoods(srchByName);
				break;
			case kbF5:
				if(Flags & fTouchScreen)
					UpdateGList(-2);
				else
					SelectGoods(srchByPrice);
				break;
			default:
				return;
		}
	}
	else
		return;
	clearEvent(event);
}

void InfoKioskDialog::ResetButtonToTextStyle(uint ctrlID)
{
	long   style = TView::SGetWindowStyle(::GetDlgItem(H(), ctrlID));
	style &= ~BS_BITMAP;
	TView::SetWindowProp(::GetDlgItem(H(), ctrlID), GWL_STYLE, reinterpret_cast<void *>(style));
}

bool InfoKioskDialog::GetInput()
{
	getCtrlString(CTL_INFKIOSK_INPUT, Input);
	return Input.NotEmptyS();
}

void InfoKioskDialog::ClearInput()
{
	setCtrlString(CTL_INFKIOSK_INPUT, Input.Z());
	selectCtrl(CTL_INFKIOSK_INPUT);
}

void InfoKioskDialog::UpdateGList(int updGdsList)
{
	if(Flags & fTouchScreen) {
		SString  grp_name;
		if(updGdsList > 0) {
			PPID  cur_id = 0;
			SmartListBox * p_tree_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_GRPLIST));
			if(SmartListBox::IsValidS(p_tree_list)) {
				p_tree_list->P_Def->getCurID(&cur_id);
				if(static_cast<StdTreeListBoxDef *>(p_tree_list->P_Def)->HasChildren(cur_id))
					updGdsList = 0;
				else
					SelGoodsGrpID = cur_id;
			}
		}
		if(updGdsList) {
			ListBoxDef   * p_def = 0;
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_GDSLIST));
			StrAssocArray * p_ts_ary = 0;
			PPWaitStart();
			if(updGdsList == -2) {
				if(GetInput()) {
					const double p = Input.ToReal();
					p_ts_ary = GObj.CreateListByPrice(LConfig.Location, R2(p));
					p_def = new StrAssocListBoxDef(p_ts_ary, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
					PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELBYPRICE, grp_name);
					grp_name.Space().Cat(p, SFMT_MONEY);
				}
				ClearInput();
			}
			else if(updGdsList == -3) {
				if(GetInput()) {
					SString pattern;
					if(Input.Len() >= INSTVSRCH_THRESHOLD)
						pattern.CatChar('!');
					pattern.Cat(Input);
					p_ts_ary = new StrAssocArray;
					GObj.P_Tbl->GetListBySubstring(pattern, p_ts_ary, -1, true);
					p_def = new StrAssocListBoxDef(p_ts_ary, lbtDblClkNotify | lbtFocNotify | lbtDisposeData);
					grp_name = Input;
				}
				ClearInput();
			}
			else {
				Goods2Tbl::Rec grp_rec;
				if(GObj.Fetch(SelGoodsGrpID, &grp_rec) > 0) {
					PPLoadStringS("group", grp_name).CatDiv(':', 2).Cat(grp_rec.Name);
				}
				else
					grp_name.Z();
				p_def = GObj.Selector(0, 0, reinterpret_cast<void *>(SelGoodsGrpID));
			}
			p_list->setDef(p_def);
			CALLPTRMEMB(p_list->P_Def, SetOption(lbtSelNotify, 1));
			p_list->Draw_();
			PPWaitStop();
		}
		else
			PPGetSubStr(PPTXT_CHKPAN_INFO, PPCHKPAN_SELGROUP, grp_name);
		showCtrl(CTL_INFKIOSK_GRPLIST,    !updGdsList);
		disableCtrl(CTL_INFKIOSK_GRPLIST,  LOGIC(updGdsList));
		::ShowWindow(::GetDlgItem(H(), MAKE_BUTTON_ID(CTL_INFKIOSK_GRPLIST, 1)), updGdsList ? SW_HIDE : SW_SHOW);
		showCtrl(CTL_INFKIOSK_GDSLIST,     updGdsList);
		disableCtrl(CTL_INFKIOSK_GDSLIST, !updGdsList);
		::ShowWindow(::GetDlgItem(H(), MAKE_BUTTON_ID(CTL_INFKIOSK_GDSLIST, 1)), updGdsList ? SW_SHOW : SW_HIDE);
		enableCommand(cmaSelect, updGdsList);
		setStaticText(CTL_INFKIOSK_GRPNAME, grp_name);
		LastCtrlID = updGdsList ? CTL_INFKIOSK_GDSLIST : CTL_INFKIOSK_GRPLIST;
	}
}

int InfoKioskDialog::ProcessGoodsSelection()
{
	int    ok = 1;
	PPID   goods_id = 0;
	SString  buf;
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_GDSLIST));
	if(SmartListBox::IsValidS(p_list))
		p_list->P_Def->getCurID(&goods_id);
	ClearInput();
	setCtrlReal(CTL_INFKIOSK_DSCNTPRICE, 0.0);
	setStaticText(CTL_INFKIOSK_INFO, buf);
	if(!SetupGoods(goods_id, 0.0)) {
		PPGetLastErrorMessage(0, buf);
		setStaticText(CTL_INFKIOSK_INFO, buf);
		ok = 0;
	}
	return ok;
}

void InfoKioskDialog::ResetListWindows()
{
	if(Flags & fTouchScreen) {
		SString font_face;
		const int   sx  = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME);
		const int   sy  = GetSystemMetrics((DlgFlags & fResizeable) ? SM_CYSIZEFRAME : SM_CYFIXEDFRAME);
		const int   cy  = GetSystemMetrics(SM_CYCAPTION);
		const int   vsx = GetSystemMetrics(SM_CXVSCROLL);
		RECT  dlg_rect;
		RECT  ctrl_rect;
		HWND  ctrl_wnd = GetDlgItem(H(), CTL_INFKIOSK_GDSLIST);
		::GetWindowRect(H(), &dlg_rect);
		GetWindowRect(ctrl_wnd, &ctrl_rect);
		ctrl_rect.right -= vsx;
		MoveWindow(ctrl_wnd, ctrl_rect.left - (dlg_rect.left + sx), ctrl_rect.top - (dlg_rect.top + sy + cy),
			ctrl_rect.right - ctrl_rect.left, ctrl_rect.bottom - ctrl_rect.top, 1);
		ctrl_wnd = ::GetDlgItem(H(), MAKE_BUTTON_ID(CTL_INFKIOSK_GDSLIST, 1));
		::GetWindowRect(ctrl_wnd, &ctrl_rect);
		ctrl_rect.left -= vsx;
		MoveWindow(ctrl_wnd, ctrl_rect.left - (dlg_rect.left + sx), ctrl_rect.top - (dlg_rect.top + sy + cy),
			ctrl_rect.right - ctrl_rect.left, ctrl_rect.bottom - ctrl_rect.top, 1);
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_TIMESNEWROMAN, font_face);
		SetCtrlFont(CTL_INFKIOSK_GRPNAME, font_face, 32);
		SetCtrlFont(CTL_INFKIOSK_GDSLIST, font_face, 24);
		SetCtrlFont(CTL_INFKIOSK_GRPLIST, font_face, 24);
	}
}

int InfoKioskDialog::SetDlgResizeParams()
{
	if(Flags & fTouchScreen) {
		SetCtrlResizeParam(CTL_INFKIOSK_GOODS, 0, 0, CTL_INFKIOSK_GRPNAME, -1, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPNAME, CTL_INFKIOSK_GOODS, 0, 0, -1, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GDSLIST, CTL_INFKIOSK_GRPNAME, 0, 0, 0, crfLinkLeft | crfResizeable);
		SetCtrlResizeParam(MAKE_BUTTON_ID(CTL_INFKIOSK_GDSLIST, 1), -1, 0, 0, 0, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPLIST, CTL_INFKIOSK_GRPNAME, 0, 0, 0, crfLinkLeft | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_CODE, CTL_INFKIOSK_GOODS, 0, CTL_INFKIOSK_PRICE, -1, crfLinkLeft | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_PRICE, CTL_INFKIOSK_CODE, 0, CTL_INFKIOSK_GOODS, -1, crfLinkRight | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_STATUS, CTL_INFKIOSK_CODE, 0, CTL_INFKIOSK_CODE, -1, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_DSCNTPRICE, CTL_INFKIOSK_PRICE, 0, CTL_INFKIOSK_PRICE, -1, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_IMAGE, 0, 0, CTL_INFKIOSK_CODE, CTL_INFKIOSK_LOTS, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_LOTS, 0, CTL_INFKIOSK_IMAGE, CTL_INFKIOSK_GOODS, 0, crfLinkRight | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_PRICE, 0, CTL_INFKIOSK_GOODS, CTL_INFKIOSK_IMAGE, CRF_LINK_LEFTRIGHTBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF1, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF2, CRF_LINK_LEFTRIGHTTOP | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF2, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF1, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF3, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF3, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF2, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF4, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF4, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF3, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF5, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF5, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_ADDINF4, CTL_INFKIOSK_GRPBOX1, CTL_INFKIOSK_GRPBOX1, CRF_LINK_LEFTRIGHTBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GOODS, CTL_INFKIOSK_LOTS, CTL_INFKIOSK_GOODS, CTL_INFKIOSK_IMAGE, CRF_LINK_ALL | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_SCARD, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_BYPRICE, CTL_INFKIOSK_GRPBOX2, CRF_LINK_LEFTTOPBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_BYPRICE, CTL_INFKIOSK_SCARD, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_BYNAME, CTL_INFKIOSK_GRPBOX2, CRF_LINK_TOPBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_BYNAME, CTL_INFKIOSK_BYPRICE, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_PRINTLBL, CTL_INFKIOSK_GRPBOX2, CRF_LINK_TOPBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_PRINTLBL, CTL_INFKIOSK_BYNAME, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ENTER, CTL_INFKIOSK_GRPBOX2, CRF_LINK_TOPBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ENTER, CTL_INFKIOSK_PRINTLBL, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CRF_LINK_RIGHTTOPBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_INPUT, 0, -1, CTL_INFKIOSK_IMAGE, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX3, 0, -1, CTL_INFKIOSK_GRPBOX4, 0, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX4, CTL_INFKIOSK_GRPBOX3, -1, 0, 0, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_INFO, CTL_INFKIOSK_GRPBOX1, -1, CTL_INFKIOSK_GRPBOX3, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_SELGDSGRP, CTL_INFKIOSK_GRPBOX4, -1, CTL_INFKIOSK_GRPBYDEF, 0, crfLinkLeft | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBYDEF, CTL_INFKIOSK_SELGDSGRP, -1, CTL_INFKIOSK_GRPBOX4, 0, crfLinkRight | crfResizeable);
	}
	else {
		SetCtrlResizeParam(CTL_INFKIOSK_GOODS, 0, 0, CTL_INFKIOSK_IMAGE, -1, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_IMAGE, CTL_INFKIOSK_GOODS, 0, 0, CTL_INFKIOSK_LOTS, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_LOTS, CTL_INFKIOSK_IMAGE, CTL_INFKIOSK_IMAGE, CTL_INFKIOSK_IMAGE, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_CODE, CTL_INFKIOSK_GOODS, 0, CTL_INFKIOSK_GOODS, -1, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_PRICE, CTL_INFKIOSK_GOODS, 0, CTL_INFKIOSK_DSCNTPRICE, -1, crfLinkLeft | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_DSCNTPRICE, CTL_INFKIOSK_PRICE, 0, CTL_INFKIOSK_GOODS, -1, crfLinkRight | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_STATUS, CTL_INFKIOSK_GOODS, 0, CTL_INFKIOSK_GOODS, -1, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX1, 0, 0, 0, 0, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GOODS, 0, CTL_INFKIOSK_GOODS, 0, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF1, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF2, CRF_LINK_LEFTRIGHTTOP | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF1, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF3, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF3, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF2, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF4, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF4, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF3, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF5, CRF_LINK_LEFTRIGHT | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_ADDINF5, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_ADDINF4, CTL_INFKIOSK_GRPBOX2, CTL_INFKIOSK_GRPBOX2, CRF_LINK_LEFTRIGHTBOTTOM | crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_INPUT, 0, -1, CTL_INFKIOSK_INFO, 0, crfResizeable);
		SetCtrlResizeParam(CTL_INFKIOSK_INFO, CTL_INFKIOSK_INPUT, -1, 0, 0, crfResizeable);
		LinkCtrlsToDlgBorders(CRF_LINK_RIGHTBOTTOM, STDCTL_OKBUTTON, CTL_INFKIOSK_PRINTLBL, STDCTL_CANCELBUTTON, 0L);
	}
	ResetListWindows();
	ResizeDlgToFullScreen();
	return 1;
}

int InfoKioskDialog::SelectSCard()
{
	int     ok = 1;
	double  dscnt_price = 0.0;
	GetInput();
	SString buf = Input;
	if(PPObjSCard::PreprocessSCardCode(buf) > 0) {
		double   pct_dis = 0.0;
		SString  info, person;
		SCardTbl::Rec  rec;
		PPSCardSerPacket  ser_pack;
		Flags &= ~fWaitOnSCard;
		THROW(ok = SCObj.SearchCode(0, buf, &rec));
		PPSetAddedMsgString(buf);
		THROW_PP(ok > 0, PPERR_SCARDNOTFOUND);
		THROW(SCSerObj.GetPacket(rec.SeriesID, &ser_pack));
		pct_dis = fdiv100i((rec.Flags & SCRDF_INHERITED) ? ser_pack.Rec.PDis : rec.PDis);
		GetPersonName(rec.PersonID, person);
		info.Printf(PPLoadTextS(PPTXT_SCARDINFO, buf), rec.Code, person.cptr(), pct_dis);
		setStaticText(CTL_INFKIOSK_INFO, info);
		if(St.GoodsID)
			if(GObj.CheckFlag(St.GoodsID, GF_NODISCOUNT))
				getCtrlData(CTL_INFKIOSK_PRICE, &dscnt_price);
			else {
				RetailExtrItem item;
				RetailPriceExtractor rpe(Rec.LocID, 0, 0, ZERODATETIME, RTLPF_USEQUOTWTIME);
				if(ser_pack.Rec.QuotKindID_s)
					item.QuotList.Add(ser_pack.Rec.QuotKindID_s, 0);
				THROW(rpe.GetPrice(St.GoodsID, 0, 0.0, &item));
				dscnt_price = item.QuotList.Get(ser_pack.Rec.QuotKindID_s);
				SETIFZ(dscnt_price, (item.Price - fdiv100r(item.Price * pct_dis)));
			}
	}
	else {
		setStaticText(CTL_INFKIOSK_INFO, PPLoadTextS(PPTXT_INPSCARDNUMBER, buf));
		Flags |= fWaitOnSCard;
	}
	CATCH
		PPGetLastErrorMessage(0, buf);
		ok = (setStaticText(CTL_INFKIOSK_INFO, buf), 0);
	ENDCATCH
	setCtrlReal(CTL_INFKIOSK_DSCNTPRICE, dscnt_price);
	ClearInput();
	return ok;
}

int InfoKioskDialog::SelectGoods(SearchParam srch)
{
	int    ok = -1;
	PPID   goods_id = 0;
	PPID   ggrp_id = 0;
	double qtty = 0.0;
	SString buf;
	ExtGoodsSelDialog * dlg = 0;
	if(GetInput() || srch == srchByNone) {
		PPSetAddedMsgString(Input);
		if(oneof2(srch, srchByBarcode, srchBySerial)) {
			Goods2Tbl::Rec grec;
			THROW_PP(GObj.GetGoodsByBarcode(Input, 0, &grec, &qtty, 0) > 0, PPERR_BARCODENFOUND);
			goods_id = grec.ID;
			ggrp_id  = grec.ParentID;
		}
		else if(oneof3(srch, srchByName, srchByPrice, srchByNone)) {
			TIDlgInitData tidi;
			long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags();
			THROW(CheckDialogPtr(&(dlg = new ExtGoodsSelDialog(0, 0, egsd_flags))));
			if(srch == srchByName) {
				SString pattern;
				StrAssocArray goods_list;
				if(Input.Len() >= INSTVSRCH_THRESHOLD)
					pattern.CatChar('!').Cat(Input);
				else
					pattern = Input;
				THROW(GObj.P_Tbl->GetListBySubstring(pattern, &goods_list, -1, true));
				dlg->setSelectionByGoodsList(&goods_list);
			}
			else if(srch == srchByPrice)
				dlg->setSelectionByPrice(Input.ToReal());
			else {
				tidi.GoodsGrpID = SelGoodsGrpID;
				tidi.GoodsID    = St.GoodsID;
				dlg->setDTS(&tidi);
			}
			while(ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&tidi) > 0) {
					goods_id = tidi.GoodsID;
					ggrp_id  = tidi.GoodsGrpID;
					break;
				}
			}
		}
	}
	SelGoodsGrpID = ggrp_id;
	THROW(ok = SetupGoods(goods_id, qtty));
	CATCH
		PPGetLastErrorMessage(0, buf);
		ok = (setStaticText(CTL_INFKIOSK_INFO, buf), 0);
		SetupGoods(0, 0.0);
	ENDCATCH
	delete dlg;
	ClearInput();
	setCtrlReal(CTL_INFKIOSK_DSCNTPRICE, 0.0);
	if(ok > 0)
		setStaticText(CTL_INFKIOSK_INFO, Input);
	return ok;
}

int InfoKioskDialog::SetupGoods(PPID goodsID, double qtty)
{
	int    ok = -1;
	SString image;
	SString word;
	SString code;
	SString buf;
	SString line_buf;
	PPGoodsPacket pack;
	St.Z();
	if(goodsID) {
		int    r = 0;
		THROW(r = GObj.GetPacket(goodsID, &pack, 0));
		if(r > 0) {
			CCheckCore cchk;
			RetailExtrItem item;
			RetailPriceExtractor rpe(Rec.LocID, 0, 0, ZERODATETIME, RTLPF_USEQUOTWTIME);
			THROW(rpe.GetPrice(goodsID, 0, 0.0, &item));
			pack.LinkFiles.Init(PPOBJ_GOODS);
			pack.LinkFiles.Load(goodsID, 0L);
			pack.LinkFiles.At(0, image);
			pack.Codes.GetSingle(0, code);
			cchk.CalcGoodsRest(goodsID, LConfig.OperDate, Rec.LocID, &St.Rest);
			line_buf = pack.ExtString;
			St.GoodsID = goodsID;
			St.Qtty = qtty;
			St.Price = item.Price;
			ok = 1;
		}
	}
	SetupInfo();
	setCtrlString(CTL_INFKIOSK_IMAGE,  image);
	setCtrlString(CTL_INFKIOSK_CODE,   code);
	setCtrlData(CTL_INFKIOSK_GOODS,    pack.Rec.Name);
	setCtrlReal(CTL_INFKIOSK_PRICE,    St.Price);
	setCtrlReal(CTL_INFKIOSK_DSCNTPRICE, 0.0);

	PPGetExtStrData(GDSEXSTR_STORAGE, line_buf, buf);
	setStaticText(CTL_INFKIOSK_ADDINF1, buf);
	PPGetExtStrData(GDSEXSTR_STANDARD, line_buf, buf);
	setStaticText(CTL_INFKIOSK_ADDINF2, buf);
	PPGetExtStrData(GDSEXSTR_INGRED, line_buf, buf);
	setStaticText(CTL_INFKIOSK_ADDINF3, buf);
	PPGetExtStrData(GDSEXSTR_ENERGY, line_buf, buf);
	setStaticText(CTL_INFKIOSK_ADDINF4, buf);
	PPGetExtStrData(GDSEXSTR_USAGE, line_buf, buf);
	setStaticText(CTL_INFKIOSK_ADDINF5, buf);
	SetupLots(goodsID);
	CATCHZOK
	return ok;
}

int InfoKioskDialog::SetupLots(PPID goodsID)
{
	const bool show_lots = LOGIC(Rec.Flags & GIF_SHOWLOTS);
	if(show_lots) {
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_INFKIOSK_LOTS));
		if(p_list) {
			p_list->freeAll();
			if(goodsID) {
				uint   i = 0, lots_count = 5;
				long   lots = 0;
				long   oprno = MAXLONG;
				ReceiptTbl::Rec lot_rec;
				for(LDATE dt = getcurdate_(); i < (uint)lots_count && BillObj->trfr->Rcpt.EnumLastLots(goodsID, Rec.LocID, &dt, &oprno, &lot_rec) > 0; i++) {
					char sub[64];
					QualityCertTbl::Rec qc_rec;
					StringSet ss(SLBColumnDelim);
					ss.add(datefmt(&lot_rec.Dt, DATF_DMY, sub));
					ss.add(datefmt(&lot_rec.Expiry, DATF_DMY, sub));
					QCObj.Search(lot_rec.QCertID, &qc_rec);
					ss.add(qc_rec.Code);
					if(!p_list->addItem(i + 1, ss.getBuf())) {
						PPError(PPERR_SLIB, 0);
						break;
					}
				}
			}
			p_list->focusItem(0);
			p_list->Draw_();
		}
	}
	showCtrl(CTL_INFKIOSK_LOTS, show_lots);
	return 1;
}

int InfoKioskDialog::PrintLabel()
{
	RetailGoodsInfo rgi;
	rgi.Qtty = St.Qtty;
	if(GObj.GetRetailGoodsInfo(St.GoodsID, 0, 0, 0, 0.0, &rgi, PPObjGoods::rgifConcatQttyToCode) > 0)
		BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, Rec.LabelPrinterID, 1);
	return 1;
}

IMPLEMENT_PPFILT_FACTORY(InfoKioskPane); InfoKioskPaneFilt::InfoKioskPaneFilt(): PPBaseFilt(PPFILT_INFOKIOSKPANE, 0, 0)
{
	SetFlatChunk(offsetof(InfoKioskPaneFilt, ReserveStart),
		offsetof(InfoKioskPaneFilt, ReserveEnd) - offsetof(InfoKioskPaneFilt, ReserveStart) + sizeof(ReserveEnd));
	Init(1, 0);
}

int PPObjGoodsInfo::EditInfoKioskPaneFilt(InfoKioskPaneFilt * pData)
{
	int    ok = -1;
	InfoKioskPaneFilt filt;
	RVALUEPTR(filt, pData);
	TDialog * dlg = new TDialog(DLG_INFOKIOSKFLT);
	THROW(CheckDialogPtr(&dlg));
	SetupPPObjCombo(dlg, CTLSEL_INFOKIOSKFLT_NODE, PPOBJ_GOODSINFO, filt.InfoKioskID, OLW_CANINSERT, 0);
	SetupPPObjCombo(dlg, CTLSEL_INFOKIOSKFLT_GRP, PPOBJ_GOODSGROUP, filt.DefaultGrpID, OLW_CANSELUPLEVEL, 0);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		uint   sel = 0;
		filt.InfoKioskID = dlg->getCtrlLong(sel = CTLSEL_INFOKIOSKFLT_NODE);
		if(!filt.InfoKioskID)
			PPErrorByDialog(dlg, sel, PPERR_INFOKIOSKNEEDED);
		else {
			filt.DefaultGrpID = dlg->getCtrlLong(CTLSEL_INFOKIOSKFLT_GRP);
			ASSIGN_PTR(pData, filt);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int ViewGoodsInfo(const InfoKioskPaneFilt * pFilt)
{
	int    ok = -1;
	PPID   goods_info_id = 0;
	PPObjGoodsInfo gi_obj;
	if(pFilt && pFilt->InfoKioskID)
		goods_info_id = pFilt->InfoKioskID;
	else {
		PPObjGoodsInfo gi_obj;
		goods_info_id = gi_obj.GetSingle();
		if(!goods_info_id)
			ListBoxSelDialog::Run(PPOBJ_GOODSINFO, &goods_info_id, 0);
	}
	if(goods_info_id) {
		int    r = 0;
		PPGoodsInfo rec;
		THROW(r = gi_obj.GetPacket(goods_info_id, &rec));
		if(r > 0) {
			THROW(CheckExecAndDestroyDialog(new InfoKioskDialog(&rec, (pFilt ? pFilt->DefaultGrpID : 0)), 0, 0));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
// Генерация товарных чеков
//
class PrcssrCCheckGenerator {
public:
	struct Param {
		Param() : SCardPeriod(5), P_Pan(0), MaxCc(0), MaxTime(0), LineDelay(100), MaxCheckDelay(5)
		{
		}
		uint   SCardPeriod;      // Ориентировочный период чеков, которые пробиваются с дисконтными картами
			// Если SCardPeriod = 5, то примерно каждый пятый чек будет пробит по карте.
		uint   MaxCc;            // Максимальное количество генерируемых чеков. 0 - не ограничено. def=0
		long   MaxTime;          // Максимаальное время генерации чеков (сек). 0 - не ограничено. def=0
		uint   LineDelay;        // Задержка между строками чека (миллисек). def=100
		uint   MaxCheckDelay;    // Максимальная задержка между чеками (сек). Фактическая задержка
			// является равномерной случайной величиной [0..MaxCheckDelay]. def=5
		CheckPaneDialog * P_Pan; // @notowned
	};
	PrcssrCCheckGenerator();
	~PrcssrCCheckGenerator();
	int    Init(const Param * pParam);
	int    Run();
private:
	Param P;
	PPObjGoods GObj;
	PPObjSCard ScObj;
	PPIDArray GoodsList;
	StrAssocArray * P_ScList;
	SRng * P_RngGoods;  // Генератор товаров
	SRng * P_RngSCard;  // Генератор дисконтных карт
	SRng * P_RngDelay;  // Генератор задержки между чеками (сек)
	SRng * P_RngQtty;   // Генератор количества в строке чека
	SRng * P_RngCount;  // Генератор количества строк в чеке
};

PrcssrCCheckGenerator::PrcssrCCheckGenerator() : P_ScList(0), P_RngGoods(0), P_RngSCard(0), P_RngDelay(0), P_RngQtty(0), P_RngCount(0)
{
}

PrcssrCCheckGenerator::~PrcssrCCheckGenerator()
{
	delete P_ScList;
	delete P_RngGoods;
	delete P_RngSCard;
	delete P_RngDelay;
	delete P_RngQtty;
	delete P_RngCount;
}

int PrcssrCCheckGenerator::Init(const Param * pParam)
{
	int    ok = 1;
	GoodsList.clear();
	ZDELETE(P_ScList);
	ZDELETE(P_RngGoods);
	ZDELETE(P_RngSCard);
	ZDELETE(P_RngDelay);
	ZDELETE(P_RngQtty);
	ZDELETE(P_RngCount);
	P = *pParam;
	THROW_INVARG(P.P_Pan);
	{
		PPIniFile ini_file;
		int    enbl = 0;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ENABLECCHECKGENERATOR, &enbl);
		THROW_PP(enbl, PPERR_CCHECKGENDISABLED);
	}
	PPWaitMsg(PPSTR_TEXT, PPTXT_CCGENINIT, 0);
	{
		const  bool dont_sel_passive = LOGIC(GObj.GetConfig().Flags & GCF_DONTSELPASSIVE);
		GoodsFilt gf;
		if(dont_sel_passive)
			gf.Flags |= GoodsFilt::fHidePassive;
		THROW(GoodsIterator::GetListByFilt(&gf, &GoodsList));
		THROW(GoodsList.getCount());
	}
	//
	//
	//
	{
		THROW(P_ScList = ScObj.MakeStrAssocList(0));
	}
	{
		LTIME ct = getcurtime_();
		THROW(P_RngGoods = SRng::CreateInstance(SRng::algMT, 0));
		P_RngGoods->Set(ct.v);
		THROW(P_RngSCard = SRng::CreateInstance(SRng::algMT, 0));
		P_RngSCard->Set(ct.v + 17);
		THROW(P_RngDelay = SRng::CreateInstance(SRng::algMT, 0));
		P_RngDelay->Set(ct.v + 23);
		THROW(P_RngQtty  = SRng::CreateInstance(SRng::algMT, 0));
		P_RngQtty->Set(ct.v + 37);
		THROW(P_RngCount = SRng::CreateInstance(SRng::algMT, 0));
		P_RngCount->Set(ct.v + 71);
	}
	CATCHZOK
	return ok;
}

int PrcssrCCheckGenerator::Run()
{
	int    ok = 1;
	ulong  cc_count = 0; // Количество сгенерированных чеков
	const  uint sc_count = P_ScList ? P_ScList->getCount() : 0;
	SString sc_code, temp_buf;
	LDATETIME tm_start = getcurdatetime_();
	LDATETIME tm_cur = getcurdatetime_();
	LDATETIME tm_limit = tm_start;
	tm_limit.addsec(P.MaxTime);
	PPWaitStart();
	P.P_Pan->EnableBeep(0);
	while((!P.MaxCc || cc_count < P.MaxCc) && (!P.MaxTime || cmp(tm_cur, tm_limit) < 0) && PPCheckUserBreak()) {
		const uint cl_count = (uint)fabs(P_RngCount->GetGaussian(3.0) + 10.0);
		for(uint i = 0; i < cl_count;) {
			const uint goods_pos = P_RngCount->GetUniformInt(GoodsList.getCount());
			if(goods_pos < GoodsList.getCount()) {
				const  PPID goods_id = GoodsList.get(goods_pos);
				Goods2Tbl::Rec goods_rec;
				if(GObj.Fetch(goods_id, &goods_rec) > 0) {
					double qtty = fabs(P_RngQtty->GetGaussian(3.0) + 1.0);
					CPosProcessor::PgsBlock pgsb(round(qtty, (goods_rec.Flags & GF_INTVAL) ? 0 : 3));
					if(pgsb.Qtty > 0.0 && P.P_Pan->SetupNewRow(goods_id, /*qtty, 0, 0*/pgsb) > 0) {
						if(P.LineDelay)
							SDelay(P.LineDelay);
						i++;
					}
				}
			}
		}
		P.P_Pan->AcceptRow();
		if(sc_count) {
			const uint sc_pos = P_RngSCard->GetUniformInt(sc_count * P.SCardPeriod);
			if(sc_pos < sc_count) {
				sc_code = P_ScList->Get(sc_pos).Txt;
				if(sc_code.NotEmptyS())
					P.P_Pan->SetSCard(P_ScList->Get(sc_pos).Txt);
			}
		}
		{
			const CPosProcessor::CcTotal cct = P.P_Pan->CalcTotal();
			//
			// выбор метода платежа и проведение чека
			//
			PPID   cc_id = 0;
			CcAmountList pl;
			pl.Add((cc_count%20 == 0) ? CCAMTTYP_BANK : CCAMTTYP_CASH, cct.Amount);
			P.P_Pan->AcceptCheck(&cc_id, &pl, 0, cct.Amount, CPosProcessor::accmRegular);
			P.P_Pan->ClearCheck();
		}
		if(P.MaxCheckDelay) {
			const uint t = P_RngDelay->GetUniformInt(P.MaxCheckDelay);
			temp_buf.Z().Cat(t);
			PPWaitMsg(PPSTR_TEXT, PPTXT_CCGENCHECKDELAY, temp_buf);
			SDelay(t * 1000);
		}
		cc_count++;
		tm_cur = getcurdatetime_();
		PPWaitMsg(temp_buf.Z().Cat(cc_count));
	}
	P.P_Pan->EnableBeep(1);
	PPWaitStop();
	return ok;
}

int CheckPaneDialog::GenerateChecks()
{
	int	   ok = 1;
	PrcssrCCheckGenerator generator;
	PrcssrCCheckGenerator::Param param;
	param.P_Pan = this;
	param.SCardPeriod = 2;
	param.MaxCc = 3; // @vmiller
	PPWaitStart();
	THROW(generator.Init(&param));
	THROW(generator.Run());
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}
