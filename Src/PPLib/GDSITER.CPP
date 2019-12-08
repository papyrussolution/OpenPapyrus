// GDSITER.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
// GoodsIterator
//
GoodsIterator::Ext & GoodsIterator::Ext::Z()
{
	GoodsID = 0;
	Price = 0.0;
	Rest = 0.0;
	PriceDtm.Z();
	SfList.Z();
	return *this;
}

// static
int FASTCALL GoodsIterator::GetListByGroup(PPID grpID, PPIDArray * pList)
{
	GoodsIterator iter(grpID, 0);
	return (!iter.P_List || pList->copy(*iter.P_List)) ? 1 : PPSetErrorSLib();
}

// static
int FASTCALL GoodsIterator::GetListByFilt(const GoodsFilt * pFilt, PPIDArray * pList, PPIDArray * pDupDynGrpList)
{
	int    ok = -1;
	if(pFilt && pList) {
		GoodsIterator giter(pFilt, 0, pDupDynGrpList);
		if(pDupDynGrpList == 0 && giter.P_IterQuery == 0 && giter.P_List) {
			pList->copy(*giter.P_List);
		}
		else {
			PPIDArray tmp_ary;
			Goods2Tbl::Rec grec;
			while(giter.Next(&grec) > 0)
				THROW(tmp_ary.add(grec.ID));
			pList->copy(tmp_ary);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

//static
int FASTCALL GoodsIterator::GetListByFilt(const GoodsFilt * pFilt, StrAssocArray * pList, int byName)
{
	int    ok = -1;
	if(pFilt && pList) {
		GoodsIterator giter(pFilt, 0, 0);
		Goods2Tbl::Rec goods_rec;
		UintHashTable el;
		if(giter.P_IterQuery == 0 && giter.P_List) {
			PPObjGoods goods_obj;
			for(uint i = 0; i < giter.P_List->getCount(); i++) {
				const char * p_name = 0;
				const PPID goods_id = giter.P_List->get(i);
				if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
					if(byName || goods_rec.Flags & GF_ABBREQNAME)
						p_name = goods_rec.Name;
					else if(goods_obj.Search(goods_id, &goods_rec) > 0)
						p_name = goods_rec.Abbr;
					if(p_name) {
						if(!el.Has((uint)goods_id)) {
							THROW_SL(pList->AddFast(goods_id, goods_rec.ParentID, p_name));
							el.Add((uint)goods_id);
						}
					}
				}
			}
		}
		else {
			while(giter.Next(&goods_rec) > 0) {
				if(!el.Has((uint)goods_rec.ID)) {
					THROW_SL(pList->AddFast(goods_rec.ID, goods_rec.ParentID, (byName ? goods_rec.Name : goods_rec.Abbr)));
					el.Add((uint)goods_rec.ID);
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

void FASTCALL GoodsIterator::InitInstance(PPIDArray * pDupDynGrpList)
{
	P_DupDynGrpList = pDupDynGrpList;
	P_IterQuery = 0;
	P_List = 0;
	P_MtxList = 0;
	P_ActualHash = 0;
	P_UhttsPack = 0;
	SfClsID = 0;
}

SLAPI GoodsIterator::GoodsIterator(const GoodsFilt * f, int aOrder, PPIDArray * pDupDynGrpList /*=0*/)
{
	InitInstance(pDupDynGrpList);
	Init(f, aOrder);
}

SLAPI GoodsIterator::GoodsIterator(PPID aGrp, int aOrder, PPIDArray * pDupDynGrpList /*=0*/)
{
	InitInstance(pDupDynGrpList);
	Init(aGrp, aOrder);
}

SLAPI GoodsIterator::GoodsIterator(int aOrder, PPIDArray * pDupDynGrpList /*=0*/)
{
	InitInstance(pDupDynGrpList);
	Init(static_cast<const GoodsFilt *>(0), aOrder);
}

SLAPI GoodsIterator::~GoodsIterator()
{
	delete P_List;
	delete P_MtxList;
	delete P_ActualHash;
	delete P_IterQuery;
	delete P_UhttsPack;
}

const IterCounter & SLAPI GoodsIterator::GetIterCounter() const { return Counter; }

int SLAPI GoodsIterator::Init(int aOrder)
{
	Filt.Init(1, 0);
	return Init((GoodsFilt*)0, aOrder);
}

int SLAPI GoodsIterator::FilterList(PPIDArray * pSrcList, PPIDArray * pDestList, long implFlags)
{
	PROFILE_START
	if(pSrcList->getCount()) {
		pSrcList->sort();
		CALLPTRMEMB(pDestList, sort());
		for(uint j = 0; j < pSrcList->getCount();) {
			PROFILE_START
			const PPID id = pSrcList->at(j++);
			int    suitable = 1;
			if(!id)
				suitable = 0;
			else if(Filt.Flags & GoodsFilt::fOutOfMatrix && P_MtxList && P_MtxList->bsearch(id))
				suitable = 0;
			else if(!GObj.CheckForFilt(&Filt, id, implFlags))
				suitable = 0;
			if(suitable) {
				if(pDestList && !pDestList->bsearch(id))
					pDestList->ordInsert(id, 0);
			}
			else if(pDestList == 0)
				pSrcList->atFree(--j);
			PROFILE_END
		}
	}
	PROFILE_END
	return 1;
}

int SLAPI GoodsIterator::Helper_MakeListByGroup(const PPIDArray * pGrpList, PPID grpID, PPIDArray * pList, int doIntersect)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPIDArray grp_list, goods_list;
	if(pGrpList) {
		THROW(grp_list.add(pGrpList));
	}
	if(grpID) {
		THROW(grp_list.add(grpID));
	}
	grp_list.sortAndUndup();
	uint   i = grp_list.getCount();
	if(i) {
		PPIDArray id_list;
		SString fmt_buf, msg_buf;
		if(CS_SERVER)
			PPLoadText(PPTXT_SELECTINGGOODSBYGROUP, fmt_buf);
		do {
			id_list.clear();
			const PPID grp_id = grp_list.at(--i);
			Goods2Tbl::Rec ggrec;
			if(GObj.Fetch(grp_id, &ggrec) > 0) {
				if(ggrec.Flags & (GF_ALTGROUP|GF_GENERIC)) {
					if(ggrec.Flags & GF_GENERIC) {
						THROW(GObj.GetGenericList(grp_id, &id_list));
					}
					else if(ggrec.Flags & GF_DYNAMIC) {
						GoodsFilt grp_filt;
						if(GObj.P_Tbl->GetGroupFilt(grp_id, &grp_filt) > 0) {
							int    inited = 0;
							if(!P_DupDynGrpList) {
								THROW_MEM(P_DupDynGrpList = new PPIDArray);
								inited = 1;
							}
							if(P_DupDynGrpList->addUnique(grp_id) > 0) {
								GoodsIterator::GetListByFilt(&grp_filt, &id_list, P_DupDynGrpList); // @recursion
							}
							if(inited)
								ZDELETE(P_DupDynGrpList);
						}
					}
					else {
						p_ref->Assc.GetListByPrmr(PPASS_ALTGOODSGRP, grp_id, &id_list);
						if(CS_SERVER) {
							long _mc = SVectorBase::GetCount(pList) + id_list.getCount();
							(msg_buf = fmt_buf).Space().Cat(_mc);
							PPWaitMsg(msg_buf);
						}
					}
					THROW(FilterList(&id_list, &goods_list, GOCFIF_GROUP|GOCFIF_BCLEN|GOCFIF_ARCODE|GOCFIF_SYSJ|GOCFIF_TAG));
				}
				else {
					const int is_excl_alt_fold = BIN(ggrec.Flags & GF_FOLDER && ggrec.Flags & GF_EXCLALTFOLD);
					GoodsCore * p_t = GObj.P_Tbl;
					PPIDArray term_grp_list, temp_alt_list;
					THROW(p_t->GetGroupTerminalList(grp_id, &term_grp_list, 0));
					term_grp_list.sort();
					for(uint j = 0; j < term_grp_list.getCount(); j++) {
						const PPID int_grp_id = term_grp_list.get(j);
						temp_alt_list.clear();
						if(is_excl_alt_fold) {
							if(GObj.CheckFlag(int_grp_id, GF_DYNAMICALTGRP) > 0) {
								THROW(Helper_MakeListByGroup(0, int_grp_id, &temp_alt_list, 0)); // @recursion
								THROW(id_list.add(&temp_alt_list));
							}
							else {
								THROW(p_ref->Assc.GetListByPrmr(PPASS_ALTGOODSGRP, int_grp_id, &temp_alt_list));
								if(CS_SERVER) {
									long _mc = SVectorBase::GetCount(pList) + temp_alt_list.getCount();
									(msg_buf = fmt_buf).Space().Cat(_mc);
									PPWaitMsg(msg_buf);
								}
								THROW(FilterList(&temp_alt_list, &id_list, GOCFIF_GROUP|GOCFIF_BCLEN|GOCFIF_ARCODE|GOCFIF_SYSJ|GOCFIF_TAG));
							}
						}
						else if(GObj.CheckFlag(int_grp_id, GF_ALTGROUP) > 0) {
							THROW(Helper_MakeListByGroup(0, int_grp_id, &temp_alt_list, 0)); // @recursion
							THROW(id_list.add(&temp_alt_list));
						}
						else {
							union {
								Goods2Tbl::Key1 k1;
							} k; // #1 || #3
							DBQ * dbq = 0;
							IterIdx = 1;
							BExtQuery q(p_t, IterIdx);
							q.select(p_t->ID, p_t->ParentID, p_t->UnitID, p_t->PhUnitID, p_t->ManufID,
								p_t->GoodsTypeID, p_t->GdsClsID, p_t->BrandID, p_t->Flags, p_t->TaxGrpID, 0L);
							if(Filt.SrchStr_.NotEmpty())
								q.addField(p_t->Name);
							dbq = & (p_t->ParentID == int_grp_id);
							dbq = ppcheckfiltid(dbq, p_t->ManufID,  Filt.ManufID);
							dbq = ppcheckfiltid(dbq, p_t->BrandID,  Filt.GetResultBrandList().GetSingle());
							dbq = ppcheckfiltid(dbq, p_t->GdsClsID, Filt.Ep.GdsClsID);
							dbq = ppcheckfiltid(dbq, p_t->UnitID,   Filt.UnitID);
							dbq = ppcheckfiltid(dbq, p_t->PhUnitID, Filt.PhUnitID);
							if(Filt.Flags & GoodsFilt::fUndefType)
								dbq = & (*dbq && p_t->GoodsTypeID == 0L);
							else
								dbq = ppcheckfiltid(dbq, p_t->GoodsTypeID, Filt.GoodsTypeID);
							q.where(*dbq);
							MEMSZERO(k);
							k.k1.Kind = PPGDSK_GOODS;
							k.k1.ParentID = int_grp_id;
							for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
								const PPID id = p_t->data.ID;
								if(!(Filt.Flags & GoodsFilt::fOutOfMatrix) || !P_MtxList || !P_MtxList->bsearch(id)) {
									if(GObj.CheckForFilt(&Filt, 0, GOCFIF_GROUP|GOCFIF_BCLEN|GOCFIF_ARCODE|GOCFIF_SYSJ|GOCFIF_TAG)) {
										THROW(id_list.add(id));
										if(CS_SERVER) {
											long _mc = SVectorBase::GetCount(pList) + id_list.getCount();
											if((_mc % 100) == 0) {
												(msg_buf = fmt_buf).Space().Cat(_mc);
												PPWaitMsg(msg_buf);
											}
										}
									}
								}
							}
						}
					}
					goods_list.add(&id_list);
				}
			}
		} while(i);
		ok = 1;
	}
	else if(doIntersect) {
		THROW(FilterList(pList, &goods_list, GOCFIF_GROUP|GOCFIF_BCLEN|GOCFIF_ARCODE|GOCFIF_SYSJ|GOCFIF_TAG));
		ok = 1;
	}
	goods_list.sortAndUndup();
	if(doIntersect) {
		pList->sort();
		goods_list.intersect(pList, 1);
	}
	THROW_SL(pList->copy(goods_list));
	CATCHZOK
	return ok;
}

int SLAPI GoodsIterator::Init(PPID grpID, int aOrder)
{
	GoodsFilt temp_filt;
	temp_filt.GrpID = grpID;
	return Init(&temp_filt, aOrder);
}

static int IntersectByTempList(PPIDArray * pMainList, PPIDArray * pTempList, int * pDoIntersect)
{
	int    ok = 1;
	pTempList->sort();
	if(*pDoIntersect) {
		pMainList->intersect(pTempList, 1);
	}
	else
		THROW(pMainList->copy(*pTempList));
	*pDoIntersect = 1;
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(GoodsIdListByName, p1, p2)
{
	int    cm = 0;
	PPObjGoods * p_goods_obj = static_cast<PPObjGoods *>(pExtraData);
	const PPID id1 = *static_cast<const PPID *>(p1);
	const PPID id2 = *static_cast<const PPID *>(p2);
	if(p_goods_obj) {
		Goods2Tbl::Rec rec1, rec2;
		int    r1 = p_goods_obj->Fetch(id1, &rec1);
		int    r2 = p_goods_obj->Fetch(id2, &rec2);
		if(r1 > 0) {
			if(r2 > 0)
				cm = stricmp866(rec1.Name, rec2.Name);
			else
				cm = 1;
		}
		else {
			if(r2 > 0)
				cm = -1;
			else
				cm = 0;
		}
	}
	return cm;
}

int SLAPI GoodsIterator::Init(const GoodsFilt * pFilt, int aOrder)
{
	int    ok = 1, r;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	Goods2Tbl::Rec goods_rec;
	PPIDArray len_list, temp_list;
	PPIDArray * p_excl_list = 0;
	ZDELETE(P_List);
	ZDELETE(P_MtxList);
	ZDELETE(P_ActualHash);
	BExtQuery::ZDelete(&P_IterQuery);
	SfClsID = 0;
	ExtList.clear();
	ZDELETE(P_UhttsPack);
	ExtSfTitleList.Z();
	Order = oneof2(aOrder, ordByName, ordByAbbr) ? aOrder : 0;
	if(pFilt) {
		Filt = *pFilt;
		Filt.Setup();
	}
	else
		Filt.Init(1, 0);
	const long mf = (GoodsFilt::fRestrictByMatrix | GoodsFilt::fOutOfMatrix);
	if(Filt.Flags & mf) {
		ObjIdListFilt mtx_loc_list;
		if(Filt.MtxLocID)
			mtx_loc_list.Add(Filt.MtxLocID);
		else
			mtx_loc_list = Filt.LocList;
		if((Filt.Flags & mf) == mf)
			Filt.Flags &= ~GoodsFilt::fOutOfMatrix;
		THROW_MEM(P_MtxList = new PPIDArray);
		THROW(r = GObj.P_Tbl->GetMatrix(mtx_loc_list, 0, P_MtxList));
		if(r > 0)
			P_MtxList->sort();
		else
			ZDELETE(P_MtxList);
	}
	if(Filt.IsEmpty() || Filt.Flags & GoodsFilt::fNegation) {
		THROW(CreateIterQuery());
	}
	else {
		int    do_intersect = 0;
		THROW_MEM(P_List = new PPIDArray);
		if(Filt.RestrictQuotKindID) {
			temp_list.clear();

			PPID   uhttsloc_id = 0;
			int    do_calc_uhtt_rest = 0;
			PPOprKind op_rec;
			PPViewQuot qview;
			QuotFilt qfilt;
			qfilt.QuotKindID = Filt.RestrictQuotKindID;

			if(Filt.UhttStoreID) {
				PPObjUhttStore uhtts_obj;
				if(P_UhttsPack)
					P_UhttsPack->destroy();
				else
					THROW_MEM(P_UhttsPack = new PPUhttStorePacket);
				if(uhtts_obj.GetPacket(Filt.UhttStoreID, P_UhttsPack) > 0) {
					qfilt.SellerID = P_UhttsPack->Rec.PersonID;
					qfilt.LocID = uhttsloc_id = P_UhttsPack->Rec.LocID;
					if(uhttsloc_id && P_UhttsPack->Rec.Flags & (PPUhttStore::fShowRest|PPUhttStore::fDontShowZeroRestItems))
						do_calc_uhtt_rest = 1;
				}
				else
					ZDELETE(P_UhttsPack);
			}
			const PPID rest_op_id  = (do_calc_uhtt_rest && GetOpBySymb("GOODSREST", &op_rec) > 0) ? op_rec.ID : 0;
			const PPID order_op_id = (do_calc_uhtt_rest && GetOpBySymb("DRAFTORDER", &op_rec) > 0) ? op_rec.ID : 0;
			if(!qfilt.LocID)
				qfilt.LocList = Filt.LocList;
			qfilt.Flags |= QuotFilt::fActualOnly;
			qfilt.Flags |= QuotFilt::fListOnly;
			THROW(qview.Init_(&qfilt));
			const PPQuotItemArray * p_qlist = qview.GetQList();
			if(p_qlist) {
				for(uint i = 0; i < p_qlist->getCount(); i++) {
					const PPQuotItem_ & r_item = p_qlist->at(i);
					int   skip = 0;
					double rest = 0.0;
					if(rest_op_id) {
						p_bobj->CalcDraftTransitRest(rest_op_id, order_op_id, r_item.GoodsID, uhttsloc_id, 0 /* flags */, &rest, 0);
						if(rest <= 0.0 && (P_UhttsPack->Rec.Flags & PPUhttStore::fDontShowZeroRestItems))
							skip = 1;
					}
					if(!skip) {
						temp_list.add(r_item.GoodsID);
						if(uhttsloc_id && r_item.LocID == uhttsloc_id) {
							InnerExt ext;
							MEMSZERO(ext);
							ext.GoodsID = r_item.GoodsID;
							ext.CurID = r_item.CurID;
							ext.Price = r_item.Val;
							ext.Rest = rest;
							ext.PriceDtm = r_item.Dtm;
							THROW_SL(ExtList.insert(&ext));
						}
					}
				}
			}
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		if(P_MtxList && Filt.Flags & GoodsFilt::fRestrictByMatrix) {
			THROW(IntersectByTempList(P_List, P_MtxList, &do_intersect));
		}
		if(Filt.SupplID || !Filt.LotPeriod.IsZero()) {
			GoodsByTransferFilt gbt_filt(&Filt);
			temp_list.clear();
			THROW(p_bobj->trfr->GetGoodsIdList(gbt_filt, temp_list));
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		if(Filt.GetBarcodeLenList(len_list) > 0) {
			// @v8.1.0 (теперь это делает GetListByBarcodeLen) temp_list.clear();
			THROW(GObj.P_Tbl->GetListByBarcodeLen(&len_list, temp_list));
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		if(Filt.IsRestrictedByAr()) {
			temp_list.clear();
			THROW(GObj.P_Tbl->GetListByAr(Filt.CodeArID, &temp_list));
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		if(Filt.Flags & Filt.fWithStrucOnly || Filt.GoodsStrucID) {
			temp_list.clear();
			Goods2Tbl::Key5 k5;
			Goods2Tbl * t = GObj.P_Tbl;
			MEMSZERO(k5);
			BExtQuery q(t, 5);
			if(Filt.GoodsStrucID) {
				k5.StrucID = Filt.GoodsStrucID;
				q.select(t->ID, 0L).where(t->Kind == PPGDSK_GOODS && t->StrucID == Filt.GoodsStrucID);
			}
			else {
				q.select(t->ID, 0L).where(t->Kind == PPGDSK_GOODS && t->StrucID > 0L);
			}
			for(q.initIteration(0, &k5, spGe); q.nextIteration() > 0;)
				THROW_SL(temp_list.add(t->data.ID));
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		{
			const ObjIdListFilt & r_brand_list = Filt.GetResultBrandList();
			if(r_brand_list.GetCount()) {
				temp_list.clear();
				for(uint i = 0; i < r_brand_list.GetCount(); i++) {
					const PPID brand_id = r_brand_list.Get().get(i);
					Goods2Tbl::Key3 k3;
					Goods2Tbl * t = GObj.P_Tbl;
					MEMSZERO(k3);
					k3.Kind = PPGDSK_GOODS;
					k3.BrandID = brand_id;
					if(t->search(3, &k3, spEq)) {
						BExtQuery q(t, 3);
						q.select(t->ID, 0L).where(t->Kind == PPGDSK_GOODS && t->BrandID == brand_id);
						for(q.initIteration(0, &k3, spEq); q.nextIteration() > 0;)
							THROW_SL(temp_list.add(t->data.ID));
					}
				}
				THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
			}
		}
		if(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			temp_list.clear();
			THROW(p_sj->GetObjListByEventPeriod(PPOBJ_GOODS, Filt.P_SjF->UserID, &Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, temp_list));
			THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
		}
		if(Filt.P_TagF && !Filt.P_TagF->IsEmpty()) {
			UintHashTable _list, _excl_list;
			const int tfret = TagObj.GetObjListByFilt(PPOBJ_GOODS, Filt.P_TagF, _list, _excl_list);
			if(tfret == 100) {
				if(_excl_list.GetCount()) {
					THROW_MEM(SETIFZ(p_excl_list, new PPIDArray));
					for(ulong _v = 0; _excl_list.Enum(&_v);)
						p_excl_list->addUnique((PPID)_v);
				}
			}
			else if(tfret > 0) {
				if(_excl_list.GetCount()) {
					THROW_MEM(SETIFZ(p_excl_list, new PPIDArray));
					for(ulong _v = 0; _excl_list.Enum(&_v);)
						p_excl_list->addUnique((PPID)_v);
				}
				if(do_intersect) {
					P_List->intersect(&_list);
				}
				else {
					for(ulong _v = 0; _list.Enum(&_v);) {
						P_List->addUnique((PPID)_v);
					}
				}
				do_intersect = 1;
			}
		}
		{
			Filt.GetExtssData(Filt.extssBarcodeText, temp_buf.Z());
			if(temp_buf.NotEmptyS()) {
				StringSet bc_ss(',', temp_buf);
				BarcodeArray bc_list;
				temp_list.clear();
				for(uint bc_ss_pos = 0; bc_ss.get(&bc_ss_pos, temp_buf);) {
					bc_list.clear();
					if(GObj.P_Tbl->SearchBarcodeSubstrExt(temp_buf.Strip(), &bc_list) > 0) {
						for(uint i = 0; i < bc_list.getCount(); i++) {
							temp_list.add(bc_list.at(i).GoodsID);
						}
					}
				}
				temp_list.sortAndUndup();
				THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
			}
		}
		THROW(r = Helper_MakeListByGroup(&Filt.GrpIDList.Get(), Filt.GrpID, P_List, do_intersect));
		if(r > 0) {
			if(p_excl_list) {
				for(uint i = 0; i < p_excl_list->getCount(); i++)
					P_List->freeByKey(p_excl_list->get(i), 0);
			}
			Counter.Init(P_List->getCount());
		}
		else {
			Filt.GetExtssData(Filt.extssNameText, temp_buf.Z());
			if(temp_buf.NotEmptyS()) {
				temp_list.clear();
				THROW(GObj.P_Tbl->GetListBySubstring(temp_buf, &temp_list, BIN(Filt.Flags & GoodsFilt::fHidePassive)));
				THROW(IntersectByTempList(P_List, &temp_list, &do_intersect));
				THROW(FilterList(P_List, 0, GOCFIF_GROUP|GOCFIF_BCLEN|GOCFIF_ARCODE|GOCFIF_SYSJ|GOCFIF_TAG));
				Counter.Init(P_List->getCount());
			}
			else {
				ZDELETE(P_List);
				THROW(CreateIterQuery());
			}
		}
	}
	if(Filt.Flags & GoodsFilt::fActualOnly) {
		THROW_MEM(P_ActualHash = new UintHashTable);
		UnlimTypeList.clear();
		{
			PPGoodsType2 gt_rec;
			for(SEnum en = PPRef->Enum(PPOBJ_GOODSTYPE, 0); en.Next(&gt_rec) > 0;) {
				if(gt_rec.Flags & (GTF_UNLIMITED|GTF_AUTOCOMPL)) {
					THROW_SL(UnlimTypeList.add(gt_rec.ID));
				}
			}
			UnlimTypeList.sort();
		}
		if(Filt.LocList.IsExists()) {
			UintHashTable temp_hash;
			for(uint i = 0; i < Filt.LocList.GetCount(); i++) {
				temp_hash.Clear();
				THROW(GObj.GetOpenedList(Filt.LocList.Get().at(i), P_List, temp_hash));
				THROW_SL(P_ActualHash->Add(temp_hash));
			}
		}
		else {
			THROW(GObj.GetOpenedList(0, P_List, *P_ActualHash));
		}
		if(P_List) {
			temp_list.clear();
			for(uint i = 0; i < P_List->getCount(); i++) {
				const PPID goods_id = P_List->get(i);
				if(GObj.Fetch(goods_id, &goods_rec) > 0 && CheckActual(goods_rec))
					temp_list.add(goods_id);
			}
			P_List->copy(temp_list);
			Counter.Init(P_List->getCount());
		}
	}
	if(P_List) {
		if(oneof2(Order, ordByName, ordByAbbr))
			P_List->SVector::sort(PTR_CMPFUNC(GoodsIdListByName), &GObj);
		else
			P_List->sort();
		if(P_UhttsPack && P_UhttsPack->Sd.GetCount()) {
			PPUhttStoreSelDescr::Entry sd_entry;
			temp_list.clear();
			uint   i;
			for(i = 0; i < P_UhttsPack->Sd.GetCount(); i++) {
				if(P_UhttsPack->Sd.GetEntry(i, sd_entry) && sd_entry.Attr == PPUhttStoreSelDescr::attrClass)
					temp_list.add(sd_entry.ClsID);
			}
			temp_list.sortAndUndup();
			if(temp_list.getCount()) {
				PPIDArray spot_cls_list;
				for(i = 0; i < P_List->getCount(); i++) {
					const PPID goods_id = P_List->get(i);
					if(GObj.Fetch(goods_id, &goods_rec) > 0 && goods_rec.GdsClsID)
						spot_cls_list.addUnique(goods_rec.GdsClsID);
				}
				temp_list.intersect(&spot_cls_list, 0);
				// @todo Если temp_list содержит более одного класса, то нужен какой-то механизм выбора
				// конкретного класса для определения набора селекторов.
				SfClsID = temp_list.getCount() ? temp_list.get(0) : 0;
			}
			THROW(P_UhttsPack->GetSelectorListInfo(SfClsID, ExtSfTitleList));
		}
		ExtList.sort(CMPF_LONG);
	}
	CATCHZOK
	delete p_excl_list;
	return ok;
}

/* @v9.4.12 int FASTCALL GoodsIterator::CheckActual(const Goods2Tbl::Rec & rRec) const
{
	int    yes = 1;
	if(P_ActualHash && !P_ActualHash->Has((uint)rRec.ID)) {
		if(!(rRec.Flags & (GF_GENERIC|GF_UNLIM|GF_AUTOCOMPL)) && !(rRec.GoodsTypeID && UnlimTypeList.bsearch(rRec.GoodsTypeID)))
			yes = 0;
		if(rRec.Flags & (GF_GENERIC|GF_UNLIM|GF_AUTOCOMPL))
			return 1;
		else if(rRec.GoodsTypeID && UnlimTypeList.bsearch(rRec.GoodsTypeID))
			return 1;
		else
			return 0;
	}
	else
		return 1;
	return yes;
} */

// @v9.4.12 modified
int FASTCALL GoodsIterator::CheckActual(const Goods2Tbl::Rec & rRec) const
{
	int    yes = 1;
	if(P_ActualHash && !P_ActualHash->Has((uint)rRec.ID)) {
		if(!(rRec.Flags & (GF_GENERIC|GF_UNLIM|GF_AUTOCOMPL)) && !(rRec.GoodsTypeID && UnlimTypeList.bsearch(rRec.GoodsTypeID)))
			yes = 0;
	}
	return yes;
}

int SLAPI GoodsIterator::GetSelectorListInfo(PPID * pClsID, StrAssocArray & rList) const
{
	int    ok = 1;
	if(P_List && P_UhttsPack && P_UhttsPack->Sd.GetCount()) {
		ASSIGN_PTR(pClsID, SfClsID);
		rList = ExtSfTitleList;
	}
	else {
		ASSIGN_PTR(pClsID, 0);
		rList.Z();
		ok = 0;
	}
	return ok;
}

int SLAPI GoodsIterator::GetSelectorListItem(long handler, PPUhttStoreSelDescr::Entry & rEntry) const
{
	return (P_UhttsPack && handler > 0 && handler <= (long)P_UhttsPack->Sd.GetCount()) ? P_UhttsPack->Sd.GetEntry(handler-1, rEntry) : 0;
}

int SLAPI GoodsIterator::Next(Goods2Tbl::Rec * pRec, GoodsIterator::Ext * pExt)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			Goods2Tbl::Rec rec;
			GObj.P_Tbl->copyBufTo(&rec);
			int    r = 1;
			{
				if(P_MtxList && ((Filt.Flags & GoodsFilt::fRestrictByMatrix && !P_MtxList->bsearch(rec.ID)) ||
					(Filt.Flags & GoodsFilt::fOutOfMatrix && P_MtxList->bsearch(rec.ID))))
					r = BIN(Filt.Flags & GoodsFilt::fNegation);
				else if(P_List && !P_List->bsearch(rec.ID))
					r = BIN(Filt.Flags & GoodsFilt::fNegation);
				else if(!CheckActual(rec))
					r = BIN(Filt.Flags & GoodsFilt::fNegation);
				else
					r = GObj.CheckForFilt(&Filt, 0);
			}
			if(r) {
				ASSIGN_PTR(pRec, rec);
				ok = 1;
			}
			Counter.Increment();
		}
	}
	else if(P_List) {
		PPObjGoodsClass gc_obj;
		for(; ok < 0 && Counter < P_List->getCount(); Counter.Increment()) {
			const PPID goods_id = P_List->at(Counter);
			if(GObj.Search(goods_id, pRec) > 0) {
				ok = 1;
				if(pExt && pRec) {
					uint elp = 0;
					if(ExtList.bsearch(&goods_id, &elp, CMPF_LONG)) {
						PPUhttStoreSelDescr::Entry sd_entry;
						SString ext_sd_text;
						long   ext_sd_id = 0;
						long   ext_sd_parent_id = 0;
						ObjTagItem tag_item;
						GoodsExtTbl::Rec goods_ext_rec;
						PPGdsClsPacket gc_pack;
						const InnerExt & r_ext = ExtList.at(elp);
						assert(r_ext.GoodsID == goods_id);
						pExt->Z();
						pExt->GoodsID = goods_id;
						pExt->CurID = r_ext.CurID;
						pExt->Price = r_ext.Price;
						pExt->Rest = r_ext.Rest;
						pExt->PriceDtm = r_ext.PriceDtm;
						if(SfClsID && gc_obj.Fetch(SfClsID, &gc_pack) > 0) {
							GObj.P_Tbl->GetExt(goods_id, &goods_ext_rec);
						}
						// @v10.6.4 else { MEMSZERO(goods_ext_rec); }
						for(uint i = 0; i < ExtSfTitleList.getCount(); i++) {
							StrAssocArray::Item item = ExtSfTitleList.at_WithoutParent(i);
							const uint sd_pos = (item.Id - 1);
							assert(sd_pos < P_UhttsPack->Sd.GetCount());
							ext_sd_text.Z();
							ext_sd_id = 0;
							ext_sd_parent_id = 0;
							if(P_UhttsPack->Sd.GetEntry(sd_pos, sd_entry)) {
								switch(sd_entry.Attr) {
									case PPUhttStoreSelDescr::attrGroup:
										ext_sd_id = pRec->ParentID;
										GetGoodsName(ext_sd_id, ext_sd_text);
										break;
									case PPUhttStoreSelDescr::attrBrand:
										ext_sd_id = pRec->BrandID;
										GetGoodsName(ext_sd_id, ext_sd_text);
										break;
									case PPUhttStoreSelDescr::attrName:
										ext_sd_id = pRec->ID;
										ext_sd_text = pRec->Name;
										break;
									case PPUhttStoreSelDescr::attrClass:
										if(SfClsID && pRec->GdsClsID == SfClsID && goods_ext_rec.GoodsID == goods_id) {
											if(oneof4(sd_entry.GcClsf, PPGdsCls::eKind, PPGdsCls::eGrade, PPGdsCls::eAdd, PPGdsCls::eAdd2)) {
												gc_pack.GetExtProp(&goods_ext_rec, sd_entry.GcClsf, &ext_sd_id, ext_sd_text);
											}
											else if(oneof4(sd_entry.GcClsf, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW)) {
												double val = 0.0;
												gc_pack.GetExtDim(&goods_ext_rec, sd_entry.GcClsf, &val);
												ext_sd_text.Z().Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
											}
										}
										break;
									case PPUhttStoreSelDescr::attrTag:
										if(sd_entry.TagID && TagObj.FetchTag(goods_id, sd_entry.TagID, &tag_item) > 0) {
											if(tag_item.TagDataType == OTTYP_ENUM) {
												tag_item.GetEnumData(&ext_sd_id, &ext_sd_parent_id, &ext_sd_text, 0);
											}
											else {
												tag_item.GetInt(&ext_sd_id);
												tag_item.GetStr(ext_sd_text);
											}
										}
										break;
								}
							}
							if(ext_sd_parent_id)
								pExt->SfList.AddFast(ext_sd_id, ext_sd_parent_id, ext_sd_text);
							else
								pExt->SfList.AddFast(ext_sd_id, ext_sd_text);
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI GoodsIterator::CreateIterQuery()
{
	int    ok = 1;
	union {
		Goods2Tbl::Key0 k0;
		Goods2Tbl::Key2 k2;
		Goods2Tbl::Key4 k4;
	} k;
	PPIDArray temp;
	Goods2Tbl * p_t = GObj.P_Tbl;
	DBQ * dbq = 0;
	IterIdx = (Order == ordByName) ? 2 : ((Order == ordByAbbr) ? 4 : 0);
	THROW_MEM(P_IterQuery = new BExtQuery(p_t, IterIdx, 16));
	P_IterQuery->selectAll();
	dbq = & (*dbq && p_t->Kind == PPGDSK_GOODS);
	if(!(Filt.Flags & GoodsFilt::fNegation)) {
		dbq = ppcheckfiltid(dbq, p_t->ManufID,  Filt.ManufID);
		dbq = ppcheckfiltid(dbq, p_t->BrandID,  Filt.GetResultBrandList().GetSingle());
		dbq = ppcheckfiltid(dbq, p_t->GdsClsID, Filt.Ep.GdsClsID);
		dbq = ppcheckfiltid(dbq, p_t->UnitID,   Filt.UnitID);
		dbq = ppcheckfiltid(dbq, p_t->PhUnitID, Filt.PhUnitID);
		if(Filt.Flags & GoodsFilt::fUndefType)
			dbq = & (*dbq && p_t->GoodsTypeID == 0L);
		else
			dbq = ppcheckfiltid(dbq, p_t->GoodsTypeID, Filt.GoodsTypeID);
	}
	P_IterQuery->where(*dbq);
	MEMSZERO(k);
	if(IterIdx == 2)
		k.k2.Kind = PPGDSK_GOODS;
	else if(IterIdx == 4)
		k.k4.Kind = PPGDSK_GOODS;
	P_IterQuery->initIteration(0, &k, spGe);
	PPInitIterCounter(Counter, GObj.P_Tbl);
	CATCHZOK
	return ok;
}
//
//
//
#define GRPCODESIZE 16

IMPL_CMPFUNC(_GroupListItem, i1, i2)
{
	const char * p1 = static_cast<const char *>(i1) + sizeof(PPID) + GRPCODESIZE;
	const char * p2 = static_cast<const char *>(i2) + sizeof(PPID) + GRPCODESIZE;
	return stricmp866(p1, p2);
}

IMPL_CMPFUNC(_GroupListItemCode, i1, i2)
{
	const char * p1 = static_cast<const char *>(i1) + sizeof(PPID);
	const char * p2 = static_cast<const char *>(i2) + sizeof(PPID);
	return strnicmp(p1, p2, GRPCODESIZE);
}

SLAPI GoodsGroupIterator::GoodsGroupIterator(PPID parentID, long flags/*int addZeroGroup, int sortByCode*/)
{
	Init(parentID, flags/*addZeroGroup, sortByCode*/);
}

int SLAPI GoodsCore::MakeFullName(PPID id, PPID parentID, SString & rBuf)
{
	int    ok = -1;
	Goods2Tbl::Rec grec;
	rBuf.Z();
	if(Fetch(id, &grec) > 0 && grec.Kind == PPGDSK_GROUP) {
		if(id == parentID || BelongToGroup(id, parentID) > 0) {
			PPID   parent_id = 0;
			char   name_buf[128];
			PPIDArray cycle_list;
			SStack name_stack(sizeof(name_buf));
			cycle_list.add(id);
			do {
				parent_id = grec.ParentID;
				if(cycle_list.lsearch(parent_id)) {
					SString ss("#CYCLE");
					CorrectCycleLink(id, 0, 1);
					STRNSCPY(name_buf, ss.Space().Cat(strip(grec.Name)));
					THROW_SL(name_stack.push(name_buf));
					break;
				}
				else {
					cycle_list.add(parent_id);
					STRNSCPY(name_buf, strip(grec.Name));
					THROW_SL(name_stack.push(name_buf));
				}
			} while(parent_id && Fetch(parent_id, &grec) > 0);
			while(name_stack.pop(name_buf))
				rBuf.CatDivIfNotEmpty('/', 1).Cat(name_buf);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI GoodsGroupIterator::Init(PPID parentID, long flags)
{
	ParentID = parentID;
	Flags    = flags;
	IterIndex = 0;
	List.freeAll();

	int    ok = 1;
	int    is_excl_alt_fold = 0;
	const  size_t hs = sizeof(PPID);
	const  size_t cs =(size_t)GRPCODESIZE;
	char   code[cs];
	SString grp_name;

	PPID   upper_parent_id = 0;
	Goods2Tbl::Key1 k1;
	Goods2Tbl::Rec  grec;
	MEMSZERO(k1);
	k1.Kind = PPGDSK_GROUP;
	if(ParentID && GObj.Fetch(ParentID, &grec) > 0) {
		upper_parent_id = grec.ID;
		if(grec.Flags & GF_EXCLALTFOLD)
			is_excl_alt_fold = 1;
	}
	while(GObj.P_Tbl->search(1, &k1, spGt) && k1.Kind == PPGDSK_GROUP) {
		GObj.P_Tbl->copyBufTo(&grec);
		int    accept = 0;
		if(grec.Flags & GF_ALTGROUP) {
			if(is_excl_alt_fold || (Flags & (fEnumAltGroups | fEnumAltGroupsOnly)))
				accept = 1;
		}
		else if(grec.Flags & GF_FOLDER)
			accept = 1;
		else if(!(Flags & fEnumAltGroupsOnly))
			accept = 1;
		if(accept) {
		//if(!(grec.Flags & (GF_FOLDER | GF_ALTGROUP)) || (grec.Flags & GF_ALTGROUP && is_excl_alt_fold))
			if(GObj.P_Tbl->MakeFullName(grec.ID, upper_parent_id, grp_name) > 0) {
				char * ptr = 0;
				code[0] = 0;
				THROW(GObj.GetSingleBarcode(grec.ID, code, sizeof(code)));
				size_t s = hs + cs + grp_name.Len() + 1;
				THROW_MEM(ptr = new char[s]);
				*(PPID *)ptr = grec.ID;
				memcpy(ptr+hs, code, cs);
				memcpy(ptr+hs+cs, grp_name.cptr(), grp_name.Len()+1);
				THROW_SL(List.insert(ptr));
			}
		}
	}
	if(Flags & fAddZeroGroup) {
		char * ptr = 0;
		size_t s = hs + cs + 1;
		THROW_MEM(ptr = new char[s]);
		memzero(ptr, s);
		THROW_SL(List.insert(ptr));
	}
	List.sort((Flags & fSortByCode) ? PTR_CMPFUNC(_GroupListItemCode) : PTR_CMPFUNC(_GroupListItem));
	CATCH
		ok = 0;
		List.freeAll();
	ENDCATCH
	return ok;
}

int SLAPI GoodsGroupIterator::Get(PPID id, SString & rBuf) const
{
	rBuf.Z();
	//
	uint   pos = 0;
	if(List.lsearch(&id, &pos, CMPF_LONG)) {
		rBuf = (const char *)(PTR8(List.at(pos)) + sizeof(PPID) + GRPCODESIZE);
		return 1;
	}
	else
		return -1;
	//
	/*
	for(uint i = 0; i < List.getCount(); i++) {
		const char * ptr = (const char *)List.at(i);
		if(*(PPID *)ptr == id) {
			rBuf = ptr + sizeof(PPID) + GRPCODESIZE;
			return 1;
		}
	}
	return -1;
	*/
}

int SLAPI GoodsGroupIterator::Next(PPID * pID, SString & rBuf)
{
	if(IterIndex < List.getCount()) {
		const char * ptr = (const char *)List.at(IterIndex);
		ASSIGN_PTR(pID, *(PPID *)ptr);
		if(Flags & fSortByCode)
			(rBuf = ptr + sizeof(PPID)).ShiftLeftChr('@');
		else
			rBuf.Z();
		rBuf.CatDivIfNotEmpty('-', 1).Cat(ptr + sizeof(PPID) + GRPCODESIZE);
		IterIndex++;
		return 1;
	}
	else
		return -1;
}

/*int SLAPI GoodsGroupIterator::Next(PPID * pID, char * pBuf, size_t bufLen)
{
	SString buf;
	int    ok = Next(pID, buf);
	buf.CopyTo(pBuf, bufLen);
	return ok;
}*/

int SLAPI GoodsGroupIterator::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	long   temp_val = 0;
	THROW_SL(pCtx->Serialize(dir, ParentID,  rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags,     rBuf));
	THROW_SL(pCtx->Serialize(dir, &List,     rBuf));
	THROW_SL(pCtx->Serialize(dir, (temp_val = IterIndex), rBuf));
	if(dir == -1) IterIndex = temp_val;
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPGoodsReplaceNameParam::PPGoodsReplaceNameParam() : BrandID(0), GoodsGrpID(0), ManufID(0), Flags(0)
{
}

int SLAPI PPObjGoods::MakeReplaceStr(const PPGoodsPacket * pPack,
	const PPGoodsReplaceNameParam * pParam, const char * pFragment, SString & rBuf)
{
	int    ok = 1;
	SString temp_buf;
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket gc_pack;
	Goods2Tbl::Rec grp_rec;

	PPSymbTranslator st;
	for(const char * p = pParam->RplcPattern; p && *p;)
		if(*p == '#') {
			rBuf.Cat(pFragment);
			p++;
		}
		else if(*p == '@') {
			size_t next = 1;
			temp_buf.Z();
			long   sym  = st.Translate(p, &next);
			switch(sym) {
				case PPSYM_GRNAME:
					if(pPack->Rec.ParentID && Search(pPack->Rec.ParentID, &grp_rec) > 0)
						temp_buf = grp_rec.Name;
					break;
				case PPSYM_PHPERU:
					if(pPack->Rec.PhUPerU > 0)
						temp_buf.Cat(pPack->Rec.PhUPerU, SFMT_QTTY);
					break;
				case PPSYM_GC_NAME:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID)
						temp_buf = gc_pack.Rec.Name;
					break;
				case PPSYM_GC_KIND:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUsePropKind)
						gc_pack.FormatProp(&gc_pack.PropKind, pPack->ExtRec.KindID, temp_buf);
					break;
				case PPSYM_GC_GRADE:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUsePropGrade)
						gc_pack.FormatProp(&gc_pack.PropGrade, pPack->ExtRec.GradeID, temp_buf);
					break;
				case PPSYM_GC_ADDPROP:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd)
						gc_pack.FormatProp(&gc_pack.PropAdd, pPack->ExtRec.AddObjID, temp_buf);
					break;
				case PPSYM_GC_DIMX:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUseDimX)
						gc_pack.FormatDim(&gc_pack.DimX, pPack->ExtRec.X, temp_buf);
					break;
				case PPSYM_GC_DIMY:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUseDimY)
						gc_pack.FormatDim(&gc_pack.DimY, pPack->ExtRec.Y, temp_buf);
					break;
				case PPSYM_GC_DIMZ:
					if(!gc_pack.Rec.ID && pPack->Rec.GdsClsID)
						gc_obj.GetPacket(pPack->Rec.GdsClsID, &gc_pack);
					if(gc_pack.Rec.ID && gc_pack.Rec.Flags & PPGdsCls::fUseDimZ)
						gc_pack.FormatDim(&gc_pack.DimZ, pPack->ExtRec.Z, temp_buf);
					break;
			}
			rBuf.Cat(temp_buf);
			p += next;
		}
		else
			rBuf.CatChar(*p++);
	return ok;
}

int SLAPI PPObjGoods::Helper_ReplaceName(const PPGoodsPacket * pPack, const PPGoodsReplaceNameParam * pParam, char * pDest, size_t bufLen)
{
	int    ok = -1;
	int    found = 0;
	SString temp_buf;
	if(pParam->SrchPattern.NotEmpty()) {
		size_t pos = 0, len = 0;
		SSrchParam sp(pParam->SrchPattern, 0, 0);
		if(searchstr(pDest, sp, &pos, &len)) {
			if(pParam->Flags & PPGoodsReplaceNameParam::fRestoreLastHistoryName) {
				ok = 1;
			}
			else {
				if(pParam->Flags & PPGoodsReplaceNameParam::fDoReplace) {
					SString fragment, new_fragment;
					fragment.CopyFromN(pDest+pos, len);
					MakeReplaceStr(pPack, pParam, fragment, new_fragment);
					char   temp2[512];
					STRNSCPY(temp2, pDest);
					replacestr(temp2, new_fragment, &pos, &len, sizeof(temp2));
					temp_buf = temp2;
				}
				found = 1;
			}
		}
	}
	else {
		if(pParam->Flags & PPGoodsReplaceNameParam::fRestoreLastHistoryName) {
			ok = 1;
		}
		else {
			if(pParam->Flags & PPGoodsReplaceNameParam::fDoReplace)
				MakeReplaceStr(pPack, pParam, pDest, temp_buf);
			found = 1;
		}
	}
	if(found) {
		if(pParam->Flags & PPGoodsReplaceNameParam::fDoReplace) {
			if(temp_buf.Len() < bufLen) {
				temp_buf.CopyTo(pDest, bufLen);
				ok = 1;
			}
			else {
				PPSetErrorSLib();
				SLibError = SLERR_BOUNDS;
				ok = -1;
			}
		}
		else
			ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoods::ReplaceName(PPGoodsPacket * pPack, const PPGoodsReplaceNameParam * pParam)
{
	int    ok = -1;
	if(pParam->Flags & PPGoodsReplaceNameParam::fRestoreLastHistoryName) {
		char   temp_name_buf[sizeof(pPack->Rec.Name)];
		STRNSCPY(temp_name_buf, pPack->Rec.Name);
		if(Helper_ReplaceName(pPack, pParam, temp_name_buf, sizeof(temp_name_buf)) > 0) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj) {
				LDATETIME moment;
				SysJournalTbl::Rec sj_rec;
				PPIDArray acn_list;
				acn_list.add(PPACN_OBJUPD);
				if(p_sj->GetLastObjEvent(PPOBJ_GOODS, pPack->Rec.ID, &acn_list, &moment, &sj_rec) > 0) {
					if(sj_rec.Action == PPACN_OBJUPD && sj_rec.Extra > 0) {
						SBuffer buf;
						PPGoodsPacket hist_pack;
						//
						ObjVersioningCore * p_ovc = PPRef->P_OvT; // @v9.8.11
						if(p_ovc && p_ovc->InitSerializeContext(1)) {
							PPObjID oid;
							long   vv = 0;
							if(p_ovc->Search(sj_rec.Extra, &oid, &vv, &buf) > 0) {
								SSerializeContext & r_sctx = p_ovc->GetSCtx();
								if(SerializePacket(-1, &hist_pack, buf, &r_sctx, 0)) {
									if(!sstreq(hist_pack.Rec.Name, pPack->Rec.Name) || !sstreq(hist_pack.Rec.Abbr, pPack->Rec.Abbr)) {
										STRNSCPY(pPack->Rec.Name, hist_pack.Rec.Name);
										STRNSCPY(pPack->Rec.Abbr, hist_pack.Rec.Abbr);
										ok = 1;
									}
								}
							}
						}
						/* @v9.8.11 else {
							if(HistGoods->Search(sj_rec.Extra, &buf) > 0) {
								if(SerializePacket_(0, &hist_pack, &buf)) {
									if(!sstreq(hist_pack.Rec.Name, pPack->Rec.Name) || !sstreq(hist_pack.Rec.Abbr, pPack->Rec.Abbr)) {
										STRNSCPY(pPack->Rec.Name, hist_pack.Rec.Name);
										STRNSCPY(pPack->Rec.Abbr, hist_pack.Rec.Abbr);
										ok = 1;
									}
								}
							}
						}*/
					}
				}
			}
		}
	}
	else {
		if(pParam->Flags & PPGoodsReplaceNameParam::fAbbr) {
			ok = Helper_ReplaceName(pPack, pParam, pPack->Rec.Abbr, sizeof(pPack->Rec.Abbr));
		}
		else if(pParam->Flags & PPGoodsReplaceNameParam::fNameAndAbbr) {
			ok = Helper_ReplaceName(pPack, pParam, pPack->Rec.Name, sizeof(pPack->Rec.Name));
			if(ok)
				ok = Helper_ReplaceName(pPack, pParam, pPack->Rec.Abbr, sizeof(pPack->Rec.Abbr));
		}
		else
			ok = Helper_ReplaceName(pPack, pParam, pPack->Rec.Name, sizeof(pPack->Rec.Name));
	}
	return ok;
}
