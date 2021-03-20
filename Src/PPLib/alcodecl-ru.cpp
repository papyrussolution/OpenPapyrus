// ALCODECL-RU.CPP
// Copyright (c) A.Sobolev 2021
// @codepage UTF-8
// Алкогольная декларация (Россия)
//
#include <pp.h>
#pragma hdrstop

class AlcoDeclRuFilt : public PPBaseFilt {
public:
	AlcoDeclRuFilt();

	char   ReserveStart[128]; // @anchor 
	DateRange Period;         // 
	long   Flags;
	long   Reserve;           // @anchor Заглушка для отмера "плоского" участка фильтра	
	ObjIdListFilt LocList;
	SString AlcoCodeList;     // Список символов видов алкогольной продукции, которыми следует ограничить отчет
};

struct AlcoDeclRuViewItem {
	struct SupplEntry {
		LDATE  Dt;
		char   InvcCode[32];
		char   CLB[32];      // ГТД
		double Qtty;         // ДАЛ
	};
	struct MovEntry {
		double StockBeg;  // П100000000006 Остаток на начало отчетного периода (дал)
		double StockEnd;  // П100000000020 Остаток на конец отчетного периода (дал)
		double RcptManuf; // П100000000007 Поступление - закупки от организаций- производителей (дал)
		double RcptWhs;   // П100000000008 Поступление - закупки от организаций оптовой торговли (дал)
		double RcptImp;   // П100000000009 Поступление - закупки по импорту (дал)
		// П100000000010 Поступление - закупки итого (дал)
		double SaleRet;   // П100000000011 Поступление - возврат от покупателей (дал)
		double RcptEtc;   // П100000000012 Прочие поступления (дал)
		double RcptIntr;  // П100000000013 Поступление - перемещение внутри одной организации (дал)
		// П100000000014 Поступление - всего (дал)
		double ExpRetail; // П100000000015 Расход - объем розничной продажи (дал)
		double ExpEtc;    // П100000000016 Прочий расход (дал)
		double SupplRet;  // П100000000017 Возврат поставщику (дал)
		double ExpIntr;   // П100000000018 Расход - перемещение внутри одной организации (дал)
		// П100000000019 Расход всего (дал)
	};
	char   AlcoCode[16];
	PPID   ManufID; // Импортер/производитель
	int    ItemKind;
	SupplEntry SE;
	MovEntry   ME;
};

class PPViewAlcoDeclRu : public PPView {
public:
	enum {
		kUndef = 0,
		kReceipt,
		kReceiptRet,
		kMov
	};
	PPViewAlcoDeclRu();
	~PPViewAlcoDeclRu();
	virtual int Init_(const PPBaseFilt * pBaseFilt);
	virtual int EditBaseFilt(PPBaseFilt * pBaseFilt);
	int    InitIteration();
	int    FASTCALL NextIteration(AlcoDeclRuViewItem *);
private:
	AlcoDeclRuFilt Filt;
	TempAlcoDeclRu_RcptTbl * P_TempR_T;
	TempAlcoDeclRu_MovTbl * P_TempM_T;
};

IMPLEMENT_PPFILT_FACTORY(AlcoDeclRu); AlcoDeclRuFilt::AlcoDeclRuFilt() : PPBaseFilt(PPFILT_ALCODECLRU, 0, 0)
{
	SetFlatChunk(offsetof(AlcoDeclRuFilt, ReserveStart),
		offsetof(AlcoDeclRuFilt, Reserve)-offsetof(AlcoDeclRuFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(AlcoDeclRuFilt, LocList));
	SetBranchSString(offsetof(AlcoDeclRuFilt, AlcoCodeList));
	Init(1, 0);
}

PPViewAlcoDeclRu::PPViewAlcoDeclRu() : PPView(0, &Filt, PPVIEW_ALCODECLRU, 0, 0), P_TempR_T(0), P_TempM_T(0)
{
}
	
PPViewAlcoDeclRu::~PPViewAlcoDeclRu()
{
	delete P_TempR_T;
	delete P_TempM_T;
}

/*virtual*/int PPViewAlcoDeclRu::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	return ok;
}

/*virtual*/int PPViewAlcoDeclRu::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int PPViewAlcoDeclRu::InitIteration()
{
	int    ok = -1;
	return ok;
}

int FASTCALL PPViewAlcoDeclRu::NextIteration(AlcoDeclRuViewItem * pItem)
{
	int    ok = -1;
	return ok;
}