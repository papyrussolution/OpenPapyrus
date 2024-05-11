// TECH.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
TGSArray::TGSArray() : SVector(sizeof(Item)), GStrucID(0)
{
}

uint TGSArray::GetItemsCount() const { return SVector::getCount(); }
void TGSArray::SetStrucID(PPID strucID) { GStrucID = strucID; }
PPID TGSArray::GetStrucID() const { return GStrucID; }

int TGSArray::GetGoodsList(PPIDArray * pList) const
{
	int    ok = -1;
	if(pList) {
		pList->Z();
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		PPIDArray generic_list;
		for(uint i = 0; i < getCount(); i++) {
			const Item * p_item = static_cast<const Item *>(at(i));
			if(goods_obj.Fetch(p_item->GoodsID, &goods_rec) > 0) {
				if(goods_rec.Flags & GF_GENERIC) {
					generic_list.Z();
					goods_obj.GetGenericList(p_item->GoodsID, &generic_list);
					THROW_SL(pList->add(&generic_list));
				}
				else {
					THROW_SL(pList->add(p_item->GoodsID));
				}
			}
		}
		if(pList->getCount())
			pList->sortAndUndup();
	}
	CATCHZOK
	if(ok && getCount())
		ok = 1;
	return ok;
}

int TGSArray::SearchGoods(PPID goodsID, int * pSign, SString * pFormula) const
{
	int    ok = -1;
	uint   pos = 0;
	CALLPTRMEMB(pFormula, Z());
	if(lsearch(&goodsID, &pos, CMPF_LONG)) {
		const Item * p_item = static_cast<const Item *>(at(pos));
		ASSIGN_PTR(pSign, p_item->Sign);
		if(pFormula) {
			GetS(p_item->FormulaP, *pFormula);
		}
		ok = 1;
	}
	else {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		PPIDArray generic_list;
		for(uint i = 0; ok < 0 && i < getCount(); i++) {
			const Item * p_item = static_cast<const Item *>(at(i));
			if(goods_obj.Fetch(p_item->GoodsID, &goods_rec) > 0 && goods_rec.Flags & GF_GENERIC) {
				generic_list.Z();
				goods_obj.GetGenericList(p_item->GoodsID, &generic_list);
				if(generic_list.lsearch(goodsID, &pos)) {
					ASSIGN_PTR(pSign, p_item->Sign);
					if(pFormula)
						GetS(p_item->FormulaP, *pFormula);
					ok = 1;						
				}
			}
		}
	}
	return ok;
}

int TGSArray::AddItem(PPID goodsID, int sign, const char * pFormula)
{
	int    ok = 1;
	Item   item;
	item.GoodsID = goodsID;
	item.Sign = sign;
	item.Reserve = 0;
	AddS(pFormula, &item.FormulaP);
	if(!insert(&item)) {
		ok = PPSetErrorSLib();
	}
	return ok;
}

TGSArray & TGSArray::Z()
{
	SVector::clear();
	SStrGroup::ClearS();
	GStrucID = 0;
	return *this;
}
//
// @ModuleDef(PPObjTech)
//
PPTechPacket::PPTechPacket()
{
}

TLP_IMPL(PPObjTech, TechTbl, P_Tbl);

/*static*/int PPObjTech::GenerateCode(int kind, SString & rBuf, int use_ta)
{
	int    ok = 1;
	long   c = 0;
	PPProcessorConfig cfg;
	rBuf.Z();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(PPObjProcessor::ReadConfig(&cfg));
		c = ++cfg.TecCounter;
		THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PRCCFG, &cfg, sizeof(cfg), 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	if(kind == 1) // Tooling
		rBuf.Cat("TLNG").Space();
	rBuf.CatLongZ(c, 6);
	return ok;
}

PPObjTech::PPObjTech(void * extraPtr) : PPObject(PPOBJ_TECH), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

PPObjTech::~PPObjTech()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjTech::SearchAuto(PPID prcID, PPID goodsID, PPID * pTechID)
{
	int    ok = -1;
	PPID   tech_id = 0;
	PPObjGoods goods_obj;
	TechTbl::Key2 k2;
	MEMSZERO(k2);
	k2.PrcID = prcID;
	BExtQuery q(P_Tbl, 2);
	q.selectAll().where(P_Tbl->PrcID == prcID && P_Tbl->Kind == static_cast<long>(TECK_AUTO));
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		if(goodsID == P_Tbl->data.GoodsID || goods_obj.BelongToGroup(goodsID, P_Tbl->data.GoodsID, 0) > 0) {
			tech_id = P_Tbl->data.ID;
			ok = 1;
		}
	}
	ASSIGN_PTR(pTechID, tech_id);
	return ok;
}

int PPObjTech::SearchAutoForGoodsCreation(PPID prcID, PPID * pGoodsGrpID)
{
	int    ok = -1;
	PPID   goods_id = 0;
	if(prcID) {
		PPIDArray prc_list;
		prc_list.addUnique(prcID);
		PPObjProcessor prc_obj;
		prc_obj.GetParentsList(prcID, &prc_list);
		for(uint i = 0; ok < 0 && i < prc_list.getCount(); i++) {
			const  PPID prc_id = prc_list.get(i);
			TechTbl::Key2 k2;
			MEMSZERO(k2);
			k2.PrcID = prc_id;
			BExtQuery q(P_Tbl, 2);
			q.selectAll().where(P_Tbl->PrcID == prc_id && P_Tbl->Kind == static_cast<long>(TECK_AUTO));
			for(q.initIteration(false, &k2, spGe); ok < 0 && q.nextIteration() > 0;) {
				goods_id = P_Tbl->data.GoodsID;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pGoodsGrpID, goods_id);
	return ok;
}

int PPObjTech::CreateAutoTech(PPID prcID, PPID goodsID, PPID * pTechID, int use_ta)
{
	int    ok = -1;
	PPID   tech_id = 0;
	PPID   auto_tech_id = 0;
	SString temp_buf;
	PPObjProcessor prc_obj;
	ProcessorTbl::Rec prc_rec;
	PPIDArray list;
	THROW(GetListByPrcGoods(prcID, goodsID, &list));
	if(list.getCount() == 0) {
		PPID   prc_id = prcID;
		PPID   tech_id = 0;
		do {
			if(SearchAuto(prc_id, goodsID, &auto_tech_id) > 0) {
				PPTechPacket pack;
				PPTechPacket new_pack;
				PPTransaction tra(use_ta);
				THROW(tra);
				THROW(GetPacket(auto_tech_id, &pack) > 0);
				THROW(InitPacket(&new_pack, 0, 0));
				new_pack.Rec.Kind  = TECK_GENERAL;
				new_pack.Rec.PrcID = pack.Rec.PrcID;
				new_pack.Rec.GoodsID = goodsID;
				new_pack.Rec.Sign = pack.Rec.Sign;
				new_pack.Rec.InitQtty = pack.Rec.InitQtty;
				SETFLAGBYSAMPLE(new_pack.Rec.Flags, pack.Rec.Flags, TECF_CALCTIMEBYROWS); // @v11.3.1
				SETFLAGBYSAMPLE(new_pack.Rec.Flags, pack.Rec.Flags, TECF_AUTOMAIN); // @v11.3.1
				PPGetExtStrData(TECEXSTR_CAPACITY, pack.ExtString, temp_buf);
				if(temp_buf.NotEmptyS()) {
					double capacity = 0.0;
					if(PPExprParser::CalcExpression(temp_buf, &capacity, 0, 0) > 0 && capacity >= 0.0) {
						new_pack.Rec.Capacity = capacity;
					}
				}
				// @v11.1.12 STRNSCPY(new_pack.Rec.Memo, pack.Rec.Memo);
				new_pack.SMemo = pack.SMemo; // @v11.1.12
				THROW(PutPacket(&tech_id, &new_pack, 0));
				THROW(tra.Commit());
				ASSIGN_PTR(pTechID, tech_id);
				ok = 1;
			}
			else if(prc_obj.Fetch(prc_id, &prc_rec) > 0) {
				prc_id = prc_rec.ParentID;
			}
			else
				prc_id = 0;
		} while(prc_id && ok < 0);
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjTech::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }
/*virtual*/const char * PPObjTech::GetNamePtr() { return P_Tbl->data.Code; }

SString & PPObjTech::GetItemMemo(PPID id, SString & rBuf) // @v11.1.12
{
	rBuf.Z();
	PPRef->UtrC.GetText(TextRefIdent(Obj, id, PPTRPROP_MEMO), rBuf);
	rBuf.Transf(CTRANSF_UTF8_TO_INNER);
	return rBuf;
}

int PPObjTech::SearchByCode(const char * pCode, TechTbl::Rec * pRec)
{
	TechTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.Code, pCode);
	return SearchByKey(P_Tbl, 1, &k1, pRec);
}

int PPObjTech::SearchAnalog(const TechTbl::Rec & rKey, PPID * pID, TechTbl::Rec * pRec)
{
    int    ok = -1;
    PPID   id = 0;
    TechTbl::Rec rec;
    /*if(rKey.Code[0] && SearchByCode(rKey.Code, &rec) > 0) {
		id = rec.ID;
    }
    else*/
	{
    	TechTbl::Key2 k2;
		MEMSZERO(k2);
		k2.PrcID = rKey.PrcID;
		k2.GoodsID = rKey.GoodsID;
		k2.GStrucID = rKey.GStrucID;
		if(SearchByKey(P_Tbl, 2, &k2, &rec) > 0) {
			id = rec.ID;
		}
    }
    if(id) {
    	ASSIGN_PTR(pID, id);
		ASSIGN_PTR(pRec, rec);
		ok = 1;
    }
    return ok;
}

int PPObjTech::GetNextSibling(PPID parentID, PPID siblingID, TechTbl::Rec * pNextRec)
{
	int    ok = -1;
	TechTbl::Rec s_rec;
	if(siblingID) {
		if(Search(siblingID, &s_rec) > 0 && s_rec.ParentID == parentID) {
			TechTbl::Key5 k5;
			k5.ParentID = s_rec.ParentID;
			k5.OrderN = s_rec.OrderN;
			if(P_Tbl->search(5, &k5, spGt) && P_Tbl->data.ParentID == s_rec.ParentID) {
				P_Tbl->copyBufTo(pNextRec);
				ok = 1;
			}
		}
	}
	else {
		TechTbl::Key5 k5;
		k5.ParentID = parentID;
		k5.OrderN = 0;
		if(P_Tbl->search(5, &k5, spGt) && P_Tbl->data.ParentID == parentID) {
			P_Tbl->copyBufTo(pNextRec);
			ok = 1;
		}
	}
	return ok;
}

int PPObjTech::GetToolingCondition(PPID id, SString & rFormula)
{
	SString ext_str;
	if(PPRef->GetPropVlrString(Obj, id, TECPRP_EXTSTR, ext_str) > 0)
		if(PPGetExtStrData(TECEXSTR_TLNGCOND, ext_str, rFormula) > 0)
			return 1;
	return -1;
}

int PPObjTech::GetGoodsStruc(PPID id, PPGoodsStruc * pGs)
{
	int    ok = -1;
	TechTbl::Rec tec_rec;
	THROW_INVARG(pGs);
	if(Fetch(id, &tec_rec) > 0 && tec_rec.GStrucID) {
		PPObjGoodsStruc gs_obj;
		THROW_PP(gs_obj.Get(tec_rec.GStrucID, pGs) > 0, PPERR_UNDEFGOODSSTRUC);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjTech::GetGoodsStrucList(PPID id, int useSubst, PPGoodsStruc * pGs, TGSArray * pList)
{
	int    ok = 1, r, r2;
	TechTbl::Rec tec_rec;
	PPGoodsStruc gs;
	THROW(r = Search(id, &tec_rec));
	if(r > 0) {
		THROW(pList->AddItem(tec_rec.GoodsID, tec_rec.Sign, 0));
		THROW(r = GetGoodsStruc(id, &gs));
		if(r > 0) {
			PPGoodsStrucItem gs_item;
			double qtty = 0.0;
			pList->SetStrucID(gs.Rec.ID); // @v11.0.5
			for(uint i = 0; (r = gs.EnumItemsExt(&i, &gs_item, tec_rec.GoodsID, 1, &qtty)) > 0;) {
				int    sign;
				if(gs_item.Median < 0)
					sign = tec_rec.Sign;
				else if(gs_item.Median > 0)
					sign = -tec_rec.Sign;
				else // gs_item.Median == 0
					sign = -tec_rec.Sign;
				THROW(pList->AddItem(gs_item.GoodsID, sign, gs_item.Formula__));
				if(useSubst) {
					PPGoodsStruc subst_gs;
					if(LoadGoodsStruc(PPGoodsStruc::Ident(gs_item.GoodsID, GSF_SUBST), &subst_gs) > 0) {
						PPGoodsStrucItem subst_gsi;
						double subst_qtty = 0.0;
						for(uint j = 0; (r2 = subst_gs.EnumItemsExt(&j, &subst_gsi, gs_item.GoodsID, 1, &subst_qtty)) > 0;)
							THROW(pList->AddItem(subst_gsi.GoodsID, sign, subst_gsi.Formula__));
						THROW(r2);
					}
				}
			}
			THROW(r);
		}
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pGs, gs);
	return ok;
}

int PPObjTech::GetListByPrc(PPID prcID, PPIDArray * pList) { return AddItemsToList(0, pList, 0, prcID, 0); }
int PPObjTech::GetGoodsListByPrc(PPID prcID, PPIDArray * pList) { return AddItemsToList(0, 0, pList, prcID, 0); }
int PPObjTech::GetListByPrcGoods(PPID prcID, PPID goodsID, PPIDArray * pList) { return AddItemsToList(0, pList, 0, prcID, goodsID); }
int PPObjTech::GetListByGoods(PPID goodsID, PPIDArray * pList) { return AddItemsToList(0, pList, 0, 0, goodsID); }
int PPObjTech::DeleteObj(PPID id) { return RemoveByID(P_Tbl, id, 0); }
int PPObjTech::GetListByGoodsStruc(PPID goodsStrucID, PPIDArray * pList) { return AddItemsToList(0, pList, 0, (TECEXDF_GSTRUC | goodsStrucID), 0); } // @v11.7.6

int PPObjTech::IsChildOf(PPID techID, PPID parentID)
{
	int    ok = -1;
	TechTbl::Rec rec;
	for(PPID tec_id = techID; ok < 0 && tec_id;) {
		if(Fetch(tec_id, &rec) > 0) {
			if(rec.ParentID == parentID)
				ok = 1;
			else
				tec_id = rec.ParentID;
		}
		else
			ok = 0;
	}
	return ok;
}

int PPObjTech::GetChildList(PPID techID, PPIDArray & rList)
{
	int    ok = -1;
	TechTbl::Key5 k5;
	k5.ParentID = techID;
	k5.OrderN = 0;
	if(P_Tbl->search(5, &k5, spGt) && P_Tbl->data.ParentID == techID) do {
		rList.addUnique(P_Tbl->data.ID);
		ok = 1;
	} while(P_Tbl->search(5, &k5, spNext) && P_Tbl->data.ParentID == techID);
	return ok;
}

int PPObjTech::GetTerminalChildList(PPID techID, PPIDArray & rList)
{
	rList.Z();
	PPIDArray recur_list;
	Helper_GetTerminalChildList(techID, rList, recur_list);
	return rList.getCount() ? 1 : -1;
}

int PPObjTech::Helper_GetTerminalChildList(PPID techID, PPIDArray & rList, LongArray & rRecurList)
{
	int    ok = 1;
	PPIDArray inner_child_list;
	TechTbl::Rec tec_rec;
	if(Fetch(techID, &tec_rec) > 0 && !rRecurList.lsearch(techID)) {
		GetChildList(techID, inner_child_list);
		if(inner_child_list.getCount()) {
			for(uint i = 0; i < inner_child_list.getCount(); i++) {
				Helper_GetTerminalChildList(inner_child_list.get(i), rList, rRecurList); // @recursion
			}
		}
		else
			rList.add(techID);
	}
	return ok;
}

/*static*/int PPObjTech::SetupCombo(TDialog * dlg, uint ctlID, PPID id, long olwFlags, PPID prcID, PPID goodsID)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_combo) {
		PPObjTech tec_obj;
		StrAssocArray * p_list = new StrAssocArray;
		if(p_list) {
			if(tec_obj.AddItemsToList(p_list, 0, 0, prcID, goodsID)) {
				PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_TECH, p_list, olwFlags | OLW_CANINSERT, 0);
				if(p_lw) {
					if(id == 0 && p_list->getCount() == 1)
						id = p_list->Get(0).Id;
					p_combo->setListWindow(p_lw, id);
					ok = 1;
				}
			}
			else
				ZDELETE(p_list);
		}
		else
			PPSetErrorNoMem();
	}
	else
		ok = -1;
	return ok;
}

int PPObjTech::GetPacket(PPID id, PPTechPacket * pPack)
{
	int    ok = -1;
	THROW_INVARG(pPack);
	if(Search(id, &pPack->Rec) > 0) {
		THROW(PPRef->GetPropVlrString(Obj, id, TECPRP_EXTSTR, pPack->ExtString));
		GetItemMemo(id, pPack->SMemo); // @v11.1.12
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjTech::PutPacket(PPID * pID, PPTechPacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   log_action_id = 0;
	TechTbl::Rec rec;
	SString ext_buffer;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack)
			SETFLAG(pPack->Rec.Flags, TECF_EXTSTRING, pPack->ExtString.NotEmpty());
		if(*pID) {
			THROW(Search(*pID, &rec) > 0);
			if(pPack == 0) {
				THROW(RemoveObjV(*pID, 0, 0, 0));
			}
			else {
				{
					TechTbl::Key5 k5;
					k5.ParentID = pPack->Rec.ParentID;
					k5.OrderN = pPack->Rec.OrderN;
					if(P_Tbl->search(5, &k5, spEq) && P_Tbl->data.ID != *pID) {
						k5.ParentID = pPack->Rec.ParentID;
						k5.OrderN = MAXLONG;
						if(P_Tbl->search(5, &k5, spLt) && P_Tbl->data.ParentID == pPack->Rec.ParentID)
							pPack->Rec.OrderN = P_Tbl->data.OrderN+1;
						else
							pPack->Rec.OrderN = 1;
					}
				}
				THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0)); // @v11.3.10 @fix pPack-->&pPack->Rec
				THROW(p_ref->PutPropVlrString(Obj, *pID, TECPRP_EXTSTR, pPack->ExtString));
				log_action_id = PPACN_OBJUPD;
			}
			Dirty(*pID);
		}
		else if(pPack) {
			{
				TechTbl::Key5 k5;
				k5.ParentID = pPack->Rec.ParentID;
				k5.OrderN = MAXLONG;
				if(P_Tbl->search(5, &k5, spLt) && P_Tbl->data.ParentID == pPack->Rec.ParentID)
					pPack->Rec.OrderN = P_Tbl->data.OrderN+1;
				else
					pPack->Rec.OrderN = 1;
			}
			// @v11.6.4 {
			if(pPack->Rec.Kind == TECK_FOLDER) {
				//const long __SurrogateGoodsId_Start = -524288L;
				long surrogate_goods_id = 0;
				TechTbl::Key2 k2;
				MEMSZERO(k2);
				k2.GoodsID = MINLONG32;
				if(P_Tbl->search(2, &k2, spGt) && P_Tbl->data.GoodsID <= PPConst::TechSurrogateGoodsIdStart)
					surrogate_goods_id = P_Tbl->data.GoodsID-1;
				else
					surrogate_goods_id = PPConst::TechSurrogateGoodsIdStart;
				pPack->Rec.GoodsID = surrogate_goods_id;
			}
			// } @v11.6.4 
			THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0)); // @v11.3.10 @fix pPack-->&pPack->Rec
			pPack->Rec.ID = *pID;
			THROW(p_ref->PutPropVlrString(Obj, *pID, TECPRP_EXTSTR, pPack->ExtString));
			log_action_id = PPACN_OBJADD;
		}
		// @v11.1.12 {
		{
			if(pPack)
				(ext_buffer = pPack->SMemo).Strip();
			else
				ext_buffer.Z();
			THROW(p_ref->UtrC.SetText(TextRefIdent(Obj, *pID, PPTRPROP_MEMO), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		// } @v11.1.12 
		DS.LogAction(log_action_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// Нормализованное представление производительности - это количество единиц товара,
// обработанного за секунду.
//
struct CalcCapacity {
	CalcCapacity() : Flags(0), Unit(SUOM_SECOND), Val(0.0)
	{
	}
	DECL_INVARIANT_C();
	// Descr: возвращает нормализованное значение производительности
	double Normalyze() const;
	//
	// Descr: конвертирует нормализованное представление, заданное параметром val
	//   в значение CalcCapacity::Val в соответствии с установленными параметрами
	//   CalcCapacity::Unit и CalcCapacity::Flags
	//
	int    SetNorma(double val);
	SString & ToText(SString & rBuf) const
	{
		return rBuf.Z().Cat((long)Unit).Semicol().Cat((long)Flags);
	}
	int    FromText(const char *);

	int    Save() const;
	int    Restore();

	enum {
		fReverse  = 0x0001,
		fAbsolute = 0x0002
	};
	int    Unit;    // SUOM_XXX
	int    Flags;
	double Val;
};

IMPL_INVARIANT_C(CalcCapacity)
{
	PPSetError(PPERR_INVCAPACITYVAL);
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(!((Flags & fReverse) && (Flags & fAbsolute)), pInvP);
	S_ASSERT_P((Flags & ~(fReverse|fAbsolute)) == 0, pInvP);
	S_ASSERT_P(oneof4(Unit, SUOM_SECOND, SUOM_MINUTE, SUOM_HOUR, SUOM_DAY), pInvP);
	S_ASSERT_P(Val >= 0.0, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

int CalcCapacity::FromText(const char * pBuf)
{
	int    ok = 1;
	if(pBuf) {
		StringSet ss(';', pBuf);
		uint pos = 0;
		SString temp_buf;
		if(ss.get(&pos, temp_buf)) {
			Unit = temp_buf.ToLong();
			//
			// Обеспечение обратной совместимости с идентификатороми единиц измерения до v7.5.8
			//
			if(Unit == 0)
				Unit = SUOM_SECOND;
			else if(Unit == 1)
				Unit = SUOM_MINUTE;
			else if(Unit == 2)
				Unit = SUOM_HOUR;
			else if(Unit == 3)
				Unit = SUOM_DAY;
			//
			if(ss.get(&pos, temp_buf))
				Flags = satoi(temp_buf);
		}
		ok = InvariantC(0);
	}
	else
		ok = -1;
	return ok;
}

double CalcCapacity::Normalyze() const
{
	double div;
	if(Unit == SUOM_MINUTE)
		div = 60.0;
	else if(Unit == SUOM_HOUR)
		div = 3600.0;
	else if(Unit == SUOM_DAY)
		div = SlConst::SecsPerDayR;
	else
		div = 1.0;
	double v = 0.0;
	if(Flags & fReverse)
		v = 1.0 / (Val * div);
	else if(Flags & fAbsolute)
		v = Val * div;
	else
		v = Val / div;
	return round(v, 10);
}

int CalcCapacity::SetNorma(double val)
{
	double div;
	if(Unit == SUOM_MINUTE)
		div = 60.0;
	else if(Unit == SUOM_HOUR)
		div = 3600.0;
	else if(Unit == SUOM_DAY)
		div = SlConst::SecsPerDayR;
	else
		div = 1.0;
	if(Flags & fReverse)
		Val = 1.0 / (div * val);
	else if(Flags & fAbsolute)
		Val = val / div;
	else
		Val = val * div;
	//Val = (Flags & fReverse) ? (1.0 / (div * val)) : (val * div);
	return InvariantC(0);
}

static const char * WrParam_CalcCapacity = "CalcCapacity";

int CalcCapacity::Save() const
{
	WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 0);
	SString buf;
	reg_key.PutString(WrParam_CalcCapacity, ToText(buf));
	return 1;
}

int CalcCapacity::Restore()
{
	int    ok = -1;
	WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 1);
	SString temp_buf;
	if(reg_key.GetString(WrParam_CalcCapacity, temp_buf))
		ok = FromText(temp_buf);
	return ok;
}

int EditCapacity(CalcCapacity * pData)
{
	class CalcCapacityDialog : public TDialog {
		DECL_DIALOG_DATA(CalcCapacity);
	public:
		CalcCapacityDialog() : TDialog(DLG_CAPACITY)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_CAPACITY_TIMEUNIT, 0, SUOM_SECOND);
			AddClusterAssoc(CTL_CAPACITY_TIMEUNIT, 1, SUOM_MINUTE);
			AddClusterAssoc(CTL_CAPACITY_TIMEUNIT, 2, SUOM_HOUR);
			AddClusterAssoc(CTL_CAPACITY_TIMEUNIT, 3, SUOM_DAY);
			SetClusterData(CTL_CAPACITY_TIMEUNIT, Data.Unit);
			{
				long   r = 0;
				if(Data.Flags & Data.fReverse)
					r = 1;
				else if(Data.Flags & Data.fAbsolute)
					r = 2;
				AddClusterAssocDef(CTL_CAPACITY_REVERSE, 0, 0);
				AddClusterAssoc(CTL_CAPACITY_REVERSE, 1, 1);
				AddClusterAssoc(CTL_CAPACITY_REVERSE, 2, 2);
				SetClusterData(CTL_CAPACITY_REVERSE, r);
			}
			setCtrlReal(CTL_CAPACITY_VAL, Data.Val);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			if(getPage()) {
				Data.Save();
				ASSIGN_PTR(pData, Data);
				return 1;
			}
			else
				return 0;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_CAPACITY_TIMEUNIT) || event.isClusterClk(CTL_CAPACITY_REVERSE)) {
				double val = Data.Normalyze();
				if(getPage()) {
					Data.SetNorma(val);
					SetClusterData(CTL_CAPACITY_TIMEUNIT, Data.Unit);
					long   r = 0;
					if(Data.Flags & Data.fReverse)
						r = 1;
					else if(Data.Flags & Data.fAbsolute)
						r = 2;
					SetClusterData(CTL_CAPACITY_REVERSE, r);
					setCtrlReal(CTL_CAPACITY_VAL, Data.Val);
				}
				clearEvent(event);
			}
		}
		int    getPage()
		{
			Data.Unit = GetClusterData(CTL_CAPACITY_TIMEUNIT);
			long   r = GetClusterData(CTL_CAPACITY_REVERSE);
			Data.Flags &= ~(Data.fReverse|Data.fAbsolute);
			if(r == 1)
				Data.Flags |= Data.fReverse;
			else if(r == 2)
				Data.Flags |= Data.fAbsolute;
			getCtrlData(CTL_CAPACITY_VAL, &Data.Val);
			return 1;
		}
	};
	DIALOG_PROC_BODY(CalcCapacityDialog, pData);
}

//#define GRP_GOODS     1
//#define GRP_PRC       2
//#define GRP_PREVGOODS 3
//
//
//
int PPObjTech::EditDialog(PPTechPacket * pData)
{
	if(pData) {
		if(pData->Rec.Kind == TECK_FOLDER) {
			class TechFolderDialog : public TDialog {
				DECL_DIALOG_DATA(PPTechPacket);
				enum {
					//ctlgroupGoods     = 1,
					ctlgroupPrc       = 2,
				};
			public:			
				TechFolderDialog() : TDialog(DLG_TECHFOLDER)
				{
					addGroup(ctlgroupPrc, new PrcCtrlGroup(CTLSEL_TECH_PRC));
				}
				DECL_DIALOG_SETDTS()
				{
					int    ok = 1;
					RVALUEPTR(Data, pData);
					setCtrlData(CTL_TECH_CODE, Data.Rec.Code);
					setCtrlData(CTL_TECH_ID,   &Data.Rec.ID);
					disableCtrl(CTL_TECH_ID, 1);
					{
						PrcCtrlGroup::Rec prc_grp_rec(Data.Rec.PrcID);
						setGroupData(ctlgroupPrc, &prc_grp_rec);
					}
					SetupPPObjCombo(this, CTLSEL_TECH_PARENT, PPOBJ_TECH, Data.Rec.ParentID, OLW_CANSELUPLEVEL, 0);
					setCtrlString(CTL_TECH_MEMO, Data.SMemo);
					return ok;
				}
				DECL_DIALOG_GETDTS()
				{
					int    ok = 1;
					uint   sel = 0;
					TechTbl::Rec tec_rec;
					PPObjTech tec_obj;
					PrcCtrlGroup::Rec prc_grp_rec;
					getCtrlData(sel = CTL_TECH_CODE, Data.Rec.Code);
					THROW_PP(*strip(Data.Rec.Code), PPERR_CODENEEDED);
					if(tec_obj.SearchByCode(Data.Rec.Code, &tec_rec) > 0 && tec_rec.ID != Data.Rec.ID) {
						PPObject::SetLastErrObj(PPOBJ_TECH, tec_rec.ID);
						CALLEXCEPT_PP(PPERR_DUPSYMB);
					}
					getGroupData(ctlgroupPrc, &prc_grp_rec);
					Data.Rec.PrcID = prc_grp_rec.PrcID;
					sel = CTLSEL_TECH_PRC;
					// (в группирующей технологии процессор не обязателен) THROW_PP(Data.Rec.PrcID, PPERR_PRCNEEDED);
					getCtrlData(sel = CTLSEL_TECH_PARENT, &Data.Rec.ParentID);
					THROW_PP(!Data.Rec.ID || Data.Rec.ParentID != Data.Rec.ID, PPERR_TECHCANTBESELFPARENTED);
					getCtrlString(CTL_TECH_MEMO, Data.SMemo);
					ASSIGN_PTR(pData, Data);
					CATCHZOKPPERRBYDLG
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isCbSelected(CTLSEL_TECH_PARENT)) {
						UI_LOCAL_LOCK_ENTER
						PPID parent_id = getCtrlLong(CTLSEL_TECH_PARENT);
						if(Data.Rec.ID && parent_id == Data.Rec.ID) {
							setCtrlLong(CTLSEL_TECH_PARENT, 0);
							{
								SString err_msg;
								PPGetMessage(mfError, PPERR_TECHCANTBESELFPARENTED, 0, 1, err_msg);
								SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления //
								PPTooltipMessage(err_msg, 0, H(), 20000, GetColorRef(SClrRed),
									SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
							}
						}
						UI_LOCAL_LOCK_LEAVE
					}
				}
			};
			DIALOG_PROC_BODY(TechFolderDialog, pData);
		}
		else if(oneof2(pData->Rec.Kind, TECK_GENERAL, TECK_AUTO)) {
			class TechDialog : public TDialog {
				DECL_DIALOG_DATA(PPTechPacket);
				enum {
					ctlgroupGoods     = 1,
					ctlgroupPrc       = 2,
				};
			public:
				explicit TechDialog(uint dlgID) : TDialog(/*DLG_TECH*/dlgID)
				{
					addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_TECH_GGRP, CTLSEL_TECH_GOODS));
					addGroup(ctlgroupPrc, new PrcCtrlGroup(CTLSEL_TECH_PRC));
				}
				DECL_DIALOG_SETDTS()
				{
					RVALUEPTR(Data, pData);
					setCtrlData(CTL_TECH_CODE, Data.Rec.Code);
					setCtrlData(CTL_TECH_ID,   &Data.Rec.ID);
					disableCtrl(CTL_TECH_ID, 1);
					if(Data.Rec.Kind == TECK_GENERAL) {
						GoodsCtrlGroup::Rec rec(0, Data.Rec.GoodsID, 0, GoodsCtrlGroup::enableInsertGoods | GoodsCtrlGroup::disableEmptyGoods);
						setGroupData(ctlgroupGoods, &rec);
						setCapacity();
					}
					else if(Data.Rec.Kind == TECK_AUTO) {
						SString form;
						GoodsCtrlGroup::Rec rec(Data.Rec.GoodsID, 0, 0, GoodsCtrlGroup::enableInsertGoods);
						setGroupData(ctlgroupGoods, &rec);
						PPGetExtStrData(TECEXSTR_CAPACITY, Data.ExtString, form);
						setCtrlString(CTL_TECH_CAPACITYFORM, form);
					}
					PrcCtrlGroup::Rec prc_grp_rec(Data.Rec.PrcID);
					setGroupData(ctlgroupPrc, &prc_grp_rec);
					SetupPPObjCombo(this, CTLSEL_TECH_GSTRUC, PPOBJ_GOODSSTRUC, Data.Rec.GStrucID, OLW_CANINSERT, reinterpret_cast<void *>NZOR(Data.Rec.GoodsID, -1));
					SetupPPObjCombo(this, CTLSEL_TECH_PARENT, PPOBJ_TECH, Data.Rec.ParentID, OLW_CANSELUPLEVEL, 0);
					setCtrlLong(CTL_TECH_ORDERN, Data.Rec.OrderN);
					AddClusterAssoc(CTL_TECH_SIGN,  0, -1);
					AddClusterAssoc(CTL_TECH_SIGN,  1,  1);
					AddClusterAssocDef(CTL_TECH_SIGN, 2,  0);
					SetClusterData (CTL_TECH_SIGN, Data.Rec.Sign);
					AddClusterAssoc(CTL_TECH_FLAGS, 0, TECF_RECOMPLMAINGOODS);
					AddClusterAssoc(CTL_TECH_FLAGS, 1, TECF_CALCTIMEBYROWS);
					AddClusterAssoc(CTL_TECH_FLAGS, 2, TECF_AUTOMAIN);
					AddClusterAssoc(CTL_TECH_FLAGS, 3, TECF_RVRSCMAINGOODS); // @v10.0.06
					SetClusterData (CTL_TECH_FLAGS, Data.Rec.Flags);
					setCtrlData(CTL_TECH_ROUNDING, &Data.Rec.Rounding);
					setCtrlData(CTL_TECH_INITQTTY, &Data.Rec.InitQtty);
					// @v11.1.12 setCtrlData(CTL_TECH_MEMO, Data.Rec.Memo);
					setCtrlString(CTL_TECH_MEMO, Data.SMemo); // @v11.1.12
					setCtrlData(CTL_TECH_CIPMAX, &Data.Rec.CipMax);
					SetupCtrls();
					return 1;
				}
				DECL_DIALOG_GETDTS()
				{
					int    ok = 1;
					uint   sel = 0;
					long   temp_long = 0;
					TechTbl::Rec tec_rec;
					PPObjTech tec_obj;
					GoodsCtrlGroup::Rec rec;
					PrcCtrlGroup::Rec prc_grp_rec;

					getCtrlData(sel = CTL_TECH_CODE, Data.Rec.Code);
					THROW_PP(*strip(Data.Rec.Code), PPERR_CODENEEDED);
					if(tec_obj.SearchByCode(Data.Rec.Code, &tec_rec) > 0 && tec_rec.ID != Data.Rec.ID) {
						PPObject::SetLastErrObj(PPOBJ_TECH, tec_rec.ID);
						CALLEXCEPT_PP(PPERR_DUPSYMB);
					}
					getGroupData(ctlgroupPrc, &prc_grp_rec);
					Data.Rec.PrcID = prc_grp_rec.PrcID;
					sel = CTLSEL_TECH_PRC;
					THROW_PP(Data.Rec.PrcID, PPERR_PRCNEEDED);
					getCtrlData(sel = CTLSEL_TECH_PARENT, &Data.Rec.ParentID);
					THROW_PP(!Data.Rec.ID || Data.Rec.ParentID != Data.Rec.ID, PPERR_TECHCANTBESELFPARENTED); // @v11.3.9
					sel = CTLSEL_TECH_GOODS;
					THROW(getGroupData(ctlgroupGoods, &rec));
					if(Data.Rec.Kind == TECK_GENERAL)
						Data.Rec.GoodsID = rec.GoodsID;
					else if(Data.Rec.Kind == TECK_AUTO)
						Data.Rec.GoodsID = rec.GrpID;
					getCtrlData(CTLSEL_TECH_GSTRUC, &Data.Rec.GStrucID);
					if(Data.Rec.Kind == TECK_GENERAL)
						getCapacity();
					else if(Data.Rec.Kind == TECK_AUTO) {
						SString form;
						getCtrlString(CTL_TECH_CAPACITYFORM, form);
						PPPutExtStrData(TECEXSTR_CAPACITY, Data.ExtString, form.Strip());
					}
					if(GetClusterData(CTL_TECH_SIGN, &temp_long))
						Data.Rec.Sign = (int16)temp_long;
					GetClusterData(CTL_TECH_FLAGS, &Data.Rec.Flags);
					getCtrlData(CTL_TECH_ROUNDING, &Data.Rec.Rounding);
					getCtrlData(CTL_TECH_INITQTTY, &Data.Rec.InitQtty);
					getCtrlData(CTL_TECH_CIPMAX, &Data.Rec.CipMax);
					// @v11.1.12 getCtrlData(CTL_TECH_MEMO, Data.Rec.Memo);
					getCtrlString(CTL_TECH_MEMO, Data.SMemo); // @v11.1.12
					ASSIGN_PTR(pData, Data);
					CATCHZOKPPERRBYDLG
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isCmd(cmGoodsStruc)) {
						getCtrlData(CTLSEL_TECH_GOODS, &Data.Rec.GoodsID);
						if(Data.Rec.GoodsID) {
							PPObjGoods goods_obj;
							if(goods_obj.EditGoodsStruc(Data.Rec.GoodsID) > 0)
								SetupPPObjCombo(this, CTLSEL_TECH_GSTRUC, PPOBJ_GOODSSTRUC, Data.Rec.GStrucID, OLW_SETUPSINGLE, 
									reinterpret_cast<void *>(Data.Rec.GoodsID));
						}
					}
					else if(event.isCbSelected(CTLSEL_TECH_GOODS)) {
						PPID   prev_goods_id = Data.Rec.GoodsID;
						Data.Rec.GoodsID = getCtrlLong(CTLSEL_TECH_GOODS);
						if(Data.Rec.GoodsID != prev_goods_id) {
							Data.Rec.GStrucID = 0;
							SetupPPObjCombo(this, CTLSEL_TECH_GSTRUC, PPOBJ_GOODSSTRUC, Data.Rec.GStrucID,
								OLW_SETUPSINGLE, reinterpret_cast<void *>(Data.Rec.GoodsID ? Data.Rec.GoodsID : -1));
						}
					}
					else if(event.isCbSelected(CTLSEL_TECH_PARENT)) {
						UI_LOCAL_LOCK_ENTER
						PPID parent_id = getCtrlLong(CTLSEL_TECH_PARENT);
						if(Data.Rec.ID && parent_id == Data.Rec.ID) {
							setCtrlLong(CTLSEL_TECH_PARENT, 0);
							{
								SString err_msg;
								PPGetMessage(mfError, PPERR_TECHCANTBESELFPARENTED, 0, 1, err_msg);
								SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления //
								PPTooltipMessage(err_msg, 0, H(), 20000, GetColorRef(SClrRed),
									SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
							}
						}
						UI_LOCAL_LOCK_LEAVE
					}
					else if(event.isCbSelected(CTLSEL_TECH_PRC))
						SetupCtrls();
					else if(event.isClusterClk(CTL_TECH_SIGN))
						SetupCtrls();
					else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_TECH_CAPACITY))
						calcCapacity();
					else
						return;
					clearEvent(event);
				}
				void   SetupCtrls()
				{
					long   temp_long = 0;
					SString temp_buf;
					PrcCtrlGroup::Rec prc_grp_rec;
					if(GetClusterData(CTL_TECH_SIGN, &temp_long))
						Data.Rec.Sign = (int16)temp_long;
					getGroupData(ctlgroupPrc, &prc_grp_rec);
					Data.Rec.PrcID = prc_grp_rec.PrcID;
					int    enable_recompl_flag = 0;
					ProcessorTbl::Rec prc_rec;
					PrcObj.GetRecWithInheritance(Data.Rec.PrcID, &prc_rec, 1);
					// @v11.3.10 {
					{
						const char * p_label_sign = (prc_rec.Flags & PRCF_TECHCAPACITYREV) ? "tech_capacity_secperpc" : "tech_capacity";
						setLabelText(CTL_TECH_CAPACITY, PPLoadStringS(p_label_sign, temp_buf));
					}
					// } @v11.3.10 
					disableCtrl(CTL_TECH_CIPMAX, !(prc_rec.Flags & PRCF_ALLOWCIP));
					if(Data.Rec.Sign > 0) {
						if(prc_rec.ID && GetOpType(prc_rec.WrOffOpID) == PPOPT_GOODSMODIF)
							enable_recompl_flag = 1;
					}
					if(!enable_recompl_flag) {
						Data.Rec.Flags &= ~TECF_RECOMPLMAINGOODS;
						SetClusterData(CTL_TECH_FLAGS, Data.Rec.Flags);
					}
					DisableClusterItem(CTL_TECH_FLAGS, 0, !enable_recompl_flag);
				}
				void   calcCapacity()
				{
					CalcCapacity param;
					param.Restore();
					getCapacity();
					if(Data.Rec.Flags & TECF_ABSCAPACITYTIME) {
						param.Flags |= param.fAbsolute;
						param.Flags &= ~param.fReverse;
					}
					param.SetNorma(Data.Rec.Capacity);
					if(EditCapacity(&param) > 0) {
						Data.Rec.Capacity = param.Normalyze();
						SETFLAG(Data.Rec.Flags, TECF_ABSCAPACITYTIME, (param.Flags & param.fAbsolute));
						setCapacity();
					}
				}
				void   setCapacity()
				{
					SString temp_buf;
					if(Data.Rec.Flags & TECF_ABSCAPACITYTIME) {
						temp_buf.CatChar('t').Cat(Data.Rec.Capacity, MKSFMTD(0, 10, NMBF_NOTRAILZ|NMBF_NOZERO));
					}
					else {
						ProcessorTbl::Rec prc_rec;
						PrcObj.GetRecWithInheritance(Data.Rec.PrcID, &prc_rec, 1);
						if(Data.Rec.Capacity > 0.0) {
							if(prc_rec.Flags & PRCF_TECHCAPACITYREV)
								temp_buf.Cat(1.0 / Data.Rec.Capacity, MKSFMTD(0, 0, 0));
							else
								temp_buf.Cat(Data.Rec.Capacity, MKSFMTD(0, 10, NMBF_NOTRAILZ|NMBF_NOZERO));
						}
					}
					setCtrlString(CTL_TECH_CAPACITY, temp_buf);
				}
				void   getCapacity()
				{
					int    absolute = 0;
					SString temp_buf;
					getCtrlString(CTL_TECH_CAPACITY, temp_buf);
					temp_buf.Strip();
					if(oneof2(temp_buf.C(0), 't', 'T')) {
						absolute = 1;
						temp_buf.ShiftLeft();
						Data.Rec.Capacity = temp_buf.ToReal();
					}
					else {
						double c = temp_buf.ToReal();
						if(c > 0.0) {
							ProcessorTbl::Rec prc_rec;
							PrcObj.GetRecWithInheritance(Data.Rec.PrcID, &prc_rec, 1);
							if(Data.Rec.Capacity > 0.0) {
								if(prc_rec.Flags & PRCF_TECHCAPACITYREV)
									Data.Rec.Capacity = 1.0 / c;
								else
									Data.Rec.Capacity = c;
							}
						}
						else
							Data.Rec.Capacity = 0.0;
					}
				}
				PPObjProcessor PrcObj;
			};
			DIALOG_PROC_BODY_P1(TechDialog, ((pData->Rec.Kind == TECK_GENERAL) ? DLG_TECH : DLG_TECHAUTO), pData);
		}
		else if(pData->Rec.Kind == TECK_TOOLING) {
			class ToolingDialog : public TDialog {
				DECL_DIALOG_DATA(PPTechPacket);
				enum {
					ctlgroupGoods     = 1,
					ctlgroupPrc       = 2,
					ctlgroupPrevGoods = 3,
				};
			public:
				ToolingDialog() : TDialog(DLG_TOOLING), WasNewStrucCreated(0)
				{
					addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_TECH_GGRP, CTLSEL_TECH_GOODS));
					addGroup(ctlgroupPrevGoods, new GoodsCtrlGroup(CTLSEL_TECH_PREVGGRP, CTLSEL_TECH_PREVGOODS));
					addGroup(ctlgroupPrc, new PrcCtrlGroup(CTLSEL_TECH_PRC));
				}
				DECL_DIALOG_SETDTS()
				{
					RVALUEPTR(Data, pData);
					setCtrlData(CTL_TECH_CODE, Data.Rec.Code);
					setCtrlData(CTL_TECH_ID,   &Data.Rec.ID);
					disableCtrl(CTL_TECH_ID, 1);
					PrcCtrlGroup::Rec prc_grp_rec(Data.Rec.PrcID);
					setGroupData(ctlgroupPrc, &prc_grp_rec);
					SetupPPObjCombo(this, CTLSEL_TECH_PARENT, PPOBJ_TECH, Data.Rec.ParentID, OLW_CANSELUPLEVEL, 0);
					setCtrlLong(CTL_TECH_ORDERN, Data.Rec.OrderN);
					{
						GoodsCtrlGroup::Rec rec(0, Data.Rec.GoodsID, 0, GoodsCtrlGroup::enableInsertGoods);
						setGroupData(ctlgroupGoods, &rec);
					}
					{
						GoodsCtrlGroup::Rec rec(0, Data.Rec.PrevGoodsID, 0, GoodsCtrlGroup::enableInsertGoods);
						setGroupData(ctlgroupPrevGoods, &rec);
					}
					{
						LTIME tm;
						tm.settotalsec(Data.Rec.Duration);
						setCtrlData(CTL_TECH_TIME, &tm);
					}
					SString form_cond;
					PPGetExtStrData(TECEXSTR_TLNGCOND, Data.ExtString, form_cond);
					setCtrlString(CTL_TECH_TRANSCOND, form_cond);
					// @v11.1.12 setCtrlData(CTL_TECH_MEMO, Data.Rec.Memo);
					setCtrlString(CTL_TECH_MEMO, Data.SMemo); // @v11.1.12
					return 1;
				}
				DECL_DIALOG_GETDTS()
				{
					int    ok = 1;
					uint   sel = 0;
					TechTbl::Rec tec_rec;
					PPObjTech tec_obj;
					PrcCtrlGroup::Rec prc_grp_rec;
					getCtrlData(sel = CTL_TECH_CODE, Data.Rec.Code);
					THROW_PP(*strip(Data.Rec.Code), PPERR_CODENEEDED);
					if(tec_obj.SearchByCode(Data.Rec.Code, &tec_rec) > 0 && tec_rec.ID != Data.Rec.ID) {
						PPObject::SetLastErrObj(PPOBJ_TECH, tec_rec.ID);
						CALLEXCEPT_PP(PPERR_DUPSYMB);
					}
					getGroupData(ctlgroupPrc, &prc_grp_rec);
					Data.Rec.PrcID = prc_grp_rec.PrcID;
					sel = CTLSEL_TECH_PRC;
					THROW_PP(Data.Rec.PrcID, PPERR_PRCNEEDED);
					getCtrlData(sel = CTLSEL_TECH_PARENT, &Data.Rec.ParentID);
					THROW_PP(!Data.Rec.ID || Data.Rec.ParentID != Data.Rec.ID, PPERR_TECHCANTBESELFPARENTED); // @v11.3.9
					sel = CTLSEL_TECH_GOODS;
					{
						GoodsCtrlGroup::Rec rec;
						THROW(getGroupData(ctlgroupGoods, &rec));
						Data.Rec.GoodsID = NZOR(rec.GoodsID, rec.GrpID);
					}
					{
						GoodsCtrlGroup::Rec rec;
						THROW(getGroupData(ctlgroupPrevGoods, &rec));
						Data.Rec.PrevGoodsID = NZOR(rec.GoodsID, rec.GrpID);
					}
					{
						LTIME tm;
						getCtrlData(CTL_TECH_TIME, &tm);
						Data.Rec.Duration = tm.totalsec();
					}
					{
						SString form_cond;
						getCtrlString(CTL_TECH_TRANSCOND, form_cond);
						PPPutExtStrData(TECEXSTR_TLNGCOND, Data.ExtString, form_cond.Strip());
					}
					// @v11.1.12 getCtrlData(CTL_TECH_MEMO, Data.Rec.Memo);
					getCtrlString(CTL_TECH_MEMO, Data.SMemo); // @v11.1.12
					ASSIGN_PTR(pData, Data);
					CATCHZOKPPERRBYDLG
					return ok;
				}
			private:
				DECL_HANDLE_EVENT
				{
					TDialog::handleEvent(event);
					if(event.isCbSelected(CTLSEL_TECH_PARENT)) {
						UI_LOCAL_LOCK_ENTER
						PPID parent_id = getCtrlLong(CTLSEL_TECH_PARENT);
						if(Data.Rec.ID && parent_id == Data.Rec.ID) {
							setCtrlLong(CTLSEL_TECH_PARENT, 0);
							{
								SString err_msg;
								PPGetMessage(mfError, PPERR_TECHCANTBESELFPARENTED, 0, 1, err_msg);
								SMessageWindow::DestroyByParent(H()); // Убираем с экрана предыдущие уведомления //
								PPTooltipMessage(err_msg, 0, H(), 20000, GetColorRef(SClrRed),
									SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus|SMessageWindow::fLargeText);
							}
						}
						UI_LOCAL_LOCK_LEAVE
					}
					else if(event.isCmd(cmGoodsStruc)) {
						PPObjGoodsStruc gs_obj;
						PPID   org_struc_id = Data.Rec.GStrucID;
						gs_obj.Edit(&Data.Rec.GStrucID, 0);
						if(org_struc_id == 0 && Data.Rec.GStrucID)
							WasNewStrucCreated = 1;
					}
					else if(event.isCmd(cmToolingTransGcMask)) {
						GdsClsParamMask gcpm;
						gcpm.ClsID = Data.Rec.TransClsID;
						gcpm.Mask  = Data.Rec.TransMask;
						if(gcpm.Edit(0) > 0) {
							Data.Rec.TransClsID = gcpm.ClsID;
							Data.Rec.TransMask  = gcpm.Mask;
						}
					}
					else
						return;
					clearEvent(event);
				}
				PPObjProcessor PrcObj;
				int    WasNewStrucCreated;
			};
			DIALOG_PROC_BODY(ToolingDialog, pData);
		}
	}
	return PPSetErrorInvParam();
}

int PPObjTech::InitPacket(PPTechPacket * pPack, long extraData, int use_ta)
{
	int    ok = 1;
	SString temp_buf;
	TechTbl::Rec rec;
	THROW(GenerateCode(BIN(extraData & TECEXDF_TOOLING), temp_buf, use_ta));
	temp_buf.CopyTo(rec.Code, sizeof(rec.Code));
	if(extraData & TECEXDF_TOOLING) {
		rec.Kind = TECK_TOOLING;
		rec.Sign = 0;
	}
	else if(extraData & TECEXDF_AUTO) {
		rec.Kind = TECK_AUTO;
		rec.Sign = 0;
	}
	else if(extraData & TECEXDF_FOLDER) { // @v11.6.3
		rec.Kind = TECK_FOLDER;
		rec.Sign = 0;
	}
	else {
		rec.Kind = TECK_GENERAL;
		rec.Sign = 1; // По умолчанию - выход
	}
	if(extraData & TECEXDF_GOODS)
		rec.GoodsID = (extraData & TECEXDF_MASK);
	else if(extraData & TECEXDF_PARENT)
		rec.ParentID = (extraData & TECEXDF_MASK);
	else
		rec.PrcID = (extraData & TECEXDF_MASK);
	if(pPack) {
		pPack->Rec = rec;
		pPack->ExtString.Z();
	}
	CATCHZOK
	return ok;
}

int PPObjTech::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    valid_data = 0;
	PPTechPacket pack;
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		THROW(InitPacket(&pack, reinterpret_cast<long>(extraPtr), 1));
	}
	while(!valid_data && EditDialog(&pack) > 0)
		if(PutPacket(pID, &pack, 1)) {
			ok = valid_data = cmOK;
		}
		else
			PPError();
	CATCHZOKPPERR
	return ok;
}

int PPObjTech::ChangeOrderN(PPID techID, int sow, int use_ta)
{
	int    ok = -1;
	THROW_INVARG(oneof2(sow, SOW_NORD, SOW_SOUTH));
	{
		int    do_swap = 0;
		PPID   swap_id = 0;
		long   new_ord_n = 0;
		long   test_ord_n = 1000000;
		TechTbl::Rec main_rec;
		TechTbl::Key5 k5, k5_test;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchByID(P_Tbl, PPOBJ_TECH, techID, &main_rec) > 0);
		if(sow == SOW_NORD) {
			k5.ParentID = main_rec.ParentID;
			k5.OrderN = main_rec.OrderN;
			if(P_Tbl->search(5, &k5, spLt) && P_Tbl->data.ParentID == main_rec.ParentID) {
				swap_id = P_Tbl->data.ID;
				new_ord_n = P_Tbl->data.OrderN;
				do_swap = 1;
			}
		}
		else if(sow == SOW_SOUTH) {
			k5.ParentID = main_rec.ParentID;
			k5.OrderN = main_rec.OrderN;
			if(P_Tbl->search(5, &k5, spGt) && P_Tbl->data.ParentID == main_rec.ParentID) {
				swap_id = P_Tbl->data.ID;
				new_ord_n = P_Tbl->data.OrderN;
				do_swap = 1;
			}
		}
		if(do_swap) {
			do {
				k5_test.ParentID = main_rec.ParentID;
				k5_test.OrderN = ++test_ord_n;
			} while(P_Tbl->search(5, &k5_test, spEq));
			THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == swap_id), set(P_Tbl->OrderN, dbconst(test_ord_n))));
			THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == techID), set(P_Tbl->OrderN, dbconst(new_ord_n))));
			THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == swap_id), set(P_Tbl->OrderN, dbconst(main_rec.OrderN))));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjTech::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = -1;
	if(sampleID > 0) {
		PPTechPacket sample_pack, pack;
		SString temp_buf;
		THROW(CheckRights(PPR_INS));
		THROW(GetPacket(sampleID, &sample_pack) > 0);
		pack = sample_pack;
		pack.Rec.ID = 0;
		THROW(GenerateCode(pack.Rec.Kind, temp_buf, 1));
		temp_buf.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
		while(ok <= 0 && (ok = EditDialog(&pack)) > 0)
			if(PutPacket(pID, &pack, 1))
				ok = 1;
			else
				ok = PPErrorZ();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjTech::Helper_AddItemToList(StrAssocArray * pList, PPID techID, PPID parentID, const char * pCode, LongArray & rRecurList)
{
	int    ok = 1;
	if(pList && !pList->Search(techID) && techID != parentID) { // @v11.3.9 (techID != parentID)
		TechTbl::Rec parent_rec;
		if(parentID && !rRecurList.lsearch(parentID)) {
			if(Fetch(parentID, &parent_rec) > 0) {
				assert(parent_rec.ID == parentID); // @paranoic
				THROW(Helper_AddItemToList(pList, parent_rec.ID, parent_rec.ParentID, parent_rec.Code, rRecurList)); // @recursion
			}
			else
				parentID = 0;
		}
		if(!pList->Search(techID)) {
			//
			// Во избежание бесконечной рекурсии еще раз проверяем, чтобы добавляемой записи не было в списке
			// (после предыдущей проверки в список мог быть добавлен элемент techID)
			//
			THROW_SL(pList->Add(techID, parentID, pCode));
			rRecurList.add(techID); // @v11.3.9
		}
	}
	CATCHZOK
	return ok;
}

int PPObjTech::AddItemsToList(StrAssocArray * pList, PPIDArray * pIdList, PPIDArray * pGoodsIdList, long extraParam, PPID goodsID)
{
	int    ok = 1;
	int    idx = 0;
	PPID   prc_id = 0;
	PPObjGoods goods_obj;
	PPIDArray id_list;
	LongArray recur_list;
	DBQ  * dbq = 0;
	union {
		TechTbl::Key2 k2;
		TechTbl::Key3 k3;
		TechTbl::Key4 k4;
	} k;
	MEMSZERO(k);
	if(extraParam & TECEXDF_GOODS) {
		idx = 3;
		k.k3.GoodsID = (extraParam & TECEXDF_MASK);
		dbq = ppcheckfiltid(dbq, P_Tbl->GoodsID, k.k3.GoodsID);
	}
	else if(extraParam & TECEXDF_GSTRUC) {
		idx = 4;
		k.k4.GStrucID = (extraParam & TECEXDF_MASK);
		k.k4.GoodsID = MINLONG32; // @v11.6.4
		dbq = ppcheckfiltid(dbq, P_Tbl->GStrucID, k.k4.GStrucID);
	}
	else {
		idx = 2;
		prc_id = (extraParam & TECEXDF_MASK);
		k.k2.PrcID = prc_id;
		k.k2.GoodsID = MINLONG32; // @v11.6.4
		dbq = ppcheckfiltid(dbq, P_Tbl->PrcID, k.k2.PrcID);
	}
	if(extraParam & TECEXDF_TOOLING)
		dbq = &(*dbq && P_Tbl->Kind == 1L);
	else {
		dbq = &(*dbq && (P_Tbl->Kind == static_cast<long>(TECK_GENERAL) || P_Tbl->Kind == static_cast<long>(TECK_FOLDER)));
	}
	RVALUEPTR(id_list, pIdList);
	BExtQuery q(P_Tbl, idx);
	q.select(P_Tbl->ID, P_Tbl->ParentID, P_Tbl->OrderN, P_Tbl->GoodsID, P_Tbl->Code, 0).where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		TechTbl::Rec tec_rec;
		P_Tbl->copyBufTo(&tec_rec);
		if(!(prc_id && goodsID) || tec_rec.GoodsID == labs(goodsID)) {
			if(id_list.lsearch(tec_rec.ID)) {
				// Зацикливание рекурсии. Следует оборвать рекурсию.
				prc_id = 0;
				break;
			}
			else
				THROW(id_list.add(tec_rec.ID));
			recur_list.Z();
			THROW(Helper_AddItemToList(pList, tec_rec.ID, tec_rec.ParentID, tec_rec.Code, recur_list));
			if(pGoodsIdList && tec_rec.GoodsID) {
				const  PPID goods_id = tec_rec.GoodsID;
				Goods2Tbl::Rec goods_rec;
				if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
					if(goods_rec.Flags & GF_GENERIC) {
						/*
						PPIDArray gen_list;
						goods_obj.P_Tbl->GetDynGenericList(goods_id, &gen_list);
						THROW(pGoodsIdList->addUnique(&gen_list));
						*/
					}
					else {
						THROW(pGoodsIdList->addUnique(goods_id));
					}
				}
			}
		}
	}
	if(prc_id) {
		PPObjProcessor prc_obj;
		ProcessorTbl::Rec prc_rec;
		if(prc_obj.Fetch(prc_id, &prc_rec) > 0 && prc_rec.ParentID)
			THROW(AddItemsToList(pList, &id_list, pGoodsIdList, prc_rec.ParentID, goodsID)); // @recursion
	}
	CATCHZOK
	ASSIGN_PTR(pIdList, id_list);
	return ok;
}

/* @construction static IMPL_CMPFUNC(Tec_OrdN_Name, i1, i2)
{
	assert(pExtraData);
	if(pExtraData) {
		PPObjTech * p_obj = static_cast<PPObjTech *>(pExtraData);
	}
	else {

	}
}*/

StrAssocArray * PPObjTech::MakeStrAssocList(void * extraPtr)
{
	StrAssocArray * p_list = new StrAssocArray;
	if(p_list) {
		if(!AddItemsToList(p_list, 0, 0, reinterpret_cast<long>(extraPtr), 0))
			ZDELETE(p_list);
		else {
			// @v11.2.6 {
			// Если все технологии относятся к единственному процессору, то сортируем по {OrderN; Name},
			// иначе сортируем по Name
			long   single_prc_id = -1;
			TechTbl::Rec tec_rec;
			for(uint i = 0; single_prc_id != 0 && i < p_list->getCount(); i++) {
				StrAssocArray::Item item = p_list->at_WithoutParent(i);
				if(Fetch(item.Id, &tec_rec) > 0) {
					if(single_prc_id < 0)
						single_prc_id = tec_rec.PrcID;
					else if(tec_rec.PrcID != single_prc_id)
						single_prc_id = 0;
				}
			}
			if(single_prc_id > 0) {
				//p_list->sort
				// @stub Пока сортируем просто по тексту. Оказалось, StrAssocArray не предоставляет сервиса 
				// сортировки по произвольной внешней функции
				p_list->SortByText();
			}
			else {
				p_list->SortByText();
			}
			// } @v11.2.6 
		}
	}
	else
		PPSetErrorNoMem();
	return p_list;
}

int PPObjTech::Browse(void * extraPtr)
{
	if(extraPtr) {
		const long extra_param = reinterpret_cast<long>(extraPtr);
		TechFilt filt;
		if(extra_param & TECEXDF_GOODS)
			filt.GoodsID = (extra_param & TECEXDF_MASK);
		else if(extra_param & TECEXDF_GSTRUC)
			filt.GStrucID = (extra_param & TECEXDF_MASK);
		else
			filt.PrcID = (extra_param & TECEXDF_MASK);
		if(extra_param & TECEXDF_TOOLING)
			filt.Kind = TECK_TOOLING;
		else if(extra_param & TECEXDF_AUTO)
			filt.Kind = TECK_AUTO;
		ViewTech(&filt);
	}
	else
		ViewTech(0);
	return 1;
}
//
//
//
IMPL_DESTROY_OBJ_PACK(PPObjTech, PPTechPacket);

int PPObjTech::SerializePacket(int dir, PPTechPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SerializeSignature srzs(Obj, dir, rBuf); // @v11.1.12 
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->ExtString, rBuf));
	// @v11.1.12 {
	if(srzs.V.IsGe(11, 1, 12)) {
		THROW_SL(pSCtx->Serialize(dir, pPack->SMemo, rBuf)); 
	}
	// } @v11.1.12
	CATCHZOK
	return ok;
}

int  PPObjTech::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjTech, PPTechPacket>(this, p, id, stream, pCtx); }

int  PPObjTech::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPTechPacket * p_pack = p ? static_cast<PPTechPacket *>(p->Data) : 0;
	if(p_pack) {
		if(stream == 0) {
			TechTbl::Rec same_rec;
			PPID   same_id = 0;
			if(*pID == 0 && SearchAnalog(p_pack->Rec, &same_id, &same_rec) > 0) {
				TechTbl::Rec rec_by_code;
				if(SearchByCode(p_pack->Rec.Code, &rec_by_code) > 0 && rec_by_code.ID != same_id) {
					//
					// Предупреждаем конфликт по коду для записи, сопоставленной по {PrcID, GoodsID, GStrucID}
					//
                    STRNSCPY(p_pack->Rec.Code, same_rec.Code);
				}
				*pID = same_id;
			}
			else if(*pID && Search(*pID, &same_rec) > 0) {
				same_id = *pID;
			}
			const int is_new = BIN(*pID == 0);
			int    r = PutPacket(pID, p_pack, 1);
			if(!r) {
				pCtx->OutputAcceptObjErrMsg(Obj, p_pack->Rec.ID, p_pack->Rec.Code);
				ok = -1;
			}
			else if(r > 0)
				ok = is_new ? 101 : 102; // @ObjectCreated : @ObjectUpdated
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

int  PPObjTech::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPTechPacket * p_pack = static_cast<PPTechPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROCESSOR, &p_pack->Rec.PrcID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODS, &p_pack->Rec.GoodsID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &p_pack->Rec.GStrucID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODS, &p_pack->Rec.PrevGoodsID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSCLASS, &p_pack->Rec.TransClsID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_TECH, &p_pack->Rec.ParentID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
class TechCache : public ObjCache {
public:
	TechCache() : ObjCache(PPOBJ_TECH, sizeof(Data))
	{
	}
private:
	struct Data : public ObjCacheEntry {
		long   ParentID;    //
		long   OrderN;      //
		long   PrcID;       // ->Processor.ID
		long   GoodsID;     // ->Goods.ID
		long   GStrucID;    // ->Ref(PPOBJ_GOODSSTRUC)
		long   Flags;       // TECF_XXX
		int16  Sign;        // @#{-1,0,+1} -1 - расход, +1 - приход, 0 - остаток не меняется (использование) //
		int16  Kind;        //
		long   Duration;    //
		float  InitQtty;    //
		double Cost;        // Суммарная стоимость операции на одну торговую единицу GoodsID
		double Capacity;    // Производительность процессора ProcID при использовании этой технологии
		double Rounding;
	};
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long extraData);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int TechCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjTech tec_obj;
	TechTbl::Rec rec;
	if(tec_obj.Search(id, &rec) > 0) {
		#define FLD(f) p_cache_rec->f = rec.f
		FLD(ParentID);
		FLD(OrderN);
		FLD(PrcID);
		FLD(GoodsID);
		FLD(GStrucID);
		FLD(Sign);
		FLD(Kind);
		FLD(Flags);
		FLD(InitQtty);
		FLD(Duration);
		FLD(Cost);
		FLD(Capacity);
		FLD(Rounding);
		#undef FLD
		ok = PutName(rec.Code, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void TechCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	TechTbl::Rec * p_data_rec = static_cast<TechTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(ParentID);
	FLD(OrderN);
	FLD(PrcID);
	FLD(GoodsID);
	FLD(GStrucID);
	FLD(Sign);
	FLD(Kind);
	FLD(Flags);
	FLD(InitQtty);
	FLD(Duration);
	FLD(Cost);
	FLD(Capacity);
	FLD(Rounding);
	#undef FLD
	GetName(pEntry, p_data_rec->Code, sizeof(p_data_rec->Code));
}

int PPObjTech::Fetch(PPID id, TechTbl::Rec * pRec)
{
	TechCache * p_cache = GetDbLocalCachePtr <TechCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int PPObjTech::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_PROCESSOR) {
			TechTbl::Key2 k2;
			MEMSZERO(k2);
			k2.PrcID = _id;
			if(P_Tbl->search(2, &k2, spGe) && k2.PrcID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
		else if(_obj == PPOBJ_GOODS) {
			TechTbl::Key3 k3;
			MEMSZERO(k3);
			k3.GoodsID = _id;
			if(P_Tbl->search(3, &k3, spGe) && k3.GoodsID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
	}
	return ok;
}
//
// @ModuleDef(PPViewTech)
//
IMPLEMENT_PPFILT_FACTORY(Tech); TechFilt::TechFilt() : PPBaseFilt(PPFILT_TECH, 0, 0)
{
	SetFlatChunk(offsetof(TechFilt, ReserveStart),
		offsetof(TechFilt, Reserve)-offsetof(TechFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewTech::PPViewTech() : PPView(&TecObj, &Filt, PPVIEW_TECH, 0, REPORT_TECHVIEW)
{
}

const TechFilt * PPViewTech::GetFilt() const { return &Filt; }

class TechFiltDialog : public TDialog {
	DECL_DIALOG_DATA(TechFilt);
	enum {
		ctlgroupGoods     = 1,
		ctlgroupPrc       = 2,
	};
public:
	TechFiltDialog() : TDialog(DLG_TECHFILT)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_TECHFILT_GGRP, CTLSEL_TECHFILT_GOODS));
		addGroup(ctlgroupPrc,   new PrcCtrlGroup(CTLSEL_TECHFILT_PRC));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PrcCtrlGroup::Rec prc_grp_rec(Data.PrcID);
		setGroupData(ctlgroupPrc, &prc_grp_rec);
		SetupPPObjCombo(this, CTLSEL_TECHFILT_PARENT, PPOBJ_TECH, Data.ParentID, OLW_CANSELUPLEVEL, 0); // @v11.0.2 removed flag OLW_SETUPSINGLE
		GoodsCtrlGroup::Rec rec(0, Data.GoodsID);
		setGroupData(ctlgroupGoods, &rec);
		AddClusterAssocDef(CTL_TECHFILT_KIND, 0, 0);
		AddClusterAssoc(CTL_TECHFILT_KIND, 1, 1);
		AddClusterAssoc(CTL_TECHFILT_KIND, 2, 2);
		SetClusterData(CTL_TECHFILT_KIND, Data.Kind);
		AddClusterAssocDef(CTL_TECHFILT_SIGN,  0,  TechFilt::signAll);
		AddClusterAssoc(CTL_TECHFILT_SIGN,  1,  TechFilt::signMinusOnly);
		AddClusterAssoc(CTL_TECHFILT_SIGN,  2,  TechFilt::signPlusOnly);
		AddClusterAssoc(CTL_TECHFILT_SIGN,  3,  TechFilt::signUsageOnly);
		SetClusterData(CTL_TECHFILT_SIGN, Data.Sign);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		PrcCtrlGroup::Rec prc_grp_rec;
		GoodsCtrlGroup::Rec rec;
		getGroupData(ctlgroupPrc, &prc_grp_rec);
		Data.PrcID = prc_grp_rec.PrcID;
		getCtrlData(CTLSEL_TECHFILT_PARENT, &Data.ParentID);
		sel = CTLSEL_TECHFILT_GOODS;
		THROW(getGroupData(ctlgroupGoods, &rec));
		Data.GoodsID = rec.GoodsID;
		GetClusterData(CTL_TECHFILT_KIND, &Data.Kind);
		GetClusterData(CTL_TECHFILT_SIGN,  &Data.Sign);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
};

int PPViewTech::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	TechFilt * p_filt = static_cast<TechFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(TechFiltDialog, p_filt);
}

int PPViewTech::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt) > 0);
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

int PPViewTech::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();

	int    ok = 1, idx = 0;
	TechTbl * p_t = TecObj.P_Tbl;
	DBQ * dbq = 0;
	union {
		TechTbl::Key2 k2;
		TechTbl::Key3 k3;
		TechTbl::Key4 k4;
	} k, k_;
	MEMSZERO(k);
	if(Filt.GoodsID) {
		idx = 3;
		k.k3.GoodsID = Filt.GoodsID;
	}
	else if(Filt.PrcID) {
		idx = 2;
		k.k2.PrcID = Filt.PrcID;
	}
	else if(Filt.GStrucID) {
		idx = 4;
		k.k4.GStrucID = Filt.GStrucID;
	}
	else {
		idx = 2;
	}
	dbq = ppcheckfiltid(dbq, p_t->GoodsID, Filt.GoodsID);
	dbq = ppcheckfiltid(dbq, p_t->PrcID, Filt.PrcID);
	dbq = ppcheckfiltid(dbq, p_t->GStrucID, Filt.GStrucID);
	if(Filt.Sign == TechFilt::signMinusOnly)
		dbq = &(*dbq && p_t->Sign < 0L);
	else if(Filt.Sign == TechFilt::signPlusOnly)
		dbq = &(*dbq && p_t->Sign > 0L);
	else if(Filt.Sign == TechFilt::signUsageOnly)
		dbq = &(*dbq && p_t->Sign == 0L);
	dbq = &(*dbq && p_t->Kind == Filt.Kind);
	P_IterQuery = new BExtQuery(p_t, idx);
	P_IterQuery->select(p_t->ID, 0L).where(*dbq);
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	return ok;
}

int FASTCALL PPViewTech::NextIteration(TechViewItem * pItem)
{
	while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		if(TecObj.Search(TecObj.P_Tbl->data.ID, pItem) > 0)
			return 1;
	}
	return -1;
}

void * PPViewTech::GetEditExtraParam()
{
	long   v = 0;
	if(Filt.PrcID)
		v = Filt.PrcID;
	else if(Filt.GoodsID)
		v = (Filt.GoodsID | TECEXDF_GOODS);
	else if(Filt.ParentID)
		v = (Filt.ParentID | TECEXDF_PARENT);
	if(Filt.Kind == TECK_TOOLING)
		v |= TECEXDF_TOOLING;
	else if(Filt.Kind == 2)
		v |= TECEXDF_AUTO;
	return reinterpret_cast<void *>(v);
}

DBQuery * PPViewTech::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	const uint   brw_id = (Filt.Kind == TECK_TOOLING) ? BROWSER_TECHTOOLING : BROWSER_TECH;
	TechTbl * p_tect = 0;
	ReferenceTbl * p_reft = 0;
	DBQuery * q  = 0;
	DBQ  * dbq = 0;
	DBE    dbe_prc;
	DBE    dbe_goods;
	DBE    dbe_prev_goods;
	DBE    dbe_parent;
	DBE    dbe_capacity; // @v11.3.10
	THROW(CheckTblPtr(p_tect = new TechTbl));
	THROW(CheckTblPtr(p_reft = new ReferenceTbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_prc, PPDbqFuncPool::IdObjNamePrc, p_tect->PrcID);
	PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, p_tect->GoodsID);
	PPDbqFuncPool::InitObjNameFunc(dbe_prev_goods, PPDbqFuncPool::IdObjNameGoods, p_tect->PrevGoodsID);
	PPDbqFuncPool::InitObjNameFunc(dbe_parent, PPDbqFuncPool::IdObjNameTech, p_tect->ParentID);
	// @v11.3.10 {
	{
		dbe_capacity.init();
		dbe_capacity.push(p_tect->PrcID);
		dbe_capacity.push(p_tect->Capacity);
		dbe_capacity.push(static_cast<DBFunc>(PPDbqFuncPool::IdTechCapacity));
	}
	// } @v11.3.10 
	q = & select(p_tect->ID, 0L).from(p_tect, p_reft, 0L); // #0
	q->addField(p_tect->Code);      // #1
	q->addField(dbe_prc);           // #2
	q->addField(dbe_goods);         // #3
	q->addField(p_reft->ObjName);   // #4
	q->addField(p_tect->Sign);      // #5
	// @v11.3.10 q->addField(p_tect->Capacity);  // #6
	q->addField(dbe_capacity);      // #6 @v11.3.10
	q->addField(p_tect->Duration);  // #7
	q->addField(dbe_prev_goods);    // #8
	q->addField(p_tect->InitQtty);  // #9
	q->addField(dbe_parent);        // #10
	q->addField(p_tect->OrderN);    // #11
	dbq = ppcheckfiltid(dbq, p_tect->GoodsID, Filt.GoodsID);
	dbq = ppcheckfiltid(dbq, p_tect->PrcID, Filt.PrcID);
	dbq = ppcheckfiltid(dbq, p_tect->GStrucID, Filt.GStrucID);
	dbq = ppcheckfiltid(dbq, p_tect->ParentID, Filt.ParentID);
	if(Filt.Sign == TechFilt::signMinusOnly)
		dbq = &(*dbq && p_tect->Sign < 0L);
	else if(Filt.Sign == TechFilt::signPlusOnly)
		dbq = &(*dbq && p_tect->Sign > 0L);
	else if(Filt.Sign == TechFilt::signUsageOnly)
		dbq = &(*dbq && p_tect->Sign == 0L);
	if(Filt.Kind == TECK_GENERAL)
		dbq = &(*dbq && (p_tect->Kind == static_cast<long>(TECK_GENERAL) || p_tect->Kind == static_cast<long>(TECK_FOLDER)));
	else
		dbq = &(*dbq && p_tect->Kind == Filt.Kind);
	dbq = & (*dbq && ((p_reft->ObjID += p_tect->GStrucID) && p_reft->ObjType == PPOBJ_GOODSSTRUC));
	q->where(*dbq);
	if(Filt.GoodsID)
		q->orderBy(p_tect->GoodsID, 0L);
	else if(Filt.PrcID)
		q->orderBy(p_tect->PrcID, 0L);
	else {
		q->orderBy(p_tect->ParentID, p_tect->OrderN, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		if(Filt.PrcID) {
			pSubTitle->CatDivIfNotEmpty(';', 0);
			CatObjectName(PPOBJ_PROCESSOR, Filt.PrcID, *pSubTitle);
		}
		if(Filt.GoodsID) {
			pSubTitle->CatDivIfNotEmpty(';', 0);
			CatObjectName(PPOBJ_GOODS, Filt.GoodsID, *pSubTitle);
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete p_tect;
			delete p_reft;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewTech::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		TechViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_TECH, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewTech::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		TechTbl::Rec tec_rec;
		switch(ppvCmd) {
			case PPVCMD_ADDBYSAMPLE:
				{
					PPID   sample_id = id;
					id = 0;
					ok = -1;
					if(sample_id && TecObj.AddBySample(&id, sample_id) > 0)
						ok = 1;
				}
				break;
			case PPVCMD_ADDGROUP: // @v11.6.3
				{
					PPID   new_id = 0;
					if(TecObj.Edit(&new_id, reinterpret_cast<void *>(TECEXDF_FOLDER)) == cmOK) {
						id = new_id;
						ok = 1;
					}
				}
				break;
			case PPVCMD_VIEWCHILDS:
				ok = -1;
				{
					TechFilt inner_filt;
					inner_filt.ParentID = id;
					PPView::Execute(PPVIEW_TECH, &inner_filt, 1, 0);
				}
				break;
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(TecObj.Search(id, &tec_rec) > 0) {
					PPObjGoods goods_obj;
					if(goods_obj.Edit(&tec_rec.GoodsID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITPRC:
				ok = -1;
				if(TecObj.Search(id, &tec_rec) > 0 && tec_rec.PrcID) {
					PPObjProcessor prc_obj;
					if(prc_obj.Edit(&tec_rec.PrcID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_EDITCAPACITY: // @v11.3.10
				ok = -1;
				{
					PPTechPacket pack;
					if(TecObj.GetPacket(id, &pack) > 0) {
						CalcCapacity cc;
						bool is_rev = false;
						if(pack.Rec.PrcID) {
							ProcessorTbl::Rec prc_rec;
							if(PrcObj.GetRecWithInheritance(pack.Rec.PrcID, &prc_rec, 1) > 0 && prc_rec.Flags & PRCF_TECHCAPACITYREV) 
								is_rev = true;
						}
						cc.SetNorma(pack.Rec.Capacity);
						if(is_rev) {
							cc.Flags |= CalcCapacity::fReverse;
							cc.Unit = SUOM_MINUTE;
						}
						if(EditCapacity(&cc) > 0) {
							double nv = cc.Normalyze();
							if(nv != pack.Rec.Capacity) {
								pack.Rec.Capacity = nv;
								if(TecObj.PutPacket(&id, &pack, 1)) 
									ok = 1;
								else
									PPError();
							}
						}
					}
				}
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit(0);
				break;
			case PPVCMD_MOVEUP:
				ok = -1;
				if(TecObj.ChangeOrderN(id, SOW_NORD, 1) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_MOVEDOWN:
				ok = -1;
				if(TecObj.ChangeOrderN(id, SOW_SOUTH, 1) > 0) {
					ok = 1;
				}
				break;
		}
	}
	return ok;
}

int ViewTech(const TechFilt * pFilt) { return PPView::Execute(PPVIEW_TECH, pFilt, 1, 0); }

#if 0 // {
int ViewTech(const TechFilt * pFilt)
{
	int    ok = 1, view_in_use = 0, r = -1;
	int    modeless = GetModelessStatus();
	PPViewBrowser * p_prev_win = 0;
	TechFilt flt;
	PPViewTech * p_v = new PPViewTech;
	if(modeless)
		p_prev_win = static_cast<PPViewBrowser *>(PPFindLastBrowser());
	if(pFilt)
		flt = *pFilt;
	else if(p_prev_win)
		flt = *((PPViewTech *)(p_prev_win->P_View))->GetFilt();
	else
		flt.Init();
	while(pFilt || (r = p_v->EditFilt(&flt)) > 0) {
		THROW(p_v->Init(&flt));
		PPCloseBrowser(p_prev_win);
		THROW(p_v->Browse(modeless));
		if(modeless || pFilt) {
			view_in_use = 1;
			break;
		}
	}
	THROW(r);
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	return ok;
}
#endif // } 0
//
// Implementation of PPALDD_Tech
//
PPALDD_CONSTRUCTOR(Tech)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjTech;
	}
}

PPALDD_DESTRUCTOR(Tech)
{
	Destroy();
	delete static_cast<PPObjTech *>(Extra[0].Ptr);
}

int PPALDD_Tech::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		TechTbl::Rec rec;
		PPObjTech * p_obj = static_cast<PPObjTech *>(Extra[0].Ptr);
		if(p_obj->Search(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Code, rec.Code);
			H.PrcID    = rec.PrcID;
			H.GoodsID  = rec.GoodsID;
			H.GStrucID = rec.GStrucID;
			{
				SString r_temp_buf = SLS.AcquireRvlStr();
				if(GetObjectName(PPOBJ_GOODSSTRUC, rec.GStrucID, r_temp_buf) > 0) {
					if(r_temp_buf.NotEmptyS()) {
						STRNSCPY(H.GStrucName, r_temp_buf);
					}
					else 
						STRNSCPY(H.GStrucName, "BOM #");
				}
			}
			H.Flags    = rec.Flags;
			H.Sign     = rec.Sign;
			H.Cost     = rec.Cost;
			H.Capacity = rec.Capacity;
			{
				// @v11.1.12 STRNSCPY(H.Memo, rec.Memo);
				// @v11.1.12 {
				SString r_temp_buf = SLS.AcquireRvlStr();
				p_obj->GetItemMemo(rFilt.ID, r_temp_buf);
				STRNSCPY(H.Memo, r_temp_buf); 
				// } @v11.1.12 
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_TechView
//
PPALDD_CONSTRUCTOR(TechView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TechView) { Destroy(); }

int PPALDD_TechView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Tech, rsrv);
	H.FltPrcID   = p_filt->PrcID;
	H.FltGoodsID = p_filt->GoodsID;
	H.FltGsID    = p_filt->GStrucID;
	H.SignFlags  = p_filt->Sign;
	H.Flags      = p_filt->Flags;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TechView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(Tech);
}

int PPALDD_TechView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Tech);
	I.TechID = item.ID;
	// @todo I.TimingTxt[16]
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TechView::Destroy() { DESTROY_PPVIEW_ALDD(Tech); }
//
//
//
//#define _CMP(a,b,x) (((a)==(b)) ? (0 : ((a)<(b)) ? -1 : 1))

class ToolingSelector {
public:
	struct Entry {
		PPID   ID;
		PPID   PrcID;
		PPID   GoodsID;
		PPID   PrevGoodsID;
		PPID   TransClsID;
		long   TransMask;
		int16  PrcKind;       // 1 - prc, 2 - prc group
		int16  IsTranMask;    // 1 - задана маска перехода, 1000 - маска перехода не задана
		int16  GoodsKind;     // 1 - goods, 2 - gen goods, 3 - group, 4 - altgroup, 5 - folder, 1000 - zero
		int16  PrevGoodsKind; // 1 - goods, 2 - gen goods, 3 - group, 4 - altgroup, 5 - folder, 1000 - zero
		int16  IsFormula;     // 1 - задана формула перехода, 1000 - формула перехода не задана
		int16  _Align;
	};
	ToolingSelector(PPID prcID, PPID goodsID, PPID prevGoodsID) : List(sizeof(Entry)), PrcID(prcID), GoodsID(goodsID), PrevGoodsID(prevGoodsID)
	{
	}
	int    Run(TSVector <TechTbl::Rec> * pList); // @v9.8.4 TSArray-->TSVect
private:
	int    LoadList(PPID prcID); // @recursion
	bool   IsSuited(const Entry * pEntry);

	PPObjTech TecObj;
	PPObjGoods GObj;
	PPObjProcessor PrcObj;
	PPIDArray PrcParentList;
	PPID   PrcID;
	PPID   GoodsID;
	PPID   PrevGoodsID;
	SArray List;
};

IMPL_CMPFUNC(ToolingEntry, i1, i2)
{
	const ToolingSelector::Entry * p1 = static_cast<const ToolingSelector::Entry *>(i1);
	const ToolingSelector::Entry * p2 = static_cast<const ToolingSelector::Entry *>(i2);
	if(p1->PrcID == p2->PrcID) {
		if(p1->GoodsID == p2->GoodsID) {
			if(p1->PrevGoodsID == p2->PrevGoodsID)
				return 0;
			else {
				if(p1->PrevGoodsKind == p2->PrevGoodsKind)
					if(p1->IsTranMask == p2->IsTranMask) {
						if(p1->IsFormula == p2->IsFormula)
							return 0;
						else if(p1->IsFormula < p2->IsFormula)
							return -1;
						else
							return 1;
					}
					else if(p1->IsTranMask < p2->IsTranMask)
						return -1;
					else
						return 1;
				else if(p1->PrevGoodsKind < p2->PrevGoodsKind)
					return -1;
				else
					return 1;
			}
		}
		else {
			if(p1->GoodsKind == p2->GoodsKind) {
				if(p1->IsTranMask == p2->IsTranMask)
					if(p1->IsFormula == p2->IsFormula)
						return 0;
					else if(p1->IsFormula < p2->IsFormula)
						return -1;
					else
						return 1;
				else if(p1->IsTranMask < p2->IsTranMask)
					return -1;
				else
					return 1;
			}
			else if(p1->GoodsKind < p2->GoodsKind)
				return -1;
			else
				return 1;
		}
	}
	else {
		if(p1->PrcKind == p2->PrcKind)
			return 0;
		else if(p1->PrcKind < p2->PrcKind)
			return -1;
		else
			return 1;
	}
}

int ToolingSelector::LoadList(PPID prcID)
{
	int16  prc_kind = 100;
	Goods2Tbl::Rec goods_rec;
	ProcessorTbl::Rec prc_rec;
	if(PrcObj.Fetch(prcID, &prc_rec) > 0) {
		if(prc_rec.Kind == PPPRCK_GROUP)
			prc_kind = 2;
		else if(prc_rec.Kind == PPPRCK_PROCESSOR)
			prc_kind = 1;
	}
	else
		MEMSZERO(prc_rec);
	TechTbl::Key2 k2;
	TechTbl * p_t = TecObj.P_Tbl;
	BExtQuery q(p_t, 2, 64);
	q.select(p_t->ID, p_t->PrcID, p_t->GoodsID, p_t->PrevGoodsID, p_t->TransClsID, p_t->TransMask, p_t->Flags, 0L).
		where(p_t->PrcID == prcID && p_t->Kind == static_cast<long>(TECK_TOOLING));
	MEMSZERO(k2);
	k2.PrcID = prcID;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		Entry entry;
		MEMSZERO(entry);
		entry.ID        = p_t->data.ID;
		entry.PrcID     = p_t->data.PrcID;
		entry.PrcKind   = prc_kind;
		entry.GoodsID   = p_t->data.GoodsID;
		entry.GoodsKind = 1000;
		entry.TransClsID = p_t->data.TransClsID;
		entry.TransMask  = p_t->data.TransMask;
		entry.IsTranMask = (entry.TransClsID && entry.TransMask) ? 1 : 1000;
		entry.IsFormula  = 1000;
		if(p_t->data.Flags & TECF_EXTSTRING) {
			SString formula;
			if(TecObj.GetToolingCondition(entry.ID, formula) > 0)
				entry.IsFormula = 1;
		}
		if(GObj.Fetch(entry.GoodsID, &goods_rec) > 0)
			if(goods_rec.Kind == PPGDSK_GOODS)
				entry.GoodsKind = (goods_rec.Flags & GF_GENERIC) ? 2 : 1;
			else
				entry.GoodsKind = (goods_rec.Flags & GF_ALTGROUP) ? 4 : ((goods_rec.Flags & GF_FOLDER) ? 5 : 3);
		entry.PrevGoodsID = p_t->data.PrevGoodsID;
		if(GObj.Fetch(entry.PrevGoodsID, &goods_rec) > 0)
			if(goods_rec.Kind == PPGDSK_GOODS)
				entry.PrevGoodsKind = (goods_rec.Flags & GF_GENERIC) ? 2 : 1;
			else
				entry.PrevGoodsKind = (goods_rec.Flags & GF_ALTGROUP) ? 4 : ((goods_rec.Flags & GF_FOLDER) ? 5 : 3);
		if(!List.insert(&entry))
			return PPSetErrorSLib();
	}
	if(prc_rec.ParentID)
		if(!LoadList(prc_rec.ParentID)) // @recursion
			return 0;
	return 1;
}

bool ToolingSelector::IsSuited(const Entry * pEntry)
{
	bool   is_suited = false;
	if(PrcID == pEntry->PrcID)
		is_suited = true;
	else if(PrcParentList.lsearch(pEntry->PrcID))
		is_suited = true;
	else
		is_suited = false;
	if(is_suited) {
		if(GoodsID == pEntry->GoodsID)
			is_suited = true;
		else if(GObj.BelongToGroup(GoodsID, pEntry->GoodsID, 0) > 0)
			is_suited = true;
		else if(pEntry->GoodsID == 0)
			is_suited = true;
		else
			is_suited = false;
	}
	if(is_suited) {
		if(PrevGoodsID == pEntry->PrevGoodsID)
			is_suited = true;
		else if(GObj.BelongToGroup(PrevGoodsID, pEntry->PrevGoodsID, 0) > 0)
			is_suited = true;
		else if(pEntry->PrevGoodsID == 0)
			is_suited = true;
		else
			is_suited = false;
	}
	if(is_suited) {
		if(pEntry->TransClsID && pEntry->TransMask) {
			is_suited = false; // @!
			Goods2Tbl::Rec goods_rec, prev_goods_rec;
			if(GObj.Fetch(GoodsID, &goods_rec) > 0 && GObj.Fetch(PrevGoodsID, &prev_goods_rec) > 0) {
				if(goods_rec.GdsClsID == prev_goods_rec.GdsClsID && goods_rec.GdsClsID == pEntry->TransClsID) {
					GoodsExtTbl::Rec ext_rec, prev_ext_rec;
					if(GObj.P_Tbl->GetExt(GoodsID, &ext_rec) > 0 && GObj.P_Tbl->GetExt(PrevGoodsID, &prev_ext_rec) > 0)
						if(PPGdsCls::IsEqByDynGenMask(pEntry->TransMask, &ext_rec, &prev_ext_rec))
							is_suited = false;
						else
							is_suited = true;
				}
			}
		}
	}
	if(is_suited && pEntry->IsFormula) {
		SString formula;
		if(TecObj.GetToolingCondition(pEntry->ID, formula) > 0) {
			GdsClsCalcExprContext ctx(GoodsID, PrevGoodsID);
			double result = 0.0;
			const  bool is_cfg_debug = LOGIC(CConfig.Flags & CCFLG_DEBUG);
			if(PPCalcExpression(formula, &result, &ctx)) {
				if(is_cfg_debug) {
					SString msg_buf;
					PPLoadText(PPTXT_LOG_TOOLINGSEL_FORMULA, msg_buf);
					msg_buf.Space().Cat(formula).Eq().Cat(result, SFMT_QTTY);
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				}
				if(result != 1.0)
					is_suited = false;
			}
			else {
				is_suited = false;
				if(is_cfg_debug)
					PPLogMessage(PPFILNAM_DEBUG_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			}
		}
	}
	return is_suited;
}

int ToolingSelector::Run(TSVector <TechTbl::Rec> * pList)
{
	int    ok = -1;
	uint   i;
	Entry * p_entry;
	SString fmt_buf, msg_buf;
	const bool is_cc_debug = LOGIC(CConfig.Flags & CCFLG_DEBUG);
	if(is_cc_debug) {
		PPLoadText(PPTXT_LOG_TOOLINGSEL_START, fmt_buf);
		PPFormat(fmt_buf, &msg_buf, PrcID, PrevGoodsID, GoodsID);
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
	List.freeAll();
	THROW(LoadList(PrcID));
	List.sort(PTR_CMPFUNC(ToolingEntry));
	THROW(PrcObj.GetParentsList(PrcID, &PrcParentList));
	for(i = 0; List.enumItems(&i, (void **)&p_entry);)
		if(IsSuited(p_entry)) {
			TechTbl::Rec tec_rec;
			THROW(TecObj.Fetch(p_entry->ID, &tec_rec) > 0);
			if(is_cc_debug) {
				PPLoadText(PPTXT_LOG_TOOLINGSEL_SEL, fmt_buf);
				PPFormat(fmt_buf, &msg_buf, tec_rec.ID);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			}
			if(pList)
				pList->insert(&tec_rec);
			ok = 1;
		}
	if(ok < 0) {
		if(is_cc_debug)
			PPLogMessage(PPFILNAM_DEBUG_LOG, PPSTR_TEXT, PPTXT_LOG_TOOLINGSEL_NOTHING, LOGMSGF_TIME|LOGMSGF_USER);
	}
	CATCHZOK
	return ok;
}

int PPObjTech::SelectTooling(PPID prcID, PPID goodsID, PPID prevGoodsID, TSVector <TechTbl::Rec> * pList)
{
	ToolingSelector ts(prcID, goodsID, prevGoodsID);
	return ts.Run(pList);
}
