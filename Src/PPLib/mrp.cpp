// MRP.CPP
// Copyright (c) A.Sobolev 2004, 2005, 2006, 2007, 2008, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(MrpTabCore)
//
MrpReqItem::MrpReqItem(PPID goodsID, long flags, double req, double price) : GoodsID(goodsID), Flags(flags), Req(req), Price(price)
{
}

MrpReqArray::MrpReqArray() : TSVector <MrpReqItem>()
{
}

int MrpReqArray::Add(PPID goodsID, long flags, double req, double price)
{
	int    ok = 1;
	uint   pos = 0;
	if(lsearch(&goodsID, &pos, CMPF_LONG)) {
		MrpReqItem & r_item = at(pos);
		r_item.Req += req;
		r_item.Price += price;
		r_item.Flags |= (flags & MRPLF_IGNOREREST);
		ok = 2;
	}
	else {
		MrpReqItem item(goodsID, (flags & MRPLF_IGNOREREST), req, price);
		ok = insert(&item) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

MrpTabCore::MrpTabCore() : MrpTabTbl()
{
}

int MrpTabCore::Search(PPID id, MrpTabTbl::Rec * pRec) { return SearchByID(this, PPOBJ_MRPTAB, id, pRec); }

int MrpTabCore::GetSubList(PPID tabID, PPIDArray * pList)
{
	int    ok = -1;
	MrpTabTbl::Key3 k3;
	MEMSZERO(k3);
	k3.ParentID = tabID;
	k3.LocID = -MAXLONG;
	BExtQuery q(this, 3, 64);
	q.select(this->ID, 0L).where(this->ParentID == tabID);
	for(q.initIteration(false, &k3, spGe); q.nextIteration() > 0;) {
		CALLPTRMEMB(pList, addUnique(data.ID));
		ok = 1;
	}
	return ok;
}

int MrpTabCore::GetParentID(PPID tabID, PPID * pParentID)
{
	if(Search(tabID, 0) > 0) {
		ASSIGN_PTR(pParentID, data.ParentID);
		return data.ParentID ? 1 : -1;
	}
	else
		return 0;
}

int MrpTabCore::SearchByLink(PPID objType, PPID objID, PPID locID, LDATE dt, MrpTabTbl::Rec * pRec)
{
	MrpTabTbl::Key1 k1;
	MEMSZERO(k1);
	k1.LinkObjType = objType;
	k1.LinkObjID = objID;
	k1.LocID = locID;
	k1.Dt = dt;
	return SearchByKey(this, 1, &k1, pRec);
}

int MrpTabCore::Create(PPID * pID, const MrpTabTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	int    r;
	MrpTabTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = SearchByLink(pRec->LinkObjType, pRec->LinkObjID, pRec->LocID, pRec->Dt, 0));
		STRNSCPY(rec.Name, pRec->Name);
		rec.ParentID = pRec->ParentID;
		rec.LinkObjType = pRec->LinkObjType;
		rec.LinkObjID = pRec->LinkObjID;
		rec.LocID = pRec->LocID;
		rec.Dt = pRec->Dt;
		CopyBufFrom(&rec, sizeof(rec));
		THROW_DB(insertRec(0, pID));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::Update(PPID id, const MrpTabTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	MrpTabTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(id, &rec) > 0);
		STRNSCPY(rec.Name, pRec->Name);
		rec.ParentID    = pRec->ParentID;
		rec.LinkObjType = pRec->LinkObjType;
		rec.LinkObjID   = pRec->LinkObjID;
		rec.LocID       = pRec->LocID;
		rec.Dt  = pRec->Dt;
		THROW_DB(updateRecBuf(&rec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::RemoveLines(PPID id, int use_ta)
{
	return deleteFrom(&Lines, use_ta, Lines.TabID == id) ? 1 : PPSetErrorDB();
}

int MrpTabCore::Remove(PPID id, int use_ta)
{
	int    ok = 1;
	uint   i;
	PPIDArray sub_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		GetSubList(id, &sub_list);
		for(i = 0; i < sub_list.getCount(); i++) {
			THROW_DB(deleteFrom(&Lines, 0, Lines.TabID == sub_list.at(i)));
			THROW_DB(deleteFrom(this, 0, this->ID == sub_list.at(i)));
		}
		THROW_DB(deleteFrom(&Lines, 0, Lines.TabID == id));
		THROW_DB(deleteFrom(this, 0, this->ID == id));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::SearchLine(PPID tabID, PPID destID, PPID srcID, MrpLineTbl::Rec * pRec)
{
	MrpLineTbl::Key1 k1;
	k1.TabID = tabID;
	k1.DestID = destID;
	k1.SrcID = srcID;
	return SearchByKey(&Lines, 1, &k1, pRec);
}

int MrpTabCore::GetTotalLine(PPID tabID, PPID goodsID, MrpLineTbl::Rec * pRec) { return SearchLine(tabID, goodsID, MRPSRCV_TOTAL, pRec); }
int MrpTabCore::SearchLineByID(PPID lineID, MrpLineTbl::Rec * pRec) { return SearchByID(&Lines, 0, lineID, pRec); }

int MrpTabCore::AddLine(PPID id, PPID destID, PPID srcID, double destReqQtty, double srcReqQtty, double price, long flags, int use_ta)
{
	int    ok = 1;
	MrpLineTbl::Rec line_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchLine(id, destID, srcID, &line_rec) > 0) {
			if(flags & MRPLF_SUBST)
				THROW_PP(line_rec.Flags & MRPLF_SUBST, PPERR_NSUBSTMRPLINEONSUBST);
			line_rec.DestReqQtty += destReqQtty;
			line_rec.SrcReqQtty  += srcReqQtty;
			line_rec.Price += price;
			THROW_DB(Lines.updateRecBuf(&line_rec));
		}
		else {
			line_rec.Clear();
			line_rec.TabID  = id;
			line_rec.DestID = destID;
			line_rec.SrcID  = srcID;
			line_rec.DestReqQtty = destReqQtty;
			line_rec.SrcReqQtty  = srcReqQtty;
			line_rec.Price = price;
			line_rec.Flags |= (flags & (MRPLF_TERMINAL|MRPLF_SUBST|MRPLF_IGNOREREST));
			THROW_DB(Lines.insertRecBuf(&line_rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::AddCTab(const CMrpTab * pTab, int use_ta)
{
	int    ok = 1;
	BExtInsert bei(&Lines);
	CMrpTab::Row * p_row;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; pTab->enumItems(&i, (void **)&p_row);) {
			MrpLineTbl::Rec rec;
			rec.TabID = p_row->TabID;
			rec.DestID = p_row->DestID;
			rec.SrcID = p_row->SrcID;
			rec.DestReqQtty = p_row->DestReq;
			rec.SrcReqQtty = p_row->SrcReq;
			rec.Price = p_row->Price;
			rec.Flags = p_row->Flags;
			THROW_DB(bei.insert(&rec));
		}
		THROW(bei.flash());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::SetFlag(PPID tabID, PPID destID, PPID srcID, long flag, int set, int use_ta)
{
	int    ok = -1;
	MrpLineTbl::Rec line_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchLine(tabID, destID, srcID, &line_rec) > 0) {
			SETFLAG(line_rec.Flags, flag, set);
			THROW_DB(Lines.updateRecBuf(&line_rec));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::EnumLinesByDest(PPID id, PPID destID, PPID * pSrcID, MrpLineTbl::Rec * pRec)
{
	MrpLineTbl::Key1 k1;
	k1.TabID = id;
	k1.DestID = destID;
	k1.SrcID = *pSrcID;
	if(Lines.search(1, &k1, spGt) && k1.TabID == id && k1.DestID == destID) {
		Lines.CopyBufTo(pRec);
		*pSrcID = Lines.data.SrcID;
		return 1;
	}
	else
		return BTRNFOUND ? -1 : PPSetErrorDB();
}

int MrpTabCore::EnumLinesBySrc(PPID id, PPID srcID, PPID * pDestID, MrpLineTbl::Rec * pRec)
{
	MrpLineTbl::Key2 k2;
	k2.TabID = id;
	k2.SrcID = srcID;
	k2.DestID = *pDestID;
	if(Lines.search(1, &k2, spGt) && k2.TabID == id && k2.SrcID == srcID) {
		Lines.CopyBufTo(pRec);
		*pDestID = Lines.data.DestID;
		return 1;
	}
	else
		return BTRNFOUND ? -1 : PPSetErrorDB();
}

int MrpTabCore::GetSrcList(PPID tabID, PPID destID, RAssocArray * pList)
{
	MrpLineTbl::Key1 k1;
	k1.TabID = tabID;
	k1.DestID = destID;
	k1.SrcID = -MAXLONG;
	BExtQuery q(&Lines, 1, 128);
	q.select(Lines.SrcID, 0L).where(Lines.TabID == tabID && Lines.DestID == destID);
	for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
		CALLPTRMEMB(pList, Add(Lines.data.SrcID, Lines.data.SrcReqQtty, 1, 0));
	}
	return pList->getCount() ? 1 : -1;
}

int MrpTabCore::GetDestList(PPID tabID, PPID srcID, int minusSrcReq, MrpReqArray * pList)
{
	int    ok = -1;
	MrpLineTbl::Key1 k1;
	k1.TabID = tabID;
	k1.SrcID = srcID;
	k1.DestID = -MAXLONG;
	BExtQuery q(&Lines, 1, 128);
	q.select(Lines.DestID, Lines.DestReqQtty, Lines.SrcReqQtty, Lines.Price, Lines.Flags, 0L).
		where(Lines.TabID == tabID && Lines.SrcID == srcID);
	for(q.initIteration(false, &k1, spGe); ok && q.nextIteration() > 0;) {
		ok = 1;
		if(pList) {
			double val = minusSrcReq ? (Lines.data.DestReqQtty - Lines.data.SrcReqQtty) : Lines.data.DestReqQtty;
			if(!pList->Add(Lines.data.DestID, Lines.data.Flags, val, Lines.data.Price))
				ok = 0;
		}
	}
	return ok;
}

int MrpTabCore::IsGoodsFlagged(PPID tabID, PPID goodsID, long flag)
{
	MrpLineTbl::Rec rec;
	int    r = SearchLine(tabID, goodsID, 0, &rec);
	return (r > 0) ? ((rec.Flags & flag) ? 1 : -1) : (r ? -1 : 0);
}

int MrpTabCore::IsTerminalGoods(PPID tabID, PPID goodsID)
	{ return IsGoodsFlagged(tabID, goodsID, MRPLF_TERMINAL); }
int MrpTabCore::IsReplacedGoods(PPID tabID, PPID goodsID)
	{ return IsGoodsFlagged(tabID, goodsID, MRPLF_REPLACED); }

int MrpTabCore::GetDependencyList(PPID tabID, PPID destGoodsID, PUGL * pList)
{
	MrpLineTbl::Key1 k1;
	k1.TabID = tabID;
	k1.DestID = destGoodsID;
	k1.SrcID = 0;
	BExtQuery q(&Lines, 1, 128);
	q.select(Lines.SrcID, 0L).
		where(Lines.TabID == tabID && Lines.DestID == destGoodsID && Lines.SrcID > 0.0);
	for(q.initIteration(false, &k1, spGt); q.nextIteration() > 0;) {
		PUGI   pugi;
		pugi.GoodsID = Lines.data.SrcID;
		if(IsTerminalGoods(tabID, pugi.GoodsID) > 0)
			pugi.Flags |= PUGI::fTerminal;
		pList->Add(&pugi, ZERODATE);
	}
	return pList->getCount() ? 1 : -1;
}

int MrpTabCore::Helper_GetDeficit(const MrpLineTbl::Rec & rRec, int terminal, int replacePassiveGoods, PUGL * pList)
{
	int    ok = -1;
	if(terminal > 0 && !(rRec.Flags & MRPLF_TERMINAL))
		ok = -1;
	else if(terminal < 0 && rRec.Flags & MRPLF_TERMINAL)
		ok = -1;
	else if(rRec.DestDfct >= BillCore::GetQttyEpsilon()) {
		PPID   goods_id = rRec.DestID;
		double ratio = 1.0;
		PUGI   pugi;
		if(replacePassiveGoods) {
			Goods2Tbl::Rec goods_rec;
			PPObjGoods goods_obj;
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0 && (goods_rec.Flags & (GF_PASSIV|GF_GENERIC))) {
				RAssocArray alt_goods_list;
				THROW(goods_obj.GetSubstList(goods_id, 0, alt_goods_list));
				for(uint i = 0; i < alt_goods_list.getCount(); i++) {
					const  PPID alt_goods_id = alt_goods_list.at(i).Key;
					if(goods_obj.Fetch(alt_goods_id, &goods_rec) > 0 && !(goods_rec.Flags & (GF_PASSIV|GF_GENERIC))) {
						ratio = alt_goods_list.at(i).Val;
						goods_id = alt_goods_id;
						break;
					}
				}
			}
		}
		pugi.GoodsID = goods_id;
		pugi.LocID   = pList->LocID;
		SETFLAG(pugi.Flags, PUGI::fTerminal, rRec.Flags & MRPLF_TERMINAL);
		pugi.NeededQty  = rRec.DestReqQtty * ratio;
		pugi.DeficitQty = rRec.DestDfct * ratio;
		pugi.Price = R5(fdivnz(rRec.Price, rRec.DestReqQtty));
		THROW(pList->Add(&pugi, pList->Dt));
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
// terminal: -1 - non terminal only, 0 - all, 1 - terminal only
//
int MrpTabCore::GetDeficitList_(PPID tabID, PPID srcID, int terminal, int replacePassiveGoods, PUGL * pList)
{
	int    ok = -1;
	PPObjGoods * p_goods_obj = 0;
	MrpTabTbl::Rec tab_rec;
	MrpLineTbl::Rec rec;
	THROW(Search(tabID, &tab_rec) > 0);
	{
		pList->Dt = tab_rec.Dt;
		pList->LocID = tab_rec.LocID;
		RAssocArray alt_goods_list;
		MrpLineTbl::Key2 k2;
		BExtQuery q(&Lines, 2);
		DBQ * dbq = &(Lines.TabID == tabID && Lines.SrcID == srcID/* && Lines.DestDfct > 0.0*/);
		q.select(Lines.DestID, Lines.DestReqQtty, Lines.DestDfct, Lines.Flags, Lines.Price, 0L);
		if(srcID != MRPSRCV_TOTAL) {
			q.where(Lines.TabID == tabID && Lines.SrcID == srcID);
			MEMSZERO(k2);
			k2.TabID = tabID;
			k2.SrcID = srcID;
			PPIDArray dest_list;
			for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
				Lines.CopyBufTo(&rec);
				dest_list.add(rec.DestID);
			}
			for(uint i = 0; i < dest_list.getCount(); i++) {
				const  PPID goods_id = dest_list.get(i);
				if(GetTotalLine(tabID, goods_id, &rec) > 0) {
					int    gdr = 0;
					THROW(gdr = Helper_GetDeficit(rec, terminal, replacePassiveGoods, pList));
					if(gdr > 0)
						ok = 1;
				}
			}
		}
		else {
			q.where(Lines.TabID == tabID && Lines.SrcID == srcID && Lines.DestDfct > 0.0);
			MEMSZERO(k2);
			k2.TabID = tabID;
			k2.SrcID = srcID;
			for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
				Lines.CopyBufTo(&rec);
				int    gdr = 0;
				THROW(gdr = Helper_GetDeficit(rec, terminal, replacePassiveGoods, pList));
				if(gdr > 0)
					ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_goods_obj;
	return ok;
}

int MrpTabCore::GetSubst(PPID tabID, GoodsReplacementArray * pGra)
{
	int    ok = -1;
	MrpTabTbl::Rec tab_rec;
	PPObjGoods goods_obj;
	THROW(Search(tabID, &tab_rec) > 0);
	{
		MrpLineTbl::Key2 k2;
		BExtQuery q(&Lines, 2);
		q.select(Lines.DestID, Lines.DestReqQtty, Lines.DestDfct, Lines.Flags, 0L).
			where(Lines.TabID == tabID && Lines.SrcID == (long)MRPSRCV_TOTAL);
		MEMSZERO(k2);
		k2.TabID = tabID;
		for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
			MrpLineTbl::Rec rec;
			Lines.CopyBufTo(&rec);
			if(rec.Flags & MRPLF_TERMINAL && rec.Flags & MRPLF_REPLACED) {
				const  PPID dest_id = rec.DestID;
				MrpLineTbl::Key1 k1;
				BExtQuery q2(&Lines, 1);
				q2.select(Lines.SrcID, Lines.SrcReqQtty, Lines.Flags, 0L).where(Lines.TabID == tabID && Lines.DestID == dest_id);
				MEMSZERO(k1);
				k1.TabID = tabID;
				k1.DestID = dest_id;
				for(q2.initIteration(false, &k1, spGe); q2.nextIteration() > 0;) {
					const  PPID src_id = Lines.data.SrcID;
					double ratio = 0.0;
					if(src_id > 0 && Lines.data.Flags & MRPLF_SUBST && goods_obj.IsGoodsCompatibleByUnit(src_id, dest_id, &ratio) > 0) {
						THROW(pGra->Add(dest_id, src_id, Lines.data.SrcReqQtty, ratio));
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::SetRest(PPID id, PPID destID, const GoodsRestVal * pVal, double * pDeficit, int use_ta)
{
	int    ok = 1;
	double dfct = 0.0;
	MrpLineTbl::Rec line_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchLine(id, destID, MRPSRCV_TOTAL, &line_rec) > 0) {
			if(pVal) {
				const  double req = line_rec.DestReqQtty;
				double rest = pVal->Rest;
				dfct  = req - rest;
				line_rec.DestRest = rest;
				line_rec.DestDfct = (dfct > 0.0) ? dfct : 0.0;
			}
			else { // pVal==0 ==> destID - нелимитируемый ресурс
				line_rec.Flags |= MRPLF_UNLIM;
				line_rec.DestRest = 0.0;
				line_rec.DestDfct = 0.0;
			}
			THROW_DB(Lines.updateRecBuf(&line_rec));
			if(dfct > 0 && line_rec.Flags & MRPLF_TERMINAL)
				ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pDeficit, dfct);
	return ok;
}

int MrpTabCore::SetSubstRest(PPID id, PPID destID, PPID srcID, const GoodsRestVal * pVal, double ratio, double * pDeficit, int use_ta)
{
	int    ok = 1;
	double dfct = 0.0;
	double subst_used_qtty = 0.0;
	int    is_src_line_exists = 0;
	MrpLineTbl::Rec line_rec, line_rec2, subst_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchLine(id, destID, MRPSRCV_TOTAL, &line_rec) > 0) {
			//
			// Выясняем сколько замещающего товара уже использовано (subst_used_qtty) этой MRP-таблицей
			//
			if(SearchLine(id, srcID, MRPSRCV_TOTAL, &subst_rec) > 0) {
				subst_used_qtty = subst_rec.DestReqQtty;
				is_src_line_exists = 1;
			}
			const  double rest = pVal->Rest - subst_used_qtty;
			//
			// Если есть, что тратить, то запускаем процесс
			//
			if(rest > 0.0) {
				if(SearchLine(id, destID, srcID, &line_rec2) > 0 && !(line_rec2.Flags & MRPLF_SUBST)) {
					SString msg_buf, fmt_buf;
					PPObjGoods goods_obj;
					Goods2Tbl::Rec dest_goods_rec, src_goods_rec;
					PPLoadText(PPTXT_LOG_INVMRPSUBT, fmt_buf);
					goods_obj.Fetch(destID, &dest_goods_rec);
					goods_obj.Fetch(srcID,  &src_goods_rec);
					msg_buf.Printf(fmt_buf, dest_goods_rec.Name, src_goods_rec.Name);
					PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				}
				else {
					const  double req  = line_rec.DestDfct * ratio;
					const  double usage = MIN(req, rest);
					double dest_usage = usage / ratio;
					dfct = line_rec.DestDfct - dest_usage;
					THROW_PP(line_rec.Flags & MRPLF_TERMINAL, PPERR_MRPSUBSTNTERM);
					line_rec.Flags |= MRPLF_REPLACED;
					line_rec.SrcReqQtty += dest_usage;
					line_rec.DestDfct   = dfct;
					const double dr_sq = line_rec.DestRest + line_rec.SrcReqQtty;
					line_rec.Cost  = (line_rec.Cost  * dr_sq + pVal->Cost  * usage) / (dr_sq + dest_usage);
					line_rec.Price = (line_rec.Price * dr_sq + pVal->Price * usage) / (dr_sq + dest_usage);
					//
					// Возвращаем указатель на запись line_rec и меняем ее
					//
					THROW(SearchLineByID(line_rec.ID, 0) > 0);
					THROW_DB(Lines.updateRecBuf(&line_rec));
					THROW(AddLine(id, destID, srcID, dest_usage, usage, pVal->Price * dest_usage, MRPLF_SUBST, 0));
					{
						const double rest_addendum = is_src_line_exists ? 0.0 : pVal->Rest;
						THROW(AddLine(id, srcID, MRPSRCV_TOTAL, usage, rest_addendum, pVal->Price * usage, /*MRPLF_SUBST*/0, 0));
					}
					if(dfct > 0.0)
						ok = 2;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pDeficit, dfct);
	return ok;
}

int MrpTabCore::RemoveSubst(PPID tabID, PPID destID, PPID srcID, int use_ta)
{
	int    ok = -1;
	MrpLineTbl::Rec rec, subst_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(SearchLine(tabID, destID, MRPSRCV_TOTAL, &rec) > 0) {
			double decr_qtty = 0;
			THROW_PP(rec.Flags & MRPLF_TERMINAL, PPERR_MRPSUBSTNTERM);
			if(srcID) {
				if(SearchLine(tabID, destID, srcID, &subst_rec) > 0)
					decr_qtty += subst_rec.DestReqQtty;
				THROW_DB(deleteFrom(&Lines, 0, Lines.ID == subst_rec.ID));
			}
			else {
				while(EnumLinesByDest(tabID, destID, &srcID, &subst_rec) > 0)
					decr_qtty += subst_rec.DestReqQtty;
				THROW_DB(deleteFrom(&Lines, 0, Lines.TabID == tabID && Lines.DestID == destID));
			}
			if(decr_qtty) {
				rec.SrcReqQtty -= decr_qtty;
				rec.DestDfct   += decr_qtty;
				if(rec.SrcReqQtty <= 0.0)
					rec.Flags &= ~MRPLF_REPLACED;
				THROW(UpdateByID(&Lines, 0, rec.ID, &rec, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabCore::Aggregate(PPID destTabID, PPID srcTabID, int use_ta)
{
	int    ok = 1;
	MrpLineTbl::Key1 k1;
	BExtQuery q(&Lines, 1);
	q.selectAll().where(Lines.TabID == srcTabID);
	MEMSZERO(k1);
	k1.TabID = srcTabID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(q.initIteration(false, &k1, spGt); q.nextIteration() > 0;) {
			MrpLineTbl::Rec src_rec;
			Lines.CopyBufTo(&src_rec);
			THROW(AddLine(destTabID, src_rec.DestID, src_rec.SrcID, src_rec.DestReqQtty,
				src_rec.SrcReqQtty, src_rec.Price, src_rec.Flags & MRPLF_TERMINAL, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// @ModuleDef(PPObjMrpTab)
//
IMPL_CMPFUNC(CMrpRow, i1, i2)
{
	const CMrpTab::Row * p1 = static_cast<const CMrpTab::Row *>(i1);
	const CMrpTab::Row * p2 = static_cast<const CMrpTab::Row *>(i2);
	int    r = cmp_long(p1->TabID, p2->TabID);
	return NZOR(NZOR(r, cmp_long(p1->DestID, p2->DestID)), cmp_long(p1->SrcID, p2->SrcID));
}

CMrpTab::CMrpTab() : SArray(sizeof(Row))
{
}

CMrpTab::Row & FASTCALL CMrpTab::at(uint i) const
{
	return *static_cast<Row *>(SArray::at(i));
}

int CMrpTab::SetFlag(PPID tabID, PPID destID, PPID srcID, long flag, int set)
{
	int    ok = -1;
	uint   p = 0;
	if(Search(tabID, destID, srcID, &p, 0)) {
		Row & r_row = at(p);
		SETFLAG(r_row.Flags, flag, set);
		ok = 1;
	}
	return ok;
}

void CMrpTab::Sort()
{
	sort(PTR_CMPFUNC(CMrpRow));
}

int CMrpTab::Search(PPID tabID, PPID destID, PPID srcID, uint * pPos, CMrpTab::Row * pRow) const
{
	uint   pos = DEREFPTRORZ(pPos);
	Row    pat;
	pat.TabID  = tabID;
	pat.DestID = destID;
	pat.SrcID  = srcID;
	int    sr = (SArray::VFlags & arySorted) ? bsearch(&pat, &pos, PTR_CMPFUNC(CMrpRow)) : lsearch(&pat, &pos, PTR_CMPFUNC(CMrpRow));
	if(sr) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pRow, at(pos));
		return 1;
	}
	else {
		memzero(pRow, sizeof(*pRow));
		return 0;
	}
}

int CMrpTab::Add__(PPID tabID, PPID destID, PPID srcID, double destReqQtty, double srcReqQtty, double price,
	/*int term*/ long flags)
{
	int    ok = 1;
	uint   p = 0;
	Row    row;
	row.TabID  = tabID;
	row.DestID = destID;
	row.SrcID  = srcID;
	if(lsearch(&row, &p, PTR_CMPFUNC(CMrpRow))) {
		Row & r_row = at(p);
		double sum_dest = r_row.DestReq + destReqQtty;
		if(price == 0.0 && r_row.Price > 0 && r_row.DestReq != 0.0)
			r_row.Price  = (r_row.Price / r_row.DestReq) * sum_dest;
		else
			r_row.Price += price;
		r_row.DestReq  = sum_dest;
		r_row.SrcReq  += srcReqQtty;
		//SETFLAG(r_row.Flags, MRPLF_TERMINAL, term);
		//
		//SETFLAG(r_row.Flags, MRPLF_TERMINAL,   flags & MRPLF_TERMINAL);
		//SETFLAG(r_row.Flags, MRPLF_IGNOREREST, flags & MRPLF_IGNOREREST);
		//
		r_row.Flags |= (int16)(flags & MRPLF_TERMINAL);
		r_row.Flags |= (int16)(flags & MRPLF_IGNOREREST);
	}
	else {
		row.DestReq = destReqQtty;
		row.SrcReq  = srcReqQtty;
		row.Price   = price;
		//row.Flags   = term ? MRPLF_TERMINAL : 0;
		row.Flags   = (int16)(flags & (MRPLF_TERMINAL|MRPLF_IGNOREREST));
		THROW_SL(insert(&row));
	}
	CATCHZOK
	return ok;
}

int CMrpTab::Aggregate(PPID destTabID)
{
	int    ok = 1;
	CMrpTab total;
	Row  * p_row;
	uint   i;
	for(i = 0; enumItems(&i, (void **)&p_row);) {
		//THROW(total.Add(destTabID, p_row->DestID, p_row->SrcID, p_row->DestReq, p_row->SrcReq, p_row->Price, BIN(p_row->Flags & MRPLF_TERMINAL)));
		THROW(total.Add__(destTabID, p_row->DestID, p_row->SrcID, p_row->DestReq, p_row->SrcReq, p_row->Price, p_row->Flags));
	}
	for(i = 0; total.enumItems(&i, (void **)&p_row);)
		THROW_SL(insert(p_row));
	CATCHZOK
	return ok;
}
//
//
//
MrpTabLeaf::MrpTabLeaf(PPID tabID, PPID locID, LDATE dt) : TabID(tabID), LocID(locID), Dt(dt)
{
}

MrpTabPacket::MrpTabPacket() : TSVector <MrpTabLeaf>()
{
	ObjType  = 0;
	ObjID    = 0;
	BaseID   = 0;
	Name[0]  = 0;
}

MrpTabPacket & FASTCALL MrpTabPacket::operator = (const MrpTabPacket & s)
{
	copy(s);
	ObjType = s.ObjType;
	ObjID = s.ObjID;
	BaseID = s.ObjID;
	STRNSCPY(Name, s.Name);
	Cache.copy(s.Cache);
	return *this;
}

void MrpTabPacket::Init(PPID objType, PPID objID, const char * pName)
{
	Cache.freeAll();
	ObjType = objType;
	ObjID = objID;
	BaseID = 0;
	STRNSCPY(Name, pName);
}

void MrpTabPacket::Destroy()
{
	freeAll();
	BaseID = 0;
	Cache.freeAll();
}

IMPL_CMPFUNC(MrpTabLeaf, i1, i2) { RET_CMPCASCADE3(static_cast<const MrpTabLeaf *>(i1), static_cast<const MrpTabLeaf *>(i2), Dt, LocID, TabID); }

bool MrpTabPacket::IsTree() const { return LOGIC(BaseID); }
const char * MrpTabPacket::GetName() const { return Name; }
PPID MrpTabPacket::GetBaseID() const { return (getCount() == 1 && !BaseID) ? at(0).TabID : BaseID; }
void FASTCALL MrpTabPacket::SetBaseID(PPID id) { BaseID = id; }
void MrpTabPacket::Sort() { sort(PTR_CMPFUNC(MrpTabLeaf)); }
void MrpTabPacket::SortCache() { Cache.Sort(); }
int  MrpTabPacket::Flash(MrpTabCore * pTbl, int use_ta) { return pTbl->AddCTab(&Cache, use_ta); }

void MrpTabPacket::GetCommonParam(PPIDArray * pLocList, DateRange * pPeriod) const
{
	MrpTabLeaf * p_item;
	CALLPTRMEMB(pLocList, freeAll());
	CALLPTRMEMB(pPeriod, Set(MAXDATE, ZERODATE));
	for(uint i = 0; enumItems(&i, (void **)&p_item);) {
		//
		// Определяем общие параметры списка листьев:
		//   список складов, минимальная и максимальная даты
		//
		if(pLocList)
			if(p_item->LocID)
				pLocList->addUnique(p_item->LocID);
		CALLPTRMEMB(pPeriod, AdjustToDate(p_item->Dt));
	}
}

int MrpTabPacket::GetTabID(PPID locID, LDATE dt, PPID * pTabID) const
{
	MrpTabLeaf * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);) {
		if(p_item->LocID == locID && p_item->Dt == dt) {
			ASSIGN_PTR(pTabID, p_item->TabID);
			return 1;
		}
	}
	return 0;
}

int FASTCALL MrpTabPacket::AddLeaf(const MrpTabTbl::Rec * pRec)
{
	MrpTabLeaf leaf(pRec->ID, pRec->LocID, pRec->Dt);
	return insert(&leaf) ? 1 : PPSetErrorSLib();
}

int MrpTabPacket::GetLeaf(PPID tabID, MrpTabLeaf * pLeaf) const
{
	uint pos = 0;
	if(lsearch(&tabID, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pLeaf, at(pos));
		return 1;
	}
	else
		return 0;
}

void MrpTabPacket::CreateLeafRec(PPID locID, LDATE dt, MrpTabTbl::Rec & rRec) const
{
	memzero(&rRec, sizeof(rRec));
	rRec.ParentID = GetBaseID();
	rRec.LinkObjType = ObjType;
	rRec.LinkObjID = ObjID;
	rRec.Dt = dt;
	rRec.LocID = locID;
}

int MrpTabPacket::GetList(PPIDArray * pList) const
{
	MrpTabLeaf * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);)
		pList->addUnique(p_item->TabID);
	if(BaseID)
		pList->addUnique(BaseID);
	return pList->getCount() ? 1 : -1;
}

int MrpTabPacket::AddLine__(PPID tabID, PPID destID, PPID srcID, double destReq, double srcReq, double price, /*int term*/long flags)
{
	return Cache.Add__(tabID, destID, srcID, destReq, srcReq, price, /*term ? MRPLF_TERMINAL : 0*/flags);
}

int MrpTabPacket::SetTerminal(PPID tabID, PPID destID, int term)
{
	return Cache.SetFlag(tabID, destID, MRPSRCV_TOTAL, MRPLF_TERMINAL, term);
}

int MrpTabPacket::IsTerminal(PPID tabID, PPID destID) const
{
	CMrpTab::Row row;
	return Cache.Search(tabID, destID, MRPSRCV_TOTAL, 0, &row) ? BIN(row.Flags & MRPLF_TERMINAL) : -1;
}

int MrpTabPacket::GetDestList(PPID tabID, PPID srcID, int minusSrcReq, MrpReqArray * pList) const
{
	int    ok = -1;
	CMrpTab::Row * p_row;
	for(uint i = 0; ok && Cache.enumItems(&i, (void **)&p_row);) {
		if(p_row->TabID == tabID && p_row->SrcID == srcID) {
			ok = 1;
			if(pList) {
				double val = minusSrcReq ? (p_row->DestReq - p_row->SrcReq) : p_row->DestReq;
				if(!pList->Add(p_row->DestID, p_row->Flags, val, p_row->Price))
					ok = 0;
			}
		}
	}
	return ok;
}

int MrpTabPacket::Aggregate()
{
	int    ok = 1;
	if(IsTree() && GetBaseID()) {
		if(!Cache.Aggregate(GetBaseID()))
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}
//
//
//
//static
int FASTCALL PPObjMrpTab::ReadConfig(PPMrpTabConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_MRPTABCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

//static
int PPObjMrpTab::GetCounter(long * pCounter, int use_ta)
{
	int    ok = 1;
	long   c = 0;
	PPMrpTabConfig cfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ReadConfig(&cfg));
		c = ++cfg.Counter;
		THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_MRPTABCFG, &cfg, sizeof(cfg), 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pCounter, c);
	return ok;
}

//static
void PPObjMrpTab::GenerateName(PPID linkObjType, PPID linkObjID, SString * pName, int use_ta)
{
	if(pName) {
		long   c = 0;
		GetCounter(&c, use_ta);
		if(linkObjType && linkObjID) {
			CatObjectName(linkObjType, linkObjID, *pName);
			pName->Space();
		}
		pName->CatChar('#').Cat(c);
	}
}

TLP_IMPL(PPObjMrpTab, MrpTabCore, P_Tbl);

PPObjMrpTab::PPObjMrpTab(void * extraPtr) : PPObject(PPOBJ_MRPTAB), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
}

PPObjMrpTab::~PPObjMrpTab()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjMrpTab::Search(PPID id, void * pRec) { return P_Tbl->Search(id, (MrpTabTbl::Rec *)pRec); }
/*virtual*/const char * PPObjMrpTab::GetNamePtr() { return P_Tbl->data.Name; }
int PPObjMrpTab::DeleteObj(PPID id) { return P_Tbl->Remove(id, 0); }

int PPObjMrpTab::SetupLinkObjTypeCombo(TDialog * dlg, uint ctlID, PPID initObjType)
{
	PPIDArray obj_type_list;
	obj_type_list.addzlist(PPOBJ_GOODS, PPOBJ_BILL, PPOBJ_DRAFTWROFF, 0);
	return SetupObjListCombo(dlg, ctlID, initObjType, &obj_type_list);
}
//
//
//
class MrpTabDialog : public TDialog {
public:
	MrpTabDialog() : TDialog(DLG_MRPTAB)
	{
		SetupCalDate(CTLCAL_MRPTAB_DATE, CTL_MRPTAB_DATE);
	}
	int    setDTS(const MrpTabTbl::Rec *);
	int    getDTS(MrpTabTbl::Rec *);
private:
	DECL_HANDLE_EVENT;

	MrpTabTbl::Rec Data;
};

IMPL_HANDLE_EVENT(MrpTabDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmaMore)) {
		const  PPID id = Data.LinkObjID;
		if(Data.LinkObjType && id) {
			if(EditPPObj(Data.LinkObjType, id) > 0) {
				SString name_buf;
				if(GetObjectName(Data.LinkObjType, id, name_buf) > 0)
					setCtrlString(CTL_MRPTAB_LINKOBJNAME, name_buf);
			}
		}
		clearEvent(event);
	}
}

int MrpTabDialog::setDTS(const MrpTabTbl::Rec * pData)
{
	Data = *pData;
	setCtrlData(CTL_MRPTAB_NAME, Data.Name);
	setCtrlData(CTL_MRPTAB_ID, &Data.ID);
	setCtrlData(CTL_MRPTAB_PARENTID, &Data.ParentID);
	PPObjMrpTab::SetupLinkObjTypeCombo(this, CTLSEL_MRPTAB_LINKOBJ, Data.LinkObjType);
	if(Data.LinkObjType) {
		SString obj_name;
		GetObjectName(Data.LinkObjType, Data.LinkObjID, obj_name);
		setCtrlString(CTL_MRPTAB_LINKOBJNAME, obj_name);
	}
	disableCtrls(1, CTL_MRPTAB_ID, CTLSEL_MRPTAB_LINKOBJ, CTL_MRPTAB_LINKOBJNAME, 0);
	setCtrlData(CTL_MRPTAB_DATE, &Data.Dt);
	SetupPPObjCombo(this, CTLSEL_MRPTAB_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	enableCommand(cmaMore, Data.LinkObjType && Data.LinkObjID);
	return 1;
}

int MrpTabDialog::getDTS(MrpTabTbl::Rec * pData)
{
	int    ok = 1;
	getCtrlData(CTL_MRPTAB_PARENTID, &Data.ParentID);
	getCtrlData(CTL_MRPTAB_NAME, Data.Name);
	getCtrlData(CTL_MRPTAB_DATE, &Data.Dt);
	getCtrlData(CTLSEL_MRPTAB_LOC, &Data.LocID);
	if(*strip(Data.Name) == 0) {
		selectCtrl(CTL_MRPTAB_NAME);
		ok = (PPError(PPERR_NAMENEEDED, 0), 0);
	}
	else
		ASSIGN_PTR(pData, Data);
	return ok;
}

int PPObjMrpTab::EditDialog(MrpTabTbl::Rec * pRec) { DIALOG_PROC_BODY(MrpTabDialog, pRec); }

int PPObjMrpTab::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	bool   is_new = false;
	const  PPConfig & r_cfg = LConfig;
	MrpTabTbl::Rec rec;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	else {
		rec.Dt = r_cfg.OperDate;
		rec.LocID = r_cfg.Location;
	}
	if(EditDialog(&rec) > 0) {
		if(*pID) {
			THROW(P_Tbl->Update(*pID, &rec, 1));
		}
		else {
			THROW(P_Tbl->Create(pID, &rec, 1));
		}
		r = cmOK;
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjMrpTab::Browse(void * extraPtr)
{
	return ViewMrpTab(0);
}

int PPObjMrpTab::CheckForFilt(const MrpTabFilt * pFilt, PPID id, MrpTabTbl::Rec * pRec)
{
	MrpTabTbl::Rec rec;
	if(pRec == 0) {
		if(Search(id, &rec) > 0)
			pRec = &rec;
		else
			return 0;
	}
	if(!pFilt)
		return 1;
	// @v10.8.3 @fix if(pFilt->ParentID) return (pRec->ParentID != pFilt->ParentID) ? 0 : 1;
	if(!CheckFiltID(pFilt->ParentID, pRec->ParentID)) return 0; // @v10.8.3 @fix
	if(pFilt->LinkObjType) {
		if(pRec->LinkObjType != pFilt->LinkObjType)
			return 0;
		else if(pFilt->LinkObjID && pRec->LinkObjID != pFilt->LinkObjID)
			return 0;
	}
	if(!CheckFiltID(pFilt->LocID, pRec->LocID))
		return 0;
	else if(!pFilt->Period.CheckDate(pRec->Dt))
		return 0;
	else if(pFilt->Flags & MrpTabFilt::fSkipChilds && pRec->ParentID)
		return 0;
	else
		return 1;
}

int PPObjMrpTab::LoadPacket(PPID tabID, MrpTabPacket * pTree)
{
	int    ok = 1, r;
	PPID   parent_id = 0;
	MrpTabTbl::Rec rec;
	PPIDArray sub_list;
	THROW(r = P_Tbl->GetParentID(tabID, &parent_id));
	if(r < 0)
		parent_id = tabID;
	THROW(Search(parent_id, &rec) > 0);
	THROW(r = P_Tbl->GetSubList(parent_id, &sub_list));
	if(r > 0) {
		pTree->Init(rec.LinkObjType, rec.LinkObjID, rec.Name);
		pTree->SetBaseID(parent_id);
		for(uint i = 0; i < sub_list.getCount(); i++)
			if(Search(sub_list.at(i), &rec) > 0)
				pTree->AddLeaf(&rec);
	}
	else
		pTree->AddLeaf(&rec);
	CATCHZOK
	return ok;
}

int PPObjMrpTab::AddIndep(MrpTabPacket * pMrpPack, PPID tabID, PPID goodsID, double req, double price, int ignoreRest)
{
	const long lflags = ignoreRest ? (MRPLF_TERMINAL|MRPLF_IGNOREREST) : MRPLF_TERMINAL;
	return pMrpPack->AddLine__(tabID, goodsID, MRPSRCV_TOTAL, req, 0, price, lflags);
}

/*static*/int PPObjMrpTab::GetAvailGoodsRest(PPID goodsID, const MrpTabLeaf * pLeaf, LDATE afterDate, double * pRest)
{
	*pRest = 0;
	const  double ignore_epsilon = BillCore::GetQttyEpsilon();
	DateRange lot_period;
	lot_period.Set(afterDate, pLeaf->Dt);
	return BillObj->trfr->GetAvailableGoodsRest(goodsID, pLeaf->LocID, lot_period, ignore_epsilon, pRest);
}

int PPObjMrpTab::SetupRest(const MrpTabPacket * pPack, const MrpTabLeaf * pLeaf, long cflags, int use_ta)
{
	int    ok = 1, r;
	uint   i = 0;
	PPObjGoods goods_obj;
	MrpReqArray dest_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_Tbl->GetDestList(pLeaf->TabID, MRPSRCV_TOTAL, 0, &dest_list));
		for(i = 0; i < dest_list.getCount(); i++) {
			const MrpReqItem & r_dest_item = dest_list.at(i);
			const  PPID goods_id = r_dest_item.GoodsID;
			double rest = 0.0;
			double dfct = 0.0;
			if(goods_obj.CheckFlag(goods_id, GF_UNLIM)) {
				THROW(r = P_Tbl->SetRest(pLeaf->TabID, goods_id, 0, &dfct, 0));
			}
			else {
				if(!(cflags & cfIgnoreRest) && !(r_dest_item.Flags & MRPLF_IGNOREREST)) {
					if(pPack) {
						THROW(pPack->GetAvailGoodsRest(goods_id, pLeaf, &rest));
					}
					else {
						THROW(PPObjMrpTab::GetAvailGoodsRest(goods_id, pLeaf, ZERODATE, &rest));
					}
				}
				GoodsRestVal grv(0, rest);
				THROW(r = P_Tbl->SetRest(pLeaf->TabID, goods_id, &grv, &dfct, 0));
			}
			if(r == 2) {
				//
				// Если обнаружился дефицит и позиция goods_id является терминальной
				//
				RAssocArray alt_goods_list;
				THROW(goods_obj.GetSubstList(goods_id, 0, alt_goods_list));
				for(uint j = 0; dfct > 0.0 && j < alt_goods_list.getCount(); j++) {
					PPID   alt_goods_id = alt_goods_list.at(j).Key;
					double ratio = alt_goods_list.at(j).Val;
					rest = 0.0;
					if(!(cflags & cfIgnoreRest)) {
						if(pPack) {
							THROW(pPack->GetAvailGoodsRest(alt_goods_id, pLeaf, &rest));
						}
						else {
							THROW(PPObjMrpTab::GetAvailGoodsRest(alt_goods_id, pLeaf, ZERODATE, &rest));
						}
					}
					if(rest > 0.0) {
						GoodsRestVal grv(0, rest);
						THROW(r = P_Tbl->SetSubstRest(pLeaf->TabID, goods_id, alt_goods_id, &grv, ratio, &dfct, 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int MrpTabPacket::GetAvailGoodsRest(PPID goodsID, const MrpTabLeaf * pLeaf, double * pRest) const
{
	int    ok = 1;
	double rest = 0.0;
	LDATE  after_date = ZERODATE;
	for(uint i = 0; i < getCount(); i++) {
		const MrpTabLeaf & r_leaf = at(i);
		if(r_leaf.TabID != pLeaf->TabID && r_leaf.LocID == pLeaf->LocID && r_leaf.Dt < pLeaf->Dt && r_leaf.Dt > after_date) {
			uint   pos = 0;
			CMrpTab::Row row;
			if(Cache.Search(r_leaf.TabID, goodsID, MRPSRCV_TOTAL, &pos, &row) && row.Flags & MRPLF_REST) {
				if(row.SrcReq > row.DestReq)
					rest = row.SrcReq - row.DestReq;
				else
					rest = 0.0;
				after_date = r_leaf.Dt;
			}
		}
	}
	{
		const  double ignore_epsilon = BillCore::GetQttyEpsilon();
		double avl_rest = 0.0;
		DateRange lot_period;
		lot_period.Set(after_date ? plusdate(after_date, 1) : ZERODATE, pLeaf->Dt);
		THROW(BillObj->trfr->GetAvailableGoodsRest(goodsID, pLeaf->LocID, lot_period, ignore_epsilon, &avl_rest));
		rest += avl_rest;
	}
	CATCHZOK
	ASSIGN_PTR(pRest, rest);
	return ok;
}

int MrpTabPacket::GetCTerminalList(PPID tabID, CMrpTab & rList) const
{
	int    ok = 1;
	rList.clear();
	for(uint i = 0; i < Cache.getCount(); i++) {
		const CMrpTab::Row & r_row = Cache.at(i);
		if(r_row.TabID == tabID && r_row.Flags & MRPLF_TERMINAL) {
			THROW_SL(rList.insert(&r_row));
		}
	}
	CATCHZOK
	return ok;
}

int MrpTabPacket::ProcessReq(const MrpTabLeaf * pLeaf, const MrpReqItem & rReq, double * pExtReq, int dep, long cflags)
{
	int    ok = 1;
	double rest = 0.0, ext_req = 0.0;
	//double price = dep ? 0 : rReq.Price;
	uint   pos = 0;
	CMrpTab::Row row;
	int    rest_takenin = 0;
	Cache.Sort(); // @v8.4.8
	if(Cache.Search(pLeaf->TabID, rReq.GoodsID, MRPSRCV_TOTAL, &pos, &row) && row.Flags & MRPLF_REST) {
		rest = row.SrcReq;
		rest_takenin = 1;
	}
	else {
		if(!(cflags & PPObjMrpTab::cfIgnoreRest) && !(rReq.Flags & MRPLF_IGNOREREST)) {
			THROW(GetAvailGoodsRest(rReq.GoodsID, pLeaf, &rest));
		}
	}
	//
	// Рассчитываем требуемое количество сверх остатка с учетом всех зарегистрированных
	// до сих пор требований
	//
	// Для независимых требований (dep == 0) rReq.Req эквивалентно row.DestReq.
	// По-этому, для этого случая расчет дополнительного количества несколько иной.
	//
	if(dep)
		ext_req = (row.DestReq > rest) ? rReq.Req : (row.DestReq + rReq.Req) - rest;
	else if(rReq.Req > rest)
		ext_req = rReq.Req - rest;
	//
	// Учитываем все запрошенное количество и остаток (если не учтен)
	// Для независимого требования (dep == 0) не следует увеличивать количество (оно уже учтено)
	//
	THROW(AddLine__(pLeaf->TabID, rReq.GoodsID, MRPSRCV_TOTAL, dep ? rReq.Req : 0, rest_takenin ? 0 : rest, /*price*/0, 0));
	if(!rest_takenin)
		THROW(Cache.SetFlag(pLeaf->TabID, rReq.GoodsID, MRPSRCV_TOTAL, MRPLF_REST, 1));
	CATCHZOK
	ASSIGN_PTR(pExtReq, ext_req);
	return ok;
}

int PPObjMrpTab::Helper_ExpandReq(MrpTabPacket * pPack, const MrpTabLeaf * pLeaf, const MrpReqItem & rReq, int dep, long cflags, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	int    r, terminal = 1;
	const  PPID goods_id = rReq.GoodsID;
	double ext_req = 0.0;
	THROW(pPack->ProcessReq(pLeaf, rReq, &ext_req, dep, cflags));
	//
	// Если требуемое количество меньше или равно нулю, то структуру не разворачиваем (дефицита нет)
	//
	if(ext_req > 0.0) {
		PPObjGoods goods_obj;
		PPGoodsStruc gs;
		THROW(r = goods_obj.LoadGoodsStruc(PPGoodsStruc::Ident(goods_id, GSF_COMPL, GSF_PARTITIAL, pLeaf->Dt), &gs));
		if(r > 0) {
			PPGoodsStrucItem gs_item;
			int    r2 = 0;
			uint   sav_recur_pos = 0;
			double src_qtty = 0.0;
			if(pRecurTrace) {
				if(pRecurTrace->addUnique(gs.Rec.ID) < 0) {
					SString goods_name;
					CALLEXCEPT_PP_S(PPERR_MRPTABRECURGS, GetGoodsName(goods_id, goods_name));
				}
				else
					sav_recur_pos = pRecurTrace->getCount();
			}
			for(uint p = 0; (r2 = gs.EnumItemsExt(&p, &gs_item, goods_id, ext_req, &src_qtty)) > 0;) {
				Goods2Tbl::Rec item_goods_rec;
				if(goods_obj.Fetch(gs_item.GoodsID, &item_goods_rec) > 0) { // @v10.4.10 
					MrpReqItem reqi(gs_item.GoodsID, 0/*flags*/, src_qtty, 0.0);
					THROW(r = Helper_ExpandReq(pPack, pLeaf, reqi, 1, cflags, pRecurTrace)); // @recursion
					THROW(pPack->AddLine__(pLeaf->TabID, goods_id, gs_item.GoodsID, ext_req, src_qtty, 0, (r == 2) ? 0 : MRPLF_TERMINAL));
				}
				// @v10.4.10 {
				else {
					SString msg_buf;
					GetGoodsName(goods_id, msg_buf);
					msg_buf.Space();
					if(gs.Rec.Name[0])
						msg_buf.Cat(gs.Rec.Name).Space();
					msg_buf.CatEq("goods_id", gs_item.GoodsID);
					CALLEXCEPT_PP_S(PPERR_GSTRUCHASUNDEFGOODS, msg_buf);
				}
				// @v10.4.10 {
			}
			THROW(r2);
			terminal = 0;
			if(pRecurTrace) {
				if(sav_recur_pos && pRecurTrace->getCount() == sav_recur_pos)
					pRecurTrace->atFree(sav_recur_pos-1);
			}
			ok = 2;
		}
	}
	else if(pPack->IsTerminal(pLeaf->TabID, goods_id) == 0)
		terminal = 0;
	//
	// Добавляем строку требования, дифференцированного по признаку Зависимое/Независимое
	// Здесь в качестве требуемого количества используем rReq.Req (не req.Req) ибо это -
	// внешнее затребованное количество без поправки на остаток
	THROW(pPack->AddLine__(pLeaf->TabID, goods_id, dep ? MRPSRCV_DEP : MRPSRCV_INDEP, rReq.Req, 0, 0, terminal ? MRPLF_TERMINAL : 0));
	THROW(pPack->SetTerminal(pLeaf->TabID, goods_id, terminal));
	CATCHZOK
	return ok;
}

int PPObjMrpTab::ExpandReq(MrpTabPacket * pPack, const MrpTabLeaf * pLeaf, long cflags)
{
	int    ok = 1;
	PPIDArray recur_trace;
	MrpReqArray dest_list;
	pPack->GetDestList(pLeaf->TabID, MRPSRCV_TOTAL, /*1*/0, &dest_list);
	for(uint i = 0; ok && i < dest_list.getCount(); i++) {
		recur_trace.clear();
		if(!Helper_ExpandReq(pPack, pLeaf, dest_list.at(i), 0, cflags, &recur_trace))
			ok = 0;
	}
	return ok;
}
//
//
//
int PPObjMrpTab::FinishPacket(MrpTabPacket * pTree, long cflags, int use_ta)
{
	int    ok = -1;
	MrpTabTbl::Rec base_rec;
	MrpTabLeaf base_leaf(pTree->GetBaseID(), 0, ZERODATE);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_PP(Search(base_leaf.TabID, &base_rec) > 0, PPERR_BASEMRPNFOUND);
		THROW(P_Tbl->RemoveLines(base_leaf.TabID, 0));
		if(pTree->IsTree()) {
			SString msg_buf;
			PPLoadText(PPTXT_MRPTABFINISHING, msg_buf);
			uint   i, j;
			PPIDArray loc_list;
			MrpTabLeaf * p_item;
			pTree->Sort(); // Сортируем листья пакета в порядке {TabID, Dt, LocID}
			DateRange bounds;
			pTree->GetCommonParam(&loc_list, &bounds);
			base_leaf.Dt = bounds.upp;
			//
			// Учет остатков по позициям с независимым спросом
			//
			{
				const uint lc_ = loc_list.getCount();
				for(j = 0; j < lc_; j++) {
					const uint tc_ = pTree->getCount();
					for(i = 0; pTree->enumItems(&i, (void **)&p_item);) {
						if(p_item->LocID == loc_list.at(j)) {
							THROW(ExpandReq(pTree, p_item, cflags));
							THROW(P_Tbl->RemoveLines(p_item->TabID, 0));
						}
						PPWaitPercent(1 + i + (j * tc_), (lc_ * tc_), msg_buf);
					}
				}
			}
			THROW(pTree->Aggregate());
			THROW(pTree->Flash(P_Tbl, 0));
			{
				//
				// Инициализируем остатки по каждому складу в порядке увеличения даты
				//
				PPLoadText(PPTXT_MRPTABRESTINIT, msg_buf);
				pTree->SortCache();
				const uint lc_ = loc_list.getCount();
				for(j = 0; j < loc_list.getCount(); j++) {
					const uint tc_ = pTree->getCount();
					for(i = 0; pTree->enumItems(&i, (void **)&p_item);) {
						if(p_item->LocID == loc_list.at(j)) {
							THROW(SetupRest(pTree, p_item, cflags, 0));
						}
						PPWaitPercent(1 + i + (j * tc_), (lc_ * tc_), msg_buf);
					}
				}
				base_leaf.LocID = loc_list.getSingle();
				THROW(SetupRest(0, &base_leaf, cflags, 0));
			}
		}
		else {
			base_leaf.LocID = base_rec.LocID;
			base_leaf.Dt    = base_rec.Dt;
			//
			// Учет остатков по позициям с независимым спросом
			//
			THROW(ExpandReq(pTree, &base_leaf, cflags));
			THROW(pTree->Flash(P_Tbl, 0));
			THROW(SetupRest(0, &base_leaf, cflags, 0));
		}
		if(pTree->GetName()) {
			STRNSCPY(base_rec.Name, pTree->GetName());
			THROW(P_Tbl->Update(base_leaf.TabID, &base_rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjMrpTab::DestroyPacket(MrpTabPacket * pTree, int use_ta)
{
	PPID   id = pTree->GetBaseID();
	return id ? P_Tbl->Remove(id, use_ta) : -1;
}

int PPObjMrpTab::CreateTreeLeaf(MrpTabPacket * pTree, PPID locID, LDATE dt, PPID * pTabID)
{
	int    ok = 1;
	PPID   tab_id = 0;
	MrpTabTbl::Rec rec;
	pTree->CreateLeafRec(locID, dt, rec);
	THROW(P_Tbl->Create(&tab_id, &rec, 0));
	rec.ID = tab_id;
	THROW(pTree->AddLeaf(&rec));
	ASSIGN_PTR(pTabID, tab_id);
	CATCHZOK
	return ok;
}

int PPObjMrpTab::GetTabID(MrpTabPacket * pTree, PPID locID, LDATE dt, PPID * pTabID, int use_ta)
{
	int    ok = 1;
	PPID   tab_id = 0;
	MrpTabTbl::Rec rec;
	if(!pTree->GetTabID(locID, dt, pTabID)) {
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pTree->getCount() == 1 && !pTree->IsTree()) {
			MrpTabLeaf & leaf = pTree->at(0);
			THROW(Search(leaf.TabID, &rec) > 0);
			if(rec.LocID == 0 && !rec.Dt) {
				//
				// Если в единственном листе списка не инициализированы склад и дата,
				// то инициализируем их и оставляем лист единственным
				//
				rec.LocID = locID;
				rec.Dt = dt;
				THROW(P_Tbl->Update(leaf.TabID, &rec, 0));
				leaf.LocID = locID;
				leaf.Dt = dt;
				ASSIGN_PTR(pTabID, leaf.TabID);
			}
			else {
				//
				// Если в единственном листе списка склад и дата инициализированы,
				// то создаем корневой лист и прикрепляем к нему новый с требуемыми
				// значениями склада и даты, а также тот, который был до этого единственным.
				//
				uint   i;
				PPID   base_id = 0;
				pTree->CreateLeafRec(0, ZERODATE, rec);
				rec.ParentID = 0;
				THROW(P_Tbl->Create(&base_id, &rec, 0));
				for(i = 0; i < pTree->getCount(); i++) {
					THROW(Search(pTree->at(i).TabID, &rec) > 0);
					rec.ParentID = base_id;
					THROW(P_Tbl->Update(rec.ID, &rec, 0));
				}
				pTree->SetBaseID(base_id);
				THROW(CreateTreeLeaf(pTree, locID, dt, pTabID));
			}
		}
		else {
			THROW(CreateTreeLeaf(pTree, locID, dt, pTabID));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjMrpTab::GetDeficitList(const MrpTabPacket * pPack, PPID srcID, int terminal, int replacePassiveGoods, PUGL * pList)
{
	int    ok = -1, r;
	if(pPack->IsTree()) {
		MrpTabLeaf * p_item;
		for(uint i = 0; pPack->enumItems(&i, (void **)&p_item);) {
			THROW(r = P_Tbl->GetDeficitList_(p_item->TabID, srcID, terminal, replacePassiveGoods, pList));
			if(r > 0)
				ok = 1;
		}
	}
	else {
		THROW(r = P_Tbl->GetDeficitList_(pPack->GetBaseID(), srcID, terminal, replacePassiveGoods, pList));
		if(r > 0)
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjMrpTab::CreateByGoods(PPID * pID, const char * pName, PPID goodsID, PPID locID, LDATE dt, int use_ta)
{
	MrpTabTbl::Rec rec;
	STRNSCPY(rec.Name, pName);
	rec.LinkObjType = PPOBJ_GOODS;
	rec.LinkObjID = goodsID;
	rec.LocID = locID;
	rec.Dt = dt;
	return P_Tbl->Create(pID, &rec, use_ta);
}

int PPObjMrpTab::CreateByBill(PPID * pID, const char * pName, PPID billID, int use_ta)
{
	int    ok = 1;
	PPBillPacket pack;
	BillTbl::Rec bill_rec;
	MrpTabTbl::Rec rec;
	THROW(BillObj->Search(billID, &bill_rec) > 0);
	STRNSCPY(rec.Name, NZOR(pName, bill_rec.Code));
	rec.LinkObjType = PPOBJ_BILL;
	rec.LinkObjID   = billID;
	rec.LocID       = bill_rec.LocID;
	rec.Dt  = bill_rec.Dt;
	THROW(P_Tbl->Create(pID, &rec, use_ta));
	CATCHZOK
	return ok;
}

int PPObjMrpTab::CreateByDraftWrOff(PPID * pID, const char * pName, PPID dwoID, PPID locID, LDATE dt, int use_ta)
{
	int    ok = 1;
	PPObjDraftWrOff dwo_obj;
	PPDraftWrOff dwo_rec;
	MrpTabTbl::Rec rec, same_rec;
	THROW(dwo_obj.Search(dwoID, &dwo_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		STRNSCPY(rec.Name, NZOR(pName, dwo_rec.Name));
		rec.LinkObjType = PPOBJ_DRAFTWROFF;
		rec.LinkObjID   = dwoID;
		rec.LocID       = locID;
		rec.Dt  = dt;
		if(P_Tbl->SearchByLink(rec.LinkObjType, rec.LinkObjID, rec.LocID, rec.Dt, &same_rec) > 0)
			THROW(P_Tbl->Remove(same_rec.ID, 0));
		THROW(P_Tbl->Create(pID, &rec, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjMrpTab::ArrangePugl(PPID tabID, PUGL * pSrc, uint pos, PUGL * pDest)
{
	int    ok = 1;
	PUGI * p_pugi = (PUGI *)pSrc->at(pos);
	if(!pDest->SearchGoods(p_pugi->GoodsID, 0)) {
		PUGL   dep;
		PUGI * p_dep_pugi;
		P_Tbl->GetDependencyList(tabID, p_pugi->GoodsID, &dep);
		for(uint i = 0; dep.enumItems(&i, (void **)&p_dep_pugi);) {
			uint   j = 0;
			if(pSrc->SearchGoods(p_dep_pugi->GoodsID, &j))
				ArrangePugl(tabID, pSrc, j, pDest); // @recursion
		}
		pDest->insert(p_pugi);
	}
	return ok;
}

int PPObjMrpTab::CreateModif(const MrpTabLeaf * pLeaf, PPID mrpSrcID, PPID opID, const PUGL::SetLotManufTimeParam * pSlmt, PPIDArray * pBillList, PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	PUGL   pugl;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(P_Tbl->GetDeficitList_(pLeaf->TabID, mrpSrcID, -1, 0, &pugl) > 0) {
			GoodsReplacementArray gra;
			uint   j;
			SString goods_name;
			PUGL   arranged_pugl;
			PUGI * p_pugi;
			for(j = 0; j < pugl.getCount(); j++)
				THROW(ArrangePugl(pLeaf->TabID, &pugl, j, &arranged_pugl));
			THROW(P_Tbl->GetSubst(pLeaf->TabID, &gra));
			for(j = 0; arranged_pugl.enumItems(&j, (void **)&p_pugi);) {
				int    r = 0;
				PPID   bill_id = 0;
				PUGL   single;
				single.Dt = pLeaf->Dt;
				single.LocID = pLeaf->LocID;
				RVALUEPTR(single.Slmt, pSlmt);
				THROW(single.Add(p_pugi, pLeaf->Dt));
				THROW(r = BillObj->CreateModifByPUGL(opID, &bill_id, &single, 0, &gra));
				if(pBillList && bill_id)
					THROW(pBillList->addUnique(bill_id));
				if(pLogger)
					if(r > 0)
						pLogger->LogAcceptMsg(PPOBJ_BILL, bill_id, 0);
					else {
						pLogger->LogString(PPTXT_MRPMODIFCREATINGFAILED, GetGoodsName(p_pugi->GoodsID, goods_name));
						single.Log(pLogger);
					}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjMrpTab::DoMaintain(LDATE toDt)
{
	int    ok = 1;
	long   total = 0;
	SString msg;
	IterCounter counter;
	MrpTabTbl::Key2 k;
	k.Dt = toDt;
	k.LocID = MAXLONG;
	{
		SString file_name;
		SString fmt_buf;
		PPLoadText(PPTXT_DBMAINTAIN, fmt_buf);
		SFsPath ps(P_Tbl->GetName());
		ps.Merge(SFsPath::fNam|SFsPath::fExt, file_name);
		msg.Printf(fmt_buf, file_name.cptr());
	}
	k.Dt = toDt;
	k.LocID = MAXLONG;
	if(P_Tbl->search(2, &k, spLe) && k.Dt <= toDt && k.Dt != ZERODATE) {
		total = P_Tbl->data.ParentID ? total + 2 : total + 1;
		while(P_Tbl->search(2, &k, spPrev) && k.Dt <= toDt && k.Dt != ZERODATE)
			total = P_Tbl->data.ParentID ? total + 2 : total + 1;
	}
	counter.Init(total);
	k.Dt = toDt;
	k.LocID = MAXLONG;
	while(P_Tbl->search(2, &k, spLe) && k.Dt <= toDt && k.Dt != ZERODATE) {
		PPID   id = NZOR(P_Tbl->data.ParentID, P_Tbl->data.ID);
		THROW(P_Tbl->Remove(id, 1));
		counter.Increment();
		PPWaitPercent(counter, msg);
	}
	CATCHZOK
	return ok;
}

