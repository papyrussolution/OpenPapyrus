// CCHECK.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

static int DoCCheckTextLog()
{
	const PPCommConfig & r_ccfg = CConfig;
	return BIN(/* @temp(..31/12/2010) {*/r_ccfg.Flags & CCFLG_DEBUG || /*}*/ r_ccfg.Flags & CCFLG_LOGCCHECK);
}

//#define DECL_INVARIANT_C()    int InvariantC(SInvariantParam * pInvP) const
//#define IMPL_INVARIANT_C(cls) int cls::InvariantC(SInvariantParam * pInvP) const
//
//
//
CcAmountEntry::CcAmountEntry()
{
	THISZERO();
}

bool FASTCALL CcAmountEntry::IsEq(const CcAmountEntry & rS) const
{
	return (CheckID == rS.CheckID && Type == rS.Type && AddedID == rS.AddedID && Amount == rS.Amount &&
		CurID == rS.CurID && CurAmount == rS.CurAmount);
}

int CcAmountEntry::GetTypeText(SString & rBuf) const
{
	int    ok = 1;
	if(Type == CCAMTTYP_CASH)
		PPLoadString("paymt_cash", rBuf);
	else if(Type == CCAMTTYP_BANK)
		PPLoadString("paymt_bank", rBuf);
	else if(Type == CCAMTTYP_CRDCARD)
		PPLoadString("crdcard", rBuf);
	else {
		rBuf = "Unknown";
		ok = 0;
	}
	return ok;
}
//
//
//
CcAmountList::CcAmountList() : TSVector <CcAmountEntry> ()
{
}

CcAmountList & CcAmountList::Z()
{
	clear();
	return *this;
}

int CcAmountList::GetSign() const
{
	int    sign = 0;
	int    mix = 0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Amount < 0.0) {
			if(!sign || sign < 0)
				sign = -1;
			else {
				mix = 1;
				sign = 0;
				break;
			}
		}
		else if(r_entry.Amount >= 0.0) {
			if(!sign || sign > 0)
				sign = +1;
			else {
				mix = 1;
				sign = 0;
				break;
			}
		}
	}
	return sign;
}

double CcAmountList::ScaleTo(double targetAmount)
{
	double result = 0.0;
	if(targetAmount == 0.0) {
		Z();
	}
	else {
		double total = GetTotal();
		if(total != 0.0) {
			double p = targetAmount / total;
			const uint lc = getCount();
			if(lc == 1) {
				at(0).Amount = targetAmount;
			}
			else {
				double rs = 0.0;
				for(uint i = 0; i < lc; i++) {
					CcAmountEntry & r_entry = at(i);
					r_entry.Amount = R2(r_entry.Amount * p);
					rs += r_entry.Amount;
				}
				//
				// Если в результате округления возникла разница между итоговой и целевой суммами,
				// то компенсируем ее за счет либо кассы, либо банка, либо всего чего угодно (в порядке очереди)
				//
				assert(rs == GetTotal());
				double diff = targetAmount - rs;
				if(diff != 0.0) {
					double c = Get(CCAMTTYP_CASH);
					if(c != 0.0) {
						for(uint i = 0; i < lc; i++) {
							CcAmountEntry & r_entry = at(i);
							if(r_entry.Type == CCAMTTYP_CASH) {
								r_entry.Amount += diff;
								break;
							}
						}
					}
					diff = targetAmount - GetTotal();
					if(diff != 0.0) {
						c = Get(CCAMTTYP_BANK);
						if(c != 0.0) {
							for(uint i = 0; i < lc; i++) {
								CcAmountEntry & r_entry = at(i);
								if(r_entry.Type == CCAMTTYP_BANK) {
									r_entry.Amount += diff;
									break;
								}
							}
						}
						diff = targetAmount - GetTotal();
						if(diff != 0.0) {
							CcAmountEntry & r_entry = at(0);
							r_entry.Amount += diff;
						}
					}
				}
				assert(targetAmount == GetTotal());
				result = GetTotal();
			}
		}
	}
	return result;
}

int CcAmountList::InvertSign()
{
	int    ok = 0;
	const  int  sign = GetSign();
	if(sign < 0) {
		for(uint i = 0; i < getCount(); i++) {
			CcAmountEntry & r_entry = at(i);
			assert(r_entry.Amount < 0.0);
			r_entry.Amount = -r_entry.Amount;
		}
		ok = -1;
	}
	else if(sign > 0) {
		for(uint i = 0; i < getCount(); i++) {
			CcAmountEntry & r_entry = at(i);
			assert(r_entry.Amount >= 0.0);
			r_entry.Amount = -r_entry.Amount;
		}
		ok = +1;
	}
	return ok;
}

int CcAmountList::Search(int type, int32 addedID, uint * pPos) const
{
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Type == type && r_entry.AddedID == addedID) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pPos, 0);
	return 0;
}

int CcAmountList::SearchAddedID(int32 addedID, uint * pPos) const
{
	if(addedID) {
		for(uint i = 0; i < getCount(); i++) {
			const CcAmountEntry & r_entry = at(i);
			if(r_entry.AddedID == addedID) {
				ASSIGN_PTR(pPos, i);
				return 1;
			}
		}
	}
	ASSIGN_PTR(pPos, 0);
	return 0;
}

int CcAmountList::Implement_Set(int type, double amt, int32 addedID, int replace, int dontRemove)
{
	int    ok = 0;
	uint   pos = 0;
	if(Search(type, addedID, &pos)) {
		CcAmountEntry & r_entry = at(pos);
		if(replace) {
			if(amt == 0.0) {
				if(dontRemove) {
					r_entry.Amount = 0.0;
				}
				else {
					atFree(pos);
				}
				ok = 3;
			}
			else {
				r_entry.Amount = amt;
				ok = 1;
			}
		}
		else if(amt != 0.0) {
			r_entry.Amount += amt;
			ok = 1;
		}
		else
			ok = -1;
	}
	else if(amt != 0.0) {
		CcAmountEntry entry;
		entry.Type = type;
		entry.AddedID = addedID;
		entry.Amount = amt;
		insert(&entry);
		ok = 2;
	}
	else
		ok = -1;
	return ok;
}

int    CcAmountList::Set(int type, double amt, int32 addedID) { return Implement_Set(type, amt, addedID, 1, 0); }
int    CcAmountList::Add(int type, double amt, int32 addedID) { return Implement_Set(type, amt, addedID, 0, 0); }
double CcAmountList::Replace(int type, double amt, int32 addedID, int r1, int r2) { return Implement_Replace(type, amt, addedID, 0, r1, r2); }
double CcAmountList::ReplaceDontRemove(int type, double amt, int32 addedID, int r1, int r2) { return Implement_Replace(type, amt, addedID, 1, r1, r2); }

double CcAmountList::Implement_Replace(int type, double amt, int32 addedID, int dontRemove, int r1, int r2)
{
	assert(type != 0);
	assert(type != r1);
	assert(type != r2);
	const int list_sign = GetSign();
	const int amt_sign = (amt < 0.0) ? -1 : +1;
	double total = GetTotal();
	double ex_amt = Get(type, addedID);
	double result = ex_amt; // @v10.3.4 (избыточный вызов) Get(type, addedID)-->ex_amt 
	int    do_invert_sign = 0;
	if(list_sign == amt_sign) {
		if(list_sign < 0) {
			do_invert_sign = InvertSign();
			assert(do_invert_sign);

			amt = -amt;
			total = GetTotal();
			ex_amt = Get(type, addedID);
			result = ex_amt; // @v10.3.4 (избыточный вызов) Get(type, addedID)-->ex_amt 
		}
		SETMIN(amt, total);
		const double p1 = r1 ? Get(r1) : 0.0;
		const double p2 = r2 ? Get(r2) : 0.0;
		if(amt > ex_amt) {
			if(result < amt && p1 > 0.0) {
				double _addendum = MIN(p1, (amt - result));
				result += _addendum;
				uint c = getCount();
				if(c) do {
					CcAmountEntry & r_entry = at(--c);
					if(r_entry.Type == r1) {
						if(r_entry.Amount <= _addendum) {
							atFree(c);
							_addendum -= r_entry.Amount;
						}
						else {
							r_entry.Amount -= _addendum;
							_addendum = 0.0;
						}
					}
				} while(c && _addendum > 0.0);
			}
			if(result < amt && p2 > 0.0) {
				double _addendum = MIN(p2, (amt - result));
				result += _addendum;
				uint c = getCount();
				if(c) do {
					CcAmountEntry & r_entry = at(--c);
					if(r_entry.Type == r2) {
						if(r_entry.Amount <= _addendum) {
							atFree(c);
							_addendum -= r_entry.Amount;
						}
						else {
							r_entry.Amount -= _addendum;
							_addendum = 0.0;
						}
					}
				} while(c && _addendum > 0.0);
			}
		}
		else if(amt < ex_amt) {
			const double up_diff_p1 = total - p1;
			const double up_diff_p2 = total - p2;
			if(r1 && result > amt && up_diff_p1 > 0.0) {
				double _sub = MIN((result - amt), up_diff_p1);
				uint c = getCount();
				if(c) do {
					CcAmountEntry & r_entry = at(--c);
					if(r_entry.Type == r1) {
						r_entry.Amount += _sub;
						result -= _sub;
						_sub = 0.0;
					}
				} while(c && _sub > 0.0);
				if(_sub > 0.0) {
					Add(r1, _sub, 0);
					result -= _sub;
					_sub = 0.0;
				}
			}
			if(r2 && result > amt && up_diff_p2 > 0.0) {
				double _sub = MIN((result - amt), up_diff_p2);
				uint c = getCount();
				if(c) do {
					CcAmountEntry & r_entry = at(--c);
					if(r_entry.Type == r2) {
						r_entry.Amount += _sub;
						result -= _sub;
						_sub = 0.0;
					}
				} while(c && _sub > 0.0);
				if(_sub > 0.0) {
					Add(r2, _sub, 0);
					result -= _sub;
					_sub = 0.0;
				}
			}
		}
		else { // amt == ex_amt
		}
		Implement_Set(type, result, addedID, 1, dontRemove);
		if(do_invert_sign) {
			InvertSign();
			total = -total;
			result = -result;
		}
		assert(feqeps(GetTotal(), total, 1E-7)); // @v10.9.0 feqeps
	}
	return result;
}

int CcAmountList::Normalize()
{
	int    ok = -1;
	uint   c = getCount();
	if(c) do {
		if(feqeps(at(--c).Amount, 0.0, 1E-7)) { // @v10.9.0 feqeps
			atFree(c);
			ok = 1;
		}
	} while(c);
	return ok;
}

double CcAmountList::GetTotal() const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		result += r_entry.Amount;
	}
	return result;
}

double FASTCALL CcAmountList::Get(int type) const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Type == type)
			result += r_entry.Amount;
	}
	return result;
}

double CcAmountList::Get(int type, LongArray * pAddedIdList) const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Type == type) {
			if(pAddedIdList && r_entry.AddedID)
				pAddedIdList->addUnique(r_entry.AddedID);
			result += r_entry.Amount;
		}
	}
	return result;
}

double CcAmountList::Get(int type, int32 addedID) const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Type == type && r_entry.AddedID == addedID)
			result += r_entry.Amount;
	}
	return result;
}

double CcAmountList::GetBonusAmount(PPObjSCard * pScObj) const
{
	PPObjSCard * p_sc_obj = pScObj;
	double result = 0;
	for(uint i = 0; i < getCount(); i++) {
		const CcAmountEntry & r_entry = at(i);
		if(r_entry.Type == CCAMTTYP_CRDCARD && r_entry.AddedID) {
			if(SETIFZ(p_sc_obj, new PPObjSCard)) {
				const int scst = p_sc_obj->GetCardType(r_entry.AddedID);
				if(scst == scstBonus)
					result += r_entry.Amount;
			}
		}
	}
	if(p_sc_obj != pScObj) {
		ZDELETE(p_sc_obj);
	}
	return result;
}
//
//
//
CCheckItem::CCheckItem()
{
	THISZERO();
}

CCheckItem & CCheckItem::Z()
{
	THISZERO();
	return *this;
}

CCheckItem & FASTCALL CCheckItem::operator = (const CCheckItem & rS)
{
	if(&rS != this)
		memcpy(this, &rS, sizeof(*this));
	return *this;
}

CCheckItem & FASTCALL CCheckItem::operator = (const CCheckLineTbl::Rec & rS)
{
	GoodsID  = rS.GoodsID;
	Quantity = rS.Quantity;
	Price    = intmnytodbl(rS.Price);
	Discount = rS.Dscnt;
	Flags    = (rS.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? cifIsPrinted : 0;
	Division = (rS.DivID >= CHECK_LINE_IS_PRINTED_BIAS) ? rS.DivID - CHECK_LINE_IS_PRINTED_BIAS : rS.DivID;
	return *this;
}

CCheckItem & FASTCALL CCheckItem::operator = (const CCheckLineExtTbl::Rec & rS)
{
	STRNSCPY(Serial, rS.Serial);
	Queue = rS.Queue;
	SETFLAG(Flags, cifGrouped,       rS.Flags & CCheckPacket::LineExt::fGroup);
	SETFLAG(Flags, cifModifier,      rS.Flags & CCheckPacket::LineExt::fModifier);
	SETFLAG(Flags, cifPartOfComplex, rS.Flags & CCheckPacket::LineExt::fPartOfComplex);
	SETFLAG(Flags, cifQuotedByGift,  rS.Flags & CCheckPacket::LineExt::fQuotedByGift);
	SETFLAG(Flags, cifFixedPrice,    rS.Flags & CCheckPacket::LineExt::fFixedPrice);
	return *this;
}

void CCheckItem::GetRec(CCheckLineTbl::Rec & rRec, int ret) const
{
	MEMSZERO(rRec);
	rRec.DivID = Division;
	if(Flags & cifIsPrinted)
		rRec.DivID += CHECK_LINE_IS_PRINTED_BIAS;
	rRec.GoodsID  = GoodsID;
	rRec.Quantity = ret ? -fabs(Quantity) : fabs(Quantity);
	if(Flags & cifQuotedByGift && BeforeGiftPrice > 0.0) {
		rRec.Price = dbltointmny(BeforeGiftPrice);
		rRec.Dscnt = (BeforeGiftPrice - Price);
	}
	else {
		const double amount = R5(Price - Discount);
		rRec.Price = dbltointmny(Price);
		rRec.Dscnt = Discount;
		const double tamt = R5(intmnytodbl(rRec.Price) - rRec.Dscnt);
		if(tamt != amount /* @v8.2.3 && (CConfig.Flags2 & CCFLG2_ADJCPANCCLINETRANS)*/)
			rRec.Dscnt += (tamt - amount);
	}
}

int CCheckItem::GetRec(CCheckLineExtTbl::Rec & rRec) const
{
	int    ok = -1;
	MEMSZERO(rRec);
	if(Queue > 0 || (Flags & (cifGrouped|cifModifier|cifPartOfComplex|cifQuotedByGift|cifFixedPrice)) || Serial[0]) {
		rRec.Queue = Queue;
		SETFLAG(rRec.Flags, CCheckPacket::LineExt::fGroup,         Flags & cifGrouped);
		SETFLAG(rRec.Flags, CCheckPacket::LineExt::fModifier,      Flags & cifModifier);
		SETFLAG(rRec.Flags, CCheckPacket::LineExt::fPartOfComplex, Flags & cifPartOfComplex);
		SETFLAG(rRec.Flags, CCheckPacket::LineExt::fQuotedByGift,  Flags & cifQuotedByGift);
		SETFLAG(rRec.Flags, CCheckPacket::LineExt::fFixedPrice,    Flags & cifFixedPrice);
		STRNSCPY(rRec.Serial, Serial);
		strip(rRec.Serial);
		ok = 1;
	}
	return ok;
}

double CCheckItem::NetPrice() const { return (Price - Discount); }
double CCheckItem::GetAmount() const { return R2((Price - Discount) * Quantity); }

int CCheckItem::SetupGiftQuot(double quot, int forceZero)
{
	if(quot > 0.0 || (quot == 0.0 && forceZero)) {
		BeforeGiftPrice = Price;
		Price = quot;
		Discount = 0.0;
		Flags |= cifQuotedByGift;
		return 1;
	}
	else
		return 0;
}

int CCheckItem::ResetGiftQuot()
{
	if(Flags & cifQuotedByGift) {
		Price = BeforeGiftPrice;
		BeforeGiftPrice = 0.0;
		Discount = 0.0;
		Flags &= ~cifQuotedByGift;
		return 1;
	}
	else
		return -1;
}

int CCheckItem::CanMerge(const CCheckPacket * pPack, const CCheckItem & rItem) const
{
	return BIN(rItem.Price == Price && rItem.Discount == Discount && !(rItem.Flags & cifGiftDiscount) &&
		!(rItem.Flags & cifModifier) && !(Flags & cifModifier) &&
		(!(pPack->Rec.Flags & CCHKF_SUSPENDED) || (rItem.Flags & cifIsPrinted) == (Flags & cifIsPrinted)) &&
		rItem.Division == Division && rItem.Queue == Queue && sstreq(rItem.Serial, Serial) && 
		sstreq(rItem.EgaisMark, EgaisMark) && sstreq(rItem.RemoteProcessingTa, RemoteProcessingTa) && sstreq(rItem.ChZnMark, ChZnMark));
}

int CCheckItem::SplitByQtty(double restQtty, CCheckItem & rNewItem)
{
	int    ok = 1;
	if(Quantity > restQtty && restQtty > 0.0 && !(Flags & cifGift)) {
		rNewItem.Z();
		rNewItem = *this;
		rNewItem.GiftID = 0;
		rNewItem.BeforeGiftPrice = 0.0;
		rNewItem.Flags &= ~(cifModifier | cifGrouped);
		const double qtty = Quantity;
		const double phqtty = PhQtty;
		Quantity = qtty - restQtty;
		rNewItem.Quantity = restQtty;
		if(phqtty != 0.0) {
			PhQtty = phqtty / (1.0 + restQtty / Quantity);
			rNewItem.PhQtty = phqtty / (1.0 + Quantity / restQtty);
		}
		assert((Quantity + rNewItem.Quantity) == qtty);
		assert((PhQtty + rNewItem.PhQtty) == phqtty);
	}
	else
		ok = 0;
	return ok;
}
//
// CCheckPacket
//
CCheckPacket::LineExt::LineExt()
{
	THISZERO();
}

bool CCheckPacket::LineExt::IsEmpty() const
{
	return (Queue <= 0 && !(Flags & (fGroup|fModifier|fPartOfComplex|fQuotedByGift|fFixedPrice))); // @v10.1.7 fFixedPrice
}

CCheckPacket::PreprocessChZnCodeResult::PreprocessChZnCodeResult() : LineIdx(0), CheckResult(0), Reason(0), ProcessingResult(0), ProcessingCode(0), Status(0)
{
}

CCheckPacket::PreprocessChZnCodeResult & CCheckPacket::PreprocessChZnCodeResult::Z()
{
	THISZERO();
	return *this;
}
//
//
//
IMPL_INVARIANT_C(CCheckPacket)
{
	S_INVARIANT_PROLOG(pInvP);
	//
	// Доплата по кредитной карте ненулевая только при установленной доп кредитной карте
	//
	// @v9.0.4 S_ASSERT_P(!Ext.AddCrdCardPaym || Ext.AddCrdCardID, pInvP);
	//
	// Наличие доплаты по кредитной карте должно быть помечено флагом CCHKF_ADDINCORPCRD
	//
	// @v9.0.4 S_ASSERT_P(!Ext.AddCrdCardPaym || Rec.Flags & CCHKF_ADDINCORPCRD, pInvP);
	//
	// Дополнительная кредитная карта может быть установлена только тогда, когда в чеке
	// установлена основная кредитная карта.
	//
	// @v9.0.4 S_ASSERT_P(!Ext.AddCrdCardID || (Rec.SCardID && Rec.Flags & CCHKF_INCORPCRD), pInvP);
	//
	// Флаг CCHKF_INCORPCRD не может быть установлен если в чеке нет ссылки на карту.
	//
	// @v10.4.1 (кредитная карта может фигурировать в списке платежей) S_ASSERT_P(!(Rec.Flags & CCHKF_INCORPCRD) || Rec.SCardID, pInvP);
	{
		//
		// Фискальная и нефискальная суммы чека должны в итоге давать полную сумму чеку.
		//
		double amt = fabs(MONEYTOLDBL(Rec.Amount));
		double fiscal = 0.0, nonfiscal = 0.0;
		S_ASSERT_P(HasNonFiscalAmount(&fiscal, &nonfiscal) || nonfiscal == 0.0, pInvP);
		S_ASSERT_P(R6(amt) == R6(fiscal+nonfiscal), pInvP);
	}
	S_ASSERT_P(!HasExt() || Rec.Flags & CCHKF_EXT, pInvP);
	//
	// Чек заказа стола обязательно должен содержать номер заказываемого стола
	//
	S_ASSERT_P(!(Rec.Flags & CCHKF_ORDER) || Ext.TableNo, pInvP);
	//
	// Количество гостей может быть указано только при заданном номере стола
	//
	S_ASSERT_P(!Ext.GuestCount || Ext.TableNo, pInvP);
	//
	// Флаг CCHKF_CLOSEDORDER может быть установлен только у чека заказа стола либо у чека с доставкой
	//
	S_ASSERT_P(!(Rec.Flags & CCHKF_CLOSEDORDER) || (Rec.Flags & (CCHKF_ORDER|CCHKF_DELIVERY)), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

CCheckPacket::CCheckPacket() : UpdFlags(0), PrintPtt(pttUndef), PctDis(0), Discount(0.0), _Cash(0.0), _OrdPrepay(0.0), P_DlvrAddr(0), P_Stirb(0), P_PpChZnCodeResultList(0)
{
}

CCheckPacket::CCheckPacket(const CCheckPacket & rS) : P_DlvrAddr(0), P_Stirb(0), P_PpChZnCodeResultList(0)
{
	Copy(rS);
}

CCheckPacket & FASTCALL CCheckPacket::operator = (const CCheckPacket & rS)
{
	Copy(rS);
	return *this;
}

CCheckPacket::~CCheckPacket()
{
	ZDELETE(P_DlvrAddr);
	delete static_cast<SCardSpecialTreatment::IdentifyReplyBlock *>(P_Stirb); // @v10.9.0
	ZDELETE(P_PpChZnCodeResultList); // @v11.1.11
}

CCheckPacket & CCheckPacket::Z()
{
	UpdFlags = 0;
	PrintPtt = pttUndef; // @v11.1.5
	PctDis   = 0;
	Discount = 0.0;
	_Cash    = 0.0;
	_OrdPrepay  = 0.0;
	MEMSZERO(Ext);
	MEMSZERO(Rec);
	// @v10.9.0 UhttScHash.Z();
	Items_.clear();
	ExtList.clear();
	LnTextList.Z();
	ZDELETE(P_DlvrAddr);
	delete static_cast<SCardSpecialTreatment::IdentifyReplyBlock *>(P_Stirb); // @v10.9.0
	P_Stirb = 0; // @v10.9.0
	ZDELETE(P_PpChZnCodeResultList); // @v11.1.11
	CcAl.clear();
	return *this;
}

int CCheckPacket::Copy(const CCheckPacket & rS)
{
	int    ok = 1;
	PPExtStrContainer::Copy(rS);
	UpdFlags = rS.UpdFlags;
	PrintPtt = rS.PrintPtt; // @v11.1.5
	PctDis = rS.PctDis;
	Discount = rS.Discount;
	_Cash = rS._Cash;
	_OrdPrepay = rS._OrdPrepay;
	Ext = rS.Ext;
	Rec = rS.Rec;
	Items_ = rS.Items_;
	ExtList = rS.ExtList;
	CcAl = rS.CcAl;
	LnTextList = rS.LnTextList;
	if(rS.P_DlvrAddr) {
		SETIFZ(P_DlvrAddr, new LocationTbl::Rec);
		ASSIGN_PTR(P_DlvrAddr, *rS.P_DlvrAddr);
	}
	else {
		ZDELETE(P_DlvrAddr);
	}
	P_Stirb = 0;
	// @v11.1.11 {
	if(rS.P_PpChZnCodeResultList) {
		if(P_PpChZnCodeResultList)
			*P_PpChZnCodeResultList = *rS.P_PpChZnCodeResultList;
		else
			P_PpChZnCodeResultList = new TSVector <PreprocessChZnCodeResult>(*rS.P_PpChZnCodeResultList);
	}
	else if(P_PpChZnCodeResultList) {
		ZDELETE(P_PpChZnCodeResultList);
	}
	// } @v11.1.11 
	return ok;
}

int CCheckPacket::IsEq(const CCheckPacket & rS, long options) const
{
	/*
	struct Rec {
		int32  ID;
		int32  Code;
		int32  CashID;
		int32  UserID;
		int32  SessID;
		LDATE  Dt;
		LTIME  Tm;
		int32  Flags;
		char   Amount[8];  // money[8.2]
		char   Discount[8]; // money[8.2]
		int32  SCardID;
	} data;
	*/
	#define CMP_FLD(f) if(Rec.f != rS.Rec.f) return 0
	if(options == 0) {
		CMP_FLD(ID);
		CMP_FLD(Code);
		CMP_FLD(CashID);
		CMP_FLD(UserID);
		CMP_FLD(SessID);
		CMP_FLD(Dt);
		CMP_FLD(Tm);
	}
	CMP_FLD(Flags);
	CMP_FLD(SCardID);
	#undef CMP_FLD
	if(MONEYTOLDBL(Rec.Amount) != MONEYTOLDBL(rS.Rec.Amount))
		return 0;
	if(MONEYTOLDBL(Rec.Discount) != MONEYTOLDBL(rS.Rec.Discount))
		return 0;
	#define CMP_FLD(f) if(Ext.f != rS.Ext.f) return 0
	CMP_FLD(SalerID);
	CMP_FLD(TableNo);
	CMP_FLD(AddPaym_unused); // @v9.0.4 _unused
	CMP_FLD(GuestCount);
	CMP_FLD(AddrID);
	CMP_FLD(AddCrdCardID_unused); // @v9.0.4 _unused
	// @v10.6.8 CMP_FLD(AddCrdCardPaym_unused); // @v9.0.4 _unused
	CMP_FLD(CreationUserID); // @v10.6.8
	CMP_FLD(LinkCheckID);
	CMP_FLD(StartOrdDtm);
	CMP_FLD(EndOrdDtm);
	CMP_FLD(CreationDtm);
	#undef CMP_FLD
	if(!sstreq(Ext.Memo, rS.Ext.Memo))
		return 0;
	return 1;
}

int CCheckPacket::HasExt() const
{
	// @v10.6.8 Ext.CreationUserID
	return BIN(Ext.TableNo || Ext.SalerID || Ext.GuestCount || Ext.LinkCheckID ||
		!!Ext.StartOrdDtm || !!Ext.EndOrdDtm || !!Ext.CreationDtm || Ext.CreationUserID || Ext.Memo[0] || Ext.AddrID);
}

uint  CCheckPacket::GetCount() const { return Items_.getCount(); }
const CCheckLineTbl::Rec & FASTCALL CCheckPacket::GetLine(uint pos) const { return Items_.at(pos); }

int CCheckPacket::EnumLines(uint * pPos, CCheckLineTbl::Rec * pItem, SString * pSerial) const
{
	CCheckLineTbl::Rec * p_item;
	if(Items_.enumItems(pPos, (void **)&p_item)) {
		ASSIGN_PTR(pItem, *p_item);
		if(pSerial) {
			// *Pos была увеличена на 1 функцией enumItems()
			GetLineTextExt(*pPos, CCheckPacket::lnextSerial, *pSerial);
		}
		return 1;
	}
	else
		return 0;
}

int CCheckPacket::EnumLines(uint * pPos, CCheckItem * pItem) const
{
	CCheckLineTbl::Rec * p_item;
	if(Items_.enumItems(pPos, (void **)&p_item)) {
		if(pItem) {
			const  int _pos = *pPos; // *Pos была увеличена на 1 функцией enumItems()
			uint   ep = 0;
			SString serial;
			pItem->Z() = *p_item;
			GetLineTextExt(_pos, CCheckPacket::lnextSerial, serial);
			serial.CopyTo(pItem->Serial, sizeof(pItem->Serial));
			GetLineTextExt(_pos, CCheckPacket::lnextEgaisMark, serial);
			serial.CopyTo(pItem->EgaisMark, sizeof(pItem->EgaisMark));
			GetLineTextExt(_pos, CCheckPacket::lnextRemoteProcessingTa, serial); // @v10.1.6
			serial.CopyTo(pItem->RemoteProcessingTa, sizeof(pItem->RemoteProcessingTa)); // @v10.1.6
			GetLineTextExt(_pos, CCheckPacket::lnextChZnGtin, serial); // @v10.4.12
			serial.CopyTo(pItem->ChZnGtin, sizeof(pItem->ChZnGtin)); // @v10.4.12
			GetLineTextExt(_pos, CCheckPacket::lnextChZnSerial, serial); // @v10.4.12
			serial.CopyTo(pItem->ChZnSerial, sizeof(pItem->ChZnSerial)); // @v10.4.12
			GetLineTextExt(_pos, CCheckPacket::lnextChZnMark, serial); // @v10.6.9
			serial.CopyTo(pItem->ChZnMark, sizeof(pItem->ChZnMark)); // @v10.6.9
			if(ExtList.lsearch(&_pos, &ep, CMPF_LONG)) {
				const LineExt & r_le = ExtList.at(ep);
				pItem->Queue = r_le.Queue;
				SETFLAG(pItem->Flags, cifGrouped,  r_le.Flags & r_le.fGroup);
				SETFLAG(pItem->Flags, cifModifier, r_le.Flags & r_le.fModifier);
				SETFLAG(pItem->Flags, cifPartOfComplex, r_le.Flags & r_le.fPartOfComplex);
				SETFLAG(pItem->Flags, cifQuotedByGift,  r_le.Flags & r_le.fQuotedByGift);
				SETFLAG(pItem->Flags, cifFixedPrice,    r_le.Flags & r_le.fFixedPrice); // @v10.1.7
			}
		}
		return 1;
	}
	else
		return 0;
}

void CCheckPacket::InitLineIteration()
{
	Items_.setPointer(0);
}

int CCheckPacket::NextLineIteration(CCheckLineTbl::Rec * pItem, SString * pSerial)
{
	uint   pos = Items_.getPointer();
	int    ok = EnumLines(&pos, pItem, pSerial);
	if(ok)
		Items_.setPointer(pos);
	return ok;
}

int CCheckPacket::RemoveLine_(uint pos)
{
	if(pos < Items_.getCount()) {
		Items_.atFree(pos);
		uint   idx = pos+1;
		uint   _p = 0;
		LnTextList.Remove(idx);
		if(ExtList.lsearch(&idx, &_p, CMPF_LONG))
			ExtList.atFree(_p);
		return 1;
	}
	else
		return 0;
}

int CCheckPacket::CopyLines(const CCheckPacket & rS)
{
	int    ok = 1;
	THROW_SL(Items_.copy(rS.Items_));
	THROW_SL(LnTextList.Copy(rS.LnTextList));
	THROW_SL(ExtList.copy(rS.ExtList));
	CATCHZOK
	return ok;
}

int CCheckPacket::SearchLine(int rByCheck, uint * pPos) const
{
	int16  rbc = static_cast<int16>(rByCheck);
	return Items_.lsearch(&rbc, pPos, PTR_CMPFUNC(int16), offsetof(CCheckLineTbl::Rec, RByCheck));
}

static const int ccpack_textext_ident_list[] = { 
	CCheckPacket::extssMemo, 
	CCheckPacket::extssSign, 
	CCheckPacket::extssEgaisUrl, 
	CCheckPacket::extssRemoteProcessingTa,
	CCheckPacket::extssChZnProcessingTag, // @v11.0.1
	CCheckPacket::extssBuyerINN,          // @v11.0.4 ИНН покупателя (при формировании чека по документу)
	CCheckPacket::extssBuyerName,         // @v11.0.4 Наименование покупателя (при формировании чека по документу)
	CCheckPacket::extssBuyerPhone,        // @v11.0.4 Телефон покупателя //
	CCheckPacket::extssBuyerEMail         // @v11.3.6 
};

int CCheckPacket::PackTextExt(SString & rResult) const
{
	rResult.Z();
	int    ok = -1;
	SString temp_buf;
	for(uint i = 0; i < SIZEOFARRAY(ccpack_textext_ident_list); i++) {
		const int fld_id = ccpack_textext_ident_list[i];
		assert(fld_id > 0 && fld_id < 100);
		if(fld_id > 0 && fld_id < 100) {
			GetExtStrData(fld_id, temp_buf);
			if(temp_buf.NotEmptyS()) {
				PPPutExtStrData(fld_id, rResult, temp_buf);
				ok = 1;
			}
		}
	}
	if(Helper_PackLineTextExt(rResult) > 0)
		ok = 1;
	return ok;
}

int CCheckPacket::UnpackTextExt(const SString & rBuf)
{
	return Helper_UnpackTextExt(rBuf, this, &this->LnTextList);
}

int CCheckPacket::Helper_PackLineTextExt(SString & rResult) const
{
	int    ok = -1;
	SString line_buf;
	SString temp_buf;
    for(uint i = 0; i < LnTextList.getCount(); i++) {
        StrAssocArray::Item item = LnTextList.Get(i);
		PPExtStringStorage ess;
		line_buf = item.Txt;
		int    fld_id = 0;
		for(uint p = 0; ess.Enum(line_buf, &p, &fld_id, temp_buf) > 0;) {
			if(temp_buf.NotEmptyS()) {
				if(fld_id > 0 && fld_id < 100) {
					PPPutExtStrData(/*(i+1)*/item.Id * 100 + fld_id, rResult, temp_buf); // @v9.9.0 @fix (i+1)-->item.Id
					ok = 1;
				}
				else {
					; // @error
				}
			}
		}
    }
	return ok;
}

/* @v10.3.9 int CCheckPacket::PackLineTextExt(SString & rResult) const
{
	rResult.Z();
	return Helper_PackLineTextExt(rResult);
}*/

/*static*/int CCheckPacket::Helper_UnpackTextExt(const SString & rBuf, PPExtStrContainer * pSc, StrAssocArray * pList)
{
	int   ok = -1;
	CALLPTRMEMB(pSc, SetBuffer(0));
	CALLPTRMEMB(pList, Z());
	SString temp_buf;
	SString line_buf;
	PPExtStringStorage ess;
	int    fld_id = 0;
	for(uint p = 0; ess.Enum(rBuf, &p, &fld_id, temp_buf) > 0;) {
		if(temp_buf.NotEmptyS()) {
			if(fld_id > 100) {
				if(pList) {
					const int row_no = fld_id / 100;
					const int fld_no = fld_id % 100;
					Helper_SetLineTextExt(row_no, fld_no, *pList, temp_buf);
				}
				ok = 1;
			}
			else {
				for(uint j = 0; j < SIZEOFARRAY(ccpack_textext_ident_list); j++) {
					if(fld_id == ccpack_textext_ident_list[j]) {
						CALLPTRMEMB(pSc, PutExtStrData(fld_id, temp_buf));
						ok = 1;
						break;
					}
				}
			}
			/*else if(oneof5(fld_id, CCheckPacket::extssMemo, CCheckPacket::extssSign, CCheckPacket::extssEgaisUrl, 
				CCheckPacket::extssRemoteProcessingTa, CCheckPacket::extssChZnProcessingTag)) {
				CALLPTRMEMB(pSc, PutExtStrData(fld_id, temp_buf));
				ok = 1;
			}*/
		}
	}
	return ok;
}

/* @v10.3.9 (replaced with Helper_UnpackTextExt) int CCheckPacket::Helper_UnpackLineTextExt(const SString & rBuf, StrAssocArray & rList)
{
	int   ok = 1;
	rList.Z();
	SString temp_buf;
	SString line_buf;
	PPExtStringStorage ess;
	int    fld_id = 0;
	for(uint p = 0; ess.Enum(rBuf, &p, &fld_id, temp_buf) > 0;) {
		if(temp_buf.NotEmptyS()) {
			if(fld_id > 100) {
				const int row_no = fld_id / 100;
				const int fld_no = fld_id % 100;
				Helper_SetLineTextExt(row_no, fld_no, rList, temp_buf);
			}
			else {
				; // @error
			}
		}
	}
	return ok;
}*/

/*static*/int CCheckPacket::Helper_SetLineTextExt(int pos /* 1.. */, int lnextId, StrAssocArray & rList, const char * pText)
{
	int    ok = 1;
	SString common_buf;
	rList.GetText(pos, common_buf);
	PPPutExtStrData(lnextId, common_buf, pText);
	common_buf.Strip();
	rList.Add(pos, common_buf, 1);
	return ok;
}

/*static*/int CCheckPacket::Helper_GetLineTextExt(int pos /* 1.. */, int lnextId, const StrAssocArray & rList, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	SString & r_common_buf = SLS.AcquireRvlStr();
	rList.GetText(pos, r_common_buf);
	if(r_common_buf.NotEmpty())
		ok = PPGetExtStrData(lnextId, r_common_buf, rBuf);
	return ok;
}

/* @v10.3.9 int CCheckPacket::UnpackLineTextExt(const SString & rBuf) 
{ 
	// @v10.3.9 return Helper_UnpackLineTextExt(rBuf, LnTextList); 
	return Helper_UnpackTextExt(rBuf, 0, &LnTextList); // @v10.3.9 
}*/
int CCheckPacket::SetLineTextExt(int pos /* 1.. */, int lnextId, const char * pText) { return Helper_SetLineTextExt(pos, lnextId, LnTextList, pText); }
int CCheckPacket::GetLineTextExt(int pos /* 1.. */, int lnextId, SString & rBuf) const { return Helper_GetLineTextExt(pos, lnextId, LnTextList, rBuf); }

int CCheckPacket::SetLineExt(int pos /* 1.. */, const LineExt & rExt)
{
	int    ok = -1, found = 0;
	uint   _p = 0;
	if(ExtList.lsearch(&pos, &_p, CMPF_LONG))
		found = 1;
	if(!rExt.IsEmpty()) {
		if(found) {
			LineExt & r_le = ExtList.at(_p);
			r_le.Queue = rExt.Queue;
			SETFLAG(r_le.Flags, r_le.fGroup,         rExt.Flags & rExt.fGroup);
			SETFLAG(r_le.Flags, r_le.fModifier,      rExt.Flags & rExt.fModifier);
			SETFLAG(r_le.Flags, r_le.fPartOfComplex, rExt.Flags & rExt.fPartOfComplex);
			SETFLAG(r_le.Flags, r_le.fQuotedByGift,  rExt.Flags & rExt.fQuotedByGift);
			SETFLAG(r_le.Flags, r_le.fFixedPrice,    rExt.Flags & rExt.fFixedPrice); // @v10.1.7
			ok = 1;
		}
		else {
			LineExt le;
			le.ItemIdx = pos;
			le.Queue = rExt.Queue;
			SETFLAG(le.Flags, le.fGroup,         rExt.Flags & rExt.fGroup);
			SETFLAG(le.Flags, le.fModifier,      rExt.Flags & rExt.fModifier);
			SETFLAG(le.Flags, le.fPartOfComplex, rExt.Flags & rExt.fPartOfComplex);
			SETFLAG(le.Flags, le.fQuotedByGift,  rExt.Flags & rExt.fQuotedByGift);
			SETFLAG(le.Flags, le.fFixedPrice,    rExt.Flags & rExt.fFixedPrice); // @v10.1.7
			THROW_SL(ExtList.insert(&le));
			ok = 2;
		}
	}
	else if(found) {
		ExtList.atFree(_p);
		ok = 3;
	}
	CATCHZOK
	return ok;
}

int CCheckPacket::SetSCardSpecialTreatmentIdentifyReplyBlock(const void * pStirb)
{
	const SCardSpecialTreatment::IdentifyReplyBlock * p_stirb_outer = static_cast<const SCardSpecialTreatment::IdentifyReplyBlock *>(pStirb);
	SCardSpecialTreatment::IdentifyReplyBlock * p_stirb_inner = static_cast<SCardSpecialTreatment::IdentifyReplyBlock *>(P_Stirb);
	if(p_stirb_outer) {
		if(p_stirb_outer->SpecialTreatment) {
			SETIFZ(p_stirb_inner, new SCardSpecialTreatment::IdentifyReplyBlock);
			*p_stirb_inner = *p_stirb_outer;
		}
		else if(!isempty(p_stirb_outer->Hash)) {
			SETIFZ(p_stirb_inner, new SCardSpecialTreatment::IdentifyReplyBlock);
			STRNSCPY(p_stirb_inner->Hash, p_stirb_outer->Hash);
		}
		else {
			ZDELETE(p_stirb_inner);
		}
	}
	else {
		ZDELETE(p_stirb_inner);
	}
	P_Stirb = p_stirb_inner;
	return 1;
}

int CCheckPacket::GetSCardSpecialTreatmentIdentifyReplyBlock(void * pStirb) const
{
	if(P_Stirb) {
		ASSIGN_PTR(static_cast<SCardSpecialTreatment::IdentifyReplyBlock *>(pStirb), *static_cast<const SCardSpecialTreatment::IdentifyReplyBlock *>(P_Stirb));
		return 1;
	}
	else {
		if(pStirb)
			static_cast<SCardSpecialTreatment::IdentifyReplyBlock *>(pStirb)->Z();
		return 0;
	}
}

int CCheckPacket::_SetLineGoodsID(uint lineIdx/*0..*/, PPID goodsID)
{
	if(lineIdx < Items_.getCount()) {
		Items_.at(lineIdx).GoodsID = goodsID;
		return 1;
	}
	else
		return -1;
}

int CCheckPacket::GetLineExt(int pos /* 1.. */, LineExt & rExt) const
{
	int    ok = 0;
	uint   _p = 0;
	if(ExtList.lsearch(&pos, &_p, CMPF_LONG)) {
		rExt = ExtList.at(_p);
		ok = 1;
	}
	else
		MEMSZERO(rExt);
	return ok;
}

int CCheckPacket::SetLineChZnPreprocessResult(int pos /*[1..]*/, const CCheckPacket::PreprocessChZnCodeResult * pResult)
{
	int    ok = 1;
	if(pos > 0 && pos <= Items_.getCountI()) {
		if(pResult) {
			bool found = false;
			if(P_PpChZnCodeResultList) {
				for(uint i = 0; i < P_PpChZnCodeResultList->getCount(); i++) {
					if(P_PpChZnCodeResultList->at(i).LineIdx == pos) {
						if(found) {
							P_PpChZnCodeResultList->atFree(i);
							assert(i > 0); // Иначе found не мог бы быть true.
							i--;
						}
						else {
							P_PpChZnCodeResultList->at(i) = *pResult;
							P_PpChZnCodeResultList->at(i).LineIdx = pos;
							found = true;
						}
					}
				}
			}
			else {
				P_PpChZnCodeResultList = new TSVector <PreprocessChZnCodeResult>();
			}
			if(!found) {
				P_PpChZnCodeResultList->insert(pResult);
				P_PpChZnCodeResultList->at(P_PpChZnCodeResultList->getCount()-1).LineIdx = pos;
			}
		}
		else if(P_PpChZnCodeResultList) {
			uint i = P_PpChZnCodeResultList->getCount();
			if(i) do {
				PreprocessChZnCodeResult & r_item = P_PpChZnCodeResultList->at(--i);
				if(r_item.LineIdx == pos) {
					P_PpChZnCodeResultList->atFree(i);
				}
			} while(i);
		}
	}
	else
		ok = 0;
	return ok;
}

const CCheckPacket::PreprocessChZnCodeResult * CCheckPacket::GetLineChZnPreprocessResult(int pos /*[1..]*/) const
{
	PreprocessChZnCodeResult * p_result = 0;
	if(SVector::GetCount(P_PpChZnCodeResultList)) {
		if(pos >= 1 && pos <= Items_.getCountI()) {
			for(uint i = 0; !p_result && i < P_PpChZnCodeResultList->getCount(); i++) {
				if(P_PpChZnCodeResultList->at(i).LineIdx == pos)
					p_result = &P_PpChZnCodeResultList->at(i);
			}
		}
	}
	return p_result;
}

int CCheckPacket::MergeLines(long /*options*/)
{
	int    ok = 1;
	uint   i = Items_.getCount();
	if(i) do {
		CCheckLineTbl::Rec & r_item = Items_.at(--i);
		uint p = 0;
		if(i && Items_.lsearch(&r_item.GoodsID, &p, CMPF_LONG, offsetof(CCheckLineTbl::Rec, GoodsID)) && p < i) {
			Items_.at(p).Quantity += r_item.Quantity;
			RemoveLine_(i);
		}
	} while(i);
	return ok;
}

void CCheckPacket::ClearLines()
{
	Items_.clear();
	ExtList.clear();
	LnTextList.Z();
}

int CCheckPacket::InsertItem_(const CCheckLineTbl::Rec * pItem, const char * pSerial, const char * pEgaisMark)
{
	int    ok = 1;
	THROW_SL(Items_.insert(pItem));
	const int idx = Items_.getCountI();
	if(!isempty(pSerial)) {
		THROW(SetLineTextExt(idx, CCheckPacket::lnextSerial, pSerial));
	}
	if(!isempty(pEgaisMark)) {
		THROW(SetLineTextExt(idx, CCheckPacket::lnextEgaisMark, pEgaisMark));
	}
	CATCHZOK
	return ok;
}

int CCheckPacket::InsertItem(const CCheckItem & rItem)
{
	int    ok = 1;
	CCheckLineTbl::Rec rec;
	rItem.GetRec(rec, BIN(Rec.Flags & CCHKF_RETURN));
	THROW_SL(Items_.insert(&rec));
	{
		const int idx = static_cast<int>(Items_.getCount());
		if(rItem.Serial[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextSerial, rItem.Serial));
		}
		if(rItem.EgaisMark[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextEgaisMark, rItem.EgaisMark));
		}
		// @v10.1.6 {
		if(rItem.RemoteProcessingTa[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextRemoteProcessingTa, rItem.RemoteProcessingTa));
		}
		// } @v10.1.6
		// @v10.4.12 {
		if(rItem.ChZnGtin[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextChZnGtin, rItem.ChZnGtin));
		}
		if(rItem.ChZnSerial[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextChZnSerial, rItem.ChZnSerial));
		}
		// } @v10.4.12 
		// @v10.6.9 {
		if(rItem.ChZnMark[0]) {
			THROW(SetLineTextExt(idx, CCheckPacket::lnextChZnMark, rItem.ChZnMark));
		}
		// } @v10.6.9 
		{
			LineExt le;
			le.Queue = rItem.Queue;
			SETFLAG(le.Flags, le.fGroup,    rItem.Flags & cifGrouped);
			SETFLAG(le.Flags, le.fModifier, rItem.Flags & cifModifier);
			SETFLAG(le.Flags, le.fPartOfComplex, rItem.Flags & cifPartOfComplex);
			SETFLAG(le.Flags, le.fQuotedByGift,  rItem.Flags & cifQuotedByGift);
			SETFLAG(le.Flags, le.fFixedPrice,    rItem.Flags & cifFixedPrice); // @v10.1.7
			THROW(SetLineExt(idx, le));
		}
	}
	CATCHZOK
	return ok;
}

int CCheckPacket::InsertItem(PPID goodsID, double qtty, double price, double dscnt, short div /* =0 */, int isPrinted /* =0 */)
{
	CCheckLineTbl::Rec item;
	// @v10.6.4 MEMSZERO(item);
	item.DivID = static_cast<short>(div);
	if(isPrinted)
		item.DivID += CHECK_LINE_IS_PRINTED_BIAS;
	item.GoodsID  = goodsID;
	item.Quantity = qtty;
	item.Price    = dbltointmny(price);
	item.Dscnt = dscnt;
	return InsertItem_(&item);
}

int CCheckPacket::SetupPaymList(const CcAmountList * pList)
{
	int    ok = -1;
	SString temp_buf;
	if(pList && pList->getCount()) {
		const double _total = pList->GetTotal();
		double _amt = 0.0, _discount;
		CalcAmount(&_amt, &_discount);
		// @v10.3.4 const double epsilon = 1E-6;
		// @v10.3.4 if(fabs(_amt - _total) > epsilon) {
		if(!feqeps(_amt, _total, 1E-6)) { // @v10.3.4 
			CCheckCore::MakeCodeString(&Rec, temp_buf);
			ok = PPSetError(PPERR_CCPAYMLISTDIFFAMT, temp_buf);
		}
		else if(pList->getCount() == 1) {
			int    typ = pList->at(0).Type;
			int    do_clear_paymlist = 1;
			if(typ == CCAMTTYP_BANK) {
				Rec.Flags |= CCHKF_BANKING;
				Rec.Flags &= ~CCHKF_INCORPCRD;
			}
			else {
				Rec.Flags &= ~CCHKF_BANKING;
				if(typ == CCAMTTYP_CRDCARD) {
					CcAl = *pList;
					Rec.Flags |= CCHKF_PAYMLIST;
					Rec.Flags |= CCHKF_INCORPCRD;
					do_clear_paymlist = 0;
				}
				else {
					Rec.Flags &= ~CCHKF_INCORPCRD;
				}
			}
			if(do_clear_paymlist) {
				CcAl.freeAll();
				Rec.Flags &= ~CCHKF_PAYMLIST;
			}
			ok = 2;
		}
		else {
			CcAl = *pList;
			Rec.Flags |= CCHKF_PAYMLIST;
			ok = 1;
		}
	}
	else {
		CcAl.freeAll();
		Rec.Flags &= ~CCHKF_PAYMLIST;
	}
	return ok;
}

void CCheckPacket::SetTotalDiscount__(double dis, long flags/*int pctdis, int plusDiscount*/)
{
	uint   i;
	uint   last_index = 0;
	double min_qtty = SMathConst::Max;
	double max_price = 0.0;
	double amount = 0.0, qtty, p, d;
	CCheckLineTbl::Rec * p_item;
	for(i = 0; Items_.enumItems(&i, (void **)&p_item);) {
		qtty = fabs(p_item->Quantity);
		p = intmnytodbl(p_item->Price);
		amount = R2(amount + p * qtty);
		if(qtty > 0.0 && (qtty < min_qtty || (qtty == min_qtty && p > max_price))) {
			last_index = i;
			min_qtty = qtty;
			max_price = p;
		}
	}
	double discount = 0.0;
	double part_dis = 0.0;
	double part_amount = 0.0;
	if(flags & stdfPlus) {
		discount = (flags & stdfPctDis) ? ((dis * amount) / (100.0 - dis)) : dis;
		amount += discount;
	}
	else
		discount = (flags & stdfPctDis) ? (dis * fdiv100r(amount)) : dis;
	/* @v9.8.1 {*/ if(last_index && (dis / max_price) <= 0.01) {
		p_item = &Items_.at(last_index-1);
		d = (discount / fabs(p_item->Quantity));
		p_item->Dscnt = d;
		if(flags & stdfPlus) {
			//p_item->Price = dbltointmny(intmnytodbl(p_item->Price) + p_item->Dscnt);
			p_item->Dscnt = -d;
		}
		else
			p_item->Dscnt = d;
	}
	else /* } @v9.8.1 */ {
		for(i = 0; Items_.enumItems(&i, (void **)&p_item);)
			if(i != last_index) {
				qtty = fabs(p_item->Quantity);
				if(qtty > 0.0) {
					p = intmnytodbl(p_item->Price);
					d = (p * (discount - part_dis) / (amount - part_amount));
					p_item->Dscnt = d;
					if(flags & stdfPlus) {
						//p_item->Price = dbltointmny(intmnytodbl(p_item->Price) + p_item->Dscnt);
						p_item->Dscnt = -d;
					}
					else
						p_item->Dscnt = d;
					part_dis += (d * qtty);
					part_amount += (p * qtty);
				}
			}
		if(last_index) {
			p_item = &Items_.at(last_index-1);
			d = ((discount - part_dis) / fabs(p_item->Quantity));
			p_item->Dscnt = d;
			if(flags & stdfPlus) {
				//p_item->Price = dbltointmny(intmnytodbl(p_item->Price) + p_item->Dscnt);
				p_item->Dscnt = -d;
			}
			else
				p_item->Dscnt = d;
		}
	}
}

double FASTCALL CCheckPacket::GetItemAmount(const CCheckLineTbl::Rec & rItem) const
{
	const double s  = intmnytodbl(rItem.Price) * rItem.Quantity;
	const double ds = rItem.Dscnt * rItem.Quantity;
	return (s - ds);
}

double FASTCALL CCheckPacket::GetItemDiscount(const CCheckLineTbl::Rec & rItem) const
{
	return (rItem.Dscnt * rItem.Quantity);
}

void CCheckPacket::CalcAmount(double * pAmt, double * pDscnt) const
{
	double amount = 0.0, discount = 0.0;
	CCheckLineTbl::Rec item;
	for(uint i = 0; EnumLines(&i, &item);) {
		const double qtty = item.Quantity;
		const double s  = intmnytodbl(item.Price) * qtty;
		const double ds = item.Dscnt * qtty;
		amount   = R2(amount + s - ds);
		discount = R2(discount + ds);
	}
	ASSIGN_PTR(pAmt, amount);
	ASSIGN_PTR(pDscnt, discount);
}

void CCheckPacket::SetupAmount(double * pAmt, double * pDscnt)
{
	double amt = 0.0, dscnt = 0.0;
	CalcAmount(&amt, &dscnt);
	LDBLTOMONEY(amt, Rec.Amount);
	LDBLTOMONEY(dscnt, Rec.Discount);
	ASSIGN_PTR(pAmt, amt);
	ASSIGN_PTR(pDscnt, dscnt);
}

int CCheckPacket::HasNonFiscalAmount(double * pFiscal, double * pNonFiscal) const
{
	int    yes = 0;
	double amt = fabs(MONEYTOLDBL(Rec.Amount));
	double fiscal = 0.0;
	double nonfiscal = 0.0;
	if(CcAl.getCount()) {
		nonfiscal = fabs(CcAl.Get(CCAMTTYP_CRDCARD));
		fiscal = amt - nonfiscal;
		yes = BIN(nonfiscal != 0.0);
	}
	else {
		if(Rec.Flags & CCHKF_INCORPCRD) {
			if(Rec.Flags & CCHKF_ADDPAYM) {
				fiscal = 0.0; // @v9.0.4 intmnytodbl(Ext.AddPaym)-->0.0
				nonfiscal = amt - fiscal;
			}
			else
				nonfiscal = amt;
			//
			PPObjSCard sc_obj;
			const PPID charge_goods_id = sc_obj.GetChargeGoodsID(Rec.SCardID);
			if(charge_goods_id) {
				CCheckLineTbl::Rec item;
				for(uint i = 0; EnumLines(&i, &item);) {
					if(item.GoodsID == charge_goods_id) {
						const double a = GetItemAmount(item);
						fiscal += a;
						nonfiscal -= a;
					}
				}
				fiscal = R2(fiscal);
				nonfiscal = R2(nonfiscal);
			}
			yes = BIN(nonfiscal != 0.0);
		}
		else
			fiscal = amt;
	}
	ASSIGN_PTR(pFiscal, fiscal);
	ASSIGN_PTR(pNonFiscal, nonfiscal);
	assert(R6(amt) == R6(fiscal+nonfiscal));
	return yes;
}

double CCheckPacket::GetFiscalAmount() const
{
	double amt = 0.0;
	if(CcAl.getCount()) {
		amt = fabs(MONEYTOLDBL(Rec.Amount)) - fabs(CcAl.Get(CCAMTTYP_CRDCARD));
	}
	else if(Rec.Flags & CCHKF_INCORPCRD) {
		/* @v10.3.4 @useless
		if(Rec.Flags & CCHKF_ADDPAYM)
			amt = 0.0; // intmnytodbl(Ext.AddPaym)-->0.0
		else
			amt = 0.0;
		*/
		PPObjSCard sc_obj;
		const PPID charge_goods_id = sc_obj.GetChargeGoodsID(Rec.SCardID);
		if(charge_goods_id) {
			CCheckLineTbl::Rec item;
			for(uint i = 0; EnumLines(&i, &item);)
				if(item.GoodsID == charge_goods_id)
					amt += GetItemAmount(item);
			amt = R2(amt);
		}
	}
	else
		amt = fabs(MONEYTOLDBL(Rec.Amount));
	return amt;
}

int CCheckPacket::SetDlvrAddr(const LocationTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec && Rec.Flags & CCHKF_DELIVERY) {
		if(SETIFZ(P_DlvrAddr, new LocationTbl::Rec)) {
			*P_DlvrAddr = *pRec;
			ok = 1;
		}
		else
			ok = PPSetErrorNoMem();
	}
	else if(P_DlvrAddr) {
		ZDELETE(P_DlvrAddr);
		ok = 1;
	}
	return ok;
}

const LocationTbl::Rec * CCheckPacket::GetDlvrAddr() const { return P_DlvrAddr; }
SString & CCheckPacket::MakeBarcodeIdent(SString & rBuf) const { return rBuf.Z().Cat("CA").CatLongZ(Rec.CashID, 4).CatLongZ(Rec.Code, 6); }

/*static*/int CCheckPacket::ParseBarcodeIdent(const char * pIdent, BarcodeIdentStruc * pResult)
{
	int    ok = 0;
	const  size_t len = sstrlen(pIdent);
	BarcodeIdentStruc result;
	result.Prefix = 0;
	result.PosId = 0;
	result.CcCode = 0;
	if(len == 12) {
        uint  s = 0;
		{
			// "CA"
			const uchar p_fl[] = { 'C', 'c', 209, 241, 145, 225 }; // 'C' в латинской и русской раскладках (ANSI и OEM)
			const uchar p_sl[] = { 'A', 'a', 212, 244, 148, 228 }; // 'A' в латинской и русской раскладках (ANSI и OEM)
			const uchar c1 = static_cast<uchar>(pIdent[0]);
			const uchar c2 = static_cast<uchar>(pIdent[1]);
			for(uint i = 0; !s && i < SIZEOFARRAY(p_fl); i++)
				if(c1 == p_fl[i])
					for(uint j = 0; !s && j < SIZEOFARRAY(p_sl); j++)
						if(c2 == p_sl[j])
							s = 1;
		}
        if(s) {
			SString temp_buf;
			temp_buf.Z().CatN(pIdent+2, 4);
			result.PosId = temp_buf.ToLong();
			temp_buf.Z().CatN(pIdent+6, 6);
			result.CcCode = temp_buf.ToLong();
			if(result.PosId > 0 && result.CcCode > 0) {
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pResult, result);
	return ok;
}
//
//
//
CSessTotal::CSessTotal()
{
	THISZERO();
}

void FASTCALL CSessTotal::Add(const CSessTotal * pSubTotal)
{
	CheckCount     += pSubTotal->CheckCount;
	Amount         += pSubTotal->Amount;
	Discount       += pSubTotal->Discount;
	BnkAmount      += pSubTotal->BnkAmount;    // @CSCardAmount
	CSCardAmount   += pSubTotal->CSCardAmount;
	WORetAmount    += pSubTotal->WORetAmount;
	WORetBnkAmount += pSubTotal->WORetBnkAmount;
	BnkDiscount    += pSubTotal->BnkDiscount;
	AltRegCount    += pSubTotal->AltRegCount; // @v10.0.02
	AltRegAmount   += pSubTotal->AltRegAmount; // @v10.0.02
}
//
// CCheckCore
//
/*static*/SString & FASTCALL CCheckCore::MakeCodeString(const CCheckTbl::Rec * pRec, SString & rBuf)
{
	rBuf.Z().Cat(pRec->Dt).CatDiv(':', 1);
	if(pRec->Tm)
		rBuf.Cat(pRec->Tm, MKSFMT(0, TIMF_HMS | TIMF_MSEC)).CatDiv(':', 1);
	return rBuf.CatLongZ(pRec->CashID, 4).CatDiv(':', 1).Cat(pRec->Code);
}

CCheckCore::CCheckCore() : CCheckTbl(), P_ChkOpJrnl(0)
{
	const PPCommConfig & r_ccfg = CConfig;
	P_Ext = (r_ccfg.Flags & CCFLG_USECCHECKEXT) ? new CCheckExtTbl : 0;
	P_LnExt = (r_ccfg.Flags & CCFLG_USECCHECKLINEEXT) ? new CCheckLineExtTbl : 0;
	{
		ReadEquipConfig(&EqCfg);
		if(EqCfg.Flags & PPEquipConfig::fWriteToChkOpJrnl)
			P_ChkOpJrnl = new CheckOpJrnl(this);
	}
}

CCheckCore::~CCheckCore()
{
	delete P_Ext;
	delete P_LnExt;
	ZDELETE(P_ChkOpJrnl);
}

const PPEquipConfig & CCheckCore::GetEqCfg() const { return EqCfg; } // @v10.9.9

int CCheckCore::HasExt()
{
	if(P_Ext) {
		RECORDNUMBER num_recs = 0;
		P_Ext->getNumRecs(&num_recs);
		if(num_recs > 10)
			return 1;
	}
	return 0;
}

int CCheckCore::Search(PPID id, CCheckTbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_CCHECK, id, pRec);
}

int CCheckCore::Search(PPID cashID, LDATE dt, LTIME tm, CCheckTbl::Rec * pRec)
{
	CCheckTbl::Key1 k;
	k.Dt = dt;
	k.Tm = tm;
	k.CashID = cashID;
	return SearchByKey(this, 1, &k, pRec);
}

int CCheckCore::SearchByTimeAndCard(PPID cardID, LDATE dt, LTIME tm, CCheckTbl::Rec * pRec)
{
	CCheckTbl::Key4 k;
	k.SCardID = cardID;
	k.Dt = dt;
	k.Tm = tm;
	return SearchByKey(this, 4, &k, pRec);
}

int CCheckCore::SearchByDateAndCode(long code, LDATE dt, int unprintedOnly, TSVector <CCheckTbl::Rec> * pRecList)
{
	int    ok = -1;
	if(pRecList) {
		pRecList->clear(); // @v10.6.11 freeAll()-->clear()
		CCheckTbl::Key1  k1;
		MEMSZERO(k1);
		k1.Dt = dt;
		while(search(1, &k1, spGt) && data.Dt == dt) {
			if((!code || data.Code == code) && (!unprintedOnly || !(data.Flags & CCHKF_PRINTED))) {
				THROW_SL(pRecList->insert(&data));
				ok = 1;
			}
		}
		THROW_DB(BTROKORNFOUND);
	}
	CATCHZOK
	return ok;
}

int CCheckCore::SearchForwardZCheck(PPID cashID, LDATE dt, LTIME tm, CCheckTbl::Rec * pRec)
{
	LDATETIME dtm;
	dtm.Set(dt, tm);
	CCheckTbl::Key1 k;
	BExtQuery q(this, 1, 128);
	q.select(this->ID, this->Flags, this->Dt, this->Tm, 0L).where(this->Dt >= dt && this->CashID == cashID);
	MEMSZERO(k);
	k.Dt = dt;
	k.Tm = tm;
	k.CashID = cashID;
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;)
		if(data.Flags & CCHKF_ZCHECK && cmp(dtm, data.Dt, data.Tm) < 0)
			return Search(data.ID, pRec);
	return -1;
}

int CCheckCore::GetListByExtFilt(const CCheckFilt & rFilt, ObjIdListFilt & rList)
{
	int    ok = -1;
	rList.Set(0);
	if(rFilt.HasExtFiltering()) {
		CCheckExtTbl * p_ext = P_Ext;
		if(p_ext) {
			PPIDArray temp_list; // @v10.7.3
			rList.InitEmpty();
			int    idx = 0;
			union {
				CCheckExtTbl::Key0 k0;
				CCheckExtTbl::Key2 k2;
			} k;
			MEMSZERO(k);
			if(rFilt.TableCode > 0) {
				idx = 2;
				k.k2.TableNo = rFilt.TableCode;
			}
			DBQ * dbq = 0;
			BExtQuery q(p_ext, idx);
			if(rFilt.TableCode > 0)
				dbq = ppcheckfiltid(dbq, p_ext->TableNo, rFilt.TableCode);
			dbq = ppcheckfiltid(dbq, p_ext->SalerID, rFilt.AgentID);
			dbq = ppcheckfiltid(dbq, p_ext->GuestCount, rFilt.GuestCount);
			dbq = ppcheckfiltid(dbq, p_ext->CreationUserID, rFilt.CreationUserID); // @v10.7.3
			if(rFilt.Flags & CCheckFilt::fZeroDlvrAddr) {
				dbq = &(*dbq && p_ext->AddrID == 0L);
			}
			else if(rFilt.DlvrAddrID) {
				dbq = &(*dbq && p_ext->AddrID == rFilt.DlvrAddrID);
			}
			if(rFilt.Flags & CCheckFilt::fStartOrderPeriod && !rFilt.Period.IsZero()) {
				//dbq = & (*dbq && daterange(p_ext->Dt, &rFilt.Period));
			}
			q.select(p_ext->CheckID, p_ext->CreationUserID, p_ext->CreationDtm, 0L).where(*dbq); // @v10.6.8 p_ext->CreationUserID
			const int is_start_ord_period = BIN(rFilt.Flags & CCheckFilt::fStartOrderPeriod && !rFilt.Period.IsZero());
			for(q.initIteration(false, &k, (idx == 2) ? spGe : spFirst); q.nextIteration() > 0;) {
				if(is_start_ord_period && !rFilt.Period.CheckDate(p_ext->data.CreationDtm.d))
					continue;
				// @v10.7.3 rList.Add(p_ext->data.CheckID);
				temp_list.addnz(p_ext->data.CheckID); // @v10.7.3 
			}
			// @v10.7.3 rList.Sort();
			temp_list.sortAndUndup(); // @v10.7.3
			rList.Set(&temp_list); // @v10.7.3
			ok = 1;
		}
	}
	return ok;
}

int CCheckCore::GetExt(PPID id, CCheckExtTbl::Rec * pExt)
{
	if(P_Ext && SearchByID(P_Ext, 0, id, pExt) > 0)
		return 1;
	else {
		memzero(pExt, sizeof(*pExt));
		return -1;
	}
}

int CCheckCore::UpdateExt(PPID id, const CCheckExtTbl::Rec * pExt, int use_ta)
{
	int    ok = -1;
	if(P_Ext) {
		CCheckExtTbl::Rec ccext_rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(P_Ext, 0, id, &ccext_rec) > 0);
		// @v10.6.8 (ccext_rec.CreationUserID != pExt->CreationUserID)
		if(ccext_rec.TableNo != pExt->TableNo || ccext_rec.CreationUserID != pExt->CreationUserID || ccext_rec.StartOrdDtm != pExt->StartOrdDtm ||
			ccext_rec.EndOrdDtm != pExt->EndOrdDtm || ccext_rec.CreationDtm != pExt->CreationDtm || ccext_rec.GuestCount != pExt->GuestCount ||
			ccext_rec.AddrID != pExt->AddrID || !sstreq(ccext_rec.Memo, pExt->Memo)) {
			CCheckExtTbl::Key0 k0;
			DBRowId _dbpos;
			THROW_DB(P_Ext->getPosition(&_dbpos));
			THROW_DB(P_Ext->getDirectForUpdate(0, &k0, _dbpos));
			ccext_rec.TableNo = pExt->TableNo;
			ccext_rec.StartOrdDtm = pExt->StartOrdDtm;
			ccext_rec.EndOrdDtm = pExt->EndOrdDtm;
			ccext_rec.CreationDtm = pExt->CreationDtm;
			ccext_rec.GuestCount = pExt->GuestCount;
			ccext_rec.AddrID = pExt->AddrID;
			ccext_rec.CreationUserID = pExt->CreationUserID; // @v10.6.8
			STRNSCPY(ccext_rec.Memo, pExt->Memo);
			THROW_DB(P_Ext->updateRecBuf(&ccext_rec)); // @sfu
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL CCheckCore::IsExtRecEq(const CCheckExtTbl::Rec & r1, const CCheckExtTbl::Rec & r2)
{
    int    eq = 1;
	if(r1.CheckID != r2.CheckID)
		eq = 0;
	else if(r1.SalerID != r2.SalerID)
		eq = 0;
	else if(r1.TableNo != r2.TableNo)
		eq = 0;
	else if(r1.GuestCount != r2.GuestCount)
		eq = 0;
	else if(r1.AddrID != r2.AddrID)
		eq = 0;
	else if(r1.LinkCheckID != r2.LinkCheckID)
		eq = 0;
	else if(r1.StartOrdDtm != r2.StartOrdDtm)
		eq = 0;
	else if(r1.EndOrdDtm != r2.EndOrdDtm)
		eq = 0;
	else if(r1.CreationDtm != r2.CreationDtm)
		eq = 0;
	else if(r1.CreationUserID != r2.CreationUserID) // @v10.6.8
		eq = 0;
	else if(!sstreq(r1.Memo, r2.Memo))
		eq = 0;
	return eq;
}

int CCheckCore::PutExt(const CCheckTbl::Rec * pRec, CCheckExtTbl::Rec * pExt, int useTa)
{
	int    ok = -1;
	if(pExt && pRec) {
		CCheckExtTbl::Rec org_rec;
		CCheckPacket pack;
		pack.Ext = *pExt;
		if(pack.HasExt()) {
			if(P_Ext) {
				//
				// Теоретически вероятность существования записи CCheckExt с ИД=id почти нулевая,
				// однако изредка возникает проблема дублирования индекса. Для устраненения этой
				// проблемы и предназначен следюущий код.
				//
				PPTransaction tra(useTa);
				THROW(tra);
				pExt->CheckID = pRec->ID;
				if(SearchByID(P_Ext, 0, pRec->ID, &org_rec) > 0) {
					if(!IsExtRecEq(org_rec, *pExt)) {
						THROW_DB(P_Ext->updateRecBuf(pExt));
						ok = 1;
					}
				}
				else {
					THROW_DB(P_Ext->insertRecBuf(pExt));
				}
				if(pExt->LinkCheckID) {
					CCheckTbl::Rec link_rec;
					THROW(Search(pExt->LinkCheckID, &link_rec) > 0);
					if(link_rec.Flags & CCHKF_ORDER && !(link_rec.Flags & CCHKF_CLOSEDORDER)) {
						link_rec.Flags |= CCHKF_CLOSEDORDER;
						THROW(UpdateFlags(link_rec.ID, link_rec.Flags, 0));
					}
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::GetOrderServersCheckList(PPID orderCheckID, PPIDArray & rList)
{
	int    ok = -1;
	SString cc_text;
	CCheckTbl::Rec rec;
	THROW(Search(orderCheckID, &rec) > 0);
	CCheckCore::MakeCodeString(&rec, cc_text);
	THROW_PP_S(rec.Flags & CCHKF_ORDER, PPERR_CCHKNORDER, cc_text);
	if(P_Ext) {
		CCheckExtTbl::Key1 k1;
		MEMSZERO(k1);
		k1.LinkCheckID = orderCheckID;
		while(P_Ext->search(1, &k1, spGt) && P_Ext->data.LinkCheckID == orderCheckID) {
			rList.addUnique(P_Ext->data.CheckID);
			ok = 1;
		}
		THROW(PPDbSearchError());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::ValidateCheck(PPID id, ValidateCheckParam & rParam, PPLogger & rLogger)
{
	int    ok = 1;
	CCheckPacket cc_pack;
	SString fmt_buf, chk_text, msg_buf;
	if(LoadPacket(id, 0, &cc_pack) > 0) {
		double amt = 0.0, dscnt = 0.0;
		CCheckCore::MakeCodeString(&cc_pack.Rec, chk_text);
		cc_pack.CalcAmount(&amt, &dscnt);
		const double rec_amt   = MONEYTOLDBL(cc_pack.Rec.Amount);
		const double rec_dscnt = MONEYTOLDBL(cc_pack.Rec.Discount);
		if(!feqeps(amt, rec_amt, rParam.Tolerance)) {
			rParam.ErrorFlags |= ValidateCheckParam::efInvAmount;
			// #error Сумма чека не совпадает с суммой по строкам
			rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_INVAMOUNT, fmt_buf), chk_text.cptr(), rec_amt - amt));
			rParam.ErrorFlags |= ValidateCheckParam::efInvAmount;
			ok = 0;
		}
		if(!feqeps(dscnt, rec_dscnt, rParam.Tolerance)) {
			rParam.ErrorFlags |= ValidateCheckParam::efInvDiscount;
			// #error Сумма скидки чека не совпадает с суммой скидки по строкам
			rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_INVDSCNT, fmt_buf), chk_text.cptr(), rec_dscnt - dscnt));
			rParam.ErrorFlags |= ValidateCheckParam::efInvDiscount;
			ok = 0;
		}
		// @v10.8.6 {
		{
			PPObjGoods goods_obj;
			for(uint i = 0; i < cc_pack.Items_.getCount(); i++) {
				const CCheckLineTbl::Rec & r_cc_item = cc_pack.Items_.at(i);
				if(goods_obj.Fetch(r_cc_item.GoodsID, 0) > 0) {
					; // ok
				}
				else {
					rParam.CcListWithUnresolvedGoods.add(cc_pack.Rec.ID);
					if(r_cc_item.GoodsID) {
						rParam.ErrorFlags |= ValidateCheckParam::efHandgedGoods;
						rParam.HangedGoodsList.addnz(r_cc_item.GoodsID);
					}
					else
						rParam.ErrorFlags |= ValidateCheckParam::efZeroGoods;
					PPLoadTextS(PPTXT_CCHKERR_INVGOODSID, msg_buf);
					msg_buf.CatDiv(':', 2).Cat(chk_text).Space().CatChar('#').Cat(r_cc_item.GoodsID);
					rLogger.Log(msg_buf);
					ok = 0;
				}
			}
		}
		// } @v10.8.6 
		{
			const int amt_sign = fsign(rec_amt);
			const CcAmountList & r_al = cc_pack.AL_Const();
			for(uint i = 0; i < r_al.getCount(); i++) {
				const CcAmountEntry & r_al_entry = r_al.at(i);
				if((amt_sign < 0 && r_al_entry.Amount > 0.0) || (amt_sign > 0 && r_al_entry.Amount < 0.0)) {
					rParam.ErrorFlags |= ValidateCheckParam::efInvAmtEntrySign;
					rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_INVALENTRYSIGN, fmt_buf), chk_text.cptr(), r_al_entry.Amount));
					ok = 0;
				}
			}
		}
		if(cc_pack.Rec.SCardID && Cards.Search(cc_pack.Rec.SCardID, 0) <= 0) {
			rParam.ErrorFlags |= ValidateCheckParam::efHandedSCard;
			rParam.HangedSCardList.add(cc_pack.Rec.SCardID);
			// #error Висячая ссылка на карту в чеке
			rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_INVSCARDID, fmt_buf), chk_text.cptr()));
			ok = 0;
		}
		{
			CCheckTbl::Key2 k2;
			MEMSZERO(k2);
			k2.CashID = cc_pack.Rec.CashID;
			k2.Code = cc_pack.Rec.Code;
			if(search(2, &k2, spLt) && data.CashID == cc_pack.Rec.CashID) {
				if(data.Code < (cc_pack.Rec.Code-1)) {
					SString code_buf;
					code_buf.Space().Z();
					for(long i = data.Code+1; i < cc_pack.Rec.Code; i++) {
						if(i > (data.Code+1))
							code_buf.CatDiv(',', 2);
						code_buf.Cat(data.CashID).CatChar('-').Cat(i);
					}
					rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_CODEGAP, fmt_buf), code_buf.cptr()));
				}
			}
		}
	}
	else {
		rParam.ErrorFlags |= ValidateCheckParam::efLoadingFault;
		// #error Ошибка загрузки чека %ld
		rLogger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_CCHKERR_UNABLELOAD, fmt_buf), id));
		ok = 0;
	}
	return ok;
}

int CCheckCore::GetNodeID(PPID checkID, PPID * pNodeID)
{
	int    ok = -1;
	PPID   node_id = 0;
	CCheckTbl::Rec rec;
	if(Search(checkID, &rec) > 0) {
		PPObjCSession cs_obj;
		CSessionTbl::Rec cs_rec;
		if(cs_obj.Search(rec.SessID, &cs_rec) > 0) {
			node_id = cs_rec.CashNodeID;
			ok = 1;
		}
	}
	ASSIGN_PTR(pNodeID, node_id);
	return ok;
}

int CCheckCore::LineByCheck(PPID checkID, int16 * pLine)
{
	CCheckLineTbl::Key0 k;
	k.CheckID  = checkID;
	k.RByCheck = MAXSHORT;
	if(Lines.searchKey(0, &k, spLt) && k.CheckID == checkID) {
		*pLine = k.RByCheck + 1;
		return 1;
	}
	else {
		*pLine = 1;
		return (BTROKORNFOUND) ? 1 : PPSetErrorDB();
	}
}

int CCheckCore::EnumLines(PPID checkID, int * pRByCheck, CCheckLineTbl::Rec * b)
{
	CCheckLineTbl::Key0 k;
	k.CheckID  = checkID;
	k.RByCheck = static_cast<int16>(*pRByCheck);
	if(Lines.search(0, &k, spGt) && k.CheckID == checkID) {
		Lines.copyBufTo(b);
		*pRByCheck = k.RByCheck;
		return 1;
	}
	else
		return PPDbSearchError();
}

int CCheckCore::LoadLinesBySessList(PPID goodsID, const PPIDArray * pSessList, const PPIDArray * pCashList,
	CCheckLineArray * pLinesList, LDATETIME * pLastCheckDtm)
{
	int    ok = 0;
	if(pSessList && pLinesList) {
		ObjIdListFilt sess_list, check_list;
		sess_list.Set(pSessList);
		THROW(ok = LoadChecksByList(&sess_list, pCashList, &check_list, pLastCheckDtm));
		if(ok > 0) {
			 THROW(ok = LoadLinesByList(goodsID, &check_list, pLinesList));
		}
	}
	else
		PPSetErrorInvParam();
	CATCHZOK
	return ok;
}

int CCheckCore::LoadChecksByList(const ObjIdListFilt * pSessList, const PPIDArray * pCashIDList,
	ObjIdListFilt * pCheckList, LDATETIME * pLastCheckDtm)
{
	int    ok = -1;
	LDATETIME last_dtm = ZERODATETIME;
	if(pSessList && pCheckList && pSessList->GetCount()) {
		const PPIDArray & sess_list = pSessList->Get();
		for(uint i = 0; i < sess_list.getCount(); i++) {
			const PPID  sess_id = sess_list.at(i);
			DBQ * dbq = 0;
			CCheckTbl::Key3 k3;
			BExtQuery q(this, 3);
			MEMSZERO(k3);
			k3.SessID = sess_id;
			dbq = ppcheckfiltid(dbq, SessID, sess_id);
			dbq = ppcheckfiltidlist(dbq, CashID, pCashIDList);
			q.select(ID, Dt, Tm, 0L).where(*dbq);
			for(q.initIteration(false, &k3, spGe); q.nextIteration();) {
				if(cmp(last_dtm, data.Dt, data.Tm) < 0)
					last_dtm.Set(data.Dt, data.Tm);
				pCheckList->Add(data.ID);
			}
			ok = 1;
		}
	}
	ASSIGN_PTR(pLastCheckDtm, last_dtm);
	return ok;
}

int CCheckCore::LoadLinesByList(PPID goodsID, const ObjIdListFilt * pCheckList, CCheckLineArray * pLinesList)
{
	int    ok = -1;
	if(pCheckList && pCheckList->GetCount()) {
		PPID   beg_chk = 0, end_chk = 0;
		PPIDArray check_list;
		CCheckLineTbl::Key0 k0;
		DBQ  * dbq = 0;
		BExtQuery q(&Lines, 0, 64);
		PPID   prev_ok_id = 0;
		pCheckList->CopyTo(&check_list);
		check_list.sort();
		beg_chk = check_list.at(0) - 1;
		end_chk = check_list.at(check_list.getCount() - 1) + 1;
		MEMSZERO(k0);
		k0.CheckID = beg_chk;
		q.selectAll();
		dbq = & (Lines.CheckID > beg_chk && Lines.CheckID < end_chk);
		/*
		if(goodsID)
			dbq = & (*dbq && Lines.GoodsID == goodsID);
		*/
		dbq = ppcheckfiltid(dbq, Lines.GoodsID, goodsID);
		q.where(*dbq);
		for(q.initIteration(false, &k0, spGt); q.nextIteration();) {
			const PPID check_id = Lines.data.CheckID;
			if(check_id == prev_ok_id || check_list.bsearch(check_id)) {
				prev_ok_id = check_id;
				pLinesList->insert(&Lines.data);
			}
		}
		ok = 1;
	}
	return ok;
}

int CCheckCore::GetLastCheck(PPID sessID, long cashN, CCheckTbl::Rec * pRec)
{
	CCheckTbl::Key3 k;
	k.SessID = sessID;
	k.CashID = cashN;
	k.Dt = MAXDATE;
	k.Tm = MAXTIME;
	while(search(3, &k, spLt) && data.SessID == sessID && data.CashID == cashN) {
		if(!(data.Flags & CCHKF_SKIP)) {
			copyBufTo(pRec);
			return 1;
		}
	}
	return PPDbSearchError();
}

int CCheckCore::GetLastCheckByCode(long cashN, CCheckTbl::Rec * pRec)
{
	CCheckTbl::Key2 k;
	k.CashID = cashN;
	k.Code = MAXLONG;
	k.Dt = MAXDATE;
	k.Tm = MAXTIME;
	if(search(2, &k, spLt) && k.CashID == cashN) {
		copyBufTo(pRec);
		return 1;
	}
	else
		return PPDbSearchError();
}

int CCheckCore::GetListByCode(long cashN, long code, TSVector <CCheckTbl::Rec> * pRecList)
{
	int    ok = -1;
	CCheckTbl::Key2 k;
	MEMSZERO(k);
	k.CashID = cashN;
	k.Code = code;
	if(search(2, &k, spGe) && k.CashID == cashN && k.Code == code) {
		do {
			CALLPTRMEMB(pRecList, insert(&data));
			ok = 1;
		} while(search(2, &k, spNext) && k.CashID == cashN && k.Code == code);
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int CCheckCore::GetLineExt(PPID checkID, int rByCheck, CCheckLineExtTbl::Rec * pRec)
{
	int    ok = -1;
	if(P_LnExt) {
		CCheckLineExtTbl::Key0 k0;
		k0.CheckID = checkID;
		k0.RByCheck = rByCheck;
		ok = SearchByKey(P_LnExt, 0, &k0, pRec);
	}
	return ok;
}

int CCheckCore::LoadLines(PPID checkID, PPID goodsID, CCheckPacket * pPack)
{
	int    ok = 1;
	BExtQuery q(&Lines, 0);
	q.selectAll().where(*ppcheckfiltid(&(Lines.CheckID == checkID), Lines.GoodsID, goodsID));
	CCheckLineTbl::Key0 k0;
	k0.CheckID = checkID;
	k0.RByCheck = -MAXSHORT;
	for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;)
		pPack->InsertItem_(&Lines.data);
	if(P_LnExt && goodsID == 0) {
		BExtQuery ql(P_LnExt, 0);
		ql.selectAll().where(P_LnExt->CheckID == checkID);
		CCheckLineExtTbl::Key0 lk0;
		lk0.CheckID = checkID;
		lk0.RByCheck = -MAXSHORT;
		for(ql.initIteration(false, &lk0, spGe); ql.nextIteration() > 0;) {
			CCheckLineExtTbl::Rec lx_rec;
			P_LnExt->copyBufTo(&lx_rec);
			uint pos = 0;
			if(pPack->SearchLine(lx_rec.RByCheck, &pos)) {
				CCheckPacket::LineExt le;
				le.Queue = lx_rec.Queue;
				SETFLAG(le.Flags, le.fGroup,    lx_rec.Flags & le.fGroup);
				SETFLAG(le.Flags, le.fModifier, lx_rec.Flags & le.fModifier);
				SETFLAG(le.Flags, le.fPartOfComplex, lx_rec.Flags & le.fPartOfComplex);
				SETFLAG(le.Flags, le.fQuotedByGift,  lx_rec.Flags & le.fQuotedByGift);
				SETFLAG(le.Flags, le.fFixedPrice,    lx_rec.Flags & le.fFixedPrice); // @v10.1.7
				pPack->SetLineExt(pos+1, le);
			}
		}
	}
	return ok;
}

/*static*/int CCheckCore::Helper_GetPaymList(CCheckPaymTbl * pCpTbl, PPID id, CcAmountList & rList)
{
	int    ok = -1;
	rList.Z();
	if(pCpTbl) {
		CCheckPaymTbl::Key0 cpk0;
		MEMSZERO(cpk0);
		cpk0.CheckID = id;
		if(pCpTbl->search(0, &cpk0, spGe) && pCpTbl->data.CheckID == id) {
			do {
				CcAmountEntry entry;
				entry.Type = pCpTbl->data.PaymType;
				entry.Amount = intmnytodbl(pCpTbl->data.Amount);
				entry.AddedID = pCpTbl->data.SCardID;
				entry.CurID = pCpTbl->data.CurID; // @v9.0.4
				entry.CurAmount = intmnytodbl(pCpTbl->data.CurAmount); // @v9.0.4
				THROW_SL(rList.insert(&entry));
			} while(pCpTbl->search(0, &cpk0, spNext) && pCpTbl->data.CheckID == id);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::GetPaymList(PPID id, CcAmountList & rList)
{
	return CCheckCore::Helper_GetPaymList(&PaymT, id, rList);
}

int CCheckCore::LoadPacket(PPID id, long flags, CCheckPacket * pPack)
{
	int    ok = -1;
	pPack->Z(); // @v10.8.7
	if(Search(id, &pPack->Rec) > 0) {
		ok = 1;
		MEMSZERO(pPack->Ext);
		if(!(flags & lpfNoLines))
			LoadLines(id, 0, pPack);
		{
			SString text_buf;
			THROW(PPRef->UtrC.GetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), text_buf));
			text_buf.Transf(CTRANSF_UTF8_TO_INNER);
			// @v10.3.9 pPack->UnpackLineTextExt(text_buf);
			pPack->UnpackTextExt(text_buf); // @v10.3.9
		}
		if(pPack->Rec.Flags & CCHKF_EXT) {
			int r = SearchByID(P_Ext, 0, id, &pPack->Ext);
			THROW(r);
			if(r < 0) {
				SString fmt_buf, msg_buf, cchk_code;
				CCheckCore::MakeCodeString(&pPack->Rec, cchk_code);
				msg_buf.Printf(PPLoadTextS(PPTXT_LOG_CCHKEXTNFOUND, fmt_buf), cchk_code.cptr());
				PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		THROW(GetPaymList(id, pPack->AL()));
	}
	CATCHZOK
	return ok;
}

void CCheckCore::WriteCCheckLogFile(const CCheckTbl::Rec * pRec, int action, int use_ta)
{
	CCheckPacket pack;
	pack.Rec = *pRec;
	if(DoCCheckTextLog())
		WriteCCheckLogFile(&pack, 0, action, use_ta);
	else if(P_ChkOpJrnl && pack.Rec.ID)
		THROW(P_ChkOpJrnl->LogEvent(action, &pack, 0, use_ta));
	CATCH
		PPError();
	ENDCATCH
}

void CCheckCore::WriteCCheckLogFile(const CCheckPacket * pPack, const CCheckLineTbl::Rec * pLineRec, int action, int use_ta)
{
	if(DoCCheckTextLog()) {
		SString  msg, temp_buf, saler_name;
		if(PPGetSubStr(PPTXT_CCHKLOG, action, msg) > 0) {
			GetArticleName(pPack->Ext.SalerID, saler_name);
			msg.Align(15, ADJ_LEFT).CatDiv('-', 1).Cat(MakeCodeString(&pPack->Rec, temp_buf));
			msg.CatDiv('-', 1).Cat(MONEYTOLDBL(pPack->Rec.Amount), SFMT_MONEY);
			msg.CatDiv('-', 1).Cat(MONEYTOLDBL(pPack->Rec.Discount), SFMT_MONEY);
			if(pPack->Rec.SCardID) {
				SCardTbl::Rec scard_rec;
				if(Cards.Search(pPack->Rec.SCardID, &scard_rec) > 0)
					msg.CatDiv('-', 1).Cat(scard_rec.Code);
			}
			if(saler_name.NotEmpty())
				msg.CatDiv('-', 1).Cat(saler_name);
			if(pPack->Ext.TableNo)
				msg.CatDiv('-', 1).Cat(pPack->Ext.TableNo);
			if(pLineRec) {
				GetGoodsName(pLineRec->GoodsID, temp_buf);
				msg.CatDiv('-', 1).Cat(temp_buf).CatDiv('-', 1).Cat(pLineRec->Quantity, SFMT_QTTY);
			}
			PPLogMessage(PPFILNAM_CCHECK_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
		}
	}
	if(P_ChkOpJrnl)
		THROW(P_ChkOpJrnl->LogEvent(action, pPack, pLineRec, use_ta));
	CATCH
		PPError();
	ENDCATCH
}

int CCheckCore::Add(PPID * pID, const CCheckTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	CCheckTbl::Rec header = *pRec;
	{
		//
		// Проверка на существование в БД чека начальных оборотов по карте
		//
		LDATE  d = encodedate(1, 1, 2001);
		if(header.SCardID && header.Dt == d && header.CashID == 0 && header.SessID == 0) {
			CCheckTbl::Key4 k4;
			MEMSZERO(k4);
			k4.SCardID = header.SCardID;
			k4.Dt = d;
			if(search(4, &k4, spGe) && k4.SCardID == header.SCardID && k4.Dt == d) {
				ok = -1;
			}
			else {
				//
				// Чек начальных оборотов с такой картой не найден - корректируем
				// время с тем, чтобы чек без дублирования влез в таблицу
				//
				CCheckTbl::Key1 k1;
				MEMSZERO(k1);
				k1.Dt = header.Dt;
				k1.Tm = header.Tm;
				k1.CashID = header.CashID;
				while(search(1, &k1, spEq)) {
					header.Tm.v++;
					k1.Tm = header.Tm;
				}
			}
		}
	}
	if(ok > 0) {
		PPTransaction tra(use_ta);
		THROW(tra);
#if 0 // @construction
		{
			CCheckTbl::Key1 k1;
			MEMSZERO(k1);
			k1.Dt = header.Dt;
			k1.Tm = header.Tm;
			k1.CashID = header.CashID;
			while(search(1, &k1, spEq)) {
				if(data.Code != header.Code) {
					header.Tm.v++;
					k1.Tm = header.Tm;
				}
				else {

				}
			}
		}
#endif // } 0
		copyBufFrom(&header);
		THROW_DB(insertRec(0, &id));
		if(!(header.Flags & (CCHKF_JUNK|CCHKF_SKIP)))
			THROW(Cards.AddTurnover(header.SCardID, MONEYTOLDBL(header.Amount), 0));
		THROW(tra.Commit());
		//WriteCCheckLogFile(&header, logWrited); // @vmiller Защита от дублирования записи в логе
	}
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int CCheckCore::Update(PPID id, const CCheckTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	CCheckTbl::Rec header = *pRec;
	header.ID = id;
	CCheckTbl::Rec org_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID_ForUpdate(this, PPOBJ_CCHECK, id, &org_rec) > 0);
		const double org_amt = MONEYTOLDBL(org_rec.Amount);
		const double new_amt = MONEYTOLDBL(header.Amount);
		const int org_skip = BIN(org_rec.Flags & (CCHKF_JUNK|CCHKF_SKIP));
		const int new_skip = BIN(header.Flags & (CCHKF_JUNK|CCHKF_SKIP));
		THROW_DB(updateRecBuf(&header));
		if(org_skip || new_skip || org_amt != new_amt) {
			if(!org_skip)
				THROW(Cards.AddTurnover(org_rec.SCardID, -org_amt, 0));
			if(!new_skip)
				THROW(Cards.AddTurnover(header.SCardID, new_amt, 0));
		}
		THROW(tra.Commit());
		//WriteCCheckLogFile(&header, logWrited); // @vmiller Защита от дублирования записи в логе
	}
	CATCHZOK
	return ok;
}

int CCheckCore::Helper_TurnSCardPayment(CCheckPacket * pPack, long flags, double * pUfpFactor, double * pUfpFactorUhtt, int use_ta)
{
	int    ok = 1;
	int    uhtt_err = 0;
	uint   i;
	double ufp_factor = DEREFPTRORZ(pUfpFactor);
	double ufp_factor_uhtt = DEREFPTRORZ(pUfpFactorUhtt);
	PPObjGoods * p_goods_obj = 0;
	PPUhttClient * p_uhtt_cli = 0;
	const  uint org_plc = pPack->AL().getCount();
	const  int do_turn_sc_op = BIN(!(pPack->Rec.Flags & (CCHKF_JUNK|CCHKF_SKIP|CCHKF_SUSPENDED)));
	if((pPack->Rec.Flags & CCHKF_PAYMLIST && org_plc) || (do_turn_sc_op && pPack->Rec.SCardID)) {
		CcAmountList temp_al;
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		SCardTbl::Rec sc_rec;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack->Rec.Flags & CCHKF_PAYMLIST && org_plc) {
			temp_al = pPack->AL();
		}
		if(do_turn_sc_op && pPack->Rec.SCardID && Cards.Search(pPack->Rec.SCardID, &sc_rec) > 0) {
			if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.GetType() == scstCredit) {
				const PPID crd_goods_grp_id = scs_rec.CrdGoodsGrpID;
				PPID charge_goods_id = 0;
				if(scs_rec.ChargeGoodsID)
					charge_goods_id = scs_rec.ChargeGoodsID;
				else {
					PPSCardConfig sc_cfg;
					if(PPObjSCard::FetchConfig(&sc_cfg) > 0)
						charge_goods_id = sc_cfg.ChargeGoodsID;
				}
				if(charge_goods_id || crd_goods_grp_id) {
					if(crd_goods_grp_id)
						p_goods_obj = new PPObjGoods;
					double charge_amount = 0.0;
					CCheckLineTbl::Rec ccl;
					for(i = 0; pPack->EnumLines(&i, &ccl, 0);) {
						if(crd_goods_grp_id && p_goods_obj && p_goods_obj->BelongToGroup(ccl.GoodsID, crd_goods_grp_id, 0) > 0) {
							charge_amount = -R3(ccl.Quantity);
						}
						else if(ccl.GoodsID == charge_goods_id) {
							if(crd_goods_grp_id)
								charge_amount = R3(ccl.Quantity);
							else {
								const double s  = intmnytodbl(ccl.Price) * ccl.Quantity;
								const double ds = ccl.Dscnt * ccl.Quantity;
								charge_amount += R2(s - ds);
							}
						}
					}
					if(charge_amount != 0.0)
						temp_al.Add(CCAMTTYP_CSCCHARGE, charge_amount, pPack->Rec.SCardID); // @v10.9.0 @fix (-charge_amount)-->(charge_amount)
				}
			}
		}
		const uint plc = temp_al.getCount();
		if(plc) {
			struct _UhttEntry {
				uint   P;
				char   Code[32];
			};
			SArray uhtt_list(sizeof(_UhttEntry));
			int16  rbc = 0;
			for(i = 0; i < plc; i++) {
				const CcAmountEntry & r_entry = temp_al.at(i);
				const PPID sc_id = r_entry.AddedID;
				if(!(flags & tscpfCorrection) && r_entry.Type != CCAMTTYP_CSCCHARGE) {
					CCheckPaymTbl::Rec cp_rec;
					// @v10.6.4 MEMSZERO(cp_rec);
					cp_rec.CheckID = pPack->Rec.ID;
					cp_rec.RByCheck = ++rbc;
					cp_rec.PaymType = static_cast<int16>(r_entry.Type);
					cp_rec.Amount = dbltointmny(r_entry.Amount);
					cp_rec.SCardID = sc_id;
					cp_rec.CurID = r_entry.CurID;
					cp_rec.CurAmount = dbltointmny(r_entry.CurAmount);
					THROW_DB(PaymT.insertRecBuf(&cp_rec));
					ufp_factor += 1.0;
				}
				if(sc_id && do_turn_sc_op && Cards.Search(sc_id, &sc_rec) > 0) {
					int    scst = (scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
					if(oneof2(scst, scstCredit, scstBonus)) {
						SCardOpTbl::Rec scop_rec;
						// @v10.6.4 MEMSZERO(scop_rec);
						scop_rec.SCardID = sc_id;
						scop_rec.Dt      = pPack->Rec.Dt;
						scop_rec.Tm      = pPack->Rec.Tm;
						scop_rec.LinkObjType = PPOBJ_CCHECK;
						scop_rec.LinkObjID   = pPack->Rec.ID;
						scop_rec.UserID  = pPack->Rec.UserID;
						scop_rec.Amount  = -r_entry.Amount;
						THROW(Cards.PutOpRec(&scop_rec, 0, 0));
						ufp_factor += 1.0;
						if(scs_rec.Flags & SCRDSF_UHTTSYNC && fabs(r_entry.Amount) > 0.01) {
							_UhttEntry u;
							u.P = i;
							STRNSCPY(u.Code, sc_rec.Code);
							THROW_SL(uhtt_list.insert(&u));
						}
					}
				}
			}
			if(uhtt_list.getCount() && !(flags & tscpfSkipScSpcTrt)) {
				ufp_factor_uhtt += 1.0;
				THROW_MEM(p_uhtt_cli = new PPUhttClient);
				if(!p_uhtt_cli->Auth()) {
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
					uhtt_err = 1;
					ZDELETE(p_uhtt_cli);
				}
				else {
					for(i = 0; i < uhtt_list.getCount(); i++) {
						const _UhttEntry * p_u = static_cast<const _UhttEntry *>(uhtt_list.at(i));
						const CcAmountEntry & r_entry = temp_al.at(p_u->P);
						char   uhtt_sc_code[64];
						PTR32(uhtt_sc_code)[0] = 0;
						UhttSCardPacket scp;
						ufp_factor_uhtt += 1.0;
						if(p_uhtt_cli->GetSCardByNumber(p_u->Code, scp)) {
							scp.Code.CopyTo(uhtt_sc_code, sizeof(uhtt_sc_code));
							if(uhtt_sc_code[0]) {
								if(r_entry.Amount > 0.0) {
									if(!p_uhtt_cli->WithdrawSCardAmount(uhtt_sc_code, r_entry.Amount)) {
										PPSetError(PPERR_UHTT_SCWITHDRAW, p_uhtt_cli->GetLastMessage());
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
										uhtt_err = 1;
									}
									ufp_factor_uhtt += 1.0;
								}
								else if(r_entry.Amount < 0.0) {
									if(!p_uhtt_cli->DepositSCardAmount(uhtt_sc_code, -r_entry.Amount)) {
										PPSetError(PPERR_UHTT_SCDEPOSIT, p_uhtt_cli->GetLastMessage());
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
										uhtt_err = 1;
									}
									ufp_factor_uhtt += 1.0;
								}
							}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	delete p_goods_obj;
	delete p_uhtt_cli;
	ASSIGN_PTR(pUfpFactor, ufp_factor);
	ASSIGN_PTR(pUfpFactorUhtt, ufp_factor_uhtt);
	return ok;
}

int CCheckCore::TurnSCardPayment(CCheckPacket * pPack, long flags, int use_ta)
{
	return Helper_TurnSCardPayment(pPack, flags, 0, 0, use_ta);
}

int CCheckCore::PreparePacketForWriting(PPID id, CCheckPacket * pPack, double & rUfpFactor)
{
	int    ok = 1;
	assert(id != 0 && (pPack->Rec.ID == 0 || pPack->Rec.ID == id));
	{
		//
		// Удаляем старые строки
		//
		{
			CCheckLineTbl::Key0 k0;
			k0.CheckID  = id;
			k0.RByCheck = -MAXSHORT;
			if(Lines.searchForUpdate(0, &k0, spGe) && Lines.data.CheckID == id) {
				do {
					rUfpFactor += 1.0;
					THROW_DB(Lines.deleteRec()); // @sfu
				} while(Lines.searchForUpdate(0, &k0, spNext) && Lines.data.CheckID == id);
			}
			THROW(PPDbSearchError());
		}
		if(P_LnExt) {
			rUfpFactor += 1.0;
			THROW_DB(deleteFrom(P_LnExt, 0, P_LnExt->CheckID == id));
		}
	}
	//THROW(LineByCheck(id, &last_rbc));
	//THROW(pPack->PrepareForWriting(id, last_rbc));
	//int CCheckPacket::PrepareForWriting(PPID ccheckID, int16 lastRbc)
	{
		pPack->Rec.ID = id;
		//int16  last_rbc = NZOR(lastRbc, 1);
		for(uint i = 0; i < pPack->Items_.getCount(); i++) {
			CCheckLineTbl::Rec & r_item = pPack->Items_.at(i);
			r_item.CheckID = id;
			r_item.RByCheck = static_cast<int16>(i+1);
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::UpdateCheck(CCheckPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    uhtt_err = 0;
	uint   i;
	const  PPID  id = pPack->Rec.ID;
	int16  last_rbc = 0;
	const  uint org_plc = pPack->AL().getCount();
	double ufp_factor = 0.0;
	double ufp_factor_uhtt = 0.0;
	PPUserFuncProfiler ufp(PPUPRF_CCHECKPUT);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PreprocessPacket(pPack));
		THROW(Update(id, &pPack->Rec, 0));
		ufp_factor += 1.0;
		THROW(PreparePacketForWriting(id, pPack, ufp_factor));
		{
			BExtInsert bei(&Lines);
			CCheckLineTbl::Rec ccl;
			SString serial;
			SString * p_serial = P_LnExt ? &serial : 0;
			for(i = 0; pPack->EnumLines(&i, &ccl, p_serial);) {
				THROW_DB(bei.insert(&ccl));
				ufp_factor += 1.0;
				if(P_LnExt) {
					CCheckPacket::LineExt le;
					pPack->GetLineExt(i, le);
					if(le.Queue || (le.Flags & (le.fGroup|le.fModifier|le.fPartOfComplex|le.fQuotedByGift|le.fFixedPrice))) { // @v10.1.7 le.fFixedPrice
						CCheckLineExtTbl::Rec lx_rec;
						// @v10.6.4 MEMSZERO(lx_rec);
						lx_rec.CheckID = ccl.CheckID;
						lx_rec.RByCheck = ccl.RByCheck;
						lx_rec.Queue = le.Queue;
						SETFLAG(lx_rec.Flags, le.fGroup,    le.Flags & le.fGroup);
						SETFLAG(lx_rec.Flags, le.fModifier, le.Flags & le.fModifier);
						SETFLAG(lx_rec.Flags, le.fPartOfComplex, le.Flags & le.fPartOfComplex);
						SETFLAG(lx_rec.Flags, le.fQuotedByGift,  le.Flags & le.fQuotedByGift);
						SETFLAG(lx_rec.Flags, le.fFixedPrice,  le.Flags & le.fFixedPrice); // @v10.1.7
						THROW_DB(P_LnExt->insertRecBuf(&lx_rec));
						ufp_factor += 1.0;
					}
				}
			}
			THROW_DB(bei.flash());
		}
		{
			SString text_buf;
			// @v10.3.9 pPack->PackLineTextExt(text_buf);
			pPack->PackTextExt(text_buf); // @v10.3.9
			if(text_buf.NotEmptyS()) {
				THROW(PPRef->UtrC.SetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), text_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
			}
		}
		{
			int    r = 0;
			THROW(r = PutExt(&pPack->Rec, &pPack->Ext, 0));
			if(r > 0)
				ufp_factor += 1.0;
		}
		assert(org_plc == pPack->AL_Const().getCount());
		{
			long   tscpf = 0;
			if(pPack->UpdFlags & CCheckPacket::ufSkipScSpcTrt)
				tscpf |= CCheckCore::tscpfSkipScSpcTrt;
			THROW_DB(deleteFrom(&PaymT, 0, PaymT.CheckID == id));
			THROW(Helper_TurnSCardPayment(pPack, tscpf, &ufp_factor, &ufp_factor_uhtt, 0));
		}
		THROW(tra.Commit());
	}
	ufp.SetFactor(1, ufp_factor);
	ufp.SetFactor(2, ufp_factor_uhtt);
	ufp.Commit();
	CATCHZOK
	return ok;
}

int CCheckCore::UpdateExtText(PPID id, int fldId, const char * pText, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString text_buf;
	CCheckPacket cc_pack;
	if(fldId > 0) {
		PPTransaction tra(use_ta);
		THROW(tra);
		//
		p_ref->UtrC.GetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), text_buf);
		text_buf.Transf(CTRANSF_UTF8_TO_INNER);
		cc_pack.UnpackTextExt(text_buf);
		//
		cc_pack.PutExtStrData(fldId, pText);
		//
		cc_pack.PackTextExt(text_buf);
		THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), text_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::PreprocessPacket(CCheckPacket * pPack)
{
	int    ok = 1;
	if(pPack->Rec.Dt == 0)
		getcurdatetime(&pPack->Rec.Dt, &pPack->Rec.Tm);
	if(pPack->Rec.UserID == 0) {
		PPSecur sec_rec;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		if(sec_obj.Fetch(LConfig.UserID, &sec_rec) > 0)
			pPack->Rec.UserID = sec_rec.PersonID;
	}
	if(pPack->HasExt()) {
		if(P_Ext) {
			pPack->Rec.Flags |= CCHKF_EXT;
		}
	}
	{
		SInvariantParam invp;
		int    r = pPack->InvariantC(&invp);
		assert(r != 0);
		if(pPack->UpdFlags & CCheckPacket::ufCheckInvariant) {
			if(!r) {
				SString ccheck_code;
				CCheckCore::MakeCodeString(&pPack->Rec, ccheck_code);
				PPSetError(PPERR_INVAR_CCHECKPACKET, ccheck_code);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			}
		}
	}
	return ok;
}

int CCheckCore::TurnCheck(CCheckPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    uhtt_err = 0;
	uint   i;
	PPID   id = 0;
	int16  last_rbc = 0;
	const  uint org_plc = pPack->AL().getCount();
	double ufp_factor = 0.0;
	double ufp_factor_uhtt = 0.0;
	PPUserFuncProfiler ufp(PPUPRF_CCHECKPUT);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PreprocessPacket(pPack));
		THROW(Add(&id, &pPack->Rec, 0));
		ufp_factor += 1.0;
#if 0 // {
		THROW(LineByCheck(id, &last_rbc));
		THROW(pPack->PrepareForWriting(id, last_rbc));
#endif // } 0
		THROW(PreparePacketForWriting(id, pPack, ufp_factor));
		{
			BExtInsert bei(&Lines);
			CCheckLineTbl::Rec ccl;
			SString serial;
			SString * p_serial = P_LnExt ? &serial : 0;
			for(i = 0; pPack->EnumLines(&i, &ccl, p_serial);) {
				THROW_DB(bei.insert(&ccl));
				ufp_factor += 1.0;
				if(P_LnExt) {
					CCheckPacket::LineExt le;
					pPack->GetLineExt(i, le);
					if(le.Queue || (le.Flags & (le.fGroup|le.fModifier|le.fPartOfComplex|le.fQuotedByGift|le.fFixedPrice))) { // @v10.1.7 le.fFixedPrice
						CCheckLineExtTbl::Rec lx_rec;
						// @v10.6.4 MEMSZERO(lx_rec);
						lx_rec.CheckID = ccl.CheckID;
						lx_rec.RByCheck = ccl.RByCheck;
						lx_rec.Queue = le.Queue;
						SETFLAG(lx_rec.Flags, le.fGroup,    le.Flags & le.fGroup);
						SETFLAG(lx_rec.Flags, le.fModifier, le.Flags & le.fModifier);
						SETFLAG(lx_rec.Flags, le.fPartOfComplex, le.Flags & le.fPartOfComplex);
						SETFLAG(lx_rec.Flags, le.fQuotedByGift,  le.Flags & le.fQuotedByGift);
						SETFLAG(lx_rec.Flags, le.fFixedPrice,    le.Flags & le.fFixedPrice); // @v10.1.7
						THROW_DB(P_LnExt->insertRecBuf(&lx_rec));
						ufp_factor += 1.0;
					}
				}
			}
			THROW_DB(bei.flash());
		}
		{
			SString text_buf;
			// @v10.3.9 pPack->PackLineTextExt(text_buf);
			pPack->PackTextExt(text_buf); // @v10.3.9
			if(text_buf.NotEmptyS()) {
				THROW(PPRef->UtrC.SetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), text_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
			}
		}
		{
			int    r = 0;
			THROW(r = PutExt(&pPack->Rec, &pPack->Ext, 0));
			if(r > 0)
				ufp_factor += 1.0;
		}
		assert(org_plc == pPack->AL_Const().getCount());
		{
			long   tscpf = 0;
			if(pPack->UpdFlags & CCheckPacket::ufSkipScSpcTrt)
				tscpf |= CCheckCore::tscpfSkipScSpcTrt;
			THROW(Helper_TurnSCardPayment(pPack, tscpf, &ufp_factor, &ufp_factor_uhtt, 0));
		}
		THROW(tra.Commit());
	}
	ufp.SetFactor(1, ufp_factor);
	ufp.SetFactor(2, ufp_factor_uhtt);
	ufp.Commit();
	CATCHZOK
	return ok;
}

int CCheckCore::RemovePacket(PPID id, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    r = 0;
	CCheckPacket pack;
	if(DoCCheckTextLog())
		LoadPacket(id, 0, &pack);
	{
		int    do_remove_tags = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			CCheckLineTbl::Key0 k0;
			k0.CheckID  = id;
			k0.RByCheck = 0;
			if(Lines.searchForUpdate(0, &k0, spGe) && Lines.data.CheckID == id) {
				do {
					THROW_DB(Lines.deleteRec()); // @sfu
				} while(Lines.searchForUpdate(0, &k0, spNext) && Lines.data.CheckID == id);
			}
			THROW(PPDbSearchError());
		}
		if(P_LnExt) {
			THROW_DB(deleteFrom(P_LnExt, 0, P_LnExt->CheckID == id));
		}
		THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_CCHECK, id, PPTRPROP_CC_LNEXT), static_cast<const wchar_t *>(0), 0));
		THROW(Cards.RemoveOpByCheck(id, 0));
		if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, id, 0) > 0) {
			if(data.Flags & CCHKF_JUNK)
				do_remove_tags = 1;
			else if(!(data.Flags & CCHKF_SKIP)) {
				THROW(Cards.AddTurnover(data.SCardID, -MONEYTOLDBL(data.Amount), 0));
			}
			THROW_DB(deleteRec()); // @sfu
		}
		if(P_Ext)
			THROW_DB(deleteFrom(P_Ext, 0, P_Ext->CheckID == id));
		THROW_DB(deleteFrom(&PaymT, 0, PaymT.CheckID == id));
		if(do_remove_tags) {
			p_ref->Ot.RemoveTag(PPOBJ_CCHECK, id, 0, 0);
		}
		if(DoCCheckTextLog()) {
			WriteCCheckLogFile(&pack, 0, (pack.Rec.Flags & CCHKF_JUNK) ? CCheckCore::logJunkRemoved : CCheckCore::logRemoved, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::RemoveSess(PPID sessID, int use_ta)
{
	int    ok = 1;
	CCheckTbl::Key3 k;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k);
		k.SessID = sessID;
		while(search(3, &k, spGt) && k.SessID == sessID)
			THROW(RemovePacket(data.ID, 0));
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::AddRecToSessTotal(const CCheckTbl::Rec * pRec, CSessTotal * pTotal)
{
	int    ok = 1;
	if(pRec->Flags & CCHKF_SKIP) {
		ok = -1;
	}
	else {
		pTotal->CheckCount++;
		if(!(pRec->Flags & CCHKF_ZCHECK)) {
			double amount = MONEYTOLDBL(pRec->Amount);
			double add_paym = 0.0;
			CCheckExtTbl::Rec ext_rec;
			pTotal->Amount   += amount;
			pTotal->Discount += MONEYTOLDBL(pRec->Discount);
			// @v10.0.02 {
			if(pRec->Flags & CCHKF_ALTREG) {
				pTotal->AltRegCount++;
				pTotal->AltRegAmount += amount;
			}
			// } @v10.0.02
			int    pmr = 0;
			CcAmountList _pl;
			if(pRec->Flags & CCHKF_PAYMLIST) {
				THROW(pmr = GetPaymList(pRec->ID, _pl));
			}
			if(pmr > 0) {
				const double bnkamt = _pl.Get(CCAMTTYP_BANK);
				pTotal->BnkAmount    += bnkamt;
				pTotal->CSCardAmount += _pl.Get(CCAMTTYP_CRDCARD);
				if(amount < 0.0) {
					pTotal->RetCheckCount++;
					pTotal->RetAmount += fabs(amount);
					if(bnkamt != 0.0) {
						pTotal->RetBnkCount++;
						pTotal->RetBnkAmount += bnkamt;
					}
				}
				else {
					pTotal->SaleCheckCount++;
					pTotal->WORetAmount += amount;
					if(bnkamt != 0.0) {
						pTotal->SaleBnkCount++;
						pTotal->WORetBnkAmount += bnkamt;
					}
				}
			}
			else {
				if(pRec->Flags & CCHKF_ADDPAYM && GetExt(pRec->ID, &ext_rec) > 0)
					add_paym = 0.0;
				if(pRec->Flags & CCHKF_INCORPCRD) {
					pTotal->CSCardAmount += (amount - add_paym);
					if(add_paym != 0.0 && pRec->Flags & CCHKF_BANKING) {
						pTotal->BnkAmount  += add_paym; // @CSCardAmount
						pTotal->BnkDiscount += fdivnz(add_paym, amount) * MONEYTOLDBL(pRec->Discount);
					}
				}
				else if(pRec->Flags & CCHKF_BANKING) {
					pTotal->BnkAmount   += (amount - add_paym); // @CSCardAmount
					add_paym = 0.0;
					pTotal->BnkDiscount += MONEYTOLDBL(pRec->Discount);
				}
				if(amount < 0.0) {
					pTotal->RetCheckCount++;
					pTotal->RetAmount += fabs(amount);
					if(pRec->Flags & CCHKF_BANKING) {
						pTotal->RetBnkCount++;
						pTotal->RetBnkAmount += fabs(amount);
					}
				}
				else {
					pTotal->SaleCheckCount++;
					pTotal->WORetAmount += amount;
					if(pRec->Flags & CCHKF_BANKING) {
						pTotal->SaleBnkCount++;
						pTotal->WORetBnkAmount += amount;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::GetListBySess(PPID sessID, long flags, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	CCheckTbl::Key3 k;
	MEMSZERO(k);
	k.SessID = sessID;
	BExtQuery q(this, 3);
	q.selectAll();
	q.where(this->SessID == sessID);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(!(data.Flags & CCHKF_SKIP) && (!(flags & gglfSkipUnprintedChecks) || (data.Flags & CCHKF_PRINTED))) {
			rList.add(data.ID);
		}
	}
	if(rList.getCount())
		ok = 1;
	return ok;
}

int CCheckCore::GetSessTotal(PPID sessID, long flags, CSessTotal * pTotal, BVATAccmArray * pVatList)
{
	int    ok = 1;
	CCheckPacket pack;
	ObjIdListFilt check_list;
	CCheckTbl::Key3 k;
	MEMSZERO(k);
	k.SessID = sessID;
	BExtQuery q(this, 3);
	if(flags & gglfUseFullCcPackets) {
		q.select(this->ID, this->Flags, 0);
	}
	else {
		q.selectAll();
	}
	q.where(this->SessID == sessID);
	pTotal->SessID = sessID;
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(!(data.Flags & CCHKF_SKIP) && (!(flags & gglfSkipUnprintedChecks) || (data.Flags & CCHKF_PRINTED))) {
			if(flags & gglfUseFullCcPackets) {
				// @v10.8.7 (LoadPacket) pack.Z();
				if(LoadPacket(data.ID, 0, &pack) > 0) {
					double fiscal = 0.0, nonfiscal = 0.0;
					AddRecToSessTotal(&pack.Rec, pTotal);
					pack.HasNonFiscalAmount(&fiscal, &nonfiscal);
					pTotal->FiscalAmount += fiscal;
					if(pVatList)
						THROW_SL(check_list.Add(pack.Rec.ID));
				}
			}
			else {
				AddRecToSessTotal(&data, pTotal);
				if(pVatList)
					THROW_SL(check_list.Add(data.ID));
			}
		}
	}
	if(pVatList) {
		PPObjGoods goods_obj;
		CCheckLineArray cl_list;
		THROW(LoadLinesByList(0, &check_list, &cl_list));
		const uint c = cl_list.getCount();
		for(uint i = 0; i < c; i++) {
			const CCheckLineTbl::Rec & r_ln_rec = cl_list.at(i);
			const double ln_p = intmnytodbl(r_ln_rec.Price);
			const double ln_q = r_ln_rec.Quantity;
			{
				BVATAccm  bva_item;
				PPGoodsTaxEntry gtx;
				if(goods_obj.FetchTax(r_ln_rec.GoodsID, LConfig.OperDate, 0L, &gtx) > 0)
					bva_item.PRate = gtx.GetVatRate();
				bva_item.PTrnovr  += ln_q * (ln_p - r_ln_rec.Dscnt);
				bva_item.Discount += ln_q * r_ln_rec.Dscnt;
				THROW(pVatList->Add(&bva_item));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::UpdateSCard(PPID checkID, PPID newSCardID, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, checkID, 0) > 0 && newSCardID != data.SCardID) {
			PPID   old_scard_id = data.SCardID;
			double amount = MONEYTOLDBL(data.Amount);
			data.SCardID = newSCardID;
			THROW_DB(updateRec()); // @sfu
			if(!(data.Flags & (CCHKF_JUNK|CCHKF_SKIP))) {
				THROW(Cards.AddTurnover(old_scard_id, -amount, 0));
				THROW(Cards.AddTurnover(newSCardID, amount, 0));
			}
			DS.LogAction(PPACN_SCARDBINDUPD, PPOBJ_CCHECK, checkID, old_scard_id, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::UpdateRec(PPID checkID, const CCheckTbl::Rec * pRec, int use_ta)
{
	int    ok = -1;
	CCheckTbl::Rec rec;
	if(pRec) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, checkID, &rec) > 0) {
			if(pRec->SessID != rec.SessID || pRec->Code != rec.Code || pRec->CashID != rec.CashID || pRec->Dt != rec.Dt || pRec->Tm != rec.Tm) {
				data.SessID = pRec->SessID;
				data.Code   = pRec->Code;
				data.CashID = pRec->CashID;
				data.Dt     = pRec->Dt;
				data.Tm     = pRec->Tm;
				THROW_DB(updateRec()); // @sfu
				DS.LogAction(PPACN_CCHECKUPDATED, PPOBJ_CCHECK, checkID, 0, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::UpdateFlags(PPID checkID, long flags, int use_ta)
{
	int    ok = -1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, checkID, 0) > 0) {
			if(data.Flags != flags) {
				data.Flags = flags;
				THROW_DB(updateRec()); // @sfu
				DS.LogAction(PPACN_CCHECKUPDATED, PPOBJ_CCHECK, checkID, 0, 0);
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::ToggleDeliveryCloseTag(PPID checkID, const LDATETIME & rDtm, int set, int use_ta)
{
	int    ok = -1;
	CCheckTbl::Rec cc_rec;
	CCheckExtTbl::Rec ext_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, checkID, &cc_rec) > 0) {
			SString msg_buf;
			THROW_PP_S(cc_rec.Flags & CCHKF_DELIVERY, PPERR_CCHKNDELIVERY, CCheckCore::MakeCodeString(&cc_rec, msg_buf));
			if(set) {
				THROW_SL(checkdate(rDtm.d, 1));
				THROW_SL(checktime(rDtm.t));
			}
			if(GetExt(checkID, &ext_rec) <= 0)
				MEMSZERO(ext_rec);
			if(BIN(cc_rec.Flags & CCHKF_CLOSEDORDER) != BIN(set)) {
				SETFLAG(cc_rec.Flags, CCHKF_CLOSEDORDER, set);
				THROW_DB(updateRecBuf(&cc_rec)); // @sfu
				DS.LogAction(PPACN_CCHECKTOGGLEDLVR, PPOBJ_CCHECK, checkID, BIN(set), 0);
				ok = 1;
			}
			if(ext_rec.CheckID) {
				if(!set && !!ext_rec.EndOrdDtm) {
					ext_rec.EndOrdDtm.Z();
					THROW(UpdateExt(checkID, &ext_rec, 0));
					ok = 1;
				}
				else if(set && ext_rec.EndOrdDtm != rDtm) {
					ext_rec.EndOrdDtm = rDtm;
					THROW(UpdateExt(checkID, &ext_rec, 0));
					ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::ReplaceSCard(PPID destCardID, PPID srcCardID, int use_ta)
{
	int    ok = -1;
	if(destCardID && srcCardID != destCardID) {
		PPIDArray id_list;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(GetListByCard(destCardID, 0, &id_list) > 0) {
			for(uint i = 0; i < id_list.getCount(); i++) {
				const  PPID check_id = id_list.get(i);
				if(SearchByID_ForUpdate(this, PPOBJ_CCHECK, check_id, 0) > 0) { // @v7.0.11 CCheckCore::Search-->::SearchByID_ForUpdate
					double amount = MONEYTOLDBL(data.Amount);
					data.SCardID = srcCardID;
					THROW_DB(updateRec()); // @sfu
					if(!(data.Flags & (CCHKF_JUNK|CCHKF_SKIP))) { // @v7.0.11
						THROW(Cards.AddTurnover(destCardID, -amount, 0));
						THROW(Cards.AddTurnover(srcCardID, amount, 0));
					}
					DS.LogAction(PPACN_SCARDBINDUPD, PPOBJ_CCHECK, check_id, destCardID, 0);
					ok = 1;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::SetFlagsBySess(PPID sessID, long setFlags, long resetFlags, int use_ta)
{
	int    ok = 1;
	CCheckTbl::Key3 k;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k);
		k.SessID = sessID;
		// @todo update_for
		while(searchForUpdate(3, &k, spGt) && k.SessID == sessID) {
			if(setFlags)
				data.Flags |= setFlags;
			if(resetFlags)
				data.Flags &= ~resetFlags;
			THROW_DB(updateRec()); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::IsCheckWFlags(PPID sessID, long chkFlags)
{
	int    is_found = 0;
	CCheckTbl::Key3 k;
	MEMSZERO(k);
	k.SessID = sessID;
	BExtQuery * p_q = new BExtQuery(this, 3);
	p_q->selectAll().where(SessID == sessID);
	for(p_q->initIteration(false, &k, spGe); p_q->nextIteration() > 0;)
		if(data.Flags & chkFlags) {
			is_found = 1;
			break;
		}
	delete p_q;
	return is_found;
}

int CCheckCore::GetActiveExpendByLocList(const ObjIdListFilt * pLocList, DraftRcptArray * pList)
{
	int    ok = 1;
	PPIDArray loc_list;
	ObjIdListFilt sess_list;
	PPObjCSession cs_obj;
	DraftRcptArray goods_qtty_list;
	PPObjLocation obj_loc;
	if(pLocList && !pLocList->IsEmpty())
		pLocList->CopyTo(&loc_list);
	else
		THROW(obj_loc.GetWarehouseList(&loc_list, 0));
	for(uint i = 0; i < loc_list.getCount(); i++) {
		PPID loc_id = loc_list.at(i);
		if(cs_obj.P_Tbl->GetActiveSessList(loc_id, &sess_list) > 0) {
			ObjIdListFilt check_list;
			if(LoadChecksByList(&sess_list, 0, &check_list, 0) > 0) {
				CCheckLineArray lines_list;
				if(LoadLinesByList(0, &check_list, &lines_list) > 0) {
					uint pos = lines_list.getCount();
					if(pos) do {
						uint p = 0;
						CCheckLineTbl::Rec & r_line_rec = lines_list.at(--pos);
						DraftRcptItem dr_item;
						dr_item.GoodsID = r_line_rec.GoodsID;
						dr_item.LocID   = loc_id;
						dr_item.Qtty    = r_line_rec.Quantity;
						if(goods_qtty_list.lsearch(&dr_item, &p, PTR_CMPFUNC(_2long)))
							goods_qtty_list.at(p).Qtty += r_line_rec.Quantity;
						else
							goods_qtty_list.insert(&dr_item);
					} while(pos);
				}
			}
		}
	}
	CATCHZOK
	goods_qtty_list.sort(PTR_CMPFUNC(_2long));
	ASSIGN_PTR(pList, goods_qtty_list);
	return ok;
}

int CCheckCore::Helper_GetListByMark(const char * pText, int markLnextTextId, int sentLnextTextId, PPIDArray & rCcList, BitArray * pSentList)
{
	int    ok = -1;
	rCcList.clear();
	CALLPTRMEMB(pSentList, Clear());
	SString temp_buf;
	SString target_buf;
	UnxTextRefCore & r_utrc = PPRef->UtrC;
	TextRefEnumItem iter_item;
	StrAssocArray text_by_row_list;
	PPExtStrContainer sc;
	for(SEnum en = r_utrc.Enum(PPOBJ_CCHECK, PPTRPROP_CC_LNEXT); en.Next(&iter_item) > 0;) {
		CCheckPacket::Helper_UnpackTextExt(iter_item.S.Transf(CTRANSF_UTF8_TO_INNER), &sc, &text_by_row_list);
		uint   pos = 0;
		for(uint i = 0; i < text_by_row_list.getCount(); i++) {
			StrAssocArray::Item item = text_by_row_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			if(PPGetExtStrData(markLnextTextId, temp_buf, target_buf) > 0 && target_buf.CmpNC(pText) == 0) {
				rCcList.add(iter_item.O.Id);
				if(pSentList && sentLnextTextId) {
					if(sc.GetExtStrData(sentLnextTextId, temp_buf))
						pSentList->insert(1);
					else
						pSentList->insert(0);
				}
				ok = 1;
				break;
			}
		}
	}
	assert(!pSentList || pSentList->getCount() == rCcList.getCount());
	return ok;
}

int CCheckCore::GetListByEgaisMark(const char * pText, PPIDArray & rCcList, BitArray * pSentList)
{
	return Helper_GetListByMark(pText, CCheckPacket::lnextEgaisMark, CCheckPacket::extssEgaisUrl, rCcList, pSentList);
}

int CCheckCore::GetListByChZnMark(const char * pText, PPIDArray & rCcList)
{
	return Helper_GetListByMark(pText, CCheckPacket::lnextChZnMark, 0, rCcList, 0);
}

int CCheckCore::CalcActiveExpendByGoods(PPID goodsID, PPID locID, const char * pSerial, double * pResult)
{
	int    ok = 1;
	double result = 0.0;
	ObjIdListFilt sess_list;
	PPObjCSession cs_obj;
	if(cs_obj.P_Tbl->GetActiveSessList(locID, &sess_list) > 0) {
		ObjIdListFilt check_list;
		if(LoadChecksByList(&sess_list, 0, &check_list, 0) > 0) {
			CCheckLineArray lines_list;
			if(LoadLinesByList(goodsID, &check_list, &lines_list) > 0) {
				PPObjCashNode cn_obj;
				PPCashNode cn_rec;
				CSessionTbl::Rec cs_rec;
				CCheckTbl::Rec cc_rec;
				SString text_buf;
				SString serial_buf;
				StrAssocArray ln_text_ext;
				uint   pos = lines_list.getCount();
				if(pos) do {
					CCheckLineTbl::Rec & r_line_rec = lines_list.at(--pos);
					const PPID cc_id = r_line_rec.CheckID;
					if(Search(cc_id, &cc_rec) > 0) {
						int    skip = 0;
						if(cc_rec.Flags & CCHKF_SKIP)
							skip = 1;
						else if(!(cc_rec.Flags & CCHKF_PRINTED)) {
							if(cs_obj.Search(cc_rec.SessID, &cs_rec) > 0 && cn_obj.Fetch(cs_rec.CashNodeID, &cn_rec) > 0) {
								if(cn_rec.Flags & CASHF_SKIPUNPRINTEDCHECKS)
									skip = 1;
							}
							else
								skip = 1;
						}
						if(!skip) {
							if(isempty(pSerial)) {
								result += r_line_rec.Quantity;
							}
							else if(PPRef->UtrC.GetText(TextRefIdent(PPOBJ_CCHECK, cc_id, PPTRPROP_CC_LNEXT), text_buf) > 0) {
								text_buf.Transf(CTRANSF_UTF8_TO_INNER);
								// @v10.3.9 CCheckPacket::Helper_UnpackLineTextExt(text_buf, ln_text_ext);
								CCheckPacket::Helper_UnpackTextExt(text_buf, 0, &ln_text_ext); // @v10.3.9
								CCheckPacket::Helper_GetLineTextExt(r_line_rec.RByCheck, CCheckPacket::lnextSerial, ln_text_ext, serial_buf);
								if(serial_buf == pSerial)
									result += r_line_rec.Quantity;
							}
						}
					}
				} while(pos);
			}
		}
	}
	/*
	CATCHZOK
	*/
	ASSIGN_PTR(pResult, result);
	return ok;
}

int CCheckCore::CalcGoodsRest(PPID goodsID, LDATE dt, PPID locID, double * pRest)
{
	int    ok = 1;
	double rest = 0.0;
	double exp = 0.0; // Расход по активным синхронным сессиям
	GoodsRestParam gp;
	ObjIdListFilt sess_list;
	gp.Date    = dt;
	gp.LocID   = locID;
	gp.GoodsID = goodsID;
	THROW(BillObj->trfr->GetCurRest(gp));
	rest = gp.Total.Rest;
	THROW(CalcActiveExpendByGoods(goodsID, locID, 0, &exp));
	rest -= exp;
	CATCHZOK
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int CCheckCore::IsLostJunkCheck(PPID id, const S_GUID * pExtUuid, PPSession::RegSessData * pSessData)
{
	int    yes = 0;
	ObjTagItem tag_item;
	if(PPRef->Ot.GetTag(PPOBJ_CCHECK, id, PPTAG_CCHECK_JS_UUID, &tag_item) > 0) {
		S_GUID sess_uuid;
		if(tag_item.GetGuid(&sess_uuid)) {
			if(pExtUuid && !pExtUuid->IsZero()) {
				if(sess_uuid == *pExtUuid) {
					if(pSessData) {
						pSessData->Flags |= PPSession::RegSessData::fExtraUUID;
						pSessData->Uuid = *pExtUuid;
					}
					yes = 1;
				}
			}
			else {
				PPSession::RegSessData rsd;
				if(DS.GetRegisteredSess(sess_uuid, &rsd) > 0) {
					ASSIGN_PTR(pSessData, rsd);
					yes = 1;
				}
			}
		}
	}
	return yes;
}
//
//
//
IMPL_CMPFUNC(CCheckGoodsEntry, i1, i2) { RET_CMPCASCADE4(static_cast<const CCheckGoodsEntry *>(i1), static_cast<const CCheckGoodsEntry *>(i2), Dt, Sign, GoodsID, SerialID); }

CCheckGoodsArray::CCheckGoodsArray() : TSVector <CCheckGoodsEntry> (), Sum(0.0)
{
}

LDATE CCheckGoodsArray::GetMaxDate() const
{
	LDATE  dt = ZERODATE;
	uint   i = getCount();
	if(i) do {
		const LDATE cur_dt = at(--i).Dt;
		SETMAX(dt, cur_dt);
	} while(i);
	return dt;
}

int CCheckGoodsArray::Add(LDATE dt, const CCheckLineTbl::Rec * pRec, PPID serialID)
{
	int    ok = 1;
	int    sign = (pRec->Quantity < 0) ? -1 : +1;
	const  double qtty = fabs(pRec->Quantity);
	const  double price = intmnytodbl(pRec->Price);
	double amount = fabs(price - pRec->Dscnt) * qtty;
	uint   pos = 0;
	if(price == 0.0 && pRec->Dscnt != 0.0) {
		//
		// Специальный случай: скидка суммой на весь чек, не распределенная по строкам чека.
		// Включаем такую строку в группировку для возврата.
		//
		sign = -1;
	}
	CCheckGoodsEntry key;
	MEMSZERO(key);
	key.Dt = dt;
	key.Sign = sign;
	key.GoodsID = pRec->GoodsID;
	key.SerialID = serialID;
	if(lsearch(&key, &pos, PTR_CMPFUNC(CCheckGoodsEntry))) {
		CCheckGoodsEntry & r_entry = at(pos);
		r_entry.Qtty += qtty;
		r_entry.Sum  += amount;
	}
	else {
		key.Qtty = qtty;
		key.Sum  = amount;
		THROW_SL(insert(&key));
	}
	Sum = faddwsign(Sum, amount, sign);
	CATCHZOK
	return ok;
}

int CCheckGoodsArray::AdjustToSess(double sessAmount)
{
	int    ok = -1;
	double diff = R2(sessAmount) - R2(Sum);
	if(diff > 0.01 && diff < 1.0) {
		uint   i;
		double min_qtty  = SMathConst::Max;
		double max_price = 0.0;
		uint   last_index = 0;
		CCheckGoodsEntry * p_item;
		for(i = 0; enumItems(&i, (void **)&p_item);) {
			const double q = R6(p_item->Qtty);
			const double p = R5(fdivnz(p_item->Sum, q));
			p_item->Qtty = q;
			p_item->Sum = p * q;
		}
		for(i = 0; enumItems(&i, (void **)&p_item);) {
			if(p_item->Sign > 0) {
				const double q = p_item->Qtty;
				const double p = fdivnz(p_item->Sum, q);
				if(q > 0.0 && (q < min_qtty || (q == min_qtty && p > max_price))) {
					last_index = i;
					min_qtty   = q;
					max_price  = p;
				}
			}
		}
		if(last_index) {
			p_item = &at(last_index-1);
			p_item->Sum += diff;
			p_item->Flags |= CCheckGoodsEntry::fAdj;
			ok = 1;
		}
	}
	return ok;
}

int CCheckCore::GroupingToGoodsLines(PPID sessID, CSessTotal * pTotal, CCheckGoodsArray * pCgList, long flags, int use_ta)
{
	struct ChkItem {
		PPID   ID;
		int16  DtOffs;
	};
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj; // @v11.4.0
	LotExtCodeCore * p_lotxct = p_bobj ? p_bobj->P_LotXcT : 0; // @v11.4.0
	uint   i;
	LDATE  curdt = getcurdate_();
	SArray chk_list(sizeof(ChkItem), /*32,*/ O_ARRAY);
	{
		PPTransaction tra((flags & gglfUpdChecks) && use_ta);
		THROW(tra);
		{
			CCheckTbl::Rec rec;
			CCheckTbl::Key3 k;
			MEMSZERO(k);
			k.SessID = sessID;
			BExtQuery q(this, 3);
			q.select(this->ID, this->Dt, this->Flags, this->Amount, this->Discount, 0L).where(this->SessID == sessID);
			for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
				copyBufTo(&rec);
				if(!(rec.Flags & CCHKF_SKIP) && (!(flags & gglfSkipUnprintedChecks) || (rec.Flags & CCHKF_PRINTED))) {
					if(rec.Flags & CCHKF_NOTUSED) {
						ChkItem ci;
						ci.ID = rec.ID;
						ci.DtOffs = static_cast<int16>(diffdate(rec.Dt, curdt));
						THROW_SL(chk_list.insert(&ci));
					}
					AddRecToSessTotal(&rec, pTotal);
				}
			}
		}
		if(chk_list.getCount()) {
			const  int use_ccl_ext = BIN(CConfig.Flags & CCFLG_USECCHECKLINEEXT);
			chk_list.sort(CMPF_LONG);
			PPID   lo = static_cast<const ChkItem *>(chk_list.at(0))->ID;
			PPID   up = static_cast<const ChkItem *>(chk_list.at(chk_list.getCount()-1))->ID;
			BExtQuery q(&Lines, 0, 128);
			q.selectAll().where(Lines.CheckID >= lo && Lines.CheckID <= up);
			CCheckLineTbl::Key0 k0;
			k0.CheckID  = lo;
			k0.RByCheck = -MAXSHORT;
			PPID   prev_cc_id = 0;
			SString temp_buf;
			SStringU temp_buf_u;
			StrAssocArray cc_ln_text_ext;
			for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
				uint pos = 0;
				const PPID cc_id = Lines.data.CheckID;
				if(chk_list.bsearch(&cc_id, &pos, CMPF_LONG)) {
					if(cc_id != prev_cc_id) {
						THROW(p_ref->UtrC.GetText(TextRefIdent(PPOBJ_CCHECK, cc_id, PPTRPROP_CC_LNEXT), temp_buf));
						// (Использовать будем в виде utf8-строк) temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
                        // @v10.3.9 CCheckPacket::Helper_UnpackLineTextExt(temp_buf, cc_ln_text_ext);
						CCheckPacket::Helper_UnpackTextExt(temp_buf, 0, &cc_ln_text_ext); // @v10.3.9 
					}
					int16  dt_offs = static_cast<const ChkItem *>(chk_list.at(pos))->DtOffs;
					PPID   serial_id = 0;
					if(CCheckPacket::Helper_GetLineTextExt(Lines.data.RByCheck, CCheckPacket::lnextSerial, cc_ln_text_ext, temp_buf) > 0) {
						if(temp_buf.NotEmptyS()) {
							temp_buf_u.CopyFromUtf8(temp_buf);
							THROW(p_ref->TrT.GetSelfRefText(temp_buf_u, &serial_id, 0));
						}
					}
					else {
						// @construction {
						// @v11.4.0 {
						// Попытка решить проблему сопоставления продаж через кассу по маркам егаис с лотами:
						// Если со строкой чека связана марка, то ищем соответствующую марку и, если находим, то
						// ищем код справки Б (RefB/Ref2). Этот код будем трактовать как серийный номер со спец префиксом "refb:"
						// при этом механизм хранения ссылок на этот код в таблице CGoodsLine аналогичен варианту с серийных номеров (see above).
						if(p_lotxct && CCheckPacket::Helper_GetLineTextExt(Lines.data.RByCheck, CCheckPacket::lnextEgaisMark, cc_ln_text_ext, temp_buf) > 0) {
							if(temp_buf.NotEmptyS()) {
								//temp_buf_u.CopyFromUtf8(temp_buf);
								//THROW(p_ref->TrT.GetSelfRefText(temp_buf_u, &serial_id, 0));
								TSVector <LotExtCodeTbl::Rec> rec_list;
								if(p_lotxct->GetRecListByMark(temp_buf, rec_list) > 0) {
									assert(rec_list.getCount());
									//const int is_box = BIN(rec_list.at(0).Flags & PPLotExtCodeContainer::fBox);
									BillTbl::Rec bill_rec;
									TransferTbl::Rec trfr_rec;
									SString ref_b;
									for(uint i = 0; ref_b.IsEmpty() && i < rec_list.getCount(); i++) {
										const LotExtCodeTbl::Rec & r_rec = rec_list.at(i);
										if(r_rec.BillID && p_bobj->Fetch(r_rec.BillID, &bill_rec) > 0) {
											const PPID op_type_id = GetOpType(bill_rec.OpID);
											if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND)) {
												if(r_rec.RByBill) {
													int16 row_idx = 0;
													int   rbb_target = 0;
													PPTransferItem ti_iter;
													for(int rbb_iter = 0; !rbb_target && p_bobj->trfr->EnumItems(r_rec.BillID, &rbb_iter, &ti_iter) > 0;) {
														row_idx++;
														if(row_idx == r_rec.RByBill)
															rbb_target = ti_iter.RByBill;
													}
													if(rbb_target && p_bobj->trfr->SearchByBill(r_rec.BillID, 0, rbb_target, &trfr_rec) > 0) {
														if(trfr_rec.Flags & PPTFR_RECEIPT) {
															if(trfr_rec.LotID && p_ref->Ot.GetTagStr(PPOBJ_LOT, trfr_rec.LotID, PPTAG_LOT_FSRARINFB, ref_b) > 0) {
																;
															}
														}
														else if(trfr_rec.CorrLoc && p_bobj->trfr->SearchByBill(r_rec.BillID, 1, rbb_target, &trfr_rec) > 0) { // @v10.7.3
															PPID org_lot_id = 0;
															ReceiptTbl::Rec org_lot_rec;
															if(p_bobj->trfr->Rcpt.SearchOrigin(trfr_rec.LotID, &org_lot_id, 0, &org_lot_rec) > 0) {
																if(p_ref->Ot.GetTagStr(PPOBJ_LOT, org_lot_rec.ID, PPTAG_LOT_FSRARINFB, ref_b) > 0) {
																	;
																}
															}
														}
													}
												}
											}
										}
									}
									if(ref_b.NotEmpty()) {
										(temp_buf = "rfb:").Cat(ref_b);
										temp_buf_u.CopyFromUtf8(temp_buf);
										THROW(p_ref->TrT.GetSelfRefText(temp_buf_u, &serial_id, 0));
									}
								}
							}							
						}
						// } @construction
						// } @v11.4.0 
					}
					THROW(pCgList->Add(plusdate(curdt, dt_offs), &Lines.data, serial_id));
					prev_cc_id = cc_id;
				}
			}
			if(flags & gglfUpdChecks) {
				for(i = 0; i < chk_list.getCount(); i++) {
					const PPID chk_id = static_cast<const ChkItem *>(chk_list.at(i))->ID;
					THROW_DB(updateFor(this, 0, (this->ID == chk_id), set(this->Flags, this->Flags & ~CCHKF_NOTUSED)));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CCheckCore::GetListByCard(PPID cardID, const LDATETIME * pMoment, PPIDArray * pList)
{
	CCheckTbl::Key4 k;
	BExtQuery q(this, 4, 128);
	DBQ * dbq = &(this->SCardID == cardID);
	if(pMoment)
		dbq = &(*dbq && this->Dt >= pMoment->d);
	q.select(this->ID, 0L).where(*dbq);
	MEMSZERO(k);
	k.SCardID = cardID;
	if(pMoment) {
		k.Dt = pMoment->d;
		k.Tm = pMoment->t;
	}
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
		if(!pMoment || cmp(*pMoment, data.Dt, data.Tm) < 0)
			pList->add(data.ID);
	return pList->getCount() ? 1 : -1;
}

int CCheckCore::CreateSCardsTurnoverList(const DateRange * pPeriod, RAssocArray * pList)
{
	int    ok = 1;
	const  int new_method = 0;
	if(new_method) {
		PROFILE_START
		IterCounter cntr;
		CCheckTbl::Key1 k, k_;
		BExtQuery q(this, 1, 64);
		q.select(this->ID, this->Amount, this->SCardID, 0L).where(daterange(this->Dt, pPeriod) && this->SCardID > 0L);
		MEMSZERO(k);
		k.Dt = pPeriod->low;
		k_ = k;
		cntr.Init(q.countIterations(0, &k_, spGe));
		for(q.initIteration(false, &k, spGe); ok && q.nextIteration() > 0; PPWaitPercent(cntr.Increment()))
			if(!pList->Add(data.SCardID, MONEYTOLDBL(data.Amount), 1, 0))
				ok = PPSetErrorSLib();
		PROFILE_END
	}
	else {
		PROFILE_START
		IterCounter cntr;
		CCheckTbl::Key4 k, k_;
		BExtQuery q(this, 4, 64);
		q.select(this->ID, this->Amount, this->SCardID, 0L).where(this->SCardID > 0L && daterange(this->Dt, pPeriod));
		MEMSZERO(k);
		k.SCardID = 1L;
		k.Dt = pPeriod->low;
		k_ = k;
		cntr.Init(q.countIterations(0, &k_, spGe));
		for(q.initIteration(false, &k, spGe); ok && q.nextIteration() > 0; PPWaitPercent(cntr.Increment()))
			if(!pList->Add(data.SCardID, MONEYTOLDBL(data.Amount), 1, 0))
				ok = PPSetErrorSLib();
		PROFILE_END
	}
	return ok;
}
//
// Функция пересчитывает обороты по пластиковым картам
// Алгоритм следующий:
// 1. Обнуляем обороты по всем картам. Эта фаза избыточна, но зато гарантирует правомерность
//    инкремента оборотов по карточкам, вместо замещения.
// 2. Перебираем чеки по индексу {SCardID, Dt, Tm} (#4)
// 3. Собираем обороты по каждой карте в упорядоченном массиве {CardID, Trnovr}[] (ct_list)
// @v4.7.11 Этот шаг упразднен из-за закрытия поддержки DOS-версии {
// 4. Если размер массива превысил 2000 элементов, то сбрасываем данные из массива в таблицу
//    карточек и очищаем его.
// }
// 5. По окончании цикла безусловно сбрасываем данные из массива в таблицу карточек.
//
// За счет буферизации достигается значительное ускорение процесса.
//
// Счетчик прогресса функции оперирует значениями идентификаторов карточек, а не общим количеством
// итераций (предварительный подсчет количества итераций - очень долгий процесс).
//
int CCheckCore::RecalcSCardsTurnover(int use_ta)
{
	int    ok = 1;
	PPID   card_key = 0;
	long   max_card_id = 0;
	RAssocArray ct_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		// @todo update_for
		for(card_key = 0; Cards.searchForUpdate(0, &card_key, spGt);) {
			THROW(PPCheckUserBreak());
			Cards.data.Turnover = 0.0;
			THROW_DB(Cards.updateRec()); // @sfu
			SETMAX(max_card_id, Cards.data.ID);
		}
		{
			IterCounter cntr;
			cntr.Init(max_card_id);
			CCheckTbl::Key4 k;
			BExtQuery q(this, 4, 160);
			q.select(this->ID, this->Amount, this->SCardID, this->Flags, 0L).where(this->SCardID > 0L);
			MEMSZERO(k);
			k.SCardID = 1;
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				uint   pos = 0;
				THROW(PPCheckUserBreak());
				if(!(data.Flags & (CCHKF_JUNK|CCHKF_SKIP)))
					ct_list.Add(data.SCardID, MONEYTOLDBL(data.Amount), 1, 1);
				PPWaitPercent(cntr.Add(data.SCardID-cntr));
			}
			{
				RAssoc * p_ct;
				for(uint i = 0; ct_list.enumItems(&i, (void **)&p_ct);)
					THROW(Cards.AddTurnover(p_ct->Key, p_ct->Val, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

//double CPosProcessor::Helper_CalcSCardOpBonusAmount(const CCheckLineTbl::Rec & rItem, PPObjGoods & rGObj, PPID bonusGoodsGrpID, double * pNonCrdAmt)

int CCheckCore::GetTrnovrBySCard(PPID cardID, int alg /*PPObjSCard::gtalgXXX*/, const DateRange * pPeriod,
	PPID restrGoodsGrpID, double * pDebit, double * pCredit)
{
	PPIDArray cc_list;
	CCheckTbl::Key4 k, k_;
	double dbt = 0.0;
	double crd = 0.0;
	MEMSZERO(k);
	k.SCardID = cardID;
	if(pPeriod)
		k.Dt = pPeriod->low;
	k_ = k;
	if(search(4, &k_, spGe) && data.SCardID == cardID) {
		BExtQuery q(this, 4, 256);
		q.select(this->ID, this->Amount, this->Flags, 0L).where(this->SCardID == cardID && daterange(this->Dt, pPeriod));
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			if(!(data.Flags & (CCHKF_JUNK|CCHKF_SKIP))) {
				if(restrGoodsGrpID) {
					cc_list.add(data.ID);
				}
				else {
					const double amt = MONEYTOLDBL(data.Amount);
					if(amt > 0.0) { // Sell
						if(alg != PPObjSCard::gtalgForBonus) {
							CCheckExtTbl::Rec ext_rec;
							if(data.Flags & (CCHKF_ADDPAYM|CCHKF_ADDINCORPCRD) && GetExt(data.ID, &ext_rec) > 0) {
								//
								// При доплате по дополнительной карте сумма этой доплаты хранится с минусом.
								//
								// @v9.0.4 amt -= (intmnytodbl(ext_rec.AddPaym) - intmnytodbl(ext_rec.AddCrdCardPaym));
							}
						}
						crd += amt;
					}
					else // Return or charge
						dbt += amt; // @fixme Вероятно, здесь должно быть (dbt -= amt)
				}
			}
		}
		if(restrGoodsGrpID && cc_list.getCount()) {
			PPObjGoods goods_obj;
			cc_list.sortAndUndup();
			CCheckLineArray lines_list;
			{
				ObjIdListFilt cc_list_;
				cc_list_.Set(&cc_list);
				LoadLinesByList(0, &cc_list_, &lines_list);
			}
			for(uint i = 0; i < lines_list.getCount(); i++) {
				const CCheckLineTbl::Rec & r_line_rec = lines_list.at(i);
				double non_crd_amt = 0.0;
				const double amt = CPosProcessor::Helper_CalcSCardOpBonusAmount(r_line_rec, goods_obj, restrGoodsGrpID, &non_crd_amt);
				if(amt < 0.0)
					crd += -amt;
				else if(amt > 0.0)
					dbt += amt;
			}
		}
	}
	{
		RAssocArray cc_amt_list;
		CCheckTbl::Rec cc_rec;
		CCheckPaymTbl::Key1 pk1;
		BExtQuery q(&PaymT, 1);
		q.selectAll().where(PaymT.SCardID == cardID);
		MEMSZERO(pk1);
		pk1.SCardID = cardID;
		for(q.initIteration(false, &pk1, spGe); q.nextIteration() > 0;) {
			const long p = PaymT.data.Amount;
			const double amt = intmnytodbl(p);
			const PPID cc_id = PaymT.data.CheckID;
			if(Search(cc_id, &cc_rec) > 0 && (!pPeriod || pPeriod->CheckDate(cc_rec.Dt))) { // @v11.4.10 @fix
				if(restrGoodsGrpID) {
					cc_amt_list.Add(cc_id, amt);
					cc_list.add(cc_id);
				}
				else {
					if(p > 0)
						dbt += amt;
					else if(p < 0)
						crd -= amt;
				}
			}
		}
		if(restrGoodsGrpID && cc_amt_list.getCount()) {
			PPObjGoods goods_obj;
			cc_list.sortAndUndup();
			cc_amt_list.SortByKey(); // in order to use binary searching
			CCheckLineArray lines_list;
			{
				ObjIdListFilt cc_list_;
				cc_list_.Set(&cc_list);
				LoadLinesByList(0, &cc_list_, &lines_list);
			}
			for(uint i = 0; i < lines_list.getCount(); i++) {
				const CCheckLineTbl::Rec & r_line_rec = lines_list.at(i);
				double non_crd_amt = 0.0;
				const double base_amt = fabs(cc_amt_list.Get(r_line_rec.CheckID, 1 /*binary*/));
				if(base_amt != 0.0) {
					const double amt = CPosProcessor::Helper_CalcSCardOpBonusAmount(r_line_rec, goods_obj, restrGoodsGrpID, &non_crd_amt);
					if(amt > 0.0)
						crd += (amt / base_amt);
					else
						dbt -= (amt / base_amt);
				}
			}
		}
	}
	ASSIGN_PTR(pDebit, dbt);
	ASSIGN_PTR(pCredit, crd);
	return 1;
}

int CCheckCore::UpdateSCardOpsBySess(PPID sessID, int use_ta)
{
	int    ok = 1;
	PPObjSCardSeries scs_obj;
	PPSCardSeries2 scs_rec;
	PPIDArray credit_card_series;
	for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
		if(scs_rec.Flags & SCRDSF_CREDIT)
			THROW(credit_card_series.add(scs_rec.ID));
	}
	if(credit_card_series.getCount()) {
		CCheckTbl::Key3 k3;
		SCardTbl::Rec sc_rec;
		BExtQuery q(this, 3, 64);
		q.selectAll().where(this->SessID == sessID && this->SCardID > 0L);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			MEMSZERO(k3);
			k3.SessID = sessID;
			for(q.initIteration(false, &k3, spGe); q.nextIteration() > 0;) {
				if(!(data.Flags & (CCHKF_JUNK|CCHKF_SKIP))) {
					if(data.SCardID && Cards.Search(data.SCardID, &sc_rec) > 0 && credit_card_series.lsearch(sc_rec.SeriesID)) {
						SCardOpTbl::Rec scop_rec;
						// @v10.6.4 MEMSZERO(scop_rec);
						scop_rec.SCardID = data.SCardID;
						scop_rec.Dt = data.Dt;
						scop_rec.Tm = data.Tm;
						scop_rec.LinkObjType = PPOBJ_CCHECK;
						scop_rec.LinkObjID   = data.ID;
						scop_rec.UserID  = data.UserID;
						scop_rec.Amount  = -MONEYTOLDBL(data.Amount);
						THROW(Cards.PutOpRec(&scop_rec, 0, 0));
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int CCheckCore::RemoveZeroLinkLines(int use_ta)
{
	const  uint max_id_count = 1000L;
	int    ok = 1;
	uint   i;
	SString id_msg, msg_buf, msg_templ;
	IterCounter cntr;
	PPIDArray id_list, rmv_id_list;
	CCheckLineTbl::Key0 k0;
	BExtQuery q(&Lines, 0);
	q.select(Lines.CheckID, 0);
	PPLoadText(PPTXT_LOG_ZEROLINKCCLINE, msg_templ);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		PPInitIterCounter(cntr, &Lines);
		MEMSZERO(k0);
		for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0; PPWaitPercent(cntr.Increment())) {
			{
				const uint _idlc = id_list.getCount();
				if(_idlc > max_id_count) {
					for(i = 0; i < _idlc; i++) {
						int r = Search(id_list.at(i));
						THROW(r);
						if(r < 0)
							rmv_id_list.add(id_list.at(i));
					}
					id_list.clear();
				}
			}
			id_list.add(Lines.data.CheckID);
		}
		for(i = 0; i < id_list.getCount(); i++) {
			int r = Search(id_list.at(i));
			THROW(r);
			if(r < 0)
				rmv_id_list.add(id_list.at(i));
		}
		rmv_id_list.sortAndUndup();
		for(i = 0; i < rmv_id_list.getCount(); i++) {
			const PPID rmv_cc_id = rmv_id_list.get(i);
			THROW_DB(deleteFrom(&Lines, 0, Lines.CheckID == rmv_cc_id));
			id_msg.Z().CatEq("ID", rmv_cc_id);
			msg_buf.Printf(msg_templ, id_msg.cptr());
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int RemoveZeroLinkCCLines()
{
	int    ok = 1;
	CCheckCore cc;
	PPWaitStart();
	if(!cc.RemoveZeroLinkLines(1))
		ok = PPErrorZ();
	PPWaitStop();
	return ok;
}
//
// CGoodsLine
//
CGoodsLine::CGoodsLine(char * pFileName) : CGoodsLineTbl(pFileName)
{
}

int CGoodsLine::HasAnyLineForSess(PPID sessID)
{
	CGoodsLineTbl::Key0 k;
	MEMSZERO(k);
	k.SessID  = sessID;
	return (search(0, &k, spGe) && k.SessID == sessID) ? 1 : PPDbSearchError();
}

int CGoodsLine::Search_ForUpdate(PPID sessID, LDATE dt, PPID goodsID, int sign, PPID serialID, CGoodsLineTbl::Rec * pRec)
{
	CGoodsLineTbl::Key0 k;
	MEMSZERO(k);
	k.SessID = sessID;
	k.Dt = dt;
	k.Sign = (sign < 0) ? -1 : +1;
	k.GoodsID = goodsID;
	k.SerialID = serialID;
	return SearchByKey_ForUpdate(this, 0, &k, pRec);
}

int CGoodsLine::Add(PPID sessID, const CCheckGoodsArray * pList, int use_ta)
{
	int    ok = 1;
	CCheckGoodsEntry * p_entry;
	BExtInsert bei(this);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; pList->enumItems(&i, (void **)&p_entry);) {
			if(Search_ForUpdate(sessID, p_entry->Dt, p_entry->GoodsID, p_entry->Sign, p_entry->SerialID, 0) > 0) {
				data.Qtty += p_entry->Qtty;
				data.Rest += p_entry->Qtty;
				data.Sum  += p_entry->Sum;
				THROW_DB(updateRec()); // @sfu
			}
			else {
				CGoodsLineTbl::Rec rec;
				// @v10.7.9 @ctr MEMSZERO(rec);
				rec.SessID   = sessID;
				rec.Dt       = p_entry->Dt;
				rec.GoodsID  = p_entry->GoodsID;
				rec.SerialID = p_entry->SerialID;
				rec.Sign     = p_entry->Sign;
				rec.Qtty     = p_entry->Qtty;
				rec.Rest     = p_entry->Qtty;
				rec.Sum      = p_entry->Sum;
				THROW_DB(bei.insert(&rec));
			}
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CGoodsLine::Update(const CGoodsLineTbl::Rec & rRec, int use_ta)
{
	int    ok = -1, r;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Search_ForUpdate(rRec.SessID, rRec.Dt, rRec.GoodsID, rRec.Sign, rRec.SerialID, 0));
		if(r > 0) {
			THROW_DB(updateRecBuf(&rRec)); // @sfu
			ok = 1;
		}
		else
			PPSetError(PPERR_CGLRECNFOUND);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CGoodsLine::RemoveSess(PPID sessID)
{
	return deleteFrom(this, 0, this->SessID == sessID) ? 1 : PPSetErrorDB();
}

int CGoodsLine::UndoWritingOff(PPID sessID)
{
	CGoodsLineTbl::Key0 k0;
	MEMSZERO(k0);
	k0.SessID  = sessID;
	// @todo update_for
	while(searchForUpdate(0, &k0, spGt) && k0.SessID == sessID && k0.Sign != CGOODSLINESIGN_ALTGOODSREC) {
		data.Rest = data.Qtty;
		data.AltGoodsQtty = 0;
		if(!updateRec()) // @sfu
			return PPSetErrorDB();
	}
	return (BTROKORNFOUND) ? 1 : PPSetErrorDB();
}

int CGoodsLine::CalcSessTotal(PPID sessID, CSessTotal * pTotal)
{
	CGoodsLineTbl::Key0 k;
	BExtQuery q(this, 0);
	MEMSZERO(k);
	k.SessID = sessID;
	q.selectAll().where(this->SessID == sessID);
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
		double aggr_amount = data.Sum;
		double rest = data.Rest;
		pTotal->AggrCount++;
		if(data.Qtty != 0.0 && rest != 0.0) {
			pTotal->AggrAmount += aggr_amount;
			double aggr_rest = data.Sign * fabs(aggr_amount * rest / data.Qtty);
			pTotal->AggrRest += aggr_rest;
		}
	}
	return 1;
}
//
//
//
IMPL_CMPFUNC(CSessDfctItem_DateGoods, i1, i2) { RET_CMPCASCADE2(static_cast<const CSessDfctItem *>(i1), static_cast<const CSessDfctItem *>(i2), Dt, GoodsID); }

CSessDfctList::CSessDfctList() : TSVector <CSessDfctItem> ()
{
}

int CSessDfctList::Search(const CSessDfctItem * pItem, int unite, uint * pPos) const
{
	uint   pos = 0;
	CompFunc cf = 0;
	if(unite == uniteByGoods)
		cf = CMPF_LONG;
	else if(unite == uniteByDateGoods)
		cf = PTR_CMPFUNC(CSessDfctItem_DateGoods);
	if(cf && lsearch(pItem, &pos, cf)) {
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else
		return 0;
}

int CSessDfctList::SearchByDateGoods(LDATE dt, PPID goodsID, uint * pPos, CSessDfctItem * pItem) const
{
	uint   pos = 0;
	CSessDfctItem item;
	MEMSZERO(item);
	item.Dt = dt;
	item.GoodsID = goodsID;
	if(lsearch(&item, &pos, PTR_CMPFUNC(CSessDfctItem_DateGoods))) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pItem, at(pos));
		return 1;
	}
	else
		return 0;
}

int CSessDfctList::Add(const CSessDfctItem * pItem, int unite)
{
	int    ok = -1;
	uint   pos = 0;
	if(unite && Search(pItem, unite, &pos) > 0) {
		CSessDfctItem & r_item = at(pos);
		if(r_item.Dt != pItem->Dt)
			r_item.Dt = MAX(pItem->Dt, r_item.Dt);
		if(r_item.SessID != pItem->SessID)
			r_item.SessID = 0;
		if(r_item.AltGoodsID != pItem->AltGoodsID)
			if(!r_item.AltGoodsID)
				r_item.AltGoodsID = pItem->AltGoodsID;
		r_item.Qtty += pItem->Qtty;
		r_item.Dfct += pItem->Dfct;
		r_item.Sum  += pItem->Sum;
		r_item.AltGoodsQtty += pItem->AltGoodsQtty;
		ok = 2;
	}
	else
		ok = insert(pItem) ? 1 : PPSetErrorSLib();
	return ok;
}


CSessDfctGoodsItem::CSessDfctGoodsItem() : GoodsID(0), Qtty(0.0), Sum(0.0)
{
}

double CSessDfctGoodsItem::GetPrice() const
{
	return fabs(fdivnz(Sum, Qtty));
}

int CSessDfctGoodsList::Search(PPID goodsID, CSessDfctGoodsItem * pItem)
{
	int    ok = 0;
	uint   pos = 0;
	if(bsearch(&goodsID, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pItem, at(pos));
		ok = 1;
	}
	return ok;
}

int CSessDfctGoodsList::Add(const CSessDfctGoodsItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		uint   pos = 0;
		if(lsearch(&pItem->GoodsID, &pos, CMPF_LONG)) {
			CSessDfctGoodsItem & r_item = at(pos);
			r_item.Qtty += pItem->Qtty;
			r_item.Sum  += pItem->Sum;
		}
		else {
			THROW_SL(insert(pItem));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int CGoodsLine::GetDfctGoodsList(int sign, PPID sessID, const DateRange * pPeriod, CSessDfctGoodsList * pList)
{
	int    ok = 1;
	IterCounter cntr;
	CGoodsLineTbl::Key0 k, k_;
	BExtQuery q(this, 0, 64);
	DBQ  * dbq = 0;
	dbq = ppcheckfiltid(dbq, this->SessID, sessID);
	dbq = & (*dbq && daterange(this->Dt, pPeriod));
	dbq = ppcheckfiltid(dbq, this->Sign, sign);
	q.selectAll().where(*dbq);
	k.SessID  = sessID;
	k.Dt      = pPeriod ? pPeriod->low : ZERODATE;
	k.Sign    = sign;
	k.GoodsID = 0;
	k_ = k;
	cntr.Init(q.countIterations(0, &k_, spGe));
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0; PPWaitPercent(cntr.Increment()))
		if(R6(data.Qtty) != 0 && R6(data.Rest) != 0) {
			uint pos = 0;
			CSessDfctGoodsItem item;
			item.GoodsID = data.GoodsID;
			item.Qtty    = data.Rest * data.Sign;
			item.Sum     = fabs((data.Qtty ? data.Sum / data.Qtty : 0) * item.Qtty);
			THROW(pList->Add(&item));
		}
	pList->sort(CMPF_LONG);
	CATCHZOK
	return ok;
}

int CGoodsLine::GetDfctList(int sign, PPID sessID, PPID goodsID, const DateRange * pPeriod,
	int unite, CSessDfctList * pList, PPIDArray * pDateList)
{
	int    ok = 1;
	DBQ  * dbq = 0;
	CGoodsLineTbl::Key0 k;
	BExtQuery q(this, 0, 64);
	dbq = ppcheckfiltid(dbq, this->SessID, sessID);
	dbq = & (*dbq && daterange(this->Dt, pPeriod));
	dbq = ppcheckfiltid(dbq, this->Sign, sign);
	dbq = ppcheckfiltid(dbq, this->GoodsID, goodsID);
	q.selectAll().where(*dbq);
	k.SessID  = sessID;
	k.Dt      = pPeriod ? pPeriod->low : ZERODATE;
	k.Sign    = sign;
	k.GoodsID = goodsID;
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(R6(data.Qtty) != 0 && R6(data.Rest) != 0) {
			CSessDfctItem item;
			item.SessID  = data.SessID;
			item.Dt      = data.Dt;
			item.GoodsID = data.GoodsID;
			item.Qtty    = data.Qtty * data.Sign;
			item.Dfct    = data.Rest * data.Sign;
			item.Sum     = data.Sum;
			item.AltGoodsID   = data.AltGoodsID;
			item.AltGoodsQtty = data.AltGoodsQtty;
			THROW(pList->Add(&item, unite));
			if(pDateList && data.Dt)
				pDateList->addUnique(data.Dt);
		}
	}
	CATCHZOK
	return ok;
}

int CGoodsLine::GetDfctSubstList(PPID sessID, LAssocArray * pList)
{
	int    ok = 1;
	DBQ  * dbq = 0;
	CGoodsLineTbl::Key0 k;
	BExtQuery q(this, 0, 64);
	dbq = ppcheckfiltid(dbq, this->SessID, sessID);
	q.selectAll().where(*dbq);
	MEMSZERO(k);
	k.SessID  = sessID;
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(data.AltGoodsID) {
			THROW_SL(pList->Add(((data.Sign < 0) ? -data.GoodsID : data.GoodsID), data.AltGoodsID, 0));
		}
	}
	CATCHZOK
	return ok;
}

int CGoodsLine::SetDfctSubstList(PPID sessID, const LAssocArray * pList, int use_ta)
{
	int    ok = 1;
	CGoodsLineTbl::Key0 k0;
	MEMSZERO(k0);
	k0.SessID = sessID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(searchForUpdate(0, &k0, spGe) && k0.SessID == sessID)
			do {
				PPID   goods_id = (data.Sign < 0) ? -data.GoodsID : data.GoodsID;
				PPID   alt_goods_id = 0;
				if(pList->Search(goods_id, &alt_goods_id, 0)) {
					data.AltGoodsID = alt_goods_id;
					THROW_DB(updateRec()); // @sfu
				}
			} while(searchForUpdate(0, &k0, spNext) && k0.SessID == sessID);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
CSessGrouping::CSessGrouping() : CS(), CC(), GL(), RetOpID(GetCashRetOp()), SellOpID(GetCashOp())
{
	// @v10.9.9 ReadEquipConfig(&EqCfg);
	const long eqcfgf = CC.GetEqCfg().Flags; 
	ComplDfct   = BIN(eqcfgf & PPEquipConfig::fComplDeficit);
	IgnGenGoods = BIN(eqcfgf & PPEquipConfig::fIgnGenGoodsOnDeficit);
}

int CSessGrouping::GetSess(PPID sessID, CSessionTbl::Rec * pRec)
{
	return CS.Search(sessID, pRec);
}

long CSessionCore::GetCcGroupingFlags(const CSessionTbl::Rec & rCsRec, PPID subSessID)
{
	long   flags = 0;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec, cn_sub_rec;
	if(cn_obj.Fetch(rCsRec.CashNodeID, &cn_rec) > 0) {
		if(cn_rec.Flags & CASHF_SYNC && cn_rec.Flags & CASHF_SKIPUNPRINTEDCHECKS)
			flags |= CCheckCore::gglfSkipUnprintedChecks;
		if(cn_rec.CashType == PPCMT_CASHNGROUP && subSessID) {
			//
			// Группирующий кассовый узел не может иметь флага пропуска неотпечатанных чеков.
			// В связи с этим мы вынуждены для каждой субсессии проверять этот флаг в терминальном
			// кассовом узле.
			//
			CSessionTbl::Rec sub_rec;
			if(Search(subSessID, &sub_rec) > 0 && cn_obj.Fetch(sub_rec.CashNodeID, &cn_sub_rec) > 0) {
				SETFLAG(flags, CCheckCore::gglfSkipUnprintedChecks, (cn_sub_rec.Flags & CASHF_SYNC && cn_sub_rec.Flags & CASHF_SKIPUNPRINTEDCHECKS));
			}
		}
	}
	return flags;
}

int CSessGrouping::Grouping(PPID sessID, CSessTotal * pTotal, const LAssocArray * pSubstList, int use_ta)
{
	int    ok = 1, r;
	long   ggl_flags = CCheckCore::gglfUpdChecks;
	CSessTotal total;
	CSessionTbl::Rec sess_rec;
	PPIDArray sub_sess_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(CS.Search(sessID, &sess_rec) > 0);
		THROW(r = CS.GetSubSessList(sessID, &sub_sess_list));
		{
			CCheckGoodsArray cg_list;
			if(r > 0) {
				for(uint j = 0; j < sub_sess_list.getCount(); j++) {
					PPID   sub_id = sub_sess_list.at(j);
					CSessTotal sub_total;
					long _f = (ggl_flags | CS.GetCcGroupingFlags(sess_rec, sub_id));
					THROW(CC.GroupingToGoodsLines(sub_id, &sub_total, &cg_list, _f, 0));
					THROW(CS.UpdateTotal(sub_id, &sub_total, 0, CSESSINCMPL_COMPLETE, 0));
					total.Add(&sub_total);
				}
			}
			else {
				long _f = (ggl_flags | CS.GetCcGroupingFlags(sess_rec, 0));
				THROW(CC.GroupingToGoodsLines(sessID, &total, &cg_list, _f, 0));
			}
			cg_list.AdjustToSess(total.Amount);
			THROW(GL.Add(sessID, &cg_list, 0));
			if(pSubstList)
				THROW(GL.SetDfctSubstList(sessID, pSubstList, 0));
		}
		THROW(CS.UpdateTotal(sessID, &total, 0, CSESSINCMPL_GLINES, 0));
		ASSIGN_PTR(pTotal, total);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CSessGrouping::UndoGrouping(PPID sessID, LAssocArray * pSubstList, int use_ta)
{
	int    ok = 1;
	PPIDArray sub_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pSubstList)
			THROW(GL.GetDfctSubstList(sessID, pSubstList));
		THROW(GL.RemoveSess(sessID));
		THROW(CS.GetSubSessList(sessID, &sub_list));
		uint   c = sub_list.getCount();
		if(c) do {
			PPID sub_id = sub_list.at(--c);
			THROW(GL.RemoveSess(sub_id));
			THROW(CC.SetFlagsBySess(sub_id, CCHKF_NOTUSED, 0, 0));
		} while(c);
		THROW(CC.SetFlagsBySess(sessID, CCHKF_NOTUSED, 0, 0));
		THROW(CS.SetSessIncompletness(sessID, CSESSINCMPL_CHECKS, 0));
		DS.LogAction(PPACN_UNDOCSESSGRPNG, PPOBJ_CSESSION, sessID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

struct CgBillCvtItem { // @flat
	LDATE  Dt;
	int16  Sign;
	int16  PadSign;        // @alignment
	PPID   GoodsID;
	PPID   SerialID;       //
	char   GoodsName[64];
};

IMPL_CMPFUNC(CgBillCvtItem_GoodsName, i1, i2)
{
	const CgBillCvtItem * p1 = static_cast<const CgBillCvtItem *>(i1);
	const CgBillCvtItem * p2 = static_cast<const CgBillCvtItem *>(i2);
	if(p1->Dt < p2->Dt)
		return -1;
	else if(p1->Dt > p2->Dt)
		return 1;
	else if(p1->Sign < p2->Sign)
		return -1;
	else if(p1->Sign > p2->Sign)
		return 1;
	else
		return stricmp866(p1->GoodsName, p2->GoodsName);
}

class CgBillCvtList : public TSVector <CgBillCvtItem> {
public:
	CgBillCvtList() : TSVector <CgBillCvtItem> ()
	{
	}
	int    Add(const CGoodsLineTbl::Rec *);
private:
	PPObjGoods GObj;
    LAssocArray UnifyGoodsList; // Список подстановки объединенных товаров. {OrgGoodsID, ReplacmentGoodsID}
};

int CgBillCvtList::Add(const CGoodsLineTbl::Rec * pRec)
{
	Goods2Tbl::Rec goods_rec;
	const  PPID goods_id = pRec->GoodsID;
	PPID   founded_goods_id = 0;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	{
		PPID   temp_goods_id = goods_id;
		do {
			if(GObj.Fetch(temp_goods_id, &goods_rec) > 0) {
				founded_goods_id = goods_rec.ID;
			}
		} while(!founded_goods_id && p_sj && p_sj->GetLastObjUnifyEvent(PPOBJ_GOODS, temp_goods_id, &temp_goods_id, 0) > 0);
	}
	if(founded_goods_id && founded_goods_id != goods_id) {
		UnifyGoodsList.Add(goods_id, founded_goods_id, 0);
		//
		// Если товара goods_id был замещен в результате объединения другим товаром, то
		// здесь мы все равно в список добавляем оригинальный идентификатор.
		// Функция ConvertSign потом обработает эту ситуацию.
		//
	}
	if(founded_goods_id || GObj.Fetch(pRec->AltGoodsID, &goods_rec) > 0) {
		CgBillCvtItem item;
		MEMSZERO(item);
		item.GoodsID = goods_id;
		item.SerialID = pRec->SerialID;
		item.Dt   = pRec->Dt;
		item.Sign = pRec->Sign;
		STRNSCPY(item.GoodsName, goods_rec.Name);
		return insert(&item) ? 1 : PPSetErrorSLib();
	}
	else
		return -1;
}
//
// Без транзакции проводит документ pPack.
//
int CSessGrouping::TurnBill(PPBillPacket * pPack, CSessTotal * pTotal, int isRet, OptimalAmountDamper * pOad)
{
	int    ok = -1;
	if(pPack && pPack->GetTCount()) {
		PPObjBill * p_bobj = BillObj;
		pPack->SetQuantitySign(-1);
		if(CC.GetEqCfg().Flags & PPEquipConfig::fWrOffPartStrucs)
			pPack->InsertPartitialStruc();
		p_bobj->SubstMemo(pPack);
		if(p_bobj->__TurnPacket(pPack, 0, 1, 0)) {
			if(pOad) {
				for(uint i = 0; i < pPack->GetTCount(); i++) {
					const PPTransferItem & r_ti = pPack->ConstTI(i);
					pOad->Probe(fabs(r_ti.Qtty()), r_ti.NetPrice(), i, pPack->Rec.ID);
				}
			}
			pTotal->WrOffBillCount++;
			pTotal->WrOffAmount += pPack->GetAmount(isRet);
			Logger.LogAcceptMsg(PPOBJ_BILL, pPack->Rec.ID, 0);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int CSessGrouping::ConvertSign(const int sign, const PPID sessID, const PPID locID, const void * pData, CSessTotal * pTotal, OptimalAmountDamper * pOad)
{
	int    ok = 1, r;
	const  PPID  op_id = (sign < 0) ? RetOpID : SellOpID;
	if(op_id > 0) {
		PPObjBill * p_bobj = BillObj;
		//
		// Конвертация осуществляется в несколько этапов:
		//   1. (stgMain)  Основной цикл, в процессе которого функция пытается внести в товарные документы
		//      списания каждую позицию. В ходе этого цикла по ряду позиций может быть обнаружен дефицит.
		//      В этом случае функция вносит этот дефицит в список для дальнейшей комплектации (только если
		//      конфигурация предполагает возможность комплектации дефицита во время списания).
		//
		//      После завершения цикла функция собственно и комплектует дефицит.
		//   2. (stgCompl) Цикл, в процессе которого списываются позиции, по которым был скомплектован дефицит.
		//   3. (stgSubst) В ходе предыдущих двух этапов формировался список позиций (subst_pos_list),
		//      для которых задан альтернативный (подстановочный) товар. На последнем этапе функция пытается провести
		//      дефицитное количество по подстановочным товарам.
		//
		// Важно: все эти этапы разделены из-за того, что при попытке одновременной подстановки и комплектации
		//   могут возникать коллизии одновременного расхода из одного и того же лота.
		//   Например: допустим имеется два товара "BRED" и "BRED 1/2" (буханка хлеба и пол-буханки, соответственно).
		//   Товар "BRED 1/2" может быть скомплектован из "BRED". Если в кассовой сессии продавались оба наименования,
		//   то после внесения в документ товара "BRED" (но до проведения этого документа) функция комплектации для "BRED 1/2"
		//   может использовать тот же лот "BRED". В результате, при попытке провести этот документ появится ошибка
		//   "Остаток товара слишком мал".
		//
		enum Stage {
			stgMain = 1,
			stgCompl,
			stgSubst,
			stgFinish
		};
		Stage  stage = stgMain; // Тег этапа обработки
		const CgBillCvtList * p_list = static_cast<const CgBillCvtList *>(pData);
		const PPID autocompl_op_id = CConfig.AutoComplOp;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		PPObjGoods goods_obj;
		SString fmt_buf, msg_buf;
		SString serial_buf;
		SStringU temp_buf_u;
		Goods2Tbl::Rec goods_rec;
		LongArray survey_pos_list; // Список индексов позиций в p_list, которые должны быть обработаны в процессе очередного этапа.
		LongArray subst_pos_list; // Список индексов позиций, для которых следует попробовать провести альтернативный товар.
		uint   i, j;
		for(i = 0; i < p_list->getCount(); i++)
			survey_pos_list.add(static_cast<long>(i));
		while(stage != stgFinish && survey_pos_list.getCount()) {
			TSCollection <PUGL> compl_dfct_list;
			LDATE  prev_dt = ZERODATE;
			PPBillPacket pack;
			for(j = 0; j < survey_pos_list.getCount(); j++) {
				const uint item_pos = static_cast<uint>(survey_pos_list.get(j));
				const CgBillCvtItem & r_item = p_list->at(item_pos);
				CGoodsLineTbl::Rec cg_rec;
				r = 0;
				if((sign > 0 && r_item.Sign > 0) || (sign < 0 && r_item.Sign < 0)) {
					CGoodsLineTbl::Key0 k0;
					MEMSZERO(k0);
					k0.SessID  = sessID;
					k0.Dt      = r_item.Dt;
					k0.Sign    = (r_item.Sign < 0) ? -1 : +1;
					k0.GoodsID = r_item.GoodsID;
					k0.SerialID = r_item.SerialID;
					THROW(r = SearchByKey(&GL, 0, &k0, &cg_rec));
				}
				if(r > 0) {
					ILTI   ilti;
					PPID   goods_id = (stage == stgSubst) ? cg_rec.AltGoodsID : cg_rec.GoodsID;
					double qtty = fabs(R6(cg_rec.Rest)) * cg_rec.Sign;
					const  uint cvt_flags = (IgnGenGoods ? 0 : CILTIF_USESUBST) | CILTIF_ALLOWZPRICE | CILTIF_SUBSTSERIAL;
					if(r_item.Dt != prev_dt || pack.CheckLargeBill(0)) {
						if(prev_dt)
							THROW(TurnBill(&pack, pTotal, BIN(sign < 0), pOad));
						THROW(pack.CreateBlank2(op_id, r_item.Dt, locID, 0));
						pack.SetPoolMembership(PPBillPacket::bpkCSess, sessID);
					}
					if(qtty != 0.0 && R6(cg_rec.Qtty) != 0.0) {
						PPID   founded_goods_id = 0;
						{
							PPID   temp_goods_id = goods_id;
							do {
								if(goods_obj.Fetch(temp_goods_id, &goods_rec) > 0) {
									founded_goods_id = goods_rec.ID;
								}
							} while(!founded_goods_id && p_sj && p_sj->GetLastObjUnifyEvent(PPOBJ_GOODS, temp_goods_id, &temp_goods_id, 0) > 0);
						}
						if(founded_goods_id) { // Защита от вставки в документ несуществующего товара
							if(founded_goods_id != goods_id)
								Logger.Log(PPFormatT(PPTXT_LOG_UNIFGOODSSUBST, &msg_buf, goods_id, founded_goods_id));
							ilti.GoodsID  = founded_goods_id;
							ilti.Price    = TR5(fabs(cg_rec.Sum / cg_rec.Qtty));
							ilti.SetQtty(-qtty);
							serial_buf.Z();
							if(cg_rec.SerialID && stage != stgSubst) { // Для альтернативной подстановки серию не учитываем
								if(PPRef->TrT.SearchSelfRef(cg_rec.SerialID, temp_buf_u) > 0) {
									temp_buf_u.CopyToMb(CP_UTF8, serial_buf);
									serial_buf.Transf(CTRANSF_UTF8_TO_INNER);
								}
							}
							if(qtty > 0.0 && sign > 0) {
								THROW(p_bobj->ConvertILTI(&ilti, &pack, 0, cvt_flags, serial_buf));
								if(ComplDfct && autocompl_op_id && R6(ilti.Rest) != 0.0) {
									PUGL * p_pugl = compl_dfct_list.CreateNewItem();
									THROW_SL(p_pugl);
									THROW(p_pugl->Add(&ilti, pack.Rec.LocID, item_pos, pack.Rec.Dt));
									p_pugl->SetHeader(&pack.Rec);
								}
							}
							else if(qtty < 0.0 && sign < 0) {
								THROW(p_bobj->ConvertILTI(&ilti, &pack, 0, cvt_flags, serial_buf));
							}
							if(R6(ilti.Rest + cg_rec.Rest) != 0.0) {
								cg_rec.Rest = -ilti.Rest;
								if(stage == stgSubst)
									cg_rec.AltGoodsQtty = static_cast<float>(fabs(ilti.Quantity - ilti.Rest));
								THROW(GL.Update(cg_rec, 0) > 0);
							}
						}
						else
							Logger.Log(PPFormatT(PPTXT_LOG_GOODSNFOUND, &msg_buf, goods_id));
						if(stage != stgSubst && cg_rec.AltGoodsID && cg_rec.AltGoodsID != cg_rec.GoodsID)
							subst_pos_list.addUnique(static_cast<long>(item_pos));
					}
					prev_dt = r_item.Dt;
					PPWaitPercent(item_pos+1, p_list->getCount(), WMsg);
				}
			}
			THROW(TurnBill(&pack, pTotal, BIN(sign < 0), pOad));
			survey_pos_list.clear();
			//
			// Переключение тега этапа
			//
			if(stage == stgMain) {
				stage = stgSubst;
				if(compl_dfct_list.getCount()) {
					for(j = 0; j < compl_dfct_list.getCount(); j++) {
						PUGL * p_pugl = compl_dfct_list.at(j);
						if(p_pugl) {
							PPID   compl_bill_id = 0;
							THROW(r = p_bobj->CreateModifByPUGL(autocompl_op_id, &compl_bill_id, p_pugl, sessID));
							if(r > 0) {
								Logger.LogAcceptMsg(PPOBJ_BILL, compl_bill_id, 0);
								for(i = 0; i < p_pugl->getCount(); i++)
									survey_pos_list.addUnique(static_cast<long>(static_cast<const PUGI *>(p_pugl->at(i))->Pos));
								stage = stgCompl;
							}
						}
					}
				}
			}
			else if(stage == stgCompl)
				stage = stgSubst;
			else
				stage = stgFinish;
			//
			if(stage == stgSubst)
				survey_pos_list.addUnique(&subst_pos_list);
		}
	}
	CATCHZOK
	return ok;
}

int CSessGrouping::ConvertDeficit(PPID sessID, PPID locID, CSessTotal * pTotal)
{
	int    ok = -1, r = 1;
	PPObjBill * p_bobj = BillObj;
	const PPConfig & r_cfg = LConfig;
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	if(r_eq_cfg.OpOnDfctThisLoc || r_eq_cfg.OpOnDfctOthrLoc) {
		PPObjGoods goods_obj;
		CSessionTbl::Rec sess_rec;
		PPCashNode cn_rec;
		TSCollection <PPBillPacket> pack_list;
		THROW_PP(!r_eq_cfg.OpOnDfctThisLoc || IsDraftOp(r_eq_cfg.OpOnDfctThisLoc), PPERR_INVCSESSDFCTOP);
		THROW_PP(!r_eq_cfg.OpOnDfctOthrLoc || IsDraftOp(r_eq_cfg.OpOnDfctOthrLoc), PPERR_INVCSESSDFCTOP);
		THROW(CS.Search(sessID, &sess_rec) > 0);
		THROW(SearchObject(PPOBJ_CASHNODE, sess_rec.CashNodeID, &cn_rec) > 0);
		{
			SString bill_code;
			PPBillPacket * p_pack = 0;
			r = 1;
			for(PPID bill_id = 0; p_bobj->EnumMembersOfPool(PPASS_CSDBILLPOOL, sessID, &bill_id) > 0;) {
				int    r2 = p_bobj->Search(bill_id, 0);
				THROW(r2);
				if(r2 < 0) {
					Logger.LogMsgCode(mfError, PPERR_ABSENBILLPOOLMEMB, bill_code.Z().Cat(bill_id));
					THROW(p_bobj->P_Tbl->RemoveFromPool(bill_id, PPASS_CSDBILLPOOL, sessID, 0));
				}
				else {
					THROW_MEM(p_pack = new PPBillPacket);
					THROW(p_bobj->ExtractPacket(bill_id, p_pack) > 0);
					PPObjBill::MakeCodeString(&p_pack->Rec, 1, bill_code);
					if(p_pack->IsDraft()) {
						if(p_pack->Rec.Flags & BILLF_WRITEDOFF) {
							Logger.LogMsgCode(mfInfo, PPINF_WRDOFFBILLINDFCTPOOL, bill_code);
							r = -1;
						}
						THROW(p_pack->RemoveRows(0, 0));
						THROW_SL(pack_list.insert(p_pack));
					}
					else {
						Logger.LogMsgCode(mfInfo, PPINF_NOTDRAFTINDFCTPOOL, bill_code);
						ZDELETE(p_pack);
					}
				}
			}
		}
		if(r > 0) {
			const  LDATE save_oper_date = r_cfg.OperDate;
			uint   i = 0, j, k;
			PPIDArray date_list;
			CSessDfctList dfct_list;
			CSessDfctItem * p_item, item;
			StrAssocArray goods_name_list;
			Goods2Tbl::Rec goods_rec;
			const  PPID assc_id = (cn_rec.GoodsLocAssocID && !(cn_rec.ExtFlags & CASHFX_GLASSOCPRINTONLY)) ? cn_rec.GoodsLocAssocID : PPASS_GOODS2LOC;
			GoodsToObjAssoc g2la(assc_id, PPOBJ_LOCATION);
			THROW(g2la.IsValid());
			THROW(g2la.Load());
			THROW(GL.GetDfctList(+1, sessID, 0, 0, CSessDfctList::uniteByDateGoods, &dfct_list, &date_list));
			for(i = 0; dfct_list.enumItems(&i, (void **)&p_item);) {
				if(!goods_name_list.Search(p_item->GoodsID) && goods_obj.Fetch(p_item->GoodsID, &goods_rec) > 0)
					THROW_SL(goods_name_list.Add(p_item->GoodsID, goods_rec.Name));
			}
			goods_name_list.SortByText();
			date_list.sort();
			for(k = 0; k < date_list.getCount(); k++) {
				const LDATE date = *reinterpret_cast<const LDATE *>(&date_list.at(k));
				for(i = 0; i < goods_name_list.getCount(); i++) {
					uint   pos = 0;
					PPID   goods_id = goods_name_list.Get(i).Id;
					if(dfct_list.SearchByDateGoods(date, goods_id, &pos, &item)) {
						int    is_other_loc = 0;
						PPID   op_id = 0;
						PPID   loc_id = 0;
						PPBillPacket * p_pack = 0;
						int    r = g2la.Get(goods_id, &loc_id);
						THROW(r);
						if(r < 0 || loc_id == 0)
							loc_id = locID;
						if(loc_id != locID) {
							op_id = CC.GetEqCfg().OpOnDfctOthrLoc;
							is_other_loc = 1;
						}
						else
							op_id = CC.GetEqCfg().OpOnDfctThisLoc;
						if(op_id) {
							ReceiptTbl::Rec lot_rec;
							PPTransferItem ti;
							PPOprKind op_rec;
							for(j = 0; !p_pack && j < pack_list.getCount(); j++) {
								PPBillPacket * p = pack_list.at(j);
								if(p && p->Rec.LocID == loc_id && p->Rec.Dt == date)
									p_pack = p;
							}
							if(!p_pack) {
								THROW_SL(p_pack = pack_list.CreateNewItem());
								THROW(p_pack->CreateBlank2(op_id, date, loc_id, 0));
								// @v11.1.12 PPGetWord(PPWORD_AT_AUTO, 0, p_pack->Rec.Memo, sizeof(p_pack->Rec.Memo));
								PPGetWord(PPWORD_AT_AUTO, 0, p_pack->SMemo); // @v11.1.12
								p_pack->SetPoolMembership(PPBillPacket::bpkCSessDfct, sessID);
							}
							if(GetOpData(p_pack->Rec.OpID, &op_rec) > 0) {
								if(op_rec.AccSheetID == r_cfg.LocAccSheetID)
									p_pack->Rec.Object = PPObjLocation::WarehouseToObj(locID);
								if(op_rec.AccSheet2ID == r_cfg.LocAccSheetID)
									p_pack->Rec.Object2 = PPObjLocation::WarehouseToObj(locID);
							}
							THROW(ti.Init(&p_pack->Rec));
							ti.GoodsID  = goods_id;
							ti.Quantity_ = item.Dfct;
							ti.SetupSign(p_pack->Rec.OpID);
							DS.SetOperDate(date);
							if(::GetCurGoodsPrice(goods_id, loc_id, GPRET_MOSTRECENT, 0, &lot_rec) > 0) {
								ti.UnitPerPack = lot_rec.UnitPerPack;
								ti.Cost = R5(lot_rec.Cost);
								// @v9.3.9 (Будет безусловно устанавливаться средняя цена по чекам) ti.Price = R5(lot_rec.Price); // @v7.3.1
							}
							if(ti.Price <= 0.0)
								ti.Price = TR5(item.Sum / item.Qtty);
							if(is_other_loc && (r_eq_cfg.Flags & PPEquipConfig::fIntrPriceByRetailRules || r_eq_cfg.QuotKindID)) {
								int    r = 0;
								double price = 0.0;
								PPID   quotk_id = NZOR(r_eq_cfg.QuotKindID, PPQUOTK_BASE);
								assert(quotk_id); // @paranoic
								double quot = 0.0;
								//
								// Если, в случае формирования документа на "чужом" складе, котировка должна
								// браться по складу, которому принадлежит сессия, то loc_id следует заменить на locID
								//
								const QuotIdent qi(QIDATE(date), loc_id, quotk_id);
								if(goods_obj.GetQuotExt(goods_id, qi, ti.Cost, lot_rec.Price, &quot, 1) > 0) {
									price = quot;
									r = 1;
								}
								if(r)
									ti.Price = price;
							}
							DS.SetOperDate(save_oper_date);
							THROW(p_pack->InsertRow(&ti, 0));
						}
					}
				}
			}
			for(i = 0; i < pack_list.getCount(); i++) {
				PPBillPacket * p_pack = pack_list.at(i);
				if(p_pack) {
					THROW(p_pack->InitAmounts(0));
					if(p_pack->Rec.ID) {
						THROW(p_bobj->UpdatePacket(p_pack, 0));
						Logger.LogAcceptMsg(PPOBJ_BILL, p_pack->Rec.ID, 1);
					}
					else {
						THROW(p_bobj->TurnPacket(p_pack, 0));
						Logger.LogAcceptMsg(PPOBJ_BILL, p_pack->Rec.ID, 0);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int CSessGrouping::Convert(PPID sessID, PPID locID, const void * pData, CSessTotal * pTotal, int use_ta)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	const PPConfig & r_cfg = LConfig;
	const PPID   save_loc  = r_cfg.Location;
	const LDATE  save_date = r_cfg.OperDate; // @v10.8.10 LConfig.OperDate-->getcurdate_()
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg(); // @v10.9.9
	DS.SetLocation(locID);
	OptimalAmountDamper oad;
	double local_wr_off_amount = 0.0;
	if(!(r_eq_cfg.Flags & PPEquipConfig::fDisableAdjWrOffAmount)) {
		const  PPID ret_op_id = GetCashRetOp();
		const  PPID wroff_acc_op_id = r_eq_cfg.WrOffAccOpID;
		PPID   bill_id = 0;
		CSessTotal sub_total;
		BillTbl::Rec bill_rec;
		while(p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sessID, &bill_id) > 0) {
			if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID != wroff_acc_op_id) {
				double amt = BR2(bill_rec.Amount);
				if(bill_rec.OpID == ret_op_id) {
					amt = -amt;
				}
				else {
					PPBillPacket pack;
					THROW(p_bobj->ExtractPacket(bill_id, &pack) > 0);
					for(uint i = 0; i < pack.GetTCount(); i++) {
						const PPTransferItem & r_ti = pack.ConstTI(i);
						oad.Probe(fabs(r_ti.Qtty()), r_ti.NetPrice(), i, bill_id);
					}
				}
				local_wr_off_amount += amt;
			}
		}
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ConvertSign(-1, sessID, locID, pData, pTotal, 0)); // Возвраты
		THROW(ConvertSign(+1, sessID, locID, pData, pTotal, &oad)); // Продажи
		{
			THROW(ConvertSign(-1, sessID, locID, pData, pTotal, 0)); // Возвраты (Если из-за отсутствия продаж возврат не провелся)
			THROW(ConvertSign(+1, sessID, locID, pData, pTotal, &oad)); // Продажи
		}
		THROW(ConvertDeficit(sessID, locID, pTotal));
		GL.CalcSessTotal(sessID, pTotal);
		if(!(r_eq_cfg.Flags & PPEquipConfig::fDisableAdjWrOffAmount)) {
			double to_adj_amt = (pTotal->Amount-pTotal->AggrAmount) - (local_wr_off_amount + pTotal->WrOffAmount);
			long   oad_pos, oad_ext_pos;
			if(fabs(to_adj_amt) > 0.01 && fabs(to_adj_amt) < 10.0 && oad.GetOptimal(oad_pos, &oad_ext_pos, 0, 0)) {
				PPBillPacket adj_pack;
				if(p_bobj->ExtractPacket(oad_ext_pos, &adj_pack) > 0) {
					assert(oad_pos < static_cast<long>(adj_pack.GetTCount()));
					if(oad_pos < static_cast<long>(adj_pack.GetTCount())) {
						const double org_amt = adj_pack.GetAmount(0);
						PPTransferItem & r_ti = adj_pack.TI(oad_pos);
						assert(r_ti.Qtty() != 0.0);
						if(r_ti.Qtty() != 0.0) {
							double adj = to_adj_amt / fabs(r_ti.Qtty());
							r_ti.Discount -= adj;
							THROW(adj_pack.InitAmounts());
							THROW(p_bobj->FillTurnList(&adj_pack));
							adj_pack.ProcessFlags |= PPBillPacket::pfIgnoreStatusRestr;
							THROW(p_bobj->UpdatePacket(&adj_pack, 0));
							{
								const double new_amt = adj_pack.GetAmount();
								pTotal->WrOffAmount += (new_amt - org_amt);
							}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(!ok)
		PPSaveErrContext();
	DS.SetOperDate(save_date);
	DS.SetLocation(save_loc);
	if(!ok)
		PPRestoreErrContext();
	return ok;
}

int CSessGrouping::TurnAccBill(PPID sessID, PPID locID, int use_ta) // @v11.0.4
{
	//
	// @todo Эта функция практически один-в-один повторяет соответствующий блок функции CSessGrouping::ConvertToBills
	// После отладки и проверки в реальных условиях необходимо объединить эти 2 реализации.
	// 
	int    ok = -1;
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	if(r_eq_cfg.WrOffAccOpID) {
		CSessionTbl::Rec sess_rec;
		PPObjAmountType amt_obj;
		TaxAmountIDs tais;
		BVATAccmArray bva_list;
		amt_obj.GetTaxAmountIDs(&tais, 1);
		const int do_calc_vat = BIN(r_eq_cfg.WrOffAccOpID && (tais.VatAmtID[0] || tais.VatAmtID[1] || tais.VatAmtID[2]));
		THROW(GetSess(sessID, &sess_rec) > 0);
		if(sess_rec.Incomplete == CSESSINCMPL_COMPLETE) {
			PPObjBill * p_bobj = BillObj;
			int   is_new_bill = 1;
			PPID  bill_id_to_remove = 0;
			PPBillPacket pack;
			//
			CSessTotal total;
			THROW(GL.CalcSessTotal(sessID, &total));
			THROW(CC.GetSessTotal(sessID, CS.GetCcGroupingFlags(sess_rec, sessID), &total, &bva_list));
			//
			{
				PPIDArray acc_bill_list_with_other_op; // Список бухгалтерских документов, находящихся в пуле, но с отличным от r_eq_cfg.WrOffAccOpID видом операции
				for(PPID bill_id = 0; p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sessID, &bill_id) > 0;) {
					BillTbl::Rec bill_rec;
					if(p_bobj->Search(bill_id, &bill_rec) > 0) {
						if(bill_rec.OpID == r_eq_cfg.WrOffAccOpID) {
							THROW(p_bobj->ExtractPacket(bill_id, &pack) > 0);
							is_new_bill = 0;
						}
						else if(GetOpType(bill_rec.OpID) == PPOPT_ACCTURN) {
							acc_bill_list_with_other_op.add(bill_id);
						}
					}
				}
				// @v11.0.5 {
				if(is_new_bill) {
					acc_bill_list_with_other_op.sortAndUndup(); // @paranoic
					if(acc_bill_list_with_other_op.getCount() == 1) {
						// Если мы не нашли искомый док, но в пуле сессии есть единственных бух документ, то
						// с очень высокой вероятностью это - то, что мы искали: просто удалим его ибо вид операции изменился.
						bill_id_to_remove = acc_bill_list_with_other_op.get(0);
					}
				}
				// } @v11.0.5 
			}
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				if(is_new_bill) {
					THROW(pack.CreateBlank2(r_eq_cfg.WrOffAccOpID, sess_rec.Dt, locID, 0));
				}
				else
					pack.Rec.Dt = sess_rec.Dt;
				p_bobj->SubstMemo(&pack);
				THROW(pack.Amounts.Put(PPAMT_MAIN,        0L/*curID*/, total.Amount,       1, 1));
				THROW(pack.Amounts.Put(PPAMT_DISCOUNT,    0L/*curID*/, total.Discount,     1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_CASH,     0L/*curID*/, total.Amount - total.BnkAmount - total.CSCardAmount, 1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_BANK,     0L/*curID*/, total.BnkAmount,    1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_CSCARD,   0L/*curID*/, total.CSCardAmount, 1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_AGGRSUM,  0L/*curID*/, total.AggrAmount,   1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_AGGRREST, 0L/*curID*/, total.AggrRest,     1, 1));
				if(do_calc_vat) {
					if(!is_new_bill) {
						THROW(pack.Amounts.Put(tais.VatAmtID[0], 0L/*curID*/, 0.0, 1, 1));
						THROW(pack.Amounts.Put(tais.VatAmtID[1], 0L/*curID*/, 0.0, 1, 1));
						THROW(pack.Amounts.Put(tais.VatAmtID[2], 0L/*curID*/, 0.0, 1, 1));
					}
					for(uint i = 0; i < bva_list.getCount(); i++) {
						const BVATAccm & r_bva_item = bva_list.at(i);
						if(r_bva_item.PRate == fdiv100i(tais.VatRate[0])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[0]));
							THROW(pack.Amounts.Put(tais.VatAmtID[0], 0L /* curID */, vatamt, 1, 1));
						}
						else if(r_bva_item.PRate == fdiv100i(tais.VatRate[1])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[1]));
							THROW(pack.Amounts.Put(tais.VatAmtID[1], 0L /* curID */, vatamt, 1, 1));
						}
						else if(r_bva_item.PRate == fdiv100i(tais.VatRate[2])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[2]));
							THROW(pack.Amounts.Put(tais.VatAmtID[2], 0L /* curID */, vatamt, 1, 1));
						}
					}
				}
				pack.Rec.Amount = total.Amount;
				pack.SetPoolMembership(PPBillPacket::bpkCSess, sessID);
				//
				// Что бы при ручном редактировании не снесли рассчитанные суммы зафиксируем их.
				//
				pack.Rec.Flags |= BILLF_FIXEDAMOUNTS;
				THROW(pack.InitAmounts());
				THROW(p_bobj->FillTurnList(&pack));
				if(is_new_bill) {
					// @v11.0.5 {
					if(bill_id_to_remove) {
						THROW(p_bobj->RemovePacket(bill_id_to_remove, 0));
					}
					// } @v11.0.5 
					THROW(p_bobj->TurnPacket(&pack, 0));
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
				}
				else {
					THROW(p_bobj->UpdatePacket(&pack, 0));
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ok = 0;
		Logger.LogLastError();
	ENDCATCH
	return ok;
}

int CSessGrouping::ConvertToBills(PPID sessID, PPID locID, const char * pMsg, uint sessNo, uint sessCount, int use_ta)
{
	int    ok = 1;
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg(); // @v10.9.9
	if(!(r_eq_cfg.Flags & PPEquipConfig::fCloseSessTo10Level)) {
		WMsg = pMsg;
		PPObjBill * p_bobj = BillObj;
		if(sessCount > 1 && sessNo <= sessCount)
			WMsg.Space().Cat(sessNo+1).CatChar('/').Cat(sessCount);
		PPObjAmountType amt_obj;
		TaxAmountIDs tais;
		BVATAccmArray bva_list;
		amt_obj.GetTaxAmountIDs(&tais, 1);
		const int do_calc_vat = BIN(r_eq_cfg.WrOffAccOpID && (tais.VatAmtID[0] || tais.VatAmtID[1] || tais.VatAmtID[2]));
		CgBillCvtList list;
		CSessTotal total;
		CSessTotal preserve_total;
		CSessionTbl::Rec sess_rec;
		THROW(GetSess(sessID, &sess_rec) > 0);
		{
			PPIDArray sub_list;
			CS.GetSubSessList(sessID, &sub_list);
			sub_list.add(sessID);
			for(uint i = 0; i < sub_list.getCount(); i++) {
				const PPID sess_id = sub_list.at(i);
				CSessTotal sub_total;
				long _f = CS.GetCcGroupingFlags(sess_rec, sess_id);
				THROW(CC.GetSessTotal(sess_id, _f, &sub_total, do_calc_vat ? &bva_list : 0));
				total.Add(&sub_total);
			}
			preserve_total = total;
		}
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			{
				//
				// Извлекаем список товаров, которые должны попасть в документы списания //
				//
				CGoodsLineTbl::Key0 k;
				BExtQuery q(&GL, 0);
				q.selectAll().where(GL.SessID == sessID);
				MEMSZERO(k);
				k.SessID = sessID;
				for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;)
					THROW(list.Add(&GL.data));
				//
				// Список сортируем по наименованию товара, дабы в документах строки были отсортированы также
				//
				list.sort(PTR_CMPFUNC(CgBillCvtItem_GoodsName));
			}
			THROW(Convert(sessID, locID, &list, &total, 0));
			if(r_eq_cfg.WrOffAccOpID) {
				int   is_new_bill = 1;
				PPBillPacket pack;
				{
					for(PPID bill_id = 0; p_bobj->P_Tbl->EnumMembersOfPool(PPASS_CSESSBILLPOOL, sessID, &bill_id) > 0;) {
						BillTbl::Rec bill_rec;
						if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == r_eq_cfg.WrOffAccOpID) {
							THROW(p_bobj->ExtractPacket(bill_id, &pack) > 0);
							is_new_bill = 0;
						}
					}
				}
				if(is_new_bill) {
					THROW(pack.CreateBlank2(r_eq_cfg.WrOffAccOpID, sess_rec.Dt, locID, 0));
				}
				else
					pack.Rec.Dt = sess_rec.Dt;
				p_bobj->SubstMemo(&pack);
				THROW(pack.Amounts.Put(PPAMT_MAIN,        0L/*curID*/, total.Amount,       1, 1));
				THROW(pack.Amounts.Put(PPAMT_DISCOUNT,    0L/*curID*/, total.Discount,     1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_CASH,     0L/*curID*/, total.Amount - total.BnkAmount - total.CSCardAmount, 1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_BANK,     0L/*curID*/, total.BnkAmount,    1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_CSCARD,   0L/*curID*/, total.CSCardAmount, 1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_AGGRSUM,  0L/*curID*/, total.AggrAmount,   1, 1));
				THROW(pack.Amounts.Put(PPAMT_CS_AGGRREST, 0L/*curID*/, total.AggrRest,     1, 1));
				if(do_calc_vat) {
					if(!is_new_bill) {
						THROW(pack.Amounts.Put(tais.VatAmtID[0], 0L/*curID*/, 0.0, 1, 1));
						THROW(pack.Amounts.Put(tais.VatAmtID[1], 0L/*curID*/, 0.0, 1, 1));
						THROW(pack.Amounts.Put(tais.VatAmtID[2], 0L/*curID*/, 0.0, 1, 1));
					}
					for(uint i = 0; i < bva_list.getCount(); i++) {
						const BVATAccm & r_bva_item = bva_list.at(i);
						if(r_bva_item.PRate == fdiv100i(tais.VatRate[0])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[0]));
							THROW(pack.Amounts.Put(tais.VatAmtID[0], 0L /* curID */, vatamt, 1, 1));
						}
						else if(r_bva_item.PRate == fdiv100i(tais.VatRate[1])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[1]));
							THROW(pack.Amounts.Put(tais.VatAmtID[1], 0L /* curID */, vatamt, 1, 1));
						}
						else if(r_bva_item.PRate == fdiv100i(tais.VatRate[2])) {
							double vatamt = r_bva_item.PTrnovr * SalesTaxMult(fdiv100i(tais.VatRate[2]));
							THROW(pack.Amounts.Put(tais.VatAmtID[2], 0L /* curID */, vatamt, 1, 1));
						}
					}
				}
				pack.Rec.Amount = total.Amount;
				pack.SetPoolMembership(PPBillPacket::bpkCSess, sessID);
				//
				// Что бы при ручном редактировании не снесли рассчитанные суммы зафиксируем их.
				//
				pack.Rec.Flags |= BILLF_FIXEDAMOUNTS;
				THROW(pack.InitAmounts());
				THROW(p_bobj->FillTurnList(&pack));
				if(is_new_bill) {
					THROW(p_bobj->TurnPacket(&pack, 0));
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
				}
				else {
					THROW(p_bobj->UpdatePacket(&pack, 0));
				}
			}
			THROW(CS.UpdateTotal(sessID, &total, 1, CSESSINCMPL_COMPLETE, 0));
			THROW(tra.Commit());
		}
	}
	else {
		Logger.LogMsgCode(mfError, PPERR_CSESSCOMPLLOCKED, 0);
		ok = -1;
	}
	CATCH
		ok = 0;
		Logger.LogLastError();
	ENDCATCH
	return ok;
}

int CSessGrouping::ConvertTempSessToBills(const PPIDArray * pSessList, PPID locID, PPID * pBillID, int use_ta)
{
	int    ok = -1;
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg(); // @v10.9.9
	PPID  bill_id = DEREFPTRORZ(pBillID);
	if(r_eq_cfg.OpOnTempSess) {
		PPObjBill * p_bobj = BillObj;
		BillTbl::Rec bill_rec;
		THROW_PP(IsDraftOp(r_eq_cfg.OpOnTempSess), PPERR_INVCSESSTEMPOP);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(bill_id) {
				if(p_bobj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == r_eq_cfg.OpOnTempSess)
					THROW(p_bobj->RemovePacket(bill_id, 0));
				bill_id = 0;
			}
			if(pSessList) {
				uint  i;
				CCheckGoodsArray cg_list;
				for(i = 0; i < pSessList->getCount(); i++) {
					//
					// Функция CSessGrouping::ConvertTempSessToBills на текущий момент (v5.5.7)
					// вызывается только для асинхронных кассовых узлов, поэтому, не пытаемся устанавливать
					// флаг gglfSkipUnprintedChecks в ggl_flags при вызове CC.GroupingToGoodsLines()
					//
					long   ggl_flags = 0;
					PPID   sess_id = pSessList->at(i);
					CSessTotal sub_total;
					THROW(CC.GroupingToGoodsLines(sess_id, &sub_total, &cg_list, ggl_flags, 0));
				}
				if(cg_list.getCount()) {
					PPBillPacket pack;
					LongArray rows;
					SStringU temp_buf_u;
					SString serial_buf;
					const LDATE  dt = cg_list.GetMaxDate();
					THROW(pack.CreateBlank2(r_eq_cfg.OpOnTempSess, dt, locID, 0));
					// @v11.1.12 PPGetWord(PPWORD_AT_AUTO, 0, pack.Rec.Memo, sizeof(pack.Rec.Memo));
					PPGetWord(PPWORD_AT_AUTO, 0, pack.SMemo); // @v11.1.12
					for(i = 0; i < cg_list.getCount(); i++) {
						const CCheckGoodsEntry & r_entry = cg_list.at(i);
						PPTransferItem ti;
						THROW(ti.Init(&pack.Rec));
						ti.GoodsID  = r_entry.GoodsID;
						ti.Price    = TR5(r_entry.Sum / r_entry.Qtty);
						ti.Quantity_ = r_entry.Qtty;
						ti.SetupSign(pack.Rec.OpID);
						rows.clear();
						THROW(pack.InsertRow(&ti, &rows));
						if(r_entry.SerialID) {
							if(PPRef->TrT.SearchSelfRef(r_entry.SerialID, temp_buf_u) > 0) {
								temp_buf_u.CopyToMb(CP_UTF8, serial_buf);
								serial_buf.Transf(CTRANSF_UTF8_TO_INNER);
								if(serial_buf.NotEmptyS())
									pack.LTagL.AddNumber(PPTAG_LOT_SN, &rows, serial_buf);
							}
						}
					}
					THROW(pack.InitAmounts(0));
					pack.ProcessFlags |= PPBillPacket::pfViewPercentOnTurn;
					THROW(p_bobj->TurnPacket(&pack, 0));
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					bill_id = pack.Rec.ID;
					ok = 1;
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	ASSIGN_PTR(pBillID, bill_id);
	return ok;
}

int CSessGrouping::RemoveSession(PPID sessID, int grade)
{
	int    ok = 1;
	PPObjCSession cs_obj;
	THROW(cs_obj.CheckRights(PPR_DEL));
	if(grade == CSESSINCMPL_GLINES) {
		THROW(cs_obj.UndoWritingOff(sessID, 1));
	}
	else if(grade == CSESSINCMPL_CHECKS) {
		uint   i;
		PPID * p_id;
		PPIDArray sess_ary;
		PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFFROLLBACK);
		PPTransaction tra(1);
		THROW(tra);
		THROW(cs_obj.UndoWritingOff(sessID, 0));
		THROW(GL.RemoveSess(sessID));
		THROW(CS.GetSubSessList(sessID, &sess_ary));
		for(i = 0; sess_ary.enumItems(&i, (void **)&p_id) > 0;)
			THROW(CC.SetFlagsBySess(*p_id, CCHKF_NOTUSED, 0, 0));
		THROW(CC.SetFlagsBySess(sessID, CCHKF_NOTUSED, 0, 0));
		THROW(CS.SetSessIncompletness(sessID, CSESSINCMPL_CHECKS, 0));
		THROW(tra.Commit());
	}
	else if(grade == CSESSINCMPL_COMPLETE) {
		THROW(cs_obj.RemoveObjV(sessID, 0, PPObject::use_transaction, 0));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int CSessGrouping::AttachSessToSupersess(PPID nodeID, PPID sessID, PPID * pSuperSessID, int use_ta)
{
	int     ok = -1;
	PPObjCashNode cn_obj;
	PPCashNode main_cn_rec, cn_rec;
	THROW(cn_obj.Search(nodeID, &main_cn_rec) > 0);
	if(main_cn_rec.ParentID && cn_obj.Search(main_cn_rec.ParentID, &cn_rec) > 0 && cn_rec.ExtFlags & CASHFX_UNITEGRPWROFF) {
		const PPID parent_node_id = cn_rec.ID;
		CSessionTbl::Rec csess_rec;
		CCheckTbl::Rec last_check_rec;
		THROW(CS.Search(sessID, &csess_rec) > 0);
		THROW(csess_rec.CashNodeID == nodeID); // @precondition
		if(CC.GetLastCheck(sessID, nodeID, &last_check_rec) > 0) {
			int    r;
			PPID   super_id = 0;
			PPIDArray super_candidate_list;
			LDATETIME anchor_dtm;
			PPTransaction tra(use_ta);
			THROW(tra);
			anchor_dtm.Set(last_check_rec.Dt, last_check_rec.Tm);
			THROW(CS.SetSessDateTime(sessID, anchor_dtm, 0));
			THROW(r = CS.SearchSuperCandidate(parent_node_id, anchor_dtm, &super_candidate_list));
			if(r > 0) {
				uint c = super_candidate_list.getCount();
				if(c) {
					PPIDArray child_list;
					do {
						const PPID sess_id = super_candidate_list.get(--c);
						if(GL.HasAnyLineForSess(sess_id) > 0)
							super_candidate_list.atFree(c);
						else {
							child_list.clear();
							THROW(CS.GetSubSessList(sess_id, &child_list));
							for(uint i = 0; i < child_list.getCount(); i++) {
								const PPID child_id = child_list.get(i);
								CSessionTbl::Rec child_rec;
								if(CS.Search(child_id, &child_rec) > 0) {
									CCheckTbl::Rec temp_cc_rec;
									if(CC.GetLastCheck(child_id, child_rec.CashNodeID, &temp_cc_rec) > 0) {
										LDATETIME dtm;
										dtm.Set(temp_cc_rec.Dt, temp_cc_rec.Tm);
										if(labs(diffdatetimesec(dtm, anchor_dtm)) > CSessionCore::GetUnitingPeriodSec()) {
											super_candidate_list.atFree(c);
											break;
										}
									}
								}
							}
						}
					} while(c);
				}
				if(super_candidate_list.getCount())
					super_id = super_candidate_list.get(0);
			}
			{
				PPIDArray temp_sess_list;
				temp_sess_list.add(sessID);
				if(!super_id) {
					THROW(CS.CreateSuperSess(&super_id, parent_node_id, temp_sess_list, 0));
				}
				else {
					THROW(CS.AttachToSuperSess(super_id, temp_sess_list, 0));
				}
				ok = 1;
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

struct __SessItem { // @flat
	PPID   SessID;
	LDATETIME Dtm;
	uint   SuperGroup;
	int    HasCGLines;
	PPID   SuperID;
};

IMPL_CMPFUNC(__SessItem, p1, p2)
{
	const __SessItem * p_i1 = static_cast<const __SessItem *>(p1);
	const __SessItem * p_i2 = static_cast<const __SessItem *>(p2);
	long   diff_date = 0;
	long   diff_time = diffdatetime(p_i1->Dtm, p_i2->Dtm, 2, &diff_date);
	return static_cast<int>(diff_date ? diff_date : diff_time);
}

int CSessGrouping::GroupingSessList(PPID nodeID, const PPIDArray * pList, PPIDArray * pOutList, PPIDArray * pOutTempList, int use_ta)
{
	int    ok = 1;
	uint   i, j;
	DateRange temp_sess_period;
	PPIDArray super_sess_list;
	temp_sess_period.Z();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			//
			// Группировка суперсессий
			//
			__SessItem item, * p_item, * p_item2;
			uint   last_super_num = 0;
			PPIDArray super_member_list;
			SVector temp_list(sizeof(__SessItem));
			const  long uniting_period = CSessionCore::GetUnitingPeriodSec() / 60;
			for(i = 0; i < pList->getCount(); i++) {
				PPID   sess_id = pList->at(i);
				CSessionTbl::Rec sess_rec;
				if(CS.Search(sess_id, &sess_rec) > 0) {
					CCheckTbl::Rec check_rec;
					if(CC.GetLastCheck(sess_id, sess_rec.CashNumber, &check_rec) > 0 && (sess_rec.Dt != check_rec.Dt || sess_rec.Tm != check_rec.Tm)) {
						LDATE  temp_dt = check_rec.Dt;
						LTIME  temp_tm = check_rec.Tm;
						ok = CS.CheckUniqueDateTime(sess_rec.CashNodeID, &temp_dt, &temp_tm);
						THROW_DB(updateFor(&CS, 0, (CS.ID == sess_id), set(CS.Dt, dbconst(temp_dt)).set(CS.Tm, dbconst(temp_tm))));
						//
						// Так как запись сессии была изменена, то считываем ее снова для дальнейшего использования //
						//
						THROW(CS.Search(sess_id, &sess_rec) > 0);
					}
					//
					// Корректируем период поиска временных сессий в соответствии с датой обрабатываемой сессии
					//
					if(!temp_sess_period.low)
						temp_sess_period.low = sess_rec.Dt;
					else
						SETMIN(temp_sess_period.low, sess_rec.Dt);
					SETMAX(temp_sess_period.upp, sess_rec.Dt);
					//
					if(!sess_rec.Temporary) {
						MEMSZERO(item);
						item.SessID = sess_id;
						item.Dtm.Set(sess_rec.Dt, sess_rec.Tm);
						item.SuperID = sess_rec.SuperSessID;
						item.SuperGroup = item.SuperID ? (last_super_num++) : 0;
						item.HasCGLines = BIN(GL.HasAnyLineForSess(sess_id) > 0 || (item.SuperID && GL.HasAnyLineForSess(item.SuperID) > 0)); // проверка на наличие строк группировки у суперсессии
						THROW_SL(temp_list.insert(&item));
					}
					else {
						CALLPTRMEMB(pOutTempList, addUnique(sess_id));
					}
				}
			}
			temp_list.sort(PTR_CMPFUNC(__SessItem));
			for(i = 0; temp_list.enumItems(&i, (void **)&p_item);) {
				if(!p_item->HasCGLines) {
					for(j = 0; temp_list.enumItems(&j, (void **)&p_item2);) {
						if(j != i && !p_item2->HasCGLines) {
							long   diff_date;
							long   diff_time = diffdatetime(p_item->Dtm, p_item2->Dtm, 2, &diff_date);
							if(diff_date == 0 && labs(diff_time) < uniting_period)
								if(p_item->SuperGroup == 0) {
									if(p_item2->SuperGroup == 0) {
										last_super_num++;
										p_item->SuperGroup = p_item2->SuperGroup = last_super_num;
									}
									else {
										p_item->SuperGroup = p_item2->SuperGroup;
										p_item->SuperID    = p_item2->SuperID;
									}
								}
								else if(p_item2->SuperGroup == 0) {
									p_item2->SuperGroup = p_item->SuperGroup;
									p_item2->SuperID    = p_item->SuperID;
								}
								else {
									if(p_item->SuperID) {
										if(p_item2->SuperID == 0) {
											p_item2->SuperGroup = p_item->SuperGroup;
											p_item2->SuperID    = p_item->SuperID;
										}
									}
									else if(p_item2->SuperID) {
										p_item->SuperGroup = p_item2->SuperGroup;
										p_item->SuperID    = p_item2->SuperID;
									}
									else
										p_item->SuperGroup = p_item2->SuperGroup = MIN(p_item->SuperGroup, p_item2->SuperGroup);
								}
						}
					}
				}
			}
			for(i = 1; i <= last_super_num; i++) {
				PPID   super_id = 0;
				super_member_list.clear();
				for(j = 0; temp_list.enumItems(&j, (void **)&p_item);) {
					if(p_item->SuperGroup == i) {
						THROW(super_member_list.add(p_item->SessID));
						super_id = p_item->SuperID;
					}
				}
				if(super_member_list.getCount()) {
					THROW(CS.CreateSuperSess(&super_id, nodeID, super_member_list, 0));
					THROW(CS.SetSessIncompletness(super_id, CSESSINCMPL_CHECKS, 0));
					THROW(super_sess_list.add(super_id));
				}
			}
			for(j = 0; temp_list.enumItems(&j, (void **)&p_item);)
				if(p_item->SuperGroup == 0) {
					THROW(super_sess_list.add(p_item->SessID));
				}
				else {
					THROW(CS.SetSessIncompletness(p_item->SessID, CSESSINCMPL_COMPLETE, 0));
				}
		}
		for(i = 0; i < super_sess_list.getCount(); i++) {
			THROW(Grouping(super_sess_list.at(i), 0, 0, 0));
		}
		if(pOutTempList && nodeID) {
			if(temp_sess_period.IsZero())
				temp_sess_period.SetDate(getcurdate_());
			temp_sess_period.low = plusdate(temp_sess_period.low, -7);
			THROW(CS.GetTempAsyncSessList(nodeID, &temp_sess_period, pOutTempList));
		}
		THROW_SL(pOutList->copy(super_sess_list));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CSessGrouping::ConvertSessListToBills(const PPIDArray * pSessList, PPID locID, int use_ta)
{
	int    ok = 1;
	const uint _count = pSessList->getCount();
	if(_count) {
		SString msg_buf;
		PPLoadText(PPTXT_ACSCLS_TOBILLS, msg_buf);
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < _count; i++) {
			const PPID sess_id = pSessList->get(i);
			THROW(ConvertToBills(sess_id, locID, msg_buf, i, _count, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int CSessGrouping::CompleteSession(PPID sessID)
{
	int    ok = -1;
	CSessionTbl::Rec sess_rec;
	THROW(CS.Search(sessID, &sess_rec) > 0);
	if(sess_rec.SuperSessID == 0) {
		const PPEquipConfig & r_eq_cfg = CC.GetEqCfg(); // @v10.9.9
		PPIDArray sess_list;
		THROW_PP(!(r_eq_cfg.Flags & PPEquipConfig::fCloseSessTo10Level), PPERR_CSESSCOMPLLOCKED);
		{
			PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			const PPID node_id = sess_rec.CashNodeID;
			PPID   loc_id = 0;
			THROW(cn_obj.Search(node_id, &cn_rec) > 0);
			THROW(loc_id = cn_rec.LocID);
			PPWaitStart();
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(sess_list.add(sessID));
				if(sess_rec.Incomplete == CSESSINCMPL_CHECKS)
					THROW(GroupingSessList(node_id, &sess_list, &sess_list, 0, 0));
				THROW(tra.Commit());
			}
			//
			// Транзакции формирования сессий и их списания разделены.
			//
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(ConvertSessListToBills(&sess_list, loc_id, 0));
				THROW(tra.Commit());
			}
			ok = 1;
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}
//
//
//
CTableStatus::CTableStatus(const CCheckViewItem * pCcItem)
{
	THISZERO();
	if(pCcItem) {
		TableNo = pCcItem->TableCode;
		CheckID = pCcItem->ID;
		CheckDtm.Set(pCcItem->Dt, pCcItem->Tm);
		if(pCcItem->Flags & CCHKF_SUSPENDED) {
			Status = (pCcItem->Flags & CCHKF_PREPRINT) ? sOnPayment : sBusy;
		}
		else if(pCcItem->Flags & CCHKF_ORDER && !(pCcItem->Flags & (CCHKF_CLOSEDORDER|CCHKF_SKIP))) {
			Status = sOrder;
			OrderTime = pCcItem->OrderTime;
		}
		else
			Status = sFree;
	}
}

int FASTCALL CTableStatus::Displace(const CTableStatus & rNewItem)
{
	int    ok = -1;
	LDATETIME cdtm = getcurdatetime_();
	if(TableNo == rNewItem.TableNo) {
		switch(Status) {
			case sUnkn:
			case sFree:
				if(oneof3(rNewItem.Status, sBusy, sOnPayment, sOrder))
					ok = 1;
				break;
			case sBusy:
				if(oneof2(rNewItem.Status, sBusy, sOnPayment)) {
					if(cmp(rNewItem.CheckDtm, CheckDtm) > 0)
						ok = 1;
				}
				break;
			case sOnPayment:
				if(oneof2(rNewItem.Status, sBusy, sOnPayment)) {
					if(cmp(rNewItem.CheckDtm, CheckDtm) > 0)
						ok = 1;
				}
				break;
			case sOrder:
				if(rNewItem.Status == sBusy)
					ok = 1;
				else if(rNewItem.Status == sOrder) {
					if(cmp(rNewItem.OrderTime.Start, OrderTime.Start) < 0 && cmp(rNewItem.OrderTime.Start, cdtm) > 0)
						ok = 1;
				}
				break;
		}
	}
	if(ok > 0)
		*this = rNewItem;
	return ok;
}

class CCheckCache : public ObjCache {
public:
	CCheckCache();
	int    GetCTableStatus(long tableNo, CTableStatus * pStatus);
	int    GetCTableOrderList(TSVector <CTableStatus> & rList);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, long extraData)
	{
		return -1;
	}
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
	}
	int    RefreshCTableStatusList(int force);
	LDATETIME CTableStatusTime; // Время последнего извлечения чеков для определения статусов столов
	TSVector <CTableStatus> CTStList;
	long   CTableStatusRefreshTimeout;
	LDATE  FirstActualDate; // Дата первого чека заказа, имеющего дату большую или равную текущей.
		// Это поле необходимо для сужения области просмотра чеков так как они не индексированы
		// по признаку заказа или по дате исполнения.
	ReadWriteLock CTableStatusRwl;
};

CCheckCache::CCheckCache() : ObjCache(PPOBJ_CCHECK, sizeof(ObjCacheEntry)), CTableStatusRefreshTimeout(5), CTableStatusTime(ZERODATETIME), FirstActualDate(ZERODATE)
{
}

int CCheckCache::GetCTableOrderList(TSVector <CTableStatus> & rList)
{
	int    ok = -1;
	RefreshCTableStatusList(0);
	{
		SRWLOCKER(CTableStatusRwl, SReadWriteLocker::Read);
		for(uint i = 0; i < CTStList.getCount(); i++) {
			const CTableStatus & r_st = CTStList.at(i);
			if(r_st.Status == CTableStatus::sOrder) {
				rList.insert(&r_st);
				ok = 1;
			}
		}
	}
	return ok;
}

int CCheckCache::GetCTableStatus(long tableNo, CTableStatus * pStatus)
{
	int    status = CTableStatus::sUnkn;
		RefreshCTableStatusList(0);
	{
		SRWLOCKER(CTableStatusRwl, SReadWriteLocker::Read);
		CTableStatus temp_item;
		temp_item.TableNo = tableNo;
		for(uint pos = 0; CTStList.lsearch(&tableNo, &pos, CMPF_LONG); pos++) {
			const CTableStatus & r_st = CTStList.at(pos);
			temp_item.Displace(r_st);
		}
		if(temp_item.Status != CTableStatus::sUnkn) {
			ASSIGN_PTR(pStatus, temp_item);
			status = temp_item.Status;
		}
		else {
			CTableStatus st(0);
			status = CTableStatus::sFree;
			st.Status = CTableStatus::sFree;
			st.TableNo = tableNo;
			ASSIGN_PTR(pStatus, st);
		}
	}
	return status;
}

int CCheckCache::RefreshCTableStatusList(int force)
{
	int    ok = 1;
	{
		SRWLOCKER(CTableStatusRwl, SReadWriteLocker::Write);
		const LDATETIME cdtm = getcurdatetime_();
		if(force || CTableStatusTime.d == ZERODATE || diffdatetimesec(cdtm, CTableStatusTime) > CTableStatusRefreshTimeout) {
			CTableStatusTime = cdtm;
			CTStList.clear();
			PPViewCCheck cc_view;
			CCheckFilt cc_filt;
			CCheckViewItem cc_item;
			const LDATE start = NZOR(FirstActualDate, plusdate(cdtm.d, -7));
			cc_filt.Period.Set(start, ZERODATE);
			cc_filt.Flags |= CCheckFilt::fCTableStatus;
			cc_view.Init_(&cc_filt);
			for(cc_view.InitIteration(0); cc_view.NextIteration(&cc_item) > 0;) {
				if(!FirstActualDate && cc_item.Flags & CCHKF_ORDER && cc_item.OrderTime.Start.d >= plusdate(cdtm.d, -1)) {
					FirstActualDate = cc_item.Dt;
				}
				const long table_no = cc_item.TableCode;
				uint   pos = 0;
				CTableStatus st(&cc_item);
				/*
				if(CTStList.lsearch(&table_no, &pos, CMPF_LONG)) {
					CTStList.at(pos).Displace(st);
				}
				else*/
					CTStList.insert(&st);
			}
		}
		else
			ok = -1;
	}
	return ok;
}

/*static*/int FASTCALL CCheckCore::FetchCTableStatus(long tableNo, CTableStatus * pStatus)
{
	CCheckCache * p_cache = GetDbLocalCachePtr <CCheckCache> (PPOBJ_CCHECK);
	return p_cache ? p_cache->GetCTableStatus(tableNo, pStatus) : 0;
}

/*static*/int FASTCALL CCheckCore::FetchCTableOrderList(TSVector <CTableStatus> & rList)
{
	CCheckCache * p_cache = GetDbLocalCachePtr <CCheckCache> (PPOBJ_CCHECK);
	return p_cache ? p_cache->GetCTableOrderList(rList) : 0;
}

/* @v10.3.4 @useless
//static
int CCheckCore::RecognizeOrdBarcode(const char * pCode, PPID * pOrdCheckID)
{
	PPID   cc_id = 0;
	if(!isempty(pCode)) {
		SString temp_buf = pCode;
		if(temp_buf.CmpPrefix("CTO", 1) == 0)
			cc_id = atol(temp_buf.cptr()+3);
	}
	ASSIGN_PTR(pOrdCheckID, cc_id);
	return BIN(cc_id);
}*/

int CCheckCore::SerializePacket(int dir, CCheckPacket * pPack, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	//
	// Значение tag содержит флаги:
	// -- 0x01 если чек имеет запись расширения (CCheckExt) //
	// -- 0x02 если одна или более строк имеют запись расширения (CCheckLineExt).
	// -- 0x04 если чек имеет массив сумм оплат CcAmountList
	//
	uint32 tag = 0;
	if(dir > 0) {
		if(pPack->Rec.Flags & CCHKF_EXT && P_Ext)
			tag |= 0x01;
		if(pPack->Rec.Flags & CCHKF_LINEEXT && P_LnExt)
			tag |= 0x02;
		if(pPack->Rec.Flags & CCHKF_PAYMLIST)
			tag |= 0x04;
	}
	THROW_SL(pCtx->Serialize(dir, tag, rBuf));
	THROW_SL(SerializeRecord(dir, &pPack->Rec, rBuf, pCtx));
	if(tag & 0x01) {
		if(dir < 0 && P_Ext == 0) {
			THROW_SL(pCtx->Unserialize(0, 0, 0, rBuf));
		}
		else {
			assert(P_Ext != 0); // Условие гарантированно выполняется по причине вышестоящей проверки и установки признака tag.
			THROW_SL(P_Ext->SerializeRecord(dir, &pPack->Ext, rBuf, pCtx));
		}
	}
	if(tag & 0x04) {
		uint32 c = 0;
		if(dir > 0) {
			c = pPack->CcAl.getCount();
			THROW_SL(pCtx->Serialize(dir, c, rBuf));
			for(uint i = 0; i < c; i++) {
				CcAmountEntry & r_entry = pPack->CcAl.at(i);
				THROW_SL(pCtx->Serialize(dir, r_entry.Type, rBuf));
				THROW_SL(pCtx->Serialize(dir, r_entry.AddedID, rBuf));
				THROW_SL(pCtx->Serialize(dir, r_entry.Amount, rBuf));
				THROW_SL(pCtx->Serialize(dir, r_entry.CurID, rBuf));
				THROW_SL(pCtx->Serialize(dir, r_entry.CurAmount, rBuf));
			}
		}
		else if(dir < 0) {
			pPack->CcAl.freeAll();
			THROW_SL(pCtx->Serialize(dir, c, rBuf));
			for(uint i = 0; i < c; i++) {
				CcAmountEntry entry;
				THROW_SL(pCtx->Serialize(dir, entry.Type, rBuf));
				THROW_SL(pCtx->Serialize(dir, entry.AddedID, rBuf));
				THROW_SL(pCtx->Serialize(dir, entry.Amount, rBuf));
				THROW_SL(pCtx->Serialize(dir, entry.CurID, rBuf));
				THROW_SL(pCtx->Serialize(dir, entry.CurAmount, rBuf));
				THROW_SL(pPack->CcAl.insert(&entry));
			}
		}
	}
	THROW_SL(Lines.SerializeArrayOfRecords(dir, &pPack->Items_, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, pPack->LnTextList, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
int CCheckCore::CorrectCCLineProblem01()
{
	int    ok = 1;
	if(DS.CheckExtFlag(ECF_AVERAGE)) {
		PPWaitStart();
		PPTransaction tra(1);
		THROW(tra);
		{
			//
			// Поиск и удаление строк чеков, не привязанных к чекам
			//
			IterCounter cntr;
			PPInitIterCounter(cntr, &Lines);
			CCheckLineTbl::Key0 k0;
			MEMSZERO(k0);
			if(Lines.search(0, &k0, spFirst)) do {
				int    r;
				THROW(r = SearchByID(this, PPOBJ_CCHECK, Lines.data.CheckID, 0));
				if(r < 0) {
					THROW_DB(Lines.deleteRec());
				}
				PPWaitPercent(cntr.Increment());
			} while(Lines.search(0, &k0, spNext));
		}
		{
			//
			// Удаление чеков строк, которые автоматически появились там из-за изначально непустой таблицы CCheckLine
			//
			//ValidateCheck(item.ID, 0.001, logger);
			const double tolerance = 0.001;
			IterCounter cntr;
			PPInitIterCounter(cntr, this);
			CCheckPacket cc_pack;
			CCheckTbl::Key0 k0;
			if(search(0, &k0, spFirst)) do {
				DBRowId c_pos;
				const PPID cc_id = data.ID;
				// @v10.8.7 (LoadPacket) cc_pack.Z();
				THROW_DB(getPosition(&c_pos));
				if(LoadPacket(cc_id, 0, &cc_pack) > 0) {
					double amt = 0.0, dscnt = 0.0;
					cc_pack.CalcAmount(&amt, &dscnt);
					double rec_amt   = MONEYTOLDBL(cc_pack.Rec.Amount);
					double rec_dscnt = MONEYTOLDBL(cc_pack.Rec.Discount);
					if(fabs(amt - rec_amt) > tolerance) {
						LongArray rbc_list;
						uint c = cc_pack.Items_.getCount();
						while(c--) {
							rbc_list.add(cc_pack.Items_.at(0).RByCheck);
							cc_pack.Items_.atFree(0);
							cc_pack.CalcAmount(&amt, &dscnt);
							if(fabs(amt - rec_amt) <= tolerance) {
								for(uint i = 0; i < rbc_list.getCount(); i++) {
									CCheckLineTbl::Key0 lk0;
									lk0.CheckID = cc_id;
									lk0.RByCheck = static_cast<int16>(rbc_list.get(i));
									if(Lines.searchForUpdate(&lk0, spEq)) {
										THROW_DB(Lines.deleteRec()); // @sfu
									}
									if(P_LnExt) {
										CCheckLineExtTbl::Key0 lek0;
										lek0.CheckID = cc_id;
										lek0.RByCheck = static_cast<int16>(rbc_list.get(i));
										if(P_LnExt->searchForUpdate(&lek0, spEq)) {
											THROW_DB(P_LnExt->deleteRec()); // @sfu
										}
									}
								}
								break;
							}
						}
					}
				}
				THROW_DB(getDirect(0, &k0, c_pos));
				PPWaitPercent(cntr.Increment());
			} while(search(0, &k0, spNext));
		}
		THROW(tra.Commit());
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
