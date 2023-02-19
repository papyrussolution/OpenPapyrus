// V_PCKG.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2006, 2007, 2010, 2016, 2017, 2019, 2023
//
#include <pp.h>
#pragma hdrstop

struct PckgViewListEntry {
	PPID   ID;
	char   Code[16];
};

#pragma warn -stv

static int CreatePckgViewList(PackageCore * pPckgCore, ReceiptCore * pRcptCore, const PckgFilt * pFilt, SArray * pList)
{
	PPIDArray lot_list, pckg_list;
	PckgViewListEntry entry;
	DBQ * dbq = 0;
	PackageCore * p_p = pPckgCore;
	ReceiptCore * p_r = pRcptCore;
	if(!p_p)
		return -1;
	if(pFilt->GoodsID) {
		ReceiptTbl::Key2 rk2;
		BExtQuery gq(p_r, 2);
		dbq = & (p_r->GoodsID == pFilt->GoodsID && p_r->Closed == 0L);
		dbq = ppcheckfiltid(dbq, p_r->LocID, pFilt->LocID);
		gq.select(p_r->ID, 0).where(*dbq);
		MEMSZERO(rk2);
		rk2.GoodsID = pFilt->GoodsID;
		for(gq.initIteration(false, &rk2, spGe); gq.nextIteration();) {
			PPID pckg_id = 0;
			PackageTbl::Rec pckg_rec;
			if(p_p->GetLotLink(p_r->data.ID, &pckg_id, &pckg_rec) > 0) {
				if(pckg_rec.Closed == 0 && (!pFilt->PckgTypeID ||
					pckg_rec.PckgTypeID == pFilt->PckgTypeID)) {
					MEMSZERO(entry);
					entry.ID = pckg_rec.ID;
					STRNSCPY(entry.Code, pckg_rec.Code);
					THROW_SL(pList->insert(&entry));
				}
			}
		}
	}
	else {
		PackageTbl::Key1 pk;
		BExtQuery q(p_p, 1);
		dbq = &(p_p->Closed == 0L);
		if(pFilt->PckgTypeID > 0)
			dbq = &(*dbq && p_p->PckgTypeID == pFilt->PckgTypeID);
		if(pFilt->LocID)
			dbq = &(*dbq && p_p->LocID == pFilt->LocID);
		q.select(p_p->ID, p_p->Code, 0L).where(*dbq);
		MEMSZERO(pk);
		for(q.initIteration(false, &pk, spGe); q.nextIteration();) {
			MEMSZERO(entry);
			entry.ID = p_p->data.ID;
			STRNSCPY(entry.Code, p_p->data.Code);
			THROW_SL(pList->insert(&entry));
		}
	}
	CATCH
		return 0;
	ENDCATCH
	return 1;
}

#pragma warn .stv

class SelPckgDialog : public TDialog {
public:
	SelPckgDialog(PPObjBill * pBObj);
	int    setList(const SArray *);
	int    getSelectedPckgID(PPID *);
private:
	DECL_HANDLE_EVENT;
	void   updateList();
	void   viewDetail();

	SmartListBox * P_List;
	const  SArray * P_PckgList;
	PPID   SelPckgID;
	PPObjBill * P_BObj;
};

SelPckgDialog::SelPckgDialog(PPObjBill * pBObj) : TDialog(DLG_SELPCKG), P_BObj(pBObj), SelPckgID(0), P_PckgList(0)
{
	P_List = static_cast<SmartListBox *>(getCtrlView(CTL_SELPCKG_LIST));
	if(!SetupStrListBox(P_List))
		PPError();
}

void SelPckgDialog::viewDetail()
{
	if(SmartListBox::IsValidS(P_List)) {
		PPID pckg_id = 0;
		P_List->getCurID(&pckg_id);
		if(pckg_id)
			P_BObj->ViewPckgDetail(pckg_id);
	}
}

int SelPckgDialog::setList(const SArray * pData)
{
	P_PckgList = pData;
	updateList();
	return 1;
}

void SelPckgDialog::updateList()
{
	if(P_PckgList) {
		PckgViewListEntry * p_entry;
		for(uint i = 0; P_PckgList->enumItems(&i, (void **)&p_entry);) {
			StringSet ss(SLBColumnDelim);
			ss.add(p_entry->Code);
			if(!P_List->addItem(p_entry->ID, ss.getBuf())) {
				PPError();
				return;
			}
		}
		P_List->Draw_();
	}
}

int SelPckgDialog::getSelectedPckgID(PPID * pID)
{
	if(SmartListBox::IsValidS(P_List))
		P_List->getCurID(&SelPckgID);
	else
	   	SelPckgID = 0;
	ASSIGN_PTR(pID, SelPckgID);
	return 1;
}

IMPL_HANDLE_EVENT(SelPckgDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmaMore)
			viewDetail();
		/*
		else if(TVCMD == cmSelPckgComplect || TVCMD == cmaInsert) {
			if(state & sfModal)
				endModal(cmaInsert);
			else
				return;
		}
		*/
		else
			return;
	}
	else if(TVKEYDOWN)
		if(TVKEY == KB_CTRLENTER)
			if(IsInState(sfModal)) {
				if(SmartListBox::IsValidS(P_List))
					P_List->getCurID(&SelPckgID);
				else
   					SelPckgID = 0;
				endModal(cmOK);
			}
			else
				return;
		else if(TVKEY == kbF3)
			viewDetail();
		else
			return;
	else
		return;
	clearEvent(event);
}

int PPObjBill::SelectPckg(PckgFilt * pFilt, PPID * pPckgID)
{
	int    ok = -1;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		SArray list(sizeof(PckgViewListEntry));
		SelPckgDialog * dlg = new SelPckgDialog(this);
		if(CheckDialogPtrErr(&dlg)) {
			if(CreatePckgViewList(P_PckgT, &trfr->Rcpt, pFilt, &list)) {
				dlg->setList(&list);
		        int r = ExecView(dlg);
				if(r == cmOK) {
					dlg->getSelectedPckgID(pPckgID);
					ok = 1;
				}
		        else if(r == cmaInsert) {
		        	ok = 2;
		        }
			}
			else
				ok = PPErrorZ();
		}
		else
			ok = 0;
		delete dlg;
	}
	return ok;
}

#if 0 // {

struct PckgViewItem {
	PPID   ID;           // -> Receipt.ID
	PPID   PckgTypeID;   // -> Goods2.ID (Goods2.Kind = PPGDSK_PCKGTYPE)
	char   Code[16];
	int16  Closed;
	int16  UniqCntr;
	long   Flags;

	PPID   LotID;
	PPID   GoodsID;
	double Qtty;
	double PhQtty;
	double Cost;
	double Price;
};

class PPViewPckg {
public:
	PPViewPckg();
	~PPViewPckg();

	const  PckgFilt * GetFilt() const;
	int    EditFilt(PckgFilt *);

	int    Init(PckgFilt *);
	int    InitIteration();
	int    NextIteration(PckgViewItem *);
	int    GetIterationCount(long * pNumIterations, long * pLastCount);

	int    Browse(int modeless);
	int    Print();
	int    ViewTotal();
	int    ViewDetail(PPID);
private:
	DBQuery * CreateBrowserQuery();
	int    CreateTempTable();

	PckgFilt Filt;
	//BExtQuery * P_IterQuery;

	long   NumIters;
	long   IterCount;

	//PPObjGoods GdsObj;
	//PPObjPerson PsnObj;
	//PPObjLocation LocObj;
	//PPObjBill * P_BObj;
};
//
//
//
PPViewPckg::PPViewPckg()
{
}

PPViewPckg::~PPViewPckg()
{
}

const PckgFilt * PPViewPckg::GetFilt() const { return &Filt; }

int PPViewPckg::Init(PckgFilt * pFilt)
{
	Filt = *pFilt;
	return 1;
}

#endif // 0 }
