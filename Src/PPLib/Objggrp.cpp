// OBJGGRP.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjGoodsGroup)
//
GoodsGroupRecoverParam::GoodsGroupRecoverParam() : EgaFolderID(0), Ega(egaNone), Flags(0)
{
}

GoodsGroupTotal::GoodsGroupTotal() : MaxLevel(0), Count(0), AltCount(0), FoldCount(0), GrpCount(0)
{
}

PPObjGoodsGroup::PPObjGoodsGroup(void * extraPtr) : PPObjGoods(PPOBJ_GOODSGROUP, PPGDSK_GROUP, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

/*static*/int PPObjGoodsGroup::IsAlt(PPID id) { return id ? PPObjGoodsGroup().IsAltGroup(id) : -1; }
/*static*/int PPObjGoodsGroup::IsTempAlt(PPID id) { return id ? PPObjGoodsGroup().IsTempAltGroup(id) : -1; }
/*static*/int PPObjGoodsGroup::IsDynamicAlt(PPID id) { return id ? PPObjGoodsGroup().IsDynamicAltGroup(id) : -1; }

int PPObjGoodsGroup::SearchCode(const char * pCode, BarcodeTbl::Rec * pRec)
{
	char   bar_code[32];
	bar_code[0] = '@';
	strnzcpy(bar_code + 1, pCode, sizeof(bar_code) - 1);
	strip(bar_code+1);
	return P_Tbl->SearchByBarcode(bar_code, pRec);
}

int PPObjGoodsGroup::GetLevel(PPID grpID, long * pLevel)
{
	int    r = -1;
	long   level = 0;
	Goods2Tbl::Rec rec;
	PPIDArray trace;
	for(PPID id = grpID; id && (r = Fetch(id, &rec)) > 0; id = rec.ParentID) {
		if(trace.addUnique(id) < 0) {
			//
			// Мы зациклились (замкнутый контур в дереве групп)
			//
			break;
		}
		++level;
	}
	ASSIGN_PTR(pLevel, level);
	return (r > 0) ? 1 : -1;
}

int PPObjGoodsGroup::CalcTotal(GoodsGroupTotal * pTotal)
{
	GoodsGroupTotal total;
	PPIDArray id_list;
	Goods2Tbl::Key1 k1;
	BExtQuery q(P_Tbl, 1);
	MEMSZERO(k1);
	k1.Kind = PPGDSK_GROUP;
	q.select(P_Tbl->ID, P_Tbl->Flags, P_Tbl->ParentID, 0L).where(P_Tbl->Kind == PPGDSK_GROUP);
	for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
		if(P_Tbl->data.ParentID)
			id_list.add(P_Tbl->data.ID);
		total.MaxLevel = 1;
		total.Count++;
		if(P_Tbl->data.Flags & GF_FOLDER)
			total.FoldCount++;
		else if(P_Tbl->data.Flags & GF_ALTGROUP)
			total.AltCount++;
		else
			total.GrpCount++;
	}
	id_list.sortAndUndup();
	for(uint i = 0; i < id_list.getCount(); i++) {
		long   level = 0;
		GetLevel(id_list.get(i), &level);
		SETMAX(total.MaxLevel, level);
	}
	ASSIGN_PTR(pTotal, total);
	return 1;
}

int PPObjGoodsGroup::GetListOfCounts(const IntRange * pRange, LAssocArray & rResult)
{
	rResult.clear();
	int    ok = -1;
	Goods2Tbl::Key1 k1;
	PPIDArray id_list;
	{
		BExtQuery q(P_Tbl, 1);
		MEMSZERO(k1);
		k1.Kind = PPGDSK_GROUP;
		q.select(P_Tbl->ID, P_Tbl->Flags, P_Tbl->ParentID, 0L).where(P_Tbl->Kind == PPGDSK_GROUP);
		for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
			if(!(P_Tbl->data.Flags & (GF_FOLDER|GF_ALTGROUP))) {
				id_list.add(P_Tbl->data.ID);
			}
		}
	}
	id_list.sortAndUndup();
	for(uint i = 0; i < id_list.getCount(); i++) {
		const  PPID group_id = id_list.get(i);
		BExtQuery q(P_Tbl, 1);
		MEMSZERO(k1);
		k1.Kind = PPGDSK_GOODS;
		k1.ParentID = group_id;
		q.select(P_Tbl->ID, P_Tbl->ParentID, 0L).where(P_Tbl->Kind == PPGDSK_GOODS && P_Tbl->ParentID == group_id);
		long _c = 0;
		if(pRange && pRange->upp > 0) {
			bool out_of_range = false;
			for(q.initIteration(false, &k1, spGe); !out_of_range && q.nextIteration() > 0;) {
				out_of_range = (++_c > pRange->upp);
			}
			if(!out_of_range && pRange->CheckVal(_c))
				rResult.Add(group_id, _c);
		}
		else { 
			_c = q.countIterations(false, &k1, spGe);
			rResult.Add(group_id, _c);
		}
	}
	return ok;
}

/*virtual*/int PPObjGoodsGroup::MakeReserved(long flags)
{
    int    ok = -1;
    if(flags & mrfInitializeDb) {
		long    _count = 0;
		{
			StrAssocArray * p_list = PPObjGoodsGroup::MakeStrAssocList(0);
			_count = p_list ? p_list->getCount() : 0;
		}
        if(_count == 0) {
			PPID   id = 0;
            THROW(AddSimple(&id, gpkndOrdinaryGroup, 0, "default", 0, 0, 1));
			ok = 1;
        }
    }
    CATCHZOK
    return ok;
}

//int PPObjGoodsGroup::Remove(PPID id, long extraData, uint flags /* = user_request | use_transaction */)
/*virtual*/int  PPObjGoodsGroup::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    ok = -1;
	const  bool user_request = LOGIC(options & PPObject::user_request);
	const  int  r = user_request ? CONFIRM(PPCFM_DELGOODSGROUP) : 1;
	SETFLAG(options, PPObject::user_request, 0);
	ok = r ? PPObject::RemoveObjV(id, pObjColl, options, pExtraParam) : -1;
	if(!ok && user_request)
		PPError();
	return ok;
}

int PPObjGoodsGroup::DeleteObj(PPID id)
{
	int    ok = 1;
	int    r;
	PPID   branch_id = 0;
	Goods2Tbl::Rec rec;
	if(id) {
		PPGoodsConfig cfg;
		if(PPObjGoods::ReadConfig(&cfg) > 0) {
			THROW_PP(cfg.AssetGrpID != id, PPERR_GGRPHASREFINGCFG);
			THROW_PP(cfg.DefGroupID != id, PPERR_GGRPHASREFINGCFG);
		}
	}
	while(ok > 0 && (r = P_Tbl->SearchAnyRef(PPOBJ_GOODSGROUP, id, &branch_id)) > 0) {
		if(Search(branch_id, &rec) > 0) {
			//
			// Рекурсивный вызов для подуровней. Вызываем метод PPObject::Remove без транзакции и предупреждения //
			//
			PPObject::SetLastErrObj(Obj, rec.ID);
			THROW_PP(rec.Kind == PPGDSK_GROUP, PPERR_REFSEXISTS);
			THROW((r = PPObject::RemoveObjV(branch_id, 0, 0, 0)) != 0);
			if(r < 0)
				ok = -1;
		}
	}
	if(ok > 0)
		THROW(r && P_Tbl->Update(&id, 0, 0));
	CATCHZOK
	return ok;
}

int PPObjGoodsGroup::Transmit()
{
	int    ok = -1;
	PPIDArray id_list;
	ObjTransmitParam param;
	PPGoodsConfig cfg;
	const int transmit_alt_grp = BIN(P_Tbl->FetchConfig(&cfg) > 0 && cfg.Flags & GCF_XCHG_SENDALTGROUP);
	{
		BExtQuery q(P_Tbl, 1);
		Goods2Tbl::Key1 k;
		MEMSZERO(k);
		k.Kind = PPGDSK_GROUP;
		k.ParentID = 0;
		DBQ  * dbq = &(P_Tbl->Kind == PPGDSK_GROUP);
		q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->Flags, 0L).where(*dbq);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			const long _gf = P_Tbl->data.Flags;
			if((!(_gf & GF_ALTGROUP) || (transmit_alt_grp && !(_gf & GF_DYNAMIC))) && !(_gf & GF_FOLDER))
				id_list.add(P_Tbl->data.ID);
		}
	}
	if(id_list.getCount() && ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		THROW(objid_ary.Add(PPOBJ_GOODSGROUP, id_list));
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

static int EditGoodsGroupRecoverParam(GoodsGroupRecoverParam * pData)
{
	class RcvrGoodsGroupsDialog : public TDialog {
		DECL_DIALOG_DATA(GoodsGroupRecoverParam);
	public:
		RcvrGoodsGroupsDialog() : TDialog(DLG_RCVRGGRP)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_RCVRGGRP_LOG, CTL_RCVRGGRP_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_RCVRGGRP_LOG, Data.LogFileName);
			AddClusterAssoc(CTL_RCVRGGRP_FLAGS, 0, GoodsGroupRecoverParam::fCorrect);
			AddClusterAssoc(CTL_RCVRGGRP_FLAGS, 1, GoodsGroupRecoverParam::fDelTempAltGrp);
			AddClusterAssoc(CTL_RCVRGGRP_FLAGS, 2, GoodsGroupRecoverParam::fDelUnusedBrands);
			SetClusterData(CTL_RCVRGGRP_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_RCVRGGRP_EGA, 0, GoodsGroupRecoverParam::egaNone);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 1, GoodsGroupRecoverParam::egaReport);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 2, GoodsGroupRecoverParam::egaMoveToFolder);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 3, GoodsGroupRecoverParam::egaRemove);
			SetClusterData(CTL_RCVRGGRP_FLAGS, Data.Ega);
			SetupPPObjCombo(this, CTLSEL_RCVRGGRP_EGAFOLD, PPOBJ_GOODSGROUP, Data.EgaFolderID, 0, reinterpret_cast<void *>(GGRTYP_SEL_FOLDER));
			disableCtrl(CTLSEL_RCVRGGRP_EGAFOLD, Data.Ega != GoodsGroupRecoverParam::egaMoveToFolder);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlString(CTL_RCVRGGRP_LOG, Data.LogFileName);
			GetClusterData(CTL_RCVRGGRP_FLAGS, &Data.Flags);
			GetClusterData(CTL_RCVRGGRP_EGA, &Data.Ega);
			if(Data.Ega == GoodsGroupRecoverParam::egaMoveToFolder) {
				Data.EgaFolderID = getCtrlLong(sel = CTLSEL_RCVRGGRP_EGAFOLD);
				THROW_PP(Data.EgaFolderID, PPERR_GGRPFOLDERNEEDED);
				THROW_PP(GObj.CheckFlag(Data.EgaFolderID, GF_FOLDER), PPERR_GGRPFOLDERNEEDED);
			}
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_RCVRGGRP_EGA)) {
				GetClusterData(CTL_RCVRGGRP_EGA, &Data.Ega);
				disableCtrl(CTLSEL_RCVRGGRP_EGAFOLD, Data.Ega != GoodsGroupRecoverParam::egaMoveToFolder);
				clearEvent(event);
			}
		}
		PPObjGoods GObj;
	};
	DIALOG_PROC_BODY(RcvrGoodsGroupsDialog, pData);
}

int PPObjGoodsGroup::Recover(const GoodsGroupRecoverParam * pParam, PPLogger * pLogger)
{
	int    ok = -1;
	int    outer_logger = 0;
	PPLogger _logger;
	if(pLogger)
		outer_logger = 1;
	else {
		pLogger = &_logger;
		outer_logger = 0;
	}
	GoodsGroupRecoverParam param;
	if(pParam) {
		param = *pParam;
		ok = 1;
	}
	else {
		PPGetFileName(PPFILNAM_ERR_LOG, param.LogFileName);
		if(EditGoodsGroupRecoverParam(&param) > 0)
			ok = 1;
	}
	if(ok > 0) {
		const int del_temp_alt = BIN(param.Flags & GoodsGroupRecoverParam::fDelTempAltGrp);
		Goods2Tbl::Key1 k;
		SString fmt_buf, msg_buf, dest_grp_name, temp_buf;
		PPIDArray brand_to_delete_list;
		ObjCollection obj_coll;
		PPObjBrand br_obj;
		PPWaitStart();
		if(del_temp_alt)
			obj_coll.CreateFullList(gotlfExcludeDyn|gotlfExcludeObjBill|gotlfExcludeObsolete);
		if(param.Flags & GoodsGroupRecoverParam::fDelUnusedBrands) {
			GoodsCore & r_gc = *P_Tbl;
			PPIDArray brand_list;
			PPLoadText(PPTXT_FINDINGUNREFBRANDS, msg_buf);
			PPWaitMsg(msg_buf);
			br_obj.GetListByFilt(0, &brand_list);
			brand_list.sortAndUndup();
			for(uint bridx = 0; bridx < brand_list.getCount(); bridx++) {
				const  PPID brand_id = brand_list.get(bridx);
				PPBrandPacket br_pack;
				if(br_obj.Get(brand_id, &br_pack) > 0) {
					Goods2Tbl::Key3 k3;
					MEMSZERO(k3);
					k3.Kind = PPGDSK_GOODS;
					k3.BrandID = brand_id;
					if(r_gc.search(3, &k3, spEq)) {
						;
					}
					else if(BTROKORNFOUND) {
						// На брэнд '%s' не ссылается ни один товар
						pLogger->LogString(PPTXT_LOG_BRANDHASNTREFS, br_pack.Rec.Name);
						brand_to_delete_list.add(brand_id);
					}
					else {
						PPSetErrorDB();
						pLogger->LogLastError();
					}
				}
				PPWaitPercent(bridx+1, brand_list.getCount(), msg_buf);
			}
		}
		// @v11.1.6 @fix не применялась транзакция при удалении пустых групп
		int _use_ta = BIN(param.Flags & (GoodsGroupRecoverParam::fCorrect|GoodsGroupRecoverParam::fDelTempAltGrp|GoodsGroupRecoverParam::fDelUnusedBrands) ||
			oneof2(param.Ega, GoodsGroupRecoverParam::egaMoveToFolder, GoodsGroupRecoverParam::egaRemove));
		PPTransaction tra(_use_ta);
		THROW(tra);
		{
			PPIDArray temp_dyn_list;
			PPIDArray temp_list;
			SString grp_name;
			Goods2Tbl ggrp_tbl;
			BExtQuery q(&ggrp_tbl, 1);
			MEMSZERO(k);
			pLogger->LogString(del_temp_alt ? PPTXT_LOG_DELTEMPALTGRP : PPTXT_LOG_TEMPALTGROUPS, 0);
			k.Kind = PPGDSK_GROUP;
			k.ParentID = 0;
			q.select(ggrp_tbl.ID, ggrp_tbl.Name, 0L).where(ggrp_tbl.Kind == PPGDSK_GROUP);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				const  PPID  grp_id = ggrp_tbl.data.ID;
				grp_name = ggrp_tbl.data.Name;
				if(IsTempAlt(grp_id) > 0) {
					temp_list.addUnique(grp_id);
					pLogger->Log(grp_name);
				}
				else if(CheckFlag(grp_id, GF_DYNAMICTEMPALTGRP)) {
					temp_dyn_list.addUnique(grp_id);
					pLogger->Log(grp_name);
				}
			}
			if(del_temp_alt) {
				uint   i;
				const  uint tlc = temp_list.getCount();
				const  uint tdlc = temp_dyn_list.getCount();
				PPLoadText(PPTXT_TEMPALTGRPREMOVING, msg_buf);
				for(i = 0; i < tlc; i++) {
					const  PPID grp_id = temp_list.get(i);
					if(grp_id && PPObjGoodsGroup::IsTempAlt(grp_id) > 0) {
						THROW(RemoveObjV(grp_id, &obj_coll, PPObject::not_checkrights|PPObject::not_addtolog|PPObject::no_wait_indicator, 0));
					}
					PPWaitPercent(i+1, tlc+tdlc, msg_buf);
				}
				for(i = 0; i < tdlc; i++) {
					const  PPID grp_id = temp_dyn_list.get(i);
					if(grp_id && CheckFlag(grp_id, GF_DYNAMICTEMPALTGRP)) {
						THROW(RemoveObjV(grp_id, &obj_coll, PPObject::not_checkrights|PPObject::not_addtolog|PPObject::not_objnotify|PPObject::no_wait_indicator, 0));
					}
					PPWaitPercent(tlc+i+1, tdlc, msg_buf);
				}
			}
		}
		if(brand_to_delete_list.getCount() && param.Flags & GoodsGroupRecoverParam::fCorrect) {
			for(uint bridx = 0; bridx < brand_to_delete_list.getCount(); bridx++) {
				PPID   brand_id = brand_to_delete_list.get(bridx);
				PPBrandPacket br_pack;
				if(br_obj.Get(brand_id, &br_pack) > 0) {
					int    br_del_result = br_obj.PutPacket(&brand_id, 0, 0);
					if(br_del_result > 0) {
						pLogger->LogString(PPTXT_LOG_BRANDWOREFSREMOVED, br_pack.Rec.Name);
					}
					else
						pLogger->LogLastError();
				}
			}
		}
		pLogger->LogString(PPTXT_LOG_CORRECTGGRP, 0);
		{
			SString line_buf, ega_folder_name;
			PPLoadText(PPTXT_LOG_EMPTYGGRP, line_buf);
			PPIDArray process_empty_list;
			Goods2Tbl::Rec ega_folder_rec;
			if(param.EgaFolderID && Fetch(param.EgaFolderID, &ega_folder_rec) > 0) {
				ega_folder_name = ega_folder_rec.Name;
			}
			BExtQuery q(P_Tbl, 1);
			MEMSZERO(k);
			k.Kind = PPGDSK_GROUP;
			k.ParentID = 0;
			q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->Flags, P_Tbl->ParentID, 0L).where(P_Tbl->Kind == PPGDSK_GROUP);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				Goods2Tbl::Rec grp_rec, rec;
				P_Tbl->copyBufTo(&grp_rec);
				THROW(P_Tbl->CorrectCycleLink(grp_rec.ID, pLogger, 0));
				if(grp_rec.Flags & (GF_ALTGROUP | GF_FOLDER)) {
					MEMSZERO(k);
					k.Kind = PPGDSK_GOODS;
					k.ParentID = grp_rec.ID;
					while(P_Tbl->search(1, &k, spGt) && k.Kind == PPGDSK_GOODS && k.ParentID == grp_rec.ID) {
						P_Tbl->copyBufTo(&rec);
						uint msg_id = (grp_rec.Flags & GF_ALTGROUP) ? PPTXT_LOG_ALTGGRPHASGOODS : PPTXT_LOG_FOLDERGGRPHASGOODS;
						PPLoadText(msg_id, fmt_buf);
						pLogger->Log(msg_buf.Printf(fmt_buf, rec.Name, grp_rec.Name));
						if(param.Flags & GoodsGroupRecoverParam::fCorrect) {
							PPID   goods_id = rec.ID;
							PPID   dest_grp_id = 0;
							dest_grp_name = 0;
							dest_grp_name.Cat("#MOV").CatDiv('-', 1).Cat(grp_rec.Name);
							THROW(AddSimple(&dest_grp_id, gpkndOrdinaryGroup, 0, dest_grp_name, 0, 0, 0));
							rec.ParentID = dest_grp_id;
							THROW(P_Tbl->Update(&goods_id, &rec, 0));
							DS.LogAction(PPACN_OBJUPD, PPOBJ_GOODS, goods_id, 0, 0);
							PPLoadText(PPTXT_LOG_GGRPHASGOODS_CORR, fmt_buf);
							pLogger->Log(msg_buf.Printf(fmt_buf, rec.Name, dest_grp_name.cptr()));
						}
					}
				}
				else if(param.Ega != param.egaNone) {
					MEMSZERO(k);
					k.Kind = PPGDSK_GOODS;
					k.ParentID = grp_rec.ID;
					if(!P_Tbl->search(1, &k, spGt) || k.Kind != PPGDSK_GOODS || k.ParentID != grp_rec.ID) {
						// Группа '%s' не содержит ни одного товара
						// Перенесена в папку '%s'
						// Удалена                         // GoodsGroupRecoverParam
						// Не определан папка для переноса
						// Уже находится в папке для переноса
						THROW(BTROKORNFOUND);
						PPGetSubStr(line_buf, 0, fmt_buf);
						msg_buf.Printf(fmt_buf, grp_rec.Name);
						if(param.Ega == param.egaRemove) {
							PPGetSubStr(line_buf, 2, fmt_buf);
							msg_buf.CatDiv(':', 2).Cat(fmt_buf);
							process_empty_list.addUnique(grp_rec.ID);
						}
						else if(param.Ega == param.egaMoveToFolder) {
							if(ega_folder_name.NotEmpty()) {
								if(grp_rec.ParentID == param.EgaFolderID) {
									PPGetSubStr(line_buf, 4, fmt_buf);
									msg_buf.CatDiv(':', 2).Cat(fmt_buf);
								}
								else {
									PPGetSubStr(line_buf, 1, fmt_buf);
									msg_buf.CatDiv(':', 2).Cat(temp_buf.Printf(fmt_buf, ega_folder_name.cptr()));
									process_empty_list.addUnique(grp_rec.ID);
								}
							}
							else {
								PPGetSubStr(line_buf, 3, fmt_buf);
								msg_buf.CatDiv(':', 2).Cat(fmt_buf);
							}
						}
						pLogger->Log(msg_buf);
					}
				}
			}
			if(process_empty_list.getCount()) {
				for(uint i = 0; i < process_empty_list.getCount(); i++) {
					PPID _id = process_empty_list.get(i);
					if(param.Ega == param.egaRemove) {
						THROW(RemoveObjV(_id, 0, 0, 0));
					}
					else if(param.Ega == param.egaMoveToFolder) {
						PPGoodsPacket _pack;
						THROW(GetPacket(_id, &_pack, 0) > 0);
						_pack.Rec.ParentID = param.EgaFolderID;
						THROW(PutPacket(&_id, &_pack, 0));
					}
				}
			}
		}
		{
			LAssocArray err_id_list;
			Goods2Tbl::Rec goods_rec, grp_rec;
			BExtQuery q(P_Tbl, 1);
			MEMSZERO(k);
			k.Kind = PPGDSK_GOODS;
			k.ParentID = 0;
			q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->ParentID, P_Tbl->Flags, 0L).where(P_Tbl->Kind == PPGDSK_GOODS);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				P_Tbl->copyBufTo(&goods_rec);
				if(Search(goods_rec.ParentID, &grp_rec) < 0) {
					pLogger->LogMsgCode(mfError, PPERR_HANGGOODSPARENTLNK, goods_rec.Name);
					err_id_list.Add(goods_rec.ParentID, goods_rec.ID, 0);
				}
			}
			if(param.Flags & GoodsGroupRecoverParam::fCorrect && err_id_list.getCount()) {
				PPID   zero_parent_id = 0;
				for(uint i = 0; i < err_id_list.getCount(); i++) {
					MEMSZERO(grp_rec);
					PPID   parent_id = err_id_list.at(i).Key;
					PPID   goods_id = err_id_list.at(i).Val;
					if(parent_id) {
						if(Search(parent_id, &grp_rec) < 0) {
							MEMSZERO(grp_rec);
							grp_rec.ID   = parent_id;
							grp_rec.Kind = PPGDSK_GROUP;
							sprintf(grp_rec.Name, "Group Stub #%05ld", parent_id);
							THROW_DB(P_Tbl->insertRecBuf(&grp_rec));
						}
					}
					else {
						if(zero_parent_id == 0) {
							grp_rec.Kind = PPGDSK_GROUP;
							sprintf(grp_rec.Name, "Group Stub #%05ld", 0L);
							P_Tbl->copyBufFrom(&grp_rec);
							THROW_DB(P_Tbl->insertRec(0, &zero_parent_id));
						}
						if(zero_parent_id) {
							if(Search(goods_id, &goods_rec) > 0) {
								goods_rec.ParentID = zero_parent_id;
								THROW_DB(P_Tbl->updateRecBuf(&goods_rec));
							}
						}
					}
				}
			}
		}
		THROW(tra.Commit());
		PPWaitStop();
	}
	else
		ok = -1;
	CATCHZOKPPERR
	if(!outer_logger)
		pLogger->Save(param.LogFileName, 0);
	return ok;
}

int RecoverGoodsGroups(const GoodsGroupRecoverParam * pParam)
{
	return PPObjGoodsGroup().Recover(pParam, 0);
}

int RecoverGoodsGroupsNIA()
{
	GoodsGroupRecoverParam param;
	PPGetFileName(PPFILNAM_ERR_LOG, param.LogFileName);
	param.Flags |= GoodsGroupRecoverParam::fCorrect;
	return PPObjGoodsGroup().Recover(&param, 0);
}

int PPObjGoodsGroup::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->IsValid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(pDef);
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			Goods2Tbl::Rec ggrec;
			for(uint i = 0; i < list.getCount(); i++) {
				const  PPID id = list.at(i);
				long img_id = ICON_GGROUP;
				if(Fetch(id, &ggrec) > 0) {
					if(ggrec.Flags & GF_FOLDER)
						img_id = ICON_FOLDERGRP;
					else if(ggrec.Flags & GF_ALTGROUP) {
						if(ggrec.Flags & GF_TEMPALTGRP_)
							img_id = ICON_TEMPALTGRP;
						else if(ggrec.Flags & GF_DYNAMIC)
							img_id = ICON_DYNAMICALTGRP;
						else
							img_id = ICON_ALTGRP;
					}
				}
				p_def->AddImageAssoc(id, img_id);
			}
		}
	}
	return 1;
}

/*virtual*/ListBoxDef * PPObjGoodsGroup::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(pOrgDef, flags, extraPtr);
	AssignImages(p_def);
	return p_def;
}

/*virtual*//*int PPObjGoodsGroup::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, flags, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}*/

StrAssocArray * PPObjGoodsGroup::Implement_MakeStrAssocList(long parentID, const PPIDArray * pTerminalList)
{
	long   parent_id = parentID;
	StrAssocArray * p_list = new StrAssocArray();
	bool   done = false; // Сигнал для преждевременного завершения функции (не по ошибке!)
	int    alt = 0; // 0 | -1 | 1
	bool   asset = false;
	bool   excl_asset = false;
	bool   fold_only = false;
	Goods2Tbl::Rec rec;
	PPIDArray finish_id_list; // Если нужно построить список по набору идентификаторов pTerminalList, то
		// сначала мы должны получить порожденный набор идентификаторов finish_id_list, содержащий кроме pTerminalList,
		// всю иерархию родительских групп, содержащих указанный набор.
	Goods2Tbl::Key1 k;
	BExtQuery q(P_Tbl, 1);
	DBQ  * dbq = 0;
	THROW_MEM(p_list);
	if(pTerminalList) {
		if(!pTerminalList->getCount()) 
			done = true;
		else {
			finish_id_list.add(pTerminalList);
			finish_id_list.sortAndUndup();
			SString name_buf;
			PPIDArray id_list_to_remove; // Если идентификатор из списка не будет найден, то его в следующем цикле удалим
			const uint flc = finish_id_list.getCount();
			for(uint i = 0; i < flc; i++) {
				const  PPID _id = finish_id_list.get(i);
				if(Fetch(_id, &rec) > 0) {
					for(PPID _parent_id = rec.ParentID; _parent_id; _parent_id = rec.ParentID) {
						if(Fetch(_parent_id, &rec) > 0) {
							if(!finish_id_list.lsearch(_parent_id)) {
								finish_id_list.add(_parent_id); // Список, по которому ведется перебор, изменился! Но нам не надо будет перебирать этот элемент ибо мы здесь уже все сделали!
							}
						}
						else {
							// Аварийная ситуация: в цепочки наследования есть висячий идентификатор
							break; 
						}
					}
				}
				else {
					id_list_to_remove.add(_id);
				}
			}
			if(id_list_to_remove.getCount()) {
				for(uint j = 0; j < id_list_to_remove.getCount(); j++)
					finish_id_list.removeByID(id_list_to_remove.get(j));
			}
			finish_id_list.sortAndUndup();
		}
	}
	if(!done) {
		if(parent_id >= GGRTYP_SEL_FOLDER) {
			fold_only = true;
			parent_id -= GGRTYP_SEL_FOLDER;
		}
		else {
			if(parent_id >= GGRTYP_SEL_EXCLASSET) {
				excl_asset = true;
				parent_id -= GGRTYP_SEL_EXCLASSET;
			}
			if(parent_id >= GGRTYP_SEL_ASSET) {
				asset = true;
				parent_id -= GGRTYP_SEL_ASSET;
			}
			if(parent_id >= GGRTYP_SEL_NORMAL) {
				alt     = -1;
				parent_id -= GGRTYP_SEL_NORMAL;
			}
			else if(parent_id >= GGRTYP_SEL_ALT) {
				alt     = 1;
				parent_id -= GGRTYP_SEL_ALT;
			}
		}
		MEMSZERO(k);
		k.Kind = PPGDSK_GROUP;
		dbq = & (P_Tbl->Kind == PPGDSK_GROUP);
		q.select(P_Tbl->ID, P_Tbl->ParentID, P_Tbl->Name, P_Tbl->Flags, P_Tbl->GoodsTypeID, 0L).where(*dbq);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			if(!pTerminalList || finish_id_list.bsearch(P_Tbl->data.ID)) {
				const bool is_alt  = LOGIC(P_Tbl->data.Flags & GF_ALTGROUP);
				const bool is_fold = LOGIC(P_Tbl->data.Flags & GF_FOLDER);
		#ifdef _DEBUG
				const bool is_tempalt = false; // Под отладчиком нам нужна возможность видеть динамические альт группы
		#else
				const bool is_tempalt = ((P_Tbl->data.Flags & GF_TEMPALTGROUP) == GF_TEMPALTGROUP);
		#endif
				if(!is_tempalt && (!alt || (alt < 0 && !is_alt) || (alt > 0 && is_alt) || is_fold)) {
					if(fold_only && !is_fold)
						continue;
					else if(asset) {
						if(!IsAssetType(P_Tbl->data.GoodsTypeID))
							continue;
					}
					else if(excl_asset) {
						if(IsAssetType(P_Tbl->data.GoodsTypeID))
							continue;
					}
					p_list->AddFast(P_Tbl->data.ID, P_Tbl->data.ParentID, P_Tbl->data.Name);
				}
			}
		}
		p_list->SortByText();
		// @average (очень медленно работает, хоть и дает гарантию нерекурсивности)
		// p_list->RemoveRecursion(0); // @v8.3.2
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

StrAssocArray * PPObjGoodsGroup::MakeStrAssocList(void * extraPtr)
{
	return Implement_MakeStrAssocList(reinterpret_cast<long>(extraPtr), 0);
}

static long FASTCALL GetSelBias(long p)
{
	long   bias = 0;
	if(p >= GGRTYP_SEL_NORMAL)
		bias = GGRTYP_SEL_NORMAL;
	else if(p >= GGRTYP_SEL_ALT)
		bias = GGRTYP_SEL_ALT;
	return bias;
}
//
//
//
GoodsGroupView::GoodsGroupView(PPObjGoodsGroup * _ppobj) : PPListDialog(DLG_GGVIEW, CTL_GGVIEW_LIST), CurIterID(0), P_Iter(0)
{
	ContextMenuID = CTRLMENU_GOODSGROUPLIST;
	setupList();
	setupButtons();
	Draw_();
}

GoodsGroupView::~GoodsGroupView()
{
	delete P_Iter;
}

PPID GoodsGroupView::getCurrID()
{
	PPID   id = 0;
	return (P_Box && P_Box->getCurID(&id)) ? id : 0;
}

void GoodsGroupView::editQuotations(long id, int quotCls)
{
	Goods2Tbl::Rec rec;
	if(id && GGObj.Search(id, &rec) > 0 && !(rec.Flags & GF_ALTGROUP))
		GGObj.EditQuotations(id, LConfig.Location, 0, 0, quotCls);
}

void GoodsGroupView::ViewGoodsByGroup(long id)
{
	Goods2Tbl::Rec rec;
	if(id && GGObj.Search(id, &rec) > 0 && (!(rec.Flags & GF_FOLDER) || P_Box->IsTreeList())) {
		GoodsFilt flt;
		flt.GrpIDList.Add(id);
		PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
	}
}

void GoodsGroupView::setupButtons()
{
	if(!P_Box->IsTreeList()) {
		int    down, up;
		Goods2Tbl::Rec rec;
		PPID id = getCurrID();
		if(!id) {
			down = 0;
			const long extra_long = reinterpret_cast<long>(GGObj.ExtraPtr);
			up   = BIN(extra_long-GetSelBias(extra_long));
		}
		else if(GGObj.Search(id, &rec)) {
			down = BIN(rec.Flags & GF_FOLDER);
			up   = BIN(rec.ParentID);
		}
		else
			down = up = 0;
		enableCommand(cmaLevelDown, down);
		enableCommand(cmaMore,     !down);
		enableCommand(cmaLevelUp,   up);
	}
	else
		enableCommand(cmaMore, 1);
}

int GoodsGroupView::setupList()
{
	ushort v = getCtrlUInt16(CTL_GGVIEW_GGRPTYPE);
	const  long obj_extra = reinterpret_cast<long>(GGObj.ExtraPtr);
	long   bias = GetSelBias(obj_extra);
	long   extra_val = 0;
	if(v == 0)
		extra_val = obj_extra-bias;
	else if(v == 1)
		extra_val = obj_extra-bias+GGRTYP_SEL_NORMAL;
	else
		extra_val = obj_extra-bias+GGRTYP_SEL_ALT;
	P_Box->setDef(GGObj.Selector(0, 0, reinterpret_cast<void *>(extra_val)));
	return 1;
}

int GoodsGroupView::addItem(long * pPos, long * pID)
{
	PPID   obj_id = 0;
	PPID   id = 0;
	if(P_Box->IsTreeList()) {
		PPID   parent_id = 0;
		Goods2Tbl::Rec rec;
		id = getCurrID();
		if(GGObj.Search(id, &rec) <= 0 || !(rec.Flags & GF_FOLDER)) {
			if(static_cast<StdTreeListBoxDef *>(P_Box->P_Def)->GetParent(id, &parent_id) && GGObj.Search(parent_id, 0) > 0)
				id = parent_id;
			else
				id = 0;
		}
	}
	int    r = GGObj.Edit(&obj_id, reinterpret_cast<void *>(id));
	if(r == cmOK) {
		ASSIGN_PTR(pID, obj_id);
		return 2;
	}
	else
		return (r == 0) ? 0 : -1;
}

int GoodsGroupView::editItem(long, long id)
{
	int    r = (id ? GGObj.Edit(&id, 0) : -1);
	return (r == cmOK) ? 1 : ((r == 0) ? 0 : -1);
}

int GoodsGroupView::delItem(long, long id) { return id ? GGObj.RemoveObjV(id, 0, PPObject::rmv_default, 0) : -1; }

void GoodsGroupView::updateList(PPID id)
{
	if(P_Box) {
		long   cur = -1;
		ushort v = getCtrlUInt16(CTL_GGVIEW_GGRPTYPE);
		long   groups_type = (v == 1) ? GGRTYP_SEL_NORMAL : ((v == 2) ? GGRTYP_SEL_ALT : 0);
		if(id < 0)
			cur = P_Box->P_Def ? P_Box->P_Def->_curItem() : 0;
		// @v11.1.10 GGObj.UpdateSelector(P_Box->def, 0, reinterpret_cast<void *>(groups_type));
		GGObj.Selector(P_Box->P_Def, 0, reinterpret_cast<void *>(groups_type));
		if(id >= 0) {
			if(id > 0)
				P_Box->TransmitData(+1, &id);
			P_Box->Draw_();
		}
		else
			P_Box->focusItem(cur);
		setupButtons();
	}
}

void GoodsGroupView::ViewTotal()
{
	GoodsGroupTotal total;
	GGObj.CalcTotal(&total);
	TDialog * dlg = new TDialog(DLG_GGRPTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlLong(CTL_GGRPTOTAL_COUNT, total.Count);
		dlg->setCtrlLong(CTL_GGRPTOTAL_MAXLEVEL, total.MaxLevel);
		dlg->setCtrlLong(CTL_GGRPTOTAL_ALTCOUNT, total.AltCount);
		dlg->setCtrlLong(CTL_GGRPTOTAL_FOLDCOUNT, total.FoldCount);
		dlg->setCtrlLong(CTL_GGRPTOTAL_GRPCOUNT, total.GrpCount);
		ExecViewAndDestroy(dlg);
	}
}

IMPL_HANDLE_EVENT(GoodsGroupView)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		PPID   id = getCurrID();
		if(event.isClusterClk(CTL_GGVIEW_GGRPTYPE)) {
			updateList(id);
			P_Box->P_Def->top();
			setupButtons();
		}
		else if(TVCMD == cmLBItemFocused)
			setupButtons();
		else if(TVCMD == cmTransmit)
			GGObj.Transmit();
		else if(TVCMD == cmPrint)
			Print();
		else if(TVCMD == cmTotal)
			ViewTotal();
		else if(id) {
			if(TVCMD == cmSysJournalByObj) {
				ViewSysJournal(PPOBJ_GOODSGROUP, id, 1);
			}
			else {
				switch(TVCMD) {
					case cmaMore: ViewGoodsByGroup(id); break;
					case cmQuot: editQuotations(id, PPQuot::clsGeneral); break;
					case cmSupplCost: editQuotations(id, PPQuot::clsSupplDeal); break;
					case cmGoodsMatrix: editQuotations(id, PPQuot::clsMtx); break;
					case cmGoodsMatrixRestrict: editQuotations(id, PPQuot::clsMtxRestr); break;
					default: return;
				}
				setupButtons();
			}
		}
		else
			return;
	}
	else if(event.isKeyDown(kbF9))
		ViewTotal();
	else
		return;
	clearEvent(event);
}

int GoodsGroupView::GetGrpType()
{
	return static_cast<int>(getCtrlUInt16(CTL_GGVIEW_GGRPTYPE));
}

int GoodsGroupView::InitIteration()
{
	ZDELETE(P_Iter);
	CurIterID = 0;
	long   f = 0;
	ushort v = getCtrlUInt16(CTL_GGVIEW_GGRPTYPE);
	if(v == 0)
		f |= GoodsGroupIterator::fEnumAltGroups;
	else if(v == 1)
		f = 0;
	else if(v == 2)
		f |= GoodsGroupIterator::fEnumAltGroupsOnly;
	P_Iter = new GoodsGroupIterator(0, f);
	return 1;
}

int FASTCALL GoodsGroupView::NextIteration(GoodsGroupItem * pItem)
{
	SString temp_buf;
	while(pItem && P_Iter->Next(&CurIterID, temp_buf) > 0) {
		PPGoodsPacket pack;
		if(GGObj.GetPacket(CurIterID, &pack, PPObjGoods::gpoSkipQuot) > 0) {
			*static_cast<Goods2Tbl::Rec *>(pItem) = pack.Rec;
			pack.GetGroupCode(temp_buf);
			STRNSCPY(pItem->Code, temp_buf);
			GGObj.GetLevel(CurIterID, &pItem->Level);
			return 1;
		}
	}
	return -1;
}

int GoodsGroupView::Print()
{
	return PPAlddPrint(REPORT_GOODSGROUPVIEW, PView(this), 0);
}
//
//
//
int PPObjGoodsGroup::Browse(void * extraPtr)
{
	int    ok = -1;
	ExtraPtr = 0;
	if(CheckRights(PPR_READ)) {
		GoodsGroupView * p_dlg = new GoodsGroupView(this);
		if(CheckDialogPtrErr(&p_dlg)) {
			ExecViewAndDestroy(p_dlg);
			ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int PPObjGoodsGroup::ReadGoodsFilt(PPID id, GoodsFilt * flt)
{
	return flt->ReadFromProp(PPOBJ_GOODSGROUP, id, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_);
}

int PPObjGoodsGroup::Edit(PPID * pID, void * extraPtr /*parentID*/)
{
	const  PPID extra_parent_id = reinterpret_cast<PPID>(extraPtr);
	return PPObjGoods::Edit(pID, gpkndUndef, NZOR(extra_parent_id, reinterpret_cast<PPID>(ExtraPtr)), 0, 0);
}

int PPObjGoodsGroup::AddSimple(PPID * pID, GoodsPacketKind kind, PPID parentID, const char * pName, const char * pCode, PPID unitID, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	Goods2Tbl::Rec rec;
	GoodsPacketKind inner_kind = NZOR(kind, gpkndOrdinaryGroup);
	char   code[32], name[512];
	code[0] = 0;
	name[0] = 0;
	strip(STRNSCPY(name, pName));
	strip(STRNSCPY(code, pCode));
	if(name[0]) {
		BarcodeTbl::Rec bc_rec;
		if(SearchByName(name, &id, &rec) > 0) {
			THROW_PP_S(PPObjGoods::GetRecKind(&rec) == inner_kind, PPERR_UNMATCHGGRPKIND, rec.Name);
			ok = 2;
		}
		else if(code[0] && SearchCode(code, &bc_rec) > 0) {
			id = bc_rec.GoodsID;
			THROW(Fetch(id, &rec) > 0);
			THROW_PP_S(PPObjGoods::GetRecKind(&rec) == inner_kind, PPERR_UNMATCHGGRPKIND, rec.Name);
			ok = 2;
		}
		else {
			PPGoodsPacket pack;
			THROW(InitPacket(&pack, inner_kind, parentID, 0, code));
			STRNSCPY(pack.Rec.Name, name);
			pack.Rec.UnitID = unitID;
			THROW(PutPacket(&id, &pack, use_ta));
			ok = 1;
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}
//
// @ModuleDef(PPObjPckgType)
//
PPObjPckgType::PPObjPckgType(void * extraPtr) : PPObjGoods(PPOBJ_PCKGTYPE, PPGDSK_PCKGTYPE, extraPtr)
{
}

int PPObjPckgType::Get(PPID id, PPGdsPckgType * pRec)
{
	int    r = PPObjGoods::Search(id);
	if(r > 0)
		if(pRec) {
			pRec->ID = P_Tbl->data.ID;
			STRNSCPY(pRec->Name, P_Tbl->data.Name);
			STRNSCPY(pRec->CodeTempl, P_Tbl->data.Abbr);
			pRec->Flags = P_Tbl->data.Flags;
			pRec->Counter = P_Tbl->data.TaxGrpID;
			pRec->GoodsGrpID = P_Tbl->data.WrOffGrpID;
		}
	return r;
}

PPID PPObjPckgType::GetSingle()
{
	PPID   id = 0;
	int    count = 0;
	Goods2Tbl::Key2 k2;
	MEMSZERO(k2);
	k2.Kind = Kind;
	while(P_Tbl->search(2, &k2, spGt) && k2.Kind == Kind) {
		id = P_Tbl->data.ID;
		if(++count > 1) {
			id = 0;
			break;
		}
	}
	return id;
}

int PPObjPckgType::Put(PPID * pID, PPGdsPckgType * pRec, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec raw_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec)
			pRec->Flags &= ~GF_DFLTPCKGTYPE;
		if(*pID) {
			THROW(Search(*pID, &raw_rec) > 0 && raw_rec.Kind == PPGDSK_PCKGTYPE);
			if(pRec) {
				raw_rec.Kind = PPGDSK_PCKGTYPE;
				STRNSCPY(raw_rec.Name, pRec->Name);
				STRNSCPY(raw_rec.Abbr, pRec->CodeTempl);
				raw_rec.Flags = pRec->Flags;
				raw_rec.TaxGrpID = pRec->Counter;
				raw_rec.WrOffGrpID = pRec->GoodsGrpID;
				THROW(P_Tbl->Update(pID, &raw_rec, 0));
			}
			else
				THROW(P_Tbl->Update(pID, 0, 0));
		}
		else if(pRec) {
			MEMSZERO(raw_rec);
			raw_rec.Kind = PPGDSK_PCKGTYPE;
			STRNSCPY(raw_rec.Name, pRec->Name);
			STRNSCPY(raw_rec.Abbr, pRec->CodeTempl);
			raw_rec.Flags = pRec->Flags;
			raw_rec.TaxGrpID = pRec->Counter;
			raw_rec.WrOffGrpID = pRec->GoodsGrpID;
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

ListBoxDef * PPObjPckgType::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	return _Selector2(pOrgDef, 0, PPObjGoods::selfByName, 0, 0, 0);
}

int PPObjPckgType::Browse(void * extraPtr)
{
	return CheckRights(PPR_READ) ? SimpleObjView(this, extraPtr) : PPErrorZ();
}

int PPObjPckgType::Edit(long * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    valid_data = 0;
	ushort v;
	PPGdsPckgType rec;
	TDialog * dlg = 0;
	if(*pID) {
		THROW(Get(*pID, &rec) > 0);
		SETFLAG(rec.Flags, GF_DFLTPCKGTYPE, GetConfig().DefPckgTypeID == *pID);
	}
	else
		MEMSZERO(rec);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PCKGTYPE))));
	dlg->setCtrlData(CTL_PCKGTYPE_NAME,  rec.Name);
	dlg->setCtrlData(CTL_PCKGTYPE_TEMPL, rec.CodeTempl);
	dlg->setCtrlData(CTL_PCKGTYPE_COUNTER, &rec.Counter);
	SetupPPObjCombo(dlg, CTLSEL_PCKGTYPE_GRP, PPOBJ_GOODSGROUP, rec.GoodsGrpID, 0, 0);
	dlg->AddClusterAssoc(CTL_PCKGTYPE_FLAGS, 0, GF_UNIQPCKGCODE);
	dlg->AddClusterAssoc(CTL_PCKGTYPE_FLAGS, 1, GF_DFLTPCKGTYPE);
	dlg->SetClusterData(CTL_PCKGTYPE_FLAGS, rec.Flags);
	v = (rec.Flags & GF_PCKG_AROWS) ? ((rec.Flags & GF_PCKG_ANEWROW) ? 2 : 1) : 0;
	dlg->setCtrlData(CTL_PCKGTYPE_ROWINP, &v);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_PCKGTYPE_NAME,  rec.Name);
		dlg->getCtrlData(CTL_PCKGTYPE_TEMPL, rec.CodeTempl);
		dlg->getCtrlData(CTL_PCKGTYPE_COUNTER, &rec.Counter);
		dlg->getCtrlData(CTLSEL_PCKGTYPE_GRP, &rec.GoodsGrpID);
		dlg->GetClusterData(CTL_PCKGTYPE_FLAGS, &rec.Flags);
		dlg->getCtrlData(CTL_PCKGTYPE_ROWINP, &(v = 0));
		rec.Flags &= ~(GF_PCKG_AROWS | GF_PCKG_ANEWROW);
		if(v == 1)
			rec.Flags |= GF_PCKG_AROWS;
		else if(v == 2)
			rec.Flags |= (GF_PCKG_AROWS | GF_PCKG_ANEWROW);
		if(*strip(rec.Name) == 0)
			PPErrorByDialog(dlg, CTL_PCKGTYPE_NAME, PPERR_NAMENEEDED);
		else {
			PPObjGoods goods_obj;
			const bool is_default = LOGIC(rec.Flags & GF_DFLTPCKGTYPE);
			valid_data = 1;
			ok = cmOK;
			rec.Flags &= ~GF_DFLTPCKGTYPE;
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(Put(pID, &rec, 0));
				{
					PPGoodsConfig goods_cfg;
					const  bool is_config = (PPObjGoods::ReadConfig(&goods_cfg) > 0);
					const  PPID prev_dflt_pt = goods_cfg.DefPckgTypeID;
					if(is_default)
						goods_cfg.DefPckgTypeID = *pID;
					else if(*pID && goods_cfg.DefPckgTypeID == *pID)
						goods_cfg.DefPckgTypeID = 0;
					if(is_config && goods_cfg.DefPckgTypeID != prev_dflt_pt) {
						THROW(goods_obj.WriteConfig(&goods_cfg, 0, 0));
					}
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*static*/int PPObjPckgType::CodeByTemplate(const char * pTempl, long counter, char * pBuf, size_t bufLen)
{
	char   temp_buf[128];
	int    div_list_count = 0;
	int    div_list[32];
	int    i;
	int    j;
	long   n;
	char * d = temp_buf;
	const  char * p;
	if(pTempl[0] == 0)
		longfmtz(counter, 6, temp_buf, sizeof(temp_buf));
	else {
		for(p = pTempl; *p; p++) {
			if(*p == '%') {
				if(p[1] == '9') {
					div_list[div_list_count++] = 1;
					p++;
				}
				else if(p[1] == 'A') {
					div_list[div_list_count++] = 2;
					p++;
				}
			}
		}
		j = 0;
		for(p = pTempl; *p;) {
			if(*p == '%') {
				if(p[1] == '9' || p[1] == 'A') {
					n = counter;
					for(i = div_list_count-1; i > 0; i--) {
						int div = 1;
						if(div_list[i] == 1)
							div = 10;
						else if(div_list[i] == 2)
							div = 26;
						if(i > j)
							n /= div;
						else
							n %= div;
					}
					if(p[1] == '9')
						*d = '0' + static_cast<char>(n % 10);
					else if(p[1] == 'A')
						*d = 'A' + static_cast<char>(n % 26);
					d++;
					p += 2;
					j++;
				}
				else
					*d++ = *p++;
			}
			else
				*d++ = *p++;
		}
		*d = 0;
	}
	strnzcpy(pBuf, temp_buf, bufLen);
	return 1;
}
//
// @ModuleDef(PPObjTransport)
//
PPTransportConfig::PPTransportConfig() : Flags(0), OwnerKindID(0), CaptainKindID(0)
{
}

bool FASTCALL PPTransportConfig::operator == (const PPTransportConfig & rS) const
{
	return (Flags == rS.Flags && OwnerKindID == rS.OwnerKindID && CaptainKindID == rS.CaptainKindID && NameTemplate.Cmp(rS.NameTemplate, 0) == 0);
}

struct Storage_PPTranspConfig { // @persistent @store(PropertyTbl)
	size_t GetSize() const { return (sizeof(*this) + ExtStrSize); }
	PPID   Tag;               // Const=PPOBJ_CONFIG
	PPID   ID;                // Const=PPCFG_MAIN
	PPID   Prop;              // Const=PPPRP_TRANSPCFG
	long   Flags;
	uint16 ExtStrSize;        // Размер "хвоста" под строки расширения. Общий размер записи, хранимой в БД
		// равен sizeof(Storage_PPTranspConfig) + ExtStrSize
	uint16 StrPosNameTempl;   //
	PPID   OwnerKindID;       // Вид персоналии - владельцы транспортных средств. По умолчанию - PPPRK_SHIPOWNER
	PPID   CaptainKindID;     // Вид персоналии - капитаны (водители). По умолчанию - PPPRK_CAPTAIN
	char   Reserve[52];
	long   Reserve1[2];
	//char   ExtString[];
};

/*static*/int FASTCALL PPObjTransport::ReadConfig(PPTransportConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_TRANSPCFG;
	int    ok = -1;
	int    r;
	Reference * p_ref = PPRef;
	size_t sz = sizeof(Storage_PPTranspConfig) + 256;
	Storage_PPTranspConfig * p_cfg = static_cast<Storage_PPTranspConfig *>(SAlloc::M(sz));
	THROW_MEM(p_cfg);
	THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
	if(r > 0 && p_cfg->GetSize() > sz) {
		sz = p_cfg->GetSize();
		p_cfg = static_cast<Storage_PPTranspConfig *>(SAlloc::R(p_cfg, sz));
		THROW_MEM(p_cfg);
		THROW(r = p_ref->GetPropMainConfig(prop_cfg_id, p_cfg, sz));
	}
	if(r > 0) {
		pCfg->Flags = p_cfg->Flags;
		pCfg->OwnerKindID = p_cfg->OwnerKindID;
		pCfg->CaptainKindID = p_cfg->CaptainKindID;
		if(p_cfg->StrPosNameTempl)
			pCfg->NameTemplate = reinterpret_cast<const char *>(p_cfg+1) + p_cfg->StrPosNameTempl;
		else
			pCfg->NameTemplate.Z();
		ok = 1;
	}
	else {
		pCfg->Flags = 0;
		pCfg->OwnerKindID = 0;
		pCfg->CaptainKindID = 0;
		pCfg->NameTemplate = 0;
		ok = -1;
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

/*static*/int FASTCALL PPObjTransport::WriteConfig(const PPTransportConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_TRANSPCFG;
	const  long cfg_obj_type = PPCFGOBJ_TRANSP;
	int    ok = 1;
	int    is_new = 0;
	int    r;
	size_t sz = sizeof(Storage_PPTranspConfig);
	Storage_PPTranspConfig * p_cfg = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = PPRef->GetPropMainConfig(prop_cfg_id, 0, 0));
		is_new = (r > 0) ? 0 : 1;
		if(pCfg) {
			size_t ext_size = 0;
			if(pCfg->NameTemplate.NotEmpty())
				ext_size = pCfg->NameTemplate.Len()+1;
			if(ext_size)
				ext_size++; // Нулевая позиция - исключительная //
			sz += ext_size;
			p_cfg = static_cast<Storage_PPTranspConfig *>(SAlloc::M(sz));
			memzero(p_cfg, sz);
			p_cfg->Tag   = PPOBJ_CONFIG;
			p_cfg->ID    = PPCFG_MAIN;
			p_cfg->Prop  = prop_cfg_id;
			p_cfg->Flags = pCfg->Flags;
			p_cfg->OwnerKindID = pCfg->OwnerKindID;
			p_cfg->CaptainKindID = pCfg->CaptainKindID;
			if(ext_size) {
				size_t pos = 0;
				char * p_buf = reinterpret_cast<char *>(p_cfg+1);
				p_buf[pos++] = 0;
				if(pCfg->NameTemplate.NotEmpty()) {
					p_cfg->StrPosNameTempl = static_cast<uint16>(pos);
					strcpy(p_buf+pos, pCfg->NameTemplate);
					pos += (pCfg->NameTemplate.Len()+1);
				}
			}
			p_cfg->ExtStrSize = static_cast<uint16>(ext_size);
		}
		THROW(PPObject::Helper_PutConfig(prop_cfg_id, cfg_obj_type, is_new, p_cfg, sz, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	SAlloc::F(p_cfg);
	return ok;
}

/*static*/int PPObjTransport::EditConfig()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_TRANSPCFG);
	if(CheckDialogPtrErr(&dlg)) {
		PPTransportConfig cfg;
		ReadConfig(&cfg);
		const PPTransportConfig org_cfg(cfg);
		SetupPPObjCombo(dlg, CTLSEL_TRANSPCFG_OWNERK, PPOBJ_PERSONKIND, cfg.OwnerKindID, 0, 0);
		SetupPPObjCombo(dlg, CTLSEL_TRANSPCFG_CAPTK,  PPOBJ_PERSONKIND, cfg.CaptainKindID, 0, 0);
		dlg->setCtrlString(CTL_TRANSPCFG_NAMETEMPL, cfg.NameTemplate);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			cfg.OwnerKindID = dlg->getCtrlLong(CTLSEL_TRANSPCFG_OWNERK);
			cfg.CaptainKindID = dlg->getCtrlLong(CTLSEL_TRANSPCFG_CAPTK);
			dlg->getCtrlString(CTL_TRANSPCFG_NAMETEMPL, cfg.NameTemplate);
			ok = 1;
			if(!(cfg == org_cfg)) {
				if(!WriteConfig(&cfg, 1))
					ok = PPErrorZ();
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

PPTransport::PPTransport()
{
	THISZERO();
}

bool FASTCALL PPTransport::IsEq(const PPTransport & rS) const
{
#define CMP_FLD(f) if(f != rS.f) return 0;
	CMP_FLD(ID);
	CMP_FLD(TrType);
	CMP_FLD(TrModelID);
	CMP_FLD(OwnerID);
	CMP_FLD(CountryID);
	CMP_FLD(CaptainID);
	CMP_FLD(Capacity);
	CMP_FLD(VanType);
	CMP_FLD(Flags);
	if(!sstreq(Name, rS.Name))
		return false;
	if(!sstreq(Code, rS.Code))
		return false;
	if(!sstreq(TrailerCode, rS.TrailerCode))
		return false;
	return true;
#undef CMP_FLD
}

PPTransportPacket::PPTransportPacket()
{
}

PPTransportPacket & PPTransportPacket::Z()
{
	MEMSZERO(Rec);
	TagL.Z();
	return *this;
}
	
bool FASTCALL PPTransportPacket::IsEq(const PPTransportPacket & rS) const { return (Rec.IsEq(rS.Rec) && TagL.IsEq(rS.TagL)); }

PPObjTransport::PPObjTransport(void * extraPtr) : PPObjGoods(PPOBJ_TRANSPORT, PPGDSK_TRANSPORT, extraPtr)
{
}

int PPObjTransport::Get(PPID id, PPTransportPacket * pPack)
{
	int    ok = -1;
	int    r;
	Goods2Tbl::Rec goods_rec;
	THROW(r = PPObjGoods::Search(id, &goods_rec));
	THROW_PP_S(goods_rec.Kind == PPGDSK_TRANSPORT, PPERR_INVTRANSPORTRECKIND, id);
	if(r > 0) {
		ok = 1;
		if(pPack) {
			BarcodeArray bc_list;
			pPack->Rec.ID = goods_rec.ID;
			pPack->Rec.TrType = goods_rec.GdsClsID;
			STRNSCPY(pPack->Rec.Name, goods_rec.Name);
			pPack->Rec.TrModelID = goods_rec.BrandID;
			pPack->Rec.OwnerID   = goods_rec.ManufID;
			pPack->Rec.CountryID = goods_rec.DefBCodeStrucID;
			pPack->Rec.CaptainID = goods_rec.RspnsPersonID;
			pPack->Rec.Capacity  = static_cast<long>(goods_rec.PhUPerU);
			pPack->Rec.VanType   = goods_rec.VanType;
			SETFLAG(pPack->Rec.Flags, GF_PASSIV, goods_rec.Flags & GF_PASSIV);
			P_Tbl->ReadBarcodes(id, bc_list);
			for(uint i = 0; i < bc_list.getCount(); i++) {
				BarcodeTbl::Rec & r_bc_rec = bc_list.at(i);
				if(r_bc_rec.Code[0] == '^') {
					if(r_bc_rec.Qtty == 1.0) {
						STRNSCPY(pPack->Rec.Code, r_bc_rec.Code+1);
					}
					else if(r_bc_rec.Qtty == 2.0) {
						STRNSCPY(pPack->Rec.TrailerCode, r_bc_rec.Code+1);
					}
				}
			}
			THROW(GetTagList(id, &pPack->TagL)); // @v11.2.12
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjTransport::MakeStorage(PPID id, const PPTransport * pRec, Goods2Tbl::Rec * pRawRec, BarcodeArray * pBcList)
{
	int    ok = 1;
	memzero(pRawRec, sizeof(*pRawRec));
	pRawRec->ID = id;
	pRawRec->Kind = PPGDSK_TRANSPORT;
	pRawRec->GdsClsID = pRec->TrType;
	STRNSCPY(pRawRec->Name, pRec->Name);
	pRawRec->BrandID = pRec->TrModelID;
	pRawRec->ManufID = pRec->OwnerID;
	pRawRec->DefBCodeStrucID = pRec->CountryID;
	pRawRec->RspnsPersonID = pRec->CaptainID;
	pRawRec->PhUPerU = pRec->Capacity;
	pRawRec->VanType = pRec->VanType;
	SETFLAG(pRawRec->Flags, GF_PASSIV, pRec->Flags & GF_PASSIV);
	if(pBcList) {
		SString temp_buf;
		pBcList->freeAll();
		//
		// Transport Code
		//
		temp_buf = pRec->Code;
		if(temp_buf.NotEmptyS()) {
			BarcodeTbl::Rec bc_rec;
			bc_rec.GoodsID = id;
			bc_rec.Qtty = 1.0;
			temp_buf.PadLeft(1, '^').CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
			pBcList->insert(&bc_rec);
		}
		//
		// Trailer Code
		//
		temp_buf = pRec->TrailerCode;
		if(temp_buf.NotEmptyS()) {
			BarcodeTbl::Rec bc_rec;
			bc_rec.GoodsID = id;
			bc_rec.Qtty = 2.0;
			temp_buf.PadLeft(1, '^').CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
			pBcList->insert(&bc_rec);
		}
	}
	return ok;
}

int PPObjTransport::Put(PPID * pID, const PPTransportPacket * pPack, int use_ta)
{
	int    ok = 1;
	int    action = 0;
	Goods2Tbl::Rec raw_rec;
	PPTransportPacket org_pack;
	BarcodeArray bc_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Get(*pID, &org_pack) > 0);
			if(pPack) {
				if(!pPack->IsEq(org_pack)) {
					THROW(MakeStorage(*pID, &pPack->Rec, &raw_rec, &bc_list));
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
					THROW(SetTagList(*pID, &pPack->TagL, 0)); // @v11.2.12
					action = PPACN_OBJUPD;
				}
				else
					ok = -1;
			}
			else {
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(P_Tbl->UpdateBarcodes(*pID, 0, 0));
				THROW(SetTagList(*pID, 0, 0)); // @v11.2.12
			}
		}
		else if(pPack) {
			THROW(MakeStorage(0, &pPack->Rec, &raw_rec, 0));
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			//
			// Теперь знаем идентификатор и можно сохранить список кодов
			//
			THROW(MakeStorage(*pID, &pPack->Rec, &raw_rec, &bc_list));
			THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
			THROW(SetTagList(*pID, &pPack->TagL, 0)); // @v11.2.12
			action = PPACN_OBJADD;
		}
		if(action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

ListBoxDef * PPObjTransport::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	return _Selector2(pOrgDef, 0, PPObjGoods::selfByName, extraPtr, 0, 0);
}

LongArray * PPObjTransport::MakeList(long trType)
{
	LongArray * p_list = 0;
	StrAssocListBoxDef * p_lbx_def = static_cast<StrAssocListBoxDef *>(Selector(0, 0, reinterpret_cast<void *>(trType)));
	if(p_lbx_def) {
		p_list = new LongArray;
		if(p_list)
			p_lbx_def->getIdList(*p_list);
	}
	ZDELETE(p_lbx_def);
	return p_list;
}

int PPObjTransport::Browse(void * extraPtr)
{
	return CheckRights(PPR_READ) ? SimpleObjView(this, extraPtr) : PPErrorZ();
}

class TransportDialog : public TDialog {
	DECL_DIALOG_DATA(PPTransportPacket);
public:
	explicit TransportDialog(uint dlgID) : TDialog(dlgID), LockAutoName(0)
	{
		PPObjTransport::ReadConfig(&Cfg);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		LockAutoName = 1;
		if(Cfg.NameTemplate.NotEmpty() && Data.Rec.TrType == PPTRTYP_CAR)
			selectCtrl(CTL_TRANSPORT_MODEL);
		PPID   owner_kind_id = NZOR(Cfg.OwnerKindID, PPPRK_SHIPOWNER);
		PPID   captain_kind_id = NZOR(Cfg.CaptainKindID, PPPRK_CAPTAIN);
		setCtrlData(CTL_TRANSPORT_NAME,  Data.Rec.Name);
		setCtrlData(CTL_TRANSPORT_CODE,  Data.Rec.Code);
		setCtrlData(CTL_TRANSPORT_TRAILCODE, Data.Rec.TrailerCode);
		setCtrlReal(CTL_TRANSPORT_CAPACITY, fdiv1000i(Data.Rec.Capacity));
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_MODEL, PPOBJ_TRANSPMODEL, Data.Rec.TrModelID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_OWNER, PPOBJ_PERSON, Data.Rec.OwnerID, OLW_CANINSERT, reinterpret_cast<void *>(owner_kind_id));
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_CAPTAIN, PPOBJ_PERSON, Data.Rec.CaptainID, OLW_CANINSERT, reinterpret_cast<void *>(captain_kind_id));
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_CNTRY, PPOBJ_COUNTRY, Data.Rec.CountryID, OLW_CANINSERT);
		if(Data.Rec.TrType == PPTRTYP_CAR) {
			SetupStringCombo(this, CTLSEL_TRANSPORT_VANTYP, PPTXT_VANTYPE, Data.Rec.VanType);
		}
		AddClusterAssoc(CTL_TRANSPORT_FLAGS, 0, GF_PASSIV);
		SetClusterData(CTL_TRANSPORT_FLAGS, Data.Rec.Flags);
		LockAutoName = 0;
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		Helper_GetDTS();
		if(*strip(Data.Rec.Name) == 0) {
			ok = PPErrorByDialog(this, CTL_TRANSPORT_NAME, PPERR_NAMENEEDED);
		}
		else {
			ASSIGN_PTR(pData, Data);
		}
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			Data.TagL.Oid.Obj = PPOBJ_TRANSPORT;
			EditObjTagValList(&Data.TagL, 0);
			clearEvent(event);			
		}
		else if(!LockAutoName && Cfg.NameTemplate.NotEmpty()) {
			if(event.isCbSelected(CTLSEL_TRANSPORT_MODEL) || event.isCbSelected(CTLSEL_TRANSPORT_OWNER) ||
				event.isCbSelected(CTLSEL_TRANSPORT_CAPTAIN) ||
				(event.isCmd(cmInputUpdated) &&
				(event.isCtlEvent(CTL_TRANSPORT_CODE) || event.isCtlEvent(CTL_TRANSPORT_TRAILCODE)))) {
				Helper_GetDTS();
				SString name_buf;
				TrObj.GetNameByTemplate(&Data.Rec, Cfg.NameTemplate, name_buf);
				setCtrlString(CTL_TRANSPORT_NAME, name_buf);
				clearEvent(event);
			}
		}
	}
	void   Helper_GetDTS()
	{
		getCtrlData(CTL_TRANSPORT_NAME,  Data.Rec.Name);
		getCtrlData(CTL_TRANSPORT_CODE,  Data.Rec.Code);
		getCtrlData(CTL_TRANSPORT_TRAILCODE,  Data.Rec.TrailerCode);
		getCtrlData(CTLSEL_TRANSPORT_MODEL, &Data.Rec.TrModelID);
		getCtrlData(CTLSEL_TRANSPORT_OWNER, &Data.Rec.OwnerID);
		getCtrlData(CTLSEL_TRANSPORT_CNTRY, &Data.Rec.CountryID);
		getCtrlData(CTLSEL_TRANSPORT_CAPTAIN, &Data.Rec.CaptainID);
		if(Data.Rec.TrType == PPTRTYP_CAR) {
			long   temp_val = 0;
			getCtrlData(CTLSEL_TRANSPORT_VANTYP, &temp_val);
			Data.Rec.VanType = static_cast<int16>(temp_val);
		}
		Data.Rec.Capacity = static_cast<long>(getCtrlReal(CTL_TRANSPORT_CAPACITY) * 1000.0);
		Data.Rec.Flags = static_cast<int16>(GetClusterData(CTL_TRANSPORT_FLAGS));
	}
	int    LockAutoName;
	PPTransportConfig Cfg;
	PPObjTransport TrObj;
};

int PPObjTransport::Edit(PPID * pID, void * extraPtr /*initTrType*/)
{
	int    ok = -1;
	int    valid_data = 0;
	uint   dlg_id = 0;
	long   tr_type = reinterpret_cast<long>(extraPtr);
	TDialog * sel_dlg = 0;
	PPTransportPacket pack;
	if(*pID) {
		THROW(Get(*pID, &pack) > 0);
		tr_type = pack.Rec.TrType;
	}
	else {
		pack.Z();
		if(tr_type != PPTRTYP_CAR && tr_type != PPTRTYP_SHIP) {
			tr_type = 0;
			THROW(CheckDialogPtr(&(sel_dlg = new TDialog(DLG_TRSEL))));
			sel_dlg->setCtrlUInt16(CTL_TRSEL_WHAT, 0);
			if(ExecView(sel_dlg) == cmOK) {
				ushort v = sel_dlg->getCtrlUInt16(CTL_TRSEL_WHAT);
				if(v == 0)
					tr_type = PPTRTYP_CAR;
				else if(v == 1)
					tr_type = PPTRTYP_SHIP;
			}
			ZDELETE(sel_dlg);
		}
		pack.Rec.TrType = tr_type;
	}
	if(tr_type == PPTRTYP_CAR)
		dlg_id = DLG_TR_CAR;
	else if(tr_type == PPTRTYP_SHIP)
		dlg_id = DLG_TR_SHIP;
	else
		dlg_id = 0;
	if(dlg_id) {
		if(PPDialogProcBodyID<TransportDialog, PPTransportPacket>(dlg_id, &pack) > 0) {
			valid_data = 1;
			ok = cmOK;
			THROW(Put(pID, &pack, 1));
		}
	}
	CATCHZOKPPERR
	delete sel_dlg;
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjTransport, PPTransport);

int PPObjTransport::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPTransport * p_pack = static_cast<PPTransport *>(p->Data);
		ProcessObjRefInArray(PPOBJ_TRANSPMODEL, &p_pack->TrModelID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->OwnerID, ary, replace);
		ProcessObjRefInArray(PPOBJ_COUNTRY, &p_pack->CountryID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->CaptainID, ary, replace);
		return 1;
	}
	return -1;
}

int PPObjTransport::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext *)
{
	// @v11.2.12 @dbd_exchange
	int    ok = -1;
	PPTransport * p_pack = 0;
	THROW_MEM(p->Data = new PPTransport);
	p_pack = static_cast<PPTransport *>(p->Data);
	if(stream == 0) {
		PPTransportPacket temp_pack; // @v11.2.12
		THROW(Get(id, &temp_pack) > 0);
		*p_pack = temp_pack.Rec; // @v11.2.12
	}
	else
		THROW(ReadBlk(p_pack, sizeof(*p_pack), stream));
	CATCHZOK
	return ok;
}

int PPObjTransport::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	// @v11.2.12 @dbd_exchange
	int    ok = 1;
	if(p && p->Data) {
		PPTransport * p_pack = static_cast<PPTransport *>(p->Data);
		if(stream == 0) {
			PPTransport temp_rec;
			PPTransportPacket temp_pack; // @v11.2.12
			if((*pID || SearchByName(p_pack->Name, pID, 0) > 0) && Get(*pID, &temp_pack) > 0) {
				temp_pack.Rec = *p_pack;
				if(!Put(pID, &temp_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTTRANSPORT, p_pack->ID, p_pack->Name);
					ok = -1;
				}
			}
			else {
				temp_pack.Z();
				temp_pack.Rec = *p_pack;
				if(!Put(pID, &temp_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTTRANSPORT, p_pack->ID, p_pack->Name);
					ok = -1;
				}
			}
		}
		else {
			THROW(WriteBlk(p_pack, sizeof(*p_pack), stream));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjTransport::GetNameByTemplate(PPTransport * pPack, const char * pTemplate, SString & rBuf) const
{
	int    ok = 1;
	SString buf, temp_buf;
	PPSymbTranslator st;
	for(const char * p = pTemplate; p && *p;)
		if(*p == '@') {
			size_t next = 1;
			long   sym  = st.Translate(p, &next);
			temp_buf.Z();
			switch(sym) {
				case PPSYM_MODEL:
					if(pPack->TrModelID)
						GetObjectName(PPOBJ_TRANSPMODEL, pPack->TrModelID, temp_buf);
					break;
				case PPSYM_CODE:
					temp_buf = pPack->Code;
					break;
				case PPSYM_SUBCODE:
					temp_buf = pPack->TrailerCode;
					break;
				case PPSYM_OWNER:
					if(pPack->OwnerID)
						GetPersonName(pPack->OwnerID, temp_buf);
					break;
				case PPSYM_CAPTAIN:
					if(pPack->CaptainID)
						GetPersonName(pPack->CaptainID, temp_buf);
					break;
			}
			buf.Cat(temp_buf);
			p += next;
		}
		else
			buf.CatChar(*p++);
	if(buf.NotEmptyS()) {
		rBuf = buf;
		ok = 1;
	}
	else {
		rBuf.Z();
		ok = -1;
	}
	return ok;
}
//
// @ModuleDef(PPObjBrand)
//
PPBrand::PPBrand()
{
	THISZERO();
}

bool FASTCALL PPBrand::IsEq(const PPBrand & rS) const
{
	return (ID == rS.ID && OwnerID == rS.OwnerID && ParentID == rS.ParentID && Flags == rS.Flags && sstreq(Name, rS.Name));
}

int FASTCALL PPBrand::CheckForFilt(const BrandFilt * pFilt) const
{
	int    ok = 1;
	if(pFilt) {
		if(!pFilt->OwnerList.CheckID(OwnerID))
			ok = 0;
		else if(!pFilt->ParentList.CheckID(ParentID))
			ok = 0;
		else {
			if(pFilt->SrchStr.NotEmpty()) {
				if(pFilt->Flags & BrandFilt::fSubName) {
					SString & r_temp_buf = SLS.AcquireRvlStr();
					r_temp_buf = Name;
					if(!r_temp_buf.Search(pFilt->SrchStr, 0, 1, 0))
						ok = 0;
				}
				else if(pFilt->SrchStr.CmpNC(Name) != 0) {
					ok = 0;
				}
			}
		}
	}
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(Brand); BrandFilt::BrandFilt() : PPBaseFilt(PPFILT_BRAND, 0, 1)
{
	SetFlatChunk(offsetof(BrandFilt, ReserveStart), offsetof(BrandFilt, SrchStr) - offsetof(BrandFilt, ReserveStart));
	SetBranchSString(offsetof(BrandFilt, SrchStr));
	SetBranchObjIdListFilt(offsetof(BrandFilt, ParentList));
	SetBranchObjIdListFilt(offsetof(BrandFilt, OwnerList));
	Init(1, 0);
}

BrandFilt & FASTCALL BrandFilt::operator = (const BrandFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

bool BrandFilt::IsEmpty() const { return (!Flags && SrchStr.IsEmpty() && ParentList.IsEmpty() && OwnerList.IsEmpty()); }

PPBrandPacket::PPBrandPacket() : LinkFiles(PPOBJ_BRAND)
{
}

PPBrandPacket::~PPBrandPacket()
{
	LinkFiles.Clear();
}

void PPBrandPacket::Init()
{
	LinkFiles.Clear();
	TagL.Z(); // @v11.2.12
}

bool FASTCALL PPBrandPacket::IsEq(const PPBrandPacket & rS) const { return (Rec.IsEq(rS.Rec) && TagL.IsEq(rS.TagL)); }

PPBrandPacket & FASTCALL PPBrandPacket::operator = (const PPBrandPacket & rSrc)
{
	memcpy(&Rec, &rSrc.Rec, sizeof(Rec));
	LinkFiles = rSrc.LinkFiles;
	TagL = rSrc.TagL; // @v11.2.12
	return *this;
}
//
// @ModuleDef(PPObjBrand)
//
PPObjBrand::PPObjBrand(void * extraPtr) : PPObjGoods(PPOBJ_BRAND, PPGDSK_BRAND, extraPtr)
{
}

/*static*/int FASTCALL PPObjBrand::Helper_GetRec(const Goods2Tbl::Rec & rGoodsRec, PPBrand * pRec)
{
	int    ok = 1;
	if(pRec) {
		memzero(pRec, sizeof(*pRec));
		if(rGoodsRec.Kind == PPGDSK_BRAND) {
			pRec->ID = rGoodsRec.ID;
			STRNSCPY(pRec->Name, rGoodsRec.Name);
			pRec->OwnerID   = rGoodsRec.ManufID;
			pRec->Flags     = rGoodsRec.Flags;
		}
		else
			ok = PPSetError(/*PPERR_INVBRANDRECKIND*/1, rGoodsRec.ID);
	}
	return ok;
}

int PPObjBrand::Fetch(PPID id, PPBrand * pRec)
{
	Goods2Tbl::Rec goods_rec;
	int    ok = PPObjGoods::Fetch(id, &goods_rec);
	return (ok > 0) ? Helper_GetRec(goods_rec, pRec) : ok;
}

int PPObjBrand::Put(PPID * pID, PPBrandPacket * pPack, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec raw_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			PPBrandPacket org_pack;
			//THROW(Search(*pID, &raw_rec) > 0 && raw_rec.Kind == PPGDSK_BRAND);
			THROW(Get(*pID, &org_pack) > 0);
			if(pPack) {
				if(!pPack->IsEq(org_pack) || pPack->LinkFiles.IsChanged(*pID, 0L)) {
					THROW(CheckRights(PPR_MOD));
					raw_rec.ID = org_pack.Rec.ID;
					raw_rec.Kind = PPGDSK_BRAND;
					STRNSCPY(raw_rec.Name, pPack->Rec.Name);
					raw_rec.ManufID = pPack->Rec.OwnerID;
					raw_rec.Flags   = pPack->Rec.Flags;
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					THROW(SetTagList(*pID, &pPack->TagL, 0)); // @v11.2.12
					if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
						// THROW_PP(CheckRights(PSNRT_UPDIMAGE), PPERR_NRT_UPDIMAGE);
						pPack->LinkFiles.Save(*pID, 0L);
					}
					DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				}
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(SetTagList(*pID, 0, 0)); // @v11.2.12
				{
					ObjLinkFiles _lf(PPOBJ_BRAND);
					_lf.Save(*pID, 0L);
				}
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
			}
		}
		else if(pPack) {
			THROW(CheckRights(PPR_INS));
			MEMSZERO(raw_rec);
			raw_rec.Kind = PPGDSK_BRAND;
			STRNSCPY(raw_rec.Name, pPack->Rec.Name);
			raw_rec.ManufID = pPack->Rec.OwnerID;
			raw_rec.Flags   = pPack->Rec.Flags;
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			THROW(SetTagList(*pID, &pPack->TagL, 0)); // @v11.2.12
			if(pPack->LinkFiles.IsChanged(*pID, 0L)) {
				// THROW_PP(CheckRights(PSNRT_UPDIMAGE), PPERR_NRT_UPDIMAGE);
				pPack->LinkFiles.Save(*pID, 0L);
			}
			DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBrand::Get(PPID id, PPBrandPacket * pPack)
{
	int    ok = PPObjGoods::Search(id);
	if(ok > 0) {
		if(pPack) {
			THROW(Helper_GetRec(P_Tbl->data, &pPack->Rec));
			THROW(GetTagList(id, &pPack->TagL));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjBrand::AddSimple(PPID * pID, const char * pName, PPID ownerID, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	char   name[512];
	name[0] = 0;
	strip(STRNSCPY(name, pName));
	if(name[0]) {
		if(SearchByName(name, &id) > 0) {
		}
		else {
			PPBrandPacket pack;
			STRNSCPY(pack.Rec.Name, name);
			pack.Rec.OwnerID = ownerID;
			THROW(Put(&id, &pack, use_ta));
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPObjBrand::GetListByFilt(const BrandFilt * pFilt, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray result_list;
	Goods2Tbl::Key2 k2;
	const  PPID single_owner_id = pFilt ? pFilt->OwnerList.GetSingle() : 0;
	BExtQuery q(P_Tbl, 2);
	DBQ * dbq = &(P_Tbl->Kind == Kind);
	dbq = ppcheckfiltid(dbq, P_Tbl->ManufID, single_owner_id);
	q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->GoodsTypeID, P_Tbl->ManufID, P_Tbl->Flags, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Kind = Kind;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		PPBrand rec;
		if(Helper_GetRec(P_Tbl->data, &rec) && rec.CheckForFilt(pFilt)) {
			THROW_SL(result_list.add(rec.ID));
		}
	}
	CATCHZOK
	ASSIGN_PTR(pList, result_list);
	return ok;
}

ListBoxDef * PPObjBrand::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	return _Selector2(pOrgDef, 0, PPObjGoods::selfByName, 0, 0, 0);
}

/*virtual*/void * PPObjBrand::CreateObjListWin(uint flags, void * extraPtr)
{
	return 0;
#if 0 // {
	class PPObjBrandListWindow : public PPObjListWindow {
	public:
		PPObjBrandListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaMore;
			SetToolbar(TOOLBAR_LIST_BRAND);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				getResult(&id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmUniteObj:
							if(id) {
								if(PPObject::ReplaceObjInteractive(P_Obj->Obj, id) > 0)
									update = 2;
							}
							break;
						case cmaMore:
							if(id) {
								GoodsFilt filt;
								filt.BrandList.Add(id);
								static_cast<PPApp *>(APPL)->LastCmd = TVCMD;
								PPView::Execute(PPVIEW_GOODS, &filt, 1, 0);
							}
							break;
					}
				}
				PostProcessHandleEvent(update, id);
			}
		}
	};
	return new PPObjBrandListWindow(this, flags, extraPtr);
#endif // } 0
}

int PPObjBrand::Browse(void * extraPtr)
{
	return PPView::Execute(PPVIEW_BRAND, 0, 1, 0);
#if 0 // {
	class BrandView : public ObjViewDialog {
	public:
		explicit BrandView(PPObjBrand * pObj) : ObjViewDialog(DLG_BRANDVIEW, pObj, 0)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmUniteObj)) {
				if(getCurrID()) {
					if(PPObject::ReplaceObjInteractive(P_Obj->Obj, getCurrID()) > 0) {
						updateList(-1);
					}
				}
				clearEvent(event);
			}
		}
		virtual void extraProc(long id)
		{
			if(id) {
				GoodsFilt filt;
				filt.BrandList.Add(id);
				PPView::Execute(PPVIEW_GOODS, &filt, 1, 0);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new BrandView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
#endif // } 0
}

//#define GRP_IBG 1

class BrandDialog : public TDialog {
	DECL_DIALOG_DATA(PPBrandPacket);
	enum {
		ctlgroupIbg = 1
	};
public:
	BrandDialog() : TDialog(DLG_BRAND)
	{
		addGroup(ctlgroupIbg, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_GOODS_IMAGE,
			cmAddImage, cmDelImage, 1, ImageBrowseCtrlGroup::fUseExtOpenDlg));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//
		setCtrlLong(CTL_GOODS_ID, Data.Rec.ID);
		setCtrlData(CTL_GOODS_NAME, Data.Rec.Name);
		{
			ImageBrowseCtrlGroup::Rec rec;
			Data.LinkFiles.Init(PPOBJ_BRAND);
			if(Data.Rec.Flags & GF_DERIVED_HASIMAGES)
				Data.LinkFiles.Load(Data.Rec.ID, 0L);
			Data.LinkFiles.At(0, rec.Path);
			setGroupData(ctlgroupIbg, &rec);
		}
		SetupPPObjCombo(this, CTLSEL_GOODS_MANUF, PPOBJ_PERSON, Data.Rec.OwnerID, OLW_CANINSERT, reinterpret_cast<void *>(PPPRK_MANUF));
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_GOODS_NAME,  Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTLSEL_GOODS_MANUF, &Data.Rec.OwnerID);
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(getGroupData(ctlgroupIbg, &rec))
				if(rec.Path.Len()) {
					THROW(Data.LinkFiles.Replace(0, rec.Path));
				}
				else
					Data.LinkFiles.Remove(0);
			SETFLAG(Data.Rec.Flags, GF_DERIVED_HASIMAGES, Data.LinkFiles.GetCount());
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			Data.TagL.Oid.Obj = PPOBJ_BRAND;
			EditObjTagValList(&Data.TagL, 0);
			clearEvent(event);
		}
	}
};

int PPObjBrand::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1;
	bool   valid_data = false;
	bool   is_new = false;
	BrandDialog * dlg = 0;
	PPBrandPacket pack;
	THROW(EditPrereq(pID, dlg, &is_new));
	THROW(CheckDialogPtr(&(dlg = new BrandDialog())));
	if(!is_new) {
		THROW(Get(*pID, &pack) > 0);
	}
	else
		pack.Init();
	THROW(dlg->setDTS(&pack));
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(Put(pID, &pack, 1)) {
				ok = cmOK;
				valid_data = true;
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjBrand, PPBrand);

int PPObjBrand::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPBrand * p_pack = static_cast<PPBrand *>(p->Data);
		return BIN(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->OwnerID,ary, replace));
	}
	return -1;
}

int PPObjBrand::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext *)
{
	int    ok = -1;
	PPBrand * p_pack = 0;
	THROW_MEM(p->Data = new PPBrand);
	p_pack = static_cast<PPBrand *>(p->Data);
	if(stream == 0) {
		PPBrandPacket pack;
		THROW(Get(id, &pack) > 0);
		*p_pack = pack.Rec;
	}
	else
		THROW(ReadBlk(p_pack, sizeof(*p_pack), stream));
	CATCHZOK
	return ok;
}

int PPObjBrand::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPBrand * p_pack = static_cast<PPBrand *>(p->Data);
		if(stream == 0) {
			PPBrandPacket temp_pack;
			if((*pID || SearchByName(p_pack->Name, pID, 0) > 0) && Get(*pID, &temp_pack) > 0) {
				temp_pack.Rec = *p_pack;
				if(!Put(pID, &temp_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTBRAND, p_pack->ID, p_pack->Name);
					ok = -1;
				}
			}
			else {
				temp_pack.Rec = *p_pack;
				if(!Put(pID, &temp_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTBRAND, p_pack->ID, p_pack->Name);
					ok = -1;
				}
			}
		}
		else
			THROW(WriteBlk(p_pack, sizeof(*p_pack), stream));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsGroup::SetOwner(PPID id, long curOwner, long newOwner)
{
	int    ok = -1;
	if(id && PPObjGoodsGroup::IsTempAlt(id)) {
		PPObjGoodsGroup gg_obj;
		PPTransaction tra(-1);
		THROW(tra);
		if(gg_obj.Search(id, 0) > 0) {
			long cur_owner = gg_obj.P_Tbl->data.ManufID; // as owner
			if(!cur_owner || cur_owner == curOwner) {
				gg_obj.P_Tbl->data.ManufID = newOwner;
				gg_obj.P_Tbl->updateRec();
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
	ENDCATCH
	return ok;
}

/*static*/int PPObjGoodsGroup::RemoveTempAlt(PPID id, long owner, int forceDel /*=0*/, int useTa /*=1*/)
{
	int    ok = -1;
	if(id && PPObjGoodsGroup::IsTempAlt(id) > 0) {
		Goods2Tbl::Rec rec;
		PPObjGoodsGroup gg_obj;
		if(forceDel || (gg_obj.Search(id, &rec) > 0 && rec.ManufID == owner)) {
			uint options = PPObject::not_checkrights|PPObject::not_addtolog;
			SETFLAG(options, PPObject::use_transaction, useTa);
			ok = gg_obj.RemoveObjV(id, 0, options, 0);
		}
	}
	return ok;
}

/*static*/int PPObjGoodsGroup::AddDynamicAltGroupByFilt(const GoodsFilt * pFilt, PPID * pGrpID, long owner, int useTa)
{
	int    ok = 1;
	if(pFilt && !pFilt->IsEmpty()) {
		PPID   grp_id = 0;
		SString grp_name, buf;
		SYSTEMTIME sys_time;
		GoodsFilt tmpf = *pFilt;
		Goods2Tbl::Rec grec;
		PPObjGoods g_obj;
		{
			PPTransaction tra(useTa);
			THROW(tra);
			MEMSZERO(grec);
			grec.Kind   = PPGDSK_GROUP;
			grec.Flags |= GF_DYNAMICTEMPALTGRP;
			grec.ManufID = owner;
			PPGetWord(PPWORD_TEMPALTGRP, 0, buf);
			GetSystemTime(&sys_time);
			grp_name.Printf("%s %ld%ld%ld%ld%ld%ld%ld", buf.cptr(), sys_time.wYear, sys_time.wMonth,
				sys_time.wDay, sys_time.wHour, sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds);
			STRNSCPY(grec.Name, grp_name);
			THROW(g_obj.P_Tbl->Update(&grp_id, &grec, 0));
			THROW(tmpf.WriteToProp(PPOBJ_GOODSGROUP, grp_id, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_));
			THROW(tra.Commit());
		}
		ASSIGN_PTR(pGrpID, grp_id);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsGroup::SetDynamicOwner(PPID id, long curOwner, long newOwner)
{
	int    ok = -1;
	PPObjGoodsGroup gg_obj;
	if(id && gg_obj.CheckFlag(id, GF_DYNAMICTEMPALTGRP)) {
		PPTransaction tra(-1);
		THROW(tra);
		if(gg_obj.Search(id, 0) > 0) {
			const long cur_owner = gg_obj.P_Tbl->data.ManufID; // as an owner
			if(!cur_owner || cur_owner == curOwner) {
				gg_obj.P_Tbl->data.ManufID = newOwner;
				gg_obj.P_Tbl->updateRec();
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
	ENDCATCH
	return ok;
}

/*static*/int PPObjGoodsGroup::RemoveDynamicAlt(PPID id, long owner, int forceDel /*=0*/, int useTa /*=1*/)
{
	int    ok = -1;
	PPObjGoodsGroup gg_obj;
	Goods2Tbl::Rec rec;
	if(id && gg_obj.Search(id, &rec) > 0 && (rec.Flags & GF_DYNAMICTEMPALTGRP) == GF_DYNAMICTEMPALTGRP) {
		if(forceDel || rec.ManufID == owner) {
			uint options = PPObject::not_checkrights|PPObject::not_addtolog|PPObject::not_objnotify;
			SETFLAG(options, PPObject::use_transaction, useTa);
			ok = gg_obj.RemoveObjV(id, 0, options, 0);
		}
	}
	return ok;
}
//
// @vmiller {
// @ModuleDef(PPObjSuprWare)
//
struct _GCompItem {       // @persistent @store(ObjAssocTbl)
	PPID   ID;            //
	PPID   AsscType;      // Const=PPASS_GOODSCOMP
	PPID   GoodsID;       // ИД товара
	PPID   CompID;        // ИД компонента
	long   InnerNum;      // Внутренний номер (не используется)
	double Qtty;          // Количество компонента в товаре
	PPID   UnitID;        // ИД единицы измерени
	char   Reserve[52];   //
};

PPSuprWare::PPSuprWare()
{
	THISZERO();
}

int FASTCALL PPSuprWare::IsEq(const PPSuprWare & rS) const
{
	if(SuprWareType != rS.SuprWareType)
		return 0;
	else if(SuprWareCat != rS.SuprWareCat)
		return 0;
	else if(ParentID != rS.ParentID)
		return 0;
	else if(Flags != rS.Flags)
		return 0;
	else if(!sstreq(Name, rS.Name))
		return 0;
	else if(!sstreq(Code, rS.Code))
		return 0;
	return 1;
}

PPSuprWareAssoc::PPSuprWareAssoc()
{
	THISZERO();
}

PPSuprWareAssoc & FASTCALL PPSuprWareAssoc::operator = (const PPSuprWareAssoc & rS)
{
	if(this != &rS) {
		memcpy(this, &rS, sizeof(*this));
	}
	return *this;
}

PPSuprWareAssoc & FASTCALL PPSuprWareAssoc::operator = (const ObjAssocTbl::Rec & rS)
{
	STATIC_ASSERT(sizeof(ObjAssocTbl::Rec) == sizeof(_GCompItem));
	const _GCompItem & r_s = *reinterpret_cast<const _GCompItem *>(&rS);
	GoodsID = r_s.GoodsID;
	CompID  = r_s.CompID;
	TypeID  = 0;
	Num     = r_s.InnerNum;
	UnitID  = r_s.UnitID;
	Qtty    = r_s.Qtty;
	return *this;
}

PPSuprWarePacket::PPSuprWarePacket()
{
	Init();
}

PPSuprWarePacket & FASTCALL PPSuprWarePacket::operator = (const PPSuprWarePacket & rSrc)
{
	Rec = rSrc.Rec;
	Items = rSrc.Items;
	return *this;
}

void PPSuprWarePacket::Init()
{
	MEMSZERO(Rec);
	Items.clear();
}
//
//
//
PPObjSuprWare::PPObjSuprWare(void * extraPtr) : PPObjGoods(PPOBJ_COMPGOODS, PPGDSK_SUPRWARE, extraPtr)
{
}

int PPObjSuprWare::Get(PPID id, PPSuprWarePacket * pPack)
{
	int    ok = -1, r;
	uint   i;
	Goods2Tbl::Rec goods_rec;
	THROW(r = PPObjGoods::Search(id, &goods_rec));
	THROW_PP_S(goods_rec.Kind == PPGDSK_SUPRWARE, PPERR_INVSUPRWARERECKIND, id);
	if(r > 0) {
		ok = 1;
		if(pPack) {
			pPack->Init();

			BarcodeArray bc_list;
			pPack->Rec.ID = goods_rec.ID;
			pPack->Rec.SuprWareType = goods_rec.GoodsTypeID;
			pPack->Rec.SuprWareCat  = goods_rec.WrOffGrpID;
			pPack->Rec.ParentID     = goods_rec.ParentID;
			pPack->Rec.Flags        = goods_rec.Flags;
			STRNSCPY(pPack->Rec.Name, goods_rec.Name);
			P_Tbl->ReadBarcodes(id, bc_list);
			for(i = 0; i < bc_list.getCount(); i++) {
				BarcodeTbl::Rec & r_bc_rec = bc_list.at(i);
				if(r_bc_rec.Code[0] == '~') {
					STRNSCPY(pPack->Rec.Code, r_bc_rec.Code+1);
					break;
				}
			}
			{
				PPSuprWareAssoc goods_comp;
				TSVector <ObjAssocTbl::Rec> items_list;
				THROW(PPRef->Assc.GetItemsListByPrmr(PPASS_GOODSCOMP, id, &items_list));
				for(i = 0; i < items_list.getCount(); i++) {
					const ObjAssocTbl::Rec & r_assc = items_list.at(i);
					goods_comp = r_assc;
					if(Fetch(goods_comp.GoodsID, &goods_rec) > 0)
						goods_comp.TypeID = goods_rec.WrOffGrpID;
					THROW_SL(pPack->Items.insert(&goods_comp));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjSuprWare::MakeStorage(PPID id, const PPSuprWare * pRec, Goods2Tbl::Rec * pRawRec, BarcodeArray * pBcList)
{
	int    ok = 1;
	memzero(pRawRec, sizeof(*pRawRec));
	pRawRec->ID = id;
	pRawRec->Kind = PPGDSK_SUPRWARE;
	pRawRec->GoodsTypeID = pRec->SuprWareType;
	STRNSCPY(pRawRec->Name, pRec->Name);
	pRawRec->WrOffGrpID = pRec->SuprWareCat;
	pRawRec->ParentID = pRec->ParentID;
	pRawRec->Flags = pRec->Flags;
	if(pBcList) {
		pBcList->freeAll();
		SString temp_buf(pRec->Code);
		if(temp_buf.NotEmptyS()) {
			BarcodeTbl::Rec bc_rec;
			bc_rec.GoodsID = id;
			bc_rec.Qtty = 1.0;
			temp_buf.PadLeft(1, '~').CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
			pBcList->insert(&bc_rec);
		}
	}
	return ok;
}

int PPObjSuprWare::Put(PPID * pID, const PPSuprWarePacket * pPack, int use_ta)
{
	STATIC_ASSERT(sizeof(_GCompItem)==sizeof(ObjAssocTbl::Rec));
	const  PPID assoc_type = PPASS_GOODSCOMP;
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    action = 0;
	int    skip_items = 0;
	uint   i;
	Goods2Tbl::Rec raw_rec;
	PPSuprWarePacket org_pack;
	BarcodeArray bc_list;
	SArray items(sizeof(ObjAssocTbl::Rec));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			for(i = 0; i < pPack->Items.getCount(); i++) {
				const PPSuprWareAssoc & r_item = pPack->Items.at(i);
				_GCompItem gci;
				MEMSZERO(gci);
				gci.AsscType = assoc_type;
				gci.GoodsID  = r_item.GoodsID;
				gci.CompID   = r_item.CompID;
				gci.InnerNum = r_item.Num;
				gci.UnitID   = r_item.UnitID;
				gci.Qtty     = r_item.Qtty;
				THROW_SL(items.insert(&gci));
			}
		}
		if(*pID) {
			THROW(Get(*pID, &org_pack) > 0);
			if(pPack) {
				if(!pPack->Rec.IsEq(org_pack.Rec)) {
					THROW(MakeStorage(*pID, &pPack->Rec, &raw_rec, &bc_list));
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
					action = PPACN_OBJUPD;
					ok = 1;
				}
			}
			else {
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(P_Tbl->UpdateBarcodes(*pID, 0, 0));
				action = PPACN_OBJRMV;
				ok = 1;
			}
		}
		else if(pPack) {
			THROW(MakeStorage(0, &pPack->Rec, &raw_rec, 0));
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			//
			// Теперь знаем идентификатор и можно сохранить список кодов
			//
			THROW(MakeStorage(*pID, &pPack->Rec, &raw_rec, &bc_list));
			THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
			action = PPACN_OBJADD;
			ok = 1;
		}
		if(!skip_items) {
			if(items.getCount() == 0) {
				THROW(p_ref->Assc.Remove(assoc_type, *pID, 0, 0));
				ok = 1;
			}
			else {
				long   last_num = 0;
				_GCompItem gci;
				SArray prev_items(sizeof(ObjAssocTbl::Rec));
				LongArray found_pos_list;
				for(SEnum en = p_ref->Assc.Enum(assoc_type, *pID, 0); en.Next(&gci) > 0;) {
					THROW_SL(prev_items.insert(&gci));
					SETMAX(last_num, gci.InnerNum);
				}
				for(i = 0; i < prev_items.getCount(); i++) {
					uint pos = 0;
					gci = *static_cast<const _GCompItem *>(prev_items.at(i));
					if(items.lsearch(&gci.CompID, &pos, CMPF_LONG, offsetof(_GCompItem, CompID))) {
						_GCompItem * p_list_item = static_cast<_GCompItem *>(items.at(pos));
						found_pos_list.add(static_cast<long>(pos));
						if(p_list_item->Qtty != gci.Qtty || p_list_item->UnitID != gci.UnitID) {
							gci.Qtty = p_list_item->Qtty;
							gci.UnitID = p_list_item->UnitID;
							THROW(p_ref->Assc.Update(gci.ID, (ObjAssocTbl::Rec *)&gci, 0));
							ok = 1;
						}
					}
					else {
						if(gci.InnerNum == last_num)
							last_num--;
						THROW(p_ref->Assc.Remove(gci.ID, 0));
						ok = 1;
					}
				}
				{
					BExtInsert bei(&p_ref->Assc);
					for(i = 0; i < items.getCount(); i++) {
						if(!found_pos_list.lsearch((long)i)) {
							_GCompItem * p_list_item = static_cast<_GCompItem *>(items.at(i));
							p_list_item->AsscType = assoc_type;
							p_list_item->GoodsID = *pID;
							p_list_item->InnerNum = ++last_num;
							THROW_DB(bei.insert(p_list_item));
							ok = 1;
						}
					}
					THROW_DB(bei.flash());
				}
			}
		}
		if(action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjSuprWare::PutAssoc(const PPSuprWareAssoc & rItem, int use_ta)
{
	STATIC_ASSERT(sizeof(_GCompItem)==sizeof(ObjAssocTbl::Rec));
	const  PPID assoc_type = PPASS_GOODSCOMP;
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   assc_id = 0;
	{
		_GCompItem gci, org_gci;
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(gci);
		gci.AsscType = assoc_type;
		gci.GoodsID  = rItem.GoodsID;
		gci.CompID   = rItem.CompID;
		gci.InnerNum = rItem.Num;
		gci.UnitID   = rItem.UnitID;
		gci.Qtty     = rItem.Qtty;
		if(p_ref->Assc.Search(assoc_type, rItem.GoodsID, rItem.CompID, reinterpret_cast<ObjAssocTbl::Rec *>(&org_gci)) > 0) {
			if(gci.Qtty != org_gci.Qtty || gci.UnitID != org_gci.UnitID) {
				org_gci.Qtty = gci.Qtty;
				org_gci.UnitID = gci.UnitID;
				THROW(p_ref->Assc.Update(org_gci.ID, reinterpret_cast<ObjAssocTbl::Rec *>(&org_gci), 0));
				ok = 2;
			}
			else
				ok = -1;
		}
		else {
			THROW(p_ref->Assc.SearchFreeNum(assoc_type, gci.GoodsID, &gci.InnerNum, 0));
			THROW(p_ref->Assc.Add(&assc_id, reinterpret_cast<ObjAssocTbl::Rec *>(&gci), 0));
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjSuprWare::SearchByBarcode(const char * pBarcode, BarcodeTbl::Rec * pBcRec)
{
	int    ok = -1;
	if(!isempty(pBarcode)) {
		char   bar_code[32];
		if(*pBarcode != '~') {
			bar_code[0] = '~';
			strnzcpy(bar_code + 1, pBarcode, sizeof(bar_code) - 1);
		}
		else
			strnzcpy(bar_code, pBarcode, sizeof(bar_code));
		strip(bar_code+1);
		ok = P_Tbl->SearchByBarcode(bar_code, pBcRec);
	}
	return ok;
}

int PPObjSuprWare::GetListByComponent(PPID componentID, PPIDArray & rList)
{
	const  PPID assoc_type = PPASS_GOODSCOMP;
	int    ok = -1;
	rList.clear();
	_GCompItem rec;
	for(SEnum en = PPRef->Assc.Enum(assoc_type, componentID, 1); en.Next(&rec) > 0;) {
		rList.add(rec.GoodsID);
	}
	if(rList.getCount()) {
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}
//
// Диалог, показывающий список компонентов
//
class SuprWareListDialog : public PPListDialog {
public:
	explicit SuprWareListDialog(PPSuprWarePacket * pCompGdsPack) : PPListDialog(DLG_COMPGDSLST, CTL_COMPGDSLST_LIST), P_SuprWarePack(*pCompGdsPack)
	{
		updateList(-1);
	}
private:
	virtual int  setupList();
	PPSuprWarePacket P_SuprWarePack;
};

int SuprWareListDialog::setupList()
{
	PPObjGoods goods_o;
	StringSet ss(SLBColumnDelim);
	SString str;
	for(uint i = 0; i < P_SuprWarePack.Items.getCount(); i++) {
		Goods2Tbl::Rec goods_rec;
		if(goods_o.Search(P_SuprWarePack.Items.at(i).CompID, &goods_rec) > 0) {
			ss.Z();
			ss.add(goods_rec.Name);
			if(P_SuprWarePack.Items.at(i).Qtty || P_SuprWarePack.Items.at(i).UnitID) {
				str.Z().Cat(P_SuprWarePack.Items.at(i).Qtty);
				ss.add(str);
				PPUnit unit_rec;
				if(goods_o.FetchUnit(P_SuprWarePack.Items.at(i).UnitID, &unit_rec))
					ss.add(unit_rec.Name);
			}
			if(!addStringToList(i+1, ss.getBuf()))
				return 0;
		}
	}
	return 1;
}

// Не редактирует запись. Просто показывает таблицу с содержанием товара
int PPObjSuprWare::EditList(PPID * pID)
{
	int    ok = -1;
	PPSuprWarePacket pack;
	pack.Init();
	THROW(Get(*pID, &pack));
	SuprWareListDialog * dlg = new SuprWareListDialog(&pack);
	if(CheckDialogPtrErr(&dlg))
		ok = (ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1;
	else
		ok = 0;
	CATCHZOKPPERR
	return ok;
}

class SuprWareDialog : public TDialog {
	DECL_DIALOG_DATA(PPSuprWarePacket);
public:
	SuprWareDialog() : TDialog(DLG_SUPRWARE)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		setCtrlData(CTL_SUPRWARE_NAME, Data.Rec.Name);
		setCtrlData(CTL_SUPRWARE_CODE, Data.Rec.Code);
		setCtrlLong(CTL_SUPRWARE_ID, Data.Rec.ID);
		AddClusterAssoc(CTL_SUPRWARE_TYPE, 0, SUPRWARETYPE_GOODS);
		AddClusterAssoc(CTL_SUPRWARE_TYPE, 1, SUPRWARETYPE_COMPONENT);
		SetClusterData(CTL_SUPRWARE_TYPE, Data.Rec.SuprWareType);
		SetupStringCombo(this, CTLSEL_SUPRWARE_CAT, PPTXT_COMPGDS_TYPES, Data.Rec.SuprWareCat);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_SUPRWARE_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_SUPRWARE_CODE, Data.Rec.Code);
		GetClusterData(sel = CTL_SUPRWARE_TYPE, &Data.Rec.SuprWareType);
		getCtrlData(CTLSEL_SUPRWARE_CAT, &Data.Rec.SuprWareCat);
		THROW(oneof2(Data.Rec.SuprWareType, SUPRWARETYPE_GOODS, SUPRWARETYPE_COMPONENT));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
};

int PPObjSuprWare::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	PPSuprWarePacket pack;
	if(*pID) {
		THROW(Get(*pID, &pack) > 0);
	}
	else {
		pack.Rec.SuprWareType = SUPRWARETYPE_GOODS;
	}
	while(PPDialogProcBody <SuprWareDialog, PPSuprWarePacket> (&pack) > 0) {
		THROW(Put(pID, &pack, 1));
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjSuprWare::DeleteObj(PPID id)
{
	return Put(&id, 0, 0);
}
//
// } @vmiller
//
PPViewBrand::BrwItem::BrwItem(const PPBrand * pS) : ID(0), OwnerID(0), Flags(0), ViewFlags(0), LinkGoodsCount(0)
{
	if(pS) {
		ID = pS->ID;
		OwnerID = pS->OwnerID;
		Flags = pS->Flags;
		STRNSCPY(Name, pS->Name);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		GetPersonName(OwnerID, r_temp_buf);
		STRNSCPY(OwnerName, r_temp_buf);
	}
	else {
		Name[0] = 0;
		OwnerName[0] = 0;
	}
}

PPViewBrand::PPViewBrand() : PPView(&Obj, &Filt, PPVIEW_BRAND, (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter), 0), P_DsList(0)
{
}

PPViewBrand::~PPViewBrand()
{
	ZDELETE(P_DsList);
}

int PPViewBrand::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

class BrandFiltDialog : public TDialog {
	DECL_DIALOG_DATA(BrandFilt);
public:
	enum {
		ctlgroupBrandOwner = 1
	};
	BrandFiltDialog() : TDialog(DLG_BRANDFILT)
	{
		addGroup(ctlgroupBrandOwner, new PersonListCtrlGroup(CTLSEL_BRANDFILT_OWNER, 0, cmBrandOwnerList, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		PersonListCtrlGroup::Rec brandowner_grp_rec(PPPRK_MANUF, Data.OwnerList.IsExists() ? &Data.OwnerList.Get() : 0);
		setGroupData(ctlgroupBrandOwner, &brandowner_grp_rec);
		AddClusterAssoc(CTL_BRANDFILT_FLAGS, 0, BrandFilt::fShowGoodsCount);
		SetClusterData(CTL_BRANDFILT_FLAGS, Data.Flags);
		setCtrlString(CTL_BRANDFILT_NAMESTR, Data.SrchStr);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetClusterData(CTL_BRANDFILT_FLAGS, &Data.Flags);
		{
			PersonListCtrlGroup::Rec brandowner_grp_rec;
			getGroupData(ctlgroupBrandOwner, &brandowner_grp_rec);
			Data.OwnerList.Set(brandowner_grp_rec.List.getCount() ? &brandowner_grp_rec.List : 0);
		}
		getCtrlString(CTL_BRANDFILT_NAMESTR, Data.SrchStr);
		if(Data.SrchStr.NotEmpty())
			Data.Flags |= Data.fSubName;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int PPViewBrand::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	BrandFilt * p_filt = static_cast<BrandFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(BrandFiltDialog, p_filt);
}

/*virtual*/void PPViewBrand::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_BRANDTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long count = 0;
		BrandViewItem item;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWaitStop();
		dlg->setCtrlLong(CTL_BRANDTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewBrand::InitIteration()
{
	return MakeList();
}

int FASTCALL PPViewBrand::NextIteration(BrandViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_DsList && P_DsList->testPointer()) {
		const  PPID id = static_cast<const BrwItem *>(P_DsList->at(P_DsList->getPointer()))->ID;
		PPBrandPacket pack;
		if(Obj.Get(id, &pack) > 0) {
			ASSIGN_PTR(pItem, pack.Rec);
			P_DsList->incPointer();
			ok = 1;
		}
	}
	return ok;
}

int PPViewBrand::MakeList()
{
	int    ok = 1;
	PPBrand item;
	GoodsCore * p_tbl = Obj.P_Tbl;
	PPIDArray result_list;
	Goods2Tbl::Key2 k2;
	const  PPID single_owner_id = Filt.OwnerList.GetSingle();
	PPObjGoods goods_obj;
	PPIDArray single_brand_list;
	PPIDArray goods_list;
	BExtQuery q(p_tbl, 2);
	DBQ * dbq = &(p_tbl->Kind == PPGDSK_BRAND);
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	dbq = ppcheckfiltid(dbq, p_tbl->ManufID, single_owner_id);
	q.select(p_tbl->ID, p_tbl->Name, p_tbl->GoodsTypeID, p_tbl->ManufID, p_tbl->Flags, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Kind = PPGDSK_BRAND;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		PPBrand rec;
		if(Obj.Helper_GetRec(p_tbl->data, &rec) && rec.CheckForFilt(&Filt)) {
			BrwItem new_item(&rec);
			if(Filt.Flags & Filt.fShowGoodsCount) {
				single_brand_list.clear();
				single_brand_list.add(rec.ID);
				goods_obj.P_Tbl->GetListByBrandList(single_brand_list, goods_list);
				new_item.LinkGoodsCount = goods_list.getCount();
			}
			THROW_SL(P_DsList->insert(&new_item));
		}
	}
	CATCHZOK
	CALLPTRMEMB(P_DsList, setPointer(0));
	return ok;
}

int PPViewBrand::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: pBlk->Set(p_item->Name); break; // @name
			case 2: pBlk->Set(p_item->OwnerName); break;
			case 3: pBlk->Set(static_cast<int32>(p_item->LinkGoodsCount)); break;
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewBrand::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewBrand * p_v = static_cast<PPViewBrand *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

static int PPViewBrand_CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewBrand * p_view = static_cast<PPViewBrand *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewBrand::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				const BrwItem * p_item = static_cast<const BrwItem *>(pData);
				if(p_item->Flags & GF_DERIVED_HASIMAGES) {
					pCellStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					pCellStyle->Color2 = GetColorRef(SClrGreen);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

void PPViewBrand::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewBrand::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(PPViewBrand_CellStyleFunc, pBrw);
		if(Filt.Flags & Filt.fShowGoodsCount) {
			pBrw->InsColumn(-1, "@plucount", 3, MKSTYPE(S_UINT, 4), MKSFMT(10, ALIGN_RIGHT), BCO_USERPROC);
		}
	}
}

SArray * PPViewBrand::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	PPBrand ds_item;
	THROW(MakeList());
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_BRAND);
	return p_array;
}

int PPViewBrand::OnExecBrowser(PPViewBrowser *)
{
	return -1;
}

int PPViewBrand::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	// @v12.1.10 id и preserve_id перенесены наверх дабы правильно спозиционировать курсор после изменения данных общими механизмами PPView
	PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	const  PPID preserve_id = id;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_VIEWGOODS:
				ok = -1;
				if(id) {
					GoodsFilt filt;
					filt.BrandList.Add(id);
					PPView::Execute(PPVIEW_GOODS, &filt, 1, 0);
				}
				break;
			case PPVCMD_UNITEOBJ:
				if(id && PPObject::ReplaceObjInteractive(Obj.Obj, id) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_MOUSEHOVER:
				if(id && static_cast<const BrwItem *>(pHdr)->Flags & GF_DERIVED_HASIMAGES) {
					SString img_path;
					ObjLinkFiles link_files(PPOBJ_BRAND);
					link_files.Load(id, 0L);
					link_files.At(0, img_path);
					PPTooltipMessage(0, img_path, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
						SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
				}
				break;
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0) {
		MakeList();
		if(pBrw) {
			AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_id > 0 && P_DsList->lsearch(&preserve_id, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}
