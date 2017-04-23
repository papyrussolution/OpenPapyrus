// PPDBMAKE.CPP
// Copyright (c) Osolotkin A.V, Sobolev A. 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2013, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

static int isNeededFile(const char * pName)
{
	int    ok = 0;
	SString temp_buf;
	for(int i = 0; !ok && PPGetSubStr(PPTXT_NEEDEDFILES, i, temp_buf) > 0; i++)
		if(temp_buf.CmpNC(pName) == 0)
			ok = 1;
	return ok;
}

static int64 calcNeededSize(int isEmpty)
{
	DbProvider * p_dict = CurDict;
	int64  size = 0;
	SString empty_path, base_path, file_name, tbl_name;
	DbTableStat ts;
	StrAssocArray tbl_list;
	SPathStruc ps;
	PPGetFilePath(PPPATH_ROOT, "empty", empty_path);
	empty_path.SetLastSlash();
	PPGetPath(PPPATH_DAT, base_path);
	base_path.SetLastSlash();
	p_dict->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.at(j);
		if(p_dict->GetTableInfo(item.Id, &ts) && !(ts.Flags & XTF_DICT)) {
			file_name = ts.Location;
			ps.Split(file_name);
			if(ps.Nam.C(0) != '_' && ps.Nam.CmpPrefix("tmp", 1) != 0) {
				tbl_name = ts.TblName;
				file_name = (isEmpty || !isNeededFile(tbl_name)) ? empty_path : base_path;
				file_name.Cat(ps.Nam).Cat(ps.Ext);
				if(::access(file_name, 0) == 0) {
					SFileUtil::Stat st;
					SFileUtil::GetStat(file_name, &st);
					size += st.Size;
				}
				else
					size += 2048;
				PPWaitMsg(file_name);
			}
		}
	}
	return size;
}

/* @v9.2.1 Использование заменено на RemoveDir()
static void RemovePath(const char * pPath)
{
	SDirEntry sde;
	SString src_path, file_name;
	(file_name = (src_path = pPath).SetLastSlash()).SetLastSlash().Cat("*.*");
	for(SDirec sd(file_name); sd.Next(&sde) > 0;) {
		SFile::Remove((file_name = src_path).Cat(sde.FileName));
	}
	// @v9.2.1 rmdir(src_path.RmvLastSlash());
	::RemoveDirectory(src_path.RmvLastSlash()); // @v9.2.1
}
*/

int SLAPI CreateByExample(const char * pPath)
{
	int    ok = 1;
	SString file_name, src_path, dst_path, pp, tbl_name;
	SPathStruc ps;
	DBTable * p_dst_tbl = 0;
	DBTable * p_src_tbl = 0;
	DbTableStat ts;
	StrAssocArray tbl_list;

	PPGetPath(PPPATH_DAT, pp);
	pp.SetLastSlash();
	PPWait(1);
	//
	// Вычисляем необходимый размер дискового пространства и сравниваем его с доступным
	//
	int64  disk_total = 0, disk_avail = 0;
	SFileUtil::GetDiskSpace(pPath, &disk_total, &disk_avail);
	// *2 - коэффициент запаса
	if((calcNeededSize(0) * 2) > disk_avail) {
		DBErrCode = SDBERR_BU_NOFREESPACE;
		CALLEXCEPT_PP(PPERR_DBLIB);
	}
	//
	DbProvider * p_dict = CurDict;
	p_dict->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.at(j);
		if(p_dict->GetTableInfo(item.Id, &ts) && !(ts.Flags & XTF_DICT)) {
			file_name = ts.Location;
			(src_path = pp).Cat(file_name.Strip());
			ps.Split(file_name);
			if(file_name.NotEmpty() && ps.Nam.C(0) != '_' && ps.Nam.CmpPrefix("tmp", 1) != 0 && fileExists(src_path)) {
				(dst_path = pPath).SetLastSlash().Cat(file_name);
				if(::access(dst_path, 0) == 0)
					THROW_SL(SFile::Remove(dst_path));
				tbl_name = ts.TblName;
				PPWaitMsg(tbl_name);
				THROW_MEM(p_dst_tbl = new DBTable(tbl_name, dst_path));
				THROW(p_dst_tbl->allocOwnBuffer(4096));
				if(fileExists(src_path)) {
					THROW_MEM(p_src_tbl = new DBTable(tbl_name));
					THROW(p_src_tbl->allocOwnBuffer(4096));
					if(isNeededFile(tbl_name)) {
						RECORDNUMBER rn, i = 0;
						PPTransaction tra(1);
						THROW(tra);
						p_src_tbl->getNumRecs(&rn);
						if(p_src_tbl->step(spFirst)) {
							int    is_assoc = BIN(tbl_name.CmpNC("objassoc") == 0);
							do {
								int   ins_rec = 0;
								p_dst_tbl->clearDataBuf();
								p_dst_tbl->copyBufLobFrom(p_src_tbl->getDataBuf(), p_src_tbl->getRetBufLen());
								if(is_assoc) {
									ObjAssocTbl::Rec * p_rec = (ObjAssocTbl::Rec*)p_src_tbl->getDataBuf();
									if(!oneof10(p_rec->AsscType, PPASS_BILLSET, PPASS_PAYMBILLPOOL, PPASS_OPBILLPOOL,
										PPASS_CSESSBILLPOOL, PPASS_TSESSBILLPOOL, PPASS_CSDBILLPOOL, PPASS_TSDBILLPOOL,
										PPASS_PRJBILLPOOL, PPASS_PRJPHASEBILLPOOL, PPASS_TODOBILLPOOL))
										ins_rec = 1;
								}
								else
									ins_rec = 1;
								if(ins_rec && !p_dst_tbl->insertRec()) {
									PPSaveErrContext();
									SString msg_buf, rec_txt_buf;
									msg_buf.Cat(p_dst_tbl->fileName).CatDiv('-', 1);
									//
									// Так как список полей	в p_dst_tbl не инициализирован,
									// а буфер этой таблицы идентичен буферу p_src_tbl,
									// то для вывода содержимого буфера используем p_src_tbl.
									//
									p_src_tbl->putRecToString(rec_txt_buf, 1);
									msg_buf.Cat(rec_txt_buf);
									PPRestoreErrContext();
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
									PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, 0);
									//CALLEXCEPT_PP(PPERR_DBENGINE);
								}
								p_src_tbl->clearDataBuf(); // @v8.7.2
								PPWaitPercent(i++, rn, tbl_name);
								THROW(PPCheckUserBreak());
							} while(p_src_tbl->step(spNext));
						}
						THROW(tra.Commit()); // @v7.9.6
						PPWaitPercent(rn, rn, tbl_name);
					}
				}
				THROW(PPCheckUserBreak());
			}
			ZDELETE(p_src_tbl);
			ZDELETE(p_dst_tbl);
		}
	}
	{
		//
		// Перенос последних лотов из оригинальной базы в новую
		//
		SString path;
		Goods2Tbl::Rec  goods_rec;
		ReceiptTbl::Rec rcpt_rec;
		RECORDNUMBER rn = 0, i = 0;
		ReceiptTbl dst_tbl((path = pPath).SetLastSlash().Cat("receipt.btr"));
		Goods2Tbl src_tbl((path = pPath).SetLastSlash().Cat("goods2.btr"));

		src_tbl.setDataBuf(&goods_rec, sizeof(Goods2Tbl::Rec));
		dst_tbl.setDataBuf(&rcpt_rec, sizeof(ReceiptTbl::Rec));
		src_tbl.getNumRecs(&rn);

		PPTransaction tra(1); // @v7.9.6
		THROW(tra); // @v7.9.6

		if(src_tbl.step(spFirst)) {
			ReceiptCore * p_rc = &BillObj->trfr->Rcpt;
			ObjTagCore dest_ot((path = pPath).SetLastSlash().Cat("objtag.btr")); // @v7.5.10
			MEMSZERO(rcpt_rec);
			do {
				if(p_rc->GetLastLot(goods_rec.ID, 0, MAXDATE, &rcpt_rec) > 0) {
					rcpt_rec.Quantity  = 0.0;
					rcpt_rec.Rest      = 0.0;
					rcpt_rec.Closed    = 1;
					rcpt_rec.BillID    = 0;
					rcpt_rec.PrevLotID = 0;
					THROW_DB(dst_tbl.insertRec());
					{
						//
						// @v7.5.10 {
						// Перенос тегов лота с исходной базы в создаваемую.
						//
						ObjTagList lot_tag_list;
						if(BillObj->GetTagListByLot(rcpt_rec.ID, 0, &lot_tag_list) > 0) {
							THROW(dest_ot.PutList(PPOBJ_LOT, rcpt_rec.ID, &lot_tag_list, 0));
						}
						// } @v7.5.10
					}
				}
				PPWaitPercent(i++, rn, p_rc->tableName);
				THROW(PPCheckUserBreak());
			} while(src_tbl.step(spNext));
		}
		THROW(tra.Commit()); // @v7.9.6
	}
	{
		long   val2 = 0;
		Reference2Tbl dest_tbl_ref((dst_path = pPath).SetLastSlash().Cat("ref2.btr"));
		Reference2Tbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = PPOBJ_UNASSIGNED;
		k0.ObjID = 1L;
		PPTransaction tra(1); // @v7.9.6
		THROW(tra);              // @v7.9.6
		if(dest_tbl_ref.searchForUpdate(0, &k0, spEq)) {
			val2 = dest_tbl_ref.data.Val2;
			THROW_DB(dest_tbl_ref.deleteRec()); // @sfu
		}
		if(val2) {
			ArticleTbl dest_tbl_ar((dst_path = pPath).SetLastSlash().Cat("article.btr"));
			THROW(RemoveByID(&dest_tbl_ar, val2, 0));
		}
		THROW(tra.Commit()); // @v7.9.6
	}
	PPWait(0);
	CATCH
		ZDELETE(p_src_tbl);
		ZDELETE(p_dst_tbl);
		// @v9.2.1 RemovePath(pPath);
		RemoveDir(pPath); // @v9.2.1
		ok = PPErrorZ();
	ENDCATCH
	delete p_dst_tbl;
	delete p_src_tbl;
	return ok;
}

int SLAPI MakeDatabase()
{
	#define FBB_GROUP1 1
	int    ok = -1, valid_data = 0;
	TDialog * dlg = new TDialog(DLG_MAKENEWDB);
	PPIniFile ini_file;
	PPID   dbid = 0;
	SString dbname, dbentry, dbpath;
	PPDbEntrySet2 dbes;
	THROW(CheckDialogPtr(&dlg));
	FileBrowseCtrlGroup::Setup(dlg, CTLBRW_MAKENEWDB_PATH, CTL_MAKENEWDB_PATH, FBB_GROUP1, PPTXT_SELNEWBASEDIR, PPTXT_FILPAT_DDFBTR, FileBrowseCtrlGroup::fbcgfPath);
	dbes.ReadFromProfile(&ini_file);
	{
		SString temp_buf;
		const char * p_pattern = "new-database";
		long    _uc = 0;
		dbname = p_pattern;
		dbentry = p_pattern;
		int    found = 0;
		DbLoginBlock dblb;
		do {
			if(found) {
				_uc++;
				(dbname = p_pattern).CatChar('-').Cat(_uc);
				(dbentry = p_pattern).CatChar('-').Cat(_uc);
				found = 0;
			}
			for(uint i = 0; !found && i < dbes.GetCount(); i++) {
				if(dbes.GetByPos(i, &dblb)) {
					dblb.GetAttr(dblb.attrDbSymb, temp_buf);
					if(temp_buf.CmpNC(dbentry) == 0)
						found = 1;
					else {
						dblb.GetAttr(dblb.attrDbName, temp_buf);
						if(temp_buf.CmpNC(dbname) == 0)
							found = 1;
					}
				}
			}
		} while(found);
	}
	dlg->setCtrlString(CTL_MAKENEWDB_NAME, dbname);
	dlg->setCtrlString(CTL_MAKENEWDB_SYMBOL, dbentry);
	while(!valid_data && ExecView(dlg) == cmOK) {
		dlg->getCtrlData(CTL_MAKENEWDB_TYPE, &dbid);
		dlg->getCtrlString(CTL_MAKENEWDB_NAME, dbname);
		dbname.Strip();
		dlg->getCtrlString(CTL_MAKENEWDB_SYMBOL, dbentry);
		dbentry.Strip();
		dlg->getCtrlString(CTL_MAKENEWDB_PATH, dbpath);
		if((dbname.Empty() && dbentry.Empty()) || dbpath.Empty())
			PPMessage(mfInfo | mfCancel, PPINF_EMPTYFIELD);
		else {
			dbname.SetIfEmpty(dbentry);
			dbentry.SetIfEmpty(dbname);
			PPID   i;
			SString n, pn;
			for(i = 0; i < (int)dbes.GetCount(); i++) {
				DbLoginBlock blk;
				dbes.GetByPos(i, &blk);
				blk.GetAttr(DbLoginBlock::attrDbSymb, n);
				blk.GetAttr(DbLoginBlock::attrDbFriendlyName, pn);
				pn.SetIfEmpty(n);
				if(pn.NotEmptyS())
					if(pn.CmpNC(dbname) == 0) {
						PPMessage(mfInfo|mfCancel, PPINF_BDEXISTNAME);
						i = -1;
						break;
					}
					else if(n.CmpNC(dbentry) == 0) {
						PPMessage(mfInfo|mfCancel, PPINF_BDSYMBOLEXIST);
						i = -1;
						break;
					}
			}
			if(i >= 0) {
				i = 1;
				SPathStruc ps;
				ps.Split(dbpath.SetLastSlash());
				if(!(ps.Flags & SPathStruc::fDrv && ps.Flags & SPathStruc::fDir)) {
					PPMessage(mfInfo | mfCancel, PPINF_NEEDFULLPATH, dbpath);
					i = 0;
				}
				else if(isDir(dbpath)) {
					//
					// Проверка на то, что бы каталог назначения был пустым
					//
					SString src_file;
					SDirEntry sd_entry;
					i = 1;
					(src_file = dbpath).SetLastSlash().Cat("*.*");
					for(SDirec sd(src_file); sd.Next(&sd_entry) > 0;) {
						if(sd_entry.IsFile()) {
							PPMessage(mfInfo | mfCancel, PPINF_DIRNOTEMPTY, dbpath);
							i = 0;
							break;
						}
					}
					if(i)
						i = (PPMessage(mfConf | mfYesNo, PPCFM_EXISTDIR, dbpath) == cmYes);
				}
				else if(!createDir(dbpath)) {
					i = 0;
					PPError(PPERR_SLIB, dbpath);
					// PPMessage(mfInfo | mfCancel, PPINF_CANTMKDIR);
				}
				if(i) {
					if(dbid == 0) {
						valid_data = CreateByExample(dbpath);
						if(valid_data > 0) {
							dbname.Comma().Cat(dbpath.RmvLastSlash());
							ini_file.AppendParam("dbname", dbentry, dbname, 1);
							ok = 1;
						}
					}
					else {
						//valid_data = CreateEmpty(dbpath);
						//int SLAPI CreateEmpty(const char * pPath)
						{
							class PPCreateDatabaseSession : public PPThread {
							public:
								SLAPI PPCreateDatabaseSession(const char * pDbSymb) : PPThread(PPThread::kUnknown, 0, 0)
								{
									DbSymb = pDbSymb;
									InitStartupSignal();
								}
							private:
								void SLAPI Startup()
								{
									PPThread::Startup();
									SignalStartup();
								}
								virtual void Run()
								{
									char    secret[256];
									PPVersionInfo vi = DS.GetVersionInfo();
									THROW(vi.GetSecret(secret, sizeof(secret)));
									THROW(DS.Login(DbSymb, PPSession::P_EmptyBaseCreationLogin, secret));
									DS.Logout();
									CATCH
										PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
									ENDCATCH
									memzero(secret, sizeof(secret));
								}
								SString DbSymb;
							};

							int    ok = 1;
							SString empty_path;
							SString pack_path;

							PPWait(1);
							//
							// Вычисляем необходимый размер дискового пространства и сравниваем его с доступным
							//
							int64  disk_total = 0, disk_avail = 0;
							SFileUtil::GetDiskSpace(dbpath, &disk_total, &disk_avail);
							// *3/2 - коэффициент запаса
							if((calcNeededSize(1) * 3 / 2) > disk_avail) {
								DBErrCode = SDBERR_BU_NOFREESPACE;
								CALLEXCEPT_PP(PPERR_DBLIB);
							}
							/*
							{
								SArchive arc;
								PPGetFilePath(PPPATH_PACK, "empty.zip", pack_path);
								if(fileExists(pack_path)) {
									THROW_SL(arc.Open(SArchive::tZip, pack_path, SFile::mRead));
									{
										const int64 c = arc.GetEntriesCount();
										if(c > 0) {
											(empty_path = dbpath).SetLastSlash();
											for(int64 i = 0; i < c; i++) {
												THROW(arc.ExtractEntry(i, empty_path));
											}
										}
									}
									arc.Close();
								}
							}
							*/
							dbname.Comma().Cat(dbpath.RmvLastSlash());
							ini_file.AppendParam("dbname", dbentry, dbname, 1);
							{
								PPCreateDatabaseSession * p_sess = new PPCreateDatabaseSession(dbentry);
								p_sess->Start(1);
							}
							/*CATCH
								if(PPErrCode == PPERR_USERBREAK) {
									RemoveDir(dbpath);
								}
								valid_data = PPErrorZ();
							ENDCATCH*/
							PPWait(0);
						}
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
	#undef FBB_GROUP1
}
