// V_PERSON.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
#include <process.h>
#include <ppsoapclient.h>

int SLAPI ViewPersonInfoBySCard(const char * pCode)
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
			// @v9.0.2 PPGetWord(PPWORD_CARD, 0, word);
			PPLoadString("card", word); // @v9.0.2
			buf.CR().Cat(word).CatDiv(':', 2).Space().Cat(scard_rec.Code);
			PPLoadString("rest", word);
			buf.CR().Cat(word).CatDiv(':', 2).Space().Cat(scard_rec.Rest);
			// @v9.0.2 PPGetWord(PPWORD_NAME, 0, word.Z());
			PPLoadString("name", word); // @v9.0.2
			buf.CR().Cat(word).CatDiv(':', 2).Space().Cat(psn_rec.Name);
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
SLAPI PPViewPerson::PPViewPerson() : PPView(0, &Filt, PPVIEW_PERSON), DefaultTagID(0), P_TempPsn(0), P_Fr(0)
{
}

SLAPI PPViewPerson::~PPViewPerson()
{
	delete P_TempPsn;
	delete P_Fr;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempPersonFile, TempPerson);

int SLAPI PPViewPerson::IsTempTblNeeded()
{
	int    yes = 0;
	if(Filt.AttribType || Filt.P_RegF || Filt.P_TagF || Filt.P_SjF || (Filt.Flags & PersonFilt::fTagsCrsstab))
		yes = 1;
	else {
		SString temp_buf;
		Filt.GetExtssData(PersonFilt::extssNameText, temp_buf);
		if(temp_buf.NotEmptyS())
			yes = 1;
		else {
			Filt.GetExtssData(PersonFilt::extssEmailText, temp_buf);
			if(temp_buf.NotEmptyS())
				yes = 1;
		}
		if(!yes) {
			if(Filt.NewCliPeriod.low && Filt.Flags & Filt.fNewClientsOnly && PsnObj.GetConfig().NewClientDetectionList.getCount()) {
				yes = 1;
			}
		}
	}
	return yes;
}

int SLAPI PPViewPerson::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, use_ta = 1;
	SString srch_buf;
	SString msg_buf;
	SString temp_buf;
	LocationTbl::Rec loc_rec;
	PPNewContragentDetectionBlock ncd_blk;
	THROW(Helper_InitBaseFilt(pFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempPsn);
	ZDELETE(P_Ct);
	NewCliList.Clear();
	StrPool.ClearS(); // @v9.8.4
	if(IsTempTblNeeded()) {
		IterCounter cntr;
		PersonTbl     * pt = PsnObj.P_Tbl;
		PersonKindTbl * kt = & PsnObj.P_Tbl->Kind;
		THROW(P_TempPsn = CreateTempPersonFile());
		{
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			if(oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
				PersonTbl::Rec owner_rec;
				PsnAttrViewItem vi;
				PPIDArray dlvr_loc_list;
				UintHashTable id_list;
				if(Filt.AttribType == PPPSNATTR_HANGEDADDR) {
					PROFILE_START
					PersonTbl::Key0 k0;
					BExtQuery q(pt, 0);
					q.select(pt->ID, pt->MainLoc, pt->RLoc, 0);
					MEMSZERO(k0);
					for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
						if(pt->data.MainLoc)
							id_list.Add(pt->data.MainLoc);
						if(pt->data.RLoc)
							id_list.Add(pt->data.RLoc);
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
				if(Filt.AttribType == PPPSNATTR_STANDALONEADDR && Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
					PROFILE_START
					PPIDArray sj_id_list;
					SysJournal * p_sj = DS.GetTLA().P_SysJ;
					Filt.P_SjF->Period.Actualize(ZERODATE);
					THROW(p_sj->GetObjListByEventPeriod(PPOBJ_LOCATION, Filt.P_SjF->UserID,
						&Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, sj_id_list));
					for(uint i = 0; i < sj_id_list.getCount(); i++) {
						const PPID loc_id = sj_id_list.get(i);
						if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0 && loc_rec.Flags & LOCF_STANDALONE) {
							MEMSZERO(vi);
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
						if(Filt.AttribType == PPPSNATTR_HANGEDADDR && !(loc_rec.Flags & LOCF_STANDALONE) && !id_list.Has(loc_rec.ID)) {
							if(PsnObj.LocObj.Fetch(loc_rec.ID, &loc_rec) > 0) // @v9.4.5 Search-->Fetch
								do_insert = 1;
						}
						else if(Filt.AttribType == PPPSNATTR_STANDALONEADDR && loc_rec.Flags & LOCF_STANDALONE) {
							// LocObj.P_Tbl->Enum не заполняет строковый "хвост": придется повторить извлечение записи
							if(PsnObj.LocObj.Fetch(loc_rec.ID, &loc_rec) > 0) // @v9.4.5 Search-->Fetch
								do_insert = 2;
						}
						if(do_insert) {
							MEMSZERO(vi);
							if(CreateAddrRec(loc_rec.ID, &loc_rec, (loc_rec.Flags & LOCF_STANDALONE) ? "standalone" : "unknown", &vi) > 0) {
								if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &owner_rec) > 0) {
									STRNSCPY(vi.Name, owner_rec.Name);
								}
								THROW_DB(P_TempPsn->insertRecBuf(&vi));
							}
						}
						PPWaitMsg(ideqvalstr(loc_rec.ID, msg_buf.Z())); // @v9.4.5
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
					THROW(p_sj->GetObjListByEventPeriod(PPOBJ_PERSON, Filt.P_SjF->UserID,
						&Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, local_list));
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
				if(Filt.Kind) {
					local_list.clear();
					PersonKindTbl::Key0 k0, k0_;
					MEMSZERO(k0);
					k0.KindID = Filt.Kind;
					BExtQuery kq(kt, 0);
					kq.select(kt->PersonID, 0L).where(kt->KindID == Filt.Kind);
					k0_ = k0;
					cntr.Init(kq.countIterations(0, &k0_, spGe));
					for(kq.initIteration(0, &k0, spGe); kq.nextIteration() > 0; cntr.Increment()) {
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
					if(Filt.Category || Filt.Status || srch_buf.NotEmptyS()) {
						local_list.clear();
						for(uint i = 0; i < id_list.getCount(); i++) {
							const PPID id = id_list.get(i);
							PersonTbl::Rec rec;
							if(PsnObj.Fetch(id, &rec) > 0) {
								if((!Filt.Category || rec.CatID == Filt.Category) && (!Filt.Status || rec.Status == Filt.Status)) {
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
							if((!Filt.Category || pt->data.CatID == Filt.Category) && (!Filt.Status || pt->data.Status == Filt.Status)) {
								local_list.add(pt->data.ID);
							}
						} while(pt->search(1, &k, spNext) && srch_buf.CmpNC(pt->data.Name) == 0);
					}
					else {
						BExtQuery pq(pt, Filt.Category ? 2 : 0);
						DBQ * dbq = 0;
						pq.select(pt->ID, pt->Name, pt->Status, pt->MainLoc, pt->Flags, pt->RLoc, 0L);
						dbq = ppcheckfiltid(dbq, pt->CatID, Filt.Category);
						dbq = ppcheckfiltid(dbq, pt->Status, Filt.Status);
						pq.where(*dbq);
						if(Filt.Category) {
							k.k2.CatID = Filt.Category;
						}
						k_ = k;
						cntr.Init(pq.countIterations(0, &k_, spGe));
						local_list.clear();
						for(pq.initIteration(0, &k, spGe); pq.nextIteration() > 0; cntr.Increment()) {
							if(srch_buf.Empty() || ExtStrSrch(pt->data.Name, srch_buf, 0))
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
					UintHashTable * p_used_loc_list = (Filt.AttribType == PPPSNATTR_ALLADDR && Filt.Flags & PersonFilt::fShowHangedAddr) ? &used_loc_list : 0;
					assert(use_list);
					id_list.sortAndUndup();
					cntr.Init(id_list.getCount());
					for(uint i = 0; i < id_list.getCount(); i++) {
						const PPID person_id = id_list.get(i);
						int   skip = 0;
						// @v9.4.5 {
						if(Filt.NewCliPeriod.low) {
							if(ncd_blk.IsNewPerson(person_id, Filt.NewCliPeriod))
								NewCliList.Add(person_id);
							else if(Filt.Flags & Filt.fNewClientsOnly)
								skip = 1;
						}
						// } @v9.4.5
						if(!skip) {
							THROW(AddTempRec(person_id, p_used_loc_list, 0));
						}
						PPWaitPercent(cntr.Increment());
					}
					if(p_used_loc_list) {
						PsnAttrViewItem vi;
						for(SEnum en = PsnObj.LocObj.P_Tbl->Enum(LOCTYP_ADDRESS, 0, LocationCore::eoIgnoreParent); en.Next(&loc_rec) > 0;) {
							if(!p_used_loc_list->Has((ulong)loc_rec.ID)) {
								MEMSZERO(vi);
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
				explicit SLAPI  PersonTagsCrosstab(PPViewPerson * pV) : Crosstab(), P_V(pV)
				{
				}
				virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
				{
					PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
					SetupBrowserCtColumns(p_brw);
					return p_brw;
				}
			protected:
				virtual int SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
				{
					return (pVal && P_V) ? P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0;
				}
				PPViewPerson * P_V;
			};
			if((Filt.Flags & PersonFilt::fTagsCrsstab) && P_TempPsn) {
				THROW_MEM(P_Ct = new PersonTagsCrosstab(this));
				P_Ct->SetTable(P_TempPsn, P_TempPsn->TabID);
				P_Ct->AddIdxField(P_TempPsn->ID);
				P_Ct->AddInheritedFixField(P_TempPsn->Name);
				P_Ct->AddAggrField(P_TempPsn->RegNumber);
				THROW(P_Ct->Create(use_ta));
			}
		}
	}
	else if(Filt.NewCliPeriod.low && !oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) { // @v9.4.5 else
		PersonViewItem item;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(ncd_blk.IsNewPerson(item.ID, Filt.NewCliPeriod)) {
				NewCliList.Add(item.ID);
			}
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
	}
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_TempPsn);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewPerson::GetTabTitle(long tabID, SString & rBuf) const
{
	return GetObjectName(PPOBJ_TAG, tabID, rBuf.Z());
}

int SLAPI PPViewPerson::UpdateHungedAddr(PPID addrID)
{
	int    ok = 1;
	LocationTbl::Rec loc_rec;
	{
		PsnAttrViewItem vi;
		PersonTbl::Rec owner_rec;
		MEMSZERO(vi);
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

int SLAPI PPViewPerson::AddItem(PPID * pID)
{
	int    ok = -1;
	PPID   id = 0;
	if(PsnObj.Edit(&id, (void *)Filt.Kind) == cmOK) {
		AddTempRec(id, 0, 1);
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewPerson::EditItem(PPID id)
{
	int    ok = -1;
	if(PsnObj.Edit(&id, (void *)Filt.Kind) == cmOK) {
		EditTempRec(id, 1);
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewPerson::OpenClientDir(PPID PersonId)
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
							THROW_SL(::createDir(cl_dir_name.SetLastSlash().Cat(r_reg.Num)));
							ok = 1;
						}
					}
			kind_pos++;
		}
		if(ok) {
			ok = 0;
			char   full_path_to_expl[MAXPATH];
			full_path_to_expl[0] = '\0';
			if(GetWindowsDirectory((LPTSTR)full_path_to_expl, sizeof(full_path_to_expl))) {
				setLastSlash(full_path_to_expl);
				strcat(full_path_to_expl, "explorer.exe");
				spawnl(_P_NOWAIT, full_path_to_expl, cl_dir_name.cptr(), cl_dir_name.cptr(), 0);
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

int SLAPI PPViewPerson::EditRegs(PPID id, int oneReg)
{
	int    ok = -1, r = 0;
	PPPersonPacket pack;
	THROW(PsnObj.RegObj.CheckRights(PPR_READ));
	THROW(PsnObj.GetPacket(id, &pack, 0) > 0);
	if(oneReg) {
		uint   pos = 0;
		if(Filt.AttribType == PPPSNATTR_REGISTER) {
			if(pack.Regs.GetRegister(Filt.RegTypeID, &pos, 0) > 0)
				r = PsnObj.RegObj.EditDialog(&pack.Regs.at(pos - 1), &pack.Regs, &pack);
			else {
				RegisterTbl::Rec rec;
				THROW(PPObjRegister::InitPacket(&rec, Filt.RegTypeID, PPOBJ_PERSON, pack.Rec.ID));
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
	else
		r = PsnObj.RegObj.EditList(&pack, 0);
	if(r > 0) {
		THROW(PsnObj.RegObj.CheckRights(PPR_MOD));
		THROW(PsnObj.PutPacket(&id, &pack, 1));
		THROW(EditTempRec(id, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPerson::EditDlvrAddrExtFlds(PPID id)
{
	int    ok = -1;
	LocationTbl::Rec loc_rec;
	MEMSZERO(loc_rec);
	THROW(PsnObj.LocObj.CheckRightsModByID(&id));
	if(PsnObj.LocObj.Search(id, &loc_rec) > 0 && EditDlvrAddrExtFields(&loc_rec))
		THROW(PsnObj.LocObj.PutRecord(&id, &loc_rec, 1));
	CATCHZOK
	return ok;
}

int SLAPI PPViewPerson::DeleteItem(PPID id)
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
			PPWait(1);
			for(InitIteration(); NextIteration(&item) > 0; ) {
				id_list.addUnique(item.ID);
			}
			{
				const uint cnt = id_list.getCount();
				if(cnt) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < cnt; i++) {
						const PPID _id = id_list.get(i);
						if(!PsnObj.RemoveObjV(_id, 0, 0, 0)) {
							logger.LogLastError();
						}
						PPWaitPercent(i+1, cnt);
					}
					THROW(tra.Commit());
				}
			}
			PPWait(0);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPerson::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_PERSONTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long   count = 0;
		PPWait(1);
		for(InitIteration(); NextIteration(0) > 0;) {
			count++;
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
		/*
		InitIteration();
		if(P_TempPsn)
			count = Counter.GetTotal();
		else
			for(count = 0; NextIteration(0) > 0; count++);
		*/
		dlg->setCtrlLong(CTL_PERSONTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
		return -1;
	}
	else
		return 0;
}

int SLAPI PPViewPerson::ViewTasks(PPID id)
{
	if(id) {
		int    is_client = (Filt.Kind == PPPRK_CLIENT) ? 1 : (Filt.Kind == PPPRK_EMPL) ? 0 : -1; // -1 - undef kind
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

int SLAPI PPViewPerson::ViewPersonEvents(PPID id)
{
	int    ok = -1;
	if(id) {
		PersonEventFilt flt;
		flt.PrmrID = id;
		PPView::Execute(PPVIEW_PERSONEVENT, &flt, PPView::exefModeless, 0);
	}
	return ok;
}

int SLAPI PPViewPerson::ViewRelations(PPID id)
{
	return PsnObj.EditRelationList(id);
}

int SLAPI PPViewPerson::AddRelation(PPID id)
{
	return PsnObj.EditRelation(&id, 0, 0); // (EditRelation)
}

int SLAPI PPViewPerson::Transmit(PPID id, int transmitKind)
{
	int    ok = -1;
	if(transmitKind == 0) {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			PersonViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
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
			VCard::Rec rec;
			VCard      vcard;
			fname.Cat(id).Dot().Cat("vcf");
			PPGetFilePath(PPPATH_OUT, fname, path);
			THROW(vcard.Open(path, 1));
			rec.Name = pack.Rec.Name;
			// GetMainOrgName(rec.Org);
			{  // Phones, Fax's, Emails
				uint i = 0;
				pack.ELA.GetPhones(10, temp_buf);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf) > 0)
						rec.WorkPhone = temp_buf;
					if(ss.get(&i, temp_buf) > 0)
						rec.HomePhone = temp_buf;
					if(ss.get(&i, temp_buf) > 0)
						rec.MobilePhone = temp_buf;
				}
				pack.ELA.GetPhones(10, temp_buf, ELNKRT_FAX);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf) > 0)
						rec.WorkFax = temp_buf;
					if(ss.get(&i, temp_buf.Z()) > 0)
						rec.HomeFax = temp_buf;
				}
				pack.ELA.GetPhones(10, temp_buf, ELNKRT_EMAIL);
				{
					StringSet ss(';', temp_buf);
					if(ss.get(&(i = 0), temp_buf) > 0)
						rec.Email1 = temp_buf;
					if(ss.get(&i, temp_buf) > 0)
						rec.Email2 = temp_buf;
				}
			}
			{ // Addrs
				PPID country_id = 0;
				SString buf, zip;
				WorldTbl::Rec city_rec;
				LocationTbl::Rec loc;
				PPObjWorld obj_world;

				MEMSZERO(city_rec);
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
	PPWait(0);
	return ok;
}

int FASTCALL PPViewPerson::CheckForFilt(const PersonTbl::Rec * pRec)
{
	int    ok = 1;
	if(Filt.Status && pRec->Status != Filt.Status)
		ok = 0;
	else if((Filt.Flags & PersonFilt::fVatFree) && !(pRec->Flags & PSNF_NOVATAX))
		ok = 0;
	else if((Filt.Flags & PersonFilt::fHasImages) && !(pRec->Flags & PSNF_HASIMAGES))
		ok = 0;
	else if(Filt.Category && Filt.Category != pRec->CatID)
		ok = 0;
	else if(Filt.PersonID && Filt.PersonID != pRec->ID)
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
	if(ok) {
		if(!(Filt.Flags & PersonFilt::fLocTagF) && !PPObjTag::CheckForTagFilt(PPOBJ_PERSON, pRec->ID, Filt.P_TagF))
			ok = 0;
		else if(Filt.CityID && !oneof3(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
			PPID   city_id = 0;
			if(!pRec->MainLoc)
				ok = 0;
			else if(PsnObj.LocObj.GetCity(pRec->MainLoc, &city_id, 0, 1) <= 0)
				ok = 0;
			else if(WObj.IsChildOf(city_id, Filt.CityID) <= 0)
				ok = 0;
		}
	}
	return ok;
}

int SLAPI PPViewPerson::CreateAddrRec(PPID addrID, const LocationTbl::Rec * pLocRec, const char * pAddrKindText, PsnAttrViewItem * pItem)
{
	int    ok = -1;
	LocationTbl::Rec loc_rec;
	MEMSZERO(loc_rec);
	if(addrID) {
		SString temp_buf;
		pItem->TabID = addrID;
		if(pAddrKindText && pAddrKindText[0] == '@') {
			PPLoadString(pAddrKindText+1, temp_buf);
			temp_buf.CopyTo(pItem->RegNumber, sizeof(pItem->RegNumber));
		}
		else {
			STRNSCPY(pItem->RegNumber, pAddrKindText);
		}
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
				if(Filt.AttribType == PPPSNATTR_STANDALONEADDR) {
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

int SLAPI PPViewPerson::CreateTempRec(PersonTbl::Rec * pPsnRec, PPID tabID, PsnAttrViewItem * pItem)
{
	int    ok = 1;
	SString temp_buf;
	PsnAttrViewItem item;
	MEMSZERO(item);
	item.ID = pPsnRec->ID;
	item.TabID = tabID;
	STRNSCPY(item.Name, pPsnRec->Name);
	//
	// Случаи oneof2(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_BNKACCT) обрабатываются в вызывающий функции
	//
	if(Filt.AttribType == PPPSNATTR_PHONEADDR) {
		PPELinkArray elink_ary;
		if(PsnObj.P_Tbl->GetELinks(pPsnRec->ID, &elink_ary)) {
			const int buf_len = 128; // @v9.8.4 sizeof(item.Phone);
			SString phone_list, fax_list;

			elink_ary.GetPhones(5, phone_list, ELNKRT_PHONE);
			elink_ary.GetPhones(2, fax_list, ELNKRT_FAX);
			if(fax_list.Len() && (phone_list.Len() + fax_list.Len() + 6) < buf_len)
				phone_list.CatDiv(';', 2).Cat("fax").Space().Cat(fax_list);
			//phone_list.CopyTo(item.Phone, sizeof(item.Phone));
			StrPool.AddS(phone_list, &item.PhoneP);
		}
		if(pPsnRec->MainLoc) {
			// @v9.5.5 PsnObj.LocObj.P_Tbl->GetAddress(pPsnRec->MainLoc, 0, temp_buf);
			PsnObj.LocObj.GetAddress(pPsnRec->MainLoc, 0, temp_buf); // @v9.5.5
			//temp_buf.CopyTo(item.Address, sizeof(item.Address));
			StrPool.AddS(temp_buf, &item.AddressP);
		}
		if(pPsnRec->RLoc) {
			// @v9.5.5 PsnObj.LocObj.P_Tbl->GetAddress(pPsnRec->RLoc, 0, temp_buf);
			PsnObj.LocObj.GetAddress(pPsnRec->RLoc, 0, temp_buf); // @v9.5.5
			//temp_buf.CopyTo(item.RAddress, sizeof(item.RAddress));
			StrPool.AddS(temp_buf, &item.RAddressP);
		}
		//if(item.Phone[0] == 0 && item.Address[0] == 0 && item.RAddress[0] == 0) {
		if(!item.PhoneP && !item.AddressP && !item.RAddressP) {
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	else if(Filt.AttribType == PPPSNATTR_EMAIL) {
		PPELinkArray elink_ary;
		if(PsnObj.P_Tbl->GetELinks(pPsnRec->ID, &elink_ary)) {
			SString email_list;
			elink_ary.GetPhones(1, email_list, ELNKRT_EMAIL);
			//email_list.CopyTo(item.Phone, sizeof(item.Phone));
			StrPool.AddS(email_list, &item.PhoneP);
		}
		//if(item.Phone[0] == 0) {
		if(!item.PhoneP) {
			if(Filt.EmptyAttrib == EA_NOEMPTY)
				ok = 0;
		}
		else if(Filt.EmptyAttrib == EA_EMPTY)
			ok = 0;
	}
	else if(Filt.AttribType == PPPSNATTR_REGISTER && Filt.RegTypeID) {
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
	else if(Filt.AttribType == PPPSNATTR_TAG && Filt.RegTypeID) {
		PPID   tag_id = Filt.RegTypeID - TAGOFFSET;
		ObjTagList tag_list;
		if(PPRef->Ot.GetList(PPOBJ_PERSON, pPsnRec->ID, &tag_list)) {
			const ObjTagItem * p_tag = tag_list.GetItem(tag_id);
			if(p_tag) {
				if(p_tag->TagDataType == OTTYP_BOOL)
					ltoa(p_tag->Val.IntVal, item.RegNumber, 10);
				else if(p_tag->TagDataType == OTTYP_NUMBER)
					realfmt(p_tag->Val.RealVal, SFMT_MONEY, item.RegNumber);
				else if(p_tag->TagDataType == OTTYP_STRING && p_tag->Val.PStr)
					STRNSCPY(item.RegNumber, p_tag->Val.PStr);
				else if(p_tag->TagDataType == OTTYP_ENUM) {
					PPObjectTag obj_tag_rec;
					ObjTag.Fetch(p_tag->TagID, &obj_tag_rec);
					GetObjectName(obj_tag_rec.TagEnumID, p_tag->Val.IntVal, item.RegNumber, sizeof(item.RegNumber));
				}
				else if(p_tag->TagDataType == OTTYP_DATE)
					datefmt(&p_tag->Val.DtVal, DATF_DMY, item.RegNumber);
			}
		}
		//if(item.RegSerial[0] == 0 && item.RegNumber[0] == 0) {
		if(!item.RegSerialP && item.RegNumber[0] == 0) {
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


int SLAPI PPViewPerson::Helper_InsertTempRec(TempPersonTbl::Rec & rRec)
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

int SLAPI PPViewPerson::AddTempRec(PPID id, UintHashTable * pUsedLocList, int use_ta)
{
	int    ok = -1;
	if(P_TempPsn && id) {
		int    stop = 0;
		int    is_crsst = BIN(Filt.Flags & PersonFilt::fTagsCrsstab);
		uint   tags_count = 0;
		PersonTbl::Rec psn_rec;
		PPIDArray dlvr_addr_list;
		TempPersonTbl::Key0 k0;
		PsnAttrViewItem vi; // TempPersonTbl
		ObjTagList tags;
		SString buf;
		SString buf2;
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		if(PsnObj.Search(id, &psn_rec) > 0 && CheckForFilt(&psn_rec)) {
			if(is_crsst) {
				THROW(PPRef->Ot.GetList(PPOBJ_PERSON, id, &tags));
				for(uint i = 0; i < tags.GetCount(); i++) {
					long   tab_id = DefaultTagID;
					buf = 0;
					const  ObjTagItem * p_item = tags.GetItemByPos(i);
					ObjTag.GetCurrTagVal(p_item, buf.Z());
					tab_id = p_item->TagID;
					k0.ID    = id;
					k0.TabID = tab_id;
					if(P_TempPsn->search(0, &k0, spEq))
						ok = -2;
					else if(CreateTempRec(&psn_rec, tab_id, &P_TempPsn->data) > 0) {
						buf.CopyTo(P_TempPsn->data.RegNumber, sizeof(P_TempPsn->data.RegNumber));
						THROW_DB(P_TempPsn->insertRec());
						ok = 1;
					}
				}
			}
			else if(oneof3(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
				uint   i;
				SArray rec_list(sizeof(PsnAttrViewItem));
				PsnObj.GetDlvrLocList(id, &dlvr_addr_list);
				if(Filt.AttribType == PPPSNATTR_ALLADDR) {
					if(psn_rec.MainLoc) {
						CALLPTRMEMB(pUsedLocList, Add((ulong)psn_rec.MainLoc));
						long   tab_id = psn_rec.MainLoc;
						MEMSZERO(vi);
						vi.ID = id;
						STRNSCPY(vi.Name, psn_rec.Name);
						if(CreateAddrRec(tab_id, 0, "@jaddress", &vi) > 0) {
							rec_list.insert(&vi);
						}
					}
					if(psn_rec.RLoc) {
						CALLPTRMEMB(pUsedLocList, Add((ulong)psn_rec.RLoc));
						long   tab_id = psn_rec.RLoc;
						MEMSZERO(vi);
						vi.ID = id;
						STRNSCPY(vi.Name, psn_rec.Name);
						if(CreateAddrRec(tab_id, 0, "@paddress", &vi) > 0) {
							rec_list.insert(&vi);
						}
					}
				}
				for(i = 0; i < dlvr_addr_list.getCount(); i++) {
					long   tab_id = dlvr_addr_list.get(i);
					if(tab_id && pUsedLocList) {
						pUsedLocList->Add((ulong)tab_id);
					}
					MEMSZERO(vi);
					vi.ID = id;
					STRNSCPY(vi.Name, psn_rec.Name);
					if(CreateAddrRec(tab_id, 0, "@daddress", &vi) > 0) {
						rec_list.insert(&vi);
					}
				}
				if(Filt.AttribType == PPPSNATTR_DUPDLVRADDR) {
					i = rec_list.getCount();
					if(i) do {
						const PsnAttrViewItem & r_item = *(const PsnAttrViewItem *)rec_list.at(--i);
						if(i == (rec_list.getCount()-1)) {
							int dup = 0;
							StrPool.GetS(r_item.AddressP, buf);
							for(uint j = 0; j < i; j++) {
								const PsnAttrViewItem & r_item2 = *(const PsnAttrViewItem *)rec_list.at(j);
								StrPool.GetS(r_item2.AddressP, buf2);
								//if(stricmp(r_item.Address, r_item2.Address) == 0) {
								if(buf.CmpNC(buf2) == 0) {
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
						MEMSZERO(vi);
						vi.ID = id;
						STRNSCPY(vi.Name, psn_rec.Name);
						THROW(ok = Helper_InsertTempRec(vi));
					}
				}
				else if(Filt.EmptyAttrib != EA_EMPTY) {
					for(i = 0; i < rec_list.getCount(); i++) {
						THROW(ok = Helper_InsertTempRec(*(PsnAttrViewItem *)rec_list.at(i)));
					}
				}
			}
			else if(Filt.AttribType == PPPSNATTR_BNKACCT) {
				//BnkAcctArray bac_ary;
				//PsnObj.BaObj.FetchList(id, &bac_ary);
				TSVector <PPBankAccount> bac_ary; // @v9.8.6 TSArray-->TSVector
				PsnObj.RegObj.GetBankAccountList(id, &bac_ary);
				if(bac_ary.getCount() == 0) {
					if(Filt.EmptyAttrib != EA_NOEMPTY) {
						MEMSZERO(vi);
						vi.ID = id;
						vi.TabID = 0;
						STRNSCPY(vi.Name, psn_rec.Name);
						THROW(ok = Helper_InsertTempRec(vi));
					}
				}
				else {
					if(Filt.EmptyAttrib != EA_EMPTY) {
						PersonTbl::Rec bnk_rec;
						//BankAccountTbl::Rec * p_bac_rec;
						//for(uint pos = 0; bac_ary.enumItems(&pos, (void**)&p_bac_rec) > 0;) {
						for(uint pos = 0; pos < bac_ary.getCount(); pos++) {
							const PPBankAccount & r_ba = bac_ary.at(pos);
							MEMSZERO(vi);
							vi.ID = id;
							vi.TabID = r_ba.ID;
							STRNSCPY(vi.Name, psn_rec.Name);
							if(PsnObj.Fetch(r_ba.BankID, &bnk_rec) > 0) {
								RegisterTbl::Rec bic_rec;
								//STRNSCPY(vi.BnkName, bnk_rec.Name);
								StrPool.AddS(bnk_rec.Name, &vi.BnkNameP);
								if(PsnObj.GetRegister(r_ba.BankID, PPREGT_BIC, &bic_rec) > 0) {
									//STRNSCPY(vi.Phone, bic_rec.Num);
									StrPool.AddS(bic_rec.Num, &vi.PhoneP);
								}
							}
							//STRNSCPY(vi.BnkAcct, r_ba.Acct);
							StrPool.AddS(r_ba.Acct, &vi.BnkAcctP);
							vi.RegInitDate = r_ba.OpenDate;
							if(r_ba.AccType) {
								// @todo Чаще всего здесь одно и тоже значение PPBAC_CURRENT: можно ускорить
								GetObjectName(PPOBJ_BNKACCTYPE, r_ba.AccType, buf);
								buf.CopyTo(vi.RegNumber, sizeof(vi.RegNumber));
							}
							THROW(ok = Helper_InsertTempRec(vi));
						}
					}
				}
			}
			else {
				long   tab_id = DefaultTagID;
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
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewPerson::EditTempRec(PPID id, int use_ta)
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

int SLAPI PPViewPerson::InitPersonAttribIteration()
{
	int    ok = 1;
	if(!P_TempPsn)
		ok = 0;
	else if((P_IterQuery = new BExtQuery(P_TempPsn, 1, 8)) != 0) {
		char   k[MAXKEYLEN];
		PPInitIterCounter(Counter, P_TempPsn);
		P_IterQuery->selectAll();
		memzero(k, sizeof(k));
		P_IterQuery->initIteration(0, k, spFirst);
	}
	else
		ok = PPSetErrorNoMem();
	return ok;
}

int SLAPI PPViewPerson::InitPersonIteration()
{
	int    ok = 1;
	PersonTbl * pt = PsnObj.P_Tbl;
	PersonKindTbl * kt = & PsnObj.P_Tbl->Kind;
	if(Filt.Kind) {
		PersonKindTbl::Key0 k0, k0_;
		MEMSZERO(k0);
		k0.KindID = Filt.Kind;
		THROW_MEM(P_IterQuery = new BExtQuery(kt, 0));
		P_IterQuery->select(kt->PersonID, 0L).where(kt->KindID == Filt.Kind);
		Counter.Init(P_IterQuery->countIterations(0, &(k0_ = k0), spGe));
		P_IterQuery->initIteration(0, &k0, spGe);
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
		P_IterQuery->initIteration(0, &pk, spGt);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewPerson::InitIteration()
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
	MEMSZERO(item);
	if(P_IterQuery)
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {// AHTOXA
			if(P_TempPsn) {
				if(oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					const PPID loc_id = P_TempPsn->data.TabID;
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
				PPID   id = Filt.Kind ? PsnObj.P_Tbl->Kind.data.PersonID : PsnObj.P_Tbl->data.ID;
				if(PsnObj.Search(id, &item) > 0 && CheckForFilt(&item))
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
	char   Name[80];
	int    AttribType;
};

static SArray * SLAPI CreateExtRegList(PersonFilt * pFilt, PPID * pAttrID, int onlyRegs)
{
	PPID   id = 0, attr_id = 0;
	SArray * p_ary = 0;
	ExtRegEntry ere;
	THROW_MEM(p_ary = new SArray(sizeof(ExtRegEntry)));
	if(pFilt->Status == PPPRS_COUNTRY) {
		ASSIGN_PTR(pAttrID, 0);
		return p_ary;
	}
	{
		PPObjRegisterType rt_obj;
		PPRegisterType rt_rec;
		SString reg_name, name;
		if(onlyRegs)
			reg_name = 0;
		else
			PPGetSubStr(PPTXT_PERSONATTRIBUTE, PPPSNATTR_REGISTER, reg_name);
		ere.AttribType = PPPSNATTR_REGISTER;
		for(id = 0; rt_obj.EnumItems(&id, &rt_rec) > 0;) {
			if(!pFilt->Kind || oneof2(rt_rec.PersonKindID, 0, pFilt->Kind)) {
				const long xst = CheckXORFlags(rt_rec.Flags, REGTF_PRIVATE, REGTF_LEGAL);
				if(!pFilt->Status || !xst || ((pFilt->Status == PPPRS_PRIVATE) ? (xst & REGTF_PRIVATE) : (xst & REGTF_LEGAL))) {
					ere.ExtRegID = rt_rec.ID;
					(name = reg_name).Cat(rt_rec.Name).CopyTo(ere.Name, sizeof(ere.Name));
					if(pAttrID && *pAttrID == rt_rec.ID)
						attr_id = rt_rec.ID;
					p_ary->insert(&ere);
				}
			}
		}
	}
	if(!onlyRegs) {
		PPObjTag tag_obj;
		ObjTagFilt ot_filt(PPOBJ_PERSON, ObjTagFilt::fOnlyTags);
		StrAssocArray * p_list = tag_obj.MakeStrAssocList(&ot_filt);
		if(p_list) {
			SString tag_attr, tag_name;
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
			attr_list.addzlist(PPPSNATTR_BNKACCT, PPPSNATTR_PHONEADDR, PPPSNATTR_EMAIL, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR,
				PPPSNATTR_DUPDLVRADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR, 0);
			for(uint i = 0; i < attr_list.getCount(); i++) {
				const int attr = attr_list.get(i);
				ere.ExtRegID = ++id;
				ere.AttribType = attr;
				PPGetSubStr(PPTXT_PERSONATTRIBUTE, attr, ere.Name, sizeof(ere.Name));
				if(attr == pFilt->AttribType)
					attr_id = id;
				p_ary->insert(&ere);
			}
		}
	}
	p_ary->sort(PTR_CMPFUNC(PPLBItem));
	ASSIGN_PTR(pAttrID, attr_id);
	CATCH
		ASSIGN_PTR(pAttrID, 0);
	ENDCATCH
	return p_ary;
}

static int SLAPI SetupExtRegCombo(TDialog * dlg, uint ctl, PersonFilt * pFilt, int idFromRegFilt, int onlyRegs)
{
	int    ok = 1;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	SArray   * p_ary = 0;
	if(p_combo && pFilt) {
		PPID   reg_filt_id = pFilt->P_RegF ? pFilt->P_RegF->RegTypeID : 0;
		PPID   id = idFromRegFilt ? reg_filt_id : pFilt->RegTypeID;
		ListBoxDef * def = 0;
		THROW(p_ary = CreateExtRegList(pFilt, &id, onlyRegs));
		THROW_MEM(def = new StdListBoxDef(p_ary, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, sizeof(ExtRegEntry))));
		ListWindow * p_lw = new ListWindow(def, 0, 0);
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

static int SLAPI GetExtRegData(TDialog * dlg, uint ctl, int *pAttrType, PPID *pRegId)
{
	int    ok = -1;
	if(pAttrType && pRegId) {
		*pRegId = 0;
		*pAttrType = 0;
		ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
		if(p_combo) {
			StdListBoxDef * p_def = 0;
			p_combo->TransmitData(-1, pRegId);
			if(*pRegId && (p_def = (StdListBoxDef *)p_combo->listDef()) != 0 && p_def->P_Data) {
				uint   pos = 0;
				ExtRegEntry * p_ere = p_def->P_Data->lsearch(pRegId, &pos, CMPF_LONG) ? (ExtRegEntry *)p_def->P_Data->at(pos) : 0;
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

int SLAPI GetExtRegListIds(PPID psnKindID, PPID statusID, PPIDArray * pList)
{
	pList->clear();
	int    ok = -1;
	uint   c;
	PersonFilt filt;
	SArray      * p_list = 0;
	ExtRegEntry * p_entry;
	filt.Kind   = psnKindID;
	filt.Status = statusID;
	THROW(p_list = CreateExtRegList(&filt, 0, 1));
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
IMPLEMENT_PPFILT_FACTORY(Person); SLAPI PersonFilt::PersonFilt() : PPBaseFilt(PPFILT_PERSON, 0, 1), P_RegF(0), P_TagF(0), P_SjF(0)
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

void SLAPI PersonFilt::Setup()
{
	SrchStr_.Strip();
	if(P_RegF) {
		if(!P_RegF->SerPattern.NotEmptyS() && !P_RegF->NmbPattern.NotEmptyS() && P_RegF->RegPeriod.IsZero() && P_RegF->ExpiryPeriod.IsZero()) {
			ZDELETE(P_RegF);
		}
		else
			P_RegF->Oid.Obj = PPOBJ_PERSON; // @v10.0.1
	}
	if(P_TagF && P_TagF->IsEmpty())
		ZDELETE(P_TagF);
}

int SLAPI PersonFilt::IsEmpty() const
{
	int    yes = 1;
	if(Kind || Category || Status || CityID || (P_RegF && !P_RegF->IsEmpty()) ||
		Flags & fVatFree || StaffDivID || StaffOrgID || List.GetCount() || (P_TagF && !P_TagF->IsEmpty()) || (P_SjF && !P_SjF->IsEmpty()))
		yes = 0;
	else {
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v10.0.1
		if(GetExtssData(extssNameText, r_temp_buf) > 0 && r_temp_buf.NotEmptyS())
			yes = 0;
		else if(GetExtssData(extssEmailText, r_temp_buf) > 0 && r_temp_buf.NotEmptyS())
			yes = 0;
	}
	return yes;
}

PersonFilt & FASTCALL PersonFilt::operator = (const PersonFilt &src)
{
	Copy(&src, 0);
	Setup();
	return *this;
}

int SLAPI PersonFilt::GetExtssData(int fldID, SString & rBuf) const { return PPGetExtStrData_def(fldID, extssNameText, SrchStr_, rBuf); }
int SLAPI PersonFilt::PutExtssData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, SrchStr_, pBuf); }

//virtual
int SLAPI PersonFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		struct PersonFilt_v0 : public PPBaseFilt {
			SLAPI  PersonFilt_v0() : PPBaseFilt(PPFILT_PERSON, 0, 0), P_RegF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(PersonFilt, ReserveStart), offsetof(PersonFilt, P_RegF) - offsetof(PersonFilt, ReserveStart));
				SetBranchBaseFiltPtr(PPFILT_REGISTER, offsetof(PersonFilt, P_RegF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(PersonFilt, P_TagF));
				SetBranchSString(offsetof(PersonFilt_v0, SrchStr));
				Init(1, 0);
			}
			uint8  ReserveStart[32];  // @anchor
			PPID   Kind;
			PPID   Category;
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
		CPYFLD(Kind);
		CPYFLD(Category);
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
static int SLAPI EditRegFilt(PersonFilt * pFilt)
{
	class PersonFiltAdvDialog : public TDialog {
	public:
		PersonFiltAdvDialog() : TDialog(DLG_PFLTADVOPT)
		{
			SetupCalPeriod(CTLCAL_PFLTADVOPT_REGPRD,    CTL_PFLTADVOPT_REGPRD);
			SetupCalPeriod(CTLCAL_PFLTADVOPT_EXPIRYPRD, CTL_PFLTADVOPT_EXPIRYPRD);
		}
		int setDTS(const PersonFilt * pData)
		{
			Data.Copy(pData, 0);
			RVALUEPTR(Reg, Data.P_RegF);
			SetupExtRegCombo(this, CTLSEL_PFLTADVOPT_REG, &Data, 1, 1);
			setCtrlString(CTL_PFLTADVOPT_SERIAL, Reg.SerPattern);
			setCtrlString(CTL_PFLTADVOPT_NUMBER, Reg.NmbPattern);
			SetPeriodInput(this, CTL_PFLTADVOPT_REGPRD,    &Reg.RegPeriod);
			SetPeriodInput(this, CTL_PFLTADVOPT_EXPIRYPRD, &Reg.ExpiryPeriod);
			return 1;
		}
		int getDTS(PersonFilt * pData)
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
				Reg.Oid.Obj = PPOBJ_PERSON; // @v10.0.1
				SETIFZ(pData->P_RegF, new RegisterFilt);
				*pData->P_RegF = Reg;
			}
			return 1;
		}
	private:
		PersonFilt Data;
		RegisterFilt Reg;
	};
	DIALOG_PROC_BODY(PersonFiltAdvDialog, pFilt);
}
//
//
//
int SLAPI PPViewPerson::EditBaseFilt(PPBaseFilt * pFilt)
{
	class PersonFiltDialog : public TDialog {
	public:
		PersonFiltDialog() : TDialog(DLG_PSNFLT), CluFlagsLock_(0)
		{
			SetupCalPeriod(CTLCAL_PSNFLT_NEWCLIPERIOD, CTL_PSNFLT_NEWCLIPERIOD);
			enableCommand(cmAdvOptions, 1);
		}
		int    setDTS(const PersonFilt * pFilt)
		{
			Data = *pFilt;
			SString temp_buf;
			SetupPPObjCombo(this, CTLSEL_PSNFLT_CITY, PPOBJ_WORLD, Data.CityID, OLW_CANSELUPLEVEL, 0);
			SetupPPObjCombo(this, CTLSEL_PSNFLT_KIND, PPOBJ_PRSNKIND, Data.Kind, 0, 0);
			SetupPPObjCombo(this, CTLSEL_PSNFLT_CATEGORY,   PPOBJ_PRSNCATEGORY, Data.Category, 0, 0);
			SetupPPObjCombo(this, CTLSEL_PSNFLT_STATUS, PPOBJ_PRSNSTATUS, Data.Status, 0, 0);
			SetupExtRegCombo(this, CTLSEL_PSNFLT_ATTR, &Data, 0, 0);
			if(Data.AttribType)
				setCtrlData(CTL_PSNFLT_EMPTY, &Data.EmptyAttrib);
			else
				disableCtrl(CTL_PSNFLT_EMPTY, 1);
			Data.GetExtssData(PersonFilt::extssNameText, temp_buf);
			setCtrlString(CTL_PSNFLT_NAMESTR, temp_buf);
			setCtrlUInt16(CTL_PSNFLT_VATFREE, BIN(Data.Flags & PersonFilt::fVatFree));
			AddClusterAssoc(CTL_PSNFLT_FLAGS, 0, PersonFilt::fTagsCrsstab);
			AddClusterAssoc(CTL_PSNFLT_FLAGS, 1, PersonFilt::fHasImages);
			AddClusterAssoc(CTL_PSNFLT_FLAGS, 2, PersonFilt::fShowHangedAddr);
			AddClusterAssoc(CTL_PSNFLT_FLAGS, 3, PersonFilt::fLocTagF);
			AddClusterAssoc(CTL_PSNFLT_FLAGS, 4, PersonFilt::fShowFiasRcgn);
			SetClusterData(CTL_PSNFLT_FLAGS, Data.Flags);
			DisableClusterItem(CTL_PSNFLT_FLAGS, 2, Data.AttribType != PPPSNATTR_ALLADDR);
			DisableClusterItem(CTL_PSNFLT_FLAGS, 3, !Data.IsLocAttr());
			{
				PPLocationConfig loc_cfg;
				PPObjLocation::FetchConfig(&loc_cfg);
				DisableClusterItem(CTL_PSNFLT_FLAGS, 4, !(loc_cfg.Flags & PPLocationConfig::fUseFias) || !Data.IsLocAttr());
			}
			if(PsnObj.GetConfig().NewClientDetectionList.getCount()) {
				disableCtrl(CTL_PSNFLT_NEWCLIPERIOD, 0);
				SetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &Data.NewCliPeriod);
				AddClusterAssoc(CTL_PSNFLT_NEWCLIONLY, 0, PersonFilt::fNewClientsOnly); // @v9.4.5
				SetClusterData(CTL_PSNFLT_NEWCLIONLY, Data.Flags); // @v9.4.5
				disableCtrl(CTL_PSNFLT_NEWCLIONLY, Data.NewCliPeriod.IsZero());
			}
			else {
				disableCtrl(CTL_PSNFLT_NEWCLIPERIOD, 1);
				disableCtrl(CTL_PSNFLT_NEWCLIONLY, 1);
			}
			return 1;
		}
		int    getDTS(PersonFilt * pFilt)
		{
			SString temp_buf;
			getCtrlData(CTLSEL_PSNFLT_CITY, &Data.CityID);
			getCtrlData(CTLSEL_PSNFLT_KIND, &Data.Kind);
			getCtrlData(CTLSEL_PSNFLT_CATEGORY, &Data.Category);
			getCtrlData(CTLSEL_PSNFLT_STATUS, &Data.Status);
			GetExtRegData(this, CTLSEL_PSNFLT_ATTR, &Data.AttribType, &Data.RegTypeID);
			getCtrlData(CTL_PSNFLT_EMPTY, &Data.EmptyAttrib);
			getCtrlString(CTL_PSNFLT_NAMESTR, temp_buf.Z());
			Data.PutExtssData(PersonFilt::extssNameText, temp_buf);
			SETFLAG(Data.Flags, PersonFilt::fVatFree, getCtrlUInt16(CTL_PSNFLT_VATFREE));
			GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
			if(PsnObj.GetConfig().NewClientDetectionList.getCount()) {
				GetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &Data.NewCliPeriod);
				GetClusterData(CTL_PSNFLT_NEWCLIONLY, &Data.Flags); // @v9.4.5
			}
			ASSIGN_PTR(pFilt, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmAdvOptions)) {
				EditRegFilt(&Data);
			}
			else if(event.isClusterClk(CTL_PSNFLT_FLAGS)) {
				if(!CluFlagsLock_) {
					CluFlagsLock_ = 1;
					const long preserve_flags = Data.Flags;
					GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
					if((Data.Flags & PersonFilt::fLocTagF) != (preserve_flags & PersonFilt::fLocTagF)) {
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
				}
			}
			else if(event.isCmd(cmCBSelected)) {
				const long preserve_attr = Data.AttribType;
				const long preserve_regtypeid = Data.RegTypeID;
				const int preserve_loc_attr = Data.IsLocAttr();
				int    do_rollback = 0;
				getDTS(0);
				if(preserve_loc_attr != Data.IsLocAttr() && (Data.P_TagF && !Data.P_TagF->IsEmpty())) {
                    const int c1 = BIN(Data.Flags & PersonFilt::fLocTagF && !Data.IsLocAttr());
					const int c2 = 0; //BIN(!(Data.Flags & PersonFilt::fLocTagF) && Data.IsLocAttr());
					if(c1 || c2) {
						if(CONFIRM(PPCFM_PSNFLTTAGFRESET)) {
							ZDELETE(Data.P_TagF);
						}
						else
							do_rollback = 1;
					}
				}
				if(do_rollback) {
					Data.AttribType = preserve_attr;
					Data.RegTypeID = preserve_regtypeid;
				}
				SetupExtRegCombo(this, CTLSEL_PSNFLT_ATTR, &Data, 0, 0);
				GetExtRegData(this, CTLSEL_PSNFLT_ATTR, &Data.AttribType, &Data.RegTypeID);
				if(!Data.AttribType) {
					setCtrlUInt16(CTL_PSNFLT_EMPTY, 0);
					disableCtrl(CTL_PSNFLT_EMPTY, 1);
				}
				else
					disableCtrl(CTL_PSNFLT_EMPTY, 0);
				DisableClusterItem(CTL_PSNFLT_FLAGS, 2, Data.AttribType != PPPSNATTR_ALLADDR);
				DisableClusterItem(CTL_PSNFLT_FLAGS, 3, !Data.IsLocAttr());
				{
					PPLocationConfig loc_cfg;
					PPObjLocation::FetchConfig(&loc_cfg);
					DisableClusterItem(CTL_PSNFLT_FLAGS, 4, !(loc_cfg.Flags & PPLocationConfig::fUseFias) || !Data.IsLocAttr());
				}
			}
			else if(event.isCmd(cmTags)) {
				GetClusterData(CTL_PSNFLT_FLAGS, &Data.Flags);
				const PPID tag_obj_type = (Data.Flags & PersonFilt::fLocTagF) ? PPOBJ_LOCATION : PPOBJ_PERSON;
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
					// Функция SysJournalFilt::IsEmpty считает фильтр, в котором установлен ObjType
					// не пустым. В данном случае это - не верно.
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
				DateRange temp_period;
				temp_period.Z();
				GetPeriodInput(this, CTL_PSNFLT_NEWCLIPERIOD, &temp_period);
				disableCtrl(CTL_PSNFLT_NEWCLIONLY, temp_period.IsZero());
			}
			else
				return;
			clearEvent(event);
		}
		int    CluFlagsLock_;
		PersonFilt Data;
		PPObjPerson PsnObj;
	};
	DIALOG_PROC_BODY(PersonFiltDialog, (PersonFilt*)pFilt);
}
//
//
//
int SLAPI PPViewPerson::CreateLikenessTable()
{
	int ok = -1;
	return ok;
}

int SLAPI PPViewPerson::OnExecBrowser(PPViewBrowser * pBrw)
{
	pBrw->SetupToolbarCombo(PPOBJ_PRSNKIND, Filt.Kind, 0, 0);
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
		PPID   psn_id = (Filt.AttribType != PPPSNATTR_HANGEDADDR) ? *(long *)pData : 0;
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

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewPerson * p_view = (PPViewPerson*)extraPtr;
	if(pData && pCellStyle && p_view) {
		PersonFilt * p_filt = (PersonFilt*)p_view->GetBaseFilt();
		if(!p_view->IsCrosstab() && p_filt) {
			int is_register  = (p_filt->AttribType == PPPSNATTR_REGISTER) ? oneof2(p_filt->RegTypeID, PPREGT_OKPO, PPREGT_TPID/*, PPREGT_BNKCORRACC*/) : 0;
			int is_bank_acct = p_filt->AttribType == PPPSNATTR_BNKACCT;
			if(col == 0 && p_view->HasImage(pData)) { // К персоналии привязана картинка?
				pCellStyle->Flags  = BrowserWindow::CellStyle::fLeftBottomCorner;
				pCellStyle->Color2 = GetColorRef(SClrGreen);
				ok = 1;
			}
			else if(col == 1 && p_view->IsNewCliPerson(*(PPID *)pData)) {
				pCellStyle->Flags = 0;
				pCellStyle->Color = GetColorRef(SClrOrange);
				ok = 1;
			}
			/* @v9.8.4 @todo Из-за замены текстовых полей во временной таблице на ссылки в StrPool следующий блок надо переделать
			else if((is_register && col == 3) || (is_bank_acct && col == 5)) {
				int is_valid = 0;
				SString code, bic;
				code = (is_bank_acct) ? ((BnkAcct_*)pData)->BnkAcct : ((Register_*)pData)->RegNumber;
				bic  = (is_bank_acct) ? ((BnkAcct_*)pData)->BIC     : ((Register_*)pData)->RegSerial;
				if(code.Strip().Len()) {
					if(is_bank_acct)
						is_valid = CheckBnkAcc(code, bic.Strip());
					else {
						if(p_filt->RegTypeID == PPREGT_OKPO)
							is_valid = CheckOKPO(code);
						else if(p_filt->RegTypeID == PPREGT_TPID) {
							// @v8.7.4 is_valid = CheckINN(code);
							is_valid = SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, code, code.Len()); // @v8.7.4
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
}

// virtual
void SLAPI PPViewPerson::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & PersonFilt::fShowFiasRcgn && Filt.IsLocAttr() && P_TempPsn) {
			BrowserDef * p_def = pBrw->getDef();
			if(p_def) {
				pBrw->InsColumn(-1, "FIAS ADDR",  8, 0, MKSFMT(32, 0), 0);
				pBrw->InsColumn(-1, "FIAS HOUSE", 9, 0, MKSFMT(32, 0), 0);
			}
		}
		pBrw->SetCellStyleFunc(CellStyleFunc, this);
	}
}

DBQuery * SLAPI PPViewPerson::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DBQuery * q  = 0;
	PersonTbl      * p  = 0;
	TempPersonTbl  * tmp_pt = 0;
	PersonKindTbl  * k  = 0;
	LocationTbl    * lt = 0;
	uint   brw_id = 0;
	SString title_buf;
	if(P_Ct)
		brw_id = BROWSER_PERSONCT;
	else if(Filt.AttribType == PPPSNATTR_PHONEADDR)
		brw_id = BROWSER_PERSONATTR_PHONEADDR;
	else if(Filt.AttribType == PPPSNATTR_EMAIL)
		brw_id = BROWSER_PERSONATTR_EMAIL;
	else if(oneof4(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DUPDLVRADDR))
		brw_id = BROWSER_PERSONATTR_DLVRADDR;
	else if(Filt.AttribType == PPPSNATTR_STANDALONEADDR)
		brw_id = BROWSER_PERSONATTR_STDALNADDR;
	else if(Filt.AttribType == PPPSNATTR_BNKACCT)
		brw_id = BROWSER_PERSONATTR_BNKACCT;
	else if(Filt.AttribType == PPPSNATTR_REGISTER)
		brw_id = BROWSER_PERSONATTR_REGISTER;
	else if(Filt.AttribType == PPPSNATTR_TAG)
		brw_id = BROWSER_PERSONATTR_TAG;
	else if(Filt.Flags & PersonFilt::fTagsCrsstab)
		brw_id = BROWSER_PERSONCT;
	else
		brw_id = BROWSER_PERSON;
	if(Filt.Kind)
		GetObjectName(PPOBJ_PRSNKIND, Filt.Kind, title_buf);
	if(Filt.AttribType) {
		title_buf.CatDivIfNotEmpty('-', 1);
		PPID   reg_id = Filt.RegTypeID;
		if(reg_id) {
			SArray * p_ary = CreateExtRegList(&Filt, &reg_id, 0);
			if(p_ary) {
				uint   pos = 0;
				ExtRegEntry * p_ere = p_ary->lsearch(&reg_id, &pos, CMPF_LONG) ? (ExtRegEntry *)p_ary->at(pos) : 0;
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
		//DBE    dbe_regnum;
		DBE    dbe_regser;
		DBE    dbe_fiasadrguid;
		DBE    dbe_fiashseguid;
		DBQ  * dbq = 0;
		int    tbl_count = 0;
		DBTable * tbl_l[12];
		memzero(tbl_l, sizeof(tbl_l));
		if(P_TempPsn) {
			THROW(CheckTblPtr(tmp_pt = new TempPersonTbl(P_TempPsn->GetName())));
			tbl_l[tbl_count++] = tmp_pt;
			if(!oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
				THROW(CheckTblPtr(p = new PersonTbl));
				tbl_l[tbl_count++] = p;
				if(Filt.AttribType == PPPSNATTR_ALLADDR && Filt.Flags & PersonFilt::fShowHangedAddr)
					dbq = & (p->ID += tmp_pt->ID);
				else
					dbq = & (p->ID == tmp_pt->ID);
			}
		}
		else {
			THROW(CheckTblPtr(p = new PersonTbl));
			if(Filt.Kind) {
				THROW(CheckTblPtr(k = new PersonKindTbl));
				dbq = & (k->KindID == Filt.Kind && p->ID == k->PersonID);
				tbl_l[tbl_count++] = k;
			}
			tbl_l[tbl_count++] = p;
			dbq = ppcheckfiltid(dbq, p->Status, Filt.Status);
			dbq = ppcheckfiltid(dbq, p->CatID, Filt.Category);
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
					cq.push((DBFunc)PPDbqFuncPool::IdWorldIsMemb);
					dbq = & (*dbq && cq == (long)1);
				}
			}
		}
		switch(Filt.AttribType) {
			case PPPSNATTR_PHONEADDR:
				{
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone, tmp_pt->PhoneP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr,  tmp_pt->AddressP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_raddr, tmp_pt->RAddressP, &StrPool);
					q = & select(
						p->ID,
						p->Name,
						/*tmp_pt->Phone*/dbe_phone,
						/*tmp_pt->Address*/dbe_addr,
						/*tmp_pt->RAddress*/dbe_raddr,
						p->Flags,
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
				}
				break;
			case PPPSNATTR_EMAIL:
				{
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone, tmp_pt->PhoneP, &StrPool);
					q = & select(
						p->ID,
						p->Name,
						/*tmp_pt->Phone*/dbe_phone,
						p->Flags,
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
					//PPDbqFuncPool::InitStrPoolRefFunc(dbe_regnum,  tmp_pt->RegNumberP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiasadrguid,  tmp_pt->FiasAddrGuidP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_fiashseguid,  tmp_pt->FiasHouseGuidP, &StrPool);
					q = & select(
						p->ID,                // #0
						tmp_pt->TabID,        // #1 ИД адреса
						p->Name,              // #2 Наименование персоналии
						/*tmp_pt->Address*/dbe_addr,     // #3 Строка адреса
						/*tmp_pt->BnkAcct*/dbe_bnkacct,  // #4 Код из адреса доставки
						tmp_pt->RegNumber,    // #5 Тип адреса (юридический | физический | доставки)
						dbe_city,             // #6
						p->Flags,             // #7
						/*tmp_pt->FiasAddrGuid*/dbe_fiasadrguid,  // #8
						/*tmp_pt->FiasHouseGuid*/dbe_fiashseguid, // #9
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], 0L);
				}
				break;
			case PPPSNATTR_HANGEDADDR:
			case PPPSNATTR_STANDALONEADDR:
				{
					PPDbqFuncPool::InitObjNameFunc(dbe_city, PPDbqFuncPool::IdObjNameWorld,  tmp_pt->CityID);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_addr, tmp_pt->AddressP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
					//PPDbqFuncPool::InitStrPoolRefFunc(dbe_regnum,  tmp_pt->RegNumberP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone,  tmp_pt->PhoneP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkname,  tmp_pt->BnkNameP, &StrPool);
					q = & select(
						tmp_pt->ID,        // #0
						tmp_pt->TabID,     // #1 ИД адреса
						tmp_pt->Name,      // #2 Наименование персоналии
						/*tmp_pt->Address*/dbe_addr,     // #3 Строка адреса
						/*tmp_pt->BnkAcct*/dbe_bnkacct,  // #4 Код из адреса доставки
						tmp_pt->RegNumber, // #5 Тип адреса (юридический | физический | доставки)
						dbe_city,          // #6
						/*tmp_pt->Phone*/dbe_phone,      // #7 Телефон (ассоциированный с адресом)
						/*tmp_pt->BnkName*/dbe_bnkname,  // #8 Контакт (ассоциированный с адресом)
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], 0L);
				}
				break;
			case PPPSNATTR_BNKACCT:
				{
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkname,  tmp_pt->BnkNameP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_bnkacct, tmp_pt->BnkAcctP, &StrPool);
					//PPDbqFuncPool::InitStrPoolRefFunc(dbe_regnum,  tmp_pt->RegNumberP, &StrPool);
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_phone,  tmp_pt->PhoneP, &StrPool);
					q = & select(
						p->ID,               // #0
						tmp_pt->TabID,       // #1
						p->Name,             // #2
						/*tmp_pt->BnkName*/dbe_bnkname,     // #3
						/*tmp_pt->BnkAcct*/dbe_bnkacct,     // #4
						tmp_pt->RegNumber,    // #5 Тип счета
						tmp_pt->RegInitDate,        // #6 Дата открытия //
						/*tmp_pt->Phone*/dbe_phone, // #7 БИК банка
						p->Flags,                   // #8
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
				}
				break;
			case PPPSNATTR_REGISTER:
				{
					PPDbqFuncPool::InitStrPoolRefFunc(dbe_regser,  tmp_pt->RegSerialP, &StrPool);
					//PPDbqFuncPool::InitStrPoolRefFunc(dbe_regnum,  tmp_pt->RegNumberP, &StrPool);
					q = & select(
						p->ID,
						p->Name,
						/*tmp_pt->RegSerial*/dbe_regser,
						tmp_pt->RegNumber,
						tmp_pt->RegInitDate,
						tmp_pt->RegExpiry,
						p->Flags,
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
				}
				break;
			case PPPSNATTR_TAG:
				{
					//PPDbqFuncPool::InitStrPoolRefFunc(dbe_regnum,  tmp_pt->RegNumberP, &StrPool);
					q = & select(
						p->ID,
						p->Name,
						tmp_pt->RegNumber,
						p->Flags,
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
				}
				break;
			default:
				{
					DBE    dbe_status;
					DBE    dbe_cat;
					PPDbqFuncPool::InitObjNameFunc(dbe_status, PPDbqFuncPool::IdObjNamePersonStatus, p->Status);
					PPDbqFuncPool::InitObjNameFunc(dbe_cat,    PPDbqFuncPool::IdObjNamePersonCat,    p->CatID);
					q = & select(
						p->ID,
						p->Name,
						dbe_status,
						dbe_cat,
						p->Memo,
						p->Flags,
						0L).from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], 0L);
				}
				break;
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

int SLAPI PPViewPerson::Recover()
{
	int    ok = -1;
	PPLogger logger;
	SString fmt_buf, msg_buf, addr_buf;
	PPIDArray psn_list;
	PPIDArray conflict_owner_addr_list; // Список адресов доставки, принадлежащих нескольким владельцам
	LAssocArray dlvr_addr_list; // Список ассоциаций {DlvrAddrID; PersonID}
	LAssocArray invowner_addr_list; // Список ассоциация адресов доставки, имеющий не верного владельца {DlvrAddrID; CorrectOwnerID}
	PersonViewItem item;
	PPLocationPacket loc_pack;
	PPWait(1);
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
		dlvr_addr_list.Sort();
		LAssocArray bill_dlvr_addr_list;
		int    bill_dlvr_addr_list_inited = 0;
		PPIDArray bill_list;
		PPIDArray ar_list;
		const uint c = dlvr_addr_list.getCount();
		if(c) {
			PPObjBill * p_bobj = BillObj;
			uint i = c;
			do {
				LAssoc & r_assc = dlvr_addr_list.at(--i);
				PPID   prev_addr_id = ((i+1) < c) ? dlvr_addr_list.at(i+1).Key : 0;
				if(prev_addr_id && r_assc.Key == prev_addr_id) {
					conflict_owner_addr_list.add(r_assc.Key);
					if(!bill_dlvr_addr_list_inited) {
						p_bobj->P_Tbl->GetDlvrAddrList(&bill_dlvr_addr_list);
						bill_dlvr_addr_list_inited = 1;
					}
					{
						bill_list.clear();
						bill_dlvr_addr_list.GetListByVal(r_assc.Key, bill_list);
						ar_list.clear();
						for(uint j = 0; j < bill_list.getCount(); j++) {
							const PPID bill_id = bill_list.get(j);
							BillTbl::Rec bill_rec;
							if(p_bobj->Search(bill_id, &bill_rec) > 0)
								ar_list.add(bill_rec.Object);
						}
						if(PsnObj.LocObj.GetPacket(r_assc.Key, &loc_pack) > 0)
							LocationCore::GetAddress(loc_pack, 0, addr_buf);
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
						else if(ar_list.getCount() > 0) {
							str_id = PPTXT_LOG_MANYLINGADDR_MANYBILL;
						}
						else {
							str_id = PPTXT_LOG_MANYLINGADDR_NOBILL;
						}
						{
							if(PPLoadText(str_id, fmt_buf)) {
								PPFormat(fmt_buf, &msg_buf, addr_buf.cptr(), psn_id);
								logger.Log(msg_buf);
							}
						}
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
				LocationCore::GetAddress(loc_pack, 0, addr_buf);
				const PPID inv_owner_id = loc_pack.OwnerID;
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
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPerson::RemoveHangedAddr()
{
	int    ok = -1;
	if(Filt.AttribType == PPPSNATTR_HANGEDADDR && P_TempPsn && CONFIRMCRIT(PPCFM_DELALLHANGEDADDR)) {
		PPIDArray addr_list;
		PPWait(1);
		{
			char    k[MAXKEYLEN];
			memzero(k, sizeof(k));
			BExtQuery q(P_TempPsn, 0);
			q.select(P_TempPsn->TabID, P_TempPsn->AddressP, 0);
			for(q.initIteration(0, &k, spFirst); q.nextIteration() > 0;) {
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
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewPerson::SendMail(PPID mailAccId, StrAssocArray * pMailList, PPLogger * pLogger)
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
			PPWait(1);
			for(uint i = 0; i < pMailList->getCount(); i++) {
				PPSmsSender::FormatMessageBlock fmb;
				fmb.PersonID = pMailList->Get(i).Id;
				const  char * p_mail_addr = pMailList->Get(i).Txt;
				SString text, subj;
				if(i && data.Delay > 0 && data.Delay <= (24 * 3600 * 1000)) {
					SDelay(data.Delay);
				}
				PPSmsSender::FormatMessage(data.Text, text, &fmb);
				PPSmsSender::FormatMessage(data.Subj, subj, &fmb);
				subj.Transf(CTRANSF_INNER_TO_UTF8);
				text.Transf(CTRANSF_INNER_TO_UTF8);
				THROW(::SendMail(subj, text, pMailList->Get(i).Txt, data.MailAccID, &data.FilesList, pLogger));
			}
			PPWait(0);
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

int SLAPI PPViewPerson::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString phone_buf, contact_buf, addr_buf;
	PPLogger logger;
	if(Filt.AttribType == PPPSNATTR_STANDALONEADDR) {
		PPUhttClient uhtt_cli;
		PPWait(1);
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
					loc_text_buf = 0;
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
						if(addr_buf.NotEmpty() && ret_loc_pack.Address.Empty())
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
		PPWait(0);
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PPViewPerson::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	if(pHdr) {
		hdr = *(BrwHdr *)pHdr;
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
			case PPVCMD_ADDITEM:
				{
					PPID id = 0;
					ok = AddItem(&id);
					if(ok > 0) {
						pBrw->Update();
						pBrw->search2(&id, PTR_CMPFUNC(long), srchFirst, 0);
						ok = -1; // Не надо обновлять таблицу после этого вызова
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				if(Filt.AttribType == PPPSNATTR_HANGEDADDR) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
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
				if(!oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					ok = DeleteItem(0);
				}
				break;
			case PPVCMD_EDITITEM:
				if(oneof2(Filt.AttribType, PPPSNATTR_HANGEDADDR, PPPSNATTR_STANDALONEADDR)) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
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
			case PPVCMD_TAGS:
				ok = -1;
				{
					PPID    obj_type = 0;
					PPID    obj_id = 0;
					if(Filt.IsLocAttr() && Filt.Flags & PersonFilt::fLocTagF) {
						obj_type = PPOBJ_LOCATION;
						obj_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
					}
					else {
						obj_type = PPOBJ_PERSON;
						obj_id = hdr.ID;
					}
					if(obj_type && obj_id)
						ok = EditObjTagValList(obj_type, obj_id, 0);
				}
				break;
			case PPVCMD_CURREG:
				if(Filt.IsLocAttr()) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id)
						ok = (PsnObj.LocObj.Edit(&loc_id, 0) == cmOK) ? 1 : -1;
				}
				else
					ok = EditRegs(hdr.ID, 1);
				break;
			case PPVCMD_DLVRADDREXFLDS:
				if(Filt.IsLocAttr()) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id)
						if((ok = EditDlvrAddrExtFlds(loc_id)) == 0)
							PPError();
				}
				break;
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
				if(oneof2(Filt.AttribType, PPPSNATTR_STANDALONEADDR, PPPSNATTR_HANGEDADDR)) {
					const PPID loc_id = pHdr ? ((long *)pHdr)[1] : 0;
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
				if(oneof2(Filt.AttribType, PPPSNATTR_STANDALONEADDR, PPPSNATTR_HANGEDADDR)) {
					const PPID loc_id = pHdr ? ((long *)pHdr)[1] : 0;
					if(loc_id) {
						ok = ViewSysJournal(PPOBJ_LOCATION, loc_id, 0);
					}
				}
				else if(hdr.ID)
					ok = ViewSysJournal(PPOBJ_PERSON, hdr.ID, 0);
				break;
			case PPVCMD_UNITEOBJ:
				if(oneof4(Filt.AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_HANGEDADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
					if(loc_id)
						ok = PPObjPerson::ReplaceDlvrAddr(loc_id);
				}
				else
					ok = PPObjPerson::ReplacePerson(hdr.ID, Filt.Kind);
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
					if(pBrw && pBrw->GetToolbarComboData(&psn_kind) && Filt.Kind != psn_kind) {
						Filt.Kind = psn_kind;
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_MOUSEHOVER:
				{
					const  int has_images = HasImage(pHdr);
					long   h = 0;
					pBrw->ItemByMousePos(&h, 0);
					if(oneof2(h, 0, 1)) {
						int r = 0;
						SString buf;
						PPELinkArray phones_ary;
						PersonCore::GetELinks(hdr.ID, &phones_ary);
						for(uint i = 0; i < phones_ary.getCount(); i++) {
							if(i != 0)
								buf.CR();
							GetObjectName(PPOBJ_ELINKKIND, phones_ary.at(i).KindID, buf, 1);
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
								SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|
								SMessageWindow::fChildWindow);
						}
					}
				}
				break;
			case PPVCMD_SENDSMS:
				{
					uint   what = 0;
					if(SelectorDialog(DLG_DISPTYPE, CTL_DISPTYPE_WHAT, &what) > 0) {
						PPAlbatrosConfig albtr_cfg;
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
				if(Filt.AttribType == PPPSNATTR_STANDALONEADDR) {
					PPID   loc_id = (PPID)(pHdr ? PTR32C(pHdr)[1] : 0);
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
			case PPVCMD_REGISTERS:      ok = EditRegs(hdr.ID, 0); break;
			case PPVCMD_AMOUNTS:        ok = PsnObj.EditAmountList(hdr.ID); break;
			case PPVCMD_ADDREL:         ok = AddRelation(hdr.ID); break;
			case PPVCMD_PRINT:          ok = Print(0); break;
			case PPVCMD_TOTAL:          ok = ViewTotal(); break;
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

int SLAPI PPViewPerson::CreateAuthFile(PPID psnId)
{
	int    ok = 1;
	DbProvider * p_dict = CurDict;
	const  char * p_auth_fname = "agauth.bin";
	S_GUID guid;
	SString temp_buf;
	SFile  file;
	Sdr_CPosAuth rec;
	MEMSZERO(rec);
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
		PPRef->LoadSecur(PPOBJ_USR, DS.GetTLA().Lc.User, &secur_pack);
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

int SLAPI PPViewPerson::GetSmsLists(StrAssocArray & rPsnList, StrAssocArray & rPhoneList, uint what /*=0*/)
{
	size_t i = 0;
 	SString buf, phone;
	PersonViewItem item;

 	for(InitIteration(); NextIteration(&item) > 0;) {
		PPELinkArray elink_list;
		if(PersonCore::GetELinks(item.ID, &elink_list) > 0) {
			if(what > 0)
				elink_list.GetPhones(1, phone = 0, ELNKRT_EMAIL);
			else {
				elink_list.GetItem(PPELK_MOBILE, phone.Z());
				if(phone.Empty())
					elink_list.GetItem(PPELK_WORKPHONE, phone.Z());
			}
 			if(phone.NotEmpty()) {
 				buf.Z().Cat(item.ID);
 				rPsnList.Add(i, buf);
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
int SLAPI ViewPerson(const PersonFilt * pFilt) { return PPView::Execute(PPVIEW_PERSON, pFilt, PPView::exefModeless, 0); }

int SLAPI EditMainOrg()
{
	int    ok = -1;
	PersonFilt flt;
	PPObjPerson psn_obj;
	PPCommConfig cfg;
	PPID   id = 0;
	PersonTbl::Rec psn_rec;
	MEMSZERO(cfg);
	THROW(GetCommConfig(&cfg));
	flt.Kind = PPPRK_MAIN;
	if(cfg.MainOrgID > 0 && psn_obj.P_Tbl->Search(cfg.MainOrgID) > 0)
		id = cfg.MainOrgID;
	else if(psn_obj.P_Tbl->SearchMainOrg(&psn_rec) > 0)
		id = psn_rec.ID;		//get id;
	cfg.MainOrgID = id;
	THROW(SetCommConfig(&cfg, 1));
	if(psn_obj.ExtEdit(&id, (void *)flt.Kind) == cmOK) {
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
int SLAPI PPViewPerson::Print(const void *)
{
	int    rpt_id = 0;
	PView  pv(this);
	switch(Filt.AttribType) {
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
	return PPAlddPrint(rpt_id, &pv, 0);
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
			STRNSCPY(H.Code, pc_rec.Symb); // @v6.2.1
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
	H.PsnKindID  = p_filt->Kind;
	H.StatusID   = p_filt->Status;
	H.CountryID  = 0;
	H.CityID     = p_filt->CityID;
	H.AttrType   = p_filt->AttribType;
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
	const  PersonFilt * p_filt = (const PersonFilt *)p_v->GetBaseFilt();
	I.PersonID = item.ID;
	if(p_filt) {
		if(oneof3(p_filt->AttribType, PPPSNATTR_ALLADDR, PPPSNATTR_DLVRADDR, PPPSNATTR_DUPDLVRADDR)) {
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
		if(BillObj->trfr->Rcpt.GetSupplList(rFilt.ID, 0, LConfig.OperDate, &suppl_list) > 0) {
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
