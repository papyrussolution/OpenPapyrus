//
// This file was compiled from d:\Papyrus\Src\Ppaldd\dd.ald by PPALDDC.EXE
// !!! DON'T MODIFY IT !!!
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
PPALDD * PPALDD::CreateInstance(const char * /*pResFile*/, long parentID, long resID, PPALDD ** ppa)
{
	PPALDD * p_rval = 0;
	if(!resID)
		resID = parentID;

	switch(parentID) {
		case 1: p_rval = new PPALDD_DataObject(resID, ppa); break;
		case 2: p_rval = new PPALDD_Global(resID, ppa); break;
		case 3: p_rval = new PPALDD_User(resID, ppa); break;
		case 4: p_rval = new PPALDD_DBDiv(resID, ppa); break;
		case 5: p_rval = new PPALDD_Currency(resID, ppa); break;
		case 6: p_rval = new PPALDD_CurRateType(resID, ppa); break;
		case 7: p_rval = new PPALDD_AccSheet(resID, ppa); break;
		case 8: p_rval = new PPALDD_OprKind(resID, ppa); break;
#ifdef __WIN32__
		case 9: p_rval = new PPALDD_OprKindList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 10: p_rval = new PPALDD_CurRateView(resID, ppa); break;
#endif
		case 11: p_rval = new PPALDD_Country(resID, ppa); break;
		case 12: p_rval = new PPALDD_Region(resID, ppa); break;
		case 13: p_rval = new PPALDD_City(resID, ppa); break;
		case 14: p_rval = new PPALDD_Location(resID, ppa); break;
		case 15: p_rval = new PPALDD_PersonKind(resID, ppa); break;
		case 16: p_rval = new PPALDD_RegisterType(resID, ppa); break;
		case 17: p_rval = new PPALDD_TagType(resID, ppa); break;
		case 18: p_rval = new PPALDD_PersonStatus(resID, ppa); break;
		case 19: p_rval = new PPALDD_PersonCat(resID, ppa); break;
		case 20: p_rval = new PPALDD_Person(resID, ppa); break;
		case 21: p_rval = new PPALDD_PersonReq(resID, ppa); break;
		case 22: p_rval = new PPALDD_Warehouse(resID, ppa); break;
		case 23: p_rval = new PPALDD_Article(resID, ppa); break;
#ifdef __WIN32__
		case 24: p_rval = new PPALDD_ArticleView(resID, ppa); break;
#endif
		case 25: p_rval = new PPALDD_SCardSeries(resID, ppa); break;
		case 26: p_rval = new PPALDD_SCard(resID, ppa); break;
		case 27: p_rval = new PPALDD_QuotKind(resID, ppa); break;
		case 28: p_rval = new PPALDD_Transport(resID, ppa); break;
		case 29: p_rval = new PPALDD_Brand(resID, ppa); break;
		case 30: p_rval = new PPALDD_BillStatus(resID, ppa); break;
		case 31: p_rval = new PPALDD_Bill(resID, ppa); break;
		case 32: p_rval = new PPALDD_AmountType(resID, ppa); break;
		case 33: p_rval = new PPALDD_Unit(resID, ppa); break;
		case 34: p_rval = new PPALDD_GoodsType(resID, ppa); break;
		case 35: p_rval = new PPALDD_GoodsTaxGrp(resID, ppa); break;
		case 36: p_rval = new PPALDD_GoodsClass(resID, ppa); break;
#ifdef __WIN32__
		case 37: p_rval = new PPALDD_GoodsClassView(resID, ppa); break;
#endif
		case 38: p_rval = new PPALDD_AssetWrOffGrp(resID, ppa); break;
		case 39: p_rval = new PPALDD_GoodsGroup(resID, ppa); break;
		case 40: p_rval = new PPALDD_Goods(resID, ppa); break;
		case 41: p_rval = new PPALDD_QualityCert(resID, ppa); break;
		case 42: p_rval = new PPALDD_Lot(resID, ppa); break;
#ifdef __WIN32__
		case 43: p_rval = new PPALDD_GoodsView(resID, ppa); break;
#endif
		case 44: p_rval = new PPALDD_GoodsRest(resID, ppa); break;
#ifdef __WIN32__
		case 46: p_rval = new PPALDD_GoodsTaxAnlz(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 47: p_rval = new PPALDD_GoodsOpAnlz(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 48: p_rval = new PPALDD_GoodsOpAnlzCmp(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 50: p_rval = new PPALDD_PriceListData(resID, ppa); break;
#endif
		case 52: p_rval = new PPALDD_GoodsLabel(resID, ppa); break;
		case 53: p_rval = new PPALDD_GoodsBillBase(resID, ppa); break;
		case 54: p_rval = new PPALDD_GoodsBillQCert(resID, ppa); break;
		case 55: p_rval = new PPALDD_GoodsReval(resID, ppa); break;
		case 56: p_rval = new PPALDD_GoodsBillModif(resID, ppa); break;
		case 57: p_rval = new PPALDD_CashOrder(resID, ppa); break;
#ifdef __WIN32__
		case 60: p_rval = new PPALDD_BnkPaymOrder(resID, ppa); break;
#endif
		case 61: p_rval = new PPALDD_BillPayPlan(resID, ppa); break;
#ifdef __WIN32__
		case 62: p_rval = new PPALDD_Lots(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 63: p_rval = new PPALDD_LotOps(resID, ppa); break;
#endif
		case 64: p_rval = new PPALDD_BillList(resID, ppa); break;
		case 65: p_rval = new PPALDD_ContentBList(resID, ppa); break;
		case 66: p_rval = new PPALDD_CashBook(resID, ppa); break;
		case 67: p_rval = new PPALDD_SysJournal(resID, ppa); break;
		case 79: p_rval = new PPALDD_DebtTrnovr(resID, ppa); break;
#ifdef __WIN32__
		case 81: p_rval = new PPALDD_PaymBillList(resID, ppa); break;
#endif
		case 82: p_rval = new PPALDD_TransferBase(resID, ppa); break;
		case 83: p_rval = new PPALDD_Account(resID, ppa); break;
		case 84: p_rval = new PPALDD_AccRel(resID, ppa); break;
#ifdef __WIN32__
		case 85: p_rval = new PPALDD_AccountView(resID, ppa); break;
#endif
		case 86: p_rval = new PPALDD_AccturnList(resID, ppa); break;
		case 87: p_rval = new PPALDD_AccAnlz(resID, ppa); break;
#ifdef __WIN32__
		case 91: p_rval = new PPALDD_Balance(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 92: p_rval = new PPALDD_VatBook(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 93: p_rval = new PPALDD_OpGrouping(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 94: p_rval = new PPALDD_GoodsStruc(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 95: p_rval = new PPALDD_GoodsStrucList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 96: p_rval = new PPALDD_GoodsMov(resID, ppa); break;
#endif
		case 97: p_rval = new PPALDD_CashNode(resID, ppa); break;
		case 98: p_rval = new PPALDD_CSession(resID, ppa); break;
		case 99: p_rval = new PPALDD_CCheck(resID, ppa); break;
		case 100: p_rval = new PPALDD_CCheckView(resID, ppa); break;
#ifdef __WIN32__
		case 101: p_rval = new PPALDD_CCheckDetail(resID, ppa); break;
#endif
		case 102: p_rval = new PPALDD_CSessionView(resID, ppa); break;
		case 103: p_rval = new PPALDD_CSessExc(resID, ppa); break;
#ifdef __WIN32__
		case 104: p_rval = new PPALDD_TrfrAnlzBase(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 110: p_rval = new PPALDD_Invent(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 111: p_rval = new PPALDD_PUGL(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 112: p_rval = new PPALDD_GoodsBasket(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 113: p_rval = new PPALDD_ShipmAnlz(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 114: p_rval = new PPALDD_PredictSales(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 115: p_rval = new PPALDD_GoodsTurnovr(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 116: p_rval = new PPALDD_PaymPlan(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 117: p_rval = new PPALDD_AdvanceRep(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 118: p_rval = new PPALDD_SCardList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 119: p_rval = new PPALDD_SCardOpList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 120: p_rval = new PPALDD_BillTotal(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 121: p_rval = new PPALDD_GoodsRestTotal(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 122: p_rval = new PPALDD_GTaxAnlzTotal(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 123: p_rval = new PPALDD_PersonList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 124: p_rval = new PPALDD_AssetReceipt(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 125: p_rval = new PPALDD_ObjSyncCmp(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 126: p_rval = new PPALDD_AssetView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 127: p_rval = new PPALDD_SStatView(resID, ppa); break;
#endif
		case 128: p_rval = new PPALDD_LogList(resID, ppa); break;
#ifdef __WIN32__
		case 129: p_rval = new PPALDD_Warrant(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 130: p_rval = new PPALDD_FreightList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 131: p_rval = new PPALDD_QuotView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 132: p_rval = new PPALDD_GoodsBillCmp(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 133: p_rval = new PPALDD_Complete(resID, ppa); break;
#endif
		case 134: p_rval = new PPALDD_MrpTab(resID, ppa); break;
#ifdef __WIN32__
		case 135: p_rval = new PPALDD_MrpLines(resID, ppa); break;
#endif
		case 136: p_rval = new PPALDD_Processor(resID, ppa); break;
#ifdef __WIN32__
		case 137: p_rval = new PPALDD_ProcessorView(resID, ppa); break;
#endif
		case 138: p_rval = new PPALDD_Tech(resID, ppa); break;
		case 139: p_rval = new PPALDD_TSession(resID, ppa); break;
#ifdef __WIN32__
		case 141: p_rval = new PPALDD_TechView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 142: p_rval = new PPALDD_TSessionView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 143: p_rval = new PPALDD_TSessLineView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 144: p_rval = new PPALDD_TSessAnlz(resID, ppa); break;
#endif
		case 145: p_rval = new PPALDD_Project(resID, ppa); break;
		case 146: p_rval = new PPALDD_PrjTask(resID, ppa); break;
#ifdef __WIN32__
		case 147: p_rval = new PPALDD_ProjectView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 148: p_rval = new PPALDD_PrjTaskView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 149: p_rval = new PPALDD_PrjTaskViewCt(resID, ppa); break;
#endif
		case 150: p_rval = new PPALDD_BillInfo(resID, ppa); break;
		case 151: p_rval = new PPALDD_BillInfoList(resID, ppa); break;
#ifdef __WIN32__
		case 152: p_rval = new PPALDD_PriceAnlz(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 153: p_rval = new PPALDD_PrcBusyView(resID, ppa); break;
#endif
		case 154: p_rval = new PPALDD_RptInfo(resID, ppa); break;
#ifdef __WIN32__
		case 155: p_rval = new PPALDD_SCardSerView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 156: p_rval = new PPALDD_GoodsGroupView(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 157: p_rval = new PPALDD_PersonEvent(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 158: p_rval = new PPALDD_PsnEventItem(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 159: p_rval = new PPALDD_PsnRelList(resID, ppa); break;
#endif
#ifdef __WIN32__
		case 160: p_rval = new PPALDD_JobPool(resID, ppa); break;
#endif
	}
	if(p_rval && !p_rval->IsValid()) {
		delete p_rval;
		p_rval = 0;
	}
	return p_rval;
}
