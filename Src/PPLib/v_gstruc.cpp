// V_GSTRUC.CPP
// Copyright (c) A.Starodub 2007, 2008, 2009, 2014, 2016, 2017
// @codepage UTF-8
// Таблица просмотра товарных структур
//
#include <pp.h>
#pragma hdrstop

SLAPI PPViewGoodsStruc::PPViewGoodsStruc() : PPView(0, &Filt)
{
	P_TempTbl = 0;
	DefReportId = REPORT_GOODSSTRUCLIST;
}

SLAPI PPViewGoodsStruc::~PPViewGoodsStruc()
{
	ZDELETE(P_TempTbl);
}

PPBaseFilt * SLAPI PPViewGoodsStruc::CreateFilt(void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_GOODS, &p_base_filt))
		((GoodsFilt *)p_base_filt)->Flags |= GoodsFilt::fNotUseViewOptions;
	return p_base_filt;
}

int SLAPI PPViewGoodsStruc::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return Filt.IsA(pBaseFilt) ? GoodsFilterDialog((GoodsFilt *)pBaseFilt) : 0;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsStruc);

int SLAPI PPViewGoodsStruc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PPWait(1);
	ZDELETE(P_TempTbl);
	THROW(P_TempTbl = CreateTempFile());
	{
		Goods2Tbl::Rec grec;
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		for(GoodsIterator gi(&Filt, 0); gi.Next(&grec) > 0;) {
			THROW(AddItem(grec.ID, grec.StrucID, &bei));
			PPWaitPercent(gi.GetIterCounter());
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsStruc::AddItem(PPID goodsID, PPID strucID, BExtInsert * pBei)
{
	int    ok = 1;
	PPGoodsStruc struc;
	if(GSObj.Get(strucID, &struc) > 0) {
		const  int  folder = BIN(struc.Rec.Flags & GSF_FOLDER);
		const  uint count  = folder ? struc.Childs.getCount() : struc.Items.getCount();
		TempGoodsStrucTbl::Rec temp_rec;
		for(uint i = 0; i < count; i++) {
			if(folder) {
				PPGoodsStruc * p_struc = struc.Childs.at(i);
				THROW(AddItem(goodsID, p_struc->Rec.ID, pBei)); // @recursion
			}
			else {
				SString gname, buf, word, struc_name;
				PPGoodsStrucItem & r_i = struc.Items.at(i);
				MEMSZERO(temp_rec);
				if(struc.Rec.ParentID) {
					PPGoodsStrucHeader hdr_rec;
					if(GSObj.Fetch(struc.Rec.ParentID, &hdr_rec) > 0)
						struc_name = hdr_rec.Name;
					else
						ideqvalstr(struc.Rec.ParentID, struc_name.Z());
				}
				else
					struc_name = struc.Rec.Name;
				if(struc_name.Len() == 0)
					ideqvalstr(struc.Rec.ID, struc_name).Quot('[', ']');
				temp_rec.GoodsID = goodsID;
				temp_rec.StrucID = struc.Rec.ID;
				temp_rec.ItemID  = r_i.GoodsID;
				// @v9.2.1 PPGetWord(PPWORD_STRUC, 0, word);
				PPLoadString("structure", word); // @v9.2.1
				GetGoodsName(goodsID, buf).Space().CatChar('(').Cat(word).CatChar(':').Cat(struc_name).CatChar(')');
				buf.CopyTo(temp_rec.GoodsName, sizeof(temp_rec.GoodsName));
				struc.GetTypeString(buf);
				buf.CopyTo(temp_rec.Type, sizeof(temp_rec.Type));
				r_i.GetEstimationString(buf.Z(), MKSFMTD(0, 3, ALIGN_RIGHT));
				buf.CopyTo(temp_rec.Qtty, sizeof(temp_rec.Qtty));
				STRNSCPY(temp_rec.ItemName, GetGoodsName(temp_rec.ItemID, buf));
				if(pBei) {
					THROW_DB(pBei->insert(&temp_rec));
				}
				else if(P_TempTbl)
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsStruc::UpdateTempTable(PPID goodsID, PPID parentStrucID, PPID strucID, int goodsIsItem, int use_ta)
{
	int    ok = 1;
	TempGoodsStrucTbl * t = 0;
	if(P_TempTbl && goodsID) {
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		if(goodsIsItem) {
			int    r = 1;
			Goods2Tbl::Rec grec;
			PPGoodsStruc gs;
			MEMSZERO(grec);
			THROW(CheckTblPtr(t = new TempGoodsStrucTbl(P_TempTbl->GetName())));
			{
				PPID   prev_goods = 0, prev_struc = 0;
				IterCounter c;
				TempGoodsStrucTbl::Key0 k, k_;
				BExtQuery q(t, 0);
				MEMSZERO(k);
				k.ItemID = goodsID;
				k_ = k;
				q.selectAll().where(t->ItemID == goodsID);
				c.Init(q.countIterations(0, &k_, spGe));
				for(q.initIteration(0, &k, spGe); r && q.nextIteration();) {
					if(prev_goods != t->data.GoodsID || prev_struc != t->data.StrucID) {
						THROW(UpdateTempTable(t->data.GoodsID, 0, t->data.StrucID, 0, 0)); // @recursion
						prev_goods = t->data.GoodsID;
						prev_struc = t->data.StrucID;
					}
					c.Increment();
					PPWaitPercent(c);
				}
			}
			if(r > 0 && GObj.Search(goodsID, &grec) > 0 && grec.StrucID) {
				GSObj.Get(grec.StrucID, &gs);
				THROW(UpdateTempTable(goodsID, gs.Rec.ParentID, gs.Rec.ID, 0, 0)); // @recursion
			}
		}
		else if(parentStrucID) {
			PPGoodsStruc gs;
			if(GSObj.Get(parentStrucID, &gs) > 0) {
				for(uint i = 0; i < gs.Childs.getCount(); i++) {
					const PPGoodsStruc * p_struc = (PPGoodsStruc*)gs.Childs.at(i);
					THROW(UpdateTempTable(goodsID, 0, p_struc->Rec.ID, 0, 0)); // @recursion
				}
			}
		}
		else if(strucID) {
			deleteFrom(P_TempTbl, 0, (P_TempTbl->GoodsID == goodsID && P_TempTbl->StrucID == strucID));
			THROW(AddItem(goodsID, strucID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ZDELETE(t);
	return ok;
}

int SLAPI PPViewGoodsStruc::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	TempGoodsStrucTbl::Key1 k, k_;
	BExtQuery::ZDelete(&P_IterQuery);
	MEMSZERO(k);
	P_IterQuery = new BExtQuery(P_TempTbl, 1);
	P_IterQuery->selectAll();
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewGoodsStruc::NextIteration(GoodsStrucViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		ASSIGN_PTR(pItem, P_TempTbl->data);
		return 1;
	}
	return -1;
}

DBQuery * SLAPI PPViewGoodsStruc::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	TempGoodsStrucTbl * t = 0;
	uint   brw_id = BROWSER_GOODSSTRUC;
	THROW(CheckTblPtr(t = new TempGoodsStrucTbl(P_TempTbl->GetName())));
	q = & select(
		t->GoodsID,   // #00
		t->StrucID,   // #01
		t->ItemID,    // #02
		t->GoodsName, // #03
		t->ItemName,  // #04
		t->Qtty,      // #05
		t->Type,      // #06
		0L).from(t, 0L).orderBy(t->GoodsName, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewGoodsStruc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   parent_struc_id = 0;
	BrwHdr brw_hdr;
	if(ok == -2) {
		if(pHdr)
			brw_hdr = *(BrwHdr*)pHdr;
		else
			MEMSZERO(brw_hdr);
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
			case PPVCMD_EDITGOODS:
			case PPVCMD_EDITITEMGOODS:
				if(brw_hdr.StrucID) {
					PPGoodsStruc gs;
					if(GSObj.Get(brw_hdr.StrucID, &gs) > 0) {
						int r = 0;
						if(gs.Rec.ParentID != 0) {
							parent_struc_id = gs.Rec.ParentID;
							r = GSObj.Get(parent_struc_id, &gs);
						}
						else
							r = 1;
						gs.GoodsID = brw_hdr.GoodsID; // @v6.9.2
						if(ppvCmd == PPVCMD_EDITGOODS || ppvCmd == PPVCMD_EDITITEMGOODS)
							ok = (GObj.Edit((ppvCmd == PPVCMD_EDITGOODS) ? &brw_hdr.GoodsID : &brw_hdr.ItemID, 0) == cmOK) ? 1 : -1;
						else if(r > 0 && GSObj.EditDialog(&gs) > 0) {
							int r = GSObj.Put((parent_struc_id ? &parent_struc_id : &brw_hdr.StrucID), &gs, 1);
							if(r > 0)
								ok = 1;
							else if(r == 0)
								ok = PPErrorZ();
						}
					}
				}
				break;
			case PPVCMD_TOTAL:
				ok = ViewTotal();
				break;
		}
	}
	if(ok > 0 && (ppvCmd == PPVCMD_EDITITEM || ppvCmd == PPVCMD_EDITGOODS)) {
		if(UpdateTempTable(brw_hdr.GoodsID, parent_struc_id, brw_hdr.StrucID, 0, 1) == 0)
			PPError();
		CALLPTRMEMB(pBrw, Update());
	}
	if(ok > 0 && ppvCmd == PPVCMD_EDITITEMGOODS) {
		if(UpdateTempTable(brw_hdr.ItemID, 0, 0, 1, 1) == 0)
			PPError();
		CALLPTRMEMB(pBrw, Update());
	}
	return ok;
}

int SLAPI PPViewGoodsStruc::ViewTotal()
{
	int    ok = 1;
	long   goods_count = 0, lines_count = 0;
	PPID   prev_goods = 0;
	GoodsStrucViewItem item;
	TDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_TGSTRUC))));
	for(InitIteration(); NextIteration(&item) > 0;) {
		if(item.GoodsID != prev_goods) {
			goods_count++;
			prev_goods = item.GoodsID;
		}
		lines_count++;
		PPWaitPercent(Counter);
	}
	p_dlg->setCtrlLong(CTL_TGSTRUC_LINES, lines_count);
	p_dlg->setCtrlLong(CTL_TGSTRUC_GOODS, goods_count);
	ExecViewAndDestroy(p_dlg);
	CATCHZOKPPERR
	return ok;
}
//
// Implementation of PPALDD_GoodsStrucList
//
PPALDD_CONSTRUCTOR(GoodsStrucList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsStrucList)
{
	Destroy();
}

typedef GoodsFilt GoodsStrucFilt;

int PPALDD_GoodsStrucList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsStruc, rsrv);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsStrucList::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(GoodsStruc);
}

int PPALDD_GoodsStrucList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsStruc); // PPViewGoodsStruc
	I.GsID = item.StrucID; // @v9.5.0
	I.GoodsID = item.GoodsID;
	I.ItemID  = item.ItemID;
	STRNSCPY(I.GoodsName, item.GoodsName);
	STRNSCPY(I.SQtty,     item.Qtty);
	STRNSCPY(I.Type,      item.Type);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsStrucList::Destroy()
{
	DESTROY_PPVIEW_ALDD(GoodsStruc);
}
