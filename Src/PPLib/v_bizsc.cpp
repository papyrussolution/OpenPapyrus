// V_BIZSC.CPP
// Copyright (c) A.Starodub 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2025
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
BizScValByTemplViewItem::BizScValByTemplViewItem() : Id(0), P_Vals(0)
{
	Name[0] = 0;
}

BizScValByTemplViewItem::~BizScValByTemplViewItem()
{
	ZDELETE(P_Vals);
}

BizScValByTemplViewItem & FASTCALL BizScValByTemplViewItem::operator = (const BizScValByTemplViewItem & rSrc)
{
	size_t vals_len = sstrlen(rSrc.P_Vals);
	ZDELETE(P_Vals);
	Id = rSrc.Id;
	STRNSCPY(Name, rSrc.Name);
	if(vals_len) {
		P_Vals = new char[vals_len + 1];
		strnzcpy(P_Vals, rSrc.P_Vals, vals_len + 1);
	}
	return *this;
}
//
//
//
PPBizScTemplPacket::PPBizScTemplPacket()
{
	Init();
}

PPBizScTemplPacket::~PPBizScTemplPacket()
{
}

PPBizScTemplPacket & FASTCALL PPBizScTemplPacket::operator = (const PPBizScTemplPacket & rSrc)
{
	Init();
	Rec = rSrc.Rec;
	Cols.copy(rSrc.Cols);
	Rows.copy(rSrc.Rows);
	Cells.copy(rSrc.Cells);
	return *this;
}

void PPBizScTemplPacket::Init()
{
	Cols.freeAll();
	Rows.freeAll();
	Cells.freeAll();
	MEMSZERO(Rec);
}

int PPBizScTemplPacket::AddCol(uint * pPos, PPBizScTemplCol * pCol)
{
	int    ok = -1;
	if(pCol) {
		uint cols_count = Cols.getCount();
		PPID new_id = 0L;
		for(uint i = 0; i < cols_count; i++)
			new_id = MAX(new_id, Cols.at(i).Id);
		pCol->Id = new_id + 1L;
		Cols.insert(pCol);
		ASSIGN_PTR(pPos, Cols.getCount() - 1);
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::GetCol(PPID colId, uint * pPos, PPBizScTemplCol * pCol)
{
	int  ok = 1;
	uint pos = 0;
	if(colId && Cols.lsearch(&colId, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pCol, Cols.at(pos));
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::RemoveCol(uint pos)
{
	int    ok = -1;
	if(pos >= 0 && pos < Cols.getCount()) {
		PPID col_id = Cols.at(pos).Id;
		Cols.atFree(pos);
		for(long i = Cells.getCount() - 1; i >= 0; i--) {
			if(Cells.at(i).ColId == col_id)
				Cells.atFree(i);
		}
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::GetCellListInclEmpty(long colId, long rowId, TSVector <PPBizScTemplCell> * pCells)
{
	int    ok = -1;
	uint   count = Cols.getCount();
	TSVector <PPBizScTemplCol> cols;
	PPBizScTemplRow row;
	if(colId > 0) {
		PPBizScTemplCol col;
		count = 0;
		if(GetCol(colId, 0, &col) > 0) {
			cols.insert(&col);
			count = 1;
		}
	}
	else
		cols = Cols;
	MEMSZERO(row);
	THROW_INVARG(pCells);
	pCells->freeAll();
	if(GetRow(rowId, 0, &row) > 0) {
		for(uint c = 0; c < count; c++) {
			PPBizScTemplCell cell;
			TSVector <PPBizScTemplCell> cell_list;
			MEMSZERO(cell);
			if(GetCellList(cols.at(c).Id, rowId, &cell_list) > 0 && cell_list.getCount())
				cell = cell_list.at(0);
			else {
				cell.BizScId = row.BizScId;
				STRNSCPY(cell.Formula, row.Formula);
			}
			pCells->insert(&cell);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPBizScTemplPacket::GetCellList(PPID colId, PPID rowId, TSVector <PPBizScTemplCell> * pCells)
{
	int    ok = -1;
	const  uint   count = Cells.getCount();
	CALLPTRMEMB(pCells, freeAll());
	for(uint i = 0; i < count; i++) {
		PPBizScTemplCell & r_cell = Cells.at(i);
		if((!colId || r_cell.ColId == colId) && (!rowId || r_cell.RowId == rowId)) {
			CALLPTRMEMB(pCells, insert(&r_cell));
			ok = 1;
		}
	}
	return ok;
}

int PPBizScTemplPacket::GetRow(PPID rowId, uint * pPos, PPBizScTemplRow * pRow)
{
	int    ok = -1;
	uint   pos = 0;
	if(rowId && Rows.lsearch(&rowId, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pRow, Rows.at(pos));
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::AddRow(uint * pPos, PPBizScTemplRow * pRow)
{
	int    ok = -1;
	if(pRow) {
		const  uint rows_count = Rows.getCount();
		PPID   new_id = 0L;
		for(uint i = 0; i < rows_count; i++) {
			const long _iter_id = Rows.at(i).Id;
			SETMAX(new_id, _iter_id);
		}
		pRow->Id = new_id + 1L;
		Rows.insert(pRow);
		ASSIGN_PTR(pPos, Rows.getCount() - 1);
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::RemoveRow(PPID rowId)
{
	int  ok = 1;
	uint pos = 0;
	if(GetRow(rowId, &pos, 0) > 0) {
		const PPID row_id = Rows.at(pos).Id;
		Rows.atFree(pos);
		for(long i = Cells.getCount() - 1; i >= 0; i--) {
			if(Cells.at(i).RowId == row_id)
				Cells.atFree(i);
		}
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::AddCell(uint * pPos, PPBizScTemplCell * pCell)
{
	int    ok = -1;
	if(pCell) {
		const uint count = Cells.getCount();
		PPID new_id = 0L;
		for(uint i = 0; i < count; i++)
			new_id = MAX(new_id, Cells.at(i).Id);
		pCell->Id = new_id + 1L;
		Cells.insert(pCell);
		ASSIGN_PTR(pPos, Cells.getCount() - 1);
		ok = 1;
	}
	return ok;
}

int PPBizScTemplPacket::UpdateCell(uint * pPos, PPBizScTemplCell * pCell)
{
	int    ok = -1;
	uint pos = 0;
	if(pCell) {
		if(pCell->Id == 0)
			ok = AddCell(&pos, pCell);
		else {
			THROW(Cells.lsearch(&pCell->Id, &pos, CMPF_LONG));
			Cells.at(pos) = *pCell;

		}
		ok = 1;
	}
	ASSIGN_PTR(pPos, pos);
	CATCHZOK
	return ok;
}

int PPBizScTemplPacket::CalcValues(long colId, long rowId, BizScoreCore * pBizScTbl, RealArray & rValList)
{
	int    ok = -1;
	PPBizScTemplRow row;
	MEMSZERO(row);
	rValList.freeAll();
	if(pBizScTbl && GetRow(rowId, 0, &row) > 0) {
		TSVector <PPBizScTemplCell> cell_list;
		if(GetCellListInclEmpty(colId, row.Id, &cell_list) > 0 && cell_list.getCount()) {
			DL2_Resolver resolver;
			PPBizScTemplCell * p_cell = 0;
			SString str_val;
			SString formula;
			for(uint c = 0; cell_list.enumItems(&c, (void **)&p_cell) > 0;) {
				PPBizScTemplCol col;
				MEMSZERO(col);
				str_val.Z();
				const PPID score_id = p_cell->BizScId;
				formula = p_cell->Formula;
				GetCol(p_cell->ColId, 0, &col);
				if(score_id) {
					DateRange period;
					BizScoreTbl::Key0 k0;
					BExtQuery q(pBizScTbl, 0, 8);
					MEMSZERO(k0);
					period.Set(col.DtLow, col.DtUp);
					period.Actualize(ZERODATE);
					k0.ActualDate = period.upp;
					k0.ScoreID    = score_id;
					k0.ObjID      = MAXLONG;
					q.select(pBizScTbl->ScoreID, pBizScTbl->Val, pBizScTbl->Dt, pBizScTbl->Tm, pBizScTbl->ActualDate, pBizScTbl->Str, 0L).where(pBizScTbl->UserID == LConfig.UserID && pBizScTbl->ScoreID == score_id);
					for(q.initIteration(false, &k0, spLe); q.nextIteration() > 0;) {
						if(period.CheckDate(pBizScTbl->data.ActualDate)) {
							BizScoreTbl::Rec rec;
							pBizScTbl->copyBufTo(&rec);
							(str_val = rec.Str).Cat(rec.Val);
							break;
						}
					}
				}
				else if(formula.Len()) {
					double val = resolver.Resolve(formula);
					str_val.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
				if(!(col.Flags & PPBizScTemplCol::fInvisible)) {
					double val = str_val.ToReal();
					rValList.insert(&val);
				}
				ok = 1;
			}
		}
	}
	return ok;
}

PPObjBizScTempl::PPObjBizScTempl(void * extraPtr) : PPObjReference(PPOBJ_BIZSCTEMPL, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList);
}

PPObjBizScTempl::~PPObjBizScTempl()
{
}

int PPObjBizScTempl::GetPacket(PPID id, PPBizScTemplPacket * pPack)
{
	int    ok = -1;
	pPack->Init();
	if(id && Search(id, &pPack->Rec) > 0) {
		P_Ref->GetPropArray(Obj, pPack->Rec.ID, PPPRP_BIZSCTEMPLROWS, &pPack->Rows);
		P_Ref->GetPropArray(Obj, pPack->Rec.ID, PPPRP_BIZSCTEMPLCOLS, &pPack->Cols);
		ok = BIN(P_Ref->GetPropArray(Obj, pPack->Rec.ID, PPPRP_BIZSCTEMPLCELLS, &pPack->Cells));
	}
	return ok;
}

int PPObjBizScTempl::PutPacket(PPID * pID, PPBizScTemplPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID && pPack == 0) {
			THROW(P_Ref->PutPropArray(Obj, *pID, PPPRP_BIZSCTEMPLCOLS, 0, 0));
			THROW(P_Ref->PutPropArray(Obj, *pID, PPPRP_BIZSCTEMPLROWS, 0, 0));
			THROW(P_Ref->PutPropArray(Obj, *pID, PPPRP_BIZSCTEMPLCELLS, 0, 0));
			THROW(P_Ref->RemoveItem(Obj, *pID, 0));
		}
		else {
			pPack->Rec.Tag = PPOBJ_BIZSCTEMPL;
			THROW(StoreItem(Obj, *pID, &pPack->Rec, 0));
			THROW(P_Ref->PutPropArray(Obj, P_Ref->data.ObjID, PPPRP_BIZSCTEMPLCOLS,  &pPack->Cols,  0));
			THROW(P_Ref->PutPropArray(Obj, P_Ref->data.ObjID, PPPRP_BIZSCTEMPLROWS,  &pPack->Rows,  0));
			THROW(P_Ref->PutPropArray(Obj, P_Ref->data.ObjID, PPPRP_BIZSCTEMPLCELLS, &pPack->Cells, 0));
			pPack->Rec.ID = P_Ref->data.ObjID;
			ASSIGN_PTR(pID, pPack->Rec.ID);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class BizScTemplColDialog : public TDialog {
	DECL_DIALOG_DATA(PPBizScTemplCol);
public:
	BizScTemplColDialog() : TDialog(DLG_BIZSCTI)
	{
		SetupCalPeriod(CTLCAL_BIZSCTI_PERIOD, CTL_BIZSCTI_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		setCtrlData(CTL_BIZSCTI_NAME, Data.Name);
		AddClusterAssocDef(CTL_BIZSCTI_TYPE, 0, PPBizScTemplCol::tDate);
		AddClusterAssoc(CTL_BIZSCTI_TYPE, 1, PPBizScTemplCol::tPeriod);
		AddClusterAssoc(CTL_BIZSCTI_TYPE, 2, PPBizScTemplCol::tPeriodChange);
		SetClusterData(CTL_BIZSCTI_TYPE, static_cast<long>(Data.Type));
		AddClusterAssoc(CTL_BIZSCTI_FLAGS, 0, PPBizScTemplCol::fInvisible);
		SetClusterData(CTL_BIZSCTI_FLAGS, Data.Flags);
		{
			DateRange dtr;
			dtr.Set(Data.DtLow, Data.DtUp);
			SetPeriodInput(this, CTL_BIZSCTI_PERIOD, dtr);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		long   v = 0;
		DateRange dtr;
		getCtrlData(CTL_BIZSCTI_NAME, Data.Name);
		THROW(GetPeriodInput(this, CTL_BIZSCTI_PERIOD, &dtr));
		Data.DtLow = dtr.low;
		Data.DtUp  = dtr.upp;
		GetClusterData(CTL_BIZSCTI_TYPE, &v);
		Data.Type = (int16)v;
		GetClusterData(CTL_BIZSCTI_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmaMore)) {
			clearEvent(event);
		}
	}
};

static int TestFormula(PPID bizScId, const char * pFormula, PPObjBizScore * pObj, SString & rResult)
{
	int    ok = -1;
	rResult.Space() = 0;
	if(pObj) {
		if(bizScId) {
			PPBizScorePacket test_pack;
			if(pObj->GetPacket(bizScId, &test_pack) > 0)
				ok = pObj->TestPacket(&test_pack, rResult);
		}
		else if(pFormula && sstrlen(pFormula)) {
			DL2_Resolver resolver;
			double val = resolver.Resolve(pFormula);
			rResult.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			ok = 1;
		}
	}
	return ok;
}

class BizScTemplRowDialog : public TDialog {
	DECL_DIALOG_DATA(PPBizScTemplRow);
public:
	BizScTemplRowDialog() : TDialog(DLG_BIZSCTR)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		setCtrlData(CTL_BIZSCTR_ID, &Data.Id);
		disableCtrl(CTL_BIZSCTR_ID, true);
		setCtrlData(CTL_BIZSCTR_NAME,   Data.Name);
		setCtrlData(CTL_BIZSCTR_CODE,   Data.Symb);
		SetupPPObjCombo(this, CTLSEL_BIZSCTR_BIZSC, PPOBJ_BIZSCORE, Data.BizScId, OLW_CANINSERT|OLW_CANEDIT, 0);
		setCtrlData(CTL_BIZSCTR_FORMULA, Data.Formula);
		SetupCtrl(Data.BizScId);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_BIZSCTR_NAME,      Data.Name);
		getCtrlData(CTL_BIZSCTR_CODE,      Data.Symb);
		getCtrlData(CTLSEL_BIZSCTR_BIZSC, &Data.BizScId);
		getCtrlData(CTL_BIZSCTR_FORMULA,   Data.Formula);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_BIZSCTR_BIZSC)) {
			PPID   bizsc = getCtrlLong(CTLSEL_BIZSCTR_BIZSC);
			SetupCtrl(bizsc);
			clearEvent(event);
		}
		else if(event.isCmd(cmTest)) {
			SString result;
			PPBizScTemplRow templr_rec;
			if(getDTS(&templr_rec) > 0)
				TestFormula(templr_rec.BizScId, templr_rec.Formula, &BscObj, result);
			setStaticText(CTL_BIZSCTR_TESTLINE, result);
			clearEvent(event);
		}
	}
	void   SetupCtrl(PPID bizsc)
	{
		disableCtrl(CTL_BIZSCTR_FORMULA, LOGIC(bizsc));
	}
	PPObjBizScore   BscObj;
};

class BizScTemplCellDialog : public TDialog {
	DECL_DIALOG_DATA(PPBizScTemplCell);
public:
	BizScTemplCellDialog() : TDialog(DLG_BIZSCTC)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		setCtrlData(CTL_BIZSCTC_COLID, &Data.ColId);
		disableCtrl(CTL_BIZSCTC_COLID, true);
		setCtrlData(CTL_BIZSCTC_ROWID, &Data.RowId);
		disableCtrl(CTL_BIZSCTC_ROWID, true);
		SetupPPObjCombo(this, CTLSEL_BIZSCTC_BIZSC, PPOBJ_BIZSCORE, Data.BizScId, 0, 0);
		setCtrlData(CTL_BIZSCTC_FORMULA, Data.Formula);
		SetupCtrl(Data.BizScId);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_BIZSCTC_BIZSC, &Data.BizScId);
		getCtrlData(CTL_BIZSCTC_FORMULA,   Data.Formula);
		THROW_PP(Data.BizScId != 0 || sstrlen(Data.Formula) != 0, PPERR_INPUTBIZSCOREORFORMULA);
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_BIZSCTC_BIZSC)) {
			PPID   bizsc = getCtrlLong(CTLSEL_BIZSCTC_BIZSC);
			SetupCtrl(bizsc);
			clearEvent(event);
		}
		else if(event.isCmd(cmTest)) {
			SString result;
			PPBizScTemplCell templc_rec;
			if(getDTS(&templc_rec) > 0)
				TestFormula(templc_rec.BizScId, templc_rec.Formula, &BscObj, result);
			setStaticText(CTL_BIZSCTC_TESTLINE, result);
			clearEvent(event);
		}
	}
	void   SetupCtrl(PPID bizsc)
	{
		disableCtrl(CTL_BIZSCTC_FORMULA, LOGIC(bizsc));
	}
	PPObjBizScore    BscObj;
};

class BizScTemplDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPBizScTemplPacket);
public:
	BizScTemplDialog() : PPListDialog(DLG_BIZSCT, CTL_BIZSCT_LIST)
	{
		SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(CTL_BIZSCT_ROWS));
		if(!SetupStrListBox(p_box))
			PPError();
		setSmartListBoxOption(CTL_BIZSCT_ROWS, lbtSelNotify);
		setSmartListBoxOption(CTL_BIZSCT_LIST, lbtSelNotify);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init();
		disableCtrl(CTL_BIZSCT_ID, true);
		setCtrlData(CTL_BIZSCT_ID, &Data.Rec.ID);
		setCtrlString(CTL_BIZSCT_NAME, SString(Data.Rec.Name));
		setCtrlString(CTL_BIZSCT_SYMB, SString(Data.Rec.Symb));
		updateList(-1);
		UpdateList(CTL_BIZSCT_ROWS);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTL_BIZSCT_NAME, Data.Rec.Name);
		THROW_PP(sstrlen(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
		getCtrlData(CTL_BIZSCT_SYMB, Data.Rec.Symb);
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up);
	TDialog * GetItemDialog(long itemPos, int edit);
	int  GetItemDTS(TDialog * pDlg);
	int  SetItemDTS(TDialog * pDlg);
	int  UpdateItem(long * pPos, long * pID);
	int  GetCurItem(uint ctlList, long * pPos, long * pID);
	int  UpdateList(uint ctlList);
	int  EditCell();
	int  SetupCellInfo();
	int  GetSelList() {return P_Box ? P_Box->GetId() : 0;}

	union ListItem {
		PPBizScTemplCol  Col;
		PPBizScTemplRow  Row;
		PPBizScTemplCell Cell;
	} Item_;

	long SelColId;
	PPObjBizScore BizScObj;
};

int BizScTemplDialog::UpdateList(uint ctlList)
{
	uint sel_list = GetSelList();
	P_Box = static_cast<SmartListBox *>(getCtrlView(ctlList));
	updateList(-1);
	P_Box = static_cast<SmartListBox *>(getCtrlView(sel_list));
	return 1;
}

int BizScTemplDialog::moveItem(long pos, long id, int up)
{
	int    ok = -1;
	SVector * p_list = 0;
	if(GetSelList() == CTL_BIZSCT_LIST)
		p_list = &Data.Cols;
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		p_list = &Data.Rows;
	if(pos >= 0 && p_list->getCount()) {
		if(up && pos - 1 >= 0) {
			p_list->swap(pos, pos - 1);
			ok = 1;
		}
		else if(!up) {
			p_list->swap(pos, pos + 1);
			ok = 1;
		}
	}
	return ok;
}

int BizScTemplDialog::EditCell()
{
	int    ok = -1;
	long   col_id = 0L;
	long   row_id = 0L;
	PPBizScTemplCell cell;
	BizScTemplCellDialog * p_dlg = new BizScTemplCellDialog;
	THROW(CheckDialogPtr(&p_dlg));
	MEMSZERO(cell);
	GetCurItem(CTL_BIZSCT_LIST, 0, &col_id);
	GetCurItem(CTL_BIZSCT_ROWS, 0, &row_id);
	if(col_id) {
		TSVector <PPBizScTemplCell> cell_list;
		Data.GetCellListInclEmpty(col_id, row_id, &cell_list);
		if(cell_list.getCount()) {
			cell = cell_list.at(0);
			p_dlg->setDTS(&cell);
			for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
				if(p_dlg->getDTS(&cell) > 0) {
					cell.RowId = row_id;
					cell.ColId = col_id;
					THROW(Data.UpdateCell(0, &cell));
					ok = valid_data = 1;
				}
				else
					PPError();
			}
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int BizScTemplDialog::SetupCellInfo()
{
	long   col_id = 0L;
	long   row_id = 0L;
	long   col_pos = 0L;
	long   row_pos = 0L;
	SString text;
	SString empty_word;
	TSVector <PPBizScTemplCell> cell_list;
	GetCurItem(CTL_BIZSCT_LIST, &col_pos, &col_id);
	GetCurItem(CTL_BIZSCT_ROWS, &row_pos, &row_id);
	Data.GetCellListInclEmpty(col_id, row_id, &cell_list);
	if(cell_list.getCount()) {
		PPBizScTemplCell & r_cell = cell_list.at(0);
		PPGetWord(PPWORD_CELL, 0, text);
		text.Space().Cat("(r").Cat(row_pos + 1).Cat(",c").Cat(col_pos + 1).Cat("): ");
		if(r_cell.BizScId) {
			 PPBizScorePacket pack;
			 BizScObj.Fetch(r_cell.BizScId, &pack);
			text.Cat(pack.Rec.Name);
		}
		else if(sstrlen(r_cell.Formula))
			text.Cat(r_cell.Formula);
		else {
			PPGetWord(PPWORD_EMPTY, 0, empty_word);
			text.Cat(empty_word);
		}
	}
	setStaticText(CTL_BIZSCT_CELLTEXT, (const char *)text);
	return 1;
}

IMPL_HANDLE_EVENT(BizScTemplDialog)
{
	long   pos = 0;
	long   id = 0;
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmEditCell)) {
		EditCell();
		UpdateList(CTL_BIZSCT_LIST);
	}
	else if(event.isCmd(cmLBItemFocused)) {
		// ctlList = TVINFOVIEW->GetId();
		P_Box = static_cast<SmartListBox *>(getCtrlView(TVINFOVIEW->GetId()));
		if(GetSelList() == CTL_BIZSCT_ROWS)
			UpdateList(CTL_BIZSCT_LIST);
		SetupCellInfo();
	}
	else if(TVBROADCAST && TVCMD == cmReceivedFocus) {
		uint ctl = TVINFOVIEW->GetId();
		if(oneof2(ctl, CTL_BIZSCT_LIST, CTL_BIZSCT_ROWS)) {
			CtlList = ctl;
			P_Box = static_cast<SmartListBox *>(getCtrlView(CtlList));
		}
	}
	else
		return;
	clearEvent(event);
}

int BizScTemplDialog::GetCurItem(uint ctlList, long * pPos, long * pID)
{
	long   pos = -1;
	long   id = -1;
	SmartListBox * p_box = static_cast<SmartListBox *>(getCtrlView(ctlList));
	if(SmartListBox::IsValidS(p_box)) {
		p_box->getCurID(&id);
		pos = p_box->P_Def->_curItem();
	}
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pID, id);
	return 1;
}

int BizScTemplDialog::setupList()
{
	int    ok = 1;
	long   row_id = 0L;
	SVector * p_list = 0;
	TSVector <PPBizScTemplCell> cell_list;
	SString types;
	SString temp_buf;
	PPLoadText(PPTXT_BIZSCTI_TYPESTEXT, types);
	if(GetSelList() == CTL_BIZSCT_LIST) {
		p_list = &Data.Cols;
		GetCurItem(CTL_BIZSCT_ROWS, 0, &row_id);
		Data.GetCellListInclEmpty(0, row_id, &cell_list);
	}
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		p_list = &Data.Rows;
	for(uint i = 0; i < p_list->getCount(); i++) {
		long id = 0;
		StringSet ss(SLBColumnDelim);
		temp_buf.Z().Cat(i + 1);
		ss.add(temp_buf);
		if(GetSelList() == CTL_BIZSCT_LIST) {
			PPBizScTemplCol * p_item = static_cast<PPBizScTemplCol *>(p_list->at(i));
			id = p_item->Id;
			ss.add(temp_buf = p_item->Name);
			PPGetSubStr(types, p_item->Type, temp_buf.Z());
			ss.add(temp_buf);
			temp_buf.Z().Cat(p_item->DtLow).Dot().Dot().Cat(p_item->DtUp);
			ss.add(temp_buf);
			//
			// ���������� ���� � ������
			//
			temp_buf.Z();
			if(row_id > 0) {
				PPBizScTemplCell & r_cell = cell_list.at(i);
				if(r_cell.BizScId) {
					PPBizScorePacket pack;
					BizScObj.Fetch(r_cell.BizScId, &pack);
					temp_buf = pack.Rec.Name;
				}
				else
					temp_buf = r_cell.Formula;
			}
			ss.add(temp_buf);
		}
		else if(GetSelList() == CTL_BIZSCT_ROWS) {
			PPBizScTemplRow * p_item = static_cast<PPBizScTemplRow *>(p_list->at(i));
			id = p_item->Id;
			temp_buf = p_item->Name;
			ss.add(temp_buf);
			if(p_item->BizScId) {
				 PPBizScorePacket pack;
				 BizScObj.Fetch(p_item->BizScId, &pack);
				 temp_buf = pack.Rec.Name;
			}
			else
				temp_buf = p_item->Formula;
			ss.add(temp_buf);
		}
		if(!addStringToList(id, ss.getBuf()))
			ok = PPErrorZ();
	}
	return ok;
}

TDialog * BizScTemplDialog::GetItemDialog(long itemPos, int edit)
{
	TDialog * p_dlg = 0;
	SVector * p_list = 0;
	if(GetSelList() == CTL_BIZSCT_LIST)
		p_list = &Data.Cols;
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		p_list = &Data.Rows;
	if(!edit || edit && itemPos >= 0 && itemPos < p_list->getCountI()) {
		if(GetSelList() == CTL_BIZSCT_LIST) {
			p_dlg = new BizScTemplColDialog;
			MEMSZERO(Item_.Col);
			if(edit)
				Item_.Col = *static_cast<const PPBizScTemplCol *>(p_list->at(itemPos));
		}
		else if(GetSelList() == CTL_BIZSCT_ROWS) {
			p_dlg = new BizScTemplRowDialog;
			MEMSZERO(Item_.Row);
			if(edit)
				Item_.Row = *static_cast<const PPBizScTemplRow *>(p_list->at(itemPos));
		}
	}
	return p_dlg;
}

int BizScTemplDialog::SetItemDTS(TDialog * pDlg)
{
	int    ok = -1;
	if(GetSelList() == CTL_BIZSCT_LIST)
		ok = static_cast<BizScTemplColDialog *>(pDlg)->setDTS(&Item_.Col);
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		ok = static_cast<BizScTemplRowDialog *>(pDlg)->setDTS(&Item_.Row);
	return ok;
}

int BizScTemplDialog::GetItemDTS(TDialog * pDlg)
{
	int    ok = -1;
	if(GetSelList() == CTL_BIZSCT_LIST)
		ok = static_cast<BizScTemplColDialog *>(pDlg)->getDTS(&Item_.Col);
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		ok = static_cast<BizScTemplRowDialog *>(pDlg)->getDTS(&Item_.Row);
	return ok;
}

int BizScTemplDialog::UpdateItem(long * pPos, long * pID)
{
	int    ok = 1;
	long pos = DEREFPTROR(pPos, -1);
	long id = 0;
	uint p = (pos >= 0) ? pos : 0;
	if(GetSelList() == CTL_BIZSCT_LIST) {
		PPBizScTemplCol * p_col = &Item_.Col;
		if(pos >= 0)
			Data.Cols.at(pos) = *p_col;
		else
			ok = Data.AddCol(&p, p_col);
		id = p_col->Id;
	}
	else if(GetSelList() == CTL_BIZSCT_ROWS) {
		PPBizScTemplRow * p_row = &Item_.Row;
		if(pos >= 0)
			Data.Rows.at(pos) = *p_row;
		else
			ok = Data.AddRow(&p, p_row);
		id = p_row->Id;
	}
	ASSIGN_PTR(pPos, static_cast<long>(p));
	ASSIGN_PTR(pID, id);
	return ok;
}

int BizScTemplDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	long pos = -1, id = -1;
	TDialog * p_dlg = GetItemDialog(-1, 0);
	if(CheckDialogPtrErr(&p_dlg) > 0) {
		SetItemDTS(p_dlg);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(GetItemDTS(p_dlg) > 0) {
				UpdateItem(&pos, &id);
				ASSIGN_PTR(pPos, pos);
				ASSIGN_PTR(pID,  id);
				ok = valid_data = 1;
			}
			else
				PPError();
		}
	}
	delete p_dlg;
	return ok;
}

int BizScTemplDialog::editItem(long pos, long id)
{
	int    ok = -1;
	TDialog * p_dlg = 0;
	if((p_dlg = GetItemDialog(pos, 1))) {
		if(CheckDialogPtrErr(&p_dlg) > 0) {
			SetItemDTS(p_dlg);
			for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
				if(GetItemDTS(p_dlg) > 0) {
					UpdateItem(&pos, &id);
					ok = valid_data = 1;
				}
				else
					PPError();
			}
		}
	}
	delete p_dlg;
	return ok;
}

int BizScTemplDialog::delItem(long pos, long id)
{
	int    ok = -1;
	SVector * p_list = 0;
	if(GetSelList() == CTL_BIZSCT_LIST)
		p_list = &Data.Cols;
	else if(GetSelList() == CTL_BIZSCT_ROWS)
		p_list = &Data.Rows;
	if(p_list && pos >= 0 && pos < p_list->getCountI())
		if(CONFIRM(PPCFM_DELITEM)) {
			if(GetSelList() == CTL_BIZSCT_LIST)
				ok = Data.RemoveCol(pos);
			else if(GetSelList() == CTL_BIZSCT_ROWS)
				ok = Data.RemoveRow(pos);
			else
				ok = Data.Cells.atFree(pos);
		}
	return ok;
}

/*virtual*/int PPObjBizScTempl::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	int    tagtype = 0;
	ObjTagFilt ot_filt;
	PPBizScTemplPacket pack;
	BizScTemplDialog * dlg = 0;
	THROW(CheckRights(PPR_READ));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	if(r > 0) {
		THROW(CheckDialogPtr(&(dlg = new BizScTemplDialog())));
		dlg->setDTS(&pack);
		if(!CheckRights(PPR_MOD))
			dlg->enableCommand(cmOK, 0);
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			THROW(CheckRights(PPR_MOD));
			if(dlg->getDTS(&pack)) {
				valid_data = 1;
				if(*pID)
					*pID = pack.Rec.ID;
				THROW(PutPacket(pID, &pack, 1));
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}
//
// PPViewBizScTempl
//
IMPLEMENT_PPFILT_FACTORY(BizScTempl); BizScTemplFilt::BizScTemplFilt() : PPBaseFilt(PPFILT_BIZSCTEMPL, 0, 1)
{
	SetFlatChunk(offsetof(BizScTemplFilt, ReserveStart), offsetof(BizScTemplFilt, ReserveEnd)-offsetof(BizScTemplFilt, ReserveStart)+sizeof(ReserveStart));
	Init(1, 0);
}

BizScTemplFilt & BizScTemplFilt::operator = (const BizScTemplFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewBizScTempl::PPViewBizScTempl() : PPView(0, &Filt, PPVIEW_BIZSCTEMPL, 0, 0), P_TempTbl(0)
{
}

PPViewBizScTempl::~PPViewBizScTempl()
{
	delete P_TempTbl;
}

int PPViewBizScTempl::EditBaseFilt(PPBaseFilt * pFilt) { return 1; } // @stub

void PPViewBizScTempl::MakeTempRec(const PPBizScTempl * pInRec, TempBizScTemplTbl::Rec * pOutRec)
{
	if(pInRec && pOutRec) {
		pOutRec->ID = pInRec->ID;
		STRNSCPY(pOutRec->Name, pInRec->Name);
		STRNSCPY(pOutRec->Symb, pInRec->Symb);
		pOutRec->Flags = pInRec->Flags;
	}
}

int PPViewBizScTempl::CheckIDForFilt(PPID id, PPBizScTempl * pRec)
{
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempBizScTempl);

int PPViewBizScTempl::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	{
		THROW(P_TempTbl = CreateTempFile());
		{
			BExtInsert bei(P_TempTbl);
			PPBizScTempl templ_rec;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(PPID id = 0; Obj.EnumItems(&id, &templ_rec) > 0;) {
				TempBizScTemplTbl::Rec rec;
				MakeTempRec(&templ_rec, &rec);
				THROW_DB(bei.insert(&rec));
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewBizScTempl::InitIteration()
{
	int    ok = 0;
	DBQ  * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	P_IterQuery = new BExtQuery(P_TempTbl, 0);
	if(P_IterQuery) {
		P_IterQuery->selectAll().where(*dbq);
		P_IterQuery->initIteration(false, 0, 0);
		ok = 1;
	}
	return ok;
}

int FASTCALL PPViewBizScTempl::NextIteration(BizScTemplViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration()) {
		PPBizScTempl rec;
		if(Obj.Search(P_TempTbl->data.ID, &rec) > 0) {
			ASSIGN_PTR(pItem, rec);
			ok = 1;
		}
	}
	return ok;
}

int PPViewBizScTempl::UpdateTempTable(PPID id)
{
	if(id) {
		PPBizScTempl rec;
		TempBizScTemplTbl::Rec temp_rec;
		if(Obj.Search(id, &rec) > 0 && CheckIDForFilt(id, &rec)) {
			MakeTempRec(&rec, &temp_rec);
			if(SearchByID(P_TempTbl, 0, id, 0) > 0)
				UpdateByID(P_TempTbl, 0, id, &temp_rec, 0);
			else
				AddByID(P_TempTbl, &id, &temp_rec, 0);
		}
		else
			deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id);
	}
	return 1;
}

DBQuery * PPViewBizScTempl::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	PPID brw_id = 0;
	DBQuery * q = 0;
	DBQ * dbq = 0;
	TempBizScTemplTbl * tt  = 0;
	THROW(P_TempTbl);
	THROW(CheckTblPtr(tt = new TempBizScTemplTbl(P_TempTbl->GetName())));
	q = &select(tt->ID, tt->Name, tt->Symb, 0L).from(tt, 0L).where(*dbq);
	brw_id = BROWSER_BIZSCTEMPL;
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete tt;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewBizScTempl::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   id = (pHdr) ? *static_cast<const long *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				id = 0;
				ok = Obj.Edit(&id, 0);
				break;
			case PPVCMD_EDITITEM:
				if(id > 0)
					ok = Obj.Edit(&id, 0);
				break;
			case PPVCMD_DELETEITEM:
				ok = Obj.PutPacket(&id, 0, 1);
				break;
			case PPVCMD_CALCVALUES:
				if(id) {
					BizScValByTemplFilt filt;
					filt.TemplateID = id;
					ok = PPView::Execute(PPVIEW_BIZSCVALBYTEMPL, &filt, PPView::exefModeless, 0);
				}
				break;
		}
	}
	if(ok > 0 && oneof3(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM))
		UpdateTempTable(id);
	return ok;
}
//
// PPViewBizScValByTempl
//
IMPLEMENT_PPFILT_FACTORY(BizScValByTempl); BizScValByTemplFilt::BizScValByTemplFilt() : PPBaseFilt(PPFILT_BIZSCVALBYTEMPL, 0, 1)
{
	SetFlatChunk(offsetof(BizScValByTemplFilt, ReserveStart), offsetof(BizScValByTemplFilt, ReserveEnd)-offsetof(BizScValByTemplFilt, ReserveStart)+sizeof(ReserveStart));
	Init(1, 0);
}

BizScValByTemplFilt & FASTCALL BizScValByTemplFilt::operator = (const BizScValByTemplFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewBizScValByTempl::PPViewBizScValByTempl() : PPView(0, &Filt, PPVIEW_BIZSCVALBYTEMPL, implBrowseArray, 0)
{
}

PPViewBizScValByTempl::~PPViewBizScValByTempl()
{
}

class BizScValByTemplFiltDialog : public TDialog {
	DECL_DIALOG_DATA(BizScValByTemplFilt);
public:
	BizScValByTemplFiltDialog() : TDialog(DLG_FLTBIZSCVT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		SetupPPObjCombo(this, CTLSEL_FLTBIZSCVT_TEMPL, PPOBJ_BIZSCTEMPL, Data.TemplateID, 0, 0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTLSEL_FLTBIZSCVT_TEMPL, &Data.TemplateID);
		THROW_PP(Data.TemplateID != 0, PPERR_INVBIZSCTEMPL);
		ASSIGN_PTR(pData, Data);
		CATCH
			selectCtrl(sel);
			ok = 0;
		ENDCATCH
		return ok;
	}
};

int PPViewBizScValByTempl::EditBaseFilt(PPBaseFilt * pFilt) { DIALOG_PROC_BODYERR(BizScValByTemplFiltDialog, static_cast<BizScValByTemplFilt *>(pFilt)); }

int PPViewBizScValByTempl::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // ������� ���������� ������ �� ����� ������� � ��� members != 0 ����� ��� � pBlk != 0
	SString temp_buf;
	const BizScValByTemplViewItem * p_item = static_cast<const BizScValByTemplViewItem *>(pBlk->P_SrcData);
	switch(pBlk->ColumnN) {
		case 0:
			pBlk->Set(p_item->Id);
			break;
		case 1:
			pBlk->Set(temp_buf = p_item->Name);
			break;
		default:
			if((pBlk->ColumnN - 2) >= 0) {
				int    found = 0;
				StringSet ss(SLBColumnDelim);
				ss.setBuf(p_item->P_Vals, sstrlen(p_item->P_Vals) + 1);
				temp_buf.Z();
				for(uint i = 0, p = 0; ss.get(&p, temp_buf); i++) {
					if(i == pBlk->ColumnN - 2) {
						found = 1;
						break;
					}
				}
				if(found)
					pBlk->Set(temp_buf);
				else
					pBlk->SetZero();
			}
			break;
	}
	return ok;
}

int PPViewBizScValByTempl::FetchData()
{
	int    ok = 1;
	PPBizScTemplRow * p_row = 0;
	Data.freeAll();
	for(uint r = 0; Pack.Rows.enumItems(&r, (void **)&p_row) > 0;) {
		RealArray val_list;
		if(Pack.CalcValues(0, p_row->Id, &BizScTbl, val_list) > 0) {
			uint val_count = val_list.getCount();
			SString temp_buf, val_buf;
			BizScValByTemplViewItem * p_item = new BizScValByTemplViewItem;
			THROW_MEM(p_item);
			for(uint v = 0; v < val_count; v++) {
				val_buf.Cat(val_list.at(v));
				if(v + 1 < val_count)
					val_buf.Cat(SLBColumnDelim);
			}
			p_item->Id = r;
			temp_buf = p_row->Name;
			if(temp_buf.Len() == 0) {
				if(p_row->BizScId)
					GetObjectName(PPOBJ_BIZSCORE, p_row->BizScId, temp_buf);
			}
			if(temp_buf.Len() == 0)
				temp_buf = p_row->Formula;
			temp_buf.CopyTo(p_item->Name, sizeof(p_item->Name));
			p_item->P_Vals = new char[val_buf.Len() + 1];
			val_buf.CopyTo(p_item->P_Vals, val_buf.Len() + 1);
			THROW_SL(Data.insert(p_item));
		}
	}
	CATCHZOK
	return ok;
}

int PPViewBizScValByTempl::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW_PP(BizScTObj.GetPacket(Filt.TemplateID, &Pack) > 0, PPERR_INVBIZSCTEMPL);
	THROW(FetchData());
	CATCHZOK
	return ok;
}

int PPViewBizScValByTempl::InitIteration()
{
	Counter.Init(Data.getCount());
	return 1;
}

int FASTCALL PPViewBizScValByTempl::NextIteration(BizScValByTemplViewItem * pItem)
{
	int    ok = -1;
	if(pItem && Counter < Data.getCount()) {
		ASSIGN_PTR(pItem, *static_cast<const BizScValByTemplViewItem *>(Data.at(Counter)));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

void PPViewBizScValByTempl::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		SString name;
		PPBizScTemplCol * p_col = 0;
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewBizScValByTempl *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
		for(uint i = 0; Pack.Cols.enumItems(&i, (void **)&p_col) > 0;) {
			if(!(p_col->Flags & PPBizScTemplCol::fInvisible)) {
				if(sstrlen(p_col->Name))
					name = p_col->Name;
				else
					name.Z().Cat(i);
				pBrw->insertColumn(-1, name, i + 1, MKSTYPE(S_ZSTRING, 20), ALIGN_RIGHT, BCO_USERPROC);
			}
		}
	}
}

SArray * PPViewBizScValByTempl::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SString subtitle;
	TSCollection <BizScValByTemplViewItem> * p_array = new TSCollection <BizScValByTemplViewItem>;
	for(uint i = 0; i < Data.getCount(); i++) {
		BizScValByTemplViewItem * p_item = p_array->CreateNewItem();
		*p_item = *Data.at(i);
	}
	GetObjectName(PPOBJ_BIZSCTEMPL, Filt.TemplateID, subtitle);
	ASSIGN_PTR(pSubTitle, subtitle);
	ASSIGN_PTR(pBrwId, BROWSER_BIZSCVALBYTEMPL);
	return static_cast<SArray *>(p_array);
}

int PPViewBizScValByTempl::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
 	//PPID   id = (pHdr) ? *static_cast<const long *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_VIEWTEMPLATE:
				if(Filt.TemplateID) {
					if((ok = BizScTObj.Edit(&Filt.TemplateID, 0)) > 0)
						ok = ChangeFilt(1, pBrw);
				}
			default:
				break;
		}
	}
 	return ok;
}
