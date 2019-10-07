// PPDBMAKE.CPP
// Copyright (c) Osolotkin A.V, Sobolev A. 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2013, 2016, 2017, 2018, 2019
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
		const StrAssocArray::Item item = tbl_list.Get(j);
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
	PPObjBill * p_bobj = BillObj;
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
		const StrAssocArray::Item item = tbl_list.Get(j);
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
				THROW(p_dst_tbl->allocOwnBuffer(SKILOBYTE(60))); // @v10.4.1 4096-->SKILOBYTE(1024) // @v10.4.7 @fix SKILOBYTE(1024)-->SKILOBYTE(60)
				if(fileExists(src_path)) {
					THROW_MEM(p_src_tbl = new DBTable(tbl_name));
					THROW(p_src_tbl->allocOwnBuffer(SKILOBYTE(60))); // @v10.4.1 4096-->SKILOBYTE(1024) // @v10.4.7 @fix SKILOBYTE(1024)-->SKILOBYTE(60)
					if(isNeededFile(tbl_name)) {
						RECORDNUMBER rn, i = 0;
						PPTransaction tra(1);
						THROW(tra);
						p_src_tbl->getNumRecs(&rn);
						if(p_src_tbl->step(spFirst)) {
							const int is_assoc = tbl_name.IsEqiAscii("objassoc");
							const int is_ref = tbl_name.IsEqiAscii("reference2");
							do {
								int   ins_rec = 0;
								p_dst_tbl->clearDataBuf();
								p_dst_tbl->copyBufLobFrom(p_src_tbl->getDataBuf(), p_src_tbl->getRetBufLen());
								if(is_assoc) {
									ObjAssocTbl::Rec * p_rec = static_cast<ObjAssocTbl::Rec *>(p_src_tbl->getDataBuf());
									if(!oneof10(p_rec->AsscType, PPASS_BILLSET, PPASS_PAYMBILLPOOL, PPASS_OPBILLPOOL,
										PPASS_CSESSBILLPOOL, PPASS_TSESSBILLPOOL, PPASS_CSDBILLPOOL, PPASS_TSDBILLPOOL,
										PPASS_PRJBILLPOOL, PPASS_PRJPHASEBILLPOOL, PPASS_TODOBILLPOOL))
										ins_rec = 1;
								}
								else {
									ins_rec = 1;
									// @v10.2.0 {
									if(is_ref) {
                                        Reference2Tbl::Rec * p_rec = static_cast<Reference2Tbl::Rec *>(p_src_tbl->getDataBuf());
                                        if(p_rec->ObjType == PPOBJ_CASHNODE) {
											PPCashNode2 * p_cn = reinterpret_cast<PPCashNode2 *>(p_rec);
											p_cn->CurRestBillID = 0;
											p_cn->CurSessID = 0;
											p_cn->CurDate = ZERODATE;
                                        }
									}
									// } @v10.2.0
								}
								if(ins_rec && !p_dst_tbl->insertRec()) {
									PPSaveErrContext();
									SString msg_buf, rec_txt_buf;
									msg_buf.Cat(p_dst_tbl->GetName()).CatDiv('-', 1);
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
								p_src_tbl->clearDataBuf();
								PPWaitPercent(i++, rn, tbl_name);
								THROW(PPCheckUserBreak());
							} while(p_src_tbl->step(spNext));
						}
						THROW(tra.Commit());
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
		{
			PPTransaction tra(1);
			THROW(tra);
			if(src_tbl.step(spFirst)) {
				ReceiptCore * p_rc = &p_bobj->trfr->Rcpt;
				ObjTagCore dest_ot((path = pPath).SetLastSlash().Cat("objtag.btr"));
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
							// Перенос тегов лота с исходной базы в создаваемую.
							//
							ObjTagList lot_tag_list;
							if(p_bobj->GetTagListByLot(rcpt_rec.ID, 0, &lot_tag_list) > 0) {
								THROW(dest_ot.PutList(PPOBJ_LOT, rcpt_rec.ID, &lot_tag_list, 0));
							}
						}
					}
					PPWaitPercent(i++, rn, p_rc->GetTableName());
					THROW(PPCheckUserBreak());
				} while(src_tbl.step(spNext));
			}
			THROW(tra.Commit());
		}
	}
	{
		long   val2 = 0;
		Reference2Tbl dest_tbl_ref((dst_path = pPath).SetLastSlash().Cat("ref2.btr"));
		Reference2Tbl::Key0 k0;
		MEMSZERO(k0);
		k0.ObjType = PPOBJ_UNASSIGNED;
		k0.ObjID = 1L;
		{
			PPTransaction tra(1);
			THROW(tra);
			if(dest_tbl_ref.searchForUpdate(0, &k0, spEq)) {
				val2 = dest_tbl_ref.data.Val2;
				THROW_DB(dest_tbl_ref.deleteRec()); // @sfu
			}
			if(val2) {
				ArticleTbl dest_tbl_ar((dst_path = pPath).SetLastSlash().Cat("article.btr"));
				THROW(RemoveByID(&dest_tbl_ar, val2, 0));
			}
			THROW(tra.Commit());
		}
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

struct MakeDatabaseParam {
	MakeDatabaseParam() : How(howBySample), AutoPosNodeHostID(0)
	{
	}
	enum {
		howBySample    = 1,
		howEmpty       = 2,
		howAutoPosNode = 3
	};
	long   How;
	PPID   AutoPosNodeHostID;
	SString DbName;
	SString DbSymb;
	SString Path;
};

class MakeDatabaseParamDialog : public TDialog {
public:
	enum {
		ctlgroupFbb = 1
	};
	MakeDatabaseParamDialog() : TDialog(DLG_MAKENEWDB)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_MAKENEWDB_PATH, CTL_MAKENEWDB_PATH, ctlgroupFbb, PPTXT_SELNEWBASEDIR, PPTXT_FILPAT_DDFBTR, FileBrowseCtrlGroup::fbcgfPath);
	}
	int    setDTS(const MakeDatabaseParam * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		AddClusterAssocDef(CTL_MAKENEWDB_TYPE, 0, Data.howBySample);
		AddClusterAssoc(CTL_MAKENEWDB_TYPE, 1, Data.howEmpty);
		AddClusterAssoc(CTL_MAKENEWDB_TYPE, 2, Data.howAutoPosNode);
		SetClusterData(CTL_MAKENEWDB_TYPE, Data.How);
		setCtrlString(CTL_MAKENEWDB_NAME, Data.DbName);
		setCtrlString(CTL_MAKENEWDB_SYMBOL, Data.DbSymb);
		{
			PPObjCashNode::SelFilt filt;
			filt.OnlyGroups = -1;
			filt.SyncGroup = 2; // async only 
			SetupPPObjCombo(this, CTLSEL_MAKENEWDB_POSHOST, PPOBJ_CASHNODE, Data.AutoPosNodeHostID, 0, &filt);
		}
		disableCtrl(CTLSEL_MAKENEWDB_POSHOST, Data.How != Data.howAutoPosNode);
		return ok;
	}
	int    getDTS(MakeDatabaseParam * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_MAKENEWDB_TYPE, &Data.How);
		getCtrlString(sel = CTL_MAKENEWDB_NAME, Data.DbName);
		THROW_PP(Data.DbName.NotEmptyS(), PPERR_USERINPUT);
		getCtrlString(sel = CTL_MAKENEWDB_SYMBOL, Data.DbSymb);
		THROW_PP(Data.DbSymb.NotEmptyS(), PPERR_USERINPUT);
		getCtrlString(CTL_MAKENEWDB_PATH, Data.Path);
		THROW_PP(Data.Path.NotEmptyS(), PPERR_USERINPUT);
		getCtrlData(CTLSEL_MAKENEWDB_POSHOST, &Data.AutoPosNodeHostID);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_MAKENEWDB_TYPE)) {
			GetClusterData(CTL_MAKENEWDB_TYPE, &Data.How);
			disableCtrl(CTLSEL_MAKENEWDB_POSHOST, Data.How != Data.howAutoPosNode);
			clearEvent(event);
		}
	}
	MakeDatabaseParam Data;
};

int SLAPI MakeDatabase()
{
	int    ok = -1;
	PPIniFile ini_file;
	MakeDatabaseParam param;
	PPDbEntrySet2 dbes;
	dbes.ReadFromProfile(&ini_file);
	{
		SString temp_buf;
		const char * p_pattern = "new-database";
		long    _uc = 0;
		param.DbName = p_pattern;
		param.DbSymb = p_pattern;
		int    found = 0;
		DbLoginBlock dblb;
		do {
			if(found) {
				_uc++;
				(param.DbName = p_pattern).CatChar('-').Cat(_uc);
				(param.DbSymb = p_pattern).CatChar('-').Cat(_uc);
				found = 0;
			}
			for(uint i = 0; !found && i < dbes.GetCount(); i++) {
				if(dbes.GetByPos(i, &dblb)) {
					dblb.GetAttr(dblb.attrDbSymb, temp_buf);
					if(temp_buf.CmpNC(param.DbSymb) == 0)
						found = 1;
					else {
						dblb.GetAttr(dblb.attrDbName, temp_buf);
						if(temp_buf.CmpNC(param.DbName) == 0)
							found = 1;
					}
				}
			}
		} while(found);
	}
	if(PPDialogProcBody <MakeDatabaseParamDialog, MakeDatabaseParam>(&param) > 0) {
		param.DbName.SetIfEmpty(param.DbSymb);
		param.DbSymb.SetIfEmpty(param.DbName);
		PPID   i;
		SString n, pn;
		for(i = 0; i < static_cast<int>(dbes.GetCount()); i++) {
			DbLoginBlock blk;
			dbes.GetByPos(i, &blk);
			blk.GetAttr(DbLoginBlock::attrDbSymb, n);
			blk.GetAttr(DbLoginBlock::attrDbFriendlyName, pn);
			pn.SetIfEmpty(n);
			if(pn.NotEmptyS())
				if(pn.CmpNC(param.DbName) == 0) {
					PPMessage(mfInfo|mfCancel, PPINF_BDEXISTNAME);
					i = -1;
					break;
				}
				else if(n.CmpNC(param.DbSymb) == 0) {
					PPMessage(mfInfo|mfCancel, PPINF_BDSYMBOLEXIST);
					i = -1;
					break;
				}
		}
		if(i >= 0) {
			i = 1;
			SPathStruc ps(param.Path.SetLastSlash());
			if(!(ps.Flags & SPathStruc::fDrv && ps.Flags & SPathStruc::fDir)) {
				PPMessage(mfInfo | mfCancel, PPINF_NEEDFULLPATH, param.Path);
				i = 0;
			}
			else if(IsDirectory(param.Path)) {
				//
				// Проверка на то, что бы каталог назначения был пустым
				//
				SString src_file;
				SDirEntry sd_entry;
				i = 1;
				(src_file = param.Path).SetLastSlash().Cat("*.*");
				for(SDirec sd(src_file); sd.Next(&sd_entry) > 0;) {
					if(sd_entry.IsFile()) {
						PPMessage(mfInfo | mfCancel, PPINF_DIRNOTEMPTY, param.Path);
						i = 0;
						break;
					}
				}
				if(i)
					i = (PPMessage(mfConf | mfYesNo, PPCFM_EXISTDIR, param.Path) == cmYes);
			}
			else if(!createDir(param.Path)) {
				i = 0;
				PPError(PPERR_SLIB, param.Path);
				// PPMessage(mfInfo | mfCancel, PPINF_CANTMKDIR);
			}
			if(i) {
				class PPCreateDatabaseSession : public PPThread {
				public:
					struct Param {
						Param() : Action(acnNone), NewPosNodeN(0)
						{
						}
						enum {
							acnNone = 0,
							acnCreatePosNode
						};
						int    Action;
						long   NewPosNodeN;
						S_GUID PosHostGuid;
						S_GUID NewPosNodeGuid;
						SString DbSymb;
					};
					explicit SLAPI PPCreateDatabaseSession(const Param & rParam) : PPThread(PPThread::kUnknown, 0, 0), P(rParam)
					{
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
						SString temp_buf;
						PPVersionInfo vi = DS.GetVersionInfo();
						THROW(vi.GetSecret(secret, sizeof(secret)));
						THROW(DS.Login(P.DbSymb, PPSession::P_EmptyBaseCreationLogin, secret));
						if(P.Action == P.acnCreatePosNode) {
							PPID   cn_id = 0;
							PPObjCashNode cn_obj;
							PPSyncCashNode cn_pack;
							STRNSCPY(cn_pack.Name, "POS-node");
							cn_pack.CashType = PPCMT_SYNCSYM;
							cn_pack.Flags |= CASHF_SYNC;
							if(!P.PosHostGuid.IsZero()) {
								ObjTagItem tag_item;
								if(tag_item.SetGuid(PPTAG_POSNODE_HOSTUUID, &P.PosHostGuid))
									cn_pack.TagL.PutItem(PPTAG_POSNODE_HOSTUUID, &tag_item);
							}
							if(P.NewPosNodeN > 0) {
								temp_buf.Z().Cat(P.NewPosNodeN);
								STRNSCPY(cn_pack.Symb, temp_buf);
							}
							if(!P.NewPosNodeGuid.IsZero()) {
								ObjTagItem tag_item;
								if(tag_item.SetGuid(PPTAG_POSNODE_UUID, &P.NewPosNodeGuid))
									cn_pack.TagL.PutItem(PPTAG_POSNODE_UUID, &tag_item);
							}
							cn_obj.Put(&cn_id, &cn_pack, 1);
						}
						DS.Logout();
						CATCH
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
						ENDCATCH
						memzero(secret, sizeof(secret));
					}
					Param  P;
				};
				SString empty_path;
				SString pack_path;
				if(param.How == param.howBySample) {
					if(CreateByExample(param.Path) > 0) {
						param.DbName.Comma().Cat(param.Path.RmvLastSlash());
						ini_file.AppendParam("dbname", param.DbSymb, param.DbName, 1);
						ok = 1;
					}
				}
				else if(param.How == param.howEmpty) {
					ok = 1;
					PPWait(1);
					//
					// Вычисляем необходимый размер дискового пространства и сравниваем его с доступным
					//
					int64  disk_total = 0, disk_avail = 0;
					SFileUtil::GetDiskSpace(param.Path, &disk_total, &disk_avail);
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
					param.DbName.Comma().Cat(param.Path.RmvLastSlash());
					ini_file.AppendParam("dbname", param.DbSymb, param.DbName, 1);
					{
						PPCreateDatabaseSession::Param sess_param;
						sess_param.DbSymb = param.DbSymb;
						if(param.How == param.howAutoPosNode) {
							PPObjCashNode cn_obj;
							PPAsyncCashNode cn_pack;
							if(cn_obj.GetAsync(param.AutoPosNodeHostID, &cn_pack) > 0) {
								const ObjTagItem * p_tag_item = cn_pack.TagL.GetItem(PPTAG_POSNODE_UUID);
								CALLPTRMEMB(p_tag_item, GetGuid(&sess_param.PosHostGuid));
								sess_param.NewPosNodeGuid.Generate();
								long  max_child_no = 0;
								for(uint i = 0; i < cn_pack.ApnCorrList.getCount(); i++) {
									const PPGenCashNode::PosIdentEntry * p_entry = cn_pack.ApnCorrList.at(i);
									if(p_entry) {
										SETMAX(max_child_no, p_entry->N);
									}
								}
								PPGenCashNode::PosIdentEntry * p_child_pos_entry = cn_pack.ApnCorrList.CreateNewItem();
								if(p_child_pos_entry) {
									p_child_pos_entry->Uuid = sess_param.NewPosNodeGuid;
									p_child_pos_entry->N = max_child_no+1;
									sess_param.NewPosNodeN = p_child_pos_entry->N;
									PPID  temp_cn_id = param.AutoPosNodeHostID;
									THROW(cn_obj.Put(&temp_cn_id, &cn_pack, 1));
								}
							}
						}
						PPCreateDatabaseSession * p_sess = new PPCreateDatabaseSession(sess_param);
						p_sess->Start(1);
					}
					PPWait(0);
				}
				else if(param.How == param.howAutoPosNode) {
					ok = 1;
					PPWait(1);
					//
					// Вычисляем необходимый размер дискового пространства и сравниваем его с доступным
					//
					int64  disk_total = 0, disk_avail = 0;
					SFileUtil::GetDiskSpace(param.Path, &disk_total, &disk_avail);
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
					param.DbName.Comma().Cat(param.Path.RmvLastSlash());
					ini_file.AppendParam("dbname", param.DbSymb, param.DbName, 1);
					{
						PPCreateDatabaseSession::Param sess_param;
						sess_param.Action = sess_param.acnCreatePosNode;
						sess_param.DbSymb = param.DbSymb;
						if(param.How == param.howAutoPosNode) {
							PPObjCashNode cn_obj;
							PPAsyncCashNode cn_pack;
							if(cn_obj.GetAsync(param.AutoPosNodeHostID, &cn_pack) > 0) {
								const ObjTagItem * p_tag_item = cn_pack.TagL.GetItem(PPTAG_POSNODE_UUID);
								CALLPTRMEMB(p_tag_item, GetGuid(&sess_param.PosHostGuid));
								sess_param.NewPosNodeGuid.Generate();
								long  max_child_no = 0;
								for(uint i = 0; i < cn_pack.ApnCorrList.getCount(); i++) {
									const PPGenCashNode::PosIdentEntry * p_entry = cn_pack.ApnCorrList.at(i);
									if(p_entry) {
										SETMAX(max_child_no, p_entry->N);
									}
								}
								PPGenCashNode::PosIdentEntry * p_child_pos_entry = cn_pack.ApnCorrList.CreateNewItem();
								if(p_child_pos_entry) {
									p_child_pos_entry->Uuid = sess_param.NewPosNodeGuid;
									p_child_pos_entry->N = max_child_no+1;
									sess_param.NewPosNodeN = p_child_pos_entry->N;
									PPID  temp_cn_id = param.AutoPosNodeHostID;
									THROW(cn_obj.Put(&temp_cn_id, &cn_pack, 1));
								}
							}
						}
						PPCreateDatabaseSession * p_sess = new PPCreateDatabaseSession(sess_param);
						p_sess->Start(1);
					}
					PPWait(0);
				}
			}
		}
	}
	CATCHZOKPPERR
	//delete dlg;
	return ok;
}
#undef FBB_GROUP1
