// OBJBIZSC.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

IMPL_CMPFUNC(BzsValVector_Ident, i1, i2)
{
	const BzsValVector * ptr1 = static_cast<const BzsValVector *>(i1);
	const BzsValVector * ptr2 = static_cast<const BzsValVector *>(i2);
	return CMPSIGN(ptr1->Ident, ptr2->Ident);
}

BzsVal::BzsVal() : Bzsi(0), Val(0.0)
{
}

BzsValVector::BzsValVector() : TSVector <BzsVal>(), Ident(0ULL)
{
}

int BzsValVector::Add(long id, double value)
{
	int    ok = -1;
	if(id) {
		uint   idx = 0;
		if(lsearch(&id, &idx, CMPF_LONG)) {
			BzsVal & r_item = at(idx);
			r_item.Val += value;
			ok = 2;
		}
		else {
			BzsVal new_item;
			new_item.Bzsi = id;
			new_item.Val = value;
			if(insert(&new_item))
				ok = 1;
			else
				ok = PPSetErrorSLib();
		}
	}
	else
		ok = 0;
	return ok;
}

bool BzsValVector::Get(long id, double * pValue) const
{
	bool   ok = false;
	uint   idx = 0;
	if(lsearch(&id, &idx, CMPF_LONG)) {
		ASSIGN_PTR(pValue, at(idx).Val);
		ok = true;
	}
	else {
		ASSIGN_PTR(pValue, 0.0);
	}
	return ok;
}

bool BzsValVector::GetCompound(long id, double * pValue) const
{
	bool   ok = false;
	double value = 0.0;
	switch(id) {
		case PPBZSI_ORDSHIPMDELAYDAYSAVG:
			{
				double d = 0.0;
				double c = 0.0;
				if(Get(PPBZSI_ORDSHIPMDELAYDAYS, &d) && Get(PPBZSI_SALECOUNT, &c)) {
					value = (c > 0.0) ? d / c : 0.0;
					ok = true;
				}
			}
			break;
		case PPBZSI_ORDCANCELLATIONRATE:
			{
				double cc = 0.0;
				double oc = 0.0;
				if(Get(PPBZSI_ORDCANCELLEDCOUNT, &cc) && Get(PPBZSI_ORDCOUNT, &oc)) {
					value = (oc > 0.0) ? cc / oc : 0.0;
					ok = true;
				}
			}
			break;
	}
	ASSIGN_PTR(pValue, value);
	return ok;
}

PPBizScore::PPBizScore()
{
	THISZERO();
}

PPObjBizScore::PPObjBizScore(void * extraPtr) : PPObjReference(PPOBJ_BIZSCORE, extraPtr), P_Resolver(0), P_ValTbl(0)
{
}

PPObjBizScore::~PPObjBizScore()
{
	delete P_Resolver;
	delete P_ValTbl;
}

#define BIZSCEXSTR_DESCR   1
#define BIZSCEXSTR_FORMULA 2

//int PPObjBizScore::Remove(PPID id, long, uint options)
/*virtual*/int  PPObjBizScore::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1, conf = 1;
	THROW(CheckRights(PPR_DEL));
	SETIFZ(P_ValTbl, new BizScoreCore);
	THROW_MEM(P_ValTbl);
	if(options & PPObject::user_request) {
		BizScoreTbl::Key1 k1;
		MEMSZERO(k1);
		k1.ScoreID = id;
		if(P_ValTbl->search(1, &k1, spGe) && k1.ScoreID == id)
			conf = CONFIRM(PPCFM_DELBIZSCORE);
		else
			conf = CONFIRM(PPCFM_DELETE);
	}
	if(conf) {
		PPWaitStart();
		THROW(PutPacket(&id, 0, BIN(options & PPObject::use_transaction)));
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWaitStop();
	return r;
}

int PPObjBizScore::PutPacket(PPID * pID, PPBizScorePacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   action = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
			if(*pID) {
				int r;
				THROW(CheckRights(PPR_MOD));
				THROW(r = P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				if(r < 0)
					ok = -1;
				// Событие PPACN_OBJUPD создано функцией P_Ref->UpdateItem : action не инициалазируем
			}
			else {
				THROW(CheckRights(PPR_INS));
				THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
				// Событие PPACN_OBJADD создано функцией P_Ref->AddItem : action не инициалазируем
			}
			{
				SString strg_buf;
				SString prev_strg_buf;
				PPPutExtStrData(BIZSCEXSTR_DESCR,   strg_buf, pPack->Descr);
				PPPutExtStrData(BIZSCEXSTR_FORMULA, strg_buf, pPack->Formula);
				THROW(P_Ref->GetPropVlrString(Obj, *pID, BZSPRP_DESCR, prev_strg_buf));
				if(strg_buf.Cmp(prev_strg_buf, 0) != 0) {
					THROW(P_Ref->PutPropVlrString(Obj, *pID, BZSPRP_DESCR, strg_buf));
					if(ok < 0) {
						//
						// Заголовочная запись не изменилась, но изменилась формула либо описание.
						//
						ok = 1;
						action = PPACN_OBJUPD;
					}
				}
			}
		}
		else if(*pID) {
			THROW(CheckRights(PPR_DEL));
			SETIFZ(P_ValTbl, new BizScoreCore);
			THROW_MEM(P_ValTbl);
			THROW_DB(deleteFrom(P_ValTbl, 0, P_ValTbl->ScoreID == *pID));
			THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			THROW(P_Ref->PutPropVlrString(Obj, *pID, BZSPRP_DESCR, 0));
			action = PPACN_OBJRMV;
		}
		DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBizScore::GetPacket(PPID id, PPBizScorePacket * pPack)
{
	int    ok = 1;
	PPBizScorePacket pack;
	int    r = P_Ref->GetItem(Obj, id, &pack.Rec);
	THROW(r);
	if(r > 0) {
		SString strg_buf;
		THROW(P_Ref->GetPropVlrString(Obj, id, BZSPRP_DESCR, strg_buf));
		PPGetExtStrData(BIZSCEXSTR_DESCR,   strg_buf, pack.Descr);
		PPGetExtStrData(BIZSCEXSTR_FORMULA, strg_buf, pack.Formula);
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pPack, pack);
	return ok;
}

int PPObjBizScore::ReverseFormula(PPBizScorePacket * pPack, SString & rResult)
{
	int    ok = -1;
	rResult.Z();
	if(pPack->Formula.NotEmptyS()) {
		SETIFZ(P_Resolver, new DL2_Resolver());
		ok = P_Resolver->ReverseFormula(pPack->Formula, rResult);
	}
	return ok;
}

int PPObjBizScore::TestPacket(PPBizScorePacket * pPack, SString & rResult)
{
	int    ok = -1;
	rResult.Z();
	if(pPack->Formula.NotEmptyS()) {
		SETIFZ(P_Resolver, new DL2_Resolver());
		double val = P_Resolver->Resolve(pPack->Formula);
		rResult.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
		ok = 1;
	}
	return ok;
}

// @vmiller {
class BizPrimitivCreateDialog : public TDialog {
	enum {
		ctlgroupLoc = 1
	};
public:
	BizPrimitivCreateDialog(const char * pBizScoreName) : TDialog(DLG_BIZPRCRT), BizScoreName(pBizScoreName)
	{
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_BIZPRCRT_LOC, 0, 0, cmLocList, 0, 0, 0));
		SetupCalPeriod(CTLCAL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_PERIOD);
	}
	int    setDTS(const DL2_Score * pData)
	{
		Data = *pData;
		Buf_Data = *pData;
		setCtrlString(CTL_BIZPRCRT_PRMTVNAME, BizScoreName);
		// Выбор примитива
		long   id = 0;
		uint   pos = 0;
		SString str;
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_METAVAR, str);
		Primitiv_List.Add(0, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_BILL, str);
		Primitiv_List.Add(1, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_PAYM, str);
		Primitiv_List.Add(2, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_CCHECK, str);
		Primitiv_List.Add(3, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_GOODSREST, str);
		Primitiv_List.Add(4, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_PERSONEVENT, str);
		Primitiv_List.Add(5, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_DEBT, str);
		Primitiv_List.Add(6, str);
		PPGetSubStr(PPTXT_BIZSCORE_PRIMITIVES, BIZSCORE_PRIMITIVES_BIZSCORE, str);
		Primitiv_List.Add(7, str);
		//
		id = (Primitiv_List.SearchByTextNc(Primitiv_List.Get(Data.Kind-1).Txt, &(pos = 0)) > 0) ? (uint)Primitiv_List.Get(pos).Id : 0;
		SetupStrAssocCombo(this, CTLSEL_BIZPRCRT_PRIMITIV, Primitiv_List, (long)id, 0);
		// В зависимости от выбранного примитива активируем/блокируем остальные параметры
		DisableControls(id);
		//
		ushort v = 0;
		// Выбор модификатора
		/* @v12.1.
		switch(Data.Sub) {
			case PPBZSI_AMOUNT: v = 1; break;
			case PPBZSI_AMT_COST: v = 2; break;
			case PPBZSI_AMT_PRICE: v = 3; break;
			case PPBZSI_AMT_DISCOUNT: v = 4; break;
			case PPBZSI_AMT_NETPRICE: v = 5; break;
			case PPBZSI_AMT_MARGIN: v = 6; break;
			case PPBZSI_PCTINCOME: v = 7; break;
			case PPBZSI_PCTMARGIN: v = 8; break;
			case PPBZSI_COUNT: v = 9; break;
			case PPBZSI_AVERAGE: v = 10; break;
			default: v = 0; break;
		}
		setCtrlData(CTL_BIZPRCRT_MODIF, &v);
		*/
		// @v12.1.6 {
		AddClusterAssocDef(CTL_BIZPRCRT_MODIF,  0, PPBZSI_NONE);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  1, PPBZSI_AMOUNT);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  2, PPBZSI_AMT_COST);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  3, PPBZSI_AMT_PRICE);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  4, PPBZSI_AMT_DISCOUNT);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  5, PPBZSI_AMT_NETPRICE);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  6, PPBZSI_AMT_MARGIN);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  7, PPBZSI_PCTINCOME);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  8, PPBZSI_PCTMARGIN);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF,  9, PPBZSI_COUNT);
		AddClusterAssoc(CTL_BIZPRCRT_MODIF, 10, PPBZSI_AVERAGE);
		SetClusterData(CTL_BIZPRCRT_MODIF, Data.Sub);
		// } @v12.1.6 
		// Аргументы
		PPID   loc_id = 0;
		SetPeriodInput(this, CTL_BIZPRCRT_PERIOD, Data.Period);
		SetupPPObjCombo(this, CTLSEL_BIZPRCRT_LOC, PPOBJ_LOCATION, Data.LocListID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_BIZPRCRT_GOODSGRP, PPOBJ_GOODSGROUP, Data.GoodsGrpListID, 0, 0);
		setCtrlData(CTL_BIZPRCRT_OPSYMB, Data.OpCode);
		//

		// Сравнение с периодом
		switch(Data.Cmp) {
			case DL2_Score::cmpDD: v = 1; break;
			case DL2_Score::cmpMM: v = 2; break;
			case DL2_Score::cmpQQ: v = 3; break;
			case DL2_Score::cmpYY: v = 4; break;
			case DL2_Score::cmpPrev: v = 5; break;
			default: v = 0; break;
		}
		setCtrlData(CTL_BIZPRCRT_PRDCMP, &v);
		return 1;
	}
	int    getDTS(DL2_Score * pData)
	{
        Data = Buf_Data;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_BIZPRCRT_PRIMITIV)) {
			long id = 0;
			getCtrlData(CTLSEL_BIZPRCRT_PRIMITIV, &(id = 0));
			Buf_Data.Kind = (int16)(id + 1);
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			DisableControls(id);
			clearEvent(event);
		}
		if(event.isClusterClk(CTL_BIZPRCRT_MODIF)) {
			GetClusterData(CTL_BIZPRCRT_MODIF, &Data.Sub); // @v12.1.6
			/* @v12.1.
			ushort v = 0;
			getCtrlData(CTL_BIZPRCRT_MODIF, &(v = 0));
			switch(v) {
				case 1: Buf_Data.Sub = PPBZSI_AMOUNT; break;
				case 2: Buf_Data.Sub = PPBZSI_AMT_COST; break;
				case 3: Buf_Data.Sub = PPBZSI_AMT_PRICE; break;
				case 4: Buf_Data.Sub = PPBZSI_AMT_DISCOUNT; break;
				case 5: Buf_Data.Sub = PPBZSI_AMT_NETPRICE; break;
				case 6: Buf_Data.Sub = PPBZSI_AMT_MARGIN; break;
				case 7: Buf_Data.Sub = PPBZSI_PCTINCOME; break;
				case 8: Buf_Data.Sub = PPBZSI_PCTMARGIN; break;
				case 9: Buf_Data.Sub = PPBZSI_COUNT; break;
				case 10: Buf_Data.Sub = PPBZSI_AVERAGE; break;
				default: Buf_Data.Sub = PPBZSI_NONE; break;
			}*/
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
		if(event.isClusterClk(CTL_BIZPRCRT_PRDCMP)) {
			ushort v = 0;
			getCtrlData(CTL_BIZPRCRT_PRDCMP, &(v = 0));
			switch(v) {
				case 1: Buf_Data.Cmp = DL2_Score::cmpDD; break;
				case 2: Buf_Data.Cmp = DL2_Score::cmpMM; break;
				case 3: Buf_Data.Cmp = DL2_Score::cmpQQ; break;
				case 4: Buf_Data.Cmp = DL2_Score::cmpYY; break;
				case 5: Buf_Data.Cmp = DL2_Score::cmpPrev; break;
				default: Buf_Data.Cmp = DL2_Score::cmpNone; break;
			}
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
		if(event.isCtlEvent(CTL_BIZPRCRT_PERIOD)) {
			GetPeriodInput(this, CTL_BIZPRCRT_PERIOD, &Buf_Data.Period);
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
		if(/*event.isCbSelected(CTLSEL_BIZPRCRT_LOC) || */event.isCtlEvent(CTL_BIZPRCRT_LOC)) {
			long id = 0;
			if(getGroupData(ctlgroupLoc, &Loc_Ctrl_Rec)) {
				for(uint i = 0; i < Loc_Ctrl_Rec.LocList.GetCount(); i++) {
					Obj_Loc.Fetch(Loc_Ctrl_Rec.LocList.Get(i), &Loc_Rec);
					Str_Set.add(Loc_Rec.Code);
				}
			}
			id = 0;
			Buf_Data.P_Ctx->Oc.Set(PPOBJ_LOCATION, &Str_Set, &id);
			Buf_Data.LocListID = id;

			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
		if(event.isCbSelected(CTLSEL_BIZPRCRT_GOODSGRP)) {
			Buf_Data.GoodsGrpListID = getCtrlLong(CTL_BIZPRCRT_GOODSGRP);
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
		if(event.isCtlEvent(CTL_BIZPRCRT_OPSYMB)) {
			getCtrlData(CTL_BIZPRCRT_OPSYMB, &Buf_Data.OpCode);
			Buf_Data.PutToStr(Str_Buf);
			setCtrlString(CTL_BIZPRCRT_RESULT, Str_Buf);
			clearEvent(event);
		}
	}
	void DisableControls(const long id)
	{
		switch(id + 1) {
			case DL2_Score::kBill:
				{
					// Активируем
					disableCtrls(0, CTL_BIZPRCRT_PERIOD, CTLSEL_BIZPRCRT_LOC, CTL_BIZPRCRT_OPSYMB, 0L);
					// Блокируем
					disableCtrls(1, CTLSEL_BIZPRCRT_GOODSGRP, 0L);
					// Модифиакторы
					//disableCtrls(0, CTL_BIZPRCRT_MODIF, 0L);
					// @v11.6.2 {
					const long idx_list[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
					DisableClusterItems(CTL_BIZPRCRT_MODIF, LongArray(idx_list, SIZEOFARRAY(idx_list)), 0);
					// } @v11.6.2 
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 0, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);
					if(Buf_Data.Sub == 10) {
						Buf_Data.Sub = 0;
						setCtrlData(CTL_BIZPRCRT_MODIF, &Buf_Data.Sub);
					}
					DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 1);
				}
				break;
			case DL2_Score::kPaym:
				{
					// Все разблокируем
					disableCtrls(0, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, CTL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_OPSYMB, 0L);
					// @v11.6.2 {
					const long idx_list[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
					DisableClusterItems(CTL_BIZPRCRT_MODIF, LongArray(idx_list, SIZEOFARRAY(idx_list)), 0);
					// } @v11.6.2 
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);
					// @v11.6.2 DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 0);
				}
				break;
			case DL2_Score::kCCheck:
				// Активируем
				disableCtrls(0, CTL_BIZPRCRT_PERIOD, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, 0L);
				// Блокируем
				disableCtrls(1, CTL_BIZPRCRT_OPSYMB, 0L);
				// Модифиакторы
				//disableCtrls(0, CTL_BIZPRCRT_MODIF, 0L);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 0, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);
				if(oneof4(Buf_Data.Sub, 6, 7, 8, 10)) {
					Buf_Data.Sub = 0;
					setCtrlData(CTL_BIZPRCRT_MODIF, &Buf_Data.Sub);
				}
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 1);
				break;
			case DL2_Score::kGoodsRest:
				// Активируем
				disableCtrls(0, CTL_BIZPRCRT_PERIOD, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, 0L);
				// Блокируем
				disableCtrls(1, CTL_BIZPRCRT_OPSYMB, 0L);
				// Модифиакторы
				//disableCtrls(0, CTL_BIZPRCRT_MODIF, 0L);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 0, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);

				if(oneof3(Buf_Data.Sub, 4, 5, 10)) {
					Buf_Data.Sub = 0;
					setCtrlData(CTL_BIZPRCRT_MODIF, &Buf_Data.Sub);
				}
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 1);
				break;
			case DL2_Score::kPersonEvent:
				// Все разблокируем
				disableCtrls(0, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, CTL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_OPSYMB, 0L);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 0);
				break;
			case DL2_Score::kDebt:
				// Активируем
				disableCtrls(0, CTL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_OPSYMB, 0L);
				// Блокируем
				disableCtrls(1, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, 0L);
				// Модифиакторы
				// Здесь сначала блокируем все, а потом разблокируем нужные
				//disableCtrls(1, CTL_BIZPRCRT_MODIF, 0L);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 0, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);

				if(oneof8(Buf_Data.Sub, 2, 3, 4, 5, 6, 7, 8, 10)) {
					Buf_Data.Sub = 0;
					setCtrlData(CTL_BIZPRCRT_MODIF, &Buf_Data.Sub);
				}
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 1);
				break;
			case DL2_Score::kBizScore:
				// Активируем
				disableCtrls(0, CTL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_OPSYMB, 0L);
				// Блокируем
				disableCtrls(1, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, 0L);
				// Модифиакторы
				// Здесь сначала блокируем все, а потом разблокируем нужные
				//disableCtrls(1, CTL_BIZPRCRT_MODIF, 0L);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 0, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 1, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 9, 0);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 10, 0);

				if(oneof7(Buf_Data.Sub, 2, 3, 4, 5, 6, 7, 8)) {
					Buf_Data.Sub = 0;
					setCtrlData(CTL_BIZPRCRT_MODIF, &Buf_Data.Sub);
				}
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 2, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 3, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 4, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 5, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 6, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 7, 1);
				DisableClusterItem(CTL_BIZPRCRT_MODIF, 8, 1);
				break;
			default:
				// Блокируем все
				disableCtrls(1, CTLSEL_BIZPRCRT_LOC, CTLSEL_BIZPRCRT_GOODSGRP, CTL_BIZPRCRT_PERIOD, CTL_BIZPRCRT_OPSYMB, CTL_BIZPRCRT_MODIF, 0L);
		}
	}
	DL2_Score Data;
	DL2_Score Buf_Data;
	StrAssocArray Primitiv_List;
	SString BizScoreName;
	SString Str_Buf;
	StringSet Str_Set;
	PPObjLocation Obj_Loc;
	LocationCtrlGroup::Rec Loc_Ctrl_Rec;
	LocationTbl::Rec Loc_Rec;
};
// } @vmiller

class BizScoreDialog : public TDialog {
	DECL_DIALOG_DATA(PPBizScorePacket);
public:
	BizScoreDialog() : TDialog(DLG_BIZSCORE)
	{
		PPObjBizScore bsc_obj;
		enableCommand(cmOK, bsc_obj.CheckRights(PPR_MOD));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		setCtrlLong(CTL_BIZSCORE_ID, Data.Rec.ID);
		setCtrlData(CTL_BIZSCORE_NAME, Data.Rec.Name);
		setCtrlData(CTL_BIZSCORE_SYMB, Data.Rec.Symb);
		SetupPPObjCombo(this, CTLSEL_BIZSCORE_USER, PPOBJ_USR, Data.Rec.UserID, 0, 0);
		setCtrlString(CTL_BIZSCORE_DESCR, Data.Descr);
		setCtrlString(CTL_BIZSCORE_FORMULA, Data.Formula);
		SetRealRangeInput(this, CTL_BIZSCORE_BOUNDS, &Data.Rec.Bounds);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_BIZSCORE_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_BIZSCORE_SYMB, Data.Rec.Symb);
		getCtrlData(CTLSEL_BIZSCORE_USER, &Data.Rec.UserID);
		getCtrlString(CTL_BIZSCORE_DESCR, Data.Descr);
		getCtrlString(CTL_BIZSCORE_FORMULA, Data.Formula);
		GetRealRangeInput(this, CTL_BIZSCORE_BOUNDS, &Data.Rec.Bounds);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmTest)) {
			SString result, reverse_formula;
			PPBizScorePacket test_pack;
			if(getDTS(&test_pack)) {
				BscObj.ReverseFormula(&test_pack, reverse_formula);
				setStaticText(CTL_BIZSCORE_REVTEXT, reverse_formula);
				BscObj.TestPacket(&test_pack, result);
			}
			setStaticText(CTL_BIZSCORE_TESTLINE, result);
		}
		// @vmiller
		else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_BIZSCORE_FORMULA)) {
			DL2_Score score;
			DL2_Resolver * p_resolver = 0;
			SETIFZ(p_resolver, new DL2_Resolver());
			score.Init(p_resolver);
			//if(PPDialogProcBody <BizPrimitivCreateDialog, DL2_Score> (&score) > 0) {
			BizPrimitivCreateDialog * dlg = new BizPrimitivCreateDialog(Data.Rec.Name);
			if(CheckDialogPtrErr(&dlg) && dlg->setDTS(&score)) {
				int r = -1;
				while(r <= 0 && ExecView(dlg) == cmOK)
					if(dlg->getDTS(&score)) {
						SString buf;
						score.PutToStr(buf);
						Data.Formula.Cat(buf);
						setCtrlString(CTL_BIZSCORE_FORMULA, Data.Formula);
						r = 1;
					}
			}
			//}
		}
		/* @v11.9.1 else if(event.isCtlEvent(CTL_BIZSCORE_FORMULA)) {
			getCtrlString(CTL_BIZSCORE_FORMULA, Data.Formula);
		}*/
		else
			return;
		clearEvent(event);
	}
	PPObjBizScore BscObj;
};

int PPObjBizScore::Edit(PPID * pID, void * extraPtr /*userID*/)
{
	const  PPID extra_user_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel;
	PPBizScorePacket pack;
	THROW(CheckRights(PPR_READ));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		THROW(CheckRights(PPR_INS));
		pack.Rec.UserID = extra_user_id;
	}
	if(PPDialogProcBody <BizScoreDialog, PPBizScorePacket> (&pack) > 0) {
		if(PutPacket(pID, &pack, 1))
			ok = cmOK;
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjBizScore::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = -1;
	if(sampleID > 0) {
		long   i;
		PPBizScorePacket sample_pack, pack;
		SString temp_buf;
		THROW(CheckRights(PPR_INS));
		THROW(GetPacket(sampleID, &sample_pack) > 0);
		pack = sample_pack;
		pack.Rec.ID = 0;
		temp_buf = sample_pack.Rec.Name;
		temp_buf.Trim(sizeof(pack.Rec.Name)-5);
		temp_buf.Space().CatChar('#');
		for(i = 1; i < 100; i++) {
			temp_buf.CatLongZ(i, 2);
			if(CheckDupName(0, temp_buf)) {
				temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				break;
			}
		}

		temp_buf = sample_pack.Rec.Symb;
		if(temp_buf.NotEmptyS()) {
			temp_buf.Trim(sizeof(pack.Rec.Symb)-5);
			temp_buf.Space().CatChar('#');
			for(i = 1; i < 100; i++) {
				temp_buf.CatLongZ(i, 2);
				if(CheckDupSymb(0, temp_buf)) {
					temp_buf.CopyTo(pack.Rec.Symb, sizeof(pack.Rec.Symb));
					break;
				}
			}
		}
		if(PPDialogProcBody <BizScoreDialog, PPBizScorePacket> (&pack) > 0)
			ok = PutPacket(pID, &pack, 1) ? 1 : PPErrorZ();
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjBizScore::Browse(void * extraPtr /*userID*/) { return PPView::Execute(PPVIEW_BIZSCORE, 0, 1, extraPtr /*userID*/); }
//
//
//
class BizScoreCache : public ObjCache {
public:
	BizScoreCache() : ObjCache(PPOBJ_BIZSCORE, sizeof(Data)) {}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		long   Flags;
		PPID   UserID;
		RealRange Bounds;
	};
};

int BizScoreCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjBizScore bs_obj;
	PPBizScorePacket pack;
	if(bs_obj.GetPacket(id, &pack) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=pack.Rec.Fld
		CPY_FLD(Flags);
		CPY_FLD(UserID);
		CPY_FLD(Bounds);
#undef CPY_FLD
		PPStringSetSCD ss;
		ss.add(pack.Rec.Name);
		ss.add(pack.Rec.Symb);
		ss.add(pack.Descr);
		ss.add(pack.Formula);
		PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void BizScoreCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPBizScorePacket * p_data_pack = static_cast<PPBizScorePacket *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
#define CPY_FLD(Fld) p_data_pack->Rec.Fld=p_cache_rec->Fld
	p_data_pack->Rec.Tag = PPOBJ_BIZSCORE;
	CPY_FLD(ID);
	CPY_FLD(Flags);
	CPY_FLD(UserID);
	CPY_FLD(Bounds);
#undef CPY_FLD
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	PPStringSetSCD ss;
	ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_pack->Rec.Name, sizeof(p_data_pack->Rec.Name));
	ss.get(&p, p_data_pack->Rec.Symb, sizeof(p_data_pack->Rec.Symb));
	ss.get(&p, p_data_pack->Descr);
	ss.get(&p, p_data_pack->Formula);
}

int PPObjBizScore::Fetch(PPID id, PPBizScorePacket * pRec)
{
	BizScoreCache * p_cache = GetDbLocalCachePtr <BizScoreCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : GetPacket(id, pRec);
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(BizScore); BizScoreFilt::BizScoreFilt() : PPBaseFilt(PPFILT_BIZSCORE, 0, 0)
{
	SetFlatChunk(offsetof(BizScoreFilt, ReserveStart),
		offsetof(BizScoreFilt, DescrPattern)-offsetof(BizScoreFilt, ReserveStart));
	SetBranchSString(offsetof(BizScoreFilt, DescrPattern));
	SetBranchSString(offsetof(BizScoreFilt, FormulaPattern));
	Init(1, 0);
}

BizScoreFilt & FASTCALL BizScoreFilt::operator = (const BizScoreFilt & s)
{
	Copy(&s, 1);
	return *this;
}
//
//
//
PPViewBizScore::PPViewBizScore() : PPView(&BscObj, &Filt, PPVIEW_BIZSCORE, implDontEditNullFilter, 0)
{
}

int PPViewBizScore::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class BizScoreFiltDialog : public TDialog {
	public:
		BizScoreFiltDialog() : TDialog(DLG_BIZSCOREFILT)
		{
		}
		int    setDTS(const BizScoreFilt * pFilt)
		{
			RVALUEPTR(Filt, pFilt);
			SetupPPObjCombo(this, CTLSEL_BIZSCOREFILT_USER, PPOBJ_USR, Filt.UserID, 0, 0);
			return 1;
		}
		int    getDTS(BizScoreFilt * pFilt)
		{
			int    ok = 1;
			getCtrlData(CTLSEL_BIZSCOREFILT_USER, &Filt.UserID);
			ASSIGN_PTR(pFilt, Filt);
			return ok;
		}
	private:
		BizScoreFilt  Filt;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	BizScoreFilt * p_filt = static_cast<BizScoreFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(BizScoreFiltDialog, p_filt);
}

int PPViewBizScore::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Helper_InitBaseFilt(pBaseFilt))
		ok = 1;
	else
		ok = 0;
	return ok;
}

int PPViewBizScore::InitIteration()
{
	return -1;
}

int FASTCALL PPViewBizScore::NextIteration(BizScoreViewItem * pItem)
{
	return -1;
}

DBQuery * PPViewBizScore::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_BIZSCORE;
	ReferenceTbl * t = new ReferenceTbl;
	DBE    dbe_user;
	DBE    dbe_descr;
	DBE    dbe_formula;
	DBQ  * dbq = 0;

	PPDbqFuncPool::InitObjNameFunc(dbe_user, PPDbqFuncPool::IdObjNameUser, t->Val1);
	{
		DBConst c_temp;
		dbe_descr.init();
		c_temp.init(PPOBJ_BIZSCORE);
		dbe_descr.push(c_temp);
		dbe_descr.push(t->ObjID);
		c_temp.init((long)BZSPRP_DESCR);
		dbe_descr.push(c_temp);
		c_temp.init((long)BIZSCEXSTR_DESCR);
		dbe_descr.push(c_temp);
		dbe_descr.push(static_cast<DBFunc>(PPDbqFuncPool::IdPropSubStr));

		dbe_formula.init();
		c_temp.init(PPOBJ_BIZSCORE);
		dbe_formula.push(c_temp);
		dbe_formula.push(t->ObjID);
		c_temp.init((long)BZSPRP_DESCR);
		dbe_formula.push(c_temp);
		c_temp.init((long)BIZSCEXSTR_FORMULA);
		dbe_formula.push(c_temp);
		dbe_formula.push(static_cast<DBFunc>(PPDbqFuncPool::IdPropSubStr));
	}
	dbq = &(t->ObjType == PPOBJ_BIZSCORE);
	dbq = ppcheckfiltid(dbq, t->Val1, Filt.UserID);

	q = & select(
		t->ObjID,    // #00
		t->ObjName,  // #01
		t->Symb,     // #02
		dbe_user,    // #03
		dbe_descr,   // #04
		dbe_formula, // #05
		0).from(t, 0).where(*dbq).orderBy(t->ObjType, t->ObjName, 0L);

	if(pSubTitle) {
		*pSubTitle = 0;
		if(Filt.UserID) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(Filt.UserID, &sec_rec) > 0)
				*pSubTitle = sec_rec.Name;
		}
	}
	/*
	CATCH
		ZDELETE(q);
	ENDCATCH
	*/
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewBizScore::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_USR, Filt.UserID, 0, 0);
	return -1;
}

int PPViewBizScore::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDBYSAMPLE:
				{
					const PPID sample_id = id;
					id = 0;
					ok = -1;
					if(sample_id && BscObj.AddBySample(&id, sample_id) > 0)
						ok = 1;
				}
				break;
			case PPVCMD_TB_CBX_SELECTED:
				ok = -1;
				{
					PPID   user_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&user_id) && Filt.UserID != user_id) {
						Filt.UserID = user_id;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_BIZSCOREVAL:
				ok = -1;
				if(id) {
					BizScoreValFilt filt;
					filt.BizScoreID = id;
					PPView::Execute(PPVIEW_BIZSCOREVAL, &filt, 1, 0);
				}
				break;
			case PPVCMD_CHARGE:
				ok = -1;
				if(DoBizScore(0) > 0)
					ok = 1;
				break;
		}
	}
	return ok;
}

void * PPViewBizScore::GetEditExtraParam() { return reinterpret_cast<void *>(Filt.UserID); }
//
//
//
BizScoreCore::BizScoreCore() : BizScoreTbl()
{
}

int BizScoreCore::Search(LDATE actualDate, PPID scID, PPID objID, BizScoreTbl::Rec * pRec)
{
	BizScoreTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ActualDate = actualDate;
	k0.ScoreID = scID;
	k0.ObjID = objID;
	return SearchByKey(this, 0, &k0, pRec);
}

int BizScoreCore::DeleteItem(LDATE actualDate, PPID scID, PPID objID, int use_ta)
{
	int    ok = -1;
	int    r;
	BizScoreTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Search(actualDate, scID, objID, &rec));
		if(r > 0) {
			THROW_DB(deleteRec());
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int BizScoreCore::SetItem(LDATE actualDate, PPID scID, PPID userID, long flags, double val, int use_ta)
{
	int    ok = -1;
	int    r;
	BizScoreTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Search(actualDate, scID, 0, &rec));
		if(r > 0) {
			if(val != rec.Val || flags != rec.Flags) {
				getcurdatetime(&rec.Dt, &rec.Tm);
				rec.UserID = userID;
				rec.Val = val;
				rec.Flags = flags;
				THROW_DB(updateRecBuf(&rec));
				ok = 1;
			}
		}
		else {
			rec.Clear();
			rec.ActualDate = actualDate;
			getcurdatetime(&rec.Dt, &rec.Tm);
			rec.UserID = userID;
			rec.ScoreID = scID;
			rec.Val = val;
			rec.Flags = flags;
			THROW_DB(insertRecBuf(&rec));
			ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int BizScoreCore::SetItem(LDATE actualDate, PPID scID, PPID userID, const char * pStr, int use_ta)
{
	int    ok = -1;
	int    r;
	BizScoreTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Search(actualDate, scID, 0, &rec));
		if(r > 0) {
			if(!sstreq(pStr, rec.Str)) {
				getcurdatetime(&rec.Dt, &rec.Tm);
				rec.UserID = userID;
				STRNSCPY(rec.Str, pStr);
				THROW_DB(updateRecBuf(&rec));
				ok = 1;
			}
		}
		else {
			rec.Clear();
			rec.ActualDate = actualDate;
			getcurdatetime(&rec.Dt, &rec.Tm);
			rec.UserID = userID;
			rec.ScoreID = scID;
			STRNSCPY(rec.Str, pStr);
			THROW_DB(insertRecBuf(&rec));
			ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int BizScoreCore::SetItem(LDATE actualDate, PPID scID, PPID userID, PPObjID obj, const char * pStr, int use_ta)
{
	int    ok = -1;
	int    r;
	BizScoreTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Search(actualDate, scID, obj.Id, &rec));
		if(r > 0) {
			if(obj.Id != rec.ObjID || !sstreq(pStr, rec.Str)) {
				getcurdatetime(&rec.Dt, &rec.Tm);
				rec.UserID = userID;
				rec.ObjType = obj.Obj;
				rec.ObjID = obj.Id;
				STRNSCPY(rec.Str, pStr);
				THROW_DB(updateRecBuf(&rec));
				ok = 1;
			}
		}
		else {
			rec.Clear();
			rec.ActualDate = actualDate;
			getcurdatetime(&rec.Dt, &rec.Tm);
			rec.UserID = userID;
			rec.ScoreID = scID;
			rec.ObjType = obj.Obj;
			rec.ObjID = obj.Id;
			STRNSCPY(rec.Str, pStr);
			THROW_DB(insertRecBuf(&rec));
			ok = 2;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class CreateBizScGblAcctDlg : public TDialog {
public:
	struct Param {
		SString UserName;
		SString Password;
	};

	CreateBizScGblAcctDlg() : TDialog(DLG_NEWACCT)
	{
	}
	int getDTS(CreateBizScGblAcctDlg::Param * pData)
	{
		int    ok = 1;
		getCtrlString(CTL_NEWACCT_USER, Data.UserName);
		if(Data.UserName.Len() == 0 || Data.Password.Len() == 0)
			ok = PPSetError(PPERR_INVUSERORPASSW);
		else
			ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmPassword)) {
 			char pwd[32];
			memzero(pwd, sizeof(pwd));
			if(PasswordDialog(0, pwd, sizeof(pwd), 3, 5) > 0)
				Data.Password = pwd;
		 	clearEvent(event);
		}
	}
	Param Data;
};

int CreateBizScGlblUserAcct()
{
	int    ok = -1;
	int    valid_data = 0;
	CreateBizScGblAcctDlg::Param data;
	CreateBizScGblAcctDlg * p_dlg = new CreateBizScGblAcctDlg;
	THROW(CheckDialogPtr(&p_dlg) > 0);
	for(;!valid_data && ExecView(p_dlg) == cmOK;) {
		if(p_dlg->getDTS(&data) > 0)
			ok = valid_data = 1;
		else {
			PPError();
			ok = -1;
		}
	}
	if(ok > 0) {
		ulong  crc = 0;
		SCRC32 crc32;
		SString out_buf;
		SString buf;
		SString path;
		SString sguid;
		SString file_name;
		SFile  file;
		S_GUID dbuuid;

		CurDict->GetDbUUID(&dbuuid);
		dbuuid.ToStr(S_GUID::fmtIDL, sguid);
		(file_name = data.UserName).CatChar('@').Cat(sguid).DotCat("acc");
		PPGetFilePath(PPPATH_OUT, file_name, path);
		THROW(file.Open(path, SFile::mWrite) && file.IsValid());

		buf.Cat(data.UserName).CR();
		buf.Cat(data.Password).CR();
		data.Password = 0;
		buf.Cat(sguid).CR();
		buf.Cat(LConfig.UserID).CR();

		IdeaEncrypt(0, (void *)buf.cptr(), buf.Len());
		out_buf.EncodeMime64(buf, buf.Len());
		crc = crc32.Calc(crc, out_buf.ucptr(), out_buf.Len());
		file.Write(&crc, sizeof(crc));
		file.WriteLine(out_buf);
		file.Close();
	}
	CATCHZOK
	data.Password = 0;
	return ok;
}

int GetBizScoresVals(const char * pUserName, const char * pPassword, TcpSocket * pSock)
{
	int    ok = 1;
	int    stop = 0;
	char   secret[64];
	SString buf;
	SString db_symb;
	SString bizsc_path;
	SString msg_buf;
	PPIniFile ini_file;
	PPVersionInfo vi = DS.GetVersionInfo();
	THROW_INVARG(pUserName && pPassword && pSock);
	ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BIZSCORE_DBSYMB, db_symb);
	ini_file.Get(PPINISECT_PATH, PPINIPARAM_BIZSCORES_PATH, bizsc_path);
	THROW(vi.GetSecret(secret, sizeof(secret)));
	THROW(DS.PPLogin(db_symb, PPSession::P_JobLogin, secret, PPSession::loginfSkipLicChecking));
	memzero(secret, sizeof(secret));
	{
		GlobalBizScoreArray bizsc_list;
		GlobalBizScoreCore bizsc_tbl;
		PPGlobalUserAcc user_acc;
		PPObjGlobalUserAcc glb_obj;
		//
		// Регистрация пользователей из найденных файлов
		//
		bizsc_path.SetLastSlash();
		{
			SString wildcard;
			SDirEntry entry;
			SDirec direc;
			SString path;
			(wildcard = bizsc_path).Cat("*@*.acc");
			for(direc.Init(wildcard); direc.Next(&entry) > 0;) {
				if(!entry.IsFolder()) {
					SFile file;
					entry.GetNameA(bizsc_path, path);
					if(file.Open(path, SFile::mRead) && file.IsValid()) {
						uint i = 0;
						uint32 crc = 0;
						uint32 check_crc = 0;
						PPID local_user_id = 0;
						StringSet ss("\n");
						SString user_info, user_name, pwd;
						S_GUID s_guid;
						file.CalcCRC(sizeof(long), &crc);
						file.Read(&check_crc, sizeof(check_crc));
						if(crc == check_crc) {
							while(file.ReadLine(buf) > 0)
								user_info.Cat(buf);
							Reference::Decrypt(Reference::crymRef2, user_info, user_info.Len(), buf);
							if(buf.Len() > 0) {
								ss.setBuf(buf, buf.Len() + 1);
								ss.get(&i, user_name);
								ss.get(&i, pwd);
								ss.get(&i, buf);
								s_guid.FromStr(buf);
								ss.get(&i, buf);
								local_user_id = buf.ToLong();
							}
							PPID    id;
							if(!glb_obj.Register(id, user_name, pwd, s_guid, local_user_id, 0))
								PPError();
						}
						else
							PPError(PPERR_FILECRCINVALID, path);
					}
					else
						PPError();
					file.Close();
					SFile::Remove(path);
				}
			}
		}
		THROW_PP(glb_obj.CheckPassword(pUserName, pPassword, &user_acc) > 0, PPERR_INVUSERORPASSW);
		//
		// Прием данных из xml файла, если он существует.
		//
		{
			SString str_guid;
			SString rpt_name;
			user_acc.LocalDbUuid.ToStr(S_GUID::fmtIDL, str_guid);
			bizsc_path.Cat(str_guid).SetLastSlash();
			THROW(SReport::GetReportAttributes(REPORT_BIZSCOREVALVIEW, 0, &rpt_name));
			bizsc_path.Cat(rpt_name).DotCat("xml");
			if(fileExists(bizsc_path)) {
				long   numrecs = 0;
				PPImpExpParam imp_exp_par;
				imp_exp_par.Init();
				THROW(LoadSdRecord(PPREC_BIZSCOREVAL, &imp_exp_par.InrRec));
				imp_exp_par.Direction  = 1;
				imp_exp_par.DataFormat = PPImpExpParam::dfXml;
				imp_exp_par.XdfParam.RootTag = rpt_name;
				imp_exp_par.XdfParam.RecTag  = "Iter";
				imp_exp_par.FileName.CopyFrom(bizsc_path);
				imp_exp_par.OtrRec = imp_exp_par.InrRec;
				{
					PPImpExp ie(&imp_exp_par, 0);
					THROW(ie.OpenFileForReading(0));
					ie.GetNumRecs(&numrecs);
					for(long i = 0; i < numrecs; i++) {
						S_GUID xml_guid;
						Sdr_BizScoreVal xml_rec;
						GlobalBizScoreVal * p_bizsc_rec = new GlobalBizScoreVal;
						THROW_MEM(p_bizsc_rec);
						memzero(p_bizsc_rec, sizeof(GlobalBizScoreVal));
						THROW(ie.ReadRecord(&xml_rec, sizeof(xml_rec)));
						xml_guid.FromStr(xml_rec.DbUuid);
						// THROW_PP(xml_guid == user_acc.LocalDbUuid, PPERR_INVDBGUIDINFILE);
						p_bizsc_rec->LocalDbUuid     = xml_guid;
						p_bizsc_rec->LocalUserID     = xml_rec.UserID;
						p_bizsc_rec->ActualDate      = xml_rec.ActualDate;
						p_bizsc_rec->Dtm.d   = xml_rec.Dt;
						strtotime(xml_rec.Tm, 0, &p_bizsc_rec->Dtm.t);
						p_bizsc_rec->LocalScoreID    = xml_rec.ScoreID;
						p_bizsc_rec->LocalScoreName  = xml_rec.ScoreName;
						p_bizsc_rec->LocalScoreDescr = xml_rec.ScoreDescr;
						p_bizsc_rec->Flags   = xml_rec.Flags;
						p_bizsc_rec->Val     = xml_rec.Val;
						p_bizsc_rec->StrVal  = xml_rec.Str;
						THROW_SL(bizsc_list.insert(p_bizsc_rec));
					}
				}
				THROW(bizsc_tbl.SetList(bizsc_list, 1));
				SFile::Remove(bizsc_path);
			}
		}
		//
		// Чтение данных из БД и их отправка
		//
		bizsc_list.freeAll();
		THROW(bizsc_tbl.GetLastList(user_acc.ID, &bizsc_list));
		buf.Cat(1L).CRB();
		THROW(pSock->Send(buf, buf.Len(), 0));
		if(bizsc_list.getCount()) {
			SString val;
			SString name;
			SString text;
			for(uint i = 0; i < bizsc_list.getCount(); i++) {
				GlobalBizScoreVal * p_bizsc_rec = bizsc_list.at(i);
				if(i == 0) {
					text.Z().Cat(p_bizsc_rec->Dtm).Cat("<br>");
					PPGetWord(PPWORD_CALCDATE, 1, buf);
					text.Cat(buf).CatDiv(':', 1).Cat(p_bizsc_rec->ActualDate).Cat("<br>").CRB();
					THROW(pSock->Send(text, text.Len(), 0));
				}
				name = (p_bizsc_rec->LocalScoreDescr.Len()) ? p_bizsc_rec->LocalScoreDescr : p_bizsc_rec->LocalScoreName;
				if(sstrlen(p_bizsc_rec->StrVal))
					val = p_bizsc_rec->StrVal;
				else
					val.Z().Cat(p_bizsc_rec->Val);
				if(p_bizsc_rec->Flags & BISCVF_BOUNDLOW)
					buf = "<FONT COLOR='#0000FF'>";
				else if(p_bizsc_rec->Flags & BISCVF_BOUNDUPP)
					buf = "<FONT COLOR='#FF0000'>";
				else
					buf = "<FONT>";
				buf.Cat(name).CatDiv(':', 2).Cat(val).Cat("</FONT>").CRB();
				THROW(pSock->Send(buf, buf.Len(), 0));
			}
		}
		else {
			PPLoadText(PPTXT_BIZSCORE_NODATA, buf);
			buf.CRB().Transf(CTRANSF_INNER_TO_OUTER);
			THROW(pSock->Send(buf, buf.Len(), 0));
		}
	}
	CATCH
		if(pSock) {
			SString buf2;
			buf.Cat(0L).CRB();
			THROW(pSock->Send(buf, buf.Len(), 0));
			PPGetLastErrorMessage(DS.CheckExtFlag(ECF_SYSSERVICE), buf);
			buf.Transf(CTRANSF_INNER_TO_OUTER);
			pSock->Send(buf, buf.Len(), 0);
		}
		ok = 0;
	ENDCATCH
	memzero(secret, sizeof(secret));
	DS.PPLogout();
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(BizScoreVal); BizScoreValFilt::BizScoreValFilt() : PPBaseFilt(PPFILT_BIZSCOREVAL, 0, 0)
{
	SetFlatChunk(offsetof(BizScoreValFilt, ReserveStart),
		offsetof(BizScoreValFilt, Reserve)-offsetof(BizScoreValFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

BizScoreValFilt & FASTCALL BizScoreValFilt::operator = (const BizScoreValFilt & s)
{
	Copy(&s, 1);
	return *this;
}
//
//
//
PPViewBizScoreVal::PPViewBizScoreVal() : PPView(0, &Filt, PPVIEW_BIZSCOREVAL, 0, REPORT_BIZSCOREVALVIEW)
{
}

PPViewBizScoreVal::~PPViewBizScoreVal()
{
}

PPBaseFilt * PPViewBizScoreVal::CreateFilt(const void * extraPtr) const
{
	BizScoreValFilt * p_filt = new BizScoreValFilt;
	p_filt->UserID = reinterpret_cast<long>(extraPtr);
	return p_filt;
}

int PPViewBizScoreVal::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class BizScoreValFiltDialog : public TDialog {
		DECL_DIALOG_DATA(BizScoreValFilt);
	public:
		BizScoreValFiltDialog() : TDialog(DLG_BIZSCVFILT)
		{
			SetupCalPeriod(CTLCAL_BIZSCVFILT_PERIOD, CTL_BIZSCVFILT_PERIOD);
			SetupCalDate(CTLCAL_BIZSCVFILT_DT, CTL_BIZSCVFILT_DT);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_BIZSCVFILT_PERIOD, Data.Period);
			setCtrlData(CTL_BIZSCVFILT_DT, &Data.Since.d);
			setCtrlData(CTL_BIZSCVFILT_TM, &Data.Since.t);
			SetupPPObjCombo(this, CTLSEL_BIZSCVFILT_USER, PPOBJ_USR, Data.UserID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_BIZSCVFILT_BSC,  PPOBJ_BIZSCORE, Data.BizScoreID, 0, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			THROW(GetPeriodInput(this, sel = CTL_BIZSCVFILT_PERIOD, &Data.Period));
			getCtrlData(CTL_BIZSCVFILT_DT, &Data.Since.d);
			getCtrlData(CTL_BIZSCVFILT_TM, &Data.Since.t);
			getCtrlData(CTLSEL_BIZSCVFILT_USER, &Data.UserID);
			getCtrlData(CTLSEL_BIZSCVFILT_BSC,  &Data.BizScoreID);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	BizScoreValFilt * p_filt = static_cast<BizScoreValFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(BizScoreValFiltDialog, p_filt);
}

int PPViewBizScoreVal::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		Filt.Period.Actualize(ZERODATE);
		ok = 1;
	}
	else
		ok = 0;
	return ok;
}

int PPViewBizScoreVal::CheckForFilt(const BizScoreTbl::Rec * pRec) const
{
	if(pRec) {
		if(!CheckFiltID(Filt.UserID, pRec->UserID))
			return 0;
		if(!CheckFiltID(Filt.BizScoreID, pRec->ScoreID))
			return 0;
		if(!Filt.Period.CheckDate(pRec->ActualDate))
			return 0;
		if(cmp(Filt.Since, pRec->Dt, pRec->Tm) > 0)
			return 0;
	}
	return 1;
}

int PPViewBizScoreVal::InitIteration()
{
	int    ok = 1;
	int    idx = 0;
	BizScoreTbl::Key0 k0, k0_;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	THROW_MEM(P_IterQuery = new BExtQuery(&Tbl, idx));
	P_IterQuery->selectAll();
	MEMSZERO(k0);
	if(Filt.Period.low) {
		k0.ActualDate = Filt.Period.low;
		if(Filt.BizScoreID)
			k0.ScoreID = Filt.BizScoreID;
	}
	k0_ = k0;
	Counter.Init(P_IterQuery->countIterations(0, &k0_, spGe));
	P_IterQuery->initIteration(false, &k0, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewBizScoreVal::NextIteration(BizScoreValViewItem * pItem)
{
	int    ok = -1;
	memzero(pItem, sizeof(*pItem));
	while(P_IterQuery && ok < 0 && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		if(CheckForFilt(&Tbl.data)) {
			Tbl.CopyBufTo(pItem);
			ok = 1;
		}
	}
	return ok;
}

int PPViewBizScoreVal::CalcTotal(BizScoreValTotal * pTotal)
{
	int    ok = 1;
	BizScoreValTotal total;
	total.Count = 0;
	total.Sum = 0.0;
	BizScoreValViewItem item;
	for(InitIteration(); NextIteration(&item) > 0;) {
		total.Count++;
		total.Sum += item.Val;
	}
	ASSIGN_PTR(pTotal, total);
	return ok;
}

int PPViewBizScoreVal::ViewGraph()
{
	int    ok = -1;
	if(Filt.BizScoreID) {
		SString temp_buf;
		PPObjBizScore bs_obj;
		PPBizScorePacket bs_pack;

		Generator_GnuPlot plot(0);
		Generator_GnuPlot::PlotParam param;
		param.Flags |= Generator_GnuPlot::PlotParam::fLines;
		if(bs_obj.Fetch(Filt.BizScoreID, &bs_pack) > 0)
			temp_buf = bs_pack.Rec.Name;
		else
			ideqvalstr(Filt.BizScoreID, temp_buf);
		param.Legend.Add(2, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		plot.Preamble();
		plot.SetDateTimeFormat(Generator_GnuPlot::axX);
		plot.SetGrid();
		plot.Plot(&param);
		plot.StartData(1);

		BizScoreValViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			plot.PutData(item.ActualDate);
			plot.PutData(item.Val);
			plot.PutEOR();
		}
		plot.PutEndOfData();
		if(!plot.Run())
			ok = 0;
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	if(pData && pStyle && col == 5) {
		const struct tag_Item {
			PPID   ScoreID;
			LDATE  ActualDate;
			PPID   ObjID;
			LDATETIME Dtm;
			double Val;
			char   Str[252];
		} * p_item = static_cast<const tag_Item *>(pData);
		if(p_item->ScoreID) {
			PPObjBizScore bs_obj;
			PPBizScorePacket bs_pack;
			pStyle->Flags = 0;
			if(bs_obj.Fetch(p_item->ScoreID, &bs_pack) > 0) {
				if(!bs_pack.Rec.Bounds.CheckVal(p_item->Val)) {
					if(p_item->Val < bs_pack.Rec.Bounds.low) {
						pStyle->Color = LightenColor(GetColorRef(SClrGreen), 0.5f);
						ok = 1;
					}
					else if(p_item->Val > bs_pack.Rec.Bounds.upp) {
						pStyle->Color = LightenColor(GetColorRef(SClrRed), 0.5f);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

void PPViewBizScoreVal::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, 0));
}

DBQuery * PPViewBizScoreVal::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q = 0;
	uint   brw_id = BROWSER_BIZSCOREVAL;
	DBQ  * dbq = 0;
	DBE    dbe_user;
	DBE    dbe_bizscore;
	BizScoreTbl * t = new BizScoreTbl;
	PPDbqFuncPool::InitObjNameFunc(dbe_user,     PPDbqFuncPool::IdObjNameUser, t->UserID);
	PPDbqFuncPool::InitObjNameFunc(dbe_bizscore, PPDbqFuncPool::IdObjNameBizScore, t->ScoreID);
	dbq = & daterange(t->ActualDate, &Filt.Period);
	dbq = ppcheckfiltid(dbq, t->UserID, Filt.UserID);
	dbq = ppcheckfiltid(dbq, t->ScoreID, Filt.BizScoreID);
	if(Filt.Since.d)
		dbq = &(*dbq && t->Dt >= Filt.Since.d);
	q = & select(
		t->ScoreID,    // #00
		t->ActualDate, // #01
		t->ObjID,      // #02
		t->Dt,         // #03
		t->Tm,         // #04
		t->Val,        // #05 // #07
		t->Str,        // #06 // #08
		dbe_bizscore,  // #07 // #05
		dbe_user,      // #08 // #06
		0).from(t, 0).where(*dbq).orderBy(t->ActualDate, t->ScoreID, 0L);
	if(pSubTitle) {
		*pSubTitle = 0;
		if(Filt.UserID) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(Filt.UserID, &sec_rec) > 0)
				pSubTitle->CatDivIfNotEmpty('-', 1).Cat(sec_rec.Name);
		}
		if(Filt.BizScoreID) {
			PPObjBizScore bs_obj;
			PPBizScorePacket bs_pack;
			if(bs_obj.Fetch(Filt.BizScoreID, &bs_pack) > 0)
				pSubTitle->CatDivIfNotEmpty('-', 1).Cat(bs_pack.Rec.Name);
		}
		if(!Filt.Period.IsZero()) {
			SString temp_buf;
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(PPFormatPeriod(&Filt.Period, temp_buf));
		}
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewBizScoreVal::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	struct Hdr {
		PPID  ScoreID;
		LDATE ActualDate;
		PPID  ObjID;
	};
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const Hdr * p_hdr = static_cast<const Hdr *>(pHdr);
		switch(ppvCmd) {
			case PPVCMD_VIEWBIZSCORE:
				ok = -1;
				if(p_hdr && p_hdr->ScoreID) {
					PPObjBizScore bs_obj;
					PPID   id = p_hdr->ScoreID;
					if(bs_obj.Edit(&id, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_CHARGE:
				ok = -1;
				if(DoBizScore(Filt.BizScoreID) > 0)
					ok = 1;
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(p_hdr && CONFIRM(PPCFM_DELETE)) {
					int r = Tbl.DeleteItem(p_hdr->ActualDate, p_hdr->ScoreID, p_hdr->ObjID, 1);
					if(!r)
						ok = PPErrorZ();
					else if(r > 0)
						ok = 1;
				}
				break;
			case PPVCMD_DELETEALL:
				ok = -1;
				if(CONFIRMCRIT(PPCFM_DELETE)) {
					BizScoreValViewItem item;
					for(InitIteration(); ok && NextIteration(&item) > 0;) {
						int r = Tbl.DeleteItem(item.ActualDate, item.ScoreID, item.ObjID, 1);
						if(!r)
							ok = PPErrorZ();
						else if(r > 0)
							ok = 1;
					}
				}
				break;
			case PPVCMD_GRAPH:
				ok = ViewGraph();
				break;
		}
	}
	return ok;
}

int CallbackBizScToFtpTransfer(long count, long total, const char * pMsg, int)
{
	PPWaitPercent(count, total, pMsg);
	return 1;
}

/*static*/int PPViewBizScoreVal::SendXml(PPID ftpAcctID, const char * pFilePath)
{
	int    ok = -1;
	SString ftp_path;
	WinInetFTP ftp;
	PPWaitStart();
	S_GUID guid;
	CurDict->GetDbUUID(&guid);
	guid.ToStr(S_GUID::fmtIDL, ftp_path);
	THROW_PP(ftp_path.Len() > 0, PPERR_DBGUIDUNDEF);
	THROW_PP(ftpAcctID, PPERR_FTPACCTUNDEF);
	{
		SString buf;
		SFsPath sp;
		PPInternetAccount acct;
		PPObjInternetAccount obj_acct;
		(buf = "\\\\").Cat(ftp_path).SetLastSlash();
		ftp_path = buf;
		sp.Split(pFilePath);
		THROW(obj_acct.Get(ftpAcctID, &acct));
		THROW(ftp.Init());
		THROW(ftp.Connect(&acct));
		if(!ftp.SafeCD(ftp_path, 1, 0)) {
			THROW(ftp.SafeCreateDir(ftp_path, 0));
			THROW(ftp.SafeCD(ftp_path, 1, 0));
		}
		ftp_path.SetLastSlash().Cat(sp.Nam).Dot().Cat(sp.Ext);
		THROW(ftp.SafePut(pFilePath, ftp_path, 0, CallbackBizScToFtpTransfer, 0));
		ok = 1;
	}
	CATCHZOK
	ftp.UnInit();
	PPWaitStop();
	return ok;
}
//
//
//
const uint32 PrcssrBizScore::Param::CVer = 2;

PrcssrBizScore::Param::Param()
{
	Init();
}

void PrcssrBizScore::Param::Init()
{
	THISZERO();
	Ver = CVer;
}

int PrcssrBizScore::Param::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	if(rBuf.GetAvailableSize()) {
		THROW(rBuf.Read(Ver));
		if(Ver == CVer) {
			THROW(rBuf.Read(FtpAcctID));
			THROW(rBuf.Read(Reserve, sizeof(Reserve)));
			THROW(rBuf.Read(BzsID));
			THROW(rBuf.Read(&Period, sizeof(Period)));
			THROW(rBuf.Read(Flags));
			THROW(rBuf.Read(Reserve2));
		}
		else if(Ver == 1) {
			THROW(rBuf.Read(FtpAcctID));
			THROW(rBuf.Read(Reserve, 28));
			BzsID = 0;
			THROW(rBuf.Read(&Period, sizeof(Period)));
			THROW(rBuf.Read(Flags));
			THROW(rBuf.Read(Reserve2));
		}
		else {
			Init();
			ok = -1;
		}
	}
	else
		ok = -1;
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

int PrcssrBizScore::Param::Write(SBuffer & rBuf, long)
{
	int    ok = 1;
	Ver = CVer;
	THROW(rBuf.Write(Ver));
	THROW(rBuf.Write(FtpAcctID));
	THROW(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW(rBuf.Write(BzsID));
	THROW(rBuf.Write(&Period, sizeof(Period)));
	THROW(rBuf.Write(Flags));
	THROW(rBuf.Write(Reserve2));
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

PrcssrBizScore::PrcssrBizScore() : P_Resolver(0)
{
}

PrcssrBizScore::~PrcssrBizScore()
{
	delete P_Resolver;
}

int PrcssrBizScore::InitParam(Param * pParam)
{
	CALLPTRMEMB(pParam, Init());
	return 1;
}

int PrcssrBizScore::EditParam(Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		TDialog * dlg = new TDialog(DLG_BIZSCPRC);
		Param data;
		if(CheckDialogPtrErr(&dlg)) {
			data = *pParam;
			dlg->SetupCalPeriod(CTLCAL_BIZSCPRC_PERIOD, CTL_BIZSCPRC_PERIOD);
			SetPeriodInput(dlg, CTL_BIZSCPRC_PERIOD, data.Period);
			dlg->AddClusterAssoc(CTL_BIZSCPRC_FLAGS, 0, Param::fExportXml);
			dlg->AddClusterAssoc(CTL_BIZSCPRC_FLAGS, 1, Param::fSendToFTP);
			dlg->SetClusterData(CTL_BIZSCPRC_FLAGS, data.Flags);
			SetupPPObjCombo(dlg, CTLSEL_BIZSCPRC_FTPACC, PPOBJ_INTERNETACCOUNT, data.FtpAcctID, 0,
				reinterpret_cast<void *>(PPObjInternetAccount::filtfFtp)/*INETACCT_ONLYFTP*/);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				if(!GetPeriodInput(dlg, CTL_BIZSCPRC_PERIOD, &data.Period))
					PPErrorByDialog(dlg, CTL_BIZSCPRC_PERIOD);
				else if(!data.Period.IsZero() && (!data.Period.low || !data.Period.upp))
					PPErrorByDialog(dlg, CTL_BIZSCPRC_PERIOD, PPERR_UNCLOSEDPERIOD);
				else {
					dlg->GetClusterData(CTL_BIZSCPRC_FLAGS, &data.Flags);
					dlg->getCtrlData(CTLSEL_BIZSCPRC_FTPACC, &data.FtpAcctID);
					*pParam = data;
					ok = 1;
				}
			}
		}
		else
			ok = 0;
		delete dlg;
	}
	else
		ok = 1;
	return ok;
}

int PrcssrBizScore::Init(const Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		DateRange p = pParam->Period;
		p.Actualize(ZERODATE);
		THROW_PP(p.IsZero() || (p.low && p.upp), PPERR_UNCLOSEDPERIOD);
		p.CheckAndSwap();
		P = *pParam;
		P.Period = p;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

struct __BizScoreStoreItem {
	LDATE  ActualDate;
	PPID   ID;
	PPID   UserID;
	long   Flags;
	double Val;
};

int PrcssrBizScore::Helper_Calc(LDATE actualDate, PPLogger & rLogger, int use_ta)
{
	int    ok = -1, r;
	uint   i;
	SString msg_buf;
	PPObjBizScore bs_obj;
	PPBizScore bs_rec;
	P_Resolver->SetActualDate(actualDate);
	TSArray <__BizScoreStoreItem> list;
	PPIDArray bzs_list;
	if(P.BzsID) {
		THROW_SL(bzs_list.add(P.BzsID));
	}
	else {
		for(SEnum en = bs_obj.P_Ref->Enum(PPOBJ_BIZSCORE, 0); en.Next(&bs_rec) > 0;) {
			THROW_SL(bzs_list.add(bs_rec.ID));
		}
	}
	for(i = 0; i < bzs_list.getCount(); i++) {
		const  PPID bzs_id = bzs_list.get(i);
		PPBizScorePacket bs_pack;
		THROW(PPCheckUserBreak());
		if(bs_obj.GetPacket(bzs_id, &bs_pack) > 0 && bs_pack.Formula.NotEmpty()) {
			msg_buf.Z().Cat(NZOR(actualDate, getcurdate_()));
			msg_buf.CatDiv('-', 1).Cat(bs_pack.Rec.Name).CatDiv('-', 1).Cat(bs_pack.Formula);
			PPWaitMsg(msg_buf);
			double val = P_Resolver->Resolve(bs_pack.Formula);
			long   fl = 0;
			if(bs_pack.Rec.Bounds.low != 0.0 && val < bs_pack.Rec.Bounds.low)
				fl |= BISCVF_BOUNDLOW;
			if(bs_pack.Rec.Bounds.upp != 0.0 && val > bs_pack.Rec.Bounds.upp)
				fl |= BISCVF_BOUNDUPP;
			{
				__BizScoreStoreItem item;
				MEMSZERO(item);
				item.ActualDate = actualDate;
				item.ID = bs_pack.Rec.ID;
				item.UserID = bs_pack.Rec.UserID;
				item.Flags = fl;
				item.Val = val;
				THROW_SL(list.insert(&item));
			}
		}
	}
	if(list.getCount()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < list.getCount(); i++) {
			const __BizScoreStoreItem & r_item = list.at(i);
			THROW(r = Tbl.SetItem(r_item.ActualDate, r_item.ID, r_item.UserID, r_item.Flags, r_item.Val, 0));
			if(r > 0)
				ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PrcssrBizScore::Run()
{
	int    ok = -1;
	int    r;
	PPLogger logger;
	SETIFZ(P_Resolver, new DL2_Resolver());
	SString msg_buf;
	PPWaitStart();
	if(P.Period.IsZero()) {
		THROW(r = Helper_Calc(getcurdate_(), logger, 1));
		if(r > 0)
			ok = 1;
	}
	else {
		THROW_PP(P.Period.low && P.Period.upp, PPERR_UNCLOSEDPERIOD);
		for(LDATE dt = P.Period.low; dt <= P.Period.upp; dt = plusdate(dt, 1)) {
			LDATETIME start = getcurdatetime_();
			THROW(r = Helper_Calc(dt, logger, 1));
			LDATETIME end = getcurdatetime_();
			//if(cmp(start, end) == 0) {
				//
				// Если расчет произведен слишком быстро, то
				// делаем искусственную задержку во избежании
				// дублирования индекса {Dt, Tm, ScoreID} таблицы BizScore
				//
				SDelay(20);
			//}
			if(r > 0)
				ok = 1;
		}
	}
	if(ok > 0) {
		DS.LogAction(PPACN_BIZSCOREUPDATED, PPOBJ_BIZSCORE, 0, 0, 1);
	}
	if(ok > 0) {
		if(P.Flags & Param::fExportXml) {
			PPViewBizScoreVal view;
			BizScoreValFilt filt;
			filt.Period = P.Period;
			THROW(view.Init_(&filt));
			THROW(view.ExportXml(REPORT_BIZSCOREVALVIEW, 0));
			if((P.Flags & Param::fSendToFTP)) {
				SString file_name;
				SString path;
				THROW(SReport::GetReportAttributes(REPORT_BIZSCOREVALVIEW, 0, &file_name));
				file_name.DotCat("xml");
				PPGetFilePath(PPPATH_OUT, file_name, path);
				THROW(view.SendXml(P.FtpAcctID, path));
			}
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int DoBizScore(PPID bzsID)
{
	int    ok = -1;
	PrcssrBizScore::Param p;
	PrcssrBizScore proc;
	proc.InitParam(&p);
	p.BzsID = bzsID;
	while(proc.EditParam(&p) > 0) {
		if(!proc.Init(&p))
			PPError();
		if(proc.Run()) {
			ok = 1;
			break;
		}
	}
	return ok;
}
//
//
//
GlobalBizScoreArray::GlobalBizScoreArray() : TSCollection <GlobalBizScoreVal>()
{
}

IMPL_CMPFUNC(GlobalBizScoreVal_wo_date, i1, i2)
{
	const GlobalBizScoreVal * p1 = static_cast<const GlobalBizScoreVal *>(i1);
	const GlobalBizScoreVal * p2 = static_cast<const GlobalBizScoreVal *>(i2);
	int s = memcmp(p1->LocalDbUuid.Data, p2->LocalDbUuid.Data, sizeof(S_GUID));
	if(s == 0) {
		s = cmp_long(p1->LocalUserID, p2->LocalUserID);
		if(s == 0)
			s = cmp_long(p1->LocalScoreID, p2->LocalScoreID);
	}
	return s;
}

int GlobalBizScoreArray::Add(const PPGlobalUserAcc & rGuaRec, const GlobalBizScoreTbl::Rec & rRec)
{
	int    ok = -1;
	int    do_add = 1;
	uint   pos = 0;
	const  LDATE curdt = getcurdate_();
	GlobalBizScoreVal * p_val = new GlobalBizScoreVal;
	THROW_MEM(p_val);
	p_val->LocalDbUuid = rGuaRec.LocalDbUuid;
	p_val->LocalUserID = rGuaRec.LocalUserID;
	p_val->ActualDate = rRec.ActualDate;
	p_val->Dtm.Set(rRec.Dt, rRec.Tm);
	p_val->LocalScoreID = rRec.LocalScoreID;
	p_val->LocalScoreName = rRec.ScoreName;
	p_val->LocalScoreDescr = rRec.ScoreDescr;
	p_val->Val = rRec.Val;
	p_val->Flags = rRec.Flags;
	p_val->StrVal = rRec.Str;
	while(lsearch(p_val, &pos, PTR_CMPFUNC(GlobalBizScoreVal_wo_date))) {
		const GlobalBizScoreVal * p_item = at(pos);
		long diff = diffdate(p_val->ActualDate, p_item->ActualDate);
		do_add = 0;
		if(p_val->ActualDate <= curdt) {
			//
			// Если расчетная дата нового элемента меньше или равна текущей,
			// то существующий элемент удаляется в том случае, если его расчетная //
			// дата меньше, чем у нового элемента (новый элемент "ближе" к текущему дню)
			// или больше текущей даты (новый элемент "более актуальный", так не пытается //
			// отражать будущее).
			//
			if(diff > 0 || p_item->ActualDate > curdt)
				do_add = 1;
		}
		else {
			//
			// Если расчетная дата нового элемента больше текущей,
			// то существующий элемент удаляется в том случае, если его расчетная //
			// дата больше, чем у нового элемента (новый элемент "ближе" к текущему дню)
			//
			if(diff < 0)
				do_add = 1;
		}
		if(do_add)
			atFree(pos);
		else
			pos++;
	}
	if(do_add) {
		THROW_SL(insert(p_val));
		ok = 1;
	}
	CATCHZOK
	if(ok <= 0)
		delete p_val;
	return ok;
}
//
//
//
GlobalBizScoreCore::GlobalBizScoreCore() : GlobalBizScoreTbl()
{
}

int GlobalBizScoreCore::GetLastList(PPID globalUserID, GlobalBizScoreArray * pList)
{
	int    ok = -1;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	LDATETIME since = ZERODATETIME;
	SysJournalTbl::Rec sj_rec;
	if(p_sj->GetLastEvent(PPACN_GLOBBIZSCOREUPD, 0/*extraVal*/, &since, 14, &sj_rec) > 0) {
		PPObjGlobalUserAcc gua_obj;
		PPGlobalUserAcc gua_rec;
		GlobalBizScoreVal val;
		for(long cntr = sj_rec.Extra; ok < 0 && cntr > 0; cntr--) {
			GlobalBizScoreTbl::Key2 k2;
			MEMSZERO(k2);
			k2.UpdCounter = cntr;
			k2.GlobalUserID = globalUserID;
			if(search(2, &k2, spGe) && k2.UpdCounter == cntr && k2.GlobalUserID == globalUserID) {
				ok = 1;
				do {
					GlobalBizScoreTbl::Rec rec;
					CopyBufTo(&rec);
					if(pList && gua_obj.Search(rec.GlobalUserID, &gua_rec) > 0) {
						THROW(pList->Add(gua_rec, rec));
					}
				} while(search(2, &k2, spNext) && k2.UpdCounter == sj_rec.Extra && k2.GlobalUserID == globalUserID);
			}
		}
	}
	CATCHZOK
	return ok;
}

int GlobalBizScoreCore::SetList(const GlobalBizScoreArray & rList, int use_ta)
{
	int    ok = -1;
	long   counter = 0;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	LDATETIME since = ZERODATETIME;
	SysJournalTbl::Rec sj_rec;
	if(p_sj->GetLastEvent(PPACN_GLOBBIZSCOREUPD, 0/*extraVal*/, &since, 7, &sj_rec) > 0)
		counter = sj_rec.Extra+1;
	else
		counter = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < rList.getCount(); i++) {
			THROW(SetItem(counter, *rList.at(i), 0));
			ok = 1;
		}
		if(ok > 0) {
			DS.LogAction(PPACN_GLOBBIZSCOREUPD, 0, 0, counter, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GlobalBizScoreCore::SetItem(long counter, const GlobalBizScoreVal & rVal, int use_ta)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	PPID   gua_id = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(gua_obj.SearchByLocalID(rVal.LocalDbUuid, rVal.LocalUserID, &gua_id, 0) > 0) {
			GlobalBizScoreTbl::Key0 k0;
			GlobalBizScoreTbl::Rec rec;
			k0.ActualDate = rVal.ActualDate;
			k0.GlobalUserID = gua_id;
			k0.LocalScoreID = rVal.LocalScoreID;
			if(SearchByKey(this, 0, &k0, &rec) > 0) {
				if(cmp(rVal.Dtm, rec.Dt, rec.Tm) > 0) {
					rec.UpdCounter = counter;
					rec.Dt = rVal.Dtm.d;
					rec.Tm = rVal.Dtm.t;
					rec.Val = rVal.Val;
					rec.Flags = rVal.Flags;
					rVal.LocalScoreName.CopyTo(rec.ScoreName, sizeof(rec.ScoreName));
					rVal.LocalScoreDescr.CopyTo(rec.ScoreDescr, sizeof(rec.ScoreDescr));
					rVal.StrVal.CopyTo(rec.Str, sizeof(rec.Str));
					THROW_DB(updateRecBuf(&rec));
				}
				else if(counter > rec.UpdCounter) {
					rec.UpdCounter = counter;
					THROW_DB(updateRecBuf(&rec));
				}
			}
			else {
				rec.Clear();
				rec.UpdCounter = counter;
				rec.ActualDate = rVal.ActualDate;
				rec.Dt = rVal.Dtm.d;
				rec.Tm = rVal.Dtm.t;
				rec.GlobalUserID = gua_id;
				rec.LocalScoreID = rVal.LocalScoreID;
				rec.Val = rVal.Val;
				rec.Flags = rVal.Flags;
				rVal.LocalScoreName.CopyTo(rec.ScoreName, sizeof(rec.ScoreName));
				rVal.LocalScoreDescr.CopyTo(rec.ScoreDescr, sizeof(rec.ScoreDescr));
				rVal.StrVal.CopyTo(rec.Str, sizeof(rec.Str));
				THROW_DB(insertRecBuf(&rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_BizScore
//
PPALDD_CONSTRUCTOR(BizScore)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

struct BizScoreSetBlock {
	BizScoreSetBlock() : State(0)
	{
	}
	enum {
		stInited = 0x0001
	};
	int    State;
	PPBizScorePacket Pack;
	PPBizScorePacket PreservePack;
};

PPALDD_DESTRUCTOR(BizScore)
{
	if(Extra[3].Ptr) {
		delete static_cast<BizScoreSetBlock *>(Extra[3].Ptr);
		Extra[3].Ptr = 0;
	}
	Destroy();
}

int PPALDD_BizScore::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		H.ID = rFilt.ID;
		PPObjBizScore bs_obj;
		PPBizScorePacket bs_pack;
		if(bs_obj.Fetch(H.ID, &bs_pack) > 0) {
			H.ID = bs_pack.Rec.ID;
			H.UserID = bs_pack.Rec.UserID;
			H.Flags  = bs_pack.Rec.Flags;
			STRNSCPY(H.Name, bs_pack.Rec.Name);
			STRNSCPY(H.Symb, bs_pack.Rec.Symb);
			bs_pack.Descr.CopyTo(H.Descr, sizeof(H.Descr));
			bs_pack.Formula.CopyTo(H.Formula, sizeof(H.Formula));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

int PPALDD_BizScore::Set(long iterId, int commit)
{
	int    ok = 1;
	PPObjBizScore bs_obj;
	SETIFZ(Extra[3].Ptr, new BizScoreSetBlock);
	BizScoreSetBlock * p_blk = static_cast<BizScoreSetBlock *>(Extra[3].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			//PPBizScorePacket
			// PPBizScorePacket
			p_blk->Pack.Rec.ID = H.ID;
			STRNSCPY(p_blk->Pack.Rec.Name, strip(H.Name));
			STRNSCPY(p_blk->Pack.Rec.Symb, strip(H.Symb));
			p_blk->Pack.Rec.UserID = H.UserID;
			p_blk->Pack.Rec.Flags = H.Flags;
			p_blk->Pack.Descr = H.Descr;
			p_blk->Pack.Formula = H.Formula;
			if(H.ID) {
				THROW(bs_obj.GetPacket(H.ID, &p_blk->PreservePack) > 0);
				if(p_blk->Pack.Rec.Name[0] == 0)
					STRNSCPY(p_blk->Pack.Rec.Name, p_blk->PreservePack.Rec.Name);
				if(p_blk->Pack.Rec.Symb[0] == 0)
					STRNSCPY(p_blk->Pack.Rec.Symb, p_blk->PreservePack.Rec.Symb);
			}
			// @todo Здесь проверить корректность полей
		}
		else {
			// @todo Ошибка (в этой структуре нет итераторов)
		}
	}
	else {
		// Здесь сохраняем пакет p_blk->Pack в БД
		PPID   id = p_blk->Pack.Rec.ID;
		THROW(bs_obj.PutPacket(&id, &p_blk->Pack, 1));
	}
	CATCHZOK
	if(commit) {
		delete p_blk;
		Extra[3].Ptr = 0;
	}
	return ok;
}
//
// Implementation of PPALDD_BizScoreValView
//
PPALDD_CONSTRUCTOR(BizScoreValView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(BizScoreValView) { Destroy(); }

int PPALDD_BizScoreValView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(BizScoreVal, rsrv);
	H.FltBeg     = p_filt->Period.low;
	H.FltEnd     = p_filt->Period.upp;
	H.SinceDate  = p_filt->Since.d;
	H.SinceTime  = p_filt->Since.t;
	H.FltUserID  = p_filt->UserID;
	H.FltScoreID = p_filt->BizScoreID;
	H.Flags      = p_filt->Flags;
	H.Order      = p_filt->Order;
	return DlRtm::InitData(rFilt, rsrv);
}

void PPALDD_BizScoreValView::Destroy() { DESTROY_PPVIEW_ALDD(BizScoreVal); }
int  PPALDD_BizScoreValView::InitIteration(long iterId, int sortId, long rsrv) { INIT_PPVIEW_ALDD_ITER(BizScoreVal); }

int  PPALDD_BizScoreValView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(BizScoreVal);
	I.ActualDate = item.ActualDate;
	I.Dt = item.Dt;
	I.Tm = item.Tm;
	{
		S_GUID dbuuid;
		DbProvider * p_dict = CurDict;
		CALLPTRMEMB(p_dict, GetDbUUID(&dbuuid));
		SString dbuuid_buf;
		dbuuid.ToStr(S_GUID::fmtIDL, dbuuid_buf).CopyTo(I.DbUUID, sizeof(I.DbUUID));
	}
	I.UserID     = item.UserID;
	I.ScoreID    = item.ScoreID;
	I.ObjType    = item.ObjType;
	I.ObjID      = item.ObjID;
	I.Val        = item.Val;
	STRNSCPY(I.Str, item.Str);
	FINISH_PPVIEW_ALDD_ITER();
}
//
// Implementation of PPALDD_GlobalUserAcc
//
struct GlobalUserAccBlock {
	enum {
		stFetch = 0,
		stSet
	};
	GlobalUserAccBlock() : State(stFetch)
	{
	}
	GlobalUserAccBlock & Z()
	{
		MEMSZERO(Rec);
		State = stFetch;
		return *this;
	}
	PPObjGlobalUserAcc GuaObj;
	PPGlobalUserAcc Rec;
	int    State;
};

PPALDD_CONSTRUCTOR(GlobalUserAcc)
{
	if(Valid) {
		Extra[0].Ptr = new GlobalUserAccBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
	}
}

PPALDD_DESTRUCTOR(GlobalUserAcc)
{
	Destroy();
	delete static_cast<GlobalUserAccBlock *>(Extra[0].Ptr);
}

int PPALDD_GlobalUserAcc::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	GlobalUserAccBlock & r_blk = *static_cast<GlobalUserAccBlock *>(Extra[0].Ptr);
	r_blk.Z();
	MEMSZERO(H);
	if(r_blk.GuaObj.Search(rFilt.ID, &r_blk.Rec) > 0) {
		H.ID = r_blk.Rec.ID;
		STRNSCPY(H.Name, r_blk.Rec.Name);
		STRNSCPY(H.Symb, r_blk.Rec.Symb);
		H.PersonID = r_blk.Rec.PersonID;
		H.LocalUserID = r_blk.Rec.LocalUserID;
		H.Flags = r_blk.Rec.Flags;
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

// @Muxa {
int PPALDD_GlobalUserAcc::Set(long iterId, int commit)
{
	int    ok = 1;
	GlobalUserAccBlock & r_blk = *static_cast<GlobalUserAccBlock *>(Extra[0].Ptr);
	if(r_blk.State != GlobalUserAccBlock::stSet) {
		r_blk.Z();
		r_blk.State = GlobalUserAccBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			STRNSCPY(r_blk.Rec.Name, strip(H.Name));
			STRNSCPY(r_blk.Rec.Symb, strip(H.Symb));
			STRNSCPY(r_blk.Rec.Password, strip(H.Password));
			r_blk.Rec.PersonID = H.PersonID;
		}
	}
	else {
		PPID   id = 0;
		S_GUID s_uid(SCtrGenerate_);
		THROW(r_blk.GuaObj.Register(id, r_blk.Rec.Name, r_blk.Rec.Password, s_uid, 0, r_blk.Rec.PersonID));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Z();
	return ok;
}

void PPALDD_GlobalUserAcc::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS) // @v11.3.7
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_GLOBALUSERACC, H.ID, _ARG_STR(1));
	}
}
//
// Implementation of PPALDD_UHTTStatistic
//
PPALDD_CONSTRUCTOR(UhttStatistic)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(UhttStatistic) { Destroy(); }

int PPALDD_UhttStatistic::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	{
		int    ggrps_n = 0;
		int    goods_n = 0;
		int    brands_n = 0;
		SString temp_buf;
		{
			PPID  id = 0;
			GoodsGroupIterator iter(0);
			while(iter.Next(&id, temp_buf) > 0)
				ggrps_n++;
		}
		{
			GoodsFilt      filt(0);
			GoodsIterator  iter(&filt, 0);
			Goods2Tbl::Rec rec;
			while(iter.Next(&rec) > 0)
				goods_n++;
		}
		{
			union {
				Goods2Tbl::Key2 k2;
				Goods2Tbl::Key4 k4;
			} k;
			MEMSZERO(k);
			GoodsCore gc;
			BExtQuery q(&gc, 2);
			DBQ * dbq = &(gc.Kind == PPGDSK_BRAND);
			k.k2.Kind = PPGDSK_BRAND;
			q.select(gc.ID, 0L).where(*dbq);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
				brands_n++;
		}
		MEMSZERO(H);
		H.ID = 1;
		H.GGroupsNumber = ggrps_n;
		H.GoodsNumber = goods_n;
		H.BrandsNumber = brands_n;
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}
// } @Muxa
//
// BizScore2
//
BizScore2ValuePacket::BizScore2ValuePacket()
{
}

BizScore2ValuePacket & BizScore2ValuePacket::Z()
{
	Rec.Clear();
	Text.Z();
	return *this;
}

bool FASTCALL BizScore2ValuePacket::IsEq(const BizScore2ValuePacket & rS) const
{
	bool   eq = true;
#define I(f) if(Rec.f != rS.Rec.f) return false;
	I(ID);
	I(ScoreID);
	I(ArID);
	I(Dt);
	I(PlanPeriodFn);
	I(Flags);
	I(AgentID);
	I(LinkObjID);
	I(IntVal);
	I(RealVal);
#undef I
	if(Text != rS.Text)
		eq = false;
	return eq;
}

BizScore2Core::BizScore2Core() : BizScore2Tbl()
{
}

int BizScore2Core::PutPacket(PPID * pID, BizScore2ValuePacket * pPack, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	constexpr PPID obj_type = PPOBJ_BIZSCORE2VAL;
	SString temp_buf;
	PPObjBizScore2 bs_obj;
	THROW(bs_obj.ValidateValuePacket(pPack));
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				BizScore2ValuePacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(pPack->IsEq(org_pack)) {
					ok = -1;
				}
				else {
					THROW(UpdateByID(this, obj_type, *pID, &pPack->Rec, 0));
					(temp_buf = pPack->Text).Strip().Transf(CTRANSF_INNER_TO_UTF8);
					THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(obj_type, *pID, PPTRPROP_BIZSCORE2_TEXT), temp_buf, 0));
					DS.LogAction(PPACN_OBJUPD, obj_type, *pID, 0, 0);
				}
			}
			else {
				THROW(RemoveByID(this, *pID, 0));
				THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(obj_type, *pID, PPTRPROP_BIZSCORE2_TEXT), temp_buf.Z(), 0));
				DS.LogAction(PPACN_OBJRMV, obj_type, *pID, 0, 0);
			}
		}
		else if(pPack) {
			*pID = pPack->Rec.ID;
			THROW(AddByID(this, pID, &pPack->Rec, 0));
			(temp_buf = pPack->Text).Strip().Transf(CTRANSF_INNER_TO_UTF8);
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(obj_type, *pID, PPTRPROP_BIZSCORE2_TEXT), temp_buf, 0));
			pPack->Rec.ID = *pID;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int BizScore2Core::GetPacket(PPID id, BizScore2ValuePacket * pPack)
{
	CALLPTRMEMB(pPack, Z());
	int    ok = -1;
	Reference * p_ref = PPRef;
	constexpr PPID obj_type = PPOBJ_BIZSCORE2VAL;
	BizScore2Tbl::Rec rec;
	const int sr = SearchByID(this, obj_type, id, &rec);
	if(sr > 0) {
		if(pPack) {
			pPack->Rec = rec;
			p_ref->UtrC.SearchUtf8(TextRefIdent(obj_type, id, PPTRPROP_BIZSCORE2_TEXT), pPack->Text);
			pPack->Text.Transf(CTRANSF_UTF8_TO_INNER);
		}
		ok = 1;
	}
	return ok;
}

PPBizScore2::PPBizScore2()
{
	THISZERO();
}

struct BscClsEntry {
	//
	// Descr: Флаги 
	//
	enum {
		fIncome       = 0x0001, // Доходных индикатор
		fExpense      = 0x0002, // Расходный индикатор
		fPerUnitValue = 0x0004  // Значение индикатора представлено в расчете на одну торговую единицу товара (произведение на количество будет аддитивно)     
	};
	long   Cls;
	int    Flags;     // BscClsEntry::fXXX
	int    Rank;      // Ранг класса для упорядочивания значений при пользовательском выводе. Чем меньше значение ранга, тем выше позиция показателя.
	int    TextId;    // Базовое текстовое описание 
	int    TextRId;   // Текстовое описание для результата расчета (в пересчете на контекстные параметры - количество, время etc)
};

static const BscClsEntry bsc_cls_list[] = {
	{ BSCCLS_INC_SALE_UNIT,         BscClsEntry::fIncome|BscClsEntry::fPerUnitValue,   1, PPTXT_BSCCLS_INC_SALE_UNIT,        PPTXT_BSCCLS_R_INC_SALE_UNIT        },
	{ BSCCLS_EXP_SALE_UNIT,         BscClsEntry::fExpense|BscClsEntry::fPerUnitValue,  2, PPTXT_BSCCLS_EXP_SALE_UNIT,        PPTXT_BSCCLS_R_EXP_SALE_UNIT        },
	{ BSCCLS_EXP_PURCHASE_UNIT,     BscClsEntry::fExpense|BscClsEntry::fPerUnitValue,  3, PPTXT_BSCCLS_EXP_PURCHASE_UNIT,    PPTXT_BSCCLS_R_EXP_PURCHASE_UNIT    },
	{ BSCCLS_EXP_TRANSFER_UNIT,     BscClsEntry::fExpense|BscClsEntry::fPerUnitValue,  4, PPTXT_BSCCLS_EXP_TRANSFER_UNIT,    PPTXT_BSCCLS_R_EXP_TRANSFER_UNIT    },
	{ BSCCLS_EXP_STORAGE_UNIT_TIME, BscClsEntry::fExpense,  5, PPTXT_BSCCLS_EXP_STORAGE_UNIT,     PPTXT_BSCCLS_R_EXP_STORAGE_UNIT     },
	{ BSCCLS_EXP_PRESALE_UNIT,      BscClsEntry::fExpense|BscClsEntry::fPerUnitValue,  6, PPTXT_BSCCLS_EXP_PRESALE_UNIT,     PPTXT_BSCCLS_R_EXP_PRESALE_UNIT     },
	{ BSCCLS_EXP_PRESALE_SKU_TIME,  BscClsEntry::fExpense,  7, PPTXT_BSCCLS_EXP_PRESALE_SKU_TIME, PPTXT_BSCCLS_R_EXP_PRESALE_SKU_TIME },
	{ BSCCLS_EXP_PROMO_SKU_TIME,    BscClsEntry::fExpense,  8, PPTXT_BSCCLS_EXP_PROMO_SKU_TIME,   PPTXT_BSCCLS_R_EXP_PROMO_SKU_TIME   },
	{ BSCCLS_EXP_PROMO_TIME,        BscClsEntry::fExpense,  9, PPTXT_BSCCLS_EXP_PROMO_TIME,       PPTXT_BSCCLS_R_EXP_PROMO_TIME       },
	{ BSCCLS_EXP_FUNDINGCOST,       BscClsEntry::fExpense, 10, PPTXT_BSCCLS_EXP_FUNDINGCOST,      PPTXT_BSCCLS_R_EXP_FUNDINGCOST      },
};

static int GetBscClsRank(long cls)
{
	int    rank = 0;
	if(cls) {
		for(uint i = 0; !rank && i < SIZEOFARRAY(bsc_cls_list); i++) {
			if(bsc_cls_list[i].Cls == cls) {
				rank = bsc_cls_list[i].Rank;
			}
		}
	}
	return NZOR(rank, 1000000);
}

IMPL_CMPFUNC(BscCls, i1, i2)
{
	const long * p_cls1 = static_cast<const long *>(i1);
	const long * p_cls2 = static_cast<const long *>(i2);
	int    rank1 = GetBscClsRank(*p_cls1);
	int    rank2 = GetBscClsRank(*p_cls2);
	return CMPSIGN(rank1, rank2);
}

static const BscClsEntry * SearchBscClsEntry(long cls)
{
	const BscClsEntry * p_result = 0;
	for(uint i = 0; !p_result && i < SIZEOFARRAY(bsc_cls_list); i++) {
		if(bsc_cls_list[i].Cls == cls) {
			p_result = bsc_cls_list+i;
		}
	}
	return p_result;
}

/*static*/uint PPObjBizScore2::GetBscClsList(PPIDArray & rList)
{
	rList.Z();
	for(uint i = 0; i < SIZEOFARRAY(bsc_cls_list); i++) {
		rList.add(bsc_cls_list[i].Cls);
	}
	return rList.getCount();
}

/*static*/bool PPObjBizScore2::IsBscCls_Income(long cls)
{
	const BscClsEntry * p_entry = SearchBscClsEntry(cls);
	return (p_entry && (p_entry->Flags & BscClsEntry::fIncome));
}

/*static*/bool PPObjBizScore2::IsBscCls_Expense(long cls)
{
	const BscClsEntry * p_entry = SearchBscClsEntry(cls);
	return (p_entry && (p_entry->Flags & BscClsEntry::fExpense));
}

/*static*/bool PPObjBizScore2::IsBscCls_PerUnitValue(long cls)
{
	const BscClsEntry * p_entry = SearchBscClsEntry(cls);
	return (p_entry && (p_entry->Flags & BscClsEntry::fPerUnitValue));
}

/*static*/bool PPObjBizScore2::GetBscClsName(long cls, SString & rBuf)
{
	rBuf.Z();
	bool   ok = false;
	const BscClsEntry * p_entry = SearchBscClsEntry(cls);
	if(p_entry && p_entry->TextId) {
		if(PPLoadText(p_entry->TextId, rBuf))
			ok = true;
	}
	return ok;
}

/*static*/bool PPObjBizScore2::GetBscClsResultName(long cls, SString & rBuf)
{
	rBuf.Z();
	bool   ok = false;
	const BscClsEntry * p_entry = SearchBscClsEntry(cls);
	if(p_entry && p_entry->TextRId) {
		if(PPLoadText(p_entry->TextRId, rBuf))
			ok = true;
	}
	return ok;
}

PPObjBizScore2::PPObjBizScore2(void * extraPtr) : PPObjReference(PPOBJ_BIZSCORE2, extraPtr)
{
	ImplementFlags |= (implStrAssocMakeList|implTreeSelector);
}
	
PPObjBizScore2::~PPObjBizScore2()
{
}

int PPObjBizScore2::GetPacket(PPID id, PPBizScore2Packet * pPack)
{
	int    ok = 1;
	PPBizScore2Packet pack;
	int    r = P_Ref->GetItem(Obj, id, &pack.Rec);
	THROW(r);
	if(r > 0) {
		SString text_buf;
		THROW(P_Ref->UtrC.SearchUtf8(TextRefIdent(Obj, id, PPTRPROP_BIZSCORE2), text_buf));
		text_buf.Transf(CTRANSF_UTF8_TO_INNER);
		pack.SetBuffer(text_buf.Strip());
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pPack, pack);
	return ok;
}

int PPObjBizScore2::IsPacketEq(const PPBizScore2Packet & rS1, const PPBizScore2Packet & rS2, long flags)
{
	const  int exfl[] = { PPBizScore2Packet::extssDescr, PPBizScore2Packet::extssMemo, PPBizScore2Packet::extssFormula };
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rS1.Rec.m, rS2.Rec.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMBS(Name);
	CMP_MEMBS(Symb);
	CMP_MEMB(Cls); // @v12.3.7
	CMP_MEMB(DataType);
	CMP_MEMB(TypeEnumID);
	CMP_MEMB(TypeEnumExt);
	CMP_MEMB(TimeAggrFunc);
	CMP_MEMB(HierAggrFunc);
	CMP_MEMB(AgentAggrFunc);
	CMP_MEMB(TimeCycle);
	CMP_MEMB(AgentPsnKindID);
	CMP_MEMB(LinkObjType);
	CMP_MEMB(LinkExtID);
	CMP_MEMB(Flags);
	CMP_MEMB(ParentID);
	CMP_MEMB(AccSheetID);
#undef CMP_MEMBS
#undef CMP_MEMB
	if(!rS1.PPExtStrContainer::IsEq(rS2, SIZEOFARRAY(exfl), exfl))
		return 0;
	return 1;
}

int PPObjBizScore2::PutPacket(PPID * pID, PPBizScore2Packet * pPack, int use_ta)
{
	int    ok = 1;
	PPID   action = 0;
	SString ext_buffer;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
			if(*pID) {
				int r;
				THROW(CheckRights(PPR_MOD));
				{
					PPBizScore2Packet org_pack;
					THROW(GetPacket(*pID, &org_pack) > 0);
					if(!IsPacketEq(*pPack, org_pack, 0)) {
						THROW(r = P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 0/*logAction*/, 0));
						{
							(ext_buffer = pPack->GetBuffer()).Strip();
							THROW(P_Ref->UtrC.SetTextUtf8(TextRefIdent(Obj, *pID, PPTRPROP_BIZSCORE2), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
						}
						action = PPACN_OBJUPD;
					}
					else
						ok = -1;
				}
			}
			else {
				THROW(CheckRights(PPR_INS));
				THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
				{
					(ext_buffer = pPack->GetBuffer()).Strip();
					THROW(P_Ref->UtrC.SetTextUtf8(TextRefIdent(Obj, *pID, PPTRPROP_BIZSCORE2), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
				}
				// Событие PPACN_OBJADD создано функцией P_Ref->AddItem : action не инициалазируем
			}
			{
				;
			}
		}
		else if(*pID) {
			THROW(CheckRights(PPR_DEL));
			//SETIFZ(P_ValTbl, new BizScore2Core);
			//THROW_MEM(P_ValTbl);
			//THROW_DB(deleteFrom(P_ValTbl, 0, P_ValTbl->ScoreID == *pID));
			THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			//THROW(P_Ref->PutPropVlrString(Obj, *pID, BZSPRP_DESCR, 0));
			THROW(P_Ref->UtrC.Remove(TextRefIdent(Obj, *pID, PPTRPROP_BIZSCORE2), 0));
			action = PPACN_OBJRMV;
		}
		DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

static void MakeBizScClsStrAssocList(StrAssocArray & rList)
{
	rList.Z();
	PPIDArray cls_id_list;
	PPObjBizScore2::GetBscClsList(cls_id_list);
	SString temp_buf;
	for(uint i = 0; i < cls_id_list.getCount(); i++) {
		const long cls_id = cls_id_list.get(i);
		assert(cls_id > 0);
		if(PPObjBizScore2::GetBscClsName(cls_id, temp_buf)) {
			assert(temp_buf.NotEmpty());
			rList.AddFast(cls_id, temp_buf);
		}
	}
}

class BizScore2Dialog : public TDialog {
	DECL_DIALOG_DATA(PPBizScore2Packet);
public:
	BizScore2Dialog() : TDialog(DLG_BIZSC2)
	{
		enableCommand(cmOK, BscObj.CheckRights(PPR_MOD));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		setCtrlLong(CTL_BIZSC2_ID, Data.Rec.ID);
		setCtrlData(CTL_BIZSC2_NAME, Data.Rec.Name);
		setCtrlData(CTL_BIZSC2_SYMB, Data.Rec.Symb);
		SetupPPObjCombo(this, CTLSEL_BIZSC2_PARENT, PPOBJ_BIZSCORE2, Data.Rec.ParentID, OLW_CANSELUPLEVEL);
		{
			StrAssocArray cls_list;
			MakeBizScClsStrAssocList(cls_list);
			cls_list.SortByText();
			SetupStrAssocCombo(this, CTLSEL_BIZSC2_CLS, cls_list, Data.Rec.Cls, 0);
		}
		AddClusterAssocDef(CTL_BIZSC2_DATATYPE, 0, OTTYP_NUMBER);
		AddClusterAssoc(CTL_BIZSC2_DATATYPE, 1, OTTYP_INT);
		AddClusterAssoc(CTL_BIZSC2_DATATYPE, 2, OTTYP_STRING);
		SetClusterData(CTL_BIZSC2_DATATYPE, Data.Rec.DataType);
		{
			PPIDArray obj_type_list;
			obj_type_list.addzlist(PPOBJ_GOODS, PPOBJ_PERSON, PPOBJ_BILL, 0);
			SetupObjListCombo(this, CTLSEL_BIZSC2_LINKOBJ, Data.Rec.LinkObjType, &obj_type_list);
		}
		SetupPPObjCombo(this, CTLSEL_BIZSC2_AGENTPSNK, PPOBJ_PERSONKIND, Data.Rec.AgentPsnKindID, 0);
		SetupStringCombo(this, CTLSEL_BIZSC2_AGGRFTM, PPTXT_AGGRFUNCNAMELIST, Data.Rec.TimeAggrFunc);
		SetupStringCombo(this, CTLSEL_BIZSC2_AGGRFHR, PPTXT_AGGRFUNCNAMELIST, Data.Rec.HierAggrFunc);
		SetupStringCombo(this, CTLSEL_BIZSC2_AGGRFAG, PPTXT_AGGRFUNCNAMELIST, Data.Rec.AgentAggrFunc);
		SetupStringCombo(this, CTLSEL_BIZSC2_TIMECYCLE, PPTXT_CYCLELIST, Data.Rec.TimeCycle);
		//
		Data.GetExtStrData(PPBizScore2Packet::extssDescr, temp_buf);
		setCtrlString(CTL_BIZSC2_DESCR, temp_buf);
		Data.GetExtStrData(PPBizScore2Packet::extssFormula, temp_buf);
		setCtrlString(CTL_BIZSC2_FORMULA, temp_buf);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTL_BIZSC2_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_BIZSC2_SYMB, Data.Rec.Symb);
		getCtrlData(CTLSEL_BIZSC2_PARENT, &Data.Rec.ParentID);
		getCtrlData(CTLSEL_BIZSC2_CLS, &Data.Rec.Cls);
		GetClusterData(CTL_BIZSC2_DATATYPE, &Data.Rec.DataType);
		getCtrlData(CTLSEL_BIZSC2_LINKOBJ, &Data.Rec.LinkObjType);
		getCtrlData(CTLSEL_BIZSC2_AGENTPSNK, &Data.Rec.AgentPsnKindID);
		getCtrlData(CTLSEL_BIZSC2_AGGRFTM, &Data.Rec.TimeAggrFunc);
		getCtrlData(CTLSEL_BIZSC2_AGGRFHR, &Data.Rec.HierAggrFunc);
		getCtrlData(CTLSEL_BIZSC2_AGGRFAG, &Data.Rec.AgentAggrFunc);
		getCtrlData(CTLSEL_BIZSC2_TIMECYCLE, &Data.Rec.TimeCycle);
		getCtrlString(CTL_BIZSC2_DESCR, temp_buf);
		Data.PutExtStrData(PPBizScore2Packet::extssDescr, temp_buf);
		getCtrlString(CTL_BIZSC2_FORMULA, temp_buf);
		Data.PutExtStrData(PPBizScore2Packet::extssFormula, temp_buf);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	PPObjBizScore2 BscObj;
};
	
/*virtual*/int PPObjBizScore2::Edit(PPID * pID, void * extraPtr /*userID*/)
{
	const  PPID extra_user_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel;
	PPBizScore2Packet pack;
	THROW(CheckRights(PPR_READ));
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		THROW(CheckRights(PPR_INS));
		//pack.Rec.UserID = extra_user_id;
	}
	if(PPDialogProcBody <BizScore2Dialog, PPBizScore2Packet> (&pack) > 0) {
		if(PutPacket(pID, &pack, 1))
			ok = cmOK;
		else
			PPError();
	}
	CATCHZOKPPERR
	return ok;
}

StrAssocArray * PPObjBizScore2::MakeStrAssocList(void * extraPtr)
{
	PPBizScore2 rec;
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		PPID   parent_id = rec.ParentID;
		THROW_SL(p_list->Add(rec.ID, parent_id, rec.Name));
	}
	p_list->SortByText();
	p_list->RemoveRecursion(0);
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

class PPObjBizScore2ListWindow : public PPObjListWindow {
public:
	static void * FASTCALL MakeExtraParam(PPID curId, void * extraPtr)
	{
		void * extra_ptr = extraPtr;
		return extra_ptr;
	}
	PPObjBizScore2ListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
	{
		DefaultCmd = cmaEdit;
		//SetToolbar(TOOLBAR_LIST_WORLD);
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
					case cmaInsert:
						id = 0;
						if(Flags & OLW_CANINSERT && P_Obj->Edit(&id, MakeExtraParam(id, ExtraPtr)) == cmOK)
							update = 2;
						else
							::SetFocus(H());
						break;
					case cmaMore:
						if(id) {
							/*
							long extra_param = PPObjWorld::MakeExtraParam(WORLDOBJ_STREET, parentID, 0);
							CheckExecAndDestroyDialog(new ObjWorldDialog((PPObjWorld *)ppobj, extra_param), 1, 1);
							*/
						}
					break;
				}
			}
			PostProcessHandleEvent(update, id);
		}
	}
};

/*virtual*/void * PPObjBizScore2::CreateObjListWin(uint flags, void * extraPtr)
{
	return new PPObjBizScore2ListWindow(this, flags, extraPtr);
}
	
/*virtual*/int PPObjBizScore2::Browse(void * extraPtr /*userID*/)
{
	return -1;
}

int PPObjBizScore2::ValidateValuePacket(const BizScore2ValuePacket * pValuePack)
{
	int    ok = 1;
	if(pValuePack) {
		PPBizScore2Packet sc_pack;
		THROW(oneof2(pValuePack->Rec.Kind, BizScore2ValuePacket::kRegular, BizScore2ValuePacket::kPlan)); // @todo @err
		if(pValuePack->Rec.Kind == BizScore2ValuePacket::kPlan) {
			THROW(checkdate(pValuePack->Rec.Dt) && checkdate(pValuePack->Rec.PlanPeriodFn)); // @todo @err
			THROW(pValuePack->Rec.Dt <= pValuePack->Rec.PlanPeriodFn); // @todo @err
		}
		else if(pValuePack->Rec.Kind == BizScore2ValuePacket::kRegular) {
			THROW(checkdate(pValuePack->Rec.Dt) ); // @todo @err
		}
		THROW(pValuePack->Rec.ScoreID); // @todo @err
		THROW(GetPacket(pValuePack->Rec.ScoreID, &sc_pack) > 0);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjBizScore2::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options/* = rmv_default*/, void * pExtraParam)
{
	int    r = -1;
	int    conf = 1;
	THROW(CheckRights(PPR_DEL));
	/*
	SETIFZ(P_ValTbl, new BizScoreCore);
	THROW_MEM(P_ValTbl);
	if(options & PPObject::user_request) {
		BizScoreTbl::Key1 k1;
		MEMSZERO(k1);
		k1.ScoreID = id;
		if(P_ValTbl->search(1, &k1, spGe) && k1.ScoreID == id)
			conf = CONFIRM(PPCFM_DELBIZSCORE);
		else
			conf = CONFIRM(PPCFM_DELETE);
	}
	*/
	if(conf) {
		PPWaitStart();
		THROW(PutPacket(&id, 0, BIN(options & PPObject::use_transaction)));
		r = 1;
	}
	CATCH
		r = 0;
		if(options & PPObject::user_request)
			PPError();
	ENDCATCH
	PPWaitStop();
	return r;
}

int PPObjBizScore2::EditValuePacketDialog(BizScore2ValuePacket * pValuePack)
{
	class BizScore2ValueDialog : public TDialog {
		DECL_DIALOG_DATA(BizScore2ValuePacket);
	public:
		BizScore2ValueDialog() : TDialog(DLG_BIZSC2V)
		{
			SetupCalDate(CTLCAL_BIZSC2V_DT, CTL_BIZSC2V_DT);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			//
			SetupPPObjCombo(this, CTLSEL_BIZSC2V_BSC, PPOBJ_BIZSCORE2, Data.Rec.ScoreID, 0);
			setCtrlDate(CTL_BIZSC2V_DT, Data.Rec.Dt);
			setCtrlTime(CTL_BIZSC2V_TM, Data.Rec.Tm);
			SetupIndicator();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_BIZSC2V_BSC, &Data.Rec.ScoreID);
			getCtrlData(CTL_BIZSC2V_DT, &Data.Rec.Dt);
			getCtrlData(CTL_BIZSC2V_TM, &Data.Rec.Tm);
			{
				PPBizScore2Packet pack;
				if(Data.Rec.ScoreID && Obj.GetPacket(Data.Rec.ScoreID, &pack) > 0) {
					if(pack.Rec.DataType == OTTYP_NUMBER) {
						getCtrlData(CTL_BIZSC2V_VAL_R, &Data.Rec.RealVal);
					}
					else if(pack.Rec.DataType == OTTYP_INT) {
						getCtrlData(CTL_BIZSC2V_VAL_I, &Data.Rec.IntVal);
					}
					else if(pack.Rec.DataType == OTTYP_STRING) {
						getCtrlString(CTL_BIZSC2V_VAL_T, Data.Text);
					}
					if(pack.Rec.AgentPsnKindID) {
						getCtrlData(CTLSEL_BIZSC2V_AGENT, &Data.Rec.AgentID);
					}
					else
						Data.Rec.AgentID = 0;
				}
			}
			//
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_BIZSC2V_BSC)) {
				SetupIndicator();
			}
			else
				return;
			clearEvent(event);
		}
		void    SetupIndicator()
		{
			const PPID preserve_score_id = Data.Rec.ScoreID;
			bool  hide_all_value_ctrls = true;
			PPBizScore2Packet pack;
			Data.Rec.ScoreID = getCtrlLong(CTLSEL_BIZSC2V_BSC);
			if(Data.Rec.ScoreID && Obj.GetPacket(Data.Rec.ScoreID, &pack) > 0) {
				if(pack.Rec.DataType == OTTYP_NUMBER) {
					showCtrl(CTL_BIZSC2V_VAL_R, true);
					showCtrl(CTL_BIZSC2V_VAL_I, false);
					showCtrl(CTL_BIZSC2V_VAL_T, false);
					setCtrlReal(CTL_BIZSC2V_VAL_R, Data.Rec.RealVal);
					hide_all_value_ctrls = false;
				}
				else if(pack.Rec.DataType == OTTYP_INT) {
					showCtrl(CTL_BIZSC2V_VAL_R, false);
					showCtrl(CTL_BIZSC2V_VAL_I, true);
					showCtrl(CTL_BIZSC2V_VAL_T, false);
					setCtrlLong(CTL_BIZSC2V_VAL_I, Data.Rec.IntVal);
					hide_all_value_ctrls = false;
				}
				else if(pack.Rec.DataType == OTTYP_STRING) {
					showCtrl(CTL_BIZSC2V_VAL_R, false);
					showCtrl(CTL_BIZSC2V_VAL_I, false);
					showCtrl(CTL_BIZSC2V_VAL_T, true);
					setCtrlString(CTL_BIZSC2V_VAL_T, Data.Text);
					hide_all_value_ctrls = false;
				}
			}
			if(hide_all_value_ctrls) {
				showCtrl(CTL_BIZSC2V_VAL_R, false);
				showCtrl(CTL_BIZSC2V_VAL_I, false);
				showCtrl(CTL_BIZSC2V_VAL_T, false);
			}
			SetupPersonCombo(this, CTLSEL_BIZSC2V_AGENT, Data.Rec.AgentID, 0, pack.Rec.AgentPsnKindID, 1);
		}
		PPObjBizScore2 Obj;
	};
	return PPDialogProcBody <BizScore2ValueDialog, BizScore2ValuePacket> (pValuePack);
}

class BizScore2Cache : public ObjCache {
public:
	BizScore2Cache() : ObjCache(PPOBJ_BIZSCORE2, sizeof(Data)) {}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		int32  DataType;
		long   TypeEnumID;
		long   TypeEnumExt;
		int16  TimeAggrFunc;
		int16  HierAggrFunc;
		int16  AgentAggrFunc;
		int16  TimeCycle;
		long   AgentPsnKindID;
		long   LinkObjType;
		long   LinkExtID;
		long   Flags;
		long   ParentID;
		long   AccSheetID;
		long   Cls; // @v12.3.7
	};
};

int BizScore2Cache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjBizScore2 bs_obj;
	PPBizScore2Packet pack;
	if(bs_obj.GetPacket(id, &pack) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=pack.Rec.Fld
		CPY_FLD(DataType);
		CPY_FLD(TypeEnumID);
		CPY_FLD(TypeEnumExt);
		CPY_FLD(TimeAggrFunc);
		CPY_FLD(HierAggrFunc);
		CPY_FLD(AgentAggrFunc);
		CPY_FLD(TimeCycle);
		CPY_FLD(AgentPsnKindID);
		CPY_FLD(LinkObjType);
		CPY_FLD(LinkExtID);
		CPY_FLD(Flags);
		CPY_FLD(ParentID);
		CPY_FLD(AccSheetID);
		CPY_FLD(Cls); // @v12.3.7
#undef CPY_FLD
		SString temp_buf;
		PPStringSetSCD ss;
		ss.add(pack.Rec.Name);
		ss.add(pack.Rec.Symb);
		{
			pack.GetExtStrData(PPBizScore2Packet::extssFormula, temp_buf);
			ss.add(temp_buf);
		}
		PutName(ss.getBuf(), p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void BizScore2Cache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPBizScore2Packet * p_data_pack = static_cast<PPBizScore2Packet *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
#define CPY_FLD(Fld) p_data_pack->Rec.Fld=p_cache_rec->Fld
	p_data_pack->Rec.Tag = PPOBJ_BIZSCORE2;
	CPY_FLD(ID);
	CPY_FLD(DataType);
	CPY_FLD(TypeEnumID);
	CPY_FLD(TypeEnumExt);
	CPY_FLD(TimeAggrFunc);
	CPY_FLD(HierAggrFunc);
	CPY_FLD(AgentAggrFunc);
	CPY_FLD(TimeCycle);
	CPY_FLD(AgentPsnKindID);
	CPY_FLD(LinkObjType);
	CPY_FLD(LinkExtID);
	CPY_FLD(Flags);
	CPY_FLD(ParentID);
	CPY_FLD(AccSheetID);
	CPY_FLD(Cls); // @v12.3.7
#undef CPY_FLD
	char   temp_buf[2048];
	GetName(pEntry, temp_buf, sizeof(temp_buf));
	PPStringSetSCD ss;
	ss.setBuf(temp_buf, sstrlen(temp_buf)+1);
	uint   p = 0;
	ss.get(&p, p_data_pack->Rec.Name, sizeof(p_data_pack->Rec.Name));
	ss.get(&p, p_data_pack->Rec.Symb, sizeof(p_data_pack->Rec.Symb));
	{
		ss.get(&p, temp_buf, sizeof(temp_buf));
		if(temp_buf[0])
			p_data_pack->PutExtStrData(PPBizScore2Packet::extssFormula, temp_buf);
	}
}

int PPObjBizScore2::Fetch(PPID id, PPBizScore2Packet * pRec)
{
	BizScore2Cache * p_cache = GetDbLocalCachePtr <BizScore2Cache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : GetPacket(id, pRec);
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(BizSc2Val); BizSc2ValFilt::BizSc2ValFilt() : PPBaseFilt(PPFILT_BIZSC2VAL, 0, 0)
{
	SetFlatChunk(offsetof(BizSc2ValFilt, ReserveStart),
		offsetof(BizSc2ValFilt, ReserveEnd)-offsetof(BizSc2ValFilt, ReserveStart)+sizeof(ReserveEnd));
	SetBranchSString(offsetof(BizSc2ValFilt, ExtString));
	//
	Init(1, 0);
}

BizSc2ValFilt & FASTCALL BizSc2ValFilt::operator = (const BizSc2ValFilt & rS)
{
	PPBaseFilt::Copy(&rS, 0);
	return *this;
}

PPViewBizSc2Val::BrwItem::BrwItem()
{
	THISZERO();
}

PPViewBizSc2Val::PPViewBizSc2Val() : PPView(0, &Filt, PPVIEW_BIZSC2VAL, implBrowseArray, 0), P_DsList(0)
{
}

PPViewBizSc2Val::~PPViewBizSc2Val()
{
	ZDELETE(P_DsList);
	DBRemoveTempFiles();
}

/*virtual*/PPBaseFilt * PPViewBizSc2Val::CreateFilt(const void * extraPtr) const
{
	BizSc2ValFilt * p_filt = new BizSc2ValFilt;
	return p_filt;
}

int PPViewBizSc2Val::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class BizSc2ValFiltDialog : public TDialog {
	public:
		BizSc2ValFiltDialog() : TDialog(DLG_BIZSC2VFILT)
		{
			SetupCalPeriod(CTLCAL_BIZSC2VFILT_PERIOD, CTL_BIZSC2VFILT_PERIOD);
		}
		int    setDTS(const BizSc2ValFilt * pData)
		{
			RVALUEPTR(Data, pData);
			SetupPPObjCombo(this, CTLSEL_BIZSC2VFILT_BSC, PPOBJ_BIZSCORE2, Data.ScID, 0);
			//
			return 1;
		}
		int    getDTS(BizSc2ValFilt * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(CTLSEL_BIZSC2VFILT_BSC, &Data.ScID);
			//
			ASSIGN_PTR(pData, Data);
			//CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		BizSc2ValFilt Data;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	BizSc2ValFilt * p_filt = static_cast<BizSc2ValFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(BizSc2ValFiltDialog, p_filt);
}

/*virtual*/int PPViewBizSc2Val::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

int PPViewBizSc2Val::MakeListEntry(const BizScore2Tbl::Rec & rRec, BrwItem & rEntry)
{
	int    ok = 1;
	MEMSZERO(rEntry);
	rEntry.ID = rRec.ID;
	rEntry.ScoreID = rRec.ScoreID;
	rEntry.Dt = rRec.Dt;
	rEntry.AgentID = rRec.AgentID;
	rEntry.ArID = rRec.ArID;
	rEntry.Flags = rRec.Flags;
	rEntry.LinkObjID = rRec.LinkObjID;
	rEntry.PlanPeriodFn = rRec.PlanPeriodFn;
	rEntry.RVal = rRec.RealVal;
	rEntry.IVal = rRec.IntVal;
	{
		SString temp_buf;
		PPRef->UtrC.SearchUtf8(TextRefIdent(PPOBJ_BIZSCORE2VAL, rRec.ID, PPTRPROP_BIZSCORE2_TEXT), temp_buf);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(temp_buf.NotEmptyS()) {
			StrPool.AddS(temp_buf, &rEntry.TextP);
		}
	}
	return ok;
}

int PPViewBizSc2Val::MakeList(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	{
		union {;
			BizScore2Tbl::Key1 k1;
			BizScore2Tbl::Key2 k2;
		} k;
		int    idx = 1;
		MEMSZERO(k);
		DBQ * p_dbq = &(BscT.Kind == 0);
		if(Filt.ScID) {
			p_dbq = ppcheckfiltid(p_dbq, BscT.ScoreID, Filt.ScID);
			idx = 2;
			k.k2.ScoreID = Filt.ScID;
		}
		BExtQuery q(&BscT, idx);
		q.where(*p_dbq);
		q.selectAll();
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			BrwItem new_item;
			if(MakeListEntry(BscT.data, new_item) > 0) {
				P_DsList->insert(&new_item);
			}
		}
	}
	return ok;
}

int PPViewBizSc2Val::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
	SString temp_buf;
	PPBizScore2Packet bs_pack;
	const int fr = BscObj.Fetch(p_item->ScoreID, &bs_pack);
	switch(pBlk->ColumnN) {
		case 0: pBlk->Set(p_item->ID); break; // @id
		case 1: // @date
			pBlk->Set(p_item->Dt); 
			break; 
		case 2: // @appellation
			pBlk->Set(bs_pack.Rec.Name);
			break; 
		case 3:  // @value
			temp_buf.Z();
			if(bs_pack.Rec.DataType == OTTYP_NUMBER) {
				temp_buf.Cat(p_item->RVal, MKSFMTD(0, 6, 0));
			}
			else if(bs_pack.Rec.DataType == OTTYP_INT) {
				temp_buf.Cat(p_item->IVal);
			}
			else if(bs_pack.Rec.DataType == OTTYP_STRING) {
				StrPool.GetS(p_item->TextP, temp_buf);
			}
			pBlk->Set(temp_buf);
			break;
	}
	return ok;
}

/*virtual*/SArray * PPViewBizSc2Val::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(p_array);
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_BIZSCORE2VAL);
	return p_array;
}

/*virtual*/DBQuery * PPViewBizSc2Val::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * p_result = 0;
	return p_result;
}

/*virtual*/void PPViewBizSc2Val::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewBizSc2Val *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
	}
}
	
/*virtual*/int PPViewBizSc2Val::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
	const  BrwItem * p_item = pHdr ? static_cast<const BrwItem *>(pHdr) : 0;
	PPID   new_id = 0;
	PPID   update_id = 0;
	PPID   cur_id = p_item ? p_item->ID : 0;
	if(ok == -2) {
		const  long cur_pos = p_def ? p_def->_curItem() : 0;
		long   update_pos = cur_pos;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				{
					const LDATETIME now_dtm = getcurdatetime_();
					BizScore2ValuePacket pack;
					pack.Rec.Dt = now_dtm.d;
					pack.Rec.Tm = now_dtm.t;
					if(BscObj.EditValuePacketDialog(&pack) > 0) {
						if(BscT.PutPacket(&new_id, &pack, 1)) {
							update_id = new_id;
							ok = 1;
						}
					}
				}
				break;
			case PPVCMD_EDITITEM:
				if(cur_id) {
					BizScore2ValuePacket pack;
					if(BscT.GetPacket(cur_id, &pack) > 0) {
						if(BscObj.EditValuePacketDialog(&pack) > 0) {
							if(BscT.PutPacket(&cur_id, &pack, 1)) {
								update_id = cur_id;
								ok = 1;
							}
						}
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				if(cur_id) {
					if(BscT.PutPacket(&cur_id, 0, 1)) {
						update_id = 0;
						ok = 1;
					}
				}
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				int    update_pos = -1;
				uint   temp_pos = 0;
				if(update_id > 0 && P_DsList->lsearch(&cur_id, &temp_pos, CMPF_LONG))
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
