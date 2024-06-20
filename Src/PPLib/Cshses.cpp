// CSHSES.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2024
// @codepage UTF-8
// Интерфейс с асинхронными кассовыми устройствами
//
#include <pp.h>
#pragma hdrstop
//
// PPSyncCashSession
//
/*static*/bool PPSyncCashSession::GetCurrentUserName(SString & rBuf)
{
	rBuf.Z();
	if(PPObjPerson::GetCurUserPerson(0, &rBuf) > 0) {
		;
	}
	else if(GetCurUserName(rBuf).IsEmpty()) {
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		PPSecur sec_rec;
		if(sec_obj.Fetch(LConfig.UserID, &sec_rec) > 0)
			rBuf = sec_rec.Name;
	}
	return rBuf.NotEmptyS();
}

PPSyncCashSession::PPSyncCashSession(PPID n, const char * /*pName*/, const char * /*pPort*/) : State(0), NodeID(n), Handle(-1), PortType(0), P_SlipFmt(0)
{
	Name[0] = 0;
	Port[0] = 0;
	if(CnObj.GetSync(NodeID, &SCn) > 0)
		P_SlipFmt = new PPSlipFormatter(SCn.SlipFmtPath);
	else
		State |= stError;
}

PPSyncCashSession::~PPSyncCashSession()
{
	ZDELETE(P_SlipFmt);
	if(PortType == 0 && Handle >= 0)
		close(Handle);
}

int PPSyncCashSession::IsError() const { return BIN(State & stError); }

int PPSyncCashSession::Init(const char * pName, const char * pPort)
{
	PortType = 0;
	Handle = -1;
	STRNSCPY(Name, pName);
	if(pPort) {
		STRNSCPY(Port, pPort);
		int    c = 0;
		const  int comdvcs = IsComDvcSymb(Port, &c);
		if(comdvcs == comdvcsCom && (c >= 1 && c <= 32)) {
			PortType = 2;
			Handle = c;
		}
		else if(comdvcs == comdvcsLpt && (c >= 1 && c <= 3)) {
			PortType = 1;
			Handle = c;
		}
		else if(comdvcs == comdvcsPrn) {
			PortType = 1;
			Handle = 1;
		}
		else if(sstreqi_ascii(Port, "server")) { // @v12.0.3
			PortType = 3;
		}
		if(PortType == 0)
			Handle = open(pPort, O_CREAT|/*O_TRUNC*/O_APPEND|O_TEXT|O_WRONLY, S_IWRITE);
		else {
			// @v10.3.9 c = InitChannel();
			return 1;
		}
	}
	return Handle;
}

PPSyncCashSession::OfdFactors::OfdFactors() : ChZnGuaID(0), ChZnPermissiveMode(0)
{
}

PPSyncCashSession::OfdFactors & PPSyncCashSession::OfdFactors::Z()
{
	OfdVer_.Z();
	ChZnGuaID = 0;
	ChZnPermissiveMode = 0;
	Sid.Z();
	return *this;
}

bool PPSyncCashSession::OfdFactors::IsOfdVerGe12() const { return (!OfdVer_.IsEmpty() && OfdVer_.IsGe(1, 2, 0)); }

void PPSyncCashSession::GetOfdFactors(OfdFactors & rP)
{
	rP.Z();
	if(CnObj.P_Ref) {
		if(SCn.LocID)
			CnObj.P_Ref->Ot.GetTagStr(PPOBJ_LOCATION, SCn.LocID, PPTAG_LOC_CHZNCODE, rP.Sid);
		SString temp_buf;
		CnObj.P_Ref->Ot.GetTagStr(PPOBJ_CASHNODE, NodeID, PPTAG_POSNODE_OFDVER, temp_buf);
		rP.OfdVer_.FromStr(temp_buf);
		rP.ChZnGuaID = SCn.ChZnGuaID;
		rP.ChZnPermissiveMode = SCn.ChZnPermissiveMode;
	}
}

/*static*/bool PPSyncCashSession::IsSimplifiedDraftBeerPosition(PPID posNodeID, PPID goodsID)
{
	bool yes = false;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(goods_obj.Fetch(goodsID, &goods_rec) > 0) {
		PPGoodsType gt_rec;
		if(goods_rec.GoodsTypeID && goods_obj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0) {
			if(gt_rec.ChZnProdType == GTCHZNPT_DRAFTBEER) {
				if(posNodeID) {
					PPObjCashNode cn_obj;
					PPCashNode2 cn_rec;
					if(cn_obj.Fetch(posNodeID, &cn_rec) > 0 && cn_rec.Speciality == PPCashNode::spCafe) {
						yes = true;
					}
				}
			}
		}
	}
	return yes;
}

/*static*/bool PPSyncCashSession::IsAutoWriteOffDraftBeerPosition(PPID posNodeID, PPID goodsID) // @v12.0.5
{
	bool yes = false;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(goods_obj.Fetch(goodsID, &goods_rec) > 0) {
		PPGoodsType gt_rec;
		if(goods_rec.GoodsTypeID && goods_obj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0) {
			if(gt_rec.ChZnProdType == GTCHZNPT_DRAFTBEER_AWR) {
				yes = true;
			}
		}
	}
	return yes;
}

int PPSyncCashSession::PreprocessCCheckForOfd12(const OfdFactors & rOfdf, CCheckPacket * pPack)
{
	int    ok = -1;
	if(pPack && pPack->GetCount() && rOfdf.IsOfdVerGe12()) {
		PPObjGoods goods_obj;
		CCheckLineTbl::Rec ccl;
		SString chzn_code;
		SString temp_buf;
		bool is_there_chzn_marks = false;
		bool is_there_simplified_draftbeer = false;
		{
			for(uint pos = 0; !is_there_chzn_marks && !is_there_simplified_draftbeer && pPack->EnumLines(&pos, &ccl) > 0;) {
				if(PPSyncCashSession::IsSimplifiedDraftBeerPosition(NodeID, ccl.GoodsID) && goods_obj.GetSimplifiedDraftBeerBarcode(ccl.GoodsID, temp_buf)) { // @v11.9.3
					is_there_simplified_draftbeer = true;
				}
				else {
					pPack->GetLineTextExt(pos, CCheckPacket::lnextChZnMark, chzn_code);
					if(chzn_code.NotEmptyS()) {
						GtinStruc gts;
						if(PPChZnPrcssr::InterpretChZnCodeResult(PPChZnPrcssr::ParseChZnCode(chzn_code, gts, 0)) > 0) {
							is_there_chzn_marks = true;
						}
					}
				}
			}
		}
		if(is_there_chzn_marks || is_there_simplified_draftbeer) {
			int pczcr_pre = 0;
			{
				CCheckPacket::PreprocessChZnCodeResult chzn_pp_result;
				pczcr_pre = PreprocessChZnCode(ppchzcopInit, chzn_code, 1.0, 0/*uomId*/, 0, chzn_pp_result);
			}
			if(pczcr_pre != 0) {
				for(uint pos = 0; pPack->EnumLines(&pos, &ccl) > 0;) {
					Goods2Tbl::Rec goods_rec;
					uint  uom_fragm = 0;
					if(goods_obj.Fetch(ccl.GoodsID, &goods_rec) > 0) {
						if(PPSyncCashSession::IsSimplifiedDraftBeerPosition(NodeID, ccl.GoodsID) && goods_obj.GetSimplifiedDraftBeerBarcode(ccl.GoodsID, temp_buf)) { // @v11.9.3
							CCheckPacket::PreprocessChZnCodeResult chzn_pp_result;
							chzn_code.Z().Cat("0").Cat(temp_buf);
							double ratio = 0.0;
							if(goods_obj.TranslateGoodsUnitToBase(goods_rec, SUOM_LITER, &ratio) > 0) {
								double chzn_qtty = fabs(ccl.Quantity) * ratio;
								int pczcr = PreprocessChZnCode(ppchzcopSurrogateCheck, chzn_code, chzn_qtty, SUOM_LITER, uom_fragm, chzn_pp_result);
								// @v11.9.10 {
								if(/*pczcr > 0*/true) {
									if(/*chzn_pp_result.Status == 1*/true) {
										chzn_pp_result.LineIdx = pos;
										const int accept_op = ppchzcopAccept;
										pczcr = PreprocessChZnCode(accept_op, chzn_code, chzn_qtty, 0/*uomId*/, uom_fragm, chzn_pp_result);
										PPSyncCashSession::LogPreprocessChZnCodeResult(pczcr, accept_op, chzn_code, chzn_qtty, chzn_pp_result); // @v11.2.3
										if(pczcr > 0)
											pPack->SetLineChZnPreprocessResult(pos, &chzn_pp_result);
									}
									else {
										ok = 2;
										chzn_pp_result.LineIdx = pos;
										const int accept_op = ppchzcopReject;
										pczcr = PreprocessChZnCode(accept_op, chzn_code, chzn_qtty, 0/*uomId*/, uom_fragm, chzn_pp_result);
										PPSyncCashSession::LogPreprocessChZnCodeResult(pczcr, accept_op, chzn_code, chzn_qtty, chzn_pp_result); // @v11.2.3
										// @v11.7.0 if(pczcr > 0)
										pPack->SetLineChZnPreprocessResult(/*pos*/0, &chzn_pp_result); // @v11.7.0 pos-->0 (экспериментально: чтобы кассовый чек не содержал эту марку)
									}
								}
								// } @v11.9.10 
							}
						}
						else {
							pPack->GetLineTextExt(pos, CCheckPacket::lnextChZnMark, chzn_code);
							if(chzn_code.NotEmptyS()) {
								GtinStruc gts;
								if(PPChZnPrcssr::InterpretChZnCodeResult(PPChZnPrcssr::ParseChZnCode(chzn_code, gts, 0)) > 0) {
									ok = 1;
									CCheckPacket::PreprocessChZnCodeResult chzn_pp_result;
									PPChZnPrcssr::ReconstructOriginalChZnCode(gts, chzn_code);
									const double chzn_qtty = fabs(ccl.Quantity);
									PPUnit u_rec;
									if(goods_obj.FetchUnit(goods_rec.UnitID, &u_rec) > 0 && u_rec.Fragmentation > 0 && u_rec.Fragmentation < 100000)
										uom_fragm = u_rec.Fragmentation;
									int pczcr = PreprocessChZnCode(ppchzcopCheck, chzn_code, chzn_qtty, 0/*uomId*/, uom_fragm, chzn_pp_result);
									PPSyncCashSession::LogPreprocessChZnCodeResult(pczcr, 0, chzn_code, chzn_qtty, chzn_pp_result);
									// @debug {
									//pczcr = 0;
									//chzn_pp_result.Z();
									// } @debug
									if(pczcr > 0) {
										if(chzn_pp_result.Status == 1) {
											chzn_pp_result.LineIdx = pos;
											const int accept_op = ppchzcopAccept;
											pczcr = PreprocessChZnCode(accept_op, chzn_code, chzn_qtty, 0/*uomId*/, uom_fragm, chzn_pp_result);
											PPSyncCashSession::LogPreprocessChZnCodeResult(pczcr, accept_op, chzn_code, chzn_qtty, chzn_pp_result); // @v11.2.3
											if(pczcr > 0)
												pPack->SetLineChZnPreprocessResult(pos, &chzn_pp_result);
										}
										else {
											ok = 2;
											chzn_pp_result.LineIdx = pos;
											const int accept_op = ppchzcopReject;
											pczcr = PreprocessChZnCode(accept_op, chzn_code, chzn_qtty, 0/*uomId*/, uom_fragm, chzn_pp_result);
											PPSyncCashSession::LogPreprocessChZnCodeResult(pczcr, accept_op, chzn_code, chzn_qtty, chzn_pp_result); // @v11.2.3
											// @v11.7.0 if(pczcr > 0)
												pPack->SetLineChZnPreprocessResult(/*pos*/0, &chzn_pp_result); // @v11.7.0 pos-->0 (экспериментально: чтобы кассовый чек не содержал эту марку)
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPSyncCashSession::CompleteSession(PPID sessID)
{
	int    ok = -1, r;
	CSessGrouping csg;
	CSessionTbl::Rec sess_rec;
	PPCashNode cn_rec;
	THROW(CnObj.Search(NodeID, &cn_rec) > 0);
	THROW(csg.GetSess(sessID, &sess_rec) > 0);
	if(sess_rec.SuperSessID == 0) {
		int    do_convert_to_bills = 1;
		CSessTotal   total;
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		THROW_PP(!(eq_cfg.Flags & PPEquipConfig::fCloseSessTo10Level), PPERR_CSESSCOMPLLOCKED);
		{
			PPWaitStart();
			PPObjSecur::Exclusion ose(PPEXCLRT_CSESSWROFF);
			PPTransaction tra(1);
			THROW(tra);
			if(sess_rec.Incomplete == CSESSINCMPL_CHECKS) {
				PPID   super_id = 0;
				THROW(r = csg.AttachSessToSupersess(NodeID, sessID, &super_id, 0));
				if(r > 0)
					do_convert_to_bills = 0;
				else {
					THROW(csg.Grouping(sessID, &total, 0, 0));
				}
			}
			if(do_convert_to_bills)
				THROW(csg.ConvertToBills(sessID, cn_rec.LocID, 0, 1, 1, 0));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}
//
// PPAsyncCashSession
//
PPAsyncCashSession::PPAsyncCashSession(PPID n) : CSessGrouping(), NodeID(n), Flags(0), SinceDlsID(0),
	CnFlags(~0L), CnExtFlags(~0L), P_TmpCcTbl(0), P_TmpCclTbl(0), P_TmpCpTbl(0), P_LastSessList(0), P_GCfg(0), P_Dls(0)
{
}

PPAsyncCashSession::~PPAsyncCashSession()
{
	DestroyTables();
	delete P_LastSessList;
	delete P_GCfg;
	delete P_Dls;
}

int  PPAsyncCashSession::FinishImportSession(PPIDArray * pSessList) { return -1; }
void PPAsyncCashSession::CleanUpSession() {}
int  PPAsyncCashSession::IsReadyForExport() { return -1; }
int  PPAsyncCashSession::SetGoodsRestLoadFlag(int updOnly) { return -1; }
int  PPAsyncCashSession::InteractiveQuery() { return -1; }

const PPGoodsConfig & PPAsyncCashSession::GetGoodsCfg()
{
	if(!P_GCfg) {
		P_GCfg = new PPGoodsConfig;
		PPObjGoods::ReadConfig(P_GCfg);
	}
	return P_GCfg ? *P_GCfg : (PPSetErrorNoMem(), *static_cast<PPGoodsConfig *>(0));
}

PPID PPAsyncCashSession::GetLocation()
{
	PPObjCashNode cnobj(0);
	PPGenCashNode cndata;
	return (cnobj.Get(NodeID, &cndata) > 0) ? cndata.LocID : 0;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempCCheckTbl,     TempCCheck);
PP_CREATE_TEMP_FILE_PROC(CreateTempCCheckLineTbl, TempCCheckLine);
PP_CREATE_TEMP_FILE_PROC(CreateTempCCheckPaymTbl, CCheckPaym);

int PPAsyncCashSession::CreateTables()
{
	int    ok = 1;
	DestroyTables();
	THROW(P_TmpCcTbl  = CreateTempCCheckTbl());
	THROW(P_TmpCclTbl = CreateTempCCheckLineTbl());
	THROW(P_TmpCpTbl = CreateTempCCheckPaymTbl());
	CATCH
		DestroyTables();
		ok = 0;
	ENDCATCH
	return ok;
}

void PPAsyncCashSession::DestroyTables()
{
   	ZDELETE(P_TmpCcTbl);
   	ZDELETE(P_TmpCclTbl);
	ZDELETE(P_TmpCpTbl);
}

void PPAsyncCashSession::SetupTempCcLineRec(TempCCheckLineTbl::Rec * pRec, long ccID, long ccCode, LDATE dt, int div, PPID goodsID)
{
	SETIFZ(pRec, &P_TmpCclTbl->data);
	memzero(pRec, sizeof(*pRec));
	pRec->CheckID   = ccID;
	pRec->CheckCode = ccCode;
	pRec->Dt        = dt;
	pRec->DivID     = static_cast<int16>(div);
	pRec->GoodsID   = goodsID;
}

//void PPAsyncCashSession::SetTempCcLineValues(TempCCheckLineTbl::Rec * pRec, double qtty, double price, double discount, /*const char * pSerial*/ /*=0*/)
/*void PPAsyncCashSession::SetTempCcLineValues(TempCCheckLineTbl::Rec * pRec, double qtty, double price, double discount, const PPExtStrContainer * pLnExtStrings)
{
	SETIFZ(pRec, &P_TmpCclTbl->data);
	pRec->Quantity = qtty;
	pRec->Price = dbltointmny(price);
	pRec->Dscnt = discount;
	STRNSCPY(pRec->Serial, pSerial);
}*/

int PPAsyncCashSession::SetTempCcLineValuesAndInsert(TempCCheckLineTbl * pTbl, double qtty, double price, double discount, PPExtStrContainer * pLnExtStrings)
{
	int    ok = 1;
	assert(pTbl);
	if(pTbl) {
		TempCCheckLineTbl::Rec * p_rec = &pTbl->data;
		p_rec->Quantity = qtty;
		p_rec->Price = dbltointmny(price);
		p_rec->Dscnt = discount;
		//STRNSCPY(p_rec->Serial, pSerial);
		{
			SBuffer sbuf;
			size_t tl = 0;
			if(pLnExtStrings) {
				SSerializeContext sctx;
				THROW(pLnExtStrings->SerializeB(+1, sbuf, &sctx));
				tl = sbuf.GetAvailableSize();
			}
			p_rec->ExtTextSize = static_cast<long>(tl);
			if(tl) {
				assert(tl); // Ранее мы проверили длину текста на 0
				THROW(pTbl->writeLobData(pTbl->VT, sbuf.GetBufC(), tl));
			}
		}
		if(!pTbl->insertRec())
			ok = PPSetErrorDB();
		pTbl->destroyLobData(pTbl->VT);
	}
	CATCHZOK
	return ok;

}

int PPAsyncCashSession::IsCheckExistence(PPID cashID, long code, const LDATETIME * pDT, PPID * pTempReplaceID)
{
	int    ok = 0;
	PPID   temp_replace_id = 0;
	int    r = CC.Search(cashID, pDT->d, pDT->t);
	if(r > 0) {
		const double a = MONEYTOLDBL(CC.data.Amount);
		CSessionTbl::Rec cses_rec;
		if(a == 0.0 && CS.Search(CC.data.SessID, &cses_rec) > 0 && cses_rec.Temporary) {
			SString msg_buf, fmt_buf, cc_text_buf;
			CCheckCore::MakeCodeString(&CC.data, 0, cc_text_buf);
			// PPTXT_DUPNEWCCHECKCODEZAMT        "В существующей временной кассовой сессии обнаружен чек-дублер принимаемого чека с нулевой суммой (%s)"
			msg_buf.Printf(PPLoadTextS(PPTXT_DUPNEWCCHECKCODEZAMT, fmt_buf), cc_text_buf.cptr());
			PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			temp_replace_id = CC.data.ID;
			ok = 100;
		}
		else {
			ok = 1;
		}
	}
	else if(r < 0) {
		//
		// Специальный случай: некоторые кассовые модули могут возвращать в оперативном отчете (с незавершенной сессией)
		// не оконченный чек. На этот случай проверим наличие непосредственно предшествующего чека с тем же номером по
		// той же кассе и с временем, отличающимся от времени нового чека не более, чем на 10 минут.
		//
		CCheckTbl::Key2 k2;
		k2.PosNodeID = cashID;
		k2.Code = code;
		k2.Dt = pDT->d;
		k2.Tm = pDT->t;
		if(CC.search(2, &k2, spLt) && CC.data.PosNodeID == cashID && CC.data.Code == code) {
			LDATETIME ccdtm;
			ccdtm.Set(CC.data.Dt, CC.data.Tm);
			const long diffsec = diffdatetimesec(*pDT, ccdtm);
			if(checkirange(diffsec, 1L, (10L*60L))) {
				SString msg_buf, fmt_buf, cc_text_buf, time_buf;
				CCheckCore::MakeCodeString(&CC.data, 0, cc_text_buf);
				time_buf.Cat(*pDT, DATF_DMY, TIMF_HMS|TIMF_MSEC);
				// PPTXT_DUPNEWCCHECKCODE            "В существующей кассовой сессии обнаружен чек-дублер '%s' принимаемого, время нового чека = '%s'"
				msg_buf.Printf(PPLoadTextS(PPTXT_DUPNEWCCHECKCODE, fmt_buf), cc_text_buf.cptr(), time_buf.cptr());
				PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
				temp_replace_id = CC.data.ID;
				ok = 100;
			}
		}
	}
	ASSIGN_PTR(pTempReplaceID, temp_replace_id);
	return ok;
}

int PPAsyncCashSession::SearchTempCheckByTime(PPID cashID, const LDATETIME * pDT)
{
	CCheckTbl::Key1 k1;
	k1.Dt = pDT->d;
	k1.Tm = pDT->t;
	k1.PosNodeID = cashID;
	return SearchByKey(P_TmpCcTbl, 1, &k1, 0);
}

int PPAsyncCashSession::AddTempCheck(PPID * pID, long sessNumber, long flags,
	PPID cashID, PPID code, PPID user, PPID cardID, const LDATETIME & rDT, double amt, double dscnt/*, double addPaym, double extAmt*/)
{
	int    ok = 1;
	SString msg_buf;
	if(CConfig.Flags & CCFLG_DEBUG) {
		PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf.Cat(cashID).CatDiv('-', 1).Cat(code).CatDiv('-', 1).Cat(rDT), 0);
	}
	PPID   temp_replace_id = 0;
	const  int ice = IsCheckExistence(cashID, code, &rDT, &temp_replace_id);
	assert(ice != 100 || temp_replace_id);
	if(!ice || ice == 100) {
		const int is_temp_check_exists = BIN(SearchTempCheckByTime(cashID, &rDT) > 0);
		if(!is_temp_check_exists) {
			TempCCheckTbl::Rec new_rec;
			new_rec.SessID = sessNumber;
			new_rec.CashID = cashID;
			new_rec.Code   = code;
			new_rec.UserID = user;
			new_rec.SCardID = cardID;
			new_rec.Dt     = rDT.d;
			new_rec.Tm     = rDT.t;
			LDBLTOMONEY(amt,   new_rec.Amount);
			LDBLTOMONEY(dscnt, new_rec.Discount);
			if(ice == 100) {
				assert(temp_replace_id);
				flags |= CCHKF_TEMPREPLACE;
				new_rec.TempReplaceID = temp_replace_id;
			}
			new_rec.Flags |= flags;
			P_TmpCcTbl->copyBufFrom(&new_rec);
			THROW_DB(P_TmpCcTbl->insertRec(0, pID));
		}
		else {
			SString chk_buf;
			SString fmt_buf;
			chk_buf.Z().Cat(cashID).CatDiv('-', 1).Cat(code).CatDiv('-', 1).Cat(rDT);
			msg_buf.Printf(PPLoadTextS(PPTXT_DUPTEMPCCHECK, fmt_buf), chk_buf.cptr());
			PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
			ASSIGN_PTR(pID, 0);
			ok = -2;
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPAsyncCashSession::AddTempCheckAmounts(PPID checkID, double amt, double dis)
{
	int    ok = -1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	if(P_TmpCcTbl->searchForUpdate(0, &k0, spEq)) {
		TempCCheckTbl::Rec & r_rec = P_TmpCcTbl->data;
		const double a = MONEYTOLDBL(r_rec.Amount) + amt;
		const double d = MONEYTOLDBL(r_rec.Discount) + dis;
		LDBLTOMONEY(a, r_rec.Amount);
		LDBLTOMONEY(d, r_rec.Discount);
		ok = P_TmpCcTbl->updateRec() ? 1 : PPSetErrorDB(); // @sfu
	}
	return ok;
}

int PPAsyncCashSession::UpdateTempCheckFlags(long checkID, long flags)
{
	int    ok = -1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	if(P_TmpCcTbl->searchForUpdate(0, &k0, spEq)) {
		const long _f = P_TmpCcTbl->data.Flags;
		if(_f != (_f | flags)) {
			P_TmpCcTbl->data.Flags |= flags;
			ok = P_TmpCcTbl->updateRec() ? 1 : PPSetErrorDB(); // @sfu
		}
	}
	return ok;
}

int PPAsyncCashSession::AddTempCheckPaym(long checkID, int paymType, double amount, long scardID)
{
	int    ok = 1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	THROW_PP_S(P_TmpCcTbl->search(0, &k0, spEq), PPERR_TEMPCHECKFORPAYMNFOUND, checkID);
	{
		CCheckPaymTbl::Key0 k0;
		k0.CheckID = checkID;
		k0.RByCheck = MAXSHORT;
		int16  rbc = (P_TmpCpTbl->search(0, &k0, spLe) && P_TmpCpTbl->data.CheckID == checkID) ? P_TmpCpTbl->data.RByCheck : 0;
		CCheckPaymTbl::Rec cp_rec;
		cp_rec.CheckID = checkID;
		cp_rec.RByCheck = ++rbc;
		cp_rec.PaymType = paymType;
		cp_rec.Amount = dbltointmny(amount);
		cp_rec.SCardID = scardID;
		THROW_DB(P_TmpCpTbl->insertRecBuf(&cp_rec));
	}
	CATCHZOK
	return ok;
}

int PPAsyncCashSession::SetTempCheckAmounts(PPID checkID, double amt, double dis)
{
	int    ok = -1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	if(P_TmpCcTbl->searchForUpdate(0, &k0, spEq)) {
		LDBLTOMONEY(amt, P_TmpCcTbl->data.Amount);
		LDBLTOMONEY(dis, P_TmpCcTbl->data.Discount);
		ok = P_TmpCcTbl->updateRec() ? 1 : PPSetErrorDB(); // @sfu
	}
	return ok;
}

int PPAsyncCashSession::AddTempCheckSCardID(PPID checkID, PPID scardID)
{
	int    ok = -1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	if(P_TmpCcTbl->searchForUpdate(0, &k0, spEq)) {
		P_TmpCcTbl->data.SCardID = scardID;
		ok = P_TmpCcTbl->updateRec() ? 1 : PPSetErrorDB(); // @sfu
	}
	return ok;
}

int PPAsyncCashSession::AddTempCheckGiftCardID(PPID checkID, PPID giftCardID)
{
	int    ok = -1;
	CCheckTbl::Key0 k0;
	k0.ID = checkID;
	if(P_TmpCcTbl->searchForUpdate(0, &k0, spEq)) {
		P_TmpCcTbl->data.GiftCardID = giftCardID;
		ok = P_TmpCcTbl->updateRec() ? 1 : PPSetErrorDB(); // @sfu
	}
	return ok;
}

int PPAsyncCashSession::SearchTempCheckByCode(PPID cashID, PPID code, PPID sessNo)
{
	int    ok = -1;
	CCheckTbl::Key2 k;
	MEMSZERO(k);
	k.PosNodeID = cashID;
	k.Code   = code;
	if(P_TmpCcTbl->search(2, &k, spGt) && k.PosNodeID == cashID && k.Code == code) {
		DBRowId pos;
		int    count = 0;
		do {
			if(sessNo <= 0 || P_TmpCcTbl->data.SessID == sessNo)
				if(!(P_TmpCcTbl->data.Flags & CCHKF_ZCHECK) && (++count == 1))
					THROW_DB(P_TmpCcTbl->getPosition(&pos));
		} while(P_TmpCcTbl->search(2, &k, spNext) && k.PosNodeID == cashID && k.Code == code);
		if(count > 1) {
			SString msg_buf;
			CALLEXCEPT_PP_S(PPERR_DUPTEMPCHECK, msg_buf.Cat(cashID).CatDiv(':', 1).Cat(code));
		}
		else if(count == 1) {
			THROW_DB(P_TmpCcTbl->getDirect(-1, 0, pos));
			ok = 1;
		}
	}
	THROW(PPDbSearchError());
	CATCHZOK
	return ok;
}

int PPAsyncCashSession::GetLastSess(long cashNumber, LDATETIME dtm, long * pSessNumber, PPID * pSessID)
{
	int    ok = -1;
	LDATETIME last_sess_dtm;
	last_sess_dtm.Set(MAXDATE, ZEROTIME);
	long   sess_no = DEREFPTRORZ(pSessNumber);
	PPID   sess_id = DEREFPTRORZ(pSessID);
	LastSess * p_entry = 0;
	if(P_LastSessList)
		for(uint i = 0; P_LastSessList->enumItems(&i, (void **)&p_entry);)
			if(p_entry->CashNumber == cashNumber)
				if(!pSessNumber || !(*pSessNumber) || *pSessNumber == p_entry->SessNumber)
					if(diffdatetime(dtm, p_entry->Dtm, 0, 0) <= 0 && diffdatetime(last_sess_dtm, p_entry->Dtm, 0, 0) > 0) {
						last_sess_dtm = p_entry->Dtm;
						sess_no      = p_entry->SessNumber;
						sess_id      = p_entry->SessID;
						ok = 1;
					}
	ASSIGN_PTR(pSessNumber, sess_no);
	ASSIGN_PTR(pSessID, sess_id);
	return ok;
}

int PPAsyncCashSession::SetLastSess(long cashNumber, long sessNumber, PPID sessID, LDATETIME dtm)
{
	int    ok = -1;
	uint   i;
	LastSess entry, * p_entry = 0;
	if(P_LastSessList == 0) {
		THROW_MEM(P_LastSessList = new SArray(sizeof(LastSess)));
	}
	for(i = 0; P_LastSessList->enumItems(&i, (void **)&p_entry);)
		if(p_entry->CashNumber == cashNumber && p_entry->SessNumber == sessNumber) {
			p_entry->SessID = sessID;
			p_entry->Dtm = dtm;
			ok = 1;
		}
	if(ok < 0) {
		entry.CashNumber = cashNumber;
		entry.SessNumber = sessNumber;
		entry.SessID = sessID;
		entry.Dtm = dtm;
		THROW_SL(P_LastSessList->insert(&entry));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPAsyncCashSession::GetCashSessID(LDATETIME dtm, long cashNumber, long sessNumber, int forwardSess, int temporary, PPID * pSessID)
{
	int    ok = 1, r;
	PPID   sess_id = 0;
	const  int16 cfg_assc_period = CConfig.CSessFwAsscPeriod;
	const  int16 assc_period = (cfg_assc_period > 0) ? cfg_assc_period : (24*60);
	if(forwardSess) {
		// Сессия с форвардными номерами смен. То есть, в чеке номер смены
		// не прописывается, но в конце каждой смены присутствует чек
		// идентифицирующий смену, которой принадлежали чеки до этого чека
		// и следующие за последним таким же чеком.
		if(sessNumber) {
			// Засекли чек Z-отчета

			//
			// Проверяем нет-ли уже созданной сессии по найденному чеку
			// Z-отчета (такое бывает когда при уже созданной сессии удаляют
			// такой чек).
			//
			THROW(r = CS.SearchByNumber(&sess_id, NodeID, cashNumber, sessNumber, dtm.d));
			if(r < 0)
				THROW(CS.CreateSess(&(sess_id = 0), NodeID, cashNumber, sessNumber, dtm, 0));
			THROW(SetLastSess(cashNumber, sessNumber, sess_id, dtm));
		}
		else {
			// Ищем впереди идущий номер Z-отчета
			long fw_sess_number = 0;
			CCheckTbl::Rec chk_rec;
			if(GetLastSess(cashNumber, dtm, &fw_sess_number, &sess_id) <= 0) {
				if(CC.SearchForwardZCheck(cashNumber, dtm.d, dtm.t, &chk_rec) > 0) {
					long diff_date, diff_time;
					diff_time = diffdatetime(chk_rec.Dt, chk_rec.Tm, dtm.d, dtm.t, 2, &diff_date);
					if(labs(diff_date * (60L*24L) + diff_time) < assc_period) {
						fw_sess_number = chk_rec.Code;
						if(CS.SearchByNumber(&sess_id, NodeID, cashNumber, fw_sess_number, dtm.d) <= 0)
							THROW(CS.CreateSess(&sess_id, NodeID, cashNumber, fw_sess_number, dtm, 0));
						THROW(SetLastSess(cashNumber, fw_sess_number, sess_id, dtm));
					}
					else {
						sess_id = 0;
						ok = -1;
					}
   		        }
	   		    else {
					sess_id = 0;
			   		ok = -1;
				}
			}
		}
	}
	else {
		// Ищем смену с номером sessNumber
		if(GetLastSess(cashNumber, dtm, &sessNumber, &sess_id) <= 0) {
			if(CS.SearchByNumber(&sess_id, NodeID, cashNumber, sessNumber, dtm.d) > 0) {
				if(!temporary && CS.data.Temporary)
					THROW(CS.ResetTempSessTag(sess_id, 0));
			}
			else
				THROW(CS.CreateSess(&sess_id, NodeID, cashNumber, sessNumber, dtm, temporary));
			THROW(SetLastSess(cashNumber, sessNumber, sess_id, dtm));
		}
	}
	CATCH
		ok = 0;
		sess_id = 0;
	ENDCATCH
	ASSIGN_PTR(pSessID, sess_id);
	return ok;
}

int PPAsyncCashSession::CalcSessionTotal(PPID sessID, CSessTotal * pTotal)
{
	CCheckTbl::Key3 k;
	MEMSZERO(k);
	k.SessID = sessID;
	BExtQuery * p_q = new BExtQuery(&CC, 3);
	p_q->selectAll().where(CC.SessID == sessID);
	pTotal->SessID = sessID;
	for(p_q->initIteration(false, &k, spGe); p_q->nextIteration() > 0;) {
		const CCheckTbl::Rec & r_rec = CC.data;
		if(!(r_rec.Flags & CCHKF_ZCHECK)) {
			pTotal->CheckCount++;
			pTotal->Amount   += MONEYTOLDBL(r_rec.Amount);
			pTotal->Discount += MONEYTOLDBL(r_rec.Discount);
		}
	}
	BExtQuery::ZDelete(&p_q);
	p_q = new BExtQuery(&GL, 0);
	p_q->selectAll().where(GL.SessID == sessID);
	for(p_q->initIteration(false, &k, spGe); p_q->nextIteration() > 0;) {
		pTotal->AggrAmount += GL.data.Sum;
		if(GL.data.Qtty != 0.0)
			pTotal->AggrRest += (GL.data.Rest * GL.data.Sum) / GL.data.Qtty;
	}
	delete p_q;
	return 1;
}

struct CclAssocItem { // @flat
	PPID   TempChkID;
	PPID   ChkID;
	uint16 RByCheck;
	uint16 Reserve;  // @alignment
	long   Flags;
};

struct CclExtTextItem {
	PPID   CheckID;
	StrAssocArray LnTextList;
};

int PPAsyncCashSession::FlashTempCcLines(const SVector * pList, LAssocArray * pHasExLineList)
{
	int    ok = 1;
	if(pList && pList->getCount()) {
		const  int use_ext = BIN(CConfig.Flags & CCFLG_USECCHECKLINEEXT);
		SString wait_msg;
		SString temp_buf;
		IterCounter cntr;
		PPInitIterCounter(cntr, P_TmpCclTbl);
		PPLoadText(PPTXT_FLASHTEMPCCLINES, wait_msg);
		const int dbtidx = 2;
		SBuffer sbuf;
		PPExtStrContainer ext_strings;
		BExtInsert bei(&CC.Lines);
		TempCCheckLineTbl * t = P_TmpCclTbl;
		BExtQuery q(t, dbtidx, 64);
		TempCCheckLineTbl::Key2 k2;
		TSVector <CCheckLineExtTbl::Rec> ccext_items;
		TSCollection <CclExtTextItem> ccln_extt_list;

		MEMSZERO(k2);
		q.selectAll();
		PPID   last_temp_chk_id = 0;
		uint   last_chk_pos = 0;
		for(q.initIteration(false, &k2, spFirst); q.nextIteration() > 0;) {
			uint   p = 0;
			const  TempCCheckLineTbl::Rec & r_rec = t->data;
			PPID   temp_chk_id = r_rec.CheckID;
			CCheckLineExtTbl::Rec ext_rec;
			DBRowId rec_pos;
			q.getRecPosition(&rec_pos);
			ext_strings.Z();
			sbuf.Z(); // @v11.0.2 @fix
			if(r_rec.ExtTextSize > 0) { // @v10.7.3
				THROW_DB(t->getDirect(dbtidx, 0, rec_pos)); // @v11.0.2
				t->readLobData(t->VT, sbuf);
				t->destroyLobData(t->VT);
				const size_t actual_size = sbuf.GetAvailableSize();
				if(actual_size == static_cast<size_t>(r_rec.ExtTextSize)) {
					SSerializeContext sctx;
					if(!ext_strings.SerializeB(-1, sbuf, &sctx)) {
						ext_strings.Z();
					}
				}
			}
			if(last_temp_chk_id && temp_chk_id == last_temp_chk_id)
				p = last_chk_pos;
			else if(pList->bsearch(&temp_chk_id, &p, CMPF_LONG)) {
				last_chk_pos = p;
				last_temp_chk_id = temp_chk_id;
			}
			else
				continue;
			CclAssocItem * p_item = static_cast<CclAssocItem *>(pList->at(p));
			CCheckLineTbl::Rec  line_rec;
			line_rec.CheckID  = p_item->ChkID;
			line_rec.RByCheck = ++p_item->RByCheck;
			line_rec.DivID    = r_rec.DivID;
			line_rec.GoodsID  = r_rec.GoodsID;
			line_rec.Quantity = r_rec.Quantity;
			line_rec.Price    = r_rec.Price;
			line_rec.Dscnt    = r_rec.Dscnt;
			//line_rec.Discount = r_rec.Discount;
			THROW_DB(bei.insert(&line_rec));
			{
				CclExtTextItem * p_ccln_extt_item = 0;
				const int lnextf_list[] = { CCheckPacket::lnextSerial, CCheckPacket::lnextEgaisMark, CCheckPacket::lnextChZnMark, CCheckPacket::lnextRemoteProcessingTa };
				for(uint lnefidx = 0; lnefidx < SIZEOFARRAY(lnextf_list); lnefidx++) {
					ext_strings.GetExtStrData(lnextf_list[lnefidx], temp_buf);
					if(temp_buf.NotEmptyS()) {
						if(!p_ccln_extt_item) {
							uint   cclnextt_pos = 0;
							if(ccln_extt_list.lsearch(&p_item->ChkID, &cclnextt_pos, CMPF_LONG))
								p_ccln_extt_item = ccln_extt_list.at(cclnextt_pos);
							else {
								THROW_SL(p_ccln_extt_item = ccln_extt_list.CreateNewItem());
								p_ccln_extt_item->CheckID = p_item->ChkID;
							}
						}
						CCheckPacket::Helper_SetLineTextExt(line_rec.RByCheck, lnextf_list[lnefidx], p_ccln_extt_item->LnTextList, temp_buf);
					}
				}
			}
			PPWaitPercent(cntr.Increment(), wait_msg);
		}
		THROW_DB(bei.flash());
		if(ccln_extt_list.getCount()) {
			SString ln_exttext_buf;
			SString line_buf;
			SString temp_buf;
			for(uint cclnextt_idx = 0; cclnextt_idx < ccln_extt_list.getCount(); cclnextt_idx++) {
				const CclExtTextItem * p_ccln_extt_item = ccln_extt_list.at(cclnextt_idx);
				if(p_ccln_extt_item && p_ccln_extt_item->CheckID) {
					ln_exttext_buf.Z();
					if(p_ccln_extt_item->LnTextList.getCount()) {
						for(uint i = 0; i < p_ccln_extt_item->LnTextList.getCount(); i++) {
							StrAssocArray::Item item = p_ccln_extt_item->LnTextList.Get(i);
							PPExtStringStorage ess;
							line_buf = item.Txt;
							int    fld_id = 0;
							for(uint p = 0; ess.Enum(line_buf, &p, &fld_id, temp_buf) > 0;) {
								if(temp_buf.NotEmptyS() && fld_id > 0 && fld_id < 100)
									PPPutExtStrData(item.Id * 100 + fld_id, ln_exttext_buf, temp_buf);
							}
						}
						if(ln_exttext_buf.NotEmptyS()) {
							THROW(PPRef->UtrC.SetText(TextRefIdent(PPOBJ_CCHECK, p_ccln_extt_item->CheckID, PPTRPROP_CC_LNEXT), ln_exttext_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
						}
					}
				}
			}
		}
		if(ccext_items.getCount() > 0) {
			uint ext_count = ccext_items.getCount();
			BExtInsert bei_ext(CC.P_LnExt, 0);
			for(uint i = 0; i < ext_count; i++) {
				CCheckLineExtTbl::Rec ext_rec = ccext_items.at(i);
				THROW_DB(bei_ext.insert(&ext_rec));
			}
			THROW_DB(bei_ext.flash());
		}
	}
	CATCHZOK
	return ok;
}

struct TotalLogCSessEntry { // @flat
	TotalLogCSessEntry() : SessID(0), CcCount(0), CcAmount(0.0)
	{
	}
	PPID   SessID;
	uint   CcCount;
	double CcAmount;
};

int PPAsyncCashSession::ConvertTempSession(int forwardSess, PPIDArray & rSessList, void * pTotalLogData)
{
	P_LastSessList = 0;

	int    ok = 1;
	const  int use_ext = BIN(CConfig.Flags & CCFLG_USECCHECKEXT);
	TSVector <TotalLogCSessEntry> * p_total_log_list = static_cast<TSVector <TotalLogCSessEntry> *>(pTotalLogData); // @v10.0.05
	uint   i;
	CcAmountList cp_list;
	PPObjSCardSeries scs_obj;
	THROW(GetLocation());
	{
		PPTransaction tra(1);
		THROW(tra);
		{
			PPID   kid = 0;
			SString wait_msg;
			IterCounter cntr;
			PPInitIterCounter(cntr, P_TmpCcTbl);
			PPLoadText(PPTXT_FLASHTEMPCCHECKS, wait_msg);
			SVector ccl_assoc(sizeof(CclAssocItem));
			BExtQuery ccq(P_TmpCcTbl, 0, 64);
			ccq.selectAll();
			for(ccq.initIteration(true, &kid, spLast); ccq.nextIteration() > 0;) {
				long   cc_flags_after_turn = 0;
				PPID   tmp_chk_id = 0;
				PPID   check_id = 0;
				CclAssocItem ccla_item;
				TempCCheckTbl::Rec temp_chk_rec;
				CCheckTbl::Rec chk_rec;
				P_TmpCcTbl->copyBufTo(&temp_chk_rec);
				LDATETIME dtm;
				PPID   sess_id = 0;
				TotalLogCSessEntry * p_tl_entry = 0;
				CCheckExtTbl::Rec ext_rec;
				memcpy(&chk_rec, &temp_chk_rec, sizeof(chk_rec));
				dtm.Set(chk_rec.Dt, chk_rec.Tm);
				THROW(GetCashSessID(dtm, chk_rec.PosNodeID, chk_rec.SessID, forwardSess, BIN(chk_rec.Flags & CCHKF_TEMPSESS), &sess_id));
				if(sess_id) {
					THROW(rSessList.addUnique(sess_id));
					// @v10.0.05 {
					if(p_total_log_list) {
						uint   tl_pos = 0;
						if(p_total_log_list->lsearch(&sess_id, &tl_pos, CMPF_LONG))
							p_tl_entry = &p_total_log_list->at(tl_pos);
						else {
							TotalLogCSessEntry new_tl_entry;
							new_tl_entry.SessID = sess_id;
							THROW_SL(p_total_log_list->insert(&new_tl_entry));
							p_tl_entry = &p_total_log_list->at(p_total_log_list->getCount()-1);
						}
					}
					// } @v10.0.05
				}
				chk_rec.SessID = sess_id;
				if(SurveyPeriod.low == 0 || dtm.d < SurveyPeriod.low)
					SurveyPeriod.low = dtm.d;
				if(SurveyPeriod.upp == 0 || dtm.d > SurveyPeriod.upp)
					SurveyPeriod.upp = dtm.d;
				tmp_chk_id = chk_rec.ID;
				chk_rec.ID = 0;
				chk_rec.Flags |= CCHKF_NOTUSED;
				chk_rec.Flags &= ~CCHKF_TEMPSESS;
				if(temp_chk_rec.Flags & CCHKF_TEMPREPLACE) {
					assert(temp_chk_rec.TempReplaceID);
					if(temp_chk_rec.TempReplaceID) {
						THROW(CC.RemovePacket(temp_chk_rec.TempReplaceID, 0));
					}
					chk_rec.Flags &= ~CCHKF_TEMPREPLACE;
				}
				cc_flags_after_turn = chk_rec.Flags;
				THROW(CC.Add(&check_id, &chk_rec, 0)); // @use BExtInsert ?
				// @v10.0.05 {
				if(p_tl_entry) {
					p_tl_entry->CcCount++;
					p_tl_entry->CcAmount += MONEYTOLDBL(chk_rec.Amount);
				}
				// } @v10.0.05
				{
					THROW(CCheckCore::Helper_GetPaymList(P_TmpCpTbl, tmp_chk_id, cp_list));
					const uint plc = cp_list.getCount();
					if(plc) {
						const int do_turn_sc_op = BIN(!(chk_rec.Flags & (CCHKF_JUNK|CCHKF_SKIP|CCHKF_SUSPENDED)));
						PPSCardSeries scs_rec;
						int16  rbc = 0;
						for(uint i = 0; i < plc; i++) {
							const CcAmountEntry & r_entry = cp_list.at(i);
							SCardTbl::Rec sc_rec;
							CCheckPaymTbl::Rec cp_rec;
							cp_rec.CheckID = check_id;
							cp_rec.RByCheck = ++rbc;
							cp_rec.PaymType = (int16)r_entry.Type;
							cp_rec.Amount = dbltointmny(r_entry.Amount);
							cp_rec.SCardID = r_entry.AddedID;
							cp_rec.CurID = r_entry.CurID;
							cp_rec.CurAmount = dbltointmny(r_entry.CurAmount);
							THROW_DB(CC.PaymT.insertRecBuf(&cp_rec));
							if(cp_rec.SCardID && do_turn_sc_op && CC.Cards.Search(cp_rec.SCardID, &sc_rec) > 0) {
								const int scst = (scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0) ? scs_rec.GetType() : scstUnkn;
								if(oneof2(scst, scstCredit, scstBonus)) {
									SCardOpTbl::Rec scop_rec;
									scop_rec.SCardID = sc_rec.ID;
									scop_rec.Dt      = chk_rec.Dt;
									scop_rec.Tm      = chk_rec.Tm;
									scop_rec.LinkObjType = PPOBJ_CCHECK;
									scop_rec.LinkObjID   = chk_rec.ID;
									scop_rec.UserID  = chk_rec.UserID;
									scop_rec.Amount  = -r_entry.Amount;
									THROW(CC.Cards.PutOpRec(&scop_rec, 0, 0));
								}
							}
						}
						THROW(CC.UpdateFlags(check_id, cc_flags_after_turn | CCHKF_PAYMLIST, 0));
					}
				}
				ccla_item.TempChkID = tmp_chk_id;
				ccla_item.ChkID     = check_id;
				ccla_item.RByCheck  = 0;
				ccla_item.Flags     = chk_rec.Flags;
				THROW_SL(ccl_assoc.insert(&ccla_item));
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
			LAssocArray extlines_list;
			ccl_assoc.sort(CMPF_LONG); // @v9.8.4 Перенесено из тела FlashTempCcLines ради const-аргумента
			THROW(FlashTempCcLines(&ccl_assoc, &extlines_list));
			{ // Для чеков, у которых есть строки с расширениями нужно установить соответствующий флаг
				uint count = extlines_list.getCount();
				for(uint i = 0; i < count; i++) {
					LAssoc extl_item = extlines_list.at(i);
					THROW(CC.UpdateFlags(extl_item.Key, extl_item.Val, 0));
				}
			}
		}
		if(forwardSess && P_LastSessList) {
			LastSess * p_entry = 0;
			for(i = 0; P_LastSessList->enumItems(&i, (void **)&p_entry);) {
				CCheckTbl::Key1 k;
				BExtQuery q(&CC, 1);
				q.selectAll().where(CC.PosNodeID == p_entry->CashNumber);
				k.Dt = p_entry->Dtm.d;
				k.Tm = p_entry->Dtm.t;
				k.PosNodeID = MAXLONG;
				for(q.initIteration(true, &k, spLt); q.nextIteration() > 0;) {
					if(CC.data.Flags & CCHKF_ZCHECK && CC.data.SessID != p_entry->SessID)
						break;
					if(CC.data.SessID == 0) {
						k.Dt = CC.data.Dt;
						k.Tm = CC.data.Tm;
						k.PosNodeID = CC.data.PosNodeID;
						if(CC.searchForUpdate(1, &k, spEq)) {
							CC.data.SessID = p_entry->SessID;
							THROW_DB(CC.updateRec()); // @sfu
						}
					}
				}
			}
		}
		for(i = 0; i < rSessList.getCount(); i++) {
			const  PPID sess_id = rSessList.at(i);
			THROW(CC.UpdateSCardOpsBySess(sess_id, 0));
			THROW(CS.SetSessIncompletness(sess_id, CSESSINCMPL_CHECKS, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ZDELETE(P_LastSessList);
	return ok;
}
//
//
//
int PPAsyncCashSession::LogExportingGoodsItem(const AsyncCashGoodsInfo * pInfo)
{
	int    ok = 1;
	SString msg_buf, added_info;
	if(!LogFmt_ExpGoods)
		PPLoadText(PPTXT_LOG_ACNEXP_GOODS, LogFmt_ExpGoods);
	if(pInfo)
		added_info.Cat(pInfo->BarCode).CatDiv('-', 1).Cat(pInfo->Name).CatDiv('-', 1).Cat(pInfo->Price, SFMT_MONEY);
	else
		added_info.Space();
	msg_buf.Printf(LogFmt_ExpGoods, added_info.cptr());
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
	return ok;
}

int PPAsyncCashSession::LogDebug(const char * pMsg)
{
	return (CConfig.Flags & CCFLG_DEBUG) ? PPLogMessage(PPFILNAM_DEBUG_LOG, pMsg, LOGMSGF_TIME|LOGMSGF_USER) : -1;
}

int PPAsyncCashSession::OpenSession(int updOnly, PPID sinceDlsID)
{
	int    ok = 1;
	const  long lmf = LOGMSGF_TIME|LOGMSGF_USER;
	SString fmt, msg_buf;
	PPAsyncCashNode acn_rec;
	GetNodeData(&acn_rec);
	PPLogMessage(PPFILNAM_INFO_LOG, msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ACNEXP_START, fmt), acn_rec.Name), lmf);
	int    ready = IsReadyForExport();
	if(!ready) {
		if(CC.GetEqCfg().Flags & PPEquipConfig::fIgnAcsReadyTags) {
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ACNEXP_NREADYTAGIGNORED, fmt), acn_rec.Name), lmf);
			ready = 1;
		}
		else {
			ok = PPSetError(PPERR_ASCASHNREADYFOREXP, acn_rec.Name);
			PPLogMessage(PPFILNAM_INFO_LOG, 0, LOGMSGF_LASTERR|lmf);
		}
	}
	if(ready) {
		SinceDlsID = updOnly ? sinceDlsID : 0;
		SetGoodsRestLoadFlag(updOnly);
		ok = ExportData(updOnly);
		if(ok) {
			DS.LogAction(PPACN_EXPCASHSESS, PPOBJ_CASHNODE, NodeID, updOnly, 1);
			//
			// Пустой файл, сигнализирующий о том, что экспорт данных завершен (ACSEXP.)
			//
			SString path;
			PPGetFilePath(PPPATH_OUT, PPFILNAM_SIG_ACSOPENED, path);
			createEmptyFile(path);
			DistributeFile_(path, 0/*pEndFileName*/, dfactCopy, 0, 0);
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf.Printf(PPLoadTextS(PPTXT_LOG_ACNEXP_FINISH, fmt), acn_rec.Name), lmf);
		}
	}
	return ok;
}

int PPAsyncCashSession::CloseSession(int asTempSess, DateRange * pPrd /*=0*/)
{
	SurveyPeriod.Z();
	int    ok = 1;
	int    r;
	int    i;
	int    sess_count = 0;
	int    is_forward_sess = 0;
	SString msg_debug_fmt, msg_buf;
	PPIDArray sess_list, super_sess_list;
	PPAsyncCashNode acn;
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	PPLoadText(PPTXT_DIAG_ASYNCCLOSESESS, msg_debug_fmt);
#define LOG_DEBUG(func) LogDebug(msg_buf.Printf(msg_debug_fmt, #func))
	THROW(GetNodeData(&acn));
	if(acn.CashType != PPCMT_PAPYRUS)
		PPWaitStart();
	const  PPID loc_id = acn.LocID;
	THROW_PP_S(loc_id, PPERR_UNDEFCASHNODELOC, acn.Name);
	SETFLAG(Flags, PPACSF_TEMPSESS, asTempSess);
	//
	// Находим и закрываем незавершенные сессии
	//
	if(!(r_eq_cfg.Flags & PPEquipConfig::fCloseSessTo10Level)) {
		PPWaitMsg(PPSTR_TEXT, PPTXT_ACSCLS_CLOSEINCMPL, 0);
		if(CS.GetIncompleteSessList(CSESSINCMPL_CHECKS, NodeID, &sess_list) > 0) {
			THROW(GroupingSessList(NodeID, &sess_list, &super_sess_list, 0, 1));
			THROW(ConvertSessListToBills(&super_sess_list, loc_id, 1));
		}
		sess_list.freeAll();
		super_sess_list.freeAll();
		if(CS.GetIncompleteSessList(CSESSINCMPL_GLINES, NodeID, &super_sess_list) > 0)
			THROW(ConvertSessListToBills(&super_sess_list, loc_id, 1));
	}
	//
	// Импортируем кассовые сессии
	//
	sess_list.freeAll();
	super_sess_list.freeAll();
	THROW(GetSessionData(&sess_count, &is_forward_sess, pPrd));
	LOG_DEBUG(GetSessionData);
	if(sess_count) {
		TSVector <TotalLogCSessEntry> total_log_data; // @v10.0.05
		PPWaitMsg(PPSTR_TEXT, PPTXT_ACSCLS_IMPORT, 0);
		for(i = 0; i < sess_count; i++) {
			THROW(r = ImportSession(i));
			LOG_DEBUG(ImportSession);
			if(r > 0) {
				THROW(ConvertTempSession(is_forward_sess, sess_list, &total_log_data));
				LOG_DEBUG(ConvertTempSession);
			}
		}
		// @v10.0.05 {
		if(total_log_data.getCount()) {
			for(uint j = 0; j < total_log_data.getCount(); j++) {
				const TotalLogCSessEntry & r_total_log_entry = total_log_data.at(j);
				msg_buf.Z().Cat("Imported CSession").CatDiv(':', 2).CatEq("CSessID", r_total_log_entry.SessID).Space().CatEq("CcCount", r_total_log_entry.CcCount).Space().
					CatEq("CcAmount", r_total_log_entry.CcAmount, MKSFMTD_020);
				PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		// } @v10.0.05
		THROW(FinishImportSession(&sess_list));
		LOG_DEBUG(FinishImportSession);
		PPWaitMsg(PPSTR_TEXT, PPTXT_ACSCLS_TOBILLS, 0);
		{
			Reference * p_ref = PPRef;
			//
			// Блок перестроен так, чтобы исполняться в общей транзакции {
			//
			PPIDArray temp_sess_list;
			PPGenCashNode gcn;
			PPObjCashNode cnobj;
			PPTransaction tra(1);
			THROW(tra);
			THROW(GroupingSessList(NodeID, &sess_list, &super_sess_list, &temp_sess_list, 0));
			LOG_DEBUG(GroupingSessList);
			for(i = 0; i < super_sess_list.getCountI(); i++)
				DS.LogAction(PPACN_CSESSCLOSED, PPOBJ_CSESSION, super_sess_list.at(i), 0, 0);
			if(!(r_eq_cfg.Flags & PPEquipConfig::fCloseSessTo10Level)) {
				THROW(ConvertSessListToBills(&super_sess_list, loc_id, 0));
				LOG_DEBUG(ConvertSessListToBills);
				if(r_eq_cfg.OpOnTempSess) {
					THROW(cnobj.Get(NodeID, &gcn, 0) > 0);
					THROW(r = ConvertTempSessToBills(&temp_sess_list, loc_id, &gcn.CurRestBillID, 0));
					LOG_DEBUG(ConvertSessListToBills_TempSessList);
					if(r > 0) {
						PPCashNode temp_rec;
						Reference2Tbl::Key0 k0;
						k0.ObjType = PPOBJ_CASHNODE;
						k0.ObjID = NodeID;
						if(SearchByKey_ForUpdate(p_ref, 0, &k0, &temp_rec) > 0) {
							temp_rec.CurRestBillID = gcn.CurRestBillID;
							THROW_DB(p_ref->updateRecBuf(&temp_rec)); // @sfu
						}
					}
				}
			}
			THROW(tra.Commit());
		}
		if(r_eq_cfg.Flags & PPEquipConfig::fValidateChecksOnSessClose && sess_list.getCount()) {
			ObjIdListFilt _ses_list;
			ObjIdListFilt _chk_list;
			CCheckCore::ValidateCheckParam vcp(0.02);
			_ses_list.Set(&sess_list);
			PPLoadText(PPTXT_ACSCLS_TESTCHECKS, msg_buf);
			PPWaitMsg(msg_buf);
			if(CC.LoadChecksByList(&_ses_list, 0, &_chk_list, 0) > 0 && _chk_list.IsExists()) {
				const PPIDArray & r_chk_list = _chk_list.Get();
				PPLogger logger;
				for(i = 0; i < r_chk_list.getCountI(); i++) {
					CC.ValidateCheck(r_chk_list.get(i), vcp, logger);
					PPWaitPercent(i+1, r_chk_list.getCount(), msg_buf);
				}
				logger.Save(PPFILNAM_ERR_LOG, 0);
				LOG_DEBUG(ValidateChecks);
			}
		}
	}
	CATCHZOK
	if(!ok)
		PPSaveErrContext();
	DestroyTables();
	DBRemoveTempFiles();
	PPWaitStop();
	if(!ok) {
		PPRestoreErrContext();
		LOG_DEBUG(FAIL);
	}
	else {
		LOG_DEBUG(OK);
	}
	return ok;

#undef LOG_DEBUG
}

int PPAsyncCashSession::GetExpPathSet(StringSet * pSs)
{
	int    ok = 1;
	PPAsyncCashNode acn;
	PPObjCashNode cnobj(0);
	pSs->Z();
	if(cnobj.GetAsync(NodeID, &acn) > 0) {
		if(acn.ExpPaths.NotEmpty()) {
			StringSet ss(';', acn.ExpPaths);
			*pSs = ss;
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	return ok;
}

int PPAsyncCashSession::CheckCnFlag(long f)
{
	PPAsyncCashNode cn_data;
	return BIN((CnFlags != ~0L || GetNodeData(&cn_data) > 0) && (CnFlags & f));
}

int FASTCALL PPAsyncCashSession::CheckCnExtFlag(long f)
{
	PPAsyncCashNode cn_data;
	return BIN((CnExtFlags != ~0L || GetNodeData(&cn_data) > 0) && (CnExtFlags & f));
}

int PPAsyncCashSession::GetNodeData(PPAsyncCashNode * pData)
{
	PPObjCashNode cnobj;
	const int r = cnobj.GetAsync(NodeID, pData);
	if(r > 0) {
		CnFlags = pData->Flags;
		CnExtFlags = pData->ExtFlags;
	}
	return r;
}

void FASTCALL PPAsyncCashSession::AdjustSCardCode(char * pCode)
{
	if(CheckCnFlag(CASHF_EXPCHECKD) && sstrlen(pCode) == 12)
		AddBarcodeCheckDigit(pCode);
}

int FASTCALL PPAsyncCashSession::AddCheckDigToBarcode(char * pCode)
{
	const size_t len = sstrlen(pCode);
	const PPGoodsConfig & gcfg = GetGoodsCfg();
	if(len > 3 && CheckCnFlag(CASHF_EXPCHECKD) && !(gcfg.Flags & GCF_BCCHKDIG) && !gcfg.IsWghtPrefix(pCode)) {
		AddBarcodeCheckDigit(pCode);
		return 1;
	}
	else
		return -1;
}

int PPAsyncCashSession::DistributeFile_(const char * pFileName, const char * pEndFileName, int action, const char * pSubDir /*=0*/, const char * pEmailSubj /*=0*/)
{
	const char * p_ftp_flag = "ftp:";
	StringSet ss(';', 0);
	int    ok = GetExpPathSet(&ss);
	if(ok > 0) {
		const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
		int    ftp_connected = 0;
		SString dest_file_name(pFileName);
		SString dest_nam; // @v10.8.4 Если !Empty то имя конечного файла заменяется на это
		SString dest_ext; // @v10.8.4 Если !Empty то расширение конечного файла заменяется на это
		SString path, temp_file_name;
		PPInternetAccount acct;
		PPInternetAccount mac_rec;
		PPObjInternetAccount obj_acct;
		WinInetFTP ftp;
		// @v10.8.4 {
		if(!isempty(pEndFileName)) {
			SFsPath efn_ps(pEndFileName);
			dest_nam = efn_ps.Nam;
			dest_ext = efn_ps.Ext;
		}
		// } @v10.8.4
		if(r_eq_cfg.FtpAcctID) {
			THROW(obj_acct.Get(r_eq_cfg.FtpAcctID, &acct));
		}
		{
			PPAlbatrossConfig alb_cfg;
			if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0 && alb_cfg.Hdr.MailAccID)
				THROW_PP(obj_acct.Get(alb_cfg.Hdr.MailAccID, &mac_rec) > 0, PPERR_UNDEFMAILACC);
		}
		ok = -1;
		for(uint i = 0; ss.get(&i, path);) {
			if(!isempty(pSubDir)) {
				SFsPath sp(path);
				sp.Dir.RmvLastSlash();
				if(!oneof2(pSubDir[0], '\\', '/'))
					sp.Dir.SetLastSlash();
				sp.Dir.Cat(pSubDir);
				sp.Merge(path);
			}
			/*
				dfactCopy   = 0, // скопировать файл pFileName в каждый из каталогов экспорта
				dfactDelete = 1, // удалить файлы с именем, заданным параметром pFileName
				dfactCheckExistence = 2, // проверить наличие файла с именем, заданным параметром pFileName
				dfactCheckDestPaths = 3  // проверить доступность каталогов назначения на запись
			*/
			if(path.HasPrefixIAscii(p_ftp_flag)) {
				SString ftp_path, file_name;
				SFsPath sp;
				path.ShiftLeft(sstrlen(p_ftp_flag));
				path.ShiftLeftChr('\\').ShiftLeftChr('\\').ShiftLeftChr('/').ShiftLeftChr('/');
				ftp_path.CatCharN('\\', 2).Cat(path);
				if(!ftp_connected) {
					THROW(ftp.Init());
					THROW(ftp.Connect(&acct));
					ftp_connected = 1;
				}
				sp.Split(pFileName);
				sp.Merge(0, SFsPath::fDrv|SFsPath::fDir, file_name);
				if(action == dfactCheckDestPaths) {
					SString local_path;
					PPGetPath(PPPATH_OUT, local_path);
					FILE * f_temp = fopen(MakeTempFileName(local_path, 0, 0, 0, temp_file_name), "w");
					sp.Split(local_path);
					ftp_path.SetLastSlash().Cat(sp.Nam).Dot().Cat(sp.Ext);
					if(f_temp) {
						SFile::ZClose(&f_temp);
						if(ftp.SafePut(temp_file_name, ftp_path, 0, 0, 0) > 0) {
							ftp.SafeDelete(ftp_path, 0);
							ok = 1;
						}
						else {
							ok = 0;
							break;
						}
						SFile::Remove(temp_file_name);
						ok = 1;
					}
					else {
						ok = PPSetError(PPERR_DIRNOTWACCS, local_path);
						break;
					}
				}
				else {
					ftp_path.SetLastSlash().Cat(file_name);
					if(ftp.Exists(ftp_path)) {
						if(action == dfactDelete) { // Remove file
							ftp.SafeDelete(ftp_path, 0);
							ok = 1;
						}
						else if(action == dfactCheckExistence) // Check existence
							ok = 1;
					}
					else if(action == dfactDelete)
						ok = 1; // Если файл не найден, то полагаем, что результат удаления положительный
					if(action == dfactCopy) { // Copy file
						THROW(ftp.SafePut(pFileName, ftp_path, 0, 0, 0));
						ok = 1;
					}
				}
			}
			else if(path.HasChr('@')) {
				if(action == dfactCopy && pEmailSubj)
					ok = SendMailWithAttach(pEmailSubj, pFileName, 0, path, mac_rec.ID);
				else
					ok = 1;
			}
			else {
				if(action == dfactCheckDestPaths) {
					//
					// Проверка на возможность создания файла в заданном каталоге
					//
					FILE * f_temp = fopen(MakeTempFileName(path, 0, 0, 0, temp_file_name), "w");
					if(f_temp) {
						SFile::ZClose(&f_temp);
						SFile::Remove(temp_file_name);
						ok = 1;
					}
					else {
						ok = PPSetError(PPERR_DIRNOTWACCS, path);
						break;
					}
				}
				else {
					// @v10.5.6 replacePath(buf, path, 1);
					// @v10.5.6 {
					SFsPath sp(dest_file_name);
					SFsPath sp_path(path);
					SETFLAGBYSAMPLE(sp.Flags, sp.fUNC, sp_path.Flags); // @v10.5.7
					sp.Drv = sp_path.Drv;
					sp.Dir = sp_path.Dir;
					// @v10.8.4 {
					if(dest_nam.NotEmpty())
						sp.Nam = dest_nam;
					if(dest_ext.NotEmpty())
						sp.Ext = dest_ext;
					// } @v10.8.4
					sp.Merge(temp_file_name);
					//STRNSCPY(buf, temp_file_name);
					dest_file_name = temp_file_name;
					// } @v10.5.6
					if(fileExists(dest_file_name)) {
						if(action == dfactDelete) { // Remove file
							SFile::Remove(dest_file_name);
							ok = 1;
						}
						else if(action == dfactCheckExistence) // Check existence
							ok = 1;
					}
					else if(action == dfactDelete)
						ok = 1; // Если файл не найден, то полагаем, что результат удаления положительный
					if(action == dfactCopy) { // Copy file
						copyFileByName(pFileName, dest_file_name);
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// AsyncCashGoodsIterator
//
AsyncCashGoodsIterator::AsyncCashGoodsIterator(PPID cashNodeID, long flags, PPID sinceDlsID, DeviceLoadingStat * pDls) :
	P_Dls(0), P_G2OAssoc(0), P_G2DAssoc(0), P_AcggIter(0), P_AlcPrc(0), Algorithm(algDefault)
{
	Init(cashNodeID, flags, sinceDlsID, pDls);
}

AsyncCashGoodsIterator::~AsyncCashGoodsIterator()
{
	delete P_G2OAssoc;
	delete P_AcggIter;
	delete P_AlcPrc;
}

AsyncCashGoodsGroupIterator * AsyncCashGoodsIterator::GetGroupIterator() { return P_AcggIter; }
PPID AsyncCashGoodsIterator::GetTobaccoGoodsCls() const { return TobaccoGoodsClsID; }
PPID AsyncCashGoodsIterator::GetGiftCardGoodsCls() const { return GiftCardGoodsClsID; }
const IterCounter & AsyncCashGoodsIterator::GetIterCounter() const
	{ return (Flags & ACGIF_UPDATEDONLY && Algorithm == algUpdBills) ? InnerCounter : Iter.GetIterCounter(); }
int AsyncCashGoodsIterator::GetDifferentPricesForLookBackPeriod(PPID goodsID, double basePrice, RealArray & rList)
	{ return AsyncCashGoodsIterator::__GetDifferentPricesForLookBackPeriod(goodsID, LocID, basePrice, PricesLookBackPeriod, rList); }

/*static*/int AsyncCashGoodsIterator::__GetDifferentPricesForLookBackPeriod(PPID goodsID, PPID locID, double basePrice, int lookBackPeriod, RealArray & rList)
{
	int    ok = -1;
	rList.clear();
    if(lookBackPeriod > 0) {
		const LDATE cd = getcurdate_();
		const LDATE stop_date = plusdate(cd, -lookBackPeriod);
		ReceiptCore & r_rc = BillObj->trfr->Rcpt;
		ReceiptTbl::Rec lot_rec;
#if 0 // Этот блок получает список цен только по открытым лотам {
		DateIter di(stop_date, 0);
		while(r_rc.EnumLots(goodsID, LocID, &di, &lot_rec) > 0) {
			// Transfer::GetLotPrices вызывать не следует поскольку нам нужна текущая цена
			double price = R3(lot_rec.Price);
			uint   p = 0;
			if(price > 0.0 && price != basePrice && !rList.lsearch(&price, &p, PTR_CMPFUNC(double))) {
				rList.insert(&price);
				ok = 1;
			}
		}
#else // }{ А этот блок получает список цен и по открытым и по закрытым лотам
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

PPID AsyncCashGoodsIterator::GetAlcoGoodsCls(SString * pProofExpr, SString * pVolumeExpr) const
{
	if(AlcoGoodsClsID) {
		ASSIGN_PTR(pProofExpr, AlcoProofExpr);
		ASSIGN_PTR(pVolumeExpr, AlcoVolExpr);
	}
	else {
		ASSIGN_PTR(pProofExpr, 0);
		ASSIGN_PTR(pVolumeExpr, 0);
	}
	return AlcoGoodsClsID;
}

int AsyncCashGoodsIterator::GetAlcoGoodsExtension(PPID goodsID, PPID lotID, PrcssrAlcReport::GoodsItem & rExt)
{
	int    ok = -1;
	if(goodsID) {
		if(!P_AlcPrc) {
			THROW_MEM(P_AlcPrc = new PrcssrAlcReport);
			P_AlcPrc->SetConfig(0);
			P_AlcPrc->Init();
		}
		if(P_AlcPrc->IsAlcGoods(goodsID)) {
			ok = P_AlcPrc->PreprocessGoodsItem(goodsID, lotID, 0, 0, rExt);
		}
	}
	CATCHZOK
	return ok;
}

bool AsyncCashGoodsIterator::IsSimplifiedDraftBeer(PPID goodsID) const // @v11.9.6
{
	return PPSyncCashSession::IsSimplifiedDraftBeerPosition(CashNodeID, goodsID);	
}

int AsyncCashGoodsIterator::Init(PPID cashNodeID, long flags, PPID sinceDlsID, DeviceLoadingStat * pDls)
{
	int    ok = 1;
	int    is_redo = 0; // Признак того, что выгрузка осуществляется в режиме REDO (товары, выгруженные ранее, начиная с заданной sinceDlsID)
	SString temp_buf;
	PPIniFile ini_file;
	LDATETIME last_exp_moment = ZERODATETIME;
	PPEquipConfig eq_cfg;
	ReadEquipConfig(&eq_cfg);
	P_Dls = pDls;
	Flags = (flags & ~(ACGIF_EXCLALTFOLD|ACGIF_IGNOREGWODISTAG)); // ACGIF_EXCLALTFOLD ACGIF_IGNOREGWODISTAG - internal flags
	SETFLAG(Flags, ACGIF_IGNOREGWODISTAG, eq_cfg.Flags & eq_cfg.fIgnoreNoDisGoodsTag); // @v10.6.11
	CashNodeID = cashNodeID;
	LocID    = 0;
	CodePos  = 0;
	GoodsPos = 0;
	Algorithm = algDefault;
	SinceDlsID = sinceDlsID;
	CurDate  = getcurdate_();
	GroupList.clear();
	UnitList.clear();
	GdsClsList.clear();
	GdsTypeList.clear();
	UpdatedBillList.clear(); // @v10.2.11
	{
		AlcoGoodsClsID = 0;
		TobaccoGoodsClsID = 0;
		GiftCardGoodsClsID = 0;
		AlcoProofExpr.Z();
		AlcoVolExpr.Z();
		PricesLookBackPeriod = 0;

		uint   i = 0;
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSALC, temp_buf.Z());
		StringSet ss(',', temp_buf);
		ss.get(&i, temp_buf.Z());
		if(GcObj.SearchBySymb(temp_buf, &AlcoGoodsClsID) > 0) {
			PPGdsClsPacket gc_pack;
			ss.get(&i, AlcoProofExpr);
			ss.get(&i, AlcoVolExpr);
			if(AlcoProofExpr.NotEmptyS() && AlcoVolExpr.NotEmptyS() && GcObj.Fetch(AlcoGoodsClsID, &gc_pack) > 0) {
			}
			else {
				AlcoGoodsClsID = 0;
				AlcoProofExpr.Z();
				AlcoVolExpr.Z();
			}
		}
		//
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSTOBACCO, temp_buf.Z());
		GcObj.SearchBySymb(temp_buf, &TobaccoGoodsClsID);
        //
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSGIFTCARD, temp_buf.Z());
		GcObj.SearchBySymb(temp_buf, &GiftCardGoodsClsID);
		//
		if(eq_cfg.LookBackPricePeriod > 0 && eq_cfg.LookBackPricePeriod <= 365*2) { // @v9.8.5
			PricesLookBackPeriod = eq_cfg.LookBackPricePeriod;
		}
		else {
			ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACGIPRICELOOKBACKPERIOD, &PricesLookBackPeriod);
			if(PricesLookBackPeriod < 0 || PricesLookBackPeriod > 365*2)
				PricesLookBackPeriod = 0;
		}
	}
	PPLoadText(PPTXT_LOG_ACGIMISS, VerMissMsg);
	UpdGoods.freeAll();
	RmvGoods.freeAll(); // @v10.6.8
	IterGoodsList.freeAll();
	QuotByQttyList.freeAll();
	NoDisToggleGoodsList.freeAll();
	LotThreshold = GObj.GetConfig().ACGI_Threshold;
	THROW(CnObj.GetAsync(CashNodeID, &AcnPack) > 0);
	LocID = AcnPack.LocID;
	{
		Flags &= ~ACGIF_UNCONDBASEPRICE;
		SETFLAG(Flags, ACGIF_UNCONDBASEPRICE, eq_cfg.Flags & PPEquipConfig::fUncondAsyncBasePrice);
	}
	if(AcnPack.ExtFlags & CASHFX_RESTRUSERGGRP && (Flags & ACGIF_UPDATEDONLY)) {
		PPAccessRestriction accsr;
		UserOnlyGoodsGrpID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
	}
	else
		UserOnlyGoodsGrpID = 0;
	if(AcnPack.GoodsGrpID) {
		Goods2Tbl::Rec cn_grp_rec;
		if(GObj.Fetch(AcnPack.GoodsGrpID, &cn_grp_rec) > 0 && cn_grp_rec.Flags & GF_FOLDER && cn_grp_rec.Flags & GF_EXCLALTFOLD)
			Flags |= ACGIF_EXCLALTFOLD;
	}
	if(AcnPack.ExtFlags & CASHFX_SEPARATERCPPRN) {
		P_G2DAssoc = new GoodsToObjAssoc(PPASS_GOODS2CASHNODE, PPOBJ_CASHNODE);
		CALLPTRMEMB(P_G2DAssoc, Load());
	}
	if(Flags & ACGIF_ENSUREUUID) {
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		if(tag_obj.Fetch(PPTAG_GOODS_UUID, &tag_rec) > 0) {
			; // ok
		}
		else {
			Flags &= ~ACGIF_ENSUREUUID;
			PPLogMessage(PPFILNAM_ERR_LOG, PPSTR_TEXT, PPTXT_ACGIFENSUREUUID_IGNORED, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
	}
	{
		RetailPriceExtractor::ExtQuotBlock eqb(AcnPack.ExtQuotID);
		long   rtlpf = 0;
		if(AcnPack.ExtFlags & CASHFX_IGNCONDQUOTS)
			rtlpf |= RTLPF_IGNCONDQUOTS;
		RetailExtr.Init(LocID, &eqb, 0, ZERODATETIME, rtlpf); // @v10.0.01 flags 0-->RTLPF_IGNCONDQUOTS // @v10.0.03 flags RTLPF_IGNCONDQUOTS-->rtlpf
	}
	Rec.Init();
	BcPrefixList.freeAll();
	if(GObj.GetConfig().Flags & GCF_USESCALEBCPREFIX) {
		PPObjScale sc_obj;
		sc_obj.GetListWithBcPrefix(&BcPrefixList);
	}
	if(Flags & ACGIF_INITLOCPRN) {
		LpObj.GetLocPrnAssoc(LocPrnAssoc);
		ZDELETE(P_G2OAssoc);
		THROW_MEM(P_G2OAssoc = new GoodsToObjAssoc(NZOR(AcnPack.GoodsLocAssocID, PPASS_GOODS2LOC), PPOBJ_LOCATION));
		THROW(P_G2OAssoc->IsValid());
		THROW(P_G2OAssoc->Load());
	}
	if(Flags & ACGIF_REDOSINCEDLS && SinceDlsID && P_Dls) {
		P_Dls->GetExportedObjectsSince(PPOBJ_GOODS, SinceDlsID, &UpdGoods);
		IterGoodsList = UpdGoods;
		if(AcnPack.GoodsGrpID) {
			uint c = IterGoodsList.getCount();
			if(c) do {
				const  PPID goods_id = IterGoodsList.get(--c);
				if(!GObj.BelongToGroup(goods_id, AcnPack.GoodsGrpID))
					IterGoodsList.atFree(c);
			} while(c);
		}
		IterGoodsList.sort();
		InnerCounter.Init(IterGoodsList.getCount());
		is_redo = 1;
	}
	else if(Flags & ACGIF_UPDATEDONLY) {
		int    alg = 0;
		LDATETIME moment = ZERODATETIME;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACGIALG, &Algorithm);
		if(!oneof3(Algorithm, algDefault, algUpdBillsVerify, algUpdBills))
			Algorithm = algDefault;
		if(P_Dls) {
			int    no_set_zerotime = 0;
			ini_file.GetInt(PPINISECT_SYSTEM, PPINIPARAM_ONLYLASTUPDGDSTOCASH, &no_set_zerotime);
			moment = getcurdatetime_();
			if(!no_set_zerotime)
				moment.t = ZEROTIME;
			if(UserOnlyGoodsGrpID) {
				//
				// В случае загрузки изменений только по товарам, которыми ограничен данный пользователь,
				// момент, от которого отсчитываются изменения, определяется последней загрузкой, сделанной этим
				// пользователем.
				//
				SysJournalTbl::Key1 k1;
				MEMSZERO(k1);
				k1.ObjType = PPOBJ_CASHNODE;
				k1.ObjID = CashNodeID;
				k1.Dt = moment.d;
				k1.Tm = MAXTIME;
				BExtQuery q(&SJ, 1);
				q.selectAll().where(SJ.ObjType == PPOBJ_CASHNODE && SJ.ObjID == CashNodeID && SJ.UserID == LConfig.UserID);
				for(q.initIteration(true, &k1, spLt); q.nextIteration() > 0;) {
					if(cmp(moment, SJ.data.Dt, SJ.data.Tm) > 0) {
						moment.Set(SJ.data.Dt, SJ.data.Tm);
						break;
					}
				}
			}
			else {
				DvcLoadingStatTbl::Rec dls_rec;
				PPID   prev_dls_id = 0;
				if(SinceDlsID) {
					if(P_Dls->GetPrev(SinceDlsID, &prev_dls_id, &dls_rec) > 0)
						if(cmp(moment, dls_rec.Dt, dls_rec.Tm) > 0)
							moment.Set(dls_rec.Dt, dls_rec.Tm);
				}
				else {
					if(P_Dls->GetPrev(0, &prev_dls_id, &dls_rec) > 0)
						if(cmp(moment, dls_rec.Dt, dls_rec.Tm) > 0)
							moment.Set(dls_rec.Dt, dls_rec.Tm);
				}
			}
			last_exp_moment = moment;
			P_Dls->GetUpdatedObjects(PPOBJ_GOODS, moment, &UpdGoods);
		}
		else {
			while(SJ.GetLastEvent(PPACN_EXPCASHSESS, 0/*extraVal*/, &moment, 7) > 0) {
				if(SJ.data.ObjType == PPOBJ_CASHNODE && SJ.data.ObjID == CashNodeID) {
					last_exp_moment = moment;
					SysJournalTbl::Key0 sjk0;
					sjk0.Dt = moment.d;
					sjk0.Tm = moment.t;
					BExtQuery q(&SJ, 0);
					q.selectAll().where(SJ.Dt >= moment.d && SJ.ObjType == PPOBJ_GOODS);
					for(q.initIteration(false, &sjk0, spGe); q.nextIteration() > 0;) {
						if(cmp(moment, SJ.data.Dt, SJ.data.Tm) < 0) {
							const long a = SJ.data.Action;
							if(oneof5(a, PPACN_OBJADD, PPACN_OBJUPD, PPACN_OBJUNIFY, PPACN_GOODSQUOTUPD, PPACN_QUOTUPD2)) {
								THROW(UpdGoods.add(SJ.data.ObjID));
							}
						}
					}
					break;
				}
			}
		}
		// @v10.6.8 {
		{
			PPObjGoodsBasket gb_obj;
			PPBasketPacket gb_pack;
			Goods2Tbl::Rec gb_goods_rec;
			if(gb_obj.GetPacket(PPGDSBSK_ACNUPD, &gb_pack) > 0) {
				for(uint gbidx = 0; gbidx < gb_pack.Lots.getCount(); gbidx++) {
					const  PPID gb_goods_id = labs(gb_pack.Lots.at(gbidx).GoodsID);
					if(gb_goods_id && GObj.Fetch(gb_goods_id, &gb_goods_rec) > 0)
						UpdGoods.add(gb_goods_id);
				}
			}
			if(gb_obj.GetPacket(PPGDSBSK_ACNRMV, &gb_pack) > 0) {
				for(uint gbidx = 0; gbidx < gb_pack.Lots.getCount(); gbidx++) {
					const  PPID gb_goods_id = labs(gb_pack.Lots.at(gbidx).GoodsID);
					if(gb_goods_id && GObj.Fetch(gb_goods_id, &gb_goods_rec) > 0)
						RmvGoods.add(gb_goods_id);
				}
			}
		}
		if(RmvGoods.getCount()) {
			RmvGoods.sortAndUndup();
			UpdGoods.add(&RmvGoods);
		}
		// } @v10.6.8
		UpdGoods.sortAndUndup();
		if(oneof2(Algorithm, algUpdBillsVerify, algUpdBills)) {
			IterGoodsList = UpdGoods;
			THROW(BillObj->GetGoodsListByUpdatedBills(LocID, moment, IterGoodsList, &UpdatedBillList));
			if(AcnPack.GoodsGrpID) {
				uint c = IterGoodsList.getCount();
				if(c) do {
					const  PPID goods_id = IterGoodsList.get(--c);
					if(!GObj.BelongToGroup(goods_id, AcnPack.GoodsGrpID))
						IterGoodsList.atFree(c);
				} while(c);
			}
			IterGoodsList.sort();
			InnerCounter.Init(IterGoodsList.getCount());
			// @v10.2.11 {
			if(P_Dls && P_Dls->GetCurStatID() && UpdatedBillList.getCount()) {
				P_Dls->RegisterBillList(P_Dls->GetCurStatID(), UpdatedBillList);
			}
			// } @v10.2.11
		}
	}
	{
		LDATETIME since;
		if(Flags & ACGIF_UPDATEDONLY && !!last_exp_moment) {
			since = last_exp_moment;
			since.t = ZEROTIME;
		}
		else {
			since.d = plusdate(getcurdate_(), -30);
			since.t = ZEROTIME;
		}
		PPIDArray acn_goodsnodisrmvd;
		acn_goodsnodisrmvd.add(PPACN_GOODSNODISRMVD);
		SJ.GetObjListByEventSince(PPOBJ_GOODS, &acn_goodsnodisrmvd, since, NoDisToggleGoodsList, 0);
	}
	if(Algorithm != algUpdBills || !(Flags & ACGIF_UPDATEDONLY)) {
		if(AcnPack.GoodsGrpID) {
			THROW(Iter.Init(AcnPack.GoodsGrpID, GoodsIterator::ordByName));
		}
		else {
			THROW(Iter.Init(GoodsIterator::ordByName));
		}
	}
	{
		const PPIDArray * p_group_list = 0;
		ZDELETE(P_AcggIter);
		/* @v9.8.6 if(AcnPack.GoodsGrpID && !(Flags & ACGIF_EXCLALTFOLD))*/ {
			Goods2Tbl::Rec goods_rec;
			if(Flags & ACGIF_UPDATEDONLY && Algorithm == algUpdBills) {
				for(uint i = 0; i < IterGoodsList.getCount(); i++) {
					if(GObj.Fetch(IterGoodsList.at(i), &goods_rec) > 0) {
						GroupList.add(goods_rec.ParentID);
						UnitList.addnz(goods_rec.UnitID);
						UnitList.addnz(goods_rec.PhUnitID);
						GdsClsList.addnz(goods_rec.GdsClsID);
						GdsTypeList.addnz(goods_rec.GoodsTypeID);
					}
				}
			}
			else {
				while(Iter.Next(&goods_rec) > 0) {
					GroupList.add(goods_rec.ParentID);
					UnitList.addnz(goods_rec.UnitID);
					UnitList.addnz(goods_rec.PhUnitID);
					GdsClsList.addnz(goods_rec.GdsClsID);
					GdsTypeList.addnz(goods_rec.GoodsTypeID);
				}
				//
				// Снова иницализируем итератор
				//
				if(AcnPack.GoodsGrpID) {
					THROW(Iter.Init(AcnPack.GoodsGrpID, GoodsIterator::ordByName));
				}
				else {
					THROW(Iter.Init(GoodsIterator::ordByName));
				}
			}
			GroupList.sortAndUndup();
			UnitList.sortAndUndup();
			GdsClsList.sortAndUndup();
			GdsTypeList.sortAndUndup();
		}
		if(AcnPack.GoodsGrpID && !(Flags & ACGIF_EXCLALTFOLD))
			p_group_list = &GroupList;
		THROW_MEM(P_AcggIter = new AsyncCashGoodsGroupIterator(CashNodeID, 0, P_Dls, p_group_list));
	}
	GObj.P_Tbl->ClearQuotCache();
	CATCH
		ok = 0;
		ZDELETE(P_AcggIter);
	ENDCATCH
	return ok;
}

const PPIDArray * FASTCALL AsyncCashGoodsIterator::GetRefList(int refType) const
{
	switch(refType) {
		case PPOBJ_GOODSGROUP: return &GroupList;
		case PPOBJ_UNIT: return &UnitList;
		case PPOBJ_GOODSCLASS: return &GdsClsList;
		case PPOBJ_GOODSTYPE: return &GdsTypeList;
		default: return 0;
	}
}

int AsyncCashGoodsIterator::SearchCPrice(PPID goodsID, double * pPrice)
{
	int    ok = 1;
	CCurPriceTbl::Key0 k;
	k.CashID  = CashNodeID;
	k.GoodsID = goodsID;
	if(CCP.search(0, &k, spEq)) {
		if(pPrice) {
			const double p = *pPrice;
			*pPrice = MONEYTOLDBL(CCP.data.Price);
			ok = (p == *pPrice) ? 2 : 1;
		}
	}
	else {
		*pPrice = 0;
		ok = PPDbSearchError();
	}
	return ok;
}

int AsyncCashGoodsIterator::UpdateCPrice(PPID goodsID, double price)
{
	int    ok = 1, r = 0;
	double p = price;
	THROW(r = SearchCPrice(goodsID, &p));
	if(r == 1) {
		LDBLTOMONEY(price, CCP.data.Price);
		THROW_DB(CCP.updateRec());
	}
	else if(r < 0) {
		CCP.data.CashID  = CashNodeID;
		CCP.data.GoodsID = goodsID;
		LDBLTOMONEY(price, CCP.data.Price);
		THROW_DB(CCP.insertRec());
	}
	else
		ok = 2;
	CATCHZOK
	return ok;
}

int AsyncCashGoodsIterator::Next(AsyncCashGoodsInfo * pInfo)
{
	int    ok = -1;
	const LDATETIME now_dtm = getcurdatetime_();
	Reference * p_ref = PPRef;
	int    r;
	PPIDArray acn_goodsnodisrmvd;
	acn_goodsnodisrmvd.add(PPACN_GOODSNODISRMVD);
	Goods2Tbl::Rec grec;
	SString temp_buf;
	do {
		if((Flags & ACGIF_ALLCODESPERITER) || CodePos >= Codes.getCount()) {
			if(Flags & ACGIF_UPDATEDONLY && Algorithm == algUpdBills) {
				r = -1;
				while(r < 0 && GoodsPos < IterGoodsList.getCount()) {
					InnerCounter.Increment();
					if(GObj.Search(IterGoodsList.at(GoodsPos++), &grec) > 0)
						r = 1;
				}
			}
			else {
				THROW(r = Iter.Next(&grec));
			}
			if(r < 0) {
				ok = -1;
				break;
			}
			else {
				int    updated = 1;
				double old_price = 0.0;
				RetailExtrItem rtl_ext_item;
				Rec.Init();
				rtl_ext_item.QuotList = pInfo->QuotList;
				const int c = RetailExtr.GetPrice(grec.ID, 0, 0.0, &rtl_ext_item);
				THROW(c);
				Rec.Expiry = rtl_ext_item.Expiry; // @v11.9.5
				Rec.AllowedPriceR = rtl_ext_item.AllowedPriceR; // @v10.6.4
				Rec.QuotList = rtl_ext_item.QuotList;
				const double price_ = (Flags & ACGIF_UNCONDBASEPRICE) ? rtl_ext_item.BasePrice : rtl_ext_item.Price;
				if(Flags & ACGIF_INITLOCPRN && P_G2OAssoc) {
					PPID   loc_id = 0;
					PPID   prn_id = 0;
					P_G2OAssoc->Get(grec.ID, &loc_id);
					if(LocPrnAssoc.Search(loc_id, &prn_id, 0)) {
						Rec.LocPrnID = prn_id;
						PPLocPrinter lp_rec;
						if(LpObj.Search(prn_id, &lp_rec) > 0)
							STRNSCPY(Rec.LocPrnSymb, lp_rec.Symb);
					}
				}
				if(c == GPRET_CLOSEDLOTS && LotThreshold > 0 && diffdate(now_dtm.d, rtl_ext_item.CurLotDate) > LotThreshold) { // @v10.8.10 LConfig.OperDate-->_now.d
					updated = 0;
				}
				else if(Flags & ACGIF_UPDATEDONLY) {
					DlsObjTbl::Rec dlso_rec;
					if(UserOnlyGoodsGrpID && !GObj.BelongToGroup(grec.ID, UserOnlyGoodsGrpID))
						updated = 0;
					else if(!UpdGoods.bsearch(grec.ID)) {
						if(!P_Dls) {
							old_price = price_;
							THROW(r = SearchCPrice(grec.ID, &old_price));
							if(r == 2)
								updated = 0;
						}
						else if(P_Dls->GetLastObjInfo(PPOBJ_GOODS, grec.ID, CurDate, &dlso_rec) > 0) {
							old_price = dlso_rec.Val;
							if(dbl_cmp(old_price, price_) == 0)
								updated = 0;
						}
					}
				}
				if(updated && (c > 0 || price_ > 0.0)) { // @v10.6.0 (price_ > 0.0)-->(c > 0) условие заменено для того, чтобы товар с валидной нулевой ценой загружался.
						// @v10.7.10 (c > 0)-->(c > 0 || price_ > 0.0) Товары, у которых нет лотов, но есть валидная положительная цена должны загружаться
					uint  i;
					PPUnit unit_rec;
					PPGoodsTaxEntry gtx;
					PPQuotArray quot_list(grec.ID);
					if(Algorithm == algUpdBillsVerify && !IterGoodsList.bsearch(grec.ID)) {
						PPFormat(VerMissMsg, &temp_buf, grec.ID, grec.Name, UpdGoods.bsearch(grec.ID), old_price, price_);
						PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					Rec.ID       = grec.ID;
					// Замена символов перевода каретки на пробелы
					(temp_buf = grec.Name).ReplaceChar('\x0D', ' ').ReplaceChar('\x0A', ' ').ReplaceStr("  ", " ", 0).CopyTo(Rec.Name, sizeof(Rec.Name));
					(temp_buf = grec.Abbr).ReplaceChar('\x0D', ' ').ReplaceChar('\x0A', ' ').ReplaceStr("  ", " ", 0).CopyTo(Rec.Abbr, sizeof(Rec.Abbr));
					if(Flags & ACGIF_EXCLALTFOLD)
						GObj.P_Tbl->GetExclusiveAltParent(grec.ID, AcnPack.GoodsGrpID, &Rec.ParentID);
					else
						Rec.ParentID = grec.ParentID;
					Rec.UnitID   = grec.UnitID;
					Rec.PhUnitID = grec.PhUnitID;
					Rec.PhUPerU  = grec.PhUPerU;
					Rec.ManufID  = grec.ManufID;
					Rec.GdsClsID = grec.GdsClsID;
					Rec.GoodsTypeID = grec.GoodsTypeID;
					Rec.Cost     = rtl_ext_item.Cost;
					Rec.Price    = price_;
					Rec.Precision = fpow10i(-3);
					if(AcnPack.ExtFlags & CASHFX_APPLYUNITRND && GObj.FetchUnit(grec.UnitID, &unit_rec) > 0 && unit_rec.Rounding_ > 0.0)
						Rec.Precision = unit_rec.Rounding_;
					Rec.GoodsFlags = grec.Flags;
					if(Flags & ACGIF_IGNOREGWODISTAG) // @v10.6.11
						Rec.NoDis = 0;
					else if(grec.Flags & GF_NODISCOUNT)
						Rec.NoDis  = 1;
					else if(NoDisToggleGoodsList.bsearch(grec.ID))
						Rec.NoDis = (SJ.GetLastObjEvent(PPOBJ_GOODS, grec.ID, &acn_goodsnodisrmvd, 0) > 0) ? -1 : 0;
					else
						Rec.NoDis = 0;
					if(Rec.GoodsTypeID) {
						PPGoodsType2 gt_rec;
						if(GObj.FetchGoodsType(Rec.GoodsTypeID, &gt_rec) > 0) {
							if(gt_rec.Flags & GTF_GMARKED) {
								Rec.Flags_ |= AsyncCashGoodsInfo::fGMarkedType;
								Rec.ChZnProdType = static_cast<int16>(gt_rec.ChZnProdType); // @v10.9.0
							}
							// @v11.7.10 {
							if(gt_rec.Flags & GTF_EXCISEPROFORMA)
								Rec.Flags_ |= AsyncCashGoodsInfo::fGExciseProForma;
							// } @v11.7.10 
							if(gt_rec.PriceRestrID) { // @v10.6.4
								//BillObj->GetPriceRestrictions(grec.ID, 0, )
							}
						}
					}
					Rec.ExtQuot = rtl_ext_item.ExtPrice;
					//
					// Инициализируем номер отдела, ассоциированный с группой товара
					//
					Rec.DivN = 1;
					if(AcnPack.Flags & CASHF_EXPDIVN && AcnPack.P_DivGrpList) {
						long   default_div = 1;
						int    use_default_div = 1;
						PPGenCashNode::DivGrpAssc * p_dg_item;
						for(i = 0; AcnPack.P_DivGrpList->enumItems(&i, (void **)&p_dg_item);) {
							if(p_dg_item->GrpID == 0)
								default_div = p_dg_item->DivN;
							else if(GObj.BelongToGroup(Rec.ID, p_dg_item->GrpID, 0) > 0) {
								Rec.DivN = p_dg_item->DivN;
								use_default_div = 0;
								break;
							}
						}
						if(use_default_div)
							Rec.DivN = default_div;
					}
					{
						//
						// Инициализируем (если необходимо) кассовый узел, ассоциированный с товаром
						//
						PPID   assoc_id = 0;
						PPCashNode cn_rec;
						if(P_G2DAssoc && P_G2DAssoc->Get(grec.ID, &assoc_id) > 0 && assoc_id && CnObj.Fetch(assoc_id, &cn_rec) > 0) {
							Rec.AsscPosNodeID = cn_rec.ID;
							STRNSCPY(Rec.AsscPosNodeSymb, cn_rec.Symb);
						}
					}
					if(GObj.FetchTax(grec.ID, /*LConfig.OperDate*/now_dtm.d, 0L, &gtx) > 0)
						Rec.VatRate = gtx.GetVatRate();
					CodePos = 0;
					GObj.ReadBarcodes(grec.ID, Codes);
					if(GObj.GetConfig().Flags & GCF_USESCALEBCPREFIX) {
						for(i = 0; i < BcPrefixList.getCount(); i++) {
							int r2 = GObj.GenerateScaleBarcode(grec.ID, BcPrefixList.at(i).Key, temp_buf);
							if(r2 > 0) {
								BarcodeTbl::Rec bc_rec;
								bc_rec.GoodsID = grec.ID;
								bc_rec.Qtty = 1.0;
								temp_buf.CopyTo(bc_rec.Code, sizeof(bc_rec.Code));
								Codes.insert(&bc_rec);
							}
							else if(r2 == 0)
								PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
						}
					}
					Rec.P_CodeList = &Codes;
					if(grec.Flags & GF_EXTPROP) {
						PPGoodsPacket __pack;
						THROW(p_ref->GetPropVlrString(PPOBJ_GOODS, grec.ID, GDSPRP_EXTSTRDATA, __pack.ExtString));
						if(__pack.ExtString.NotEmptyS()) {
							StringSet ss;
							__pack.GetExtStrData(GDSEXSTR_LABELNAME, Rec.LabelName);
							if(AcnPack.AddedMsgSign.NotEmpty() && __pack.PrepareAddedMsgStrings(AcnPack.AddedMsgSign, PPGoodsPacket::pamsfStrictOrder, 0, ss) == 1)
								Rec.AddedMsgList = ss;
						}
					}
					if(AcnPack.Flags & CASHF_EXPGOODSREST) {
						GoodsRestParam param;
						param.Date    = /*LConfig.OperDate*/now_dtm.d;
						param.LocID   = LocID;
						param.GoodsID = Rec.ID;
						THROW(BillObj->trfr->GetCurRest(param));
						Rec.Rest = param.Total.Rest;
					}
					THROW(GObj.P_Tbl->FetchQuotList(Rec.ID, 0, LocID, quot_list));
					QuotByQttyList.freeAll();
					for(i = 0; i < quot_list.getCount(); i++) {
						const PPQuot & r_quot = quot_list.at(i);
						if(r_quot.MinQtty)
							THROW_SL(QuotByQttyList.insert(&r_quot));
					}
					if(QuotByQttyList.getCount())
						Rec.P_QuotByQttyList = &QuotByQttyList;
					if(P_Dls) {
						DeviceLoadingStat::GoodsInfo goods_info;
						goods_info.ID = Rec.ID;
						STRNSCPY(goods_info.Name, Rec.Name);
						goods_info.Price = Rec.Price;
						P_Dls->RegisterGoods(P_Dls->GetCurStatID(), &goods_info);
					}
					else {
						THROW(UpdateCPrice(Rec.ID, Rec.Price));
					}
					// @v10.0.04 {
					if(Flags & ACGIF_ENSUREUUID) {
						THROW(GObj.GetUuid(Rec.ID, Rec.Uuid, true, -1)); // @v10.9.4
						/* @v10.9.4
						const  PPID uuid_tag_id = PPTAG_GOODS_UUID;
						ObjTagItem tag_item;
						S_GUID temp_uuid;
						if(p_ref->Ot.GetTag(PPOBJ_GOODS, Rec.ID, uuid_tag_id, &tag_item) > 0) {
							if(tag_item.GetGuid(&temp_uuid)) {
								Rec.Uuid = temp_uuid;
							}
						}
						else {
							temp_uuid.Generate();
							if(tag_item.SetGuid(uuid_tag_id, &temp_uuid)) {
								THROW(p_ref->Ot.PutTag(PPOBJ_GOODS, Rec.ID, &tag_item, -1));
								Rec.Uuid = temp_uuid;
							}
						}*/
					}
					// } @v10.0.04
					// @v10.6.8 {
					if(RmvGoods.lsearch(Rec.ID)) {
						Rec.Flags_ |= AsyncCashGoodsInfo::fDeleted;
					}
					// } @v10.6.8
				}
			}
		}
		if(CodePos < Codes.getCount() && (!(Flags & ACGIF_ALLCODESPERITER) || CodePos == 0)) {
			uint pref_pos = 0;
			const BarcodeTbl::Rec & r_cur_bc_item = Codes.at(CodePos);
			strip(STRNSCPY(Rec.BarCode, r_cur_bc_item.Code));
			if(Codes.GetPreferredItem(&pref_pos))
				strip(STRNSCPY(Rec.PrefBarCode, Codes.at(pref_pos).Code));
			else
				STRNSCPY(Rec.PrefBarCode, Rec.BarCode);
			Rec.UnitPerPack = r_cur_bc_item.Qtty;
			SETFLAG(Rec.Flags_, AsyncCashGoodsInfo::fGMarkedCode, IsInnerBarcodeType(r_cur_bc_item.BarcodeType, BARCODE_TYPE_MARKED)); // @v10.4.11
			*pInfo = Rec;
			CodePos++;
			ok = 1;
		}
		if(Flags & ACGIF_UPDATEDONLY && Algorithm == algUpdBills)
			PPWaitPercent(GoodsPos, IterGoodsList.getCount());
		else
			PPWaitPercent(GetIterCounter());
	} while(ok <= 0);
	CATCHZOK
	return ok;
}

AsyncCashGoodsInfo::AsyncCashGoodsInfo()
{
	Init();
}

void AsyncCashGoodsInfo::Init()
{
	ID = 0;
	memzero(Name, sizeof(Name));
	memzero(BarCode, sizeof(BarCode));
	memzero(PrefBarCode, sizeof(PrefBarCode));
	UnitID = 0;
	PhUnitID = 0;
	PhUPerU = 0.0;
	ManufID = 0;
	GdsClsID = 0;
	GoodsTypeID = 0;
	UnitPerPack = 0.0;
	Cost = 0.0;
	Price = 0.0;
	ExtQuot = 0.0;
	Rest = 0.0;
	Precision = fpow10i(-3);
	GoodsFlags = 0;
	// @v10.4.11 Deleted_ = 0;
	Flags_ = 0; // @v10.4.11
	NoDis = 0;
	// @v10.9.0 Reserve = 0; // @v10.4.11
	ChZnProdType = 0; // @v10.9.0
	DivN = 0;
	VatRate = 0.0;
	LocPrnID = 0;
	AsscPosNodeID = 0;
	AllowedPriceR.Z(); // @v10.6.4
	Uuid.Z(); // @v10.0.04
	AddedMsgList.Z();
	LabelName.Z();
	memzero(LocPrnSymb, sizeof(LocPrnSymb));
	memzero(AsscPosNodeSymb, sizeof(AsscPosNodeSymb));
	for(uint i = 0; i < QuotList.getCount(); i++)
		QuotList.at(i).Val = 0.0;
	P_QuotByQttyList = 0;
}

IMPL_INVARIANT_C(AsyncCashGoodsInfo)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(UnitPerPack > 0.0, pInvP);
	S_ASSERT_P(fabs(UnitPerPack) > fpow10i(-6), pInvP);
	size_t bc_len = sstrlen(BarCode);
	S_ASSERT_P(bc_len < sizeof(BarCode), pInvP);
	for(size_t i = 0; i < bc_len; i++) {
		S_ASSERT_P(isdec(BarCode[i]), pInvP);
	}
	S_INVARIANT_EPILOG(pInvP);
}

int AsyncCashGoodsInfo::AdjustBarcode(int chkDig)
{
	const size_t bclen = sstrlen(BarCode);
	if(bclen > 3 && bclen < 7) {
		padleft(BarCode, '0', 12 - bclen);
		if(chkDig)
			AddBarcodeCheckDigit(BarCode);
		return 1;
	}
	else
		return -1;
}
//
// AsyncCashSCardsIterator
//
AsyncCashSCardInfo::AsyncCashSCardInfo() : Flags(0), Rest(0.0), PsnDOB(ZERODATE)
{
}

AsyncCashSCardInfo & AsyncCashSCardInfo::Z()
{
	MEMSZERO(Rec);
	//IsClosed = 0;
	Flags = 0;
	Rest = 0.0;
	PsnDOB = ZERODATE;
	PsnName.Z();
	Phone.Z();
	PsnPhone.Z();
	Email.Z();
	return *this;
}

AsyncCashSCardsIterator::AsyncCashSCardsIterator(PPID cashNodeID, int updOnly, DeviceLoadingStat * pDLS, PPID statID) :
	P_IterQuery(0), UpdatedOnly(updOnly), P_DLS(pDLS), StatID(statID), Since(ZERODATETIME)
{
	DefSCardPersonID = SCObj.GetConfig().DefPersonID;
	PersonTbl::Rec psn_rec;
	if(PsnObj.Search(DefSCardPersonID, &psn_rec) > 0)
		DefPersonName = psn_rec.Name;
	MEMSZERO(NodeRec);
	if(cashNodeID) {
		PPObjCashNode cn_obj;
		if(cn_obj.Search(cashNodeID, &NodeRec) <= 0)
			MEMSZERO(NodeRec);
	}
	if(UpdatedOnly) {
		if(UpdatedOnly == 2 && statID && P_DLS) {
			P_DLS->GetExportedObjectsSince(PPOBJ_SCARD, statID, &UpdSCardList);
		}
		else {
			int    is_event = 0;
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			if(P_DLS) {
				const LDATETIME now_dtm = getcurdatetime_();
				PPID   last_loading = 0;
				PPID   dls_id = 0;
				DvcLoadingStatTbl::Rec dls_rec;
				if(P_DLS->GetLast(dvctCashs, cashNodeID, now_dtm, &dls_id, &dls_rec) > 0) {
					Since.Set(dls_rec.Dt, dls_rec.Tm);
					is_event = 1;
				}
			}
			if(!is_event && p_sj) {
				while(!is_event && p_sj->GetLastEvent(PPACN_EXPCASHSESS, 0/*extraVal*/, &Since, 7) > 0)
					if(p_sj->data.ObjType == PPOBJ_CASHNODE && p_sj->data.ObjID == cashNodeID)
						is_event = 1;
			}
			if(is_event) {
				//
				// Получаем список карт, которые изменились с момента последней загрузки
				//
				PPIDArray acn_list;
				acn_list.addzlist(PPACN_OBJADD, PPACN_OBJUPD, PPACN_SCARDDISUPD, PPACN_SCARDOWNERUPDATED, 0L); // @v11.4.0 PPACN_SCARDOWNERUPDATED
				p_sj->GetObjListByEventSince(PPOBJ_SCARD, &acn_list, Since, UpdSCardList, 0);
			}
			else {
				Since.Z();
				UpdatedOnly = 0;
			}
		}
	}
}

AsyncCashSCardsIterator::~AsyncCashSCardsIterator()
{
	delete P_IterQuery;
}

int AsyncCashSCardsIterator::Init(const PPSCardSerPacket * pScsPack)
{
	int    ok = 1;
	RVALUEPTR(ScsPack, pScsPack);
	SCardTbl::Key2 k2, k2_;
	MEMSZERO(k2);
	k2.SeriesID = ScsPack.Rec.ID;
	k2_ = k2;
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(SCObj.P_Tbl, 2));
	P_IterQuery->selectAll().where(SCObj.P_Tbl->SeriesID == ScsPack.Rec.ID);
	Counter.Init(P_IterQuery->countIterations(0, &k2_, spGe));
	P_IterQuery->initIteration(false, &k2, spGe);
	CATCHZOK
	return ok;
}

int AsyncCashSCardsIterator::Next(AsyncCashSCardInfo * pInfo)
{
	int    ok = -1;
	if(pInfo) {
		pInfo->Z();
		Reference * p_ref = PPRef; // @v11.3.5
		PersonTbl::Rec psn_rec;
		PPSCardPacket sc_pack; // @v10.6.3
		PPELinkArray ela;
		while(ok < 0 && P_IterQuery && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			const  PPID sc_id = SCObj.P_Tbl->data.ID;
			if(SCObj.GetPacket(sc_id, &sc_pack) > 0) {
				Rec = sc_pack.Rec;
				if(NodeRec.Flags & CASHF_EXPCHECKD) {
					if(sstrlen(Rec.Code) == 12)
						AddBarcodeCheckDigit(Rec.Code);
				}
				SCObj.SetInheritance(&ScsPack, &Rec);
				if(!UpdatedOnly || UpdSCardList.bsearch(Rec.ID)) {
					pInfo->Rec = Rec;
					SETIFZ(pInfo->Rec.PersonID, DefSCardPersonID);
					if(pInfo->Rec.PersonID) {
						if(pInfo->Rec.PersonID == DefSCardPersonID)
							pInfo->PsnName = DefPersonName;
						else if(PsnObj.Fetch(pInfo->Rec.PersonID, &psn_rec) > 0) {
							pInfo->PsnName = psn_rec.Name;
							ObjTagItem tag_item;
							StringSet ss;
							LDATE   dob = ZERODATE;
							if(p_ref->Ot.GetTag(PPOBJ_PERSON, psn_rec.ID, PPTAG_PERSON_DOB, &tag_item) > 0 && tag_item.GetDate(&dob) && checkdate(dob)) {
								pInfo->PsnDOB = dob;
							}
							PersonCore::GetELinks(psn_rec.ID, ela);
							ela.GetSinglePhone(pInfo->PsnPhone, 0);
							if(ela.GetListByType(ELNKRT_EMAIL, ss) > 0) {
								assert(ss.getCount());
								ss.get(0U, pInfo->Email);
							}
							// @v11.3.5 {
							if(psn_rec.Flags & PSNF_DONTSENDCCHECK)
								pInfo->Flags |= AsyncCashSCardInfo::fDisableSendPaperlassCCheck;
							// } @v11.3.5 
						}
					}
					SETFLAG(pInfo->Flags, AsyncCashSCardInfo::fClosed, ((Rec.Flags & SCRDF_CLOSED) || (Rec.Expiry && diffdate(Rec.Expiry, getcurdate_()) < 0))); // @v10.8.10 LConfig.OperDate-->getcurdate_()
					{
						int    scst = ScsPack.Rec.GetType();
						if(oneof2(scst, scstCredit, scstBonus)) {
							double rest = 0.0;
							SCObj.P_Tbl->GetRest(Rec.ID, ZERODATE, &rest);
							pInfo->Rest = rest;
						}
						else
							pInfo->Rest = 0.0;
					}
					sc_pack.GetExtStrData(PPSCardPacket::extssPhone, pInfo->Phone); // @v10.6.3
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int AsyncCashSCardsIterator::SetStat()
{
	int    ok = -1;
	if(P_DLS && StatID && Rec.ID) {
		DeviceLoadingStat::SCardInfo  scard_info;
		scard_info.ID = Rec.ID;
		STRNSCPY(scard_info.Code, Rec.Code);
		scard_info.Discount = fdiv100i(Rec.PDis);
		P_DLS->RegisterSCard(StatID, &scard_info);
		ok = 1;
	}
	return ok;
}
//
// AsyncCashiersIterator
//
AsyncCashiersIterator::AsyncCashiersIterator() : ProsessUnworkedPos(0), TabNumRegID(0), P_IterQuery(0), Since(ZERODATETIME)
{
}

AsyncCashiersIterator::~AsyncCashiersIterator()
{
	delete P_IterQuery;
}

int AsyncCashiersIterator::Init(PPID cashNodeID)
{
	int    ok = -1;
	PPID   psn_kind_id = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	Iterated.Z();
	Unworked.Z();
	ProsessUnworkedPos = 0;
	{
		PPEquipConfig eq_cfg;
		ReadEquipConfig(&eq_cfg);
		TabNumRegID = eq_cfg.GetCashierTabNumberRegTypeID();
		psn_kind_id = eq_cfg.CshrsPsnKindID;
	}
	if(psn_kind_id) {
		int  is_event = 0;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		if(p_sj) {
			while(!is_event && p_sj->GetLastEvent(PPACN_EXPCASHSESS, 0/*extraVal*/, &Since, 7) > 0)
				if(p_sj->data.ObjType == PPOBJ_CASHNODE && p_sj->data.ObjID == cashNodeID)
					is_event = 1;
		}
		if(!is_event)
			Since.Z();
		PersonKindTbl::Key0  k0;
		MEMSZERO(k0);
		k0.KindID = psn_kind_id;
		THROW_MEM(P_IterQuery = new BExtQuery(&PsnObj.P_Tbl->Kind, 0));
		P_IterQuery->select(PsnObj.P_Tbl->Kind.PersonID, 0L).where(PsnObj.P_Tbl->Kind.KindID == psn_kind_id);
		P_IterQuery->initIteration(false, &k0, spGe);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int AsyncCashiersIterator::Next(AsyncCashierInfo * pInfo)
{
	int    ok = -1;
	if(pInfo) {
		MEMSZERO(*pInfo);
		while(ok < 0 && (!ProsessUnworkedPos || ProsessUnworkedPos < Unworked.getCount())) {
			int  is_iteration = 0;
			if(!ProsessUnworkedPos && P_IterQuery && P_IterQuery->nextIteration() > 0) {
				is_iteration = 1;
				PPPersonPacket  psn_pack;
				if(PsnObj.GetPacket(PsnObj.P_Tbl->Kind.data.PersonID, &psn_pack, 0) > 0 && (psn_pack.CshrInfo.Flags & CIF_CASHIER)) {
					if(TabNumRegID) {
						RegisterTbl::Rec reg_rec;
						for(uint  pos = 0; psn_pack.Regs.GetRegister(TabNumRegID, &pos, &reg_rec) > 0;)
							if(reg_rec.Expiry == ZERODATE || diffdate(Since.d, reg_rec.Expiry) <= 0) {
								PPID  tab_num;
								strtolong(reg_rec.Num, &tab_num);
								if(reg_rec.Expiry != ZERODATE && diffdate(getcurdate_(), reg_rec.Expiry) > 0) // @v10.8.10 LConfig.OperDate-->getcurdate_()
									Unworked.addUnique(tab_num);
								else if(Iterated.addUnique(tab_num) > 0) {
									pInfo->TabNum   = tab_num;
									pInfo->IsWorked = 1;
								}
							}
					}
					else if(Iterated.addUnique(psn_pack.Rec.ID) > 0) {
						pInfo->TabNum   = psn_pack.Rec.ID;
						pInfo->IsWorked = 1;
					}
					if(pInfo->IsWorked) {
						SBitArray  rights_ary;
						STRNSCPY(pInfo->Name, psn_pack.Rec.Name);
						STRNSCPY(pInfo->Password, psn_pack.CshrInfo.Password);
						rights_ary.Init(&psn_pack.CshrInfo.Rights, sizeof(psn_pack.CshrInfo.Rights) * 8);
						for(size_t pos = 0; pos < sizeof(pInfo->Rights); pos++) {
							const int  right = rights_ary.get(pos);
							if(right < 0)
								break;
							else
								pInfo->Rights[pos] = right;
						}
						ok = 1;
					}
				}
			}
			if(!is_iteration) {
				if(ProsessUnworkedPos < Unworked.getCount() && Iterated.addUnique(Unworked.at(ProsessUnworkedPos)) > 0) {
					pInfo->TabNum = Unworked.at(ProsessUnworkedPos);
					ok = 1;
				}
				ProsessUnworkedPos++;
			}
		}
	}
	return ok;
}
//
// AsyncCashGoodsGroupIterator
//
AsyncCashGoodsGroupInfo::AsyncCashGoodsGroupInfo()
{
	Init();
}

void AsyncCashGoodsGroupInfo::Init()
{
	THISZERO();
}

AsyncCashGoodsGroupIterator::AsyncCashGoodsGroupIterator(PPID cashNodeID, long flags, DeviceLoadingStat * pDls, const PPIDArray * pTermGrpList) :
	P_GrpList(0)
{
	Init(cashNodeID, flags, pDls, pTermGrpList);
}

AsyncCashGoodsGroupIterator::~AsyncCashGoodsGroupIterator()
{
	ZDELETE(P_GrpList);
}

int AsyncCashGoodsGroupIterator::MakeGroupList(StrAssocArray * pTreeList, PPID parentID, uint level)
{
	int    ok = -1;
	SString temp_buf;
	for(uint i = 0; i < pTreeList->getCount(); i++) {
		StrAssocArray::Item item = pTreeList->Get(i);
		if(item.ParentId == parentID) {
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(item.Id, &goods_rec) > 0 && !(goods_rec.Flags & GF_ALTGROUP)) {
				AsyncCashGoodsGroupInfo info;
				PPGoodsTaxEntry  gtx;
				info.ID = item.Id;
				STRNSCPY(info.Name, goods_rec.Name);
				info.ParentID = item.ParentId;
				info.UnitID  = goods_rec.UnitID;
				if(GObj.FetchTax(goods_rec.ID, getcurdate_(), 0L, &gtx) > 0) // @v10.8.10 LConfig.OperDate-->getcurdate_()
					info.VatRate = gtx.GetVatRate();
				GObj.GetSingleBarcode(goods_rec.ID, 0, temp_buf);
				temp_buf.ShiftLeftChr('@');
				STRNSCPY(info.Code, temp_buf);
				info.Level = level;
				info.P_QuotByQttyList = 0;
				THROW_SL(P_GrpList->insert(&info));
				THROW(MakeGroupList(pTreeList, item.Id, level + 1)); // @recursion
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int AsyncCashGoodsGroupIterator::Init(PPID cashNodeID, long flags, DeviceLoadingStat * pDls, const PPIDArray * pTermGrpList)
{
	int    ok = 1;
	PPID   grp_id = 0;
	SString   temp_buf;
	PPIDArray grp_id_list;
	Goods2Tbl::Rec goods_rec, cn_grp_rec;
	PPObjCashNode  cn_obj;
	const  long gf_exclaltfolt = (GF_FOLDER|GF_EXCLALTFOLD);
	CashNodeID = cashNodeID;
	Flags = flags;
	P_Dls = pDls;
	Pos   = 0;
	QuotByQttyList.freeAll();
	THROW(cn_obj.GetAsync(CashNodeID, &AcnPack) > 0);
	if(AcnPack.GoodsGrpID && GObj.Fetch(AcnPack.GoodsGrpID, &cn_grp_rec) > 0 && (cn_grp_rec.Flags & gf_exclaltfolt) == gf_exclaltfolt) {
		PPIDArray child_list;
		if(GObj.P_Tbl->GetGroupTerminalList(AcnPack.GoodsGrpID, &child_list, 0) > 0 && child_list.getCount()) {
			for(uint i = 0; i < child_list.getCount(); i++) {
				grp_id = child_list.get(i);
				if(GObj.Fetch(grp_id, &goods_rec) > 0)
					grp_id_list.add(grp_id);
			}
		}
	}
	if(!grp_id_list.getCount()) {
		for(GoodsGroupIterator gg_iter(0); gg_iter.Next(&grp_id, temp_buf) > 0;) {
			if(GObj.Fetch(grp_id, &goods_rec) > 0 && !(goods_rec.Flags & GF_ALTGROUP) && (!pTermGrpList || pTermGrpList->lsearch(grp_id))) {
				do {
					grp_id_list.add(grp_id);
					grp_id = goods_rec.ParentID;
				} while(GObj.Fetch(grp_id, &goods_rec) > 0);
			}
		}
	}
	{
		StrAssocArray temp_list;
		grp_id_list.sortAndUndup();
		for(uint p = 0; p < grp_id_list.getCount(); p++) {
			if(GObj.Fetch(grp_id_list.at(p), &goods_rec) > 0) {
				const  PPID par_id = goods_rec.ParentID;
				temp_list.AddFast(goods_rec.ID, (par_id && grp_id_list.bsearch(par_id)) ? par_id : 0, goods_rec.Name);
			}
		}
		temp_list.SortByText();
		THROW_MEM(P_GrpList = new SArray(sizeof(AsyncCashGoodsGroupInfo)));
		THROW(MakeGroupList(&temp_list, 0, 0));
	}
	CATCH
		ZDELETE(P_GrpList);
		ok = 0;
	ENDCATCH
	return ok;
}

int AsyncCashGoodsGroupIterator::Next(AsyncCashGoodsGroupInfo * pInfo)
{
	int    ok = -1;
	if(P_GrpList && Pos < P_GrpList->getCount()) {
		uint   i;
		AsyncCashGoodsGroupInfo info = *static_cast<const AsyncCashGoodsGroupInfo *>(P_GrpList->at(Pos++));
		info.DivN = 1;
		if(AcnPack.Flags & CASHF_EXPDIVN && AcnPack.P_DivGrpList) {
			long   default_div = 1;
			int    use_default_div = 1;
			PPGenCashNode::DivGrpAssc * p_dg_item;
			for(i = 0; AcnPack.P_DivGrpList->enumItems(&i, (void **)&p_dg_item);) {
				if(p_dg_item->GrpID == 0)
					default_div = p_dg_item->DivN;
				else if(info.ID == p_dg_item->GrpID) {
					info.DivN = p_dg_item->DivN;
					use_default_div = 0;
					break;
				}
			}
			if(use_default_div)
				info.DivN = default_div;
		}
		PPQuotArray  quot_list(info.ID);
		QuotByQttyList.freeAll();
		THROW(GObj.P_Tbl->FetchQuotList(info.ID, 0, AcnPack.LocID, quot_list));
		for(i = 0; i < quot_list.getCount(); i++) {
			const PPQuot quot = quot_list.at(i);
			if(quot.MinQtty)
				THROW_SL(QuotByQttyList.insert(&quot));
		}
		if(QuotByQttyList.getCount())
			info.P_QuotByQttyList = &QuotByQttyList;
		ASSIGN_PTR(pInfo, info);
		ok = 1;
	}
	CATCHZOK
	return ok;
}
