// WORKBOOK.CPP
// Copyright (c) Petroglif 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
// Модуль управления рабочими книгами (произвольный контент, как правило, предназначенный для отображения в web-browser'е
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
// @ModuleDecl(PPObjWorkbook)
//
WorkbookCore::WorkbookCore() : WorkbookTbl()
{
}

int WorkbookCore::Helper_GetChildList(PPID id, int recursive, PPIDArray & rList, PPIDArray * pRecurTrace)
{
	assert(pRecurTrace != 0);
	int    ok = -1;
	WorkbookTbl::Rec rec;
	if(pRecurTrace->addUnique(id) > 0) {
		PPIDArray local_list;
		for(SEnum en = EnumByParent(id, 0); en.Next(&rec) > 0;) {
			if(rec.ID != id)
				local_list.add(rec.ID);
			ok = 1;
		}
		local_list.sortAndUndup();
		if(recursive) {
			for(uint i = 0; i < local_list.getCount(); i++) {
				const PPID next_level_id = local_list.get(i);
				if(!pRecurTrace->lsearch(next_level_id)) {
					THROW(Helper_GetChildList(next_level_id, recursive, rList, pRecurTrace)); // @recursion
				}
			}
		}
		if(ok > 0) {
			rList.add(&local_list);
			rList.sortAndUndup();
		}
	}
	CATCHZOK
	return ok;
}

int WorkbookCore::GetChildList(PPID id, int recursive, PPIDArray & rList)
{
	PPIDArray recur_trace;
	return Helper_GetChildList(id, recursive, rList, &recur_trace);
}

int WorkbookCore::InitEnum(int flags, long * pHandle)
{
	WorkbookTbl * t = this;
	BExtQuery * q = new BExtQuery(t, 0);
	if(flags & (eoIdName|eoIdSymb)) {
		q->select(t->ID, t->Rank, 0L);
		if(flags & eoIdName)
			q->addField(t->Name);
		if(flags & eoIdSymb)
			q->addField(t->Symb);
	}
	else
		q->selectAll();
	WorkbookTbl::Key0 k0;
	k0.ID = 0;
	q->initIteration(false, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

int WorkbookCore::InitEnumByParam(int fldId, PPID param, int flags, long * pHandle)
{
	int    ok = 0;
	int    idx = -1;
	assert(oneof2(fldId, idxfldParent, idxfldType));
	if(oneof2(fldId, idxfldParent, idxfldType)) {
		DBQ * dbq = 0;
		union {
			WorkbookTbl::Key3 k3;
			WorkbookTbl::Key4 k4;
		} k;
		MEMSZERO(k);
		WorkbookTbl * t = this;
		if(fldId == idxfldParent) {
			idx = 4;
			k.k4.ParentID = param;
			k.k4.Rank = -MAXLONG;
			dbq = &(t->ParentID == param);
		}
		else if(fldId == idxfldType) {
			idx = 3;
			k.k3.Type = param;
			k.k3.Rank = -MAXLONG;
			dbq = &(t->Type == param);
		}
		assert(idx >= 0);
		BExtQuery * q = new BExtQuery(t, idx);
		if(flags & (eoIdName|eoIdSymb)) {
			q->select(t->ID, t->Rank, 0L);
			if(flags & eoIdName)
				q->addField(t->Name);
			if(flags & eoIdSymb)
				q->addField(t->Symb);
		}
		else
			q->selectAll();
		q->where(*dbq);
		q->initIteration(false, &k, spGe);
		ok = EnumList.RegisterIterHandler(q, pHandle);
	}
	return ok;
}

SEnum::Imp * WorkbookCore::Enum(int options)
{
	long   h = -1;
	return InitEnum(options, &h) ? new PPTblEnum <WorkbookCore>(this, h) : 0;
}

SEnum::Imp * WorkbookCore::EnumByParent(PPID parentID, int options)
{
	long   h = -1;
	return InitEnumByParam(idxfldParent, parentID, options, &h) ? new PPTblEnum <WorkbookCore>(this, h) : 0;
}

SEnum::Imp * WorkbookCore::EnumByType(long type, int options)
{
	long   h = -1;
	return InitEnumByParam(idxfldType, type, options, &h) ? new PPTblEnum <WorkbookCore>(this, h) : 0;
}

int WorkbookCore::NextEnum(long enumHandle, WorkbookTbl::Rec * pRec)
{
	return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1;
}
//
//
//
PPWorkbookPacket::PPWorkbookPacket()
{
	destroy();
}

PPWorkbookPacket::~PPWorkbookPacket()
{
	destroy();
}

int FASTCALL PPWorkbookPacket::IsEq(const PPWorkbookPacket & rS) const
{
#define NRECFLD(f) if(Rec.f != rS.Rec.f) return 0
	NRECFLD(ID);
	NRECFLD(Type);
	NRECFLD(Rank);
	NRECFLD(Flags);
	NRECFLD(ParentID);
	NRECFLD(CssID);
	NRECFLD(LinkID);
	NRECFLD(KeywordCount);
	NRECFLD(KeywordDilute);
	NRECFLD(Dt);
	NRECFLD(Tm);
	if(!sstreq(Rec.Name, rS.Rec.Name))
		return 0;
	else if(!sstreq(Rec.Symb, rS.Rec.Symb))
		return 0;
	else if(!sstreq(Rec.Version, rS.Rec.Version))
		return 0;
	else if(!TagL.IsEq(rS.TagL))
		return 0;
	else if(ExtString != rS.ExtString)
		return 0;
	else if(F.IsChanged(Rec.ID, 0L))
		return 0;
	else
		return 1;
#undef NRECFLD
}

void PPWorkbookPacket::destroy()
{
	MEMSZERO(Rec);
	TagL.Destroy();
	F.Clear();
	ExtString.Z();
}

int PPWorkbookPacket::GetExtStrData(int fldID, SString & rBuf) const
	{ return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPWorkbookPacket::PutExtStrData(int fldID, const char * pBuf)
	{ return PPPutExtStrData(fldID, ExtString, pBuf); }
int PPWorkbookPacket::SetLongSymb(const char * pSymb)
	{ return TagL.PutItemStrNE(PPTAG_WORKBOOK_LONGCODE, pSymb); }
//
//
//
PPWorkbookConfig::PPWorkbookConfig()
{
	Z();
}

PPWorkbookConfig & PPWorkbookConfig::Z()
{
	THISZERO();
	return *this;
}

/*static*/int FASTCALL PPObjWorkbook::ReadConfig(PPWorkbookConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_WORKBOOKCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

static int PutCounter(PPID * pID, PPObjOpCounter * pOpcObj, PPOpCounterPacket * pCntr)
{
	pCntr->Head.ObjType = PPOBJ_WORKBOOK;
	pCntr->Head.OwnerObjID = -1;
	return BIN(pOpcObj->PutPacket(pID, pCntr, 0));
}

static int PPObjWorkbook_WriteConfig(PPWorkbookConfig * pCfg, PPOpCounterPacket * pCntr)
{
	int    ok = 1;
	int    is_new = 1;
	PPObjOpCounter opc_obj;
	PPWorkbookConfig prev_cfg;
	{
		Reference * p_ref = PPRef;
		PPTransaction tra(1);
		THROW(tra);
		if(p_ref->GetPropMainConfig(PPPRP_WORKBOOKCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
			is_new = 0;
		THROW(PutCounter(&pCfg->SymbCntrID, &opc_obj, pCntr));
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_WORKBOOKCFG, pCfg, sizeof(*pCfg), 0));
		DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_WORKBOOK, 0, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

static IMPL_CMPFUNC(WorkbookIdByRank_Name, p1, p2)
{
	int    si = 0;
	PPObjWorkbook * p_obj = static_cast<PPObjWorkbook *>(pExtraData);
	if(p_obj) {
		const PPID * p_id1 = static_cast<const PPID *>(p1);
		const PPID * p_id2 = static_cast<const PPID *>(p2);
		WorkbookTbl::Rec rec1;
		WorkbookTbl::Rec rec2;
		if(p_obj->Fetch(*p_id1, &rec1) > 0 && p_obj->Fetch(*p_id2, &rec2) > 0) {
			CMPCASCADE2(si, &rec1, &rec2, ParentID, Rank);
			//si = CMPSIGN(rec1.Rank, rec2.Rank);
			if(!si) {
				si = stricmp866(rec1.Name, rec2.Name);
			}
		}
	}
	return si;
}

void PPObjWorkbook::SortIdListByRankAndName(LongArray & rList)
{
	rList.SVectorBase::sort(PTR_CMPFUNC(WorkbookIdByRank_Name), this);
}

/*static*/int PPObjWorkbook::EditConfig()
{
	class WorkbookCfgDialog : public TDialog {
	public:
		struct Rec {
			PPOpCounterPacket Cntr;
			PPWorkbookConfig Cfg;
		};
		DECL_DIALOG_DATA(Rec);

		WorkbookCfgDialog() : TDialog(DLG_WBCFG)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_WBCFG_CODETEMPL, Data.Cntr.Head.CodeTemplate);
			setCtrlLong(CTL_WBCFG_CODECNTR,  Data.Cntr.Head.Counter);
			SetupPPObjCombo(this, CTLSEL_WBCFG_UHTTXEVTOK, PPOBJ_EVENTTOKEN, Data.Cfg.UhttXEvTokID, OLW_CANINSERT, 0); // @v9.3.9
			SetupPPObjCombo(this, CTLSEL_WBCFG_DEFIMGFOLD, PPOBJ_WORKBOOK, Data.Cfg.DefImageFolderID, OLW_CANINSERT|OLW_CANSELUPLEVEL, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTL_WBCFG_CODETEMPL,     Data.Cntr.Head.CodeTemplate);
			getCtrlData(CTL_WBCFG_CODECNTR,      &Data.Cntr.Head.Counter);
			getCtrlData(CTLSEL_WBCFG_UHTTXEVTOK, &Data.Cfg.UhttXEvTokID);
			getCtrlData(CTLSEL_WBCFG_DEFIMGFOLD, &Data.Cfg.DefImageFolderID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	int    ok = -1, is_new = 0;
	WorkbookCfgDialog * dlg = new WorkbookCfgDialog;
	PPObjOpCounter opc_obj;
	WorkbookCfgDialog::Rec data;
	THROW(CheckDialogPtr(&dlg));
	THROW(CheckCfgRights(PPCFGOBJ_WORKBOOK, PPR_READ, 0));
	is_new = ReadConfig(&data.Cfg);
	SETIFZ(data.Cfg.SymbCntrID, PPOPCNTR_WORKBOOK);
	opc_obj.GetPacket(data.Cfg.SymbCntrID, &data.Cntr);
	dlg->setDTS(&data);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_WORKBOOK, PPR_MOD, 0));
		if(dlg->getDTS(&data)) {
			THROW(PPObjWorkbook_WriteConfig(&data.Cfg, &data.Cntr));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

TLP_IMPL(PPObjWorkbook, WorkbookCore, P_Tbl);

PPObjWorkbook::PPObjWorkbook(void * extraPtr) : PPObject(PPOBJ_WORKBOOK), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

PPObjWorkbook::~PPObjWorkbook()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjWorkbook::Search(PPID id, void * pRec)
{
	return SearchByID(P_Tbl, Obj, id, pRec);
}

int PPObjWorkbook::SearchByName(const char * pName, PPID * pID, WorkbookTbl::Rec * pRec)
{
	WorkbookTbl::Key1 k1;
	MEMSZERO(k1);
	strip(STRNSCPY(k1.Name, pName));
	int    ok = SearchByKey(P_Tbl, 1, &k1, pRec);
	ASSIGN_PTR(pID, ((ok > 0) ? P_Tbl->data.ID : 0));
	return ok;
}

int PPObjWorkbook::SearchBySymb(const char * pSymb, PPID * pID, WorkbookTbl::Rec * pRec)
{
	//Symb (allsegnull unique mod);        // #5
	WorkbookTbl::Key5 k5;
	MEMSZERO(k5);
	strip(STRNSCPY(k5.Symb, pSymb));
	int    ok = SearchByKey(P_Tbl, 5, &k5, pRec);
	ASSIGN_PTR(pID, ((ok > 0) ? P_Tbl->data.ID : 0));
	return ok;
}

int PPObjWorkbook::SearchByLongSymb(const char * pLongSymb, PPID * pID, WorkbookTbl::Rec * pRec)
{
    int    ok = -1;
    PPID   id = 0;
    PPIDArray id_list;
    if(PPRef->Ot.SearchObjectsByStrExactly(Obj, PPTAG_WORKBOOK_LONGCODE, pLongSymb, &id_list) > 0) {
		for(uint i = 0; ok < 0 && i < id_list.getCount(); i++) {
			id = id_list.get(0);
			WorkbookTbl::Rec rec;
			if(Search(id, &rec) > 0) {
				ASSIGN_PTR(pRec, rec);
				ok = (i < (id_list.getCount()-1)) ? 2 : 1;
			}
			else
				id = 0;
		}
    }
    ASSIGN_PTR(pID, id);
    return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjWorkbook, PPWorkbookPacket);

int PPObjWorkbook::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_WORKBOOK) {
			WorkbookCore * p_t = P_Tbl;
			WorkbookTbl::Key4 k4;
			MEMSZERO(k4);
			k4.ParentID = _id;
			if(p_t->search(4, &k4, spGe) && p_t->data.ParentID == _id) {
				ok = RetRefsExistsErr(PPOBJ_WORKBOOK, p_t->data.ID);
			}
			if(ok) {
				WorkbookTbl::Key0 k0;
				MEMSZERO(k0);
				BExtQuery q(p_t, 0);
				q.select(p_t->ID, p_t->LinkID, 0).where(p_t->LinkID == _id);
				q.initIteration(false, &k0, spFirst);
				if(q.nextIteration() > 0) {
					ok = RetRefsExistsErr(PPOBJ_WORKBOOK, p_t->data.ID);
				}
			}
			if(ok) {
				WorkbookTbl::Key0 k0;
				MEMSZERO(k0);
				BExtQuery q(p_t, 0);
				q.select(p_t->ID, p_t->CssID, 0).where(p_t->CssID == _id);
				q.initIteration(false, &k0, spFirst);
				if(q.nextIteration() > 0) {
					ok = RetRefsExistsErr(PPOBJ_WORKBOOK, p_t->data.ID);
				}
			}
		}
	}
	return DBRPL_OK;
}

int PPObjWorkbook::SerializePacket(int dir, PPWorkbookPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(pPack->TagL.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, pPack->ExtString, rBuf));
	THROW(pPack->F.Serialize(dir, 0, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

/*virtual*/int  PPObjWorkbook::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    ok = -1;
	int    r;
	THROW(CheckRights(PPR_DEL));
	r = (options & user_request) ? PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) : cmYes;
	if(r == cmYes) {
		SString msg_buf;
		PPIDArray child_list;
		WorkbookTbl::Rec wb_rec;
		if(!(options & no_wait_indicator)) {
			PPWaitStart();
			if(Search(id, &wb_rec) > 0)
				msg_buf = wb_rec.Name;
		}
		PPTransaction tra(BIN(options & PPObject::use_transaction));
		THROW(tra);
		if(P_Tbl->GetChildList(id, 1, child_list) > 0) {
			child_list.sortAndUndup();
			const uint clc = child_list.getCount();
			for(uint i = 0; i < clc; i++) {
				const PPID child_id = child_list.get(i);
				if(Search(child_id, 0) > 0) {
					THROW(PPObjWorkbook::RemoveObjV(child_id, pObjColl, PPObject::no_wait_indicator, pExtraParam));  // @recursion
				}
				if(!(options & PPObject::no_wait_indicator)) {
					PPWaitPercent(i+1, clc, msg_buf);
				}
			}
		}
		THROW(PutPacket(&id, 0, 0));
		THROW(tra.Commit());
		ok = 1;
	}
	CATCH
		if(options & PPObject::user_request)
			PPError();
		ok = 0;
	ENDCATCH
	if(!(options & no_wait_indicator))
		PPWaitStop();
	return ok;
}

int PPObjWorkbook::Browse(void * extraPtr)
{
	int    ok = -1;
	ObjViewDialog * p_dlg = 0;
	if(CheckRights(PPR_READ) && CheckDialogPtr(&(p_dlg = new ObjViewDialog(DLG_OBJVIEWT, this, extraPtr)))) {
		ExecViewAndDestroy(p_dlg);
		ok = 1;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int PPObjWorkbook::SelectKeywordReverse(SString & rKeyword)
{
	rKeyword.Z();

	int    ok = -1;
	ObjTagCore & r_t = PPRef->Ot;
	ListWindow * p_lw = 0;
	SymbHashTable hash(4096, 0);
	uint   counter = 0;
	SString temp_buf;
	StringSet ss(",");
	PPIDArray tag_type_list;
	tag_type_list.add(PPTAG_GOODS_KEYWORDS);
	tag_type_list.add(PPTAG_WORKBOOK_KEYWORDS);
	tag_type_list.add(PPTAG_BILL_KEYWORDS);
	for(uint i = 0; i < tag_type_list.getCount(); i++) {
		const PPID tag_id = tag_type_list.get(i);
		ObjTagTbl::Key1 k1;
		MEMSZERO(k1);
		k1.TagID = tag_id;
		BExtQuery q(&r_t, 1);
		q.select(r_t.ObjType, r_t.ObjID, r_t.TagID, r_t.StrVal, 0).where(r_t.TagID == tag_id);
		for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
			ss.setBuf(r_t.data.StrVal, sstrlen(r_t.data.StrVal)+1);
			for(uint sp = 0; ss.get(&sp, temp_buf);) {
				if(!hash.Search(temp_buf.Strip().ToLower(), 0, 0)) {
					if(SearchByName(temp_buf, 0, 0) <= 0)
						hash.Add(temp_buf, ++counter);
				}
			}
		}
	}
	if(counter) {
		p_lw = CreateListWindow_Simple(lbtDblClkNotify);
		if(p_lw) {
			SymbHashTable::Iter iter;
			uint   _id = 0;
			StrAssocArray temp_list;
			for(hash.InitIteration(&iter); hash.NextIteration(&iter, &_id, 0, &temp_buf) > 0;) {
				temp_list.AddFast(_id, temp_buf);
			}
			temp_list.SortByText();
			for(uint j = 0; j < temp_list.getCount(); j++) {
				StrAssocArray::Item item = temp_list.Get(j);
				p_lw->listBox()->addItem(item.Id, item.Txt);
			}
			while(ok < 0 && ExecView(p_lw) == cmOK) {
				if(p_lw->getString(rKeyword)) {
					ok = 1;
				}
			}
		}
		else
			ok = PPErrorZ();
	}
	delete p_lw;
	return ok;
}

int PPObjWorkbook::SelectKeyword(SString & rKeyword)
{
	rKeyword.Z();
	int    ok = -1;
	PPID   id = 0;
	ListWindow * p_lw = CreateListWindow_Simple(lbtDblClkNotify);
	if(p_lw) {
		WorkbookTbl::Rec wb_rec;
		for(SEnum en = P_Tbl->EnumByType(PPWBTYP_KEYWORD, 0); en.Next(&wb_rec) > 0;) {
			p_lw->listBox()->addItem(wb_rec.ID, wb_rec.Name);
		}
		p_lw->listBox()->TransmitData(+1, &id);
		while(ok < 0 && ExecView(p_lw) == cmOK) {
			p_lw->listBox()->TransmitData(-1, &id);
			if(Fetch(id, &wb_rec) > 0) {
				rKeyword = wb_rec.Name;
				ok = 1;
			}
		}
	}
	else {
		PPError();
	}
	delete p_lw;
	return ok;
}

class Workbook2Dialog : public TDialog {
	DECL_DIALOG_DATA(PPWorkbookPacket);
public:
	enum {
		ctlgroupIBG = 1
	};
	explicit Workbook2Dialog(uint dlgID) : TDialog(dlgID), DisableImage(0)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_WORKBOOK_FILE, CTL_WORKBOOK_FILE, 1, 0, 0,
			FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		if(getCtrlView(CTL_WORKBOOK_IMAGE)) {
			addGroup(ctlgroupIBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_WORKBOOK_IMAGE, cmAddImage, cmDelImage, 1));
		}
		SetupCalDate(CTLCAL_WORKBOOK_DT, CTL_WORKBOOK_DT);
		SetupTimePicker(this, CTL_WORKBOOK_TM, CTLTM_WORKBOOK_TM);
	}
	DECL_DIALOG_SETDTS()
	{
		Data = *pData;
		SString temp_buf;
		setCtrlLong(CTL_WORKBOOK_ID, Data.Rec.ID);
		disableCtrl(CTL_WORKBOOK_ID, 1);
		setCtrlString(CTL_WORKBOOK_NAME, temp_buf = Data.Rec.Name);
		setCtrlString(CTL_WORKBOOK_CODE, temp_buf = Data.Rec.Symb);
		setCtrlLong(CTL_WORKBOOK_RANK, Data.Rec.Rank);
		AddClusterAssoc(CTL_WORKBOOK_TYPE, 0, PPWBTYP_SITE);
		AddClusterAssocDef(CTL_WORKBOOK_TYPE, 1, PPWBTYP_PAGE);
		AddClusterAssoc(CTL_WORKBOOK_TYPE, 2, PPWBTYP_CSS);
		AddClusterAssoc(CTL_WORKBOOK_TYPE, 3, PPWBTYP_MEDIA);
		AddClusterAssoc(CTL_WORKBOOK_TYPE, 4, PPWBTYP_FOLDER);
		SetClusterData(CTL_WORKBOOK_TYPE, Data.Rec.Type);
		AddClusterAssoc(CTL_WORKBOOK_FLAGS, 0, PPWBF_HIDDEN);
		AddClusterAssoc(CTL_WORKBOOK_FLAGS, 1, PPWBF_DONTSHOWCHILDREN);
		SetClusterData(CTL_WORKBOOK_FLAGS, Data.Rec.Flags);
		if(getCtrlView(CTL_WORKBOOK_KWFLAGS)) {
			AddClusterAssoc(CTL_WORKBOOK_KWFLAGS, 0, PPWBF_KWDONTSHOWMAINC);
			SetClusterData(CTL_WORKBOOK_KWFLAGS, Data.Rec.Flags);
		}
		setCtrlDate(CTL_WORKBOOK_DT, Data.Rec.Dt);
		setCtrlTime(CTL_WORKBOOK_TM, Data.Rec.Tm);
		WbObj.SetupParentCombo(this, CTLSEL_WORKBOOK_PARENT, Data.Rec.Type, Data.Rec.ID, Data.Rec.ParentID);
		WbObj.SetupCSSCombo(this, CTLSEL_WORKBOOK_CSS, Data.Rec.Type, Data.Rec.ID, Data.Rec.CssID);
		WbObj.SetupLinkCombo(this, CTLSEL_WORKBOOK_LINK, Data.Rec.Type, Data.Rec.ID, Data.Rec.LinkID);
		{
			SString file_path;
			Data.F.Init(PPOBJ_WORKBOOK);
			Data.F.Load(Data.Rec.ID, 0L);
			Data.F.At(0, file_path);
			setCtrlString(CTL_WORKBOOK_FILE, file_path);
			if(getCtrlView(CTL_WORKBOOK_IMAGE)) {
				if(fileExists(file_path)) {
					SFileFormat ff;
					int    f = ff.Identify(file_path);
					if(!oneof4(ff, SFileFormat::Jpeg, SFileFormat::Png, SFileFormat::Gif, SFileFormat::Bmp)) // @v9.6.2 @fix f-->ff
						DisableImage = 1;
				}
				if(DisableImage) {
					enableCommand(cmAddImage, 0);
					enableCommand(cmDelImage, 0);
					enableCommand(cmPasteImage, 0);
				}
				else {
					ImageBrowseCtrlGroup::Rec rec;
					rec.Path = file_path;
					setGroupData(ctlgroupIBG, &rec);
					disableCtrls(1, CTL_WORKBOOK_FILE, CTLBRW_WORKBOOK_FILE, 0);
				}
			}
		}
		if(Data.Rec.Type == PPWBTYP_KEYWORD) {
			setCtrlData(CTL_WORKBOOK_KWC, &Data.Rec.KeywordCount);
			setCtrlData(CTL_WORKBOOK_KWD, &Data.Rec.KeywordDilute);
			{
				ObjTagItem tag_kws;
				if(tag_kws.Init(PPTAG_WORKBOOK_KWSYN)) {
					disableCtrl(CTL_WORKBOOK_KWSYN, 0);
					const ObjTagItem * p_tag = Data.TagL.GetItem(PPTAG_WORKBOOK_KWSYN);
					if(p_tag && p_tag->GetStr(temp_buf))
						setCtrlString(CTL_WORKBOOK_KWSYN, temp_buf);
				}
				else
					disableCtrl(CTL_WORKBOOK_KWSYN, 1);
			}
			{
				ObjTagItem tag_kwl;
				if(tag_kwl.Init(PPTAG_WORKBOOK_KWLOC)) {
					disableCtrl(CTL_WORKBOOK_KWLOC, 0);
					const ObjTagItem * p_tag = Data.TagL.GetItem(PPTAG_WORKBOOK_KWLOC);
					if(p_tag && p_tag->GetStr(temp_buf))
						setCtrlString(CTL_WORKBOOK_KWLOC, temp_buf);
				}
				else
					disableCtrl(CTL_WORKBOOK_KWLOC, 1);
			}
		}
		else {
			ObjTagItem tag_kw;
			if(tag_kw.Init(PPTAG_WORKBOOK_KEYWORDS)) {
				disableCtrl(CTL_WORKBOOK_KEYWORDS, 0);
				const ObjTagItem * p_tag_kw = Data.TagL.GetItem(PPTAG_WORKBOOK_KEYWORDS);
				if(p_tag_kw && p_tag_kw->GetStr(temp_buf))
					setCtrlString(CTL_WORKBOOK_KEYWORDS, temp_buf);
			}
			else
				disableCtrl(CTL_WORKBOOK_KEYWORDS, 1);
		}
		{
			TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_WORKBOOK_DESCR));
			if(p_il) {
				p_il->setMaxLen(512);
				Data.GetExtStrData(WBEXSTR_DESCRIPTION, temp_buf);
				setCtrlString(CTL_WORKBOOK_DESCR, temp_buf);
			}
		}
		setCtrlString(CTL_WORKBOOK_VER, temp_buf = Data.Rec.Version);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		Data.Rec.ID = getCtrlLong(CTL_WORKBOOK_ID);
		getCtrlString(sel = CTL_WORKBOOK_NAME, temp_buf);
		THROW_PP(temp_buf.NotEmptyS(), PPERR_NAMENEEDED);
		temp_buf.CopyTo(Data.Rec.Name, sizeof(Data.Rec.Name));
		{
			// @todo check duplicates
		}
		getCtrlString(sel = CTL_WORKBOOK_CODE, temp_buf);
		if(temp_buf.NotEmptyS())
			temp_buf.CopyTo(Data.Rec.Symb, sizeof(Data.Rec.Symb));

		Data.Rec.Rank = getCtrlLong(CTL_WORKBOOK_RANK);

		GetClusterData(CTL_WORKBOOK_TYPE, &Data.Rec.Type);
		GetClusterData(CTL_WORKBOOK_FLAGS, &Data.Rec.Flags);
		if(getCtrlView(CTL_WORKBOOK_KWFLAGS)) {
			GetClusterData(CTL_WORKBOOK_KWFLAGS, &Data.Rec.Flags);
		}
		getCtrlData(CTL_WORKBOOK_DT, &Data.Rec.Dt);
		getCtrlData(CTL_WORKBOOK_TM, &Data.Rec.Tm);
		getCtrlData(CTLSEL_WORKBOOK_PARENT, &Data.Rec.ParentID);
		getCtrlData(CTLSEL_WORKBOOK_CSS, &Data.Rec.CssID);
		getCtrlData(CTLSEL_WORKBOOK_LINK, &Data.Rec.LinkID);
		if(Data.Rec.Type == PPWBTYP_KEYWORD) {
			getCtrlData(CTL_WORKBOOK_KWC, &Data.Rec.KeywordCount);
			getCtrlData(CTL_WORKBOOK_KWD, &Data.Rec.KeywordDilute);
			getCtrlString(CTL_WORKBOOK_KWSYN, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KWSYN, temp_buf.Strip());
			getCtrlString(CTL_WORKBOOK_KWLOC, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KWLOC, temp_buf.Strip());
		}
		else {
			getCtrlString(CTL_WORKBOOK_KEYWORDS, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KEYWORDS, temp_buf.Strip());
		}
		{
			if(getCtrlView(CTL_WORKBOOK_IMAGE) && !DisableImage) {
				ImageBrowseCtrlGroup::Rec rec;
				if(getGroupData(ctlgroupIBG, &rec))
					if(rec.Path.Len()) {
						THROW(Data.F.Replace(0, rec.Path));
					}
					else {
						Data.F.Remove(0);
					}
			}
			else {
				getCtrlString(sel = CTL_WORKBOOK_FILE, temp_buf);
				if(temp_buf.Len()) {
					THROW(Data.F.Replace(0, temp_buf));
				}
				else {
					Data.F.Remove(0);
				}
			}
		}
		if(getCtrlView(CTL_WORKBOOK_DESCR)) {
			getCtrlString(CTL_WORKBOOK_DESCR, temp_buf.Z());
			Data.PutExtStrData(WBEXSTR_DESCRIPTION, temp_buf);
		}
		getCtrlString(CTL_WORKBOOK_VER, temp_buf);
		temp_buf.CopyTo(Data.Rec.Version, sizeof(Data.Rec.Version));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		SString temp_buf;
		TDialog::handleEvent(event);
		if(event.isCmd(cmTags)) {
			getCtrlString(CTL_WORKBOOK_KWSYN, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KWSYN, temp_buf.Strip());
			getCtrlString(CTL_WORKBOOK_KWLOC, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KWLOC, temp_buf.Strip());
			getCtrlString(CTL_WORKBOOK_KEYWORDS, temp_buf.Z());
			Data.TagL.PutItemStrNE(PPTAG_WORKBOOK_KEYWORDS, temp_buf.Strip());
			Data.TagL.ObjType = PPOBJ_WORKBOOK;
			EditObjTagValList(&Data.TagL, 0);
			if(getCtrlView(CTL_WORKBOOK_KWSYN)) {
				const ObjTagItem * p_tag = Data.TagL.GetItem(PPTAG_WORKBOOK_KWSYN);
				if(p_tag && p_tag->GetStr(temp_buf.Z()))
					setCtrlString(CTL_WORKBOOK_KWSYN, temp_buf);
			}
			if(getCtrlView(CTL_WORKBOOK_KWLOC)) {
				const ObjTagItem * p_tag = Data.TagL.GetItem(PPTAG_WORKBOOK_KWLOC);
				if(p_tag && p_tag->GetStr(temp_buf.Z()))
					setCtrlString(CTL_WORKBOOK_KWLOC, temp_buf);
			}
			if(getCtrlView(CTL_WORKBOOK_KEYWORDS)) {
				const ObjTagItem * p_tag_kw = Data.TagL.GetItem(PPTAG_WORKBOOK_KEYWORDS);
				if(p_tag_kw && p_tag_kw->GetStr(temp_buf.Z()))
					setCtrlString(CTL_WORKBOOK_KEYWORDS, temp_buf);
			}
		}
		else if(event.isClusterClk(CTL_WORKBOOK_TYPE)) {
			int  new_type = GetClusterData(CTL_WORKBOOK_TYPE);
			if(new_type != Data.Rec.Type) {
				Data.Rec.Type = new_type;
				// @v11.1.10 setCtrlLong(CTLSEL_WORKBOOK_PARENT, 0);
				PPID   parent_id = getCtrlLong(CTLSEL_WORKBOOK_PARENT); // @v11.1.10
				WbObj.SetupParentCombo(this, CTLSEL_WORKBOOK_PARENT, Data.Rec.Type, Data.Rec.ID, parent_id); // @v11.1.10 parentID 0-->
			}
		}
		else if(event.isKeyDown(kbF2)) {
			if(isCurrCtlID(CTL_WORKBOOK_KEYWORDS)) {
				/*
				TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_WORKBOOK_KEYWORDS));
				if(p_il && !p_il->IsInState(sfDisabled) && WbObj.SelectKeyword(temp_buf) > 0) {
					SString line_buf;
					p_il->getText(line_buf);
					size_t c = p_il->getCaret();
					if(c < line_buf.Len())
						line_buf.Insert(c, temp_buf);
					else
						line_buf.Cat(temp_buf);
					p_il->setText(line_buf);
				}
				else
					return;
				*/
				TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_WORKBOOK_KEYWORDS));
				if(p_il && !p_il->IsInState(sfDisabled)) {
					SString line_buf;
					p_il->getText(line_buf);
					if(EditKeywordList(line_buf) > 0) {
						p_il->setText(line_buf);
					}
				}
				else
					return;
			}
			else if(isCurrCtlID(CTL_WORKBOOK_NAME)) {
				if(Data.Rec.Type == PPWBTYP_KEYWORD) {
					if(WbObj.SelectKeywordReverse(temp_buf) > 0) {
						setCtrlString(CTL_WORKBOOK_NAME, temp_buf);
					}
				}
			}
		}
#if 0 // {
		// @debug {
		else if(event.isKeyDown(kbCtrlF3)) {
			SString code, debug_buf;
			getCtrlString(CTL_WORKBOOK_CODE, code);
			if(code.NotEmptyS()) {
				PPUhttClient uhtt_cli;
				if(uhtt_cli.Auth()) {
					UhttWorkbookItemPacket uhtt_wb_pack;
					if(uhtt_cli.GetWorkbookItemByCode(code, uhtt_wb_pack) > 0)
						debug_buf = "OK";
					else
						debug_buf = "ER";
				}
			}
		}
		// } @debug
#endif // } 0
		/* @v8.1.0 Редактирование файла осуществляется из списка командой cmaMore
		if(event.isCmd(cmOpenTextFile)) {
			getCtrlString(CTL_WORKBOOK_FILE, temp_buf);
			if(temp_buf.Len())
				PPEditTextFile(temp_buf);
		}
		*/
		else
			return;
		clearEvent(event);
	}
	int EditKeywordList(SString & rList)
	{
		int    ok = -1;
		PPIDArray id_list;
		StringSet ss(',', rList);
		SString temp_buf;
		PPID   kw_id = 0;
		WorkbookTbl::Rec kw_rec;
		for(uint sp = 0; ss.get(&sp, temp_buf);) {
			kw_id = 0;
			if(temp_buf.C(0) == '#' && temp_buf.Len() > 1) {
				kw_id = temp_buf.ShiftLeft().ToLong();
				if(WbObj.Fetch(kw_id, &kw_rec) > 0 && kw_rec.Type == PPWBTYP_KEYWORD) {
					id_list.addUnique(kw_id);
				}
			}
			else if(temp_buf.C(0) == '$' && temp_buf.Len() > 1) {
				temp_buf.ShiftLeft();
				if(WbObj.SearchBySymb(temp_buf, &kw_id, &kw_rec) > 0 && kw_rec.Type == PPWBTYP_KEYWORD) {
					id_list.addUnique(kw_id);
				}
			}
			else {
				if(WbObj.SearchByName(temp_buf, &kw_id, &kw_rec) > 0 && kw_rec.Type == PPWBTYP_KEYWORD) {
					id_list.addUnique(kw_id);
				}
			}
		}
		{
			StrAssocArray src_list;
			for(SEnum en = WbObj.P_Tbl->EnumByType(PPWBTYP_KEYWORD, WorkbookCore::eoIdName); en.Next(&kw_rec) > 0;) {
				src_list.AddFast(kw_rec.ID, kw_rec.Name);
			}
			ListToListData lst(&src_list, PPOBJ_WORKBOOK, &id_list);
			lst.TitleStrID = 0; // PPTXT_XXX;
			if(ListToListDialog(&lst) > 0) {
				rList.Z();
				if(lst.P_List) {
					for(uint i = 0; i < lst.P_List->getCount(); i++) {
						kw_id = lst.P_List->get(i);
						if(WbObj.Fetch(kw_id, &kw_rec) > 0 && kw_rec.Type == PPWBTYP_KEYWORD) {
							temp_buf = kw_rec.Symb;
							if(temp_buf.NotEmptyS()) {
								rList.CatDivIfNotEmpty(',', 0).CatChar('$').Cat(temp_buf);
							}
							else {
								rList.CatDivIfNotEmpty(',', 0).CatChar('#').Cat(kw_id);
							}
						}
					}
				}
				ok = 1;
			}
		}
		return ok;
	}
	int    DisableImage;
	PPObjWorkbook    WbObj;
};

class AddWorkbookItemDialog : public TDialog {
	DECL_DIALOG_DATA(PPObjWorkbook::AddBlock);
public:
	AddWorkbookItemDialog() : TDialog(DLG_WBPRELUDE)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_WBPRELUDE_FILE, CTL_WBPRELUDE_FILE, 1, 0, 0,
			FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_WBPRELUDE_TYPE, 0, PPWBTYP_SITE);
		AddClusterAssocDef(CTL_WBPRELUDE_TYPE, 1, PPWBTYP_PAGE);
		AddClusterAssoc(CTL_WBPRELUDE_TYPE, 2, PPWBTYP_MEDIA);
		AddClusterAssoc(CTL_WBPRELUDE_TYPE, 3, PPWBTYP_FOLDER);
		AddClusterAssoc(CTL_WBPRELUDE_TYPE, 4, PPWBTYP_KEYWORD);
		AddClusterAssoc(CTL_WBPRELUDE_TYPE, 5, PPWBTYP_CSS);
		SetClusterData(CTL_WBPRELUDE_TYPE, Data.Type);
		setCtrlString(CTL_WBPRELUDE_FILE, Data.FileName);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		GetClusterData(CTL_WBPRELUDE_TYPE, &Data.Type);
		getCtrlString(CTL_WBPRELUDE_FILE, Data.FileName);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
};

PPObjWorkbook::AddBlock::AddBlock() : Type(0), ParentID(0), Flags(0)
{
}

int PPObjWorkbook::AddItem(PPID * pID, PPID parentID)
{
	int    ok = -1;
	PPObjWorkbook::AddBlock ab;
	ab.ParentID = parentID;
	if(PPDialogProcBody <AddWorkbookItemDialog, PPObjWorkbook::AddBlock> (&ab) > 0) {
		ok = Helper_Edit(pID, &ab);
	}
	return ok;
}

int PPObjWorkbook::Helper_Edit(PPID * pID, AddBlock * pAb)
{
	int    r = cmCancel, ok = 1, valid_data = 0;
	PPWorkbookPacket pack;
	Workbook2Dialog * dlg = 0;
	THROW(CheckRightsModByID(pID));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		if(pAb) {
			pack.Rec.Type = pAb->Type;
			pack.Rec.ParentID = pAb->ParentID;
			if(pAb->FileName.NotEmpty() && fileExists(pAb->FileName)) {
				pack.F.Replace(0, pAb->FileName);
				if(pack.Rec.Name[0] == 0) {
					SPathStruc ps(pAb->FileName);
					ps.Nam.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				}
			}
		}
		SETIFZ(pack.Rec.Type, PPWBTYP_PAGE);
		if(pack.Rec.ParentID) {
			WorkbookTbl::Rec parent_rec;
			if(Fetch(pack.Rec.ParentID, &parent_rec) > 0) {
				if(pack.Rec.Type == PPWBTYP_KEYWORD) {
					if(parent_rec.Type == PPWBTYP_KEYWORD) {
						pack.Rec.ParentID = parent_rec.ParentID;
					}
				}
			}
			else
				pack.Rec.ParentID = 0;
		}
	}
	THROW(CheckDialogPtr(&(dlg = new Workbook2Dialog((pack.Rec.Type == PPWBTYP_KEYWORD) ? DLG_KEYWORD : DLG_WORKBOOK))));
	dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		if(dlg->getDTS(&pack)) {
			if(*pID)
				*pID = pack.Rec.ID;
			if(pack.Rec.Symb[0] == 0) {
				SString code_buf;
				THROW(MakeUniqueCode(code_buf, 1));
				code_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
			}
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

int PPObjWorkbook::Edit(PPID * pID, void * extraPtr)
{
	return Helper_Edit(pID, 0);
}

struct WorkbookAttachFileParam {
	enum {
		ftSelection = 1,
		ftHtml = 2,
		ftText,
		ftVm
	};
	long   Type;
	SString FileName;
};

static int EditWorkbookAttachFileParam(WorkbookAttachFileParam * pData)
{
	class WorkbookAttachFileDialog : public TDialog {
	public:
		WorkbookAttachFileDialog() : TDialog(DLG_WBATTACHF)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_WBATTACHF_FILE, CTL_WBATTACHF_FILE, 1, 0,
				PPTXT_FILPAT_WORKBOOKATTACH, FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		}
		int    setDTS(const WorkbookAttachFileParam * pData)
		{
			Data = *pData;
			SetupStringCombo(this, CTLSEL_WBATTACHF_FT, PPTXT_WBATTACHFILETYPES, Data.Type);
			setCtrlString(CTL_WBATTACHF_FILE, Data.FileName);
			disableCtrl(CTL_WBATTACHF_FILE, Data.Type != Data.ftSelection);
			return 1;
		}
		int    getDTS(WorkbookAttachFileParam * pData)
		{
			Data.Type = getCtrlLong(CTLSEL_WBATTACHF_FT);
			if(Data.Type == Data.ftSelection)
				getCtrlString(CTL_WBATTACHF_FILE, Data.FileName);
			else
				Data.FileName = 0;
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_WBATTACHF_FT)) {
				Data.Type = getCtrlLong(CTLSEL_WBATTACHF_FT);
				disableCtrl(CTL_WBATTACHF_FILE, Data.Type != Data.ftSelection);
			}
			else
				return;
			clearEvent(event);
		}
		WorkbookAttachFileParam Data;
	};
	DIALOG_PROC_BODY(WorkbookAttachFileDialog, pData);
}

int PPObjWorkbook::AttachFile(PPID id, const char * pFileName, int use_ta)
{
	int    ok = 1;
	PPWorkbookPacket pack;
	THROW(GetPacket(id, &pack) > 0);
	THROW_SL(fileExists(pFileName));
	pack.F.Init(Obj);
	pack.F.Replace(0, pFileName);
	THROW(PutPacket(&id, &pack, use_ta));
	CATCHZOKPPERR
	return ok;
}

int PPObjWorkbook::AttachFile(PPID id)
{
	int    ok = -1;
	PPWorkbookPacket pack;
	if(id && GetPacket(id, &pack) > 0 && pack.F.GetCount() == 0) {
		WorkbookAttachFileParam param;
		param.Type = param.ftHtml;
		if(EditWorkbookAttachFileParam(&param) > 0) {
			if(param.Type != param.ftSelection) {
				param.FileName = 0;
				SString ext;
				//SString path;
				//PPGetPath(PPPATH_TEMP, path);
				if(param.Type == param.ftHtml)
					ext = "html";
				else if(param.Type == param.ftText)
					ext = "txt";
				else if(param.Type == param.ftVm)
					ext = "vm";
				if(ext.NotEmpty()) {
					PPMakeTempFileName("wb", ext, 0, param.FileName);
					SFile _f(param.FileName, SFile::mWrite);
					THROW_SL(_f.IsValid());
				}
			}
			THROW_SL(fileExists(param.FileName));
			pack.F.Init(Obj);
			pack.F.Replace(0, param.FileName);
			THROW(PutPacket(&id, &pack, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

PPObjWorkbook::SelectLinkBlock::SelectLinkBlock() : ID(0), AddendumID(0), Type(ltLink)
{
}

int PPObjWorkbook::SelectLinkBlock::GetWbType(PPID * pType, PPID * pAddendumType) const
{
	int    ok = 1;
	PPID   type = 0;
	PPID   addendum_type = 0;
	switch(Type) {
		case ltImage: type = PPWBTYP_MEDIA; break;
		case ltRef: type = PPWBTYP_PAGE; break;
		case ltLink: type = PPWBTYP_PAGE; break;
		case ltAnnot:
			type = PPWBTYP_PAGE;
			addendum_type = PPWBTYP_MEDIA;
			break;
		default:
			ok = 0;
			break;
	}
	ASSIGN_PTR(pType, type);
	ASSIGN_PTR(pAddendumType, addendum_type);
	return ok;
}

int PPObjWorkbook::SelectLink(PPObjWorkbook::SelectLinkBlock * pData)
{
	class SelectLinkItemDialog : public TDialog {
		DECL_DIALOG_DATA(PPObjWorkbook::SelectLinkBlock);
	public:
		SelectLinkItemDialog() : TDialog(DLG_SELWBLINK)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_SELWBLINK_TYPE, 0, PPObjWorkbook::SelectLinkBlock::ltImage);
			AddClusterAssoc(CTL_SELWBLINK_TYPE, 1, PPObjWorkbook::SelectLinkBlock::ltRef);
			AddClusterAssocDef(CTL_SELWBLINK_TYPE, 2, PPObjWorkbook::SelectLinkBlock::ltLink);
			AddClusterAssoc(CTL_SELWBLINK_TYPE, 3, PPObjWorkbook::SelectLinkBlock::ltAnnot);
			SetClusterData(CTL_SELWBLINK_TYPE, Data.Type);
			PPID   wb_type = 0, addendum_wb_type = 0;
			Data.GetWbType(&wb_type, &addendum_wb_type);
			if(wb_type)
				WbObj.SetupItemCombo(this, CTLSEL_SELWBLINK_ITEM, wb_type, Data.ID);
			else
				disableCtrl(CTLSEL_SELWBLINK_ITEM, 1);
			if(addendum_wb_type)
				WbObj.SetupItemCombo(this, CTLSEL_SELWBLINK_IMGITEM, addendum_wb_type, Data.AddendumID);
			else
				disableCtrl(CTLSEL_SELWBLINK_IMGITEM, 1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Data.Type = GetClusterData(CTL_SELWBLINK_TYPE);
			getCtrlData(CTLSEL_SELWBLINK_ITEM, &Data.ID);
			getCtrlData(CTLSEL_SELWBLINK_IMGITEM, &Data.AddendumID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELWBLINK_TYPE)) {
				int    new_type = GetClusterData(CTL_SELWBLINK_TYPE);
				if(new_type != Data.Type) {
					Data.Type = new_type;
					PPID   wb_type = 0, addendum_wb_type = 0;
					Data.GetWbType(&wb_type, &addendum_wb_type);
					disableCtrl(CTLSEL_SELWBLINK_ITEM, !wb_type);
					if(wb_type)
						WbObj.SetupItemCombo(this, CTLSEL_SELWBLINK_ITEM, wb_type, Data.ID = 0);
					setCtrlLong(CTLSEL_SELWBLINK_IMGITEM, 0);
					disableCtrl(CTLSEL_SELWBLINK_IMGITEM, !addendum_wb_type);
					if(addendum_wb_type)
						WbObj.SetupItemCombo(this, CTLSEL_SELWBLINK_IMGITEM, addendum_wb_type, Data.AddendumID = 0);
				}
			}
			else
				return;
			clearEvent(event);
		}
		PPObjWorkbook WbObj;
	};
	DIALOG_PROC_BODY(SelectLinkItemDialog, pData);
}

int PPObjWorkbook::MakeUniqueCode(SString & rBuf, int use_ta)
{
	rBuf.Z();

	int    ok = -1;
	PPObjOpCounter opc_obj;
	PPOpCounter opc_rec;
	if(opc_obj.Search(PPOPCNTR_WORKBOOK, &opc_rec) > 0) {
		char   code[64];
		PPID   _id = 0;
		PPTransaction tra(use_ta);
		THROW(tra);
		do {
			long   cntr = 0;
			PTR32(code)[0] = 0;
			THROW(opc_obj.GetCode(PPOPCNTR_WORKBOOK, &cntr, code, 20, 0, 0));
		} while(SearchBySymb(code, &(_id = 0), 0) > 0);
		THROW(tra.Commit());
		rBuf = code;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::PutPacket(PPID * pID, PPWorkbookPacket * pPack, int use_ta)
{
	assert(pID != 0);
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	PPID   hid = 0;
	PPID   acn_id = 0;
	const  int is_new = (_id == 0 && pPack);
	const  int is_edit = (_id && pPack);
	const  int is_removing = (_id && pPack == 0);
	SString ext_string;
	PPWorkbookPacket org_pack;
	Reference * p_ref = PPRef;
	THROW_PP(!pPack || pPack->Rec.Name[0], PPERR_WORKBOOKEMPTYNAME);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			ext_string = pPack->ExtString;
		}
		if(is_new) {
			{
				PPID   dup_id = 0;
				WorkbookTbl::Rec dup_rec;
				if(pPack->Rec.Symb[0]) {
					THROW_PP_S(SearchBySymb(pPack->Rec.Symb, &dup_id, &dup_rec) < 0, PPERR_DUPNEWWORKBOOKSYMB, pPack->Rec.Symb);
				}
				{
					THROW_PP_S(SearchByName(pPack->Rec.Name, &dup_id, &dup_rec) < 0, PPERR_DUPNEWWORKBOOKNAME, pPack->Rec.Name);
				}
			}
			P_Tbl->copyBufFrom(&pPack->Rec);
			THROW_DB(P_Tbl->insertRec(0, &_id));
			pPack->Rec.ID = _id;
			THROW(p_ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
			if(pPack->F.IsChanged(_id, 0L))
				pPack->F.Save(_id, 0L);
			ASSIGN_PTR(pID, _id);
			acn_id = PPACN_OBJADD;
		}
		else if(is_edit) {
			THROW(GetPacket(_id, &org_pack) > 0);
			if(pPack->IsEq(org_pack))
				ok = -1;
			else {
				THROW(UpdateByID(P_Tbl, Obj, _id, &pPack->Rec, 0));
				THROW(p_ref->Ot.PutList(Obj, _id, &pPack->TagL, 0));
				if(pPack->F.IsChanged(_id, 0L))
					pPack->F.Save(_id, 0L);
				acn_id = PPACN_OBJUPD;
			}
		}
		else if(is_removing) {
			THROW(CheckRights(PPR_DEL));
			{
				int    r = SendObjMessage(DBMSG_OBJDELETE, 0, Obj, _id);
				THROW(r != DBRPL_ERROR);
				THROW_PP(r != DBRPL_CANCEL, PPERR_USERBREAK);
				THROW(RemoveByID(P_Tbl, _id, 0));
			}
			THROW(p_ref->RemoveProperty(Obj, _id, 0, 0));
			THROW(p_ref->Ot.PutList(Obj, _id, 0, 0));
			THROW(RemoveSync(_id));
			{
				ObjLinkFiles _lf(PPOBJ_WORKBOOK);
				_lf.Save(_id, 0L);
			}
			acn_id = PPACN_OBJRMV;
		}
		if(ok > 0) {
			THROW(p_ref->PutPropVlrString(Obj, *pID, WBPRP_EXTSTRDATA, ext_string, 0));
			if(acn_id)
				DS.LogAction(acn_id, Obj, *pID, hid, 0);
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjWorkbook::GetPacket(PPID id, PPWorkbookPacket * pPack)
{
	pPack->TagL.Destroy();
	pPack->F.Clear();
	pPack->ExtString.Z();
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		Reference * p_ref = PPRef;
		THROW(p_ref->Ot.GetList(Obj, id, &pPack->TagL));
		THROW(p_ref->GetPropVlrString(Obj, id, WBPRP_EXTSTRDATA, pPack->ExtString));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::SearchAnalog(const WorkbookTbl::Rec * pSample, PPID * pID, WorkbookTbl::Rec * pRec)
{
	int    ok = -1;
	PPID   analog_id = 0;
	if(pSample->Name[0] && SearchByName(pSample->Name, &analog_id, pRec) > 0) {
		ok = 1;
	}
	else if(pSample->Symb[0] && SearchByName(pSample->Symb, &(analog_id = 0), pRec) > 0) {
		ok = 2;
	}
	else
		analog_id = 0;
	ASSIGN_PTR(pID, analog_id);
	return ok;
}

int PPObjWorkbook::Read(PPObjPack * pPack, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjWorkbook, PPWorkbookPacket>(this, pPack, id, stream, pCtx); }

int PPObjWorkbook::Write(PPObjPack * pPack, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1, r;
	if(pPack && pPack->Data) {
		SString msg_buf, fmt_buf;
		PPWorkbookPacket * p_pack = static_cast<PPWorkbookPacket *>(pPack->Data);
		if(stream == 0) {
			int    is_analog = 0;
			if((*pID) == 0) {
				PPID   same_id = 0;
				WorkbookTbl::Rec same_rec;
				p_pack->Rec.ID = 0;
				if((r = SearchAnalog(&p_pack->Rec, &same_id, &same_rec)) > 0) {
					*pID = same_id;
					is_analog = 1;
					if(r == 1)
						pCtx->OutputString(PPTXT_ACCEPTANALOG_WORKBOOK_NAME, p_pack->Rec.Name);
					else if(r == 2)
						pCtx->OutputString(PPTXT_ACCEPTANALOG_WORKBOOK_SYMB, p_pack->Rec.Symb);
				}
			}
			if(*pID) {
				p_pack->Rec.ID = *pID;
				ObjTagList org_tag_list;
				PPRef->Ot.GetList(Obj, *pID, &org_tag_list);
				org_tag_list.Merge(p_pack->TagL, ObjTagList::mumAdd|ObjTagList::mumUpdate);
				p_pack->TagL = org_tag_list;
				int    r = PutPacket(pID, p_pack, 1);
				if(!r) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTWORKBOOK, p_pack->Rec.ID, p_pack->Rec.Name);
					THROW(*pID);
					ok = -1;
				}
				else if(r > 0)
					ok = 102; // @ObjectUpdated
			}
			else {
				p_pack->Rec.ID = *pID = 0;
				if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTWORKBOOK, p_pack->Rec.ID, p_pack->Rec.Name);
					ok = -1;
				}
				else
					ok = 101; // @ObjectCreated
			}
		}
		else {
			SBuffer buffer;
			p_pack->F.Init(PPOBJ_WORKBOOK);
			p_pack->F.Load(p_pack->Rec.ID, 0L);
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::ProcessObjRefs(PPObjPack * pPack, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(pPack && pPack->Data) {
		PPWorkbookPacket * p_pack = static_cast<PPWorkbookPacket *>(pPack->Data);
		THROW(ProcessObjRefInArray(PPOBJ_WORKBOOK, &p_pack->Rec.ParentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_WORKBOOK, &p_pack->Rec.LinkID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_WORKBOOK, &p_pack->Rec.CssID, ary, replace));
		THROW(p_pack->TagL.ProcessObjRefs(ary, replace));
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::Helper_Export(PPID rootID, PPWorkbookExporter & rExporter, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	PPIDArray local_recur_trace;
	WorkbookTbl::Rec rec;
	for(SEnum en = P_Tbl->EnumByParent(rootID, 0); en.Next(&rec) > 0;) {
		if(rec.ParentID == rootID) { // @paranoic
			if(rec.ID == rec.ParentID) {
				PPSetError(PPERR_WORKBOOKSELFPAR, rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
			}
			else {
				SETIFZ(pRecurTrace, &local_recur_trace);
				int    r = pRecurTrace->addUnique(rec.ID);
				THROW_SL(r);
				if(r > 0) {
					PPWorkbookPacket pack;
					THROW(GetPacket(rec.ID, &pack) > 0);
					THROW(rExporter.ExportPacket(&pack));
					THROW(Helper_Export(rec.ID, rExporter, pRecurTrace)); // @recursion
				}
				else {
					PPSetError(PPERR_WORKBOOKRECUR, rec.Name);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::Export(PPID rootID)
{
	int    ok = 1, r;
	PPWorkbookExporter exporter;
	THROW(r = exporter.Init(0));
	if(r > 0) {
		PPWaitStart();
		THROW(Helper_Export(rootID, exporter, 0));
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjWorkbook::Helper_Transmit(PPID rootID, PPIDArray & rResultList, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	PPIDArray local_recur_trace;
	WorkbookTbl::Rec rec;
	for(SEnum en = P_Tbl->EnumByParent(rootID, 0); en.Next(&rec) > 0;) {
		if(rec.ParentID == rootID) { // @paranoic
			if(rec.ID == rec.ParentID) {
				PPSetError(PPERR_WORKBOOKSELFPAR, rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
			}
			else {
				SETIFZ(pRecurTrace, &local_recur_trace);
				int    r = pRecurTrace->addUnique(rec.ID);
				THROW_SL(r);
				if(r > 0) {
					rResultList.add(rec.ID);
					THROW(Helper_Transmit(rec.ID, rResultList, pRecurTrace)); // @recursion
				}
				else {
					PPSetError(PPERR_WORKBOOKRECUR, rec.Name);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::Transmit(PPID rootID)
{
	int    ok = 1;
	ObjTransmitParam param;
	PPIDArray transmit_list;
	THROW(Helper_Transmit(rootID, transmit_list, 0));
	transmit_list.sortAndUndup();
	if(transmit_list.getCount() && ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		PPObjIDArray objid_ary;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPWaitStart();
		THROW(objid_ary.Add(PPOBJ_WORKBOOK, transmit_list));
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

int PPObjWorkbook::Helper_RemoveAll(PPID id, PPIDArray * pRecurTrace)
{
	int    ok = 1;
	PPIDArray local_recur_trace;
	WorkbookTbl::Rec rec;
	for(SEnum en = P_Tbl->EnumByParent(id, 0); en.Next(&rec) > 0;) {
		if(rec.ParentID == id) { // @paranoic
			if(rec.ID == rec.ParentID) {
				PPSetError(PPERR_WORKBOOKSELFPAR, rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
			}
			else {
				SETIFZ(pRecurTrace, &local_recur_trace);
				int    r = pRecurTrace->addUnique(rec.ID);
				THROW_SL(r);
				if(r > 0) {
					THROW(Helper_RemoveAll(rec.ID, pRecurTrace)); // @recursion
				}
				else {
					PPSetError(PPERR_WORKBOOKRECUR, rec.Name);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
				}
			}
		}
	}
	THROW(RemoveObjV(id, 0, PPObject::no_wait_indicator, 0) > 0)
	CATCHZOK
	return ok;
}

int PPObjWorkbook::RemoveAll()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(Helper_RemoveAll(0, 0))
		THROW(tra.Commit());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}

/*virtual*/const char * PPObjWorkbook::GetNamePtr()
{
	return P_Tbl->data.Name;
}

/*virtual*/void * PPObjWorkbook::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjWorkBookListWindow : public PPObjListWindow {
	public:
		PPObjWorkBookListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaMore;
			SetToolbar(TOOLBAR_LIST_WORKBOOK);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   current_id = 0;
			if(TVCMD != cmaInsert && TVCMD != cmTransmit)
				PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				PPObjWorkbook * p_wb_obj = static_cast<PPObjWorkbook *>(P_Obj);
				getResult(&current_id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmaInsert:
							{
								PPID   id = 0;
								if(p_wb_obj->AddItem(&id, current_id) == cmOK) {
									current_id = id;
									update = 2;
								}
								else
									::SetFocus(H());
							}
							break;
						case cmImport:
							if(current_id) {
								p_wb_obj->ImportFiles(current_id, 0);
								update = 2;
							}
							break;
						case cmExport:
							p_wb_obj->Export(0);
							break;
						case cmTransmit:
							p_wb_obj->Transmit(0);
							break;
						case cmaMore:
							if(current_id) {
								WorkbookTbl::Rec rec;
								if(p_wb_obj->Search(current_id, &rec) > 0) {
									SString file_name, file_ext;
									ObjLinkFiles _f(PPOBJ_WORKBOOK);
									_f.Load(current_id, 0L);
									if(_f.At(0, file_name) <= 0) {
										if(p_wb_obj->AttachFile(current_id) > 0) {
											_f.Init(PPOBJ_WORKBOOK);
											_f.Load(current_id, 0L);
										}
									}
									if(_f.At(0, file_name) > 0) {
										SFileFormat ff;
										ff.Identify(file_name, &file_ext);
										::SetActiveWindow(APPL->H_MainWnd); // @v11.3.2 Без этого вызова окно редактора открывалось как модальное
										if(oneof6(ff, SFileFormat::Jpeg, SFileFormat::Png, SFileFormat::Tiff, SFileFormat::Gif, SFileFormat::Bmp, SFileFormat::Ico)) {
											PPTooltipMessage(rec.Name, file_name, 0/*pBrw->hWnd*/, 5000, 0, SMessageWindow::fTextAlignLeft|
												SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow|
												SMessageWindow::fLargeText|SMessageWindow::fShowOnCenter|SMessageWindow::fPreserveFocus);
										}
										else if(oneof4(ff, SFileFormat::Html, SFileFormat::Txt, SFileFormat::TxtUtf8, SFileFormat::TxtAscii) || file_ext.IsEqiAscii("vm")) {
											PPViewTextBrowser(file_name, rec.Name, "html", TOOLBAR_TEXTBROWSER_WB_HTML);
										}
										else if(oneof2(ff, SFileFormat::Xml, SFileFormat::Ini)) {
											PPViewTextBrowser(file_name, 0, rec.Name);
										}
									}
								}
							}
							break;
						case cmUp:
							break;
						case cmDown:
							break;
						default:
							break;
					}
				}
				else if(TVKEYDOWN) {
					if(TVKEY == kbCtrlF8) {
						if(p_wb_obj && CONFIRMCRIT(PPCFM_REMOVEALLWORKBOOK)) {
							if(p_wb_obj->RemoveAll() > 0)
								update = 2;
						}
					}
#ifndef NDEBUG // {
					if(TVKEY == kbCtrlX) {
						if(p_wb_obj) {
							p_wb_obj->InterchangeUhtt();
							/*
							if(current_id)
								p_wb_obj->ExportToUhtt(current_id);
							*/
						}
					}
#endif // } NDEBUG
				}
				PostProcessHandleEvent(update, current_id);
			}
		}
	};
	return new PPObjWorkBookListWindow(this, flags, extraPtr);
}

int PPObjWorkbook::AddListItem(StrAssocArray * pList, const WorkbookTbl::Rec * pRec, PPIDArray * pRecurTrace)
{
	int    ok = 1, r;
	PPIDArray local_recur_trace;
	if(pList->Search(pRec->ID))
		ok = -1;
	else {
		PPID   par_id = pRec->ParentID;
		WorkbookTbl::Rec par_rec;
		if(par_id) {
			if(par_id == pRec->ID) {
				PPSetError(PPERR_WORKBOOKSELFPAR, pRec->Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
				par_id = 0;
			}
			else if(Fetch(par_id, &par_rec) > 0) {
				SETIFZ(pRecurTrace, &local_recur_trace);
				THROW(r = pRecurTrace->addUnique(par_id));
				//THROW_PP_S(r > 0, PPERR_LOCATIONRECUR, par_rec.Name);
				if(r > 0) {
					THROW(AddListItem(pList, &par_rec, pRecurTrace)); // @recursion
				}
				else {
					PPSetError(PPERR_WORKBOOKRECUR, par_rec.Name);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
					par_id = 0;
				}
			}
			else
				par_id = 0;
		}
		THROW_SL(pList->AddFast(pRec->ID, par_id, pRec->Name));
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::Helper_MakeStrAssocList(PPID parentID, StrAssocArray * pList, UintHashTable & rRecurTrace)
{
	int    ok = 1;
	WorkbookTbl::Rec rec;
	PPIDArray added_id_list;
	for(SEnum en = P_Tbl->EnumByParent(parentID, 0); en.Next(&rec) > 0;) {
		if(rRecurTrace.Has((uint32)rec.ID)) {
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
			CALLEXCEPT_PP_S(PPERR_WORKBOOKRECUR, rec.Name);
		}
		else {
			THROW_SL(pList->AddFast(rec.ID, rec.ParentID, rec.Name));
			THROW_SL(added_id_list.add(rec.ID));
			rRecurTrace.Add((uint32)rec.ID);
		}
	}
	for(uint i = 0; i < added_id_list.getCount(); i++) {
		const PPID _id = added_id_list.get(i);
		THROW(Helper_MakeStrAssocList(_id, pList, rRecurTrace)); // @recursion
	}
	CATCHZOK
	return ok;
}

StrAssocArray * PPObjWorkbook::MakeStrAssocList(void * extraPtr)
{
	PPID   parent_id = reinterpret_cast<PPID>(extraPtr);
	PPIDArray hang_parent_list;
	UintHashTable recur_trace;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	if(parent_id == 0) {
		WorkbookTbl::Rec rec, par_rec;
		for(SEnum en = P_Tbl->Enum(0); en.Next(&rec) > 0;) {
			if(rec.ParentID && Fetch(rec.ParentID, &par_rec) <= 0)
				hang_parent_list.add(rec.ID);
		}
	}
	THROW(Helper_MakeStrAssocList(parent_id, p_list, recur_trace));
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjWorkbook::SetupParentCombo(TDialog * dlg, uint ctlID, int itemType, PPID itemID, PPID parentID)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_combo) {
		SString temp_buf;
		StrAssocArray * p_list = new StrAssocArray;
		WorkbookTbl::Rec rec;
		for(SEnum en = P_Tbl->Enum(0); en.Next(&rec) > 0;) {
			if(CheckParent(itemID, rec.ID)) {
				temp_buf.Z();
				// @v11.1.10 oneof2(itemType, PPWBTYP_PAGE, PPWBTYP_MEDIA)-->oneof4(itemType, PPWBTYP_PAGE, PPWBTYP_MEDIA, PPWBTYP_SITE, PPWBTYP_FOLDER)
				if((oneof4(itemType, PPWBTYP_PAGE, PPWBTYP_MEDIA, PPWBTYP_SITE, PPWBTYP_FOLDER) && oneof3(rec.Type, PPWBTYP_SITE, PPWBTYP_PAGE, PPWBTYP_FOLDER)) ||
					(itemType == PPWBTYP_KEYWORD && rec.Type == PPWBTYP_FOLDER)) {
					GetItemPath(rec.ID, temp_buf);
					p_list->Add(rec.ID, temp_buf);
				}
			}
		}
		p_list->SortByText();
		ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData | lbtDblClkNotify);
		if(p_lw) {
			if(parentID == 0 && itemType == PPWBTYP_MEDIA) {
				PPWorkbookConfig cfg;
				if(PPObjWorkbook::ReadConfig(&cfg) > 0 && cfg.DefImageFolderID && p_list->Search(cfg.DefImageFolderID))
					parentID = cfg.DefImageFolderID;
			}
			p_combo->setListWindow(p_lw, parentID);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPObjWorkbook::SetupCSSCombo(TDialog * dlg, uint ctlID, int itemType, PPID itemID, PPID cssID)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_combo) {
		SString temp_buf;
		StrAssocArray * p_list = new StrAssocArray;
		WorkbookTbl::Rec rec;
		for(SEnum en = P_Tbl->Enum(0); en.Next(&rec) > 0;) {
			temp_buf.Z();
			if(oneof2(itemType, PPWBTYP_SITE, PPWBTYP_PAGE) && (rec.Type == PPWBTYP_CSS)) {
				GetItemPath(rec.ID, temp_buf);
				p_list->Add(rec.ID, temp_buf);
			}
		}
		p_list->SortByText();
		ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData | lbtDblClkNotify);
		if(p_lw) {
			p_combo->setListWindow(p_lw, cssID);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPObjWorkbook::SetupLinkCombo(TDialog * dlg, uint ctlID, int itemType, PPID itemID, PPID linkID)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_combo) {
		SString temp_buf;
		StrAssocArray * p_list = new StrAssocArray;
		WorkbookTbl::Rec rec;
		//PPWorkbookPacket pack;
		for(SEnum en = P_Tbl->Enum(0); en.Next(&rec) > 0;) {
			temp_buf.Z();
			if((itemType == PPWBTYP_SITE) && (rec.Type == PPWBTYP_PAGE)) {
				GetItemPath(rec.ID, temp_buf);
				p_list->Add(rec.ID, temp_buf);
			}
		}
		p_list->SortByText();
		ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData | lbtDblClkNotify);
		if(p_lw) {
			p_combo->setListWindow(p_lw, linkID);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

int PPObjWorkbook::SetupItemCombo(TDialog * dlg, uint ctlID, int itemType, PPID itemID)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctlID));
	if(p_combo) {
		/* @v10.3.0 @fix (redundunt block with memory leak)
		SString temp_buf;
		StrAssocArray * p_list = new StrAssocArray;
		WorkbookTbl::Rec rec;
		for(SEnum en = P_Tbl->EnumByType(itemType, 0); en.Next(&rec) > 0;) {
			GetItemPath(rec.ID, temp_buf);
			p_list->Add(rec.ID, temp_buf);
		}
		*/
		{
			PPObjListWindow * p_olw = 0;
			PPObject * p_obj = GetPPObject(Obj, 0);
			if(p_obj && (p_olw = new PPObjListWindow(p_obj, OLW_CANEDIT|OLW_CANINSERT|OLW_CANSELUPLEVEL, 0)) == 0) {
				ok = PPSetErrorNoMem();
			}
			else {
				p_combo->setListWindow(p_olw, itemID);
				ok = 1;
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int PPObjWorkbook::CheckParent(PPID itemID, PPID parentID)
{
	int    ok = 1;
	WorkbookTbl::Rec rec;
	if(itemID > 0) {
		if(itemID == parentID) {
			ok = 0;
		}
		else {
			for(PPID id = parentID; id && Search(id, &rec) > 0; id = rec.ParentID) {
				if(rec.ID == rec.ParentID) {
					ok = 0;
					break;
				}
				else if(itemID == rec.ParentID) {
					ok = 0;
					break;
				}
			}
		}
	}
	return ok;
}

int PPObjWorkbook::GetItemPath(PPID itemID, SString & rPath)
{
	int   ok = 0;
	WorkbookTbl::Rec rec;
	SString temp_buf;
	rPath.Z();
	if(itemID > 0) {
		for(PPID id = itemID; id && Fetch(id, &rec) > 0; id = rec.ParentID) {
			temp_buf = rPath;
			if(temp_buf.NotEmptyS()) {
				rPath.Z().Cat(rec.Name).CatDiv('>', 1).Cat(temp_buf);
			}
			else {
				rPath = rec.Name;
			}
		}
	}
	return ok;
}

PPObjWorkbook::ImpExpParam::ImpExpParam() : RootID(0), Flags(0)
{
}

int PPObjWorkbook::EditImportParam(PPObjWorkbook::ImpExpParam * pParam)
{
	int    ok = -1;
	ImpExpParam param = *pParam;
	TDialog * dlg = new TDialog(DLG_IMPWBOOK);
	THROW(CheckDialogPtr(&dlg));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_IMPWBOOK_WILDCARD, CTL_IMPWBOOK_WILDCARD, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
	dlg->setCtrlString(CTL_IMPWBOOK_WILDCARD, param.Wildcard);
	dlg->setCtrlString(CTL_IMPWBOOK_RMVPREFIX, param.RmvFileNamePrefix);
	dlg->setCtrlString(CTL_IMPWBOOK_RMVSUFFIX, param.RmvFileNameSuffix);
	dlg->AddClusterAssoc(CTL_IMPWBOOK_FLAGS, 0, param.fRecursive);
	dlg->AddClusterAssoc(CTL_IMPWBOOK_FLAGS, 1, param.fDirAsFolder);
	dlg->AddClusterAssoc(CTL_IMPWBOOK_FLAGS, 2, param.fUpdateByName);
	dlg->AddClusterAssoc(CTL_IMPWBOOK_FLAGS, 3, param.fMedia);
	dlg->SetClusterData(CTL_IMPWBOOK_FLAGS, param.Flags);
	SetupParentCombo(dlg, CTLSEL_IMPWBOOK_ROOT, PPWBTYP_PAGE, 0, param.RootID);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_IMPWBOOK_WILDCARD, param.Wildcard);
		dlg->getCtrlString(CTL_IMPWBOOK_RMVPREFIX, param.RmvFileNamePrefix);
		dlg->getCtrlString(CTL_IMPWBOOK_RMVSUFFIX, param.RmvFileNameSuffix);
		dlg->GetClusterData(CTL_IMPWBOOK_FLAGS, &param.Flags);
		param.RootID = dlg->getCtrlLong(CTLSEL_IMPWBOOK_ROOT);
		if(!param.Wildcard.NotEmptyS()) {
			PPError(PPERR_WBIMP_INVWILDCARD, pParam->Wildcard);
		}
		else {
			*pParam = param;
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjWorkbook::Helper_Import(PPID rootID, const PPObjWorkbook::ImpExpParam & rParam, const SString & rBasePath, const SString & rNakedWc)
{
	int    ok = 1;
	SString wildcard, name_buf, file_name;
	SPathStruc ps;
	SDirEntry de;
	(wildcard = rBasePath).SetLastSlash().Cat(rNakedWc);
	{
		for(SDirec direc(wildcard, 0); direc.Next(&de) > 0;) {
			if(de.IsFile()) {
				PPWorkbookPacket pack;
				name_buf = de.FileName;
				PPWaitMsg(name_buf);
				ps.Split(name_buf);
				name_buf = ps.Nam;
				if(rParam.RmvFileNamePrefix.NotEmpty() && name_buf.CmpPrefix(rParam.RmvFileNamePrefix, 1) == 0) {
					name_buf.ShiftLeft(rParam.RmvFileNamePrefix.Len());
				}
				if(rParam.RmvFileNameSuffix.NotEmpty() && name_buf.CmpSuffix(rParam.RmvFileNameSuffix, 1) == 0) {
					if(name_buf.Len() >= rParam.RmvFileNameSuffix.Len()) {
						size_t trim_len = name_buf.Len() - rParam.RmvFileNameSuffix.Len();
						name_buf.Trim(trim_len);
					}
					else
						name_buf.Z();
				}
				if(name_buf.NotEmptyS()) {
					name_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
					PPID   id = 0;
					PPID   same_id = 0;
					WorkbookTbl::Rec same_rec;
					if(SearchByName(pack.Rec.Name, &same_id, &same_rec) > 0) {
						if(rParam.Flags & rParam.fUpdateByName) {
							pack.Rec = same_rec;
							id = same_id;
						}
						else
							id = -1;
					}
					if(id >= 0) {
						pack.Rec.ParentID = rootID;
						if(rParam.Flags & rParam.fMedia) {
							pack.Rec.Type = PPWBTYP_MEDIA;
						}
						else {
							pack.Rec.Type = PPWBTYP_PAGE;
						}
						(file_name = rBasePath).SetLastSlash().Cat(de.FileName);
						pack.F.Init(Obj);
						if(id)
							pack.F.Load(id, 0L);
						pack.F.Replace(0, file_name);
						if(MakeUniqueCode(name_buf, 0) > 0) {
							name_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
						}
						else {
							// @todo logger
						}
						if(!PutPacket(&id, &pack, 0)) {
							// @todo logger
						}
					}
				}
			}
		}
	}
	if(rParam.Flags & rParam.fRecursive) {
		(wildcard = rBasePath).SetLastSlash().Cat("*.*");
		for(SDirec direc(wildcard, 1); direc.Next(&de) > 0;) {
			if(de.Attr & 0x10 && !sstreq(de.FileName, ".") && !sstreq(de.FileName, "..")) {
				PPID   root_id = rootID;
				if(rParam.Flags & rParam.fDirAsFolder && !(rParam.Flags & rParam.fMedia)) {
					PPWorkbookPacket pack;
					name_buf = de.FileName;
					PPWaitMsg(name_buf);
					ps.Split(name_buf);
					name_buf = ps.Nam;
					if(rParam.RmvFileNamePrefix.NotEmpty() && name_buf.CmpPrefix(rParam.RmvFileNamePrefix, 1) == 0) {
						name_buf.ShiftLeft(rParam.RmvFileNamePrefix.Len());
					}
					if(rParam.RmvFileNameSuffix.NotEmpty() && name_buf.CmpSuffix(rParam.RmvFileNameSuffix, 1) == 0) {
						if(name_buf.Len() >= rParam.RmvFileNameSuffix.Len()) {
							size_t trim_len = name_buf.Len() - rParam.RmvFileNameSuffix.Len();
							name_buf.Trim(trim_len);
						}
						else
							name_buf.Z();
					}
					if(name_buf.NotEmptyS()) {
						name_buf.Space().CatChar('(').Cat("folder").CatChar(')');
						name_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
						PPID   id = 0;
						PPID   same_id = 0;
						WorkbookTbl::Rec same_rec;
						if(SearchByName(pack.Rec.Name, &same_id, &same_rec) > 0) {
							if(rParam.Flags & rParam.fUpdateByName) {
								pack.Rec = same_rec;
								id = same_id;
							}
							else
								id = -1;
						}
						if(id >= 0) {
							pack.Rec.ParentID = rootID;
							pack.Rec.Type = PPWBTYP_PAGE;
							(file_name = rBasePath).SetLastSlash().Cat(de.FileName);
							pack.F.Init(Obj);
							if(id)
								pack.F.Load(id, 0L);
							pack.F.Replace(0, file_name);
							if(MakeUniqueCode(name_buf, 0) > 0) {
								name_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
							}
							else {
								// @todo logger
							}
							if(PutPacket(&id, &pack, 0)) {
								root_id = id;
							}
							else {
								// @todo logger
							}
						}
					}
				}
				(wildcard = rBasePath).SetLastSlash().Cat(de.FileName);
				THROW(Helper_Import(root_id, rParam, wildcard, rNakedWc)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

LDATETIME PPObjWorkbook::GetLastModifTime(PPID id)
{
	LDATETIME mod_dtm = ZERODATETIME;
	if(id) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj) {
			int    is_creation = 0;
			p_sj->GetLastObjModifEvent(PPOBJ_WORKBOOK, id, &mod_dtm, &is_creation, 0);
		}
	}
	return mod_dtm;
}

LDATETIME PPObjWorkbook::GetContentLastModifTime(PPID id)
{
	LDATETIME mod_dtm = ZERODATETIME;
	if(id) {
		SString temp_buf;
		ObjLinkFiles _lf(PPOBJ_WORKBOOK);
		_lf.Load(id, 0L);
		if(_lf.At(0, temp_buf.Z())) {
			SFile::GetTime(temp_buf, 0, 0, &mod_dtm);
		}
	}
	return mod_dtm;
}

PPObjWorkbook::ProcessUhttImportBlock::ProcessUhttImportBlock(PPUhttClient & rUc,
	const TSCollection <UhttWorkbookItemPacket> & rSrcList, LDATETIME sinceEvTm) :
	R_Uc(rUc), R_SrcList(rSrcList), SinceEvTime(sinceEvTm)
{
}

int PPObjWorkbook::Helper_UhttToNativePacket(ProcessUhttImportBlock & rBlk, const UhttWorkbookItemPacket * pUhttPacket, const SString & rSymb, PPWorkbookPacket & rPack)
{
	int    ok = 1;
	STRNSCPY(rPack.Rec.Name, pUhttPacket->Name);
	STRNSCPY(rPack.Rec.Symb, rSymb);
	rPack.Rec.Dt = pUhttPacket->Dtm;
	rPack.Rec.Tm = pUhttPacket->Dtm;
	STRNSCPY(rPack.Rec.Version, pUhttPacket->Version);
	rPack.PutExtStrData(WBEXSTR_DESCRIPTION, pUhttPacket->Descr);
	rPack.Rec.Rank = pUhttPacket->Rank;
	rPack.Rec.Flags = pUhttPacket->Flags;
	rPack.Rec.Type = pUhttPacket->Type;
	rPack.Rec.KeywordCount = pUhttPacket->KeywordCount;
	rPack.Rec.KeywordDilute = pUhttPacket->KeywordDilute;
	if(pUhttPacket->TagList.getCount()) {
		PPObjTag tag_obj;
		PPID   tag_id = 0;
		PPObjectTag tag_rec;
		for(uint i = 0; i < pUhttPacket->TagList.getCount(); i++) {
			const UhttTagItem * p_ut_item = pUhttPacket->TagList.at(i);
			if(p_ut_item && tag_obj.SearchBySymb(p_ut_item->Symb, &tag_id, &tag_rec) > 0) {
				assert(tag_id == tag_rec.ID); // @paranoic
				THROW(rPack.TagL.PutItemStr(tag_id, p_ut_item->Value));
			}
		}
	}
	if(pUhttPacket->ParentID) {
		for(uint i = 0; i < rBlk.R_SrcList.getCount(); i++) {
			const UhttWorkbookItemPacket * p_link_item = rBlk.R_SrcList.at(i);
			if(p_link_item && p_link_item->ID == pUhttPacket->ParentID) {
				PPID   temp_id = 0;
				int rr = Helper_CreatePacketByUhttList(&temp_id, rBlk, i, 0);
				THROW(rr);
				rPack.Rec.ParentID = temp_id;
			}
		}
	}
	// @todo pUhttPacket->LinkID
	// @todo pUhttPacket->CssID
	{
		const LDATETIME foreign_content_mod_dtm = pUhttPacket->ContentModifDtm;
		const LDATETIME content_mod_dtm = GetContentLastModifTime(rPack.Rec.ID);
		if(!content_mod_dtm || (cmp(foreign_content_mod_dtm, content_mod_dtm) > 0 && cmp(foreign_content_mod_dtm, rBlk.SinceEvTime) > 0)) {
			SString temp_path_name;
			SString temp_buf;
			PPGetPath(PPPATH_TEMP, temp_path_name);
			(temp_buf = temp_path_name).SetLastSlash();
			if(pUhttPacket->Symb.NotEmpty())
				temp_buf.Cat(pUhttPacket->Symb);
			else
				temp_buf.Cat(pUhttPacket->ID);
			const int gcr = rBlk.R_Uc.GetWorkbookContentByID_ToFile(pUhttPacket->ID, temp_buf);
			if(gcr > 0) {
				rPack.F.Init(Obj);
				THROW(rPack.F.Replace(0, temp_buf));
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::Helper_CreatePacketByUhttList(PPID * pID, ProcessUhttImportBlock & rBlk, uint srcListPos, int use_ta)
{
	int    ok = -1;
	PPID   result_id = 0;
	SString temp_buf;
    const UhttWorkbookItemPacket * p_item = rBlk.R_SrcList.at(srcListPos);
	if(p_item) {
		PPID   temp_id = 0;
		if(rBlk.ProcessedList.Search(p_item->ID, &temp_id, 0)) {
			result_id = temp_id;
		}
		else {
			temp_buf = p_item->Symb;
			if(!temp_buf.NotEmptyS()) {
				temp_buf.Z().CatEq("UHTTID", (long)p_item->ID);
			}
			PPID   ex_id = 0;
			WorkbookTbl::Rec ex_rec;
			PPWorkbookPacket pack;
			if(SearchBySymb(temp_buf, &ex_id, &ex_rec) > 0) {
				const LDATETIME foreign_mod_dtm = p_item->ModifDtm;
				const LDATETIME mod_dtm = GetLastModifTime(ex_id);
				if(cmp(foreign_mod_dtm, mod_dtm) > 0 && cmp(foreign_mod_dtm, rBlk.SinceEvTime) > 0) {
					THROW(GetPacket(ex_id, &pack) > 0);
					THROW(Helper_UhttToNativePacket(rBlk, p_item, temp_buf, pack));
					result_id = ex_id;
					THROW(PutPacket(&result_id, &pack, 1));
					ok = 2;
				}
				else {
					result_id = ex_id;
				}
				THROW_SL(rBlk.ProcessedList.Add(p_item->ID, result_id, 0));
			}
			else {
				THROW(Helper_UhttToNativePacket(rBlk, p_item, temp_buf, pack));
				THROW(PutPacket(&result_id, &pack, 1));
				THROW_SL(rBlk.ProcessedList.Add(p_item->ID, result_id, 0));
				ok = 1;
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pID, result_id);
	return ok;
}

int PPObjWorkbook::InterchangeUhtt()
{
	int    ok = -1;
	int    is_allowed = 0;
	SString msg_buf;
	PPLogger logger;
	{
		PPIniFile ini_file;
		int    yes = 0;
		if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_WORKBOOKINTERCHANGEUHTT, &yes) > 0 && yes == 1)
			is_allowed = 1;
	}
	if(is_allowed) {
		PPWaitStart();
		SString temp_buf;
		SString last_err_msg_buf;
		PPIDArray to_transmit_list;
		PPIDArray native_list;
		PPIDArray updated_list;
		PPWorkbookConfig cfg;
		PPObjWorkbook::ReadConfig(&cfg);
		LDATETIME since_ev = ZERODATETIME;
		if(cfg.UhttXEvTokID) {
			SString ev_msg_buf;
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(p_sj) {
				LDATETIME moment;
				PPIDArray acn_list;
				acn_list.add(PPACN_EVENTTOKEN);
				if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, cfg.UhttXEvTokID, &acn_list, &moment) > 0) {
					since_ev = moment;
					// PPTXT_LOG_WBS_LASTEVENTFOUND   "Синхронизация рабочих книг: последнее найденное событие @datetime"
					logger.Log(PPFormatT(PPTXT_LOG_WBS_LASTEVENTFOUND, &msg_buf, moment));
				}
			}
		}
		{
			PPUhttClient uhtt_cli;
			TSCollection <UhttWorkbookItemPacket> result;
			THROW(uhtt_cli.Auth());
			// PPTXT_LOG_WBS_GETTINGFROMUHTT  "Получение списка рабочих книг с сервера"
			logger.Log(PPFormatT(PPTXT_LOG_WBS_GETTINGFROMUHTT, &msg_buf));
			THROW(uhtt_cli.GetWorkbookListByParentCode(0, result));
			// PPTXT_LOG_WBS_LISTVGOTFROMUHTT "Получен список рабочих книг с сервера в количестве @int записей"
			logger.Log(PPFormatT(PPTXT_LOG_WBS_LISTVGOTFROMUHTT, &msg_buf, (long)result.getCount()));
			{
				ProcessUhttImportBlock blk(uhtt_cli, result, since_ev);
				int    accept_err_occured = 0;
				{
					for(uint i = 0; i < result.getCount(); i++) {
						PPID   native_id = 0;
						const  UhttWorkbookItemPacket * p_item = blk.R_SrcList.at(i);
						int    cpr = Helper_CreatePacketByUhttList(&native_id, blk, i, 1);
						// @v10.2.5 THROW(cpr);
						if(cpr > 0) {
							updated_list.add(native_id);
							// PPTXT_LOG_WBS_WBVACCEPTED      "Рабочая книга '@zstr' акцептирована в базе данных"
							logger.Log(PPFormatT(PPTXT_LOG_WBS_WBVACCEPTED, &msg_buf, p_item->Name.cptr()));
						}
						// @v10.2.5 {
						else if(!cpr) {
							PPGetMessage(mfError, PPErrCode, 0, DS.CheckExtFlag(ECF_SYSSERVICE), last_err_msg_buf);
							temp_buf.Z().Cat(p_item->ID).CatDiv('-', 1).Cat(p_item->Symb).CatDiv('-', 1).Cat(p_item->Name);
							PPFormatT(PPTXT_LOG_WBS_WBVACCEPTERROR, &msg_buf, temp_buf.cptr());
							msg_buf.CatDiv(':', 2).Cat(last_err_msg_buf);
							logger.Log(msg_buf);
							accept_err_occured = 1;
						}
						// } @v10.2.5
						ok = 1;
					}
					updated_list.sortAndUndup();
				}
				if(!accept_err_occured) { // @v10.2.5
					WorkbookTbl::Rec rec;
					SString native_symb;
					THROW(Helper_Transmit(0, native_list, 0));
					for(uint j = 0; j < native_list.getCount(); j++) {
						const PPID native_id = native_list.get(j);
						if(!updated_list.bsearch(native_id) && Fetch(native_id, &rec) > 0) {
							native_symb = rec.Symb;
							if(native_symb.NotEmpty()) {
								int    found = 0;
								for(uint i = 0; !found && i < result.getCount(); i++) {
									const UhttWorkbookItemPacket * p_item = result.at(i);
									temp_buf = p_item->Symb;
									if(!temp_buf.NotEmptyS())
										temp_buf.Z().CatEq("UHTTID", (long)p_item->ID);
									if(temp_buf == native_symb) {
										const LDATETIME foreign_mod_dtm = p_item->ModifDtm;
										const LDATETIME mod_dtm = GetLastModifTime(native_id);
										if(cmp(mod_dtm, foreign_mod_dtm) > 0 && cmp(mod_dtm, blk.SinceEvTime) > 0)
											to_transmit_list.add(native_id);
										else {
											const LDATETIME foreign_content_mod_dtm = p_item->ContentModifDtm;
											const LDATETIME content_mod_dtm = GetContentLastModifTime(native_id);
											if(cmp(content_mod_dtm, foreign_content_mod_dtm) > 0 && cmp(content_mod_dtm, blk.SinceEvTime) > 0)
												to_transmit_list.add(native_id);
										}
										found = 1;
									}
								}
								if(!found)
									to_transmit_list.add(native_id);
							}
						}
					}
					to_transmit_list.sortAndUndup();
					for(uint tidx = 0; tidx < to_transmit_list.getCount(); tidx++) {
						const PPID to_transmit_id = to_transmit_list.get(tidx);
						const int  er = Helper_ExportToUhtt(uhtt_cli, to_transmit_id, &result, &logger);
						if(!er)
							logger.LogLastError();
					}
				}
				if(cfg.UhttXEvTokID) {
					DS.LogAction(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, cfg.UhttXEvTokID, 0, 1);
				}
			}
		}
		PPWaitStop();
	}
	CATCH
		logger.LogLastError();
		PPWaitStop();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjWorkbook::Helper_ExportToUhtt(PPUhttClient & rUc, PPID id,
	const TSCollection <UhttWorkbookItemPacket> * pForeignList, PPLogger * pLogger)
{
	int    ok = 1;
	SString temp_buf;
	SString symb_buf;
	SString msg_buf;
	PPWorkbookPacket pack;
	WorkbookTbl::Rec parent_rec;
	PPID   uhtt_id = 0;
	SString attach_file_name;
	UhttWorkbookItemPacket uhtt_pack;
	THROW(GetPacket(id, &pack) > 0);
	uhtt_pack.ID = 0;
	uhtt_pack.Type = pack.Rec.Type;
	uhtt_pack.ParentID = 0;
	if(pack.Rec.ParentID && Search(pack.Rec.ParentID, &parent_rec) > 0 && parent_rec.Symb[0]) {
		symb_buf = parent_rec.Symb;
		if(pForeignList) {
			for(uint i = 0; !uhtt_pack.ParentID && i < pForeignList->getCount(); i++) {
				const UhttWorkbookItemPacket * p_uhtt_item = pForeignList->at(i);
				if(p_uhtt_item) {
					temp_buf = p_uhtt_item->Symb;
					if(!temp_buf.NotEmptyS()) {
						temp_buf.Z().CatEq("UHTTID", (long)p_uhtt_item->ID);
					}
					if(symb_buf.CmpNC(temp_buf) == 0) {
						uhtt_pack.ParentID = p_uhtt_item->ID;
					}
				}
			}
		}
	}
	uhtt_pack.LinkID = 0;
	uhtt_pack.CssID = 0;
	uhtt_pack.Rank = pack.Rec.Rank;
	uhtt_pack.Flags = pack.Rec.Flags;
	uhtt_pack.KeywordCount = pack.Rec.KeywordCount;
	uhtt_pack.KeywordDilute = pack.Rec.KeywordDilute;
	uhtt_pack.Dtm = pack.Rec.Dt;
	uhtt_pack.Dtm = pack.Rec.Tm;
	uhtt_pack.SetName(pack.Rec.Name);
	uhtt_pack.SetSymb(pack.Rec.Symb);
	uhtt_pack.SetVersion(pack.Rec.Version);
	pack.GetExtStrData(WBEXSTR_DESCRIPTION, temp_buf);
	uhtt_pack.SetDescr(temp_buf);
	uhtt_pack.ModifDtm = GetLastModifTime(id);
	{
		LDATETIME mod_dtm = ZERODATETIME;
		ObjLinkFiles _f(PPOBJ_WORKBOOK);
		_f.Load(pack.Rec.ID, 0L);
		if(_f.At(0, temp_buf.Z())) {
			if(fileExists(temp_buf)) {
				attach_file_name = temp_buf;
				SFile::GetTime(attach_file_name, 0, 0, &mod_dtm);
			}
		}
		uhtt_pack.ContentModifDtm = mod_dtm;
	}
	{
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		for(uint i = 0; i < pack.TagL.GetCount(); i++) {
			const ObjTagItem * p_tag_item = pack.TagL.GetItemByPos(i);
			if(p_tag_item && p_tag_item->GetStr(temp_buf) && temp_buf.NotEmptyS()) {
				if(tag_obj.Fetch(p_tag_item->TagID, &tag_rec) > 0) {
					symb_buf = tag_rec.Symb;
					if(symb_buf.NotEmptyS()) {
						UhttTagItem * p_new_item = new UhttTagItem(symb_buf, temp_buf);
						THROW_MEM(p_new_item);
						THROW_SL(uhtt_pack.TagList.insert(p_new_item));
					}
				}
			}
		}
	}
	THROW(rUc.CreateWorkbookItem(&uhtt_id, uhtt_pack));
	{
		// PPTXT_LOG_WBS_WBVSENTTOUHTT    "Рабочая книга '@zstr' передана на сервер"
		(temp_buf = uhtt_pack.Name).Transf(CTRANSF_UTF8_TO_INNER);
		CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_WBS_WBVSENTTOUHTT, &msg_buf, temp_buf.cptr())));
	}
	if(attach_file_name.NotEmpty()) {
		THROW(rUc.SetWorkbookContentByID(uhtt_id, attach_file_name));
		{
			// PPTXT_LOG_WBS_WBCVSENTTOUHTT   "Содержимое рабочей книги '@zstr' передано на сервер"
			(temp_buf = uhtt_pack.Name).Transf(CTRANSF_UTF8_TO_INNER);
			CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_WBS_WBCVSENTTOUHTT, &msg_buf, temp_buf.cptr())));
		}
		ok = 3;
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::ExportToUhtt(PPID id)
{
	int    ok = -1;
	PPUhttClient uhtt_cli;
	THROW(uhtt_cli.Auth());
	THROW(Helper_ExportToUhtt(uhtt_cli, id, 0, 0));
	CATCHZOK
	return ok;
}

int PPObjWorkbook::TestImportFromUhtt()
{
	int    ok = -1;
	PPUhttClient uhtt_cli;
	if(uhtt_cli.Auth()) {
		TSCollection <UhttWorkbookItemPacket> result;
		SString line_buf;
		SString temp_path_name;
		PPGetPath(PPPATH_TEMP, temp_path_name);
		temp_path_name.SetLastSlash().Cat("test-workbook");
		int    r;
		int    gcr = 0;
		THROW_SL(createDir(temp_path_name));
		THROW(r = uhtt_cli.GetWorkbookListByParentCode(0, result));
		for(uint i = 0; i < result.getCount(); i++) {
            const UhttWorkbookItemPacket * p_item = result.at(i);
			if(p_item) {
				line_buf.Z().Cat(p_item->ID).Tab().Cat(p_item->Symb).Tab().Cat(p_item->Dtm, DATF_ISO8601, 0).Tab().
					Cat(p_item->Name);
				PPLogMessage(PPFILNAM_DEBUG_LOG, line_buf, LOGMSGF_TIME);
				//
				(line_buf = temp_path_name).SetLastSlash();
				if(p_item->Symb.NotEmpty())
					line_buf.Cat(p_item->Symb);
				else
					line_buf.Cat(p_item->ID);
				gcr = uhtt_cli.GetWorkbookContentByID_ToFile(p_item->ID, line_buf);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook::ImportFiles(PPID rootID, PPObjWorkbook::ImpExpParam * pParam)
{
	int    ok = -1;
	PPObjWorkbook::ImpExpParam param;
	if(pParam)
		pParam->RootID = rootID;
	else
		param.RootID = rootID;
	if(pParam || EditImportParam(&param) > 0) {
		WorkbookTbl::Rec root_rec;
		SETIFZ(pParam, &param);
		THROW_PP_S(pParam->Wildcard.NotEmpty(), PPERR_WBIMP_INVWILDCARD, pParam->Wildcard);
		THROW(Search(pParam->RootID, &root_rec) > 0);
		THROW(oneof2(root_rec.Type, PPWBTYP_SITE, PPWBTYP_FOLDER));
		{
			SPathStruc ps_;
			PPWaitStart();
			SPathStruc ps(pParam->Wildcard);
			SString base_path, naked_wildcard;
			ps_.Merge(&ps, SPathStruc::fDrv|SPathStruc::fDir, base_path);
			ps_.Z().Merge(&ps, SPathStruc::fNam|SPathStruc::fExt, naked_wildcard);
			if(!naked_wildcard.NotEmptyS())
				naked_wildcard = "*.*";
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(Helper_Import(pParam->RootID, *pParam, base_path, naked_wildcard));
				THROW(tra.Commit());
			}
			PPWaitStop();
		}
	}
	CATCHZOKPPERR
	return ok;
}

class WorkbookCache : public ObjCache {
public:
	WorkbookCache() : ObjCache(PPOBJ_WORKBOOK, sizeof(Workbook2Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Workbook2Data : public ObjCacheEntry {
		long   Rank;
		long   Type;           //
		long   Flags;          // @flags
		long   ParentID;       //
		long   LinkID;
		long   CssID;
	};
};

int WorkbookCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Workbook2Data * p_cache_rec = static_cast<Workbook2Data *>(pEntry);
	PPObjWorkbook wb_obj;
	WorkbookTbl::Rec rec;
	if(wb_obj.Search(id, &rec) > 0) {
		p_cache_rec->Rank = rec.Rank;
		p_cache_rec->Type  = rec.Type;
		p_cache_rec->ParentID = rec.ParentID;
		p_cache_rec->LinkID = rec.LinkID;
		p_cache_rec->CssID = rec.CssID;
		p_cache_rec->Flags = rec.Flags;
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void WorkbookCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	WorkbookTbl::Rec * p_data_rec = static_cast<WorkbookTbl::Rec *>(pDataRec);
	const Workbook2Data * p_cache_rec = static_cast<const Workbook2Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->ID       = p_cache_rec->ID;
	p_data_rec->Rank     = p_cache_rec->Rank;
	p_data_rec->Type     = p_cache_rec->Type;
	p_data_rec->ParentID = p_cache_rec->ParentID;
	p_data_rec->LinkID   = p_cache_rec->LinkID;
	p_data_rec->CssID    = p_cache_rec->CssID;
	p_data_rec->Flags    = p_cache_rec->Flags;
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}
// }

int PPObjWorkbook::Fetch(PPID id, WorkbookTbl::Rec * pRec)
{
	WorkbookCache * p_cache = GetDbLocalCachePtr <WorkbookCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(WORKBOOK, PPWorkbookImpExpParam);

PPWorkbookImpExpParam::PPWorkbookImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
}

class PrcssrWorkbookImport {
public:
	struct Param {
		SString CfgName;
	};
	PrcssrWorkbookImport();
	int    InitParam(Param *);
	int    EditParam(Param *);
	int    Init(const Param *);
	int    Run();
private:
	int    ResolveLink(const char * pLinkName, const char * pLinkSymb, PPID * pID);

	PPObjWorkbook WbObj;
	Param  P;
	PPWorkbookImpExpParam IeParam;
};

int EditWorkbookImpExpParams()
{
	int    ok = -1;
	PPWorkbookImpExpParam param;
	ImpExpParamDialog * dlg = new ImpExpParamDialog(DLG_IMPEXP, 0);
	THROW(CheckDialogPtr(&dlg));
	THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_WORKBOOK, &param, dlg));
	CATCHZOK
	delete dlg;
	return ok;
}

static int SelectWorkbookImpExpConfig(PPWorkbookImpExpParam * pParam, int import)
{
	int    ok = -1;
	uint   p = 0;
	long   id = 0;
	SString temp_buf;
	StrAssocArray list;
	PPWorkbookImpExpParam param;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_WORKBOOK, &param, &list, import ? 2 : 1));
	id = (list.SearchByText(pParam->Name, 1, &p) > 0) ? (uint)list.Get(p).Id : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, temp_buf));
	{
		PPIniFile ini_file(temp_buf, 0, 1, 1);
		while(ok < 0 && ListBoxSelDialog(&list, import ? PPTXT_TITLE_WORKBOOKIMPCFG : PPTXT_TITLE_WORKBOOKEXPCFG, &id, 0) > 0) {
			if(id) {
				list.GetText(id, temp_buf);
				pParam->ProcessName(1, temp_buf);
				pParam->ReadIni(&ini_file, temp_buf, 0);
				ok = 1;
			}
			else
				PPError(PPERR_INVWORKBOOKIMPEXPCFG);
		}
	}
	CATCHZOK
	return ok;
}

PrcssrWorkbookImport::PrcssrWorkbookImport()
{
}

int PrcssrWorkbookImport::InitParam(Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		pParam->CfgName = 0;
		ok = 1;
	}
	return ok;
}

int PrcssrWorkbookImport::EditParam(Param * pParam)
{
	int   ok = -1;
	if(SelectWorkbookImpExpConfig(&IeParam, 1) > 0) {
		pParam->CfgName = IeParam.Name;
		ok = 1;
	}
	return ok;
}

int PrcssrWorkbookImport::Init(const Param * pParam)
{
	P = *pParam;
	int    ok = 1;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_WORKBOOK, &IeParam.InrRec));
		IeParam.Direction = 1; // import
		THROW(IeParam.ProcessName(1, P.CfgName));
		THROW(IeParam.ReadIni(&ini_file, P.CfgName, 0));
	}
	CATCHZOK
	return ok;
}

int PrcssrWorkbookImport::ResolveLink(const char * pLinkName, const char * pLinkSymb, PPID * pID)
{
	PPID   id = 0, same_id = 0;
	WorkbookTbl::Rec same_rec;
	if(!isempty(pLinkName) && WbObj.SearchByName(pLinkName, &same_id, &same_rec) > 0) {
		id = same_id;
	}
	else if(!isempty(pLinkSymb) && WbObj.SearchBySymb(pLinkSymb, &same_id, &same_rec) > 0) {
		id = same_id;
	}
	ASSIGN_PTR(pID, id);
	return id ? 1 : -1;
}

int PrcssrWorkbookImport::Run()
{
	int    ok = -1, ta = 0;
	long   numrecs = 0;
	SString log_msg, fmt_buf, file_name, temp_buf;
	PPLogger logger;
	PPImpExp ie(&IeParam, 0);
	PPWaitStart();
	THROW(ie.OpenFileForReading(0));
	THROW(ie.GetNumRecs(&numrecs));
	if(numrecs) {
		int    r;
		IterCounter cntr;
		PPTransaction tra(1);
		THROW(tra);
		{
			Sdr_Workbook sdr_rec;
			cntr.Init(numrecs);
			// @v10.7.9 @ctr MEMSZERO(sdr_rec);
			while((r = ie.ReadRecord(&sdr_rec, sizeof(sdr_rec))) > 0) {
				ie.GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &sdr_rec);
				PPID   id = 0, same_id = 0;
				PPWorkbookPacket pack;
				WorkbookTbl::Rec same_rec;
				STRNSCPY(pack.Rec.Name, sdr_rec.Name);
				if(pack.Rec.Name[0]) {
					STRNSCPY(pack.Rec.Symb, sdr_rec.Symb);
					pack.Rec.Type = NZOR(sdr_rec.Type, PPWBTYP_PAGE);
					pack.Rec.Flags = sdr_rec.Flags;
					if(pack.Rec.Type == PPWBTYP_KEYWORD) {
						pack.Rec.KeywordCount  = static_cast<int16>(sdr_rec.KeywordCount);
						pack.Rec.KeywordDilute = static_cast<int16>(sdr_rec.KeywordDilute);
					}
					pack.Rec.Rank  = sdr_rec.Order;
					if(WbObj.SearchByName(pack.Rec.Name, &same_id, &same_rec) > 0) {
						id = -1;
					}
					else if(pack.Rec.Symb[0] && WbObj.SearchBySymb(pack.Rec.Symb, &same_id, &same_rec) > 0) {
						id = -1;
					}
					else {
						if(pack.Rec.Symb[0] == 0 && WbObj.MakeUniqueCode(temp_buf, 0) > 0) {
							temp_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
						}
						THROW(ResolveLink(sdr_rec.ParentName, sdr_rec.ParentSymb, &pack.Rec.ParentID));
						THROW(ResolveLink(sdr_rec.LinkName, sdr_rec.LinkSymb, &pack.Rec.LinkID));
						THROW(ResolveLink(sdr_rec.CssName, sdr_rec.CssSymb, &pack.Rec.CssID));
						//
						file_name = sdr_rec.Content;
						if(file_name.NotEmptyS() && fileExists(file_name)) {
							pack.F.Init(PPOBJ_WORKBOOK);
							if(id > 0)
								pack.F.Load(id, 0L);
							pack.F.Replace(0, file_name);
						}
						if(!WbObj.PutPacket(&id, &pack, 0)) {
							logger.LogLastError();
						}
					}
				}
				PPWaitPercent(cntr.Increment());
			}
			THROW(r);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	PPWaitStop();
	return ok;
}

int ImportWorkbook()
{
	int    ok = -1;
	PrcssrWorkbookImport prcssr;
	PrcssrWorkbookImport::Param param;
	prcssr.InitParam(&param);
	while(ok < 0 && prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			PPError();
	return ok;
}
//
//
//
PPWorkbookExporter::PPWorkbookExporter() : P_IEWorkbook(0)
{
}

PPWorkbookExporter::~PPWorkbookExporter()
{
	delete P_IEWorkbook;
}

int PPWorkbookExporter::Init(const PPWorkbookImpExpParam * pParam)
{
	int    ok = 1;
	DestFilesPath = 0;
	if(pParam)
		Param = *pParam;
	if(!pParam) {
		THROW(LoadSdRecord(PPREC_WORKBOOK, &Param.InrRec));
		ok = SelectWorkbookImpExpConfig(&Param, 0);
	}
	if(ok > 0) {
		THROW_MEM(P_IEWorkbook = new PPImpExp(&Param, 0));
		THROW(P_IEWorkbook->OpenFileForWriting(0, 1));
		{
			SPathStruc ps(P_IEWorkbook->GetParamConst().FileName);
			ps.Ext = "files";
			ps.Merge(DestFilesPath);
		}
	}
	CATCHZOK
	return ok;
}

int PPWorkbookExporter::ExportPacket(const PPWorkbookPacket * pPack)
{
	int    ok = 1;
	SString temp_buf;
	WorkbookTbl::Rec inner_rec;
	Sdr_Workbook sdr_rec;
	THROW_INVARG(pPack);
	// @v10.7.9 @ctr MEMSZERO(sdr_rec);
	sdr_rec.ID = pPack->Rec.ID;
	STRNSCPY(sdr_rec.Name, pPack->Rec.Name);
	STRNSCPY(sdr_rec.Symb, pPack->Rec.Symb);
	sdr_rec.Type = pPack->Rec.Type;
	sdr_rec.Order = pPack->Rec.Rank;
	sdr_rec.Flags = pPack->Rec.Flags;
	if(sdr_rec.Type == PPWBTYP_KEYWORD) {
		sdr_rec.KeywordCount = pPack->Rec.KeywordCount;
		sdr_rec.KeywordDilute = pPack->Rec.KeywordDilute;
	}
	if(pPack->Rec.ParentID && WbObj.Fetch(pPack->Rec.ParentID, &inner_rec) > 0) {
		sdr_rec.ParentID = inner_rec.ID;
		STRNSCPY(sdr_rec.ParentName, inner_rec.Name);
		STRNSCPY(sdr_rec.ParentSymb, inner_rec.Symb);
	}
	if(pPack->Rec.LinkID && WbObj.Fetch(pPack->Rec.LinkID, &inner_rec) > 0) {
		sdr_rec.LinkID = inner_rec.ID;
		STRNSCPY(sdr_rec.LinkName, inner_rec.Name);
		STRNSCPY(sdr_rec.LinkSymb, inner_rec.Symb);
	}
	if(pPack->Rec.CssID && WbObj.Fetch(pPack->Rec.CssID, &inner_rec) > 0) {
		sdr_rec.CssID = inner_rec.ID;
		STRNSCPY(sdr_rec.CssName, inner_rec.Name);
		STRNSCPY(sdr_rec.CssSymb, inner_rec.Symb);
	}
	{
		const ObjTagItem * p_tag_keywords = pPack->TagL.GetItem(PPTAG_WORKBOOK_KEYWORDS);
		if(p_tag_keywords) {
			p_tag_keywords->GetStr(temp_buf);
			temp_buf.CopyTo(sdr_rec.KeywordTag, sizeof(sdr_rec.KeywordTag));
		}
	}
	{
		ObjLinkFiles _lf(PPOBJ_WORKBOOK);
		_lf.Load(pPack->Rec.ID, 0L);
		if(_lf.At(0, temp_buf.Z())) {
			if(DestFilesPath.NotEmpty() && !fileExists(DestFilesPath)) {
				THROW_SL(::createDir(DestFilesPath));
			}
			SString dest_filename;
			SPathStruc ps(temp_buf);
			MakeTempFileName(DestFilesPath, "wbf", ps.Ext, 0, dest_filename);
			THROW_SL(SCopyFile(temp_buf, dest_filename, 0, FILE_SHARE_READ, 0));
			dest_filename.CopyTo(sdr_rec.Content, sizeof(sdr_rec.Content));
		}
	}
	P_IEWorkbook->GetParamConst().InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr_rec);
	THROW(P_IEWorkbook->AppendRecord(&sdr_rec, sizeof(sdr_rec)));
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_UhttWorkbook
//
struct UhttWorkbookBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttWorkbookBlock() : TagPos(0), State(stFetch)
	{
	}
	void Clear()
	{
		Pack.destroy();
		TagPos = 0;
		State = stFetch;
	}
	PPObjWorkbook WbObj;
	PPObjTag TagObj;
	PPWorkbookPacket Pack;
	uint   TagPos;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttWorkbook)
{
	if(Valid) {
		Extra[0].Ptr = new UhttWorkbookBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData("iter@TagList", &I_TagList, sizeof(I_TagList));
	}
}

PPALDD_DESTRUCTOR(UhttWorkbook)
{
	Destroy();
	delete static_cast<UhttWorkbookBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttWorkbook::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttWorkbookBlock & r_blk = *static_cast<UhttWorkbookBlock *>(Extra[0].Ptr);
	r_blk.Clear();
	MEMSZERO(H);
	if(r_blk.WbObj.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
		SString temp_buf;
		H.ID        = r_blk.Pack.Rec.ID;
		H.ParentID  = r_blk.Pack.Rec.ParentID;
		H.CssID     = r_blk.Pack.Rec.CssID;
		H.LinkID    = r_blk.Pack.Rec.LinkID;
		H.Type      = r_blk.Pack.Rec.Type;
		H.Rank      = r_blk.Pack.Rec.Rank;
		H.Flags     = r_blk.Pack.Rec.Flags;
		H.KeywordCount  = r_blk.Pack.Rec.KeywordCount;
		H.KeywordDilute = r_blk.Pack.Rec.KeywordDilute;
		H.Dt = r_blk.Pack.Rec.Dt;
		H.Tm = r_blk.Pack.Rec.Tm;
		{
			LDATETIME mod_dtm = r_blk.WbObj.GetLastModifTime(rFilt.ID);
			H.ModifDt = mod_dtm.d;
			H.ModifTm = mod_dtm.t;
		}
		{
			LDATETIME mod_dtm = r_blk.WbObj.GetContentLastModifTime(r_blk.Pack.Rec.ID);
			H.ContentModifDt = mod_dtm.d;
			H.ContentModifTm = mod_dtm.t;
		}
		STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
		STRNSCPY(H.Symb, r_blk.Pack.Rec.Symb);
		STRNSCPY(H.Version, r_blk.Pack.Rec.Version);
		{
			r_blk.Pack.GetExtStrData(WBEXSTR_DESCRIPTION, temp_buf.Z());
			STRNSCPY(H.Descr, temp_buf);
		}
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttWorkbook::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttWorkbookBlock & r_blk = *static_cast<UhttWorkbookBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@TagList"))
		r_blk.TagPos = 0;
	return -1;
}

static void ShuffleWordList(SString & rText)
{
	StringSet ss(',', rText);
	if(ss.getCount() > 1) {
		StringSet new_ss;
		SString word_buf;
		LongArray pos_list;
		for(uint sp = 0; ss.get(&sp, word_buf);) {
			if(word_buf.NotEmptyS()) {
				uint new_pos = 0;
				new_ss.add(word_buf, &new_pos);
				pos_list.add(new_pos);
			}
		}
		if(pos_list.getCount() > 1) {
			pos_list.shuffle();
			ss.clear();
			for(uint i = 0; i < pos_list.getCount(); i++) {
				new_ss.get(pos_list.get(i), word_buf);
				ss.add(word_buf);
			}
			rText = ss.getBuf();
		}
	}
}

int PPALDD_UhttWorkbook::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttWorkbookBlock & r_blk = *static_cast<UhttWorkbookBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@TagList")) {
		if(r_blk.TagPos < r_blk.Pack.TagL.GetCount()) {
			MEMSZERO(I_TagList);
			const ObjTagItem * p_item = r_blk.Pack.TagL.GetItemByPos(r_blk.TagPos);
			I_TagList.TagTypeID = p_item->TagDataType;
			{
				PPObjectTag rec;
				if(r_blk.TagObj.Fetch(p_item->TagID, &rec) > 0)
					STRNSCPY(I_TagList.TagSymb, rec.Symb);
			}
			switch(p_item->TagDataType) {
				case OTTYP_STRING:
				case OTTYP_GUID:
					p_item->GetStr(temp_buf.Z());
					if(p_item->TagID == PPTAG_WORKBOOK_KWLOC) {
						if(temp_buf.HasChr('@')) {
							StringSet ss(',', temp_buf);
							SString f_name, f_body, new_tag_text;
							for(uint sp = 0; ss.get(&sp, f_name);) {
								if(f_name.Strip().C(0) == '@' && f_name.Len() > 1) {
									PPObjFormula f_obj;
									PPID   f_id = 0;
									f_name.ShiftLeft(1);
									if(f_obj.Get(f_id, f_name, f_body) > 0)
										new_tag_text.CatDivIfNotEmpty(',', 0).Cat(f_body);
									else
										new_tag_text.CatDivIfNotEmpty(',', 0).CatChar('@').Cat(f_name);
								}
								else
									new_tag_text.CatDivIfNotEmpty(',', 0).Cat(f_name);
							}
							temp_buf = new_tag_text;
						}
						ShuffleWordList(temp_buf);
					}
					if(p_item->TagID == PPTAG_WORKBOOK_KEYWORDS) {
						if(temp_buf.HasChr('#') || temp_buf.HasChr('$')) {
							StringSet ss(',', temp_buf);
							SString f_name, f_body, new_tag_text;
							for(uint sp = 0; ss.get(&sp, f_name);) {
								if(f_name.Strip().C(0) == '#' && f_name.Len() > 1) {
									PPID   kw_id = f_name.ShiftLeft().ToLong();
									WorkbookTbl::Rec kw_rec;
									if(r_blk.WbObj.Fetch(kw_id, &kw_rec) > 0)
										new_tag_text.CatDivIfNotEmpty(',', 0).Cat(kw_rec.Name);
								}
								else if(f_name.Strip().C(0) == '$' && f_name.Len() > 1) {
									PPID   kw_id = 0;
									WorkbookTbl::Rec kw_rec;
									if(r_blk.WbObj.SearchBySymb(f_name+1, &kw_id, &kw_rec) > 0)
										new_tag_text.CatDivIfNotEmpty(',', 0).Cat(kw_rec.Name);
								}
								else
									new_tag_text.CatDivIfNotEmpty(',', 0).Cat(f_name);
							}
							temp_buf = new_tag_text;
						}
						ShuffleWordList(temp_buf);
					}
					else if(p_item->TagID == PPTAG_WORKBOOK_KWSYN) {
						ShuffleWordList(temp_buf);
					}
					temp_buf.CopyTo(I_TagList.StrVal, sizeof(I_TagList.StrVal));
					break;
				case OTTYP_NUMBER: p_item->GetReal(&I_TagList.RealVal); break;
				case OTTYP_BOOL:
				case OTTYP_INT: p_item->GetInt(&I_TagList.IntVal); break;
				case OTTYP_DATE: p_item->GetDate(&I_TagList.DateVal); break;
			}
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.TagPos++;
	}
	return ok;
}

int PPALDD_UhttWorkbook::Set(long iterId, int commit)
{
	int    ok = 1;
	SString temp_buf;
	WorkbookTbl::Rec temp_rec;
	UhttWorkbookBlock & r_blk = *static_cast<UhttWorkbookBlock *>(Extra[0].Ptr);
	const PPID glob_acc_id = DS.GetConstTLA().GlobAccID;
	if(r_blk.State != UhttWorkbookBlock::stSet) {
		r_blk.Clear();
		r_blk.State = UhttWorkbookBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			r_blk.Pack.Rec.ID = H.ID;
			if(H.ParentID && r_blk.WbObj.Search(H.ParentID, &temp_rec) > 0)
				r_blk.Pack.Rec.ParentID = H.ParentID;
			if(H.CssID && r_blk.WbObj.Search(H.CssID, &temp_rec) > 0)
				r_blk.Pack.Rec.CssID = H.CssID;
			if(H.LinkID && r_blk.WbObj.Search(H.LinkID, &temp_rec) > 0)
				r_blk.Pack.Rec.LinkID = H.LinkID;
			r_blk.Pack.Rec.Type = H.Type;
			r_blk.Pack.Rec.Rank = H.Rank;
			r_blk.Pack.Rec.Flags = H.Flags;
			r_blk.Pack.Rec.KeywordCount = static_cast<int16>(H.KeywordCount);
			r_blk.Pack.Rec.KeywordDilute = static_cast<int16>(H.KeywordDilute);
			r_blk.Pack.Rec.Dt = H.Dt;
			r_blk.Pack.Rec.Tm = H.Tm;
			(temp_buf = H.Name).Strip().RevertSpecSymb(SFileFormat::Html);
			STRNSCPY(r_blk.Pack.Rec.Name, temp_buf);
			STRNSCPY(r_blk.Pack.Rec.Symb, H.Symb);
			STRNSCPY(r_blk.Pack.Rec.Version, H.Version);
			r_blk.Pack.PutExtStrData(WBEXSTR_DESCRIPTION, (temp_buf = H.Descr).Strip().RevertSpecSymb(SFileFormat::Html));
		}
		else if(iterId == GetIterID("iter@TagList")) {
			PPID   tag_id = 0;
			if(r_blk.TagObj.FetchBySymb(I_TagList.TagSymb, &tag_id) > 0) {
				r_blk.Pack.TagL.PutItemStr(tag_id, I_TagList.StrVal);
			}
		}
	}
	else {
		PPGlobalAccRights rights_blk(PPTAG_GUA_WBRIGHTS);
		PPID   id = r_blk.Pack.Rec.ID;
		if(id == 0) {
			PPID   ex_id = 0;
			if(r_blk.Pack.Rec.Symb[0] && r_blk.WbObj.SearchBySymb(r_blk.Pack.Rec.Symb, &ex_id, &temp_rec) > 0) {
				LDATETIME foreign_mod_dtm;
				foreign_mod_dtm.Set(H.ModifDt, H.ModifTm);
				LDATETIME mod_dtm = r_blk.WbObj.GetLastModifTime(ex_id);
				if(cmp(foreign_mod_dtm, mod_dtm) > 0) {
					PPWorkbookPacket ex_pack;
					THROW_PP_S(rights_blk.IsAllow(PPGlobalAccRights::fEdit), PPERR_NORIGHTS, DS.GetTLA().GlobAccName);
					THROW(r_blk.WbObj.GetPacket(temp_rec.ID, &ex_pack) > 0); // Ранее мы нашли запись - пактет должен быть получен
					id = temp_rec.ID;
					if(r_blk.Pack.Rec.ParentID)
						ex_pack.Rec.ParentID = r_blk.Pack.Rec.ParentID;
					ex_pack.Rec.Type = r_blk.Pack.Rec.Type;
					ex_pack.Rec.Rank = r_blk.Pack.Rec.Rank;
					ex_pack.Rec.Flags = r_blk.Pack.Rec.Flags;
					ex_pack.Rec.KeywordCount = r_blk.Pack.Rec.KeywordCount;
					ex_pack.Rec.KeywordDilute = r_blk.Pack.Rec.KeywordDilute;
					ex_pack.Rec.Dt = r_blk.Pack.Rec.Dt;
					ex_pack.Rec.Tm = r_blk.Pack.Rec.Tm;
					STRNSCPY(ex_pack.Rec.Name, r_blk.Pack.Rec.Name);
					STRNSCPY(ex_pack.Rec.Version, r_blk.Pack.Rec.Version);
					r_blk.Pack.GetExtStrData(WBEXSTR_DESCRIPTION, temp_buf);
					ex_pack.PutExtStrData(WBEXSTR_DESCRIPTION, temp_buf);
					ex_pack.TagL.Merge(r_blk.Pack.TagL, ObjTagList::mumAdd|ObjTagList::mumUpdate);
					THROW(r_blk.WbObj.PutPacket(&id, &ex_pack, 1));
					Extra[4].Ptr = reinterpret_cast<void *>(id);
				}
				else
					Extra[4].Ptr = reinterpret_cast<void *>(ex_id);
			}
			else {
				THROW_PP_S(rights_blk.IsAllow(PPGlobalAccRights::fCreate), PPERR_NORIGHTS, DS.GetTLA().GlobAccName);
				THROW(r_blk.WbObj.PutPacket(&id, &r_blk.Pack, 1));
				Extra[4].Ptr = reinterpret_cast<void *>(id);
			}
		}
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
