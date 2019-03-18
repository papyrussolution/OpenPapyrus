// AT2EAT.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2006, 2007, 2016, 2017, 2019
//
#include <pp.h>
#pragma hdrstop

int SLAPI PPObjBill::ConvertGenAccturnToExtAccBill(PPID srcID, PPID * pDestID, const CvtAt2Ab_Param * pParam, int use_ta)
{
	int    ok = 1;
	double amt;
	PPOprKind op_rec;
	PPBillPacket src_pack;
	PPBillPacket dest_pack;
	PPAccTurn * p_at = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ExtractPacket(srcID, &src_pack) > 0);
		THROW_PP(src_pack.Turns.getCount(), PPERR_UNABLECVTBILL2BILL);
		THROW(GetOpData(src_pack.Rec.OpID, &op_rec) > 0);
		THROW_PP(op_rec.OpTypeID == PPOPT_ACCTURN /*&&!(op_rec.Flags & OPKF_EXTACCTURN)*/, PPERR_UNABLECVTBILL2BILL);
		THROW(GetOpData(pParam->OpID, &op_rec) > 0);
		p_at = & src_pack.Turns.at(0);
		THROW(dest_pack.CreateBlank(pParam->OpID, 0, pParam->LocID ? pParam->LocID : src_pack.Rec.LocID, use_ta));
		dest_pack.Rec.Object  = pParam->ObjID;
		dest_pack.Rec.Object2 = pParam->ExtObjID;
		if(p_at->DbtSheet) {
			if(p_at->DbtSheet == op_rec.AccSheetID) {
				SETIFZ(dest_pack.Rec.Object, p_at->DbtID.ar);
			}
			else if(p_at->DbtSheet == op_rec.AccSheet2ID) {
				SETIFZ(dest_pack.Rec.Object2, p_at->DbtID.ar);
			}
		}
		if(p_at->CrdSheet) {
			if(p_at->CrdSheet == op_rec.AccSheetID) {
				SETIFZ(dest_pack.Rec.Object, p_at->CrdID.ar);
			}
			else if(p_at->CrdSheet == op_rec.AccSheet2ID) {
				SETIFZ(dest_pack.Rec.Object2, p_at->CrdID.ar);
			}
		}
		dest_pack.Rec.Dt = src_pack.Rec.Dt;
		STRNSCPY(dest_pack.Rec.Code, src_pack.Rec.Code);
		dest_pack.Rec.LocID = pParam->LocID ? pParam->LocID : src_pack.Rec.LocID;
		STRNSCPY(dest_pack.Rec.Memo, src_pack.Rec.Memo);
		amt = (pParam->Flags & CvtAt2Ab_Param::fNegAmount) ? -p_at->Amount : p_at->Amount;
		dest_pack.Rec.Amount = BR2(amt);
		dest_pack.Amounts.Put(PPAMT_MAIN, 0L, amt, 0, 1);
		//dest_pack.InitAmounts();
		THROW(FillTurnList(&dest_pack));
		THROW(TurnPacket(&dest_pack, 0));
		ASSIGN_PTR(pDestID, dest_pack.Rec.ID);
		THROW(RemovePacket(srcID, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

class CvtAt2Ab_Dialog : public TDialog {
public:
	CvtAt2Ab_Dialog() : TDialog(DLG_CVTAT2AB), AccSheetID(0), AccSheet2ID(0)
	{
		MEMSZERO(Data);
	}
	int    setDTS(const CvtAt2Ab_Param *);
	int    getDTS(CvtAt2Ab_Param *);
private:
	DECL_HANDLE_EVENT;
	PPID   AccSheetID;
	PPID   AccSheet2ID;
	CvtAt2Ab_Param Data;
};

int CvtAt2Ab_Dialog::setDTS(const CvtAt2Ab_Param * pData)
{
	Data = *pData;

	ushort v;
	PPOprKind op_rec;
	SetupOprKindCombo(this, CTLSEL_CVTAT2AB_OP, 0, 0, Data.P_OpList, OPKLF_OPLIST);
	if(GetOpData(Data.OpID, &op_rec) > 0) {
		AccSheetID  = (op_rec.AccSheetID != 0)  ? op_rec.AccSheetID  : (Data.ObjID = 0);
		AccSheet2ID = (op_rec.AccSheet2ID != 0) ? op_rec.AccSheet2ID : (Data.ExtObjID = 0);
	}
	else
		AccSheet2ID = AccSheetID = Data.ExtObjID = Data.ObjID = 0;
	SetupArCombo(this, CTLSEL_CVTAT2AB_OBJ,    Data.ObjID,    OLW_CANINSERT, AccSheetID, 0);
	SetupArCombo(this, CTLSEL_CVTAT2AB_EXTOBJ, Data.ExtObjID, OLW_CANINSERT, AccSheet2ID, 0);
	SetupPPObjCombo(this, CTLSEL_CVTAT2AB_LOC, PPOBJ_LOCATION, Data.LocID, 0);
	v = 0;
	SETFLAG(v, 0x01, Data.Flags & CvtAt2Ab_Param::fNegAmount);
	setCtrlData(CTL_CVTAT2AB_FLAGS, &v);
	return 1;
}

int CvtAt2Ab_Dialog::getDTS(CvtAt2Ab_Param * pData)
{
	ushort v = 0;
	getCtrlData(CTLSEL_CVTAT2AB_OP,     &Data.OpID);
	getCtrlData(CTLSEL_CVTAT2AB_OBJ,    &Data.ObjID);
	getCtrlData(CTLSEL_CVTAT2AB_EXTOBJ, &Data.ExtObjID);
	getCtrlData(CTLSEL_CVTAT2AB_LOC,    &Data.LocID);
	getCtrlData(CTL_CVTAT2AB_FLAGS,     &(v = 0));
	SETFLAG(Data.Flags, CvtAt2Ab_Param::fNegAmount, v & 0x01);
	if(Data.OpID == 0)
		return (PPError(PPERR_OPRKINDNEEDED, 0), 0);
	else {
		ASSIGN_PTR(pData, Data);
		return 1;
	}
}

IMPL_HANDLE_EVENT(CvtAt2Ab_Dialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_CVTAT2AB_OP)) {
		PPOprKind op_rec;
		PPID   op_id = getCtrlLong(CTLSEL_CVTAT2AB_OP);
		if(GetOpData(op_id, &op_rec) > 0) {
			if(op_rec.AccSheet2ID != 0) {
				if(op_rec.AccSheet2ID != AccSheet2ID)
					AccSheet2ID = op_rec.AccSheet2ID;
			}
			else
				AccSheet2ID = Data.ExtObjID = 0;
			if(op_rec.AccSheetID != 0) {
				if(op_rec.AccSheetID != AccSheetID)
					AccSheetID = op_rec.AccSheetID;
			}
			else
				AccSheetID = Data.ObjID = 0;
		}
		else
			AccSheetID = AccSheet2ID = Data.ObjID = Data.ExtObjID = 0;
		SetupArCombo(this, CTLSEL_CVTAT2AB_OBJ,    0, OLW_CANINSERT, AccSheetID, 0);
		SetupArCombo(this, CTLSEL_CVTAT2AB_EXTOBJ, 0, OLW_CANINSERT, AccSheet2ID, 0);
		clearEvent(event);
	}
}
//
//
//
int SLAPI PPViewAccturn::ConvertGenAccturnToExtAccBill()
{
	int    ok = -1, frrl_tag = 0;
	uint   i;
	PPID   op_id;
	CvtAt2Ab_Dialog * dlg = 0;
	AccturnViewItem item;
	if(oneof2(Filt.Aco, ACO_2, ACO_3) && Filt.DbtAcct.ac && Filt.CrdAcct.ac) {
		PPOprKind op_rec;
		PPIDArray op_list;
		int    skip = 0;
		if(IsGenericOp(Filt.OpID) > 0) {
			GetGenericOpList(Filt.OpID, &op_list);
			for(i = 0; !skip && i < op_list.getCount(); i++)
				if(GetOpType(op_list.at(i), &op_rec) != PPOPT_ACCTURN)
					skip = 1;
		}
		else if(GetOpType(Filt.OpID, &op_rec) != PPOPT_ACCTURN)
			skip = 1;
		if(!skip) {
			op_list.freeAll();
			for(op_id = 0; EnumOperations(PPOPT_ACCTURN, &op_id, &op_rec) > 0;) {
				if(op_rec.Flags & OPKF_EXTACCTURN)
					op_list.add(op_id);
			}
			{
				int    valid_data = 0;
				CvtAt2Ab_Param param;
				MEMSZERO(param);
				param.P_OpList = &op_list;
				THROW(CheckDialogPtr(&(dlg = new CvtAt2Ab_Dialog())));
				dlg->setDTS(&param);
				while(!valid_data && ExecView(dlg) == cmOK) {
					if(dlg->getDTS(&param)) {
						PPIDArray bill_id_list;
						valid_data = 1;
						ZDELETE(dlg);
						PPWait(1);
						{
							PPTransaction tra(1);
							THROW(tra);
							THROW(P_ATC->LockingFRR(1, &frrl_tag, 0));
							for(InitIteration(); NextIteration(&item) > 0;)
								if(P_BObj->Search(item.BillID) > 0)
									bill_id_list.addUnique(item.BillID);
							for(i = 0; i < bill_id_list.getCount(); i++) {
								THROW(P_BObj->ConvertGenAccturnToExtAccBill(bill_id_list.at(i), 0, &param, 0));
								PPWaitPercent(i+1, bill_id_list.getCount());
							}
							THROW(P_ATC->LockingFRR(0, &frrl_tag, 0));
							THROW(tra.Commit());
						}
						PPWait(0);
						ok = 1;
					}
				}
			}
		}
	}
	CATCH
		P_ATC->LockingFRR(-1, &frrl_tag, 0);
		ok = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok;
}
