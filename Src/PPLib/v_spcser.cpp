// V_SPCSER.CPP
// Copyright (c) A.Starodub 2012, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

IMPLEMENT_PPFILT_FACTORY(SpecSeries); SpecSeriesFilt::SpecSeriesFilt() : PPBaseFilt(PPFILT_SPECSERIES, 0, 1)
{
	SetFlatChunk(offsetof(SpecSeriesFilt, ReserveStart),
		offsetof(SpecSeriesFilt, ReserveEnd) - offsetof(SpecSeriesFilt, ReserveStart));
	SetBranchSString(offsetof(SpecSeriesFilt, Serial));
	Init(1, 0);
}

SpecSeriesFilt & FASTCALL SpecSeriesFilt::operator = (const SpecSeriesFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewSpecSeries::PPViewSpecSeries() : PPView(0, &Filt, PPVIEW_SPECSERIES, 0, 0)
{
}

PPViewSpecSeries::~PPViewSpecSeries()
{
}

class SpecSerFiltDlg : public TDialog {
public:
	enum {
		ctlgroupGoods = 1
	};
	SpecSerFiltDlg() : TDialog(DLG_SPCSERFLT)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_SPCSERFLT_GGRP, CTLSEL_SPCSERFLT_GOODS));
		SetupCalPeriod(CTLCAL_SPCSERFLT_PERIOD, CTL_SPCSERFLT_PERIOD);
	}
	int setDTS(const SpecSeriesFilt *);
	int getDTS(SpecSeriesFilt *);
private:
	SpecSeriesFilt Data;
};

int SpecSerFiltDlg::setDTS(const SpecSeriesFilt * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	GoodsCtrlGroup::Rec ggrp_rec(Data.GoodsGrpID, Data.GoodsID);
	setGroupData(ctlgroupGoods, &ggrp_rec);
	AddClusterAssoc(CTL_SPCSERFLT_KIND, -1, SPCSERIK_SPOILAGE);
	AddClusterAssoc(CTL_SPCSERFLT_KIND, 0, SPCSERIK_SPOILAGE);
	SetClusterData(CTL_SPCSERFLT_KIND, Data.InfoKind);
	SetPeriodInput(this, CTL_SPCSERFLT_PERIOD, &Data.Period);
	return 1;
}

int SpecSerFiltDlg::getDTS(SpecSeriesFilt * pData)
{
	int    ok = 1;
	ushort sel_ctl = 0;
	GoodsCtrlGroup::Rec ggrp_rec;
	getGroupData(ctlgroupGoods, &ggrp_rec);
	Data.GoodsGrpID = ggrp_rec.GrpID;
	Data.GoodsID    = ggrp_rec.GoodsID;
	GetClusterData(CTL_SPCSERFLT_KIND, &Data.InfoKind);
	THROW(GetPeriodInput(this, sel_ctl = CTL_SPCSERFLT_PERIOD, &Data.Period));
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = (selectCtrl(sel_ctl), 0);
	ENDCATCH
	return ok;
}

/*virtual*/int PPViewSpecSeries::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODYERR(SpecSerFiltDlg, static_cast<SpecSeriesFilt *>(pBaseFilt));
}

/*virtual*/int PPViewSpecSeries::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW(Helper_InitBaseFilt(pFilt));
	CATCHZOK
	return ok;
}

int PPViewSpecSeries::InitIteration()
{
	int    ok = 1;
	SpecSeries2Tbl::Key1 k1, k_;
	DBQ * dbq = 0;
	THROW_MEM(P_IterQuery = new BExtQuery(&Tbl, 1));
	MEMSZERO(k1);
	P_IterQuery->selectAll();
	dbq = & (*dbq && daterange(Tbl.InfoDate, &Filt.Period));
	dbq = ppcheckfiltid(dbq, Tbl.GoodsID,    Filt.GoodsID);
	dbq = ppcheckfiltid(dbq, Tbl.InfoKind,   Filt.InfoKind);
	P_IterQuery->where(*dbq);
	k1.InfoKind = Filt.InfoKind;
	k_ = k1;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k1, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewSpecSeries::NextIteration(SpecSeriesViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery->nextIteration() > 0) {
		SpecSeriesViewItem item = *((SpecSeriesViewItem*)&Tbl.data);
		if(Tbl.Search(item.ID, &item) > 0) {
			ASSIGN_PTR(pItem, item);
			Counter.Increment();
			ok = 1;
		}
	}
	return ok;
}

static IMPL_DBE_PROC(dbqf_spcsn_textfld_iisi)
{
	char   result_buf[128];
	if(option == CALC_SIZE) {
		result->init(sizeof(result_buf));
	}
	else {
		SString temp_buf;
		PPID   goods_id = params[0].lval;
		PPID   manuf_id = params[1].lval;
		temp_buf = params[2].sptr;
		int   fld_id = params[3].lval;
		SpecSeries2Tbl::Rec ss_rec;
		// @v10.7.9 @ctr MEMSZERO(ss_rec);
		temp_buf.CopyTo(ss_rec.Tail, sizeof(ss_rec.Tail));
		temp_buf.Z();
		if(fld_id == SPCSNEXSTR_GOODSNAME && goods_id) {
			GetGoodsName(goods_id, temp_buf);
		}
		else if(fld_id == SPCSNEXSTR_MANUFNAME && manuf_id) {
			GetPersonName(manuf_id, temp_buf);
		}
		if(!temp_buf.NotEmptyS())
			SpecSeriesCore::GetExField(&ss_rec, fld_id, temp_buf);
		temp_buf.CopyTo(result_buf, sizeof(result_buf));
		result->init(result_buf);
	}
}

// static
int PPViewSpecSeries::DynFuncSpcSnTextFld = 0;

/*virtual*/DBQuery * PPViewSpecSeries::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncSpcSnTextFld, BTS_STRING, dbqf_spcsn_textfld_iisi, 4, BTS_INT, BTS_INT, BTS_STRING, BTS_INT);

	uint   brw_id = BROWSER_SPECSERIES;
	DBQ * dbq = 0;
	DBQuery * q = 0;
	SpecSeries2Tbl * t = 0;
	DBE    dbe_goods_name;
	DBE    dbe_manuf_name;
	THROW(CheckTblPtr(t = new SpecSeries2Tbl(Tbl.GetName())));
	{
		dbe_goods_name.init();
		dbe_goods_name.push(t->GoodsID);
		dbe_goods_name.push(t->ManufID);
		dbe_goods_name.push(t->Tail);

		DBConst dbc_long;
		dbc_long.init((long)SPCSNEXSTR_GOODSNAME);
		dbe_goods_name.push(dbc_long);
		dbe_goods_name.push(static_cast<DBFunc>(PPViewSpecSeries::DynFuncSpcSnTextFld));
	}
	{
		dbe_manuf_name.init();
		dbe_manuf_name.push(t->GoodsID);
		dbe_manuf_name.push(t->ManufID);
		dbe_manuf_name.push(t->Tail);

		DBConst dbc_long;
		dbc_long.init((long)SPCSNEXSTR_MANUFNAME);
		dbe_manuf_name.push(dbc_long);
		dbe_manuf_name.push(static_cast<DBFunc>(PPViewSpecSeries::DynFuncSpcSnTextFld));
	}
	dbq = & (*dbq && daterange(t->InfoDate, &Filt.Period));
	dbq = ppcheckfiltid(dbq, Tbl.GoodsID,  Filt.GoodsID);
	dbq = ppcheckfiltid(dbq, Tbl.InfoKind, Filt.InfoKind);
	q = &select(
		t->ID,        // #0
		t->GoodsID,   // #1
		t->InfoDate,  // #2
		t->Barcode,   // #3
		t->Serial,    // #4
		dbe_goods_name, // #5
		dbe_manuf_name, // #6
		0L).from(t, 0).where(*dbq);
	q->orderBy(t->InfoKind, t->InfoIdent, 0L);
	THROW(CheckQueryPtr(q));
	ASSIGN_PTR(pBrwId, brw_id);
	CATCH
		if(q)
			ZDELETE(q);
		else
			ZDELETE(t);
	ENDCATCH
	return q;
}

/*virtual*/void PPViewSpecSeries::PreprocessBrowser(PPViewBrowser * pBrw)
{
}

class SpecSerDlg : public TDialog {
	DECL_DIALOG_DATA(SpecSeries2Tbl::Rec);
public:
	enum {
		ctlgroupGoods = 1
	};
	SpecSerDlg() : TDialog(DLG_SPCSER)
	{
		SetupCalDate(CTLCAL_SPCSER_DT, CTL_SPCSER_DT);
		SetupCalDate(CTLCAL_SPCSER_ALLOWDT, CTL_SPCSER_ALLOWDT);
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_SPCSER_GGRP, CTLSEL_SPCSER_GOODS));
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			MEMSZERO(Data);
		GoodsCtrlGroup::Rec ggrp_rec(0, Data.GoodsID);
		SString temp_buf;
		setCtrlData(CTL_SPCSER_DT,       &Data.InfoDate);
		setCtrlData(CTL_SPCSER_INFOIDENT, Data.InfoIdent);
		setCtrlData(CTL_SPCSER_ALLOWDT,  &Data.AllowDate);
		setCtrlData(CTL_SPCSER_BARCODE,   Data.Barcode);
		setCtrlData(CTL_SPCSER_SERIAL,    Data.Serial);

		SpecSeriesCore::GetExField(&Data, SPCSNEXSTR_GOODSNAME, temp_buf);
		setCtrlString(CTL_SPCSER_GOODSNAME, temp_buf);
		SpecSeriesCore::GetExField(&Data, SPCSNEXSTR_MANUFNAME, temp_buf.Z());
		setCtrlString(CTL_SPCSER_MANUFNAME, temp_buf);
		SpecSeriesCore::GetExField(&Data, SPCSNEXSTR_DESCRIPTION, temp_buf.Z());
		setCtrlString(CTL_SPCSER_INFO, temp_buf);

		AddClusterAssoc(CTL_SPCSER_FLAGS, 0, SPCSELIF_FALSIFICATION);
		AddClusterAssoc(CTL_SPCSER_FLAGS, 1, SPCSELIF_ALLOW);
		SetClusterData(CTL_SPCSER_FLAGS, Data.Flags);
		setGroupData(ctlgroupGoods, &ggrp_rec);
		SetupPersonCombo(this, CTLSEL_SPCSER_MANUF, Data.ManufID, 0, PPPRK_MANUF, 0);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTL_SPCSER_DT, &Data.InfoDate);
		THROW_SL(checkdate(&Data.InfoDate));
		getCtrlData(CTL_SPCSER_INFOIDENT, Data.InfoIdent);
		getCtrlData(sel = CTL_SPCSER_ALLOWDT, &Data.AllowDate);
		THROW_SL(checkdate(Data.AllowDate, 1));
		getCtrlData(sel = CTL_SPCSER_BARCODE, Data.Barcode);
		getCtrlData(sel = CTL_SPCSER_SERIAL, Data.Serial);
		THROW_PP(sstrlen(Data.Serial) > 0, PPERR_SERIALNEEDED);
		getCtrlString(sel = CTL_SPCSER_GOODSNAME, temp_buf.Z());
		THROW_PP(temp_buf.NotEmptyS(), PPERR_GOODSNEEDED);
		SpecSeriesCore::SetExField(&Data, SPCSNEXSTR_GOODSNAME, temp_buf);
		getCtrlData(CTLSEL_SPCSER_GOODS, &Data.GoodsID);
		getCtrlString(sel = CTL_SPCSER_MANUFNAME, temp_buf.Z());
		THROW_PP(temp_buf.NotEmptyS(), PPERR_MANUFACTURERNEEDED);
		SpecSeriesCore::SetExField(&Data, SPCSNEXSTR_MANUFNAME, temp_buf);
		getCtrlString(CTL_SPCSER_INFO, temp_buf.Z());
		SpecSeriesCore::SetExField(&Data, SPCSNEXSTR_DESCRIPTION, temp_buf);
		getCtrlData(CTLSEL_SPCSER_MANUF, &Data.ManufID);
		GetClusterData(CTL_SPCSER_FLAGS, &Data.Flags);
		Data.InfoKind = SPCSERIK_SPOILAGE;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SPCSER_GOODS) || event.isCbSelected(CTLSEL_SPCSER_MANUF)) {
			SetupCtrls();
			clearEvent(event);
		}
	}
	void   SetupCtrls();
};

void SpecSerDlg::SetupCtrls()
{
	PPID   id = 0;
	SString name;
 	getCtrlData(CTLSEL_SPCSER_GOODS, &id);
	if(id) {
		GetObjectName(PPOBJ_GOODS, id, name);
		setCtrlString(CTL_SPCSER_GOODSNAME, name);
	}
	disableCtrl(CTL_SPCSER_GOODSNAME, (id != 0));

	getCtrlData(CTLSEL_SPCSER_MANUF, &(id = 0));
	if(id) {
		GetObjectName(PPOBJ_PERSON, id, name.Z());
		setCtrlString(CTL_SPCSER_MANUFNAME, name);
	}
	disableCtrl(CTL_SPCSER_MANUFNAME, (id != 0));
}

int PPViewSpecSeries::AddItem()
{
	int    ok = -1;
	PPID   id = 0;
	SpecSeries2Tbl::Rec rec;
	// @v10.7.9 @ctr MEMSZERO(rec);
	SpecSerDlg * p_dlg = new SpecSerDlg();
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&rec);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&rec)) {
			if(Tbl.Put(&id, &rec, 1) > 0)
				ok = valid_data = 1;
			else {
				id = 0;
				PPError();
			}
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int PPViewSpecSeries::EditItem(PPID id)
{
	int    ok = -1;
	SpecSeries2Tbl::Rec rec;
	SpecSerDlg * p_dlg = new SpecSerDlg();
	THROW(Tbl.Search(id, &rec) > 0);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&rec);
	for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		if(!p_dlg->getDTS(&rec))
			PPError();
		else if(Tbl.Put(&id, &rec, 1) > 0)
			ok = valid_data = 1;
		else
			PPError();
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int PPViewSpecSeries::DeleteItem(PPID id)
{
	return Tbl.Put(&id, 0, 1) ? 1 : PPErrorZ();
}

int PPViewSpecSeries::Import()
{
	return ImportSpecSeries();
}

/*virtual*/int PPViewSpecSeries::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem();
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(id)
					ok = EditItem(id);
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(id)
					ok = DeleteItem(id);
				break;
			case PPVCMD_IMPORT:
				ok = Import();
				break;
			case PPVCMD_IMPORTUHTT:
				ok = ImportUhtt();
				break;
			case PPVCMD_EXPORTUHTT:
				ok = ExportUhtt();
				break;
		}
	}
	return ok;
}

// @Muxa {
int PPViewSpecSeries::ImportUhtt()
{
	int    ok = 1;
	long   accepted_count = 0;
	uint   n_rec = 0;
	SString temp_buf, fmt_buf, msg_buf;
	PPUhttClient uc;
	PPLogger     logger;
	IterCounter  cntr;
	SpecSeries2Tbl::Rec    rec;
	UhttSpecSeriesPacket * p_pack;
	TSCollection <UhttSpecSeriesPacket> uhtt_series_list;
	DateRange  date_rng;
	date_rng.upp = 0;
	date_rng.low = 0;
	if(DateRangeDialog(0, 0, &date_rng) > 0) {
		PPWait(1);
		SString    period;
		{
			char   pb[64];
			periodfmt(&date_rng, pb);
			period.Cat(pb);
		}
		THROW(uc.Auth());
		THROW(uc.GetSpecSeriesByPeriod(period, uhtt_series_list));
		n_rec = uhtt_series_list.getCount();
		cntr.Init(n_rec);
		for(uint i = 0; i < n_rec; i++) {
			int   dup = 0;
			StrAssocArray list;
			THROW(p_pack = uhtt_series_list.at(i));
			THROW(Tbl.GetListBySerial(p_pack->InfoKind, p_pack->Serial, &list));
			for(uint j = 0; j < list.getCount(); j++) {
				if(Tbl.Search(list.Get(j).Id, &rec) > 0) {
					LDATE dt;
					strtodate(p_pack->InfoDate, DATF_YMD, &dt);
					if((p_pack->Serial == rec.Serial) && (p_pack->Barcode == rec.Barcode) &&
						(p_pack->InfoIdent == rec.InfoIdent) && (rec.InfoDate == dt)) {
							temp_buf.Z().Cat(p_pack->GoodsName).CatDiv(':', 1).Cat(p_pack->Serial).CatDiv(':', 1).Cat(p_pack->InfoDate);
							logger.LogString(PPTXT_IMPSPOIL_DUP, temp_buf);
							dup = 1;
							break;
					}
				}
			}
			if(!dup) {
				MEMSZERO(rec);
				rec.ID = 0;
				rec.GoodsID = p_pack->GoodsID;
				rec.ManufID = p_pack->ManufID;
				rec.ManufCountryID = p_pack->ManufCountryID;
				rec.LabID = p_pack->LabID;
				STRNSCPY(rec.Serial, p_pack->Serial);
				STRNSCPY(rec.Barcode, p_pack->Barcode);
				if(sstrlen(p_pack->GoodsName))
					SpecSeriesCore::SetExField(&rec, SPCSNEXSTR_GOODSNAME, p_pack->GoodsName);
				if(sstrlen(p_pack->ManufName))
					SpecSeriesCore::SetExField(&rec, SPCSNEXSTR_MANUFNAME, p_pack->ManufName);
				strtodate(p_pack->InfoDate, DATF_YMD, &rec.InfoDate);
				rec.InfoKind = p_pack->InfoKind;
				STRNSCPY(rec.InfoIdent, p_pack->InfoIdent);
				strtodate(p_pack->AllowDate, DATF_YMD, &rec.AllowDate);
				STRNSCPY(rec.AllowNumber, p_pack->AllowNumber);
				STRNSCPY(rec.LetterType, p_pack->LetterType);
				rec.Flags = p_pack->Flags;
				{
					PPID   id = 0;
					Tbl.Put(&id, &rec, 1);
					if(id > 0) {
						accepted_count++;
						logger.Log(PPFormatT(PPTXT_LOG_UHTT_SPECSERIESIMP, &msg_buf, (const char *)rec.Serial,
							temp_buf.Z().Cat(rec.InfoDate).cptr(), p_pack->GoodsName.cptr(), (const char *)rec.Barcode));
					}
					else {
						PPGetLastErrorMessage(0, temp_buf);
						logger.Log(PPFormatT(PPTXT_LOG_UHTT_SPECSERIESIMPFAULT, &msg_buf, (const char *)rec.Serial, temp_buf.cptr()));
					}
				}
			}
			PPWaitPercent(cntr.Increment());
		}
		PPWait(0);
		PPMessage(mfInfo|mfOK, PPINF_RCVCURRSCOUNT, temp_buf.Z().Cat(accepted_count));
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewSpecSeries::ExportUhtt()
{
	int    ok = -1;
	PPID   id = 0;
	SpecSeriesViewItem item;
	PPUhttClient uc;
	PPLogger logger;
	SString  temp_buf, msg_buf, fmt_buf;
	PPWait(1);
	THROW(uc.Auth());
	THROW(InitIteration());
	while(NextIteration(&item) > 0) {
		UhttSpecSeriesPacket pack;
		pack.ID = 0;
		pack.GoodsID = item.GoodsID;
		pack.ManufID = item.ManufID;
		pack.ManufCountryID = item.ManufCountryID;
		pack.LabID = item.LabID;
		pack.SetSerial(item.Serial);
		pack.Barcode = item.Barcode;
		SpecSeriesCore::GetExField(&item, SPCSNEXSTR_GOODSNAME, temp_buf.Z());
		pack.SetGoodsName(temp_buf);
		SpecSeriesCore::GetExField(&item, SPCSNEXSTR_MANUFNAME, temp_buf.Z());
		pack.SetManufName(temp_buf);
		{
			int d = 0, m = 0, y = 0;
			item.InfoDate.decode(&d, &m, &y);
			if(d && m && y)
				pack.InfoDate.Z().Cat(y).Cat("-").Cat(m).Cat("-").Cat(d);
		}
		pack.InfoKind = item.InfoKind;
		pack.SetInfoIdent(item.InfoIdent);
		{
			int d = 0, m = 0, y = 0;
			item.AllowDate.decode(&d, &m, &y);
			if(d && m && y)
				pack.AllowDate.Z().Cat(y).Cat("-").Cat(m).Cat("-").Cat(d);
		}
		pack.SetAllowNumber(item.AllowNumber);
		pack.SetLetterType(item.LetterType);
		pack.Flags = item.Flags;
		if(uc.CreateSpecSeries(&id, pack) > 0) {
			logger.Log(PPFormatT(PPTXT_LOG_UHTT_SPECSERIESEXP, &msg_buf, (const char *)item.Serial));
		}
		else {
			logger.Log(PPFormatT(PPTXT_LOG_UHTT_SPECSERIESEXPFAULT, &msg_buf, (const char *)item.Serial, uc.GetLastMessage().cptr()));
		}
		PPWaitPercent(GetCounter());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}
// } @Muxa
