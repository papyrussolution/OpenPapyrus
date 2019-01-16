// PAYMENT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop

struct _PaymentEntry {
	LDATE  Date;           // Дата документа
	char   Code[24];       // Номер документа
	char   StatusName[48]; //
	char   OpName[48];     // Наименование вида операции
	double Amount;         // Сумма документа
	double Payment;        // Сумма учтенная как оплата
	double Rest;           // Остаток долга
	char   Memo[256];      // Примечание
	//
	PPID   ID;
	PPID   CurID;
	int16  LinkKind;   // 1 - link, 2 - pool, 3 - writeoff link
	int16  Pad2;       // @alignment
	PPID   LinkBillID; //
	PPID   RcknBillID; //
};
//
// @ModuleDef(PPViewPaymBill)
//
// класс используется только для перечисления оплат
struct PaymBillFilt {
	PaymBillFilt() : LinkBillID(0), LinkOpID(0), Kind(0)
	{
	}
	PPID   LinkBillID;
	PPID   LinkOpID;
	int    Kind;
};

typedef _PaymentEntry PaymBillViewItem;
//
// ARG(id   ID): Документ, привязки к которому следует показать
// ARG(kind IN):
//   0 - Оплаты по документу
//   1 - Начисления ренты по документу
//   2 - Зачитывающие оплаты по зачетному документу
//   3 - Зачтенные документы по зачетному документу
//   reserved 4 - Зачетные документы, соответствующие оплатам долгового документа
//   5 - Документы списания драфт-документа
//
SArray * SLAPI PPObjBill::MakePaymentList(PPID id, int kind)
{
	_PaymentEntry entry, * p_entry;
	uint   i;
	int    r;
	uint   blnk = 0;
	PPID   member_id = 0;
	PPObjBillStatus bs_obj;
	PPBillStatus bs_rec;
	BillTbl::Rec bill_rec;
	double debt, total = 0.0;
	DateIter  diter;
	SArray  * p_ary = 0;
	THROW_MEM(p_ary = new SArray(sizeof(entry)));
	THROW(P_Tbl->Search(id, &bill_rec) > 0);
	debt = BR2(bill_rec.Amount);
	if(kind == 0)
		blnk = (BLNK_PAYMRETN | BLNK_CHARGEPAYM);
	else if(kind == 1)
		blnk = BLNK_CHARGE;
	else if(kind == 5)
		blnk = BLNK_WROFFDRAFT;
	else
		blnk = BLNK_PAYMRETN;
	while(1) {
		int    is_paym_charge = 0;
		int16  link_kind = 0;
		if(oneof2(kind, 0, 1)) {
			int    paym_r = P_Tbl->EnumLinks(id, &diter, blnk, &bill_rec);
			if(paym_r < 0) {
				do {
					r = P_Tbl->EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &member_id);
				} while(r > 0 && P_Tbl->Search(member_id, &bill_rec) <= 0);
				link_kind = 2;
			}
			else {
				if(CheckOpFlags(bill_rec.OpID, OPKF_CHARGENEGPAYM))
					is_paym_charge = 1;
				r = 1;
				link_kind = 1;
			}
		}
		else if(kind == 2) {
			do {
				r = P_Tbl->EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &member_id);
			} while(r > 0 && P_Tbl->Search(member_id, &bill_rec) <= 0);
			link_kind = 2;
		}
		else if(kind == 3) {
			do {
				r = P_Tbl->EnumMembersOfPool(PPASS_PAYMBILLPOOL, id, &member_id);
			} while(r > 0 && (P_Tbl->Search(member_id, &bill_rec) <= 0 || P_Tbl->Search(bill_rec.LinkBillID, &bill_rec) <= 0));
		}
		else if(kind == 5) {
			//
			// Документы списания драфт-документа
			//
			r = P_Tbl->EnumLinks(id, &diter, blnk, &bill_rec);
			link_kind = 3;
		}
		if(r > 0) {
			if(ObjRts.CheckOpID(bill_rec.OpID, PPR_READ)) {
				PPID   pool_id = 0;
				int    no_paym = 0; // Статус документа предписывает не учитывать оплату
				MEMSZERO(entry);
				entry.ID    = bill_rec.ID;
				entry.LinkBillID = bill_rec.LinkBillID;
				if(IsMemberOfPool(bill_rec.ID, PPASS_PAYMBILLPOOL, &pool_id) > 0)
					entry.RcknBillID = pool_id;
				entry.Date  = bill_rec.Dt;
				entry.CurID = bill_rec.CurID;
				entry.LinkKind = link_kind;
				STRNSCPY(entry.Code, bill_rec.Code);
				if(bill_rec.StatusID && bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0) {
					no_paym = BIN(bs_rec.Flags & BILSTF_LOCK_PAYMENT);
					STRNSCPY(entry.StatusName, bs_rec.Name);
				}
				STRNSCPY(entry.Memo, bill_rec.Memo);
				GetOpName(bill_rec.OpID, entry.OpName, sizeof(entry.OpName));
				entry.Amount = bill_rec.Amount;
				entry.Payment = no_paym ? 0.0 : BR2(is_paym_charge ? -bill_rec.Amount : bill_rec.Amount);
				if(oneof2(kind, 1, 3)) { // charge or reckoned bills
					double temp = 0.0;
					P_Tbl->GetAmount(entry.ID, PPAMT_PAYMENT, entry.CurID, &temp);
					entry.Rest = entry.Payment - temp;
				}
				else {
					entry.Rest = 0.0;
					total += entry.Payment;
				}
				THROW_SL(p_ary->insert(&entry));
			}
		}
		else
			break;
	}
	THROW(r);
	p_ary->sort(CMPF_LONG);
	if(kind != 1)
		for(i = 0; p_ary->enumItems(&i, (void**)&p_entry);)
			p_entry->Rest = (debt -= p_entry->Payment);
	CATCH
		ZDELETE(p_ary);
	ENDCATCH
	return p_ary;
}

IMPLEMENT_PPFILT_FACTORY(LinkedBill); SLAPI LinkedBillFilt::LinkedBillFilt() : PPBaseFilt(PPFILT_LINKEDBILL, 0, 1)
{
	SetFlatChunk(offsetof(LinkedBillFilt, ReserveStart),
		offsetof(LinkedBillFilt, ReserveEnd)-offsetof(LinkedBillFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

SLAPI PPViewLinkedBill::PPViewLinkedBill() : PPView(0, &Filt, PPVIEW_LINKEDBILL), P_BObj(BillObj), PrevPaym(0.0), PrevKind(-1)
{
	ImplementFlags |= implBrowseArray;
	MEMSZERO(Rec);
}

SLAPI PPViewLinkedBill::~PPViewLinkedBill()
{
}

PPBaseFilt * SLAPI PPViewLinkedBill::CreateFilt(void * extraPtr) const
{
	LinkedBillFilt * p_filt = new LinkedBillFilt;
	if(p_filt)
		p_filt->BillID = (long)extraPtr;
	else
		PPSetErrorNoMem();
	return p_filt;
}

int SLAPI PPViewLinkedBill::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return 1;
}

int SLAPI PPViewLinkedBill::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PrevKind = -1;
	MEMSZERO(Rec);
	THROW(P_BObj->Search(Filt.BillID, &Rec) > 0);
	THROW(MakeList());
	CATCHZOK
	return ok;
}

int SLAPI PPViewLinkedBill::InitIteration()
{
	Counter.Init(List.getCount());
	return 1;
}

int FASTCALL PPViewLinkedBill::NextIteration(LinkedBillViewItem * pItem)
{
	int    ok = -1;
	if(pItem && P_BObj) {
		while(Counter < Counter.GetTotal() && ok < 0) {
			if(pItem) {
				const Entry & r_entry = List.at(Counter);
				if(P_BObj->Search(r_entry.ID, pItem) > 0) { // @v7.0.12 Fetch-->Search (кэш не хранит примечание к документу)
					pItem->Payment = r_entry.Payment;
					pItem->Rest = r_entry.Rest;
					pItem->LinkBillID = r_entry.LinkBillID;
					pItem->RcknBillID = r_entry.RcknBillID;
					ok = 1;
				}
			}
			else
				ok = 1;
			Counter.Increment();
		}
	}
	return ok;
}

int SLAPI PPViewLinkedBill::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		Entry * p_item = (Entry *)pBlk->P_SrcData;
		PPID   bill_id = p_item->ID;
		BillTbl::Rec bill_rec;
		//const  size_t src_data_size = stsize(pBlk->TypeID);
		int    r = 0;
		if(P_BObj->Fetch(bill_id, &bill_rec) > 0)
			r = 1;
		else
			MEMSZERO(bill_rec);
		switch(pBlk->ColumnN) {
			case 0: // @id
				pBlk->Set(bill_id);
				break;
			case 1: // @date
				pBlk->Set(bill_rec.Dt);
				break;
			case 2: // @code
				pBlk->Set(bill_rec.Code);
				break;
			case 3: // @billstatus.name
				{
					PPObjBillStatus bs_obj;
					PPBillStatus bs_rec;
					if(bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0)
						pBlk->Set(bs_rec.Name);
					else
						pBlk->SetZero();
				}
				break;
			case 4: // @oprkind.name
				GetOpName(bill_rec.OpID, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 5: // @amount
				pBlk->Set(bill_rec.Amount);
				break;
			case 6: // @payment
				pBlk->Set(p_item->Payment);
				break;
			case 7: // @rest
				pBlk->Set(p_item->Rest);
				break;
			case 8: // @memo
				MemoList.GetText(bill_rec.ID, temp_buf);
				pBlk->Set(temp_buf);
				break;
		}
	}
	return ok;
}

// static
int FASTCALL PPViewLinkedBill::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewLinkedBill * p_v = (PPViewLinkedBill *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

void SLAPI PPViewLinkedBill::PreprocessBrowser(PPViewBrowser * pBrw)
{
	CALLPTRMEMB(pBrw, SetDefUserProc(PPViewLinkedBill::GetDataForBrowser, this));
}

SArray  * SLAPI PPViewLinkedBill::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(List);
	uint   brw_id = 0;
	if(Filt.Kind == LinkedBillFilt::lkPayments) {
		brw_id = BROWSER_PAYMENTS;
		DateIter di;
		if(P_BObj->P_Tbl->EnumLinks(Filt.BillID, &di, BLNK_RETURN) > 0 || (Rec.Flags & BILLF_NEEDPAYMENT))
			P_BObj->P_Tbl->GetAmount(Filt.BillID, PPAMT_PAYMENT, Rec.CurID, &PrevPaym);
	}
	else if(Filt.Kind == LinkedBillFilt::lkCharge) {
		brw_id = BROWSER_CHARGES;
		//if(Rec.Flags & BILLF_RENT)
	}
	else if(Filt.Kind == LinkedBillFilt::lkReckon) {
		brw_id = BROWSER_RECKONINGS;
		if(CheckOpFlags(Rec.OpID, OPKF_RECKON))
			P_BObj->P_Tbl->GetAmount(Filt.BillID, PPAMT_PAYMENT, Rec.CurID, &PrevPaym);
	}
	else if(Filt.Kind == LinkedBillFilt::lkWrOffDraft) {
		brw_id = BROWSER_DRAFTWROFFS;
	}
	if(pSubTitle) {
		P_BObj->MakeCodeString(&Rec, PPObjBill::mcsAddOpName, *pSubTitle);
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewLinkedBill::MakeList()
{
	List.freeAll();
	MemoList.Z();

	int    ok = 1;
	BillCore * p_bt = P_BObj->P_Tbl;
	Entry  entry, * p_entry;
	uint   blnk = 0;
	PPID   member_id = 0;
	PPObjBillStatus bs_obj;
	PPBillStatus bs_rec;
	BillTbl::Rec bill_rec;
	double debt, total = 0.0;
	DateIter  diter;
	THROW(P_BObj->Search(Filt.BillID, &bill_rec) > 0);
	debt = BR2(bill_rec.Amount);
	if(Filt.Kind == LinkedBillFilt::lkPayments)
		blnk = (BLNK_PAYMRETN | BLNK_CHARGEPAYM);
	else if(Filt.Kind == LinkedBillFilt::lkCharge)
		blnk = BLNK_CHARGE;
	else if(Filt.Kind == LinkedBillFilt::lkWrOffDraft)
		blnk = BLNK_WROFFDRAFT;
	else
		blnk = BLNK_PAYMRETN;
	for(int r = 1; r > 0;) {
		int    is_paym_charge = 0;
		int16  link_kind = 0;
		if(oneof2(Filt.Kind, LinkedBillFilt::lkPayments, LinkedBillFilt::lkCharge)) {
			int    paym_r = p_bt->EnumLinks(Filt.BillID, &diter, blnk, &bill_rec);
			if(paym_r < 0) {
				do {
					r = p_bt->EnumMembersOfPool(PPASS_PAYMBILLPOOL, Filt.BillID, &member_id);
				} while(r > 0 && P_BObj->Search(member_id, &bill_rec) <= 0);
				link_kind = 2;
			}
			else {
				if(CheckOpFlags(bill_rec.OpID, OPKF_CHARGENEGPAYM))
					is_paym_charge = 1;
				r = 1;
				link_kind = 1;
			}
		}
		else if(Filt.Kind == LinkedBillFilt::lkReckon) {
			do {
				r = p_bt->EnumMembersOfPool(PPASS_PAYMBILLPOOL, Filt.BillID, &member_id);
			} while(r > 0 && P_BObj->Search(member_id, &bill_rec) <= 0);
			link_kind = 2;
		}
		else if(Filt.Kind == LinkedBillFilt::lkByReckon) {
			do {
				r = p_bt->EnumMembersOfPool(PPASS_PAYMBILLPOOL, Filt.BillID, &member_id);
			} while(r > 0 && (P_BObj->Search(member_id, &bill_rec) <= 0 || P_BObj->Search(bill_rec.LinkBillID, &bill_rec) <= 0));
		}
		else if(Filt.Kind == LinkedBillFilt::lkWrOffDraft) {
			//
			// Документы списания драфт-документа
			//
			r = p_bt->EnumLinks(Filt.BillID, &diter, blnk, &bill_rec);
			link_kind = 3;
		}
		THROW(r)
		if(r > 0) {
			if(ObjRts.CheckOpID(bill_rec.OpID, PPR_READ)) {
				PPID   pool_id = 0;
				int    no_paym = 0; // Статус документа предписывает не учитывать оплату
				MEMSZERO(entry);
				entry.ID    = bill_rec.ID;
				entry.LinkBillID = bill_rec.LinkBillID;
				if(P_BObj->IsMemberOfPool(bill_rec.ID, PPASS_PAYMBILLPOOL, &pool_id) > 0)
					entry.RcknBillID = pool_id;
				entry.LinkKind = link_kind;
				if(bill_rec.StatusID && bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0)
					no_paym = BIN(bs_rec.Flags & BILSTF_LOCK_PAYMENT);
				entry.Payment = no_paym ? 0.0 : BR2(is_paym_charge ? -bill_rec.Amount : bill_rec.Amount);
				if(oneof2(Filt.Kind, LinkedBillFilt::lkCharge, LinkedBillFilt::lkByReckon)) { // charge or reckoned bills
					double temp = 0.0;
					p_bt->GetAmount(entry.ID, PPAMT_PAYMENT, bill_rec.CurID, &temp);
					entry.Rest = entry.Payment - temp;
				}
				else {
					entry.Rest = 0.0;
					total += entry.Payment;
				}
				THROW_SL(List.insert(&entry));
				if(bill_rec.Memo[0])
					MemoList.Add(bill_rec.ID, bill_rec.Memo);
			}
		}
	}
	List.sort(CMPF_LONG);
	if(Filt.Kind != LinkedBillFilt::lkCharge) {
		for(uint i = 0; List.enumItems(&i, (void**)&p_entry);)
			p_entry->Rest = (debt -= p_entry->Payment);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewLinkedBill::ViewTotal()
{
	TDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_PAYMTOTAL)))) {
		long   count = 0;
		double amount = 0.0;
		for(uint i = 0; i < List.getCount(); i++) {
			BillTbl::Rec bill_rec;
			if(P_BObj && P_BObj->Fetch(List.at(i).ID, &bill_rec) > 0) {
				amount += bill_rec.Amount;
			}
			count++;
		}
		dlg->setCtrlLong(CTL_PAYMTOTAL_COUNT, count);
		dlg->setCtrlReal(CTL_PAYMTOTAL_AMOUNT, amount);
		ExecViewAndDestroy(dlg);
	}
	return -1;
}

int SLAPI PPViewLinkedBill::Print(const void * pHdr)
{
	int    ok = -1;
	if(pHdr) {
		PPBillPacket pack;
		PPID   bill_id = pHdr ? *(PPID *)pHdr : 0;
		if(bill_id) {
			if(P_BObj->ExtractPacket(bill_id, &pack))
				if(Filt.Kind == LinkedBillFilt::lkWrOffDraft || pack.Rec.Flags & BILLF_BANKING || CheckOpPrnFlags(pack.Rec.OpID, OPKF_PRT_INVOICE))
					PrintGoodsBill(&pack);
				else
					PrintCashOrderByGoodsBill(&pack);
			else
				ok = PPErrorZ();
		}
	}
	else
		Helper_Print(REPORT_PAYMBILLLIST, 0);
	return ok;
}

int SLAPI PPViewLinkedBill::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd == PPVCMD_PRINT) ? -2 : PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		AryBrowserDef * p_def = pBrw ? (AryBrowserDef *)pBrw->getDef() : 0;
		const  long cur_pos = p_def ? p_def->_curItem() : 0;
		long   update_pos = cur_pos;
		PPID   update_id = 0;
		PPID   bill_id = pHdr ? *(long *)pHdr : 0;
		BillTbl::Rec bill_rec;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = -1;
				{
					PPID   op_type_id = 0;
					if(Filt.Kind == LinkedBillFilt::lkPayments)
						op_type_id = PPOPT_PAYMENT;
					else if(Filt.Kind == LinkedBillFilt::lkCharge)
	   					op_type_id = PPOPT_CHARGE;
					else if(Filt.Kind == LinkedBillFilt::lkWrOffDraft) {
						if(P_BObj->Search(Filt.BillID, &bill_rec) > 0) {
							PPObjOprKind op_obj;
							PPDraftOpEx doe;
							if(op_obj.GetDraftExData(bill_rec.OpID, &doe) > 0) {
								if(doe.WrOffOpID) {
									if(List.getCount() == 0 || (doe.Flags & DROXF_MULTWROFF)) {
										PPObjBill::AddBlock ab;
										ab.LinkBillID = Filt.BillID;
										ab.OpID = doe.WrOffOpID;
										ab.ObjectID = NZOR(bill_rec.Object, doe.WrOffObjID);
										if(P_BObj->AddGoodsBill(&update_id, &ab) == cmOK)
											ok = 1;
									}
								}
							}
						}
					}
					if(op_type_id) {
						PPObjBill::AddBlock ab;
						ab.LinkBillID = Rec.ID;
						ab.OpID = SelectOprKind(0, Rec.OpID, op_type_id, 0L);
						if(ab.OpID == 0)
							PPError();
						else if(ab.OpID > 0 && P_BObj->AddGoodsBill(&update_id, &ab) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(bill_id && P_BObj->EditGoodsBill(bill_id, 0) == cmOK) {
					update_id = bill_id;
					ok = 1;
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				{
					int    done = 0;
					if(bill_id && P_BObj->Search(bill_id, &bill_rec) > 0) {
						if(oneof3(Filt.Kind, LinkedBillFilt::lkPayments, LinkedBillFilt::lkReckon, LinkedBillFilt::lkWrOffDraft)) {
							if(!IsOpPaymOrRetn(bill_rec.OpID)) {
								uint   v = 0;
								if(SelectorDialog(DLG_SELRMVPAYM, CTL_SELRMVPAYM_WHAT, &v) > 0) {
									if(v == 1) {
										const Entry * p_entry = (const Entry *)pHdr;
										if(p_entry && P_BObj->P_Tbl->BreakOffLink(bill_id, Filt.BillID, p_entry->LinkKind, 1) > 0) {
											update_pos = (cur_pos > 0) ? (cur_pos-1) : 0;
											ok = 1;
										}
									}
									else if(v == 0) {
										if(P_BObj->RemoveObjV(bill_id, 0, PPObject::rmv_default, 0) > 0) {
											update_pos = (cur_pos > 0) ? (cur_pos-1) : 0;
											ok = 1;
										}
									}
								}
								done = 1;
							}
						}
						if(!done && P_BObj->RemoveObjV(bill_id, 0, PPObject::rmv_default, 0) > 0) {
							update_pos = (cur_pos > 0) ? (cur_pos-1) : 0;
							ok = 1;
						}
					}
				}
				break;
			case PPVCMD_ADDBYSAMPLE:
				ok = -1;
				{
					PPObjBill::AddBlock ab;
					ab.SampleBillID = bill_id;
					if(oneof2(Filt.Kind, LinkedBillFilt::lkPayments, LinkedBillFilt::lkCharge) && ab.SampleBillID) {
						if(P_BObj->AddGoodsBill(&bill_id, &ab) == cmOK) {
							update_id = bill_id;
							ok = 1;
						}
					}
				}
				break;
			case PPVCMD_CHANGESTATUS:
				ok = -1;
				if(P_BObj->EditBillStatus(bill_id) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_SETWLABEL:
				ok = -1;
				if(bill_id) {
					P_BObj->SetWLabel(bill_id, -1);
					ok = 1;
				}
				break;
			case PPVCMD_ACCTURNSBYBILL:
				ok = -1;
				P_BObj->ViewAccturns(bill_id);
				break;
			case PPVCMD_VIEWOP:
				{
					PPObjOprKind op_obj;
					ok = (P_BObj->Search(bill_id, &bill_rec) > 0 && op_obj.Edit(&bill_rec.OpID, 0) == cmOK) ? 1 : -1;
				}
				break;
			case PPVCMD_TOGGLE:
				if(PrevKind >= 0) {
					Filt.Kind = PrevKind;
					PrevKind = -1;
					ok = 1;
				}
				else if(Filt.Kind == LinkedBillFilt::lkPayments) {
					PrevKind = Filt.Kind;
					Filt.Kind = LinkedBillFilt::lkByReckon;
					ok = 1;
				}
				else if(Filt.Kind == LinkedBillFilt::lkReckon) {
					PrevKind = Filt.Kind;
					Filt.Kind = LinkedBillFilt::lkByReckon;
					ok = 1;
				}
				else if(Filt.Kind == LinkedBillFilt::lkByReckon) {
					PrevKind = Filt.Kind;
					Filt.Kind = LinkedBillFilt::lkReckon;
					ok = 1;
				}
				break;
			case PPVCMD_PAYMENT:
				ok = -1;
				if(oneof2(Filt.Kind, LinkedBillFilt::lkCharge, LinkedBillFilt::lkWrOffDraft) && P_BObj->ViewPayments(bill_id, 0) > 0) {
					ok = 1;
				}
				break;
			case PPVCMD_CHARGE:
				ok = -1;
				if(Filt.Kind == LinkedBillFilt::lkCharge && Filt.BillID && P_BObj->AutoCharge(Filt.BillID) > 0)
					ok = 1;
				break;
			case PPVCMD_COMPARE:
				ok = -1;
				if(Filt.Kind == LinkedBillFilt::lkWrOffDraft && bill_id) {
					PPIDArray rh_bill_list;
					if(P_BObj->GetComplementGoodsBillList(bill_id, rh_bill_list) > 0)
						ViewGoodsBillCmp(bill_id, rh_bill_list, 0);
				}
				break;
			case PPVCMD_PRINT:
				ok = Print(pHdr);
				break;
			case PPVCMD_POSPRINTBYBILL:
				P_BObj->PosPrintByBill(bill_id);
				break;
			case PPVCMD_PRINTLIST:
				ok = Print(0);
				break;
		}
		if(ok > 0) {
			MakeList();
			if(pBrw) {
				if(p_def) {
					SArray * p_array = new SArray(List);
					p_def->setArray(p_array, 0, 1);
					pBrw->setRange(p_array->getCount());
					uint   temp_pos = 0;
					if(update_id > 0 && List.lsearch(&update_id, &temp_pos, CMPF_LONG))
						update_pos = temp_pos;
					if(update_pos >= 0)
						pBrw->go(update_pos);
					else if(update_pos == MAXLONG)
						pBrw->go(p_array->getCount() - 1);
				}
				pBrw->Update();
			}
		}
	}
	return ok;
}

int SLAPI PPObjBill::ViewPayments(PPID id, int kind)
{
	//static int SLAPI Execute(int viewID, const PPBaseFilt * pFilt, int asModeless, long extraParam = 0);
	LinkedBillFilt filt;
	filt.BillID = id;
	filt.Kind = kind;
	int    ok = PPView::Execute(PPVIEW_LINKEDBILL, &filt, 0, 0);
	return ok;
}
//
// Reckoning
//
SLAPI ReckonOpArItem::ReckonOpArItem()
{
	THISZERO();
}

SLAPI ReckonOpArList::ReckonOpArList() : TSArray <ReckonOpArItem> ()
{
}

SLAPI ReckonOpArList::~ReckonOpArList()
{
	Destroy();
}

void SLAPI ReckonOpArList::Destroy()
{
	ReckonOpArItem * p_item;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		ZDELETE(p_item->P_BillIDList);
}

PPID FASTCALL ReckonOpArList::GetPaymOpIDByBillID(PPID billID) const
{
	ReckonOpArItem * p_item = 0;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(p_item->P_BillIDList && p_item->P_BillIDList->lsearch(billID))
			return p_item->PaymentOpID;
	return 0L;
}

int SLAPI ReckonOpArList::IsBillListSortingNeeded() const
{
	ReckonOpArItem * p_item = 0;
	int    c = 0;
	for(uint i = 0; enumItems(&i, (void**)&p_item);)
		if(p_item->P_BillIDList && p_item->P_BillIDList->getCount())
			c++;
	return (c > 1) ? 1 : 0;
}

int SLAPI PPObjBill::Reckon(PPID paymBillID, PPID debtBillID, PPID reckonOpKindID, PPID * pReckonBillID,
	int dateOption /* RECKON_DATE_XXX */, LDATE reckonDate, int use_ta)
{
	int    ok = 1;
	PPID   cur_id = 0;
	double amt_debt = 0.0, amt_tmp = 0.0;
	double amt_paym = 0.0;
	double amt_reckon = 0.0;
	BillTbl::Rec bill_rec, paym_bill_rec;

	ASSIGN_PTR(pReckonBillID, 0L);
	THROW(Search(debtBillID, &bill_rec) > 0);
	cur_id = bill_rec.CurID;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(paymBillID, &paym_bill_rec) > 0);
		THROW_PP(cur_id == paym_bill_rec.CurID, PPERR_INCOMPPAYMCUR);
		P_Tbl->GetAmount(paymBillID, PPAMT_MAIN,    cur_id, &amt_paym);
		P_Tbl->GetAmount(paymBillID, PPAMT_PAYMENT, cur_id, &amt_tmp);
		amt_paym  -= amt_tmp;
		P_Tbl->GetAmount(debtBillID, PPAMT_PAYMENT, cur_id, &amt_tmp);
		amt_debt   = BR2(bill_rec.Amount) - amt_tmp;
		amt_reckon = MIN(amt_debt, amt_paym);
		if(amt_reckon > 0.0) {
			AmtList al;
			PPBillPacket pack;
			THROW(pack.CreateBlank(reckonOpKindID, debtBillID, bill_rec.LocID, 0));
			pack.SetPoolMembership(PPBillPacket::bpkReckon, paymBillID);
			pack.Rec.LocID = bill_rec.LocID;
			if(dateOption == RECKON_DATE_CURR)
				pack.Rec.Dt = LConfig.OperDate;
			else if(dateOption == RECKON_DATE_PAYM)
				pack.Rec.Dt = paym_bill_rec.Dt;
			else if(dateOption == RECKON_DATE_DEBT)
				pack.Rec.Dt = bill_rec.Dt;
			else if(dateOption == RECKON_DATE_USER)
				pack.Rec.Dt = reckonDate;
			else
				pack.Rec.Dt = bill_rec.Dt;
			if(pack.Rec.Dt < bill_rec.Dt)
				pack.Rec.Dt = bill_rec.Dt;
			pack.Rec.CurID  = cur_id;
			pack.Rec.Amount = BR2(amt_reckon);
			al.Put(PPAMT_MAIN, cur_id, amt_reckon, 0, 0);
			if(cur_id) {
				double crate = 1.0;
				LDATE  crate_date = pack.Rec.Dt;
				GetCurRate(cur_id, &crate_date, &crate);
				al.Put(PPAMT_CRATE, cur_id, crate, 0, 0);
			}
			SubstMemo(&pack);
			THROW(pack.SumAmounts(&al));
			THROW(pack.InitAmounts(&al));
			THROW(FillTurnList(&pack));
			THROW(TurnPacket(&pack, 0));
			ASSIGN_PTR(pReckonBillID, pack.Rec.ID);
		}
		else
			ok = -1;
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		ASSIGN_PTR(pReckonBillID, 0L);
	ENDCATCH
	return ok;
}

int SLAPI PPObjBill::GetAlternateArticle(PPID arID, PPID sheetID, PPID * pAltArID)
{
	int    ok = -1;
	PPObjAccSheet acs_obj;
	PPAccSheet main_acs_rec, alt_acs_rec;
	ArticleTbl::Rec ar_rec;
	ASSIGN_PTR(pAltArID, 0L);
	THROW(acs_obj.Fetch(sheetID, &alt_acs_rec) > 0); // shr
	THROW(ArObj.Fetch(arID, &ar_rec) > 0);
	THROW(acs_obj.Fetch(ar_rec.AccSheetID, &main_acs_rec) > 0); // s
	if(alt_acs_rec.Assoc == main_acs_rec.Assoc && main_acs_rec.Assoc == PPOBJ_PERSON) {
		PPObjPerson po;
		if(po.P_Tbl->IsBelongToKind(ar_rec.ObjID, alt_acs_rec.ObjGroup) > 0 && ArObj.P_Tbl->SearchObjRef(sheetID, ar_rec.ObjID, &ar_rec) > 0) {
			ASSIGN_PTR(pAltArID, ar_rec.ID);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::GetPayableOpListByReckonOp(const PPReckonOpEx * pRcknData, PPID arID, ReckonOpArList * pList)
{
	int    ok = -1;
	uint   i;
	PPID   paym_sheet_id = 0;
	ArticleTbl::Rec ar_rec;
	THROW(ArObj.Fetch(arID, &ar_rec) > 0);
	paym_sheet_id = ar_rec.AccSheetID;
	for(i = 0; i < pRcknData->OpList.getCount(); i++) {
		PPID   article_id = 0;
		PPOprKind opk;
		PPID   paym_op_id = pRcknData->OpList.at(i);
		GetOpData(paym_op_id, &opk);
		if(opk.LinkOpID) {
			GetOpData(opk.LinkOpID, &opk);
			PPID   op_id        = opk.ID;
			PPID   acc_sheet_id = opk.AccSheetID;
			long   opk_flags    = opk.Flags;
			if(!acc_sheet_id && opk.LinkOpID) {
				GetOpData(opk.LinkOpID, &opk);
				acc_sheet_id = opk.AccSheetID;
			}
			if(acc_sheet_id && opk_flags & OPKF_NEEDPAYMENT) {
				if(acc_sheet_id != paym_sheet_id) {
					if(GetAlternateArticle(arID, acc_sheet_id, &article_id) <= 0)
						continue;
				}
				else
					article_id = arID;
			}
			if(article_id) {
				const  PPID rel_type_id = NZOR(pRcknData->PersonRelTypeID, PPPSNRELTYP_AFFIL);
				PPIDArray ar_list;
				ArObj.GetRelPersonList(article_id, rel_type_id, 1, &ar_list);
				ar_list.addUnique(article_id);
				for(uint j = 0; j < ar_list.getCount(); j++) {
					ReckonOpArItem item;
					item.PaymentOpID = paym_op_id;
					item.PaymentArID = arID;
					item.PayableOpID = op_id;
					item.PayableArID = ar_list.at(j);
					ok = 1;
					if(pList)
						THROW_SL(pList->insert(&item));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::GetPaymentOpListByDebtOp(PPID debtOpID, PPID arID, ReckonOpArList * pList)
{
	int    ok = 1;
	uint   i, j;
	PPOprKind op_rec;
	PPIDArray op_list, paym_op_list, ar_list;
	PPID   prim_sheet_id = 0; // Таблица, к которой относится статья arID
	ArticleTbl::Rec ar_rec;
	if(arID) {
		THROW(ArObj.Fetch(arID, &ar_rec) > 0);
		prim_sheet_id = ar_rec.AccSheetID;
	}
	THROW(GetReckonOpList(&op_list));
	THROW(P_OpObj->GetPaymentOpList(debtOpID, &paym_op_list));
	for(i = 0; i < op_list.getCount(); i++) {
		PPID   op_id = op_list.at(i);
		PPReckonOpEx reckon_data;
		if(GetOpData(op_id, &op_rec) > 0 && op_rec.Flags & OPKF_RECKON) {
			PPID   acc_sheet_id = op_rec.AccSheetID;
			if(!acc_sheet_id) {
				PPOprKind link_op_rec;
				if(op_rec.LinkOpID && GetOpData(op_rec.LinkOpID, &link_op_rec) > 0)
					acc_sheet_id = link_op_rec.AccSheetID;
			}
			if(acc_sheet_id) {
				PPID   article_id = 0;
				ar_list.clear();
				if(arID) {
					if(acc_sheet_id != prim_sheet_id) {
						if(GetAlternateArticle(arID, acc_sheet_id, &article_id) <= 0)
							continue;
					}
					else
						article_id = arID;
				}
				ar_list.addUnique(article_id);
				THROW(P_OpObj->GetReckonExData(op_id, &reckon_data));
				for(uint k = 0; k < ar_list.getCount(); k++) {
					for(j = 0; j < reckon_data.OpList.getCount(); j++) {
						const  PPID paym_op_id = reckon_data.OpList.get(j);
						if(paym_op_list.lsearch(paym_op_id)) {
							ReckonOpArItem item;
							item.PaymentOpID = paym_op_id;
							item.PaymentArID = arID;
							item.PayableOpID = op_id;
							item.PayableArID = ar_list.get(k);
							item.PayableAccSheetID = acc_sheet_id;
							THROW_SL(pList->insert(&item));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::GatherPayableBills(ReckonOpArItem * pItem, PPID curID,
	PPID locID, PPID obj2ID, const DateRange * pPeriod, double * pDebt)
{
	int    ok = 1;
	double debt = 0.0;
	BillTbl::Key3 k;
	DBQ  * dbq = 0;
	int    check_payout_flag = CheckOpFlags(pItem->PayableOpID, OPKF_RECKON) ? 0 : 1;
	k.Object = pItem->PayableArID;
	k.Dt = pPeriod ? pPeriod->low : ZERODATE;
	k.BillNo = 0;
	BExtQuery q(P_Tbl, 3);
	q.select(P_Tbl->ID, P_Tbl->Amount, P_Tbl->CurID, P_Tbl->Flags, 0L);
	dbq = & (P_Tbl->Object == pItem->PayableArID && daterange(P_Tbl->Dt, pPeriod) && P_Tbl->OpID == pItem->PayableOpID);
	dbq = ppcheckfiltid(dbq, P_Tbl->LocID, locID);
	if(obj2ID >= 0)
		dbq = & (*dbq && P_Tbl->Object2 == obj2ID);
	if(curID >= 0)
		dbq = & (*dbq && P_Tbl->CurID == curID);
	q.where(*dbq);
	for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
		if(!check_payout_flag || !(P_Tbl->data.Flags & BILLF_PAYOUT)) {
			const PPID   bill_id = P_Tbl->data.ID;
			double payment = 0.0;
			double amount  = BR2(P_Tbl->data.Amount);
			THROW(P_Tbl->GetAmount(bill_id, PPAMT_PAYMENT, curID, &payment));
			debt += (amount - payment);
			if(payment < amount)
				if(pItem->P_BillIDList)
					THROW(pItem->P_BillIDList->add(bill_id));
		}
	}
	if(pDebt)
		*pDebt += debt;
	CATCHZOK
	return ok;
}

class CfmReckoningDialog : public TDialog {
public:
	CfmReckoningDialog(uint dlgID, PPObjBill * pBObj) : TDialog(dlgID), P_BObj(pBObj)
	{
		MEMSZERO(Data);
		P_List = (SmartListBox *)getCtrlView(CTL_CFM_RECKONING_LIST);
		if(!SetupStrListBox(P_List))
			PPError();
		SetupCalDate(CTLCAL_CFM_RECKONING_DT, CTL_CFM_RECKONING_DT);
	}
	int    setDTS(CfmReckoningParam *);
	int    getDTS(CfmReckoningParam *);
private:
	DECL_HANDLE_EVENT;
	int    updateList();
	void   setupDate();
	int    getDateSettings();

	PPObjBill * P_BObj;
	CfmReckoningParam Data;
	SmartListBox * P_List;
};

int CfmReckoningDialog::updateList()
{
	if(P_List && Data.P_BillList) {
		P_List->freeAll();
		StringSet ss(SLBColumnDelim);
		SString sub;
		for(uint i = 0; i < Data.P_BillList->getCount(); i++) {
			BillTbl::Rec bill_rec;
			PPID   bill_id = Data.P_BillList->at(i);
			ss.clear();
			if(P_BObj->Fetch(bill_id, &bill_rec) > 0) {
				double amt = BR2(bill_rec.Amount);
				double paym;
				PPBillExt ext;
				P_BObj->P_Tbl->CalcPayment(bill_rec.ID, 0, 0, Data.CurID, &paym);
				ss.add(sub.Z().Cat(bill_rec.Dt, DATF_DMY));
				ss.add(bill_rec.Code);
				ss.add(sub.Z().Cat(amt, SFMT_MONEY));
				ss.add(sub.Z().Cat(amt-paym, SFMT_MONEY));
				if(P_BObj->FetchExt(bill_id, &ext) > 0 && ext.AgentID) {
					GetArticleName(ext.AgentID, sub);
					ss.add(sub);
				}
				else
					ss.add(sub.Z());
			}
			else
				ss.add(sub.Z().Cat(bill_id));
			P_List->addItem(i+1, ss.getBuf());
		}
		P_List->Draw_();
		return 1;
	}
	else
		return 0;
}

IMPL_HANDLE_EVENT(CfmReckoningDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_CFM_RECKONING_DTSEL)) {
		getDateSettings();
		setupDate();
		clearEvent(event);
	}
	else if(event.isCmd(cmRcknSelectedBill)) {
		if(IsInState(sfModal)) {
			endModal(cmRcknSelectedBill);
			return; // После endModal не следует обращаться к this
		}
	}
	else if(event.isCmd(cmLBDblClk)) {
		if(TVINFOVIEW == P_List && P_List && IsInState(sfModal)) {
			endModal(cmRcknSelectedBill);
			return; // После endModal не следует обращаться к this
		}
	}
	else if(TVBROADCAST && event.isCtlEvent(CTL_CFM_RECKONING_LIST)) {
		enum { all_button, single_button } toggle;
		if(TVCMD == cmReceivedFocus)
			toggle = single_button;
		else if(TVCMD == cmReleasedFocus)
			toggle = all_button;
		else
			return;
		SetDefaultButton(CTL_CFM_RECKONING_SELBUT, toggle == single_button);
		SetDefaultButton(CTL_CFM_RECKONING_ALLBUT, toggle == all_button);
	}
}

void CfmReckoningDialog::setupDate()
{
	ushort v;
	LDATE  dt;
	if(Data.DateOption == RECKON_DATE_CURR) {
		v = 0;
		dt = LConfig.OperDate;
	}
	else if(Data.DateOption == RECKON_DATE_PAYM) {
		v = 1;
		dt = Data.DebtOrPaym ? ZERODATE : Data.BillDt;
	}
	else if(Data.DateOption == RECKON_DATE_DEBT) {
		v = 2;
		dt = Data.DebtOrPaym ? Data.BillDt : ZERODATE;
	}
	else if(Data.DateOption == RECKON_DATE_USER) {
		v  = 3;
		dt = Data.Dt;
	}
	setCtrlData(CTL_CFM_RECKONING_DTSEL, &v);
	setCtrlData(CTL_CFM_RECKONING_DT, &dt);
	disableCtrls(v != 3, CTL_CFM_RECKONING_DT, CTLCAL_CFM_RECKONING_DT, 0);
}

int CfmReckoningDialog::setDTS(CfmReckoningParam * pCRP)
{
	SString obj_name;
	Data = *pCRP;

	SString info_buf;
	SString fmt_buf;
	SString debt_buf, amt_buf;
	SString temp_buf;
	GetArticleName(Data.ArticleID, obj_name);
	debt_buf.Z().Cat(Data.TotalDebt, MKSFMTD(0, 2, 0));
	amt_buf.Z().Cat(Data.PaymAmount, MKSFMTD(0, 2, 0));
	GetCurSymbText(Data.CurID, temp_buf);
	if(Data.DebtOrPaym) {
		//@cfmrcknparam_info_reverse                "По контрагенту '@zstr' существуют документы на сумму @zstr, позволяющие зачесть заданный документ, долг по которому составляет @zstr"
		PPLoadString("cfmrcknparam_info_reverse", fmt_buf);
		PPFormat(fmt_buf, &info_buf, obj_name.cptr(), debt_buf.cptr(), amt_buf.cptr(), temp_buf.cptr());
	}
	else {
		//@cfmrcknparam_info_direct                 "По контрагенту '@zstr' существуют неоплаченные документы на сумму @zstr. Заданный документ позволяет зачесть сумму @zstr"
		PPLoadString("cfmrcknparam_info_direct", fmt_buf);
		PPFormat(fmt_buf, &info_buf, obj_name.cptr(), debt_buf.cptr(), amt_buf.cptr(), temp_buf.cptr());
	}
	setStaticText(CTL_CFM_RECKONING_INFO, info_buf);

	PPGetSubStr(PPTXT_FONTFACE, PPFONT_MSSANSSERIF, temp_buf);
	SetCtrlFont(CTL_CFM_RECKONING_INFO, temp_buf, 16);

	//setCtrlString(CTL_CFM_RECKONING_OBJ, obj_name);
	//setCtrlData(CTL_CFM_RECKONING_DEBT, &pCRP->TotalDebt);
	//setCtrlData(CTL_CFM_RECKONING_AMT,  &pCRP->PaymAmount);
	//setStaticText(CTL_CFM_RECKONING_CURSYM, GetCurSymbText(pCRP->CurID, obj_name));
	//disableCtrls(1, CTL_CFM_RECKONING_OBJ, CTL_CFM_RECKONING_DEBT, CTL_CFM_RECKONING_AMT, 0);
	setupDate();
	updateList();
	return 1;
}

int CfmReckoningDialog::getDateSettings()
{
	int    ok = 1;
	ushort v = 0;
	LDATE  dt;
	getCtrlData(CTL_CFM_RECKONING_DTSEL, &v);
	getCtrlData(CTL_CFM_RECKONING_DT, &dt);
	if(v == 0) {
		Data.DateOption = RECKON_DATE_CURR;
		Data.Dt = LConfig.OperDate;
	}
	else if(v == 1) {
		Data.DateOption = RECKON_DATE_PAYM;
		Data.Dt = Data.BillDt;
	}
	else if(v == 2) {
		Data.DateOption = RECKON_DATE_DEBT;
		Data.Dt = ZERODATE;
	}
	else if(v == 3) {
		Data.DateOption = RECKON_DATE_USER;
		Data.Dt = dt;
		if(!checkdate(dt, 0))
			ok = PPSetError(PPERR_RECKON_INVUSRDT);
	}
	if(ok && !checkdate(Data.Dt, 1))
		ok = PPSetErrorSLib();
	return ok;
}

int CfmReckoningDialog::getDTS(CfmReckoningParam * pCRP)
{
	if(getDateSettings()) {
		Data.SelectedBillID = 0;
		if(P_List && P_List->def && Data.P_BillList) {
			long i = 0;
			P_List->getCurID(&i);
			Data.SelectedBillID = (i > 0 && i <= (long)Data.P_BillList->getCount()) ? Data.P_BillList->at((uint)i-1) : 0;
		}
		ASSIGN_PTR(pCRP, Data);
		return 1;
	}
	else
		return PPErrorZ();
}

static int SLAPI ConfirmReckoning(PPObjBill * pBObj, CfmReckoningParam * pCRP)
{
	int    ok = -1, valid_data = 0, r;
	CfmReckoningDialog * dlg = new CfmReckoningDialog((pCRP->DebtOrPaym ? DLG_CFM_RECKONDEBT : DLG_CFM_RECKONING), pBObj);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pCRP);
		while(!valid_data && ((r = ExecView(dlg)) == cmYes || r == cmRcknSelectedBill))
			if(dlg->getDTS(pCRP)) {
				valid_data = 1;
				ok = (r == cmYes) ? 1 : 2;
			}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

static int SLAPI SortBillListByDate(PPIDArray * pList, PPObjBill * pBObj)
{
	struct _i { // @flat
		LDATE dt;
		PPID  id;
	} item;
	int    ok = 1;
	uint   i;
	SVector temp_list(sizeof(item));
	for(i = 0; i < pList->getCount(); i++) {
		BillTbl::Rec bill_rec;
		if(pBObj->Search(pList->get(i), &bill_rec) > 0) {
			item.dt = bill_rec.Dt;
			item.id = bill_rec.ID;
			THROW_SL(temp_list.insert(&item));
		}
	}
	pList->freeAll();
	temp_list.sort(PTR_CMPFUNC(_2long));
	for(i = 0; i < temp_list.getCount(); i++)
		THROW_SL(pList->add(((_i*)temp_list.at(i))->id));
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::Helper_Reckon(PPID billID, ReckonOpArList * pOpList, CfmReckoningParam * pParam, int dontConfirm, int use_ta)
{
	int    ok = -1, r = 1;
	uint   i;
	PPID   op_id = 0, rckn_id = 0, debt_id = 0, paym_id = 0;
	if(pOpList->IsBillListSortingNeeded() > 0)
		if(!SortBillListByDate(pParam->P_BillList, this))
			ok = 0;
	if(ok) {
		if(pParam->ForceBillID) {
			r = 2;
			dontConfirm = 1;
		}
		if(dontConfirm || (r = ConfirmReckoning(this, pParam)) > 0) {
			if(r == 1) {
				PPID * p_id;
				for(i = 0; pParam->P_BillList->enumItems(&i, (void**)&p_id);) {
					if((op_id = pOpList->GetPaymOpIDByBillID(*p_id)) != 0) {
						debt_id = pParam->DebtOrPaym ? billID : *p_id;
						paym_id = pParam->DebtOrPaym ? *p_id : billID;
						if(Reckon(paym_id, debt_id, op_id, &(rckn_id = 0), pParam->DateOption, pParam->Dt, use_ta)) {
							pParam->ResultBillList.addnz(rckn_id);
							ok = 1;
						}
						else {
							ok = 0;
							break;
						}
					}
				}
			}
			else if(r == 2) {
				PPID   id = NZOR(pParam->ForceBillID, pParam->SelectedBillID);
				if(id > 0 && (op_id = pOpList->GetPaymOpIDByBillID(id)) != 0) {
					debt_id = pParam->DebtOrPaym ? billID : id;
					paym_id = pParam->DebtOrPaym ? id : billID;
					ok = Reckon(paym_id, debt_id, op_id, &rckn_id, pParam->DateOption, pParam->Dt, use_ta) ?
						(pParam->ForceBillID ? 3 : 2) : 0;
					if(ok > 0 && rckn_id)
						pParam->ResultBillList.add(rckn_id);
				}
			}
		}
	}
	return ok;
}

SLAPI CfmReckoningParam::CfmReckoningParam() : P_BillList(0), DebtOrPaym(0), DateOption(RECKON_DATE_PAYM),
	ArticleID(0), CurID(0), TotalDebt(0.0), PaymAmount(0.0), BillDt(ZERODATE), Dt(ZERODATE), ForceBillID(0), SelectedBillID(0)
{

}

void SLAPI CfmReckoningParam::Init(int debtOrPaym, BillTbl::Rec * pRec, double debt, double paym, PPIDArray * pBillList)
{
	DebtOrPaym = debtOrPaym;
	DateOption = RECKON_DATE_PAYM;
	ArticleID  = pRec->Object;
	CurID      = pRec->CurID;
	TotalDebt  = debt;
	PaymAmount = paym;
	BillDt     = pRec->Dt;
	Dt         = ZERODATE;
	ForceBillID = 0;
	SelectedBillID = 0;
	ResultBillList.clear();
	P_BillList = pBillList;
}

static int SLAPI SelectAltObject(PPID * pArID)
{
	class SelectAltReckonObjDialog : public TDialog {
	public:
		SelectAltReckonObjDialog() : TDialog(DLG_REQALTOBJ)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_REQALTOBJ_SHEET)) {
				const PPID acs_id = getCtrlLong(CTLSEL_REQALTOBJ_SHEET);
				SetupArCombo(this, CTLSEL_REQALTOBJ_CLIENT, 0L, OLW_LOADDEFONOPEN, acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
				clearEvent(event);
			}
		}
	};
	int    ok = -1;
	PPID   ar_id = *pArID;
	TDialog * dlg = new SelectAltReckonObjDialog;
	if(CheckDialogPtrErr(&dlg)) {
		PPID   acs_id = 0;
		GetArticleSheetID(ar_id, &acs_id);
		SetupPPObjCombo(dlg, CTLSEL_REQALTOBJ_SHEET, PPOBJ_ACCSHEET, acs_id, 0, 0);
		SetupArCombo(dlg, CTLSEL_REQALTOBJ_CLIENT, ar_id, OLW_LOADDEFONOPEN, acs_id, sacfDisableIfZeroSheet|sacfNonGeneric);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTLSEL_REQALTOBJ_CLIENT, &ar_id);
			if(ar_id) {
				ASSIGN_PTR(pArID, ar_id);
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

void SLAPI PPObjBill::Helper_PopupReckonInfo(PPIDArray & rResultBillList)
{
	SString fmt_buf, msg_buf;
	long   result_count = 0;
	double result_amount = 0.0;
	if(rResultBillList.getCount()) {
		rResultBillList.sortAndUndup();
		for(uint i = 0; i < rResultBillList.getCount(); i++) {
			const PPID result_bill_id = rResultBillList.get(i);
			BillTbl::Rec result_rec;
			if(Search(result_bill_id, &result_rec) > 0) {
				result_count++;
				result_amount += result_rec.Amount;
			}
		}
	}
	if(result_count) {
		PPFormatT(PPTXT_RECKON_NZ_INFO, &msg_buf, result_count, result_amount);
		PPTooltipMessage(msg_buf, 0, 0, 20000, GetColorRef(SClrGreen),
			SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus);
	}
	else {
		PPLoadText(PPTXT_RECKON_ZERO_INFO, msg_buf);
		PPTooltipMessage(msg_buf, 0, 0, 10000, GetColorRef(SClrPink),
			SMessageWindow::fTopmost|SMessageWindow::fSizeByText|SMessageWindow::fPreserveFocus);
	}
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
}

int SLAPI PPObjBill::ReckoningPaym(PPID billID, const ReckonParam & rParam, int use_ta)
{
	int    ok = 1;
	BillTbl::Rec bill_rec;
	PPIDArray paym_op_list;
	ReckonOpArItem * p_item = 0;
	ReckonOpArList op_list;
	THROW(Search(billID, &bill_rec) > 0);
	if(bill_rec.Object && CheckOpFlags(bill_rec.OpID, OPKF_RECKON)) {
		double paym_amt = 0.0;
		P_Tbl->GetAmount(billID, PPAMT_PAYMENT, bill_rec.CurID, &paym_amt);
		paym_amt = R2(bill_rec.Amount - paym_amt);
		if(paym_amt > 0.0) {
			PPID   debtor_id = bill_rec.Object;
			PPReckonOpEx reckon_data;
			ReckonParam param;
			param = rParam;
			int    dont_cfm = BIN(param.Flags & ReckonParam::fDontConfirm);
			THROW(P_OpObj->GetReckonExData(bill_rec.OpID, &reckon_data));
			if(reckon_data.Flags & ROXF_REQALTOBJ && !(param.Flags & ReckonParam::fAutomat))
				if(SelectAltObject(&debtor_id) <= 0)
					debtor_id = 0;
			if(debtor_id && (!(param.Flags & ReckonParam::fAutomat) || (reckon_data.Flags & ROXF_AUTOPAYM))) {
				int    r = 1;
				const  PPID loc_id  = (reckon_data.Flags & ROXF_THISLOCONLY) ? bill_rec.LocID : 0;
				const  PPID obj2_id = (reckon_data.Flags & ROXF_THISALTOBJONLY) ? bill_rec.Object2 : -1;
				PPIDArray result_bill_list;
				PPIDArray bill_list;
				DateRange period;
				reckon_data.GetDebtPeriod(bill_rec.Dt, &period);
				// @v9.0.0 {
				if(!period.low) {
					DateRange cdp;
					GetDefaultClientDebtPeriod(cdp);
					if(checkdate(cdp.low, 0))
						period.low = cdp.low;
				}
				// } @v9.0.0
				if(reckon_data.Flags & ROXF_BYEXTOBJ && bill_rec.Object2 && bill_rec.Object2 != debtor_id)
					debtor_id = bill_rec.Object2;
				THROW(GetPayableOpListByReckonOp(&reckon_data, debtor_id, &op_list));
				do {
					uint   i;
					double total_debt = 0.0;
					bill_list.clear();
					r = 1;
					for(i = 0; op_list.enumItems(&i, (void**)&p_item);) {
						THROW_MEM(SETIFZ(p_item->P_BillIDList, new PPIDArray));
						p_item->P_BillIDList->clear();
						THROW(GatherPayableBills(p_item, bill_rec.CurID, loc_id, obj2_id, &period, &total_debt));
						THROW(bill_list.add(p_item->P_BillIDList));
					}
					if(total_debt > 0.0) {
						CfmReckoningParam crp;
						crp.Init(0, &bill_rec, total_debt, paym_amt, &bill_list);
						crp.ArticleID = debtor_id;
						if(!(reckon_data.Flags & ROXF_CFM_PAYM))
							dont_cfm = 1;
						if(param.ForceBillID || (param.ForceBillDate && !isempty(param.ForceBillCode))) {
							for(i = 0; i < bill_list.getCount(); i++) {
								if(param.ForceBillID && bill_list.get(i) == param.ForceBillID) {
									crp.ForceBillID = param.ForceBillID;
									break;
								}
								else if(param.ForceBillDate && !isempty(param.ForceBillCode)) {
									BillTbl::Rec sel_bill_rec;
									if(Search(bill_list.get(i), &sel_bill_rec) > 0) {
										if(sel_bill_rec.Dt == param.ForceBillDate && stricmp(sel_bill_rec.Code, param.ForceBillCode) == 0) {
											crp.ForceBillID = sel_bill_rec.ID;
											break;
										}
									}
								}
							}
							if(crp.ForceBillID == 0)
								ok = r = -2;
						}
						if(r > 0) {
							THROW(r = Helper_Reckon(billID, &op_list, &crp, dont_cfm, use_ta));
							if(r == 2) {
								P_Tbl->GetAmount(billID, PPAMT_PAYMENT, bill_rec.CurID, &paym_amt);
								paym_amt = R2(bill_rec.Amount - paym_amt);
							}
							result_bill_list.add(&crp.ResultBillList);
						}
					}
				} while(r == 2 && paym_amt > 0);
				if(rParam.Flags & rParam.fPopupInfo)
					Helper_PopupReckonInfo(result_bill_list);
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBill::ReckoningDebt(PPID billID, const ReckonParam & rParam, int use_ta)
{
	int    ok = 1;
	uint   i;
	BillTbl::Rec bill_rec;
	PPIDArray paym_op_list;
	ReckonOpArList op_list;
	ReckonOpArItem * p_item = 0;
	THROW(Search(billID, &bill_rec) > 0);
	if(bill_rec.Object && CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT)) {
		double debt_amt = 0.0;
		P_Tbl->GetAmount(billID, PPAMT_PAYMENT, bill_rec.CurID, &debt_amt);
		debt_amt = R2(bill_rec.Amount - debt_amt);
		if(debt_amt > 0.0) {
			int    r = 1;
			int    dont_cfm = 1;
			int    do_popup = 0;
			PPIDArray bill_list;
			PPIDArray result_bill_list;
			PPReckonOpEx reckon_data;
			ReckonParam param;
			param = rParam;
			THROW(GetPaymentOpListByDebtOp(bill_rec.OpID, bill_rec.Object, &op_list));
			do {
				double total_amount = 0.0;
				bill_list.clear();
				r = 1;
				for(i = 0; op_list.enumItems(&i, (void**)&p_item);) {
					THROW(P_OpObj->GetReckonExData(p_item->PayableOpID, &reckon_data));
					if(!(param.Flags & ReckonParam::fDontConfirm) && dont_cfm && reckon_data.Flags & ROXF_CFM_DEBT)
						dont_cfm = 0;
					if(!(param.Flags & ReckonParam::fAutomat) || (reckon_data.Flags & ROXF_AUTODEBT)) {
						const  PPID loc_id  = (reckon_data.Flags & ROXF_THISLOCONLY) ? bill_rec.LocID : 0L;
						const  PPID obj2_id = (reckon_data.Flags & ROXF_THISALTOBJONLY) ? bill_rec.Object2 : -1;
						DateRange period;
						if(reckon_data.GetReckonPeriod(bill_rec.Dt, &period) > 0) {
							// @v9.0.0 {
							if(!period.low) {
								DateRange cdp;
								GetDefaultClientDebtPeriod(cdp);
								if(checkdate(cdp.low, 0))
									period.low = cdp.low;
							}
							// } @v9.0.0
							THROW_MEM(SETIFZ(p_item->P_BillIDList, new PPIDArray));
							p_item->P_BillIDList->clear();
		   		    		THROW(GatherPayableBills(p_item, bill_rec.CurID, loc_id, obj2_id, &period, &total_amount));
							THROW(bill_list.add(p_item->P_BillIDList));
						}
					}
				}
				if(total_amount > 0.0) {
					CfmReckoningParam crp;
					crp.Init(1, &bill_rec, total_amount, debt_amt, &bill_list);
					THROW(r = Helper_Reckon(billID, &op_list, &crp, dont_cfm, use_ta));
					if(r == 2) {
						P_Tbl->GetAmount(billID, PPAMT_PAYMENT, bill_rec.CurID, &debt_amt);
						debt_amt = R2(bill_rec.Amount - debt_amt);
	   	        	}
					result_bill_list.add(&crp.ResultBillList);
					do_popup = 1;
				}
			} while(r == 2 && debt_amt > 0.0);
			if(rParam.Flags & rParam.fPopupInfo && do_popup)
				Helper_PopupReckonInfo(result_bill_list);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPObjBill::GetPayableBillList_(const PPIDArray * pOpList, PPID arID, PPID curID, PayableBillList * pList)
{
	int    ok = 1;
	const  LDATE current_date = getcurdate_();
	const  int by_links = 0;
	uint   i;
	double paym = 0.0;
	// @v10.3.0 (never used) double amt = 0.0;
	PPID   single_op = pOpList ? pOpList->getSingle() : 0L;
	PayableBillListItem * p_item;
	PayableBillList tmp_list;
	BExtQuery q(P_Tbl, 3, 64);
	DBQ * dbq = 0;
	q.select(P_Tbl->ID, P_Tbl->Dt, P_Tbl->OpID, P_Tbl->Flags, P_Tbl->CurID, P_Tbl->Amount, P_Tbl->Object2, 0L);
	dbq = & (P_Tbl->Object == arID /*&& daterange(tbl->Dt, &Filt.Period)*/);
	dbq = ppcheckfiltid(dbq, P_Tbl->OpID, single_op);
	if(curID >= 0)
		dbq = & (*dbq && P_Tbl->CurID == curID);
	q.where(*dbq);
	BillTbl::Key3 k;
	MEMSZERO(k);
	k.Object = arID;
	{
		//
		// Для ускорения проверки отсутствия идентификатора найденного документа в списке pList
		// переносим идентификаторы из pList во временный список ndup_list и сортируем его.
		//
		LongArray ndup_list;
		pList->GetIdList(ndup_list);
		ndup_list.sort();
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(!ndup_list.bsearch(P_Tbl->data.ID) && (single_op || !pOpList || pOpList->lsearch(P_Tbl->data.OpID)))
				THROW(tmp_list.AddBill(&P_Tbl->data));
		}
	}
	for(i = 0; i < tmp_list.getCount(); i++) {
		DateRange * p_paym_period = 0;
		p_item = &tmp_list.at(i);
		P_Tbl->CalcPayment(p_item->ID, by_links, p_paym_period, p_item->CurID, &paym);
		if((p_item->Amount - paym) > 0.0)
			THROW(pList->AddPayableItem(p_item, 0, paym, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::GetReceivableBillList(PPID arID, PPID curID, PayableBillList * pList)
{
	int    ok = 1, m;
	ReckonOpArList op_list;
	PPIDArray payable_op_list;
	PPIDArray tmp_op_list;
	ArticleTbl::Rec ar_rec;
	THROW(ArObj.Fetch(arID, &ar_rec) > 0);
	THROW(P_OpObj->GetPayableOpList(ar_rec.AccSheetID, &payable_op_list));
	for(uint j = 0; j < payable_op_list.getCount(); j++) {
		op_list.clear();
		THROW(GetPaymentOpListByDebtOp(payable_op_list.get(j), arID, &op_list));
		for(uint i = 0; i < op_list.getCount(); i++) {
			ReckonOpArItem item = op_list.at(i);
			tmp_op_list.clear();
			tmp_op_list.add(item.PayableOpID);
			for(m = op_list.getCount()-1; m > (int)i; m--) {
				ReckonOpArItem * p_item = &op_list.at(m);
				if(p_item->PayableArID == item.PayableArID) {
					tmp_op_list.addUnique(p_item->PayableOpID);
					op_list.atFree(m);
				}
			}
			THROW(GetPayableBillList_(&tmp_op_list, item.PayableArID, curID, pList));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPObjBill::DebtBlock::DebtBlock() : Amount(0.0), Debt(0.0), HasMatured(0), MaxDelay(0), MaxExpiry(0)
{
}

PPObjBill::DebtBlock & PPObjBill::DebtBlock::Z()
{
	Amount = 0.0;
	Debt = 0.0;
	HasMatured = 0;
	MaxDelay = 0;
	MaxExpiry = 0;
	DebtDimList.clear();
	return *this;
}

int SLAPI PPObjBill::DebtBlock::AddDimItem(PPID dimID, double debt, int expiry)
{
	int    ok = 1;
	uint   pos = 0;
	if(DebtDimList.lsearch(&dimID, &pos, CMPF_LONG)) {
		DimItem & r_item = DebtDimList.at(pos);
		r_item.Debt += debt;
		SETMAX(r_item.MaxExpiry, expiry);
	}
	else {
		DimItem item;
		MEMSZERO(item);
		item.DimID = dimID;
		item.Debt  = debt;
		item.MaxExpiry = expiry;
		DebtDimList.insert(&item);
	}
	return ok;
}

int SLAPI PPObjBill::DebtBlock::GetDimList(RAssocArray & rList) const
{
	rList.clear();
	for(uint i = 0; i < DebtDimList.getCount(); i++) {
		const DimItem & r_item = DebtDimList.at(i);
		rList.Add(r_item.DimID, r_item.Debt);
	}
	return 1;
}

DateRange * FASTCALL PPObjBill::GetDefaultClientDebtPeriod(DateRange & rPeriod) const
{
	DateRange * p_result = 0;
	rPeriod.Z();
	const PPBillConfig & r_cfg = GetConfig();
	const LDATE low_debt_calc_date = r_cfg.LowDebtCalcDate.getactual(ZERODATE);
	if(checkdate(low_debt_calc_date, 0)) {
		rPeriod.low = low_debt_calc_date;
		rPeriod.upp = ZERODATE;
		p_result = &rPeriod;
	}
	return p_result;
}
//
//
//
int SLAPI PPObjBill::CalcClientDebt(PPID clientID, const DateRange * pPeriod, int diffByDebtDim, DebtBlock & rBlk)
{
	int    ok = 1;
	int    has_matured_debt = 0;
	int    max_delay = 0;
	int    max_expiry = 0;
	const  LDATE curdate = LConfig.OperDate;
	double a = 0.0, p = 0.0, t;
	ArticleTbl::Rec ar_rec;
	PPIDArray op_list;
	DateRange period;
	{
		PPUserFuncProfiler ufp(PPUPRF_CALCARTDEBT);
		double ufp_factor = 0.0;
		period.Set(pPeriod);
		rBlk.Z();
		if(ArObj.Fetch(clientID, &ar_rec) > 0) {
			P_OpObj->GetPayableOpList(ar_rec.AccSheetID, &op_list);
			if(op_list.getCount()) {
				const int use_omt_paymamt = BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
				PROFILE_START
				BillCore * p_t = P_Tbl;
				PPObjDebtDim dd_obj;
				PPBillExt ext;
				LAssocArray debt_dim_agent_list;
				PPIDArray dim_list;
				if(diffByDebtDim) {
					dd_obj.FetchAgentList(&debt_dim_agent_list);
				}
				op_list.sort();
				BillTbl::Key3 k;
				k.Object = clientID;
				k.Dt     = period.low;
				k.BillNo = 0;
				BExtQuery q(p_t, 3, 256);
				if(use_omt_paymamt) {
					q.select(p_t->ID, p_t->Dt, p_t->Flags, p_t->Amount, p_t->OpID, p_t->PaymAmount, 0L).
						where(p_t->Object == clientID && daterange(p_t->Dt,  &period) /* (такое ограничение не работает) && tbl->PaymAmount < tbl->Amount*/);
					for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
						if(op_list.bsearch(p_t->data.OpID)) {
							BillTbl::Rec rec;
							p_t->copyBufTo(&rec);
							a += BR2(rec.Amount);
							t = rec.PaymAmount;
							p += t;
							double debt = BR2(rec.Amount) - t;
							if(debt > 1.0) {
								LDATE  mature_date = ZERODATE;
								int    _delay = diffdate(curdate, rec.Dt);
								if(p_t->GetLastPayDate(rec.ID, &mature_date) <= 0)
									mature_date = rec.Dt;
								int    _exp = diffdate(curdate, mature_date);
								SETMAX(max_delay, _delay);
								SETMAX(max_expiry, _exp);
								if(mature_date < curdate)
									has_matured_debt = 1;
								if(diffByDebtDim && rec.Flags & BILLF_EXTRA && FetchExt(rec.ID, &ext) > 0 && ext.AgentID) {
									dim_list.clear();
									debt_dim_agent_list.GetListByVal(ext.AgentID, dim_list);
									for(uint i = 0; i < dim_list.getCount(); i++)
										THROW(rBlk.AddDimItem(dim_list.get(i), debt, _exp));
								}
								ufp_factor += 1.1;
							}
							else
								ufp_factor += 1.0;
						}
					}
				}
				else {
					const int use_conveyor = 1;
					q.select(p_t->ID, p_t->Dt, p_t->Flags, p_t->Amount, p_t->OpID, 0L).
						where(p_t->Object == clientID && daterange(p_t->Dt,  &period));
					if(use_conveyor) {
						struct _BI { // @flat
							PPID   ID;
							LDATE  Dt;
							PPID   OpID;
							long   Flags;
							double Amount;
						};
						SVector bi_list(sizeof(_BI)); // @v9.8.12 SArray-->SVector
						for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
							if(op_list.bsearch(p_t->data.OpID)) {
								_BI bi_item;
								MEMSZERO(bi_item);
								bi_item.ID = p_t->data.ID;
								bi_item.Dt = p_t->data.Dt;
								bi_item.OpID = p_t->data.OpID;
								bi_item.Flags = p_t->data.Flags;
								bi_item.Amount = p_t->data.Amount;
								THROW_SL(bi_list.insert(&bi_item));
							}
						}
						for(uint j = 0; j < bi_list.getCount(); j++) {
							const _BI & r_bi_item = *(const _BI *)bi_list.at(j);
							a += BR2(r_bi_item.Amount);
							THROW(p_t->GetAmount(r_bi_item.ID, PPAMT_PAYMENT, 0L /* @curID */, &t));
							p += t;
							double debt = BR2(r_bi_item.Amount) - t;
							if(debt > 1.0) {
								LDATE  mature_date = ZERODATE;
								int    _delay = diffdate(curdate, r_bi_item.Dt);
								if(p_t->GetLastPayDate(r_bi_item.ID, &mature_date) <= 0)
									mature_date = r_bi_item.Dt;
								int    _exp = diffdate(curdate, mature_date);
								SETMAX(max_delay, _delay);
								SETMAX(max_expiry, _exp);
								if(mature_date < curdate)
									has_matured_debt = 1;
								if(diffByDebtDim && r_bi_item.Flags & BILLF_EXTRA && FetchExt(r_bi_item.ID, &ext) > 0 && ext.AgentID) {
									dim_list.clear();
									debt_dim_agent_list.GetListByVal(ext.AgentID, dim_list);
									for(uint i = 0; i < dim_list.getCount(); i++)
										THROW(rBlk.AddDimItem(dim_list.get(i), debt, _exp));
								}
								ufp_factor += 1.1;
							}
							else {
								ufp_factor += 1.0;
							}
						}
					}
					else {
						for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
							if(op_list.bsearch(p_t->data.OpID)) {
								BillTbl::Rec rec;
								p_t->copyBufTo(&rec);
								a += BR2(rec.Amount);
								THROW(p_t->GetAmount(rec.ID, PPAMT_PAYMENT, 0L /* @curID */, &t));
								p += t;
								double debt = BR2(rec.Amount) - t;
								if(debt > 1.0) {
									LDATE  mature_date = ZERODATE;
									int    _delay = diffdate(curdate, rec.Dt);
									if(p_t->GetLastPayDate(rec.ID, &mature_date) <= 0)
										mature_date = rec.Dt;
									int    _exp = diffdate(curdate, mature_date);
									SETMAX(max_delay, _delay);
									SETMAX(max_expiry, _exp);
									if(mature_date < curdate)
										has_matured_debt = 1;
									if(diffByDebtDim && rec.Flags & BILLF_EXTRA && FetchExt(rec.ID, &ext) > 0 && ext.AgentID) {
										dim_list.clear();
										debt_dim_agent_list.GetListByVal(ext.AgentID, dim_list);
										for(uint i = 0; i < dim_list.getCount(); i++)
											THROW(rBlk.AddDimItem(dim_list.get(i), debt, _exp));
									}
									ufp_factor += 1.1;
								}
								else
									ufp_factor += 1.0;
							}
						}
					}
				}
				PROFILE_END
			}
		}
		rBlk.Amount = R2(a);
		rBlk.Debt = R2(a - p);
		rBlk.HasMatured = has_matured_debt;
		rBlk.MaxDelay = max_delay;
		rBlk.MaxExpiry = max_expiry;
		ufp.SetFactor(1, ufp_factor);
		ufp.Commit();
	}
	CATCHZOK
	return ok;
}
//
//
//
struct CBO_BillEntry { // @flat
	PPID   ID;
	LDATE  Dt;
	PPID   ArID;
	PPID   Ar2ID;
	double Amount;
	char   Code[24];
};

IMPL_CMPFUNC(CBO_BillEntry, i1, i2) { RET_CMPCASCADE4((const CBO_BillEntry *)i1, (const CBO_BillEntry *)i2, ArID, Ar2ID, Dt, Amount); }

int SLAPI PPObjBill::CreateBankingOrders(const PPIDArray & rBillList, long flags, PPGPaymentOrderList & rOrderList)
{
	int    ok = -1;
	SVector list(sizeof(CBO_BillEntry)); // @v9.8.4 SArray-->SVector
	for(uint i = 0; i < rBillList.getCount(); i++) {
		BillTbl::Rec bill_rec;
		if(Search(rBillList.get(i), &bill_rec) > 0 && bill_rec.Object && CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT)) {
			double paym = 0.0;
			P_Tbl->CalcPayment(bill_rec.ID, 1, 0, bill_rec.CurID, &paym);
			if(paym < bill_rec.Amount) {
				CBO_BillEntry entry;
				MEMSZERO(entry);
				entry.ID = bill_rec.ID;
				entry.Dt = bill_rec.Dt;
				entry.ArID = bill_rec.Object;
				entry.Ar2ID = bill_rec.Object2;
				entry.Amount = fabs(bill_rec.Amount - paym);
				STRNSCPY(entry.Code, bill_rec.Code);
				THROW_SL(list.insert(&entry));
			}
		}
	}
	if(list.getCount()) {
		list.sort(PTR_CMPFUNC(CBO_BillEntry));
		{
			//
			// В конец списка добавляем пустой элемент дабы не
			// повторять функцию формирования баковского ордера после завершения цикла.
			//
			CBO_BillEntry entry;
			MEMSZERO(entry);
			entry.ArID = MAXLONG;
			list.insert(&entry);
		}
		PPID   last_ar_id = 0;
		double amount = 0.0;
		PPObjPerson psn_obj;
		PPIDArray link_bill_list;
		CBO_BillEntry * p_prev_entry = 0;
		for(uint i = 0; i < list.getCount(); i++) {
			CBO_BillEntry * p_entry = (CBO_BillEntry *)list.at(i);
			if(last_ar_id && last_ar_id != p_entry->ArID) {
				if(amount > 0.0) {
					PPID   payer_id = 0;
					PPID   payer_bnk_acc_id = 0;
					PPID   rcvr_id = 0;
					PPID   rcvr_bnk_acc_id = 0;
					if(flags & cboIn) {
						payer_id = ObjectToPerson(p_prev_entry->ArID, 0);
						GetMainOrgID(&rcvr_id);
					}
					else {
						GetMainOrgID(&payer_id);
						rcvr_id = ObjectToPerson(p_prev_entry->ArID, 0);
					}
					if(payer_id && rcvr_id) {
						int r2 = 0;
						if(psn_obj.GetSingleBnkAcct(payer_id, 0, &payer_bnk_acc_id, 0) > 0)
							r2++;
						if(psn_obj.GetSingleBnkAcct(rcvr_id, 0, &rcvr_bnk_acc_id, 0) > 0)
							r2++;
						if(r2 == 2 || !(flags & cboSkipUndefBnkAcc)) {
							PPGPaymentOrder * p_new_order = rOrderList.CreateNewItem();
							THROW_SL(p_new_order);
							p_new_order->Options = 0;
							p_new_order->Dt = getcurdate_();
							p_new_order->ArID  = p_prev_entry->ArID;
							p_new_order->Ar2ID = p_prev_entry->Ar2ID;
							p_new_order->PayerID = payer_id;
							p_new_order->PayerBnkAccID = payer_bnk_acc_id;
							p_new_order->RcvrID = rcvr_id;
							p_new_order->RcvrBnkAccID = rcvr_bnk_acc_id;
							p_new_order->Amount = amount;
							p_new_order->BnkPaymMethod = BNKPAYMMETHOD_DEFAULT;
							p_new_order->BnkQueueing = BNKQUEUEING_DEFAULT;
							p_new_order->PayerStatus = 1;
							p_new_order->LinkBillList = link_bill_list;
							ok = 1;
						}
					}
				}
				amount = 0.0;
				link_bill_list.clear();
			}
			if(p_entry->Amount > 0.0) {
				link_bill_list.add(p_entry->ID);
				amount += p_entry->Amount;
			}
			last_ar_id = p_entry->ArID;
			p_prev_entry = p_entry;
		}
	}
	CATCHZOK
	return ok;
}
//
// @obsolete Используется только для обслуживания экспортной структуры данных PaymBillList
//
class PPViewPaymBill {
public:
	SLAPI  PPViewPaymBill();
	SLAPI ~PPViewPaymBill();
	void   SLAPI Init(const PaymBillFilt *);
	int    SLAPI InitIteration();
	int    FASTCALL NextIteration(PaymBillViewItem *);
	const  PaymBillFilt * SLAPI GetFilt() const { return &Filt; }
	const  IterCounter & SLAPI GetIterCounter() const { return Counter; }
private:
	PPObjBill * P_BObj;
	PaymBillFilt Filt;
	IterCounter Counter;
	SArray * P_PaymBillList;
};

SLAPI PPViewPaymBill::PPViewPaymBill() : P_BObj(BillObj), P_PaymBillList(0)
{
}

SLAPI PPViewPaymBill::~PPViewPaymBill()
{
	ZDELETE(P_PaymBillList);
}

void SLAPI PPViewPaymBill::Init(const PaymBillFilt * pFilt)
{
	if(!RVALUEPTR(Filt, pFilt))
		MEMSZERO(Filt);
}

int SLAPI PPViewPaymBill::InitIteration()
{
	int    ok = -1;
	if(P_BObj) {
		delete P_PaymBillList;
		P_PaymBillList = P_BObj->MakePaymentList(Filt.LinkBillID, Filt.Kind);
		Counter.Init(P_PaymBillList->getCount());
		ok = 1;
	}
	return ok;
}

int FASTCALL PPViewPaymBill::NextIteration(PaymBillViewItem * pItem)
{
	int    ok = -1;
	if(pItem && P_PaymBillList && P_BObj) {
		if(Counter < Counter.GetTotal()) {
			const _PaymentEntry * p_be = (const PaymBillViewItem *)P_PaymBillList->at(Counter);
			BillTbl::Rec bill_rec;
			if(P_BObj->Search(p_be->ID, &bill_rec) > 0)
				STRNSCPY(pItem->Memo, bill_rec.Memo);
			STRNSCPY(pItem->Code, p_be->Code);
			STRNSCPY(pItem->StatusName, p_be->StatusName);
			STRNSCPY(pItem->OpName, p_be->OpName);
			pItem->Amount  = p_be->Amount;
			pItem->Payment = p_be->Payment;
			pItem->Rest    = p_be->Rest;
			pItem->ID      = p_be->ID;
			pItem->LinkBillID = p_be->LinkBillID;
			pItem->RcknBillID = p_be->RcknBillID;
			pItem->CurID   = p_be->CurID;
			pItem->Date    = p_be->Date;
			Counter.Increment();
			ok = 1;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PaymBillList
//
PPALDD_CONSTRUCTOR(PaymBillList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PaymBillList) { Destroy(); }

int PPALDD_PaymBillList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(LinkedBill, rsrv);
	H.LinkBillID = p_filt->BillID;
	H.Kind       = p_filt->Kind;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PaymBillList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(LinkedBill);
}

int PPALDD_PaymBillList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(LinkedBill);
	SString temp_buf;
	STRNSCPY(I.BillNo, item.Code);
	GetOpName(item.OpID, temp_buf);
	temp_buf.CopyTo(I.OpName, sizeof(I.OpName));
	STRNSCPY(I.Memo,   item.Memo);
	I.ItLinkBillID = item.LinkBillID;
	I.ItRcknBillID = item.RcknBillID;
	I.Amount     = item.Payment;
	I.CurrencyID = item.CurID;
	I.PaymDt     = item.Dt;
	I.Rest       = item.Rest;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PaymBillList::Destroy()
{
	DESTROY_PPVIEW_ALDD(PaymBill);
}
//
//
//
#if 0 // @v9.7.10 @obsolete {

class PaymentBrowser : public BrowserWindow { // @obsolete
public:
	PaymentBrowser(uint rezID, PPID _id, int _kind, PPObjBill * _ppobj) :
		BrowserWindow(rezID, _ppobj->MakePaymentList(_id, _kind))
	{
		BillTbl::Rec rec;
		PrevKind = -1;
		Kind   = _kind;
		P_BObj  = _ppobj;
		LinkBillID  = _id;
		LinkOpID = (P_BObj->Search(LinkBillID, &rec) > 0) ? rec.OpID : 0;
	}
private:
	DECL_HANDLE_EVENT;
	void   print();
	void   updateList(long);
	PPID   getCurrID();

	int    Kind;
	int    PrevKind;  // Значение Kind до переключения вида
	PPID   LinkBillID;
	PPID   LinkOpID;
	PPObjBill * P_BObj;
};

IMPL_HANDLE_EVENT(PaymentBrowser)
{
	BrowserWindow::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert:
				if(oneof2(Kind, 0, 1)) {
					PPID   op_type_id = 0;
					if(Kind == 1)
	   					op_type_id = PPOPT_CHARGE;
					else if(Kind == 0)
						op_type_id = PPOPT_PAYMENT;
					/*
					else if(Kind == 5) {
						BillTbl::Rec bill_rec;
						if(P_BObj->Fetch(LinkBillID, &bill_rec) > 0) {
							PPObjOprKind op_obj;
							PPDraftOpEx doe;
							if(op_obj.GetDraftExData(bill_rec.OpID, &doe) > 0) {
								if(bill_rec.Flags & BILLF_WRITEDOFF) {

								}
							}
						}
					}
					*/
					if(op_type_id) {
						PPObjBill::AddBlock ab;
						ab.LinkBillID = LinkBillID;
						ab.OpID = SelectOprKind(0, LinkOpID, op_type_id, 0L);
						if(ab.OpID == 0)
							PPError();
						else if(ab.OpID > 0 && P_BObj->AddGoodsBill(0, &ab) == cmOK)
							updateList(MAXLONG);
					}
				}
				break;
			case cmaDelete:
				{
					int    done = 0;
					PPID   c = getCurrID();
					BillTbl::Rec bill_rec;
					if(c && P_BObj->Search(c, &bill_rec) > 0) {
						if(oneof2(Kind, 0, 2)) {
							if(!IsOpPaymOrRetn(bill_rec.OpID)) {
								uint   v = 0;
								if(SelectorDialog(DLG_SELRMVPAYM, CTL_SELRMVPAYM_WHAT, &v) > 0) {
									if(v == 1) {
										_PaymentEntry * d = (_PaymentEntry *)view->getCurItem();
										if(d && P_BObj->P_Tbl->BreakOffLink(c, LinkBillID, d->LinkKind, 1) > 0)
											updateList(-1);
									}
									else if(v == 0) {
										if(P_BObj->RemoveObjV(c, 0, PPObject::rmv_default, 0) > 0)
											updateList(-1);
									}
								}
								done = 1;
							}
						}
						if(!done && P_BObj->RemoveObjV(c, 0, PPObject::rmv_default, 0) > 0)
							updateList(-1);
					}
				}
				break;
			case cmaEdit:
				{
					PPID   c = getCurrID();
					if(c && P_BObj->EditGoodsBill(c, 0) == cmOK)
						updateList(-1);
				}
				break;
			case cmPrint:
				print();
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbAltF2:
				{
					PPObjBill::AddBlock ab;
					ab.SampleBillID = getCurrID();
					if(oneof2(Kind, 0, 1) && ab.SampleBillID) {
						PPID   bill_id = 0;
						if(P_BObj->AddGoodsBill(&bill_id, &ab) == cmOK)
							updateList(MAXLONG);
					}
				}
				break;
			case kbCtrlF6:
				if(P_BObj->EditBillStatus(getCurrID()) > 0)
					updateList(-1);
				break;
			case kbF5:
				if(Kind == 1 && P_BObj->ViewPayments(getCurrID(), LinkedBillFilt::lkPayments) > 0)
					updateList(-1);
				break;
			case kbF6:
				if(Kind == 1 && LinkBillID && P_BObj->AutoCharge(LinkBillID) > 0)
					updateList(MAXLONG);
				break;
			case kbF7:
				print();
				break;
			case kbAltF7:
				{
					uint   rpt_id = REPORT_PAYMBILLLIST;
					PaymBillFilt flt;
					PPViewPaymBill v;
					flt.LinkBillID = LinkBillID;
					flt.LinkOpID   = LinkOpID;
					flt.Kind       = Kind;
					v.Init(&flt);
					PView  pf(&v);
					PPReportEnv env;
					env.PrnFlags = SReport::DisableGrouping;
					PPAlddPrint(rpt_id, &pf, &env);
				}
				break;
			case kbF8:
				{
					PPID   bill_id = getCurrID();
					if(bill_id) {
						P_BObj->SetWLabel(bill_id, -1);
						updateList(-1);
					}
				}
				break;
			case kbF9:
				{
					TDialog * dlg = 0;
					if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_PAYMTOTAL)))) {
						AryBrowserDef * p_def = (AryBrowserDef *)view->getDef();
						long   count = 0;
						double amount = 0.0;
						for(long i = 0; i < p_def->getRecsCount(); i++) {
							_PaymentEntry * p_entry = (_PaymentEntry *)p_def->getRow(i);
							count++;
							amount += p_entry->Payment;
						}
						dlg->setCtrlLong(CTL_PAYMTOTAL_COUNT, count);
						dlg->setCtrlReal(CTL_PAYMTOTAL_AMOUNT, amount);
						ExecViewAndDestroy(dlg);
					}
				}
				break;
			default:
				if(TVCHR == kbCtrlA)
					P_BObj->ViewAccturns(getCurrID());
				else if(TVCHR == kbCtrlT) {
					int    upd = 1;
					if(PrevKind >= 0) {
						Kind = PrevKind;
						PrevKind = -1;
					}
					else if(Kind == 0) {
						PrevKind = Kind;
						Kind = 3;
					}
					else if(Kind == 2) {
						PrevKind = Kind;
						Kind = 3;
					}
					else if(Kind == 3) {
						PrevKind = Kind;
						Kind = 2;
					}
					else
						upd = 0;
					if(upd)
						updateList(-1);
				}
				else if(TVCHR == kbCtrlZ)
					P_BObj->PosPrintByBill(getCurrID());
				else
					return;
		}
	}
	else
		return;
	clearEvent(event);
}

void PaymentBrowser::updateList(long pos)
{
	AryBrowserDef * p_def = (AryBrowserDef *)view->getDef();
	if(p_def) {
		long   c = p_def->_curItem();
		p_def->setArray(0, 0, 1);
		SArray * p_list = P_BObj->MakePaymentList(LinkBillID, Kind);
		if(p_list) {
			lock();
			p_def->setArray(p_list, 0, 1);
			view->setRange(p_list->getCount());
			if(pos < 0)
				view->go(c);
			else if(pos == MAXLONG)
				view->go(p_list->getCount() - 1);
			unlock();
		}
		else
			PPError();
	}
}

PPID PaymentBrowser::getCurrID()
{
	void * d = view->getCurItem();
	return d ? ((_PaymentEntry*)d)->ID : 0;
}

void PaymentBrowser::print()
{
	PPBillPacket pack;
	PPID   cur_id = getCurrID();
	if(cur_id)
		if(P_BObj->ExtractPacket(cur_id, &pack))
			if(pack.Rec.Flags & BILLF_BANKING || CheckOpPrnFlags(pack.Rec.OpID, OPKF_PRT_INVOICE))
				PrintGoodsBill(&pack);
			else
				PrintCashOrderByGoodsBill(&pack);
		else
			PPError();
}
#endif // } 0 @v9.7.10 @obsolete
