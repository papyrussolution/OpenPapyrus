// V_PLIST.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage windows-1251
//
// @todo Убрать вкладку "Дополнительно" из фильтра (она полностью дублирует опции товарного фильтра)
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
//
//
PriceLineIdent::PriceLineIdent()
{
	THISZERO();
}
//
// PriceListCore
//
PriceListCore::PriceListCore() : PriceListTbl()
{
}

int PriceListCore::Search(PPID id, void * b) { return SearchByID(this, PPOBJ_PRICELIST, id, b); }
int PriceListCore::Add(PPID * pID, void * b, int use_ta) { return AddByID(this, pID, b, use_ta); }
int PriceListCore::Update(PPID id, void * b, int use_ta) { return UpdateByID(this, 0, id, b, use_ta); }

int PriceListCore::Remove(PPID id, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(deleteFrom(&Lines, 0, Lines.ListID == id));
		THROW(deleteFrom(this, 0, this->ID == id));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PriceLineTbl::Key0 * PriceListCore::IdentToKey0(const PriceLineIdent * pIdent, PriceLineTbl::Key0 * pKey)
{
	pKey->ListID  = pIdent->PListID;
	pKey->GoodsID = pIdent->GoodsID;
	pKey->QuotKindID = pIdent->QuotKindID;
	pKey->LineNo = pIdent->LineNo;
	return pKey;
}

int PriceListCore::SearchLine(const PriceLineIdent * pIdent, void * b)
{
	PriceLineTbl::Key0 k;
	return SearchByKey(&Lines, 0, IdentToKey0(pIdent, &k), b);
}

int PriceListCore::SearchGoodsLine(PriceLineIdent * pIdent, void * b)
{
	PriceLineTbl::Key0 k;
	IdentToKey0(pIdent, &k);
	if(Lines.search(0, &k, spGe)) {
		if(k.ListID == pIdent->PListID && k.GoodsID == pIdent->GoodsID && k.QuotKindID == pIdent->QuotKindID) {
			pIdent->PListID = k.ListID;
			pIdent->GoodsID = k.GoodsID;
			pIdent->QuotKindID = k.QuotKindID;
			pIdent->LineNo = k.LineNo;
			Lines.copyBufTo(b);
			return 1;
		}
	}
	return PPDbSearchError();
}

int PriceListCore::EnumLines(const PriceLineIdent * pIdent, PriceLineIdent * pIter, void * b)
{
	PriceLineTbl::Key0 k;
	IdentToKey0(pIter, &k);
	if(Lines.search(0, &k, spGt)) {
		if((!pIdent->PListID || pIdent->PListID == k.ListID) &&
			(!pIdent->GoodsID || pIdent->GoodsID == k.GoodsID) &&
			(!pIdent->QuotKindID || pIdent->QuotKindID == k.QuotKindID)) {
			pIter->PListID = k.ListID;
			pIter->GoodsID = k.GoodsID;
			pIter->QuotKindID = k.QuotKindID;
			pIter->LineNo = k.LineNo;
			Lines.copyBufTo(b);
			return 1;
		}
	}
	return PPDbSearchError();
}

int PriceListCore::GetLineNo(const PriceLineIdent * pIdent, short * pLineNo)
{
	int    ok = 1;
	PriceLineTbl::Key0 k;
	IdentToKey0(pIdent, &k);
	if(Lines.search(0, &k, spLt) && k.ListID == pIdent->PListID &&
		k.GoodsID == pIdent->GoodsID && k.QuotKindID == pIdent->QuotKindID) {
		ASSIGN_PTR(pLineNo, (k.LineNo + 1));
	}
	else if(BTROKORNFOUND) {
		ASSIGN_PTR(pLineNo, 1);
	}
	else
		ok = PPSetErrorDB();
	return ok;
}

int PriceListCore::AddLine(PPID id, void * b, PriceLineIdent * pIdent, int useSubst, int use_ta)
{
	int    ok = 1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	PriceLineIdent ident;
	PriceLineTbl::Rec * p_rec = static_cast<PriceLineTbl::Rec *>(b);
	if(!useSubst)
		THROW(goods_obj.Fetch(p_rec->GoodsID, &goods_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		ident.PListID = id;
		ident.GoodsID = p_rec->GoodsID;
		ident.QuotKindID = p_rec->QuotKindID;
		THROW(GetLineNo(&ident, &p_rec->LineNo));
		ident.LineNo = p_rec->LineNo;
		p_rec->ListID = id;
		if(!useSubst) {
			STRNSCPY(p_rec->Name, goods_rec.Name);
			p_rec->GoodsGrpID = goods_rec.ParentID;
			p_rec->ManufID    = goods_rec.ManufID;
			p_rec->UnitID     = goods_rec.UnitID;
		}
		THROW_DB(Lines.insertRecBuf(p_rec));
		THROW(tra.Commit());
		ASSIGN_PTR(pIdent, ident);
	}
	CATCHZOK
	return ok;
}

int PriceListCore::UpdateLine(const PriceLineIdent * pIdent, PriceLineTbl::Rec * pRec, int useSubst, int use_ta)
{
	int    ok = 1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(!useSubst)
		THROW(goods_obj.Fetch(pRec->GoodsID, &goods_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchLine(pIdent) > 0);
		if(!useSubst) {
			pRec->GoodsGrpID = goods_rec.ParentID;
			pRec->ManufID    = goods_rec.ManufID;
			pRec->UnitID     = goods_rec.UnitID;
		}
		THROW_DB(Lines.updateRecBuf(pRec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PriceListCore::RemoveLine(const PriceLineIdent * pIdent, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchLine(pIdent) > 0);
		THROW_DB(Lines.deleteRec());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
PriceListViewItem::PriceListViewItem()
{
	Clear();
}

void PriceListViewItem::Clear()
{
	memzero(this, offsetof(PriceListViewItem, GoodsName_));
	GoodsName_.Z();
	GoodsGrpName_.Z();
	Memo_.Z();
}
//
// @ModuleDecl(PPViewPriceList)
//
IMPLEMENT_PPFILT_FACTORY(PriceList); PriceListFilt::PriceListFilt() : PPBaseFilt(PPFILT_PRICELIST, 0, 2)
{
	SetFlatChunk(offsetof(PriceListFilt, ReserveStart),
		offsetof(PriceListFilt, Memo)-offsetof(PriceListFilt, ReserveStart));
	SetBranchSString(offsetof(PriceListFilt, Memo));
	SetBranchObjIdListFilt(offsetof(PriceListFilt, GrpIDList));
	SetBranchObjIdListFilt(offsetof(PriceListFilt, GoodsIDList));
	Init(1, 0);
}

PriceListFilt & FASTCALL PriceListFilt::operator = (const PriceListFilt & s)
{
	Copy(&s, 1);
	return *this;
}

int PriceListFilt::Setup()
{
	if(GrpIDList.IsExists()) {
		GoodsGrpID = GrpIDList.GetSingle();
		if(GoodsGrpID)
			GrpIDList.Set(0);
	}
	return 1;
}
//
// Defaults:
//   LocID = LConfig.Location
//   UserID = -1;
//
PPViewPriceList::PPViewPriceList() : PPView(0, &Filt, PPVIEW_PRICELIST, 0, 0), P_BObj(BillObj), P_GGIter(0), P_TempTbl(0), State(0), NewGoodsGrpID(0)
{
	MEMSZERO(Cfg);
}

PPViewPriceList::~PPViewPriceList()
{
	delete P_GGIter;
	delete P_TempTbl;
}

PPBaseFilt * PPViewPriceList::CreateFilt(void * extraPtr) const
{
	PriceListFilt * p_filt = new PriceListFilt;
	if(p_filt) {
		p_filt->LocID = LConfig.Location;
		p_filt->UserID = -1;
	}
	else
		PPSetErrorNoMem();
	return p_filt;
}

int PPViewPriceList::GetLastQCertID(PPID goodsID, LDATE dt, PPID * pQCertID)
{
	int    ok = 1;
	PPID   qcert_id = 0;
	double price = 0.0;
	ReceiptTbl::Rec lot_rec;
	if(::GetCurGoodsPrice(goodsID, Filt.LocID, GPRET_MOSTRECENT, &price, &lot_rec) > 0)
		qcert_id = lot_rec.QCertID;
	ok = qcert_id ? 1 : -1;
	ASSIGN_PTR(pQCertID, qcert_id);
	return ok;
}

int PPViewPriceList::SearchLine(PriceLineIdent * pIdent, PriceLineTbl::Rec * pRec)
{
	PriceLineTbl::Key0 k;
	k.ListID     = pIdent->PListID;
	k.GoodsID    = pIdent->GoodsID;
	k.QuotKindID = pIdent->QuotKindID;
	k.LineNo     = pIdent->LineNo;
	return SearchByKey(&Tbl.Lines, 0, &k, pRec);
}

int PPViewPriceList::SetGoodsPrice(const RecalcParamBlock * pRPB, PPID quotKindID, double unitsPerPack, double price, int isPresent, int use_ta)
{
	int    ok = 1;
	char   name[128];
	long   gds_code = 0;
    PPID   subst_id = pRPB->GoodsID;
	SString memo;
	PriceLineTbl::Rec pline_rec;
	PriceLineIdent ident;
	memzero(name, sizeof(name));
	THROW(SubstGoods(pRPB->GoodsID, &subst_id, name, sizeof(name)));
	ident.PListID = Filt.PListID;
	ident.GoodsID = subst_id;
	ident.QuotKindID = quotKindID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Cfg.ExtFldMemoSubst) {
			SString line_buf;
			THROW(PPRef->GetPropVlrString(PPOBJ_GOODS, ident.GoodsID, GDSPRP_EXTSTRDATA, line_buf));
			PPGetExtStrData((int)Cfg.ExtFldMemoSubst, line_buf, memo);
		}
		if(!Filt.Sgg /* @v9.2.2 (GetGoodsCodeInAltGrp все проверит) && Filt.GoodsGrpID && GObj.IsAltGroup(Filt.GoodsGrpID) > 0*/) {
			/* @v9.2.2 if(PPRef->Assc.Search(PPASS_ALTGOODSGRP, Filt.GoodsGrpID, pRPB->GoodsID) > 0)
				gds_code = PPRef->Assc.data.InnerNum; */
			GObj.P_Tbl->GetGoodsCodeInAltGrp(pRPB->GoodsID, Filt.GoodsGrpID, &gds_code); // @v9.2.2
		}
		if(Tbl.SearchGoodsLine(&ident, &pline_rec) > 0) {
			pline_rec.UnitPerPack = unitsPerPack;
			pline_rec.IsPresent   = isPresent;
			pline_rec.Price     = price;
			pline_rec.AddPrice1 = pRPB->AddPrices[0];
			pline_rec.AddPrice2 = pRPB->AddPrices[1];
			pline_rec.AddPrice3 = pRPB->AddPrices[2];
			pline_rec.Expiry    = pRPB->Expiry;
			pline_rec.GoodsCode = gds_code;
			pline_rec.Rest      = pRPB->Rest;
			STRNSCPY(pline_rec.Name, name);
			memo.CopyTo(pline_rec.Memo, sizeof(pline_rec.Memo));
			THROW(Tbl.UpdateLine(&ident, &pline_rec, BIN(Filt.Sgg), 0)); // @vrf
		}
		else {
			MEMSZERO(pline_rec);
			pline_rec.ListID  = Filt.PListID;
			pline_rec.GoodsID = subst_id;
			pline_rec.QuotKindID = quotKindID;
			pline_rec.UnitPerPack = unitsPerPack;
			pline_rec.Price     = price;
			pline_rec.AddPrice1 = pRPB->AddPrices[0];
			pline_rec.AddPrice2 = pRPB->AddPrices[1];
			pline_rec.AddPrice3 = pRPB->AddPrices[2];
			pline_rec.IsPresent = isPresent;
			pline_rec.Expiry  = pRPB->Expiry;
			pline_rec.GoodsCode = gds_code;
			pline_rec.Rest      = pRPB->Rest;
			STRNSCPY(pline_rec.Name, name);
			memo.CopyTo(pline_rec.Memo, sizeof(pline_rec.Memo));
			THROW(Tbl.AddLine(Filt.PListID, &pline_rec, &ident, BIN(Filt.Sgg), 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewPriceList::GetPriceByQuot(RecalcParamBlock * pRPB, PPID quotKindID, double * pPrice)
{
	int    done  = 0;
	double price = *pPrice;
	const  QuotIdent qi(pRPB->Dt, Filt.LocID, quotKindID, 0, Filt.ArticleID);
	if(GObj.GetQuotExt(pRPB->GoodsID, qi, pRPB->Cost, pRPB->BasePrice, &price, 1) > 0) {
		if(pRPB->GoodsPriceWoTaxes) {
			PPQuotKind qk_rec;
			PPGoodsTaxEntry gtx;
			const PPID op_id = (pRPB->QkObj.Fetch(quotKindID, &qk_rec) > 0) ? qk_rec.OpID : 0;
			if(GObj.FetchTax(pRPB->GoodsID, LConfig.OperDate, op_id, &gtx) > 0)
				GObj.AdjPriceToTaxes(gtx.TaxGrpID, pRPB->TaxFactor, &price, 1);
		}
		done = 1;
	}
	else
		price = 0.0;
	ASSIGN_PTR(pPrice, price);
	return done ? 1 : -1;
}

int PPViewPriceList::SetByQuot(RecalcParamBlock * pRPB, double unitsPerPack, int isPresent, int use_ta)
{
	int    ok = 1, r;
	PPQuotKind qk_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Filt.QuotKindID > 0) {
			double price = pRPB->BasePrice;
			THROW(r = GetPriceByQuot(pRPB, Filt.QuotKindID, &price));
			if(r > 0)
				THROW(SetGoodsPrice(pRPB, Filt.QuotKindID, unitsPerPack, price, isPresent, 0));
		}
		else {
			for(PPID quot_kind_id = 0; pRPB->QkObj.EnumItems(&quot_kind_id, &qk_rec) > 0;) {
				double price = pRPB->BasePrice;
				THROW(r = GetPriceByQuot(pRPB, quot_kind_id, &price));
				if(r > 0)
					THROW(SetGoodsPrice(pRPB, quot_kind_id, unitsPerPack, price, isPresent, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(PRICELIST, PPPriceListImpExpParam);

PPPriceListImpExpParam::PPPriceListImpExpParam(uint recId, long flags) : PPImpExpParam(0, 0)
{
}

int EditPriceListImpExpParams()
{
	int    ok = -1;
	PPPriceListImpExpParam param;
	ImpExpParamDialog * p_dlg = new ImpExpParamDialog(DLG_IMPEXP, 0);
	THROW(CheckDialogPtr(&p_dlg));
	THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_PRICELIST, &param, p_dlg));
	CATCHZOK
	delete p_dlg;
	return ok;
}

int SelectPriceListImportCfg(PPPriceListImpExpParam * pParam, int forExport)
{
	int    ok = -1;
	uint   p = 0;
	PPID   id = 0;
	SString ini_file_name;
	PPPriceListImpExpParam param;
	StrAssocArray list;
	THROW_INVARG(pParam);
	param = *pParam;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_PRICELIST, &param, &list, forExport ? 1 : 2));
	if(list.SearchByText(param.Name, 1, &p) > 0)
		id = (uint)list.Get(p).Id;
	if(ListBoxSelDialog(&list, forExport ? PPTXT_PLISTEXPORTCFG : PPTXT_PLISTIMPORTCFG, &id, 0) > 0 && id > 0) {
		SString sect;
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		list.GetText(id, sect);
		param.ProcessName(1, sect);
		param.ReadIni(&ini_file, sect, 0);
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pParam, param);
	return ok;
}

// @v9.8.4 typedef TSVector <Sdr_PriceList> Sdr_PriceListArray; // @v9.8.4 TSArray-->TSVector

class PPPriceListImporter {
public:
	PPPriceListImporter() : P_View(0)
	{
	}
	int    Init(PPViewPriceList * pView);
	int    Init(PPViewPriceList * pView, const PPPriceListImpExpParam * pParam);
	int    Run();
private:
	PPPriceListImpExpParam Param;
	Sdr_PriceListArray Rows;
	Sdr_PriceListArray UnResolvedRows;
	PPViewPriceList * P_View;
};

int PPPriceListImporter::Init(PPViewPriceList * pView)
{
	int    ok = -1;
	THROW_INVARG(pView);
	P_View = pView;
	Rows.freeAll();
	UnResolvedRows.freeAll();
	ok = 1;
	CATCHZOK
	return ok;
}

int PPPriceListImporter::Init(PPViewPriceList * pView, const PPPriceListImpExpParam * pParam)
{
	int    ok = -1;
	THROW(Init(pView));
	ok = RVALUEPTR(Param, pParam) ? 1 : SelectPriceListImportCfg(&Param, 0);
	CATCHZOK
	return ok;
}

int PPPriceListImporter::Run()
{
	int    ok = -1, goods_resolved = 0;
	uint   i;
	long   count = 0;
	SString temp_buf;
	Sdr_PriceList pl_rec;
	ResolveGoodsItemList unres_goods_list;
	PPLogger logger;
	const PriceListFilt * p_filt = P_View ? static_cast<const PriceListFilt *>(P_View->GetBaseFilt()) : 0;
	ArticleCore  art_tbl;
	PPID   ar_id = (p_filt) ? p_filt->ArticleID : 0;
	PPImpExp ie(&Param, 0);
	PPObjGoods gobj;
	const  int use_ar_goodscode = BIN(CConfig.Flags & CCFLG_USEARGOODSCODE);
	PPWaitStart();
	THROW(ie.OpenFileForReading(0));
	ie.GetNumRecs(&count);
	for(i = 0; i < (uint)count; i++) {
		int    found = 0;
		BarcodeTbl::Rec bcrec;
		ArGoodsCodeTbl::Rec code_rec;
		MEMSZERO(pl_rec);
		// @v10.7.9 @ctr MEMSZERO(bcrec);
		// @v10.7.9 @ctr MEMSZERO(code_rec);
		THROW(ie.ReadRecord(&pl_rec, sizeof(pl_rec)));
		(temp_buf = pl_rec.Barcode).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(pl_rec.Barcode, temp_buf);
		(temp_buf = pl_rec.GoodsName).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(pl_rec.GoodsName, temp_buf);
		if(ar_id && use_ar_goodscode && gobj.P_Tbl->SearchByArCode(ar_id, pl_rec.Barcode, &code_rec) > 0) {
			pl_rec.GoodsID = code_rec.GoodsID;
			found = 1;
		}
		if(!found) {
			if(gobj.SearchByBarcode(pl_rec.Barcode, &bcrec) > 0) {
				found = 1;
				pl_rec.GoodsID = bcrec.GoodsID;
			}
			else if(gobj.SearchByName(pl_rec.GoodsName, &pl_rec.GoodsID) > 0)
				found = 1;
		}
		if(found) {
			if(!Rows.lsearch(&pl_rec, 0, CMPF_LONG))
				THROW_SL(Rows.insert(&pl_rec));
		}
		else {
			THROW_SL(UnResolvedRows.insert(&pl_rec));
		}
		PPWaitPercent(i + 1, count);
	}
	PPWaitStop();
	for(i = 0; i < UnResolvedRows.getCount(); i++) {
		ResolveGoodsItem g_i(i+1);
		STRNSCPY(g_i.Barcode,   UnResolvedRows.at(i).Barcode);
		STRNSCPY(g_i.GoodsName, UnResolvedRows.at(i).GoodsName);
		unres_goods_list.insert(&g_i);
	}
	if(unres_goods_list.getCount()) {
		if(ResolveGoodsDlg(&unres_goods_list, RESOLVEGF_SHOWBARCODE | RESOLVEGF_SHOWRESOLVED) > 0) {
			for(i = 0; i < unres_goods_list.getCount(); i++) {
				pl_rec = UnResolvedRows.at(i);
				pl_rec.GoodsID = unres_goods_list.at(i).ResolvedGoodsID;
				if(pl_rec.GoodsID && !Rows.lsearch(&pl_rec, 0, CMPF_LONG)) {
					THROW_SL(Rows.insert(&pl_rec));
				}
				else if(!pl_rec.GoodsID) {
					SString msg;
					msg.Printf(PPLoadTextS(PPTXT_UNRESOLVEDGOODSITEM, temp_buf), pl_rec.GoodsID, pl_rec.GoodsName, pl_rec.Barcode);
					logger.Log(msg);
				}
			}
			goods_resolved = 1;
		}
	}
	else
		goods_resolved = 1;
	if(goods_resolved > 0) {
		PPWaitStart();
		THROW(ok = P_View->UpdatePriceList(LConfig.OperDate, &Rows, 1));
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

double PPViewPriceList::GetRest(PPID goodsID)
{
	double rest = 0.0;
	if(Filt.Flags & PLISTF_CALCREST) {
		GoodsRestParam gp;
		gp.GoodsID     = goodsID;
		gp.CalcMethod  = GoodsRestParam::pcmSum;
		gp.Date        = Filt.Dt;
		if(State & stFiltArIsSupple) // @v9.3.4
			gp.SupplID = Filt.ArticleID;
		gp.LocID       = Filt.LocID;
		P_BObj->trfr->GetRest(gp);
		rest = gp.Total.Rest;
	}
	return rest;
}

int PPViewPriceList::SubstGoods(PPID goodsID, PPID * pSubstID, char * pBuf, size_t bufSize)
{
	int    ok = 1;
	PPID   subst_id = goodsID;
	SString temp_buf;
	PPObjGoods::SubstBlock sgg_blk;
	sgg_blk.LocID = Filt.LocID;
	THROW(GObj.SubstGoods(goodsID, &subst_id, Filt.Sgg, &sgg_blk, &Gsl));
	if(pBuf) {
		if(Filt.Sgg && goodsID != subst_id) {
			PPGoodsPacket  g_pack;
			PPGdsClsPacket gc_pack;
			THROW(GObj.GetPacket(goodsID, &g_pack, PPObjGoods::gpoSkipQuot)); // @v8.3.7 PPObjGoods::gpoSkipQuot
			THROW(GCObj.Fetch(g_pack.Rec.GdsClsID, &gc_pack));
			if(gc_pack.NameConv.NotEmptyS())
				gc_pack.GetNameByTemplate(&g_pack, gc_pack.NameConv, pBuf, bufSize, Filt.Sgg);
			else {
				GObj.GetSubstText(subst_id, Filt.Sgg, &Gsl, temp_buf);
				temp_buf.CopyTo(pBuf, bufSize);
			}
		}
		else {
			GetGoodsName(goodsID, temp_buf).CopyTo(pBuf, bufSize);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pSubstID, subst_id);
	return ok;
}

int PPViewPriceList::InitRPB(RecalcParamBlock * pRPB,
	Goods2Tbl::Rec * pGoodsRec, int lotPriceWoTaxes, double cost, double price, LDATE expiry)
{
	double tax_factor = 1.0;
	if(pGoodsRec->Flags & GF_PRICEWOTAXES || lotPriceWoTaxes)
		GObj.MultTaxFactor(pGoodsRec->ID, &tax_factor);
	if(lotPriceWoTaxes /*pGoodsRec->Flags & GF_PRICEWOTAXES*/)
		GObj.AdjPriceToTaxes(pGoodsRec->TaxGrpID, tax_factor, &price, 1);
	pRPB->Dt = Filt.Dt; // @v9.0.9
	pRPB->TaxFactor = tax_factor;
	pRPB->GoodsID   = pGoodsRec->ID;
	pRPB->GoodsGrpID = pGoodsRec->ParentID;
	pRPB->TaxGrpID  = pGoodsRec->TaxGrpID;
	pRPB->Cost      = cost;
	pRPB->BasePrice = price;
	pRPB->LotPriceWoTaxes   = lotPriceWoTaxes;
	pRPB->GoodsPriceWoTaxes = BIN(pGoodsRec->Flags & GF_PRICEWOTAXES);
	pRPB->Expiry = expiry;
	pRPB->Rest   = GetRest(pRPB->GoodsID);
	MEMSZERO(pRPB->AddPrices);
	for(uint i = 0; i < 3; i++)
		if(Cfg.AddPriceQuot[i]) {
			pRPB->AddPrices[i] = pRPB->BasePrice;
			GetPriceByQuot(pRPB, Cfg.AddPriceQuot[i], &pRPB->AddPrices[i]);
		}
	return 1;
}

int PPViewPriceList::UpdatePriceList(LDATE date, const Sdr_PriceListArray * pList, int useTa)
{
	int    ok = 1;
	PriceListTbl::Rec plist_rec;
	ReceiptTbl::Rec lot_rec;
	Goods2Tbl::Rec gr;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	RecalcParamBlock rpb;
	//
	GoodsFilt goods_filt;
	Gsl.Clear();
	ReadPriceListConfig(&Cfg);
	{
		PPTransaction tra(useTa);
		THROW(tra);
		THROW(Tbl.Search(Filt.PListID, &plist_rec) > 0);
		Filt.Dt = date;
		Tbl.data.Dt = date;
		Tbl.data.GoodsGrpID = Filt.GoodsGrpID;
		SETFLAG(Tbl.data.Flags, PLISTF_EXCLGGRP,    Filt.Flags & PLISTF_EXCLGGRP);
		SETFLAG(Tbl.data.Flags, PLISTF_PRESENTONLY, Filt.Flags & PLISTF_PRESENTONLY);
		THROW_DB(Tbl.updateRec());
		THROW(PPRef->PutPropArray(PPOBJ_PRICELIST, Filt.PListID, PLPRP_GDSGRPLIST, &Filt.GrpIDList.Get(), 0));
		{
			double price = 0.0;
			GoodsIterator iter;
			Goods2Tbl::Rec goods_grp_rec;
			PriceLineIdent pl_ident, pl_iter;
			// @v10.7.9 @ctr MEMSZERO(goods_grp_rec);
			pl_ident.PListID = Filt.PListID;
			pl_iter = pl_ident;
			while(Tbl.EnumLines(&pl_ident, &pl_iter) > 0)
				THROW(Tbl.RemoveLine(&pl_iter, 0));
			for(uint i = 0; i < pList->getCount(); i++) {
				const Sdr_PriceList & r_pl_rec = pList->at(i);
				THROW(GObj.Fetch(r_pl_rec.GoodsID, &gr) > 0);
				THROW(::GetCurGoodsPrice(gr.ID, plist_rec.LocID, GPRET_MOSTRECENT, &price, &lot_rec));
				price = r_pl_rec.Price;
				THROW(InitRPB(&rpb, &gr, 0, R5(lot_rec.Cost), price, ZERODATE));
				rpb.AddPrices[0] = r_pl_rec.AddPrice1;
				rpb.AddPrices[1] = r_pl_rec.AddPrice2;
				rpb.AddPrices[2] = r_pl_rec.AddPrice3;
				THROW(SetGoodsPrice(&rpb, -1, lot_rec.UnitPerPack, rpb.BasePrice, 1, 0));
				PPWaitPercent(iter.GetIterCounter());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	Gsl.Clear();
	if(ok > 0)
		UpdateTempTbl(0);
	return ok;
}

int PPViewPriceList::UpdatePriceList(LDATE date, int rmvOld, int use_ta)
{
	int    ok = 1;
	PriceListTbl::Rec plist_rec;
	PriceLineTbl::Rec linr;
	ReceiptTbl::Rec lot_rec;
	Goods2Tbl::Rec gr;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	RecalcParamBlock rpb;
	//
	GoodsFilt goods_filt;
	Gsl.Clear(); // AHTOXA
	goods_filt.LotPeriod = Filt.LotPeriod;
	goods_filt.SupplID   = Filt.LotSupplID;
	goods_filt.LocList.Add(Filt.LocID);
	goods_filt.Flags    |= (GoodsFilt::fExcludeAsset | GoodsFilt::fIncludeIntr);
	if(Filt.Flags & PLISTF_NEWP)
		goods_filt.Flags |= GoodsFilt::fNewLots;
	if(!(Filt.Flags & PLISTF_EXCLGGRP)) {
		goods_filt.GrpID = Filt.GoodsGrpID;
		goods_filt.GrpIDList = Filt.GrpIDList;
	}
	//
	ReadPriceListConfig(&Cfg);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Tbl.Search(Filt.PListID, &plist_rec) > 0);
		if((date && date != plist_rec.Dt) || Tbl.data.GoodsGrpID != Filt.GoodsGrpID ||
			(Tbl.data.Flags & PLISTF_EXCLGGRP) != (Filt.Flags & PLISTF_EXCLGGRP) ||
			(Tbl.data.Flags & PLISTF_PRESENTONLY) != (Filt.Flags & PLISTF_PRESENTONLY)) {
			Filt.Dt = date;
			Tbl.data.Dt = date;
			Tbl.data.GoodsGrpID = Filt.GoodsGrpID;
			SETFLAG(Tbl.data.Flags, PLISTF_EXCLGGRP,    Filt.Flags & PLISTF_EXCLGGRP);
			SETFLAG(Tbl.data.Flags, PLISTF_PRESENTONLY, Filt.Flags & PLISTF_PRESENTONLY);
			THROW_DB(Tbl.updateRec());
			THROW(PPRef->PutPropArray(PPOBJ_PRICELIST, Filt.PListID, PLPRP_GDSGRPLIST, &Filt.GrpIDList.Get(), 0));
		}
		if(!plist_rec.SupplID || Filt.Flags & PLISTF_BYQUOT) {
			double price;
			GoodsIterator iter;
			PriceLineIdent pl_ident, pl_iter;
			pl_ident.PListID = Filt.PListID;
			pl_iter = pl_ident;
			while(Tbl.EnumLines(&pl_ident, &pl_iter) > 0) {
				Goods2Tbl::Rec goods_rec;
				PPQuotKind qk_rec;
				if(rmvOld || GObj.Fetch(pl_iter.GoodsID, &goods_rec) < 0 || (pl_iter.QuotKindID > 0 && rpb.QkObj.Fetch(pl_iter.QuotKindID, &qk_rec) < 0)) {
					THROW(Tbl.RemoveLine(&pl_iter, 0));
				}
			}
			iter.Init(&goods_filt, 0);
			while(iter.Next(&gr) > 0) {
				if(Filt.Flags & PLISTF_EXCLGGRP && (Filt.GoodsGrpID || Filt.GrpIDList.IsExists())) {
					int   is_found   = 0;
					uint  grp_count  = Filt.GrpIDList.IsExists() ? Filt.GrpIDList.GetCount() : 1;
					PPID  gds_grp_id = Filt.GoodsGrpID;
					for(uint i = 0; !is_found && i < grp_count; i++) {
						if(Filt.GrpIDList.IsExists())
							gds_grp_id = Filt.GrpIDList.Get().at(i);
						if(GObj.BelongToGroup(gr.ID, gds_grp_id) > 0)
							is_found = 1;
					}
					if(is_found)
						continue;
				}
				PPID   quot_kind_id;
				int    is_present = 0, r;
				Goods2Tbl::Rec goods_grp_rec;
				if(GObj.CheckFlag(gr.ID, GF_UNLIM)) {
					const QuotIdent qi(plist_rec.LocID, PPQUOTK_BASE);
					THROW(r = GObj.GetQuotExt(gr.ID, qi, &price, 1));
					MEMSZERO(lot_rec);
					r = GPRET_PRESENT;
				}
				else {
					//
					// Цену определяем по самому последнему лоту,
					//
					THROW(::GetCurGoodsPrice(gr.ID, plist_rec.LocID, GPRET_MOSTRECENT, &price, &lot_rec));
					//
					// Теперь снова вызываем GetCurrentGoodsPrice с флагом GPRET_PRESENT
					// для определения того, есть ли указанный товар на остатке
					//
					THROW(r = ::GetCurGoodsPrice(gr.ID, plist_rec.LocID, 0, 0, 0));
				}
				if(gr.TaxGrpID == 0 && GObj.Fetch(gr.ParentID, &goods_grp_rec) > 0)
					gr.TaxGrpID = goods_grp_rec.TaxGrpID;
				THROW(InitRPB(&rpb, &gr, BIN(lot_rec.Flags & LOTF_PRICEWOTAXES), R5(lot_rec.Cost), price, lot_rec.Expiry));
				if(r == GPRET_PRESENT || (plist_rec.LocID == 0 && r == GPRET_OTHERLOC)) {
					is_present = 1;
					quot_kind_id = (plist_rec.Flags & PLISTF_BYQUOT) ? 0L : -1L;
					THROW(SetGoodsPrice(&rpb, quot_kind_id, lot_rec.UnitPerPack, rpb.BasePrice, is_present, 0));
				}
				else if(!(Filt.Flags & PLISTF_PRESENTONLY) && oneof2(r, GPRET_CLOSEDLOTS, GPRET_OTHERLOC)) {
					is_present = 0;
					quot_kind_id = (plist_rec.Flags & PLISTF_BYQUOT) ? 0L : -1L;
					THROW(SetGoodsPrice(&rpb, quot_kind_id, lot_rec.UnitPerPack, rpb.BasePrice, is_present, 0));
				}
				if(plist_rec.QuotKindID >= 0)
					THROW(SetByQuot(&rpb, lot_rec.UnitPerPack, is_present, 0));
				if(!is_present) {
					PPID   subst_id = 0;
					MEMSZERO(pl_ident);
					pl_ident.PListID = Filt.PListID;
					THROW(GObj.SubstGoods(gr.ID, &subst_id, Filt.Sgg, 0, &Gsl)); // AHTOXA
					pl_ident.GoodsID = subst_id;
					pl_iter = pl_ident;
					while(Tbl.EnumLines(&pl_ident, &pl_iter, &linr) > 0)
						if(linr.IsPresent) {
							linr.IsPresent = 0;
							THROW(Tbl.UpdateLine(&pl_iter, &linr, BIN(Filt.Sgg), 0));
						}
				}
				PPWaitPercent(iter.GetIterCounter());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	Gsl.Clear(); // AHTOXA
	return ok;
}

int PPViewPriceList::AddPriceList(PPID * pListID, PriceListTbl::Rec * pRec, int useTa)
{
	return Tbl.Add(pListID, pRec, useTa);
}

int PPViewPriceList::SearchListByFilt(PriceListFilt * pFilt, PPID * pID, PriceListTbl::Rec * pRec)
{
	PPID   quot_kind_id = (pFilt->Flags & PLISTF_BYQUOT) ? pFilt->QuotKindID : -1L;
	PPID   user_id = (pFilt->Flags & PLISTF_USECOMMON) ? -1L : LConfig.UserID;
	PriceListTbl::Key1 k;
	k.SupplID = pFilt->ArticleID;
	k.UserID  = user_id;
	k.LocID   = pFilt->LocID;
	k.QuotKindID = quot_kind_id;
	k.Dt      = MAXDATE;
	if(Tbl.search(1, &k, spLe) && k.SupplID == pFilt->ArticleID &&
		k.LocID == pFilt->LocID && k.QuotKindID == quot_kind_id && k.UserID == user_id) {
		ASSIGN_PTR(pID, Tbl.data.ID);
		ASSIGN_PTR(pRec, Tbl.data);
		return 1;
	}
	else
		return PPDbSearchError();
}

int PPViewPriceList::UpdateTempTbl(PriceLineIdent * pIdent)
{
	int    ok = -1;
	PriceLineTbl::Rec rec;
	if(P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(pIdent) {
			if(SearchLine(pIdent, &rec) > 0) {
				rec.Rest = GetRest(pIdent->GoodsID); // AHTOXA
				PriceLineTbl::Key0 k;
				k.ListID     = pIdent->PListID;
				k.GoodsID    = pIdent->GoodsID;
				k.QuotKindID = pIdent->QuotKindID;
				k.LineNo     = pIdent->LineNo;
				if(SearchByKey_ForUpdate(P_TempTbl, 0, &k, 0) > 0) {
					THROW_DB(P_TempTbl->updateRecBuf(&rec)); // @sfu
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, (P_TempTbl->ListID == pIdent->PListID &&
					P_TempTbl->GoodsID == pIdent->GoodsID && P_TempTbl->QuotKindID == pIdent->QuotKindID &&
					P_TempTbl->LineNo == (long)pIdent->LineNo)));
			}
		}
		else {
			BExtInsert bei(P_TempTbl);
			PriceLineTbl::Key0 k;
			THROW_DB(deleteFrom(P_TempTbl, 0, *reinterpret_cast<DBQ *>(0)));
			MEMSZERO(k);
			k.ListID = Filt.PListID;
			BExtQuery q(&Tbl.Lines, 0);
			DBQ * dbq = &(Tbl.Lines.ListID == Filt.PListID);
			if(Filt.QuotKindID > 0 && !(Filt.Flags & PLISTF_IGNZEROQUOT))
				dbq = &(*dbq && (Tbl.Lines.QuotKindID == Filt.QuotKindID));
			if(Filt.Flags & PLISTF_PRESENTONLY)
				dbq = &(*dbq && (Tbl.Lines.IsPresent > 0L));
			q.selectAll().where(*dbq);
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
				Tbl.Lines.copyBufTo(&rec);
				int   is_found   = 0;
				uint  grp_count  = Filt.GrpIDList.IsExists() ? Filt.GrpIDList.GetCount() : 1;
				PPID  gds_grp_id = Filt.GoodsGrpID;
				for(uint i = 0; !is_found && i < grp_count; i++) {
					if(Filt.GrpIDList.IsExists())
						gds_grp_id = Filt.GrpIDList.Get().at(i);
					if(GObj.BelongToGroup(rec.GoodsID, gds_grp_id) > 0)
						is_found = 1;
				}
				if((Filt.Flags & PLISTF_EXCLGGRP) ? is_found : !is_found)
					continue;
				if(Filt.QuotKindID > 0 && !(Filt.Flags & PLISTF_IGNZEROQUOT) && rec.QuotKindID != Filt.QuotKindID)
					continue;
				if(Filt.Flags & PLISTF_PRESENTONLY && rec.IsPresent == 0)
					continue;
				if(!Filt.PriceRange.IsZero() && Filt.PriceRange.Check(rec.Price) == 0)
					continue;
				rec.Rest = GetRest(rec.GoodsID); // AHTOXA
				THROW_DB(bei.insert(&rec));
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, PriceLine);

int PPViewPriceList::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	// @v9.3.4 {
	{
		State = 0;
		if(Filt.ArticleID) {
			PPID   acs_id = 0;
            if(GetArticleSheetID(Filt.ArticleID, &acs_id, 0) > 0 && acs_id && acs_id == GetSupplAccSheet())
				State |= stFiltArIsSupple;
		}
	}
	// } @v9.3.4
	if(!Filt.GoodsIDList.GetCount()) {
		THROW(Tbl.Search(Filt.PListID) > 0);
		BExtQuery::ZDelete(&P_IterQuery);
		ZDELETE(P_GGIter);
		ZDELETE(P_TempTbl);
		ReadPriceListConfig(&Cfg);
		if(Filt.GoodsGrpID || Filt.GrpIDList.IsExists() || Filt.PriceRange.IsZero() == 0) {
			Goods2Tbl::Rec goods_grp_rec;
			if(Filt.PriceRange.IsZero() == 0 || (Filt.Flags & PLISTF_EXCLGGRP) || Filt.GrpIDList.IsExists() || (
				GObj.Fetch(Filt.GoodsGrpID, &goods_grp_rec) > 0 && (goods_grp_rec.Flags & (GF_FOLDER | GF_ALTGROUP)))) {
				ZDELETE(P_TempTbl);
				Gsl.Clear();
				THROW(P_TempTbl = CreateTempFile());
				THROW(UpdateTempTbl(0));
				Gsl.Clear();
			}
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewPriceList::InitIterQuery(PPID grpID)
{
	union {
		PriceLineTbl::Key1 k1;
		PriceLineTbl::Key2 k2;
	} k, k_;
	int    sp_mode = spFirst;
	DBQ  * dbq = 0;
	PriceLineTbl * p_tbl = NZOR(P_TempTbl, &Tbl.Lines);
	delete P_IterQuery;
	P_IterQuery = new BExtQuery(p_tbl, IterIdx, 64);
	P_IterQuery->select(p_tbl->ListID, p_tbl->GoodsID, p_tbl->LineNo, p_tbl->QuotKindID, 0L);
	MEMSZERO(k);
	if(grpID) {
		if(P_TempTbl)
			dbq = & (p_tbl->GoodsGrpID == grpID);
		else {
			dbq = & (p_tbl->ListID == Filt.PListID && p_tbl->GoodsGrpID == grpID);
			if(Filt.QuotKindID > 0 && !(Filt.Flags & PLISTF_IGNZEROQUOT))
				dbq = & (*dbq && p_tbl->QuotKindID == Filt.QuotKindID);
			if(Filt.Flags & PLISTF_PRESENTONLY)
				dbq = & (*dbq && p_tbl->IsPresent == 1L);
		}
		P_IterQuery->where(*dbq);
		k.k2.ListID = Filt.PListID;
		k.k2.GoodsGrpID = grpID;
		sp_mode = spGe;
	}
	else {
		if(!P_TempTbl) {
			dbq = & (p_tbl->ListID == Filt.PListID);
			if(Filt.QuotKindID > 0 && !(Filt.Flags & PLISTF_IGNZEROQUOT))
				dbq = & (*dbq && p_tbl->QuotKindID == Filt.QuotKindID);
			if(Filt.Flags & PLISTF_PRESENTONLY)
				dbq = & (*dbq && p_tbl->IsPresent == 1L);
			P_IterQuery->where(*dbq);
		}
		k.k1.ListID = Filt.PListID;
		sp_mode = spGe;
	}
	if(grpID == 0) {
		k_ = k;
		Counter.Init(P_IterQuery->countIterations(0, &k_, sp_mode));
	}
	P_IterQuery->initIteration(0, &k, sp_mode);
	return 1;
}

int PPViewPriceList::InitIteration(IterOrder ord)
{
	int    ok = 1;
	IterIdx = 0;
	ZDELETE(P_GGIter);
	if(!Filt.GoodsIDList.GetCount()) {
		if(ord == PPViewPriceList::OrdByGoodsName || Filt.Sgg != sggNone)
			IterIdx = 1;
		else if(ord == PPViewPriceList::OrdByGrpName_GoodsName)
			IterIdx = 2;
		else
			IterIdx = 2;
		BExtQuery::ZDelete(&P_IterQuery);
		InitIterQuery(0);
		if(IterIdx == 2) {
			BExtQuery::ZDelete(&P_IterQuery);
			PPID   init_grp_id = 0;
			IterGrpName_ = 0;
			if(Filt.Flags & PLISTF_EXCLGGRP || Filt.GrpIDList.IsExists() || PPObjGoodsGroup::IsAlt(Filt.GoodsGrpID) > 0)
				init_grp_id = 0;
			else
				init_grp_id = Filt.GoodsGrpID;
			P_GGIter = new GoodsGroupIterator(init_grp_id,
				(ord == PPViewPriceList::OrdByGrpCode_GoodsName) ? GoodsGroupIterator::fSortByCode : 0);
			ok = (NextOuterIteration() > 0) ? 1 : -1;
		}
	}
	else
		Counter.Init(Filt.GoodsIDList.GetCount());
	return ok;
}

int PPViewPriceList::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName_) > 0) {
		InitIterQuery(grp_id);
		return 1;
	}
	else
		return -1;
}

// AHTOXA { для печати ценников по списку товаров
int PPViewPriceList::NextIterationByList(PriceListViewItem * pItem)
{
	int    ok = -1;
	const  uint i = Counter;
	const  PPIDArray & goods_list = Filt.GoodsIDList.Get();
	PPID   goods_id = 0;
	if(i < goods_list.getCount()) {
		Goods2Tbl::Rec goods_rec;
		RetailGoodsInfo rgi;
		goods_id = goods_list.at(i);
		if(GObj.Fetch(goods_id, &goods_rec) > 0) {
			GObj.GetRetailGoodsInfo(goods_id, Filt.LocID, &rgi);
			pItem->PListID      = 0;
			pItem->LineNo       = i;
			pItem->LN   = i;
			pItem->GoodsGrpID   = goods_rec.ParentID;
			pItem->GoodsID      = goods_id;
			pItem->QuotKindID   = 0;
			pItem->ManufID      = goods_rec.ManufID;
			pItem->UnitID       = goods_rec.UnitID;
			pItem->UnitsPerPack = 0;
			pItem->Price        = rgi.Price;
			pItem->AddPrice1    = 0;
			pItem->AddPrice2    = 0;
			pItem->AddPrice3    = 0;
			pItem->Expiry       = ZERODATE;
			pItem->GoodsGrpName_ = IterGrpName_;
			/* @v9.2.2 if(Filt.GoodsGrpID && GObj.IsAltGroup(Filt.GoodsGrpID) > 0)
				if(PPRef->Assc.Search(PPASS_ALTGOODSGRP, Filt.GoodsGrpID, goods_id) > 0)
					pItem->GoodsCode = PPRef->Assc.data.InnerNum; */
			GObj.P_Tbl->GetGoodsCodeInAltGrp(goods_id, Filt.GoodsGrpID, &pItem->GoodsCode); // @v9.2.2
			pItem->GoodsName_ = goods_rec.Name;
			GetLastQCertID(pItem->GoodsID, Filt.Dt, &pItem->QCertID);
		}
		Counter.Increment();
		ok = 1;
	}
	return ok;
}
// } AHTOXA

int FASTCALL PPViewPriceList::NextIteration(PriceListViewItem * pItem)
{
	// AHTOXA { для печати ценников по списку товаров
	if(Filt.GoodsIDList.GetCount())
		return NextIterationByList(pItem);
	// } AHTOXA
	do {
		if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			if(pItem) {
				// memzero(pItem, sizeof(PriceListViewItem));
				pItem->Clear();
				PriceLineTbl * p_tbl = NZOR(P_TempTbl, &Tbl.Lines);
				DBRowId pos;
				if(P_IterQuery->getRecPosition(&pos) && p_tbl->getDirect(-1, 0, pos)) {
					PriceLineTbl::Rec & rec = p_tbl->data;
					pItem->PListID      = rec.ListID;
					pItem->LineNo       = rec.LineNo;
					pItem->LN   = Counter;
					pItem->GoodsGrpID   = rec.GoodsGrpID;
					pItem->GoodsID      = rec.GoodsID;
					pItem->QuotKindID   = rec.QuotKindID;
					pItem->ManufID      = rec.ManufID;
					pItem->UnitID       = rec.UnitID;
					pItem->UnitsPerPack = rec.UnitPerPack;
					pItem->Price        = rec.Price;
					pItem->AddPrice1    = rec.AddPrice1;
					pItem->AddPrice2    = rec.AddPrice2;
					pItem->AddPrice3    = rec.AddPrice3;
					pItem->Expiry       = rec.Expiry;
					pItem->Rest = rec.Rest;
					pItem->GoodsGrpName_ = IterGrpName_;
					pItem->GoodsCode    = rec.GoodsCode;
					pItem->GoodsName_   = rec.Name;
					pItem->Memo_        = rec.Memo;
					GetLastQCertID(pItem->GoodsID, Filt.Dt, &pItem->QCertID);
				}
				else
					return PPSetErrorDB();
			}
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

int PPViewPriceList::RemoveLine(PriceLineIdent * pIdent)
{
	int    ok = -1;
	if(pIdent) {
		if((Cfg.Flags & PLISTF_NOTCFMDEL) || CONFIRM(PPCFM_DELETE)) { // AHTOXA
			THROW(Tbl.RemoveLine(pIdent, 1));
			UpdateTempTbl(pIdent);
			ok = 1;
		}
	}
	else if(CONFIRM(PPCFM_REMOVEALLBYFILT)) {
		PriceLineIdent pl_ident, pl_iter;
		pl_ident.PListID = Filt.PListID;
		pl_iter = pl_ident;
		PPWaitStart();
		{
			PPTransaction tra(1);
			THROW(tra);
			while(Tbl.EnumLines(&pl_ident, &pl_iter) > 0)
				THROW(Tbl.RemoveLine(&pl_iter, 0));
			THROW(tra.Commit());
		}
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPriceList::Print(const void * pHdr)
{
	int    ok = 1, rpt_id;
	ushort v = 0;
	TDialog * dlg = 0;
	double pctadd = 0.0;
	PPReportEnv env;
	env.Sort = OrdByDefault;
	THROW(Tbl.Search(Filt.PListID) > 0);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNPLST))));
	dlg->setCtrlData(CTL_PRNPLST_WHAT, &v);
	dlg->setCtrlData(CTL_PRNPLST_PCTADD, &pctadd);
	dlg->setCtrlData(CTL_PRNPLST_SORTBY, &(v = 0));
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_PRNPLST_SORTBY, &(v = 0));
		if(v == 2)
			env.Sort = OrdByGrpCode_GoodsName;
		else if(v == 1)
			env.Sort = OrdByGrpName_GoodsName;
		else
			env.Sort = OrdByDefault;
		dlg->getCtrlData(CTL_PRNPLST_WHAT,   &(v = 0));
		dlg->getCtrlData(CTL_PRNPLST_PCTADD, &pctadd);
		//
		const int use_pckg = BIN(LConfig.Flags & CFGFLG_USEPACKAGE);
		Filt.Flags &= ~PLISTF_FILLQCERT;
		if(v == 0)         // Собственно прайс-лист
			rpt_id = use_pckg ? REPORT_PRICELISTPACK : REPORT_PRICELIST;
		else if(v == 1)   // Прайс-лист с производителем
			rpt_id = use_pckg ? REPORT_PRICELISTPACKM : REPORT_PRICELISTM;
		else if(v == 2) { // Прайс-лист с дополнительными ценами
			if(Cfg.AddPriceQuot[0])
				if(Cfg.AddPriceQuot[1])
					if(Cfg.AddPriceQuot[2])
						rpt_id = REPORT_PRICELISTA3;
					else
						rpt_id = REPORT_PRICELISTA2;
				else
					rpt_id = REPORT_PRICELISTA1;
			else // default [same as (v == 0)]
				rpt_id = use_pckg ? REPORT_PRICELISTPACK : REPORT_PRICELIST;
		}
		else if(v == 3) { // Сертификаты к прайс-листу
			rpt_id = REPORT_QCERTLISTPL;
			Filt.Flags |= PLISTF_FILLQCERT;
		}
		else if(v == 4) // большие ценники к прайс листу
			rpt_id = REPORT_PLABELBIG;
		else if(v == 5) // маленькие ценники к прайс листу
			rpt_id = REPORT_PLABELSMALL;
		Filt.PctAdd = pctadd;
		PPAlddPrint(rpt_id, PView(this), &env);
	}
	else
		ok = -1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int PPViewPriceList::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		PriceLineIdent ident;
		if(pHdr) {
			const struct _H {
				PPID   GoodsID;
				PPID   QuotKindID;
				short  LineNo;
			} * h = static_cast<const _H *>(pHdr);
			ident.PListID    = Filt.PListID;
			ident.GoodsID    = h->GoodsID;
			ident.QuotKindID = h->QuotKindID;
			ident.LineNo     = h->LineNo;
		}
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				ok = (ident.PListID && EditLine(&ident) > 0) ? 1 : -1;
				break;
			case PPVCMD_ADDITEM:
				ok = -1;
				{
					PPID   goods_grp_id = NewGoodsGrpID;
					long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags(); // @v10.7.7
					if(Filt.Flags & PLISTF_PRESENTONLY) 
						egsd_flags |= ExtGoodsSelDialog::fExistsOnly;
					ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, goods_grp_id, egsd_flags);
					if(CheckDialogPtrErr(&dlg)) {
						while(ExecView(dlg) == cmOK) {
							TIDlgInitData tidi;
							if(dlg->getDTS(&tidi) > 0) {
								NewGoodsGrpID = tidi.GoodsGrpID;
								PriceLineIdent _ident;
								_ident.PListID    = Filt.PListID;
								_ident.GoodsID    = tidi.GoodsID;
								_ident.QuotKindID = 0;
								_ident.LineNo     = 0;
								if(AddLine(&_ident) > 0)
									pBrw->Update();
							}
						}
						delete dlg;
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = (ident.PListID && RemoveLine(&ident) > 0) ? 1 : -1;
				break;
			case PPVCMD_DELETEALL:
				ok = RemoveLine(0);
				break;
			case PPVCMD_EDITGOODS:
				ok = (ident.GoodsID > 0 && GObj.Edit(&ident.GoodsID, 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(ident.GoodsID)
					::ViewLots(ident.GoodsID, Filt.LocID, 0, 0, 0);
				break;
			case PPVCMD_ADDTOBASKET:
				ok = -1;
				{
					PriceLineTbl::Rec rec;
					if(ident.PListID && SearchLine(&ident, &rec) > 0)
						AddGoodsToBasket(rec.GoodsID, Filt.LocID, 0, rec.Price);
				}
				break;
			case PPVCMD_ADDFROMBASKET:
				ok = ConvertBasketToLines();
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				ConvertLinesToBasket();
				break;
			/* @v7.6.0
			case PPVCMD_POSTALBATROS:
				ok = -1;
				SendPList();
				break;
			*/
			case PPVCMD_EXPORTUHTT:
				ok = -1;
				ExportUhtt();
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_IMPORT:
				ok = -1;
				{
					int    ta = 0;
					PPPriceListImporter imp;
					int    r = imp.Init(this, 0);
					ok = (r > 0) ? imp.Run() : (r ? -1 : 0);
					if(!ok)
						PPError();
				}
				break;
			case PPVCMD_REFRESH:
				ok = -1;
				if(Tbl.Search(Filt.PListID) > 0) {
					TDialog * dlg = new TDialog(DLG_PLISTUPD);
					if(CheckDialogPtrErr(&dlg)) {
						LDATE   plist_date = getcurdate_();
						dlg->SetupCalDate(CTLCAL_PLISTUPD_DT, CTL_PLISTUPD_DT);
						dlg->setCtrlUInt16(CTL_PLISTUPD_SEL, 0);
						dlg->setCtrlDate(CTL_PLISTUPD_DT, plist_date);
						if(ExecView(dlg) == cmOK) {
							ushort v = dlg->getCtrlUInt16(CTL_PLISTUPD_SEL);
							plist_date = dlg->getCtrlDate(CTL_PLISTUPD_DT);
							if(!checkdate(plist_date))
								plist_date = getcurdate_();
							PPWaitStart();
							if(!UpdatePriceList(plist_date, (v == 1), 1))
								ok = PPErrorZ();
							else {
								UpdateTempTbl(0);
								ok = 1;
							}
							PPWaitStop();
						}
						delete dlg;
					}
				}
				break;
		}
	}
	return ok;
}

DBQuery * PPViewPriceList::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_PLINES;
	int    by_all_quots = BIN(Filt.Flags & PLISTF_BYQUOT && Filt.QuotKindID == 0);
	PriceLineTbl * pl = P_TempTbl ? new PriceLineTbl(P_TempTbl->GetName()) : new PriceLineTbl;
	DBQuery * q = 0;
	DBQ  * dbq = 0;
	DBE    dbe_unit;
	DBE    dbe_quotkind;
	if(by_all_quots)
		brw_id = BROWSER_PLINES_BYALLQUOTS;
	else
		brw_id = BROWSER_PLINES;
	THROW(CheckTblPtr(pl));
	PPDbqFuncPool::InitObjNameFunc(dbe_unit,     PPDbqFuncPool::IdObjNameUnit,     pl->UnitID);
	PPDbqFuncPool::InitObjNameFunc(dbe_quotkind, PPDbqFuncPool::IdObjNameQuotKind, pl->QuotKindID);
	if(!P_TempTbl) {
		dbq = & (pl->ListID == Filt.PListID);
		dbq = ppcheckfiltid(dbq, pl->GoodsGrpID, Filt.GoodsGrpID);
		dbq = ppcheckfiltid(dbq, pl->ManufID,    Filt.ManufID);
		if(!(Filt.Flags & PLISTF_IGNZEROQUOT))
			dbq = ppcheckfiltid(dbq, pl->QuotKindID, Filt.QuotKindID);
		if(Filt.Flags & PLISTF_PRESENTONLY)
			dbq = & (*dbq && pl->IsPresent == 1L);
	}
	q = & select(
		pl->GoodsID,       // #00
		pl->QuotKindID,    // #01
		pl->LineNo,        // #02
		pl->Name,          // #03
		dbe_unit,          // #04
		pl->UnitPerPack,   // #05
		pl->Price,         // #06
		pl->AddPrice1,     // #07
		pl->AddPrice2,     // #08
		pl->AddPrice3,     // #09
		pl->Memo,          // #10
		pl->Rest,          // #11
		dbe_quotkind,      // #12
		0L).from(pl, 0L).where(*dbq);
	if(Filt.GoodsGrpID && !P_TempTbl)
		q->orderBy(pl->ListID, pl->GoodsGrpID, pl->Name, 0L);
	else
		q->orderBy(pl->ListID, pl->Name, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		GetLocationName(Filt.LocID, *pSubTitle);
	}
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete pl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

void PPViewPriceList::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & PLISTF_CALCREST) {
			uint rest_col = (Filt.Flags & PLISTF_BYQUOT && Filt.QuotKindID == 0) ? 5 : 7;
			pBrw->insertColumn(rest_col, "@rest", 11, 0, MKSFMTD(0, 3, NMBF_NOZERO | NMBF_NOTRAILZ), 0);
		}
	}
}
//
// PLineFiltDialog
//
class PLineFiltDialog : public TDialog {
public:
	PLineFiltDialog() : TDialog(DLG_PLINEFLT)
	{
	}
	int    setDTS(const PriceListFilt *);
	int    getDTS(PriceListFilt *);
private:
	DECL_HANDLE_EVENT;
	PriceListFilt Data;
};

int PLineFiltDialog::setDTS(const PriceListFilt * pFilt)
{
	int    ok = -1;
	if(RVALUEPTR(Data, pFilt)) {
		SetupPPObjCombo(this, CTLSEL_PLINEFLT_GGRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
		if(Data.GrpIDList.IsExists()) {
			SetComboBoxListText(this, CTLSEL_PLINEFLT_GGRP);
			disableCtrl(CTLSEL_PLINEFLT_GGRP, 1);
		}
		else
			disableCtrl(CTLSEL_PLINEFLT_GGRP, 0);
		setCtrlUInt16(CTL_PLINEFLT_EXCLGGRP, BIN(Data.Flags & PLISTF_EXCLGGRP));
		setCtrlUInt16(CTL_PLINEFLT_FLAGS, BIN(Data.Flags & PLISTF_PRESENTONLY));
		ok = 1;
	}
	return ok;
}

int PLineFiltDialog::getDTS(PriceListFilt * pFilt)
{
	int    ok = 1;
	Data.ManufID = 0;
	getCtrlData(CTLSEL_PLINEFLT_GGRP, &Data.GoodsGrpID);
	SETFLAG(Data.Flags, PLISTF_PRESENTONLY, getCtrlUInt16(CTL_PLINEFLT_FLAGS));
	SETFLAG(Data.Flags, PLISTF_EXCLGGRP,    getCtrlUInt16(CTL_PLINEFLT_EXCLGGRP));
	ASSIGN_PTR(pFilt, Data);
	return ok;
}

IMPL_HANDLE_EVENT(PLineFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmGrpList)) {
		{
			PPIDArray temp_list;
			Data.GrpIDList.CopyTo(&temp_list);
			ListToListData ll_data(PPOBJ_GOODSGROUP, (void *)GGRTYP_SEL_NORMAL, &temp_list);
			ll_data.Flags     |= ListToListData::fIsTreeList;
			ll_data.TitleStrID = PPTXT_SELGOODSGRPS;
			if(ListToListDialog(&ll_data)) {
				Data.GrpIDList.Set(&temp_list);
				Data.Setup();
				setCtrlData(CTLSEL_PLINEFLT_GGRP, &Data.GoodsGrpID);
				if(Data.GrpIDList.IsExists()) {
					SetComboBoxListText(this, CTLSEL_PLINEFLT_GGRP);
					disableCtrl(CTLSEL_PLINEFLT_GGRP, 1);
				}
				else
					disableCtrl(CTLSEL_PLINEFLT_GGRP, 0);
			}
			else
				PPError();
		}
		clearEvent(event);
	}
}
//
// PListFiltDialog
//
class PListFiltDialog : public TDialog {
public:
	enum {
		ctlgroupGoodsGrp = 1
	};
	explicit PListFiltDialog(PPViewPriceList * pV) : TDialog(DLG_PLISTFLT), P_PLV(pV)
	{
		addGroup(ctlgroupGoodsGrp, new GoodsFiltCtrlGroup(0, CTLSEL_PLIST_GGRP, cmGoodsFilt));
	}
	int    setDTS(const PriceListFilt *);
	int    getDTS(PriceListFilt *);
private:
	DECL_HANDLE_EVENT;
	void   setupList();

	PPViewPriceList * P_PLV;
	PriceListFilt Data;
};

IMPL_HANDLE_EVENT(PListFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_PLIST_LOC)) {
		getCtrlData(CTLSEL_PLIST_LOC, &Data.LocID);
		setupList();
	}
	else if(event.isCbSelected(CTLSEL_PLIST_QUOTK)) {
		getCtrlData(CTLSEL_PLIST_QUOTK, &Data.QuotKindID);
		setupList();
	}
	else if(event.isCbSelected(CTLSEL_PLIST_ACCSHEET)) {
		const PPID acs_id = getCtrlLong(CTLSEL_PLIST_ACCSHEET);
		SetupArCombo(this, CTLSEL_PLIST_ARTICLE, (Data.ArticleID = 0), OLW_LOADDEFONOPEN, acs_id, sacfDisableIfZeroSheet);
		setupList();
	}
	else if(event.isClusterClk(CTL_PLIST_KIND)) {
		ushort v = getCtrlUInt16(CTL_PLIST_KIND);
		SETFLAG(Data.Flags, PLISTF_BYQUOT, v);
		if(Data.Flags & PLISTF_BYQUOT) {
			if(Data.QuotKindID < 0)
				Data.QuotKindID = 0L;
		}
		else if(Data.QuotKindID >= 0)
			Data.QuotKindID = -1L;
		setupList();
	}
	else if(event.isCmd(cmAdvOptions)) {
		GoodsFilt g_filt;
		g_filt.LotPeriod = Data.LotPeriod;
		SETFLAG(g_filt.Flags, GoodsFilt::fNewLots, Data.Flags & PLISTF_NEWP);
		g_filt.SupplID = Data.LotSupplID;
		g_filt.LocList.Add(Data.LocID);
		if(GoodsFilterAdvDialog(&g_filt, 0) > 0) {
			Data.LotSupplID = g_filt.SupplID;
			Data.LotPeriod  = g_filt.LotPeriod;
			SETFLAG(Data.Flags, PLISTF_NEWP, g_filt.Flags & GoodsFilt::fNewLots);
			Data.LocID = g_filt.LocList.GetSingle();
			SetupPPObjCombo(this, CTLSEL_PLIST_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
		}
	}
	else
		return;
	clearEvent(event);
}

void PListFiltDialog::setupList()
{
	int    r;
	PPID   plist_id = 0;
	PriceListTbl::Rec plist_rec;
	disableCtrl(CTLSEL_PLIST_QUOTK, (Data.Flags & PLISTF_BYQUOT) ? 0 : 1);
	if((r = P_PLV->SearchListByFilt(&Data, &plist_id, &plist_rec)) > 0) {
		Data.Dt = plist_rec.Dt;
		setCtrlData(CTL_PLIST_DATE, &plist_rec.Dt);
		setCtrlData(CTL_PLIST_MEMO, plist_rec.Memo);
		Data.PListID = plist_id;
		Data.GoodsGrpID = plist_rec.GoodsGrpID;
		SETFLAG(Data.Flags, PLISTF_EXCLGGRP,    plist_rec.Flags & PLISTF_EXCLGGRP);
		SETFLAG(Data.Flags, PLISTF_PRESENTONLY, plist_rec.Flags & PLISTF_PRESENTONLY);
		SETFLAG(Data.Flags, PLISTF_IGNZEROQUOT, plist_rec.Flags & PLISTF_IGNZEROQUOT);
		Data.Memo = plist_rec.Memo;
	}
	else {
		char   tmp[16];
		LDATE  dt = LConfig.OperDate;
		Data.PListID = 0;
		setCtrlData(CTL_PLIST_DATE, &dt);
		PTR32(tmp)[0] = 0;
		setCtrlData(CTL_PLIST_MEMO, tmp);
		if(!r)
			PPError();
	}
	if(Data.PListID && !Data.GoodsGrpID) {
		PPIDArray temp_list;
		PPRef->GetPropArray(PPOBJ_PRICELIST, Data.PListID, PLPRP_GDSGRPLIST, &temp_list);
		Data.GrpIDList.Set(temp_list.getCount() ? &temp_list : 0);
	}
}

int PListFiltDialog::setDTS(const PriceListFilt * pFilt)
{
	int    ok = 1;
	PPID   acs_id = 0;
	Data = *pFilt;
	SetupPPObjCombo(this, CTLSEL_PLIST_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	SetupPPObjCombo(this, CTLSEL_PLIST_QUOTK, PPOBJ_QUOTKIND, ((Data.QuotKindID > 0) ? Data.QuotKindID : 0L), 0, 0);
	setCtrlData(CTL_PLIST_DATE, &Data.Dt);
	ushort v = BIN(Data.Flags & PLISTF_BYQUOT);
	disableCtrl(CTLSEL_PLIST_QUOTK, v == 0);
	setCtrlUInt16(CTL_PLIST_KIND, v);
	setCtrlString(CTL_PLIST_MEMO, Data.Memo);
	SetupSubstGoodsCombo(this, CTLSEL_PLIST_SUBST, Data.Sgg);
	GetArticleSheetID(Data.ArticleID, &acs_id);
	SetupPPObjCombo(this, CTLSEL_PLIST_ACCSHEET, PPOBJ_ACCSHEET, acs_id, 0, 0);
	SetupArCombo(this, CTLSEL_PLIST_ARTICLE, Data.ArticleID, OLW_LOADDEFONOPEN, acs_id, sacfDisableIfZeroSheet);
	AddClusterAssoc(CTL_PLIST_FLAGS, 0, PLISTF_USECOMMON);
	AddClusterAssoc(CTL_PLIST_FLAGS, 1, PLISTF_CALCREST);
	SetClusterData(CTL_PLIST_FLAGS, Data.Flags);
	AddClusterAssoc(CTL_PLIST_IGNZEROQUOT, 0, PLISTF_IGNZEROQUOT);
	SetClusterData(CTL_PLIST_IGNZEROQUOT, Data.Flags);
	{
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(ctlgroupGoodsGrp, &gf_rec);
		AddClusterAssoc(CTL_PLIST_LINEFLAGS, 0, PLISTF_EXCLGGRP);
		AddClusterAssoc(CTL_PLIST_LINEFLAGS, 1, PLISTF_PRESENTONLY);
		SetClusterData(CTL_PLIST_LINEFLAGS, Data.Flags);
	}
	SetRealRangeInput(this, CTL_PLIST_PRICERNG, &Data.PriceRange, 2);
	setupList();
	return ok;
}

int PListFiltDialog::getDTS(PriceListFilt * pFilt)
{
	int    ok = 1;
	ushort v;
	PPID   prev_user  = Data.UserID;
	PPID   prev_ar    = Data.ArticleID;
	PPID   prev_loc   = Data.LocID;
	PPID   prev_quot  = Data.QuotKindID;
	getCtrlData(CTL_PLIST_DATE, &Data.Dt);
	getCtrlData(CTLSEL_PLIST_LOC, &Data.LocID);
	getCtrlData(CTLSEL_PLIST_QUOTK, &Data.QuotKindID);
	getCtrlData(CTL_PLIST_KIND, &(v = 0));
	SETFLAG(Data.Flags, PLISTF_BYQUOT, v);
	if(!(Data.Flags & PLISTF_BYQUOT))
		Data.QuotKindID = -1;
	else if(Data.QuotKindID < 0)
		Data.QuotKindID = 0;
	getCtrlString(CTL_PLIST_MEMO, Data.Memo);
	getCtrlData(CTLSEL_PLIST_SUBST, &Data.Sgg);
	GetClusterData(CTL_PLIST_FLAGS, &Data.Flags);
	GetClusterData(CTL_PLIST_IGNZEROQUOT, &Data.Flags);
	Data.UserID = (Data.Flags & PLISTF_USECOMMON) ? -1 : LConfig.UserID;
	getCtrlData(CTLSEL_PLIST_ARTICLE, &Data.ArticleID);
	{
		GoodsFiltCtrlGroup::Rec gf_rec;
		GetClusterData(CTL_PLIST_LINEFLAGS, &Data.Flags);
		getGroupData(ctlgroupGoodsGrp, &gf_rec);
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
	}
	if(Data.UserID != prev_user || Data.ArticleID != prev_ar || Data.LocID != prev_loc || Data.QuotKindID != prev_quot) {
		PPID   plist_id = 0;
		P_PLV->SearchListByFilt(&Data, &plist_id, 0);
		Data.PListID = plist_id;
	}
	GetRealRangeInput(this, CTL_PLIST_PRICERNG, &Data.PriceRange);
	ASSIGN_PTR(pFilt, Data);
	return ok;
}

int PPViewPriceList::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1, valid_data = 0;
	PListFiltDialog * dlg = 0;
	PriceListFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = static_cast<PriceListFilt *>(pBaseFilt);
	THROW(CheckDialogPtr(&(dlg = new PListFiltDialog(this))));
	THROW(dlg->setDTS(p_filt));
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(p_filt)) {
			//
			// Изменяем либо создаем новый прайс-лист по заданным фильтром условиям
			//
			PriceListTbl::Rec plist_rec;
			MEMSZERO(plist_rec);
			plist_rec.ID      = p_filt->PListID;
			plist_rec.SupplID = p_filt->ArticleID;
			plist_rec.UserID  = p_filt->UserID;
			plist_rec.LocID   = p_filt->LocID;
			plist_rec.Dt      = NZOR(p_filt->Dt, LConfig.OperDate);
			plist_rec.QuotKindID = p_filt->QuotKindID;
			plist_rec.GoodsGrpID = p_filt->GoodsGrpID;
			plist_rec.Flags = (p_filt->Flags & (PLISTF_BYQUOT | PLISTF_EXCLGGRP | PLISTF_IGNZEROQUOT));
			STRNSCPY(plist_rec.Memo, p_filt->Memo);
			if(plist_rec.ID) {
				if(!Tbl.Update(plist_rec.ID, &plist_rec, 1))
					ok = 0;
			}
			else {
				if(!Tbl.Add(&plist_rec.ID, &plist_rec, 1))
					ok = 0;
				else
					p_filt->PListID = plist_rec.ID;
			}
			valid_data = 1;
			if(ok)
				if(!PPRef->PutPropArray(PPOBJ_PRICELIST, p_filt->PListID, PLPRP_GDSGRPLIST, &p_filt->GrpIDList.Get(), 0))
					ok = PPErrorZ();
				else
					ok = 1;
			else
				PPError();
		}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// PLineDialog
//
class PLineDialog : public TDialog {
	DECL_DIALOG_DATA(PriceLineTbl::Rec);
public:
	enum {
		ctlgroupGoods = 1
	};
	explicit PLineDialog(PPViewPriceList * pV) : TDialog(DLG_PLINE), P_PLV(pV)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_PLINE_GGRP, CTLSEL_PLINE_GOODS));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		GoodsCtrlGroup::Rec gcgr(0, Data.GoodsID);
		setGroupData(ctlgroupGoods, &gcgr);
		if(Data.QuotKindID >= 0)
			SetupPPObjCombo(this, CTLSEL_PLINE_QUOTKIND, PPOBJ_QUOTKIND, Data.QuotKindID, 0, 0);
		else
			disableCtrl(CTLSEL_PLINE_QUOTKIND, 1);
		setCtrlData(CTL_PLINE_UPP, &Data.UnitPerPack);
		setCtrlData(CTL_PLINE_PRICE, &Data.Price);
		setCtrlData(CTL_PLINE_AP1, &Data.AddPrice1);
		setCtrlData(CTL_PLINE_AP2, &Data.AddPrice2);
		setCtrlData(CTL_PLINE_AP3, &Data.AddPrice3);
		setCtrlData(CTL_PLINE_MEMO, Data.Memo);
		setCtrlData(CTL_PLINE_GOODSCODE, &Data.GoodsCode);
		setCtrlData(CTL_PLINE_PLABELGNAME, Data.Name);
		if(Data.LineNo == 0 && Data.GoodsID)
			replyGoodsSelection(1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GoodsCtrlGroup::Rec gcgr;
		getGroupData(ctlgroupGoods, &gcgr);
		Data.GoodsID = gcgr.GoodsID;
		if(Data.GoodsID == 0)
			ok = (PPError(PPERR_GOODSNEEDED, 0), 0);
		else {
			if(Data.QuotKindID >= 0)
				getCtrlData(CTLSEL_PLINE_QUOTKIND, &Data.QuotKindID);
			getCtrlData(CTL_PLINE_UPP, &Data.UnitPerPack);
			getCtrlData(CTL_PLINE_PRICE, &Data.Price);
			getCtrlData(CTL_PLINE_AP1, &Data.AddPrice1);
			getCtrlData(CTL_PLINE_AP2, &Data.AddPrice2);
			getCtrlData(CTL_PLINE_AP3, &Data.AddPrice3);
			getCtrlData(CTL_PLINE_MEMO, Data.Memo);
			getCtrlData(CTL_PLINE_PLABELGNAME, Data.Name);
			ASSIGN_PTR(pData, Data);
		}
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    replyGoodsSelection(int enforce);
	PPViewPriceList * P_PLV;
};

int PLineDialog::replyGoodsSelection(int enforce)
{
	PPID   goods_id = 0;
	getCtrlData(CTLSEL_PLINE_GOODS, &goods_id);
	if(enforce || goods_id != Data.GoodsID) {
		const  PPID loc_id = static_cast<const PriceListFilt *>(P_PLV->GetBaseFilt())->LocID;
		double price = 0.0;
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
			double uppack = 0.0;
			ReceiptTbl::Rec lot_rec;
			PPViewPriceList::RecalcParamBlock rpb;
			Data.GoodsID = goods_id;
			int    r = ::GetCurGoodsPrice(goods_id, loc_id, GPRET_MOSTRECENT, &price, &lot_rec);
			int    is_present = oneof2(r, GPRET_PRESENT, GPRET_OTHERLOC);
			P_PLV->InitRPB(&rpb, &goods_rec, BIN(lot_rec.Flags & LOTF_PRICEWOTAXES), R5(lot_rec.Cost), price, lot_rec.Expiry);
			if(r > 0)
				uppack = lot_rec.UnitPerPack;
			const PPID qk_id = static_cast<const PriceListFilt *>(P_PLV->GetBaseFilt())->QuotKindID;
			if(qk_id >= 0) {
				price = rpb.BasePrice;
				if(P_PLV->GetPriceByQuot(&rpb, qk_id, &price) > 0)
					rpb.BasePrice = price;
			}
			setCtrlData(CTL_PLINE_UPP,   &uppack);
			setCtrlData(CTL_PLINE_PRICE, &rpb.BasePrice);
			setCtrlData(CTL_PLINE_AP1,   &rpb.AddPrices[0]);
			setCtrlData(CTL_PLINE_AP2,   &rpb.AddPrices[1]);
			setCtrlData(CTL_PLINE_AP3,   &rpb.AddPrices[2]);
			Data.Expiry = lot_rec.Expiry;
		}
	}
	return 1;
}

IMPL_HANDLE_EVENT(PLineDialog)
{
	//PPID   lot;
	PPID   goods_id, loc_id;
	//ReceiptTbl::Rec rec;
	TDialog::handleEvent(event);
	if(event.isCmd(cmLot)) {
		loc_id = static_cast<const PriceListFilt *>(P_PLV->GetBaseFilt())->LocID;
		goods_id = getCtrlLong(CTLSEL_PLINE_GOODS);
		PPObjBill::SelectLotParam slp(goods_id, loc_id, 0, PPObjBill::SelectLotParam::fFillLotRec);
		//if(SelectLot(loc_id, goods_id, 0, &lot, &rec) > 0) {
		if(BillObj->SelectLot2(slp) > 0) {
			setCtrlReal(CTL_PLINE_PRICE, R5(slp.RetLotRec.Price));
			setCtrlReal(CTL_PLINE_UPP,   NZOR(slp.RetLotRec.UnitPerPack, 1));
		}
	}
	else if(event.isCmd(cmQuot)) {
		goods_id = getCtrlLong(CTLSEL_PLINE_GOODS);
		if(goods_id > 0) {
			PPObjGoods gobj;
			loc_id = static_cast<const PriceListFilt *>(P_PLV->GetBaseFilt())->LocID;
			gobj.EditQuotations(goods_id, loc_id, -1L/*@curID*/, 0, PPQuot::clsGeneral);
		}
	}
	else if(event.isCbSelected(CTLSEL_PLINE_GOODS))
		replyGoodsSelection(0);
	/*
	else if(event.isCbSelection(CTLSEL_PLINE_GGRP)) {
		getCtrlData(CTLSEL_PLINE_GGRP, &goods_id);
		SetupPPObjCombo(this, CTLSEL_PLINE_GOODS, PPOBJ_GOODS, 0, OLW_CANINSERT | OLW_CANEDIT, goods_id);
		price = 0;
		setCtrlData(CTL_PLINE_PRICE, &price);
		setCtrlData(CTL_PLINE_UPP,   &price);
	}
	*/
	else
		return;
	clearEvent(event);
}

int PPViewPriceList::AddLine(PriceLineIdent * pIdent)
{
	int    ok = -1, valid_data = 0;
	PriceLineTbl::Rec rec;
	PLineDialog * dlg = 0;
	Gsl.Clear(); // AHTOXA
	THROW(CheckDialogPtr(&(dlg = new PLineDialog(this))));
	// @v10.7.9 @ctr MEMSZERO(rec);
	rec.ListID  = pIdent->PListID;
	rec.GoodsID = pIdent->GoodsID;
	rec.QuotKindID = Filt.QuotKindID;
	dlg->setDTS(&rec);
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(&rec)) {
			valid_data = 1;
			rec.Rest = GetRest(rec.GoodsID);
			/* @v9.2.2 if(!Filt.Sgg && Filt.GoodsGrpID && GObj.IsAltGroup(Filt.GoodsGrpID) > 0)
				if(PPRef->Assc.Search(PPASS_ALTGOODSGRP, Filt.GoodsGrpID, rec.GoodsID) > 0)
					rec.GoodsCode = PPRef->Assc.data.InnerNum; */
			if(!Filt.Sgg)
				GObj.P_Tbl->GetGoodsCodeInAltGrp(rec.GoodsID, Filt.GoodsGrpID, &rec.GoodsCode); // @v9.2.2
			THROW(SubstGoods(rec.GoodsID, &rec.GoodsID, rec.Name, sizeof(rec.Name)));
			if(Tbl.AddLine(rec.ListID, &rec, pIdent, BIN(Filt.Sgg), 1)) {
				UpdateTempTbl(pIdent);
				ok = 1;
			}
			else
				ok = PPErrorZ();
		}
	CATCHZOKPPERR
	delete dlg;
	Gsl.Clear(); // AHTOXA
	return ok;
}

int PPViewPriceList::EditLine(PriceLineIdent * pIdent)
{
	int    ok = -1, valid_data = 0;
	PriceLineTbl::Rec rec;
	PLineDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new PLineDialog(this)))) {
		if(SearchLine(pIdent, &rec) > 0) {
			pIdent->LineNo = rec.LineNo;
			dlg->setDTS(&rec);
			dlg->disableCtrls(1, CTLSEL_PLINE_GGRP, CTLSEL_PLINE_GOODS, CTLSEL_PLINE_QUOTKIND, 0);
			while(!valid_data && ExecView(dlg) == cmOK)
				if(dlg->getDTS(&rec)) {
					valid_data = 1;
					rec.Rest = GetRest(rec.GoodsID);
					if(!Tbl.UpdateLine(pIdent, &rec, BIN(Filt.Sgg), 1))
						ok = PPErrorZ();
					else {
						UpdateTempTbl(pIdent);
						ok = 1;
					}
				}
		}
		delete dlg;
	}
	return ok;
}

class PPPriceListExporter {
public:
	PPPriceListExporter() : P_IE(0)
	{
	}
	~PPPriceListExporter()
	{
		ZDELETE(P_IE);
	}
	int    Init(const PPPriceListImpExpParam * pParam);
	int    Export(const PriceListViewItem * pItem);
	int    DistributeFile()
	{
		CALLPTRMEMB(P_IE, CloseFile());
		return Param.DistributeFile(0);
	}

	PPPriceListImpExpParam Param;
private:
	PPImpExp * P_IE;
	PPObjGoods GObj;
};

int PPPriceListExporter::Init(const PPPriceListImpExpParam * pParam)
{
	int    ok = 1;
	RVALUEPTR(Param, pParam);
	if(!pParam) {
		THROW(LoadSdRecord(PPREC_PRICELIST, &Param.InrRec));
		ok = SelectPriceListImportCfg(&Param, 1);
	}
	if(ok > 0) {
		THROW_MEM(P_IE = new PPImpExp(&Param, 0));
		THROW(P_IE->OpenFileForWriting(0, 1));
	}
	CATCHZOK
	return ok;
}

int PPPriceListExporter::Export(const PriceListViewItem * pItem)
{
	int    ok = 1;
	SString temp_buf;
	Goods2Tbl::Rec goods_rec;
	Sdr_PriceList sdr;
	// @v10.7.9 @ctr MEMSZERO(sdr);
	THROW_INVARG(pItem && P_IE);
	sdr.GoodsID = pItem->GoodsID;
	sdr.AltGrpPLU = pItem->GoodsCode;
	sdr.Price = pItem->Price;
	sdr.AddPrice1 = pItem->AddPrice1;
	sdr.AddPrice2 = pItem->AddPrice2;
	sdr.AddPrice3 = pItem->AddPrice3;
	sdr.UnitPerPack = pItem->UnitsPerPack;
	sdr.Rest = pItem->Rest;
	sdr.Expiry = pItem->Expiry;
	STRNSCPY(sdr.Memo, pItem->Memo_);
	if(GObj.Fetch(pItem->GoodsID, &goods_rec) > 0) {
		STRNSCPY(sdr.GoodsName, goods_rec.Name);
		GObj.GetSingleBarcode(goods_rec.ID, temp_buf);
		STRNSCPY(sdr.Barcode, temp_buf);
		{
			PPGoodsTaxEntry tax_entry;
			if(GObj.FetchTax(goods_rec.ID, getcurdate_(), 0, &tax_entry) > 0) {
                sdr.VatRate = tax_entry.GetVatRate();
			}
		}
		if(goods_rec.ManufID) {
            GetPersonName(goods_rec.ManufID, temp_buf);
            STRNSCPY(sdr.ManufName, temp_buf);
		}
		if(goods_rec.BrandID) {
			GetObjectName(PPOBJ_BRAND, goods_rec.BrandID, temp_buf);
			STRNSCPY(sdr.BrandName, temp_buf);
		}
		{
			GoodsStockExt gse;
			if(GObj.GetStockExt(goods_rec.ID, &gse, 1) > 0) {
				if(gse.MinShippmQtty > 0.0) {
					sdr.MinShippmQtty = gse.MinShippmQtty;
					if(gse.GseFlags & gse.fMultMinShipm)
						sdr.ShippmMult = gse.MinShippmQtty;
				}
			}
		}
	}
	Param.InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr);
	THROW(P_IE->AppendRecord(&sdr, sizeof(sdr)));
	CATCHZOK
	return ok;
}

int PPViewPriceList::SendPList()
{
	int    ok = 1;
	PriceListConfig plist_cfg;
	ReadPriceListConfig(&plist_cfg);
	PPWaitStart();
	if(!(plist_cfg.Flags & PLISTF_SENDINXML)) {
		PriceListViewItem item;
		char   sdt[12];
		SString path;
		SString msg_buf;
		SString temp_buf;
		PPObjPerson pobj;
		PPObjUnit   uobj;
		PPObjQCert	qobj;
		PPIniFile * p_ini_file = 0;
		DbfTable  * plst_tbl = 0;
		PPLoadText(PPTXT_PLISTTOALBATROS, msg_buf);
		PPWaitMsg(msg_buf);
		THROW_MEM(p_ini_file = new PPIniFile(PPGetFilePathS(PPPATH_OUT, PPFILNAM_PLIST_CNF, path), TRUE));
		p_ini_file->Append(PPINISECT_PRICELIST, PPINIPARAM_PL_USEPACKAGE, (LConfig.Flags & CFGFLG_USEPACKAGE) ? "yes" : "no", 1);
		datefmt(&LConfig.OperDate, MKSFMT(0, DATF_DMY | DATF_CENTURY), sdt);
		p_ini_file->Append(PPINISECT_PRICELIST, PPINIPARAM_PL_OPERDATE, sdt, 1);
		{
			temp_buf = plist_cfg.OrgName;
			if(!temp_buf.NotEmptyS())
				GetMainOrgName(temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(plist_cfg.OrgName, sizeof(plist_cfg.OrgName));
		}
		p_ini_file->Append(PPINISECT_PRICELIST, PPINIPARAM_PL_ORGNAME, plist_cfg.OrgName, 1);
		if(*strip(plist_cfg.Business) != 0) {
			(temp_buf = plist_cfg.Business).Transf(CTRANSF_INNER_TO_OUTER);
			p_ini_file->Append(PPINISECT_PRICELIST, PPINIPARAM_PL_BUSINESS, temp_buf, 1);
		}
		if(*strip(plist_cfg.PublisherMail) != 0) {
			(temp_buf = plist_cfg.PublisherMail).Transf(CTRANSF_INNER_TO_OUTER);
			p_ini_file->Append(PPINISECT_PRICELIST, PPINIPARAM_PL_PUBMAIL, temp_buf, 1);
		}
		ZDELETE(p_ini_file);

		THROW(plst_tbl = CreateDbfTable(DBFS_Exp_Price_List, PPGetFilePathS(PPPATH_OUT, PPFILNAM_PLIST_DBF, path), TRUE));
		plst_tbl->bottom();
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			PPUnit unit_rec;
			DbfRecord plst_rec(plst_tbl);
			plst_rec.empty();                   // upp
			(temp_buf = item.GoodsGrpName_).Transf(CTRANSF_INNER_TO_OUTER);
			plst_rec.put(1, temp_buf);
			(temp_buf = item.GoodsName_).Transf(CTRANSF_INNER_TO_OUTER);
			plst_rec.put(2, temp_buf);
			plst_rec.put(3, item.Price);        //
			plst_rec.put(4, item.GoodsGrpID);   // "GRPID\0",   "N", 4, 0,
			plst_rec.put(5, item.GoodsID);      // "GOODSID\0", "N", 6, 0,
			plst_rec.put(6, item.UnitID);       // "UNITID\0",  "N", 6, 0,
			plst_rec.put(7, item.UnitsPerPack); // "UNITS\0",   "N", 8, 0,
			uobj.Fetch(item.UnitID, &unit_rec);
			(temp_buf = unit_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
			plst_rec.put(8, temp_buf);              // UnitName
			plst_rec.put(9, item.QuotKindID);   // "QUOTID\0", "N", 6, 0,
			plst_rec.put(10, item.ManufID);     // "MANUFID\0", "N", 6, 0,
			GObj.GetSingleBarcode(item.GoodsID, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
			plst_rec.put(12, temp_buf);             // "BARCODE\0", "C", 12, 0
			(temp_buf = item.Memo_).Transf(CTRANSF_INNER_TO_OUTER);
			plst_rec.put(13, temp_buf);             // "PMEMO\0", "C", 128, 0
			THROW_PP(plst_tbl->appendRec(&plst_rec), PPERR_DBFWRFAULT);
			PPWaitPercent(GetCounter(), msg_buf);
		}
		ZDELETE(plst_tbl);
	}
	else
		THROW(SendPListInXmlFormat());
	THROW(Export());
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewPriceList::ConvertBasketToLines()
{
	int    ok = -1, r;
	PPObjGoodsBasket gb_obj;
	PPBasketCombine basket;
	THROW(r = gb_obj.SelectBasket(basket));
	if(r > 0) {
		ILTI * p_item;
		ReceiptTbl::Rec rr;
		PriceListTbl::Rec plist_rec;
		ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
		RecalcParamBlock rpb;
		PPWaitStart();
		PPTransaction tra(1);
		THROW(tra);
		THROW(Tbl.Search(Filt.PListID, &plist_rec) > 0);
		for(uint i = 0; basket.Pack.Lots.enumItems(&i, (void **)&p_item);) {
			PPID   quot_kind_id;
			double price = 0.0;
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(p_item->GoodsID, &goods_rec) > 0) {
				//
				// Цену определяем по самому последнему лоту,
				//
				::GetCurGoodsPrice(goods_rec.ID, Filt.LocID, GPRET_MOSTRECENT, &price, &rr);
				//
				// Теперь снова вызываем ::GetCurGoodsPrice с флагом GPRET_PRESENT
				// для определения того, есть ли указанный товар на остатке
				//
				::GetCurGoodsPrice(goods_rec.ID, Filt.LocID, 0, 0, 0);
				int    is_present = oneof2(r, GPRET_PRESENT, GPRET_OTHERLOC);
				if(goods_rec.TaxGrpID == 0 && GObj.Search(goods_rec.ParentID) > 0)
					goods_rec.TaxGrpID = GObj.P_Tbl->data.TaxGrpID;
				THROW(InitRPB(&rpb, &goods_rec, BIN(rr.Flags & LOTF_PRICEWOTAXES), R5(rr.Cost), price, rr.Expiry));
				quot_kind_id = (plist_rec.Flags & PLISTF_BYQUOT) ? 0L : -1L;
				THROW(SetGoodsPrice(&rpb, quot_kind_id, rr.UnitPerPack, rpb.BasePrice, is_present, 0));
				if(plist_rec.QuotKindID >= 0)
					THROW(SetByQuot(&rpb, rr.UnitPerPack, is_present, 0));
				ok = 1;
			}
		}
		THROW(tra.Commit());
		if(ok > 0)
			UpdateTempTbl(0);
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewPriceList::ConvertLinesToBasket()
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	param.SelPrice = 1;
	THROW(r = GetBasketByDialog(&param, "PriceList", DLG_GBDATAPRICE));
	if(r > 0) {
		PriceListViewItem item;
		PPWaitStart();
		for(InitIteration(PPViewPriceList::OrdByGoodsName); NextIteration(&item) > 0;) {
			ILTI   i_i;
			ReceiptTbl::Rec lot_rec;
			// @v10.7.0 @ctr MEMSZERO(lot_rec);
			THROW(::GetCurGoodsPrice(item.GoodsID, Filt.LocID, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
			i_i.GoodsID     = item.GoodsID;
			i_i.UnitPerPack = item.UnitsPerPack;
			i_i.Cost        = R5(lot_rec.Cost);
			if(param.SelPrice == 1)
				i_i.Price = item.Price;
			else if(param.SelPrice == 2)
				i_i.Price = item.AddPrice1;
			else if(param.SelPrice == 3)
				i_i.Price = item.AddPrice2;
			else if(param.SelPrice == 4)
				i_i.Price = item.AddPrice3;
			else
				i_i.Price = item.Price;
			SETIFZ(i_i.Price, item.Price);
			i_i.CurPrice = 0;
			i_i.Flags    = 0;
			i_i.Suppl    = lot_rec.SupplID;
			i_i.QCert    = NZOR(item.QCertID, lot_rec.QCertID);
			i_i.Expiry   = NZOR(item.Expiry,  lot_rec.Expiry);
			i_i.Quantity = fabs(lot_rec.Quantity);
			i_i.Rest     = i_i.Quantity;
			THROW(param.Pack.AddItem(&i_i, 0, param.SelReplace));
		}
		PPWaitStop();
		THROW(GoodsBasketDialog(param, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

static int FASTCALL WriteHelper(FILE * pOut, const char * pFieldName, char * pFieldValue, int * pNeedComma)
{
	int    ok = 0;
	if(pOut && pFieldName) {
		if(!pFieldValue || *strip(pFieldValue) != 0) {
			if(pNeedComma && *pNeedComma)
				fprintf(pOut, ",");
			fprintf(pOut, "%s", pFieldName);
			ASSIGN_PTR(pNeedComma, 1);
		}
		ok = 1;
	}
	return ok;
}

static char * FASTCALL XmlConvertStr(const char * pStr, int cvtOem, char * pBuf, size_t bufLen)
{
	SString temp_buf(pStr);
	if(cvtOem)
		temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
	XMLReplaceSpecSymb(temp_buf, 0);
	temp_buf.CopyTo(pBuf, bufLen);
	return pBuf;
}

static int FASTCALL XmlWriteData(FILE * pStream, const char * pField, const char * pData, int skipEmpty)
{
	char val_buf[128], par_buf[128], *p;
	strip(STRNSCPY(val_buf, pData));
	if(skipEmpty && val_buf[0] == 0)
		return -1;
	p = par_buf;
	*p++ = '<';
	p = stpcpy(p, pField);
	*p++ = '>';
	*p = 0;
	fputs(par_buf, pStream);
	fputs(pData, pStream);

	p = par_buf;
	*p++ = '<';
	*p++ = '/';
	p = stpcpy(p, pField);
	*p++ = '>';
	*p = 0;
	fputs(par_buf, pStream);
	fputc('\n', pStream);
	return 1;
}

static int FASTCALL XmlWriteData(FILE * pStream, const char * pField, long data, int skipZero)
{
	char   str_val[64];
	return (skipZero && data == 0) ? -1 : XmlWriteData(pStream, pField, ltoa(data, str_val, 10), 0);
}

static int FASTCALL XmlWriteData(FILE * pStream, const char * pField, double data, int skipZero)
{
	char   str_val[64];
	if(skipZero && data == 0)
		return -1;
	sprintf(str_val, "%lf", data);
	return XmlWriteData(pStream, pField, str_val, 0);
}

static int FASTCALL XmlWriteTag(FILE * pStream, const char * pTag, int start, int newLine)
{
	char par_buf[128], *p = par_buf;
	*p++ = '<';
	if(!start)
		*p++ = '/';
	p = stpcpy(p, pTag);
	*p++ = '>';
	if(newLine)
		*p++ = '\n';
	*p = 0;
	fputs(par_buf, pStream);
	return 1;
}

// WriteHelper(p_xml_file, "<!ELEMENT Name          (#PCDATA)>\n", s_i.Name         , 0);
static SString & FASTCALL XmlDeclElement(const char * pFieldName, SString & rBuf)
{
	rBuf.Z().CatChar('<').CatChar('!').Cat("ELEMENT").Space().Cat(pFieldName).Space().
		CatChar('(').CatChar('#').Cat("PCDATA").CatChar(')').CatChar('>').CR();
	return rBuf;
}

int PPViewPriceList::SendPListInXmlFormat()
{
	struct SellerInfo {
		char Name[512];
		char FullName[512];
		char Business[512];
		char SellerName[512];
		char Country[512];
		char City[512];
		char Address[512];
		char Phone[128];
		char Phone2[128];
		char Email[44];
		char EmailAlbatros[44];
		char INN[128];
		char OperDate[128];
		long Flags;
	};
	int    ok = 1, need_comma = 0;
	SString msg_buf;
	SString path;
	FILE * p_xml_file = 0;
	PPIniFile ini_file;
	PriceListViewItem item;
	PPIDArray g_id_ary;
	PPObjPerson pobj;
	PPObjUnit   uobj;
	PPObjQCert	qobj;
	PriceListConfig plist_cfg;

	int	   pos = -1, pos2 = -1;
	uint   i = 0;
	SString temp_buf;
	PPID * p_item = 0, id = 0;
	PPCommConfig cfg;
	PersonTbl::Rec psn_rec;
	SellerInfo s_i;
	PPPersonPacket psn_pack;
	PPObjWorld w_obj;
	WorldTbl::Rec w_rec;
	MEMSZERO(s_i);
	// @v10.7.9 @ctr MEMSZERO(psn_rec);
	MEMSZERO(cfg);
	THROW(ini_file.IsValid());
	ReadPriceListConfig(&plist_cfg);
	PPLoadText(PPTXT_PLISTTOALBATROS, msg_buf);
	if(*strip(plist_cfg.OrgName) == 0) {
		SString main_org_name;
		GetMainOrgName(main_org_name);
		main_org_name.CopyTo(plist_cfg.OrgName, sizeof(plist_cfg.OrgName));
	}
	XmlConvertStr(plist_cfg.OrgName, 1, s_i.Name, sizeof(s_i.Name));
	STRNSCPY(s_i.FullName, s_i.Name);
	if(*strip(plist_cfg.Business) != 0)
		XmlConvertStr(plist_cfg.Business, 1, s_i.Business, sizeof(s_i.Business));
	ini_file.Get(PPINISECT_SYSTEM, PPINIPARAM_ALBATROS_SELLERNAME, temp_buf);
	XMLReplaceSpecSymb(temp_buf, 0);
	temp_buf.CopyTo(s_i.SellerName, sizeof(s_i.SellerName));
	THROW(GetCommConfig(&cfg));
	if(cfg.MainOrgID > 0 && pobj.P_Tbl->Search(cfg.MainOrgID) > 0)
		id = cfg.MainOrgID;
	else if(pobj.P_Tbl->SearchMainOrg(&psn_rec) > 0)
		id = psn_rec.ID;
	if(id > 0) {
		THROW(pobj.GetPacket(id, &psn_pack, 0) > 0);
		if(w_obj.Fetch(psn_pack.Loc.CityID, &w_rec) > 0) {
			XmlConvertStr(w_rec.Name, 1, s_i.City, sizeof(s_i.City));
			if(w_obj.GetCountryByChild(w_rec.ID, &w_rec) > 0)
				XmlConvertStr(w_rec.Name, 1, s_i.Country, sizeof(s_i.Country));
		}
		if(psn_pack.GetRAddress(0, temp_buf) <= 0)
			psn_pack.GetAddress(0, temp_buf);
		temp_buf.CopyTo(s_i.Address, sizeof(s_i.Address));
		XmlConvertStr(s_i.Address, 1, s_i.Address, sizeof(s_i.Address));
		psn_pack.ELA.GetItem(PPELK_WORKPHONE, temp_buf);
		XMLReplaceSpecSymb(temp_buf, 0);
		temp_buf.CopyTo(s_i.Phone, sizeof(s_i.Phone));
		psn_pack.ELA.GetItem(PPELK_ALTPHONE,  temp_buf) > 0 || psn_pack.ELA.GetItem(PPELK_HOMEPHONE, temp_buf) > 0;
		XMLReplaceSpecSymb(temp_buf, 0);
		temp_buf.CopyTo(s_i.Phone2, sizeof(s_i.Phone2));
		if(s_i.Phone[0] == 0) {
			STRNSCPY(s_i.Phone, s_i.Phone2);
			s_i.Phone2[0] = 0;
		}
		psn_pack.GetRegNumber(PPREGT_TPID, temp_buf);
		XMLReplaceSpecSymb(temp_buf, 0);
		temp_buf.CopyTo(s_i.INN, sizeof(s_i.INN));
	}
	if(*strip(plist_cfg.PublisherMail) != 0) {
		XmlConvertStr(plist_cfg.PublisherMail, 1, s_i.Email, sizeof(s_i.Email));
		STRNSCPY(s_i.EmailAlbatros, s_i.Email);
	}
	temp_buf.Z().Cat(LConfig.OperDate, MKSFMT(0, DATF_DMY | DATF_CENTURY));
	XMLReplaceSpecSymb(temp_buf, 0);
	temp_buf.CopyTo(s_i.OperDate, sizeof(s_i.OperDate));

	SETFLAG(s_i.Flags, CFGFLG_USEPACKAGE, LConfig.Flags & CFGFLG_USEPACKAGE);
	PPGetFilePath(PPPATH_OUT, PPFILNAM_PLIST_XML, path);
	PPSetAddedMsgString(path);
	THROW_PP((p_xml_file = fopen(path, "w")) != NULL, PPERR_CANTOPENFILE);
	fprintf(p_xml_file, "<?xml version=\"1.0\" encoding=\"Windows-1251\" ?>\n");
	fprintf(p_xml_file, "<!DOCTYPE AlbData [\n");
	XMLWriteSpecSymbEntities(p_xml_file);
	fprintf(p_xml_file, "<!ELEMENT AlbData (SellerInfo, Goods, Group)>\n");
	fprintf(p_xml_file, "<!ELEMENT SellerInfo (");
	WriteHelper(p_xml_file, "Name"         , s_i.Name         , &need_comma);
	WriteHelper(p_xml_file, "FullName"     , s_i.FullName     , &need_comma);
	WriteHelper(p_xml_file, "Business"     , s_i.Business     , &need_comma);
	WriteHelper(p_xml_file, "SellerName"   , s_i.SellerName   , &need_comma);
	WriteHelper(p_xml_file, "LoginName"    , 0                , &need_comma);
	WriteHelper(p_xml_file, "Password"     , 0                , &need_comma);
	WriteHelper(p_xml_file, "Country"      , s_i.Country      , &need_comma);
	WriteHelper(p_xml_file, "City"         , s_i.City         , &need_comma);
	WriteHelper(p_xml_file, "Address"      , s_i.Address      , &need_comma);
	WriteHelper(p_xml_file, "Phone"        , s_i.Phone        , &need_comma);
	WriteHelper(p_xml_file, "Phone2"       , s_i.Phone2       , &need_comma);
	WriteHelper(p_xml_file, "Email"        , s_i.Email        , &need_comma);
	WriteHelper(p_xml_file, "EmailAlbatros", s_i.EmailAlbatros, &need_comma);
	WriteHelper(p_xml_file, "INN"          , s_i.INN          , &need_comma);
	WriteHelper(p_xml_file, "OperDate"     , s_i.OperDate     , &need_comma);
	WriteHelper(p_xml_file, "Flags"        , 0                , &need_comma);
	fprintf(p_xml_file, ")>\n");

	{
		//SString line_buf;
		//line_buf.CatXmlElem("Name", 0, 0).CR();
		WriteHelper(p_xml_file, XmlDeclElement("Name",       temp_buf), s_i.Name, 0);
		WriteHelper(p_xml_file, XmlDeclElement("FullName",   temp_buf), s_i.FullName, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Business",   temp_buf), s_i.Business, 0);
		WriteHelper(p_xml_file, XmlDeclElement("SellerName", temp_buf), s_i.SellerName, 0);
		WriteHelper(p_xml_file, XmlDeclElement("LoginName",  temp_buf), 0, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Password",   temp_buf), 0, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Country",    temp_buf), s_i.Country, 0);
		WriteHelper(p_xml_file, XmlDeclElement("City",       temp_buf), s_i.City, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Address",    temp_buf), s_i.Address, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Phone",      temp_buf), s_i.Phone, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Phone2",     temp_buf), s_i.Phone2, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Email",      temp_buf), s_i.Email, 0);
		WriteHelper(p_xml_file, XmlDeclElement("EmailAlbatros", temp_buf), s_i.EmailAlbatros, 0);
		WriteHelper(p_xml_file, XmlDeclElement("INN",        temp_buf), s_i.INN, 0);
		WriteHelper(p_xml_file, XmlDeclElement("OperDate",   temp_buf), s_i.OperDate, 0);
		WriteHelper(p_xml_file, XmlDeclElement("Flags",      temp_buf), 0, 0);
	}

	need_comma = 0;
	fprintf(p_xml_file, "<!ELEMENT Goods (");
	WriteHelper(p_xml_file, "GoodsID"      , 0, &need_comma);
	WriteHelper(p_xml_file, "GroupID"      , 0, &need_comma);
	WriteHelper(p_xml_file, "GoodsName"    , 0, &need_comma);
	WriteHelper(p_xml_file, "Barcode"      , 0, &need_comma);
	WriteHelper(p_xml_file, "UnitName"     , 0, &need_comma);
	WriteHelper(p_xml_file, "UnitsPerPack" , 0, &need_comma);
	WriteHelper(p_xml_file, "Price"        , 0, &need_comma);
	WriteHelper(p_xml_file, "Price2"       , 0, &need_comma);
	/* Not included for present {
	WriteHelper(p_xml_file, "Price3"       , 0, &need_comma);
	WriteHelper(p_xml_file, "Rest"         , 0, &need_comma);
	} Not included for present*/
	WriteHelper(p_xml_file, "Description"  , 0, &need_comma);
	fprintf(p_xml_file, ")>\n");
	fprintf(p_xml_file, XmlDeclElement("GoodsID", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("GroupID", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("GoodsName", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("Barcode", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("UnitName", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("UnitsPerPack", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("Price", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("Price2", temp_buf).cptr());
	/* Not included for present {
	fprintf(p_xml_file, XmlDeclElement("Price3", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("Rest", temp_buf).cptr());
	} Not included for present*/
	fprintf(p_xml_file, XmlDeclElement("Description", temp_buf).cptr());

	need_comma = 0;
	fprintf(p_xml_file, "<!ELEMENT Group (");
	WriteHelper(p_xml_file, "ID"   , 0, &need_comma);
	WriteHelper(p_xml_file, "Name" , 0, &need_comma);
	fprintf(p_xml_file, ")>\n");
	fprintf(p_xml_file, XmlDeclElement("ID", temp_buf).cptr());
	fprintf(p_xml_file, XmlDeclElement("Name", temp_buf).cptr());

	fprintf(p_xml_file, "]>\n");
	XmlWriteTag(p_xml_file, "AlbData", 1, 1);
	XmlWriteTag(p_xml_file, "SellerInfo", 1, 1);
	XmlWriteData(p_xml_file, "Name",       s_i.Name, 1);
	XmlWriteData(p_xml_file, "FullName",   s_i.FullName, 1);
	XmlWriteData(p_xml_file, "Business",   s_i.Business, 1);
	XmlWriteData(p_xml_file, "SellerName", s_i.SellerName, 1);
	XmlWriteData(p_xml_file, "Country",    s_i.Country, 1);
	XmlWriteData(p_xml_file, "City",       s_i.City, 1);
	XmlWriteData(p_xml_file, "Address",    s_i.Address, 1);
	XmlWriteData(p_xml_file, "Phone",      s_i.Phone, 1);
	XmlWriteData(p_xml_file, "Phone2",     s_i.Phone2, 1);
	XmlWriteData(p_xml_file, "Email",      s_i.Email, 1);
	XmlWriteData(p_xml_file, "EmailAlbatros", s_i.EmailAlbatros, 1);
	XmlWriteData(p_xml_file, "INN",           s_i.INN, 1);
	XmlWriteData(p_xml_file, "OperDate",      s_i.OperDate, 1);
	XmlWriteData(p_xml_file, "Flags", s_i.Flags, 0);
	XmlWriteTag(p_xml_file, "SellerInfo", 0, 1);

	for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
		PPUnit unit_rec;
		XmlWriteTag(p_xml_file, "Goods", 1, 1);
		XmlWriteData(p_xml_file, "GoodsID", item.GoodsID, 0);
		XmlWriteData(p_xml_file, "GroupID", item.GoodsGrpID, 0);
		XMLReplaceSpecSymb((temp_buf = item.GoodsName_).Transf(CTRANSF_INNER_TO_OUTER), 0);
		XmlWriteData(p_xml_file, "GoodsName", temp_buf, 0);
		GObj.GetSingleBarcode(item.GoodsID, temp_buf);
		XMLReplaceSpecSymb(temp_buf, 0);
		XmlWriteData(p_xml_file, "Barcode", temp_buf, 0);
		uobj.Fetch(item.UnitID, &unit_rec);
		XMLReplaceSpecSymb((temp_buf = unit_rec.Name).Transf(CTRANSF_INNER_TO_OUTER), 0);
		XmlWriteData(p_xml_file, "UnitName", temp_buf, 0);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(item.UnitsPerPack).Transf(CTRANSF_INNER_TO_OUTER), 0);
		XmlWriteData(p_xml_file, "UnitsPerPack", temp_buf, 0);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(item.Price), 0);
		XmlWriteData(p_xml_file, "Price", temp_buf, 0);
		XMLReplaceSpecSymb(temp_buf.Z().Cat(item.Price * item.UnitsPerPack), 0);
		XmlWriteData(p_xml_file, "Price2", temp_buf, 0);
		XMLReplaceSpecSymb((temp_buf = item.Memo_).Transf(CTRANSF_INNER_TO_OUTER), 0);
		XmlWriteData(p_xml_file, "Description", temp_buf, 0);
		XmlWriteTag(p_xml_file, "Goods", 0, 1);
		g_id_ary.addUnique(item.GoodsGrpID);
		PPWaitPercent(GetCounter(), msg_buf);
	}
	for(i = 0; g_id_ary.enumItems(&i, (void **)&p_item);) {
		XmlWriteTag(p_xml_file, "Group", 1, 1);
		XmlWriteData(p_xml_file, "ID", *p_item, 0);
		XMLReplaceSpecSymb(GetGoodsName(*p_item, temp_buf).Transf(CTRANSF_INNER_TO_OUTER), 0);
		XmlWriteData(p_xml_file, "Name", temp_buf, 0);
		XmlWriteTag(p_xml_file, "Group", 0, 1);
		PPWaitPercent(i, g_id_ary.getCount(), msg_buf);
	}
	XmlWriteTag(p_xml_file, "AlbData", 0, 1);
	CATCHZOK
	SFile::ZClose(&p_xml_file);
	return ok;
}
/*
	goodsname, 10, Наименование товара; barcode, , Штрихкод; unit, , Единица измерения; pack, , Емкость упаковки;
	price, , Цена за единицу; packprice, , Цена за упаковку; memo, , Технические характеристики
*/
enum PriceListExportVarStr {
	plevsGoodsName = 0,
	plevsBarcode,
	plevsUnit,
	plevsPack,
	plevsPrice,
	plevsPackPrice,
	plevsMemo
};

struct PriceListExportSpecItem {
	PriceListExportVarStr Var;
	char   Title[64];
	long   Size;
};

static int ParsePriceListExportSpec(const char * pRowDef, const char * pVarStr, int defSpec, SArray * pSpec)
{
	SString row, row_name, sub;
	StringSet ss(';', pRowDef);
	for(uint i = 0; ss.get(&i, row);) {
		PriceListExportSpecItem item;
		MEMSZERO(item);
		StringSet ss2(',', row);
		uint   j = 0;
		if(ss2.get(&j, row_name)) {
			int    var_idx = -1;
			if(row_name.NotEmptyS() && PPSearchSubStr(pVarStr, &var_idx, row_name, 1)) {
				item.Var = (PriceListExportVarStr)var_idx;
				if(ss2.get(&j, item.Title, sizeof(item.Title))) {
					strip(item.Title);
					SOemToChar(item.Title);
					if(ss2.get(&j, sub)) {
						item.Size = sub.ToLong();
						if(item.Size < 0)
							item.Size = 0;
					}
				}
				switch(item.Var) {
					case plevsGoodsName: SETIFZ(item.Size,  54); break;
					case plevsBarcode:   SETIFZ(item.Size,  15); break;
					case plevsUnit:      SETIFZ(item.Size,  10); break;
					case plevsPrice:     SETIFZ(item.Size,  14); break;
					case plevsMemo:      SETIFZ(item.Size, 100); break;
					case plevsPack:
						if(!defSpec || (LConfig.Flags & CFGFLG_USEPACKAGE)) {
							SETIFZ(item.Size, 16);
						}
						else
							continue;
						break;
					case plevsPackPrice:
						if(!defSpec || (LConfig.Flags & CFGFLG_USEPACKAGE)) {
							SETIFZ(item.Size, 14);
						}
						else
							continue;
						break;
				}
				pSpec->insert(&item);
			}
		}
	}
	return 1;
}

static int PutSylkHeader(const SArray * pSpec, SylkWriter & sw, int rowNo)
{
	PriceListExportSpecItem * p_item;
	SString temp_buf;
	for(uint i = 0; pSpec->enumItems(&i, (void **)&p_item);) {
		switch(p_item->Var) {
			case plevsGoodsName:
				sw.PutFormat("FG0L;STBM6", 0, 0, rowNo);
				sw.PutFormat("FG0L", 0, i, rowNo);
				PPLoadString("appellation", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsBarcode:
				sw.PutFormat("FG0L", 0, i, rowNo);
				PPLoadString("barcode", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsUnit:
				sw.PutFormat("FG0L", 0, i, rowNo);
				PPLoadString("munit_s", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsPack:
				sw.PutFormat("FG0R", 0, i, rowNo);
				PPLoadString("unitperpack", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsPrice:
				sw.PutFormat("FG0R", 0, i, rowNo);
				PPLoadString("priceofunit", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsPackPrice:
				sw.PutFormat("FG0R", 0, i, rowNo);
				PPLoadString("priceofpack", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
			case plevsMemo:
				sw.PutFormat("FG0L", 0, i, rowNo);
				PPLoadString("memo", temp_buf);
				sw.PutVal(p_item->Title[0] ? p_item->Title : temp_buf.Transf(CTRANSF_INNER_TO_OUTER), 0);
				break;
		}
	}
	return 1;
}

int PPViewPriceList::Export()
{
	int    ok = 1, r;
	PPPriceListExporter l_e;
	THROW(r = l_e.Init(0));
	if(r > 0) {
		PPWaitStart();
		PriceListViewItem item;
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			THROW(l_e.Export(&item));
			PPWaitPercent(GetCounter());
		}
		l_e.DistributeFile();
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPriceList::Export_Pre9302()
{
	int    ok = -1;
	uint   i;
	SString main_org_name;
	SString fontface_arialcyr, fontface_tnr;
	SString temp_buf;
	char   date_buf[32];
	SString path;
	SArray spec(sizeof(PriceListExportSpecItem));
	PriceListExportSpecItem * p_spec_item;
	PriceListConfig plist_cfg;
	PPWaitStart();
	ReadPriceListConfig(&plist_cfg);
	{
		int def_spec = 0;
		if(*strip(plist_cfg.ExportSpec) == 0) {
			PPLoadText(PPTXT_DEFPLISTEXPORTSPEC, temp_buf);
			STRNSCPY(plist_cfg.ExportSpec, temp_buf);
			def_spec = 1;
		}
		PPLoadText(PPTXT_PLISTEXPORTVAR, temp_buf);
		ParsePriceListExportSpec(plist_cfg.ExportSpec, temp_buf, def_spec, &spec);
	}
	if(*strip(plist_cfg.OrgName) == 0)
		GetMainOrgName(main_org_name);
	else
		main_org_name = plist_cfg.OrgName;
	datefmt(&LConfig.OperDate, DATF_DMY|DATF_CENTURY, date_buf);

	PriceListViewItem item;
	int    row_no = 1, col_no = 0;
	PPGetFilePath(PPPATH_OUT, "price.xls", path);
	PPGetSubStr(PPTXT_FONTFACE, PPFONT_TIMESNEWROMAN, fontface_tnr);
	PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, fontface_arialcyr);
	SylkWriter sw(path);

	sw.PutRec("ID", "PPapyrus");
	for(i = 0; i < 4; i++)
		sw.PutFont('F', fontface_tnr, 9, 0);
	sw.PutFont('E', fontface_arialcyr, 14, slkfsBold);   // M5
	sw.PutFont('E', fontface_arialcyr,  9, slkfsBold);   // M6
	sw.PutFont('E', fontface_arialcyr,  8, slkfsItalic); // M7
	sw.PutFont('E', fontface_arialcyr,  9, slkfsBold | slkfsUnderline); // M8
	sw.PutRec('F', "G");
	for(i = 0; spec.enumItems(&i, (void **)&p_spec_item);)
		sw.PutColumnWidth(i, i, p_spec_item->Size);
	sw.PutFormat("FC0L", 7, 1, row_no);
	sw.PutVal("Файл подготовлен системой Papyrus", 0);
	sw.PutFormat("FC0L", 5, 2, row_no);
	temp_buf.Printf("Прайс-лист на %s", date_buf);
	sw.PutVal(temp_buf, 0);
	row_no++;
	sw.PutFormat("FC0L", 5, 2, row_no);
	sw.PutVal(main_org_name, 1);
	row_no++;

	PPIDArray grp_stack, prev_grp_stack;
	PPID   prev_grp_id = 0;
	for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
		if(item.GoodsGrpID != prev_grp_id) {
			Goods2Tbl::Rec grp_rec;
			for(PPID grp_id = item.GoodsGrpID; grp_id && GObj.Search(grp_id, &grp_rec) > 0;) {
				grp_stack.atInsert(0, &grp_id);
				grp_id = grp_rec.ParentID;
			}
			if(!prev_grp_stack.getCount() || grp_stack.at(0) != prev_grp_stack.at(0)) {
				MEMSZERO(grp_rec);
				GObj.Search(grp_stack.at(0), &grp_rec);

				row_no++;
				sw.PutFormat("FC0L", 5, 0, row_no);
				sw.PutFormat("FC0L", 0, 1, row_no);
				sw.PutVal(grp_rec.Name, 1);
				row_no++;
				PutSylkHeader(&spec, sw, row_no);
				row_no++;
			}

			row_no++;
			sw.PutFormat("FC0L", 8, 0, row_no);
			sw.PutFormat("FC0L", 0, 1, row_no);
			sw.PutVal(item.GoodsGrpName_, 1);
			row_no++;

			prev_grp_stack.copy(grp_stack);
			prev_grp_id = item.GoodsGrpID;
		}
		for(i = 0; spec.enumItems(&i, (void **)&p_spec_item);) {
			switch(p_spec_item->Var) {
				case plevsGoodsName:
					sw.PutFormat("FG0L", 0, i, row_no);
					sw.PutVal(item.GoodsName_, 1);
					break;
				case plevsBarcode:
					sw.PutFormat("FG0L", 0, i, row_no);
					GObj.GetSingleBarcode(item.GoodsID, temp_buf);
					sw.PutVal(temp_buf, 1);
					break;
				case plevsUnit:
					sw.PutFormat("FG0L", 0, i, row_no);
					GetObjectName(PPOBJ_UNIT, item.UnitID, temp_buf);
					sw.PutVal(temp_buf, 1);
					break;
				case plevsPack:
					sw.PutFormat("FG0R", 0, i, row_no);
					if(item.UnitsPerPack > 0)
						sw.PutVal(item.UnitsPerPack);
					break;
				case plevsPrice:
					sw.PutFormat("FG0R", 0, i, row_no);
					if(item.Price > 0)
						sw.PutVal(item.Price);
					break;
				case plevsPackPrice:
					sw.PutFormat("FG0R", 0, i, row_no);
					if(item.UnitsPerPack > 0)
						sw.PutVal(item.Price * item.UnitsPerPack);
					break;
				case plevsMemo:
					sw.PutFormat("FG0L", 0, i, row_no);
					sw.PutVal(item.Memo_, 1);
					break;
			}
		}
		row_no++;
		PPWaitPercent(GetCounter());
	}
	sw.PutLine("E");
	PPWaitStop();
	return ok;
}

int PPViewPriceList::ExportUhtt()
{
	int    ok = -1;
	const  PPID src_loc_id = Filt.LocID;
	if(src_loc_id) {
		int    uhtt_src_loc_id = 0;
		int    uhtt_dlvr_loc_id = 0;
		PriceListViewItem item;
		BarcodeArray bc_list;
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		UhttBillPacket uhtt_pack;
		UhttLocationPacket uhtt_loc_pack;
		PPUhttClient uhtt_cli;
		PPWaitStart();
		THROW(uhtt_cli.Auth());
		THROW(loc_obj.Fetch(src_loc_id, &loc_rec) > 0);
		THROW_PP_S(loc_rec.Code[0], PPERR_LOCSYMBUNDEF, loc_rec.Name);
		THROW(uhtt_cli.GetLocationByCode(loc_rec.Code, uhtt_loc_pack) > 0);
		uhtt_src_loc_id = uhtt_loc_pack.ID;
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0; PPWaitPercent(IterCounter())) {
			int    uhtt_goods_id = 0;
			if(uhtt_cli.GetUhttGoods(labs(item.GoodsID), 0, &uhtt_goods_id, 0) > 0) {
				UhttQuotPacket uhtt_qp;
				uhtt_qp.GoodsID = uhtt_goods_id;
				uhtt_qp.LocID = uhtt_src_loc_id;
				uhtt_qp.Value = item.Price;
				if(!uhtt_cli.SetQuot(uhtt_qp)) {
					// @logmsg
				}
			}
			else {
			}
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
int FASTCALL ReadPriceListConfig(PriceListConfig * pCfg)
{
	memzero(pCfg, sizeof(*pCfg));
	int    r = PPRef->GetPropMainConfig(PPPRP_PLISTCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(PriceListConfig));
	return r;
}

int EditPriceListConfig()
{
	int    ok = -1, is_new = 0;
	PriceListConfig cfg;
	TDialog * dlg = 0;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	THROW(CheckCfgRights(PPCFGOBJ_PRICELIST, PPR_READ, 0));
	THROW(is_new = ReadPriceListConfig(&cfg));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_PLISTCFG))));
	SetupPPObjCombo(dlg, CTLSEL_PLISTCFG_P1Q, PPOBJ_QUOTKIND, cfg.AddPriceQuot[0], 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_PLISTCFG_P2Q, PPOBJ_QUOTKIND, cfg.AddPriceQuot[1], 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_PLISTCFG_P3Q, PPOBJ_QUOTKIND, cfg.AddPriceQuot[2], 0, 0);
	SetupPPObjCombo(dlg, CTLSEL_PLISTCFG_GGRP, PPOBJ_GOODSGROUP, cfg.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
	if((p_cb = static_cast<ComboBox *>(dlg->getCtrlView(CTLSEL_PLISTCFG_EXTFLD))) != 0) {
		int    idx = 0, fld_list[12];
		SString goods_ex_titles, item_buf;
		PPObjGoods::ReadGoodsExTitles(0, goods_ex_titles);
		THROW(p_lw = CreateListWindow_Simple(lbtDblClkNotify));
		fld_list[0] = GDSEXSTR_STORAGE;
		fld_list[1] = GDSEXSTR_STANDARD;
		fld_list[2] = GDSEXSTR_INGRED;
		fld_list[3] = GDSEXSTR_ENERGY;
		fld_list[4] = GDSEXSTR_USAGE;
		for(idx = 0; idx < 5; idx++) {
			PPGetExtStrData(fld_list[idx], goods_ex_titles, item_buf);
			if(item_buf.IsEmpty())
				item_buf.CatChar('A'+idx);
			p_lw->listBox()->addItem(fld_list[idx], item_buf);
		}
		p_cb->setListWindow(p_lw, cfg.ExtFldMemoSubst);
	}
	dlg->AddClusterAssoc(CTL_PLISTCFG_EXCLGGRP, 0, PLISTF_EXCLGGRP);
	dlg->SetClusterData(CTL_PLISTCFG_EXCLGGRP, cfg.Flags);
	dlg->AddClusterAssoc(CTL_PLISTCFG_FLAGS, 0, PLISTF_PRESENTONLY);
	dlg->AddClusterAssoc(CTL_PLISTCFG_FLAGS, 1, PLISTF_SENDINXML);
	dlg->AddClusterAssoc(CTL_PLISTCFG_FLAGS, 2, PLISTF_NOTCFMDEL);
	dlg->SetClusterData(CTL_PLISTCFG_FLAGS, cfg.Flags);
	dlg->setCtrlData(CTL_PLISTCFG_ORGNAME, cfg.OrgName);
	dlg->setCtrlData(CTL_PLISTCFG_BUSINESS, cfg.Business);
	dlg->setCtrlData(CTL_PLISTCFG_PUBMAIL, cfg.PublisherMail);
	dlg->setCtrlData(CTL_PLISTCFG_EXPORTSPEC, cfg.ExportSpec);
	if(ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_PRICELIST, PPR_MOD, 0));
		dlg->getCtrlData(CTLSEL_PLISTCFG_P1Q,  &cfg.AddPriceQuot[0]);
		dlg->getCtrlData(CTLSEL_PLISTCFG_P2Q,  &cfg.AddPriceQuot[1]);
		dlg->getCtrlData(CTLSEL_PLISTCFG_P3Q,  &cfg.AddPriceQuot[2]);
		dlg->getCtrlData(CTLSEL_PLISTCFG_GGRP, &cfg.GoodsGrpID);
		dlg->getCtrlData(CTLSEL_PLISTCFG_EXTFLD, &cfg.ExtFldMemoSubst);
		dlg->GetClusterData(CTL_PLISTCFG_FLAGS,    &cfg.Flags);
		dlg->GetClusterData(CTL_PLISTCFG_EXCLGGRP, &cfg.Flags);
		dlg->getCtrlData(CTL_PLISTCFG_ORGNAME,  cfg.OrgName);
		dlg->getCtrlData(CTL_PLISTCFG_BUSINESS, cfg.Business);
		dlg->getCtrlData(CTL_PLISTCFG_PUBMAIL, cfg.PublisherMail);
		dlg->getCtrlData(CTL_PLISTCFG_EXPORTSPEC, cfg.ExportSpec);
		THROW(PPObject::Helper_PutConfig(PPPRP_PLISTCFG, PPCFGOBJ_PRICELIST, (is_new == -1), &cfg, sizeof(cfg), 1));
		ok = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
// Implementation of PPALDD_PriceListData
//
PPALDD_CONSTRUCTOR(PriceListData)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PriceListData) { Destroy(); }

int PPALDD_PriceListData::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(PriceList, rsrv);

	H.CfgPriceQuot1 = p_v->Cfg.AddPriceQuot[0];
	H.CfgPriceQuot2 = p_v->Cfg.AddPriceQuot[1];
	H.CfgPriceQuot3 = p_v->Cfg.AddPriceQuot[2];

	H.LocID   = p_filt->LocID;
	H.SupplID = p_filt->ArticleID;
	H.FltQuotKindID = p_filt->QuotKindID;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltManufID    = p_filt->ManufID;
	p_filt->Memo.CopyTo(H.PLMemo, sizeof(H.PLMemo));
	H.PListID      = p_filt->PListID;
	H.Date = p_filt->Dt;
	H.FltAddPct    = p_filt->PctAdd;
	H.fByQuot      = BIN(p_filt->Flags & PLISTF_BYQUOT);
	H.fPresentOnly = BIN(p_filt->Flags & PLISTF_PRESENTONLY);
	H.fExcludeGoodsGrp = BIN(p_filt->Flags & PLISTF_EXCLGGRP);
	H.FltLotBeg	   = p_filt->LotPeriod.low;
	H.FltLotEnd	   = p_filt->LotPeriod.upp;
	if(p_filt->LotPeriod.low)
		H.fPlistNew = BIN(p_filt->Flags & PLISTF_NEWP);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PriceListData::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER_ORD(PriceList, static_cast<PPViewPriceList::IterOrder>(sortId));
}

int PPALDD_PriceListData::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(PriceList);
	I.LN  = item.LN;
	I.QCertID     = item.QCertID;
	I.GoodsGrpID  = item.GoodsGrpID;
	I.GoodsID     = item.GoodsID;
	I.QuotKindID  = item.QuotKindID;
	I.UnitID      = item.UnitID;
	I.ManufID     = item.ManufID;
	I.UnitPerPack = item.UnitsPerPack;
	I.LineNo      = item.LineNo;
	I.Price       = item.Price;
	I.AddPrice1   = item.AddPrice1;
	I.AddPrice2   = item.AddPrice2;
	I.AddPrice3   = item.AddPrice3;
	I.Rest        = item.Rest;
	I.Expiry      = item.Expiry;
	I.GoodsCode   = item.GoodsCode;
	if(H.FltAddPct != 0) {
		// @v10.9.11 I.Price = CalcSelling(I.Price, H.FltAddPct);
		I.Price = PPObjQuotKind::RoundUpPrice(0, I.Price + I.Price * fdiv100r(H.FltAddPct)); // @v10.9.11 
	}
	STRNSCPY(I.GoodsName, item.GoodsName_);
	STRNSCPY(I.ExtGroupName, item.GoodsGrpName_);
	STRNSCPY(I.Memo,      item.Memo_);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PriceListData::Destroy() { DESTROY_PPVIEW_ALDD(PriceList); }
