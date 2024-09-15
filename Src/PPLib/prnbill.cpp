// PRNBILL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

//static const char * BillMultiplePrintCfg2 = "BillMultiplePrintCfg2"; // @v11.2.0
// PPConst::WrParam_BillMultiplePrintCfg2
//#define BILL_FORM_COUNT 13

class MultiPrintDialog : public TDialog {
	DECL_DIALOG_DATA(BillMultiPrintParam);
public:
	MultiPrintDialog(PPID oprType) : TDialog(DLG_PRNGBILLM), OprType(oprType)
	{
		SString temp_buf;
		setCtrlString(CTL_PRNGBILL_MAINORG, GetMainOrgName(temp_buf));
		GetLocationName(LConfig.Location, temp_buf);
		setCtrlString(CTL_PRNGBILL_LOC, temp_buf);
		//disableCtrls(1, CTL_PRNGBILL_MAINORG, CTL_PRNGBILL_LOC, 0);
		setCtrlReadOnly(CTL_PRNGBILL_MAINORG, 1);
		setCtrlReadOnly(CTL_PRNGBILL_LOC, 1);
	}
	long   GetNumCopies(uint ctl)
	{
		SString temp_buf;
		TView::SGetWindowText(GetDlgItem(H(), ctl), temp_buf);
		return temp_buf.ToLong();
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		//LAssocArray rpt_info_list;
		RVALUEPTR(Data, pData);
		setCtrlUInt16(CTL_PRNGBILL_WHAT, static_cast<uint16>(Data.FormBits));
		for(uint i = 0; i < BillMultiPrintParam::pb__Count; i++) {
			const uint spin_ctl = CTL_PRNGBILL_SPIN+i;
			const uint copy_ctl = CTL_PRNGBILL_NUMCOPIES+i;
			long   num_copies = NZOR(Data.CopyCounter[i], 1);
			long   checked    = (Data.FormBits & (1 << i));
			SetupSpin(spin_ctl, copy_ctl, 1, 10, num_copies);
			EnableWindow(GetDlgItem(H(), spin_ctl), BIN(checked));
			EnableWindow(GetDlgItem(H(), copy_ctl), BIN(checked));
		}
		AddClusterAssoc(CTL_PRNGBILL_FLAGS, 0, BillMultiPrintParam::fMakeOutCopies);
		AddClusterAssoc(CTL_PRNGBILL_FLAGS, 1, BillMultiPrintParam::fUpdatedPricesOnly);
		//dlg->setCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
		if(OprType != PPOPT_GOODSRECEIPT || !(Data.FormBits & Data.pbPriceTag)) {
			//only_price_chng = 0;
			//dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
			Data.Flags &= ~BillMultiPrintParam::fUpdatedPricesOnly;
			DisableClusterItem(CTL_PRNGBILL_FLAGS, 1, true);
		}
		//dlg->setCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
		//dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(oprType != PPOPT_GOODSRECEIPT || rpt_info_list.getCount() <= 7 || !rpt_info_list.at(7).Val));
		//DisableClusterItem(CTL_PRNGBILL_FLAGS, 1, BIN(OprType != PPOPT_GOODSRECEIPT || !(Data.FormBits & Data.pbPriceTag)));
		SetClusterData(CTL_PRNGBILL_FLAGS, Data.Flags);
		//
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		{
			//uint16 v = 0;
			//uint16 p = 0;
			//getCtrlData(CTL_PRNGBILL_WHAT,  &v);
			//getCtrlData(CTL_PRNGBILL_PRICE, &p);
			Data.FormBits = static_cast<uint32>(getCtrlUInt16(CTL_PRNGBILL_WHAT));
			Data.Flags = GetClusterData(CTL_PRNGBILL_FLAGS);
			//ASSIGN_PTR(pAmtTypes, p+1);
			if(Data.FormBits) {
				for(uint c = 0; c < BillMultiPrintParam::pb__Count; c++) {
					if(Data.FormBits & (1 << c)) {
						long nc = GetNumCopies(CTL_PRNGBILL_NUMCOPIES + c);
						Data.CopyCounter[c] = (checkirange(nc, 1L, 10L)) ? static_cast<uint16>(nc) : 1;
						//rSelAry.Add(static_cast<PPID>(c+1), GetNumCopies(CTL_PRNGBILL_NUMCOPIES + c), 0);
					}
					else 
						Data.CopyCounter[c] = 0;
				}
			}
			//ASSIGN_PTR(pDivCopiesFlag, /*div_copies*/BIN(prn_flags & BillMultiPrintParam::fMakeOutCopies));
			//ASSIGN_PTR(pOnlyPriceChangedFlag, /*only_price_chng*/BIN(prn_flags & BillMultiPrintParam::fUpdatedPricesOnly));
		}
		//
		ASSIGN_PTR(pData, Data);
		return ok;
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
				for(c = 0; c < BillMultiPrintParam::pb__Count; c++)
					if((static_cast<uint16>(w) >> c) & 0x0001)
						i++;
				if(p_clu) {
					for(c = 0; c < BillMultiPrintParam::pb__Count; c++) {
						const int checked = p_clu->mark(c);
						HWND w_nc = GetDlgItem(H(), CTL_PRNGBILL_NUMCOPIES + c);
						HWND w_spin = GetDlgItem(H(), CTL_PRNGBILL_SPIN + c);
						if(i < 2 || !checked)
							SendDlgItemMessage(H(), CTL_PRNGBILL_SPIN + c, UDM_SETPOS, 0, MAKELONG(1, 0));
						EnableWindow(w_nc, checked);
						EnableWindow(w_spin, checked);
						if(c == BillMultiPrintParam::pbPriceTag) // AHTOXA Ценник
							disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, BIN(OprType != PPOPT_GOODSRECEIPT || !checked));
					}
				}
				else
					disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
				clearEvent(event);
			}
		}
		else if(event.isCmd(cmSave)) {
			/*if(p_clu) {
				//int    div_copies = 0;
				//int    only_price_chng = 0;
				SString prn_cfg;
				//getCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
				//getCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
				Data.Flags = GetClusterData(CTL_PRNGBILL_FLAGS);
				for(uint c = 0; c < BillMultiPrintParam::pb__Count; c++) {
					const int checked = p_clu->mark(c);
					if(c != 0)
						prn_cfg.Semicol();
					prn_cfg.Cat(c + 1).Comma().Cat(BIN(checked)).Comma().Cat(GetNumCopies(CTL_PRNGBILL_NUMCOPIES + c));
				}
				WinRegKey key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
				key.PutString(BillMultiplePrintCfg, prn_cfg);
				key.PutDWord(BillMultiplePrintDivByCopies, (uint32)(Data.Flags & Data.fMakeOutCopies));
				key.PutDWord(BillMultiplePrintOnlyPriceChanged, (uint32)(Data.Flags & Data.fUpdatedPricesOnly));
			}*/
			BillMultiPrintParam temp_data;
			if(getDTS(&temp_data)) {
				if(!temp_data.LocalSave())
					PPError();
			}
			clearEvent(event);
		}
	}
	const  PPID   OprType;
};

BillMultiPrintParam::BillMultiPrintParam() : FormBits(0), Flags(0)
{
	MEMSZERO(Reserve);
	MEMSZERO(CopyCounter);
}

bool BillMultiPrintParam::IsEmpty() const
{
	return (FormBits == 0);
}

bool FASTCALL BillMultiPrintParam::IsEq(const BillMultiPrintParam & rS) const
{
	return (FormBits == rS.FormBits && Flags == rS.Flags && memcmp(CopyCounter, rS.CopyCounter, sizeof(CopyCounter)) == 0 &&
		CustomFormNames == rS.CustomFormNames);
}
	
BillMultiPrintParam & BillMultiPrintParam::Z()
{
	FormBits = 0;
	Flags = 0;
	MEMSZERO(Reserve);
	MEMSZERO(CopyCounter);
	CustomFormNames.Z();
	return *this;
}

int BillMultiPrintParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 signature = PPConst::Signature_BillMultiPrintParam;
	THROW_SL(pSCtx->Serialize(dir, signature, rBuf));
	THROW(dir > 0 || signature == PPConst::Signature_BillMultiPrintParam);
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Reserve), Reserve, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, FormBits, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(CopyCounter), CopyCounter, rBuf, 0));
	THROW_SL(CustomFormNames.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int BillMultiPrintParam::LocalSave()
{
	int    ok = 1;
	if(IsEmpty()) {
		WinRegKey key;
		THROW_SL(key.DeleteValue(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, PPConst::WrParam_BillMultiplePrintCfg2));
	}
	else {
		SSerializeContext sctx;
		SBuffer sbuf;
		THROW(Serialize(+1, sbuf, &sctx));
		{
			WinRegKey key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 0);
			THROW_SL(key.PutBinary(PPConst::WrParam_BillMultiplePrintCfg2, sbuf.constptr(), sbuf.GetAvailableSize()));
		}
	}
	CATCHZOK
	return ok;
}
	
int BillMultiPrintParam::LocalRestore()
{
	Z();
	int    ok = -1;
	WinRegKey key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 1);
	size_t rec_size = 0;
	int r = key.GetRecSize(PPConst::WrParam_BillMultiplePrintCfg2, &rec_size);
	if(r > 0) {
		STempBuffer tb(rec_size + 64); // (+64) insurance
		THROW_SL(key.GetBinary(PPConst::WrParam_BillMultiplePrintCfg2, tb, tb.GetSize()) > 0);
		{
			SBuffer sbuf;
			SSerializeContext sctx;
			THROW_SL(sbuf.Write(tb, rec_size));
			THROW(Serialize(-1, sbuf, &sctx));
		}
	}
	else if(r < 0) {
		ok = -1;
		//
		// Пытаемся считать данные в формате, предшествующем v11.2.0
		//
		WinRegKey old_key(HKEY_CURRENT_USER, PPConst::WrKey_SysSettings, 1);
		{
			static const char * BillMultiplePrintCfg              = "BillMultiplePrintCfg";
			static const char * BillMultiplePrintDivByCopies      = "BillMultiplePrintDivByCopies";
			static const char * BillMultiplePrintOnlyPriceChanged = "BillMultiplePrintOnlyPriceChanged";
			SString temp_buf;
			old_key.GetString(BillMultiplePrintCfg, temp_buf);
			if(temp_buf.NotEmptyS()) {
				StringSet ss(';', temp_buf);
				for(uint p = 0, i = 0; ss.get(&p, temp_buf); i++) {
					uint   p1 = 0;
					StringSet ss1(',', temp_buf);
					ss1.get(&p1, temp_buf);   // ID
					ss1.get(&p1, temp_buf);   // State
					long   checked = temp_buf.ToLong();
					SETFLAG(FormBits, 0x0001 << i, checked);
					ss1.get(&p1, temp_buf);  // Num copies
					if(checked) {
						CopyCounter[i] = inrangeordefault(static_cast<int>(temp_buf.ToLong()), 1, 10, 1);
					}
				}
				uint32 val = 0;
				old_key.GetDWord(BillMultiplePrintDivByCopies, &val);
				//div_copies = (val) ? 1 : div_copies;
				if(val)
					Flags |= BillMultiPrintParam::fMakeOutCopies;
				old_key.GetDWord(BillMultiplePrintOnlyPriceChanged, &val);
				//only_price_chng = val ? 1 : only_price_chng;
				if(val)
					Flags |= BillMultiPrintParam::fUpdatedPricesOnly;
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int BillMultiPrintParam::EditDialog(PPID opTypeID, BillMultiPrintParam * pData)
{
	DIALOG_PROC_BODY_P1(MultiPrintDialog, opTypeID, pData);
}
//
// Descr: Структура, устанавливающая соответствие между опцией печати документа, 
//    индексом элемента в диалогах и приоритетом вывода на печать.
//
struct BillPrintFormEntry { // @flat
	ulong  Flag;            // OPKF_PRT_XXX Флаг опций печати документов. 0 - default-вариант (всегда есть возможность выбора)
	uint16 DialogEntryIdx;  // Индекс в диалогах выбора форм(ы) [0..]
	uint8  Priority;        // Приоритет вывода на печать (0 - наивысший, 1 - ниже и т.д.)
	uint8  NumCopies;       // Значение используется непосредственно при печати. Пользователь выбирает это поле в диалоге. 
};

static IMPL_CMPFUNC(BillPrintFormEntry, p1, p2)
{
	RET_CMPCASCADE2(static_cast<const BillPrintFormEntry *>(p1), static_cast<const BillPrintFormEntry *>(p2), Priority, DialogEntryIdx);
}

static const BillPrintFormEntry BillPrintFormList[] = {
	{ 0L,                  0, 1, 0 }, //    0. Накладная
	{ OPKF_PRT_QCERT,      1, 1, 0 }, //    1. Сертификаты
	{ OPKF_PRT_INVOICE,    2, 1, 0 }, //    2. Счет-фактура
	{ OPKF_PRT_CASHORD,    3, 1, 0 }, //    3. Кассовый ордер
	{ OPKF_PRT_LADING,     4, 1, 0 }, //    4. Товарно-транспортная накладна
	{ OPKF_PRT_LADING,     5, 1, 0 }, //    5. Товарно-транспортная накладная (транспортный раздел)
	{ OPKF_PRT_SRVACT,     6, 1, 0 }, //    6. Акт выполненных работ
	{ OPKF_PRT_PLABEL,     7, 1, 0 }, //    7. Ценник
	{ OPKF_PRT_PAYPLAN,    8, 1, 0 }, //    8. План платежей
	{ OPKF_PRT_TARESALDO,  9, 1, 0 }, //    9. Сальдо по отгруженной таре
	{ OPKF_PRT_LOCDISP,   10, 1, 0 }, //    10. Наряд на складскую сборку
	{ OPKF_PRT_LOTTAGIMG, 11, 2, 0 }, //    11. Изображения из тегов лотов
	{ OPKF_PRT_INVOICE,   12, 0, 0 }, //    12. Универсальный передаточный документ	
};

static int SelectForm(int interactive, long opPrnFlags, PPID arID, uint * pAmtTypes, 
	/*LAssocArray & rSelAry*/TSVector <BillPrintFormEntry> & rSelectionList, PPID oprType, long * pOutFlags)
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
		const  PPID OprType;
	};
	int    ok = 1;
	int    v = 0;
	long   prn_flags = 0;
	ushort p = 0;
	uint   res_id = 0;
	SString temp_buf;
	PPObjArticle ar_obj;
	TDialog * dlg = 0;
	if(pAmtTypes == 0) {
		// @v10.3.0 Теперь используется (с приоритетом) интерфейсная настройка для разрешения/запрета множественной печати
		bool   allow_mult_print = false;
		const  UserInterfaceSettings uis = APPL->GetUiSettings();
		// @v11.2.6 const int uis_r = uis.Restore();
		if((uis.Flags & uis.fEnalbeBillMultiPrint) && !(uis.Flags & uis.fDisableBillMultiPrint))
			allow_mult_print = true;
		else if(!(uis.Flags & uis.fEnalbeBillMultiPrint) && (uis.Flags & uis.fDisableBillMultiPrint))
			allow_mult_print = false;
		else {
			PPBillConfig cfg;
			if(PPObjBill::ReadConfig(&cfg) > 0 && cfg.Flags & BCF_ALLOWMULTIPRINT)
				allow_mult_print = true;
		}
		res_id = allow_mult_print ? DLG_PRNGBILLM : DLG_PRNGBILL;
	}
	else if(opPrnFlags & OPKF_PRT_EXTFORMFLAGS)
		res_id = DLG_PRNGBILL2;
	else
		res_id = DLG_PRNGBILL3;
	if(res_id == DLG_PRNGBILLM) {
		//dlg = new MultiPrintDialog(/*res_id,*/oprType);
		BillMultiPrintParam bmpp;
		bool do_restore_local = true;
		PPClientAgreement agt;
		if(arID >= 0 && ar_obj.GetClientAgreement(arID, agt, 1/*use_default*/) > 0) {
			if(!agt.Bmpp.IsEmpty()) {
				bmpp = agt.Bmpp;
				do_restore_local = false;
			}
		}
		if(interactive) {
			if(do_restore_local)
				bmpp.LocalRestore();
			if(BillMultiPrintParam::EditDialog(oprType, &bmpp) > 0) {
				//rSelAry.clear();
				rSelectionList.clear();
				for(uint i = 0; i < BillMultiPrintParam::pb__Count; i++) {
					if(bmpp.FormBits & (1 << i)) {
						uint8 num_copies = static_cast<uint8>(inrangeordefault(bmpp.CopyCounter[i], 1, 10, 1));
						//rSelAry.Add(static_cast<PPID>(i+1), num_copies, 0);
						BillPrintFormEntry new_entry(BillPrintFormList[i]);
						new_entry.NumCopies = num_copies;
						rSelectionList.insert(&new_entry);
					}
				}
				ASSIGN_PTR(pOutFlags, bmpp.Flags);
				//ASSIGN_PTR(pDivCopiesFlag, BIN(bmpp.Flags & BillMultiPrintParam::fMakeOutCopies));
				//ASSIGN_PTR(pOnlyPriceChangedFlag, BIN(bmpp.Flags & BillMultiPrintParam::fUpdatedPricesOnly));
				ok = 1;
			}
			else
				ok = -1;
		}
		else {
			if(!agt.Bmpp.IsEmpty() && !(agt.Flags & AGTF_DEFAULT)) {
				//rSelAry.clear();
				rSelectionList.clear();
				for(uint i = 0; i < BillMultiPrintParam::pb__Count; i++) {
					if(bmpp.FormBits & (1 << i)) {
						uint8 num_copies = static_cast<uint8>(inrangeordefault(bmpp.CopyCounter[i], 1, 10, 1));
						//rSelAry.Add(static_cast<PPID>(i+1), num_copies, 0);
						BillPrintFormEntry new_entry(BillPrintFormList[i]);
						new_entry.NumCopies = num_copies;
						rSelectionList.insert(&new_entry);
					}
				}
				ASSIGN_PTR(pOutFlags, bmpp.Flags);
			}
			ok = 1;
		}
	}
	else {
		bool do_exit = false;
		TCluster * clu = 0;
		dlg = new BillPrintDialog(res_id, oprType);
		THROW(CheckDialogPtr(&dlg));
		clu = static_cast<TCluster *>(dlg->getCtrlView(CTL_PRNGBILL_WHAT));
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
		//    10. Наряд на складскую сборку
		//    11. Изображения из тегов лотов
		//    12. Универсальный передаточный документ
		//
		if(opPrnFlags & OPKF_PRT_EXTFORMFLAGS) {
			if(clu) {
				/*
				const LAssocBase flags_to_cluster_item_list[] = {
					// { 0, 0 },                //    0. Накладная //
					{ OPKF_PRT_QCERT,      1 }, //    1. Сертификаты
					{ OPKF_PRT_INVOICE,    2 }, //    2. Счет-фактура
					{ OPKF_PRT_CASHORD,    3 }, //    3. Кассовый ордер
					{ OPKF_PRT_LADING,     4 }, //    4. Товарно-транспортная накладна
					{ OPKF_PRT_LADING,     5 }, //    5. Товарно-транспортная накладная (транспортный раздел)
					{ OPKF_PRT_SRVACT,     6 }, //    6. Акт выполненных работ
					{ OPKF_PRT_PLABEL,     7 }, //    7. Ценник
					{ OPKF_PRT_PAYPLAN,    8 }, //    8. План платежей
					{ OPKF_PRT_TARESALDO,  9 }, //    9. Сальдо по отгруженной таре
					// {0, 10 },                //    10. Наряд на складскую сборку
					{ static_cast<long>(OPKF_PRT_LOTTAGIMG), 11 }, //    11. Изображения из тегов лотов
					{ OPKF_PRT_INVOICE,   12 }, //    12. Универсальный передаточный документ
				};
				for(uint i = 0; i < SIZEOFARRAY(flags_to_cluster_item_list); i++) {
					if(!(opPrnFlags & flags_to_cluster_item_list[i].Key))
						clu->disableItem(i+1, flags_to_cluster_item_list[i].Val); // @v11.2.2 @fix i-->i+1
				}
				clu->disableItem(10, 0); // @v11.2.2 Наряд на складскую сборку
				*/
				for(uint i = 0; i < SIZEOFARRAY(BillPrintFormList); i++) {
					const BillPrintFormEntry & r_entry = BillPrintFormList[i];
					if(!oneof2(r_entry.Flag, 0,  OPKF_PRT_LOCDISP) && !(opPrnFlags & r_entry.Flag)) {
						clu->disableItem(i/*+1*/, r_entry.DialogEntryIdx);
					}
				}
			}
		}
		else {
			v = 1;
			if(!pAmtTypes) {
				//rSelAry.Add(static_cast<PPID>(v), 1, 0);
				BillPrintFormEntry new_entry(BillPrintFormList[0]);
				new_entry.NumCopies = 1;
				rSelectionList.insert(&new_entry);
				do_exit = true;
			}
		}
		if(!do_exit) {
			if(pAmtTypes)
				p = (*pAmtTypes - 1);
			if(res_id == DLG_PRNGBILL2) {
				if(oprType != PPOPT_GOODSRECEIPT) {
					//only_price_chng = 0;
					prn_flags &= ~BillMultiPrintParam::fUpdatedPricesOnly;
					dlg->disableCtrl(CTL_PRNGBILL_ONLYPRCHNG, 1);
				}
				dlg->setCtrlUInt16(CTL_PRNGBILL_ONLYPRCHNG, /*&only_price_chng*/BIN(prn_flags & BillMultiPrintParam::fUpdatedPricesOnly));
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
				//dlg->getCtrlData(CTL_PRNGBILL_FDIVCOPIES, &div_copies);
				//dlg->getCtrlData(CTL_PRNGBILL_ONLYPRCHNG, &only_price_chng);
				const uint16 __v = dlg->getCtrlUInt16(CTL_PRNGBILL_ONLYPRCHNG);
				SETFLAG(prn_flags, BillMultiPrintParam::fUpdatedPricesOnly, __v);
				ASSIGN_PTR(pAmtTypes, p+1);
				if(clu) {
					//v++;
					//rSelAry.Add(static_cast<PPID>(v), 1, 0);
					assert(v >= 0 && v < SIZEOFARRAY(BillPrintFormList));
					BillPrintFormEntry new_entry(BillPrintFormList[v]);
					new_entry.NumCopies = 1;
					rSelectionList.insert(&new_entry);
				}
				else {
					//rSelAry.Add(0L, 1, 0);
					BillPrintFormEntry new_entry(BillPrintFormList[v]);
					new_entry.NumCopies = 1;
					rSelectionList.insert(&new_entry);
				}
				ASSIGN_PTR(pOutFlags, prn_flags);
				//ASSIGN_PTR(pDivCopiesFlag, /*div_copies*/BIN(prn_flags & BillMultiPrintParam::fMakeOutCopies));
				//ASSIGN_PTR(pOnlyPriceChangedFlag, /*only_price_chng*/BIN(prn_flags & BillMultiPrintParam::fUpdatedPricesOnly));
			}
			else
				ok = -1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

static int PrintInvoice(PPBillPacket * pPack, int prnflags)
{
	int    ok = 1;
	int    val = 0;
	PPReportEnv env;
	PPIniFile ini_file;
	env.PrnFlags |= prnflags;
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_INVFOOTONBOTT, &val) && val)
		env.PrnFlags |= SReport::FooterOnBottom;
	pPack->GetContextEmailAddr(env.EmailAddr);
	pPack->Rec.Flags |= BILLF_PRINTINVOICE;
	ok = PPAlddPrint(REPORT_INVOICE, PPFilt(pPack), &env);
	pPack->Rec.Flags &= ~BILLF_PRINTINVOICE;
	return ok;
}

bool PPObjBill::IsPriceChanged(const PPTransferItem * pTi, long procFlags)
{
	bool price_chng = true; // Цена изменилась по отношению к предыдущему лоту. Если не установлен флаг pfPrintChangedPriceOnly, то игнорируется.
	if(procFlags & PPBillPacket::pfPrintChangedPriceOnly) {
		//
		// Будем печатать только те товары, цены на которые изменились.
		//
		ReceiptTbl::Rec prev_rec, rec;
		if(trfr->Rcpt.Search(pTi->LotID, &rec) > 0) {
			int r = trfr->Rcpt.GetPreviousLot(rec.GoodsID, rec.LocID, rec.Dt, rec.OprNo, &prev_rec);
			price_chng = (r <= 0 || rec.Price != prev_rec.Price);
			if(!price_chng) {
				double prev_rest = 0.0;
				trfr->GetRest(prev_rec.ID, rec.Dt, rec.OprNo, &prev_rest, 0);
				if(prev_rest <= 0.0)
					price_chng = true;
			}
		}
	}
	return price_chng;
}

static int PrintBillImages(const PPBillPacket * pPack, int prnFlags)
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
			if(p_bobj->IsPriceChanged(p_ti, pPack->ProcessFlags)) {
				for(uint j = 0; j < tag_count; j++) {
					const ObjTagItem * p_item = tag_list.GetItemByPos(j);
					if(p_item && p_item->TagDataType == OTTYP_IMAGE) {
						if(p_item->GetStr(path) > 0 && path.NotEmptyS() && fileExists(path))
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
//
// interactive: 1 - yes, 0 - no, -1 - prepare for non interactive multiprint
//
int STDCALL Helper_PrintGoodsBill(PPBillPacket * pPack, SVector ** ppAry, long * pOutPrnFlags, int interactive)
{
	int    ok = 1;
	long   out_prn_flags = DEREFPTRORZ(pOutPrnFlags); // BillMultiPrintParam::fXXX
	//int    div_copies_flag = 0;
	int    num_div_copies = 1;
	int    num_copies = 1;
	int    i;
	int    j;
	//const  int prn_no_ask = BIN(ppAry && *ppAry && printingNoAsk);
	uint   c;
	uint   alt_rpt_id = 0;
	uint   amt_types = 0;
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
		uint rpt_id = (pPack->P_PaymOrder && pPack->P_PaymOrder->Flags & BNKPAYMF_REQ) ? REPORT_BNKPAYMREQ : REPORT_BNKPAYMORDER;
		ok = PPAlddPrint(rpt_id, PPFilt(pPack), &env);
	}
	else if(pPack->OpTypeID == PPOPT_GOODSREVAL) {
		long   f = (opk.PrnFlags & (OPKF_PRT_BUYING | OPKF_PRT_SELLING));
		uint   rpt_id = (f == (OPKF_PRT_BUYING | OPKF_PRT_SELLING) || f == 0) ? REPORT_GREVALBILL : REPORT_GREVALBILLP;
		ok = PPAlddPrint(rpt_id, PPFilt(pPack), &env);
	}
	else if(pPack->OpTypeID == PPOPT_CORRECTION) {
		const  bool is_exp_correction = (pPack->P_LinkPack && pPack->P_LinkPack->OpTypeID == PPOPT_GOODSEXPEND);
		ok = PPAlddPrint(is_exp_correction ? REPORT_INVOICECORR_EXP : REPORT_INVOICECORR, PPFilt(pPack), &env);
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
		ok = PPAlddPrint(REPORT_WARRANT, PPFilt(pPack), &env);
	}
	else if(pPack->OpTypeID == PPOPT_ACCTURN && opk.SubType == OPSUBT_ADVANCEREP) {
		ok = PPAlddPrint(REPORT_ADVANCEREP, PPFilt(pPack), &env);
	}
	else if(pPack->OpTypeID == PPOPT_ACCTURN && !alt_rpt_id) {
		long   f = (opk.PrnFlags & (OPKF_PRT_CASHORD | OPKF_PRT_INVOICE | OPKF_PRT_PAYPLAN));
		long   prf = 0;
		if(oneof2(f, 0, OPKF_PRT_CASHORD))
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
		else if(prf == OPKF_PRT_PAYPLAN)
			ok = PPAlddPrint(REPORT_BILLPAYPLAN, PPFilt(pPack), &env);
	}
	else {
		TSVector <RptSel> rpt_ids;
		TSVector <RptSel> copy_rpt_ids;
		if(!interactive && ppAry && *ppAry)
			rpt_ids.copy(**ppAry);
		else if(interactive && pPack->Rec.CurID) { // @v11.2.4 @fix (if)-->(else if)
			RptSel rs(0, NZOR(alt_rpt_id, REPORT_INVOICECUR));
			THROW(rpt_ids.insert(&rs));
			amt_types = 3;
		}
		else if(interactive && pPack->OpTypeID == PPOPT_GOODSMODIF) {
			RptSel rs(0, NZOR(alt_rpt_id, REPORT_GOODSBILLMODIF));
			THROW(rpt_ids.insert(&rs));
		}
		else {
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
				//LAssocArray sel_ary;
				TSVector <BillPrintFormEntry> selection_list;
				PPID   ar_id = (interactive < 0) ? 0 : pPack->Rec.Object;
				uint * p_amt_types = (opk.PrnFlags & OPKF_PRT_SELPRICE) ? &amt_types : 0;
				THROW(ok = SelectForm(interactive, opk.PrnFlags, ar_id, p_amt_types, /*sel_ary*/selection_list, pPack->OpTypeID, &out_prn_flags));
				selection_list.sort(PTR_CMPFUNC(BillPrintFormEntry));
				pPack->OutAmtType = amt_types;
				if(ok > 0) {
					if(!/*sel_ary*/selection_list.getCount()) {
						if(interactive > 0)
							rpt_ids.clear();
					}
					else {
						rpt_ids.clear();
						for(c = 0; c < /*sel_ary*/selection_list.getCount(); c++) {
							RptSel temp_rs(0, 0);
							temp_rs = rs;
							//temp_rs.SelID = sel_ary.at(c).Key;
							temp_rs.SelID = selection_list.at(c).DialogEntryIdx+1;
							switch(temp_rs.SelID) {
								case 2: temp_rs.RptID = CheckOpPrnFlags(pPack->Rec.OpID, OPKF_PRT_QCG) ? REPORT_QCERTLISTG : REPORT_QCERTLIST; break;
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
									if(out_prn_flags & BillMultiPrintParam::fUpdatedPricesOnly)
										temp_rs.OrBppFlags |= PPBillPacket::pfPrintChangedPriceOnly;
									else
										temp_rs.NotBppFlags |= PPBillPacket::pfPrintChangedPriceOnly;
									pPack->ProcessFlags |= PPBillPacket::pfPrintPLabel;
									SETFLAG(pPack->ProcessFlags, PPBillPacket::pfPrintChangedPriceOnly, BIN(out_prn_flags & BillMultiPrintParam::fUpdatedPricesOnly));
									break;
								case 9: break; // План платежей
								case 10:
									temp_rs.RptID = REPORT_BILLTARESALDO;
									temp_rs.OrBppFlags = PPBillPacket::pfPrintTareSaldo;
									pPack->ProcessFlags |= PPBillPacket::pfPrintTareSaldo;
									break;
								case 11: temp_rs.RptID = REPORT_GOODSBILLLOCDISP; break; // Наряд на складскую сборку
								case 12: temp_rs.RptID = 0; break; // Изображения из тегов лотов
								case 13: temp_rs.RptID = REPORT_UNIBILL; break; // Универсальный передаточный документ
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
							//temp_rs.NumCopies = sel_ary.at(c).Val;
							temp_rs.NumCopies = selection_list.at(c).NumCopies;
							THROW(rpt_ids.insert(&temp_rs));
						}
					}
				}
			}
			else {
				SETIFZ(rs.NumCopies, 1);
				THROW(rpt_ids.insert(&rs));
			}
		}
		if(ok > 0) {
			const bool dont_print_really = false; // for @debug set to true
			if(interactive >= 0 && !dont_print_really) {
				env.PrnFlags = SReport::NoRepError;
				if(rpt_ids.getCount() > 1 || /*printingNoAsk*/!interactive)
					env.PrnFlags |= SReport::PrintingNoAsk;
				if(/*div_copies_flag*/out_prn_flags & BillMultiPrintParam::fMakeOutCopies) {
					// @v10.8.7 for(uint i = 0; i < rpt_ids.getCount(); i++) { SETMAX(num_div_copies, static_cast<long>(rpt_ids.at(i).NumCopies)); }
					SForEachVectorItem(rpt_ids, i) { SETMAX(num_div_copies, static_cast<long>(rpt_ids.at(i).NumCopies)); } // @v10.8.7 
				}
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
								ok = PrintInvoice(pPack, env.PrnFlags);
							else if(r_rs.SelID == 4)
								ok = PrintCashOrderByGoodsBill(pPack, env.PrnFlags);
							else if(r_rs.SelID == 12) // Печать изображений из тегов лотов
								ok = PrintBillImages(pPack, env.PrnFlags);
							else
								ok = PPAlddPrint(r_rs.RptID, PPFilt(pPack), &env);
						}
						r_rs.NumCopies -= num_copies;
					}
					THROW(ok);
				}
			}
			if(interactive < 0) {
				if(ppAry && !*ppAry) {
					THROW_MEM(*ppAry = new SVector(sizeof(RptSel)));
					(*ppAry)->copy(rpt_ids);
				}
				ASSIGN_PTR(pOutPrnFlags, out_prn_flags);
			}
		}
	}
	CATCHZOKPPERR
	pPack->ProcessFlags = (preserve_process_flags & ~PPBillPacket::pfPrintOnlyUnlimGoods);
	return ok;
}

int STDCALL PrepareBillMultiPrint(PPBillPacket * pFirstPack, SVector ** ppAry, long * pOutPrnFlags)
	{ return Helper_PrintGoodsBill(pFirstPack, ppAry, pOutPrnFlags, -1/*prepare for multi-printing*/); }
int STDCALL MultiPrintGoodsBill(PPBillPacket * pPack, const SVector * pAry, long outPrnFlags)
	{ return Helper_PrintGoodsBill(pPack, const_cast<SVector **>(&pAry), &outPrnFlags, 0/*non-interactive*/); }
int STDCALL PrintGoodsBill(PPBillPacket * pPack)
	{ return Helper_PrintGoodsBill(pPack, 0, 0, 1/*interactive*/); }

int STDCALL PrintCashOrderByGoodsBill(PPBillPacket * pPack, int prnflags)
{
	int    ok = 1;
	const  double amt = BR2(pPack->Rec.Amount);
	if(amt != 0.0) {
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

int STDCALL PrintCashOrder(PPBillPacket * pPack, int pay_rcv, int prnflags)
{
	PPFilt pf(pPack);
	pf.ID  = pay_rcv ? 1 : 2;
	PPReportEnv env;
	env.PrnFlags = prnflags;
	pPack->GetContextEmailAddr(env.EmailAddr);
	uint   rpt_id = pay_rcv ? REPORT_CASHPAYORDER : REPORT_CASHRCVORDER;
	return PPAlddPrint(rpt_id, pf, &env);
}
