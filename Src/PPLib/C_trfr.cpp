// C_TRFR.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2020, 2021, 2023, 2024
// @codepage UTF-8
// Процедуры корректировки товарных проводок
//
#include <pp.h>
#pragma hdrstop
//
// Descr: Корректировка ошибок в лотах и товарных строках, возникших из-за ошибок объединения документов
//   внутреннего перемещения.
//
int Transfer::CorrectIntrUnite()
{
	int    ok = 1;
	SString fmt_buf, msg_buf;
	PPObjBill * p_bill_obj = BillObj;
	ReceiptTbl::Key4 k4;
	BExtQuery q(&Rcpt, 4);
	PPLogger logger;
	q.selectAll().where(Rcpt.PrevLotID > 0L);
	MEMSZERO(k4);
	k4.PrevLotID = 1;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(q.initIteration(false, &k4, spGe); q.nextIteration() > 0;) {
			const  PPID prev_lot_id = Rcpt.data.PrevLotID;
			if(prev_lot_id) {
				const  PPID lot_id = Rcpt.data.ID;
				const  PPID bill_id = Rcpt.data.BillID;
				int r = p_bill_obj->Search(bill_id, 0);
				THROW(r);
				if(r < 0) {
					DBRowId trfr_pos, rcpt_pos;
					ReceiptTbl::Rec lot_rec, prev_lot_rec;
					TransferTbl::Rec trfr_rec, org_trfr_rec;
					BillTbl::Rec org_bill_rec;
					Rcpt.copyBufTo(&lot_rec);
					DateIter di; // PPFormat
					int    corrected = 0;
					// Обнаружен порожденный лот с висячей ссылкой на документ: @date @goods @real
					PPFormatT(PPTXT_CORINTRUN_DETECTEDLOT, &msg_buf, lot_rec.Dt, lot_rec.GoodsID, lot_rec.Quantity);
					logger.Log(msg_buf);
					//
					THROW_DB(q.getRecPosition(&rcpt_pos));
					if(EnumByLot(lot_id, &di, &trfr_rec) > 0 && trfr_rec.Flags & PPTFR_RECEIPT && trfr_rec.BillID == bill_id && trfr_rec.Reverse == 1) {
						// PPTXT_CORINTRUN_DETECTEDTRFR    "Найдена запись Transfer, сгенерировшая лот: @date @goods @real"
						PPFormatT(PPTXT_CORINTRUN_DETECTEDTRFR, &msg_buf, trfr_rec.Dt, trfr_rec.GoodsID, trfr_rec.Quantity);
						logger.Log(msg_buf);
						//
						THROW_DB(getPosition(&trfr_pos));
						if(Rcpt.Search(prev_lot_id, &prev_lot_rec) > 0) {
							// PPTXT_CORINTRUN_PREVLOTFOUND    "Найден родительский лот: @date @goods @bill"
							PPFormatT(PPTXT_CORINTRUN_PREVLOTFOUND, &msg_buf, prev_lot_rec.Dt, prev_lot_rec.GoodsID, prev_lot_rec.BillID);
							logger.Log(msg_buf);
							//
							di.Init(trfr_rec.Dt, trfr_rec.Dt);
							while(EnumByLot(prev_lot_id, &di, &org_trfr_rec) > 0) {
								if(p_bill_obj->Search(org_trfr_rec.BillID, &org_bill_rec) > 0) {
									if(IsIntrOp(org_bill_rec.OpID) == INTREXPND) {
										int r2 = SearchByBill(org_trfr_rec.BillID, 1, org_trfr_rec.RByBill, 0);
										THROW(r2);
										if(r2 < 0) {
											//
											// Нашли оригинальную запись, к которой должна быть привязана trfr_rec
											//
											THROW_DB(getDirectForUpdate(0, 0, trfr_pos));
											copyBufTo(&trfr_rec);
											trfr_rec.BillID = org_trfr_rec.BillID;
											trfr_rec.RByBill = org_trfr_rec.RByBill;
											trfr_rec.Quantity = fabs(org_trfr_rec.Quantity);
											THROW_DB(updateRecBuf(&trfr_rec)); // @sfu
											//
											THROW_DB(Rcpt.getDirectForUpdate(0, 0, rcpt_pos));
											Rcpt.copyBufTo(&lot_rec);
											lot_rec.BillID = org_trfr_rec.BillID;
											lot_rec.Quantity = fabs(org_trfr_rec.Quantity);
											THROW_DB(Rcpt.updateRecBuf(&lot_rec)); // @sfu
											//
											// PPTXT_CORINTRUN_CORRECTED       "Ошибка исправлена"
											logger.Log(PPLoadTextS(PPTXT_CORINTRUN_CORRECTED, fmt_buf));
											//
											corrected = 1;
										}
									}
								}
								else {
									// PPTXT_CORINTRUN_ERRBILLNFOUND   "Попутная ошибка: не найден документ id=@int для операции по лоту"
									PPFormatT(PPTXT_CORINTRUN_ERRBILLNFOUND, &msg_buf, org_trfr_rec.BillID);
									logger.Log(msg_buf);
								}
							}
						}
						else {
							PPFormatT(PPTXT_CORINTRUN_ERRPARLOTNFOUND, &msg_buf, prev_lot_id); // "Не найден родительский лот id=@int"
							logger.Log(msg_buf);
						}
					}
					else {
						logger.Log(PPLoadTextS(PPTXT_CORINTRUN_ERRTRFRNFOUND, fmt_buf)); // "Не найдена запись Transfer, сгегерировшая лот"
					}
					if(!corrected) {
						logger.Log(PPLoadTextS(PPTXT_CORINTRUN_NOTCORRECTED, fmt_buf)); // "Ошибка НЕ исправлена"
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

int Transfer::CorrectReverse()
{
	struct Param {
		enum {
			fRecover = 0x0001
		};
		DateRange Period;
		long   Flags;
		SString LogFileName;
	};
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	int    do_process = 0;
	Param param;
	param.Period.Z();
	param.Flags = 0;
	param.LogFileName = "trfr_rev.log";
	{
		TDialog * dlg = new TDialog(DLG_CORTRFRREV);
		THROW(CheckDialogPtr(&dlg));
		dlg->SetupCalPeriod(CTLCAL_CORTRFRREV_PERIOD, CTL_CORTRFRREV_PERIOD);
		SetPeriodInput(dlg, CTL_CORTRFRREV_PERIOD, &param.Period);
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORTRFRREV_LOG, CTL_CORTRFRREV_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		//PPGetFileName(PPFILNAM_ABSLOTS_LOG, log_fname);
		dlg->setCtrlString(CTL_CORTRFRREV_LOG, param.LogFileName);
		dlg->setCtrlUInt16(CTL_CORTRFRREV_FLAGS, BIN(param.Flags & Param::fRecover));
		while(!do_process && ExecView(dlg) == cmOK) {
			if(!GetPeriodInput(dlg, CTL_CORTRFRREV_PERIOD, &param.Period))
				PPError();
			else {
				dlg->getCtrlString(CTL_CORTRFRREV_LOG, param.LogFileName);
				SETFLAG(param.Flags, Param::fRecover, BIN(dlg->getCtrlUInt16(CTL_CORTRFRREV_FLAGS)));
				do_process = 1;
			}
		}
		delete dlg;
		dlg = 0;
	}
	if(do_process) {
		SString msg_buf, fmt_buf;
		DBRowId pos;
		TransferTbl::Key1 k1;
		PPLogger logger;

		PPWaitStart();

		PPTransaction tra(1);
		THROW(tra);

		MEMSZERO(k1);
		k1.Dt = param.Period.low;
		BExtQuery q(this, 1, 256);
		q.selectAll().where(daterange(this->Dt, &param.Period) && this->Reverse == (long)1);
		for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
			TransferTbl::Rec rec, correct_rec;
			PPWaitDate(data.Dt);
			copyBufTo(&rec);
			copyBufTo(&correct_rec);
			THROW_DB(getPosition(&pos));
			TransferTbl::Key0 k0;
			MEMSZERO(k0);
			k0.BillID = rec.BillID;
			k0.RByBill = rec.RByBill;
			k0.Reverse = 0;
			if(search(0, &k0, spEq)) {
				int    err = 0;
				//
				// PPTXT_TRFRREVERR_INVBILL     "Несоответствие документа, владеющего зеркальной строкой Transfer {@bill; @int} (@bill)"
				// PPTXT_TRFRREVERR_INVQTTY     "Несоответствие количества в зеркальной строке Transfer {@bill; @int} (@real # @real)"
				// PPTXT_TRFRREVERR_INVCOST     "Несоответствие цены поступления в зеркальной строке Transfer {@bill; @int} (@real # @real)"
				// PPTXT_TRFRREVERR_INVPRICE    "Несоответствие чистой цены реализации в зеркальной строке Transfer {@bill; @int} (@real # @real)"
				//
				if(data.BillID != rec.BillID) {
					logger.Log(PPFormatT(PPTXT_TRFRREVERR_INVBILL, &msg_buf, rec.BillID, rec.RByBill, data.BillID));
					correct_rec.BillID = data.BillID;
					err = 2;
				}
				if(data.Quantity != -rec.Quantity) {
					logger.Log(PPFormatT(PPTXT_TRFRREVERR_INVQTTY, &msg_buf, rec.BillID, rec.RByBill, rec.Quantity, -data.Quantity));
					err = 1;
				}
				if(TR5(data.Cost) != TR5(rec.Cost)) {
					logger.Log(PPFormatT(PPTXT_TRFRREVERR_INVCOST, &msg_buf, rec.BillID, rec.RByBill, rec.Cost, data.Cost));
					err = 1;
				}
				if(TR5(data.Price-data.Discount) != TR5(rec.Price-rec.Discount)) {
					logger.Log(PPFormatT(PPTXT_TRFRREVERR_INVPRICE, &msg_buf, rec.BillID, rec.RByBill, (rec.Price-rec.Discount), (data.Price-data.Discount)));
					err = 1;
				}
				if(param.Flags & Param::fRecover) {
					if(err == 1) {
						int    _rbb = rec.RByBill - 1;
						PPTransferItem ti;
						PPObjBill::TBlock tb_; // @v8.0.3
						THROW(p_bobj->BeginTFrame(rec.BillID, tb_)); // @v8.0.3
						if(EnumItems(rec.BillID, &_rbb, &ti) > 0 && UpdateItem(&ti, tb_.Rbb(), 1, 0)) {
							// recovered
						}
						else
							logger.LogLastError();
						THROW(p_bobj->FinishTFrame(rec.BillID, tb_)); // @v8.0.3
					}
				}
			}
			else if(BTRNFOUND) {
				// PPTXT_TRFRREVERR_LOSTREC  "Висячая зеркальная строка Transfer {@bill; @int; @goods}"
				logger.Log(PPFormatT(PPTXT_TRFRREVERR_LOSTREC, &msg_buf, rec.BillID, rec.RByBill, rec.GoodsID));
				if(param.Flags & Param::fRecover) {
					if(!RemoveItem(rec.BillID, 1, rec.RByBill, 0))
						logger.LogLastError();
				}
			}
			else {
				PPSetErrorDB();
				logger.LogLastError();
			}
		}
		THROW(tra.Commit());
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int RecoverAbsenceLots()
{
	int    ok = -1, valid_data = 0;
	PPObjBill * p_bobj = BillObj;
	TDialog * dlg = 0;
	TransferTbl::Key1 k1;
	Transfer * trfr = p_bobj->trfr;
	PPLogger logger;

	SString log_fname;
	DateRange period;
	int    correct = 1;
	ushort v = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_CABSLOTS))));
	dlg->SetupCalPeriod(CTLCAL_CABSLOTS_PERIOD, CTL_CABSLOTS_PERIOD);
	period.Z();
	SetPeriodInput(dlg, CTL_CABSLOTS_PERIOD, &period);
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CABSLOTS_LOG, CTL_CABSLOTS_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	PPGetFileName(PPFILNAM_ABSLOTS_LOG, log_fname);
	dlg->setCtrlString(CTL_CABSLOTS_LOG, log_fname);
	dlg->setCtrlUInt16(CTL_CABSLOTS_FLAGS, BIN(correct));
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		if(!GetPeriodInput(dlg, CTL_CABSLOTS_PERIOD, &period))
			PPError();
		else {
			dlg->getCtrlString(CTL_CABSLOTS_LOG, log_fname);
			correct = BIN(dlg->getCtrlUInt16(CTL_CABSLOTS_FLAGS));
			valid_data = 1;
		}
	}
	delete dlg;
	dlg = 0;
	if(valid_data) {
		SString msg_buf, log_buf;
		IterCounter cntr;
		PPWaitStart();
		if(period.IsZero())
			PPInitIterCounter(cntr, trfr);
		else {
			BExtQuery q(trfr, 1, 128);
			q.select(trfr->Dt, 0L).where(daterange(trfr->Dt, &period));
			MEMSZERO(k1);
			k1.Dt = period.low;
			cntr.Init(q.countIterations(0, &k1, spGt));
		}
		{
			PPTransaction tra(1);
			THROW(tra);
			MEMSZERO(k1);
			k1.Dt = period.low;
			while(trfr->search(1, &k1, spGt) && (!period.upp || k1.Dt <= period.upp)) {
				DBRowId pos;
				TransferTbl::Rec rec, mirror;
				trfr->copyBufTo(&rec);
				THROW_DB(trfr->getPosition(&pos));
				if(rec.Flags & PPTFR_RECEIPT && !(rec.Flags & PPTFR_ACK) && trfr->Rcpt.Search(rec.LotID) < 0) {
					long   oprno = rec.OprNo;
					PPID   prev_lot_id = 0, force_lot_id = 0;
					BillTbl::Rec bill_rec;
					PPTransferItem ti;

					ti.SetupByRec(&rec);
					force_lot_id = ti.LotID;
					ti.LotID = 0;
					if(p_bobj->Search(rec.BillID, &bill_rec) > 0) {
						// Отсутствующий лот {@goods - @loc - @int @bill}
						logger.Log(PPFormatS(PPMSG_ERROR, PPERR_ABSLOTS, &log_buf, ti.GoodsID, ti.LocID, ti.BillID, ti.BillID));
						if(correct) {
							if(!trfr->data.Reverse)
								ti.Suppl = bill_rec.Object;
	   						else if(trfr->SearchMirror(ti.Date, oprno, &mirror) > 0) {
								if(trfr->Rcpt.Search(mirror.LotID) > 0) {
									prev_lot_id = mirror.LotID;
									ti.Suppl = trfr->Rcpt.data.SupplID;
								}
							}
							ti.LotID = prev_lot_id;
							THROW(trfr->AddLotItem(&ti, force_lot_id));
							if(trfr->data.LotID == 0) {
								THROW_DB(trfr->getDirect(1, 0, pos));
								trfr->data.LotID = ti.LotID;
								THROW_DB(trfr->updateRec());
							}
						}
					}
					else {
						logger.Log(PPFormatS(PPMSG_ERROR, PPERR_ABSLOTBILL, &log_buf, rec.BillID, ti.GoodsID, ti.LocID, force_lot_id));
					}
				}
				PPWaitPercent(cntr.Increment());
			}
			THROW(tra.Commit());
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	logger.Save(log_fname, 0);
	return ok;
}

int Transfer::CorrectByLot(PPID lot, int (*MsgProc)(int err, PPID lot, const TransferTbl::Rec*))
{
	int    ok = 1, ta = 0;
	PPObjBill * p_bobj = BillObj;
	int    reply = 1;
	int    closed;
	int    err;
	int    cost_err, price_err;
	int    reval = 0;
	double rest = 0.0;
	LDATE  dt = ZERODATE;
	long   oprno = 0;
	PPID   billID;
	int    _rbb;
	ReceiptTbl::Rec rcptr, srr;
	PPTransferItem ti;
	THROW(PPStartTransaction(&ta, 1));
	THROW(Rcpt.Search(lot, &rcptr) > 0);
	srr = rcptr;
	THROW(reval = SearchReval(lot, rcptr.Dt));
	if(reval < 0)
		reval = 0;
	if(EnumByLot(lot, &dt, &oprno) > 0) {
		if(data.Quantity != rcptr.Quantity) {
			reply = MsgProc ? MsgProc(1, lot, &data) : 1;
			if(reply > 0) {
				rcptr.Quantity = data.Quantity;
				srr = rcptr;
				THROW(Rcpt.Update(lot, &rcptr, 0));
			}
		}
		if(reply >= 0) {
			do {
				rest += data.Quantity;
				cost_err = price_err = 0;
				if((err = 2, data.Rest != rest) || (err = 4, cost_err) || (err = 5, price_err)) {
					reply = MsgProc ? MsgProc(err, lot, &data) : 1;
					if(reply > 0) {
						if(price_err || cost_err) {
							billID = data.BillID;
							PPObjBill::TBlock tb_; // @v8.0.3
							THROW(p_bobj->BeginTFrame(billID, tb_)); // @v8.0.3
							_rbb = data.RByBill - 1;
							THROW(EnumItems(billID, &_rbb, &ti) > 0);
							ti.Cost  = R5(rcptr.Cost);
							ti.Price = R5(rcptr.Price);
							THROW(UpdateItem(&ti, tb_.Rbb(), fUpdEnableUpdChildLot, 0));
							THROW(p_bobj->RecalcTurns(billID, 0, 0));
							THROW(p_bobj->FinishTFrame(billID, tb_)); // @v8.0.3
						}
						else {
							data.Rest = rest;
							THROW_DB(updateRec());
						}
					}
					else if(reply < 0)
						break;
				}
			} while(EnumByLot(lot, &dt, &oprno) > 0);
			if(reply >= 0) {
				closed = BIN(rest == 0);
				if(rcptr.Rest != rest || (rcptr.Closed && !closed && !(rcptr.Flags & LOTF_CLOSEDORDER)) || (!rcptr.Closed && closed)) {
					reply = MsgProc ? MsgProc(3, lot, &data) : 1;
					if(reply > 0) {
						rcptr.Closed = closed;
						rcptr.Rest = rest;
						srr = rcptr;
						THROW(Rcpt.Update(lot, &rcptr, 0));
					}
				}
			}
		}
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	if(ok)
		return (reply < 0) ? -1 : 1;
	return 0;
}

int Transfer::CorrectCurRest(PPID goodsID, const PPIDArray * pLocList, PPLogger * pLogger, int correct)
{
	int    ok = -1, ta = 0;
	SString fmt_buf, log_msg;
	GoodsRestParam p;
	PPIDArray temp_loc_list;
	if(pLocList == 0) {
		PPObjLocation loc_obj;
		loc_obj.GetWarehouseList(&temp_loc_list, 0);
		pLocList = &temp_loc_list;
	}
	for(uint j = 0; j < pLocList->getCount(); j++) {
		p.LocID = pLocList->get(j);
		for(uint i = 0; i < 2; i++) {
			CurRestTbl::Key0 k;
			p.GoodsID = i ? -goodsID : goodsID;
			THROW(GetRest(p));
			k.GoodsID = p.GoodsID;
			k.LocID   = p.LocID;
			const double rest = R6(p.Total.Rest);
			int   empty = 0; // Признак того, что при нулевом остатке на складе нет ни одного лота
			if(rest == 0.0) {
				ReceiptTbl::Rec lot_rec;
				int r = Rcpt.GetLastLot(p.GoodsID, p.LocID, MAXDATE, &lot_rec);
				THROW(r);
				if(r < 0)
					empty = 1;
			}
			if(CRest.search(0, &k, spEq)) {
				if(rest != CRest.data.Rest || empty) {
					if(pLogger) {
						if(!empty)
							PPLoadString(PPMSG_ERROR, PPERR_CRESTCORRUPT_NEQ, fmt_buf);
						else
							PPLoadString(PPMSG_ERROR, PPERR_CRESTCORRUPT_EXREC, fmt_buf);
						pLogger->Log(PPFormat(fmt_buf, &log_msg, p.LocID, goodsID, CRest.data.Rest, rest));
					}
					if(correct) {
						THROW(PPStartTransaction(&ta, 1));
						THROW_DB(CRest.searchForUpdate(0, &k, spEq));
						CRest.data.Rest = rest;
						if(!empty) {
							CRest.data.Rest = rest;
							THROW_DB(CRest.updateRec()); // @sfu
						}
						else {
							THROW_DB(CRest.deleteRec()); // @sfu
						}
						THROW(PPCommitWork(&ta));
						ok = 2;
					}
					else
						ok = 1;
				}
			}
			else if(BTRNFOUND) {
				if(!empty) {
					CALLPTRMEMB(pLogger, Log(PPFormatS(PPMSG_ERROR, PPERR_CRESTCORRUPT_ABS, &log_msg, p.LocID, goodsID, rest)));
					if(correct) {
						THROW(PPStartTransaction(&ta, 1));
						CRest.data.GoodsID = p.GoodsID;
						CRest.data.LocID   = p.LocID;
						CRest.data.Rest    = rest;
						THROW_DB(CRest.insertRec());
						THROW(PPCommitWork(&ta));
						ok = 2;
					}
					else
						ok = 1;
				}
			}
			else {
				CALLEXCEPT_PP(PPERR_DBENGINE);
			}
		}
	}
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

int Transfer::CorrectCurRest(const char * pLogName, int correct)
{
	int    ok = 1;
	SString log_msg, fmt_buf;
	Goods2Tbl::Rec goods_rec;
	PPObjLocation loc_obj;
	PPIDArray wh_list; // Список идентификаторов складов
	loc_obj.GetWarehouseList(&wh_list, 0);
	PPLogger logger;
	PPWaitStart();
	for(GoodsIterator goods_iter((PPID)0, 0); goods_iter.Next(&goods_rec) > 0;) {
		THROW(CorrectCurRest(goods_rec.ID, &wh_list, &logger, correct));
		PPWaitPercent(goods_iter.GetIterCounter());
	}
	CATCHZOK
	PPWaitStop();
	logger.Save(pLogName, 0);
	return ok;
}

int CorrectCurRest()
{
	int    ok = -1;
	SString log_fname;
	uint   correct = 1;
	TDialog * dlg = new TDialog(DLG_CORCREST);
	THROW(CheckDialogPtr(&dlg));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORCREST_LOG, CTL_CORCREST_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	PPGetFileName(PPFILNAM_CURRESTERR_LOG, log_fname);
	dlg->setCtrlString(CTL_CORCREST_LOG, log_fname);
	dlg->setCtrlData(CTL_CORCREST_FLAGS, &correct);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_CORCREST_LOG, log_fname);
		dlg->getCtrlData(CTL_CORCREST_FLAGS, &correct);
		correct = BIN(correct);
		THROW(BillObj->trfr->CorrectCurRest(log_fname.Strip(), correct));
		ok = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int CorrectLotsCloseTags()
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	int    err;
	PPID   id = 0;
	IterCounter cntr;
	TransferTbl::Key2 k;
	ReceiptTbl::Rec * rec = & p_bobj->trfr->Rcpt.data;
	PPWaitStart();
	PPInitIterCounter(cntr, &p_bobj->trfr->Rcpt);
	{
		PPTransaction tra(1);
		THROW(tra);
		while(p_bobj->trfr->Rcpt.search(0, &id, spGt)) {
			err = 0;
			rec->Rest = R6(rec->Rest);
			if(rec->Closed) {
				if(rec->Rest != 0.0) {
					if(!(rec->Flags & LOTF_CLOSEDORDER)) {
						rec->Closed = 0;
						rec->CloseDate = MAXDATE;
						err = 1;
					}
				}
				else {
					MEMSZERO(k);
					k.LotID = id + 1;
					if(p_bobj->trfr->search(2, &k, spLt) && k.LotID == id)
						if(k.Dt != rec->CloseDate) {
							rec->CloseDate = k.Dt;
							err = 1;
						}
				}
			}
			else if(rec->Rest == 0.0) {
				rec->Closed = 1;
				MEMSZERO(k);
				k.LotID = id + 1;
				if(p_bobj->trfr->search(2, &k, spLt) && k.LotID == id) {
					rec->CloseDate = k.Dt;
					err = 1;
				}
			}
			else if(rec->CloseDate != MAXLONG) {
				rec->CloseDate = MAXDATE;
   				err = 1;
			}
			if(err)
				THROW_DB(p_bobj->trfr->Rcpt.updateRec());
			PPWaitPercent(cntr.Increment());
		}
		THROW(tra.Commit());
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

static int test_taxgrp(const ReceiptTbl::Rec * pRec, void * extraPtr)
{
	const  PPID parent_tax_grp_id = reinterpret_cast<PPID>(extraPtr);
	return BIN(pRec->InTaxGrpID != parent_tax_grp_id);
}

int Transfer::CorrectLotTaxGrp()
{
	int    ok = 1, ta = 0, frrl_tag = 0;
	PPObjBill * p_bobj = BillObj;
	ReceiptTbl::Key4 k;
	long   total_count = 0, _count = 0;
	PPIDArray bill_list_to_recalc;
	PPWaitStart();
	BExtQuery q(&Rcpt, 4);
	q.select(Rcpt.ID, Rcpt.InTaxGrpID, 0L).where(Rcpt.PrevLotID == 0L);
	total_count = q.countIterations(0, MEMSZERO(k), spFirst);
	THROW(PPStartTransaction(&ta, 1));
	THROW(p_bobj->atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
	MEMSZERO(k);
	for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
		PPID   parent_id  = Rcpt.data.ID;
		PPID   tax_grp_id = Rcpt.data.InTaxGrpID;
		PPID * p_lot_id;
		PPIDArray childs;
		Rcpt.GatherChildren(parent_id, &childs, test_taxgrp, reinterpret_cast<void *>(tax_grp_id));
		for(uint i = 0; childs.enumItems(&i, (void **)&p_lot_id);) {
			LDATE  dt = ZERODATE;
			long   oprno = 0;
			THROW_DB(updateFor(&Rcpt, 0, (Rcpt.ID == *p_lot_id), set(Rcpt.InTaxGrpID, dbconst(tax_grp_id))));
			while(EnumByLot(*p_lot_id, &dt, &oprno) > 0)
				bill_list_to_recalc.addUnique(data.BillID);
		}
		PPWaitPercent(++_count, total_count++);
	}
	for(uint i = 0; i < bill_list_to_recalc.getCount(); i++) {
		THROW(p_bobj->RecalcTurns(bill_list_to_recalc.get(i), 0, 0));
	}
	THROW(p_bobj->atobj->P_Tbl->LockingFRR(0, &frrl_tag, 0));
	THROW(PPCommitWork(&ta));
	PPWaitStop();
	CATCH
		p_bobj->atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int CorrectLotSuppl()
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPOprKind op_rec;
	BillTbl::Rec bill_rec;
	SString msg_buf, log_fname;
	SString out_buf, bill_code;
	PPGetFileName(PPFILNAM_LOTSUPPL_ERR, log_fname);
	PPLogger logger;
	PPWaitStart();
	for(PPID op = 0; EnumOperations(PPOPT_GOODSRECEIPT, &op, &op_rec) > 0;) {
		for(DateIter diter; p_bobj->P_Tbl->EnumByOpr(op, &diter, &bill_rec) > 0;) {
			THROW(PPCheckUserBreak());
			PPWaitMsg(PPObjBill::MakeCodeString(&bill_rec, 1, msg_buf));
			int    err = 0;
			PPBillPacket pack;
			PPTransferItem * pti;
			uint i = 0;
			if(!p_bobj->ExtractPacket(bill_rec.ID, &pack))
				logger.LogLastError();
			else {
				while(!err && pack.EnumTItems(&i, &pti))
					if(pti->Suppl != pack.Rec.Object && !(pti->Flags & PPTFR_FORCESUPPL))
						err = 1;
				if(err) {
					GetArticleName(pack.Rec.Object, out_buf);
					out_buf.CatDivIfNotEmpty('-', 1).Cat(PPObjBill::MakeCodeString(&pack.Rec, 0, bill_code));
					logger.Log(out_buf);
					if(!p_bobj->UpdatePacket(&pack, 1))
						logger.LogLastError();
				}
			}
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	logger.Save(log_fname, 0);
	return ok;
}

int CorrectZeroQCertRefs()
{
	int    ok = 1, r;
	char   msg[64];
	PPObjQCert qcobj;
	ReceiptCore & rcpt = BillObj->trfr->Rcpt;
	PPID   lot_id = 0;
	RECORDNUMBER errcount = 0;
	IterCounter cntr;
	THROW(PPInitIterCounter(cntr, &rcpt));
	PPWaitStart();
	{
		PPTransaction tra(1);
		THROW(tra);
		while(rcpt.search(0, &lot_id, spGt)) {
			r = 1;
			if(rcpt.data.QCertID && (r = qcobj.Search(rcpt.data.QCertID)) < 0) {
				errcount++;
				rcpt.data.QCertID = 0;
				THROW_DB(rcpt.updateRec());
			}
			THROW(r);
			PPWaitPercent(cntr.Increment(), ultoa(errcount, msg, 10));
		}
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}
//
//
//
PPLotFaultArray::PPLotFaultArray(PPID lotID, PPLogger & rLogger) : SVector(sizeof(PPLotFault)), LotID(lotID), P_Logger(&rLogger)
{
}

PPLotFault & FASTCALL PPLotFaultArray::at(uint p) const
{
	return *static_cast<PPLotFault *>(SVector::at(p));
}

int PPLotFaultArray::AddFault(int fault, const ReceiptTbl::Rec * pRec, PPID childID, PPID parentID)
{
	PPLotFault f;
	MEMSZERO(f);
	f.Fault = fault;
	if(pRec)
		f.Dt = pRec->Dt;
	f.ChildLotID = childID;
	f.ParentLotID = parentID;
	return insert(&f) ? 1 : PPSetErrorSLib();
}

int PPLotFaultArray::AddFault(int fault, const TransferTbl::Rec * pRec, double act, double valid)
{
	PPLotFault f;
	MEMSZERO(f);
	f.Fault = fault;
	if(pRec) {
		f.Dt    = pRec->Dt;
		f.OprNo = pRec->OprNo;
	}
	f.ActualVal = act;
	f.ValidVal  = valid;
	return insert(&f) ? 1 : PPSetErrorSLib();
}

int PPLotFaultArray::_HasOpFault(int fault, LDATE dt, long oprno, uint * p) const
{
	for(uint i = 0; i < getCount(); i++) {
		const PPLotFault & lf = at(i);
		if(lf.Fault == fault && lf.Dt == dt && lf.OprNo == oprno) {
			ASSIGN_PTR(p, i);
			return 1;
		}
	}
	return 0;
}

int PPLotFaultArray::HasFault(int faultId, PPLotFault * pFault, uint * pPos) const
{
	for(uint i = 0; i < getCount(); i++) {
		const PPLotFault & lf = at(i);
		if(lf.Fault == faultId) {
			ASSIGN_PTR(pFault, lf);
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	return 0;
}

int PPLotFaultArray::HasLcrFault() const
{
	for(uint i = 0; i < getCount(); i++) {
		const PPLotFault & lf = at(i);
		if(oneof3(lf.Fault, PPLotFault::LcrInvRest, PPLotFault::LcrAbsence, PPLotFault::LcrWaste))
			return 1;
	}
	return 0;
}

int PPLotFaultArray::HasCostOpFault(LDATE dt, long oprno, uint * p) const
{
	return _HasOpFault(PPLotFault::OpCost, dt, oprno, p);
}

int PPLotFaultArray::HasPriceOpFault(LDATE dt, long oprno, uint * p) const
{
	return _HasOpFault(PPLotFault::OpPrice, dt, oprno, p);
}

int PPLotFaultArray::AddMessage()
{
	if(P_Logger) {
		SString msg;
		for(uint i = 0; i < getCount(); i++)
			P_Logger->Log(Message(i, msg));
	}
	return 1;
}

SString & PPLotFaultArray::Message(uint p, SString & rBuf)
{
	rBuf.Z();
	if(p < getCount()) {
		int    msg_id = PPERR_ELOT_UNKNOWN;
		PPLotFault & f = at(p);
		switch(f.Fault) {
			case PPLotFault::NoOps:       msg_id = PPERR_ELOT_NOOPS;    break;
			case PPLotFault::FirstBillID: msg_id = PPERR_ELOT_FBILLID;  break;
			case PPLotFault::FirstDt:     msg_id = PPERR_ELOT_FDATE;    break;
			case PPLotFault::FirstCost:   msg_id = PPERR_ELOT_FCOST;    break;
			case PPLotFault::FirstPrice:  msg_id = PPERR_ELOT_FPRICE;   break;
			case PPLotFault::RevalOldCost:  msg_id = PPERR_ELOT_REVALOLDCOST;  break;
			case PPLotFault::RevalOldPrice: msg_id = PPERR_ELOT_REVALOLDPRICE; break;
			case PPLotFault::Quantity:    msg_id = PPERR_ELOT_QTTY;     break;
			case PPLotFault::OpGoodsID:   msg_id = PPERR_ELOT_OPGOODS;  break;
			case PPLotFault::OpLocation:  msg_id = PPERR_ELOT_OPLOC;    break;
			case PPLotFault::OpRest:      msg_id = PPERR_ELOT_OPREST;   break;
			case PPLotFault::Rest:        msg_id = PPERR_ELOT_REST;     break;
			case PPLotFault::CloseTag:    msg_id = PPERR_ELOT_CLOSETAG; break;
			case PPLotFault::CloseDate:   msg_id = PPERR_ELOT_CLOSEDT;  break;
			case PPLotFault::OpCost:      msg_id = PPERR_ELOT_OPCOST;   break;
			case PPLotFault::OpPrice:     msg_id = PPERR_ELOT_OPPRICE;  break;
			case PPLotFault::RefGoods:    msg_id = PPERR_ELOT_REFGOODS; break;
			case PPLotFault::RefGoodsZero: msg_id = PPERR_ELOT_REFGOODSZERO; break;
			case PPLotFault::RefPrevEqID: msg_id = PPERR_ELOT_PREVEQID; break;
			case PPLotFault::OrdReserveFlag: msg_id = PPERR_ELOT_ORDRESERVEFLAG; break;
			case PPLotFault::OpFlagsCWoVat:  msg_id = PPERR_ELOT_FLAGSCWOVAT; break;
			case PPLotFault::CyclicLink:  msg_id = PPERR_ELOT_CYCLICLINK; break;
			case PPLotFault::WtRest:      msg_id = PPERR_ELOT_WTREST;   break;
			case PPLotFault::OpWtRest:    msg_id = PPERR_ELOT_OPWTREST; break;
			case PPLotFault::NoPack:      msg_id = PPERR_ELOT_NOPACK; break;
			case PPLotFault::PackDifferentGSE: msg_id = PPERR_ELOT_PACKDIFFGSE; break;
			case PPLotFault::PrevLotGoods: msg_id = PPERR_ELOT_PREVLOTGOODS; break;
			case PPLotFault::PrevLotLoc:   msg_id = PPERR_ELOT_PREVLOTLOC;   break;
			case PPLotFault::PrevLotFlagsCWoVat: msg_id = PPERR_ELOT_PREVLOTFLAGSCWOVAT;  break;
			case PPLotFault::LcrInvRest: msg_id = PPERR_ELOT_LCRINVREST; break; // Неверный остаток в записи таблицы остатков по лотам
			case PPLotFault::LcrAbsence: msg_id = PPERR_ELOT_LCRABSENCE; break; // Отсутствует запись в таблице остатков по лотам
			case PPLotFault::LcrWaste:   msg_id = PPERR_ELOT_LCRWASTE;   break; // Лишняя запись в таблице остатков по лотам
			case PPLotFault::LcrDb:      msg_id = PPERR_ELOT_LCRDB;      break; // Ошибка извлечения записи из таблицы остатков по лотам
			case PPLotFault::NonUniqSerial: msg_id = PPERR_ELOT_NONUNIQSERIAL; break;
			case PPLotFault::OrdOpOnSimpleLot: msg_id = PPERR_ELOT_ORDOPONSIMPLELOT; break;
			case PPLotFault::NegativeRest: msg_id = PPERR_ELOT_NEGATIVEREST; break;
			case PPLotFault::InadqIndepPhFlagOn: msg_id = PPERR_ELOT_INADQINDEPPHFLAGON; break;
			case PPLotFault::InadqIndepPhFlagOff: msg_id = PPERR_ELOT_INADQINDEPPHFLAGON; break;
			case PPLotFault::NonSingleRcptOp: msg_id = PPERR_ELOT_NONSINGLERCPTOP; break;
			case PPLotFault::InadqLotWoTaxFlagOn: msg_id = PPERR_ELOT_INADQLOTWOTAXFLAGON; break;
			case PPLotFault::InadqTrfrWoTaxFlagOn: msg_id = PPERR_ELOT_INADQTRFRWOTAXFLAGON; break;
			case PPLotFault::EgaisCodeAlone:       msg_id = PPERR_ELOT_EGAISCODEALONE; break;
			case PPLotFault::NoEgaisCode:          msg_id = PPERR_ELOT_NOEGAISCODE; break;
			case PPLotFault::NoEgaisCodeAmbig:     msg_id = PPERR_ELOT_NOEGAISCODEAMBIG; break;
			case PPLotFault::ThereIsQCert:         msg_id = PPERR_ELOT_THEREISQCERT; break; // @v10.4.10
			default: msg_id = PPERR_ELOT_UNKNOWN; break;
		}
		SString b, lot_str, temp_buf;
		if(PPLoadString(PPMSG_ERROR, msg_id, b)) {
			char   dtbuf[32];
			ReceiptTbl::Rec lot_rec;
			lot_str.Cat(LotID).Space();
			if(BillObj->trfr->Rcpt.Search(LotID, &lot_rec) > 0) {
				lot_str.CatChar('[').Cat(lot_rec.Dt).CatDiv('-', 1);
				GetGoodsName(labs(lot_rec.GoodsID), temp_buf);
				lot_str.Cat(temp_buf);
				lot_str.CatChar(']');
			}
			datefmt(&f.Dt, DATF_DMY, dtbuf);
			switch(f.Fault) {
				case PPLotFault::OpCost:
				case PPLotFault::OpPrice:
				case PPLotFault::OpRest:
				case PPLotFault::OpWtRest:
					rBuf.Printf(b, lot_str.cptr(), dtbuf, f.OprNo, f.ActualVal, f.ValidVal);
					break;
				case PPLotFault::Quantity:
				case PPLotFault::Rest:
				case PPLotFault::WtRest:
				case PPLotFault::FirstCost:
				case PPLotFault::FirstPrice:
				case PPLotFault::RevalOldCost:
				case PPLotFault::RevalOldPrice:
					rBuf.Printf(b, lot_str.cptr(), f.ActualVal, f.ValidVal);
					break;
				case PPLotFault::OpGoodsID:
				case PPLotFault::OpLocation:
				case PPLotFault::InadqIndepPhFlagOn:
				case PPLotFault::InadqIndepPhFlagOff:
				case PPLotFault::NonSingleRcptOp: // @v8.5.7
					rBuf.Printf(b, lot_str.cptr(), dtbuf, f.OprNo);
					break;
				case PPLotFault::CyclicLink:
					rBuf.Printf(b, lot_str.cptr(), f.ChildLotID, f.ParentLotID);
					break;
				case PPLotFault::LinkNotFound:
					rBuf.Printf(b, lot_str.cptr(), f.ChildLotID, f.ParentLotID);
					break;
				case PPLotFault::LcrInvRest:
					lot_str.Space().CatChar('[').Cat(f.Dt).CatChar(']');
					rBuf.Printf(b, lot_str.cptr(), f.ActualVal, f.ValidVal);
					break;
				case PPLotFault::LcrAbsence:
					lot_str.Space().CatChar('[').Cat(f.Dt);
					if(f.EndDate)
						lot_str.Dot().Dot().Cat(f.EndDate);
					lot_str.CatChar(']');
					rBuf.Printf(b, lot_str.cptr(), f.ActualVal, f.ValidVal);
					break;
				case PPLotFault::LcrWaste:
					lot_str.Space().CatChar('[').Cat(f.Dt);
					if(f.EndDate)
						lot_str.Dot().Dot().Cat(f.EndDate);
					lot_str.CatChar(']');
					rBuf.Printf(b, lot_str.cptr(), f.ActualVal, f.ValidVal);
					break;
				case PPLotFault::RefGoods:
				case PPLotFault::RefGoodsZero:
				case PPLotFault::LcrDb:
					rBuf.Printf(b, lot_str.cptr());
					break;
				case PPLotFault::NonUniqSerial:
					BillObj->GetSerialNumberByLot(LotID, temp_buf, 1);
					lot_str.Space().CatBrackStr(temp_buf);
					rBuf.Printf(b, lot_str.cptr());
					break;
				case PPLotFault::OrdOpOnSimpleLot:
					temp_buf.Z();
					rBuf.Printf(b, lot_str.cptr(), temp_buf.cptr());
					break;
				case PPLotFault::InadqLotWoTaxFlagOn:
					rBuf.Printf(b, lot_str.cptr());
					break;
				case PPLotFault::InadqTrfrWoTaxFlagOn:
					rBuf.Printf(b, lot_str.cptr(), dtbuf, f.OprNo);
					break;
				default:
					rBuf.Printf(b, lot_str.cptr());
					break;
			}
		}
	}
	return  rBuf;
}

int Transfer::ProcessLotFault(PPLotFaultArray & rList, int fault, double act, double valid)
{
	return rList.AddFault(fault, &data, act, valid);
}

class RevalArray : public SVector {
public:
	struct Reval { // @flat
		LDATE  Dt;
		long   OprNo;
		double OldCost;
		double OldPrice;
		double NewCost;
		double NewPrice;
	};
	RevalArray(double cost, double price) : SVector(sizeof(Reval)), LotCost(cost), LotPrice(price)
	{
	}
	int    Add(const TransferTbl::Rec * pRec, int first);
	int    Shift();
	int    GetPrices(LDATE dt, long oprno, double * pCost, double * pPrice, long * pRevalIdx);
	//
	double LotCost;
	double LotPrice;
};

int RevalArray::Add(const TransferTbl::Rec * rec, int first)
{
	Reval reval;
	if(first) {
		reval.OldCost  = reval.OldPrice = 0.0;
		reval.NewCost  = TR5(rec->Cost);
		reval.NewPrice = TR5(rec->Price);
	}
	else if(rec->Flags & PPTFR_REVAL) {
		reval.NewCost  = reval.NewPrice = 0.0;
		reval.OldCost  = TR5(rec->Cost);
		reval.OldPrice = TR5(rec->Price);
	}
	else
		return -1;
	reval.Dt = rec->Dt;
	reval.OprNo  = rec->OprNo;
	return insert(&reval) ? 1 : PPSetErrorSLib();
}

int RevalArray::Shift()
{
	if(getCount()) {
		double cost  = LotCost;
		double price = LotPrice;
		for(uint i = getCount() - 1; i > 0; i--) {
			Reval * p_rvl = static_cast<Reval *>(at(i));
			p_rvl->NewCost  = cost;
			p_rvl->NewPrice = price;
			cost  = p_rvl->OldCost;
			price = p_rvl->OldPrice;
		}
	}
	Reval reval;
	reval.Dt    = MAXDATE;
	reval.OprNo = MAXLONG;
	reval.NewCost  = reval.OldCost  = LotCost;
	reval.NewPrice = reval.OldPrice = LotPrice;
	return insert(&reval) ? 1 : PPSetErrorSLib();
}

int RevalArray::GetPrices(LDATE dt, long oprno, double * pCost, double * pPrice, long * pRevalIdx)
{
	for(uint i = 0; i < getCount(); i++) {
		const Reval * p_rvl = static_cast<const Reval *>(at(i));
		if(p_rvl->Dt > dt || (p_rvl->Dt == dt && p_rvl->OprNo > oprno)) {
			*pCost  = p_rvl->OldCost;
			*pPrice = p_rvl->OldPrice;
			ASSIGN_PTR(pRevalIdx, (long)i);
			return 1;
		}
	}
	*pCost  = LotCost;
	*pPrice = LotPrice;
	ASSIGN_PTR(pRevalIdx, -1);
	return 1;
}

int Transfer::CheckLot(PPID lotID, const ReceiptTbl::Rec * pRec, long flags, PPLotFaultArray & rResultList)
{
	int    ok = 1;
	LcrBlock * p_lcr = 0;
	LcrBlock2 * p_lcr2 = 0;
	if(lotID) {
		PPObjBill * p_bobj = BillObj;
		Reference * p_ref = PPRef;
		const PPCommConfig & r_ccfg = CConfig;
		ReceiptTbl::Rec rec;
		Goods2Tbl::Rec goods_rec; // Запись товара, к которому привязан лот
		SString serial_buf;
		SString egais_code;
		PPObjGoods goods_obj;
		if(!RVALUEPTR(rec, pRec)) {
			THROW(Rcpt.Search(lotID, &rec) > 0);
		}
		if(rec.GoodsID) {
			const  PPID goods_id = labs(rec.GoodsID);
            if(goods_obj.Search(goods_id, &goods_rec) > 0) {
				if(flags & TLRF_SETALCCODETOGOODS) {
					if(p_ref->Ot.GetTagStr(PPOBJ_LOT, rec.ID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code) > 0) {
						if(goods_obj.P_Tbl->SearchByBarcode(egais_code, 0, 0) > 0) {
							;
						}
						else {
                            rResultList.AddFault(PPLotFault::EgaisCodeAlone, &rec, 0, 0);
						}
					}
				}
				if(flags & TLRF_SETALCCODETOLOTS) {
					if(p_ref->Ot.GetTagStr(PPOBJ_LOT, rec.ID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code) <= 0) {
						BarcodeArray bc_list;
						goods_obj.P_Tbl->ReadBarcodes(goods_id, bc_list);
						uint bcc = bc_list.getCount();
						if(bcc) do {
							const BarcodeTbl::Rec & r_bc_rec = bc_list.at(--bcc);
							if(sstrlen(r_bc_rec.Code) != 19)
								bc_list.atFree(bcc);
						} while(bcc);
						if(bc_list.getCount() == 1)
							rResultList.AddFault(PPLotFault::NoEgaisCode, &rec, 0, 0);
						else if(bc_list.getCount() > 1)
							rResultList.AddFault(PPLotFault::NoEgaisCodeAmbig, &rec, 0, 0);
					}
				}
            }
            else
				rResultList.AddFault(PPLotFault::RefGoods, &rec, 0, 0);
		}
		else
			rResultList.AddFault(PPLotFault::RefGoodsZero, &rec, 0, 0);
		if(rec.BillID) { // Если не котировка и не специальный лот
			if(flags & TLRF_REPAIRPACK && rec.UnitPerPack <= 0.0) {
				rResultList.AddFault(PPLotFault::NoPack, &rec, 0, 0);
			}
			else if(flags & TLRF_REPAIRPACKUNCOND) {
				GoodsStockExt gse;
				if(goods_obj.GetStockExt(labs(rec.GoodsID), &gse, 0) > 0 && gse.Package > 0 && !feqeps(gse.Package, rec.UnitPerPack, 1E-7))
					rResultList.AddFault(PPLotFault::PackDifferentGSE, &rec, 0, 0);
			}
			if(flags & TLRF_SETINHQCERT && rec.GoodsID > 0 && !rec.QCertID) {
				PPID last_qcert_id = 0;
				PPID last_qcert_lot_id = 0;
				if(Rcpt.GetLastQCert(rec.GoodsID, rec.Dt, rec.LocID, &last_qcert_id, &last_qcert_lot_id) > 0) {
					rResultList.AddFault(PPLotFault::ThereIsQCert, &rec, 0, last_qcert_lot_id);
				}
			}
			if(rec.PrevLotID) {
				if(rec.ID == rec.PrevLotID)
					ProcessLotFault(rResultList, PPLotFault::RefPrevEqID, 0, 0);
				//
				// Проверка на циклическую ссылку и плохие связи
				//
				PPID   org_lot_id = rec.ID;
				PPIDArray looked;
				ReceiptTbl::Rec lot_rec, prev_lot_rec;
				prev_lot_rec = rec;
				for(PPID id = rec.PrevLotID; id != 0; id = lot_rec.PrevLotID) {
					if(Rcpt.Search(id, &lot_rec) > 0) {
						if(lot_rec.GoodsID != prev_lot_rec.GoodsID) {
							rResultList.AddFault(PPLotFault::PrevLotGoods, &lot_rec, lot_rec.ID, prev_lot_rec.ID);
						}
						if(lot_rec.LocID == prev_lot_rec.LocID) {
							rResultList.AddFault(PPLotFault::PrevLotLoc, &lot_rec, lot_rec.ID, prev_lot_rec.ID);
						}
						if((lot_rec.Flags & LOTF_COSTWOVAT) != (prev_lot_rec.Flags & LOTF_COSTWOVAT)) {
							rResultList.AddFault(PPLotFault::PrevLotFlagsCWoVat, &lot_rec, lot_rec.ID, prev_lot_rec.ID);
						}
						if(looked.lsearch(id)) {
							rResultList.AddFault(PPLotFault::CyclicLink, &rec, org_lot_id, id);
							break;
						}
						else {
							looked.add(id);
							org_lot_id = id;
						}
					}
					else {
						rResultList.AddFault(PPLotFault::LinkNotFound, &rec, org_lot_id, id);
						break;
					}
					prev_lot_rec = lot_rec;
				}
			}
			if(rec.GoodsID < 0) { // Заказ
				BillTbl::Rec bill_rec;
				if(p_bobj->Search(rec.BillID, &bill_rec) > 0) {
					if(CheckOpFlags(bill_rec.OpID, OPKF_ORDRESERVE)) {
						if(!(rec.Flags & LOTF_ORDRESERVE))
							ProcessLotFault(rResultList, PPLotFault::OrdReserveFlag, 0, 0);
					}
					else if(rec.Flags & LOTF_ORDRESERVE)
						ProcessLotFault(rResultList, PPLotFault::OrdReserveFlag, 0, 0);
					if(bill_rec.Flags & BILLF_CLOSEDORDER && !(rec.Flags & LOTF_CLOSEDORDER))
						ProcessLotFault(rResultList, PPLotFault::CloseTag, 0, 0);
				}
			}
			if(rec.Rest < 0.0) {
				ProcessLotFault(rResultList, PPLotFault::NegativeRest, 0, 0);
			}
			if(flags & TLRF_CHECKUNIQSERIAL) {
				p_bobj->GetSerialNumberByLot(rec.ID, serial_buf, 0);
				if(p_bobj->AdjustSerialForUniq(rec.GoodsID, rec.ID, 1, serial_buf) > 0)
					ProcessLotFault(rResultList, PPLotFault::NonUniqSerial, 0, 0);
			}
			if(rec.Flags & LOTF_PRICEWOTAXES && !(goods_rec.Flags & GF_PRICEWOTAXES)) {
				ProcessLotFault(rResultList, PPLotFault::InadqLotWoTaxFlagOn, 0, 0);
			}
			{
				TransferTbl::Key2 k;
				long   op_count = 0;
				double rest    = 0.0;
				double ph_rest = 0.0;
				LDATE  last_dt = ZERODATE;
				//LcrBlock lcr(LcrBlock::opTest, P_LcrT, 0);
				RevalArray reval_list(R5(rec.Cost), R5(rec.Price));
				DBRowIdArray rcpt_pos_list; // Список операций, имеющих флаг PPTFR_RECEIPT
				BExtQuery q(this, 2);
				if(rec.Dt >= r_ccfg.LcrUsageSince) {
					if(P_Lcr2T) {
						THROW_MEM(p_lcr2 = new LcrBlock2(LcrBlockBase::opTest, P_Lcr2T, 0));
						p_lcr2->InitLot(lotID);
					}
					else if(P_LcrT) {
						THROW_MEM(p_lcr = new LcrBlock(LcrBlockBase::opTest, P_LcrT, 0));
						p_lcr->InitLot(lotID);
					}
				}
				q.selectAll().where(LotID == lotID);
				MEMSZERO(k);
				k.LotID = lotID;
				for(q.initIteration(false, &k, spGt); q.nextIteration() > 0; op_count++) {
					DBRowId p;
					q.getRecPosition(&p);
					last_dt = data.Dt;
					if(op_count == 0) {
						int    invalid_first_op = 0;
						reval_list.Add(&data, 1);
						if(rec.BillID != data.BillID) {
							ProcessLotFault(rResultList, PPLotFault::FirstBillID, 0, 0);
							invalid_first_op = 1;
						}
						if(rec.Dt != data.Dt) {
							ProcessLotFault(rResultList, PPLotFault::FirstDt, 0, 0);
							invalid_first_op = 1;
						}
						if(!invalid_first_op) {
							if(R6(rec.Quantity - data.Quantity) != 0.0)
								ProcessLotFault(rResultList, PPLotFault::Quantity, rec.Quantity, data.Quantity);
						}
					}
					else if(data.Flags & PPTFR_REVAL)
						reval_list.Add(&data, 0);
					if(labs(rec.GoodsID) != labs(data.GoodsID))
						ProcessLotFault(rResultList, PPLotFault::OpGoodsID, 0, 0);
					else if(data.GoodsID) {
						if((data.Flags & PPTFR_INDEPPHQTTY)  && !(goods_rec.Flags & GF_USEINDEPWT))
							ProcessLotFault(rResultList, PPLotFault::InadqIndepPhFlagOn, 0, 0);
						else if(!(data.Flags & PPTFR_INDEPPHQTTY) && (goods_rec.Flags & GF_USEINDEPWT))
							ProcessLotFault(rResultList, PPLotFault::InadqIndepPhFlagOff, 0, 0);
					}
					if(rec.GoodsID > 0 && rec.LocID != data.LocID)
						ProcessLotFault(rResultList, PPLotFault::OpLocation, 0, 0);
					if(data.Flags & PPTFR_COSTWOVAT && !(rec.Flags & LOTF_COSTWOVAT))
						ProcessLotFault(rResultList, PPLotFault::OpFlagsCWoVat, 1, 0);
					if(!(data.Flags & PPTFR_COSTWOVAT) && rec.Flags & LOTF_COSTWOVAT)
						ProcessLotFault(rResultList, PPLotFault::OpFlagsCWoVat, 0, 1);
					if(data.Flags & PPTFR_PRICEWOTAXES && !(goods_rec.Flags & GF_PRICEWOTAXES))
						ProcessLotFault(rResultList, PPLotFault::InadqTrfrWoTaxFlagOn, 0, 0);
					rest = R6(rest + data.Quantity);
					ph_rest = R6(ph_rest + data.WtQtty);
					if(R6(data.Rest - rest) != 0.0)
						ProcessLotFault(rResultList, PPLotFault::OpRest, data.Rest, rest);
					if(R6(data.WtRest - ph_rest) != 0.0)
						ProcessLotFault(rResultList, PPLotFault::OpWtRest, (double)data.WtRest, ph_rest);
					if(data.Flags & PPTFR_RECEIPT) {
						THROW_SL(rcpt_pos_list.insert(&p));
					}
					CALLPTRMEMB(p_lcr2, Process(data));
					CALLPTRMEMB(p_lcr, Process(data));
				}
				if(p_lcr2) {
					p_lcr2->FinishLot();
					p_lcr2->TranslateErr(&rResultList);
					ZDELETE(p_lcr2);
				}
				else if(p_lcr) {
					p_lcr->FinishLot();
					p_lcr->TranslateErr(&rResultList);
					ZDELETE(p_lcr);
				}
				if(op_count > 0) {
					if(R6(rec.Rest - rest) != 0)
						ProcessLotFault(rResultList, PPLotFault::Rest, rec.Rest, rest);
					if(R6(rec.WtRest - ph_rest) != 0)
						ProcessLotFault(rResultList, PPLotFault::WtRest, (double)rec.WtRest, ph_rest);
					if(rest == 0.0) {
						if(!rec.Closed)
							ProcessLotFault(rResultList, PPLotFault::CloseTag, 0, 0);
						if(rec.GoodsID > 0)
							if(rec.CloseDate != last_dt)
								ProcessLotFault(rResultList, PPLotFault::CloseDate, 0, 0);
					}
					else if(rec.GoodsID > 0) {
						if(rec.Closed)
							ProcessLotFault(rResultList, PPLotFault::CloseTag, 0 , 0);
						if(rec.CloseDate != MAXLONG)
							ProcessLotFault(rResultList, PPLotFault::CloseDate, 0, 0);
					}
				}
				else
					ProcessLotFault(rResultList, PPLotFault::NoOps, 0, 0);
				if(rcpt_pos_list.getCount() > 1) {
					DBRowId last_p = rcpt_pos_list.at(rcpt_pos_list.getCount()-1);
					THROW_DB(getDirect(0, 0, last_p));
					ProcessLotFault(rResultList, PPLotFault::NonSingleRcptOp, 0, 0);
				}
				if(rec.GoodsID > 0) {  // Not Order
					reval_list.Shift();
					k.LotID = lotID;
					k.Dt    = ZERODATE;
					k.OprNo = 0;
					op_count = 0;
					for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
						if(!(data.Flags & PPTFR_REVAL)) {
							if(op_count) {
								if(data.Flags & PPTFR_ORDER) {
									BillTbl::Rec bill_rec;
									if(p_bobj->Search(data.BillID, &bill_rec) > 0 && GetOpType(bill_rec.OpID) == PPOPT_GOODSORDER) {
										ProcessLotFault(rResultList, PPLotFault::OrdOpOnSimpleLot, 0.0, 0.0);
									}
								}
							}
							double cost, price;
							double op_cost  = TR5(data.Cost);
							double op_price = TR5(data.Price);
							long   reval_idx = -1;
							reval_list.GetPrices(data.Dt, data.OprNo, &cost, &price, &reval_idx);
							if(R5(op_cost - cost) != 0.0) {
								if(op_count)
									ProcessLotFault(rResultList, PPLotFault::OpCost, op_cost, cost);
								else {
									ProcessLotFault(rResultList, PPLotFault::FirstCost, cost, op_cost);
									if(reval_idx < 0) {
										ProcessLotFault(rResultList, PPLotFault::FirstCost, cost, op_cost);
										reval_list.LotCost = op_cost;
									}
									else {
										RevalArray::Reval * p_rai = static_cast<RevalArray::Reval *>(reval_list.at(reval_idx));
										if(p_rai) {
											//
											// Для правильной инициализации ошибки необходимо извлечь
											// как текущую ошибочную запись переоценки
											//
											TransferTbl::Key1 k1;
											MEMSZERO(k1);
											k1.Dt = p_rai->Dt;
											k1.OprNo = p_rai->OprNo;
											if(search(1, &k1, spEq))
												ProcessLotFault(rResultList, PPLotFault::RevalOldCost, cost, op_cost);
											else
												ProcessLotFault(rResultList, PPLotFault::FirstCost, cost, op_cost);
											p_rai->OldCost = op_cost;
										}
										else
											ProcessLotFault(rResultList, PPLotFault::FirstCost, cost, op_cost);
									}
								}
							}
							if(R5(op_price - price) != 0.0) {
								if(op_count)
									ProcessLotFault(rResultList, PPLotFault::OpPrice, op_price, price);
								else {
									if(reval_idx < 0) {
										ProcessLotFault(rResultList, PPLotFault::FirstPrice, price, op_price);
										reval_list.LotPrice = op_price;
									}
									else {
										RevalArray::Reval * p_rai = static_cast<RevalArray::Reval *>(reval_list.at(reval_idx));
										if(p_rai) {
											//
											// Для правильной инициализации ошибки необходимо извлечь
											// как текущую ошибочную запись переоценки
											//
											TransferTbl::Key1 k1;
											MEMSZERO(k1);
											k1.Dt = p_rai->Dt;
											k1.OprNo = p_rai->OprNo;
											if(search(1, &k1, spEq))
												ProcessLotFault(rResultList, PPLotFault::RevalOldPrice, price, op_price);
											else
												ProcessLotFault(rResultList, PPLotFault::FirstPrice, price, op_price);
											p_rai->OldPrice = op_price;
										}
										else
											ProcessLotFault(rResultList, PPLotFault::FirstPrice, price, op_price);
									}
								}
							}
						}
						op_count++;
					}
				}
			}
			ok = rResultList.getCount() ? -1 : 1;
		}
	}
	CATCHZOK
	delete p_lcr;
	delete p_lcr2;
	return ok;
}

int Transfer::RecoverLot(PPID lotID, PPLotFaultArray * pFaultList, long flags, int use_ta)
{
	int    ok = 1, err_lot = 0, r;
	ReceiptTbl::Rec lot_rec;
	SString msg_buf, serial_buf;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(lotID && pFaultList && pFaultList->getCount() && flags & (TLRF_REPAIR|TLRF_ADJUNUQSERIAL|TLRF_SETALCCODETOGOODS|TLRF_SETALCCODETOLOTS)) {
		PPObjBill * p_bobj = BillObj;
		Reference * p_ref = PPRef;
		PPLotFault fault;
		uint   fault_pos = 0;
		//THROW(PPStartTransaction(&ta, use_ta));
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Rcpt.Search(lotID, &lot_rec) > 0);
		if(flags & TLRF_ADJUNUQSERIAL) {
			if(pFaultList->HasFault(PPLotFault::NonUniqSerial, &fault, &fault_pos)) {
				p_bobj->GetSerialNumberByLot(lotID, serial_buf, 0);
				if(p_bobj->AdjustSerialForUniq(lot_rec.GoodsID, lotID, 0, serial_buf) > 0) {
					THROW(p_bobj->SetSerialNumberByLot(lotID, serial_buf, 0));
				}
			}
		}
		if(flags & TLRF_SETALCCODETOLOTS) {
		}
		if(flags & TLRF_SETALCCODETOGOODS) {
			if(pFaultList->HasFault(PPLotFault::EgaisCodeAlone, &fault, &fault_pos)) {
				SString egais_code;
				if(p_ref->Ot.GetTagStr(PPOBJ_LOT, lotID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code) > 0) {
					if(goods_obj.P_Tbl->SearchByBarcode(egais_code, 0, 0) < 0) {
						THROW(goods_obj.P_Tbl->AddBarcode(labs(lot_rec.GoodsID), egais_code, 1.0, 0));
					}
				}
			}
		}
		if(flags & TLRF_SETALCCODETOLOTS) {
			if(pFaultList->HasFault(PPLotFault::NoEgaisCode, &fault, &fault_pos)) {
				SString ex_egais_code;
				if(p_ref->Ot.GetTagStr(PPOBJ_LOT, lotID, PPTAG_LOT_FSRARLOTGOODSCODE, ex_egais_code) <= 0) {
					const  PPID goods_id = labs(lot_rec.GoodsID);
					BarcodeArray bc_list;
					goods_obj.P_Tbl->ReadBarcodes(goods_id, bc_list);
					uint bcc = bc_list.getCount();
					if(bcc) do {
						const BarcodeTbl::Rec & r_bc_rec = bc_list.at(--bcc);
						if(sstrlen(r_bc_rec.Code) != 19)
							bc_list.atFree(bcc);
					} while(bcc);
					if(bc_list.getCount() == 1) {
						ObjTagItem tag_item;
						if(tag_item.SetStr(PPTAG_LOT_FSRARLOTGOODSCODE, bc_list.at(0).Code)) {
							THROW(p_ref->Ot.PutTag(PPOBJ_LOT, lotID, &tag_item, 0));
						}
					}
				}
			}
		}
		if(flags & TLRF_REPAIR) {
			{
				GoodsStockExt gse;
				int    gse_inited = 0;
				if(flags & TLRF_REPAIRPACK) {
					if(pFaultList->HasFault(PPLotFault::NoPack, &fault, &fault_pos)) {
						LDATE dt = lot_rec.Dt;
						long  oprno = lot_rec.OprNo;
						ReceiptTbl::Rec temp_rec;
						r = 0;
						while(Rcpt.EnumLastLots(lot_rec.GoodsID, 0, &dt, &oprno, &temp_rec) > 0) {
							if(temp_rec.UnitPerPack > 0.0) {
								lot_rec.UnitPerPack = temp_rec.UnitPerPack;
								err_lot = 1;
								r = 1;
								break;
							}
						}
						if(!r) {
							if(goods_obj.GetStockExt(labs(lot_rec.GoodsID), &gse) > 0) {
								gse_inited = 1;
								if(gse.Package > 0) {
									lot_rec.UnitPerPack = gse.Package;
									err_lot = 1;
								}
							}
						}
					}
				}
				if(flags & TLRF_SETINHQCERT) {
					if(pFaultList->HasFault(PPLotFault::ThereIsQCert, &fault, &fault_pos)) {
						if(!lot_rec.QCertID) { // @paranoic
							PPID last_qcert_id = 0;
							PPID last_qcert_lot_id = 0;
							if(Rcpt.GetLastQCert(lot_rec.GoodsID, lot_rec.Dt, lot_rec.LocID, &last_qcert_id, &last_qcert_lot_id) > 0) {
								lot_rec.QCertID = last_qcert_id;
								err_lot = 1;
							}
						}
					}
				}
				if(flags & TLRF_REPAIRPACKUNCOND) {
					if(pFaultList->HasFault(PPLotFault::PackDifferentGSE, &fault, &fault_pos)) {
                        if(gse_inited || goods_obj.GetStockExt(labs(lot_rec.GoodsID), &gse) > 0) {
							if(gse.Package > 0 && lot_rec.UnitPerPack != gse.Package) {
								lot_rec.UnitPerPack = gse.Package;
								err_lot = 1;
							}
                        }
					}
				}
			}
			if(pFaultList->HasFault(PPLotFault::CyclicLink, &fault, &fault_pos)) {
				if(fault.ChildLotID == lot_rec.ID && fault.ParentLotID == lot_rec.PrevLotID) {
					if(pFaultList->Message(fault_pos, msg_buf).NotEmpty()) {
						msg_buf.CatDiv(':', 1).Cat("CORRECTED");
						PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
					}
					lot_rec.PrevLotID = 0;
					err_lot = 1;
				}
			}
			if(lot_rec.PrevLotID && (pFaultList->HasFault(PPLotFault::PrevLotFlagsCWoVat, &fault, &fault_pos))) {
				if(pFaultList->Message(fault_pos, msg_buf).NotEmpty()) {
					msg_buf.CatDiv(':', 1).Cat("CORRECTED");
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
				}
				INVERSEFLAG(lot_rec.Flags, LOTF_COSTWOVAT);
				err_lot = 1;
			}
			if(lot_rec.PrevLotID && (pFaultList->HasFault(PPLotFault::PrevLotGoods, &fault, &fault_pos) ||
				pFaultList->HasFault(PPLotFault::PrevLotLoc, &fault, &fault_pos))) {
				if(pFaultList->Message(fault_pos, msg_buf).NotEmpty()) {
					msg_buf.CatDiv(':', 1).Cat("CORRECTED");
					PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
				}
				lot_rec.PrevLotID = 0;
				err_lot = 1;
			}
			if(pFaultList->HasFault(PPLotFault::LinkNotFound, &fault, &fault_pos)) {
				if(fault.ChildLotID == lot_rec.ID && fault.ParentLotID == lot_rec.PrevLotID) {
					if(pFaultList->Message(fault_pos, msg_buf).NotEmpty()) {
						msg_buf.CatDiv(':', 1).Cat("CORRECTED");
						PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
					}
					lot_rec.PrevLotID = 0;
					err_lot = 1;
				}
			}
			if(flags & TLRF_REPAIRCOST) {
				if(pFaultList->HasFault(PPLotFault::FirstCost, &fault, &fault_pos)) {
                    if(fault.ActualVal >= 0.0 && fault.ValidVal > 0.0) { // @v10.4.2 (fault.ActualVal == 0.0)-->(fault.ActualVal >= 0.0)
						lot_rec.Cost = fault.ValidVal;
						err_lot = 1;
                    }
				}
			}
			if(flags & TLRF_REPAIRPRICE) {
				if(pFaultList->HasFault(PPLotFault::FirstPrice, &fault, &fault_pos)) {
					if(fault.ActualVal >= 0.0 && fault.ValidVal > 0.0) { // @v10.4.2 (fault.ActualVal == 0.0)-->(fault.ActualVal >= 0.0)
						lot_rec.Price = fault.ValidVal;
						err_lot = 1;
                    }
				}
			}
			if(lot_rec.GoodsID < 0) { // Заказ
				BillTbl::Rec bill_rec;
				if(p_bobj->Search(lot_rec.BillID, &bill_rec) > 0) {
					if(CheckOpFlags(bill_rec.OpID, OPKF_ORDRESERVE)) {
						if(!(lot_rec.Flags & LOTF_ORDRESERVE)) {
							lot_rec.Flags |= LOTF_ORDRESERVE;
							err_lot = 1;
						}
					}
					else if(lot_rec.Flags & LOTF_ORDRESERVE) {
						lot_rec.Flags &= ~LOTF_ORDRESERVE;
						err_lot = 1;
					}
					if(bill_rec.Flags & BILLF_CLOSEDORDER) {
						if(!(lot_rec.Flags & LOTF_CLOSEDORDER)) {
							lot_rec.Flags |= LOTF_CLOSEDORDER;
							err_lot = 1;
						}
					}
				}
			}
			// @v8.9.0 {
			if(flags & TLRF_REPAIRWOTAXFLAGS && pFaultList->HasFault(PPLotFault::InadqLotWoTaxFlagOn, &fault, &fault_pos)) {
				lot_rec.Flags &= ~LOTF_PRICEWOTAXES;
				err_lot = 1;
			}
			// } @v8.9.0
			if(lot_rec.BillID) { // Если не котировка
				DBRowIdArray rcpt_pos_list; // Список операций, имеющих флаг PPTFR_RECEIPT
				long   oprno = 0;
				long   count = 0;
				double rest    = 0.0;
				double ph_rest = 0.0;
				LDATE  last_date = ZERODATE, dt = ZERODATE;
				while((r = EnumByLot(lotID, &dt, &oprno)) > 0) {
					DBRowId p;
					THROW_DB(getPosition(&p));
					PPID   bill_id = data.BillID;
					int    err_op = 0, err_price = 0;
					uint   fp;
					last_date = data.Dt;
					if(pFaultList->HasFault(PPLotFault::NonSingleRcptOp, 0, 0)) {
						if(data.Flags & PPTFR_RECEIPT) {
							THROW_SL(rcpt_pos_list.insert(&p));
						}
					}
					if(!count) {
						if(lot_rec.ID && lot_rec.ID == lot_rec.PrevLotID) {
							TransferTbl::Rec sav_data;
							copyBufTo(&sav_data);
							lot_rec.PrevLotID = (SearchMirror(data.Dt, data.OprNo, 0) > 0 && data.Reverse == 0) ? data.LotID : 0;
							copyBufFrom(&sav_data);
							err_lot = 1;
						}
						if(lot_rec.BillID != data.BillID) {
							lot_rec.BillID = data.BillID;
							err_lot = 1;
						}
						if(lot_rec.Dt != data.Dt) {
							lot_rec.Dt = data.Dt;
							IncDateKey(&Rcpt, 1, lot_rec.Dt, &(lot_rec.OprNo = 0));
							err_lot = 1;
						}
						if(R6(lot_rec.Quantity - data.Quantity) != 0) {
							lot_rec.Quantity = data.Quantity;
							err_lot = 1;
						}
					}
					if(pFaultList->_HasOpFault(PPLotFault::OrdOpOnSimpleLot, dt, oprno, &fp)) {
						PPTransferItem ti;
						ti.SetupByRec(&data);
						ti.LotID = 0;
						ti.GoodsID = -labs(ti.GoodsID);
						ti.Flags |= PPTFR_ORDER;
						THROW(AddLotItem(&ti, 0));
						data.LotID = ti.LotID;
						data.GoodsID = -labs(ti.GoodsID);
						data.Flags |= PPTFR_ORDER;
						err_op = 1;
					}
					else { // При такой ошибке не следует пытаться исправить другие ошибки
						// @v8.5.7 {
						if(pFaultList->HasFault(PPLotFault::NonSingleRcptOp, 0, 0) && rcpt_pos_list.getCount() > 1) {
							if(data.Flags & PPTFR_RECEIPT) {
								data.Flags &= ~PPTFR_RECEIPT;
								err_op = 1;
							}
						}
						// } @v8.5.7
						if(labs(lot_rec.GoodsID) != labs(data.GoodsID)) {
							data.GoodsID = lot_rec.GoodsID;
							err_op = 1;
						}
						if(lot_rec.GoodsID > 0 && lot_rec.LocID != data.LocID) {
							data.LocID = lot_rec.LocID;
							err_op = 1;
						}
						rest = R6(rest + data.Quantity);
						if(R6(data.Rest - rest) != 0) {
							data.Rest = rest;
							err_op = 1;
						}
						ph_rest = R6(ph_rest + data.WtQtty);
						if(R6(data.WtRest - ph_rest) != 0) {
							data.WtRest = (float)R6(ph_rest);
							err_op = 1;
						}
						if(flags & TLRF_INDEPHQTTY) {
							if(pFaultList->HasFault(PPLotFault::InadqIndepPhFlagOn, 0, 0)) {
								if(data.Flags & PPTFR_INDEPPHQTTY) {
									data.Flags &= ~PPTFR_INDEPPHQTTY;
									err_op = 1;
								}
							}
							/* установливать флаг не следует - много сложных последствий
							else if(pFaultList->HasFault(PPLotFault::InadqIndepPhFlagOn, 0, 0)) {
							}
							*/
						}
						// @v8.9.0 {
						if(flags & TLRF_REPAIRWOTAXFLAGS && pFaultList->HasFault(PPLotFault::InadqTrfrWoTaxFlagOn, 0, 0)) {
							if(data.Flags & PPTFR_PRICEWOTAXES) {
								if(goods_obj.Search(labs(data.GoodsID), &goods_rec) > 0 && !(goods_rec.Flags & GF_PRICEWOTAXES)) {
									data.Flags &= ~PPTFR_PRICEWOTAXES;
									err_op = 1;
								}
							}
						}
						// } @v8.9.0
						if(lot_rec.GoodsID > 0) {
							if(flags & TLRF_REPAIRCOST) {
								if(pFaultList->HasCostOpFault(dt, oprno, &fp)) {
									data.Cost = TR5(pFaultList->at(fp).ValidVal);
									err_op = 1;
									err_price = 1;
								}
								if(pFaultList->_HasOpFault(PPLotFault::RevalOldCost, dt, oprno, &fp)) {
									assert(data.Flags & PPTFR_REVAL);
									data.Cost = TR5(pFaultList->at(fp).ValidVal);
									err_op = 1;
									err_price = 1;
								}
							}
							if(flags & TLRF_REPAIRPRICE) {
								if(pFaultList->HasPriceOpFault(dt, oprno, &fp)) {
									double p0 = TR5(data.Price);
									double d0 = TR5(data.Discount);
									double p  = TR5(pFaultList->at(fp).ValidVal);
									data.Price = TR5(p);
									data.Discount = TR5(p - p0 + d0);
									err_op    = 1;
									err_price = 1;
								}
								if(pFaultList->_HasOpFault(PPLotFault::RevalOldPrice, dt, oprno, &fp)) {
									assert(data.Flags & PPTFR_REVAL);
									data.Price = TR5(pFaultList->at(fp).ValidVal);
									err_op = 1;
									err_price = 1;
								}
							}
						}
						if(data.Flags & PPTFR_COSTWOVAT && !(lot_rec.Flags & LOTF_COSTWOVAT)) {
							SETFLAG(data.Flags, PPTFR_COSTWOVAT, lot_rec.Flags & LOTF_COSTWOVAT);
							err_op = 1;
						}
						if(!(data.Flags & PPTFR_COSTWOVAT) && lot_rec.Flags & LOTF_COSTWOVAT) {
							SETFLAG(data.Flags, PPTFR_COSTWOVAT, lot_rec.Flags & LOTF_COSTWOVAT);
							err_op = 1;
						}
					}
					if(err_op) {
						THROW_DB(updateRec());
						if(err_price)
							THROW(p_bobj->RecalcTurns(bill_id, 0, 0));
					}
					count++;
				}
				THROW_DB(r);
				if(R6(lot_rec.Rest - rest) != 0) {
					lot_rec.Rest = rest;
					err_lot = 1;
				}
				if(R6(lot_rec.WtRest - ph_rest) != 0) {
					lot_rec.WtRest = static_cast<float>(R6(ph_rest));
					err_lot = 1;
				}
				if(rest == 0.0) {
					if(!lot_rec.Closed) {
						lot_rec.Closed = 1;
						err_lot = 1;
					}
					if(lot_rec.GoodsID > 0 && lot_rec.CloseDate != last_date) {
						lot_rec.CloseDate = last_date;
						err_lot = 1;
					}
				}
				else if(lot_rec.GoodsID > 0) {
					if(lot_rec.Closed) {
						lot_rec.Closed = 0;
						err_lot = 1;
					}
					if(lot_rec.CloseDate != MAXDATE) {
						lot_rec.CloseDate = MAXDATE;
						err_lot = 1;
					}
				}
			}
			// @v9.9.0 Изменен порядок вызова следующих 2 блоков - не срабатывало удаление "пустых" лотов
			if(pFaultList->HasFault(PPLotFault::NoOps, &fault, &fault_pos) && flags & TLRF_RMVLOST) {
				THROW_DB(deleteFrom(&Rcpt, 0, Rcpt.ID == lotID));
			}
			else {
				if(err_lot) {
					THROW(UpdateByID(&Rcpt, PPOBJ_LOT, lotID, &lot_rec, 0));
					err_lot = 0;
				}
				if(pFaultList->HasLcrFault()) {
					if(P_Lcr2T) {
						THROW(Helper_RecalcLotCRest2(lotID, 0, 1));
					}
					else if(P_LcrT) {
						THROW(Helper_RecalcLotCRest(lotID, 0, 1));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int Transfer::RecalcLcr()
{
	int    ok = 1;
	int    inner_tbl = 0;
	const PPCommConfig & r_ccfg = CConfig;
	const LDATE start_date = checkdate(r_ccfg.LcrUsageSince) ? r_ccfg.LcrUsageSince : ZERODATE;
	if(r_ccfg.Flags2 & CCFLG2_USELCR2) {
		LotCurRest2Tbl * p_tbl = P_Lcr2T;
		if(!P_Lcr2T) {
			P_Lcr2T = new LotCurRest2Tbl;
			inner_tbl = 2;
		}
		if(P_Lcr2T) {
			IterCounter cntr;
			ReceiptTbl::Key1 k1;
			MEMSZERO(k1);
			k1.Dt = start_date;
			BExtQuery q(&Rcpt, 1);
			q.select(Rcpt.ID, Rcpt.Dt, 0L).where(Rcpt.Dt >= start_date);
			PPWaitStart();
			PPInitIterCounter(cntr, &Rcpt);
			BExtInsert bei(P_Lcr2T);
			{
				PPTransaction tra(1);
				THROW(tra);
				for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
					THROW(Helper_RecalcLotCRest2(Rcpt.data.ID, &bei, 0));
					PPWaitPercent(cntr.Increment());
					if((static_cast<uint>(cntr) % 10000) == 0) {
						THROW_DB(bei.flash());
						THROW(tra.Commit());
						THROW(tra.Start(1));
					}
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
		}
	}
	else {
		LotCurRestTbl * p_tbl = P_LcrT;
		if(!P_LcrT) {
			P_LcrT = new LotCurRestTbl;
			inner_tbl = 1;
		}
		if(P_LcrT) {
			IterCounter cntr;
			ReceiptTbl::Key1 k1;
			MEMSZERO(k1);
			k1.Dt = start_date;
			BExtQuery q(&Rcpt, 1);
			q.select(Rcpt.ID, Rcpt.Dt, 0L).where(Rcpt.Dt >= start_date);
			PPWaitStart();
			PPInitIterCounter(cntr, &Rcpt);
			BExtInsert bei(P_LcrT);
			{
				PPTransaction tra(1);
				THROW(tra);
				for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
					THROW(Helper_RecalcLotCRest(Rcpt.data.ID, &bei, 0));
					PPWaitPercent(cntr.Increment());
					if((static_cast<uint>(cntr) % 10000) == 0) {
						THROW_DB(bei.flash());
						THROW(tra.Commit());
						THROW(tra.Start(1));
					}
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	if(inner_tbl == 2) {
		ZDELETE(P_Lcr2T);
	}
	else if(inner_tbl == 1) {
		ZDELETE(P_LcrT);
	}
	PPWaitStop();
	return ok;
}

int PPObjBill::CorrectPckgCloseTag()
{
	int    ok = 1;
	if(CcFlags & CCFLG_USEGOODSPCKG) {
		IterCounter cntr;
		PPID   k = 0;
		PackageTbl::Rec pckg_rec;
		PPWaitStart();
		PPInitIterCounter(cntr, P_PckgT);
		{
			PPTransaction tra(1);
			THROW(tra);
			while(P_PckgT->search(0, &k, spGt)) {
				ReceiptTbl::Rec lot_rec;
				P_PckgT->copyBufTo(&pckg_rec);
				if(trfr->Rcpt.Search(pckg_rec.ID, &lot_rec) > 0) {
					int16 old_val = BIN(pckg_rec.Closed);
					pckg_rec.Closed = (lot_rec.Rest > 0) ? 0 : 1;
					P_PckgT->copyBufFrom(&pckg_rec);
					if(old_val != pckg_rec.Closed)
						if(!pckg_rec.Closed && !P_PckgT->AdjustUniqCntr(&pckg_rec)) {
							PPError();
							PPWaitStart();
						}
						else
							THROW(UpdateByID(P_PckgT, PPOBJ_PACKAGE, pckg_rec.ID, &pckg_rec, 0));
				}
				PPWaitPercent(cntr.Increment());
			}
			THROW(tra.Commit());
		}
		PPWaitStop();
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

#if 0 // {

int CorrectIntrReverse(PPID billID)
{
	int ok = 1, ta = 0, r;
	PPBillPacket pack;
	Transfer * trfr = BillObj->trfr;
	THROW(BillObj->ExtractPacket(billID, &pack) > 0);
	THROW(PPStartTransaction(&ta, 1));
	if(IsIntrExpndOp(pack.Rec.OpID)) {
		PPTransferItem * ti;
		for(uint i = 0; pack.EnumTItems(&i, &ti);) {
			PPWaitPercent(i, pack.GetTCount(), pack.Rec.Code);
			PPID   saveLot, tmp;
			double saveDiscount;
			THROW(r = trfr->_SearchByBill(billID, 1, ti->RByBill, 0));
			if(r < 0 && !(ti->Flags & PPTFR_ONORDER)) {
				THROW_PP(!(ti->Flags & PPTFR_UNLIM), PPERR_UNLIMINTROP);
				saveLot      = ti->LotID;
				tmp  = ti->Location;
				saveDiscount = ti->Discount;
				ti->Price   -= ti->Discount;
				ti->Discount = 0.0;
				ti->Location = ti->CorrLoc;
				ti->CorrLoc  = tmp;
				ti->Quantity = -ti->Quantity;
				ti->Flags   &= ~PPTFR_UNITEINTR;
				INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
				//
				// Откладываем обработку ошибки в AddItem до восстановления значений ti
				//
				r = trfr->AddItem(ti, 0);
				INVERSEFLAG(ti->Flags, PPTFR_RECEIPT);
				ti->Flags   |= PPTFR_UNITEINTR;
				ti->Quantity = -ti->Quantity;
				ti->Discount = saveDiscount;
				ti->Price   += saveDiscount;
				tmp  = ti->Location;
				ti->Location = ti->CorrLoc;
				ti->CorrLoc  = tmp;
				ti->LotID    = saveLot;
				THROW(r);
			}
		}
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

#endif // 0 }

#if 0 // @v5.1.9 {
int ShrinkLots()
{
	int    ok = 1, ta = 0;
	Goods2Tbl::Rec goods_rec;
	ReceiptCore * p_rc = &BillObj->trfr->Rcpt;
	ReceiptTbl::Key2 k2;
	GoodsIterator iter;

	THROW(PPStartTransaction(&ta, 1));
	MEMSZERO(k2);
	k2.GoodsID = -MAXLONG;
	while(p_rc->search(2, &k2, spGt) && k2.GoodsID <= 0)
		THROW_DB(p_rc->deleteRec());
	while(iter.Next(&goods_rec) > 0) {
		PPID   lot_id = 0;
		if(p_rc->GetLastLot(goods_rec.ID, 0, MAXDATE, 0) > 0) {
			lot_id = p_rc->data.ID;
			p_rc->data.Quantity  = 0;
			p_rc->data.Rest      = 0;
			p_rc->data.Closed    = 1;
			p_rc->data.BillID    = 0;
			p_rc->data.PrevLotID = 0;
			THROW_DB(p_rc->updateRec());
		}
		MEMSZERO(k2);
		k2.GoodsID = goods_rec.ID;
		for(int sp = spGe; p_rc->search(2, &k2, sp) && k2.GoodsID == goods_rec.ID; sp = spGt)
			if(p_rc->data.ID != lot_id)
				THROW_DB(p_rc->deleteRec());
	}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
#endif // } @v5.1.9

class PrcssrAbsentGoods {
public:
	struct Param {
		enum {
			fCorrectErrors = 0x0001
		};
		PPID   DefUnitID;
		PPID   DefGroupID;
		long   Flags;
		SString LogFileName;
	};
	PrcssrAbsentGoods()
	{
		P_BObj = BillObj;
	}
	int    InitParam(Param * pParam)
	{
		memzero(pParam, sizeof(Param));
		PPGetFileName(PPFILNAM_ABSGOODS_LOG, pParam->LogFileName);
		pParam->DefUnitID = GObj.GetConfig().DefUnitID;
		return 1;
	}
	int    Init(const Param * pParam)
	{
		P = *pParam;
		AbsentCount = 0;
		return 1;
	}
	int    EditParam(Param *);
	int    Run();
private:
	int    ProcessGoods(PPID goodsID, PPID lotID, PPID billID, PPLogger &);

	Param  P;
	long   AbsentCount;
	PPObjGoods GObj;
	PPObjGoodsGroup GgObj;
	PPObjBill * P_BObj;
};

int PrcssrAbsentGoods::EditParam(Param * pParam)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_CABSGOODS)))) {
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CABSGOODS_LOG, CTL_CABSGOODS_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		dlg->setCtrlString(CTL_CABSGOODS_LOG, pParam->LogFileName);
		dlg->setCtrlUInt16(CTL_CABSGOODS_FLAGS, BIN(pParam->Flags & Param::fCorrectErrors));
		SetupPPObjCombo(dlg, CTLSEL_CABSGOODS_GGRP, PPOBJ_GOODSGROUP, pParam->DefGroupID, 0, 0);
		SetupPPObjCombo(dlg, CTLSEL_CABSGOODS_UNIT, PPOBJ_UNIT, pParam->DefUnitID, 0, 0);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_CABSGOODS_LOG, pParam->LogFileName);
			SETFLAG(pParam->Flags, Param::fCorrectErrors, dlg->getCtrlUInt16(CTL_CABSGOODS_FLAGS) & 0x01);
			dlg->getCtrlData(CTLSEL_CABSGOODS_GGRP, &pParam->DefGroupID);
			dlg->getCtrlData(CTLSEL_CABSGOODS_UNIT, &pParam->DefUnitID);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PrcssrAbsentGoods::ProcessGoods(PPID goodsID, PPID lotID, PPID billID, PPLogger & rLogger)
{
	int    ok = 1, r;
	THROW(r = GObj.Search(goodsID, 0));
	if(r < 0) {
		AbsentCount++;
		ReceiptTbl::Rec lot_rec;
		BillTbl::Rec bill_rec;
		SString fmt_buf, bill_buf, log_buf;
		PPLoadString(PPMSG_ERROR, PPERR_ABSGOODS, fmt_buf);
		if(lotID && P_BObj->trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
			if(P_BObj->Search(lot_rec.BillID, &bill_rec) > 0)
				PPObjBill::MakeCodeString(&bill_rec, 0, bill_buf);
		}
		else if(P_BObj->Search(billID, &bill_rec) > 0)
			PPObjBill::MakeCodeString(&bill_rec, 0, bill_buf);
		//"Отсутствует товар ID=%ld (лот ID=%ld Bill=%s N=%s q=%lf c=%.2lf p=%.2lf)"
		log_buf.Printf(fmt_buf, goodsID, lotID, bill_buf.cptr(), bill_rec.Code, lot_rec.Quantity, R5(lot_rec.Cost), R5(lot_rec.Price));
		rLogger.Log(log_buf);
		if(P.Flags & Param::fCorrectErrors) {
			Goods2Tbl::Rec goods_rec;
			if(P.DefGroupID == 0)
				THROW(GgObj.AddSimple(&P.DefGroupID, gpkndOrdinaryGroup, 0, "Average", 0, P.DefUnitID, 0));
			goods_rec.ID = goodsID;
			goods_rec.Kind = PPGDSK_GOODS;
			(log_buf = "Goods Stub").Space().CatChar('#').CatLongZ(goodsID, 5).CopyTo(goods_rec.Name, sizeof(goods_rec.Name));
			STRNSCPY(goods_rec.Abbr, goods_rec.Name);
			goods_rec.ParentID = P.DefGroupID;
			goods_rec.UnitID = P.DefUnitID;
			THROW_DB(GObj.P_Tbl->insertRecBuf(&goods_rec));
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrAbsentGoods::Run()
{
	int    ok = 1, ta = 0;
	PPLogger logger;
	PPWaitStart();
	THROW(PPStartTransaction(&ta, (P.Flags & Param::fCorrectErrors) ? 1 : 0));
	{
		IterCounter counter;
		{
			ReceiptCore * p_rcpt = &P_BObj->trfr->Rcpt;
			ReceiptTbl::Key0 k0;
			BExtQuery q(p_rcpt, 0, 128);
			q.select(p_rcpt->ID, p_rcpt->GoodsID, 0L);
			MEMSZERO(k0);
			counter.Init(q.countIterations(0, &k0, spFirst));
			MEMSZERO(k0);
			for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
				THROW(ProcessGoods(labs(p_rcpt->data.GoodsID), p_rcpt->data.ID, 0, logger));
				PPWaitPercent(counter.Increment());
			}
		}
		if(P_BObj->P_CpTrfr) {
			CpTransfTbl * t = P_BObj->P_CpTrfr;
			CpTransfTbl::Key0 cpk0, cpk0_;
			PPIDArray id_list;
			BExtQuery cpq(t, 0, 128);
			cpq.select(t->BillID, t->RByBill, t->GoodsID, 0L);
			MEMSZERO(cpk0);
			counter.Init(cpq.countIterations(0, &(cpk0_ = cpk0), spFirst));
			for(cpq.initIteration(false, &cpk0, spFirst); cpq.nextIteration() > 0;) {
				THROW(ProcessGoods(labs(t->data.GoodsID), 0, t->data.BillID, logger));
				PPWaitPercent(counter.Increment());
			}
		}
	}
	THROW(PPCommitWork(&ta));
	PPWaitStop();
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	logger.Save(P.LogFileName, 0);
	return ok;
}

int RecoverAbsenceGoods()
{
	int    ok = -1;
	PrcssrAbsentGoods prcssr;
	PrcssrAbsentGoods::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}
//
//
//
static int CheckLotList(PPIDArray & rLotList, PPIDArray & rAbsLotList, PPLogger & rLogger)
{
	int    ok = 1;
	SString msg_buf, fmt_buf, added_msg;
	ReceiptCore & r_rc = BillObj->trfr->Rcpt;
	uint   c = rLotList.getCount();
	for(uint i = 0; i < c; i++) {
		PPID   lot_id = rLotList.at(i);
		if(lot_id) {
			int    r = r_rc.Search(lot_id);
			if(!r) {
				PPGetLastErrorMessage(1, added_msg);
				PPFormatT(PPTXT_SEARCHLOTFAULT, &msg_buf, lot_id, added_msg.cptr());
				rLogger.Log(PPFormat(fmt_buf, &msg_buf, lot_id, added_msg.cptr()));
				ok = -1;
			}
			else if(r < 0) {
				if(rAbsLotList.addUnique(lot_id) > 0)
					rLogger.LogLastError();
				ok = -1;
			}
		}
	}
	rLotList.clear();
	return ok;
}

struct BadTrfrEntry { // @flat
	long   N;
	LDATE  Dt;
	long   OprNo;
	int    WasRemoved;
	char   Descr[240];
};

class BadTrfrEntryListDialog : public PPListDialog {
public:
	BadTrfrEntryListDialog(SVector * pList) : PPListDialog(DLG_R_TFR_A, CTL_R_TFR_A_LIST), P_List(pList)
	{
		updateList(-1);
	}
private:
	virtual int setupList()
	{
		if(P_List) {
			for(uint i = 0; i < P_List->getCount(); i++) {
				const BadTrfrEntry * p_entry = static_cast<const BadTrfrEntry *>(P_List->at(i));
				if(!p_entry->WasRemoved)
					addStringToList(p_entry->N, p_entry->Descr);
			}
		}
		return 1;
	}
	virtual int delItem(long pos, long id);
	SVector * P_List;
};

int BadTrfrEntryListDialog::delItem(long pos, long id)
{
	int    ok = -1;
	if(P_List && pos >= 0 && pos < P_List->getCountI()) {
		if(CONFIRM(PPCFM_DELETE)) {
			const BadTrfrEntry * p_entry = static_cast<const BadTrfrEntry *>(P_List->at(pos));
			Transfer * p_trfr = BillObj->trfr;
			THROW_DB(deleteFrom(p_trfr, 1, (p_trfr->Dt == p_entry->Dt && p_trfr->OprNo == p_entry->OprNo)));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int RecoverTransfer()
{
	int    ok = 1, ta = 0;
	int    do_recover = 1;
	const  double big = 1.e9;
	SString fmt_buf, msg_buf, fp_err_var, added_msg;
	PPLogger logger;
	PPObjGoods goods_obj;
	PPIDArray temp_lot_list, abs_lot_list;
	LAssocArray goods_list;
	Transfer * p_trfr = BillObj->trfr;
	TransferTbl::Rec & r_data = p_trfr->data;
	TransferTbl::Key1 k1;
	IterCounter cntr;
	PPInitIterCounter(cntr, p_trfr);
	SVector bad_list(sizeof(BadTrfrEntry));
	BExtQuery q(p_trfr, 1);
	PPWaitStart();
	q.selectAll();
	MEMSZERO(k1);
	for(q.initIteration(false, &k1, spFirst); q.nextIteration() > 0; cntr.Increment()) {
		temp_lot_list.addUnique(r_data.LotID);
		{
			int    fpok = 1;
			fp_err_var = 0;
#define _FZEROINV(v) if(!SIEEE754::IsValid(r_data.v) || fabs(r_data.v) > big) { fp_err_var.CatDivIfNotEmpty(';', 2).Cat(#v); fpok = 0; }
			_FZEROINV(Quantity);
			_FZEROINV(Rest);
			_FZEROINV(Cost);
			_FZEROINV(WtQtty);
			_FZEROINV(WtRest);
			_FZEROINV(Price);
			_FZEROINV(QuotPrice);
			_FZEROINV(Discount);
			_FZEROINV(CurPrice);
			if(!fpok) {
				logger.Log(PPFormatT(PPTXT_TRFRRECERROR, &msg_buf, r_data.BillID, r_data.GoodsID, r_data.LotID, fp_err_var.cptr()));
				BadTrfrEntry entry;
				MEMSZERO(entry);
				entry.N = bad_list.getCount()+1;
				entry.Dt = r_data.Dt;
				entry.OprNo = r_data.OprNo;
				STRNSCPY(entry.Descr, msg_buf);
				bad_list.insert(&entry);
			}
#undef _FZEROINV
		}
		if(temp_lot_list.getCount() > 4096)
			THROW(CheckLotList(temp_lot_list, abs_lot_list, logger));
		if((cntr % 1000) == 0)
			PPWaitPercent(cntr);
	}
	THROW(CheckLotList(temp_lot_list, abs_lot_list, logger));
	PPWaitStop();
	if(bad_list.getCount()) {
		BadTrfrEntryListDialog * dlg = new BadTrfrEntryListDialog(&bad_list);
		if(CheckDialogPtrErr(&dlg)) {
			do_recover = BIN(ExecViewAndDestroy(dlg) == cmOK);
		}
	}
#if 1 // {
	if(do_recover) {
		uint   i, j, c = abs_lot_list.getCount();
		if(c) {
			LDATE  first_date = ZERODATE;
			PPID   loc_id = 0;
			PPBillPacketCollection pack_list; // key: Rec.LocID
			PPBillPacket * p_pack = 0;
			PPWaitMsg("Recovering absence lots...");
			abs_lot_list.sort();
			THROW(PPStartTransaction(&ta, 1));
			for(i = 0; i < c; i++) {
				TransferTbl::Rec rec;
				PPID   lot_id = abs_lot_list.at(i);
				PPID   goods_id = 0;
				double qtty = 0.0, cost = 0.0, price = 0.0;
				double max_qtty = 0.0;
				int    is_first_op = 1;
				for(DateIter di; p_trfr->EnumByLot(lot_id, &di, &rec) > 0;) {
					if(!first_date || di.dt < first_date)
						first_date = di.dt;
					if(is_first_op) {
						if(rec.Flags & PPTFR_RECEIPT) {
							//
							// Первая операция по лоту должна генерировать лот - следует
							// использовать процедуру восстановления отсутствующих лотов.
							//
							qtty = 0;
							loc_id = 0;
							break;
						}
						else {
							cost  = rec.Cost;
							price = rec.Price;
						}
					}
					qtty    -= rec.Quantity;
					if(rec.Quantity < 0)
						max_qtty -= rec.Quantity;
					loc_id   = rec.LocID;
					goods_id = rec.GoodsID;
					is_first_op = 0;
				}
				if((qtty > 0 || !is_first_op) && loc_id && goods_id) {
					PPTransferItem ti;
					for(j = 0; !p_pack && j < pack_list.getCount(); j++) {
						PPBillPacket * p = pack_list.at(j);
						if(p && p->Rec.LocID == loc_id)
							p_pack = p;
					}
					if(!p_pack) {
						THROW_SL(p_pack = pack_list.CreateNewItem());
						THROW(p_pack->CreateBlank2(CConfig.ReceiptOp, first_date, loc_id, 0));
						PPGetWord(PPWORD_AT_AUTO, 0, p_pack->SMemo);
					}
					THROW(ti.Init(&p_pack->Rec));
					ti.GoodsID  = goods_id;
					ti.Cost     = cost;
					ti.Price    = price;
					ti.Quantity_ = (qtty > 0) ? qtty : max_qtty;
					ti.SetupSign(p_pack->Rec.OpID);
					ti.LotID  = lot_id;
					ti.TFlags |= PPTransferItem::tfForceLotID;
					THROW(p_pack->InsertRow(&ti, 0));
				}
			}
			for(i = 0; i < pack_list.getCount(); i++) {
				p_pack = pack_list.at(i);
				if(p_pack) {
					p_pack->Rec.Dt = first_date;
					THROW(p_pack->InitAmounts(0));
					DS.GetTLA().Cc.Flags |= CCFLG_TRFR_DONTRECALCREVAL;
					THROW(BillObj->TurnPacket(p_pack, 0));
					logger.LogAcceptMsg(PPOBJ_BILL, p_pack->Rec.ID, 0);
				}
			}
			THROW(PPCommitWork(&ta));
		}
	}
#endif // } 0
	PPWaitStop();
	CATCH
		PPRollbackWork(&ta);
		PPError();
		ok = (logger.LogLastError(), 0);
	ENDCATCH
	logger.Save("recover_transfer.log", 0);
	DS.GetTLA().Cc.Flags &= ~CCFLG_TRFR_DONTRECALCREVAL;
	return ok;
}
//
// RecoverAbsenceAccounts
//
class PrcssrAbsenceAccounts {
public:
	struct Param {
		enum {
			fCorrectErrors = 0x0001
		};
		PPID   CurID;
		long   Flags;
		SString LogFileName;
	};
	PrcssrAbsenceAccounts()
	{
	}
	int    InitParam(Param * pParam)
	{
		memzero(pParam, sizeof(Param));
		PPGetFileName(PPFILNAM_ABSACCOUNTS_LOG, pParam->LogFileName);
		return 1;
	}
	int    Init(const Param * pParam)
	{
		if(!RVALUEPTR(P, pParam))
			MEMSZERO(P);
		AbsentCount = 0;
		return 1;
	}
	int    EditParam(Param *);
	int    Run();
private:
	Param  P;
	long   AbsentCount;
	AcctRel ARel;
	//AccountCore AcctTbl;
	PPObjAccount AccObj;
};

int PrcssrAbsenceAccounts::EditParam(Param * pParam)
{
	int    ok = -1;
	TDialog * dlg = 0;
	Param  p;
	if(!RVALUEPTR(p, pParam))
		MEMSZERO(p);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_ABSACCT))));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_ABSACCT_LOGFNAME, CTL_ABSACCT_LOGFNAME, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
	dlg->setCtrlString(CTL_ABSACCT_LOGFNAME, p.LogFileName);
	dlg->AddClusterAssoc(CTL_ABSACCT_FLAGS, 0, Param::fCorrectErrors);
	dlg->SetClusterData(CTL_ABSACCT_FLAGS, p.Flags);
	SetupCurrencyCombo(dlg, CTLSEL_ABSACCT_CUR, p.CurID, 0, 1, 0);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_ABSACCT_LOGFNAME, p.LogFileName);
		dlg->GetClusterData(CTL_ABSACCT_FLAGS, &p.Flags);
		dlg->getCtrlData(CTLSEL_ABSACCT_CUR,   &p.CurID);
		ASSIGN_PTR(pParam, p);
		ok = 1;
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int PrcssrAbsenceAccounts::Run()
{
	// @todo Протестировать обязательно!
	int    ok = 1;
	AcctRelTbl::Key0 k;
	SString fmt_buf, rcvrd_word;
	DBQ * dbq = 0;
	BExtQuery q(&ARel, 0);
	PPLogger logger;
	MEMSZERO(k);
	PPLoadString(PPMSG_ERROR, PPERR_ABSACCOUNT, fmt_buf);
	PPGetWord(PPWORD_RECOVERED, 0, rcvrd_word);
	dbq = ppcheckfiltid(dbq, ARel.CurID, P.CurID);
	q.selectAll().where(*dbq);
	for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
		int    recovered = 0;
		SString log_buf, acc_name_buf;
		AcctRelTbl::Rec acr_rec;
		ARel.copyBufTo(&acr_rec);
		if(AccObj.Search(acr_rec.AccID) < 0) {
			if(P.Flags & Param::fCorrectErrors) {
				PPAccount temp_rec;
				PPAccountPacket pack;
				PPID   new_acc_id = 0;
				pack.Rec.ID    = acr_rec.AccID;
				pack.Rec.A.Ac  = acr_rec.Ac;
				pack.Rec.A.Sb  = acr_rec.Sb;
				pack.Rec.CurID = acr_rec.CurID;
				pack.Rec.Type  = ACY_BAL;
				pack.Rec.Kind  = ACT_AP;
				while(AccObj.SearchNum(pack.Rec.A.Ac, pack.Rec.A.Sb, pack.Rec.CurID, &temp_rec) > 0) {
					pack.Rec.A.Sb += 100;
				}
				acc_name_buf.Z().Cat(pack.Rec.A.Ac).Dot().Cat(pack.Rec.A.Sb).Space().CatChar('#').Cat(pack.Rec.ID);
				acc_name_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				GetArticleSheetID(acr_rec.ArticleID, &pack.Rec.AccSheetID);
				THROW(AccObj.PutPacket(&new_acc_id, &pack, 1));
				recovered = 1;
			}
			log_buf.Printf(fmt_buf, ARel.data.AccID, ARel.data.Ac, ARel.data.Sb);
			if(recovered)
				log_buf.CatDiv('-', 1).Cat(rcvrd_word);
			logger.Log(log_buf);
			AbsentCount++;
		}
	}
	CATCHZOK
	logger.Save(P.LogFileName, 0);
	return ok;
}

int RecoverAbsenceAccounts()
{
	int    ok = -1;
	PrcssrAbsenceAccounts prcssr;
	PrcssrAbsenceAccounts::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	return ok;
}
//
//
//
class PrcssrAbsenceTrfr {
public:
	struct Param {
		long   Flags;
	};
	PrcssrAbsenceTrfr()
	{
		MEMSZERO(P);
		P_BObj = BillObj;
	}
	int    InitParam(Param * pParam)
	{
		memzero(pParam, sizeof(*pParam));
		return 1;
	}
	int    EditParam(Param *)
	{
		return 1;
	}
	int    Init(const Param *);
	int    Run();
private:
	int    MakeTrfrByLot(const ReceiptTbl::Rec & rLotRec, const BillTbl::Rec & rBillRec,
		TransferTbl::Rec & rRec, Transfer::Rec & rMirrorRec);

	Param  P;
	PPObjBill * P_BObj;
};

int PrcssrAbsenceTrfr::Init(const Param * pParam)
{
	P = *pParam;
	return 1;
}

long FASTCALL MASK_TFR_FLAGS(long f); // @prototype

int PrcssrAbsenceTrfr::MakeTrfrByLot(const ReceiptTbl::Rec & rLotRec, const BillTbl::Rec & rBillRec,
	TransferTbl::Rec & rRec, Transfer::Rec & rMirrorRec)
{
	int    ok = 1;
	int    reverse = 0;
	long   oprno = 0;
	PPTransferItem temp_ti(&rBillRec, TISIGN_UNDEF);
	MEMSZERO(rRec);
	rRec.LocID     = rLotRec.LocID;
	rRec.Dt        = rLotRec.Dt;
	rRec.BillID    = rLotRec.BillID;
	THROW(P_BObj->trfr->RecByBill(rRec.BillID, &rRec.RByBill));
	THROW(P_BObj->trfr->GetOprNo(rRec.Dt, &oprno));
	rRec.OprNo = oprno;
	if(rLotRec.PrevLotID == 0) {
		rRec.Reverse = 0;
		rRec.CorrLoc = 0;
	}
	else {
		reverse = 1;
		rRec.Reverse = 1;
		rRec.CorrLoc = rBillRec.LocID;
		if(rRec.CorrLoc == rRec.LocID)
			ok = -1;
		else {
			MEMSZERO(rMirrorRec);
			rMirrorRec.LocID     = rBillRec.LocID;
			rMirrorRec.Dt        = rLotRec.Dt;
			rMirrorRec.OprNo     = oprno+1;
			rMirrorRec.BillID    = rLotRec.BillID;
			rMirrorRec.RByBill   = rRec.RByBill;
			rMirrorRec.Reverse   = 0;
			rMirrorRec.LotID     = rLotRec.PrevLotID;
			rMirrorRec.GoodsID   = rLotRec.GoodsID;
			rMirrorRec.Flags     = MASK_TFR_FLAGS(temp_ti.Flags);
			rMirrorRec.Quantity  = -rLotRec.Quantity;
			rMirrorRec.Rest      = 0; // ??
			rMirrorRec.Cost      = rLotRec.Cost;
			rMirrorRec.WtQtty    = rLotRec.WtQtty;
			rMirrorRec.WtRest    = 0; // ??
			rMirrorRec.Price     = rLotRec.Price;
			rMirrorRec.QuotPrice = 0.0;
			rMirrorRec.Discount  = 0.0;
			rMirrorRec.CurID     = rBillRec.CurID;
			ok = 2;
		}
	}
	rRec.LotID     = rLotRec.ID;
	rRec.GoodsID   = rLotRec.GoodsID;
	if(reverse)
		rRec.Flags = MASK_TFR_FLAGS(temp_ti.Flags) | PPTFR_RECEIPT;
	else
		rRec.Flags = MASK_TFR_FLAGS(temp_ti.Flags);
	rRec.Quantity  = rLotRec.Quantity;
	rRec.Rest      = rLotRec.Quantity;
	rRec.Cost      = rLotRec.Cost;
	rRec.WtQtty    = rLotRec.WtQtty;
	rRec.WtRest    = rLotRec.WtRest;
	rRec.Price     = rLotRec.Price;
	rRec.QuotPrice = 0.0;
	rRec.Discount  = 0.0;
	rRec.CurID     = rBillRec.CurID;
	// ?? rRec.CurPrice
	CATCHZOK
	return ok;
}

int PrcssrAbsenceTrfr::Run()
{
	int    ok = -1, ta = 0;
	PPLogger logger;
	LDATE  startdate = encodedate(1, 4, 2007);
	Transfer & trfr = *P_BObj->trfr;
	ReceiptCore & rcpt = trfr.Rcpt;
	ReceiptTbl::Key1 rk1;
	BExtQuery q(&rcpt, 1);
	q.selectAll();
	MEMSZERO(rk1);
	rk1.Dt = startdate;
	THROW(PPStartTransaction(&ta, 1));
	for(q.initIteration(false, &rk1, spGe); q.nextIteration() > 0;) {
		PPWaitDate(rcpt.data.Dt);
		TransferTbl::Key2 tk2;
		tk2.LotID = rcpt.data.ID;
		tk2.Dt = rcpt.data.Dt;
		tk2.OprNo = 0;
		int r = trfr.search(2, &tk2, spGe);
		TransferTbl::Rec new_rec, new_mirror;
		BillTbl::Rec bill_rec;
		int r2 = -2;
		if(r) {
			if(trfr.data.LotID != rcpt.data.ID || !(trfr.data.Flags & PPTFR_RECEIPT) || trfr.data.Quantity <= 0) {
				if(P_BObj->Search(rcpt.data.BillID, &bill_rec) > 0) {
					r2 = MakeTrfrByLot(rcpt.data, bill_rec, new_rec, new_mirror);
				}
			}
		}
		else if(BTRNFOUND) {
			if(P_BObj->Search(rcpt.data.BillID, &bill_rec) > 0) {
				r2 = MakeTrfrByLot(rcpt.data, bill_rec, new_rec, new_mirror);
			}
		}
		else
			logger.LogLastError();
		if(r2 == 0)
			logger.LogLastError();
		else if(r2 > 0) {
			if(r2 == 1) {
				if(!trfr.insertRecBuf(&new_rec))
					logger.LogLastError();
			}
			else if(r2 == 2) {
				if(!trfr.insertRecBuf(&new_rec))
					logger.LogLastError();
				if(!trfr.insertRecBuf(&new_mirror))
					logger.LogLastError();
			}
		}
	}
	PPCommitWork(&ta);
	CATCH
		PPRollbackWork(&ta);
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int RecoverAbsenceTrfr()
{
	int    ok = -1;
	PrcssrAbsenceTrfr prcssr;
	PrcssrAbsenceTrfr::Param param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0) {
		PPWaitStart();
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
		PPWaitStop();
	}
	return ok;
}
//
//
//
#if 0 // @construction {

class PrcssrReceiptPacking {
public:
	struct Param {
		long   Flags;
	};
	PrcssrReceiptPacking() : P_BObj(BillObj)
	{
		MEMSZERO(P);
	}
	int    InitParam(Param * pParam)
	{
		memzero(pParam, sizeof(*pParam));
		return 1;
	}
	int    EditParam(Param *)
	{
		return 1;
	}
	int    Init(const Param *);
	int    Run();
private:
	Param  P;
	PPObjBill * P_BObj;
};

int PrcssrReceiptPacking::Run()
{
	const uint max_free_count = 1000000;
	const uint ta_quant = 100;

	//const uint max_free_count = 1000; // @debug
	//const uint ta_quant = 10;         // @debug

	int    ok = 1;
	SString msg_buf, fmt_buf;
	PPLogger logger;
	ReceiptCore & r_lot_t = P_BObj->trfr->Rcpt;
	Transfer & r_trfr_t = *P_BObj->trfr;
	CpTransfCore * p_cptrfr_t = P_BObj->P_CpTrfr;
	PackageLinkTbl pkl_t;
	LotCurRestTbl lcr_t;
	LocTransfTbl lt_t;
	CpTransfTbl ct_t;
	TSessionCore tses_t;
	ObjTagCore & r_ot_t = PPRef->Ot;
	PPTransaction * p_tra = 0;
	uint   processed_count = 0;
	uint   free_id_count = 0;
	UintHashTable free_id_tab;

	struct CpTransfLotKey {
		PPID   LotID;
		PPID   BillID;
		int16  RByBill;
	};
	SArray cp_transf_lot_list(sizeof(CpTransfLotKey));

	struct TSessLineLotKey {
		PPID   LotID;
		PPID   SessID;
		long   OprNo;
	};
	SArray tsesln_lot_list(sizeof(TSessLineLotKey));
	PPWaitStart();
	//
	// Сканируем таблицу Receipt на предмет окон идентификаторов
	//
	{
		PPLoadText(PPTXT_LOTPACK_SCAN, msg_buf);
		PPWaitMsg(msg_buf);
		ReceiptTbl::Key0 k0;
		BExtQuery q(&r_lot_t, 0);
		q.select(r_lot_t.ID, 0);
		k0.ID = 1;
		long   prev_id = 0;
		for(q.initIteration(false, &k0, spGe); free_id_count < max_free_count && q.nextIteration() > 0;) {
			long   id = r_lot_t.data.ID;
			if(id > (prev_id+1)) {
				for(long i = prev_id+1; i < id; i++) {
					free_id_tab.Add((ulong)i);
					free_id_count++;
				}
			}
			prev_id = id;
		}
	}
	//
	// Поле OrdLotID в таблице CpTransfer не проиндексировано - сканируем таблицу и создаем список
	// ссылок на лоты в ней.
	//
	if(p_cptrfr_t) {
		CpTransfTbl::Key0 k0;
		MEMSZERO(k0);
		BExtQuery q(p_cptrfr_t, 0);
		q.select(p_cptrfr_t->BillID, p_cptrfr_t->RByBill, p_cptrfr_t->OrdLotID, 0).where(p_cptrfr_t->OrdLotID > 0L);
		for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
			CpTransfLotKey item;
			item.LotID = p_cptrfr_t->data.OrdLotID;
			item.BillID = p_cptrfr_t->data.BillID;
			item.RByBill = p_cptrfr_t->data.RByBill;
			THROW_SL(cp_transf_lot_list.insert(&item));
		}
		cp_transf_lot_list.sort(CMPF_LONG);
	}
	//
	// Поле LotID в таблице TSessLine не проиндексировано - сканируем таблицу и создаем список
	// ссылок на лоты в ней.
	//
	{
		TSessLineTbl::Key0 k0;
		MEMSZERO(k0);
		BExtQuery q(&tses_t.Lines, 0);
		q.select(tses_t.Lines.TSessID, tses_t.Lines.OprNo, tses_t.Lines.LotID, 0).where(tses_t.Lines.LotID > 0L);
		for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
			TSessLineLotKey item;
			item.LotID = tses_t.Lines.data.LotID;
			item.SessID = tses_t.Lines.data.TSessID;
			item.OprNo = tses_t.Lines.data.OprNo;
			THROW_SL(tsesln_lot_list.insert(&item));
		}
		tsesln_lot_list.sort(CMPF_LONG);
	}
	{
		/*
			Receipt: PrevLotID     +
			Transfer: LotID        +
			PackageLink: LotID     +
			LotCurRest: LotID      +
			LocTransf: LotID       +
			CpTransf: OrdLotID
			TSession: OrderLotID   +
			TSessLine: LotID
			ObjectTag
		*/
		ulong  free_tab_iter = 0;
		ReceiptTbl::Key0 k0;
		BExtQuery q(&r_lot_t, 0);
		q.select(r_lot_t.ID, 0);
		k0.ID = MAXLONG;
		THROW_MEM(p_tra = new PPTransaction(0, 1));
		THROW(*p_tra);
		while(r_lot_t.search(0, &k0, spLt) && free_id_tab.Enum(&free_tab_iter)) {
			PPID   new_id = 0;
			ReceiptTbl::Rec rec, new_rec;
			//
			// Скопировали буфер данных из найденной записи
			//
 			r_lot_t.copyBufTo(&rec);
			r_lot_t.copyBufTo(&new_rec);
			//
			// Сразу удаляем запись - иначе вставка новой записи не состоится из-за дублирования индексов
			//
			THROW_DB(r_lot_t.deleteRec()); // @sfu
			//
			// Подменяем идентификатор и вставляем запись назад
			//
			new_rec.ID = (long)free_tab_iter;
			r_lot_t.copyBufFrom(&new_rec);
			THROW_DB(r_lot_t.insertRec(0, &new_id));
			//
			assert(new_id == new_rec.ID);
			assert(new_id == (long)free_tab_iter);
			{
				ReceiptTbl::Key4 k4; // PrevLotID, Dt, OprNo (unique mod);                // #4
				MEMSZERO(k4);
				k4.PrevLotID = rec.ID;
				while(r_lot_t.searchForUpdate(4, &k4, spGe) && r_lot_t.data.PrevLotID == rec.ID) {
					r_lot_t.data.PrevLotID = new_id;
					THROW_DB(r_lot_t.updateRec()); // @sfu
					MEMSZERO(k4);
					k4.PrevLotID = rec.ID;
				}
			}
			{
				TransferTbl::Key2 k2;
				MEMSZERO(k2);
				k2.LotID = rec.ID;
				while(r_trfr_t.searchForUpdate(2, &k2, spGe) && r_trfr_t.data.LotID == rec.ID) {
					r_trfr_t.data.LotID = new_id;
					THROW_DB(r_trfr_t.updateRec()); // @sfu
					MEMSZERO(k2);
					k2.LotID = rec.ID;
				}
			}
			{
				PackageLinkTbl::Key1 k1;
				k1.LotID = rec.ID;
				if(pkl_t.searchForUpdate(1, &k1, spEq) && pkl_t.data.LotID == rec.ID) {
					pkl_t.data.LotID = new_id;
					THROW_DB(pkl_t.updateRec()); // @sfu
				}
			}
			{
				LotCurRestTbl::Key0 k0;
				MEMSZERO(k0);
				k0.LotID = rec.ID;
				while(lcr_t.searchForUpdate(0, &k0, spGe) && lcr_t.data.LotID == rec.ID) {
					lcr_t.data.LotID = new_id;
					THROW_DB(lcr_t.updateRec()); // @sfu
					MEMSZERO(k0);
					k0.LotID = rec.ID;
				}
			}
			{
				LocTransfTbl::Key1 k1;
				MEMSZERO(k1);
				k1.LotID = rec.ID;
				while(lt_t.searchForUpdate(1, &k1, spGe) && lt_t.data.LotID == rec.ID) {
					lt_t.data.LotID = new_id;
					THROW_DB(lt_t.updateRec()); // @sfu
					MEMSZERO(k1);
					k1.LotID = rec.ID;
				}
			}
			if(p_cptrfr_t) {
				uint   cppos = 0;
				if(cp_transf_lot_list.bsearch(&rec.ID, &cppos, CMPF_LONG)) {
					for(uint p = cppos; p < cp_transf_lot_list.getCount(); p++) {
						const CpTransfLotKey * p_key = static_cast<const CpTransfLotKey *>(cp_transf_lot_list.at(p));
						if(p_key && p_key->LotID == rec.ID) {
							CpTransfTbl::Key0 k0;
							k0.BillID = p_key->BillID;
							k0.RByBill = p_key->RByBill;
							if(p_cptrfr_t->searchForUpdate(0, &k0, spEq)) {
								if(p_cptrfr_t->data.OrdLotID == rec.ID) { // @paranoic
									p_cptrfr_t->data.OrdLotID = new_id;
									THROW_DB(p_cptrfr_t->updateRec()); // @sfu
								}
							}
						}
						else
							break;
					}
				}
			}
			{
				TSessionTbl::Key7 k7;
				MEMSZERO(k7);
				k7.OrderLotID = rec.ID;
				while(tses_t.searchForUpdate(7, &k7, spGe) && tses_t.data.OrderLotID == rec.ID) {
					tses_t.data.OrderLotID = new_id;
					THROW_DB(tses_t.updateRec()); // @sfu
					MEMSZERO(k7);
					k7.OrderLotID = rec.ID;
				}
			}
			{
				uint   tslp = 0;
				if(tsesln_lot_list.bsearch(&rec.ID, &tslp, CMPF_LONG)) {
					for(uint p = tslp; p < tsesln_lot_list.getCount(); p++) {
						const TSessLineLotKey * p_key = static_cast<const TSessLineLotKey *>(tsesln_lot_list.at(p));
						if(p_key && p_key->LotID == rec.ID) {
							TSessLineTbl::Key0 k0;
							k0.TSessID = p_key->SessID;
							k0.OprNo = p_key->OprNo;
							if(tses_t.Lines.searchForUpdate(0, &k0, spEq)) {
								if(tses_t.Lines.data.LotID == rec.ID) { // @paranoic
									tses_t.Lines.data.LotID = new_id;
									THROW_DB(tses_t.Lines.updateRec()); // @sfu
								}
							}
						}
						else
							break;
					}
				}
			}
			{
				ObjTagTbl::Key0 k0;
				MEMSZERO(k0);
				k0.ObjType = PPOBJ_LOT;
				k0.ObjID = rec.ID;
				while(r_ot_t.searchForUpdate(0, &k0, spGe) && r_ot_t.data.ObjType == PPOBJ_LOT && r_ot_t.data.ObjID == rec.ID) {
					r_ot_t.data.ObjID = new_id;
					THROW_DB(r_ot_t.updateRec()); // @sfu
					MEMSZERO(k0);
					k0.ObjType = PPOBJ_LOT;
					k0.ObjID = rec.ID;
				}
			}
			processed_count++;
			if((processed_count % ta_quant) == 0) {
				THROW(p_tra->Commit());
				ZDELETE(p_tra);
				THROW_MEM(p_tra = new PPTransaction(0, 1));
				THROW(*p_tra);
			}
			if((processed_count % 10) == 0) {
				PPLoadText(PPTXT_LOTPACK_PROCESS, fmt_buf);
				PPWaitMsg((msg_buf = fmt_buf).Space().Cat(processed_count).Slash().Cat(free_id_count));
			}
		}
		THROW(p_tra->Commit());
		ZDELETE(p_tra);
	}
	PPWaitStop();
	CATCHZOKPPERR
	delete p_tra;
	return ok;
}

#endif // } 0 @construction

int ReceiptPacking()
{
	//PrcssrReceiptPacking prcssr;
	//return prcssr.Run();
	return -1;
}
