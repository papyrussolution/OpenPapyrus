// prc_billautocreate.cpp
// Copyright (c) A.Sobolev 2017, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
int PrcssrBillAutoCreate::CreateDraftByCSessRule(const CSessCrDraftParam * pParam)
{
	int    ok = -1;
	if(pParam && (pParam->RuleGrpID || pParam->RuleID) && !pParam->Period.IsZero()) {
		PPViewCSess cs_view;
		CSessFilt cs_filt;
		cs_filt.Period = pParam->Period;
		cs_filt.NodeList_ = pParam->NodeList;
		SETFLAG(cs_filt.Flags, CSessFilt::fOnlySuperSess, pParam->Flags & CSessCrDraftParam::fSuperSessOnly);
		THROW(cs_view.Init_(&cs_filt));
		THROW(cs_view.CreateDrafts(pParam->RuleGrpID, pParam->RuleID, 0));
	}
	CATCHZOK
	return ok;
}
//
//
//
struct _OrdArEntry {
	PPID   ArID;
	PPID   GoodsGrpID;       // ->Goods2.ID
	PPID   LocID;            // ->Location.ID
	PPID   MngrID;           // ->Person.ID Менеджер, закрепленный за этой группой закупки
	int16  OrdPrdDays;       //
	int16  DuePrdDays;       //
	DateRepeating Dr;        // Периодичность заказа
};

static SString & MakeOrderAutocreationTag(const _OrdArEntry & rEntry, LDATE dt, SString & rBuf)
{
    return rBuf.Z().Cat("PCZ").Dot().Cat(rEntry.ArID).Dot().Cat(rEntry.GoodsGrpID).Dot().
		Cat(rEntry.LocID).Dot().Cat(rEntry.MngrID).Dot().Cat(dt, MKSFMT(0, DATF_YMD|DATF_CENTURY|DATF_NODIV));
}

// static
int PrcssrBillAutoCreate::CreateDraftBySupplOrders(const SStatFilt * pFilt)
{
	_OrdArEntry ord_entry;
	int    ok = 1, r = 0;
	SString temp_buf, fmt_buf, msg_buf;
	SArray ar_list(sizeof(_OrdArEntry));
	PPLogger log;
	PPPredictConfig pr_cfg;
	PPBillPacket * p_pack = 0;
	SStatFilt  * p_filt = 0;
	PPObjArticle ar_obj;
	PPViewSStat view;
	PPIDArray list;
	PrcssrPrediction::GetPredictCfg(&pr_cfg);
	LDATE  cur_dt = getcurdate_();
	THROW_MEM(p_filt = static_cast<SStatFilt *>(view.CreateFilt(reinterpret_cast<void *>(1))));
	if(pFilt)
		r = p_filt->Copy(pFilt, 0);
	else
		r = view.EditBaseFilt(p_filt);
	PPWaitStart();
	if(r > 0) {
		PPObjBill * p_bobj = BillObj;
		if(p_filt->SupplID)
			list.add(p_filt->SupplID);
		else
			THROW(ar_obj.P_Tbl->GetListBySheet(GetSupplAccSheet(), &list, 0));
		for(uint i = 0; i < list.getCount(); i++) {
			PPID   article_id = list.at(i);
			PPSupplAgreement suppl_agt;
			if(ar_obj.GetSupplAgreement(article_id, &suppl_agt, 1) > 0 && (suppl_agt.Flags & AGTF_AUTOORDER)) {
				for(uint j = 0; j < suppl_agt.OrderParamList.getCount(); j++) {
                    const PPSupplAgreement::OrderParamEntry & r_entry = suppl_agt.OrderParamList.at(j);
                    if(r_entry.OrdPrdDays && r_entry.Dr.Prd) {
						PPID   loc_id = 0;
						if(r_entry.LocID == 0 && p_filt->LocList.GetSingle())
							loc_id = p_filt->LocList.GetSingle();
						else if(p_filt->LocList.CheckID(r_entry.LocID))
							loc_id = r_entry.LocID;
						if(loc_id) {
							MEMSZERO(ord_entry);
							ord_entry.ArID = article_id;
							ord_entry.GoodsGrpID = r_entry.GoodsGrpID;
							ord_entry.LocID = loc_id;
							ord_entry.MngrID = r_entry.MngrID;
							ord_entry.OrdPrdDays = r_entry.OrdPrdDays;
							ord_entry.DuePrdDays = r_entry.Fb.DuePrdDays;
							ord_entry.Dr = r_entry.Dr;
							THROW_SL(ar_list.insert(&ord_entry));
						}
                    }
					else {
						GetArticleName(article_id, temp_buf.Z());
						msg_buf.Printf(PPLoadStringS(PPMSG_ERROR, PPERR_AUTOORDINVPARAM, fmt_buf), article_id, temp_buf.cptr());
						log.Log(msg_buf);
					}
				}
			}
		}
		{
			TSCollection <PPBillPacket> bill_pack_list;
			{
				const LDATE base_date = encodedate(1, 1, cur_dt.year());
				for(uint i = 0; i < ar_list.getCount(); i++) {
					ord_entry = *static_cast<const _OrdArEntry *>(ar_list.at(i));
					LDATE  doc_dt = ZERODATE;
					DateRepIterator dr_iter(ord_entry.Dr, base_date);
					do {
						doc_dt = dr_iter.Next();
					} while(doc_dt != ZERODATE && doc_dt < cur_dt);
					if(doc_dt == cur_dt) {
						p_filt->OrdTerm = ord_entry.OrdPrdDays;
						p_filt->SetupCfgOptions(pr_cfg);
						p_filt->SupplID = ord_entry.ArID;
						p_filt->GoodsGrpID = ord_entry.GoodsGrpID;
						if(view.Init_(p_filt) > 0) {
							int    r;
							THROW_MEM(p_pack = new PPBillPacket);
							r = view.CreatePurchaseBill(doc_dt, 1, p_pack, 1);
							if(ord_entry.DuePrdDays >= 0)
								p_pack->Rec.DueDate = plusdate(p_pack->Rec.Dt, ord_entry.DuePrdDays);
							if(r > 0) {
								int    skip = 0;
								MakeOrderAutocreationTag(ord_entry, doc_dt, temp_buf);
								PPIDArray ex_bill_list;
                                PPRef->Ot.SearchObjectsByStr(PPOBJ_BILL, PPTAG_BILL_AUTOCREATION, temp_buf, &ex_bill_list);
                                for(uint j = 0; !skip && j < ex_bill_list.getCount(); j++) {
                                	const PPID ex_bill_id = ex_bill_list.get(j);
                                	BillTbl::Rec ex_bill_rec;
									if(p_bobj->Search(ex_bill_id, &ex_bill_rec) > 0) {
										// PPTXT_SUPPLORDEXISTS         "Документ заказа поставщику '%s' был создан ранее"
										PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
										msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_SUPPLORDEXISTS, fmt_buf), temp_buf.cptr());
										log.Log(msg_buf);
										skip = 1;
									}
                                }
                                if(!skip) {
									ObjTagItem tag;
									if(tag.SetStr(PPTAG_BILL_AUTOCREATION, MakeOrderAutocreationTag(ord_entry, doc_dt, temp_buf))) {
										p_pack->BTagL.PutItem(PPTAG_BILL_AUTOCREATION, &tag);
									}
									else {
										log.LogLastError();
									}
									THROW_SL(bill_pack_list.insert(p_pack));
									p_pack = 0;
                                }
							}
							else if(!r)
								log.LogLastError();
						}
					}
					PPWaitPercent(i + 1, ar_list.getCount());
				}
			}
			if(bill_pack_list.getCount()) {
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < bill_pack_list.getCount(); i++) {
					PPBillPacket * p_item = bill_pack_list.at(i);
					if(p_item) {
						if(p_bobj->TurnPacket(p_item, 0)) {
							PPObjBill::MakeCodeString(&p_item->Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
							msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_SUPPLORDERCREATED, fmt_buf), temp_buf.cptr());
							log.Log(msg_buf);
						}
						else
							log.LogLastError();
					}
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOKPPERR
	ZDELETE(p_filt);
	ZDELETE(p_pack);
	PPWaitStop();
	return ok;
}
//
//
struct TaBonusSupplItem {
    PPID   SupplID;
    PPID   GoodsID;
    double Qtty;
    double Cost;
    double Price;
    double Discount;
};

static SString & MakeTaAutocreationTag(PPID arID, PPID locID, LDATE dt, SString & rBuf)
{
    return rBuf.Z().Cat("TAB").Dot().Cat(arID).Dot().Cat(locID).Dot().Cat(dt, MKSFMT(0, DATF_YMD|DATF_CENTURY|DATF_NODIV));
}

int PrcssrBillAutoCreate::CreateDraftByTrfrAnlz()
{
	//
	// Очень специализированная функция, формирующая драфт-документы по бонусам, предоставленным
	// покупателям. Контрагентами таких документов являются поставщики.
	// -- Цены в драфт-документах:
	//      -- номинальная цена реализации равна полной цене отгрузки
	//      -- скидка равна полной цене заказа. Таким образом, полная цена реализации в драфт-документе будет равна бонусной скидке.
	//      -- цена поступления равна просуммированной величине в ценах поступления из документов отгрузки,
	//         поделенной на суммарное отгруженное количество.
	// Для будущих применений значение speciality будет отличаться от 1
	//
	int    speciality = 1;

    int    ok = -1;
    PPLogger logger;
    SString temp_buf;
    SString bill_tag_buf;
    SString msg_buf, fmt_buf;
	PPIDArray ex_bill_list;
    if(P.A == PPBillAutoCreateParam::aDraftByTrfrAnlz) {
        if(P.P_TaF && P.OpID) {
			if(!P.P_TaF->Grp || P.P_TaF->HasGoodsGrouping()) {
				if(speciality == 1) {
					//if(P.P_TaF->SupplID) {
					if(!P.P_TaF->Grp && !P.P_TaF->Sgg && !P.P_TaF->Sgd && !P.P_TaF->Sgp) {
						PPObjBill * p_bobj = BillObj;
						PPObjGoods goods_obj;
						const   PPID loc_id = LConfig.Location;
						LDATE   last_rep_date = ZERODATE; // Дата самой поздней операции в отчете
						PPViewTrfrAnlz view;
						TrfrAnlzViewItem item;
						TSArray <TaBonusSupplItem> result_list;
						THROW(view.Init_(P.P_TaF));
						for(view.InitIteration(view.OrdByDefault); view.NextIteration(&item) > 0;) {
							if(fabs(item.ExtValue[0]) >= 1.0) {
								ReceiptTbl::Rec lot_rec;
								if(item.LotID && p_bobj->trfr->Rcpt.Search(item.LotID, &lot_rec) > 0 && lot_rec.SupplID) {
									SETMAX(last_rep_date, item.Dt);
									TaBonusSupplItem new_result;
									MEMSZERO(new_result);
									new_result.GoodsID = item.GoodsID;
									new_result.SupplID = lot_rec.SupplID;
									new_result.Qtty = item.Qtty;
									new_result.Cost = item.Cost;
									new_result.Price = item.Price;
                                    new_result.Discount = item.Price - item.ExtValue[0];
                                    uint   idx = 0;
                                    if(result_list.lsearch(&new_result, &idx, PTR_CMPFUNC(_2long))) {
										TaBonusSupplItem & r_ex_result = result_list.at(idx);
										assert(r_ex_result.GoodsID == new_result.GoodsID && r_ex_result.SupplID == new_result.SupplID);
										r_ex_result.Qtty += new_result.Qtty;
										r_ex_result.Cost += new_result.Cost;
										r_ex_result.Price += new_result.Price;
										r_ex_result.Discount += new_result.Discount;
                                    }
                                    else {
										THROW_SL(result_list.insert(&new_result));
                                    }
								}
							}
						}
						if(result_list.getCount() && checkdate(last_rep_date)) {
							uint   i;
							PPIDArray suppl_list;
							result_list.sort(PTR_CMPFUNC(_2long));
							for(i = 0; i < result_list.getCount(); i++) {
								THROW_SL(suppl_list.add(result_list.at(i).SupplID));
							}
							suppl_list.sortAndUndup();
							for(i = 0; i < suppl_list.getCount(); i++) {
								const PPID suppl_id = suppl_list.get(i);
								PPBillPacket::SetupObjectBlock sob;
                                PPBillPacket bp;
								int    skip = 0;
								MakeTaAutocreationTag(suppl_id, loc_id, last_rep_date, bill_tag_buf);
								ex_bill_list.clear();
                                PPRef->Ot.SearchObjectsByStr(PPOBJ_BILL, PPTAG_BILL_AUTOCREATION, bill_tag_buf, &ex_bill_list);
                                for(uint j = 0; !skip && j < ex_bill_list.getCount(); j++) {
                                	const PPID ex_bill_id = ex_bill_list.get(j);
                                	BillTbl::Rec ex_bill_rec;
									if(p_bobj->Search(ex_bill_id, &ex_bill_rec) > 0) {
										PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
										msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_TADRAFTEXISTS, fmt_buf), temp_buf.cptr());
										logger.Log(msg_buf);
										skip = 1;
									}
                                }
                                if(!skip) {
									ObjTagItem tag;
									PPTransaction tra(1);
									THROW(tra);
									THROW(bp.CreateBlank2(P.OpID, last_rep_date, loc_id, 0));
									if(tag.SetStr(PPTAG_BILL_AUTOCREATION, bill_tag_buf)) {
										bp.BTagL.PutItem(PPTAG_BILL_AUTOCREATION, &tag);
									}
									else {
										logger.LogLastError();
									}
									if(bp.SetupObject(suppl_id, sob)) {
										for(uint j = 0; j < result_list.getCount(); j++) {
											const TaBonusSupplItem & r_result_item = result_list.at(j);
											if(r_result_item.SupplID == suppl_id) {
												PPTransferItem ti(&bp.Rec, TISIGN_UNDEF);
												if(ti.SetupGoods(r_result_item.GoodsID, 0) > 0) {
													const double qtty = R6(fabs(r_result_item.Qtty));
													ti.GoodsID = r_result_item.GoodsID;
													ti.Cost = R5(r_result_item.Cost / qtty);
													ti.Price = R5(r_result_item.Price / qtty);
													ti.Discount = R5(r_result_item.Discount / qtty);
													ti.Quantity_ = qtty;
													THROW(bp.LoadTItem(&ti, 0, 0));
												}
											}
										}
										if(p_bobj->__TurnPacket(&bp, 0, 1, 0)) {
											PPObjBill::MakeCodeString(&bp.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
											msg_buf.Printf(PPLoadStringS(PPSTR_TEXT, PPTXT_TADRAFTCREATED, fmt_buf), temp_buf.cptr());
											logger.Log(msg_buf);
										}
										else
											logger.LogLastError();

									}
									THROW(tra.Commit());
                                }
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
//
//
//
int PPBillAutoCreateParam::InitInstance()
{
	P_TaF = 0;
    P_SsF = 0;
    P_CcF = 0;
    P_CsF = 0;
	SetFlatChunk(offsetof(PPBillAutoCreateParam, ReserveStart),
		offsetof(PPBillAutoCreateParam, Reserve)-offsetof(PPBillAutoCreateParam, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_TRFRANLZ, offsetof(PPBillAutoCreateParam, P_TaF));
	SetBranchBaseFiltPtr(PPFILT_SSTAT,    offsetof(PPBillAutoCreateParam, P_SsF));
	SetBranchBaseFiltPtr(PPFILT_CCHECK,   offsetof(PPBillAutoCreateParam, P_CcF));
	SetBranchBaseFiltPtr(PPFILT_CSESS,    offsetof(PPBillAutoCreateParam, P_CsF));
	return Init(1, 0);
}

PPBillAutoCreateParam::PPBillAutoCreateParam() : PPBaseFilt(PPFILT_BILLAUTOCREATEPARAM, 0, 0)
{
	InitInstance();
}

PPBillAutoCreateParam::PPBillAutoCreateParam(const PPBillAutoCreateParam & rS) : PPBaseFilt(PPFILT_BILLAUTOCREATEPARAM, 0, 0)
{
	InitInstance();
	Copy(&rS, 1);
}

PPBillAutoCreateParam & FASTCALL PPBillAutoCreateParam::operator = (const PPBillAutoCreateParam & rS)
{
	Copy(&rS, 1);
	return *this;
}

PPBaseFilt * FASTCALL PPBillAutoCreateParam::GetInnerFilt(int a) const
{
	if(a == -1)
		a = A;
	switch(a) {
		case aDraftByTrfrAnlz: return P_TaF;
		case aDraftBySuspCc:   return P_CcF;
		case aDraftBySupplOrders: return P_SsF;
		case aDraftByCcRule: return P_CsF;
		default: return 0;
	}
}

PrcssrBillAutoCreate::PrcssrBillAutoCreate() : P_TaV(0), P_SsV(0), P_CcV(0), P_CsV(0)
{
}

PrcssrBillAutoCreate::~PrcssrBillAutoCreate()
{
    ZDELETE(P_TaV);
    ZDELETE(P_SsV);
    ZDELETE(P_CcV);
    ZDELETE(P_CsV);
}

int PrcssrBillAutoCreate::InitParam(PPBillAutoCreateParam * pParam)
{
	int    ok = 1;
	return ok;
}

int PrcssrBillAutoCreate::EditParam(PPBillAutoCreateParam * pParam)
{
	class BillAutoCreateParamDialog : public TDialog {
	public:
		BillAutoCreateParamDialog() : TDialog(DLG_PRCBAUTOC)
		{
		}
		int    setDTS(const PPBillAutoCreateParam * pData)
		{
			int    ok = 1;
            RVALUEPTR(Data, pData);
            AddClusterAssoc(CTL_PRCBAUTOC_WHAT, 0, PPBillAutoCreateParam::aDraftByTrfrAnlz);
            AddClusterAssoc(CTL_PRCBAUTOC_WHAT, 1, PPBillAutoCreateParam::aDraftBySuspCc);
            AddClusterAssoc(CTL_PRCBAUTOC_WHAT, 2, PPBillAutoCreateParam::aDraftBySupplOrders);
            AddClusterAssoc(CTL_PRCBAUTOC_WHAT, 3, PPBillAutoCreateParam::aDraftByCcRule);
            SetClusterData(CTL_PRCBAUTOC_WHAT, Data.A);
            SetupOprKindCombo(this, CTLSEL_PRCBAUTOC_OP, Data.OpID, 0, 0, 0);
			SetupPPObjCombo(this, CTLSEL_PRCBAUTOC_RULEGRP, PPOBJ_DFCREATERULE, Data.RuleGrpID, 0, reinterpret_cast<void *>(PPDFCRRULE_ONLYGROUPS));
			SetupPPObjCombo(this, CTLSEL_PRCBAUTOC_RULE, PPOBJ_DFCREATERULE, Data.RuleID, 0, reinterpret_cast<void *>(PPDFCRRULE_ONLYRULES));
            return ok;
		}
		int    getDTS(PPBillAutoCreateParam * pData)
		{
			int    ok = 1;
			GetClusterData(CTL_PRCBAUTOC_WHAT, &Data.A);
			getCtrlData(CTLSEL_PRCBAUTOC_OP, &Data.OpID);
			Data.RuleGrpID = getCtrlLong(CTLSEL_PRCBAUTOC_RULEGRP);
			Data.RuleID    = getCtrlLong(CTLSEL_PRCBAUTOC_RULE);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmFilter)) {
				GetClusterData(CTL_PRCBAUTOC_WHAT, &Data.A);
				EditFilt();
			}
			/*
			else if(event.isClusterClk(CTL_PRCBAUTOC_WHAT)) {
				const long preserve_a = Data.A;
				long   new_a = GetClusterData(CTL_PRCBAUTOC_WHAT);
				if(new_a != preserve_a) {
				}
			}
			*/
		}
		int    EditFilt()
		{
			int    ok = -1;
			switch(Data.A) {
				case PPBillAutoCreateParam::aDraftByTrfrAnlz:
					{
						PPViewTrfrAnlz view;
						TrfrAnlzFilt * p_filt = (TrfrAnlzFilt *)view.CreateFilt(PPView::GetDescriptionExtra(view.GetViewId()));
						THROW(p_filt);
						RVALUEPTR(*p_filt, Data.P_TaF);
						if(view.EditBaseFilt(p_filt) > 0) {
							THROW_MEM(SETIFZ(Data.P_TaF, new TrfrAnlzFilt));
							*Data.P_TaF = *p_filt;
							ok = 1;
						}
						ZDELETE(p_filt);
					}
					break;
				case PPBillAutoCreateParam::aDraftBySuspCc:
					{
						PPViewCCheck view;
						CCheckFilt * p_filt = (CCheckFilt *)view.CreateFilt(reinterpret_cast<void *>(1));
						THROW(p_filt);
						RVALUEPTR(*p_filt, Data.P_CcF);
						if(view.EditBaseFilt(p_filt) > 0) {
							THROW_MEM(SETIFZ(Data.P_CcF, new CCheckFilt));
							*Data.P_CcF = *p_filt;
							ok = 1;
						}
						ZDELETE(p_filt);
					}
					break;
				case PPBillAutoCreateParam::aDraftBySupplOrders:
					{
						PPViewSStat view;
						SStatFilt * p_filt = (SStatFilt *)view.CreateFilt(reinterpret_cast<void *>(1));
						THROW(p_filt);
						RVALUEPTR(*p_filt, Data.P_SsF);
						if(view.EditBaseFilt(p_filt) > 0) {
							THROW_MEM(SETIFZ(Data.P_SsF, new SStatFilt));
							*Data.P_SsF = *p_filt;
							ok = 1;
						}
						ZDELETE(p_filt);
					}
					break;
				case PPBillAutoCreateParam::aDraftByCcRule:
					{
						PPViewCSess view;
						CSessFilt * p_filt = (CSessFilt *)view.CreateFilt(reinterpret_cast<void *>(1));
						THROW(p_filt);
						RVALUEPTR(*p_filt, Data.P_CsF);
						if(view.EditBaseFilt(p_filt) > 0) {
							THROW_MEM(SETIFZ(Data.P_CsF, new CSessFilt));
							*Data.P_CsF = *p_filt;
							ok = 1;
						}
						ZDELETE(p_filt);
					}
					break;
			}
			CATCHZOKPPERR
			return ok;
		}
		PPBillAutoCreateParam Data;
	};
	DIALOG_PROC_BODY(BillAutoCreateParamDialog, pParam);
}

int PrcssrBillAutoCreate::Init(const PPBillAutoCreateParam * pParam)
{
	int    ok = 1;
	RVALUEPTR(P, pParam);
	return ok;
}

int PrcssrBillAutoCreate::Run()
{
	int    ok = -1;
	switch(P.A) {
		case PPBillAutoCreateParam::aDraftByTrfrAnlz:
			ok = CreateDraftByTrfrAnlz();
			break;
		case PPBillAutoCreateParam::aDraftBySuspCc:
			break;
		case PPBillAutoCreateParam::aDraftBySupplOrders:
			if(P.P_SsF)
				ok = PrcssrBillAutoCreate::CreateDraftBySupplOrders(P.P_SsF);
			break;
		case PPBillAutoCreateParam::aDraftByCcRule:
			if(P.P_CsF) {
				CSessCrDraftParam ext_param;
				ext_param.RuleID = P.RuleID;
				ext_param.RuleGrpID = P.RuleGrpID;
				ext_param.Period = P.P_CsF->Period;
				ext_param.NodeList = P.P_CsF->NodeList_;
				if(P.P_CsF->fOnlySuperSess)
					ext_param.Flags |= ext_param.fSuperSessOnly;
				ok = PrcssrBillAutoCreate::CreateDraftByCSessRule(&ext_param);
			}
			break;
	}
	return ok;
}
