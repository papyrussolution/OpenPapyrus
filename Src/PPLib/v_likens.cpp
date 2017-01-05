// V_LIKENS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2016
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewObjLikeness)
//

IMPLEMENT_PPFILT_FACTORY(ObjLikeness); SLAPI ObjLikenessFilt::ObjLikenessFilt() : PPBaseFilt(PPFILT_OBJLIKENESS, 0, 0)
{
	SetFlatChunk(offsetof(ObjLikenessFilt, ReserveStart),
		offsetof(ObjLikenessFilt, Reserve) - offsetof(ObjLikenessFilt, ReserveStart) + sizeof(Reserve));
	Init(1, 0);
}

ObjLikenessFilt & FASTCALL ObjLikenessFilt::operator = (const ObjLikenessFilt &src)
{
	Copy(&src, 0);
	return *this;
}

SLAPI PPViewObjLikeness::PPViewObjLikeness() : PPView(0, &Filt, PPVIEW_OBJLIKENESS)
{
}

SLAPI PPViewObjLikeness::~PPViewObjLikeness()
{
}

class ObjLikenessFiltDialog : public TDialog {
public:
	  ObjLikenessFiltDialog() : TDialog(DLG_LIKEOBJFLT) {}

	  int setDTS(const ObjLikenessFilt *);
	  int getDTS(ObjLikenessFilt *);
private:
	  ObjLikenessFilt Data;
};

int ObjLikenessFiltDialog::setDTS(const ObjLikenessFilt * pData)
{
	double rate_pct = 0;
	PPIDArray obj_list;
	if(!RVALUEPTR(Data, pData))
		Data.Init(1, 0);
	obj_list.add(PPOBJ_GOODS);
	// obj_list.add(PPOBJ_PERSON);
	SetupObjListCombo(this, CTLSEL_LIKEOBJFLT_OBJ, Data.ObjTypeID, &obj_list);
	rate_pct = (long)(Data.Rate * 100);
	setCtrlData(CTL_LIKEOBJFLT_RATEPCT, &rate_pct);
	return 1;
}

int ObjLikenessFiltDialog::getDTS(ObjLikenessFilt * pData)
{
	int    ok = 1, sel = CTLSEL_LIKEOBJFLT_OBJ;
	SString buf;
	Data.ObjTypeID = getCtrlLong(CTLSEL_LIKEOBJFLT_OBJ);
	buf.Cat(Data.ObjTypeID);
	THROW_PP_S(oneof2(Data.ObjTypeID, PPOBJ_GOODS, PPOBJ_PERSON), PPERR_OBJNOTSUPPORTED, buf);
	getCtrlData(sel = CTL_LIKEOBJFLT_RATEPCT, &Data.Rate);
	THROW_PP(Data.Rate >= 0 && Data.Rate <= 100, PPERR_PERCENTINPUT);
	Data.Rate /= 100;
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = (selectCtrl(sel), 0);
	ENDCATCH
	return ok;
}

int SLAPI PPViewObjLikeness::EditBaseFilt(PPBaseFilt * pFilt)
{
	DIALOG_PROC_BODYERR(ObjLikenessFiltDialog, (ObjLikenessFilt *)pFilt);
}

int SLAPI PPViewObjLikeness::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString buf;
	THROW(Helper_InitBaseFilt(pFilt));
	buf.Cat(Filt.ObjTypeID);
	THROW_PP_S(oneof2(Filt.ObjTypeID, PPOBJ_GOODS, PPOBJ_PERSON), PPERR_OBJNOTSUPPORTED, buf);
	CATCHZOK
	return ok;
}

int SLAPI PPViewObjLikeness::InitIteration()
{
	int    ok  = 1;
	Counter.Init();
	ZDELETE(P_IterQuery);
	{
		ObjLikenessTbl::Key0 k0, k0_;
		THROW_MEM(P_IterQuery = new BExtQuery(&Tbl, 0));
		P_IterQuery->selectAll();
		MEMSZERO(k0);
		k0.ObjType = Filt.ObjTypeID;
		Counter.Init(P_IterQuery->countIterations(0, &(k0_ = k0), spGe));
		P_IterQuery->initIteration(0, &k0, spFirst);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewObjLikeness::NextIteration(ObjLikenessViewItem * pItem)
{
	int ok = -1;
	if(P_IterQuery->nextIteration() > 0) {
		ASSIGN_PTR(pItem, Tbl.data);
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

DBQuery * SLAPI PPViewObjLikeness::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    func_name = 0;
	uint   brw_id = BROWSER_OBJLIKENESS;
	SString sub_title;
	DBQuery        * q  = 0;
	ObjLikenessTbl * l = 0;
	DBE dbe_pct_rate, dbe_name1, dbe_name2;
	DBQ  * dbq = 0;

	THROW(CheckTblPtr(l = new ObjLikenessTbl(Tbl.fileName)));
	dbq = ppcheckfiltid(dbq, l->ObjType, Filt.ObjTypeID);
	dbq =& (*dbq && l->Rate >= Filt.Rate);
	{
		dbe_pct_rate.init();
		dbe_pct_rate.push(l->Rate);
		DBConst dbc_doub;
		dbc_doub.init(1.0);
		dbe_pct_rate.push(dbc_doub);
		dbe_pct_rate.push((DBFunc)PPDbqFuncPool::IdPercent);
	}
	if(Filt.ObjTypeID == PPOBJ_PERSON)
		func_name = PPDbqFuncPool::IdObjNamePerson;
	else if(Filt.ObjTypeID == PPOBJ_GOODS)
		func_name = PPDbqFuncPool::IdObjNameGoods;
	PPDbqFuncPool::InitObjNameFunc(dbe_name1, func_name, l->ID1);
	PPDbqFuncPool::InitObjNameFunc(dbe_name2, func_name, l->ID2);
	q = & select(
		l->ObjType,
		l->ID1,
		l->ID2,
		dbe_pct_rate,
		dbe_name1,
		dbe_name2,
		0L);
	if(pSubTitle)
		GetObjectTitle(Filt.ObjTypeID, sub_title);
	q->from(l, 0L).where(*dbq).orderBy(l->ObjType, l->Rate, 0L);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete l;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	ASSIGN_PTR(pSubTitle, sub_title);
	return q;
}

int SLAPI PPViewObjLikeness::UniteObjects(PPID objType, PPID srcID, PPID destID)
{
	int    ok = -1;
	if(CONFIRM(PPCFM_UNITEOBJ)) {
		PPTransaction tra(1);
		THROW(tra);
		THROW(PPObject::ReplaceObj(objType, srcID, destID, 0));
		THROW_DB(deleteFrom(&Tbl, 0, Tbl.ObjType == objType && Tbl.ID1 == srcID && Tbl.ID2 == destID));
		THROW_DB(deleteFrom(&Tbl, 0, Tbl.ObjType == objType && Tbl.ID1 == destID && Tbl.ID2 == srcID));
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewObjLikeness::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	BrwHdr hdr;
	if(pHdr)
		hdr = *((BrwHdr*)pHdr);
	else
		MEMSZERO(hdr);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EDITITEM1:
			case PPVCMD_EDITITEM2:
				if(hdr.ObjType == PPOBJ_GOODS) {
					int r = 0;
					PPObjGoods gobj;
					if((r = gobj.Edit((ppvCmd == PPVCMD_EDITITEM1) ? &hdr.ID1 : &hdr.ID2, 0)) == cmOK)
						ok = 1;
					else if(r == 0)
						ok = 0;
				}
				break;
			case PPVCMD_UNITE:
				UniteObjects(hdr.ObjType, hdr.ID1, hdr.ID2);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewObjLikeness::Print(const void *)
{
	int ok = -1;
	return ok;
}

// static
int SLAPI PPViewObjLikeness::CreateLikenessTable()
{
	int    ok = -1;
	double rate = 0.5;
	long   i;
	SString buf;
	PPID obj_type = 0;
	StrAssocArray obj_list;
	PPIniFile ini_file;
	ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_LIKENESSRATE, buf);
	rate = buf.ToReal();
	rate = (rate > 0 && rate <= 1) ? rate : 0.5;
	obj_list.Add(PPOBJ_GOODS, GetObjectTitle(PPOBJ_GOODS, buf));
	// obj_list.Add(PPOBJ_PERSON, GetObjectTitle(PPOBJ_PERSON, buf));
	if(ListBoxSelDialog(&obj_list, "@objtype", &obj_type, 0) > 0 && obj_type > 0) {
		int    swap = 0;
		long   count = 0;
		ObjLikenessTbl tbl;
		ObjLikenessTbl::Rec lkns_rec;
		Goods2Tbl::Key0 k0;
		Goods2Tbl::Rec grec1, grec2;
		SArray likeness_list(sizeof(ObjLikenessTbl::Rec), 32, O_ARRAY);
		StrAssocArray list;
		PPObjGoods gobj;
		GoodsIterator gi;

		MEMSZERO(k0);
		MEMSZERO(grec1);
		MEMSZERO(grec2);
		MEMSZERO(lkns_rec);
		lkns_rec.ObjType = obj_type;
		PPWait(1);
		PROFILE_START
		for(gi.Init(0); gi.Next(&grec1) > 0;)
			THROW_SL(list.Add(grec1.ID, strlen(grec1.Name), grec1.Name));
		PROFILE_END
		PROFILE_START
		count = list.getCount();
		deleteFrom(&tbl, 0, tbl.ObjType == obj_type);
		PROFILE_END
		PROFILE_START
		MEMSZERO(grec1);
#if 0 // {
		for(i = 0; i < count; i++) {
			StrAssocArray::Item item1, item2;
			item1 = list.at(i);
			grec1.ID = item1.Id;
			STRNSCPY(grec1.Name, item1.Txt);
			for(long j = i + 1; j < count; j++) {
				item2 = list.at(j);
				grec2.ID = item2.Id;
				STRNSCPY(grec2.Name, item2.Txt);
				if((lkns_rec.Rate = gobj.CalcLikeness(&grec1, &grec2, &swap, 0)) >= rate) {
					lkns_rec.ID1 = (swap) ? grec2.ID : grec1.ID;
					lkns_rec.ID2 = (swap) ? grec1.ID : grec2.ID;
					THROW_SL(likeness_list.insert(&lkns_rec));
				}
			}
			PPWaitPercent(i, count);
		}
#else
		list.SortByLength(1 /* descend */);
		ApproxStrSrchParam asc_param;
		MEMSZERO(asc_param);
		asc_param.method = 1;
		asc_param.weight = 1.0;
		asc_param.no_case = 1;
		for(i = 0; i < count; i++) {
			StrAssocArray::Item item1, item2;
			item1 = list.at(i);
			ApproxStrComparator asc(item1.Txt, &asc_param);
			for(long j = i + 1; j < count; j++) { // все элементы сравниваем с друг другом только 1 раз
				item2 = list.at(j);
				double s = asc.Next(item2.Txt);
				if(s >= rate) {
					lkns_rec.ID1  = item1.Id;
					lkns_rec.ID2  = item2.Id;
					lkns_rec.Rate = s;
					THROW_SL(likeness_list.insert(&lkns_rec));
				}
			}
			PPWaitPercent(i, count);
		}
#endif // } 0
		PROFILE_END
		PROFILE_START
		ok = 1;
		{
			PPTransaction tra(1);
			THROW(tra);
			{
				BExtInsert bei(&tbl);
				count = likeness_list.getCount();
				for(i = 0; i < count; i++) {
					THROW_DB(bei.insert((ObjLikenessTbl::Rec*)likeness_list.at(i)));
					if((count % 10) == 0)
						PPWaitPercent(i, count);
				}
				THROW_DB(bei.flash());
			}
			THROW(tra.Commit());
		}
		PROFILE_END
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}
