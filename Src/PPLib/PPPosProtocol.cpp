// PPPOSPROTOCOL.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
/*

pap

source:
	system:
	version:
	name:
	uuid:
	code:

destination:
	system:
	version:
	name:
	uuid:
	code:
*/

/*
	query-csession [id] [date] [daterange] [last] [current]
		tokId, tokCode, tokPeriod, tokTime
	query-refs [objtype]
		tokObjType
*/
class ACS_PAPYRUS_APN : public PPAsyncCashSession {
public:
	SLAPI  ACS_PAPYRUS_APN(PPID n, PPID parent) : PPAsyncCashSession(n), ParentNodeID(parent), P_Pib(0), StatID(0)
	{
		if(GetNodeData(&Acn) > 0) {
			Acn.GetLogNumList(LogNumList);
			ExpPath = Acn.ExpPaths;
			ImpPath = Acn.ImpFiles;
		}
	}
	SLAPI ~ACS_PAPYRUS_APN()
	{
		delete P_Pib;
	}
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0)
	{
		int    ok = -1;
		int    is_forward = 0;
		ASSIGN_PTR(pSessCount, 0);
		CALLPTRMEMB(pPrd, SetZero());
		ZDELETE(P_Pib);
		THROW_MEM(P_Pib = new PPPosProtocol::ProcessInputBlock(this));
		P_Pib->PosNodeID = NodeID;
		P_Pib->Flags |= (P_Pib->fStoreReadBlocks|P_Pib->fBackupProcessed|P_Pib->fRemoveProcessed|P_Pib->fProcessSessions); // @v9.9.12 P_Pib->fProcessSessions
		int    pir = Pp.ProcessInput(*P_Pib);
		THROW(pir);
		if(P_Pib->SessionCount) {
			THROW(CreateTables());
			ASSIGN_PTR(pSessCount, P_Pib->SessionCount);
			ASSIGN_PTR(pPrd, P_Pib->SessionPeriod);
			ok = 1;
		}
		CATCH
			ZDELETE(P_Pib);
			ok = 0;
		ENDCATCH
		ASSIGN_PTR(pIsForwardSess, is_forward);
		return ok;
	}
	virtual int SLAPI ImportSession(int sessN)
	{
		int    ok = -1;
        int    local_n = 0;
		SString temp_buf;
		SString wait_msg_tmpl, wait_msg;
		const TSCollection <PPPosProtocol::ReadBlock> * p_rb_list = P_Pib ? (const TSCollection <PPPosProtocol::ReadBlock> *)P_Pib->GetStoredReadBlocks() : 0;
		PPLoadText(PPTXT_IMPORTCHECKS, wait_msg_tmpl);
		THROW(CreateTables());
		if(p_rb_list) {
			for(uint i = 0; ok < 0 && i < p_rb_list->getCount(); i++) {
				const PPPosProtocol::ReadBlock * p_ib = p_rb_list->at(i);
				if(p_ib) {
					const PPPosProtocol::RouteObjectBlock * p_src_route_blk = 0;
					int    pos_no = 0;
					for(uint _csidx = 0; ok < 0 && _csidx < p_ib->RefList.getCount(); _csidx++) {
						const PPPosProtocol::ObjBlockRef & r_ref = p_ib->RefList.at(_csidx);
						int    type = 0;
						if(r_ref.Type == PPPosProtocol::obSource) {
							p_src_route_blk = (const PPPosProtocol::RouteObjectBlock *)p_ib->GetItem(_csidx, &type);
							THROW(type == PPPosProtocol::obSource);
							if(p_src_route_blk) {
								const PPGenCashNode::PosIdentEntry * p_pie = Acn.SearchPosIdentEntryByGUID(p_src_route_blk->Uuid);
								pos_no = p_pie ? p_pie->N : 0;
							}
						}
						else if(r_ref.Type == PPPosProtocol::obCSession) {
							if(local_n != sessN) {
								local_n++;
							}
							else {
								int    local_pos_no = 0;
								//PPID   src_ar_id = 0;
								PPPosProtocol::ResolveGoodsParam rgp;
								SCardTbl::Rec sc_rec;
								const PPPosProtocol::CSessionBlock * p_cb = (const PPPosProtocol::CSessionBlock *)p_ib->GetItem(_csidx, &type);
								const uint rc = p_ib->RefList.getCount();
								THROW(type == PPPosProtocol::obCSession);
								wait_msg.Printf(wait_msg_tmpl, temp_buf.Z().Cat(p_cb->Dtm, DATF_DMY, TIMF_HMS).cptr());
								if(p_cb->PosBlkP) {
									const PPPosProtocol::PosNodeBlock * p_pnb = (const PPPosProtocol::PosNodeBlock *)p_ib->GetItem(p_cb->PosBlkP, &type);
									if(p_pnb) {
										assert(type == PPPosProtocol::obPosNode);
										p_ib->GetS(p_pnb->CodeP, temp_buf);
										const PPGenCashNode::PosIdentEntry * p_pie = Acn.SearchPosIdentEntryByName(temp_buf);
										local_pos_no = p_pie ? p_pie->N : 0;
									}
								}
								SETIFZ(local_pos_no, pos_no);
								SETIFZ(local_pos_no, 1);
								{
        							PPTransaction tra(1);
        							THROW(tra);
									for(uint cc_refi = 0; cc_refi < rc; cc_refi++) {
										const PPPosProtocol::ObjBlockRef & r_cc_ref = p_ib->RefList.at(cc_refi);
										//const long pos_n = 1; // @stub // Целочисленный номер кассы
										if(r_cc_ref.Type == PPPosProtocol::obCCheck) {
											int    cc_type = 0;
											const PPPosProtocol::CCheckBlock * p_ccb = (const PPPosProtocol::CCheckBlock *)p_ib->GetItem(cc_refi, &cc_type);
											THROW(cc_type == PPPosProtocol::obCCheck);
											if(p_ccb->CSessionBlkP == _csidx) {
												int    ccr = 0;
												PPID   cc_id = 0;
												PPID   cashier_id = 0;
												PPID   sc_id = 0; // ИД персональной карты, привязанной к чеку
												LDATETIME cc_dtm = p_ccb->Dtm;
												double cc_amount = p_ccb->Amount;
												double cc_discount = p_ccb->Discount;
												long   cc_flags = 0;
												if(p_ccb->SaCcFlags & CCHKF_RETURN)
													cc_flags |= CCHKF_RETURN;
												if(p_ccb->SCardBlkP) {
													int    sc_type = 0;
													const PPPosProtocol::SCardBlock * p_scb = (const PPPosProtocol::SCardBlock *)p_ib->GetItem(p_ccb->SCardBlkP, &sc_type);
													assert(sc_type == PPPosProtocol::obSCard);
													if(p_scb->NativeID)
														sc_id = p_scb->NativeID;
													else {
														p_ib->GetS(p_scb->CodeP, temp_buf);
														temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
														if(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0)
															sc_id = sc_rec.ID;
														else {
															; // @log
														}
													}
												}
												THROW(ccr = AddTempCheck(&cc_id, p_cb->Code, cc_flags, local_pos_no, p_ccb->Code, cashier_id, sc_id, &cc_dtm, cc_amount, cc_discount));
												if(ccr > 0) { // @v9.9.12
													for(uint cl_refi = 0; cl_refi < rc; cl_refi++) {
														const PPPosProtocol::ObjBlockRef & r_cl_ref = p_ib->RefList.at(cl_refi);
														if(r_cl_ref.Type == PPPosProtocol::obCcLine) {
															int    cl_type = 0;
															const PPPosProtocol::CcLineBlock * p_clb = (const PPPosProtocol::CcLineBlock *)p_ib->GetItem(cl_refi, &cl_type);
															THROW(cl_type == PPPosProtocol::obCcLine);
															if(p_clb->CCheckBlkP == cc_refi) {
																short  div_n = (short)p_clb->DivN;
																double qtty = (cc_flags & CCHKF_RETURN) ? -fabs(p_clb->Qtty) : fabs(p_clb->Qtty);
																double price = p_clb->Price;
																double dscnt = (p_clb->SumDiscount != 0.0 && qtty != 0.0) ? (p_clb->SumDiscount / qtty) : p_clb->Discount;
																PPID   goods_id = 0;
																if(p_clb->GoodsBlkP) {
																	int    g_type = 0;
																	const PPPosProtocol::GoodsBlock * p_gb = (const PPPosProtocol::GoodsBlock *)p_ib->GetItem(p_clb->GoodsBlkP, &g_type);
																	assert(g_type == PPPosProtocol::obGoods);
																	if(p_gb->NativeID) {
																		goods_id = p_gb->NativeID;
																	}
																	else {
																		THROW(Pp.ResolveGoodsBlock(*p_gb, p_clb->GoodsBlkP, 1, rgp, &goods_id));
																	}
																}
																SetupTempCcLineRec(0, cc_id, p_cb->Code, cc_dtm.d, div_n, goods_id);
																SetTempCcLineValues(0, qtty, price, dscnt, 0 /*serial*/);
																THROW_DB(P_TmpCclTbl->insertRec());
															}
														}
														else if(r_cl_ref.Type == PPPosProtocol::obPayment) {
															int    cl_type = 0;
															const PPPosProtocol::CcPaymentBlock * p_cpb = (const PPPosProtocol::CcPaymentBlock *)p_ib->GetItem(cl_refi, &cl_type);
															THROW(cl_type == PPPosProtocol::obPayment);
															if(p_cpb->CCheckBlkP == cc_refi) {
																PPID   paym_sc_id = 0;
																if(p_cpb->SCardBlkP && p_cpb->PaymType == CCAMTTYP_CRDCARD) {
																	int    sc_type = 0;
																	const PPPosProtocol::SCardBlock * p_scb = (const PPPosProtocol::SCardBlock *)p_ib->GetItem(p_cpb->SCardBlkP, &sc_type);
																	assert(sc_type == PPPosProtocol::obSCard);
																	if(p_scb->NativeID)
																		paym_sc_id = p_scb->NativeID;
																	else {
																		p_ib->GetS(p_scb->CodeP, temp_buf);
																		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
																		if(ScObj.SearchCode(0, temp_buf, &sc_rec) > 0)
																			paym_sc_id = sc_rec.ID;
																		else {
																			; // @log
																		}
																	}
																}
																THROW(AddTempCheckPaym(cc_id, p_cpb->PaymType, p_cpb->Amount, paym_sc_id));
															}
														}
													}
												}
											}
										}
										PPWaitPercent(cc_refi+1, rc, wait_msg);
									}
									THROW(tra.Commit());
								}
								ok = 1;
							}
						}
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int SLAPI FinishImportSession(PPIDArray * pList)
	{
		Pp.DestroyReadBlock();
		return 1;
	}
	virtual int SLAPI InteractiveQuery()
	{
		int    ok = -1;
		TSVector <PPPosProtocol::QueryBlock> qb_list; // @v9.8.4 TSArray-->TSVector
		if(PPPosProtocol::EditPosQuery(qb_list) > 0) {
			if(qb_list.getCount()) {
				for(uint i = 0; i < qb_list.getCount(); i++) {
					THROW(Pp.SendQuery(NodeID, qb_list.at(i)));
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
private:
	PPID   StatID;
	PPID   ParentNodeID;
	PPAsyncCashNode Acn;
	PPIDArray  LogNumList;
	PPIDArray  SessAry;
	SString    ExpPath;
	SString    ImpPath;
	PPObjGoods GObj;
	PPObjPerson PsnObj;
	PPObjSCard  ScObj;

	PPPosProtocol Pp; // Экземпляр PPPosProtocol создаваемый для импорта сессий
	PPPosProtocol::ProcessInputBlock * P_Pib;
};

class CM_PAPYRUS : public PPCashMachine {
public:
	SLAPI  CM_PAPYRUS(PPID posNodeID) : PPCashMachine(posNodeID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface()
	{
		return new ACS_PAPYRUS_APN(NodeID, ParentNodeID);
	}
};

REGISTER_CMT(PAPYRUS,0,1);

//virtual
int SLAPI ACS_PAPYRUS_APN::ExportData(int updOnly)
{
	return Pp.ExportDataForPosNode(NodeID, updOnly, SinceDlsID);
}

int SLAPI PPPosProtocol::InitSrcRootInfo(PPID posNodeID, PPPosProtocol::RouteBlock & rInfo)
{
	int    ok = 1;
	PPCashNode cn_rec;
	SString temp_buf;
	rInfo.Destroy();
	if(posNodeID && CnObj.Search(posNodeID, &cn_rec) > 0) {
		ObjTagItem tag_item;
		S_GUID uuid;
		if(PPRef->Ot.GetTag(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_UUID, &tag_item) > 0 && tag_item.GetGuid(&uuid) > 0) {
			rInfo.Uuid = uuid;
		}
		{
			(temp_buf = cn_rec.Symb).Strip();
			if(temp_buf.IsDigit()) {
				long cn_n = temp_buf.ToLong();
				if(cn_n > 0)
					rInfo.Code = temp_buf;
			}
		}
	}
	{
		DbProvider * p_dict = CurDict;
		if(p_dict) {
			if(rInfo.Uuid.IsZero())
				p_dict->GetDbUUID(&rInfo.Uuid);
			if(rInfo.Code.Empty())
				p_dict->GetDbSymb(rInfo.Code);
		}
	}
	{
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(rInfo.System);
		vi.GetVersion().ToStr(rInfo.Version);
	}
	return ok;
}

int SLAPI PPPosProtocol::SendQuery(PPID posNodeID, const PPPosProtocol::QueryBlock & rQ)
{
	int    ok = 1;
	StringSet ss_out_files;
	SString out_file_name;
	SString temp_buf;
	PPAsyncCashNode acn_pack;
	THROW(oneof3(rQ.Q, QueryBlock::qTest, QueryBlock::qRefs, QueryBlock::qCSession));
	//PPMakeTempFileName("pppp", "xml", 0, out_file_name);
	if(!posNodeID || CnObj.GetAsync(posNodeID, &acn_pack) <= 0) {
		acn_pack.ID = 0;
	}
	THROW(SelectOutFileName(acn_pack.ID, "query", ss_out_files));
	for(uint ssp = 0; ss_out_files.get(&ssp, out_file_name);) {
		PPPosProtocol::WriteBlock wb;
		THROW(StartWriting(out_file_name, wb));
		{
			PPPosProtocol::RouteBlock rb_src;
			InitSrcRootInfo(posNodeID, rb_src);
			THROW(WriteRouteInfo(wb, "source", rb_src));
			//THROW(WriteSourceRoute(wb));
			/*
			{
				PPPosProtocol::RouteBlock rb_dest;
				THROW(WriteRouteInfo(wb, "destination", rb_dest));
			}
			*/
			{
				PPPosProtocol::RouteBlock rb_dest;
				int    dest_list_written = 0;
				if(acn_pack.ID) {
					for(uint i = 0; i < acn_pack.ApnCorrList.getCount(); i++) {
						const PPGenCashNode::PosIdentEntry * p_dest_entry = acn_pack.ApnCorrList.at(i);
						if(p_dest_entry && p_dest_entry->N > 0) {
							rb_dest.Destroy();
							rb_dest.Code.Cat(p_dest_entry->N);
							rb_dest.Uuid = p_dest_entry->Uuid;
							THROW(WriteRouteInfo(wb, "destination", rb_dest));
							dest_list_written = 1;
						}
					}
					if(!dest_list_written) {
						LongArray n_list;
						acn_pack.GetLogNumList(n_list);
						for(uint i = 0; i < n_list.getCount(); i++) {
							const long n_item = n_list.get(i);
							if(n_item > 0) {
								rb_dest.Destroy();
								rb_dest.Code.Cat(n_item);
								THROW(WriteRouteInfo(wb, "destination", rb_dest));
								dest_list_written = 2;
							}
						}
					}
					// THROW(dest_list_written); // @todo error
				}
			}
		}
		{
			if(rQ.Q == QueryBlock::qCSession) {
				SXml::WNode w_s(wb.P_Xw, "query-csession");
				if(rQ.Flags & QueryBlock::fCSessCurrent) {
					w_s.PutInner("current", 0);
				}
				else if(rQ.Flags & QueryBlock::fCSessLast) {
					w_s.PutInner("last", 0);
				}
				else if(!rQ.Period.IsZero()) {
					THROW(checkdate(rQ.Period.low, 1) && checkdate(rQ.Period.upp, 1));
					temp_buf.Z();
					if(rQ.Period.low)
						temp_buf.Cat(rQ.Period.low, DATF_ISO8601|DATF_CENTURY);
					temp_buf.Dot().Dot();
					if(rQ.Period.upp)
						temp_buf.Cat(rQ.Period.upp, DATF_ISO8601|DATF_CENTURY);
					w_s.PutInner("period", temp_buf);
				}
				else if(rQ.CSess) {
					temp_buf.Z().Cat(rQ.CSess);
					w_s.PutInner((rQ.Flags & QueryBlock::fCSessN) ? "code" : "id", temp_buf);
				}
			}
			else if(rQ.Q == QueryBlock::qRefs) {
				SXml::WNode w_s(wb.P_Xw, "query-refs");
			}
			else if(rQ.Q == QueryBlock::qTest) {
				SXml::WNode w_s(wb.P_Xw, "query-test");
			}
		}
		FinishWriting(wb);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ExportDataForPosNode(PPID nodeID, int updOnly, PPID sinceDlsID)
{
	int    ok = 1;
	PPIDArray qk_list; // Список идентификаторов видов котировок, которые должны выгружаться
	PPIDArray unit_list; // Список единиц измерения, которые необходимо выгрузить
	PPIDArray used_qk_list; // Список идентификаторов видов котировок, которые выгружались
	SString out_file_name;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	PPPosProtocol::WriteBlock wb;
	PPObjQuotKind qk_obj;
	PPObjGoods goods_obj;

	PPAsyncCashNode cn_data;
	THROW(CnObj.GetAsync(nodeID, &cn_data) > 0);
	wb.LocID = cn_data.LocID; // @v9.6.7
	PPWait(1);
	PPMakeTempFileName("pppp", /*"xml"*/"ppyp", 0, out_file_name);
	{
		DeviceLoadingStat dls;
		PPID   stat_id = 0;
		dls.StartLoading(&stat_id, dvctCashs, nodeID, 1);
		THROW(StartWriting(out_file_name, wb));
		{
			PPQuotKind qk_rec;
			for(SEnum en = qk_obj.Enum(0); en.Next(&qk_rec) > 0;) {
				if(qk_rec.ID == PPQUOTK_BASE || (qk_rec.Flags & QUOTKF_RETAILED)) {
					qk_list.add(qk_rec.ID);
				}
			}
			qk_list.sortAndUndup();
		}
		{
			PPPosProtocol::RouteBlock rb_src;
			InitSrcRootInfo(nodeID, rb_src);
			THROW(WriteRouteInfo(wb, "source", rb_src));
		}
		{
			PPPosProtocol::RouteBlock rb;
			int    dest_list_written = 0;
			{
				for(uint i = 0; i < cn_data.ApnCorrList.getCount(); i++) {
					const PPGenCashNode::PosIdentEntry * p_dest_entry = cn_data.ApnCorrList.at(i);
					if(p_dest_entry && p_dest_entry->N > 0) {
						rb.Destroy();
						rb.Code.Cat(p_dest_entry->N);
						rb.Uuid = p_dest_entry->Uuid;
						THROW(WriteRouteInfo(wb, "destination", rb));
						dest_list_written = 1;
					}
				}
			}
			if(!dest_list_written) {
                LongArray n_list;
                cn_data.GetLogNumList(n_list);
                for(uint i = 0; i < n_list.getCount(); i++) {
                    const long n_item = n_list.get(i);
                    if(n_item > 0) {
						rb.Destroy();
						rb.Code.Cat(n_item);
						THROW(WriteRouteInfo(wb, "destination", rb));
						dest_list_written = 2;
                    }
                }
			}
			THROW_PP_S(dest_list_written, PPERR_PPPP_CORRPOSLISTNEEDED, cn_data.Name);
		}
		/*
		{
			SXml::WNode n_scope(wb.P_Xw, "destination");
		}
		*/
		{
			SXml::WNode n_scope(wb.P_Xw, "refs");
			{
				long   acgi_flags = ACGIF_ALLCODESPERITER;
				PPQuotArray qlist; // "Сырой" список котировок, полученных из БД
				PPQuotArray qlist_result; // Список котировок, которые следует экспортировать
				if(updOnly)
					acgi_flags |= ACGIF_UPDATEDONLY;
				AsyncCashGoodsIterator acgi(nodeID, acgi_flags, sinceDlsID, &dls);
				{
					//
					// Единицы измерения
					//
					const PPIDArray * p_unit_list = acgi.GetRefList(PPOBJ_UNIT);
                    if(p_unit_list && p_unit_list->getCount()) {
						const PPID def_unit_id = GObj.GetConfig().DefUnitID;
						PPIDArray unit_list(*p_unit_list);
						uint i;
						PPUnit unit_rec;
						i = unit_list.getCount();
						do {
							const PPID unit_id = unit_list.get(--i);
                            if(goods_obj.FetchUnit(unit_id, &unit_rec) > 0) {
								PPUnit base_unit_rec;
                            	if(unit_rec.BaseUnitID && goods_obj.FetchUnit(unit_rec.BaseUnitID, &base_unit_rec) > 0)
                                    unit_list.add(unit_rec.BaseUnitID);
                            }
						} while(i);
						//
						// Форсированно добавляем зарезервированную единицу LITER в список экспорта
						// Она нам понадобиться для передачи алкогольных товаров.
						//
						if(!unit_list.lsearch(PPUNT_LITER) && goods_obj.FetchUnit(PPUNT_LITER, &unit_rec) > 0)
							unit_list.add(PPUNT_LITER);
						unit_list.sortAndUndup();
						for(i = 0; i < unit_list.getCount(); i++) {
							const PPID unit_id = unit_list.get(i);
                            if(goods_obj.FetchUnit(unit_id, &unit_rec) > 0) {
								SXml::WNode w_s(wb.P_Xw, "unit");
								w_s.PutInner("id", temp_buf.Z().Cat(unit_rec.ID));
								w_s.PutInner("name", EncText(unit_rec.Name));
								w_s.PutInnerSkipEmpty("symb", EncText(unit_rec.Abbr));
								w_s.PutInnerSkipEmpty("code", EncText(unit_rec.Code));
								if(unit_id == def_unit_id) {
									w_s.PutInner("default", "true");
								}
								if(unit_rec.Flags & PPUnit::Physical)
									w_s.PutInner("physical", "true");
								if(unit_rec.Rounding > 0.0) {
									temp_buf.Z().Cat(unit_rec.Rounding, MKSFMTD(0, 8, NMBF_NOTRAILZ|NMBF_OMITEPS));
									w_s.PutInner("rounding", temp_buf);
								}
								if(unit_rec.BaseUnitID && unit_rec.BaseRatio > 0.0 && unit_list.lsearch(unit_rec.BaseUnitID)) {
									w_s.PutInner("base", temp_buf.Z().Cat(unit_rec.BaseUnitID));
									w_s.PutInner("baseratio", temp_buf.Z().Cat(unit_rec.BaseRatio, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
								}
                            }
						}
                    }
				}
				if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
					AsyncCashGoodsGroupInfo acggi_info;
					AsyncCashGoodsGroupIterator * p_group_iter = acgi.GetGroupIterator();
					if(p_group_iter) {
						while(p_group_iter->Next(&acggi_info) > 0) {
							qlist.clear();
							qlist_result.clear();
							goods_obj.GetQuotList(acggi_info.ID, /*cn_data.LocID*/0, qlist);
							// @v10.0.0 {
							for(uint qkidx = 0; qkidx < qk_list.getCount(); qkidx++) {
								const PPID qk_id = qk_list.get(qkidx);
								const QuotIdent qi(cn_data.LocID, qk_id, 0, 0);
								uint   ql_pos = 0;
								if(qlist.SearchNearest(qi, &ql_pos) > 0) {
									qlist_result.insert(&qlist.at(ql_pos));
									used_qk_list.addUnique(qk_id);
								}
							}
							// } @v10.0.0 
							/* @v10.0.0 {
								uint qp = qlist.getCount();
								if(qp) do {
									PPQuot & r_q = qlist.at(--qp);
									if(!qk_list.bsearch(r_q.Kind))
										qlist.atFree(qp);
									else
										used_qk_list.addUnique(r_q.Kind);
								} while(qp);
							}*/
							THROW(WriteGoodsGroupInfo(wb, "goodsgroup", acggi_info, &qlist_result));
						}
					}
				}
				{
					// Товары
					AsyncCashGoodsInfo acgi_item;
					wb.P_Acgi = &acgi;
					while(acgi.Next(&acgi_item) > 0) {
						qlist.clear();
						qlist_result.clear();
						goods_obj.GetQuotList(acgi_item.ID, cn_data.LocID, qlist);
						// @v10.0.0 {
						for(uint qkidx = 0; qkidx < qk_list.getCount(); qkidx++) {
							const PPID qk_id = qk_list.get(qkidx);
							const QuotIdent qi(cn_data.LocID, qk_id, 0, 0);
							uint   ql_pos = 0;
							if(qlist.SearchNearest(qi, &ql_pos) > 0) {
								qlist_result.insert(&qlist.at(ql_pos));
								used_qk_list.addUnique(qk_id);
							}
						}
						// } @v10.0.0 
						/*@v10.0.0 {
							uint qp = qlist.getCount();
							if(qp) do {
								PPQuot & r_q = qlist.at(--qp);
								if(!qk_list.bsearch(r_q.Kind))
									qlist.atFree(qp);
								else
									used_qk_list.addUnique(r_q.Kind);
							} while(qp);
						}*/
						THROW(WriteGoodsInfo(wb, "ware", acgi_item, &qlist_result));
						PPWaitPercent(acgi.GetIterCounter());
					}
					wb.P_Acgi = 0;
				}
			}
			{
				//
				// Замечение по ссылке карт на серию:
				// Допускается экспортировать карты с непосредственной ссылкой на серию:
				// <card><id>21</id><code>100001</code><series><id>1001</id></series></card>
				// Либо в блоке каждой серии перечислить карты бех прямой ссылки на серию.
				// <scardseries><id>1001</id><name>Some series</name>
				//    <card><id>21</id><code>100001</code></card>
				//    <card><id>22</id><code>100002</code></card>
				// </scardseries>
				// Очевидно, 2-й способ позволяет сэкономить на размере xml-файла. Именно по этому
				// мы его и предпочтем.
				// При разборе допустимы оба варианта!
				//
				PPObjSCardSeries scs_obj;
				PPSCardSerPacket scs_pack;
				PPSCardSeries scs_rec;
				{
					//
					// Карты
					//
					LAssocArray scard_quot_list;
					AsyncCashSCardsIterator acci(nodeID, updOnly, &dls, sinceDlsID);
					for(PPID ser_id = 0, idx = 1; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
						const int scs_type = scs_rec.GetType();
						AsyncCashSCardInfo acci_item;
						if(scs_obj.GetPacket(ser_id, &scs_pack) > 0) {
							SXml::WNode w_s(wb.P_Xw, "scardseries");
							w_s.PutInner("id", temp_buf.Z().Cat(scs_pack.Rec.ID));
							w_s.PutInner("name", EncText(scs_pack.Rec.Name));
							w_s.PutInnerSkipEmpty("code", EncText(scs_pack.Rec.Symb));
							//
							if(scs_rec.QuotKindID_s)
								THROW_SL(scard_quot_list.Add(scs_rec.ID, scs_rec.QuotKindID_s, 0));
							(msg_buf = fmt_buf).CatDiv(':', 2).Cat(scs_rec.Name);
							for(acci.Init(&scs_pack); acci.Next(&acci_item) > 0;) {
								THROW(WriteSCardInfo(wb, "card", acci_item));
								//scs_list.add(acci_item.Rec.SeriesID);
								PPWaitPercent(acci.GetCounter(), msg_buf);
							}
						}
					}
				}
			}
			{
				//
				// Виды котировок
				//
				if(used_qk_list.getCount()) {
					PPQuotKind qk_rec;
					for(uint i = 0; i < used_qk_list.getCount(); i++) {
						if(qk_obj.Search(used_qk_list.get(i), &qk_rec) > 0) {
							THROW(WriteQuotKindInfo(wb, "quotekind", qk_rec));
						}
					}
				}
			}
		}
		FinishWriting(wb);
		if(stat_id)
			dls.FinishLoading(stat_id, 1, 1);
	}
	THROW(TransportFileOut(out_file_name, nodeID, "refs"));
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PPPosProtocol::TransportFileOut(const SString & rOutFileName, PPID srcPosNodeID, const char * pInfix)
{
	int   ok = 1;
	THROW_SL(fileExists(rOutFileName));
	{
		StringSet ss_out_files;
		THROW(SelectOutFileName(srcPosNodeID, pInfix, ss_out_files));
		{
			int   copy_result = 0;
			SString temp_buf;
			for(uint ssp = 0; ss_out_files.get(&ssp, temp_buf);) {
				if(SCopyFile(rOutFileName, temp_buf, 0, FILE_SHARE_READ, 0))
					copy_result = 1;
			}
			if(copy_result)
				SFile::Remove(rOutFileName);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPPosProtocol::WriteBlock::WriteBlock() : P_Xw(0), P_Xd(0), P_Root(0), LocID(0), P_Acgi(0), FileDtm(ZERODATETIME)
{
	FileUUID.Generate();
}

SLAPI PPPosProtocol::WriteBlock::~WriteBlock()
{
	Destroy();
}

void PPPosProtocol::WriteBlock::Destroy()
{
	ZDELETE(P_Root);
	ZDELETE(P_Xd);
	xmlFreeTextWriter(P_Xw);
	P_Xw = 0;
	P_Acgi = 0;
	UsedQkList.freeAll();
}

SLAPI PPPosProtocol::RouteBlock::RouteBlock()
{
}

void SLAPI PPPosProtocol::RouteBlock::Destroy()
{
	Uuid.Z();
	System.Z();
	Version.Z();
	Code.Z();
}

int SLAPI PPPosProtocol::RouteBlock::IsEmpty() const
{
	return BIN(Uuid.IsZero() && System.Empty() && Version.Empty() && Code.Empty());
}

int FASTCALL PPPosProtocol::RouteBlock::IsEqual(const RouteBlock & rS) const
{
	int    yes = 1;
	if(!Uuid.IsZero())
		yes = rS.Uuid.IsZero() ? 0 : (Uuid == rS.Uuid);
	else if(!rS.Uuid.IsZero())
		yes = 0;
	else if(Code.NotEmpty())
		yes = rS.Code.NotEmpty() ? (Code == rS.Code) : 0;
	else if(rS.Code.NotEmpty())
		yes = 0;
	return yes;
}
//
//
//
SLAPI PPPosProtocol::ObjectBlock::ObjectBlock() : Flags_(0), ID(0), NativeID(0), NameP(0)
{
}

SLAPI PPPosProtocol::PosNodeBlock::PosNodeBlock() : ObjectBlock(), CodeP(0), CodeI(0)
{
}

SLAPI PPPosProtocol::QuotKindBlock::QuotKindBlock() : ObjectBlock(), CodeP(0), Rank(0), Reserve(0)
{
    Period.SetZero();
	TimeRestriction.SetZero();
	AmountRestriction.Clear();
}

SLAPI PPPosProtocol::UnitBlock::UnitBlock() : ObjectBlock(), CodeP(0), SymbP(0), PhUnitBlkP(0), UnitFlags(0), BaseId(0), BaseRatio(0.0), PhRatio(0.0)
{
}

SLAPI PPPosProtocol::GoodsBlock::GoodsBlock() :
	ObjectBlock(), ParentBlkP(0), InnerId(0),  GoodsFlags(0), SpecialFlags(0), UnitBlkP(0),
	PhUnitBlkP(0),  PhUPerU(0.0), Price(0.0), Rest(0.0), AlcoProof(0), VatRate(0), SalesTaxRate(0),
	TaxGrpID(0), GoodsTypeID(0), AlcoRuCatP(0)
{
}

SLAPI PPPosProtocol::GoodsGroupBlock::GoodsGroupBlock() : ObjectBlock(), CodeP(0), ParentBlkP(0)
	{}
SLAPI PPPosProtocol::LotBlock::LotBlock() : ObjectBlock(), GoodsBlkP(0), Dt(ZERODATE), Expiry(ZERODATE), Cost(0.0), Price(0.0), Rest(0.0), SerailP(0)
	{}
SLAPI PPPosProtocol::PersonBlock::PersonBlock() : ObjectBlock(), CodeP(0)
	{}
SLAPI PPPosProtocol::SCardSeriesBlock::SCardSeriesBlock() : ObjectBlock(), RefP(0), CodeP(0), QuotKindBlkP(0)
	{}
SLAPI PPPosProtocol::SCardBlock::SCardBlock() : ObjectBlock(), CodeP(0), OwnerBlkP(0), SeriesBlkP(0), Discount(0.0)
	{}
SLAPI PPPosProtocol::CSessionBlock::CSessionBlock() : ObjectBlock(), ID(0), Code(0), PosBlkP(0), Dtm(ZERODATETIME)
	{}
SLAPI PPPosProtocol::CCheckBlock::CCheckBlock() : ObjectBlock(), Code(0), CcFlags(0), SaCcFlags(0), CTableN(0), GuestCount(0), CSessionBlkP(0),
	AddrBlkP(0), AgentBlkP(0), Amount(0.0), Discount(0.0), Dtm(ZERODATETIME), CreationDtm(ZERODATETIME), SCardBlkP(0), MemoP(0)
	{}
SLAPI PPPosProtocol::CcLineBlock::CcLineBlock() : ObjectBlock(), CcID(0), RByCheck(0), CclFlags(0), DivN(0), Queue(0), GoodsBlkP(0),
	Qtty(0.0), Price(0.0), Discount(0.0), SumDiscount(0.0), Amount(0.0), CCheckBlkP(0), SerialP(0), EgaisMarkP(0)
	{}
SLAPI PPPosProtocol::CcPaymentBlock::CcPaymentBlock() : CcID(0), PaymType(0), Amount(0.0), SCardBlkP(0)
	{}
SLAPI PPPosProtocol::QueryBlock::QueryBlock()
	{ Init(qUnkn); }

void FASTCALL PPPosProtocol::QueryBlock::Init(int q)
{
	THISZERO();
	Q = oneof3(q, qTest, qCSession, qRefs) ? q : qUnkn;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionLast()
{
	Init(qCSession);
	Flags = fCSessLast;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionCurrent()
{
	Init(qCSession);
	Flags = fCSessCurrent;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByID(PPID sessID)
{
	Init(qCSession);
	CSess = sessID;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByNo(long sessN)
{
	Init(qCSession);
	Flags = fCSessN;
	CSess = sessN;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryCSessionByDate(const DateRange & rPeriod)
{
	Init(qCSession);
	Period = rPeriod;
}

void SLAPI PPPosProtocol::QueryBlock::SetQueryRefs() { Init(qRefs); }
void SLAPI PPPosProtocol::QueryBlock::SetQueryTest() { Init(qTest); }

SLAPI PPPosProtocol::QuotBlock::QuotBlock() { THISZERO(); }
SLAPI PPPosProtocol::ParentBlock::ParentBlock() { THISZERO(); }
SLAPI PPPosProtocol::GoodsCode::GoodsCode() { THISZERO(); }

SLAPI PPPosProtocol::RouteObjectBlock::RouteObjectBlock() : ObjectBlock(), Direction(0), SystemP(0), VersionP(0), CodeP(0)
{
}

SLAPI PPPosProtocol::ObjBlockRef::ObjBlockRef(int t, uint pos) : Type(t), P(pos)
{
}

int SLAPI PPPosProtocol::ReadBlock::Implement_CreateItem(SVector & rList, const void * pNewBlk, int type, uint * pRefPos)
{
	int    ok = 1;
	ObjBlockRef ref(type, rList.getCount());
	THROW_SL(rList.insert(pNewBlk));
	ASSIGN_PTR(pRefPos, RefList.getCount());
	THROW_SL(RefList.insert(&ref));
	CATCHZOK
	return ok;
}

int  SLAPI PPPosProtocol::ReadBlock::CreateItem(int type, uint * pRefPos)
{
	int    ok = 1;
	switch(type) {
		case obGoods:       THROW(Helper_CreateItem(GoodsBlkList, type, pRefPos)); break;
		case obGoodsGroup:  THROW(Helper_CreateItem(GoodsGroupBlkList, type, pRefPos)); break;
		case obPerson:      THROW(Helper_CreateItem(PersonBlkList, type, pRefPos)); break;
		case obGoodsCode:   THROW(Helper_CreateItem(GoodsCodeList, type, pRefPos)); break;
		case obLot:         THROW(Helper_CreateItem(LotBlkList, type, pRefPos)); break;
		case obSCardSeries: THROW(Helper_CreateItem(ScsBlkList, type, pRefPos)); break; // @v9.8.7
		case obSCard:       THROW(Helper_CreateItem(SCardBlkList, type, pRefPos)); break;
		case obParent:      THROW(Helper_CreateItem(ParentBlkList, type, pRefPos)); break;
		case obUnit:        THROW(Helper_CreateItem(UnitBlkList, type, pRefPos)); break; // @v9.8.6
		case obQuotKind:    THROW(Helper_CreateItem(QkBlkList, type, pRefPos)); break;
		case obQuot:        THROW(Helper_CreateItem(QuotBlkList, type, pRefPos)); break;
		case obSource:      THROW(Helper_CreateItem(SrcBlkList, type, pRefPos)); break;
		case obDestination: THROW(Helper_CreateItem(DestBlkList, type, pRefPos)); break;
		case obCSession:    THROW(Helper_CreateItem(CSessBlkList, type, pRefPos)); break;
		case obCCheck:      THROW(Helper_CreateItem(CcBlkList, type, pRefPos)); break;
		case obCcLine:      THROW(Helper_CreateItem(CclBlkList, type, pRefPos)); break;
		case obPayment:     THROW(Helper_CreateItem(CcPaymBlkList, type, pRefPos)); break;
		case obPosNode:     THROW(Helper_CreateItem(PosBlkList, type, pRefPos)); break;
		case obQuery:       THROW(Helper_CreateItem(QueryList, type, pRefPos)); break;
		default:
			assert(0);
			CALLEXCEPT();
			break;
	}
	CATCHZOK
	return ok;
}

PPPosProtocol::ReadBlock & FASTCALL PPPosProtocol::ReadBlock::Copy(const PPPosProtocol::ReadBlock & rS)
{
	Destroy();
	SStrGroup::CopyS(rS);
	#define CPY_FLD(f) f = rS.f
	CPY_FLD(SrcFileName);
	CPY_FLD(SrcFileUUID); // @v9.9.12
	CPY_FLD(SrcFileDtm); // @v9.9.12
	CPY_FLD(SrcBlkList);
	CPY_FLD(DestBlkList);
	CPY_FLD(GoodsBlkList);
	CPY_FLD(GoodsGroupBlkList);
	CPY_FLD(GoodsCodeList);
	CPY_FLD(LotBlkList);
	CPY_FLD(QkBlkList);
	CPY_FLD(UnitBlkList); // @v9.8.6
	CPY_FLD(QuotBlkList);
	CPY_FLD(PersonBlkList);
	CPY_FLD(ScsBlkList);
	CPY_FLD(SCardBlkList);
	CPY_FLD(ParentBlkList);
	CPY_FLD(PosBlkList);
	CPY_FLD(CSessBlkList);
	CPY_FLD(CcBlkList);
	CPY_FLD(CclBlkList);
	CPY_FLD(CcPaymBlkList);
	CPY_FLD(QueryList);
	CPY_FLD(RefList);
	#undef CPY_FLD
	return *this;
}

int SLAPI PPPosProtocol::ReadBlock::GetRouteItem(const RouteObjectBlock & rO, RouteBlock & rR) const
{
	int    ok = 1;
	rR.Destroy();
	rR.Uuid = rO.Uuid;
	GetS(rO.CodeP, rR.Code);
	GetS(rO.SystemP, rR.System);
	GetS(rO.VersionP, rR.Version);
	return ok;
}

int SLAPI PPPosProtocol::ReadBlock::IsTagValueBoolTrue() const
{
	return (TagValue.Empty() || TagValue.CmpNC("true") == 0 || TagValue.CmpNC("yes") == 0 || TagValue == "1");
}

void * SLAPI PPPosProtocol::ReadBlock::GetItemWithTest(uint refPos, int type) const
{
	int    test_type = 0;
	void * p_item = GetItem(refPos, &test_type);
	assert(test_type == type);
	return p_item;
}

void * SLAPI PPPosProtocol::ReadBlock::GetItem(uint refPos, int * pType) const
{
	void * p_ret = 0;
	int    type = 0;
	if(refPos < RefList.getCount()) {
		const ObjBlockRef & r_ref = RefList.at(refPos);
		type = r_ref.Type;
		switch(type) {
			case obGoods:       p_ret = &GoodsBlkList.at(r_ref.P); break;
			case obGoodsGroup:  p_ret = &GoodsGroupBlkList.at(r_ref.P); break;
			case obPerson:      p_ret = &PersonBlkList.at(r_ref.P); break;
			case obGoodsCode:   p_ret = &GoodsCodeList.at(r_ref.P); break;
			case obSCardSeries: p_ret = &ScsBlkList.at(r_ref.P); break;
			case obSCard:       p_ret = &SCardBlkList.at(r_ref.P); break;
			case obParent:      p_ret = &ParentBlkList.at(r_ref.P); break;
			case obQuotKind:    p_ret = &QkBlkList.at(r_ref.P); break;
			case obUnit:        p_ret = &UnitBlkList.at(r_ref.P); break; // @v9.8.6
			case obQuot:        p_ret = &QuotBlkList.at(r_ref.P); break;
			case obSource:      p_ret = &SrcBlkList.at(r_ref.P); break;
			case obDestination: p_ret = &DestBlkList.at(r_ref.P); break;
			case obCSession:    p_ret = &CSessBlkList.at(r_ref.P); break;
			case obCCheck:      p_ret = &CcBlkList.at(r_ref.P); break;
			case obCcLine:      p_ret = &CclBlkList.at(r_ref.P); break;
			case obPayment:     p_ret = &CcPaymBlkList.at(r_ref.P); break;
			case obPosNode:     p_ret = &PosBlkList.at(r_ref.P); break;
			case obQuery:       p_ret = &QueryList.at(r_ref.P); break;
			case obLot:         p_ret = &LotBlkList.at(r_ref.P); break;
			default:
				assert(0);
				CALLEXCEPT();
				break;
		}
	}
	CATCH
		p_ret = 0;
	ENDCATCH
	ASSIGN_PTR(pType, type);
	return p_ret;
}

IMPL_CMPCFUNC(ObjBlockRef_, p1, p2) { RET_CMPCASCADE2((const PPPosProtocol::ObjBlockRef *)p1, (const PPPosProtocol::ObjBlockRef *)p2, Type, P); }

/*void SLAPI PPPosProtocol::ReadBlock::SortRefList()
{
	RefList.sort(PTR_CMPCFUNC(ObjBlockRef_));
}*/

int SLAPI PPPosProtocol::ReadBlock::SearchRef(int type, uint pos, uint * pRefPos) const
{
	int    ok = 0;
	const  uint _c = RefList.getCount();
	for(uint i = 0; i < _c; i++) {
		const ObjBlockRef & r_ref = RefList.at(i);
		if(r_ref.Type == type && r_ref.P == pos) {
			ASSIGN_PTR(pRefPos, i);
			return 1;
		}
	}
	ASSIGN_PTR(pRefPos, _c);
	return 0;
}

struct PPPP_SearchAnalogResult {
	PPPP_SearchAnalogResult() : Ok(0), RefPos(0)
	{
	}
	int    operator !() const { return (Ok == 0); }
	void   Found(uint pos)
	{
		Ok = 1;
		RefPos = pos;
	}
	int    Ok;
	uint   RefPos;
};

int SLAPI PPPosProtocol::ReadBlock::SearchAnalogRef_SCardSeries(const PPPosProtocol::SCardSeriesBlock & rBlk, uint exclPos, uint * pRefPos) const
{
	PPPP_SearchAnalogResult result;
	if(rBlk.Flags_ & rBlk.fRefItem) {
		SString code_buf;
		GetS(rBlk.CodeP, code_buf);
		const int code_isnt_empty = code_buf.NotEmpty();
		if(rBlk.ID || code_isnt_empty) {
			SString temp_buf;
			for(uint i = 0; !result && i < ScsBlkList.getCount(); i++) {
				const SCardSeriesBlock * p_item = &ScsBlkList.at(i);
				const uint _rtidx = p_item->RefP;
				if(_rtidx && (!exclPos || _rtidx != exclPos)) {
					const SCardSeriesBlock * p_test_item = (const SCardSeriesBlock *)GetItemWithTest(_rtidx, obSCardSeries);
					assert(p_test_item);
					if(p_item->Flags_ & p_item->fRefItem) {
						if(rBlk.ID) {
							if(p_item->ID == rBlk.ID) {
								if(code_isnt_empty) {
									if(GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf)
										result.Found(_rtidx);
								}
								else if(p_item->CodeP == 0)
									result.Found(_rtidx);
							}
						}
						else if(code_isnt_empty && GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf)
							result.Found(_rtidx);
					}
				}
			}
		}
	}
	ASSIGN_PTR(pRefPos, result.RefPos);
	return result.Ok;
}

int SLAPI PPPosProtocol::ReadBlock::SearchAnalogRef_QuotKind(const PPPosProtocol::QuotKindBlock & rBlk, uint exclPos, uint * pRefPos) const
{
	PPPP_SearchAnalogResult result;
	if(rBlk.Flags_ & rBlk.fRefItem) {
		SString code_buf;
		GetS(rBlk.CodeP, code_buf);
		const int code_isnt_empty = code_buf.NotEmpty();
		if(rBlk.ID || code_isnt_empty) {
			SString temp_buf;
			for(uint i = 0; !result && i < RefList.getCount(); i++) {
				if((!exclPos || i != exclPos) && RefList.at(i).Type == obQuotKind) {
					const QuotKindBlock * p_item = (const QuotKindBlock *)GetItemWithTest(i, obQuotKind);
					if(p_item->Flags_ & p_item->fRefItem) {
						if(rBlk.ID) {
							if(p_item->ID == rBlk.ID) {
								if(code_isnt_empty) {
									if(GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf)
										result.Found(i);
								}
								else if(p_item->CodeP == 0)
									result.Found(i);
							}
						}
						else if(code_isnt_empty && GetS(p_item->CodeP, temp_buf) && code_buf == temp_buf)
							result.Found(i);
					}
				}
			}
		}
	}
	ASSIGN_PTR(pRefPos, result.RefPos);
	return result.Ok;
}

const PPPosProtocol::QuotKindBlock * FASTCALL PPPosProtocol::ReadBlock::SearchAnalog_QuotKind(const PPPosProtocol::QuotKindBlock & rBlk) const
{
	const QuotKindBlock * p_ret = 0;
	if(rBlk.ID) {
		for(uint i = 0; !p_ret && i < QkBlkList.getCount(); i++) {
			const QuotKindBlock & r_item = QkBlkList.at(i);
			if(r_item.NativeID && r_item.ID == rBlk.ID) {
				p_ret = &r_item;
			}
		}
	}
	return p_ret;
}
//
// Descr: Находит входящий аналог блока персоналии rBlk с идентифицированным
//   NativeID.
// Returns:
//   Указатель на найденный блок-аналог
//   Если поиск оказался безуспешным, то возвращает 0
//
const PPPosProtocol::PersonBlock * FASTCALL PPPosProtocol::ReadBlock::SearchAnalog_Person(const PPPosProtocol::PersonBlock & rBlk) const
{
	const PersonBlock * p_ret = 0;
	if(rBlk.ID) {
		for(uint i = 0; !p_ret && i < PersonBlkList.getCount(); i++) {
			const PersonBlock & r_item = PersonBlkList.at(i);
			if(r_item.NativeID && r_item.ID == rBlk.ID) {
				p_ret = &r_item;
			}
		}
	}
	return p_ret;
}

SLAPI PPPosProtocol::QueryProcessBlock::QueryProcessBlock() : PosNodeID(0)
{
}

SLAPI PPPosProtocol::ProcessInputBlock::ProcessInputBlock()
{
	Helper_Construct();
}

SLAPI PPPosProtocol::ProcessInputBlock::ProcessInputBlock(PPAsyncCashSession * pAcs)
{
	Helper_Construct();
	P_ACS = pAcs;
}

void SLAPI PPPosProtocol::ProcessInputBlock::Helper_Construct()
{
	P_ACS = 0;
	P_RbList = 0;
	Flags = 0;
	PosNodeID = 0;
	SessionCount = 0;
	SessionPeriod.SetZero();
}

SLAPI PPPosProtocol::ProcessInputBlock::~ProcessInputBlock()
{
	if(P_RbList) {
		delete (TSCollection <PPPosProtocol::ReadBlock> *)P_RbList;
		P_RbList = 0;
	}
}

const void * SLAPI PPPosProtocol::ProcessInputBlock::GetStoredReadBlocks() const { return P_RbList; }

SLAPI PPPosProtocol::PPPosProtocol() : P_BObj(BillObj)
	{}
SLAPI PPPosProtocol::~PPPosProtocol()
	{}

const SString & FASTCALL PPPosProtocol::EncText(const char * pS)
{
	EncBuf = pS;
	PROFILE(XMLReplaceSpecSymb(EncBuf, "&<>\'"));
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

int SLAPI PPPosProtocol::WritePosNode(WriteBlock & rB, const char * pScopeXmlTag, PPCashNode & rInfo)
{
    int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
        w_s.PutInner("id", temp_buf.Z().Cat(rInfo.ID));
        w_s.PutInnerSkipEmpty("code", EncText(rInfo.Symb));
        w_s.PutInnerSkipEmpty("name", EncText(rInfo.Name));
	}
    return ok;
}

int SLAPI PPPosProtocol::WriteCSession(WriteBlock & rB, const char * pScopeXmlTag, const CSessionTbl::Rec & rInfo)
{
	int    ok = 1;
	PPID   src_ar_id = 0; // Статья аналитического учета, соответствующая источнику данных
	LDATETIME dtm;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", temp_buf.Z().Cat(rInfo.ID));
		w_s.PutInner("code", temp_buf.Z().Cat(rInfo.SessNumber));
		if(rInfo.CashNodeID) {
			//PPObjCashNode cn_obj;
			PPCashNode cn_rec;
			if(CnObj.Search(rInfo.CashNodeID, &cn_rec) > 0) {
				THROW(WritePosNode(rB, "pos", cn_rec));
			}
		}
		{
			dtm.Set(rInfo.Dt, rInfo.Tm);
			temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
			w_s.PutInner("time", EncText(temp_buf));
		}
		{
			PPIDArray cc_list;
			ArGoodsCodeArray ar_code_list;
            CCheckCore * p_cc = ScObj.P_CcTbl;
            if(p_cc) {
				CCheckPacket cc_pack;
				SCardTbl::Rec sc_rec;
                THROW(p_cc->GetListBySess(rInfo.ID, 0, cc_list));
				for(uint i = 0; i < cc_list.getCount(); i++) {
					const PPID cc_id = cc_list.get(i);
					cc_pack.Init();
					if(p_cc->LoadPacket(cc_id, 0, &cc_pack) > 0) {
						const double cc_amount = MONEYTOLDBL(cc_pack.Rec.Amount);
						const double cc_discount = MONEYTOLDBL(cc_pack.Rec.Discount);

						SXml::WNode w_cc(rB.P_Xw, "cc");
                        w_cc.PutInner("id",   temp_buf.Z().Cat(cc_pack.Rec.ID));
                        w_cc.PutInner("code", temp_buf.Z().Cat(cc_pack.Rec.Code));
						{
							dtm.Set(cc_pack.Rec.Dt, cc_pack.Rec.Tm);
							temp_buf.Z().Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
							w_cc.PutInner("time", EncText(temp_buf));
						}
						if(cc_pack.Rec.Flags) {
							temp_buf.Z().CatHex(cc_pack.Rec.Flags);
							w_cc.PutInner("flags", EncText(temp_buf));
						}
						if(cc_pack.Rec.Flags & CCHKF_RETURN) {
							w_cc.PutInner("return", "");
						}
						temp_buf.Z().Cat(cc_amount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
						w_cc.PutInner("amount", temp_buf);
						if(cc_discount != 0.0) {
							temp_buf.Z().Cat(cc_discount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
							w_cc.PutInner("discount", temp_buf);
						}
						if(cc_pack.Rec.SCardID && ScObj.Search(cc_pack.Rec.SCardID, &sc_rec) > 0) {
							SXml::WNode w_sc(rB.P_Xw, "card");
							w_sc.PutInner("code", EncText(sc_rec.Code));
						}
						for(uint ln_idx = 0; ln_idx < cc_pack.GetCount(); ln_idx++) {
							const CCheckLineTbl::Rec & r_item = cc_pack.GetLine(ln_idx);

							const double item_qtty  = fabs(r_item.Quantity);
							const double item_price = intmnytodbl(r_item.Price);
							const double item_discount = r_item.Dscnt;
							const double item_amount   = item_price * item_qtty;
							const double item_sum_discount = item_discount * item_qtty;

							SXml::WNode w_ccl(rB.P_Xw, "ccl");
                            w_ccl.PutInner("id", temp_buf.Z().Cat(r_item.RByCheck));
                            if(r_item.GoodsID) {
								Goods2Tbl::Rec goods_rec;
								if(GObj.Search(r_item.GoodsID, &goods_rec) > 0) {
									SXml::WNode w_w(rB.P_Xw, "ware");
									GObj.P_Tbl->ReadArCodesByAr(goods_rec.ID, src_ar_id, &ar_code_list);
									if(ar_code_list.getCount()) {
										w_w.PutInner("id", EncText(temp_buf.Z().Cat(ar_code_list.at(0).Code)));
									}
									w_w.PutInner("innerid", temp_buf.Z().Cat(goods_rec.ID));
                                    GObj.GetSingleBarcode(r_item.GoodsID, temp_buf);
                                    if(temp_buf.NotEmptyS()) {
										w_w.PutInner("code", EncText(temp_buf));
                                    }
								}
                            }
                            w_ccl.PutInner("qtty", temp_buf.Z().Cat(item_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
                            w_ccl.PutInner("price", temp_buf.Z().Cat(item_price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
                            if(item_discount != 0.0)
								w_ccl.PutInner("discount", temp_buf.Z().Cat(item_discount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
							w_ccl.PutInner("amount", temp_buf.Z().Cat(item_amount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
							if(item_sum_discount != 0.0)
								w_ccl.PutInner("sumdiscount", temp_buf.Z().Cat(item_sum_discount, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
						}
						{
							const CcAmountList & r_al = cc_pack.AL_Const();
							if(r_al.getCount()) {
								for(uint pi = 0; pi < r_al.getCount(); pi++) {
									const CcAmountEntry & r_ai = r_al.at(pi);
									SXml::WNode w_cp(rB.P_Xw, "payment");
									{
										temp_buf.Z().Cat(r_ai.Amount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
										w_cp.PutInner("amount", temp_buf);
										if(r_ai.Type == CCAMTTYP_CASH) {
											w_cp.PutInner("type", "cash");
										}
										else if(r_ai.Type == CCAMTTYP_BANK) {
											w_cp.PutInner("type", "bank");
										}
										else if(r_ai.Type == CCAMTTYP_CRDCARD) {
											w_cp.PutInner("type", "card");
											if(r_ai.AddedID && ScObj.Search(r_ai.AddedID, &sc_rec) > 0) {
												SXml::WNode w_sc(rB.P_Xw, "card");
												w_sc.PutInner("code", EncText(sc_rec.Code));
											}
										}
										else
											w_cp.PutInner("type", "unkn");
									}
								}
							}
							else {
								SXml::WNode w_cp(rB.P_Xw, "payment");
								temp_buf.Z().Cat(cc_amount, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								w_cp.PutInner("amount", temp_buf);
								if(cc_pack.Rec.Flags & CCHKF_BANKING) {
									w_cp.PutInner("type", "bank");
								}
								else if(cc_pack.Rec.Flags & CCHKF_INCORPCRD) {
									w_cp.PutInner("type", "card");
									if(cc_pack.Rec.SCardID && ScObj.Search(cc_pack.Rec.SCardID, &sc_rec) > 0) {
										SXml::WNode w_sc(rB.P_Xw, "card");
										w_sc.PutInner("code", EncText(sc_rec.Code));
									}
								}
								else {
									w_cp.PutInner("type", "cash");
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

int SLAPI PPPosProtocol::WriteGoodsGroupInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashGoodsGroupInfo & rInfo, const PPQuotArray * pQList)
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", temp_buf.Z().Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		if(rInfo.Code[0]) {
			temp_buf = rInfo.Code;
			w_s.PutInner("code", EncText(temp_buf));
		}
		if(rInfo.ParentID) {
			SXml::WNode w_p(rB.P_Xw, "parent");
			w_p.PutInner("id", temp_buf.Z().Cat(rInfo.ParentID));
		}
		if(pQList && pQList->getCount()) {
			for(uint i = 0; i < pQList->getCount(); i++) {
				WriteQuotInfo(rB, "quote", PPOBJ_GOODSGROUP, pQList->at(i));
			}
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::WriteRouteInfo(WriteBlock & rB, const char * pScopeXmlTag, const PPPosProtocol::RouteBlock & rInfo)
{
	int    ok = 1;
	if(!rInfo.IsEmpty()) {
		SString temp_buf;
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		PPVersionInfo vi = DS.GetVersionInfo();
		vi.GetProductName(temp_buf);
		w_s.PutInnerSkipEmpty("system", EncText(rInfo.System));
		w_s.PutInnerSkipEmpty("version", EncText(rInfo.Version));
		if(!rInfo.Uuid.IsZero()) {
			rInfo.Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
			w_s.PutInner("uuid", EncText(temp_buf));
		}
		w_s.PutInnerSkipEmpty("code", EncText(rInfo.Code));
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPPosProtocol::WriteGoodsInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashGoodsInfo & rInfo, const PPQuotArray * pQList)
{
	int    ok = 1;
	int    use_lookbackprices = 0;
	SString temp_buf;
	PrcssrAlcReport::GoodsItem agi;
	const  int is_spirit = BIN(rInfo.GdsClsID && rB.P_Acgi && rB.P_Acgi->GetAlcoGoodsExtension(rInfo.ID, 0, agi) > 0);
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", temp_buf.Z().Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		if(rInfo.P_CodeList && rInfo.P_CodeList->getCount()) {
			for(uint i = 0; i < rInfo.P_CodeList->getCount(); i++) {
				const BarcodeTbl::Rec & r_bc_rec = rInfo.P_CodeList->at(i);
				w_s.PutInner("code", EncText(r_bc_rec.Code));
			}
		}
		if(rInfo.ParentID) {
			SXml::WNode w_p(rB.P_Xw, "parent");
			w_p.PutInner("id", temp_buf.Z().Cat(rInfo.ParentID));
		}
		if(rInfo.UnitID) {
			PPUnit unit_rec;
			SXml::WNode w_u(rB.P_Xw, "unit");
			w_u.PutInner("id", temp_buf.Z().Cat(rInfo.UnitID));
			if(is_spirit && agi.Proof > 0.0 && agi.Volume > 0.0 && GObj.FetchUnit(PPUNT_LITER, &unit_rec) > 0) {
				//
				// Для алкоголя искусственно устанавливаем единицу измерения //
				//
				SXml::WNode w_pu(rB.P_Xw, "phunit");
				w_pu.PutInner("id", temp_buf.Z().Cat(PPUNT_LITER));
				w_pu.PutInner("ratio", temp_buf.Z().Cat(agi.Volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
			}
			else if(rInfo.PhUnitID) {
				SXml::WNode w_pu(rB.P_Xw, "phunit");
				w_pu.PutInner("id", temp_buf.Z().Cat(rInfo.PhUnitID));
				if(rInfo.PhUPerU > 0.0)
					w_pu.PutInner("ratio", temp_buf.Z().Cat(rInfo.PhUPerU, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
			}
		}
		if(rInfo.VatRate > 0.0) {
			w_s.PutInner("vatrate", temp_buf.Z().Cat(rInfo.VatRate, MKSFMTD(0, 1, NMBF_NOTRAILZ)));
		}
		if(is_spirit) {
			if(agi.Proof > 0.0)
				w_s.PutInner("alcoproof", temp_buf.Z().Cat(agi.Proof, MKSFMTD(0, 1, NMBF_NOTRAILZ)));
			if(agi.CategoryCode.NotEmpty())
				w_s.PutInner("alcorucat", agi.CategoryCode);
		}
		if(rInfo.GoodsTypeID) {
			PPGoodsType gt_rec;
			if(GObj.FetchGoodsType(rInfo.GoodsTypeID, &gt_rec) > 0) {
				if(gt_rec.Flags & GTF_UNLIMITED)
					w_s.PutInner("unlimited", 0);
				if(gt_rec.Flags & GTF_LOOKBACKPRICES) {
					w_s.PutInner("lookbackprices", 0);
					use_lookbackprices = 1;
				}
			}
		}
		if(rInfo.NoDis > 0) { // Значение <0 имеет специальный смысл
			w_s.PutInner("nodiscount", 0);
		}
		if(rInfo.Price > 0.0)
			w_s.PutInner("price", temp_buf.Z().Cat(rInfo.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
		if(rInfo.Rest > 0.0)
			w_s.PutInner("rest", temp_buf.Z().Cat(rInfo.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
        {
			//static
			/*
			int SLAPI AsyncCashGoodsIterator::__GetDifferentPricesForLookBackPeriod(PPID goodsID, PPID locID, double basePrice, int lookBackPeriod, RealArray & rList)
			{
				int    ok = -1;
				rList.clear();
				if(lookBackPeriod > 0) {
					// @v9.9.0 (unused) const double base_price = R3(basePrice);
					const LDATE cd = getcurdate_();
					const LDATE stop_date = plusdate(cd, -lookBackPeriod);
					ReceiptCore & r_rc = BillObj->trfr->Rcpt;
					ReceiptTbl::Rec lot_rec;
					LDATE   enm_dt = MAXDATE;
					long    enm_opn = MAXLONG;
					while(r_rc.EnumLastLots(goodsID, locID, &enm_dt, &enm_opn, &lot_rec) > 0 && enm_dt >= stop_date) {
						double price = R3(lot_rec.Price);
						uint   p = 0;
						if(price > 0.0 && price != basePrice && !rList.lsearch(&price, &p, PTR_CMPFUNC(double))) {
							rList.insert(&price);
							ok = 1;
						}
					}
			#endif // }
				}
				return ok;
			}
			*/
        	//
        	// Лоты
        	//
			ReceiptCore & r_rc = P_BObj->trfr->Rcpt;
			LotArray lot_list;
			LotArray lbp_lot_list; // Список лотов с отличными от rInfo.Price ценами
			const long lbpp = CsObj.GetEqCfg().LookBackPricePeriod;
			if(use_lookbackprices && lbpp) {
				// @v9.9.0 (unused) const double base_price = rInfo.Price;
				const LDATE cd = getcurdate_();
				const LDATE stop_date = plusdate(cd, -lbpp);
				RealArray list_of_diffprices;
				ReceiptTbl::Rec lot_rec;
				LDATE   enm_dt = MAXDATE;
				long    enm_opn = MAXLONG;
				while(r_rc.EnumLastLots(rInfo.ID, rB.LocID, &enm_dt, &enm_opn, &lot_rec) > 0 && enm_dt >= stop_date) {
					double price = R3(lot_rec.Price);
					if(price > 0.0 && price != rInfo.Price && !list_of_diffprices.lsearch(&price, 0, PTR_CMPFUNC(double))) {
						list_of_diffprices.insert(&price);
						lbp_lot_list.insert(&lot_rec);
					}
				}
			}
			r_rc.GetListOfOpenedLots(+1, rInfo.ID, rB.LocID, MAXDATE, &lot_list);
			if(lbp_lot_list.getCount()) {
				for(uint i = 0; i < lbp_lot_list.getCount(); i++) {
					const ReceiptTbl::Rec & r_lot_rec = lbp_lot_list.at(i);
					if(!lot_list.lsearch(&r_lot_rec.ID, 0, CMPF_LONG))
						lot_list.insert(&r_lot_rec);
				}
			}
			lot_list.sort(PTR_CMPFUNC(Receipt_DtOprNo_Asc));
			{
                for(uint i = 0; i < lot_list.getCount(); i++) {
					const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
					SXml::WNode w_l(rB.P_Xw, "lot");
					w_l.PutInnerValidDate("date", r_lot_rec.Dt, DATF_ISO8601|DATF_CENTURY);
					w_l.PutInnerValidDate("expiry", r_lot_rec.Expiry, DATF_ISO8601|DATF_CENTURY);
					if(r_lot_rec.Cost > 0.0)
						w_l.PutInner("cost", temp_buf.Z().Cat(r_lot_rec.Cost, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
					if(r_lot_rec.Price > 0.0)
						w_l.PutInner("price", temp_buf.Z().Cat(r_lot_rec.Price, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
					if(r_lot_rec.Rest > 0.0)
						w_l.PutInner("rest", temp_buf.Z().Cat(r_lot_rec.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
					if(P_BObj->GetSerialNumberByLot(r_lot_rec.ID, temp_buf, 1) > 0)
						w_l.PutInnerSkipEmpty("serial", EncText(temp_buf));
                }
			}
        }
		if(pQList && pQList->getCount()) {
			for(uint i = 0; i < pQList->getCount(); i++) {
				WriteQuotInfo(rB, "quote", PPOBJ_GOODS, pQList->at(i));
			}
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::WriteQuotKindInfo(WriteBlock & rB, const char * pScopeXmlTag, const PPQuotKind & rInfo)
{
    int    ok = 1;
    SString temp_buf;
	TimeRange tr;
    {
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", temp_buf.Z().Cat(rInfo.ID));
		w_s.PutInner("name", EncText(rInfo.Name));
		w_s.PutInnerSkipEmpty("code", EncText(rInfo.Symb));
		w_s.PutInner("rank", temp_buf.Z().Cat(rInfo.Rank));
		if(!rInfo.Period.IsZero() || rInfo.HasWeekDayRestriction() || rInfo.GetTimeRange(tr) > 0 || !rInfo.AmtRestr.IsZero()) {
			SXml::WNode w_r(rB.P_Xw, "restriction");
			if(!rInfo.Period.IsZero()) {
				w_r.PutInner("period", EncText(temp_buf.Z().Cat(rInfo.Period, 0)));
			}
			if(rInfo.HasWeekDayRestriction()) {
				temp_buf.Z();
				for(uint d = 0; d < 7; d++) {
					temp_buf.CatChar((rInfo.DaysOfWeek & (1 << d)) ? '1' : '0');
				}
				w_r.PutInner("weekday", EncText(temp_buf));
			}
			if(rInfo.GetTimeRange(tr) > 0) {
				SXml::WNode w_t(rB.P_Xw, "timerange");
				w_t.PutInner("low", temp_buf.Z().Cat(tr.low, TIMF_HMS));
				w_t.PutInner("upp", temp_buf.Z().Cat(tr.upp, TIMF_HMS));
			}
			if(!rInfo.AmtRestr.IsZero()) {
				SXml::WNode w_a(rB.P_Xw, "amountrange");
				if(rInfo.AmtRestr.low)
					w_a.PutInner("low", temp_buf.Z().Cat(rInfo.AmtRestr.low));
				if(rInfo.AmtRestr.upp)
					w_a.PutInner("upp", temp_buf.Z().Cat(rInfo.AmtRestr.upp));
			}
		}
    }
    return ok;
}

int SLAPI PPPosProtocol::WriteQuotInfo(WriteBlock & rB, const char * pScopeXmlTag, PPID parentObj, const PPQuot & rInfo)
{
    int    ok = -1;
	SString temp_buf;
	if(rInfo.Kind && rInfo.GoodsID) {
		PPQuotKind qk_rec;
		Goods2Tbl::Rec goods_rec;
		if(QkObj.Fetch(rInfo.Kind, &qk_rec) > 0 && GObj.Fetch(rInfo.GoodsID, &goods_rec) > 0) {
			if(goods_rec.Kind == PPGDSK_GOODS || (goods_rec.Kind == PPGDSK_GROUP && !(goods_rec.Flags & GF_ALTGROUP))) {
				SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
				if(parentObj != PPOBJ_QUOTKIND) {
					SXml::WNode w_k(rB.P_Xw, "kind");
					w_k.PutInner("id", temp_buf.Z().Cat(rInfo.Kind));
				}
				if(goods_rec.Kind == PPGDSK_GOODS && parentObj != PPOBJ_GOODS) {
					SXml::WNode w_g(rB.P_Xw, "ware");
					w_g.PutInner("id", temp_buf.Z().Cat(rInfo.GoodsID));
				}
				else if(goods_rec.Kind == PPGDSK_GROUP && parentObj != PPOBJ_GOODSGROUP) {
					SXml::WNode w_g(rB.P_Xw, "goodsgroup");
					w_g.PutInner("id", temp_buf.Z().Cat(rInfo.GoodsID));
				}
				if(rInfo.MinQtty > 0) {
					w_s.PutInner("minqtty", temp_buf.Z().Cat(rInfo.MinQtty));
				}
				if(!rInfo.Period.IsZero()) {
					w_s.PutInner("period", EncText(temp_buf.Z().Cat(rInfo.Period, 0)));
				}
				w_s.PutInner("value", EncText(rInfo.PutValToStr(temp_buf)));
				ok = 1;
			}
		}
	}
    return ok;
}

int SLAPI PPPosProtocol::WritePersonInfo(WriteBlock & rB, const char * pScopeXmlTag, PPID codeRegTypeID, const PPPersonPacket & rPack)
{
    int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		w_s.PutInner("id", temp_buf.Z().Cat(rPack.Rec.ID));
		w_s.PutInner("name", EncText(rPack.Rec.Name));
		if(codeRegTypeID) {
            rPack.GetRegNumber(codeRegTypeID, temp_buf);
            if(temp_buf.NotEmptyS()) {
				w_s.PutInner("code", EncText(temp_buf));
            }
		}
	}
    return ok;
}

int SLAPI PPPosProtocol::WriteSCardInfo(WriteBlock & rB, const char * pScopeXmlTag, const AsyncCashSCardInfo & rInfo)
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode w_s(rB.P_Xw, pScopeXmlTag);
		const int sc_type = 0; // @todo
		w_s.PutInner("id", temp_buf.Z().Cat(rInfo.Rec.ID));
		w_s.PutInner("code", EncText(rInfo.Rec.Code));
		/*
		if(rInfo.Rec.SeriesID) {
			SXml::WNode w_r(rB.P_Xw, "series");
			w_r.PutInner("id", temp_buf.Z().Cat(rInfo.Rec.SeriesID));
		}
		*/
		if(rInfo.Rec.PersonID) {
			PPPersonPacket psn_pack;
			if(PsnObj.GetPacket(rInfo.Rec.PersonID, &psn_pack, 0) > 0) {
				PPID   reg_typ_id = 0;
				PPSCardConfig sc_cfg;
				PPObjSCard::FetchConfig(&sc_cfg);
				if(sc_cfg.PersonKindID) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(sc_cfg.PersonKindID, &pk_rec) > 0)
						reg_typ_id = pk_rec.CodeRegTypeID;
				}
				THROW(WritePersonInfo(rB, "owner", reg_typ_id, psn_pack));
			}
		}
		w_s.PutInnerValidDate("expiry", rInfo.Rec.Expiry, DATF_ISO8601|DATF_CENTURY);
		if(rInfo.Rec.PDis > 0)
			w_s.PutInner("discount", temp_buf.Z().Cat(fdiv100i(rInfo.Rec.PDis), MKSFMTD(0, 2, NMBF_NOTRAILZ)));
		//if(rInfo.P_QuotByQttyList)
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::StartWriting(const char * pFileName, PPPosProtocol::WriteBlock & rB)
{
	int    ok = 1;
	SString temp_buf;
	THROW_LXML(rB.P_Xw = xmlNewTextWriterFilename(pFileName, 9 /*compression*/), 0);
	xmlTextWriterSetIndent(rB.P_Xw, 1);
	xmlTextWriterSetIndentString(rB.P_Xw, (const xmlChar*)"\t");
	THROW_MEM(rB.P_Xd = new SXml::WDoc(rB.P_Xw, cpUTF8));
	rB.P_Root = new SXml::WNode(rB.P_Xw, "PapyrusAsyncPosInterchange");
	// @v9.9.12 {
	{
		SXml::WNode w_s(rB.P_Xw, "file");
		rB.FileUUID.ToStr(S_GUID::fmtIDL, temp_buf);
		w_s.PutInner("uuid", EncText(temp_buf));
		rB.FileDtm = getcurdatetime_();
		temp_buf.Z().Cat(rB.FileDtm, DATF_ISO8601|DATF_CENTURY, 0);
		w_s.PutInner("timestamp", temp_buf);
	}
	// } @v9.9.12 
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::FinishWriting(PPPosProtocol::WriteBlock & rB)
{
	rB.Destroy();
	return 1;
}

//static
void PPPosProtocol::Scb_StartDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, StartDocument());
}

//static
void PPPosProtocol::Scb_EndDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, EndDocument());
}

//static
void PPPosProtocol::Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, StartElement((const char *)pName, (const char **)ppAttrList));
}

//static
void PPPosProtocol::Scb_EndElement(void * ptr, const xmlChar * pName)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, EndElement((const char *)pName));
}

//static
void PPPosProtocol::Scb_Characters(void * ptr, const uchar * pC, int len)
{
	CALLTYPEPTRMEMB(PPPosProtocol, ptr, Characters((const char *)pC, len));
}

int PPPosProtocol::StartDocument()
{
	RdB.TagValue.Z();
	return 1;
}

int PPPosProtocol::EndDocument()
{
	return 1;
}

int FASTCALL PPPosProtocol::Helper_PushQuery(int queryType)
{
	int    ok = 1;
	uint   ref_pos = 0;
	assert(oneof2(queryType, QueryBlock::qCSession, QueryBlock::qRefs));
	THROW(RdB.CreateItem(obQuery, &ref_pos));
	RdB.RefPosStack.push(ref_pos);
	{
		QueryBlock * p_item = (QueryBlock *)RdB.GetItemWithTest(ref_pos, obQuery);
		p_item->Q = queryType;
	}
	CATCHZOK
	return ok;
}

PPPosProtocol::QueryBlock * PPPosProtocol::Helper_RenewQuery(uint & rRefPos, int queryType)
{
	QueryBlock * p_blk = 0;
	RdB.RefPosStack.pop(rRefPos);
	THROW(Helper_PushQuery(queryType));
	rRefPos = PeekRefPos();
	p_blk = (QueryBlock *)RdB.GetItemWithTest(rRefPos, obQuery);
	assert(p_blk && p_blk->Q == queryType);
	CATCH
		p_blk = 0;
	ENDCATCH
	return p_blk;
}

int PPPosProtocol::StartElement(const char * pName, const char ** ppAttrList)
{
	/*
			stRefsOccured   = 0x0004,
			stQueryOccured  = 0x0008,
			stCSessOccured  = 0x0010
	*/
	int    ok = 1;
	uint   ref_pos = 0;
    (RdB.TempBuf = pName).ToLower();
    int    tok = 0;
    if(RdB.P_ShT) {
		uint _ut = 0;
		RdB.P_ShT->Search(RdB.TempBuf, &_ut, 0);
		tok = _ut;
    }
    if(tok == PPHS_PPPP_START) {
		RdB.State |= RdB.stHeaderOccured;
	}
	else {
		THROW_PP(RdB.State & RdB.stHeaderOccured, PPERR_FILEISNTPPAPI);
		if(RdB.Phase == RdB.phPreprocess) {
			switch(tok) {
				case PPHS_SOURCE:
					THROW(RdB.CreateItem(obSource, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_DESTINATION:
					THROW(RdB.CreateItem(obDestination, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_QUERYCSESS: RdB.State |= RdB.stQueryOccured; break;
				case PPHS_QUERYREFS:  RdB.State |= RdB.stQueryOccured; break;
				case PPHS_REFS:       RdB.State |= RdB.stRefsOccured;  break;
				case PPHS_CSESSION:   RdB.State |= RdB.stCSessOccured; break;
			}
		}
		else if(RdB.Phase == RdB.phProcess) {
			const uint link_ref_pos = PeekRefPos();
			int    link_type = 0;
			switch(tok) {
				case PPHS_SOURCE:
					THROW(RdB.CreateItem(obSource, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_DESTINATION:
					THROW(RdB.CreateItem(obDestination, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_QUERYCSESS:
					RdB.State |= RdB.stQueryOccured;
					THROW(Helper_PushQuery(QueryBlock::qCSession));
					break;
				case PPHS_QUERYREFS:
					RdB.State |= RdB.stQueryOccured;
					THROW(Helper_PushQuery(QueryBlock::qRefs));
					break;
				case PPHS_POS:
					THROW(RdB.CreateItem(obPosNode, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_WARE:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obGoods, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCcLine) {
							GoodsBlock * p_item = (GoodsBlock *)RdB.GetItemWithTest(ref_pos, obGoods);
							p_item->Flags_ |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_GOODSGROUP:
					THROW(RdB.CreateItem(obGoodsGroup, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_UNIT:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obUnit, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obGoods) {
							UnitBlock * p_item = (UnitBlock *)RdB.GetItemWithTest(ref_pos, obUnit);
							p_item->Flags_ |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_PHUNIT:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obUnit, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(oneof2(link_type, obUnit, obGoods)) {
							UnitBlock * p_item = (UnitBlock *)RdB.GetItemWithTest(ref_pos, obUnit);
							p_item->Flags_ |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_LOT:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obLot, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						{
							LotBlock * p_item = (LotBlock *)RdB.GetItemWithTest(ref_pos, obLot);
							if(link_type == obGoods)
								p_item->GoodsBlkP = link_ref_pos; // Лот ссылается на позицию товара, которому принадлежит
						}
					}
					break;
				case PPHS_SCARDSERIES:
				case PPHS_SERIES: // Сокращение для scardseries. Допустимо использовать только для определение ссылки персональной карты на серию.
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obSCardSeries, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						{
							SCardSeriesBlock * p_item = (SCardSeriesBlock *)RdB.GetItemWithTest(ref_pos, obSCardSeries);
							p_item->RefP = ref_pos;
							if(link_type == obSCard) {
								p_item->Flags_ |= ObjectBlock::fRefItem;
							}
						}
					}
					break;
				case PPHS_CARD:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obSCard, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						SCardBlock * p_item = (SCardBlock *)RdB.GetItemWithTest(ref_pos, obSCard);
						if(oneof2(link_type, obCCheck, obPayment)) {
							p_item->Flags_ |= ObjectBlock::fRefItem;
						}
						else if(link_type == obSCardSeries) {
							p_item->SeriesBlkP = link_ref_pos;
						}
					}
					break;
				case PPHS_QUOTE:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obQuot, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						{
							QuotBlock * p_item = (QuotBlock *)RdB.GetItemWithTest(ref_pos, obQuot);
							if(link_type == obGoods) {
								p_item->GoodsBlkP = link_ref_pos; // Котировка ссылается на позицию товара, которому принадлежит
							}
							else if(link_type == obGoodsGroup) {
								p_item->BlkFlags |= p_item->fGroup;
								p_item->GoodsGroupBlkP = link_ref_pos; // Котировка ссылается на позицию товарной группы, которой принадлежит
							}
						}
					}
					break;
				case PPHS_KIND:
					{
						int  type = 0;
						void * p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuot) {
							uint   qk_ref_pos = 0;
							THROW(RdB.CreateItem(obQuotKind, &qk_ref_pos));
							{
								QuotKindBlock * p_qk_blk = (QuotKindBlock *)RdB.GetItemWithTest(qk_ref_pos, obQuotKind);
								p_qk_blk->Flags_ |= ObjectBlock::fRefItem;
							}
							RdB.RefPosStack.push(qk_ref_pos);
						}
					}
					break;
				case PPHS_QUOTEKIND:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obQuotKind, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obSCardSeries) {
							QuotKindBlock * p_item = (QuotKindBlock *)RdB.GetItemWithTest(ref_pos, obQuotKind);
							p_item->Flags_ |= ObjectBlock::fRefItem;
						}
					}
					break;
				case PPHS_CSESSION:
					RdB.State |= RdB.stCSessOccured;
					THROW(RdB.CreateItem(obCSession, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_CC:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obCCheck, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						assert(link_type == obCSession);
						if(link_type == obCSession) {
							CCheckBlock * p_item = (CCheckBlock *)RdB.GetItemWithTest(ref_pos, obCCheck);
							p_item->CSessionBlkP = link_ref_pos; // Чек ссылается на позицию сессии, которой принадлежит
							assert(p_item->CSessionBlkP);
						}
					}
					break;
				case PPHS_CCL:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obCcLine, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCCheck) {
							CcLineBlock * p_item = (CcLineBlock *)RdB.GetItemWithTest(ref_pos, obCcLine);
							p_item->CCheckBlkP = link_ref_pos; // Строка ссылается на позицию чека, которому принадлежит
						}
					}
					break;
				case PPHS_PAYMENT:
					{
						void * p_link_item = RdB.GetItem(link_ref_pos, &link_type);
						THROW(RdB.CreateItem(obPayment, &ref_pos));
						RdB.RefPosStack.push(ref_pos);
						if(link_type == obCCheck) {
							CcPaymentBlock * p_item = (CcPaymentBlock *)RdB.GetItemWithTest(ref_pos, obPayment);
							p_item->CCheckBlkP = link_ref_pos; // Оплата ссылается на позицию чека, которому принадлежит
						}
					}
					break;
				case PPHS_OWNER:
					THROW(RdB.CreateItem(obPerson, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_PARENT:
					THROW(RdB.CreateItem(obParent, &ref_pos));
					RdB.RefPosStack.push(ref_pos);
					break;
				case PPHS_CODE:
					{
						int  type = 0;
						void * p_item = PeekRefItem(&ref_pos, &type);
						if(type == obGoods) {
							uint   goods_code_ref_pos = 0;
							THROW(RdB.CreateItem(obGoodsCode, &goods_code_ref_pos));
							RdB.RefPosStack.push(goods_code_ref_pos);
							{
								GoodsCode * p_item = (GoodsCode *)RdB.GetItemWithTest(goods_code_ref_pos, obGoodsCode);
								p_item->GoodsBlkP = ref_pos; // Код ссылается на позицию товара, которому принадлежит
							}
						}
					}
					break;
				case PPHS_REFS:
					RdB.State |= RdB.stRefsOccured;
					break;
				case PPHS_SYSTEM:
				case PPHS_VERSION:
				case PPHS_UUID:
				case PPHS_TIME:
				case PPHS_TIMESTAMP: // @v9.9.12
				case PPHS_ID:
				case PPHS_NAME:
				case PPHS_RANK:
				case PPHS_PRICE:
				case PPHS_DISCOUNT:
				case PPHS_RETURN:
				case PPHS_RESTRICTION:
				case PPHS_PERIOD:
				case PPHS_WEEKDAY:
				case PPHS_TIMERANGE:
				case PPHS_AMOUNTRANGE:
				case PPHS_LOW:
				case PPHS_UPP:
				case PPHS_VALUE:
				case PPHS_EXPIRY: // lot
				case PPHS_FLAGS:
				case PPHS_AMOUNT:
				case PPHS_QTTY:
				case PPHS_SUMDISCOUNT:
				case PPHS_INNERID:
				case PPHS_OBJ:
				case PPHS_LAST:
				case PPHS_CURRENT:
				case PPHS_DATE: // log
				case PPHS_COST: // lot
				case PPHS_SERIAL: // lot
				case PPHS_TYPE:
				case PPHS_PHYSICAL: // unit
				case PPHS_BASE: // unit
				case PPHS_BASERATIO: // unit
				case PPHS_ROUNDING: // unit
				case PPHS_RATIO: // unit
				case PPHS_DEFAULT: // unit
				case PPHS_NODISCOUNT: // goods
				case PPHS_ALCOPROOF:
				case PPHS_ALCORUCAT:
				case PPHS_VATRATE:
				case PPHS_SALESTAXRATE:
				case PPHS_UNLIMITED:
				case PPHS_LOOKBACKPRICES:
					break;
			}
		}
	}
	RdB.TokPath.push(tok);
	RdB.TagValue.Z();
	CATCH
		SaxStop();
		RdB.State |= RdB.stError;
		ok = 0;
	ENDCATCH
    return ok;
}

void FASTCALL PPPosProtocol::Helper_AddStringToPool(uint * pPos)
{
	if(RdB.TagValue.NotEmptyS())
		RdB.AddS(RdB.TagValue, pPos);
}

uint SLAPI PPPosProtocol::PeekRefPos() const
{
	void * p_test = RdB.RefPosStack.SStack::peek();
	return p_test ? *(uint *)p_test : UINT_MAX;
}

void * SLAPI PPPosProtocol::PeekRefItem(uint * pRefPos, int * pType) const
{
	void * p_result = 0;
	void * p_test = RdB.RefPosStack.SStack::peek();
	if(p_test) {
		const uint ref_pos = RdB.RefPosStack.peek();
		ASSIGN_PTR(pRefPos, ref_pos);
		p_result = RdB.GetItem(ref_pos, pType);
	}
	else {
		ASSIGN_PTR(pRefPos, UINT_MAX);
	}
	return p_result;
}

int PPPosProtocol::EndElement(const char * pName)
{
	int    tok = 0;
	int    ok = RdB.TokPath.pop(tok);
	uint   ref_pos = 0;
	uint   parent_ref_pos = 0;
	void * p_item = 0;
	int    type = 0;
	int    is_processed = 1;
	if(RdB.Phase == RdB.phPreprocess) {
		switch(tok) {
			case PPHS_FILE:
				break;
			case PPHS_PPPP_START:
				RdB.State &= ~RdB.stHeaderOccured;
				break;
			case PPHS_CODE:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obSource:
					case obDestination: Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->CodeP); break;
						break;
				}
				break;
			case PPHS_SYSTEM:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->SystemP);
				}
				break;
			case PPHS_VERSION:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->VersionP);
				}
				break;
			case PPHS_TIMESTAMP:
				{
					const int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_FILE) {
						if(RdB.TagValue.NotEmptyS()) {
							strtodatetime(RdB.TagValue, &RdB.SrcFileDtm, DATF_ISO8601, 0);
						}
					}
				}
				break;
			case PPHS_UUID:
				{
					const int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_FILE) {
						if(RdB.TagValue.NotEmptyS()) {
							THROW_SL(RdB.SrcFileUUID.FromStr(RdB.TagValue));
						}
					}
					else {
						p_item = PeekRefItem(&ref_pos, &type);
						if(oneof2(type, obSource, obDestination)) {
							if(RdB.TagValue.NotEmptyS()) {
								S_GUID uuid;
								THROW_SL(uuid.FromStr(RdB.TagValue));
								((RouteObjectBlock *)p_item)->Uuid = uuid;
							}
						}
					}
				}
				break;
			case PPHS_SOURCE:
			case PPHS_DESTINATION:
				RdB.RefPosStack.pop(ref_pos);
				break;
			default:
				is_processed = 0;
				break;
		}
	}
	else if(RdB.Phase == RdB.phProcess) {
		switch(tok) {
			case PPHS_TIMESTAMP:
				{
					const int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_FILE) {
						if(RdB.TagValue.NotEmptyS()) {
							strtodatetime(RdB.TagValue, &RdB.SrcFileDtm, DATF_ISO8601, 0);
						}
					}
				}
				break;
			case PPHS_UUID:
				{
					const int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_FILE) {
						if(RdB.TagValue.NotEmptyS()) {
							THROW_SL(RdB.SrcFileUUID.FromStr(RdB.TagValue));
						}
					}
					else {
						p_item = PeekRefItem(&ref_pos, &type);
						if(oneof2(type, obSource, obDestination)) {
							if(RdB.TagValue.NotEmptyS()) {
								S_GUID uuid;
								THROW_SL(uuid.FromStr(RdB.TagValue));
								((RouteObjectBlock *)p_item)->Uuid = uuid;
							}
						}
					}
				}
				break;
			case PPHS_PPPP_START:
				RdB.State &= ~RdB.stHeaderOccured;
				break;
			case PPHS_OBJ:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qRefs) {
						if(RdB.TagValue.NotEmptyS()) {
							long   obj_type_ext = 0;
							PPID   obj_type = DS.GetObjectTypeBySymb(RdB.TagValue, &obj_type_ext);
							if(obj_type) {
								if(p_blk->ObjType) {
									THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qRefs));
								}
								p_blk->ObjType = obj_type;
							}
						}
					}
				}
				break;
			case PPHS_LAST:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qCSession) {
						if(p_blk->CSess || !p_blk->Period.IsZero()) {
							THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
						}
						p_blk->Flags |= QueryBlock::fCSessLast;
					}
				}
				break;
			case PPHS_CURRENT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obQuery) {
					QueryBlock * p_blk = (QueryBlock *)p_item;
					if(p_blk->Q == QueryBlock::qCSession) {
						if(p_blk->CSess || !p_blk->Period.IsZero()) {
							THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
						}
						p_blk->Flags |= QueryBlock::fCSessCurrent;
					}
				}
				break;
			case PPHS_POS:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCSession) {
					((CSessionBlock *)p_item)->PosBlkP = ref_pos;
				}
				break;
			case PPHS_WARE:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCcLine) {
					((CcLineBlock *)p_item)->GoodsBlkP = ref_pos;
				}
				break;
			case PPHS_UNIT:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obGoods) {
					((GoodsBlock *)p_item)->UnitBlkP = ref_pos;
				}
				break;
			case PPHS_PHUNIT:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obUnit) {
					((UnitBlock *)p_item)->PhUnitBlkP = ref_pos;
				}
				else if(type == obGoods) {
					((GoodsBlock *)p_item)->PhUnitBlkP = ref_pos;
				}
				break;
			case PPHS_QUOTE:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obGoods) {
					((SCardBlock *)p_item)->OwnerBlkP = ref_pos;
				}
				else if(type == obGoodsGroup) {

				}
				break;
			case PPHS_CARD:
				RdB.RefPosStack.pop(ref_pos);
				//
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obCCheck) {
					((CCheckBlock *)p_item)->SCardBlkP = ref_pos;
				}
				else if(type == obPayment) {
					((CcPaymentBlock *)p_item)->SCardBlkP = ref_pos;
				}
				break;
			case PPHS_QUOTEKIND:
			case PPHS_QUOTKIND:
			case PPHS_KIND:
				{
					RdB.RefPosStack.pop(ref_pos);
					int    this_type = 0;
					void * p_this_item = RdB.GetItem(ref_pos, &this_type);
					if(this_type == obQuotKind) {
						//
						// Пытаемся найти аналогичный вид котировки. Если успешно, то принятый блок удаляем
						// и используем найденный аналог
						//
						if(ref_pos == (RdB.RefList.getCount()-1)) {
							const ObjBlockRef obr = RdB.RefList.at(ref_pos); // note для obr нельзя применять ссылку - элемент может быть удален ниже
							assert(obr.Type == obQuotKind);
							if(obr.P == (RdB.QkBlkList.getCount()-1)) {
								uint   other_ref_pos = 0;
								if(RdB.SearchAnalogRef_QuotKind(*(QuotKindBlock *)p_this_item, ref_pos, &other_ref_pos)) {
									RdB.RefList.atFree(ref_pos);
									RdB.QkBlkList.atFree(obr.P);
									ref_pos = other_ref_pos;
								}
							}
						}
						parent_ref_pos = PeekRefPos();
						p_item = RdB.GetItem(parent_ref_pos, &type);
						if(type == obQuot) {
							((QuotBlock *)p_item)->QuotKindBlkP = ref_pos;
						}
						else if(type == obSCardSeries) {
							((SCardSeriesBlock *)p_item)->QuotKindBlkP = ref_pos;
						}
					}
				}
				break;
			case PPHS_SCARDSERIES:
			case PPHS_SERIES:
				{
					RdB.RefPosStack.pop(ref_pos);
					int    this_type = 0;
					void * p_this_item = RdB.GetItem(ref_pos, &this_type);
					if(this_type == obSCardSeries) {
						//
						// Пытаемся найти аналогичную серию. Если успешно, то принятый блок удаляем и используем найденный аналог
						//
						if(ref_pos == (RdB.RefList.getCount()-1)) {
							const ObjBlockRef obr = RdB.RefList.at(ref_pos); // note для obr нельзя применять ссылку - элемент может быть удален ниже
							assert(obr.Type == obSCardSeries);
							if(obr.P == (RdB.ScsBlkList.getCount()-1)) {
								uint   other_ref_pos = 0;
								if(RdB.SearchAnalogRef_SCardSeries(*(SCardSeriesBlock *)p_this_item, ref_pos, &other_ref_pos)) {
									RdB.RefList.atFree(ref_pos);
									RdB.ScsBlkList.atFree(obr.P);
									ref_pos = other_ref_pos;
								}
							}
						}
						parent_ref_pos = PeekRefPos();
						p_item = RdB.GetItem(parent_ref_pos, &type);
						if(type == obSCard) {
							((SCardBlock *)p_item)->SeriesBlkP = ref_pos;
						}
					}
				}
				break;
			case PPHS_OWNER:
				RdB.RefPosStack.pop(ref_pos);
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obSCard) {
					((SCardBlock *)p_item)->OwnerBlkP = ref_pos;
				}
				break;
			case PPHS_PARENT:
				RdB.RefPosStack.pop(ref_pos);
				parent_ref_pos = PeekRefPos();
				p_item = RdB.GetItem(parent_ref_pos, &type);
				if(type == obGoods) {
					((GoodsBlock *)p_item)->ParentBlkP = ref_pos;
				}
				else if(type == obGoodsGroup) {
					((GoodsGroupBlock *)p_item)->ParentBlkP = ref_pos;
				}
				break;
			case PPHS_TYPE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obPayment) {
					if(RdB.TagValue.CmpNC("cash") == 0) {
						((CcPaymentBlock *)p_item)->PaymType = CCAMTTYP_CASH;
					}
					else if(RdB.TagValue.CmpNC("bank") == 0) {
						((CcPaymentBlock *)p_item)->PaymType = CCAMTTYP_BANK;
					}
					else if(RdB.TagValue.CmpNC("card") == 0) {
						((CcPaymentBlock *)p_item)->PaymType = CCAMTTYP_CRDCARD;
					}
				}
				break;
			case PPHS_PERIOD:
				{
					DateRange period;
					if(strtoperiod(RdB.TagValue, &period, 0)) {
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind) {
							((QuotKindBlock *)p_item)->Period = period;
						}
						else if(type == obQuery) {
							QueryBlock * p_blk = (QueryBlock *)p_item;
							p_blk->Period = period;
							if(p_blk->Q == QueryBlock::qCSession) {
								if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
									THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
								}
								p_blk->Period = period;
							}
						}
					}
				}
				break;
			case PPHS_LOW:
				{
					int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_TIMERANGE) {
						LTIME   t = ZEROTIME;
						if(strtotime(RdB.TagValue, TIMF_HMS, &t)) {
							p_item = PeekRefItem(&ref_pos, &type);
							if(type == obQuotKind)
								((QuotKindBlock *)p_item)->TimeRestriction.low = t;
						}
					}
					else if(prev_tok == PPHS_AMOUNTRANGE) {
						double v = RdB.TagValue.ToReal();
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind)
							((QuotKindBlock *)p_item)->AmountRestriction.low = v;
					}
				}
				break;
			case PPHS_UPP:
				{
					int prev_tok = RdB.TokPath.peek();
					if(prev_tok == PPHS_TIMERANGE) {
						LTIME   t = ZEROTIME;
						if(strtotime(RdB.TagValue, TIMF_HMS, &t)) {
							p_item = PeekRefItem(&ref_pos, &type);
							if(type == obQuotKind)
								((QuotKindBlock *)p_item)->TimeRestriction.upp = t;
						}
					}
					else if(prev_tok == PPHS_AMOUNTRANGE) {
						double v = RdB.TagValue.ToReal();
						p_item = PeekRefItem(&ref_pos, &type);
						if(type == obQuotKind)
							((QuotKindBlock *)p_item)->AmountRestriction.upp = v;
					}
				}
				break;
			case PPHS_ID:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obPosNode: ((PosNodeBlock *)p_item)->ID = _val_id; break;
						case obGoods: ((GoodsBlock *)p_item)->ID = _val_id; break;
						case obGoodsGroup: ((GoodsGroupBlock *)p_item)->ID = _val_id; break;
						case obPerson: ((PersonBlock *)p_item)->ID = _val_id; break;
						case obSCardSeries: ((SCardSeriesBlock *)p_item)->ID = _val_id; break;
						case obSCard: ((SCardBlock *)p_item)->ID = _val_id; break;
						case obParent: ((ParentBlock *)p_item)->ID = _val_id; break;
						case obQuotKind: ((QuotKindBlock *)p_item)->ID = _val_id; break;
						case obUnit: ((UnitBlock *)p_item)->ID = _val_id; break; // @v9.8.6
						case obCSession: ((CSessionBlock *)p_item)->ID = _val_id; break;
						case obCCheck: ((CCheckBlock *)p_item)->ID = _val_id; break;
						case obCcLine: ((CcLineBlock *)p_item)->RByCheck = _val_id; break;
						case obQuery:
							{
								QueryBlock * p_blk = (QueryBlock *)p_item;
								if(p_blk->Q == QueryBlock::qCSession) {
									if(_val_id) {
										if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
											THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
										}
										p_blk->CSess = _val_id;
										p_blk->Flags &= ~QueryBlock::fCSessN;
									}
								}
							}
							break;
					}
				}
				break;
			case PPHS_INNERID:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obGoods) {
						((GoodsBlock *)p_item)->InnerId = _val_id;
					}
				}
				break;
			case PPHS_VALUE:
				{
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obQuot:
							{
								PPQuot temp_q;
								temp_q.GetValFromStr(RdB.TagValue);
								((QuotBlock *)p_item)->Value = temp_q.Quot;
								((QuotBlock *)p_item)->QuotFlags = temp_q.Flags;
							}
							break;
					}
				}
				break;
			case PPHS_RANK:
				{
					const long _val_id = RdB.TagValue.ToLong();
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obQuotKind:
							((QuotKindBlock *)p_item)->Rank = (int16)_val_id;
							break;
					}
				}
				break;
			case PPHS_NAME:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obPosNode: Helper_AddStringToPool(&((PosNodeBlock *)p_item)->NameP); break;
					case obGoods:   Helper_AddStringToPool(&((GoodsBlock *)p_item)->NameP); break;
					case obGoodsGroup: Helper_AddStringToPool(&((GoodsGroupBlock *)p_item)->NameP); break;
					case obPerson: Helper_AddStringToPool(&((PersonBlock *)p_item)->NameP); break;
					case obSCardSeries: Helper_AddStringToPool(&((SCardSeriesBlock *)p_item)->NameP); break;
					case obSCard: break;
					case obQuotKind: Helper_AddStringToPool(&((QuotKindBlock *)p_item)->NameP); break;
					case obUnit: Helper_AddStringToPool(&((UnitBlock *)p_item)->NameP); break; // @v9.8.6
				}
				break;
			case PPHS_NODISCOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					if(RdB.IsTagValueBoolTrue())
						((GoodsBlock *)p_item)->GoodsFlags |= GF_NODISCOUNT;
				}
				break;
			case PPHS_VATRATE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					const double _value = RdB.TagValue.ToReal();
					((GoodsBlock *)p_item)->VatRate = dbltoint2(_value);
				}
				break;
			case PPHS_SALESTAXRATE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					const double _value = RdB.TagValue.ToReal();
					((GoodsBlock *)p_item)->SalesTaxRate = dbltoint2(_value);
				}
				break;
			case PPHS_ALCOPROOF:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					const double _value = RdB.TagValue.ToReal();
					((GoodsBlock *)p_item)->AlcoProof = dbltoint2(_value);
				}
				break;
			case PPHS_ALCORUCAT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					Helper_AddStringToPool(&((GoodsBlock *)p_item)->AlcoRuCatP);
				}
				break;
			case PPHS_UNLIMITED:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					if(RdB.IsTagValueBoolTrue())
						((GoodsBlock *)p_item)->SpecialFlags |= GoodsBlock::spcfUnlim;
				}
				break;
			case PPHS_LOOKBACKPRICES:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods) {
					if(RdB.IsTagValueBoolTrue())
						((GoodsBlock *)p_item)->SpecialFlags |= GoodsBlock::spcfLookBackPrices;
				}
				break;
			case PPHS_SERIAL:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obLot)
					Helper_AddStringToPool(&((LotBlock *)p_item)->SerailP);
				break;
			case PPHS_SYMB:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					Helper_AddStringToPool(&((UnitBlock *)p_item)->SymbP);
				}
				break;
			case PPHS_CODE:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obPosNode:     Helper_AddStringToPool(&((PosNodeBlock *)p_item)->CodeP); break;
					case obGoodsGroup:  Helper_AddStringToPool(&((GoodsGroupBlock *)p_item)->CodeP); break;
					case obPerson:      Helper_AddStringToPool(&((PersonBlock *)p_item)->CodeP); break;
					case obSCardSeries: Helper_AddStringToPool(&((PersonBlock *)p_item)->CodeP); break;
					case obSCard:       Helper_AddStringToPool(&((SCardBlock *)p_item)->CodeP); break;
					case obParent:      Helper_AddStringToPool(&((ParentBlock *)p_item)->CodeP); break;
					case obQuotKind:    Helper_AddStringToPool(&((QuotKindBlock *)p_item)->CodeP); break;
					case obUnit:        Helper_AddStringToPool(&((UnitBlock *)p_item)->CodeP); break; // @v9.8.6
					case obSource:
					case obDestination: Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->CodeP); break;
					case obGoods:
						break;
					case obGoodsCode:
						Helper_AddStringToPool(&((GoodsCode *)p_item)->CodeP);
						RdB.RefPosStack.pop(ref_pos);
						break;
					case obCSession:
						if(RdB.TagValue.NotEmptyS()) {
							long   icode = RdB.TagValue.ToLong();
							((CSessionBlock *)p_item)->Code = icode;
						}
						break;
					case obCCheck:
						if(RdB.TagValue.NotEmptyS()) {
							long   icode = RdB.TagValue.ToLong();
							((CCheckBlock *)p_item)->Code = icode;
						}
						break;
					case obQuery:
						{
							QueryBlock * p_blk = (QueryBlock *)p_item;
							if(p_blk->Q == QueryBlock::qCSession) {
								const long csess_n = RdB.TagValue.ToLong();
								if(csess_n) {
									if(p_blk->CSess || !p_blk->Period.IsZero() || (p_blk->Flags & (QueryBlock::fCSessCurrent|QueryBlock::fCSessLast))) {
										THROW(p_blk = Helper_RenewQuery(ref_pos, QueryBlock::qCSession));
									}
									p_blk->CSess = csess_n;
									p_blk->Flags |= QueryBlock::fCSessN;
								}
							}
						}
						break;
				}
				break;
			case PPHS_BASE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					((UnitBlock *)p_item)->BaseId = RdB.TagValue.ToLong();
				}
				break;
			case PPHS_BASERATIO:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					const double _value = RdB.TagValue.ToReal();
					if(_value >= 0.0)
						((UnitBlock *)p_item)->BaseRatio = _value;
				}
				break;
			case PPHS_RATIO:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					const double _value = RdB.TagValue.ToReal();
					if(_value >= 0.0)
						((UnitBlock *)p_item)->PhRatio = _value;
				}
				break;
			case PPHS_PHYSICAL:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					if(RdB.IsTagValueBoolTrue())
						((UnitBlock *)p_item)->UnitFlags |= PPUnit::Physical;
					else
						((UnitBlock *)p_item)->UnitFlags &= ~PPUnit::Physical;
				}
				break;
			case PPHS_DEFAULT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obUnit) {
					if(RdB.IsTagValueBoolTrue())
						((UnitBlock *)p_item)->UnitFlags |= PPUnit::Default;
					else
						((UnitBlock *)p_item)->UnitFlags &= ~PPUnit::Default;
				}
				break;
			case PPHS_REST:
				{
					const double _value = RdB.TagValue.ToReal();
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						if(_value >= 0.0)
							((LotBlock *)p_item)->Rest = _value;
					}
					else if(type == obGoods) {
						if(_value >= 0.0)
							((GoodsBlock *)p_item)->Rest = _value;
					}
				}
				break;
			case PPHS_COST:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obLot)
					((LotBlock *)p_item)->Cost = RdB.TagValue.ToReal();
				break;
			case PPHS_PRICE:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obGoods)
					((GoodsBlock *)p_item)->Price = RdB.TagValue.ToReal();
				else if(type == obCcLine)
					((CcLineBlock *)p_item)->Price = RdB.TagValue.ToReal();
				else if(type == obLot)
					((LotBlock *)p_item)->Price = RdB.TagValue.ToReal();
				break;
			case PPHS_DISCOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obSCard)
					((SCardBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				else if(type == obCCheck)
					((CCheckBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				else if(type == obCcLine)
					((CcLineBlock *)p_item)->Discount = RdB.TagValue.ToReal();
				break;
			case PPHS_RETURN:
				p_item = PeekRefItem(&ref_pos, &type);
				THROW_PP_S(type == obCCheck, PPERR_PPPP_MISPL_RETURN, RdB.SrcFileName);
				((CCheckBlock *)p_item)->SaCcFlags |= CCHKF_RETURN;
				break;
			case PPHS_SUMDISCOUNT:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obCcLine)
					((CcLineBlock *)p_item)->SumDiscount = RdB.TagValue.ToReal();
				break;
			case PPHS_QTTY:
				p_item = PeekRefItem(&ref_pos, &type);
				if(type == obCcLine)
					((CcLineBlock *)p_item)->Qtty = RdB.TagValue.ToReal();
				break;
			case PPHS_SYSTEM:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->SystemP);
				}
				break;
			case PPHS_VERSION:
				p_item = PeekRefItem(&ref_pos, &type);
				if(oneof2(type, obSource, obDestination)) {
					Helper_AddStringToPool(&((RouteObjectBlock *)p_item)->VersionP);
				}
				break;
			case PPHS_DATE:
				{
					LDATE dt = strtodate_(RdB.TagValue, DATF_ISO8601);
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						((LotBlock *)p_item)->Dt = dt;
					}
				}
				break;
			case PPHS_EXPIRY:
				{
					LDATE dt = strtodate_(RdB.TagValue, DATF_ISO8601);
					p_item = PeekRefItem(&ref_pos, &type);
					if(type == obLot) {
						((LotBlock *)p_item)->Expiry = dt;
					}
				}
				break;
			case PPHS_TIME:
				{
					LDATETIME dtm;
					strtodatetime(RdB.TagValue, &dtm, DATF_ISO8601, 0);
					p_item = PeekRefItem(&ref_pos, &type);
					switch(type) {
						case obCSession: ((CSessionBlock *)p_item)->Dtm = dtm; break;
						case obCCheck:   ((CCheckBlock *)p_item)->Dtm = dtm; break;
					}
				}
				break;
			case PPHS_FLAGS:
				p_item = PeekRefItem(&ref_pos, &type);
				switch(type) {
					case obCCheck:
						{
							uint32 _f = 0;
							size_t real_len = 0;
							RdB.TagValue.DecodeHex(0, &_f, sizeof(_f), &real_len);
							if(real_len == sizeof(_f)) {
								((CCheckBlock *)p_item)->CcFlags = _f;
							}
						}
						break;
				}
				break;
			case PPHS_AMOUNT:
				{
					p_item = PeekRefItem(&ref_pos, &type);
					const double _value = RdB.TagValue.ToReal();
					switch(type) {
						case obCCheck: ((CCheckBlock *)p_item)->Amount = _value; break;
						case obCcLine: ((CcLineBlock *)p_item)->Amount = _value; break;
						case obPayment: ((CcPaymentBlock *)p_item)->Amount = _value; break;
					}
				}
				break;
			case PPHS_SOURCE:
			case PPHS_DESTINATION:
			case PPHS_GOODSGROUP:
			case PPHS_LOT:
			case PPHS_CSESSION:
			case PPHS_CC:
			case PPHS_CCL:
			case PPHS_PAYMENT:
				RdB.RefPosStack.pop(ref_pos);
				break;
			case PPHS_RESTRICTION:
			case PPHS_REFS:
			case PPHS_QUERYCSESS:
			case PPHS_QUERYREFS:
			case PPHS_TIMERANGE:
			case PPHS_AMOUNTRANGE:
			case PPHS_FILE: // @v9.9.12
				break;
			default:
				is_processed = 0;
				break;
		}
	}
	if(is_processed) {
		(RdB.TempBuf = pName).ToLower();
		if(RdB.P_ShT) {
			uint _ut = 0;
			RdB.P_ShT->Search(RdB.TempBuf, &_ut, 0);
			assert((int)_ut == tok);
		}
	}
	assert(ok);
	CATCH
		SaxStop();
		RdB.State |= RdB.stError;
		ok = 0;
	ENDCATCH
    return ok;
}

int PPPosProtocol::Characters(const char * pS, size_t len)
{
	//
	// Одна строка может быть передана несколькими вызовами. По этому StartElement обнуляет
	// буфер RdB.TagValue, а здесь каждый вызов дополняет существующую строку входящими символами
	//
	RdB.TagValue.CatN(pS, len);
	return 1;
}

extern "C" xmlParserCtxt * xmlCreateURLParserCtxt(const char * filename, int options);
void FASTCALL xmlDetectSAX2(xmlParserCtxt * ctxt); // @prototype

SLAPI PPPosProtocol::ReadBlock::ReadBlock() : P_SaxCtx(0), State(0), Phase(phUnkn), P_ShT(PPGetStringHash(PPSTR_HASHTOKEN)), SrcFileDtm(ZERODATETIME)
{
}

SLAPI PPPosProtocol::ReadBlock::~ReadBlock()
{
	Destroy();
	P_ShT = 0;
}

void SLAPI PPPosProtocol::ReadBlock::Destroy()
{
	if(P_SaxCtx) {
		P_SaxCtx->sax = 0;
		xmlFreeDoc(P_SaxCtx->myDoc);
		P_SaxCtx->myDoc = 0;
		xmlFreeParserCtxt(P_SaxCtx);
		P_SaxCtx = 0;
	}
	State = 0;
	Phase = phUnkn;

	TempBuf.Z();
	TagValue.Z();
	SrcFileName.Z();
	SrcFileUUID.Z(); // @v9.9.12
	SrcFileDtm.Z(); // @v9.9.12
	TokPath.freeAll();
	RefPosStack.clear();
	SrcBlkList.freeAll();
	DestBlkList.freeAll();
	GoodsBlkList.freeAll();
	GoodsGroupBlkList.freeAll();
	GoodsCodeList.freeAll();
	LotBlkList.freeAll();
	QkBlkList.freeAll();
	UnitBlkList.freeAll(); // @v9.8.6
	QuotBlkList.freeAll();
	PersonBlkList.freeAll();
	ScsBlkList.freeAll(); // @v9.8.7
	SCardBlkList.freeAll();
	ParentBlkList.freeAll();
	PosBlkList.freeAll();
	CSessBlkList.freeAll();
	CcBlkList.freeAll();
	CclBlkList.freeAll();
	CcPaymBlkList.freeAll();
	QueryList.freeAll();
	RefList.freeAll();
}

int SLAPI PPPosProtocol::CreateGoodsGroup(const GoodsGroupBlock & rBlk, uint refPos, int isFolder, PPID * pID)
{
	int    ok = -1;
	SString name_buf, code_buf;
	PPID   native_id = 0;
	if(rBlk.NativeID) {
		native_id = rBlk.NativeID;
		ok = 2;
	}
	else {
		Goods2Tbl::Rec gg_rec;
		BarcodeTbl::Rec bc_rec;
		PPID   inner_parent_id = 0;
		if(rBlk.ParentBlkP) {
			int   inner_type = 0;
			ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(rBlk.ParentBlkP, &inner_type);
			assert(p_inner_blk);
			if(p_inner_blk) {
				assert(inner_type == obParent);
				THROW(CreateParentGoodsGroup(*p_inner_blk, 1, &inner_parent_id)); // @recursion
			}
		}
		RdB.GetS(rBlk.NameP, name_buf);
		name_buf.Transf(CTRANSF_UTF8_TO_INNER);
		RdB.GetS(rBlk.CodeP, code_buf);
		code_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(code_buf.NotEmptyS() && GgObj.SearchCode(code_buf, &bc_rec) > 0 && GgObj.Search(bc_rec.GoodsID, &gg_rec) > 0) {
			if(gg_rec.Kind == PPGDSK_GROUP && !(gg_rec.Flags & GF_ALTGROUP)) {
				//if((isFolder && gg_rec.Flags & GF_FOLDER) || (!isFolder && !(gg_rec.Flags & GF_FOLDER))) {
				{
					native_id = gg_rec.ID;
					ok = 3;
				}
			}
		}
		if(!native_id) {
			PPID   inner_id = 0;
			if(name_buf.NotEmptyS() && GgObj.SearchByName(name_buf, &inner_id, &gg_rec) > 0) {
				if(gg_rec.Kind == PPGDSK_GROUP && !(gg_rec.Flags & GF_ALTGROUP)) {
					//if((isFolder && gg_rec.Flags & GF_FOLDER) || (!isFolder && !(gg_rec.Flags & GF_FOLDER))) {
					{
						native_id = gg_rec.ID;
						ok = 3;
					}
				}
			}
		}
		if(!native_id) {
			PPGoodsPacket gg_pack;
			THROW(GgObj.InitPacket(&gg_pack, isFolder ? gpkndFolderGroup : gpkndOrdinaryGroup, inner_parent_id, 0, code_buf));
			STRNSCPY(gg_pack.Rec.Name, name_buf);
			if(gg_pack.Rec.ParentID) {
				Goods2Tbl::Rec parent_rec;
				if(GgObj.Search(gg_pack.Rec.ParentID, &parent_rec) > 0) {
					if(parent_rec.Kind != PPGDSK_GROUP || !(parent_rec.Flags & GF_FOLDER) || (parent_rec.Flags & (GF_ALTGROUP|GF_EXCLALTFOLD))) {
						gg_pack.Rec.ParentID = 0;
						// @todo message
					}
				}
				else {
					gg_pack.Rec.ParentID = 0;
					// @todo message
				}
			}
			THROW(GgObj.PutPacket(&native_id, &gg_pack, 1));
			ok = 1;
		}
	}
	// @v10.0.0 {
	if(native_id && !(rBlk.Flags_ & rBlk.fRefItem)) {
		//
		// Акцепт котировок
		//
		PPQuotArray quot_list;
		quot_list.GoodsID = native_id;
		for(uint k = 0; k < RdB.QuotBlkList.getCount(); k++) {
			const QuotBlock & r_qb = RdB.QuotBlkList.at(k);
			if(r_qb.GoodsGroupBlkP == refPos) {
				assert(r_qb.BlkFlags & r_qb.fGroup);
				int    type_qk = 0;
				const QuotKindBlock * p_qk_item = (const QuotKindBlock *)RdB.GetItem(r_qb.QuotKindBlkP, &type_qk);
				if(p_qk_item) {
					assert(type_qk == obQuotKind);
					if(p_qk_item->NativeID) {
						const QuotIdent qi(0 /*locID*/, p_qk_item->NativeID, 0 /*curID*/, 0);
						quot_list.SetQuot(qi, r_qb.Value, r_qb.QuotFlags, r_qb.MinQtty, r_qb.Period.IsZero() ? 0 : &r_qb.Period);
					}
				}
			}
		}
		THROW(GObj.PutQuotList(native_id, &quot_list, 1));
	}
	// } @v10.0.0 
	ASSIGN_PTR(pID, native_id);
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::CreateParentGoodsGroup(const ParentBlock & rBlk, int isFolder, PPID * pID)
{
	int    ok = -1;
	PPID   native_id = 0;
	SString code_buf, temp_buf;
	SString name_buf;
	RdB.GetS(rBlk.CodeP, code_buf);
	code_buf.Transf(CTRANSF_UTF8_TO_INNER);
	if(rBlk.ID || code_buf.NotEmptyS()) {
		for(uint i = 0; i < RdB.GoodsGroupBlkList.getCount(); i++) {
			GoodsGroupBlock & r_grp_blk = RdB.GoodsGroupBlkList.at(i);
			int   this_block = 0;
			if(rBlk.ID) {
				if(r_grp_blk.ID == rBlk.ID) {
					this_block = 1;
				}
			}
			else if(r_grp_blk.CodeP) {
				RdB.GetS(r_grp_blk.CodeP, temp_buf);
				if(code_buf == temp_buf) {
					this_block = 1;
				}
			}
			if(this_block) {
				uint   ref_pos = 0;
				THROW_PP(RdB.SearchRef(obGoodsGroup, i, &ref_pos), PPERR_PPPP_INNERREFNF_GG);
				THROW(ok = CreateGoodsGroup(r_grp_blk, ref_pos, isFolder, &native_id));
				break;
			}
		}
	}
	ASSIGN_PTR(pID, native_id);
	CATCHZOK
	return ok;
}

SLAPI PPPosProtocol::ResolveGoodsParam::ResolveGoodsParam() : DefParentID(0), DefUnitID(0), LocID(0), SrcArID(0),
	AlcGdsClsID(0), AlcProofDim(0), AlcVolumeDim(0), AlcRuCatDim(0)
{
}

void SLAPI PPPosProtocol::ResolveGoodsParam::SetupGoodsPack(const PPPosProtocol::ReadBlock & rRB, const GoodsBlock & rBlk, PPGoodsPacket & rPack) const
{
	SETFLAGBYSAMPLE(rPack.Rec.Flags, GF_NODISCOUNT, rBlk.GoodsFlags);
	rPack.Rec.GoodsTypeID = rBlk.GoodsTypeID;
	rPack.Rec.TaxGrpID = rBlk.TaxGrpID;
	if(rBlk.AlcoProof > 0 && AlcGdsClsID && AlcProofDim) {
		rPack.Rec.GdsClsID = AlcGdsClsID;
		rPack.ExtRec.GoodsClsID = AlcGdsClsID;
		GcPack.RealToExtDim(inttodbl2(rBlk.AlcoProof), AlcProofDim, rPack.ExtRec);
		if(/*rBlk.PhUPerU*/rPack.Rec.PhUPerU > 0.0 && AlcVolumeDim) { // Мы ожидаем здесь соотношение литры/единицу
			GcPack.RealToExtDim(/*rBlk.PhUPerU*/rPack.Rec.PhUPerU, AlcVolumeDim, rPack.ExtRec);
		}
		if(rBlk.AlcoRuCatP && AlcRuCatDim) {
			SString temp_buf;
			rRB.GetS(rBlk.AlcoRuCatP, temp_buf);
			if(temp_buf.NotEmptyS())
				GcPack.RealToExtDim(temp_buf.ToReal(), AlcRuCatDim, rPack.ExtRec);
		}
	}
}

int SLAPI PPPosProtocol::ResolveGoodsBlock(const GoodsBlock & rBlk, uint refPos, int asRefOnly, const ResolveGoodsParam & rP, PPID * pNativeID)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPID   native_id = rBlk.NativeID;
	SString temp_buf;
	Goods2Tbl::Rec ex_goods_rec;
	Goods2Tbl::Rec parent_rec;
	BarcodeTbl::Rec ex_bc_rec;
	PPIDArray pretend_obj_list;
	if(rBlk.Flags_ & ObjectBlock::fRefItem) {
		//
		// Для объектов, переданных как ссылка мы должны найти аналоги в нашей БД, но создавать не будем
		//
		PPID   pretend_id = 0;
		if(rBlk.ID > 0 && GObj.Search(rBlk.ID, &ex_goods_rec) > 0) {
			pretend_id = ex_goods_rec.ID;
		}
		for(uint j = 0; j < RdB.GoodsCodeList.getCount(); j++) {
			const GoodsCode & r_c = RdB.GoodsCodeList.at(j);
			if(r_c.GoodsBlkP == refPos) {
				RdB.GetS(r_c.CodeP, temp_buf);
				if(temp_buf.NotEmptyS()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					THROW_PP_S(temp_buf.Len() < sizeof(ex_bc_rec.Code), PPERR_PPPP_GOODSCODELENEXC, temp_buf);
					if(GObj.SearchByBarcode(temp_buf, &ex_bc_rec, &ex_goods_rec, 0 /* no adopt */) > 0)
						pretend_obj_list.add(ex_goods_rec.ID);
				}
			}
		}
		if(pretend_obj_list.getCount()) {
			pretend_obj_list.sortAndUndup();
			if(pretend_id) {
				if(pretend_obj_list.lsearch(pretend_id)) {
					native_id = pretend_id; // Это - без сомнения наш идентификатор (соответствует и по id и по коду)
				}
				else {
					// @todo Есть сомнения. Идентификатор найден, но не соответствует кодам, переданным нам вместе с ним
					if(pretend_obj_list.getCount() == 1) {
						native_id = pretend_obj_list.get(0); // Предпочтем значение, найденное по коду. Не уверен, но
							// так, по-моему, надежнее, чем по идентификатору.
					}
					else {
						native_id = pretend_id; // Если соответствий по кодам несколько, то идентификатор выглядит надежнее
					}
				}
			}
			else if(pretend_obj_list.getCount() == 1) {
				native_id = pretend_obj_list.get(0); // Единственный код без идентификатора - вполне надежный критерий
			}
			else {
				// @todo Есть сомнения: переданы несколько кодов с разными соответствиями идентификаторам
				native_id = pretend_obj_list.getLast(); // Пока используем последний - он скорее всего самый новый
			}
		}
		else if(pretend_id)
			native_id = pretend_id;
		else {
			; // @err Невозможно сопоставить переданную ссылку на товар
		}
	}
	else if(!asRefOnly) {
		PPGoodsPacket goods_pack;
		PPQuotArray quot_list;
		GObj.InitPacket(&goods_pack, gpkndGoods, 0, 0, 0);
		quot_list.GoodsID = 0;
		int    use_ar_code = 0;
		PPID   goods_by_ar_id = 0;
		if(rBlk.ID > 0) {
			temp_buf.Z().Cat(rBlk.ID);
			if(GObj.P_Tbl->SearchByArCode(rP.SrcArID, temp_buf, 0, &ex_goods_rec) > 0) {
				goods_by_ar_id = ex_goods_rec.ID;
			}
			else {
				ArGoodsCodeTbl::Rec new_ar_code;
				MEMSZERO(new_ar_code);
				new_ar_code.ArID = rP.SrcArID;
				new_ar_code.Pack = 1000; // =1
				STRNSCPY(new_ar_code.Code, temp_buf);
				THROW_SL(goods_pack.ArCodes.insert(&new_ar_code));
			}
			use_ar_code = 1;
		}
		RdB.GetS(rBlk.NameP, temp_buf);
		temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		STRNSCPY(goods_pack.Rec.Name, temp_buf);
		STRNSCPY(goods_pack.Rec.Abbr, temp_buf);
		if(GObj.SearchByName(temp_buf, 0, &ex_goods_rec) > 0) {
			if(use_ar_code && (/*goods_by_ar_id && */ex_goods_rec.ID != goods_by_ar_id)) {
				THROW(GObj.ForceUndupName(goods_by_ar_id, temp_buf));
				THROW(GObj.UpdateName(ex_goods_rec.ID, temp_buf, 1));
			}
			else if(!use_ar_code) { // Для товара с внешним идентификатором аналог по наименованию не принимаем в расчет
				pretend_obj_list.add(ex_goods_rec.ID);
			}
		}
		{
			PPID   parent_id = 0;
			if(rBlk.ParentBlkP) {
				int   inner_type = 0;
				ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(rBlk.ParentBlkP, &inner_type);
				assert(p_inner_blk);
				if(p_inner_blk) {
					assert(inner_type == obParent);
					if(inner_type == obParent) {
						THROW(CreateParentGoodsGroup(*p_inner_blk, 0, &parent_id));
					}
				}
			}
			{
				if(GgObj.Fetch(parent_id, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_FOLDER|GF_EXCLALTFOLD)))
					goods_pack.Rec.ParentID = parent_id;
				else
					goods_pack.Rec.ParentID = rP.DefParentID;
			}
		}
		if(rBlk.UnitBlkP) {
			int   inner_type = 0;
			UnitBlock * p_inner_blk = (UnitBlock *)RdB.GetItem(rBlk.UnitBlkP, &inner_type);
			assert(p_inner_blk);
			if(p_inner_blk) {
				assert(inner_type == obUnit);
				if(inner_type == obUnit && p_inner_blk->NativeID) {
					goods_pack.Rec.UnitID = p_inner_blk->NativeID;
					if(p_inner_blk->PhUnitBlkP) {
						int   phinner_type = 0;
						UnitBlock * p_phinner_blk = (UnitBlock *)RdB.GetItem(p_inner_blk->PhUnitBlkP, &phinner_type);
						assert(p_phinner_blk);
						if(p_phinner_blk) {
							assert(phinner_type == obUnit);
							if(phinner_type == obUnit && p_phinner_blk->NativeID) {
								goods_pack.Rec.PhUnitID = p_phinner_blk->NativeID;
								goods_pack.Rec.PhUPerU = p_phinner_blk->PhRatio;
							}
						}
					}
				}
			}
		}
		for(uint j = 0; j < RdB.GoodsCodeList.getCount(); j++) {
			const GoodsCode & r_c = RdB.GoodsCodeList.at(j);
			if(r_c.GoodsBlkP == refPos) {
				RdB.GetS(r_c.CodeP, temp_buf);
				if(temp_buf.NotEmptyS()) {
					temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					THROW_PP_S(temp_buf.Len() < sizeof(ex_bc_rec.Code), PPERR_PPPP_GOODSCODELENEXC, temp_buf);
					THROW(goods_pack.Codes.Add(temp_buf, 0, 1.0));
					if(GObj.SearchByBarcode(temp_buf, &ex_bc_rec, &ex_goods_rec, 0 /* no adopt */) > 0) {
						if(use_ar_code && (goods_by_ar_id && ex_goods_rec.ID != goods_by_ar_id)) {
							THROW(GObj.P_Tbl->RemoveDupBarcode(goods_by_ar_id, temp_buf, 1));
						}
						else
							pretend_obj_list.add(ex_goods_rec.ID);
					}
				}
			}
		}
		rP.SetupGoodsPack(RdB, rBlk, goods_pack);
		pretend_obj_list.sortAndUndup();
		if(use_ar_code) {
			if(goods_by_ar_id) {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = goods_by_ar_id;
				THROW(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0);
				STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
				STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
				if(goods_pack.Rec.ParentID)
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				ex_goods_pack.Codes = goods_pack.Codes;
				//
				// Соображения насчет кодов по статьям:
				// если (use_ar_code && goods_by_ar_id), то нужный код уже находится в ex_goods_pack
				// Кроме него там могут быть и иные коды по статьям, не входящие в компетенцию
				// данного импортируемого пакета.
				// Таким образом, никаких манипуляций со списком кодов по статьям здесь не производим.
				//
				if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
					parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				}
				ex_goods_pack.Rec.UnitID = goods_pack.Rec.UnitID;
				ex_goods_pack.Rec.PhUnitID = goods_pack.Rec.PhUnitID;
				ex_goods_pack.Rec.PhUPerU = goods_pack.Rec.PhUPerU;
				rP.SetupGoodsPack(RdB, rBlk, ex_goods_pack);
				THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
				native_id = ex_goods_id;
			}
			else if(pretend_obj_list.getCount() == 0) {
				PPID   new_goods_id = 0;
				SETIFZ(goods_pack.Rec.UnitID, rP.DefUnitID);
				THROW(GObj.PutPacket(&new_goods_id, &goods_pack, 1));
				native_id = new_goods_id;
			}
			else {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = pretend_obj_list.get(0);
				THROW(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0);
				STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
				STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
				if(goods_pack.Rec.ParentID)
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				ex_goods_pack.Codes = goods_pack.Codes;
				for(uint bci = 0; bci < goods_pack.Codes.getCount(); bci++) {
					const BarcodeTbl::Rec & r_bc_rec = goods_pack.Codes.at(bci);
					uint   bcp = 0;
					if(ex_goods_pack.Codes.SearchCode(r_bc_rec.Code, &bcp)) {
						ex_goods_pack.Codes.atFree(bcp);
					}
					ex_goods_pack.Codes.insert(&r_bc_rec);
					THROW(GObj.P_Tbl->RemoveDupBarcode(ex_goods_id, r_bc_rec.Code, 1));
				}
				for(uint aci = 0; aci < goods_pack.ArCodes.getCount(); aci++) {
					THROW_SL(ex_goods_pack.ArCodes.insert(&goods_pack.ArCodes.at(aci)));
				}
				if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
					parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				}
				ex_goods_pack.Rec.UnitID = goods_pack.Rec.UnitID;
				ex_goods_pack.Rec.PhUnitID = goods_pack.Rec.PhUnitID;
				ex_goods_pack.Rec.PhUPerU = goods_pack.Rec.PhUPerU;
				rP.SetupGoodsPack(RdB, rBlk, ex_goods_pack);
				THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
				native_id = ex_goods_id;
			}
		}
		else {
			if(pretend_obj_list.getCount() == 0) {
				PPID   new_goods_id = 0;
				SETIFZ(goods_pack.Rec.UnitID, rP.DefUnitID);
				THROW(GObj.PutPacket(&new_goods_id, &goods_pack, 1));
				native_id = new_goods_id;
			}
			else if(pretend_obj_list.getCount() == 1) {
				PPGoodsPacket ex_goods_pack;
				PPID   ex_goods_id = pretend_obj_list.get(0);
				THROW(GObj.GetPacket(ex_goods_id, &ex_goods_pack, 0) > 0);
				STRNSCPY(ex_goods_pack.Rec.Name, goods_pack.Rec.Name);
				STRNSCPY(ex_goods_pack.Rec.Abbr, goods_pack.Rec.Abbr);
				if(goods_pack.Rec.ParentID)
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				ex_goods_pack.Codes = goods_pack.Codes;
				if(goods_pack.Rec.ParentID && GgObj.Search(goods_pack.Rec.ParentID, &parent_rec) > 0 &&
					parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
					ex_goods_pack.Rec.ParentID = goods_pack.Rec.ParentID;
				}
				ex_goods_pack.Rec.UnitID = goods_pack.Rec.UnitID;
				ex_goods_pack.Rec.PhUnitID = goods_pack.Rec.PhUnitID;
				ex_goods_pack.Rec.PhUPerU = goods_pack.Rec.PhUPerU;
				rP.SetupGoodsPack(RdB, rBlk, ex_goods_pack);
				THROW(GObj.PutPacket(&ex_goods_id, &ex_goods_pack, 1));
				native_id = ex_goods_id;
			}
			else {
				; // @error импортируемый товар может быть сопоставлен более чем одному товару в бд.
			}
		}
		if(native_id) {
			{
				//
				// Акцепт котировок
				//
				quot_list.GoodsID = native_id;
				for(uint k = 0; k < RdB.QuotBlkList.getCount(); k++) {
					const QuotBlock & r_qb = RdB.QuotBlkList.at(k);
					if(r_qb.GoodsBlkP == refPos) {
						assert(!(r_qb.BlkFlags & r_qb.fGroup));
						int    type_qk = 0;
						const QuotKindBlock * p_qk_item = (const QuotKindBlock *)RdB.GetItem(r_qb.QuotKindBlkP, &type_qk);
						if(p_qk_item) {
							assert(type_qk == obQuotKind);
							if(p_qk_item->NativeID) {
								const QuotIdent qi(0 /*locID*/, p_qk_item->NativeID, 0 /*curID*/, 0);
								quot_list.SetQuot(qi, r_qb.Value, r_qb.QuotFlags, r_qb.MinQtty, r_qb.Period.IsZero() ? 0 : &r_qb.Period);
							}
						}
					}
				}
				//
				// Базовую цену загружаем в список quot_list последней на случай, если в переданном списке котировок
				// была указана и базовая (дабы перебить неоднозначность в пользу Price).
				//
				if(rBlk.Price > 0.0) {
					const QuotIdent qi(0 /*locID*/, PPQUOTK_BASE, 0 /*curID*/, 0);
					quot_list.SetQuot(qi, rBlk.Price, 0 /*flags*/, 0, 0 /* period */);
				}
				THROW(GObj.PutQuotList(native_id, &quot_list, 1));
			}
			if(rP.LocID) {
				//
				// Акцепт лотов
				//
				ReceiptCore & r_rcpt = P_BObj->trfr->Rcpt;
				LotArray lot_list;
				StrAssocArray serial_list;
				for(uint k = 0; k < RdB.LotBlkList.getCount(); k++) {
					const LotBlock & r_blk = RdB.LotBlkList.at(k);
					if(r_blk.GoodsBlkP == refPos) {
						ReceiptTbl::Rec lot_rec;
						MEMSZERO(lot_rec);
						lot_rec.GoodsID = native_id;
						lot_rec.Dt = r_blk.Dt;
						lot_rec.Expiry = r_blk.Expiry;
						lot_rec.Cost = r_blk.Cost;
						lot_rec.Price = r_blk.Price;
						lot_rec.Quantity = r_blk.Rest;
						lot_rec.Rest = r_blk.Rest;
						if(lot_rec.Rest <= 0.0) {
							lot_rec.Closed = 1;
							lot_rec.Flags |= LOTF_CLOSED;
						}
						lot_rec.Flags |= LOTF_SURROGATE;
						lot_rec.LocID = rP.LocID;
						THROW_SL(lot_list.insert(&lot_rec));
                        if(RdB.GetS(r_blk.SerailP, temp_buf) && temp_buf.NotEmptyS()) {
                            serial_list.Add((long)lot_list.getCount(), temp_buf);
                        }
					}
				}
				if(lot_list.getCount() == 0 && rBlk.Rest > 0.0) {
					ReceiptTbl::Rec lot_rec;
					MEMSZERO(lot_rec);
					lot_rec.GoodsID = native_id;
					lot_rec.Dt = getcurdate_();
					lot_rec.Quantity = rBlk.Rest;
					lot_rec.Rest = rBlk.Rest;
					if(lot_rec.Rest <= 0.0) {
						lot_rec.Closed = 1;
						lot_rec.Flags |= LOTF_CLOSED;
					}
					lot_rec.Flags |= LOTF_SURROGATE;
					lot_rec.LocID = rP.LocID;
					THROW_SL(lot_list.insert(&lot_rec));
				}
				{
					PPTransaction tra(1);
					THROW(tra);
					//
					// Удаляем все существующие SURROGATE-лоты
					//
					{
						PPIDArray lot_id_list_to_remove;
						ReceiptTbl::Key2 rk2;
						MEMSZERO(rk2);
						rk2.GoodsID = native_id;
						if(r_rcpt.search(2, &rk2, spGe) && r_rcpt.data.GoodsID == native_id) do {
							if(r_rcpt.data.Flags & LOTF_SURROGATE && r_rcpt.data.BillID == 0) {
								THROW_SL(lot_id_list_to_remove.add(r_rcpt.data.ID));
                                THROW_DB(r_rcpt.deleteRec());
							}
						} while(r_rcpt.search(2, &rk2, spNext) && r_rcpt.data.GoodsID == native_id);
						for(uint _lidx = 0; _lidx < lot_id_list_to_remove.getCount(); _lidx++) {
							THROW(p_ref->Ot.RemoveTag(PPOBJ_LOT, lot_id_list_to_remove.get(_lidx), 0, 0));
						}
					}
					//
					// Вставляем новые SURROGATE-лоты
					//
					{
                        for(uint _lidx = 0; _lidx < lot_list.getCount(); _lidx++) {
							ReceiptTbl::Rec lot_rec = lot_list.at(_lidx);
							long   oprno = 1;
							ReceiptTbl::Key1 rk1;
							MEMSZERO(rk1);
							rk1.Dt = lot_rec.Dt;
							rk1.OprNo = oprno;
							while(r_rcpt.search(1, &rk1, spEq)) {
								rk1.OprNo = ++oprno;
							}
							lot_rec.OprNo = oprno;
							{
                        		ReceiptTbl::Key0 rk0;
								MEMSZERO(rk0);
								THROW_DB(r_rcpt.insertRecBuf(&lot_rec, 0, &rk0));
								if(serial_list.GetText(_lidx+1, temp_buf)) {
									THROW(P_BObj->SetSerialNumberByLot(rk0.ID, temp_buf, 0));
								}
							}
                        }
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pNativeID, native_id);
	return ok;
}

int SLAPI PPPosProtocol::AcceptData(PPID posNodeID, int silent)
{
	int    ok = 1;
	uint   qpb_list_idx = 0;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	SString code_buf;
	SString name_buf;
	SString symb_buf;
	SString wait_msg_buf;
	PPIDArray temp_id_list;
	PPIDArray pretend_obj_list; // Список ид объектов, которые соответствуют импортируемому
	PPObjUnit u_obj;
	PPObjSCardSeries scs_obj;
	PPObjPersonKind pk_obj;
	ResolveGoodsParam rgp;
	rgp.DefUnitID = GObj.GetConfig().DefUnitID;
	rgp.DefParentID = GObj.GetConfig().DefGroupID;
	SString def_unit_name;
	if(!silent)
		PPWait(1);
	if(posNodeID) {
		PPCashNode cn_rec;
		if(CnObj.Search(posNodeID, &cn_rec) > 0)
			rgp.LocID = cn_rec.LocID;
	}
	{
		//
		// Прежде всего разберем данные об источнике и получателях данных
		//
		if(RdB.SrcBlkList.getCount() == 1) {
			RouteObjectBlock & r_blk = RdB.SrcBlkList.at(0);
			if(!r_blk.Uuid.IsZero()) {
				// @todo Создать или найти глобальную учетную запись и сопоставленную ей аналитическую статью src_ar_id
			}
		}
		else {
			if(RdB.SrcBlkList.getCount() > 1) {
				; // @error
			}
			else { // no src info
				; // @log
			}
		}
		//

		//
		if(RdB.DestBlkList.getCount()) {
			for(uint i = 0; i < RdB.DestBlkList.getCount(); i++) {
			}
		}
	}
	{
		//
		// Акцепт видов котировок.
		// 2 фазы
		//   1-я фаза - виды котировок без флага ObjectBlock::fRefItem
		//   2-я фаза - виды котировок не зависимо от флага ObjectBlock::fRefItem (без NativeID)
		//
		PPLoadText(PPTXT_IMPQUOTKIND, wait_msg_buf);
		const uint __count = RdB.QkBlkList.getCount();
		for(uint phase = 0; phase < 2; phase++) {
			for(uint i = 0; i < __count; i++) {
				QuotKindBlock & r_blk = RdB.QkBlkList.at(i);
				if(((phase > 0) || !(r_blk.Flags_ & r_blk.fRefItem)) && !r_blk.NativeID) {
					PPID   native_id = 0;
					PPQuotKind qk_rec;
					const QuotKindBlock * p_analog = RdB.SearchAnalog_QuotKind(r_blk);
					if(p_analog) {
						r_blk.NativeID = p_analog->NativeID;
					}
					else {
						PPQuotKindPacket pack;
						RdB.GetS(r_blk.CodeP, code_buf);
						code_buf.Transf(CTRANSF_UTF8_TO_INNER);
						RdB.GetS(r_blk.NameP, name_buf);
						name_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(code_buf.NotEmptyS()) {
							if(QkObj.SearchBySymb(code_buf, &native_id, &qk_rec) > 0)
								pack.Rec = qk_rec;
							else
								native_id = 0;
						}
						else if(name_buf.NotEmpty()) {
							if(QkObj.SearchByName(name_buf, &native_id, &qk_rec) > 0)
								pack.Rec = qk_rec;
							else
								native_id = 0;
						}
						if(name_buf.NotEmpty())
							STRNSCPY(pack.Rec.Name, name_buf);
						if(pack.Rec.Name[0] == 0)
							STRNSCPY(pack.Rec.Name, code_buf);
						if(pack.Rec.Name[0] == 0) {
							temp_buf.Z().CatChar('#').Cat(r_blk.ID);
							STRNSCPY(pack.Rec.Name, temp_buf);
						}
						STRNSCPY(pack.Rec.Symb, code_buf);
						pack.Rec.Rank = r_blk.Rank;
						pack.Rec.SetAmtRange(&r_blk.AmountRestriction);
						pack.Rec.SetTimeRange(r_blk.TimeRestriction);
						pack.Rec.Period = r_blk.Period;
						THROW(QkObj.PutPacket(&native_id, &pack, 1));
						r_blk.NativeID = native_id;
					}
				}
				PPWaitPercent((phase * __count) + i+1, __count * 2, wait_msg_buf);
			}
		}
	}
	{
		PPUnit u_rec;
		LAssocArray unit_foreign_to_native_assoc;
		if(rgp.DefUnitID && u_obj.Search(rgp.DefUnitID, &u_rec) > 0) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			rgp.DefUnitID = 0;
			def_unit_name = "default";
			while(!rgp.DefUnitID && u_obj.SearchByName(def_unit_name, 0, &u_rec) > 0) {
				if(u_rec.Flags & PPUnit::Trade) {
					rgp.DefUnitID = u_rec.ID;
				}
				else
					(def_unit_name = "default").CatChar('-').CatLongZ(++def_counter, 3);
			}
		}
		//
		// Акцепт единиц измерения
		//
		{
			PPTransaction tra(1);
			THROW(tra);
			const uint __count = RdB.UnitBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				UnitBlock & r_blk = RdB.UnitBlkList.at(i);
				if(!(r_blk.Flags_ & r_blk.fRefItem)) {
					PPUnit unit_rec;
					MEMSZERO(unit_rec);
					RdB.GetS(r_blk.NameP, name_buf);
					name_buf.Transf(CTRANSF_UTF8_TO_INNER);
					if(name_buf.NotEmptyS()) {
						const  int is_default = BIN(r_blk.UnitFlags & PPUnit::Default);
						PPID   native_id = 0;
						RdB.GetS(r_blk.SymbP, symb_buf);
						symb_buf.Transf(CTRANSF_UTF8_TO_INNER);
						RdB.GetS(r_blk.CodeP, code_buf);
						code_buf.Transf(CTRANSF_UTF8_TO_INNER);
						STRNSCPY(unit_rec.Name, name_buf);
						STRNSCPY(unit_rec.Abbr, symb_buf);
						STRNSCPY(unit_rec.Code, code_buf);
						SETFLAGBYSAMPLE(unit_rec.Flags, PPUnit::Physical, r_blk.UnitFlags);
						unit_rec.Flags |= PPUnit::Trade;
						if(r_blk.ID > 0 && r_blk.ID < 1000) {
							unit_rec.ID = r_blk.ID;
							native_id = r_blk.ID;
							if(u_obj.Search(native_id, &u_rec) > 0) {
								int r = u_obj.ref->UpdateItem(PPOBJ_UNIT, native_id, &unit_rec, 1, 0);
								THROW(r);
							}
							else {
								THROW(u_obj.ref->AddItem(PPOBJ_UNIT, &native_id, &unit_rec, 0));
							}
						}
						else {
							PPID   temp_id = 0;
							if(u_obj.SearchByName(unit_rec.Name, &native_id, &u_rec) > 0) {
								int r = u_obj.ref->UpdateItem(PPOBJ_UNIT, native_id, &unit_rec, 1, 0);
								THROW(r);
							}
							else {
								THROW(u_obj.ref->AddItem(PPOBJ_UNIT, &native_id, &unit_rec, 0));
							}
							if(unit_rec.Abbr[0] && u_obj.SearchBySymb(unit_rec.Abbr, &temp_id, &u_rec) > 0 && temp_id != native_id) {
								u_rec.Abbr[0] = 0;
								int r = u_obj.ref->UpdateItem(PPOBJ_UNIT, temp_id, &u_rec, 1, 0);
							}
						}
						r_blk.NativeID = native_id;
						if(is_default)
							rgp.DefUnitID = native_id;
						if(r_blk.NativeID && r_blk.ID) {
							unit_foreign_to_native_assoc.Add(r_blk.ID, r_blk.NativeID);
						}
					}
				}
			}
			THROW(tra.Commit());
		}
		//
		// Теперь, после акцепта всех единиц измерения, идентифицируем ссылки на единицы измерения
		//
		{
			const uint __count = RdB.UnitBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				UnitBlock & r_blk = RdB.UnitBlkList.at(i);
				if(r_blk.Flags_ & r_blk.fRefItem) {
					long   native_id = 0;
					if(r_blk.ID && unit_foreign_to_native_assoc.Search(r_blk.ID, &native_id, 0) && native_id) {
						r_blk.NativeID = native_id;
					}
					else {
						PPID   temp_id = 0;
						RdB.GetS(r_blk.NameP, name_buf);
						name_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(name_buf.NotEmptyS() && u_obj.SearchByName(name_buf, &temp_id, 0) > 0) {
							r_blk.NativeID = temp_id;
						}
						else {
							RdB.GetS(r_blk.SymbP, symb_buf);
							symb_buf.Transf(CTRANSF_UTF8_TO_INNER);
							if(symb_buf.NotEmptyS() && u_obj.SearchBySymb(symb_buf, &(temp_id = 0), 0) > 0) {
								r_blk.NativeID = temp_id;
							}
						}
					}
				}
			}
		}
	}
	{
		//
		// Акцепт товарных групп
		//
		{
			//
			// Сначала прогоним цикл по группам с целью создать или идентифицировать все группы-папки
			//
			PPLoadText(PPTXT_IMPGOODSGRP, wait_msg_buf);
			const uint __count = RdB.GoodsGroupBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				GoodsGroupBlock & r_blk = RdB.GoodsGroupBlkList.at(i);
				PPID   parent_id = 0;
				if(r_blk.ParentBlkP) {
					int   inner_type = 0;
					ParentBlock * p_inner_blk = (ParentBlock *)RdB.GetItem(r_blk.ParentBlkP, &inner_type);
					assert(p_inner_blk);
					if(p_inner_blk) {
						assert(inner_type == obParent);
						THROW(CreateParentGoodsGroup(*p_inner_blk, 1, &parent_id));
					}
				}
				PPWaitPercent(i+1, __count * 2, wait_msg_buf);
			}
		}
		{
			//
			// Теперь создадим все группы нижнего уровня (папки уже идентифицированы на предыдущем проходе)
			//
			PPLoadText(PPTXT_IMPGOODSGRP, wait_msg_buf);
			const uint __count = RdB.GoodsGroupBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				GoodsGroupBlock & r_blk = RdB.GoodsGroupBlkList.at(i);
				if(r_blk.NativeID == 0) {
					uint   ref_pos = 0;
					THROW_PP(RdB.SearchRef(obGoodsGroup, i, &ref_pos), PPERR_PPPP_INNERREFNF_GG);
					THROW(CreateGoodsGroup(r_blk, ref_pos, 0, &r_blk.NativeID));
				}
				PPWaitPercent(__count+i+1, __count * 2, wait_msg_buf);
			}
		}
	}
	{
		//
		// Акцепт товаров
		//
		PPGoodsPacket goods_pack;
		PPQuotArray quot_list;
		Goods2Tbl::Rec parent_rec;
		SString def_parent_name;
		if(rgp.DefParentID && GgObj.Search(rgp.DefParentID, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
			; // @ok
		}
		else {
			long   def_counter = 0;
			rgp.DefParentID = 0;
			def_parent_name = "default";
			while(!rgp.DefParentID && GgObj.SearchByName(def_parent_name, 0, &parent_rec) > 0) {
				if(parent_rec.Kind == PPGDSK_GROUP && !(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER))) {
					rgp.DefParentID = parent_rec.ID;
				}
				else
					(def_parent_name = "default").CatChar('-').CatLongZ(++def_counter, 3);
			}
		}
		if(!rgp.DefParentID || rgp.DefParentID != GObj.GetConfig().DefGroupID || !rgp.DefUnitID || rgp.DefUnitID != GObj.GetConfig().DefUnitID) {
			PPTransaction tra(1);
			THROW(tra);
			if(!rgp.DefParentID) {
				THROW(GgObj.AddSimple(&rgp.DefParentID, gpkndOrdinaryGroup, 0, def_parent_name, 0, 0, 0));
			}
			if(!rgp.DefUnitID) {
				THROW(u_obj.AddSimple(&rgp.DefUnitID, def_unit_name, PPUnit::Trade, 0));
			}
			if(rgp.DefParentID != GObj.GetConfig().DefGroupID || rgp.DefUnitID != GObj.GetConfig().DefUnitID) {
				PPGoodsConfig new_cfg;
				GObj.ReadConfig(&new_cfg);
				new_cfg.DefGroupID = rgp.DefParentID;
				new_cfg.DefUnitID = rgp.DefUnitID;
				THROW(GObj.WriteConfig(&new_cfg, 0, 0));
			}
			THROW(tra.Commit());
		}
		assert(rgp.DefParentID > 0);
		assert(rgp.DefUnitID > 0);
		{
			PPLoadText(PPTXT_IMPGOODS, wait_msg_buf);
			const uint __count = RdB.GoodsBlkList.getCount();
			int   is_there_alc = 0; // Если обнаружена ненулевая алкогольная крепость в каком-либо товаре, то !0
			struct SurrGoodsTypeEntry {
				SurrGoodsTypeEntry(long flags) : NativeID(0), Flags(flags)
				{
				}
				long  NativeID;
				long  Flags;
			};
			struct SurrTaxRateEntry {
				SurrTaxRateEntry(long vatRate, long stRate) : NativeID(0), VatRate(vatRate), SalesTaxRate(stRate)
				{
				}
				long   NativeID;
				long   VatRate;      // @fixedpoint2
				long   SalesTaxRate; // @fixedpoint2
			};
			SVector gt_list(sizeof(SurrGoodsTypeEntry)); // Список обнаруженных комбинаций флагов, относящихся к типам товаров
			SVector tax_rate_list(sizeof(SurrTaxRateEntry));
			{
				//
				// Предварительный промотр списка товаров с целью идентификации
				// необходимых типов и классов товаров, а так же налоговых групп,
				// которые придется найти или создать.
				//
				for(uint i = 0; i < __count; i++) {
					GoodsBlock & r_blk = RdB.GoodsBlkList.at(i);
					//uint   ref_pos = 0;
					//THROW_PP(RdB.SearchRef(obGoods, i, &ref_pos), PPERR_PPPP_INNERREFNF_G);
					if(r_blk.AlcoProof > 0) {
						is_there_alc = 1;
					}
					if(r_blk.SpecialFlags & (r_blk.spcfLookBackPrices|r_blk.spcfUnlim)) {
						long gt_flags = 0;
						if(r_blk.SpecialFlags & r_blk.spcfLookBackPrices)
							gt_flags |= GTF_LOOKBACKPRICES;
						if(r_blk.SpecialFlags & r_blk.spcfUnlim)
							gt_flags |= GTF_UNLIMITED;
						int    is_gt_found = 0;
						for(uint j = 0; !is_gt_found && j < gt_list.getCount(); j++) {
							const SurrGoodsTypeEntry * p_entry = (const SurrGoodsTypeEntry *)gt_list.at(j);
							if(p_entry->Flags == gt_flags) {
								r_blk.GoodsTypeID = j+1;
								is_gt_found = 1;
							}
						}
						if(!is_gt_found) {
							SurrGoodsTypeEntry new_entry(gt_flags);
							gt_list.insert(&new_entry);
							r_blk.GoodsTypeID = gt_list.getCount();
						}
					}
					if(r_blk.VatRate > 0 || r_blk.SalesTaxRate > 0) {
						int    is_tg_found = 0;
						for(uint j = 0; !is_tg_found && j < tax_rate_list.getCount(); j++) {
							const SurrTaxRateEntry * p_entry = (const SurrTaxRateEntry *)tax_rate_list.at(j);
							if(p_entry->VatRate == r_blk.VatRate && p_entry->SalesTaxRate == r_blk.SalesTaxRate) {
								r_blk.TaxGrpID = j+1;
								is_tg_found = 1;
							}
						}
						if(!is_tg_found) {
							SurrTaxRateEntry new_entry(r_blk.VatRate, r_blk.SalesTaxRate);
							tax_rate_list.insert(&new_entry);
							r_blk.TaxGrpID = tax_rate_list.getCount();
						}
					}
					PPWaitPercent(i+1, __count, wait_msg_buf);
				}
			}
			if(is_there_alc) {
				THROW(PrcssrAlcReport::AutoConfigure(0));
				{
					PrcssrAlcReport::Config alccfg;
					THROW(PrcssrAlcReport::ReadConfig(&alccfg) > 0);
					{
						PPObjGoodsClass gc_obj;
						PPGdsClsPacket gc_pack;
						THROW_PP(alccfg.E.AlcGoodsClsID && gc_obj.Fetch(alccfg.E.AlcGoodsClsID, &gc_pack) > 0, PPERR_ALCRCFG_INVALCGOODSCLS);
						THROW_PP(oneof4(alccfg.E.ProofClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW), PPERR_ALCRCFG_INVPROOFDIM);
						rgp.AlcGdsClsID = alccfg.E.AlcGoodsClsID;
						rgp.AlcProofDim = alccfg.E.ProofClsDim;
						rgp.AlcVolumeDim = alccfg.VolumeClsDim; // @v9.8.12
						rgp.AlcRuCatDim = alccfg.CategoryClsDim; // @v9.9.0
						rgp.GcPack = gc_pack;
					}
				}
			}
			if(tax_rate_list.getCount()) {
				PPObjGoodsTax gt_obj;
				for(uint j = 0; j < tax_rate_list.getCount(); j++) {
					SurrTaxRateEntry * p_entry = (SurrTaxRateEntry *)tax_rate_list.at(j);
					PPID   gt_id = 0;
					if(gt_obj.GetByScheme(&gt_id, inttodbl2(p_entry->VatRate), 0.0, inttodbl2(p_entry->SalesTaxRate), 0/*flags*/, 1/*use_ta*/)) {
						p_entry->NativeID = gt_id;
					}
				}
			}
			if(gt_list.getCount()) {
				PPObjGoodsType gty_obj;
				PPGoodsType gty_rec;
				for(SEnum en = gty_obj.Enum(0); en.Next(&gty_rec) > 0;) {
					for(uint j = 0; j < gt_list.getCount(); j++) {
						SurrGoodsTypeEntry * p_entry = (SurrGoodsTypeEntry *)gt_list.at(j);
						if(!p_entry->NativeID) {
							if((p_entry->Flags & (GTF_LOOKBACKPRICES|GTF_UNLIMITED)) == (gty_rec.Flags & (GTF_LOOKBACKPRICES|GTF_UNLIMITED))) {
								p_entry->NativeID = gty_rec.ID;
							}
						}
					}
				}
				{
					const char * p_gty_name_prefix = "Surrogate Type";
					const char * p_gty_code_prefix = "SURRTYP";
					for(uint j = 0; j < gt_list.getCount(); j++) {
						SurrGoodsTypeEntry * p_entry = (SurrGoodsTypeEntry *)gt_list.at(j);
						if(!p_entry->NativeID) {
							long   sfx_val = 0;
							name_buf = p_gty_name_prefix;
							while(gty_obj.SearchByName(name_buf, 0, 0) > 0) {
								(name_buf = p_gty_name_prefix).Space().CatChar('#').CatLongZ(++sfx_val, 2);
							}
							sfx_val = 0;
							code_buf = p_gty_code_prefix;
							while(gty_obj.SearchBySymb(code_buf, 0, 0) > 0) {
								(code_buf = p_gty_code_prefix).Space().CatChar('#').CatLongZ(++sfx_val, 2);
							}
							MEMSZERO(gty_rec);
							STRNSCPY(gty_rec.Name, name_buf);
							STRNSCPY(gty_rec.Symb, code_buf);
							gty_rec.Flags |= p_entry->Flags;
							THROW(PPRef->AddItem(PPOBJ_GOODSTYPE, &p_entry->NativeID, &gty_rec, 1));
						}
					}
				}
			}
			{
				//
				// Здесь мы расставляем разрешенные реальные идентификаторы налоговых групп и типов товаров
				// в соответствующие товарные ячейки.
				//
				for(uint i = 0; i < __count; i++) {
					GoodsBlock & r_blk = RdB.GoodsBlkList.at(i);
					const uint gty_idx = r_blk.GoodsTypeID;
					if(gty_idx) {
						assert(gty_idx > 0 && gty_idx <= gt_list.getCount());
						r_blk.GoodsTypeID = ((const SurrGoodsTypeEntry *)gt_list.at(gty_idx-1))->NativeID;
					}
					const uint tax_idx = r_blk.TaxGrpID;
					if(tax_idx) {
						assert(tax_idx > 0 && tax_idx <= tax_rate_list.getCount());
						r_blk.TaxGrpID = ((const SurrTaxRateEntry *)tax_rate_list.at(tax_idx-1))->NativeID;
					}
				}
			}
			{
				for(uint i = 0; i < __count; i++) {
					GoodsBlock & r_blk = RdB.GoodsBlkList.at(i);
					PPID   native_id = 0;
					uint   ref_pos = 0;
					THROW_PP(RdB.SearchRef(obGoods, i, &ref_pos), PPERR_PPPP_INNERREFNF_G);
					THROW(ResolveGoodsBlock(r_blk, ref_pos, 0, rgp/*def_parent_id, def_unit_id, src_ar_id, loc_id*/, &native_id));
					r_blk.NativeID = native_id;
					PPWaitPercent(i+1, __count, wait_msg_buf);
				}
			}
		}
	}
	{
		LAssocArray scs_foreign_to_native_assoc;
		PPSCardSerPacket scs_pack;
		PPSCardSeries scs_rec;
		PPSCardConfig sc_cfg;
		ScObj.FetchConfig(&sc_cfg);
		PPID   def_dcard_ser_id = sc_cfg.DefSerID; // Серия дисконтных карт по умолчанию
		PPID   def_ccard_ser_id = sc_cfg.DefCreditSerID; // Серия кредитных карт по умолчанию
		{
			//
			// Акцепт серий персональных карт
			//
			PPTransaction tra(1);
			THROW(tra);
			const uint __count = RdB.ScsBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				SCardSeriesBlock & r_blk = RdB.ScsBlkList.at(i);
				long   native_id = 0;
				if(!(r_blk.Flags_ & ObjectBlock::fRefItem)) {
					scs_pack.Init();
					RdB.GetS(r_blk.CodeP, code_buf);
					code_buf.Transf(CTRANSF_UTF8_TO_INNER);
					RdB.GetS(r_blk.NameP, name_buf);
					name_buf.Transf(CTRANSF_UTF8_TO_INNER);
					if(!name_buf.NotEmptyS())
						name_buf = code_buf;
					if(!name_buf.NotEmptyS())
						name_buf.Z().CatChar('#').Cat(r_blk.ID);
					if(code_buf.NotEmptyS()) {
						if(scs_obj.SearchBySymb(code_buf, &native_id, &scs_rec) > 0) {
							//scs_pack.Rec = scs_rec;
							assert(scs_rec.ID == native_id); // @paranoic
							THROW(scs_obj.GetPacket(native_id, &scs_pack) > 0);
						}
						else
							native_id = 0;
					}
					else if(name_buf.NotEmpty()) {
						if(scs_obj.SearchByName(name_buf, &native_id, &scs_rec) > 0) {
							//scs_pack.Rec = scs_rec;
							assert(scs_rec.ID == native_id); // @paranoic
							THROW(scs_obj.GetPacket(native_id, &scs_pack) > 0);
						}
						else
							native_id = 0;
					}
					STRNSCPY(scs_pack.Rec.Name, name_buf);
					STRNSCPY(scs_pack.Rec.Symb, code_buf);
					if(r_blk.QuotKindBlkP) {
						int   inner_type = 0;
						QuotKindBlock * p_qk_blk = (QuotKindBlock *)RdB.GetItem(r_blk.QuotKindBlkP, &inner_type);
						if(p_qk_blk && inner_type == obQuotKind)
							scs_pack.Rec.QuotKindID_s = p_qk_blk->NativeID;
					}
					THROW(scs_obj.PutPacket(&native_id, &scs_pack, 0));
					r_blk.NativeID = native_id;
					if(r_blk.NativeID && r_blk.ID) {
						scs_foreign_to_native_assoc.Add(r_blk.ID, r_blk.NativeID);
					}
				}
			}
			THROW(tra.Commit());
		}
		//
		// Теперь, после акцепта всех серий, идентифицируем ссылки на серии карт
		//
		{
			const uint __count = RdB.ScsBlkList.getCount();
			for(uint i = 0; i < __count; i++) {
				SCardSeriesBlock & r_blk = RdB.ScsBlkList.at(i);
				if(r_blk.Flags_ & r_blk.fRefItem) {
					long   native_id = 0;
					if(r_blk.ID && scs_foreign_to_native_assoc.Search(r_blk.ID, &native_id, 0) && native_id) {
						r_blk.NativeID = native_id;
					}
					else {
						PPID   temp_id = 0;
						RdB.GetS(r_blk.NameP, name_buf);
						name_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(name_buf.NotEmptyS() && u_obj.SearchByName(name_buf, &temp_id, 0) > 0) {
							r_blk.NativeID = temp_id;
						}
						else {
							RdB.GetS(r_blk.CodeP, symb_buf);
							symb_buf.Transf(CTRANSF_UTF8_TO_INNER);
							if(symb_buf.NotEmptyS() && u_obj.SearchBySymb(symb_buf, &(temp_id = 0), 0) > 0) {
								r_blk.NativeID = temp_id;
							}
						}
					}
				}
			}
		}
		{
			//
			// Акцепт персональных карт
			//
			SString def_dcard_ser_name;
			SString def_ccard_ser_name;
			SCardTbl::Rec _ex_sc_rec;
			PPPersonPacket psn_pack;
			if(def_dcard_ser_id && scs_obj.Search(def_dcard_ser_id, &scs_rec) > 0) {
				; // @ok
			}
			else {
				long   def_counter = 0;
				def_dcard_ser_id = 0;
				def_dcard_ser_name = "default";
				while(!def_dcard_ser_id && scs_obj.SearchByName(def_dcard_ser_name, 0, &scs_rec) > 0) {
					def_dcard_ser_id = scs_rec.ID;
				}
			}
			if(def_ccard_ser_id && scs_obj.Search(def_ccard_ser_id, &scs_rec) > 0) {
				; // @ok
			}
			else {
				long   def_counter = 0;
				def_ccard_ser_id = 0;
				def_ccard_ser_name = "default-credit";
				while(!def_ccard_ser_id && scs_obj.SearchByName(def_ccard_ser_name, 0, &scs_rec) > 0) {
					if(scs_rec.GetType() == scstCredit)
						def_ccard_ser_id = scs_rec.ID;
					else
						(def_ccard_ser_name = "default-credit").CatChar('-').CatLongZ(++def_counter, 3);
				}
			}
			if(!def_dcard_ser_id || !def_ccard_ser_id) {
				PPTransaction tra(1);
				THROW(tra);
				{
					ScObj.ReadConfig(&sc_cfg);
					if(!def_dcard_ser_id) {
						scs_pack.Init();
						STRNSCPY(scs_pack.Rec.Name, def_dcard_ser_name);
						scs_pack.Rec.SetType(scstDiscount);
						scs_pack.Rec.PersonKindID = PPPRK_CLIENT;
						THROW(scs_obj.PutPacket(&def_dcard_ser_id, &scs_pack, 0));
						sc_cfg.DefSerID = def_dcard_ser_id;
					}
					if(!def_ccard_ser_id) {
						scs_pack.Init();
						STRNSCPY(scs_pack.Rec.Name, def_ccard_ser_name);
						scs_pack.Rec.SetType(scstCredit);
						scs_pack.Rec.PersonKindID = PPPRK_CLIENT;
						THROW(scs_obj.PutPacket(&def_ccard_ser_id, &scs_pack, 0));
						sc_cfg.DefCreditSerID = def_ccard_ser_id;
					}
					THROW(ScObj.WriteConfig(&sc_cfg, 0));
				}
				THROW(tra.Commit());
			}
			{
				PPLoadText(PPTXT_IMPSCARD, wait_msg_buf);
				const uint __count = RdB.SCardBlkList.getCount();
				for(uint i = 0; i < __count; i++) {
					SCardBlock & r_blk = RdB.SCardBlkList.at(i);
					//uint   ref_pos = 0;
					// (very slow) THROW_PP(RdB.SearchRef(obSCard, i, &ref_pos), PPERR_PPPP_INNERREFNF_SC);
					if(r_blk.Flags_ & ObjectBlock::fRefItem) {
						//
						// Для объектов, переданных как ссылка мы должны найти аналоги в нашей БД, но создавать не будем
						//
						RdB.GetS(r_blk.CodeP, temp_buf);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(ScObj.SearchCode(0, temp_buf, &_ex_sc_rec) > 0) {
							//
							// Здесь все просто - нашли по коду, значит наша карта
							// Note: однако, мы абстрагируемся от вероятной сквозной неуникальности номеров карт.
							//
							r_blk.NativeID = _ex_sc_rec.ID;
						}
					}
					else {
						PPSCardPacket sc_pack;
						pretend_obj_list.clear();
						RdB.GetS(r_blk.CodeP, temp_buf);
						temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						STRNSCPY(sc_pack.Rec.Code, temp_buf);
						if(r_blk.Discount != 0.0) {
							sc_pack.Rec.PDis = R0i(r_blk.Discount * 100);
						}
						if(r_blk.SeriesBlkP) {
							int   inner_type = 0;
							SCardSeriesBlock * p_scs_blk = (SCardSeriesBlock *)RdB.GetItem(r_blk.SeriesBlkP, &inner_type);
							if(p_scs_blk && inner_type == obSCardSeries)
								sc_pack.Rec.SeriesID = p_scs_blk->NativeID;
						}
						if(!sc_pack.Rec.SeriesID) {
							assert(def_dcard_ser_id);
							sc_pack.Rec.SeriesID = def_dcard_ser_id;
						}
						if(r_blk.OwnerBlkP) {
							PPPersonKind pk_rec;
							PPID   owner_status_id = PPPRS_PRIVATE;
							int    ref_type = 0;
							PersonBlock * p_psn_blk = (PersonBlock *)RdB.GetItem(r_blk.OwnerBlkP, &ref_type);
							assert(p_psn_blk);
							assert(ref_type == obPerson);
							if(p_psn_blk->NativeID) {
								sc_pack.Rec.PersonID = p_psn_blk->NativeID;
							}
							else {
								const PersonBlock * p_analog = RdB.SearchAnalog_Person(*p_psn_blk);
								if(p_analog) {
									sc_pack.Rec.PersonID = p_analog->NativeID;
									p_psn_blk->NativeID = p_analog->NativeID;
								}
							}
							if(!sc_pack.Rec.PersonID) {
								psn_pack.destroy();
								psn_pack.Rec.Status = owner_status_id;
								RdB.GetS(p_psn_blk->CodeP, code_buf);
								code_buf.Transf(CTRANSF_UTF8_TO_INNER);
								RdB.GetS(p_psn_blk->NameP, temp_buf);
								temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								STRNSCPY(psn_pack.Rec.Name, temp_buf);
								PPID   owner_kind_id = (sc_pack.Rec.SeriesID && scs_obj.Fetch(sc_pack.Rec.SeriesID, &scs_rec) > 0) ? scs_rec.PersonKindID : 0;
								SETIFZ(owner_kind_id, ScObj.GetConfig().PersonKindID);
								SETIFZ(owner_kind_id, PPPRK_CLIENT);
								if(owner_kind_id && pk_obj.Search(owner_kind_id, &pk_rec) > 0) {
									PersonTbl::Rec psn_rec;
									PPID   reg_type_id = pk_rec.CodeRegTypeID;
									temp_id_list.clear();
									if(code_buf.NotEmptyS() && reg_type_id && PsnObj.GetListByRegNumber(reg_type_id, owner_kind_id, code_buf, temp_id_list) > 0) {
										if(temp_id_list.getCount() == 1)
											sc_pack.Rec.PersonID = temp_id_list.getSingle();
										else if(temp_id_list.getCount() > 1) {
											for(uint k = 0; !sc_pack.Rec.PersonID && k < temp_id_list.getCount(); k++) {
												if(PsnObj.Search(temp_id_list.get(k), &psn_rec) > 0 && stricmp866(psn_rec.Name, psn_pack.Rec.Name) == 0)
													sc_pack.Rec.PersonID = psn_rec.ID;
											}
										}
									}
									if(!sc_pack.Rec.PersonID && psn_pack.Rec.Name[0]) {
										temp_id_list.clear();
										temp_id_list.add(owner_kind_id);
										if(PsnObj.SearchFirstByName(psn_pack.Rec.Name, &temp_id_list, 0, &psn_rec) > 0)
											sc_pack.Rec.PersonID = psn_rec.ID;
									}
									if(sc_pack.Rec.PersonID) {
										PPPersonPacket ex_psn_pack;
										THROW(PsnObj.GetPacket(sc_pack.Rec.PersonID, &ex_psn_pack, 0) > 0);
										STRNSCPY(ex_psn_pack.Rec.Name, psn_pack.Rec.Name);
										if(reg_type_id && code_buf.NotEmptyS()) {
											THROW(ex_psn_pack.AddRegister(reg_type_id, code_buf, 1));
										}
										psn_pack.Kinds.addUnique(owner_kind_id);
										THROW(PsnObj.PutPacket(&sc_pack.Rec.PersonID, &ex_psn_pack, 1));
									}
									else {
										if(reg_type_id && code_buf.NotEmptyS()) {
											RegisterTbl::Rec reg_rec;
											MEMSZERO(reg_rec);
											reg_rec.RegTypeID = reg_type_id;
											STRNSCPY(reg_rec.Num, code_buf);
										}
										psn_pack.Kinds.addUnique(owner_kind_id);
										THROW(PsnObj.PutPacket(&sc_pack.Rec.PersonID, &psn_pack, 1));
									}
									p_psn_blk->NativeID = sc_pack.Rec.PersonID;
								}
								else {
									; // @error Не удалось создать персоналию-владельца карты из-за не определенности вида
								}
							}
						}
						if(ScObj.SearchCode(0, sc_pack.Rec.Code, &_ex_sc_rec) > 0) {
							pretend_obj_list.add(_ex_sc_rec.ID);
						}
						//
						if(pretend_obj_list.getCount() == 0) {
							PPID   new_sc_id = 0;
							THROW(ScObj.PutPacket(&new_sc_id, &sc_pack, 1));
							r_blk.NativeID = new_sc_id;
						}
						else if(pretend_obj_list.getCount() == 1) {
							PPID   sc_id = pretend_obj_list.get(0);
							PPSCardPacket ex_sc_pack;
							THROW(ScObj.GetPacket(sc_id, &ex_sc_pack) > 0);
							STRNSCPY(ex_sc_pack.Rec.Code, sc_pack.Rec.Code);
							ex_sc_pack.Rec.SeriesID = sc_pack.Rec.SeriesID;
							ex_sc_pack.Rec.PDis = sc_pack.Rec.PDis;
							ex_sc_pack.Rec.PersonID = sc_pack.Rec.PersonID;
							THROW(ScObj.PutPacket(&sc_id, &ex_sc_pack, 1));
							r_blk.NativeID = sc_id;
						}
						else {
							; // @error импортируемая карта может быть сопоставлена более чем одной карте в бд.
						}
					}
					PPWaitPercent(i+1, __count, wait_msg_buf);
				}
			}
		}
	}
	{
		//
		// Акцепт кассовых сессий
		//
		for(uint i = 0; i < RdB.CSessBlkList.getCount(); i++) {
			CSessionBlock & r_blk = RdB.CSessBlkList.at(i);
			uint   ref_pos = 0;
			THROW_PP(RdB.SearchRef(obCSession, i, &ref_pos), PPERR_PPPP_INNERREFNF_CSESS);
			for(uint ccidx = 0; ccidx < RdB.CcBlkList.getCount(); ccidx++) {
				CCheckBlock & r_cc_blk = RdB.CcBlkList.at(ccidx);
				if(r_cc_blk.CSessionBlkP == ref_pos) {
					uint   cc_ref_pos = 0;
					THROW_PP(RdB.SearchRef(obCCheck, i, &cc_ref_pos), PPERR_PPPP_INNERREFNF_CC);
					for(uint clidx = 0; clidx < RdB.CclBlkList.getCount(); clidx++) {
						CcLineBlock & r_cl_blk = RdB.CclBlkList.at(clidx);
						if(r_cl_blk.CCheckBlkP == cc_ref_pos) {

						}
					}
				}
			}
		}
	}
	CATCHZOK
	if(!silent)
		PPWait(0);
    return ok;
}

void PPPosProtocol::SaxStop()
{
	xmlStopParser(RdB.P_SaxCtx);
}

int SLAPI PPPosProtocol::SaxParseFile(const char * pFileName, int preprocess, int silent)
{
    int    ok = 1;
    SString msg_buf;
	if(!silent)
		PPWait(1);
	xmlSAXHandler saxh;
	MEMSZERO(saxh);
	saxh.startDocument = Scb_StartDocument;
	saxh.endDocument = Scb_EndDocument;
	saxh.startElement = Scb_StartElement;
	saxh.endElement = Scb_EndElement;
	saxh.characters = Scb_Characters;

	PPFormatT(PPTXT_POSPROT_PARSING, &msg_buf, pFileName);
	PPWaitMsg(msg_buf);
	xmlFreeParserCtxt(RdB.P_SaxCtx);
	THROW(RdB.P_SaxCtx = xmlCreateURLParserCtxt(pFileName, 0));
	if(RdB.P_SaxCtx->sax != (xmlSAXHandler *)&xmlDefaultSAXHandler)
		SAlloc::F(RdB.P_SaxCtx->sax);
	RdB.P_SaxCtx->sax = &saxh;
	xmlDetectSAX2(RdB.P_SaxCtx);
	RdB.P_SaxCtx->userData = this;
	RdB.Phase = preprocess ? RdB.phPreprocess : RdB.phProcess;
	RdB.SrcFileName = pFileName;
	xmlParseDocument(RdB.P_SaxCtx);
	THROW_LXML(RdB.P_SaxCtx->wellFormed, RdB.P_SaxCtx);
	THROW(RdB.P_SaxCtx->errNo == 0);
	THROW(!(RdB.State & RdB.stError));
	// (Нельзя сортировать - позиции важны) RdB.SortRefList(); // @v9.8.2
	CATCHZOK
	if(RdB.P_SaxCtx) {
		RdB.P_SaxCtx->sax = 0;
		xmlFreeParserCtxt(RdB.P_SaxCtx);
	}
	RdB.P_SaxCtx = 0;
	if(!silent)
		PPWait(0);
    return ok;
}

void SLAPI PPPosProtocol::DestroyReadBlock()
{
    RdB.Destroy();
}

int SLAPI PPPosProtocol::Helper_GetPosNodeInfo_ForInputProcessing(const PPCashNode * pCnRec, TSVector <PosNodeISymbEntry> & rISymbList, TSVector <PosNodeUuidEntry> & rUuidList)
{
	int    ok = -1;
	SString temp_buf;
	(temp_buf = pCnRec->Symb).Strip();
	if(temp_buf.IsDigit()) {
		long   isymb = temp_buf.ToLong();
		if(isymb > 0) {
			PosNodeISymbEntry new_entry;
			new_entry.PosNodeID = pCnRec->ID;
			new_entry.ISymb = isymb;
			rISymbList.insert(&new_entry);
			ok = 1;
		}
	}
	{
		ObjTagItem tag_item;
		if(PPRef->Ot.GetTag(PPOBJ_CASHNODE, pCnRec->ID, PPTAG_POSNODE_UUID, &tag_item) > 0) {
			S_GUID cn_uuid;
			if(tag_item.GetGuid(&cn_uuid) && !cn_uuid.IsZero()) {
				PosNodeUuidEntry new_entry;
				new_entry.PosNodeID = pCnRec->ID;
				new_entry.Uuid = cn_uuid;
				THROW_SL(rUuidList.insert(&new_entry));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::BackupInputFile(const char * pFileName)
{
	int    ok = 1;
	int    use_arc = 0;
	long   n = 0;
	SString src_file_name;
	SString src_file_ext;
	SPathStruc ps(pFileName);
	src_file_name = ps.Nam;
	src_file_ext = ps.Ext;
	if(!use_arc) {
		SString backup_path;
		ps.Nam = "backup";
		ps.Ext.Z();
		ps.Merge(backup_path);
		THROW_SL(createDir(backup_path));
		{
			int   _found = 0;
			SString to_backup_name;
			do {
				_found = 0;
				(to_backup_name = backup_path).SetLastSlash().Cat(src_file_name);
				if(n)
					to_backup_name.CatChar('-').CatLongZ(n, 4);
				if(src_file_ext.NotEmpty()) {
					if(src_file_ext.C(0) != '.')
						to_backup_name.Dot();
					to_backup_name.Cat(src_file_ext);
				}
				if(fileExists(to_backup_name)) {
					n++;
					_found = 1;
				}
			} while(_found);
			THROW_SL(copyFileByName(pFileName, to_backup_name));
		}
	}
	else {
		SString arc_file_name;
		ps.Nam = "pppp-backup";
		ps.Ext = "zip";
		ps.Merge(arc_file_name);
		{
			SArchive arc;
			SString temp_buf;
			SString to_arc_name;
			THROW_SL(arc.Open(SArchive::tZip, arc_file_name, SFile::mReadWrite));
			{
				const int64 zec = arc.GetEntriesCount();
				int   _found = 0;
				do {
					_found = 0;
					to_arc_name = src_file_name;
					if(n)
						to_arc_name.CatChar('-').CatLongZ(n, 4);
					if(src_file_ext.NotEmpty()) {
						if(src_file_ext.C(0) != '.')
							to_arc_name.Dot();
						to_arc_name.Cat(src_file_ext);
					}
					for(int64 i = 0; !_found && i < zec; i++) {
						arc.GetEntryName(i, temp_buf);
						if(temp_buf.CmpNC(to_arc_name) == 0) {
							n++;
							_found = 1;
						}
					}
				} while(_found);
			}
			THROW_SL(arc.AddEntry(pFileName, to_arc_name, 0));
		}
	}
    CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
		ok = 0;
	ENDCATCH
	return ok;
}

struct PosProtocolFileProcessedEntry {
	SLAPI  PosProtocolFileProcessedEntry()
	{
		THISZERO();
	}
	S_GUID FileUUID;
	LDATETIME FileDtm;
	S_GUID DestRouteUUID;
};

static int SLAPI MakePosProtocolFileProcessedListName(const char * pPath, SString & rFileName)
{
	rFileName.Z();
	SPathStruc ps(pPath);
	ps.Nam = "ppfplist";
	ps.Ext.Z();
	ps.Merge(rFileName);
	return 1;
}

static int SLAPI AppendPosProtocolFileProcessedListEntry(const char * pPath, PosProtocolFileProcessedEntry & rEntry)
{
	int    ok = -1;
	if((!rEntry.FileUUID.IsZero() || checkdate(rEntry.FileDtm.d, 0)) && !rEntry.DestRouteUUID.IsZero()) {
		SString temp_buf;
		MakePosProtocolFileProcessedListName(pPath, temp_buf);
		SFile f_out(temp_buf, SFile::mAppend);
		THROW_SL(f_out.IsValid());
		temp_buf.Z().Cat(rEntry.FileUUID, S_GUID::fmtIDL).Tab().
			Cat(rEntry.FileDtm, DATF_ISO8601|DATF_CENTURY, 0).Tab().Cat(rEntry.DestRouteUUID, S_GUID::fmtIDL).CR();
		THROW_SL(f_out.WriteLine(temp_buf));
		ok = 1;
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
		ok = 0;
	ENDCATCH
	return ok;
}

static int SLAPI ReadPosProtocolFileProcessedList(const char * pPath, TSVector <PosProtocolFileProcessedEntry> & rList)
{
	int    ok = -1;
	SString temp_buf;
	MakePosProtocolFileProcessedListName(pPath, temp_buf);
	if(fileExists(temp_buf)) {
		SFile f_in(temp_buf, SFile::mRead);
		THROW_SL(f_in.IsValid());
		{
			SString line_buf;
			StringSet ss;
			while(f_in.ReadLine(line_buf)) {
				line_buf.Chomp();
				ss.clear();
				line_buf.Tokenize("\t", ss);
				uint   fld_n = 0;
				PosProtocolFileProcessedEntry new_entry;
				int   is_rec_ok = 1;
				for(uint ssp = 0; ss.get(&ssp, temp_buf); fld_n++) {
					if(fld_n == 0) {
						if(!new_entry.FileUUID.FromStr(temp_buf.Strip())) 
							is_rec_ok = 0;
					}
					else if(fld_n == 1) {
						strtodatetime(temp_buf, &new_entry.FileDtm, DATF_ISO8601, 0);
						if(!checkdate(new_entry.FileDtm.d, 0))
							is_rec_ok = 0;
					}
					else if(fld_n == 2) {
						if(!new_entry.DestRouteUUID.FromStr(temp_buf.Strip()))
							is_rec_ok = 0;
					}
				}
				if(is_rec_ok) {
					THROW_SL(rList.insert(&new_entry));
					ok = 1;
				}
			}
		}
	}
	CATCH
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPPosProtocol::ProcessInput(PPPosProtocol::ProcessInputBlock & rPib)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	const char * p_base_name = "pppp";
	const char * p_done_suffix = "-done";
	SString temp_buf;
	StringSet to_remove_file_list;
	if(!(rPib.Flags & rPib.fSilent))
		PPWait(1);
	{
		SFileEntryPool fep;
		TSVector <PosNodeUuidEntry> pos_node_uuid_list;
		TSVector <PosNodeISymbEntry> pos_node_isymb_list;
		StringSet ss_paths;
		TSVector <PosProtocolFileProcessedEntry> processed_file_list;
		{
			PPCashNode cn_rec;
			if(rPib.PosNodeID) {
				PPAsyncCashNode acn_pack;
				PPSyncCashNode scn_pack;
				THROW(CnObj.Search(rPib.PosNodeID, &cn_rec) > 0);
				THROW(Helper_GetPosNodeInfo_ForInputProcessing(&cn_rec, pos_node_isymb_list, pos_node_uuid_list));
				if(cn_rec.Flags & CASHF_ASYNC && CnObj.GetAsync(rPib.PosNodeID, &acn_pack) > 0) {
					if(acn_pack.ImpFiles.NotEmptyS()) {
						StringSet ss_row_paths(';', acn_pack.ImpFiles);
						for(uint ssrp_pos = 0; ss_row_paths.get(&ssrp_pos, temp_buf);) {
							if(isDir(temp_buf.RmvLastSlash()))
								ss_paths.add(temp_buf);
						}
					}
				}
				else if(cn_rec.Flags & CASHF_SYNC && CnObj.GetSync(rPib.PosNodeID, &scn_pack) > 0) {
					if(scn_pack.GetPropString(ACN_EXTSTR_FLD_IMPFILES, temp_buf)) {
						if(isDir(temp_buf.RmvLastSlash()))
							ss_paths.add(temp_buf);
					}
				}
			}
			else {
				for(SEnum en = CnObj.Enum(0); en.Next(&cn_rec) > 0;) {
					THROW(Helper_GetPosNodeInfo_ForInputProcessing(&cn_rec, pos_node_isymb_list, pos_node_uuid_list));
				}
			}
			if(!ss_paths.getCount()) {
				THROW(PPGetPath(PPPATH_IN, temp_buf));
				ss_paths.add(temp_buf.Strip().RmvLastSlash());
			}
		}
		{
			SString in_path;
			SDirEntry de;
			SString done_plus_xml_suffix;
			(done_plus_xml_suffix = p_done_suffix).Dot().Cat(/*"xml"*/"ppyp");
			for(uint ssp_pos = 0; ss_paths.get(&ssp_pos, in_path);) {
				ReadPosProtocolFileProcessedList(in_path, processed_file_list); // @v9.9.12
				(temp_buf = in_path).SetLastSlash().Cat(p_base_name).Cat(/*"*.xml"*/"*.ppyp");
				for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
					if(de.IsFile()) {
						const size_t fnl = sstrlen(de.FileName);
						if(fnl <= done_plus_xml_suffix.Len() || !sstreqi_ascii(de.FileName+fnl-done_plus_xml_suffix.Len(), done_plus_xml_suffix)) {
							THROW_SL(fep.Add(in_path, de));
						}
					}
				}
			}
		}
		if(fep.GetCount()) {
			fep.Sort(SFileEntryPool::scByWrTime/*|SFileEntryPool::scDesc*/);
			//
			S_GUID this_db_uuid;
			SString this_db_symb;
			SString in_file_name;
			DbProvider * p_dict = CurDict;
			if(p_dict) {
				p_dict->GetDbUUID(&this_db_uuid);
				p_dict->GetDbSymb(this_db_symb);
			}
			for(uint i = 0; i < fep.GetCount(); i++) {
				if(fep.Get(i, 0, &in_file_name) && fileExists(in_file_name) && SFile::WaitForWriteSharingRelease(in_file_name, 6000)) { // @v10.0.02 60000-->6000
					DestroyReadBlock();
					PPID   my_cn_id = 0;
					S_GUID  my_pos_node_uuid; 
					int    pr = SaxParseFile(in_file_name, 1 /* preprocess */, BIN(rPib.Flags & rPib.fSilent));
					THROW(pr);
					if(pr > 0) {
						int    is_my_file = 0;
						RouteBlock root_blk;
						for(uint didx = 0; !is_my_file && didx < RdB.DestBlkList.getCount(); didx++) {
							const RouteObjectBlock & r_dest = RdB.DestBlkList.at(didx);
							RdB.GetRouteItem(r_dest, root_blk);
							//
							// Если отправитель указал не пустой UUID это означает, что мы может принять
							// данные только в ту базу, которая соответствует этому UUID (сопоставление по символу невозможно).
							//
							if(!root_blk.Uuid.IsZero()) {
								if(root_blk.Uuid == this_db_uuid)
									is_my_file = 1;
								else {
									for(uint cnuidx = 0; !is_my_file && cnuidx < pos_node_uuid_list.getCount(); cnuidx++) {
										if(root_blk.Uuid == pos_node_uuid_list.at(cnuidx).Uuid) {
											my_cn_id = pos_node_uuid_list.at(cnuidx).PosNodeID;
											my_pos_node_uuid = pos_node_uuid_list.at(cnuidx).Uuid;
											is_my_file = 2;
										}
									}
								}
							}
							else if(!is_my_file) {
								if(root_blk.Code.IsDigit()) {
									long   isymb = root_blk.Code.ToLong();
									uint   isymb_pos = 0;
									if(isymb > 0 && pos_node_isymb_list.lsearch(&isymb, &isymb_pos, CMPF_LONG)) {
										my_cn_id = pos_node_isymb_list.at(isymb_pos).PosNodeID;
										is_my_file = 3;
									}
								}
							}
						}
						if(is_my_file) {
							int   i_ve_allready_processed_file = 0;
							if(!RdB.SrcFileUUID.IsZero() && !my_pos_node_uuid.IsZero() && processed_file_list.getCount()) {
								for(uint pflidx = 0; !i_ve_allready_processed_file && pflidx < processed_file_list.getCount(); pflidx++) {
									const PosProtocolFileProcessedEntry & r_pfentry = processed_file_list.at(pflidx);
									if(r_pfentry.FileUUID == RdB.SrcFileUUID && r_pfentry.DestRouteUUID == my_pos_node_uuid)
										i_ve_allready_processed_file = 1;
								}
							}
							if(!i_ve_allready_processed_file) {
								SETIFZ(my_cn_id, rPib.PosNodeID);
								DestroyReadBlock();
								pr = SaxParseFile(in_file_name, 0 /* !preprocess */, BIN(rPib.Flags & rPib.fSilent));
								THROW(pr);
								if(rPib.Flags & rPib.fStoreReadBlocks) {
									if(!rPib.P_RbList) {
										THROW_MEM(rPib.P_RbList = new TSCollection <ReadBlock>);
									}
									TSCollection <ReadBlock> * p_rb_list = (TSCollection <ReadBlock> *)rPib.P_RbList;
									ReadBlock * p_persistent_rb = p_rb_list->CreateNewItem();
									THROW_SL(p_persistent_rb);
									p_persistent_rb->Copy(RdB);
								}
								{
									for(uint csidx = 0; csidx < RdB.CSessBlkList.getCount(); csidx++) {
										const CSessionBlock & r_cs_blk = RdB.CSessBlkList.at(csidx);
										rPib.SessionCount++;
										rPib.SessionPeriod.AdjustToDate(r_cs_blk.Dtm.d);
									}
								}
								{
									uint   qpb_list_idx = 0;
									RouteBlock src_;
									if(RdB.SrcBlkList.getCount() == 1) {
										RouteObjectBlock & r_blk = RdB.SrcBlkList.at(0);
										RdB.GetRouteItem(r_blk, src_);
										for(uint j = 0; !qpb_list_idx && j < rPib.QpBlkList.getCount(); j++) {
											QueryProcessBlock * p_qpb = rPib.QpBlkList.at(j);
											if(p_qpb->PosNodeID == my_cn_id && p_qpb->R.IsEqual(src_))
												qpb_list_idx = j+1;
										}
										if(!qpb_list_idx) {
											uint _pos = 0;
											QueryProcessBlock * p_new_qpb = rPib.QpBlkList.CreateNewItem(&_pos);
											THROW_SL(p_new_qpb);
											p_new_qpb->PosNodeID = my_cn_id;
											p_new_qpb->R = src_;
											qpb_list_idx = _pos+1;
										}
									}
									if(qpb_list_idx) {
										QueryProcessBlock * p_qpb = rPib.QpBlkList.at(qpb_list_idx-1);
										for(uint qidx = 0; qidx < RdB.QueryList.getCount(); qidx++) {
											const QueryBlock & r_qb = RdB.QueryList.at(qidx);
											THROW_SL(p_qpb->QL.insert(&r_qb));
										}
									}
								}
							}
							{
								int    err_on_accept_occured = 0;
								int    do_backup_file = 0;
								int    do_remove_file = BIN(rPib.Flags & rPib.fRemoveProcessed);
								int    all_receipients_processed_file = 1;
								if(rPib.Flags & rPib.fProcessRefs) {
									if(AcceptData(my_cn_id, BIN(rPib.Flags & rPib.fSilent)))
										do_backup_file = 1;
									else {
										err_on_accept_occured = 1;
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
									}
								}
								if(!err_on_accept_occured) {
									if(rPib.Flags & rPib.fProcessQueries)
										do_backup_file = 1;
									if(rPib.Flags & rPib.fProcessSessions) {
										if(rPib.Flags & rPib.fBackupProcessed)
											do_backup_file = 1;
									}
								}
								if(RdB.DestBlkList.getCount() > 1) {
									//
									// В случае, если получателей файла больше одного (то есть, он нужен еще кому-то)
									// мы предпринимаем специальные меры для регистрации факта обработки файла нами.
									// Это возможно только в том случае, если мы как получатель указаны в файле по GUID'у!
									//
									if(!my_pos_node_uuid.IsZero() && !RdB.SrcFileUUID.IsZero()) {
										TSVector <PosProtocolFileProcessedEntry> local_processed_file_list;
										ReadPosProtocolFileProcessedList(in_file_name, local_processed_file_list); 
										for(uint didx = 0; all_receipients_processed_file && didx < RdB.DestBlkList.getCount(); didx++) {
											const RouteObjectBlock & r_dest = RdB.DestBlkList.at(didx);
											RdB.GetRouteItem(r_dest, root_blk);
											if(!root_blk.Uuid.IsZero() && root_blk.Uuid != my_pos_node_uuid) {
												int  _uuid_found = 0;
												for(uint pflidx = 0; !_uuid_found && pflidx < local_processed_file_list.getCount(); pflidx++) {
													const PosProtocolFileProcessedEntry & r_pfentry = local_processed_file_list.at(pflidx);
													if(r_pfentry.FileUUID == RdB.SrcFileUUID && r_pfentry.DestRouteUUID == root_blk.Uuid)
														_uuid_found = 1;
												}
												if(!_uuid_found)
													all_receipients_processed_file = 0;
											}
										}
									}
									if(!i_ve_allready_processed_file && !err_on_accept_occured) { // Мы уже есть в списке тех, кто обработал файл - не надо ничего добавлять
										// Файл нельзя удалять - он еще не всеми обработан.
										// В этом случае мы должны добавить собственную метку, означающую, что нами файл обработан
										PosProtocolFileProcessedEntry new_pfentry;
										new_pfentry.DestRouteUUID = my_pos_node_uuid;
										new_pfentry.FileUUID = RdB.SrcFileUUID;
										new_pfentry.FileDtm = RdB.SrcFileDtm;
										AppendPosProtocolFileProcessedListEntry(in_file_name, new_pfentry);
									}
								}
								if(do_backup_file) {
									if(!all_receipients_processed_file) {
										//
										// Если файл еще не всеми обработан, но при этом старше 3 суток, то все равно архивируем и удаляем его
										// (вполне возможно, что остальные получатели ждут его в других каталогах).
										//
										if(!!RdB.SrcFileDtm && diffdatetimesec(getcurdatetime_(), RdB.SrcFileDtm) > (3600*24*3))
											all_receipients_processed_file = 1;
									}
									if(all_receipients_processed_file) {
										int    backup_ok = (rPib.Flags & rPib.fBackupProcessed) ? BackupInputFile(in_file_name) : 1;
										if(do_remove_file && backup_ok)
											to_remove_file_list.add(in_file_name);
									}
								}
								DestroyReadBlock();
							}
						}
					}
				}
			}
			if(rPib.Flags & rPib.fProcessQueries) {
				PPIDArray csess_list;
				for(uint bidx = 0; bidx < rPib.QpBlkList.getCount(); bidx++) {
					const QueryProcessBlock * p_qpb = rPib.QpBlkList.at(bidx);
					if(p_qpb) {
						csess_list.clear();
						const PPID cn_id = p_qpb->PosNodeID;
						PPID  sync_cn_id = 0;
						PPID  async_cn_id = 0;
						PPCashNode cn_rec;
						CSessionTbl::Rec cs_rec;
						if(CnObj.Search(cn_id, &cn_rec) > 0) {
							if(cn_rec.Flags & CASHF_SYNC)
								sync_cn_id = cn_id;
							else if(cn_rec.Flags & CASHF_ASYNC)
								async_cn_id = cn_id;
						}
						for(uint qidx = 0; qidx < p_qpb->QL.getCount(); qidx++) {
							const QueryBlock & r_q = p_qpb->QL.at(qidx);
							/*
								qTest = 1, // Тестовый запрос для проверки обмена данными
        						qCSession, // Запрос кассовых сессий
        						qRefs,     // Запрос справочников
							*/
							if(r_q.Q == QueryBlock::qTest) {
							}
							else if(r_q.Q == QueryBlock::qCSession) {
								if(cn_id) {
									if(!r_q.Period.IsZero()) {
										PPViewCSess cs_view;
										CSessFilt cs_filt;
										cs_filt.NodeList_.Add(cn_id);
										cs_filt.Period = r_q.Period;
										if(cs_view.Init_(&cs_filt)) {
											CSessViewItem cs_item;
											for(cs_view.InitIteration(PPViewCSess::ordByDefault); cs_view.NextIteration(&cs_item) > 0;) {
												if(!cs_item.Temporary && cs_item.Incomplete <= 10 && cs_item.ID != cn_rec.CurSessID) {
													csess_list.add(cs_item.ID);
												}
											}
										}
									}
									if(r_q.Flags & QueryBlock::fCSessLast) {
										if(CsObj.P_Tbl->SearchLast(cn_id, 1010, 0, &cs_rec) > 0)
											csess_list.add(cs_rec.ID);
									}
									if(r_q.Flags & QueryBlock::fCSessCurrent) {
										if(sync_cn_id) {
											if(cn_rec.CurSessID && CsObj.Search(cn_rec.CurSessID, &cs_rec) > 0)
												csess_list.add(cs_rec.ID);
										}
										else if(async_cn_id) {
											if(CsObj.P_Tbl->SearchLast(async_cn_id, 10, 0, &cs_rec) > 0)
												csess_list.add(cs_rec.ID);
										}
									}
									if(r_q.CSess) {
										if(r_q.Flags & QueryBlock::fCSessN) { // сессия по номеру
											PPID   cs_id = 0;
											if(CsObj.P_Tbl->SearchByNumber(&cs_id, cn_id, cn_id, r_q.CSess, ZERODATE) > 0)
												csess_list.add(cs_id);
										}
										else { // сессия по идентификатору
											if(CsObj.P_Tbl->Search(r_q.CSess, &cs_rec) > 0) {
												if(cs_rec.CashNodeID == cn_id)
													csess_list.add(cs_rec.ID);
											}
										}
									}
								}
							}
							else if(r_q.Q == QueryBlock::qRefs) {
							}
						}
						if(csess_list.getCount()) {
							csess_list.sortAndUndup();
							RouteBlock rb_src;
							InitSrcRootInfo(cn_id, rb_src);
							ExportPosSession(csess_list, cn_id, &rb_src, &p_qpb->R);
						}
					}
				}
			}
		}
	}
	if(!(rPib.Flags & rPib.fSilent))
		PPWait(0);
	CATCH
		if(rPib.Flags & rPib.fSilent) {
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_USER|LOGMSGF_TIME);
		}
		else
			PPErrorZ();
		ok = 0;
	ENDCATCH
	DestroyReadBlock();
	{
		for(uint ssp = 0; to_remove_file_list.get(&ssp, temp_buf);) {
			SFile::Remove(temp_buf);
		}
	}
	return ok;
}

int SLAPI PPPosProtocol::SelectOutFileName(PPID srcPosNodeID, const char * pInfix, StringSet & rResultSs)
{
	rResultSs.clear();

	int    ok = -1;
	const char * p_base_name = "pppp";
	SString temp_buf;
	SString temp_result_buf;
	SString path;
	StringSet ss_paths(";");
	{
		int    path_done = 0;
		PPCashNode cn_rec;
		if(srcPosNodeID && CnObj.Search(srcPosNodeID, &cn_rec) > 0) {
			if(cn_rec.Flags & CASHF_ASYNC) {
				PPAsyncCashNode acn_pack;
				if(CnObj.GetAsync(srcPosNodeID, &acn_pack) > 0) {
					if(acn_pack.ExpPaths.NotEmpty()) {
						ss_paths.setBuf(acn_pack.ExpPaths);
						path_done = 1;
					}
				}
			}
			else if(cn_rec.Flags & CASHF_SYNC) {
				PPSyncCashNode scn_pack;
				if(CnObj.GetSync(srcPosNodeID, &scn_pack) > 0) {
					if(scn_pack.GetPropString(ACN_EXTSTR_FLD_IMPFILES, temp_buf) > 0 && temp_buf.NotEmptyS()) {
						ss_paths.setBuf(temp_buf);
						path_done = 1;
					}
				}
			}
		}
		if(!path_done) {
			THROW(PPGetPath(PPPATH_OUT, temp_buf));
			ss_paths.setBuf(temp_buf);
		}
	}
	for(uint ssp = 0; ss_paths.get(&ssp, path);) {
		path.RmvLastSlash();
		if(isDir(path) || createDir(path)) {
			long   _seq = 0;
			do {
				temp_buf = p_base_name;
				if(!isempty(pInfix))
					temp_buf.CatChar('-').Cat(pInfix);
				if(_seq)
					temp_buf.CatChar('-').Cat(_seq);
				temp_buf.Dot().Cat(/*"xml"*/"ppyp");
				(temp_result_buf = path).SetLastSlash().Cat(temp_buf);
				_seq++;
			} while(::fileExists(temp_result_buf));
			rResultSs.add(temp_result_buf);
			ok = 1;
		}
		else {
			// @todo log message
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPPosProtocol::ExportPosSession(const PPIDArray & rSessList, PPID srcPosNodeID, const PPPosProtocol::RouteBlock * pSrc, const PPPosProtocol::RouteBlock * pDestination)
{
	int    ok = -1;
	SString out_file_name;
	SString temp_buf;
	PPPosProtocol::WriteBlock wb;
	PPMakeTempFileName("pppp", /*"xml"*/"ppyp", 0, out_file_name);
	{
		THROW(StartWriting(out_file_name, wb));
		{
			if(pSrc) {
				THROW(WriteRouteInfo(wb, "source", *pSrc));
			}
			else {
				PPPosProtocol::RouteBlock rb_src;
				InitSrcRootInfo(srcPosNodeID, rb_src);
				THROW(WriteRouteInfo(wb, "source", rb_src));
			}
			//THROW(WriteSourceRoute(wb));
			if(pDestination) {
				THROW(WriteRouteInfo(wb, "destination", *pDestination));
			}
		}
		{
			PPObjCSession cs_obj;
			CSessionTbl::Rec cs_rec;
			for(uint i = 0; i < rSessList.getCount(); i++) {
				const PPID sess_id = rSessList.get(i);
				if(cs_obj.Search(sess_id, &cs_rec) > 0) {
					if(!WriteCSession(wb, "csession", cs_rec)) {
						; // @todo logerror
					}
				}
			}
		}
		FinishWriting(wb);
	}
	THROW(TransportFileOut(out_file_name, srcPosNodeID, "csess"));
	ok = 1;
	CATCHZOK
	return ok;
}

//static
int SLAPI PPPosProtocol::EditPosQuery(TSVector <PPPosProtocol::QueryBlock> & rQList) // @v9.8.4 TSArray-->TSVector
{
	enum {
		qvLastSession = 1,
		qvCurrSession,
		qvSessByPeriod,
		qvSessByNumber,
		qvSessByID,
		qvRefs,
		qvTest
	};
	class PosQueryDialog : public TDialog {
	public:
        SLAPI  PosQueryDialog() : TDialog(DLG_POSNQUERY)
        {
        	SetupCalPeriod(CTLCAL_POSNQUERY_PERIOD, CTL_POSNQUERY_PERIOD);
        }
        int    SLAPI setDTS(const TSVector <PPPosProtocol::QueryBlock> * pData)
        {
        	int    ok = 1;
        	//RVALUEPTR(Data, pData);
        	AddClusterAssocDef(CTL_POSNQUERY_Q, 0, qvLastSession);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 1, qvCurrSession);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 2, qvSessByPeriod);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 3, qvSessByNumber);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 4, qvSessByID);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 5, qvRefs);
        	AddClusterAssoc(CTL_POSNQUERY_Q, 6, qvTest);
        	SetClusterData(CTL_POSNQUERY_Q, qvLastSession);
			SetupCtrls();
        	return ok;
        }
        int    SLAPI getDTS(TSVector <PPPosProtocol::QueryBlock> * pData)
        {
        	int    ok = 1;
			uint   sel = 0;
			long   qv = 0;
			SString input_buf;
			SString temp_buf;
			Data.freeAll();
			{
				getCtrlString(CTL_POSNQUERY_N, input_buf);
				StringSet ss;
				LongArray n_list;
				input_buf.Tokenize(";,", ss);
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					const long n = temp_buf.ToLong();
					if(n > 0)
						n_list.add(n);
				}
				if(n_list.getCount()) {
					n_list.sortAndUndup();
					// @todo
				}
			}
			GetClusterData(sel = CTL_POSNQUERY_Q, &qv);
			switch(qv) {
				case qvLastSession:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qCSession;
						qb.Flags |= qb.fCSessLast;
						Data.insert(&qb);
					}
					break;
				case qvCurrSession:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qCSession;
						qb.Flags |= qb.fCSessCurrent;
						Data.insert(&qb);
					}
					break;
				case qvSessByPeriod:
					{
						PPPosProtocol::QueryBlock qb;
						THROW(GetPeriodInput(this, sel = CTL_POSNQUERY_PERIOD, &qb.Period));
						THROW_PP(qb.Period.low && qb.Period.upp, PPERR_INVPERIOD);
						qb.Q = qb.qCSession;
						Data.insert(&qb);
					}
					break;
				case qvSessByNumber:
				case qvSessByID:
					{
						LongArray n_list;
						getCtrlString(sel = CTL_POSNQUERY_SESSL, input_buf);
						StringSet ss;
						input_buf.Tokenize(";,", ss);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							const long n = temp_buf.ToLong();
							if(n > 0)
								n_list.add(n);
						}
						THROW_PP(n_list.getCount(), PPERR_USERINPUT);
						n_list.sortAndUndup();
						for(uint i = 0; i < n_list.getCount(); i++) {
							PPPosProtocol::QueryBlock qb;
							qb.Q = qb.qCSession;
							qb.CSess = n_list.get(i);
							if(qv == qvSessByNumber)
								qb.Flags |= qb.fCSessN;
							Data.insert(&qb);
						}
					}
					break;
				case qvRefs:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qRefs;
						Data.insert(&qb);
					}
					break;
				case qvTest:
					{
						PPPosProtocol::QueryBlock qb;
						qb.Q = qb.qTest;
						Data.insert(&qb);
					}
					break;
				default:
					CALLEXCEPT_PP(PPERR_USERINPUT);
					break;
			}
        	ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
        	return ok;
        }
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_POSNQUERY_Q)) {
				SetupCtrls();
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			long   qv = 0;
            GetClusterData(CTL_POSNQUERY_Q, &qv);
			disableCtrl(CTL_POSNQUERY_PERIOD, (qv != qvSessByPeriod));
			disableCtrl(CTL_POSNQUERY_SESSL, !oneof2(qv, qvSessByNumber, qvSessByID));
		}
		TSVector <PPPosProtocol::QueryBlock> Data; // @v9.8.4 TSArray-->TSVector
	};
	DIALOG_PROC_BODY(PosQueryDialog, &rQList);
}

int SLAPI RunInputProcessThread(PPID posNodeID)
{
	class PosInputProcessThread : public PPThread {
	public:
		struct InitBlock {
			InitBlock(PPID posNodeID) : PosNodeID(posNodeID), ForcePeriodMs(0)
			{
			}
			SString DbSymb;
			SString UserName;
			SString Password;
			PPID   PosNodeID;
			uint   ForcePeriodMs; // Период форсированной проверки входящего каталога (ms)
		};
		SLAPI PosInputProcessThread(const InitBlock & rB) : PPThread(PPThread::kPpppProcessor, 0, 0), IB(rB)
		{
			InitStartupSignal();
		}
	private:
		void SLAPI Startup()
		{
			PPThread::Startup();
			SignalStartup();
		}
		void   FASTCALL DoProcess(PPPosProtocol & rPppp)
		{
			PPPosProtocol::ProcessInputBlock pib;
			pib.PosNodeID = IB.PosNodeID;
			pib.Flags = (pib.fProcessRefs|pib.fProcessQueries|pib.fBackupProcessed|pib.fRemoveProcessed|pib.fSilent);
			rPppp.ProcessInput(pib);
		}
		virtual void Run()
		{
			DirChangeNotification * p_dcn = 0;
			SString msg_buf, temp_buf;
			STimer timer;
			Evnt   stop_event(SLS.GetStopEventName(temp_buf), Evnt::modeOpen);
			THROW(DS.Login(IB.DbSymb, IB.UserName, IB.Password));
			IB.Password.Obfuscate();
			{
        		//
        		// Login has been done
        		//
        		SString in_path;
        		PPObjCashNode cn_obj;
				PPSyncCashNode cn_pack;
				THROW(cn_obj.GetSync(IB.PosNodeID, &cn_pack) > 0);
				if(cn_pack.GetPropString(ACN_EXTSTR_FLD_IMPFILES, temp_buf) > 0 && isDir(temp_buf.RmvLastSlash())) { // @v9.9.12 RmvLastSlash()
					in_path = temp_buf;
				}
				else {
					PPGetPath(PPPATH_IN, in_path);
				}
				in_path.RmvLastSlash();
				if(!isDir(in_path)) {

				}
				else {
					uint   local_force_period_ms = 0;
					PPPosProtocol pppp;
					DoProcess(pppp); // Первоначальная обработка входящих данных
					DirChangeNotification * p_dcn = 0; // new DirChangeNotification(in_path, 0, FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME);
					//THROW(p_dcn);
					for(int stop = 0; !stop;) {
						int    h_count = 0;
						int    evidx_stop = -1;
						int    evidx_dcn = -1;
						int    evidx_forceperiod = -1;
						int    evidx_localforceperiod = -1;
						HANDLE h_list[32];
						{
							evidx_stop = h_count++;
							h_list[evidx_stop] = stop_event;
						}
						if(p_dcn) {
							evidx_dcn = h_count++;
							h_list[evidx_dcn] = *p_dcn;
						}
						else if(local_force_period_ms == 0)
							local_force_period_ms = 10000;
						if(local_force_period_ms && (!IB.ForcePeriodMs || local_force_period_ms < IB.ForcePeriodMs)) {
							LDATETIME dtm = getcurdatetime_();
							dtm.addsec(local_force_period_ms / 1000);
							timer.Set(dtm, 0);
							{
								evidx_localforceperiod = h_count++;
								h_list[evidx_localforceperiod] = timer;
							}
							local_force_period_ms = 0;
						}
						else if(IB.ForcePeriodMs) {
							LDATETIME dtm = getcurdatetime_();
							dtm.addsec(IB.ForcePeriodMs / 1000);
							timer.Set(dtm, 0);
							{
								evidx_forceperiod = h_count++;
								h_list[evidx_forceperiod] = timer;
							}
							local_force_period_ms = 0;
						}
						uint   r = WaitForMultipleObjects(h_count, h_list, 0, INFINITE);
						if(evidx_stop >= 0 && r == (WAIT_OBJECT_0 + evidx_stop)) { // stop event
							stop = 1; // quit loop
						}
						else if(evidx_dcn >= 0 && r == (WAIT_OBJECT_0 + evidx_dcn)) { // file created
							DoProcess(pppp);
							p_dcn->Next();
						}
						else if(evidx_localforceperiod >= 0 && (r == (WAIT_OBJECT_0 + evidx_localforceperiod))) { // timer
							DoProcess(pppp);
						}
						else if(evidx_forceperiod >= 0 && (r == (WAIT_OBJECT_0 + evidx_forceperiod))) { // timer
							DoProcess(pppp);
						}
						else if(r == WAIT_FAILED) {
							// error
						}
					}
				}
			}
			CATCH
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_LASTERR);
			ENDCATCH
			delete p_dcn;
			DS.Logout();
		}
		InitBlock IB;
	};
	int    ok = -1;

	TSCollection <PPThread::Info> thread_info_list;
	DS.GetThreadInfoList(PPThread::kPpppProcessor, thread_info_list);
	if(!thread_info_list.getCount()) {
		Reference * p_ref = PPRef;
		PPID   user_id = 0;
		PPSecur usr_rec;
		char    pw[128];
		SString db_symb;
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		THROW(cn_obj.Search(posNodeID, &cn_rec) > 0);
		{
			DbProvider * p_dict = CurDict;
			CALLPTRMEMB(p_dict, GetDbSymb(db_symb));
		}
		{
			ObjTagItem tag_item;
			S_GUID host_uuid;
			if(p_ref->Ot.GetTag(PPOBJ_CASHNODE, posNodeID, PPTAG_POSNODE_HOSTUUID, &tag_item) > 0 && tag_item.GetGuid(&host_uuid) > 0) {
				const PPThreadLocalArea & r_tla = DS.GetConstTLA();
				PosInputProcessThread::InitBlock ib(posNodeID);
				ib.ForcePeriodMs = 15000;
				ib.DbSymb = db_symb;
				ib.UserName = r_tla.UserName;
				THROW(p_ref->SearchName(PPOBJ_USR, &user_id, ib.UserName, &usr_rec) > 0);
				Reference::GetPassword(&usr_rec, pw, sizeof(pw));
				ib.Password = pw;
				memzero(pw, sizeof(pw));
				{
					PosInputProcessThread * p_sess = new PosInputProcessThread(ib);
					p_sess->Start(1);
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
