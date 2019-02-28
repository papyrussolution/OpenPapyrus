// DBENTSET.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2014, 2015, 2016, 2017, 2018 
// @Kernel
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

static int FASTCALL GetAveragePath(const char * name, SString & rBuf)
{
	char   drv[MAXDRIVE], dir[MAXDIR], fil[MAXFILE], ext[MAXEXT];
	char   buf[512];
	fnsplit(SLS.GetExePath(), drv, dir, fil, ext);
	fil[0] = ext[0] = 0;
	size_t p = sstrlen(dir);
	if(p) {
		p--;
		if(dir[p] == '\\')
			p--;
		while(p > 0 && dir[p] != '\\') // @v8.9.10 (p>=0)-->(p>0)
			p--;
	}
	if(dir[p] != '\\')
		dir[p] = '\\';
	p++;
	p += sstrlen(strcpy(dir + p, name));
	dir[p++] = '\\';
	dir[p] = 0;
	fnmerge(buf, drv, dir, fil, ext);
	rBuf = buf;
	return 1;
}
//
//
//
static const char * P_DefaultSymb = "$default$"; // @attention Используется так же в качестве ключа шифровки пароля //
static const char * P_DbNameSect = "dbname";
#define PWCRYPTBUFSIZE 128 // Длина шифруемого буфера, в котором содержится пароль

PPDbEntrySet2::PPDbEntrySet2() : DbLoginBlockArray()
{
}

// normal=Normal Database,c:\papyrus\ppy\data\normal
// ora-test=ORACLE:ora-test,c:\papyrus\ppy\data\ora-test,PPYDEV01@SYSTEM:PROTON

int SLAPI PPDbEntrySet2::MakeProfileLine(const DbLoginBlock * pBlk, SString & rBuf) const
{
	int    ok = 1;
	int    server_type = sqlstNone;
	SString temp_buf;

	rBuf.Z();
	pBlk->GetAttr(DbLoginBlock::attrServerType, temp_buf);
	if(temp_buf.IsEqiAscii("ORACLE") || temp_buf.IsEqiAscii("ORA")) {
		server_type = sqlstORA;
		rBuf.Cat("ORACLE").CatChar(':');
	}
	pBlk->GetAttr(DbLoginBlock::attrDbFriendlyName, temp_buf);
	rBuf.Cat(temp_buf).Comma();
	pBlk->GetAttr(DbLoginBlock::attrDbPath, temp_buf);
	rBuf.Cat(temp_buf);
	if(server_type == sqlstORA) {
		pBlk->GetAttr(DbLoginBlock::attrDbName, temp_buf);
		rBuf.Comma().Cat(temp_buf);
		pBlk->GetAttr(DbLoginBlock::attrUserName, temp_buf);
		rBuf.CatChar('@').Cat(temp_buf);
		pBlk->GetAttr(DbLoginBlock::attrPassword, temp_buf);
		if(temp_buf.NotEmptyS()) {
			char   pw_buf[512];
			size_t real_size = 0;
			IdeaRandMem(pw_buf, sizeof(pw_buf));
			temp_buf.CopyTo(pw_buf, sizeof(pw_buf));
			IdeaEncrypt(P_DefaultSymb, pw_buf, PWCRYPTBUFSIZE);
			temp_buf.EncodeMime64(pw_buf, PWCRYPTBUFSIZE);
			rBuf.CatChar(':').Cat(temp_buf);
		}
	}
	else {
		pBlk->GetAttr(DbLoginBlock::attrDictPath, temp_buf);
		if(temp_buf.NotEmptyS())
			rBuf.Comma().Cat(temp_buf);
	}
	return ok;
}

int SLAPI PPDbEntrySet2::ParseProfileLine(const char * pLine, DbLoginBlock * pBlk) const
{
	int    ok = 1;
	int    server_type = sqlstNone;
	SString temp_buf, left, right;
	SStrScan scan(pLine);
	scan.Skip();
	if(scan.Skip().SearchChar(',')) {
		scan.Get(temp_buf);
		scan.IncrLen(1);
		if(temp_buf.Divide(':', left, right) > 0) {
			left.Strip();
			right.Strip();
			if(left.IsEqiAscii("ORACLE") || left.IsEqiAscii("ORA")) {
				pBlk->SetAttr(DbLoginBlock::attrServerType, "ORACLE");
				server_type = sqlstORA;
			}
			else if(left.IsEqiAscii("DEFAULT") || left.IsEqiAscii("DEF")) {
				pBlk->SetAttr(DbLoginBlock::attrServerType, "DEFAULT");
			}
			else if(left.IsEqiAscii("BTRIEVE") || left.IsEqiAscii("BTR")) {
				pBlk->SetAttr(DbLoginBlock::attrServerType, "BTRIEVE");
			}
			else {
				CALLEXCEPT_PP_S(PPERR_INVPROFILESERVERTYPE, left);
			}
			pBlk->SetAttr(DbLoginBlock::attrDbFriendlyName, right);
		}
		else
			pBlk->SetAttr(DbLoginBlock::attrDbFriendlyName, left);
		//
		//
		//
		{
			SString data_path;
			if(scan.Skip().SearchChar(',')) {
				scan.Get(data_path);
				scan.IncrLen(1);
			}
			(left = scan).Strip();
			if(server_type == sqlstORA) {
				THROW_PP_S(scan.Skip().SearchChar('@'), PPERR_INVPROFILESQLDBP, scan);
				scan.Get(temp_buf);
				scan.IncrLen(1);
				temp_buf.Strip();
				pBlk->SetAttr(DbLoginBlock::attrDbName, temp_buf);
				(temp_buf = scan).Strip().Divide(':', left, right);
				pBlk->SetAttr(DbLoginBlock::attrUserName, left.Strip());
				if(right.NotEmptyS()) {
					char   pw_buf[512];
					size_t real_size = 0;
					right.DecodeMime64(pw_buf, sizeof(pw_buf), &real_size);
					assert(real_size == PWCRYPTBUFSIZE);
					IdeaDecrypt(P_DefaultSymb, pw_buf, real_size);
					pBlk->SetAttr(DbLoginBlock::attrPassword, pw_buf);
					memzero(pw_buf, sizeof(pw_buf));
				}
			}
			else if(data_path.NotEmpty())
				pBlk->SetAttr(DbLoginBlock::attrDictPath, left);
			else
				data_path = left;
			if(data_path.NotEmptyS()) {
				DS.ConvertPathToUnc(data_path.Strip().RmvLastSlash());
				pBlk->SetAttr(DbLoginBlock::attrDbPath, data_path);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDbEntrySet2::ReadFromProfile(PPIniFile * pIniFile, int existsPathOnly /*= 1*/, int dontLoadDefDict /*= 0*/)
{
	long   sSYS = 0x00535953L; // "SYS"
	long   sDAT = 0x00544144L; // "DAT"
	int    ok = 1;
	SString entry_symb, entry_buf, def_dict, def_data, temp_buf;
	PPIniFile * p_ini_file = NZOR(pIniFile, new PPIniFile);
	THROW_MEM(p_ini_file);
	if(p_ini_file->IsValid()) {
		// @construction (see comments below) SPathStruc ps;
		StringSet entries;
		p_ini_file->Get((uint)PPINISECT_PATH, (uint)PPINIPARAM_SYS, def_dict);
		p_ini_file->Get(PPINISECT_PATH, PPINIPARAM_DAT, def_data);
		p_ini_file->GetEntries(P_DbNameSect, &entries);
		for(uint pos = 0; entries.get(&pos, entry_symb);) {
			DbLoginBlock blk;
			PROFILE(p_ini_file->GetParam(P_DbNameSect, entry_symb, entry_buf));
			blk.SetAttr(DbLoginBlock::attrDbSymb, entry_symb);
			int    r;
			PROFILE(r = ParseProfileLine(entry_buf, &blk));
			if(r) {
				blk.GetAttr(DbLoginBlock::attrDictPath, temp_buf);
				blk.GetAttr(DbLoginBlock::attrDbPath, temp_buf);
				if(temp_buf.Empty() && !dontLoadDefDict)
					blk.SetAttr(DbLoginBlock::attrDbPath, def_dict);
				if(existsPathOnly) {
					//
					// @construction ps.Split(temp_buf);
					// @todo Здесь необходимо идентифицировать доступность
					// компьютера, на который ссылается каталог и, если он не доступен,
					// запомнить дабы для следующих каталогов не проверять доступность (ибо очень долго).
					//
					if(IsDirectory(temp_buf))
						THROW_SL(Add(0, &blk, 1));
				}
				else {
					THROW_SL(Add(0, &blk, 1));
				}
			}
		}
	}
	if(def_data.Empty())
		GetAveragePath((char *)&sDAT, def_data);
	if(def_dict.Empty())
		GetAveragePath((char *)&sSYS, def_dict);
	{
		DbLoginBlock blk;
		blk.SetAttr(DbLoginBlock::attrDbSymb, P_DefaultSymb);
		//blk.SetAttr(DbLoginBlock::attrDbFriendlyName, 0);
		blk.SetAttr(DbLoginBlock::attrDbPath, def_data);
		blk.SetAttr(DbLoginBlock::attrDictPath, def_dict);
		THROW_SL(Add(0, &blk, 1));
		SetDefaultSelection();
	}
	CATCHZOK
	if(!pIniFile)
		delete p_ini_file;
	return ok;
}

int SLAPI PPDbEntrySet2::RegisterEntry(PPIniFile * pIniFile, const DbLoginBlock * pBlk)
{
	int    ok = 1;
	PPIniFile * p_ini_file = NZOR(pIniFile, new PPIniFile);
	THROW_MEM(p_ini_file);
	if(p_ini_file->IsValid()) {
		SString entry_name, line_buf, name, data_path, dict_path;
		pBlk->GetAttr(DbLoginBlock::attrDbSymb, entry_name);
		if(entry_name.NotEmptyS()) {
			MakeProfileLine(pBlk, line_buf);
			THROW(p_ini_file->AppendParam(P_DbNameSect, entry_name, line_buf, 1));
		}
		/*
		if(useDTI && pUsername && pPassword) {
			SString server_name, pack_path;
			if(p_ini_file->Get(PPINISECT_PATH, PPINIPARAM_PACK, pack_path) > 0) {
				if(::access(data_path, 0))
					THROW_SL(createDir(data_path));
				CopyDataStruct(pack_path, data_path, BDictionary::DdfTableFileName);
				CopyDataStruct(pack_path, data_path, BDictionary::DdfFieldFileName);
				CopyDataStruct(pack_path, data_path, BDictionary::DdfIndexFileName);
			}
			PervasiveDBCatalog c;
			THROW(c.ServernameFromFilename(data_path, server_name));
			THROW(c.Connect(server_name, pUsername, pPassword));
			THROW(c.CreateDB(entry_name, data_path, data_path));
		}
		*/
	}
	CATCHZOK
	if(!pIniFile)
		delete p_ini_file;
	return ok;
}

long SLAPI PPDbEntrySet2::SetDefaultSelection()
{
	DbLoginBlock blk;
	/* @v9.0.9
	SelId = 0;
	if(GetBySymb(P_DefaultSymb, &blk) > 0) {
		SString temp_buf;
		blk.GetAttr(DbLoginBlock::attrID, temp_buf);
		SelId = temp_buf.ToLong();
	}
	*/
	SelId = GetBySymb(P_DefaultSymb, &blk); // @v9.0.9
	return SelId;
}
