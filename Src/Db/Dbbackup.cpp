// DBBACKUP.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <db.h>

int CallbackCompress(long, long, const char *, int);
/*
	Формат файла backup.dat
	Текстовый файл. Данные об одной копии записываются в одну строку.
	Разделители полей - запятая (',').

	copy_set,
	copy_id,
	dest_path,
	dest_subdir,
	date (dd/mm/yyyy),
	time (hh:mm:ss),
	copy_format (0 - raw copy),
	src_size (bytes),
	dest_size (bytes),
	check_sum
*/
//
//
//
#define DEFAULT_SPACE_SAFETY_FACTOR 1200

SLAPI BCopySet::BCopySet(const char * pName) : TSCollection <BCopyData> (), Name(pName)
{
}

static IMPL_CMPFUNC(BCopyData_Dt, i1, i2) { return cmp(((BCopyData *)i1)->Dtm, ((BCopyData *)i2)->Dtm); }

static IMPL_CMPFUNC(BCopyData_DtDesc, i1, i2)
{
	int    r = CMPFUNC(BCopyData_Dt, i1, i2);
	return r ? -r : 0;
}

int SLAPI BCopySet::Sort(BCopySet::Order ord)
{
	if(ord == ordByDate)
		sort(PTR_CMPFUNC(BCopyData_Dt));
	else if(ord == ordByDateDesc)
		sort(PTR_CMPFUNC(BCopyData_DtDesc));
	return 1;
}
//
//
//
const char * BackupInfoFile = "BACKUP.INF";

SLAPI DBBackup::InfoFile::InfoFile(DbProvider * pDb)
{
	if(pDb) {
		SString data_path;
		pDb->GetDataPath(data_path);
		MakeFileName(data_path, BackupInfoFile, FileName, sizeof(FileName));
	}
	else
		FileName[0] = 0;
	Stream = 0;
}

SLAPI DBBackup::InfoFile::~InfoFile()
{
	CloseStream();
}

int SLAPI DBBackup::InfoFile::MakeFileName(const char * pPath, const char * pFileName, char * pBuf, size_t bufLen)
{
	SString temp;
	if(pPath)
		(temp = pPath).SetLastSlash();
	else {
		SPathStruc ps(FileName);
		ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, temp);
		temp.SetLastSlash();
	}
	temp.Cat(pFileName).CopyTo(pBuf, bufLen);
	return 1;
}

int SLAPI DBBackup::InfoFile::OpenStream(int readOnly)
{
	char   mode[16];
	CloseStream();
	if(!fileExists(FileName)) {
		mode[0] = 'w';
		mode[1] = 0;
		Stream = fopen(FileName, mode);
		if(Stream == 0)
			return (DBErrCode = SDBERR_FCRFAULT, 0);
		CloseStream();
	}
	mode[0] = 'r';
	if(readOnly)
		mode[1] = 0;
	else {
		mode[1] = '+';
		mode[2] = 0;
	}
	Stream = fopen(FileName, mode);
	return Stream ? 1 : (DBErrCode = SDBERR_FOPENFAULT, 0);
}

int SLAPI DBBackup::InfoFile::CloseStream()
{
	SFile::ZClose(&Stream);
	return 1;
}

int SLAPI DBBackup::InfoFile::ReadSet(BCopySet * set)
{
	int    ok = 1;
	BCopyData bc_data;
	if(OpenStream(1)) {
		while(ok && ReadRecord(Stream, &bc_data) > 0)
			if(set->Name.Empty() || bc_data.Set.CmpNC(set->Name) == 0)
				set->insert(new BCopyData(bc_data));
		CloseStream();
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DBBackup::InfoFile::ReadItem(long copyID, BCopyData * pData)
{
	int    ok = -1;
	BCopyData bc_data;
	if(OpenStream(1)) {
		while(ok < 0 && ReadRecord(Stream, &bc_data) > 0) {
			if(bc_data.ID == copyID) {
				ASSIGN_PTR(pData, bc_data);
				ok = 1;
				break;
			}
		}
		CloseStream();
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DBBackup::InfoFile::AddRecord(BCopyData * data)
{
	int    ok = 1;
	long   max_id = 1;
	BCopyData temp_rec;
	if(OpenStream(0)) {
		while(ReadRecord(Stream, &temp_rec) > 0)
			if(temp_rec.ID >= max_id)
				max_id = temp_rec.ID + 1;
		data->ID = max_id;
		if(WriteRecord(Stream, data))
			CloseStream();
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DBBackup::InfoFile::RemoveRecord(const char * pSet, long id)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	long   i;
	char   temp_name[MAXPATH];
	char   nam[32];
	FILE * temp_stream = 0;
	BCopyData data;
	//
	// @todo use MakeTempFileName() {
	//
	for(i = 1; i < 100000L; i++) {
		sprintf(nam, "BI%06ld.TMP", i);
		MakeFileName(0, nam, temp_name, sizeof(temp_name));
		if(!fileExists(temp_name))
			break;
	}
	// }
	THROW(OpenStream(1));
	THROW_V(temp_stream = fopen(temp_name, "w"), SDBERR_FCRFAULT);
	while(ReadRecord(Stream, &data) > 0)
		if(data.Set.CmpNC(pSet) != 0 || data.ID != id) {
			THROW(WriteRecord(temp_stream, &data));
		}
	SFile::ZClose(&temp_stream);
	CloseStream();
	THROW_V(SFile::Remove(FileName), SDBERR_SLIB);
	THROW_V(SFile::Rename(temp_name, FileName), SDBERR_SLIB);
	CATCH
		ok = 0;
		SFile::ZClose(&temp_stream);
		CloseStream();
	ENDCATCH
	return ok;
}

int SLAPI DBBackup::InfoFile::WriteRecord(FILE * stream, const BCopyData * pData)
{
	int    ok = 1;
	int    copy_format = BIN(pData->Flags & BCOPYDF_USECOMPRESS);
	if(stream) {
		/*
		fprintf(stream, "%s,%ld,%s,%s,%s,%s,%d,%lu,%lu,%lu\n",
			data->Set, data->ID, data->CopyPath, data->SubDir, sdt, stm,
			copy_format, data->SrcSize, data->DestSize, data->CheckSum);
		*/
		SString line_buf;
		line_buf.
			Cat(pData->Set).Comma().
			Cat(pData->ID).Comma().
			Cat(pData->CopyPath).Comma().
			Cat(pData->SubDir).Comma().
			Cat(pData->Dtm.d, DATF_DMY|DATF_CENTURY).Comma().
			Cat(pData->Dtm.t, TIMF_HMS).Comma().
			Cat(copy_format).Comma().
			Cat(pData->SrcSize).Comma().
			Cat(pData->DestSize).Comma().
			Cat(pData->CheckSum).CR();
		fputs(line_buf, stream);
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DBBackup::InfoFile::ReadRecord(FILE * stream, BCopyData * pData)
{
	int    ok = -1;
	char   buf[1024];
	memzero(pData, sizeof(*pData));
	while(fgets(buf, sizeof(buf), stream)) {
		if(*strip(chomp(buf))) {
			if(pData) {
				uint   pos = 0, i = 0;
				SString str;
				StringSet ss(',', buf);
				while(ss.get(&pos, str)) {
					str.Strip();
					switch(i++) {
						case 0: pData->Set = str; break;
						case 1: pData->ID = str.ToLong(); break;
						case 2: pData->CopyPath = str; break;
						case 3: pData->SubDir = str; break;
						case 4: strtodate(str, DATF_DMY, &pData->Dtm.d); break;
						case 5: strtotime(str, TIMF_HMS, &pData->Dtm.t); break;
						case 6:
							SETFLAG(pData->Flags, BCOPYDF_USECOMPRESS, str.ToLong());
							break; // copy format
						case 7: pData->SrcSize  = _atoi64(str); break;
						case 8: pData->DestSize = _atoi64(str); break;
						case 9: pData->CheckSum = strtoul(str, 0, 10); break;
					}
				}
			}
			ok = 1;
			break;
		}
	}
	return ok;
}
//
//
//
SLAPI DBBackup::DBBackup() : P_Db(0), InfoF(0), AbortProcessFlag(0), SpaceSafetyFactor(DEFAULT_SPACE_SAFETY_FACTOR),
	TotalCopySize(0), TotalCopyReady(0)
{
}

SLAPI DBBackup::~DBBackup()
{
}

int SLAPI DBBackup::CBP_CopyProcess(const char *, const char *, int64, int64, int64, int64)
{
	return SPRGRS_CONTINUE;
}

uint SLAPI DBBackup::GetSpaceSafetyFactor()
{
	return SpaceSafetyFactor;
}

void SLAPI DBBackup::SetSpaceSafetyFactor(uint v)
{
	SpaceSafetyFactor = v;
}

int SLAPI DBBackup::SetDictionary(DbProvider * pDb)
{
	P_Db = pDb;
	ZDELETE(InfoF);
	if(P_Db) {
		InfoF = new DBBackup::InfoFile(P_Db);
		if(InfoF == 0) {
			P_Db = 0;
			return (DBErrCode = SDBERR_NOMEM, 0);
		}
	}
	return 1;
}

int SLAPI DBBackup::GetCopySet(BCopySet * pSet)
{
	return P_Db ? InfoF->ReadSet(pSet) : (DBErrCode = SDBERR_BU_DICTNOPEN, 0);
}

int SLAPI DBBackup::GetCopyData(long copyID, BCopyData * pData)
{
	return P_Db ? InfoF->ReadItem(copyID, pData) : (DBErrCode = SDBERR_BU_DICTNOPEN, 0);
}

int SLAPI DBBackup::CheckAvailableDiskSpace(const char * pPath, int64 size)
{
	int64 total, free_space;
	SFileUtil::GetDiskSpace(pPath, &total, &free_space);
	return (free_space > ((size * SpaceSafetyFactor) / 1000L)) ? 1 : (DBErrCode = SDBERR_BU_NOFREESPACE, 0);
}

static void LogMessage(BackupLogFunc fnLog, int recId, const char * pInfo, long initParam)
{
	if(fnLog)
		fnLog(recId, pInfo, initParam);
}

int SLAPI DBBackup::CheckCopy(BCopyData * pData, const CopyParams & rCP, BackupLogFunc fnLog, long initParam)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	int    use_compression = BIN(pData && (pData->Flags & BCOPYDF_USECOMPRESS));
	BTBLID i = 0;
	SString path, spart;
	DbTableStat ts;
	StrAssocArray tbl_list;
	CopyParams cp;
	cp.TotalSize = 0;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	P_Db->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.Get(j);
		if(P_Db->GetTableInfo(item.Id, &ts) > 0 && !(ts.Flags & XTF_DICT) && ts.Location.NotEmpty()) {
			TablePartsEnum tpe(0);
			path = ts.Location;
			ulong  sz = 0;
			for(tpe.Init(P_Db->MakeFileName_(ts.TblName, path)); tpe.Next(spart) > 0;) {
				if(fileExists(spart)) {
					SFileUtil::Stat stat;
					if(SFileUtil::GetStat(spart, &stat)) {
						cp.TotalSize += stat.Size;
						// @v9.6.4 char * s;
						// @v9.6.4 THROW_V(s = newStr(spart), SDBERR_NOMEM);
						// @v9.6.4 THROW_V(cp.FileList.insert(s), SDBERR_SLIB);
						THROW_V(cp.SsFiles.add(spart), SDBERR_SLIB); // @v9.6.4
					}
					else {
						LogMessage(fnLog, BACKUPLOG_ERR_GETFILEPARAM, spart, initParam);
						CALLEXCEPT();
					}
				}
			}
		}
	}
	// check total size criteria
	if(!use_compression) {
		long double diff = (long double)(rCP.TotalSize - cp.TotalSize) * 100;
		diff = (diff < 0) ? -diff : diff;
		THROW_V(cp.TotalSize && (diff / (long double)cp.TotalSize) <= 30, SDBERR_BU_COPYINVALID);
	}
	// check files count criteria
	{
		const uint pcp_ssf_count = rCP.SsFiles.getCount();
		const uint cp_ssf_count = cp.SsFiles.getCount();
		double diff = (double)labs((long)pcp_ssf_count - (long)cp_ssf_count) * 100;
		THROW_V(cp_ssf_count && (diff / (double)cp_ssf_count) <= 30, SDBERR_BU_COPYINVALID);
	}
	CATCHZOK
	return ok;
}

int SLAPI DBBackup::Backup(BCopyData * pData, BackupLogFunc fnLog, long initParam)
{
	// DbProvider Implement_Open
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	int    use_compression    = BIN(pData->Flags & BCOPYDF_USECOMPRESS);
	int    use_copycontinouos = BIN(pData->Flags & BCOPYDF_USECOPYCONT);
	int    do_release_cont = BIN(pData->Flags & BCOPYDF_RELEASECONT);
	LDATETIME cur_dtm = getcurdatetime_();
	DbTableStat ts;
	StrAssocArray tbl_list;
	SString path, spart;
	SStrCollection file_list2;
	StringSet copycont_filelist(",");
	CopyParams cp;
	cp.TotalSize = 0;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	P_Db->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.Get(j);
		if(P_Db->GetTableInfo(item.Id, &ts) > 0 && !(ts.Flags & XTF_DICT) && ts.Location.NotEmpty()) {
			TablePartsEnum tpe(0);
			path = ts.Location;
			if(tpe.Init(P_Db->MakeFileName_(ts.TblName, path))) {
				for(int is_first = 1; tpe.Next(spart) > 0; is_first = 0) {
					if(fileExists(spart)) {
						SFileUtil::Stat stat;
						if(SFileUtil::GetStat(spart, &stat)) {
							cp.TotalSize += stat.Size;
							// @v9.6.4 char * s;
							// @v9.6.4 THROW_V(s = newStr(spart), SDBERR_NOMEM);
							// @v9.6.4 THROW_V(cp.FileList.insert(s), SDBERR_SLIB);
							THROW_V(cp.SsFiles.add(spart), SDBERR_SLIB); // @v9.6.4
							if(is_first) {
								copycont_filelist.add(spart);
								file_list2.insert(newStr(spart));
							}
						}
						else {
							LogMessage(fnLog, BACKUPLOG_ERR_GETFILEPARAM, spart, initParam);
							CALLEXCEPT();
						}
					}
				}
			}
		}
	}
	if(do_release_cont) {
		// THROW_V(Btrieve::RemoveContinuous(copycont_filelist.getBuf()), SDBERR_BTRIEVE);
		SString msg_buf;
		for(uint j = 0; j < tbl_list.getCount(); j++) {
			const StrAssocArray::Item item = tbl_list.Get(j);
			if(P_Db->GetTableInfo(item.Id, &ts) > 0 && !(ts.Flags & XTF_DICT) && ts.Location.NotEmpty()) {
				TablePartsEnum tpe(0);
				path = ts.Location;
				if(tpe.Init(P_Db->MakeFileName_(ts.TblName, path))) {
					for(int is_first = 1; tpe.Next(spart) > 0; is_first = 0) {
						if(fileExists(spart)) {
							SFileUtil::Stat stat;
							if(SFileUtil::GetStat(spart, &stat)) {
								cp.TotalSize += stat.Size;
								// @v9.6.4 char * s;
								// @v9.6.4 THROW_V(s = newStr(spart), SDBERR_NOMEM);
								// @v9.6.4 THROW_V(cp.FileList.insert(s), SDBERR_SLIB);
								THROW_V(cp.SsFiles.add(spart), SDBERR_SLIB); // @v9.6.4
								if(is_first) {
									DBTable _tbl(item.Txt);
									if(_tbl.IsOpened()) {
										int r2 = Btrieve::RemoveContinuous(_tbl.GetFileName());
										if(r2) {
											(msg_buf = "Remove continuous").CatDiv(':', 2).Cat("OK for file").Space().Cat(_tbl.GetFileName());
											SLS.LogMessage(0, msg_buf, 0);
										}
										else {
											(msg_buf = "Remove continuous").CatDiv(':', 2).Cat("error removing continuous mode for file").Space().Cat(_tbl.GetFileName());
											SLS.LogMessage(0, msg_buf, 0);
										}
									}
									else {
										(msg_buf = "Remove continuous").CatDiv(':', 2).Cat("error opening file").Space().Cat(_tbl.GetFileName());
										SLS.LogMessage(0, msg_buf, 0);
									}
								}
							}
							else {
								LogMessage(fnLog, BACKUPLOG_ERR_GETFILEPARAM, spart, initParam);
								CALLEXCEPT();
							}
						}
					}
				}
			}
		}
	}
	else {
		THROW(MakeCopyPath(pData, cp.Path));
		cp.TempPath = pData->TempPath;
		if(pData->BssFactor > 0)
			SetSpaceSafetyFactor((uint)pData->BssFactor);
		if(use_compression || CheckAvailableDiskSpace(cp.Path, cp.TotalSize)) {
			SString data_path;
			pData->SrcSize = cp.TotalSize;
			if(use_copycontinouos)
				THROW_V(Btrieve::AddContinuous(copycont_filelist.getBuf()), SDBERR_BTRIEVE); // @!
			ok = DoCopy(&cp, use_compression, fnLog, initParam);
			if(use_copycontinouos)
				THROW_V(Btrieve::RemoveContinuous(copycont_filelist.getBuf()), SDBERR_BTRIEVE);
			THROW(ok);
			P_Db->GetDataPath(data_path);
			THROW(CopyLinkFiles(data_path, cp.Path, fnLog, initParam));
			pData->DestSize = cp.TotalSize;
			pData->Dtm = cur_dtm;
			THROW(InfoF->AddRecord(pData));
		}
		else {
			SFile::Remove(cp.Path.RmvLastSlash());
			CALLEXCEPT();
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI DBBackup::GetCopyParams(const BCopyData * data, DBBackup::CopyParams * params)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	SString copy_path, wildcard, file_name;
	SDirEntry dir_entry;
	SDirec * direc = 0;
	params->TotalSize = 0;
	params->CheckSum  = 0;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	(copy_path = data->CopyPath).Strip().SetLastSlash().Cat(data->SubDir).Strip();
	DBS.SetAddedMsgString(copy_path);
	THROW_V(::access(copy_path.RmvLastSlash(), 0) == 0, SDBERR_BU_NOCOPYPATH);
	(wildcard = copy_path).SetLastSlash().Cat("*.*");
	direc = new SDirec(wildcard);
	for(; direc->Next(&dir_entry) > 0;)
		if(!(dir_entry.Attr & 0x10)) {
			(file_name = copy_path).SetLastSlash().Cat(dir_entry.FileName);
			SFileUtil::Stat stat;
			if(SFileUtil::GetStat(file_name, &stat)) {
				params->TotalSize += stat.Size;
				// @v9.6.4 char * s = newStr(file_name);
				// @v9.6.4 THROW_V(s, SDBERR_NOMEM);
				// @v9.6.4 THROW_V(params->FileList.insert(s), SDBERR_SLIB);
				THROW_V(params->SsFiles.add(file_name), SDBERR_SLIB); // @v9.6.4
			}
		}
	params->Path = copy_path;
	if(data->BssFactor > 0)
		SetSpaceSafetyFactor((uint)data->BssFactor);
	CATCHZOK
	ZDELETE(direc);
	return ok;
}

int SLAPI DBBackup::RemoveDatabase(int safe)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	DbTableStat ts;
	StrAssocArray tbl_list;
	SString path, spart;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	P_Db->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.Get(j);
		if(P_Db->GetTableInfo(item.Id, &ts) > 0 && !(ts.Flags & XTF_DICT) && ts.Location.NotEmpty()) {
			int    first = 0;
			TablePartsEnum tpe(0);
			path = ts.Location;
			for(tpe.Init(P_Db->MakeFileName_(ts.TblName, path)); tpe.Next(spart, &first) > 0;) {
				if(fileExists(spart)) {
					if(safe) {
						SString safe_file;
						tpe.ReplaceExt(first, spart, safe_file);
						if(fileExists(safe_file))
							SFile::Remove(safe_file);
						THROW_V(SFile::Rename(spart, safe_file), SDBERR_SLIB);
					}
					else {
						THROW_V(SFile::Remove(spart), SDBERR_SLIB);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI DBBackup::RestoreRemovedDB(int restoreFiles)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	SString path, spart, spart_saved;
	DbTableStat ts;
	StrAssocArray tbl_list;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	P_Db->GetListOfTables(0, &tbl_list);
	for(uint j = 0; j < tbl_list.getCount(); j++) {
		const StrAssocArray::Item item = tbl_list.Get(j);
		if(P_Db->GetTableInfo(item.Id, &ts) > 0 && !(ts.Flags & XTF_DICT) && ts.Location.NotEmpty()) {
			path = ts.Location;
			int    first = 0;
			TablePartsEnum tpe(0);
			SString saved_file = P_Db->MakeFileName_(ts.TblName, path);
			SPathStruc::ReplaceExt(saved_file, "___", 1);
			for(tpe.Init(saved_file); tpe.Next(spart_saved, &first) > 0;) {
				tpe.ReplaceExt(first, spart_saved, spart);
				if(restoreFiles) {
					if(fileExists(spart_saved)) {
						if(fileExists(spart))
							SFile::Remove(spart);
						THROW_V(SFile::Rename(spart_saved, spart), SDBERR_SLIB);
					}
				}
				else if(fileExists(spart_saved))
					SFile::Remove(spart_saved);
			}
		}
	}
	CATCHZOK
	return ok;
}

#define SUBDIR_LINKFILES "LinkFiles" // PPTXT_LNKFILESDIR

int SLAPI DBBackup::CopyLinkFiles(const char * pSrcPath, const char * pDestPath, BackupLogFunc fnLog, long initParam)
{
	int    ok = -1;
	SString buf, src_dir, dest_dir;
	src_dir.CopyFrom(pSrcPath).SetLastSlash().Cat(SUBDIR_LINKFILES).RmvLastSlash();
	dest_dir.CopyFrom(pDestPath).SetLastSlash().Cat(SUBDIR_LINKFILES).RmvLastSlash();
	if(IsDirectory(src_dir)) {
		SString src_path, dest_path;
		SDirec direc;
		SDirEntry fb;

		RemoveDir(dest_dir);
		createDir(dest_dir);
		src_dir.SetLastSlash();
		dest_dir.SetLastSlash();
		(buf = src_dir).Cat("*.*");
		for(direc.Init(buf); direc.Next(&fb) > 0;) {
			if(!(fb.Attr & 0x10)) {
				(src_path = src_dir).Cat(fb.FileName);
				(dest_path = dest_dir).Cat(fb.FileName);
				if(SCopyFile(src_path, dest_path, DBBackup::CopyProgressProc, FILE_SHARE_READ, this) <= 0)
					LogMessage(fnLog, BACKUPLOG_ERR_COPY, src_path, initParam);
			}
		}
		ok = 1;
	}
	return ok;
}

int SLAPI DBBackup::CopyByRedirect(const char * pDBPath, BackupLogFunc fnLog, long initParam)
{
	SString redirect_file;
	redirect_file.CopyFrom(pDBPath).SetLastSlash().Cat(FILE_REDIRECT);
	if(fileExists(redirect_file)) {
		SFile f(redirect_file, SFile::mRead);
		if(f.IsValid()) {
 			SString buf;
			while(f.ReadLine(buf) > 0) {
				uint j = 0;
				SString spart;
				SPathStruc ps;
				SString tbl_name, dest_path, src_path;
				TablePartsEnum tpe(0);

				buf.Divide('=', tbl_name, dest_path);
				dest_path.TrimRightChr('\xA');
				dest_path.TrimRightChr('\xD');
				SPathStruc::ReplaceExt(tbl_name, ".btr", 1);
				dest_path.SetLastSlash().Cat(tbl_name);
				src_path.CopyFrom(pDBPath).SetLastSlash().Cat(tbl_name);

				for(tpe.Init(src_path); tpe.Next(spart) > 0;) {
					if(fileExists(spart)) {
						SPathStruc sp(spart);
						SPathStruc::ReplaceExt(dest_path, sp.Ext, 1);
						if(SCopyFile(spart, dest_path, DBBackup::CopyProgressProc, FILE_SHARE_READ, this) <= 0)
							LogMessage(fnLog, BACKUPLOG_ERR_COPY, src_path, initParam);
						else
							SFile::Remove(spart);
					}
				}
			}
		}
	}
	return 1;
}

int SLAPI DBBackup::Restore(BCopyData * pData, BackupLogFunc fnLog, long initParam)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	CopyParams cp;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	THROW(GetCopyParams(pData, &cp));
	P_Db->GetDataPath(cp.Path);
	if(CheckAvailableDiskSpace(cp.Path, cp.TotalSize) && CheckCopy(pData, cp, fnLog, initParam) > 0) {
		SString copy_path;
		THROW(RemoveDatabase(1));
		THROW(DoCopy(&cp, -BIN(pData->SrcSize != pData->DestSize), fnLog, initParam));
		THROW(CopyByRedirect(cp.Path, fnLog, initParam));
		(copy_path = pData->CopyPath).Strip().SetLastSlash().Cat(pData->SubDir).Strip();
		THROW(CopyLinkFiles(copy_path, cp.Path, fnLog, initParam)); // v5.6.12 AHTOXA
	}
	else
		CALLEXCEPT();
	CATCHZOK
	RestoreRemovedDB(ok != 1);
	return ok;
}

int SLAPI DBBackup::RemoveCopy(BCopyData * pData, BackupLogFunc fnLog, long initParam)
{
	EXCEPTVAR(DBErrCode);
	int    ok = 1;
	CopyParams cp;
	//char * p_src_file = 0;
	SString src_file_name;
	THROW_V(P_Db, SDBERR_BU_DICTNOPEN);
	GetCopyParams(pData, &cp);
	// @v9.6.4 for(i = 0; ok > 0 && cp.FileList.enumItems(&i, (void**)&p_src_file);) {
	for(uint ssp = 0; cp.SsFiles.get(&ssp, src_file_name);) {
		SFile::Remove(src_file_name);
		if(pData->DestSize != pData->SrcSize) {
			SPathStruc::ReplaceExt(src_file_name, "BT_", 1);
			SFile::Remove(src_file_name);
		}
	}
	RemoveDir(cp.Path);
	THROW(InfoF->RemoveRecord(pData->Set, pData->ID));
	LogMessage(fnLog, BACKUPLOG_SUC_REMOVE, pData->CopyPath, initParam);
	CATCHZOK
	return ok;
}

int SLAPI DBBackup::MakeCopyPath(BCopyData * data, SString & rDestPath)
{
	int    ok = 1;
	long   count = 1;
	SString path, subdir;
	rDestPath.Z();
	(path = data->CopyPath).RmvLastSlash();
	if(::access(path, 0) != 0) {
		if(!createDir(path))
			ok = (DBErrCode = SDBERR_SLIB, 0);
	}
	if(ok) {
		ulong  set_sum = 0;
		for(int n = 0; n < sizeof(data->Set) && data->Set[n] != 0; n++)
			set_sum += (ulong)data->Set[n];
		set_sum %= 10000L;
		do {
			subdir.Z().CatLongZ((long)set_sum, 4).CatLongZ(count++, 4);
			(path = data->CopyPath).SetLastSlash().Cat(subdir);
		} while(::access(path, 0) == 0);
		path.SetLastSlash();
		if(!createDir(path))
			ok = (DBErrCode = SDBERR_SLIB, 0);
		else {
			rDestPath = path;
			data->SubDir = subdir;
		}
	}
	return ok;
}

//static
int DBBackup::CopyProgressProc(const SDataMoveProgressInfo * scfd)
{
	DBBackup * dbb = (DBBackup *)scfd->ExtraPtr;
	return dbb->CBP_CopyProcess(scfd->P_Src, scfd->P_Dest, dbb->TotalCopySize, scfd->SizeTotal, dbb->TotalCopyReady + scfd->SizeDone, scfd->SizeDone);
}

int SLAPI DBBackup::DoCopy(DBBackup::CopyParams * pParam, long compr, BackupLogFunc fnLog, long initParam)
{
	EXCEPTVAR(DBErrCode);
#ifndef __CONFIG__
	int (* p_callback_proc)(long, long, const char *, int) = CallbackCompress;
#else
	int (* p_callback_proc)(long, long, const char *, int) = 0;
#endif //__CONFIG__

	int    ok = 1, use_temp_path = 0;
	uint   i = 0;
	SString src_file_name;
	SString dest, dest_file;
	SPathStruc ps, ps_inner;
	StringSet temp_ss_files;
	LogMessage(fnLog, BACKUPLOG_BEGIN, pParam->Path, initParam);
	if(compr > 0 && pParam->TempPath.NotEmptyS() && fileExists(pParam->TempPath)) {
		use_temp_path = 1;
		dest = pParam->TempPath;
	}
	else
		dest = pParam->Path;
	ps.Split(dest.SetLastSlash());
	TotalCopySize  = pParam->TotalSize;
	TotalCopyReady = 0;
	for(uint ssp = 0; pParam->SsFiles.get(&ssp, src_file_name);) {
		int64 sz = 0;
		SFileUtil::Stat stat;
		ps_inner.Split(src_file_name);
		ps_inner.Merge(&ps, SPathStruc::fDrv|SPathStruc::fDir, dest_file);
		if(!SFileUtil::GetStat(src_file_name, &stat))
			LogMessage(fnLog, BACKUPLOG_ERR_GETFILEPARAM, src_file_name, initParam);
		else if(compr) {
			SPathStruc::ReplaceExt(dest_file, (compr > 0) ? "bt_" : "btr", 1);
			if(use_temp_path) {
				temp_ss_files.add(dest_file);
				if(fileExists(dest_file))
					SFile::Remove(dest_file);
			}
			if(DoCompress(src_file_name, dest_file, &sz, (compr > 0), p_callback_proc) <= 0) {
				int    rec_id = (compr > 0) ? BACKUPLOG_ERR_COMPRESS : ((SLibError == SLERR_INVALIDCRC) ?
					BACKUPLOG_ERR_DECOMPRESSCRC : BACKUPLOG_ERR_DECOMPRESS);
				LogMessage(fnLog, rec_id, src_file_name, initParam);
			}
		}
		else {
			sz = stat.Size;
			if(SCopyFile(src_file_name, dest_file, DBBackup::CopyProgressProc, FILE_SHARE_READ, this) <= 0)
				LogMessage(fnLog, BACKUPLOG_ERR_COPY, src_file_name, initParam);
		}
		if(compr >= 0) {
			if(!use_temp_path)
				LogMessage(fnLog, BACKUPLOG_SUC_COPY, src_file_name, initParam);
		}
		else
			LogMessage(fnLog, BACKUPLOG_SUC_RESTORE, dest_file, initParam);
		TotalCopyReady += sz;
	}
	if(use_temp_path) {
		THROW(CheckAvailableDiskSpace(pParam->Path, TotalCopyReady));
		ps.Split((dest = pParam->Path).SetLastSlash());
		for(uint temp_ssp = 0; temp_ss_files.get(&temp_ssp, src_file_name);) {
			ps_inner.Split(src_file_name);
			ps_inner.Merge(&ps, SPathStruc::fDrv|SPathStruc::fDir, dest_file);
			THROW_V(SCopyFile(src_file_name, dest_file, DBBackup::CopyProgressProc, FILE_SHARE_READ, this) > 0, SDBERR_SLIB);
			SFile::Remove(src_file_name);
			if(compr > 0)
				LogMessage(fnLog, BACKUPLOG_SUC_COPY, src_file_name, initParam);
			else
				LogMessage(fnLog, BACKUPLOG_SUC_RESTORE, dest_file, initParam);
		}
	}
	pParam->TotalSize = TotalCopyReady;
	LogMessage(fnLog, BACKUPLOG_END, pParam->Path, initParam);
	CATCH
		ok = 0;
		LogMessage(fnLog, (compr > 0) ? BACKUPLOG_ERR_COPY : BACKUPLOG_ERR_RESTORE, src_file_name, initParam);
	ENDCATCH
	return ok;
}
