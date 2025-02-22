// V_PERSON.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
#include <ppsoapclient.h>

static void DrawClientActivityStatistics(PPID psnID) // @v12.2.6 @construction
{
	PPObjPerson psn_obj;
	PPObjPerson::ClientActivityStatistics cas;
	TSVector <uint16> date_list_;
	SString temp_buf;
	int    ok = psn_obj.ReadClientActivityStatistics(psnID, cas, &date_list_);
	// gnuplot line: set arrow from 4.5, graph 0 to 4.5, graph 3 nohead
	if(ok > 0) {
		if(date_list_.getCount() >= 7) {
			StatBase date_gap_stat;
			LongArray gap_list;
			SHistogram hg;
			hg.SetupDynamic(0.0, 1.0);
			for(uint i = 0; i < date_list_.getCount(); i++) {
				if(i) {
					const LDATE prev_dt = plusdate(cas.FirstEventDt, date_list_.at(i-1));
					const LDATE dt = plusdate(cas.FirstEventDt, date_list_.at(i));
					long _gap = diffdate(dt, prev_dt);
					assert(_gap > 0);
					gap_list.add(_gap);
					date_gap_stat.Step(static_cast<double>(_gap));
					hg.Put(static_cast<double>(_gap));
				}
			}
			//
			date_gap_stat.Finish();
			double _gap_days_avg = date_gap_stat.GetExp();
			double _gap_days_stddev = date_gap_stat.GetStdDev();
			RealRange range_x;
			range_x.low = DBL_MAX;
			range_x.upp = -DBL_MAX;
			{
				Generator_GnuPlot plot(0);
				Generator_GnuPlot::PlotParam param;
				param.Flags |= Generator_GnuPlot::PlotParam::fHistogram;
				plot.StartData(0/*putLegend*/);
				param.Legend.Add(1, "value");
				for(uint hi = 0; hi < hg.GetResultCount(); hi++) {
					SHistogram::Result hr;
					hg.GetResult(hi, &hr);
					range_x.SetupMinMax(hr.Low);
					temp_buf.Z().Cat(hr.Low, MKSFMTD(0, 0, 0));
					plot.PutData(temp_buf, 1); // #1
					plot.PutData(hr.Count);    // #2
					plot.PutEOR();
				}
				plot.PutEndOfData();
				plot.Preamble();
				plot.SetGrid();
				plot.SetStyleFill("solid");
				{
					const double avg = cas.GapDaysAvg;
					const double left_stddev = avg-cas.GapDaysStdDev;
					const double right_stddev = avg+cas.GapDaysStdDev;
					range_x.SetupMinMax(left_stddev);
					range_x.SetupMinMax(right_stddev);
					{
						Generator_GnuPlot::Coord from_x(Generator_GnuPlot::Coord::csFirst, avg);
						Generator_GnuPlot::Coord from_y(Generator_GnuPlot::Coord::csGraph, 0.0);
						Generator_GnuPlot::Coord to_x(Generator_GnuPlot::Coord::csFirst, avg);
						Generator_GnuPlot::Coord to_y(Generator_GnuPlot::Coord::csGraph, 1.0);
						plot.SetArrow(from_x, from_y, to_x, to_y, Generator_GnuPlot::arrhNoHead);
					}
					{
						Generator_GnuPlot::Coord from_x(Generator_GnuPlot::Coord::csFirst, left_stddev);
						Generator_GnuPlot::Coord from_y(Generator_GnuPlot::Coord::csGraph, 0.0);
						Generator_GnuPlot::Coord to_x(Generator_GnuPlot::Coord::csFirst, left_stddev);
						Generator_GnuPlot::Coord to_y(Generator_GnuPlot::Coord::csGraph, 1.0);
						plot.SetArrow(from_x, from_y, to_x, to_y, Generator_GnuPlot::arrhNoHead);
					}
					{
						Generator_GnuPlot::Coord from_x(Generator_GnuPlot::Coord::csFirst, right_stddev);
						Generator_GnuPlot::Coord from_y(Generator_GnuPlot::Coord::csGraph, 0.0);
						Generator_GnuPlot::Coord to_x(Generator_GnuPlot::Coord::csFirst, right_stddev);
						Generator_GnuPlot::Coord to_y(Generator_GnuPlot::Coord::csGraph, 1.0);
						plot.SetArrow(from_x, from_y, to_x, to_y, Generator_GnuPlot::arrhNoHead);
					}
				}
				plot.SetAxisRange(Generator_GnuPlot::axX, range_x.low-1.0, range_x.upp+1.0);
				plot.Plot(&param);
				ok = plot.Run();
				// set arrow from 0.5, graph 0 to 0.5, graph 3 nohead
			}
		}
	}
}

int ViewPersonInfoBySCard(const char * pCode)
{
	int    ok = -1;
	SCardTbl::Rec scard_rec;
	PPObjSCard obj_scard;
	if(obj_scard.SearchCode(0, pCode, &scard_rec) > 0 && scard_rec.PersonID > 0) {
		PersonTbl::Rec psn_rec;
		PPObjPerson obj_person;
		if(obj_person.Search(scard_rec.PersonID, &psn_rec) > 0) {
			SString img_path, buf, word;
			COLORREF msg_color = GetColorRef(SClrLightgreen);
			if(psn_rec.Flags & PSNF_HASIMAGES) {
				ObjLinkFiles link_files(PPOBJ_PERSON);
				link_files.Load(psn_rec.ID, 0L);
				link_files.At(0, img_path);
			}
			buf.CR().Cat(PPLoadStringS("card", word)).CatDiv(':', 2).Space().Cat(scard_rec.Code);
			buf.CR().Cat(PPLoadStringS("rest", word)).CatDiv(':', 2).Space().Cat(scard_rec.Rest);
			buf.CR().Cat(PPLoadStringS("name", word)).CatDiv(':', 2).Space().Cat(psn_rec.Name);
			if(scard_rec.Expiry) {
				PPSCardConfig sc_cfg;
				if(PPObjSCard::FetchConfig(&sc_cfg) > 0 && sc_cfg.WarnExpiryBefore > 0) {
					const long _diff = diffdate(scard_rec.Expiry, getcurdate_());
					if(_diff <= sc_cfg.WarnExpiryBefore) {
						PPLoadText(PPTXT_SCARDWILLEXPIRESOON, word);
                        buf.CR().Cat(word).Space().Cat(scard_rec.Expiry);
                        msg_color = GetColorRef(SClrOrange);
					}
				}
			}
			PPTooltipMessage(buf, img_path, 0/*pBrw->hWnd*/, 5000, 0, SMessageWindow::fTextAlignLeft|
				SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow|
				SMessageWindow::fLargeText|SMessageWindow::fShowOnCenter|SMessageWindow::fPreserveFocus);
			ok = 1;
		}
	}
	return ok;
}
//
//
//
PPViewPerson::PPViewPerson() : PPView(&PsnObj, &Filt, PPVIEW_PERSON, implUseQuickTagEditFunc, 0), 
	DefaultTagID(0), P_TempPsn(0), P_Fr(0), P_ClientActivityStateList(0) // @v11.2.8 0-->&PsnObj; implUseQuickTagEditFunc
{
}

PPViewPerson::~PPViewPerson()
{
	delete P_TempPsn;
	delete P_Fr;
	delete P_ClientActivityStateList; // @v12.2.2
}

PP_CREATE_TEMP_FILE_PROC(CreateTempPersonFile, TempPerson);

bool PPViewPerson::IsTempTblNeeded()
{
	bool   yes = false;
	if(Filt.GetAttribType() || Filt.P_RegF || Filt.P_TagF || Filt.P_SjF || (Filt.Flags & PersonFilt::fTagsCrsstab))
		yes = true;
	else {
		SString temp_buf;
		Filt.GetExtssData(PersonFilt::extssNameText, temp_buf);
		if(temp_buf.NotEmptyS())
			yes = true;
		else {
			Filt.GetExtssData(PersonFilt::extssEmailText, temp_buf);
			if(temp_buf.NotEmptyS())
				yes = true;
		}
		/* @v12.2.2 if(!yes) {
			if(Filt.NewCliPeriod.low && Filt.Flags & Filt.fNewClientsOnly && PsnObj.GetConfig().ClientActivityDetectionList.getCount())
				yes = true;
		}*/
	}
	return yes;
}

/*virtual*/PPBaseFilt * PPViewPerson::CreateFilt(const void * extraPtr) const
{
	PersonFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_PERSON, reinterpret_cast<PPBaseFilt **>(&p_filt))) {
		int speciality = reinterpret_cast<int>(extraPtr);
		if(speciality == PersonFilt::spcClientActivityStats) {
			p_filt->Flags |= PersonFilt::fCliActivityStats;
		}
	}
	return static_cast<PPBaseFilt *>(p_filt);	
}

/*virtual*/int PPViewPerson::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	const  int use_ta = 1;
	const  LDATE now_date = getcurdate_();
	SString srch_buf;
	SString msg_buf;
	SString temp_buf;
	LocationTbl::Rec loc_rec;
	// @v12.2.6 замещено более общим функционалом анализа клиентской активности PPNewContragentDetectionBlock ncd_blk;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempPsn);
	ZDELETE(P_Ct);
	NewCliList.Clear();
	StrPool.ClearS();
	ZDELETE(P_ClientActivityStateList); // @v12.2.2
	PsnObj.DirtyConfig(); // @v12.2.4
	// @v12.2.3 {
	{
		const LDATE actual_date = checkdate(Filt.ClientActivityEvalDate) ? Filt.ClientActivityEvalDate : now_date;
		assert(checkdate(actual_date));
		Filt.ClientActivityEvalDate = actual_date;
		Filt.NewCliPeriod.Actualize(actual_date);
	}
	// } @v12.2.3 
	if(IsTempTblNeeded()) {
		IterCounter cntr;
		PersonTbl * pt = PsnObj.P_Tbl;
		PersonKindTbl * kt = & PsnObj.P_Tbl->Kind;
		THROW(P_TempPsn = CreateTempPersonFile());
		{
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			if(oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
				PersonTbl::Rec owner_rec;
				PPIDArray dlvr_loc_list;
				UintHashTable id_list;
				if(Filt.GetAttribType() == PPPSNATTR_HANGEDADDR) {
					PROFILE_START
					PersonTbl::Key0 k0;
					BExtQuery q(pt, 0);
					q.select(pt->ID, pt->MainLoc, pt->RLoc, 0);
					MEMSZERO(k0);
					for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
						id_list.AddNZ(pt->data.MainLoc);
						id_list.AddNZ(pt->data.RLoc);
						dlvr_loc_list.clear();
						if(PsnObj.GetDlvrLocList(pt->data.ID, &dlvr_loc_list) > 0) {
							for(uint i = 0; i < dlvr_loc_list.getCount(); i++)
								id_list.Add(dlvr_loc_list.get(i));
						}
					}
					{
						LAssocArray dlvr_addr_list;
						BillObj->P_Tbl->GetDlvrAddrList(&dlvr_addr_list);
						for(uint i = 0; i < dlvr_addr_list.getCount(); i++)
							id_list.Add(dlvr_addr_list.at(i).Val);
					}
					PROFILE_END
				}
				if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR && Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
					PROFILE_START
					PPIDArray sj_id_list;
					SysJournal * p_sj = DS.GetTLA().P_SysJ;
					Filt.P_SjF->Period.Actualize(ZERODATE);
					THROW(p_sj->GetObjListByEventPeriod(PPOBJ_LOCATION, Filt.P_SjF->UserID,
						&Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, sj_id_list));
					for(uint i = 0; i < sj_id_list.getCount(); i++) {
						const  PPID loc_id = sj_id_list.get(i);
						if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0 && loc_rec.Flags & LOCF_STANDALONE) {
							PsnAttrViewItem vi;
							if(CreateAddrRec(loc_rec.ID, &loc_rec, "standalone", &vi) > 0) {
								if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &owner_rec) > 0)
									STRNSCPY(vi.Name, owner_rec.Name);
								THROW_DB(P_TempPsn->insertRecBuf(&vi));
							}
						}
					}
					PROFILE_END
				}
				else {
					PROFILE_START
					for(SEnum en = PsnObj.LocObj.P_Tbl->Enum(LOCTYP_ADDRESS, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
						int do_insert = 0;
						if(Filt.GetAttribType() == PPPSNATTR_HANGEDADDR && !(loc_rec.Flags & LOCF_STANDALONE) && !id_list.Has(loc_rec.ID)) {
							if(PsnObj.LocObj.Fetch(loc_rec.ID, &loc_rec) > 0)
								do_insert = 1;
						}
						else if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR && loc_rec.Flags & LOCF_STANDALONE) {
							// LocObj.P_Tbl->Enum не заполняет строковый "хвост": придется повторить извлечение записи
							if(PsnObj.LocObj.Fetch(loc_rec.ID, &loc_rec) > 0)
								do_insert = 2;
						}
						if(do_insert) {
							PsnAttrViewItem vi;
							if(CreateAddrRec(loc_rec.ID, &loc_rec, (loc_rec.Flags & LOCF_STANDALONE) ? "standalone" : "unknown", &vi) > 0) {
								if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &owner_rec) > 0) {
									STRNSCPY(vi.Name, owner_rec.Name);
								}
								THROW_DB(P_TempPsn->insertRecBuf(&vi));
							}
						}
						PPWaitMsg(ideqvalstr(loc_rec.ID, msg_buf.Z()));
					}
					PROFILE_END
				}
			}
			else {
				if(Filt.Flags & PersonFilt::fTagsCrsstab)
					ObjTag.EnumItems(&DefaultTagID);
				else
					DefaultTagID = 0;
				PPIDArray id_list, local_list;
				int    use_list = 0;
				if(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
					SysJournal * p_sj = DS.GetTLA().P_SysJ;
					local_list.clear();
					Filt.P_SjF->Period.Actualize(ZERODATE);
					THROW(p_sj->GetObjListByEventPeriod(PPOBJ_PERSON, Filt.P_SjF->UserID, &Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, local_list));
					if(use_list)
						id_list.intersect(&local_list);
					else {
						id_list = local_list;
						use_list = 1;
					}
				}
				if(Filt.P_RegF) {
					local_list.clear();
					PsnObj.RegObj.SearchByFilt(Filt.P_RegF, 0, &local_list);
					if(use_list)
						id_list.intersect(&local_list);
					else {
						id_list = local_list;
						use_list = 1;
					}
				}
				if(Filt.PersonKindID) {
					local_list.clear();
					PersonKindTbl::Key0 k0, k0_;
					MEMSZERO(k0);
					k0.KindID = Filt.PersonKindID;
					BExtQuery kq(kt, 0);
					kq.select(kt->PersonID, 0L).where(kt->KindID == Filt.PersonKindID);
					k0_ = k0;
					cntr.Init(kq.countIterations(0, &k0_, spGe));
					for(kq.initIteration(false, &k0, spGe); kq.nextIteration() > 0; cntr.Increment()) {
						local_list.add(kt->data.PersonID);
						PPWaitPercent(cntr);
					}
					if(use_list)
						id_list.intersect(&local_list);
					else {
						id_list = local_list;
						use_list = 1;
					}
				}
				if(use_list) {
					Filt.GetExtssData(PersonFilt::extssNameText, srch_buf);
					if(Filt.PersonCategoryID || Filt.Status || srch_buf.NotEmptyS()) {
						local_list.clear();
						for(uint i = 0; i < id_list.getCount(); i++) {
							const  PPID id = id_list.get(i);
							PersonTbl::Rec rec;
							if(PsnObj.Fetch(id, &rec) > 0) {
								if((!Filt.PersonCategoryID || rec.CatID == Filt.PersonCategoryID) && (!Filt.Status || rec.Status == Filt.Status)) {
									if(srch_buf.NotEmptyS()) {
										if(Filt.Flags & PersonFilt::fPrecName) {
											if(srch_buf.CmpNC(rec.Name) == 0)
												local_list.add(id);
										}
										else {
											if(ExtStrSrch(rec.Name, srch_buf, 0))
												local_list.add(id);
										}
									}
									else
										local_list.add(id);
								}
							}
						}
						id_list = local_list;
					}
				}
				else {
					union {
						PersonTbl::Key0 k0;
						PersonTbl::Key1 k1;
						PersonTbl::Key2 k2;
					} k, k_;
					MEMSZERO(k);
					Filt.GetExtssData(PersonFilt::extssNameText, srch_buf);
					if(srch_buf.NotEmptyS() && Filt.Flags & PersonFilt::fPrecName) {
						srch_buf.CopyTo(k.k1.Name, sizeof(k.k1.Name));
						if(pt->search(1, &k, spEq)) do {
							if((!Filt.PersonCategoryID || pt->data.CatID == Filt.PersonCategoryID) && (!Filt.Status || pt->data.Status == Filt.Status)) {
								local_list.add(pt->data.ID);
							}
						} while(pt->search(1, &k, spNext) && srch_buf.CmpNC(pt->data.Name) == 0);
					}
					else {
						BExtQuery pq(pt, Filt.PersonCategoryID ? 2 : 0);
						DBQ * dbq = 0;
						pq.select(pt->ID, pt->Name, pt->Status, pt->MainLoc, pt->Flags, pt->RLoc, 0L);
						dbq = ppcheckfiltid(dbq, pt->CatID, Filt.PersonCategoryID);
						dbq = ppcheckfiltid(dbq, pt->Status, Filt.Status);
						pq.where(*dbq);
						if(Filt.PersonCategoryID) {
							k.k2.CatID = Filt.PersonCategoryID;
						}
						k_ = k;
						cntr.Init(pq.countIterations(0, &k_, spGe));
						local_list.clear();
						for(pq.initIteration(false, &k, spGe); pq.nextIteration() > 0; cntr.Increment()) {
							if(srch_buf.IsEmpty() || ExtStrSrch(pt->data.Name, srch_buf, 0))
								local_list.add(pt->data.ID);
							PPWaitPercent(cntr);
						}
					}
					if(use_list)
						id_list.intersect(&local_list);
					else {
						id_list = local_list;
						use_list = 1;
					}
				}
				{
					UintHashTable used_loc_list;
					UintHashTable * p_used_loc_list = (Filt.GetAttribType() == PPPSNATTR_ALLADDR && Filt.Flags & PersonFilt::fShowHangedAddr) ? &used_loc_list : 0;
					assert(use_list);
					id_list.sortAndUndup();
					cntr.Init(id_list.getCount());
					for(uint i = 0; i < id_list.getCount(); i++) {
						const  PPID person_id = id_list.get(i);
						bool   skip = false;
						/* @v12.2.2 (замещено более общим функционалом анализа клиентской активности) if(Filt.NewCliPeriod.low) {
							if(ncd_blk.IsNewPerson(person_id, Filt.NewCliPeriod))
								NewCliList.Add(person_id);
							else if(Filt.Flags & Filt.fNewClientsOnly)
								skip = true;
						}*/
						if(!skip) {
							THROW(AddTempRec(person_id, p_used_loc_list, 0));
						}
						PPWaitPercent(cntr.Increment());
					}
					if(p_used_loc_list) {
						for(SEnum en = PsnObj.LocObj.P_Tbl->Enum(LOCTYP_ADDRESS, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
							if(!p_used_loc_list->Has(static_cast<ulong>(loc_rec.ID))) {
								PsnAttrViewItem vi;
								if(CreateAddrRec(loc_rec.ID, 0, "@haddress", &vi) > 0) {
									THROW_DB(P_TempPsn->insertRecBuf(&vi));
								}
							}
						}
					}
				}
			}
			THROW(tra.Commit());
		}
		{
			class PersonTagsCrosstab : public Crosstab {
			public:
				explicit PersonTagsCrosstab(PPViewPerson * pV) : Crosstab(), P_V(pV)
				{
				}
				virtual BrowserWindow * CreateBrowser(uint brwId, int dataOwner)
				{
					PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
					SetupBrowserCtColumns(p_brw);
					return p_brw;
				}
			protected:
				virtual void GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
				{
					if(pVal && P_V) 
						P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
				}
				PPViewPerson * P_V;
			};
			if((Filt.Flags & PersonFilt::fTagsCrsstab) && P_TempPsn) {
				THROW_MEM(P_Ct = new PersonTagsCrosstab(this));
				P_Ct->SetTable(P_TempPsn, P_TempPsn->TabID);
				P_Ct->AddIdxField(P_TempPsn->ID);
				P_Ct->AddInheritedFixField(P_TempPsn->Name);
				//P_Ct->AddInheritedFixField(P_TempPsn->RAddressP); // @erik добавить позже   
				P_Ct->AddAggrField(P_TempPsn->RegNumber);
				THROW(P_Ct->Create(use_ta));
			}
		}
	}
	/* @v12.2.2 (замещено более общим функционалом анализа клиентской активности)
	else if(Filt.NewCliPeriod.low && !oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
		PersonViewItem item;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(ncd_blk.IsNewPerson(item.ID, Filt.NewCliPeriod)) {
				NewCliList.Add(item.ID);
			}
			PPWaitPercent(GetCounter());
		}
		PPWaitStop();
	}*/
	// @v12.2.2 {
	if(Filt.Flags & PersonFilt::fCliActivityStats) {
		assert(P_ClientActivityStateList == 0); // @see top of function
		PersonViewItem item;
		PPWaitStart();
		LAssocArray local_client_activity_state_list;
		for(InitIteration(); NextIteration(&item) > 0;) {
			PPObjPerson::ClientActivityState cas;
			cas.PersonID = item.ID;
			cas.ActualDate = Filt.ClientActivityEvalDate;
			cas.NewCliPeriod = Filt.NewCliPeriod;
			PsnObj.IdentifyClientActivityState(cas);
			local_client_activity_state_list.Add(item.ID, cas.State);
			PPWaitPercent(GetCounter());
		}
		//
		// Отложенная инициализация P_ClientActivityStateList из-за того, что метод NextIteration фильтрует
		// записи опираясь на P_ClientActivityStateList.
		//
		THROW_MEM(P_ClientActivityStateList = new LAssocArray(local_client_activity_state_list));
		PPWaitStop();		
	}
	// } @v12.2.2 
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_TempPsn);
		ok = 0;
	ENDCATCH
	return ok;
}

void PPViewPerson::GetTabTitle(long tabID, SString & rBuf) const { GetObjectName(PPOBJ_TAG, tabID, rBuf.Z()); }
int  PPViewPerson::ViewRelations(PPID id) { return PsnObj.EditRelationList(id); }
int  PPViewPerson::AddRelation(PPID id) { return PsnObj.EditRelation(&id, 0, 0); }

int PPViewPerson::UpdateHungedAddr(PPID addrID)
{
	int    ok = 1;
	LocationTbl::Rec loc_rec;
	{
		PsnAttrViewItem vi;
		PersonTbl::Rec owner_rec;
		PPTransaction tra(1);
		THROW(tra);
		if(PsnObj.LocObj.Search(addrID, &loc_rec) > 0 && CreateAddrRec(loc_rec.ID, &loc_rec, (loc_rec.Flags & LOCF_STANDALONE) ? "standalone" : "unknown", &vi) > 0) {
			if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &owner_rec) > 0) {
				STRNSCPY(vi.Name, owner_rec.Name);
			}
			{
				TempPersonTbl::Rec rec;
				TempPersonTbl::Key0 k0;
				MEMSZERO(k0);
				k0.ID = 0;
				k0.TabID = addrID;
				if(SearchByKey_ForUpdate(P_TempPsn, 0, &k0, &rec) > 0) {
					THROW_DB(P_TempPsn->updateRecBuf(&vi)); // @sfu
				}
			}
		}
		else {
			THROW_DB(deleteFrom(P_TempPsn, 0, (P_TempPsn->ID == 0L && P_TempPsn->TabID == addrID)));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*virtual*/void * PPViewPerson::GetEditExtraParam()
{
	void * p_result = 0;
	if(Filt.PersonKindID) {
		p_result = reinterpret_cast<void *>(Filt.PersonKindID);
	}
	return p_result;
}

int PPViewPerson::AddItem(PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(PsnObj.Edit(&id, reinterpret_cast<void *>(Filt.PersonKindID)) == cmOK) {
		AddTempRec(id, 0, 1);
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	return ok;
}

int PPViewPerson::EditItem(PPID id)
{
	int    ok = -1;
	if(PsnObj.Edit(&id, reinterpret_cast<void *>(Filt.PersonKindID)) == cmOK) {
		EditTempRec(id, 1);
		ok = 1;
	}
	return ok;
}

int PPViewPerson::OpenClientDir(PPID PersonId)
{
	int    ok = 0;
	SString cl_dir_name = PsnObj.GetConfig().TopFolder;
	if(cl_dir_name.NotEmptyS()) {
		ok = 0;
		PPObjPersonKind pk_obj;
		PPPersonPacket pack;
		PsnObj.GetPacket(PersonId,  &pack, 0);
		uint   kind_pos = 0;
		//
		// @todo Цикл ужасный - переделать
		//
		while(!ok && kind_pos < pack.Kinds.getCount()) {
			PPPersonKind kind_rec;
			PPID   id = 0;
			while(!ok && pk_obj.EnumItems(&id, &kind_rec) > 0)
				if(pack.Kinds.at(kind_pos) == kind_rec.ID)
					for(uint regs_pos = 0; !ok && regs_pos < pack.Regs.getCount(); regs_pos++) {
						RegisterTbl::Rec & r_reg = pack.Regs.at(regs_pos);
						if(r_reg.RegTypeID == kind_rec.FolderRegTypeID) {
							THROW_SL(SFile::CreateDir(cl_dir_name.SetLastSlash().Cat(r_reg.Num)));
							ok = 1;
						}
					}
			kind_pos++;
		}
		if(ok) {
			ok = 0;
			TCHAR full_path_to_expl[MAX_PATH];
			full_path_to_expl[0] = 0;
			if(GetWindowsDirectory(full_path_to_expl, sizeof(full_path_to_expl))) {
				SString final_path;
				final_path = SUcSwitch(full_path_to_expl);
				final_path.SetLastSlash().Cat("explorer.exe");
				//setLastSlash(full_path_to_expl);
				//strcat(full_path_to_expl, "explorer.exe");
				spawnl(_P_NOWAIT, /*full_path_to_expl*/final_path, cl_dir_name.cptr(), cl_dir_name.cptr(), 0);
				ok = 1;
			}
			//HKEY expl_key;
			//DWORD size_of_path = 512;
			//if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			//	"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
			//	0, KEY_QUERY_VALUE, &expl_key) == ERROR_SUCCESS &&
			//	RegQueryValueEx(expl_key, "SystemRoot", NULL,
			//	NULL, (LPBYTE)full_path_to_expl, &size_of_path) == ERROR_SUCCESS) {
			//		setLastSlash(full_path_to_expl);
			//		strcat(full_path_to_expl, "explorer.exe");
			//		spawnl(_P_NOWAIT, full_path_to_expl, cl_dir_name, cl_dir_name, 0);
			//		ok = 1;
			//}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPerson::EditRegs(PPID personID, PPID locID, int oneReg)
{
	int    ok = -1;
	int    r = 0;
	ObjTagCore & r_ot = PPRef->Ot;
	bool do_update_temp_rec = false;
	PPPersonPacket pack;
	THROW(PsnObj.RegObj.CheckRights(PPR_READ));
	THROW(PsnObj.GetPacket(personID, &pack, 0) > 0);
	if(oneReg) {
		uint   pos = 0;
		if(Filt.Flags & PersonFilt::fLocTagF && Filt.GetAttribType() && locID) {
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Search(locID, &loc_rec) > 0) {
				if(Filt.GetAttribType() == PPPSNATTR_REGISTER) {
					RegisterArray reg_list;
					PsnObj.RegObj.P_Tbl->GetByLocation(locID, &reg_list);
					if(reg_list.GetRegister(Filt.RegTypeID, &pos, 0) > 0) {
						r = PsnObj.RegObj.EditDialog(&reg_list.at(pos-1), &pack.Regs, &pack);
					}
					else {
						RegisterTbl::Rec rec;
						THROW(PPObjRegister::InitPacket(&rec, Filt.RegTypeID, PPObjID(PPOBJ_LOCATION, locID), 0));
						do {
							r = PsnObj.RegObj.EditDialog(&rec, &reg_list, &pack);
							if(r > 0 && PsnObj.RegObj.CheckUnique(rec.RegTypeID, &reg_list)) {
								THROW_SL(reg_list.insert(&rec));
							}
							else if(r >= 0)
								r = PPErrorZ();
						} while(r == 0);
					}
					if(r > 0) {
						THROW(r = PsnObj.RegObj.P_Tbl->PutByLocation(locID, &reg_list, 1));
						if(r > 0) {
							do_update_temp_rec = true;
							THROW(EditTempRec(personID, 1));
							r = -1; // prevent updating person packet below
							ok = 1;
						}
					}
				}
				else if(Filt.GetAttribType() == PPPSNATTR_TAG) {
					ObjTagItem tag_item;
					const PPID tag_id = Filt.RegTypeID - TAGOFFSET;
					if(r_ot.GetTag(PPOBJ_LOCATION, locID, tag_id, &tag_item) > 0 || tag_item.Init(tag_id)) {
						if(EditObjTagItem(PPOBJ_LOCATION, locID, &tag_item, 0) > 0) {
							if(r_ot.PutTag(PPOBJ_LOCATION, locID, &tag_item, 1)) {
								do_update_temp_rec = true;
								ok = 1;
							}
							else
								PPError();
						}
					}
				}
				else {
					if(PsnObj.LocObj.Edit(&locID, 0) > 0) {
						ok = 1;
						do_update_temp_rec = true;
					}
				}
			}
		}
		else if(Filt.GetAttribType() == PPPSNATTR_REGISTER) {
			if(pack.Regs.GetRegister(Filt.RegTypeID, &pos, 0) > 0)
				r = PsnObj.RegObj.EditDialog(&pack.Regs.at(pos - 1), &pack.Regs, &pack);
			else {
				RegisterTbl::Rec rec;
				THROW(PPObjRegister::InitPacket(&rec, Filt.RegTypeID, PPObjID(PPOBJ_PERSON, pack.Rec.ID), 0));
				do {
					r = PsnObj.RegObj.EditDialog(&rec, &pack.Regs, &pack);
					if(r > 0 && PsnObj.RegObj.CheckUnique(rec.RegTypeID, &pack.Regs)) {
						THROW_SL(pack.Regs.insert(&rec));
					}
					else if(r >= 0)
						r = PPErrorZ();
				} while(r == 0);
			}
		}
	}
	else {
		r = PsnObj.RegObj.EditList(&pack, 0);
	}
	if(r > 0) {
		THROW(PsnObj.RegObj.CheckRights(PPR_MOD));
		THROW(PsnObj.PutPacket(&personID, &pack, 1));
		do_update_temp_rec = true;
		ok = 1;
	}
	if(do_update_temp_rec) {
		THROW(EditTempRec(personID, 1));
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPerson::EditDlvrAddrExtFlds(PPID id)
{
	int    ok = -1;
	LocationTbl::Rec loc_rec;
	THROW(PsnObj.LocObj.CheckRightsModByID(&id));
	if(PsnObj.LocObj.Search(id, &loc_rec) > 0 && EditDlvrAddrExtFields(&loc_rec))
		THROW(PsnObj.LocObj.PutRecord(&id, &loc_rec, 1));
	CATCHZOK
	return ok;
}

struct UpdPersonListParam {
	UpdPersonListParam() : Action(acnAcqKind), ExtObj(0)
	{
	}
	enum {
		acnAcqKind      = 1,
		acnRevokeKind   = 2,
		acnRemoveAll    = 3,
		acnGenerateUUID = 4
	};
	long    Action;
	PPID    ExtObj;
};

class UpdPersonListParamDialog : public TDialog {
	typedef UpdPersonListParam DlgDataType;
	DlgDataType Data;
public:
	UpdPersonListParamDialog() : TDialog(DLG_UPDPLIST)
	{
	}
	int setDTS(const DlgDataType * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		AddClusterAssocDef(CTL_UPDPLIST_WHAT, 0, Data.acnAcqKind);
		AddClusterAssocDef(CTL_UPDPLIST_WHAT, 1, Data.acnRevokeKind);
		AddClusterAssocDef(CTL_UPDPLIST_WHAT, 2, Data.acnGenerateUUID);
		AddClusterAssocDef(CTL_UPDPLIST_WHAT, 3, Data.acnRemoveAll);
		SetClusterData(CTL_UPDPLIST_WHAT, Data.Action);
		{
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			DisableClusterItem(CTL_UPDPLIST_WHAT, 2, (tag_obj.Fetch(PPTAG_PERSON_UUID, &tag_rec) <= 0));
		}
		SetupControls(0);
		return ok;
	}
	int getDTS(DlgDataType * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_UPDPLIST_WHAT, &Data.Action);
		switch(Data.Action) {
			case UpdPersonListParam::acnAcqKind:
			case UpdPersonListParam::acnRevokeKind:
				getCtrlData(sel = CTLSEL_UPDPLIST_EXTOBJ, &Data.ExtObj);
				THROW_PP(Data.ExtObj && SearchObject(PPOBJ_PERSONKIND, Data.ExtObj, 0) > 0, PPERR_PSNKINDNEEDED);
				THROW_PP(Data.ExtObj != PPPRK_MAIN || PPMaster, PPERR_UNCHANGABLEPERSONKIND);
				break;
			case UpdPersonListParam::acnGenerateUUID:
				break;
			case UpdPersonListParam::acnRemoveAll:
				{
					SString answ;
					SString yes_str;
					getCtrlString(sel = CTL_UPDPLIST_YES, answ);
					PPLoadString("yes", yes_str);
					THROW_PP(stricmp866(answ, yes_str) == 0, PPERR_STRICTCONFIRMTEXTIGNORED);
				}
				break;
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_UPDPLIST_WHAT)) {
			const long preserve_acn = Data.Action;
			GetClusterData(CTL_UPDPLIST_WHAT, &Data.Action);
			if(Data.Action != preserve_acn) {
				SetupControls(1);
			}
			clearEvent(event);
		}
		else
			return;
	}
	void SetupControls(int initZero)
	{
		SString label_buf;
		if(oneof2(Data.Action, Data.acnAcqKind, Data.acnRevokeKind)) {
			PPLoadString("psnkind", label_buf);
			if(initZero)
				Data.ExtObj = 0;
			SetupPPObjCombo(this, CTLSEL_UPDPLIST_EXTOBJ, PPOBJ_PERSONKIND, Data.ExtObj, 0);
			disableCtrl(CTL_UPDPLIST_YES, 1);
		}
		if(Data.Action == Data.acnGenerateUUID) {
			disableCtrl(CTL_UPDPLIST_YES, 1);
		}
		else {
			PPLoadString("updpersonlist_extobj", label_buf);
			disableCtrl(CTL_UPDPLIST_YES, 0);
		}
		setLabelText(CTL_UPDPLIST_EXTOBJ, label_buf);
	}
};

int PPViewPerson::UpdateList()
{
	int    ok = -1;
	UpdPersonListParam param;
	THROW(PsnObj.CheckRights(PSNRT_MULTUPD));
	if(PPDialogProcBody <UpdPersonListParamDialog, UpdPersonListParam>(&param) > 0) {
		if(oneof4(param.Action, UpdPersonListParam::acnAcqKind, UpdPersonListParam::acnRevokeKind, UpdPersonListParam::acnRemoveAll, UpdPersonListParam::acnGenerateUUID)) {
			PersonViewItem item;
			PPIDArray id_list;
			PPLogger logger;
			PPWaitStart();
			for(InitIteration(); NextIteration(&item) > 0;) {
				id_list.add(item.ID);
			}
			id_list.sortAndUndup();
			const uint cnt = id_list.getCount();
			if(cnt) {
				switch(param.Action) {
					case UpdPersonListParam::acnAcqKind:
						{
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < cnt; i++) {
								const  PPID _id = id_list.get(i);
								if(!PsnObj.P_Tbl->AddKind(_id, param.ExtObj, 0)) {
									logger.LogLastError();
								}
								PPWaitPercent(i+1, cnt);
							}
							THROW(tra.Commit());
						}
						break;
					case UpdPersonListParam::acnRevokeKind:
						{
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < cnt; i++) {
								const  PPID _id = id_list.get(i);
								if(!PsnObj.P_Tbl->RemoveKind(_id, param.ExtObj, 0)) {
									logger.LogLastError();
								}
								PPWaitPercent(i+1, cnt);
							}
							THROW(tra.Commit());
						}
						break;
					case UpdPersonListParam::acnGenerateUUID:
						{
							Reference * p_ref = PPRef;
							const  PPID obj_type = PPOBJ_PERSON;
							const  PPID tag_id = PPTAG_PERSON_UUID;
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < cnt; i++) {
								const  PPID _id = id_list.get(i);
								S_GUID uuid;
								if(p_ref->Ot.GetTagGuid(obj_type, _id, tag_id, uuid) < 0) {
									uuid.Generate();
									ObjTagItem tag_item;
									if(!tag_item.SetGuid(tag_id, &uuid)) 
										logger.LogLastError();
									else if(!p_ref->Ot.PutTag(obj_type, _id, &tag_item, 0))
										logger.LogLastError();
								}
								PPWaitPercent(i+1, cnt);
							}
							THROW(tra.Commit());
						}
						break;
					case UpdPersonListParam::acnRemoveAll:
						THROW(PsnObj.CheckRights(PPR_DEL));
						{
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < cnt; i++) {
								const  PPID _id = id_list.get(i);
								if(!PsnObj.RemoveObjV(_id, 0, 0, 0)) {
									logger.LogLastError();
								}
								PPWaitPercent(i+1, cnt);
							}
							THROW(tra.Commit());
						}
						break;
				}
			}
			PPWaitStop();
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPerson::DeleteItem(PPID id)
{
	int    ok = -1;
	if(id) {
		if(PsnObj.RemoveObjV(id, 0, PPObject::rmv_default, 0) > 0) {
			if(P_TempPsn)
				THROW_DB(deleteFrom(P_TempPsn, 1, (P_TempPsn->ID == id)));
			ok = 1;
		}
	}
	else {
		THROW(PsnObj.CheckRights(PSNRT_MULTUPD));
		THROW(PsnObj.CheckRights(PPR_DEL));
		if(CONFIRMCRIT(PPCFM_DELALLPERSON)) {
			PersonViewItem item;
			PPIDArray id_list;
			PPLogger logger;
			PPWaitStart();
			for(InitIteration(); NextIteration(&item) > 0;) {
				id_list.addUnique(item.ID);
			}
			{
				const uint cnt = id_list.getCount();
				if(cnt) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < cnt; i++) {
						const  PPID _id = id_list.get(i);
						if(!PsnObj.RemoveObjV(_id, 0, PPObject::no_wait_indicator, 0)) {
							logger.LogLastError();
						}
						PPWaitPercent(i+1, cnt);
					}
					THROW(tra.Commit());
				}
			}
			PPWaitStop();
		}
	}
	CATCHZOKPPERR
	return ok;
}

void PPViewPerson::ViewTotal()
{
	uint   dlg_id = P_ClientActivityStateList ? DLG_PERSONTOTAL_CA : DLG_PERSONTOTAL;
	TDialog * dlg = new TDialog(dlg_id);
	if(CheckDialogPtrErr(&dlg)) {
		long   count = 0;
		long   cas_nodata = 0;
		long   cas_reg = 0;
		long   cas_delayed = 0;
		long   cas_hldelayed = 0;
		long   cas_new = 0;
		PersonViewItem item;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0;) {
			count++;
			if(P_ClientActivityStateList) {
				uint   pos = 0;
				if(P_ClientActivityStateList->Search(item.ID, &pos)) {
					const long state = P_ClientActivityStateList->at(pos).Val;
					switch(state & ~PPObjPerson::ClientActivityState::stfNewClient) {
						case PPObjPerson::ClientActivityState::stNoData: cas_nodata++; break;
						case PPObjPerson::ClientActivityState::stDelayedTa: cas_delayed++; break;
						case PPObjPerson::ClientActivityState::stHopelesslyDelayedTa: cas_hldelayed++; break;
						case PPObjPerson::ClientActivityState::stRegularTa: cas_reg++; break;
					}
					if(state & PPObjPerson::ClientActivityState::stfNewClient)
						cas_new++;
				}
			}
			PPWaitPercent(GetCounter());
		}
		PPWaitStop();
		dlg->setCtrlLong(CTL_PERSONTOTAL_COUNT, count);
		dlg->setCtrlLong(CTL_PERSONTOTAL_CA_ND, cas_nodata);
		dlg->setCtrlLong(CTL_PERSONTOTAL_CA_R, cas_reg);
		dlg->setCtrlLong(CTL_PERSONTOTAL_CA_D, cas_delayed);
		dlg->setCtrlLong(CTL_PERSONTOTAL_CA_L, cas_hldelayed);
		dlg->setCtrlLong(CTL_PERSONTOTAL_CA_N, cas_new);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewPerson::ViewTasks(PPID id)
{
	if(id) {
		int    is_client = (Filt.PersonKindID == PPPRK_CLIENT) ? 1 : (Filt.PersonKindID == PPPRK_EMPL) ? 0 : -1; // -1 - undef kind
		if(is_client < 0) {
			PPPerson  psn_pack;
			if(PsnObj.P_Tbl->Get(id, &psn_pack) > 0 && psn_pack.Kinds.lsearch(PPPRK_EMPL))
				is_client = 0;
		}
		PrjTaskFilt  pt_flt;
		pt_flt.Kind = TODOKIND_TASK;
		if(is_client)
			pt_flt.ClientID = id;
		else
			pt_flt.EmployerID = id;
		ViewPrjTask(&pt_flt);
		return 1;
	}
	return -1;
}

int PPViewPerson::ViewPersonEvents(PPID id)
{
	int    ok = -1;
	if(id) {
		PersonEventFilt flt;
		flt.PrmrID = id;
		PPView::Execute(PPVIEW_PERSONEVENT, &flt, PPView::exefModeless, 0);
	}
	return ok;
}

int PPViewPerson::Transmit(PPID id, int transmitKind)
{
	int    ok = -1;
	if(transmitKind == 0) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PersonViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWaitStart();
			for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
				objid_ary.Add(PPOBJ_PERSON, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	else if(transmitKind == 1) {
		PPIDArray id_list;
		id_list.add(id);
		THROW(SendCharryObject(PPDS_CRRPERSON, id_list));
	}
	else if(transmitKind == 2) {
		PPPersonPacket pack;
		if(id && PsnObj.GetPacket(id, &pack, 0)) {
			SString    fname, path, temp_buf;
			SlVCard::Rec rec;
			SlVCard      vcard;
			fname.Cat(id).DotCat("vcf");
			PPGetFilePath(PPPATH_OUT, fname, path);
			THROW(vcard.Open(path, 1));
			rec.Name = pack.Rec.Name;
			// GetMainOrgName(rec.Org);
			{  // Phones, Fax's, Emails
				uint i = 0;
				pack.ELA.GetPhones(10, temp_buf);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf))
						rec.WorkPhone = temp_buf;
					if(ss.get(&i, temp_buf))
						rec.HomePhone = temp_buf;
					if(ss.get(&i, temp_buf))
						rec.MobilePhone = temp_buf;
				}
				pack.ELA.GetPhones(10, temp_buf, ELNKRT_FAX);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf))
						rec.WorkFax = temp_buf;
					if(ss.get(&i, temp_buf))
						rec.HomeFax = temp_buf;
				}
				pack.ELA.GetPhones(10, temp_buf, ELNKRT_EMAIL);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf))
						rec.Email1 = temp_buf;
					if(ss.get(&i, temp_buf))
						rec.Email2 = temp_buf;
				}
			}
			{ // Addrs
				PPID country_id = 0;
				SString buf, zip;
				WorldTbl::Rec city_rec;
				LocationTbl::Rec loc;
				PPObjWorld obj_world;
				loc = pack.RLoc;
				LocationCore::GetExField(&pack.RLoc, LOCEXSTR_FULLADDR, temp_buf.Z());
				if(!temp_buf.Len())
					LocationCore::GetExField(&pack.RLoc, LOCEXSTR_SHORTADDR, temp_buf);
				if(!temp_buf.Len()) {
					LocationCore::GetExField(&pack.Loc, LOCEXSTR_FULLADDR, temp_buf);
					loc = pack.Loc;
				}
				if(!temp_buf.Len())
					LocationCore::GetExField(&pack.Loc, LOCEXSTR_SHORTADDR, temp_buf);
				if(!temp_buf.Len()) {
					if(pack.RLoc.CityID != 0)
						loc = pack.RLoc;
					else
						loc = pack.Loc;
				}
				temp_buf.ReplaceChar(';', ' ');
				temp_buf.Semicol();
				if(obj_world.Fetch(loc.CityID, &city_rec) > 0) {
					(buf = city_rec.Name).ReplaceChar(';', ' ');
					temp_buf.Cat(buf);
					country_id = city_rec.CountryID;
				}
				temp_buf.Semicol();
				if(city_rec.ParentID && obj_world.Fetch(city_rec.ParentID, &city_rec) > 0 && city_rec.Kind == WORLDOBJ_REGION) {
					(buf = city_rec.Name).ReplaceChar(';', ' ');
					temp_buf.Cat(buf);
				}
				temp_buf.Semicol();
				LocationCore::GetExField(&loc, LOCEXSTR_ZIP, zip);
				temp_buf.Cat(zip).Semicol();
				if(country_id && obj_world.Fetch(country_id, &city_rec) > 0 && city_rec.Kind == WORLDOBJ_COUNTRY) {
					(buf = city_rec.Name).ReplaceChar(';', ' ');
					temp_buf.Cat(buf);
				}
				buf.Z().CatCharN(';', 2).Cat(temp_buf);
				temp_buf = buf;
				rec.WorkAddr = temp_buf;
			}
			THROW(vcard.Put(&rec));
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewPerson::CheckIDForFilt(PPID id, const PersonTbl::Rec * pRec)
{
	int    ok = 1;
	PersonTbl::Rec _rec;
	if(pRec || PsnObj.Search(id, &_rec) > 0) {
		SETIFZ(pRec, &_rec);
		if(!CheckFiltID(Filt.Status, pRec->Status))
			ok = 0;
		else if((Filt.Flags & PersonFilt::fVatFree) && !(pRec->Flags & PSNF_NOVATAX))
			ok = 0;
		else if((Filt.Flags & PersonFilt::fHasImages) && !(pRec->Flags & PSNF_HASIMAGES))
			ok = 0;
		else if(!CheckFiltID(Filt.PersonCategoryID, pRec->CatID))
			ok = 0;
		else if(!CheckFiltID(Filt.PersonID, pRec->ID))
			ok = 0;
		else {
			const bool local_ok = PPViewPerson::CheckClientActivityState(id, Filt.ClientActivityStateFlags, P_ClientActivityStateList); // @v12.2.2
			if(!local_ok)
				ok = 0;
			else {
				SString srch_buf;
				Filt.GetExtssData(PersonFilt::extssNameText, srch_buf);
				if(srch_buf.NotEmptyS()) {
					if(Filt.Flags & PersonFilt::fPrecName) {
						if(srch_buf.CmpNC(pRec->Name) != 0)
							ok = 0;
					}
					else {
						if(!ExtStrSrch(pRec->Name, srch_buf, 0))
							ok = 0;
					}
				}
			}
		}
		if(ok) {
			if(!(Filt.Flags & PersonFilt::fLocTagF) && !PPObjTag::CheckForTagFilt(PPOBJ_PERSON, pRec->ID, Filt.P_TagF))
				ok = 0;
			else if(Filt.CityID && !oneof3(Filt.GetAttribType(), PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
				PPID   city_id = 0;
				if(!pRec->MainLoc)
					ok = 0;
				else if(PsnObj.LocObj.GetCity(pRec->MainLoc, &city_id, 0, 1) <= 0)
					ok = 0;
				else if(!WObj.IsChildOf(city_id, Filt.CityID))
					ok = 0;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPViewPerson::CreateAddrRec(PPID addrID, const LocationTbl::Rec * pLocRec, const char * pAddrKindText, PsnAttrViewItem * pItem)
{
	int    ok = -1;
	LocationTbl::Rec loc_rec;
	if(addrID) {
		SString temp_buf;
		pItem->TabID = addrID;
		/* @v12.1.11
		if(pAddrKindText && pAddrKindText[0] == '@')
			PPLoadStringS(pAddrKindText+1, temp_buf).CopyTo(pItem->RegNumber, sizeof(pItem->RegNumber));
		else
			STRNSCPY(pItem->RegNumber, pAddrKindText);
		*/
		// @v12.1.11 {
		if(pAddrKindText && pAddrKindText[0] == '@')
			PPLoadStringS(pAddrKindText+1, temp_buf);
		else
			temp_buf = pAddrKindText;
		StrPool.AddS(temp_buf, &pItem->AddrTypeP);
		// } @v12.1.11 
		if(pLocRec || PsnObj.LocObj.Search(addrID, &loc_rec) > 0) {
			const LocationTbl::Rec * p_loc_rec = NZOR(pLocRec, &loc_rec);
			if(Filt.CityID && p_loc_rec->CityID != Filt.CityID) {
				;
			}
			else if((Filt.Flags & PersonFilt::fLocTagF) && !PPObjTag::CheckForTagFilt(PPOBJ_LOCATION, addrID, Filt.P_TagF)) {
				;
			}
			else {
				LocationCore::GetAddress(*p_loc_rec, 0, temp_buf);
				StrPool.AddS(temp_buf, &pItem->AddressP);
				// @v12.1.11 {
				LocationCore::GetExField(p_loc_rec, LOCEXSTR_PHONE, temp_buf);
				StrPool.AddS(temp_buf, &pItem->PhoneP);
				LocationCore::GetExField(p_loc_rec, LOCEXSTR_EMAIL, temp_buf);
				StrPool.AddS(temp_buf, &pItem->EMailP);
				// } @v12.1.11 
				if(Filt.Flags & PersonFilt::fShowFiasRcgn && SETIFZ(P_Fr, new PPFiasReference)) {
					PPLocAddrStruc las(temp_buf.Transf(CTRANSF_INNER_TO_OUTER), P_Fr);
					S_GUID uuid;
					if(las.FiasStreetID) {
						FiasAddrObjTbl::Rec fa_rec;
						if(P_Fr->FT.SearchAddrByID(las.FiasStreetID, &fa_rec) > 0) {
							if(P_Fr->FT.UrT.Search(fa_rec.IdUuRef, uuid) > 0) {
								uuid.ToStr(S_GUID::fmtIDL, temp_buf.Z());
								StrPool.AddS(temp_buf, &pItem->FiasAddrGuidP);
							}
						}
					}
					if(las.FiasHouseID) {
						FiasHouseObjTbl::Rec fh_rec;
						if(P_Fr->FT.SearchHouse(las.FiasHouseID, &fh_rec) > 0) {
							if(P_Fr->FT.UrT.Search(fh_rec.IdUuRef, uuid) > 0) {
								uuid.ToStr(S_GUID::fmtIDL, temp_buf.Z());
								StrPool.AddS(temp_buf, &pItem->FiasHouseGuidP);
							}
						}
					}
				}
				StrPool.AddS(p_loc_rec->Code, &pItem->BnkAcctP);
				pItem->CityID = p_loc_rec->CityID;
				if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR) {
					LocationCore::GetExField(p_loc_rec, LOCEXSTR_PHONE, temp_buf);
					StrPool.AddS(temp_buf, &pItem->PhoneP);
					LocationCore::GetExField(p_loc_rec, LOCEXSTR_EMAIL, temp_buf);
					StrPool.AddS(temp_buf, &pItem->RAddressP);
					LocationCore::GetExField(p_loc_rec, LOCEXSTR_CONTACT, temp_buf);
					StrPool.AddS(temp_buf, &pItem->BnkNameP);
				}
				ok = 1;
			}
		}
	}
	if(Filt.EmptyAttrib == EA_NOEMPTY && !addrID)
		ok = 0;
	else if(Filt.EmptyAttrib == EA_EMPTY && addrID)
		ok = 0;
	return ok;
}

int PPViewPerson::Helper_GetTagValue(PPObjID oid, PPID tagID, SString & rBuf)
{
	rBuf.Z();
	int   ok = -1;
	ObjTagItem tag_item;
	if(PPRef->Ot.GetTag(oid.Obj, oid.Id, tagID, &tag_item) > 0) {
		tag_item.GetStr(rBuf);
		ok = 1;
	}
	return ok;
}

int PPViewPerson::CreateTempRec(PersonTbl::Rec * pPsnRec, PPID tabID, PsnAttrViewItem * pItem)
{
	int    ok = 1;
	SString temp_buf;
	PsnAttrViewItem item;
	item.ID = pPsnRec->ID;
	item.TabID = tabID;
	STRNSCPY(item.Name, pPsnRec->Name);
	//
	// Случаи oneof2(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_BNKACCT) обрабатываются в вызывающий функции
	//
	if(Filt.GetAttribType() == PPPSNATTR_PHONEADDR) {
		PPELinkArray elink_ary;
		if(PsnObj.P_Tbl->GetELinks(pPsnRec->ID, elink_ary)) {
			const int buf_len = 128;
			SString phone_list, fax_list;
			elink_ary.GetPhones(5, phone_list, ELNKRT_PHONE);
			elink_ary.GetPhones(2, fax_list, ELNKRT_FAX);
			if(fax_list.Len() && (phone_list.Len() + fax_list.Len() + 6) < buf_len)
				phone_list.CatDiv(';', 2).Cat("fax").Space().Cat(fax_list);
			//phone_list.CopyTo(item.Phone, sizeof(item.Phone));
			StrPool.AddS(phone_list, &item.PhoneP);
		}
		if(pPsnRec->MainLoc) {
			PsnObj.LocObj.GetAddress(pPsnRec->MainLoc, 0, temp_buf);
			StrPool.AddS(temp_buf, &item.AddressP);
		}
		if(pPsnRec->RLoc) {
			PsnObj.LocObj.GetAddress(pPsnRec->RLoc, 0, temp_buf);
			StrPool.AddS(temp_buf, &item.RAddressP);
		}
		if(!item.PhoneP && !item.AddressP && !item.RAddressP) {
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	else if(Filt.GetAttribType() == PPPSNATTR_EMAIL) {
		PPELinkArray elink_ary;
		if(PsnObj.P_Tbl->GetELinks(pPsnRec->ID, elink_ary)) {
			elink_ary.GetPhones(1, temp_buf, ELNKRT_EMAIL);
			StrPool.AddS(temp_buf, &item.EMailP); // @v12.1.11 item.PhoneP-->item.EMailP
		}
		if(!item.EMailP) { // @v12.1.11 item.PhoneP-->item.EMailP
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	else if(Filt.GetAttribType() == PPPSNATTR_REGISTER && Filt.RegTypeID) {
		RegisterTbl::Rec reg_rec;
		if(PsnObj.GetRegister(pPsnRec->ID, Filt.RegTypeID, &reg_rec) > 0) {
			//STRNSCPY(item.RegSerial, reg_rec.Serial);
			StrPool.AddS(reg_rec.Serial, &item.RegSerialP);
			STRNSCPY(item.RegNumber, reg_rec.Num);
			item.RegInitDate = reg_rec.Dt;
			item.RegExpiry   = reg_rec.Expiry;
		}
		//if(item.RegSerial[0] == 0 && item.RegNumber[0] == 0) {
		if(!item.RegSerialP && !item.RegNumber[0]) {
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	else if(Filt.GetAttribType() == PPPSNATTR_TAG && Filt.RegTypeID) {
		Helper_GetTagValue(PPObjID(PPOBJ_PERSON, pPsnRec->ID), Filt.RegTypeID - TAGOFFSET, temp_buf);
		STRNSCPY(item.RegNumber, temp_buf);
		//if(item.RegSerial[0] == 0 && item.RegNumber[0] == 0) {
		if(item.RegNumber[0] == 0) {
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	if(ok == 0) {
		item.ID = 0;
		item.Name[0] = 0;
	}
	ASSIGN_PTR(pItem, item);
	return ok;
}

int PPViewPerson::Helper_InsertTempRec(const TempPersonTbl::Rec & rRec)
{
	int    ok = 1;
	TempPersonTbl::Key0 k0;
	k0.ID    = rRec.ID;
	k0.TabID = rRec.TabID;
	if(P_TempPsn->searchForUpdate(0, &k0, spEq)) {
		ok = P_TempPsn->updateRecBuf(&rRec) ? 1 : PPSetErrorDB();
	}
	else
		ok = P_TempPsn->insertRecBuf(&rRec) ? 1 : PPSetErrorDB();
	return ok;
}

int PPViewPerson::AddTempRec(PPID id, UintHashTable * pUsedLocList, int use_ta)
{
	int    ok = -1;
	if(P_TempPsn && id) {
		int    stop = 0;
		const  bool is_crsst = LOGIC(Filt.Flags & PersonFilt::fTagsCrsstab);
		uint   tags_count = 0;
		PersonTbl::Rec psn_rec;
		PPIDArray dlvr_addr_list;
		TempPersonTbl::Key0 k0;
		ObjTagList tags;
		SString temp_buf;
		SString buf2;
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		if(PsnObj.Search(id, &psn_rec) > 0 && CheckIDForFilt(id, &psn_rec)) {
			if(is_crsst) {
				THROW(PPRef->Ot.GetList(PPOBJ_PERSON, id, &tags));
				for(uint i = 0; i < tags.GetCount(); i++) {
					long   tab_id = DefaultTagID;
					const  ObjTagItem * p_item = tags.GetItemByPos(i);
					ObjTag.GetCurrTagVal(p_item, temp_buf);
					tab_id = p_item->TagID;
					k0.ID    = id;
					k0.TabID = tab_id;
					if(P_TempPsn->search(0, &k0, spEq))
						ok = -2;
					else if(CreateTempRec(&psn_rec, tab_id, &P_TempPsn->data) > 0) {
						temp_buf.CopyTo(P_TempPsn->data.RegNumber, sizeof(P_TempPsn->data.RegNumber));
						THROW_DB(P_TempPsn->insertRec());
						ok = 1;
					}
				}
			}
			else {
				const int attr_type = Filt.GetAttribType();
				const int loc_attr = (Filt.Flags & PersonFilt::fLocTagF && attr_type) ? PPPSNATTR_ALLADDR : attr_type;
				if(oneof3(loc_attr, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
					uint   i;
					SArray rec_list(sizeof(PsnAttrViewItem));
					PsnObj.GetDlvrLocList(id, &dlvr_addr_list);
					if(loc_attr == PPPSNATTR_ALLADDR) {
						if(psn_rec.MainLoc) {
							CALLPTRMEMB(pUsedLocList, Add((ulong)psn_rec.MainLoc));
							long   tab_id = psn_rec.MainLoc;
							PsnAttrViewItem vi;
							vi.ID = id;
							STRNSCPY(vi.Name, psn_rec.Name);
							if(CreateAddrRec(tab_id, 0, "@jaddress", &vi) > 0) {
								rec_list.insert(&vi);
							}
						}
						if(psn_rec.RLoc) {
							CALLPTRMEMB(pUsedLocList, Add((ulong)psn_rec.RLoc));
							long   tab_id = psn_rec.RLoc;
							PsnAttrViewItem vi;
							vi.ID = id;
							STRNSCPY(vi.Name, psn_rec.Name);
							if(CreateAddrRec(tab_id, 0, "@paddress", &vi) > 0) {
								rec_list.insert(&vi);
							}
						}
					}
					for(i = 0; i < dlvr_addr_list.getCount(); i++) {
						long   tab_id = dlvr_addr_list.get(i);
						PsnAttrViewItem vi;
						if(tab_id && pUsedLocList) {
							pUsedLocList->Add((ulong)tab_id);
						}
						vi.ID = id;
						STRNSCPY(vi.Name, psn_rec.Name);
						if(CreateAddrRec(tab_id, 0, "@daddress", &vi) > 0) {
							rec_list.insert(&vi);
						}
					}
					if(loc_attr == PPPSNATTR_DUPDLVRADDR) {
						i = rec_list.getCount();
						if(i) do {
							const PsnAttrViewItem & r_item = *static_cast<const PsnAttrViewItem *>(rec_list.at(--i));
							if(i == (rec_list.getCount()-1)) {
								int dup = 0;
								StrPool.GetS(r_item.AddressP, temp_buf);
								for(uint j = 0; j < i; j++) {
									const PsnAttrViewItem & r_item2 = *static_cast<const PsnAttrViewItem *>(rec_list.at(j));
									StrPool.GetS(r_item2.AddressP, buf2);
									if(temp_buf.CmpNC(buf2) == 0) {
										dup = 1;
										break;
									}
								}
								if(!dup)
									rec_list.atFree(i);
							}
						} while(i);
					}
					if(rec_list.getCount() == 0) {
						if(Filt.EmptyAttrib != EA_NOEMPTY && !Filt.CityID) {
							PsnAttrViewItem vi;
							vi.ID = id;
							STRNSCPY(vi.Name, psn_rec.Name);
							THROW(ok = Helper_InsertTempRec(vi));
						}
					}
					else if(Filt.EmptyAttrib != EA_EMPTY) {
						for(i = 0; i < rec_list.getCount(); i++) {
							PsnAttrViewItem * p_attr_view = static_cast<PsnAttrViewItem *>(rec_list.at(i));
							// @v12.1.11 {
							bool local_ok = true;
							if(Filt.Flags & PersonFilt::fLocTagF && attr_type) {
								if(attr_type == PPPSNATTR_EMAIL) {
									if(!p_attr_view->EMailP) {
										if(Filt.EmptyAttrib == EA_NOEMPTY)
											local_ok = false;
									}
									else if(Filt.EmptyAttrib == EA_EMPTY)
										local_ok = false;
								}
								else if(attr_type == PPPSNATTR_PHONEADDR) {
									if(!p_attr_view->PhoneP) {
										if(Filt.EmptyAttrib == EA_NOEMPTY)
											local_ok = false;
									}
									else if(Filt.EmptyAttrib == EA_EMPTY)
										local_ok = false;
								}
								else if(attr_type == PPPSNATTR_REGISTER && Filt.RegTypeID) {
									RegisterTbl::Rec reg_rec;
									if(PsnObj.LocObj.GetRegister(p_attr_view->TabID, Filt.RegTypeID, ZERODATE, false, &reg_rec) > 0) {
										StrPool.AddS(reg_rec.Serial, &p_attr_view->RegSerialP);
										STRNSCPY(p_attr_view->RegNumber, reg_rec.Num);
										p_attr_view->RegInitDate = reg_rec.Dt;
										p_attr_view->RegExpiry   = reg_rec.Expiry;
									}
									if(!p_attr_view->RegSerialP && !p_attr_view->RegNumber[0]) {
										if(Filt.EmptyAttrib == EA_NOEMPTY)
											local_ok = false;
									}
									else if(Filt.EmptyAttrib == EA_EMPTY)
										local_ok = false;
								}
								else if(attr_type == PPPSNATTR_TAG && Filt.RegTypeID) {
									Helper_GetTagValue(PPObjID(PPOBJ_LOCATION, p_attr_view->TabID), Filt.RegTypeID - TAGOFFSET, temp_buf);
									STRNSCPY(p_attr_view->RegNumber, temp_buf);
									if(!p_attr_view->RegSerialP && p_attr_view->RegNumber[0] == 0) {
										if(Filt.EmptyAttrib == EA_NOEMPTY)
											local_ok = false;
									}
									else if(Filt.EmptyAttrib == EA_EMPTY)
										local_ok = false;
								}								
							}
							// } @v12.1.11 
							if(local_ok)
								THROW(ok = Helper_InsertTempRec(*p_attr_view));
						}
					}
				}
				else if(attr_type == PPPSNATTR_BNKACCT) {
					TSVector <PPBankAccount> bac_ary;
					PsnObj.RegObj.GetBankAccountList(id, &bac_ary);
					if(bac_ary.getCount() == 0) {
						if(Filt.EmptyAttrib != EA_NOEMPTY) {
							PsnAttrViewItem vi;
							vi.ID = id;
							vi.TabID = 0;
							STRNSCPY(vi.Name, psn_rec.Name);
							THROW(ok = Helper_InsertTempRec(vi));
						}
					}
					else {
						if(Filt.EmptyAttrib != EA_EMPTY) {
							PersonTbl::Rec bnk_rec;
							for(uint pos = 0; pos < bac_ary.getCount(); pos++) {
								const PPBankAccount & r_ba = bac_ary.at(pos);
								PsnAttrViewItem vi;
								vi.ID = id;
								vi.TabID = r_ba.ID;
								STRNSCPY(vi.Name, psn_rec.Name);
								if(PsnObj.Fetch(r_ba.BankID, &bnk_rec) > 0) {
									RegisterTbl::Rec bic_rec;
									StrPool.AddS(bnk_rec.Name, &vi.BnkNameP);
									if(PsnObj.GetRegister(r_ba.BankID, PPREGT_BIC, &bic_rec) > 0) {
										StrPool.AddS(bic_rec.Num, &vi.PhoneP);
									}
								}
								StrPool.AddS(r_ba.Acct, &vi.BnkAcctP);
								vi.RegInitDate = r_ba.OpenDate;
								if(r_ba.AccType) {
									// @todo Чаще всего здесь одно и тоже значение PPBAC_CURRENT: можно ускорить
									GetObjectName(PPOBJ_BNKACCTYPE, r_ba.AccType, temp_buf);
									temp_buf.CopyTo(vi.RegNumber, sizeof(vi.RegNumber));
								}
								THROW(ok = Helper_InsertTempRec(vi));
							}
						}
					}
				}
				else {
					const long tab_id = DefaultTagID;
					k0.ID    = id;
					k0.TabID = tab_id;
					if(P_TempPsn->search(0, &k0, spEq))
						ok = -2;
					else if(CreateTempRec(&psn_rec, tab_id, &P_TempPsn->data) > 0) {
						THROW_DB(P_TempPsn->insertRec());
						ok = 1;
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewPerson::EditTempRec(PPID id, int use_ta)
{
	int    ok = -1;
	if(P_TempPsn && id) {
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(P_TempPsn, 0, P_TempPsn->ID == id));
		THROW(AddTempRec(id, 0, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewPerson::InitPersonAttribIteration()
{
	int    ok = 1;
	if(!P_TempPsn)
		ok = 0;
	else if((P_IterQuery = new BExtQuery(P_TempPsn, 1, 8)) != 0) {
		BtrDbKey k_;
		PPInitIterCounter(Counter, P_TempPsn);
		P_IterQuery->selectAll();
		P_IterQuery->initIteration(0, k_, spFirst);
	}
	else
		ok = PPSetErrorNoMem();
	return ok;
}

int PPViewPerson::InitPersonIteration()
{
	int    ok = 1;
	PersonTbl * pt = PsnObj.P_Tbl;
	PersonKindTbl * kt = & PsnObj.P_Tbl->Kind;
	if(Filt.PersonKindID) {
		PersonKindTbl::Key0 k0, k0_;
		MEMSZERO(k0);
		k0.KindID = Filt.PersonKindID;
		THROW_MEM(P_IterQuery = new BExtQuery(kt, 0));
		P_IterQuery->select(kt->PersonID, 0L).where(kt->KindID == Filt.PersonKindID);
		Counter.Init(P_IterQuery->countIterations(0, &(k0_ = k0), spGe));
		P_IterQuery->initIteration(false, &k0, spGe);
	}
	else {
		DBQ * dbq = 0;
		PPID pk = 0, pk_; // #0
		THROW_MEM(P_IterQuery = new BExtQuery(pt, 0));
		P_IterQuery->select(pt->ID, pt->Name, pt->Status, pt->MainLoc, pt->Flags, pt->RLoc, 0L);
		dbq = ppcheckfiltid(dbq, pt->ID, Filt.PersonID);
		dbq = ppcheckfiltid(dbq, pt->Status, Filt.Status);
		P_IterQuery->where(*dbq);
		Counter.Init(P_IterQuery->countIterations(0, &(pk_ = pk), spGt));
		P_IterQuery->initIteration(false, &pk, spGt);
	}
	CATCHZOK
	return ok;
}

int PPViewPerson::InitIteration()
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempPsn) {
		THROW(InitPersonAttribIteration());
	}
	else {
		THROW(InitPersonIteration());
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewPerson::NextIteration(PersonViewItem * pItem)
{
	int    ok = -1;
	PersonViewItem item;
	if(P_IterQuery)
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {// AHTOXA
			if(P_TempPsn) {
				if(oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					const  PPID loc_id = P_TempPsn->data.TabID;
					LocationTbl::Rec loc_rec;
					if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0) {
						item.AttrItem = P_TempPsn->data;
						ok = 1;
					}
				}
				else if(PsnObj.Search(P_TempPsn->data.ID, &item) > 0) {
					item.AttrItem = P_TempPsn->data;
					ok = 1;
				}
			}
			else {
				const PPID id = Filt.PersonKindID ? PsnObj.P_Tbl->Kind.data.PersonID : PsnObj.P_Tbl->data.ID;
				if(PsnObj.Search(id, &item) > 0 && CheckIDForFilt(id, &item))
					ok = 1;
			}
			Counter.Increment();
		}
	ASSIGN_PTR(pItem, item);
	return ok;
}
//
// ExtRegCombo
//
struct ExtRegEntry {
	PPID   ExtRegID;
	char   Name[256];   // @v12.1.10 [80]-->[256]
	int    AttribType;
};

enum {
	sercfIdFromRegFilt = 0x0001,
	sercfRegsOnly      = 0x0002,
	sercfLocAttrs      = 0x0004    
};

static SArray * CreateExtRegList(const PersonFilt * pFilt, PPID * pAttrID, uint flags/*sercfXXX*//*bool onlyRegs*/)
{
	PPID   id = 0;
	PPID   attr_id = 0;
	ExtRegEntry ere;
	SArray * p_ary = new SArray(sizeof(ExtRegEntry));
	THROW_MEM(p_ary);
	if(!oneof2(pFilt->Status, PPPRS_COUNTRY, PPPRS_REGION)) { // @v12.1.10 PPPRS_REGION
		{
			PPObjRegisterType rt_obj;
			PPRegisterType rt_rec;
			SString reg_name;
			SString name;
			if(flags & sercfRegsOnly)
				reg_name.Z();
			else
				PPGetSubStr(PPTXT_PERSONATTRIBUTE, PPPSNATTR_REGISTER, reg_name);
			ere.AttribType = PPPSNATTR_REGISTER;
			for(id = 0; rt_obj.EnumItems(&id, &rt_rec) > 0;) {
				bool  suitable = false;
				if(flags & sercfLocAttrs) {
					if(rt_rec.Flags & REGTF_LOCATION)
						suitable = true;
				}
				else {
					if(!pFilt->PersonKindID || oneof2(rt_rec.PersonKindID, 0, pFilt->PersonKindID)) {
						const long xst = CheckXORFlags(rt_rec.Flags, REGTF_PRIVATE, REGTF_LEGAL);
						suitable = (!pFilt->Status || !xst || ((pFilt->Status == PPPRS_PRIVATE) ? (xst & REGTF_PRIVATE) : (xst & REGTF_LEGAL)));
					}
				}
				if(suitable) {
					ere.ExtRegID = rt_rec.ID;
					(name = reg_name).Cat(rt_rec.Name).CopyTo(ere.Name, sizeof(ere.Name));
					if(pAttrID && *pAttrID == rt_rec.ID)
						attr_id = rt_rec.ID;
					p_ary->insert(&ere);
				}
			}
		}
		if(!(flags & sercfRegsOnly)) {
			PPObjTag tag_obj;
			ObjTagFilt ot_filt((flags & sercfLocAttrs) ? PPOBJ_LOCATION : PPOBJ_PERSON, ObjTagFilt::fOnlyTags);
			StrAssocArray * p_list = tag_obj.MakeStrAssocList(&ot_filt);
			if(p_list) {
				SString tag_attr;
				SString tag_name;
				PPGetSubStr(PPTXT_PERSONATTRIBUTE, PPPSNATTR_TAG, tag_attr);
				ere.AttribType = PPPSNATTR_TAG;
				id = TAGOFFSET;
				for(uint i = 0; i < p_list->getCount(); i++) {
					StrAssocArray::Item item = p_list->Get(i);
					ere.ExtRegID = TAGOFFSET + item.Id;
					(tag_name = tag_attr).Cat(item.Txt).CopyTo(ere.Name, sizeof(ere.Name));
					if(pAttrID && *pAttrID == ere.ExtRegID)
						attr_id = ere.ExtRegID;
					p_ary->insert(&ere);
					id = (ere.ExtRegID > id) ? ere.ExtRegID : id;
				}
				ZDELETE(p_list);
			}
			{
				LongArray attr_list;
				if(flags & sercfLocAttrs) {
					attr_list.addzlist(PPPSNATTR_PHONEADDR, PPPSNATTR_EMAIL, 0);
				}
				else {
					attr_list.addzlist(PPPSNATTR_BNKACCT, PPPSNATTR_PHONEADDR, PPPSNATTR_EMAIL, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR,
						PPPSNATTR_DUPDLVRADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR, 0);
				}
				for(uint i = 0; i < attr_list.getCount(); i++) {
					const int attr = attr_list.get(i);
					ere.ExtRegID = ++id;
					ere.AttribType = attr;
					PPGetSubStr(PPTXT_PERSONATTRIBUTE, attr, ere.Name, sizeof(ere.Name));
					if(attr == pFilt->GetAttribType())
						attr_id = id;
					p_ary->insert(&ere);
				}
			}
		}
		p_ary->sort(PTR_CMPFUNC(PPLBItem));
	}
	ASSIGN_PTR(pAttrID, attr_id);
	CATCH
		ASSIGN_PTR(pAttrID, 0);
	ENDCATCH
	return p_ary;
}

static int SetupExtRegCombo(TDialog * dlg, uint ctl, const PersonFilt * pFilt, uint flags/*sercfXXX*//*int idFromRegFilt, int onlyRegs*/)
{
	int    ok = 1;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	SArray   * p_ary = 0;
	if(p_combo && pFilt) {
		PPID   reg_filt_id = pFilt->P_RegF ? pFilt->P_RegF->RegTypeID : 0;
		PPID   id = (flags & sercfIdFromRegFilt) ? reg_filt_id : pFilt->RegTypeID;
		ListBoxDef * def = 0;
		THROW(p_ary = CreateExtRegList(pFilt, &id, flags));
		THROW_MEM(def = new StdListBoxDef(p_ary, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, sizeof(ExtRegEntry))));
		ListWindow * p_lw = new ListWindow(def);
		THROW_MEM(p_lw);
		p_combo->setListWindow(p_lw);
		if(id)
			p_combo->TransmitData(+1, &id);
		else {
			p_combo->setInputLineText(0);
			p_combo->setUndefTag(1);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		delete p_ary;
		PPError(); // ?
	ENDCATCH
	return ok;
}

static int GetExtRegData(TDialog * dlg, uint ctl, int * pAttrType, PPID *pRegId)
{
	int    ok = -1;
	if(pAttrType && pRegId) {
		*pRegId = 0;
		*pAttrType = 0;
		ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
		if(p_combo) {
			StdListBoxDef * p_def = 0;
			p_combo->TransmitData(-1, pRegId);
			if(*pRegId && (p_def = static_cast<StdListBoxDef *>(p_combo->listDef())) != 0 && p_def->P_Data) {
				uint   pos = 0;
				const ExtRegEntry * p_ere = p_def->P_Data->lsearch(pRegId, &pos, CMPF_LONG) ? static_cast<const ExtRegEntry *>(p_def->P_Data->at(pos)) : 0;
				if(p_ere) {
					if((*pAttrType  = p_ere->AttribType) != PPPSNATTR_REGISTER && *pAttrType != PPPSNATTR_TAG)
						*pRegId = 0;
					ok = 1;
				}
				else
					*pRegId = 0;
			}
		}
	}
	return ok;
}

int GetExtRegListIds(PPID psnKindID, PPID statusID, PPIDArray * pList)
{
	pList->clear();
	int    ok = -1;
	uint   c;
	PersonFilt filt;
	SArray * p_list = 0;
	ExtRegEntry * p_entry;
	filt.PersonKindID = psnKindID;
	filt.Status = statusID;
	THROW(p_list = CreateExtRegList(&filt, 0, sercfRegsOnly));
	for(c = 0; p_list->enumItems(&c, (void **)&p_entry) > 0;) {
		THROW(pList->add(p_entry->ExtRegID));
		ok = 1;
	}
	CATCHZOK
	delete p_list;
	return ok;
}
//
// Фильтр по персоналиям
//
IMPLEMENT_PPFILT_FACTORY(Person); PersonFilt::PersonFilt() : PPBaseFilt(PPFILT_PERSON, 0, 1), P_RegF(0), P_TagF(0), P_SjF(0)
{
	SetFlatChunk(offsetof(PersonFilt, ReserveStart),
		offsetof(PersonFilt, P_RegF) - offsetof(PersonFilt, ReserveStart));
	SetBranchBaseFiltPtr(PPFILT_REGISTER, offsetof(PersonFilt, P_RegF));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(PersonFilt, P_TagF));
	SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(PersonFilt, P_SjF));
	SetBranchObjIdListFilt(offsetof(PersonFilt, List));
	SetBranchSString(offsetof(PersonFilt, SrchStr_));
	Init(1, 0);
}

void PersonFilt::Setup()
{
	SrchStr_.Strip();
	if(P_RegF) {
		if(!P_RegF->SerPattern.NotEmptyS() && !P_RegF->NmbPattern.NotEmptyS() && P_RegF->RegPeriod.IsZero() && P_RegF->ExpiryPeriod.IsZero()) {
			ZDELETE(P_RegF);
		}
		else
			P_RegF->Oid.Obj = PPOBJ_PERSON;
	}
	if(P_TagF && P_TagF->IsEmpty())
		ZDELETE(P_TagF);
}

bool PersonFilt::IsEmpty() const
{
	bool   yes = true;
	if(PersonKindID || PersonCategoryID || Status || CityID || (P_RegF && !P_RegF->IsEmpty()) ||
		Flags & fVatFree || StaffDivID || StaffOrgID || List.GetCount() || (P_TagF && !P_TagF->IsEmpty()) || (P_SjF && !P_SjF->IsEmpty()))
		yes = false;
	else {
		if((Flags & fCliActivityStats) && ClientActivityStateFlags != 0 && ClientActivityStateFlags != 0xffff) { // @v12.2.2
			yes = false;
		}
		else {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			if(GetExtssData(extssNameText, r_temp_buf) > 0 && r_temp_buf.NotEmptyS())
				yes = false;
			else if(GetExtssData(extssEmailText, r_temp_buf) > 0 && r_temp_buf.NotEmptyS())
				yes = false;
		}
	}
	return yes;
}

PersonFilt & FASTCALL PersonFilt::operator = (const PersonFilt &src)
{
	Copy(&src, 0);
	Setup();
	return *this;
}

int PersonFilt::GetAttribType() const
{
	return AttribType;
}

int PersonFilt::GetLocAttribType() const
{
	int    result = 0;
	if(Flags & PersonFilt::fLocTagF && AttribType)
		result = PPPSNATTR_ALLADDR;
	else if(oneof5(AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR, PPPSNATTR_STANDALONEADDR))
		result = AttribType;
	return result;
}

bool PersonFilt::SetAttribType(int attribType)
{
	AttribType = attribType;
	return true;
}

bool PersonFilt::IsLocAttr() const { return oneof5(AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DUPDLVRADDR, PPPSNATTR_STANDALONEADDR); }

int PersonFilt::GetExtssData(int fldID, SString & rBuf) const { return PPGetExtStrData_def(fldID, extssNameText, SrchStr_, rBuf); }
int PersonFilt::PutExtssData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, SrchStr_, pBuf); }

/*virtual*/int PersonFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		struct PersonFilt_v0 : public PPBaseFilt {
			PersonFilt_v0() : PPBaseFilt(PPFILT_PERSON, 0, 0), P_RegF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(PersonFilt, ReserveStart), offsetof(PersonFilt, P_RegF) - offsetof(PersonFilt, ReserveStart));
				SetBranchBaseFiltPtr(PPFILT_REGISTER, offsetof(PersonFilt, P_RegF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(PersonFilt, P_TagF));
				SetBranchSString(offsetof(PersonFilt_v0, SrchStr));
				Init(1, 0);
			}
			uint8  ReserveStart[32];  // @anchor
			PPID   PersonKindID;
			PPID   PersonCategoryID;
			PPID   Status;
			PPID   CityID;
			int    AttribType;
			int    EmptyAttrib;
			PPID   RegTypeID;
			PPID   StaffOrgID;
			PPID   StaffDivID;
			PPID   PersonID;
			long   Flags;
			RegisterFilt * P_RegF;    // @anchor
			TagFilt * P_TagF;
			SString SrchStr;
		};
		PersonFilt_v0 fv0;
		THROW(fv0.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
#define CPYFLD(f) f = fv0.f
		CPYFLD(PersonKindID);
		CPYFLD(PersonCategoryID);
		CPYFLD(Status);
		CPYFLD(CityID);
		CPYFLD(AttribType);
		CPYFLD(EmptyAttrib);
		CPYFLD(RegTypeID);
		CPYFLD(StaffOrgID);
		CPYFLD(StaffDivID);
		CPYFLD(PersonID);
		CPYFLD(Flags);
#undef CPYFLD
		{
			ZDELETE(P_RegF);
			if(fv0.P_RegF) {
				THROW_MEM(P_RegF = new RegisterFilt);
				*P_RegF = *fv0.P_RegF;
			}
		}
		{
			ZDELETE(P_TagF);
			if(fv0.P_TagF) {
				THROW_MEM(P_TagF = new TagFilt);
				*P_TagF = *fv0.P_TagF;
			}
		}
		SrchStr_ = fv0.SrchStr;
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
// Диалог дополнительных опций фильтра по персоналиям
//
static int EditRegFilt(PersonFilt * pFilt)
{
	class PersonFiltAdvDialog : public TDialog {
		DECL_DIALOG_DATA(PersonFilt);
	public:
		PersonFiltAdvDialog() : TDialog(DLG_PFLTADVOPT)
		{
			SetupCalPeriod(CTLCAL_PFLTADVOPT_REGPRD,    CTL_PFLTADVOPT_REGPRD);
			SetupCalPeriod(CTLCAL_PFLTADVOPT_EXPIRYPRD, CTL_PFLTADVOPT_EXPIRYPRD);
		}
		DECL_DIALOG_SETDTS()
		{
			Data.Copy(pData, 0);
			RVALUEPTR(Reg, Data.P_RegF);
			SetupExtRegCombo(this, CTLSEL_PFLTADVOPT_REG, &Data, sercfIdFromRegFilt|sercfRegsOnly);
			setCtrlString(CTL_PFLTADVOPT_SERIAL, Reg.SerPattern);
			setCtrlString(CTL_PFLTADVOPT_NUMBER, Reg.NmbPattern);
			SetPeriodInput(this, CTL_PFLTADVOPT_REGPRD,    &Reg.RegPeriod);
			SetPeriodInput(this, CTL_PFLTADVOPT_EXPIRYPRD, &Reg.ExpiryPeriod);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			getCtrlData(CTLSEL_PFLTADVOPT_REG, &Reg.RegTypeID);
			getCtrlString(CTL_PFLTADVOPT_SERIAL, Reg.SerPattern);
			getCtrlString(CTL_PFLTADVOPT_NUMBER, Reg.NmbPattern);
			GetPeriodInput(this, CTL_PFLTADVOPT_REGPRD,    &Reg.RegPeriod);
			GetPeriodInput(this, CTL_PFLTADVOPT_EXPIRYPRD, &Reg.ExpiryPeriod);
			if(Reg.IsEmpty()) {
				ZDELETE(pData->P_RegF);
			}
			else {
				Reg.Oid.Obj = PPOBJ_PERSON;
				SETIFZ(pData->P_RegF, new RegisterFilt);
				*pData->P_RegF = Reg;
			}
			return 1;
		}
	private:
		RegisterFilt Reg;
	};
	DIALOG_PROC_BODY(PersonFiltAdvDialog, pFilt);
}
//
//
//
class PersonFiltDialog : public TDialog {
	DECL_DIALOG_DATA(PersonFilt);
public:
	PersonFiltDialog(uint rezID) : TDialog(NZOR(rezID, DLG_PSNFLT)), CluFlagsLock_(0)
	{
		IsThereCasDetectionList = LOGIC(PsnObj.GetConfig().ClientActivityDetectionList.getCount());
		SetupCalDate(CTLCAL_PSNFLT_CASDT, CTL_PSNFLT_CASDT); // @v12.2.2
		SetupCalPeriod(CTLCAL_PSNFLT_NEWCLIPERIOD, CTL_PSNFLT_NEWCLIPERIOD);
		enableCommand(cmAdvOptions, 1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		SetupPPObjCombo(this, CTLSEL_PSNFLT_CITY, PPOBJ_WORLD, Data.CityID, OLW_CANSELUPLEVEL|OLW_WORDSELECTOR, 0);
		SetupPPObjCombo(this, CTLSEL_PSNFLT_KIND, PPOBJ_PERSONKIND, Data.PersonKindID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_PSNFLT_CATEGORY, PPOBJ_PRSNCATEGORY, Data.PersonCategoryID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_PSNFLT_STATUS, PPOBJ_PRSNSTATUS, Data.Status, 0, 0);
		SetupExtRegCombo(this, CTLSEL_PSNFLT_ATTR, &Data, (Data.Flags & PersonFilt::fLocTagF) ? sercfLocAttrs : 0);
		if(Data.GetAttribType())
			setCtrlData(CTL_PSNFLT_EMPTY, &Data.EmptyAttrib);
		else
			disableCtrl(CTL_PSNFLT_EMPTY, 1);
		Data.GetExtssData(PersonFilt::extssNameText, temp_buf);
		setCtrlString(CTL_PSNFLT_NAMESTR, temp_buf);
		SetupWordSelector(CTL_PSNFLT_NAMESTR, new TextHistorySelExtra("personfilt-nametext-common"), 0, 2, WordSel_ExtraBlock::fFreeText);
		setCtrlUInt16(CTL_PSNFLT_VATFREE, BIN(Data.Flags & PersonFilt::fVatFree));
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 0, PersonFilt::fTagsCrsstab);
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 1, PersonFilt::fHasImages);
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 2, PersonFilt::fShowHangedAddr);
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 3, PersonFilt::fLocTagF);
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 4, PersonFilt::fShowFiasRcgn);
		AddClusterAssoc(CTL_PSNFLT_FLAGS, 5, PersonFilt::fCliActivityStats); // @v12.2.2
		SetClusterData(CTL_PSNFLT_FLAGS, Data.Flags);
		DisableClusterItem(CTL_PSNFLT_FLAGS, 2, Data.GetAttribType() != PPPSNATTR_ALLADDR);
		// @v12.1.10 DisableClusterItem(CTL_PSNFLT_FLAGS, 3, !Data.IsLocAttr());
		{
			PPLocationConfig loc_cfg;
			PPObjLocation::FetchConfig(&loc_cfg);
			DisableClusterItem(CTL_PSNFLT_FLAGS, 4, !(loc_cfg.Flags & PPLocationConfig::fUseFias) || !Data.IsLocAttr());
		}
		setCtrlData(CTL_PSNFLT_CASDT, &Data.ClientActivityEvalDate); // @v12.2.2
		SetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &Data.NewCliPeriod);
		// @v12.2.2 AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 0, PersonFilt::fNewClientsOnly);
		// @v12.2.2 SetClusterData(CTL_PSNFLT_NEWCLIONLY, Data.Flags);
		// @v12.2.2 {
		{
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 0, (1 << PPObjPerson::ClientActivityState::stUndef));
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 1, (1 << PPObjPerson::ClientActivityState::stNoData));
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 2, (1 << PPObjPerson::ClientActivityState::stRegularTa));
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 3, (1 << PPObjPerson::ClientActivityState::stDelayedTa));
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 4, (1 << PPObjPerson::ClientActivityState::stHopelesslyDelayedTa));
			AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 5, PPObjPerson::ClientActivityState::stfNewClient);
			SetClusterData(CTL_PSNFLT_NEWCLIONLY, Data.ClientActivityStateFlags);
		}
		SetupCtrls();
		// } @v12.2.2 
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		SString temp_buf;
		getCtrlData(CTLSEL_PSNFLT_CITY, &Data.CityID);
		getCtrlData(CTLSEL_PSNFLT_KIND, &Data.PersonKindID);
		getCtrlData(CTLSEL_PSNFLT_CATEGORY, &Data.PersonCategoryID);
		getCtrlData(CTLSEL_PSNFLT_STATUS, &Data.Status);
		{
			int    attr_type = 0;
			GetExtRegData(this, CTLSEL_PSNFLT_ATTR, &attr_type, &Data.RegTypeID);
			Data.SetAttribType(attr_type);
		}
		getCtrlData(CTL_PSNFLT_EMPTY, &Data.EmptyAttrib);
		getCtrlString(CTL_PSNFLT_NAMESTR, temp_buf.Z());
		Data.PutExtssData(PersonFilt::extssNameText, temp_buf);
		SETFLAG(Data.Flags, PersonFilt::fVatFree, getCtrlUInt16(CTL_PSNFLT_VATFREE));
		GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
		if(IsThereCasDetectionList) {
			getCtrlData(CTL_PSNFLT_CASDT, &Data.ClientActivityEvalDate); // @v12.2.2
			GetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &Data.NewCliPeriod);
			// @v12.2.2 GetClusterData(CTL_PSNFLT_NEWCLIONLY, &Data.Flags);
			Data.ClientActivityStateFlags = static_cast<uint16>(GetClusterData(CTL_PSNFLT_NEWCLIONLY)); // @v12.2.2 
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmAdvOptions)) {
			EditRegFilt(&Data);
		}
		else if(event.isCmd(cmClientActivityConfig)) { // @v12.2.4
			PPPersonConfig psn_cfg;
			PPObjPerson::ReadConfig(&psn_cfg);
			if(PPObjPerson::EditClientAcitivityConfig(psn_cfg) > 0) {
				if(PPObjPerson::WriteConfig(&psn_cfg, 1)) {
					PsnObj.DirtyConfig();
				}
			}
		}
		else if(event.isClusterClk(CTL_PSNFLT_FLAGS)) {
			if(!CluFlagsLock_) {
				CluFlagsLock_ = 1;
				const long preserve_flags = Data.Flags;
				GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
				if((Data.Flags & PersonFilt::fLocTagF) != (preserve_flags & PersonFilt::fLocTagF)) {
					SetupExtRegCombo(this, CTLSEL_PSNFLT_ATTR, &Data, (Data.Flags & PersonFilt::fLocTagF) ? sercfLocAttrs : 0); // @v12.1.10
					if(Data.P_TagF && !Data.P_TagF->IsEmpty()) {
						if(CONFIRM(PPCFM_PSNFLTTAGFRESET)) {
							ZDELETE(Data.P_TagF);
						}
						else {
							SETFLAGBYSAMPLE(Data.Flags, PersonFilt::fLocTagF, preserve_flags);
							SetClusterData(CTL_PSNFLT_FLAGS, Data.Flags);
						}
					}
				}
				CluFlagsLock_ = 0;
				SetupCtrls(); // @v12.2.2
			}
		}
		else if(event.isCmd(cmCBSelected)) {
			const long preserve_attr = Data.GetAttribType();
			const long preserve_regtypeid = Data.RegTypeID;
			const bool preserve_loc_attr = Data.IsLocAttr();
			int    do_rollback = 0;
			getDTS(0);
			if(preserve_loc_attr != Data.IsLocAttr() && (Data.P_TagF && !Data.P_TagF->IsEmpty())) {
                const bool c1 = (Data.Flags & PersonFilt::fLocTagF && !Data.IsLocAttr());
				const bool c2 = false; //(!(Data.Flags & PersonFilt::fLocTagF) && Data.IsLocAttr());
				if(c1 || c2) {
					if(CONFIRM(PPCFM_PSNFLTTAGFRESET)) {
						ZDELETE(Data.P_TagF);
					}
					else
						do_rollback = 1;
				}
			}
			if(do_rollback) {
				Data.SetAttribType(preserve_attr);
				Data.RegTypeID = preserve_regtypeid;
			}
			SetupExtRegCombo(this, CTLSEL_PSNFLT_ATTR, &Data, (Data.Flags & PersonFilt::fLocTagF) ? sercfLocAttrs : 0);
			{
				int    attr_type = 0;
				GetExtRegData(this, CTLSEL_PSNFLT_ATTR, &attr_type, &Data.RegTypeID);
				Data.SetAttribType(attr_type);
			}
			if(!Data.GetAttribType()) {
				setCtrlUInt16(CTL_PSNFLT_EMPTY, 0);
				disableCtrl(CTL_PSNFLT_EMPTY, 1);
			}
			else
				disableCtrl(CTL_PSNFLT_EMPTY, 0);
			DisableClusterItem(CTL_PSNFLT_FLAGS, 2, Data.GetAttribType() != PPPSNATTR_ALLADDR);
			// @v12.1.10 DisableClusterItem(CTL_PSNFLT_FLAGS, 3, !Data.IsLocAttr());
			{
				PPLocationConfig loc_cfg;
				PPObjLocation::FetchConfig(&loc_cfg);
				DisableClusterItem(CTL_PSNFLT_FLAGS, 4, !(loc_cfg.Flags & PPLocationConfig::fUseFias) || !Data.IsLocAttr());
			}
		}
		else if(event.isCmd(cmTags)) {
			GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
			const  PPID tag_obj_type = (Data.Flags & PersonFilt::fLocTagF) ? PPOBJ_LOCATION : PPOBJ_PERSON;
			if(!SETIFZ(Data.P_TagF, new TagFilt()))
				PPError(PPERR_NOMEM);
			else if(!EditTagFilt(tag_obj_type, Data.P_TagF))
				PPError();
			if(Data.P_TagF->IsEmpty())
				ZDELETE(Data.P_TagF);
		}
		else if(event.isCmd(cmSysjFilt2)) {
			SysJournalFilt sj_filt;
			RVALUEPTR(sj_filt, Data.P_SjF);
			sj_filt.ObjType = PPOBJ_PERSON;
			if(EditSysjFilt2(&sj_filt) > 0) {
				SETIFZ(Data.P_SjF, new SysJournalFilt);
				ASSIGN_PTR(Data.P_SjF, sj_filt);
			}
			if(Data.P_SjF) {
				//
				// Функция SysJournalFilt::IsEmpty считает фильтр, в котором установлен ObjType не пустым. В данном случае это - не верно.
				//
				Data.P_SjF->ObjType = 0;
				if(Data.P_SjF->IsEmpty()) {
					ZDELETE(Data.P_SjF);
				}
				else
					Data.P_SjF->ObjType = PPOBJ_PERSON;
			}
		}
		else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_PSNFLT_NEWCLIPERIOD)) {
			if(IsThereCasDetectionList) {
				SetupCtrls();
			}
		}
		else
			return;
		clearEvent(event);
	}
	void   SetupCtrls()
	{
		if(!IsThereCasDetectionList) {
			DisableClusterItem(CTL_PSNFLT_FLAGS, 5, true);
			disableCtrl(CTL_PSNFLT_CASDT, true);
			disableCtrl(CTL_PSNFLT_NEWCLIPERIOD, true);
			disableCtrl(CTL_PSNFLT_NEWCLIONLY, true);
			enableCommand(cmClientActivityConfig, false); // @v12.2.4
		}
		else {
			DisableClusterItem(CTL_PSNFLT_FLAGS, 5, false);
			bool disable_sub_cas_ctrls = !(Data.Flags & PersonFilt::fCliActivityStats);
			disableCtrl(CTL_PSNFLT_CASDT, disable_sub_cas_ctrls);
			disableCtrl(CTL_PSNFLT_NEWCLIPERIOD, disable_sub_cas_ctrls);
			disableCtrl(CTL_PSNFLT_NEWCLIONLY, disable_sub_cas_ctrls);
			enableCommand(cmClientActivityConfig, !disable_sub_cas_ctrls); // @v12.2.4
			{
				bool disable_newclionly_flag = true;
				if(!disable_sub_cas_ctrls) {
					DateRange temp_period;
					temp_period.Z();
					GetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &temp_period);
					disable_newclionly_flag = temp_period.IsZero();
				}
				DisableClusterItem(CTL_PSNFLT_NEWCLIONLY, 5, disable_newclionly_flag);
			}
		}
	}
	int    CluFlagsLock_;
	PPObjPerson PsnObj;
	bool   IsThereCasDetectionList;
};

int PPViewPerson::EditBaseFilt(PPBaseFilt * pFilt)
{
	PersonFilt * p_filt = static_cast<PersonFilt *>(pFilt);
	if(p_filt) {
		uint   dlg_id = (p_filt->Flags & PersonFilt::fCliActivityStats) ? DLG_CLIACTFILT : DLG_PSNFLT;
		DIALOG_PROC_BODY_P1(PersonFiltDialog, dlg_id, p_filt);
	}
	else
		return 0;
}
//
//
//
int PPViewPerson::CreateLikenessTable()
{
	int    ok = -1;
	return ok;
}

int PPViewPerson::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_PERSONKIND, Filt.PersonKindID, 0, 0);
	return -1;
}

/*struct Register_ {
	long   ID;
	char   Name[128];
	char   RegSerial[12];
	char   RegNumber[64];
	LDATE  RegInitDate;
	LDATE  RegExpiry;
	long   Flags;
};

struct BnkAcct_ {
	long   ID;
	long   TabID;
	char   Name[128];
	char   BnkName[128];
	char   BnkAcct[28];
	char   AcctType[64];
	LDATE  RegInitDate;
	char   BIC[128];
	long   Flags;
};*/

int FASTCALL PPViewPerson::HasImage(const void * pData)
{
	long flags = 0L;
	if(pData) {
		const PPID psn_id = (Filt.GetAttribType() != PPPSNATTR_HANGEDADDR) ? *static_cast<const long *>(pData) : 0;
		PersonTbl::Rec psn_rec;
		if(PsnObj.Fetch(psn_id, &psn_rec) > 0)
			flags = psn_rec.Flags;
	}
	return BIN(flags & PSNF_HASIMAGES);
}

int FASTCALL PPViewPerson::IsNewCliPerson(PPID id) const
{
	return NewCliList.Has(id);
}

SString & FASTCALL PPViewPerson::GetFromStrPool(uint strP, SString & rBuf) const
{
	StrPool.GetS(strP, rBuf);
	return rBuf;
}

/*static*/int PPViewPerson::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pCellStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		PPViewPerson * p_view = static_cast<PPViewPerson *>(pBrw->P_View);
		if(p_def && p_view && col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewPerson::BrwHdr * p_hdr = static_cast<const PPViewPerson::BrwHdr *>(pData);
			const PersonFilt * p_filt = static_cast<const PersonFilt *>(p_view->GetBaseFilt());
			//
			if(!p_view->IsCrosstab() && p_filt) {
				int is_register  = (p_filt->GetAttribType() == PPPSNATTR_REGISTER) ? oneof2(p_filt->RegTypeID, PPREGT_OKPO, PPREGT_TPID/*, PPREGT_BNKCORRACC*/) : 0;
				int is_bank_acct = (p_filt->GetAttribType() == PPPSNATTR_BNKACCT);
				if(col == 0 && p_view->HasImage(pData)) { // К персоналии привязана картинка?
					pCellStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
					pCellStyle->Color2 = GetColorRef(SClrGreen);
					ok = 1;
				}
				/*else if(col == 1 && p_view->IsNewCliPerson(*static_cast<const  PPID *>(pData))) {
					pCellStyle->Flags = 0;
					pCellStyle->Color = GetColorRef(SClrOrange);
					ok = 1;
				}*/
				else if(r_col.OrgOffs == 1) { // name
					if(p_view->P_ClientActivityStateList) {
						uint caslp = 0;
						if(p_view->P_ClientActivityStateList->Search(p_hdr->ID, &caslp)) {
							const long state = p_view->P_ClientActivityStateList->at(caslp).Val;
							switch(state & ~PPObjPerson::ClientActivityState::stfNewClient) {
								case PPObjPerson::ClientActivityState::stDelayedTa:
									ok = pCellStyle->SetLeftBottomCornerColor(GetColorRef(SClrRed));
									break;
								case PPObjPerson::ClientActivityState::stHopelesslyDelayedTa:
									ok = pCellStyle->SetLeftBottomCornerColor(GetColorRef(SClrBlack));
									break;
								case PPObjPerson::ClientActivityState::stRegularTa:
									ok = pCellStyle->SetLeftBottomCornerColor(GetColorRef(SClrGreen));
									break;
							}
							if(state & PPObjPerson::ClientActivityState::stfNewClient) {
								ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
							}
						}
					}
				}
				/*else if(r_col.OrgOffs == 9) { // client activity state
				}*/
				/* @v9.8.4 @todo Из-за замены текстовых полей во временной таблице на ссылки в StrPool следующий блок надо переделать
				else if((is_register && col == 3) || (is_bank_acct && col == 5)) {
					int is_valid = 0;
					// @v10.8.1 {
					STokenRecognizer tr;
					SNaturalTokenStat nts;
					SNaturalTokenArray nta;
					tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), &nts); 
					// } @v10.8.1 
					SString code = is_bank_acct ? ((BnkAcct_*)pData)->BnkAcct : ((Register_*)pData)->RegNumber;
					SString bic  = is_bank_acct ? ((BnkAcct_*)pData)->BIC     : ((Register_*)pData)->RegSerial;
					if(code.Strip().Len()) {
						if(is_bank_acct)
							is_valid = CheckBnkAcc(code, bic.Strip());
						else {
							if(p_filt->RegTypeID == PPREGT_OKPO) {
								// @v10.8.1 is_valid = CheckOKPO(code);
								is_valid = (nta.Has(SNTOK_RU_OKPO) > 0.0f); // @v10.8.1 
							}
							else if(p_filt->RegTypeID == PPREGT_TPID) {
								// @v8.7.4 is_valid = CheckINN(code);
								// @v10.8.1 is_valid = SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, code, code.Len()); // @v8.7.4
								is_valid = (nta.Has(SNTOK_RU_INN) > 0.0f); // @v10.8.1 
							}
							else
								is_valid = CheckCorrAcc(code, bic);
						}
						ok = 1;
					}
					if(ok > 0) {
						pCellStyle->Flags = 0;
						if(is_valid)
							pCellStyle->Color = GetColorRef(SClrAqua);
						else
							pCellStyle->Color = GetColorRef(SClrCoral);
					}
				}
				*/
			}
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewPerson * p_view = static_cast<PPViewPerson *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pCellStyle, p_brw) : -1;
	}
	return ok;
#if 0 // {
	int    ok = -1;
	PPViewPerson * p_view = static_cast<PPViewPerson *>(extraPtr);
	if(pData && pCellStyle && p_view) {
		const PersonFilt * p_filt = static_cast<const PersonFilt *>(p_view->GetBaseFilt());
		if(!p_view->IsCrosstab() && p_filt) {
			int is_register  = (p_filt->GetAttribType() == PPPSNATTR_REGISTER) ? oneof2(p_filt->RegTypeID, PPREGT_OKPO, PPREGT_TPID/*, PPREGT_BNKCORRACC*/) : 0;
			int is_bank_acct = (p_filt->GetAttribType() == PPPSNATTR_BNKACCT);

			const  BrowserDef * p_def = pBrw->getDef();
			if(col >= 0 && col < p_def->getCountI()) {
				const BroColumn & r_col = p_def->at(col);
			}

			if(col == 0 && p_view->HasImage(pData)) { // К персоналии привязана картинка?
				pCellStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
				pCellStyle->Color2 = GetColorRef(SClrGreen);
				ok = 1;
			}
			else if(col == 1 && p_view->IsNewCliPerson(*static_cast<const  PPID *>(pData))) {
				pCellStyle->Flags = 0;
				pCellStyle->Color = GetColorRef(SClrOrange);
				ok = 1;
			}
			/* @v9.8.4 @todo Из-за замены текстовых полей во временной таблице на ссылки в StrPool следующий блок надо переделать
			else if((is_register && col == 3) || (is_bank_acct && col == 5)) {
				int is_valid = 0;
				// @v10.8.1 {
				STokenRecognizer tr;
				SNaturalTokenStat nts;
				SNaturalTokenArray nta;
				tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta.Z(), &nts); 
				// } @v10.8.1 
				SString code = is_bank_acct ? ((BnkAcct_*)pData)->BnkAcct : ((Register_*)pData)->RegNumber;
				SString bic  = is_bank_acct ? ((BnkAcct_*)pData)->BIC     : ((Register_*)pData)->RegSerial;
				if(code.Strip().Len()) {
					if(is_bank_acct)
						is_valid = CheckBnkAcc(code, bic.Strip());
					else {
						if(p_filt->RegTypeID == PPREGT_OKPO) {
							// @v10.8.1 is_valid = CheckOKPO(code);
							is_valid = (nta.Has(SNTOK_RU_OKPO) > 0.0f); // @v10.8.1 
						}
						else if(p_filt->RegTypeID == PPREGT_TPID) {
							// @v8.7.4 is_valid = CheckINN(code);
							// @v10.8.1 is_valid = SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, code, code.Len()); // @v8.7.4
							is_valid = (nta.Has(SNTOK_RU_INN) > 0.0f); // @v10.8.1 
						}
						else
							is_valid = CheckCorrAcc(code, bic);
					}
					ok = 1;
				}
				if(ok > 0) {
					pCellStyle->Flags = 0;
					if(is_valid)
						pCellStyle->Color = GetColorRef(SClrAqua);
					else
						pCellStyle->Color = GetColorRef(SClrCoral);
				}
			}
			*/
		}
	}
	return ok;
#endif // } 0
}

/*virtual*/void PPViewPerson::PreprocessBrowser(PPViewBrowser * pBrw)
{
	SString temp_buf;
	if(pBrw) {
		BrowserDef * p_def = pBrw->getDef();
		CALLPTRMEMB(p_def, SetCapHeight(2));
		if(Filt.Flags & PersonFilt::fShowFiasRcgn && Filt.IsLocAttr() && P_TempPsn) {
			if(p_def) {
				pBrw->InsColumn(-1, "FIAS ADDR",  8, 0, MKSFMT(32, 0), 0);
				pBrw->InsColumn(-1, "FIAS HOUSE", 9, 0, MKSFMT(32, 0), 0);
			}
		}
		else if(Filt.Flags & PersonFilt::fLocTagF && Filt.GetAttribType()) { // @v12.1.11
			if(!P_Ct) {
				if(Filt.GetAttribType() == PPPSNATTR_REGISTER) {
					pBrw->InsColumn(-1, "@series", 13, 0, MKSFMT(32, 0), 0);
					pBrw->InsColumn(-1, "@number", 12, 0, MKSFMT(32, 0), 0);
					pBrw->InsColumn(-1, "@register_dt", 14, 0, DATF_GERMAN, 0);
					pBrw->InsColumn(-1, "@register_expiry", 15, 0, DATF_GERMAN, 0);
				}
				else if(Filt.GetAttribType() == PPPSNATTR_TAG) {
					pBrw->InsColumn(-1, "@tag", 12, 0, MKSFMT(128, 0), 0);
				}
				else if(Filt.GetAttribType() == PPPSNATTR_EMAIL) {
					pBrw->InsColumn(-1, "@email", 11, 0, MKSFMT(128, 0), 0);
				}
				else if(Filt.GetAttribType() == PPPSNATTR_PHONEADDR) {
					pBrw->InsColumn(-1, "@phone", 10, 0, MKSFMT(128, 0), 0);
				}
			}
		}
		// @v12.2.2 {
		if(Filt.Flags & PersonFilt::fCliActivityStats && !Filt.GetAttribType() && !P_Ct) {
			if(p_def) {
				uint   column_idx = p_def->getCount();
				p_def->SetCapHeight(3);
				pBrw->InsColumn(-1, "@clientactivity_eventcount",    6, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0);
				pBrw->InsColumn(-1, "@clientactivity_gapdaysavg",    7, 0, MKSFMTD(0, 1, NMBF_NOZERO), 0);
				pBrw->InsColumn(-1, "@clientactivity_gapdaysstddev", 8, 0, MKSFMTD(0, 1, NMBF_NOZERO), 0);
				pBrw->InsColumn(-1, "delay-days", 10, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0); // @v12.2.4
				pBrw->InsColumn(-1, "delay-sd",   11, 0, MKSFMTD(0, 3, NMBF_NOZERO), 0); // @v12.2.4
				pBrw->InsColumn(-1, "@clientactivity_state",         9, 0, 0, 0);
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = 6;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("clientactivity", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
		}
		// } @v12.2.2 
		//@erik 02.06.2019{
//		if(P_Ct && Filt.AttribType == PPPSNATTR_ALLADDR) {
//			BrowserDef * p_def = pBrw->getDef();
//			if(p_def) {
//				pBrw->InsColumn(-1, "Address", 3, 0, MKSFMT(32, 0), 0);
//			}
//		}
		//@erik}
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

/*static*/bool PPViewPerson::CheckClientActivityState(PPID personID, long filtFlags, const LAssocArray * pClientActivityStateList)
{
	bool   ok = false;
	//const  PPID   person_id = static_cast<long>(params[0].lval);
	//const  PPID   filt_flags = static_cast<long>(params[1].lval);
	//const  LAssocArray * p_clientactivitystatelist = reinterpret_cast<const LAssocArray *>(params[2].ptrval);
	if(filtFlags && (filtFlags & 0xffff) != 0xffff && pClientActivityStateList) {
		uint caslp = 0;
		if(pClientActivityStateList->Search(personID, &caslp)) {
			const long state = pClientActivityStateList->at(caslp).Val;
			switch(state & ~PPObjPerson::ClientActivityState::stfNewClient) {
				case PPObjPerson::ClientActivityState::stNoData:
					if(filtFlags & (1 << PPObjPerson::ClientActivityState::stNoData))
						ok = true;
					break;
				case PPObjPerson::ClientActivityState::stDelayedTa:
					if(filtFlags & (1 << PPObjPerson::ClientActivityState::stDelayedTa))
						ok = true;
					break;
				case PPObjPerson::ClientActivityState::stHopelesslyDelayedTa:
					if(filtFlags & (1 << PPObjPerson::ClientActivityState::stHopelesslyDelayedTa))
						ok = true;
					break;
				case PPObjPerson::ClientActivityState::stRegularTa:
					if(filtFlags & (1 << PPObjPerson::ClientActivityState::stRegularTa))
						ok = true;
					break;
			}
			if(state & PPObjPerson::ClientActivityState::stfNewClient) {
				if(filtFlags & PPObjPerson::ClientActivityState::stfNewClient)
					ok = true;
			}
		}
		else {
			if(filtFlags & (1 << PPObjPerson::ClientActivityState::stUndef))
				ok = true;
		}
	}
	else
		ok = true;
	return ok;
}

static IMPL_DBE_PROC(dbqf_person_check_clientactivitystatus_iip)
{
	const  PPID   person_id = static_cast<long>(params[0].lval);
	const  PPID   filt_flags = static_cast<long>(params[1].lval);
	const  LAssocArray * p_clientactivitystatelist = reinterpret_cast<const LAssocArray *>(params[2].ptrval);
	bool   ok = PPViewPerson::CheckClientActivityState(person_id, filt_flags, p_clientactivitystatelist);
	result->init(static_cast<long>(ok));
}

int PPViewPerson::DynFuncCheckClientActivityStatus = DbqFuncTab::RegisterDynR(BTS_INT, dbqf_person_check_clientactivitystatus_iip, 3, BTS_INT, BTS_INT, BTS_PTR);

DBQuery * PPViewPerson::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	PersonTbl * p  = 0;
	TempPersonTbl  * tmp_pt = 0;
	PersonKindTbl  * k  = 0;
	LocationTbl    * lt = 0;
	uint   brw_id = 0;
	SString title_buf;
	if(P_Ct)
		brw_id = BROWSER_PERSONCT;
	else if(Filt.Flags & PersonFilt::fCliActivityStats) // @v12.2.8
		brw_id = BROWSER_CLIACTIVITY;
	else if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR)
		brw_id = BROWSER_PERSONATTR_STDALNADDR;
	else if(oneof4(Filt.GetLocAttribType(), PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DUPDLVRADDR))
		brw_id = BROWSER_PERSONATTR_DLVRADDR;
	else if(Filt.GetAttribType() == PPPSNATTR_PHONEADDR)
		brw_id = BROWSER_PERSONATTR_PHONEADDR;
	else if(Filt.GetAttribType() == PPPSNATTR_EMAIL)
		brw_id = BROWSER_PERSONATTR_EMAIL;
	else if(Filt.GetAttribType() == PPPSNATTR_BNKACCT)
		brw_id = BROWSER_PERSONATTR_BNKACCT;
	else if(Filt.GetAttribType() == PPPSNATTR_REGISTER)
		brw_id = BROWSER_PERSONATTR_REGISTER;
	else if(Filt.GetAttribType() == PPPSNATTR_TAG)
		brw_id = BROWSER_PERSONATTR_TAG;
	else if(Filt.Flags & PersonFilt::fTagsCrsstab)
		brw_id = BROWSER_PERSONCT;
	else
		brw_id = BROWSER_PERSON;
	if(Filt.PersonKindID)
		GetObjectName(PPOBJ_PERSONKIND, Filt.PersonKindID, title_buf);
	if(Filt.GetAttribType()) {
		title_buf.CatDivIfNotEmpty('-', 1);
		PPID   reg_id = Filt.RegTypeID;
		if(reg_id) {
			SArray * p_ary = CreateExtRegList(&Filt, &reg_id, 0/*flags sercfXXX*/);
			if(p_ary) {
				uint   pos = 0;
				const  ExtRegEntry * p_ere = p_ary->lsearch(&reg_id, &pos, CMPF_LONG) ? static_cast<const ExtRegEntry *>(p_ary->at(pos)) : 0;
				if(p_ere)
					title_buf.Cat(p_ere->Name);
				delete p_ary;
			}
		}
	}
	ASSIGN_PTR(pBrwId,    brw_id);
	ASSIGN_PTR(pSubTitle, title_buf);
	if(P_Ct) {
		q = PPView::CrosstabDbQueryStub;
	}
	else {
		DBE    cq;
		DBE    dbe_city;
		DBE    dbe_phone;
		DBE    dbe_addr;
		DBE    dbe_raddr;
		DBE    dbe_bnkacct;
		DBE    dbe_bnkname;
		DBE    dbe_regser;
		DBE    dbe_fiasadrguid;
		DBE    dbe_fiashseguid;
		DBE    dbe_addrtype; // @v12.1.11
		DBE    dbe_email; // @v12.1.11
		DBE    dbe_check_clientactivitystate; // @v12.2.2
		DBQ  * dbq = 0;
		int    tbl_count = 0;
		DBTable * tbl_l[12];
		memzero(tbl_l, sizeof(tbl_l));
		if(P_TempPsn) {
			THROW(CheckTblPtr(tmp_pt = new TempPersonTbl(P_TempPsn->GetName())));
			tbl_l[tbl_count++] = tmp_pt;
			if(!oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
				THROW(CheckTblPtr(p = new PersonTbl));
				tbl_l[tbl_count++] = p;
				if(Filt.GetLocAttribType() == PPPSNATTR_ALLADDR && Filt.Flags & PersonFilt::fShowHangedAddr)
					dbq = & (p->ID += tmp_pt->ID);
				else
					dbq = & (p->ID == tmp_pt->ID);
			}
		}
		else {
			THROW(CheckTblPtr(p = new PersonTbl));
			if(Filt.PersonKindID) {
				THROW(CheckTblPtr(k = new PersonKindTbl));
				dbq = & (k->KindID == Filt.PersonKindID && p->ID == k->PersonID);
				tbl_l[tbl_count++] = k;
			}
			tbl_l[tbl_count++] = p;
			dbq = ppcheckfiltid(dbq, p->Status, Filt.Status);
			dbq = ppcheckfiltid(dbq, p->CatID, Filt.PersonCategoryID);
			dbq = ppcheckflag(dbq, p->Flags, PSNF_NOVATAX, BIN(Filt.Flags & PersonFilt::fVatFree));
			dbq = ppcheckflag(dbq, p->Flags, PSNF_HASIMAGES, BIN(Filt.Flags & PersonFilt::fHasImages));
			dbq = ppcheckfiltid(dbq, p->ID, Filt.PersonID);
			if(Filt.CityID) {
				THROW(CheckTblPtr(lt = new LocationTbl));
				tbl_l[tbl_count++] = lt;
				dbq = & (*dbq && lt->ID == p->MainLoc);
				{
					cq.init();
					cq.push(lt->CityID);
					DBConst dbc_long;
					dbc_long.init(Filt.CityID);
					cq.push(dbc_long);
					cq.push(static_cast<DBFunc>(PPDbqFuncPool::IdWorldIsMemb));
					dbq = & (*dbq && cq == 1L);
				}
			}
		}
		if(Filt.ClientActivityStateFlags && (Filt.ClientActivityStateFlags & 0xffff) != 0xffff && P_ClientActivityStateList) {
			dbe_check_clientactivitystate.init();
			dbe_check_clientactivitystate.push(p->ID);
			dbe_check_clientactivitystate.push(dbconst(static_cast<long>(Filt.ClientActivityStateFlags)));
			dbe_check_clientactivitystate.push(dbconst(static_cast<const void *>(P_ClientActivityStateList)));
			dbe_check_clientactivitystate.push(static_cast<DBFunc>(DynFuncCheckClientActivityStatus));
			dbq = &(*dbq && dbe_check_clientactivitystate == 1L);
		}
		if(Filt.Flags & PersonFilt::fLocTagF && Filt.GetAttribType()) { // @v12.1.11
			PPDbqFuncPool::InitObjNameFunc(dbe_city, PPDbqFuncPool::IdObjNameWorld,  tmp_pt->CityID);
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr, tmp_pt->AddressP, &StrPool);
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiasadrguid,  tmp_pt->FiasAddrGuidP, &StrPool);
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiashseguid,  tmp_pt->FiasHouseGuidP, &StrPool);
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_addrtype,  tmp_pt->AddrTypeP, &StrPool); // @v12.1.11
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone, tmp_pt->PhoneP, &StrPool); // @v12.1.11
			PPDbqFuncPool::InitStrPoolRefFunc(dbe_email, tmp_pt->EMailP, &StrPool); // @v12.1.11
			q = & select(
				p->ID,                // #0
				tmp_pt->TabID,        // #1 ИД адреса
				p->Name,              // #2 Наименование персоналии
				dbe_addr,             // #3 Строка адреса
				dbe_bnkacct,          // #4 Код из адреса доставки
				/*tmp_pt->RegNumber*/dbe_addrtype, // #5 Тип адреса (юридический | физический | доставки)
				dbe_city,             // #6
				p->Flags,             // #7
				dbe_fiasadrguid,      // #8
				dbe_fiashseguid,      // #9
				dbe_phone,            // #10 @v12.1.11
				dbe_email,            // #11 @v12.1.11
				tmp_pt->RegNumber,    // #12 @v12.1.11 либо текстовое представление тега (PPPSNATTR_TAG), либо номер регистрационного документа (PPPSNATTR_REGISTER)
				0L);
			if(Filt.GetAttribType() == PPPSNATTR_REGISTER) {
				PPDbqFuncPool::InitStrPoolRefFunc(dbe_regser,  tmp_pt->RegSerialP, &StrPool); // @v12.1.11
				q->addField(dbe_regser);          // #13 
				q->addField(tmp_pt->RegInitDate); // #14
				q->addField(tmp_pt->RegExpiry);   // #15
			}
			q->from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], 0L);
		}
		else {
			switch(Filt.GetAttribType()) {
				case PPPSNATTR_PHONEADDR:
					{
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone, tmp_pt->PhoneP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr,  tmp_pt->AddressP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_raddr, tmp_pt->RAddressP, &StrPool);
						q = & select(
							p->ID,                         // #0 
							p->Name,                       // #1
							/*tmp_pt->Phone*/dbe_phone,    // #2  
							/*tmp_pt->Address*/dbe_addr,   // #3
							/*tmp_pt->RAddress*/dbe_raddr, // #4
							p->Flags,                      // #5
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
				case PPPSNATTR_EMAIL:
					{
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone, tmp_pt->EMailP, &StrPool); // @v12.1.11 tmp_pt->PhoneP-->tmp_pt->EMailP
						q = & select(
							p->ID,                      // #0
							p->Name,                    // #1 
							/*tmp_pt->Phone*/dbe_phone, // #2
							p->Flags,                   // #3
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
				case PPPSNATTR_ALLADDR:
				case PPPSNATTR_DLVRADDR:
				case PPPSNATTR_DUPDLVRADDR:
					{
						PPDbqFuncPool::InitObjNameFunc(dbe_city, PPDbqFuncPool::IdObjNameWorld,  tmp_pt->CityID);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr, tmp_pt->AddressP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiasadrguid,  tmp_pt->FiasAddrGuidP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiashseguid,  tmp_pt->FiasHouseGuidP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_addrtype,  tmp_pt->AddrTypeP, &StrPool); // @v12.1.11
						q = & select(
							p->ID,                // #0
							tmp_pt->TabID,        // #1 ИД адреса
							p->Name,              // #2 Наименование персоналии
							dbe_addr,             // #3 Строка адреса
							dbe_bnkacct,          // #4 Код из адреса доставки
							/*tmp_pt->RegNumber*/dbe_addrtype, // #5 Тип адреса (юридический | физический | доставки)
							dbe_city,             // #6
							p->Flags,             // #7
							dbe_fiasadrguid,      // #8
							dbe_fiashseguid,      // #9
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], 0L);
					}
					break;
				case PPPSNATTR_HANGEDADDR:
				case PPPSNATTR_STANDALONEADDR:
					{
						PPDbqFuncPool::InitObjNameFunc(dbe_city, PPDbqFuncPool::IdObjNameWorld,  tmp_pt->CityID);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr, tmp_pt->AddressP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone,  tmp_pt->PhoneP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkname,  tmp_pt->BnkNameP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_addrtype,  tmp_pt->AddrTypeP, &StrPool); // @v12.1.11
						q = & select(
							tmp_pt->ID,        // #0
							tmp_pt->TabID,     // #1 ИД адреса
							tmp_pt->Name,      // #2 Наименование персоналии
							dbe_addr,          // #3 Строка адреса
							dbe_bnkacct,       // #4 Код из адреса доставки
							/*tmp_pt->RegNumber*/dbe_addrtype, // #5 Тип адреса (юридический | физический | доставки)
							dbe_city,          // #6
							dbe_phone,         // #7 Телефон (ассоциированный с адресом)
							dbe_bnkname,       // #8 Контакт (ассоциированный с адресом)
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], 0L);
					}
					break;
				case PPPSNATTR_BNKACCT:
					{
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkname,  tmp_pt->BnkNameP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone,  tmp_pt->PhoneP, &StrPool);
						q = & select(
							p->ID,               // #0
							tmp_pt->TabID,       // #1
							p->Name,             // #2
							dbe_bnkname,         // #3
							dbe_bnkacct,         // #4
							tmp_pt->RegNumber,   // #5 Тип счета
							tmp_pt->RegInitDate, // #6 Дата открытия //
							dbe_phone,           // #7 БИК банка
							p->Flags,            // #8
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
				case PPPSNATTR_REGISTER:
					{
						PPDbqFuncPool::InitStrPoolRefFunc(dbe_regser,  tmp_pt->RegSerialP, &StrPool);
						q = & select(
							p->ID,               // #0
							p->Name,             // #1
							dbe_regser,          // #2  
							tmp_pt->RegNumber,   // #3 
							tmp_pt->RegInitDate, // #4
							tmp_pt->RegExpiry,   // #5
							p->Flags,            // #6
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
				case PPPSNATTR_TAG:
					{
						q = & select(
							p->ID,             // #0
							p->Name,           // #1
							tmp_pt->RegNumber, // #2
							p->Flags,          // #3 
							0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
				default:
					{
						DBE    dbe_status;
						DBE    dbe_cat;
						DBE    dbe_memo; // @v11.1.12
						PPDbqFuncPool::InitObjNameFunc(dbe_status, PPDbqFuncPool::IdObjNamePersonStatus, p->Status);
						PPDbqFuncPool::InitObjNameFunc(dbe_cat,    PPDbqFuncPool::IdObjNamePersonCat,    p->CatID);
						PPDbqFuncPool::InitObjNameFunc(dbe_memo,   PPDbqFuncPool::IdObjMemoPerson,       p->ID); // @v11.1.12
						q = & select(
							p->ID,      // #0
							p->Name,    // #1 
							dbe_status, // #2
							dbe_cat,    // #3
							dbe_memo,   // #4 @v11.1.12 p->Memo-->dbe_memo
							p->Flags,   // #5
							0L);
						// @v12.2.2 {
						if(Filt.Flags & PersonFilt::fCliActivityStats) {
							DBE    dbe_cas_evntcount;
							DBE    dbe_cas_gapdaysavg;
							DBE    dbe_cas_gapdaysstddev;
							DBE    dbe_cas_cdd;  // @v12.2.4
							DBE    dbe_cas_cdsd; // @v12.2.4
							DBE    dbe_ca_state;
							{
								dbe_cas_evntcount.init();
								dbe_cas_evntcount.push(p->ID);
								dbe_cas_evntcount.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_cas_evntcount.push(dbconst(PPObjPerson::casiEventCount));
								dbe_cas_evntcount.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityStatisticsIndicator));
								q->addField(dbe_cas_evntcount); // #6
							}
							{
								dbe_cas_gapdaysavg.init();
								dbe_cas_gapdaysavg.push(p->ID);
								dbe_cas_gapdaysavg.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_cas_gapdaysavg.push(dbconst(PPObjPerson::casiGapDaysAvg));
								dbe_cas_gapdaysavg.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityStatisticsIndicator));
								q->addField(dbe_cas_gapdaysavg); // #7
							}
							{
								dbe_cas_gapdaysstddev.init();
								dbe_cas_gapdaysstddev.push(p->ID);
								dbe_cas_gapdaysstddev.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_cas_gapdaysstddev.push(dbconst(PPObjPerson::casiGapDaysStdDev));
								dbe_cas_gapdaysstddev.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityStatisticsIndicator));
								q->addField(dbe_cas_gapdaysstddev); // #8
							}
							{
								dbe_ca_state.init();
								dbe_ca_state.push(p->ID);
								dbe_ca_state.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_ca_state.push(dbconst(Filt.NewCliPeriod.low));
								dbe_ca_state.push(dbconst(Filt.NewCliPeriod.upp));
								dbe_ca_state.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityState));
								q->addField(dbe_ca_state); // #9
							}
							// @v12.2.4 {
							{
								dbe_cas_cdd.init();
								dbe_cas_cdd.push(p->ID);
								dbe_cas_cdd.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_cas_cdd.push(dbconst(PPObjPerson::casiCurrentDelayDays));
								dbe_cas_cdd.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityStatisticsIndicator));
								q->addField(dbe_cas_cdd); // #10
							}
							{
								dbe_cas_cdsd.init();
								dbe_cas_cdsd.push(p->ID);
								dbe_cas_cdsd.push(dbconst(Filt.ClientActivityEvalDate));
								dbe_cas_cdsd.push(dbconst(PPObjPerson::casiCurrentDelaySd));
								dbe_cas_cdsd.push(static_cast<DBFunc>(PPDbqFuncPool::IdClientActivityStatisticsIndicator));
								q->addField(dbe_cas_cdsd); // #11
							}
							// } @v12.2.4
						}
						// } @v12.2.2 
						q->from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
					}
					break;
			}
		}
		q->where(*dbq).orderBy((tmp_pt) ? tmp_pt->Name : p->Name, 0L);
		THROW(CheckQueryPtr(q));
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete lt;
			delete k;
			delete p;
			delete tmp_pt;
		}
	ENDCATCH
	return q;
}

int PPViewPerson::Recover()
{
	int    ok = -1;
	PPLogger logger;
	SString fmt_buf;
	SString msg_buf;
	SString addr_buf;
	SString temp_buf;
	PPIDArray psn_list;
	PPIDArray conflict_owner_addr_list; // Список адресов доставки, принадлежащих нескольким владельцам
	LAssocArray dlvr_addr_list; // Список ассоциаций {DlvrAddrID; PersonID}
	LAssocArray invowner_addr_list; // Список ассоциация адресов доставки, имеющий не верного владельца {DlvrAddrID; CorrectOwnerID}
	PersonViewItem item;
	PPLocationPacket loc_pack;
	PPWaitStart();
	for(InitIteration(); NextIteration(&item) > 0;) {
		PPWaitPercent(GetCounter());
		if(psn_list.addUnique(item.ID) > 0) {
			PPPersonPacket pack;
			THROW(PsnObj.GetPacket(item.ID, &pack, 0) > 0);
			for(uint ap = 0; pack.EnumDlvrLoc(&ap, &loc_pack);) {
				dlvr_addr_list.Add(loc_pack.ID, item.ID, 0);
				if(loc_pack.OwnerID != pack.Rec.ID) {
					invowner_addr_list.Add(loc_pack.ID, item.ID, 0);
				}
			}
		}
	}
	{
		PPObjBill * p_bobj = BillObj;
		dlvr_addr_list.Sort();
		LAssocArray bill_dlvr_addr_list;
		BillTbl::Rec bill_rec;
		LocationTbl::Rec loc_rec;
		PPIDArray bill_list;
		PPIDArray ar_list;
		PPIDArray hanged_addr_list;
		const uint c = dlvr_addr_list.getCount();
		p_bobj->P_Tbl->GetDlvrAddrList(&bill_dlvr_addr_list);
		for(uint bdaidx = 0; bdaidx < bill_dlvr_addr_list.getCount(); bdaidx++) {
			const  PPID bill_id = bill_dlvr_addr_list.at(bdaidx).Key;
			const  PPID addr_id = bill_dlvr_addr_list.at(bdaidx).Val;
			if(addr_id && !hanged_addr_list.lsearch(addr_id)) {
				if(PsnObj.LocObj.Search(addr_id, &loc_rec) < 0) {
					if(p_bobj->Search(bill_id, &bill_rec) > 0) {
						hanged_addr_list.add(addr_id);
						//PPTXT_LOG_HANGEDDLVRADDRINBILL            "Документ '@zstr' ссылается на несуществующий адрес #@long как на адрес доставки"
						PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName, temp_buf);
						if(PPLoadText(PPTXT_LOG_HANGEDDLVRADDRINBILL, fmt_buf)) {
							PPFormat(fmt_buf, &msg_buf, temp_buf.cptr(), addr_id);
							bill_dlvr_addr_list.GetListByVal(addr_id, bill_list);
							if(bill_list.getCount() > 1) {
								bill_list.sortAndUndup();
								msg_buf.Space().CatChar('(').Cat("there are").Space().Cat(bill_list.getCount()-1).Space().Cat("more documents").CatChar(')');
							}
							logger.Log(msg_buf);
						}
					}
				}
			}
		}
		if(c) {
			uint i = c;
			do {
				LAssoc & r_assc = dlvr_addr_list.at(--i);
				PPID   prev_addr_id = ((i+1) < c) ? dlvr_addr_list.at(i+1).Key : 0;
				if(prev_addr_id && r_assc.Key == prev_addr_id) {
					conflict_owner_addr_list.add(r_assc.Key);
					bill_list.clear();
					bill_dlvr_addr_list.GetListByVal(r_assc.Key, bill_list);
					ar_list.clear();
					for(uint j = 0; j < bill_list.getCount(); j++) {
						const  PPID bill_id = bill_list.get(j);
						if(p_bobj->Search(bill_id, &bill_rec) > 0)
							ar_list.add(bill_rec.Object);
					}
					if(PsnObj.LocObj.GetPacket(r_assc.Key, &loc_pack) > 0) {
						LocationCore::GetAddress(loc_pack, 0, addr_buf);
						temp_buf.Z().CatChar('#').Cat(loc_pack.ID).Space().Cat(addr_buf);
						addr_buf = temp_buf;
					}
					/*
					PPTXT_LOG_MANYLINGADDR_ONEBILL  "Адрес доставки '@zstr' принадлежит нескольким персоналиям при этом все документы с этим адресом относятся к персоналии '@person'"
					PPTXT_LOG_MANYLINGADDR_MANYBILL "Адрес доставки '@zstr' принадлежит нескольким персоналиям при этом документы с этим адресом относятся к разным персоналиям"
					PPTXT_LOG_MANYLINGADDR_NOBILL   "Адрес доставки '@zstr' принадлежит нескольким персоналиям при этом нет документов с этим адресом"
					*/
					PPID   psn_id = 0;
					uint   str_id = 0;
					if(ar_list.getCount() == 1) {
						psn_id = ObjectToPerson(ar_list.get(0), 0);
						str_id = PPTXT_LOG_MANYLINGADDR_ONEBILL;
					}
					else if(ar_list.getCount())
						str_id = PPTXT_LOG_MANYLINGADDR_MANYBILL;
					else
						str_id = PPTXT_LOG_MANYLINGADDR_NOBILL;
					if(PPLoadText(str_id, fmt_buf)) {
						PPFormat(fmt_buf, &msg_buf, addr_buf.cptr(), psn_id);
						logger.Log(msg_buf);
					}
				}
			} while(i);
		}
		if(invowner_addr_list.getCount()) {
			PPTransaction tra(1);
			THROW(tra);
			conflict_owner_addr_list.sortAndUndup();
			for(uint i = 0; i < invowner_addr_list.getCount(); i++) {
				PPID addr_id = invowner_addr_list.at(i).Key;
				PPID valid_owner_id = invowner_addr_list.at(i).Val;
				THROW(PsnObj.LocObj.GetPacket(addr_id, &loc_pack) > 0);
				{
					LocationCore::GetAddress(loc_pack, 0, addr_buf);
					temp_buf.Z().CatChar('#').Cat(loc_pack.ID).Space().Cat(addr_buf);
					addr_buf = temp_buf;
				}
				const  PPID inv_owner_id = loc_pack.OwnerID;
				if(!conflict_owner_addr_list.bsearch(addr_id)) {
					if(inv_owner_id != valid_owner_id) {
						// PPTXT_LOG_DLVRADDINVOWNER       "Адрес доставки '@zstr' ссылается не на того владельца (@person), которому принадлежит (@person)"
						loc_pack.OwnerID = valid_owner_id;
						THROW(PsnObj.LocObj.PutPacket(&addr_id, &loc_pack, 0));
						if(PPLoadText(PPTXT_LOG_DLVRADDINVOWNER, fmt_buf)) {
							PPFormat(fmt_buf, &msg_buf, addr_buf.cptr(), inv_owner_id, valid_owner_id);
							logger.Log(msg_buf);
						}
					}
				}
				else {
					// PPTXT_LOG_DLVRADDINVOWNER_NC    "Адрес доставки '@zstr' ссылается не на того владельца (@person), которому принадлежит (@person): исправление невозможно из-за конфликта между владельцами"
					if(PPLoadText(PPTXT_LOG_DLVRADDINVOWNER_NC, fmt_buf)) {
						PPFormat(fmt_buf, &msg_buf, addr_buf.cptr(), inv_owner_id, valid_owner_id);
						logger.Log(msg_buf);
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}

int PPViewPerson::RemoveHangedAddr()
{
	int    ok = -1;
	if(Filt.GetAttribType() == PPPSNATTR_HANGEDADDR && P_TempPsn && CONFIRMCRIT(PPCFM_DELALLHANGEDADDR)) {
		PPIDArray addr_list;
		PPWaitStart();
		{
			BtrDbKey k_;
			BExtQuery q(P_TempPsn, 0);
			q.select(P_TempPsn->TabID, P_TempPsn->AddressP, 0);
			for(q.initIteration(0, k_, spFirst); q.nextIteration() > 0;) {
				addr_list.addUnique(P_TempPsn->data.TabID);
			}
		}
		if(addr_list.getCount()) {
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; i < addr_list.getCount(); i++) {
				THROW(PPCheckUserBreak());
				THROW(PsnObj.LocObj.RemoveObjV(addr_list.get(i), 0, PPObject::no_wait_indicator, 0));
				PPWaitPercent(i+1, addr_list.getCount());
			}
			THROW(tra.Commit());
			ok = 1;
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewPerson::SendMail(PPID mailAccId, const StrAssocArray * pMailList, PPLogger * pLogger)
{
	int    ok = -1;
	SendMailDialog * p_dlg = 0;
	if(pMailList && pMailList->getCount() > 0) {
		SendMailDialog::Rec data;
		THROW(CheckDialogPtr(&(p_dlg = new SendMailDialog())));
		data.MailAccID = mailAccId;
		data.AddrList = *pMailList;
		p_dlg->setDTS(&data);
		for(int valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
			if(p_dlg->getDTS(&data) > 0)
				ok = valid_data = 1;
			else
				PPError();
		}
		if(ok > 0) {
			PPWaitStart();
			for(uint i = 0; i < pMailList->getCount(); i++) {
				PPSmsSender::FormatMessageBlock fmb;
				fmb.PersonID = pMailList->Get(i).Id;
				const  char * p_mail_addr = pMailList->Get(i).Txt;
				SString text, subj;
				if(i && data.Delay > 0 && data.Delay <= (SlConst::SecsPerDay * 1000)) {
					SDelay(data.Delay);
				}
				PPSmsSender::FormatMessage_(data.Text, text, &fmb);
				PPSmsSender::FormatMessage_(data.Subj, subj, &fmb);
				subj.Transf(CTRANSF_INNER_TO_UTF8);
				text.Transf(CTRANSF_INNER_TO_UTF8);
				THROW(::SendMail(subj, text, pMailList->Get(i).Txt, data.MailAccID, &data.FilesList, pLogger));
			}
			PPWaitStop();
		}
	}
	else if(pLogger) {
		SString msg;
		PPLoadError(PPERR_MAILACCOUNTSNF, msg.Z(), 0);
		pLogger->Log(msg);
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int PPViewPerson::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString phone_buf, contact_buf, addr_buf;
	PPLogger logger;
	if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR) {
		PPUhttClient uhtt_cli;
		PPWaitStart();
		THROW(uhtt_cli.Auth());
		{
			PersonViewItem item;
			for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
				PPID loc_id = item.AttrItem.TabID;
				LocationTbl::Rec loc_rec;
				if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0) {
					long    uhtt_loc_id = 0;
					UhttLocationPacket uhtt_loc_pack, ret_loc_pack;
					LocationCore::GetExField(&loc_rec, LOCEXSTR_PHONE, phone_buf);
					LocationCore::GetExField(&loc_rec, LOCEXSTR_CONTACT, contact_buf);
					LocationCore::GetAddress(loc_rec, 0, addr_buf);
					loc_text_buf.Z();
					if(loc_rec.Code[0])
						loc_text_buf.Cat(loc_rec.Code);
					if(phone_buf.NotEmpty())
						loc_text_buf.CatDivIfNotEmpty(';', 2).Cat(phone_buf);
					if(contact_buf.NotEmpty())
						loc_text_buf.CatDivIfNotEmpty(';', 2).Cat(contact_buf);
					if(addr_buf.NotEmptyS())
						loc_text_buf.CatDivIfNotEmpty(';', 2).Cat(addr_buf);

					//PPTXT_UHTTEXPLOC_FOUND      "На сервере Universe-HTT адрес '@zstr' найден по коду"
					//PPTXT_UHTTEXPLOC_EXPORTED   "Адрес '@zstr' экспортирован на сервер Universe-HTT"
					//PPTXT_UHTTEXPLOC_CODEASSGN  "Адресу '@zstr' присвоен код с сервера Universe-HTT"
					//PPTXT_UHTTEXPLOC_EGETBYCOD  "Ошибка обратного получения адреса '@zstr' по идентификатору Universe-HTT: @zstr"
					//PPTXT_UHTTEXPLOC_EEXPORT    "Ошибка экспорта адреса '@zstr' на сервер Universe-HTT: @zstr"
					int   dont_update = 0;
					if(loc_rec.Code[0] && uhtt_cli.GetLocationByCode(loc_rec.Code, ret_loc_pack) > 0) {
						uhtt_loc_id = ret_loc_pack.ID;
						if(addr_buf.NotEmpty() && ret_loc_pack.Address.IsEmpty())
							dont_update = 0;
						else
							dont_update = 1;
					}
					if(dont_update) {
						uhtt_loc_id = ret_loc_pack.ID;
						//
						PPLoadText(PPTXT_UHTTEXPLOC_FOUND, fmt_buf);
						PPFormat(fmt_buf, &msg_buf, loc_text_buf.cptr());
						logger.Log(msg_buf);
					}
					else {
						uhtt_loc_pack.ID = uhtt_loc_id;
						uhtt_loc_pack.SetCode(loc_rec.Code);
						uhtt_loc_pack.SetPhone(phone_buf);
						uhtt_loc_pack.SetContact(contact_buf);
						if(LocationCore::GetExField(&loc_rec, LOCEXSTR_EMAIL, temp_buf) && temp_buf.NotEmptyS())
							uhtt_loc_pack.SetEMail(temp_buf);
						uhtt_loc_pack.Type = LOCTYP_ADDRESS;
						uhtt_loc_pack.Flags = LOCF_STANDALONE;
						if(addr_buf.NotEmpty()) {
							uhtt_loc_pack.SetAddress(addr_buf);
							uhtt_loc_pack.Flags |= LOCF_MANUALADDR;
						}
						uhtt_loc_pack.Latitude = loc_rec.Latitude;
						uhtt_loc_pack.Longitude = loc_rec.Longitude;
						int    cr = uhtt_cli.CreateStandaloneLocation(&uhtt_loc_id, uhtt_loc_pack);
						if(cr) {
							PPLoadText(PPTXT_UHTTEXPLOC_EXPORTED, fmt_buf);
							PPFormat(fmt_buf, &msg_buf, loc_text_buf.cptr());
							logger.Log(msg_buf);
							//
							if(uhtt_cli.GetLocationByID(uhtt_loc_id, ret_loc_pack) > 0) {
								ret_loc_pack.Code.CopyTo(loc_rec.Code, sizeof(loc_rec.Code));
								THROW(PsnObj.LocObj.PutRecord(&loc_id, &loc_rec, 1));
								//
								PPLoadText(PPTXT_UHTTEXPLOC_CODEASSGN, fmt_buf);
								PPFormat(fmt_buf, &msg_buf, loc_text_buf.cptr());
								logger.Log(msg_buf);
							}
							else {
								// Ошибка обратного получения адреса по идентификатору Universe-HTT
								temp_buf = uhtt_cli.GetLastMessage();
								PPLoadText(PPTXT_UHTTEXPLOC_CODEASSGN, fmt_buf);
								PPFormat(fmt_buf, &msg_buf, loc_text_buf.cptr(), temp_buf.cptr());
								logger.Log(msg_buf);
							}
						}
						else {
							// Ошибка экспорта адреса на сервер Universe-HTT
							temp_buf = uhtt_cli.GetLastMessage();
							PPLoadText(PPTXT_UHTTEXPLOC_EEXPORT, fmt_buf);
							PPFormat(fmt_buf, &msg_buf, loc_text_buf.cptr(), temp_buf.cptr());
							logger.Log(msg_buf);
						}
					}
				}
			}
		}
		PPWaitStop();
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int PPViewPerson::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	if(pHdr) {
		hdr = *static_cast<const BrwHdr *>(pHdr);
		if(P_Ct) {
			uint   tab_idx = pBrw ? pBrw->GetCurColumn() : 0;
			DBFieldList fld_list;
			if(P_Ct->GetIdxFields(hdr.ID, &fld_list) > 0)
				fld_list.GetValue(0, &hdr.ID, 0);
		}
	}
	else
		MEMSZERO(hdr);
	int ok = PPView::ProcessCommand(ppvCmd, &hdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_GRAPH: // @v12.2.6 @construction
				if(Filt.Flags & PersonFilt::fCliActivityStats) {
					if(hdr.ID)
						DrawClientActivityStatistics(hdr.ID);
				}
				break;
			case PPVCMD_CLIACTIVITYDETAILS:
				if(Filt.Flags & PersonFilt::fCliActivityStats) {
					if(hdr.ID) {
						ClientActivityDetailsFilt filt;
						filt.PersonID = hdr.ID;
						PPView::Execute(PPVIEW_CLIENTACTIVITYDETAILS, &filt, 0, 0);
					}
				}
				break;
			case PPVCMD_ADDITEM:
				{
					PPID id = 0;
					ok = AddItem(&id);
					if(ok > 0) {
						pBrw->Update();
						pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
						ok = -1; // Не надо обновлять таблицу после этого вызова
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				if(Filt.GetAttribType() == PPPSNATTR_HANGEDADDR) {
					PPID   loc_id = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id && CONFIRM(PPCFM_DELETE)) {
						if(!PsnObj.LocObj.PutRecord(&loc_id, 0, 1) || !UpdateHungedAddr(loc_id))
							PPError();
						else
							ok = 1;
					}
				}
				else if(hdr.ID)
					ok = DeleteItem(hdr.ID);
				break;
			case PPVCMD_DELETEALL:
				ok = -1;
				if(!oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					ok = UpdateList();
				}
				break;
			case PPVCMD_EDITITEM:
				if(oneof2(Filt.GetAttribType(), PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					PPID   loc_id = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id) {
						if(PsnObj.LocObj.Edit(&loc_id, 0) > 0) {
							if(!UpdateHungedAddr(loc_id))
								PPError();
							else
								ok = 1;
						}
					}
				}
				else if(hdr.ID != 0)
					ok = EditItem(hdr.ID);
				break;
			case PPVCMD_QUICKTAGEDIT: // @v11.2.8
				// В этой команде указатель pHdr занят под список идентификаторов тегов, соответствующих нажатой клавише
				// В связи с этим текущий элемент таблицы придется получить явным вызовом pBrw->getCurItem()
				//
				{
					const BrwHdr * p_row = static_cast<const BrwHdr *>(pBrw->getCurItem());
					ok = PPView::Helper_ProcessQuickTagEdit(PPObjID(PPOBJ_PERSON, p_row ? p_row->ID : 0), pHdr/*(LongArray *)*/);
				}
				break;
			case PPVCMD_TAGS:
				ok = -1;
				{
					PPObjID oid;
					if(Filt.IsLocAttr() && Filt.Flags & PersonFilt::fLocTagF)
						oid.Set(PPOBJ_LOCATION, static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0));
					else
						oid.Set(PPOBJ_PERSON, hdr.ID);
					if(oid.IsFullyDefined())
						ok = EditObjTagValList(oid.Obj, oid.Id, 0);
				}
				break;
			case PPVCMD_CURREG:
				if(Filt.IsLocAttr()) {
					PPID   _id_to_edit = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(_id_to_edit)
						ok = (PsnObj.LocObj.Edit(&_id_to_edit, 0) == cmOK) ? 1 : -1;
				}
				else {
					PPID   loc_id = 0;
					if(Filt.Flags & PersonFilt::fLocTagF && Filt.GetAttribType()) {
						loc_id = static_cast<PPID>(PTR32C(pHdr)[1]);
					}
					ok = EditRegs(hdr.ID, loc_id, 1);
				}
				break;
			/* @v12.2.8 case PPVCMD_DLVRADDREXFLDS:
				if(Filt.IsLocAttr()) {
					const PPID loc_id = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id)
						if((ok = EditDlvrAddrExtFlds(loc_id)) == 0)
							PPError();
				}
				break;*/
			case PPVCMD_RELATIONS:
			case PPVCMD_REVERSEREL:
				{
					PersonRelFilt filt;
					if(ppvCmd == PPVCMD_RELATIONS)
						filt.PrmrPersonID = hdr.ID;
					else
						filt.ScndPersonID = hdr.ID;
					ok = PPView::Execute(PPVIEW_PERSONREL, &filt, PPView::exefModeless, 0);
				}
				break;
			case PPVCMD_EDITFIXSTAFF:
				if(hdr.ID)
					StLObj.EditFixedStaffPost(hdr.ID);
				ok = -1;
				break;
			case PPVCMD_VIEWSCARDS:
				if(oneof2(Filt.GetAttribType(), PPPSNATTR_STANDALONEADDR, PPPSNATTR_HANGEDADDR)) {
					const  PPID loc_id = pHdr ? static_cast<const long *>(pHdr)[1] : 0;
					if(loc_id) {
						SCardFilt sc_flt;
						sc_flt.LocID = loc_id;
						ViewSCard(&sc_flt, 1);
					}
				}
				else if(hdr.ID) {
					SCardFilt sc_flt;
					sc_flt.PersonID = hdr.ID;
					ViewSCard(&sc_flt, 1);
				}
				ok = -1;
				break;
			case PPVCMD_SYSJ:
				if(oneof2(Filt.GetAttribType(), PPPSNATTR_STANDALONEADDR, PPPSNATTR_HANGEDADDR)) {
					const  PPID loc_id = pHdr ? static_cast<const long *>(pHdr)[1] : 0;
					if(loc_id) {
						ok = ViewSysJournal(PPOBJ_LOCATION, loc_id, 0);
					}
				}
				else if(hdr.ID)
					ok = ViewSysJournal(PPOBJ_PERSON, hdr.ID, 0);
				break;
			case PPVCMD_UNITEOBJ:
				if(oneof4(Filt.GetAttribType(), PPPSNATTR_ALLADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
					PPID   loc_id = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id)
						ok = PPObjPerson::ReplaceDlvrAddr(loc_id);
				}
				else
					ok = PPObjPerson::ReplacePerson(hdr.ID, Filt.PersonKindID);
				break;
			case PPVCMD_ARTICLES:
				if(hdr.ID) {
					ArticleFilt ar_flt;
					ar_flt.PersonID = hdr.ID;
					ok = ViewArticle(&ar_flt);
				}
				break;
			case PPVCMD_STAFFCAL:
				if(hdr.ID) {
					StaffCalFilt sc_flt;
					sc_flt.LinkObjType = PPOBJ_PERSON;
					sc_flt.LinkObjList.Add(hdr.ID);
					ShowObjects(PPOBJ_STAFFCAL, &sc_flt);
				}
				break;
			case PPVCMD_TRANSPOWNER:
				if(hdr.ID) {
					TransportFilt   t_filt;
					t_filt.OwnerID = hdr.ID;
					PPView::Execute(PPVIEW_TRANSPORT, &t_filt, PPView::exefModeless, 0);
				}
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID psn_kind = 0;
					if(pBrw && pBrw->GetToolbarComboData(&psn_kind) && Filt.PersonKindID != psn_kind) {
						Filt.PersonKindID = psn_kind;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_MOUSEHOVER:
				{
					const  int has_images = HasImage(pHdr);
					long   h = 0;
					if(pBrw->ItemByMousePos(&h, 0) && oneof2(h, 0, 1)) {
						int r = 0;
						SString buf;
						PPELinkArray phones_ary;
						PersonCore::GetELinks(hdr.ID, phones_ary);
						for(uint i = 0; i < phones_ary.getCount(); i++) {
							if(i != 0)
								buf.CR();
							CatObjectName(PPOBJ_ELINKKIND, phones_ary.at(i).KindID, buf);
							buf.CatDiv(':', 2).Cat(phones_ary.at(i).Addr);
							r = 1;
						}
						if(r > 0 || has_images) {
							SString img_path;
							if(has_images) {
								ObjLinkFiles link_files(PPOBJ_PERSON);
								link_files.Load(hdr.ID, 0L);
								link_files.At(0, img_path);
							}
							PPTooltipMessage(buf, img_path, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
								SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
						}
					}
				}
				break;
			case PPVCMD_SENDSMS:
				{
					uint   what = 0;
					if(SelectorDialog(DLG_DISPTYPE, CTL_DISPTYPE_WHAT, &what) > 0) {
						PPAlbatrossConfig albtr_cfg;
 						SString msg;
 						PPLogger logger;
 						StrAssocArray psn_list;
 						StrAssocArray phone_list;
 						if(PPAlbatrosCfgMngr::Get(&albtr_cfg) > 0) {
							GetSmsLists(psn_list, phone_list, what);
 							if(phone_list.getCount()) {
								if(what > 0)
									SendMail(albtr_cfg.Hdr.MailAccID, &phone_list, &logger);
								else if(!PPObjSmsAccount::BeginDelivery(albtr_cfg.Hdr.SmsAccID, psn_list, phone_list))
									PPError();
 							}
 							else {
								if(what == 0)
									PPLoadText(PPTXT_SMS_NOPHONENUMBER, msg);
								else
									PPLoadError(PPERR_MAILACCOUNTSNF, msg, 0);
 								logger.Log(msg);
 							}
 						}
 					}
 				}
				break;
			case PPVCMD_VIEWCCHECKS:
				ok = -1;
				if(Filt.GetAttribType() == PPPSNATTR_STANDALONEADDR) {
					const PPID loc_id = static_cast<PPID>(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id) {
						CCheckFilt cc_filt;
						cc_filt.DlvrAddrID = loc_id;
						ViewCCheck(&cc_filt, 0);
					}
				}
				break;
			case PPVCMD_CREATEAUTHFILE:
				ok = -1;
				CreateAuthFile(hdr.ID);
				break;
			case PPVCMD_EVENTS:         ok = ViewPersonEvents(hdr.ID); break;
			case PPVCMD_FOLDER:         ok = OpenClientDir(hdr.ID); break;
			case PPVCMD_REGISTERS:      ok = EditRegs(hdr.ID, 0, 0); break;
			case PPVCMD_AMOUNTS:        ok = PsnObj.EditAmountList(hdr.ID); break;
			case PPVCMD_ADDREL:         ok = AddRelation(hdr.ID); break;
			case PPVCMD_PRINT:          ok = Print(0); break;
			case PPVCMD_TOTAL:          
				ok = -1;
				ViewTotal(); 
				break;
			case PPVCMD_TASKS:          ok = ViewTasks(hdr.ID); break;
			case PPVCMD_TRANSMIT:       ok = Transmit(hdr.ID, 0); break;
			case PPVCMD_TRANSMKIND:     ok = Transmit(hdr.ID, 1); break;
			case PPVCMD_EXPORTVCARD:    ok = Transmit(hdr.ID, 2); break;
			case PPVCMD_EXPORTUHTT:     ok = ExportUhtt(); break;
			case PPVCMD_RMVHANGEDITEMS: ok = RemoveHangedAddr(); break;
			case PPVCMD_DORECOVER:      ok = Recover(); break;
		}
	}
	return ok;
}

int PPViewPerson::CreateAuthFile(PPID psnId)
{
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	const  char * p_auth_fname = "agauth.bin";
	S_GUID guid;
	SString temp_buf;
	SFile  file;
	Sdr_CPosAuth rec;
	rec._id = 1;
	p_dict->GetDbUUID(&guid);
	guid.ToStr(S_GUID::fmtIDL, temp_buf);
	temp_buf.CopyTo(rec.DbUUID, sizeof(rec.DbUUID));

	p_dict->GetDbSymb(temp_buf.Z());
	temp_buf.CopyTo(rec.DbSymb, sizeof(rec.DbSymb));

	rec.AgentID = psnId;
	GetObjectName(PPOBJ_PERSON, psnId, temp_buf);
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
	temp_buf.CopyTo(rec.AgentName, sizeof(rec.AgentName));
	{
		SString path;
		PPSecurPacket secur_pack;
		PPRef->LoadSecur(PPOBJ_USR, DS.GetTLA().Lc.UserID, &secur_pack);
		Reference::GetPassword(&secur_pack.Secur, rec.Password, sizeof(rec.Password));
		(temp_buf = rec.Password).Transf(CTRANSF_INNER_TO_UTF8);
		temp_buf.CopyTo(rec.Password, sizeof(rec.Password));

		(temp_buf = secur_pack.Secur.Name).Transf(CTRANSF_INNER_TO_UTF8);
		temp_buf.CopyTo(rec.UserName, sizeof(rec.UserName));

		PPGetFilePath(PPPATH_OUT, p_auth_fname, path);
		//@todo encrypt(&rec, sizeof(rec));
		THROW_SL(file.Open(path, SFile::mWrite));
		THROW_SL(file.Write(&rec, sizeof(rec)));
	}
	CATCHZOKPPERR
	file.Close();
	return ok;
}

int PPViewPerson::GetSmsLists(StrAssocArray & rPsnList, StrAssocArray & rPhoneList, uint what /*=0*/)
{
	long   i = 0;
 	SString temp_buf;
	SString phone;
	PersonViewItem item;
 	for(InitIteration(); NextIteration(&item) > 0;) {
		PPELinkArray elink_list;
		if(PersonCore::GetELinks(item.ID, elink_list) > 0) {
			if(what > 0)
				elink_list.GetPhones(1, phone, ELNKRT_EMAIL);
			else {
				elink_list.GetItem(PPELK_MOBILE, phone);
				if(phone.IsEmpty())
					elink_list.GetItem(PPELK_WORKPHONE, phone);
			}
 			if(phone.NotEmpty()) {
 				temp_buf.Z().Cat(item.ID);
 				rPsnList.Add(i, temp_buf);
				rPhoneList.Add((what > 0) ? item.ID : i, phone);
 				i++;
 			}
 		}
 	}
	return 1;
}

//
//
//
int ViewPerson(const PersonFilt * pFilt) { return PPView::Execute(PPVIEW_PERSON, pFilt, PPView::exefModeless, 0); }

int EditMainOrg()
{
	int    ok = -1;
	PersonFilt flt;
	PPObjPerson psn_obj;
	PPCommConfig cfg;
	PPID   id = 0;
	PersonTbl::Rec psn_rec;
	MEMSZERO(cfg);
	THROW(GetCommConfig(&cfg));
	flt.PersonKindID = PPPRK_MAIN;
	if(cfg.MainOrgID > 0 && psn_obj.P_Tbl->Search(cfg.MainOrgID) > 0)
		id = cfg.MainOrgID;
	else if(psn_obj.P_Tbl->SearchMainOrg(&psn_rec) > 0)
		id = psn_rec.ID;		//get id;
	cfg.MainOrgID = id;
	THROW(SetCommConfig(&cfg, 1));
	if(psn_obj.ExtEdit(&id, reinterpret_cast<void *>(flt.PersonKindID)) == cmOK) {
		THROW(GetCommConfig(&cfg));
		cfg.MainOrgID = id;
		THROW(SetCommConfig(&cfg, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
// Printing
//
int PPViewPerson::Print(const void *)
{
	int    rpt_id = 0;
	switch(Filt.GetAttribType()) {
		case PPPSNATTR_BNKACCT: rpt_id = REPORT_PSNATTRBNKACCT; break;
		case PPPSNATTR_PHONEADDR: rpt_id = REPORT_PSNATTRPHONEADDR; break;
		case PPPSNATTR_EMAIL:     rpt_id = REPORT_PSNATTREMAIL; break;
		case PPPSNATTR_REGISTER: rpt_id = REPORT_PSNATTRREGISTER; break;
		case PPPSNATTR_TAG: rpt_id = REPORT_PSNATTRTAG; break;
		case PPPSNATTR_ALLADDR:
		case PPPSNATTR_HANGEDADDR:
		case PPPSNATTR_DLVRADDR:
		case PPPSNATTR_DUPDLVRADDR:
		case PPPSNATTR_STANDALONEADDR: rpt_id = REPORT_PSNATTRADDR; break;
		default: rpt_id = REPORT_PERSONLIST; break;
	}
	return PPAlddPrint(rpt_id, PView(this), 0);
}
//
// Implementation of PPALDD_RegisterType
//
PPALDD_CONSTRUCTOR(RegisterType)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(RegisterType) { Destroy(); }

int PPALDD_RegisterType::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		PPRegisterType rt_rec;
		MEMSZERO(H);
		H.ID = rFilt.ID;
		if(SearchObject(PPOBJ_REGISTERTYPE, rFilt.ID, &rt_rec) > 0) {
			STRNSCPY(H.Name, rt_rec.Name);
			STRNSCPY(H.Symb, rt_rec.Symb);
			H.KindID = rt_rec.PersonKindID;
			H.RegOrgKindID = rt_rec.RegOrgKind;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonKind
//
PPALDD_CONSTRUCTOR(PersonKind)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(PersonKind) { Destroy(); }

int PPALDD_PersonKind::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		PPObjPersonKind pk_obj;
		PPPersonKind pk_rec;
		if(pk_obj.Fetch(rFilt.ID, &pk_rec) > 0) {
			H.ID = pk_rec.ID;
			STRNSCPY(H.Name, pk_rec.Name);
			STRNSCPY(H.Code, pk_rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonStatus
//
PPALDD_CONSTRUCTOR(PersonStatus)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(PersonStatus) { Destroy(); }

int PPALDD_PersonStatus::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPersonStatus ps_obj;
		PPPersonStatus ps_rec;
		if(ps_obj.Fetch(rFilt.ID, &ps_rec) > 0) {
			H.ID = ps_rec.ID;
			STRNSCPY(H.Name, ps_rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonCat
//
PPALDD_CONSTRUCTOR(PersonCat)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(PersonCat) { Destroy(); }

// @Muxa {
int PPALDD_PersonCat::Set(long iterId, int commit)
{
	int    ok = 1;
	SETIFZ(Extra[3].Ptr, new PPPersonCat);
	PPPersonCat * p_prsn_cat = static_cast<PPPersonCat *>(Extra[3].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			p_prsn_cat->ID = 0;
			STRNSCPY(p_prsn_cat->Name, strip(H.Name));
			STRNSCPY(p_prsn_cat->Symb, strip(H.Code));
		}
		else {
			// @todo Ошибка (в этой структуре нет итераторов)
		}
	}
	else {
		PPObjPersonCat obj_prsn_cat;
		PPID   id = 0;
		THROW(obj_prsn_cat.AddItem(&id, p_prsn_cat, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit) {
		delete p_prsn_cat;
		Extra[3].Ptr = 0;
	}
	return ok;
}
// } @Muxa

int PPALDD_PersonCat::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjPersonCat pc_obj;
		PPPersonCat pc_rec;
		if(pc_obj.Fetch(rFilt.ID, &pc_rec) > 0) {
			H.ID = pc_rec.ID;
			STRNSCPY(H.Name, pc_rec.Name);
			STRNSCPY(H.Code, pc_rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_PersonList
//
PPALDD_CONSTRUCTOR(PersonList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(PersonList) { Destroy(); }

int PPALDD_PersonList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Person, rsrv);
	H.PsnKindID  = p_filt->PersonKindID;
	H.StatusID   = p_filt->Status;
	H.CountryID  = 0;
	H.CityID     = p_filt->CityID;
	H.AttrType   = p_filt->GetAttribType();
	H.RegTypeID  = p_filt->RegTypeID;
	H.TagTypeID  = p_filt->RegTypeID - TAGOFFSET;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_PersonList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Person);
}

int PPALDD_PersonList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Person);
	const  PersonFilt * p_filt = static_cast<const PersonFilt *>(p_v->GetBaseFilt());
	I.PersonID = item.ID;
	if(p_filt) {
		if(oneof3(p_filt->GetAttribType(), PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
			I.AddrID = item.AttrItem.TabID;
		}
	}
	{
		SString temp_buf;
		STRNSCPY(I.Phone,    p_v->GetFromStrPool(item.AttrItem.PhoneP, temp_buf));
		STRNSCPY(I.Address,  p_v->GetFromStrPool(item.AttrItem.AddressP, temp_buf));
		STRNSCPY(I.RAddress, p_v->GetFromStrPool(item.AttrItem.RAddressP, temp_buf));
		STRNSCPY(I.BnkName,  p_v->GetFromStrPool(item.AttrItem.BnkNameP, temp_buf));
		STRNSCPY(I.BnkAcct,  p_v->GetFromStrPool(item.AttrItem.BnkAcctP, temp_buf));
		STRNSCPY(I.Serial,   p_v->GetFromStrPool(item.AttrItem.RegSerialP, temp_buf));
		STRNSCPY(I.Number,   item.AttrItem.RegNumber);
	}
	I.InitDate = item.AttrItem.RegInitDate;
	I.Expiry   = item.AttrItem.RegExpiry;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_PersonList::Destroy() { DESTROY_PPVIEW_ALDD(Person); }
//
// Implementation of PPALDD_World
//
PPALDD_CONSTRUCTOR(World)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	Extra[0].Ptr = new PPObjWorld;
}

PPALDD_DESTRUCTOR(World)
{
	Destroy();
	delete static_cast<PPObjWorld *>(Extra[0].Ptr);
}

int PPALDD_World::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		WorldTbl::Rec rec;
		if(static_cast<PPObjWorld *>(Extra[0].Ptr)->Fetch(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			H.Kind = rec.Kind;
			H.ParentID = rec.ParentID;
			H.CountryID = rec.CountryID;
			H.CurrencyID = rec.CurrencyID;
			H.Status = rec.Status;
			H.Flags = rec.Flags;
			H.Latitude = rec.Latitude;
			H.Longitude = rec.Longitude;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Abbr, rec.Abbr);
			STRNSCPY(H.Phone, rec.Phone);
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.ZIP, rec.ZIP);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_City
//
PPALDD_CONSTRUCTOR(City)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjWorld;
	}
}

PPALDD_DESTRUCTOR(City)
{
	Destroy();
	delete static_cast<PPObjWorld *>(Extra[0].Ptr);
}

int PPALDD_City::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		WorldTbl::Rec rec;
		if(static_cast<PPObjWorld *>(Extra[0].Ptr)->Fetch(rFilt.ID, &rec) > 0) {
			H.ID = H.WID = rec.ID;
			H.CountryID = rec.CountryID;
			H.RegionID  = rec.ParentID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Abbr,  rec.Abbr);
			STRNSCPY(H.Phone, rec.Phone);
			STRNSCPY(H.Code,  rec.Code);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Country
//
PPALDD_CONSTRUCTOR(Country)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjWorld;
	}
}

PPALDD_DESTRUCTOR(Country)
{
	Destroy();
	delete static_cast<PPObjWorld *>(Extra[0].Ptr);
}

int PPALDD_Country::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		WorldTbl::Rec rec;
		if(static_cast<PPObjWorld *>(Extra[0].Ptr)->Fetch(rFilt.ID, &rec) > 0) {
			H.ID = H.WID = rec.ID;
			STRNSCPY(H.Name,  rec.Name);
			STRNSCPY(H.Abbr,  rec.Abbr);
			STRNSCPY(H.Phone, rec.Phone);
			STRNSCPY(H.Code,  rec.Code);
			{
				SString temp_buf;
				PPObjWorld::GetNativeCountryName(temp_buf);
				H.fNativeLand = BIN(temp_buf.CmpNC(rec.Name) == 0);
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_SupplNameList
//
PPALDD_CONSTRUCTOR(SupplNameList)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(SupplNameList)
{
	Destroy();
	Extra[0].Ptr = 0;
}

int PPALDD_SupplNameList::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		const char * p_end = "...";
		size_t end_len = sstrlen(p_end);
		PPIDArray suppl_list;
		if(BillObj->trfr->Rcpt.GetSupplList(rFilt.ID, 0, getcurdate_(), &suppl_list) > 0) {
			size_t suppl_names_size = sizeof(H.SupplNames);
			SString suppl_name, buf;
			for(uint i = 0; i < suppl_list.getCount(); i++) {
				GetArticleName(suppl_list.at(i), buf);
				if(buf.Len() > 0) {
					if(suppl_name.Len() + buf.Len() > suppl_names_size) {
						if(suppl_name.Len() + end_len > suppl_names_size)
							suppl_name.Trim(suppl_names_size - 1 - end_len);
						suppl_name.Cat(p_end);
						break;
					}
					else {
						suppl_name.Cat(buf);
						if(i != suppl_list.getCount() - 1)
							suppl_name.Semicol();
					}
				}
			}
			suppl_name.CopyTo(H.SupplNames, sizeof(H.SupplNames));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(ClientActivityDetails); ClientActivityDetailsFilt::ClientActivityDetailsFilt() : PPBaseFilt(PPFILT_CLIENTACTIVITYDETAILS, 0, 1)
{
	SetFlatChunk(offsetof(ClientActivityDetailsFilt, ReserveStart), offsetof(ClientActivityDetailsFilt, ReserveEnd) - offsetof(ClientActivityDetailsFilt, ReserveStart));
	Init(1, 0);
}

PPViewClientActivityDetails::PPViewClientActivityDetails() : PPView(0, &Filt, PPVIEW_CLIENTACTIVITYDETAILS, PPView::implBrowseArray, 0), P_DsList(0)
{
}

PPViewClientActivityDetails::~PPViewClientActivityDetails()
{
	delete P_DsList;
}

/*virtual*/int PPViewClientActivityDetails::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	IList.clear();
	if(Filt.PersonID) {
		PrcssrClientActivityStatistics prcssr;
		prcssr.ScanDetailedActivityListForSinglePerson(Filt.PersonID, IList);
	}
	CATCHZOK
	return ok;
}

int PPViewClientActivityDetails::InitIteration()
{
	int    ok = 1;
	if(P_DsList) {
		Counter.Init(P_DsList->getCount());
	}
	else
		ok = -1;
	return ok;
}
	
int FASTCALL PPViewClientActivityDetails::NextIteration(ClientActivityDetailsViewItem * pItem)
{
	int    ok = -1;
	if(P_DsList) {
		for(; ok < 0 && Counter < P_DsList->getCount(); Counter.Increment()) {
			if(pItem) {
				const BrwItem & r_item = P_DsList->at(Counter);
				// @todo
			}
			else
				ok = 1;
		}
	}
	return ok;
}

int PPViewClientActivityDetails::MakeList()
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	PPIDArray result_list;
	SString temp_buf;
	SString obj_buf;
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new TSArray <BrwItem>();
	for(uint i = 0; i < IList.getCount(); i++) {
		const ClientActivityDetailedEntry & r_item = IList.at(i);
		BrwItem new_item;
		new_item.Oid = r_item.Oid;
		new_item.Dtm = r_item.Dtm;
		temp_buf.Z();
		obj_buf.Z();
		switch(r_item.Oid.Obj) {
			case PPOBJ_BILL:
				{
					BillTbl::Rec bill_rec;
					if(p_bobj->Fetch(r_item.Oid.Id, &bill_rec) > 0) {
						PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, temp_buf);
						obj_buf.Cat("Bill").CatDiv(':', 2).Cat(temp_buf);
					}
					else {
						obj_buf.Cat("Bill").Space().CatChar('#').Cat(r_item.Oid.Id);
					}
				}
				break;
			case PPOBJ_SCARD:
				{
					SCardCore::OpBlock op_blk;
					SCardTbl::Rec sc_rec;
					if(ScObj.Fetch(r_item.Oid.Id, &sc_rec) > 0 && ScObj.P_Tbl->GetOp(r_item.Oid.Id, r_item.Dtm, &op_blk) > 0) {
						obj_buf.Cat("SCard").CatDiv(':', 2).Cat(sc_rec.Code).Space().Cat(r_item.Dtm, DATF_ISO8601CENT, 0).Space().Cat(op_blk.Amount, MKSFMTD(0, 2, 0));
					}
					else {
						obj_buf.Cat("SCardOp").Space().CatChar('#').Cat(r_item.Oid.Id).Space().Cat(r_item.Dtm, DATF_ISO8601CENT, 0);
					}
				}
				break;
			case PPOBJ_CCHECK:
				{
					CCheckCore * p_cc = ScObj.P_CcTbl;
					CCheckTbl::Rec cc_rec;
					if(p_cc && p_cc->Search(r_item.Oid.Id, &cc_rec) > 0) {
						CCheckCore::MakeCodeString(&cc_rec, 0, temp_buf);
						obj_buf.Cat("CCheck").CatDiv(':', 2).Cat(temp_buf);
					}
					else {
						obj_buf.Cat("CCheck").Space().CatChar('#').Cat(r_item.Oid.Id);
					}
				}
				break;
			case PPOBJ_PERSONEVENT:
				{
					PersonEventTbl::Rec pe_rec;
					if(PeObj.Search(r_item.Oid.Id, &pe_rec) > 0) {
						PeObj.MakeCodeString(&pe_rec, temp_buf);
						obj_buf.Cat("PersonEvent").CatDiv(':', 2).Cat(temp_buf);
					}
					else {
						obj_buf.Cat("PersonEvent").Space().CatChar('#').Cat(r_item.Oid.Id);
					}
				}
				break;
		}
		STRNSCPY(new_item.TransactionName, obj_buf);
		P_DsList->insert(&new_item);
	}
	//CATCHZOK
	CALLPTRMEMB(P_DsList, setPointer(0));
	return ok;
}

int PPViewClientActivityDetails::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: 
				pBlk->Set(p_item->Dtm.d);
				break;
			case 1: 
				pBlk->Set(p_item->TransactionName);
				break;
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewClientActivityDetails::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewClientActivityDetails * p_v = static_cast<PPViewClientActivityDetails *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

/*virtual*/void PPViewClientActivityDetails::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewClientActivityDetails::GetDataForBrowser, this);
	}
}

/*virtual*/SArray * PPViewClientActivityDetails::CreateBrowserArray(uint * pBrwId, SString * pSubTitle) // @construction
{
	TSArray <BrwItem> * p_array = 0;
	THROW(MakeList());
	p_array = new TSArray <BrwItem>(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, BROWSER_CLIACTIVITYDETAILS);
	return p_array;
}
