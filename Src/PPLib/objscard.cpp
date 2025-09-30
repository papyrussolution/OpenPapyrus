// OBJSCARD.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Модуль, управляющий объектом PPObjSCard - персональные карты
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

PPSCardPacket::PPSCardPacket() : PPExtStrContainer()
{
}

PPSCardPacket & PPSCardPacket::Z()
{
	Rec.Clear();
    SetBuffer(0);
	return *this;
}

/*static*/int FASTCALL PPObjSCard::PreprocessSCardCode(SString & rCode)
{
	int    ok = -1;
	if(rCode.Len()) {
		SString templ_buf;
		SString buf;
		SString input(rCode);
		StringSet patterns(';', DS.GetConstTLA().SCardPatterns);
		for(uint p = 0; ok < 0 && patterns.get(&p, buf);) {
			templ_buf.Cat(buf);
			if(buf.Len() < 2 || templ_buf.Last() != '"') {
				templ_buf.Semicol();
			}
			else if(templ_buf.C(0) == '"') {
				templ_buf.ShiftLeft().TrimRight();
				size_t  code_pos = 0;
				if(templ_buf.Search("*C*", code_pos, 1, &code_pos)) {
					int    is_suitable = 1;
					size_t last_code_pos = 0;
					SString code_buf(input);
					SString prefix;
					SString postfix;
					(prefix = templ_buf).Trim(code_pos);
					(postfix = templ_buf).ShiftLeft(code_pos + 3);
					if(prefix.NotEmpty()) {
						is_suitable = code_buf.Search(prefix, (code_pos = 0), 0, &code_pos);
						code_buf.ShiftLeft(code_pos + prefix.Len());
					}
					if(is_suitable && postfix.NotEmpty()) {
						is_suitable = code_buf.Search(postfix, last_code_pos, 0, &last_code_pos);
						code_buf.Trim(last_code_pos);
					}
					if(is_suitable) {
						rCode = code_buf;
						ok = 1;
					}
					else
						templ_buf.Z();
				}
			}
		}
		if(ok < 0) {
			rCode = (input[0] == '#') ? (input+1) : input;
			ok = 1;
		}
	}
	return ok;
}
//
//
/*static*/int FASTCALL PPObjSCard::ReadConfig(PPSCardConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_SCARDCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

/*static*/int FASTCALL PPObjSCard::WriteConfig(const PPSCardConfig * pCfg, int use_ta)
{
	int    ok = 1;
	int    r;
	Reference * p_ref = PPRef;
	PPSCardConfig ex_cfg;
	THROW(CheckCfgRights(PPCFGOBJ_SCARD, PPR_MOD, 0));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = p_ref->GetPropMainConfig(PPPRP_SCARDCFG, &ex_cfg, sizeof(ex_cfg)));
		{
			const bool is_new = (r <= 0);
			THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_SCARDCFG, pCfg, 0));
			DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_SCARD, 0, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObjSCard::FetchConfig(PPSCardConfig * pCfg) { return PPObjSCardSeries::FetchConfig(pCfg); }

int PPObjSCard::EditConfig()
{
	class SCardCfgDialog : public TDialog {
		DECL_DIALOG_DATA(PPSCardConfig);
		enum {
			ctlgroupGoods = 1
		};
	public:
		SCardCfgDialog() : TDialog(DLG_SCARDCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_SCARDCFG_PSNKND, PPOBJ_PERSONKIND, Data.PersonKindID, 0, 0);
			PPID   psn_kind_id = NZOR(Data.PersonKindID, PPPRK_CLIENT);
			SetupPPObjCombo(this, CTLSEL_SCARDCFG_DEFPSN, PPOBJ_PERSON, Data.DefPersonID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(psn_kind_id));
			GoodsCtrlGroup::Rec rec(0, Data.ChargeGoodsID, 0, 0);
			addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_SCARDCFG_CHARGEGG, CTLSEL_SCARDCFG_CHARGEG));
			setGroupData(ctlgroupGoods, &rec);
			SetupPPObjCombo(this, CTLSEL_SCARDCFG_CHRGAMT, PPOBJ_AMOUNTTYPE, Data.ChargeAmtID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_SCARDCFG_DEFSER, PPOBJ_SCARDSERIES, Data.DefSerID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_SCARDCFG_DEFCSER, PPOBJ_SCARDSERIES, Data.DefCreditSerID, OLW_CANINSERT);
			setCtrlLong(CTL_SCARDCFG_WARNEXPB, Data.WarnExpiryBefore);
			AddClusterAssoc(CTL_SCARDCFG_FLAGS, 0, PPSCardConfig::fAcceptOwnerInDispDiv);
			AddClusterAssoc(CTL_SCARDCFG_FLAGS, 1, PPSCardConfig::fDontUseBonusCards);
			AddClusterAssoc(CTL_SCARDCFG_FLAGS, 2, PPSCardConfig::fCheckBillDebt);
			AddClusterAssoc(CTL_SCARDCFG_FLAGS, 3, PPSCardConfig::fDisableBonusByDefault);
			SetClusterData(CTL_SCARDCFG_FLAGS, Data.Flags);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			PPSCardSeries scs_rec;
			GoodsCtrlGroup::Rec rec;
			getCtrlData(CTLSEL_SCARDCFG_PSNKND, &Data.PersonKindID);
			getCtrlData(CTLSEL_SCARDCFG_DEFPSN, &Data.DefPersonID);
			getCtrlData(CTLSEL_SCARDCFG_CHRGAMT, &Data.ChargeAmtID);
			getCtrlData(sel = CTLSEL_SCARDCFG_DEFSER, &Data.DefSerID);
			if(Data.DefSerID) {
				THROW(ScsObj.Fetch(Data.DefSerID, &scs_rec) > 0);
				THROW_PP(!(scs_rec.Flags & SCRDSF_CREDIT), PPERR_NONCREDITCARDSERNEEDED);
			}
			getCtrlData(sel = CTLSEL_SCARDCFG_DEFCSER, &Data.DefCreditSerID);
			if(Data.DefCreditSerID) {
				THROW(ScsObj.Fetch(Data.DefCreditSerID, &scs_rec) > 0);
				THROW_PP(scs_rec.Flags & SCRDSF_CREDIT, PPERR_CREDITCARDSERNEEDED);
			}
			getCtrlData(sel = CTL_SCARDCFG_WARNEXPB, &Data.WarnExpiryBefore);
			THROW_PP(Data.WarnExpiryBefore >= 0 && Data.WarnExpiryBefore <= 365, PPERR_USERINPUT);
			GetClusterData(CTL_SCARDCFG_FLAGS, &Data.Flags);
			getGroupData(ctlgroupGoods, &rec);
			sel = CTL_SCARDCFG_CHARGEG;
			THROW_PP(!rec.GoodsID || GObj.CheckFlag(rec.GoodsID, GF_UNLIM), PPERR_INVSCARDCHARGEGOODS);
			Data.ChargeGoodsID = rec.GoodsID;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SCARDCFG_PSNKND)) {
				PPID   psn_kind_id = getCtrlLong(CTLSEL_SCARDCFG_PSNKND);
				if(psn_kind_id != Data.PersonKindID) {
					SetupPPObjCombo(this, CTLSEL_SCARDCFG_DEFPSN, PPOBJ_PERSON, 0, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(NZOR(psn_kind_id, PPPRK_CLIENT)));
					Data.PersonKindID = psn_kind_id;
				}
				clearEvent(event);
			}
		}
		PPObjGoods GObj;
		PPObjSCardSeries ScsObj;
	};
	int    ok = -1;
	int    valid_data = 0;
	int    is_new = 0;
	PPSCardConfig cfg;
	SCardCfgDialog * dlg = 0;
	THROW(CheckCfgRights(PPCFGOBJ_SCARD, PPR_READ, 0));
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&(dlg = new SCardCfgDialog)));
	dlg->setDTS(&cfg);
	while(!valid_data && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_SCARD, PPR_MOD, 0));
		if(dlg->getDTS(&cfg))
			ok = valid_data = 1;
	}
	if(ok > 0) {
		THROW(WriteConfig(&cfg, 1));
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
PPSCardSeries2::PPSCardSeries2()
{
	THISZERO();
	VerifTag = 2;
}

bool FASTCALL PPSCardSeries2::IsEq(const PPSCardSeries2 & rS) const
{
#define I(f) if(f != rS.f) return false;
	I(BonusChrgGrpID);
	I(VerifTag);
	I(BonusGrpID);
	I(CrdGoodsGrpID);
	I(ChargeGoodsID);
	I(Issue);
	I(Expiry);
	I(PDis);
	I(MaxCredit);
	I(Flags);
	I(QuotKindID_s);
	I(PersonKindID);
	I(ParentID);
	I(SpecialTreatment);
	I(RsrvPoolDestSerID);
	I(FixedBonus);
	if(!sstreq(Name, rS.Name))
		return false;
	else if(!sstreq(Symb, rS.Symb))
		return false;
	return true;
#undef I
}

int PPSCardSeries2::GetType() const
{
	if(Flags & SCRDSF_RSRVPOOL)
		return scstRsrvPool;
	else if(Flags & SCRDSF_GROUP)
		return scstGroup;
	else if(Flags & SCRDSF_BONUS)
		return scstBonus;
	else if(Flags & SCRDSF_CREDIT)
		return scstCredit;
	else
		return scstDiscount;
}

int PPSCardSeries2::SetType(int type)
{
	int    ok = 1;
	const  long preserve_flags = Flags;
	switch(type) {
		case scstDiscount:
			Flags &= ~(SCRDSF_CREDIT|SCRDSF_BONUS|SCRDSF_GROUP|SCRDSF_RSRVPOOL);
			break;
		case scstCredit:
			Flags |= SCRDSF_CREDIT;
			Flags &= ~(SCRDSF_BONUS|SCRDSF_GROUP|SCRDSF_RSRVPOOL);
			break;
		case scstBonus:
			Flags |= SCRDSF_BONUS;
			Flags &= ~(SCRDSF_CREDIT|SCRDSF_GROUP|SCRDSF_RSRVPOOL);
			break;
		case scstGroup:
			Flags |= SCRDSF_GROUP;
			Flags &= ~(SCRDSF_CREDIT|SCRDSF_BONUS|SCRDSF_RSRVPOOL);
			break;
		case scstRsrvPool:
			Flags |= SCRDSF_RSRVPOOL;
			Flags &= ~(SCRDSF_CREDIT|SCRDSF_BONUS|SCRDSF_GROUP|SCRDSF_GROUP);
			break;
		default:
			ok = 0;
			break;
	}
	return (ok && Flags == preserve_flags) ? -1 : ok;
}

int PPSCardSeries2::Verify()
{
	int    ok = -1;
	if(VerifTag == 0) {
		//
		// Верификация v7.3.7 заключается в удалении из поля Flags всех бит, отличных от SCRDSF_CREDIT и SCRDSF_USEDSCNTIFNQUOT.
		//
		const long preserve_flags = Flags;
		Flags &= (SCRDSF_CREDIT|SCRDSF_USEDSCNTIFNQUOT); // @notanerror Удаляем все флаги кроме указанных
		VerifTag = 2;
		if(Flags != preserve_flags)
			ok = 1;
	}
	return ok;
}
//
//
//
TrnovrRngDis::TrnovrRngDis()
{
	THISZERO();
}

int TrnovrRngDis::GetResult(double currentValue, double * pResult) const
{
	int    ok = 0;
	double result = currentValue;
    if(Flags & fDiscountAddValue)
		result = currentValue + Value;
	else if(Flags & fDiscountMultValue)
		result = currentValue * Value;
	else
		result = Value;
	ok = (result != currentValue) ? 1 : -1;
	ASSIGN_PTR(pResult, result);
	return ok;
}
//
PPSCardSerRule::PPSCardSerRule() : TSVector <TrnovrRngDis>()
{
	Z();
}

PPSCardSerRule & FASTCALL PPSCardSerRule::operator = (const PPSCardSerRule & s)
{
	copy(s);
	Ver = s.Ver;
	memcpy(&Fb, &s.Fb, sizeof(Fb));
	TrnovrPeriod = s.TrnovrPeriod;
	return *this;
}

int PPSCardSerRule::CheckTrnovrRng(const TrnovrRngDis & rItem, long pos) const
{
	int    ok = ((rItem.Flags & TrnovrRngDis::fZeroTurnover) || (rItem.R.upp > rItem.R.low && rItem.R.low >= 0.0)) ? 1 : PPSetError(PPERR_TRNOVRRNG);
	TrnovrRngDis * p_item = 0;
	for(uint i = 0; ok == 1 && enumItems(&i, (void **)&p_item) > 0;) {
		if(pos < 0 || pos != i - 1) {
			if(rItem.Flags & TrnovrRngDis::fZeroTurnover && (p_item->Flags & TrnovrRngDis::fZeroTurnover)) {
				ok = PPSetError(PPERR_SCSERRULE_DUPZEROTRNOVR);
			}
			else if(p_item->R.Check(rItem.R.upp) || p_item->R.Check(rItem.R.low) || (rItem.R.low <= p_item->R.low && rItem.R.upp >= p_item->R.upp))
				ok = PPSetError(PPERR_INTRSTRNOVRRNG);
		}
	}
	return ok;
}

int PPSCardSerRule::ValidateItem(int ruleType, const TrnovrRngDis & rItem, long pos) const
{
	int    ok = 1;
	THROW(CheckTrnovrRng(rItem, pos));
	THROW_PP(ruleType == rultBonus || !(rItem.Flags & rItem.fBonusAbsoluteValue), PPERR_INVSCSRULEITEMFLAG);
	if(rItem.Flags & rItem.fBonusAbsoluteValue) {
		THROW_PP_S(rItem.Value >= -100000 && rItem.Value <= 100000, PPERR_INVSCSRULEITEMABSVAL, "-100000..100000");
	}
	else if(rItem.Flags & rItem.fDiscountAddValue) {
		THROW_PP_S(rItem.Value >= -100 && rItem.Value <= 100, PPERR_INVSCSRULEITEMDAV, "-100..100");
	}
	else if(rItem.Flags & rItem.fDiscountMultValue) {
		THROW_PP_S(rItem.Value >= 0 && rItem.Value <= 10, PPERR_INVSCSRULEITEMDMV, "0..10");
	}
	else {
		THROW_PP_S(rItem.Value >= -300 && rItem.Value <= 100, PPERR_INVSCSRULEITEMPCTVAL, "-300..100");
	}
	CATCHZOK
	return ok;
}

bool FASTCALL PPSCardSerRule::IsEq(const PPSCardSerRule & rS) const
{
	bool   eq = true;
	if(Ver != rS.Ver)
		eq = false;
	else if(TrnovrPeriod != rS.TrnovrPeriod)
		eq = false;
	else if(Fb.Flags != rS.Fb.Flags)
		eq = false;
	// @v11.4.9 {
	else if(Fb.CancelPrd != rS.Fb.CancelPrd)
		eq = false;
	else if(Fb.CancelPrcCount != rS.Fb.CancelPrcCount)
		eq = false;
	// } @v11.4.9 
	else if(getCount() != rS.getCount())
		eq = false;
	else if(getCount()) {
		for(uint i = 0; eq && i < getCount(); i++) {
			const TrnovrRngDis & r_i1 = at(i);
			const TrnovrRngDis & r_i2 = rS.at(i);
			if(r_i1.R != r_i2.R)
				eq = false;
			else if(r_i1.Value != r_i2.Value)
				eq = false;
			else if(r_i1.SeriesID != r_i2.SeriesID)
				eq = false;
			else if(r_i1.Flags != r_i2.Flags)
				eq = false;
		}
	}
	return eq;
}

PPSCardSerRule & PPSCardSerRule::Z()
{
	clear();
	Ver = 1;
	MEMSZERO(Fb);
	TrnovrPeriod = 0;
	return *this;
}

int PPSCardSerRule::IsList() const
{
	return BIN(count > 1);
}

#if 0 // {
int PPSCardSerRule::GetPDisValue(double amt, double * pValue) const
{
	int    ok = -1;
	double discount = 0.0;
	TrnovrRngDis * p_item = 0;
	for(uint i = 0; ok < 0 && i < getCount(); i++) {
		const TrnovrRngDis & r_item = at(i);
		if(r_item.R.Check(amt)) {
			discount = r_item.Value;
			ok = 1;
		}
	}
	if(ok > 0) {
		ASSIGN_PTR(pValue, discount);
	}
	return ok;
}
#endif // } 0

const TrnovrRngDis * PPSCardSerRule::SearchItem(double amount) const
{
	const TrnovrRngDis * p_item = 0;
	for(uint i = 0; !p_item && i < getCount(); i++) {
		const TrnovrRngDis & r_item = at(i);
		if(r_item.R.Check(amount))
			p_item = &r_item;
	}
	return p_item;
}

int PPSCardSerRule::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Ver, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, TrnovrPeriod, rBuf));
	THROW_SL(pSCtx->Serialize(dir, this /* as SArray */, rBuf));
	CATCHZOK
	return ok;
}

PPSCardSerPacket::Ext::Ext()
{
	Init();
}

void PPSCardSerPacket::Ext::Init()
{
	THISZERO();
}

bool PPSCardSerPacket::Ext::IsEmpty() const
{
	return (!UsageTmStart && !UsageTmEnd && (!UsageTmStart || !checktime(UsageTmStart)) && (!UsageTmEnd || !checktime(UsageTmEnd)) && !CodeTempl[0]);
}

bool FASTCALL PPSCardSerPacket::Ext::IsEq(const Ext & rS) const
{
	bool   yes = true;
	if(UsageTmStart != rS.UsageTmStart || UsageTmEnd != rS.UsageTmEnd || strcmp(CodeTempl, rS.CodeTempl) != 0)
		yes = false;
	return yes;
}

PPSCardSerPacket::PPSCardSerPacket()
{
	Z();
}

PPSCardSerPacket & PPSCardSerPacket::Z()
{
	UpdFlags = 0;
	MEMSZERO(Rec);
	Rule.Z();
	CcAmtDisRule.Z();
	QuotKindList_.clear();
	Eb.Init();
	return *this;
}

int PPSCardSerPacket::Normalize()
{
	int    ok = 1;
	PPIDArray temp_list;
	if(QuotKindList_.getCount()) {
		//
		// Пусть это и крайне маловероятно, но предупредим существование дубликатов в списке видов котировок.
		// Кроме того, проверим виды котировок в списке на предмет того, чтобы они не принадлежали специальным видам.
		//
		PPObjQuotKind qk_obj;
		PPQuotKindPacket qk_pack;
		const PPObjQuotKind::Special qk_spc(PPObjQuotKind::Special::ctrInitializeWithCache);
		for(uint i = 0; i < QuotKindList_.getCount(); i++) {
			const  PPID qk_id = QuotKindList_.get(i);
			if(qk_id && qk_obj.Fetch(qk_id, &qk_pack) > 0 && qk_spc.GetCategory(qk_id) == PPQC_PRICE)
				temp_list.addUnique(qk_id);
		}
	}
	if(temp_list.getCount() == 0) {
		Rec.Flags &= ~SCRDSF_USEQUOTKINDLIST;
	}
	else if(temp_list.getCount() == 1) {
		Rec.QuotKindID_s = temp_list.get(0);
		Rec.Flags &= ~SCRDSF_USEQUOTKINDLIST;
		temp_list.clear();
	}
	else if(temp_list.getCount() > 1) {
		Rec.QuotKindID_s = 0;
		Rec.Flags |= SCRDSF_USEQUOTKINDLIST;
	}
	QuotKindList_ = temp_list;
	return ok;
}

bool FASTCALL PPSCardSerPacket::IsEq(const PPSCardSerPacket & rS) const
{
	bool   eq = true;
	if(!Rec.IsEq(rS.Rec))
		eq = false;
	else if(!Rule.IsEq(rS.Rule))
		eq = false;
	else if(!CcAmtDisRule.IsEq(rS.CcAmtDisRule))
		eq = false;
	else if(!BonusRule.IsEq(rS.BonusRule))
		eq = false;
	else if(!QuotKindList_.IsEq(&rS.QuotKindList_))
		eq = false;
	else if(!Eb.IsEq(rS.Eb))
		eq = false;
	return eq;
}

int PPSCardSerPacket::GetDisByRule(double trnovr, TrnovrRngDis & rEntry) const
{
	int    ok = -1;
	TrnovrRngDis * p_item = 0;
	for(uint i = 0; ok < 0 && Rule.enumItems(&i, (void **)&p_item) > 0;) {
		if(trnovr == 0.0 && p_item->Flags & TrnovrRngDis::fZeroTurnover) { // @v11.3.10
			rEntry = *p_item;
			ok = 1;
		}
		else if(p_item->R.Check(trnovr)) {
			rEntry = *p_item;
			ok = 1;
		}
	}
	return ok;
}

class SCardRuleDlg : public PPListDialog {
	DECL_DIALOG_DATA(PPSCardSerRule);
public:
	SCardRuleDlg(int ruleType) : PPListDialog(DLG_SCARDRULE, CTL_SCARDRULE_TRNOVRRNG), RuleType(ruleType)
	{
		SmartListBox * p_lb = static_cast<SmartListBox *>(getCtrlView(CTL_SCARDRULE_TRNOVRRNG));
		const char * p_title_symb = 0;
		switch(RuleType) {
			case PPSCardSerRule::rultDisc:
				p_title_symb = "scardrule_dis";
				CALLPTRMEMB(p_lb, SetupColumns("@lbt_scardrule_dis"));
				break;
			case PPSCardSerRule::rultCcAmountDisc:
				p_title_symb = "scardrule_ccdis";
				CALLPTRMEMB(p_lb, SetupColumns("@lbt_scardrule_ccdis"));
				break;
			case PPSCardSerRule::rultBonus:
				p_title_symb = "scardrule_bonus";
				CALLPTRMEMB(p_lb, SetupColumns("@lbt_scardrule_bonus"));
				break;
		}
		disableCtrls(RuleType != PPSCardSerRule::rultBonus, CTLSEL_SCARDRULE_CNCLPRD, CTL_SCARDRULE_CNCLPRDC, 0);
		if(p_title_symb) {
			SString title_buf;
			setTitle(PPLoadStringS(p_title_symb, title_buf));
		}
		showCtrl(CTLSEL_SCARDRULE_PRD, (RuleType != PPSCardSerRule::rultCcAmountDisc));
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		SetupStringCombo(this, CTLSEL_SCARDRULE_PRD, PPTXT_CYCLELIST, Data.TrnovrPeriod);
		if(RuleType == PPSCardSerRule::rultBonus) {
			SetupStringCombo(this, CTLSEL_SCARDRULE_CNCLPRD, PPTXT_CYCLELIST, Data.Fb.CancelPrd);
			setCtrlData(CTL_SCARDRULE_CNCLPRDC, &Data.Fb.CancelPrcCount);
			disableCtrl(CTL_SCARDRULE_CNCLPRDC, !oneof6(Data.Fb.CancelPrd, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL));
		}
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		getCtrlData(CTLSEL_SCARDRULE_PRD, &Data.TrnovrPeriod);
		if(RuleType == PPSCardSerRule::rultBonus) {
			getCtrlData(CTLSEL_SCARDRULE_CNCLPRD, &Data.Fb.CancelPrd);
			getCtrlData(CTL_SCARDRULE_CNCLPRDC, &Data.Fb.CancelPrcCount);
		}
		else {
			Data.Fb.CancelPrd = 0;
			Data.Fb.CancelPrcCount = 0;
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SCARDRULE_CNCLPRD)) {
			if(RuleType == PPSCardSerRule::rultBonus) {
				getCtrlData(CTLSEL_SCARDRULE_CNCLPRD, &Data.Fb.CancelPrd);
				disableCtrl(CTL_SCARDRULE_CNCLPRDC, !oneof6(Data.Fb.CancelPrd, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL));
				clearEvent(event);
			}
		}
	}
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  delItem(long pos, long id);
	virtual int  editItem(long pos, long id);
	int    EditTrnovrRng(long pos);

	const  int  RuleType;
};

int SCardRuleDlg::EditTrnovrRng(long pos)
{
	class SCardRuleDialog : public TDialog {
		DECL_DIALOG_DATA(TrnovrRngDis);
		const int RuleType;
	public:
		SCardRuleDialog(uint dlgId, const int ruleType) : TDialog(dlgId), RuleType(ruleType)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			//
			SetRealRangeInput(this, CTL_TRNVRRNG_RANGE, &Data.R);
			setCtrlUInt16(CTL_TRNVRRNG_ZEROTRNOVR, BIN(Data.Flags & Data.fZeroTurnover));
			setCtrlData(CTL_TRNVRRNG_DIS, &Data.Value);
			SetupPPObjCombo(this, CTLSEL_TRNVRRNG_SERIES, PPOBJ_SCARDSERIES, ((RuleType == PPSCardSerRule::rultDisc) ? Data.SeriesID : 0), 0, 0);
			showCtrl(CTLSEL_TRNVRRNG_SERIES, (RuleType == PPSCardSerRule::rultDisc));
			if(RuleType == PPSCardSerRule::rultBonus) {
				const long   method = (Data.Flags & Data.fBonusAbsoluteValue) ? 2 : 1;
				AddClusterAssocDef(CTL_TRNVRRNG_METHOD, 0, 1);
				AddClusterAssoc(CTL_TRNVRRNG_METHOD, 1, 2);
				SetClusterData(CTL_TRNVRRNG_METHOD, method);
			}
			else {
				const long  method = (Data.Flags & Data.fDiscountAddValue) ? 2 : ((Data.Flags & Data.fDiscountMultValue) ? 3 : 1);
				AddClusterAssocDef(CTL_TRNVRRNG_METHOD, 0, 1);
				AddClusterAssoc(CTL_TRNVRRNG_METHOD, 1, 2);
				AddClusterAssoc(CTL_TRNVRRNG_METHOD, 2, 3);
				SetClusterData(CTL_TRNVRRNG_METHOD, method);
			}
			SetupCtrl();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			//
			getCtrlData(CTL_TRNVRRNG_DIS, &Data.Value);
			getCtrlData(CTLSEL_TRNVRRNG_SERIES, &Data.SeriesID);
			if(RuleType == PPSCardSerRule::rultBonus) {
				const long method = GetClusterData(CTL_TRNVRRNG_METHOD);
				Data.Flags &= ~(Data.fBonusAbsoluteValue|Data.fDiscountAddValue|Data.fDiscountMultValue);
				SETFLAG(Data.Flags, Data.fBonusAbsoluteValue, method == 2);
			}
			else {
				const long method = GetClusterData(CTL_TRNVRRNG_METHOD);
				Data.Flags &= ~(Data.fBonusAbsoluteValue|Data.fDiscountAddValue|Data.fDiscountMultValue);
				if(method == 2)
					Data.Flags |= Data.fDiscountAddValue;
				else if(method == 3)
					Data.Flags |= Data.fDiscountMultValue;
			}
			GetRealRangeInput(this, CTL_TRNVRRNG_RANGE, &Data.R);
			SETFLAG(Data.Flags, Data.fZeroTurnover, getCtrlUInt16(CTL_TRNVRRNG_ZEROTRNOVR));
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_TRNVRRNG_ZEROTRNOVR)) {
				SetupCtrl();
			}
			else
				return;
			clearEvent(event);
		}
		void SetupCtrl()
		{
			uint16 zt = getCtrlUInt16(CTL_TRNVRRNG_ZEROTRNOVR);
			disableCtrl(CTL_TRNVRRNG_RANGE, zt);
		}
	};
	int    ok = -1;
	int    valid_data = 0;
	TrnovrRngDis range;
	SCardRuleDialog * p_dlg = new SCardRuleDialog((RuleType == PPSCardSerRule::rultBonus) ? DLG_SCBONUSRULE : DLG_TRNVRRNG, RuleType);
	THROW(CheckDialogPtr(&p_dlg));
	if(pos >= 0 && pos < Data.getCountI())
		range = Data.at(static_cast<uint>(pos));
	p_dlg->setDTS(&range);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&range)) {
			if(Data.ValidateItem(RuleType, range, pos)) {
				if(pos >= 0)
					Data.at(static_cast<uint>(pos)) = range;
				else
					THROW_SL(Data.insert(&range));
				ok = valid_data = 1;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SCardRuleDlg::addItem(long * pPos, long * pID)
{
	int    ok = EditTrnovrRng(-1);
	if(ok > 0)
		ASSIGN_PTR(pPos, 0);
	return ok;
}

int SCardRuleDlg::editItem(long pos, long id)
{
	return EditTrnovrRng(pos);
}

int SCardRuleDlg::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos <= Data.getCountI() && Data.getCountI() > 0 && CONFIRM(PPCFM_DELETE))
		ok = Data.atFree(pos);
	return ok;
}

int SCardRuleDlg::setupList()
{
	int    ok = -1;
	SString buf;
	TrnovrRngDis * p_item = 0;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; Data.enumItems(&i, (void **)&p_item) > 0;) {
		ss.Z();
		if(p_item->Flags & TrnovrRngDis::fZeroTurnover)
			PPLoadString("trnovrrngdis_zerotrnovr", buf);
		else
			buf.Z().Cat(p_item->R.low, MKSFMTD(0, 2, NMBF_NOTRAILZ)).CatCharN('.', 2).
				Cat(p_item->R.upp, MKSFMTD(0, 2, NMBF_NOTRAILZ)).Space().Cat("RUB");
		ss.add(buf);
		if(p_item->Flags & TrnovrRngDis::fBonusAbsoluteValue)
			buf.Z().CatChar('$').Cat(p_item->Value, MKSFMTD(0, 2, NMBF_NOTRAILZ));
		else if(p_item->Flags & TrnovrRngDis::fDiscountAddValue)
			buf.Z().CatChar('+').CatChar('$').Cat(p_item->Value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('%');
		else if(p_item->Flags & TrnovrRngDis::fDiscountMultValue)
			buf.Z().CatChar('*').CatChar('$').Cat(p_item->Value, MKSFMTD(0, 5, NMBF_NOTRAILZ)).CatChar('%');
		else
			buf.Z().Cat(p_item->Value, MKSFMTD(0, 2, NMBF_NOTRAILZ)).CatChar('%');
		ss.add(buf);
		buf.Z();
		if(p_item->SeriesID)
			GetObjectName(PPOBJ_SCARDSERIES, p_item->SeriesID, buf);
		ss.add(buf);
		if(!addStringToList(i, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

SCardSeriesFilt::SCardSeriesFilt() : ParentID(0), Flags(0), SpecialTreatment(0)
{
}

PPObjSCardSeries::PPObjSCardSeries(void * extraPtr) : PPObjReference(PPOBJ_SCARDSERIES, extraPtr), P_ScObj(0)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

PPObjSCardSeries::~PPObjSCardSeries()
{
	delete P_ScObj;
}

SCardSeriesFilt & PPObjSCardSeries::InitFilt(void * extraPtr, SCardSeriesFilt & rFilt) const
{
	if(extraPtr) {
		rFilt = *static_cast<const SCardSeriesFilt *>(extraPtr);
	}
	else {
		rFilt.ParentID = 0;
		rFilt.Flags = 0;
	}
	return rFilt;
}

int PPObjSCardSeries::CheckForFilt(const SCardSeriesFilt * pFilt, const PPSCardSeries & rRec) const
{
	int    ok = 1;
	if(pFilt) {
		if(!(pFilt->Flags & SCardSeriesFilt::fShowPassive) && rRec.Flags & SCRDSF_PASSIVE)
			ok = 0;
		else if(pFilt->Flags & SCardSeriesFilt::fOnlyGroups && !(rRec.Flags & SCRDSF_GROUP))
			ok = 0;
		else if(pFilt->Flags & SCardSeriesFilt::fOnlySeries && (rRec.Flags & SCRDSF_GROUP))
			ok = 0;
		else if(pFilt->Flags & SCardSeriesFilt::fOnlyReal && (rRec.Flags & (SCRDSF_GROUP|SCRDSF_RSRVPOOL)))
			ok = 0;
		else if(pFilt->ParentID && (rRec.ParentID != pFilt->ParentID && rRec.ID != pFilt->ParentID))
			ok = 0;
		else if(pFilt->SpecialTreatment && rRec.SpecialTreatment != pFilt->SpecialTreatment)
			ok = 0;
	}
	else if(rRec.Flags & SCRDSF_PASSIVE)
		ok = 0;
	return ok;
}

/*virtual*/ListBoxDef * PPObjSCardSeries::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(pOrgDef, flags, extraPtr);
	AssignImages(p_def);
	return p_def;
}

/*virtual*//*int PPObjSCardSeries::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, flags, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}*/

int PPObjSCardSeries::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->IsValid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(pDef);
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			PPSCardSeries2 scs_rec;
			for(uint i = 0; i < list.getCount(); i++) {
				PPID id = list.at(i);
				if(Fetch(id, &scs_rec) > 0) {
					long img_id = 0;
					int scs_type = scs_rec.GetType();
					switch(scs_type) {
						case scstRsrvPool: break;
						case scstGroup: img_id = PPDV_FOLDER01; break;
						case scstBonus: img_id = PPDV_SCARDBONUS; break;
						case scstCredit: img_id = PPDV_SCARDCREDIT; break;
						case scstDiscount: img_id = PPDV_SCARDDISCOUNT; break;
					}
					if(img_id)
						p_def->AddVecImageAssoc(id, img_id);
				}
			}
		}
	}
	return 1;
}

StrAssocArray * PPObjSCardSeries::MakeStrAssocList(void * extraPtr)
{
	SCardSeriesFilt scs_filt;
	InitFilt(extraPtr, scs_filt);
	SString temp_buf;
	PPSCardSeries rec;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if(CheckForFilt(&scs_filt, rec)) {
			PPID   parent_id = rec.ParentID;
			THROW_SL(p_list->Add(rec.ID, parent_id, rec.Name));
		}
	}
	p_list->RemoveRecursion(0);
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjSCardSeries::GetCodeRange(PPID serID, SString & rLow, SString & rUpp)
{
	PPSCardSerPacket pack;
	rLow.Z();
	rUpp.Z();
	return (GetPacket(serID, &pack) > 0) ? SCardCore::GetCodeRange(pack.Eb.CodeTempl, rLow, rUpp) : -1;
}

class Storage_SCardRule {  // @persistent @store(PropertyTbl)
public:
	void * operator new(size_t sz, const PPSCardSerRule * pSrc, size_t preallocCount, int prop)
	{
		const size_t item_size = pSrc->getItemSize();
		size_t allocated_count = MAX(pSrc->getCount(), preallocCount);
		size_t s = sz + item_size * allocated_count;
		Storage_SCardRule * p = static_cast<Storage_SCardRule *>(SAlloc::M(s));
		if(p) {
			memzero(p, s);
			p->ObjType = PPOBJ_SCARDSERIES;
			p->Prop = prop;
			p->Ver = 1;
			p->CancelPrd = pSrc->Fb.CancelPrd; // @v11.4.9
			p->CancelPrcCount = pSrc->Fb.CancelPrcCount; // @v11.4.9
			p->TrnovrPeriod = pSrc->TrnovrPeriod;
			p->ItemSize = item_size;
			p->AllocatedCount = allocated_count;
			p->ItemsCount = pSrc->getCount();
			for(uint i = 0; i < p->ItemsCount; i++)
				memcpy(PTR8(p + 1) + i * item_size, &pSrc->at(i), item_size);
		}
		return p;
	}
	int    CopyTo(PPSCardSerRule * pDest) const
	{
		int    ok = 1;
		pDest->Ver = Ver;
		pDest->TrnovrPeriod = TrnovrPeriod;
		MEMSZERO(pDest->Fb);
		pDest->Fb.CancelPrd = CancelPrd; // @v11.4.9
		pDest->Fb.CancelPrcCount = CancelPrcCount; // @v11.4.9
		pDest->freeAll();
		if(ItemSize == pDest->getItemSize()) {
			for(uint i = 0; i < ItemsCount; i++) {
				pDest->insert(PTR8C(this+1)+(i * ItemSize));
			}
		}
		else
			ok = 0;
		return ok;
	}
	size_t GetSize() const { return (sizeof(*this) + AllocatedCount * ItemSize); }
	PPID   ObjType;
	PPID   ObjID;
	PPID   Prop;
	long   Ver;            //
	uint8  Reserve[48];    // @reserve // @v11.4.9 [52]-->[48]
	int16  CancelPrd;      // @v11.4.9 Тип периода обнуления суммы бонусов 
	uint16 CancelPrcCount; // @v11.4.9 Количество периодов типа CancelPrd после истечения которых сумма бонусов обнуляется.
	uint32 ItemSize;       // @internal Размер элемента.
	uint32 AllocatedCount; // @internal в базе данных AllocatedCount == ItemsCount
	uint32 ItemsCount;     //
	long   TrnovrPeriod;   // SCARDSER_AUTODIS_XXX
	// Items[...]
};

struct Storage_SCardSerExt {
	Storage_SCardSerExt()
	{
		THISZERO();
		Prop = SCARDSERIES_EXT;
		Ver = 1;
	}
	Storage_SCardSerExt(const PPSCardSerPacket::Ext & rS)
	{
		Set(rS);
	}
	void Set(const PPSCardSerPacket::Ext & rS)
	{
		THISZERO();
		Prop = SCARDSERIES_EXT;
		Ver = 1;
		STRNSCPY(CodeTempl, rS.CodeTempl);
		UsageTmStart = rS.UsageTmStart;
		UsageTmEnd = rS.UsageTmEnd;
	}
	PPSCardSerPacket::Ext & Get(PPSCardSerPacket::Ext & rS) const
	{
		rS.Init();
		STRNSCPY(rS.CodeTempl, CodeTempl);
		rS.UsageTmStart = UsageTmStart;
		rS.UsageTmEnd = UsageTmEnd;
		return rS;
	}
	PPID   ObjType;
	PPID   ObjID;
	PPID   Prop;
	long   Ver;            //
	char   CodeTempl[32];  // (перенесено из заголовочной структуры) Шаблон номеров карт
	char   Reserve[20];    // [52]-->[20]
	LTIME  UsageTmStart;   // 
	LTIME  UsageTmEnd;     // 
	long   Reserve2[2];    //
};

int PPObjSCardSeries::Search(PPID id, void * b)
{
	PPSCardSeries2 * p_rec = static_cast<PPSCardSeries2 *>(b);
	int    ok = PPObjReference::Search(id, p_rec);
	if(ok > 0 && p_rec)
		p_rec->Verify();
	return ok;
}

int PPObjSCardSeries::GetPacket(PPID id, PPSCardSerPacket * pPack)
{
	int    ok = -1, r = -1;
	Reference * p_ref = PPRef;
	PropPPIDArray * p_rec = 0;
	Storage_SCardRule * p_strg = 0;
	PPSCardSerPacket pack;
	if(PPCheckGetObjPacketID(Obj, id)) {
		THROW(Search(id, &pack.Rec) > 0);
		//
		// Правила для чеков
		//
		THROW_MEM(p_strg = new (&pack.CcAmtDisRule, 128, SCARDSERIES_CCHECKRULE) Storage_SCardRule);
		if(p_ref->GetProperty(Obj, id, SCARDSERIES_CCHECKRULE, p_strg, p_strg->GetSize()) > 0)
			THROW(p_strg->CopyTo(&pack.CcAmtDisRule));
		ZDELETE(p_strg);
		//
		// Правила для расчета бонусов
		//
		THROW_MEM(p_strg = new (&pack.CcAmtDisRule, 128, SCARDSERIES_BONUSRULE) Storage_SCardRule);
		if(p_ref->GetProperty(Obj, id, SCARDSERIES_BONUSRULE, p_strg, p_strg->GetSize()) > 0)
			THROW(p_strg->CopyTo(&pack.BonusRule));
		ZDELETE(p_strg);
		//
		//
		//
		THROW_MEM(p_strg = new (&pack.Rule, 128, SCARDSERIES_RULE2) Storage_SCardRule);
		if((r = p_ref->GetProperty(Obj, id, SCARDSERIES_RULE2, p_strg, p_strg->GetSize())) > 0) {
			THROW(p_strg->CopyTo(&pack.Rule));
		}
		else {
			struct Item_ {
				RealRange R;
				double Value;
			};
			const  size_t init_count = 256 / sizeof(Item_);
			size_t sz = sizeof(long) + sizeof(PropPPIDArray) + init_count * sizeof(Item_);
			THROW_MEM(p_rec = static_cast<PropPPIDArray *>(SAlloc::C(1, sz)));
			pack.Rule.freeAll();
			if((r = p_ref->GetProperty(Obj, id, SCARDSERIES_RULE, p_rec, sz)) > 0) {
				if(p_rec->Count > (long)init_count) {
					sz = sizeof(long) + sizeof(PropPPIDArray) + (size_t)p_rec->Count * sizeof(Item_);
					SAlloc::F(p_rec);
					THROW_MEM(p_rec = static_cast<PropPPIDArray *>(SAlloc::C(1, sz)));
					THROW(p_ref->GetProperty(Obj, id, SCARDSERIES_RULE, p_rec, sz) > 0);
				}
				memcpy(&pack.Rule.TrnovrPeriod, PTR8(p_rec + 1), sizeof(long));
				for(uint i = 0; i < (uint)p_rec->Count; i++) {
					TrnovrRngDis item;
					MEMSZERO(item);
					const Item_ * p_item_ = reinterpret_cast<const Item_ *>(PTR8(p_rec+1) + sizeof(long) + i * sizeof(Item_));
					item.R = p_item_->R;
					item.Value = p_item_->Value;
					THROW_SL(pack.Rule.insert(&item));
				}
				pack.Rule.Ver = 0;
				MEMSZERO(pack.Rule.Fb);
			}
		}
		THROW(r);
		//
		// Список видов котировок
		//
		THROW(p_ref->GetPropArray(Obj, pack.Rec.ID, SCARDSERIES_QKLIST, &pack.QuotKindList_)); // @v7.4.0
		{
			// Блок расширения
			Storage_SCardSerExt se;
			int    se_r = 0;
			THROW(se_r = p_ref->GetProperty(Obj, pack.Rec.ID, SCARDSERIES_EXT, &se, sizeof(se))); // @v8.7.12
			if(se_r > 0)
				se.Get(pack.Eb);
		}
		ok = 1;
	}
	else
		pack.Z();
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	SAlloc::F(p_rec);
	delete p_strg;
	return ok;
}

int PPObjSCardSeries::PutPacket(PPID * pID, PPSCardSerPacket * pPack, int use_ta)
{
	int    ok = -1, eq = 0;
	Reference * p_ref = PPRef;
	int    rec_updated = 0;
	PPID   acn_id = 0;
	Storage_SCardRule * p_strg = 0;
	if(pID) {
		if(*pID) {
			SETIFZ(P_ScObj, new PPObjSCard);
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			pPack->Normalize();
			if(*pID) {
				PPSCardSerPacket pattern;
				THROW(CheckRights(PPR_MOD));
				THROW(GetPacket(*pID, &pattern) > 0);
				if(pattern.IsEq(*pPack))
					eq = 1;
				else {
					pPack->Rec.VerifTag = 2;
					THROW(rec_updated = StoreItem(Obj, *pID ? pPack->Rec.ID : 0, &pPack->Rec, 0));
					THROW(p_ref->PutProp(Obj, pPack->Rec.ID, SCARDSERIES_RULE, 0, 0)); // Удаляем старую версию правил пересчета карт записи
					if(pattern.Rec.ID && (pPack->Rec.PDis != pattern.Rec.PDis ||
						pPack->Rec.MaxCredit != pattern.Rec.MaxCredit || pPack->Rec.Expiry != pattern.Rec.Expiry)) {
						THROW(P_ScObj->UpdateBySeries(pattern.Rec.ID, 0));
					}
				}
			}
			else {
				THROW(CheckRights(PPR_INS));
				pPack->Rec.VerifTag = 2;
				THROW(StoreItem(Obj, 0, &pPack->Rec, 0));
			}
			if(!eq) {
				THROW_MEM(p_strg = new (&pPack->Rule, 0, SCARDSERIES_RULE2) Storage_SCardRule);
				THROW(p_ref->PutProp(Obj, pPack->Rec.ID, SCARDSERIES_RULE2, p_strg, p_strg->GetSize()));
				//
				// Правила для скидок по чекам
				//
				ZDELETE(p_strg);
				THROW_MEM(p_strg = new (&pPack->CcAmtDisRule, 0, SCARDSERIES_CCHECKRULE) Storage_SCardRule);
				THROW(p_ref->PutProp(Obj, pPack->Rec.ID, SCARDSERIES_CCHECKRULE, p_strg, p_strg->GetSize()));
				//
				// Правило расчета бонусов
				//
				if(!*pID || !(pPack->UpdFlags & PPSCardSerPacket::ufDontChgBonusRule)) {
					ZDELETE(p_strg);
					THROW_MEM(p_strg = new (&pPack->BonusRule, 0, SCARDSERIES_BONUSRULE) Storage_SCardRule);
					THROW(p_ref->PutProp(Obj, pPack->Rec.ID, SCARDSERIES_BONUSRULE, p_strg, p_strg->GetSize()));
				}
				//
				if(!*pID || !(pPack->UpdFlags & PPSCardSerPacket::ufDontChgQkList)) {
					THROW(p_ref->PutPropArray(Obj, pPack->Rec.ID, SCARDSERIES_QKLIST, &pPack->QuotKindList_, 0));
				}
				if(!*pID || !(pPack->UpdFlags & PPSCardSerPacket::ufDontChgExt)) {
					Storage_SCardSerExt se(pPack->Eb);
					THROW(p_ref->PutProp(Obj, pPack->Rec.ID, SCARDSERIES_EXT, (pPack->Eb.IsEmpty() ? 0 : &se), sizeof(se), 0));
				}
				ASSIGN_PTR(pID, pPack->Rec.ID);
				if(rec_updated < 0) {
					//
					// Если запись не была изменена методом PPObjReference::EditItem (по причине эквивалентности),
					// но правила, тем не менее, изменились, наша функция должна самостоятельно позаботиться о
					// записи в системный журнал.
					//
					acn_id = PPACN_OBJUPD;
				}
				ok = 1;
			}
			else
				ok = -1;
		}
		else if(*pID) {
			THROW(CheckRights(PPR_DEL));
			THROW(p_ref->RemoveProperty(Obj, *pID, 0, 0));
			THROW(RemoveObjV(*pID, 0, 0, 0));
			acn_id = PPACN_OBJRMV;
			ok = 1;
		}
		if(acn_id)
			DS.LogAction(acn_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
		if(ok > 0)
			Dirty(*pID);
	}
	CATCHZOK
	delete p_strg;
	return ok;
}

int PPObjSCardSeries::Helper_GetChildList(PPID id, PPIDArray & rList, PPIDArray * pStack)
{
	int    ok = 1;
	PPIDArray inner_stack;
	PPSCardSeries rec;
	if(Fetch(id, &rec) > 0) {
		if(rec.GetType() == scstGroup) {
			SETIFZ(pStack, &inner_stack);
			if(pStack->addUnique(id) < 0) {
				PPSetError(PPERR_SCARDSERIESCYCLE, rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
				CALLEXCEPT();
			}
			else {
				PPSCardSeries child_rec;
				for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&child_rec) > 0;)
					if(child_rec.ParentID == id)
						THROW(Helper_GetChildList(child_rec.ID, rList, pStack)); // @recursion
			}
		}
		else
			rList.addUnique(id);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjSCardSeries::GetChildList(PPID id, PPIDArray & rList)
{
	return Helper_GetChildList(id, rList, 0);
}

int PPObjSCardSeries::GetSeriesWithSpecialTreatment(LAssocArray & rList)
{
	int    ok = -1;
	rList.clear();
	PPSCardSeries rec;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if(oneof2(rec.SpecialTreatment, SCRDSSPCTRT_AZ, SCRDSSPCTRT_UDS)) {
			rList.Add(rec.ID, rec.SpecialTreatment);
			ok = 1;
		}
	}
	return ok;
}

SCardChargeRule::SCardChargeRule() : SerID(0), Period(0)
{
	Ap.Z(); // @v11.3.5
}

/*static*/int PPObjSCardSeries::SelectRule(SCardChargeRule * pSelRule)
{
	class SCardChargeRuleDialog : public TDialog {
		DECL_DIALOG_DATA(SCardChargeRule);
	public:
		SCardChargeRuleDialog() : TDialog(DLG_SSAUTODIS)
		{
			SetupCalPeriod(CTLCAL_SSAUTODIS_AP, CTL_SSAUTODIS_AP);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_SSAUTODIS_SSER, PPOBJ_SCARDSERIES, Data.SerID, 0, 0);
			AddClusterAssoc(CTL_SSAUTODIS_PRD,  0, SCARDSER_AUTODIS_PREVPRD);
			AddClusterAssocDef(CTL_SSAUTODIS_PRD,  1, SCARDSER_AUTODIS_THISPRD);
			AddClusterAssoc(CTL_SSAUTODIS_PRD,  2, SCARDSER_AUTODIS_ARBITRARYPRD); // @v11.3.5
			SetClusterData(CTL_SSAUTODIS_PRD, Data.Period);
			SetPeriodInput(this, CTL_SSAUTODIS_AP, Data.Ap);
			Setup();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			const PPSCardSerRule * p_rule = 0;
			getCtrlData(sel = CTLSEL_SSAUTODIS_SSER, &Data.SerID);
			// @v11.3.9 THROW_PP(Data.SerID, PPERR_SCARDSERNEEDED);
			if(Data.SerID) { // @v11.3.9
				PPSCardSerPacket scs_pack;
				THROW(ScsObj.GetPacket(Data.SerID, &scs_pack) > 0);
				p_rule = (scs_pack.Rec.GetType() == scstBonus) ? &scs_pack.BonusRule : &scs_pack.Rule;
			}
			GetClusterData(CTL_SSAUTODIS_PRD, &Data.Period);
			GetPeriodInput(this, CTL_SSAUTODIS_AP, &Data.Ap);
			if(Data.Period == SCARDSER_AUTODIS_ARBITRARYPRD) {
				DateRange temp_range(Data.Ap);
				temp_range.Actualize(ZERODATE);
				sel = CTL_SSAUTODIS_AP;
				THROW_SL(checkdate(temp_range.low) && checkdate(temp_range.upp));
				THROW(temp_range.low <= temp_range.upp);
			}
			else {
				sel = CTL_SSAUTODIS_PRD;
				THROW(oneof2(Data.Period, SCARDSER_AUTODIS_PREVPRD, SCARDSER_AUTODIS_THISPRD));
				THROW(!p_rule || oneof6(p_rule->TrnovrPeriod, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL));
			}
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SSAUTODIS_SSER))
				Setup();
			else if(event.isClusterClk(CTL_SSAUTODIS_PRD))
				Setup();
			else
				return;
			clearEvent(event);
		}
		void    Setup()
		{
			PPSCardSerPacket scs_pack;
			SString info_buf;
			SString temp_buf;
			getCtrlData(CTLSEL_SSAUTODIS_SSER, &Data.SerID);
			GetClusterData(CTL_SSAUTODIS_PRD, &Data.Period);
			if(Data.SerID && ScsObj.GetPacket(Data.SerID, &scs_pack) > 0) {
				const PPSCardSerRule & r_rule = (scs_pack.Rec.GetType() == scstBonus) ? scs_pack.BonusRule : scs_pack.Rule;
				const long prev_p = GetClusterData(CTL_SSAUTODIS_PRD);
				long _p = prev_p;
				PPLoadString("scardserrule_trnovrperiod", info_buf);
				if(r_rule.TrnovrPeriod > 0) {
					setCtrlReadOnly(CTL_SSAUTODIS_AP, true);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 0, false);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 1, false);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 2, true);
					if(prev_p == SCARDSER_AUTODIS_ARBITRARYPRD)
						_p = SCARDSER_AUTODIS_PREVPRD;
					PPGetSubStrById(PPTXT_CYCLELIST, r_rule.TrnovrPeriod, temp_buf);
					info_buf.CatDiv(':', 2).Cat(temp_buf);
					{
						// @v12.3.7 char b[128];
						DateRange dr;
						dr.SetPeriod(getcurdate_(), r_rule.TrnovrPeriod);
						if(_p == SCARDSER_AUTODIS_PREVPRD)
							dr.SetPeriod(plusdate(dr.low, -1), r_rule.TrnovrPeriod);
						// @v12.3.7 temp_buf = periodfmt(dr, b);
						dr.ToStr(0, temp_buf); // @v12.3.7
						setCtrlString(CTL_SSAUTODIS_AP, temp_buf);
					}
				}
				else {
					setCtrlReadOnly(CTL_SSAUTODIS_AP, false);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 0, true);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 1, true);
					DisableClusterItem(CTL_SSAUTODIS_PRD, 2, false);
					if(oneof2(prev_p, SCARDSER_AUTODIS_PREVPRD, SCARDSER_AUTODIS_THISPRD)) {
						_p = SCARDSER_AUTODIS_ARBITRARYPRD;
					}
					info_buf.CatDiv(':', 2).Cat(PPLoadStringS("undefined", temp_buf));
					{
						// @v12.3.7 char b[128];
						// @v12.3.7 temp_buf = periodfmt(Data.Ap, b);
						Data.Ap.ToStr(0, temp_buf); // @v12.3.7
						setCtrlString(CTL_SSAUTODIS_AP, temp_buf);
					}
				}
				if(_p != prev_p)
					SetClusterData(CTL_SSAUTODIS_PRD, _p);
			}
			else {
				DisableClusterItem(CTL_SSAUTODIS_PRD, 0, false); // @v11.3.9 true-->false
				DisableClusterItem(CTL_SSAUTODIS_PRD, 1, false); // @v11.3.9 true-->false
				DisableClusterItem(CTL_SSAUTODIS_PRD, 2, false); // @v11.3.9 true-->false
				info_buf.Z();
			}
			setStaticText(CTL_SSAUTODIS_ST_INFO, info_buf);
		}
		PPObjSCardSeries ScsObj;
	};
	DIALOG_PROC_BODY(SCardChargeRuleDialog, pSelRule);
}

/*static*/int PPObjSCardSeries::SetSCardsByRule(const SCardChargeRule * pSelRule)
{
	int    ok = -1;
	PPLogger logger;
	SCardChargeRule scs_rule;
	if(pSelRule) {
		scs_rule = *pSelRule;
		ok = 1;
	}
	else {
		THROW(ok = SelectRule(&scs_rule));
	}
	PPWaitStart();
	if(ok > 0) {
		PPObjSCardSeries scs_obj;
		PPObjSCard sc_obj;
		for(PPID id = 0; scs_rule.SerID || scs_obj.EnumItems(&id) > 0;) {
			id = NZOR(scs_rule.SerID, id);
			THROW(sc_obj.UpdateBySeriesRule2(id, /*BIN(scs_rule.Period == SCARDSER_AUTODIS_PREVPRD)*/scs_rule, &logger, 1));
			ok = 1;
			if(scs_rule.SerID)
				break;
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	logger.Save(PPFILNAM_INFO_LOG, 0);
	return ok;
}

int PPObjSCardSeries::Edit(PPID * pID, void * extraPtr)
{
	class SCardSeriaDlg : public TDialog {
	public:
		SCardSeriaDlg(PPObjSCardSeries * pObj) : TDialog(DLG_SCARDSER), P_SCSerObj(pObj)
		{
			SetupCalDate(CTLCAL_SCARDSER_DATE,   CTL_SCARDSER_DATE);
			SetupCalDate(CTLCAL_SCARDSER_EXPIRY, CTL_SCARDSER_EXPIRY);
		}
		int    setDTS(const PPSCardSerPacket * pData)
		{
			if(!RVALUEPTR(Data, pData))
				Data.Z();
			long   _type = Data.Rec.GetType();
			{
				SCardSeriesFilt scs_filt;
				scs_filt.Flags = scs_filt.fOnlyGroups;
				SetupPPObjCombo(this, CTLSEL_SCARDSER_PARENT, PPOBJ_SCARDSERIES, Data.Rec.ParentID, 0, &scs_filt);
			}
			{
				SCardSeriesFilt scs_filt;
				scs_filt.Flags = scs_filt.fOnlyReal;
				SetupPPObjCombo(this, CTLSEL_SCARDSER_RPDEST, PPOBJ_SCARDSERIES, Data.Rec.RsrvPoolDestSerID, 0, &scs_filt);
			}
			AddClusterAssocDef(CTL_SCARDSER_TYPE, 0, scstDiscount);
			AddClusterAssoc(CTL_SCARDSER_TYPE, 1, scstCredit);
			AddClusterAssoc(CTL_SCARDSER_TYPE, 2, scstBonus);
			AddClusterAssoc(CTL_SCARDSER_TYPE, 3, scstGroup);
			AddClusterAssoc(CTL_SCARDSER_TYPE, 4, scstRsrvPool);
			SetClusterData(CTL_SCARDSER_TYPE, _type);
			setCtrlData(CTL_SCARDSER_NAME, Data.Rec.Name);
			setCtrlData(CTL_SCARDSER_SYMB, Data.Rec.Symb);
			setCtrlData(CTL_SCARDSER_ID,   &Data.Rec.ID);
			disableCtrl(CTL_SCARDSER_ID,   (!PPMaster || Data.Rec.ID));
			setCtrlData(CTL_SCARDSER_CODETEMPL, Data.Eb.CodeTempl);
			//
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 0, SCRDSF_USEDSCNTIFNQUOT);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 1, SCRDSF_MINQUOTVAL);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 2, SCRDSF_UHTTSYNC);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 3, SCRDSF_DISABLEADDPAYM);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 4, SCRDSF_NEWSCINHF);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 5, SCRDSF_TRANSFDISCOUNT);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 6, SCRDSF_PASSIVE);
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 7, SCRDSF_ALLOWOWNERAUTOCR); // @v11.6.3
			AddClusterAssoc(CTL_SCARDSER_FLAGS, 8, SCRDSF_QUANTACCOUNTING); // @v12.0.5
			SetClusterData(CTL_SCARDSER_FLAGS, Data.Rec.Flags);
			//
			SetupPPObjCombo(this, CTLSEL_SCARDSER_QUOTKIND, PPOBJ_QUOTKIND, Data.Rec.QuotKindID_s, OLW_CANINSERT, 0);
			SetupQuotKind(0);
			if(!Data.Rec.PersonKindID) {
				PPSCardConfig sc_cfg;
				Data.Rec.PersonKindID = (PPObjSCardSeries::FetchConfig(&sc_cfg) > 0 && sc_cfg.PersonKindID) ? sc_cfg.PersonKindID : GetSellPersonKind();
			}
			SetupPPObjCombo(this, CTLSEL_SCARDSER_PSNKIND,  PPOBJ_PERSONKIND, Data.Rec.PersonKindID, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0);
			SetupByType(0);
			setCtrlData(CTL_SCARDSER_DATE,   &Data.Rec.Issue);
			setCtrlData(CTL_SCARDSER_EXPIRY, &Data.Rec.Expiry);
			setCtrlReal(CTL_SCARDSER_PDIS, fdiv100i(Data.Rec.PDis));
			setCtrlData(CTL_SCARDSER_MAXCRED, &Data.Rec.MaxCredit);
			setCtrlReal(CTL_SCARDSER_FIXBON, fdiv100i(Data.Rec.FixedBonus));
			SetTimeRangeInput(this, CTL_SCARDSER_USAGETM, TIMF_HM, &Data.Eb.UsageTmStart, &Data.Eb.UsageTmEnd);
			{
				long   bonus_ext_rule = 0;
				double bonus_ext_rule_val = 0.0;
				if(Data.Rec.Flags & SCRDSF_BONUSER_ONBNK)
					bonus_ext_rule = 1;
				AddClusterAssoc(CTL_SCARDSER_BONER, 0, 1);
				SetClusterData(CTL_SCARDSER_BONER, bonus_ext_rule);
				if(bonus_ext_rule)
					bonus_ext_rule_val = ((double)Data.Rec.BonusChrgExtRule) / 10.0;
				setCtrlReal(CTL_SCARDSER_BONERVAL, bonus_ext_rule_val);
			}
			SetupStringCombo(this, CTLSEL_SCARDSER_SPCTRT, PPTXT_SCARDSERSPCTREATMENT, Data.Rec.SpecialTreatment);
			return 1;
		}
		int    getDTS(PPSCardSerPacket * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			double pdis = 0.0;
			getCtrlData(sel = CTL_SCARDSER_NAME, Data.Rec.Name);
			THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
			THROW(P_SCSerObj->CheckDupName(Data.Rec.ID, Data.Rec.Name));
			getCtrlData(sel = CTL_SCARDSER_SYMB, Data.Rec.Symb);
			THROW(P_SCSerObj->P_Ref->CheckUniqueSymb(PPOBJ_SCARDSERIES, Data.Rec.ID, Data.Rec.Symb, offsetof(PPSCardSeries, Symb)));
			getCtrlData(CTLSEL_SCARDSER_PARENT, &Data.Rec.ParentID);
			getCtrlData(CTL_SCARDSER_ID,   &Data.Rec.ID);
			getCtrlData(CTL_SCARDSER_CODETEMPL, Data.Eb.CodeTempl);
			getCtrlData(sel = CTL_SCARDSER_DATE,   &Data.Rec.Issue);
			THROW_SL(checkdate(Data.Rec.Issue, 1));
			getCtrlData(sel = CTL_SCARDSER_EXPIRY, &Data.Rec.Expiry);
			THROW_SL(checkdate(Data.Rec.Expiry, 1));
			long   _type = GetClusterData(CTL_SCARDSER_TYPE);
			Data.Rec.SetType(_type);
			GetClusterData(CTL_SCARDSER_FLAGS, &Data.Rec.Flags);
			getCtrlData(CTLSEL_SCARDSER_QUOTKIND, &Data.Rec.QuotKindID_s);
			getCtrlData(CTLSEL_SCARDSER_PSNKIND,  &Data.Rec.PersonKindID);
			if(Data.Rec.GetType() == scstBonus) {
				Data.Rec.BonusGrpID = getCtrlLong(CTLSEL_SCARDSER_CRDGGRP);
				Data.Rec.CrdGoodsGrpID = 0;
				Data.Rec.BonusChrgGrpID = getCtrlLong(CTLSEL_SCARDSER_BONCGRP);
				Data.Rec.ChargeGoodsID = 0;
			}
			else if(Data.Rec.GetType() == scstCredit) {
				Data.Rec.CrdGoodsGrpID = getCtrlLong(CTLSEL_SCARDSER_CRDGGRP);
				Data.Rec.ChargeGoodsID = getCtrlLong(CTLSEL_SCARDSER_BONCGRP);
			}
			else {
				Data.Rec.BonusGrpID = 0;
				Data.Rec.CrdGoodsGrpID = 0;
				Data.Rec.BonusChrgGrpID = 0;
				Data.Rec.ChargeGoodsID = 0;
			}
			Data.Rec.RsrvPoolDestSerID = (Data.Rec.GetType() == scstRsrvPool) ? getCtrlLong(CTLSEL_SCARDSER_RPDEST) : 0;
			Data.Rec.PersonKindID = NZOR(Data.Rec.PersonKindID, PPPRK_CLIENT);
			getCtrlData(sel = CTL_SCARDSER_PDIS, &pdis);
			THROW_PP(pdis >= -300 && pdis <= 100, PPERR_USERINPUT);
			Data.Rec.PDis = fmul100i(pdis);
			getCtrlData(CTL_SCARDSER_MAXCRED, &Data.Rec.MaxCredit);
			{
				double rval = getCtrlReal(sel = CTL_SCARDSER_FIXBON);
				Data.Rec.FixedBonus = fmul100i(rval);
			}
			THROW(GetTimeRangeInput(this, CTL_SCARDSER_USAGETM, TIMF_HM, &Data.Eb.UsageTmStart, &Data.Eb.UsageTmEnd));
			{
				long   bonus_ext_rule = 0;
				double bonus_ext_rule_val = 0.0;
				if(Data.Rec.Flags & SCRDSF_BONUSER_ONBNK)
					bonus_ext_rule = 1;
				GetClusterData(CTL_SCARDSER_BONER, &bonus_ext_rule);
				Data.Rec.Flags &= ~SCRDSF_BONUSER_ONBNK;
				if(bonus_ext_rule) {
					if(bonus_ext_rule & 0x01)
						Data.Rec.Flags |= SCRDSF_BONUSER_ONBNK;
					bonus_ext_rule_val = getCtrlReal(CTL_SCARDSER_BONERVAL);
					Data.Rec.BonusChrgExtRule = (int16)(bonus_ext_rule_val * 10.0);
				}
			}
			getCtrlData(CTLSEL_SCARDSER_SPCTRT, &Data.Rec.SpecialTreatment);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmSCardRule)) {
				if(!EditRule(PPSCardSerRule::rultDisc))
					PPError();
			}
			if(event.isCmd(cmBonusRule)) {
				if(Data.Rec.GetType() == scstBonus)
					if(!EditRule(PPSCardSerRule::rultBonus))
						PPError();
			}
			else if(event.isCmd(cmCCheckRule)) {
				if(!EditRule(PPSCardSerRule::rultCcAmountDisc))
					PPError();
			}
			else if(event.isCmd(cmQuotKindList)) {
				PPID   qk_id = getCtrlLong(CTLSEL_SCARDSER_QUOTKIND);
				if(Data.QuotKindList_.getCount() == 0 && qk_id)
					Data.QuotKindList_.add(qk_id);
				ListToListData ll_data(PPOBJ_QUOTKIND, 0, &Data.QuotKindList_);
				ll_data.TitleStrID = PPTXT_SELQUOTKIND;
				int    r = ListToListDialog(&ll_data);
				if(r > 0)
					SetupQuotKind(1);
				else if(!r)
					PPError();
			}
			else if(event.isClusterClk(CTL_SCARDSER_TYPE)) {
				long   _prev_type = Data.Rec.GetType();
				long   _type = GetClusterData(CTL_SCARDSER_TYPE);
				if(Data.Rec.SetType(_type) > 0) {
					SetupByType(_prev_type);
				}
			}
			else if(event.isCbSelected(CTLSEL_SCARDSER_QUOTKIND)) {
				getCtrlData(CTLSEL_SCARDSER_QUOTKIND, &Data.Rec.QuotKindID_s);
				DisableClusterItem(CTL_SCARDSER_FLAGS, 0, !Data.Rec.QuotKindID_s);
			}
			else
				return;
			clearEvent(event);
		}
		int    EditRule(int ruleType)
		{
			PPSCardSerRule * p_rule = 0;
			switch(ruleType) {
				case PPSCardSerRule::rultCcAmountDisc: p_rule = &Data.CcAmtDisRule; break;
				case PPSCardSerRule::rultDisc: p_rule = &Data.Rule; break;
				case PPSCardSerRule::rultBonus: p_rule = &Data.BonusRule; break;
				default: break;
			}
			if(p_rule) {
				DIALOG_PROC_BODY_P1(SCardRuleDlg, ruleType, p_rule);
			}
			else
				return -1;
		}
		void   SetupQuotKind(int byList)
		{
			if(Data.QuotKindList_.getCount() > 1) {
				setCtrlLong(CTLSEL_SCARDSER_QUOTKIND, Data.Rec.QuotKindID_s = 0);
				SetComboBoxListText(this, CTLSEL_SCARDSER_QUOTKIND);
				disableCtrl(CTLSEL_SCARDSER_QUOTKIND, true);
				Data.Rec.QuotKindID_s = 0;
			}
			else {
				disableCtrl(CTLSEL_SCARDSER_QUOTKIND, false);
				if(Data.QuotKindList_.getCount() == 1)
					setCtrlLong(CTLSEL_SCARDSER_QUOTKIND, Data.Rec.QuotKindID_s = Data.QuotKindList_.get(0));
				else if(byList)
					setCtrlLong(CTLSEL_SCARDSER_QUOTKIND, Data.Rec.QuotKindID_s = 0);
			}
			DisableClusterItem(CTL_SCARDSER_FLAGS, 0, !(Data.Rec.QuotKindID_s || Data.QuotKindList_.getCount()));
			DisableClusterItem(CTL_SCARDSER_FLAGS, 1, Data.QuotKindList_.getCount() < 2);
		}
		void   SetupByType(int prevType)
		{
			SString temp_buf;
			int    txt_id = 0;
			int    txt2_id = 0;
			const  long _type = Data.Rec.GetType();
			PPID   init_grp_id = 0;
			enableCommand(cmBonusRule, _type == scstBonus);
			disableCtrl(CTLSEL_SCARDSER_RPDEST, _type != scstRsrvPool);
			if(prevType == scstBonus)
				Data.Rec.BonusChrgGrpID = getCtrlLong(CTLSEL_SCARDSER_BONCGRP);
			else if(prevType == scstCredit)
				Data.Rec.ChargeGoodsID = getCtrlLong(CTLSEL_SCARDSER_BONCGRP);
			DisableClusterItem(CTL_SCARDSER_FLAGS, 3, _type != scstCredit);
			if(_type == scstBonus) {
				txt_id = PPTXT_SCS_AUTOWROFFGGRP_BON;
				txt2_id = PPTXT_SCS_BONUSCHARGEGGRP;
				init_grp_id = Data.Rec.BonusGrpID;
				setCtrlLong(CTLSEL_SCARDSER_CRDGGRP, Data.Rec.BonusGrpID);
				showCtrl(CTLSEL_SCARDSER_BONCGRP, true);
				showCtrl(CTL_SCARDSER_BONER, true);
				showCtrl(CTL_SCARDSER_BONERVAL, true);
				SetupPPObjCombo(this, CTLSEL_SCARDSER_BONCGRP, PPOBJ_GOODSGROUP, Data.Rec.BonusChrgGrpID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
			}
			else {
				showCtrl(CTL_SCARDSER_BONER, false);
				showCtrl(CTL_SCARDSER_BONERVAL, false);
				if(_type == scstCredit) {
					txt_id = PPTXT_SCS_AUTOWROFFGGRP_CRD;
					txt2_id = PPTXT_SCS_CHARGEGOODS;
					init_grp_id = Data.Rec.CrdGoodsGrpID;
					showCtrl(CTLSEL_SCARDSER_BONCGRP, true);
					{
						PPSCardConfig sc_cfg;
						PPObjSCardSeries::FetchConfig(&sc_cfg);
						if(sc_cfg.ChargeGoodsID) {
							PPObjGoods goods_obj;
							Goods2Tbl::Rec goods_rec;
							if(goods_obj.Fetch(sc_cfg.ChargeGoodsID, &goods_rec) > 0)
								SetupPPObjCombo(this, CTLSEL_SCARDSER_BONCGRP, PPOBJ_GOODS, Data.Rec.ChargeGoodsID,
									OLW_CANINSERT|OLW_LOADDEFONOPEN, reinterpret_cast<void *>(goods_rec.ParentID));
						}
					}
				}
				else {
					txt_id = PPTXT_SCS_AUTOWROFFGGRP;
					txt2_id = 0;
					init_grp_id = Data.Rec.CrdGoodsGrpID;
					setCtrlLong(CTLSEL_SCARDSER_CRDGGRP, Data.Rec.CrdGoodsGrpID);
					showCtrl(CTLSEL_SCARDSER_BONCGRP, false);
				}
			}
			showCtrl(CTL_SCARDSER_FIXBON, oneof2(_type, scstCredit, scstBonus));
			if(prevType == 0)
				SetupPPObjCombo(this, CTLSEL_SCARDSER_CRDGGRP, PPOBJ_GOODSGROUP, init_grp_id, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0);
			setLabelText(CTL_SCARDSER_CRDGGRP, PPLoadTextS(txt_id, temp_buf));
			setLabelText(CTL_SCARDSER_BONCGRP, PPLoadTextS(txt2_id, temp_buf));
		}
		PPSCardSerPacket Data;
		PPObjSCardSeries * P_SCSerObj;
	};
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	ushort v = 0;
	double pdis = 0.0;
	PPSCardSerPacket pack;
	SCardSeriaDlg * p_dlg = 0;
	THROW(GetPacket(*pID, &pack));
	THROW(CheckDialogPtr(&(p_dlg = new SCardSeriaDlg(this))));
	p_dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
		if(!p_dlg->getDTS(&pack))
			PPError();
		else {
			THROW(PutPacket(pID, &pack, 1));
			valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

IMPL_DESTROY_OBJ_PACK(PPObjSCardSeries, PPSCardSerPacket);

int PPObjSCardSeries::SerializePacket(int dir, PPSCardSerPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->Rule.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->CcAmtDisRule.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->BonusRule.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, &pPack->QuotKindList_, rBuf));
	THROW(pSCtx->SerializeBlock(dir, sizeof(pPack->Eb), &pPack->Eb, rBuf, 1));
	CATCHZOK
	return ok;
}

int PPObjSCardSeries::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjSCardSeries, PPSCardSerPacket>(this, p, id, stream, pCtx); }

int PPObjSCardSeries::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPSCardSerPacket * p_pack = p ? static_cast<PPSCardSerPacket *>(p->Data) : 0;
	if(p_pack)
		if(stream == 0) {
			p_pack->Rec.ID = *pID;
			// @v9.8.9 (блок расширения теперь передается) p_pack->UpdFlags |= PPSCardSerPacket::ufDontChgExt; // @v8.7.12
			if(!PutPacket(pID, p_pack, 1)) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSCARDSER, p_pack->Rec.ID, p_pack->Rec.Name);
				ok = -1;
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	CATCHZOK
	return ok;
}

int PPObjSCardSeries::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPSCardSerPacket * p_pack = static_cast<PPSCardSerPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_QUOTKIND,   &p_pack->Rec.QuotKindID_s, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSONKIND,   &p_pack->Rec.PersonKindID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_pack->Rec.CrdGoodsGrpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_pack->Rec.BonusGrpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &p_pack->Rec.BonusChrgGrpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_SCARDSERIES, &p_pack->Rec.ParentID, ary, replace));
		for(uint i = 0; i < p_pack->QuotKindList_.getCount(); i++) {
			THROW(ProcessObjRefInArray(PPOBJ_QUOTKIND, &p_pack->QuotKindList_.at(i), ary, replace));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
SCardSeriesView::SCardSeriesView(PPObjSCardSeries * _ppobj) : ObjViewDialog(DLG_SCSERIESVIEW, _ppobj, 0), CurPrnPos(0)
{
}

void SCardSeriesView::extraProc(long id)
{
	if(id)
		ShowObjects(PPOBJ_SCARD, reinterpret_cast<void *>(id));
}

int SCardSeriesView::InitIteration()
{
	CurPrnPos = 0;
	return 1;
}

int FASTCALL SCardSeriesView::NextIteration(PPSCardSeries * pRec)
{
	int    ok = -1;
	if(pRec && SmartListBox::IsValidS(P_List)) {
		const SArray * p_scs_ary = static_cast<const StdListBoxDef *>(P_List->P_Def)->P_Data;
		if(p_scs_ary && p_scs_ary->getCount() > CurPrnPos) {
			PPID   id = *static_cast<const  PPID *>(p_scs_ary->at(CurPrnPos++));
			if(P_Obj && P_Obj->Search(id, pRec) > 0)
				ok = 1;
		}
	}
	return ok;
}

int SCardSeriesView::Print()
{
	return PPAlddPrint(REPORT_SCARDSERIESVIEW, PView(this), 0);
}

int PPObjSCardSeries::Browse(void * extraPtr)
{
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new SCardSeriesView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}
//
//
//
PPObjSCardSeriesListWindow::PPObjSCardSeriesListWindow(PPObject * pObj, uint flags, void * extraPtr) :
	PPObjListWindow(pObj, flags, extraPtr), CurIterPos(0)
{
	if(pObj) {
		static_cast<PPObjSCardSeries *>(pObj)->InitFilt(extraPtr, Filt);
	}
	ExtraPtr = &Filt;
	DefaultCmd = cmaMore;
	SetToolbar(TOOLBAR_LIST_SCARDSERIES);
}

int PPObjSCardSeriesListWindow::InitIteration()
{
	CurIterPos = 0;
	return 1;
}

int FASTCALL PPObjSCardSeriesListWindow::NextIteration(PPSCardSeries * pRec)
{
	int    ok = -1;
	if(pRec && P_Def) {
		const StrAssocArray * p_scs_ary = static_cast<const StrAssocListBoxDef *>(P_Def)->getArray();
		if(p_scs_ary && CurIterPos < p_scs_ary->getCount()) {
			StrAssocArray::Item item = p_scs_ary->Get(CurIterPos++);
			PPID   id = item.Id;
			PPObjSCardSeries * p_sc_obj = static_cast<PPObjSCardSeries *>(P_Obj);
			if(p_sc_obj && p_sc_obj->Search(id, pRec) > 0)
				ok = 1;
		}
	}
	return ok;
}

IMPL_HANDLE_EVENT(PPObjSCardSeriesListWindow)
{
	int    update = 0;
	PPObjListWindow::handleEvent(event);
	if(P_Obj) {
		PPID   current_id = 0;
		PPObjSCardSeries * p_sc_obj = static_cast<PPObjSCardSeries *>(P_Obj);
		getResult(&current_id);
		if(TVCOMMAND) {
			switch(TVCMD) {
				case cmaMore:
					if(current_id) {
						SCardFilt filt;
						filt.SeriesID = current_id;
						static_cast<PPApp *>(APPL)->LastCmd = TVCMD;
						PPView::Execute(PPVIEW_SCARD, &filt, 1, 0);
					}
					break;
				case cmPrint:
					PPAlddPrint(REPORT_SCARDSERIESVIEW, PView(this), 0);
					break;
				default:
					break;
			}
		}
		PostProcessHandleEvent(update, current_id);
	}
}

/*virtual*/void * PPObjSCardSeries::CreateObjListWin(uint flags, void * extraPtr)
{
	return new PPObjSCardSeriesListWindow(this, flags, extraPtr);
}
//
//
//
PPObjSCard::AddParam::AddParam(PPID serID, PPID ownerID) : SerID(serID), OwnerID(ownerID), LocID(0)
{
}

PPObjSCard::Filt::Filt() : Signature(PPConst::Signature_PPObjSCard_Filt), SeriesID(0), OwnerID(0)
{
}

TLP_IMPL(PPObjSCard, CCheckCore, P_CcTbl);

PPObjSCard::PPObjSCard(void * extraPtr) : PPObject(PPOBJ_SCARD), ExtraPtr(extraPtr), P_CsObj(0),
	DoObjVer_SCard(CConfig.Flags2 & CCFLG2_USEHISTSCARD)
{
	TLP_OPEN(P_CcTbl);
	P_Tbl = P_CcTbl ? &P_CcTbl->Cards : 0;
	ImplementFlags |= implStrAssocMakeList;
	Cfg.Flags &= ~PPSCardConfig::fValid;
}

PPObjSCard::~PPObjSCard()
{
	TLP_CLOSE(P_CcTbl);
	delete P_CsObj;
}

const PPSCardConfig & PPObjSCard::GetConfig()
{
	if(!(Cfg.Flags & PPSCardConfig::fValid))
		PPObjSCard::ReadConfig(&Cfg);
	return Cfg;
}

PPID FASTCALL PPObjSCard::GetChargeGoodsID(PPID cardID)
{
	PPID   charge_goods_id = GetConfig().ChargeGoodsID;
	if(cardID) {
		SCardTbl::Rec sc_rec;
		if(Fetch(cardID, &sc_rec) > 0) {
			PPObjSCardSeries scs_obj;
			PPSCardSeries scs_rec;
			if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0 && scs_rec.GetType() == scstCredit && scs_rec.ChargeGoodsID)
				charge_goods_id = scs_rec.ChargeGoodsID;
		}
	}
	return charge_goods_id;
}

int PPObjSCard::Search(PPID id, void * b) { return P_Tbl->Search(id, static_cast<SCardTbl::Rec *>(b)); }
/*virtual*/const char * PPObjSCard::GetNamePtr() { return P_Tbl->data.Code; }

int PPObjSCard::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTSCARD, CTL_RTSCARD_FLAGS, CTL_RTSCARD_SFLAGS, bufSize, rt, pDlg);
}

int PPObjSCard::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_SCARDSERIES) {
			SCardTbl::Key2 k;
			MEMSZERO(k);
			k.SeriesID = _id;
			if(P_Tbl->search(2, &k, spGe) && k.SeriesID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
			else
				ok = (BTROKORNFOUND) ? DBRPL_OK : PPSetErrorDB();
		}
		else if(_obj == PPOBJ_PERSON) {
			SCardTbl::Key3 k;
			MEMSZERO(k);
			k.PersonID = _id;
			if(P_Tbl->search(3, &k, spEq))
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
			else
				ok = (BTROKORNFOUND) ? DBRPL_OK : PPSetErrorDB();
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_PERSON) {
			SCardTbl::Key3 k;
			MEMSZERO(k);
			k.PersonID = _id;
			while(ok && P_Tbl->search(3, &k, spEq)) {
				P_Tbl->data.PersonID = reinterpret_cast<long>(extraPtr);
				if(!P_Tbl->updateRec())
					ok = PPSetErrorDB();
			}
			if(ok != DBRPL_ERROR)
				ok = (BTROKORNFOUND) ? DBRPL_OK : PPSetErrorDB();
		}
	}
	return ok;
}

int PPObjSCard::IsCreditSeries(PPID scSerID)
{
	return BIN(GetSeriesType(scSerID) == scstCredit);
}

/*static*/int PPObjSCard::GetSeriesType(PPID scSerID)
{
	PPObjSCardSeries scs_obj;
	PPSCardSeries scs_rec;
	return (scs_obj.Fetch(scSerID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
}

int PPObjSCard::GetCardType(PPID cardID)
{
	SCardTbl::Rec rec;
	return (cardID && Search(cardID, &rec) > 0) ? GetSeriesType(rec.SeriesID) : scstUnkn;
}

int PPObjSCard::IsCreditCard(PPID cardID)
{
	return BIN(GetCardType(cardID) == scstCredit);
}

int PPObjSCard::SearchCode(PPID seriesID, const char * pCode, SCardTbl::Rec * pRec)
{
	return P_Tbl->SearchCode(seriesID, pCode, pRec);
}

int PPObjSCard::Helper_GetListBySubstring(const char * pSubstr, PPID seriesID, void * pList, long flags)
{
	int    ok = 1;
	int    r = 0;
	const  size_t substr_len = sstrlen(pSubstr);
	PPIDArray * p_list = 0;
	StrAssocArray * p_str_list = 0;
	if(flags & clsfStrList)
		p_str_list = static_cast<StrAssocArray *>(pList);
	else
		p_list = (PPIDArray *)pList;
	if(substr_len) {
		PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
		if(p_cli) {
			SString q, temp_buf;
			q.Cat("SELECT").Space().Cat("SCARD").Space().Cat("BY").Space();
			q.Cat("SUBNAME");
			{
				if(flags & clsfFromBeg)
					temp_buf = pSubstr;
				else
					temp_buf.Z().CatChar('*').Cat(pSubstr);
				q.CatParStr(temp_buf).Space();
			}
			if(seriesID) {
				q.Cat("SERIES").CatParStr(temp_buf.Z().Cat(seriesID)).Space();
			}
			q.Cat("FORMAT").DotCat("BIN").CatParStr(static_cast<const char *>(0));
			PPJobSrvReply reply;
			if(p_cli->ExecSrvCmd(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				if(p_list) {
					StrAssocArray temp_str_list;
					temp_str_list.Read(reply, 0);
					for(uint i = 0; i < temp_str_list.getCount(); i++) {
						StrAssocArray::Item item = temp_str_list.at_WithoutParent(i);
						THROW_SL(p_list->addUnique(item.Id));
					}
				}
				else if(p_str_list) {
					p_str_list->Read(reply, 0);
					p_str_list->ClearParents();
				}
				r = 1;
			}
		}
		if(!r) {
			const StrAssocArray * p_full_list = GetFullList();
			if(p_full_list) {
				SCardTbl::Rec sc_rec;
				const uint c = p_full_list->getCount();
				for(uint i = 0; ok && i < c; i++) {
					StrAssocArray::Item item = p_full_list->at_WithoutParent(i);
					if(flags & clsfFromBeg)
						r = BIN(strncmp(item.Txt, pSubstr, substr_len) == 0);
					else
						r = ExtStrSrch(item.Txt, pSubstr, 0);
					if(r > 0) {
						if(!seriesID || (Fetch(item.Id, &sc_rec) > 0 && sc_rec.SeriesID == seriesID)) {
							if(p_list) {
								if(!p_list->addUnique(item.Id)) {
									//
									// Здесь THROW не годится из-за того, что сразу после завершения цикла
									// необходимо быстро сделать ReleaseFullList
									//
									ok = PPSetErrorSLib();
								}
							}
							else if(p_str_list) {
								p_str_list->Add(item.Id, item.Txt);
							}
						}
					}
				}
				ReleaseFullList(p_full_list);
				p_full_list = 0;
			}
			else {
				union {
					SCardTbl::Key1 k1;
					SCardTbl::Key2 k2;
				} k;
				MEMSZERO(k);
				int   sp = spFirst;
				int   idx = seriesID ? 2 : 1;
				BExtQuery q(P_Tbl, 2);
				q.select(P_Tbl->ID, P_Tbl->SeriesID, P_Tbl->Code, 0L);
				if(seriesID) {
					q.where(P_Tbl->SeriesID == seriesID);
					k.k2.SeriesID = seriesID;
					if(flags & clsfFromBeg) {
						STRNSCPY(k.k2.Code, pSubstr);
					}
					sp = spGe;
				}
				else {
					if(flags & clsfFromBeg) {
						STRNSCPY(k.k1.Code, pSubstr);
						sp = spGe;
					}
				}
				for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
					if(flags & clsfFromBeg) {
						r = BIN(strncmp(P_Tbl->data.Code, pSubstr, substr_len) == 0);
					}
					else {
						r = ExtStrSrch(P_Tbl->data.Code, pSubstr, 0);
					}
					if(r > 0) {
						if(p_list) {
							THROW_SL(p_list->addUnique(P_Tbl->data.ID));
						}
						else if(p_str_list) {
							THROW_SL(p_str_list->Add(P_Tbl->data.ID, P_Tbl->data.Code));
						}
					}
					else if(flags & clsfFromBeg)
						break;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::GetListBySubstring(const char * pSubstr, PPID seriesID, StrAssocArray * pList, int fromBegStr)
{
	long   flags = clsfStrList;
	if(fromBegStr)
		flags |= clsfFromBeg;
	int    ok = Helper_GetListBySubstring(pSubstr, seriesID, pList, flags);
	CALLPTRMEMB(pList, SortByText());
	return ok;
}

int PPObjSCard::UpdateBySeries(PPID seriesID, int use_ta)
{
	int    ok = -1;
	PPSCardSerPacket scs_pack;
	PPObjSCardSeries scs_obj;
	if(scs_obj.GetPacket(seriesID, &scs_pack) > 0) {
		SCardTbl::Key2 k2;
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k2);
		k2.SeriesID = scs_pack.Rec.ID;
		while(P_Tbl->search(2, &k2, spGt) && k2.SeriesID == scs_pack.Rec.ID) {
			SCardTbl::Rec rec;
			THROW_DB(P_Tbl->rereadForUpdate(2, &k2));
			P_Tbl->CopyBufTo(&rec);
			const long  prev_pdis = rec.PDis;
			const LDATE prev_expiry = rec.Expiry;
			if(SetInheritance(&scs_pack, &rec) > 0) {
				THROW_DB(P_Tbl->updateRecBuf(&rec)); // @sfu
				DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, rec.ID, 0, 0);
				if(rec.PDis != prev_pdis)
					DS.LogAction(PPACN_SCARDDISUPD, PPOBJ_SCARD, rec.ID, prev_pdis, 0);
				if(rec.Expiry != prev_expiry)
					DS.LogAction(PPACN_SCARDEXPRYUPD, PPOBJ_SCARD, rec.ID, static_cast<long>(prev_expiry), 0);
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::CheckUniq()
{
	int    ok = 1;
	SString prev_code;
	SCardTbl::Key1 k;
	BExtQuery q(P_Tbl, 1);
	MEMSZERO(k);
	q.select(P_Tbl->Code, 0L);
	for(q.initIteration(false, &k, spGe); ok && q.nextIteration() > 0;) {
		if(prev_code.Len() && prev_code.CmpNC(P_Tbl->data.Code) == 0)
			ok = 0;
		prev_code.CopyFrom(P_Tbl->data.Code);
	}
	return ok;
}

int PPObjSCard::CheckExpiredBillDebt(PPID scardID)
{
	int    ok = 1;
	if(scardID) {
		BillTbl::Key6 k6;
		MEMSZERO(k6);
		k6.SCardID = scardID;
		BillCore * t = BillObj->P_Tbl;
		if(t->search(6, &k6, spGe) && t->data.SCardID == scardID) {
			const LDATE _cd = getcurdate_();
			double matdebt = 0.0;
			SString temp_buf;
			PayPlanArray payplan;
			BExtQuery q(t, 6);
			q.select(t->ID, t->Amount, t->Flags, t->CurID, 0L).where(t->SCardID == scardID);
			for(q.initIteration(false, &k6, spGe); ok && q.nextIteration() > 0;) {
				if(t->data.Flags & BILLF_NEEDPAYMENT) {
					const  PPID bill_id = t->data.ID;
					const  double amount = t->data.Amount;
					double payment = 0.0;
					t->GetAmount(bill_id, PPAMT_PAYMENT, t->data.CurID, &payment);
					if((amount - payment) > 1.0) {
						t->GetPayPlan(bill_id, &payplan);
						LDATE last_dt = ZERODATE;
						if(payplan.GetLast(&last_dt, 0, 0) > 0 && last_dt < _cd) {
							BillTbl::Rec bill_rec;
							temp_buf.Z();
							if(t->Search(bill_id, &bill_rec) > 0) {
								PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddSCard, temp_buf);
							}
							PPSetError(PPERR_SCARDHASMATDEBT, temp_buf);
							ok = 0;
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPObjSCard::NotifyAboutRecentOps(const LDATETIME & rSince)
{
	int    ok = -1;
	//
	// Из-за того, что таблица SCardOp не имеет индекса по времени (только с префиксом карты)
	// придется идти сложным путем: найти все системные события по картам с момента rSince
	// и, получив идентификаторы соответствующих карт, перебрать операции по каждой из них.
	//
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj) {
		SString temp_buf;
		SString phone_buf;
		SString msg_buf;
		PPIDArray sc_id_list;
		PPIDArray acn_list;
		acn_list.addzlist(PPACN_OBJUPD, PPACN_SCARDBONUSCHARGE, 0);
		p_sj->GetObjListByEventSince(PPOBJ_SCARD, &acn_list, rSince, sc_id_list, 0);
		if(sc_id_list.getCount()) {
			TSVector <SCardCore::UpdateRestNotifyEntry> urne_list;
			sc_id_list.sortAndUndup();
			for(uint i = 0; i < sc_id_list.getCount(); i++) {
				const  PPID sc_id = sc_id_list.get(i);
				PPSCardPacket sc_pack;
				//
				// Дабы не тратить время на перебор операций по карте, у которой нет телефона, сразу проверим чтобы таковой был.
				//
				if(GetPacket(sc_id, &sc_pack) > 0) {
					if(sc_pack.GetExtStrData(sc_pack.extssPhone, temp_buf) > 0 && temp_buf.NotEmptyS() && FormatPhone(temp_buf, phone_buf, msg_buf)) {
						LDATETIME dtm = rSince;
						dtm.t.addhs(-100);
						SCardOpTbl::Rec op_rec;
						while(P_Tbl->EnumOpByCard(sc_id, &dtm, &op_rec) > 0) {
							if(!(op_rec.Flags & SCARDOPF_NOTIFYSENDED)) {
								SCardCore::UpdateRestNotifyEntry new_entry;
								new_entry.SCardID = op_rec.SCardID;
								new_entry.OpDtm.Set(op_rec.Dt, op_rec.Tm);
								new_entry.NewRest = op_rec.Rest;
								new_entry.PrevRest = op_rec.Rest - op_rec.Amount;
								urne_list.insert(&new_entry);
							}
						}
					}
				}
			}
			FinishSCardUpdNotifyList(urne_list);
		}
	}
	return ok;
}

int PPObjSCard::FinishSCardUpdNotifyList(const TSVector <SCardCore::UpdateRestNotifyEntry> & rList)
{
	int    ok = -1;
	if(rList.getCount()) {
		SString fmt_buf, msg_buf;
		SString temp_buf;
		PPPersonConfig psn_cfg;
		PPObjPerson::ReadConfig(&psn_cfg);
		if(!psn_cfg.SmsProhibitedTr.IsZero() && psn_cfg.SmsProhibitedTr.Check(getcurtime_())) {
			PPLoadText(PPTXT_SMSDECLINEDDUETIMERANGE, fmt_buf);
			temp_buf.Z().Cat(psn_cfg.SmsProhibitedTr.low, TIMF_HM).Dot().Dot().Cat(psn_cfg.SmsProhibitedTr.upp, TIMF_HM);
			msg_buf.Printf(fmt_buf, temp_buf.cptr());
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
		}
		else {
			PPAlbatrossConfig  albtr_cfg;
			SmsClient sms_cli(0);
			if(PPAlbatrosCfgMngr::Get(&albtr_cfg) && sms_cli.SmsInit_(albtr_cfg.Hdr.SmsAccID, "UHTT") > 0) {
				Reference * p_ref = PPRef;
				PPObjBill * p_bobj = BillObj;
				GtaJournalCore * p_gtaj = DS.GetGtaJ();
				SString phone_buf;
				SString status_buf;
				StrAssocArray gua_to_prefix_list;
				UintHashTable gua_byprefix_list;
				p_ref->Ot.GetObjectList(PPOBJ_GLOBALUSERACC, PPTAG_GUA_SCARDPREFIX, gua_byprefix_list);
				for(ulong iter_gua_id = 0; gua_byprefix_list.Enum(&iter_gua_id);) {
					if(p_ref->Ot.GetTagStr(PPOBJ_GLOBALUSERACC, (long)iter_gua_id, PPTAG_GUA_SCARDPREFIX, temp_buf) > 0) {
						gua_to_prefix_list.Add((long)iter_gua_id, temp_buf, 0);
					}
				}
				for(uint i = 0; i < rList.getCount(); i++) {
					const SCardCore::UpdateRestNotifyEntry & r_entry = rList.at(i);
					PPSCardPacket sc_pack;
					if(GetPacket(r_entry.SCardID, &sc_pack) > 0) {
						SCardOpTbl::Rec op_rec;
						int   do_notify = 0; // 1 - draw, 2 - withdraw
						double _withdraw = R2(r_entry.NewRest - r_entry.PrevRest);
						SETMAX(_withdraw, 0.0);
						double _draw = R2(r_entry.PrevRest - r_entry.NewRest);
						SETMAX(_draw, 0.0);
						if(_draw > 0.0 && sc_pack.Rec.Flags & SCRDF_NOTIFYDRAW)
							do_notify = 1;
						else if(_withdraw > 0.0 && sc_pack.Rec.Flags & SCRDF_NOTIFYWITHDRAW)
							do_notify = 2;
						if(do_notify && sc_pack.GetExtStrData(sc_pack.extssPhone, temp_buf) > 0 && temp_buf.NotEmptyS() && FormatPhone(temp_buf, phone_buf, msg_buf)) {
							PPID   gua_id = 0;
							double amount = 0.0;
							if(do_notify == 1) {
								PPLoadText(PPTXT_SCARDDRAW, fmt_buf);
								amount = _draw;
							}
							else if(do_notify == 2) {
								PPLoadText(PPTXT_SCARDWITHDRAW, fmt_buf);
								amount = _withdraw;
							}
							temp_buf = sc_pack.Rec.Code;
							for(uint i = 0; i < gua_to_prefix_list.getCount(); i++) {
								StrAssocArray::Item gua_to_prefix_item = gua_to_prefix_list.at_WithoutParent(i);
								if(temp_buf.HasPrefixNC(gua_to_prefix_item.Txt)) {
									gua_id = gua_to_prefix_item.Id;
									temp_buf.ShiftLeft(sstrlen(gua_to_prefix_item.Txt));
									break;
								}
							}
							if(temp_buf.NotEmptyS()) {
								int    skip = 0;
								if(!!r_entry.OpDtm && P_Tbl->SearchOp(r_entry.SCardID, r_entry.OpDtm.d, r_entry.OpDtm.t, &op_rec) > 0) {
									if(op_rec.Flags & SCARDOPF_NOTIFYSENDED) {
										skip = 1;
									}
								}
								if(!skip) {
									PPGta  gta_blk;
									if(gua_id && p_bobj) {
										gta_blk.GlobalUserID = gua_id;
										gta_blk.Op = GTAOP_SMSSEND;
										p_bobj->InitGta(gta_blk);
										if(gta_blk.Quot != 0.0) { // Для рассылки SMS кредит не применяется!
											if((gta_blk.SCardRest/*+ gta_blk.SCardMaxCredit*/) <= 0.0) {
												PPSetError(PPERR_GTAOVERDRAFT);
												PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
												skip = 1;
											}
										}
									}
									msg_buf.Z();
									PPFormat(fmt_buf, &msg_buf, temp_buf.cptr(), amount);
									if(sms_cli.SendSms(phone_buf, msg_buf, status_buf) > 0) {
										if(!!r_entry.OpDtm) {
											int    local_err = 0;
											if(P_Tbl->SearchOp(r_entry.SCardID, r_entry.OpDtm.d, r_entry.OpDtm.t, &op_rec) > 0) {
												PPTransaction tra(1);
												if(!tra)
													local_err = 1;
												else if(!P_Tbl->ScOp.rereadForUpdate(-1, 0))
													local_err = 1;
												else {
													op_rec.Flags |= SCARDOPF_NOTIFYSENDED;
													if(!P_Tbl->ScOp.updateRecBuf(&op_rec))
														local_err = 1;
													else if(!tra.Commit())
														local_err = 1;
												}
											}
											if(local_err)
												PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
										}
										if(gua_id && p_bobj) {
											gta_blk.Count = 1;
											gta_blk.Duration = ZEROTIME;
											gta_blk.Dtm = getcurdatetime_();
											if(p_gtaj) {
												if(!p_gtaj->CheckInOp(gta_blk, 1)) {
													PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
												}
											}
										}
									}
									else
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
								}
							}
						}
					}
				}
			}
			sms_cli.SmsRelease_();
		}
	}
	return ok;
}

int PPObjSCard::CheckRestrictions(const SCardTbl::Rec * pRec, long flags, LDATETIME dtm)
{
	int    ok = 1;
	if(pRec) {
		if(pRec->Flags & SCRDF_CLOSED) {
			if(pRec->Flags & SCRDF_NEEDACTIVATION) {
				if(pRec->Flags & SCRDF_AUTOACTIVATION) {
					ok = 2;
				}
				else {
					CALLEXCEPT_PP_S(PPERR_SCARDACTIVATIONNEEDED, pRec->Code); // @v7.7.2
				}
			}
			else {
				CALLEXCEPT_PP_S(PPERR_SCARDCLOSED, pRec->Code);
			}
		}
		THROW_PP_S(!pRec->Expiry || cmp(dtm, pRec->Expiry, MAXDAYTIMESEC) <= 0, PPERR_SCARDEXPIRED, pRec->Code); // @v8.3.8 ZEROTIME-->encodetime(23, 59, 59)
		if(!(flags & chkrfIgnoreUsageTime)) {
			THROW_PP_S(!pRec->UsageTmStart || !dtm.t || dtm.t >= pRec->UsageTmStart, PPERR_SCARDTIME, pRec->Code);
			THROW_PP_S(!pRec->UsageTmEnd  || !dtm.t || dtm.t <= pRec->UsageTmEnd,   PPERR_SCARDTIME, pRec->Code);
		}
		if(GetConfig().Flags & PPSCardConfig::fCheckBillDebt) {
			THROW(CheckExpiredBillDebt(pRec->ID));
		}
		{
			SString added_msg_buf;
			TSVector <SCardCore::OpBlock> frz_op_list;
			if(P_Tbl->GetFreezingOpList(pRec->ID, frz_op_list) > 0) {
				for(uint i = 0; i < frz_op_list.getCount(); i++) {
					const SCardCore::OpBlock & r_ob = frz_op_list.at(i);
					if(r_ob.CheckFreezingPeriod(ZERODATE)) {
						(added_msg_buf = pRec->Code).Space().Cat(r_ob.FreezingPeriod);
						THROW_PP_S(!r_ob.FreezingPeriod.CheckDate(dtm.d), PPERR_SCARDONFREEZING, added_msg_buf);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

struct _SCardTrnovrItem {
	enum {
		fDontChangeDiscount = 0x0001
	};
	PPID   SCardID;
	long   Flags;
	double BonusDbt;
	double BonusCrd;
	double DscntTrnovr;
};

int PPObjSCard::UpdateBySeriesRule2(PPID seriesID, /*int prevTrnovrPrd*/const SCardChargeRule & rRule, PPLogger * pLog, int use_ta)
{
	int    ok = 1;
	int    prd_delta = 0;
	LDATE  prev_prd_beg = ZERODATE;
	SString fmt_buf;
	SString msg_buf;
	SString scard_name;
	SString temp_buf;
	SString temp_buf2;
	//char   prd_txt[32];
	TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
	SCardTbl::Key2 k2;
	PPUserFuncProfiler ufp(PPUPRF_SCARDUPDBYRULE);
	double ufp_factor = 0.0;
	double ufp_factor2 = 0.0;
	PPSCardSerPacket pack;
	PPObjSCardSeries obj_scs;
	if(obj_scs.GetPacket(seriesID, &pack) > 0) {
		TSArray <_SCardTrnovrItem> sct_list;
		enum {
			_cfBonusRule    = 0x0001,
			_cfDiscountRule = 0x0002
		};
		const  LDATE _cur_date = getcurdate_();
		long   case_flags = 0;
		int32  bonus_period_idx = 0;
		DateRange bonus_period;
		DateRange dscnt_period;
		bonus_period.Z();
		dscnt_period.Z();
		if(pack.Rec.GetType() == scstBonus && pack.BonusRule.getCount()) {
			if(rRule.Period == SCARDSER_AUTODIS_ARBITRARYPRD) {
				bonus_period = rRule.Ap;
				bonus_period.Actualize(ZERODATE);
			}
			else if(oneof6(pack.BonusRule.TrnovrPeriod, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL)) {
				THROW_SL(bonus_period.SetPeriod(_cur_date, pack.BonusRule.TrnovrPeriod));
				if(rRule.Period == SCARDSER_AUTODIS_PREVPRD) {
					THROW_SL(bonus_period.SetPeriod(plusdate(bonus_period.low, -1), pack.BonusRule.TrnovrPeriod));
				}
			}
			/*else {
				bonus_period = rRule.Ap;
				bonus_period.Actualize(ZERODATE);
			}*/
			/* @v11.3.5 
			if(pack.BonusRule.TrnovrPeriod) {
				THROW_SL(bonus_period.SetPeriod(_cur_date, pack.BonusRule.TrnovrPeriod));
				if(prevTrnovrPrd) {
					THROW_SL(bonus_period.SetPeriod(plusdate(bonus_period.low, -1), pack.BonusRule.TrnovrPeriod));
				}
				Quotation2Core::PeriodToPeriodIdx(&bonus_period, &bonus_period_idx);
			}
			else
				bonus_period.Set(ZERODATE, MAXDATEVALID);
			*/
			Quotation2Core::PeriodToPeriodIdx(&bonus_period, &bonus_period_idx); // @v11.3.9 @fix
			case_flags |= _cfBonusRule;
		}
		if(pack.Rule.getCount()) {
			if(rRule.Period == SCARDSER_AUTODIS_ARBITRARYPRD) {
				dscnt_period = rRule.Ap;
				dscnt_period.Actualize(ZERODATE);
			}
			else if(oneof6(pack.Rule.TrnovrPeriod, PRD_DAY, PRD_WEEK, PRD_MONTH, PRD_QUART, PRD_SEMIAN, PRD_ANNUAL)) {
				THROW_SL(dscnt_period.SetPeriod(_cur_date, pack.Rule.TrnovrPeriod));
				if(rRule.Period == SCARDSER_AUTODIS_PREVPRD) {
					THROW_SL(dscnt_period.SetPeriod(plusdate(dscnt_period.low, -1), pack.Rule.TrnovrPeriod));
				}
			}
			/*else {
				dscnt_period = rRule.Ap;
				dscnt_period.Actualize(ZERODATE);
			}*/
			/* @v11.3.5 
			if(pack.Rule.TrnovrPeriod) {
				THROW_SL(dscnt_period.SetPeriod(_cur_date, pack.Rule.TrnovrPeriod));
				if(prevTrnovrPrd) {
					THROW_SL(dscnt_period.SetPeriod(plusdate(dscnt_period.low, -1), pack.Rule.TrnovrPeriod));
				}
			}
			*/
			case_flags |= _cfDiscountRule;
		}
		if(case_flags) {
			TSArray <SCardTbl::Rec> scr_list;
			MEMSZERO(k2);
			k2.SeriesID = pack.Rec.ID;
			BExtQuery q(P_Tbl, 2);
			q.selectAll().where(P_Tbl->SeriesID == pack.Rec.ID);
			for(q.initIteration(false, &k2, spGt); q.nextIteration() > 0;) {
				SCardTbl::Rec rec;
				P_Tbl->CopyBufTo(&rec);
				THROW_SL(scr_list.insert(&rec));
			}
			const uint scrlc = scr_list.getCount();
			const  PPID bonus_charge_grp_id = pack.Rec.BonusChrgGrpID;
			for(uint i = 0; i < scrlc; i++) {
				const SCardTbl::Rec & r_sc_rec = scr_list.at(i);
				_SCardTrnovrItem item;
				MEMSZERO(item);
				item.SCardID = r_sc_rec.ID;
				if(case_flags & _cfBonusRule) {
					double dbt = 0.0, crd = 0.0;
					THROW(GetTurnover(r_sc_rec, PPObjSCard::gtalgForBonus, bonus_period, bonus_charge_grp_id, &dbt, &crd));
					item.BonusDbt = dbt;
					item.BonusCrd = crd;
					ufp_factor += 1.0;
				}
				if(case_flags & _cfDiscountRule) {
					if(r_sc_rec.Flags & SCRDF_INHERITED)
						item.Flags |= _SCardTrnovrItem::fDontChangeDiscount;
					else {
						double trnovr = 0.0;
						if(pack.Rule.TrnovrPeriod) {
							THROW(GetTurnover(r_sc_rec, PPObjSCard::gtalgDefault, dscnt_period, bonus_charge_grp_id, 0, &trnovr));
							ufp_factor += 1.0;
						}
						else
							trnovr = r_sc_rec.Turnover;
						item.DscntTrnovr = trnovr;
					}
				}
				THROW_SL(sct_list.insert(&item));
			}
		}
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pack.Rec.GetType() == scstBonus && pack.BonusRule.getCount()) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			PPIDArray acn_list;
			acn_list.add(PPACN_SCARDBONUSCHARGE);
			if(pLog) {
				PPLoadText(PPTXT_LOG_SCBONUSCHARGE, fmt_buf);
				SString & r_period_buf = SLS.AcquireRvlStr();
				r_period_buf.Cat(bonus_period, true);
				pLog->Log(msg_buf.Printf(fmt_buf, pack.Rec.Name, r_period_buf.cptr()));
			}
			for(uint i = 0; i < sct_list.getCount(); i++) {
				const _SCardTrnovrItem & r_sct_item = sct_list.at(i);
				const TrnovrRngDis * p_item = 0;
				if(r_sct_item.BonusCrd > 0.0 && (p_item = pack.BonusRule.SearchItem(r_sct_item.BonusCrd)) != 0) {
					const double bonus_amount = R2((p_item->Flags & TrnovrRngDis::fBonusAbsoluteValue) ? p_item->Value : (r_sct_item.BonusCrd * (p_item->Value / 100.0)));
					if(bonus_amount > 0.0) {
						SCardTbl::Rec sc_rec;
						if(P_Tbl->Search(r_sct_item.SCardID, &sc_rec) > 0) {
							SCardCore::OpBlock op_blk;
							if(pack.BonusRule.TrnovrPeriod) {
								int    skip = 0;
								if(p_sj) {
									LDATETIME ev_dtm;
									SysJournalTbl::Rec ev_rec;
									if(p_sj->GetLastObjEvent(PPOBJ_SCARD, r_sct_item.SCardID, &acn_list, &ev_dtm, &ev_rec) > 0) {
										if(ev_rec.Extra == bonus_period_idx) {
											if(pLog) {
												PPLoadText(PPTXT_LOG_SCBONUSCHARGEDUP, fmt_buf);
												pLog->Log(msg_buf.Printf(fmt_buf, sc_rec.Code, temp_buf.Z().Cat(bonus_period, 1).cptr()));
											}
											skip = 1;
										}
									}
								}
								if(!skip) {
									op_blk.SCardID = sc_rec.ID;
									op_blk.Dtm = getcurdatetime_();
									op_blk.Amount = bonus_amount;
									THROW(P_Tbl->PutOpBlk(op_blk, &urn_list, 0));
									DS.LogAction(PPACN_SCARDBONUSCHARGE, PPOBJ_SCARD, sc_rec.ID, bonus_period_idx, 0);
									ufp_factor2 += 1.0;
								}
							}
							else {
								double rest = 0.0;
								THROW(P_Tbl->GetRest(sc_rec.ID, ZERODATE, &rest));
								if(bonus_amount > rest) {
									op_blk.SCardID = sc_rec.ID;
									op_blk.Dtm = getcurdatetime_();
									op_blk.Amount = bonus_amount - rest;
									THROW(P_Tbl->PutOpBlk(op_blk, &urn_list, 0));
									DS.LogAction(PPACN_SCARDBONUSCHARGE, PPOBJ_SCARD, sc_rec.ID, bonus_period_idx, 0);
									ufp_factor2 += 1.0;
								}
							}
						}
					}
				}
			}
		}
		if(pack.Rule.getCount()) {
			MEMSZERO(k2);
			k2.SeriesID = pack.Rec.ID;
			if(pLog) {
				PPLoadText(PPTXT_LOG_SCDISRECALC, fmt_buf);
				SString & r_period_buf = SLS.AcquireRvlStr();
				r_period_buf.Cat(dscnt_period, true);
				pLog->Log(msg_buf.Printf(fmt_buf, pack.Rec.Name, r_period_buf.cptr()));
			}
			for(uint i = 0; i < sct_list.getCount(); i++) {
				const _SCardTrnovrItem & r_sct_item = sct_list.at(i);
				TrnovrRngDis entry;
				if(!(r_sct_item.Flags & _SCardTrnovrItem::fDontChangeDiscount) && pack.GetDisByRule(r_sct_item.DscntTrnovr, entry) > 0) {
					SCardTbl::Rec sc_rec;
					if(P_Tbl->Search(r_sct_item.SCardID, &sc_rec) > 0) {
						const long prev_pdis = sc_rec.PDis;
						double new_pdis = 0.0;
						const int  _gr = entry.GetResult(fdiv100i(prev_pdis), &new_pdis);
						const long lpdis = (long)(new_pdis * 100.0);
						const bool upd_dis = (sc_rec.PDis != lpdis);
						const bool upd_ser = (entry.SeriesID && entry.SeriesID != sc_rec.SeriesID);
						if(upd_dis || upd_ser) {
							int    skip = 0;
							PPSCardSeries mov_ser_rec;
							DBRowId _dbpos;
							THROW_DB(P_Tbl->getPosition(&_dbpos));
							(scard_name = pack.Rec.Name).CatChar('-').Cat(sc_rec.Code);
							if(upd_ser) {
								if(obj_scs.Search(entry.SeriesID, &mov_ser_rec) > 0) {
									if(mov_ser_rec.GetType() == pack.Rec.GetType()) {
										sc_rec.SeriesID = entry.SeriesID;
									}
									else {
										PPLoadText(PPTXT_LOG_UNCOMPSCARDSER, fmt_buf);
										msg_buf.Printf(fmt_buf, scard_name.cptr(), (const char *)mov_ser_rec.Name);
										if(pLog)
											pLog->Log(msg_buf);
										else
											PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
										skip = 1;
									}
								}
								else {
									PPLoadText(PPTXT_LOG_INVSCARDSER, fmt_buf);
									ideqvalstr(entry.SeriesID, temp_buf.Z());
									PPGetLastErrorMessage(1, temp_buf2);
									msg_buf.Printf(fmt_buf, temp_buf.cptr(), scard_name.cptr(), temp_buf2.cptr());
									if(pLog)
										pLog->Log(msg_buf);
									else
										PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
									skip = 1;
								}
							}
							if(!skip) {
								sc_rec.PDis = lpdis;
								THROW_DB(P_Tbl->getDirectForUpdate(0, 0, _dbpos));
								THROW_DB(P_Tbl->updateRecBuf(&sc_rec)); // @sfu
								DS.LogAction(PPACN_OBJUPD, PPOBJ_SCARD, sc_rec.ID, 0, 0);
								ufp_factor2 += 1.0;
								if(upd_dis)
									DS.LogAction(PPACN_SCARDDISUPD, PPOBJ_SCARD, sc_rec.ID, prev_pdis, 0);
								//
								// Вывод информации в текстовый журнал
								//
								PPLoadText(PPTXT_LOG_SCARDRULEAPPLY, fmt_buf);
								msg_buf.Printf(fmt_buf, scard_name.cptr(), r_sct_item.DscntTrnovr).Space();
								if(upd_dis) {
									PPLoadText(PPTXT_LOG_ADD_SCARDDISUPD, fmt_buf);
									msg_buf.Cat(temp_buf.Printf(fmt_buf, fdiv100i(prev_pdis), new_pdis));
								}
								if(upd_ser) {
									PPLoadText(PPTXT_LOG_ADD_SCARDMOVED, fmt_buf);
									if(upd_dis)
										msg_buf.CatDiv(';', 2);
									msg_buf.Cat(temp_buf.Printf(fmt_buf, (const char *)mov_ser_rec.Name));
								}
								if(pLog)
									pLog->Log(msg_buf);
								else
									PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
							}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	FinishSCardUpdNotifyList(urn_list);
	ufp.SetFactor(0, (double)ufp_factor);
	ufp.SetFactor(1, (double)ufp_factor2);
	ufp.Commit();
	CATCHZOK
	return ok;
}

// @Muxa @v7.3.9
SString & PPObjSCard::CalcSCardHash(const char * pNumber, SString & rHash)
{
	rHash.Z();
#define SCARD_HASH_LEN   4
	SCRC32  crc32;
	ulong   crc;
	SString temp_buf;
	char    buf[128];
	if(!isempty(pNumber)) {
		STRNSCPY(buf, pNumber);
		crc = crc32.Calc(0, buf, sstrlen(buf));
		rHash.Cat(crc >> 7).Trim(SCARD_HASH_LEN);
		if(rHash.Len() < SCARD_HASH_LEN)
			rHash.PadLeft(SCARD_HASH_LEN - rHash.Len(), '0');
	}
#undef SCARD_HASH_LEN
	return rHash;
}
// }
int PPObjSCard::CreateTurnoverList(const DateRange * pPeriod, RAssocArray * pList)
{
	return (P_CcTbl->CreateSCardsTurnoverList(pPeriod, pList) && BillObj->P_Tbl->CreateSCardsTurnoverList(pPeriod, pList));
}

int PPObjSCard::GetTurnover(const SCardTbl::Rec & rRec, int alg, const DateRange & rPeriod, PPID restrGoodsGrpID, double * pDebit, double * pCredit)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	double dbt = 0.0, crd = 0.0;
	double bill_dbt = 0.0, bill_crd = 0.0;
	DateRange period;
	PPObjSCardSeries scs_obj;
	PPSCardSeries scs_rec;
	MEMSZERO(scs_rec);
	scs_obj.Fetch(rRec.SeriesID, &scs_rec);
	PROFILE_START
	if(alg == gtalgDefault) {
		if(scs_rec.GetType() == scstCredit) {
			LDATETIME dtm;
			dtm.Set(rPeriod.low, ZEROTIME);
			SCardOpTbl::Rec scop_rec;
			while(P_Tbl->EnumOpByCard(rRec.ID, &dtm, &scop_rec) > 0 && (!rPeriod.upp || dtm.d <= rPeriod.upp)) {
				if(scop_rec.Amount > 0.0)
					dbt += scop_rec.Amount;
				else
					crd += scop_rec.Amount;
			}
		}
		else {
			period.low = MAX(rPeriod.low, rRec.Dt);
			period.upp = rPeriod.upp;
			P_CcTbl->GetTrnovrBySCard(rRec.ID, alg, &period, /*restrGoodsGrpID*/0, &dbt, &crd);
		}
		p_bobj->P_Tbl->GetTrnovrBySCard(rRec.ID, &rPeriod, /*restrGoodsGrpID*/0, &bill_dbt, &bill_crd);
		crd += (bill_crd - bill_dbt);
	}
	else if(alg == gtalgByCheck) {
		period.low = MAX(rPeriod.low, rRec.Dt);
		period.upp = rPeriod.upp;
		P_CcTbl->GetTrnovrBySCard(rRec.ID, alg, &period, /*restrGoodsGrpID*/0, &dbt, &crd);
	}
	else if(alg == gtalgByBill) {
		p_bobj->P_Tbl->GetTrnovrBySCard(rRec.ID, &rPeriod, /*restrGoodsGrpID*/0, &bill_dbt, &bill_crd);
		crd += (bill_crd - bill_dbt);
	}
	else if(alg == gtalgForBonus) {
		period.low = MAX(rPeriod.low, rRec.Dt);
		period.upp = rPeriod.upp;
		P_CcTbl->GetTrnovrBySCard(rRec.ID, alg, &period, restrGoodsGrpID, &dbt, &crd);
		p_bobj->P_Tbl->GetTrnovrBySCard(rRec.ID, &rPeriod, restrGoodsGrpID, &bill_dbt, &bill_crd);
		crd += (bill_crd - bill_dbt);
	}
	else if(alg == gtalgByOp) {
		LDATETIME dtm;
		dtm.Set(rPeriod.low, ZEROTIME);
		SCardOpTbl::Rec scop_rec;
		while(P_Tbl->EnumOpByCard(rRec.ID, &dtm, &scop_rec) > 0 && (!rPeriod.upp || dtm.d <= rPeriod.upp)) {
			if(scop_rec.Amount > 0.0)
				dbt += scop_rec.Amount;
			else
				crd += scop_rec.Amount;
		}
	}
	PROFILE_END
	ASSIGN_PTR(pDebit, dbt);
	ASSIGN_PTR(pCredit, crd);
	return ok;
}

int PPObjSCard::GetTurnover(PPID cardID, int alg, const DateRange & rPeriod, PPID restrGoodsGrpID, double * pDebit, double * pCredit)
{
	int    ok = 1;
	SCardTbl::Rec rec;
	if(Search(cardID, &rec) > 0)
		ok = GetTurnover(rec, alg, rPeriod, restrGoodsGrpID, pDebit, pCredit);
	else {
		ASSIGN_PTR(pDebit, 0.0);
		ASSIGN_PTR(pCredit, 0.0);
		ok = -1;
	}
	return ok;
}

int PPObjSCard::PutUhttOp(PPID cardID, double amount)
{
	int    ok = -1;
	amount = R2(amount);
	if(!feqeps(amount, 0.0, 1e-2)) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		SCardTbl::Rec sc_rec;
		THROW(Search(cardID, &sc_rec) > 0);
		if(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) {
			const int scst = scs_rec.GetType();
			if(oneof2(scst, scstCredit, scstBonus) && scs_rec.Flags & SCRDSF_UHTTSYNC) {
				PPUhttClient uhtt_cli;
				UhttSCardPacket scp;
				double uhtt_rest = 0.0;
				THROW(uhtt_cli.Auth());
				THROW(uhtt_cli.GetSCardByNumber(sc_rec.Code, scp));
				THROW(uhtt_cli.GetSCardRest(scp.Code, 0, uhtt_rest));
				if(amount > 0.0) {
					THROW(uhtt_cli.DepositSCardAmount(scp.Code, amount));
					ok = 1;
				}
				else if(amount < 0.0) {
					THROW_PP_S(fabs(amount) <= (uhtt_rest + scp.Overdraft), PPERR_UHTTSCARDREST, sc_rec.Code);
					THROW(uhtt_cli.WithdrawSCardAmount(scp.Code, fabs(amount)));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::ActivateRec(SCardTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec) {
		if(pRec->Flags & SCRDF_NEEDACTIVATION && pRec->Flags & SCRDF_CLOSED) {
			pRec->Flags &= ~(SCRDF_NEEDACTIVATION|SCRDF_CLOSED|SCRDF_AUTOACTIVATION);
			if(pRec->PeriodTerm) {
				const LDATE cd = getcurdate_();
				LDATE  dt = cd;
				plusperiod(&dt, pRec->PeriodTerm, ((pRec->PeriodCount > 0) ? pRec->PeriodCount : 1), 0);
				pRec->Expiry = (dt > cd) ? plusdate(dt, -1) : dt;
			}
			ok = 1;
		}
	}
	return ok;
}

//int PPObjSCard::SetInheritance(const PPSCardSeries * pSerRec, SCardTbl::Rec * pRec)
int PPObjSCard::SetInheritance(const PPSCardSerPacket * pScsPack, SCardTbl::Rec * pRec)
{
	int    ok = -1;
	if(pRec && pRec->Flags & SCRDF_INHERITED) {
		PPSCardSerPacket subst_scs_pack;
		if(!pScsPack) {
			PPObjSCardSeries scs_obj;
			THROW(scs_obj.GetPacket(pRec->SeriesID, &subst_scs_pack) > 0);
			pScsPack = &subst_scs_pack;
		}
		assert(pScsPack != 0);
		if(pRec->PDis != pScsPack->Rec.PDis) {
			pRec->PDis = pScsPack->Rec.PDis;
			ok = 1;
		}
		if(pRec->MaxCredit != pScsPack->Rec.MaxCredit) {
			pRec->MaxCredit = pScsPack->Rec.MaxCredit;
			ok = 1;
		}
		if(pRec->Dt != pScsPack->Rec.Issue) {
			pRec->Dt = pScsPack->Rec.Issue;
			ok = 1;
		}
		if(pRec->Expiry != pScsPack->Rec.Expiry) {
			pRec->Expiry = pScsPack->Rec.Expiry;
			ok = 1;
		}
		if(pRec->UsageTmStart != pScsPack->Eb.UsageTmStart && pScsPack->Eb.UsageTmStart) {
			pRec->UsageTmStart = pScsPack->Eb.UsageTmStart;
			ok = 1;
		}
		if(pRec->UsageTmEnd != pScsPack->Eb.UsageTmEnd && pScsPack->Eb.UsageTmEnd) {
			pRec->UsageTmEnd = pScsPack->Eb.UsageTmEnd;
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::Create_(PPID * pID, PPID seriesID, PPID ownerID, const SCardTbl::Rec * pPatternRec, SString & rNumber, const char * pPassword, long flags, int use_ta)
{
	int    ok = 1;
	SString number;
	PPObjSCardSeries scs_obj;
	PPSCardSerPacket scs_pack;
	THROW(CheckRights(PPR_INS));
	{
		//
		// Определяем серию карт и проверяем ее.
		//
		int    is_def_series = 0;
		if(!seriesID) {
			const PPSCardConfig & r_cfg = GetConfig();
			if(flags & cdfCreditCard) {
				THROW_PP(seriesID = r_cfg.DefCreditSerID, PPERR_NODEFCREDITSCS);
			}
			else {
				THROW_PP(seriesID = r_cfg.DefSerID, PPERR_NODEFSCS);
			}
			is_def_series = 1;
		}
		THROW(scs_obj.GetPacket(seriesID, &scs_pack) > 0);
		if(flags & cdfCreditCard) {
			THROW_PP(scs_pack.Rec.Flags & SCRDSF_CREDIT, PPERR_CREDITCARDSERNEEDED);
		}
		else {
			THROW_PP(!(scs_pack.Rec.Flags & SCRDSF_CREDIT), PPERR_NONCREDITCARDSERNEEDED);
		}
	}
	{
		//
		// Проверяем номер карты.
		//
		number = rNumber;
		if(number.NotEmptyS()) {
			SCardTbl::Rec temp_rec;
			int r = P_Tbl->SearchCode(seriesID, number, &temp_rec);
			THROW(r);
			THROW_PP_S(r < 0, PPERR_DUPLSCARDFOUND, number);
		}
		else {
			THROW_PP_S(scs_pack.Eb.CodeTempl[0] != 0, PPERR_UNDEFSCSCODETEMPL, scs_pack.Rec.Name);
			THROW_PP_S(P_Tbl->MakeCodeByTemplate(scs_pack.Rec.ID, scs_pack.Eb.CodeTempl, number) > 0, PPERR_UNABLEMKSCCODEBYTEMPL, scs_pack.Eb.CodeTempl);
		}
	}
	{
		PPSCardPacket pack;
		number.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
		pack.Rec.SeriesID = seriesID;
		pack.Rec.PersonID = ownerID;
		pack.Rec.Dt = getcurdate_();
		if(pPatternRec) {
			pack.Rec.PDis = pPatternRec->PDis;
			pack.Rec.MaxCredit = pPatternRec->MaxCredit;
			pack.Rec.Expiry = pPatternRec->Expiry;
			pack.Rec.UsageTmStart = pPatternRec->UsageTmStart;
			pack.Rec.UsageTmEnd = pPatternRec->UsageTmEnd;
			pack.Rec.AutoGoodsID = pPatternRec->AutoGoodsID;
		}
		else {
			pack.Rec.PDis      = scs_pack.Rec.PDis;
			pack.Rec.MaxCredit = scs_pack.Rec.MaxCredit;
			pack.Rec.Expiry    = scs_pack.Rec.Expiry;
			pack.Rec.Flags    |= SCRDF_INHERITED;
		}
		// @v11.9.10 {
		if(!isempty(pPassword)) {
			pack.PutExtStrData(PPSCardPacket::extssPassword, pPassword);
		}
		// } @v11.9.10 
		THROW(PutPacket(pID, &pack, use_ta));
		rNumber = number;
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::AutoFill(PPID seriesID, int use_ta)
{
	int    ok = -1; // sample template: L01(290%09[1..6])^
	SString pattern;
	PPObjSCardSeries scs_obj;
	PPSCardSerPacket scs_pack;
	PPInputStringDialogParam isd_param;
	THROW(CheckRights(PPR_INS));
	THROW(scs_obj.GetPacket(seriesID, &scs_pack) > 0);
	pattern = scs_pack.Eb.CodeTempl;
	PPLoadText(PPTXT_SCARDCODETEMPL, isd_param.Title);
	isd_param.P_Wse = new TextHistorySelExtra("scardcodetemplate-common");
	if(InputStringDialog(isd_param, pattern) > 0) {
		PPWaitStart();
		THROW(P_Tbl->AutoFill(scs_pack, pattern, use_ta));
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjSCard::VerifyOwner(PPSCardPacket & rScPack, PPID posNodeID, int updateImmediately)
{
	int    ok = -1;
	SCardSpecialTreatment * p_spctrt = 0;
	SString phone;
	rScPack.GetExtStrData(PPSCardPacket::extssPhone, phone);
	if(phone.NotEmpty()) {
		uint   check_code = 0;
		SCardSpecialTreatment::CardBlock cb;
		THROW(!updateImmediately || CheckRights(PPR_MOD));
		{
			p_spctrt = SCardSpecialTreatment::CreateInstance(cb.SpecialTreatment);
			if(p_spctrt && p_spctrt->GetCapability() & SCardSpecialTreatment::capfVerifyPhone) {
				if(SCardSpecialTreatment::InitSpecialCardBlock(rScPack.Rec.ID, posNodeID, cb) > 0) {
					int r = p_spctrt->VerifyOwner(&cb);
					if(r > 0) {
						rScPack.Rec.Flags |= SCRDF_OWNERVERIFIED;
						if(updateImmediately && rScPack.Rec.ID) {
							THROW(SetFlags(rScPack.Rec.ID, SCRDF_OWNERVERIFIED, 1));
						}
						ok = 1;
					}
					else if(!r)
						ok = 0;
				}
			}
			if(ok < 0) {
				if(VerifyPhoneNumberBySms(phone, rScPack.Rec.Code, &check_code, 0) > 0) {
					if(check_code) {
						rScPack.Rec.Flags |= SCRDF_OWNERVERIFIED;
						if(updateImmediately && rScPack.Rec.ID) {
							THROW(SetFlags(rScPack.Rec.ID, SCRDF_OWNERVERIFIED, 1));
						}
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_spctrt;
	return ok;
}

class SCardDialog : public TDialog {
public:
	enum {
		ctlgroupSpcDvcInp = 1,
		ctlgroupLoc       = 2
	};
	explicit SCardDialog(long options = 0) : TDialog(DLG_SCARD), Options(options)
	{
		SetupCalDate(CTLCAL_SCARD_DATE,   CTL_SCARD_DATE);
		SetupCalDate(CTLCAL_SCARD_EXPIRY, CTL_SCARD_EXPIRY);
		addGroup(ctlgroupSpcDvcInp, new SpecialInputCtrlGroup(CTL_SCARD_CODE, 500));
		if(getCtrlView(CTL_SCARD_PHONEINPUT)) {
			LocationCtrlGroup * p_loc_grp = new LocationCtrlGroup(0, 0, CTL_SCARD_PHONEINPUT, 0, cmAddress, LocationCtrlGroup::fStandaloneByPhone, 0);
			if(p_loc_grp) {
				p_loc_grp->SetInfoCtl(CTL_SCARD_ADDRINFO);
				addGroup(ctlgroupLoc, p_loc_grp);
			}
		}
		if(Options & PPObjSCard::edfDisableCode)
			setCtrlReadOnly(CTL_SCARD_CODE, true);
	}
	int    setDTS(const PPSCardPacket * pData, const PPSCardSerPacket * pScsPack);
	int    getDTS(PPSCardPacket * pData);
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_SCARD_FLAGS)) {
			SetupCtrls();
		}
		else if(event.isCbSelected(CTLSEL_SCARD_SERIES)) {
			PPID   person_id = getCtrlLong(CTLSEL_SCARD_PERSON);
			PPID   series_id = getCtrlLong(CTLSEL_SCARD_SERIES);
			Data.Rec.AutoGoodsID = getCtrlLong(CTLSEL_SCARD_AUTOGOODS);
			SetupSeries(series_id, person_id);
		}
		else if(event.isCbSelected(CTLSEL_SCARD_PERSON)) { // @v11.6.3
			PPID   person_id = getCtrlLong(CTLSEL_SCARD_PERSON);
			PPID   series_id = getCtrlLong(CTLSEL_SCARD_SERIES);
			bool   enable_person_autocreation = false;
			if(series_id && !person_id)  {
				PPSCardSeries2 scs_rec;
				if(ObjSCardSer.Fetch(series_id, &scs_rec) > 0 && scs_rec.Flags & SCRDSF_ALLOWOWNERAUTOCR)
					enable_person_autocreation = true;
			}
			enableCommand(cmAutoCreateOwner, enable_person_autocreation);
		}
		else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_SCARD_PHONE)) {
			SetupPhoneButton(this, CTL_SCARD_PHONE, cmSCardAction1);
		}
		else if(event.isCmd(cmSCardAction1)) {
			const  PPID def_phn_svc_id = DS.GetConstTLA().DefPhnSvcID;
			if(def_phn_svc_id) {
				SString temp_buf;
				getCtrlString(CTL_SCARD_PHONE, temp_buf);
				if(temp_buf.Len() >= 5) {
					SString phone_buf;
					temp_buf.Transf(CTRANSF_INNER_TO_UTF8).Utf8ToLower();
					if(PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone_buf).Len() >= 5)
						PPObjPhoneService::PhoneTo(phone_buf);
				}
			}
		}
		else if(event.isCmd(cmInputUpdated)) {
			if(event.isCtlEvent(CTL_SCARD_PHONE)) {
				SString phone;
				getCtrlString(CTL_SCARD_PHONE, phone);
				enableCommand(cmVerify, phone.NotEmptyS());
			}
			else
				return;
		}
		else if(event.isCmd(cmVerify)) {
			//SString phone, scard_code;
			SString temp_buf;
			getCtrlString(CTL_SCARD_PHONE, temp_buf);
			Data.PutExtStrData(PPSCardPacket::extssPhone, temp_buf);
			getCtrlString(CTL_SCARD_CODE, temp_buf);
			STRNSCPY(Data.Rec.Code, temp_buf);
			if(ScObj.VerifyOwner(Data, 0/*posNodeID*/, 0) > 0) {
				;
			}
			/*if(phone.NotEmptyS() && scard_code.NotEmptyS()) {
				uint   check_code = 0;
				if(VerifyPhoneNumberBySms(phone, scard_code, &check_code) > 0) {
					if(check_code) {
                        ; // @todo Какую-то отметку сделать
					}
				}
			}*/
		}
		else if(event.isCmd(cmNotifyOptions))
			EditNotifyOptions();
		else if(event.isCmd(cmAutoCreateOwner)) { // @v11.6.3
			PPID   person_id = getCtrlLong(CTLSEL_SCARD_PERSON);
			PPID   series_id = getCtrlLong(CTLSEL_SCARD_SERIES);
			SString code_buf;
			getCtrlString(CTL_SCARD_CODE, code_buf);
			STRNSCPY(Data.Rec.Code, code_buf);
			if(series_id && !person_id && code_buf.NotEmptyS())  {
				PPSCardSeries2 scs_rec;
				if(ObjSCardSer.Fetch(series_id, &scs_rec) > 0 && scs_rec.Flags & SCRDSF_ALLOWOWNERAUTOCR) {
					PPID   psn_kind_id = scs_rec.PersonKindID;
					SETIFZ(psn_kind_id, ScObj.GetConfig().PersonKindID);
					SETIFZ(psn_kind_id, PPPRK_CLIENT);					
					if(psn_kind_id) {
						PPID   new_psn_id = 0;
						if(PsnObj.AddSimple(&new_psn_id, code_buf, psn_kind_id, PPPRS_PRIVATE, 1) > 0) {
							Data.Rec.PersonID = new_psn_id;
							SetupPersonCombo(this, CTLSEL_SCARD_PERSON, new_psn_id, OLW_CANINSERT|OLW_LOADDEFONOPEN, psn_kind_id, 0);
						}
					}
				}
			}			
		}
		else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_SCARD_EXPIRY)) {
			if(DateAddDialog(&ScExpiryPeriodParam) > 0 && checkdate(ScExpiryPeriodParam.ResultDate, 0)) {
				setCtrlDate(CTL_SCARD_EXPIRY, ScExpiryPeriodParam.ResultDate);
			}
		}
		else
			return;
		clearEvent(event);
	}
	void   SetDiscount()
	{
		setCtrlReal(CTL_SCARD_PDIS, fdiv100i(Data.Rec.PDis));
	}
	void   GetDiscount(uint * pSel)
	{
		uint   sel = 0;
		const  double val = getCtrlReal(sel = CTL_SCARD_PDIS);
		Data.Rec.PDis = fmul100i(val);
		ASSIGN_PTR(pSel, sel);
	}
	void   SetFixedBonus()
	{
		setCtrlReal(CTL_SCARD_FIXBON, fdiv100i(Data.Rec.FixedBonus));
	}
	void   GetFixedBonus(uint * pSel)
	{
		uint   sel = 0;
		const  double val = getCtrlReal(sel = CTL_SCARD_FIXBON);
		Data.Rec.FixedBonus = fmul100i(val);
		ASSIGN_PTR(pSel, sel);
	}
	void   SetupSeries(PPID seriesID, PPID personID);
	void   SetupCtrls();
	int    EditNotifyOptions()
	{
        int    ok = -1;
        TDialog * dlg = new TDialog(DLG_SCNTFOPT);
        if(CheckDialogPtrErr(&dlg)) {
			dlg->AddClusterAssoc(CTL_SCNTFOPT_FLAGS, 0, SCRDF_NOTIFYDISCOUNT);
			dlg->AddClusterAssoc(CTL_SCNTFOPT_FLAGS, 1, SCRDF_NOTIFYWITHDRAW);
			dlg->AddClusterAssoc(CTL_SCNTFOPT_FLAGS, 2, SCRDF_NOTIFYDRAW);
			dlg->SetClusterData(CTL_SCNTFOPT_FLAGS, Data.Rec.Flags);
			if(ExecView(dlg) == cmOK) {
				dlg->GetClusterData(CTL_SCNTFOPT_FLAGS, &Data.Rec.Flags);
				ok = 1;
			}
        }
        else
			ok = 0;
		delete dlg;
		return ok;
	}

	long   Options;
	DateAddDialogParam ScExpiryPeriodParam;
	PPObjSCard ScObj;
	PPSCardPacket Data;
	PPSCardSerPacket ScsPack;
	PPObjSCardSeries ObjSCardSer;
	PPObjPerson PsnObj;
};

void SCardDialog::SetupCtrls()
{
	const  long   prev_flags = Data.Rec.Flags;
	GetClusterData(CTL_SCARD_FLAGS, &Data.Rec.Flags);
	const  long   preserve_flags = Data.Rec.Flags;
	long   flags = Data.Rec.Flags;
	if((flags & SCRDF_INHERITED) != (prev_flags & SCRDF_INHERITED)) {
		GetDiscount(0);
		GetFixedBonus(0);
		getCtrlData(CTL_SCARD_MAXCRED, &Data.Rec.MaxCredit);
		getCtrlData(CTL_SCARD_DATE,   &Data.Rec.Dt);
		getCtrlData(CTL_SCARD_EXPIRY, &Data.Rec.Expiry);
		GetTimeRangeInput(this, CTL_SCARD_USAGETM, TIMF_HM, &Data.Rec.UsageTmStart, &Data.Rec.UsageTmEnd);
		if(ScObj.SetInheritance(0, &Data.Rec) > 0) {
			SetDiscount();
			SetFixedBonus();
			setCtrlData(CTL_SCARD_MAXCRED, &Data.Rec.MaxCredit);
			setCtrlDate(CTL_SCARD_DATE,   Data.Rec.Dt);
			setCtrlDate(CTL_SCARD_EXPIRY, Data.Rec.Expiry);
			SetTimeRangeInput(this, CTL_SCARD_USAGETM, TIMF_HM, &Data.Rec.UsageTmStart, &Data.Rec.UsageTmEnd);
		}
	}
	if(!(flags & SCRDF_CLOSED) && (prev_flags & SCRDF_CLOSED)) {
		flags &= ~SCRDF_NEEDACTIVATION;
		Data.Rec.PeriodTerm  = static_cast<int16>(getCtrlLong(CTLSEL_SCARD_PRD));
		Data.Rec.PeriodCount = static_cast<int16>(getCtrlUInt16(CTL_SCARD_PRDCOUNT));
		if(Data.Rec.PeriodTerm) {
			LDATE  dt = getcurdate_();
			plusperiod(&dt, Data.Rec.PeriodTerm, ((Data.Rec.PeriodCount > 0) ? Data.Rec.PeriodCount : 1), 0);
			setCtrlDate(CTL_SCARD_EXPIRY, dt);
			flags &= ~SCRDF_INHERITED;
		}
	}
	else if(flags & SCRDF_NEEDACTIVATION) {
		flags |= SCRDF_CLOSED;
	}
	if(!(flags & SCRDF_NEEDACTIVATION))
		flags &= ~SCRDF_AUTOACTIVATION;
	//
	// Все-таки, карта, не имеющая флага SCRDF_NEEDACTIVATION должна допускать ввод периода действия после
	// активации дабы можно было установить требование активации уже после установки периода.
	// disableCtrls(!(flags & SCRDF_NEEDACTIVATION), CTL_SCARD_PRD, CTLSEL_SCARD_PRD, CTL_SCARD_PRDCOUNT, 0);
	//
	disableCtrls((flags & SCRDF_INHERITED), CTL_SCARD_DATE, CTL_SCARD_EXPIRY, CTL_SCARD_PDIS, CTL_SCARD_MAXCRED, CTL_SCARD_FIXBON, 0);
	DisableClusterItem(CTL_SCARD_FLAGS, 5, !(flags & SCRDF_NEEDACTIVATION));
	if(flags != preserve_flags)
		SetClusterData(CTL_SCARD_FLAGS, Data.Rec.Flags = flags);
}

void SCardDialog::SetupSeries(PPID seriesID, PPID personID)
{
	PPID   psn_kind_id = 0;
	PPID   goods_grp_id = 0;
	PPSCardSeries2 scs_rec;
	SString info_buf;
	bool   enable_person_autocreation = false; // @v11.6.3 
	int    scst = scstUnkn;
	if(seriesID && ObjSCardSer.Fetch(seriesID, &scs_rec) > 0) {
		psn_kind_id = scs_rec.PersonKindID;
		goods_grp_id = scs_rec.CrdGoodsGrpID;
		scst = scs_rec.GetType();
		if(oneof2(scst, scstBonus, scstCredit) && scs_rec.Flags & SCRDSF_UHTTSYNC) {
			SString sc_code, uhtt_hash;
			getCtrlString(CTL_SCARD_CODE, sc_code);
			if(sc_code.NotEmptyS()) {
				int   uhtt_err = 1;
				double uhtt_rest = 0.0;
				PPUhttClient uhtt_cli;
				if(uhtt_cli.Auth()) {
					UhttSCardPacket scp;
					if(uhtt_cli.GetSCardByNumber(sc_code, scp)) {
						uhtt_hash = scp.Hash;
						if(uhtt_cli.GetSCardRest(/*scp.Code*/sc_code, 0, uhtt_rest))
							uhtt_err = 0;
					}
				}
				(info_buf = "UHTT").CatDiv(':', 2);
				if(uhtt_err)
					info_buf.Cat("Error");
				else
					info_buf.Cat(uhtt_rest, SFMT_MONEY).Space().CatChar('(').Cat(uhtt_hash).CatChar(')');
			}
		}
	}
	SETIFZ(psn_kind_id, ScObj.GetConfig().PersonKindID);
	SETIFZ(psn_kind_id, PPPRK_CLIENT);
	if(PsnObj.P_Tbl->IsBelongsToKind(personID, psn_kind_id) <= 0)
		personID = 0;
	SetupPersonCombo(this, CTLSEL_SCARD_PERSON, personID, OLW_CANINSERT|OLW_LOADDEFONOPEN, psn_kind_id, 0);
	// @v11.6.3 {
	if(scs_rec.Flags & SCRDSF_ALLOWOWNERAUTOCR && !personID)
		enable_person_autocreation = true;
	enableCommand(cmAutoCreateOwner, enable_person_autocreation); // @v11.6.3 
	// } @v11.6.3
	if(goods_grp_id)
		SetupPPObjCombo(this, CTLSEL_SCARD_AUTOGOODS, PPOBJ_GOODS, Data.Rec.AutoGoodsID, 0, reinterpret_cast<void *>(goods_grp_id));
	else
		setCtrlLong(CTLSEL_SCARD_AUTOGOODS, Data.Rec.AutoGoodsID = 0);
	disableCtrl(CTLSEL_SCARD_AUTOGOODS, !goods_grp_id);
	disableCtrl(CTL_SCARD_PHONE, scst == scstRsrvPool);
	disableCtrl(CTLSEL_SCARD_PERSON, scst == scstRsrvPool);
	disableCtrl(CTL_SCARD_PSW, scst == scstRsrvPool);
	showCtrl(CTLSEL_SCARD_POOLDSER, scst == scstRsrvPool);
	disableCtrl(CTLSEL_SCARD_POOLDSER, scst != scstRsrvPool);
	showCtrl(CTL_SCARD_FIXBON, oneof2(scst, scstCredit, scstBonus));
	setStaticText(CTL_SCARD_ST_INFO, info_buf);
}

int SCardDialog::setDTS(const PPSCardPacket * pData, const PPSCardSerPacket * pScsPack)
{
	Data = *pData;
	if(!RVALUEPTR(ScsPack, pScsPack))
		ScsPack.Z();
	int    ok = 1;
	SString temp_buf;
	ScObj.SetInheritance(&ScsPack, &Data.Rec);
	SetupPPObjCombo(this, CTLSEL_SCARD_SERIES, PPOBJ_SCARDSERIES, Data.Rec.SeriesID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_SCARD_POOLDSER, PPOBJ_SCARDSERIES, Data.Rec.PoolDestSerID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_SCARD_AUTOGOODS, PPOBJ_GOODS, Data.Rec.AutoGoodsID, OLW_CANINSERT|OLW_LOADDEFONOPEN, 0);
	setCtrlData(CTL_SCARD_CODE, Data.Rec.Code);
	setCtrlData(CTL_SCARD_ID, &Data.Rec.ID);
	{
		LocationCtrlGroup::Rec lcg_rec;
		lcg_rec.LocList.Add(Data.Rec.LocID);
		setGroupData(ctlgroupLoc, &lcg_rec);
	}
	SetupSeries(Data.Rec.SeriesID, Data.Rec.PersonID);
	Data.GetExtStrData(Data.extssPassword, temp_buf);
	setCtrlString(CTL_SCARD_PSW, temp_buf);
	Data.GetExtStrData(Data.extssMemo, temp_buf);
	setCtrlString(CTL_SCARD_MEMO, temp_buf);
	{
		Data.GetExtStrData(Data.extssPhone, temp_buf);
		setCtrlString(CTL_SCARD_PHONE, temp_buf);
		enableCommand(cmVerify, temp_buf.NotEmptyS());
	}
	{
		Data.GetExtStrData(Data.extssOuterId, temp_buf);
		setCtrlString(CTL_SCARD_OUTERID, temp_buf);
	}
	AddClusterAssoc(CTL_SCARD_FLAGS, 0, SCRDF_INHERITED);
	AddClusterAssoc(CTL_SCARD_FLAGS, 1, SCRDF_CLOSED);
	AddClusterAssoc(CTL_SCARD_FLAGS, 2, SCRDF_CLOSEDSRV);
	AddClusterAssoc(CTL_SCARD_FLAGS, 3, SCRDF_NOGIFT);
	AddClusterAssoc(CTL_SCARD_FLAGS, 4, SCRDF_NEEDACTIVATION);
	AddClusterAssoc(CTL_SCARD_FLAGS, 5, SCRDF_AUTOACTIVATION);
	AddClusterAssoc(CTL_SCARD_FLAGS, 6, SCRDF_OWNERVERIFIED);
	SetClusterData(CTL_SCARD_FLAGS, Data.Rec.Flags);
	setCtrlDate(CTL_SCARD_DATE,   Data.Rec.Dt);
	setCtrlDate(CTL_SCARD_EXPIRY, Data.Rec.Expiry);
	SetDiscount();
	SetFixedBonus();
	setCtrlData(CTL_SCARD_MAXCRED, &Data.Rec.MaxCredit);
	SetTimeRangeInput(this, CTL_SCARD_USAGETM, TIMF_HM, &Data.Rec.UsageTmStart, &Data.Rec.UsageTmEnd);
	SetupStringCombo(this, CTLSEL_SCARD_PRD, PPTXT_CYCLELIST, Data.Rec.PeriodTerm);
	setCtrlUInt16(CTL_SCARD_PRDCOUNT, Data.Rec.PeriodCount);
	SetupSpin(CTLSPN_SCARD_PRDCOUNT, CTL_SCARD_PRDCOUNT, 0, 365*4+1, Data.Rec.PeriodCount);
	SetupCtrls();
	SetupPhoneButton(this, CTL_SCARD_PHONE, cmSCardAction1);
	if(Data.Rec.SeriesID)
		selectCtrl(CTL_SCARD_CODE);
	return ok;
}

int SCardDialog::getDTS(PPSCardPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	SString temp_buf;
	getCtrlData(sel = CTLSEL_SCARD_SERIES, &Data.Rec.SeriesID);
	THROW_PP(Data.Rec.SeriesID, PPERR_SCARDSERNEEDED);
	if(!(Options & PPObjSCard::edfDisableCode)) {
		getCtrlData(sel = CTL_SCARD_CODE, Data.Rec.Code);
		strip(Data.Rec.Code);
		THROW_PP(Data.Rec.Code[0], PPERR_SCARDCODENEEDED);
	}
	getCtrlData(sel = CTLSEL_SCARD_PERSON, &Data.Rec.PersonID);
	{
		LocationCtrlGroup::Rec lcg_rec;
		getGroupData(ctlgroupLoc, &lcg_rec);
		Data.Rec.LocID = lcg_rec.LocList.GetSingle();
	}
	{
		getCtrlString(CTL_SCARD_PSW, temp_buf);
		Data.PutExtStrData(Data.extssPassword, temp_buf.Strip());
	}
	{
		getCtrlString(CTL_SCARD_MEMO, temp_buf);
		Data.PutExtStrData(Data.extssMemo, temp_buf.Strip());
	}
	{
		getCtrlString(CTL_SCARD_PHONE, temp_buf);
		Data.PutExtStrData(Data.extssPhone, temp_buf.Strip());
	}
	getCtrlData(sel = CTLSEL_SCARD_AUTOGOODS, &Data.Rec.AutoGoodsID);
	GetClusterData(CTL_SCARD_FLAGS, &Data.Rec.Flags);
	getCtrlData(sel = CTL_SCARD_DATE,   &Data.Rec.Dt);
	THROW_SL(checkdate(Data.Rec.Dt, 1));
	getCtrlData(sel = CTL_SCARD_EXPIRY, &Data.Rec.Expiry);
	THROW_SL(checkdate(Data.Rec.Expiry, 1));
	GetDiscount(&sel);
	GetFixedBonus(&sel);
	getCtrlData(sel = CTL_SCARD_MAXCRED, &Data.Rec.MaxCredit);
	THROW(GetTimeRangeInput(this, CTL_SCARD_USAGETM, TIMF_HM, &Data.Rec.UsageTmStart, &Data.Rec.UsageTmEnd));
	Data.Rec.PeriodTerm  = static_cast<int16>(getCtrlLong(CTLSEL_SCARD_PRD));
	Data.Rec.PeriodCount = static_cast<int16>(getCtrlUInt16(CTL_SCARD_PRDCOUNT));
	{
		getCtrlData(sel = CTLSEL_SCARD_POOLDSER, &Data.Rec.PoolDestSerID);
		if(Data.Rec.PoolDestSerID) {
			PPSCardSeries scs_rec;
			const int scst = (ObjSCardSer.Fetch(Data.Rec.PoolDestSerID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
			THROW_PP_S(scst != scstRsrvPool, PPERR_SCARDPOOLDSERISPOOL, scs_rec.Name);
		}
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int PPObjSCard::SetFlags(PPID id, long flags, int use_ta)
{
	int    ok = -1;
	SCardTbl::Rec rec;
	THROW(CheckRights(PPR_MOD));
	if(P_Tbl->Search(id, &rec) > 0) {
		if(rec.Flags != flags) {
			rec.Flags = flags;
			THROW(UpdateByID(P_Tbl, Obj, id, &rec, use_ta));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::FindAndEdit(PPID * pID, const AddParam * pParam)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_SCARDNUM);
	if(CheckDialogPtrErr(&dlg)) {
		AddParam local_add_param;
		RVALUEPTR(local_add_param, pParam);
		SString code;
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_SCARDNUM_SCARDNUM, code);
			if(code.NotEmptyS()) {
				int    found = 0;
				SCardTbl::Rec rec;
				if(P_Tbl->SearchCode(0, code, &rec) > 0)
					found = 1;
				else {
					//
					// Если в конфигурации не установлен признак CCFLG_THROUGHSCARDUNIQ
					// (сквозная уникальность номеров карт), то, возможно, карта найдется //
					// с ненулевым значением серии.
					//
					if(P_Tbl->SearchCode(local_add_param.SerID, code, &rec) > 0)
						found = 1;
				}
				if(found) {
					ASSIGN_PTR(pID, rec.ID);
					if(Helper_Edit(pID, &local_add_param) == cmOK) {
						ok = 1;
					}
				}
				else if(CONFIRM_S(PPCFM_ADDNEWSCARD, code)) {
					local_add_param.Code = code;
					if(Helper_Edit(pID, &local_add_param) == cmOK) {
						ok = 2;
					}
				}
			}
			else
				PPError(PPERR_SCARDCODENEEDED);
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjSCard::EditDialog(PPSCardPacket * pPack, long flags)
{
	int    ok = -1;
	SCardDialog * dlg = 0;
	if(pPack) {
		PPSCardPacket pack;
		PPSCardSerPacket scs_pack;
		PPObjSCardSeries ser_obj;
		pack = *pPack;
		if(pack.Rec.SeriesID) {
			THROW(ser_obj.GetPacket(pack.Rec.SeriesID, &scs_pack) > 0);
		}
		THROW(CheckDialogPtrErr(&(dlg = new SCardDialog(flags))));
		THROW(dlg->setDTS(&pack, &scs_pack));
		while(ok < 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&pack)) {
				ASSIGN_PTR(pPack, pack);
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjSCard::FindDiscountBorrowingCard(PPID ownerID, SCardTbl::Rec * pRec)
{
	int    ok = -1;
	if(ownerID) {
		PPIDArray card_id_list;
		P_Tbl->GetListByPerson(ownerID, 0, &card_id_list);
		if(card_id_list.getCount()) {
			PPObjSCardSeries scs_obj;
			PPSCardSeries scs_rec;
			SCardTbl::Rec rec;
			LAssocArray sc_dis_list; // Список доступных для применения карт, ассоциированных с соответствующим процентом скидки
			for(uint i = 0; i < card_id_list.getCount(); i++) {
				const  PPID sc_id = card_id_list.get(i);
				if(Search(sc_id, &rec) > 0 && scs_obj.Fetch(rec.SeriesID, &scs_rec) > 0 && scs_rec.Flags & SCRDSF_TRANSFDISCOUNT) {
					const int cr = CheckRestrictions(&rec, chkrfIgnoreUsageTime, getcurdatetime_());
					if(cr == 1 && rec.PDis > 0) // Значение cr==2 не годится (карта требует активации с возможностью автоактивации)
						sc_dis_list.Add(sc_id, rec.PDis, 0);
				}
			}
			if(sc_dis_list.getCount() == 1) {
				THROW(Search(sc_dis_list.at(0).Key, &rec) > 0); // После всех телодвижений выше невозможность найти карту - ошибка
				ASSIGN_PTR(pRec, rec);
                ok = 1;
			}
			else if(sc_dis_list.getCount() > 1) {
				sc_dis_list.Sort();
				long   common_pdis = 0;
				for(uint scdlidx = 0; scdlidx < sc_dis_list.getCount(); scdlidx++) {
                    const  PPID sc_id = sc_dis_list.at(scdlidx).Key;
					const long pdis = sc_dis_list.at(scdlidx).Val;
					if(pdis != common_pdis) {
						if(common_pdis == 0)
							common_pdis = pdis;
						else {
							common_pdis = 0;
							break;
						}
					}
				}
				if(common_pdis > 0) {
                    //
                    // Если во всех картах скидки одинаковы, просто берем ту карту, у которой больший идент
                    // (вероятно, самая новая, но это - не очень важно)
                    //
                    const  PPID sc_id = sc_dis_list.at(sc_dis_list.getCount()-1).Key;
					THROW(Search(sc_id, &rec) > 0); // После всех телодвижений выше невозможность найти карту - ошибка
					ASSIGN_PTR(pRec, rec);
					ok = 1;
				}
				else {
					//
					// Если претендентов несколько, то выбираем по последнему времени создания/изменения скидки
					//
					LDATETIME last_upd_moment = ZERODATETIME;
					PPID   last_upd_sc_id = 0;
					SysJournal * p_sj = DS.GetTLA().P_SysJ;
					if(p_sj) {
						PPIDArray ev_list;
						ev_list.addzlist(PPACN_OBJADD, PPACN_SCARDDISUPD, 0L);
						for(uint j = 0; j < sc_dis_list.getCount(); j++) {
							const  PPID sc_id = sc_dis_list.at(j).Key;
							LDATETIME ev_dtm;
							if(p_sj->GetLastObjEvent(PPOBJ_SCARD, sc_id, &ev_list, &ev_dtm) > 0 && cmp(ev_dtm, last_upd_moment) > 0) {
								last_upd_moment = ev_dtm;
								last_upd_sc_id = sc_id;
							}
						}
					}
					{
						const  PPID sc_id = NZOR(last_upd_sc_id, sc_dis_list.at(sc_dis_list.getCount()-1).Key);
						THROW(Search(sc_id, &rec) > 0); // После всех телодвижений выше невозможность найти карту - ошибка
						ASSIGN_PTR(pRec, rec);
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::Helper_Edit(PPID * pID, const AddParam * pParam)
{
	int    ok = -1;
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	long   prev_pdis = 0;
	PPSCardPacket pack;
	PPObjSCardSeries ser_obj;
	PPSCardSerPacket scs_pack;
	SCardDialog * dlg = new SCardDialog();
	THROW(CheckDialogPtr(&dlg));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
		prev_pdis = pack.Rec.PDis;
		if(pack.Rec.SeriesID) {
			THROW(ser_obj.GetPacket(pack.Rec.SeriesID, &scs_pack) > 0);
			if(pack.Rec.Flags & SCRDF_INHERITED)
				prev_pdis = scs_pack.Rec.PDis;
		}
	}
	else {
		if(pParam) {
			pack.Rec.SeriesID = pParam->SerID;
			pack.Rec.PersonID = pParam->OwnerID;
			if(!pack.Rec.PersonID)
				pack.Rec.LocID = pParam->LocID;
			pParam->Code.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
			pack.PutExtStrData(pack.extssPhone, pParam->Phone);
		}
		if(pack.Rec.SeriesID && ser_obj.GetPacket(pack.Rec.SeriesID, &scs_pack) > 0) {
			if(pack.Rec.Code[0] == 0 && scs_pack.Eb.CodeTempl) {
				SString new_code;
				if(P_Tbl->MakeCodeByTemplate(scs_pack.Rec.ID, scs_pack.Eb.CodeTempl, new_code) > 0)
					STRNSCPY(pack.Rec.Code, new_code);
			}
			if(scs_pack.Rec.Flags & SCRDSF_NEWSCINHF) {
				pack.Rec.Flags |= SCRDF_INHERITED;
				THROW(SetInheritance(&scs_pack, &pack.Rec));
			}
			else if(pack.Rec.PersonID) {
				SCardTbl::Rec dbc_rec;
				if(FindDiscountBorrowingCard(pack.Rec.PersonID, &dbc_rec) > 0)
                    pack.Rec.PDis = dbc_rec.PDis;
			}
		}
	}
	THROW(dlg->setDTS(&pack, &scs_pack));
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(PutPacket(pID, &pack, 1))
				valid_data = 1;
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjSCard::Edit(PPID * pID, void * extraPtr /*serID*/)
{
	AddParam param(reinterpret_cast<PPID>(extraPtr));
	return Helper_Edit(pID, &param);
}

int PPObjSCard::Edit(PPID * pID, const AddParam & rParam)
{
	return Helper_Edit(pID, &rParam);
}

int PPObjSCard::ViewVersion(PPID histID)
{
	int    ok = -1;
	SCardDialog * dlg = 0;
	if(histID) {
		SBuffer buf;
		PPSCardPacket pack;
		ObjVersioningCore * p_ovc = PPRef->P_OvT;
		if(p_ovc && p_ovc->InitSerializeContext(1)) {
			SSerializeContext & r_sctx = p_ovc->GetSCtx();
			PPObjID oid;
			long   vv = 0;
			THROW(p_ovc->Search(histID, &oid, &vv, &buf) > 0);
			THROW(SerializePacket(-1, &pack, buf, &r_sctx));
			{
				THROW(CheckDialogPtr(&(dlg = new SCardDialog())));
				dlg->setDTS(&pack, 0/**/);
				dlg->enableCommand(cmOK, 0);
				ExecView(dlg);
			}
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjSCard::IsPacketEq(const PPSCardPacket & rS1, const PPSCardPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
	CMP_MEMB(ID);
	CMP_MEMB(SeriesID);
	CMP_MEMB(PersonID);
	CMP_MEMB(Flags);
	CMP_MEMB(Dt);
	CMP_MEMB(Expiry);
	CMP_MEMB(PDis);
	CMP_MEMB(AutoGoodsID);
	CMP_MEMB(MaxCredit);
	CMP_MEMB(Turnover);
	CMP_MEMB(Rest);
	CMP_MEMB(InTrnovr);
	CMP_MEMB(UsageTmStart);
	CMP_MEMB(UsageTmEnd);
	CMP_MEMB(PeriodTerm);
	CMP_MEMB(PeriodCount);
	CMP_MEMB(LocID);
	CMP_MEMB(FixedBonus);
	CMP_MEMB(PoolDestSerID);
#undef CMP_MEMB
	if(strcmp(rS1.Rec.Code, rS2.Rec.Code) != 0)
		return 0;
	{
		SString t1, t2;
        rS1.GetExtStrData(PPSCardPacket::extssMemo, t1);
        rS2.GetExtStrData(PPSCardPacket::extssMemo, t2);
        if(t1 != t2)
			return 0;
		else {
			rS1.GetExtStrData(PPSCardPacket::extssPassword, t1);
			rS2.GetExtStrData(PPSCardPacket::extssPassword, t2);
			if(t1 != t2)
				return 0;
			else {
				rS1.GetExtStrData(PPSCardPacket::extssPhone, t1);
				rS2.GetExtStrData(PPSCardPacket::extssPhone, t2);
				if(t1 != t2)
					return 0;
				else {
					rS1.GetExtStrData(PPSCardPacket::extssOuterId, t1);
					rS2.GetExtStrData(PPSCardPacket::extssOuterId, t2);
					if(t1 != t2)
						return 0;
				}
			}
		}
	}
	return 1;
}

int PPObjSCard::GetPacket(PPID id, PPSCardPacket * pPack)
{
	int    ok = -1;
	assert(pPack);
	pPack->Z();
	if(PPCheckGetObjPacketID(Obj, id)) {
		if(Search(id, &pPack->Rec) > 0) {
			{
				SString text_buf;
				THROW(PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_SCARDEXT), text_buf));
				text_buf.Transf(CTRANSF_UTF8_TO_INNER);
				pPack->SetBuffer(text_buf.Strip());
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::PutPacket(PPID * pID, PPSCardPacket * pPack, int use_ta)
{
	int    ok = 1, r;
	Reference * p_ref = PPRef;
	ObjVersioningCore * p_ovc = p_ref->P_OvT;
	LocationCore * p_loc_core = LocObj.P_Tbl;
	PPID   hid = 0; // Версионный идентификатор для сохранения в системном журнале
	SBuffer hist_buf;
	int    do_dirty = 0;
	PPID   id = DEREFPTRORZ(pID);
	const  int do_index_phones = BIN(CConfig.Flags2 & CCFLG2_INDEXEADDR);
	SString temp_buf;
	PPID   log_action_id = 0;
	PPSCardPacket org_pack;
	PPSCardSerPacket scs_pack;
	SString ext_buffer;
	if(pPack) {
		PPObjSCardSeries scs_obj;
		THROW_PP(pPack->Rec.SeriesID, PPERR_UNDEFSCARDSER);
		THROW(scs_obj.GetPacket(pPack->Rec.SeriesID, &scs_pack) > 0);
		if(id != 0) {
			THROW_PP(pPack->Rec.Code[0], PPERR_UNDEFSCARDCODE);
		}
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(id) {
			THROW(GetPacket(id, &org_pack) > 0);
			if(pPack == 0) {
				//
				// Удаление пакета
				//
				THROW(CheckRights(PPR_DEL));
				if(DoObjVer_SCard) {
					if(p_ovc && p_ovc->InitSerializeContext(0)) {
						SSerializeContext & r_sctx = p_ovc->GetSCtx();
						THROW(SerializePacket(+1, &org_pack, hist_buf, &r_sctx));
					}
				}
				THROW(RemoveObjV(id, 0, 0, 0));
				if(do_index_phones) {
					org_pack.GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
					PPObjID objid(Obj, id);
					THROW(p_loc_core->IndexPhone(temp_buf, &objid, 1, 0));
				}
				do_dirty = 1;
			}
			else {
				//
				// Изменение пакета
				//
				if(IsPacketEq(*pPack, org_pack, 0)) {
					//
					// Ничего не изменилось
					//
					log_action_id = 0;
				}
				else {
					SCardTbl::Rec same_code_rec;
					THROW(CheckRights(PPR_MOD));
					THROW(r = SearchCode(pPack->Rec.SeriesID, pPack->Rec.Code, &same_code_rec));
					THROW_PP_S(r < 0 || same_code_rec.ID == id, PPERR_DUPLSCARDFOUND, pPack->Rec.Code);
					if(DoObjVer_SCard) {
						if(p_ovc && p_ovc->InitSerializeContext(0)) {
							SSerializeContext & r_sctx = p_ovc->GetSCtx();
							THROW(SerializePacket(+1, &org_pack, hist_buf, &r_sctx));
						}
					}
					THROW(UpdateByID(P_Tbl, Obj, id, &pPack->Rec, 0));
					(ext_buffer = pPack->GetBuffer()).Strip();
					THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, id, PPTRPROP_SCARDEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
					if(do_index_phones) {
						SString org_pack_phone;
						org_pack.GetExtStrData(PPSCardPacket::extssPhone, org_pack_phone);
						pPack->GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
						if(temp_buf != org_pack_phone) {
							PPObjID objid(Obj, id);
							THROW(p_loc_core->IndexPhone(org_pack_phone, &objid, 1, 0));
							THROW(p_loc_core->IndexPhone(temp_buf, &objid, 0, 0));
						}
					}
					log_action_id = PPACN_OBJUPD;
					if(pPack->Rec.PDis != org_pack.Rec.PDis)
						DS.LogAction(PPACN_SCARDDISUPD, PPOBJ_SCARD, id, org_pack.Rec.PDis, 0);
					if(pPack->Rec.Expiry != org_pack.Rec.Expiry)
						DS.LogAction(PPACN_SCARDEXPRYUPD, PPOBJ_SCARD, id, static_cast<long>(org_pack.Rec.Expiry), 0);
					do_dirty = 1;
				}
			}
		}
		else if(pPack) {
			//
			// Вставка нового пакета
			//
			THROW(CheckRights(PPR_INS));
			if(pPack->Rec.Code[0] == 0) {
				SString number;
				THROW_PP_S(scs_pack.Eb.CodeTempl[0] != 0, PPERR_UNDEFSCSCODETEMPL, scs_pack.Rec.Name);
				THROW_PP_S(P_Tbl->MakeCodeByTemplate(scs_pack.Rec.ID, scs_pack.Eb.CodeTempl, number) > 0, PPERR_UNABLEMKSCCODEBYTEMPL, scs_pack.Eb.CodeTempl);
				number.CopyTo(pPack->Rec.Code, sizeof(pPack->Rec.Code));
			}
			THROW(r = SearchCode(pPack->Rec.SeriesID, pPack->Rec.Code, 0));
			THROW_PP_S(r < 0, PPERR_DUPLSCARDFOUND, pPack->Rec.Code);
			THROW(AddObjRecByID(P_Tbl, Obj, &id, &pPack->Rec, 0));
			(ext_buffer = pPack->GetBuffer()).Strip();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, id, PPTRPROP_SCARDEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
			if(do_index_phones) {
				pPack->GetExtStrData(PPSCardPacket::extssPhone, temp_buf);
				PPObjID objid(Obj, id);
				THROW(p_loc_core->IndexPhone(temp_buf, &objid, 0, 0));
			}
			pPack->Rec.ID = id;
			*pID = id;
			log_action_id = PPACN_OBJADD;
		}
		if(id && DoObjVer_SCard && hist_buf.GetAvailableSize()) {
			if(p_ovc && p_ovc->InitSerializeContext(0)) {
				THROW(p_ovc->Add(&hid, PPObjID(Obj, id), &hist_buf, 0));
			}
		}
		if(log_action_id) {
			DS.LogAction(log_action_id, Obj, id, hid, 0);
		}
		THROW(tra.Commit());
	}
	if(do_dirty)
		Dirty(id);
	CATCHZOK
	return ok;
}

int PPObjSCard::DeleteObj(PPID id)
{
	int    ok = 1;
	CCheckTbl::Key4 k4;
	MEMSZERO(k4);
	k4.SCardID = id;
	if(!DS.CheckExtFlag(ECF_AVERAGE) && P_CcTbl->search(4, &k4, spGe) && k4.SCardID == id) {
		ok = RetRefsExistsErr(PPOBJ_CCHECK, P_CcTbl->data.ID);
	}
	else {
		THROW(RemoveByID(P_Tbl, id, 0));
		THROW(PPRef->UtrC.SetText(TextRefIdent(Obj, id, PPTRPROP_SCARDEXT), (const wchar_t *)0, 0));
		THROW(P_Tbl->RemoveOpAll(id, 0));
		DS.LogAction(PPACN_OBJRMV, Obj, id, 0, 0);
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::Browse(void * extraPtr)
{
	SCardFilt flt;
	flt.SeriesID = reinterpret_cast<long>(extraPtr);
	return ViewSCard(&flt, 0);
}

int PPObjSCard::SelectCardFromReservePool(PPID * pPoolID, PPID destSeriesID, PPID * pID, int use_ta)
{
	int    ok = -1;
	PPID   sc_id = 0;
	PPID   pool_id = DEREFPTRORZ(pPoolID);
	PPID   dest_series_id = destSeriesID;
	PPObjSCardSeries scs_obj;
	PPSCardSeries2 scs_rec;
	PPSCardSeries2 scs_dest_rec;
	if(pool_id) {
		THROW(scs_obj.Search(pool_id, &scs_rec) > 0);
		THROW_PP_S(scs_rec.GetType() == scstRsrvPool, PPERR_SCARDSERMUSTBERSRVPOOL, scs_rec.Name);
		SETIFZ(dest_series_id, scs_rec.RsrvPoolDestSerID);
	}
	else {
		for(SEnum en = scs_obj.Enum(0); !pool_id && en.Next(&scs_rec) > 0;) {
			if(scs_rec.GetType() == scstRsrvPool && !(scs_rec.Flags & SCRDSF_PASSIVE)) {
				pool_id = scs_rec.ID;
				SETIFZ(dest_series_id, scs_rec.RsrvPoolDestSerID);
			}
		}
	}
	THROW_PP(pool_id, PPERR_NOSCARDSERRSRVPOOL);
	THROW(scs_obj.Search(dest_series_id, &scs_dest_rec) > 0);
	const int dest_ser_type = scs_dest_rec.GetType();
	THROW_PP_S(oneof3(dest_ser_type, scstDiscount, scstCredit, scstBonus), PPERR_SCARDSERMUSTBEREGTYPE, scs_dest_rec.Name);
	{
		SCardTbl::Key2 k2;
		MEMSZERO(k2);
		k2.SeriesID = pool_id;
		while(!sc_id && P_Tbl->search(2, &k2, spGt) && k2.SeriesID == pool_id) {
			sc_id = P_Tbl->data.ID;
		}
		THROW_PP_S(sc_id, PPERR_SCARDSERRSRVPOOLISEMPTY, scs_rec.Name);
		{
			PPSCardPacket pack;
			THROW(GetPacket(sc_id, &pack) > 0);
			pack.Rec.SeriesID = dest_series_id;
			{
				PPID   temp_id = sc_id;
				THROW(PutPacket(&sc_id, &pack, use_ta));
				ok = 1;
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPoolID, pool_id);
	ASSIGN_PTR(pID, sc_id);
	return ok;
}

int PPObjSCard::SearchCodeExt(const char * pCode, const LAssocArray * pSpcTrtScsList, PPIDArray & rScList, TSVector <SCardSpecialTreatment::IdentifyReplyBlock> & rScpRbList)
{
	rScList.clear();
	rScpRbList.clear();
	int    ok = -1;
	SCardTbl::Rec sc_rec;
	SString temp_buf(pCode);
	if(PreprocessSCardCode(temp_buf) > 0) {
		PPIDArray id_list;
		const int mc = P_Tbl->GetListByCode(temp_buf, &id_list);
		/*if(SearchCode(0, temp_buf, &sc_rec) > 0) {
			rScList.add(sc_rec.ID);
			ok = 1;
		}*/
		if(id_list.getCount()) {
			rScList = id_list;
			ok = 1;
		}
		else if(pSpcTrtScsList) {
			for(uint i = 0; i < pSpcTrtScsList->getCount(); i++) {
				const  PPID scs_id = pSpcTrtScsList->at(i).Key;
				const long spc_trt = pSpcTrtScsList->at(i).Val;
				assert(scs_id && spc_trt);
				SCardSpecialTreatment * p_st = SCardSpecialTreatment::CreateInstance(spc_trt);
				if(p_st) {
					SCardSpecialTreatment::IdentifyReplyBlock irb;
					STRNSCPY(irb.InCode, pCode);
					int ict = p_st->IdentifyCode(irb, scs_id, 1/*use_ta*/);
					if(oneof3(ict, SCardSpecialTreatment::ictHash, SCardSpecialTreatment::ictPhone, SCardSpecialTreatment::ictSCardCode)) {
						assert(irb.ScID);
						rScList.add(irb.ScID);
						rScpRbList.insert(&irb);
						ok = 1;
					}
					ZDELETE(p_st);
				}
			}
		}
	}
	if(ok < 0) {
		PPEAddr::Phone::NormalizeStr(pCode, 0, temp_buf);
		if(temp_buf.NotEmptyS()) {
			PPIDArray phone_id_list;
			PPObjLocation loc_obj;
			loc_obj.P_Tbl->SearchPhoneIndex(temp_buf, 0, phone_id_list);
			for(uint i = 0; i < phone_id_list.getCount(); i++) {
				const  PPID ea_id = phone_id_list.get(i);
				EAddrTbl::Rec ea_rec;
				if(loc_obj.P_Tbl->GetEAddr(ea_id, &ea_rec) > 0 && ea_rec.LinkObjType == PPOBJ_SCARD) {
					if(Search(ea_rec.LinkObjID, &sc_rec) > 0) {
						rScList.add(sc_rec.ID);
						ok = 2;
					}
				}
			}
		}
	}
	rScList.sortAndUndup();
	return ok;
}
//
//
//
class SCardTransmitPacket {
public:
	friend class PPObjSCard;

	SCardTransmitPacket();
	void   destroy();
	int    PutCheck(const CCheckTbl::Rec *);
	int    PutOp(const SCardOpTbl::Rec *);
	const  SCardTbl::Rec & GetRec() const { return P.Rec; }
private:
	//SCardTbl::Rec Rec;
	PPSCardPacket P;
	LDATETIME Since;
	//
	// При передаче в другой раздел, поле CCheckTbl::Rec.SessID
	// получает ИД кассового узла (PPOBJ_CASHNODE), к которому
	// относится чек, кроме того в записи чека устанавливаетс
	// флаг CCHKF_TRANSMIT
	//
	TSVector <CCheckTbl::Rec> CheckList;
	TSVector <SCardOpTbl::Rec> ScOpList;
};

SCardTransmitPacket::SCardTransmitPacket() : Since(ZERODATETIME)
{
}

void SCardTransmitPacket::destroy()
{
	P.Z();
	Since.Z();
	CheckList.freeAll();
	ScOpList.freeAll();
}

int SCardTransmitPacket::PutCheck(const CCheckTbl::Rec * pItem) { return (!pItem || CheckList.insert(pItem)) ? 1 : PPSetErrorSLib(); }
int SCardTransmitPacket::PutOp(const SCardOpTbl::Rec * pItem)   { return (!pItem || ScOpList.insert(pItem))  ? 1 : PPSetErrorSLib(); }
//
//
//
int PPObjSCard::GetTransmitPacket(PPID id, const LDATETIME * pMoment, SCardTransmitPacket * pPack, const ObjTransmContext * pCtx)
{
	int    ok = -1;
	pPack->destroy();
	if(GetPacket(id, &pPack->P) > 0) {
		pPack->Since = *pMoment;
		LDATETIME i;
		PPIDArray chk_list;
		CCheckTbl::Rec chk_rec;
		SCardOpTbl::Rec scop_rec;
		if(!(pCtx->Cfg.Flags & DBDXF_SYNCSCARDWOCHECKS)) {
			if(P_CcTbl->GetListByCard(id, &pPack->Since, &chk_list) > 0) {
				for(uint j = 0; j < chk_list.getCount(); j++) {
					if(P_CcTbl->Search(chk_list.at(j), &chk_rec) > 0) {
						if(!(chk_rec.Flags & (CCHKF_SYNC|CCHKF_TRANSMIT))) {
							chk_rec.PosNodeID = 0;
							if(chk_rec.SessID) {
								CSessionTbl::Rec cs_rec;
								THROW_MEM(SETIFZ(P_CsObj, new PPObjCSession));
								if(P_CsObj->Search(chk_rec.SessID, &cs_rec) > 0)
									chk_rec.PosNodeID = cs_rec.CashNodeID;
							}
						}
						chk_rec.SessID = 0;
						chk_rec.UserID = 0;
						THROW(pPack->PutCheck(&chk_rec));
					}
				}
			}
		}
		for(i = pPack->Since; P_Tbl->EnumOpByCard(id, &i, &scop_rec) > 0;) {
			scop_rec.LinkObjType = 0;
			scop_rec.LinkObjID = 0;
			scop_rec.UserID = 0;
			THROW(pPack->PutOp(&scop_rec));
		}
		ok = 1;
	}
	CATCH
		ok = 0;
		pPack->destroy();
	ENDCATCH
	return ok;
}

int PPObjSCard::PutTransmitPacket(PPID * pID, SCardTransmitPacket * pPack, int update, ObjTransmContext * pCtx, int use_ta)
{
	int    ok = 1;
	SCardTbl::Rec same_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if((*pID && Search(*pID, &same_rec) > 0) || SearchCode(pPack->P.Rec.SeriesID, pPack->P.Rec.Code, &same_rec) > 0) {
			*pID = same_rec.ID;
			long   prev_pdis = same_rec.PDis;
			const  int accept_owner_in_disp_div = BIN(GetConfig().Flags & PPSCardConfig::fAcceptOwnerInDispDiv);
			if(update || accept_owner_in_disp_div) {
				int    do_update = 1;
				pPack->P.Rec.Turnover = same_rec.Turnover;
				if(!update && accept_owner_in_disp_div) {
					if(pPack->P.Rec.PersonID != same_rec.PersonID) {
						PPID   save_owner_id = pPack->P.Rec.PersonID;
						pPack->P.Rec = same_rec;
						pPack->P.Rec.PersonID = save_owner_id;
						do_update = 1;
					}
					else
						do_update = 0;
				}
				pPack->P.Rec.ID = *pID;
				if(do_update) {
					if(!PutPacket(pID, &pPack->P, 0)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSCARD, pPack->P.Rec.ID, pPack->P.Rec.Code);
						ok = -1;
					}
				}
			}
		}
		else {
			pPack->P.Rec.ID = 0;
			pPack->P.Rec.Turnover = 0;
			*pID = 0;
			if(!PutPacket(pID, &pPack->P, 0)) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSCARD, pPack->P.Rec.ID, pPack->P.Rec.Code);
				ok = -1;
			}
		}
		if(ok > 0) {
			uint   i;
			SString chk_code;
			CCheckTbl::Rec * p_chk_rec;
			SCardOpTbl::Rec * p_op_rec;
			for(i = 0; pPack->CheckList.enumItems(&i, (void **)&p_chk_rec);) {
				if(p_chk_rec->PosNodeID) {
					if(P_CcTbl->Search(p_chk_rec->PosNodeID, p_chk_rec->Dt, p_chk_rec->Tm) > 0)
						continue;
				}
				else if(P_CcTbl->SearchByTimeAndCard(*pID, p_chk_rec->Dt, p_chk_rec->Tm) > 0)
					continue;
				PPID   chk_id = 0;
				PPID   org_chk_id = p_chk_rec->ID;
				p_chk_rec->SCardID = *pID;
				p_chk_rec->ID = 0;
				if(!P_CcTbl->Add(&chk_id, p_chk_rec, 0)) {
					CCheckCore::MakeCodeString(p_chk_rec, 0, chk_code);
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTCCHECK, org_chk_id, chk_code);
				}
			}
			for(i = 0; pPack->ScOpList.enumItems(&i, (void **)&p_op_rec);)
				if(P_Tbl->SearchOp(*pID, p_op_rec->Dt, p_op_rec->Tm, 0) < 0) {
					p_op_rec->SCardID = *pID;
					if(!P_Tbl->PutOpRec(p_op_rec, 0, 0)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTSCARDOP, pPack->P.Rec.ID, pPack->P.Rec.Code);
						ok = -1;
					}
				}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjSCard, SCardTransmitPacket);

int PPObjSCard::SerializePacket(int dir, SCardTransmitPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->P.Rec, rBuf, pSCtx));
	THROW(pPack->P.SerializeB(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->Since, rBuf));
	THROW_SL(P_CcTbl->SerializeArrayOfRecords(dir, &pPack->CheckList, rBuf, pSCtx));
	THROW_SL(P_Tbl->ScOp.SerializeArrayOfRecords(dir, &pPack->ScOpList, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjSCard::SerializePacket(int dir, PPSCardPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->SerializeB(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjSCard::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SCardTransmitPacket * p_pack = new SCardTransmitPacket;
	THROW_MEM(p_pack);
	if(stream == 0) {
		THROW(GetTransmitPacket(id, &pCtx->TransmitSince, p_pack, pCtx) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
		THROW(SerializePacket(-1, p_pack, buffer, &pCtx->SCtx));
	}
	p->Data = p_pack;
	CATCH
		ok = 0;
		delete p_pack;
	ENDCATCH
	return ok;
}

int PPObjSCard::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	SCardTransmitPacket * p_pack = p ? static_cast<SCardTransmitPacket *>(p->Data) : 0;
	if(p_pack) {
		if(stream == 0) {
			THROW(ok = PutTransmitPacket(pID, p_pack, ((p->Flags & PPObjPack::fDispatcher) ? 0 : 1), pCtx, 1));
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjSCard::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		uint   i;
		SCardTransmitPacket * p_pack = static_cast<SCardTransmitPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_SCARDSERIES, &p_pack->P.Rec.SeriesID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->P.Rec.PersonID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->P.Rec.LocID, ary, replace));
		{
			CCheckTbl::Rec * p_chk_rec;
			for(i = 0; p_pack->CheckList.enumItems(&i, (void **)&p_chk_rec);) {
				THROW(ProcessObjRefInArray(PPOBJ_CASHNODE, &p_chk_rec->PosNodeID, ary, replace));
			}
		}
		{
			SCardOpTbl::Rec * p_scop_rec;
			for(i = 0; p_pack->ScOpList.enumItems(&i, (void **)&p_scop_rec);) {
				THROW(ProcessObjRefInArray(p_scop_rec->LinkObjType, &p_scop_rec->LinkObjID, ary, replace));
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

StrAssocArray * PPObjSCard::MakeStrAssocList(void * extraPtr /*cardSerID*/)
{
	const  Filt * p_filt = static_cast<const Filt *>(extraPtr);
	const  PPID ser_id = (p_filt && p_filt->Signature == PPConst::Signature_PPObjSCard_Filt) ? p_filt->SeriesID : reinterpret_cast<PPID>(extraPtr);
	const  PPID owner_id = (p_filt && p_filt->Signature == PPConst::Signature_PPObjSCard_Filt) ? p_filt->OwnerID : 0;
	union {;
		SCardTbl::Key2 k2;
		SCardTbl::Key3 k3;
	} k;
	StrAssocArray * p_list = new StrAssocArray;
	DBQ  * dbq = 0;
	int    idx = 0;
	MEMSZERO(k);
	if(owner_id) {
		idx = 3;
		k.k3.PersonID = owner_id;
	}
	else {
		idx = 2;
		k.k2.SeriesID = ser_id;
	}
	BExtQuery q(P_Tbl, idx);
	THROW_MEM(p_list);
	dbq = ppcheckfiltid(dbq, P_Tbl->SeriesID, ser_id);
	dbq = ppcheckfiltid(dbq, P_Tbl->PersonID, owner_id);
	q.select(P_Tbl->ID, P_Tbl->Code, 0L).where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
		THROW_SL(p_list->AddFast(P_Tbl->data.ID, P_Tbl->data.Code));
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjSCard::IndexPhones(PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	SString phone, main_city_prefix, city_prefix, temp_buf;
	{
		PPID   main_city_id = 0;
		WorldTbl::Rec main_city_rec;
		if(GetMainCityID(&main_city_id) > 0 && LocObj.FetchCity(main_city_id, &main_city_rec) > 0)
			PPEAddr::Phone::NormalizeStr(main_city_rec.Phone, 0, main_city_prefix);
	}
	{
		UnxTextRefCore & r_utrc = PPRef->UtrC;
		TextRefEnumItem iter_item;
		PPTransaction tra(use_ta);
		THROW(tra);
		for(SEnum en = r_utrc.Enum(PPOBJ_SCARD, PPTRPROP_SCARDEXT); en.Next(&iter_item) > 0;) {
			PPGetExtStrData(PPSCardPacket::extssPhone, iter_item.S, temp_buf);
			if(temp_buf.NotEmptyS()) { // temp_buf - в кодировке UTF-8
				temp_buf.Utf8ToLower();
				PPEAddr::Phone::NormalizeStr(temp_buf, 0, phone);
				if(phone.Len() >= 5) {
					PPID   city_id = 0;
					city_prefix = main_city_prefix;
					PPObjID objid(iter_item.O);
					if(city_prefix.Len()) {
						size_t sl = phone.Len() + city_prefix.Len();
						if(oneof2(sl, 10, 11))
							phone = (temp_buf = city_prefix).Cat(phone);
					}
					if(!LocObj.P_Tbl->IndexPhone(phone, &objid, 0, 0)) {
						if(pLogger)
							pLogger->LogLastError();
						else
							CALLEXCEPT();
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
class SCardSeriesCache : public ObjCache {
public:
	SCardSeriesCache() : ObjCache(PPOBJ_SCARDSERIES, sizeof(Data))
	{
		MEMSZERO(Cfg);
	}
	int    GetConfig(PPSCardConfig * pCfg, int enforce); // @sync_w
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		LDATE  Issue;
		LDATE  Expiry;
		long   PDis;
		double MaxCredit;
		long   Flags;
		long   QuotKindID_s;
		long   PersonKindID;
		long   CrdGoodsGrpID;
		long   BonusGrpID;
		long   BonusChrgGrpID;
		long   ChargeGoodsID;
		long   ParentID;
		int16  BonusChrgExtRule;
		int16  Reserve;
	};
	PPSCardConfig Cfg;
	ReadWriteLock CfgLock;
};

int SCardSeriesCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjSCardSeries scs_obj;
	PPSCardSeries rec;
	if(scs_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(Issue);
		CPY_FLD(Expiry);
		CPY_FLD(PDis);
		CPY_FLD(MaxCredit);
		CPY_FLD(Flags);
		CPY_FLD(QuotKindID_s);
		CPY_FLD(PersonKindID);
		CPY_FLD(CrdGoodsGrpID);
		CPY_FLD(BonusGrpID);
		CPY_FLD(BonusChrgGrpID);
		CPY_FLD(ChargeGoodsID);
		CPY_FLD(BonusChrgExtRule);
		CPY_FLD(ParentID);
#undef CPY_FLD
		StringSet & r_ss = DS.AcquireRvlSsSCD();
		r_ss.add(rec.Name);
		r_ss.add(rec.Symb);
		ok = PutName(r_ss.getBuf(), p_cache_rec);
	}
	return ok;
}

void SCardSeriesCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPSCardSeries * p_data_rec = static_cast<PPSCardSeries *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(PPSCardSeries));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_SCARDSERIES;
	CPY_FLD(ID);
	CPY_FLD(Issue);
	CPY_FLD(Expiry);
	CPY_FLD(PDis);
	CPY_FLD(MaxCredit);
	CPY_FLD(Flags);
	CPY_FLD(QuotKindID_s);
	CPY_FLD(PersonKindID);
	CPY_FLD(CrdGoodsGrpID);
	CPY_FLD(BonusGrpID);
	CPY_FLD(BonusChrgGrpID);
	CPY_FLD(ChargeGoodsID);
	CPY_FLD(BonusChrgExtRule);
	CPY_FLD(ParentID);
#undef CPY_FLD
	char   temp_buf[1024];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	StringSet & r_ss = DS.AcquireRvlSsSCD();
	r_ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	r_ss.get(&p, p_data_rec->Name, sizeof(p_data_rec->Name));
	r_ss.get(&p, p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

int SCardSeriesCache::GetConfig(PPSCardConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(CfgLock, SReadWriteLocker::Read);
		if(!(Cfg.Flags & PPSCardConfig::fValid) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(Cfg.Flags & PPSCardConfig::fValid) || enforce) {
				PPObjSCard::ReadConfig(&Cfg);
				Cfg.Flags |= PPSCardConfig::fValid;
			}
		}
		ASSIGN_PTR(pCfg, Cfg);
	}
	return 1;
}

IMPL_OBJ_FETCH(PPObjSCardSeries, PPSCardSeries, SCardSeriesCache);

/*static*/int FASTCALL PPObjSCardSeries::FetchConfig(PPSCardConfig * pCfg)
{
	SCardSeriesCache * p_cache = GetDbLocalCachePtr <SCardSeriesCache> (PPOBJ_SCARDSERIES, 1);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		memzero(pCfg, sizeof(*pCfg));
		return 0;
	}
}
//
//
//
class SCardCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry { // size=44
		PPID   SeriesID;
		PPID   PersonID;
		PPID   LocID;
		long   Flags;
		LDATE  Dt;
		LDATE  Expiry;
		long   PDis;
		long   AutoGoodsID;
		double MaxCredit;
		LTIME  UsageTmStart;
		LTIME  UsageTmEnd;
		int16  PeriodTerm;
		int16  PeriodCount;
	};
	SCardCache() : ObjCacheHash(PPOBJ_SCARD, sizeof(Data), SMEGABYTE(2), 8), FullCardList(1)
	{
	}
	~SCardCache()
	{
	}
	virtual void FASTCALL Dirty(PPID id); // @sync_w
	const  StrAssocArray * GetFullList(); // @sync_w
	void   FASTCALL ReleaseFullList(const StrAssocArray * pList);
	int    FetchExtText(PPID id, SString & rBuf)
	{
		return ExtBlk.Fetch(id, rBuf, 0);
	}
	int    FetchUhttEntry(const char * pCode, PPObjSCard::UhttEntry * pEntry);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	class FclArray : public StrAssocArray {
	public:
		FclArray(int use) : StrAssocArray(), Use(use), Inited(0)
		{
		}
		void   FASTCALL Dirty(PPID cardID)
		{
			DirtyTable.Add(static_cast<uint32>(labs(cardID)));
		}
		int    Use;
		int    Inited;
		UintHashTable DirtyTable;
	};
	class SCardCache_ExtText_Block : public ObjCache::ExtTextBlock {
	private:
		virtual int Implement_Get(PPID id, SString & rBuf, void * extraPtr)
		{
			int    ok = -1;
			if(PPRef->UtrC.GetText(TextRefIdent(PPOBJ_SCARD, id, PPTRPROP_SCARDEXT), rBuf) > 0 && rBuf.NotEmpty()) {
				rBuf.Transf(CTRANSF_UTF8_TO_INNER);
				ok = 1;
			}
			return ok;
		}
	};
	FclArray FullCardList;
	ReadWriteLock FclLock;
	SCardCache_ExtText_Block ExtBlk;
	TSArray <PPObjSCard::UhttEntry> UhttList;
	ReadWriteLock UhttLock;
};

int SCardCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjSCard sc_obj;
	SCardTbl::Rec rec;
	if(id && sc_obj.Search(id, &rec) > 0) {

		#define CPY_FLD(f) p_cache_rec->f = rec.f

		CPY_FLD(SeriesID);
		CPY_FLD(PersonID);
		CPY_FLD(LocID);
		CPY_FLD(Flags);
		CPY_FLD(Dt);
		CPY_FLD(Expiry);
		CPY_FLD(PDis);
		CPY_FLD(AutoGoodsID);
		CPY_FLD(MaxCredit);
		CPY_FLD(UsageTmStart);
		CPY_FLD(UsageTmEnd);
		CPY_FLD(PeriodTerm);
		CPY_FLD(PeriodCount);

		#undef CPY_FLD

		MultTextBlock b;
		b.Add(rec.Code);
		//b.Add(rec.Password);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SCardCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	SCardTbl::Rec * p_data_rec = static_cast<SCardTbl::Rec *>(pDataRec);
	if(p_data_rec) {
		const Data * p_cache_rec = static_cast<const Data *>(pEntry);
		memzero(p_data_rec, sizeof(*p_data_rec));

		#define CPY_FLD(f) p_data_rec->f = p_cache_rec->f

		CPY_FLD(ID);
		CPY_FLD(SeriesID);
		CPY_FLD(PersonID);
		CPY_FLD(LocID);
		CPY_FLD(Flags);
		CPY_FLD(Dt);
		CPY_FLD(Expiry);
		CPY_FLD(PDis);
		CPY_FLD(AutoGoodsID);
		CPY_FLD(MaxCredit);
		CPY_FLD(UsageTmStart);
		CPY_FLD(UsageTmEnd);
		CPY_FLD(PeriodTerm);
		CPY_FLD(PeriodCount);

		#undef CPY_FLD

		MultTextBlock b(this, pEntry);
		b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
		//b.Get(p_data_rec->Password, sizeof(p_data_rec->Password));
	}
}

const StrAssocArray * SCardCache::GetFullList()
{
	int    err = 0;
	const StrAssocArray * p_result = 0;
	if(FullCardList.Use) {
		{
			SRWLOCKER(FclLock, SReadWriteLocker::Read);
			if(!FullCardList.Inited || FullCardList.DirtyTable.GetCount()) {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				if(!FullCardList.Inited || FullCardList.DirtyTable.GetCount()) {
					PPObjSCard sc_obj;
					if(!FullCardList.Inited) {
						SString msg_buf, fmt_buf;
						uint   _mc = 0;
						if(!DS.IsThreadInteractive())
							PPLoadText(PPTXT_GETTINGFULLTEXTLIST, fmt_buf);
						SCardCore * p_tbl = sc_obj.P_Tbl;
						BExtQuery q(p_tbl, 0, 24);
						q.select(p_tbl->ID, p_tbl->SeriesID, p_tbl->Code, 0L);
						FullCardList.Z();
						SCardTbl::Key0 k0;
						for(q.initIteration(false, &k0, spFirst); !err && q.nextIteration() > 0;) {
							_mc++;
							if(!FullCardList.AddFast(p_tbl->data.ID, p_tbl->data.Code)) {
								PPSetErrorSLib();
								err = 1;
							}
							else if(!DS.IsThreadInteractive()) {
								if((_mc % 1000) == 0)
									PPWaitMsg((msg_buf = fmt_buf).Space().Cat(_mc));
							}
						}
					}
					else {
						SCardTbl::Rec sc_rec;
						for(ulong id = 0; !err && FullCardList.DirtyTable.Enum(&id);) {
							if(Get((long)id, &sc_rec) > 0) { // Извлекаем наименование из кэша (из самого себя): так быстрее.
								if(!FullCardList.Add(id, sc_rec.Code, 1)) {
									PPSetErrorSLib();
									err = 1;
								}
							}
						}
					}
					if(!err) {
						FullCardList.DirtyTable.Clear();
						FullCardList.Inited = 1;
					}
				}
			}
		}
		if(!err) {
			#if SLTRACELOCKSTACK
			SLS.LockPush(SLockStack::ltRW_R, __FILE__, __LINE__);
			#endif
			FclLock.ReadLock_();
			p_result = &FullCardList;
		}
	}
	return p_result;
}

void FASTCALL SCardCache::ReleaseFullList(const StrAssocArray * pList)
{
	if(pList && pList == &FullCardList) {
		FclLock.Unlock_();
		#if SLTRACELOCKSTACK
		SLS.LockPop();
		#endif
	}
}

void FASTCALL SCardCache::Dirty(PPID id)
{
	ObjCacheHash::Dirty(id);
	ExtBlk.Dirty(id);
	{
		SRWLOCKER(FclLock, SReadWriteLocker::Write);
		FullCardList.Dirty(id);
	}
}

int SCardCache::FetchUhttEntry(const char * pCode, PPObjSCard::UhttEntry * pEntry)
{
	const long rest_actual_timeout = SlDebugMode::CT() ? 30 : 10;
	const uint max_entries = 20;
	int    ok = -1;
	uint   pos = 0;
	uint   i;
	uint   lru_pos = 0;
	LDATETIME lru_time;
	lru_time.SetFar();
	{
		SRWLOCKER(UhttLock, SReadWriteLocker::Read);
		for(i = 0; !pos && i < UhttList.getCount(); i++) {
			const PPObjSCard::UhttEntry & r_entry = UhttList.at(i);
			if(cmp(r_entry.ActualDtm, lru_time) < 0) {
				lru_time = r_entry.ActualDtm;
				lru_pos = (i+1);
			}
			if(strcmp(r_entry.Code, pCode) == 0) {
				pos = (i+1);
				const LDATETIME now_dtm = getcurdatetime_();
				if(diffdatetimesec(now_dtm, r_entry.ActualDtm) <= rest_actual_timeout) {
					ASSIGN_PTR(pEntry, r_entry);
					ok = 1;
				}
				else {
					ok = -2;
				}
			}
		}
		if(ok < 0) {
			PPUhttClient uhtt_cli;
			if(uhtt_cli.Auth()) {
				SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
				double uhtt_rest = 0.0;
				if(ok == -2) {
					assert(pos > 0);
					PPObjSCard::UhttEntry & r_entry = UhttList.at(pos-1);
					assert(r_entry.UhttCode[0] && pos > 0);
					if(uhtt_cli.GetSCardRest(r_entry.UhttCode, 0, uhtt_rest)) {
						r_entry.Rest = R2(uhtt_rest);
						r_entry.ActualDtm = getcurdatetime_();
						ASSIGN_PTR(pEntry, r_entry);
						ok = 1;
					}
					else
						ok = 0;
				}
				else {
					UhttSCardPacket scp;
					if(uhtt_cli.GetSCardByNumber(pCode, scp)) {
						PPObjSCard::UhttEntry entry;
						MEMSZERO(entry);
						STRNSCPY(entry.Code, pCode);
						scp.Code.CopyTo(entry.UhttCode, sizeof(entry.UhttCode));
						scp.Hash.CopyTo(entry.UhttHash, sizeof(entry.UhttHash));
						if(uhtt_cli.GetSCardRest(entry.UhttCode, 0, uhtt_rest)) {
							entry.Rest = R2(uhtt_rest);
							entry.ActualDtm = getcurdatetime_();
							if(UhttList.getCount() >= max_entries && lru_pos)
								UhttList.at(lru_pos) = entry;
							else
								UhttList.insert(&entry);
							ASSIGN_PTR(pEntry, entry);
							ok = 1;
						}
						else
							ok = 0;
					}
					else
						ok = 0;
				}
			}
			else
				ok = 0;
		}
	}
	return ok;
}

const StrAssocArray * PPObjSCard::GetFullList()
{
	SCardCache * p_cache = GetDbLocalCachePtr <SCardCache> (PPOBJ_SCARD);
	return p_cache ? p_cache->GetFullList() : 0;
}

void PPObjSCard::ReleaseFullList(const StrAssocArray * pList)
{
	SCardCache * p_cache = GetDbLocalCachePtr <SCardCache> (PPOBJ_SCARD);
	CALLPTRMEMB(p_cache, ReleaseFullList(pList));
}

int PPObjSCard::FetchUhttEntry(const char * pCode, PPObjSCard::UhttEntry * pEntry)
{
	SCardCache * p_cache = GetDbLocalCachePtr <SCardCache> (PPOBJ_SCARD);
	return p_cache ? p_cache->FetchUhttEntry(pCode, pEntry) : 0;
}

int PPObjSCard::FetchExtText(PPID id, int fldId, SString & rBuf)
{
	rBuf.Z();
	SCardCache * p_cache = GetDbLocalCachePtr <SCardCache> (PPOBJ_SCARD);
	if(p_cache) {
		SString & r_ext_text = SLS.AcquireRvlStr();
        if(p_cache->FetchExtText(id, r_ext_text) > 0)
			PPGetExtStrData(fldId, r_ext_text, rBuf);
	}
	return rBuf.NotEmpty() ? 1 : -1;
}

IMPL_OBJ_FETCH(PPObjSCard, SCardTbl::Rec, SCardCache);
IMPL_OBJ_DIRTY(PPObjSCard, SCardCache);
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(SCARD, PPSCardImpExpParam);

PPSCardImpExpParam::PPSCardImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
}

/*virtual*/int PPSCardImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(DefSeriesSymb.NotEmpty())
			param_list.Add(PPSCARDPAR_DEFSERIESSYMB, temp_buf = DefSeriesSymb);
		if(OwnerRegTypeCode.NotEmpty())
			param_list.Add(PPSCARDPAR_OWNERREGTYPESYMB, temp_buf = OwnerRegTypeCode);
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		DefSeriesSymb = 0;
		OwnerRegTypeCode = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case PPSCARDPAR_DEFSERIESSYMB:
					DefSeriesSymb = temp_buf;
					break;
				case PPSCARDPAR_OWNERREGTYPESYMB:
					OwnerRegTypeCode = temp_buf;
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPSCardImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name, param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	if(Direction != 0) {
		THROW(PPLoadText(PPTXT_SCARDPARAMS, params));
		if(DefSeriesSymb.NotEmpty()) {
			PPGetSubStr(params, PPSCARDPAR_DEFSERIESSYMB, fld_name);
			pFile->AppendParam(pSect, fld_name, DefSeriesSymb, 1);
		}
		if(OwnerRegTypeCode.NotEmpty()) {
			PPGetSubStr(params, PPSCARDPAR_OWNERREGTYPESYMB, fld_name);
			pFile->AppendParam(pSect, fld_name, OwnerRegTypeCode, 1);
		}
	}
	CATCHZOK
	return ok;
}

int PPSCardImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_SCARDPARAMS, params));
	if(PPGetSubStr(params, PPSCARDPAR_DEFSERIESSYMB, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			DefSeriesSymb = param_val;
	}
	if(PPGetSubStr(params, PPSCARDPAR_OWNERREGTYPESYMB, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			OwnerRegTypeCode = param_val;
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}
//
//
//
SCardImpExpDialog::SCardImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPSCARD, 0)
{
}

int SCardImpExpDialog::setDTS(const PPSCardImpExpParam * pData)
{
	int    ok = 1;
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);
	setCtrlString(CTL_IMPEXPSCARD_SERSYMB, Data.DefSeriesSymb);
	setCtrlString(CTL_IMPEXPSCARD_REGCODE, Data.OwnerRegTypeCode);
	return ok;
}

int SCardImpExpDialog::getDTS(PPSCardImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	getCtrlString(sel = CTL_IMPEXPSCARD_SERSYMB, Data.DefSeriesSymb);
	getCtrlString(sel = CTL_IMPEXPSCARD_REGCODE, Data.OwnerRegTypeCode);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int EditSCardParam(const char * pIniSection)
{
	int    ok = -1;
	SCardImpExpDialog * dlg = 0;
	PPSCardImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
   		THROW(CheckDialogPtr(&(dlg = new SCardImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_SCARD, &param.InrRec));
   		direction = param.Direction;
   		if(!isempty(pIniSection))
   			THROW(param.ReadIni(&ini_file, pIniSection, 0));
   		dlg->setDTS(&param);
   		while(ok <= 0 && ExecView(dlg) == cmOK) {
   			if(dlg->getDTS(&param)) {
   				int    is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
   				if(!isempty(pIniSection))
   					if(is_new)
   						ini_file.RemoveSection(pIniSection);
   					else
   						ini_file.ClearSection(pIniSection);
   				PPSetError(PPERR_DUPOBJNAME);
   				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
   					ok = 1;
   				else
   					PPError();
   			}
   			else
   				PPError();
		}
   	}
	CATCHZOKPPERR
   	delete dlg;
   	return ok;
}
//
//
//
class PPSCardImporter {
public:
	PPSCardImporter() : P_IE(0)
	{
	}
	~PPSCardImporter()
	{
		ZDELETE(P_IE);
	}
	int    Run(const char * pCfgName, int use_ta);
private:
	int    IdentifyOwner(const char * pName, const char * pCode, PPID kindID, PPID regTypeID, PPID * pID);

	PPSCardImpExpParam Param;
	PPImpExp * P_IE;
	PPObjSCard ScObj;
	PPObjSCardSeries ScsObj;
	PPObjPerson PsnObj;
};

static int SelectSCardImportCfgs(PPSCardImpExpParam * pParam, int import)
{
	int    ok = -1;
	int    valid_data = 0;
	uint   p = 0;
	long   id = 0;
	SString ini_file_name, sect;
	StrAssocArray list;
	PPSCardImpExpParam param;
	TDialog * p_dlg = 0;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_SCARD, &param, &list, import ? 2 : 1));
	id = (list.SearchByTextNc(pParam->Name, &p) > 0) ? (uint)list.Get(p).Id : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		#if SLTEST_RUNNING
			//
			// В режиме автоматического тестирования конфигурация выбирается автоматически по имени pParam->Name
			//
			for(int i = 1; ok < 0 && i < (int)list.getCount(); i++) {
				list.GetText(i, sect);
				if(strstr(sect, pParam->Name)) {
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					ok = 1;
				}
			}
		#endif
		while(ok < 0 && ListBoxSelDialog::Run(&list, PPTXT_TITLE_SCARDIMPCFG, &id) > 0) {
			if(id) {
				list.GetText(id, sect);
				pParam->ProcessName(1, sect);
				pParam->ReadIni(&ini_file, sect, 0);
				valid_data = ok = 1;
			}
			else
				PPError(PPERR_INVSCARDIMPEXPCFG);
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int PPSCardImporter::IdentifyOwner(const char * pName, const char * pCode, PPID kindID, PPID regTypeID, PPID * pID)
{
	int    ok = 1;
	PPID   owner_id = 0;
	PersonTbl::Rec psn_rec;
	PPIDArray temp_list;
	if(!isempty(pCode) && regTypeID) {
		if(PsnObj.GetListByRegNumber(regTypeID, kindID, pCode, temp_list) > 0) {
			if(temp_list.getCount() == 1)
				owner_id = temp_list.getSingle();
			else if(temp_list.getCount() > 1) {
				for(uint i = 0; !owner_id && i < temp_list.getCount(); i++) {
					if(PsnObj.Search(temp_list.get(i), &psn_rec) > 0 && stricmp866(psn_rec.Name, pName) == 0)
						owner_id = psn_rec.ID;
				}
			}
		}
	}
	if(!owner_id && !isempty(pName) && kindID) {
		temp_list.clear();
		temp_list.add(kindID);
		if(PsnObj.SearchFirstByName(pName, &temp_list, 0, &psn_rec) > 0)
			owner_id = psn_rec.ID;
		else {
			PPPersonPacket pack;
			pack.Rec.Status = PPPRS_PRIVATE;
			STRNSCPY(pack.Rec.Name, pName);
			pack.Kinds.add(kindID);
			if(!isempty(pCode) && regTypeID) {
				THROW(pack.AddRegister(regTypeID, pCode, 1));
			}
			THROW(PsnObj.PutPacket(&owner_id, &pack, 0));
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, owner_id);
	return ok;
}

int PPSCardImporter::Run(const char * pCfgName, int use_ta)
{
	int    ok = 1, r = 0;
	SString wait_msg, temp_buf, tok_buf;
	ZDELETE(P_IE);
	THROW(LoadSdRecord(PPREC_SCARD, &Param.InrRec));
	if(pCfgName) {
		uint p = 0;
		StrAssocArray list;
		PPSCardImpExpParam param;
		SString ini_file_name;
		Param.Name = pCfgName;
		Param.Direction = 1;
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_SCARD, &param, &list, 2));
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		SString sect;
		for(int i = 1; i < (int)list.getCount(); i++) {
			list.GetText(i, sect);
			if(strstr(sect, pCfgName)) {
				Param.ProcessName(1, sect);
				Param.ReadIni(&ini_file, sect, 0);
				r = 1;
				break;
			}
		}
	}
	else if(SelectSCardImportCfgs(&Param, 1) > 0)
		r = 1;
	if(r == 1) {
		THROW_MEM(P_IE = new PPImpExp(&Param, 0));
		{
			PPID   owner_kind_id = 0;
			PPID   owner_reg_type_id = 0;
			PPID   def_series_id = 0;
			PPIDArray psn_list;
			PPSCardSeries def_series_rec;
			PPSCardSeries scs_rec;
			MEMSZERO(def_series_rec);
			if(Param.OwnerRegTypeCode.NotEmpty())
				PPObjRegisterType::GetByCode(Param.OwnerRegTypeCode, &owner_reg_type_id);
			if(Param.DefSeriesSymb.NotEmpty()) {
				ScsObj.SearchBySymb(Param.DefSeriesSymb, &def_series_id, &def_series_rec);
			}
			if(!def_series_id) {
				ScsObj.SearchByName("default", &def_series_id, &def_series_rec);
			}
			PPWaitStart();
			PPLoadText(PPTXT_IMPSCARD, wait_msg);
			PPWaitMsg(wait_msg);
			IterCounter cntr;
			PPTransaction tra(use_ta);
			THROW(tra);
			if(P_IE->OpenFileForReading(0)) {
				PPSCardPacket sc_pack;
				long   numrecs = 0;
				P_IE->GetNumRecs(&numrecs);
				cntr.SetTotal(numrecs);
				for(uint i = 0; i < static_cast<uint>(numrecs); i++) {
					PPID   scs_id = 0;
					PPID   sc_id = 0;
					PPID   temp_id = 0;
					Sdr_SCard sdr_rec;
					sc_pack.Z();
					THROW(P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec)));
					P_IE->GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &sdr_rec);
					if(sdr_rec.SeriesSymb[0] && ScsObj.SearchBySymb(sdr_rec.SeriesSymb, &temp_id, 0) > 0)
						scs_id = temp_id;
					else if(sdr_rec.SeriesName[0] && ScsObj.SearchByName(sdr_rec.SeriesName, &temp_id, 0) > 0)
						scs_id = temp_id;
					else
						scs_id = def_series_id;
					if(sdr_rec.Code[0]) {
						if(ScObj.SearchCode(scs_id, sdr_rec.Code, &sc_pack.Rec) > 0) {
							sc_id = sc_pack.Rec.ID;
							if(!sc_pack.Rec.PersonID) {
								if(sc_pack.Rec.SeriesID && ScsObj.Fetch(sc_pack.Rec.SeriesID, &scs_rec) > 0)
									owner_kind_id = scs_rec.PersonKindID;
								SETIFZ(owner_kind_id, ScObj.GetConfig().PersonKindID);
								THROW(IdentifyOwner(sdr_rec.OwnerName, sdr_rec.OwnerCode, owner_kind_id, owner_reg_type_id, &sc_pack.Rec.PersonID));
							}
							if(sdr_rec.PercentDis > 0.0 && sdr_rec.PercentDis < 50.0) {
								sc_pack.Rec.PDis = fmul100i(sdr_rec.PercentDis);
							}
							if(sdr_rec.MaxCredit > 0.0)
								sc_pack.Rec.MaxCredit = sdr_rec.MaxCredit;
							if(checkdate(sdr_rec.Expiry))
								sc_pack.Rec.Expiry = sdr_rec.Expiry;
							{
								PPLoadString("yes", tok_buf);
								if(sdr_rec.ClosedTag[0]) {
									(temp_buf = sdr_rec.ClosedTag).Strip();
									SETFLAG(sc_pack.Rec.Flags, SCRDF_CLOSED, temp_buf == "1" || temp_buf.IsEqiAscii("yes") || temp_buf.CmpNC(tok_buf) == 0);
								}
								else if(sdr_rec.OpenedTag[0]) {
									(temp_buf = sdr_rec.OpenedTag).Strip();
									SETFLAG(sc_pack.Rec.Flags, SCRDF_CLOSED, !((temp_buf == "1" || temp_buf.IsEqiAscii("yes") || temp_buf.CmpNC(tok_buf) == 0)));
								}
							}
							THROW(ScObj.PutPacket(&sc_id, &sc_pack, 0));
						}
						else {
							STRNSCPY(sc_pack.Rec.Code, sdr_rec.Code);
							if(scs_id)
								sc_pack.Rec.SeriesID = scs_id;
							else if(sdr_rec.SeriesSymb[0] || sdr_rec.SeriesName[0]) {
								PPSCardSerPacket scs_pack;
								STRNSCPY(scs_pack.Rec.Name, sdr_rec.SeriesName[0] ? sdr_rec.SeriesName : sdr_rec.SeriesSymb);
								STRNSCPY(scs_pack.Rec.Symb, sdr_rec.SeriesSymb);
								if(sdr_rec.CardTypeTag == 1)
									scs_pack.Rec.SetType(scstCredit);
								else if(sdr_rec.CardTypeTag == 2)
									scs_pack.Rec.SetType(scstBonus);
								else
									scs_pack.Rec.SetType(scstDiscount);
								if(checkdate(sdr_rec.IssueDate))
									scs_pack.Rec.Issue = sdr_rec.IssueDate;
								THROW(ScsObj.PutPacket(&scs_id, &scs_pack, 0));
							}
							sc_pack.Rec.SeriesID = scs_id;
							if(sc_pack.Rec.SeriesID) {
								if(sc_pack.Rec.SeriesID && ScsObj.Fetch(sc_pack.Rec.SeriesID, &scs_rec) > 0)
									owner_kind_id = scs_rec.PersonKindID;
								SETIFZ(owner_kind_id, ScObj.GetConfig().PersonKindID);
								THROW(IdentifyOwner(sdr_rec.OwnerName, sdr_rec.OwnerCode, owner_kind_id, owner_reg_type_id, &sc_pack.Rec.PersonID));
								if(sdr_rec.PercentDis > 0.0 && sdr_rec.PercentDis < 50.0) {
									sc_pack.Rec.PDis = fmul100i(sdr_rec.PercentDis);
								}
								if(sdr_rec.MaxCredit > 0.0)
									sc_pack.Rec.MaxCredit = sdr_rec.MaxCredit;
								if(checkdate(sdr_rec.Expiry))
									sc_pack.Rec.Expiry = sdr_rec.Expiry;
								{
									PPLoadString("yes", tok_buf);
									if(sdr_rec.ClosedTag[0]) {
										(temp_buf = sdr_rec.ClosedTag).Strip();
										SETFLAG(sc_pack.Rec.Flags, SCRDF_CLOSED, (temp_buf == "1" || temp_buf.IsEqiAscii("yes") || temp_buf.CmpNC(tok_buf) == 0));
									}
									else if(sdr_rec.OpenedTag[0]) {
										(temp_buf = sdr_rec.OpenedTag).Strip();
										SETFLAG(sc_pack.Rec.Flags, SCRDF_CLOSED, !(temp_buf == "1" || temp_buf.IsEqiAscii("yes") || temp_buf.CmpNC(tok_buf) == 0));
									}
								}
								THROW(ScObj.PutPacket(&sc_id, &sc_pack, 0));
							}
						}
					}
					PPWaitPercent(cntr.Increment(), wait_msg);
				}
			}
			PPWaitStop();
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int ImportSCard()
{
	PPSCardImporter prcssr;
	return prcssr.Run(0, 1) ? 1 : PPErrorZ();
}
//
//
//
int ConvertSCardSeries9809()
{
	struct SCardSeries_Before9809 {
		long   Tag;
		long   ID;
		char   Name[48];
		char   Symb[20];
		PPID   ChargeGoodsID;
		PPID   BonusChrgGrpID;
		int16  BonusChrgExtRule;
		uint8  Reserve2;
		int8   VerifTag;
		PPID   BonusGrpID;
		PPID   CrdGoodsGrpID;
		char   CodeTempl[20];
		LDATE  Issue;
		LDATE  Expiry;
		long   PDis;
		double MaxCredit;
		long   Flags;
		long   QuotKindID_s;
		long   PersonKindID;
	};
	STATIC_ASSERT(sizeof(PPSCardSeries2) == sizeof(SCardSeries_Before9809));
	int    ok = -1;
	LDATETIME moment;
	PPIDArray acn_list;
	acn_list.add(PPACN_EVENTTOKEN);
	Reference * p_ref = 0;
    SysJournal * p_sj = new SysJournal;
    THROW(p_sj);
    if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_CVTSCS9809, &acn_list, &moment) > 0) {
    	ok = -1; // Конвертация уже сделана
    }
    else {
		const  PPID obj_type = PPOBJ_SCARDSERIES;
		Reference2Tbl::Key0 k0;
        PPSCardSeries2 scs_rec;
		THROW(p_ref = new Reference);
		MEMSZERO(k0);
		{
			PPTransaction tra(1);
			THROW(tra);
			k0.ObjType = obj_type;
			if(p_ref->search(0, &k0, spGt)) do {
				p_ref->CopyBufTo(&scs_rec);
				if(scs_rec.VerifTag < 2) {
					const  PPID _id = scs_rec.ID;
					SCardSeries_Before9809 old_rec;
					char    code_templ[20];
					memcpy(&old_rec, &scs_rec, sizeof(old_rec));
					memzero(code_templ, sizeof(code_templ));
					STRNSCPY(code_templ, old_rec.CodeTempl);
                    scs_rec.QuotKindID_s = old_rec.QuotKindID_s;
                    scs_rec.PersonKindID = old_rec.PersonKindID;
                    scs_rec.ParentID = 0;
                    scs_rec.Reserve4 = 0;

					const long flags_mask = 0x03ff;
					scs_rec.Flags &= flags_mask;
					scs_rec.FixedBonus = 0;
                    scs_rec.VerifTag = 2;
                    p_ref->CopyBufFrom(&scs_rec);
                    THROW_DB(p_ref->updateRec());
                    if(code_templ[0]) {
						PPSCardSerPacket::Ext ext;
						Storage_SCardSerExt se;
						int    se_r = 0;
						THROW(se_r = p_ref->GetProperty(obj_type, _id, SCARDSERIES_EXT, &se, sizeof(se)));
						if(se_r > 0)
							se.Get(ext);
                        STRNSCPY(ext.CodeTempl, code_templ);
                        se.Set(ext);
						THROW(p_ref->PutProp(obj_type, _id, SCARDSERIES_EXT, &se, sizeof(se), 0));
                    }
				}
			} while(p_ref->search(0, &k0, spNext) && p_ref->data.ObjType == obj_type);
			THROW(p_sj->LogEvent(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, PPEVTOK_CVTSCS9809, 0, 0));
			THROW(tra.Commit());
		}
    }
    CATCHZOK
	delete p_ref;
	delete p_sj;
	return ok;
}
