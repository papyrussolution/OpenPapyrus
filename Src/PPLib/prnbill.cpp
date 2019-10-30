// PRNBILL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

static const char * BillMultiplePrintCfg              = "BillMultiplePrintCfg";
static const char * BillMultiplePrintDivByCopies      = "BillMultiplePrintDivByCopies";
static const char * BillMultiplePrintOnlyPriceChanged = "BillMultiplePrintOnlyPriceChanged";

#define BILL_FORM_COUNT 13

static int SLAPI SelectForm(long f, uint * pAmtTypes, LAssocArray & rSelAry, PPID oprType, int * pDivCopiesFlag, int * pOnlyPriceChangedFlag)
{
	class BillPrintDialog : public TDialog {
	public:
		BillPrintDialog(uint dlgID, PPID oprType) : TDialog(dlgID), OprType(oprType)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmClusterClk)) {
				if(event.isCtlEvent(CTL_PRNGBILL_WHAT)) {
					TCluster * p_clu = static_cast<TCluster *>(getCtrlView(CTL_PRNGBILL_WHAT));
					disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(OprType != PPOPT_GOODSRECEIPT || !p_clu || !p_clu->mark(7)));
					clearEvent(event);
				}
			}
		}

		const PPID   OprType;
	};
	class MultiPrintDialog : public TDialog {
	public:
		MultiPrintDialog(uint dlgID, PPID oprType) : TDialog(dlgID), OprType(oprType)
		{
		}
		long   GetNumCopies(uint ctl)
		{
			SString temp_buf;
			TView::SGetWindowText(GetDlgItem(H(), ctl), temp_buf);
			return temp_buf.ToLong();
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			TCluster * p_clu = static_cast<TCluster *>(getCtrlView(CTL_PRNGBILL_WHAT));
			if(event.isCmd(cmClusterClk)) {
				if(event.isCtlEvent(CTL_PRNGBILL_WHAT)) {
					uint16 w = getCtrlUInt16(CTL_PRNGBILL_WHAT);
					int    i = 0;
					uint   c;
					for(c = 0; c < BILL_FORM_COUNT; c++)
						if(((uint16)w >> c) & 0x0001)
							i++;
					if(p_clu) {
						for(c = 0; c < BILL_FORM_COUNT; c++) {
							const int checked = p_clu->mark(c);
							HWND w_nc = GetDlgItem(H(), CTL_PRNGBILL_NUMCOPIES + c);
							HWND w_spin = GetDlgItem(H(), CTL_PRNGBILL_SPIN + c);
							if(i < 2 || !checked)
								SendDlgItemMessage(H(), CTL_PRNGBILL_SPIN + c, UDM_SETPOS, 0, MAKELONG(1, 0));
							EnableWindow(w_nc, /* @v8.1.3 i > 1 &&*/ checked);
							EnableWindow(w_spin, /* @v8.1.3 i > 1 &&*/ checked);
							if(c == 7) // @v6.4.8 AHTOXA Ценник
								disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(OprType != PPOPT_GOODSRECEIPT || !checked));
						}
					}
					else
						disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1); // @v6.4.8 AHTOXA
					clearEvent(event);
				}
			}
			else if(event.isCmd(cmSave)) {
				if(p_clu) {
					int    div_copies = 0, only_price_chng = 0;
					SString prn_cfg;
					getCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
					getCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
					for(uint c = 0; c < BILL_FORM_COUNT; c++) {
						int checked = p_clu->mark(c);
						if(c != 0)
							prn_cfg.Semicol();
						prn_cfg.Cat(c + 1).Comma().Cat(BIN(checked)).Comma().Cat(GetNumCopies(CTL_PRNGBILL_NUMCOPIES + c));
					}
					WinRegKey key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
					key.PutString(BillMultiplePrintCfg, prn_cfg);
					key.PutDWord(BillMultiplePrintDivByCopies, (uint32)div_copies);
					key.PutDWord(BillMultiplePrintOnlyPriceChanged, (uint32)only_price_chng);
				}
				clearEvent(event);
			}
		}
		const PPID   OprType;
	};
	int    ok = 1;
	int    v = 0;
	int    div_copies = 0;
	int    only_price_chng = 0;
	ushort p;
	uint   res_id;
	SString temp_buf;
	TDialog * dlg;
	if(pAmtTypes == 0) {
		// @v10.3.0 Теперь используется (с приоритетом) интерфейсная настройка для разрешения/запрета множественной печати
		int    allow_mult_print = 0;
		UserInterfaceSettings uis;
		const int uis_r = uis.Restore();
		if((uis.Flags & uis.fEnalbeBillMultiPrint) && !(uis.Flags & uis.fDisableBillMultiPrint))
			allow_mult_print = 1;
		else if(!(uis.Flags & uis.fEnalbeBillMultiPrint) && (uis.Flags & uis.fDisableBillMultiPrint))
			allow_mult_print = 0;
		else {
			PPBillConfig cfg;
			if(PPObjBill::ReadConfig(&cfg) > 0 && cfg.Flags & BCF_ALLOWMULTIPRINT)
				allow_mult_print = 1;
		}
		res_id = allow_mult_print ? DLG_PRNGBILLM : DLG_PRNGBILL;
	}
	else if(f & OPKF_PRT_EXTFORMFLAGS)
		res_id = DLG_PRNGBILL2;
	else
		res_id = DLG_PRNGBILL3;
	if(res_id == DLG_PRNGBILLM)
		dlg = new MultiPrintDialog(res_id, oprType);
	else
		dlg = new BillPrintDialog(res_id, oprType);
	if(!CheckDialogPtr(&dlg))
		return 0;
	TCluster * clu = static_cast<TCluster *>(dlg->getCtrlView(CTL_PRNGBILL_WHAT));
	//
	// Порядок пунктов в диалоге:
	//    0. Накладная //
	//    1. Сертификаты
	//    2. Счет-фактура
	//    3. Кассовый ордер
	//    4. Товарно-транспортная накладна
	//    5. Товарно-транспортная накладная (транспортный раздел)
	//    6. Акт выполненных работ
	//    7. Ценник
	//    8. План платежей
	//    9. Сальдо по отгруженной таре
	//    10. Наряд на складскую сборку // @v7.1.12
	//    11. Изображения из тегов лотов
	//    12. Универсальный передаточный документ // @v8.1.3
	//
	if(f & OPKF_PRT_EXTFORMFLAGS) {
		#define DELETE_CLUSTER_ITEM(c,i) (c)->disableItem(i, 1)
		if(clu) {
			if(!(f & OPKF_PRT_LOTTAGIMG))
				DELETE_CLUSTER_ITEM(clu, 11);
			if(!(f & OPKF_PRT_TARESALDO))
				DELETE_CLUSTER_ITEM(clu, 9);
			if(!(f & OPKF_PRT_PAYPLAN))
				DELETE_CLUSTER_ITEM(clu, 8);
			if(!(f & OPKF_PRT_PLABEL))
				DELETE_CLUSTER_ITEM(clu, 7);
			if(!(f & OPKF_PRT_SRVACT))
				DELETE_CLUSTER_ITEM(clu, 6);
			if(!(f & OPKF_PRT_LADING)) {
				DELETE_CLUSTER_ITEM(clu, 5);
				DELETE_CLUSTER_ITEM(clu, 4);
			}
			if(!(f & OPKF_PRT_CASHORD))
				DELETE_CLUSTER_ITEM(clu, 3);
			if(!(f & OPKF_PRT_INVOICE)) {
				DELETE_CLUSTER_ITEM(clu, 2);
				DELETE_CLUSTER_ITEM(clu, 12);
			}
			if(!(f & OPKF_PRT_QCERT))
				DELETE_CLUSTER_ITEM(clu, 1);
		}
		#undef DELETE_CLUSTER_ITEM
	}
	else {
		v = 1;
		if(!pAmtTypes) {
			rSelAry.Add(static_cast<PPID>(v), 1, 0);
			delete dlg;
			return ok;
		}
	}
	if(pAmtTypes)
		p = (*pAmtTypes - 1);
	if(res_id == DLG_PRNGBILLM) {
		TCluster * p_clu = static_cast<TCluster *>(dlg->getCtrlView(CTL_PRNGBILL_WHAT));
		LAssocArray rpt_info_list;
		v = 1;
		if(p_clu) {
			SString sbuf;
			WinRegKey key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1);
			temp_buf.Z();
			key.GetString(BillMultiplePrintCfg, temp_buf);
			StringSet ss(';', temp_buf);
			for(uint p = 0, i = 0; ss.get(&p, sbuf) > 0; i++) {
				uint   p1 = 0;
				StringSet ss1(',', sbuf);
				ss1.get(&p1, sbuf);   // ID
				ss1.get(&p1, sbuf);   // State
				long   checked = p_clu->isItemEnabled(i) ? sbuf.ToLong() : 0;
				SETFLAG(v, 0x0001 << i, checked);
				ss1.get(&p1, sbuf);   // Num copies
				rpt_info_list.Add(sbuf.ToLong(), checked, 0);
			}
			uint32 val = 0;
			key.GetDWord(BillMultiplePrintDivByCopies, &val);
			div_copies = (val) ? 1 : div_copies;
			key.GetDWord(BillMultiplePrintOnlyPriceChanged, &val);
			only_price_chng = val ? 1 : only_price_chng;
		}
		for(uint i = 0, spin_ctl = CTL_PRNGBILL_SPIN, copy_ctl = CTL_PRNGBILL_NUMCOPIES; spin_ctl < CTL_PRNGBILL_SPIN13 + 1; spin_ctl++, copy_ctl++, i++) {
			long   num_copies = 1;
			long   checked    = 0;
			if(i < rpt_info_list.getCount()) {
				num_copies = rpt_info_list.at(i).Key;
				checked    = rpt_info_list.at(i).Val;
			}
			dlg->SetupSpin(spin_ctl, copy_ctl, 1, 10, num_copies);
			EnableWindow(GetDlgItem(dlg->H(), spin_ctl), BIN(checked));
			EnableWindow(GetDlgItem(dlg->H(), copy_ctl), BIN(checked));
		}
		dlg->setCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
		if(oprType != PPOPT_GOODSRECEIPT || rpt_info_list.getCount() <= 7 || !rpt_info_list.at(7).Val) {
			only_price_chng = 0;
			dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
		}
		dlg->setCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
		dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(oprType != PPOPT_GOODSRECEIPT || rpt_info_list.getCount() <= 7 || !rpt_info_list.at(7).Val));
	}
	else if(res_id == DLG_PRNGBILL2) {
		if(oprType != PPOPT_GOODSRECEIPT) {
			only_price_chng = 0;
			dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
		}
		dlg->setCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
		dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(oprType != PPOPT_GOODSRECEIPT));
	}
	else {
		dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
	}
	dlg->setCtrlData(CTL_PRNGBILL_WHAT,  &v);
	dlg->setCtrlData(CTL_PRNGBILL_PRICE, &p);
	{
		dlg->setCtrlString(CTL_PRNGBILL_MAINORG, GetMainOrgName(temp_buf));
		GetLocationName(LConfig.Location, temp_buf);
		dlg->setCtrlString(CTL_PRNGBILL_LOC, temp_buf);
		dlg->disableCtrls(1, CTL_PRNGBILL_MAINORG, CTL_PRNGBILL_LOC, 0);
	}
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_PRNGBILL_WHAT,  &v);
		dlg->getCtrlData(CTL_PRNGBILL_PRICE, &p);
		dlg->getCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
		dlg->getCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
		ASSIGN_PTR(pAmtTypes, p+1);
		if(clu)
			if(res_id == DLG_PRNGBILLM) {
				if(v == 0)
					ok = -1;
				else {
					for(uint c = 0; c < BILL_FORM_COUNT; c++)
						if(((uint16)v >> c) & 0x0001)
							rSelAry.Add(static_cast<PPID>(c+1), static_cast<MultiPrintDialog *>(dlg)->GetNumCopies(CTL_PRNGBILL_NUMCOPIES + c), 0);
				}
			}
			else {
				v++;
				rSelAry.Add((PPID)v, 1, 0);
			}
		else
			rSelAry.Add((PPID)0, 1, 0);
		ASSIGN_PTR(pDivCopiesFlag, div_copies);
		ASSIGN_PTR(pOnlyPriceChangedFlag, only_price_chng);
	}
	else
		ok = -1;
	delete dlg;
	return ok;
}

static int SLAPI PrintInvoice(PPBillPacket * pPack, int prnflags)
{
	int    ok = 1, val = 0;
	PPReportEnv env;
	PPIniFile ini_file;
	env.PrnFlags |= prnflags;
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_INVFOOTONBOTT, &val) && val)
		env.PrnFlags |= SReport::FooterOnBottom;
	pPack->GetContextEmailAddr(env.EmailAddr);
	pPack->Rec.Flags |= BILLF_PRINTINVOICE;
	PPFilt pf(pPack);
	ok = PPAlddPrint(REPORT_INVOICE, &pf, &env);
	pPack->Rec.Flags &= ~BILLF_PRINTINVOICE;
	return ok;
}

static int SLAPI IsPriceChanged(const PPTransferItem * pTi, long procFlags)
{
	int price_chng = 1; // Цена изменилась по отношению к предыдущему лоту. Если не установлен флаг pfPrintChangedPriceOnly, то игнорируется.
	if(procFlags & PPBillPacket::pfPrintChangedPriceOnly) {
		//
		// Будем печатать только те товары, цены на которые изменились.
		//
		PPObjBill * p_bobj = BillObj;
		ReceiptCore * p_rcpt = (p_bobj && p_bobj->trfr) ? &p_bobj->trfr->Rcpt : 0;
		if(p_rcpt) {
			ReceiptTbl::Rec prev_rec, rec;
			if(p_rcpt->Search(pTi->LotID, &rec) > 0) {
				int r = p_rcpt->GetPreviousLot(rec.GoodsID, rec.LocID, rec.Dt, rec.OprNo, &prev_rec);
				price_chng = BIN(r <= 0 || rec.Price != prev_rec.Price);
				if(!price_chng) {
					double prev_rest = 0.0;
					p_bobj->trfr->GetRest(prev_rec.ID, rec.Dt, rec.OprNo, &prev_rest, 0);
					if(prev_rest <= 0.0)
						price_chng = 1;
				}
			}
		}
	}
	return price_chng;
}

static int SLAPI PrintBillImages(const PPBillPacket * pPack, int prnFlags)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	SString path;
	SPrinting prn(APPL->H_MainWnd);
	PPTransferItem * p_ti = 0;
	THROW_SL(prn.Init(0));
	for(uint i = 0; pPack->EnumTItems(&i, &p_ti) > 0;) {
		PPID org_lot_id = 0L;
		if(p_bobj->trfr->Rcpt.SearchOrigin(p_ti->LotID, &org_lot_id, 0, 0) > 0) {
			ObjTagList tag_list;
			p_bobj->GetTagListByLot(org_lot_id, 1, &tag_list);
			const uint tag_count = tag_list.GetCount();
			if(IsPriceChanged(p_ti, pPack->ProcessFlags) > 0) {
				for(uint j = 0; j < tag_count; j++) {
					const ObjTagItem * p_item = tag_list.GetItemByPos(j);
					if(p_item && p_item->TagDataType == OTTYP_IMAGE) {
						if(p_item->GetStr(path = 0) > 0 && path.NotEmptyS() && fileExists(path))
							THROW(prn.PrintImage(path));
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

struct RptSel { // @flat
	RptSel(PPID selID, uint rptID) : SelID(selID), RptID(rptID), NumCopies(1), OrBppFlags(0), NotBppFlags(0)
	{
	}
	PPID   SelID;
	uint   RptID;
	uint   NumCopies;
	long   OrBppFlags;
	long   NotBppFlags;
};

int FASTCALL PrintGoodsBill(PPBillPacket * pPack, SVector ** ppAry, int printingNoAsk) // @v9.8.6 SArray-->SVector
{
	int    ok = 1, div_copies_flag = 0, num_div_copies = 1, num_copies = 1, i, j;
	int    prn_no_ask = BIN(ppAry && *ppAry && printingNoAsk);
	uint   c, alt_rpt_id = 0, amt_types = 0;
	const  long preserve_process_flags = pPack->ProcessFlags;
	PPOprKind opk;
	PPObjOprKind op_obj;
	PPReportEnv env;
	pPack->GetContextEmailAddr(env.EmailAddr);
	pPack->ProcessFlags &= ~(PPBillPacket::pfPrintOnlyUnlimGoods|PPBillPacket::pfPrintPLabel|PPBillPacket::pfPrintQCertList);
	GetOpData(pPack->Rec.OpID, &opk);
	op_obj.GetExtStrData(pPack->Rec.OpID, OPKEXSTR_DEFPRNFORM, env.DefPrnForm);
	if(env.DefPrnForm.NotEmpty())
		GetReportIDByName(env.DefPrnForm, &alt_rpt_id);
	//
	if(pPack->Rec.Flags & BILLF_BANKING) {
		PPFilt pf(pPack);
		uint rpt_id = (pPack->P_PaymOrder && pPack->P_PaymOrder->Flags & BNKPAYMF_REQ) ? REPORT_BNKPAYMREQ : REPORT_BNKPAYMORDER;
		ok = PPAlddPrint(rpt_id, &pf, &env);
	}
	else if(pPack->OpTypeID == PPOPT_GOODSREVAL) {
		long   f = (opk.PrnFlags & (OPKF_PRT_BUYING | OPKF_PRT_SELLING));
		uint   rpt_id = (f == (OPKF_PRT_BUYING | OPKF_PRT_SELLING) || f == 0) ? REPORT_GREVALBILL : REPORT_GREVALBILLP;
		PPFilt pf(pPack);
		ok = PPAlddPrint(rpt_id, &pf, &env);
	}
	else if(pPack->OpTypeID == PPOPT_CORRECTION) {
		PPFilt pf(pPack);
		const  int is_exp_correction = BIN(pPack->P_LinkPack && pPack->P_LinkPack->OpTypeID == PPOPT_GOODSEXPEND); // @v10.3.8
		ok = PPAlddPrint(is_exp_correction ? REPORT_INVOICECORR_EXP : REPORT_INVOICECORR, &pf, &env);
	}
	else if(pPack->OpTypeID == PPOPT_INVENTORY) {
		PPViewInventory iv;
		InventoryFilt filt;
		filt.Setup(pPack->Rec.ID);
		THROW(iv.Init_(&filt));
		THROW(iv.Print(0));
	}
	else if(pPack->OpTypeID == PPOPT_PAYMENT && opk.PrnFlags & OPKF_PRT_INVOICE) {
		ok = PrintInvoice(pPack, 0);
	}
	else if(pPack->OpTypeID == PPOPT_ACCTURN && opk.SubType == OPSUBT_WARRANT) {
		PPFilt pf(pPack);
		ok = PPAlddPrint(REPORT_WARRANT, &pf, &env);
	}
	else if(pPack->OpTypeID == PPOPT_ACCTURN && opk.SubType == OPSUBT_ADVANCEREP) {
		PPFilt pf(pPack);
		ok = PPAlddPrint(REPORT_ADVANCEREP, &pf, &env);
	}
	else if(pPack->OpTypeID == PPOPT_ACCTURN && !alt_rpt_id) {
		long   f = (opk.PrnFlags & (OPKF_PRT_CASHORD | OPKF_PRT_INVOICE | OPKF_PRT_PAYPLAN));
		long   prf = 0;
		if(f == 0 || f == OPKF_PRT_CASHORD)
			prf = OPKF_PRT_CASHORD;
		else if(f == OPKF_PRT_INVOICE)
			prf = OPKF_PRT_INVOICE;
		else if(f == OPKF_PRT_PAYPLAN)
			prf = OPKF_PRT_PAYPLAN;
		else {
			ok = -1;
			TDialog * dlg = new TDialog(DLG_PRNACCTURN);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->AddClusterAssocDef(CTL_PRNACCTURN_FORM, 0, OPKF_PRT_CASHORD);
				dlg->AddClusterAssoc(CTL_PRNACCTURN_FORM, 1, OPKF_PRT_PAYPLAN);
				dlg->AddClusterAssoc(CTL_PRNACCTURN_FORM, 2, OPKF_PRT_INVOICE);
				dlg->SetClusterData(CTL_PRNACCTURN_FORM, prf);
				dlg->DisableClusterItem(CTL_PRNACCTURN_FORM, 0, !(f & OPKF_PRT_CASHORD));
				dlg->DisableClusterItem(CTL_PRNACCTURN_FORM, 1, !(f & OPKF_PRT_PAYPLAN));
				dlg->DisableClusterItem(CTL_PRNACCTURN_FORM, 2, !(f & OPKF_PRT_INVOICE));
				if(ExecView(dlg) == cmOK) {
					dlg->GetClusterData(CTL_PRNACCTURN_FORM, &prf);
					ok = 1;
				}
				delete dlg;
			}
			else
				ok = 0;
		}
		if(prf == OPKF_PRT_CASHORD)
			ok = PrintCashOrder(pPack, (opk.Flags & OPKF_PROFITABLE) ? 0 : 1);
		else if(prf == OPKF_PRT_INVOICE)
			ok = PrintInvoice(pPack, 0);
		else if(prf == OPKF_PRT_PAYPLAN) {
			PPFilt pf(pPack);
			ok = PPAlddPrint(REPORT_BILLPAYPLAN, &pf, &env);
		}
	}
	else {
		TSVector <RptSel> rpt_ids; // @v9.8.6 TSArray-->TSVector
		TSVector <RptSel> copy_rpt_ids; // @v9.8.6 TSArray-->TSVector
		if(prn_no_ask)
			rpt_ids.copy(**ppAry);
		if(pPack->Rec.CurID && !prn_no_ask) {
			RptSel rs(0, NZOR(alt_rpt_id, REPORT_INVOICECUR));
			THROW(rpt_ids.insert(&rs));
			amt_types = 3;
		}
		else if(pPack->OpTypeID == PPOPT_GOODSMODIF && !prn_no_ask) {
			RptSel rs(0, NZOR(alt_rpt_id, REPORT_GOODSBILLMODIF));
			THROW(rpt_ids.insert(&rs));
		}
		else if(!prn_no_ask) {
			RptSel rs(0, 0);
			if(opk.PrnFlags & OPKF_PRT_BUYING && opk.PrnFlags & OPKF_PRT_SELLING) {
				rs.RptID = NZOR(alt_rpt_id, REPORT_GOODSBILLCP);
				amt_types = 3;
			}
			else {
				if(alt_rpt_id)
					rs.RptID = alt_rpt_id;
				else if(opk.OpTypeID == PPOPT_GOODSORDER)
					rs.RptID = REPORT_GOODSORDER;
				else if(opk.PrnOrder == TiIter::ordBySuppl)
					rs.RptID = (opk.PrnFlags & OPKF_PRT_VATAX) ? REPORT_GOODSBILLVTSUPPL : REPORT_GOODSBILLSUPPL;
				else if(opk.PrnOrder == TiIter::ordByStorePlaceGrpGoods)
					rs.RptID = REPORT_GOODSBILLWPGGRP;
				else
					rs.RptID = (opk.PrnFlags & OPKF_PRT_VATAX) ? REPORT_GOODSBILLVT : REPORT_GOODSBILL;
				if(opk.PrnFlags & OPKF_PRT_BUYING)
					amt_types = 1;
				else if(opk.PrnFlags & OPKF_PRT_SELLING)
					amt_types = 2;
				else // Default
					amt_types = (IsSellingOp(opk.ID) > 0 || IsIntrExpndOp(opk.ID)) ? 2 : 1;
			}
			pPack->OutAmtType = amt_types;
			if(opk.PrnFlags & (OPKF_PRT_EXTFORMFLAGS|OPKF_PRT_SELPRICE)) {
				int    only_price_changed_flag = 0;
				LAssocArray sel_ary;
				THROW(ok = SelectForm(opk.PrnFlags, (opk.PrnFlags & OPKF_PRT_SELPRICE) ? &amt_types : 0, sel_ary, pPack->OpTypeID, &div_copies_flag, &only_price_changed_flag));
				pPack->OutAmtType = amt_types;
				if(ok > 0) {
					for(c = 0; c < sel_ary.getCount(); c++) {
						RptSel temp_rs(0, 0);
						temp_rs = rs;
						temp_rs.SelID = sel_ary.at(c).Key;
						switch(temp_rs.SelID) {
							case 2:
								temp_rs.RptID = CheckOpPrnFlags(pPack->Rec.OpID, OPKF_PRT_QCG) ? REPORT_QCERTLISTG : REPORT_QCERTLIST;
								break;
							case 3:
							case 4: temp_rs.RptID = 0; break;
							case 5: temp_rs.RptID = REPORT_GOODSLADINGBILL; break;
							case 6: temp_rs.RptID = REPORT_GOODSLADINGBACK; break;
							case 7:
								temp_rs.RptID = REPORT_SERVICEACT;
								temp_rs.OrBppFlags = PPBillPacket::pfPrintOnlyUnlimGoods;
								pPack->ProcessFlags |= PPBillPacket::pfPrintOnlyUnlimGoods;
								break;
							case 8:
								temp_rs.RptID = REPORT_PLABEL;
								temp_rs.OrBppFlags |= PPBillPacket::pfPrintPLabel;
								if(only_price_changed_flag)
									temp_rs.OrBppFlags |= PPBillPacket::pfPrintChangedPriceOnly;
								else
									temp_rs.NotBppFlags |= PPBillPacket::pfPrintChangedPriceOnly;
								pPack->ProcessFlags |= PPBillPacket::pfPrintPLabel;
								SETFLAG(pPack->ProcessFlags, PPBillPacket::pfPrintChangedPriceOnly, BIN(only_price_changed_flag));
								break;
							case 9: // План платежей
								break;
							case 10:
								temp_rs.RptID = REPORT_BILLTARESALDO;
								temp_rs.OrBppFlags = PPBillPacket::pfPrintTareSaldo;
								pPack->ProcessFlags |= PPBillPacket::pfPrintTareSaldo;
								break;
							case 11: // Наряд на складскую сборку
								temp_rs.RptID = REPORT_GOODSBILLLOCDISP;
								break;
							case 12: // Изображения из тегов лотов
								temp_rs.RptID = 0;
								break;
							case 13: // Универсальный передаточный документ // @v8.1.3
								temp_rs.RptID = REPORT_UNIBILL;
								break;
							default:
								if(alt_rpt_id)
									temp_rs.RptID = alt_rpt_id;
								else if(amt_types == 3)
									temp_rs.RptID = REPORT_GOODSBILLCP;
								else if(opk.OpTypeID == PPOPT_GOODSORDER)
									temp_rs.RptID = REPORT_GOODSORDER;
								else if(opk.PrnFlags & OPKF_PRT_VATAX)
									temp_rs.RptID = (opk.PrnOrder == TiIter::ordBySuppl) ? REPORT_GOODSBILLVTSUPPL : REPORT_GOODSBILLVT;
								else if(opk.PrnOrder == TiIter::ordBySuppl)
									temp_rs.RptID = REPORT_GOODSBILLSUPPL;
								else if(opk.PrnOrder == TiIter::ordByStorePlaceGrpGoods)
									temp_rs.RptID = REPORT_GOODSBILLWPGGRP;
								else
									temp_rs.RptID = REPORT_GOODSBILL;
								break;
						}
						temp_rs.NumCopies = sel_ary.at(c).Val;
						THROW(rpt_ids.insert(&temp_rs));
					}
				}
			}
			else {
				SETIFZ(rs.NumCopies, 1);
				THROW(rpt_ids.insert(&rs));
			}
		}
		if(ok > 0) {
			PPFilt pf(pPack);
			env.PrnFlags = SReport::NoRepError;
			if(rpt_ids.getCount() > 1 || printingNoAsk)
				env.PrnFlags |= SReport::PrintingNoAsk;
			/* @v8.6.6 @obsolete
			if(LConfig.Flags & CFGFLG_NEJPBILL && !(env.PrnFlags & SReport::PrintingNoAsk) && rpt_ids.at(0).RptID)
				env.PrnFlags |= SReport::NoEjectAfter;
			*/
			if(div_copies_flag)
				for(uint i = 0; i < rpt_ids.getCount(); i++)
					num_div_copies = MAX((long)rpt_ids.at(i).NumCopies, num_div_copies);
			copy_rpt_ids = rpt_ids;
			for(i = 0; i < num_div_copies; i++) {
				for(c = 0; ok > 0 && c < copy_rpt_ids.getCount(); c++) {
					RptSel & r_rs = copy_rpt_ids.at(c);
					int    num_copies = (num_div_copies > 1) ? BIN(r_rs.NumCopies > 0) : r_rs.NumCopies;
					pPack->ProcessFlags = (preserve_process_flags & ~(PPBillPacket::pfPrintOnlyUnlimGoods|PPBillPacket::pfPrintPLabel|PPBillPacket::pfPrintQCertList));
					if(r_rs.OrBppFlags)
						pPack->ProcessFlags |= r_rs.OrBppFlags;
					if(r_rs.NotBppFlags)
						pPack->ProcessFlags &= ~r_rs.NotBppFlags;
					for(j = 0; ok > 0 && j < num_copies; j++) {
						if(r_rs.SelID == 3)
							ok = PrintInvoice(pPack, env.PrnFlags); // @v7.1.7 pPack-->&temp_pack
						else if(r_rs.SelID == 4)
							ok = PrintCashOrderByGoodsBill(pPack, env.PrnFlags); // @v7.1.7 pPack-->&temp_pack
						else if(r_rs.SelID == 12) // Печать изображений из тегов лотов
							ok = PrintBillImages(pPack, env.PrnFlags);
						else
							ok = PPAlddPrint(r_rs.RptID, &pf, &env);
					}
					r_rs.NumCopies -= num_copies;
				}
				THROW(ok);
			}
			if(ppAry && !*ppAry && printingNoAsk) {
				THROW_MEM(*ppAry = new SVector(sizeof(RptSel))); // @v9.8.6 SArray-->SVector
				(*ppAry)->copy(rpt_ids);
			}
		}
	}
	CATCHZOKPPERR
	pPack->ProcessFlags = (preserve_process_flags & ~PPBillPacket::pfPrintOnlyUnlimGoods);
	return ok;
}

int SLAPI PrintCashOrderByGoodsBill(PPBillPacket * pPack, int prnflags)
{
	int    ok = 1;
	const  double amt = BR2(pPack->Rec.Amount);
	if(amt != 0) {
		int    x;
		if(pPack->OpTypeID == PPOPT_PAYMENT) {
			PPOprKind op_rec, link_op_rec;
			GetOpData(pPack->Rec.OpID, &op_rec);
			PPID   link_op_type = GetOpType(op_rec.LinkOpID, &link_op_rec);
			if(link_op_type == PPOPT_ACCTURN)
				x = BIN(link_op_rec.Flags & OPKF_PROFITABLE);
			else
				x = BIN(IsExpendOp(op_rec.LinkOpID) > 0 || link_op_type == PPOPT_GOODSORDER);
			x = (amt > 0) ? x : !x;
		}
		else if(pPack->OpTypeID == PPOPT_GOODSORDER)
			x = 1;
		else if(pPack->Rec.Flags & (BILLF_GRECEIPT|BILLF_GEXPEND))
			x = (IsExpendOp(pPack->Rec.OpID) > 0);
		else if(pPack->OpTypeID == PPOPT_DRAFTRECEIPT)
			x = 0;
		else if(pPack->OpTypeID == PPOPT_DRAFTEXPEND)
			x = 1;
		else
			return -1;
		ok = PrintCashOrder(pPack, BIN(x == 0), prnflags);
	}
	return ok;
}

int SLAPI PrintCashOrder(PPBillPacket * pPack, int pay_rcv, int prnflags)
{
	PPFilt pf(pPack);
	pf.ID  = pay_rcv ? 1 : 2;
	PPReportEnv env;
	env.PrnFlags = prnflags;
	pPack->GetContextEmailAddr(env.EmailAddr);
	uint   rpt_id = pay_rcv ? REPORT_CASHPAYORDER : REPORT_CASHRCVORDER;
	return PPAlddPrint(rpt_id, &pf, &env);
}


