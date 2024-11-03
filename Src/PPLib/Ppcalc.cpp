// PPCALC.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000-2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

typedef int (*PPCalcFunc)(const StringSet * pParamList, char * pRes, size_t resBufLen);

int PPCFuncPaperRollOuterLayer(const StringSet *, char *, size_t);
int PPCFuncPaperRollUsagePart(const StringSet *, char *, size_t);

struct PPCalcFuncAssoc {
	uint16 FuncID;
	PPCalcFunc Func;
};

static const PPCalcFuncAssoc CFA[] = {
	{PPCFUNC_PAPER_ROLLOUTERLAYER, PPCFuncPaperRollOuterLayer},
	{PPCFUNC_PAPER_ROLLUSAGEPART,  PPCFuncPaperRollUsagePart}
};
//
// p1 - Diameter, p2 - Width, p3 - Outer sheets count
//
int PPCFuncPaperRollOuterLayer(const StringSet * pParamList, char * pRes, size_t resBufLen)
{
	int    ok = 1;
	uint   pos = 0;
	char   param[64];
	double D = 0.0, W = 0.0;
	long   N = 0;
	double R = 0.0;
	if(pParamList->get(&pos, param, sizeof(param))) {
		strtodoub(param, &D);
		if(pParamList->get(&pos, param, sizeof(param))) {
			strtodoub(param, &W);
			if(pParamList->get(&pos, param, sizeof(param)))
				strtolong(param, &N);
			else
				ok = 0;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	R = SMathConst::Pi * D * W * N;
	realfmt(R, MKSFMTD(0, 8, NMBF_NOTRAILZ), param);
	strnzcpy(pRes, param, resBufLen);
	return ok;
}
//
// p1 - Source Diam, p2 - Rest Diam, p3 - Bobbin Diam, p4 - Source weight
//
int PPCFuncPaperRollUsagePart(const StringSet * pParamList, char * pRes, size_t resBufLen)
{
	int    ok = 1;
	SString param;
	uint   pos = 0;
	double SD = 0.0, RD = 0.0, BD = 0.0, W = 0.0, R = 0.0;
	THROW(pParamList->get(&pos, param));
	SD = param.ToReal();
	THROW(pParamList->get(&pos, param));
	RD = param.ToReal();
	THROW(pParamList->get(&pos, param));
	BD = param.ToReal();
	THROW(pParamList->get(&pos, param));
	W = param.ToReal();
	R = W * (SD*SD - RD*RD);
	W = (SD*SD - BD*BD);
	THROW(W != 0.0);
	R = R / W;
	CATCH
		R = 0.0;
		ok = 0;
	ENDCATCH
    param.Z().Cat(R, MKSFMTD(0, 8, NMBF_NOTRAILZ));
	strnzcpy(pRes, param, resBufLen);
	return ok;
}

PPCalcFuncEntry::PPCalcFuncEntry() : FuncID(0), RetType(0), ParamCount(0), P_ParamTypeList(0)
{
}

PPCalcFuncEntry::~PPCalcFuncEntry()
{
	delete P_ParamTypeList;
}

PPCalcFuncList::PPCalcFuncList() : TSCollection <PPCalcFuncEntry> ()
{
}

int PPCalcFuncList::Load()
{
	int    ok = 1;
	Release();
	TVRez * p_rez = P_SlRez;
	if(p_rez) {
		uint   i, j, num_func = 0;
		THROW_PP(p_rez->findResource(CACLFUNC_DESCR, PP_RCDATA), PPERR_RESFAULT);
		THROW_PP(num_func = p_rez->getUINT(), PPERR_RESFAULT);
		for(i = 0; i < num_func; i++) {
			PPCalcFuncEntry * p_entry = new PPCalcFuncEntry;
			THROW_MEM(p_entry);
			p_entry->FuncID = p_rez->getUINT();
			p_rez->getString(p_entry->Name, 0);
			p_rez->getString(p_entry->Description, 0);
			p_entry->RetType    = p_rez->getUINT();
			p_entry->ParamCount = p_rez->getUINT();
			if(p_entry->ParamCount) {
				THROW_MEM(p_entry->P_ParamTypeList = new uint[p_entry->ParamCount]);
				for(j = 0; j < p_entry->ParamCount; j++)
					p_entry->P_ParamTypeList[j] = p_rez->getUINT();
			}
			THROW_SL(insert(p_entry));
		}
	}
	CATCH
		freeAll();
		ok = 0;
	ENDCATCH
	return ok;
}

void PPCalcFuncList::Release()
{
	freeAll();
}

uint16 FASTCALL PPCalcFuncList::SearchFuncByName(const char * pName) const
{
	const uint c = getCount();
	for(uint i = 0; i < c; i++) {
		PPCalcFuncEntry * p_entry = at(i);
		if(p_entry->Name.CmpNC(pName) == 0)
			return p_entry->FuncID;
	}
	return PPSetError(PPERR_UNDEFPPCFUNCID);
}

const PPCalcFuncEntry * FASTCALL PPCalcFuncList::SearchFunc(uint16 funcID) const
{
	const uint c = getCount();
	for(uint i = 0; i < c; i++)
		if(at(i)->FuncID == funcID)
			return at(i);
	return (PPErrCode = PPERR_UNDEFPPCFUNCID, (PPCalcFuncEntry *)0);
}

int PPCalcFuncList::ReadParams(uint16 funcID, const char * pStr, size_t * pEndPos, StringSet * pParamList) const
{
	int    ok = 1;
	size_t p = 0;
	const  PPCalcFuncEntry * p_entry = 0;
	THROW(p_entry = SearchFunc(funcID));
	{
		int    lparent = 0;
		while(pStr[p] == ' ' || pStr[p] == '\t' || pStr[p] == '\n')
			p++;
		if(pStr[p] == '(') {
			lparent = 1;
			p++;
		}
		for(uint i = 0; i < p_entry->ParamCount; i++) {
			while(pStr[p] == ' ' || pStr[p] == '\t' || pStr[p] == '\n')
				p++;
			char   param_buf[64];
			const  char * p_delim = sstrchr(pStr+p, ',');
			size_t delim_dist = p_delim ? (p_delim - pStr) : static_cast<size_t>(MAXSHORT);
			char * p_dest = param_buf;
			while(pStr[p] != 0 && p != delim_dist && pStr[p] != ')')
				*p_dest++ = pStr[p++];
			if(p_delim && pStr[p] == *p_delim)
				p++;
			*p_dest = 0;
			if(oneof2(p_entry->P_ParamTypeList[i], BTS_REAL, BTS_INT))
				strip(param_buf);
			pParamList->add(param_buf);
			if(pStr[p] == ')' || pStr[p] == 0)
				THROW_PP(i == (p_entry->ParamCount-1), PPERR_INVPPCFUNCPARCOUNT);
		}
		while(pStr[p] == ' ' || pStr[p] == '\t' || pStr[p] == '\n')
			p++;
		if(lparent) {
			THROW_PP_S(pStr[p] == ')', PPERR_INVEXPR, pStr);
			p++;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pEndPos, p);
	return ok;
}

int PPCalcFuncList::CalcFunc(uint16 funcID, const StringSet * pParams, char * pRes, size_t resBufLen) const
{
	int    ok = -1;
	//const PPCalcFuncEntry * p_entry = 0;
	//THROW(p_entry = SearchFunc(funcID));
	ASSIGN_PTR(pRes, 0);
	for(uint i = 0; i < SIZEOFARRAY(CFA); i++) {
		if(CFA[i].FuncID == funcID) {
			THROW(CFA[i].Func(pParams, pRes, resBufLen));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

double PPRound(double v, double prec, int dir)
{
	static const double __tolerance = 1.0e-10;
	// @todo return round(v, prec, dir);
	if(prec == 0.0)
		prec = fpow10i(-2);
	const double r = v / prec;
	const double _fl = floor(r);
	const double _cl = ceil(r);
	if(dir > 0)
		return (prec * (feqeps(_fl, r, __tolerance) ? _fl : _cl));
	else if(dir < 0)
		return (prec * (feqeps(_cl, r, __tolerance) ? _cl : _fl));
	else {
		const double down = prec * _fl;
		const double up   = prec * _cl;
		return ((2.0 * v - down - up) < (-__tolerance)) ? down : up;
	}
}
//
//
//
CalcPriceParam::CalcPriceParam()
{
	THISZERO();
}

int CalcPriceParam::Save() const
{
	// VaPercent, RoundPrec, RoundDir, fRoundVat, fVatAboveAddition
	WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 0);
	StringSet ss(';', 0);
	char   temp_buf[64];
	ss.add(realfmt(VaPercent, MKSFMTD(0, 4, 0), temp_buf));
	ss.add(realfmt(RoundPrec, MKSFMTD(0, 4, 0), temp_buf));
	ss.add(intfmt(RoundDir, 0, temp_buf));
	ss.add(intfmt(BIN(Flags & fRoundVat), 0, temp_buf));
	ss.add(intfmt(BIN(Flags & fVatAboveAddition), 0, temp_buf));
	reg_key.PutString(PPConst::WrParam_CalcPriceParam, ss.getBuf());
	return 1;
}

int CalcPriceParam::Restore()
{
	// VaPercent, RoundPrec, RoundDir, fRoundVat, fVatAboveAddition
	WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 1);
	//char   buf[128];
	SString temp_buf;
	if(reg_key.GetString(PPConst::WrParam_CalcPriceParam, temp_buf)) {
		StringSet ss(';', temp_buf);
		uint   pos = 0;
		if(ss.get(&pos, temp_buf)) {
			VaPercent = R4(satof(temp_buf));
			if(ss.get(&pos, temp_buf)) {
				RoundPrec = R4(satof(temp_buf));
				if(ss.get(&pos, temp_buf)) {
					RoundDir = static_cast<int16>(atol(temp_buf));
					if(ss.get(&pos, temp_buf)) {
						SETFLAG(Flags, fRoundVat, atol(temp_buf));
						if(ss.get(&pos, temp_buf))
							SETFLAG(Flags, fVatAboveAddition, atol(temp_buf));
						return 1;
					}
				}
			}
		}
	}
	return -1;
}
//
// Descr: Возвращает минимальный множитель, цены кратные которому
//   дают расчет суммы НДС без остатка.
//   Множитель возвращается долях денежной единицы, определяемых параметром prec
// ARG(rate IN): ставка НДС (в долях от единицы, например - 0.20 (20%))
// ARG(prec IN): @{0..6} точность представления результата. 0 - до целых значений,
//   3 - с точностью 0.001 и т.д.
//
ulong GetMinVatDivisor(double rate, uint prec)
{
	assert(prec >= 0 && prec <= 6);
	const double _mult = fpow10i(prec + 2);
    const ulong  r = static_cast<ulong>(R6(rate) * _mult);
    const ulong  d = static_cast<ulong>((1.0 + R6(rate)) * _mult);
    UlongArray r_list, d_list;
    Factorize(r, &r_list);
    Factorize(d, &d_list);
    MutualReducePrimeMultiplicators(d_list, r_list, 0);
	ulong v = 1;
	SForEachVectorItem(d_list, i) { v *= d_list.at(i); }
	return v;
}
//
//
//
double CalcPriceParam::Calc(double inPrice, double * pVatRate, double * pVatSum, double * pExcise) const
{
	GTaxVect gtv;
	PPGoodsTaxEntry gte;
	double tax_factor = 1.0;
	double vat_rate = 0.0;
	int    calc_taxes = 0;
	Goods2Tbl::Rec goods_rec;
	PPObjGoods goods_obj;
	if(Flags & fCostWoVat) {
		if(!(Flags & fVatAboveAddition) && goods_obj.Fetch(GoodsID, &goods_rec) > 0)
			goods_obj.AdjCostToVat(InTaxGrpID, goods_rec.TaxGrpID, Dt, 1, &inPrice, 1, 0);
	}
	else if(Flags & fVatAboveAddition) {
		if(goods_obj.Fetch(GoodsID, &goods_rec) > 0)
			goods_obj.AdjCostToVat(InTaxGrpID, goods_rec.TaxGrpID, Dt, 1, &inPrice, 0, 0);
	}
	double price = inPrice;
	if(goods_obj.FetchTax(GoodsID, /*Dt*/ZERODATE, 0, &gte) > 0) {
		goods_obj.MultTaxFactor(GoodsID, &tax_factor);
		gtv.Calc_(&gte, price, tax_factor, GTAXVF_AFTERTAXES, 0);
		vat_rate = gtv.GetTaxRate(GTAX_VAT, 0);
		price = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
		calc_taxes = 1;
	}
	price += price * fdiv100r(VaPercent);
	//
	if(calc_taxes && Flags & fVatAboveAddition) {
		//
		// @v4.8.7
		// По видимому, следующая строка лишняя (она приводит к неверному значению tax_factor).
		// Не понятно только каким образом те клиенты, которые в течении, как минимум 1.5 года
		// использовали эту функцию не матюгались.
		//
		// goods_obj.MultTaxFactor(GoodsID, &tax_factor);
		gtv.Calc_(&gte, price, tax_factor, GTAXVF_AFTERTAXES | GTAXVF_EXCISE, 0);
		price = gtv.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE | GTAXVF_VAT);
	}
	//
	if(Flags & fRoundVat) {
		//
		// Расчет цены кратной ставке НДС
		//
		ulong  div = GetMinVatDivisor(fdiv100r(vat_rate), 2);
		ulong  p   = static_cast<ulong>(R0i(price * 100.0));
		ulong  n   = p / div;
		ulong  mod = p % div;
		if(mod != 0)
			if(RoundDir < 0 || (RoundDir == 0 && mod < (div-mod)))
				price = R2(fdiv100i(n * div));
			else //if(RoundDir > 0 || (RoundDir == 0 && mod >= (div-mod)))
				price = R2(fdiv100i((n+1) * div));
	}
	else
		price = PPRound(price, RoundPrec, RoundDir);
	//
	// После окончательного расчета цены необходимо заново рассчитать налоги
	//
	double excise = 0.0, vat_sum = 0.0;
	if(calc_taxes) {
		gtv.Calc_(&gte, price, tax_factor, GTAXVF_BEFORETAXES, 0);
		excise   = gtv.GetValue(GTAXVF_EXCISE);
		vat_sum  = gtv.GetValue(GTAXVF_VAT);
		if(Flags & fExclTaxes)
			price = gtv.GetValue(GTAXVF_AFTERTAXES);
	}
	ASSIGN_PTR(pVatRate, vat_rate);
	ASSIGN_PTR(pVatSum,  vat_sum);
	ASSIGN_PTR(pExcise,  excise);
	return price;
}
//
//
//
class CalcPriceDialog : public TDialog {
	DECL_DIALOG_DATA(CalcPriceParam);
public:
	CalcPriceDialog() : TDialog(DLG_CALCPRICE)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SETIFZ(Data.VaPercent, DS.GetTLA().Lid.VaPercent);
		int    is_price_wo_taxes = 0;
		Goods2Tbl::Rec goods_rec;
		if(Data.GoodsID && GObj.Search(Data.GoodsID, &goods_rec) > 0) {
			setCtrlData(CTL_CALCPRICE_GOODS, goods_rec.Name);
			is_price_wo_taxes = BIN(goods_rec.Flags & GF_PRICEWOTAXES);
		}
		setCtrlData(CTL_CALCPRICE_COST, &Data.Cost);
		setCtrlData(CTL_CALCPRICE_PERCENT, &Data.VaPercent);
		SETFLAG(Data.Flags, CalcPriceParam::fExclTaxes, is_price_wo_taxes);
		AddClusterAssoc(CTL_CALCPRICE_EXCLTAXES, 0, CalcPriceParam::fExclTaxes);
		AddClusterAssoc(CTL_CALCPRICE_EXCLTAXES, 1, CalcPriceParam::fVatAboveAddition);
		SetClusterData(CTL_CALCPRICE_EXCLTAXES, Data.Flags);
		setCtrlData(CTL_CALCPRICE_PREC, &Data.RoundPrec);
		AddClusterAssocDef(CTL_CALCPRICE_ROUND, 0, 0);
		AddClusterAssoc(CTL_CALCPRICE_ROUND, 1, -1);
		AddClusterAssoc(CTL_CALCPRICE_ROUND, 2, +1);
		SetClusterData(CTL_CALCPRICE_ROUND, Data.RoundDir);
		AddClusterAssoc(CTL_CALCPRICE_ROUNDVAT, 0, CalcPriceParam::fRoundVat);
		SetClusterData(CTL_CALCPRICE_ROUNDVAT, Data.Flags);
		if(!(Data.Flags & CalcPriceParam::fCostWoVat))
			setStaticText(CTL_CALCPRICE_COSTWOVAT, 0);
		SetupPrice();
		disableCtrl(CTL_CALCPRICE_COST, 1);
		selectCtrl(CTL_CALCPRICE_PERCENT);
		TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_CALCPRICE_PERCENT));
		CALLPTRMEMB(p_il, selectAll(1));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		SetupPrice();
		DS.GetTLA().Lid.VaPercent = Data.VaPercent;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
	//
	// Descr: Считывает параметры расчета из полей диалога и рассчитывает
	//   цену исходя из этих параметров. Наконец, устанавливает расчитанную цену
	//   в поле CTL_CALCPRICE_PRICE
	// Returns:
	//   !0 - если цена в поле изменилась (разница между старой и новой ценой более 0.005)
	//   0 -  в противном случае.
	//
	int    SetupPrice()
	{
		long   temp_long = 0;
		double old_price = getCtrlReal(CTL_CALCPRICE_PRICE);
		double cost = getCtrlReal(CTL_CALCPRICE_COST);
		getCtrlData(CTL_CALCPRICE_PERCENT, &Data.VaPercent);
		GetClusterData(CTL_CALCPRICE_EXCLTAXES, &Data.Flags);
		getCtrlData(CTL_CALCPRICE_PREC,        &Data.RoundPrec);
		GetClusterData(CTL_CALCPRICE_ROUNDVAT, &Data.Flags);
		GetClusterData(CTL_CALCPRICE_ROUND, &(temp_long = 0));
		Data.RoundDir = (int16)temp_long;
		double vat_rate = 0.0, vat_sum = 0.0, excise = 0.0;
		double price = Data.Calc(cost, &vat_rate, &vat_sum, &excise);
		setCtrlData(CTL_CALCPRICE_EXCISE,  &excise);
		setCtrlData(CTL_CALCPRICE_VATRATE, &vat_rate);
		setCtrlData(CTL_CALCPRICE_VATSUM,  &vat_sum);
		setCtrlData(CTL_CALCPRICE_PRICE,   &price);
		Data.Price = price;
		return BIN(fabs(price - old_price) > 0.005);
	}
private:
	DECL_HANDLE_EVENT
	{
		if(event.isCmd(cmOK) && SetupPrice()) {
			selectCtrl(CTL_CALCPRICE_PRICE);
			clearEvent(event);
		}
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_CALCPRICE_EXCLTAXES) || event.isClusterClk(CTL_CALCPRICE_ROUND) || event.isClusterClk(CTL_CALCPRICE_ROUNDVAT))
			SetupPrice();
		else if(TVBROADCAST && (TVCMD == cmReleasedFocus || TVCMD == cmCommitInput)) {
			if(event.isCtlEvent(CTL_CALCPRICE_PERCENT) || event.isCtlEvent(CTL_CALCPRICE_PREC))
				SetupPrice();
			else
				return;
		}
		else
			return;
		clearEvent(event);
	}
	PPObjGoods GObj;
};

int CalcPrice(CalcPriceParam * pParam)
{
	return PPDialogProcBody<CalcPriceDialog, CalcPriceParam>(pParam);
}
//
//
//
class CalcDiffDialog : public TDialog {
public:
	CalcDiffDialog() : TDialog(DLG_CALCDIFF_L /* @v7.0.6 IsLargeDlg() ? DLG_CALCDIFF_L : DLG_CALCDIFF*/)
	{
	}
	int  setupDiff(); // Returns: !0 если цена в поле изменилась и 0 в противном случае.
private:
	DECL_HANDLE_EVENT;
};

int CalcDiffDialog::setupDiff()
{
	double old_diff = round(getCtrlReal(CTL_CALCDIFF_DIFF), 4);
	double diff = round(getCtrlReal(CTL_CALCDIFF_CASH) - getCtrlReal(CTL_CALCDIFF_AMOUNT), 4);
	return (diff != old_diff) ? (setCtrlReal(CTL_CALCDIFF_DIFF, diff), 1) : 0;
}

IMPL_HANDLE_EVENT(CalcDiffDialog)
{
	if(event.isCmd(cmOK) && setupDiff()) {
		selectCtrl(CTL_CALCDIFF_CASH);
		clearEvent(event);
	}
	TDialog::handleEvent(event);
	if(event.isCmd(cmInputUpdated)) {
		if(event.isCtlEvent(CTL_CALCDIFF_CASH))
			setupDiff();
	}
	else if(TVBROADCAST && (TVCMD == cmReleasedFocus || TVCMD == cmCommitInput))
		if(event.isCtlEvent(CTL_CALCDIFF_CASH))
			setupDiff();
}

int CalcDiff(double amount, double * pDiff)
{
	int    ok = -1, valid = 0;
	double diff = DEREFPTRORZ(pDiff);
	double cash = amount + diff;
	CalcDiffDialog * dlg = new CalcDiffDialog();
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlData(CTL_CALCDIFF_DIFF,   &diff);
		dlg->setCtrlData(CTL_CALCDIFF_AMOUNT, &amount);
		dlg->setCtrlData(CTL_CALCDIFF_CASH,   &cash);
		dlg->setupDiff();
		dlg->selectCtrl(CTL_CALCDIFF_CASH);
		while(!valid && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_CALCDIFF_CASH, &cash);
			dlg->getCtrlData(CTL_CALCDIFF_DIFF, &diff);
			if(diff < 0.0)
				PPErrorByDialog(dlg, CTL_CALCDIFF_CASH, PPERR_CASH_LT_PAYMENT);
			else
				ok = valid = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	ASSIGN_PTR(pDiff, diff);
	return ok;
}
//
// Калькулятор общего назначения //
//
int PPCalculator(void * hParentWnd, const char * pInitData)
{
	class CalcDialog : public TDialog {
	public:
		CalcDialog() : TDialog(DLG_CALC), Err(0), Prec(0), Result(0.0)
		{
			CFL.Load();
			enableCommand(cmCalcEq, 1);
			setState(sfFocused, true);
			setCtrlReal(CTL_CALC_MEMVIEW, DS.GetTLA().Lid.CalcMem);
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmCancel) || (TVKEYDOWN && TVKEY == kbEsc)) {
				DS.GetTLA().Lid.CalcMem = getCtrlReal(CTL_CALC_MEMVIEW);
			}
			TDialog::handleEvent(event);
			if(TVCOMMAND)
				switch(TVCMD) {
					case cmCalcEq:
						{
							Calc();
							if(Err) {
								TInputLine * il = static_cast<TInputLine *>(getCtrlView(CTL_CALC_RESULT));
								if(il) {
									il->setText("Error");
									il->selectAll(true);
								}
							}
							else
								setCtrlString(CTL_CALC_RESULT, ResultText);
						}
						break;
					case cmCalcMem:      memToInput();     break;
					case cmCalcPlusMem:  memPlusResult(0); break;
					case cmCalcMinusMem: memPlusResult(1); break;
					default: return;
				}
			else if(TVKEYDOWN)
				if(TVKEY == kbAltM)
					memToInput();
				else if(TVKEY == kbAltEqual)
					memPlusResult(0);
				else
					return;
			else
				return;
			clearEvent(event);
		}
		void   Calc()
		{
			ResultText.Z();
			Result = 0.0;
			Err = 0;
			SString input;
			getCtrlString(CTL_CALC_INPUT, input);
			if(input.NotEmptyS()) {
				bool done = false;
				// @v11.0.2 {
				{
					GtinStruc gts;
					if(PPChZnPrcssr::InterpretChZnCodeResult(PPChZnPrcssr::ParseChZnCode(input, gts, PPChZnPrcssr::pchzncfPretendEverythingIsOk)) > 0) {
						SString temp_buf;
						SString serial_buf;
						gts.GetToken(GtinStruc::fldGTIN14, &temp_buf);
						gts.GetToken(GtinStruc::fldSerial, &serial_buf);
						if(temp_buf.NotEmpty() && serial_buf.NotEmpty()) {
							(ResultText = temp_buf).Cat(serial_buf);
							done = true;
						}
					}
				}
				// } @v11.0.2 
				if(!done) {
					input.ReplaceChar(',', '.');
					Err = !PPCalcExpression(input, &Result, &CFL);
					if(Err)
						ResultText.Cat("ERROR");
					else
						ResultText.Cat(Result, MKSFMTD(0, 12, NMBF_NOTRAILZ|NMBF_OMITEPS));
				}
			}
		}
		void   memToInput()
		{
			TInputLine * mil = static_cast<TInputLine *>(getCtrlView(CTL_CALC_MEMVIEW));
			TInputLine * iil = static_cast<TInputLine *>(getCtrlView(CTL_CALC_INPUT));
			if(mil && iil) {
				SString input, mem;
				const size_t pos = iil->getCaret();
				mil->getText(mem);
				iil->getText(input);
				input.Insert(pos, mem.Quot(' ', ' '));
				iil->setText(input);
				iil->Draw_();
				iil->selectAll(0);
				iil->setCaret(pos + mem.Len());
			}
		}
		void   memPlusResult(int minus)
		{
			double m = getCtrlReal(CTL_CALC_MEMVIEW);
			Calc();
			setCtrlReal(CTL_CALC_MEMVIEW, faddwsign(m, Result, minus ? -1 : +1));
		}

		int    Err;
		int    Prec;
		double Result;
		SString ResultText;
		PPCalcFuncList CFL;
	};
	MemLeakTracer mlt;
	int    ok = 1;
	double c = 0.0;
	char   ct[256];
	if(pInitData)
		STRNSCPY(ct, pInitData);
	else
		TView::messageCommand(APPL->P_DeskTop, cmGetFocusedNumber, &c);
	CalcDialog * dlg = new CalcDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(c != 0) {
			realfmt(c, MKSFMTD(0, 6, NMBF_NOTRAILZ), ct);
			dlg->setCtrlData(CTL_CALC_INPUT, ct);
		}
		ExecViewAndDestroy(dlg);
	}
	else
		ok = 0;
	return ok;
}
//
//
//
struct CalcTaxPriceParam {
	CalcTaxPriceParam() : GoodsID(0), Dt(ZERODATE), OpID(0), PriceWoTaxes(0.0), Price(0.0), CalcWotaxByWtax(0)
	{
	}
	PPID   GoodsID;
	LDATE  Dt;
	PPID   OpID;
	double PriceWoTaxes;
	double Price;
	int    CalcWotaxByWtax;
};

class CalcTaxPriceDialog : public TDialog {
	DECL_DIALOG_DATA(CalcTaxPriceParam);
public:
	CalcTaxPriceDialog() : TDialog(DLG_CALCTAXPRICE)
	{
		SetupCalDate(CTLCAL_CALCTAXPRICE_DT, CTL_CALCTAXPRICE_DT);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		ushort v = 0;
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(Data.GoodsID, &goods_rec) > 0) {
			setStaticText(CTL_CALCTAXPRICE_GOODS, GetGoodsName(Data.GoodsID, temp_buf));
			GetObjectName(PPOBJ_GOODSTAX, goods_rec.TaxGrpID, temp_buf);
			setStaticText(CTL_CALCTAXPRICE_TG, temp_buf);
		}
		else {
			setStaticText(CTL_CALCTAXPRICE_GOODS, temp_buf);
			setStaticText(CTL_CALCTAXPRICE_TG,    temp_buf);
		}
		setCtrlData(CTL_CALCTAXPRICE_DT, &Data.Dt);
		SetupPPObjCombo(this, CTLSEL_CALCTAXPRICE_OP, PPOBJ_OPRKIND, Data.OpID, 0, 0);
		setCtrlData(CTL_CALCTAXPRICE_WOTAX,   &Data.PriceWoTaxes);
		setCtrlData(CTL_CALCTAXPRICE_WTAX,    &Data.Price);
		setCtrlUInt16(CTL_CALCTAXPRICE_CALCFLG, BIN(Data.CalcWotaxByWtax));
		disableCtrl(CTL_CALCTAXPRICE_WOTAX,    Data.CalcWotaxByWtax);
		disableCtrl(CTL_CALCTAXPRICE_WTAX,    !Data.CalcWotaxByWtax);
		calc();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		getCtrlData(CTL_CALCTAXPRICE_DT,      &Data.Dt);
		getCtrlData(CTLSEL_CALCTAXPRICE_OP,   &Data.OpID);
		getCtrlData(CTL_CALCTAXPRICE_WOTAX,   &Data.PriceWoTaxes);
		getCtrlData(CTL_CALCTAXPRICE_WTAX,    &Data.Price);
		Data.CalcWotaxByWtax = BIN(getCtrlUInt16(CTL_CALCTAXPRICE_CALCFLG));
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	void   calc();

	PPObjGoods GObj;
};

void CalcTaxPriceDialog::calc()
{
	getDTS(0);
	PPGoodsTaxEntry gtx;
	if(GObj.FetchTax(Data.GoodsID, Data.Dt, Data.OpID, &gtx) > 0) {
		double tax_factor = 1.0;
		GObj.MultTaxFactor(Data.GoodsID, &tax_factor);
		if(Data.CalcWotaxByWtax) {
			GTaxVect vect;
			vect.Calc_(&gtx, Data.Price, tax_factor, GTAXVF_BEFORETAXES, GTAXVF_SALESTAX);
			Data.PriceWoTaxes = R2(vect.GetValue(GTAXVF_AFTERTAXES));
		}
		else {
			double price = Data.PriceWoTaxes;
			GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &price, 1);
			Data.Price = price;
 		}
		setCtrlData(CTL_CALCTAXPRICE_WOTAX, &Data.PriceWoTaxes);
		setCtrlData(CTL_CALCTAXPRICE_WTAX,  &Data.Price);
	}
}

IMPL_HANDLE_EVENT(CalcTaxPriceDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmCBSelected))
		calc();
	else if(event.isClusterClk(CTL_CALCTAXPRICE_CALCFLG)) {
		ushort v = getCtrlUInt16(CTL_CALCTAXPRICE_CALCFLG);
		disableCtrl(CTL_CALCTAXPRICE_WOTAX, v);
		disableCtrl(CTL_CALCTAXPRICE_WTAX, !v);
	}
	else if(TVBROADCAST)
		if(oneof2(TVCMD, cmReceivedFocus, cmCommitInput))
			calc();
		else
			return;
	else
		return;
	clearEvent(event);
}

int CalcTaxPrice(PPID goodsID, PPID opID, LDATE dt, double price, int /*= 0*/)
{
	int    ok = -1;
	CalcTaxPriceParam param;
	param.GoodsID = goodsID;
	param.OpID = opID;
	param.Dt   = dt;
	param.PriceWoTaxes = price;
	CalcTaxPriceDialog * dlg = new CalcTaxPriceDialog;
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&param);
		ExecViewAndDestroy(dlg);
		ok = 1;
	}
	else
		ok = 0;
	return ok;
}
//
//
//
PosPaymentBlock::PosPaymentBlock(const CcAmountList * pCcPl, double bonusMaxPart)
{
	Z();
	BonusAmt = BonusRest;
	Kind = cpmCash;
	DeliveryAmt = NoteAmt - CashAmt;
	DisabledKinds = 0;
	RVALUEPTR(CcPl, pCcPl);
	BonusMaxPart = bonusMaxPart;
}

IMPL_INVARIANT_C(PosPaymentBlock)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(oneof3(Kind, cpmCash, cpmBank, cpmIncorpCrd), pInvP);
	S_ASSERT_P((Amount + BonusAmt) == (CashAmt + BankAmt + ScAmt), pInvP);
	S_ASSERT_P(BankAmt == 0.0 || Kind == cpmBank, pInvP);
	S_ASSERT_P(ScAmt == 0.0 || SCardID != 0, pInvP);
	S_ASSERT_P(BonusRest >= 0.0, pInvP);
	S_ASSERT_P(BonusAmt >= 0.0, pInvP);
	S_ASSERT_P(BonusAmt <= BonusRest, pInvP);
	S_ASSERT_P(DeliveryAmt == (NoteAmt - CashAmt), pInvP);
	S_ASSERT_P(oneof3(EAddr.AddrType, 0, SNTOK_EMAIL, SNTOK_PHONE), pInvP); // @v11.3.6
	S_ASSERT_P(EAddr.EAddr.IsEmpty() || oneof2(EAddr.AddrType, SNTOK_EMAIL, SNTOK_PHONE), pInvP); // @v11.3.6
	S_ASSERT_P(EAddr.AddrType == 0 || EAddr.EAddr.NotEmpty(), pInvP); // @v11.3.6
	S_ASSERT_P(!(Flags & fPaperless) || !EAddr.IsEmpty(), pInvP); // @v11.3.6
	S_INVARIANT_EPILOG(pInvP);
}

PosPaymentBlock & PosPaymentBlock::Z()
{
	Amount = 0.0;
	CashAmt = BankAmt = ScAmt = 0.0;
	NoteAmt = 0.0;
	SCardID = 0;
	SCardRest = 0.0;
	BonusRest = 0.0;
	BonusAmt = BonusRest;
	ExclSCardID = 0;
	Kind = cpmCash;
	DeliveryAmt = NoteAmt - CashAmt;
	DisabledKinds = 0;
	// @v11.3.6 AltCashReg = -1;
	Flags = 0; // @v11.3.6
	SetBuyersEAddr(0, 0); // @v11.3.6
	CcPl.freeAll();
	BonusMaxPart = 0.0;
	UsableBonus = 0.0;
	AmtToPaym = 0.0;
	Kind = cpmUndef;
	Total = 0.0;
	Discount = 0.0;
	return *this;
}

PosPaymentBlock & PosPaymentBlock::Init(const CPosProcessor * pCpp)
{
	assert(pCpp);
	Z();
	const CPosProcessor::CcTotal cct = pCpp->CalcTotal();
	Total = cct.Amount;
	Discount = cct.Discount;
	UsableBonus = pCpp->GetUsableBonus();
	BonusMaxPart = pCpp->GetBonusMaxPart();
	return *this;
}

bool PosPaymentBlock::SetBuyersEAddr(int addrType, const char * pAddr)
{
	bool    ok = true;
	THROW(oneof3(addrType, 0, SNTOK_EMAIL, SNTOK_PHONE));
	THROW(isempty(pAddr) || oneof2(addrType, SNTOK_EMAIL, SNTOK_PHONE));
	THROW(addrType == 0 || !isempty(pAddr));
	EAddr.AddrType = addrType;
	(EAddr.EAddr = pAddr).Strip();
	CATCHZOK
	return ok;
}

double PosPaymentBlock::GetTotal() const { return Total; }
double PosPaymentBlock::GetDiscount() const { return Discount; }
double PosPaymentBlock::GetPctDiscount() const { return fdivnz((Discount * 100.0), (Total + Discount)); }
double PosPaymentBlock::GetUsableBonus() const { return UsableBonus; }

int PosPaymentBlock::EditDialog2()
{
	class PosPaymentDialog2 : public PPListDialog {
		DECL_DIALOG_DATA(PosPaymentBlock);
		enum {
			dummyFirst = 1,
			brushInvalid,
			brushEAddrPhone,
			brushEAddrEmail,
		};
	public:
		PosPaymentDialog2() : PPListDialog(DLG_CPPAYM2, CTL_CPPAYM_CCRDLIST), Data(0, 0.0), State(stEnableBonus), EAddrInputState(0)
		{
			SString font_face;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_IMPACT, font_face);
			SetCtrlFont(CTL_CPPAYM_CSHAMT, font_face, 26);
			SetCtrlFont(CTL_CPPAYM_BNKAMT, font_face, 26);
			Ptb.SetBrush(brushInvalid, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
			Ptb.SetBrush(brushEAddrPhone, SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushEAddrEmail, SPaintObj::bsSolid, GetColorRef(SClrCadetblue),  0);
			// @v11.3.7 {
			if(!DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) {
				showCtrl(CTL_CPPAYM_EADDR, 0);
				showCtrl(CTL_CPPAYM_EADDRINF, 0);
				showCtrl(CTL_CPPAYM_PAPERLESS, 0);
				showCtrl(CTLFRAME_CPPAYM_PAPERLESS, 0);
			}
			// } @v11.3.7
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			TotalConst = Data.CcPl.GetTotal();
			setCtrlReal(CTL_CPPAYM_AMOUNT, TotalConst);
			AddClusterAssoc(CTL_CPPAYM_KIND, 0, cpmCash);
			AddClusterAssoc(CTL_CPPAYM_KIND, 1, cpmBank);
			AddClusterAssoc(CTL_CPPAYM_KIND, 2, cpmIncorpCrd);
			DisableClusterItem(CTL_CPPAYM_KIND, 0, BIN(Data.DisabledKinds & (1 << cpmCash)));
			DisableClusterItem(CTL_CPPAYM_KIND, 1, BIN(Data.DisabledKinds & (1 << cpmBank)));
			DisableClusterItem(CTL_CPPAYM_KIND, 2, BIN(Data.DisabledKinds & (1 << cpmIncorpCrd)));
			// @v12.1.10 {
			if(Data.Kind == cpmUndef) {
				if(Data.Flags & Data.fPreferCashlessPayment && !(Data.DisabledKinds & (1 << cpmBank)))
					Data.Kind = cpmBank;
				else if(!(Data.DisabledKinds & (1 << cpmCash)))
					Data.Kind = cpmCash;
				else {
					; // Надеюсь последующий код как-то разрулит эту ситуацию :)
				}
			}
			// } @v12.1.10 
			SetClusterData(CTL_CPPAYM_KIND, Data.Kind);
			disableCtrl(CTL_CPPAYM_CSHAMT, (Data.DisabledKinds & (1 << cpmCash)));
			disableCtrl(CTL_CPPAYM_BNKAMT, (Data.DisabledKinds & (1 << cpmBank)));
			disableCtrl(CTL_CPPAYM_CRDCARDAMT, (Data.DisabledKinds & (1 << cpmIncorpCrd)));
			SetupKind(cpmUndef);
			setCtrlReal(CTL_CPPAYM_CSHAMT, Data.CcPl.Get(CCAMTTYP_CASH));
			setCtrlReal(CTL_CPPAYM_BNKAMT, Data.CcPl.Get(CCAMTTYP_BANK));
			setCtrlReal(CTL_CPPAYM_CRDCARDAMT, 0.0);
			setCtrlReadOnly(CTL_CPPAYM_CRDCARDAMT, 1);
			// @v12.0.6 {
			AddClusterAssoc(CTL_CPPAYM_CASHLESSBPEQ, 0, PosPaymentBlock::fCashlessBypassEq);
			SetClusterData(CTL_CPPAYM_CASHLESSBPEQ, Data.Flags);
			showCtrl(CTL_CPPAYM_CASHLESSBPEQ, (Data.Flags & PosPaymentBlock::fCashlessBypassEqEnabled));
			// } @v12.0.6 
			double bonus = Data.CcPl.GetBonusAmount(&ScObj);
			if(R2(bonus) > 0.0099) {
				SString text_buf;
				PPLoadTextS(PPTXT_BONUSAVAILABLE, text_buf).CatDiv(':', 2).Cat(bonus, SFMT_MONEY);
				SetClusterItemText(CTL_CPPAYM_USEBONUS, 0, text_buf);
				SETFLAG(State, stEnableBonus, !(ScObj.GetConfig().Flags & PPSCardConfig::fDisableBonusByDefault));
				ToggleBonusAvailability(LOGIC(State & stEnableBonus), true/*force*/);
				setCtrlUInt16(CTL_CPPAYM_USEBONUS, BIN(State & stEnableBonus));
			}
			else
				showCtrl(CTL_CPPAYM_USEBONUS, 0);
			/* @v11.3.6 
			showCtrl(CTL_CPPAYM_ALTCASHREG, BIN(Data.AltCashReg >= 0));
			if(Data.AltCashReg >= 0)
				setCtrlUInt16(CTL_CPPAYM_ALTCASHREG, BIN(Data.AltCashReg == 1));
			*/
			// @v11.3.6 {
			showCtrl(CTL_CPPAYM_ALTCASHREG, BIN(Data.Flags & PosPaymentBlock::fAltCashRegEnabled)); 
			if(Data.Flags & PosPaymentBlock::fAltCashRegEnabled)
				setCtrlUInt16(CTL_CPPAYM_ALTCASHREG, BIN(Data.Flags & PosPaymentBlock::fAltCashRegUse));
			if(DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) { // @v11.3.7
				//Data.BuyersEAddr.SetIfEmpty(DS.GetConstTLA().PaperlessCheque_FakeEAddr);
				if(Data.EAddr.IsEmpty())
					Data.EAddr.SetEMail(DS.GetConstTLA().PaperlessCheque_FakeEAddr);
				setCtrlString(CTL_CPPAYM_EADDR, Data.EAddr.EAddr);
				setCtrlUInt16(CTL_CPPAYM_PAPERLESS, BIN(Data.Flags & PosPaymentBlock::fPaperless));
			}
			// } @v11.3.6
			updateList(-1);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			double val = R2(getCtrlReal(CTL_CPPAYM_CSHAMT));
			Data.CcPl.Set(CCAMTTYP_CASH, val);
			val = R2(getCtrlReal(CTL_CPPAYM_BNKAMT));
			Data.CcPl.Set(CCAMTTYP_BANK, val);
			Data.CcPl.Normalize();
			/* @v11.3.6 
			if(Data.AltCashReg >= 0) {
				uint16 v = getCtrlUInt16(CTL_CPPAYM_ALTCASHREG);
				Data.AltCashReg = BIN(v == 1);
			}*/
			// @v11.3.6 {
			uint16 v = (Data.Flags & PosPaymentBlock::fAltCashRegEnabled) ? getCtrlUInt16(CTL_CPPAYM_ALTCASHREG) : 0;
			SETFLAG(Data.Flags, PosPaymentBlock::fAltCashRegUse, v == 1);
			Data.SetBuyersEAddr(0, 0);
			if(DS.CheckExtFlag(ECF_PAPERLESSCHEQUE)) { // @v11.3.7
				SString eaddr_buf;
				getCtrlString(CTL_CPPAYM_EADDR, eaddr_buf);
				const int eaddr_status = GetEAddrStatus(eaddr_buf);
				if(oneof2(eaddr_status, SNTOK_EMAIL, SNTOK_PHONE)) {
					if(eaddr_status == SNTOK_PHONE) {
						SString normal_phone;
						eaddr_buf = PPEAddr::Phone::NormalizeStr(eaddr_buf, 0, normal_phone);
					}
					Data.SetBuyersEAddr(eaddr_status, eaddr_buf);
					v = getCtrlUInt16(CTL_CPPAYM_PAPERLESS);
					SETFLAG(Data.Flags, PosPaymentBlock::fPaperless, v);
				}
				else {
					Data.Flags &= ~PosPaymentBlock::fPaperless;
				}
			}
			else { // @v11.3.7
				Data.Flags &= ~PosPaymentBlock::fPaperless;
			}
			// } @v11.3.6 
			// @v12.0.6 {
			if(Data.Flags & PosPaymentBlock::fCashlessBypassEqEnabled)
				GetClusterData(CTL_CPPAYM_CASHLESSBPEQ, &Data.Flags);
			else
				SETFLAG(Data.Flags, PosPaymentBlock::fCashlessBypassEq, 0);
			// } @v12.0.6 
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{  
			//static bool isOK = false; // Проверка на повторное нажатие кнопки Enter, в случае ошибочного сканирования в поле ввода
			TInputLine * p_il = static_cast<TInputLine*>(getCtrlView(CTL_CPPAYM_CASH)); 
			if(p_il) {
				TInputLine::Statistics stat;
				p_il->GetStatistics(&stat);
				if(event.isCmd(cmOK)/*&& isOK*/) {
					if(stat.SymbCount && stat.IntervalMean < /*3.0*/20.0 && stat.Flags & stat.fSerialized) {
						p_il->setText(0);
						clearEvent(event);
						//isOK = false; 
					}
				}
			}	// @SevaSob @v11.4.02
			if(!event.isCmd(cmOK) || SetupCrdCard(0) <= 0) {
				if(event.isCmd(cmLBDblClk)) {
					if(event.isCtlEvent(CTL_CPPAYM_CCRDLIST)) {
						long   _id = 0;
						P_Box->getCurID(&_id);
						if(_id) {
							SCardTbl::Rec sc_rec;
							for(uint i = 0; i < Data.CcPl.getCount(); i++) {
								const CcAmountEntry & r_entry = Data.CcPl.at(i);
								if(r_entry.AddedID == _id && ScObj.Fetch(_id, &sc_rec) > 0) {
									setCtrlData(CTL_CPPAYM_CRDCARD, sc_rec.Code);
									double nv = Data.CcPl.Replace(CCAMTTYP_CRDCARD, 0.0, _id, CCAMTTYP_CASH, CCAMTTYP_BANK);
									SetupAmount(0);
									{
										setCtrlReadOnly(CTL_CPPAYM_CRDCARDAMT, 0);
										setCtrlReal(CTL_CPPAYM_CRDCARDAMT, r_entry.Amount);
										selectCtrl(CTL_CPPAYM_CRDCARDAMT);
									}
									updateList(-1);
									break;
								}
							}
						}
					}
				}
				else {
					PPListDialog::handleEvent(event);
					if(event.isClusterClk(CTL_CPPAYM_KIND)) {
						SetupKind(cpmUndef);
					}
					else if(event.isClusterClk(CTL_CPPAYM_USEBONUS)) {
						ToggleBonusAvailability(LOGIC(getCtrlUInt16(CTL_CPPAYM_USEBONUS)), false/*force*/);
					}
					// @v11.3.12 {
					else if(event.isKeyDown(GetSwitchKey(0))) {
						const CheckPaymMethod k = static_cast<CheckPaymMethod>(GetClusterData(CTL_CPPAYM_KIND));
						if(k == cpmCash)
							SetupKind(cpmBank);
						else if(k == cpmBank)
							SetupKind(cpmIncorpCrd);
						else if(k == cpmIncorpCrd)
							SetupKind(cpmCash);
					}
					// } @v11.3.12 
					else if(TVCMD == cmCtlColor) {
						TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
						if(p_dc && getCtrlHandle(CTL_CPPAYM_EADDR) == p_dc->H_Ctl) {
							int brush_ident = 0;
							if(EAddrInputState == SNTOK_PHONE)
								brush_ident = brushEAddrPhone;
							else if(EAddrInputState == SNTOK_EMAIL)
								brush_ident = brushEAddrEmail;
							else if(EAddrInputState < 0)
								brush_ident = brushInvalid;
							if(brush_ident) {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brush_ident));
							}
						}
						else
							return;
					}
					else if(event.isCmd(cmInputUpdated)) {
						if(!(State & stLock)) {
							if(event.isCtlEvent(CTL_CPPAYM_BNKAMT)) {
								double val = R2(getCtrlReal(CTL_CPPAYM_BNKAMT));
								double nv = Data.CcPl.Replace(CCAMTTYP_BANK, val, 0, CCAMTTYP_CASH, 0);
								if(nv != val) {
									State |= stLock;
									setCtrlReal(CTL_CPPAYM_BNKAMT, nv);
									State &= ~stLock;
								}
								Data.DeliveryAmt = R2(Data.NoteAmt - Data.CcPl.Get(CCAMTTYP_CASH));
								SetupAmount(CTL_CPPAYM_BNKAMT);
							}
							else if(event.isCtlEvent(CTL_CPPAYM_CSHAMT)) {
								const double val = R2(getCtrlReal(CTL_CPPAYM_CSHAMT));
								const double nv = Data.CcPl.Replace(CCAMTTYP_CASH, val, 0, CCAMTTYP_BANK, 0);
								if(nv != val) {
									State |= stLock;
									setCtrlReal(CTL_CPPAYM_CSHAMT, nv);
									State &= ~stLock;
								}
								Data.DeliveryAmt = R2(Data.NoteAmt - Data.CcPl.Get(CCAMTTYP_CASH));
								SetupAmount(CTL_CPPAYM_CSHAMT);
							}
							else if(event.isCtlEvent(CTL_CPPAYM_CRDCARDAMT)) {
								/*
								Lock__ = 1;
								double val = R2(getCtrlReal(CTL_CPPAYM_CRDCARDAMT));
								Data.CcPl.Set(CCAMTTYP_CRDCARD, val);
								if(val != 0.0)
									Data.CcPl.Set(CCAMTTYP_BANK, 0.0);
								val = R2(TotalConst - Data.CcPl.Get(CCAMTTYP_CRDCARD) - Data.CcPl.Get(CCAMTTYP_BANK));
								Data.CcPl.Set(CCAMTTYP_CASH, val);
								Data.DeliveryAmt = R2(Data.NoteAmt - val);
								SetupAmount(CTL_CPPAYM_CRDCARDAMT);
								Lock__ = 0;
								*/
							}
							else if(event.isCtlEvent(CTL_CPPAYM_CASH)) {
								//isOK = true; // @SevaSob @v11.4.02
								double val = R2(getCtrlReal(CTL_CPPAYM_CASH));
								Data.NoteAmt = val;
								Data.DeliveryAmt = R2(val - Data.CcPl.Get(CCAMTTYP_CASH));
								SetupAmount(CTL_CPPAYM_CASH);
							}
							else if(event.isCtlEvent(CTL_CPPAYM_CRDCARD)) {
								State |= stLock;
								SetupCrdCard(1);
								State &= ~stLock;
							}
							else if(event.isCtlEvent(CTL_CPPAYM_EADDR)) { // @v11.3.6
								SString eaddr_buf;
								getCtrlString(CTL_CPPAYM_EADDR, eaddr_buf);
								EAddrInputState = GetEAddrStatus(eaddr_buf);
								drawCtrl(CTL_CPPAYM_EADDR);
							}
						}
					}
					else
						return;
				}
				clearEvent(event);
			}
		}
		virtual int editItem(long pos, long id)
		{
            int    ok = -1;
			if(id) {
				for(uint i = 0; i < Data.CcPl.getCount(); i++) {
					const CcAmountEntry & r_entry = Data.CcPl.at(i);
					if(r_entry.AddedID == id) {
						CcAmountEntry temp_entry = r_entry;
                        if(EditCcAmount(&temp_entry) > 0) {
							Data.CcPl.Replace(CCAMTTYP_CRDCARD, temp_entry.Amount, id, CCAMTTYP_CASH, CCAMTTYP_BANK);
							SetupAmount(0);
							ok = 1;
                        }
						break;
					}
				}
			}
            return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			if(id) {
				for(uint i = 0; i < Data.CcPl.getCount(); i++) {
					const CcAmountEntry & r_entry = Data.CcPl.at(i);
					if(r_entry.AddedID == id && CONFIRM(PPCFM_DELCCPAYMENTRY)) {
						Data.CcPl.Replace(CCAMTTYP_CRDCARD, 0.0, id, CCAMTTYP_CASH, CCAMTTYP_BANK);
						SetupAmount(0);
						ok = 1;
						break;
					}
				}
			}
			return ok;
		}
		virtual int setupList()
		{
			int    ok = 1;
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < Data.CcPl.getCount(); i++) {
				const CcAmountEntry & r_entry = Data.CcPl.at(i);
				if(r_entry.Type == CCAMTTYP_CRDCARD) {
					ss.Z();
					temp_buf.Z();
					if(r_entry.AddedID) {
						SCardTbl::Rec sc_rec;
						if(ScObj.Search(r_entry.AddedID, &sc_rec) > 0) {
							PPSCardSeries scs_rec;
							const int scst = (ScsObj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
							if(oneof2(scst, scstCredit, scstBonus)) {
								if(!ScRestList.Has(r_entry.AddedID)) {
									//
									// Функция GetCrdCardRest занесет остаток по карте в кэш
									//
									double sc_rest = GetCrdCardRest(r_entry.AddedID);
								}
							}
							temp_buf = sc_rec.Code;
						}
						else
							ideqvalstr(r_entry.AddedID, temp_buf);
					}
					else
						ideqvalstr(0, temp_buf);
					ss.add(temp_buf);
					temp_buf.Z().Cat(r_entry.Amount);
					ss.add(temp_buf);
					temp_buf.Z();
					temp_buf.Cat(ScRestList.Get(r_entry.AddedID, 0) - r_entry.Amount, SFMT_MONEY);
					ss.add(temp_buf);
					THROW(addStringToList(r_entry.AddedID, ss.getBuf()));
				}
			}
			CATCHZOK
			return ok;
		}
		void   ToggleBonusAvailability(bool available, bool force)
		{
			if(BIN(State & stEnableBonus) != BIN(available) || force) {
				SETFLAG(State, stEnableBonus, available);
				int    do_update = 0;
				for(uint i = 0; i < Data.CcPl.getCount(); i++) {
					CcAmountEntry & r_entry = Data.CcPl.at(i);
					if(r_entry.Type == CCAMTTYP_CRDCARD && r_entry.AddedID) {
						SCardTbl::Rec sc_rec;
						if(ScObj.Fetch(r_entry.AddedID, &sc_rec) > 0) {
							PPSCardSeries scs_rec;
							const int scst = (ScsObj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
							if(scst == scstBonus) {
								if(State & stEnableBonus) {
									double sc_rest = GetCrdCardRest(sc_rec.ID);
									// @v11.0.4 const double max_bonus = R2((Data.BonusMaxPart < 1.0) ? (TotalConst * Data.BonusMaxPart) : TotalConst); // @v10.9.1 R2
									const double max_bonus = Data.GetUsableBonus(); // @v11.0.4
									double b = Data.CcPl.GetBonusAmount(&ScObj);
									double bonus = MIN(sc_rest, (max_bonus - b));
									double nv = Data.CcPl.Replace(CCAMTTYP_CRDCARD, bonus, r_entry.AddedID, CCAMTTYP_CASH, CCAMTTYP_BANK);
								}
								else {
									if(Data.Kind == cpmBank)
										Data.CcPl.ReplaceDontRemove(CCAMTTYP_CRDCARD, 0.0, r_entry.AddedID, CCAMTTYP_BANK, CCAMTTYP_CASH);
									else
										Data.CcPl.ReplaceDontRemove(CCAMTTYP_CRDCARD, 0.0, r_entry.AddedID, CCAMTTYP_CASH, CCAMTTYP_BANK);
								}
								do_update = 1;
							}
						}
					}
				}
				if(do_update) {
					setCtrlReal(CTL_CPPAYM_CSHAMT, Data.CcPl.Get(CCAMTTYP_CASH));
					setCtrlReal(CTL_CPPAYM_BNKAMT, Data.CcPl.Get(CCAMTTYP_BANK));
					setCtrlReal(CTL_CPPAYM_CRDCARDAMT, 0.0);
					updateList(-1);
				}
			}
		}
		int    EditCcAmount(CcAmountEntry * pData)
		{
			int    ok = -1;
			TDialog * dlg = new TDialog(DLG_EDITCCAMT);
			if(CheckDialogPtrErr(&dlg)) {
				SString info_buf, temp_buf;
                if(pData->Type == CCAMTTYP_CRDCARD && pData->AddedID) {
                	SCardTbl::Rec sc_rec;
					if(ScObj.Fetch(pData->AddedID, &sc_rec) > 0) {
						PPSCardSeries scs_rec;
						const int scst = (ScsObj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
						PPLoadString("card", temp_buf);
						info_buf.Cat(temp_buf).CatDiv(':', 2).Cat(sc_rec.Code);
						if(oneof2(scst, scstCredit, scstBonus)) {
							double sc_rest = GetCrdCardRest(sc_rec.ID);
							PPLoadString("rest", temp_buf);
                            info_buf.CR().Cat(temp_buf).CatDiv(':', 2).Cat(sc_rest, MKSFMTD_020);
                            dlg->setStaticText(CTL_EDITCCAMT_INFO, info_buf);
                            dlg->setCtrlReal(CTL_EDITCCAMT_AMT, pData->Amount);
                            while(ok < 0 && ExecView(dlg) == cmOK) {
                                double new_value = dlg->getCtrlReal(CTL_EDITCCAMT_AMT);
                                if(new_value < 0.0) {
									PPErrorByDialog(dlg, CTL_EDITCCAMT_AMT, PPERR_USERINPUT);
                                }
                                else if(new_value > sc_rest) {
									PPSetError(PPERR_MAXCRD_OVERDRAFT, sc_rec.Code);
									PPErrorByDialog(dlg, CTL_EDITCCAMT_AMT);
                                }
                                else if(new_value > TotalConst) {
									PPSetError(PPERR_OVERPAYMENT, temp_buf.Z().Cat(TotalConst, MKSFMTD_020));
									PPErrorByDialog(dlg, CTL_EDITCCAMT_AMT);
                                }
                                else {
									pData->Amount = new_value;
									ok = 1;
                                }
                            }
						}
					}
                }
			}
			delete dlg;
			return ok;
		}
		double GetCrdCardRest(PPID cardID)
		{
			double rest = 0.0;
			SCardTbl::Rec sc_rec;
			if(ScObj.Fetch(cardID, &sc_rec) > 0) {
				PPSCardSeries scs_rec;
				const int scst = (ScsObj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
				if(oneof2(scst, scstCredit, scstBonus)) {
					if(scs_rec.Flags & SCRDSF_UHTTSYNC) {
						PPObjSCard::UhttEntry uhtt_sc;
						if(ScObj.FetchUhttEntry(sc_rec.Code, &uhtt_sc) > 0)
							rest = uhtt_sc.Rest;
					}
					else
						ScObj.P_Tbl->GetRest(sc_rec.ID, MAXDATE, &rest);
					if(rest < 0.0)
						rest = 0.0;
				}
				ScRestList.Remove(sc_rec.ID, 0);
				ScRestList.Add(sc_rec.ID, rest);
			}
			return rest;
		}
		//
		// ARG(onScCodeInput IN): Если !0 то функция вызывается во время ввода номера карты - не очищать поле при неверном коде
		//
		int    SetupCrdCard(int onScCodeInput) 
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlString(CTL_CPPAYM_CRDCARD, temp_buf);
			SCardTbl::Rec sc_rec;
			if(temp_buf.NotEmptyS()) {
				if(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0) {
					if(Data.CcPl.SearchAddedID(sc_rec.ID, 0)) {
					}
					else {
						PPSCardSeries scs_rec;
						const int scst = (ScsObj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
						if(oneof2(scst, scstCredit, scstBonus)) {
							double sc_rest = GetCrdCardRest(sc_rec.ID);
							if(sc_rest > 0.0 || TotalConst < 0.0) {
								double to_replace_amt = 0.0;
								double v = 0.0;
								const double input_amount = getCtrlReal(CTL_CPPAYM_CRDCARDAMT);
								if(scst == scstBonus) {
									if(State & stEnableBonus && Data.BonusMaxPart > 0.0) {
										const double max_bonus = (Data.BonusMaxPart < 1.0) ? (TotalConst * Data.BonusMaxPart) : TotalConst;
										double b = Data.CcPl.GetBonusAmount(&ScObj);
										to_replace_amt = MIN(sc_rest, (max_bonus - b));
									}
								}
								else {
									to_replace_amt = (TotalConst >= 0.0) ? sc_rest : (Data.CcPl.Get(CCAMTTYP_CASH) + Data.CcPl.Get(CCAMTTYP_BANK));
								}
								if(input_amount > 0.0 && input_amount <= to_replace_amt)
									to_replace_amt = input_amount;
								if(to_replace_amt > 0.0) {
									if(onScCodeInput) {
										setCtrlReadOnly(CTL_CPPAYM_CRDCARDAMT, 0);
										setCtrlReal(CTL_CPPAYM_CRDCARDAMT, to_replace_amt);
										selectCtrl(CTL_CPPAYM_CRDCARDAMT);
									}
									else {
										v = Data.CcPl.Replace(CCAMTTYP_CRDCARD, to_replace_amt, sc_rec.ID, CCAMTTYP_CASH, CCAMTTYP_BANK);
									}
								}
								if(v != 0.0) {
									assert(sc_rest >= v);
									SetupAmount(/*CTL_CPPAYM_CRDCARDAMT*/0);
									updateList(-1);
									ok = 1;
								}
							}
						}
					}
				}
				if(!onScCodeInput) {
					setCtrlString(CTL_CPPAYM_CRDCARD, temp_buf.Z());
					setCtrlReadOnly(CTL_CPPAYM_CRDCARDAMT, 1);
					setCtrlReal(CTL_CPPAYM_CRDCARDAMT, 0.0);
				}
			}
			else
				ok = -1;
			return ok;
		}
		int GetSwitchKey(SString * pText) const
		{
			ASSIGN_PTR(pText, "[F12]");
			return kbF12;
		}
		void   SetPaymKindSwitchKeyText(TCluster * pClu, uint itemIdx)
		{
			if(pClu) {
				SString temp_buf;
				SString switch_key_text;
				GetSwitchKey(&switch_key_text);
				const uint cc = pClu->getNumItems();
				for(uint i = 0; i < cc; i++) {
					pClu->GetText(i, temp_buf);
					size_t sktpos = 0;
					if(temp_buf.Search(switch_key_text, 0, 0, &sktpos)) {
						if(i != itemIdx)
							pClu->SetText(i, temp_buf.Trim(sktpos).Strip());
					}
					else if(i == itemIdx)
						pClu->SetText(i, temp_buf.Space().Cat(switch_key_text));
				}
			}
		}
		void   SetupKind(CheckPaymMethod outerKind)
		{
			TCluster * p_clu = (TCluster *)getCtrlView(CTL_CPPAYM_KIND);
			if(p_clu) {
				SString temp_buf;
				if(outerKind == cpmUndef) {
					Data.Kind = static_cast<CheckPaymMethod>(GetClusterData(CTL_CPPAYM_KIND));
				}
				else {
					SetClusterData(CTL_CPPAYM_KIND, outerKind);
					Data.Kind = outerKind;
				}
				switch(Data.Kind) {
					case cpmCash:
						{
							const double val = TotalConst - Data.CcPl.Get(CCAMTTYP_CRDCARD);
							Data.CcPl.Set(CCAMTTYP_CASH, val);
							Data.CcPl.Set(CCAMTTYP_BANK, 0.0);
							SetupAmount();
							State |= stLock;
							selectCtrl(CTL_CPPAYM_CASH);
							State &= ~stLock;
							SetPaymKindSwitchKeyText(p_clu, 1);
						}
						break;
					case cpmBank:
						{
							const double val = TotalConst - Data.CcPl.Get(CCAMTTYP_CRDCARD);
							Data.CcPl.Set(CCAMTTYP_CASH, 0.0);
							Data.CcPl.Set(CCAMTTYP_BANK, val);
							SetupAmount();
							SetPaymKindSwitchKeyText(p_clu, 2);
						}
						break;
					case cpmIncorpCrd:
						getCtrlString(CTL_CPPAYM_CRDCARDAMT, temp_buf);
						if(!temp_buf.NotEmptyS())
							selectCtrl(CTL_CPPAYM_CRDCARDAMT);
						SetupCrdCard(0);
						SetPaymKindSwitchKeyText(p_clu, 0);
						break;
				}
			}
		}
		void   SetupAmount(uint lockCtl = 0)
		{
			if(!(State & stLock)) {
				State |= stLock;
				if(lockCtl != CTL_CPPAYM_CSHAMT)
					setCtrlReal(CTL_CPPAYM_CSHAMT, Data.CcPl.Get(CCAMTTYP_CASH));
				if(lockCtl != CTL_CPPAYM_BNKAMT)
					setCtrlReal(CTL_CPPAYM_BNKAMT, Data.CcPl.Get(CCAMTTYP_BANK));
				if(lockCtl != CTL_CPPAYM_CRDCARDAMT)
					setCtrlReal(CTL_CPPAYM_CRDCARDAMT, Data.CcPl.Get(CCAMTTYP_CRDCARD));
				{
					SString temp_buf;
					if(Data.SCardID)
						temp_buf.Z().Cat(R2(Data.SCardRest - Data.ScAmt), MKSFMTD(0, 2, NMBF_NOZERO));
					setStaticText(CTL_CPPAYM_CRDCARDREST, temp_buf);
				}
				if(lockCtl != CTL_CPPAYM_CASH)
					setCtrlReal(CTL_CPPAYM_CASH, Data.NoteAmt);
				setCtrlReal(CTL_CPPAYM_DIFF, (Data.DeliveryAmt > 0.0) ? Data.DeliveryAmt : 0.0); // @v10.9.0 (Data.DeliveryAmt)-->((Data.DeliveryAmt > 0.0) ? Data.DeliveryAmt : 0.0)
				State &= ~stLock;
			}
		}
		int    GetEAddrStatus(SString & rBuf)
		{
			int    status = 0;
			if(rBuf.NotEmptyS()) {
				SNaturalTokenArray nta;
				Trgn.Run(rBuf.ucptr(), rBuf.Len(), nta, 0);
				if(nta.Has(SNTOK_PHONE))
					status = SNTOK_PHONE;
				else if(nta.Has(SNTOK_EMAIL))
					status = SNTOK_EMAIL;
				else
					status = -1;
			}
			else
				status = 0;
			return status;
		}
		PPObjSCard ScObj;
		PPObjSCardSeries ScsObj;
		RAssocArray ScRestList;
		//int    Lock__;
		//int    EnableBonus;
		enum {
			stLock        = 0x0001,
			stEnableBonus = 0x0002
		};
		uint   State;
		int    EAddrInputState; // 0 - empty, -1 - invalid, SNTOK_PHONE, SNTOK_EMAIL
		double TotalConst;
		PPTokenRecognizer Trgn;
		SPaintToolBox Ptb;
	};

	DIALOG_PROC_BODY(PosPaymentDialog2, this);
}
//
//
//
