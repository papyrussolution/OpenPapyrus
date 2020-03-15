// ATRNTMPL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Шаблон бухгалтерской проводки
//
#include <pp.h>
#pragma hdrstop

SLAPI PPAccTurn::PPAccTurn() : DbtSheet(0), CrdSheet(0), Date(ZERODATE), BillID(0), RByBill(0), Reserve(0), CurID(0), CRate(0.0), Amount(0.0), Opr(0), Flags(0)
{
	PTR32(BillCode)[0] = 0;
}

int FASTCALL PPAccTurn::IsEqual(const PPAccTurn & rS) const
{
	int    eq = 1;
	if(DbtID != rS.DbtID)
		eq = 0;
	/* @v9.0.0 DbtSheet избыточное поле и инициализируется по DbtID
	else if(DbtSheet != rS.DbtSheet)
		eq = 0;
	*/
	else if(CrdID != rS.CrdID)
		eq = 0;
	/* @v9.0.0 CrdSheet избыточное поле и инициализируется по CrdID
	else if(CrdSheet != rS.CrdSheet)
		eq = 0;
	*/
	else if(Date != rS.Date)
		eq = 0;
	else if(BillID != rS.BillID)
		eq = 0;
	else if(CurID != rS.CurID)
		eq = 0;
	else if(!feqeps(R2(Amount), R2(rS.Amount), 1e-6)) // @v8.1.2 R2()
		eq = 0;
	else if(Opr != rS.Opr)
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(strcmp(BillCode, rS.BillCode) != 0)
		eq = 0;
	/*
	else if(RByBill != rS.RByBill)
		eq = 0;
	*/
	return eq;
}

void SLAPI PPAccTurn::SwapDbtCrd()
{
	memswap(&DbtID, &CrdID, sizeof(AcctID));
	Exchange(&DbtSheet, &CrdSheet);
}
//
//
//
PPAccTurnTempl::ATSubstObjects::ATSubstObjects()
{
}

void PPAccTurnTempl::ATSubstObjects::destroy()
{
	PrimList.clear();
	ForeignList.clear();
}

PPAccTurnTempl::ATSubstObjects::Item::Item() : AcsID(0)
{
	// @v10.7.3 @ctr Aid.Z();
}
//
//
//
class PPAccTurnTempl_Before6406 { // size=PROPRECFIXSIZE
public:
	PPID   ObjType;       // Const=PPOBJ_OPRKIND
	PPID   ObjID;         // ->Ref(PPOBJ_OPRKIND)
	PPID   ID;            // Номер шаблона для операции (1..PP_MAXATURNTEMPLATES)
	AcctID DbtID;
	AcctID CrdID;
	long   Flags;
	char   Expr[52];      // Формула для суммы проводки (текст)
	//
	// Следующее поле целиком содержится в хвосте переменной длины записи таблицы Property.
	// Для того, чтобы после прочтения из БД оно содержало корректное значение, его необходимо
	// обнулить перед вызовом Reference::GetProp.
	//
	// В это поле сначала заносится порядок подстановки для первичного объекта, затем разделитель -1L,
	// затем порядок подстановки для вторичного объекта, затем двоичный нуль (0L).
	// Если для первичного объекта нет явного порядка, то Subst[0] == -1L.
	// Завершающий нуль должен присутствовать всегда (смотри предыдущий параграф).
	//
	// Теоретически реальное поле Subst может иметь длину больше чем sizeof(Subst).
	// Если это случится (Subst[7] != 0), то необходимо как-то сигнализировать об исключительной ситуации.
	//
	PPID   Subst[8];      // Переменные объектов для подстановки в проводку
	DateRange Period;     // Период действия шаблона
};

//static
int SLAPI PPAccTurnTempl::Convert_6407(PropertyTbl::Rec * pRec)
{
	int    ok = 1;
	PPAccTurnTempl_Before6406 * p_b6406 = reinterpret_cast<PPAccTurnTempl_Before6406 *>(pRec);
	if(p_b6406->Flags & ATTF_CVT6406) {
		ok = -1;
	}
	else {
		PPAccTurnTempl rec;
		MEMSZERO(rec);
#define __FLD(f) rec.f = p_b6406->f
		__FLD(ObjType);
		__FLD(ObjID);
		__FLD(ID);
		__FLD(DbtID);
		__FLD(CrdID);
		__FLD(Flags);
		__FLD(Period);
#undef __FLD
		STRNSCPY(rec.Expr, p_b6406->Expr);
		memcpy(rec.Subst, p_b6406->Subst, sizeof(p_b6406->Subst));
		rec.Flags |= ATTF_CVT6406;
		memcpy(pRec, &rec, sizeof(rec));
		ok = 1;
	}
	return ok;
}

#define LINKFLAG   0x00008000L
#define RCKNFLAG   0x00010000L
#define PARENTFLAG 0x00020000L
#define LMASK      (LINKFLAG|RCKNFLAG|PARENTFLAG)

int SLAPI PPAccTurnTempl::GetObjByVar(PPID var, ATBillParam * pParam, PPID * pObjID) const
{
	/*
	PPSYM_LOCATION
	PPSYM_OBJECT
	PPSYM_BILLOBJ2
	PPSYM_PAYER
	PPSYM_AGENT
	PPSYM_ADVLNACC
	PPSYM_ADVLNAR
	*/
	int    ok = 1;
	int    use_parent = 0;
	PPID   obj_id = 0;
	PPBillPacket * p_pack = 0;
	if(pParam) {
		if(var & LINKFLAG)
			p_pack = pParam->P_LinkPack;
		else if(var & RCKNFLAG)
			p_pack = pParam->P_RcknPack;
		else
			p_pack = pParam->P_Pack;
	}
	switch(var & ~LMASK) {
		case PPSYM_LOCATION:
			if(p_pack && p_pack->Rec.LocID)
				if((obj_id = PPObjLocation::WarehouseToObj(p_pack->Rec.LocID)) == 0)
					ok = 0;
			break;
		case PPSYM_OBJECT:
			if(p_pack) {
				obj_id = p_pack->Rec.Object;
				if(var & PARENTFLAG)
					use_parent = 1;
			}
			break;
		case PPSYM_BILLOBJ2:
			if(p_pack) {
				obj_id = p_pack->Rec.Object2;
				if(var & PARENTFLAG)
					use_parent = 1;
			}
			break;
		case PPSYM_PAYER:
			if(p_pack) {
				obj_id = p_pack->Ext.PayerID;
				if(var & PARENTFLAG)
					use_parent = 1;
			}
			break;
		case PPSYM_AGENT:
			if(p_pack) {
				obj_id = p_pack->Ext.AgentID;
				if(var & PARENTFLAG)
					use_parent = 1;
			}
			break;
		case PPSYM_ADVLNACC:
			if(p_pack) {
				if(!(pParam->Flags & ATBillParam::fAr)) {
					PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(pParam->AdvItemIdx);
					if(&r_item)
						obj_id = r_item.AccID;
				}
			}
			break;
		case PPSYM_ADVLNAR:
			if(p_pack) {
				if(pParam->Flags & ATBillParam::fAr) {
					if(pParam->AccSheetID) {
						PPObjArticle ar_obj;
						ArticleTbl::Rec ar_rec;
						if(ar_obj.P_Tbl->SearchNum(pParam->AccSheetID, pParam->AdvItemIdx, &ar_rec) > 0)
							obj_id = ar_rec.ID;
					}
				}
				else {
					PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(pParam->AdvItemIdx);
					if(&r_item)
						obj_id = r_item.ArID;
				}
			}
			break;
		default:
			ok = PPSetError(PPERR_INVATTSUBSTVAR);
			break;
	}
	if(use_parent && obj_id) {
		PPObjArticle ar_obj;
		PPID   rel_ar_id = 0;
		if(ar_obj.GetRelPersonSingle(obj_id, PPPSNRELTYP_AFFIL, 0, &rel_ar_id) > 0)
			obj_id = rel_ar_id;
	}
	ASSIGN_PTR(pObjID, obj_id);
	return ok;
}

static int FASTCALL IsAccAssocArticle(PPID arID, PPID * pAccID)
{
	int    ok = 0;
	PPID   acs_id = 0, obj_id = 0;
	PPObjAccSheet acs_obj;
	if(GetArticleSheetID(arID, &acs_id, &obj_id) > 0) {
		if(acs_obj.IsAssoc(acs_id, PPOBJ_ACCOUNT2, 0) > 0 || acs_obj.IsAssoc(acs_id, PPOBJ_ACCOUNT_PRE9004, 0) > 0)
			ok = 1;
	}
	ASSIGN_PTR(pAccID, obj_id);
	return ok;
}

	// <<PPAccTurnTempl::SetupAccounts()
int SLAPI PPAccTurnTempl::GetSubstObjects(ATBillParam * pParam, ATSubstObjects * pAtso, int byAcc) const
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	int    count = 0;
	int    ord = 0;
	int    is_prim_list_present = 0;
	int    is_foreign_list_present = 0;
	pAtso->destroy();
	for(const PPID * p = Subst; *p != 0; p++, count++) {
		THROW_PP(count < SIZEOFARRAY(Subst), PPERR_UNTERMATTSUBST);
		if(*p == -1L) {
			ord = 1;
		}
		else {
			if(ord == 0)
				is_prim_list_present = 1;
			else
				is_foreign_list_present = 1;
			ATSubstObjects::Item item;
			{
				if(*p & LINKFLAG) {
					if(pParam->P_Pack->Rec.LinkBillID && pParam->P_LinkPack == 0) {
						THROW_MEM(pParam->P_LinkPack = new PPBillPacket);
						THROW(p_bobj->ExtractPacketWithFlags(pParam->P_Pack->Rec.LinkBillID, pParam->P_LinkPack, BPLD_SKIPTRFR));
					}
				}
				else if(*p & RCKNFLAG) {
					if(!(pParam->Flags & ATBillParam::fIsRcknInited)) {
						if(pParam->P_Pack->PaymBillID) {
							THROW_MEM(pParam->P_RcknPack = new PPBillPacket);
							THROW(p_bobj->ExtractPacketWithFlags(pParam->P_Pack->PaymBillID, pParam->P_RcknPack, BPLD_SKIPTRFR));
					    }
						pParam->Flags |= ATBillParam::fIsRcknInited;
				    }
				}
				PPID   temp_ac_id = 0;
				PPID   temp_ar_id = 0;
				THROW(GetObjByVar(*p, pParam, &temp_ar_id));
				if(byAcc) {
					if(*p == PPSYM_ADVLNACC) {
						item.Aid.ac = temp_ar_id;
						item.Aid.ar = 0;
					}
					else if(IsAccAssocArticle(temp_ar_id, &temp_ac_id)) {
						item.Aid.ac = temp_ac_id;
						item.Aid.ar = temp_ar_id;
					}
					else {
					}
				}
				else if(*p == PPSYM_ADVLNACC) {
					item.Aid.ar = 0;
				}
				else {
					item.Aid.ac = temp_ac_id;
					item.Aid.ar = temp_ar_id;
				}
				if(item.Aid.ac || item.Aid.ar) {
					if(ord == 0)
						pAtso->PrimList.insert(&item);
					else
						pAtso->ForeignList.insert(&item);
				}
			}
		}
	}
	if(!is_prim_list_present) {
		if(pParam->P_Pack->Rec.LocID) {
			ATSubstObjects::Item item;
			THROW(item.Aid.ar = PPObjLocation::WarehouseToObj(pParam->P_Pack->Rec.LocID));
			item.AcsID = LConfig.LocAccSheetID;
			pAtso->PrimList.insert(&item);
		}
	}
	if(!is_foreign_list_present) {
		ATSubstObjects::Item item;
		item.Aid.ar = pParam->P_Pack->Rec.Object;
		item.AcsID = pParam->P_Pack->AccSheetID;
		pAtso->ForeignList.insert(&item);
	}
	{
		uint c = pAtso->PrimList.getCount();
		if(c) do {
			ATSubstObjects::Item & r_item = pAtso->PrimList.at(--c);
			if(byAcc && !r_item.Aid.ac && !IsAccAssocArticle(r_item.Aid.ar, &r_item.Aid.ac))
				pAtso->PrimList.atFree(c);
			else if(!byAcc && GetArticleSheetID(r_item.Aid.ar, &r_item.AcsID) <= 0)
				pAtso->PrimList.atFree(c);
		} while(c);
	}
	{
		uint c = pAtso->ForeignList.getCount();
		if(c) do {
			ATSubstObjects::Item & r_item = pAtso->ForeignList.at(--c);
			if(byAcc && !r_item.Aid.ac && !IsAccAssocArticle(r_item.Aid.ar, &r_item.Aid.ac))
				pAtso->ForeignList.atFree(c);
			else if(GetArticleSheetID(r_item.Aid.ar, &r_item.AcsID) <= 0)
				pAtso->ForeignList.atFree(c);
		} while(c);
	}
	CATCHZOK
	return ok;
}

	// <<PPAccTurnTempl::SetupAccounts()
int SLAPI PPAccTurnTempl::SubstAcct(int side, PPAccTurn * at, const ATSubstObjects * atso, const AcctID * pDbt, const AcctID * pCrd) const
{
	int      ok    = 1;
	PPObjBill * p_bobj = BillObj;
	int      subst = 0;		// 1 - prim, 2 - foreign, 3 - both
	PPID   & r_sheet_id  = (side == PPDEBIT) ? at->DbtSheet : at->CrdSheet;
	AcctID & r_acctid = (side == PPDEBIT) ? at->DbtID    : at->CrdID;
	long     artfix = (side == PPDEBIT) ? ATTF_DARTFIX : ATTF_CARTFIX;
	int      primOnCrd = (Flags & ATTF_PRIMONCREDIT) ? 1 : 0;
	PPObjAccTurn * p_atobj = p_bobj->atobj;
	PPAccount acrec;
	r_acctid.ac = (side == PPDEBIT) ? pDbt->ac : pCrd->ac;
	r_acctid.ar = (side == PPDEBIT) ? pDbt->ar : pCrd->ar;
	if(!(Flags & artfix)) {
		uint    prim_subst_pos = 0;
		uint    foreign_subst_pos = 0;
		THROW(p_atobj->P_Tbl->AccObj.Search(r_acctid.ac, &acrec) > 0);
		r_sheet_id = acrec.AccSheetID;
		if(r_sheet_id != 0) {
			{
				int    prim_subst = 0;
				for(uint i = 0; !prim_subst && i < atso->PrimList.getCount(); i++) {
					if(atso->PrimList.at(i).AcsID == r_sheet_id) {
						prim_subst_pos = i+1;
						prim_subst = 1;
					}
				}
				subst = prim_subst;
			}
			{
				int    foreign_subst = 0;
				for(uint i = 0; !foreign_subst && i < atso->ForeignList.getCount(); i++) {
					if(atso->ForeignList.at(i).AcsID == r_sheet_id) {
						if(subst)
							if((primOnCrd && side == PPCREDIT) || (!primOnCrd && side == PPDEBIT))
								foreign_subst = 1;
							else
								foreign_subst = 2;
						else
							foreign_subst = 2;
						foreign_subst_pos = i+1;
					}
				}
				if(foreign_subst)
					subst = foreign_subst;
			}
			if(subst) {
				if(subst == 1) {
					if(prim_subst_pos)
						r_acctid.ar = atso->PrimList.at(prim_subst_pos-1).Aid.ar;
				}
				else {
					if(foreign_subst_pos)
						r_acctid.ar = atso->ForeignList.at(foreign_subst_pos-1).Aid.ar;
				}
			}
			else {
				PPID   foreign_ar_id = 0;
				for(uint i = 0; i < atso->ForeignList.getCount(); i++) {
					foreign_ar_id = atso->ForeignList.at(i).Aid.ar;
					THROW(p_bobj->GetAlternateArticle(foreign_ar_id, r_sheet_id, &r_acctid.ar));
				}
				if(r_acctid.ar <= 0) {
					SString msg_buf, name_buf;
					Acct   acct;
					PPID   cur_id = 0;
					p_atobj->ConvertAcctID(r_acctid, &acct, &cur_id, 0);
					GetArticleName(foreign_ar_id, msg_buf);
					msg_buf.Quot('\'', '\'').CatChar('-').CatChar('>').Cat(acct.ToStr(0, name_buf));
					GetObjectName(PPOBJ_ACCSHEET, r_sheet_id, name_buf, 0);
					msg_buf.Space().Cat(name_buf.Quot('(', ')'));
					CALLEXCEPT_PP_S(PPERR_UNABLESUBSTOBJ, msg_buf);
				}
			}
		}
		else
			r_acctid.ar = 0;
	}
	CATCH
		r_acctid.Z();
		ok = 0;
	ENDCATCH
	return ok;
}

	// <<PPAccTurnTempl::SetupAccounts()
int SLAPI PPAccTurnTempl::ResolveAlias(int side, AcctID * pAcct, const ATSubstObjects * pAtso) const
{
	int    ok = 1;
	PPObjAccount & r_acc_obj = BillObj->atobj->P_Tbl->AccObj;
	PPAccount acc_rec;
	if(pAcct->ac && r_acc_obj.Fetch(pAcct->ac, &acc_rec) > 0 && acc_rec.Type == ACY_ALIAS) {
		LAssocArray alias_subst;
		PPID   unresolved_ar_id = 0; // Для сообщения об ошибке
		const  TSVector <ATSubstObjects::Item> * p_atso_list = 0; // @v9.8.4 TSArray-->TSVector
		if(side == PPDEBIT) {
			p_atso_list = (Flags & ATTF_PRIMONCREDIT) ? &pAtso->ForeignList : &pAtso->PrimList;
		}
		else { // side == PPCREDIT
			p_atso_list = (Flags & ATTF_PRIMONCREDIT) ? &pAtso->PrimList : &pAtso->ForeignList;
		}
		assert(p_atso_list);
		int    found = 0;
		for(uint i = 0; !found && i < p_atso_list->getCount(); i++) {
			const PPID ar_id = p_atso_list->at(0).Aid.ar;
			PPID   acc_id = 0;
			alias_subst.clear();
			if(PPObjArticle::GetAliasSubst(ar_id, &alias_subst) > 0 && alias_subst.Search(pAcct->ac, &acc_id, 0)) {
				pAcct->ac = acc_id;
				found = 1;
			}
			else
				unresolved_ar_id = ar_id;
		}
		if(!found) {
			if(Flags & ATTF_SKIPEMPTYALIAS) {
				ok = -1;
			}
			else {
				SString msg_buf, ar_name;
				GetArticleName(unresolved_ar_id, ar_name);
				msg_buf.Cat(acc_rec.Name).CatDiv('>', 1).Cat(ar_name);
				ok = PPSetError(PPERR_UNABLERESOLVEALIAS, msg_buf);
			}
		}
	}
	return ok;
}

int SLAPI PPAccTurnTempl::SetupAccounts(ATBillParam & rParam, PPID curID, PPAccTurn * pAT) const
{
	int    ok = 1;
	double cur_rate = 1.0;
	ATSubstObjects  atso;
	PPObjAccount & r_acc_obj = BillObj->atobj->P_Tbl->AccObj;
	//AccountCore & r_acc_core = *r_acc_obj.P_Tbl;
	PPAccount acc_rec;
	AcctID dbt = DbtID;
	AcctID crd = CrdID;
	if(curID != 0 && curID != LConfig.BaseCurID) {
		rParam.P_Pack->Amounts.Get(PPAMT_CRATE, curID, &cur_rate);
		if(cur_rate == 0.0)
			cur_rate = 1.0;
	}
	pAT->CurID = curID;
	pAT->CRate = cur_rate;
	if(pAT->Amount != 0.0 && (!(Flags & ATTF_SKIPNEG) || pAT->Amount > 0.0)) {
		int    is_outbal_ac = 0;
		{
			ATSubstObjects atso_alias;
			int    is_atso_alias_inited = 0;
			AcctID aid;
			if(dbt.ac && r_acc_obj.Fetch(dbt.ac, &acc_rec) > 0 && acc_rec.Type == ACY_ALIAS) {
				int   r_alias = 0;
				aid = dbt;
				if(!is_atso_alias_inited) {
					THROW(GetSubstObjects(&rParam, &atso_alias, 0));
					is_atso_alias_inited = 1;
				}
				r_alias = ResolveAlias(PPDEBIT, &aid, &atso_alias);
				THROW(r_alias);
				if(r_alias > 0)
					dbt.ac = aid.ac;
				else
					ok = -1;
			}
			if(crd.ac && r_acc_obj.Fetch(crd.ac, &acc_rec) > 0 && acc_rec.Type == ACY_ALIAS) {
				int   r_alias = 0;
				aid = crd;
				if(!is_atso_alias_inited) {
					THROW(GetSubstObjects(&rParam, &atso_alias, 0));
					is_atso_alias_inited = 1;
				}
				r_alias = ResolveAlias(PPCREDIT, &aid, &atso_alias);
				THROW(r_alias);
				if(r_alias > 0)
					crd.ac = aid.ac;
				else
					ok = -1;
			}
		}
		if(ok > 0) {
			if(dbt.ac == 0 || crd.ac == 0) {
				ATSubstObjects acc_atso;
				THROW(GetSubstObjects(&rParam, &acc_atso, 1));
				if(dbt.ac) {
					if(!acc_atso.ForeignList.getCount() || !acc_atso.ForeignList.at(0).Aid.ac) {
						if(acc_atso.PrimList.getCount())
							crd.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
					else if(!acc_atso.PrimList.getCount() || !acc_atso.PrimList.at(0).Aid.ac) {
						if(acc_atso.ForeignList.getCount())
							crd.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
					else if(Flags & ATTF_PRIMONCREDIT) {
						if(acc_atso.PrimList.getCount())
							crd.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
					else {
						if(acc_atso.ForeignList.getCount())
							crd.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
				}
				else if(crd.ac) {
					if(!acc_atso.ForeignList.getCount() || !acc_atso.ForeignList.at(0).Aid.ac) {
						if(acc_atso.PrimList.getCount())
							dbt.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
					else if(!acc_atso.PrimList.getCount() || !acc_atso.PrimList.at(0).Aid.ac) {
						if(acc_atso.ForeignList.getCount())
							dbt.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
					else if(Flags & ATTF_PRIMONCREDIT) {
						if(acc_atso.ForeignList.getCount())
							dbt.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
					else {
						if(acc_atso.PrimList.getCount())
							dbt.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
				}
				else if(acc_atso.ForeignList.getCount() == 0 || acc_atso.ForeignList.at(0).Aid.ac == 0) {
					if(acc_atso.PrimList.getCount()) {
						dbt.ac = crd.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
				}
				else if(acc_atso.PrimList.getCount() == 0 || acc_atso.PrimList.at(0).Aid.ac == 0) {
					if(acc_atso.ForeignList.getCount()) {
						dbt.ac = crd.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
				}
				else if(Flags & ATTF_PRIMONCREDIT) {
					if(acc_atso.ForeignList.getCount()) {
						dbt.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
					if(acc_atso.PrimList.getCount()) {
						crd.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
				}
				else {
					if(acc_atso.PrimList.getCount()) {
						dbt.ac = acc_atso.PrimList.at(0).Aid.ac;
					}
					if(acc_atso.ForeignList.getCount()) {
						crd.ac = acc_atso.ForeignList.at(0).Aid.ac;
					}
				}
			}
			THROW(GetSubstObjects(&rParam, &atso, 0)); // Инициализация atso. Далее этот блок не меняется.
			// @# atso=const {
			if((!atso.PrimList.getCount() || !atso.PrimList.at(0).Aid.ar) && Flags & ATTF_PSKIPONZOBJ)
				ok = -1;
			if((!atso.ForeignList.getCount() || !atso.ForeignList.at(0).Aid.ar) && Flags & ATTF_PSKIPONZOBJ)
				ok = -1;
			else {
				int    r1 = 0, r2 = 0;
				THROW(r1 = ResolveAlias(PPDEBIT,  &dbt, &atso));
				THROW(r2 = ResolveAlias(PPCREDIT, &crd, &atso));
				if(r1 < 0 || r2 < 0)
					ok = -1;
				else {
					THROW_PP(dbt.ac && r_acc_obj.Search(dbt.ac, &acc_rec) > 0, PPERR_ATTMUSTBEFIX);
					if(oneof2(acc_rec.Type, ACY_OBAL, ACY_REGISTER))
						is_outbal_ac = 1;
					SETFLAG(pAT->Flags, PPAF_OUTBAL, is_outbal_ac);
					SETFLAG(pAT->Flags, PPAF_OUTBAL_TRANSFER, is_outbal_ac && crd.ac);
					THROW_PP(crd.ac || (pAT->Flags & PPAF_OUTBAL), PPERR_ATTMUSTBEFIX);
					if(curID) {
						PPID   cur_acc_id = 0;
						THROW(r_acc_obj.SearchCur(dbt.ac, curID, &cur_acc_id, 0));
						dbt.ac = cur_acc_id;
						if(crd.ac) {
							THROW(r_acc_obj.SearchCur(crd.ac, curID, &cur_acc_id, 0));
							crd.ac = cur_acc_id;
						}
					}
					THROW(SubstAcct(PPDEBIT, pAT, &atso, &dbt, &crd));
					if(crd.ac) {
						THROW(SubstAcct(PPCREDIT, pAT, &atso, &dbt, &crd));
						if(pAT->Amount < 0 && Flags & ATTF_INVERTNEG) {
							pAT->SwapDbtCrd();
							pAT->Amount = -pAT->Amount;
						}
					}
				}
			}
			// } @# atso=const
		}
	}
	else
		ok = -1;
	CATCHZOK
	ZDELETE(rParam.P_LinkPack);
	ZDELETE(rParam.P_RcknPack);
	return ok;
}

int SLAPI PPAccTurnTempl::EnumerateExtLines(const PPBillPacket * pPack, ExtLinesBlock * pBlk) const
{
	int    ok = 0;
	if(pPack) {
		pBlk->Idx = 0;
		pBlk->AccSheetID = 0;
		pBlk->SubstAr = 0;
		pBlk->SubstArList.clear();
		pBlk->P_Pack = pPack;
		if(pPack->OpTypeID == PPOPT_ACCTURN && GetOpSubType(pPack->Rec.OpID) == OPSUBT_ACCWROFF) {
			pBlk->AccWrOff = 1;
			PPOprKind op_rec;
			if(GetOpData(pPack->Rec.OpID, &op_rec) > 0 && op_rec.AccSheetID) {
				PPObjArticle ar_obj;
				pBlk->AccSheetID = op_rec.AccSheetID;
				ar_obj.P_Tbl->GetListBySheet(op_rec.AccSheetID, &pBlk->SubstArList, 0);
			}
		}
		else
			pBlk->AccWrOff = 0;
	}
	else
		pBlk->Idx++;
	if(Flags & ATTF_BYADVLINES) {
		if(pBlk->AccWrOff) {
			PPObjArticle ar_obj;
			while(!ok && pBlk->Idx < pBlk->SubstArList.getCount()) {
				ArticleTbl::Rec ar_rec;
				if(ar_obj.Fetch(pBlk->SubstArList.get(pBlk->Idx), &ar_rec) > 0) {
					pBlk->SubstAr = ar_rec.Article;
					ok = 2;
				}
				else
					pBlk->Idx++;
			}
		}
		else {
			PPObjAdvBillKind abk_obj;
			while(!ok && pBlk->Idx < pBlk->P_Pack->AdvList.GetCount()) {
				const PPAdvBillItemList::Item & r_abi = pBlk->P_Pack->AdvList.Get(pBlk->Idx);
				PPAdvBillKind abk_rec;
				if(&r_abi) {
					if(r_abi.AdvBillKindID && BillObj->Search(r_abi.AdvBillID, 0) > 0 &&
						abk_obj.Search(r_abi.AdvBillKindID, &abk_rec) > 0 && abk_rec.Flags & PPAdvBillKind::fSkipAccturn)
						ok = -1;
					else {
						pBlk->SubstAr = pBlk->Idx;
						ok = 1;
					}
				}
				else
					pBlk->Idx++;
			}
		}
	}
	return ok;
}

int SLAPI PPAccTurnTempl::CreateBaseProjectionAccturns(PPBillPacket * pPack)
{
	int    ok = 1;
	PPIDArray cur_list;
	pPack->Amounts.GetCurList(-1L, &cur_list);
	uint   adv_item_idx = 0;
	PPAccTurn pattern_at;
	pPack->CreateAccTurn(&pattern_at);
	if(Period.CheckDate(pattern_at.Date)) {
		ExtLinesBlock elb;
		int    r = EnumerateExtLines(pPack, &elb);
		do {
			if(r >= 0) {
				PPAccTurn at = pattern_at;
				for(uint j = 0; j < cur_list.getCount(); j++) {
					PPID   cur_id   = cur_list.at(j);
					double temp_amt = 0.0;
					if(Flags & ATTF_EXPRESSION) {
						THROW(PPCalcExpression(Expr, &temp_amt, pPack, cur_id, elb.SubstAr));
					}
					else if(pPack->Rec.CurID == cur_id)
						if(r == 1) {
			   				if(&pPack->AdvList.Get(elb.Idx))
								temp_amt = pPack->AdvList.Get(elb.Idx).Amount;
						}
						else
							temp_amt = BR2(pPack->Rec.Amount);
					at.Amount += R2(temp_amt);
				}
				if(Flags & ATTF_INTROUNDING)
					at.Amount = R0(at.Amount);
				{
					ATBillParam param;
					MEMSZERO(param);
					param.P_Pack = pPack;
					param.AdvItemIdx = elb.SubstAr;
					if(r == 2) {
						param.AccSheetID = elb.AccSheetID;
						param.Flags |= ATBillParam::fAr;
					}
					THROW(ok = SetupAccounts(param, 0L, &at));
				}
				if(ok > 0)
					THROW_SL(pPack->Turns.insert(&at));
			}
		} while((r = EnumerateExtLines(0, &elb)) != 0);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPAccTurnTempl::CreateAccturns(PPBillPacket * pPack)
{
	int    ok = 1;
	int    r = 0;
	pPack->ErrCause = 0;
	PPIDArray cur_list;
	BillObj->atobj->P_Tbl->AccObj.GetIntersectCurList(DbtID.ac, CrdID.ac, &cur_list);
	for(uint j = 0; j < cur_list.getCount(); j++) {
		const PPID cur_id = cur_list.at(j);
		PPAccTurn pattern_at;
		pPack->CreateAccTurn(&pattern_at);
		if(Period.CheckDate(pattern_at.Date)) {
			ExtLinesBlock elb;
			r = EnumerateExtLines(pPack, &elb);
			do {
				pPack->ErrCause = 0;
				if(r >= 0) {
					if(r == 1) {
						pPack->ErrCause = PPBillPacket::err_on_advline;
						pPack->ErrLine = elb.Idx;
					}
					PPAccTurn at = pattern_at;
					if(Flags & ATTF_EXPRESSION) {
						THROW(PPCalcExpression(Expr, &at.Amount, pPack, cur_id, elb.SubstAr));
					}
					else if(pPack->Rec.CurID == cur_id)
						if(r == 1) {
							const PPAdvBillItemList::Item & r_abi = pPack->AdvList.Get(elb.Idx);
							if(&r_abi)
								at.Amount = r_abi.Amount;
						}
						else
							at.Amount = BR2(pPack->Rec.Amount);
					if(Flags & ATTF_INTROUNDING)
						at.Amount = R0(at.Amount);
					{
						ATBillParam param;
						MEMSZERO(param);
						param.P_Pack = pPack;
						param.AdvItemIdx = elb.SubstAr;
						if(r == 2) {
							param.AccSheetID = elb.AccSheetID;
							param.Flags |= ATBillParam::fAr;
						}
						THROW(ok = SetupAccounts(param, cur_id, &at));
					}
					if(ok > 0)
						THROW_SL(pPack->Turns.insert(&at));
				}
			} while((r = EnumerateExtLines(0, &elb)) != 0);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPAccTurnTempl::ParseSubstString(const char * str, int * lp, int * _skipzobj)
{
	int    ok = 1;
	if(str) {
		PPSymbTranslator st;
		int    skipzobj = 0;
		SString s;
		(s = str).Strip();
		if(s[0] == '!') {
			skipzobj = 1;
			s.ShiftLeft();
		}
		StringSet ss(',', s);
		for(uint ssp = 0; ok && ss.get(&ssp, s);) {
			if(s.NotEmpty()) {
				int    link = 0;
				size_t next = 0;
				PPID   v = st.Translate(s, &next);
				if(v == PPSYM_LINK) {
					s.ShiftLeft(next).Strip();
					if(s[0] != '.') {
						ok = PPSetError(PPERR_INVATTSUBSTVAR);
						v = 0;
					}
					else {
						link = 1;
						next = 0;
						v = st.Translate(s.ShiftLeft(), &next);
					}
				}
				else if(v == PPSYM_RECKON) {
					s.ShiftLeft(next).Strip();
					if(s[0] != '.') {
						ok = PPSetError(PPERR_INVATTSUBSTVAR);
						v = 0;
					}
					else {
						link = 2;
						next = 0;
						v = st.Translate(s.ShiftLeft(), &next);
					}
				}
				if(v && GetObjByVar(v, 0, 0)) {
					if(*lp < (SIZEOFARRAY(Subst) - 1)) {
						if(link == 1)
							v |= LINKFLAG;
						else if(link == 2)
							v |= RCKNFLAG;
						s.ShiftLeft(next).Strip();
						if(s[0] == '.') {
							next = 0;
							PPID modif = st.Translate(s.ShiftLeft(), &next);
							if(modif == PPSYM_PARENT)
								v |= PARENTFLAG;
						}
						Subst[(*lp)++] = v;
					}
					else
						break;
				}
				else {
					ok = 0;
				}
			}
		}
		Subst[(*lp)++] = 0;
		*_skipzobj = skipzobj;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPAccTurnTempl::SetupSubst(const char * pPrimStr, const char * pForeignStr)
{
	int    ok = 1;
	int    div, lp = 0;
	int    skipzobj = 0;
	Flags &= ~ATTF_PSUBSTRULE;
	THROW(ParseSubstString(pPrimStr, &lp, &skipzobj));
	SETFLAG(Flags, ATTF_PSKIPONZOBJ, skipzobj);
	if(lp > 1)
		Flags |= ATTF_PSUBSTRULE;
	div = lp;
	Flags &= ~ATTF_FSUBSTRULE;
	THROW(ParseSubstString(pForeignStr, &lp, &skipzobj));
	SETFLAG(Flags, ATTF_FSKIPONZOBJ, skipzobj);
	if(lp > (div+1)) {
		Subst[div-1] = -1L;
		Flags |= ATTF_FSUBSTRULE;
	}
	CATCH
		Subst[0] = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPAccTurnTempl::SubstToString(SString & rBuf, int * lp, int skipzobj)
{
	int    ok = 1;
	int    is_first = 1;
	rBuf.Z();
	SString temp_buf;
	if(skipzobj)
		rBuf.CatChar('!').Space();
	size_t p = 0;
	long   v = Subst[(*lp)];
	PPSymbTranslator st;
	THROW(st);
	while(v != -1L && v != 0) {
		{
			if(!is_first)
				rBuf.CatChar(',');
			is_first = 0;
		}
		if(v & LINKFLAG) {
			THROW(st.Retranslate(PPSYM_LINK, temp_buf));
			rBuf.Cat(temp_buf).CatChar('.');
			v &= ~LINKFLAG;
		}
		else if(v & RCKNFLAG) {
			THROW(st.Retranslate(PPSYM_RECKON, temp_buf));
			rBuf.Cat(temp_buf).CatChar('.');
			v &= ~RCKNFLAG;
		}
		THROW(st.Retranslate(v & ~LMASK, temp_buf));
		rBuf.Cat(temp_buf);
		if(v & PARENTFLAG) {
			rBuf.CatChar('.');
			THROW(st.Retranslate(PPSYM_PARENT, temp_buf));
			rBuf.Cat(temp_buf);
		}
		v = Subst[++(*lp)];
	}
	CATCHZOK
	return ok;
}

int SLAPI PPAccTurnTempl::SubstToStrings(SString & rPrimStr, SString & rForeignStr)
{
	int    lp = 0;
	rPrimStr.Z();
	rForeignStr.Z();
	if(!SubstToString(rPrimStr, &lp, BIN(Flags & ATTF_PSKIPONZOBJ)))
		return 0;
	if(Subst[lp++] == -1L) {
		if(!SubstToString(rForeignStr, &lp, BIN(Flags & ATTF_FSKIPONZOBJ)))
			return 0;
	}
	return 1;
}

int SLAPI PPAccTurnTempl::AccTemplToStr(int side, SString & rBuf) const
{
	Acct   acct;
	PPID   cur_id = 0;
	const AcctID & r_acctid = (side == PPDEBIT) ? DbtID : CrdID;
	const long ac_fixed = (side == PPDEBIT) ? (Flags & ATTF_DACCFIX) : (Flags & ATTF_CACCFIX);
	const long ar_fixed = (side == PPDEBIT) ? (Flags & ATTF_DARTFIX) : (Flags & ATTF_CARTFIX);
	BillObj->atobj->ConvertAcctID(r_acctid, &acct, &cur_id, 1 /* useCache */);
	acct.ToStr(ACCF_DEFAULT, rBuf).Space();
	if(ac_fixed)
		rBuf.CatChar('X');
	else if(ar_fixed)
		rBuf.CatChar('Y');
	return 1;
}

int SLAPI PPAccTurnTempl::AccTemplFromStr(int side, const char * pBuf)
{
	int    ok = 1;
	AcctID acctid;
	Acct   acct;
	long   ac_fixed = 0, ar_fixed = 0;
	char   temp_buf[32];
	const  char * p = pBuf;
	if(p == 0) {
		temp_buf[0] = 0;
		p = temp_buf;
	}
	while(*p != 0 && toupper(*p) != 'X' && toupper(*p) != 'Y')
		p++;
	acct.FromStr(0, pBuf);
	BillObj->atobj->P_Tbl->ConvertAcct(&acct, 0, &acctid, 0);
	if(toupper(p[0]) == 'X')
		ac_fixed = 1;
	else if(toupper(p[0]) == 'Y')
		ar_fixed = 1;
	if(p[0] && p[1]) {
		if(toupper(p[1]) == 'X')
			ac_fixed = 1;
		if(toupper(p[1]) == 'Y')
			ar_fixed = 1;
	}
	if(side == PPDEBIT) {
		DbtID = acctid;
		SETFLAG(Flags, ATTF_DACCFIX, ac_fixed);
		SETFLAG(Flags, ATTF_DARTFIX, ar_fixed);
	}
	else if(side == PPCREDIT) {
		CrdID = acctid;
		SETFLAG(Flags, ATTF_CACCFIX, ac_fixed);
		SETFLAG(Flags, ATTF_CARTFIX, ar_fixed);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}
//
// Диалог шаблона бухгалтерской проводки
//
class ATurnTmplDialog : public TDialog {
public:
	enum {
		ctlgroupDbt = 1,
		ctlgroupCrd = 2,
	};
	ATurnTmplDialog(uint rezID, PPObjAccTurn * _ppobj) : TDialog(rezID), ppobj(_ppobj)
	{
		SetupCalCtrl(CTLCAL_ATRNTMPL_PERIOD, this, CTL_ATRNTMPL_PERIOD, 1);
		AcctCtrlGroup * p_acc_grp = 0;
		setCtrlOption(CTL_ATRNTMPL_DTEXT,  ofFramed, 1);
		setCtrlOption(CTL_ATRNTMPL_CTEXT,  ofFramed, 1);
		setCtrlOption(CTL_ATRNTMPL_SFRAME, ofFramed, 1);
		p_acc_grp = new AcctCtrlGroup(CTL_ATRNTMPL_DACC, CTL_ATRNTMPL_DART, CTLSEL_ATRNTMPL_DACCNAME, CTLSEL_ATRNTMPL_DARTNAME);
		addGroup(ctlgroupDbt, p_acc_grp);
		p_acc_grp = new AcctCtrlGroup(CTL_ATRNTMPL_CACC, CTL_ATRNTMPL_CART, CTLSEL_ATRNTMPL_CACCNAME, CTLSEL_ATRNTMPL_CARTNAME);
		addGroup(ctlgroupCrd, p_acc_grp);
		setDTS(0);
	}
	int    setDTS(const PPAccTurnTempl *);
	int    getDTS(PPAccTurnTempl *);
	PPObjAccTurn * ppobj;
	PPAccTurnTempl data;
private:
	DECL_HANDLE_EVENT;
	void   symbToFormula(const char * pSymb);
	void   setFlags()
	{
		setCtrlUInt16(CTL_ATRNTMPL_PRIMARY, BIN(data.Flags & ATTF_PRIMONCREDIT));
		AddClusterAssoc(CTL_ATRNTMPL_DFIX, 0, ATTF_DACCFIX);
		AddClusterAssoc(CTL_ATRNTMPL_DFIX, 1, ATTF_DARTFIX);
		SetClusterData(CTL_ATRNTMPL_DFIX, data.Flags);
		AddClusterAssoc(CTL_ATRNTMPL_CFIX, 0, ATTF_CACCFIX);
		AddClusterAssoc(CTL_ATRNTMPL_CFIX, 1, ATTF_CARTFIX);
		SetClusterData(CTL_ATRNTMPL_CFIX, data.Flags);
	}
	void   getFlags()
	{
		data.Flags = 0;
		const ushort v = getCtrlUInt16(CTL_ATRNTMPL_PRIMARY);
		SETFLAG(data.Flags, ATTF_PRIMONCREDIT, v);
		GetClusterData(CTL_ATRNTMPL_DFIX, &data.Flags);
		GetClusterData(CTL_ATRNTMPL_CFIX, &data.Flags);
	}
	int    getSheetOfAcc(AcctID * pAcctId, PPID * pAcsID)
	{
		return ppobj->P_Tbl->AccObj.InitAccSheetForAcctID(pAcctId, pAcsID);
	}
	void   swapPrim();
	int    prim; // PPDEBIT || PPCREDIT
};

void ATurnTmplDialog::swapPrim()
{
	char   pt[32], ft[32];
	TCluster * clu = static_cast<TCluster *>(getCtrlView(CTL_ATRNTMPL_PRIMARY));
	if(clu) {
		clu->getText(0, pt, sizeof(pt));
		clu->getText(1, ft, sizeof(ft));
		clu->setText(0, ft);
		clu->setText(1, pt);
		clu->Draw_();
	}
}

void ATurnTmplDialog::symbToFormula(const char * pSymb)
{
	TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_ATRNTMPL_AMOUNT));
	if(p_il) {
		SString symb = pSymb;
		SString input;
		p_il->getText(input);
		size_t pos = p_il->getCaret();
		input.Insert(pos, symb.Quot(' ', ' '));
		p_il->setText(input);
		p_il->Draw_();
		p_il->selectAll(0);
		p_il->setCaret(pos + symb.Len());
	}
}

IMPL_HANDLE_EVENT(ATurnTmplDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_ATRNTMPL_PRIMARY)) {
		ushort v = getCtrlUInt16(CTL_ATRNTMPL_PRIMARY);
		if(v == 0) {
			if(prim == PPCREDIT) {
				swapPrim();
				prim = PPDEBIT;
			}
		}
		else {
			if(prim == PPDEBIT) {
				swapPrim();
				prim = PPCREDIT;
			}
		}
		clearEvent(event);
	}
	else if(event.isKeyDown(kbF2) || event.isCmd(cmAturnTmplSelAmtSymb)) {
		if(TVCOMMAND || isCurrCtlID(CTL_ATRNTMPL_AMOUNT)) {
			PPID   id = 0;
			int    kind = 0;
			SString symb;
			if(SelectAmountSymb(&id, selSymbAmount | selSymbFormula, &kind, symb) > 0)
				symbToFormula(symb.Strip());
		}
		clearEvent(event);
	}
}

int ATurnTmplDialog::setDTS(const PPAccTurnTempl * pData)
{
	SString prim_subst, foreign_subst;
	ushort v = 0;
	AcctCtrlGroup::Rec rec;
	if(!RVALUEPTR(data, pData))
		MEMSZERO(data);
	setFlags();
	prim = (data.Flags & ATTF_PRIMONCREDIT) ? PPCREDIT : PPDEBIT;
	setCtrlData(CTL_ATRNTMPL_AMOUNT, data.Expr);
	getSheetOfAcc(&(rec.AcctId = data.DbtID), &rec.AccSheetID);
	rec.AccSelParam = ACY_SEL_BALOBALALIAS;
	setGroupData(ctlgroupDbt, &rec);
	getSheetOfAcc(&(rec.AcctId = data.CrdID), &rec.AccSheetID);
	setGroupData(ctlgroupCrd, &rec);
	data.SubstToStrings(prim_subst, foreign_subst);
	setCtrlString(CTL_ATRNTMPL_PSUBST, prim_subst);
	setCtrlString(CTL_ATRNTMPL_FSUBST, foreign_subst);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 0, ATTF_SKIPNEG);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 1, ATTF_INVERTNEG);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 2, ATTF_BASEPROJECTION);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 3, ATTF_INTROUNDING);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 4, ATTF_PASSIVE);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 5, ATTF_BYADVLINES);
	AddClusterAssoc(CTL_ATRNTMPL_SKIPNEG, 6, ATTF_SKIPEMPTYALIAS);
	SetClusterData(CTL_ATRNTMPL_SKIPNEG, data.Flags);
	SetPeriodInput(this, CTL_ATRNTMPL_PERIOD, &data.Period);
	return 1;
}

int ATurnTmplDialog::getDTS(PPAccTurnTempl * pData)
{
	char   prim_subst[256], foreign_subst[256];
	ushort v = 0;
	AcctCtrlGroup::Rec dbt_acc_rec, crd_acc_rec;
	getFlags();
	THROW(getGroupData(ctlgroupDbt, &dbt_acc_rec));
	data.DbtID = dbt_acc_rec.AcctId;
	THROW(getGroupData(ctlgroupCrd, &crd_acc_rec));
	data.CrdID = crd_acc_rec.AcctId;
	selectCtrl(CTL_ATRNTMPL_AMOUNT);
	getCtrlData(CTL_ATRNTMPL_AMOUNT, data.Expr);
	if(dbt_acc_rec.AccType != ACY_ALIAS && crd_acc_rec.AccType != ACY_ALIAS) {
		if(oneof2(dbt_acc_rec.AccType, ACY_OBAL, ACY_REGISTER)) {
			THROW_PP(!crd_acc_rec.AcctId.ac || crd_acc_rec.AccType != ACY_BAL, PPERR_INVACCTYPEPAIR);
		}
	}
	if(oneof2(dbt_acc_rec.AccType, ACY_OBAL, ACY_REGISTER) && crd_acc_rec.AcctId.ac == 0)
		data.Flags |= ATTF_CACCFIX;
	if(*strip(data.Expr)) {
		data.Flags |= ATTF_EXPRESSION;
		THROW_PP(data.Flags & (ATTF_DACCFIX | ATTF_CACCFIX), PPERR_ATTMUSTBEFIX);
	}
	else
		data.Flags &= ~ATTF_EXPRESSION;
	getCtrlData(CTL_ATRNTMPL_PSUBST, prim_subst);
	getCtrlData(CTL_ATRNTMPL_FSUBST, foreign_subst);
	THROW(data.SetupSubst(prim_subst, foreign_subst));
	GetClusterData(CTL_ATRNTMPL_SKIPNEG, &data.Flags);
	THROW(GetPeriodInput(this, CTL_ATRNTMPL_PERIOD, &data.Period));
	CATCH
		return PPErrorZ();
	ENDCATCH
	*pData = data;
	return 1;
}

int SLAPI EditAccTurnTemplate(PPObjAccTurn * pObj, PPAccTurnTempl * pData) { DIALOG_PROC_BODY_P2(ATurnTmplDialog, DLG_ATRNTMPL, pObj, pData); }
//
// Список сумм и формул
//
class SelAmtSymbDialog : public TDialog {
public:
	SelAmtSymbDialog(long options) : TDialog(DLG_SELAMTSYMB), Options(options), SelID(0), SelKind(0)
	{
		P_List = static_cast<SmartListBox *>(getCtrlView(CTL_SELAMTSYMB_LIST));
		StrAssocListBoxDef * p_def = new StrAssocListBoxDef(0, lbtDisposeData|lbtDblClkNotify);
		if(p_def) {
			P_List->setDef(p_def);
			updateList();
		}
		else
			PPError(PPERR_NOMEM);
	}
	int    getSelectedSymb(PPID * pID, int * pKind, SString & rSymbBuf) const;
private:
	DECL_HANDLE_EVENT;
	int    MakeList(StrAssocArray * pList);
	void   updateList();
	SmartListBox * P_List;
	long   Options;
	PPObjFormula FrmObj;
	PPObjAmountType AtObj;
	PPObjStaffCal ScObj;
	PPID   SelID;
	int    SelKind;
	SString SelSymb;
};

int SelAmtSymbDialog::getSelectedSymb(PPID * pID, int * pKind, SString & rSymbBuf) const
{
	if(SelID) {
		ASSIGN_PTR(pID, SelID);
		ASSIGN_PTR(pKind, SelKind);
		rSymbBuf = SelSymb;
		return 1;
	}
	else {
		ASSIGN_PTR(pID, 0);
		ASSIGN_PTR(pKind, 0);
		rSymbBuf.Z();
		return 0;
	}
}

IMPL_HANDLE_EVENT(SelAmtSymbDialog)
{
	if(event.isCmd(cmOK) && IsInState(sfModal)) {
		if(P_List && P_List->def) {
			SString temp_buf;
			long   i = 0;
			P_List->getCurID(&i);
			P_List->getCurString(temp_buf);
			StringSet ss(SLBColumnDelim);
			ss.setBuf(temp_buf, temp_buf.Len()+1);
			uint pos = 0;
			if(ss.get(&pos, temp_buf) && ss.get(&pos, temp_buf))
				SelSymb = temp_buf;
			SelKind = (i >> 24);
			SelID = (i & ~0xff000000L);
		}
		clearEvent(event);
		endModal(cmOK); // После endModal не следует обращаться к this
	}
	else {
		TDialog::handleEvent(event);
		/*
		if(event.isCmd(cmaInsert)) {
			PPID   id = 0;
			if(FromObj.Edit(&id, 0) == cmOK)
				updateList();
		}
		else*/ if(event.isCmd(cmaEdit) || event.isCmd(cmLBDblClk)) {
			if(P_List && P_List->def) {
				int    upd = 0;
				long   i = 0;
				P_List->getCurID(&i);
				long   id = (i & ~0xff000000L);
				switch(i >> 24) {
					case selSymbAmount:   upd = BIN(AtObj.Edit(&id, 0) == cmOK); break;
					case selSymbFormula:  upd = BIN(FrmObj.Edit(&id, 0) == cmOK); break;
					case selSymbStaffCal: upd = BIN(ScObj.Edit(&id, 0) == cmOK); break;
				}
				if(upd)
					updateList();
			}
		}
		else
			return;
		clearEvent(event);
	}
}

int SelAmtSymbDialog::MakeList(StrAssocArray * pList)
{
	int    ok = 1;
	long   id;
	SString frm_name, frm_expr;
	StringSet ss(SLBColumnDelim);
	if(Options & selSymbAmount) {
		PPAmountType at_rec;
		for(id = 0; AtObj.EnumItems(&id, &at_rec) > 0;) {
			ss.clear();
			ss.add("A");
			ss.add(at_rec.Symb);
			ss.add(at_rec.Name);
			pList->Add((id | (selSymbAmount << 24)), ss.getBuf());
		}
	}
	if(Options & selSymbFormula) {
		ReferenceTbl::Rec frm_rec;
		for(id = 0; FrmObj.EnumItems(&id, &frm_rec) > 0;) {
			FrmObj.Get(id, frm_name, frm_expr);
			ss.clear();
			ss.add("F");
			ss.add(frm_name);
			ss.add(frm_expr);
			pList->Add((id | (selSymbFormula << 24)), ss.getBuf());
		}
	}
	if(Options & selSymbStaffCal) {
		PPStaffCal sc_rec;
		for(id = 0; ScObj.EnumItems(&id, &sc_rec) > 0;) {
			if(sc_rec.LinkObjType == 0 && sc_rec.LinkObjID == 0) {
				ss.clear();
				ss.add("C");
				ss.add(sc_rec.Symb);
				ss.add(sc_rec.Name);
				pList->Add((id | (selSymbStaffCal << 24)), ss.getBuf());
			}
		}
	}
	if(Options & selSymbSalPeriod) {
		SString temp_buf;
		StringSet f(';', PPLoadTextS(PPTXT_SALFRM_PERIOD, temp_buf));
		id = 1;
		for(uint pos = 0; f.get(&pos, temp_buf); id++) {
			temp_buf.Divide(',', frm_name, frm_expr);
			ss.clear();
			ss.add("P");
			ss.add(frm_name);
			ss.add(frm_expr);
			pList->Add((id | (selSymbSalPeriod << 24)), ss.getBuf());
		}
	}
	return ok;
}

void SelAmtSymbDialog::updateList()
{
	if(P_List) {
		StrAssocArray * p_list = new StrAssocArray;
		if(MakeList(p_list)) {
			static_cast<StrAssocListBoxDef *>(P_List->def)->setArray(p_list);
			Draw_();
		}
		else
			delete p_list;
	}
}

int SLAPI SelectAmountSymb(PPID * pID, long options, int * pKind, SString & rSymbBuf)
{
	int    ok = -1;
	SelAmtSymbDialog * dlg = 0;
	rSymbBuf.Z();
	if(CheckDialogPtrErr(&(dlg = new SelAmtSymbDialog(options)))) {
		if(ExecView(dlg) == cmOK)
			ok = dlg->getSelectedSymb(pID, pKind, rSymbBuf) ? 1 : -1;
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}
