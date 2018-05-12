// V_TSANLZ.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2010, 2013, 2014, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewTSessAnlz)
//
IMPLEMENT_PPFILT_FACTORY(TSessAnlz); SLAPI TSessAnlzFilt::TSessAnlzFilt() : PPBaseFilt(PPFILT_TSESSANLZ, 0, 0)
{
	P_TSesFilt = 0;
	SetFlatChunk(offsetof(TSessAnlzFilt, ReserveStart),
		offsetof(TSessAnlzFilt, SessIdList)-offsetof(TSessAnlzFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(TSessAnlzFilt, SessIdList));
	SetBranchBaseFiltPtr(PPFILT_TSESSION, offsetof(TSessAnlzFilt, P_TSesFilt));
	Init(1, 0);
}

TSessAnlzFilt & FASTCALL TSessAnlzFilt::operator = (const TSessAnlzFilt & s)
{
	Copy(&s, 1);
	return *this;
}

void SLAPI TSessAnlzFilt::SetOuterTSessFilt(const TSessionFilt * pTSesFilt)
{
	if(pTSesFilt) {
		P_TSesFilt = new TSessionFilt;
		*P_TSesFilt = *pTSesFilt;
	}
	else
		ZDELETE(P_TSesFilt);
}

const TSessionFilt * SLAPI TSessAnlzFilt::GetOuterTSessFilt() const { return P_TSesFilt; }
int SLAPI TSessAnlzFilt::IsDiffFlag() const { return (DiffPrc || DiffMg || DiffDt); }
//
//
//
struct TSessAnlzEntry { // @flat
	int    IsTotalRow() const { return (DtVal == MAXLONG && PrcID == MAXLONG && PrmrGoodsID == MAXLONG); }
	long   DtVal;       // Дата операции
	PPID   PrcID;
	PPID   PrmrGoodsID; // Основной товар технологии, к которому относится движение товара GoodsID
	PPID   GoodsID;
	int16  PlanPhUnit;  // Количество представлено в физических единицах
	uint16 Flags;
	double InQtty;
	double OutQtty;
	double PlanInQtty;
	double PlanOutQtty;
	double PlanDev;
	double OutRest;
	double InCompPart;  // Часть компонента в общей сумме входа  (в физ единицах)
	double OutCompPart; // Часть компонента в общей сумме выхода (в физ единицах)
};

struct AddQttyBlock {
	long   DtVal;
	PPID   PrcID;
	PPID   PrmrGoodsID;
	PPID   GoodsID;
	int    Planned;
	int    Sign;
	int    PlanPhUnit;
	double Qtty;
	/* @construction
	double PhQtty;         // Количество физических единиц (когда для товара применяется независимая //
		// физическая единица не удается однозначно рассчитать PhQtty зная Qtty).
	*/
	double CompPart;
};

class TSessAnlzList : public TSVector <TSessAnlzEntry> { // @v9.8.4 TSArray-->TSVector
public:
	SLAPI  TSessAnlzList(SubstGrpGoods sgg, GoodsSubstList * pGsl, int addTotal) : TSVector <TSessAnlzEntry> (), Sgg(sgg), AddTotal(addTotal)
	{
		if(pGsl) {
			P_ScndSggList = pGsl;
			IsScndSggListOwn = 0;
		}
		else {
			P_ScndSggList = new GoodsSubstList;
			IsScndSggListOwn = 1;
		}
	}
	SLAPI ~TSessAnlzList();
	int    SLAPI Search(long dtVal, PPID prcID, PPID prmrGoodsID, PPID goodsID, uint * pPos, int useSubst = 0);
	int    SLAPI AddQtty(const AddQttyBlock & rBlk, int planned, int sign, double qtty, double compPart, int planPhUnit = 0);
	int    SLAPI SetEntryVal(TSessAnlzEntry * pEntry, int planned, int sign, double qtty, double compPart, int planPhUnit);
	int    SLAPI SetRest(long dtVal, PPID prcID, PPID goodsID, double rest);
	int    SLAPI IsProcessed(PPID goodsID, PPID prmrGoodsID) const
	{
		return BIN(ProcessedGoodsList.SearchPair(goodsID, prmrGoodsID, 0));
	}
	int    SLAPI CompletePlan(PPID prmrGoodsID, PPID prcID, const RAssocArray * pPlanList);
	TSessAnlzEntry & FASTCALL GetTotalRow(PPID goodsID);
	int    SLAPI ProcessTotalPlan();
	int    SLAPI ProcessCompParts();

	SubstGrpGoods Sgg;
	GoodsSubstList PrmrSggList;
	GoodsSubstList * P_ScndSggList;
	PPObjGoods GObj;
	//
	// Хранит пары {scndGoodsID, prmrGoodsID}
	//
	LAssocArray ProcessedGoodsList;
	int    IsScndSggListOwn;
	int    AddTotal;
};

SLAPI TSessAnlzList::~TSessAnlzList()
{
	if(IsScndSggListOwn)
		delete P_ScndSggList;
}

IMPL_CMPFUNC(TSessAnlzEntry, i1, i2)
{
	TSessAnlzEntry * p1 = (TSessAnlzEntry *)i1;
	TSessAnlzEntry * p2 = (TSessAnlzEntry *)i2;
	if(p1->DtVal < p2->DtVal)
		return -1;
	else if(p1->DtVal > p2->DtVal)
		return 1;
	else if(p1->PrcID < p2->PrcID)
		return -1;
	else if(p1->PrcID > p2->PrcID)
		return 1;
	else if(p1->PrmrGoodsID < p2->PrmrGoodsID)
		return -1;
	else if(p1->PrmrGoodsID > p2->PrmrGoodsID)
		return 1;
	else if(p1->GoodsID < p2->GoodsID)
		return -1;
	else if(p1->GoodsID > p2->GoodsID)
		return 1;
	else
		return 0;
}

int SLAPI TSessAnlzList::CompletePlan(PPID prmrGoodsID, PPID prcID, const RAssocArray * pPlanList)
{
	AddQttyBlock blk;
	MEMSZERO(blk);
	blk.PrcID = prcID;
	for(uint i = 0; i < pPlanList->getCount(); i++) {
		const RAssoc & r_plan_item = pPlanList->at(i);
		TSessAnlzEntry test;
		MEMSZERO(test);
		test.PrcID = prcID;
		test.PrmrGoodsID = prmrGoodsID;
		test.GoodsID = r_plan_item.Key;
		uint   pos = 0;
		if(!lsearch(&test, &pos, PTR_CMPFUNC(TSessAnlzEntry))) {
			double qtty = r_plan_item.Val;
			int    sign = (qtty < 0.0) ? -1 : +1;
			blk.GoodsID = r_plan_item.Key;
			AddQtty(blk, 2, sign, fabs(qtty), 0);
		}
	}
	return 1;
}

TSessAnlzEntry & FASTCALL TSessAnlzList::GetTotalRow(PPID goodsID)
{
	TSessAnlzEntry test;
	MEMSZERO(test);
	test.DtVal = MAXLONG;
	test.PrcID = MAXLONG;
	test.PrmrGoodsID = MAXLONG;
	test.GoodsID = goodsID;
	uint   pos = 0;
	if(!lsearch(&test, &pos, PTR_CMPFUNC(TSessAnlzEntry))) {
		insert(&test);
		pos = getCount()-1;
	}
	return at(pos);
}

int SLAPI TSessAnlzList::ProcessTotalPlan()
{
	uint   i;
	for(i = 0; i < getCount(); i++) {
		TSessAnlzEntry & r_entry = at(i);
		if(r_entry.IsTotalRow()) {
			r_entry.PlanInQtty = 0;
			r_entry.PlanOutQtty = 0;
		}
	}
	for(i = 0; i < getCount(); i++) {
		TSessAnlzEntry & r_entry = at(i);
		if(!r_entry.IsTotalRow()) {
			TSessAnlzEntry & r_total_entry = GetTotalRow(r_entry.GoodsID);
			r_total_entry.PlanInQtty  += r_entry.PlanInQtty;
			r_total_entry.PlanOutQtty += r_entry.PlanOutQtty;
		}
	}
	return 1;
}

int SLAPI TSessAnlzList::Search(long dtVal, PPID prcID, PPID prmrGoodsID, PPID goodsID, uint * pPos, int useSubst)
{
	uint   pos = 0;
	TSessAnlzEntry entry;
	entry.DtVal = dtVal;
	entry.PrcID = prcID;
	entry.PrmrGoodsID = prmrGoodsID;
	entry.GoodsID = goodsID;
	if(useSubst) {
		GObj.SubstGoods(prmrGoodsID, &entry.PrmrGoodsID, Sgg, 0, &PrmrSggList);
		GObj.SubstGoods(goodsID,     &entry.GoodsID,     Sgg, 0, P_ScndSggList);
	}
	if(lsearch(&entry, &pos, PTR_CMPFUNC(TSessAnlzEntry))) {
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else
		return 0;
}

int SLAPI TSessAnlzList::SetRest(long dtVal, PPID prcID, PPID goodsID, double rest)
{
	int    ok = 1;
	uint   pos = 0;
	PPID   goods_id = goodsID;
	GObj.SubstGoods(goodsID, &goods_id, Sgg, 0, P_ScndSggList);
	if(Search(dtVal, prcID, 0, goods_id, &pos)) {
		TSessAnlzEntry & r_entry = at(pos);
		r_entry.OutRest = rest;
	}
	else {
		TSessAnlzEntry entry;
		MEMSZERO(entry);
		entry.DtVal   = dtVal;
		entry.PrcID   = prcID;
		entry.GoodsID = goods_id;
		entry.OutRest = rest;
		if(!insert(&entry))
			ok = PPSetErrorSLib();
	}
	return ok;
}

int SLAPI TSessAnlzList::SetEntryVal(TSessAnlzEntry * pEntry, int planned, int sign, double qtty, double compPart, int planPhUnit)
{
	if(sign > 0)
		if(planned == 1)
			pEntry->PlanOutQtty += qtty;
		else if(planned == 2)
			pEntry->PlanOutQtty = qtty;
		else {
			pEntry->OutQtty += qtty;
			pEntry->OutCompPart += compPart;
		}
	else if(sign < 0)
		if(planned == 1)
			pEntry->PlanInQtty += qtty;
		else if(planned == 2)
			pEntry->PlanInQtty = qtty;
		else {
			pEntry->InQtty += qtty;
			pEntry->InCompPart += compPart;
		}
	if(planPhUnit)
		pEntry->PlanPhUnit = 1;
	return 1;
}

int SLAPI TSessAnlzList::AddQtty(const AddQttyBlock & rBlk, int planned, int sign, double qtty, double compPart, int planPhUnit)
{
	int    ok = 1;
	uint   pos = 0;
	PPID   prmr_goods_id = rBlk.PrmrGoodsID;
	PPID   scnd_goods_id = rBlk.GoodsID;
	ProcessedGoodsList.AddUnique(rBlk.GoodsID, rBlk.PrmrGoodsID, 0);
	THROW(GObj.SubstGoods(rBlk.PrmrGoodsID, &prmr_goods_id, Sgg, 0, &PrmrSggList));
	THROW(GObj.SubstGoods(rBlk.GoodsID, &scnd_goods_id, Sgg, 0, P_ScndSggList));
	if(Search(rBlk.DtVal, rBlk.PrcID, prmr_goods_id, scnd_goods_id, &pos))
		SetEntryVal(&at(pos), planned, sign, qtty, compPart, planPhUnit);
	else {
		TSessAnlzEntry entry;
		MEMSZERO(entry);
		entry.DtVal       = rBlk.DtVal;
		entry.PrcID       = rBlk.PrcID;
		entry.PrmrGoodsID = prmr_goods_id;
		entry.GoodsID     = scnd_goods_id;
		SetEntryVal(&entry, planned, sign, qtty, compPart, planPhUnit);
		THROW_SL(insert(&entry));
	}
	//
	// Вставка итогов по товарным позициям
	//
	if(AddTotal && rBlk.DtVal != MAXLONG && rBlk.PrcID != MAXLONG) { // Избегаем зацикливания //
		if(AddTotal != 2 || rBlk.PrmrGoodsID != rBlk.GoodsID) {
			AddQttyBlock inner_blk = rBlk;
			inner_blk.DtVal = MAXLONG;
			inner_blk.PrcID = MAXLONG;
			inner_blk.PrmrGoodsID = MAXLONG;
			THROW(AddQtty(inner_blk, planned, sign, qtty, compPart)); // @recursion
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI TSessAnlzList::ProcessCompParts()
{
	struct E {
		PPID   PrmrGoodsID;
		double In;
		double Out;
	};
	uint   i, pos;
	SVector total_list(sizeof(E)); // @v9.9.3 SArray-->SVector
	TSessAnlzEntry * p_entry;
	for(i = 0; enumItems(&i, (void **)&p_entry);) {
		if(total_list.lsearch(&p_entry->PrmrGoodsID, &(pos = 0), CMPF_LONG)) {
			E * p_e = (E *)total_list.at(pos);
			p_e->In  += p_entry->InCompPart;
			p_e->Out += p_entry->OutCompPart;
		}
		else {
			E e;
			e.PrmrGoodsID = p_entry->PrmrGoodsID;
			e.In  = p_entry->InCompPart;
			e.Out = p_entry->OutCompPart;
			total_list.insert(&e);
		}
	}
	for(i = 0; enumItems(&i, (void **)&p_entry);) {
		if(total_list.lsearch(&p_entry->PrmrGoodsID, &(pos = 0), CMPF_LONG)) {
			E * p_e2 = (E *)total_list.at(pos);
			if(p_e2->In)
				p_entry->InCompPart /= fdiv100r(p_e2->In);
			if(p_e2->Out)
				p_entry->OutCompPart /= fdiv100r(p_e2->Out);
		}
	}
	return 1;
}
//
//
//
SLAPI PPViewTSessAnlz::PPViewTSessAnlz() : PPView(0, &Filt, PPVIEW_TSESSANLZ), P_TempTbl(0)
{
}

SLAPI PPViewTSessAnlz::~PPViewTSessAnlz()
{
	delete P_TempTbl;
}
//
//
//
class TSessAnlzFilt3Dialog : public TDialog {
public:
	TSessAnlzFilt3Dialog() : TDialog(DLG_TSESANLZFILT3)
	{
	}
	int    setDTS(const TSessAnlzFilt *);
	int    getDTS(TSessAnlzFilt *);
private:
	DECL_HANDLE_EVENT;
	TSessAnlzFilt Data;
};

IMPL_HANDLE_EVENT(TSessAnlzFilt3Dialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_TSESANLZFILT_DIFPRC)) {
		GetClusterData(CTL_TSESANLZFILT_DIFPRC, &Data.DiffPrc);
		DisableClusterItem(CTL_TSESANLZFILT_FLAGS, 4, (!Data.PlanSessID || Data.DiffPrc != TSessAnlzFilt::difprcAr));
		clearEvent(event);
	}
}

int TSessAnlzFilt3Dialog::setDTS(const TSessAnlzFilt * pData)
{
	Data = *pData;
	setCtrlUInt16(CTL_TSESANLZFILT_WHAT, BIN(Data.Flags & TSessAnlzFilt::fAll));
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 0, TSessAnlzFilt::fAddTotalRows);
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 1, TSessAnlzFilt::fPrmrGoodsOnly);
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 2, TSessAnlzFilt::fShowRest);
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 3, TSessAnlzFilt::fExtrapolToPeriod);
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 4, TSessAnlzFilt::fInterpolPlanToAr);
	AddClusterAssoc(CTL_TSESANLZFILT_FLAGS, 5, TSessAnlzFilt::fCalcCompParts);
	SetClusterData(CTL_TSESANLZFILT_FLAGS, Data.Flags);

	AddClusterAssoc(CTL_TSESANLZFILT_DIFPRC, 0, TSessAnlzFilt::difprcNone);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFPRC, 1, TSessAnlzFilt::difprcPrc);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFPRC, 2, TSessAnlzFilt::difprcPrcGroup);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFPRC, 3, TSessAnlzFilt::difprcAr);
	SetClusterData(CTL_TSESANLZFILT_DIFPRC, Data.DiffPrc);

	AddClusterAssoc(CTL_TSESANLZFILT_DIFMG, 0, TSessAnlzFilt::difmgNone);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFMG, 1, TSessAnlzFilt::difmgGoods);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFMG, 2, TSessAnlzFilt::difmgGroup);
	SetClusterData(CTL_TSESANLZFILT_DIFMG, Data.DiffMg);

	AddClusterAssoc(CTL_TSESANLZFILT_DIFDT, 0, TSessAnlzFilt::difdtNone);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFDT, 1, TSessAnlzFilt::difdtDate);
	AddClusterAssoc(CTL_TSESANLZFILT_DIFDT, 2, TSessAnlzFilt::difdtSupersess);
	SetClusterData(CTL_TSESANLZFILT_DIFDT, Data.DiffDt);

	AddClusterAssoc(CTL_TSESANLZFILT_NMGOPT, 0, TSessAnlzFilt::difdtNone);
	AddClusterAssoc(CTL_TSESANLZFILT_NMGOPT, 1, TSessAnlzFilt::difdtDate);
	long   nmgopt = 0;
	if(Data.Flags & TSessAnlzFilt::fNmgAllRestrict)
		nmgopt = 2;
	else if(Data.Flags & TSessAnlzFilt::fNmgRestrictOnly)
		nmgopt = 1;
	else
		nmgopt = 0;
	AddClusterAssoc(CTL_TSESANLZFILT_NMGOPT, 0, 0);
	AddClusterAssoc(CTL_TSESANLZFILT_NMGOPT, 1, 1);
	AddClusterAssoc(CTL_TSESANLZFILT_NMGOPT, 2, 2);
	SetClusterData(CTL_TSESANLZFILT_NMGOPT, nmgopt);
	SetupPPObjCombo(this, CTLSEL_TSESANLZFILT_NMG, PPOBJ_GOODSGROUP, Data.NmGoodsGrpID, OLW_CANSELUPLEVEL, 0);
	SetupSubstGoodsCombo(this, CTLSEL_TSESANLZFILT_SUBS, Data.Sgg); // @v6.0.2 AHTOXA
	disableCtrl(CTL_TSESANLZFILT_WHAT, Data.PlanSessID);
	DisableClusterItem(CTL_TSESANLZFILT_FLAGS, 3, !Data.PlanSessID);
	DisableClusterItem(CTL_TSESANLZFILT_FLAGS, 4, (!Data.PlanSessID || Data.DiffPrc != TSessAnlzFilt::difprcAr));
	return 1;
}

int TSessAnlzFilt3Dialog::getDTS(TSessAnlzFilt * pData)
{
	ushort v = getCtrlUInt16(CTL_TSESANLZFILT_WHAT);
	SETFLAG(Data.Flags, TSessAnlzFilt::fAll, v == 1);
	GetClusterData(CTL_TSESANLZFILT_FLAGS, &Data.Flags);
	GetClusterData(CTL_TSESANLZFILT_DIFPRC, &Data.DiffPrc);
	GetClusterData(CTL_TSESANLZFILT_DIFMG,  &Data.DiffMg);
	GetClusterData(CTL_TSESANLZFILT_DIFDT,  &Data.DiffDt);
	long   nmgopt = 0;
	GetClusterData(CTL_TSESANLZFILT_NMGOPT, &nmgopt);
	Data.Flags &= ~(TSessAnlzFilt::fNmgRestrictOnly | TSessAnlzFilt::fNmgAllRestrict);
	if(nmgopt == 0) {
		;
	}
	else if(nmgopt == 1)
		Data.Flags |= TSessAnlzFilt::fNmgRestrictOnly;
	else if(nmgopt == 2)
		Data.Flags |= TSessAnlzFilt::fNmgAllRestrict;
	getCtrlData(CTLSEL_TSESANLZFILT_NMG, &Data.NmGoodsGrpID);
	getCtrlData(CTLSEL_TSESANLZFILT_SUBS, &Data.Sgg);
	ASSIGN_PTR(pData, Data);
	return 1;
}
//

int SLAPI PPViewTSessAnlz::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODY(TSessAnlzFilt3Dialog, (TSessAnlzFilt *)pBaseFilt);
}

int SLAPI PPViewTSessAnlz::IsGoodsBelongToGen(PPID goodsID, PPID * pGenID)
{
	int    ok = -1;
	PPID   gen_id = 0;
	if(GenList.Search(goodsID, &gen_id, 0) > 0) {
		ok = gen_id ? 1 : -1;
	}
	else if(GObj.P_Tbl->BelongToGen(goodsID, &(gen_id = 0)) > 0) {
		ok = 1;
		PPIDArray gen_list;
		if(GObj.P_Tbl->GetGenericList(gen_id, &gen_list) > 0)
			for(uint i = 0; i < gen_list.getCount(); i++)
				THROW(GenList.Add(gen_list.get(i), gen_id, 0));
	}
	else
		THROW(GenList.Add(goodsID, 0, 0));
	CATCHZOK
	ASSIGN_PTR(pGenID, gen_id);
	return ok;
}

int SLAPI PPViewTSessAnlz::GetPlanWithSubst(PPID arID, PPID * pGoodsID, double * pPlan, int * pSign)
{
	int    ok = -1;
	double plan = 0.0;
	PPID   goods_id = *pGoodsID;
	if(goods_id) {
		if(Filt.PlanSessID) {
			double q = 0.0;
			PPID   gen_id = 0;
			if(PlanList.Search(goods_id, &q, 0)) {
				plan = fabs(q);
				if(pSign)
					*pSign = (q < 0.0) ? -1 : 1;
				ok = 1;
			}
			else if(IsGoodsBelongToGen(goods_id, &gen_id) > 0) {
				if(PlanList.Search(gen_id, &q, 0)) {
					goods_id = gen_id;
					plan = fabs(q);
					if(pSign)
						*pSign = (q < 0.0) ? -1 : 1;
					ok = 2;
				}
			}
		}
	}
	*pGoodsID = goods_id;
	if(ok > 0) {
		if(arID && Filt.Flags & TSessAnlzFilt::fInterpolPlanToAr) {
			double ratio = ArTimesList.Get(arID);
			plan *= ratio;
		}
		ASSIGN_PTR(pPlan, plan);
	}
	return ok;
}

int SLAPI PPViewTSessAnlz::NmGoodsSubst(PPID prmrGoodsID, PPID * pGoodsID)
{
	if(Filt.NmGoodsGrpID && *pGoodsID != prmrGoodsID)
		if(GObj.BelongToGroup(*pGoodsID, Filt.NmGoodsGrpID) > 0) {
			if(!(Filt.Flags & (TSessAnlzFilt::fNmgRestrictOnly | TSessAnlzFilt::fNmgAllRestrict)))
				*pGoodsID = Filt.NmGoodsGrpID;
		}
		else
			return 0;
	return 1;
}

int SLAPI PPViewTSessAnlz::ProcessSession(const TSessionTbl::Rec * pRec, TSessAnlzList * pResult)
{
	int    ok = 1;
	//
	// Общая физическая единица измерения, используемая для расчета долей компонентов
	// в общем производстве.
	// В дальнейшем следует организованть поле в структуре товара, определяющее общую
	// единицу измерения.
	//
	const  PPID   comm_ph_unit_id = PPUNT_KILOGRAM;
	AddQttyBlock blk;
	MEMSZERO(blk);
	const  PPID ar_id = pRec->ArID;
	if(Filt.DiffDt == TSessAnlzFilt::difdtDate)
		blk.DtVal = pRec->StDt;
	else if(Filt.DiffDt == TSessAnlzFilt::difdtSupersess)
		blk.DtVal = (pRec->Flags & TSESF_SUPERSESS) ? pRec->ID : pRec->ParentID;
	if(Filt.DiffPrc == TSessAnlzFilt::difprcPrc)
		blk.PrcID = pRec->PrcID;
	else if(Filt.DiffPrc == TSessAnlzFilt::difprcPrcGroup) {
		ProcessorTbl::Rec prc_rec;
		if(TSesObj.GetPrc(pRec->PrcID, &prc_rec, 0, 1) > 0)
			blk.PrcID = prc_rec.ParentID;
	}
	else if(Filt.DiffPrc == TSessAnlzFilt::difprcAr)
		blk.PrcID = pRec->ArID;
	double prmr_qtty = 0.0;
	double prmr_plan = 0.0;
	PPIDArray seen_goods_list; // Список товаров (не основных) плановое количество по которым
		// уже добавлено в таблицу. Поддержка этого списка позволяет избежать множественного
		// учета плана расхода (выход) по неосновным позициям, которые встречаются в сессии несколько раз
	PPGoodsStruc gs;
	TechTbl::Rec tec_rec;
	TSessLineTbl::Rec line_rec;
	if(TSesObj.GetTech(pRec->TechID, &tec_rec) > 0) {
		// @todo @# (!tec_rec.PrcID || tec_rec.PrcID == pRec->PrcID)
		blk.PrmrGoodsID = tec_rec.GoodsID;
		if(tec_rec.GStrucID) {
			PPObjGoodsStruc gs_obj;
			gs_obj.Get(tec_rec.GStrucID, &gs);
		}
		if(blk.PrmrGoodsID) {
			for(SEnum en = TSesObj.P_Tbl->EnumLines(pRec->ID); en.Next(&line_rec) > 0;) {
				if(line_rec.GoodsID == blk.PrmrGoodsID && !(line_rec.Flags & TSESLF_OUTREST))
					prmr_qtty = faddwsign(prmr_qtty, line_rec.Qtty, line_rec.Sign);
			}
			if(!Filt.DiffMg)
				blk.PrmrGoodsID = 0;
		}
		if(!(Filt.Flags & TSessAnlzFilt::fNmgAllRestrict) || GObj.BelongToGroup(tec_rec.GoodsID, Filt.NmGoodsGrpID) > 0) {
			//
			// Устанавливаем плановые количества.
			// Если есть плановая сессия, то плановые величины берем из нее, в противном
			// случае, плановые величины берем из рабочих сессий.
			//
			if(Filt.PlanSessID) {
				double plan = 0.0;
				int    sign = tec_rec.Sign;
				blk.GoodsID = tec_rec.GoodsID;
				GetPlanWithSubst(ar_id, &blk.GoodsID, &plan, &sign);
				prmr_plan = plan;
				GetPlanWithSubst(ar_id, &blk.PrmrGoodsID, 0, 0);
				THROW(pResult->AddQtty(blk, 2, sign, plan, 0));
			}
			else if(pRec->PlannedQtty > 0) {
				blk.GoodsID = tec_rec.GoodsID;
				THROW(pResult->AddQtty(blk, 1, tec_rec.Sign, pRec->PlannedQtty, 0));
			}
		}
	}
	else
		MEMSZERO(tec_rec);
	//
	// Если не требуется дифференцировать отчет по основному товару,
	// то реализуем это, обнуляя ид основного товара.
	// При этом плановое количество зависимых позиций все равно рассчитываем исходя из
	// полученного (израсходованного) количества основного товара
	//
	if(!Filt.DiffMg)
		blk.PrmrGoodsID = 0;
	if(Filt.PlanSessID)
		GetPlanWithSubst(ar_id, &blk.PrmrGoodsID, 0, 0);
	if(gs.Rec.ID && prmr_qtty != 0 && !(Filt.Flags & TSessAnlzFilt::fPrmrGoodsOnly)) {
		PPGoodsStrucItem gs_item;
		double plan;
		for(uint gs_pos = 0; gs.EnumItemsExt(&gs_pos, &gs_item, tec_rec.GoodsID, fabs(prmr_qtty), &plan) > 0;) {
			int    sign = 0;
			if(gs_item.Median < 0)
				sign = tec_rec.Sign;
			else if(gs_item.Median > 0)
				sign = -tec_rec.Sign;
			int    _sign = sign;
			blk.GoodsID = gs_item.GoodsID;
			int    r = GetPlanWithSubst(ar_id, &blk.GoodsID, &plan, &_sign);
			if(plan != 0)
				if(NmGoodsSubst(blk.PrmrGoodsID, &blk.GoodsID)) {
					int    planned_tag = 1;
					if(Filt.PlanSessID && r > 0 && _sign == sign)
						planned_tag = 2;
					else
						plan = fabs(plan);
					THROW(pResult->AddQtty(blk, planned_tag, sign, plan, 0));
				}
				else
					continue;
		}
	}
	{
		for(SEnum en = TSesObj.P_Tbl->EnumLines(pRec->ID); en.Next(&line_rec) > 0;) {
			if(Filt.Flags & TSessAnlzFilt::fNmgAllRestrict && GObj.BelongToGroup(line_rec.GoodsID, Filt.NmGoodsGrpID) <= 0)
				continue;
			if(!(line_rec.Flags & TSESLF_OUTREST) &&
				(line_rec.GoodsID == tec_rec.GoodsID || !(Filt.Flags & TSessAnlzFilt::fPrmrGoodsOnly))) {
				const  int sign = line_rec.Sign;
				int    plan_ph_unit = 0;
				double comppart = 0.0;
				double fact = line_rec.Qtty; // Фактически произведенное (израсходованное) количество.
				blk.GoodsID = line_rec.GoodsID;
				if(Filt.PlanSessID) {
					int    _sign = line_rec.Sign;
					double _plan = 0.0, phuperu;
					int    r = GetPlanWithSubst(ar_id, &blk.GoodsID, &_plan, &_sign);
					if(PhTagPlanList.bsearch(blk.GoodsID)) {
						//
						// Факт должен быть представлен в физических единицах
						//
						plan_ph_unit = 1;
						if(line_rec.Flags & TSESLF_INDEPPHQTTY)
							fact = line_rec.WtQtty; // Независимый учет в физических единицах
						else {
							GObj.GetPhUPerU(blk.GoodsID, 0, &phuperu);
							fact *= phuperu;
						}
					}
					if(NmGoodsSubst(blk.PrmrGoodsID, &blk.GoodsID)) {
						if(r > 0 && _plan != 0 && _sign == sign)
							THROW(pResult->AddQtty(blk, 2, _sign, _plan, 0));
					}
					else
						continue;
				}
				else if(!NmGoodsSubst(blk.PrmrGoodsID, &blk.GoodsID))
					continue;
				if(Filt.Flags & TSessAnlzFilt::fCalcCompParts) {
					PPID   ph_unit_id = 0;
					double phuperu;
					if(GObj.GetPhUPerU(line_rec.GoodsID, &ph_unit_id, &phuperu) > 0 && ph_unit_id == comm_ph_unit_id)
						comppart = fact * phuperu;
				}
				THROW(pResult->AddQtty(blk, 0, sign, fact, comppart, plan_ph_unit));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTSessAnlz::CreateBySess(PPID sessID, TSessAnlzList * pResult, PPIDArray * pProcessedList)
{
	int    ok = -1;
	long   enum_handle = -1;
	TSessionTbl::Rec sess_rec;
	TSessLineTbl::Rec line_rec;
	ProcessorTbl::Rec prc_rec;
	if(!pProcessedList || !pProcessedList->lsearch(sessID)) {
		if(TSesObj.Search(sessID, &sess_rec) > 0) {
			PPIDArray child_list;
			if(TSesObj.P_Tbl->GetChildIDList(sessID, 0, &child_list) > 0) {
				for(uint i = 0; i < child_list.getCount(); i++)
					THROW(CreateBySess(child_list.at(i), pResult, pProcessedList)); // @recursion
				//
				// Расчет фиксированных остатков
				//
				if(Filt.Flags & TSessAnlzFilt::fShowRest && !Filt.DiffPrc)
					if(TSesObj.GetPrc(sess_rec.PrcID, &prc_rec, 0, 1) > 0 && prc_rec.Flags & PRCF_STOREGOODSREST) {
						THROW(TSesObj.P_Tbl->InitLineEnum(sessID, &enum_handle));
						while(TSesObj.P_Tbl->NextLineEnum(enum_handle, &line_rec) > 0)
							if(line_rec.Flags & TSESLF_OUTREST)
								THROW(pResult->SetRest(ZERODATE, 0, line_rec.GoodsID, line_rec.Qtty));
						TSesObj.P_Tbl->DestroyIter(enum_handle);
					}
			}
			else {
				THROW(ProcessSession(&sess_rec, pResult));
			}
		}
		CALLPTRMEMB(pProcessedList, addUnique(sessID));
	}
	CATCHZOK
	TSesObj.P_Tbl->DestroyIter(enum_handle);
	return ok;
}

int SLAPI PPViewTSessAnlz::GetDutySchedPacket(PPID prcID, PPDutySchedPacket * pDsPack)
{
	int    ok = -1;
	PPOprKind op_rec;
	ProcessorTbl::Rec prc_rec;
	if(TSesObj.GetPrc(prcID, &prc_rec, 1, 1) > 0 && GetOpData(prc_rec.WrOffOpID, &op_rec) > 0 && op_rec.AccSheetID) {
		PPObjDutySched ds_obj;
		PPID   ds_id = 0;
		if(ds_obj.SearchByObjType(PPOBJ_ARTICLE, op_rec.AccSheetID, &ds_id) > 0)
			if(ds_obj.GetPacket(ds_id, pDsPack) > 0)
				ok = 1;
	}
	return ok;
}

int SLAPI PPViewTSessAnlz::CalcArTimes(const PPDutySchedPacket * pDsPack, const STimeChunk & rBounds, RAssocArray * pDurationList)
{
	int    ok = -1;
	PPDutySchedPacket::EnumParam ep;
	pDurationList->freeAll();
	if(pDsPack->InitIteration(&ep, rBounds) > 0) {
		while(pDsPack->NextIteration(&ep) > 0) {
			pDurationList->Add(ep.ObjID, ep.Duration, 1, 0);
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

// @v8.6.6 PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempTSessRep);

int SLAPI PPViewTSessAnlz::CreateTSessFiltByPlan(const TSessionTbl::Rec * pPlanRec, TSessionFilt * pFilt) const
{
	pFilt->PrcID = pPlanRec->PrcID;
	pFilt->StPeriod.Set(pPlanRec->StDt, pPlanRec->FinDt);
	pFilt->StTime = pPlanRec->StTm;
	if(pPlanRec->FinTm) {
		pFilt->FnPeriod.Set(pPlanRec->StDt, pPlanRec->FinDt);
		pFilt->FnTime = pPlanRec->FinTm;
	}
	pFilt->StatusFlags = (1 << TSESST_CLOSED);
	return 1;
}

int SLAPI PPViewTSessAnlz::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	{
		ZDELETE(P_TempTbl);
		PlanList.freeAll();
		PhTagPlanList.freeAll();
		ArTimesList.freeAll();
		uint   i;
		TSessAnlzEntry * p_entry = 0; // @nonallocated
		PPObjGoods goods_obj;
		Gsl.Clear();
		Gsl.SaveAssoc = 1;
		int    add_total = BIN(Filt.Flags & TSessAnlzFilt::fAddTotalRows && Filt.IsDiffFlag());
		if(add_total)
			if(!Filt.DiffDt && !Filt.DiffPrc) {
				//
				// Если нет дифференциации по процессору и дате, то устанавливаем
				// условие, согласно которому не будут суммироваться итоги по строкам,
				// в которых PrmrGoodsID == scndGoodsID (вторичный товар эквивалентен первичному)
				//
				add_total = 2;
			}
		TSessAnlzList result(Filt.Sgg, &Gsl, add_total);
		PPIDArray sess_list, processed_list;
		PPWait(1);
		if(Filt.PlanSessID) {
			long   h = -1;
			TSessionTbl::Rec sess_rec;
			TSessLineTbl::Rec line_rec;
			THROW(TSesObj.P_Tbl->InitLineEnum(Filt.PlanSessID, &h));
			while(TSesObj.P_Tbl->NextLineEnum(h, &line_rec) > 0) {
				double qtty = 0.0;
				if(line_rec.Sign > 0)
					qtty = line_rec.Qtty;
				else if(line_rec.Sign < 0)
					qtty = -line_rec.Qtty;
				PlanList.Add(line_rec.GoodsID, qtty);
				if(line_rec.Flags & TSESLF_PLAN_PHUNIT)
					PhTagPlanList.addUnique(line_rec.GoodsID);
			}
			TSesObj.P_Tbl->DestroyIter(h);
			PhTagPlanList.sort();
			if(TSesObj.Search(Filt.PlanSessID, &sess_rec) > 0) {
				long   plan_hours = 0; // Количество часов в плановом периоде
				long   fact_hours = 0; // Количество уже отработанных часов
				if(Filt.Flags & TSessAnlzFilt::fExtrapolToPeriod)
					if(sess_rec.StDt && sess_rec.FinDt && sess_rec.FinDt >= sess_rec.StDt) {
						plan_hours  = diffdate(sess_rec.FinDt, sess_rec.StDt) * 24;
						plan_hours -= sess_rec.StTm.hour();
						plan_hours += sess_rec.FinTm ? sess_rec.FinTm.hour() : 24;
					}
				if(Filt.Flags & TSessAnlzFilt::fInterpolPlanToAr) {
					PPDutySchedPacket ds_pack;
					if(GetDutySchedPacket(sess_rec.PrcID, &ds_pack) > 0) {
						RAssocArray ar_times;
						STimeChunk bounds;
						bounds.Start.Set(sess_rec.StDt, sess_rec.StTm);
						bounds.Finish.Set(sess_rec.FinDt, sess_rec.FinTm);
						THROW(CalcArTimes(&ds_pack, bounds, &ArTimesList));
						{
							const double total = ArTimesList.GetTotal();
							if(total != 0.0)
								ArTimesList.Scale(1.0 / total);
							else {
								ArTimesList.freeAll();
								Filt.Flags &= ~TSessAnlzFilt::fInterpolPlanToAr;
							}
						}
					}
					else
						Filt.Flags &= ~TSessAnlzFilt::fInterpolPlanToAr;
				}
				LDATETIME max_dtm = ZERODATETIME;
				TSessionFilt ts_filt;
				PPViewTSession ts_view;
				TSessionViewItem ts_item;
				CreateTSessFiltByPlan(&sess_rec, &ts_filt);
				THROW(ts_view.Init_(&ts_filt));
				for(ts_view.InitIteration(0); ts_view.NextIteration(&ts_item) > 0;) {
					sess_list.addUnique(ts_item.ID);
					if(cmp(max_dtm, ts_item.FinDt, ts_item.FinTm) < 0)
						max_dtm.Set(ts_item.FinDt, ts_item.FinTm);
				}
				if(plan_hours && sess_list.getCount()) {
					fact_hours  = diffdate(max_dtm.d, sess_rec.StDt) * 24;
					fact_hours -= sess_rec.StTm.hour();
					fact_hours += sess_rec.FinTm ? sess_rec.FinTm.hour() : 24;
					const double part = ((double)fact_hours) / ((double)plan_hours);
					for(i = 0; i < PlanList.getCount(); i++)
						PlanList.at(i).Val *= part;
				}
			}
		}
		else if(Filt.SessIdList.IsExists())
			sess_list = Filt.SessIdList.Get();
		for(i = 0; i < sess_list.getCount(); i++) {
			THROW(CreateBySess(sess_list.at(i), &result, &processed_list));
			PPWaitPercent(i+1, sess_list.getCount());
		}
		if(Filt.PlanSessID) {
			if(!Filt.IsDiffFlag()) {
				//
				// Если отчет строится по плану и фильтрация не предусматривает дифференциацию,
				// то добавляем в отчет позиции, которые есть в плане, но не было в производстве.
				//
				result.CompletePlan(0, 0, &PlanList);
			}
			if(Filt.Flags & TSessAnlzFilt::fInterpolPlanToAr && Filt.Flags & TSessAnlzFilt::fAddTotalRows) {
				for(i = 0; i < ArTimesList.getCount(); i++) {
					const RAssoc & r_ar_item = ArTimesList.at(i);
					RAssocArray temp_plan = PlanList;
					temp_plan.Scale(r_ar_item.Val);
					result.CompletePlan(0, r_ar_item.Key, &temp_plan);
				}
				result.ProcessTotalPlan();
			}
		}
		if(Filt.Flags & TSessAnlzFilt::fCalcCompParts)
			result.ProcessCompParts();
		THROW(P_TempTbl = CreateTempFile <TempTSessRepTbl> ());
		{
			//
			// Из буфера отчета формируем записи таблицы
			//
			SString temp_buf, word_total;
			PPGetWord(PPWORD_TOTAL, 0, word_total);
			TempTSessRepTbl::Rec rec;
			PPObjArticle ar_obj;
			BExtInsert bei(P_TempTbl);
			for(i = 0; result.enumItems(&i, (void **)&p_entry);) {
				MEMSZERO(rec);
				rec.DtVal       = p_entry->DtVal;
				rec.PrcID       = p_entry->PrcID;
				rec.PrmrGoodsID = p_entry->PrmrGoodsID;
				rec.GoodsID     = p_entry->GoodsID;
				rec.NotPrmrLine = (rec.PrmrGoodsID == rec.GoodsID) ? 0 : 1;
				rec.PlanPhUnit  = p_entry->PlanPhUnit;
				rec.InQtty      = p_entry->InQtty;
				rec.OutQtty     = p_entry->OutQtty;
				rec.PlanInQtty  = p_entry->PlanInQtty;
				rec.PlanOutQtty = p_entry->PlanOutQtty;
				rec.OutRest     = p_entry->OutRest;
				rec.InCompPart  = p_entry->InCompPart;
				rec.OutCompPart = p_entry->OutCompPart;
				if(rec.PrmrGoodsID == MAXLONG)
					STRNSCPY(rec.PrmrGoodsName, word_total);
				else {
					GObj.GetSubstText(rec.PrmrGoodsID, Filt.Sgg, &result.PrmrSggList, temp_buf);
					STRNSCPY(rec.PrmrGoodsName, temp_buf);
				}
				GObj.GetSubstText(rec.GoodsID, Filt.Sgg, result.P_ScndSggList, temp_buf);
				STRNSCPY(rec.GoodsName, temp_buf);
				if(rec.DtVal == MAXLONG)
					STRNSCPY(rec.DtText, word_total);
				else if(Filt.DiffDt == TSessAnlzFilt::difdtDate)
					datefmt(&rec.DtVal, DATF_DMY|DATF_CENTURY, rec.DtText);
				else if(Filt.DiffDt == TSessAnlzFilt::difdtSupersess) {
					TSessionTbl::Rec tses_rec;
					if(TSesObj.Search(rec.DtVal, &tses_rec) > 0) {
						TSesObj.MakeName(&tses_rec, temp_buf);
						STRNSCPY(rec.DtText, temp_buf);
					}
				}
				//
				if(rec.PrcID == MAXLONG)
					STRNSCPY(rec.PrcName, word_total);
				else if(oneof2(Filt.DiffPrc, TSessAnlzFilt::difprcPrc, TSessAnlzFilt::difprcPrcGroup)) {
					ProcessorTbl::Rec prc_rec;
					if(TSesObj.GetPrc(rec.PrcID, &prc_rec, 0, 1) > 0)
						STRNSCPY(rec.PrcName, prc_rec.Name);
				}
				else if(Filt.DiffPrc == TSessAnlzFilt::difprcAr) {
					ArticleTbl::Rec ar_rec;
					if(ar_obj.Fetch(rec.PrcID, &ar_rec) > 0)
						STRNSCPY(rec.PrcName, ar_rec.Name);
				}
				THROW_DB(bei.insert(&rec));
			}
			THROW_DB(bei.flash());
		}
	}
	CATCH
		ok = 0;
		ZDELETE(P_TempTbl);
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI PPViewTSessAnlz::InitIteration()
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempTbl) {
		TempTSessRepTbl::Key3 k3, k3_;
		MEMSZERO(k3);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 3));
		P_IterQuery->selectAll();
		k3_ = k3;
		Counter.Init(P_IterQuery->countIterations(0, &k3_, spFirst));
		P_IterQuery->initIteration(0, &k3, spFirst);
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewTSessAnlz::NextIteration(TSessAnlzViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		RecToViewItem(&P_TempTbl->data, pItem);
		Counter.Increment();
		return 1;
	}
	return -1;
}

DBQuery * SLAPI PPViewTSessAnlz::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_TSESSANLZ;
	DBQ  * dbq = 0;
	TempTSessRepTbl * p_tt = 0;
	DBQuery * p_q = 0;
	if(P_TempTbl) {
		THROW(CheckTblPtr(p_tt = new TempTSessRepTbl(P_TempTbl->GetName())));
		p_q = & select(
			p_tt->ID__,            // #0
			p_tt->DtText,          // #1
			p_tt->PrcName,         // #2
			p_tt->PrmrGoodsName,   // #3
			p_tt->GoodsName,       // #4
			p_tt->InQtty,          // #5
			p_tt->OutQtty,         // #6
			p_tt->PlanInQtty,      // #7
			p_tt->PlanOutQtty,     // #8
			p_tt->OutRest,         // #9
			p_tt->InCompPart,      // #10
			p_tt->OutCompPart,     // #11
			0L).from(p_tt, 0L);
		p_q->orderBy(p_tt->PrcID, p_tt->PrmrGoodsName, p_tt->GoodsName, 0L);
	}
	THROW(CheckQueryPtr(p_q));
	if(pSubTitle) {
		if(Filt.SessIdList.IsExists() && Filt.SessIdList.Get().getSingle()) {
			PPID   single_sess_id = Filt.SessIdList.Get().getSingle();
			GetObjectName(PPOBJ_TSESSION, single_sess_id, *pSubTitle, 1);
		}
		else {
			if(Filt.PrcID) {
				pSubTitle->CatDivIfNotEmpty(';', 0);
				GetObjectName(PPOBJ_PROCESSOR, Filt.PrcID, *pSubTitle, 1);
			}
		}
	}
	CATCH
		if(p_q)
			ZDELETE(p_q);
		else
			delete p_tt;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

void SLAPI PPViewTSessAnlz::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw && P_TempTbl) {
		DBQBrowserDef * p_def = (DBQBrowserDef *)pBrw->getDef();
		const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
		if(p_q) {
			uint   col_no = 0;
			SString buf;
			if(Filt.DiffDt) {
				if(Filt.DiffDt == TSessAnlzFilt::difdtSupersess) {
					// @v9.2.7 PPGetWord(PPWORD_SUPERSESS, 0, buf);
					PPLoadString("supersession", buf); // @v9.2.7
				}
				else { //if(Filt.DiffDt == TSessAnlzFilt::difdtDt)
					PPLoadString("date", buf);
				}
				pBrw->insertColumn(col_no++, buf, 1, 0, 0, 0);
			}
			if(Filt.DiffPrc) {
				// @v9.0.2 PPGetWord((Filt.DiffPrc == TSessAnlzFilt::difprcAr) ? PPWORD_AR : PPWORD_PRC, 0, buf);
				PPLoadString(((Filt.DiffPrc == TSessAnlzFilt::difprcAr) ? "article" : "processor"), buf); // @v9.0.2
				pBrw->insertColumn(col_no++, buf, 2, 0, 0, 0);
			}
			if(Filt.DiffMg) {
				pBrw->InsColumnWord(col_no++, PPWORD_PRMRGOODS, 3, 0, 0, 0);
			}
			if(Filt.Flags & TSessAnlzFilt::fCalcCompParts) {
				pBrw->insertColumn(5+col_no++, "InPart",  10, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
				pBrw->insertColumn(5+col_no++, "OutPart", 11, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
			}
		}
	}
}

int SLAPI PPViewTSessAnlz::GetPhQtty(const TSessAnlzViewItem * pItem, PPID * pPhUnitID, double * pIn, double * pOut)
{
	int    ok = -1;
	double in = 0.0;
	double out = 0.0;
	if(pItem) {
		if(pItem->PlanPhUnit) {
			in  = pItem->InQtty;
			out = pItem->OutQtty;
		}
		else {
			double phuperu;
			GObj.GetPhUPerU(pItem->GoodsID, pPhUnitID, &phuperu);
			in  = pItem->InQtty  * phuperu;
			out = pItem->OutQtty * phuperu;
		}
		ok = 1;
	}
	ASSIGN_PTR(pIn, in);
	ASSIGN_PTR(pOut, out);
	return ok;
}

void SLAPI PPViewTSessAnlz::RecToViewItem(const TempTSessRepTbl::Rec * pRec, TSessAnlzViewItem * pItem)
{
	if(pItem && pRec) {
		memzero(pItem, sizeof(*pItem));
		pItem->__ID  = pRec->ID__;
		pItem->DtVal = pRec->DtVal;
		pItem->PrcID = pRec->PrcID;
		pItem->NotPrmrLine = pRec->NotPrmrLine;
		pItem->PlanPhUnit  = pRec->PlanPhUnit;
		pItem->PrmrGoodsID = pRec->PrmrGoodsID;
		pItem->GoodsID     = pRec->GoodsID;
		pItem->InQtty      = pRec->InQtty;
		pItem->OutQtty     = pRec->OutQtty;
		pItem->PlanInQtty  = pRec->PlanInQtty;
		pItem->PlanOutQtty = pRec->PlanOutQtty;
		pItem->PlanDev     = pRec->PlanDev;
		pItem->OutRest     = pRec->OutRest;
		pItem->InCompPart  = pRec->InCompPart;
		pItem->OutCompPart = pRec->OutCompPart;
		STRNSCPY(pItem->PrmrGoodsName, pRec->PrmrGoodsName);
		STRNSCPY(pItem->GoodsName, pRec->GoodsName);
		STRNSCPY(pItem->PrcName, pRec->PrcName);
		STRNSCPY(pItem->DtText,  pRec->DtText);
		GetPhQtty(pItem, 0, &pItem->InQttyPh, &pItem->OutQttyPh);
	}
}

int SLAPI PPViewTSessAnlz::GetItem(PPID __id, TSessAnlzViewItem * pItem)
{
	int    ok = -1;
	TempTSessRepTbl::Rec rec;
	if(SearchByID(P_TempTbl, 0, __id, &rec) > 0) {
		RecToViewItem(&rec, pItem);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewTSessAnlz::EditGoods(PPID goodsID)
{
	int    ok = -1;
	if(goodsID) {
		if(Filt.Sgg) {
			PPIDArray id_list;
			Gsl.GetGoodsBySubstID(goodsID, &id_list);
			if(id_list.getCount() == 1) {
				goodsID = id_list.at(0);
				if(GObj.Edit(&goodsID, 0) == cmOK)
					ok = 1;
			}
			else if(id_list.getCount() > 1) {
				ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, 0, ExtGoodsSelDialog::fByName);
				if(CheckDialogPtrErr(&dlg)) {
					dlg->setSelectionByGoodsList(&id_list);
					TIDlgInitData d;
					while(ExecView(dlg) == cmOK) {
						dlg->getDTS(&d);
						if(d.GoodsID && GObj.Edit(&d.GoodsID, 0) == cmOK)
							ok = 1;
					}
				}
				else
					ok = 0;
				delete dlg;
			}
		}
		else if(GObj.Edit(&goodsID, 0) == cmOK)
			ok = 1;
	}
	return ok;
}

int SLAPI PPViewTSessAnlz::Print(const void * pHdr)
{
	return Helper_Print((Filt.Flags & TSessAnlzFilt::fCalcCompParts) ? REPORT_TSESSANLZ_COMPPART : REPORT_TSESSANLZ);
}

int SLAPI PPViewTSessAnlz::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   __id = pHdr ? *(PPID *)pHdr : 0;
		TSessAnlzViewItem item;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.GoodsID)
					ok = EditGoods(item.GoodsID);
				break;
			case PPVCMD_EDITPRC:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.PrcID) {
					PPObjProcessor prc_obj;
					if(prc_obj.Edit(&item.PrcID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_VIEWTSESS:
				ok = -1;
				if(GetItem(__id, &item) > 0 && item.GoodsID) {
					TSessionTbl::Rec plan_rec;
					TSessLineFilt filt;
					filt.GoodsID = item.GoodsID;
					if(Filt.SessIdList.IsExists())
						filt.TSesList = Filt.SessIdList;
					else if(TSesObj.Search(Filt.PlanSessID, &plan_rec) > 0) {
						TSessionFilt ts_filt;
						PPViewTSession ts_view;
						TSessionViewItem ts_item;
						CreateTSessFiltByPlan(&plan_rec, &ts_filt);
						if(ts_view.Init_(&ts_filt)) {
							PPIDArray child_list;
							for(ts_view.InitIteration(0); ts_view.NextIteration(&ts_item) > 0;) {
								child_list.clear();
								filt.TSesList.Add(ts_item.ID);
								if(TSesObj.P_Tbl->GetChildIDList(ts_item.ID, 0, &child_list) > 0)
									for(uint i = 0; i < child_list.getCount(); i++)
										filt.TSesList.Add(child_list.at(i));
							}
						}
						else
							PPError();
					}
					::ViewTSessLine(&filt);
				}
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_TSessAnlz
//
PPALDD_CONSTRUCTOR(TSessAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TSessAnlz) { Destroy(); }

int PPALDD_TSessAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(TSessAnlz, rsrv);
	SString temp_buf;
	H.PlanSessID    = p_filt->PlanSessID;
	H.SingleSessID  = p_filt->SessIdList.IsExists() ? p_filt->SessIdList.Get().getSingle() : 0;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	const TSessionFilt * p_tses_filt = p_filt->GetOuterTSessFilt();
	if(p_tses_filt) {
		H.FltPrcID  = p_tses_filt->PrcID;
		H.FltStBeg  = p_tses_filt->StPeriod.low;
		H.FltStEnd  = p_tses_filt->StPeriod.upp;
		H.FltStTime = p_tses_filt->StTime;
		H.FltFnBeg  = p_tses_filt->FnPeriod.low;
		H.FltFnEnd  = p_tses_filt->FnPeriod.upp;
		H.FltFnTime = p_tses_filt->FnTime;
		H.FltArID   = p_tses_filt->ArID;
		H.FltAr2ID  = p_tses_filt->Ar2ID;
	}
	H.FltFlags      = p_filt->Flags;
#define SETASSIGNVAR(var) H.var=p_filt->var
	SETASSIGNVAR(DiffPrc);
	SETASSIGNVAR(DiffMg);
	SETASSIGNVAR(DiffDt);
#undef  SETASSIGNVAR
#define SETFLAGVAR(var) H.var=BIN(p_filt->Flags&TSessAnlzFilt::var)
	SETFLAGVAR(fAll);
	SETFLAGVAR(fPrmrGoodsOnly);
	SETFLAGVAR(fShowRest);
	SETFLAGVAR(fExtrapolToPeriod);
	SETFLAGVAR(fAddTotalRows);
	SETFLAGVAR(fNmgRestrictOnly);
#undef SETFLAGVAR
	LDATETIME beg, end;
	if(p_tses_filt) {
		beg.Set(p_tses_filt->StPeriod.low, p_tses_filt->StTime);
		end.Set(p_tses_filt->StPeriod.upp, ZEROTIME);
		PPFormatPeriod(beg, end, temp_buf).CopyTo(H.StPeriod, sizeof(H.StPeriod));
		beg.Set(p_tses_filt->FnPeriod.low, p_tses_filt->FnTime);
		end.Set(p_tses_filt->FnPeriod.upp, ZEROTIME);
		PPFormatPeriod(beg, end, temp_buf).CopyTo(H.FnPeriod, sizeof(H.FnPeriod));
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TSessAnlz::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(TSessAnlz);
}

int PPALDD_TSessAnlz::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(TSessAnlz);
	I.Dt.v        = item.DtVal;
	I.PrcID       = item.PrcID;
	I.GoodsID     = item.GoodsID;
	STRNSCPY(I.PrmrGoodsName, item.PrmrGoodsName);
	STRNSCPY(I.GoodsName,     item.GoodsName);
	STRNSCPY(I.PrcName,       item.PrcName);
	STRNSCPY(I.DtText,        item.DtText);
	if(item.PrcID == MAXLONG) {
		// @v9.7.4 memset(I.Intrn_PrcName, 'я', sizeof(I.Intrn_PrcName)-1);
		// @v9.7.4 I.Intrn_PrcName[sizeof(I.Intrn_PrcName)-1] = 0;
		// @v9.7.4 SCharToOem(I.Intrn_PrcName);
		// @v9.7.4 {
		SString temp_buf;
		temp_buf.CatCharN('я', sizeof(I.Intrn_PrcName)-1).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(I.Intrn_PrcName, temp_buf);
		// } @v9.7.4
	}
	else
		STRNSCPY(I.Intrn_PrcName, I.PrcName);
	I.NotPrmrLine = item.NotPrmrLine;
	I.PrmrGoodsID = item.PrmrGoodsID;
	I.PlanPhUnit  = item.PlanPhUnit;
	I.InQtty      = item.InQtty;
	I.OutQtty     = item.OutQtty;
	I.InCompPart  = item.InCompPart;
	I.OutCompPart = item.OutCompPart;
	I.InQttyPh    = item.InQttyPh;
	I.OutQttyPh   = item.OutQttyPh;
	I.PlanInQtty  = item.PlanInQtty;
	I.PlanOutQtty = item.PlanOutQtty;
	I.OutRest     = item.OutRest;
	I.Dev         = 0.0;
	I.DevPct      = 0.0;
	if(item.InQtty) {
		I.Dev = item.InQtty - item.PlanInQtty;
		I.DevPct = fdivnz(100.0 * I.Dev, item.PlanInQtty);
	}
	else if(item.OutQtty) {
		I.Dev = item.OutQtty - item.PlanOutQtty;
		I.DevPct = fdivnz(100.0 * I.Dev, item.PlanOutQtty);
	}
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TSessAnlz::Destroy() { DESTROY_PPVIEW_ALDD(TSessAnlz); }
