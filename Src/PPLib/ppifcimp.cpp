// PPIFCIMP.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage UTF-8
//
// Реализация интерфейсов
//
#include <pp.h>
#pragma hdrstop
#include <process.h>
// @v9.6.2 (moved to pp.h) #include <ppidata.h>

static int SLAPI IfcImpCheckDictionary()
{
	return CurDict ? 1 : PPSetError(PPERR_NOTLOGGEDIN);
}

static void FASTCALL ReleaseUnknObj(IUnknown ** ppUnkn)
{
	if(*ppUnkn) {
		(*ppUnkn)->Release();
		(*ppUnkn) = 0;
	}
}

SDateRange FASTCALL DateRangeToOleDateRange(DateRange period)
{
	SDateRange rng;
	rng.Low = (OleDate)period.low;
	rng.Upp = (OleDate)period.upp;
	return rng;
}

DateRange FASTCALL OleDateRangeToDateRange(const SDateRange & rRng)
{
	OleDate _low; _low.v = rRng.Low;
	OleDate _upp; _upp.v = rRng.Upp;
	DateRange period;
	period.low = _low;
	period.upp = _upp;
	return period;
}

SIterCounter GetPPViewIterCounter(const void * ppviewPtr, int * pAppError)
{
	const PPView * p_v = (const PPView *)ppviewPtr;
	SIterCounter cntr;
	if(p_v) {
		const IterCounter & inner_cntr = p_v->GetCounter();
		cntr.Count = inner_cntr;
		cntr.Total = inner_cntr.GetTotal();
	}
	else {
		cntr.Count = cntr.Total = 0;
		ASSIGN_PTR(pAppError, 1);
	}
	return cntr;
}

IUnknown * GetPPObjIStrAssocList(SCoClass * pCls, PPObject * pObj, long extraParam)
{
	IUnknown * p = 0;
	if(pObj) {
		if(pCls->CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p)) {
			StrAssocArray * p_list_env = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p);
			if(p_list_env) {
				StrAssocArray * p_list = pObj->MakeStrAssocList((void *)extraParam);
				if(p_list) {
					*p_list_env = *p_list;
					p_list_env->setPointer(0);
					ZDELETE(p_list);
				}
				else
					ReleaseUnknObj(&p);
			}
			else
				ReleaseUnknObj(&p);
		}
	}
	return p;
}

IUnknown * GetIStrAssocList(SCoClass * pCls, StrAssocArray * pList, int delList = 1)
{
	IUnknown * p = 0;
	if(pList && pCls && pCls->CreateInnerInstance("StrAssocList", "IStrAssocList", (void**)&p)) {
		StrAssocArray * p_list_env = (StrAssocArray*)SCoClass::GetExtraPtrByInterface(p);
		if(p_list_env) {
			*p_list_env = *pList;
			p_list_env->setPointer(0);
		}
		else
			ReleaseUnknObj(&p);
	}
	if(delList)
		ZDELETE(pList);
	return p;
}

IUnknown * GetIPapyrusAmountList(SCoClass * pCls, AmtList * pList, int delList = 1)
{
	IUnknown * p = 0;
	if(pList && pCls && pCls->CreateInnerInstance("PPAmountList", "IPapyrusAmountList", (void**)&p)) {
		AmtList * p_list_env = (AmtList*)SCoClass::GetExtraPtrByInterface(p);
		if(p_list_env)
			*p_list_env = *pList;
		else
			ReleaseUnknObj(&p);
	}
	if(delList)
		ZDELETE(pList);
	return p;
}

IUnknown * GetILotList(SCoClass * pCls, SArray * pList, int delList = 1)
{
	IUnknown * p = 0;
	if(pList && pCls && pCls->CreateInnerInstance("PPLotList", "ILotList", (void**)&p)) {
		SArray * p_list_env = (SArray*)SCoClass::GetExtraPtrByInterface(p);
		if(p_list_env) {
			*p_list_env = *pList;
			p_list_env->setPointer(0);
		}
		else
			ReleaseUnknObj(&p);
	}
	if(delList)
		ZDELETE(pList);
	return p;
}

int Use001()
{
	return 1;
}

#define USE_IMPL_DL6ICLS_PapyrusTextAnalyzer
#define USE_IMPL_DL6ICLS_LongList
#define USE_IMPL_DL6ICLS_StrAssocList
#define USE_IMPL_DL6ICLS_CompleteList
#define USE_IMPL_DL6ICLS_PPTaggedStringList
#define USE_IMPL_DL6ICLS_PPRetailPriceExtractor
#define USE_IMPL_DL6ICLS_PPSFile
#define USE_IMPL_DL6ICLS_PPDbfCreateFlds
#define USE_IMPL_DL6ICLS_PPDbfRecord
#define USE_IMPL_DL6ICLS_PPDbfTable
#define USE_IMPL_DL6ICLS_PPFtp
#define USE_IMPL_DL6ICLS_PPRtlPriceExtractor
#define USE_IMPL_DL6ICLS_PPUtil
#define USE_IMPL_DL6ICLS_PPSession
#define USE_IMPL_DL6ICLS_PPDL200Resolver
#define USE_IMPL_DL6ICLS_PPAmountList
#define USE_IMPL_DL6ICLS_PPObjUnit
#define USE_IMPL_DL6ICLS_PPObjOprKind
#define USE_IMPL_DL6ICLS_PPObjAccSheet
#define USE_IMPL_DL6ICLS_PPObjCashNode
#define USE_IMPL_DL6ICLS_PPObjQuotKind
#define USE_IMPL_DL6ICLS_PPObjGoodsTax
#define USE_IMPL_DL6ICLS_PPObjArticle
#define USE_IMPL_DL6ICLS_PPObjStyloPalm
#define USE_IMPL_DL6ICLS_PPObjCurrency
#define USE_IMPL_DL6ICLS_PPObjGoodsClass
#define USE_IMPL_DL6ICLS_PPObjGoods
#define USE_IMPL_DL6ICLS_PPObjGoodsGroup
#define USE_IMPL_DL6ICLS_PPLocAddrStruc
#define USE_IMPL_DL6ICLS_PPObjLocation
#define USE_IMPL_DL6ICLS_PPObjPerson
#define USE_IMPL_DL6ICLS_PPBillPacket
#define USE_IMPL_DL6ICLS_PPObjBill
#define USE_IMPL_DL6ICLS_PPObjWorld
#define USE_IMPL_DL6ICLS_PPObjRegister
#define USE_IMPL_DL6ICLS_PPPersonRelTypePacket
#define USE_IMPL_DL6ICLS_PPObjPersonRelType
#define USE_IMPL_DL6ICLS_PPFiltCCheck
#define USE_IMPL_DL6ICLS_PPViewCCheck
#define USE_IMPL_DL6ICLS_PPFiltTrfrAnlz
#define USE_IMPL_DL6ICLS_PPViewTrfrAnlz
#define USE_IMPL_DL6ICLS_PPFiltGoods
#define USE_IMPL_DL6ICLS_PPViewGoods
#define USE_IMPL_DL6ICLS_PPFiltGoodsRest
#define USE_IMPL_DL6ICLS_PPViewGoodsRest
#define USE_IMPL_DL6ICLS_PPFiltBill
#define USE_IMPL_DL6ICLS_PPViewBill
#define USE_IMPL_DL6ICLS_PPQuotation
#define USE_IMPL_DL6ICLS_PPFiltGoodsOpAnlz
#define USE_IMPL_DL6ICLS_PPViewGoodsOpAnlz
#define USE_IMPL_DL6ICLS_PPObjTransport
#define USE_IMPL_DL6ICLS_PPObjProcessor
#define USE_IMPL_DL6ICLS_PPObjTSession
#define USE_IMPL_DL6ICLS_PPObjPrjTask
#define USE_IMPL_DL6ICLS_PPFiltPrjTask
#define USE_IMPL_DL6ICLS_PPViewPrjTask
#define USE_IMPL_DL6ICLS_PPObjProject
#define USE_IMPL_DL6ICLS_PPFiltProject
#define USE_IMPL_DL6ICLS_PPViewProject
#define USE_IMPL_DL6ICLS_PPObjBrand
#define USE_IMPL_DL6ICLS_PPObjQCert
#define USE_IMPL_DL6ICLS_PPFiltOpGrouping
#define USE_IMPL_DL6ICLS_PPViewOpGrouping
#define USE_IMPL_DL6ICLS_PPFiltDebtTrnovr
#define USE_IMPL_DL6ICLS_PPViewDebtTrnovr
#define USE_IMPL_DL6ICLS_PPFiltLotOp
#define USE_IMPL_DL6ICLS_PPViewLotOp
#define USE_IMPL_DL6ICLS_PPObjDebtDim
#define USE_IMPL_DL6ICLS_PPLotList
#define USE_IMPL_DL6ICLS_AlcRepOpList
#define USE_IMPL_DL6ICLS_PrcssrAlcReport
#define USE_IMPL_DL6ICLS_PPSysJournal
#define USE_IMPL_DL6ICLS_PPObjTag
#define USE_IMPL_DL6ICLS_PPFias
#define USE_IMPL_DL6ICLS_PPCCheckPacket
#define USE_IMPL_DL6ICLS_PPObjCCheck
#define USE_IMPL_DL6ICLS_PPObjSCardSeries
#define USE_IMPL_DL6ICLS_PPObjSCard

#include "..\rsrc\dl600\ppifc_auto.cpp"
//
//
//
/*
struct __ITextAnalyzerBlock {
	__ITextAnalyzerBlock() : R(), Fb(R)
	{
		Inited = 0;
	}
	PPTextAnalyzer A;
	PPTextAnalyzer::Replacer R;
	PPTextAnalyzer::FindBlock Fb;
	int    Inited;
};
*/

DL6_IC_CONSTRUCTOR(PapyrusTextAnalyzer, DL6ICLS_PapyrusTextAnalyzer_VTab)
{
	SLS.SetCodepage(cp1251);
	//ExtraPtr = new __ITextAnalyzerBlock;
	ExtraPtr = new PPTextAnalyzerWrapper;
}

DL6_IC_DESTRUCTOR(PapyrusTextAnalyzer)
{
	//delete (__ITextAnalyzerBlock *)ExtraPtr;
	delete (PPTextAnalyzerWrapper *)ExtraPtr;
}
//
// Interface IPapyrusTextAnalyzer implementation
//
int32 DL6ICLS_PapyrusTextAnalyzer::Init(SString & ruleFileName)
{
	int    ok = 1;
	//__ITextAnalyzerBlock * p_obj = (__ITextAnalyzerBlock *)ExtraPtr;
	PPTextAnalyzerWrapper * p_obj = (PPTextAnalyzerWrapper *)ExtraPtr;
	if(p_obj) {
		if(!p_obj->Init(ruleFileName, PPTextAnalyzerWrapper::fEncInner))
			ok = RaiseAppError();
		/*
		if(p_obj->A.ParseReplacerFile(ruleFileName, p_obj->R)) {
			p_obj->Inited = 1;
		}
		else
			ok = RaiseAppError();
		*/
	}
	return ok;
}

SString & DL6ICLS_PapyrusTextAnalyzer::ReplaceString(SString & inputText)
{
	//__ITextAnalyzerBlock * p_obj = (__ITextAnalyzerBlock *)ExtraPtr;
	PPTextAnalyzerWrapper * p_obj = (PPTextAnalyzerWrapper *)ExtraPtr;
	if(p_obj) {
		if(!p_obj->ReplaceString(inputText, RetStrBuf))
			AppError = 1;
		/*
		RetStrBuf = 0;
		if(p_obj->Inited) {
			inputText.Strip().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
			p_obj->A.Reset(0);
			p_obj->A.ProcessString(p_obj->R, 0, inputText, RetStrBuf, &p_obj->Fb, 0);
			if(RetStrBuf.Empty())
				RetStrBuf = inputText;
			RetStrBuf.Transf(CTRANSF_OUTER_TO_INNER);
		}
		else {
			AppError = 1;
		}
		*/
	}
	return RetStrBuf;
}
//
//
//
DL6_IC_CONSTRUCTOR(LongList, DL6ICLS_LongList_VTab)
{
	ExtraPtr = new LongArray;
}

DL6_IC_DESTRUCTOR(LongList)
{
	delete (LongArray *)ExtraPtr;
}
//
// Interface ILongList implementation
//
int32 DL6ICLS_LongList::GetCount()
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return p_data ? (int32)p_data->getCount() : RaiseAppError();
}

int32 DL6ICLS_LongList::Get(int32 idx)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return (p_data && idx < (int32)p_data->getCount()) ? (int32)p_data->get(idx) : RaiseAppError();
}

int32 DL6ICLS_LongList::Search(int32 val)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	uint   idx = 0;
	return (p_data && p_data->lsearch(val, &idx)) ? (idx+1) : 0;
}

int32 DL6ICLS_LongList::BSearch(int32 val)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	uint   idx = 0;
	return (p_data && p_data->bsearch(val, &idx)) ? (idx+1) : 0;
}

int32 DL6ICLS_LongList::Add(int32 val)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return p_data ? (int32)p_data->add(val) : RaiseAppError();
}

int32 DL6ICLS_LongList::AddUnique(int32 val)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return p_data ? (int32)p_data->addUnique(val) : RaiseAppError();
}

int32 DL6ICLS_LongList::Remove(int32 idx)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return (p_data && idx < (int32)p_data->getCount()) ? (int32)p_data->atFree(idx) : RaiseAppError();
}

int32 DL6ICLS_LongList::RemoveByVal(int32 val)
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	return p_data ? (int32)p_data->freeByKey(val, 0) : RaiseAppError();
}

void DL6ICLS_LongList::Sort()
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	if(p_data)
		p_data->sort();
	else
		AppError = 1;
}

void DL6ICLS_LongList::SortAndUndup()
{
	LongArray * p_data = (LongArray *)ExtraPtr;
	if(p_data)
		p_data->sortAndUndup();
	else
		AppError = 1;
}
//
// StrAssocList
//
DL6_IC_CONSTRUCTOR(StrAssocList, DL6ICLS_StrAssocList_VTab)
	{ ExtraPtr = new StrAssocArray; }
DL6_IC_DESTRUCTOR(StrAssocList)
	{ delete (StrAssocArray *)ExtraPtr; }
//
// Interface IStrAssocList implementation
//
int32 DL6ICLS_StrAssocList::GetCount()
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	return p_data ? (int32)p_data->getCount() : RaiseAppError();
}

static void StrAssocItemToSTaggedString(const StrAssocArray::Item & rSrc, STaggedString * pDest)
{
	if(pDest) {
		pDest->Id = rSrc.Id;
		pDest->ParentId = rSrc.ParentId;
		SString temp_buf = rSrc.Txt;
		temp_buf.CopyToOleStr(&pDest->Text);
	}
}

static void ClearSTaggedString(STaggedString * pDest)
{
	if(pDest) {
		pDest->Id = pDest->ParentId = 0;
		SysFreeString(pDest->Text);
		pDest->Text = 0;
	}
}

int32 DL6ICLS_StrAssocList::Get(int32 pos, STaggedString* pItem)
{
	int    ok = 0;
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(p_data) {
		StrAssocItemToSTaggedString(p_data->at((uint)pos), pItem);
		if(((uint)pos) < p_data->getCount())
			ok = 1;
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_StrAssocList::SearchById(int32 id, STaggedString* pItem)
{
	int    ok = 0;
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data))) {
		uint pos = 0;
		if(p_data->Search(id, &pos)) {
			StrAssocItemToSTaggedString(p_data->at(pos), pItem);
			ok = 1;
		}
		else
			ClearSTaggedString(pItem);
	}
	return ok;
}

int32 DL6ICLS_StrAssocList::SearchByText(SString & text, STaggedString* pItem)
{
	int    ok = 0;
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data))) {
		uint pos = 0;
		if(p_data->SearchByText(text, PTR_CMPFUNC(PcharNoCase), &pos)) {
			StrAssocItemToSTaggedString(p_data->at(pos), pItem);
			ok = 1;
		}
		else
			ClearSTaggedString(pItem);
	}
	return ok;
}

IMPL_CMPFUNC(PcharNoCaseLen, i1, i2) { return strnicmp866((Pchar)i1, (char *)i2, strlen((Pchar)i2)); }

int32 DL6ICLS_StrAssocList::SearchByTextPattern(SString & rText, STaggedString * pItem)
{
	int    ok = 0;
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data))) {
		uint pos = 0;
		if(p_data->SearchByText(rText, PTR_CMPFUNC(PcharNoCaseLen), &pos)) {
			StrAssocItemToSTaggedString(p_data->at(pos), pItem);
			ok = 1;
		}
		else
			ClearSTaggedString(pItem);
	}
	return ok;
}

SString & DL6ICLS_StrAssocList::GetTextById(int32 id)
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(p_data) {
		uint pos = 0;
		RetStrBuf = p_data->Search(id, &pos) ? p_data->at(pos).Txt : (const char *)0;
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return RetStrBuf;
}

void DL6ICLS_StrAssocList::InitIteration()
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data)))
		p_data->setPointer(0);
}

int32 DL6ICLS_StrAssocList::NextIteration(STaggedString* pItem)
{
	int    ok = 0;
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data))) {
		uint p = p_data->getPointer();
		if(p < p_data->getCount()) {
			StrAssocItemToSTaggedString(p_data->at(p), pItem);
			p_data->incPointer();
			ok = 1;
		}
		else
			ClearSTaggedString(pItem);
	}
	return ok;
}

void DL6ICLS_StrAssocList::Clear()
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data)))
		p_data->Clear();
}

void DL6ICLS_StrAssocList::Add(int32 itemId, int32 parentId, SString & text)
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	SetAppError(p_data && p_data->Add(itemId, parentId, text, 1));
}

void DL6ICLS_StrAssocList::Sort(int32 byText)
{
	StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
	if(SetAppError(BIN(p_data)))
		if(byText)
			p_data->SortByText();
		else
			p_data->SortByID();
}

void DL6ICLS_StrAssocList::Clone(IStrAssocList** ppClone)
{
	IStrAssocList * p = 0;
	if(SetAppError(CreateInnerInstance(P_Scope->GetName(), "IStrAssocList", (void **)&p))) {
		StrAssocArray * p_data = (StrAssocArray *)ExtraPtr;
		StrAssocArray * p_outer_data = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p);
		if(SetAppError(p_data && p_outer_data))
			*p_outer_data = *p_data;
	}
	ASSIGN_PTR(ppClone, p);
}

int32 DL6ICLS_StrAssocList::CloneByParent(int32 parentId, IStrAssocList** ppClone)
{
	return FuncNotSupported();
}
//
// PPRetailPriceExtractor
//
DL6_IC_CONSTRUCTION_EXTRA(PPRtlPriceExtractor, DL6ICLS_PPRtlPriceExtractor_VTab, RetailPriceExtractor);
//
// Interface IPapyrusRtlPriceExtractor implementation
//
int32 DL6ICLS_PPRtlPriceExtractor::Init(int32 locID, int32 extQuotKindID, PpyRtlPriceFlags flags)
{
	if(ExtraPtr) {
		RetailPriceExtractor & r_rpe = *(RetailPriceExtractor*)ExtraPtr;
		RetailPriceExtractor::ExtQuotBlock eqb(extQuotKindID);
		return r_rpe.Init(locID, &eqb, 0, ZERODATETIME, flags);
	}
	else
		return -1;
}

int32 DL6ICLS_PPRtlPriceExtractor::GetPrice(int32 goodsID, SPpyI_RtlExtr * pItem)
{
	int    ok = -1;
	if(ExtraPtr && pItem) {
		RetailPriceExtractor & r_rpe = *(RetailPriceExtractor*)ExtraPtr;
		RetailExtrItem item;
		if(r_rpe.GetPrice(goodsID, 0, 0.0, &item) > 0) {
#define FLD(f) pItem->f = item.f
			FLD(Cost);
			FLD(Price);
			FLD(ExtPrice);
			pItem->CurLotDate = (OleDate)item.CurLotDate;
			pItem->Expiry = (OleDate)item.Expiry;
#undef FLD
			ok = 1;
		}
	}
	return ok;
}
//
// PPSFile
//
DL6_IC_CONSTRUCTION_EXTRA(PPSFile, DL6ICLS_PPSFile_VTab, SFile);
//
// Interface ISFile implementation
//
int32 DL6ICLS_PPSFile::Open(SString & rFileName, SFileMode mode)
{
	SFile * p_f = (SFile*)ExtraPtr;
	return (p_f) ? p_f->Open(rFileName, mode) : 0;
}

int32 DL6ICLS_PPSFile::Close()
{
	SFile * p_f = (SFile*)ExtraPtr;
	return (p_f) ? p_f->Close() : 0;
}

int32 DL6ICLS_PPSFile::IsValid()
{
	SFile * p_f = (SFile*)ExtraPtr;
	return (p_f) ? p_f->IsValid() : 0;
}

int32 DL6ICLS_PPSFile::WriteLine(SString & rBuf)
{
	SFile * p_f = (SFile*)ExtraPtr;
	return (p_f) ? p_f->WriteLine(rBuf) : 0;
}

int32 DL6ICLS_PPSFile::WriteLine2(SString & rBuf, ISCodepage cp)
{
	int    ok = 0;
	SFile * p_f = (SFile*)ExtraPtr;
	if(p_f) {
		if(cp == cpANSI || cp == cp1251)
			rBuf.Transf(CTRANSF_INNER_TO_OUTER);
		else if(cp == cpUTF8)
			rBuf.Transf(CTRANSF_INNER_TO_UTF8);
		else
			; // asis Строка в эту функцию передана в кодировке OEM
		ok = p_f->WriteLine(rBuf);
	}
	return ok;
}

int32 DL6ICLS_PPSFile::ReadLine(SString * pBuf)
{
	int    ok = 0;
	SFile * p_f = (SFile*)ExtraPtr;
	SString buf;
	if(p_f) {
		if((ok = p_f->ReadLine(buf)) > 0) {
			buf.TrimRightChr('\n').TrimRightChr('\r');
			ASSIGN_PTR(pBuf, buf);
		}
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return ok;
}

int32 DL6ICLS_PPSFile::CalcSize(long * pLoWord, long * pHiWord)
{
	int    ok = 0;
	SFile * p_f = (SFile*)ExtraPtr;
	SString buf;
	if(p_f) {
		int64 sz = 0;
		if(p_f->CalcSize(&sz)) {
			ASSIGN_PTR(pLoWord, (long)(sz & 0x00000000ffffffff));
			ASSIGN_PTR(pHiWord, (long)((sz & 0xffffffff00000000) >> 32));
			ok = 1;
		}
	}
	else {
		RetStrBuf = 0;
		AppError  = 1;
	}
	return ok;
}
//
// PPDbfTable
//
struct InnerExtraDbfCreateFlds {
	InnerExtraDbfCreateFlds()
	{
		Init();
	}
	void Init()
	{
		Flds.freeAll();
		Idx = 0;
	}
	int    Add(SDbfCreateFld * pFld);
	int    InitIteration();
	int    NextIteration(SDbfCreateFld * pFld);
	int32  GetCount();

	SStrCollection Flds;
	int32  Idx;
};

int InnerExtraDbfCreateFlds::Add(SDbfCreateFld * pFld)
{
	int ok = -1;
	if(pFld) {
		SString temp_buf;
		SDbfCreateFld * p_fld = new SDbfCreateFld;
		p_fld->Prec = pFld->Prec;
		p_fld->Size = pFld->Size;
		p_fld->Type = pFld->Type;
		p_fld->Name = 0;
		temp_buf.CopyFromOleStr(pFld->Name).CopyToOleStr(&p_fld->Name);
		Flds.insert(p_fld);
		ok = 1;
	}
	return ok;
}

int InnerExtraDbfCreateFlds::InitIteration()
{
	Idx = 0;
	return (Flds.getCount() > 0) ? 1 : -1;
}

int InnerExtraDbfCreateFlds::NextIteration(SDbfCreateFld * pFld)
{
	int ok = -1;
	if(Idx < (int32)Flds.getCount()) {
		if(pFld) {
			SString temp_buf;
			SDbfCreateFld * p_fld = (SDbfCreateFld*)Flds.at(Idx);
			pFld->Prec = p_fld->Prec;
			pFld->Size = p_fld->Size;
			pFld->Type = p_fld->Type;
			temp_buf.CopyFromOleStr(p_fld->Name).CopyToOleStr(&pFld->Name);
			Idx++;
		}
		ok = 1;
	}
	return ok;
}

int32 InnerExtraDbfCreateFlds::GetCount()
{
	return Flds.getCount();
}

DL6_IC_CONSTRUCTOR(PPDbfCreateFlds, DL6ICLS_PPDbfCreateFlds_VTab)
{
	ExtraPtr = new InnerExtraDbfCreateFlds;
}

DL6_IC_DESTRUCTOR(PPDbfCreateFlds)
{
	InnerExtraDbfCreateFlds * p_e = (InnerExtraDbfCreateFlds*)ExtraPtr;
	if(p_e)
		p_e->Init();
	ZDELETE(p_e);
}
//
// Interface ISDbfCreateFlds implementation
//
int32 DL6ICLS_PPDbfCreateFlds::Add(SDbfCreateFld * pFld)
{
	InnerExtraDbfCreateFlds * p_e = (InnerExtraDbfCreateFlds*)ExtraPtr;
	return (p_e) ? p_e->Add(pFld) : -1;
}

int32 DL6ICLS_PPDbfCreateFlds::InitIteration()
{
	InnerExtraDbfCreateFlds * p_e = (InnerExtraDbfCreateFlds*)ExtraPtr;
	return (p_e) ? p_e->InitIteration() : -1;
}

int32 DL6ICLS_PPDbfCreateFlds::NextIteration(SDbfCreateFld* pFld)
{
	InnerExtraDbfCreateFlds * p_e = (InnerExtraDbfCreateFlds*)ExtraPtr;
	return (p_e) ? p_e->NextIteration(pFld) : -1;
}

int32 DL6ICLS_PPDbfCreateFlds::GetCount()
{
	InnerExtraDbfCreateFlds * p_e = (InnerExtraDbfCreateFlds*)ExtraPtr;
	return (p_e) ? p_e->GetCount() : 0;
}

DL6_IC_CONSTRUCTOR(PPDbfRecord, DL6ICLS_PPDbfRecord_VTab)
	{ ExtraPtr = 0; }
DL6_IC_DESTRUCTOR(PPDbfRecord)
	{ delete (DbfRecord *)ExtraPtr; }
//
// Interface ISDbfRecord implementation
//
int32 DL6ICLS_PPDbfRecord::Empty()
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	return (p_rec) ? p_rec->empty() : -1;
}

int32 DL6ICLS_PPDbfRecord::PutString(int32 fldN, SString & val)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	// @v8.4.2 {
	if(p_rec) {
		SCodepage cp = p_rec->getCodePage();
		if(cp == cpANSI || cp == cp1251) {
			val.Transf(CTRANSF_INNER_TO_OUTER);
		}
		return p_rec->put(fldN, (const char*)val);
	}
	else
		return 0;
	// } @v8.4.2
	// @v8.4.2 return p_rec ? p_rec->put(fldN, (const char*)val) : 0;
}

int32 DL6ICLS_PPDbfRecord::PutDouble(int32 fldN, double val)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	return (p_rec) ? p_rec->put(fldN, val) : 0;
}

int32 DL6ICLS_PPDbfRecord::PutLong(int32 fldN, int32 val)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	return (p_rec) ? p_rec->put(fldN, val) : 0;
}

int32 DL6ICLS_PPDbfRecord::PutInt(int32 fldN, int32 val)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	return (p_rec) ? p_rec->put(fldN, (int)val) : 0;
}

int32 DL6ICLS_PPDbfRecord::PutDate(int32 fldN, LDATE val)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	return (p_rec) ? p_rec->put(fldN, val) : 0;
}

SString & DL6ICLS_PPDbfRecord::GetString(int32 fldN)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	RetStrBuf = 0;
	if(p_rec)
		p_rec->get(fldN, RetStrBuf);
	return RetStrBuf;
}

double DL6ICLS_PPDbfRecord::GetDouble(int32 fldN)
{
	double val = 0;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	if(p_rec)
		p_rec->get(fldN, val);
	return val;
}

int32 DL6ICLS_PPDbfRecord::GetLong(int32 fldN)
{
	long val = 0;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	if(p_rec)
		p_rec->get(fldN, val);
	return val;
}

int32 DL6ICLS_PPDbfRecord::GetInt(int32 fldN)
{
	int    val = 0;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	CALLPTRMEMB(p_rec, get(fldN, val));
	return (int32)val;
}

LDATE DL6ICLS_PPDbfRecord::GetDate(int32 fldN)
{
	LDATE val = ZERODATE;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	CALLPTRMEMB(p_rec, get(fldN, val));
	return val;
}

SString & DL6ICLS_PPDbfRecord::GetFieldName(uint32 fldN)
{
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	RetStrBuf = 0;
	if(p_rec) {
		char buf[128];
		memzero(buf, sizeof(buf));
		p_rec->getFieldName(fldN, buf, sizeof(buf));
		RetStrBuf.CopyFrom(buf);
	}
	return RetStrBuf;
}

int32 DL6ICLS_PPDbfRecord::GetFieldNumber(SString & rFldName)
{
	int    fldn = -1;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	CALLPTRMEMB(p_rec, getFieldNumber(rFldName, &fldn));
	return (int32)fldn;
}

SDbfFldType DL6ICLS_PPDbfRecord::GetFieldType(uint32 fldN)
{
	int    type = 0;
	DbfRecord * p_rec = (DbfRecord*)ExtraPtr;
	CALLPTRMEMB(p_rec, getFieldType(fldN, &type));
	return (SDbfFldType)type;
}

DL6_IC_CONSTRUCTOR(PPDbfTable, DL6ICLS_PPDbfTable_VTab)
	{ ExtraPtr = 0; }
DL6_IC_DESTRUCTOR(PPDbfTable)
	{ delete (DbfTable *)ExtraPtr; }
//
// Interface ISDbfTable implementation
//
int32 DL6ICLS_PPDbfTable::Open(SString & fileName)
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(!p_tbl) {
		p_tbl = new DbfTable(fileName);
		ExtraPtr = p_tbl;
	}
	else {
		Close();
		p_tbl->open();
	}
	return IsOpened();
}

int32 DL6ICLS_PPDbfTable::IsOpened()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->isOpened() : 0;
}

SString & DL6ICLS_PPDbfTable::GetName()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	RetStrBuf = 0;
	if(p_tbl)
		RetStrBuf.CopyFrom(p_tbl->getName());
	return RetStrBuf;
}

SString & DL6ICLS_PPDbfTable::GetFieldName(uint32 fldN)
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	RetStrBuf = 0;
	if(p_tbl) {
		char buf[128];
		memzero(buf, sizeof(buf));
		p_tbl->getFieldName(fldN, buf, sizeof(buf));
		RetStrBuf.CopyFrom(buf);
	}
	return RetStrBuf;
}

int32 DL6ICLS_PPDbfTable::GetFieldNumber(SString & rFldName)
{
	int fldn = -1;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl)
		p_tbl->getFieldNumber(rFldName, &fldn);
	return fldn;
}

uint32 DL6ICLS_PPDbfTable::GetNumRecs()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->getNumRecs() : 0;
}

uint32 DL6ICLS_PPDbfTable::GetRecSize()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->getRecSize() : 0;
}

uint32 DL6ICLS_PPDbfTable::GetNumFields()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->getNumFields() : 0;
}

uint32 DL6ICLS_PPDbfTable::GetPosition()
{
	ulong pos = 0;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl)
		p_tbl->getPosition(&pos);
	return pos;
}

int32 DL6ICLS_PPDbfTable::Close()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->close() : -1;
}

int32 DL6ICLS_PPDbfTable::Create(ISDbfCreateFlds * pFldsDescr)
{
	return Create3(pFldsDescr, icpOEM, -1);
}

int32 DL6ICLS_PPDbfTable::Create2(ISDbfCreateFlds * pFldsDescr, ISCodepage cp)
{
	return Create3(pFldsDescr, cp, -1);
}

int32 DL6ICLS_PPDbfTable::Create3(ISDbfCreateFlds * pFldsDescr, ISCodepage cp, long infoByte)
{
	int    ok = -1;
	int32  num_flds = 0;
	DBFCreateFld  * p_flds_info = 0;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	InnerExtraDbfCreateFlds * p_flds = (InnerExtraDbfCreateFlds*)SCoClass::GetExtraPtrByInterface(pFldsDescr);
	if(p_tbl && p_flds && (num_flds = p_flds->GetCount()) > 0) {
		SDbfCreateFld outer_fld_inf;
		THROW_MEM(p_flds_info = new DBFCreateFld[num_flds]);
		outer_fld_inf.Name = 0;
		p_flds->InitIteration();
		for(int i = 0; p_flds->NextIteration(&outer_fld_inf) > 0; i++) {
			SString temp_buf;
			DBFCreateFld inner_fld_inf;
			temp_buf.CopyFromOleStr(outer_fld_inf.Name);
			temp_buf.CopyTo(inner_fld_inf.Name, sizeof(inner_fld_inf.Name));
			if(outer_fld_inf.Type == dbftString)
				inner_fld_inf.Type = (uint8)'C';
			else if(outer_fld_inf.Type == dbftInt || outer_fld_inf.Type == dbftLong || outer_fld_inf.Type == dbftDouble)
				inner_fld_inf.Type = (uint8)'N';
			else if(outer_fld_inf.Type == dbftDate)
				inner_fld_inf.Type = (uint8)'D';
			else if(outer_fld_inf.Type == dbftLogical)
				inner_fld_inf.Type = (uint8)'L';
			else {
				THROW_PP(0, PPERR_DL200_INVENTRYTYPE);
			}
			inner_fld_inf.Size = (uint8)outer_fld_inf.Size;
			inner_fld_inf.Prec = (uint8)outer_fld_inf.Prec;
			p_flds_info[i] = inner_fld_inf;
		}
		ok = p_tbl->create(num_flds, p_flds_info, (SCodepage)cp, infoByte);
	}
	CATCHZOK
	ZDELETE(p_flds_info);
	return ok;
}

int32 DL6ICLS_PPDbfTable::GoToRec(uint32 num)
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->goToRec((ulong)num) : -1;
}

int32 DL6ICLS_PPDbfTable::Top()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->top() : -1;
}

int32 DL6ICLS_PPDbfTable::Bottom()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->bottom() : -1;
}

int32 DL6ICLS_PPDbfTable::Next()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->next() : -1;
}

int32 DL6ICLS_PPDbfTable::Prev()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->prev() : -1;
}

int32 DL6ICLS_PPDbfTable::DeleteRec()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->deleteRec() : -1;
}

ISDbfRecord * DL6ICLS_PPDbfTable::MakeRec()
{
	void * p_ifc = 0;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl) {
		THROW(CreateInnerInstance("PPDbfRecord", "ISDbfRecord", &p_ifc));
		SCoClass::SetExtraPtrByInterface(p_ifc, p_tbl->makeRec());
	}
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (ISDbfRecord*)p_ifc;
}

int32 DL6ICLS_PPDbfTable::GetRec(ISDbfRecord* pRec)
{
	int    ok = -1;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl) {
		DbfRecord * p_rec = (DbfRecord*)SCoClass::GetExtraPtrByInterface(pRec);
		if(p_rec)
			ok = p_tbl->getRec(p_rec);
	}
	return ok;
}

int32 DL6ICLS_PPDbfTable::AppendRec(ISDbfRecord* pRec)
{
	int    ok = -1;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl) {
		DbfRecord * p_rec = (DbfRecord*)SCoClass::GetExtraPtrByInterface(pRec);
		if(p_rec)
			ok = p_tbl->appendRec(p_rec);
	}
	return ok;
}

int32 DL6ICLS_PPDbfTable::UpdateRec(ISDbfRecord* pRec)
{
	int ok = -1;
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	if(p_tbl) {
		DbfRecord * p_rec = (DbfRecord*)SCoClass::GetExtraPtrByInterface(pRec);
		if(p_rec)
			ok = p_tbl->updateRec(p_rec);
	}
	return ok;
}

int32 DL6ICLS_PPDbfTable::Flush()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->flush() : -1;
}

int32 DL6ICLS_PPDbfTable::InitBuffer()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->initBuffer() : -1;
}

int32 DL6ICLS_PPDbfTable::ReleaseBuffer()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->releaseBuffer() : -1;
}

int32 DL6ICLS_PPDbfTable::IsDeletedRec()
{
	DbfTable * p_tbl = (DbfTable*)ExtraPtr;
	return (p_tbl) ? p_tbl->isDeletedRec() : -1;
}
//
// PPFtp
//
DL6_IC_CONSTRUCTOR(PPFtp, DL6ICLS_PPFtp_VTab)
	{ ExtraPtr = 0; }
DL6_IC_DESTRUCTOR(PPFtp)
	{ delete (WinInetFTP*)ExtraPtr; }
//
// Interface ISDbfTable implementation
//
int32 DL6ICLS_PPFtp::Init(SFtpConfig * pCfg)
{
	int    ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(!p_ftp) {
		p_ftp = new WinInetFTP;
		ExtraPtr = p_ftp;
	}
	if(p_ftp) {
		SString temp_buf;
		PPInetConnConfig cfg;
		MEMSZERO(cfg);
		// PPTXT_INETCONNAGENTS
		if(pCfg->Agent == ftpagNetscape)
			STRNSCPY(cfg.Agent, "Netscape Communicator");
		else if(pCfg->Agent == ftpagOpera)
			STRNSCPY(cfg.Agent, "Opera");
		else
			STRNSCPY(cfg.Agent, "Microsoft Internet Explorer");
		temp_buf.CopyFromOleStr(pCfg->ProxyHost);
		temp_buf.CopyTo(cfg.ProxyHost, sizeof(cfg.ProxyHost));
		temp_buf.CopyFromOleStr(pCfg->ProxyPort);
		temp_buf.CopyTo(cfg.ProxyPort, sizeof(cfg.ProxyPort));
		cfg.MaxTries = (pCfg->MaxTries > 0) ? pCfg->MaxTries : 3;
		cfg.AccessType = (long)pCfg->AccessType;
		ok = p_ftp->Init(&cfg);
	}
	return ok;
}

int32 DL6ICLS_PPFtp::Connect(SFtpAccount * pAcct)
{
	int ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp) {
		SString temp_buf;
		PPInternetAccount acct;
		temp_buf.CopyFromOleStr(pAcct->Host);
		acct.SetExtField(FTPAEXSTR_HOST, temp_buf);
		temp_buf.CopyFromOleStr(pAcct->User);
		acct.SetExtField(FTPAEXSTR_USER, temp_buf);
		temp_buf.CopyFromOleStr(pAcct->Password);
		acct.SetPassword(temp_buf);
		temp_buf.Cat((pAcct->Port) ? pAcct->Port : 21L);
		acct.SetExtField(FTPAEXSTR_PORT, temp_buf);
		acct.Flags = pAcct->Flags;
		ok = p_ftp->Connect(&acct);
	}
	return ok;
}

int32 DL6ICLS_PPFtp::Get(SString & rLocalPath, SString & rFtpPath)
{
	int ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp)
		ok = p_ftp->SafeGet(rLocalPath, rFtpPath, 0, 0, 0);
	return ok;
}

int32 DL6ICLS_PPFtp::Put(SString & rLocalPath, SString & rFtpPath)
{
	int    ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp)
		ok = p_ftp->SafePut(rLocalPath, rFtpPath, 0, 0, 0);
	return ok;
}

int32 DL6ICLS_PPFtp::Delete(SString & rFtpPath)
{
	int    ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp)
		ok = p_ftp->SafeDelete(rFtpPath, 0);
	return ok;
}

int32 DL6ICLS_PPFtp::DeleteWOCD(SString & rFtpPath)
{
	int    ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp)
		ok = p_ftp->SafeDeleteWOCD(rFtpPath, 0);
	return ok;
}

IStrAssocList * DL6ICLS_PPFtp::GetFileList(SString & rDir, SString & rMask)
{
	IUnknown * p = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	StrAssocArray * p_file_list = 0;
	THROW(CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p));
	THROW(p_file_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p));
	if(p_ftp)
		THROW(p_ftp->SafeGetFileList(rDir, p_file_list, rMask, 0));
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (IStrAssocList *)p;
}

int32 DL6ICLS_PPFtp::CD(SString & rFtpPath, int32 isFullPath)
{
	int32  ok = 0;
	WinInetFTP * p_ftp = (WinInetFTP*)ExtraPtr;
	if(p_ftp)
		THROW(ok = p_ftp->SafeCD(rFtpPath, isFullPath, 0));
	CATCH
		AppError = 1;
	ENDCATCH
	return ok;
}
//
// PPUtil
//
DL6_IC_CONSTRUCTOR(PPUtil, DL6ICLS_PPUtil_VTab)
	{}
DL6_IC_DESTRUCTOR(PPUtil)
	{}

//
// Interface IPapyrusUtil implementation
//
SDateRange DL6ICLS_PPUtil::StrToDateRange(SString & str)
{
	int    ok = 0;
	SDateRange outer_period;
	DateRange period;
	if(strtoperiod(str, &period, 0)) {
		period.Actualize(ZERODATE);
		if(checkdate(period.low, 1) && checkdate(period.upp, 1) && (!period.upp || diffdate(period.upp, period.low) >= 0)) {
			outer_period.Low = (OleDate)period.low;
			outer_period.Upp = (OleDate)period.upp;
			ok = 1;
		}
	}
	if(!ok) {
		outer_period.Low = 0;
		outer_period.Upp = 0;
		AppError = (PPSetError(PPERR_INVPERIODINPUT, str), 1);
	}
	return outer_period;
}

SString & DL6ICLS_PPUtil::DateRangeToStr(LDATE low, LDATE upp)
{
	DateRange period;
	period.Set(low, upp);
	char   temp_buf[64];
	RetStrBuf = periodfmt(&period, temp_buf);
	return RetStrBuf;
}

SString & DL6ICLS_PPUtil::ToChar(SString & rBuf)
{
	return rBuf.Transf(CTRANSF_INNER_TO_OUTER);
}

SString & DL6ICLS_PPUtil::ToOem(SString & rBuf)
{
	return rBuf.ToOem();
}

SString & DL6ICLS_PPUtil::UTF8ToOem(SString & rBuf)
{
	return rBuf.Utf8ToOem();
}

SString & DL6ICLS_PPUtil::UTF8ToChar(SString & rBuf)
{
	return rBuf.Utf8ToChar();
}

int32 DL6ICLS_PPUtil::ToLong(SString & rBuf)
{
	return rBuf.ToLong();
}

double DL6ICLS_PPUtil::ToDouble(SString & rBuf)
{
	return rBuf.ToReal();
}

int32 DL6ICLS_PPUtil::CheckFlag(int32 flags, int32 flag)
{
	return ((flags & flag) == flag) ? 1 : 0;
}

int32 DL6ICLS_PPUtil::GetSellAccSheet()
{
	return IfcImpCheckDictionary() ? ::GetSellAccSheet() : 0;
}

int32 DL6ICLS_PPUtil::GetAgentAccSheet()
{
	return IfcImpCheckDictionary() ? ::GetAgentAccSheet() : 0;
}

int32 DL6ICLS_PPUtil::GetSupplAccSheet()
{
	return IfcImpCheckDictionary() ? ::GetSupplAccSheet() : 0;
}

int32 DL6ICLS_PPUtil::ObjectToPerson(int32 articleID)
{
	return IfcImpCheckDictionary() ? ::ObjectToPerson(articleID) : 0;
}

int32 DL6ICLS_PPUtil::PersonToObject(int32 personID, int32 accSheetID)
{
	int32  obj_id = 0;
	if(IfcImpCheckDictionary()) {
		PPObjArticle ar_obj;
		ar_obj.P_Tbl->PersonToArticle(personID, accSheetID, &obj_id);
	}
	return obj_id;
}

int32 DL6ICLS_PPUtil::ObjectToWarehouse(long articleID)
{
	return IfcImpCheckDictionary() ? PPObjLocation::ObjToWarehouse(articleID) : 0;
}

int32 DL6ICLS_PPUtil::WarehouseToObject(int32 locID)
{
	return IfcImpCheckDictionary() ? PPObjLocation::WarehouseToObj(locID) : 0;
}

SString & DL6ICLS_PPUtil::GetObjectName(PpyObjectIdent objType, long objID)
 {
	SString msg_buf;
	PPID   ppy_objtyp = 0;
	THROW(IfcImpCheckDictionary());
	switch(objType) {
		case ppoOprKind:        ppy_objtyp = PPOBJ_OPRKIND;       break;
		case ppoCashNode:       ppy_objtyp = PPOBJ_CASHNODE;      break;
		case ppoQuotKind:       ppy_objtyp = PPOBJ_QUOTKIND;      break;
		case ppoAccSheet:       ppy_objtyp = PPOBJ_ACCSHEET;      break;
		case ppoArticle:        ppy_objtyp = PPOBJ_ARTICLE;       break;
		case ppoStaff:          ppy_objtyp = PPOBJ_STAFFLIST2;    break;
		case ppoSalCharge:      ppy_objtyp = PPOBJ_SALCHARGE;     break;
		case ppoStyloPalm:      ppy_objtyp = PPOBJ_STYLOPALM;     break;
		case ppoTranpModel:     ppy_objtyp = PPOBJ_TRANSPMODEL;   break;
		case ppoPersonCategory: ppy_objtyp = PPOBJ_PRSNCATEGORY;  break;
		case ppoCurrency:       ppy_objtyp = PPOBJ_CURRENCY;      break;
		case ppoGoodsClass:     ppy_objtyp = PPOBJ_GOODSCLASS;    break;
		case ppoGoods:          ppy_objtyp = PPOBJ_GOODS;         break;
		case ppoGoodsGroup:     ppy_objtyp = PPOBJ_GOODSGROUP;    break;
		case ppoLocation:       ppy_objtyp = PPOBJ_LOCATION;      break;
		case ppoBill:           ppy_objtyp = PPOBJ_BILL;          break;
		case ppoWorld:          ppy_objtyp = PPOBJ_WORLD;         break;
		case ppoRegister:       ppy_objtyp = PPOBJ_REGISTER;      break;
		case ppoPerson:         ppy_objtyp = PPOBJ_PERSON;        break;
		case ppoPersonRelType:  ppy_objtyp = PPOBJ_PERSONRELTYPE; break;
		case ppoTSession:       ppy_objtyp = PPOBJ_TSESSION;      break;
		case ppoPrjTask:        ppy_objtyp = PPOBJ_PRJTASK;       break;
		case ppoProject:        ppy_objtyp = PPOBJ_PROJECT;       break;
		case ppoBrand:          ppy_objtyp = PPOBJ_BRAND;         break;
		case ppoCCheck:         ppy_objtyp = PPOBJ_CCHECK;        break;
		default:
			msg_buf.Z().Cat(objType);
			break;
	}
	::GetObjectName(ppy_objtyp, objID, RetStrBuf);
	CATCH
		RetStrBuf = 0;
		AppError = 1;
	ENDCATCH
	return RetStrBuf;
}

int32 DL6ICLS_PPUtil::Spawnl(int32 wait, SString & rPath, SString & rParams)
{
	if(rParams.Len())
		_spawnl((wait) ? _P_WAIT : _P_NOWAIT, rPath, rParams, 0);
	else
		_spawnl((wait) ? _P_WAIT : _P_NOWAIT, rPath, rPath, 0);
	return 1;
}

int32 DL6ICLS_PPUtil::RemoveFile(SString & rFileName)
{
	SFile::Remove(rFileName);
	return 1;
}

int32 DL6ICLS_PPUtil::IsFileExists(SString & rFileName)
{
	return ::fileExists(rFileName);
}

SString & DL6ICLS_PPUtil::ReadPPIniParamS(PpyIniSection section, SString & rParam)
{
	RetStrBuf = 0;
	THROW(IfcImpCheckDictionary());
	{
		PPIniFile ini_file;
		ini_file.Get(section, rParam, RetStrBuf);
	}
	CATCH
		AppError  = 1;
		RetStrBuf = 0;
	ENDCATCH
	return RetStrBuf;
}

SString & DL6ICLS_PPUtil::ReadPPIniParam(PpyIniSection section, PpyIniParam param)
{
	RetStrBuf = 0;
	THROW(IfcImpCheckDictionary());
	{
		long   inner_param = 0;
		switch(param) {
			case SpecEncodeSymbs:
				inner_param = PPINIPARAM_SPECENCODESYMBS;
				break;
			case BaltikaWoTareBeerGGrpCode:
				inner_param = PPINIPARAM_BALTIKAWOTAREBEERGGRPCODE;
				break;
		}
		if(inner_param) {
			PPIniFile ini_file;
			ini_file.Get(section, inner_param, RetStrBuf);
		}
	}
	CATCH
		AppError  = 1;
		RetStrBuf = 0;
	ENDCATCH
	return RetStrBuf;
}

SString & DL6ICLS_PPUtil::EncodeString(SString & rSrc, SString & rEncodeStr, int32 decode)
{
	return (RetStrBuf = 0).EncodeString(rSrc, rEncodeStr, (int)decode);
}

int32 DL6ICLS_PPUtil::GetNativeCountry()
{
	PPID   id = 0;
	PPObjWorld obj_world;
	SetAppError(obj_world.GetNativeCountry(&id) > 0);
	return (int32)id;
}

SString & DL6ICLS_PPUtil::MakeGUID()
{
	S_GUID guid;
	guid.Generate();
	guid.ToStr(S_GUID::fmtIDL, RetStrBuf = 0);
	return RetStrBuf;
}

int32 DL6ICLS_PPUtil::SendMail(int32 acctID, SString & rSubject, SString & rMessage, SString & rMail, IStrAssocList * pAttachments)
{
	int    ok = 1;
	StrAssocArray * p_attachments = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(pAttachments);
	SStrCollection files_list;
	if(p_attachments)
		for(uint i = 0; i < p_attachments->getCount(); i++)
			files_list.insert(newStr(p_attachments->at(i).Txt));
	rSubject.Transf(CTRANSF_INNER_TO_UTF8);
	rMessage.Transf(CTRANSF_INNER_TO_UTF8);
	rMail.Transf(CTRANSF_INNER_TO_UTF8);
	// @v9.6.5 {
	if(acctID == 0) {
		PPAlbatrosConfig a_cfg;
		if(PPAlbatrosCfgMngr::Get(&a_cfg) > 0 && a_cfg.Hdr.MailAccID)
			acctID = a_cfg.Hdr.MailAccID;
	}
	// } @v9.6.5
	THROW(::SendMail(rSubject, rMessage, rMail, acctID, (p_attachments) ? &files_list : 0, 0));
	CATCHZOK
	return ok;
}

int32 DL6ICLS_PPUtil::GetTagValue(PpyObjectIdent objType, int32 objID, int32 tagID, SString * pValue)
{
	int    ok = -1;
	int32  obj_type = objType;
	SString tagv;
	if(obj_type) {
		ObjTagList list;
		if(PPRef->Ot.GetList(obj_type, objID, &list)) {
			const ObjTagItem * p_item = list.GetItem(tagID);
			if(p_item)
				ok = p_item->GetStr(tagv);
		}
	}
	SetAppError(ok);
	ASSIGN_PTR(pValue, tagv);
	return ok;
}

int32 DL6ICLS_PPUtil::GetTagGUID(PpyObjectIdent objType, int32 objID, int32 tagID, SString* pValue)
{
	int    ok = -1;
	SString tagv;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	if(tag_obj.Fetch(tagID, &tag_rec) > 0 && tag_rec.ObjTypeID == objType && tag_rec.TagDataType == OTTYP_GUID) {
		ObjTagItem tag_item;
        if(PPRef->Ot.GetTag(objType, objID, tagID, &tag_item) > 0) {
            ok = tag_item.GetStr(tagv);
        }
        else {
			S_GUID uuid;
            THROW(SearchObject(objType, objID, 0) > 0);
			THROW_SL(uuid.Generate());
			THROW(tag_item.SetGuid(tagID, &uuid))
			THROW(PPRef->Ot.PutTag(objType, objID, &tag_item, -1));
			THROW(PPRef->Ot.GetTag(objType, objID, tagID, &tag_item) > 0);
			ok = tag_item.GetStr(tagv);
        }
	}
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	ASSIGN_PTR(pValue, tagv);
	return ok;
}

int32 DL6ICLS_PPUtil::GetTagValueExt(PpyObjectIdent objType, int32 objID, int32 tagID, PpyTagValue* pValue)
{
	int    ok = -1;
	int32  obj_type = objType;
	SString tagv;
	if(obj_type) {
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(obj_type, objID, tagID, &tag_item) > 0) {
			if(pValue) {
				SString temp_buf;
				pValue->TagID = tag_item.TagID;
				pValue->ObjType = objType;
				pValue->ObjID = objID;
				pValue->TagType = (PpyOTagType)tag_item.TagDataType;
				tag_item.GetInt(&pValue->VInt);
				tag_item.GetReal(&pValue->VReal);
				tag_item.GetStr(temp_buf);
				temp_buf.CopyToOleStr(&pValue->VStr);
				{
					LDATETIME dtm;
					tag_item.GetTimestamp(&dtm);
					pValue->VDt = dtm.d;
					pValue->VTm = dtm.t;
				}
				if(tag_item.TagDataType == OTTYP_ENUM) {
					tag_item.GetEnumData(0, 0, 0, &temp_buf);
					temp_buf.CopyToOleStr(&pValue->VSymb);
				}
			}
			ok = 1;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPUtil::PutTagValue(PpyObjectIdent objType, int32 objID, int32 tagID, SString & rValue)
{
	int    ok = -1;
	int32  obj_type = objType;
	if(obj_type) {
		ObjTagItem item;
		item.Init(tagID);
		item.SetStr(tagID, rValue);
		ok = PPRef->Ot.PutTag(obj_type, objID, &item, 1);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPUtil::DiagBarcode(SString & inputBarcode, int32* pStd, SString* pDiagText, SString* pOutputBarcode)
{
	int    std = 0;
	int    diag = 0;
	SString outp_code, msg_buf;
	int    ok = PPObjGoods::DiagBarcode(inputBarcode, &diag, &std, &outp_code);
	if(ok <= 0) {
		PPObjGoods::GetBarcodeDiagText(diag, msg_buf);
	}
	ASSIGN_PTR(pStd, std);
	ASSIGN_PTR(pDiagText, msg_buf.Transf(CTRANSF_INNER_TO_OUTER));
	ASSIGN_PTR(pOutputBarcode, outp_code);
	return ok;
}

int32 DL6ICLS_PPUtil::RoshenMakeHash(SString & rBuf, SString * pHashCode)
{
	int    ok = 1;
	uint   len = rBuf.Len();
	uint64 hash = 0;
	SString hash_code;
	for(uint i = 0; i < len; i++)
		hash = hash * 31 + (int)rBuf.C(i);
	hash_code.Cat(hash);
	ASSIGN_PTR(pHashCode, hash_code);
	return ok;
}

int32 DL6ICLS_PPUtil::SetConfigParam(SString & param, SString & value)
{
    PPThreadLocalArea & r_tla = DS.GetTLA();
    r_tla.SetIfcConfigParam(param, value);
	/*
	if(param.CmpNC("GTaxVect_Prec") == 0) {
        long prec = value.ToLong();
        if(prec)
	}
	*/
	return 1;
}

int32 DL6ICLS_PPUtil::GetSupplInterchangeConfig(int32 supplID, PpySupplInterchangeConfig * pValue)
{
	PPSupplAgreement agt;
	int    result = PPObjArticle::GetSupplAgreement(supplID, &agt, 0);
	if(pValue) {
		SString temp_buf;
		pValue->SupplID = supplID;
		pValue->GoodsGrpID = agt.Ep.GoodsGrpID;
		pValue->ExpendOp = agt.Ep.ExpendOp;
		pValue->RcptOp = agt.Ep.RcptOp;
		pValue->SupplRetOp = agt.Ep.SupplRetOp;
		pValue->RetOp = agt.Ep.RetOp;
		pValue->MovInOp = agt.Ep.MovInOp;
		pValue->MovOutOp = agt.Ep.MovOutOp;
		pValue->PriceQuotID = agt.Ep.PriceQuotID;
		pValue->DefUnitID = agt.Ep.Fb.DefUnitID;
		pValue->ProtVer = agt.Ep.ProtVer;
		agt.Ep.GetExtStrData(agt.Ep.extssClientCode, temp_buf);
		temp_buf.CopyToOleStr(&pValue->ClientCode);
		agt.Ep.GetExtStrData(agt.Ep.extssEDIPrvdrSymb, temp_buf);
		temp_buf.CopyToOleStr(&pValue->EDIPrvdrSymb);
		agt.Ep.GetExtStrData(agt.Ep.extssRemoveAddr, temp_buf);
		temp_buf.CopyToOleStr(&pValue->RemoveAddr);
		agt.Ep.GetExtStrData(agt.Ep.extssAccsName, temp_buf);
		temp_buf.CopyToOleStr(&pValue->AccsName);
		agt.Ep.GetExtStrData(agt.Ep.extssAccsPassw, temp_buf);
		temp_buf.CopyToOleStr(&pValue->AccsPassw);
		agt.Ep.GetExtStrData(agt.Ep.extssTechSymbol, temp_buf);
		temp_buf.CopyToOleStr(&pValue->TechSymbol);
		pValue->SequenceID = agt.Ep.Fb.SequenceID; // @v9.5.0
		pValue->CliCodeTagID = agt.Ep.Fb.CliCodeTagID; // @v9.5.0
		pValue->LocCodeTagID = agt.Ep.Fb.LocCodeTagID; // @v9.5.0
		//
		{
			PPAlbatrosConfig acfg;
			PPAlbatrosCfgMngr::Get(&acfg);
			PPOprKind op_rec;
			if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND))
				pValue->OrderOp = acfg.Hdr.OpID;
			else
				pValue->OrderOp = 0;
		}
	}
	return result;
}

int32 DL6ICLS_PPUtil::UniformFileTransm(SString & srcUrl, SString & destUrl, int32 flags, IFileFormat iff, SString & accsName, SString & accsPassw)
{
	SUniformFileTransmParam param;
	param.SrcPath = srcUrl;
	param.DestPath = destUrl;
	param.Flags = flags;
	param.Format = iff;
	param.AccsName = accsName;
	param.AccsPassword = accsPassw;
	return param.Run(0, 0);
}
//
//
//
DL6_IC_CONSTRUCTOR(PPSysJournal, DL6ICLS_PPSysJournal_VTab)
{
}

DL6_IC_DESTRUCTOR(PPSysJournal)
{
}
//
// Interface IPapyrusSysJournal implementation
//
static ILongList* Impement_GetObjectListBySjSince(SCoClass * pCoCls, PpyObjectIdent objType, LDATE since, const PPIDArray & rAcnList)
{
	IUnknown * p = 0;
	LongArray * p_id_list = 0;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	LDATETIME dtm;
	dtm.Set(since, ZEROTIME);
	THROW(p_sj);
	THROW(pCoCls->CreateInnerInstance("LongList", "ILongList", (void **)&p));
	THROW(p_id_list = (LongArray *)SCoClass::GetExtraPtrByInterface(p));
	p_sj->GetObjListByEventSince(objType, &rAcnList, dtm, *p_id_list);
	CATCH
		ReleaseUnknObj(&p);
		pCoCls->AppError = 1;
	ENDCATCH
	return (ILongList*)p;
}

ILongList* DL6ICLS_PPSysJournal::GetCreatedObjectListSince(PpyObjectIdent objType, LDATE since)
{
	PPIDArray acn_list;
	if(objType == PPOBJ_BILL)
		acn_list.add(PPACN_TURNBILL);
	else
		acn_list.add(PPACN_OBJADD);
	return Impement_GetObjectListBySjSince(this, objType, since, acn_list);
}

ILongList* DL6ICLS_PPSysJournal::GetModifiedObjectListSince(PpyObjectIdent objType, LDATE since)
{
	PPIDArray acn_list;
	if(objType == PPOBJ_BILL)
		acn_list.add(PPACN_UPDBILL);
	else
		acn_list.add(PPACN_OBJUPD);
	return Impement_GetObjectListBySjSince(this, objType, since, acn_list);
}
//
// PPSession {
//
struct PPSessionData {
	long   ID;
};

DL6_IC_CONSTRUCTION_EXTRA(PPSession, DL6ICLS_PPSession_VTab, PPSessionData);
//
// Interface IPapyrusSession implementation
//
IStrAssocList* DL6ICLS_PPSession::GetDatabaseList(int32 nameKind)
{
	IUnknown * p = 0;
	THROW(CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p));
	StrAssocArray * p_list_env = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p);
	THROW(p_list_env);
	{
		PPDbEntrySet2 set;
		SString db_symb, db_name, db_path;
		PPIniFile ini_file;
		long   lo = 0;
		if(nameKind == 1)
			lo = DbLoginBlockArray::loUseFriendlyName;
		else if(nameKind == 2)
			lo = DbLoginBlockArray::loUseDbPath;
		else
			lo = DbLoginBlockArray::loUseDbSymb;
		THROW(ini_file.IsValid());
		THROW(set.ReadFromProfile(&ini_file, 0, 0));
		THROW(set.MakeList(p_list_env, lo));
	}
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (IStrAssocList *)p;
}

int32 DL6ICLS_PPSession::GetDatabaseInfo(int32 id, SPpyDatabaseInfo* pInfo)
{
	PPDbEntrySet2 set;
	DbLoginBlock blk;
	PPIniFile ini_file;
	THROW(ini_file.IsValid());
	THROW(set.ReadFromProfile(&ini_file, 0, 0));
	THROW_INVARG(id > 0 && id <= (long)set.GetCount());
	THROW(set.GetByID(id, &blk));
	if(pInfo) {
		SString temp_buf;
		pInfo->ID = id;
		pInfo->Flags = 0;
		pInfo->Type = 0;
		blk.GetAttr(DbLoginBlock::attrDbSymb, temp_buf);
		temp_buf.CopyToOleStr(&pInfo->Symb);
		blk.GetAttr(DbLoginBlock::attrDbFriendlyName, temp_buf);
		temp_buf.CopyToOleStr(&pInfo->Name);
		blk.GetAttr(DbLoginBlock::attrDbPath, temp_buf);
		temp_buf.CopyToOleStr(&pInfo->Path);
		blk.GetAttr(DbLoginBlock::attrDictPath, temp_buf);
		temp_buf.CopyToOleStr(&pInfo->SysPath);
	}
	CATCH
		AppError = 1;
	ENDCATCH
	return !AppError;
}

int32 DL6ICLS_PPSession::Login(SString & dbName, SString & userName, SString & password)
	{ return DS.Login(dbName, userName, password) ? 1 : RaiseAppError(); }
int32 DL6ICLS_PPSession::Logout()
	{ return DS.Logout() ? 1 : RaiseAppError(); }
SString & DL6ICLS_PPSession::GetObjectTitle(PpyObjectIdent objType)
	{ return ::GetObjectTitle(objType, RetStrBuf); }

IPapyrusObject * DL6ICLS_PPSession::CreateObject(PpyObjectIdent objType)
{
	void * p_ifc = 0;
	const char * p_cls_name = 0;
	SString msg_buf;
	THROW(IfcImpCheckDictionary());
	switch(objType) {
		case ppoUnit:           p_cls_name = "PPObjUnit";           break;
		case ppoOprKind:        p_cls_name = "PPObjOprKind";        break;
		case ppoCashNode:       p_cls_name = "PPObjCashNode";       break;
		case ppoQuotKind:       p_cls_name = "PPObjQuotKind";       break;
		case ppoTag:            p_cls_name = "PPObjTag";            break;
		case ppoGoodsTax:       p_cls_name = "PPObjGoodsTax";       break;
		case ppoAccSheet:       p_cls_name = "PPObjAccSheet";       break;
		case ppoArticle:        p_cls_name = "PPObjArticle";        break;
		case ppoStaff:          p_cls_name = "PPObjStaff";          break;
		case ppoTransport:      p_cls_name = "PPObjTransport";      break;
		case ppoCCheck:         p_cls_name = "PPObjCCheck";         break; // @v8.7.1
		case ppoProcessor:      p_cls_name = "PPObjProcessor";      break;
		case ppoTSession:       p_cls_name = "PPObjTSession";       break;
		case ppoSalCharge:      p_cls_name = "PPObjSalCharge";      break;
		case ppoStyloPalm:      p_cls_name = "PPObjStyloPalm";      break;
		case ppoCurrency:       p_cls_name = "PPObjCurrency";       break;
		case ppoGoodsClass:     p_cls_name = "PPObjGoodsClass";     break;
		case ppoGoods:          p_cls_name = "PPObjGoods";          break;
		case ppoGoodsGroup:     p_cls_name = "PPObjGoodsGroup";     break;
		case ppoLocation:       p_cls_name = "PPObjLocation";       break;
		case ppoBill:           p_cls_name = "PPObjBill";           break;
		case ppoWorld:          p_cls_name = "PPObjWorld";          break;
		case ppoRegister:       p_cls_name = "PPObjRegister";       break;
		case ppoPerson:         p_cls_name = "PPObjPerson";         break;
		case ppoPersonRelType:  p_cls_name = "PPObjPersonRelType";  break;
		case ppoPrjTask:        p_cls_name = "PPObjPrjTask";        break;
		case ppoProject:        p_cls_name = "PPObjProject";        break;
		case ppoBrand:          p_cls_name = "PPObjBrand";          break;
		case ppoQCert:          p_cls_name = "PPObjQCert";          break;
		case ppoDebtDim:        p_cls_name = "PPObjDebtDim";        break;
		case ppoSCardSeries:    p_cls_name = "PPObjSCardSeries";    break; // @v8.7.1
		case ppoSCard:          p_cls_name = "PPObjSCard";          break; // @v8.7.1
		default:
			msg_buf.Z().Cat(objType);
			break;
	}
	THROW_PP_S(p_cls_name, PPERR_UNDEFPPOBJTYPE, msg_buf);
	THROW(CreateInnerInstance(p_cls_name, "IPapyrusObject", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IPapyrusObject *)p_ifc;
}

IPapyrusUtil * DL6ICLS_PPSession::CreateUtil()
{
	void * p_ifc = 0;
	THROW(IfcImpCheckDictionary());
	THROW(CreateInnerInstance("PPUtil", "IPapyrusUtil", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IPapyrusUtil*)p_ifc;
}

IUnknown * DL6ICLS_PPSession::CreateSpecClass(PpySpecClassIdent clsType)
{
	void * p_ifc = 0;
	const char * p_cls_name = 0, * p_ifc_name = 0;
	SString msg_buf;
	THROW(IfcImpCheckDictionary());
	switch(clsType) {
		case spclsUtil:
			p_cls_name = "PPUtil";
			p_ifc_name = "IPapyrusUtil";
			break;
		case spclsRtlPriceExtr:
			p_cls_name = "PPRtlPriceExtractor";
			p_ifc_name = "IPapyrusRtlPriceExtractor";
			break;
		case spclsDL200Resolver:
			p_cls_name = "PPDL200Resolver";
			p_ifc_name = "IPapyrusDL200Resolver";
			break;
		case spclsQuotation:
			p_cls_name = "PPQuotation";
			p_ifc_name = "IPapyrusQuot";
			break;
		case spclsAlcRepOpList:
			p_cls_name = "AlcRepOpList";
			p_ifc_name = "IAlcRepOpList";
			break;
		case spclsPrcssrAlcReport:
			p_cls_name = "PrcssrAlcReport";
			p_ifc_name = "IPrcssrAlcReport";
			break;
		case spclsCCheck: // @v8.7.1
			p_cls_name = "PPObjCCheck";
			p_ifc_name = "IPapyrusObjCCheck";
			break;
		default:
			msg_buf.Z().Cat(clsType);
			break;
	}
	THROW_PP_S(p_cls_name && p_ifc_name, PPERR_UNDEFCLSTYPE, msg_buf);
	THROW(CreateInnerInstance(p_cls_name, p_ifc_name, &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IUnknown*)p_ifc;
}

IPapyrusView * DL6ICLS_PPSession::CreateView(PpyViewIdent viewID)
{
	void * p_ifc = 0;
	const char * p_cls_name = 0;
	SString msg_buf;
	THROW(IfcImpCheckDictionary());
	switch(viewID) {
		case ppvCCheck:      p_cls_name = "PPViewCCheck";         break;
		case ppvTrfrAnlz:    p_cls_name = "PPViewTrfrAnlz";       break;
		case ppvLot:         p_cls_name = "PPViewLot";            break;
		case ppvGoods:       p_cls_name = "PPViewGoods";          break;
		case ppvGoodsRest:   p_cls_name = "PPViewGoodsRest";      break;
		case ppvBill:        p_cls_name = "PPViewBill";           break;
		case ppvGoodsOpAnlz: p_cls_name = "PPViewGoodsOpAnlz";    break;
		case ppvTSession:    p_cls_name = "PPViewTSession";       break;
		case ppvPrjTask:     p_cls_name = "PPViewPrjTask";        break;
		case ppvProject:     p_cls_name = "PPViewProject";        break;
		case ppvOpGrouping:  p_cls_name = "PPViewOpGrouping";     break;
		case ppvDebtTrnovr:  p_cls_name = "PPViewDebtTrnovr";     break;
		case ppvLotOp:       p_cls_name = "PPViewLotOp";          break;
		default:
			msg_buf.Z().Cat(viewID);
			break;
	}
	THROW_PP_S(p_cls_name, PPERR_UNKNOWNVIEWID, msg_buf);
	THROW(CreateInnerInstance(p_cls_name, "IPapyrusView", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IPapyrusView *)p_ifc;
}

int32 DL6ICLS_PPSession::GetStatusInfo(SPpySessionInfo* pInfo)
{
	if(pInfo) {
		SString temp_buf;
		PPThreadLocalArea & r_tla = DS.GetTLA();
		pInfo->SessID = r_tla.GetId();
		pInfo->Flags = 0;
		DbProvider * p_db = CurDict;
		if(p_db) {
			p_db->GetDbName(temp_buf);
			temp_buf.CopyToOleStr(&pInfo->DbName);
			p_db->GetDbSymb(temp_buf);
			temp_buf.CopyToOleStr(&pInfo->DbSymb);
			p_db->GetSysPath(temp_buf);
			temp_buf.CopyToOleStr(&pInfo->SysPath);
			p_db->GetDataPath(temp_buf);
			temp_buf.CopyToOleStr(&pInfo->DbPath);
		}
		else {
			temp_buf.Z();
			temp_buf.CopyToOleStr(&pInfo->DbName);
			temp_buf.CopyToOleStr(&pInfo->DbSymb);
			temp_buf.CopyToOleStr(&pInfo->SysPath);
			temp_buf.CopyToOleStr(&pInfo->DbPath);
		}
		pInfo->DbDivID = DS.LCfg().DBDiv;
		pInfo->UserID  = DS.LCfg().User;
		GetMainOrgID(&pInfo->MainOrgID);
		{
			r_tla.InitMainOrgData(0); // @v8.6.1
			pInfo->MainOrgDirector   = CConfig.MainOrgDirector_;
			pInfo->MainOrgAccountant = CConfig.MainOrgAccountant_;
		}
	}
	return 1;
}

struct LoginDialogParam {
	void GetText(HWND hDlg, uint ctl, SString & rBuf);
	void SetText(HWND hDlg, uint ctl, SString & rBuf);
	void SetupDBSelCombo(HWND hDlg);
	void GetDBSel(HWND hwndDlg);

	PPDbEntrySet2 * P_DbEs;
	SString UserName;
	SString Password;
};

void LoginDialogParam::SetText(HWND hDlg, uint ctl, SString & rBuf)
{
	SString buf;
	(buf = rBuf).Transf(CTRANSF_INNER_TO_OUTER);
	// @v9.1.5 SendMessage(GetDlgItem(hDlg, ctl), (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)(const char*)buf);
	TView::SSetWindowText(GetDlgItem(hDlg, ctl), buf); // @v9.1.5
}

void LoginDialogParam::GetText(HWND hDlg, uint ctl, SString & rBuf)
{
	// @v9.1.5 char * p_buf = 0;
	HWND hwnd_ctl = GetDlgItem(hDlg, ctl);
	size_t buf_size = SendMessage(hwnd_ctl, (UINT)WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0) + 1;
	// @v9.1.5 p_buf = new char[buf_size];
	// @v9.1.5 memzero(p_buf, buf_size);
	// @v9.1.5 SendMessage(hwnd_ctl, (UINT)WM_GETTEXT, (WPARAM)buf_size, (LPARAM)p_buf);
	TView::SGetWindowText(hwnd_ctl, rBuf);
	rBuf.Transf(CTRANSF_OUTER_TO_INNER);
	// @v9.1.5 (rBuf = p_buf).ToOem();
	// @v9.1.5 delete []p_buf;
}

void LoginDialogParam::SetupDBSelCombo(HWND hDlg)
{
	if(P_DbEs) {
		PPID   entry_id = P_DbEs->GetSelection();
		SString n, pn;
		DbLoginBlock dlb;
		for(uint i = 1; i <= P_DbEs->GetCount(); i++) {
			P_DbEs->GetByID(i, &dlb);
			dlb.GetAttr(DbLoginBlock::attrDbSymb, n);
			dlb.GetAttr(DbLoginBlock::attrDbFriendlyName, pn);
			if(pn.Empty())
				pn = n;
			if(pn.NotEmptyS()) {
				pn.Transf(CTRANSF_INNER_TO_OUTER);
				SendDlgItemMessage(hDlg, CTLSEL_LOGIN_DB, CB_ADDSTRING, (WPARAM)0, (LPARAM)pn.cptr());
				if(i == entry_id)
					SendDlgItemMessage(hDlg, CTLSEL_LOGIN_DB, CB_SETCURSEL, (WPARAM)i - 1, 0);
			}
		}
	}
}

void LoginDialogParam::GetDBSel(HWND hDlg)
{
	if(P_DbEs) {
		long sel = (long)SendDlgItemMessage((HWND) hDlg, CTLSEL_LOGIN_DB, (UINT)CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
		P_DbEs->SetSelection(sel + 1);
	}
}

BOOL CALLBACK LoginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LoginDialogParam * p_param = (LoginDialogParam*)TView::GetWindowUserData(hwndDlg);
	switch(uMsg) {
		case WM_INITDIALOG:
			{
				long   x = 0, y = 0;
				RECT   parent_rect, rect;
				HWND   parent = GetParent(hwndDlg);
				SendDlgItemMessage(hwndDlg, CTLSEL_LOGIN_DB, CB_RESETCONTENT, (WPARAM)0,  (LPARAM)0);
				if(parent) {
					GetWindowRect(parent,  &parent_rect);
					GetWindowRect(hwndDlg, &rect);
					y  = (parent_rect.bottom - parent_rect.top) / 2 - (rect.bottom - rect.top) / 2;
					x  = (parent_rect.right - parent_rect.left) / 2 - (rect.right - rect.left) / 2;
					MoveWindow(hwndDlg, x, y, rect.right - rect.left, rect.bottom - rect.top, 0);
				}
				TView::PreprocessWindowCtrlText(hwndDlg); // @v9.1.1
				p_param = (LoginDialogParam*)lParam;
				if(p_param) {
					TView::SetWindowUserData(hwndDlg, p_param);
					SendDlgItemMessage(hwndDlg, CTL_LOGIN_PASSWORD, EM_SETPASSWORDCHAR, '*', 0);
					p_param->SetText(hwndDlg, CTL_LOGIN_NAME,     p_param->UserName);
					p_param->SetText(hwndDlg, CTL_LOGIN_PASSWORD, p_param->Password);
					p_param->SetupDBSelCombo(hwndDlg);
				}
			}
			break;
		case WM_COMMAND:
			{
				uint cmd = LOWORD(wParam);
				if(cmd == CTLSEL_LOGIN_DB) {
				}
				else if(cmd == STDCTL_OKBUTTON || cmd == IDCANCEL) {
					if(p_param && cmd != IDCANCEL) {
						p_param->GetText(hwndDlg, CTL_LOGIN_NAME, p_param->UserName);
						p_param->GetText(hwndDlg, CTL_LOGIN_PASSWORD, p_param->Password);
						p_param->GetDBSel(hwndDlg);
					}
					EndDialog(hwndDlg, (cmd == STDCTL_OKBUTTON) ? cmOK : cmCancel);
				}
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

int32 DL6ICLS_PPSession::LoginDialog(PPYHWND pParent, SString * pDbName, SString * pUserName, SString * pPassword, int32 editOnly)
{
	int32  ok = -1;
	PPID   dbentry_id = 0;
	HWND   parent = (pParent) ? *((HWND*)pParent) : 0;
	SString db_name;
	LDATE  dt = getcurdate_();
	PPIniFile  ini_file;
	PPDbEntrySet2 dbes;
	DbLoginBlock dlb;
	LoginDialogParam param;

	dbes.ReadFromProfile(&ini_file);
	RVALUEPTR(param.UserName, pUserName);
	RVALUEPTR(param.Password, pPassword);
	if(pDbName && pDbName->Len()) {
		SString data_path;
		dbentry_id = dbes.GetBySymb(*pDbName, &dlb);
		dlb.GetAttr(DbLoginBlock::attrDbPath, data_path);
		if(::access(data_path, 0) != 0)
			dbentry_id = 0;
	}
	else if(dbes.GetCount() == 1)
		dbentry_id = 1;
	dbes.SetSelection(dbentry_id);
	param.P_DbEs = &dbes;
	while(ok <= 0 && DialogBoxParam(SLS.GetHInst(), MAKEINTRESOURCE(DLGW_LOGIN), parent, (DLGPROC)LoginDialogProc, (LPARAM)(long)&param) == cmOK) {
		DS.SetOperDate(dt);
		dbes.GetByID(dbentry_id = dbes.GetSelection(), &dlb);
		dlb.GetAttr(DbLoginBlock::attrDbSymb, db_name);
		if(editOnly == 0) {
			if((ok = Login(db_name, param.UserName, param.Password)) <= 0) {
				SString buf;
				if(PPGetLastErrorMessage(DS.CheckExtFlag(ECF_SYSSERVICE), buf)) {
					::MessageBox(parent, (const char*)buf.Transf(CTRANSF_INNER_TO_OUTER), "", MB_OK|MB_ICONERROR); // @unicodeproblem
					AppError = 0;
				}
			}
		}
		else
			ok = 1;
		if(ok > 0) {
			ASSIGN_PTR(pUserName, param.UserName);
			ASSIGN_PTR(pPassword, param.Password);
			ASSIGN_PTR(pDbName, db_name);
			AppError = 0;
		}
	}
	SetAppError(ok);
	return ok;
}
//
// } PPSession
// PPDL200Resolver {
//
DL6_IC_CONSTRUCTOR(PPDL200Resolver, DL6ICLS_PPDL200Resolver_VTab)
{
	ExtraPtr = new DL2_Resolver;
}

DL6_IC_DESTRUCTOR(PPDL200Resolver)
{
	delete ((DL2_Resolver *)ExtraPtr);
}
//
// Interface IPapyrusDL200Resolver implementation
//
int32  DL6ICLS_PPDL200Resolver::SetPeriod(SDateRange * pPeriod)
{
	DateRange prd;
	DL2_Resolver * p_prcssr = (DL2_Resolver *)ExtraPtr;
	if(pPeriod)
		prd = OleDateRangeToDateRange(*pPeriod);
	return (p_prcssr) ? p_prcssr->SetPeriod(prd) : RaiseAppError();
}

double DL6ICLS_PPDL200Resolver::Resolve(SString & rExpression)
{
	DL2_Resolver * p_prcssr = (DL2_Resolver *)ExtraPtr;
	return (p_prcssr) ? p_prcssr->Resolve(rExpression) : RaiseAppError();
}

SString & DL6ICLS_PPDL200Resolver::ResolveName(SString & rMetaVar)
{
	RetStrBuf = 0;
	DL2_Resolver * p_prcssr = (DL2_Resolver *)ExtraPtr;
	if(SetAppError(BIN(p_prcssr)))
		p_prcssr->ResolveName(rMetaVar, RetStrBuf);
	return RetStrBuf;
}
//
// } PPDL200Resolver
//
// PPAmountList {
//
DL6_IC_CONSTRUCTION_EXTRA(PPAmountList, DL6ICLS_PPAmountList_VTab, AmtList)
//
// Interface IPapyrusAmountList implementation
//
int32 DL6ICLS_PPAmountList::GetCount()
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return (p_list) ? p_list->getCount() : -1;
}

int32 DL6ICLS_PPAmountList::InitIteration()
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return FuncNotSupported();
}

int32 DL6ICLS_PPAmountList::NextIteration(PpyAmountEntry * pEntry)
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return FuncNotSupported();
}

int32 DL6ICLS_PPAmountList::Add(PpyAmountEntry * pEntry)
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return (p_list && pEntry) ? p_list->Add((long)pEntry->AmtTypeID, pEntry->CurID, pEntry->Amt) : -1;
}

int32 DL6ICLS_PPAmountList::RemoveByPos(int32 pos)
{
	int ok = -1;
	AmtList * p_list = (AmtList*)ExtraPtr;
	if(p_list && pos < (long)p_list->getCount()) {
		p_list->atFree(pos);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPAmountList::Remove(PpyAmountType amtTypeID, int32 curID)
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return (p_list) ? p_list->Remove(amtTypeID, curID) : -1;
}

double DL6ICLS_PPAmountList::Get(PpyAmountType amtTypeID, int32 curID)
{
	AmtList * p_list = (AmtList*)ExtraPtr;
	return (p_list) ? p_list->Get((long)amtTypeID, curID) : 0;
}

int32 DL6ICLS_PPAmountList::GetByPos(int32 pos, PpyAmountEntry * pItem)
{
	int    ok = -1;
	AmtList * p_list = (AmtList*)ExtraPtr;
	if(p_list && pItem && pos < (long)p_list->getCount()) {
		AmtEntry item = p_list->at(pos);
		pItem->AmtTypeID = (PpyAmountType)item.AmtTypeID;
		pItem->CurID     = item.CurID;
		pItem->Amt       = item.Amt;
		ok = 1;
	}
	return ok;
}

void DL6ICLS_PPAmountList::Clear()
{
	AmtList * p_list = (AmtList *)ExtraPtr;
	CALLPTRMEMB(p_list, freeAll());
}
//
// } PPAmountList
// PPObjTag {
//
static void FillObjTagRec(const PPObjectTag * pInner, SPpyO_Tag * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = pInner->Tag;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(Flags);
	FLD(LinkObjGrp);
	FLD(TagEnumID);
	FLD(ObjTypeID);
	FLD(TagGroupID);
	#undef FLD
	pOuter->TagDataType = (PpyOTagType)pInner->TagDataType;
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjTag, DL6ICLS_PPObjTag_VTab, PPObjTag);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjTag::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjTag * p_obj = (PPObjTag *)ExtraPtr;
	if(p_obj) {
		PPObjectTag tag_rec;
		MEMSZERO(tag_rec);
		ok = p_obj->Search(id, &tag_rec);
		FillObjTagRec(&tag_rec, (SPpyO_Tag*)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjTag::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjTag * p_obj = (PPObjTag *)ExtraPtr;
	if(p_obj) {
		PPObjectTag tag_rec;
		PPID   id = 0;
		MEMSZERO(tag_rec);
		if(kind == 1) {
			ok = p_obj->SearchBySymb(text, &id);
			if(ok > 0 && p_obj->Search(id, &tag_rec) > 0) {
				;
			}
			else
				ok = -1;
		}
		else { // (kind == 0)
			ok = p_obj->SearchByName(text, &id, &tag_rec);
		}
		FillObjTagRec(&tag_rec, (SPpyO_Tag *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjTag::GetName(int32 id)
{
	char   name_buf[64];
	PPObjTag * p_obj = (PPObjTag *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjTag::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjTag::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjTag::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjTag
// PPObjUnit {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjUnit, DL6ICLS_PPObjUnit_VTab, PPObjUnit);
//
// Interface IPapyrusObject implementation
//
static void FillUnitRec(const PPUnit * pInner, SPpyO_Unit * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(BaseRatio);
	pOuter->Flags = (PpyOUnitFlags)pInner->Flags;
	FLD(BaseUnitID);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

int32 DL6ICLS_PPObjUnit::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjUnit * p_obj = (PPObjUnit *)ExtraPtr;
	if(p_obj) {
		PPUnit u_rec;
		MEMSZERO(u_rec);
		ok = p_obj->Search(id, &u_rec);
		FillUnitRec(&u_rec, (SPpyO_Unit*)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjUnit::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjUnit * p_obj = (PPObjUnit *)ExtraPtr;
	if(p_obj) {
		PPUnit u_rec;
		PPID   id = 0;
		MEMSZERO(u_rec);
		ok = p_obj->SearchByName(text, &id, &u_rec);
		FillUnitRec(&u_rec, (SPpyO_Unit *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjUnit::GetName(int32 id)
{
	char   name_buf[64];
	PPObjUnit * p_obj = (PPObjUnit *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList * DL6ICLS_PPObjUnit::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjUnit::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjUnit::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjUnit
//
// PPObjOprKind {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjOprKind, DL6ICLS_PPObjOprKind_VTab, PPObjOprKind)
//
// Interface IPapyrusObject implementation
//
static void FillOprKindRec(const PPOprKind * pInner, SPpyO_OprKind * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(Tag);
	FLD(ID);
	FLD(Rank);
	FLD(LinkOpID);
	FLD(AccSheet2ID);
	FLD(OpCounterID);
	pOuter->PrnFlags = (PpyOOprKindPrnFlags)pInner->PrnFlags;
	FLD(DefLocID);
	FLD(PrnOrder);
	pOuter->SubType  = (PpyOOprKindSubType)pInner->SubType;
	pOuter->Flags    = (PpyOOprKindFlags)pInner->Flags;
	pOuter->OpTypeID = (PpyOOprKindType)pInner->OpTypeID;
	FLD(AccSheetID);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

int32 DL6ICLS_PPObjOprKind::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjOprKind * p_obj = (PPObjOprKind *)ExtraPtr;
	if(p_obj) {
		PPOprKind oprk_rec;
		ok = p_obj->Search(id, &oprk_rec);
		FillOprKindRec(&oprk_rec, (SPpyO_OprKind *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjOprKind::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjOprKind * p_obj = (PPObjOprKind *)ExtraPtr;
	if(p_obj) {
		PPOprKind op_rec;
		PPID id = 0;
		if(kind == 1) {
			ok = p_obj->SearchBySymb(text, &id);
			if(ok > 0 && p_obj->Search(id, &op_rec) > 0) {
				;
			}
			else
				ok = -1;
		}
		else { // (kind == 0)
			ok = p_obj->SearchByName(text, &id, &op_rec);
		}
		FillOprKindRec(&op_rec, (SPpyO_OprKind *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjOprKind::GetName(int32 id)
{
	char   name_buf[64];
	PPObjOprKind * p_obj = (PPObjOprKind *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList * DL6ICLS_PPObjOprKind::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjOprKind::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjOprKind::Update(int32 id, long flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// Interface IPapyrusObjOprKind implementation
//
int32 DL6ICLS_PPObjOprKind::GetDraftExData(int32 opID, SPpyO_DraftOpEx * pData)
{
	int    ok = -1;
	PPObjOprKind * p_obj = (PPObjOprKind *)ExtraPtr;
	if(p_obj) {
		PPDraftOpEx doe;
		if((ok = p_obj->GetDraftExData(opID, &doe)) > 0) {
#define FLD(f) pData->f = doe.f
		FLD(WrOffOpID);
		FLD(WrOffObjID);
		FLD(WrOffComplOpID);
		pData->Flags = (PpyODraftOpFlags)doe.Flags;
#undef FLD
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjOprKind::BelongToGeneric(int32 opID, int32 genOpID)
{
	int    ok = 0;
	PPObjOprKind * p_obj = (PPObjOprKind *)ExtraPtr;
	if(p_obj) {
		PPIDArray op_list;
		ok = BIN(opID == genOpID || p_obj->GetGenericList(genOpID, &op_list) > 0 && op_list.lsearch(opID) > 0);
	}
	return ok;
}
//
// } PPObjOprKind
// PPObjAccSheet {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjAccSheet, DL6ICLS_PPObjAccSheet_VTab, PPObjAccSheet)
//
// Interface IPapyrusObject implementation
//
static void FillAccSheetRec(const PPAccSheet * pInner, SPpyO_AccSheet * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(BinArID);
	FLD(CodeRegTypeID);
	FLD(Flags);
	FLD(Assoc);
	FLD(ObjGroup);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	memzero(pOuter->Reserve, sizeof(pOuter->Reserve));
}

int32 DL6ICLS_PPObjAccSheet::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjAccSheet * p_obj = (PPObjAccSheet *)ExtraPtr;
	if(p_obj) {
		PPAccSheet acs_rec;
		ok = p_obj->Fetch(id, &acs_rec);
		FillAccSheetRec(&acs_rec, (SPpyO_AccSheet *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjAccSheet::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjAccSheet * p_obj = (PPObjAccSheet *)ExtraPtr;
	if(p_obj) {
		PPAccSheet acs_rec;
		PPID   id = 0;
		ok = p_obj->SearchByName(text, &id, &acs_rec);
		FillAccSheetRec(&acs_rec, (SPpyO_AccSheet *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjAccSheet::GetName(int32 id)
{
	PPObjAccSheet * p_obj = (PPObjAccSheet *)ExtraPtr;
	if(p_obj) {
		PPAccSheet acs_rec;
		if(p_obj->Fetch(id, &acs_rec) > 0)
			RetStrBuf = acs_rec.Name;
		else
			ideqvalstr(id, RetStrBuf);
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjAccSheet::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjAccSheet::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjAccSheet::Update(int32 id, long flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjAccSheet
// PPObjArticle {
//
void FillArticleRec(const PPArticlePacket * pInner, SPpyO_Article * pOuter)
{
	SString temp_buf;
#define FLD_REC(f) pOuter->f = pInner->Rec.f
	FLD_REC(AccSheetID);
	FLD_REC(Article);
	FLD_REC(ObjID);
	FLD_REC(Closed);
	FLD_REC(Flags);
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	memzero(pOuter->Reserve, sizeof(pOuter->Reserve));
#undef FLD_REC
#define FLD_CA(f) pOuter->Ca##f = pInner->P_CliAgt->f
	if(pInner->P_CliAgt) {
		FLD_CA(Flags);
		pOuter->CaBegDt   = (OleDate)pInner->P_CliAgt->BegDt;
		pOuter->CaExpiry  = (OleDate)pInner->P_CliAgt->Expiry;
		FLD_CA(MaxCredit);
		FLD_CA(MaxDscnt);
		FLD_CA(Dscnt);
		FLD_CA(DefPayPeriod);
		FLD_CA(DefAgentID);
		FLD_CA(DefQuotKindID);
		(temp_buf = pInner->P_CliAgt->Code).CopyToOleStr(&pOuter->CaCode);
	}
	else {
		pOuter->CaFlags = 0;
		pOuter->CaBegDt = 0;
		pOuter->CaExpiry = 0;
		pOuter->CaMaxCredit = 0;
		pOuter->CaMaxDscnt = 0;
		pOuter->CaDscnt = 0;
		pOuter->CaDefPayPeriod = 0;
		pOuter->CaDefAgentID = 0;
		pOuter->CaDefQuotKindID = 0;
	}
	memzero(pOuter->CaReserve, sizeof(pOuter->CaReserve));
#undef FLD_CA
#define FLD_SA(f) pOuter->Sa##f = pInner->P_SupplAgt->f
	if(pInner->P_SupplAgt) {
		FLD_SA(Flags);
		pOuter->SaBegDt   = (OleDate)pInner->P_SupplAgt->BegDt;
		pOuter->SaExpiry  = (OleDate)pInner->P_SupplAgt->Expiry;
		FLD_SA(DefPayPeriod);
		FLD_SA(DefAgentID);
		FLD_SA(DefDlvrTerm);
		FLD_SA(PctRet);
	}
	else {
		pOuter->SaFlags = 0;
		pOuter->SaBegDt = 0;
		pOuter->SaExpiry = 0;
		pOuter->SaDefPayPeriod = 0;
		pOuter->SaDefAgentID = 0;
		pOuter->SaDefDlvrTerm = 0;
		pOuter->SaPctRet = 0;
	}
	memzero(pOuter->SaReserve, sizeof(pOuter->SaReserve));
#undef FLD_SA
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjArticle, DL6ICLS_PPObjArticle_VTab, PPObjArticle);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjArticle::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjArticle * p_obj = (PPObjArticle *)ExtraPtr;
	if(p_obj) {
		PPArticlePacket pack;
		SPpyO_Article * p_rec = (SPpyO_Article *)rec;
		if((ok = p_obj->GetPacket(id, &pack)) != 0) {
			p_rec->ID = id;
			FillArticleRec(&pack, p_rec);
		}
		else
			AppError = 1;
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPObjArticle::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjArticle * p_obj = (PPObjArticle *)ExtraPtr;
	if(p_obj) {
		ArticleTbl::Rec art_rec;
		SPpyO_Article * p_rec = (SPpyO_Article *)rec;
		MEMSZERO(art_rec);
		if((ok = p_obj->P_Tbl->SearchName(extraParam, text, &art_rec)) > 0) {
			PPArticlePacket pack;
			if((ok = p_obj->GetPacket(art_rec.ID, &pack)) != 0) {
				p_rec->ID = art_rec.ID;
				FillArticleRec(&pack, p_rec);
			}
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjArticle::GetName(int32 id)
{
	GetArticleName(id, RetStrBuf);
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjArticle::GetSelector(int32 extraParam)
{
	IStrAssocList * p = 0;
	PPObjArticle * p_ar_obj = (PPObjArticle *)ExtraPtr;
	if(p_ar_obj) {
		ArticleFilt filt;
		filt.AccSheetID = extraParam;
		filt.Ft_Closed = -1;
		p_ar_obj->SetCurrFilt(&filt);
		p = (IStrAssocList *)GetPPObjIStrAssocList(this, p_ar_obj, (long)&filt);
	}
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjArticle::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = -1;
	PPObjArticle * p_obj = (PPObjArticle *)ExtraPtr;
	SString temp_buf;
	THROW(p_obj);
	SPpyO_Article * p_rec = (SPpyO_Article *)pRec;
	THROW_PP_S(p_rec->RecTag == ppoArticle, PPERR_INVSTRUCTAG, "ppoArticle");
	temp_buf.CopyFromOleStr(p_rec->Name);
	THROW(p_obj->AddSimple(pID, p_rec->AccSheetID, temp_buf, p_rec->Article, (flags & 0x0001) ? 0 : 1));
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjArticle::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPObjArticle * p_obj = (PPObjArticle *)ExtraPtr;
	PPArticlePacket pack;
	SString temp_buf;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	THROW(p_obj);
	SPpyO_Article * p_rec = (SPpyO_Article *)rec;
	THROW_PP_S(p_rec->RecTag == ppoArticle, PPERR_INVSTRUCTAG, "ppoArticle");
	{
		PPTransaction tra((flags & 0x0001) ? 0 : 1);
		THROW(tra);
		THROW(p_obj->GetPacket(id, &pack) > 0);
		THROW(acs_obj.Fetch(pack.Rec.AccSheetID, &acs_rec) > 0);
		{
			pack.DontUpdateAliasSubst = 1; // Извне нам не могут прислать подстановку алиасов.
				// Следовательно, не меняем ее.
			temp_buf.CopyFromOleStr(p_rec->Name);
			if(temp_buf.NotEmptyS())
				temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
			pack.Rec.Flags |= (p_rec->Flags & ARTRF_STOPBILL);
			pack.Rec.Closed = p_rec->Closed ? 1 : 0;
			if(acs_rec.Flags & ACSHF_USECLIAGT || pack.P_CliAgt) {
				PPClientAgreement cli_agt;
				cli_agt.ClientID = id;
				cli_agt.BegDt  = p_rec->CaBegDt;
				cli_agt.Expiry = p_rec->CaExpiry;
				cli_agt.MaxCredit = p_rec->CaMaxCredit;
				cli_agt.MaxDscnt  = p_rec->CaMaxDscnt;
				cli_agt.Dscnt     = p_rec->CaDscnt;
				cli_agt.DefPayPeriod  = (int16)p_rec->CaDefPayPeriod;
				cli_agt.DefAgentID    = p_rec->CaDefAgentID;
				cli_agt.DefQuotKindID = p_rec->CaDefQuotKindID;
				pack.SetClientAgreement(&cli_agt, 1);
			}
			if(acs_rec.Flags & ACSHF_USESUPPLAGT || pack.P_SupplAgt) {
				PPSupplAgreement  suppl_agt;
				suppl_agt.SupplID = id;
				suppl_agt.BegDt  = p_rec->SaBegDt;
				suppl_agt.Expiry = p_rec->SaExpiry;
				suppl_agt.DefPayPeriod = (int16)p_rec->SaDefPayPeriod;
				suppl_agt.DefAgentID  = p_rec->SaDefAgentID;
				suppl_agt.DefDlvrTerm = (int16)p_rec->SaDefDlvrTerm;
				suppl_agt.PctRet = (int16)p_rec->SaPctRet;
				pack.SetSupplAgreement(&suppl_agt, 1);
			}
			THROW(p_obj->PutPacket(&id, &pack, 0));
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}
//
// } PPObjArticle
// PPObjCashNode {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjCashNode, DL6ICLS_PPObjCashNode_VTab, PPObjCashNode);
//
// Interface IPapyrusObject implementation
//
static void FillCashNodeRec(const PPCashNode * pInner, SPpyO_CashNode * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	pOuter->DeviceType = pInner->CashType;
	FLD(LocID);
	FLD(CurSessID);
	FLD(Flags);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

int32 DL6ICLS_PPObjCashNode::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjCashNode * p_obj = (PPObjCashNode *)ExtraPtr;
	if(p_obj) {
		PPCashNode cn_rec;
		MEMSZERO(cn_rec);
		ok = p_obj->Search(id, &cn_rec);
		FillCashNodeRec(&cn_rec, (SPpyO_CashNode *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjCashNode::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjCashNode * p_obj = (PPObjCashNode *)ExtraPtr;
	if(p_obj) {
		PPCashNode cn_rec;
		PPID   id = 0;
		MEMSZERO(cn_rec);
		ok = p_obj->SearchByName(text, &id, &cn_rec);
		FillCashNodeRec(&cn_rec, (SPpyO_CashNode *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjCashNode::GetName(int32 id)
{
	char   name_buf[64];
	PPObjCashNode * p_obj = (PPObjCashNode *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList * DL6ICLS_PPObjCashNode::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjCashNode::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjCashNode::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjCashNode
// PPObjQuotKind {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjQuotKind, DL6ICLS_PPObjQuotKind_VTab, PPObjQuotKind);
//
// Interface IPapyrusObject implementation
//
static void FillQuotKindRec(const PPQuotKind * pInner, SPpyO_QuotKind * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(Discount);
	pOuter->Period.Low = (OleDate)pInner->Period.low;
	pOuter->Period.Upp = (OleDate)pInner->Period.upp;
	FLD(BeginTm);
	FLD(EndTm);
	FLD(Rank);
	FLD(OpID);
	pOuter->Flags = (PpyOQuotKindFlags)pInner->Flags;
	FLD(AccSheetID);
	FLD(DaysOfWeek);
	pOuter->UsingWSCard = (PpyUsingWithSCard)pInner->UsingWSCard;
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
}

int32 DL6ICLS_PPObjQuotKind::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjQuotKind * p_obj = (PPObjQuotKind *)ExtraPtr;
	if(p_obj) {
		PPQuotKind qk_rec;
		MEMSZERO(qk_rec);
		ok = p_obj->Search(id, &qk_rec);
		FillQuotKindRec(&qk_rec, (SPpyO_QuotKind *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjQuotKind::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjQuotKind * p_obj = (PPObjQuotKind *)ExtraPtr;
	if(p_obj) {
		PPQuotKind qk_rec;
		PPID   id = 0;
		MEMSZERO(qk_rec);
		// @v9.0.8 ok = p_obj->SearchByName(text, &id, &qk_rec);
		// @v9.0.8 {
		if(kind == 1) {
			ok = p_obj->SearchBySymb(text, &id);
			if(ok > 0 && p_obj->Search(id, &qk_rec) > 0) {
				;
			}
			else
				ok = -1;
		}
		else { // (kind == 0)
			ok = p_obj->SearchByName(text, &id, &qk_rec);
		}
		// } @v9.0.8
		FillQuotKindRec(&qk_rec, (SPpyO_QuotKind *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjQuotKind::GetName(int32 id)
{
	char   name_buf[64];
	PPObjQuotKind * p_obj = (PPObjQuotKind *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList * DL6ICLS_PPObjQuotKind::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjQuotKind::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjQuotKind::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjQuotKind
// PPObjGoodsTax {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjGoodsTax, DL6ICLS_PPObjGoodsTax_VTab, PPObjGoodsTax);
//
// Interface IPapyrusObject implementation
//
static void FillGoodsTaxRec(const PPGoodsTax * pInner, SPpyO_GoodsTax * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(VAT);
	FLD(Excise);
	FLD(SalesTax);
	FLD(Flags);
	FLD(Order);
	FLD(UnionVect);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
}

int32 DL6ICLS_PPObjGoodsTax::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsTax * p_obj = (PPObjGoodsTax *)ExtraPtr;
	if(p_obj) {
		PPGoodsTax gt_rec;
		MEMSZERO(gt_rec);
		ok = p_obj->Search(id, &gt_rec);
		FillGoodsTaxRec(&gt_rec, (SPpyO_GoodsTax *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjGoodsTax::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsTax * p_obj = (PPObjGoodsTax *)ExtraPtr;
	if(p_obj) {
		PPGoodsTax gt_rec;
		PPID   id = 0;
		MEMSZERO(gt_rec);
		ok = p_obj->SearchByName(text, &id, &gt_rec);
		FillGoodsTaxRec(&gt_rec, (SPpyO_GoodsTax *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjGoodsTax::GetName(int32 id)
{
	char   name_buf[64];
	PPObjGoodsTax * p_obj = (PPObjGoodsTax *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList * DL6ICLS_PPObjGoodsTax::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjGoodsTax::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjGoodsTax::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// } PPObjGoodsTax
// PPObjStyloPalm  {
//
static void FillStyloPalmRec(const PPStyloPalmPacket * pInner, SPpyO_StyloPalm * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	// @v8.6.8 FLD(LocID);
	FLD(GoodsGrpID);
	FLD(AgentID);
	FLD(GroupID);
	FLD(OrderOpID);
	pOuter->Flags = (PpyOPalmFlags)pInner->Rec.Flags;
	FLD(FTPAcctID);
	#undef FLD
	pOuter->LocID = pInner->LocList.GetSingle(); // @v8.6.8
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->P_Path).CopyToOleStr(&pOuter->Path);
	(temp_buf = pInner->P_FTPPath).CopyToOleStr(&pOuter->FTPPath);
}

static void FillStyloPalmPack(const SPpyO_StyloPalm * pInner, PPStyloPalmPacket * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->Rec.f = pInner->f
	FLD(ID);
	// @v8.6.8 FLD(LocID);
	FLD(GoodsGrpID);
	FLD(AgentID);
	FLD(GroupID);
	FLD(Flags);
	FLD(FTPAcctID);
	FLD(OrderOpID);
	#undef FLD
	// @v8.6.8 {
	pOuter->LocList.Set(0);
	if(pInner->LocID)
		pOuter->LocList.Add(pInner->LocID);
	// } @v8.6.8
	pOuter->Rec.Tag = PPOBJ_STYLOPALM;
	temp_buf.CopyFromOleStr(pInner->Name).CopyTo(pOuter->Rec.Name, sizeof(pOuter->Rec.Name));
	temp_buf.CopyFromOleStr(pInner->Path);
	if(temp_buf.Len()) {
		pOuter->P_Path = new char[temp_buf.Len() + 1];
		temp_buf.CopyTo(pOuter->P_Path, temp_buf.Len());
	}
	temp_buf.CopyFromOleStr(pInner->FTPPath);
	if(temp_buf.Len()) {
		pOuter->P_FTPPath = new char[temp_buf.Len() + 1];
		temp_buf.CopyTo(pOuter->P_FTPPath, temp_buf.Len());
	}
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjStyloPalm, DL6ICLS_PPObjStyloPalm_VTab, PPObjStyloPalm);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjStyloPalm::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjStyloPalm * p_obj = (PPObjStyloPalm *)ExtraPtr;
	if(p_obj) {
		PPStyloPalmPacket pack;
		ok = p_obj->GetPacket(id, &pack);
		FillStyloPalmRec(&pack, (SPpyO_StyloPalm *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjStyloPalm::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjStyloPalm * p_obj = (PPObjStyloPalm *)ExtraPtr;
	if(p_obj) {
		PPStyloPalm stylo_rec;
		PPStyloPalmPacket pack;
		PPID   id = 0;
		MEMSZERO(stylo_rec);
		if((ok = p_obj->SearchByName(text, &id, &stylo_rec)) > 0)
			ok = p_obj->GetPacket(stylo_rec.ID, &pack);
		FillStyloPalmRec(&pack, (SPpyO_StyloPalm *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjStyloPalm::GetName(int32 id)
{
	char   name_buf[64];
	PPObjStyloPalm * p_obj = (PPObjStyloPalm *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjStyloPalm::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjStyloPalm::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = -1;
	PPID id = 0;
	SString temp_buf;
	SPpyO_StyloPalm * p_rec = (SPpyO_StyloPalm *)pRec;
	PPStyloPalmPacket pack;
	PPObjStyloPalm * p_obj = (PPObjStyloPalm *)ExtraPtr;

	THROW(p_obj);
	THROW_PP_S(p_rec->RecTag == ppoStyloPalm, PPERR_INVSTRUCTAG, "ppoStyloPalm");
	FillStyloPalmPack(p_rec, &pack);
	pack.Rec.ID = 0;
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	ASSIGN_PTR(pID, (int32)id);
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjStyloPalm::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int    ok = -1;
	SString temp_buf;
	SPpyO_StyloPalm * p_rec = (SPpyO_StyloPalm *)rec;
	PPStyloPalmPacket pack;
	PPObjStyloPalm * p_obj = (PPObjStyloPalm *)ExtraPtr;
	THROW(p_obj);
	THROW_PP_S(p_rec->RecTag == ppoStyloPalm, PPERR_INVSTRUCTAG, "ppoStyloPalm");
	FillStyloPalmPack(p_rec, &pack);
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}
//
// } PPObjStyloPalm
// PPObjCurrency {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjCurrency, DL6ICLS_PPObjCurrency_VTab, PPObjCurrency);
//
// Interface IPapyrusObject implementation
//
static void Copy_CurrencyRec(const PPCurrency * pInner, SPpyO_Currency * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = PPOBJ_CURRENCY;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(Flags);
	FLD(Code);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
}

static void Copy_CurrencyRec(const SPpyO_Currency * pOuter, PPCurrency * pInner)
{
	SString temp_buf;
	#define FLD(f) pInner->f = pOuter->f
	FLD(ID);
	FLD(Flags);
	FLD(Code);
	#undef FLD
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Name, sizeof(pInner->Name));
	temp_buf.CopyFromOleStr(pOuter->Symb).CopyTo(pInner->Symb, sizeof(pInner->Symb));
}

int32 DL6ICLS_PPObjCurrency::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjCurrency * p_obj = (PPObjCurrency *)ExtraPtr;
	if(p_obj) {
		PPCurrency pack;
		ok = p_obj->Fetch(id, &pack);
		Copy_CurrencyRec(&pack, (SPpyO_Currency *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjCurrency::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjCurrency * p_obj = (PPObjCurrency *)ExtraPtr;
	if(p_obj) {
		PPCurrency pack;
		PPID   id = 0;
		ok = p_obj->SearchByName(text, &id, &pack);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjCurrency::GetName(int32 id)
{
	PPObjCurrency * p_obj = (PPObjCurrency *)ExtraPtr;
	if(p_obj)
		p_obj->GetName(id, &RetStrBuf);
	else
		RetStrBuf = 0;
	return RetStrBuf;
}

IStrAssocList* DL6ICLS_PPObjCurrency::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjCurrency::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjCurrency::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	return FuncNotSupported();
}
//
// Interface IPapyrusObjCurrency implementation
//
int32 DL6ICLS_PPObjCurrency::GetConfig(PpyOCurrencyCfg * pCfg)
{
	if(pCfg) {
		pCfg->BaseCurID      = LConfig.BaseCurID;
		pCfg->BaseRateTypeID = LConfig.BaseRateTypeID;
	}
	return 1;
}

int32 DL6ICLS_PPObjCurrency::GetRate(int32 curID, double * pRate)
{
	int ok = -1;
	CurRateCore cr_core;
	LDATE dt;
	ok = cr_core.GetRate(curID, LConfig.BaseRateTypeID, LConfig.BaseCurID, &dt, pRate);
	SetAppError(ok);
	return ok;
}
// } PPObjCurrency
//
// PPObjGoodsClass {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjGoodsClass, DL6ICLS_PPObjGoodsClass_VTab, PPObjGoodsClass);
//
// Interface IPapyrusObject implementation
//
static void Copy_GdsClsProp(const PPGdsClsProp * pInner, SPpyO_GcProp * pOuter)
{
	SString temp_buf;
	pOuter->ItemsListID = pInner->ItemsListID;
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

static void Copy_GdsClsProp(const SPpyO_GcProp * pOuter, PPGdsClsProp * pInner)
{
	SString temp_buf;
	pInner->ItemsListID = pOuter->ItemsListID;
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Name, sizeof(pInner->Name));
}

static void Copy_GdsClsDim(const PPGdsClsDim * pInner, SPpyO_GcDim * pOuter)
{
	SString temp_buf;
	pOuter->Scale = pInner->Scale;
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

static void Copy_GdsClsDim(const SPpyO_GcDim * pOuter, PPGdsClsDim * pInner)
{
	SString temp_buf;
	pInner->Scale = pOuter->Scale;
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Name, sizeof(pInner->Name));
}

static void Copy_GoodsClassRec(const PPGdsClsPacket * pInner, SPpyO_GoodsClass * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = PPOBJ_GOODSCLASS;
	#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	FLD(DefGrpID);
	FLD(DefUnitID);
	FLD(DefPhUnitID);
	FLD(DefPhUPerU);
	FLD(DefTaxGrpID);
	FLD(DefGoodsTypeID);
	pOuter->Flags = (PpyOGoodsClassFlags)pInner->Rec.Flags;
	FLD(DynGenMask);
	#undef FLD
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->NameConv).CopyToOleStr(&pOuter->NameConv);
	(temp_buf = pInner->AbbrConv).CopyToOleStr(&pOuter->AbbrConv);
	(temp_buf = pInner->PhUPerU_Formula).CopyToOleStr(&pOuter->PhUPerU_Formula);
	(temp_buf = pInner->TaxMult_Formula).CopyToOleStr(&pOuter->TaxMult_Formula);
	(temp_buf = pInner->Package_Formula).CopyToOleStr(&pOuter->Package_Formula);
	Copy_GdsClsProp(&pInner->PropKind, &pOuter->PropKind);
	Copy_GdsClsProp(&pInner->PropGrade, &pOuter->PropGrade);
	Copy_GdsClsProp(&pInner->PropAdd, &pOuter->PropAdd);
	Copy_GdsClsProp(&pInner->PropAdd2, &pOuter->PropAdd2);
	Copy_GdsClsDim(&pInner->DimX, &pOuter->DimX);
	Copy_GdsClsDim(&pInner->DimY, &pOuter->DimY);
	Copy_GdsClsDim(&pInner->DimZ, &pOuter->DimZ);
	Copy_GdsClsDim(&pInner->DimW, &pOuter->DimW);
}

static void Copy_GoodsClassRec(const SPpyO_GoodsClass * pOuter, PPGdsClsPacket * pInner)
{
	SString temp_buf;
	#define FLD(f) pInner->Rec.f = pOuter->f
	FLD(ID);
	FLD(DefGrpID);
	FLD(DefUnitID);
	FLD(DefPhUnitID);
	FLD(DefPhUPerU);
	FLD(DefTaxGrpID);
	FLD(DefGoodsTypeID);
	pInner->Rec.Flags = pOuter->Flags;
	FLD(DynGenMask);
	#undef FLD
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Rec.Name, sizeof(pInner->Rec.Name));
	pInner->NameConv.CopyFromOleStr(pOuter->NameConv);
	pInner->AbbrConv.CopyFromOleStr(pOuter->AbbrConv);
	pInner->PhUPerU_Formula.CopyFromOleStr(pOuter->PhUPerU_Formula);
	pInner->TaxMult_Formula.CopyFromOleStr(pOuter->TaxMult_Formula);
	pInner->Package_Formula.CopyFromOleStr(pOuter->Package_Formula);
	Copy_GdsClsProp(&pOuter->PropKind,  &pInner->PropKind);
	Copy_GdsClsProp(&pOuter->PropGrade, &pInner->PropGrade);
	Copy_GdsClsProp(&pOuter->PropAdd,   &pInner->PropAdd);
	Copy_GdsClsProp(&pOuter->PropAdd2,  &pInner->PropAdd2);
	Copy_GdsClsDim(&pOuter->DimX, &pInner->DimX);
	Copy_GdsClsDim(&pOuter->DimY, &pInner->DimY);
	Copy_GdsClsDim(&pOuter->DimZ, &pInner->DimZ);
	Copy_GdsClsDim(&pOuter->DimW, &pInner->DimW);
}

int32 DL6ICLS_PPObjGoodsClass::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsClass * p_obj = (PPObjGoodsClass *)ExtraPtr;
	if(p_obj) {
		PPGdsClsPacket pack;
		ok = p_obj->Fetch(id, &pack);
		Copy_GoodsClassRec(&pack, (SPpyO_GoodsClass *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjGoodsClass::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsClass * p_obj = (PPObjGoodsClass *)ExtraPtr;
	if(p_obj) {
		PPGdsClsPacket pack;
		PPGdsCls gc_rec;
		PPID   id = 0;
		ok = p_obj->SearchByName(text, &id, &gc_rec);
		if(ok > 0)
			ok = p_obj->Fetch(id, &pack);
		Copy_GoodsClassRec(&pack, (SPpyO_GoodsClass *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjGoodsClass::GetName(int32 id)
{
	PPObjGoodsClass * p_obj = (PPObjGoodsClass *)ExtraPtr;
	if(p_obj)
		p_obj->GetName(id, &RetStrBuf);
	else
		RetStrBuf = 0;
	return RetStrBuf;
}

IStrAssocList* DL6ICLS_PPObjGoodsClass::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjGoodsClass::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = -1;
	PPID   id = 0;
	SString temp_buf;
	SPpyO_GoodsClass * p_rec = (SPpyO_GoodsClass *)pRec;
	PPGdsClsPacket pack;
	PPObjGoodsClass * p_obj = (PPObjGoodsClass *)ExtraPtr;
	THROW(p_obj);
	THROW_PP_S(p_rec->RecTag == ppoGoodsClass, PPERR_INVSTRUCTAG, "ppoGoodsClass");
	Copy_GoodsClassRec(p_rec, &pack);
	pack.Rec.ID = 0;
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	ASSIGN_PTR(pID, (int32)id);
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjGoodsClass::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	int    ok = -1;
	SString temp_buf;
	SPpyO_GoodsClass * p_rec = (SPpyO_GoodsClass *)pRec;
	PPGdsClsPacket pack;
	PPObjGoodsClass * p_obj = (PPObjGoodsClass *)ExtraPtr;
	THROW(p_obj);
	THROW_PP_S(p_rec->RecTag == ppoGoodsClass, PPERR_INVSTRUCTAG, "ppoGoodsClass");
	Copy_GoodsClassRec(p_rec, &pack);
	pack.Rec.ID = id;
	THROW(p_obj->PutPacket(&id, &pack, (flags & 0x0001) ? 0 : 1));
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}
// } PPObjGoodsClass
//
// PPObjGoods {
//
static void FillGoodsRec(PPGoodsPacket * pInner, SPpyO_Goods * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	FLD(Kind);
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Rec.Abbr).CopyToOleStr(&pOuter->Abbr);
	FLD(ParentID);
	FLD(GoodsTypeID);
	FLD(UnitID);
	FLD(PhUnitID);
	FLD(PhUPerU);
	FLD(ManufID);
	FLD(StrucID);
	FLD(TaxGrpID);
	FLD(WrOffGrpID);
	pOuter->Flags = (PpyOGoodsFlags)pInner->Rec.Flags;
	FLD(GdsClsID);
	FLD(BrandID);
	FLD(DefBCodeStrucID);
	#undef FLD
	#define FLD(f) pOuter->f = pInner->ExtRec.f
	//
	// Поля расширения товара
	//
	FLD(GoodsClsID);
	FLD(UniqCntr);
	if(pInner->Rec.GdsClsID) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(pInner->Rec.GdsClsID, &gc_pack) > 0) {
			gc_pack.GetExtDim(&pInner->ExtRec, PPGdsCls::eX, &pOuter->X);
			gc_pack.GetExtDim(&pInner->ExtRec, PPGdsCls::eY, &pOuter->Y);
			gc_pack.GetExtDim(&pInner->ExtRec, PPGdsCls::eZ, &pOuter->Z);
			gc_pack.GetExtDim(&pInner->ExtRec, PPGdsCls::eW, &pOuter->W);

			gc_pack.GetExtProp(&pInner->ExtRec, PPGdsCls::eKind, &pOuter->KindID, temp_buf);
			temp_buf.CopyToOleStr(&pOuter->KindText);
			gc_pack.GetExtProp(&pInner->ExtRec, PPGdsCls::eGrade, &pOuter->GradeID, temp_buf);
			temp_buf.CopyToOleStr(&pOuter->GradeText);
			gc_pack.GetExtProp(&pInner->ExtRec, PPGdsCls::eAdd, &pOuter->AddObjID, temp_buf);
			temp_buf.CopyToOleStr(&pOuter->AddObjText);
			gc_pack.GetExtProp(&pInner->ExtRec, PPGdsCls::eAdd2, &pOuter->AddObj2ID, temp_buf);
			temp_buf.CopyToOleStr(&pOuter->AddObj2Text);
		}
	}
	#undef FLD
	#define FLD(f) pOuter->f = pInner->Stock.f
	//
	// Размерности товара
	//
	FLD(Brutto);
	pOuter->Length = pInner->Stock.PckgDim.Length;
	pOuter->Width = pInner->Stock.PckgDim.Width;
	pOuter->Height = pInner->Stock.PckgDim.Height;
	pOuter->MinStock = pInner->Stock.GetMinStock(0);
	FLD(Package);
	FLD(MinShippmQtty);
	FLD(ExpiryPeriod);
	#undef FLD
	pOuter->GseFlags = (PpyGseFlags)pInner->Stock.GseFlags; // @v8.6.8
	//
	// Дополнительные поля //
	//
	pInner->GetExtStrData(GDSEXSTR_STORAGE, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Storage);
	pInner->GetExtStrData(GDSEXSTR_STANDARD, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Standard);
	pInner->GetExtStrData(GDSEXSTR_INGRED, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Ingred);
	pInner->GetExtStrData(GDSEXSTR_ENERGY, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Energy);
	pInner->GetExtStrData(GDSEXSTR_USAGE, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Usage);
	pInner->GetExtStrData(GDSEXSTR_LABELNAME, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->LabelName);
}

//
// Заполняем только те поля, которые нужны для формирования алкодекларации
//
static void FillGoodsRec_AlcRep(const Goods2Tbl::Rec * pRec, const PPGdsClsPacket * pGCPack, GoodsStockExt * pStock, const GoodsExtTbl::Rec * pExt, SPpyO_Goods * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pRec->f
	FLD(ID);
	FLD(Kind);
	(temp_buf = pRec->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pRec->Abbr).CopyToOleStr(&pOuter->Abbr);
	FLD(ParentID);
	FLD(GoodsTypeID);
	FLD(UnitID);
	FLD(PhUnitID);
	FLD(PhUPerU);
	FLD(ManufID);
	FLD(StrucID);
	FLD(TaxGrpID);
	FLD(WrOffGrpID);
	pOuter->Flags = (PpyOGoodsFlags)pRec->Flags;
	FLD(GdsClsID);
	FLD(BrandID);
	FLD(DefBCodeStrucID);
	#undef FLD
	#define FLD(f) pOuter->f = pExt->f
	//
	// Поля расширения товара
	//
	FLD(GoodsClsID);
	FLD(UniqCntr);
	if(pGCPack && pGCPack->Rec.ID) {
		pGCPack->GetExtDim(pExt, PPGdsCls::eX, &pOuter->X);
		pGCPack->GetExtDim(pExt, PPGdsCls::eY, &pOuter->Y);
		pGCPack->GetExtDim(pExt, PPGdsCls::eZ, &pOuter->Z);
		pGCPack->GetExtDim(pExt, PPGdsCls::eW, &pOuter->W);

		pGCPack->GetExtProp(pExt, PPGdsCls::eKind, &pOuter->KindID, temp_buf);
		temp_buf.CopyToOleStr(&pOuter->KindText);
		pGCPack->GetExtProp(pExt, PPGdsCls::eGrade, &pOuter->GradeID, temp_buf);
		temp_buf.CopyToOleStr(&pOuter->GradeText);
		pGCPack->GetExtProp(pExt, PPGdsCls::eAdd, &pOuter->AddObjID, temp_buf);
		temp_buf.CopyToOleStr(&pOuter->AddObjText);
		pGCPack->GetExtProp(pExt, PPGdsCls::eAdd2, &pOuter->AddObj2ID, temp_buf);
		temp_buf.CopyToOleStr(&pOuter->AddObj2Text);
	}
	#undef FLD
	#define FLD(f) pOuter->f = pStock->f
	//
	// Размерности товара
	//
	FLD(Brutto);
	pOuter->Length   = pStock->PckgDim.Length;
	pOuter->Width    = pStock->PckgDim.Width;
	pOuter->Height   = pStock->PckgDim.Height;
	pOuter->MinStock = pStock->GetMinStock(0);
	FLD(Package);
	FLD(MinShippmQtty);
	FLD(ExpiryPeriod);
	#undef FLD
	//
	// Дополнительные поля //
	//
	/*
	pInner->GetExtStrData(GDSEXSTR_STORAGE, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Storage);
	pInner->GetExtStrData(GDSEXSTR_STANDARD, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Standard);
	pInner->GetExtStrData(GDSEXSTR_INGRED, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Ingred);
	pInner->GetExtStrData(GDSEXSTR_ENERGY, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Energy);
	pInner->GetExtStrData(GDSEXSTR_USAGE, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Usage);
	pInner->GetExtStrData(GDSEXSTR_LABELNAME, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->LabelName);
	*/
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjGoods, DL6ICLS_PPObjGoods_VTab, PPObjGoods);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjGoods::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoods * p_obj = (PPObjGoods *)ExtraPtr;
	if(p_obj) {
		PPGoodsPacket pack;
		ok = p_obj->GetPacket(id, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
		FillGoodsRec(&pack, (SPpyO_Goods*)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjGoods::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoods * p_obj = (PPObjGoods *)ExtraPtr;
	if(p_obj) {
		Goods2Tbl::Rec grec;
		PPGoodsPacket pack;
		PPID   id = 0;
		MEMSZERO(grec);
		if((ok = p_obj->SearchByName(text, &id, &grec)) > 0) {
			ok = p_obj->GetPacket(grec.ID, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
			FillGoodsRec(&pack, (SPpyO_Goods*)rec);
		}
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjGoods::GetName(int32 id)
{
	char   name_buf[64];
	PPObjGoods * p_obj = (PPObjGoods *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjGoods::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjGoods::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjGoods::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int   ok = 1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		PPGoodsPacket pack;
		if(p_obj->GetPacket(id, &pack, PPObjGoods::gpoSkipQuot) > 0) {
			const SPpyO_Goods * p_src_rec = (const SPpyO_Goods *)rec;
			if(p_src_rec) {
				SString name_buf;
				THROW_PP_S(p_src_rec->RecTag == ppoGoods, PPERR_INVSTRUCTAG, "ppoGoodsGroup");
				if(p_src_rec->ParentID)
					pack.Rec.ParentID = p_src_rec->ParentID;
				name_buf.CopyFromOleStr(p_src_rec->Name);
				if(name_buf.NotEmptyS()) {
					STRNSCPY(pack.Rec.Name, name_buf);
					STRNSCPY(pack.Rec.Abbr, name_buf);
				}
                ok = p_obj->PutPacket(&id, &pack, 1);
				THROW(ok);
			}
		}
	}
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}
//
// Interface IPapyrusObjGoods implementation
//
IStrAssocList* DL6ICLS_PPObjGoods::GetBarcodes(int32 goodsID)
{
	StrAssocArray * p_list = 0;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		PPGoodsPacket pack;
		if(p_obj->GetPacket(goodsID, &pack, PPObjGoods::gpoSkipQuot) > 0) { // @v8.3.7 PPObjGoods::gpoSkipQuot
			THROW_MEM(p_list = new StrAssocArray);
			for(uint i = 0; i < pack.Codes.getCount(); i++)
				p_list->Add(i + 1, (long)(pack.Codes.at(i).Qtty * 100), pack.Codes.at(i).Code);
		}
	}
	CATCH
	ENDCATCH
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}

IStrAssocList* DL6ICLS_PPObjGoods::GetQuotations(int32 goodsID)
{
	StrAssocArray * p_list = 0;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		PPQuotArray quots;
		if(p_obj->GetQuotList(goodsID, 0, quots) > 0 && quots.getCount()) {
			SString temp_buf;
			THROW_MEM(p_list = new StrAssocArray);
			for(uint i = 0; i < quots.getCount(); i++) {
				const PPQuot & r_q = quots.at(i);
				temp_buf.Z().Cat(r_q.Quot);
				p_list->Add(r_q.ID, temp_buf);
			}
		}
	}
	CATCH
	ENDCATCH
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}

int32 DL6ICLS_PPObjGoods::GetQuot(int32 goodsID, int32 quotKind, double * pQuot)
{
	int    ok = -1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		const QuotIdent qi(0, quotKind);
		ok = p_obj->GetQuotExt(goodsID, qi, pQuot, 1);
	}
	return ok;
}

int32 DL6ICLS_PPObjGoods::SearchByBarcode(SString & rBCode, SPpyO_Goods * pGRec, int32 adoptSearching)
{
	int    ok = -1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		BarcodeTbl::Rec code_rec;
		Goods2Tbl::Rec grec;
		MEMSZERO(grec);
		if((ok = p_obj->SearchByBarcode(rBCode, &code_rec, &grec, adoptSearching)) > 0) {
			PPGoodsPacket pack;
			ok = p_obj->GetPacket(grec.ID, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
			// @v8.9.7 {
			if(ok > 0) {
				ok = (code_rec.Qtty > 0) ? (int)(code_rec.Qtty * 1000.0) : 1;
			}
			// } @v8.9.7
			FillGoodsRec(&pack, pGRec);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjGoods::SearchByArCode(long arID, SString & rBCode, SPpyO_Goods * pGRec)
{
	int    ok = -1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		ArGoodsCodeTbl::Rec code_rec;
		Goods2Tbl::Rec grec;
		MEMSZERO(grec);
		if((ok = p_obj->P_Tbl->SearchByArCode(arID, rBCode, &code_rec, &grec)) > 0) {
			PPGoodsPacket pack;
			ok = p_obj->GetPacket(grec.ID, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
			// @v8.9.7 {
			if(ok > 0) {
                ok = (code_rec.Pack > 0) ? code_rec.Pack : 1;
			}
			// } @v8.9.7
			FillGoodsRec(&pack, pGRec);
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjGoods::GetSingleBarcode(int32 goodsID)
{
	RetStrBuf = 0;
	((PPObjGoods *)ExtraPtr)->GetSingleBarcode(goodsID, RetStrBuf);
	return RetStrBuf;
}

SString & DL6ICLS_PPObjGoods::GetArCode(int32 goodsID, int32 arID)
{
	RetStrBuf = 0;
	((PPObjGoods *)ExtraPtr)->P_Tbl->GetArCode(arID, goodsID, RetStrBuf, 0);
	return RetStrBuf;
}

SString & DL6ICLS_PPObjGoods::GetArCodeWQtty(int32 goodsID, int32 arID, int32 * pPack)
{
	RetStrBuf = 0;
	((PPObjGoods *)ExtraPtr)->P_Tbl->GetArCode(arID, goodsID, RetStrBuf, pPack);
	return RetStrBuf;
}

int32 DL6ICLS_PPObjGoods::SetArCode(int32 goodsID, int32 arID, SString & code)
{
	if(!((PPObjGoods *)ExtraPtr)->P_Tbl->SetArCode(goodsID, arID, code, 1))
		AppError = 1;
	return (AppError == 0);
}

int32 DL6ICLS_PPObjGoods::BelongToGroup(int32 goodsID, int32 grpID, int32 * pSubGrpID)
{
	return ((PPObjGoods *)ExtraPtr)->BelongToGroup(goodsID, grpID, pSubGrpID);
}

double DL6ICLS_PPObjGoods::CalcVatSum(int32 goodsID, double qtty, double price, int32 withoutVat, LDATE dt)
{
	double result = 0.0;
	PPObjGoods * p_obj = ((PPObjGoods *)ExtraPtr);
	Goods2Tbl::Rec goods_rec;
	if(p_obj && p_obj->Fetch(goodsID, &goods_rec) > 0) {
		p_obj->CalcCostVat(0, goods_rec.TaxGrpID, dt, qtty, price, &result, withoutVat, 0);
	}
	return result;
}

int32 DL6ICLS_PPObjGoods::GetClassPropSymb(int32 goodsID, PpyOGoodsClassProperty clsProp, SString * pSymb)
{
	int32 ok = -1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		PPGoodsPacket pack;
		if(p_obj->GetPacket(goodsID, &pack, PPObjGoods::gpoSkipQuot) > 0 && pack.Rec.GdsClsID) { // @v8.3.7 PPObjGoods::gpoSkipQuot
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(pack.Rec.GdsClsID, &gc_pack) > 0) {
				PPID id = 0, ot = 0;
				switch(clsProp) {
					case clspKind    : id = pack.ExtRec.KindID;    ot = gc_pack.PropKind.ItemsListID;   break;
					case clspGrade   : id = pack.ExtRec.GradeID;   ot = gc_pack.PropGrade.ItemsListID;  break;
					case clspAddObj  : id = pack.ExtRec.AddObjID;  ot = gc_pack.PropAdd.ItemsListID;    break;
					case clspAddObj2 : id = pack.ExtRec.AddObj2ID; ot = gc_pack.PropAdd2.ItemsListID;   break;
					default: break;
				}
				if(ot && id) {
					PPObjReference * p_obj = new PPObjReference(ot, 0);
					if(p_obj) {
						Reference2Tbl::Rec rec;
						if(p_obj->Search(id, &rec) > 0) {
							ASSIGN_PTR(pSymb, rec.Symb);
							ok = 1;
						}
						delete p_obj;
					}
					else
						ok = (0, PPSetError(PPERR_NOMEM));
				}
			}
		}
	}
	SetAppError(ok);
	return ok;
}

double DL6ICLS_PPObjGoods::GetRest(int32 goodsID, int32 locID, LDATE dt, long supplID)
{
	double rest = 0.0;
	GoodsRestParam gp;
	gp.GoodsID     = goodsID;
	gp.CalcMethod  = GoodsRestParam::pcmSum;
	gp.Date        = dt;
	gp.SupplID     = supplID;
	gp.LocID       = locID;
	BillObj->trfr->GetRest(&gp);
	rest = gp.Total.Rest;
	return rest;
}

int32 DL6ICLS_PPObjGoods::SearchQttyByBarcode(SString & rBCode, SPpyO_Goods * pGRec, double * pQtty, int32 adoptSearching)
{
	int    ok = -1;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		Goods2Tbl::Rec grec;
		MEMSZERO(grec);
		if((ok = p_obj->SearchByBarcode(rBCode, 0, &grec, adoptSearching)) > 0) {
			PPGoodsPacket pack;
			ok = p_obj->GetPacket(grec.ID, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
			FillGoodsRec(&pack, pGRec);
		}
		else {
			GoodsCodeSrchBlock blk;
			rBCode.CopyTo(blk.Code, sizeof(blk.Code));
			if((ok = p_obj->SearchByCodeExt(&blk)) > 0) {
				SString temp_buf;
				pGRec->ID = blk.Rec.ID;
				(temp_buf = blk.Rec.Name).CopyToOleStr(&pGRec->Name);
				*pQtty = blk.Qtty;
			}
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjGoods::SetVad(int32 goodsID, SPpyO_Goods * pGRec)
{
	int    ok = 1;
	PPObjGoods * p_obj = (PPObjGoods *)ExtraPtr;
	if(p_obj) {
		GoodsStockExt gse;
		SString ext_string, temp_buf;
		SStringU temp_u_buf;
		Goods2Tbl::Rec goods_rec;
		PPTransaction tra(1);
		THROW(tra);
		THROW(p_obj->Search(goodsID, &goods_rec) > 0);
		THROW(p_obj->P_Tbl->GetStockExt(goodsID, &gse, 0));
		THROW(PPRef->GetPropVlrString(PPOBJ_GOODS, goodsID, GDSPRP_EXTSTRDATA, ext_string));

		//   Brutto, Length, Width, Height, MinStock, Package, ExpiryPeriod
		//   Storage, Standard, Ingred, Energy, Usage, LabelName

		gse.Brutto = pGRec->Brutto;
		gse.PckgDim.Width = pGRec->Width;
		gse.PckgDim.Length = pGRec->Length;
		gse.PckgDim.Height = pGRec->Height;
		gse.SetMinStock(0, pGRec->MinStock);
		gse.Package = pGRec->Package;
		gse.MinShippmQtty = pGRec->MinShippmQtty;
		gse.ExpiryPeriod = (int16)pGRec->ExpiryPeriod;
		//
		(temp_u_buf = pGRec->Storage).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_STORAGE, ext_string, temp_buf);
		//
		(temp_u_buf = pGRec->Standard).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_STANDARD, ext_string, temp_buf);
		//
		(temp_u_buf = pGRec->Ingred).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_INGRED, ext_string, temp_buf);
		//
		(temp_u_buf = pGRec->Energy).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_ENERGY, ext_string, temp_buf);
		//
		(temp_u_buf = pGRec->Usage).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_USAGE, ext_string, temp_buf);
		//
		(temp_u_buf = pGRec->LabelName).CopyToUtf8(temp_buf, 1);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		PPPutExtStrData(GDSEXSTR_LABELNAME, ext_string, temp_buf);
		//
		THROW(p_obj->P_Tbl->PutStockExt(goodsID, &gse, 0));
		THROW(PPRef->PutPropVlrString(PPOBJ_GOODS, goodsID, GDSPRP_EXTSTRDATA, ext_string));
		{
			long new_goods_flags = goods_rec.Flags;
			SETFLAG(new_goods_flags, GF_EXTPROP, ext_string.NotEmptyS());
			if(new_goods_flags != goods_rec.Flags) {
				PPID   key_id = goodsID;
				if(SearchByKey_ForUpdate(p_obj->P_Tbl, 0, &key_id, &goods_rec) > 0) {
					goods_rec.Flags = new_goods_flags;
					THROW_DB(p_obj->P_Tbl->updateRecBuf(&goods_rec)); // @sfu
					p_obj->Dirty(goods_rec.ID);
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

SString & DL6ICLS_PPObjGoods::ProcessName(SString & orgName, int32 flags)
{
	RetStrBuf = 0;
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj) {
		PPObjGoods::ProcessNameBlock blk;
		blk.OrgName = orgName;
		blk.Flags = flags;
		p_obj->ProcessName(blk, RetStrBuf);
	}
	return RetStrBuf;
}
//
// } PPObjGoods
// PPObjGoodsGroup {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjGoodsGroup, DL6ICLS_PPObjGoodsGroup_VTab, PPObjGoodsGroup);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjGoodsGroup::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup *)ExtraPtr;
	if(p_obj) {
		PPGoodsPacket pack;
		ok = p_obj->GetPacket(id, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
		FillGoodsRec(&pack, (SPpyO_Goods*)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjGoodsGroup::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup *)ExtraPtr;
	if(p_obj) {
		Goods2Tbl::Rec grec;
		PPGoodsPacket pack;
		PPID   id = 0;
		MEMSZERO(grec);
		if((ok = p_obj->SearchByName(text, &id, &grec)) > 0) {
			ok = p_obj->GetPacket(grec.ID, &pack, PPObjGoods::gpoSkipQuot); // @v8.3.7 PPObjGoods::gpoSkipQuot
			FillGoodsRec(&pack, (SPpyO_Goods*)rec);
		}
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjGoodsGroup::GetName(int32 id)
{
	char   name_buf[64];
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjGoodsGroup::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjGoodsGroup::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = -1;
	PPID   id = 0;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup *)ExtraPtr;
	SString name_buf;
	THROW(p_obj);
	SPpyO_Goods * p_rec = (SPpyO_Goods *)pRec;
	THROW_PP_S(p_rec->RecTag == ppoGoodsGroup, PPERR_INVSTRUCTAG, "ppoGoodsGroup");
	{
		Goods2Tbl::Rec ex_rec;
		name_buf.CopyFromOleStr(p_rec->Name);
		if(p_obj->SearchByName(name_buf, &id, &ex_rec) > 0) {
			ok = 2;
		}
		else {
			PPGoodsPacket pack;
			GoodsPacketKind grp_kind = gpkndOrdinaryGroup;
			if(p_rec->Flags & gofFOLDER)
				grp_kind = gpkndFolderGroup;
			THROW(p_obj->InitPacket(&pack, grp_kind, p_rec->ParentID, 0, 0));
			STRNSCPY(pack.Rec.Name, name_buf);
			pack.Rec.UnitID = p_rec->UnitID;
			THROW(p_obj->PutPacket(&id, &pack, 1));
			ok = 1;
		}
		ASSIGN_PTR(pID, id);
	}
	CATCH
		ok = 0;
		AppError = 1;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjGoodsGroup::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjGoodsGroup::SearchCode(SString & rCode)
{
	int32 ggrp_id = 0;
	BarcodeTbl::Rec bc_rec;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup *)ExtraPtr;
	MEMSZERO(bc_rec);
	if(p_obj && rCode.Len() && p_obj->SearchCode(rCode, &bc_rec) > 0)
		ggrp_id = bc_rec.GoodsID;
	return ggrp_id;
}
//
// Interface IPapyrusObjGoodsGroup implementation
//
IStrAssocList * DL6ICLS_PPObjGoodsGroup::GetGGroupsFromAltGrp(long altGrpID)
{
	StrAssocArray * p_list = 0;
	Goods2Tbl::Rec rec;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup*)ExtraPtr;
	if(p_obj && p_obj->Fetch(altGrpID, &rec) > 0) {
		PPIDArray grp_list;
		if((rec.Flags & GF_DYNAMICALTGRP) == GF_DYNAMICALTGRP) {
			PPGoodsPacket pack;
			if(p_obj->GetPacket(altGrpID, &pack, PPObjGoods::gpoSkipQuot) > 0) { // @v8.3.7 PPObjGoods::gpoSkipQuot
				if(pack.P_Filt) {
					if(pack.P_Filt->GrpIDList.IsExists())
						pack.P_Filt->GrpIDList.CopyTo(&grp_list);
					else if(pack.P_Filt->GrpID)
						grp_list.add(pack.P_Filt->GrpID);
				}
			}
		}
		// @v9.5.0 {
		else {
			p_obj->P_Tbl->GetGroupTerminalList(altGrpID, &grp_list, 0);
		}
		// } @v9.5.0
		if(grp_list.getCount()) {
			p_list = new StrAssocArray;
			if(p_list) {
				grp_list.sortAndUndup();
				for(uint i = 0; i < grp_list.getCount(); i++) {
					const PPID id = grp_list.at(i);
					if(p_obj->Fetch(id, &rec) > 0)
						p_list->AddFast(id, rec.Name);
				}
			}
		}
	}
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}

int32 DL6ICLS_PPObjGoodsGroup::IsDynamicAlt(long grpID)
{
	return PPObjGoodsGroup::IsDynamicAlt(grpID);
}

IStrAssocList* DL6ICLS_PPObjGoodsGroup::GetHierarchy(int32 grpID)
{
	StrAssocArray * p_list = 0;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup*)ExtraPtr;
	if(p_obj) {
		p_list = new StrAssocArray;
		p_obj->GetHierarchy(grpID, p_list);
	}
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}

IStrAssocList* DL6ICLS_PPObjGoodsGroup::GetChildList(int32 parentGrpID)
{
	StrAssocArray * p_list = 0;
	PPObjGoodsGroup * p_obj = (PPObjGoodsGroup*)ExtraPtr;
	if(p_obj && parentGrpID)
		p_list = p_obj->MakeStrAssocList((void *)parentGrpID);
	SETIFZ(p_list, new StrAssocArray);
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}
//
// } PPObjGoodsGroup
// PPObjLocAddrStruc {
//
DL6_IC_CONSTRUCTOR(PPLocAddrStruc, DL6ICLS_PPLocAddrStruc_VTab)
{
	ExtraPtr = new PPLocAddrStruc(PPLocAddrStruc::conditionalConstructWithFias);
}

DL6_IC_DESTRUCTOR(PPLocAddrStruc)
{
	delete ((PPLocAddrStruc *)ExtraPtr);
}
//
// Interface ILocAddrStruc implementation
//
int32 DL6ICLS_PPLocAddrStruc::Recognize(SString & addr)
{
	PPLocAddrStruc * p_las = (PPLocAddrStruc *)ExtraPtr;
	return p_las ? p_las->Recognize(addr.Transf(CTRANSF_INNER_TO_OUTER)) : ((AppError = 1), 0);
}

SString & DL6ICLS_PPLocAddrStruc::Get(PpyLocAddrPart partId)
{
	RetStrBuf = 0;
	PPLocAddrStruc * p_las = (PPLocAddrStruc *)ExtraPtr;
	if(p_las)
		p_las->Get(partId, RetStrBuf);
	else
		AppError = 1;
	return RetStrBuf.Transf(CTRANSF_OUTER_TO_INNER);
}
//
// } PPObjLocAddrStruc
// PPObjLocation {
//
static void FillLocationRec(const LocationTbl::Rec * pInner, SPpyO_Location * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(Counter);
	FLD(ParentID);
	FLD(Type);
	FLD(OwnerID);
	pOuter->Flags = (PpyOLocationFlags)pInner->Flags;
	FLD(CityID);
	FLD(RspnsPersonID);
	FLD(Latitude);
	FLD(Longitude);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	LocationCore::GetExField(pInner, LOCEXSTR_ZIP, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->ZIP);
	LocationCore::GetExField(pInner, LOCEXSTR_SHORTADDR, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->Address);
	LocationCore::GetExField(pInner, LOCEXSTR_FULLADDR, temp_buf);
	temp_buf.CopyToOleStr(&pOuter->FullAddr);
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjLocation, DL6ICLS_PPObjLocation_VTab, PPObjPerson);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjLocation::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	if(p_obj) {
		LocationTbl::Rec loc_rec;
		MEMSZERO(loc_rec);
		ok = p_obj->LocObj.Search(id, &loc_rec);
		if(ok > 0 && !loc_rec.OwnerID)
			p_obj->AdjustLocationOwner(loc_rec);
		FillLocationRec(&loc_rec, (SPpyO_Location *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjLocation::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	if(p_obj) {
		LocationTbl::Rec loc_rec;
		PPID   id = 0;
		MEMSZERO(loc_rec);
		if(kind == 0) {
			ok = p_obj->LocObj.SearchName(LOCTYP_WAREHOUSE, 0, text, &id);
			if(ok > 0)
				ok = p_obj->LocObj.Search(id, &rec);
		}
		else if(kind == 1)
			ok = p_obj->LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, text, &id, &loc_rec);
		if(ok > 0 && !loc_rec.OwnerID)
			p_obj->AdjustLocationOwner(loc_rec);
		FillLocationRec(&loc_rec, (SPpyO_Location*)rec);
	}
	if(!ok)
		AppError = 1;
	return ok;
}

SString & DL6ICLS_PPObjLocation::GetName(int32 id)
{
	char   name_buf[64];
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	int    ok = p_obj ? p_obj->LocObj.GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjLocation::GetSelector(int32 extraParam)
{
	// @v9.2.5 {
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	IStrAssocList * p = p_obj ? (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)&p_obj->LocObj, extraParam) : 0;
	// } @v9.2.5
	// @v9.2.5 IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjLocation::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjLocation::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// Interface IPapyrusObjLocation implementation
//
SString & DL6ICLS_PPObjLocation::GetAddress(int32 locID)
{
	//PPObjLocation * p_obj = (PPObjLocation *)ExtraPtr;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	RetStrBuf = 0;
	AppError = 0;
	if(p_obj) {
		/* @v9.5.5
		if(p_obj->LocObj.P_Tbl)
			p_obj->LocObj.P_Tbl->GetAddress(locID, 0, RetStrBuf);
		*/
		p_obj->LocObj.GetAddress(locID, 0, RetStrBuf); // @v9.5.5
	}
	return RetStrBuf;
}

int32 DL6ICLS_PPObjLocation::SearchByCode(SString & rCode, PpyOLocationType locType, SPpyO_Location * pRec)
{
	int32 ok = -1;
	//PPObjLocation * p_obj = (PPObjLocation*)ExtraPtr;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	if(p_obj) {
		LocationTbl::Rec loc_rec;
		MEMSZERO(loc_rec);
		if((ok = p_obj->LocObj.P_Tbl->SearchCode((int)locType, rCode, 0, &loc_rec)) > 0) {
			if(!loc_rec.OwnerID)
				p_obj->AdjustLocationOwner(loc_rec);
			FillLocationRec(&loc_rec, pRec);
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjLocation::GetDlvrAddrExtFld(int32 locID, int32 extFldID)
{
	LocationTbl::Rec loc_rec;
	//PPObjLocation * p_obj = (PPObjLocation *)ExtraPtr;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	RetStrBuf = 0;
	MEMSZERO(loc_rec);
	if(p_obj->LocObj.Search(locID, &loc_rec) > 0) {
		if(LocationCore::GetExField(&loc_rec, extFldID, RetStrBuf) == 0)
			AppError = 1;
	}
	else
		AppError = 1;
	return RetStrBuf;
}

int32 DL6ICLS_PPObjLocation::SetDlvrAddrExtFld(int32 locID, int32 extFldID, SString & rValue)
{
	int    ok = -1;
	LocationTbl::Rec loc_rec;
	//PPObjLocation * p_obj = (PPObjLocation *)ExtraPtr;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	MEMSZERO(loc_rec);
	if(p_obj->LocObj.Search(locID, &loc_rec) > 0)
		if((ok = LocationCore::SetExField(&loc_rec, extFldID, rValue)) > 0)
			ok = p_obj->LocObj.PutRecord(&loc_rec.ID, &loc_rec, 1);
	SetAppError(ok);
	return ok;
}

static void FASTCALL FillRegisterRec(const RegisterTbl::Rec * pInner, SPpyO_Register * pOuter, int special)
{
	SString temp_buf;
#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	if(pInner->ObjType == PPOBJ_PERSON) {
		pOuter->PsnID = pInner->ObjID;
		pOuter->LocID = 0;
	}
	else if(pInner->ObjType == PPOBJ_LOCATION) {
		pOuter->PsnID = 0;
		pOuter->LocID = pInner->ObjID;
	}
	else {
		pOuter->PsnID = 0;
		pOuter->LocID = 0;
	}
	FLD(PsnEventID);
	FLD(RegTypeID);
	pOuter->Dt = (OleDate)pInner->Dt;
	FLD(RegOrgID);
	pOuter->Expiry = (OleDate)pInner->Expiry;
	FLD(UniqCntr);
	FLD(Flags);
#undef FLD
	if(special)
		pOuter->SurID = pInner->ExtID;
	else
		pOuter->SurID = 0;
	(temp_buf = pInner->Serial).CopyToOleStr(&pOuter->Serial);
	(temp_buf = pInner->Num).CopyToOleStr(&pOuter->Number);
}

int32 DL6ICLS_PPObjLocation::GetRegisterD(int32 locID, int32 regType, LDATE actualDate, int32 inheritFromPerson, SPpyO_Register* pRec)
{
	int    ok = 0;
	RegisterTbl::Rec rec;
	MEMSZERO(rec);
	//PPObjLocation * p_obj = (PPObjLocation *)ExtraPtr;
	PPObjPerson * p_obj = (PPObjPerson *)ExtraPtr;
	ok = p_obj->LocObj.GetRegister(locID, regType, actualDate, inheritFromPerson, &rec);
	FillRegisterRec(&rec, (SPpyO_Register *)pRec, 0);
	return ok;
}
//
// } PPObjLocation
// PPObjPerson {
//
struct InnerExtraObjPerson {
	InnerExtraObjPerson()
	{
		P_Obj = 0;
		P_Pack = 0;
		Init();
	}
	~InnerExtraObjPerson()
	{
		ZDELETE(P_Pack);
		ZDELETE(P_Obj);
	}
	void Init()
	{
		ZDELETE(P_Obj);
		ZDELETE(P_Pack);
	}
	int EnumRel(PPID id, PPID relTypeID, int reverse, PPID * pRelPersonID);

	PPObjPerson * P_Obj;
	PPPersonPacket * P_Pack;
	//
	// Descr: Список отношений
	//
	struct RelEntry {
		RelEntry(PPID id, PPID relTypeID, int reverse)
		{
			ID = id;
			RelTypeID = relTypeID;
			Reverse = reverse;
		}
		PPID   ID;
		PPID   RelTypeID;
		int    Reverse;
		LAssocArray List;
	};
	TSCollection <RelEntry> RelList;
};

int InnerExtraObjPerson::EnumRel(PPID id, PPID relTypeID, int reverse, PPID * pRelPersonID)
{
	int    ok = -1;
	uint   i;
	PPID   rel_id = pRelPersonID ? *pRelPersonID : 0;
	for(i = 0; i < RelList.getCount(); i++) {
		RelEntry * p_entry = RelList.at(i);
		if(p_entry->ID == id && p_entry->RelTypeID == relTypeID && p_entry->Reverse == reverse) {
			if(rel_id == 0) {
				RelList.atFree(i);
			}
			else {
				for(uint j = 0; ok < 0 && j < p_entry->List.getCount(); j++) {
					if(p_entry->List.at(j).Key == rel_id) {
						if(j < (p_entry->List.getCount()-1)) {
							rel_id = p_entry->List.at(j+1).Key;
							ok = 1;
						}
						else {
							p_entry = 0;
							RelList.atFree(i);
							rel_id = 0;
							ok = 0;
						}
					}
				}
			}
			break;
		}
	}
	if(ok < 0 && rel_id == 0) {
		RelEntry * p_entry = new RelEntry(id, relTypeID, reverse);
		if(P_Obj->P_Tbl->GetRelList(id, &p_entry->List, reverse) > 0) {
			i = p_entry->List.getCount();
			if(i) {
				//
				// Удаляем из списка все нулевые идентификаторы и записи, не относящиеся //
				// к заданному типу персонального отношения.
				//
				do {
					--i;
					if(p_entry->List.at(i).Key == 0 || p_entry->List.at(i).Val != relTypeID)
						p_entry->List.atFree(i);
				} while(i);
				if(p_entry->List.getCount()) {
					RelList.insert(p_entry);
					rel_id = p_entry->List.at(0).Key;
					ok = 1;
				}
			}
		}
		if(ok <= 0)
			delete p_entry;
	}
	ASSIGN_PTR(pRelPersonID, rel_id);
	return ok;
}

DL6_IC_CONSTRUCTOR(PPObjPerson, DL6ICLS_PPObjPerson_VTab)
{
	InnerExtraObjPerson * p_e = new InnerExtraObjPerson;
	if(p_e)
		p_e->P_Obj = new PPObjPerson;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjPerson)
{
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	ZDELETE(p_e);
}

void FillPersonRec(const PPPersonPacket * pInner, SPpyO_Person * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	FLD(Status);
	FLD(MainLoc);
	FLD(Flags);
	FLD(RLoc);
	FLD(Division);
	FLD(Position);
	FLD(CatID);
	pOuter->UpdFlags = (PpyOPsnUpdateFlags)pInner->UpdFlags;
	#undef FLD
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Rec.Memo).CopyToOleStr(&pOuter->Memo);
	{
		pInner->GetExtName(temp_buf);
		temp_buf.CopyToOleStr(&pOuter->ExtString);
	}
}

//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjPerson::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		PPPersonPacket pack;
		if((ok = p_e->P_Obj->GetPacket(id, &pack, 0)) > 0) {
			FillPersonRec(&pack, (SPpyO_Person*)rec);
			ZDELETE(p_e->P_Pack);
			p_e->P_Pack = new PPPersonPacket;
			*(p_e->P_Pack) = pack;
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		PPID id = 0;
		if(p_e->P_Obj->P_Tbl->SearchByName(text, &id) > 0) {
			PPPersonPacket pack;
			if((ok = p_e->P_Obj->GetPacket(id, &pack, 0)) > 0) {
				FillPersonRec(&pack, (SPpyO_Person*)rec);
				ZDELETE(p_e->P_Pack);
				p_e->P_Pack = new PPPersonPacket;
				*(p_e->P_Pack) = pack;
			}
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjPerson::GetName(int32 id)
{
	int    ok = 0;
	char   name_buf[64];
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	memzero(name_buf, sizeof(name_buf));
	ok = (p_e && p_e->P_Obj) ? p_e->P_Obj->GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjPerson::GetSelector(int32 extraParam)
{
	AppError = 1;
	return 0;
}

int32 DL6ICLS_PPObjPerson::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPerson::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}

//
// Interface IPapyrusObjPerson implementation
//
int FillPacket(void * extraPtr, long personID, PPPersonPacket ** ppPack)
{
	int    ok = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)extraPtr;
	if(p_e) {
		if((!p_e->P_Pack || p_e->P_Pack->Rec.ID != personID) && p_e->P_Obj) {
			PPPersonPacket pack;
			if(p_e->P_Obj->GetPacket(personID, &pack, 0) > 0) {
				ZDELETE(p_e->P_Pack);
				p_e->P_Pack = new PPPersonPacket;
				*(p_e->P_Pack) = pack;
				if(*ppPack)
					*ppPack = p_e->P_Pack;
			}
		}
		if(p_e->P_Pack) {
			*ppPack = p_e->P_Pack;
			ok = 1;
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::GetLoc(int32 personID, int32 getRLoc, SPpyO_Location * pLoc)
{
	int    ok = 0;
	PPPersonPacket * p_pack = 0;
	if(pLoc && FillPacket(ExtraPtr, personID, &p_pack) > 0) {
		if(getRLoc) {
			FillLocationRec(&p_pack->RLoc, pLoc);
		}
		else {
			FillLocationRec(&p_pack->Loc, pLoc);
		}
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::GetPersonReq(int32 personID, SPersonReq * pReq)
{
	int    ok = 0;
	PersonReq req;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(pReq && p_e && p_e->P_Obj && (ok = p_e->P_Obj->GetPersonReq(personID, &req)) > 0) {
		SString temp_buf;
		pReq->Flags         = req.Flags;
		pReq->SrchRegTypeID = req.SrchRegTypeID;
		(temp_buf = req.Name).CopyToOleStr(&pReq->Name);
		(temp_buf = req.ExtName).CopyToOleStr(&pReq->ExtName);
		(temp_buf = req.Addr).CopyToOleStr(&pReq->Addr);
		(temp_buf = req.RAddr).CopyToOleStr(&pReq->RAddr);
		(temp_buf = req.Phone1).CopyToOleStr(&pReq->Phone1);
		(temp_buf = req.TPID).CopyToOleStr(&pReq->TPID);
		(temp_buf = req.KPP).CopyToOleStr(&pReq->KPP);
		(temp_buf = req.OKONF).CopyToOleStr(&pReq->OKONF);
		(temp_buf = req.OKPO).CopyToOleStr(&pReq->OKPO);
		(temp_buf = req.SrchCode).CopyToOleStr(&pReq->SrchCode);
		(temp_buf = req.Memo).CopyToOleStr(&pReq->Memo);
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::GetCashierInfo(int32 personID, SPpyO_CashierInfo * pCshrInfo)
{
	int    ok = 0;
	PPPersonPacket * p_pack = 0;
	if(pCshrInfo && FillPacket(ExtraPtr, personID, &p_pack) > 0) {
		SString temp_buf;
		(temp_buf = p_pack->CshrInfo.Password).CopyToOleStr(&pCshrInfo->Password);
		pCshrInfo->Rights = p_pack->CshrInfo.Rights;
		pCshrInfo->Flags = (PpyOCashierFlags)p_pack->CshrInfo.Flags;
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::EnumKinds(int32 personID, int32 * pIdx, int32* pKindID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPerson::EnumRelations(int32 personID, int32 relTypeID, int32 reverse, int32 * pRelPersonID)
{
	int    ok = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e) {
		if(p_e->EnumRel(personID, relTypeID, reverse, pRelPersonID) > 0)
			ok = 1;
		else
			ok = -1;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::EnumRegisters2(int32 personID, int32 regType, int32 * pIdx, SPpyO_Register * pRegRec)
{
	int    ok = 0;
	PPPersonPacket * p_pack = 0;
	if(pIdx && FillPacket(ExtraPtr, personID, &p_pack) > 0) {
		uint count = p_pack->Regs.getCount();
		uint idx = (uint)*pIdx;
		for(; ok != 1 && idx < count; idx++) {
			RegisterTbl::Rec & r_reg_rec = p_pack->Regs.at(idx);
			if(!regType || r_reg_rec.RegTypeID == regType) {
				FillRegisterRec(&r_reg_rec, pRegRec, 0);
				ok = 1;
			}
		}
		*pIdx = (int32)idx;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::EnumRegisters(int32 personID, int32 * pIdx, int32* pRegID)
{
	int    ok = 0;
	PPPersonPacket * p_pack = 0;
	if(pIdx && FillPacket(ExtraPtr, personID, &p_pack) > 0) {
		uint idx = (uint)*pIdx;
		if(idx < p_pack->Regs.getCount()) {
			ASSIGN_PTR(pRegID, p_pack->Regs.at(idx).ID);
			ok = 1;
		}
		(*pIdx)++;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::EnumDlvrLocs(int32 personID, int32 * pIdx, SPpyO_Location * pLoc)
{
	int    ok = 0;
	PPPersonPacket * p_pack = 0;
	if(pIdx && FillPacket(ExtraPtr, personID, &p_pack) > 0) {
		uint idx = (uint)*pIdx;
		PPLocationPacket loc_pack;
		if(p_pack->EnumDlvrLoc(&idx, &loc_pack) > 0) {
			if(pLoc)
				FillLocationRec(&loc_pack, pLoc);
			ok = 1;
		}
		*pIdx = (int32)idx;
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::EnumELink(int32 personID, int32 * pIdx, int32* pELinkID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPerson::EnumTags(int32 personID, int32 * pIdx, int32* pTagID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPerson::EnumBankAccts(int32 personID, int32 * pIdx, int32* pBnkAcctID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPerson::GetPersonByLocID(int32 locID, int32 personKindID)
{
	PPID   person_id = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		// @v7.4.0 {
		PPObjLocation * p_loc_obj = &p_e->P_Obj->LocObj;
		LocationTbl::Rec loc_rec;
		if(p_loc_obj->Search(locID, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
			person_id = loc_rec.OwnerID;
		}
		// @v7.4.0 {
		/* @v7.4.0
		PPIDArray psn_list;
		if(p_e->P_Obj->GetListByKind(personKindID, &psn_list, 0) > 0) {
			for(uint i = 0; !person_id && i < psn_list.getCount(); i++) {
				PPPersonPacket pack;
				if(p_e->P_Obj->GetPacket(psn_list.at(i), &pack, 0) > 0) {
					person_id = (pack.Loc.ID == locID || pack.RLoc.ID == locID) ? pack.Rec.ID : 0;
					if(person_id <= 0) {
						LocationTbl::Rec lrec;
						MEMSZERO(lrec);
						for(uint pos = 0; !person_id && pack.EnumDlvrLoc(&pos, &lrec) > 0;)
							person_id = (lrec.ID == locID) ? pack.Rec.ID : 0;
					}
				}
			}
		}
		*/
	}
	return person_id;
}

int32 DL6ICLS_PPObjPerson::IsBelongToKind(long personID, long kindID)
{
	int32  r = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj)
		r = p_e->P_Obj->P_Tbl->IsBelongToKind(personID, kindID);
	else
		AppError = 1;
	return r;
}

SString & DL6ICLS_PPObjPerson::FormatRegister(int32 personID, int32 regTypeID)
{
	char   temp_buf[256];
	temp_buf[0] = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj)
		p_e->P_Obj->FormatRegister(personID, regTypeID, temp_buf, sizeof(temp_buf));
	else
		AppError = 1;
	return (RetStrBuf = temp_buf);
}

SString & DL6ICLS_PPObjPerson::GetRegNumber(int32 personID, int32 regTypeID)
{
	RetStrBuf = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj)
		p_e->P_Obj->GetRegNumber(personID, regTypeID, RetStrBuf);
	else
		AppError = 1;
	return RetStrBuf;
}

SString & DL6ICLS_PPObjPerson::GetRegNumberD(int32 personID, LDATE actualDate, int32 regTypeID)
{
	RetStrBuf = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj)
		p_e->P_Obj->GetRegNumber(personID, regTypeID, actualDate, RetStrBuf);
	else
		AppError = 1;
	return RetStrBuf;
}

int32 DL6ICLS_PPObjPerson::IsTagAssigned(int32 personID, int32 tagID)
{
	int    ok = 0;
	ObjTagList list;
	if(PPRef->Ot.GetList(PPOBJ_PERSON, personID, &list)) {
		const ObjTagItem * p_item = list.GetItem(tagID);
		if(p_item)
			ok = 1;
	}
	return ok;
}

SString & DL6ICLS_PPObjPerson::FormatTag(int32 personID, int32 tagID)
{
	RetStrBuf = 0;
	ObjTagList list;
	if(PPRef->Ot.GetList(PPOBJ_PERSON, personID, &list)) {
		const ObjTagItem * p_item = list.GetItem(tagID);
		if(p_item) {
			/* @v8.2.5
			if(p_item->TagDataType == OTTYP_ENUM) {
				PPObjTagPacket pack;
				PPObjTag obj_tag;
				if(obj_tag.GetPacket(tagID, &pack) > 0)
					pack.EnumList.Get(p_item->Val.IntVal, RetStrBuf);
			}
			else */
				p_item->GetStr(RetStrBuf);
		}
	}
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjPerson::GetListByRegNumber(long regTypeID, long kindID, SString & rSerial, SString & rNumber)
{
	StrAssocArray * p_list = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		PPIDArray psn_list;
		if(p_e->P_Obj->GetListByRegNumber(regTypeID, kindID, (const char*)rSerial, (const char*)rNumber, psn_list) > 0) {
			THROW_MEM(p_list = new StrAssocArray);
			for(uint i = 0; i < psn_list.getCount(); i++)
				p_list->Add(psn_list.at(i), 0, rNumber);
		}
	}
	CATCH
	ENDCATCH
	return (IStrAssocList *)GetIStrAssocList(this, p_list);
}

int32 DL6ICLS_PPObjPerson::SetRegNumber(long psnID, long regTypeID, SString & rNumber)
{
	int32 ok = -1;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		PPPersonPacket pack;
		if(p_e->P_Obj->GetPacket(psnID, &pack, 0) > 0) {
			THROW(pack.AddRegister(regTypeID, rNumber));
			THROW(p_e->P_Obj->PutPacket(&psnID, &pack, 1));
			ok = 1;
		}
	}
	CATCH
		AppError = 1;
		ok = 0;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjPerson::SetTag(int32 psnID, int32 tagID, SString & rValue)
{
	int32 ok = -1;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		PPPersonPacket pack;
		if(psnID && tagID && p_e->P_Obj->GetPacket(psnID, &pack, 0) > 0) {
			ObjTagItem tag_item;
			tag_item.Init(tagID);
			if(tag_item.TagDataType == OTTYP_NUMBER)
				tag_item.SetInt(tagID, rValue.ToLong());
			else
				tag_item.SetStr(tagID, rValue);
			THROW(pack.TagL.PutItem(tagID, &tag_item));
			THROW(p_e->P_Obj->PutPacket(&psnID, &pack, 1));
			ok = 1;
		}
	}
	CATCH
		AppError = 1;
		ok = 0;
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPObjPerson::GetRegister(int32 psnID, int32 regType, SPpyO_Register * pRec)
{
	int32  ok = -1;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson *)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		RegisterTbl::Rec rec;
		MEMSZERO(rec);
		if(p_e->P_Obj->GetRegister(psnID, regType, &rec) > 0) {
			FillRegisterRec(&rec, (SPpyO_Register *)pRec, 0);
			ok = 1;
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::GetRegisterD(int32 psnID, int32 regType, LDATE actualDate, SPpyO_Register * pRec)
{
	int32  ok = -1;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson *)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		RegisterTbl::Rec rec;
		MEMSZERO(rec);
		if(p_e->P_Obj->GetRegister(psnID, regType, actualDate, &rec) > 0) {
			FillRegisterRec(&rec, (SPpyO_Register *)pRec, 0);
			ok = 1;
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjPerson::IsPrivate(int32 psnID)
{
	int32 r = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	if(p_e && p_e->P_Obj) {
		int is_private = 0;
		p_e->P_Obj->GetStatus(psnID, 0, &is_private);
		r = (int32)is_private;
	}
	return r;
}

IStrAssocList * DL6ICLS_PPObjPerson::GetRegList(int32 psnID, int32 regType)
{
	int32  r = 0;
	IUnknown * p = 0;
	StrAssocArray * p_reg_list = 0;
	InnerExtraObjPerson * p_e = (InnerExtraObjPerson*)ExtraPtr;
	THROW(CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p));
	THROW(p_reg_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p));
	if(p_e && p_e->P_Obj) {
		RegisterArray reg_list;
		if(p_e->P_Obj->GetRegList(psnID, &reg_list, 1) > 0) {
			SString temp_buf;
			uint count = reg_list.getCount();
			for(uint i = 0; i < count; i++) {
				const RegisterTbl::Rec & r_reg = reg_list.at(i);
				if(r_reg.RegTypeID == regType) {
					temp_buf.Z().Cat(r_reg.Dt).Cat("..").Cat(r_reg.Expiry);
					p_reg_list->Add(i + 1, 0, (const char*)temp_buf);
				}
			}
		}
	}
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (IStrAssocList*)p;
}
//
// } PPObjPerson
// PPObjBill {
//
struct InnerBillExtra {
	PPObjBill    * P_BObj;
	PPBillPacket * P_Pack;
};

static void FillBillRec(const PPBillPacket * pInner, SPpyO_Bill * pOuter)
{
	SString temp_buf;
	//
	// Основные поля документа
	//
#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	pOuter->Dt   = (OleDate)pInner->Rec.Dt;
	FLD(BillNo);
	pOuter->DueDate   = (OleDate)pInner->Rec.DueDate;
	FLD(OpID);
	FLD(StatusID);
	FLD(UserID);
	FLD(MainOrgID);
	FLD(LocID);
	FLD(Object);
	FLD(Object2);
	FLD(CurID);
	FLD(CRate);
	FLD(Amount);
	FLD(LinkBillID);
	pOuter->Flags = (PpyOBillFlags)pInner->Rec.Flags;
	FLD(Flags2);
	FLD(SCardID);
	(temp_buf = pInner->Rec.Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->Rec.Memo).CopyToOleStr(&pOuter->Memo);
#undef FLD
	//
	// Дополнительные поля документа
	//
#define FLD(f) pOuter->f = pInner->Ext.f
	FLD(PayerID);
	FLD(AgentID);
	pOuter->InvoiceDate   = (OleDate)pInner->Ext.InvoiceDate;
	pOuter->PaymBillDate   = (OleDate)pInner->Ext.PaymBillDate;
	FLD(Ft_STax);
	FLD(CreatorID);
	FLD(ExtPriceQuotKindID);
	(temp_buf = pInner->Ext.InvoiceCode).CopyToOleStr(&pOuter->InvoiceCode);
	(temp_buf = pInner->Ext.PaymBillCode).CopyToOleStr(&pOuter->PaymBillCode);
	pOuter->DlvrAddrID = pInner->P_Freight ? pInner->P_Freight->DlvrAddrID : 0;
#undef FLD
	//
	// Условия ренты
	//
#define FLD(f) pOuter->f = pInner->Rent.f
	pOuter->PeriodLow = (OleDate)pInner->Rent.Period.low;
	pOuter->PeriodUpp = (OleDate)pInner->Rent.Period.upp;
	FLD(Cycle);
	FLD(Percent);
	FLD(PartAmount);
	pOuter->RentFlags = (PpyORentFlags)pInner->Rent.Flags;
	FLD(ChargeDayOffs);
#undef FLD
	//
	// Остальные поля //
	//
#define FLD(f) pOuter->f = pInner->f
	FLD(OutAmtType);
	FLD(QuotKindID);
	pOuter->OprType = pInner->OpTypeID;
	pOuter->AccSheet = pInner->AccSheetID;
	FLD(Counter);
	FLD(PaymBillID);
	FLD(CSessID);
	FLD(SampleBillID);
#undef FLD
}

static void FillBillPacket(const SPpyO_Bill * pInner, PPBillPacket * pOuter, int fillNotZero = 0)
{
	SString temp_buf;
	// Основные поля документа
	#define FLD(f) (pInner->f || !fillNotZero) ? pOuter->Rec.f = pInner->f : pOuter->Rec.f = pOuter->Rec.f;
	FLD(ID);
	FLD(Dt);
	FLD(BillNo);
	FLD(DueDate);
	FLD(OpID);
	FLD(StatusID);
	FLD(UserID);
	FLD(MainOrgID);
	FLD(LocID);
	FLD(Object);
	FLD(Object2);
	FLD(CurID);
	FLD(CRate);
	FLD(Amount);
	FLD(LinkBillID);
	if(fillNotZero)
		pOuter->Rec.Flags |= (long)pInner->Flags;
	else
		pOuter->Rec.Flags = (long)pInner->Flags;
	FLD(Flags2);
	FLD(SCardID);
	temp_buf.CopyFromOleStr(pInner->Code);
	if(temp_buf.Len() || fillNotZero == 0)
		temp_buf.CopyTo(pOuter->Rec.Code, sizeof(pOuter->Rec.Code));
	temp_buf.CopyFromOleStr(pInner->Memo);
	if(temp_buf.Len() || fillNotZero == 0)
		temp_buf.CopyTo(pOuter->Rec.Memo, sizeof(pOuter->Rec.Memo));
	#undef FLD
	// Дополнительные поля документа
	#define FLD(f) pOuter->Ext.f = pInner->f;
	FLD(PayerID);
	FLD(AgentID);
	FLD(InvoiceDate);
	FLD(PaymBillDate);
	pOuter->Ext.Ft_STax = (int16)pInner->Ft_STax;
	FLD(CreatorID);
	FLD(ExtPriceQuotKindID);
	temp_buf.CopyFromOleStr(pInner->InvoiceCode).CopyTo(pOuter->Ext.InvoiceCode, sizeof(pOuter->Ext.InvoiceCode));
	temp_buf.CopyFromOleStr(pInner->PaymBillCode).CopyTo(pOuter->Ext.PaymBillCode, sizeof(pOuter->Ext.PaymBillCode));
	if(pInner->DlvrAddrID) {
		pOuter->P_Freight = new PPFreight;
		pOuter->P_Freight->DlvrAddrID = pInner->DlvrAddrID;
	}
	#undef FLD
	// Условия ренты
	#define FLD(f) pOuter->Rent.f = pInner->f
	pOuter->Rent.Period.low = pInner->PeriodLow;
	pOuter->Rent.Period.upp = pInner->PeriodUpp;
	pOuter->Rent.Cycle = (int16)pInner->Cycle;
	FLD(Percent);
	FLD(PartAmount);
	pOuter->Rent.Flags = (long)pInner->RentFlags;
	FLD(ChargeDayOffs);
	#undef FLD
	// Остальные поля //
	#define FLD(f) (pInner->f || !fillNotZero) ? pOuter->f = pInner->f : pOuter->f = pOuter->f;
	FLD(OutAmtType);
	FLD(QuotKindID);
	if(pInner->OprType || !fillNotZero)
		pOuter->OpTypeID = pInner->OprType;
	if(pInner->AccSheet || !fillNotZero)
		pOuter->AccSheetID = pInner->AccSheet;
	FLD(Counter);
	FLD(PaymBillID);
	FLD(CSessID);
	FLD(SampleBillID);
	#undef FLD
}

static void FillTrfrItemRec(const PPTransferItem * pInner, SPpyO_TrfrItem * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	pOuter->Date = (OleDate)pInner->Date;
	FLD(BillID);
	FLD(RByBill);
	FLD(CurID);
	FLD(LocID);
	FLD(GoodsID);
	FLD(LotID);
	FLD(OrdLotID);
	FLD(UnitPerPack);
	pOuter->Quantity = pInner->Quantity_;
	FLD(WtQtty);
	pOuter->Rest = pInner->Rest_;
	FLD(Cost);
	FLD(Price);
	FLD(Discount);
	FLD(CurPrice);
	FLD(QuotPrice);
	FLD(LotTaxGrpID);
	FLD(QCert);
	FLD(Suppl);
	pOuter->Amount = pInner->CalcAmount(0);
	pOuter->Flags  = (PpyOTrfrItemFlags)pInner->Flags;
	pOuter->Expiry = (OleDate)pInner->Expiry;
	pOuter->LotDate  = (OleDate)pInner->LotDate;
	#undef FLD
}

static void FillInnerTrfrItem(const SPpyO_TrfrItem * pInner, PPTransferItem * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(Date);
	FLD(BillID);
	pOuter->RByBill = (int16)pInner->RByBill;
	pOuter->CurID   = (int16)pInner->CurID;
	FLD(LocID);
	FLD(GoodsID);
	FLD(LotID);
	FLD(OrdLotID);
	FLD(UnitPerPack);
	pOuter->Quantity_ = pInner->Quantity;
	FLD(WtQtty);
	pOuter->Rest_ = pInner->Rest;
	FLD(Cost);
	FLD(Price);
	FLD(Discount);
	FLD(CurPrice);
	FLD(QuotPrice);
	FLD(LotTaxGrpID);
	FLD(QCert);
	FLD(Suppl);
	pOuter->Flags  = (long)pInner->Flags;
	FLD(Expiry);
	FLD(LotDate);
	#undef FLD
}

DL6_IC_CONSTRUCTOR(PPObjBill, DL6ICLS_PPObjBill_VTab)
{
	InnerBillExtra * p_e = new InnerBillExtra;
	if(p_e) {
		p_e->P_BObj = new PPObjBill;
		p_e->P_Pack = 0;
	}
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjBill)
{
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e) {
		ZDELETE(p_e->P_BObj);
		ZDELETE(p_e->P_Pack);
	}
	ZDELETE(p_e);
}
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjBill::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	InnerBillExtra * p_e = (InnerBillExtra *)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		PPBillPacket bpack;
		ok = p_e->P_BObj->ExtractPacket(id, &bpack);
		ok = (ok == 0 && DS.GetTLA().LastErr == PPERR_OBJNFOUND) ? -1 : ok;
		FillBillRec(&bpack, (SPpyO_Bill*)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjBill::GetName(int32 id)
{
	char   name_buf[64];
	InnerBillExtra * p_e = (InnerBillExtra *)ExtraPtr;
	int    ok = (p_e && p_e->P_BObj) ? p_e->P_BObj->GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjBill::GetSelector(int32 extraParam)
{
	AppError = 1;
	return 0;
}

int32 DL6ICLS_PPObjBill::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjBill::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}

DL6_IC_CONSTRUCTION_EXTRA(PPBillPacket, DL6ICLS_PPBillPacket_VTab, PPBillPacket);
//
// Interface IPapyrusBillPacket implementation
//
int32 DL6ICLS_PPBillPacket::Init()
{
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	return (p_pack) ? (p_pack->destroy(), 1) : 0;
}

int32 DL6ICLS_PPBillPacket::PutHeader(SPpyO_Bill * pHeader)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pHeader) {
		LDATE dt = ZERODATE;
		dt = pHeader->Dt;
		p_pack->destroy();
		p_pack->OutAmtType = 0;
		THROW(ok = p_pack->CreateBlank2(pHeader->OpID, dt, pHeader->LocID, 1));
		FillBillPacket(pHeader, p_pack, 1);
	}
	CATCHZOK
	return ok;
}

int32 DL6ICLS_PPBillPacket::UpdateHeader(SPpyO_Bill * pHeader)
{
	int ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pHeader) {
		FillBillPacket(pHeader, p_pack);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPBillPacket::GetHeader(SPpyO_Bill * pHeader)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pHeader) {
		FillBillRec(p_pack, pHeader);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPBillPacket::PutItem(SPpyO_TrfrItem * pItem)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pItem) {
		PPTransferItem ti;
		FillInnerTrfrItem(pItem, &ti);
		THROW(ti.Init(&p_pack->Rec));
		THROW(ok = p_pack->SetupRow(0, &ti, 0, 0));
		THROW(ok = p_pack->InsertRow(&ti, 0));
	}
	CATCH
		ok = RaiseAppError();
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPBillPacket::LoadTItem(SPpyO_TrfrItem * pItem, SString & rClb, SString & rBarcode)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pItem) {
		PPTransferItem ti;
		FillInnerTrfrItem(pItem, &ti);
		THROW(ti.Init(&p_pack->Rec));
		THROW(ok = p_pack->LoadTItem(&ti, rClb, rBarcode));
	}
	CATCH
		ok = RaiseAppError();
	ENDCATCH
	return ok;
}

int32 DL6ICLS_PPBillPacket::EnumItems(int32 * pIdx, SPpyO_TrfrItem * pItem)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack) {
		PPTransferItem ti;
		if(pIdx && *pIdx == 0) {
			long    flags = ETIEF_UNITEBYGOODS;
			SString temp_buf;
			if(DS.GetConstTLA().GetIfcConfigParam("ETIEF_DIFFBYLOT", temp_buf) > 0 && temp_buf == "1")
				flags |= ETIEF_DIFFBYLOT;
			p_pack->InitExtTIter(flags);
		}
		if((ok = p_pack->EnumTItemsExt(0, &ti)) > 0) {
			FillTrfrItemRec(&ti, pItem);
			if(pIdx)
				(*pIdx)++;
			ok = 1;
		}
	}
	return ok;
}

IStrAssocList * DL6ICLS_PPBillPacket::GetOrderList()
{
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	StrAssocArray assoc_list;
	if(p_pack) {
		PPIDArray list;
		p_pack->GetOrderList(list);
		for(uint i = 0; i < list.getCount(); i++) {
			assoc_list.Add(list.get(i), 0, 0);
		}
	}
	return (assoc_list.getCount()) ? (IStrAssocList*)GetIStrAssocList(this, &assoc_list, 0) : 0;
}

IPapyrusAmountList * DL6ICLS_PPBillPacket::GetAmountList()
{
	AmtList * p_list = 0;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack)
		p_list = &p_pack->Amounts;
	return (p_list && p_list->getCount()) ? (IPapyrusAmountList*)GetIPapyrusAmountList(this, p_list, 0) : 0;
}

int32 DL6ICLS_PPBillPacket::GetTaxInfo(SPpyO_TrfrItem * pItem, PpyOTrfrItemAmtType tiAmtType, SPpy_TaxInfo * pTaxInfo)
{
	int    ok = -1;
	SPpy_TaxInfo tax_info;
	PPTransferItem ti;
	GTaxVect vect(5); // @v8.7.0 default(2)-->5
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	MEMSZERO(tax_info);
	if(p_pack && pItem) {
		PPGoodsTaxEntry tax_entry;
		PPObjGoods gobj;
		FillInnerTrfrItem(pItem, &ti);
		vect.CalcTI(&ti, p_pack->Rec.OpID, (long)tiAmtType);
		gobj.FetchTax(ti.GoodsID, p_pack->Rec.Dt, p_pack->Rec.OpID, &tax_entry);
		tax_info.TaxGrpID  = (tax_entry.TaxGrpID & 0x00ffffff);
		tax_info.VatAmount = vect.GetValue(GTAXVF_VAT);
		tax_info.VatRate   = tax_entry.GetVatRate();
		ASSIGN_PTR(pTaxInfo, tax_info);
		ok = 1;
	}
	if(!ok)
		AppError = 1;
	return ok;
}

void FillFreightRec(const PPFreight * pInner, SPpyO_Freight * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(DlvrAddrID);
	FLD(NmbOrigsBsL);
	FLD(PortOfLoading);
	FLD(PortOfDischarge);
	FLD(CaptainID);
	FLD(Cost);
	FLD(AgentID);
	FLD(ShipID);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	pOuter->IssueDate   = (OleDate)pInner->IssueDate;
	pOuter->ArrivalDate = (OleDate)pInner->ArrivalDate;
	pOuter->TrType = (PpyOTransportType)pInner->TrType;
}

int32 DL6ICLS_PPBillPacket::GetFreight(SPpyO_Freight * pFreight)
{
	int ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && pFreight && p_pack->P_Freight) {
		FillFreightRec(p_pack->P_Freight, pFreight);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPBillPacket::GetTagValue(long tagID, SString * pValue)
{
	int    ok = -1;
	SString val;
	const ObjTagItem * p_tag_item = 0;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack && (p_tag_item = p_pack->BTagL.GetItem(tagID))) {
		p_tag_item->GetStr(val);
		ok = 1;
	}
	ASSIGN_PTR(pValue, val);
	return ok;
}

int32 DL6ICLS_PPBillPacket::PutTagValue(int32 tagID, SString & rValue)
{
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	return p_pack ? p_pack->BTagL.PutItemStr(tagID, rValue) : -1;
}

int32 DL6ICLS_PPBillPacket::PutRowTagValue(int32 tagID, int32 rowIdx, SString & rValue)
{
	int    ok = -1;
	SString val;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	if(p_pack) {
		ObjTagList tag_list;
		ObjTagList * p_prev_tag_list = p_pack->LTagL.Get(rowIdx);
		if(p_prev_tag_list)
			tag_list = *p_prev_tag_list;
		if(tag_list.PutItemStr(tagID, rValue) > 0)
			ok = p_pack->LTagL.Set(rowIdx, &tag_list);
	}
	return ok;
}

LDATE DL6ICLS_PPBillPacket::GetLastPayDate()
{
	LDATE dt = ZERODATE;
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	CALLPTRMEMB(p_pack, GetLastPayDate(&dt));
	return dt;
}
//
// CompleteList
//
IUnknown * GetICompleteList(SCoClass * pCls, CompleteArray * pList, int delList = 1)
{
	IUnknown * p = 0;
	if(pList && pCls && pCls->CreateInnerInstance("CompleteList", "ICompleteList", (void**)&p)) {
		CompleteArray * p_list_env = (CompleteArray*)SCoClass::GetExtraPtrByInterface(p);
		if(p_list_env) {
			*p_list_env = *pList;
			p_list_env->setPointer(0);
		}
		else
			ReleaseUnknObj(&p);
	}
	if(delList)
		ZDELETE(pList);
	return p;
}

DL6_IC_CONSTRUCTOR(CompleteList, DL6ICLS_CompleteList_VTab)
	{ ExtraPtr = new CompleteArray; }
DL6_IC_DESTRUCTOR(CompleteList)
	{ delete (CompleteArray *)ExtraPtr; }
//
// Interface ICompleteList implementation
//
void FillCompleteRec(const CompleteItem * pInner, SCompleteItem * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(GoodsID);
	FLD(LotID);
	FLD(BillID);
	FLD(Dt);
	FLD(Expiry);
	FLD(ArID);
	FLD(Qtty);
	FLD(Cost);
	FLD(Price);
	#undef FLD
	(temp_buf = pInner->Serial).CopyToOleStr(&pOuter->Serial);
	pOuter->Flags = (SCompleteItemFlags)pInner->Flags;
}

int32 DL6ICLS_CompleteList::GetCount()
{
	CompleteArray * p_data = (CompleteArray *)ExtraPtr;
	return p_data ? (int32)p_data->getCount() : RaiseAppError();
}

int32 DL6ICLS_CompleteList::Get(int32 pos, SCompleteItem * pItem)
{
	int ok = -1;
	CompleteArray * p_data = (CompleteArray*)ExtraPtr;
	if(p_data && pos < (long)p_data->getCount()) {
		CompleteItem item = p_data->at(pos);
		FillCompleteRec(&item, pItem);
		ok = 1;
	}
	SetAppError(ok);
	return ok;
}

void DL6ICLS_CompleteList::Clear()
{
	CompleteArray * p_data = (CompleteArray*)ExtraPtr;
	CALLPTRMEMB(p_data, freeAll());
}
//
// Implementation ILotList
//
void FillLotRec(const ReceiptTbl::Rec * pInner, SPpyO_Lot * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(BillID);
	FLD(LocID);
	FLD(OprNo);
	FLD(Closed);
	FLD(GoodsID);
	FLD(QCertID);
	FLD(UnitPerPack);
	FLD(Quantity);
	FLD(WtQtty);
	FLD(WtRest);
	FLD(Cost);
	FLD(ExtCost);
	FLD(Price);
	FLD(Rest);
	FLD(PrevLotID);
	FLD(SupplID);
	FLD(InTaxGrpID);
	#undef FLD
	pOuter->Dt = (OleDate)pInner->Dt;
	pOuter->CloseDate = (OleDate)pInner->CloseDate;
	pOuter->Expiry = (OleDate)pInner->Expiry;
	pOuter->Flags = (PpyOLotFlags)pInner->Flags;
}

void FillLotRec(const SPpyO_Lot * pInner, ReceiptTbl::Rec * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(BillID);
	FLD(LocID);
	FLD(OprNo);
	// @v8.1.2  FLD(Closed);
	pOuter->Closed = (int16)pInner->Closed; // @v8.1.2
	FLD(GoodsID);
	FLD(QCertID);
	FLD(UnitPerPack);
	FLD(Quantity);
	// @v8.1.2 FLD(WtQtty);
	pOuter->WtQtty = (float)pInner->WtQtty; // @v8.1.2
	// @v8.1.2 FLD(WtRest);
	pOuter->WtRest = (float)pInner->WtRest; // @v8.1.2
	FLD(Cost);
	FLD(ExtCost);
	FLD(Price);
	FLD(Rest);
	FLD(PrevLotID);
	FLD(SupplID);
	FLD(InTaxGrpID);
	FLD(Dt);
	FLD(CloseDate);
	FLD(Expiry);
	#undef FLD
	pOuter->Flags = (long)pInner->Flags;
}

DL6_IC_CONSTRUCTOR(PPLotList, DL6ICLS_PPLotList_VTab)
	{ ExtraPtr = new SArray(sizeof(ReceiptTbl::Rec), 128, O_ARRAY); }
DL6_IC_DESTRUCTOR(PPLotList)
	{ delete (SArray*)ExtraPtr; }
//
// Interface ILotList implementation
//
int32 DL6ICLS_PPLotList::GetCount()
{
	SArray * p_data = (SArray *)ExtraPtr;
	return p_data ? (int32)p_data->getCount() : RaiseAppError();
}

int32 DL6ICLS_PPLotList::Get(int32 pos, SPpyO_Lot * pLot)
{
	int    ok = 0;
	SArray * p_data = (SArray *)ExtraPtr;
	if(p_data) {
		if(((uint)pos) < p_data->getCount()) {
			FillLotRec((ReceiptTbl::Rec*)p_data->at((uint)pos), pLot);
			ok = 1;
		}
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPLotList::SearchById(int32 id, SPpyO_Lot * pLot, int32 bSearch)
{
	int    ok = 0;
	SArray * p_data = (SArray*)ExtraPtr;
	if(p_data) {
		uint pos = 0;
		ReceiptTbl::Rec lot;
		MEMSZERO(lot);
		if(bSearch)
			ok = p_data->bsearch(&id, &pos, PTR_CMPFUNC(long));
		else
			ok = p_data->lsearch(&id, &pos, PTR_CMPFUNC(long));
		if(ok > 0)
			FillLotRec((ReceiptTbl::Rec*)p_data->at((uint)pos), pLot);
		else
			memzero(pLot, sizeof(SPpyO_Lot));

	}
	else
		AppError = 1;
	return ok;
}

void DL6ICLS_PPLotList::InitIteration()
{
	SArray * p_data = (SArray*)ExtraPtr;
	if(p_data)
		p_data->setPointer(0);
	else
		AppError = 1;
}

int32 DL6ICLS_PPLotList::NextIteration(SPpyO_Lot * pLot)
{
	int    ok = 0;
	SArray * p_data = (SArray *)ExtraPtr;
	if(p_data) {
		uint p = p_data->getPointer();
		if(p < p_data->getCount()) {
			FillLotRec((ReceiptTbl::Rec*)p_data->at((uint)p), pLot);
			p_data->incPointer();
			ok = 1;
		}
		else
			memzero(pLot, sizeof(SPpyO_Lot));
	}
	else
		AppError = 1;
	return ok;
}

void DL6ICLS_PPLotList::Clear()
{
	SArray * p_data = (SArray *)ExtraPtr;
	if(p_data)
		p_data->freeAll();
	else
		AppError = 1;
}

void DL6ICLS_PPLotList::Add(SPpyO_Lot * pLot)
{
	ReceiptTbl::Rec lot;
	SArray * p_data = (SArray *)ExtraPtr;
	FillLotRec(pLot, &lot);
	if(!p_data || !p_data->insert(&lot))
		AppError = 1;
}

void DL6ICLS_PPLotList::Sort()
{
	SArray * p_data = (SArray*)ExtraPtr;
	if(p_data)
		p_data->sort(PTR_CMPFUNC(long));
	else
		AppError = 1;
}

void DL6ICLS_PPLotList::Clone(ILotList** ppClone)
{
	ILotList * p = 0;
	if(CreateInnerInstance(P_Scope->GetName(), "ILotList", (void **)&p)) {
		SArray * p_data = (SArray*)ExtraPtr;
		SArray * p_outer_data = (SArray*)SCoClass::GetExtraPtrByInterface(p);
		if(p_data && p_outer_data)
			*p_outer_data = *p_data;
		else
			AppError = 1;
	}
	else
		AppError = 1;
	ASSIGN_PTR(ppClone, p);
}
//
// Implementation IPapyrusObjBill interface
//
int32 DL6ICLS_PPObjBill::EnumBillRows(long billID, SPpyO_TrfrItem * pRow)
{
	int ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e->P_BObj) {
		int inited = 1;
		if(!p_e->P_Pack || billID != p_e->P_Pack->Rec.ID) {
			if(!p_e->P_Pack)
				p_e->P_Pack = new PPBillPacket;
			if(p_e->P_Pack && p_e->P_BObj->ExtractPacket(billID, p_e->P_Pack) > 0)
				p_e->P_Pack->InitExtTIter(ETIEF_UNITEBYGOODS);
			else
				inited = 0;
		}
		if(inited && p_e->P_Pack) {
			PPTransferItem ti;
			if((ok = p_e->P_Pack->EnumTItemsExt(0, &ti)) > 0)
				FillTrfrItemRec(&ti, pRow);
			else
				ZDELETE(p_e->P_Pack);
		}
	}
	return ok;
}

IPapyrusBillPacket* DL6ICLS_PPObjBill::CreatePacket()
{
	void * p_ifc = 0;
	THROW(IfcImpCheckDictionary());
	THROW(CreateInnerInstance("PPBillPacket", "IPapyrusBillPacket", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IPapyrusBillPacket*)p_ifc;
}

int32 DL6ICLS_PPObjBill::SearchAnalog(SPpyO_Bill * pSample, int32 * pID, SPpyO_Bill * pRec)
{
	int ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e->P_BObj) {
		PPID id = 0;
		PPBillPacket bpack;
		FillBillPacket(pSample, &bpack);
		if(p_e->P_BObj->P_Tbl->SearchAnalog(&bpack.Rec, &id, 0) > 0 && p_e->P_BObj->ExtractPacket(id, &bpack) > 0) {
			FillBillRec(&bpack, pRec);
			ASSIGN_PTR(pID, (int32)id);
			ok = 1;
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjBill::PutPacket(IPapyrusBillPacket * pPack)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket *)SCoClass::GetExtraPtrByInterface(pPack);
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_pack && p_e->P_BObj) {
		if(p_e->P_BObj->P_Tbl->SearchAnalog(&p_pack->Rec, 0, 0) > 0)
			ok = -2;
		else {
			p_pack->InitAmounts();
			ok = p_e->P_BObj->TurnPacket(p_pack, 1);
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetPacket(int32 id, IPapyrusBillPacket * pPack)
{
	int    ok = -1;
	PPBillPacket * p_pack = (PPBillPacket*)SCoClass::GetExtraPtrByInterface(pPack);
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_pack && p_e->P_BObj) {
		ok = p_e->P_BObj->ExtractPacket(id, p_pack);
		ok = (ok == 0 && DS.GetTLA().LastErr == PPERR_OBJNFOUND) ? -1 : ok; // @v8.6.10
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::CalcClientDebt(long clientID, SDateRange * pPeriod, SDebtBlock * pBlk)
{
	int    ok = -1;
	PPObjBill::DebtBlock blk;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e->P_BObj) {
		DateRange period;
		period.SetZero();
		if(pPeriod) {
			period.low = pPeriod->Low;
			period.upp = pPeriod->Upp;
		}
		ok = BillObj->CalcClientDebt(clientID, (pPeriod ? &period : 0), 0, blk);
#define FLD(f) pBlk->f = blk.f
		FLD(Amount);
		FLD(Debt);
		pBlk->HasMatured = (int)blk.HasMatured;
		pBlk->MaxDelay   = (int)blk.MaxDelay;
		pBlk->MaxExpiry  = (int)blk.MaxExpiry;

#undef FLD
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetFreight(int32 billID, SPpyO_Freight * pFreight)
{
	int    ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj && pFreight) {
		PPFreight freight;
		if(p_e->P_BObj->P_Tbl->GetFreight(billID, &freight) > 0) {
			FillFreightRec(&freight, pFreight);
			ok = 1;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetClbNumberByLot(int32 lotID, int32 * pIsParentLot, SString * pBuf)
{
	int ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		int is_parent_lot = 0;
		SString temp_buf;
		if(p_e->P_BObj->GetClbNumberByLot(lotID, &is_parent_lot, temp_buf) > 0) {
			ASSIGN_PTR(pIsParentLot, (int32)is_parent_lot);
			ASSIGN_PTR(pBuf, temp_buf);
			ok = 1;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::SearchByGuid(SString & rGuidStr, SPpyO_Bill * pRec)
{
	int    ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		S_GUID uuid;
		uuid.SetZero();
		if(uuid.FromStr(rGuidStr) > 0) {
			BillTbl::Rec bill_rec;
			PPBillPacket bpack;
			MEMSZERO(bill_rec);
			if(p_e->P_BObj->SearchByGuid(uuid, &bill_rec) > 0 && p_e->P_BObj->ExtractPacket(bill_rec.ID, &bpack) > 0) {
				FillBillRec(&bpack, pRec);
				ok = 1;
			}
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjBill::PutGuid(long billID, SString & rGuidStr)
{
	int    ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		S_GUID  uuid;
		uuid.SetZero();
		if(uuid.FromStr(rGuidStr) > 0) {
			BillTbl::Rec bill_rec;
			MEMSZERO(bill_rec);
			if((ok = p_e->P_BObj->SearchByGuid(uuid, &bill_rec)) <= 0) {
				if(ok < 0)
					ok = p_e->P_BObj->PutGuid(billID, &uuid, 0);
			}
			else
				ok = -2;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetGuid(long billID, SString * pGuidStr)
{
	int    ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		S_GUID  uuid;
		SString temp_buf;
		if(p_e->P_BObj->GetGuid(billID, &uuid) > 0) {
			uuid.ToStr(S_GUID::fmtIDL, temp_buf);
			ASSIGN_PTR(pGuidStr, (const char*)temp_buf);
			ok = 1;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetLotTagValue(int32 lotID, int32 tagID, SString * pValue)
{
	int ok = -1;
	SString val;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		ObjTagList list;
		if(p_e->P_BObj->GetTagListByLot(lotID, 0, &list) > 0) {
			const ObjTagItem * p_item = list.GetItem(tagID);
			if(p_item) {
				if(p_item->TagDataType != OTTYP_OBJLINK)
					ok = p_item->GetStr(val);
				else {
					long v = 0;
					ok = p_item->GetInt(&v);
					val.Cat(v);
				}
			}
		}
	}
	ASSIGN_PTR(pValue, val);
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBill::GetTagValue(int32 billID, int32 tagID, SString * pValue)
{
	int    ok = -1;
	SString val;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		PPBillPacket pack;
		if(p_e->P_BObj->ExtractPacket(billID, &pack) > 0) {
			const ObjTagItem * p_tag_item = 0;
			if(p_tag_item = pack.BTagL.GetItem(tagID)) {
				p_tag_item->GetStr(val);
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pValue, val);
	return ok;
}

int32 DL6ICLS_PPObjBill::PutTagValue(int32 billID, int32 tagID, SString & rValue)
{
	int ok = -1;
	SString val;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		// @todo Неэффективная реализация - можно просто изменить тег не гоняя весь документ из БД и обратно
		PPBillPacket pack;
		if(p_e->P_BObj->ExtractPacket(billID, &pack) > 0) {
			pack.BTagL.PutItemStr(tagID, rValue);
			ok = p_e->P_BObj->TurnPacket(&pack, 1);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjBill::GetOriginalLot(int32 lotID, int32 * pOrgLotID, SPpyO_Lot * pLot, SPpyO_Lot * pOriginalLot)
{
	int    ok = -1;
	PPID   org_lot_id = 0L;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		ReceiptTbl::Rec lot, org_lot;
		MEMSZERO(lot);
		MEMSZERO(org_lot);
		if(p_e->P_BObj->trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, &lot, &org_lot) > 0) {
			if(pLot)
				FillLotRec(&lot, pLot);
			if(pOriginalLot)
				FillLotRec(&org_lot, pOriginalLot);
			ok = 1;
		}
	}
	ASSIGN_PTR(pOrgLotID, org_lot_id);
	SetAppError(ok);
	return ok;
}

static BExtQuery & FASTCALL MakeLotSelectFldList(BExtQuery & rQ, const ReceiptTbl & rT)
{
	return rQ.select(rT.ID, rT.BillID, rT.Dt, rT.OprNo, rT.GoodsID, rT.LocID, rT.SupplID,
		rT.InTaxGrpID, rT.Flags, rT.UnitPerPack, rT.PrevLotID, rT.Cost, rT.Price,
		rT.Rest, rT.Closed, 0L);
}

struct LotQueryBlock {
	LotQueryBlock()
	{
		Idx = -1;
		SpMode = -1;
		Reverse = 0;
		P_Q = 0;
		memzero(Key, sizeof(Key));
	}
	~LotQueryBlock()
	{
		BExtQuery::ZDelete(&P_Q);
	}

	int   Idx;
	int   SpMode;
	int   Reverse;
	BExtQuery * P_Q;
	uint8 Key[ALIGNSIZE(MAXKEYLEN, 2)];
};

int SLAPI MakeLotQuery(ReceiptCore & rRcpt, LotQueryBlock & rBlk, int lcr, ulong lowId, ulong uppId, ObjIdListFilt & rLocList, LDATE dt)
{
	assert(!lcr || dt);
	int    ok = 1;
	uint   query_buf_capacity = 256;
	union {
		ReceiptTbl::Key0 k0;
		ReceiptTbl::Key1 k1;
		ReceiptTbl::Key2 k2;
		ReceiptTbl::Key3 k3;
		ReceiptTbl::Key5 k5;
		ReceiptTbl::Key7 k7;
	} kx;
	MEMSZERO(kx);
	DBQ  * dbq = 0;
	const  PPID single_loc_id = rLocList.GetSingle();
	if(lowId && uppId) {
		assert(lowId < uppId);
		rBlk.Idx = 0;
		rBlk.SpMode = spGe;
		kx.k0.ID = (long)lowId;
		dbq = &(rRcpt.ID >= (long)lowId && rRcpt.ID <= (long)uppId);
		query_buf_capacity = MIN(256, (uppId-lowId+1));
	}
	else {
		if(!lcr /*&& Filt.CalcMethod == GoodsRestParam::pcmLastLot*/)
			rBlk.Reverse = 1;
		if(!dt || lcr) {
			const int null_rest = BIN(!lcr/*&& Filt.Flags & GoodsRestFilt::fNullRest*/);
			if(single_loc_id) {
				rBlk.Idx = 7;
				kx.k7.LocID = single_loc_id;
				kx.k7.Closed = 0;
				dbq = & (rRcpt.LocID == single_loc_id);
				if(rBlk.Reverse) {
					kx.k7.Closed = null_rest;
					kx.k7.Dt    = MAXDATE;
					kx.k7.OprNo = MAXLONG;
					rBlk.SpMode = spLt;
				}
				else
					rBlk.SpMode = spGt;
			}
			else {
				rBlk.Idx = 3;
				if(rBlk.Reverse) {
					kx.k3.Closed  = null_rest;
					kx.k3.GoodsID = MAXLONG;
					kx.k3.LocID   = MAXLONG;
					kx.k3.Dt      = MAXDATE;
					kx.k3.OprNo   = MAXLONG;
					rBlk.SpMode = spLt;
				}
				else {
					if(0/*Filt.Flags & GoodsRestFilt::fCalcOrder*/)
						kx.k3.GoodsID = -MAXLONG;
					rBlk.SpMode = spGt;
				}
			}
			if(!null_rest)
				dbq = & (*dbq && rRcpt.Closed == 0L);
		}
		/*
		else if(Filt.SupplID && !(Filt.Flags & GoodsRestFilt::fWoSupplier)) {
			rBlk.Idx = 5;
			dbq = & (r_t->SupplID == Filt.SupplID);
			kx.k5.SupplID = Filt.SupplID;
			if(rBlk.Reverse) {
				kx.k5.Dt    = MAXDATE;
				kx.k5.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
		*/
		else if(single_loc_id) {
			rBlk.Idx = 7;
			kx.k7.LocID = single_loc_id;
			dbq = & (rRcpt.LocID == single_loc_id);
			if(rBlk.Reverse) {
				kx.k7.Closed = 1000;
				kx.k7.Dt    = MAXDATE;
				kx.k7.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
		else {
			rBlk.Idx = 1;
			if(rBlk.Reverse) {
				kx.k1.Dt    = MAXDATE;
				kx.k1.OprNo = MAXLONG;
				rBlk.SpMode = spLt;
			}
			else
				rBlk.SpMode = spGt;
		}
	}
	if(dt)
		dbq = & (*dbq && rRcpt.Dt <= dt);
	if(!oneof2(rBlk.Idx, 3, 7))
		dbq = ppcheckfiltidlist(dbq, rRcpt.LocID, &rLocList.Get());
	/*
	if((Filt.SupplID && rBlk.Idx != 5 && !(Filt.Flags & GoodsRestFilt::fWoSupplier))
		dbq = & (*dbq && r_t->SupplID == Filt.SupplID);
	*/
	if(1/*!(Filt.Flags & GoodsRestFilt::fCalcOrder)*/)
		dbq = & (*dbq && rRcpt.GoodsID > 0L);
	if(!lcr /*&& !(Filt.Flags & GoodsRestFilt::fNullRest)*/)
		if(dt)
			dbq = & (*dbq && rRcpt.CloseDate >= dt);
		else if(!oneof2(rBlk.Idx, 3, 7))
			dbq = & (*dbq && rRcpt.Closed == 0L);
	//
	// Гарантируем, что Idx и SpMode инициализированы
	//
	assert(rBlk.Idx != -1);
	assert(rBlk.SpMode != -1);
	//
	THROW_MEM(rBlk.P_Q = new BExtQuery(&rRcpt, rBlk.Idx, query_buf_capacity));
	MakeLotSelectFldList(*rBlk.P_Q, rRcpt).where(*dbq);
	CATCHZOK
	memcpy(rBlk.Key, &kx, sizeof(kx));
	return ok;
}

int SLAPI SelectLcrLots(ReceiptCore & rRcpt, const PPIDArray & rIdList, const UintHashTable & rLcrList, SArray & rList, ObjIdListFilt & rLocList, LDATE dt)
{
	int    ok = 1;
	const uint id_count = rIdList.getCount();
	if(id_count < 3) {
		for(uint i = 0; i < id_count; i++) {
			if(rRcpt.Search(rIdList.get(i), 0) > 0) {
				THROW_SL(rList.insert(&rRcpt.data));
			}
		}
	}
	else {
		LotQueryBlock q_blk;
		THROW(MakeLotQuery(rRcpt, q_blk, 0, rIdList.get(0), rIdList.getLast(), rLocList, dt));
		for(q_blk.P_Q->initIteration(q_blk.Reverse, q_blk.Key, q_blk.SpMode); q_blk.P_Q->nextIteration() > 0;) {
			if(rLcrList.Has(rRcpt.data.ID))
				THROW_SL(rList.insert(&rRcpt.data));
		}
	}
	CATCHZOK
	return ok;
}

ILotList * DL6ICLS_PPObjBill::GetCurLotList(LDATE lowDt, LDATE uppDt, int32 goodsGrpID, IStrAssocList * pLocList)
{
	int    ok = 1, r, use_lot_idx = 1;
	ReceiptCore rt;
	Transfer trfr;
	UintHashTable lcr_lot_list;
	RAssocArray lcr_rest_list;
	SString msg_buf;
	SArray lot_list(sizeof(ReceiptTbl::Rec));
	ObjIdListFilt loc_list;
	{
		StrAssocArray * p_loc_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(pLocList);
		if(p_loc_list) {
			uint loc_count = p_loc_list->getCount();
			for(uint i = 0; i < loc_count; i++)
				loc_list.Add(p_loc_list->at(i).Id);
			loc_list.Sort();
		}
	}

	if(CConfig.LcrUsage == 2 && (r = trfr.GetLcrList(lowDt, &lcr_lot_list, &lcr_rest_list)) > 0) {
		THROW(r);
		{
			const long max_hole = 5;
			PPIDArray id_list;
			for(ulong lot_id_ = 0, lot_id_prev = 0; lcr_lot_list.Enum(&lot_id_); lot_id_prev = lot_id_) {
				if((lot_id_ - lot_id_prev) > max_hole) {
					THROW(SelectLcrLots(rt, id_list, lcr_lot_list, lot_list, loc_list, lowDt));
					id_list.clear();
				}
				id_list.add((long)lot_id_);
			}
			THROW(SelectLcrLots(rt, id_list, lcr_lot_list, lot_list, loc_list, lowDt));
		}
		lot_list.sort(PTR_CMPFUNC(ReceiptTbl_DtOprNo));
		{
			const uint lc = lot_list.getCount();
			for(uint i = 0; i < lc; i++) {
				ReceiptTbl::Rec & r_lot_rec = *(ReceiptTbl::Rec *)lot_list.at(i);
				double rest = 0.0;
				r = lcr_rest_list.Search(r_lot_rec.ID, &rest, 0, 1);
				assert(r);
				if(r)
					r_lot_rec.Rest = rest;
				else
					r_lot_rec.Rest = 0.0; // @err Такого не должно быть.
			}
		}
		{
			//
			// Открытые лоты
			//
			LotQueryBlock q_blk;
			SString temp_buf;
			THROW(MakeLotQuery(rt, q_blk, 1, 0, 0, loc_list, lowDt));
			for(q_blk.P_Q->initIteration(q_blk.Reverse, q_blk.Key, q_blk.SpMode); q_blk.P_Q->nextIteration() > 0;) {
				if(!lcr_lot_list.Has((ulong)rt.data.ID))
					THROW_SL(lot_list.insert(&rt.data));
			}
		}
	}
	else {
		LotViewItem item;
		LotFilt   filt;
		PPViewLot v;

		use_lot_idx = 0;
		filt.Operation.Set(lowDt, uppDt);
    	filt.GoodsGrpID= goodsGrpID;
    	filt.Flags |= LotFilt::fSkipClosedBeforeOp;
		THROW(v.Init_(&filt));
		for(v.InitIteration(); v.NextIteration(&item) > 0;) {
			ReceiptTbl::Rec rec;
			memcpy(&rec, &item, sizeof(ReceiptTbl::Rec));
			rec.Rest = item.BegRest;
			lot_list.insert(&rec);
		}
	}
	{
		PPIDArray group_goods_list, * p_group_goods_list = 0;
		if(use_lot_idx) {
			GoodsFilt goods_flt;
			goods_flt.GrpID = goodsGrpID;
			THROW(GoodsIterator::GetListByFilt(&goods_flt, &group_goods_list));
			group_goods_list.sortAndUndup();
			p_group_goods_list = &group_goods_list;
		}
		uint lot_count = lot_list.getCount();
		for(long i = (long)lot_count - 1; i >= 0; i--) {
			int to_del = 0;
			ReceiptTbl::Rec * p_lot_rec = (ReceiptTbl::Rec*)lot_list.at(i);
			if(p_group_goods_list) {
				if(p_group_goods_list->bsearch(p_lot_rec->GoodsID) <= 0)
					to_del = 1;
			}
			if(!to_del && !loc_list.IsEmpty() && loc_list.CheckID(p_lot_rec->LocID) <= 0)
				to_del = 1;
			if(to_del)
				lot_list.atFree(i);
		}
	}
	CATCH
		AppError = 1;
	ENDCATCH
	return (ILotList*)GetILotList(this, &lot_list, 0);
}

int32 DL6ICLS_PPObjBill::GetLastLot(int32 goodsID, int32 locID, LDATE forDate, SPpyO_Lot* pLot)
{
	int    ok = -1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		ReceiptTbl::Rec lot_rec;
        ok = p_e->P_BObj->trfr->Rcpt.GetLastLot(goodsID, locID, forDate, &lot_rec);
		if(ok > 0) {
			FillLotRec(&lot_rec, pLot);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjBill::CalcGoodsRest(SPpyGoodsRestBlock* pBlk)
{
	int    ok = 1;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		GoodsRestParam gp;
		gp.CalcMethod = pBlk->CalcMethod;
		gp.Flags_ = pBlk->Flags;
        gp.DiffParam = GoodsRestParam::_diffNone;
        gp.Date = pBlk->Date;
        gp.OprNo = pBlk->OprNo;
        gp.LocID = pBlk->LocID;
        gp.GoodsID = pBlk->GoodsID;
        gp.SupplID = pBlk->SupplID;
        gp.AgentID = pBlk->AgentID;
		gp.Flags_ &= ~GoodsRestParam::fCostByQuot; // @v9.5.8
        if(pBlk->QuotKindID) {
			gp.Flags_ |= GoodsRestParam::fPriceByQuot;
			gp.QuotKindID = pBlk->QuotKindID;
        }
        else {
			gp.Flags_ &= ~GoodsRestParam::fPriceByQuot;
        }
        ok = p_e->P_BObj->trfr->GetRest(&gp);
		pBlk->Count = gp.Total.Count;
		pBlk->Rest = gp.Total.Rest;
		pBlk->Cost = gp.Total.Cost;
		pBlk->Price = gp.Total.Price;
		pBlk->LotID = gp.Total.LotID;
		pBlk->Deficit = gp.Total.Deficit;
		pBlk->DraftRcpt = gp.Total.DraftRcpt;
	}
	return ok;
}

double DL6ICLS_PPObjBill::GetRestByTag(LDATE dt, int32 goodsID, int32 tagID, SString & rTagVal, IStrAssocList * pLocList)
{
	double rest = 0.0;
	InnerBillExtra * p_e = (InnerBillExtra*)ExtraPtr;
	if(p_e && p_e->P_BObj) {
		GoodsRestParam gp;
		gp.DiffParam   |= GoodsRestParam::_diffLotTag;
		gp.Date         = dt;
		gp.GoodsID      = goodsID;
		gp.DiffLotTagID = tagID;
		if(pLocList) {
			StrAssocArray * p_loc_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(pLocList);
			if(p_loc_list) {
				uint count = p_loc_list->getCount();
				for(uint i = 0; i < count; i++)
					gp.LocList.add(p_loc_list->at(i).Id);
			}
		}
		if(p_e->P_BObj->trfr->GetRest(&gp) > 0) {
			uint count = gp.getCount();
			for(uint i = 0; rest == 0.0 && i < count; i++) {
				GoodsRestVal & r_grv = gp.at(i);
				if(rTagVal.Cmp(r_grv.LotTagText, 0) == 0)
					 rest = r_grv.Rest;
			}
		}
	}
	return rest;
}

ICompleteList * DL6ICLS_PPObjBill::GetComplete(int32 lotID)
{
	InnerBillExtra * p_ie = (InnerBillExtra*)ExtraPtr;
	CompleteArray list;
	if(p_ie && p_ie->P_BObj)
		p_ie->P_BObj->GetComplete(lotID, PPObjBill::gcfGatherSources, &list);
	return (ICompleteList*)GetICompleteList(this, &list, 0);
}

int32 DL6ICLS_PPObjBill::GetRest(int32 lotID, LDATE dt, double * pRest, double * pPhRest)
{
	int ok = -1;
	InnerBillExtra * p_ie = (InnerBillExtra*)ExtraPtr;
	if(p_ie && p_ie->P_BObj && p_ie->P_BObj->trfr)
		ok = p_ie->P_BObj->trfr->GetRest(lotID, dt, pRest, pPhRest);
	if(!ok)
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPObjBill::GetDebtDim(int32 billID, int32 * pDebtDimID)
{
	int       ok = -1;
	PPID      dd_id = 0;
	PPObjBill bill_obj;
	PPBillExt bill_ext;
	if(billID) {
		if(bill_obj.FetchExt(billID, &bill_ext) > 0) {
			if(bill_ext.AgentID) {
				LAssocArray debt_dim_agent_list;
				PPIDArray dim_list;
				PPObjDebtDim dd_obj;
				dd_obj.FetchAgentList(&debt_dim_agent_list);
				debt_dim_agent_list.GetListByVal(bill_ext.AgentID, dim_list);
				if(dim_list.getCount()) {
					dd_id = dim_list.get(0);
					ok = (dim_list.getCount() > 1) ? 2 : 1;
				}
			}
		}
	}
	ASSIGN_PTR(pDebtDimID, dd_id);
	return ok;
}

LDATE DL6ICLS_PPObjBill::GetLastPayDate(int32 billID)
{
	LDATE            dt = ZERODATE;
	PayPlanTbl       Pays;
	PayPlanTbl::Key0 k;
	k.BillID  = billID;
	k.PayDate = MAXDATE;
	if(Pays.search(0, &k, spLt) && k.BillID == billID)
		dt = (OleDate)Pays.data.PayDate;
	return dt;
}

int32 DL6ICLS_PPObjBill::CalcPayment(int32 billID, SDateRange * pPeriod, int32 curID, double * PaymentAmount)
{
	int    ok = 0;
	if(billID) {
		BillCore  bc;
		DateRange period;
		period.SetZero();
		if(pPeriod) {
			period.low = pPeriod->Low;
			period.upp = pPeriod->Upp;
		}
		ok = bc.CalcPayment(billID, 1, (pPeriod ? &period : 0), curID, PaymentAmount);
	}
	return ok;
}

IStrAssocList * DL6ICLS_PPObjBill::GetDeletedBillList(SDateRange * pPeriod)
{
	IUnknown * p = 0;
	THROW(CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p));
	StrAssocArray * p_bill_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p);
	THROW(p_bill_list);
	{
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		PPIDArray obj_list, act_list;
		DateRange period;
		period.SetZero();
		if(pPeriod) {
			period.low = pPeriod->Low;
			period.upp = pPeriod->Upp;
		}
		act_list.add(PPACN_RMVBILL);
		if(p_sj && p_sj->GetObjListByEventPeriod(PPOBJ_BILL, 0, &act_list, &period, obj_list) > 0) {
			uint count = obj_list.getCount();
			for(uint i = 0; i < count; i++)
				p_bill_list->Add(obj_list.get(i), 0, "");
		}
	}
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (IStrAssocList*)p;
}
//
// }
// PPObjWorld  {
//;
static void FillWorldRec(const WorldTbl::Rec * pInner, SPpyO_World * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	pOuter->Kind = (PpyOWorldKind)pInner->Kind;
	FLD(ParentID);
	FLD(CountryID);
	FLD(Status);
	FLD(Flags);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Abbr).CopyToOleStr(&pOuter->Abbr);
	(temp_buf = pInner->Phone).CopyToOleStr(&pOuter->Phone);
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->ZIP).CopyToOleStr(&pOuter->ZIP);
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjWorld, DL6ICLS_PPObjWorld_VTab, PPObjWorld);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjWorld::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjWorld * p_obj = (PPObjWorld *)ExtraPtr;
	if(p_obj) {
		WorldTbl::Rec wrec;
		MEMSZERO(wrec);
		ok = p_obj->Search(id, &wrec);
		FillWorldRec(&wrec, (SPpyO_World *)rec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjWorld::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjWorld * p_obj = (PPObjWorld *)ExtraPtr;
	if(p_obj) {
		WorldTbl::Rec wrec;
		PPID   id = 0;
		MEMSZERO(wrec);
		ok = p_obj->SearchByName(kind, text, &wrec);
		FillWorldRec(&wrec, (SPpyO_World *)rec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjWorld::GetName(int32 id)
{
	char   name_buf[64];
	PPObjWorld * p_obj = (PPObjWorld *)ExtraPtr;
	int    ok = p_obj->GetName(id, name_buf, sizeof(name_buf));
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjWorld::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjWorld::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjWorld::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
// } PPObjWorld
//
// PPObjRegister  {
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjRegister, DL6ICLS_PPObjRegister_VTab, PPObjRegister);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjRegister::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjRegister * p_obj = (PPObjRegister *)ExtraPtr;
	if(p_obj) {
		RegisterTbl::Rec reg_rec;
		MEMSZERO(reg_rec);
		ok = p_obj->Search(id, &reg_rec);
		FillRegisterRec(&reg_rec, (SPpyO_Register *)rec, 0);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjRegister::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjRegister::GetName(int32 id)
{
	AppError = 1;
	return (RetStrBuf = 0);
}

IStrAssocList* DL6ICLS_PPObjRegister::GetSelector(int32 extraParam)
{
	AppError = 1;
	return 0;
}

int32 DL6ICLS_PPObjRegister::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjRegister::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// IPapyrusObjRegister
//
int32 DL6ICLS_PPObjRegister::SearchByNumber(long regTypeID, SString & rSn, SString & rNmbr, SPpyO_Register * pRegister)
{
	int    ok = 0;
	PPObjRegister * p_obj = (PPObjRegister*)ExtraPtr;
	if(p_obj) {
		RegisterTbl::Rec reg_rec;
		MEMSZERO(reg_rec);
		ok = p_obj->SearchByNumber(0, regTypeID, rSn, rNmbr, &reg_rec);
		FillRegisterRec(&reg_rec, pRegister, 0);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjRegister::Fetch(int32 id, SPpyO_Register * pRegister)
{
	int    ok = 0;
	PPObjRegister * p_obj = (PPObjRegister *)ExtraPtr;
	if(p_obj) {
		RegisterTbl::Rec reg_rec;
		MEMSZERO(reg_rec);
		ok = p_obj->Fetch(id, &reg_rec);
		FillRegisterRec(&reg_rec, (SPpyO_Register *)pRegister, 0);
	}
	SetAppError(ok);
	return ok;
}
// } PPObjRegister
//
// PPObjPersonRelType  {
//
static void FillPersonRelTypeRec(const PPPersonRelType * pInner, SPpyO_PersonRelType * pOuter)
{
	if(pInner && pOuter) {
		SString temp_buf;
#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
#undef FLD
		pOuter->RecTag            = ppoPersonRelType;
		pOuter->StatusRestriction = (PpyOPersonRelTypeStatusRestr)pInner->StatusRestriction;
		pOuter->Cardinality       = (PpyOPersonRelTypeCardinality)pInner->Cardinality;
		pOuter->Flags             = (PpyOPersonRelTypeFlags)pInner->Flags;
		(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
		(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
	}
}

static void FillPersonRelTypeRec(const SPpyO_PersonRelType * pInner, PPPersonRelType * pOuter)
{
	SString temp_buf;
#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
#undef FLD
	pOuter->Tag               = PPOBJ_PERSONRELTYPE;
	pOuter->StatusRestriction = (int16)pInner->StatusRestriction;
	pOuter->Cardinality       = (int16)pInner->Cardinality;
	pOuter->Flags             = (long)pInner->Flags;
	temp_buf.CopyFromOleStr(pInner->Name).CopyTo(pOuter->Name, sizeof(pOuter->Name));
	temp_buf.CopyFromOleStr(pInner->Symb).CopyTo(pOuter->Symb, sizeof(pOuter->Symb));
}

DL6_IC_CONSTRUCTION_EXTRA(PPObjPersonRelType, DL6ICLS_PPObjPersonRelType_VTab, PPObjPersonRelType);
//
// Interface IPapyrusObject implementation
//
int32 DL6ICLS_PPObjPersonRelType::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	IMPL_PPIFC_EXTPTRVAR(PPObjPersonRelType);
	if(p_ext) {
		PPPersonRelType inner_rec;
		if(p_ext->Search(id, &inner_rec) > 0) {
			FillPersonRelTypeRec(&inner_rec, (SPpyO_PersonRelType *)rec);
			ok = 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int32 DL6ICLS_PPObjPersonRelType::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	IMPL_PPIFC_EXTPTRVAR(PPObjPersonRelType);
	if(p_ext) {
		PPPersonRelType inner_rec;
		PPID   id = 0;
		if(kind == 1) {
			if(p_ext->SearchSymb(&id, text) > 0 && p_ext->Search(id, &inner_rec) > 0) {
				FillPersonRelTypeRec(&inner_rec, (SPpyO_PersonRelType *)rec);
				ok = 1;
			}
			else
				ok = -1;
		}
		else {
			if(p_ext->SearchByName(text, &id, &inner_rec) > 0) {
				FillPersonRelTypeRec(&inner_rec, (SPpyO_PersonRelType *)rec);
				ok = 1;
			}
			else
				ok = 0;
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjPersonRelType::GetName(int32 id)
{
	int    ok = 0;
	RetStrBuf = 0;
	IMPL_PPIFC_EXTPTRVAR(PPObjPersonRelType);
	if(p_ext) {
		PPPersonRelTypePacket inner_pack;
		if(p_ext->Fetch(id, &inner_pack) > 0)
			RetStrBuf = inner_pack.Rec.Name;
		else if(id)
			ideqvalstr(id, RetStrBuf);
	}
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PPObjPersonRelType::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjPersonRelType::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjPersonRelType::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}

DL6_IC_CONSTRUCTION_EXTRA(PPPersonRelTypePacket, DL6ICLS_PPPersonRelTypePacket_VTab, PPPersonRelTypePacket);
//
// Interface IPapyrusPersonRelTypePacket implementation
//
int32 DL6ICLS_PPPersonRelTypePacket::Init()
{
	PPBillPacket * p_pack = (PPBillPacket*)ExtraPtr;
	return (p_pack) ? (p_pack->destroy(), 1) : 0;
}

int32 DL6ICLS_PPPersonRelTypePacket::Put(SPpyO_PersonRelType * pRec)
{
	int ok = -1;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket*)ExtraPtr;
	if(p_pack && pRec) {
		p_pack->Init();
		FillPersonRelTypeRec(pRec, &p_pack->Rec);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPPersonRelTypePacket::Get(SPpyO_PersonRelType * pRec)
{
	int ok = -1;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket*)ExtraPtr;
	if(p_pack && pRec) {
		FillPersonRelTypeRec(&p_pack->Rec, pRec);
		ok = 1;
	}
	return ok;
}

IStrAssocList * DL6ICLS_PPPersonRelTypePacket::GetInhRegTypeList()
{
	IStrAssocList * p_list = 0;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket*)ExtraPtr;
	StrAssocArray assoc_list;
	if(p_pack) {
		PPIDArray & list = p_pack->InhRegTypeList;
		for(uint i = 0; i < list.getCount(); i++)
			 assoc_list.Add(list.at(i), 0, 0);
	}
	return (assoc_list.getCount()) ? (IStrAssocList*)GetIStrAssocList(this, &assoc_list, 0) : 0;
}

int32 DL6ICLS_PPPersonRelTypePacket::PutInhRegTypeList(IStrAssocList * pList)
{
	int ok = -1;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket*)ExtraPtr;
	StrAssocArray         * p_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(pList);
	if(p_pack && pList) {
		p_pack->InhRegTypeList.freeAll();
		for(uint i = 0; i < p_list->getCount(); i++)
			p_pack->InhRegTypeList.add(p_list->at(i).Id);
	}
	return ok;
}
//
// IPapyrusObjPersonRelType
//
IPapyrusPersonRelTypePacket * DL6ICLS_PPObjPersonRelType::CreatePacket()
{
	void * p_ifc = 0;
	THROW(IfcImpCheckDictionary());
	THROW(CreateInnerInstance("PPPersonRelTypePacket", "IPapyrusPersonRelTypePacket", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (IPapyrusPersonRelTypePacket*)p_ifc;
}

int32 DL6ICLS_PPObjPersonRelType::PutPacket(long * pID, IPapyrusPersonRelTypePacket * pPack, int32 useTa)
{
	int ok = -1;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket *)SCoClass::GetExtraPtrByInterface(pPack);
	PPObjPersonRelType    * p_obj  = (PPObjPersonRelType*)ExtraPtr;
	if(p_pack && p_obj)
		ok = p_obj->PutPacket(pID, p_pack, 1);
	return ok;
}

int32 DL6ICLS_PPObjPersonRelType::GetPacket(int32 id, IPapyrusPersonRelTypePacket * pPack)
{
	int ok = -1;
	PPPersonRelTypePacket * p_pack = (PPPersonRelTypePacket*)SCoClass::GetExtraPtrByInterface(pPack);
	PPObjPersonRelType    * p_obj  = (PPObjPersonRelType*)ExtraPtr;
	if(p_pack && p_obj)
		ok = p_obj->GetPacket(id, p_pack);
	return ok;
}
// } PPObjPersonRelType
//
// PPObjPrjTask
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjPrjTask, DL6ICLS_PPObjPrjTask_VTab, PPObjPrjTask);
//
// Interface IPapyrusObject implementation
//
static void FillPrjTaskRec(const PrjTaskTbl::Rec * pInner, SPpyO_PrjTask * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(ProjectID);
		FLD(CreatorID);
		FLD(GroupID);
		FLD(EmployerID);
		FLD(ClientID);
		FLD(TemplateID);
		FLD(DrPrd);
		FLD(DrKind);
		FLD(DrDetail);
		FLD(DlvrAddrID);
		FLD(LinkTaskID);
		FLD(Amount);
		FLD(OpenCount);
		FLD(BillArID);
	#undef FLD
	pOuter->Priority    = (PpyOPrjTaskPriority)pInner->Priority;
	pOuter->Status      = (PpyOPrjTaskStatus)pInner->Status;
	pOuter->Flags       = (PpyOPrjTaskFlags)pInner->Flags;
	pOuter->Kind        = (PpyOPrjTaskKind)pInner->Kind;
	pOuter->Dt          = (OleDate)pInner->Dt;
	pOuter->Tm          = (OleDate)pInner->Tm;
	pOuter->StartDt     = (OleDate)pInner->StartDt;
	pOuter->StartTm     = (OleDate)pInner->StartTm;
	pOuter->EstFinishDt = (OleDate)pInner->EstFinishDt;
	pOuter->EstFinishTm = (OleDate)pInner->EstFinishTm;
	pOuter->FinishDt    = (OleDate)pInner->FinishDt;
	pOuter->FinishTm    = (OleDate)pInner->FinishTm;
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->Descr).CopyToOleStr(&pOuter->Descr);
	(temp_buf = pInner->Memo).CopyToOleStr(&pOuter->Memo);
}

void FillPrjTaskRec(const SPpyO_PrjTask * pInner, PrjTaskTbl::Rec * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(ProjectID);
		FLD(CreatorID);
		FLD(GroupID);
		FLD(EmployerID);
		FLD(ClientID);
		FLD(TemplateID);
		FLD(Priority);
		FLD(Status);
		FLD(DrDetail);
		FLD(DlvrAddrID);
		FLD(LinkTaskID);
		FLD(Amount);
		FLD(OpenCount);
		FLD(BillArID);
		FLD(Dt);
		FLD(Tm);
		FLD(StartDt);
		FLD(StartTm);
		FLD(EstFinishDt);
		FLD(EstFinishTm);
		FLD(FinishDt);
		FLD(FinishTm);
	#undef FLD
	pOuter->DrPrd  = (int16)pInner->DrPrd;
	pOuter->DrKind = (int16)pInner->DrKind;
	pOuter->Flags  = (long)pInner->Flags;
	pOuter->Kind   = (long)pInner->Kind;
	temp_buf.CopyFromOleStr(pInner->Code).CopyTo(pOuter->Code, sizeof(pOuter->Code));
	temp_buf.CopyFromOleStr(pInner->Descr).CopyTo(pOuter->Descr, sizeof(pOuter->Descr));
	temp_buf.CopyFromOleStr(pInner->Memo).CopyTo(pOuter->Memo, sizeof(pOuter->Memo));
}

int32 DL6ICLS_PPObjPrjTask::Search(int32 id, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjPrjTask * p_obj = (PPObjPrjTask *)ExtraPtr;
	if(p_obj) {
		PrjTaskTbl::Rec rec;
		MEMSZERO(rec);
		ok = p_obj->Search(id, &rec);
		FillPrjTaskRec(&rec, (SPpyO_PrjTask *)pRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjPrjTask::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjPrjTask::GetName(int32 id)
{
	AppError = 1;
	return (RetStrBuf = 0);
}

IStrAssocList* DL6ICLS_PPObjPrjTask::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjPrjTask::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 0;
	PPObjPrjTask * p_obj = (PPObjPrjTask *)ExtraPtr;
	SPpyO_PrjTask * p_rec = (SPpyO_PrjTask*)pRec;
	THROW_PP_S(p_rec->RecTag == ppoPrjTask, PPERR_INVSTRUCTAG, "ppoPrjTask");
	if(p_obj) {
		PrjTaskTbl::Rec init_rec, rec;
		PPTransaction tra((flags & 0x0001) ? 0 : 1);
		THROW(tra);
		FillPrjTaskRec(p_rec, &rec);
		THROW(p_obj->InitPacket(&init_rec, rec.Kind, rec.ProjectID, rec.ClientID, rec.EmployerID, 0));
		STRNSCPY(rec.Code, init_rec.Code);
		rec.Kind = init_rec.Kind;
		THROW(p_obj->PutPacket(pID, &rec, 0));
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjPrjTask::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjPrjTask * p_obj = (PPObjPrjTask *)ExtraPtr;
	SPpyO_PrjTask * p_rec = (SPpyO_PrjTask*)pRec;
	PrjTaskTbl::Rec rec;
	THROW_PP_S(p_rec->RecTag == ppoPrjTask, PPERR_INVSTRUCTAG, "ppoPrjTask");
	if(p_obj) {
		PPTransaction tra((flags & 0x0001) ? 0 : 1);
		THROW(tra);
		FillPrjTaskRec(p_rec, &rec);
		NZOR(rec.Kind, TODOKIND_TASK);
		THROW(p_obj->PutPacket(&id, &rec, 0));
		THROW(tra.Commit());
	}
	ok = 1;
	CATCHZOK
	SetAppError(ok);
	return ok;
}
// } PPObjPrjTask
//
//
// PPObjProject
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjProject, DL6ICLS_PPObjProject_VTab, PPObjProject);
//
// Interface IPapyrusObject implementation
//
static void FillProjectRec(const ProjectTbl::Rec * pInner, SPpyO_Project * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(ParentID);
		FLD(MngrID);
		FLD(ClientID);
		FLD(TemplateID);
		FLD(Flags);
		FLD(BillOpID);
	#undef FLD
	pOuter->Status      = (PpyOProjectStatus)pInner->Status;
	pOuter->Kind        = (PpyOProjectKind)pInner->Kind;
	pOuter->Dt          = (OleDate)pInner->Dt;
	pOuter->BeginDt     = (OleDate)pInner->BeginDt;
	pOuter->EstFinishDt = (OleDate)pInner->EstFinishDt;
	pOuter->FinishDt    = (OleDate)pInner->FinishDt;
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->Descr).CopyToOleStr(&pOuter->Descr);
	(temp_buf = pInner->Memo).CopyToOleStr(&pOuter->Memo);
}

void FillProjectRec(const SPpyO_Project * pInner, ProjectTbl::Rec * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(ParentID);
		FLD(MngrID);
		FLD(ClientID);
		FLD(TemplateID);
		FLD(Flags);
		FLD(BillOpID);
		FLD(Dt);
		FLD(BeginDt);
		FLD(EstFinishDt);
		FLD(FinishDt);
	#undef FLD
	pOuter->Status      = (long)pInner->Status;
	pOuter->Kind        = (long)pInner->Kind;
	temp_buf.CopyFromOleStr(pInner->Name).CopyTo(pOuter->Name, sizeof(pOuter->Name));
	temp_buf.CopyFromOleStr(pInner->Code).CopyTo(pOuter->Code, sizeof(pOuter->Code));
	temp_buf.CopyFromOleStr(pInner->Descr).CopyTo(pOuter->Descr, sizeof(pOuter->Descr));
	temp_buf.CopyFromOleStr(pInner->Memo).CopyTo(pOuter->Memo, sizeof(pOuter->Memo));
}

int32 DL6ICLS_PPObjProject::Search(int32 id, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjProject * p_obj = (PPObjProject *)ExtraPtr;
	if(p_obj) {
		ProjectTbl::Rec rec;
		MEMSZERO(rec);
		ok = p_obj->Search(id, &rec);
		FillProjectRec(&rec, (SPpyO_Project*)pRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjProject::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjProject::GetName(int32 id)
{
	int    ok = 0;
	PPObjProject * p_obj = (PPObjProject *)ExtraPtr;
	RetStrBuf = 0;
	if(p_obj)
		ok = p_obj->GetFullName(id, RetStrBuf);
	return RetStrBuf;
}

IStrAssocList* DL6ICLS_PPObjProject::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	if(!p)
		AppError = 1;
	return p;
}

int32 DL6ICLS_PPObjProject::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 0;
	PPObjProject * p_obj = (PPObjProject *)ExtraPtr;
	SPpyO_Project * p_rec = (SPpyO_Project*)pRec;
	THROW_PP_S(p_rec->RecTag == ppoProject, PPERR_INVSTRUCTAG, "ppoProject");
	if(p_obj) {
		ProjectTbl::Rec init_rec, rec;
		{
			PPTransaction tra((flags & 0x0001) ? 0 : 1);
			THROW(tra);
			FillProjectRec(p_rec, &rec);
			THROW(p_obj->InitPacket(&init_rec, rec.Kind, rec.ParentID, 0));
			STRNSCPY(rec.Code, init_rec.Code);
			rec.Kind = init_rec.Kind;
			rec.Dt = (rec.Dt == ZERODATE) ? init_rec.Dt : rec.Dt;
			NZOR(rec.Status, PPPRJSTS_ACTIVE);
			THROW(p_obj->PutPacket(pID, &rec, 0));
			THROW(tra.Commit());
		}
		ok = 1;
	}
	CATCHZOK
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjProject::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjProject * p_obj = (PPObjProject *)ExtraPtr;
	SPpyO_Project * p_rec = (SPpyO_Project*)pRec;
	ProjectTbl::Rec rec;
	THROW_PP_S(p_rec->RecTag == ppoProject, PPERR_INVSTRUCTAG, "ppoProject");
	if(p_obj) {
		PPTransaction tra((flags & 0x0001) ? 0 : 1);
		THROW(tra);
		FillProjectRec(p_rec, &rec);
		NZOR(rec.Kind, PPPRJK_PROJECT);
		THROW(p_obj->PutPacket(&id, &rec, 0));
		THROW(tra.Commit());
	}
	ok = 1;
	CATCHZOK
	SetAppError(ok);
	return ok;
}
// } PPObjPrjTask
//
// PPObjBrand
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjBrand, DL6ICLS_PPObjBrand_VTab, PPObjBrand);
//
// Interface IPapyrusObject implementation
//
static void FillBrand(const PPBrand * pInner, SPpyO_Brand * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(OwnerID);
	#undef FLD
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
}

void FillBrand(const SPpyO_Brand * pInner, PPBrand * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(OwnerID);
	#undef FLD
	temp_buf.CopyFromOleStr(pInner->Name).CopyTo(pOuter->Name, sizeof(pOuter->Name));
}

int32 DL6ICLS_PPObjBrand::Search(int32 id, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjBrand * p_obj = (PPObjBrand *)ExtraPtr;
	if(p_obj) {
		PPBrand rec;
		MEMSZERO(rec);
		ok = p_obj->Fetch(id, &rec);
		FillBrand(&rec, (SPpyO_Brand *)pRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjBrand::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjBrand::GetName(int32 id)
{
	AppError = 1;
	return (RetStrBuf = 0);
}

IStrAssocList* DL6ICLS_PPObjBrand::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjBrand::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjBrand::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	return FuncNotSupported();
}
// } PPObjBrand
//
// PPObjQCert
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjQCert, DL6ICLS_PPObjQCert_VTab, PPObjQCert);
//
// Interface IPapyrusObject implementation
//
static void FillQCert(const QualityCertTbl::Rec * pInner, SPpyO_QCert * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = ppoQCert;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(Passive);
		FLD(GoodsID);
		FLD(RegOrgan);
	#undef FLD
	pOuter->ProduceDate = (OleDate)pInner->ProduceDate;
	pOuter->InitDate    = (OleDate)pInner->InitDate;
	pOuter->Expiry      = (OleDate)pInner->Expiry;
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->BlankCode).CopyToOleStr(&pOuter->BlankCode);
	(temp_buf = pInner->GoodsName).CopyToOleStr(&pOuter->GoodsName);
	(temp_buf = pInner->Manuf).CopyToOleStr(&pOuter->Manuf);
	(temp_buf = pInner->SPrDate).CopyToOleStr(&pOuter->SPrDate);
	(temp_buf = pInner->Etc).CopyToOleStr(&pOuter->Etc);
	(temp_buf = pInner->InnerCode).CopyToOleStr(&pOuter->InnerCode);
}

void FillQCert(const SPpyO_QCert * pInner, QualityCertTbl::Rec * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
		FLD(ID);
		FLD(GoodsID);
		FLD(RegOrgan);
		FLD(ProduceDate);
		FLD(InitDate);
		FLD(Expiry);
	#undef FLD
	pOuter->Passive = (int16)pInner->Passive;
	temp_buf.CopyFromOleStr(pInner->Code).CopyTo(pOuter->Code, sizeof(pOuter->Code));
	temp_buf.CopyFromOleStr(pInner->BlankCode).CopyTo(pOuter->BlankCode, sizeof(pOuter->BlankCode));
	temp_buf.CopyFromOleStr(pInner->GoodsName).CopyTo(pOuter->GoodsName, sizeof(pOuter->GoodsName));
	temp_buf.CopyFromOleStr(pInner->Manuf).CopyTo(pOuter->Manuf, sizeof(pOuter->Manuf));
	temp_buf.CopyFromOleStr(pInner->SPrDate).CopyTo(pOuter->SPrDate, sizeof(pOuter->SPrDate));
	temp_buf.CopyFromOleStr(pInner->Etc).CopyTo(pOuter->Etc, sizeof(pOuter->Etc));
	temp_buf.CopyFromOleStr(pInner->InnerCode).CopyTo(pOuter->InnerCode, sizeof(pOuter->InnerCode));
}

int32 DL6ICLS_PPObjQCert::Search(int32 id, PPYOBJREC pRec)
{
	int    ok = 0;
	PPObjQCert * p_obj = (PPObjQCert*)ExtraPtr;
	if(p_obj) {
		QualityCertTbl::Rec rec;
		MEMSZERO(rec);
		ok = p_obj->Search(id, &rec);
		FillQCert(&rec, (SPpyO_QCert*)pRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjQCert::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjQCert::GetName(int32 id)
{
	AppError = 1;
	return (RetStrBuf = 0);
}

IStrAssocList* DL6ICLS_PPObjQCert::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjQCert::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjQCert::Update(int32 id, int32 flags, PPYOBJREC pRec)
{
	return FuncNotSupported();
}
// } PPObjQCert
//
// PPFiltCCheck {
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltCCheck, DL6ICLS_PPFiltCCheck_VTab, CCheckFilt);
//
// Interface IPpyFilt_CCheck implementation
//
void DL6ICLS_PPFiltCCheck::SetPeriod(LDATE low, LDATE upp)
	{ ((CCheckFilt *)ExtraPtr)->Period.Set(low, upp); }

int32  DL6ICLS_PPFiltCCheck::get_CashNodeID()
{
	return ((CCheckFilt *)ExtraPtr)->NodeList.GetSingle();
}
void   DL6ICLS_PPFiltCCheck::put_CashNodeID(int32 value)
{
	((CCheckFilt *)ExtraPtr)->NodeList.Add(value);
}
int32  DL6ICLS_PPFiltCCheck::get_CashNumber()             { IMPL_PPIFC_GETPROP(CCheckFilt, CashNumber); }
void   DL6ICLS_PPFiltCCheck::put_CashNumber(int32 value)  { IMPL_PPIFC_PUTPROP(CCheckFilt, CashNumber); }
int32  DL6ICLS_PPFiltCCheck::get_GoodsGrpID()             { IMPL_PPIFC_GETPROP(CCheckFilt, GoodsGrpID); }
void   DL6ICLS_PPFiltCCheck::put_GoodsGrpID(int32 value)  { IMPL_PPIFC_PUTPROP(CCheckFilt, GoodsGrpID); }
int32  DL6ICLS_PPFiltCCheck::get_GoodsID()                { IMPL_PPIFC_GETPROP(CCheckFilt, GoodsID); }
void   DL6ICLS_PPFiltCCheck::put_GoodsID(int32 value)     { IMPL_PPIFC_PUTPROP(CCheckFilt, GoodsID); }
int32  DL6ICLS_PPFiltCCheck::get_SCardSerID()             { IMPL_PPIFC_GETPROP(CCheckFilt, SCardSerID); }
void   DL6ICLS_PPFiltCCheck::put_SCardSerID(int32 value)  { IMPL_PPIFC_PUTPROP(CCheckFilt, SCardSerID); }
int32  DL6ICLS_PPFiltCCheck::get_SCardID()                { IMPL_PPIFC_GETPROP(CCheckFilt, SCardID); }
void   DL6ICLS_PPFiltCCheck::put_SCardID(int32 value)     { IMPL_PPIFC_PUTPROP(CCheckFilt, SCardID); }
int32  DL6ICLS_PPFiltCCheck::get_CashierID()              { IMPL_PPIFC_GETPROP(CCheckFilt, CashierID); }
void   DL6ICLS_PPFiltCCheck::put_CashierID(int32 value)   { IMPL_PPIFC_PUTPROP(CCheckFilt, CashierID); }
int32  DL6ICLS_PPFiltCCheck::get_AgentID()                { IMPL_PPIFC_GETPROP(CCheckFilt, AgentID); }
void   DL6ICLS_PPFiltCCheck::put_AgentID(int32 value)     { IMPL_PPIFC_PUTPROP(CCheckFilt, AgentID); }
int32  DL6ICLS_PPFiltCCheck::get_TableCode()              { IMPL_PPIFC_GETPROP(CCheckFilt, TableCode); }
void   DL6ICLS_PPFiltCCheck::put_TableCode(int32 value)    { IMPL_PPIFC_PUTPROP(CCheckFilt, TableCode); }
int32  DL6ICLS_PPFiltCCheck::get_MinCode()                 { IMPL_PPIFC_GETPROP(CCheckFilt, CodeR.low); }
void   DL6ICLS_PPFiltCCheck::put_MinCode(int32 value)      { IMPL_PPIFC_PUTPROP(CCheckFilt, CodeR.low); }
int32  DL6ICLS_PPFiltCCheck::get_MaxCode()                 { IMPL_PPIFC_GETPROP(CCheckFilt, CodeR.upp); }
void   DL6ICLS_PPFiltCCheck::put_MaxCode(int32 value)      { IMPL_PPIFC_PUTPROP(CCheckFilt, CodeR.upp); }
double DL6ICLS_PPFiltCCheck::get_MinAmount()               { IMPL_PPIFC_GETPROP(CCheckFilt, AmtR.low); }
void   DL6ICLS_PPFiltCCheck::put_MinAmount(double value)   { IMPL_PPIFC_PUTPROP(CCheckFilt, AmtR.low); }
double DL6ICLS_PPFiltCCheck::get_MaxAmount()               { IMPL_PPIFC_GETPROP(CCheckFilt, AmtR.upp); }
void   DL6ICLS_PPFiltCCheck::put_MaxAmount(double value)   { IMPL_PPIFC_PUTPROP(CCheckFilt, AmtR.upp); }
double DL6ICLS_PPFiltCCheck::get_MinQtty()                 { IMPL_PPIFC_GETPROP(CCheckFilt, QttyR.low); }
void   DL6ICLS_PPFiltCCheck::put_MinQtty(double value)     { IMPL_PPIFC_PUTPROP(CCheckFilt, QttyR.low); }
double DL6ICLS_PPFiltCCheck::get_MaxQtty()                 { IMPL_PPIFC_GETPROP(CCheckFilt, QttyR.upp); }
void   DL6ICLS_PPFiltCCheck::put_MaxQtty(double value)     { IMPL_PPIFC_PUTPROP(CCheckFilt, QttyR.upp); }
double DL6ICLS_PPFiltCCheck::get_AmountQuant()             { IMPL_PPIFC_GETPROP(CCheckFilt, AmountQuant); }
void   DL6ICLS_PPFiltCCheck::put_AmountQuant(double value) { IMPL_PPIFC_PUTPROP(CCheckFilt, AmountQuant); }

PpyVCCheckFlags DL6ICLS_PPFiltCCheck::get_Flags()       { return (PpyVCCheckFlags)((CCheckFilt *)ExtraPtr)->Flags; }
void  DL6ICLS_PPFiltCCheck::put_Flags(PpyVCCheckFlags value) { IMPL_PPIFC_PUTPROP(CCheckFilt, Flags); }
PpyVCCheckGrouping DL6ICLS_PPFiltCCheck::get_Grp()           { return (PpyVCCheckGrouping)((CCheckFilt *)ExtraPtr)->Grp; }
void DL6ICLS_PPFiltCCheck::put_Grp(PpyVCCheckGrouping value) { ((CCheckFilt *)ExtraPtr)->Grp = (CCheckFilt::Grouping)value; }
SDateRange DL6ICLS_PPFiltCCheck::get_Period()
	{ return DateRangeToOleDateRange(((CCheckFilt *)ExtraPtr)->Period); }

LTIME DL6ICLS_PPFiltCCheck::get_MinTime()                  { IMPL_PPIFC_GETPROP(CCheckFilt, TimePeriod.low); }
void  DL6ICLS_PPFiltCCheck::put_MinTime(LTIME value)       { IMPL_PPIFC_PUTPROP(CCheckFilt, TimePeriod.low); }
LTIME DL6ICLS_PPFiltCCheck::get_MaxTime()                  { IMPL_PPIFC_GETPROP(CCheckFilt, TimePeriod.upp); }
void  DL6ICLS_PPFiltCCheck::put_MaxTime(LTIME value)       { IMPL_PPIFC_PUTPROP(CCheckFilt, TimePeriod.upp); }
int32 DL6ICLS_PPFiltCCheck::get_WeekDays()                 { IMPL_PPIFC_GETPROP(CCheckFilt, WeekDays);   }
void DL6ICLS_PPFiltCCheck::put_WeekDays(int32 value)       { IMPL_PPIFC_PUTPROP_CAST(CCheckFilt, WeekDays, uint8); }
int32 DL6ICLS_PPFiltCCheck::get_HourBefore()               { IMPL_PPIFC_GETPROP(CCheckFilt, HourBefore); }
void DL6ICLS_PPFiltCCheck::put_HourBefore(int32 value)     { IMPL_PPIFC_PUTPROP_CAST(CCheckFilt, HourBefore, uint8); }
//
// } PPFileCCheck
// PPViewCCheck {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewCCheck, DL6ICLS_PPViewCCheck_VTab, PPViewCCheck);
//
// Interface IPapyrusView implementation
//
IUnknown * DL6ICLS_PPViewCCheck::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltCCheck", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewCCheck::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(CCheck);
}

int32 DL6ICLS_PPViewCCheck::InitIteration(int32 order)
{
	return ((PPViewCCheck*)ExtraPtr)->InitIteration(order);
}

int32 DL6ICLS_PPViewCCheck::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_CCheck * p_item = (SPpyVI_CCheck *)item;
	CCheckViewItem inner_item;
	if(((PPViewCCheck *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		LDATETIME dtm;
		p_item->RecTag     = PPVIEWITEM_CCHECK;
		p_item->ID         = inner_item.ID;
		p_item->Code       = inner_item.Code;
		p_item->CashID     = inner_item.CashID;
		p_item->UserID     = inner_item.UserID;
		p_item->SessID     = inner_item.SessID;
		p_item->Flags      = inner_item.Flags;
		p_item->SCardID    = inner_item.SCardID;
		dtm.Set(inner_item.Dt, inner_item.Tm);
		p_item->Dtm        = (OleDate)dtm;
		p_item->TableCode  = inner_item.TableCode;
		p_item->AgentID    = inner_item.AgentID;
		p_item->G_GoodsID  = inner_item.G_GoodsID;
		p_item->G_Count    = inner_item.G_Count;
		p_item->G_Amount   = inner_item.G_Amount;
		p_item->G_Discount = inner_item.G_Discount;
		p_item->G_PctPart  = inner_item.G_PctPart;
		p_item->G_Qtty     = inner_item.G_Qtty;
		p_item->CashNodeID = inner_item.CashNodeID;
		p_item->Amount     = MONEYTOLDBL(inner_item.Amount);
		p_item->Discount   = MONEYTOLDBL(inner_item.Discount);
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewCCheck::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewCCheck::GetTotal(PPYVIEWTOTAL total)
{
	PPViewCCheck * p_v = (PPViewCCheck *)ExtraPtr;
	if(p_v && total) {
		CCheckTotal inner_total;
		SPpyVT_CCheck * p_total = (SPpyVT_CCheck *)total;
		if(p_v->CalcTotal(&inner_total)) {
			p_total->Count    = inner_total.Count;
			p_total->Qtty     = inner_total.Qtty;
			p_total->Amount   = inner_total.Amount;
			p_total->Discount = inner_total.Discount;
		}
		else
			AppError = 1;
	}
	return !AppError;
}
//
// } PPViewCCheck
// PPFiltTrfrAnlz {
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltTrfrAnlz, DL6ICLS_PPFiltTrfrAnlz_VTab, TrfrAnlzFilt);
//
// Interface IPpyFilt_TrfrAnlz implementation
//
void DL6ICLS_PPFiltTrfrAnlz::SetPeriod(LDATE low, LDATE upp)
	{ ((TrfrAnlzFilt *)ExtraPtr)->Period.Set(low, upp); }
void DL6ICLS_PPFiltTrfrAnlz::SetLotsPeriod(LDATE low, LDATE upp)
	{ ((TrfrAnlzFilt *)ExtraPtr)->LotsPeriod.Set(low, upp); }
SDateRange DL6ICLS_PPFiltTrfrAnlz::get_Period()
	{ return DateRangeToOleDateRange(((TrfrAnlzFilt *)ExtraPtr)->Period); }
SDateRange DL6ICLS_PPFiltTrfrAnlz::get_LotsPeriod()
	{ return DateRangeToOleDateRange(((TrfrAnlzFilt *)ExtraPtr)->LotsPeriod); }
void  DL6ICLS_PPFiltTrfrAnlz::AddLocationID(int32 value)
	{ TrfrAnlzFilt * p_filt = (TrfrAnlzFilt*)ExtraPtr; p_filt->LocList.Add(value); }
void  DL6ICLS_PPFiltTrfrAnlz::ClearLocList()
	{ TrfrAnlzFilt * p_filt = (TrfrAnlzFilt*)ExtraPtr; p_filt->LocList.FreeAll(); }

int32 DL6ICLS_PPFiltTrfrAnlz::get_OpID()               { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, OpID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_OpID(int32 value)    { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, OpID); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_LocID()              { return ((TrfrAnlzFilt*)ExtraPtr)->LocList.GetSingle(); }
void  DL6ICLS_PPFiltTrfrAnlz::put_LocID(int32 value)   { TrfrAnlzFilt * p_filt = (TrfrAnlzFilt*)ExtraPtr; p_filt->LocList.FreeAll(); p_filt->LocList.Add(value); } // @v5.6.11 AHTOXA
int32 DL6ICLS_PPFiltTrfrAnlz::get_SupplID()            { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, SupplID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_SupplID(int32 value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, SupplID); }

int32 DL6ICLS_PPFiltTrfrAnlz::get_ArID()
{
	//IMPL_PPIFC_GETPROP(TrfrAnlzFilt, ArID);
	return ((TrfrAnlzFilt *)ExtraPtr)->ArList.GetSingle();
}
void  DL6ICLS_PPFiltTrfrAnlz::put_ArID(int32 value)
{
	//IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, ArID);
	((TrfrAnlzFilt *)ExtraPtr)->ArList.Add(value);
}
int32 DL6ICLS_PPFiltTrfrAnlz::get_DlvrAddrID()         { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, DlvrAddrID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_DlvrAddrID(int32 value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, DlvrAddrID); }

int32 DL6ICLS_PPFiltTrfrAnlz::get_AgentID()
{
	// IMPL_PPIFC_GETPROP(TrfrAnlzFilt, AgentID);
	return ((TrfrAnlzFilt *)ExtraPtr)->AgentList.GetSingle();
}
void  DL6ICLS_PPFiltTrfrAnlz::put_AgentID(int32 value)
{
	// IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, AgentID);
	((TrfrAnlzFilt *)ExtraPtr)->AgentList.Add(value);
}
int32 DL6ICLS_PPFiltTrfrAnlz::get_PsnCatID()           { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, PsnCatID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_PsnCatID(int32 value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, PsnCatID); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_CityID()              { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, CityID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_CityID(int32 value)   { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, CityID); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_GoodsGrpID()          { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, GoodsGrpID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_GoodsGrpID(int32 value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, GoodsGrpID); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_GoodsID()               { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, GoodsID); }
void  DL6ICLS_PPFiltTrfrAnlz::put_GoodsID(int32 value)  { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, GoodsID); }
PpyVTrfrAnlzFlags DL6ICLS_PPFiltTrfrAnlz::get_Flags()   { IMPL_PPIFC_GETPROP_CAST(TrfrAnlzFilt, Flags, PpyVTrfrAnlzFlags); }
void  DL6ICLS_PPFiltTrfrAnlz::put_Flags(PpyVTrfrAnlzFlags value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, Flags); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_InitOrd()            { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, InitOrd); }
void  DL6ICLS_PPFiltTrfrAnlz::put_InitOrd(int32 value) { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, InitOrd); }
int32 DL6ICLS_PPFiltTrfrAnlz::get_CtKind()             { IMPL_PPIFC_GETPROP(TrfrAnlzFilt, CtKind); }
void  DL6ICLS_PPFiltTrfrAnlz::put_CtKind(int32 value)  { IMPL_PPIFC_PUTPROP(TrfrAnlzFilt, CtKind); }
PpyVTrfrAnlzGrouping DL6ICLS_PPFiltTrfrAnlz::get_Grp() { IMPL_PPIFC_GETPROP_CAST(TrfrAnlzFilt, Grp, PpyVTrfrAnlzGrouping); }
void  DL6ICLS_PPFiltTrfrAnlz::put_Grp(PpyVTrfrAnlzGrouping value) { IMPL_PPIFC_PUTPROP_CAST(TrfrAnlzFilt, Grp, TrfrAnlzFilt::Grouping); }
PpyVSubstGrpGoods DL6ICLS_PPFiltTrfrAnlz::get_Sgg() { IMPL_PPIFC_GETPROP_CAST(TrfrAnlzFilt, Sgg, PpyVSubstGrpGoods); }
void  DL6ICLS_PPFiltTrfrAnlz::put_Sgg(PpyVSubstGrpGoods value) { IMPL_PPIFC_PUTPROP_CAST(TrfrAnlzFilt, Sgg, SubstGrpGoods); }
PpyVSubstGrpPerson DL6ICLS_PPFiltTrfrAnlz::get_Sgp() { IMPL_PPIFC_GETPROP_CAST(TrfrAnlzFilt, Sgp, PpyVSubstGrpPerson); }
void  DL6ICLS_PPFiltTrfrAnlz::put_Sgp(PpyVSubstGrpPerson value) { IMPL_PPIFC_PUTPROP_CAST(TrfrAnlzFilt, Sgp, SubstGrpPerson); }
PpyVSubstGrpDate DL6ICLS_PPFiltTrfrAnlz::get_Sgd() { IMPL_PPIFC_GETPROP_CAST(TrfrAnlzFilt, Sgd, PpyVSubstGrpDate); }
void  DL6ICLS_PPFiltTrfrAnlz::put_Sgd(PpyVSubstGrpDate value) { IMPL_PPIFC_PUTPROP_CAST(TrfrAnlzFilt, Sgd, SubstGrpDate); }
//
// } PPFiltTrfrAnlz
// PPViewTrfrAnlz {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewTrfrAnlz, DL6ICLS_PPViewTrfrAnlz_VTab, PPViewTrfrAnlz);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewTrfrAnlz::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltTrfrAnlz", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewTrfrAnlz::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(TrfrAnlz);
}

int32 DL6ICLS_PPViewTrfrAnlz::InitIteration(int32 order)
{
	return ((PPViewTrfrAnlz *)ExtraPtr)->InitIteration((PPViewTrfrAnlz::IterOrder)order);
}

void FillTrfrAnlzViewItem(const TrfrAnlzViewItem * pInner, SPpyVI_TrfrAnlz * pOuter)
{
		SString temp_buf;
		pOuter->RecTag = PPVIEWITEM_TRFRANLZ;
#define FLD(f) pOuter->f = pInner->f
		pOuter->Dt = (OleDate)pInner->Dt;
		FLD(OprNo);
		FLD(BillID);
		FLD(LocID);
		FLD(ArticleID);
		FLD(PersonID);
		FLD(OpID);
		FLD(GoodsID);
		FLD(SubGoodsClsID);
		FLD(LotID);
		FLD(DlvrLocID);
		FLD(LocCount);
		FLD(Qtty);
		FLD(PhQtty);
		FLD(Rest);
		FLD(Cost);
		FLD(Price);
		FLD(Discount);
		FLD(Amount);
		FLD(SaldoQtty);
		FLD(SaldoAmt);
		FLD(PVat);
		//(temp_buf = pInner->BillCode).CopyToOleStr(&pOuter->BillCode);
		//(temp_buf = pInner->DtText).CopyToOleStr(&pOuter->DtText);
		//(temp_buf = pInner->GoodsText).CopyToOleStr(&pOuter->GoodsText);
		//(temp_buf = pInner->PersonText).CopyToOleStr(&pOuter->PersonText);
		pInner->BillCode_.CopyToOleStr(&pOuter->BillCode);
		pInner->DtText_.CopyToOleStr(&pOuter->DtText);
		pInner->GoodsText_.CopyToOleStr(&pOuter->GoodsText);
		pInner->PersonText_.CopyToOleStr(&pOuter->PersonText);
#undef FLD
}

int32 DL6ICLS_PPViewTrfrAnlz::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_TrfrAnlz * p_item = (SPpyVI_TrfrAnlz *)item;
	PPViewTrfrAnlz * p_view = (PPViewTrfrAnlz *)ExtraPtr;
	if(p_view) {
		TrfrAnlzViewItem inner_item;
		if(p_view->NextIteration(&inner_item) > 0) {
			FillTrfrAnlzViewItem(&inner_item, p_item);
			ok = 1;
		}
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewTrfrAnlz::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewTrfrAnlz::GetTotal(PPYVIEWTOTAL total)
{
	PPViewTrfrAnlz * p_v = (PPViewTrfrAnlz *)ExtraPtr;
	if(p_v && total) {
		TrfrAnlzTotal inner_total;
		SPpyVT_TrfrAnlz * p_total = (SPpyVT_TrfrAnlz *)total;
		if(p_v->CalcTotal(&inner_total)) {
#define FLD(f) p_total->f = inner_total.f
			p_total->RecTag = 0; // @!
			FLD(Count);
			FLD(Qtty);
			FLD(PhQtty);
			FLD(Cost);
			FLD(Price);
			FLD(Discount);
			FLD(Amount);
			FLD(SaldoQtty);
			FLD(SaldoAmt);
			FLD(PVat);
#undef FLD
		}
		else
			AppError = 1;
	}
	return !AppError;
}
// } PPViewTrfrAnlz
//
//
// Alc report
//
int32 DL6ICLS_PPViewTrfrAnlz::InitAlcRepParam(SPpyV_TrfrAnlz_AlcRepParam * pParam)
{
	int    ok = 0;
	PPViewTrfrAnlz * p_v = (PPViewTrfrAnlz *)ExtraPtr;
	if(p_v) {
		AlcReportParam param;
		param.ImpExpTag        = pParam->ImpExpTag;
		param.ManufOptBuyerTag = pParam->ManufOptBuyerTag;
		param.ManufKindID      = pParam->ManufKindID;
		param.ImportKindID     = pParam->ImportKindID;
		p_v->SetAlcRepParam(&param);
		ok = 1;
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPViewTrfrAnlz::NextIteration_AlcRep(SPpyVI_TrfrAnlz_AlcRep * pItem)
{
	int    ok = -1;
	PPViewTrfrAnlz * p_v = (PPViewTrfrAnlz *)ExtraPtr;
	TrfrAnlzViewItem_AlcRep item;
	if(p_v && p_v->NextIteration_AlcRep(&item) > 0) {
		FillTrfrAnlzViewItem(&item.Item, &pItem->Item);
		FillLotRec(&item.OrgLotRec, &pItem->OrgLotRec);
		FillGoodsRec_AlcRep(&item.GoodsRec, &item.GCPack, &item.GoodsStock, &item.GoodsExt, &pItem->GoodsRec);
		pItem->IsImport     = item.IsImport;
		pItem->IsExport     = item.IsExport;
		pItem->IsManuf      = item.IsManuf;
		pItem->IsOptBuyer   = item.IsOptBuyer;
		pItem->PersonID     = item.PersonID;
		pItem->OrgLot_Prsn_SupplID = item.OrgLot_Prsn_SupplID;
		ok = 1;
	}
	SetAppError(ok);
	return ok;
}

struct AlcRepOpList {
	int Init(PPID opID, PPObjOprKind * pObj, PPIDArray & rList)
	{
		rList.freeAll();
		if(opID) {
			pObj->GetGenericList(opID, &rList);
			if(opID)
				rList.add(opID);
			rList.sort();
		}
		return 1;
	}
	int Check(PPID opID, PPIDArray & rList) const
	{
		return BIN(rList.bsearch(opID, 0) > 0);
	}
	PPIDArray RcptList;
	PPIDArray RcptEtcList;
	PPIDArray RetList;
	PPIDArray SupplRetList;
	PPIDArray ExpList;
	PPIDArray ExpEtcList;
	PPIDArray IntrMovList;
};

DL6_IC_CONSTRUCTOR(AlcRepOpList, DL6ICLS_AlcRepOpList_VTab)
{
	AlcRepOpList * p_e = new AlcRepOpList();
	ExtraPtr = p_e;

}
DL6_IC_DESTRUCTOR(AlcRepOpList)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	ZDELETE(p_e);
}

int32 DL6ICLS_AlcRepOpList::Init(int32 genRcpt, int32 genRcptEtc, int32 genRet, int32 genSupplRet, int32 genExp, int32 genExpEtc, int32 genIntrMov)
{
	int    ok = 0;
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	if(p_e) {
		PPObjOprKind obj;
		p_e->Init(genRcpt,     &obj, p_e->RcptList);
		p_e->Init(genRcptEtc,  &obj, p_e->RcptEtcList);
		p_e->Init(genRet,      &obj, p_e->RetList);
		p_e->Init(genSupplRet, &obj, p_e->SupplRetList);
		p_e->Init(genExp,      &obj, p_e->ExpList);
		p_e->Init(genExpEtc,   &obj, p_e->ExpEtcList);
		p_e->Init(genIntrMov,  &obj, p_e->IntrMovList);
		ok = 1;
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_AlcRepOpList::IsRcpt(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->RcptList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsRcptEtc(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->RcptEtcList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsRet(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->RetList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsSupplRet(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->SupplRetList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsExp(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->ExpList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsExpEtc(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->ExpEtcList) : 0;
}

int32 DL6ICLS_AlcRepOpList::IsIntrMov(int32 opID)
{
	AlcRepOpList * p_e = (AlcRepOpList*)ExtraPtr;
	return (p_e) ? p_e->Check(opID, p_e->IntrMovList) : 0;
}
//
// } Alc report
//
DL6_IC_CONSTRUCTOR(PrcssrAlcReport, DL6ICLS_PrcssrAlcReport_VTab)
{
	PrcssrAlcReport * p_prc = new PrcssrAlcReport;
	ExtraPtr = p_prc;
}

DL6_IC_DESTRUCTOR(PrcssrAlcReport)
{
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	ZDELETE(p_prc);
}
//
// Interface IPrcssrAlcReport implementation
//
int32 DL6ICLS_PrcssrAlcReport::Init()
{
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	return p_prc ? p_prc->Init() : 0;
}

int32 DL6ICLS_PrcssrAlcReport::GetConfig(SAlcRepConfig * pCfg)
{
	int    ok = -1;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		p_prc->SetConfig(0);
		if(pCfg) {
			#define FLD(f) pCfg->f = p_prc->Cfg.f
			FLD(RcptOpID);
			FLD(SaleRetOpID);
			FLD(RcptEtcOpID);
			FLD(ExpndOpID);
			FLD(SupplRetOpID);
			FLD(ExpndEtcOpID);
			FLD(IntrExpndOpID);
			FLD(AlcGoodsGrpID);
			FLD(BeerGoodsGrpID);
			FLD(CategoryTagID);
			FLD(AlcLicRegTypeID);
			FLD(WhsExpTagID);
			FLD(ManufImpTagID);
			FLD(KppRegTypeID);
			FLD(KppDlvrExt);
			FLD(CategoryClsDim);
			FLD(VolumeClsDim);
			#undef FLD
			pCfg->TranspLicRegTypeID = p_prc->Cfg.E.TransportLicRegTypeID;
			SString temp_buf;
			(temp_buf = p_prc->Cfg.SubstCategoryCode).CopyToOleStr(&pCfg->SubstCategoryCode);
			ok = 1;
		}
	}
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::SetConfig(SAlcRepConfig * pCfg)
{
	int    ok = 1;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		PrcssrAlcReport::Config cfg;
		#define FLD(f) cfg.f = pCfg->f
		FLD(RcptOpID);
		FLD(SaleRetOpID);
		FLD(RcptEtcOpID);
		FLD(ExpndOpID);
		FLD(SupplRetOpID);
		FLD(ExpndEtcOpID);
		FLD(IntrExpndOpID);
		FLD(AlcGoodsGrpID);
		FLD(BeerGoodsGrpID);
		FLD(CategoryTagID);
		FLD(AlcLicRegTypeID);
		FLD(WhsExpTagID);
		FLD(ManufImpTagID);
		FLD(KppRegTypeID);
		FLD(KppDlvrExt);
		#undef FLD
		cfg.CategoryClsDim = (int16)pCfg->CategoryClsDim;
		cfg.VolumeClsDim = (int16)pCfg->VolumeClsDim;
		cfg.E.TransportLicRegTypeID = pCfg->TranspLicRegTypeID;
		SString temp_buf;
		temp_buf.CopyFromOleStr(pCfg->SubstCategoryCode).CopyTo(cfg.SubstCategoryCode, sizeof(cfg.SubstCategoryCode));
		ok = p_prc->SetConfig(&cfg);
	}
	else
		ok = 0;
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::ConfigAddStorageLoc(int32 locID)
{
	int    ok = 1;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		if(p_prc->Cfg.StorageLocList.add(locID)) {
			p_prc->Cfg.StorageLocList.sortAndUndup();
		}
		else {
			ok = PPSetErrorSLib();
		}
	}
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::ConfigAddLotManufTag(int32 manufTagID)
{
	int    ok = 1;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		if(p_prc->Cfg.LotManufTagList.add(manufTagID)) {
			p_prc->Cfg.LotManufTagList.sortAndUndup();
		}
		else {
			ok = PPSetErrorSLib();
		}
	}
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::IsStorageLoc(int32 locID)
{
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	return p_prc ? p_prc->IsStorageLoc(locID) : 0;
}

int32 DL6ICLS_PrcssrAlcReport::IsStorageBillLoc(int32 billID)
{
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	return p_prc ? p_prc->IsStorageBillLoc(billID) : 0;
}

int32 DL6ICLS_PrcssrAlcReport::IsAlcGoods(int32 goodsID)
{
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	return p_prc ? p_prc->IsAlcGoods(goodsID) : 0;
}

int32 DL6ICLS_PrcssrAlcReport::PreprocessGoodsItem(int32 goodsID, int32 lotID, int32 flags, SAlcRepGoodsItem* pItem)
{
	int    ok = 1;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		PrcssrAlcReport::GoodsItem item;
        ok = p_prc->PreprocessGoodsItem(goodsID, lotID, 0, flags, item);
		if(pItem) {
			pItem->StatusFlags = item.StatusFlags;
			pItem->GoodsID = item.GoodsID;
			pItem->LotID = item.LotID;
			pItem->Volume = item.Volume;
			pItem->Brutto = item.Brutto;
			SString temp_buf;
			(temp_buf = item.CategoryCode).CopyToOleStr(&pItem->CategoryCode);
			(temp_buf = item.CategoryName).Transf(CTRANSF_OUTER_TO_INNER).CopyToOleStr(&pItem->CategoryName); // Эта строка изначально в CHAR-кодировке
			(temp_buf = item.MsgPool).CopyToOleStr(&pItem->MsgPool);
		}
	}
	else
		ok = 0;
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::GetLotManuf(int32 lotID, SString* pMsg)
{
	PPID   manuf_id = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		SString temp_buf;
		p_prc->GetLotManufID(lotID, &manuf_id, &temp_buf);
		ASSIGN_PTR(pMsg, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
	}
	return manuf_id;
}

/*
IUnknown * GetPPObjIStrAssocList(SCoClass * pCls, PPObject * pObj, long extraParam)
{
	IUnknown * p = 0;
	if(pObj) {
		if(pCls->CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p)) {
			StrAssocArray * p_list_env = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p);
			if(p_list_env) {
				StrAssocArray * p_list = pObj->MakeStrAssocList(extraParam);
				if(p_list) {
					*p_list_env = *p_list;
					p_list_env->setPointer(0);
					ZDELETE(p_list);
				}
				else
					ReleaseUnknObj(&p);
			}
			else
				ReleaseUnknObj(&p);
		}
	}
	return p;
}
*/

// @construction
ILongList * DL6ICLS_PrcssrAlcReport::GetWkrRegisterListByPeriod(int32 wkr, int32 psnID, int32 locID, SDateRange* pPeriod)
{
	IUnknown * p = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	LongArray * p_reg_id_list = 0;
	THROW(p_prc);
	THROW(CreateInnerInstance("LongList", "ILongList", (void **)&p));
	THROW(p_reg_id_list = (LongArray *)SCoClass::GetExtraPtrByInterface(p));
	{
		DateRange period;
		if(pPeriod)
			period = OleDateRangeToDateRange(*pPeriod);
		else
			period.SetZero();
		RegisterArray reg_list;
		p_prc->GetWkrRegisterListByPeriod(wkr, psnID, locID, period, &reg_list);
        for(uint i = 0; i < reg_list.getCount(); i++) {
        	p_reg_id_list->add(reg_list.at(i).ID);
        }
	}
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (ILongList*)p;
}
//

int32 DL6ICLS_PrcssrAlcReport::FetchRegister(int32 regID, PPID psnID, PPID locID, SPpyO_Register * pRec)
{
	int    ok = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		RegisterTbl::Rec reg_rec;
		ok = p_prc->FetchRegister(regID, psnID, locID, &reg_rec);
		FillRegisterRec(&reg_rec, (SPpyO_Register *)pRec, 1);
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PrcssrAlcReport::GetWkrRegister(long wkr, int32 psnID, int32 locID, LDATE actualDate, SPpyO_Register* pRec)
{
	int    ok = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		RegisterTbl::Rec reg_rec;
		ok = p_prc->GetWkrRegister(wkr, psnID, locID, actualDate, &reg_rec);
		FillRegisterRec(&reg_rec, (SPpyO_Register *)pRec, 1);
	}
	else
		AppError = 1;
	return ok;
}

SString & DL6ICLS_PrcssrAlcReport::GetWkrRegisterNumber(long wkr, int32 psnID, int32 locID, LDATE actualDate)
{
	int    ok = 0;
	RetStrBuf = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	if(p_prc) {
		RegisterTbl::Rec reg_rec;
		ok = p_prc->GetWkrRegister(wkr, psnID, locID, actualDate, &reg_rec);
		RetStrBuf = reg_rec.Num;
	}
	else
		AppError = 1;
	return RetStrBuf;
}

IStrAssocList * DL6ICLS_PrcssrAlcReport::GetWkrRegisterList(int32 wkr, int32 psnID, int32 locID)
{
	RegisterArray reg_list;
	IUnknown * p = 0;
	StrAssocArray * p_reg_list = 0;
	PrcssrAlcReport * p_prc = (PrcssrAlcReport *)ExtraPtr;
	THROW(CreateInnerInstance("StrAssocList", "IStrAssocList", (void **)&p));
	THROW(p_reg_list = (StrAssocArray *)SCoClass::GetExtraPtrByInterface(p));
	THROW(p_prc);
	if(p_prc->GetWkrRegisterList(wkr, psnID, locID, ZERODATE, &reg_list) > 0) {
		SString temp_buf;
		uint count = reg_list.getCount();
		for(uint i = 0; i < count; i++) {
			const RegisterTbl::Rec & r_reg = reg_list.at(i);
			temp_buf.Z().Cat(r_reg.Dt).Cat("..").Cat(r_reg.Expiry);
			p_reg_list->Add(i + 1, 0, (const char*)temp_buf);
		}
	}
	CATCH
		ReleaseUnknObj(&p);
		AppError = 1;
	ENDCATCH
	return (IStrAssocList*)p;
}
//
// PPViewGoods {
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltGoods, DL6ICLS_PPFiltGoods_VTab, GoodsFilt);
//
// Interface IPpyFilt_Goods implementation
//
void DL6ICLS_PPFiltGoods::SetLotPeriod(LDATE low, LDATE upp)
	{ ((GoodsFilt *)ExtraPtr)->LotPeriod.Set(low, upp); }
SDateRange DL6ICLS_PPFiltGoods::get_LotPeriod()
	{ return DateRangeToOleDateRange(((GoodsFilt *)ExtraPtr)->LotPeriod); }
void DL6ICLS_PPFiltGoods::AddGoodsGroupID(int32 id)
{
	GoodsFilt * p_filt = ((GoodsFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->GrpIDList.Add(id);
}

int32 DL6ICLS_PPFiltGoods::get_GrpID()                     { IMPL_PPIFC_GETPROP(GoodsFilt, GrpID); }
void  DL6ICLS_PPFiltGoods::put_GrpID(int32 value)          { IMPL_PPIFC_PUTPROP(GoodsFilt, GrpID); }
int32 DL6ICLS_PPFiltGoods::get_ManufID()                   { IMPL_PPIFC_GETPROP(GoodsFilt, ManufID); }
void  DL6ICLS_PPFiltGoods::put_ManufID(int32 value)        { IMPL_PPIFC_PUTPROP(GoodsFilt, ManufID); }
int32 DL6ICLS_PPFiltGoods::get_ManufCountryID()            { IMPL_PPIFC_GETPROP(GoodsFilt, ManufCountryID); }
void  DL6ICLS_PPFiltGoods::put_ManufCountryID(int32 value) { IMPL_PPIFC_PUTPROP(GoodsFilt, ManufCountryID); }
int32 DL6ICLS_PPFiltGoods::get_UnitID()                    { IMPL_PPIFC_GETPROP(GoodsFilt, UnitID); }
void  DL6ICLS_PPFiltGoods::put_UnitID(int32 value)         { IMPL_PPIFC_PUTPROP(GoodsFilt, UnitID); }
int32 DL6ICLS_PPFiltGoods::get_PhUnitID()                  { IMPL_PPIFC_GETPROP(GoodsFilt, PhUnitID); }
void  DL6ICLS_PPFiltGoods::put_PhUnitID(int32 value)       { IMPL_PPIFC_PUTPROP(GoodsFilt, PhUnitID); }
int32 DL6ICLS_PPFiltGoods::get_SupplID()                   { IMPL_PPIFC_GETPROP(GoodsFilt, SupplID); }
void  DL6ICLS_PPFiltGoods::put_SupplID(int32 value)        { IMPL_PPIFC_PUTPROP(GoodsFilt, SupplID); }
int32 DL6ICLS_PPFiltGoods::get_GoodsTypeID()               { IMPL_PPIFC_GETPROP(GoodsFilt, GoodsTypeID); }
void  DL6ICLS_PPFiltGoods::put_GoodsTypeID(int32 value)    { IMPL_PPIFC_PUTPROP(GoodsFilt, GoodsTypeID); }
int32 DL6ICLS_PPFiltGoods::get_TaxGrpID()                  { IMPL_PPIFC_GETPROP(GoodsFilt, TaxGrpID); }
void  DL6ICLS_PPFiltGoods::put_TaxGrpID(int32 value)       { IMPL_PPIFC_PUTPROP(GoodsFilt, TaxGrpID); }
int32 DL6ICLS_PPFiltGoods::get_LocID()                     { IMPL_PPIFC_GETPROP(GoodsFilt, LocList.GetSingle()); }
void  DL6ICLS_PPFiltGoods::put_LocID(int32 value)
{
	//IMPL_PPIFC_PUTPROP(GoodsFilt, LocID);
	((GoodsFilt *)ExtraPtr)->LocList.SetSingle(value);
}
PpyVGoodsFlags DL6ICLS_PPFiltGoods::get_Flags()            { IMPL_PPIFC_GETPROP_CAST(GoodsFilt, Flags, PpyVGoodsFlags); }
void  DL6ICLS_PPFiltGoods::put_Flags(PpyVGoodsFlags value) { IMPL_PPIFC_PUTPROP(GoodsFilt, Flags); }
int32 DL6ICLS_PPFiltGoods::get_VatRate()                   { IMPL_PPIFC_GETPROP(GoodsFilt, VatRate); }
void  DL6ICLS_PPFiltGoods::put_VatRate(int32 value)        { IMPL_PPIFC_PUTPROP(GoodsFilt, VatRate); }
LDATE  DL6ICLS_PPFiltGoods::get_VatDate()                  { IMPL_PPIFC_GETPROP(GoodsFilt, VatDate); }
void  DL6ICLS_PPFiltGoods::put_VatDate(LDATE value)        { IMPL_PPIFC_PUTPROP(GoodsFilt, VatDate); }
int32 DL6ICLS_PPFiltGoods::get_BrandID()                   { IMPL_PPIFC_GETPROP(GoodsFilt, BrandList.GetSingle()); }
void  DL6ICLS_PPFiltGoods::put_BrandID(int32 value)
{
	//IMPL_PPIFC_PUTPROP(GoodsFilt, BrandID);
	((GoodsFilt *)ExtraPtr)->BrandList.SetSingle(value);
}
void  DL6ICLS_PPFiltGoods::put_SrchStr(SString & value)    { IMPL_PPIFC_PUTPROP(GoodsFilt, SrchStr_); }
SString & DL6ICLS_PPFiltGoods::get_SrchStr()               { IMPL_PPIFC_GETPROP(GoodsFilt, SrchStr_); }
void  DL6ICLS_PPFiltGoods::put_BarcodeLen(SString & value) { IMPL_PPIFC_PUTPROP(GoodsFilt, BarcodeLen); }
SString & DL6ICLS_PPFiltGoods::get_BarcodeLen()            { IMPL_PPIFC_GETPROP(GoodsFilt, BarcodeLen); }

void  DL6ICLS_PPFiltGoods::put_GdsClsID(int32 value)       { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.GdsClsID); }
int32 DL6ICLS_PPFiltGoods::get_GdsClsID()                  { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.GdsClsID); }
int32 DL6ICLS_PPFiltGoods::get_CodeArID()                  { IMPL_PPIFC_GETPROP(GoodsFilt, CodeArID); }
void  DL6ICLS_PPFiltGoods::put_CodeArID(int32 value)       { IMPL_PPIFC_PUTPROP(GoodsFilt, CodeArID); }

#define FILTGOODS_CLSF_BLOCK(cl) \
void  DL6ICLS_PPFiltGoods::put_##cl##ID(int32 value)         { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.##cl##List); } \
int32 DL6ICLS_PPFiltGoods::get_##cl##ID()                    { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.##cl##List.GetSingle()); }

FILTGOODS_CLSF_BLOCK(Kind)
FILTGOODS_CLSF_BLOCK(AddObj)
FILTGOODS_CLSF_BLOCK(Grade)
FILTGOODS_CLSF_BLOCK(AddObj2)
/*
void  DL6ICLS_PPFiltGoods::put_KindID(int32 value)         { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.KindID); }
int32 DL6ICLS_PPFiltGoods::get_KindID()                    { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.KindID); }
void  DL6ICLS_PPFiltGoods::put_AddObjID(int32 value)       { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.AddObjID); }
int32 DL6ICLS_PPFiltGoods::get_AddObjID()                  { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.AddObjID); }
void  DL6ICLS_PPFiltGoods::put_GradeID(int32 value)        { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.GradeID); }
int32 DL6ICLS_PPFiltGoods::get_GradeID()                   { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.GradeID); }
void  DL6ICLS_PPFiltGoods::put_AddObj2ID(int32 value)      { IMPL_PPIFC_PUTPROP(GoodsFilt, Ep.AddObj2ID); }
int32 DL6ICLS_PPFiltGoods::get_AddObj2ID()                 { IMPL_PPIFC_GETPROP(GoodsFilt, Ep.AddObj2ID); }
*/

#define FILTGOODS_DIM_BLOCK(alpha) \
void   DL6ICLS_PPFiltGoods::put_Dim##alpha##Min(double value) { ((GoodsFilt*)ExtraPtr)->Ep.Dim##alpha##_Rng.low = value; } \
double DL6ICLS_PPFiltGoods::get_Dim##alpha##Min()             { return ((GoodsFilt*)ExtraPtr)->Ep.Dim##alpha##_Rng.low;  } \
void   DL6ICLS_PPFiltGoods::put_Dim##alpha##Max(double value) { ((GoodsFilt*)ExtraPtr)->Ep.Dim##alpha##_Rng.upp = value; } \
double DL6ICLS_PPFiltGoods::get_Dim##alpha##Max()             { return ((GoodsFilt*)ExtraPtr)->Ep.Dim##alpha##_Rng.upp;  }

FILTGOODS_DIM_BLOCK(X)
FILTGOODS_DIM_BLOCK(Y)
FILTGOODS_DIM_BLOCK(Z)
FILTGOODS_DIM_BLOCK(W)
/*
void  DL6ICLS_PPFiltGoods::put_DimXMin(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimX_Rng.low = value; }
double DL6ICLS_PPFiltGoods::get_DimXMin()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimX_Rng.low; }
void  DL6ICLS_PPFiltGoods::put_DimXMax(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimX_Rng.upp = value; }
double DL6ICLS_PPFiltGoods::get_DimXMax()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimX_Rng.upp; }
void  DL6ICLS_PPFiltGoods::put_DimYMin(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimY_Rng.low = value; }
double DL6ICLS_PPFiltGoods::get_DimYMin()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimY_Rng.low; }
void  DL6ICLS_PPFiltGoods::put_DimYMax(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimY_Rng.upp = value; }
double DL6ICLS_PPFiltGoods::get_DimYMax()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimY_Rng.upp; }
void  DL6ICLS_PPFiltGoods::put_DimZMin(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimZ_Rng.low = value; }
double DL6ICLS_PPFiltGoods::get_DimZMin()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimZ_Rng.low; }
void  DL6ICLS_PPFiltGoods::put_DimZMax(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimZ_Rng.upp = value; }
double DL6ICLS_PPFiltGoods::get_DimZMax()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimZ_Rng.upp; }
void  DL6ICLS_PPFiltGoods::put_DimWMin(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimW_Rng.low = value; }
double DL6ICLS_PPFiltGoods::get_DimWMin()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimW_Rng.low; }
void  DL6ICLS_PPFiltGoods::put_DimWMax(double value)
	{ ((GoodsFilt*)ExtraPtr)->Ep.DimW_Rng.upp = value; }
double DL6ICLS_PPFiltGoods::get_DimWMax()
	{ return ((GoodsFilt*)ExtraPtr)->Ep.DimW_Rng.upp; }
*/

DL6_IC_CONSTRUCTION_EXTRA(PPViewGoods, DL6ICLS_PPViewGoods_VTab, PPViewGoods);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewGoods::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltGoods", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewGoods::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(Goods);
}

int32 DL6ICLS_PPViewGoods::InitIteration(int32 order)
{
	return ((PPViewGoods *)ExtraPtr)->InitIteration((PPViewGoods::IterOrder)order);
}

int32 DL6ICLS_PPViewGoods::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_Goods * p_item = (SPpyVI_Goods *)item;
	GoodsViewItem inner_item;
	if(((PPViewGoods *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_GOODS;
#define FLD(f) p_item->f = inner_item.f
		FLD(ID);
		FLD(Kind);
		FLD(ParentID);
		FLD(GoodsTypeID);
		FLD(UnitID);
		FLD(PhUnitID);
		FLD(PhUPerU);
		FLD(ManufID);
		FLD(StrucID);
		FLD(TaxGrpID);
		FLD(WrOffGrpID);
		FLD(Flags);
		FLD(GdsClsID);
		FLD(BrandID);
		FLD(DefBCodeStrucID);

		FLD(Brutto);
		p_item->Length = inner_item.PckgDim.Length;
		p_item->Width  = inner_item.PckgDim.Width;
		p_item->Height = inner_item.PckgDim.Height;
		FLD(MinStock);
		FLD(Package);
		(temp_buf = inner_item.Name).CopyToOleStr(&p_item->Name);
		(temp_buf = inner_item.Abbr).CopyToOleStr(&p_item->Abbr);
		(temp_buf = inner_item.Barcode).CopyToOleStr(&p_item->Barcode);
		(temp_buf = inner_item.StrucType).CopyToOleStr(&p_item->StrucType);
#undef FLD
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewGoods::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewGoods::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
// } PPViewGoods
//
//
// PPViewGoodsRest {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewGoodsRest, DL6ICLS_PPViewGoodsRest_VTab, PPViewGoodsRest);
//
// Interface IPpyFilt_Goods implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltGoodsRest, DL6ICLS_PPFiltGoodsRest_VTab, GoodsRestFilt);

void DL6ICLS_PPFiltGoodsRest::SetPrgnPeriod(LDATE low, LDATE upp)
	{ ((GoodsRestFilt *)ExtraPtr)->PrgnPeriod.Set(low, upp); }
SDateRange DL6ICLS_PPFiltGoodsRest::get_PrgnPeriod()
	{ return DateRangeToOleDateRange(((GoodsRestFilt *)ExtraPtr)->PrgnPeriod); }
void DL6ICLS_PPFiltGoodsRest::AddLocationID(int32 id)
{
	GoodsRestFilt * p_filt = ((GoodsRestFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->LocList.Add(id);
}

PpyVGoodsRestCalcMethod DL6ICLS_PPFiltGoodsRest::get_CalcMethod()             { IMPL_PPIFC_GETPROP_CAST(GoodsRestFilt, CalcMethod, PpyVGoodsRestCalcMethod); }
void  DL6ICLS_PPFiltGoodsRest::put_CalcMethod(PpyVGoodsRestCalcMethod value)  { IMPL_PPIFC_PUTPROP(GoodsRestFilt, CalcMethod); }

PpyVGoodsRestFlags DL6ICLS_PPFiltGoodsRest::get_Flags()                       { IMPL_PPIFC_GETPROP_CAST(GoodsRestFilt, Flags, PpyVGoodsRestFlags); }
void  DL6ICLS_PPFiltGoodsRest::put_Flags(PpyVGoodsRestFlags value)            { IMPL_PPIFC_PUTPROP(GoodsRestFilt, Flags); }
int32 DL6ICLS_PPFiltGoodsRest::get_AmtType()             		              { IMPL_PPIFC_GETPROP(GoodsRestFilt, AmtType); }
void  DL6ICLS_PPFiltGoodsRest::put_AmtType(int32 value)                       { IMPL_PPIFC_PUTPROP(GoodsRestFilt, AmtType); }

int32 DL6ICLS_PPFiltGoodsRest::get_CalcPrognosis()
{
	// @v9.5.8 IMPL_PPIFC_GETPROP(GoodsRestFilt, CalcPrognosis);
	return BIN(((GoodsRestFilt *)ExtraPtr)->Flags2 & GoodsRestFilt::f2CalcPrognosis); // @v9.5.8
}
void  DL6ICLS_PPFiltGoodsRest::put_CalcPrognosis(int32 value)
{
	// @v9.5.8 IMPL_PPIFC_PUTPROP(GoodsRestFilt, CalcPrognosis);
	SETFLAG(((GoodsRestFilt *)ExtraPtr)->Flags2, GoodsRestFilt::f2CalcPrognosis, value); // @v9.5.8
}

LDATE DL6ICLS_PPFiltGoodsRest::get_Date()                                     { IMPL_PPIFC_GETPROP(GoodsRestFilt, Date); }
void  DL6ICLS_PPFiltGoodsRest::put_Date(LDATE value)                          { IMPL_PPIFC_PUTPROP(GoodsRestFilt, Date); }
int32 DL6ICLS_PPFiltGoodsRest::get_SupplID()                                  { IMPL_PPIFC_GETPROP(GoodsRestFilt, SupplID); }
void  DL6ICLS_PPFiltGoodsRest::put_SupplID(int32 value)                       { IMPL_PPIFC_PUTPROP(GoodsRestFilt, SupplID); }
int32 DL6ICLS_PPFiltGoodsRest::get_GoodsGrpID()                               { IMPL_PPIFC_GETPROP(GoodsRestFilt, GoodsGrpID); }
void  DL6ICLS_PPFiltGoodsRest::put_GoodsGrpID(int32 value)                    { IMPL_PPIFC_PUTPROP(GoodsRestFilt, GoodsGrpID); }
int32 DL6ICLS_PPFiltGoodsRest::get_QuotKindID()                               { IMPL_PPIFC_GETPROP(GoodsRestFilt, QuotKindID); }
void  DL6ICLS_PPFiltGoodsRest::put_QuotKindID(int32 value)                    { IMPL_PPIFC_PUTPROP(GoodsRestFilt, QuotKindID); }
int32 DL6ICLS_PPFiltGoodsRest::get_AgentID()                                  { IMPL_PPIFC_GETPROP(GoodsRestFilt, AgentID); }
void  DL6ICLS_PPFiltGoodsRest::put_AgentID(int32 value)                       { IMPL_PPIFC_PUTPROP(GoodsRestFilt, AgentID); }
int32 DL6ICLS_PPFiltGoodsRest::get_BrandID()                                  { IMPL_PPIFC_GETPROP(GoodsRestFilt, BrandID); }
void  DL6ICLS_PPFiltGoodsRest::put_BrandID(int32 value)                       { IMPL_PPIFC_PUTPROP(GoodsRestFilt, BrandID); }
PpyVSubstGrpGoods DL6ICLS_PPFiltGoodsRest::get_Sgg()                          { IMPL_PPIFC_GETPROP_CAST(GoodsRestFilt, Sgg, PpyVSubstGrpGoods); }
void  DL6ICLS_PPFiltGoodsRest::put_Sgg(PpyVSubstGrpGoods value)               { IMPL_PPIFC_PUTPROP_CAST(GoodsRestFilt, Sgg, SubstGrpGoods); }
LDATE DL6ICLS_PPFiltGoodsRest::get_DeficitDt()                                { IMPL_PPIFC_GETPROP(GoodsRestFilt, DeficitDt); }
void  DL6ICLS_PPFiltGoodsRest::put_DeficitDt(LDATE value)                     { IMPL_PPIFC_PUTPROP(GoodsRestFilt, DeficitDt); }
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewGoodsRest::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltGoodsRest", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewGoodsRest::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(GoodsRest);
}

int32 DL6ICLS_PPViewGoodsRest::InitIteration(int32 order)
{
	return ((PPViewGoodsRest *)ExtraPtr)->InitIteration((PPViewGoodsRest::IterOrder)order);
}

int32 DL6ICLS_PPViewGoodsRest::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyVI_GoodsRest * p_item = (SPpyVI_GoodsRest *)item;
	GoodsRestViewItem inner_item;
	if(((PPViewGoodsRest *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_GOODSREST;
#define FLD(f) p_item->f = inner_item.f
		FLD(GoodsID);
		FLD(GoodsGrpID);
		FLD(LocID);
		FLD(IsPredictTrust);
		FLD(Rest);
		FLD(Deficit);
		FLD(PhRest);
		FLD(Order);
		FLD(UnitPerPack);
		FLD(Cost);
		FLD(Price);
		FLD(SumCost);
		FLD(SumPrice);
		FLD(PctAddedVal);
		FLD(Predict);
		FLD(RestInDays);
		FLD(MinStock);
		FLD(SupplOrder);
		FLD(SubstAsscCount);
		FLD(DraftRcpt);
#undef FLD
		(temp_buf = inner_item.GoodsGrpName).CopyToOleStr(&p_item->GoodsGrpName); // @v8.1.3 inner_item.P_GoodsGrpName-->inner_item.GoodsGrpName
		(temp_buf = inner_item.GoodsName).CopyToOleStr(&p_item->GoodsName);
		(temp_buf = inner_item.UnitName).CopyToOleStr(&p_item->UnitName);
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewGoodsRest::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewGoodsRest::GetTotal(PPYVIEWTOTAL total)
{
	GoodsRestTotal inner_total;
	PPViewGoodsRest * p_v = (PPViewGoodsRest *)ExtraPtr;
	SPpyVT_GoodsRest * p_total = (SPpyVT_GoodsRest*)total;
	if(p_v && p_total && p_v->GetTotal(&inner_total) > 0) {
		p_total->RecTag = 0; // @!
#define FLD(f) p_total->f = inner_total.f
		FLD(Count);
		FLD(Quantity);
		FLD(PhQtty);
		FLD(Order);
		FLD(SumCost);
		FLD(SumPrice);
		FLD(PctAddedVal);
		FLD(DraftRcpt);
		FLD(SumDraftCost);
		FLD(SumDraftPrice);
#undef FLD
	}
	return !AppError;
}
//
// } PPViewGoodsRest
//
//
// PPViewBill {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewBill, DL6ICLS_PPViewBill_VTab, PPViewBill);
//
// Interface IPpyFilt_Bill implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltBill, DL6ICLS_PPFiltBill_VTab, BillFilt);

//
// Interface IPpyFilt_Bill implementation
//
void DL6ICLS_PPFiltBill::SetPeriod(LDATE low, LDATE upp)
{
	((BillFilt *)ExtraPtr)->Period.Set(low, upp);
}

void DL6ICLS_PPFiltBill::SetPaymPeriod(LDATE low, LDATE upp)
{
	((BillFilt *)ExtraPtr)->PaymPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltBill::AddLocationID(int32 id)
{
	BillFilt * p_filt = ((BillFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->LocList.Add(id);
}

int32 DL6ICLS_PPFiltBill::get_Tag() {IMPL_PPIFC_GETPROP(BillFilt, Tag);}
void  DL6ICLS_PPFiltBill::put_Tag(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, Tag);}
PpyVBrowseBillsType DL6ICLS_PPFiltBill::get_Bbt() {IMPL_PPIFC_GETPROP_CAST(BillFilt, Bbt, PpyVBrowseBillsType);}
void  DL6ICLS_PPFiltBill::put_Bbt(PpyVBrowseBillsType value) {IMPL_PPIFC_PUTPROP_CAST(BillFilt, Bbt, BrowseBillsType);}

SDateRange DL6ICLS_PPFiltBill::get_Period()
	{ return DateRangeToOleDateRange(((BillFilt *)ExtraPtr)->Period); }

SDateRange DL6ICLS_PPFiltBill::get_PaymPeriod()
	{ return DateRangeToOleDateRange(((BillFilt *)ExtraPtr)->PaymPeriod); }

int32 DL6ICLS_PPFiltBill::get_MainOrgID() {IMPL_PPIFC_GETPROP(BillFilt, MainOrgID);}
void  DL6ICLS_PPFiltBill::put_MainOrgID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, MainOrgID);}
int32 DL6ICLS_PPFiltBill::get_PoolOpID() {IMPL_PPIFC_GETPROP(BillFilt, PoolOpID);}
void  DL6ICLS_PPFiltBill::put_PoolOpID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, PoolOpID);}
int32 DL6ICLS_PPFiltBill::get_OpID() {IMPL_PPIFC_GETPROP(BillFilt, OpID);}
void  DL6ICLS_PPFiltBill::put_OpID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, OpID);}
int32 DL6ICLS_PPFiltBill::get_CurID() {IMPL_PPIFC_GETPROP(BillFilt, CurID);}
void  DL6ICLS_PPFiltBill::put_CurID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, CurID);}
int32 DL6ICLS_PPFiltBill::get_AccSheetID() {IMPL_PPIFC_GETPROP(BillFilt, AccSheetID);}
void  DL6ICLS_PPFiltBill::put_AccSheetID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, AccSheetID);}
int32 DL6ICLS_PPFiltBill::get_ObjectID() {IMPL_PPIFC_GETPROP(BillFilt, ObjectID);}
void  DL6ICLS_PPFiltBill::put_ObjectID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, ObjectID);}
int32 DL6ICLS_PPFiltBill::get_Object2ID() {IMPL_PPIFC_GETPROP(BillFilt, Object2ID);}
void  DL6ICLS_PPFiltBill::put_Object2ID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, Object2ID);}
int32 DL6ICLS_PPFiltBill::get_PayerID() {IMPL_PPIFC_GETPROP(BillFilt, PayerID);}
void  DL6ICLS_PPFiltBill::put_PayerID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, PayerID);}
int32 DL6ICLS_PPFiltBill::get_AgentID() {IMPL_PPIFC_GETPROP(BillFilt, AgentID);}
void  DL6ICLS_PPFiltBill::put_AgentID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, AgentID);}
int32 DL6ICLS_PPFiltBill::get_CreatorID() {IMPL_PPIFC_GETPROP(BillFilt, CreatorID);}
void  DL6ICLS_PPFiltBill::put_CreatorID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, CreatorID);}
int32 DL6ICLS_PPFiltBill::get_StatusID() {IMPL_PPIFC_GETPROP(BillFilt, StatusID);}
void  DL6ICLS_PPFiltBill::put_StatusID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, StatusID);}
int32 DL6ICLS_PPFiltBill::get_AssocID() {IMPL_PPIFC_GETPROP(BillFilt, AssocID);}
void  DL6ICLS_PPFiltBill::put_AssocID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, AssocID);}
int32 DL6ICLS_PPFiltBill::get_PoolBillID() {IMPL_PPIFC_GETPROP(BillFilt, PoolBillID);}
void  DL6ICLS_PPFiltBill::put_PoolBillID(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, PoolBillID);}
PpyVBillFlags DL6ICLS_PPFiltBill::get_Flags() {IMPL_PPIFC_GETPROP_CAST(BillFilt, Flags, PpyVBillFlags);}
void  DL6ICLS_PPFiltBill::put_Flags(PpyVBillFlags value) {IMPL_PPIFC_PUTPROP(BillFilt, Flags);}
PpyVBillDenyFlags DL6ICLS_PPFiltBill::get_DenyFlags() {IMPL_PPIFC_GETPROP_CAST(BillFilt, DenyFlags, PpyVBillDenyFlags);}
void  DL6ICLS_PPFiltBill::put_DenyFlags(PpyVBillDenyFlags value) {IMPL_PPIFC_PUTPROP(BillFilt, DenyFlags);}
PpyVBillCCMode DL6ICLS_PPFiltBill::get_ClientCardMode() {IMPL_PPIFC_GETPROP_CAST(BillFilt, ClientCardMode, PpyVBillCCMode);}
void  DL6ICLS_PPFiltBill::put_ClientCardMode(PpyVBillCCMode value) {IMPL_PPIFC_PUTPROP(BillFilt, ClientCardMode);}
int32 DL6ICLS_PPFiltBill::get_Ft_STax() {IMPL_PPIFC_GETPROP(BillFilt, Ft_STax);}
void  DL6ICLS_PPFiltBill::put_Ft_STax(int32 value) {IMPL_PPIFC_PUTPROP_CAST(BillFilt, Ft_STax, int16);}
int32 DL6ICLS_PPFiltBill::get_Ft_ClosedOrder() {IMPL_PPIFC_GETPROP(BillFilt, Ft_ClosedOrder);}
void  DL6ICLS_PPFiltBill::put_Ft_ClosedOrder(int32 value) {IMPL_PPIFC_PUTPROP_CAST(BillFilt, Ft_ClosedOrder, int16);}

double DL6ICLS_PPFiltBill::get_AmtRangeMin()
{
	BillFilt * p_filt = (BillFilt*)ExtraPtr;
	return (p_filt) ? p_filt->AmtRange.low : 0;
}

void DL6ICLS_PPFiltBill::put_AmtRangeMin(double value)
{
	BillFilt * p_filt = (BillFilt*)ExtraPtr;
	if(p_filt)
		p_filt->AmtRange.low = value;
}

double DL6ICLS_PPFiltBill::get_AmtRangeMax()
{
	BillFilt * p_filt = (BillFilt*)ExtraPtr;
	return (p_filt) ? p_filt->AmtRange.upp : 0;
}

void DL6ICLS_PPFiltBill::put_AmtRangeMax(double value)
{
	BillFilt * p_filt = (BillFilt*)ExtraPtr;
	if(p_filt)
		p_filt->AmtRange.upp = value;
}

int32 DL6ICLS_PPFiltBill::get_Sel() {IMPL_PPIFC_GETPROP(BillFilt, Sel);}

void DL6ICLS_PPFiltBill::put_Sel(int32 value) {IMPL_PPIFC_PUTPROP(BillFilt, Sel);}

//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewBill::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltBill", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewBill::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(Bill);
}

int32 DL6ICLS_PPViewBill::InitIteration(int32 order)
{
	return ((PPViewBill *)ExtraPtr)->InitIteration((PPViewBill::IterOrder)order);
}

int32 DL6ICLS_PPViewBill::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyVI_Bill * p_item = (SPpyVI_Bill *)item;
	BillViewItem inner_item;
	if(((PPViewBill *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_BILL;
#define FLD(f) p_item->f = inner_item.f
		FLD(ID);
		p_item->Dt = (OleDate)inner_item.Dt;
		FLD(BillNo);
		p_item->DueDate = (OleDate)inner_item.DueDate;
		FLD(OpID);
		FLD(StatusID);
		FLD(UserID);
		FLD(MainOrgID);
		FLD(LocID);
		FLD(Object);
		FLD(Object2);
		FLD(CurID);
		FLD(CRate);
		FLD(Amount);
		FLD(LinkBillID);
		FLD(Flags);
		FLD(Flags2);
		FLD(SCardID);
		(temp_buf = inner_item.Code).CopyToOleStr(&p_item->Code);
		(temp_buf = inner_item.Memo).CopyToOleStr(&p_item->Memo);
		FLD(Debit);
		FLD(Credit);
		FLD(Saldo);
		p_item->LastPaymDate = (OleDate)inner_item.LastPaymDate;
#undef FLD
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewBill::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewBill::GetTotal(PPYVIEWTOTAL total)
{
	BillTotal inner_total;
	PPViewBill * p_v = (PPViewBill *)ExtraPtr;
	SPpyVT_Bill * p_total = (SPpyVT_Bill*)total;
	if(p_v && p_total && p_v->CalcTotal(&inner_total) > 0) {
		p_total->RecTag = 0; // @!
#define FLD(f) p_total->f = inner_total.f
		FLD(Count);
		FLD(Sum);
		FLD(Debt);
		FLD(InSaldo);
		FLD(Debit);
		FLD(Credit);
		FLD(OutSaldo);
#undef FLD
	}
	return !AppError;
}
//
// } PPViewBill
//
//
// PPQuotation {
//
DL6_IC_CONSTRUCTION_EXTRA(PPQuotation, DL6ICLS_PPQuotation_VTab, PPObjGoods);
//
// Interface IPapyrusQuot implementation
//
int32 DL6ICLS_PPQuotation::get_MatrixQuotKind()
{
	long   qk_id = ((PPObjGoods *)ExtraPtr)->GetConfig().MtxQkID;
	if(qk_id == 0) {
		PPSetError(PPERR_UNDEFGOODSMATRIX);
		AppError = 1;
	}
	return qk_id;
}

int32 DL6ICLS_PPQuotation::get_SupplDealQuotKind()
{
	long   qk_id = DS.GetConstTLA().SupplDealQuotKindID;
	if(qk_id == 0) {
		PPSetError(PPERR_UNDEFSUPPLDEAL);
		AppError = 1;
	}
	return qk_id;
}

int32 DL6ICLS_PPQuotation::get_SupplDealLowBoundQuotKind()
{
	long   qk_id = DS.GetConstTLA().SupplDevDnQuotKindID;
	if(qk_id == 0) {
		PPSetError(PPERR_UNDEFSUPPLDEALLOWDEV);
		AppError = 1;
	}
	return qk_id;
}

int32 DL6ICLS_PPQuotation::get_SupplDealUppBoundQuotKind()
{
	long   qk_id = DS.GetConstTLA().SupplDevUpQuotKindID;
	if(qk_id == 0) {
		PPSetError(PPERR_UNDEFSUPPLDEALUPPDEV);
		AppError = 1;
	}
	return qk_id;
}

static PPID TranslateQuotKind(PpyQuotCat cat, long inQuotKindID)
{
	PPID   qk_id = 0;
	if(cat == qcQuot) {
		qk_id = inQuotKindID;
	}
	else if(cat == qcMatrix) {
		PPObjGoods goods_obj;
		qk_id = goods_obj.GetConfig().MtxQkID;
		THROW_PP(qk_id, PPERR_UNDEFGOODSMATRIX);
		THROW_PP(!inQuotKindID || inQuotKindID == qk_id, PPERR_UNMATCHEDQUOTKIND);
	}
	else if(cat == qcSupplDeal) {
		qk_id = DS.GetConstTLA().SupplDealQuotKindID;
		THROW_PP(qk_id, PPERR_UNDEFSUPPLDEAL);
		THROW_PP(!inQuotKindID || inQuotKindID == qk_id, PPERR_UNMATCHEDQUOTKIND);
	}
	else {
		SString msg_buf;
		msg_buf.Cat(cat);
		CALLEXCEPT_PP_S(PPERR_INVQUOTCAT, msg_buf);
	}
	{
		PPObjQuotKind qk_obj;
		PPQuotKind qk_rec;
		THROW(qk_obj.Fetch(qk_id, &qk_rec) > 0);
	}
	CATCH
		qk_id = 0;
	ENDCATCH
	return qk_id;
}

static long QuotTagToFlags(long tag)
{
	long f = 0;
	if(tag == qvtPctCost)
		f |= PPQuot::fPctOnCost;
	else if(tag == qvtPctPrice)
		f |= PPQuot::fPctOnPrice;
	else if(tag == qvtPctAdd)
		f |= PPQuot::fPctOnAddition;
	else if(tag == qvtPctBase)
		f |= PPQuot::fPctOnBase;
	else if(tag == qvtDisabled)
		f |= PPQuot::fPctDisabled;
	return f;
}

static long QuotFlagsToTag(long flags)
{
	long   tag = 0;
	if(flags & PPQuot::fPctOnCost)
		tag = qvtPctCost;
	else if(flags & PPQuot::fPctOnPrice)
		tag = qvtPctPrice;
	else if(flags & PPQuot::fPctOnAddition)
		tag = qvtPctAdd;
	else if(flags & PPQuot::fPctOnBase)
		tag = qvtPctBase;
	else if(flags & PPQuot::fPctDisabled)
		tag = qvtDisabled;
	else
		tag = qvtAbs;
	return tag;
}

int32 DL6ICLS_PPQuotation::SetQuot(int32 goodsID, SPpyI_Quot * pQuot)
{
	PPQuot quot(goodsID);
	quot.ArID  = pQuot->ArID;
	quot.LocID = pQuot->LocID;
	quot.CurID = pQuot->CurID;
	quot.Quot  = pQuot->Val;
	quot.Flags = QuotTagToFlags(pQuot->Tag);
	if(quot.Flags & (PPQuot::fPctDisabled|PPQuot::fZero))
		quot.Quot = 0.0;
	THROW(quot.Kind = TranslateQuotKind(pQuot->Cat, pQuot->QuotKindID));
	THROW(((PPObjGoods *)ExtraPtr)->P_Tbl->SetQuot(quot, 1));
	CATCH
		AppError = 1;
	ENDCATCH
	return (AppError == 0);
}

SPpyI_Quot DL6ICLS_PPQuotation::GetQuot(SPpyI_QuotIdent* pIdent, int32 goodsID)
{
	SPpyI_Quot oq;
	PPQuot quot;
	QuotIdent qi(pIdent->LocID, 0, pIdent->CurID, pIdent->ArID);
	MEMSZERO(oq);
	THROW(qi.QuotKindID = TranslateQuotKind(pIdent->Cat, pIdent->QuotKindID));
	if(((PPObjGoods *)ExtraPtr)->P_Tbl->GetQuotNearest(goodsID, qi, &quot, 1)) {
		oq.LocID = quot.LocID;
		oq.QuotKindID = quot.Kind;
		oq.CurID = quot.CurID;
		oq.ArID = quot.ArID;
		oq.Val = quot.Quot;
		oq.Tag = (PpyQuotValTag)QuotFlagsToTag(quot.Flags);
	}
	CATCH
		AppError = 1;
	ENDCATCH
	return oq;
}

SString & DL6ICLS_PPQuotation::QuotValToString(SPpyI_Quot* pQuot)
{
	PPQuot quot;
	quot.ArID  = pQuot->ArID;
	quot.LocID = pQuot->LocID;
	quot.CurID = pQuot->CurID;
	quot.Quot = pQuot->Val;
	quot.Flags = QuotTagToFlags(pQuot->Tag);
	if(quot.Flags & (PPQuot::fPctDisabled|PPQuot::fZero))
		quot.Quot = 0.0;
	quot.PutValToStr(RetStrBuf);
	return RetStrBuf;
}

SPpyI_Quot DL6ICLS_PPQuotation::StringToQuotVal(SString & str)
{
	SPpyI_Quot oq;
	MEMSZERO(oq);
	PPQuot quot;
	quot.GetValFromStr(str);
	oq.Val = quot.Quot;
	oq.Tag = (PpyQuotValTag)QuotFlagsToTag(quot.Flags);
	return oq;
}

double DL6ICLS_PPQuotation::GetQuotVal(SPpyI_QuotIdent* pIdent, int32 goodsID, double cost, double price)
{
	double val = 0.0;
	QuotIdent qi(pIdent->LocID, 0, pIdent->CurID, pIdent->ArID);
	THROW(qi.QuotKindID = TranslateQuotKind(pIdent->Cat, pIdent->QuotKindID));
	THROW(((PPObjGoods *)ExtraPtr)->P_Tbl->GetQuot(goodsID, qi, cost, price, &val, 1));
	CATCH
		AppError = 1;
	ENDCATCH
	return val;
}

int32 DL6ICLS_PPQuotation::GetMatrix(int32 goodsID, int32 locID)
{
	return ((PPObjGoods *)ExtraPtr)->BelongToMatrix(goodsID, locID);
}

int32 DL6ICLS_PPQuotation::SetMatrix(int32 goodsID, int32 locID, int32 val)
{
	PPQuot quot(goodsID);
	long   qk_id = ((PPObjGoods *)ExtraPtr)->GetConfig().MtxQkID;
	THROW_PP(qk_id, PPERR_UNDEFGOODSMATRIX);
	quot.LocID = locID;
	quot.Kind = qk_id;
	if(val > 0)
		quot.Quot = 1.0;
	else if(val < 0)
		quot.Quot = -1.0;
	THROW(((PPObjGoods *)ExtraPtr)->P_Tbl->SetQuot(quot, 1));
	CATCH
		AppError = 1;
	ENDCATCH
	return BIN(AppError == 0);
}

SPpyI_SupplDeal DL6ICLS_PPQuotation::GetSupplDeal(int32 goodsID, int32 locID, int32 curID, int32 supplID)
{
	SPpyI_SupplDeal od;
	MEMSZERO(od);
	QuotIdent qi(locID, 0, curID, supplID);
	PPSupplDeal sd;
	((PPObjGoods *)ExtraPtr)->GetSupplDeal(goodsID, qi, &sd, 1);
	od.Val = sd.Cost;
	od.LowBound = sd.DnDev;
	od.UppBound = sd.UpDev;
	od.Disabled = sd.IsDisabled;
	return od;
}

int32 DL6ICLS_PPQuotation::SetSupplDeal(int32 goodsID, int32 locID, int32 curID, int32 supplID, SPpyI_SupplDeal* pDeal)
{
	PPObjGoods * p_obj = (PPObjGoods*)ExtraPtr;
	if(p_obj && pDeal) {
		PPSupplDeal sd;
		QuotIdent qi(locID, 0, curID, supplID);
		MEMSZERO(sd);
		sd.DnDev      = pDeal->LowBound;
		sd.UpDev      = pDeal->UppBound;
		sd.Cost       = pDeal->Val;
		sd.IsDisabled = pDeal->Disabled;
	 	THROW(p_obj->SetSupplDeal(goodsID, qi, &sd, 1));
	}
	CATCH
		AppError = 1;
	ENDCATCH
	return BIN(AppError == 0);
}
//
// } PPQuotation
//
//
// PPViewGoodsOpAnlz {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewGoodsOpAnlz, DL6ICLS_PPViewGoodsOpAnlz_VTab, PPViewGoodsOpAnalyze);
//
// Interface IPpyFilt_GoodsOpAnlz implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltGoodsOpAnlz, DL6ICLS_PPFiltGoodsOpAnlz_VTab, GoodsOpAnalyzeFilt);
//
// Interface IPpyFilt_GoodsOpAnlz implementation
//
void DL6ICLS_PPFiltGoodsOpAnlz::SetPeriod(LDATE low, LDATE upp)
{
	((GoodsOpAnalyzeFilt *)ExtraPtr)->Period.Set(low, upp);
}

void DL6ICLS_PPFiltGoodsOpAnlz::SetCmpPeriod(LDATE low, LDATE upp)
{
	((GoodsOpAnalyzeFilt *)ExtraPtr)->CmpPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltGoodsOpAnlz::AddLocationID(int32 id)
{
	GoodsOpAnalyzeFilt * p_filt = ((GoodsOpAnalyzeFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->LocList.Add(id);
}

void DL6ICLS_PPFiltGoodsOpAnlz::FreeLocList()
{
	GoodsOpAnalyzeFilt * p_filt = ((GoodsOpAnalyzeFilt*)ExtraPtr);
	if(p_filt)
		p_filt->LocList.FreeAll();
}

void DL6ICLS_PPFiltGoodsOpAnlz::AddBillID(int32 id)
{
	GoodsOpAnalyzeFilt * p_filt = ((GoodsOpAnalyzeFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->BillList.Add(id);
}

void DL6ICLS_PPFiltGoodsOpAnlz::FreeBillList()
{
	GoodsOpAnalyzeFilt * p_filt = ((GoodsOpAnalyzeFilt*)ExtraPtr);
	if(p_filt)
		p_filt->BillList.FreeAll();
}

SDateRange DL6ICLS_PPFiltGoodsOpAnlz::get_Period()
	{ return DateRangeToOleDateRange(((GoodsOpAnalyzeFilt *)ExtraPtr)->Period); }

SDateRange DL6ICLS_PPFiltGoodsOpAnlz::get_CmpPeriod()
	{ return DateRangeToOleDateRange(((GoodsOpAnalyzeFilt *)ExtraPtr)->CmpPeriod); }


LDATE DL6ICLS_PPFiltGoodsOpAnlz::get_CmpRestCalcDate() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, CmpRestCalcDate);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_CmpRestCalcDate(LDATE value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, CmpRestCalcDate);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_OpID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, OpID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_OpID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, OpID);}

PpyVGoodsOpAnlzOpGrp DL6ICLS_PPFiltGoodsOpAnlz::get_OpGrpID() {IMPL_PPIFC_GETPROP_CAST(GoodsOpAnalyzeFilt, OpGrpID, PpyVGoodsOpAnlzOpGrp);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_OpGrpID(PpyVGoodsOpAnlzOpGrp value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, OpGrpID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_AccSheetID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, AccSheetID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_AccSheetID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, AccSheetID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_ObjectID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, ObjectID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_ObjectID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, ObjectID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_ObjCityID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, ObjCityID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_ObjCityID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, ObjCityID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_SupplID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, SupplID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_SupplID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, SupplID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_AgentID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, AgentID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_AgentID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, AgentID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_SupplAgentID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, SupplAgentID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_SupplAgentID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, SupplAgentID);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_GoodsGrpID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, GoodsGrpID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_GoodsGrpID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, GoodsGrpID);}

PpyVGoodsOpAnlzFlags DL6ICLS_PPFiltGoodsOpAnlz::get_Flags() {IMPL_PPIFC_GETPROP_CAST(GoodsOpAnalyzeFilt, Flags, PpyVGoodsOpAnlzFlags);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_Flags(PpyVGoodsOpAnlzFlags value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, Flags);}

PpyVSubstGrpGoods DL6ICLS_PPFiltGoodsOpAnlz::get_Sgg() {IMPL_PPIFC_GETPROP_CAST(GoodsOpAnalyzeFilt, Sgg, PpyVSubstGrpGoods);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_Sgg(PpyVSubstGrpGoods value) {((GoodsOpAnalyzeFilt*)ExtraPtr)->Sgg = (SubstGrpGoods)value;}

LDATE DL6ICLS_PPFiltGoodsOpAnlz::get_RestCalcDate() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, RestCalcDate);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_RestCalcDate(LDATE value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, RestCalcDate);}

int32 DL6ICLS_PPFiltGoodsOpAnlz::get_QuotKindID() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, QuotKindID);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_QuotKindID(int32 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, QuotKindID);}

int16 DL6ICLS_PPFiltGoodsOpAnlz::get_ABCAnlzGroup() {IMPL_PPIFC_GETPROP(GoodsOpAnalyzeFilt, ABCAnlzGroup);}
void  DL6ICLS_PPFiltGoodsOpAnlz::put_ABCAnlzGroup(int16 value) {IMPL_PPIFC_PUTPROP(GoodsOpAnalyzeFilt, ABCAnlzGroup);}

double DL6ICLS_PPFiltGoodsOpAnlz::get_A() { return ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[0]; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_A(double value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[0] = value; }

double DL6ICLS_PPFiltGoodsOpAnlz::get_B() { return ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[1]; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_B(double value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[1] = value; }

double DL6ICLS_PPFiltGoodsOpAnlz::get_C() { return ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[2]; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_C(double value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[2] = value; }

double DL6ICLS_PPFiltGoodsOpAnlz::get_D() { return ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[3]; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_D(double value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[3] = value; }

double DL6ICLS_PPFiltGoodsOpAnlz::get_E() { return ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[4]; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_E(double value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GrpFract[4] = value; }

PpyVGoodsOpAnlzABCGrp DL6ICLS_PPFiltGoodsOpAnlz::get_ABCGroupBy() { return (PpyVGoodsOpAnlzABCGrp)((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GroupBy; }
void  DL6ICLS_PPFiltGoodsOpAnlz::put_ABCGroupBy(PpyVGoodsOpAnlzABCGrp value) { ((GoodsOpAnalyzeFilt *)ExtraPtr)->ABCAnlz.GroupBy = value; }
//
// Interface IPapyrusView implementation
//
IUnknown * DL6ICLS_PPViewGoodsOpAnlz::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltGoodsOpAnlz", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewGoodsOpAnlz::Init(IUnknown* pFilt)
{
	IPpyFilt_GoodsOpAnlz * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_GoodsOpAnlz", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(((PPViewGoodsOpAnalyze *)ExtraPtr)->Init_((const GoodsOpAnalyzeFilt *)GetExtraPtrByInterface(p_ifc_filt)));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewGoodsOpAnlz::InitIteration(int32 order)
{
	return ((PPViewGoodsOpAnalyze *)ExtraPtr)->InitIteration((PPViewGoodsOpAnalyze::IterOrder)order);
}

int32 DL6ICLS_PPViewGoodsOpAnlz::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyVI_GoodsOpAnlz * p_item = (SPpyVI_GoodsOpAnlz *)item;
	GoodsOpAnalyzeViewItem inner_item;
	if(((PPViewGoodsOpAnalyze *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_GOODSOPANALYZE;
#define FLD(f) p_item->f = inner_item.f
		FLD(LocID);
		p_item->InOutTag = (long)inner_item.InOutTag;
		FLD(GoodsID);
		FLD(GoodsGrpID);
		FLD(SubstArID);
		FLD(SubstPsnID);
		FLD(SubstLocID);
		FLD(UnitPerPack);
		FLD(PhQtty);
		FLD(OldCost);
		FLD(PlanQtty);
		FLD(OldPrice);
		FLD(PlanSumPrice);
		FLD(Cost);
		FLD(Price);
		FLD(PctVal);
		p_item->Qtty.Val         = inner_item.Qtty.Val;
		p_item->Qtty.Cmp         = inner_item.Qtty.Cm;
		p_item->SumCost.Val      = inner_item.SumCost.Val;
		p_item->SumCost.Cmp      = inner_item.SumCost.Cm;
		p_item->SumPrice.Val     = inner_item.SumPrice.Val;
		p_item->SumPrice.Cmp     = inner_item.SumPrice.Cm;
		p_item->Income.Val       = inner_item.Income.Val;
		p_item->Income.Cmp       = inner_item.Income.Cm;
		p_item->Rest.Val         = inner_item.Rest.Val;
		p_item->Rest.Cmp         = inner_item.Rest.Cm;
		p_item->RestCostSum.Val  = inner_item.RestCostSum.Val;
		p_item->RestCostSum.Cmp  = inner_item.RestCostSum.Cm;
		p_item->RestPriceSum.Val = inner_item.RestPriceSum.Val;
		p_item->RestPriceSum.Cmp = inner_item.RestPriceSum.Cm;
		(temp_buf = inner_item.GoodsName).CopyToOleStr(&p_item->GoodsName);
#undef FLD
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewGoodsOpAnlz::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewGoodsOpAnlz::GetTotal(PPYVIEWTOTAL total)
{
	GoodsOpAnalyzeTotal inner_total;
	PPViewGoodsOpAnalyze * p_v = (PPViewGoodsOpAnalyze *)ExtraPtr;
	SPpyVT_GoodsOpAnlz * p_total = (SPpyVT_GoodsOpAnlz*)total;
	if(p_v && p_total && p_v->CalcTotal(&inner_total) > 0) {
		p_total->RecTag = 0; // @!
#define FLD(f) p_total->f = inner_total.f
		FLD(Count);
		FLD(Qtty);
		FLD(PhQtty);
		FLD(Cost);
		FLD(Price);
		FLD(Income);
		FLD(RestCost);
		FLD(RestPrice);
		FLD(PlanQtty);
		FLD(PlanSum);
		FLD(InCount);
		FLD(InQtty);
		FLD(InPhQtty);
		FLD(InCost);
		FLD(InPrice);
		FLD(InIncome);
#undef FLD
	}
	return !AppError;
}
//
// } PPViewGoodsOpAnlz
//
DL6_IC_CONSTRUCTOR(PPObjTransport, DL6ICLS_PPObjTransport_VTab)
{
	PPObjTransport * p_e = new PPObjTransport;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjTransport)
{
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	ZDELETE(p_e);
}
//
// Interface IPapyrusObject implementation
//
static void FillTransportRec(const PPTransport * pInner, SPpyO_Transport * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	pOuter->RecTag = ppoTransport;
	FLD(ID);
	pOuter->TrType = (PpyOTransportType)pInner->TrType;
	FLD(TrModelID);
	FLD(OwnerID);
	FLD(CountryID);
	FLD(CaptainID);
	FLD(Capacity);
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	(temp_buf = pInner->TrailerCode).CopyToOleStr(&pOuter->TrailerCode);
	#undef FLD
}

static int AcceptTransportRec(const SPpyO_Transport * pOuter, PPTransport * pInner)
{
	int    ok = 1;
	SString temp_buf;
	THROW_PP_S(pOuter->RecTag == ppoTransport, PPERR_INVSTRUCTAG, "ppoTransport");
	#define FLD(f) pInner->f = pOuter->f
	FLD(ID);
	FLD(TrType);
	FLD(TrModelID);
	FLD(OwnerID);
	FLD(CountryID);
	FLD(CaptainID);
	pInner->Capacity = (long)pOuter->Capacity;
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Name, sizeof(pInner->Name));
	temp_buf.CopyFromOleStr(pOuter->Code).CopyTo(pInner->Code, sizeof(pInner->Code));
	temp_buf.CopyFromOleStr(pOuter->TrailerCode).CopyTo(pInner->TrailerCode, sizeof(pInner->TrailerCode));
	#undef FLD
	CATCHZOK
	return ok;
}

int32 DL6ICLS_PPObjTransport::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	if(p_e) {
		PPTransport inner_rec;
		if((ok = p_e->Get(id, &inner_rec)) > 0) {
			FillTransportRec(&inner_rec, (SPpyO_Transport *)rec);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjTransport::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = -1;
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	if(p_e) {
		PPTransport inner_rec;
		PPID   id = 0;
		if(p_e->SearchByName(text, &id, 0) > 0 && p_e->Get(id, &inner_rec) > 0) {
			FillTransportRec(&inner_rec, (SPpyO_Transport *)rec);
			ok = 1;
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjTransport::GetName(int32 id)
{
	int    ok = 0;
	char   name_buf[64];
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	memzero(name_buf, sizeof(name_buf));
	ok = p_e ? p_e->GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjTransport::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjTransport::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 1;
	PPID   id = 0;
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	PPTransport tr_rec;
	THROW_INVARG(p_e);
	THROW(AcceptTransportRec((SPpyO_Transport *)pRec, &tr_rec));
	tr_rec.ID = 0;
	THROW(p_e->Put(&id, &tr_rec, (flags & 1) ? 0 : 1));
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int32 DL6ICLS_PPObjTransport::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPObjTransport * p_e = (PPObjTransport *)ExtraPtr;
	PPTransport tr_rec;
	THROW_INVARG(p_e);
	THROW(AcceptTransportRec((SPpyO_Transport *)rec, &tr_rec));
	THROW(p_e->Put(&id, &tr_rec, (flags & 1) ? 0 : 1));
	CATCHZOK
	return ok;
}
//
//
//
DL6_IC_CONSTRUCTOR(PPObjProcessor, DL6ICLS_PPObjProcessor_VTab)
{
	PPObjProcessor * p_e = new PPObjProcessor;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjProcessor)
{
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	ZDELETE(p_e);
}
//
// Interface IPapyrusObject implementation
//
static void FillProcessorRec(const PPProcessorPacket * pInner, SPpyO_Processor * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->Rec.f
	pOuter->RecTag = ppoProcessor;
	FLD(ID);
	FLD(ParentID);
	pOuter->Kind = (PpyOProcessorKind)pInner->Rec.Kind;
	FLD(LocID);
	FLD(TimeUnitID);
	pOuter->Flags = (PpyOProcessorFlags)pInner->Rec.Flags;
	FLD(LinkObjType);
	FLD(LinkObjID);
	FLD(WrOffOpID);
	FLD(WrOffArID);
	FLD(SuperSessTiming);
	FLD(RestAltGrpID);
	FLD(PrinterID);
	FLD(LabelCount);
	(temp_buf = pInner->Rec.Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Rec.Code).CopyToOleStr(&pOuter->Code);
	#undef FLD
}

static int AcceptProcessorRec(const SPpyO_Processor * pOuter, PPProcessorPacket * pInner)
{
	int    ok = 1;
	SString temp_buf;
	THROW_PP_S(pOuter->RecTag == ppoProcessor, PPERR_INVSTRUCTAG, "ppoProcessor");
	#define FLD(f) pInner->Rec.f = pOuter->f
	FLD(ID);
	FLD(ParentID);
	FLD(Kind);
	FLD(LocID);
	FLD(TimeUnitID);
	FLD(Flags);
	FLD(LinkObjType);
	FLD(LinkObjID);
	FLD(WrOffOpID);
	FLD(WrOffArID);
	FLD(SuperSessTiming);
	FLD(RestAltGrpID);
	FLD(PrinterID);
	pInner->Rec.LabelCount = (int16)pOuter->LabelCount;
	temp_buf.CopyFromOleStr(pOuter->Name).CopyTo(pInner->Rec.Name, sizeof(pInner->Rec.Name));
	temp_buf.CopyFromOleStr(pOuter->Code).CopyTo(pInner->Rec.Code, sizeof(pInner->Rec.Code));
	#undef FLD
	CATCHZOK
	return ok;
}

int32 DL6ICLS_PPObjProcessor::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	if(p_e) {
		PPProcessorPacket inner_rec;
		if((ok = p_e->GetPacket(id, &inner_rec)) > 0) {
			FillProcessorRec(&inner_rec, (SPpyO_Processor *)rec);
		}
	}
	return ok;
}
//
// ARG(extraParam IN): PpyOProcessorKind Вид процессора. Если extraParam == 0, то функция будет искать
//   сначала собственно процессоры с указанным именем, а затем группы процессоров.
//   Используется только для kind == 0 (поиск по миени, но не по коду).
//
int32 DL6ICLS_PPObjProcessor::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	if(p_e) {
		PPID   id = 0;
		ProcessorTbl::Rec __rec;
		PPProcessorPacket inner_pack;
		if(kind == 0) {
			if((ok = p_e->SearchByName(extraParam, text, &id, &__rec)) > 0) {
				if(p_e->GetPacket(id, &inner_pack) > 0) {
					FillProcessorRec(&inner_pack, (SPpyO_Processor *)rec);
				}
			}
		}
		else if(kind == 1) {
			if((ok = p_e->SearchByCode(text, &id, &__rec)) > 0) {
				if(p_e->GetPacket(id, &inner_pack) > 0) {
					FillProcessorRec(&inner_pack, (SPpyO_Processor *)rec);
				}
			}
		}
	}
	return ok;
}

SString & DL6ICLS_PPObjProcessor::GetName(int32 id)
{
	int    ok = 0;
	char   name_buf[64];
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	memzero(name_buf, sizeof(name_buf));
	ok = p_e ? p_e->GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjProcessor::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjProcessor::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 1;
	PPID   id = 0;
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	PPProcessorPacket inner_pack;
	THROW_INVARG(p_e);
	THROW(AcceptProcessorRec((SPpyO_Processor *)pRec, &inner_pack));
	inner_pack.Rec.ID = 0;
	THROW(p_e->PutPacket(&id, &inner_pack, (flags & 1) ? 0 : 1));
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int32 DL6ICLS_PPObjProcessor::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPObjProcessor * p_e = (PPObjProcessor *)ExtraPtr;
	PPProcessorPacket inner_pack;
	THROW_INVARG(p_e);
	THROW(AcceptProcessorRec((SPpyO_Processor *)rec, &inner_pack));
	THROW(p_e->PutPacket(&id, &inner_pack, (flags & 1) ? 0 : 1));
	CATCHZOK
	return ok;
}
//
//
//
DL6_IC_CONSTRUCTOR(PPObjTSession, DL6ICLS_PPObjTSession_VTab)
{
	PPObjTSession * p_e = new PPObjTSession;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjTSession)
{
	PPObjTSession * p_e = (PPObjTSession *)ExtraPtr;
	ZDELETE(p_e);
}
//
// Interface IPapyrusObject implementation
//
static void FillTSessionRec(const TSessionTbl::Rec * pInner, SPpyO_TSession * pOuter)
{
	SString temp_buf;
	#define FLD(f) pOuter->f = pInner->f
	pOuter->RecTag = ppoTSession;
	FLD(ID);
	FLD(ParentID);
	FLD(Num);
	FLD(TechID);
	FLD(PrcID);
	pOuter->StDt = (OleDate)pInner->StDt;
	pOuter->StTm = (OleDate)pInner->StTm;
	pOuter->FinDt = (OleDate)pInner->FinDt;
	pOuter->FinTm = (OleDate)pInner->FinTm;
	FLD(Incomplete);
	FLD(Status);
	pOuter->Flags = (PpyOTSessionFlags)pInner->Flags;
	FLD(ArID);
	FLD(Ar2ID);
	FLD(PlannedTiming);
	FLD(PlannedQtty);
	FLD(ActQtty);
	FLD(OrderLotID);
	FLD(PrevSessID);
	FLD(Amount);
	FLD(LinkBillID);
	FLD(SCardID);
	FLD(ToolingTime);
	(temp_buf = pInner->Memo).CopyToOleStr(&pOuter->Memo);
	#undef FLD
}

static int AcceptTSessionRec(const SPpyO_TSession * pOuter, TSessionTbl::Rec * pInner)
{
	int    ok = 1;
	SString temp_buf;
	THROW_PP_S(pOuter->RecTag == ppoTSession, PPERR_INVSTRUCTAG, "ppoTSession");
	#define FLD(f) pInner->f = pOuter->f
	FLD(ID);
	FLD(ParentID);
	FLD(Num);
	FLD(TechID);
	FLD(PrcID);
	FLD(StDt);
	FLD(StTm);
	FLD(FinDt);
	FLD(FinTm);
	pInner->Incomplete = (int16)pOuter->Incomplete;
	pInner->Status = (int16)pOuter->Status;
	FLD(Flags);
	FLD(ArID);
	FLD(Ar2ID);
	FLD(PlannedTiming);
	FLD(PlannedQtty);
	FLD(ActQtty);
	FLD(OrderLotID);
	FLD(PrevSessID);
	FLD(Amount);
	FLD(LinkBillID);
	FLD(SCardID);
	FLD(ToolingTime);
	temp_buf.CopyFromOleStr(pOuter->Memo).CopyTo(pInner->Memo, sizeof(pInner->Memo));
	#undef FLD
	CATCHZOK
	return ok;
}

int32 DL6ICLS_PPObjTSession::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjTSession * p_e = (PPObjTSession *)ExtraPtr;
	if(p_e) {
		TSessionTbl::Rec inner_rec;
		if((ok = p_e->Search(id, &inner_rec)) > 0) {
			FillTSessionRec(&inner_rec, (SPpyO_TSession *)rec);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjTSession::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return -1;
}

SString & DL6ICLS_PPObjTSession::GetName(int32 id)
{
	int    ok = 0;
	PPObjTSession * p_e = (PPObjTSession *)ExtraPtr;
	char   name_buf[64];
	memzero(name_buf, sizeof(name_buf));
	ok = p_e ? p_e->GetName(id, name_buf, sizeof(name_buf)) : 0;
	return (RetStrBuf = name_buf);
}

IStrAssocList* DL6ICLS_PPObjTSession::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	if(!p)
		AppError = 1;
	return p;
}

int32 DL6ICLS_PPObjTSession::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	int    ok = 1;
	PPID   id = 0;
	PPObjTSession * p_e = (PPObjTSession *)ExtraPtr;
	TSessionTbl::Rec inner_rec;
	THROW_INVARG(p_e);
	THROW(AcceptTSessionRec((SPpyO_TSession *)pRec, &inner_rec));
	inner_rec.ID = 0;
	THROW(p_e->PutRec(&id, &inner_rec, (flags & 1) ? 0 : 1));
	CATCHZOK
	ASSIGN_PTR(pID, id);
	return ok;
}

int32 DL6ICLS_PPObjTSession::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	int    ok = 1;
	PPObjTSession * p_e = (PPObjTSession *)ExtraPtr;
	TSessionTbl::Rec inner_rec;
	THROW_INVARG(p_e);
	THROW(AcceptTSessionRec((SPpyO_TSession *)rec, &inner_rec));
	THROW(p_e->PutRec(&id, &inner_rec, (flags & 1) ? 0 : 1));
	CATCHZOK
	return ok;
}
//
// PPViewPrjTask
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltPrjTask, DL6ICLS_PPFiltPrjTask_VTab, PrjTaskFilt);
//
// Interface IPpyFilt_PrjTask implementation
//
void DL6ICLS_PPFiltPrjTask::SetPeriod(LDATE low, LDATE upp)
	{ ((PrjTaskFilt *)ExtraPtr)->Period.Set(low, upp); }

void DL6ICLS_PPFiltPrjTask::SetStartPeriod(LDATE low, LDATE upp)
	{ ((PrjTaskFilt *)ExtraPtr)->StartPeriod.Set(low, upp); }

void DL6ICLS_PPFiltPrjTask::SetEstFinishPeriod(LDATE low, LDATE upp)
	{ ((PrjTaskFilt *)ExtraPtr)->EstFinishPeriod.Set(low, upp); }

void DL6ICLS_PPFiltPrjTask::SetFinishPeriod(LDATE low, LDATE upp)
	{ ((PrjTaskFilt *)ExtraPtr)->FinishPeriod.Set(low, upp); }

void DL6ICLS_PPFiltPrjTask::IncludeStatus(PpyOPrjTaskStatus status)
	{ ((PrjTaskFilt *)ExtraPtr)->IncludeStatus((long)status); }

void DL6ICLS_PPFiltPrjTask::ExcludeStatus(PpyOPrjTaskStatus status)
	{ ((PrjTaskFilt *)ExtraPtr)->ExcludeStatus((long)status); }

void DL6ICLS_PPFiltPrjTask::IncludePriority(PpyOPrjTaskPriority priority)
	{ ((PrjTaskFilt *)ExtraPtr)->IncludePrior((long)priority); }

void DL6ICLS_PPFiltPrjTask::ExcludePriority(PpyOPrjTaskPriority priority)
	{ ((PrjTaskFilt *)ExtraPtr)->ExcludePrior((long)priority); }

PpyVPrjTaskTabType DL6ICLS_PPFiltPrjTask::get_TabType() { return (PpyVPrjTaskTabType)((PrjTaskFilt*)ExtraPtr)->TabType; }

void DL6ICLS_PPFiltPrjTask::put_TabType(PpyVPrjTaskTabType value) { IMPL_PPIFC_PUTPROP_CAST(PrjTaskFilt, TabType, PrjTaskFilt::EnumTabType); }

PpyVPrjTaskTabParam DL6ICLS_PPFiltPrjTask::get_TabParam() { return (PpyVPrjTaskTabParam)((PrjTaskFilt*)ExtraPtr)->TabParam; }

void DL6ICLS_PPFiltPrjTask::put_TabParam(PpyVPrjTaskTabParam value) { IMPL_PPIFC_PUTPROP_CAST(PrjTaskFilt, TabParam, PrjTaskFilt::EnumTabParam); }

PpyOPrjTaskKind DL6ICLS_PPFiltPrjTask::get_Kind() { return (PpyOPrjTaskKind)((PrjTaskFilt*)ExtraPtr)->Kind; }

void DL6ICLS_PPFiltPrjTask::put_Kind(PpyOPrjTaskKind value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, Kind); }

PpyVPrjTaskSortOrder DL6ICLS_PPFiltPrjTask::get_Order() { return (PpyVPrjTaskSortOrder)((PrjTaskFilt*)ExtraPtr)->Order; }

void DL6ICLS_PPFiltPrjTask::put_Order(PpyVPrjTaskSortOrder value) { IMPL_PPIFC_PUTPROP_CAST(PrjTaskFilt, Order, PrjTaskFilt::EnumOrder); }

int32 DL6ICLS_PPFiltPrjTask::get_ProjectID() {return ((PrjTaskFilt*)ExtraPtr)->ProjectID; }

void DL6ICLS_PPFiltPrjTask::put_ProjectID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, ProjectID); }

int32 DL6ICLS_PPFiltPrjTask::get_ClientID()  {return ((PrjTaskFilt*)ExtraPtr)->ClientID; }

void DL6ICLS_PPFiltPrjTask::put_ClientID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, ClientID); }

int32 DL6ICLS_PPFiltPrjTask::get_EmployerID()  {return ((PrjTaskFilt*)ExtraPtr)->EmployerID; }

void DL6ICLS_PPFiltPrjTask::put_EmployerID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, EmployerID); }

int32 DL6ICLS_PPFiltPrjTask::get_TemplateID() {return ((PrjTaskFilt*)ExtraPtr)->TemplateID; }

void DL6ICLS_PPFiltPrjTask::put_TemplateID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, TemplateID); }

int32 DL6ICLS_PPFiltPrjTask::get_CreatorID() {return ((PrjTaskFilt*)ExtraPtr)->CreatorID; }

void DL6ICLS_PPFiltPrjTask::put_CreatorID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, CreatorID); }

int32 DL6ICLS_PPFiltPrjTask::get_CliCityID() {return ((PrjTaskFilt*)ExtraPtr)->CliCityID; }

void DL6ICLS_PPFiltPrjTask::put_CliCityID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, CliCityID); }

int32 DL6ICLS_PPFiltPrjTask::get_LinkTaskID() {return ((PrjTaskFilt*)ExtraPtr)->LinkTaskID; }

void DL6ICLS_PPFiltPrjTask::put_LinkTaskID(int32 value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, LinkTaskID); }

SDateRange DL6ICLS_PPFiltPrjTask::get_Period()
	{ return DateRangeToOleDateRange(((PrjTaskFilt *)ExtraPtr)->Period); }

SDateRange DL6ICLS_PPFiltPrjTask::get_StartPeriod()
	{ return DateRangeToOleDateRange(((PrjTaskFilt *)ExtraPtr)->StartPeriod); }

SDateRange DL6ICLS_PPFiltPrjTask::get_EstFinishPeriod()
	{ return DateRangeToOleDateRange(((PrjTaskFilt *)ExtraPtr)->EstFinishPeriod); }

SDateRange DL6ICLS_PPFiltPrjTask::get_FinishPeriod()
	{ return DateRangeToOleDateRange(((PrjTaskFilt *)ExtraPtr)->FinishPeriod); }

PpyVPrjTaskFlags DL6ICLS_PPFiltPrjTask::get_Flags() { return (PpyVPrjTaskFlags)((PrjTaskFilt*)ExtraPtr)->Flags; }

void DL6ICLS_PPFiltPrjTask::put_Flags(PpyVPrjTaskFlags value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, Flags); }

LTIME DL6ICLS_PPFiltPrjTask::get_StartTmPeriodBeg() { IMPL_PPIFC_GETPROP(PrjTaskFilt, StartTmPeriodBeg); }

void DL6ICLS_PPFiltPrjTask::put_StartTmPeriodBeg(LTIME value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, StartTmPeriodBeg); }

LTIME DL6ICLS_PPFiltPrjTask::get_StartTmPeriodEnd() { IMPL_PPIFC_GETPROP(PrjTaskFilt, StartTmPeriodEnd); }

void DL6ICLS_PPFiltPrjTask::put_StartTmPeriodEnd(LTIME value) { IMPL_PPIFC_PUTPROP(PrjTaskFilt, StartTmPeriodEnd); }

PpyVSubstGrpDate DL6ICLS_PPFiltPrjTask::get_Sgd() {return (PpyVSubstGrpDate)((PrjTaskFilt*)ExtraPtr)->Sgd; }

void DL6ICLS_PPFiltPrjTask::put_Sgd(PpyVSubstGrpDate value) { IMPL_PPIFC_PUTPROP_CAST(PrjTaskFilt, Sgd, SubstGrpDate); }
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewPrjTask, DL6ICLS_PPViewPrjTask_VTab, PPViewPrjTask);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewPrjTask::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltPrjTask", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewPrjTask::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(PrjTask);
}

int32 DL6ICLS_PPViewPrjTask::InitIteration(int32 order)
{
	return ((PPViewPrjTask*)ExtraPtr)->InitIteration();
}

int32 DL6ICLS_PPViewPrjTask::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_PrjTask * p_item = (SPpyVI_PrjTask *)item;
	PrjTaskViewItem inner_item;
	if(((PPViewPrjTask *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		#define FLD(f) p_item->f = inner_item.f
			FLD(ID);
			FLD(ProjectID);
			FLD(CreatorID);
			FLD(GroupID);
			FLD(EmployerID);
			FLD(ClientID);
			FLD(TemplateID);
			FLD(DrPrd);
			FLD(DrKind);
			FLD(DrDetail);
			FLD(DlvrAddrID);
			FLD(LinkTaskID);
			FLD(Amount);
			FLD(OpenCount);
			FLD(BillArID);
		#undef FLD
		p_item->RecTag      = ppvPrjTask;
		p_item->Priority    = (PpyOPrjTaskPriority)inner_item.Priority;
		p_item->Status      = (PpyOPrjTaskStatus)inner_item.Status;
		p_item->Flags       = (PpyOPrjTaskFlags)inner_item.Flags;
		p_item->Kind        = (PpyOPrjTaskKind)inner_item.Kind;
		p_item->Dt          = (OleDate)inner_item.Dt;
		p_item->Tm          = (OleDate)inner_item.Tm;
		p_item->StartDt     = (OleDate)inner_item.StartDt;
		p_item->StartTm     = (OleDate)inner_item.StartTm;
		p_item->EstFinishDt = (OleDate)inner_item.EstFinishDt;
		p_item->EstFinishTm = (OleDate)inner_item.EstFinishTm;
		p_item->FinishDt    = (OleDate)inner_item.FinishDt;
		p_item->FinishTm    = (OleDate)inner_item.FinishTm;
		(temp_buf = inner_item.Code).CopyToOleStr(&p_item->Code);
		(temp_buf = inner_item.Descr).CopyToOleStr(&p_item->Descr);
		(temp_buf = inner_item.Memo).CopyToOleStr(&p_item->Memo);
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewPrjTask::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewPrjTask::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
// PPViewProject
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltProject, DL6ICLS_PPFiltProject_VTab, ProjectFilt);
//
// Interface IPpyFilt_Project implementation
//
void DL6ICLS_PPFiltProject::SetStartPeriod(LDATE low, LDATE upp)
	{ ((ProjectFilt *)ExtraPtr)->StartPeriod.Set(low, upp); }

void DL6ICLS_PPFiltProject::SetEstFinishPeriod(LDATE low, LDATE upp)
	{ ((ProjectFilt *)ExtraPtr)->EstFinishPeriod.Set(low, upp); }

int32 DL6ICLS_PPFiltProject::get_ParentID() { return ((ProjectFilt*)ExtraPtr)->ParentID; }

void DL6ICLS_PPFiltProject::put_ParentID(int32 value) { IMPL_PPIFC_PUTPROP(ProjectFilt, ParentID); }

int32 DL6ICLS_PPFiltProject::get_ClientID() { return ((ProjectFilt*)ExtraPtr)->ClientID; }

void DL6ICLS_PPFiltProject::put_ClientID(int32 value) { IMPL_PPIFC_PUTPROP(ProjectFilt, ClientID); }

int32 DL6ICLS_PPFiltProject::get_MngrID() { return ((ProjectFilt*)ExtraPtr)->MngrID; }

void DL6ICLS_PPFiltProject::put_MngrID(int32 value) { IMPL_PPIFC_PUTPROP(ProjectFilt, MngrID); }

PpyVProjectFlags DL6ICLS_PPFiltProject::get_Flags() { return (PpyVProjectFlags)((ProjectFilt*)ExtraPtr)->Flags; }

void DL6ICLS_PPFiltProject::put_Flags(PpyVProjectFlags value) { IMPL_PPIFC_PUTPROP(ProjectFilt, Flags); }

PpyVProjectSortOrder DL6ICLS_PPFiltProject::get_SortOrd() { return (PpyVProjectSortOrder)((ProjectFilt*)ExtraPtr)->SortOrd; }

void DL6ICLS_PPFiltProject::put_SortOrd(PpyVProjectSortOrder value) { IMPL_PPIFC_PUTPROP(ProjectFilt, SortOrd); }

SDateRange DL6ICLS_PPFiltProject::get_StartPeriod()
	{ return DateRangeToOleDateRange(((ProjectFilt *)ExtraPtr)->StartPeriod); }

SDateRange DL6ICLS_PPFiltProject::get_EstFinishPeriod()
	{ return DateRangeToOleDateRange(((ProjectFilt *)ExtraPtr)->EstFinishPeriod); }
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewProject, DL6ICLS_PPViewProject_VTab, PPViewProject);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewProject::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltProject", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewProject::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(Project);
}

int32 DL6ICLS_PPViewProject::InitIteration(int32 order)
{
	return ((PPViewProject*)ExtraPtr)->InitIteration();
}

int32 DL6ICLS_PPViewProject::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_Project * p_item = (SPpyVI_Project *)item;
	ProjectViewItem inner_item;
	if(((PPViewProject *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		#define FLD(f) p_item->f = inner_item.f
			FLD(PrjTaskID);
			FLD(ID);
			FLD(ParentID);
			FLD(MngrID);
			FLD(ClientID);
			FLD(TemplateID);
			FLD(Flags);
			FLD(BillOpID);
		#undef FLD
		p_item->RecTag      = ppvProject;
		p_item->Kind        = (PpyOProjectKind)inner_item.Kind;
		p_item->Status      = (PpyOProjectStatus)inner_item.Status;
		p_item->Dt          = (OleDate)inner_item.Dt;
		p_item->BeginDt     = (OleDate)inner_item.BeginDt;
		p_item->EstFinishDt = (OleDate)inner_item.EstFinishDt;
		p_item->FinishDt    = (OleDate)inner_item.FinishDt;
		(temp_buf = inner_item.Name).CopyToOleStr(&p_item->Name);
		(temp_buf = inner_item.Code).CopyToOleStr(&p_item->Code);
		(temp_buf = inner_item.Descr).CopyToOleStr(&p_item->Descr);
		(temp_buf = inner_item.Memo).CopyToOleStr(&p_item->Memo);
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewProject::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewProject::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
//
// PPViewOpGrouping {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewOpGrouping, DL6ICLS_PPViewOpGrouping_VTab, PPViewOpGrouping);
//
// Interface IPpyFilt_OpGrouping implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltOpGrouping, DL6ICLS_PPFiltOpGrouping_VTab, OpGroupingFilt);

void DL6ICLS_PPFiltOpGrouping::SetPeriod(LDATE low, LDATE upp)
{
	((OpGroupingFilt*)ExtraPtr)->Period.Set(low, upp);
}

void DL6ICLS_PPFiltOpGrouping::SetLotsPeriod(LDATE low, LDATE upp)
{
	((OpGroupingFilt*)ExtraPtr)->LotsPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltOpGrouping::SetShipmentPeriod(LDATE low, LDATE upp)
{
	((OpGroupingFilt*)ExtraPtr)->ShipmentPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltOpGrouping::AddLocationID(int32 id)
{
	OpGroupingFilt * p_filt = ((OpGroupingFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->LocList.Add(id);
}

void DL6ICLS_PPFiltOpGrouping::FreeLocList()
{
	OpGroupingFilt * p_filt = ((OpGroupingFilt*)ExtraPtr);
	if(p_filt)
		p_filt->LocList.FreeAll();
}

SDateRange DL6ICLS_PPFiltOpGrouping::get_Period()
	{ return DateRangeToOleDateRange(((OpGroupingFilt *)ExtraPtr)->Period); }

SDateRange DL6ICLS_PPFiltOpGrouping::get_LotsPeriod()
	{ return DateRangeToOleDateRange(((OpGroupingFilt *)ExtraPtr)->LotsPeriod); }

SDateRange DL6ICLS_PPFiltOpGrouping::get_ShipmentPeriod()
	{ return DateRangeToOleDateRange(((OpGroupingFilt *)ExtraPtr)->ShipmentPeriod); }

int32 DL6ICLS_PPFiltOpGrouping::get_OpID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, OpID);}
void  DL6ICLS_PPFiltOpGrouping::put_OpID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, OpID);}

int32 DL6ICLS_PPFiltOpGrouping::get_CurID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, CurID);}
void  DL6ICLS_PPFiltOpGrouping::put_CurID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, CurID);}

int32 DL6ICLS_PPFiltOpGrouping::get_SupplID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, SupplID);}
void  DL6ICLS_PPFiltOpGrouping::put_SupplID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, SupplID);}

int32 DL6ICLS_PPFiltOpGrouping::get_ArID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, ArID);}
void  DL6ICLS_PPFiltOpGrouping::put_ArID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, ArID);}

int32 DL6ICLS_PPFiltOpGrouping::get_GoodsGrpID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, GoodsGrpID);}
void  DL6ICLS_PPFiltOpGrouping::put_GoodsGrpID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, GoodsGrpID);}

int32 DL6ICLS_PPFiltOpGrouping::get_GoodsID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, GoodsID);}
void  DL6ICLS_PPFiltOpGrouping::put_GoodsID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, GoodsID);}

int32 DL6ICLS_PPFiltOpGrouping::get_ExtGoodsTypeID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, ExtGoodsTypeID);}
void  DL6ICLS_PPFiltOpGrouping::put_ExtGoodsTypeID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, ExtGoodsTypeID);}

int32 DL6ICLS_PPFiltOpGrouping::get_SupplAgentID() {IMPL_PPIFC_GETPROP(OpGroupingFilt, SupplAgentID);}
void  DL6ICLS_PPFiltOpGrouping::put_SupplAgentID(int32 value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, SupplAgentID);}

PpyVOpGroupingCycleStats DL6ICLS_PPFiltOpGrouping::get_CycleStat() {IMPL_PPIFC_GETPROP_CAST(OpGroupingFilt, CycleStat, PpyVOpGroupingCycleStats);}
void  DL6ICLS_PPFiltOpGrouping::put_CycleStat(PpyVOpGroupingCycleStats value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, CycleStat);}

PpyVOpGroupingFlags DL6ICLS_PPFiltOpGrouping::get_Flags() {IMPL_PPIFC_GETPROP_CAST(OpGroupingFilt, Flags, PpyVOpGroupingFlags);}
void  DL6ICLS_PPFiltOpGrouping::put_Flags(PpyVOpGroupingFlags value) {IMPL_PPIFC_PUTPROP(OpGroupingFilt, Flags);}

int32 DL6ICLS_PPFiltOpGrouping::get_Cycle() {return ((OpGroupingFilt*)ExtraPtr)->Cycl.Cycle;}
void  DL6ICLS_PPFiltOpGrouping::put_Cycle(int32 value) {((OpGroupingFilt*)ExtraPtr)->Cycl.Cycle = (int16)value;}

int32 DL6ICLS_PPFiltOpGrouping::get_NumCycles() {return ((OpGroupingFilt*)ExtraPtr)->Cycl.NumCycles;}
void  DL6ICLS_PPFiltOpGrouping::put_NumCycles(int32 value) {((OpGroupingFilt*)ExtraPtr)->Cycl.NumCycles = (int16)value;}
//
// Interface IPapyrusView implementation
//
IUnknown * DL6ICLS_PPViewOpGrouping::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltOpGrouping", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewOpGrouping::Init(IUnknown* pFilt)
{
	IPpyFilt_OpGrouping * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_OpGrouping", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(((PPViewOpGrouping *)ExtraPtr)->Init_((const OpGroupingFilt *)GetExtraPtrByInterface(p_ifc_filt)));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewOpGrouping::InitIteration(int32 order)
{
	return ((PPViewOpGrouping *)ExtraPtr)->InitIteration();
}

int32 DL6ICLS_PPViewOpGrouping::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyVI_OpGrouping * p_item = (SPpyVI_OpGrouping *)item;
	OpGroupingViewItem inner_item;
	if(((PPViewOpGrouping *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_OPGROUPING;
#define FLD(f) p_item->f = inner_item.f
		FLD(ObjectID);
		FLD(OpID);
		FLD(GoodsTaxGrpID);
		FLD(LotTaxGrpID);
		FLD(Count);
		FLD(LnCount);
		FLD(AvgLn);
		FLD(Qtty);
		FLD(PhQtty);
		FLD(Amount);
		FLD(Cost);
		FLD(Price);
		FLD(Discount);
		FLD(Income);
		FLD(ExtCost);
		FLD(ExtPrice);
		FLD(VatSum);
		FLD(ExciseSum);
		FLD(STaxSum);
#undef FLD
		p_item->Dt = (OleDate)inner_item.Dt;
		p_item->fVatFreeSuppl = (int)inner_item.fVatFreeSuppl;
		p_item->fToggleSTax   = (int)inner_item.fToggleSTax;
		p_item->Sign          = (int)inner_item.Sign;
		(temp_buf = inner_item.OpName).CopyToOleStr(&p_item->OpName);
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewOpGrouping::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewOpGrouping::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
// } PPViewOpGrouping
//
// PPViewDebtTrnovr {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewDebtTrnovr, DL6ICLS_PPViewDebtTrnovr_VTab, PPViewDebtTrnovr);
//
// Interface IPpyFilt_DebtTrnovr implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltDebtTrnovr, DL6ICLS_PPFiltDebtTrnovr_VTab, DebtTrnovrFilt);

void DL6ICLS_PPFiltDebtTrnovr::SetPeriod(LDATE low, LDATE upp)
{
	((DebtTrnovrFilt*)ExtraPtr)->Period.Set(low, upp);
}

void DL6ICLS_PPFiltDebtTrnovr::SetPaymPeriod(LDATE low, LDATE upp)
{
	((DebtTrnovrFilt*)ExtraPtr)->PaymPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltDebtTrnovr::SetExpiryPeriod(LDATE low, LDATE upp)
{
	((DebtTrnovrFilt*)ExtraPtr)->ExpiryPeriod.Set(low, upp);
}

void DL6ICLS_PPFiltDebtTrnovr::AddLocationID(int32 id)
{
	DebtTrnovrFilt * p_filt = ((DebtTrnovrFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->LocIDList.add(id);
}

void DL6ICLS_PPFiltDebtTrnovr::FreeLocList()
{
	DebtTrnovrFilt * p_filt = ((DebtTrnovrFilt*)ExtraPtr);
	if(p_filt)
		p_filt->LocIDList.freeAll();
}

void DL6ICLS_PPFiltDebtTrnovr::AddContragentID(int32 id)
{
	DebtTrnovrFilt * p_filt = ((DebtTrnovrFilt*)ExtraPtr);
	if(id && p_filt)
		p_filt->CliIDList.add(id);
}

void DL6ICLS_PPFiltDebtTrnovr::FreeContragentList()
{
	DebtTrnovrFilt * p_filt = ((DebtTrnovrFilt*)ExtraPtr);
	if(p_filt)
		p_filt->CliIDList.freeAll();
}

SDateRange DL6ICLS_PPFiltDebtTrnovr::get_Period()
	{ return DateRangeToOleDateRange(((DebtTrnovrFilt *)ExtraPtr)->Period); }

SDateRange DL6ICLS_PPFiltDebtTrnovr::get_PaymPeriod()
	{ return DateRangeToOleDateRange(((DebtTrnovrFilt *)ExtraPtr)->PaymPeriod); }

SDateRange DL6ICLS_PPFiltDebtTrnovr::get_ExpiryPeriod()
	{ return DateRangeToOleDateRange(((DebtTrnovrFilt *)ExtraPtr)->ExpiryPeriod); }

int32 DL6ICLS_PPFiltDebtTrnovr::get_AccSheetID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, AccSheetID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_AccSheetID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, AccSheetID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_CurID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, CurID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_CurID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, CurID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_AgentID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, AgentID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_AgentID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, AgentID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_PayerID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, PayerID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_PayerID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, PayerID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_CityID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, CityID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_CityID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, CityID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_CategoryID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, CategoryID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_CategoryID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, CategoryID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_AccSheet2ID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, AccSheet2ID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_AccSheet2ID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, AccSheet2ID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_Article2ID() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, Article2ID);}
void  DL6ICLS_PPFiltDebtTrnovr::put_Article2ID(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, Article2ID);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_ExtExpiryTerm() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, ExtExpiryTerm);}
void  DL6ICLS_PPFiltDebtTrnovr::put_ExtExpiryTerm(int32 value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, ExtExpiryTerm);}

double DL6ICLS_PPFiltDebtTrnovr::get_ExtExpiryMinPart() {IMPL_PPIFC_GETPROP(DebtTrnovrFilt, ExtExpiryMinPart);}
void  DL6ICLS_PPFiltDebtTrnovr::put_ExtExpiryMinPart(double value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, ExtExpiryMinPart);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_Cycle() {return ((DebtTrnovrFilt*)ExtraPtr)->Cf.Cycle;}
void  DL6ICLS_PPFiltDebtTrnovr::put_Cycle(int32 value) {((DebtTrnovrFilt*)ExtraPtr)->Cf.Cycle = (int16)value;}

int32 DL6ICLS_PPFiltDebtTrnovr::get_NumCycles() { return ((DebtTrnovrFilt*)ExtraPtr)->Cf.NumCycles; }
void  DL6ICLS_PPFiltDebtTrnovr::put_NumCycles(int32 value) { ((DebtTrnovrFilt*)ExtraPtr)->Cf.NumCycles = (int16)value; }

PpyVDebtTrnovrSortOrder DL6ICLS_PPFiltDebtTrnovr::get_SortOrder() {IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, InitOrder, PpyVDebtTrnovrSortOrder);}
void  DL6ICLS_PPFiltDebtTrnovr::put_SortOrder(PpyVDebtTrnovrSortOrder value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, InitOrder);}

PpyVDebtTrnovrFlags DL6ICLS_PPFiltDebtTrnovr::get_Flags() {IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, Flags, PpyVDebtTrnovrFlags);}
void  DL6ICLS_PPFiltDebtTrnovr::put_Flags(PpyVDebtTrnovrFlags value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, Flags);}

int32 DL6ICLS_PPFiltDebtTrnovr::get_OpID() { return ((DebtTrnovrFilt*)ExtraPtr)->OpID; }
void DL6ICLS_PPFiltDebtTrnovr::put_OpID(int32 value) { ((DebtTrnovrFilt*)ExtraPtr)->OpID = value; }
PpyVSubstGrpBill DL6ICLS_PPFiltDebtTrnovr::get_Sgb()
	{ IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, Sgb.S, PpyVSubstGrpBill); }
void DL6ICLS_PPFiltDebtTrnovr::put_Sgb(PpyVSubstGrpBill value)
	{ IMPL_PPIFC_PUTPROP_CAST(DebtTrnovrFilt, Sgb.S, SubstGrpBill::_S); }
PpyVSubstGrpPerson DL6ICLS_PPFiltDebtTrnovr::get_Sgp()
	{ IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, Sgb.S2.Sgp, PpyVSubstGrpPerson); }
void DL6ICLS_PPFiltDebtTrnovr::put_Sgp(PpyVSubstGrpPerson value)
	{ IMPL_PPIFC_PUTPROP_CAST(DebtTrnovrFilt, Sgb.S2.Sgp, SubstGrpPerson); }
PpyVSubstGrpDate DL6ICLS_PPFiltDebtTrnovr::get_Sgd()
	{ IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, Sgb.S2.Sgd, PpyVSubstGrpDate); }
void DL6ICLS_PPFiltDebtTrnovr::put_Sgd(PpyVSubstGrpDate value)
	{ IMPL_PPIFC_PUTPROP_CAST(DebtTrnovrFilt, Sgb.S2.Sgd, SubstGrpDate); }

PpyVDebtTrnovrCycleKind DL6ICLS_PPFiltDebtTrnovr::get_CycleKind() {IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, CycleKind, PpyVDebtTrnovrCycleKind);}
void  DL6ICLS_PPFiltDebtTrnovr::put_CycleKind(PpyVDebtTrnovrCycleKind value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, CycleKind);}

PpyVDebtTrnovrExtKind DL6ICLS_PPFiltDebtTrnovr::get_ExtKind() {IMPL_PPIFC_GETPROP_CAST(DebtTrnovrFilt, ExtKind, PpyVDebtTrnovrExtKind);}
void  DL6ICLS_PPFiltDebtTrnovr::put_ExtKind(PpyVDebtTrnovrExtKind value) {IMPL_PPIFC_PUTPROP(DebtTrnovrFilt, ExtKind);}
//
// Interface IPapyrusView implementation
//
IUnknown * DL6ICLS_PPViewDebtTrnovr::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltDebtTrnovr", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewDebtTrnovr::Init(IUnknown* pFilt)
{
	IPpyFilt_DebtTrnovr * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_DebtTrnovr", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(((PPViewDebtTrnovr *)ExtraPtr)->Init_((const DebtTrnovrFilt *)GetExtraPtrByInterface(p_ifc_filt)));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewDebtTrnovr::InitIteration(int32 order)
{
	return ((PPViewDebtTrnovr *)ExtraPtr)->InitIteration((PPViewDebtTrnovr::IterOrder)order);
}

int32 DL6ICLS_PPViewDebtTrnovr::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SString temp_buf;
	SPpyVI_DebtTrnovr * p_item = (SPpyVI_DebtTrnovr *)item;
	DebtTrnovrViewItem inner_item;
	if(((PPViewDebtTrnovr *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_DEBTTRNOVR;
		p_item->ArticleID = inner_item.ID_;
#define FLD(f) p_item->f = inner_item.f
		FLD(Ar);
		FLD(PersonID);
		FLD(BillID);
		FLD(CurID);
		FLD(TabID);
		FLD(Debit);
		FLD(Credit);
		FLD(Debt);
		FLD(RPaym);
		FLD(Reckon);
		FLD(RDebt);
		FLD(TDebt);
		FLD(DebitCount);
		FLD(CreditCount);
		FLD(IsStop);
		FLD(MaxDelay);
#undef FLD
		p_item->AvgPaym = inner_item._AvgPaym;
		(temp_buf = inner_item.TabText).CopyToOleStr(&p_item->TabText);
		(temp_buf = inner_item.ArName).CopyToOleStr(&p_item->ArName);
		p_item->PayDate = (OleDate)inner_item.PayDate;
		p_item->LastPaymDate = (OleDate)inner_item.LastPaymDate;
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewDebtTrnovr::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewDebtTrnovr::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
// } PPViewDebtTrnovr
//
//
// PPViewLotOp {
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewLotOp, DL6ICLS_PPViewLotOp_VTab, PPViewLotOp);
//
// Interface IPpyFilt_LotOp implementation
//
DL6_IC_CONSTRUCTION_EXTRA(PPFiltLotOp, DL6ICLS_PPFiltLotOp_VTab, LotOpFilt);

int32 DL6ICLS_PPFiltLotOp::get_LotID() {IMPL_PPIFC_GETPROP(LotOpFilt, LotID);}
void  DL6ICLS_PPFiltLotOp::put_LotID(int32 value) {IMPL_PPIFC_PUTPROP(LotOpFilt, LotID);}

PpyVLotOpFlags DL6ICLS_PPFiltLotOp::get_Flags() {IMPL_PPIFC_GETPROP_CAST(LotOpFilt, Flags, PpyVLotOpFlags);}
void  DL6ICLS_PPFiltLotOp::put_Flags(PpyVLotOpFlags value) {IMPL_PPIFC_PUTPROP(LotOpFilt, Flags);}
//
// Interface IPapyrusView implementation
//
IUnknown * DL6ICLS_PPViewLotOp::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltLotOp", 0, (void **)&p_filt) ? p_filt : (IUnknown *)RaiseAppError();
}

int32 DL6ICLS_PPViewLotOp::Init(IUnknown* pFilt)
{
	IPpyFilt_LotOp * p_ifc_filt = 0;
	S_GUID uuid;
	THROW_INVARG(pFilt);
	THROW(GetInnerUUID("IPpyFilt_LotOp", uuid));
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt)));
	THROW(((PPViewLotOp *)ExtraPtr)->Init_((const LotOpFilt *)GetExtraPtrByInterface(p_ifc_filt)));
	CATCH
		AppError = 1;
	ENDCATCH
	CALLTYPEPTRMEMB(IUnknown, p_ifc_filt, Release());
	return !AppError;
}

int32 DL6ICLS_PPViewLotOp::InitIteration(int32 order)
{
	return ((PPViewLotOp *)ExtraPtr)->InitIteration();
}

int32 DL6ICLS_PPViewLotOp::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_LotOp * p_item = (SPpyVI_LotOp *)item;
	LotOpViewItem inner_item;
	if(((PPViewLotOp *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		p_item->RecTag = PPVIEWITEM_LOTOP;
#define FLD(f) p_item->f = inner_item.f
		FLD(LocID);
		FLD(OprNo);
		FLD(BillID);
		FLD(CorrLoc);
		FLD(LotID);
		FLD(GoodsID);
		FLD(Flags);
		FLD(Quantity);
		FLD(Rest);
		FLD(Cost);
		FLD(Price);
		FLD(QuotPrice);
		FLD(Discount);
		FLD(CurID);
		FLD(CurPrice);
#undef FLD
		p_item->Dt = (OleDate)inner_item.Dt;
		p_item->RByBill = (long)inner_item.RByBill;
		p_item->Reverse = (long)inner_item.Reverse;
		p_item->WtQtty  = (double)inner_item.WtQtty;
		p_item->WtRest  = (double)inner_item.WtRest;
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewLotOp::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewLotOp::GetTotal(PPYVIEWTOTAL total)
{
	return FuncNotSupported();
}
//
// } PPViewLotOp
//
// PPObjDebtDim {
//
DL6_IC_CONSTRUCTOR(PPObjDebtDim, DL6ICLS_PPObjDebtDim_VTab)
{
	PPObjDebtDim * p_e = new PPObjDebtDim;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjDebtDim)
{
	PPObjDebtDim * p_e = (PPObjDebtDim *)ExtraPtr;
	ZDELETE(p_e);
}
//
// Interface IPapyrusObject implementation
//
static void FillDebtDimtRec(const PPDebtDim * pInner, SPpyO_DebtDim * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = ppoDebtDim;
	pOuter->ID = pInner->ID;
	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
}

int32 DL6ICLS_PPObjDebtDim::Search(int32 id, PPYOBJREC rec)
{
	int    ok = 0;
	PPObjDebtDim * p_e = (PPObjDebtDim *)ExtraPtr;
	if(p_e) {
		PPDebtDim inner_rec;
		if((ok = p_e->Fetch(id, &inner_rec)) > 0) {
			FillDebtDimtRec(&inner_rec, (SPpyO_DebtDim *)rec);
		}
	}
	return ok;
}

int32 DL6ICLS_PPObjDebtDim::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC rec)
{
	return FuncNotSupported();
}

SString & DL6ICLS_PPObjDebtDim::GetName(int32 id)
{
	AppError = 1;
	return (RetStrBuf = 0);
}

IStrAssocList* DL6ICLS_PPObjDebtDim::GetSelector(int32 extraParam)
{
	AppError = 1;
	return 0;
}

int32 DL6ICLS_PPObjDebtDim::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjDebtDim::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
// } PPObjDebtDim
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPFias, DL6ICLS_PPFias_VTab, PPFiasReference)
//
// Interface IFias implementation
//
int32 DL6ICLS_PPFias::SearchAddr(int32 id, SPpyO_FiasAddr* pRec)
{
	int    ok = 0;
	PPFiasReference * p = (PPFiasReference *)ExtraPtr;
	if(p) {
		FiasAddrObjTbl::Rec rec;
		MEMSZERO(rec);
		ok = p->SearchObjByID(id, &rec, 1 /*use_cache*/);
		if(ok > 0) {
            if(pRec) {
				SString temp_buf, descr_buf;
				S_GUID uuid;
				pRec->ID = rec.IdUuRef;
				pRec->ParentID = rec.ParentUuRef;
				pRec->Level = rec.LevelStatus;
				//
				temp_buf.Z();
                if(p->FT.UrT.Search(rec.IdUuRef, uuid) > 0)
					uuid.ToStr(S_GUID::fmtIDL, temp_buf);
                temp_buf.CopyToOleStr(&pRec->AddrUuid);
                //
				temp_buf.Z();
                if(p->FT.UrT.Search(rec.RecUuID, uuid) > 0)
					uuid.ToStr(S_GUID::fmtIDL, temp_buf);
                temp_buf.CopyToOleStr(&pRec->RecUuid);
                //
                temp_buf.Z().CatLongZ(rec.PostalCode, 6).CopyToOleStr(&pRec->PostalCode);
				p->GetText(rec.NameTRef, temp_buf.Z());
				temp_buf.CopyToOleStr(&pRec->Name);
				p->GetText(rec.OfcNameTRef, temp_buf.Z());
				temp_buf.CopyToOleStr(&pRec->OfcName);
				p->GetText(rec.SnTRef, temp_buf.Z());
				temp_buf.CopyToOleStr(&pRec->ShortDescr);

				descr_buf = 0;
				int    level = rec.LevelStatus;
				PPFiasReference::IdentifyShortDescription(temp_buf, &level, &descr_buf);
				descr_buf.CopyToOleStr(&pRec->Descr);
            }
            ok = 1;
		}
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPFias::SearchHouse(int32 id, SPpyO_FiasHouse* pRec)
{
	int    ok = 0;
	PPFiasReference * p = (PPFiasReference *)ExtraPtr;
	if(p) {
		FiasHouseObjTbl::Rec rec;
		MEMSZERO(rec);
		ok = p->SearchHouseByID(id, &rec);
		if(ok > 0) {
            if(pRec) {
				SString temp_buf;
				S_GUID uuid;
				p->GetText(rec.NumTRef, temp_buf.Z());
				StringSet num_ss(':', temp_buf);
				pRec->ID = rec.IdUuRef;
				pRec->AddrID = rec.ParentUuRef;
				//
				pRec->HouseUuid = 0;
				temp_buf.Z();
                if(p->FT.UrT.Search(rec.IdUuRef, uuid) > 0)
					uuid.ToStr(S_GUID::fmtIDL, temp_buf);
                temp_buf.CopyToOleStr(&pRec->HouseUuid);
                //
				pRec->RecUuid = 0;
				temp_buf.Z().CatLongZ(rec.PostalCode, 6).CopyToOleStr(&pRec->PostalCode);
				uint   num_p = 0;
				if(num_ss.get(&num_p, temp_buf)) {
					temp_buf.CopyToOleStr(&pRec->HouseN);
					if(num_ss.get(&num_p, temp_buf)) {
						temp_buf.CopyToOleStr(&pRec->BuildN);
						if(num_ss.get(&num_p, temp_buf)) {
							temp_buf.CopyToOleStr(&pRec->StructN);
						}
					}
				}
            }
            ok = 1;
		}
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPFias::SearchAddrByGuid(SString & pGuidStr, SPpyO_FiasAddr* pRec)
{
	int    ok = 0;
	PPFiasReference * p = (PPFiasReference *)ExtraPtr;
	if(p) {
		FiasAddrObjTbl::Rec rec;
		MEMSZERO(rec);
		S_GUID uuid;
		if(uuid.FromStr(pGuidStr)) {
            ok = p->SearchObjByUUID(uuid, &rec);
			if(ok > 0) {
				if(pRec) {
					SString temp_buf, descr_buf;
					S_GUID uuid;
					pRec->ID = rec.IdUuRef;
					pRec->ParentID = rec.ParentUuRef;
					pRec->Level = rec.LevelStatus;
					//
					temp_buf.Z();
					if(p->FT.UrT.Search(rec.IdUuRef, uuid) > 0)
						uuid.ToStr(S_GUID::fmtIDL, temp_buf);
					temp_buf.CopyToOleStr(&pRec->AddrUuid);
					//
					temp_buf.Z();
					if(p->FT.UrT.Search(rec.RecUuID, uuid) > 0)
						uuid.ToStr(S_GUID::fmtIDL, temp_buf);
					temp_buf.CopyToOleStr(&pRec->RecUuid);
					//
					temp_buf.Z().CatLongZ(rec.PostalCode, 6).CopyToOleStr(&pRec->PostalCode);
					p->GetText(rec.NameTRef, temp_buf.Z());
					temp_buf.CopyToOleStr(&pRec->Name);
					p->GetText(rec.OfcNameTRef, temp_buf.Z());
					temp_buf.CopyToOleStr(&pRec->OfcName);
					p->GetText(rec.SnTRef, temp_buf.Z());
					temp_buf.CopyToOleStr(&pRec->ShortDescr);

					descr_buf = 0;
					int    level = rec.LevelStatus;
					PPFiasReference::IdentifyShortDescription(temp_buf, &level, &descr_buf);
					descr_buf.CopyToOleStr(&pRec->Descr);
				}
				ok = 1;
			}
		}
	}
	else
		AppError = 1;
	return ok;
}

int32 DL6ICLS_PPFias::SearchHouseByGuid(SString & pGuidStr, SPpyO_FiasHouse* pRec)
{
	int    ok = 0;
	PPFiasReference * p = (PPFiasReference *)ExtraPtr;
	if(p) {
		FiasHouseObjTbl::Rec rec;
		MEMSZERO(rec);
		S_GUID uuid;
		if(uuid.FromStr(pGuidStr)) {
			ok = p->SearchHouseByUUID(uuid, &rec);
			if(ok > 0) {
				if(pRec) {
					SString temp_buf;
					S_GUID uuid;
					p->GetText(rec.NumTRef, temp_buf.Z());
					StringSet num_ss(':', temp_buf);
					pRec->ID = rec.IdUuRef;
					pRec->AddrID = rec.ParentUuRef;
					//
					pRec->HouseUuid = 0;
					temp_buf.Z();
					if(p->FT.UrT.Search(rec.IdUuRef, uuid) > 0)
						uuid.ToStr(S_GUID::fmtIDL, temp_buf);
					temp_buf.CopyToOleStr(&pRec->HouseUuid);
					//
					pRec->RecUuid = 0;
					temp_buf.Z().CatLongZ(rec.PostalCode, 6).CopyToOleStr(&pRec->PostalCode);
					uint   num_p = 0;
					if(num_ss.get(&num_p, temp_buf)) {
						temp_buf.CopyToOleStr(&pRec->HouseN);
						if(num_ss.get(&num_p, temp_buf)) {
							temp_buf.CopyToOleStr(&pRec->BuildN);
							if(num_ss.get(&num_p, temp_buf)) {
								temp_buf.CopyToOleStr(&pRec->StructN);
							}
						}
					}
				}
				ok = 1;
			}
		}
	}
	else
		AppError = 1;
	return ok;
}
//
//
//
static void FillCCheckRec(const CCheckPacket * pInner, SPpyO_CCheck * pOuter)
{
	SString temp_buf;
#define FLD(f) pOuter->f = pInner->Rec.f
	FLD(ID);
	FLD(Code);
	if(pInner->Rec.SessID) {
		PPObjCSession cs_obj;
		CSessionTbl::Rec cs_rec;
		pOuter->PosNodeID = (cs_obj.Fetch(pInner->Rec.SessID, &cs_rec) > 0) ? cs_rec.CashNodeID : 0;
	}
	else
		pOuter->PosNodeID = pInner->Rec.CashID;
	pOuter->PosNumber = pInner->Rec.CashID;
	FLD(UserID);
	pOuter->CSessID = pInner->Rec.SessID;
	pOuter->Dt = (OleDate)pInner->Rec.Dt;
	pOuter->Tm = (OleDate)pInner->Rec.Tm;
	FLD(Flags);
	pOuter->Amount = MONEYTOLDBL(pInner->Rec.Amount);
	pOuter->Discount = MONEYTOLDBL(pInner->Rec.Discount);
	FLD(SCardID);
	{
		temp_buf.Z();
		if(pInner->Rec.SCardID) {
			PPObjSCard sc_obj;
			SCardTbl::Rec sc_rec;
			if(sc_obj.Fetch(pInner->Rec.SCardID, &sc_rec) > 0)
				temp_buf = sc_rec.Code;
		}
		temp_buf.CopyToOleStr(&pOuter->SCardCode);
	}
    pOuter->SalerID = pInner->Ext.SalerID;
    pOuter->CTableNo = pInner->Ext.TableNo;
    pOuter->GuestCount = pInner->Ext.GuestCount;
    pOuter->AddrID = pInner->Ext.AddrID;
    pOuter->LinkCCheckID = pInner->Ext.LinkCheckID;
    pOuter->StartOrdDt = pInner->Ext.StartOrdDtm.d;
    pOuter->StartOrdTm = pInner->Ext.StartOrdDtm.t;
    pOuter->EndOrdDt = pInner->Ext.EndOrdDtm.d;
    pOuter->EndOrdTm = pInner->Ext.EndOrdDtm.t;
    pOuter->CreationDt = pInner->Ext.CreationDtm.d;
    pOuter->CreationTm = pInner->Ext.CreationDtm.t;
    (temp_buf = pInner->Ext.Memo).CopyToOleStr(&pOuter->Memo);
#undef FLD
}

static void FillCCheckItemRec(const CCheckItem * pInner, SPpyO_CCheckLine * pOuter)
{
	SString temp_buf;
#define FLD(f) pOuter->f = pInner->f
	FLD(GoodsID);
	FLD(Quantity);
	FLD(PhQtty);
	FLD(Price);
	FLD(Discount);
	FLD(BeforeGiftPrice);
	FLD(GiftID);
	FLD(Flags);
	FLD(Division);
	FLD(Queue);

	(temp_buf = pInner->BarCode).CopyToOleStr(&pOuter->Barcode);
	(temp_buf = pInner->Serial).CopyToOleStr(&pOuter->Serial);
	(temp_buf = pInner->GoodsName).CopyToOleStr(&pOuter->GoodsName);
#undef FLD
}

DL6_IC_CONSTRUCTION_EXTRA(PPCCheckPacket, DL6ICLS_PPCCheckPacket_VTab, CCheckPacket);
//
// Interface ICCheckPacket implementation
//
int32 DL6ICLS_PPCCheckPacket::Init()
{
	CCheckPacket * p_pack = (CCheckPacket *)ExtraPtr;
	return p_pack ? (p_pack->Init(), 1) : 0;
}

int32 DL6ICLS_PPCCheckPacket::PutHeader(SPpyO_CCheck* pHeader)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPCCheckPacket::GetHeader(SPpyO_CCheck* pHeader)
{
	int    ok = -1;
	CCheckPacket * p_pack = (CCheckPacket*)ExtraPtr;
	if(p_pack && pHeader) {
		FillCCheckRec(p_pack, pHeader);
		ok = 1;
	}
	return ok;
}

int32 DL6ICLS_PPCCheckPacket::AddItem(SPpyO_CCheckLine* pItem)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPCCheckPacket::GetItemsCount()
{
	int    _c = 0;
	CCheckPacket * p_pack = (CCheckPacket *)ExtraPtr;
	if(p_pack)
		_c = (int)p_pack->GetCount();
	else
		SetAppError(0);
	return _c;
}

int32 DL6ICLS_PPCCheckPacket::GetItem(int32 position, SPpyO_CCheckLine* pItem)
{
	int    ok = 0;
	CCheckPacket * p_pack = (CCheckPacket *)ExtraPtr;
	if(p_pack && position >= 0 && position < (int32)p_pack->GetCount()) {
		uint   _pos = (uint)position;
		CCheckItem item;
		if(p_pack->EnumLines(&_pos, &item)) {
			if(pItem)
				FillCCheckItemRec(&item, pItem);
			ok = 1;
		}
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPCCheckPacket::EnumItems(int32* pIdx, SPpyO_CCheckLine* pItem)
{
	int    ok = 0;
	CCheckPacket * p_pack = (CCheckPacket *)ExtraPtr;
	if(p_pack) {
		uint   _pos = pIdx ? (uint)*pIdx : 0;
		CCheckItem item;
		if(p_pack->EnumLines(&_pos, &item)) {
			if(pItem)
				FillCCheckItemRec(&item, pItem);
			ASSIGN_PTR(pIdx, (int32)_pos);
			ok = 1;
		}
		else
			ok = -1;
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPCCheckPacket::AddPaymItem(SPpyO_CCheckPaym* pPaymItem)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPCCheckPacket::GetPaymItemsCount()
{
	int    _c = 0;
	CCheckPacket * p_pack = (CCheckPacket *)ExtraPtr;
	if(p_pack) {
		const CcAmountList & r_al = p_pack->AL_Const();
		_c = (int)r_al.getCount();
	}
	else
		SetAppError(0);
	return _c;
}

int32 DL6ICLS_PPCCheckPacket::GetPaymItem(int32 position, SPpyO_CCheckPaym* pPaymItem)
{
	return FuncNotSupported();
}
//
//
//
struct InnerCCheckExtra {
	PPObjSCard ScObj;
	PPObjCSession CsObj;
};

DL6_IC_CONSTRUCTOR(PPObjCCheck, DL6ICLS_PPObjCCheck_VTab)
{
	InnerCCheckExtra * p_e = new InnerCCheckExtra;
	ExtraPtr = p_e;
}

DL6_IC_DESTRUCTOR(PPObjCCheck)
{
	InnerCCheckExtra * p_e = (InnerCCheckExtra*)ExtraPtr;
	ZDELETE(p_e);
}
//
// Interface IPapyrusObjCCheck implementation
//
int32 DL6ICLS_PPObjCCheck::Search(int32 id, SPpyO_CCheck* pRec)
{
	int    ok = 0;
	InnerCCheckExtra * p_e = (InnerCCheckExtra *)ExtraPtr;
	if(p_e) {
		CCheckPacket pack;
		ok = p_e->ScObj.P_CcTbl->LoadPacket(id, 0, &pack);
		FillCCheckRec(&pack, pRec);
	}
	SetAppError(ok);
	return ok;
}

ICCheckPacket* DL6ICLS_PPObjCCheck::CreatePacket()
{
	void * p_ifc = 0;
	THROW(IfcImpCheckDictionary());
	THROW(CreateInnerInstance("PPCCheckPacket", "ICCheckPacket", &p_ifc));
	CATCH
		AppError = 1;
		p_ifc = 0;
	ENDCATCH
	return (ICCheckPacket*)p_ifc;
}

int32 DL6ICLS_PPObjCCheck::GetPacket(int32 id, ICCheckPacket* pPack)
{
	int    ok = 0;
	InnerCCheckExtra * p_e = (InnerCCheckExtra *)ExtraPtr;
	if(p_e) {
		CCheckPacket * p_pack = (CCheckPacket *)SCoClass::GetExtraPtrByInterface(pPack);
		if(p_pack)
			ok = p_e->ScObj.P_CcTbl->LoadPacket(id, 0, p_pack);
	}
	SetAppError(ok);
	return ok;
}
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPObjSCardSeries, DL6ICLS_PPObjSCardSeries_VTab, PPObjSCardSeries)
//
// Interface IPapyrusObject implementation
//
static void FillSCardSeriesRec(const PPSCardSeries * pInner, SPpyO_SCardSeries * pOuter)
{
	SString temp_buf;
	pOuter->RecTag = PPOBJ_SCARDSERIES;
#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(ChargeGoodsID);
	FLD(BonusChrgGrpID);
	FLD(BonusGrpID);
	FLD(CrdGoodsGrpID);
	FLD(Issue);
	FLD(Expiry);
	pOuter->PctDiscount = fdiv100i(pInner->PDis);
	FLD(MaxCredit);
	FLD(Flags);
	pOuter->QuotKindID = pInner->QuotKindID_s;
	FLD(PersonKindID);

	(temp_buf = pInner->Name).CopyToOleStr(&pOuter->Name);
	(temp_buf = pInner->Symb).CopyToOleStr(&pOuter->Symb);
	(temp_buf = pInner->CodeTempl).CopyToOleStr(&pOuter->CodeTempl);
#undef FLD
}

int32 DL6ICLS_PPObjSCardSeries::Search(int32 id, PPYOBJREC pOuterRec)
{
	int    ok = 0;
	PPObjSCardSeries * p_obj = (PPObjSCardSeries *)ExtraPtr;
	if(p_obj) {
		PPSCardSeries inner_rec;
		ok = p_obj->Search(id, &inner_rec);
		FillSCardSeriesRec(&inner_rec, (SPpyO_SCardSeries *)pOuterRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjSCardSeries::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC pOuterRec)
{
	int    ok = 0;
	PPObjSCardSeries * p_obj = (PPObjSCardSeries *)ExtraPtr;
	if(p_obj) {
		PPSCardSeries inner_rec;
		PPID id = 0;
		if(kind == 1) {
			ok = p_obj->SearchBySymb(text, &id);
			if(ok > 0 && p_obj->Search(id, &inner_rec) > 0) {
				;
			}
			else
				ok = -1;
		}
		else { // (kind == 0)
			ok = p_obj->SearchByName(text, &id, &inner_rec);
		}
		FillSCardSeriesRec(&inner_rec, (SPpyO_SCardSeries*)pOuterRec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjSCardSeries::GetName(int32 id)
{
	PPObjSCardSeries * p_obj = (PPObjSCardSeries *)ExtraPtr;
	if(p_obj) {
		PPSCardSeries inner_rec;
		if(p_obj->Fetch(id, &inner_rec) > 0)
			RetStrBuf = inner_rec.Name;
		else
			ideqvalstr(id, RetStrBuf);
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return RetStrBuf;
}

IStrAssocList* DL6ICLS_PPObjSCardSeries::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjSCardSeries::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjSCardSeries::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}


DL6_IC_CONSTRUCTION_EXTRA(PPObjSCard, DL6ICLS_PPObjSCard_VTab, PPObjSCard)
//
// Interface IPapyrusObject implementation
//
static void FillSCardRec(const SCardTbl::Rec * pInner, SPpyO_SCard * pOuter)
{
	SString temp_buf;
#define FLD(f) pOuter->f = pInner->f
	FLD(ID);
	FLD(SeriesID);
	FLD(PersonID);
	FLD(Flags);
	FLD(Dt);
	FLD(Expiry);
	pOuter->PctDiscount = fdiv100i(pInner->PDis);
	FLD(AutoGoodsID);
	FLD(MaxCredit);
	FLD(Turnover);
	FLD(Rest);
	FLD(UsageTmStart);
	FLD(UsageTmEnd);
	FLD(PeriodTerm);
	FLD(PeriodCount);

	(temp_buf = pInner->Code).CopyToOleStr(&pOuter->Code);
	temp_buf.Z().CopyToOleStr(&pOuter->Password); // Передаем пустую строку
#undef FLD
}

int32 DL6ICLS_PPObjSCard::Search(int32 id, PPYOBJREC pOuterRec)
{
	int    ok = 0;
	PPObjSCard * p_obj = (PPObjSCard *)ExtraPtr;
	if(p_obj) {
		SCardTbl::Rec inner_rec;
		MEMSZERO(inner_rec);
		ok = p_obj->Search(id, &inner_rec);
		FillSCardRec(&inner_rec, (SPpyO_SCard *)pOuterRec);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjSCard::SearchByName(SString & text, int32 kind, int32 extraParam, PPYOBJREC pOuterRec)
{
	int    ok = 0;
	PPObjSCard * p_obj = (PPObjSCard *)ExtraPtr;
	if(p_obj) {
		SCardTbl::Rec inner_rec;
		MEMSZERO(inner_rec);
		ok = p_obj->P_Tbl->SearchCode(extraParam, text, &inner_rec);
		FillSCardRec(&inner_rec, (SPpyO_SCard*)pOuterRec);
	}
	SetAppError(ok);
	return ok;
}

SString & DL6ICLS_PPObjSCard::GetName(int32 id)
{
	PPObjSCard * p_obj = (PPObjSCard *)ExtraPtr;
	if(p_obj) {
		SCardTbl::Rec inner_rec;
		if(p_obj->Fetch(id, &inner_rec) > 0)
			RetStrBuf = inner_rec.Code;
		else
			ideqvalstr(id, RetStrBuf);
	}
	else {
		RetStrBuf = 0;
		AppError = 1;
	}
	return RetStrBuf;
}

IStrAssocList* DL6ICLS_PPObjSCard::GetSelector(int32 extraParam)
{
	IStrAssocList * p = (IStrAssocList *)GetPPObjIStrAssocList(this, (PPObject *)ExtraPtr, extraParam);
	SetAppError(BIN(p));
	return p;
}

int32 DL6ICLS_PPObjSCard::Create(PPYOBJREC pRec, int32 flags, int32* pID)
{
	return FuncNotSupported();
}

int32 DL6ICLS_PPObjSCard::Update(int32 id, int32 flags, PPYOBJREC rec)
{
	return FuncNotSupported();
}
//
// Interface IPapyrusObjSCard implementation
//
int32 DL6ICLS_PPObjSCard::GetPacket(int32 id, SPpyO_SCard* pPack)
{
	return Search(id, pPack);
}

int32 DL6ICLS_PPObjSCard::PutPacket(int32* pID, SPpyO_SCard* pPack, int32 useTa)
{
	int    ok = 0;
	PPObjSCard * p_obj = (PPObjSCard *)ExtraPtr;
	if(p_obj) {
		//SCardTbl::Rec inner_rec;
		//MEMSZERO(inner_rec);
		SString temp_buf;
		PPSCardPacket inner_pack;
		inner_pack.Rec.ID = pPack->ID;
		inner_pack.Rec.SeriesID = pPack->SeriesID;
		inner_pack.Rec.PersonID = pPack->PersonID;
		inner_pack.Rec.Flags = pPack->Flags;
		inner_pack.Rec.Dt = pPack->Dt;
		inner_pack.Rec.Expiry = pPack->Expiry;
		inner_pack.Rec.PDis = (long)(pPack->PctDiscount * 100L);
		inner_pack.Rec.AutoGoodsID = pPack->AutoGoodsID;
		inner_pack.Rec.MaxCredit = pPack->MaxCredit;
		inner_pack.Rec.Turnover = pPack->Turnover;
		inner_pack.Rec.Rest = pPack->Rest;
		inner_pack.Rec.UsageTmStart = pPack->UsageTmStart;
		inner_pack.Rec.UsageTmEnd = pPack->UsageTmEnd;
		inner_pack.Rec.PeriodTerm = (int16)pPack->PeriodTerm;
		inner_pack.Rec.PeriodCount = (int16)pPack->PeriodCount;
		temp_buf.CopyFromOleStr(pPack->Code).CopyTo(inner_pack.Rec.Code, sizeof(inner_pack.Rec.Code));
		temp_buf.CopyFromOleStr(pPack->Password);
		inner_pack.PutExtStrData(inner_pack.extssPassword, temp_buf);
		//temp_buf.CopyTo(inner_rec.Password, sizeof(inner_rec.Password))
		ok = p_obj->PutPacket(pID, &inner_pack, useTa);
	}
	SetAppError(ok);
	return ok;
}

int32 DL6ICLS_PPObjSCard::SetFreezingPeriod(int32 id, SDateRange* pDateRange, int32 useTa)
{
	return FuncNotSupported();
}

