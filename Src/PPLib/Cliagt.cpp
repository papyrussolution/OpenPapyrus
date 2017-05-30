// CLIAGT.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// Соглашения с клиентами об условиях торговли
//
#include <pp.h>
#pragma hdrstop
// @v9.6.2 (moved to pp.h) #include <ppidata.h>

SLAPI PPClientAgreement::PPClientAgreement()
{
	Init();
}

SLAPI PPClientAgreement::PPClientAgreement(const PPClientAgreement & rSrc)
{
	Init();
	memcpy(this, &rSrc, offsetof(PPClientAgreement, DebtLimList));
	DebtLimList = rSrc.DebtLimList;
}

int SLAPI PPClientAgreement::Init()
{
	memzero(this, offsetof(PPClientAgreement, DebtLimList));
	DebtLimList.freeAll();
	return 1;
}

int FASTCALL PPClientAgreement::IsEqual(const PPClientAgreement & rS) const
{
#define CMP_FLD(f) if((f) != (rS.f)) return 0
	CMP_FLD(Flags);
	CMP_FLD(BegDt);
	CMP_FLD(Expiry);
	CMP_FLD(MaxCredit);
	CMP_FLD(MaxDscnt);
	CMP_FLD(Dscnt);
	CMP_FLD(DefPayPeriod);
	CMP_FLD(PriceRoundDir);
	CMP_FLD(DefAgentID);
	CMP_FLD(DefQuotKindID);
	CMP_FLD(ExtObjectID);
	CMP_FLD(LockPrcBefore);
	CMP_FLD(PriceRoundPrec);
	CMP_FLD(RetLimPrd);
	CMP_FLD(RetLimPart);
	CMP_FLD(PaymDateBase);
#undef CMP_FLD
	if(strcmp(Code, rS.Code) != 0)
		return 0;
	else {
		const uint _c1 = DebtLimList.getCount();
		const uint _c2 = rS.DebtLimList.getCount();
		if(_c1 != _c2)
			return 0;
		else if(_c1) {
			for(uint i = 0; i < _c1; i++) {
				const DebtLimit & r_e1 = DebtLimList.at(i);
				const DebtLimit & r_e2 = rS.DebtLimList.at(i);
				if(memcmp(&r_e1, &r_e2, sizeof(r_e1)) != 0)
					return 0;
			}
		}
	}
	return 1;
}

int SLAPI PPClientAgreement::IsEmpty() const
{
	const long nempty_flags_mask = (AGTF_DONTCALCDEBTINBILL|AGTF_PRICEROUNDING);
	return ((Flags & nempty_flags_mask) || BegDt || Expiry || MaxCredit || MaxDscnt || Dscnt || DefPayPeriod ||
		DefAgentID || DefQuotKindID || ExtObjectID || LockPrcBefore || strlen(Code) > 0 || DebtLimList.getCount() ||
		(RetLimPrd && RetLimPart)) ? 0 : 1;
}

PPClientAgreement & FASTCALL PPClientAgreement::operator = (const PPClientAgreement & rSrc)
{
	memcpy(this, &rSrc, offsetof(PPClientAgreement, DebtLimList));
	DebtLimList = rSrc.DebtLimList;
	return *this;
}

struct DebtLimit_Before715 {
	PPID   DebtDimID;
	double Limit;
	long   Flags;
};

int SLAPI PPClientAgreement::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint8  ind = 0;
	// @v7.2.0 SArray temp_list(sizeof(DebtLimit_Before715)); // @v7.1.5
	if(dir > 0) {
		if(IsEmpty())
			ind = 1;
		rBuf.Write(ind);
		if(ind == 0) {
			THROW_SL(pCtx->SerializeBlock(dir, offsetof(PPClientAgreement, DebtLimList), this, rBuf, 0));
			THROW_SL(pCtx->Serialize(dir, &DebtLimList, rBuf)); // @v7.2.0
#if 0 // @v7.2.0 {
			// @v7.1.5 {
			for(uint i = 0; i < DebtLimList.getCount(); i++) {
				DebtLimit & r_dd_item = DebtLimList.at(i);
				DebtLimit_Before715 item;
				item.DebtDimID = r_dd_item.DebtDimID;
				item.Limit = r_dd_item.Limit;
				item.Flags = r_dd_item.Flags;
				THROW_SL(temp_list.insert(&item));
			}
			THROW_SL(pCtx->Serialize(dir, &temp_list, rBuf));
			// } @v7.1.5
			// @v7.1.5 THROW_SL(pCtx->Serialize(dir, &DebtLimList, rBuf));
#endif // } 0 @v7.2.0
		}
	}
	else if(dir < 0) {
		rBuf.Read(ind);
		if(ind == 0) {
			THROW_SL(pCtx->SerializeBlock(dir, offsetof(PPClientAgreement, DebtLimList), this, rBuf, 0));
			THROW_SL(pCtx->Serialize(dir, &DebtLimList, rBuf)); // @v7.2.0
#if 0 // @v7.2.0 {
			// @v7.1.5 THROW_SL(pCtx->Serialize(dir, &DebtLimList, rBuf));
			// @v7.1.5 {
			THROW_SL(pCtx->Serialize(dir, &temp_list, rBuf));
			DebtLimList.clear();
			for(uint i = 0; i < temp_list.getCount(); i++) {
				DebtLimit_Before715 & r_item = *(DebtLimit_Before715 *)temp_list.at(i);
				DebtLimit dd_item;
				dd_item.DebtDimID = r_item.DebtDimID;
				dd_item.Limit = r_item.Limit;
				dd_item.Flags = r_item.Flags;
				dd_item.LockPrcBefore = ZERODATE;
				THROW_SL(DebtLimList.insert(&dd_item));
			}
			// } @v7.1.5
#endif // } 0 @v7.2.0
		}
		else {
			Init();
		}
	}
	CATCHZOK
	return ok;
}

double SLAPI PPClientAgreement::GetCreditLimit(PPID debtDimID) const
{
	double limit = 0.0;
	int    has_dd_item = 0;
	if(debtDimID) {
		for(uint i = 0; !has_dd_item && i < DebtLimList.getCount(); i++) {
			if(DebtLimList.at(i).DebtDimID == debtDimID) {
				limit = DebtLimList.at(i).Limit;
				has_dd_item = 1;
			}
		}
	}
	return has_dd_item ? limit : MaxCredit;
}

int FASTCALL PPClientAgreement::IsStopped(PPID debtDimID) const
{
	DebtLimit * p_item = GetDebtDimEntry(debtDimID);
	return p_item ? BIN(p_item->Flags & DebtLimit::fStop) : -1;
}

PPClientAgreement::DebtLimit * FASTCALL PPClientAgreement::GetDebtDimEntry(PPID debtDimID) const
{
	DebtLimit * p_item = 0;
	if(debtDimID)
		for(uint i = 0; !p_item && i < DebtLimList.getCount(); i++)
			if(DebtLimList.at(i).DebtDimID == debtDimID)
				p_item = &DebtLimList.at(i);
	return p_item;
}

struct _PPClientAgt {      // @persistent @store(PropertyTbl) @#{size=PROPRECFIXSIZE}
	long   Tag;            // Const=PPOBJ_ARTICLE
	long   ArtID;          // ->Article.ID
	long   PropID;         // Const=ARTPRP_CLIAGT
	long   Flags;          //
	LDATE  BegDt;          //
	LDATE  Expiry;         //
	double MaxCredit;      // Максимальный кредит
	double MaxDscnt;       // Максимальная скидка в %% (>= 100% - неограниченная)
	double Dscnt;          // Обычная скидка в %%
	short  DefPayPeriod;   // Количество дней от отгрузки до оплаты по умолчанию
	PPID   DefAgentID;     // Агент, закрепленный за клиентом
	PPID   DefQuotKindID;  // Вид котировки, используемый для отгрузки этому клиенту
	char   Code[12];       // @v5.7.12 Номер соглашения //
	PPID   ExtObjectID;    // @v5.9.3 Дополнительный объект (таблица дополнительных объектов для общего соглашения)
	LDATE  LockPrcBefore;  // @v6.0.7 Дата, до которой процессинг должников не меняет параметры соглашения //
	int16  PriceRoundDir;  // @v6.4.1 Направление округления окончательной цены в документах
	float  PriceRoundPrec; // @v6.4.1 Точность округления окончательной цены в документах
	int16  RetLimPrd;      // @v7.1.5 Период ограничения доли возвратов от суммы товарооборота
	uint16 RetLimPart;     // @v7.1.5 Макс доля возвратов от суммы товарооборота за период RetLimPrd (в промилле)
	long   PaymDateBase;   // @v8.4.2
};

//static
int SLAPI PPObjArticle::PropToClientAgt(const PropertyTbl::Rec * pPropRec, PPClientAgreement * pAgt, int loadDebtLimList /*=0*/)
{
	int    ok = 1;
	const _PPClientAgt * p_agt = (const _PPClientAgt *)pPropRec;
	pAgt->ClientID  = p_agt->ArtID;
	pAgt->Flags     = (p_agt->Flags | AGTF_LOADED);
	pAgt->BegDt     = p_agt->BegDt;
	pAgt->Expiry    = p_agt->Expiry;
	pAgt->MaxCredit = p_agt->MaxCredit;
	pAgt->MaxDscnt  = p_agt->MaxDscnt;
	pAgt->Dscnt     = p_agt->Dscnt;
	pAgt->DefPayPeriod  = p_agt->DefPayPeriod;
	pAgt->DefAgentID    = p_agt->DefAgentID;
	pAgt->DefQuotKindID = p_agt->DefQuotKindID;
	pAgt->ExtObjectID   = p_agt->ExtObjectID;
	STRNSCPY(pAgt->Code, p_agt->Code);
	pAgt->LockPrcBefore  = p_agt->LockPrcBefore;
	pAgt->PriceRoundDir  = p_agt->PriceRoundDir;
	pAgt->PriceRoundPrec = p_agt->PriceRoundPrec;
	pAgt->RetLimPrd  = p_agt->RetLimPrd;
	pAgt->RetLimPart = p_agt->RetLimPart;
	pAgt->PaymDateBase = p_agt->PaymDateBase; // @v8.4.2
	if(loadDebtLimList) {
		Reference * p_ref = PPRef;
		if(pAgt->Flags & AGTF_DDLIST715) {
			p_ref->GetPropArray(PPOBJ_ARTICLE, pAgt->ClientID, ARTPRP_DEBTLIMLIST2, &pAgt->DebtLimList);
		}
		else {
			SArray temp_list(sizeof(DebtLimit_Before715));
			p_ref->GetPropArray(PPOBJ_ARTICLE, pAgt->ClientID, ARTPRP_DEBTLIMLIST, &temp_list);
			pAgt->DebtLimList.clear();
			for(uint i = 0; i < temp_list.getCount(); i++) {
				DebtLimit_Before715 & r_item = *(DebtLimit_Before715 *)temp_list.at(i);
				PPClientAgreement::DebtLimit dd_item;
				dd_item.DebtDimID = r_item.DebtDimID;
				dd_item.Limit = r_item.Limit;
				dd_item.Flags = r_item.Flags;
				dd_item.LockPrcBefore = ZERODATE;
				THROW_SL(pAgt->DebtLimList.insert(&dd_item));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjArticle::HasClientAgreement(PPID id)
{
	int    yes = 0;
	if(id > 0) {
		PropertyTbl::Rec prop_rec;
		PPClientAgreement agt;
		MEMSZERO(prop_rec);
		if(PPRef->GetProp(PPOBJ_ARTICLE, id, ARTPRP_CLIAGT, &prop_rec, sizeof(prop_rec)) > 0) {
			agt.Init();
			PropToClientAgt(&prop_rec, &agt, 0);
			agt.ClientID = id;
			yes = BIN(!agt.IsEmpty());
		}
	}
	return yes;
}

int SLAPI PPObjArticle::GetClientAgreement(PPID id, PPClientAgreement * pAgt, int use_default)
{
	int    ok = 1, r, is_default = 0;
	int    r2 = 0;
	Reference * p_ref = PPRef;
	PropertyTbl::Rec prop_rec, def_prop_rec;
	PPClientAgreement def_agt;
	PPPersonRelTypePacket rt_pack;
	PPIDArray rel_list;
	MEMSZERO(prop_rec);
	THROW(r = p_ref->GetProp(PPOBJ_ARTICLE, id, ARTPRP_CLIAGT, &prop_rec, sizeof(prop_rec)));
	if(r < 0 && id) {
		PPID   mainorg_id = 0, mainorg_arid = 0;
		//PPPersonRelType relt_rec;
		GetMainOrgID(&mainorg_id);
		P_Tbl->PersonToArticle(mainorg_id, GetSellAccSheet(), &mainorg_arid);
		// @v8.2.2 if(id != mainorg_arid && ObjRelTyp.Search(PPPSNRELTYP_AFFIL, &relt_rec) > 0 && (relt_rec.Flags & PPPersonRelType::fInhMainOrgAgreement)) {
		if(id != mainorg_arid && ObjRelTyp.Fetch(PPPSNRELTYP_AFFIL, &rt_pack) > 0 && (rt_pack.Rec.Flags & PPPersonRelType::fInhMainOrgAgreement)) { // @v8.2.2
			if(GetRelPersonList(id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0 && rel_list.getCount() && rel_list.lsearch(mainorg_arid) > 0) {
				THROW(r = p_ref->GetProp(PPOBJ_ARTICLE, mainorg_arid, ARTPRP_CLIAGT, &prop_rec, sizeof(prop_rec)));
			}
		}
	}
	// @v8.2.2 {
	if(r < 0 && id) {
		if(ObjRelTyp.Fetch(PPPSNRELTYP_AFFIL, &rt_pack) > 0 && (rt_pack.Rec.Flags & PPPersonRelType::fInhAgreements)) {
			if(GetRelPersonList(id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0) {
				for(uint i = 0; r < 0 && i < rel_list.getCount(); i++) {
					PPID rel_ar_id = ObjectToPerson(rel_list.get(i), 0);
					THROW(r = p_ref->GetProp(PPOBJ_ARTICLE, rel_ar_id, ARTPRP_CLIAGT, &prop_rec, sizeof(prop_rec)));
				}
			}
		}
	}
	// } @v8.2.2
	if(use_default) {
		THROW(r2 = p_ref->GetProp(PPOBJ_ARTICLE, 0, ARTPRP_CLIAGT, &def_prop_rec, sizeof(def_prop_rec)));
		if(r2 > 0)
			PropToClientAgt(&def_prop_rec, &def_agt, 0);
		if(r < 0 && id) {
			ok = 2;
			if(r2 > 0) {
				is_default = 1;
				prop_rec = def_prop_rec;
				r = 1;
			}
		}
	}
	if(r > 0) {
		pAgt->Init();
		PropToClientAgt(&prop_rec, pAgt, 1);
		pAgt->ClientID = id;
		if(is_default)
			pAgt->Flags |= AGTF_DEFAULT;
		// @v7.1.9 {
		else if(r2 > 0) {
			if(pAgt->RetLimPrd == 0 && pAgt->RetLimPart == 0) {
				pAgt->RetLimPrd = def_agt.RetLimPrd;
				pAgt->RetLimPart = def_agt.RetLimPart;
			}
		}
		// } @v7.1.9
	}
	else {
		pAgt->Flags |= AGTF_LOADED;
		ok = -1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjArticle::PutClientAgreement(PPID id, PPClientAgreement * pAgt, int use_ta)
{
	int    ok = 1;
	_PPClientAgt _agt;
	THROW(CheckRights(ARTRT_CLIAGT));
	if(pAgt) {
		MEMSZERO(_agt);
		_agt.Tag          = PPOBJ_ARTICLE;
		_agt.ArtID        = id;
		_agt.PropID       = ARTPRP_CLIAGT;
		_agt.Flags        = ((pAgt->Flags & ~AGTF_LOADED) | AGTF_DDLIST715);
		_agt.BegDt        = pAgt->BegDt;
		_agt.Expiry       = pAgt->Expiry;
		_agt.MaxCredit    = pAgt->MaxCredit;
		_agt.MaxDscnt     = pAgt->MaxDscnt;
		_agt.Dscnt        = pAgt->Dscnt;
		_agt.DefPayPeriod = pAgt->DefPayPeriod;
		_agt.DefAgentID   = pAgt->DefAgentID;
		_agt.DefQuotKindID = pAgt->DefQuotKindID;
		_agt.ExtObjectID    = pAgt->ExtObjectID;
		_agt.LockPrcBefore  = pAgt->LockPrcBefore;
		_agt.PriceRoundDir  = pAgt->PriceRoundDir;
		_agt.PriceRoundPrec = pAgt->PriceRoundPrec;
		_agt.RetLimPrd    = pAgt->RetLimPrd;
		_agt.RetLimPart   = pAgt->RetLimPart;
		_agt.PaymDateBase = pAgt->PaymDateBase; // @v8.4.2
		STRNSCPY(_agt.Code, pAgt->Code);
	}
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(p_ref->PutProp(PPOBJ_ARTICLE, id, ARTPRP_CLIAGT, (pAgt ? &_agt : 0), sizeof(_agt), 0));
		THROW(p_ref->PutPropArray(PPOBJ_ARTICLE, id, ARTPRP_DEBTLIMLIST, 0, 0)); // @temp(..1/06/2012)
		THROW(p_ref->PutPropArray(PPOBJ_ARTICLE, id, ARTPRP_DEBTLIMLIST2, (pAgt ? &pAgt->DebtLimList : 0), 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
class AgtDialog : public TDialog {
public:
	AgtDialog(uint dlgID) : TDialog(dlgID)
	{
		ArID = 0;
	}
protected:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmBills)) {
			if(ArID) {
				BillFilt flt;
				flt.ObjectID = ArID;
				flt.Flags   |= BillFilt::fShowDebt;
				ViewGoodsBills(&flt, 1);
			}
			clearEvent(event);
		}
	}
	PPID   ArID;
};
//
//
//
class DebtLimListDialog : public PPListDialog {
public:
	DebtLimListDialog() : PPListDialog(DLG_DEBTLIMLIST, CTL_DEBTLIMLIST_LIST)
	{
		PPObjDebtDim dd_obj;
		P_DebtDimList = dd_obj.MakeStrAssocList(0);
	}
	~DebtLimListDialog()
	{
		ZDELETE(P_DebtDimList);
	}
	int    setDTS(const TSArray <PPClientAgreement::DebtLimit> * pData)
	{
		//long    id = 0;
		if(pData)
			Data.copy(*pData);
		else
			Data.freeAll();
		updateList(-1);
		if(P_DebtDimList)
			for(uint i = 0; i < Data.getCount(); i++)
				P_DebtDimList->Remove(Data.at(i).DebtDimID);
		return 1;
	}
	int    getDTS(TSArray <PPClientAgreement::DebtLimit> * pData)
	{
		int    ok = 1;
		CALLPTRMEMB(pData, copy(Data));
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int setupList();
	int    Edit(PPClientAgreement::DebtLimit * pItem);
	StrAssocArray * P_DebtDimList;
	TSArray <PPClientAgreement::DebtLimit> Data;
};

IMPL_HANDLE_EVENT(DebtLimListDialog)
{
	PPListDialog::handleEvent(event);
	if(TVKEYDOWN && TVKEY == kbSpace) {
		uint pos = 0;
		long id = 0L;
		getSelection(&id);
		if(id > 0 && Data.lsearch(&id, &pos, PTR_CMPFUNC(long)) > 0) {
			long fstop = PPClientAgreement::DebtLimit::fStop;
			long flags = Data.at(pos).Flags;
			SETFLAG(Data.at(pos).Flags, fstop, !(flags & fstop));
			updateList(pos);
		}
		clearEvent(event);
	}
}

int DebtLimListDialog::setupList()
{
	int    ok = -1;
	for(uint i = 0; i < Data.getCount(); i++) {
		StringSet ss(SLBColumnDelim);
		SString temp_buf;
		PPClientAgreement::DebtLimit debt_lim = Data.at(i);
		GetObjectName(PPOBJ_DEBTDIM, debt_lim.DebtDimID, temp_buf);
		ss.add(temp_buf, 0);
		(temp_buf = 0).Cat(debt_lim.Limit, SFMT_MONEY);
		ss.add(temp_buf, 0);
		(temp_buf = 0).CatChar((debt_lim.Flags & PPClientAgreement::DebtLimit::fStop) ? 'X' : ' ');
		ss.add(temp_buf, 0);
		if(!addStringToList(debt_lim.DebtDimID, ss.getBuf()))
			ok = PPErrorZ();
	}
	return ok;
}

class DebtLimItemDialog : public TDialog {
public:
	DebtLimItemDialog(StrAssocArray * pDebtDimList) : TDialog(DLG_DBTLIMITEM)
	{
		P_DebtDimList = pDebtDimList;
		SetupCalDate(CTLCAL_DBTLIMITEM_LOCKPRCB, CTL_DBTLIMITEM_LOCKPRCB);
	}
	int    setDTS(const PPClientAgreement::DebtLimit * pData)
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		if(P_DebtDimList)
			SetupStrAssocCombo(this, CTLSEL_DBTLIMITEM_DIM, P_DebtDimList, Data.DebtDimID, 0, 0);
		else
			SetupPPObjCombo(this, CTLSEL_DBTLIMITEM_DIM, PPOBJ_DEBTDIM, Data.DebtDimID, 0, 0);
		setCtrlData(CTL_DBTLIMITEM_LIMIT, &Data.Limit);
		AddClusterAssoc(CTL_DBTLIMITEM_FLAGS, 0, PPClientAgreement::DebtLimit::fStop);
		SetClusterData(CTL_DBTLIMITEM_FLAGS, Data.Flags);
		setCtrlDate(CTL_DBTLIMITEM_LOCKPRCB, Data.LockPrcBefore);
		return 1;
	}
	int    getDTS(PPClientAgreement::DebtLimit * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTLSEL_DBTLIMITEM_DIM, &Data.DebtDimID);
		THROW_PP(Data.DebtDimID, PPERR_USERINPUT);
		getCtrlData(CTL_DBTLIMITEM_LIMIT, &Data.Limit);
		GetClusterData(CTL_DBTLIMITEM_FLAGS, &Data.Flags);
		Data.LockPrcBefore = getCtrlDate(sel = CTL_DBTLIMITEM_LOCKPRCB);
		THROW_SL(checkdate(Data.LockPrcBefore, 1));
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	StrAssocArray * P_DebtDimList;
	PPClientAgreement::DebtLimit Data;
};

int DebtLimListDialog::Edit(PPClientAgreement::DebtLimit * pItem)
{
	int    ok = -1;
	DebtLimItemDialog * dlg = new DebtLimItemDialog(P_DebtDimList);
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(pItem);
	for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		if(dlg->getDTS(pItem) > 0) {
			if(P_DebtDimList && pItem)
				P_DebtDimList->Remove(pItem->DebtDimID);
			ok = valid_data = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int DebtLimListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long   pos = -1, id = 0;
	PPClientAgreement::DebtLimit debt_lim;
	MEMSZERO(debt_lim);
	if(Edit(&debt_lim) > 0) {
		Data.insert(&debt_lim);
		pos = Data.getCount() - 1;
		id = debt_lim.DebtDimID;
		ok = 1;
	}
	ASSIGN_PTR(pID, id);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int DebtLimListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.getCount()) {
		SString dim_name;
		PPClientAgreement::DebtLimit debt_lim = Data.at(pos);
		GetObjectName(PPOBJ_DEBTDIM, id, dim_name);
		P_DebtDimList->Add(id, dim_name);
		if(Edit(&debt_lim) > 0) {
			Data.at(pos) = debt_lim;
			ok = 1;
		}
	}
	return ok;
}

int DebtLimListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(pos >= 0 && pos < (long)Data.getCount()) {
		ok = Data.atFree(pos);
		if(ok > 0) {
			SString dim_name;
			GetObjectName(PPOBJ_DEBTDIM, id, dim_name);
			P_DebtDimList->Add(id, dim_name);
		}
	}
	return ok;
}

int SLAPI EditDebtLimList(PPClientAgreement & rCliAgt)
{
	DIALOG_PROC_BODY(DebtLimListDialog, &rCliAgt.DebtLimList);
}

int SLAPI SetupPaymDateBaseCombo(TDialog * pDlg, uint ctlID, long initVal)
{
	SString buf, id_buf, txt_buf;
	StrAssocArray ary;
	PPLoadText(PPTXT_PAYMDATEBASE, buf);
	StringSet ss(';', buf);
	for(uint i = 0; ss.get(&i, buf) > 0;)
		if(buf.Divide(',', id_buf, txt_buf) > 0)
			ary.Add(id_buf.ToLong(), txt_buf);
    {
    	PPObjTag tag_obj;
    	PPObjectTag tag_rec;
		for(SEnum en = PPRef->Enum(PPOBJ_TAG, 0); en.Next(&tag_rec) > 0;) {
			if(tag_rec.ID > PPClientAgreement::pdbTagBias && tag_rec.ObjTypeID == PPOBJ_BILL && oneof2(tag_rec.TagDataType, OTTYP_DATE, OTTYP_TIMESTAMP)) {
				(buf = 0).Cat("Tag").CatDiv(':', 2).Cat(tag_rec.Name);
				ary.Add(tag_rec.ID, 0, buf);
			}
		}
    }
	return SetupStrAssocCombo(pDlg, ctlID, &ary, initVal, 0);
}

int SLAPI PPObjArticle::EditClientAgreement(PPClientAgreement * agt)
{
	class CliAgtDialog : public AgtDialog {
	public:
		CliAgtDialog() : AgtDialog(DLG_CLIAGT)
		{
			SetupCalDate(CTLCAL_CLIAGT_DATE, CTL_CLIAGT_DATE);
			SetupCalDate(CTLCAL_CLIAGT_EXPIRY, CTL_CLIAGT_EXPIRY);
			SetupCalDate(CTLCAL_CLIAGT_LOCKPRCBEFORE, CTL_CLIAGT_LOCKPRCBEFORE);
			enableCommand(cmOK, ArObj.CheckRights(ARTRT_CLIAGT));
		}
		int  setDTS(const PPClientAgreement * pAgt)
		{
			SString ar_name;
			double added_limit_val = 0.0; // @v8.2.4
			int    added_limit_term = 0;  // @v8.2.4
			data = *pAgt;
			ArID = data.ClientID;
			GetArticleName(data.ClientID, ar_name);
			setCtrlString(CTL_CLIAGT_CLIENT, ar_name);
			setCtrlReadOnly(CTL_CLIAGT_CLIENT, 1);
			setCtrlReadOnly(CTL_CLIAGT_CURDEBT, 1);
			if(data.ClientID) {
				DateRange cdp;
				PPObjBill::DebtBlock blk;
				BillObj->CalcClientDebt(data.ClientID, BillObj->GetDefaultClientDebtPeriod(cdp), 0, blk);
				setCtrlReal(CTL_CLIAGT_CURDEBT, blk.Debt);
			}
			else
				enableCommand(cmBills, 0);
			setCtrlData(CTL_CLIAGT_CODE, data.Code);
			setCtrlDate(CTL_CLIAGT_DATE,      data.BegDt);
			setCtrlDate(CTL_CLIAGT_EXPIRY,    data.Expiry);
			setCtrlDate(CTL_CLIAGT_LOCKPRCBEFORE, data.LockPrcBefore);
			setCtrlData(CTL_CLIAGT_MAXCREDIT, &data.MaxCredit);
			setCtrlData(CTL_CLIAGT_MAXDSCNT,  &data.MaxDscnt);
			setCtrlData(CTL_CLIAGT_DSCNT,     &data.Dscnt);
			setCtrlData(CTL_CLIAGT_PAYPERIOD, &data.DefPayPeriod);
			SetupPaymDateBaseCombo(this, CTLSEL_CLIAGT_PAYMDTBASE, data.PaymDateBase); // @v8.4.2
			SetupArCombo(this, CTLSEL_CLIAGT_AGENT, data.DefAgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet|sacfNonGeneric);
			SetupPPObjCombo(this, CTLSEL_CLIAGT_QUOTKIND, PPOBJ_QUOTKIND, data.DefQuotKindID, 0, 0);
			if(data.ClientID) {
				PPClientAgreement agt;
				const PPID acs_id = (ArObj.GetClientAgreement(0, &agt) > 0) ? agt.ExtObjectID : 0;
				SString  ext_obj;
				// @v9.1.4 PPGetWord(PPWORD_EXTOBJECT, 0, ext_obj);
				PPLoadString("bill_object2", ext_obj); // @v9.1.4
				setLabelText(CTL_CLIAGT_EXTOBJECT, ext_obj);
				SetupArCombo(this, CTLSEL_CLIAGT_EXTOBJECT, data.ExtObjectID, OLW_LOADDEFONOPEN|OLW_CANINSERT, acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
				AddClusterAssoc(CTL_CLIAGT_FLAGS, 0, AGTF_DONTCALCDEBTINBILL);
				AddClusterAssoc(CTL_CLIAGT_FLAGS, 1, AGTF_USEMARKEDGOODSONLY);
				AddClusterAssoc(CTL_CLIAGT_FLAGS, 2, AGTF_DONTUSEMINSHIPMQTTY); // @v8.4.4
				SetClusterData(CTL_CLIAGT_FLAGS, data.Flags);
				// @v8.2.4 {
				if(data.MaxCredit != 0.0) {
					PPDebtorStatConfig ds_cfg;
					if(PPDebtorStatConfig::Read(&ds_cfg) > 0 && ds_cfg.LimitAddedTerm > 0 && ds_cfg.LimitTerm > 0) {
						added_limit_term = ds_cfg.LimitAddedTerm;
						added_limit_val = R0((data.MaxCredit / ds_cfg.LimitTerm) * added_limit_term);
					}
				}
				// } @v8.2.4
			}
			else
				SetupPPObjCombo(this, CTLSEL_CLIAGT_EXTOBJECT, PPOBJ_ACCSHEET, data.ExtObjectID, 0, 0);
			// @v8.2.4 {
			if(added_limit_val != 0.0) {
				showCtrl(CTL_CLIAGT_ADDEDLIMITVAL, 1);
				SString fmt_buf, label_buf;
				getLabelText(CTL_CLIAGT_ADDEDLIMITVAL, fmt_buf);
				label_buf.Printf(fmt_buf, added_limit_term);
				setLabelText(CTL_CLIAGT_ADDEDLIMITVAL, label_buf);
				setCtrlReal(CTL_CLIAGT_ADDEDLIMITVAL, added_limit_val);
			}
			else {
				showCtrl(CTL_CLIAGT_ADDEDLIMITVAL, 0);
			}
			// } @v8.2.4
			// @v7.1.5 {
			SetupStringCombo(this, CTLSEL_CLIAGT_RETLIMPRD, PPTXT_CYCLELIST, data.RetLimPrd);
			setCtrlReal(CTL_CLIAGT_RETLIM, fdiv100i(data.RetLimPart));
			// } @v7.1.5
			return 1;
		}
		int    getDTS(PPClientAgreement * pAgt)
		{
			int    ok = 1, sel = 0;
			getCtrlData(CTL_CLIAGT_CODE, data.Code);
			getCtrlData(sel = CTL_CLIAGT_DATE,      &data.BegDt);
			THROW_SL(checkdate(data.BegDt, 1));
			getCtrlData(sel = CTL_CLIAGT_EXPIRY,    &data.Expiry);
			THROW_SL(checkdate(data.Expiry, 1));
			getCtrlData(sel = CTL_CLIAGT_LOCKPRCBEFORE, &data.LockPrcBefore);
			THROW_SL(checkdate(data.LockPrcBefore, 1));
			getCtrlData(sel = CTL_CLIAGT_MAXCREDIT, &data.MaxCredit);
			THROW_PP(data.MaxCredit >= 0L, PPERR_USERINPUT);
			getCtrlData(sel = CTL_CLIAGT_MAXDSCNT,  &data.MaxDscnt);
			THROW_PP(data.MaxDscnt >= 0L && data.MaxDscnt <= 100, PPERR_USERINPUT);
			getCtrlData(sel = CTL_CLIAGT_DSCNT,     &data.Dscnt);
			THROW_PP(data.Dscnt >= -100 && data.Dscnt <= 100 /*&&data.Dscnt <= data.MaxDscnt*/, PPERR_INVCLIAGTDIS);
			getCtrlData(CTL_CLIAGT_PAYPERIOD, &data.DefPayPeriod);
			getCtrlData(CTL_CLIAGT_PAYMDTBASE, &data.PaymDateBase); // @v8.4.2
			getCtrlData(CTLSEL_CLIAGT_AGENT,  &data.DefAgentID);
			getCtrlData(CTLSEL_CLIAGT_QUOTKIND, &data.DefQuotKindID);
			getCtrlData(CTLSEL_CLIAGT_EXTOBJECT, &data.ExtObjectID);
			GetClusterData(CTL_CLIAGT_FLAGS, &data.Flags);
			data.RetLimPrd = (int16)getCtrlLong(CTLSEL_CLIAGT_RETLIMPRD);
			if(data.RetLimPrd) {
				double retlim = getCtrlReal(CTL_CLIAGT_RETLIM);
				data.RetLimPart = (retlim > 0.0) ? (uint16)(retlim * 100.0) : 0;
			}
			else
				data.RetLimPart = 0;
			ASSIGN_PTR(pAgt, data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			AgtDialog::handleEvent(event);
			if(event.isCmd(cmRounding))
				editRoundingParam();
			else if(event.isCmd(cmDebtLimList))
				EditDebtLimList(data);
			else
				return;
			clearEvent(event);
		}
		int  editRoundingParam()
		{
			class CliAgtRndDialog : public TDialog {
			public:
				CliAgtRndDialog() : TDialog(DLG_CLIAGTRND)
				{
				}
				int setDTS(const PPClientAgreement * pData)
				{
					int    ok = 1;
					Data = *pData;
					AddClusterAssoc(CTL_CLIAGTRND_FLAGS, 0, AGTF_PRICEROUNDING);
					SetClusterData(CTL_CLIAGTRND_FLAGS, Data.Flags);
					setCtrlData(CTL_CLIAGTRND_PREC, &Data.PriceRoundPrec); // ! float
					AddClusterAssocDef(CTL_CLIAGTRND_ROUND,  0, 0);
					AddClusterAssoc(CTL_CLIAGTRND_ROUND,  1, -1);
					AddClusterAssoc(CTL_CLIAGTRND_ROUND,  2, +1);
					SetClusterData(CTL_CLIAGTRND_ROUND, Data.PriceRoundDir);
					AddClusterAssoc(CTL_CLIAGTRND_ROUNDVAT, 0, AGTF_PRICEROUNDVAT);
					SetClusterData(CTL_CLIAGTRND_ROUNDVAT, Data.Flags);
					SetupCtrls();
					return ok;
				}
				int getDTS(PPClientAgreement * pData)
				{
					int    ok = 1;
					GetClusterData(CTL_CLIAGTRND_FLAGS, &Data.Flags);
					getCtrlData(CTL_CLIAGTRND_PREC, &Data.PriceRoundPrec); // ! float
					GetClusterData(CTL_CLIAGTRND_ROUND, &Data.PriceRoundDir);
					GetClusterData(CTL_CLIAGTRND_ROUNDVAT, &Data.Flags);
					ASSIGN_PTR(pData, Data);
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isClusterClk(CTL_CLIAGTRND_FLAGS)) {
						GetClusterData(CTL_CLIAGTRND_FLAGS, &Data.Flags);
						SetupCtrls();
						clearEvent(event);
					}
				}
				void   SetupCtrls()
				{
					disableCtrls(!BIN(Data.Flags & AGTF_PRICEROUNDING),
						CTL_CLIAGTRND_PREC, CTL_CLIAGTRND_ROUND, CTL_CLIAGTRND_ROUNDVAT, 0);
				}
				PPClientAgreement Data;
			};
			DIALOG_PROC_BODY(CliAgtRndDialog, &data);
		}
		PPObjArticle ArObj;
		PPClientAgreement data;
	};
	DIALOG_PROC_BODY(CliAgtDialog, agt);
}

int SLAPI PPObjArticle::EditAgreement(PPID arID)
{
	int    ok = -1, r;
	ArticleTbl::Rec ar_rec;
	if(Search(arID, &ar_rec) > 0) {
		int    agt_kind = -1;
		THROW(agt_kind = GetAgreementKind(&ar_rec));
		if(agt_kind == 1) {
			PPClientAgreement cli_agt_rec;
			THROW(r = GetClientAgreement(ar_rec.ID, &cli_agt_rec));
			cli_agt_rec.ClientID = ar_rec.ID;
			if(EditClientAgreement(&cli_agt_rec) > 0) {
				THROW(PutClientAgreement(ar_rec.ID, &cli_agt_rec, 1));
				ok = 1;
			}
		}
		else if(agt_kind == 2) {
			PPSupplAgreement suppl_agt_rec;
			THROW(GetSupplAgreement(ar_rec.ID, &suppl_agt_rec, 0));
			suppl_agt_rec.SupplID = ar_rec.ID;
			if(EditSupplAgreement(&suppl_agt_rec) > 0) {
				THROW(PutSupplAgreement(ar_rec.ID, &suppl_agt_rec, 1));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

// static
int SLAPI PPObjArticle::DefaultClientAgreement()
{
	int    ok = 1;
	PPObjArticle arobj;
	PPClientAgreement agt;
	THROW(CheckCfgRights(PPCFGOBJ_CLIENTDEAL, PPR_MOD, 0));
	THROW(arobj.CheckRights(ARTRT_CLIAGT));
	THROW(arobj.GetClientAgreement(0, &agt));
	if(arobj.EditClientAgreement(&agt) > 0)
		THROW(arobj.PutClientAgreement(0, &agt, 1));
	CATCHZOKPPERR
	return ok;
}
//
//
//
SLAPI PPSupplAgreement::ExchangeParam::ExchangeParam()
{
	Clear();
}

int FASTCALL PPSupplAgreement::ExchangeParam::Copy(const PPSupplAgreement::ExchangeParam & rS)
{
#define CPY_FLD(f) f=rS.f
	CPY_FLD(LastDt);
	CPY_FLD(GoodsGrpID);
	CPY_FLD(ExpendOp);
	CPY_FLD(RcptOp);
	CPY_FLD(SupplRetOp);
	CPY_FLD(RetOp);
	CPY_FLD(MovInOp);
	CPY_FLD(MovOutOp);
	CPY_FLD(PriceQuotID);
	CPY_FLD(ProtVer);
	CPY_FLD(ConnAddr);
	CPY_FLD(ExtString);
	CPY_FLD(Fb);
#undef CPY_FLD
	DebtDimList = rS.DebtDimList; // @v9.1.3
	return 1;
}

PPSupplAgreement::ExchangeParam & FASTCALL PPSupplAgreement::ExchangeParam::operator = (const ExchangeParam & rS)
{
	Copy(rS);
	return *this;
}
//
// Descr: Структура конфигурации обмена данными с поставщиком
//   Хранится в таблице Property с координатами {PPOBJ_ARTICLE, SupplID, ARTPRP_SUPPLAGT_EXCH}
// Начиная с @v8.5.0 структура устарела - применяется только для обратной совместимости при чтении из базы данных.
//
struct PPSupplExchangeCfg { // @persistent @store(PropertyTbl)
	SLAPI  PPSupplExchangeCfg()
	{
		Clear();
	}
	PPSupplExchangeCfg & Clear()
	{
		THISZERO();
		return *this;
	}
	int    SLAPI IsEmpty() const
	{
		return ((!SupplID || !GGrpID || !IP) && isempty(PrvdrSymb)); // @vmiller @added --> isempty(PrvdrSymb) // ???
	}
	PPID   Tag;            // const=PPOBJ_ARTICLE
	PPID   SupplID;        // ИД поставщика
	PPID   PropID;         // const=ATRPRP_SUPPLAGT_EXCH
	LDATE  LastDt;         // Дата последнего обмена
	PPID   GGrpID;         // Группа товаров
	PPID   TechID;         // Технология обмена
	char   ClientCode[16]; // Код данного клиента на сервере поставщика
	PPID   ExpendOp;       // Вид операции отгрузки
	ulong  IP;             //
	int16  Port;           //
	PPID   RcptOp;         // Операция прихода
	PPID   SupplRetOp;     // Операция возврата поставщику
	PPID   RetOp;          // Операция возврата от покупател
	PPID   MovInOp;        // Перемещение со склада (расход)
	PPID   MovOutOp;       // Перемещение на склад (приход)
	PPID   PriceQuotID;    // Котировка по которой будем назначать цену в документах
	uint16 Ver;            // Версия протокола обмена
	char   PrvdrSymb[12];  // Символ провайдера EDI
	//char   Reserve[12];    // @reserve
};

PPSupplAgreement::ExchangeParam & FASTCALL PPSupplAgreement::ExchangeParam::operator = (const PPSupplExchangeCfg & rS)
{
	Clear();
	LastDt = rS.LastDt;
	GoodsGrpID = rS.GGrpID;
	ExpendOp = rS.ExpendOp;
	RcptOp = rS.RcptOp;
	SupplRetOp = rS.SupplRetOp;
	RetOp = rS.RetOp;
	MovInOp = rS.MovInOp;
	MovOutOp = rS.MovOutOp;
	PriceQuotID = rS.PriceQuotID;
	ProtVer = rS.Ver;
	{
		SString temp_buf;
		PutExtStrData(extssEDIPrvdrSymb, (temp_buf = rS.PrvdrSymb).Strip());
		PutExtStrData(extssClientCode, (temp_buf = rS.ClientCode).Strip());
	}
	ConnAddr.Set(rS.IP, rS.Port);
	return *this;
}

PPSupplAgreement::ExchangeParam & PPSupplAgreement::ExchangeParam::Clear()
{
	LastDt = ZERODATE;
	GoodsGrpID = 0;
	ExpendOp = 0;
	RcptOp = 0;
	SupplRetOp = 0;
	RetOp = 0;
	MovInOp = 0;
	MovOutOp = 0;
	PriceQuotID = 0;
	ProtVer = 0;
	ConnAddr.Clear();
	ExtString = 0;
	MEMSZERO(Fb);
	DebtDimList.Set(0); // @v9.1.3
	return *this;
}

int FASTCALL PPSupplAgreement::ExchangeParam::IsEqual(const ExchangeParam & rS) const
{
#define CMP_FLD(f) if((f) != (rS.f)) return 0
	CMP_FLD(LastDt);
	CMP_FLD(GoodsGrpID);
	CMP_FLD(ExpendOp);
	CMP_FLD(RcptOp);
	CMP_FLD(SupplRetOp);
	CMP_FLD(RetOp);
	CMP_FLD(MovInOp);
	CMP_FLD(MovOutOp);
	CMP_FLD(PriceQuotID);
	CMP_FLD(ProtVer);
	CMP_FLD(ConnAddr);
	CMP_FLD(ExtString);
	CMP_FLD(Fb.DefUnitID); // @v9.2.4
	CMP_FLD(Fb.CliCodeTagID); // @v9.4.4
	CMP_FLD(Fb.LocCodeTagID); // @v9.4.4
	CMP_FLD(Fb.BillAckTagID); // @v9.5.7
	CMP_FLD(Fb.SequenceID); // @v9.4.2
	CMP_FLD(Fb.StyloPalmID); // @v9.5.5
#undef CMP_FLD
	if(!DebtDimList.IsEqual(rS.DebtDimList)) return 0; // @v9.1.3
	return 1;
}

int  SLAPI PPSupplAgreement::ExchangeParam::Serialize_Before_v9103_(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, LastDt, rBuf));
	THROW_SL(pSCtx->Serialize(dir, GoodsGrpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ExpendOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, RcptOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, SupplRetOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, RetOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MovInOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MovOutOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, PriceQuotID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ProtVer, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 0));
	THROW_SL(ConnAddr.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, ExtString, rBuf));
	CATCHZOK
	return ok;
}

int SLAPI PPSupplAgreement::ExchangeParam::Serialize_(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, LastDt, rBuf));
	THROW_SL(pSCtx->Serialize(dir, GoodsGrpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ExpendOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, RcptOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, SupplRetOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, RetOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MovInOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MovOutOp, rBuf));
	THROW_SL(pSCtx->Serialize(dir, PriceQuotID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ProtVer, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 0));
	THROW_SL(ConnAddr.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, ExtString, rBuf));
	THROW_SL(DebtDimList.Serialize(dir, rBuf, pSCtx)); // @v9.1.3
	CATCHZOK
	return ok;
}

int SLAPI PPSupplAgreement::ExchangeParam::IsEmpty() const
{
	if(GoodsGrpID || !ConnAddr.IsEmpty())
		return 0;
	else {
		SString temp_buf;
		if(GetExtStrData(extssClientCode, temp_buf) > 0 && temp_buf.NotEmptyS())
			return 0;
		else if(GetExtStrData(extssEDIPrvdrSymb, temp_buf) > 0 && temp_buf.NotEmptyS())
			return 0;
		else if(DebtDimList.GetCount()) // @v9.1.3
			return 0;
		else
			return 1;
	}
}

int SLAPI PPSupplAgreement::ExchangeParam::GetExtStrData(int fldID, SString & rBuf) const
{
	return PPGetExtStrData(fldID, ExtString, rBuf);
}

int SLAPI PPSupplAgreement::ExchangeParam::PutExtStrData(int fldID, const char * pStr)
{
	return PPPutExtStrData(fldID, ExtString, pStr);
}

SLAPI PPSupplAgreement::OrderParamEntry::OrderParamEntry()
{
	THISZERO();
}

int FASTCALL PPSupplAgreement::OrderParamEntry::IsEqual(const OrderParamEntry & rS) const
{
#define CMP_FLD(f) if((f) != (rS.f)) return 0
	CMP_FLD(GoodsGrpID);
	CMP_FLD(LocID);
	CMP_FLD(MngrID);
	CMP_FLD(OrdPrdDays);
	CMP_FLD(Fb.DuePrdDays); // @v8.5.2
	CMP_FLD(Dr);
#undef CMP_FLD
	return 1;
}

int SLAPI PPSupplAgreement::OrderParamEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, GoodsGrpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, LocID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MngrID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, OrdPrdDays, rBuf));
	THROW_SL(Dr.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 0));
	CATCHZOK
	return ok;
}

SLAPI PPSupplAgreement::PPSupplAgreement()
{
	Clear();
}

int FASTCALL PPSupplAgreement::IsEqual(const PPSupplAgreement & rS) const
{
#define CMP_FLD(f) if((f) != (rS.f)) return 0
	// Ver не сравниваем - не значащее поле
	CMP_FLD(SupplID);
	CMP_FLD(Flags);
	CMP_FLD(BegDt);
	CMP_FLD(Expiry);
	CMP_FLD(DefPayPeriod);
	CMP_FLD(InvPriceAction);
	CMP_FLD(DefDlvrTerm);
	CMP_FLD(PctRet);
	// @v8.5.2 CMP_FLD(OrdPrdDays);
	CMP_FLD(DefAgentID);
	CMP_FLD(CostQuotKindID);
	CMP_FLD(PurchaseOpID);
	CMP_FLD(DevUpQuotKindID);
	CMP_FLD(DevDnQuotKindID);
	CMP_FLD(MngrRelID);
	CMP_FLD(Dr);
#undef CMP_FLD
	if(!Ep.IsEqual(rS.Ep))
		return 0;
	{
		const uint _c1 = OrderParamList.getCount();
		const uint _c2 = rS.OrderParamList.getCount();
		if(_c1 != _c2)
			return 0;
		else if(_c1) {
			for(uint i = 0; i < _c1; i++) {
				const OrderParamEntry & r_e1 = OrderParamList.at(i);
				const OrderParamEntry & r_e2 = rS.OrderParamList.at(i);
				if(!r_e1.IsEqual(r_e2))
					return 0;
			}
		}
	}
	return 1;
}

PPSupplAgreement & PPSupplAgreement::Clear()
{
	memzero(this, offsetof(PPSupplAgreement, Ep));
	Ep.Clear();
	OrderParamList.clear();
	return *this;
}

int SLAPI PPSupplAgreement::IsEmpty() const
{
	if(Flags || BegDt || Expiry || DefPayPeriod || DefAgentID || DefDlvrTerm || PctRet)
		return 0;
	else if(!Ep.IsEmpty())
		return 0;
	else if(OrderParamList.getCount())
		return 0;
	else
		return 1;
}

int FASTCALL PPSupplAgreement::RestoreAutoOrderParams(const PPSupplAgreement & rS)
{
	SETFLAGBYSAMPLE(Flags, AGTF_AUTOORDER, rS.Flags);
	// @v8.5.2 OrdPrdDays = rS.OrdPrdDays;
	DefDlvrTerm = rS.DefDlvrTerm;
	Dr = rS.Dr;
	OrderParamList = rS.OrderParamList;
	return 1;
}

int SLAPI PPSupplAgreement::SearchOrderParamEntry(const OrderParamEntry & rKey, int thisPos, uint * pFoundPos) const
{
	for(uint i = 0; i < OrderParamList.getCount(); i++) {
		if((int)i != thisPos) {
			const OrderParamEntry & r_entry = OrderParamList.at(i);
			if(r_entry.GoodsGrpID == rKey.GoodsGrpID && r_entry.LocID == rKey.LocID && r_entry.MngrID == rKey.MngrID) {
				ASSIGN_PTR(pFoundPos, i);
				return 1;
			}
		}
	}
	return 0;
}

int SLAPI PPSupplAgreement::SetOrderParamEntry(int pos, OrderParamEntry & rEntry, uint * pResultPos)
{
	int    ok = 1;
	const  uint _c = OrderParamList.getCount();
	THROW_PP(SearchOrderParamEntry(rEntry, pos, 0) == 0, PPERR_DUPSORDPE);
	THROW_PP(DateRepeating::IsValidPrd(rEntry.Dr.Prd), PPERR_INVDATEREP);
	if(pos < 0 || pos >= (int)_c) {
		THROW_SL(OrderParamList.insert(&rEntry));
		ASSIGN_PTR(pResultPos, _c);
	}
	else {
		OrderParamList.at(pos) = rEntry;
		ASSIGN_PTR(pResultPos, (uint)pos);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPSupplAgreement::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, SupplID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, BegDt, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Expiry, rBuf));
	THROW_SL(pSCtx->Serialize(dir, DefPayPeriod, rBuf));
	THROW_SL(pSCtx->Serialize(dir, InvPriceAction, rBuf));
	THROW_SL(pSCtx->Serialize(dir, DefDlvrTerm, rBuf));
	THROW_SL(pSCtx->Serialize(dir, PctRet, rBuf));
	// @v8.5.2 THROW_SL(pSCtx->Serialize(dir, OrdPrdDays, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Reserve_opd, rBuf)); // @v8.5.2
	THROW_SL(pSCtx->Serialize(dir, DefAgentID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, CostQuotKindID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, PurchaseOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, DevUpQuotKindID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, DevDnQuotKindID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, MngrRelID, rBuf));
	THROW_SL(Dr.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 0));
	if(dir < 0 && Ver.IsLt(9, 1, 3)) {
		THROW(Ep.Serialize_Before_v9103_(dir, rBuf, pSCtx));
	}
	else {
		THROW(Ep.Serialize_(dir, rBuf, pSCtx));
	}
	THROW_SL(TSArray_Serialize(OrderParamList, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

struct _PPSupplAgt {       // @persistent @store(PropertyTbl)
	long   Tag;            // Const=PPOBJ_ARTICLE
	long   ArtID;          // ->Article.ID
	long   PropID;         // Const=ARTPRP_SUPPLAGT
	long   Flags;          //
	LDATE  BegDt;          // Дата начала действия соглашения //
	LDATE  Expiry;         // Дата окончания действия соглашения //
	int16  OrdPrdDays;     // Период для расчета заказа поставщику при автоматическом формировании документов заказа поставщику (в днях)
	int16  OrdDrPrd;       // Календарь для автоматического создания документов
	int16  OrdDrKind;      //   заказа поставщику
	int16  Reserve1;       // @reserve
	double Reserve2;       // @reserve
	double Reserve3;       // @reserve
	short  DefPayPeriod;   // Количество дней от отгрузки до оплаты по умолчанию
	PPID   DefAgentID;     // Агент, закрепленный за поставщиком
	PPID   CostQuotKindID; // Вид котировки, управляющей контрактными ценами поставщиков
	short  DefDlvrTerm;    // Срок доставки товара в днях, начиная с даты документа закупки
	short  PctRet;         // Максимальный объем возврата товара по накладной в процентах от суммы накладной
	PPID   PurchaseOpID;   // Вид операции закупки (драфт-приход)
	PPID   DevUpQuotKindID; // Вид котировки, ограничивающий верхнюю границу отклонения фактических цен от контрактных
	PPID   DevDnQuotKindID; // Вид котировки, ограничивающий нижнюю границу отклонения фактических цен от контрактных
	PPID   MngrRelID;      // Тип отношения, определяющий привязку поставщиков к менеджерам
	int16  InvPriceAction; // Действие при неправильной контрактной цене
	long   OrdDrDtl;
};

// static
int SLAPI PPObjArticle::PropToSupplAgt(const PropertyTbl::Rec * pPropRec, PPSupplAgreement * pAgt)
{
	const _PPSupplAgt * p_agt = (const _PPSupplAgt *)pPropRec;
	pAgt->SupplID         = p_agt->ArtID;
	pAgt->Flags           = (p_agt->Flags | AGTF_LOADED);
	pAgt->BegDt           = p_agt->BegDt;
	pAgt->Expiry          = p_agt->Expiry;
	pAgt->DefPayPeriod    = p_agt->DefPayPeriod;
	pAgt->DefAgentID      = p_agt->DefAgentID;
	pAgt->CostQuotKindID  = p_agt->CostQuotKindID;
	pAgt->DefDlvrTerm     = p_agt->DefDlvrTerm;
	pAgt->PctRet          = p_agt->PctRet;
	pAgt->PurchaseOpID    = p_agt->PurchaseOpID;
	pAgt->DevUpQuotKindID = p_agt->DevUpQuotKindID;
	pAgt->DevDnQuotKindID = p_agt->DevDnQuotKindID;
	pAgt->MngrRelID       = p_agt->MngrRelID;
	pAgt->InvPriceAction  = p_agt->InvPriceAction;
	// @v8.5.2 pAgt->OrdPrdDays      = p_agt->OrdPrdDays;
    pAgt->Dr.Init(p_agt->OrdDrPrd, p_agt->OrdDrKind);
    pAgt->Dr.LongToDtl(p_agt->OrdDrDtl);
	return 1;
}

// static
int SLAPI PPObjArticle::HasSupplAgreement(PPID id)
{
	int    yes = 0;
	if(id > 0) {
		SBuffer _buf;
		PropertyTbl::Rec prop_rec;
		Reference * p_ref = PPRef;
		if(p_ref->GetPropSBuffer(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT2, _buf) > 0)
			yes = 1;
		else if(p_ref->GetProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT, &prop_rec, sizeof(prop_rec)) > 0)
			yes = 1;
	}
	return yes;
}

// static
int SLAPI PPObjArticle::GetSupplAgreement(PPID id, PPSupplAgreement * pAgt, int useInheritance)
{
	int    ok = -1;
	if(pAgt) {
		pAgt->Clear();
		int    r = 0;
		SBuffer _buf;
		Reference * p_ref = PPRef;
		THROW(r = p_ref->GetPropSBuffer(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT2, _buf));
		if(r > 0) {
			SSerializeContext ctx;
			THROW(pAgt->Serialize(-1, _buf, &ctx));
			pAgt->Flags |= AGTF_LOADED;
			ok = 1;
		}
		else {
			PropertyTbl::Rec prop_rec;
			THROW(r = p_ref->GetProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT, &prop_rec, sizeof(prop_rec)));
			if(r > 0) {
				PPSupplExchangeCfg _ex_cfg;
				THROW(p_ref->GetProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT_EXCH, &_ex_cfg, sizeof(_ex_cfg)));
				pAgt->Ep = _ex_cfg;
				PropToSupplAgt(&prop_rec, pAgt);
				pAgt->SupplID = id;
				ok = 1;
			}
			else {
				// @v8.2.2 {
				if(id && useInheritance) {
					PPObjPersonRelType rt_obj;
					PPPersonRelTypePacket rt_pack;
					PPIDArray rel_list;
					if(rt_obj.Fetch(PPPSNRELTYP_AFFIL, &rt_pack) > 0 && (rt_pack.Rec.Flags & PPPersonRelType::fInhAgreements)) {
						PPObjArticle ar_obj;
						if(ar_obj.GetRelPersonList(id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0) {
							for(uint i = 0; r < 0 && i < rel_list.getCount(); i++) {
								PPID rel_ar_id = ObjectToPerson(rel_list.get(i), 0);
								// @v8.5.0 {
								if(GetSupplAgreement(rel_ar_id, pAgt, 0) > 0) { // @recursion не зависимо от версии хранения результат будет верным
									pAgt->SupplID = id;
									ok = 2;
								}
								// } @v8.5.0
								/* @v8.5.0
								THROW(r = p_ref->GetProp(PPOBJ_ARTICLE, rel_ar_id, ARTPRP_SUPPLAGT, &prop_rec, sizeof(prop_rec)));
								if(r > 0) {
									THROW(p_ref->GetProp(PPOBJ_ARTICLE, rel_ar_id, ARTPRP_SUPPLAGT_EXCH, &pAgt->ExchCfg, sizeof(pAgt->ExchCfg)));
									PropToSupplAgt(&prop_rec, pAgt);
									pAgt->SupplID = id;
									ok = 2;
								}
								*/
							}
						}
					}
				}
				// } @v8.2.2
				if(r < 0)
					pAgt->Flags |= AGTF_LOADED;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjArticle::PutSupplAgreement(PPID id, PPSupplAgreement * pAgt, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	THROW(CheckRights(ARTRT_CLIAGT));
#if 1
	{
		SBuffer _buf;
		if(pAgt) {
			SSerializeContext ctx;
			THROW(pAgt->Serialize(+1, _buf, &ctx));
		}
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(p_ref->PutPropSBuffer(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT2, _buf, 0));
			THROW(p_ref->PutProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT, 0, 0, 0));
			THROW(p_ref->PutProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT_EXCH, 0, 0, 0));
			THROW(tra.Commit());
		}
	}
#else // }{
	{
		_PPSupplAgt _agt;
		if(pAgt) {
			MEMSZERO(_agt);
			_agt.Tag             = PPOBJ_ARTICLE;
			_agt.ArtID           = id;
			_agt.PropID          = ARTPRP_SUPPLAGT;
			_agt.Flags           = (pAgt->Flags & ~AGTF_LOADED);
			_agt.BegDt           = pAgt->BegDt;
			_agt.Expiry          = pAgt->Expiry;
			_agt.DefPayPeriod    = pAgt->DefPayPeriod;
			_agt.DefAgentID      = pAgt->DefAgentID;
			_agt.CostQuotKindID  = pAgt->CostQuotKindID;
			_agt.DefDlvrTerm     = pAgt->DefDlvrTerm;
			_agt.PctRet          = pAgt->PctRet;
			_agt.PurchaseOpID    = pAgt->PurchaseOpID;
			_agt.DevUpQuotKindID = pAgt->DevUpQuotKindID;
			_agt.DevDnQuotKindID = pAgt->DevDnQuotKindID;
			_agt.MngrRelID       = pAgt->MngrRelID;
			_agt.InvPriceAction  = pAgt->InvPriceAction;
			_agt.OrdDrPrd = pAgt->Dr.Prd;
			_agt.OrdDrKind = pAgt->Dr.RepeatKind;
			_agt.OrdDrDtl = pAgt->Dr.DtlToLong();
			_agt.OrdPrdDays      = pAgt->OrdPrdDays;
		}
		THROW(p_ref->PutProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT, (pAgt ? &_agt : 0), 0, use_ta));
		THROW(p_ref->PutProp(PPOBJ_ARTICLE, id, ARTPRP_SUPPLAGT_EXCH, (pAgt && !pAgt->ExchCfg.IsEmpty()) ? &pAgt->ExchCfg : 0, 0, use_ta));
	}
#endif // }
	CATCHZOK
	return ok;
}
//
//
//
static int EditOrderCalendar(DateRepeating * pData)
{
	int    ok = -1;
	RepeatingDialog * dlg = new RepeatingDialog;
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pData);
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(pData)) {
				ok = 1;
			}
			else
				PPError();
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class SupplAgtDialog : public PPListDialog {
public:
	SupplAgtDialog(PPID supplID) : PPListDialog(supplID ? DLG_SUPPLAGT : DLG_DEFSUPPLAGT, CTL_SUPPLAGT_ORDPL)
		//AgtDialog(supplID ? DLG_SUPPLAGT : DLG_DEFSUPPLAGT)
	{
		ArID = supplID;
		SetupCalDate(CTLCAL_SUPPLAGT_DATE, CTL_SUPPLAGT_DATE);
		SetupCalDate(CTLCAL_SUPPLAGT_EXPIRY, CTL_SUPPLAGT_EXPIRY);
		enableCommand(cmOK, ArObj.CheckRights(ARTRT_CLIAGT));
		enableCommand(cmExchangeCfg, ArID != 0);
		updateList(-1);
	}
	int  setDTS(const PPSupplAgreement*);
	int  getDTS(PPSupplAgreement*);
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmBills)) {
			if(ArID) {
				BillFilt flt;
				flt.ObjectID = ArID;
				flt.Flags   |= BillFilt::fShowDebt;
				ViewGoodsBills(&flt, 1);
			}
		}
		else if(event.isCmd(cmExchangeCfg)) {
			EditExchangeCfg();
		}
		else if(event.isCmd(cmOrdCalendar)) {
			EditOrderCalendar(&Data.Dr);
		}
		else if(event.isClusterClk(CTL_SUPPLAGT_FLAGS)) {
			setupCtrls(GetClusterData(CTL_SUPPLAGT_FLAGS));
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList()
	{
		int    ok = 1;
		SString sub;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < Data.OrderParamList.getCount(); i++) {
			const PPSupplAgreement::OrderParamEntry & r_entry = Data.OrderParamList.at(i);
			ss.clear(1);
			sub = 0;
			if(r_entry.GoodsGrpID)
				GetGoodsName(r_entry.GoodsGrpID, sub);
			ss.add(sub);
			sub = 0;
			if(r_entry.LocID)
				GetLocationName(r_entry.LocID, sub);
			ss.add(sub);
			/*
			sub = 0;
			if(r_entry.MngrID)
				GetPersonName(r_entry.MngrID, sub);
			ss.add(sub);
			*/
			ss.add((sub = 0).Cat(r_entry.OrdPrdDays));
			ss.add((sub = 0).Cat(r_entry.Fb.DuePrdDays));
			ss.add(r_entry.Dr.Format(0, sub = 0));
			THROW(addStringToList(i+1, ss.getBuf()));
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
        PPSupplAgreement::OrderParamEntry entry;
        while(ok < 0 && EditOrdParamEntry(ArID, &entry, -1) > 0) {
			uint   pos = 0;
			if(!Data.SetOrderParamEntry(-1, entry, &pos))
				PPError();
			else {
				ASSIGN_PTR(pPos, pos);
				ASSIGN_PTR(pID, pos+1);
				ok = 1;
			}
        }
        return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < (long)Data.OrderParamList.getCount()) {
			PPSupplAgreement::OrderParamEntry & r_entry = Data.OrderParamList.at(pos);
			while(ok < 0 && EditOrdParamEntry(ArID, &r_entry, pos) > 0) {
				if(!Data.SetOrderParamEntry(pos, r_entry, 0))
					PPError();
				else {
					ok = 1;
				}
			}
		}
        return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(pos >= 0 && pos < (long)Data.OrderParamList.getCount()) {
			Data.OrderParamList.atFree(pos);
			ok = 1;
		}
        return ok;
	}
	int    EditExchangeCfg();
	int    EditOrdCalendar();
	int    setupCtrls(long flags);
	int    EditOrdParamEntry(PPID arID, PPSupplAgreement::OrderParamEntry * pEntry, int pos);

	PPID   ArID;
	PPSupplAgreement Data;
	PPObjArticle ArObj;
};

int SupplAgtDialog::EditOrdParamEntry(PPID arID, PPSupplAgreement::OrderParamEntry * pEntry, int pos)
{
    class SOrdParamEntryDialog : public TDialog {
	public:
		SOrdParamEntryDialog(PPID arID) : TDialog(DLG_SORDPE)
		{
			ArID = arID;
		}
		int  setDTS(PPSupplAgreement::OrderParamEntry * pData)
		{
			RVALUEPTR(Data, pData);
			if(ArID) {
				SString temp_buf;
				GetArticleName(ArID, temp_buf);
				setCtrlString(CTL_SORDPE_SUPPL, temp_buf);
			}
			selectCtrl(CTL_SORDPE_GOODSGRP);
            SetupPPObjCombo(this, CTLSEL_SORDPE_GOODSGRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, 0, 0);
            SetupPersonCombo(this, CTLSEL_SORDPE_MNGR, Data.MngrID, 0, 0, 0);
            SetupLocationCombo(this, CTLSEL_SORDPE_LOC, Data.LocID, 0, LOCTYP_WAREHOUSE, 0);
            setCtrlData(CTL_SORDPE_ORDPRD, &Data.OrdPrdDays);
            setCtrlData(CTL_SORDPE_DUEPRD, &Data.Fb.DuePrdDays);
            return 1;
		}
		int  getDTS(PPSupplAgreement::OrderParamEntry * pData)
		{
			int    ok = 1;
            getCtrlData(CTLSEL_SORDPE_GOODSGRP, &Data.GoodsGrpID);
            getCtrlData(CTLSEL_SORDPE_MNGR, &Data.MngrID);
            getCtrlData(CTLSEL_SORDPE_LOC, &Data.LocID);
            getCtrlData(CTL_SORDPE_ORDPRD, &Data.OrdPrdDays);
            getCtrlData(CTL_SORDPE_DUEPRD, &Data.Fb.DuePrdDays);
            ASSIGN_PTR(pData, Data);
            return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmOrdCalendar))
				EditOrderCalendar(&Data.Dr);
			else
				return;
			clearEvent(event);
		}
		PPSupplAgreement::OrderParamEntry Data;
		PPID   ArID;
    };
    DIALOG_PROC_BODY_P1(SOrdParamEntryDialog, arID, pEntry);
}

int SupplAgtDialog::setupCtrls(long flags)
{
	int    to_disable = BIN(!(flags & AGTF_AUTOORDER));
	disableCtrl(CTL_SUPPLAGT_ORDPRD, to_disable);
	enableCommand(cmOrdCalendar, !to_disable);
	if(to_disable) {
		setCtrlLong(CTL_SUPPLAGT_ORDPRD, 0L);
		Data.Dr.Init(0, 0);
	}
	else if(!DateRepeating::IsValidPrd(Data.Dr.Prd)) { // @v8.3.12 if(!DateRepeating::IsValidPrd(Data.OrdDrPrd))
		Data.Dr.Init(PRD_DAY);
	}
	return 1;
}

static int EditSupplExchOpList(PPSupplAgreement::ExchangeParam * pData)
{
	class SupplExpOpListDialog : public PPListDialog {
	private:
		ObjTagFilt PsnTagFlt;
		ObjTagFilt LocTagFlt;
		ObjTagFilt BillTagFlt;
	public:
		SupplExpOpListDialog() : PPListDialog(DLG_SUPPLEOPS, CTL_SUPPLEOPS_DBTDIM),
			PsnTagFlt(PPOBJ_PERSON), LocTagFlt(PPOBJ_LOCATION), BillTagFlt(PPOBJ_BILL)
		{
		}
		int    setDTS(const PPSupplAgreement::ExchangeParam * pData)
		{
			PPOprKind op_kind;
			PPIDArray op_list;
			if(!RVALUEPTR(Data, pData))
				Data.Clear();
			// Расход товара
			{
				MEMSZERO(op_kind);
				for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
					if(oneof2(op_kind.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GENERIC))
						op_list.add(op_id);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_EXP, Data.ExpendOp, 0, &op_list, OPKLF_OPLIST);
			}
			// Приход товара
			{
				op_list.freeAll();
				MEMSZERO(op_kind);
				for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
					if(oneof2(op_kind.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_GENERIC))
						op_list.add(op_id);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_RCPT, Data.RcptOp, 0, &op_list, OPKLF_OPLIST);
			}
			// Возврат товара от покупател
			{
				op_list.freeAll();
				MEMSZERO(op_kind);
				for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
					if(oneof2(op_kind.OpTypeID, PPOPT_GOODSRETURN, PPOPT_GENERIC))
						op_list.add(op_id);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_RET, Data.RetOp, 0, &op_list, OPKLF_OPLIST);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_SUPRET, Data.SupplRetOp, 0, &op_list, OPKLF_OPLIST);
			}
			// Межскладской расход товара, возврат товара поставщику
			{
				op_list.freeAll();
				MEMSZERO(op_kind);
				for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
					if(op_kind.OpTypeID == PPOPT_GOODSEXPEND)
						op_list.add(op_id);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_MOVOUT, Data.MovOutOp, 0, &op_list, OPKLF_OPLIST);
			}
			// Межсладской приход товара
			{
				op_list.freeAll();
				MEMSZERO(op_kind);
				for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
					if(op_kind.OpTypeID == PPOPT_GOODSRECEIPT)
						op_list.add(op_id);
				SetupOprKindCombo(this, CTLSEL_SUPPLEOPS_MOVIN, Data.MovInOp, 0, &op_list, OPKLF_OPLIST);
			}
			SetupPPObjCombo(this, CTLSEL_SUPPLEOPS_UNIT, PPOBJ_UNIT, Data.Fb.DefUnitID, OLW_CANINSERT, 0); // @v9.2.4
			SetupPPObjCombo(this, CTLSEL_SUPPLEOPS_CLICTAG, PPOBJ_TAG, Data.Fb.CliCodeTagID, OLW_CANINSERT, &PsnTagFlt); // @v9.4.4
			SetupPPObjCombo(this, CTLSEL_SUPPLEOPS_LOCCTAG, PPOBJ_TAG, Data.Fb.LocCodeTagID, OLW_CANINSERT, &LocTagFlt); // @v9.4.4
			SetupPPObjCombo(this, CTLSEL_SUPPLEOPS_BACKTAG, PPOBJ_TAG, Data.Fb.BillAckTagID, OLW_CANINSERT, &BillTagFlt); // @v9.5.7

			updateList(-1);
			return 1;
		}
		int    getDTS(PPSupplAgreement::ExchangeParam * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTLSEL_SUPPLEOPS_EXP, &Data.ExpendOp);
			// @v9.1.3 THROW_PP(Data.ExpendOp, PPERR_INVEXPENDOP);
			getCtrlData(CTLSEL_SUPPLEOPS_RCPT,      &Data.RcptOp);
			getCtrlData(CTLSEL_SUPPLEOPS_SUPRET,    &Data.SupplRetOp);
			getCtrlData(CTLSEL_SUPPLEOPS_RET,       &Data.RetOp);
			getCtrlData(CTLSEL_SUPPLEOPS_MOVOUT,    &Data.MovOutOp);
			getCtrlData(CTLSEL_SUPPLEOPS_MOVIN,     &Data.MovInOp);
			getCtrlData(CTLSEL_SUPPLEOPS_UNIT,      &Data.Fb.DefUnitID); // @v9.2.4
			getCtrlData(CTLSEL_SUPPLEOPS_CLICTAG,   &Data.Fb.CliCodeTagID); // @v9.4.4
			getCtrlData(CTLSEL_SUPPLEOPS_LOCCTAG,   &Data.Fb.LocCodeTagID); // @v9.4.4
			getCtrlData(CTLSEL_SUPPLEOPS_BACKTAG,   &Data.Fb.BillAckTagID); // @v9.5.7
			ASSIGN_PTR(pData, Data);
			/* @v9.1.3
			CATCH
				selectCtrl(sel);
				ok = 0;
			ENDCATCH
			*/
			return ok;
		}
	private:
		virtual int setupList()
		{
			int    ok = 1;
			PPDebtDim dd_rec;
			for(uint i = 0; i < Data.DebtDimList.GetCount(); i++) {
				const PPID dd_id = Data.DebtDimList.Get(i);
				if(dd_id && DdObj.Search(dd_id, &dd_rec) > 0) {
					THROW(addStringToList(dd_id, dd_rec.Name));
				}
			}
			CATCHZOK
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			PPDebtDim dd_rec;
			PPID   sel_id = PPObjDebtDim::Select();
			if(sel_id > 0 && DdObj.Search(sel_id, &dd_rec) > 0) {
				if(Data.DebtDimList.Search(sel_id, 0, 0)) {
					PPError(PPERR_DUPDEBTDIMINLIST, dd_rec.Name);
				}
				else {
					if(Data.DebtDimList.Add(sel_id)) {
						ASSIGN_PTR(pPos, Data.DebtDimList.GetCount() - 1);
						ASSIGN_PTR(pID, sel_id);
						ok = 1;
					}
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			return id ? Data.DebtDimList.Remove(id, 0) : -1;
		}
		PPSupplAgreement::ExchangeParam Data;
		PPObjDebtDim DdObj;
	};
	DIALOG_PROC_BODYERR(SupplExpOpListDialog, pData);
}

int SupplAgtDialog::EditExchangeCfg()
{
	class SupplExchangeCfgDialog : public TDialog {
	public:
		SupplExchangeCfgDialog() : TDialog(DLG_SUPLEXCHCFG)
		{
			SetupCalDate(CTLCAL_SUPLEXCHCFG_LASTDT, CTL_SUPLEXCHCFG_LASTDT);
		}
		int    setDTS(const PPSupplAgreement::ExchangeParam * pData)
		{
			SString temp_buf;
			PPOprKind op_kind;
			PPIDArray op_list;
			if(!RVALUEPTR(Data, pData))
				Data.Clear();
			SetupPPObjCombo(this, CTLSEL_SUPLEXCHCFG_GGRP,  PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
			SetupPPObjCombo(this, CTLSEL_SUPLEXCHCFG_STYLO, PPOBJ_STYLOPALM,  Data.Fb.StyloPalmID, OLW_CANSELUPLEVEL, 0); // @v9.5.5
			MEMSZERO(op_kind);
			for(PPID op_id = 0; EnumOperations(0, &op_id, &op_kind) > 0;)
				if(oneof2(op_kind.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_GENERIC))
					op_list.add(op_id);
			// SetupOprKindCombo(this, CTLSEL_SUPLEXCHCFG_OP, Data.OpID, 0, &op_list, OPKLF_OPLIST);
			// SetupPPObjCombo(p_dlg, CTLSEL_SUPLEXCHCFG_TECH, PPOBJ_GOODSGROUP, Data.GGrpID, 0, 0);
			{
				SString tech_symb;
                long    _tech_id = 0;
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_symb);
                PPLoadText(PPTXT_SUPPLIXTECH, temp_buf);
                StringSet ss_tech_symb_list(';', temp_buf);
                StrAssocArray tech_list;
                long    _ss_id = 0;
                for(uint ssp = 0; ss_tech_symb_list.get(&ssp, temp_buf);) {
					++_ss_id;
					tech_list.Add(_ss_id, temp_buf);
					if(tech_symb.CmpNC(temp_buf) == 0)
						_tech_id = _ss_id;
                }
                SetupStrAssocCombo(this, CTLSEL_SUPLEXCHCFG_TECH, &tech_list, _tech_id, 0);
			}
			setCtrlData(CTL_SUPLEXCHCFG_LASTDT,  &Data.LastDt);
			{
				setCtrlString(CTL_SUPLEXCHCFG_IP, Data.ConnAddr.ToStr(InetAddr::fmtAddr, temp_buf));
				int16 port = Data.ConnAddr.GetPort();
				setCtrlData(CTL_SUPLEXCHCFG_PORT, &port);
			}
			{
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssClientCode, temp_buf);
				setCtrlString(CTL_SUPLEXCHCFG_CLICODE, temp_buf);
			}
			SetupPPObjCombo(this, CTLSEL_SUPLEXCHCFG_QUOT, PPOBJ_QUOTKIND, Data.PriceQuotID, 0, 0);
			setCtrlData(CTL_SUPLEXCHCFG_VER, &Data.ProtVer);
			{
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, temp_buf);
				setCtrlString(CTL_SUPLEXCHCFG_PRVDR, temp_buf);
			}
			// @v9.2.0 {
			{
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssRemoveAddr, temp_buf);
				setCtrlString(CTL_SUPLEXCHCFG_TADDR, temp_buf);
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssAccsName, temp_buf);
				setCtrlString(CTL_SUPLEXCHCFG_TNAME, temp_buf);
				Data.GetExtStrData(PPSupplAgreement::ExchangeParam::extssAccsPassw, temp_buf);
				setCtrlString(CTL_SUPLEXCHCFG_TPW, temp_buf);
			}
			// } @v9.2.0
			return 1;
		}
		int    getDTS(PPSupplAgreement::ExchangeParam * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			SString  temp_buf;
			getCtrlData(sel = CTLSEL_SUPLEXCHCFG_GGRP, &Data.GoodsGrpID);
			getCtrlData(sel = CTLSEL_SUPLEXCHCFG_STYLO, &Data.Fb.StyloPalmID); // @v9.5.5
			{
				getCtrlString(CTL_SUPLEXCHCFG_PRVDR, temp_buf = 0);
				Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, temp_buf);
				// @v9.1.3 if(temp_buf.Empty()) // @vmiller Если указан EDI провайдер, то группу товаров указывать не обязательно
				// @v9.1.3 THROW_PP(Data.GoodsGrpID, PPERR_INVGOODSGRP);
			}
			// getCtrlData(CTLSEL_SUPLEXCHCFG_OP,   &Data.OpID);
			// getCtrlData(CTLSEL_SUPLEXCHCFG_TECH, &Data.TechID);
			{
				SString tech_symb;
                long    _tech_id = getCtrlLong(CTLSEL_SUPLEXCHCFG_TECH);
                if(_tech_id) {
					PPLoadText(PPTXT_SUPPLIXTECH, temp_buf);
					StringSet ss_tech_symb_list(';', temp_buf);
					long    _ss_id = 0;
					for(uint ssp = 0; tech_symb.Empty() && ss_tech_symb_list.get(&ssp, temp_buf);) {
						++_ss_id;
						if(_ss_id == _tech_id)
							tech_symb = temp_buf;
					}
					Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_symb);
                }
			}
			{
				int16  port = 0;
				getCtrlString(sel = CTL_SUPLEXCHCFG_IP, temp_buf = 0);
				THROW_PP(temp_buf.NotEmptyS(), PPERR_INVIP);
				getCtrlData(CTL_SUPLEXCHCFG_PORT, &port);
				THROW_SL(Data.ConnAddr.Set(temp_buf, port));
			}
			getCtrlData(CTL_SUPLEXCHCFG_LASTDT,  &Data.LastDt);
			{
				getCtrlString(CTL_SUPLEXCHCFG_CLICODE, temp_buf = 0);
				Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssClientCode, temp_buf);
			}
			// @v9.2.0 {
			{
				getCtrlString(CTL_SUPLEXCHCFG_TADDR, temp_buf = 0);
				Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssRemoveAddr, temp_buf);
				getCtrlString(CTL_SUPLEXCHCFG_TNAME, temp_buf = 0);
				Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssAccsName, temp_buf);
				getCtrlString(CTL_SUPLEXCHCFG_TPW, temp_buf = 0);
				Data.PutExtStrData(PPSupplAgreement::ExchangeParam::extssAccsPassw, temp_buf);
			}
			// } @v9.2.0
			getCtrlData(CTLSEL_SUPLEXCHCFG_QUOT, &Data.PriceQuotID);
			getCtrlData(CTL_SUPLEXCHCFG_VER,     &Data.ProtVer);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmaMore)) {
				EditSupplExchOpList(&Data);
				clearEvent(event);
			}
		}
		//int    EditOps();
		PPSupplAgreement::ExchangeParam Data;
	};
	int    ok = -1, valid_data = 0;
	//PPSupplAgreement::ExchangeParam ep;
	SupplExchangeCfgDialog * dlg = new SupplExchangeCfgDialog();
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(&Data.Ep);
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(&Data.Ep) > 0)
			valid_data = ok = 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SupplAgtDialog::setDTS(const PPSupplAgreement * pAgt)
{
	SString ar_name;
	RVALUEPTR(Data, pAgt);
	ArID = Data.SupplID;
	GetArticleName(Data.SupplID, ar_name);
	setCtrlString(CTL_SUPPLAGT_SUPPLIER, ar_name);
	disableCtrl(CTL_SUPPLAGT_SUPPLIER, 1);
	setCtrlData(CTL_SUPPLAGT_DATE,      &Data.BegDt);
	setCtrlData(CTL_SUPPLAGT_EXPIRY,    &Data.Expiry);
	setCtrlData(CTL_SUPPLAGT_PAYPERIOD, &Data.DefPayPeriod);
	setCtrlData(CTL_SUPPLAGT_DELIVERY,  &Data.DefDlvrTerm);
	setCtrlData(CTL_SUPPLAGT_MAXRETURN, &Data.PctRet);

	SetupArCombo(this, CTLSEL_SUPPLAGT_AGENT, Data.DefAgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet|sacfNonGeneric);
	SetupPPObjCombo(this, CTLSEL_SUPPLAGT_OPRKIND,  PPOBJ_OPRKIND,  Data.PurchaseOpID,    0, (void *)PPOPT_DRAFTRECEIPT);
	SetupPPObjCombo(this, CTLSEL_SUPPLAGT_QUOTCOST, PPOBJ_QUOTKIND, Data.CostQuotKindID,  0, (void *)QuotKindFilt::fAll);
	SetupPPObjCombo(this, CTLSEL_SUPPLAGT_QUOTUP,   PPOBJ_QUOTKIND, Data.DevUpQuotKindID, 0, (void *)QuotKindFilt::fAll);
	SetupPPObjCombo(this, CTLSEL_SUPPLAGT_QUOTDOWN, PPOBJ_QUOTKIND, Data.DevDnQuotKindID, 0, (void *)QuotKindFilt::fAll);
	SetupPPObjCombo(this, CTLSEL_SUPPLAGT_MNGRREL,  PPOBJ_PERSONRELTYPE, Data.MngrRelID,  0);
	if(!ArID) {
		long   inv_price_action = (long)Data.InvPriceAction;
		AddClusterAssoc(CTL_SUPPLAGT_INVPACTION, 0, PPSupplAgreement::invpaRestrict);
		AddClusterAssoc(CTL_SUPPLAGT_INVPACTION, 1, PPSupplAgreement::invpaWarning);
		AddClusterAssoc(CTL_SUPPLAGT_INVPACTION, 2, PPSupplAgreement::invpaDoNothing);
		SetClusterData(CTL_SUPPLAGT_INVPACTION, inv_price_action);

		AddClusterAssoc(CTL_SUPPLAGT_FLAGS2, 0, AGTF_USESDONPURCHOP);
		SetClusterData(CTL_SUPPLAGT_FLAGS2, Data.Flags);
		enableCommand(cmBills, 0);
	}
	else {
		AddClusterAssoc(CTL_SUPPLAGT_FLAGS, 0, AGTF_USEMARKEDGOODSONLY);
		AddClusterAssoc(CTL_SUPPLAGT_FLAGS, 1, AGTF_SUBCOSTONSUBPARTSTR);
		AddClusterAssoc(CTL_SUPPLAGT_FLAGS, 2, AGTF_AUTOORDER);
		SetClusterData(CTL_SUPPLAGT_FLAGS, Data.Flags);
		// @v8.5.2 long v = (long)Data.OrdPrdDays;
		// @v8.5.2 setCtrlLong(CTL_SUPPLAGT_ORDPRD, v);
	}
	setupCtrls(Data.Flags);
	updateList(-1);
	return 1;
}

int SupplAgtDialog::getDTS(PPSupplAgreement * pAgt)
{
	int    ok = 1, sel = 0;
	getCtrlData(sel = CTL_SUPPLAGT_DATE,   &Data.BegDt);
	THROW_SL(checkdate(Data.BegDt, 1));
	getCtrlData(sel = CTL_SUPPLAGT_EXPIRY, &Data.Expiry);
	THROW_SL(checkdate(Data.Expiry, 1));
	getCtrlData(CTL_SUPPLAGT_PAYPERIOD,    &Data.DefPayPeriod);
	getCtrlData(CTL_SUPPLAGT_DELIVERY,     &Data.DefDlvrTerm);
	getCtrlData(sel = CTL_SUPPLAGT_MAXRETURN, &Data.PctRet);
	THROW_PP(Data.PctRet >= 0L && Data.PctRet <= 100, PPERR_USERINPUT);
	getCtrlData(CTLSEL_SUPPLAGT_AGENT,     &Data.DefAgentID);
	getCtrlData(CTLSEL_SUPPLAGT_OPRKIND,   &Data.PurchaseOpID);
	getCtrlData(CTLSEL_SUPPLAGT_QUOTCOST,  &Data.CostQuotKindID);
	getCtrlData(CTLSEL_SUPPLAGT_QUOTUP,    &Data.DevUpQuotKindID);
	getCtrlData(CTLSEL_SUPPLAGT_QUOTDOWN,  &Data.DevDnQuotKindID);
	getCtrlData(CTLSEL_SUPPLAGT_MNGRREL,   &Data.MngrRelID);
	if(!ArID) {
		GetClusterData(CTL_SUPPLAGT_INVPACTION, &Data.InvPriceAction);
		GetClusterData(CTL_SUPPLAGT_FLAGS2, &Data.Flags);
	}
	else {
		GetClusterData(CTL_SUPPLAGT_FLAGS, &Data.Flags);
		// @v8.5.2 Data.OrdPrdDays = (int16)getCtrlLong(CTL_SUPPLAGT_ORDPRD);
		// @v8.5.2 THROW_PP(!(Data.Flags & AGTF_AUTOORDER) || Data.OrdPrdDays > 0, PPERR_INVPERIODINPUT);
	}
	ASSIGN_PTR(pAgt, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

// static
int SLAPI PPObjArticle::EditSupplAgreement(PPSupplAgreement * pAgt)
{
	DIALOG_PROC_BODY_P1(SupplAgtDialog, pAgt ? pAgt->SupplID : 0, pAgt);
}

// static
int SLAPI PPObjArticle::DefaultSupplAgreement()
{
	int    ok = 1;
	PPObjArticle arobj;
	PPSupplAgreement agt;
	THROW(CheckCfgRights(PPCFGOBJ_SUPPLDEAL, PPR_MOD, 0));
	THROW(arobj.CheckRights(ARTRT_CLIAGT));
	THROW(arobj.GetSupplAgreement(0, &agt, 0));
	if(arobj.EditSupplAgreement(&agt) > 0)
		THROW(arobj.PutSupplAgreement(0, &agt, 1));
	CATCHZOKPPERR
	return ok;
}

//static
int SLAPI PPObjArticle::GetAgreementKind(const ArticleTbl::Rec * pArRec)
{
	int    ok = -1;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	THROW_INVARG(pArRec);
	THROW(acs_obj.Fetch(pArRec->AccSheetID, &acs_rec) > 0);
	if(pArRec->AccSheetID == GetSupplAccSheet() && acs_rec.Flags & ACSHF_USESUPPLAGT)
		ok = 2;
	else if(pArRec->AccSheetID == GetSellAccSheet() || acs_rec.Flags & ACSHF_USECLIAGT)
		ok = 1;
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_Agreement
//
PPALDD_CONSTRUCTOR(Agreement)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjArticle;
	}
}

PPALDD_DESTRUCTOR(Agreement)
{
	Destroy();
	delete (PPObjArticle*)Extra[0].Ptr;
	Extra[0].Ptr = 0;
}

int PPALDD_Agreement::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		SString temp_buf;
		ArticleTbl::Rec  ar_rec;
		PPObjArticle * p_ar_obj = (PPObjArticle *)(Extra[0].Ptr);
		if(p_ar_obj->Search(rFilt.ID, &ar_rec) > 0) {
			PPAccSheet acs_rec;
			PPObjAccSheet acc_sheet_obj;
			if(acc_sheet_obj.Fetch(ar_rec.AccSheetID, &acs_rec) > 0) {
				PPClientAgreement  cli_agt;
				PPSupplAgreement   suppl_agt;
				if((acs_rec.Flags & ACSHF_USECLIAGT) && p_ar_obj->GetClientAgreement(H.ID, &cli_agt, 0) > 0) {
					H.AgentID      = cli_agt.DefAgentID;
					H.ExtObjectID  = cli_agt.ExtObjectID; // @v7.5.9
					H.QKindID      = cli_agt.DefQuotKindID;
					H.BegDt        = cli_agt.BegDt;
					H.Expiry       = cli_agt.Expiry;
					H.MaxCredit    = cli_agt.MaxCredit;
					H.MaxDscnt     = cli_agt.MaxDscnt;
					H.Dscnt        = cli_agt.Dscnt;
					H.DefPayPeriod = cli_agt.DefPayPeriod;
					STRNSCPY(H.Code, cli_agt.Code);
				}
				else if((acs_rec.Flags & ACSHF_USESUPPLAGT) && p_ar_obj->GetSupplAgreement(H.ID, &suppl_agt, 0) > 0) {
					H.AgentID      = suppl_agt.DefAgentID;
					H.BegDt        = suppl_agt.BegDt;
					H.Expiry       = suppl_agt.Expiry;
					H.DefPayPeriod = suppl_agt.DefPayPeriod;
					H.DefDlvrTerm  = suppl_agt.DefDlvrTerm;
					H.PctRet       = suppl_agt.PctRet;
					// @v8.5.0 {
                    suppl_agt.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, temp_buf = 0);
                    temp_buf.CopyTo(H.EDIPrvdrSymb, sizeof(H.EDIPrvdrSymb));
                    // } @v8.5.0
					// @v8.5.0 STRNSCPY(H.EDIPrvdrSymb, suppl_agt.ExchCfg.PrvdrSymb); // @v8.0.6
				}
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
