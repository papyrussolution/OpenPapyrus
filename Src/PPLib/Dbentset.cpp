// DBENTSET.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2014, 2015, 2016, 2017, 2018, 2020, 2021, 2022
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

static void FASTCALL GetAveragePath(const char * pName, SString & rBuf)
{
	SPathStruc ps(SLS.GetExePath());
	size_t p = ps.Dir.Len();
	if(p) {
		p--;
		if(oneof2(ps.Dir.C(p), '\\', '/'))
			p--;
		while(p > 0 && !oneof2(ps.Dir.C(p), '\\', '/'))
			p--;
	}
	ps.Dir.Trim(p).SetLastSlash().Cat(pName).SetLastSlash();
	ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, rBuf);
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
// @v10.9.2 ora-test=ORACLE:ora-test,c:\papyrus\ppy\data\ora-test,url
/*
	url базы данных:
		host, port, user, password - как обычно в URL
		дополнительные аргументы - именованными параметрами
*/

/*static*/const char * DbLoginBlock::GetDefaultDbSymb() { return P_DefaultSymb; }

int DbLoginBlock::UrlParse(const char * pUrl)
{
	int    ok = 1;
	SString temp_buf;
	InetUrl url(pUrl);
	int proto = url.GetProtocol();
	SqlServerType st = sqlstNone;
	if(url.GetComponent(InetUrl::cPassword, 1, temp_buf) && temp_buf.NotEmptyS()) {
		char   pw_buf[512];
		size_t real_size = 0;
		temp_buf.DecodeMime64(pw_buf, sizeof(pw_buf), &real_size);
		if(real_size == PWCRYPTBUFSIZE) {
			IdeaDecrypt(P_DefaultSymb, pw_buf, real_size);
			SetAttr(attrPassword, pw_buf);
			memzero(pw_buf, sizeof(pw_buf));
		}
	}
	switch(proto) {
		case InetUrl::protFile:
			if(url.GetComponent(InetUrl::cUserName, 0, temp_buf))
				SetAttr(attrUserName, temp_buf);
			url.Composite(InetUrl::stHost|InetUrl::stPath, temp_buf);
			// @v10.9.3 @fix {
			{
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				DS.ConvertPathToUnc(temp_buf.Strip().RmvLastSlash());
			}
			// } @v10.9.3 
			SetAttr(attrDbPath, temp_buf);
			SetAttr(attrServerType, 0);
			break;
		case InetUrl::prot_p_MYSQL:
			st = sqlstMySQL;
			GetSqlServerTypeSymb(st, temp_buf);
			SetAttr(attrServerType, temp_buf);
			if(url.GetComponent(InetUrl::cUserName, 0, temp_buf))
				SetAttr(attrUserName, temp_buf);
			if(url.GetQueryParam("db", 0, temp_buf) > 0)
				SetAttr(attrDbName, temp_buf);
			url.Composite(InetUrl::stHost|InetUrl::stPort, temp_buf);
			SetAttr(attrServerUrl, temp_buf);
			break;
		case InetUrl::prot_p_ORACLE:
			st = sqlstMySQL;
			GetSqlServerTypeSymb(st, temp_buf);
			SetAttr(attrServerType, temp_buf);
			if(url.GetComponent(InetUrl::cUserName, 0, temp_buf))
				SetAttr(attrUserName, temp_buf);
			if(url.GetQueryParam("db", 0, temp_buf) > 0)
				SetAttr(attrDbName, temp_buf);
			url.Composite(InetUrl::stHost|InetUrl::stPort, temp_buf);
			SetAttr(attrServerUrl, temp_buf);
			break;
		case InetUrl::prot_p_SQLITE:
			break;
	}
	return ok;
}

int DbLoginBlock::UrlCompose(SString & rUrlBuf) const
{
	int    ok = 1;
	int    server_type = sqlstNone;
	SString temp_buf;
	SString pw_crypted_buf;
	InetUrl url;
	InetUrl inner_url;
	rUrlBuf.Z();
	GetAttr(DbLoginBlock::attrServerType, temp_buf);
	server_type = GetSqlServerTypeBySymb(temp_buf);
	GetAttr(attrServerUrl, temp_buf);
	inner_url.Parse(temp_buf);
	{
		int pw_isnt_empty = 0;
		if(GetAttr(attrPassword, temp_buf) > 0 && temp_buf.NotEmpty())
			pw_isnt_empty = 1;
		else if(inner_url.GetComponent(InetUrl::cPassword, 0, temp_buf) && temp_buf.NotEmpty())
			pw_isnt_empty = 1;
		if(pw_isnt_empty) {
			char   pw_buf[512];
			size_t real_size = 0;
			// @v11.1.1 IdeaRandMem(pw_buf, sizeof(pw_buf));
			SObfuscateBuffer(pw_buf, sizeof(pw_buf)); // @v11.1.1 
			temp_buf.CopyTo(pw_buf, sizeof(pw_buf));
			IdeaEncrypt(P_DefaultSymb, pw_buf, PWCRYPTBUFSIZE);
			temp_buf.EncodeMime64(pw_buf, PWCRYPTBUFSIZE);
			pw_crypted_buf.EncodeUrl(temp_buf, 1);
		}
	}
	if(server_type == sqlstMySQL) {
		url.SetProtocol(InetUrl::prot_p_MYSQL);
		if(inner_url.GetComponent(InetUrl::cHost, 0, temp_buf))
			url.SetComponent(InetUrl::cHost, temp_buf);
		if(inner_url.GetComponent(InetUrl::cPort, 0, temp_buf))
			url.SetComponent(InetUrl::cPort, temp_buf);
		if(GetAttr(attrUserName, temp_buf) > 0)
			url.SetComponent(InetUrl::cUserName, temp_buf);
		else if(inner_url.GetComponent(InetUrl::cUserName, 0, temp_buf))
			url.SetComponent(InetUrl::cUserName, temp_buf);
		if(pw_crypted_buf.NotEmpty())
			url.SetComponent(InetUrl::cPassword, pw_crypted_buf);
		if(GetAttr(attrDbName, temp_buf) > 0 && temp_buf.NotEmpty())
			url.SetQueryParam("db", temp_buf);
		else if(inner_url.GetQueryParam("db", 0, temp_buf) && temp_buf.NotEmpty())
			url.SetQueryParam("db", temp_buf);
	}
	else if(server_type == sqlstORA) {
		url.SetProtocol(InetUrl::prot_p_ORACLE);
		if(GetAttr(attrUserName, temp_buf) > 0)
			url.SetComponent(InetUrl::cUserName, temp_buf);
		else if(inner_url.GetComponent(InetUrl::cUserName, 0, temp_buf))
			url.SetComponent(InetUrl::cUserName, temp_buf);
		if(pw_crypted_buf.NotEmpty())
			url.SetComponent(InetUrl::cPassword, pw_crypted_buf);
		if(GetAttr(attrDbName, temp_buf) > 0 && temp_buf.NotEmpty())
			url.SetQueryParam("db", temp_buf);
		else if(inner_url.GetQueryParam("db", 0, temp_buf) && temp_buf.NotEmpty())
			url.SetQueryParam("db", temp_buf);
	}
	else if(server_type == sqlstSQLite) {
		url.SetProtocol(InetUrl::prot_p_SQLITE);
	}
	else if(server_type == sqlstNone) {
		url.SetProtocol(InetUrl::protFile);
		if(GetAttr(attrDbPath, temp_buf) > 0) {
			InetUrl inner2_url(temp_buf);
			if(inner2_url.GetComponent(InetUrl::cHost, 0, temp_buf))
				url.SetComponent(InetUrl::cHost, temp_buf);
			if(inner2_url.GetComponent(InetUrl::cPath, 0, temp_buf))
				url.SetComponent(InetUrl::cPath, temp_buf);
		}
		else {
			if(inner_url.GetComponent(InetUrl::cHost, 0, temp_buf)) 
				url.SetComponent(InetUrl::cHost, temp_buf);
			if(inner_url.GetComponent(InetUrl::cPath, 0, temp_buf)) 
				url.SetComponent(InetUrl::cPath, temp_buf);
		}
	}
	else {
	}
	url.Composite(InetUrl::stAll, rUrlBuf);
	return ok;
}

int PPDbEntrySet2::MakeProfileLine(const DbLoginBlock * pBlk, SString & rBuf) const
{
	int    ok = 1;
	int    server_type = sqlstNone;
	SString temp_buf;
	//
	pBlk->GetAttr(DbLoginBlock::attrDbFriendlyName, rBuf);
	rBuf.Comma();
	pBlk->UrlCompose(temp_buf);
	rBuf.Cat(temp_buf);
#if 0 // {
	rBuf.Z();
	pBlk->GetAttr(DbLoginBlock::attrServerType, temp_buf);
	server_type = GetSqlServerTypeBySymb(temp_buf);
	if(server_type != sqlstNone) {
		rBuf.Cat(temp_buf).Colon();
	}
	pBlk->GetAttr(DbLoginBlock::attrDbFriendlyName, temp_buf);
	rBuf.Cat(temp_buf).Comma();
	pBlk->GetAttr(DbLoginBlock::attrDbPath, temp_buf);
	rBuf.Cat(temp_buf);
	if(oneof3(server_type, sqlstORA, sqlstMSS, sqlstMySQL)) {
		pBlk->GetAttr(DbLoginBlock::attrDbName, temp_buf);
		rBuf.Comma().Cat(temp_buf);
		pBlk->GetAttr(DbLoginBlock::attrUserName, temp_buf);
		rBuf.CatChar('@').Cat(temp_buf);
		pBlk->GetAttr(DbLoginBlock::attrPassword, temp_buf);
		if(temp_buf.NotEmptyS()) {
			char   pw_buf[512];
			size_t real_size = 0;
			// @v11.1.1 IdeaRandMem(pw_buf, sizeof(pw_buf));
			SObfuscateBuffer(pw_buf, sizeof(pw_buf)); // @v11.1.1 
			temp_buf.CopyTo(pw_buf, sizeof(pw_buf));
			IdeaEncrypt(P_DefaultSymb, pw_buf, PWCRYPTBUFSIZE);
			temp_buf.EncodeMime64(pw_buf, PWCRYPTBUFSIZE);
			rBuf.Colon().Cat(temp_buf);
		}
	}
	else {
		pBlk->GetAttr(DbLoginBlock::attrDictPath, temp_buf);
		if(temp_buf.NotEmptyS())
			rBuf.Comma().Cat(temp_buf);
	}
#endif // } 0
	return ok;
}

int PPDbEntrySet2::ParseProfileLine(const char * pLine, DbLoginBlock * pBlk) const
{
	int    ok = 1;
	SqlServerType server_type = sqlstNone;
	SString temp_buf, left, right;
	// @v10.9.2 {
	{
		(temp_buf = pLine).Strip();
		if(temp_buf.Divide(',', left, right) > 0) {
			pBlk->SetAttr(DbLoginBlock::attrDbFriendlyName, left);
			temp_buf = right;
		}
		pBlk->UrlParse(temp_buf);
	}
	// } @v10.9.2 
#if 0 // @v10.9.2 {
	SStrScan scan(pLine);
	scan.Skip();
	if(scan.Skip().SearchChar(',')) {
		scan.Get(temp_buf);
		scan.IncrLen(1);
		if(temp_buf.Divide(':', left, right) > 0) {
			left.Strip();
			right.Strip();
			server_type = GetSqlServerTypeBySymb(left);
			if(oneof2(server_type, sqlstGeneric, sqlstNone))
				pBlk->SetAttr(DbLoginBlock::attrServerType, "DEFAULT");
			else 
				pBlk->SetAttr(DbLoginBlock::attrServerType, "BTRIEVE");
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
			if(oneof3(server_type, sqlstORA, sqlstMSS, sqlstMySQL)) {
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
			else if(data_path.NotEmpty()) {
				left.Transf(CTRANSF_INNER_TO_OUTER); // @v10.7.4
				pBlk->SetAttr(DbLoginBlock::attrDictPath, left);
			}
			else
				data_path = left;
			if(data_path.NotEmptyS()) {
				data_path.Transf(CTRANSF_INNER_TO_OUTER); // @v10.7.4
				DS.ConvertPathToUnc(data_path.Strip().RmvLastSlash());
				pBlk->SetAttr(DbLoginBlock::attrDbPath, data_path);
			}
		}
	}
	CATCHZOK
#endif // } 0
	return ok;
}

int PPDbEntrySet2::ReadFromProfile(PPIniFile * pIniFile, int existsPathOnly /*= 1*/, int dontLoadDefDict /*= 0*/)
{
	int    ok = 1;
	SString temp_buf;
	SString entry_symb;
	SString entry_buf;
	SString def_dict;
	SString def_data;
	SString server_type_symb;
	StringSet entries;
	PPIniFile * p_ini_file = NZOR(pIniFile, new PPIniFile);
	THROW_MEM(p_ini_file);
	THROW_SL(p_ini_file->IsValid());
	// @construction (see comments below) SPathStruc ps;
	p_ini_file->Get(PPINISECT_PATH, PPINIPARAM_SYS, def_dict);
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
			blk.GetAttr(DbLoginBlock::attrServerType, server_type_symb);
			const SqlServerType server_type = GetSqlServerTypeBySymb(server_type_symb);
			if(temp_buf.IsEmpty()) {
				if(!dontLoadDefDict)
					blk.SetAttr(DbLoginBlock::attrDbPath, def_dict);
			}
			if(existsPathOnly && server_type != sqlstMySQL) { // @v10.9.3 @debug (server_type != sqlstMySQL)
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
	if(def_data.IsEmpty())
		GetAveragePath("dat", def_data);
	if(def_dict.IsEmpty())
		GetAveragePath("sys", def_dict);
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

int PPDbEntrySet2::RegisterEntry(PPIniFile * pIniFile, const DbLoginBlock * pBlk)
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

long PPDbEntrySet2::SetDefaultSelection()
{
	DbLoginBlock blk;
	SelId = GetBySymb(P_DefaultSymb, &blk);
	return SelId;
}
