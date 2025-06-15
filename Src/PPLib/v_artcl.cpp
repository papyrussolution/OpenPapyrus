// V_ARTCL.CPP
// A.Starodub, A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2024, 2025
// @codepage UTF-8
// Реализация контроллера анализ данных PPViewArticle
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewArticle)
//
IMPLEMENT_PPFILT_FACTORY(Article); ArticleFilt::ArticleFilt() : PPBaseFilt(PPFILT_ARTICLE, 0, 0)
{
	SetFlatChunk(offsetof(ArticleFilt, ReserveStart),
		offsetof(ArticleFilt, Reserve)-offsetof(ArticleFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

ArticleFilt & FASTCALL ArticleFilt::operator = (const ArticleFilt & s)
{
	this->Copy(&s, 1);
	return *this;
}

PPViewArticle::PPViewArticle() : PPView(&ArObj, &Filt, 0, 0, 0), P_TempTbl(0), AgtProp(0), P_DebtDimList(0),
	LimitTerm(0), AddedLimitTerm(0), CtrlX(0), CurIterOrd(ordByName)
{
}

PPViewArticle::~PPViewArticle()
{
	delete P_TempTbl;
}

void * PPViewArticle::GetEditExtraParam() { return const_cast<ArticleFilt *>(ArObj.GetCurrFilt()); /*@badcast*/ }

PPBaseFilt * PPViewArticle::CreateFilt(const void * extraPtr) const
{
	ArticleFilt * p_filt = new ArticleFilt;
	p_filt->AccSheetID = reinterpret_cast<long>(extraPtr);
	return p_filt;
}

int PPViewArticle::UpdateTempTable(PPID arID)
{
	int    ok = -1, r = -1;
	if(P_TempTbl) {
		TempArAgtTbl::Rec rec;
		PPClientAgreement cli_agt;
		PPSupplAgreement suppl_agt;
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(AgtProp == ARTPRP_CLIAGT2) { // @v11.2.0 ARTPRP_CLIAGT-->ARTPRP_CLIAGT2
			if(ArObj.GetClientAgreement(arID, cli_agt, 0) > 0) {
				rec.ArID        = arID;
				rec.Beg = cli_agt.BegDt;
				rec.Expiry      = cli_agt.Expiry;
				rec.DefPayTerm  = cli_agt.DefPayPeriod;
				rec.DefAgentID    = cli_agt.DefAgentID;
				rec.DefQuotKindID = cli_agt.DefQuotKindID;
				rec.Discount      = cli_agt.Dscnt;
				rec.MaxDiscount   = cli_agt.MaxDscnt;
				rec.MaxCredit     = cli_agt.MaxCredit;
				rec.ExtObjectID   = cli_agt.ExtObjectID;
				rec.Flags = cli_agt.Flags;
				STRNSCPY(rec.Code, cli_agt.Code_); // @v11.2.0 cli_agt.Code2-->cli_agt.Code_
				InitDebtLim(&rec, &cli_agt);
				r = 1;
			}
		}
		else if(AgtProp == ARTPRP_SUPPLAGT) {
			if(ArObj.GetSupplAgreement(arID, &suppl_agt, 0) > 0) {
				rec.ArID        = arID;
				rec.Beg = suppl_agt.BegDt;
				rec.Expiry      = suppl_agt.Expiry;
				rec.DefPayTerm  = suppl_agt.DefPayPeriod;
				rec.DefDlvrTerm = suppl_agt.DefDlvrTerm;
				rec.DefAgentID  = suppl_agt.DefAgentID;
				rec.Flags       = suppl_agt.Flags;
				r = 1;
			}
		}
		if(SearchByID_ForUpdate(P_TempTbl, 0, arID, 0) > 0) {
			if(r > 0) {
				THROW_DB(P_TempTbl->updateRecBuf(&rec)); // @sfu
			}
			else {
				THROW_DB(P_TempTbl->deleteRec()); // @sfu
			}
		}
		else {
			THROW_DB(P_TempTbl->insertRecBuf(&rec));
		}
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempArAgt);

int PPViewArticle::InitDebtLim(TempArAgtTbl::Rec * pRec, const PPClientAgreement * pCliAgt)
{
	if(P_DebtDimList && pRec && pCliAgt) {
		uint   lim_count = (P_DebtDimList->getCount() < DEBTDIM_BRW_SHOWCOUNT) ? P_DebtDimList->getCount() : DEBTDIM_BRW_SHOWCOUNT;
		char * p_mc = reinterpret_cast<char *>(&pRec->MaxCredit1);
		for(uint i = 0; i < lim_count; i++) {
			uint   pos = 0;
			long   debt_dim_id = P_DebtDimList->Get(i).Id;
			if(pCliAgt->DebtLimList.lsearch(&debt_dim_id, &pos, CMPF_LONG)) {
				PPClientAgreement::DebtLimit dbt_lim = pCliAgt->DebtLimList.at(pos);
				SETFLAG(pRec->StopFlags, 1 << i, dbt_lim.Flags & PPClientAgreement::DebtLimit::fStop);
				*reinterpret_cast<double *>(p_mc + sizeof(double)*i) = dbt_lim.Limit;
			}
		}
	}
	return 1;
}

int PPViewArticle::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	AgtProp = 0;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	LimitTerm = 0;
	AddedLimitTerm = 0;
	ZDELETE(P_TempTbl);
	ZDELETE(P_DebtDimList);
	if(Filt.Flags & (ArticleFilt::fShowAgreement|ArticleFilt::fCheckObj)) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		AgtProp = 0; // ARTPRP_CLIAGT2, ARTPRP_SUPPLAGT
		if(Filt.Flags & ArticleFilt::fShowAgreement) {
			if(acs_obj.Fetch(Filt.AccSheetID, &acs_rec) > 0) {
				if(acs_rec.Flags & ACSHF_USECLIAGT) {
					PPObjDebtDim obj_dd;
					AgtProp = ARTPRP_CLIAGT2; // @v11.2.0 ARTPRP_CLIAGT-->ARTPRP_CLIAGT2
					P_DebtDimList = obj_dd.MakeStrAssocList(0);
				}
				else if(acs_rec.Flags & ACSHF_USESUPPLAGT)
					AgtProp = ARTPRP_SUPPLAGT;
			}
			if(Filt.Flags & Filt.fShowAddedLimit) {
				PPDebtorStatConfig ds_cfg;
				if(PPDebtorStatConfig::Read(&ds_cfg) > 0 && ds_cfg.LimitAddedTerm > 0 && ds_cfg.LimitTerm > 0) {
					LimitTerm = ds_cfg.LimitTerm;
					AddedLimitTerm = ds_cfg.LimitAddedTerm;
				}
			}
		}
		if(AgtProp) {
			THROW(P_TempTbl = CreateTempFile());
			{
				PropertyTbl::Key0 k0;
				BExtInsert bei(P_TempTbl);
				PropertyTbl & r_pt = PPRef->Prop;
				if(AgtProp == ARTPRP_CLIAGT2) { // @v11.2.0 ARTPRP_CLIAGT-->ARTPRP_CLIAGT2
					PPIDArray _id_list;
					PropertyTbl::Key1 k1;
					BExtQuery q(&r_pt, 1);
					q.select(r_pt.ObjType, r_pt.ObjID, r_pt.Prop, 0L).where(r_pt.ObjType == PPOBJ_ARTICLE && r_pt.Prop == ARTPRP_CLIAGT2);
					MEMSZERO(k1);
					k1.ObjType = PPOBJ_ARTICLE;	
					k1.Prop = ARTPRP_CLIAGT2;
					PPTransaction tra(ppDbDependTransaction, 1);
					THROW(tra);
					for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
						_id_list.addnz(r_pt.data.ObjID);
					}
					if(_id_list.getCount()) {
						_id_list.sortAndUndup();
						PPTransaction tra(ppDbDependTransaction, 1);
						THROW(tra);
						for(uint i = 0; i < _id_list.getCount(); i++) {
							const  PPID _id = _id_list.get(i);
							PPClientAgreement cli_agt;
							if(ArObj.GetClientAgreement(_id, cli_agt) > 0) {
								TempArAgtTbl::Rec rec;
								rec.ArID        = cli_agt.ClientID;
								rec.Beg = cli_agt.BegDt;
								rec.Expiry      = cli_agt.Expiry;
								rec.DefPayTerm  = cli_agt.DefPayPeriod;
								rec.DefAgentID    = cli_agt.DefAgentID;
								rec.DefQuotKindID = cli_agt.DefQuotKindID;
								rec.Discount      = cli_agt.Dscnt;
								rec.MaxDiscount   = cli_agt.MaxDscnt;
								rec.MaxCredit     = cli_agt.MaxCredit;
								rec.ExtObjectID   = cli_agt.ExtObjectID;
								rec.Flags = cli_agt.Flags;
								STRNSCPY(rec.Code, cli_agt.Code_);
								InitDebtLim(&rec, &cli_agt);
								THROW_DB(bei.insert(&rec));
							}
						}
						THROW_DB(bei.flash());
						THROW(tra.Commit());
					}
#if 0 // @v11.2.0 {
					BExtQuery q(&r_pt, 0);
					q.selectAll().where(r_pt.ObjType == PPOBJ_ARTICLE && r_pt.Prop == ARTPRP_CLIAGT);
					MEMSZERO(k0);
					k0.ObjType = PPOBJ_ARTICLE;
					{
						PPTransaction tra(ppDbDependTransaction, 1);
						THROW(tra);
						for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
							TempArAgtTbl::Rec rec;
							PPClientAgreement cli_agt;
							PPObjArticle::PropToClientAgt(&r_pt.data, &cli_agt, 1);
							rec.ArID        = cli_agt.ClientID;
							rec.Beg = cli_agt.BegDt;
							rec.Expiry      = cli_agt.Expiry;
							rec.DefPayTerm  = cli_agt.DefPayPeriod;
							rec.DefAgentID    = cli_agt.DefAgentID;
							rec.DefQuotKindID = cli_agt.DefQuotKindID;
							rec.Discount      = cli_agt.Dscnt;
							rec.MaxDiscount   = cli_agt.MaxDscnt;
							rec.MaxCredit     = cli_agt.MaxCredit;
							rec.ExtObjectID   = cli_agt.ExtObjectID;
							rec.Flags = cli_agt.Flags;
							STRNSCPY(rec.Code, cli_agt.Code_); // @v10.2.9 Code-->Code2 // @v11.2.0 cli_agt.Code2-->cli_agt.Code_
							InitDebtLim(&rec, &cli_agt);
							THROW_DB(bei.insert(&rec));
						}
						THROW_DB(bei.flash());
						THROW(tra.Commit());
					}
#endif // } 0 @v11.2.0
				}
				else if(AgtProp == ARTPRP_SUPPLAGT) {
					//
					// Соглашение с поставщиком может хранится в Property с идентификатором ARTPRP_SUPPLAGT (формат до v8.5.0),
					// и с иденти ARTPRP_SUPPLAGT2 (формат v8.5.0 и выше). По этому пришлось так усложнить выборку соглашений:
					// сначала быстро получаем статьи, которые имеют соглашения, а потом для каждой такой статьи
					// полной функцией PPObjArticle::GetSupplAgreement извлекаем соглашение
					//
					PPIDArray ar_agt_list;
					{
						BExtQuery q(&r_pt, 0);
						q.select(r_pt.ObjID, 0).where(r_pt.ObjType == PPOBJ_ARTICLE && r_pt.Prop == ARTPRP_SUPPLAGT);
						MEMSZERO(k0);
						k0.ObjType = PPOBJ_ARTICLE;
						for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
							ar_agt_list.add(r_pt.data.ObjID);
						}
					}
					{
						BExtQuery q(&r_pt, 0);
						q.select(r_pt.ObjID, 0).where(r_pt.ObjType == PPOBJ_ARTICLE && r_pt.Prop == ARTPRP_SUPPLAGT2);
						MEMSZERO(k0);
						k0.ObjType = PPOBJ_ARTICLE;
						for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
							ar_agt_list.add(r_pt.data.ObjID);
						}
					}
					if(ar_agt_list.getCount()) {
						PPTransaction tra(ppDbDependTransaction, 1);
						THROW(tra);
						ar_agt_list.sortAndUndup();
						PPSupplAgreement suppl_agt;
						for(uint i = 0; i < ar_agt_list.getCount(); i++) {
							const  PPID ar_id = ar_agt_list.get(i);
							if(ArObj.GetSupplAgreement(ar_id, &suppl_agt, 0) > 0) {
								TempArAgtTbl::Rec rec;
								rec.ArID        = suppl_agt.SupplID;
								rec.Beg = suppl_agt.BegDt;
								rec.Expiry      = suppl_agt.Expiry;
								rec.DefPayTerm  = suppl_agt.DefPayPeriod;
								rec.DefDlvrTerm = suppl_agt.DefDlvrTerm;
								rec.DefAgentID  = suppl_agt.DefAgentID;
								rec.Flags       = suppl_agt.Flags;
								THROW_DB(bei.insert(&rec));
							}
						}
						THROW_DB(bei.flash());
						THROW(tra.Commit());
					}
				}
			}
		}
		if(Filt.Flags & ArticleFilt::fCheckObj) {
			PPIDArray id_list;
			ArObj.P_Tbl->GetListBySheet(Filt.AccSheetID, &id_list, 0);
			if(id_list.getCount()) {
				SString msg;
				if(!P_TempTbl)
					THROW(P_TempTbl = CreateTempFile());
				{
					PPTransaction tra(ppDbDependTransaction, 1);
					THROW(tra);
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID ar_id = id_list.get(i);
						ArticleTbl::Rec ar_rec;
						if(ArObj.Search(ar_id, &ar_rec) > 0 && !ArObj.CheckObject(&ar_rec, &msg)) {
							TempArAgtTbl::Rec rec;
							TempArAgtTbl::Key0 k0;
							MEMSZERO(k0);
							k0.ArID = ar_rec.ID;
							if(SearchByKey_ForUpdate(P_TempTbl, 0, &k0, &rec) > 0) {
								msg.CopyTo(rec.Msg, sizeof(rec.Msg));
								THROW_DB(P_TempTbl->updateRecBuf(&rec));
							}
							else {
								rec.Clear();
								rec.ArID = ar_rec.ID;
								msg.CopyTo(rec.Msg, sizeof(rec.Msg));
								THROW_DB(P_TempTbl->insertRecBuf(&rec));
							}
						}
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	ArObj.SetCurrFilt(&Filt);
	CATCH
		ZDELETE(P_TempTbl);
		AgtProp = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewArticle::InitIteration()
{
	int    ok = 1;
	PPAccSheet acs_rec;
	DBQ  * dbq = 0;
	union {
		ArticleTbl::Key0 k0;
		ArticleTbl::Key1 k1;
		ArticleTbl::Key2 k2;
		ArticleTbl::Key3 k3;
	} k, temp_k;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.Order == PPViewArticle::ordByName)
		CurIterOrd = 2;
	else if(Filt.Order == PPViewArticle::ordByArticle)
		CurIterOrd = 1;
	else
		CurIterOrd = 0;
	THROW_MEM(P_IterQuery = new BExtQuery(ArObj.P_Tbl, CurIterOrd));
	P_IterQuery->selectAll();
	if(Filt.PersonID)
		dbq = &(*dbq && ArObj.P_Tbl->ObjID == Filt.PersonID);
	else {
		THROW(SearchObject(PPOBJ_ACCSHEET, Filt.AccSheetID, &acs_rec) > 0);
		dbq = &(*dbq && ArObj.P_Tbl->AccSheetID == acs_rec.ID);
	}
	P_IterQuery->where(*dbq);
	if(Filt.PersonID)
		k.k3.ObjID = Filt.PersonID;
	else if(Filt.Order == PPViewArticle::ordByDefault)
		k.k0.ID = 0;
	else if(Filt.Order == PPViewArticle::ordByArticle)
		k.k1.AccSheetID = acs_rec.ID;
	else
		k.k2.AccSheetID = acs_rec.ID;
	temp_k = k;
	Counter.Init(P_IterQuery->countIterations(0, &temp_k, spGe));
	P_IterQuery->initIteration(false, &k, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewArticle::NextIteration(ArticleViewItem * pItem)
{
	int    ok = -1;
	while(ok < 0 && P_IterQuery && P_IterQuery->nextIteration() > 0) {
		Counter.Increment();
		ArticleTbl::Rec rec;
		ArObj.P_Tbl->copyBufTo(&rec);
		if(Filt.Ft_Closed < 0 && rec.Closed)
			continue;
		if(Filt.Ft_Closed > 0 && !rec.Closed)
			continue;
		if(Filt.Ft_Stop < 0 && rec.Flags & ARTRF_STOPBILL)
			continue;
		if(Filt.Ft_Stop > 0 && !(rec.Flags & ARTRF_STOPBILL))
			continue;
		ASSIGN_PTR(pItem, rec);
		ok = 1;
	}
	return ok;
}

int PPViewArticle::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class ArticleFiltDialog : public TDialog {
		DECL_DIALOG_DATA(ArticleFilt);
	public:
		ArticleFiltDialog() : TDialog(DLG_ARTICLEFLT)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_ARTICLEFLT_ORDER, &Data.Order);
			SetupPPObjCombo(this, CTLSEL_ARTICLEFLT_ACCSID, PPOBJ_ACCSHEET, Data.AccSheetID, 0, 0);
			//disableCtrl(CTLSEL_ARTICLEFLT_ACCSID, true);
			AddClusterAssocDef(CTL_ARTICLEFLT_FTCLOSED, 0, 0);
			AddClusterAssoc(CTL_ARTICLEFLT_FTCLOSED, 1, -1);
			AddClusterAssoc(CTL_ARTICLEFLT_FTCLOSED, 2, +1);
			SetClusterData(CTL_ARTICLEFLT_FTCLOSED, Data.Ft_Closed);

			AddClusterAssocDef(CTL_ARTICLEFLT_FTSTOP, 0, 0);
			AddClusterAssoc(CTL_ARTICLEFLT_FTSTOP, 1, -1);
			AddClusterAssoc(CTL_ARTICLEFLT_FTSTOP, 2, +1);
			SetClusterData(CTL_ARTICLEFLT_FTSTOP, Data.Ft_Stop);

			AddClusterAssoc(CTL_ARTICLEFLT_FLAGS, 0, ArticleFilt::fShowAgreement);
			AddClusterAssoc(CTL_ARTICLEFLT_FLAGS, 1, ArticleFilt::fCheckObj);
			AddClusterAssoc(CTL_ARTICLEFLT_FLAGS, 2, ArticleFilt::fShowAddedLimit);
			if(Data.Flags & ArticleFilt::fShowAgreement)
				DisableClusterItem(CTL_ARTICLEFLT_FLAGS, 2, 0);
			else {
				Data.Flags &= ~ArticleFilt::fShowAddedLimit;
				DisableClusterItem(CTL_ARTICLEFLT_FLAGS, 2, 1);
			}
			SetClusterData(CTL_ARTICLEFLT_FLAGS, Data.Flags);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			long   temp_long = 0;
			getCtrlData(sel = CTLSEL_ARTICLEFLT_ACCSID, &Data.AccSheetID);
			THROW_PP(Data.AccSheetID, PPERR_ACCSHEETNEEDED);
			getCtrlData(CTL_ARTICLEFLT_ORDER, &Data.Order);
			GetClusterData(CTL_ARTICLEFLT_FTCLOSED, &temp_long);
			Data.Ft_Closed = (int16)temp_long;
			GetClusterData(CTL_ARTICLEFLT_FTSTOP, &temp_long);
			Data.Ft_Stop = (int16)temp_long;
			GetClusterData(CTL_ARTICLEFLT_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_ARTICLEFLT_FLAGS)) {
				long flags = GetClusterData(CTL_ARTICLEFLT_FLAGS);
				if(flags & ArticleFilt::fShowAgreement)
					DisableClusterItem(CTL_ARTICLEFLT_FLAGS, 2, 0);
				else {
					flags &= ~ArticleFilt::fShowAddedLimit;
					DisableClusterItem(CTL_ARTICLEFLT_FLAGS, 2, 1);
				}
				if(flags != Data.Flags) {
					SetClusterData(CTL_ARTICLEFLT_FLAGS, Data.Flags = flags);
				}
			}
			else
				return;
			clearEvent(event);
		}
	};
	int    ok = -1;
	ArticleFiltDialog * dlg = 0;
	ArticleFilt * p_filt = 0;
	THROW(Filt.IsA(pBaseFilt));
	p_filt = static_cast<ArticleFilt *>(pBaseFilt);
	THROW(ArObj.CheckRights(PPR_READ));
	THROW(CheckDialogPtr(&(dlg = new ArticleFiltDialog)));
	dlg->setDTS(p_filt);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(p_filt)) {
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewArticle::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		ArticleViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_ARTICLE, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewArticle::RecoverLinkObjects()
{
	int    ok = -1;
	if(Filt.AccSheetID) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Search(Filt.AccSheetID, &acs_rec) > 0) {
			if(acs_rec.Assoc == PPOBJ_LOCATION) {
				PPObjLocation loc_obj;
				ArticleViewItem item;
				for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
					if(item.ObjID) {
						LocationTbl::Rec loc_rec;
						int    r = loc_obj.Search(item.ObjID, &loc_rec);
						THROW(r);
						if(r < 0) {
							PPID   id = 0;
							MEMSZERO(loc_rec);
							loc_rec.ID = item.ObjID;
							loc_rec.Type = LOCTYP_WAREHOUSE;
							STRNSCPY(loc_rec.Name, item.Name);
							THROW(loc_obj.PutRecord(&id, &loc_rec, 1));
							ok = 1;
						}
						else if(r > 0 && (loc_rec.Name[0] == 0 || loc_rec.Type == 0)) {
							PPID   id = item.ObjID;
							loc_rec.Type = LOCTYP_WAREHOUSE;
							STRNSCPY(loc_rec.Name, item.Name);
							THROW(loc_obj.PutRecord(&id, &loc_rec, 1));
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewArticle::EditLinkObject(PPID arID)
{
	int    ok = -1;
	ArticleTbl::Rec rec;
	if(ArObj.Fetch(arID, &rec) > 0 && rec.ObjID && rec.AccSheetID) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Fetch(rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc) {
			ok = EditPPObj(acs_rec.Assoc, rec.ObjID);
		}
	}
	return ok;
}

int PPViewArticle::EditDebtDimList(PPID arID)
{
	int    ok = -1;
	ArticleTbl::Rec ar_rec;
	if(ArObj.Search(arID, &ar_rec) > 0) {
		int    agt_kind = -1;
		THROW(agt_kind = ArObj.GetAgreementKind(&ar_rec));
		if(agt_kind == 1) {
			PPClientAgreement cli_agt_rec;
			THROW(ArObj.GetClientAgreement(ar_rec.ID, cli_agt_rec));
			cli_agt_rec.ClientID = ar_rec.ID;
			if(EditDebtLimList(cli_agt_rec) > 0) {
				THROW(ArObj.PutClientAgreement(ar_rec.ID, &cli_agt_rec, 1));
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

struct ArMassUpdParam {
	ArMassUpdParam();
	enum {
		aUndef  = 0,
		aUpdate = 1,
		aRemoveAll
	};
	enum {
		fCreateAgreement = 0x0001
	};
	long   Action;
	long   Flags;
	long   PayPeriod;
	long   DeliveryPeriod;
};

ArMassUpdParam::ArMassUpdParam() : Action(aUpdate), Flags(0), PayPeriod(0), DeliveryPeriod(0)
{
}

class ArMassUpdDialog : public TDialog {
	DECL_DIALOG_DATA(ArMassUpdParam);
public:
	ArMassUpdDialog() : TDialog(DLG_ARMASSUPD)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		AddClusterAssocDef(CTL_ARMASSUPD_WHAT, 0, ArMassUpdParam::aUpdate);
		AddClusterAssocDef(CTL_ARMASSUPD_WHAT, 1, ArMassUpdParam::aRemoveAll);
		SetClusterData(CTL_ARMASSUPD_WHAT, Data.Action);
		AddClusterAssoc(CTL_ARMASSUPD_FLAGS, 0, ArMassUpdParam::fCreateAgreement);
		SetClusterData(CTL_ARMASSUPD_FLAGS, Data.Flags);
		setCtrlLong(CTL_ARMASSUPD_PAYPERIOD, Data.PayPeriod);
		setCtrlLong(CTL_ARMASSUPD_DELIVERY, Data.DeliveryPeriod);
		SetupCtrls();
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_ARMASSUPD_WHAT,  &Data.Action);
		GetClusterData(CTL_ARMASSUPD_FLAGS, &Data.Flags);
		if(Data.Action == ArMassUpdParam::aUpdate) {
			Data.PayPeriod = getCtrlLong(sel = CTL_ARMASSUPD_PAYPERIOD);
			THROW_PP(Data.PayPeriod >= 0 && Data.PayPeriod <= 20*365, PPERR_USERINPUT);
			Data.DeliveryPeriod = getCtrlLong(sel = CTL_ARMASSUPD_DELIVERY);
			THROW_PP(Data.DeliveryPeriod >= 0 && Data.DeliveryPeriod <= 366, PPERR_USERINPUT);
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_ARMASSUPD_WHAT)) {
			GetClusterData(CTL_ARMASSUPD_WHAT,  &Data.Action);
			SetupCtrls();
		}
	}
	void    SetupCtrls()
	{
		disableCtrl(CTL_ARMASSUPD_FLAGS, (Data.Action == ArMassUpdParam::aRemoveAll));
		disableCtrl(CTL_ARMASSUPD_PAYPERIOD, (Data.Action == ArMassUpdParam::aRemoveAll));
		disableCtrl(CTL_ARMASSUPD_DELIVERY,  (Data.Action == ArMassUpdParam::aRemoveAll));
	}
};

int PPViewArticle::UpdateAll()
{
	int    ok = -1;
	ArMassUpdParam param;
	PPIDArray id_list;
	ArticleViewItem item;
	THROW(ArObj.CheckRights(ARTRT_MULTUPD));
	for(InitIteration(); NextIteration(&item) > 0;) {
		id_list.add(item.ID);
	}
	id_list.sortAndUndup();
	const uint cnt = id_list.getCount();
	if(cnt) {
		if(PPDialogProcBody<ArMassUpdDialog, ArMassUpdParam>(&param) > 0) {
			PPLogger logger;
			SString fmt_buf;
			SString msg_buf;
			if(param.Action == ArMassUpdParam::aUpdate) {
				if(param.PayPeriod > 0 || param.DeliveryPeriod > 0) {
					THROW(ArObj.CheckRights(PPR_MOD));
					{
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; i < cnt; i++) {
							const  PPID _id = id_list.get(i);
							PPArticlePacket pack;
							if(_id && ArObj.GetPacket(_id, &pack) > 0) {
								int    do_update = 0;
								const int agt_kind = PPObjArticle::GetAgreementKind(&pack.Rec);
								if(agt_kind == 1) { // client agreements
									if(param.PayPeriod > 0) {
										if(pack.P_CliAgt) {
											if(pack.P_CliAgt->DefPayPeriod != param.PayPeriod) {
												pack.P_CliAgt->DefPayPeriod = (int16)param.PayPeriod;
												do_update = 1;
											}
										}
										else if(param.Flags & ArMassUpdParam::fCreateAgreement) {
											PPClientAgreement new_agt;
											new_agt.DefPayPeriod = (int16)param.PayPeriod;
											pack.SetClientAgreement(&new_agt, 1);
											do_update = 2;
										}
									}
								}
								else if(agt_kind == 2) { // suppl agreements
									if(pack.P_SupplAgt) {
										if(pack.P_SupplAgt->DefPayPeriod != param.PayPeriod) {
											pack.P_SupplAgt->DefPayPeriod = (int16)param.DeliveryPeriod;
											do_update = 1;
										}
										if(pack.P_SupplAgt->DefDlvrTerm != param.DeliveryPeriod) {
											pack.P_SupplAgt->DefDlvrTerm = (int16)param.DeliveryPeriod;
											do_update = 1;
										}
									}
									else if(param.Flags & ArMassUpdParam::fCreateAgreement) {
										PPSupplAgreement new_agt;
										if(param.PayPeriod > 0)
											new_agt.DefPayPeriod = (int16)param.PayPeriod;
										if(param.DeliveryPeriod > 0)
											new_agt.DefDlvrTerm = (int16)param.DeliveryPeriod;
										pack.SetSupplAgreement(&new_agt, 1);
										do_update = 2;
									}
								}
								if(do_update) {
									PPID   temp_id = _id;
									if(ArObj.PutPacket(&temp_id, &pack, 0)) {
										//PPTXT_ARMASSUPD_UPD               "Изменена статья аналитического учета '@zstr'"
										//PPTXT_ARMASSUPD_UPD_CRAGT         "Изменена статья аналитического учета '@zstr' (создано соглашение)"
										if(do_update == 2) {
											logger.Log(PPFormatT(PPTXT_ARMASSUPD_UPD_CRAGT, &msg_buf, pack.Rec.Name));
										}
										else {
											logger.Log(PPFormatT(PPTXT_ARMASSUPD_UPD, &msg_buf, pack.Rec.Name));
										}
										ok = 1;
									}
									else
										logger.LogLastError();
								}
							}
							PPWaitPercent(i+1, cnt);
						}
						THROW(tra.Commit());
					}
				}
			}
			else if(param.Action == ArMassUpdParam::aRemoveAll) {
				THROW(ArObj.CheckRights(PPR_DEL));
				{
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < cnt; i++) {
						const  PPID _id = id_list.get(i);
						if(_id) {
							if(!ArObj.RemoveObjV(_id, 0, 0, 0)) {
								logger.LogLastError();
							}
							else
								ok = 1;
						}
						PPWaitPercent(i+1, cnt);
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewArticle::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_DELETEALL:
				ok = UpdateAll();
				break;
			case PPVCMD_ADDGROUP:
				ok = ArObj.EditGrpArticle(&(id = 0), Filt.AccSheetID);
				break;
			case PPVCMD_EDTILINKOBJ:
				ok = EditLinkObject(id);
				break;
			case PPVCMD_EDITAGREEMENT:
				ok = ArObj.EditAgreement(id);
				if(ok > 0)
					UpdateTempTable(id);
				break;
			case PPVCMD_EDITDEBTLIMS:
				ok = EditDebtDimList(id);
				if(ok > 0)
					UpdateTempTable(id);
				break;
			case PPVCMD_VIEWQUOT:
				ok = -1;
				if(id) {
					ArticleTbl::Rec ar_rec;
					if(ArObj.Search(id, &ar_rec) > 0) {
						QuotFilt quot_flt;
						quot_flt.ArID = id;
						quot_flt.QkCls = (PPObjArticle::GetAgreementKind(&ar_rec) == 2) ? PPQuot::clsSupplDeal : PPQuot::clsGeneral;
						ViewQuot(&quot_flt);
					}
				}
				break;
			case PPVCMD_VIEWBILLS:
				ok = -1;
				if(id) {
					ArticleTbl::Rec ar_rec;
					if(ArObj.Search(id, &ar_rec) <= 0)
						ar_rec.Clear();
					BillFilt bill_flt;
					bill_flt.AccSheetID = ar_rec.AccSheetID;
					bill_flt.ObjectID   = id;
					bill_flt.Bbt        = bbtRealTypes;
					bill_flt.Flags      = BillFilt::fSetupNewBill;
					::ViewGoodsBills(&bill_flt, true/*modeless*/);
				}
				break;
			case PPVCMD_VIEWGOODS:
				ok = -1;
				if(id) {
					GoodsFilt filt;
					filt.Flags |= GoodsFilt::fShowArCode;
					filt.CodeArID = id;
					PPView::Execute(PPVIEW_GOODS, &filt, 1, 0);
				}
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit(id);
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID   acs_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&acs_id) && Filt.AccSheetID != acs_id) {
						Filt.AccSheetID = acs_id;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_INPUTCHAR:
				if(static_cast<const char *>(pHdr)[0] == kbCtrlX)
					CtrlX++;
				else
					CtrlX = 0;
				if(CtrlX == 2) {
					ok = RecoverLinkObjects();
					CtrlX = 0;
				}
				break;
		}
	}
	else if(ok > 0 && pHdr && *static_cast<const  PPID *>(pHdr)) {
		UpdateTempTable(*static_cast<const  PPID *>(pHdr));
	}
	return ok;
}

int PPViewArticle::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_ACCSHEET, Filt.AccSheetID, 0, 0);
	return -1;
}

DBQuery * PPViewArticle::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	static DbqStringSubst stop_subst(2);  // @global @threadsafe
	DBQuery * p_q = 0;
	ArticleTbl   * a  = 0;
	ReferenceTbl * rf = 0;
	TempArAgtTbl * tt = 0;
	DBE    dbe_agent;
	DBE    dbe_quotkind;
	DBE    dbe_extobj;
	DBE    dbe_supple; // @vmiller
	DBE    cq, cq_stop[DEBTDIM_BRW_SHOWCOUNT];
	DBE    alim, alim_dim[DEBTDIM_BRW_SHOWCOUNT];
	uint   brw_id = BROWSER_ARTICLE;

	if(ArObj.P_Tbl) {
		DBE  * dbe_stop = 0;
		DBQ  * dbq = 0;
		PPAccSheet acs_rec;
		THROW(ArObj.CheckRights(PPR_READ));
		THROW(CheckTblPtr(a = new ArticleTbl(ArObj.P_Tbl->GetName())));
		dbe_stop = & flagtoa(a->Flags, ARTRF_STOPBILL, stop_subst.Get(PPTXT_AR_STOP));
		if(Filt.PersonID) {
			THROW(CheckTblPtr(rf = new ReferenceTbl));
			p_q = & select(
				a->ID,       // #00
				a->Article,  // #01
				a->Name,     // #02
				a->Closed,   // #03
				*dbe_stop,   // #04
				rf->ObjName, // #05
				0L).from(rf, a, 0L);
			dbq = &(rf->ObjType == PPOBJ_ACCSHEET && rf->ObjID == a->AccSheetID && a->ObjID == Filt.PersonID);
		}
		else {
			THROW(SearchObject(PPOBJ_ACCSHEET, Filt.AccSheetID, &acs_rec) > 0);
			p_q = & select(
				a->ID,       // #00
				a->Article,  // #01
				a->Name,     // #02
				a->Closed,   // #03
				*dbe_stop,   // #04
				a->ID,       // #05 @stub
				0L).from(a, 0L);
			dbq = &(a->AccSheetID == acs_rec.ID);
		}
		if(P_TempTbl) {
			SString fld_name;
			if(AgtProp == ARTPRP_CLIAGT2) // @v11.2.0 ARTPRP_CLIAGT-->ARTPRP_CLIAGT2
				brw_id = BROWSER_ARTICLE_AGTCLI;
			else if(AgtProp == ARTPRP_SUPPLAGT)
				brw_id = BROWSER_ARTICLE_AGTSUPPL;
			THROW(CheckTblPtr(tt = new TempArAgtTbl(P_TempTbl->GetName())));
			PPDbqFuncPool::InitObjNameFunc(dbe_agent, PPDbqFuncPool::IdObjNameAr, tt->DefAgentID);
			PPDbqFuncPool::InitObjNameFunc(dbe_quotkind, PPDbqFuncPool::IdObjNameQuotKind, tt->DefQuotKindID);
			PPDbqFuncPool::InitObjNameFunc(dbe_extobj, PPDbqFuncPool::IdObjNameAr, tt->ExtObjectID);
			PPDbqFuncPool::InitObjNameFunc(dbe_supple, PPDbqFuncPool::IdGetAgrmntSymbol, tt->ArID); // @vmiller
			/*
			for(uint i = 0; i < DEBTDIM_BRW_SHOWCOUNT; i++)
				PPDbqFuncPool::InitObjNameFunc(dbe_dbt_dim[i], PPDbqFuncPool::IdObjNameDebtDim, *(&tt->DebtDimID1 + sizeof(DBField)*i));
			*/
			p_q->addField(tt->Beg);         // #06
			p_q->addField(tt->Expiry);      // #07
			p_q->addField(tt->DefPayTerm);  // #08
			p_q->addField(tt->DefDlvrTerm); // #09
			p_q->addField(dbe_agent);       // #10
			p_q->addField(tt->Discount);    // #11
			p_q->addField(tt->MaxDiscount); // #12
			p_q->addField(tt->MaxCredit);   // #13
			p_q->addField(dbe_quotkind);    // #14
			p_q->addField(tt->Code);        // #15
			p_q->addField(dbe_extobj);      // #16
			{
				cq.init();
				cq.push(tt->Flags);
				DBConst dbc_long;
				dbc_long.init(AGTF_AUTOORDER);
				cq.push(dbc_long);
				dbc_long.init("");
				cq.push(dbc_long);
				cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdYesWordByFlag));
				p_q->addField(cq);          // #17
			}
			if(Filt.Flags & ArticleFilt::fCheckObj)
				p_q->addField(tt->Msg);     // #18
			else
				p_q->addField(tt->Code);    // #18 @stub @v8.2.4
			if(LimitTerm && AddedLimitTerm) {
				alim.init();
				alim.push(tt->MaxCredit);
				DBConst dbc_long;
				dbc_long.init(LimitTerm);
				alim.push(dbc_long);
				dbc_long.init(AddedLimitTerm);
				alim.push(dbc_long);
				alim.push(static_cast<DBFunc>(PPDbqFuncPool::IdAddedCreditLimit));
				p_q->addField(alim);          // #19
			}
			else {
				p_q->addField(tt->MaxCredit); // #19 @stub
			}
			p_q->addField(dbe_supple);        // #20 // @vmiller // @v8.3.6 @fix #19-->#20
			for(uint i = 0; i < DEBTDIM_BRW_SHOWCOUNT; i++) {
				DBConst dbc_long;
				DBField field;
				(fld_name = "MaxCredit").Cat((long)i+1);
				tt->getFieldByName(fld_name, &field);
				p_q->addField(field);
				if(LimitTerm && AddedLimitTerm) {
					alim_dim[i].init();
					alim_dim[i].push(field);
					DBConst dbc_long;
					dbc_long.init(LimitTerm);
					alim_dim[i].push(dbc_long);
					dbc_long.init(AddedLimitTerm);
					alim_dim[i].push(dbc_long);
					alim_dim[i].push(static_cast<DBFunc>(PPDbqFuncPool::IdAddedCreditLimit));
					p_q->addField(alim_dim[i]);
				}
				else {
					p_q->addField(field); // @stub
				}
				{
					cq_stop[i].init();
					cq_stop[i].push(tt->StopFlags);
					dbc_long.init((long)(0x00000001 << i));
					cq_stop[i].push(dbc_long);
					dbc_long.init("X\0");
					cq_stop[i].push(dbc_long);
					cq_stop[i].push(static_cast<DBFunc>(PPDbqFuncPool::IdYesWordByFlag));
					p_q->addField(cq_stop[i]);
				}
			}
			p_q->addTable(tt);
			dbq = &(*dbq && (tt->ArID += a->ID));
		}
		delete dbe_stop;
		dbq = ppcheckflag(dbq, a->Flags, ARTRF_STOPBILL, Filt.Ft_Stop);
		if(Filt.Ft_Closed > 0)
			dbq = &(*dbq && a->Closed > 0L);
		else if(Filt.Ft_Closed < 0)
			dbq = &(*dbq && a->Closed == 0L);
		p_q->where(*dbq);
		if(Filt.Order == PPViewArticle::ordByArticle)
			p_q->orderBy(a->AccSheetID, a->Article, 0L);
		else if(Filt.Order == PPViewArticle::ordByName)
			p_q->orderBy(a->AccSheetID, a->Name, 0L);
		else
			p_q->orderBy(a->ID, 0L);
	}
	THROW(CheckQueryPtr(p_q));
	if(pSubTitle)
		GetObjectName(PPOBJ_ACCSHEET, Filt.AccSheetID, *pSubTitle);
	CATCH
		if(p_q)
			ZDELETE(p_q);
		else {
			delete a;
			delete rf;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

void PPViewArticle::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.PersonID) {
			pBrw->InsColumn(2, "@accsheet", 5, 0L, MKSFMTD(20, 0, 0), 0);
		}
		if(Filt.Flags & ArticleFilt::fCheckObj) {
			pBrw->InsColumn(-1, "@message", 18, 0L, MKSFMT(40, 0), 0);
		}
		/*
		if(AgtProp == ARTPRP_SUPPLAGT)
			pBrw->SetCellStyleFunc(CellStyleFunc, this);
		*/
		uint   beg_dim_col = 16;
		if(LimitTerm && AddedLimitTerm) {
			pBrw->InsColumn(14, "@addcreditlimit", 19, 0L, MKSFMTD(12, 2, ALIGN_RIGHT|NMBF_NOZERO), 0);
			beg_dim_col++;
		}
		if(P_DebtDimList) {
			uint   count = (P_DebtDimList->getCount() < DEBTDIM_BRW_SHOWCOUNT) ? P_DebtDimList->getCount() : DEBTDIM_BRW_SHOWCOUNT;
			uint   beg_fld_pos = 21;
			const  int col_in_grp = (LimitTerm && AddedLimitTerm) ? 3 : 2;
			for(uint i = 0, pos = beg_fld_pos; i < count; i++) {
				const StrAssocArray::Item entry = P_DebtDimList->Get(i);
				BroGroup grp;
				grp.First = beg_dim_col + (i * col_in_grp);
				grp.Count = col_in_grp;
				grp.Height = 1;
				grp.P_Text = newStr(entry.Txt);
				pBrw->getDef()->AddColumnGroup(&grp);
				pBrw->InsColumn(-1, "@creditlimit",  pos++, 0L, MKSFMTD(12, 2, ALIGN_RIGHT|NMBF_NOZERO), 0);
				if(LimitTerm && AddedLimitTerm)
					pBrw->InsColumn(-1, "@addcreditlimit",  pos, 0L, MKSFMTD(12, 2, ALIGN_RIGHT|NMBF_NOZERO), 0);
				pos++;
				pBrw->InsColumn(-1, "@stop", pos++, 0L, MKSFMT(3, 0), 0);
			}
		}
	}
}

int PPViewArticle::Print(const void * pHdr)
{
	uint  rpt_id = 0;
	if(AgtProp == ARTPRP_CLIAGT2) // @v11.2.0 ARTPRP_CLIAGT-->ARTPRP_CLIAGT2
		rpt_id = REPORT_ARTCLVIEWWCLIAGT;
	else if(AgtProp == ARTPRP_SUPPLAGT)
		rpt_id = REPORT_ARTCLVIEWWSPPLAGT;
	else
		rpt_id = REPORT_ARTCLVIEW;
	return Helper_Print(rpt_id, Filt.Order);
}
//
//
//
int ViewArticle(const ArticleFilt * pFilt) { return PPView::Execute(PPVIEW_ARTICLE, pFilt, PPView::exefModeless, 0); }
//
// Implementation of PPALDD_ArticleView
//
PPALDD_CONSTRUCTOR(ArticleView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ArticleView) { Destroy(); }

int PPALDD_ArticleView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Article, rsrv);
	SString temp_buf;
	GetObjectName(PPOBJ_ACCSHEET, p_filt->AccSheetID, temp_buf.Z());
	STRNSCPY(H.FltAccSheetName, temp_buf);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ArticleView::InitIteration(PPIterID iterId, int sortId, long)
{
	INIT_PPVIEW_ALDD_ITER(Article);
}

int PPALDD_ArticleView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Article);
	I.ArticleID = item.ID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_ArticleView::Destroy() { DESTROY_PPVIEW_ALDD(Article); }
