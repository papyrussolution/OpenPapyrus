// OBJGGRP.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjGoodsGroup)
//
SLAPI GoodsGroupRecoverParam::GoodsGroupRecoverParam() : EgaFolderID(0), Ega(egaNone), Flags(0)
{
}

SLAPI PPObjGoodsGroup::PPObjGoodsGroup(void * extraPtr) : PPObjGoods(PPOBJ_GOODSGROUP, extraPtr)
{
	Kind = PPGDSK_GROUP;
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

// static
int SLAPI PPObjGoodsGroup::IsAlt(PPID id)
{
	if(id) {
		PPObjGoodsGroup ggobj;
		return ggobj.IsAltGroup(id);
	}
	return -1;
}

// static
int SLAPI PPObjGoodsGroup::IsTempAlt(PPID id)
{
	if(id) {
		PPObjGoodsGroup ggobj;
		return ggobj.IsTempAltGroup(id);
	}
	return -1;
}

// static
int SLAPI PPObjGoodsGroup::IsDynamicAlt(PPID id)
{
	if(id) {
		PPObjGoodsGroup ggobj;
		return ggobj.IsDynamicAltGroup(id);
	}
	return -1;
}

int SLAPI PPObjGoodsGroup::SearchCode(const char * pCode, BarcodeTbl::Rec * pRec)
{
	char   bar_code[32];
	bar_code[0] = '@';
	strnzcpy(bar_code + 1, pCode, sizeof(bar_code) - 1);
	strip(bar_code+1);
	return P_Tbl->SearchByBarcode(bar_code, pRec);
}

int SLAPI PPObjGoodsGroup::GetLevel(PPID grpID, long * pLevel)
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

int SLAPI PPObjGoodsGroup::CalcTotal(GoodsGroupTotal * pTotal)
{
	GoodsGroupTotal total;
	MEMSZERO(total);
	PPIDArray id_list;
	Goods2Tbl::Key1 k1;
	BExtQuery q(P_Tbl, 1);
	MEMSZERO(k1);
	k1.Kind = PPGDSK_GROUP;
	q.select(P_Tbl->ID, P_Tbl->Flags, P_Tbl->ParentID, 0L).where(P_Tbl->Kind == PPGDSK_GROUP);
	for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
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
		if(level > total.MaxLevel)
			total.MaxLevel = level;
	}
	ASSIGN_PTR(pTotal, total);
	return 1;
}

//virtual
int SLAPI PPObjGoodsGroup::MakeReserved(long flags)
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

//int SLAPI PPObjGoodsGroup::Remove(PPID id, long extraData, uint flags /* = user_request | use_transaction */)
//virtual
int  SLAPI PPObjGoodsGroup::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    ok = -1;
	int    user_request = BIN(options & PPObject::user_request);
	int    r = (user_request) ? CONFIRM(PPCFM_DELGOODSGROUP) : 1;
	SETFLAG(options, PPObject::user_request, 0);
	ok = r ? PPObject::RemoveObjV(id, pObjColl, options, pExtraParam) : -1;
	if(!ok && user_request)
		PPError();
	return ok;
}

int SLAPI PPObjGoodsGroup::DeleteObj(PPID id)
{
	int    ok = 1, r;
	PPID   branch_id = 0;
	Goods2Tbl::Rec rec;
	// @v9.4.9 {
	if(id) {
		PPGoodsConfig cfg;
		if(PPObjGoods::ReadConfig(&cfg) > 0) {
			THROW_PP(cfg.AssetGrpID != id, PPERR_GGRPHASREFINGCFG);
			THROW_PP(cfg.DefGroupID != id, PPERR_GGRPHASREFINGCFG);
		}
	}
	// } @v9.4.9
	while(ok > 0 && (r = P_Tbl->SearchAnyRef(PPOBJ_GOODSGROUP, id, &branch_id)) > 0) {
		if(Search(branch_id, &rec) > 0) {
			//
			// Рекурсивный вызов для подуровней. Вызываем
			// метод PPObject::Remove без транзакции и предупреждения //
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

int SLAPI PPObjGoodsGroup::Transmit()
{
	int    ok = -1;
	PPIDArray id_list;
	ObjTransmitParam param;
	PPGoodsConfig cfg;
	const int transmit_alt_grp = BIN(P_Tbl->FetchConfig(&cfg) > 0 && cfg.Flags & GCF_XCHG_SENDALTGROUP);
	{
		BExtQuery q(P_Tbl, 1);
		DBQ  * dbq = 0;
		Goods2Tbl::Key1 k;
		MEMSZERO(k);
		k.Kind = PPGDSK_GROUP;
		k.ParentID = 0;
		dbq = & (P_Tbl->Kind == PPGDSK_GROUP);
		q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->Flags, 0L).where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			const long _gf = P_Tbl->data.Flags;
			if((!(_gf & GF_ALTGROUP) || (transmit_alt_grp && !(_gf & GF_DYNAMIC))) && !(_gf & GF_FOLDER))
				id_list.add(P_Tbl->data.ID);
		}
	}
	if(id_list.getCount() && ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWait(1);
		THROW(objid_ary.Add(PPOBJ_GOODSGROUP, id_list));
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

static int SLAPI EditGoodsGroupRecoverParam(GoodsGroupRecoverParam * pData)
{
	class RcvrGoodsGroupsDialog : public TDialog {
	public:
		RcvrGoodsGroupsDialog() : TDialog(DLG_RCVRGGRP)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_RCVRGGRP_LOG, CTL_RCVRGGRP_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		}
		int setDTS(const GoodsGroupRecoverParam * pData)
		{
			int    ok = 1;
			Data = *pData;
			setCtrlString(CTL_RCVRGGRP_LOG, Data.LogFileName);
			AddClusterAssoc(CTL_RCVRGGRP_FLAGS, 0, GoodsGroupRecoverParam::fCorrect);
			AddClusterAssoc(CTL_RCVRGGRP_FLAGS, 1, GoodsGroupRecoverParam::fDelTempAltGrp);
			SetClusterData(CTL_RCVRGGRP_FLAGS, Data.Flags);
			AddClusterAssocDef(CTL_RCVRGGRP_EGA, 0, GoodsGroupRecoverParam::egaNone);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 1, GoodsGroupRecoverParam::egaReport);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 2, GoodsGroupRecoverParam::egaMoveToFolder);
			AddClusterAssoc(CTL_RCVRGGRP_EGA, 3, GoodsGroupRecoverParam::egaRemove);
			SetClusterData(CTL_RCVRGGRP_FLAGS, Data.Ega);
			SetupPPObjCombo(this, CTLSEL_RCVRGGRP_EGAFOLD, PPOBJ_GOODSGROUP, Data.EgaFolderID, 0, (void *)GGRTYP_SEL_FOLDER);
			disableCtrl(CTLSEL_RCVRGGRP_EGAFOLD, Data.Ega != GoodsGroupRecoverParam::egaMoveToFolder);
			return ok;
		}
		int getDTS(GoodsGroupRecoverParam * pData)
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
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
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
		GoodsGroupRecoverParam Data;
		PPObjGoods GObj;
	};
	DIALOG_PROC_BODY(RcvrGoodsGroupsDialog, pData);
}

int SLAPI PPObjGoodsGroup::Recover(const GoodsGroupRecoverParam * pParam, PPLogger * pLogger)
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
		ObjCollection obj_coll;
		if(del_temp_alt)
			obj_coll.CreateFullList(gotlfExcludeDyn|gotlfExcludeObjBill|gotlfExcludeObsolete);
		PPWait(1);
		PPTransaction tra(BIN(param.Flags & (GoodsGroupRecoverParam::fCorrect|GoodsGroupRecoverParam::fDelTempAltGrp)));
		THROW(tra);
		{
			PPIDArray temp_dyn_list;
			PPIDArray temp_list;
			SString grp_name;
			Goods2Tbl ggrp_tbl;
			BExtQuery q(&ggrp_tbl, 1);
			MEMSZERO(k);
			if(del_temp_alt)
				pLogger->LogString(PPTXT_LOG_DELTEMPALTGRP, 0);
			else
				pLogger->LogString(PPTXT_LOG_TEMPALTGROUPS, 0);
			k.Kind = PPGDSK_GROUP;
			k.ParentID = 0;
			q.select(ggrp_tbl.ID, ggrp_tbl.Name, 0L).where(ggrp_tbl.Kind == PPGDSK_GROUP);
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
				PPID    grp_id = ggrp_tbl.data.ID;
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
					const PPID grp_id = temp_list.get(i);
					if(grp_id && PPObjGoodsGroup::IsTempAlt(grp_id) > 0) {
						THROW(RemoveObjV(grp_id, &obj_coll, PPObject::not_checkrights|PPObject::not_addtolog|PPObject::no_wait_indicator, 0));
					}
					PPWaitPercent(i+1, tlc+tdlc, msg_buf);
				}
				for(i = 0; i < tdlc; i++) {
					const PPID grp_id = temp_dyn_list.get(i);
					if(grp_id && CheckFlag(grp_id, GF_DYNAMICTEMPALTGRP)) {
						THROW(RemoveObjV(grp_id, &obj_coll, PPObject::not_checkrights|PPObject::not_addtolog|PPObject::not_objnotify|PPObject::no_wait_indicator, 0));
					}
					PPWaitPercent(tlc+i+1, tdlc, msg_buf);
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
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
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
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
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
		PPWait(0);
	}
	else
		ok = -1;
	CATCHZOKPPERR
	if(!outer_logger)
		pLogger->Save(param.LogFileName, 0);
	return ok;
}

int SLAPI RecoverGoodsGroups(const GoodsGroupRecoverParam * pParam)
{
	PPObjGoodsGroup gg_obj;
	return gg_obj.Recover(pParam, 0);
}

int SLAPI RecoverGoodsGroupsNIA()
{
	GoodsGroupRecoverParam param;
	PPGetFileName(PPFILNAM_ERR_LOG, param.LogFileName);
	param.Flags |= GoodsGroupRecoverParam::fCorrect;
	PPObjGoodsGroup gg_obj;
	return gg_obj.Recover(&param, 0);
}

int SLAPI PPObjGoodsGroup::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->valid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		StdTreeListBoxDef * p_def = static_cast<StdTreeListBoxDef *>(pDef);
		p_def->ClearImageAssocList();
		if(p_def->getIdList(list) > 0) {
			Goods2Tbl::Rec ggrec;
			MEMSZERO(ggrec);
			for(uint i = 0; i < list.getCount(); i++) {
				PPID id = list.at(i);
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

// virtual
ListBoxDef * SLAPI PPObjGoodsGroup::Selector(void * extraPtr)
{
	ListBoxDef * p_def = PPObject::Selector(extraPtr);
	AssignImages(p_def);
	return p_def;
}

// virtual
int SLAPI PPObjGoodsGroup::UpdateSelector(ListBoxDef * pDef, void * extraPtr)
{
	int    ok = PPObject::UpdateSelector(pDef, extraPtr);
	if(ok > 0)
		AssignImages(pDef);
	return ok;
}

StrAssocArray * PPObjGoodsGroup::MakeStrAssocList(void * extraPtr)
{
	long   parent = reinterpret_cast<long>(extraPtr);
	StrAssocArray * p_list = new StrAssocArray();
	int    alt = 0, asset = 0, excl_asset = 0, fold_only = 0;
	Goods2Tbl::Key1 k;
	BExtQuery q(P_Tbl, 1);
	DBQ  * dbq = 0;
	THROW_MEM(p_list);
	if(parent >= GGRTYP_SEL_FOLDER) {
		fold_only = 1;
		parent -= GGRTYP_SEL_FOLDER;
	}
	else {
		if(parent >= GGRTYP_SEL_EXCLASSET) {
			excl_asset = 1;
			parent -= GGRTYP_SEL_EXCLASSET;
		}
		if(parent >= GGRTYP_SEL_ASSET) {
			asset = 1;
			parent -= GGRTYP_SEL_ASSET;
		}
		if(parent >= GGRTYP_SEL_NORMAL) {
			alt     = -1;
			parent -= GGRTYP_SEL_NORMAL;
		}
		else if(parent >= GGRTYP_SEL_ALT) {
			alt     = 1;
			parent -= GGRTYP_SEL_ALT;
		}
	}
	MEMSZERO(k);
	k.Kind = PPGDSK_GROUP;
	dbq = & (P_Tbl->Kind == PPGDSK_GROUP);
	q.select(P_Tbl->ID, P_Tbl->ParentID, P_Tbl->Name, P_Tbl->Flags, P_Tbl->GoodsTypeID, 0L).where(*dbq);
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
		const int is_alt  = BIN(P_Tbl->data.Flags & GF_ALTGROUP);
		const int is_fold = BIN(P_Tbl->data.Flags & GF_FOLDER);
#ifdef _DEBUG
		//
		// Под отладчиком нам нужна возможность видеть динамические альт группы
		//
		const int is_tempalt = 0;
#else
		const int is_tempalt = BIN((P_Tbl->data.Flags & GF_TEMPALTGROUP) == GF_TEMPALTGROUP);
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
	p_list->SortByText(); // @v7.4.2
	// @average (очень медленно работает, хоть и дает гарантию нерекурсивности)
	// p_list->RemoveRecursion(0); // @v8.3.2
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
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
	if(id && GGObj.Search(id, &rec) > 0 && (!(rec.Flags & GF_FOLDER) || P_Box->isTreeList())) {
		GoodsFilt flt;
		flt.GrpIDList.Add(id);
		PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
	}
}

void GoodsGroupView::setupButtons()
{
	if(!P_Box->isTreeList()) {
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
	P_Box->setDef(GGObj.Selector(reinterpret_cast<void *>(extra_val)));
	return 1;
}

int GoodsGroupView::addItem(long * pPos, long * pID)
{
	PPID   obj_id = 0;
	PPID   id = 0;
	if(P_Box->isTreeList()) {
		PPID   parent_id = 0;
		Goods2Tbl::Rec rec;
		id = getCurrID();
		if(GGObj.Search(id, &rec) <= 0 || !(rec.Flags & GF_FOLDER)) {
			if(static_cast<StdTreeListBoxDef *>(P_Box->def)->GetParent(id, &parent_id) && GGObj.Search(parent_id, 0) > 0)
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
	int    r = id ? GGObj.Edit(&id, 0) : -1;
	return (r == cmOK) ? 1 : ((r == 0) ? 0 : -1);
}

int GoodsGroupView::delItem(long, long id)
{
	return id ? GGObj.RemoveObjV(id, 0, PPObject::rmv_default, 0) : -1;
}

void GoodsGroupView::updateList(PPID id)
{
	if(P_Box) {
		long   cur = -1;
		ushort v = getCtrlUInt16(CTL_GGVIEW_GGRPTYPE);
		long   groups_type = (v == 1) ? GGRTYP_SEL_NORMAL : ((v == 2) ? GGRTYP_SEL_ALT : 0);
		if(id < 0)
			cur = P_Box->def ? P_Box->def->_curItem() : 0;
		GGObj.UpdateSelector(P_Box->def, reinterpret_cast<void *>(groups_type));
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

IMPL_HANDLE_EVENT(GoodsGroupView)
{
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		PPID   id = getCurrID();
		if(TVCMD == cmLBItemFocused)
			setupButtons();
		else if(event.isClusterClk(CTL_GGVIEW_GGRPTYPE)) {
			updateList(id);
			P_Box->def->top();
			setupButtons();
		}
		else if(TVCMD == cmTransmit) {
			GGObj.Transmit();
		}
		else if(TVCMD == cmPrint)
			Print();
		else if(id) {
			if(TVCMD == cmSysJournalByObj) {
				ViewSysJournal(PPOBJ_GOODSGROUP, id, 1);
			}
			else {
				if(TVCMD == cmaMore)
					ViewGoodsByGroup(id);
				else if(TVCMD == cmQuot)
					editQuotations(id, PPQuot::clsGeneral);
				else if(TVCMD == cmSupplCost)
					editQuotations(id, PPQuot::clsSupplDeal);
				else if(TVCMD == cmGoodsMatrix)
					editQuotations(id, PPQuot::clsMtx);
				else if(TVCMD == cmGoodsMatrixRestrict)
					editQuotations(id, PPQuot::clsMtxRestr);
				else
					return;
				setupButtons();
			}
		}
		else
			return;
	}
	else if(event.isKeyDown(kbF9)) {
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
	else
		return;
	clearEvent(event);
}

int GoodsGroupView::GetGrpType()
{
	return (int)getCtrlUInt16(CTL_GGVIEW_GGRPTYPE);
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
	PView  pv(this);
	return PPAlddPrint(REPORT_GOODSGROUPVIEW, &pv, 0);
}
//
//
//
int SLAPI PPObjGoodsGroup::Browse(void * extraPtr)
{
	ExtraPtr = 0;
	GoodsGroupView * p_dlg = 0;
	if(CheckRights(PPR_READ) && CheckDialogPtr(&(p_dlg = new GoodsGroupView(this)))) {
		ExecViewAndDestroy(p_dlg);
		return 1;
	}
	return PPErrorZ();
}

int SLAPI PPObjGoodsGroup::ReadGoodsFilt(PPID id, GoodsFilt * flt)
{
	return flt->ReadFromProp(PPOBJ_GOODSGROUP, id, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_);
}

int SLAPI PPObjGoodsGroup::Edit(PPID * pID, void * extraPtr /*parentID*/)
{
	const PPID extra_parent_id = reinterpret_cast<PPID>(extraPtr);
	return PPObjGoods::Edit(pID, gpkndUndef, NZOR(extra_parent_id, reinterpret_cast<PPID>(ExtraPtr)), 0, 0);
}

int SLAPI PPObjGoodsGroup::AddSimple(PPID * pID, GoodsPacketKind kind, PPID parentID, const char * pName, const char * pCode, PPID unitID, int use_ta)
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
SLAPI PPObjPckgType::PPObjPckgType(void * extraPtr) : PPObjGoods(PPOBJ_PCKGTYPE, extraPtr)
{
	Kind = PPGDSK_PCKGTYPE;
}

int SLAPI PPObjPckgType::Get(PPID id, PPGdsPckgType * pRec)
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

PPID SLAPI PPObjPckgType::GetSingle()
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

int SLAPI PPObjPckgType::Put(PPID * pID, PPGdsPckgType * pRec, int use_ta)
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

ListBoxDef * SLAPI PPObjPckgType::Selector(void * extraPtr)
{
	return _Selector2(0, 0, PPObjGoods::selfByName, 0, 0, 0);
}

int SLAPI PPObjPckgType::Browse(void * extraPtr)
{
	return CheckRights(PPR_READ) ? SimpleObjView(this, extraPtr) : PPErrorZ();
}

int SLAPI PPObjPckgType::Edit(long * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
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
			int    is_default = BIN(rec.Flags & GF_DFLTPCKGTYPE);
			valid_data = 1;
			ok = cmOK;
			rec.Flags &= ~GF_DFLTPCKGTYPE;
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(Put(pID, &rec, 0));
				{
					PPGoodsConfig goods_cfg;
					int    is_config = (PPObjGoods::ReadConfig(&goods_cfg) > 0);
					PPID   prev_dflt_pt = goods_cfg.DefPckgTypeID;
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

//static
int SLAPI PPObjPckgType::CodeByTemplate(const char * pTempl, long counter, char * pBuf, size_t bufLen)
{
	char   temp_buf[128];
	int    div_list_count = 0;
	int    div_list[32];
	int    i, j;
	long   n;
	char * d = temp_buf;
	const char * p;
	if(pTempl[0] == 0)
		longfmtz(counter, 6, temp_buf, sizeof(temp_buf));
	else {
		for(p = pTempl; *p; p++)
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
SLAPI PPTransportConfig::PPTransportConfig() : Flags(0), OwnerKindID(0), CaptainKindID(0)
{
}

int FASTCALL PPTransportConfig::operator == (const PPTransportConfig & rS) const
{
	return BIN(Flags == rS.Flags && OwnerKindID == rS.OwnerKindID &&
		CaptainKindID == rS.CaptainKindID && NameTemplate.Cmp(rS.NameTemplate, 0) == 0);
}

struct Storage_PPTranspConfig { // @persistent @store(PropertyTbl)
	size_t GetSize() const
	{
		return (sizeof(*this) + ExtStrSize);
	}
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

//static
int FASTCALL PPObjTransport::ReadConfig(PPTransportConfig * pCfg)
{
	const  long prop_cfg_id = PPPRP_TRANSPCFG;
	int    ok = -1, r;
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

//static
int FASTCALL PPObjTransport::WriteConfig(const PPTransportConfig * pCfg, int use_ta)
{
	const  long prop_cfg_id = PPPRP_TRANSPCFG;
	const  long cfg_obj_type = PPCFGOBJ_TRANSP;
	int    ok = 1, is_new = 0, r;
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

//static
int SLAPI PPObjTransport::EditConfig()
{
	int    ok = -1;
	PPTransportConfig cfg, org_cfg;
	TDialog * dlg = new TDialog(DLG_TRANSPCFG);
	if(CheckDialogPtrErr(&dlg)) {
		ReadConfig(&cfg);
		org_cfg = cfg;
		SetupPPObjCombo(dlg, CTLSEL_TRANSPCFG_OWNERK, PPOBJ_PRSNKIND, cfg.OwnerKindID, 0, 0);
		SetupPPObjCombo(dlg, CTLSEL_TRANSPCFG_CAPTK,  PPOBJ_PRSNKIND, cfg.CaptainKindID, 0, 0);
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

int FASTCALL PPTransport::IsEqual(const PPTransport & rS) const
{
#define CMP_FLD(f) if(f != rS.f) return 0;
	CMP_FLD(ID);
	CMP_FLD(TrType);
	CMP_FLD(TrModelID);
	CMP_FLD(OwnerID);
	CMP_FLD(CountryID);
	CMP_FLD(CaptainID);
	CMP_FLD(Capacity);
	CMP_FLD(VanType); // @v10.2.0
	CMP_FLD(Flags); // @v10.2.4
	if(!sstreq(Name, rS.Name))
		return 0;
	if(!sstreq(Code, rS.Code))
		return 0;
	if(!sstreq(TrailerCode, rS.TrailerCode))
		return 0;
	return 1;
#undef CMP_FLD
}

SLAPI PPObjTransport::PPObjTransport(void * extraPtr) : PPObjGoods(PPOBJ_TRANSPORT, extraPtr)
{
	Kind = PPGDSK_TRANSPORT;
}

int SLAPI PPObjTransport::Get(PPID id, PPTransport * pRec)
{
	int    ok = -1, r;
	Goods2Tbl::Rec goods_rec;
	THROW(r = PPObjGoods::Search(id, &goods_rec));
	THROW_PP_S(goods_rec.Kind == PPGDSK_TRANSPORT, PPERR_INVTRANSPORTRECKIND, id);
	if(r > 0) {
		ok = 1;
		if(pRec) {
			BarcodeArray bc_list;
			pRec->ID = goods_rec.ID;
			pRec->TrType = goods_rec.GdsClsID;
			STRNSCPY(pRec->Name, goods_rec.Name);
			pRec->TrModelID = goods_rec.BrandID;
			pRec->OwnerID   = goods_rec.ManufID;
			pRec->CountryID = goods_rec.DefBCodeStrucID;
			pRec->CaptainID = goods_rec.RspnsPersonID;
			pRec->Capacity  = static_cast<long>(goods_rec.PhUPerU);
			pRec->VanType   = goods_rec.VanType; // @v10.2.0
			SETFLAG(pRec->Flags, GF_PASSIV, goods_rec.Flags & GF_PASSIV); // @v10.2.4
			P_Tbl->ReadBarcodes(id, bc_list);
			for(uint i = 0; i < bc_list.getCount(); i++) {
				BarcodeTbl::Rec & r_bc_rec = bc_list.at(i);
				if(r_bc_rec.Code[0] == '^') {
					if(r_bc_rec.Qtty == 1.0) {
						STRNSCPY(pRec->Code, r_bc_rec.Code+1);
					}
					else if(r_bc_rec.Qtty == 2.0) {
						STRNSCPY(pRec->TrailerCode, r_bc_rec.Code+1);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjTransport::MakeStorage(PPID id, const PPTransport * pRec, Goods2Tbl::Rec * pRawRec, BarcodeArray * pBcList)
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
	pRawRec->VanType = pRec->VanType; // @v10.2.0
	SETFLAG(pRawRec->Flags, GF_PASSIV, pRec->Flags & GF_PASSIV); // @v10.2.4
	if(pBcList) {
		BarcodeTbl::Rec bc_rec;
		SString temp_buf;
		pBcList->freeAll();
		//
		// Transport Code
		//
		temp_buf = pRec->Code;
		if(temp_buf.NotEmptyS()) {
			MEMSZERO(bc_rec);
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
			MEMSZERO(bc_rec);
			bc_rec.GoodsID = id;
			bc_rec.Qtty = 2.0;
			temp_buf.PadLeft(1, '^').CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
			pBcList->insert(&bc_rec);
		}
	}
	return ok;
}

int SLAPI PPObjTransport::Put(PPID * pID, const PPTransport * pRec, int use_ta)
{
	int    ok = 1;
	int    action = 0;
	Goods2Tbl::Rec raw_rec;
	PPTransport org_rec;
	BarcodeArray bc_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Get(*pID, &org_rec) > 0);
			if(pRec) {
				if(!pRec->IsEqual(org_rec)) {
					THROW(MakeStorage(*pID, pRec, &raw_rec, &bc_list));
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
					THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
					action = PPACN_OBJUPD;
				}
				else
					ok = -1;
			}
			else {
				THROW(P_Tbl->Update(pID, 0, 0));
				THROW(P_Tbl->UpdateBarcodes(*pID, 0, 0));
			}
		}
		else if(pRec) {
			THROW(MakeStorage(0, pRec, &raw_rec, 0));
			THROW(P_Tbl->Update(pID, &raw_rec, 0));
			//
			// Теперь знаем идентификатор и можно сохранить список кодов
			//
			THROW(MakeStorage(*pID, pRec, &raw_rec, &bc_list));
			THROW(P_Tbl->UpdateBarcodes(*pID, &bc_list, 0));
			action = PPACN_OBJADD;
		}
		if(action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

ListBoxDef * SLAPI PPObjTransport::Selector(void * extraPtr)
{
	return _Selector2(0, 0, PPObjGoods::selfByName, extraPtr, 0, 0);
}

LongArray * SLAPI PPObjTransport::MakeList(long trType)
{
	LongArray * p_list = 0;
	StrAssocListBoxDef * p_lbx_def = static_cast<StrAssocListBoxDef *>(Selector(reinterpret_cast<void *>(trType)));
	if(p_lbx_def) {
		p_list = new LongArray;
		if(p_list)
			p_lbx_def->getIdList(*p_list);
	}
	ZDELETE(p_lbx_def);
	return p_list;
}

int SLAPI PPObjTransport::Browse(void * extraPtr)
{
	return CheckRights(PPR_READ) ? SimpleObjView(this, extraPtr) : PPErrorZ();
}

class TransportDialog : public TDialog {
public:
	explicit TransportDialog(uint dlgID) : TDialog(dlgID), LockAutoName(0)
	{
		PPObjTransport::ReadConfig(&Cfg);
	}
	int    setDTS(const PPTransport * pData)
	{
		Data = *pData;
		LockAutoName = 1;
		if(Cfg.NameTemplate.NotEmpty() && Data.TrType == PPTRTYP_CAR)
			selectCtrl(CTL_TRANSPORT_MODEL);
		PPID   owner_kind_id = NZOR(Cfg.OwnerKindID, PPPRK_SHIPOWNER);
		PPID   captain_kind_id = NZOR(Cfg.CaptainKindID, PPPRK_CAPTAIN);
		setCtrlData(CTL_TRANSPORT_NAME,  Data.Name);
		setCtrlData(CTL_TRANSPORT_CODE,  Data.Code);
		setCtrlData(CTL_TRANSPORT_TRAILCODE, Data.TrailerCode);
		setCtrlReal(CTL_TRANSPORT_CAPACITY, fdiv1000i(Data.Capacity));
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_MODEL, PPOBJ_TRANSPMODEL, Data.TrModelID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_OWNER, PPOBJ_PERSON, Data.OwnerID, OLW_CANINSERT, (void *)owner_kind_id);
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_CAPTAIN, PPOBJ_PERSON, Data.CaptainID, OLW_CANINSERT, (void *)captain_kind_id);
		SetupPPObjCombo(this, CTLSEL_TRANSPORT_CNTRY, PPOBJ_COUNTRY, Data.CountryID, OLW_CANINSERT);
		// @v10.2.0 {
		if(Data.TrType == PPTRTYP_CAR) {
			SetupStringCombo(this, CTLSEL_TRANSPORT_VANTYP, PPTXT_VANTYPE, Data.VanType);
		}
		// } @v10.2.0
		// @v10.2.4 {
		AddClusterAssoc(CTL_TRANSPORT_FLAGS, 0, GF_PASSIV);
		SetClusterData(CTL_TRANSPORT_FLAGS, Data.Flags);
		// } @v10.2.4
		LockAutoName = 0;
		return 1;
	}
	int    getDTS(PPTransport * pData)
	{
		int    ok = 1;
		Helper_GetDTS();
		if(*strip(Data.Name) == 0) {
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
		if(!LockAutoName && Cfg.NameTemplate.NotEmpty()) {
			if(event.isCbSelected(CTLSEL_TRANSPORT_MODEL) || event.isCbSelected(CTLSEL_TRANSPORT_OWNER) ||
				event.isCbSelected(CTLSEL_TRANSPORT_CAPTAIN) ||
				(event.isCmd(cmInputUpdated) &&
				(event.isCtlEvent(CTL_TRANSPORT_CODE) || event.isCtlEvent(CTL_TRANSPORT_TRAILCODE)))) {
				Helper_GetDTS();
				SString name_buf;
				TrObj.GetNameByTemplate(&Data, Cfg.NameTemplate, name_buf);
				setCtrlString(CTL_TRANSPORT_NAME, name_buf);
				clearEvent(event);
			}
		}
	}
	void   Helper_GetDTS()
	{
		getCtrlData(CTL_TRANSPORT_NAME,  Data.Name);
		getCtrlData(CTL_TRANSPORT_CODE,  Data.Code);
		getCtrlData(CTL_TRANSPORT_TRAILCODE,  Data.TrailerCode);
		getCtrlData(CTLSEL_TRANSPORT_MODEL, &Data.TrModelID);
		getCtrlData(CTLSEL_TRANSPORT_OWNER, &Data.OwnerID);
		getCtrlData(CTLSEL_TRANSPORT_CNTRY, &Data.CountryID);
		getCtrlData(CTLSEL_TRANSPORT_CAPTAIN, &Data.CaptainID);
		// @v10.2.0 {
		if(Data.TrType == PPTRTYP_CAR) {
			long   temp_val = 0;
			getCtrlData(CTLSEL_TRANSPORT_VANTYP, &temp_val);
			Data.VanType = (int16)temp_val;
		}
		// } @v10.2.0
		Data.Capacity = (long)(getCtrlReal(CTL_TRANSPORT_CAPACITY) * 1000.0);
		Data.Flags = (int16)GetClusterData(CTL_TRANSPORT_FLAGS); // @v10.2.4
	}
	int    LockAutoName;
	PPTransport Data;
	PPTransportConfig Cfg;
	PPObjTransport TrObj;
};

int SLAPI PPObjTransport::Edit(PPID * pID, void * extraPtr /*initTrType*/)
{
	int    ok = -1, valid_data = 0;
	uint   dlg_id = 0;
	long   tr_type = reinterpret_cast<long>(extraPtr);
	TDialog * sel_dlg = 0;
	PPTransport rec;
	if(*pID) {
		THROW(Get(*pID, &rec) > 0);
		tr_type = rec.TrType;
	}
	else {
		MEMSZERO(rec);
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
		rec.TrType = tr_type;
	}
	if(tr_type == PPTRTYP_CAR)
		dlg_id = DLG_TR_CAR;
	else if(tr_type == PPTRTYP_SHIP)
		dlg_id = DLG_TR_SHIP;
	else
		dlg_id = 0;
	if(dlg_id) {
		if(PPDialogProcBodyID<TransportDialog, PPTransport>(dlg_id, &rec) > 0) {
			valid_data = 1;
			ok = cmOK;
			THROW(Put(pID, &rec, 1));
		}
	}
	CATCHZOKPPERR
	delete sel_dlg;
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjTransport, PPTransport);

int SLAPI PPObjTransport::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPTransport * p_pack = (PPTransport *)p->Data;
		ProcessObjRefInArray(PPOBJ_TRANSPMODEL, &p_pack->TrModelID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->OwnerID, ary, replace);
		ProcessObjRefInArray(PPOBJ_COUNTRY, &p_pack->CountryID, ary, replace);
		ProcessObjRefInArray(PPOBJ_PERSON,  &p_pack->CaptainID, ary, replace);
		return 1;
	}
	return -1;
}

int SLAPI PPObjTransport::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext *)
{
	int    ok = -1;
	PPTransport * p_pack = 0;
	THROW_MEM(p->Data = new PPTransport);
	p_pack = (PPTransport *)p->Data;
	if(stream == 0) {
		THROW(Get(id, p_pack) > 0);
	}
	else
		THROW(ReadBlk(p_pack, sizeof(*p_pack), stream));
	CATCHZOK
	return ok;
}

int SLAPI PPObjTransport::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPTransport * p_pack = (PPTransport *)p->Data;
		if(stream == 0) {
			PPTransport temp_rec;
			if((*pID || SearchByName(p_pack->Name, pID, 0) > 0) && Get(*pID, &temp_rec) > 0) {
				if(!Put(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTTRANSPORT, p_pack->ID, p_pack->Name);
					ok = -1;
				}
			}
			else {
				if(!Put(pID, p_pack, 1)) {
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

int SLAPI PPObjTransport::GetNameByTemplate(PPTransport * pPack, const char * pTemplate, SString & rBuf) const
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
						GetObjectName(PPOBJ_TRANSPMODEL, pPack->TrModelID, temp_buf, 0);
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
SLAPI PPBrand::PPBrand()
{
	THISZERO();
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
					SString temp_buf = Name;
					if(!temp_buf.Search(pFilt->SrchStr, 0, 1, 0))
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
IMPLEMENT_PPFILT_FACTORY(Brand); SLAPI BrandFilt::BrandFilt() : PPBaseFilt(PPFILT_BRAND, 0, 1)
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

int SLAPI BrandFilt::IsEmpty() const
{
	int    yes = 1;
	if(Flags != 0)
		yes = 0;
	else if(SrchStr.NotEmpty())
		yes = 0;
	else if(!ParentList.IsEmpty())
		yes = 0;
	else if(!OwnerList.IsEmpty())
		yes = 0;
	return yes;
}

SLAPI PPBrandPacket::PPBrandPacket()
{
	Init();
}

SLAPI PPBrandPacket::~PPBrandPacket()
{
	LinkFiles.Clear();
}

void SLAPI PPBrandPacket::Init()
{
	MEMSZERO(Rec);
	LinkFiles.Clear();
}

PPBrandPacket & FASTCALL PPBrandPacket::operator = (const PPBrandPacket & rSrc)
{
	memcpy(&Rec, &rSrc.Rec, sizeof(Rec));
	LinkFiles = rSrc.LinkFiles;
	return *this;
}
//
// @ModuleDef(PPObjBrand)
//
SLAPI PPObjBrand::PPObjBrand(void * extraPtr) : PPObjGoods(PPOBJ_BRAND, extraPtr)
{
	Kind = PPGDSK_BRAND;
}

// static
int FASTCALL PPObjBrand::Helper_GetRec(const Goods2Tbl::Rec & rGoodsRec, PPBrand * pRec)
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
		else {
			PPSetError(/*PPERR_INVBRANDRECKIND*/1, rGoodsRec.ID);
			ok = 0;
		}
	}
	return ok;
}

int SLAPI PPObjBrand::Fetch(PPID id, PPBrand * pRec)
{
	Goods2Tbl::Rec goods_rec;
	int    ok = PPObjGoods::Fetch(id, &goods_rec);
	return (ok > 0) ? Helper_GetRec(goods_rec, pRec) : ok;
}


int SLAPI PPObjBrand::Get(PPID id, PPBrandPacket * pPack)
{
	int    ok = PPObjGoods::Search(id);
	return (ok > 0) ? Helper_GetRec(P_Tbl->data, &pPack->Rec) : ok;
}

int SLAPI PPObjBrand::Put(PPID * pID, PPBrandPacket * pPack, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec raw_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Search(*pID, &raw_rec) > 0 && raw_rec.Kind == PPGDSK_BRAND);
			if(pPack) {
				if(pPack->Rec.OwnerID != raw_rec.ManufID || strcmp(pPack->Rec.Name, raw_rec.Name) != 0 || pPack->Rec.Flags != raw_rec.Flags ||
					pPack->Rec.Flags & BRNDF_HASIMAGES
				) {
					THROW(CheckRights(PPR_MOD));
					raw_rec.Kind = PPGDSK_BRAND;
					STRNSCPY(raw_rec.Name, pPack->Rec.Name);
					raw_rec.ManufID = pPack->Rec.OwnerID;
					raw_rec.Flags   = pPack->Rec.Flags;
					THROW(P_Tbl->Update(pID, &raw_rec, 0));
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

int SLAPI PPObjBrand::AddSimple(PPID * pID, const char * pName, PPID ownerID, int use_ta)
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

int SLAPI PPObjBrand::GetListByFilt(const BrandFilt * pFilt, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray result_list;
	Goods2Tbl::Key2 k2;
	const PPID single_owner_id = pFilt->OwnerList.GetSingle();
	BExtQuery q(P_Tbl, 2);
	DBQ * dbq = &(P_Tbl->Kind == Kind);
	dbq = ppcheckfiltid(dbq, P_Tbl->ManufID, single_owner_id);
	q.select(P_Tbl->ID, P_Tbl->Name, P_Tbl->GoodsTypeID, P_Tbl->ManufID, P_Tbl->Flags, 0L).where(*dbq);
	MEMSZERO(k2);
	k2.Kind = Kind;
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		PPBrand rec;
		if(Helper_GetRec(P_Tbl->data, &rec)) {
			if(rec.CheckForFilt(pFilt)) {
				THROW_SL(result_list.add(rec.ID));
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pList, result_list);
	return ok;
}

ListBoxDef * SLAPI PPObjBrand::Selector(void * extraPtr)
{
	return _Selector2(0, 0, PPObjGoods::selfByName, 0, 0, 0);
}

// virtual
void * SLAPI PPObjBrand::CreateObjListWin(uint flags, void * extraPtr)
{
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
								((PPApp*)APPL)->LastCmd = TVCMD;
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
}

int SLAPI PPObjBrand::Browse(void * extraPtr)
{
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
}

#define GRP_IBG 1

int SLAPI PPObjBrand::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1, valid_data = 0, is_new = 0;
	TDialog * dlg = 0;
	PPBrandPacket pack;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_BRAND))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Get(*pID, &pack) > 0);
	}
	else
		pack.Init();
	dlg->setCtrlLong(CTL_GOODS_ID, pack.Rec.ID); // @v8.2.9
	dlg->setCtrlData(CTL_GOODS_NAME, pack.Rec.Name);
	dlg->addGroup(GRP_IBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_GOODS_IMAGE,
		cmAddImage, cmDelImage, 1, ImageBrowseCtrlGroup::fUseExtOpenDlg));
	{
		ImageBrowseCtrlGroup::Rec rec;
		pack.LinkFiles.Init(PPOBJ_BRAND);
		if(pack.Rec.Flags & BRNDF_HASIMAGES)
			pack.LinkFiles.Load(pack.Rec.ID, 0L);
		pack.LinkFiles.At(0, rec.Path);
		dlg->setGroupData(GRP_IBG, &rec);
	}
	SetupPPObjCombo(dlg, CTLSEL_GOODS_MANUF, PPOBJ_PERSON, pack.Rec.OwnerID, OLW_CANINSERT, (void *)PPPRK_MANUF);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_GOODS_NAME,  pack.Rec.Name);
		dlg->getCtrlData(CTLSEL_GOODS_MANUF, &pack.Rec.OwnerID);
		{
			ImageBrowseCtrlGroup::Rec rec;
			if(dlg->getGroupData(GRP_IBG, &rec))
				if(rec.Path.Len()) {
					THROW(pack.LinkFiles.Replace(0, rec.Path));
				}
				else
					pack.LinkFiles.Remove(0);
			SETFLAG(pack.Rec.Flags, BRNDF_HASIMAGES, pack.LinkFiles.GetCount());
		}
		if(*strip(pack.Rec.Name) == 0)
			PPErrorByDialog(dlg, CTL_GOODS_NAME, PPERR_NAMENEEDED);
		else if(Put(pID, &pack, 1)) {
			valid_data = 1;
			ok = cmOK;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjBrand, PPBrand);

int SLAPI PPObjBrand::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPBrand * p_pack = (PPBrand *)p->Data;
		return BIN(ProcessObjRefInArray(PPOBJ_PERSON, &p_pack->OwnerID,ary, replace));
	}
	return -1;
}

int SLAPI PPObjBrand::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext *)
{
	int    ok = -1;
	PPBrand * p_pack = 0;
	THROW_MEM(p->Data = new PPBrand);
	p_pack = (PPBrand *)p->Data;
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

int SLAPI PPObjBrand::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPBrand * p_pack = (PPBrand *)p->Data;
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

// static
int SLAPI PPObjGoodsGroup::SetOwner(PPID id, long curOwner, long newOwner)
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
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
	ENDCATCH
	return ok;
}

// static
int SLAPI PPObjGoodsGroup::RemoveTempAlt(PPID id, long owner, int forceDel /*=0*/, int useTa /*=1*/)
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

// static
int SLAPI PPObjGoodsGroup::AddDynamicAltGroupByFilt(const GoodsFilt * pFilt, PPID * pGrpID, long owner, int useTa)
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

// static
int SLAPI PPObjGoodsGroup::SetDynamicOwner(PPID id, long curOwner, long newOwner)
{
	int    ok = -1;
	PPObjGoodsGroup gg_obj;
	if(id && gg_obj.CheckFlag(id, GF_DYNAMICTEMPALTGRP)) {
		PPTransaction tra(-1);
		THROW(tra);
		if(gg_obj.Search(id, 0) > 0) {
			const long cur_owner = gg_obj.P_Tbl->data.ManufID; // as owner
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
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
	ENDCATCH
	return ok;
}

// static
int SLAPI PPObjGoodsGroup::RemoveDynamicAlt(PPID id, long owner, int forceDel /*=0*/, int useTa /*=1*/)
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

SLAPI PPSuprWare::PPSuprWare()
{
	THISZERO();
}

int FASTCALL PPSuprWare::IsEqual(const PPSuprWare & rS) const
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

SLAPI PPSuprWareAssoc::PPSuprWareAssoc()
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
	assert(sizeof(ObjAssocTbl::Rec) == sizeof(_GCompItem));
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

void SLAPI PPSuprWarePacket::Init()
{
	MEMSZERO(Rec);
	Items.clear();
}
//
//
//
SLAPI PPObjSuprWare::PPObjSuprWare(void * extraPtr) : PPObjGoods(PPOBJ_COMPGOODS, extraPtr)
{
	Kind = PPGDSK_SUPRWARE;
}

int SLAPI PPObjSuprWare::Get(PPID id, PPSuprWarePacket * pPack)
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

// static
int SLAPI PPObjSuprWare::MakeStorage(PPID id, const PPSuprWare * pRec, Goods2Tbl::Rec * pRawRec, BarcodeArray * pBcList)
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
		BarcodeTbl::Rec bc_rec;
		SString temp_buf;
		pBcList->freeAll();
		temp_buf = pRec->Code;
		if(temp_buf.NotEmptyS()) {
			MEMSZERO(bc_rec);
			bc_rec.GoodsID = id;
			bc_rec.Qtty = 1.0;
			temp_buf.PadLeft(1, '~').CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
			pBcList->insert(&bc_rec);
		}
	}
	return ok;
}

int SLAPI PPObjSuprWare::Put(PPID * pID, const PPSuprWarePacket * pPack, int use_ta)
{
	assert(sizeof(_GCompItem)==sizeof(ObjAssocTbl::Rec));

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
				if(!pPack->Rec.IsEqual(org_pack.Rec)) {
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
					gci = *(_GCompItem *)prev_items.at(i);
					if(items.lsearch(&gci.CompID, &pos, PTR_CMPFUNC(long), offsetof(_GCompItem, CompID))) {
						_GCompItem * p_list_item = static_cast<_GCompItem *>(items.at(pos));
						found_pos_list.add((long)pos);
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

int SLAPI PPObjSuprWare::PutAssoc(PPSuprWareAssoc & rItem, int use_ta)
{
	assert(sizeof(_GCompItem)==sizeof(ObjAssocTbl::Rec));

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

int SLAPI PPObjSuprWare::SearchByBarcode(const char * pBarcode, BarcodeTbl::Rec * pBcRec)
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

int SLAPI PPObjSuprWare::GetListByComponent(PPID componentID, PPIDArray & rList)
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

int SLAPI SuprWareListDialog::setupList()
{
	Goods2Tbl::Rec goods_rec;
	PPObjGoods goods_o;
	StringSet ss(SLBColumnDelim);
	SString str;
	PPUnit unit;
	for(uint i = 0; i < P_SuprWarePack.Items.getCount(); i++) {
		MEMSZERO(goods_rec);
		if(goods_o.Search(P_SuprWarePack.Items.at(i).CompID, &goods_rec) > 0) {
			ss.clear();
			ss.add(goods_rec.Name);
			if(P_SuprWarePack.Items.at(i).Qtty || P_SuprWarePack.Items.at(i).UnitID) {
				str.Z().Cat(P_SuprWarePack.Items.at(i).Qtty);
				ss.add(str);
				MEMSZERO(unit);
				if(goods_o.FetchUnit(P_SuprWarePack.Items.at(i).UnitID, &unit))
					ss.add(unit.Name);
			}
			if(!addStringToList(i+1, ss.getBuf()))
				return 0;
		}
	}
	return 1;
}

// Не редактирует запись. Просто показывает таблицу с содержанием товара
int SLAPI PPObjSuprWare::EditList(PPID * pID)
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
public:
	SuprWareDialog() : TDialog(DLG_SUPRWARE)
	{
	}
	int    setDTS(const PPSuprWarePacket * pData)
	{
		Data = *pData;

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
	int    getDTS(PPSuprWarePacket * pData)
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
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	PPSuprWarePacket Data;
};

int SLAPI PPObjSuprWare::Edit(PPID * pID, void * extraPtr)
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

int SLAPI PPObjSuprWare::DeleteObj(PPID id)
{
	return Put(&id, 0, 0);
}
//
// } @vmiller
//
