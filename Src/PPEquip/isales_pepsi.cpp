// ISALES_PEPSI.CPP
// Copyright (c) A.Sobolev 2016
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

#if 0 // @movedto(ppsupplix.cpp) {

class iSalesPepsi {
public:
	SLAPI  iSalesPepsi();
	SLAPI ~iSalesPepsi();

	int    SLAPI Init(PPID arID);

	int    SLAPI ReceiveGoods();
	int    SLAPI ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult);
	int    SLAPI ReceiveReceipts();
	int    SLAPI ReceiveOrders();

	int    SLAPI SendPrices();
	int    SLAPI SendStocks();
	int    SLAPI SendInvoices();
	int    SLAPI SendDebts();

	int    SLAPI SendStatus(const TSCollection <iSalesTransferStatus> & rList);
private:
	int    SLAPI PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	int    SLAPI Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <iSalesBillPacket> & rList);

	enum {
		stInited    = 0x0001,
		stEpDefined = 0x0002
	};
	PPID   ArID;
    PPSupplAgreement::ExchangeParam Ep;
    DateRange BillExportPeriod;
	long   State;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	SString SvcUrl;
	SString UserName;
	SString Password;
	SString LastMsg;
	SString LogFileName;

	PPObjBill * P_BObj;
	PPObjArticle ArObj;
	PPObjLocation LocObj;
	PPObjGoods GObj;
};

SLAPI iSalesPepsi::iSalesPepsi()
{
	P_BObj = BillObj;
	State = 0;
	ArID = 0;
	P_DestroyFunc = 0;
	BillExportPeriod.SetZero();
	PPGetFilePath(PPPATH_LOG, "isalespepsi.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapPepsi.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib) {
			UserName = "1c_advent it";
			Password = ".392590advP";
			P_DestroyFunc = (void *)P_Lib->GetProcAddr("iSalesDestroyResult");
		}
	}
}

SLAPI iSalesPepsi::~iSalesPepsi()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

int SLAPI iSalesPepsi::Init(PPID arID)
{
	ArID = 0;
	State = 0;
	SvcUrl = 0;
	UserName = 0;
	Password = 0;

	int    ok = 1;
	ArticleTbl::Rec ar_rec;
	PPSupplAgreement agt;
	THROW(ArObj.Search(arID, &ar_rec) > 0);
	ArID = arID;
	if(ArObj.GetSupplAgreement(arID, &agt) > 0) {
        Ep = agt.Ep;
        Ep.GetExtStrData(Ep.extssRemoveAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		State |= stEpDefined;
	}
	State |= stInited;
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL iSalesPepsi::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		((UHTT_DESTROYRESULT)P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

// "1c_advent it"; ".392590advP"

int SLAPI iSalesPepsi::ReceiveGoods()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString lib_path;
	SString temp_buf, url;
	SString out_file_name;
	TSCollection <iSalesGoodsPacket> * p_result = 0;
	ISALESGETGOODSLIST_PROC func = 0;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETGOODSLIST_PROC)P_Lib->GetProcAddr("iSalesGetGoodsList"));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		PPGetFilePath(PPPATH_OUT, "isales-pepsi-products.txt", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesGoodsPacket * p_item = p_result->at(i);
				if(p_item) {
					(line_buf = 0).Cat(p_item->OuterCode).Tab().Cat(p_item->NativeCode).Tab().
						Cat(p_item->TypeOfProduct).Tab().Cat(p_item->UnitCode).Tab().
						Cat(p_item->VatRate).Tab().Cat(p_item->Name).Tab().Cat(p_item->Abbr).Tab().Cat(p_item->UomList.getCount());
					for(uint j = 0; j < p_item->UomList.getCount(); j++) {
						const iSalesUOM * p_uom = p_item->UomList.at(j);
						if(p_uom) {
							line_buf.Tab();
							line_buf.Cat(p_uom->Barcode).Tab().Cat(p_uom->Code).Tab().
								Cat(p_uom->Netto).Tab().Cat(p_uom->Brutto).Tab().
								Cat(p_uom->Width).Tab().Cat(p_uom->Height).Tab().Cat(p_uom->Length);
						}
					}
					line_buf.Transf(CTRANSF_INNER_TO_UTF8);
					f_out.WriteLine(line_buf.CR());
				}
			}
		}
	}
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::ReceiveOrders()
{
	// OrdersTransfer
    int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString src_order_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;

	DateRange period;
	period.Set(encodedate(1, 1, 2016), encodedate(31, 12, 2030));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETORDERLIST_PROC)P_Lib->GetProcAddr("iSalesGetOrderList"));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password, &period);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	if(p_result->getCount()) {
		PPAlbatrosConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillPacket * p_src_pack = p_result->at(i);
				if(p_src_pack) {
					BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					PPBillPacket pack;
					//ArticleTbl::Rec ar_rec;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;

					src_order_code = p_src_pack->Code;
					const  PPID   _src_loc_id = p_src_pack->SrcLocCode.ToLong();
					const  PPID   _src_psn_id = p_src_pack->PayerCode.ToLong();
					const  PPID   _src_dlvrloc_id = p_src_pack->ShipTo.ToLong();
					const  PPID   _src_agent_id = p_src_pack->AgentCode.ToLong();
					PPID   ar_id = 0;
					PPID   loc_id = 0;
					if(_src_loc_id && loc_obj.Fetch(_src_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE)
						loc_id = loc_rec.ID;
					THROW(pack.CreateBlank_WithoutCode(acfg.Hdr.OpID, 0, loc_id, 1));
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d, 0) ? p_src_pack->Dtm.d : getcurdate_();
					STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					if(_src_psn_id && ArObj.P_Tbl->PersonToArticle(_src_psn_id, op_rec.AccSheetID, &ar_id) > 0) {
						pack.SetupObject(ar_id, sob);
                        if(_src_dlvrloc_id && loc_obj.Search(_src_dlvrloc_id, &loc_rec) > 0 &&
							loc_rec.Type == LOCTYP_ADDRESS && (!loc_rec.OwnerID || loc_rec.OwnerID == _src_psn_id)) {
                            PPFreight freight;
                            freight.DlvrAddrID = _src_dlvrloc_id;
                            pack.SetFreight(&freight);
						}
					}
					if(_src_agent_id && ArObj.P_Tbl->PersonToArticle(_src_agent_id, GetAgentAccSheet(), &(ar_id = 0)) > 0) {
						pack.Ext.AgentID = ar_id;
					}
					if(P_BObj->tbl->SearchAnalog(&pack.Rec, &ex_bill_id, &ex_bill_rec) > 0) {
						;
					}
					else {
						for(uint j = 0; j < p_src_pack->Items.getCount(); j++) {
							const iSalesBillItem * p_src_item = p_src_pack->Items.at(j);
							if(p_src_item) {
								PPTransferItem ti;
								THROW(ti.Init(&pack.Rec));
								{
									const PPID _src_goods_id = p_src_item->NativeGoodsCode.ToLong();
									if(_src_goods_id && GObj.Fetch(_src_goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
										ti.GoodsID = goods_rec.ID;
										ti.Quantity_ = fabs(p_src_item->Qtty);
										if(p_src_item->Amounts.getCount()) {
											const iSalesBillAmountEntry * p_src_amt_entry = p_src_item->Amounts.at(0);
											if(p_src_amt_entry) {
												ti.Price = p_src_amt_entry->GrossPrice;
											}
										}
										THROW(pack.LoadTItem(&ti, 0, 0));
									}
								}
							}
						}
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem(0);
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcOrder;
							(p_new_status->Ident = 0).CatChar('O').Cat(src_order_code);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult((void **)&p_result);
    CATCHZOK
    return ok;
}

int SLAPI iSalesPepsi::ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult)
{
	rResult.freeAll();

	int    ok = -1;
	PPSoapClientSession sess;
	SString lib_path;
	SString temp_buf, url;
	SString out_file_name;
	TSCollection <iSalesRoutePacket> * p_result = 0;
	ISALESGETROUTELIST_PROC func = 0;
	DateRange period;
	period.Set(encodedate(1, 1, 2016), encodedate(31, 12, 2030));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETROUTELIST_PROC)P_Lib->GetProcAddr("iSalesGetRouteList"));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password, &period);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		THROW_SL(TSCollection_Copy(rResult, *p_result));
	}
	{
		PPGetFilePath(PPPATH_OUT, "isales-pepsi-routs.txt", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesRoutePacket * p_item = p_result->at(i);
				if(p_item) {
					(line_buf = 0).Cat(p_item->Ident).Tab().Cat(p_item->TypeOfRoute).Tab().
						Cat(p_item->NativeAgentCode).Tab().Cat(p_item->Valid).Tab().Cat(p_item->ErrMsg).Tab().
						Cat(p_item->VisitList.getCount());
					for(uint j = 0; j < p_item->VisitList.getCount(); j++) {
                        const iSalesVisit * p_visit = p_item->VisitList.at(j);
                        if(p_visit) {
							line_buf.Tab();
							line_buf.Cat(p_visit->Ident).Tab().Cat(p_visit->OuterClientCode).Tab().Cat(p_visit->InnerClientCode).Tab().
								Cat(p_visit->InitDate, DATF_DMY|DATF_CENTURY).Tab().Cat(p_visit->Freq).Tab().
								Cat(p_visit->DayOfWeek).Tab().Cat(p_visit->Order).Tab().Cat(p_visit->Valid);
                        }
					}
					line_buf.Transf(CTRANSF_INNER_TO_UTF8);
					f_out.WriteLine(line_buf.CR());
				}
			}
		}
	}
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendStatus(const TSCollection <iSalesTransferStatus> & rList)
{
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	if(rList.getCount()) {
		int    result = 0;
		ISALESPUTTRANSFERSTATUS_PROC func = 0;
		THROW_SL(func = (ISALESPUTTRANSFERSTATUS_PROC)P_Lib->GetProcAddr("iSalesPutTransferStatus"));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &rList);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		DestroyResult((void **)&p_result);
    }
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendPrices()
{
	// CustomerPricesTransfer
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	//TSCollection <iSalesStockCountingWhPacket> outp_packet;
	StringSet ss_outer_cli_code;
	{
		TSCollection <iSalesRoutePacket> routs;
		THROW(ReceiveRouts(routs));
		for(uint i = 0; i < routs.getCount(); i++) {
			const iSalesRoutePacket * p_item = routs.at(i);
			if(p_item) {
				for(uint j = 0; j < p_item->VisitList.getCount(); j++) {
					const iSalesVisit * p_visit = p_item->VisitList.at(j);
					if(p_visit && p_visit->OuterClientCode.NotEmpty()) {
						ss_outer_cli_code.add(p_visit->InnerClientCode);
					}
				}
			}
		}
		ss_outer_cli_code.sortAndUndup();
	}
	if(ss_outer_cli_code.getCount()) {
		int    result = 0;
		int    do_send = 0;
		TSCollection <iSalesPriceListPacket> pl_list;
		{
			iSalesPriceListPacket * p_new_pl = pl_list.CreateNewItem(0);
			THROW_SL(p_new_pl);
			p_new_pl->PriceListID = 1;
			p_new_pl->OuterCliCodeList = ss_outer_cli_code;
			GoodsFilt gfilt;
			if(Ep.GoodsGrpID)
				gfilt.GrpIDList.Add(Ep.GoodsGrpID);
            gfilt.Flags |= GoodsFilt::fShowArCode;
            gfilt.CodeArID = ArID;
            Goods2Tbl::Rec goods_rec;
			for(GoodsIterator gi(&gfilt, 0, 0); gi.Next(&goods_rec) > 0;) {
				double price = 0.0;
				int32  ar_code_pack = 0;
				if(GObj.tbl->GetArCode(ArID, goods_rec.ID, temp_buf, &ar_code_pack) > 0 && temp_buf.NotEmptyS()) {
					if(Ep.PriceQuotID) {
						QuotIdent qi(0, Ep.PriceQuotID, 0, 0);
						GObj.GetQuotExt(goods_rec.ID, qi, 0.0, 0.0, &price, 0);
					}
					if(price == 0.0) {
						//GetLastLot(PPID goodsID, PPID locID, LDATE date, ReceiptTbl::Rec * pLotRec);
						ReceiptTbl::Rec lot_rec;
						if(BillObj->trfr->Rcpt.GetLastLot(goods_rec.ID, 0, MAXDATE, &lot_rec) > 0) {
							price = lot_rec.Price;
						}
					}
					if(price > 0.0) {
						iSalesPriceItem * p_new_pl_item = p_new_pl->Prices.CreateNewItem(0);
						THROW_SL(p_new_pl_item);
                        p_new_pl_item->OuterCode = temp_buf;
                        p_new_pl_item->Price = price;
                        do_send = 1;
					}
				}
			}
		}
		if(do_send) {
			ISALESPUTPRICES_PROC func = 0;
			THROW_SL(func = (ISALESPUTPRICES_PROC)P_Lib->GetProcAddr("iSalesPutPrices"));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &pl_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			DestroyResult((void **)&p_result);
		}
    }
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendStocks()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	PPViewGoodsRest gr_view;
	GoodsRestFilt gr_filt;
	GoodsRestViewItem gr_item;
	TSCollection <iSalesStockCountingWhPacket> outp_packet;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);

    gr_filt.Date = ZERODATE;
    gr_filt.GoodsGrpID = Ep.GoodsGrpID;
    gr_filt.Flags |= GoodsRestFilt::fEachLocation;
    THROW(gr_view.Init_(&gr_filt));
    for(gr_view.InitIteration(PPViewGoodsRest::OrdByDefault); gr_view.NextIteration(&gr_item) > 0;) {
        const PPID loc_id = gr_item.LocID;
        uint   _pos = 0;
		iSalesStockCountingWhPacket * p_loc_item = 0;
		if(GObj.tbl->GetArCode(ArID, gr_item.GoodsID, temp_buf = 0, 0) > 0) {
			if(outp_packet.lsearch(&loc_id, &_pos, CMPF_LONG)) {
				p_loc_item = outp_packet.at(_pos);
			}
			else {
				LocationTbl::Rec loc_rec;
				if(LocObj.Search(loc_id, &loc_rec) > 0) {
					THROW_SL(p_loc_item = outp_packet.CreateNewItem(0));
					p_loc_item->WhID = loc_id;
					p_loc_item->WhCode = loc_rec.Code;
				}
			}
			if(p_loc_item) {
				/*
					struct iSalesStockCountingItem {
						SString OuterCode;
						int   Type; // Тип остатков: 0 - годные, 1 - брак, 2 - резерв
						int   UnitCode; // 0 - штука, 1 - коробка, 2 - условная коробка
						double Qtty; // Количество на остатке
					};
				*/
				iSalesStockCountingItem * p_new_item = p_loc_item->Items.CreateNewItem(0); // new iSalesStockCountingItem;
				THROW_SL(p_new_item);
				p_new_item->OuterCode = temp_buf;
				p_new_item->Qtty = gr_item.Rest;
				p_new_item->Type = 0;
				p_new_item->UnitCode = 1004/*0*/;
			}
		}
    }
    {
		SString * p_result = 0;
		ISALESPUTSTOCKCOUNTING_PROC func = 0;
		THROW_SL(func = (ISALESPUTSTOCKCOUNTING_PROC)P_Lib->GetProcAddr("iSalesPutStockCounting"));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
        DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		DestroyResult((void **)&p_result);
    }
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <iSalesBillPacket> & rList)
{
	int    ok = -1;
	if(opID) {
		SString temp_buf;
		SString cli_code; // Буфер для кода клиента
		SString own_code; // Наш код в системе поставщика
		SysJournalTbl::Rec sj_rec;
		PPViewBill b_view;
		BillFilt b_filt;
		BillViewItem view_item;
		PPBillPacket pack;

		Ep.GetExtStrData(Ep.extssClientCode, own_code);
		SysJournal * p_sj = DS.GetTLA().P_SysJ;

		b_filt.OpID = opID;
		b_filt.Period = BillExportPeriod;
		THROW(b_view.Init_(&b_filt));
		for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
			if(BillObj->ExtractPacket(view_item.ID, &pack, 0) > 0) {
				StrAssocArray ti_pos_list;
                for(uint i = 0; i < pack.GetTCount(); i++) {
					const PPTransferItem & r_ti = pack.ConstTI(i);
                    if(GObj.BelongToGroup(r_ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.tbl->GetArCode(ArID, r_ti.GoodsID, temp_buf = 0, 0) > 0) {
						ti_pos_list.Add((long)i, temp_buf, 0);
                    }
                }
				if(ti_pos_list.getCount()) {
					/* iSalesBillPacket
						* SString iSalesId;
						* int    DocType; // 0 - отгрузка, 1 - документ реализации, 2 - счет на оплату, 3 - возврат от покупателя,
							// 4 - возврат обратной реализации, 5 - возврат по акту, 6 - накладная доставки/приход, 7 - документ поступления,
							// 8 - оплата дистибьютору, 9 - взаимозачет, 10 - движение долга между клиентами, 11 - списание/начисление долга,
							// 12 - аванс от клиента, 13 - заказ от клиента, 14 - выплата (дистрибьютора контрагенту)
						* int    ExtDocType; // (должно быть пусто)
						* int    Status; // 0 - проведен, 1 - отменен
						* SString Code;    // Номер документа
						* SString ExtCode; // Номер внешнего документа
						* LDATETIME Dtm;    // Дата/время документа (передается строкой dd.mm.yyyy hh:mm:ss)
						* LDATETIME IncDtm; // Дата входящего документа (должно быть пусто)
						* LDATETIME ExtDtm; // Дата/время внешнего документа (передается строкой dd.mm.yyyy hh:mm:ss)
						* LDATETIME CreationDtm; // Дата/время создания документа (передается строкой dd.mm.yyyy hh:mm:ss)
						* LDATETIME LastUpdDtm;  // Дата/время последнего изменения документа (передается строкой dd.mm.yyyy hh:mm:ss)
						* LDATE   DueDate;  // Срок оплаты
						* SString ShipFrom; // Код грузоотправителя (iSales код дистрибьютора при отгрузке на клиента, Native-код клиента при возврате)
						* SString ShipTo;   // Код грузополучателя (Native-код клиента (точки доставки) при документе реализации, iSales код дистрибьютора при возврате)
						* SString SellerCode; // Код продавца (iSales код дистрибьютора при отгрузке на клиента, Native-код клиента при возврате)
						* SString PayerCode;  // Код плательщика (Native-код клиента при отгрузке на клиента, iSales код дистрибьютора при возврате)
						* SString Memo;      // Примечание
						* SString SrcLocCode;  // Код склада-отправителя
						* SString DestLocCode; // Код склада-получателя
						* SString AgentCode;   // Native-код агента
						SString AuthId;      // Ключевой пользователь (Native-код), кто зарегистрировал документ в iSales
						SString EditId;      // Ключевой пользователь (Native-код), кто изменил документ в iSales
						TSCollection <iSalesBillItem> Items;
						TSCollection <iSalesBillAmountEntry> Amounts;
					*/
					cli_code = 0;
					if(pack.P_Freight && pack.P_Freight->DlvrAddrID)
						cli_code.Cat(pack.P_Freight->DlvrAddrID);
					else {
						PPID   psn_id = ObjectToPerson(pack.Rec.Object, 0);
						cli_code.Cat(psn_id);
					}
                    iSalesBillPacket * p_new_pack = rList.CreateNewItem(0);
					THROW_SL(p_new_pack);
					{
						iSalesBillAmountEntry * p_amt_entry = 0;
						THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem(0));
						p_amt_entry->SetType = 0;
						THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem(0));
						p_amt_entry->SetType = 1;
						THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem(0));
						p_amt_entry->SetType = 2;
					}
                    p_new_pack->DocType = outerDocType;
                    p_new_pack->ExtDocType = 0;
                    p_new_pack->ExtCode = 0;
                    p_new_pack->ExtDtm.SetZero();
                    BillCore::GetCode(p_new_pack->Code = pack.Rec.Code);
					p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
                    p_new_pack->Dtm.Set(pack.Rec.Dt, ZEROTIME);
					(p_new_pack->iSalesId = 0).Cat(p_new_pack->Code).Space().Cat(pack.Rec.Dt, DATF_GERMAN|DATF_CENTURY).Space().Cat(outerDocType);
                    p_new_pack->IncDtm.SetZero();
                    pack.Pays.GetLast(&p_new_pack->DueDate, 0, 0);
                    p_new_pack->Status = 0;
                    (p_new_pack->Memo = pack.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8);
                    if(outerDocType == 3) { // Возврат
						p_new_pack->SellerCode = cli_code;
						p_new_pack->ShipFrom = cli_code;
						p_new_pack->PayerCode = own_code;
						p_new_pack->ShipTo = own_code;
						p_new_pack->DestLocCode.Cat(pack.Rec.LocID);
                    }
					else {
						p_new_pack->SellerCode = own_code;
						p_new_pack->ShipFrom = own_code;
						p_new_pack->PayerCode = cli_code;
						p_new_pack->ShipTo = cli_code;
						p_new_pack->SrcLocCode.Cat(pack.Rec.LocID);
					}
					if(pack.Ext.AgentID)
						p_new_pack->AgentCode.Cat(pack.Ext.AgentID);
					if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, pack.Rec.ID, &sj_rec) > 0)
						p_new_pack->CreationDtm.Set(sj_rec.Dt, sj_rec.Tm);
					else
						p_new_pack->CreationDtm.SetZero();
					{
						int    is_creation = 0;
						LDATETIME moment;
						if(p_sj && p_sj->GetLastObjModifEvent(PPOBJ_BILL, pack.Rec.ID, &moment, &is_creation, &sj_rec) > 0 && !is_creation)
							p_new_pack->LastUpdDtm = moment;
						else
							p_new_pack->LastUpdDtm.SetZero();
					}
					for(uint j = 0; j < ti_pos_list.getCount(); j++) {
						StrAssocArray::Item ti_pos_item = ti_pos_list.at_WithoutParent(j);
						const PPTransferItem & r_ti = pack.ConstTI(ti_pos_item.Id);
						Goods2Tbl::Rec goods_rec;
						if(GObj.Fetch(r_ti.GoodsID, &goods_rec) > 0) {
							iSalesBillItem * p_new_item = p_new_pack->Items.CreateNewItem(0);
							THROW_SL(p_new_item);
							/* iSalesBillItem
								int    LineN; // Номер строки
								SString OuterGoodsCode;  // Outer-код товара
								SString NativeGoodsCode; // Native-код товара
								SString Country; // Страна происхождения
								SString CLB;     // Номер ГТД
								int    UnitCode;
								double Qtty;
								SString Memo; // DESC
								TSCollection <iSalesBillAmountEntry> Amounts;
							*/
							const double qtty = fabs(r_ti.Quantity_);
							p_new_item->LineN = r_ti.RByBill;
							p_new_item->OuterGoodsCode = ti_pos_item.Txt;
							(p_new_item->NativeGoodsCode = 0).Cat(r_ti.GoodsID);
							p_new_item->Country = 0;
							p_new_item->CLB = 0;
							p_new_item->UnitCode = 1004;
							p_new_item->Qtty = qtty;
							p_new_item->Memo = 0;
							{
								/* iSalesBillAmountEntry
									int    SetType;       // 0 - Значения, рассчитанные на основе базовых цен, 1 - Значения, которые д.б. в реальности, 2 - Значения, которые будут печататься в документах
									double NetPrice;
									double GrossPrice;
									double NetSum;
									double DiscAmount;
									double DiscNetSum;
									double VatSum;
									double GrossSum;
									double DiscGrossSum;
								*/
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem(0);
								THROW_SL(p_amt_entry);
								p_amt_entry->SetType = 0;
								double vat_sum_in_price = 0.0;
								double full_price = r_ti.NetPrice();
								GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, 1.0, full_price, &vat_sum_in_price, 0, 0);
								p_amt_entry->NetPrice = full_price - vat_sum_in_price;
								p_amt_entry->GrossPrice = full_price;
								p_amt_entry->NetSum = (full_price - vat_sum_in_price) * qtty;
								p_amt_entry->DiscAmount = 0.0;
								p_amt_entry->DiscNetSum = (full_price - vat_sum_in_price) * qtty;
								p_amt_entry->VatSum = vat_sum_in_price * qtty;
								p_amt_entry->GrossSum = full_price * qtty;
								p_amt_entry->DiscGrossSum = 0.0;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(0);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
							{
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem(0);
								THROW_SL(p_amt_entry);
								*p_amt_entry = *p_new_item->Amounts.at(0);
								p_amt_entry->SetType = 1;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(1);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
							{
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem(0);
								THROW_SL(p_amt_entry);
								*p_amt_entry = *p_new_item->Amounts.at(0);
								p_amt_entry->SetType = 2;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(2);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
						}
					}
				}
			}
		}
	}
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendInvoices()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	TSCollection <iSalesBillPacket> outp_packet;
	BillExportPeriod.Set(encodedate(1, 6, 2016), encodedate(30, 6, 2016));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
    THROW(Helper_MakeBillList(Ep.ExpendOp, 1, outp_packet));
    {
		SString * p_result = 0;
		ISALESPUTBILLS_PROC func = 0;
		THROW_SL(func = (ISALESPUTBILLS_PROC)P_Lib->GetProcAddr("iSalesPutBills"));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
        DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		DestroyResult((void **)&p_result);
    }
    CATCHZOK
	return ok;
}

int SLAPI Test_iSalesPepsi()
{
	int    ok = 1;
	SupplInterchangeFilt filt;
	PrcssrSupplInterchange prc;
	THROW(prc.EditParam(&filt));
	/*
	iSalesPepsi cli;
	TSCollection <iSalesRoutePacket> routs;
	THROW(cli.Init(13743)); // ООО "ПепсиКо Холдингс"
	THROW(cli.ReceiveOrders());
	//THROW(cli.SendInvoices());
	//THROW(cli.SendStocks());
	//THROW(cli.SendPrices());
	//THROW(cli.ReceiveRouts(routs));
	//THROW(cli.ReceiveGoods());
	*/
	CATCHZOKPPERR
	return ok;
}

#endif // } 0 @movedto(ppsupplix.cpp)