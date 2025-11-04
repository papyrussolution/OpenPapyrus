// V_UNITEC.CPP
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Аналитическая таблица в стиле unit-economics 
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(UnitEc); UnitEcFilt::UnitEcFilt() : PPBaseFilt(PPFILT_UNITEC, 0, 0)
{
	SetFlatChunk(offsetof(UnitEcFilt, ReserveStart),
		offsetof(UnitEcFilt, Reserve)-offsetof(UnitEcFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

UnitEcFilt & FASTCALL UnitEcFilt::operator = (const UnitEcFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

PPViewUnitEc::InitialGoodsParam::InitialGoodsParam() : RcptQtty(0.0), ExpectedDemandPerWeek(0.0)
{
}

bool FASTCALL PPViewUnitEc::InitialGoodsParam::IsEq(const InitialGoodsParam & rS) const
{
	return (RcptQtty == rS.RcptQtty && ExpectedDemandPerWeek == rS.ExpectedDemandPerWeek);
}

PPViewUnitEc::Indicator::Indicator() : ID(0), Cls(0), Value(0.0)
{
}

PPViewUnitEc::IndicatorVector::IndicatorVector() : TSVector <Indicator>(), ID(0), AddendumID(0)
{
}

int PPViewUnitEc::IndicatorVector::Add(long bizScoreId, long bizScoreCls, double value)
{
	int    ok = -1;
	if(bizScoreId) {
		Indicator * p_entry = GetBsEntry(bizScoreId);
		if(p_entry) {
			p_entry->Value += value;
		}
		else {
			Indicator new_entry;
			new_entry.ID = bizScoreId;
			new_entry.Cls = 0; // Даже если bizScoreCls != 0 здесь - 0 поскольку для класса существует отдельный группирующий элемент.
			new_entry.Value = value;
			insert(&new_entry);
		}
		ok = 1;
	}
	if(bizScoreCls) {
		Indicator * p_entry = GetClsEntry(bizScoreCls);
		if(p_entry) {
			p_entry->Value += value;
		}
		else {
			Indicator new_entry;
			new_entry.ID = 0;
			new_entry.Cls = bizScoreCls; 
			new_entry.Value = value;
			insert(&new_entry);
		}
		ok = 1;
	}
	return ok;
}

void PPViewUnitEc::IndicatorVector::GetClsList(LongArray & rList) const
{
	rList.Z();
	for(uint i = 0; i < getCount(); i++) {
		const PPViewUnitEc::Indicator & r_item = at(i);
		if(r_item.Cls)
			rList.add(r_item.Cls);
	}
	rList.sortAndUndup();
}

const PPViewUnitEc::Indicator * PPViewUnitEc::IndicatorVector::GetClsEntryC(long cls) const
{
	const PPViewUnitEc::Indicator * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const PPViewUnitEc::Indicator & r_item = at(i);
		if(r_item.Cls == cls)
			p_result = &r_item;
	}
	return p_result;
}

const PPViewUnitEc::Indicator * PPViewUnitEc::IndicatorVector::GetBsEntryC(long bsID) const
{
	const PPViewUnitEc::Indicator * p_result = 0;
	for(uint i = 0; !p_result && i < getCount(); i++) {
		const PPViewUnitEc::Indicator & r_item = at(i);
		if(r_item.ID == bsID) {
			p_result = &r_item;
		}
	}
	return p_result;
}

PPViewUnitEc::Indicator * PPViewUnitEc::IndicatorVector::GetClsEntry(long cls) { return const_cast<PPViewUnitEc::Indicator *>(GetClsEntryC(cls)); }
PPViewUnitEc::Indicator * PPViewUnitEc::IndicatorVector::GetBsEntry(long bsID) { return const_cast<PPViewUnitEc::Indicator *>(GetBsEntryC(bsID)); }

double PPViewUnitEc::IndicatorVector::GetTotalIncome() const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const PPViewUnitEc::Indicator & r_item = at(i);
		if(r_item.Cls && PPObjBizScore2::IsBscCls_Income(r_item.Cls))
			result += r_item.Value;
	}
	return result;
}

double PPViewUnitEc::IndicatorVector::GetTotalExpense() const
{
	double result = 0.0;
	for(uint i = 0; i < getCount(); i++) {
		const PPViewUnitEc::Indicator & r_item = at(i);
		if(r_item.Cls && PPObjBizScore2::IsBscCls_Expense(r_item.Cls))
			result += r_item.Value;
	}
	return result;
}

PPViewUnitEc::FactorSet::FactorSet() : DaysCount(0), SaleQtty(0.0), RcptQtty(0.0), StorageDaysQtty(0.0)
{
}
		
PPViewUnitEc::FactorSet & PPViewUnitEc::FactorSet::Z()
{
	DaysCount = 0;
	SaleQtty = 0.0;
	RcptQtty = 0.0;
	StorageDaysQtty = 0.0;
	return *this;
}

PPViewUnitEc::PPViewUnitEc() : PPView(0, &Filt, PPVIEW_UNITEC, (implBrowseArray), 0), P_DsList(0)
{
}

PPViewUnitEc::~PPViewUnitEc()
{
	delete P_DsList;
}

int PPViewUnitEc::InitIteration()
{
	return -1;
}

int FASTCALL PPViewUnitEc::NextIteration(UnitEcViewItem * pItem)
{
	return 0;
}

/*virtual*/int PPViewUnitEc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		IndicatorList.freeAll();
		Filt.Period.Actualize(ZERODATE);
		MakeProcessingList(IndicatorList);
		ok = 1;
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int PPViewUnitEc::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class UnitEcFiltDialog : public TDialog {
		DECL_DIALOG_DATA(UnitEcFilt);
	public:
		UnitEcFiltDialog() : TDialog(DLG_UNITECFILT)
		{
			SetupCalPeriod(CTLCAL_UNITECFILT_PERIOD, CTL_UNITECFILT_PERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_UNITECFILT_PERIOD, Data.Period);
			AddClusterAssoc(CTL_UNITECFILT_FLAGS, 0, UnitEcFilt::fPivot);
			SetClusterData(CTL_UNITECFILT_FLAGS, Data.Flags);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetPeriodInput(this, CTL_UNITECFILT_PERIOD, &Data.Period);
			GetClusterData(CTL_UNITECFILT_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	UnitEcFilt * p_filt = static_cast<UnitEcFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(UnitEcFiltDialog, p_filt);
}

int PPViewUnitEc::EvaluateFactors(const IndicatorVector * pVec, FactorSet & rSet)
{
	int    ok = -1;
	if(pVec) {
		if(checkdate(Filt.Period.low) && checkdate(Filt.Period.upp)) {
			const long pl = Filt.Period.GetLength();
			if(pl > 0)
				rSet.DaysCount = static_cast<uint>(pl);
		}
		if(pVec->IgP.ExpectedDemandPerWeek > 0.0 && rSet.DaysCount) {
			rSet.SaleQtty = (pVec->IgP.ExpectedDemandPerWeek * rSet.DaysCount) / 7.0;
		}
		if(pVec->IgP.RcptQtty > 0.0) {
			rSet.RcptQtty = pVec->IgP.RcptQtty;
			if(rSet.SaleQtty > 0.0) {
				SETMIN(rSet.SaleQtty, rSet.RcptQtty); // Мы не можем рассматривать продажи большие, чем поступившее количество (если оно определено)
			}
			else {
				rSet.SaleQtty = rSet.RcptQtty;
			}
		}
		else {
			if(rSet.SaleQtty <= 0.0) {
				rSet.SaleQtty = 1.0;
			}
			rSet.RcptQtty = rSet.SaleQtty;
		}
	}
	return ok;
}

int PPViewUnitEc::MakeEntry(IndicatorVector * pVec, BrwItem * pItem)
{
	int    ok = -1;
	PPGoodsStruc gs;
	if(pVec) {
		pVec->SVector::clear();
		const PPID struc_id = pVec->AddendumID;
		if(GsObj.Get(struc_id, &gs) > 0 && gs.Rec.Flags & GSF_PRICEPLANNING) {
			gs.GoodsID = pVec->ID; // @mandatory Товар может быть не определен в структуре, а формулы без товара не работают.
			PPGoodsStruc::FormulaResolutionCache rcache(gs);
			PPBizScore2Packet bs_pack;
			if(pItem) {
				pItem->ID = pVec->ID;
				pItem->GStrucID = struc_id;
			}
			ok = 1;
			PPIDArray deferred_funding_cost_idx_list; // [1..gs.Items.getCount()]
			double funding_amount = 0.0;
			for(uint i = 0; i < gs.Items.getCount(); i++) {
				const PPGoodsStrucItem & r_item = gs.Items.at(i);
				if(r_item.Flags & GSIF_BIZSC2) {
					//
					// Будем обрабатывать только те элементы структуры, для которых определен класс показателя.
					// Просто пока не ясно как оперировать показателями без класса.
					//
					if(BsObj.Fetch(r_item.GoodsID, &bs_pack) > 0) {
						if(bs_pack.Rec.Cls) { 
							FactorSet factors;
							double value = 0.0;
							if(gs.GetItemValue(i, &rcache, &value) > 0) {
								double result_value = 0.0;
								EvaluateFactors(pVec, factors);
								//
								// Здесь будет грусто: придется перебрать все классы показателей и каждый обсчитать 
								// отдельно, ибо она так задуманы, что каждый задает собственную схему расчета.
								//
								switch(bs_pack.Rec.Cls) {
									case BSCCLS_INC_SALE_UNIT:
										{
											result_value = factors.SaleQtty * value;
										}
										break;
									case BSCCLS_EXP_SALE_UNIT:
										{
											result_value = factors.SaleQtty * value;
										}
										break;
									case BSCCLS_EXP_PURCHASE_UNIT:
										{
											result_value = factors.RcptQtty * value;
											funding_amount += result_value;
										}
										break;
									case BSCCLS_EXP_TRANSFER_UNIT:
										{
											result_value = factors.RcptQtty * value;
										}
										break;
									case BSCCLS_EXP_STORAGE_UNIT_TIME:
										{
											//
											// Стоимость хранения вычисляется как интеграл под кривой изменения запасов, умноженный на цену хранения
											// единицы в сутки.
											// Так как наша кривая изменения запасов очень незамысловатая, а именно, наклонная прямая от 
											// начального количества (factors.RcptQtty) до нуля с тангенсом наклона (factors.RcptQtty / Dayly_Demand),
											// то вычисление интеграла заменяется вычислением площади соответствующего прямоугольного треугольника.
											// А именно (factors.RcptQtty * (factors.RcptQtty / Dayly_Demand)) / 2
											// Если период времени, на который мы прогнозируем (factors.DaysCount), меньше чем (factors.RcptQtty / d), то
											// формула такова:
											// 
											// S = a * t - (a * t^2) / (2 * b) (1)
											// где t = factors.DaysCount
											//   a = factors.RcptQtty
											//   b = factors.RcptQtty / d
											// С учетом этого (b = a/d) сокращаем выражение до:
											// 
											// S = a * t - d/2 * t^2 (2)
											// 
											// Ну и t заменяем на MIN(factors.DaysCount, (factors.RcptQtty / Dayly_Demand)) и 
											// уравнение (2) становится инвариантным относительно того факта меньше расчетный период
											// времени истощения запасов или нет.
											//
											if(pVec->IgP.ExpectedDemandPerWeek > 0.0) {
												const double d = (pVec->IgP.ExpectedDemandPerWeek / 7.0);
												const double t = smin(static_cast<double>(factors.DaysCount), factors.RcptQtty / d);
												result_value = value * (factors.RcptQtty * t - d/2.0 * t * t);

												//result_value = part * ((factors.RcptQtty * factors.RcptQtty) * value) / (2.0 * d);
												
												//const uint n = MIN(factors.DaysCount, round(factors.RcptQtty / d, 0, +1));
												//double _E1 = (value * factors.RcptQtty * n);
												//double _E2 = (value * d * ((n + n * n) / 2));
												//result_value = _E1 - _E2;
											}
										}
										break;
									case BSCCLS_EXP_FUNDINGCOST:
										deferred_funding_cost_idx_list.add(i+1);
										break;
									case BSCCLS_EXP_PRESALE_UNIT:
										{
											result_value = factors.RcptQtty * value;
											funding_amount += result_value;
										}
										break;
									case BSCCLS_EXP_PRESALE_SKU_TIME:
										{
											const double d = (pVec->IgP.ExpectedDemandPerWeek / 7.0);
											const double t = smin(static_cast<double>(factors.DaysCount), factors.RcptQtty / d);
											if(t > 0.0) {
												result_value = value * t / 365.0;
											}
										}
										break;
									case BSCCLS_EXP_PROMO_SKU_TIME:
										break;
									case BSCCLS_EXP_PROMO_TIME:
										break;
								}
								pVec->Add(bs_pack.Rec.ID, bs_pack.Rec.Cls, result_value);
							}
						}
					}
				}
			}
			if(deferred_funding_cost_idx_list.getCount() && funding_amount > 0.0 && pVec->IgP.ExpectedDemandPerWeek > 0.0) {
				for(uint ri = 0; ri < deferred_funding_cost_idx_list.getCount(); ri++) {
					const uint gs_item_idx = deferred_funding_cost_idx_list.get(ri) - 1;
					assert(gs_item_idx >= 0 && gs_item_idx < gs.Items.getCount());
					const PPGoodsStrucItem & r_item = gs.Items.at(gs_item_idx);
					assert(r_item.Flags & GSIF_BIZSC2);
					assert(BsObj.Fetch(r_item.GoodsID, &bs_pack) > 0);
					assert(bs_pack.Rec.Cls);
					if(r_item.Flags & GSIF_BIZSC2 && BsObj.Fetch(r_item.GoodsID, &bs_pack) > 0 && bs_pack.Rec.Cls) {
						FactorSet factors;
						double value = 0.0;
						if(gs.GetItemValue(gs_item_idx, &rcache, &value) > 0) {
							double result_value = 0.0;
							EvaluateFactors(pVec, factors);
							assert(bs_pack.Rec.Cls == BSCCLS_EXP_FUNDINGCOST);
							if(bs_pack.Rec.Cls == BSCCLS_EXP_FUNDINGCOST) {
								//
								// Здесь при расчете те же соображения, что и при расчете стоимости хранения (see above BSCCLS_EXP_STORAGE_UNIT_TIME) //
								//
								const double _cost = funding_amount / factors.RcptQtty;
								const double p = (value / (100.0 * 365.0));
								//const double d = (pVec->IgP.ExpectedDemandPerWeek / 7.0);
								//const uint n = MIN(factors.DaysCount, round(factors.RcptQtty / d, 0, +1));
								//double _E1 = (p * factors.RcptQtty * _cost * n);
								//double _E2 = (p * d * _cost * ((n + n * n) / 2));
								//result_value = _E1 - _E2;
								{
									const double d = (pVec->IgP.ExpectedDemandPerWeek / 7.0);
									const double t = smin(static_cast<double>(factors.DaysCount), factors.RcptQtty / d);
									result_value = (_cost * p) * (factors.RcptQtty * t - d/2.0 * t * t);
								}
								pVec->Add(bs_pack.Rec.ID, bs_pack.Rec.Cls, result_value);
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPViewUnitEc::GetCommonIndicatorClsList(LongArray & rList)
{
	rList.Z();
	int    ok = 1;
	LongArray temp_list;
	for(uint i = 0; i < IndicatorList.getCount(); i++) {
		const IndicatorVector * p_vec = IndicatorList.at(i);
		if(p_vec) {
			p_vec->GetClsList(temp_list);
			rList.add(&temp_list);
		}
	}
	rList.sortAndUndup();
	rList.SVector::sort(PTR_CMPFUNC(BscCls));
	return ok;
}

const PPViewUnitEc::IndicatorVector * PPViewUnitEc::GetEntryByID_Const(long id) const
{
	const PPViewUnitEc::IndicatorVector * p_result = 0;
	for(uint i = 0; !p_result && i < IndicatorList.getCount(); i++) {
		const IndicatorVector * p_vec = IndicatorList.at(i);
		if(p_vec && p_vec->ID == id)
			p_result = p_vec;
	}
	return p_result;
}

PPViewUnitEc::IndicatorVector * PPViewUnitEc::GetEntryByID(long id) { return const_cast<PPViewUnitEc::IndicatorVector *>(GetEntryByID_Const(id)); }

int PPViewUnitEc::MakeProcessingList(TSCollection <IndicatorVector> & rList)
{
	rList.freeAll();
	int    ok = -1;
	GoodsFilt goods_filt;
	Goods2Tbl::Rec goods_rec;
	PPIDArray child_idlist;
	if(Filt.GoodsGroupID)
		goods_filt.GrpIDList.Add(Filt.GoodsGroupID);
	goods_filt.Flags |= GoodsFilt::fWithStrucOnly;
	for(GoodsIterator gi(&goods_filt, 0); gi.Next(&goods_rec) > 0;) {
		if(!(goods_rec.Flags & GF_PASSIV)) {
			PPGoodsStrucHeader2 gsh;
			if(GsObj.Fetch(goods_rec.StrucID, &gsh) > 0) {
				if(gsh.Flags & GSF_PRICEPLANNING) {
					IndicatorVector * p_new_item = rList.CreateNewItem();
					THROW_SL(p_new_item);
					p_new_item->ID = goods_rec.ID;
					p_new_item->AddendumID = goods_rec.StrucID;
					{
						GoodsStockExt gse;
						if(GObj.GetStockExt(goods_rec.ID, &gse, 1) > 0 && gse.Package > 0)
							p_new_item->IgP.RcptQtty = gse.Package;
						else
							p_new_item->IgP.RcptQtty = 1.0;
					}
					MakeEntry(p_new_item, 0);
				}
				else if(gsh.Flags & GSF_FOLDER) {
					child_idlist.Z();
					if(GsObj.GetChildIDList(gsh.ID, &child_idlist) > 0) {
						bool   local_done = false;
						for(uint i = 0; !local_done && i < child_idlist.getCount(); i++) {
							const PPID child_gs_id = child_idlist.get(i);
							if(GsObj.Fetch(child_gs_id, &gsh) > 0) {
								if(gsh.Flags & GSF_PRICEPLANNING) {
									IndicatorVector * p_new_item = rList.CreateNewItem();
									THROW_SL(p_new_item);
									p_new_item->ID = goods_rec.ID;
									p_new_item->AddendumID = child_gs_id;
									{
										GoodsStockExt gse;
										if(GObj.GetStockExt(goods_rec.ID, &gse, 1) > 0 && gse.Package > 0)
											p_new_item->IgP.RcptQtty = gse.Package;
										else
											p_new_item->IgP.RcptQtty = 1.0;
									}
									MakeEntry(p_new_item, 0);
									local_done = true;
								}
							}
						}
					}
				}
			}
		}
		//PPWaitPercent(gi.GetIterCounter());
	}
	if(rList.getCount())
		ok = 1;
	CATCHZOK
	return ok;
}

int PPViewUnitEc::MakeList(PPViewBrowser * pBrw)
{
	int    ok = -1;
	LongArray cls_list;
	bool   is_any_bscls = false;
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	if(Filt.Flags & UnitEcFilt::fPivot) {
		GetCommonIndicatorClsList(cls_list);
	}
	for(uint i = 0; i < IndicatorList.getCount(); i++) {
		const IndicatorVector * p_indicator_vec = IndicatorList.at(i);
		if(p_indicator_vec) {
			if(Filt.Flags & UnitEcFilt::fPivot) {
				if(p_indicator_vec->getCount()) {
					for(uint clsi = 0; clsi < cls_list.getCount(); clsi++) {
						const int cls = cls_list.get(clsi);
						double value = 0.0;
						const Indicator * p_ind = p_indicator_vec->GetClsEntryC(cls);
						if(p_ind) {
							value = p_ind->Value;
							BrwItem bi;
							bi.ID = p_indicator_vec->ID;
							bi.GStrucID = p_indicator_vec->AddendumID;
							bi.Cls = cls;
							bi.IndicatorID = p_ind->ID;
							P_DsList->insert(&bi);
						}
					}
				}
				else {
					BrwItem bi;
					bi.ID = p_indicator_vec->ID;
					bi.GStrucID = p_indicator_vec->AddendumID;
					bi.Cls = 0;
					bi.IndicatorID = 0;
					P_DsList->insert(&bi);
				}
			}
			else {
				BrwItem bi;
				bi.ID = p_indicator_vec->ID;
				bi.GStrucID = p_indicator_vec->AddendumID;
				bi.Cls = 0;
				bi.IndicatorID = 0;
				P_DsList->insert(&bi);
			}
		}
	}
	return ok;
}

int PPViewUnitEc::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	SString temp_buf;
	const BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
	switch(pBlk->ColumnN) {
		case 1: 
			GetGoodsName(p_item->ID, temp_buf);
			pBlk->Set(temp_buf);
			break;
		case 2: // InitialGoodsParam::RcptQtty
			{
				const IndicatorVector * p_vec = GetEntryByID_Const(p_item->ID);
				double value = p_vec ? p_vec->IgP.RcptQtty : 0.0;
				pBlk->Set(value);
			}
			break;
		case 3: // InitialGoodsParam::ExpectedDemandPerWeek
			{
				const IndicatorVector * p_vec = GetEntryByID_Const(p_item->ID);
				double value = p_vec ? p_vec->IgP.ExpectedDemandPerWeek : 0.0;
				pBlk->Set(value);
			}
			break;
		case 1001: // @construction (pivot) indicator name
			temp_buf.Z();
			if(p_item->Cls) {
				PPObjBizScore2::GetBscClsResultName(p_item->Cls, temp_buf);
				if(temp_buf.IsEmpty())
					temp_buf.CatChar('#').Cat("bscls").CatChar('-').Cat(p_item->Cls);
			}
			pBlk->Set(temp_buf);
			break;
		case 1002: // @construction (pivot) indicator value
			if(p_item->Cls) {
				const IndicatorVector * p_vec = GetEntryByID_Const(p_item->ID);
				double value = 0.0;
				if(p_vec) {
					const Indicator * p_ind = p_vec->GetClsEntryC(p_item->Cls);
					if(p_ind)
						value = p_ind->Value;
				}
				pBlk->Set(value);
			}
			break;
		default:
			if(pBlk->ColumnN == 4000) {
				double value = 0.0;
				const IndicatorVector * p_vec = GetEntryByID_Const(p_item->ID);
				if(p_vec) {
					const double income = p_vec->GetTotalIncome();
					const double expense = p_vec->GetTotalExpense();
					value = income - expense;
				}
				pBlk->Set(value);
			}
			else if(pBlk->ColumnN > 2000) {
				const int cls = (pBlk->ColumnN - 2000);
				const IndicatorVector * p_vec = GetEntryByID_Const(p_item->ID);
				double value = 0.0;
				if(p_vec) {
					const Indicator * p_ind = p_vec->GetClsEntryC(cls);
					if(p_ind)
						value = p_ind->Value;
				}
				pBlk->Set(value);
			}
			break;
	}
	return ok;
}

void PPViewUnitEc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	SString temp_buf;
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewUnitEc *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
	if(!(Filt.Flags & UnitEcFilt::fPivot)) {
		LongArray cls_list;
		GetCommonIndicatorClsList(cls_list);
		bool is_any_bscls = false;
		for(uint i = 0; i < cls_list.getCount(); i++) {
			const int cls = cls_list.get(i);
			const int column_id = (cls + 2000);
			PPObjBizScore2::GetBscClsResultName(cls, temp_buf);
			if(temp_buf.IsEmpty())
				temp_buf.CatChar('#').Cat("bscls").CatChar('-').Cat(cls);
			pBrw->InsColumn(-1, temp_buf, column_id, T_DOUBLE, MKSFMTD(0, 2, 0), BCO_USERPROC);
			is_any_bscls = true;
		}
		if(is_any_bscls) {
			const int column_id = 4000;
			PPLoadString("profit", temp_buf);
			pBrw->InsColumn(-1, temp_buf, column_id, T_DOUBLE, MKSFMTD(0, 2, 0), BCO_USERPROC);
		}
	}
}

/*virtual*/SArray * PPViewUnitEc::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	uint   brw_id = 0;
	if(Filt.Flags & UnitEcFilt::fPivot) {
		brw_id = BROWSER_UNITEC_PVT;
	}
	else {
		brw_id = BROWSER_UNITEC;
	}
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(p_array);
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

/*virtual*/int PPViewUnitEc::Detail(const void *, PPViewBrowser * pBrw) { return -1; } // @stub

int PPViewUnitEc::EditInitialGoodsParam(const BrwItem * pItem)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(pItem && pItem->ID) {
		IndicatorVector * p_vec = GetEntryByID(pItem->ID);
		if(p_vec) {
			dlg = new TDialog(DLG_UNITECIGP);
			if(CheckDialogPtr(&dlg)) {
				const InitialGoodsParam preserve_igp(p_vec->IgP);
				dlg->setCtrlReal(CTL_UNITECIGP_RCPTQTY, p_vec->IgP.RcptQtty);
				dlg->setCtrlReal(CTL_UNITECIGP_EWDEMAND, p_vec->IgP.ExpectedDemandPerWeek);
				while(ok < 0 && ExecView(dlg) == cmOK) {
					p_vec->IgP.RcptQtty = dlg->getCtrlReal(CTL_UNITECIGP_RCPTQTY);
					p_vec->IgP.ExpectedDemandPerWeek = dlg->getCtrlReal(CTL_UNITECIGP_EWDEMAND);
					ok = 1;
				}
				if(p_vec->IgP == preserve_igp)
					ok = -1;
			}
		}
	}
	delete dlg;
	return ok;
}

/*virtual*/int PPViewUnitEc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	const  BrwItem * p_item = static_cast<const BrwItem *>(pHdr);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS: 
				if(p_item && p_item->ID) {
					PPID   temp_id = p_item->ID;
					if(GObj.Edit(&temp_id, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_UNIIEC_IGP:
				{
					if(EditInitialGoodsParam(p_item) > 0) {
						IndicatorVector * p_vec = GetEntryByID(p_item->ID);
						MakeEntry(p_vec, 0);
						ok = 1;
					}
				}
				break;
		}
	}
	return ok;
}
