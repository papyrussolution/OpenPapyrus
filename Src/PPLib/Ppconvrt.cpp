// PPCONVRT.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
// Конвертация файлов данных при изменениях версий
//
#include <pp.h>
#pragma hdrstop
#include <private\_ppo.h>

int PPReEncryptDatabaseChain(PPObjBill * pBObj, Reference * pRef, const char * pSrcEncPw, const char * pDestEncPw, int use_ta);

int ConvertCipher(const char * pDbSymb, const char * pMasterPassword, const char * pSrcIniFileName, const char * pDestIniFileName)
{
	int    ok = 1;
	int    is_dict_opened = 0;
	SString temp_buf;
	SString src_pw_buf;
	SString dest_pw_buf;
	THROW_PP_S(!isempty(pDbSymb), PPERR_INVPARAM_EXT, __FUNCTION__"/pDbSymb");
	THROW(fileExists(pSrcIniFileName));
	THROW(fileExists(pDestIniFileName));
	{
		SIniFile ini_file_src(pSrcIniFileName);
		SIniFile ini_file_dest(pDestIniFileName);
		PapyrusPrivateBlock ppb_src;
		PapyrusPrivateBlock ppb_dest;
        THROW(ppb_src.ReadFromIni(ini_file_src));
        THROW(ppb_dest.ReadFromIni(ini_file_dest));
        if(ppb_src.DefPassword.NotEmpty() && ppb_dest.DefPassword.NotEmpty()) {
            //Reference
			PPDbEntrySet2 dbes;
			DbLoginBlock dblb;
			PPIniFile ini_file;
			dbes.ReadFromProfile(&ini_file);
			int dbsel = dbes.GetBySymb(pDbSymb, &dblb);
			if(dbsel > 0) {
				THROW(DS.OpenDictionary2(&dblb, 0));
				is_dict_opened = 1;
				PPID   user_id = 0;
				PPSecur sec_rec;
				THROW_MEM(PPRef = new Reference);
				THROW_MEM(BillObj = new PPObjBill(0));
				Reference * p_ref = PPRef;
				PPObjBill * p_bobj = BillObj;
				{
					const  PPID secur_obj_list[] = { PPOBJ_CONFIG, PPOBJ_USRGRP, PPOBJ_USR };
					PPTransaction tra(1);
					THROW(tra);
					THROW(PPReEncryptDatabaseChain(p_bobj, p_ref, ppb_src.DefPassword, ppb_dest.DefPassword, 0)); // Собсвенная транзакция
					for(uint si = 0; si < SIZEOFARRAY(secur_obj_list); si++) {
						const  PPID sec_obj_type = secur_obj_list[si];
						for(PPID sec_id = 0; p_ref->EnumItems(sec_obj_type, &sec_id, &sec_rec) > 0;) {
							if(sstrlen(sec_rec.Password)) {
								THROW(Reference::Helper_Decrypt_(Reference::crymRef2, ppb_src.DefPassword, sec_rec.Password, sizeof(sec_rec.Password), temp_buf));
							}
							else
								temp_buf.Z();
							THROW(Reference::Helper_Encrypt_(Reference::crymRef2, ppb_dest.DefPassword, temp_buf, sec_rec.Password, sizeof(sec_rec.Password)));
							THROW(Reference::VerifySecur(&sec_rec, 1));
							THROW(p_ref->UpdateItem(sec_obj_type, sec_id, &sec_rec, 0, 0));
						}
					}
					{
						PPAlbatrossConfig acfg;
						if(PPAlbatrosCfgMngr::Helper_Get(p_ref, &acfg) > 0) {
							SString password;
							acfg.GetExtStrData(ALBATROSEXSTR_UHTTPASSW, password);
							Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, password/*acfg.UhttPassword*/, /*UHTT_PW_SIZE*/20, temp_buf);
							Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, temp_buf, /*UHTT_PW_SIZE*/20, password/*acfg.UhttPassword*/);
							acfg.PutExtStrData(ALBATROSEXSTR_UHTTPASSW, password);
							THROW(PPAlbatrosCfgMngr::Helper_Put(p_ref, &acfg, 0));
						}
					}
					{
						PPPhoneService phs_rec;
						for(PPID phs_id = 0; p_ref->EnumItems(PPOBJ_PHONESERVICE, &phs_id, &phs_rec) > 0;) {
							if(p_ref->GetPropVlrString(PPOBJ_PHONESERVICE, phs_id, PHNSVCPRP_TAIL, temp_buf) > 0) {
								PPGetExtStrData(PHNSVCEXSTR_PASSWORD, temp_buf, src_pw_buf);
								Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, src_pw_buf, /*PHNSVC_PW_SIZE*/64, dest_pw_buf);
								Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, dest_pw_buf, /*PHNSVC_PW_SIZE*/64, src_pw_buf);
								PPPutExtStrData(PHNSVCEXSTR_PASSWORD, temp_buf, src_pw_buf);
								THROW(p_ref->PutPropVlrString(PPOBJ_PHONESERVICE, phs_id, PHNSVCPRP_TAIL, temp_buf));
							}
						}
					}
					{
						PPInternetAccount in_rec;
						for(PPID in_id = 0; p_ref->EnumItems(PPOBJ_INTERNETACCOUNT, &in_id, &in_rec) > 0;) {
							if(p_ref->GetPropVlrString(PPOBJ_INTERNETACCOUNT, in_id, MACPRP_EXTRA, in_rec.ExtStr)) {
								{
									PPGetExtStrData(MAEXSTR_RCVPASSWORD, temp_buf, src_pw_buf);
									Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, src_pw_buf, /*POP3_PW_SIZE*/20, dest_pw_buf);
									Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, dest_pw_buf, /*POP3_PW_SIZE*/20, src_pw_buf);
									PPPutExtStrData(MAEXSTR_RCVPASSWORD, temp_buf, src_pw_buf);
								}
								{
									PPGetExtStrData(FTPAEXSTR_PASSWORD, temp_buf, src_pw_buf);
									Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, src_pw_buf, /*POP3_PW_SIZE*/20, dest_pw_buf);
									Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, dest_pw_buf, /*POP3_PW_SIZE*/20, src_pw_buf);
									PPPutExtStrData(FTPAEXSTR_PASSWORD, temp_buf, src_pw_buf);
								}
								{
									PPGetExtStrData(FTPAEXSTR_PROXYPASSWORD, temp_buf, src_pw_buf);
									Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, src_pw_buf, /*POP3_PW_SIZE*/20, dest_pw_buf);
									Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, dest_pw_buf, /*POP3_PW_SIZE*/20, src_pw_buf);
									PPPutExtStrData(FTPAEXSTR_PROXYPASSWORD, temp_buf, src_pw_buf);
								}
								THROW(p_ref->PutPropVlrString(PPOBJ_INTERNETACCOUNT, in_id, MACPRP_EXTRA, temp_buf));
							}
						}
					}
					THROW(tra.Commit());
				}
			}
        }
	}
	CATCHZOK
	ZDELETE(BillObj);
	ZDELETE(PPRef);
	if(is_dict_opened)
		DBS.CloseDictionary();
	return ok;
}
//
//
//
#define CONVERT_PROC(proc_name, class_name) \
int proc_name() \
{                     \
	PPWaitStart();        \
	class_name cvt;   \
	int    ok = BIN(cvt.Convert()); \
	PPWaitStop();                         \
	return ok;                         \
}

class PPTableConversion {
	enum {
		stCommonRefFlagsUpdated = 0x0001 // Устанавливается если в таблице PPRef и ее членах был установлен флаг XTF_DISABLEOUTOFTAMSG
	};
	uint   State;
public:
	int    Convert();
protected:
	PPTableConversion() : State(0)
	{
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion) = 0;
	virtual void DestroyTable(DBTable * pTbl);
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen) = 0;
	virtual int Final(DBTable * pTbl) { return -1; }
	PPLogger Logger;
	void   RestoreCommonRefFlags()
	{
		if(State & stCommonRefFlagsUpdated) {
			Reference * p_ref = PPRef;
			if(p_ref) {
				p_ref->ResetFlag(XTF_DISABLEOUTOFTAMSG);
				p_ref->Ot.ResetFlag(XTF_DISABLEOUTOFTAMSG);
				p_ref->UtrC.ResetFlag(XTF_DISABLEOUTOFTAMSG);
			}
		}
	}
	Reference * GetReferenceInstance(Reference ** ppMemberPtr)
	{
		Reference * p_ref = 0;
		assert(ppMemberPtr);
		if(ppMemberPtr) {
			if(*ppMemberPtr) {
				p_ref = *ppMemberPtr;
			}
			else if(PPRef) {
				p_ref = PPRef;
				if(!(State & stCommonRefFlagsUpdated)) {
					p_ref->SetFlag(XTF_DISABLEOUTOFTAMSG);
					p_ref->Ot.SetFlag(XTF_DISABLEOUTOFTAMSG);
					p_ref->UtrC.SetFlag(XTF_DISABLEOUTOFTAMSG);
					State |= stCommonRefFlagsUpdated;
				}
			}
			else {
				*ppMemberPtr = new Reference;
				if(*ppMemberPtr) {
					p_ref = *ppMemberPtr;
					p_ref->SetFlag(XTF_DISABLEOUTOFTAMSG);
					p_ref->Ot.SetFlag(XTF_DISABLEOUTOFTAMSG);
					p_ref->UtrC.SetFlag(XTF_DISABLEOUTOFTAMSG);
				}
			}
		}
		return p_ref;
	}
};

void PPTableConversion::DestroyTable(DBTable * pTbl)
{
	delete pTbl;
}

int PPTableConversion::Convert()
{
	//
	// Так как сеанс конвертации может состоять из нескольких конвертаций отдельных таблиц
	// необходимо иметь общий каталог, в который будут сбрасываться резервные копии всех конвертируемых в
	// данном сеансе файлов.
	//
	static SString _BakPath;
	//
	int    ok = 1;
	int    need_conversion = 0;
	int    db_locked = 0;
	uint   i;
	RECORDSIZE   rec_size;
	RECORDNUMBER num_recs = 0;
	RECORDNUMBER count = 0;
	RECORDNUMBER skip_count = 0;
	char * p_old_buf = 0;
	DBTable * p_tbl     = CreateTableInstance(&need_conversion);
	DBTable * p_cr_tbl  = 0;
	DBTable * p_old_tbl = 0;
	SString temp_buf;

	DBTablePartitionList tpl; // Агрегат, содержащий информацию об оригинальных файлах, подлежащих конвертации
	DBTablePartitionList::Entry tpe, tpe_con;
	LongArray moved_pos_list; // Список позиций файлов в массиве tpl, которые фактически были перемещены в резервный каталог

	ENTER_CRITICAL_SECTION
	if(need_conversion) {
		SString ext_str, new_path, file_path;
		SString old_fname_to_convert; // Имя файла, который мы будет открывать для конвертации.
		SFsPath path_struc;
		THROW(DS.GetSync().LockDB());
		db_locked = 1;
		SString tbl_name(p_tbl->GetTableName());
		const SString file_name = p_tbl->GetName();
		DestroyTable(p_tbl);
		p_tbl = 0;

		path_struc.Split(file_name);
		if(_BakPath.IsEmpty() || !::fileExists(_BakPath)) {
			for(i = 1; i < 100000; i++) {
				SFsPath ps2;
				ps2.Copy(&path_struc, SFsPath::fDrv|SFsPath::fDir);
				ps2.Dir.SetLastSlash().Cat("CVT").CatLongZ((long)i, 5);
				ps2.Merge(temp_buf);
				if(!fileExists(temp_buf)) {
					THROW_SL(SFile::CreateDir(temp_buf));
					_BakPath = temp_buf;
					break;
				}
			}
		}
		THROW_PP_S(_BakPath.NotEmpty() && fileExists(_BakPath), /*PPERR_CVT_BAKPATHFAULT*/1, file_name);
		{
			LongArray to_copy_pos_list; // Список позиций файлов в массиве tpl, которые необходимо скопировать в резервный каталог
			THROW_DB(tpl.Init(file_name, 0, 0));
			if(tpl.GetConEntry(tpe_con)) {
				//
				// Если существует файл continuous-состояния (^^^) то дадим ему несколько секунд на исчезновение...
				//
				SDelay(5000);
				// ... и снова инициализируем tpl
				THROW_DB(tpl.Init(file_name, 0, 0));
			}
			for(i = 0; i < tpl.GetCount(); i++) {
				THROW_SL(tpl.Get(i, tpe));
				if((tpe.Flags && !(tpe.Flags & tpl.fBu)) && (tpe.Flags & (tpl.fMain|tpl.fExt|tpl.fCon))) {
					THROW_PP_S(SFile::IsOpenedForWriting(tpe.Path) == 0, PPERR_CVT_FILEWROPENED/*1*/, tpe.Path);
					to_copy_pos_list.add(static_cast<long>(i));
				}
			}
			for(i = 0; i < to_copy_pos_list.getCount(); i++) {
				THROW_SL(tpl.Get(to_copy_pos_list.get(i), tpe));
				SFsPath::ReplacePath(temp_buf = tpe.Path, _BakPath, 1);
				THROW_SL(SFile::Rename(tpe.Path, temp_buf));
				moved_pos_list.add(i);
				if(!(tpe.Flags & tpl.fBu) && tpe.Flags & tpl.fMain) {
					assert(!old_fname_to_convert.NotEmpty());
					old_fname_to_convert = temp_buf;
				}
			}
		}
		{
			THROW_MEM(p_cr_tbl = new DBTable(tbl_name, file_name));
			THROW_DB(p_cr_tbl->IsOpened());
			ZDELETE(p_cr_tbl);
			THROW_MEM(p_tbl = CreateTableInstance(0));
			p_tbl->SetFlag(XTF_DISABLEOUTOFTAMSG);
			THROW_MEM(p_old_tbl = new DBTable(0, old_fname_to_convert, omNormal/*, p*/));
			rec_size = 4096;
			p_old_tbl->getNumRecs(&num_recs);
			THROW_MEM(p_old_buf = new char[rec_size]);
			memzero(p_old_buf, rec_size);
			p_old_tbl->SetDBuf(p_old_buf, rec_size);
			if(p_old_tbl->step(spFirst)) {
				do {
					int    new_rec_len = p_old_tbl->GetRetBufLen();
					if(ConvertRec(p_tbl, p_old_buf, &new_rec_len) > 0) {
						if(!p_tbl->insertRec()) {
							PPSetError(PPERR_DBENGINE);
							Logger.LogLastError();
						}
					}
					else {
						skip_count++;
					}
					memzero(p_old_buf, rec_size);
					PPWaitPercent(++count, num_recs, tbl_name);
					memzero(p_old_buf, rec_size);
				} while(p_old_tbl->step(spNext));
			}
			THROW(Final(p_tbl));
			p_tbl->ResetFlag(XTF_DISABLEOUTOFTAMSG);
		}
	}
	else
		ok = -1;
	LEAVE_CRITICAL_SECTION
	CATCH
		ZDELETE(p_cr_tbl);
		DestroyTable(p_tbl);
		p_tbl = 0;
		ZDELETE(p_old_tbl);
		//
		// Раз ничего не вышло, то возвращаем назад перемещенные файлы
		//
		for(i = 0; i < moved_pos_list.getCount(); i++) {
			tpl.Get(moved_pos_list.get(i), tpe);
			SFsPath::ReplacePath(temp_buf = tpe.Path, _BakPath, 1);
			SFile::Remove(tpe.Path);
			SFile::Rename(temp_buf, tpe.Path);
		}
		ok = 0;
	ENDCATCH
	if(db_locked) {
		DS.GetSync().UnlockDB();
	}
	ZDELETE(p_cr_tbl);
	DestroyTable(p_tbl);
	p_tbl = 0;
	ZDELETE(p_old_tbl);
	delete p_old_buf;
	return ok;
}

#if 0 // {
//
//
//
class PPCvtSJ329 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SysJournalTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = (recsz == sizeof(SysJournalTbl::Rec)) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		struct OldSjRec {
			LDATE   Dt;
			LTIME   Tm;
			long    User;
			long    Action;
			long    ObjType;
			long    ObjID;
			char    Info[32];
		} * p_old_sj_rec = (OldSjRec*)rec;

		SysJournalTbl::Rec * p_data = (SysJournalTbl::Rec*)tbl->getDataBuf();
		p_data->Dt = p_old_sj_rec->Dt;
		p_data->Tm = p_old_sj_rec->Tm;
		p_data->User = p_old_sj_rec->User;
		p_data->Action = p_old_sj_rec->Action;
		p_data->ObjType = p_old_sj_rec->ObjType;
		p_data->ObjID = p_old_sj_rec->ObjID;
		if(p_data->Action == PPACN_EXPCASHSESS) {
			int16 e = *(int16 *)p_old_sj_rec->Info;
			p_data->Extra = e;
		}
		else
			p_data->Extra = 0;
		return 1;
	}
};

CONVERT_PROC(Convert329, PPCvtSJ329);
//
// Эта конвертация перенесена в Convert4405
//
class PPCvtRegister3512 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = (recsz == sizeof(RegisterTbl::Rec)) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		pTbl->clearDataBuf();
		RegisterTbl::Rec tmp_buf;
		MEMSZERO(tmp_buf);
		memcpy(&tmp_buf, pRec, sizeof(RegisterTbl::Rec));
		char   serial_buf[48];
		int    c = 0;
		STRNSCPY(serial_buf, tmp_buf.Serial);
		while(1) {
			RegisterTbl::Key3 k3;
			MEMSZERO(k3);
			k3.RegTypeID = tmp_buf.RegTypeID;
			STRNSCPY(k3.Serial, tmp_buf.Serial);
			STRNSCPY(k3.Number, tmp_buf.Number);
			if(pTbl->search(3, &k3, spEq)) {
				sprintf(tmp_buf.Serial, "%s #%d", serial_buf, ++c);
				strip(tmp_buf.Serial);
			}
			else {
				pTbl->CopyBufFrom(&tmp_buf, sizeof(tmp_buf));
				break;
			}
		}
		return 1;
	}
};

CONVERT_PROC(Convert3512, PPCvtRegister3512);

// @v9.0.4 (Перенесено ниже) #endif // } 0
//
//
//
class PPCvtCCheck372 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CCheckTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(CCheckTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const struct OldCCheckRec {
			long    ID;
			long    Code;
			long    CashID;
			long    UserID;
			long    SessID;
			LDATE   Dt;
			LTIME   Tm;
			long    Flags;
			char    Amount[8];
			char    Discount[8];
		} * p_old_data = static_cast<const OldCCheckRec *>(pRec);

		CCheckTbl::Rec * p_data = static_cast<CCheckTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(CCheckTbl::Rec));
		p_data->ID     = p_old_data->ID;
		p_data->Code   = p_old_data->Code;
		p_data->CashID = p_old_data->CashID;
		p_data->UserID = p_old_data->UserID;
		p_data->SessID = p_old_data->SessID;
		p_data->Dt     = p_old_data->Dt;
		p_data->Tm     = p_old_data->Tm;
		p_data->Flags  = p_old_data->Flags;
		memcpy(p_data->Amount,   p_old_data->Amount,   sizeof(p_data->Amount));
		memcpy(p_data->Discount, p_old_data->Discount, sizeof(p_data->Discount));
		p_data->SCardID = 0; // @newfield
		return 1;
	}
};

CONVERT_PROC(Convert372, PPCvtCCheck372);
//
//
//
class PPCvtAccount400 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new AccountTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(PPAccount));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const struct OldAccRec {
			long    ID;
			short   Ac;
			short   Sb;
			long    CurID;
			long    AccSheetID;
			LDATE   OpenDate;
			short   Reserve1;
			short   Type;
			short   Kind;
			long    Flags;
			char    Name[48];
			double  Limit;
			double  Overdraft;
			short   AccessLevel;
			LDATE   FRRL_Date;
			char    Reserve2[8];
		} * p_old_data = static_cast<const OldAccRec *>(pRec);
		PPAccount * p_data = static_cast<PPAccount *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(PPAccount));

		p_data->ID = p_old_data->ID;
		p_data->Ac = p_old_data->Ac;
		p_data->Sb = p_old_data->Sb;
		p_data->CurID = p_old_data->CurID;
		p_data->AccSheetID = p_old_data->AccSheetID;
		p_data->OpenDate = p_old_data->OpenDate;
		switch(p_old_data->Type) {
			case 0: p_data->Type = ACY_BAL; break;
			case 1: p_data->Type = ACY_OBAL; break;
			case 2: p_data->Type = ACY_AGGR; break;
			case 3: p_data->Type = ACY_REGISTER; break;
			default: p_data->Type = p_old_data->Type; break;
		}
		p_data->Kind = p_old_data->Kind;
		p_data->Flags = p_old_data->Flags;
		p_data->Limit = p_old_data->Limit;
		p_data->Overdraft = p_old_data->Overdraft;
		p_data->FRRL_Date = p_old_data->FRRL_Date;
		STRNSCPY(p_data->Name, p_old_data->Name);
		AccountCore::GenerateCode(*p_data);
		return 1;
	}
};

CONVERT_PROC(Convert400, PPCvtAccount400);
//
//
//
class PPCvtObjTag31102 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new ObjTagTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16 num_keys = 0;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 4);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		ObjTagTbl::Rec * p_data = static_cast<ObjTagTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(ObjTagTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert31102, PPCvtObjTag31102);
//
//
//
class PPCvtCCheck31110 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CCheckTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16 num_keys = 0;
			p_tbl->getNumKeys(&num_keys);
			if(num_keys < 5)
				*pNeedConversion = 1;
			else {
				int    num_seg = 0;
				DBIdxSpec * p_is = p_tbl->getIndexSpec(4, &num_seg);
				*pNeedConversion = BIN(p_is && num_seg == 4);
				SAlloc::F(p_is);
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		CCheckTbl::Rec * p_data = static_cast<CCheckTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(CCheckTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert31110, PPCvtCCheck31110);

#endif // } 0 @v9.0.4 (снята поддержка Convert31110, Convert31102, Convert400, Convert372)
//
//
//
#if 0 // moved to PPCvtBill11112 {
class PPCvtBill4108 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			if(recsz < (sizeof(BillTbl::Rec) - sizeof(((BillTbl::Rec*)0)->Memo)))
				*pNeedConversion = 1;
			else
				*pNeedConversion = 0;
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		struct OldBillRec { // Size = 76+160
			long   ID;       // Ид. документа
			char   Code[10];    // Код документа
			LDATE  Dt;          // Дата документа
			long   BillNo;      // Номер документа за день
			long   OprKind;     // Вид операции          ->Ref(PPOBJ_OPRKIND)
			long   UserID;      // Пользователь          ->Ref(PPOBJ_USR)
			long   Location;    // Позиция               ->Location.ID
			long   Object;      // Контрагент            ->Article.ID
			long   Object2;     // Дополнительный объект ->Article.ID
			long   CurID;       // Валюта (0 - базовая)  ->Ref(PPOBJ_CURRENCY)
			double CRate;       // Курс валюты для пересчета в базовую валюту
			char   Amount[8];   // Номинальная сумма (в единицах CurID)
			long   LinkBillID;  // Связанный документ    ->Bill.ID
			long   Flags;       // Флаги
			int16  AccessLevel; // Уровень доступа к документу
			//long   SCardID;     // @v4.1.8 ->SCard.ID
			char   Memo[160];   // Примечание
		} * p_rec = (OldBillRec *)rec;
		BillTbl::Rec * p_data = (BillTbl::Rec*)tbl->getDataBuf();
		tbl->clearDataBuf();

#define CPY_FLD(f) p_data->f = p_rec->f
		CPY_FLD(ID);
		CPY_FLD(Dt);
		CPY_FLD(BillNo);
		CPY_FLD(UserID);
		CPY_FLD(Object);
		CPY_FLD(Object2);
		CPY_FLD(CurID);
		CPY_FLD(CRate);
		CPY_FLD(LinkBillID);
		CPY_FLD(Flags);
#undef CPY_FLD
		p_data->OpID = p_rec->OprKind;
		p_data->LocID = p_rec->Location;
		p_data->Amount = MONEYTOLDBL(p_rec->Amount);
		STRNSCPY(p_data->Code, p_rec->Code);
		STRNSCPY(p_data->Memo, p_rec->Memo);
		*pNewRecLen = -1;
		return 1;
	}
};
#endif // } 0 moved to PPCvtBill11112

#if 0 // { Перенесено в PPCvtCCheckLine5207
class PPCvtCCheckLine4108 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtCCheckLine4108::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new CCheckLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		//
		// В новом формате запись короче (28 байтов вместо 40)
		//
		*pNeedConversion = BIN(recsz > sizeof(CCheckLineTbl::Rec));
	}
	return p_tbl;
}

int PPCvtCCheckLine4108::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldCCheckLineRec {
		long   CheckID;      // -> CCheck.ID
		long   RByCheck;     // Счетчик строк по чеку
		long   DivID;        // Отдел магазина
		long   GoodsID;      // -> Goods.ID
		double Quantity;     // Количество товара
		char   Price[8];     // Цена
		char   Discount[8];  // Скидка
	} * p_old_data = static_cast<const OldCCheckLineRec *>(pRec);
	CCheckLineTbl::Rec * p_data = static_cast<CCheckLineTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(CCheckLineTbl::Rec));
	p_data->CheckID = p_old_data->CheckID;
	p_data->RByCheck = (short)p_old_data->RByCheck;
	p_data->DivID = (short)p_old_data->DivID;
	p_data->GoodsID = p_old_data->GoodsID;
	p_data->Quantity = p_old_data->Quantity;
	p_data->Price = dbltointmny(MONEYTOLDBL(p_old_data->Price));
	// @v5.2.7 p_data->Discount = dbltointmny(MONEYTOLDBL(p_old_data->Discount));
	p_data->Dscnt = MONEYTOLDBL(p_old_data->Discount); // @v5.2.7
	return 1;
}

#endif // } 0
//
//
//
#if 0 // @v12.4.1 {
class PPCvtSCardOp4108 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SCardOpTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			//
			// В новом формате запись короче (48 байтов вместо 56)
			//
			*pNeedConversion = BIN(recsz > sizeof(SCardOpTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		SCardOpTbl::Rec * p_data = static_cast<SCardOpTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(SCardOpTbl::Rec));
		return 1;
	}
};

int Convert4108()
{
	int    ok = 1;
	PPWaitStart();
	/* moved to PPCvtBill11112 @v11.1.12 if(ok) {
		PPCvtBill4108 cvt;
		ok = cvt.Convert() ? 1 : PPErrorZ();
	}*/
	/* @v5.2.7
	if(ok) {
		PPCvtCCheckLine4108 cvt2;
		ok = cvt2.Convert() ? 1 : PPErrorZ();
	}
	*/
	if(ok) {
		PPCvtSCardOp4108 cvt3;
		ok = cvt3.Convert() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
#endif // } @v12.4.1
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtAdvBillItem6202 {
class PPCvtAdvBillItem4208 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new AdvBillItemTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16 flags = 0;
			if(p_tbl->getTabFlags(&flags))
				*pNeedConversion = (flags & XTF_VLR) ? 0 : 1;
			else {
				*pNeedConversion = 0;
				ZDELETE(p_tbl);
				PPSetErrorDB();
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		AdvBillItemTbl::Rec * p_data = (AdvBillItemTbl::Rec*)pNewTbl->getDataBuf();
		pNewTbl->clearDataBuf();
		memcpy(p_data, pOldRec, sizeof(AdvBillItemTbl::Rec) - sizeof(((AdvBillItemTbl::Rec*)0)->Memo));
		*pNewRecLen = -1;
		return 1;
	}
};

CONVERT_PROC(Convert4208, PPCvtAdvBillItem4208);

#endif // } 0 @v6.2.2 Moved to PPCvtAdvBillItem6202
//
//
//
#if 0 // Перенесено в Convert5200 {

class PPCvtGoods4405 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtGoods4405::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new Goods2Tbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(Goods2Tbl::Rec));
	}
	return p_tbl;
}

int PPCvtGoods4405::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldRec {
		long    ID;
		long    Kind;
		char    Name[48];
		char    Abbr[48];
		long    ParentID;
		long    GoodsTypeID;
		long    UnitID;
		long    PhUnitID;
		double  PhUPerU;
		long    ManufID;
		long    StrucID;
		long    TaxGrpID;
		long    WrOffGrpID;
		long    Flags;
		long    GdsClsID;
		long    BrandID;
	} * p_old_rec = static_cast<const OldRec *>(pRec);
	Goods2Tbl::Rec * p_data = static_cast<Goods2Tbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(Goods2Tbl::Rec));

	p_data->ID = p_old_rec->ID;
	p_data->Kind = p_old_rec->Kind;
	memcpy(p_data->Name, p_old_rec->Name, sizeof(p_old_rec->Name));
	memcpy(p_data->Abbr, p_old_rec->Abbr, sizeof(p_old_rec->Abbr));
	p_data->ParentID = p_old_rec->ParentID;
	p_data->GoodsTypeID = p_old_rec->GoodsTypeID;
	p_data->UnitID     = p_old_rec->UnitID;
	p_data->PhUnitID   = p_old_rec->PhUnitID;
	p_data->PhUPerU    = p_old_rec->PhUPerU;
	p_data->ManufID    = p_old_rec->ManufID;
	p_data->StrucID    = p_old_rec->StrucID;
	p_data->TaxGrpID   = p_old_rec->TaxGrpID;
	p_data->WrOffGrpID = p_old_rec->WrOffGrpID;
	p_data->Flags      = p_old_rec->Flags;
	p_data->GdsClsID   = p_old_rec->GdsClsID;
	p_data->BrandID    = p_old_rec->BrandID;
	return 1;
}

#endif // } 0
#if 0 // @v12.4.1 {
class PPCvtQCert4405 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new QualityCertTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys = 0;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys == 4);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		QualityCertTbl::Rec * p_data = static_cast<QualityCertTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(QualityCertTbl::Rec));
		return 1;
	}
};
//
//
//
class PPCvtPriceLine4405 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
private:
	int    _pre380format;
};

DBTable * PPCvtPriceLine4405::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new PriceLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		if(recsz < (sizeof(PriceLineTbl::Rec)-sizeof(((PriceLineTbl::Rec*)0)->Memo))) {
			*pNeedConversion = 1;
			_pre380format = BIN(recsz < 152);
		}
		else
			*pNeedConversion = 0;
	}
	return p_tbl;
}

int PPCvtPriceLine4405::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldPriceLineRec1 {
		long    ListID;
		long    GoodsID;
		short   LineNo;
		long    GoodsGrpID;
		long    ManufID;
		char    Name[40];
		long    UnitID;
		short   IsPresent;
		double  UnitPerPack;
		char    Price[8];
		long    QuotKindID;
		double  AddPrice1;
		double  AddPrice2;
		double  AddPrice3;
		char    Reserve[32];
		LDATE   Expiry;
		char    Memo[128];
	} * p_old_data1 = static_cast<const OldPriceLineRec1 *>(pRec);
	const struct OldPriceLineRec2 {
		long    ListID;
		long    GoodsID;
		short   LineNo;
		long    GoodsGrpID;
		long    ManufID;
		char    Name[48];
		long    UnitID;
		short   IsPresent;
		double  UnitPerPack;
		char    Price[8];
		long    QuotKindID;
		double  AddPrice1;
		double  AddPrice2;
		double  AddPrice3;
		char    Reserve[24];
		double  Rest;
		LDATE   Expiry;
		char    Memo[128];
	} * p_old_data2 = static_cast<const OldPriceLineRec2 *>(pRec);

	PriceLineTbl::Rec * p_data = static_cast<PriceLineTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(PriceLineTbl::Rec));
	if(_pre380format) {
		p_data->ListID      = p_old_data1->ListID;
		p_data->GoodsID     = p_old_data1->GoodsID;
		p_data->LineNo      = p_old_data1->LineNo;
		p_data->GoodsGrpID  = p_old_data1->GoodsGrpID;
		p_data->ManufID     = p_old_data1->ManufID;
		STRNSCPY(p_data->Name, p_old_data1->Name);
		p_data->UnitID      = p_old_data1->UnitID;
		p_data->IsPresent   = p_old_data1->IsPresent;
		p_data->UnitPerPack = p_old_data1->UnitPerPack;
		p_data->Price       = MONEYTOLDBL(p_old_data1->Price);
		p_data->QuotKindID  = p_old_data1->QuotKindID;
		p_data->AddPrice1   = p_old_data1->AddPrice1;
		p_data->AddPrice2   = p_old_data1->AddPrice2;
		p_data->AddPrice3   = p_old_data1->AddPrice3;
		p_data->Expiry      = p_old_data1->Expiry;
		STRNSCPY(p_data->Memo, p_old_data1->Memo);
	}
	else {
		p_data->ListID      = p_old_data2->ListID;
		p_data->GoodsID     = p_old_data2->GoodsID;
		p_data->LineNo      = p_old_data2->LineNo;
		p_data->GoodsGrpID  = p_old_data2->GoodsGrpID;
		p_data->ManufID     = p_old_data2->ManufID;
		STRNSCPY(p_data->Name, p_old_data2->Name);
		p_data->UnitID      = p_old_data2->UnitID;
		p_data->IsPresent   = p_old_data2->IsPresent;
		p_data->UnitPerPack = p_old_data2->UnitPerPack;
		p_data->Price       = MONEYTOLDBL(p_old_data2->Price);
		p_data->QuotKindID  = p_old_data2->QuotKindID;
		p_data->AddPrice1   = p_old_data2->AddPrice1;
		p_data->AddPrice2   = p_old_data2->AddPrice2;
		p_data->AddPrice3   = p_old_data2->AddPrice3;
		p_data->Rest        = p_old_data2->Rest;
		p_data->Expiry      = p_old_data2->Expiry;
		STRNSCPY(p_data->Memo, p_old_data2->Memo);
	}
	return 1;
}
//
//
//
class PPCvtRegister4405 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			if(recsz != sizeof(RegisterTbl::Rec))
				*pNeedConversion = 1;
			else {
				int    num_seg = 0;
				DBIdxSpec * p_is = p_tbl->getIndexSpec(3, &num_seg);
				*pNeedConversion = BIN(p_is && num_seg == 3);
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		pTbl->clearDataBuf();
		SString temp_buf;
		RegisterTbl::Rec tmp_rec;
		memcpy(&tmp_rec, pRec, sizeof(RegisterTbl::Rec));
		tmp_rec.UniqCntr = 0;
		tmp_rec.Flags = 0;
		char   serial_buf[48];
		int    c = 0;
		STRNSCPY(serial_buf, tmp_rec.Serial);
		while(1) {
			RegisterTbl::Key3 k3;
			MEMSZERO(k3);
			k3.RegTypeID = tmp_rec.RegTypeID;
			STRNSCPY(k3.Serial, tmp_rec.Serial);
			STRNSCPY(k3.Num, tmp_rec.Num);
			k3.UniqCntr = 0;
			if(pTbl->search(3, &k3, spEq)) {
				c++;
				temp_buf.Z().Cat(serial_buf).Space().CatChar('#').Cat(c).Strip();
				STRNSCPY(tmp_rec.Serial, temp_buf);
			}
			else {
				pTbl->CopyBufFrom(&tmp_rec, sizeof(tmp_rec));
				break;
			}
		}
		return 1;
	}
};

int Convert4405()
{
	int    ok = 1;
	PPWaitStart();
#if 0 // Перенесено в Convert5200 {
	if(ok) {
		PPCvtGoods4405    gds_cvt;
		ok = gds_cvt.Convert() ? 1 : PPErrorZ();
	}
#endif // } 0
	if(ok) {
		PPCvtQCert4405    qc_cvt;
		ok = qc_cvt.Convert() ? 1 : PPErrorZ();
	}
	if(ok) {
		PPCvtPriceLine4405 pl_cvt;
		ok = pl_cvt.Convert() ? 1 : PPErrorZ();
	}
	if(ok) {
		PPCvtRegister4405 reg_cvt;
		ok = reg_cvt.Convert() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
//
//
//
class PPCvtHistBill4515 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtHistBill4515::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new HistBillTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(HistBillTbl::Rec));
	}
	return p_tbl;
}

int PPCvtHistBill4515::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldRec {
		long   ID;
		long   InnerID;
		long   BillID;
		long   Ver;
		char   Code[24];
		LDATE  Dt;
		long   OprKind;
		long   Location;
		long   Object;
		long   Object2;
		long   CurID;
		double CRate;
		double Amount;
		long   LinkBillID;
		long   Flags;
		long   SCardID;
		long   PayerID;
		long   AgentID;
	} * p_old_data = static_cast<const OldRec *>(pRec);
	HistBillTbl::Rec * p_data = static_cast<HistBillTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(HistBillTbl::Rec));
	p_data->ID = p_old_data->ID;
	p_data->BillID     = p_old_data->BillID;
	p_data->InnerID    = p_old_data->InnerID;
	p_data->Ver        = p_old_data->Ver;
	STRNSCPY(p_data->Code, p_old_data->Code);
	p_data->Dt = p_old_data->Dt;
	p_data->OpID       = p_old_data->OprKind;
	p_data->LocID      = p_old_data->Location;
	p_data->Object     = p_old_data->Object;
	p_data->Object2    = p_old_data->Object2;
	p_data->CurID      = p_old_data->CurID;
	p_data->CRate      = p_old_data->CRate;
	p_data->Amount     = p_old_data->Amount;
	p_data->LinkBillID = p_old_data->LinkBillID;
	p_data->Flags      = p_old_data->Flags;
	p_data->SCardID    = p_old_data->SCardID;
	p_data->PayerID    = p_old_data->PayerID;
	p_data->AgentID    = p_old_data->AgentID;
	memzero(p_data->Reserve, sizeof(p_data->Reserve));
	return 1;
}

CONVERT_PROC(Convert4515, PPCvtHistBill4515);
//
//
//
class PPCvtReceipt477 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ReceiptTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(ReceiptTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		struct OldRec {
			long    ID;
			long    BillID;
			long    Location;
			LDATE   Dt;
			long    OprNo;
			short   Closed;
			long    GoodsID;
			long    QCert;
			double  UnitPerPack;
			double  Quantity;
			char    Cost[8];
			char    Price[8];
			double  Rest;
			long    PrevLot;
			long    Suppl;
			LDATE   CloseDate;
			LDATE   Expiry;
			long    InTaxGrpID;
			long    Flags;
		};
		const OldRec * p_old_rec = static_cast<const OldRec *>(rec);
		ReceiptTbl::Rec * data = static_cast<ReceiptTbl::Rec *>(tbl->getDataBuf());
		memzero(data, sizeof(ReceiptTbl::Rec));
		data->ID      = p_old_rec->ID;
		data->BillID  = p_old_rec->BillID;
		data->LocID   = p_old_rec->Location;
		data->Dt      = p_old_rec->Dt;
		data->OprNo   = p_old_rec->OprNo;
		data->Reserve1 = 0;
		data->Closed  = p_old_rec->Closed;
		data->GoodsID = p_old_rec->GoodsID;
		data->QCertID = p_old_rec->QCert;
		data->UnitPerPack = p_old_rec->UnitPerPack;
		data->Quantity = p_old_rec->Quantity;
		// @v5.1.5 data->Weight   = 0;
		data->WtQtty   = 0; // @v5.1.5
		data->WtRest   = 0; // @v5.1.5
		data->Cost     = MONEYTOLDBL(p_old_rec->Cost);
		data->ExtCost  = 0;
		data->Price    = MONEYTOLDBL(p_old_rec->Price);
		data->Rest     = p_old_rec->Rest;
		data->PrevLotID = p_old_rec->PrevLot;
		data->SupplID   = p_old_rec->Suppl;
		data->CloseDate = p_old_rec->CloseDate;
		data->Expiry    = p_old_rec->Expiry;
		data->InTaxGrpID = p_old_rec->InTaxGrpID;
		data->Flags      = p_old_rec->Flags;
		return 1;
	}
};

int Convert4707()
{
	int    ok = 1;
	int    is_billobj_existed = 0;
	PPWaitStart();
	if(BillObj) {
		ZDELETE(BillObj);
		is_billobj_existed = 1;
	}
	PPCvtReceipt477 cvt;
	PROFILE(ok = BIN(cvt.Convert()));
	if(is_billobj_existed)
		BillObj = new PPObjBill(0);
	PPWaitStop();
	return ok;
}
//
//
//
// AHTOXA {
class PPPriceList4802 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PriceListTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DBIdxSpec * p_is = p_tbl->getIndexSpec(1, &num_seg);
			*pNeedConversion = BIN(p_is && num_seg != 5);
			SAlloc::F(p_is);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		PriceListTbl::Rec * p_data = static_cast<PriceListTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(PriceListTbl::Rec));
		p_data->UserID  = -1;
		return 1;
	}
};

CONVERT_PROC(Convert4802, PPPriceList4802);

// } AHTOXA

class PPCvtDlsObj4805 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new DlsObjTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(DlsObjTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		struct OldRec {
			long    DlsID;
			short   ObjType;
			long    ObjID;
			double  Val;
		};
		OldRec * p_old_rec = (OldRec *)rec;
		DlsObjTbl::Rec * data = (DlsObjTbl::Rec*)tbl->getDataBuf();
		memzero(data, sizeof(DlsObjTbl::Rec));
		data->DlsID   = p_old_rec->DlsID;
		data->ObjType = p_old_rec->ObjType;
		data->ObjID   = p_old_rec->ObjID;
		data->LVal    = 0;
		data->Val     = p_old_rec->Val;
		return 1;
	}
};

CONVERT_PROC(Convert4805, PPCvtDlsObj4805);
#endif // } @v12.4.1
#if 0 // moved to PPCvtBill11112 {
//
// Conversion 4.9.11
// Bill, PayPlan, Transfer
//
class PPCvtBill4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			*pNeedConversion = BIN(recsz < (sizeof(BillTbl::Rec) - sizeof(((BillTbl::Rec*)0)->Memo)));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		struct OldBillRec { // Size = 76+160
			long   ID;       // Ид. документа
			char   Code[10];    // Код документа
			LDATE  Dt;          // Дата документа
			long   BillNo;      // Номер документа за день
			long   OprKind;     // Вид операции          ->Ref(PPOBJ_OPRKIND)
			long   UserID;      // Пользователь          ->Ref(PPOBJ_USR)
			long   Location;    // Позиция               ->Location.ID
			long   Object;      // Контрагент            ->Article.ID
			long   Object2;     // Дополнительный объект ->Article.ID
			long   CurID;       // Валюта (0 - базовая)  ->Ref(PPOBJ_CURRENCY)
			double CRate;       // Курс валюты для пересчета в базовую валюту
			char   Amount[8];   // Номинальная сумма (в единицах CurID)
			long   LinkBillID;  // Связанный документ    ->Bill.ID
			long   Flags;       // Флаги
			int16  AccessLevel; // Уровень доступа к документу
			long   SCardID;     // @v4.1.8 ->SCard.ID
			char   Memo[160];   // Примечание
		} * p_old_rec = (OldBillRec *)rec;

		BillTbl::Rec * p_data = (BillTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();

		p_data->ID      = p_old_rec->ID;
		p_data->Dt      = p_old_rec->Dt;
		p_data->BillNo  = p_old_rec->BillNo;
		p_data->OpID    = p_old_rec->OprKind;
		p_data->UserID  = p_old_rec->UserID;
		p_data->LocID   = p_old_rec->Location;
		p_data->Object  = p_old_rec->Object;
		p_data->Object2 = p_old_rec->Object2;
		p_data->CurID   = p_old_rec->CurID;
		p_data->CRate   = p_old_rec->CRate;
		p_data->Amount  = MONEYTOLDBL(p_old_rec->Amount);
		p_data->LinkBillID = p_old_rec->LinkBillID;
		p_data->Flags   = p_old_rec->Flags;
		p_data->SCardID = p_old_rec->SCardID;
		STRNSCPY(p_data->Code, p_old_rec->Code);
		STRNSCPY(p_data->Memo, p_old_rec->Memo);
		*pNewRecLen = -1;
		return 1;
	}
};
#endif // } 0 moved to PPCvtBill11112
#if 0 // @v12.4.1 {
class PPCvtPayPlan4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PayPlanTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(PayPlanTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		struct OldPayPlanRec { // Size=16
			long   BillID;      // Ид. документа ->Bill.ID
			LDATE  PayDate;     // Дата оплаты
			char   Amount[8];   // Сумма оплаты @v4.9.11 money[8]-->double
		} * p_old_rec = (OldPayPlanRec *)rec;

		PayPlanTbl::Rec * p_data = (PayPlanTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();

		p_data->BillID  = p_old_rec->BillID;
		p_data->PayDate = p_old_rec->PayDate;
		p_data->Amount  = MONEYTOLDBL(p_old_rec->Amount);
		return 1;
	}
};

class PPCvtTransfer4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * needConversion);
	virtual int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * PPCvtTransfer4911::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new TransferTbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(TransferTbl::Rec));
	}
	return tbl;
}

int PPCvtTransfer4911::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	struct OldTransferRec { // Size = 88
		long   Location;    // Позиция                   ->Location.ID
		LDATE  Dt;          // Дата передачи (не обязательно дублирует Bill.Dt)
		long   OprNo;       // Номер операции за день
		long   BillID;      // Ид документа              ->Bill.ID
		int16  RByBill;     // Номер операции по документу
		int16  Reverse;     // Признак зеркальной записи
		long   CorrLoc;     // Корреспондирующая позиция ->Location.ID
		long   LotID;       // Ид приходной записи       ->Receipt.ID
		long   GoodsID;     // Ид товара                 ->Goods.ID
		long   Flags;       // Флаги
		double Quantity;    // Количество товара (Приход +/Расход -/Переоценка 0)
		double Rest;        // Остаток после операции (Location, Lot)
		char   Cost[8];     // Цена поступления //
		char   Price[8];    // Цена реализации  //
		char   Discount[8]; // Скидка
		long   CurID;       // Валюта цены ->Ref(PPOBJ_CURRENCY)
		char   CurPrice[8]; // Цена в валюте CurID (В зависимости от операции
		// это может быть либо цена поступления либо цена реализации).
		// Если CurID == 0, то CurPrice == (Flags & PPTFR_SELLING) ? Cost : (Price-Discount)
	} * p_old_rec = (OldTransferRec *)rec;
	TransferTbl::Rec * p_data = (TransferTbl::Rec *)tbl->getDataBuf();
	tbl->clearDataBuf();

	p_data->LocID   = p_old_rec->Location;
	p_data->Dt      = p_old_rec->Dt;
	p_data->OprNo   = p_old_rec->OprNo;
	p_data->BillID  = p_old_rec->BillID;
	p_data->RByBill = p_old_rec->RByBill;
	p_data->Reverse = p_old_rec->Reverse;
	p_data->CorrLoc = p_old_rec->CorrLoc;
	p_data->LotID   = p_old_rec->LotID;
	p_data->GoodsID = p_old_rec->GoodsID;
	p_data->Flags   = p_old_rec->Flags;
	p_data->Quantity = p_old_rec->Quantity;
	p_data->Rest     = p_old_rec->Rest;
	p_data->Cost     = MONEYTOLDBL(p_old_rec->Cost);
	p_data->Price    = MONEYTOLDBL(p_old_rec->Price);
	p_data->Discount = MONEYTOLDBL(p_old_rec->Discount);
	if(p_data->Flags & PPTFR_QUOT)
		p_data->QuotPrice = MONEYTOLDBL(p_old_rec->CurPrice);
	else
		p_data->CurPrice = MONEYTOLDBL(p_old_rec->CurPrice);
	p_data->CurID    = p_old_rec->CurID;
	return 1;
}

class PPCvtCpTransf4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new CpTransfTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 2);
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		CpTransfTbl::Rec * data = static_cast<CpTransfTbl::Rec *>(tbl->getDataBuf());
		memcpy(data, rec, sizeof(CpTransfTbl::Rec));
		return 1;
	}
};

class PPCvtTSession4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new TSessionTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  flags = 0;
			if(tbl->getTabFlags(&flags)) {
				if(flags & XTF_VLR) {
					int16  num_keys;
					tbl->getNumKeys(&num_keys);
					*pNeedConversion = BIN(num_keys < 9); // @v5.0.2 8-->9
				}
				else
					*pNeedConversion = 1;
			}
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		TSessionTbl::Rec * p_data = (TSessionTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();
		memcpy(p_data, rec, sizeof(TSessionTbl::Rec));
		*pNewRecLen = -1;
		return 1;
	}
};
#endif // } 0 @v12.4.1
//
#if 0 // Перенесено в Convert5810 {

class PPCvtCGoodsLine4911 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * needConversion);
	virtual int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * PPCvtCGoodsLine4911::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new CGoodsLineTbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(CGoodsLineTbl::Rec));
		else {
			*pNeedConversion = 0;
			ZDELETE(tbl);
			PPSetErrorDB();
		}
	}
	return tbl;
}

int PPCvtCGoodsLine4911::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	struct OldCGoodsLineRec { //
		LDATE  Dt;
		long   SessID;      // ->CSession.ID
		long   GoodsID;     // ->Goods.ID
		double Quantity;
		double Rest;
		char   Sum[8];          // @v4.9.15 money-->double
		long   AltGoodsID;      // ->Goods.ID Товар, который может быть списан вместо GoodsID
		float  AltGoodsQtty;    // @v4.4.10 Количество, списанное по альтернативному товару
	} * p_old_rec = (OldCGoodsLineRec *)rec;

	CGoodsLineTbl::Rec * p_data = (CGoodsLineTbl::Rec *)tbl->getDataBuf();
	tbl->clearDataBuf();
	p_data->Dt      = p_old_rec->Dt;
	p_data->Sign    = 1;
	p_data->Reserve = 0;
	p_data->SessID  = p_old_rec->SessID;
	p_data->GoodsID = p_old_rec->GoodsID;
	p_data->Qtty    = p_old_rec->Quantity;
	p_data->Rest    = p_old_rec->Rest;
	p_data->Sum     = MONEYTOLDBL(p_old_rec->Sum);
	p_data->AltGoodsID   = p_old_rec->AltGoodsID;
	p_data->AltGoodsQtty = p_old_rec->AltGoodsQtty;
	return 1;
}

static CONVERT_PROC(_ConvertCGoodsLine4911, PPCvtCGoodsLine4911);

#endif // } 0 Перенесено в Convert5810

// moved to PPCvtBill11112 static CONVERT_PROC(_ConvertBill4911,       PPCvtBill4911);
#if 0 // @v12.4.1 {
static CONVERT_PROC(_ConvertPayPlan4911,    PPCvtPayPlan4911);
static CONVERT_PROC(_ConvertTransfer4911,   PPCvtTransfer4911);
static CONVERT_PROC(_ConvertCpTransf4911,   PPCvtCpTransf4911);
static CONVERT_PROC(_ConvertTSession4911,   PPCvtTSession4911);

int Convert4911()
{
	return (/*(moved to PPCvtBill11112) _ConvertBill4911() &&*/_ConvertPayPlan4911() && _ConvertCpTransf4911() &&
		_ConvertTransfer4911() && _ConvertTSession4911() /*&& _ConvertCGoodsLine4911()*/);
}
#endif // } @v12.4.1
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtPrjTask6202 {
class PPCvtPrjTask5006 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PrjTaskTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 7); // @v5.0.6  6-->7
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		PrjTaskTbl::Rec * p_data = (PrjTaskTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();
		memcpy(p_data, rec, sizeof(PrjTaskTbl::Rec));
		*pNewRecLen = -1;
		return 1;
	}
};

CONVERT_PROC(Convert5006, PPCvtPrjTask5006);
#endif // } 0 @v6.2.2 Moved to PPCvtPrjTask6202

#if 0 // Перенесено в Convert6407 {
// @v5.0.9 AHTOXA {
class PPCvtInventory5009 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new InventoryTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 3); // @v5.0.10  2-->3
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * pRec, int * pNewRecLen)
	{
		const struct OldRec { // size=92
			long   BillID;
			long   OprNo;
			long   GoodsID;
			long   Flags;
			int16  AutoLine;
			int16  WritedOff;
			double UnitPerPack;
			double Quantity;
			double StockRest;
			char   Price[8];
			char   StockPrice[8];
			int16  Reserve1;
			int16  DiffSign;
			double DiffQtty;
			double DiffPctQtty;
			double UnwritedDiff;
			char   Reserve2[4];
		} * p_old_rec = static_cast<const OldRec *>(pRec);
		InventoryTbl::Rec * p_rec = static_cast<InventoryTbl::Rec *>(tbl->getDataBuf());
		tbl->clearDataBuf();
		p_rec->BillID        = p_old_rec->BillID;
		p_rec->OprNo = p_old_rec->OprNo;
		p_rec->GoodsID       = p_old_rec->GoodsID;
		p_rec->Flags = p_old_rec->Flags;
    	INVENT_SETDIFFSIGN(p_rec->Flags, p_old_rec->DiffSign);
		INVENT_SETAUTOLINE(p_rec->Flags, p_old_rec->AutoLine);
		SETFLAG(p_rec->Flags, INVENTF_WRITEDOFF, p_old_rec->WritedOff);
		p_rec->UnitPerPack   = p_old_rec->UnitPerPack;
		p_rec->Quantity      = p_old_rec->Quantity;
		p_rec->StockRest     = p_old_rec->StockRest;
		p_rec->Price = MONEYTOLDBL(p_old_rec->Price);
		p_rec->StockPrice    = MONEYTOLDBL(p_old_rec->StockPrice);
		p_rec->DiffQtty      = p_old_rec->DiffQtty;
		p_rec->DiffPctQtty   = p_old_rec->DiffPctQtty;
		p_rec->UnwritedDiff  = p_old_rec->UnwritedDiff;
		p_rec->CSesDfctQtty  = 0;
		p_rec->CSesDfctPrice = 0;
		p_rec->WrOffPrice    = p_rec->StockPrice;
		return 1;
	}
};

CONVERT_PROC(Convert5009, PPCvtInventory5009);
// } @v5.0.9 AHTOXA
#endif // } 0 Перенесено в Convert6407
//
//
//
#if 0 // @v12.4.1 {
class PPCvtGoodsExt5109 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new GoodsExtTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DBIdxSpec * p_is = p_tbl->getIndexSpec(1, &num_seg);
			*pNeedConversion = BIN(p_is && num_seg < 10);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		GoodsExtTbl::Rec * data = (GoodsExtTbl::Rec*)tbl->getDataBuf();
		memcpy(data, rec, sizeof(GoodsExtTbl::Rec));
		return 1;
	}
};
#endif // } @v12.4.1
#if 0 // @v6.2.2 Moved to PPCvtGoods6202 {
class PPCvtGoods5200 : public PPTableConversion {
public:
	int    RecSizeChanged;

	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		RecSizeChanged = 0;
		DBTable * p_tbl = new Goods2Tbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				if(recsz < sizeof(Goods2Tbl::Rec)) {
					*pNeedConversion = 1;
					RecSizeChanged = 1;
				}
				else {
					*pNeedConversion = 0;
					int16  num_keys;
					p_tbl->getNumKeys(&num_keys);
					*pNeedConversion = BIN(num_keys < 6);
				}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * pNewRecLen)
	{
		Goods2Tbl::Rec * p_data = static_cast<Goods2Tbl::Rec *>(pTbl->getDataBuf());
		if(RecSizeChanged) {
			const struct OldRec {
				long    ID;
				long    Kind;
				char    Name[48];
				char    Abbr[48];
				long    ParentID;
				long    GoodsTypeID;
				long    UnitID;
				long    PhUnitID;
				double  PhUPerU;
				long    ManufID;
				long    StrucID;
				long    TaxGrpID;
				long    WrOffGrpID;
				long    Flags;
				long    GdsClsID;
				long    BrandID;
			} * p_old_rec = static_cast<const OldRec *>(pRec);
			memzero(p_data, sizeof(Goods2Tbl::Rec));
			p_data->ID   = p_old_rec->ID;
			p_data->Kind = p_old_rec->Kind;
			memcpy(p_data->Name, p_old_rec->Name, sizeof(p_old_rec->Name));
			memcpy(p_data->Abbr, p_old_rec->Abbr, sizeof(p_old_rec->Abbr));
			p_data->ParentID    = p_old_rec->ParentID;
			p_data->GoodsTypeID = p_old_rec->GoodsTypeID;
			p_data->UnitID     = p_old_rec->UnitID;
			p_data->PhUnitID   = p_old_rec->PhUnitID;
			p_data->PhUPerU    = p_old_rec->PhUPerU;
			p_data->ManufID    = p_old_rec->ManufID;
			p_data->StrucID    = p_old_rec->StrucID;
			p_data->TaxGrpID   = p_old_rec->TaxGrpID;
			p_data->WrOffGrpID = p_old_rec->WrOffGrpID;
			p_data->Flags      = p_old_rec->Flags;
			p_data->GdsClsID   = p_old_rec->GdsClsID;
			p_data->BrandID    = p_old_rec->BrandID;
		}
		else
			memcpy(p_data, pRec, sizeof(Goods2Tbl::Rec));
		return 1;
	}
};
#endif // } 0 @v6.2.2 Moved to PPCvtGoods6202
//CONVERT_PROC(Convert5109, PPCvtGoodsExt5109);
#if 0 // @v12.4.1 {
int Convert5200()
{
	int    ok = 1;
	PPWaitStart();
	if(ok) {
		PPCvtGoodsExt5109 ge_cvt;
		ok = ge_cvt.Convert() ? 1 : PPErrorZ();
	}
#if 0 // @v6.2.2 Moved to PPCvtGoods6202 {
	if(ok) {
		PPCvtGoods5200 gds_cvt;
		ok = gds_cvt.Convert() ? 1 : PPErrorZ();
	}
#endif // } 0 @v6.2.2 Moved to PPCvtGoods6202
	PPWaitStop();
	return ok;
}
//
// Convert5207
//
class PPCvtCCheckLine5207 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
private:
	int    ver;
};

DBTable * PPCvtCCheckLine5207::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new CCheckLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		if(recsz == 40)
			ver = 2; // 4.1.8
		else if(recsz == 28)
			ver = 1;
		else if(recsz == 32)
			ver = 0;
		else
			ver = 0; // @error
		*pNeedConversion = BIN(ver);
	}
	return p_tbl;
}

int PPCvtCCheckLine5207::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldCCheckLineRec {
		long   CheckID;      // -> CCheck.ID
		long   RByCheck;     // Счетчик строк по чеку
		long   DivID;        // Отдел магазина
		long   GoodsID;      // -> Goods.ID
		double Quantity;     // Количество товара
		char   Price[8];     // Цена
		char   Discount[8];  // Скидка
	} * p_4108_data = static_cast<const OldCCheckLineRec *>(pRec);
	const struct CCheckLineBefore5207 { // Size = 28 /* before @v4.1.8 - 40 bytes */
		long   CheckID;      // ->CCheck.ID
		int16  RByCheck;     // Счетчик строк по чеку
		int16  DivID;        // Отдел магазина
		long   GoodsID;      // ->Goods.ID
		double Quantity;     // Количество товара
		long   Price;        // Цена 0.01
		long   Discount;     // Скидка 0.01
	} * p_5207_data = static_cast<const CCheckLineBefore5207 *>(pRec);
	CCheckLineTbl::Rec * p_data = static_cast<CCheckLineTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(CCheckLineTbl::Rec));
	if(ver == 2) {
		p_data->CheckID  = p_4108_data->CheckID;
		p_data->RByCheck = (short)p_4108_data->RByCheck;
		p_data->DivID    = (short)p_4108_data->DivID;
		p_data->GoodsID  = p_4108_data->GoodsID;
		p_data->Quantity = p_4108_data->Quantity;
		p_data->Price    = dbltointmny(MONEYTOLDBL(p_4108_data->Price));
		p_data->Dscnt    = MONEYTOLDBL(p_4108_data->Discount);
	}
	else if(ver == 1) {
		p_data->CheckID  = p_5207_data->CheckID;
		p_data->RByCheck = p_5207_data->RByCheck;
		p_data->DivID    = p_5207_data->DivID;
		p_data->GoodsID  = p_5207_data->GoodsID;
		p_data->Quantity = p_5207_data->Quantity;
		p_data->Price    = p_5207_data->Price;
		p_data->Dscnt    = intmnytodbl(p_5207_data->Discount);
	}
	else
		*p_data = *static_cast<const CCheckLineTbl::Rec *>(pRec);
	return 1;
}
#endif // } @v12.4.1
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtLocation6202 {

class PPCvtLocation5207 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new LocationTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < 156);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const struct OldLocationRec {
			long   ID;
			long   Counter;
			long   ParentID;
			char   Name[32];
			long   Type;
			long   OwnerID;
			long   Flags;
			long   CityID;
			char   ZIP[12];
			char   Address[48];
			char   Reserve[2];
			long   RspnsPersonID;
			char   Code[10];
		} * p_old_data = static_cast<const OldLocationRec *>(pRec);
		LocationTbl::Rec * p_data = static_cast<LocationTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(LocationTbl::Rec));
	#define CPY_FLD(Fld) p_data->Fld=p_old_data->Fld
		CPY_FLD(ID);
		CPY_FLD(Counter);
		CPY_FLD(ParentID);
		CPY_FLD(Type);
		CPY_FLD(OwnerID);
		CPY_FLD(Flags);
		CPY_FLD(CityID);
		CPY_FLD(RspnsPersonID);
	#undef CPY_FLD
		STRNSCPY(p_data->Name, p_old_data->Name);
		STRNSCPY(p_data->ZIP, p_old_data->ZIP);
		STRNSCPY(p_data->Code, p_old_data->Code);
		STRNSCPY(p_data->Address, p_old_data->Address);
		return 1;
	}
};

#endif // } 0 @v6.2.2 Moved to PPCvtLocation6202
#if 0 // @v12.4.1 {
int Convert5207()
{
	int    ok = 1;
	PPWaitStart();
	{
		ok = PPObjWorld::Convert() ? 1 : PPErrorZ();
		if(!ok)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	}
	{
		PROFILE_START
		PPCvtCCheckLine5207 cvt_ccl;
		ok = cvt_ccl.Convert() ? 1 : PPErrorZ();
		if(!ok)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
		PROFILE_END
	}
	/* @v6.2.2 Moved to PPCvtLocation6202
	{
		PPCvtLocation5207 cvt_loc;
		ok = cvt_loc.Convert() ? 1 : PPErrorZ();
		if(!ok)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	}
	*/
	PPWaitStop();
	return ok;
}
#endif // } 0 @v12.4.1
//
//
//
#if 0 // {

class PPCvtBarcode5305 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtBarcode5305::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new BarcodeTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz == 32); // Новый размер =28 bytes
	}
	return p_tbl;
}

int PPCvtBarcode5305::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldBarcodeRec {
		long   GoodsID;     // Ид товара
		double Qtty;        // Количество единиц товара в упаковке
		long   BarcodeType; // Тип кодировки
		char   Code[16];    // Штрихкод
	} * p_old_data = static_cast<const OldBarcodeRec *>(pRec);
	BarcodeTbl::Rec * p_data = static_cast<BarcodeTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(LocationTbl::Rec));
	p_data->GoodsID = p_old_data->GoodsID;
	p_data->Pack    = (long)(p_old_data->Qtty * 1000L);
	p_data->ArID    = 0;
	memcpy(p_data->Code, p_old_data->Code, sizeof(p_data->Code));
	return 1;
}

#endif // } 0
//
//
//
#if 0 // @v12.4.1 {
class PPCvtStaffPost5501 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PersonPostTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz > sizeof(PersonPostTbl::Rec)); // Новый размер меньше предыдущего
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * newTbl, void * pRec, int * pNewRecLen)
	{
		const struct OldRec {
			long    ID;
			char    Code[12];
			long    PostID;
			long    PersonID;
			LDATE   Dt;
			char    Name[48];
			long    OrgID;
			long    DivID;
		} * p_old_rec = static_cast<const OldRec *>(pRec);
		PersonPostTbl::Rec * p_data = static_cast<PersonPostTbl::Rec *>(newTbl->getDataBuf());
		newTbl->clearDataBuf();
		p_data->ID = p_old_rec->ID;
		STRNSCPY(p_data->Code, p_old_rec->Code);
		p_data->StaffID  = p_old_rec->PostID;
		p_data->PersonID = p_old_rec->PersonID;
		p_data->Dt       = p_old_rec->Dt;
		p_data->Flags |= 0x10000000L; // Список сумм (если есть) в старом формате
		*pNewRecLen = sizeof(PersonPostTbl::Rec);
		return 1;
	}
};

class PPCvtStaffCal5501 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new StaffCalendarTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DBIdxSpec * p_is = p_tbl->getIndexSpec(0, &num_seg);
			*pNeedConversion = BIN(p_is && num_seg == 2);
			SAlloc::F(p_is);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * pNewRecLen)
	{
		StaffCalendarTbl::Rec * p_data = static_cast<StaffCalendarTbl::Rec *>(pTbl->getDataBuf());
		memcpy(p_data, pRec, sizeof(StaffCalendarTbl::Rec));
		return 1;
	}
};

int Convert5501()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtStaffPost5501 cvt;
		ok = cvt.Convert() ? 1 : PPErrorZ();
		if(!ok)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	}
	{
		PPCvtStaffCal5501 cvt;
		ok = cvt.Convert() ? 1 : PPErrorZ();
		if(!ok)
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	}
	PPWaitStop();
	return ok;
}
#endif // } 0 @v12.4.1
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtProp6202 {

class PPCvtProperty5506 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * needConversion);
	int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * PPCvtProperty5506::CreateTableInstance(int * needConversion)
{
	struct __PPCustDisp {     // @v4.7.6
		PPID   Tag;           // Const = PPOBJ_CASHNODE
		PPID   CashNodeID;    // ИД кассового узла
		PPID   Prop;          // Const = CNPRP_CUSTDISP
		long   Type;          // Тип дисплея покупателя //
		char   Port[8];       // Порт (COM)
		char   Reserve[52];
		long   Reserve2[2];
	};

	struct __PPTouchScreen {  // @v5.1.2
		PPID   Tag;           // Const = PPOBJ_CASHNODE
		PPID   CashNodeID;    // ИД кассового узла
		PPID   Prop;          // Const = CNPRP_TOUCHSCREEN
		PPID   TouchScreenID; // ИД TouchScreen
		char   Reserve[60];
		long   Reserve2[2];
	};

	struct __PPExtDevices {   // @v5.5.6
		PPID   Tag;           // Const = PPOBJ_CASHNODE
		PPID   CashNodeID;    // ИД кассового узла
		PPID   Prop;          // Const = CNPRP_EXTDEVICES
		PPID   TouchScreenID; // ИД TouchScreen
		PPID   ExtCashNodeID; // ИД дополнительного кассового узла
		long   CustDispType;  // Тип дисплея покупателя //
		char   CustDispPort[8]; // Порт дисплея покупателя (COM)
		char   Reserve[44];
		long   Reserve2[2];
	};

	int   ta = 0;
	PPID  cn_id;
	PPCashNode  cn_rec;
	Reference * p_ref = 0;
	THROW_MEM(p_ref = new Reference);
	THROW(PPStartTransaction(&ta, 1));
	for(cn_id = 0; p_ref->EnumItems(PPOBJ_CASHNODE, &cn_id, &cn_rec) > 0;)
		if(cn_rec.Flags & CASHF_SYNC)
			if(p_ref->GetProp(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES) > 0)
				break;
			else {
				__PPCustDisp    cd;
				__PPTouchScreen ts;
				int  is_cd = BIN(p_ref->GetProp(PPOBJ_CASHNODE, cn_id, CNPRP_CUSTDISP, &cd, sizeof(cd)) > 0);
				int  is_ts = BIN(p_ref->GetProp(PPOBJ_CASHNODE, cn_id, CNPRP_TOUCHSCREEN, &ts, sizeof(ts)) > 0);
				if(is_cd || is_ts) {
					__PPExtDevices  ed;
					MEMSZERO(ed);
					if(is_cd) {
						ed.CustDispType = cd.Type;
						STRNSCPY(ed.CustDispPort, cd.Port);
						THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_CUSTDISP, 0));
					}
					if(is_ts) {
						ed.TouchScreenID = ts.TouchScreenID;
						THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_TOUCHSCREEN, 0));
					}
					THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES, &ed));
				}
			}
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
	ENDCATCH
	ASSIGN_PTR(needConversion, 0);
	delete p_ref;
	return 0;
}

int PPCvtProperty5506::ConvertRec(DBTable * /*tbl*/, void * /*rec*/, int * /*pNewRecLen*/)
{
	return -1;
}

int Convert5506()
{
	int    ok = 1;
	PPWaitStart();
	PPCvtProperty5506 cvt;
	ok = cvt.Convert() ? 1 : PPErrorZ();
	if(!ok)
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	PPWaitStop();
	return ok;
}
#endif // } 0 @v6.2.2 Moved to PPCvtProp6202
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtWorld6202 {
class PPCvtWorld5512 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new WorldTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys; // DBTable
			tbl->getNumKeys(&num_keys);
			if(num_keys < 5)
				*pNeedConversion = 1;
			else {
				int    num_seg = 0;
				DBIdxSpec * p_is = tbl->getIndexSpec(4, &num_seg);
				if(!(p_is->flags & XIF_MOD))
					*pNeedConversion = 1;
				else
					*pNeedConversion = 0;
				delete p_is;
			}
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		WorldTbl::Rec * p_data = (WorldTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();
		memcpy(p_data, rec, sizeof(WorldTbl::Rec));
		*pNewRecLen = -1;
		return 1;
	}
};

CONVERT_PROC(Convert5512, PPCvtWorld5512);
#endif // } 0 @v6.2.2 Moved to PPCvtWorld6202
//
//
//
#if 0 // @v12.4.1 {
class PPCvtObjSync5608 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ObjSyncTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			if(recsz > sizeof(ObjSyncTbl::Rec))
				*pNeedConversion = 1;
			else
				*pNeedConversion = 0;
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		struct OldObjSyncRec {
			int32  ObjType;
			int32  ObjID;
			int32  CommonID;
			int32  DBID;
			LDATE  Dt;
			LTIME  Tm;
			int16  Deleted;
			int32  Flags;
		};
		ObjSyncTbl::Rec * p_data = static_cast<ObjSyncTbl::Rec *>(tbl->getDataBuf());
		tbl->clearDataBuf();
		const OldObjSyncRec * p_old_rec = static_cast<const OldObjSyncRec *>(rec);
		if(p_old_rec->DBID > 0) {
			p_data->DBID      = (short)p_old_rec->DBID;
			p_data->ObjType   = (short)p_old_rec->ObjType;
			p_data->ObjID     = p_old_rec->ObjID;
			p_data->CommIdPfx = (short)(p_old_rec->CommonID >> 24);
			p_data->CommID    = (p_old_rec->CommonID & 0x00ffffff);
			p_data->Flags     = 0;
			p_data->Dt        = p_old_rec->Dt;
			p_data->Tm        = p_old_rec->Tm;
			return 1;
		}
		else {
			SString fmt_buf, msg_buf;
			Logger.Log(msg_buf.Printf(PPLoadTextS(PPTXT_LOG_INVCONVERTEDREC, fmt_buf), "DBID=0"));
			return -1;
		}
	}
};

CONVERT_PROC(Convert5608, PPCvtObjSync5608);
//
//
//
#define IMPL_REF2CVT_FUNC(rec) void ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\
	STATIC_ASSERT(sizeof(rRec) == sizeof(Reference_ObsoleteTbl::Rec)); \
	STATIC_ASSERT(sizeof(rRec2) == sizeof(Reference2Tbl::Rec)); \
	MEMSZERO(rRec2); \
	rRec2.Tag = rRec.Tag; \
	rRec2.ID = rRec.ID; \
	STRNSCPY(rRec2.Name, rRec.Name);

#define IMPL_REF2CVT_FUNC_NA(rec) void ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\
	MEMSZERO(rRec2); \
	rRec2.Tag = rRec.Tag; \
	rRec2.ID = rRec.ID; \
	STRNSCPY(rRec2.Name, rRec.Name);

#define REF2CVT_ASSIGN(f) rRec2.f = rRec.f

IMPL_REF2CVT_FUNC(PPObjectTag) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LinkObjGrp);
	REF2CVT_ASSIGN(TagEnumID);
	REF2CVT_ASSIGN(TagDataType);
	REF2CVT_ASSIGN(ObjTypeID);
	REF2CVT_ASSIGN(TagGroupID);
}

IMPL_REF2CVT_FUNC(PPSecur) // {
	int    ok = 1;
	SString temp_buf;
	//
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(PwUpdate);
	REF2CVT_ASSIGN(ParentID);
	REF2CVT_ASSIGN(PersonID);
	//
	THROW(Reference::Decrypt(Reference::crymDefault, rRec.Password, sizeof(rRec.Password), temp_buf));
	THROW(Reference::Encrypt(Reference::crymRef2, temp_buf, rRec2.Password, sizeof(rRec2.Password)));
	Reference::VerifySecur(&rRec2, 1);
	CATCHZOK
}

IMPL_REF2CVT_FUNC(PPBarcodeStruc) // {
	STRNSCPY(rRec2.Templ, rRec.Templ);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPUnit) // {
	STRNSCPY(rRec2.Abbr, rRec.Abbr);
	REF2CVT_ASSIGN(BaseUnitID);
	REF2CVT_ASSIGN(BaseRatio);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPNamedObjAssoc) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(ScndObjGrp);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(PrmrObjType);
	REF2CVT_ASSIGN(ScndObjType);
}

IMPL_REF2CVT_FUNC(PPPersonKind) // {
	REF2CVT_ASSIGN(CodeRegTypeID);
	REF2CVT_ASSIGN(FolderRegTypeID);
}

IMPL_REF2CVT_FUNC(PPPersonStatus) // {
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPELinkKind) // {
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Type);
}

IMPL_REF2CVT_FUNC(PPCurrency) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Code);
}

IMPL_REF2CVT_FUNC(PPCurRateType) // {
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPAmountType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Tax);
	REF2CVT_ASSIGN(TaxRate);
}

IMPL_REF2CVT_FUNC(PPOprType) // {
	STRNSCPY(rRec2.Pict, rRec.Pict);
}

IMPL_REF2CVT_FUNC(PPOpCounter) // {
	STRNSCPY(rRec2.CodeTemplate, rRec.CodeTemplate);
	REF2CVT_ASSIGN(ObjType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Counter);
	REF2CVT_ASSIGN(OwnerObjID);
}

IMPL_REF2CVT_FUNC(PPGdsCls) // {
	REF2CVT_ASSIGN(DefGrpID);
	REF2CVT_ASSIGN(DefUnitID);
	REF2CVT_ASSIGN(DefPhUnitID);
	REF2CVT_ASSIGN(DefPhUPerU);
	REF2CVT_ASSIGN(DefTaxGrpID);
	REF2CVT_ASSIGN(DefGoodsTypeID);
	REF2CVT_ASSIGN(EditDlgID);
	REF2CVT_ASSIGN(FiltDlgID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(DynGenMask);
}

IMPL_REF2CVT_FUNC(PPAssetWrOffGrp) // {
	STRNSCPY(rRec2.Code, rRec.Code);
	REF2CVT_ASSIGN(WrOffType);
	REF2CVT_ASSIGN(WrOffTerm);
	REF2CVT_ASSIGN(Limit);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPOprKind) // {
	REF2CVT_ASSIGN(Rank);
	REF2CVT_ASSIGN(LinkOpID);
	REF2CVT_ASSIGN(AccSheet2ID);
	REF2CVT_ASSIGN(OpCounterID);
	REF2CVT_ASSIGN(PrnFlags);
	REF2CVT_ASSIGN(DefLocID);
	REF2CVT_ASSIGN(PrnOrder);
	REF2CVT_ASSIGN(SubType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(OpTypeID);
	REF2CVT_ASSIGN(AccSheetID);
}

IMPL_REF2CVT_FUNC(PPBillStatus) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Rank);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPAccSheet) // {
	REF2CVT_ASSIGN(BinArID);
	REF2CVT_ASSIGN(CodeRegTypeID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Assoc);
	REF2CVT_ASSIGN(ObjGroup);
}

IMPL_REF2CVT_FUNC(PPCashNode) // {
	memcpy(rRec2.Port, rRec.Port, sizeof(rRec2.Port));
	REF2CVT_ASSIGN(GoodsLocAssocID);
	REF2CVT_ASSIGN(SleepTimeout);
	REF2CVT_ASSIGN(CurRestBillID);
	// @v6.6.4 memcpy(rRec2.Accum, rRec.Accum, sizeof(rRec2.Accum));
	rRec2.ParentID = 0;
	// @v7.2.12 rRec2.Reserve4 = 0; // @v6.6.4
	rRec2.GoodsGrpID = 0; // @v7.2.12
	REF2CVT_ASSIGN(DownBill);
	REF2CVT_ASSIGN(CashType);
	REF2CVT_ASSIGN(LogNum);
	REF2CVT_ASSIGN(DrvVerMajor);
	REF2CVT_ASSIGN(DrvVerMinor);
	REF2CVT_ASSIGN(CurSessID);
	REF2CVT_ASSIGN(ExtQuotID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LocID);
	REF2CVT_ASSIGN(CurDate);
}

IMPL_REF2CVT_FUNC(PPLocPrinter) // {
	STRNSCPY(rRec2.Port, rRec.Port);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LocID);
}

IMPL_REF2CVT_FUNC(PPStyloPalm) // {
	// @v8.6.8 REF2CVT_ASSIGN(LocID);
	rRec2.ObsoleteLocID = rRec.LocID; // @v8.6.8
	REF2CVT_ASSIGN(GoodsGrpID);
	REF2CVT_ASSIGN(OrderOpID);
	REF2CVT_ASSIGN(AgentID);
	REF2CVT_ASSIGN(GroupID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(FTPAcctID);
}

IMPL_REF2CVT_FUNC(PPTouchScreen) // {
	STRNSCPY(rRec2.GdsListFontName, rRec.GdsListFontName);
	REF2CVT_ASSIGN(TouchScreenType);
	REF2CVT_ASSIGN(AltGdsGrpID);
	REF2CVT_ASSIGN(GdsListFontHight);
	REF2CVT_ASSIGN(GdsListEntryGap);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC_NA(PPBarcodePrinter) // {
	STRNSCPY(rRec2.Port, rRec.Port);
	STRNSCPY(rRec2.LabelName, rRec.LabelName);
	REF2CVT_ASSIGN(Cpp);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(PrinterType);
}

IMPL_REF2CVT_FUNC_NA(PPInternetAccount) // {
	REF2CVT_ASSIGN(SmtpAuthType);
	REF2CVT_ASSIGN(Timeout);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPDBDiv) // {
	STRNSCPY(rRec2.Addr, rRec.Addr);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(IntrRcptOpr);
	REF2CVT_ASSIGN(OutCounter);
}

IMPL_REF2CVT_FUNC(PPGoodsType) // {
	REF2CVT_ASSIGN(WrOffGrpID);
	REF2CVT_ASSIGN(AmtCVat);
	REF2CVT_ASSIGN(AmtCost);
	REF2CVT_ASSIGN(AmtPrice);
	REF2CVT_ASSIGN(AmtDscnt);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPGoodsStrucHeader) // {
	REF2CVT_ASSIGN(VariedPropObjType);
	REF2CVT_ASSIGN(Period);
	REF2CVT_ASSIGN(CommDenom);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(ParentID);
}

IMPL_REF2CVT_FUNC(PPGoodsTax) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(VAT);
	REF2CVT_ASSIGN(Excise);
	REF2CVT_ASSIGN(SalesTax);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Order);
	REF2CVT_ASSIGN(UnionVect);
}

IMPL_REF2CVT_FUNC(PPRegisterType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(PersonKindID);
	REF2CVT_ASSIGN(RegOrgKind);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(CounterID);
}

IMPL_REF2CVT_FUNC(PPQuotKind) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Discount);
	REF2CVT_ASSIGN(Period);
	REF2CVT_ASSIGN(BeginTm);
	REF2CVT_ASSIGN(EndTm);
	REF2CVT_ASSIGN(Rank);
	REF2CVT_ASSIGN(DaysOfWeek);
	REF2CVT_ASSIGN(UsingWSCard);
	REF2CVT_ASSIGN(OpID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(AccSheetID);
}

IMPL_REF2CVT_FUNC(PPPsnOpKind) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(PairOp);
	REF2CVT_ASSIGN(RegTypeID);
	REF2CVT_ASSIGN(ExValGrp);
	REF2CVT_ASSIGN(ExValSrc);
	REF2CVT_ASSIGN(PairType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LinkBillOpID);
}

IMPL_REF2CVT_FUNC(PPWorldObjStatus) // {
	STRNSCPY(rRec2.Abbr, rRec.Abbr);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Kind);
	REF2CVT_ASSIGN(Code);
}

IMPL_REF2CVT_FUNC(PPPersonRelType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(StatusRestriction);
	REF2CVT_ASSIGN(Cardinality);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPSalCharge) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(AmtID);
	REF2CVT_ASSIGN(CalID);
	REF2CVT_ASSIGN(WrOffOpID);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPDateTimeRep) // {
	REF2CVT_ASSIGN(Dtr);
	REF2CVT_ASSIGN(Duration);
}

IMPL_REF2CVT_FUNC(PPDutySched) // {
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(ObjType);
	REF2CVT_ASSIGN(ObjGroup);
}

IMPL_REF2CVT_FUNC(PPStaffCal) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(PersonKind);
	REF2CVT_ASSIGN(SubstCalID);
	REF2CVT_ASSIGN(LinkObjType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LinkCalID);
	REF2CVT_ASSIGN(LinkObjID);
}

IMPL_REF2CVT_FUNC(PPScale) // {
	// @v10.5.7 Поле Port перемещено из основной записи в "хвост".
	// Таким образом, эта конвертация не сможет правильно преобразовать запись. Но это - не проблема, ибо, скорее всего
	// данная конвертация уже на актуальна за давностью лет.
	// memcpy(rRec2.Port, rRec.Port, sizeof(rRec.Port));
	//
	REF2CVT_ASSIGN(Get_NumTries);
	REF2CVT_ASSIGN(Get_Delay);
	REF2CVT_ASSIGN(Put_NumTries);
	REF2CVT_ASSIGN(Put_Delay);
	REF2CVT_ASSIGN(QuotKindID);
	REF2CVT_ASSIGN(ScaleTypeID);
	REF2CVT_ASSIGN(ProtocolVer);
	REF2CVT_ASSIGN(LogNum);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Location);
	REF2CVT_ASSIGN(AltGoodsGrp);
}

IMPL_REF2CVT_FUNC(PPBhtTerminal) // {
	memcpy(rRec2.Port, rRec.Port, sizeof(rRec.Port));
	REF2CVT_ASSIGN(ReceiptPlace);
	REF2CVT_ASSIGN(ComGet_NumTries);
	REF2CVT_ASSIGN(ComGet_Delay);
	REF2CVT_ASSIGN(ComPut_NumTries);
	REF2CVT_ASSIGN(ComPut_Delay);
	REF2CVT_ASSIGN(IntrExpndOpID);
	REF2CVT_ASSIGN(InventOpID);
	REF2CVT_ASSIGN(Cbr);
	REF2CVT_ASSIGN(BhtpTimeout);
	REF2CVT_ASSIGN(BhtpMaxTries);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(BhtTypeID);
	REF2CVT_ASSIGN(ExpendOpID);
}

IMPL_REF2CVT_FUNC(PPSCardSeries) // {
	// @v9.8.9 STRNSCPY(rRec2.CodeTempl, rRec.CodeTempl);
	rRec2.QuotKindID_s = rRec.QuotKindID;
	REF2CVT_ASSIGN(PersonKindID);
	REF2CVT_ASSIGN(PDis);
	REF2CVT_ASSIGN(MaxCredit);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Issue);
	REF2CVT_ASSIGN(Expiry);
}

IMPL_REF2CVT_FUNC(PPDraftWrOff) // {
	REF2CVT_ASSIGN(PoolOpID);
	REF2CVT_ASSIGN(DfctCompensOpID);
	REF2CVT_ASSIGN(DfctCompensArID);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPAdvBillKind) // {
	REF2CVT_ASSIGN(LinkOpID);
	REF2CVT_ASSIGN(Flags);
}

IMPL_REF2CVT_FUNC(PPGoodsBasket) // {
	REF2CVT_ASSIGN(Num);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(User);
	REF2CVT_ASSIGN(SupplID);
}

IMPL_REF2CVT_FUNC(PPDraftCreateRule) // {
	REF2CVT_ASSIGN(OpID);
	REF2CVT_ASSIGN(ArID);
	REF2CVT_ASSIGN(AgentID);
	REF2CVT_ASSIGN(GoodsGrpID);
	REF2CVT_ASSIGN(CQuot);
	REF2CVT_ASSIGN(PQuot);
	REF2CVT_ASSIGN(CostAlg);
	REF2CVT_ASSIGN(PriceAlg);
	REF2CVT_ASSIGN(CPctVal);
	REF2CVT_ASSIGN(PPctVal);
	REF2CVT_ASSIGN(MaxSum);
	REF2CVT_ASSIGN(MaxPos);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(ParentID);
}

//#define IMPL_REF2CVT_FUNC(rec) int ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\

#define CONVERT_REF2_OBJ(obj, rec) case obj: ConvertRef(*reinterpret_cast<const rec##_ *>(&ref.data), *reinterpret_cast<rec##2 *>(&ref2.data)); break

static int ConvertRef2()
{
	int    ok = 1, ta = 0, r;
	RECORDNUMBER numrec2 = 0;
	Reference2Tbl ref2;
	ref2.getNumRecs(&numrec2);
	if(numrec2 == 0) {
		Reference_ObsoleteTbl ref;
		Reference_ObsoleteTbl::Key0 k0;
		MEMSZERO(k0);
		THROW(PPStartTransaction(&ta, 1));
		if((r = ref.search(0, &k0, spFirst)) != 0)
			do {
				ref2.clearDataBuf();
				switch(ref.data.ObjType) {
					CONVERT_REF2_OBJ(PPOBJ_TAG, PPObjectTag);
					CONVERT_REF2_OBJ(PPOBJ_CONFIG, PPSecur);
					CONVERT_REF2_OBJ(PPOBJ_USR, PPSecur);
					CONVERT_REF2_OBJ(PPOBJ_USRGRP, PPSecur);
					CONVERT_REF2_OBJ(PPOBJ_BCODESTRUC, PPBarcodeStruc);
					CONVERT_REF2_OBJ(PPOBJ_UNIT, PPUnit);
					CONVERT_REF2_OBJ(PPOBJ_NAMEDOBJASSOC, PPNamedObjAssoc);
					CONVERT_REF2_OBJ(PPOBJ_PERSONKIND,   PPPersonKind);
					CONVERT_REF2_OBJ(PPOBJ_PRSNSTATUS, PPPersonStatus);
					CONVERT_REF2_OBJ(PPOBJ_ELINKKIND, PPELinkKind);
					CONVERT_REF2_OBJ(PPOBJ_CURRENCY, PPCurrency);
					CONVERT_REF2_OBJ(PPOBJ_CURRATETYPE, PPCurRateType);
					CONVERT_REF2_OBJ(PPOBJ_AMOUNTTYPE, PPAmountType);
					CONVERT_REF2_OBJ(PPOBJ_OPRTYPE, PPOprType);
					CONVERT_REF2_OBJ(PPOBJ_OPCOUNTER, PPOpCounter);
					CONVERT_REF2_OBJ(PPOBJ_GOODSCLASS, PPGdsCls);
					CONVERT_REF2_OBJ(PPOBJ_ASSTWROFFGRP, PPAssetWrOffGrp);
					CONVERT_REF2_OBJ(PPOBJ_OPRKIND, PPOprKind);
					CONVERT_REF2_OBJ(PPOBJ_BILLSTATUS, PPBillStatus);
					CONVERT_REF2_OBJ(PPOBJ_ACCSHEET, PPAccSheet);
					CONVERT_REF2_OBJ(PPOBJ_CASHNODE, PPCashNode);
					CONVERT_REF2_OBJ(PPOBJ_LOCPRINTER, PPLocPrinter);
					CONVERT_REF2_OBJ(PPOBJ_STYLOPALM, PPStyloPalm);
					CONVERT_REF2_OBJ(PPOBJ_TOUCHSCREEN, PPTouchScreen);
					CONVERT_REF2_OBJ(PPOBJ_DBDIV, PPDBDiv);
					CONVERT_REF2_OBJ(PPOBJ_GOODSTYPE, PPGoodsType);
					CONVERT_REF2_OBJ(PPOBJ_GOODSSTRUC, PPGoodsStrucHeader);
					CONVERT_REF2_OBJ(PPOBJ_GOODSTAX,   PPGoodsTax);
					CONVERT_REF2_OBJ(PPOBJ_REGISTERTYPE, PPRegisterType);
					CONVERT_REF2_OBJ(PPOBJ_QUOTKIND, PPQuotKind);
					CONVERT_REF2_OBJ(PPOBJ_PERSONOPKIND, PPPsnOpKind);
					CONVERT_REF2_OBJ(PPOBJ_CITYSTATUS, PPWorldObjStatus);
					CONVERT_REF2_OBJ(PPOBJ_PERSONRELTYPE, PPPersonRelType);
					CONVERT_REF2_OBJ(PPOBJ_SALCHARGE, PPSalCharge);
					CONVERT_REF2_OBJ(PPOBJ_DATETIMEREP, PPDateTimeRep);
					CONVERT_REF2_OBJ(PPOBJ_DUTYSCHED, PPDutySched);
					CONVERT_REF2_OBJ(PPOBJ_STAFFCAL, PPStaffCal);
					CONVERT_REF2_OBJ(PPOBJ_SCALE, PPScale);
					CONVERT_REF2_OBJ(PPOBJ_BHT, PPBhtTerminal);
					CONVERT_REF2_OBJ(PPOBJ_SCARDSERIES, PPSCardSeries);
					CONVERT_REF2_OBJ(PPOBJ_DRAFTWROFF, PPDraftWrOff);
					CONVERT_REF2_OBJ(PPOBJ_ADVBILLKIND, PPAdvBillKind);
					CONVERT_REF2_OBJ(PPOBJ_GOODSBASKET, PPGoodsBasket);
					CONVERT_REF2_OBJ(PPOBJ_DFCREATERULE, PPDraftCreateRule);
					CONVERT_REF2_OBJ(PPOBJ_BCODEPRINTER, PPBarcodePrinter); // @!         // PPBarcodePrinter_
					CONVERT_REF2_OBJ(PPOBJ_INTERNETACCOUNT, PPInternetAccount); // @!     // PPInternetAccount_
					case PPOBJ_UNASSIGNED:
						{
							SString temp_buf;
							ref2.data.ObjType = PPOBJ_UNASSIGNED;
							ref2.data.ObjID   = ref.data.ObjID;
							ref2.data.Val1    = ref.data.Val1;
							ref2.data.Val2    = ref.data.Val2;
							memcpy(ref2.data.ObjName, ref.data.ObjName, sizeof(ref2.data.ObjName));
							THROW(Reference::Decrypt(Reference::crymDefault, ref.data.AltText, sizeof(ref.data.AltText), temp_buf));
							THROW(Reference::Encrypt(Reference::crymRef2, temp_buf, ref2.data.Symb, sizeof(ref2.data.Symb)));
						}
						break;
					default:
						ref2.data.ObjType = ref.data.ObjType;
						ref2.data.ObjID   = ref.data.ObjID;
						STRNSCPY(ref2.data.ObjName, ref.data.ObjName);
						STRNSCPY(ref2.data.Symb, ref.data.AltText);
						ref2.data.Val1    = ref.data.Val1;
						ref2.data.Val2    = ref.data.Val2;
						memcpy(ref2.data.ExtData+sizeof(ref2.data.ExtData)-sizeof(ref.data.Flags), &ref.data.Flags, sizeof(ref.data.Flags));
						break;
				}
				THROW_DB(ref2.insertRec());
			} while((r = ref.search(0, &k0, spNext)) != 0);
		THROW_DB(BTROKORNFOUND);
		THROW(PPCommitWork(&ta));
	}
	else
		ok = -1;
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	return ok;
}

//CONVERT_PROC(Convert5608, PPCvtObjSync5608);

class PPCvtCGoodsLine5810 : public PPTableConversion {
public:
	PPCvtCGoodsLine5810() : PPTableConversion(), C4911(-1)
	{
	}
	virtual DBTable * CreateTableInstance(int * needConversion);
	virtual int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
private:
	int    C4911; // Если !0, то исходный файл находится в состоянии, предшествующем v4.9.11
};

DBTable * PPCvtCGoodsLine5810::CreateTableInstance(int * pNeedConversion)
{
	if(C4911 < 0)
		C4911 = 0;
	DBTable * tbl = new CGoodsLineTbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = tbl->getRecSize();
		if(recsz < sizeof(CGoodsLineTbl::Rec)) {
			C4911 = BIN(recsz < 48);
			*pNeedConversion = 1;
		}
		else
			*pNeedConversion = 0;
	}
	return tbl;
}

int PPCvtCGoodsLine5810::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	const struct OldCGoodsLineRec4911 { //
		LDATE  Dt;
		long   SessID;
		long   GoodsID;
		double Qtty;
		double Rest;
		char   Sum[8];
		long   AltGoodsID;
		float  AltGoodsQtty;
	} * p_old_rec_4911 = static_cast<const OldCGoodsLineRec4911 *>(rec);
	const struct OldCGoodsLineRec { //
		LDATE  Dt;
		int16  Sign;
		int16  Flags;
		long   SessID;
		long   GoodsID;
		double Qtty;
		double Rest;
		double Sum;
		long   AltGoodsID;
		float  AltGoodsQtty;
	} * p_old_rec = static_cast<const OldCGoodsLineRec *>(rec);
	CGoodsLineTbl::Rec * p_data = static_cast<CGoodsLineTbl::Rec *>(tbl->getDataBuf());
	tbl->clearDataBuf();
	if(C4911 > 0) {
#define FLD_ASSIGN(f) p_data->f = p_old_rec_4911->f
		FLD_ASSIGN(Dt);
		p_data->Sign    = 1;
		p_data->Flags   = 0;
		FLD_ASSIGN(SessID);
		FLD_ASSIGN(GoodsID);
		p_data->SerialID = 0;
		FLD_ASSIGN(Qtty);
		FLD_ASSIGN(Rest);
		p_data->Sum = MONEYTOLDBL(p_old_rec_4911->Sum);
		FLD_ASSIGN(AltGoodsID);
		FLD_ASSIGN(AltGoodsQtty);
#undef FLD_ASSIGN
	}
	else {
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(Dt);
		FLD_ASSIGN(Sign);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(SessID);
		FLD_ASSIGN(GoodsID);
		p_data->SerialID = 0;
		FLD_ASSIGN(Qtty);
		FLD_ASSIGN(Rest);
		FLD_ASSIGN(Sum);
		FLD_ASSIGN(AltGoodsID);
		FLD_ASSIGN(AltGoodsQtty);
#undef FLD_ASSIGN
	}
	return 1;
}

class PPCvtBankAccount5810 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * needConversion);
	virtual int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * PPCvtBankAccount5810::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new BankAccount_Pre9004Tbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(BankAccount_Pre9004Tbl::Rec));
	}
	return tbl;
}

int PPCvtBankAccount5810::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	const struct OldBankAccountRec {
		long   ID;
		long   PersonID;
		long   BankID;
		long   AccType;
		char   Acct[24];
		LDATE  OpenDate;
		long   Flags;
		long   CorrAcc;
		long   CorrArt;
	} * p_old_rec = static_cast<const OldBankAccountRec *>(rec);
	BankAccount_Pre9004Tbl::Rec * p_data = static_cast<BankAccount_Pre9004Tbl::Rec *>(tbl->getDataBuf());
	tbl->clearDataBuf();
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
	FLD_ASSIGN(ID);
	FLD_ASSIGN(PersonID);
	FLD_ASSIGN(BankID);
	FLD_ASSIGN(AccType);
	FLD_ASSIGN(OpenDate);
	FLD_ASSIGN(Flags);
	FLD_ASSIGN(CorrAcc);
	FLD_ASSIGN(CorrArt);
	STRNSCPY(p_data->Acct, p_old_rec->Acct);
#undef FLD_ASSIGN
	return 1;
}

int Convert5810()
{
	int    ok = 1;
	PPWaitStart();
	THROW(ConvertRef2());
	{
		PPCvtCGoodsLine5810 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtBankAccount5810 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}

class PPCvtSpecSeries6109 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * needConversion);
	virtual int ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * PPCvtSpecSeries6109::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new SpecSeriesTbl;
	if(!tbl)
		PPSetError(PPERR_NOMEM);
	else if(pNeedConversion) {
		const RECORDSIZE recsz = tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(SpecSeriesTbl::Rec));
	}
	return tbl;
}

int PPCvtSpecSeries6109::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	const struct OldSpecSeriesRec {
		char   Serial[24];
		char   Barcode[24];
		char   GoodsName[64];
		char   ManufName[48];
		LDATE  InfoDate;
		int32  InfoKind;
		char   InfoIdent[24];
		uint8  Reserve[48];
	} * p_old_rec = static_cast<const OldSpecSeriesRec *>(rec);
	SpecSeriesTbl::Rec * p_data = static_cast<SpecSeriesTbl::Rec *>(tbl->getDataBuf());
	tbl->clearDataBuf();
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
	FLD_ASSIGN(InfoKind);
	FLD_ASSIGN(InfoDate);
	STRNSCPY(p_data->Serial, p_old_rec->Serial);
	STRNSCPY(p_data->Barcode, p_old_rec->Barcode);
	STRNSCPY(p_data->GoodsName, p_old_rec->GoodsName);
	STRNSCPY(p_data->ManufName, p_old_rec->ManufName);
	STRNSCPY(p_data->InfoIdent, p_old_rec->InfoIdent);
	memcpy(p_data->Reserve, p_old_rec->Reserve, sizeof(p_old_rec->Reserve));
#undef FLD_ASSIGN
	return 1;
}

/* Moved to Convert6202
int Convert6109()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtSpecSeries6109 cvt01;
		THROW(cvt01.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
*/
//
// @v6.2.2 {
//
class PPCvtObjAssoc6202 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ObjAssocTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys = 0;
			tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys == 5); // since 6.2.2 num_keys = 4
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		ObjAssocTbl::Rec * p_data = static_cast<ObjAssocTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(ObjAssocTbl::Rec));
		return 1;
	}
};

class PPCvtObjProp6202 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		int    ta = 0;
		Reference * p_ref = 0;
		DBTable * tbl = new PropertyTbl;
		THROW_MEM(tbl);
		if(pNeedConversion) {
			{ // moved from Convert5506
				struct __PPCustDisp {
					PPID   Tag;
					PPID   CashNodeID;
					PPID   Prop;
					long   Type;
					char   Port[8];
					char   Reserve[52];
					long   Reserve2[2];
				};
				struct __PPTouchScreen {
					PPID   Tag;
					PPID   CashNodeID;
					PPID   Prop;
					PPID   TouchScreenID;
					char   Reserve[60];
					long   Reserve2[2];
				};
				struct __PPExtDevices {
					PPID   Tag;
					PPID   CashNodeID;
					PPID   Prop;
					PPID   TouchScreenID;
					PPID   ExtCashNodeID;
					long   CustDispType;
					char   CustDispPort[8];
					char   Reserve[44];
					long   Reserve2[2];
				};
				PPID  cn_id;
				PPCashNode  cn_rec;
				THROW_MEM(p_ref = new Reference);
				THROW(PPStartTransaction(&ta, 1));
				for(cn_id = 0; p_ref->EnumItems(PPOBJ_CASHNODE, &cn_id, &cn_rec) > 0;) {
					if(cn_rec.Flags & CASHF_SYNC)
						if(p_ref->GetProperty(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES) > 0)
							break;
						else {
							__PPCustDisp    cd;
							__PPTouchScreen ts;
							int  is_cd = BIN(p_ref->GetProperty(PPOBJ_CASHNODE, cn_id, CNPRP_CUSTDISP, &cd, sizeof(cd)) > 0);
							int  is_ts = BIN(p_ref->GetProperty(PPOBJ_CASHNODE, cn_id, CNPRP_TOUCHSCREEN, &ts, sizeof(ts)) > 0);
							if(is_cd || is_ts) {
								__PPExtDevices  ed;
								MEMSZERO(ed);
								if(is_cd) {
									ed.CustDispType = cd.Type;
									STRNSCPY(ed.CustDispPort, cd.Port);
									THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_CUSTDISP, 0, 0));
								}
								if(is_ts) {
									ed.TouchScreenID = ts.TouchScreenID;
									THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_TOUCHSCREEN, 0, 0));
								}
								THROW(p_ref->PutProp(PPOBJ_CASHNODE, cn_id, CNPRP_EXTDEVICES, &ed, sizeof(ed)));
							}
						}
				}
				THROW(PPCommitWork(&ta));
			}
			{
				int16  num_keys = 0;
				tbl->getNumKeys(&num_keys);
				*pNeedConversion = BIN(num_keys == 1); // since 6.2.2 num_keys = 2
			}
		}
		CATCH
			PPRollbackWork(&ta);
			ZDELETE(tbl);
		ENDCATCH
		delete p_ref;
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		{
			//
			// @v6.2.12 {
			// Защита от непонятного явления: в конвертируемой записи иногда
			// содержится префикс структурированного LOB-поля. Скорее всего
			// проблема была связана с временным дефектом кода, приводящим к неверной
			// конвертации записи.
			//
			if(*pNewRecLen > offsetof(PropertyTbl::Rec, VT)) {
				SLob * p_lob = &((PropertyTbl::Rec *)pOldRec)->VT;
				if(p_lob->IsPtr()) {
					memzero(p_lob, 16);
				}
			}
			// } @v6.2.12
		}
		pNewTbl->CopyBufLobFrom(pOldRec, *pNewRecLen);
		return 1;
	}
};

class PPCvtLocation6202 : public PPTableConversion {
public:
	PPCvtLocation6202() : PPTableConversion(), Before5207(0)
	{
	}
	struct Location_Before6202 {           // size=156+160 /*136*/
		long   ID;
		long   Counter;
		long   ParentID;
		char   Name[32];
		long   Type;
		long   OwnerID;
		long   Flags;
		long   CityID;
		long   RspnsPersonID;
		char   Code[12];
		char   ZIP[12];
		int16  NumRows;
		int16  NumLayers;
		char   Address[48];
		long   MassCapacity;
		long   X;
		long   Y;
		long   Z;
		/* note */ char   FullAddr[160];
	};
	int    Before5207;

	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new LocationTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize == offsetof(Location_Before6202, FullAddr)) {
				Before5207 = 0;
				*pNeedConversion = 1;
			}
			else {
				// В версиях до 5.2.07 размер записи был такой же, как и после 6.2.02
				if(stat.IdxList.getNumKeys() < 6) {
					Before5207 = 1;
					*pNeedConversion = 1;
				}
				else
					*pNeedConversion = 0;
			}
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		pNewTbl->clearDataBuf();
		LocationTbl::Rec * p_data = static_cast<LocationTbl::Rec *>(pNewTbl->getDataBuf());
		if(Before5207) {
			struct OldLocationRec {
				long   ID;
				long   Counter;
				long   ParentID;
				char   Name[32];
				long   Type;
				long   OwnerID;
				long   Flags;
				long   CityID;
				char   ZIP[12];
				char   Address[48];
				char   Reserve[2];
				long   RspnsPersonID;
				char   Code[10];
			} * p_old_rec = static_cast<OldLocationRec *>(pOldRec);
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Counter);
			FLD_ASSIGN(ParentID);
			FLD_ASSIGN(OwnerID);
			p_data->Type  = static_cast<int16>(p_old_rec->Type);
			p_data->Flags = static_cast<int16>(p_old_rec->Flags);
			FLD_ASSIGN(CityID);
			FLD_ASSIGN(RspnsPersonID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			LocationCore::SetExField(p_data, LOCEXSTR_ZIP, p_old_rec->ZIP);
			LocationCore::SetExField(p_data, LOCEXSTR_SHORTADDR, p_old_rec->Address);
			return 1;
		}
		else {
			Location_Before6202 * p_old_rec = static_cast<Location_Before6202 *>(pOldRec);
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Counter);
			FLD_ASSIGN(ParentID);
			FLD_ASSIGN(OwnerID);
			p_data->Type  = static_cast<int16>(p_old_rec->Type);
			p_data->Flags = static_cast<int16>(p_old_rec->Flags);
			FLD_ASSIGN(CityID);
			FLD_ASSIGN(RspnsPersonID);
			FLD_ASSIGN(NumRows);
			FLD_ASSIGN(NumLayers);
			FLD_ASSIGN(MassCapacity);
			FLD_ASSIGN(X);
			FLD_ASSIGN(Y);
			FLD_ASSIGN(Z);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			LocationCore::SetExField(p_data, LOCEXSTR_ZIP, p_old_rec->ZIP);
			LocationCore::SetExField(p_data, LOCEXSTR_SHORTADDR, p_old_rec->Address);
			LocationCore::SetExField(p_data, LOCEXSTR_FULLADDR,  p_old_rec->FullAddr);
			return 1;
		}
	}
#undef FLD_ASSIGN
};

class PPCvtWorld6202 : public PPTableConversion {
public:
	struct World_Before6202 {
		long   ID;
		long   Kind;
		long   ParentID;
		long   CountryID;
		char   Name[48];
		char   Abbr[20];
		long   Status;
		long   Flags;
		char   Phone[20];
		char   Code[20];
		char   ZIP[12];
		uint8  Reserve[12];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new WorldTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < sizeof(WorldTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		WorldTbl::Rec * p_data = static_cast<WorldTbl::Rec *>(pNewTbl->getDataBuf());
		World_Before6202 * p_old_rec = static_cast<World_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(Kind);
		FLD_ASSIGN(ParentID);
		FLD_ASSIGN(CountryID);
		FLD_ASSIGN(Status);
		FLD_ASSIGN(Flags);
		STRNSCPY(p_data->Name, p_old_rec->Name);
		STRNSCPY(p_data->Abbr, p_old_rec->Abbr);
		STRNSCPY(p_data->Phone, p_old_rec->Phone);
		STRNSCPY(p_data->Code, p_old_rec->Code);
		STRNSCPY(p_data->ZIP, p_old_rec->ZIP);
		return 1;
#undef FLD_ASSIGN
	}
};
#endif // } @v12.4.1
#if 0 // (moved to PPCvtPerson11112) {
class PPCvtPerson6202 : public PPTableConversion {
public:
	struct Person_Before6202 {
		long   ID;
		char   Name[48];
		long   Status;
		long   MainLoc;
		long   Flags;
		long   RLoc;
		long   Division;
		long   Position;
		long   CatID;
		char   Memo[128];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PersonTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < offsetof(PersonTbl::Rec, Memo));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PersonTbl::Rec * p_data = static_cast<PersonTbl::Rec *>(pNewTbl->getDataBuf());
		Person_Before6202 * p_old_rec = (Person_Before6202 *)pOldRec;
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(Status);
		FLD_ASSIGN(MainLoc);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(RLoc);
		FLD_ASSIGN(Division);
		FLD_ASSIGN(Position);
		FLD_ASSIGN(CatID);
		STRNSCPY(p_data->Name, p_old_rec->Name);
		STRNSCPY(p_data->Memo, p_old_rec->Memo);
#undef FLD_ASSIGN
		return 1;
	}
};
#endif // } 0 (moved to PPCvtPerson11112)
#if 0 // @v12.4.1 {
class PPCvtPersonKind6202 : public PPTableConversion {
public:
	struct PersonKind_Before6202 {
		long   KindID;
		long   PersonID;
		char   Name[128];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PersonKindTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < sizeof(PersonKindTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PersonKindTbl::Rec * p_data = static_cast<PersonKindTbl::Rec *>(pNewTbl->getDataBuf());
		PersonKind_Before6202 * p_old_rec = static_cast<PersonKind_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(KindID);
		FLD_ASSIGN(PersonID);
		STRNSCPY(p_data->Name, p_old_rec->Name);
#undef FLD_ASSIGN
		return 1;
	}
};

class PPCvtArticle6202 : public PPTableConversion {
public:
	struct Article_Before6202 {
		long   ID;
		long   AccSheetID;
		long   Article;
		long   ObjID;
		char   Name[48];
		int16  AccessLevel;
		int16  Closed;
		long   Flags;
		uint8  Reserve2[8];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ArticleTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < sizeof(ArticleTbl::Rec));
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ArticleTbl::Rec * p_data = static_cast<ArticleTbl::Rec *>(pNewTbl->getDataBuf());
		Article_Before6202 * p_old_rec = static_cast<Article_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(AccSheetID);
		FLD_ASSIGN(Article);
		FLD_ASSIGN(ObjID);
		FLD_ASSIGN(AccessLevel);
		FLD_ASSIGN(Closed);
		FLD_ASSIGN(Flags);
		STRNSCPY(p_data->Name, p_old_rec->Name);
#undef FLD_ASSIGN
		return 1;
	}
};

struct Goods2_Before6202 {
	long   ID;
	long   Kind;
	char   Name[64];
	char   Abbr[64];
	long   ParentID;
	long   GoodsTypeID;
	long   UnitID;
	long   PhUnitID;
	double PhUPerU;
	long   ManufID;
	long   StrucID;
	long   TaxGrpID;
	long   WrOffGrpID;
	long   Flags;
	long   GdsClsID;
	long   BrandID;
	long   DefBCodeStrucID;
	long   DefPrcID;
	long   Reserve;
};

static int GetTransport_Before6202(PPID id, const Goods2_Before6202 * pOldRec, PPTransport * pRec)
{
	struct __TranspD {
		PPID   TrModelID;       // ИД модели транспортного средства
		char   TrailerCode[16]; // Номер прицепа    //
		char   Code[16];        // Номер автомобиля //
		PPID   OwnerID;
		PPID   CountryID;
		PPID   CaptainID;
		char   Reserve[16];     // @v4.4.5 Поле Abbr таблицы Goods2 расширено до 64 байт
	};
	pRec->ID = pOldRec->ID;
	pRec->TrType = pOldRec->GdsClsID;
	STRNSCPY(pRec->Name, pOldRec->Name);
	__TranspD * p_td = (__TranspD*)pOldRec->Abbr;
	pRec->TrType = pOldRec->GdsClsID;
	STRNSCPY(pRec->Code, p_td->Code);
	STRNSCPY(pRec->TrailerCode, p_td->TrailerCode);
	pRec->TrModelID = p_td->TrModelID;
	pRec->OwnerID   = p_td->OwnerID;
	pRec->CountryID = p_td->CountryID;
	pRec->CaptainID = p_td->CaptainID;
	return 1;
}

class PPCvtGoods6202 : public PPTableConversion {
public:
	struct Goods2_Before4405 {
		long   ID;
		long   Kind;
		char   Name[48];
		char   Abbr[48];
		long   ParentID;
		long   GoodsTypeID;
		long   UnitID;
		long   PhUnitID;
		double PhUPerU;
		long   ManufID;
		long   StrucID;
		long   TaxGrpID;
		long   WrOffGrpID;
		long   Flags;
		long   GdsClsID;
		long   BrandID;
	};
	int    Before4405;

	PPCvtGoods6202() : PPTableConversion(), Before4405(0)
	{
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		Before4405 = 0;
		GoodsCore * p_tbl = new GoodsCore;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(Goods2_Before6202)) {
				*pNeedConversion = 1;
				Before4405 = 1;
			}
			else if(stat.FixRecSize < sizeof(Goods2Tbl::Rec)) {
				*pNeedConversion = 1;
				Before4405 = 0;
			}
			else
				*pNeedConversion = 0;
		}
		return p_tbl;
	}
	virtual void DestroyTable(DBTable * pTbl)
	{
		delete (GoodsCore *)pTbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pOldRec, int * pNewRecLen)
	{
		int    ok = 1;
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		pTbl->clearDataBuf();
		Goods2Tbl::Rec * p_data = static_cast<Goods2Tbl::Rec *>(pTbl->getDataBuf());
		if(Before4405) {
			const Goods2_Before4405 * p_old_rec = static_cast<const Goods2_Before4405 *>(pOldRec);
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Kind);
			FLD_ASSIGN(ParentID);
			FLD_ASSIGN(GoodsTypeID);
			FLD_ASSIGN(UnitID);
			FLD_ASSIGN(PhUnitID);
			FLD_ASSIGN(PhUPerU);
			FLD_ASSIGN(ManufID);
			FLD_ASSIGN(StrucID);
			FLD_ASSIGN(TaxGrpID);
			FLD_ASSIGN(WrOffGrpID);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(GdsClsID);
			FLD_ASSIGN(BrandID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Abbr, p_old_rec->Abbr);
		}
		else {
			const Goods2_Before6202 * p_old_rec = static_cast<const Goods2_Before6202 *>(pOldRec);
			if(p_old_rec->Kind == PPGDSK_TRANSPORT) {
				PPTransport tr_rec;
				BarcodeArray bc_list;
				GetTransport_Before6202(p_old_rec->ID, p_old_rec, &tr_rec);
				THROW(PPObjTransport::MakeStorage(p_old_rec->ID, &tr_rec, p_data, &bc_list));
				THROW(static_cast<GoodsCore *>(pTbl)->UpdateBarcodes(p_old_rec->ID, &bc_list, 0));
			}
			else {
				FLD_ASSIGN(ID);
				FLD_ASSIGN(Kind);
				FLD_ASSIGN(ParentID);
				FLD_ASSIGN(GoodsTypeID);
				FLD_ASSIGN(UnitID);
				FLD_ASSIGN(PhUnitID);
				FLD_ASSIGN(PhUPerU);
				FLD_ASSIGN(ManufID);
				FLD_ASSIGN(StrucID);
				FLD_ASSIGN(TaxGrpID);
				FLD_ASSIGN(WrOffGrpID);
				FLD_ASSIGN(Flags);
				FLD_ASSIGN(GdsClsID);
				FLD_ASSIGN(BrandID);
				FLD_ASSIGN(DefBCodeStrucID);
				FLD_ASSIGN(DefPrcID);
				STRNSCPY(p_data->Name, p_old_rec->Name);
				STRNSCPY(p_data->Abbr, p_old_rec->Abbr);
			}
		}
		CATCHZOK
		return ok;
	}
#undef FLD_ASSIGN
};

class PPCvtAdvBillItem6202 : public PPTableConversion {
public:
	struct AdvBillItem_Before6202 {
		long   BillID;
		int16  RByBill;
		char   AdvCode[10];
		LDATE  AdvDt;
		long   AdvBillKindID;
		long   AdvBillID;
		long   AccID;
		long   ArID;
		long   Flags;
		long   CalcGrpID;
		double Amount;
		double ExtAmt1;
		double ExtAmt2;
		double ExtAmt3;
		double ExtAmt4;
		double ExtAmt5;
		uint8  Reserve[36];
		char   Memo[128];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new AdvBillItemTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < offsetof(AdvBillItemTbl::Rec, Memo));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		AdvBillItemTbl::Rec * p_data = static_cast<AdvBillItemTbl::Rec *>(pNewTbl->getDataBuf());
		AdvBillItem_Before6202 * p_old_rec = static_cast<AdvBillItem_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(BillID);
		FLD_ASSIGN(RByBill);
		FLD_ASSIGN(AdvDt);
		FLD_ASSIGN(AdvBillKindID);
		FLD_ASSIGN(AdvBillID);
		FLD_ASSIGN(AccID);
		FLD_ASSIGN(ArID);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(CalcGrpID);
		FLD_ASSIGN(Amount);
		FLD_ASSIGN(ExtAmt1);
		FLD_ASSIGN(ExtAmt2);
		FLD_ASSIGN(ExtAmt3);
		FLD_ASSIGN(ExtAmt4);
		FLD_ASSIGN(ExtAmt5);
		STRNSCPY(p_data->AdvCode, p_old_rec->AdvCode);
		STRNSCPY(p_data->Memo, p_old_rec->Memo);
#undef FLD_ASSIGN
		return 1;
	}
};
#endif // } 0 @v12.4.1
#if 0 // @v10.7.2 Moved to PPCvtPrjTask10702 {
class PPCvtPrjTask6202 : public PPTableConversion {
public:
	struct PrjTask_Before6202 {
		long   ID;
		long   ProjectID;
		long   Kind;
		char   Code[16];
		long   CreatorID;
		long   GroupID;
		long   EmployerID;
		long   ClientID;
		long   TemplateID;
		LDATE  Dt;
		LTIME  Tm;
		LDATE  StartDt;
		LTIME  StartTm;
		LDATE  EstFinishDt;
		LTIME  EstFinishTm;
		LDATE  FinishDt;
		LTIME  FinishTm;
		int16  Priority;
		int16  Status;
		int16  DrPrd;
		int16  DrKind;
		int32  DrDetail;
		long   Flags;
		long   DlvrAddrID;
		long   LinkTaskID;
		double Amount;
		int32  OpenCount;
		long   BillArID;
		uint8  Reserve[16];
		char   Descr[224];
		char   Memo[128];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PrjTaskTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < offsetof(PrjTaskTbl::Rec, Memo));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PrjTaskTbl::Rec * p_data = static_cast<PrjTaskTbl::Rec *>(pNewTbl->getDataBuf());
		PrjTask_Before6202 * p_old_rec = static_cast<PrjTask_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(ProjectID);
		FLD_ASSIGN(Kind);
		STRNSCPY(p_data->Code, p_old_rec->Code);
		FLD_ASSIGN(CreatorID);
		FLD_ASSIGN(GroupID);
		FLD_ASSIGN(EmployerID);
		FLD_ASSIGN(ClientID);
		FLD_ASSIGN(TemplateID);
		FLD_ASSIGN(Dt);
		FLD_ASSIGN(Tm);
		FLD_ASSIGN(StartDt);
		FLD_ASSIGN(StartTm);
		FLD_ASSIGN(EstFinishDt);
		FLD_ASSIGN(EstFinishTm);
		FLD_ASSIGN(FinishDt);
		FLD_ASSIGN(FinishTm);
		FLD_ASSIGN(Priority);
		FLD_ASSIGN(Status);
		FLD_ASSIGN(DrPrd);
		FLD_ASSIGN(DrKind);
		FLD_ASSIGN(DrDetail);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(DlvrAddrID);
		FLD_ASSIGN(LinkTaskID);
		FLD_ASSIGN(Amount);
		FLD_ASSIGN(OpenCount);
		FLD_ASSIGN(BillArID);
		STRNSCPY(p_data->Descr, p_old_rec->Descr);
		STRNSCPY(p_data->Memo, p_old_rec->Memo);
#undef FLD_ASSIGN
		return 1;
	}
};

class PPCvtProject6202 : public PPTableConversion {
public:
	struct Project_Before6202 {
		long   ID;
		long   Kind;
		long   ParentID;
		char   Name[48];
		char   Code[16];
		LDATE  Dt;
		LDATE  BeginDt;
		LDATE  EstFinishDt;
		LDATE  FinishDt;
		long   MngrID;
		long   ClientID;
		long   TemplateID;
		long   Status;
		long   Flags;
		long   BillOpID;
		uint8  Reserve[44];
		char   Descr[224];
		char   Memo[128];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new ProjectTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize < offsetof(ProjectTbl::Rec, Memo));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ProjectTbl::Rec * p_data = static_cast<ProjectTbl::Rec *>(pNewTbl->getDataBuf());
		Project_Before6202 * p_old_rec = static_cast<Project_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(Kind);
		FLD_ASSIGN(ParentID);
		STRNSCPY(p_data->Name, p_old_rec->Name);
		STRNSCPY(p_data->Code, p_old_rec->Code);
		FLD_ASSIGN(Dt);
		FLD_ASSIGN(BeginDt);
		FLD_ASSIGN(EstFinishDt);
		FLD_ASSIGN(FinishDt);
		FLD_ASSIGN(MngrID);
		FLD_ASSIGN(ClientID);
		FLD_ASSIGN(TemplateID);
		FLD_ASSIGN(Status);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(BillOpID);
		STRNSCPY(p_data->Descr, p_old_rec->Descr);
		STRNSCPY(p_data->Memo, p_old_rec->Memo);
#undef FLD_ASSIGN
		return 1;
	}
};

#endif // } 0 @v10.7.2 Moved to PPCvtPrjTask10702
#if 0 // @v12.4.1 {
class PPCvtObjSyncQueue6202 : public PPTableConversion {
public:
	struct ObjSyncQueue_Before6202 {
		long   ID;
		int16  DBID;
		int16  ObjType;
		long   ObjID;
		int16  CommIdPfx;
		long   CommID;
		int16  Flags;
		long   PrimObjID;
		LDATE  ModDt;
		LTIME  ModTm;
		long   Priority;
		long   FileId;
		long   FilePos;
		long   RedirID;
		char   ObjName[64];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new ObjSyncQueueTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			*pNeedConversion = BIN(stat.FixRecSize > offsetof(ObjSyncQueueTbl::Rec, ObjName));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ObjSyncQueueTbl::Rec * p_data = static_cast<ObjSyncQueueTbl::Rec *>(pNewTbl->getDataBuf());
		ObjSyncQueue_Before6202 * p_old_rec = static_cast<ObjSyncQueue_Before6202 *>(pOldRec);
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		FLD_ASSIGN(ID);
		FLD_ASSIGN(DBID);
		FLD_ASSIGN(ObjType);
		FLD_ASSIGN(ObjID);
		FLD_ASSIGN(CommIdPfx);
		FLD_ASSIGN(CommID);
		FLD_ASSIGN(Flags);
		FLD_ASSIGN(PrimObjID);
		FLD_ASSIGN(ModDt);
		FLD_ASSIGN(ModTm);
		FLD_ASSIGN(Priority);
		FLD_ASSIGN(FileId);
		FLD_ASSIGN(FilePos);
		FLD_ASSIGN(RedirID);
		STRNSCPY(p_data->ObjName, p_old_rec->ObjName);
#undef FLD_ASSIGN
		return 1;
	}
};

int Convert6202()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtSpecSeries6109 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtObjAssoc6202 cvt02;
		THROW(cvt02.Convert());
	}
	{
		PPCvtObjProp6202 cvt03;
		THROW(cvt03.Convert());
	}
	{
		PPCvtLocation6202 cvt04;
		THROW(cvt04.Convert());
	}
	{
		PPCvtWorld6202 cvt05;
		THROW(cvt05.Convert());
	}
	{
		// @v11.1.12 PPCvtPerson6202 cvt06;
		// @v11.1.12 THROW(cvt06.Convert());
	}
	{
		PPCvtPersonKind6202 cvt07;
		THROW(cvt07.Convert());
	}
	{
		PPCvtArticle6202 cvt08;
		THROW(cvt08.Convert());
	}
	{
		PPCvtGoods6202 cvt09;
		THROW(cvt09.Convert());
	}
	{
		PPCvtAdvBillItem6202 cvt10;
		THROW(cvt10.Convert());
	}
	{
		// @v10.7.2 PPCvtPrjTask6202 cvt11;
		// @v10.7.2 THROW(cvt11.Convert());
	}
	{
		// @v10.7.2 PPCvtProject6202 cvt12;
		// @v10.7.2 THROW(cvt12.Convert());
	}
	{
		PPCvtObjSyncQueue6202 cvt13;
		THROW(cvt13.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
//
// } @v6.2.2
//
class PPCvtSalary6303 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new SalaryTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DBIdxSpec * p_is = tbl->getIndexSpec(1, &num_seg);
			if(num_seg == 3)
				*pNeedConversion = 1;
			else
				*pNeedConversion = 0;
			delete p_is;
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		SalaryTbl::Rec * p_data = static_cast<SalaryTbl::Rec *>(tbl->getDataBuf());
		tbl->clearDataBuf();
		memcpy(p_data, rec, sizeof(SalaryTbl::Rec));
		*pNewRecLen = -1;
		return 1;
	}
};

CONVERT_PROC(Convert6303, PPCvtSalary6303);
//
//
//
class PPCvtInventory6407 : public PPTableConversion {
private:
	int    Before5009;
public:
	PPCvtInventory6407() : PPTableConversion(), Before5009(0)
	{
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new InventoryTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 3); // @v5.0.10  2-->3
			if(num_keys < 3) {
				Before5009 = 1;
				*pNeedConversion = 1;
			}
			else {
				DbTableStat stat;
				p_tbl->GetFileStat(-1, &stat);
				if(stat.FixRecSize < sizeof(InventoryTbl::Rec)) {
					*pNeedConversion = 1;
				}
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * pRec, int * pNewRecLen)
	{
		InventoryTbl::Rec * p_rec = static_cast<InventoryTbl::Rec *>(tbl->getDataBuf());
		tbl->clearDataBuf();
		if(Before5009) {
			const struct Rec_Before5009 { // size=92
				long   BillID;
				long   OprNo;
				long   GoodsID;
				long   Flags;
				int16  AutoLine;
				int16  WritedOff;
				double UnitPerPack;
				double Quantity;
				double StockRest;
				char   Price[8];
				char   StockPrice[8];
				int16  Reserve1;
				int16  DiffSign;
				double DiffQtty;
				double DiffPctQtty;
				double UnwritedDiff;
				char   Reserve2[4];
			} * p_old_rec = static_cast<const Rec_Before5009 *>(pRec);
#define FLD_ASSIGN(f) p_rec->f = p_old_rec->f
			FLD_ASSIGN(BillID);
			FLD_ASSIGN(OprNo);
			FLD_ASSIGN(GoodsID);
			FLD_ASSIGN(UnitPerPack);
			FLD_ASSIGN(Quantity);
			FLD_ASSIGN(StockRest);
			FLD_ASSIGN(DiffQtty);
			FLD_ASSIGN(DiffPctQtty);
			FLD_ASSIGN(UnwritedDiff);
			FLD_ASSIGN(Flags);
#undef FLD_ASSIGN
    		INVENT_SETDIFFSIGN(p_rec->Flags, p_old_rec->DiffSign);
			INVENT_SETAUTOLINE(p_rec->Flags, p_old_rec->AutoLine);
			SETFLAG(p_rec->Flags, INVENTF_WRITEDOFF, p_old_rec->WritedOff);
			p_rec->Price = MONEYTOLDBL(p_old_rec->Price);
			p_rec->StockPrice    = MONEYTOLDBL(p_old_rec->StockPrice);
			p_rec->CSesDfctQtty  = 0;
			p_rec->CSesDfctPrice = 0;
			p_rec->WrOffPrice    = p_rec->StockPrice;
		}
		else {
			const struct Rec_Before6407 {
				int32  BillID;
				int32  OprNo;
				int32  GoodsID;
				int32  Flags;
				double UnitPerPack;
				double Quantity;
				double StockRest;
				double Price;
				double StockPrice;
				double DiffQtty;
				double DiffPctQtty;
				double UnwritedDiff;
				double CSesDfctQtty;
				double CSesDfctPrice;
				double WrOffPrice;
			} * p_old_rec = static_cast<const Rec_Before6407 *>(pRec);
#define FLD_ASSIGN(f) p_rec->f = p_old_rec->f
			FLD_ASSIGN(BillID);
			FLD_ASSIGN(OprNo);
			FLD_ASSIGN(GoodsID);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(UnitPerPack);
			FLD_ASSIGN(Quantity);
			FLD_ASSIGN(StockRest);
			FLD_ASSIGN(Price);
			FLD_ASSIGN(StockPrice);
			FLD_ASSIGN(DiffQtty);
			FLD_ASSIGN(DiffPctQtty);
			FLD_ASSIGN(UnwritedDiff);
			FLD_ASSIGN(CSesDfctQtty);
			FLD_ASSIGN(CSesDfctPrice);
			FLD_ASSIGN(WrOffPrice);
#undef FLD_ASSIGN
		}
		return 1;
	}
};

int ConvertBillExtRec_6407(PropertyTbl::Rec * pRec); // @prototype @defined(bill.cpp)

class PPCvtProperty6407 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		//
		// Конвертируем записи PropertyTbl с координатами:
		// {PPOBJ_OPRKIND, [1..PP_MAXATURNTEMPLATES]}, {PPOBJ_BILL, BILLPRP_EXTRA}
		//
		PropertyTbl * p_tbl = new PropertyTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			PropertyTbl::Key0 k0;
			MEMSZERO(k0);
			k0.ObjType = PPOBJ_DBCONVERT;
			k0.ObjID = 0x060407;
			k0.Prop = 1;
			if(p_tbl->search(0, &k0, spEq)) {
				*pNeedConversion = 0;
			}
			else if(BTROKORNFOUND) {
				*pNeedConversion = 1;
			}
			else {
				PPSetErrorDB();
				ZDELETE(p_tbl);
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		int    ok = 1;
		PropertyTbl * p_tbl = static_cast<PropertyTbl *>(pNewTbl);
		RECORDSIZE fix_rec_size = p_tbl->getRecSize();
		PropertyTbl::Rec * p_rec = static_cast<PropertyTbl::Rec *>(p_tbl->getDataBuf());
		PropertyTbl::Rec * p_rec_old = static_cast<PropertyTbl::Rec *>(pOldRec);
		PropertyTbl::Rec temp_rec = *p_rec_old;
		if(p_rec_old->ObjType == PPOBJ_OPRKIND && p_rec_old->Prop > 0 && p_rec_old->Prop <= PP_MAXATURNTEMPLATES) {
			PPAccTurnTempl::Convert_6407(&temp_rec);
			uint s = sizeof(PPAccTurnTempl);
			p_tbl->CopyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		else if(p_rec_old->ObjType == PPOBJ_BILL && p_rec_old->Prop == BILLPRP_EXTRA) {
			*p_rec = *p_rec_old;
			ConvertBillExtRec_6407(&temp_rec);
			uint s = sizeof(BillCore::Extra_Strg);
			p_tbl->CopyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		else {
			uint s = *pNewRecLen;
			p_tbl->CopyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		return ok;
	}
	virtual int Final(DBTable * pNewTbl)
	{
		int    ok = -1;
		if(pNewTbl) {
			PropertyTbl::Rec rec;
			rec.ObjType = PPOBJ_DBCONVERT;
			rec.ObjID = 0x060407;
			rec.Prop = 1;
			ok = pNewTbl->insertRecBuf(&rec) ? 1 : PPSetErrorDB();
		}
		return ok;
	}
};

int Convert6407()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtInventory6407 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtProperty6407 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
//
//
//
class PPCvtCurRest6611 : public PPTableConversion {
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		CurRestTbl * p_tbl = new CurRestTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 2);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CurRestTbl::Rec * p_data = static_cast<CurRestTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(CurRestTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert6611, PPCvtCurRest6611);
#endif // } 0 @v12.4.1
//
//
//
#if 0 // { Перенесено в PPCvtCCheckExt7601

class PPCvtCCheckExt6708 : public PPTableConversion {
	struct CCheckExt_Before6708 {  // size=20
		long   CheckID;
		long   SalerID;
		long   TableNo;
		long   AddPaym;
		int16  GuestCount;
		uint8  Reserve[2];
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		CCheckExtTbl * p_tbl = new CCheckExtTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize == sizeof(CCheckExt_Before6708)) {
				*pNeedConversion = 1;
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CCheckExtTbl::Rec * p_data = (CCheckExtTbl::Rec *)pNewTbl->getDataBuf();
		CCheckExt_Before6708 * p_old_rec = (CCheckExt_Before6708 *)pOldRec;
		p_data->CheckID = p_old_rec->CheckID;
		p_data->SalerID = p_old_rec->SalerID;
		p_data->TableNo = p_old_rec->TableNo;
		p_data->AddPaym = p_old_rec->AddPaym;
		p_data->GuestCount = p_old_rec->GuestCount;

		memzero(p_data->Reserve, sizeof(p_data->Reserve));
		p_data->StartOrdDtm.SetZero();
		p_data->EndOrdDtm.SetZero();
		memzero(p_data->Memo, sizeof(p_data->Memo));
		return 1;
	}
};

CONVERT_PROC(Convert6708, PPCvtCCheckExt6708);

#endif // } 0 Перенесено в PPCvtCCheckExt7601
//
//
//
#if 0 // @v12.4.1 {
int ConvertQuot720()
{
	int    ok = 1;
	IterCounter cntr;
	PPObjQuotKind qk_obj;
	PPQuotKindPacket qk_pack;
	QuotationCore qc;
	Quotation2Core qc2;
	QuotationTbl::Key0 k0;
	MEMSZERO(k0);
	{
		PPTransaction tra(1);
		THROW(tra);
		PPInitIterCounter(cntr, &qc);
		PPWaitStart();
		if(qc.search(0, &k0, spFirst)) {
			do {
				if(qc.data.Actual > 0) {
					if(qk_obj.Fetch(qc.data.Kind, &qk_pack) > 0) {
						PPQuot q;
						q.GetFromRec(qc.data);
						THROW(qc2.Set(q, 0, 0, 0));
						if((cntr % 10000) == 0) {
							THROW(tra.Commit());
							THROW(tra.Start(1));
						}
					}
					else {
						// @todo log message
					}
				}
				PPWaitPercent(cntr.Increment());
			} while(qc.search(0, &k0, spNext));
		}
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class PPCvtQuot2Rel7305 : public PPTableConversion {
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		Quot2RelTbl * p_tbl = new Quot2RelTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat; // BNKeyList BNKey
			p_tbl->GetFileStat(-1, &stat);
			BNKey key1 = stat.IdxList[1];
			if(key1.getNumSeg() < 6) {
				*pNeedConversion = 1;
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		Quot2RelTbl::Rec * p_data = static_cast<Quot2RelTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(Quot2RelTbl::Rec));
		return 1;
	}
};

class PPCvtObjTag7305 : public PPTableConversion {
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		ObjTagTbl * p_tbl = new ObjTagTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(ObjTagTbl::Rec)) {
				*pNeedConversion = 1;
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		ObjTagTbl::Rec * p_data = static_cast<ObjTagTbl::Rec *>(pNewTbl->getDataBuf());
		ObjTagTbl::Rec * p_old_rec = static_cast<ObjTagTbl::Rec *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(ObjType);
		CPYFLD(ObjID);
		CPYFLD(TagID);
		CPYFLD(TagByObj);
		CPYFLD(IntVal);
		CPYFLD(RealVal);
#undef CPYFLD
		STRNSCPY(p_data->StrVal, p_old_rec->StrVal);
		return 1;
	}
};

int Convert7305()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtQuot2Rel7305 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtObjTag7305 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
#endif // } 0 @v12.4.1
//
//
//
#if 0 // @v12.2.6 {
class PPCvtVatBook7311 : public PPTableConversion {
public:
	int    Pre7208;
	PPCvtVatBook7311() : PPTableConversion(), Pre7208(0)
	{
	}
	DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new VATBookTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DbTableStat stat;
			DBIdxSpec * p_is = p_tbl->getIndexSpec(1, &num_seg);
			p_tbl->GetFileStat(-1, &stat);
			if(num_seg < 5) {
				Pre7208 = 1;
				*pNeedConversion = 1;
			}
			else if(stat.FixRecSize < sizeof(VATBookTbl::Rec)) {
				Pre7208 = 0;
				*pNeedConversion = 1;
			}
			else
				*pNeedConversion = 0;
			SAlloc::F(p_is);
		}
		return p_tbl;
	}
	int PPCvtVatBook7311::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const struct OldRec {
			int32  ID;
			char   Code[24];
			int16  LineType_;
			int16  LineSubType;
			LDATE  Dt;
			int32  LineNo;
			LDATE  InvcDt;
			LDATE  PaymDt;
			LDATE  RcptDt;
			int32  Object;
			int32  Link;
			int32  Flags;
			char   Amount[8];  // money[8.2]
			char   Excise[8];  // money[8.2]
			char   VAT0[8];    // money[8.2]
			char   Export[8];  // money[8.2]
			char   VAT1[8];    // money[8.2]
			char   SVAT1[8];   // money[8.2]
			char   VAT2[8];    // money[8.2]
			char   SVAT2[8];   // money[8.2]
			int16  Excluded;
			int32  OpID;
			char   VAT3[8];    // money[8.2]
			char   SVAT3[8];   // money[8.2]
			int32  Object2;
			int32  LocID;
			uint16 Reserve;
		} * p_old_rec = static_cast<const OldRec *>(pRec);
		VATBookTbl::Rec * p_data = static_cast<VATBookTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f)    p_data->f = p_old_rec->f
#define CPYBUFFLD(f) memcpy(p_data->f, p_old_rec->f, sizeof(p_data->f))
			CPYFLD(ID);
			if(Pre7208) {
				const long * p_old_line_type = reinterpret_cast<const long *>(&p_old_rec->LineType_);
				p_data->LineType_ = static_cast<int16>(*p_old_line_type);
				p_data->LineSubType = 0;
			}
			else {
				CPYFLD(LineType_);
				CPYFLD(LineSubType);
			}
			CPYFLD(Dt);
			CPYFLD(LineNo);
			CPYFLD(InvcDt);
			CPYFLD(PaymDt);
			CPYFLD(RcptDt);
			CPYFLD(Object);
			CPYFLD(Link);
			CPYFLD(Flags);
			CPYFLD(Excluded);
			CPYFLD(OpID);
			CPYFLD(Object2);
			CPYFLD(LocID);
			CPYBUFFLD(Code);
			CPYBUFFLD(Amount);
			CPYBUFFLD(Excise);
			CPYBUFFLD(VAT0);
			CPYBUFFLD(Export);
			CPYBUFFLD(VAT1);
			CPYBUFFLD(SVAT1);
			CPYBUFFLD(VAT2);
			CPYBUFFLD(SVAT2);
			CPYBUFFLD(VAT3);
			CPYBUFFLD(SVAT3);
#undef CPYBUFFLD
#undef CPYFLD
		return 1;
	}
};

CONVERT_PROC(Convert7311, PPCvtVatBook7311);
#endif // } 0 @v12.2.6 
//
//
//
#if 0 // @v11.1.12 moved to PPCvtTech11112 {
class PPCvtTech7506 : public PPTableConversion {
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		TechTbl * p_tbl = new TechTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 6);
		}
		OrderN_Counter = 0;
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		TechTbl::Rec * p_data = static_cast<TechTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(TechTbl::Rec));
		p_data->ParentID = 0;
		p_data->OrderN = ++OrderN_Counter;
		return 1;
	}
	long   OrderN_Counter;
};

CONVERT_PROC(Convert7506, PPCvtTech7506);
#endif // } 0 @v11.1.12 moved to PPCvtTech11112
//
//
//
#if 0 // @v12.4.1 {
class PPCvtCCheckExt7601 : public PPTableConversion {
public:
	PPCvtCCheckExt7601() : P_CcT(0), Pre6708(0)
	{
	}
	~PPCvtCCheckExt7601()
	{
		delete P_CcT;
	}
private:
	struct CCheckExt_Before6708 {  // size=20
		long   CheckID;
		long   SalerID;
		long   TableNo;
		long   AddPaym;
		int16  GuestCount;
		uint8  Reserve[2];
	};
	struct CCheckExt_Before7601 {
		int32  CheckID;
		int32  SalerID;
		int32  TableNo;
		int32  AddPaym;
		int16  GuestCount;
		uint8  Reserve[2]; // raw
		//
		int32  AddrID;
		int32  AddCrdCardID;
		int32  AddCrdCardPaym;
		int32  LinkCheckID;
		LDATETIME StartOrdDtm;
		LDATETIME EndOrdDtm;
		char   Memo[256];  // note
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		CCheckExtTbl * p_tbl = new CCheckExtTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize == sizeof(CCheckExt_Before6708)) {
				Pre6708 = 1;
				*pNeedConversion = 1;
			}
			else if(stat.FixRecSize == offsetof(CCheckExt_Before7601, Memo)) {
				Pre6708 = 0;
				*pNeedConversion = 1;
			}
			if(*pNeedConversion && !P_CcT) {
				P_CcT = new CCheckTbl;
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CCheckExtTbl::Rec * p_data = static_cast<CCheckExtTbl::Rec *>(pNewTbl->getDataBuf());
		CCheckExt_Before6708 * p_old_rec_6708 = static_cast<CCheckExt_Before6708 *>(pOldRec);
		CCheckExt_Before7601 * p_old_rec_7601 = static_cast<CCheckExt_Before7601 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
		if(Pre6708) {
			p_data->CheckID = p_old_rec_6708->CheckID;
			p_data->SalerID = p_old_rec_6708->SalerID;
			p_data->TableNo = p_old_rec_6708->TableNo;
			p_data->AddPaym_unused = p_old_rec_6708->AddPaym; // @v9.0.4 _unused
			p_data->GuestCount = p_old_rec_6708->GuestCount;

			p_data->StartOrdDtm.Z();
			p_data->EndOrdDtm.Z();
			p_data->CreationDtm.Z();
		}
		else {
			p_data->CheckID = p_old_rec_7601->CheckID;
			p_data->SalerID = p_old_rec_7601->SalerID;
			p_data->TableNo = p_old_rec_7601->TableNo;
			p_data->AddPaym_unused = p_old_rec_7601->AddPaym; // @v9.0.4 _unused
			p_data->GuestCount = p_old_rec_7601->GuestCount;

			p_data->AddrID = p_old_rec_7601->AddrID;
			p_data->AddCrdCardID_unused = p_old_rec_7601->AddCrdCardID; // @v9.0.4 _unused
			p_data->CreationUserID = 0;
			p_data->LinkCheckID = p_old_rec_7601->LinkCheckID;
			p_data->EndOrdDtm = p_old_rec_7601->EndOrdDtm;
			if(!p_old_rec_7601->StartOrdDtm) {
				p_data->StartOrdDtm.Z();
				p_data->CreationDtm.Z();
			}
			else {
				CCheckTbl::Key0 k0;
				k0.ID = p_data->CheckID;
				if(P_CcT && P_CcT->search(0, &k0, spEq)) {
					if(P_CcT->data.Flags & CCHKF_DELIVERY) {
						p_data->StartOrdDtm = p_old_rec_7601->StartOrdDtm;
						p_data->CreationDtm.Z();
					}
					else {
						p_data->StartOrdDtm.Z();
						p_data->CreationDtm = p_old_rec_7601->StartOrdDtm;
					}
				}
				else {
					p_data->StartOrdDtm.Z();
					p_data->CreationDtm.Z();
				}
			}
			STRNSCPY(p_data->Memo, p_old_rec_7601->Memo);
		}
		return 1;
	}
	CCheckTbl * P_CcT;
	int    Pre6708;
};

CONVERT_PROC(Convert7601, PPCvtCCheckExt7601);
//
//
//
class PPCvtSJ7708 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SysJournalTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			BNKey key0 = stat.IdxList[0];
			*pNeedConversion = (key0.getFlags() & XIF_DUP) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		SysJournalTbl::Rec * p_data = static_cast<SysJournalTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

class PPCvtSJR7708 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SjRsrvTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			BNKey key0 = stat.IdxList[0];
			*pNeedConversion = (key0.getFlags() & XIF_DUP) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		SjRsrvTbl::Rec * p_data = static_cast<SjRsrvTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

int Convert7708()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtSJ7708 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtSJR7708 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
#endif // } 0 @v12.4.1
//
//
//
#if 0 // @v12.0.8 {
class PPCvtSCardOp7712 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SCardOpTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(SCardOpTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct SCardOpRec_Before7712 { // size=48
			long   SCardID;
			LDATE  Dt;
			LTIME  Tm;
			long   CheckID;
			long   UserID;
			long   BillID;
			long   Flags;
			double Amount;
			double Rest;
			long   DestSCardID;
		};
		SCardOpTbl::Rec * p_data = static_cast<SCardOpTbl::Rec *>(pTbl->getDataBuf());
		const SCardOpRec_Before7712 * p_old_rec = static_cast<const SCardOpRec_Before7712 *>(pRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(SCardID);
		CPYFLD(Dt);
		CPYFLD(Tm);
		CPYFLD(UserID);
		CPYFLD(Flags);
		CPYFLD(Amount);
		CPYFLD(Rest);
		CPYFLD(DestSCardID);
#undef CPYFLD
		if(p_old_rec->CheckID) {
			p_data->LinkObjType = PPOBJ_CCHECK;
			p_data->LinkObjID = p_old_rec->CheckID;
		}
		else if(p_old_rec->BillID) {
			p_data->LinkObjType = PPOBJ_BILL;
			p_data->LinkObjID = p_old_rec->BillID;
		}
		return 1;
	}
};
#endif // } 0
#if 0 // @v12.4.1 {
class PPCvtCSession7712 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CSessionTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 5);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CSessionTbl::Rec * p_data = static_cast<CSessionTbl::Rec *>(pNewTbl->getDataBuf());
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

int Convert7712()
{
	int    ok = 1;
	PPWaitStart();
	/* @v12.0.8 {
		PPCvtSCardOp7712 cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}*/
	{
		PPCvtCSession7712 cvt2;
		ok = cvt2.Convert() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
//
// @vmiller
//
class PPCvtChkOpJrnl : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CheckOpJrnlTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(CheckOpJrnlTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct CheckOpJrnlRec_Before7909 {
			LDATE  Dt;
			LTIME  Tm;
			int32  UserID;
			int16  Action;
			int16  PrinterID;
			int32  CheckID;
			int32  CheckNum;
			float  Summ;
			float  Price;
			int32  GoodsID;
		};

		CheckOpJrnlTbl::Rec * p_data = static_cast<CheckOpJrnlTbl::Rec *>(pTbl->getDataBuf());
		const CheckOpJrnlRec_Before7909 * p_old_rec = static_cast<const CheckOpJrnlRec_Before7909 *>(pRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(Dt);
		CPYFLD(Tm);
		CPYFLD(UserID);
		CPYFLD(Action);
		CPYFLD(PrinterID);
		CPYFLD(CheckID);
		CPYFLD(CheckNum);
		CPYFLD(Summ);
		CPYFLD(Price);
		CPYFLD(GoodsID);
#undef CPYFLD
		return 1;
	}
};

//@vmiller
int Convert7907()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtChkOpJrnl cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
//
//
//
struct PPWorkbook { // @size=sizeof(Reference2Tbl::Rec)
	long   Tag;            // Const=PPOBJ_WORKBOOK_PRE813
	long   ID;             //
	char   Name[48];       // @name
	char   Symb[20];       //
	char   Reserve[44];    // @reserve
	long   CssID;          // css
	long   LinkID;         // link
	long   Order;          // order
	long   Type;           //
	long   Flags;          // @flags
	long   ParentID;       //
	long   Reserve2;       //
};

class PPWorkbookPacket_Pre813 {
public:
	PPWorkbookPacket_Pre813() : F(PPOBJ_WORKBOOK_PRE813)
	{
		destroy();
	}
	~PPWorkbookPacket_Pre813()
	{
		destroy();
	}
	void destroy()
	{
		MEMSZERO(Rec);
		TagL.Z();
		F.Clear();
	}

	PPWorkbook Rec;
	ObjTagList TagL;        // Список тегов
	ObjLinkFiles F;
};

class PPObjWorkbook_Pre813 : public PPObjReference {
public:
	explicit PPObjWorkbook_Pre813(void * extraPtr = 0) : PPObjReference(PPOBJ_WORKBOOK_PRE813, extraPtr)
	{
		ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	}
	int    GetPacket(PPID id, PPWorkbookPacket_Pre813 * pPack);
	int    RemovePacket(PPID id, int use_ta);
private:
	virtual void FASTCALL Destroy(PPObjPack * pPack);
	int    CheckParent(PPID itemID, PPID parentID);
	int    GetItemPath(PPID itemID, SString & rPath);
};

IMPL_DESTROY_OBJ_PACK(PPObjWorkbook_Pre813, PPWorkbookPacket_Pre813);

int PPObjWorkbook_Pre813::RemovePacket(PPID id, int use_ta)
{
	int    ok = 1;
	PPID   hid = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(CheckRights(PPR_DEL));
		THROW(P_Ref->RemoveItem(Obj, id, 0));
		THROW(P_Ref->RemoveProperty(Obj, id, 0, 0));
		THROW(P_Ref->Ot.PutList(Obj, id, 0, 0));
		THROW(RemoveSync(id));
		{
			ObjLinkFiles _lf(PPOBJ_WORKBOOK_PRE813);
			_lf.Save(id, 0L);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook_Pre813::GetPacket(PPID id, PPWorkbookPacket_Pre813 * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		pPack->TagL.Z();
		pPack->F.Clear();
		THROW(P_Ref->Ot.GetList(Obj, id, &pPack->TagL));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjWorkbook_Pre813::CheckParent(PPID itemID, PPID parentID)
{
	int    ok = 1;
	PPWorkbook rec;
	if(itemID > 0) {
		if(itemID == parentID) {
			ok = 0;
		}
		else {
			for(PPID id = parentID; id && Search(id, &rec) > 0; id = rec.ParentID) {
				if(rec.ID == rec.ParentID) {
					ok = 0;
					break;
				}
				else if(itemID == rec.ParentID) {
					ok = 0;
					break;
				}
			}
		}
	}
	return ok;
}

int PPObjWorkbook_Pre813::GetItemPath(PPID itemID, SString & rPath)
{
	int   ok = 0;
	PPWorkbook rec;
	SString temp_buf;
	rPath.Z();
	if(itemID > 0) {
		for(PPID id = itemID; id && Search(id, &rec) > 0; id = rec.ParentID) {
			temp_buf = rPath;
			if(temp_buf.NotEmptyS()) {
				rPath.Z().Cat(rec.Name).CatDiv('>', 1).Cat(temp_buf);
			}
			else {
				rPath = rec.Name;
			}
		}
	}
	return ok;
}

int ConvertWorkbook813()
{
	int    ok = 1;
	{
		PPObjWorkbook wb_obj;
		PPObjWorkbook_Pre813 wb_obj_pre;
		PPWorkbook rec_pre;
		SString file_name, temp_file_name; //, temp_path;
		PPIDArray id_list;
		SFsPath ps;
		PPWaitStart();
		//PPGetPath(PPPATH_TEMP, temp_path);
		PPTransaction tra(1);
		THROW(tra);
		for(SEnum en = PPRef->Enum(PPOBJ_WORKBOOK_PRE813, 0); en.Next(&rec_pre) > 0;) {
			id_list.add(rec_pre.ID);
		}
		for(uint i = 0; i < id_list.getCount(); i++) {
			PPID   id = id_list.get(i);
			PPWorkbookPacket_Pre813 pack_pre;
			if(wb_obj_pre.GetPacket(id, &pack_pre) > 0) {
				PPWorkbookPacket pack;
				pack.Rec.ID = id;
				pack.Rec.Type = pack_pre.Rec.Type;
				pack.Rec.ParentID = pack_pre.Rec.ParentID;
				pack.Rec.LinkID = pack_pre.Rec.LinkID;
				pack.Rec.CssID = pack_pre.Rec.CssID;
				pack.Rec.Rank = pack_pre.Rec.Order;
				pack.Rec.Flags = pack_pre.Rec.Flags;
				STRNSCPY(pack.Rec.Name, pack_pre.Rec.Name);
				STRNSCPY(pack.Rec.Symb, pack_pre.Rec.Symb);

				pack_pre.F.Init(PPOBJ_WORKBOOK_PRE813);
				pack_pre.F.Load(id, 0L);
				temp_file_name = 0;
				if(pack_pre.F.At(0, file_name) > 0) {
					pack.F.Init(PPOBJ_WORKBOOK);
					ps.Z().Split(file_name);
					PPMakeTempFileName("wbc813", ps.Ext, 0, temp_file_name);
					THROW_SL(SCopyFile(file_name, temp_file_name, 0, FILE_SHARE_READ, 0));
					pack.F.Replace(0, temp_file_name);
				}
				{
					wb_obj.P_Tbl->CopyBufFrom(&pack.Rec, sizeof(pack.Rec));
					THROW_DB(wb_obj.P_Tbl->insertRec());
					THROW(PPRef->Ot.PutList(PPOBJ_WORKBOOK, id, &pack.TagL, 0));
					pack.F.Save(id, 0L);
					DS.LogAction(PPACN_OBJADD, PPOBJ_WORKBOOK, id, 0, 0);
				}
				{
					THROW(wb_obj_pre.RemovePacket(id, 0));
				}
			}
			PPWaitPercent(i+1, id_list.getCount());
		}
		THROW(tra.Commit());
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class PPCvtBarcode8800 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtBarcode8800::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new BarcodeTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(BarcodeTbl::Rec)); // Новый размер =38 bytes
	}
	return p_tbl;
}

int PPCvtBarcode8800::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldBarcodeRec {
		long   GoodsID;
		double Qtty;
		long   BarcodeType;
		char   Code[16];
	} * p_old_data = static_cast<const OldBarcodeRec *>(pRec);
	BarcodeTbl::Rec * p_data = static_cast<BarcodeTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(BarcodeTbl::Rec));
	p_data->GoodsID = p_old_data->GoodsID;
	p_data->Qtty    = p_old_data->Qtty;
	p_data->BarcodeType = p_old_data->BarcodeType;
	STRNSCPY(p_data->Code, p_old_data->Code);
	return 1;
}
//
//
//
class PPCvtArGoodsCode8800 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion);
	int    ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * PPCvtArGoodsCode8800::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new ArGoodsCodeTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		const RECORDSIZE recsz = p_tbl->getRecSize();
		*pNeedConversion = BIN(recsz < sizeof(ArGoodsCodeTbl::Rec)); // Новый размер =38 bytes
	}
	return p_tbl;
}

int PPCvtArGoodsCode8800::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	const struct OldArGoodsCodeRec {
		long   GoodsID;
		long   ArID;
		long   Pack;
		char   Code[16];
	} * p_old_data = static_cast<const OldArGoodsCodeRec *>(pRec);
	ArGoodsCodeTbl::Rec * p_data = static_cast<ArGoodsCodeTbl::Rec *>(pTbl->getDataBuf());
	memzero(p_data, sizeof(ArGoodsCodeTbl::Rec));
	p_data->GoodsID = p_old_data->GoodsID;
	p_data->ArID = p_old_data->ArID;
	p_data->Pack = p_old_data->Pack;
	STRNSCPY(p_data->Code, p_old_data->Code);
	return 1;
}

int Convert8800()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtBarcode8800 cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}
	{
		PPCvtArGoodsCode8800 cvt2;
		ok = cvt2.Convert() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
//
//
//
class PPCvtCpTransf8910 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CpTransfTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz > (sizeof(CpTransfTbl::Rec) - sizeof(((CpTransfTbl::Rec*)0)->Tail)));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		struct CpTransfRec_Before8910 {
			long   BillID;        // ->Bill.ID
			int16  RByBill;       //
			int16  Reserve1;      //
			long   GoodsID;       // ->Goods2.ID
			long   LocID;         // ->Location.ID
			long   OrdLotID;      // ->Receipt.ID
			long   CurID;         // ->Ref(PPOBJ_CURRENCY)
			double UnitPerPack;   //
			double Qtty;          //
			double Rest;          // Излишек при списании
			double Cost;          //
			double Price;         //
			double Discount;      //
			double CurPrice;      //
			LDATE  Expiry;        //
			long   QCertID;       // ->QualityCert.ID
			long   InTaxGrpID;    // ->Ref(PPOBJ_GOODSTAX)
			long   Flags;         //
			char   PartNo[24];    // Номер партии (пакета)
			char   Clb[24];       // Номер ГТД
			uint8  Reserve2[28];  // @reserve
		};
		const CpTransfRec_Before8910 * p_old_data = static_cast<const CpTransfRec_Before8910 *>(rec);
		CpTransfTbl::Rec * data = static_cast<CpTransfTbl::Rec *>(tbl->getDataBuf());
		#define FLD(f) data->f = p_old_data->f
		FLD(BillID);
		FLD(RByBill);
		FLD(Reserve1);
		FLD(GoodsID);
		FLD(LocID);
		FLD(OrdLotID);
		FLD(CurID);
		FLD(UnitPerPack);
		FLD(Qtty);
		FLD(Rest);
		FLD(Cost);
		FLD(Price);
		FLD(Discount);
		FLD(CurPrice);
		FLD(Expiry);
		FLD(QCertID);
		FLD(InTaxGrpID);
		FLD(Flags);
		#undef FLD
		CpTrfrExt cpext;
		STRNSCPY(cpext.PartNo, p_old_data->PartNo);
		STRNSCPY(cpext.Clb, p_old_data->Clb);
        CpTransfCore::PutExt__(*data, &cpext);
		return 1;
	}
};

CONVERT_PROC(Convert8910, PPCvtCpTransf8910);
//
//
//
#define STAFFLIST_EXCL_ID 1000000

static int ConvertStaffList9003()
{
	int    ok = 1;
	int    db_locked = 0;
	DBTable * p_tbl = 0;
	DbProvider * p_db = CurDict;
	SString path;
	if(p_db) {
		long   tbl_id = 0;
		DbTableStat tbl_stat;
		p_db->GetTableID("StaffList_Pre9003", &tbl_id, &tbl_stat);
		if(p_db->IsFileExists_(p_db->MakeFileName_("StaffList_Pre9003", path = tbl_stat.Location))) {
			ENTER_CRITICAL_SECTION
			RECORDNUMBER numrecs = 0;
			THROW_MEM(p_tbl = new StaffList_Pre9003Tbl);
			p_tbl->getNumRecs(&numrecs);
			if(numrecs) {
				StaffList_Pre9003Tbl::Key0 k0;
				k0.ID = STAFFLIST_EXCL_ID; // Запись с эксклюзивным значением, сигнализирующая, что таблица уже отконвертирована
				if(!p_tbl->search(0, &k0, spEq)) {
					THROW_DB(BTROKORNFOUND);
					THROW(DS.GetSync().LockDB());
					db_locked = 1;
					{
						Reference ref_t;
						PPTransaction tra(1);
						THROW(tra);
						if(p_tbl->step(spFirst)) {
							do {
								StaffList_Pre9003Tbl::Rec rec;
								PPStaffEntry new_item;
								MEMSZERO(new_item);
								p_tbl->CopyBufTo(&rec);
								PPID   new_rec_id = rec.ID;
								new_item.Tag = PPOBJ_STAFFLIST2;
								new_item.ID = rec.ID;
								STRNSCPY(new_item.Name, rec.Name);
								new_item.VacancyCount = rec.VacancyCount;
								new_item.VacancyBusy = rec.VacancyBusy;
								new_item.FixedStaff = rec.FixedStaff;
								new_item.ChargeGrpID = rec.ChargeGrpID;
								new_item.Rank = rec.Rank;
								new_item.Flags = rec.Flags;
								new_item.OrgID = rec.OrgID;
								new_item.DivisionID = rec.DivisionID;
								THROW(ref_t.AddItem(PPOBJ_STAFFLIST2, &new_rec_id, &new_item, 0));
							} while(p_tbl->step(spNext));
							{
								//
								// Создаем запись с эксклюзивным значением, сигнализирующую, что таблица уже отконвертирована
								//
								StaffList_Pre9003Tbl::Rec spec_rec;
								spec_rec.ID = STAFFLIST_EXCL_ID;
								spec_rec.OrgID = STAFFLIST_EXCL_ID;
								spec_rec.DivisionID = STAFFLIST_EXCL_ID;
								THROW_DB(p_tbl->insertRecBuf(&spec_rec));
							}
						}
						THROW(tra.Commit());
					}
				}
			}
			LEAVE_CRITICAL_SECTION
		}
	}
	CATCHZOK
	delete p_tbl;
	if(db_locked)
		DS.GetSync().UnlockDB();
	return ok;
}

/*
int Convert9003()
{
	int    ok = 1;
	PPWaitStart();
	{
		ok = ConvertStaffList9003() ? 1 : PPErrorZ();
	}
	PPWaitStop();
	return ok;
}
*/
//
//
//
#define ACCOUNT_EXCL_ID   1000000
#define ACCOUNT_EXCL2_ID  1000001

static int ConvertAccount9004()
{
	int    ok = 1;
	int    db_locked = 0;
	// @v10.3.0 (never used) int    need_conversion = 0;
	DBTable * p_tbl = 0;
	DbProvider * p_db = CurDict;
	SString path;
	if(p_db) {
		long   tbl_id = 0;
		DbTableStat tbl_stat;
		p_db->GetTableID("Account_Pre9004", &tbl_id, &tbl_stat);
		if(p_db->IsFileExists_(p_db->MakeFileName_("Account_Pre9004", path = tbl_stat.Location))) {
			ENTER_CRITICAL_SECTION
			RECORDNUMBER numrecs = 0;
			THROW_MEM(p_tbl = new Account_Pre9004Tbl);
			p_tbl->getNumRecs(&numrecs);
			if(numrecs) {
				Account_Pre9004Tbl::Key0 k0;
				k0.ID = ACCOUNT_EXCL2_ID; // Запись с эксклюзивным значением, сигнализирующая, что таблица уже отконвертирована
				if(!p_tbl->search(0, &k0, spEq)) {
					THROW_DB(BTROKORNFOUND);
					THROW(DS.GetSync().LockDB());
					db_locked = 1;
					{
						Reference ref_t;
						int   err_cvt_detected = 0;
						PPTransaction tra(1);
						THROW(tra);
						{
							Account_Pre9004Tbl::Key0 k0;
							k0.ID = ACCOUNT_EXCL_ID; // Запись с эксклюзивным значением, сигнализирующая, что таблица отконвертирована (с ошибкой)
							if(p_tbl->search(0, &k0, spEq)) {
								//
								// Удаляем запись с ошибочным эксклюзивным значением
								//
								THROW_DB(p_tbl->deleteRec());
								{
									err_cvt_detected = 1;
									PPAccount acc_rec;
									ReferenceTbl::Key0 rk;
									rk.ObjType = PPOBJ_ACCOUNT2;
									rk.ObjID   = 0;
									if(ref_t.search(0, &rk, spGe) && ref_t.data.ObjType == PPOBJ_ACCOUNT2) do {
										ref_t.CopyBufTo(&acc_rec);
										PPAccount::_A_ _a = acc_rec.A;
										acc_rec.A.Ac = _a.Sb;
										acc_rec.A.Sb = _a.Ac;
										THROW_DB(ref_t.updateRecBuf(&acc_rec));
									} while(ref_t.search(0, &rk, spNext) && ref_t.data.ObjType == PPOBJ_ACCOUNT2);
									THROW_DB(BTROKORNFOUND);
								}
							}
							else {
								THROW_DB(BTROKORNFOUND);
							}
						}
						if(!err_cvt_detected) {
							if(p_tbl->step(spFirst)) {
								do {
									Account_Pre9004Tbl::Rec rec;
									PPAccount new_item;
									MEMSZERO(new_item);
									p_tbl->CopyBufTo(&rec);
									PPID   new_rec_id = rec.ID;
									new_item.Tag = PPOBJ_ACCOUNT2;
									new_item.ID = rec.ID;
									STRNSCPY(new_item.Name, rec.Name);
									STRNSCPY(new_item.Code, rec.Code);
									new_item.A.Ac = rec.Ac;
									new_item.A.Sb = rec.Sb;
									new_item.AccSheetID = rec.AccSheetID;
									new_item.MainOrgID = 0;
									new_item.CurID = rec.CurID;
									new_item.ParentID = rec.ParentID;
									new_item.Type = rec.Type;
									new_item.Kind = rec.Kind;
									new_item.Flags = rec.Flags;
									new_item.OpenDate = rec.OpenDate;
									new_item.Frrl_Date = rec.FRRL_Date;
									new_item.Limit = rec.SaldoLimit;
									new_item.Overdraft = rec.Overdraft;
									THROW(ref_t.AddItem(PPOBJ_ACCOUNT2, &new_rec_id, &new_item, 0));
									if(rec.Type == ACY_AGGR) {
										ObjRestrictArray gen_list;
										int   glr = 0;
										THROW(glr = ref_t.GetPropArray(PPOBJ_ACCOUNT_PRE9004, rec.ID, ACCPRP_GENACCLIST, &gen_list));
										if(glr > 0) {
											if(gen_list.getCount()) {
												THROW(ref_t.PutPropArray(PPOBJ_ACCOUNT2, rec.ID, ACCPRP_GENACCLIST, &gen_list, 0));
											}
											THROW(ref_t.PutPropArray(PPOBJ_ACCOUNT_PRE9004, rec.ID, ACCPRP_GENACCLIST, 0, 0));
										}
									}
								} while(p_tbl->step(spNext));
							}
						}
						{
							//
							// Создаем запись с эксклюзивным значением, сигнализирующую, что таблица уже отконвертирована
							//
							Account_Pre9004Tbl::Rec spec_rec;
							spec_rec.ID = ACCOUNT_EXCL2_ID;
							spec_rec.AccSheetID = ACCOUNT_EXCL2_ID;
							THROW_DB(p_tbl->insertRecBuf(&spec_rec));
						}
						THROW(tra.Commit());
					}
				}
			}
			LEAVE_CRITICAL_SECTION
		}
	}
	CATCHZOK
	delete p_tbl;
	if(db_locked)
		DS.GetSync().UnlockDB();
	return ok;
}

#define BANKACCOUNT_EXCL_ID 1000000

static int ConvertBankAccount9004()
{
	int    ok = 1;
	int    db_locked = 0;
	DBTable * p_tbl = 0;
	DbProvider * p_db = CurDict;
	SString path;
	SString temp_buf;
	if(p_db) {
		long   tbl_id = 0;
		DbTableStat tbl_stat;
		p_db->GetTableID("BankAccount_Pre9004", &tbl_id, &tbl_stat);
		if(p_db->IsFileExists_(p_db->MakeFileName_("BankAccount_Pre9004", path = tbl_stat.Location))) {
			ENTER_CRITICAL_SECTION
			RECORDNUMBER numrecs = 0;
			THROW_MEM(p_tbl = new BankAccount_Pre9004Tbl);
			p_tbl->getNumRecs(&numrecs);
			if(numrecs) {
				BankAccount_Pre9004Tbl::Key0 k0;
				k0.ID = BANKACCOUNT_EXCL_ID; // Запись с эксклюзивным значением, сигнализирующая, что таблица уже отконвертирована
				if(!p_tbl->search(0, &k0, spEq)) {
					THROW_DB(BTROKORNFOUND);
					THROW(DS.GetSync().LockDB());
					db_locked = 1;
					{
						Reference ref_t;
						RegisterCore reg_t;
						PPTransaction tra(1);
						THROW(tra);
						if(p_tbl->step(spFirst)) {
							do {
								BankAccount_Pre9004Tbl::Rec rec;
								PPBankAccount new_item;
								p_tbl->CopyBufTo(&rec);
								PPID   new_rec_id = rec.ID;
								new_item.ID = 0;
                                new_item.ObjType = PPOBJ_PERSON;
                                new_item.PersonID = rec.PersonID;
                                new_item.RegTypeID = PPREGT_BANKACCOUNT;
                                new_item.OpenDate = rec.OpenDate;
                                new_item.BankID = rec.BankID;
                                new_item.Expiry = ZERODATE;
                                new_item.Flags = rec.Flags;
                                new_item.AccType = rec.AccType;
								STRNSCPY(new_item.Acct, rec.Acct);
								{
									PPID   new_id = 0;
									RegisterTbl::Rec reg_rec;
									new_item.GetRegisterRec(reg_rec);
									THROW(reg_t.Add_ForceDup(&new_id, &reg_rec, 0));
								}
							} while(p_tbl->step(spNext));
							{
								//
								// Создаем запись с эксклюзивным значением, сигнализирующую, что таблица уже отконвертирована
								//
								BankAccount_Pre9004Tbl::Rec spec_rec;
								spec_rec.ID = BANKACCOUNT_EXCL_ID;
								spec_rec.AccType = BANKACCOUNT_EXCL_ID;
								THROW_DB(p_tbl->insertRecBuf(&spec_rec));
							}
						}
						THROW(tra.Commit());
					}
				}
			}
			LEAVE_CRITICAL_SECTION
		}
	}
	CATCHZOK
	delete p_tbl;
	if(db_locked)
		DS.GetSync().UnlockDB();
	return ok;
}

class PPCvtCCheckPaym9004 : public PPTableConversion {
public:
	DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CCheckPaymTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(CCheckPaymTbl::Rec)); // Новый размер =38 bytes
		}
		return p_tbl;
	}
	int    ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const struct OldCCheckPaymRec {
			long   CheckID;
			int16  RByCheck;
			int16  PaymType;
			long   Amount;
			long   SCardID;
		} * p_old_data = static_cast<const OldCCheckPaymRec *>(pRec);
		CCheckPaymTbl::Rec * p_data = static_cast<CCheckPaymTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(CCheckPaymTbl::Rec));
		p_data->CheckID  = p_old_data->CheckID;
		p_data->RByCheck = p_old_data->RByCheck;
		p_data->PaymType = p_old_data->PaymType;
		p_data->Amount   = p_old_data->Amount;
		p_data->SCardID  = p_old_data->SCardID;
		return 1;
	}
};

int Convert9004()
{
	int    ok = 1;
	PPWaitStart();
	{
		THROW(ConvertStaffList9003());
		THROW(ConvertAccount9004());
		THROW(ConvertBankAccount9004());
	}
	{
		PPCvtCCheckPaym9004 cvt2;
		THROW(cvt2.Convert());
	}
	PPWaitStop();
	CATCHZOKPPERR
	return ok;
}
#endif // } 0 @v12.4.1
//
//
//
class PPCvtGoodsDebt9108 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new GoodsDebtTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz != sizeof(GoodsDebtTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct GoodsDebtTblRec_Before9108 {
			long   GoodsID;
			long   ArID;
			LDATE  Dt;
			double SaldoQtty;
			double SaldoAmount;
			uint8  Reserve[20];
		};
		GoodsDebtTbl::Rec * p_data = static_cast<GoodsDebtTbl::Rec *>(pNewTbl->getDataBuf());
		GoodsDebtTblRec_Before9108 * p_old_rec = static_cast<GoodsDebtTblRec_Before9108 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(GoodsID);
		CPYFLD(ArID);
		CPYFLD(Dt);
		CPYFLD(SaldoQtty);
		CPYFLD(SaldoAmount);
#undef CPYFLD
		return 1;
	}
};

CONVERT_PROC(Convert9108, PPCvtGoodsDebt9108);
//
//
//
class PPCvtEgaisProduct9214 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new EgaisProductTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(EgaisProductTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct EgaisProductTblRec_Before9214 {
			long   ID;
			char   AlcCode[24];
			char   ManufRarIdent[16];
			char   ImporterRarIdent[16];
			char   CategoryCode[8];
			int32  Proof;          // Промилле
			int32  Volume;         // x100000
			LDATE  ActualDate;
		};
		EgaisProductTbl::Rec * p_data = static_cast<EgaisProductTbl::Rec *>(pNewTbl->getDataBuf());
		EgaisProductTblRec_Before9214 * p_old_rec = static_cast<EgaisProductTblRec_Before9214 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(ID);
		CPYFLD(Proof);
		CPYFLD(Volume);
		CPYFLD(ActualDate);
#undef CPYFLD
		STRNSCPY(p_data->AlcCode, p_old_rec->AlcCode);
		STRNSCPY(p_data->ManufRarIdent, p_old_rec->ManufRarIdent);
		STRNSCPY(p_data->ImporterRarIdent, p_old_rec->ImporterRarIdent);
		STRNSCPY(p_data->CategoryCode, p_old_rec->CategoryCode);
		p_data->Flags = 0;
		return 1;
	}
};

CONVERT_PROC(Convert9214, PPCvtEgaisProduct9214);
//
//
//
class PPCvtSCard9400 : public PPTableConversion {
public:
	PPCvtSCard9400() : PPTableConversion(), Stage(0)
	{
	}
private:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		SCardTbl * p_tbl = new SCardTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(SCardTbl::Rec)) {
				*pNeedConversion = 1;
				Stage = 1;
			}
			else if(stat.FixRecSize > sizeof(SCardTbl::Rec) && stat.IdxCount < 5) {
				*pNeedConversion = 1;
				Stage = 0;
			}
			else {
				*pNeedConversion = 0;
			}
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		struct SCard_Before7702 {  // size=104
			long   ID;
			char   Code[24];
			char   Password[8];
			long   SeriesID;
			long   PersonID;
			long   Flags;
			LDATE  Dt;
			LDATE  Expiry;
			long   PDis;
			long   AutoGoodsID;
			double MaxCredit;
			double Turnover;
			double Rest;
			double InTrnovr;
			LTIME  UsageTmStart;
			LTIME  UsageTmEnd;
		};
		struct SCard_Before9400 {  // size=128
			long   ID;
			char   Code[24];
			char   Password[8];
			long   SeriesID;
			long   PersonID;
			long   Flags;
			LDATE  Dt;
			LDATE  Expiry;
			long   PDis;
			long   AutoGoodsID;
			double MaxCredit;
			double Turnover;
			double Rest;
			double InTrnovr;
			LTIME  UsageTmStart;
			LTIME  UsageTmEnd;
			int16  PeriodTerm;
			int16  PeriodCount;
			uint8  Reserve[20];
		};
		int    ok = 1;
		SCardTbl::Rec * p_data = static_cast<SCardTbl::Rec *>(pNewTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
		SString pw_buf;
		if(Stage == 1) {
			STATIC_ASSERT(sizeof(SCard_Before7702)==104);
			SCard_Before7702 * p_old_rec = static_cast<SCard_Before7702 *>(pOldRec);
			p_data->ID = p_old_rec->ID;
			p_data->SeriesID = p_old_rec->SeriesID;
			p_data->PersonID = p_old_rec->PersonID;
			p_data->Flags = p_old_rec->Flags;
			p_data->Dt = p_old_rec->Dt;
			p_data->Expiry = p_old_rec->Expiry;
			p_data->PDis = p_old_rec->PDis;
			p_data->AutoGoodsID = p_old_rec->AutoGoodsID;
			p_data->MaxCredit = p_old_rec->MaxCredit;
			p_data->Turnover = p_old_rec->Turnover;
			p_data->Rest = p_old_rec->Rest;
			p_data->InTrnovr = p_old_rec->InTrnovr;
			p_data->UsageTmStart = p_old_rec->UsageTmStart;
			p_data->UsageTmEnd = p_old_rec->UsageTmEnd;
			STRNSCPY(p_data->Code, p_old_rec->Code);
			//STRNSCPY(p_data->Password, p_old_rec->Password);
			pw_buf = p_old_rec->Password;
		}
		else {
			STATIC_ASSERT(sizeof(SCard_Before9400)==128);
			const SCard_Before9400 * p_old_rec = static_cast<const SCard_Before9400 *>(pOldRec);
			p_data->ID = p_old_rec->ID;
			p_data->SeriesID = p_old_rec->SeriesID;
			p_data->PersonID = p_old_rec->PersonID;
			p_data->Flags = p_old_rec->Flags;
			p_data->Dt = p_old_rec->Dt;
			p_data->Expiry = p_old_rec->Expiry;
			p_data->PDis = p_old_rec->PDis;
			p_data->AutoGoodsID = p_old_rec->AutoGoodsID;
			p_data->MaxCredit = p_old_rec->MaxCredit;
			p_data->Turnover = p_old_rec->Turnover;
			p_data->Rest = p_old_rec->Rest;
			p_data->InTrnovr = p_old_rec->InTrnovr;
			p_data->UsageTmStart = p_old_rec->UsageTmStart;
			p_data->UsageTmEnd = p_old_rec->UsageTmEnd;
			p_data->PeriodTerm = p_old_rec->PeriodTerm;
			p_data->PeriodCount = p_old_rec->PeriodCount;
			STRNSCPY(p_data->Code, p_old_rec->Code);
			//STRNSCPY(p_data->Password, p_old_rec->Password);
			pw_buf = p_old_rec->Password;
		}
		if(pw_buf.NotEmptyS()) {
			PPExtStrContainer es;
			SString ext_buffer;
			es.PutExtStrData(PPSCardPacket::extssPassword, pw_buf);
			(ext_buffer = es.GetBuffer()).Strip();
			THROW(UtrC.SetTextUtf8(TextRefIdent(PPOBJ_SCARD, p_data->ID, PPTRPROP_SCARDEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		CATCHZOK
		return ok;
	}
private:
	int    Stage; // 1 - конвертация из версии менее 7.7.2, 0 - из более новой
	UnxTextRefCore UtrC;
};

CONVERT_PROC(Convert9400, PPCvtSCard9400);
//
//
//
int Convert9811()
{
	int    ok = 1;
	SysJournal * p_sj = 0;
	PPWaitStart();
	{
		{
			//
			// Необходимо зафиксировать в системном журнале событие,
			// которое будет разграничивать применении старой
			// и новой схемы хранения версионных объектов
			//
			LDATETIME moment;
			PPIDArray acn_list;
			acn_list.add(PPACN_EVENTTOKEN);
			THROW(p_sj = new SysJournal);
			if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_OBJHIST9811, &acn_list, &moment) > 0)
				ok = -1; // Событие уже установлено
			else {
				THROW(p_sj->LogEvent(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, PPEVTOK_OBJHIST9811, 0, 1/*use_ta*/));
			}
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	ZDELETE(p_sj);
	return ok;
}
//
//
//
class PPCvtLotExtCode10209 : public PPTableConversion {
private:
	int   Before9811;
	int   Before10012;
	struct LotExtCodeRec_Before9811 {
		long   LotID;
		char   Code[96];
	};
	struct LotExtCodeRec_Before10012 {
		long   LotID;
		long   BillID;
		int16  RByBill;
		int16  Sign;
		char   Code[80];
	};
	struct LotExtCodeRec_Before10209 {
		long   LotID;
		long   BillID;
		int16  RByBill;
		int16  Sign;
		char   Code[156];
	};
public:
	PPCvtLotExtCode10209() : PPTableConversion(), Before9811(0), Before10012(0)
	{
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new LotExtCodeTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			if(recsz == sizeof(LotExtCodeRec_Before9811))
				Before9811 = 1;
			else if(recsz == sizeof(LotExtCodeRec_Before10012))
				Before10012 = 1;
			*pNeedConversion = BIN(recsz < sizeof(LotExtCodeTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		LotExtCodeTbl::Rec * p_data = static_cast<LotExtCodeTbl::Rec *>(pNewTbl->getDataBuf());
		if(Before9811) {
			LotExtCodeTbl::Rec * p_data = static_cast<LotExtCodeTbl::Rec *>(pNewTbl->getDataBuf());
			LotExtCodeRec_Before9811 * p_old_rec = static_cast<LotExtCodeRec_Before9811 *>(pOldRec);
			memzero(p_data, sizeof(*p_data));
			p_data->LotID = p_old_rec->LotID;
			STRNSCPY(p_data->Code, p_old_rec->Code);
		}
		else if(Before10012) {
			LotExtCodeRec_Before10012 * p_old_rec = static_cast<LotExtCodeRec_Before10012 *>(pOldRec);
			memzero(p_data, sizeof(*p_data));
			p_data->LotID = p_old_rec->LotID;
			p_data->BillID = p_old_rec->BillID;
			p_data->RByBill = p_old_rec->RByBill;
			p_data->Flags = 0; //p_old_rec->Sign;
			STRNSCPY(p_data->Code, p_old_rec->Code);
		}
		else {
			LotExtCodeRec_Before10209 * p_old_rec = static_cast<LotExtCodeRec_Before10209 *>(pOldRec);
			memzero(p_data, sizeof(*p_data));
			p_data->LotID = p_old_rec->LotID;
			p_data->BillID = p_old_rec->BillID;
			p_data->RByBill = p_old_rec->RByBill;
			p_data->Flags = 0; //p_old_rec->Sign;
			STRNSCPY(p_data->Code, p_old_rec->Code);
		}
		return 1;
	}
};

CONVERT_PROC(Convert10209, PPCvtLotExtCode10209);
//
//
//
int Convert10507()
{
	struct Scale_Before10507 {  // size=128
		long   Tag;
		long   ID;
		char   Name[48];
		char   Symb[20];
		PPID   ParentID;
		char   AddedMsgSign[8];
		int16  MaxAddedLn;
		int16  MaxAddedLnCount;
		char   Reserve[8];
		char   Port[8]; // Если Flags & SCALF_TCPIP, то IP-адрес устройства упаковывается в поле Port в виде: Port[0].Port[1].Port[2].Port[3]
		uint16 Get_NumTries;
		uint16 Get_Delay;
		uint16 Put_NumTries;
		uint16 Put_Delay;
		int16  BcPrefix;
		uint16 Reserve2;
		PPID   QuotKindID;
		PPID   ScaleTypeID;
		long   ProtocolVer;
		long   LogNum;
		long   Flags;
		long   Location;
		long   AltGoodsGrp;
	};
	int    ok = 1;
	int    ppref_allocated = 0;
	if(!PPRef) {
		THROW_MEM(PPRef = new Reference);
		ppref_allocated = 0;
	}
	Reference * p_ref = PPRef;
	if(p_ref) {
		SString temp_buf;
		Scale_Before10507 sc_rec;
		PPObjScale sc_obj;
		PPTransaction tra(1);
		THROW(tra);
		for(PPID sc_id = 0; p_ref->EnumItems(PPOBJ_SCALE, &sc_id, &sc_rec) > 0;) {
			int v = 0, mj = 0, mn = 0;
			reinterpret_cast<const PPScale *>(&sc_rec)->Ver_Signature.Get(&v, &mj, &mn);
			if(v >= 10 && v <= 20 && mj >= 0 && mj <= 10 && mn >= 0 && mn <= 15) {
				;
			}
			else {
				PPScalePacket pack;
#define CPYF(f) pack.Rec.f = sc_rec.f
				CPYF(Tag);
				CPYF(ID);
				CPYF(ParentID);
				CPYF(MaxAddedLn);
				CPYF(MaxAddedLnCount);
				CPYF(Get_NumTries);
				CPYF(Get_Delay);
				CPYF(Put_NumTries);
				CPYF(Put_Delay);
				CPYF(BcPrefix);
				CPYF(QuotKindID);
				CPYF(ScaleTypeID);
				CPYF(ProtocolVer);
				CPYF(LogNum);
				CPYF(Flags);
				CPYF(Location);
				CPYF(AltGoodsGrp);
#undef CPYF
				pack.Rec.Ver_Signature = DS.GetVersion();
				STRNSCPY(pack.Rec.Name, sc_rec.Name);
				STRNSCPY(pack.Rec.Symb, sc_rec.Symb);
				if(sc_rec.Flags & SCALF_TCPIP) {
					char   ip_buf[64];
					PPObjScale::DecodeIP(sc_rec.Port, ip_buf);
					(temp_buf = ip_buf).Strip();
				}
				else
					temp_buf = sc_rec.Port;
				pack.PutExtStrData(pack.extssPort, temp_buf);
				pack.PutExtStrData(pack.extssAddedMsgSign, sc_rec.AddedMsgSign);
				if(p_ref->GetPropVlrString(PPOBJ_SCALE, sc_rec.ID, SCLPRP_EXPPATHS, temp_buf) > 0) {
					pack.PutExtStrData(pack.extssPaths, temp_buf);
				}
				{
					PPID temp_id = sc_rec.ID;
					THROW(sc_obj.PutPacket(&temp_id, &pack, 0));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	if(ppref_allocated)
		ZDELETE(PPRef);
	return ok;
}
//
//
//
class PPCvtPrjTask10702 : public PPTableConversion {
	int   Before6202;
	int   Before10702;
	Reference * P_Ref;
public:
	struct PrjTask_Before6202 {
		long   ID;
		long   ProjectID;
		long   Kind;
		char   Code[16];
		long   CreatorID;
		long   GroupID;
		long   EmployerID;
		long   ClientID;
		long   TemplateID;
		LDATE  Dt;
		LTIME  Tm;
		LDATE  StartDt;
		LTIME  StartTm;
		LDATE  EstFinishDt;
		LTIME  EstFinishTm;
		LDATE  FinishDt;
		LTIME  FinishTm;
		int16  Priority;
		int16  Status;
		int16  DrPrd;
		int16  DrKind;
		int32  DrDetail;
		long   Flags;
		long   DlvrAddrID;
		long   LinkTaskID;
		double Amount;
		int32  OpenCount;
		long   BillArID;
		uint8  Reserve[16];
		char   Descr[224];
		char   Memo[128];
	};
	struct PrjTask_Before10702 {
		int32  ID;
		int32  ProjectID;
		int32  Kind;
		char   Code[24];
		int32  CreatorID;
		int32  GroupID;
		int32  EmployerID;
		int32  ClientID;
		int32  TemplateID;
		LDATE  Dt;
		LTIME  Tm;
		LDATE  StartDt;
		LTIME  StartTm;
		LDATE  EstFinishDt;
		LTIME  EstFinishTm;
		LDATE  FinishDt;
		LTIME  FinishTm;
		int16  Priority;
		int16  Status;
		int16  DrPrd;
		int16  DrKind;
		int32  DrDetail;
		int32  Flags;
		int32  DlvrAddrID;
		int32  LinkTaskID;
		double Amount;
		int32  OpenCount;
		int32  BillArID;
		uint8  Reserve[16]; // raw
		char   Descr[256];
		char   Memo[1024]; // note
	};
	PPCvtPrjTask10702() : PPTableConversion(), Before6202(0), Before10702(0), P_Ref(0)
	{
	}
	~PPCvtPrjTask10702()
	{
		RestoreCommonRefFlags();
		ZDELETE(P_Ref);
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PrjTaskTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			Before6202 = 0;
			Before10702 = 0;
			if(stat.FixRecSize != sizeof(PrjTaskTbl::Rec)) {
				if(stat.FixRecSize == offsetof(PrjTask_Before10702, Memo))
					Before10702 = 1;
				else if(stat.FixRecSize < offsetof(PrjTask_Before10702, Memo)) {
					Before6202 = 1;
					Before10702 = 1;
				}
			}
			*pNeedConversion = BIN(Before10702 || Before6202);
			//*pNeedConversion = BIN(stat.FixRecSize < offsetof(PrjTaskTbl::Rec, Memo));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		int    ok = 1;
		PrjTaskTbl::Rec * p_data = static_cast<PrjTaskTbl::Rec *>(pNewTbl->getDataBuf());
		SString code_buf;
		SString descr_buf;
		SString memo_buf;
		const  PPID id = *static_cast<const long *>(pOldRec); // Идент записи в любом случае - самое первое поле 
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		if(Before6202) {
			PrjTask_Before6202 * p_old_rec = static_cast<PrjTask_Before6202 *>(pOldRec);
	#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
			FLD_ASSIGN(ID);
			FLD_ASSIGN(ProjectID);
			FLD_ASSIGN(Kind);
			FLD_ASSIGN(CreatorID);
			FLD_ASSIGN(GroupID);
			FLD_ASSIGN(EmployerID);
			FLD_ASSIGN(ClientID);
			FLD_ASSIGN(TemplateID);
			FLD_ASSIGN(Dt);
			FLD_ASSIGN(Tm);
			FLD_ASSIGN(StartDt);
			FLD_ASSIGN(StartTm);
			FLD_ASSIGN(EstFinishDt);
			FLD_ASSIGN(EstFinishTm);
			FLD_ASSIGN(FinishDt);
			FLD_ASSIGN(FinishTm);
			FLD_ASSIGN(Priority);
			FLD_ASSIGN(Status);
			FLD_ASSIGN(DrPrd);
			FLD_ASSIGN(DrKind);
			FLD_ASSIGN(DrDetail);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(DlvrAddrID);
			FLD_ASSIGN(LinkTaskID);
			FLD_ASSIGN(Amount);
			FLD_ASSIGN(OpenCount);
			FLD_ASSIGN(BillArID);
	#undef FLD_ASSIGN
			code_buf = p_old_rec->Code;
			descr_buf = p_old_rec->Descr;
			memo_buf = p_old_rec->Memo;
			STRNSCPY(p_data->Code, code_buf);
			//STRNSCPY(p_data->Descr, p_old_rec->Descr);
			//STRNSCPY(p_data->Memo, p_old_rec->Memo);
		}
		else if(Before10702) {
			PrjTask_Before10702 * p_old_rec = static_cast<PrjTask_Before10702 *>(pOldRec);
	#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
			FLD_ASSIGN(ID);
			FLD_ASSIGN(ProjectID);
			FLD_ASSIGN(Kind);
			FLD_ASSIGN(CreatorID);
			FLD_ASSIGN(GroupID);
			FLD_ASSIGN(EmployerID);
			FLD_ASSIGN(ClientID);
			FLD_ASSIGN(TemplateID);
			FLD_ASSIGN(Dt);
			FLD_ASSIGN(Tm);
			FLD_ASSIGN(StartDt);
			FLD_ASSIGN(StartTm);
			FLD_ASSIGN(EstFinishDt);
			FLD_ASSIGN(EstFinishTm);
			FLD_ASSIGN(FinishDt);
			FLD_ASSIGN(FinishTm);
			FLD_ASSIGN(Priority);
			FLD_ASSIGN(Status);
			FLD_ASSIGN(DrPrd);
			FLD_ASSIGN(DrKind);
			FLD_ASSIGN(DrDetail);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(DlvrAddrID);
			FLD_ASSIGN(LinkTaskID);
			FLD_ASSIGN(Amount);
			FLD_ASSIGN(OpenCount);
			FLD_ASSIGN(BillArID);
	#undef FLD_ASSIGN
			code_buf = p_old_rec->Code;
			descr_buf = p_old_rec->Descr;
			memo_buf = p_old_rec->Memo;
			STRNSCPY(p_data->Code, code_buf);
		}
		{
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PRJTASK, id, PPTRPROP_DESCR), descr_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PRJTASK, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		CATCH
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
			ok = 0;
		ENDCATCH
		return ok;
	}
};

class PPCvtProject10702 : public PPTableConversion {
	int   Before6202;
	int   Before10702;
	Reference * P_Ref;
public:
	struct Project_Before6202 {
		long   ID;
		long   Kind;
		long   ParentID;
		char   Name[48];
		char   Code[16];
		LDATE  Dt;
		LDATE  BeginDt;
		LDATE  EstFinishDt;
		LDATE  FinishDt;
		long   MngrID;
		long   ClientID;
		long   TemplateID;
		long   Status;
		long   Flags;
		long   BillOpID;
		uint8  Reserve[44];
		char   Descr[224];
		char   Memo[128];
	};
	struct Project_Before10702 {
		int32  ID;
		int32  Kind;
		int32  ParentID;
		char   Name[128];
		char   Code[24];
		LDATE  Dt;
		LDATE  BeginDt;
		LDATE  EstFinishDt;
		LDATE  FinishDt;
		int32  MngrID;
		int32  ClientID;
		int32  TemplateID;
		int32  Status;
		int32  Flags;
		int32  BillOpID;
		uint8  Reserve[44]; // raw
		char   Descr[256];
		char   Memo[1024]; // note
	};
	PPCvtProject10702() : Before6202(0), Before10702(0), P_Ref(0)
	{
	}
	~PPCvtProject10702()
	{
		RestoreCommonRefFlags();
		delete P_Ref;
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new ProjectTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			Before6202 = 0;
			Before10702 = 0;
			if(stat.FixRecSize != sizeof(ProjectTbl::Rec)) {
				if(stat.FixRecSize == offsetof(Project_Before10702, Memo))
					Before10702 = 1;
				else if(stat.FixRecSize < offsetof(Project_Before10702, Memo)) {
					Before6202 = 1;
					Before10702 = 1;
				}
			}
			*pNeedConversion = BIN(Before10702 || Before6202);
			//*pNeedConversion = BIN(stat.FixRecSize < offsetof(ProjectTbl::Rec, Memo));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ProjectTbl::Rec * p_data = static_cast<ProjectTbl::Rec *>(pNewTbl->getDataBuf());
		int    ok = 1;
		SString code_buf;
		SString descr_buf;
		SString memo_buf;
		const  PPID id = *static_cast<const long *>(pOldRec); // Идент записи в любом случае - самое первое поле 
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		if(Before6202) {
			const Project_Before6202 * p_old_rec = static_cast<const Project_Before6202 *>(pOldRec);
	#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Kind);
			FLD_ASSIGN(ParentID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			FLD_ASSIGN(Dt);
			FLD_ASSIGN(BeginDt);
			FLD_ASSIGN(EstFinishDt);
			FLD_ASSIGN(FinishDt);
			FLD_ASSIGN(MngrID);
			FLD_ASSIGN(ClientID);
			FLD_ASSIGN(TemplateID);
			FLD_ASSIGN(Status);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(BillOpID);
	#undef FLD_ASSIGN
			descr_buf = p_old_rec->Descr;
			memo_buf = p_old_rec->Memo;
		}
		else if(Before10702) {
			const Project_Before10702 * p_old_rec = static_cast<const Project_Before10702 *>(pOldRec);
	#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Kind);
			FLD_ASSIGN(ParentID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			FLD_ASSIGN(Dt);
			FLD_ASSIGN(BeginDt);
			FLD_ASSIGN(EstFinishDt);
			FLD_ASSIGN(FinishDt);
			FLD_ASSIGN(MngrID);
			FLD_ASSIGN(ClientID);
			FLD_ASSIGN(TemplateID);
			FLD_ASSIGN(Status);
			FLD_ASSIGN(Flags);
			FLD_ASSIGN(BillOpID);
	#undef FLD_ASSIGN
			descr_buf = p_old_rec->Descr;
			memo_buf = p_old_rec->Memo;
		}
		{
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PROJECT, id, PPTRPROP_DESCR), descr_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PROJECT, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		CATCH
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
			ok = 0;
		ENDCATCH
		return ok;
	}
};

int Convert10702()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtPrjTask10702 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtProject10702 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}

//@erik v10.7.3 {
int Convert10703()
{
	int    ok = 1;
	PPCommandMngr * p_mgr = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcDesktop, 0);
	THROW(p_mgr);
	THROW(p_mgr->ConvertDesktopTo(PPCommandMngr::fRWByXml));
	CATCHZOKPPERR
	delete p_mgr;
	return ok;
}
// } @erik

int Convert10903()
{
	int    ok = 1;
	PPCommandMngr * p_mgr_desktop = 0;
	PPCommandMngr * p_mgr_menu = 0;
	SysJournal * p_sj = 0;
	Reference * p_ref = 0;
	int   is_p_ref_allocated = 0;
	PPWaitStart();
	if(CurDict) {
		{
			LDATETIME moment;
			PPIDArray acn_list;
			acn_list.add(PPACN_EVENTTOKEN);
			THROW(p_sj = new SysJournal);
			if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_CVTCMDGROUP, &acn_list, &moment) > 0) {
				ok = -1; // Событие уже установлено
			}
			else {
				PPCommandGroup cg_desktop;
				PPCommandGroup cg_menu;
				THROW(p_mgr_desktop = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcDesktop, 0));
				THROW(p_mgr_menu = GetCommandMngr(PPCommandMngr::ctrfReadOnly, cmdgrpcMenu, 0));
				if(PPRef)
					p_ref = PPRef;
				else {
					p_ref = new Reference;
					is_p_ref_allocated = 1;
				}		
				if(p_ref) {
					SString db_symb;
					CurDict->GetDbSymb(db_symb);
					{
						p_mgr_desktop->Load__2(&cg_desktop, db_symb, PPCommandMngr::fRWByXml);
						p_mgr_menu->Load__2(&cg_menu, db_symb, PPCommandMngr::fRWByXml);
					}
					PPTransaction tra(1);
					THROW(tra);
					{
						PPSecur2 sec_rec;
						PPObjIDArray oid_list;
						StrAssocArray obj_name_list; // Для вывода в журнал: сопоставление наименований объектов с инексом (1..) в списке oid_list
						const  PPID obj_type_list[] = { PPOBJ_CONFIG, PPOBJ_USRGRP, PPOBJ_USR };
						for(uint objtypeidx = 0; objtypeidx < SIZEOFARRAY(obj_type_list); objtypeidx++) {
							for(SEnum en = p_ref->Enum(obj_type_list[objtypeidx], 0); en.Next(&sec_rec) > 0;) {
								oid_list.Add(sec_rec.Tag, sec_rec.ID);
								obj_name_list.Add(oid_list.getCount(), sec_rec.Name);
							}
						}
						{
							SString fmt_buf;
							SString msg_buf;
							SString oid_buf;
							SString temp_buf;
							for(uint oididx = 0; oididx < oid_list.getCount(); oididx++) {
								const PPObjID oid = oid_list.at(oididx);
								PPConfig cfg_rec;
								oid_buf.Z().Cat(oid.Obj).Semicol().Cat(oid.Id);
								if(obj_name_list.GetText(oididx+1, temp_buf)) {
									oid_buf.Space().CatChar('\'').Cat(temp_buf).CatChar('\'');
								}
								if(oid.Obj == PPOBJ_USR) {
									SString desk_name;
									PPConfigPrivate cfgp_rec;
									MEMSZERO(cfgp_rec);
									if(p_ref->GetConfig(oid.Obj, oid.Id, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)) > 0) {
										Reference::GetExField(&cfgp_rec, PCFGEXSTR_DESKTOPNAME, desk_name);
										if(cfgp_rec.DesktopID_Obsolete && !cfgp_rec.DesktopUuid) {
											const PPCommandItem * p_cg_item = cg_desktop.SearchByID(cfgp_rec.DesktopID_Obsolete, 0);
											if(p_cg_item && p_cg_item->IsKind(PPCommandItem::kGroup)) {
												const PPCommandGroup * p_cg_local = static_cast<const PPCommandGroup *>(p_cg_item);
												if(oneof2(p_cg_local->Type, cmdgrpcDesktop, cmdgrpcUndef) && !!p_cg_local->Uuid) {
													cfgp_rec.DesktopUuid = p_cg_local->Uuid;
													Reference::SetExField(&cfgp_rec, PCFGEXSTR_DESKTOPNAME, p_cg_local->Name);
													THROW(p_ref->SetConfig(oid.Obj, oid.Id, PPPRP_CFGPRIVATE, &cfgp_rec, sizeof(cfgp_rec)));
												}
											}
											uint msg_id = 0;
											if(!cfgp_rec.DesktopUuid) {
												// PPTXT_LOG_CMDGRPREFCVG_DTNOSUBSTFOUND Для объекта {%s} не удалось найти подстановку для старого идентификатора рабочего стола (%s)
												msg_id = PPTXT_LOG_CMDGRPREFCVG_DTNOSUBSTFOUND;
												temp_buf.Z().Cat(cfgp_rec.DesktopID_Obsolete);
											}
											else {
												// PPTXT_LOG_CMDGRPREFCVG_DTSETTLED Для объекта {%s} установлен новый идентификатор рабочего стола (%s)
												msg_id = PPTXT_LOG_CMDGRPREFCVG_DTSETTLED;
												temp_buf.Z().Cat(cfgp_rec.DesktopID_Obsolete).Cat("-->").Cat(cfgp_rec.DesktopUuid, S_GUID::fmtIDL|S_GUID::fmtLower);
											}
											if(msg_id) {
												PPLoadText(msg_id, fmt_buf);
												msg_buf.Printf(fmt_buf.cptr(), oid_buf.cptr(), temp_buf.cptr());
												PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME);
											}
										}
									}
								}
								if(p_ref->GetConfig(oid.Obj, oid.Id, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)) > 0) {
									//PPTXT_LOG_CMDGRPREFCVG_DTSETTLED          "Для объекта {oid} установлен новый идентификатор рабочего стола (%s)"
									//PPTXT_LOG_CMDGRPREFCVG_MENUSETTLED        "Для объекта {oid} установлен новый идентификатор меню (%s)"
									//PPTXT_LOG_CMDGRPREFCVG_DTNOSUBSTFOUND     "Для объекта {oid} не удалось найти подстановку для старого идентификатора рабочего стола (%s)"
									//PPTXT_LOG_CMDGRPREFCVG_MENUNOSUBSTFOUND   "Для объекта {oid} не удалось найти подстановку для старого идентификатора меню (%s)"
									int local_do_update = 0;
									if(cfg_rec.DesktopID_Obsolete && !cfg_rec.DesktopUuid_) {
										const PPCommandItem * p_cg_item = cg_desktop.SearchByID(cfg_rec.DesktopID_Obsolete, 0);
										if(p_cg_item && p_cg_item->IsKind(PPCommandItem::kGroup)) {
											const PPCommandGroup * p_cg_local = static_cast<const PPCommandGroup *>(p_cg_item);
											if(oneof2(p_cg_local->Type, cmdgrpcDesktop, cmdgrpcUndef) && !!p_cg_local->Uuid) {
												cfg_rec.DesktopUuid_ = p_cg_local->Uuid;
												local_do_update = 1;
											}
										}
										uint msg_id = 0;
										if(!cfg_rec.DesktopUuid_) {
											// PPTXT_LOG_CMDGRPREFCVG_DTNOSUBSTFOUND Для объекта {%s} не удалось найти подстановку для старого идентификатора рабочего стола (%s)
											msg_id = PPTXT_LOG_CMDGRPREFCVG_DTNOSUBSTFOUND;
											temp_buf.Z().Cat(cfg_rec.DesktopID_Obsolete);
										}
										else {
											// PPTXT_LOG_CMDGRPREFCVG_DTSETTLED Для объекта {%s} установлен новый идентификатор рабочего стола (%s)
											msg_id = PPTXT_LOG_CMDGRPREFCVG_DTSETTLED;
											temp_buf.Z().Cat(cfg_rec.DesktopID_Obsolete).Cat("-->").Cat(cfg_rec.DesktopUuid_, S_GUID::fmtIDL|S_GUID::fmtLower);
										}
										if(msg_id) {
											PPLoadText(msg_id, fmt_buf);
											msg_buf.Printf(fmt_buf.cptr(), oid_buf.cptr(), temp_buf.cptr());
											PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME);
										}
									}
									if(cfg_rec.MenuID_Obsolete && !cfg_rec.MenuUuid) {
										const PPCommandItem * p_cg_item = cg_menu.SearchByID(cfg_rec.MenuID_Obsolete, 0);
										if(p_cg_item && p_cg_item->IsKind(PPCommandItem::kGroup)) {
											const PPCommandGroup * p_cg_local = static_cast<const PPCommandGroup *>(p_cg_item);
											if(oneof2(p_cg_local->Type, cmdgrpcMenu, cmdgrpcUndef) && !!p_cg_local->Uuid) {
												cfg_rec.MenuUuid = p_cg_local->Uuid;
												local_do_update = 1;
											}
										}
										uint msg_id = 0;
										if(!cfg_rec.MenuUuid) {
											// PPTXT_LOG_CMDGRPREFCVG_MENUNOSUBSTFOUND Для объекта {%s} не удалось найти подстановку для старого идентификатора меню (%s)
											msg_id = PPTXT_LOG_CMDGRPREFCVG_MENUNOSUBSTFOUND;
											temp_buf.Z().Cat(cfg_rec.MenuID_Obsolete);
										}
										else {
											// PPTXT_LOG_CMDGRPREFCVG_MENUSETTLED Для объекта {%s} установлен новый идентификатор меню (%s)
											msg_id = PPTXT_LOG_CMDGRPREFCVG_MENUSETTLED;
											temp_buf.Z().Cat(cfg_rec.MenuID_Obsolete).Cat("-->").Cat(cfg_rec.MenuUuid, S_GUID::fmtIDL|S_GUID::fmtLower);
										}
										if(msg_id) {
											PPLoadText(msg_id, fmt_buf);
											msg_buf.Printf(fmt_buf.cptr(), oid_buf.cptr(), temp_buf.cptr());
											PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME);
										}
									}
									if(local_do_update) {
										THROW(p_ref->SetConfig(oid.Obj, oid.Id, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)));
									}
								}
							}
						}
					}
					THROW(p_sj->LogEvent(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, PPEVTOK_CVTCMDGROUP, 0, 0/*use_ta*/));
					THROW(tra.Commit());
				}
			}
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	ZDELETE(p_sj);
	if(is_p_ref_allocated)
		ZDELETE(p_ref);
	delete p_mgr_desktop;
	delete p_mgr_menu;
	return ok;
}
//
//
//
class PPCvtEgaisRefA10905 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new EgaisRefATbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(EgaisRefATbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct EgaisRefATblRec_Before10905 {
			long   ID;
			char   RefACode[20];
			char   AlcCode[24];
			char   ManufRarIdent[16];
			char   ImporterRarIdent[16];
			int16  CountryCode;
			int32  Volume;         // x100000
			LDATE  BottlingDate;
			LDATE  ActualDate;
			long   Flags;
			uint8  Reserve[12];
		};
		EgaisRefATbl::Rec * p_data = static_cast<EgaisRefATbl::Rec *>(pNewTbl->getDataBuf());
		EgaisRefATblRec_Before10905 * p_old_rec = static_cast<EgaisRefATblRec_Before10905 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(ID);
		CPYFLD(CountryCode);
		CPYFLD(Volume);
		CPYFLD(BottlingDate);
		CPYFLD(ActualDate);
		CPYFLD(Flags);
#undef CPYFLD
		STRNSCPY(p_data->RefACode, p_old_rec->RefACode);
		STRNSCPY(p_data->AlcCode, p_old_rec->AlcCode);
		STRNSCPY(p_data->ManufRarIdent, p_old_rec->ManufRarIdent);
		STRNSCPY(p_data->ImporterRarIdent, p_old_rec->ImporterRarIdent);
		return 1;
	}
};

CONVERT_PROC(Convert10905, PPCvtEgaisRefA10905);
//
//
//
class PPCvtTSession11004 : public PPTableConversion {
	Reference * P_Ref;
public:
	PPCvtTSession11004() : P_Ref(0)
	{
	}
	~PPCvtTSession11004()
	{
		RestoreCommonRefFlags();
		ZDELETE(P_Ref);
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new TSessionTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(TSessionTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct TSessionTblRec_Before11004 {
			long   ID;
			long   ParentID;
			long   Num;
			long   TechID;
			long   PrcID;
			LDATE  StDt;
			LTIME  StTm;
			LDATE  FinDt;
			LTIME  FinTm;
			int16  Incomplete;
			int16  Status;
			long   Flags;
			long   ArID;
			long   Ar2ID;
			long   PlannedTiming;
			double PlannedQtty;
			double ActQtty;
			long   OrderLotID;
			long   PrevSessID;
			double Amount;
			long   LinkBillID;
			long   SCardID;
			long   ToolingTime;
			long   CCheckID_;
			char   Memo[512];
		};
		int    ok = 1;
		PPProcessorPacket::ExtBlock ext;
		TSessionTbl::Rec * p_data = static_cast<TSessionTbl::Rec *>(pNewTbl->getDataBuf());
		TSessionTblRec_Before11004 * p_old_rec = static_cast<TSessionTblRec_Before11004 *>(pOldRec);
		const  PPID id = p_old_rec->ID;
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		PPObjTSession::Implement_GetExtention(p_ref, id, &ext);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
			CPYFLD(ID);
			CPYFLD(ParentID);
			CPYFLD(Num);
			CPYFLD(TechID);
			CPYFLD(PrcID);
			CPYFLD(StDt);
			CPYFLD(StTm);
			CPYFLD(FinDt);
			CPYFLD(FinTm);
			CPYFLD(Incomplete);
			CPYFLD(Status);
			CPYFLD(Flags);
			CPYFLD(ArID);
			CPYFLD(Ar2ID);
			CPYFLD(PlannedTiming);
			CPYFLD(PlannedQtty);
			CPYFLD(ActQtty);
			CPYFLD(OrderLotID);
			CPYFLD(PrevSessID);
			CPYFLD(Amount);
			CPYFLD(LinkBillID);
			CPYFLD(SCardID);
			CPYFLD(ToolingTime);
			CPYFLD(CCheckID_);
#undef CPYFLD
		{
			ext.PutExtStrData(PRCEXSTR_MEMO, p_old_rec->Memo);
			THROW(PPObjTSession::Implement_PutExtention(p_ref, id, &ext, 0));
		}
		CATCH
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_COMP);
			ok = 0;
		ENDCATCH
		return ok;
	}
};

class PPCvtTSessLine11004 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new TSessLineTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(TSessLineTbl::Rec));
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct TSessLineTblRec_Before11004 {
			long   TSessID;
			long   OprNo;
			long   GoodsID;
			long   LotID;
			long   UserID;
			int16  Sign;
			int16  Reserve;
			LDATE  Dt;
			LTIME  Tm;
			long   Flags;
			double Qtty;
			char   Serial[24];
			double Price;
			double WtQtty;
			LDATE  Expiry;
			double Discount;
			long   Reserve2;
		};
		TSessLineTbl::Rec * p_data = static_cast<TSessLineTbl::Rec *>(pNewTbl->getDataBuf());
		TSessLineTblRec_Before11004 * p_old_rec = static_cast<TSessLineTblRec_Before11004 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
			CPYFLD(TSessID);
			CPYFLD(OprNo);
			CPYFLD(GoodsID);
			CPYFLD(LotID);
			CPYFLD(UserID);
			CPYFLD(Sign);
			CPYFLD(Reserve);
			CPYFLD(Dt);
			CPYFLD(Tm);
			CPYFLD(Flags);
			CPYFLD(Qtty);
			CPYFLD(Price);
			CPYFLD(WtQtty);
			CPYFLD(Expiry);
			CPYFLD(Discount);
#undef CPYFLD
		STRNSCPY(p_data->Serial, p_old_rec->Serial);
		return 1;
	}
};

int Convert11004()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtTSessLine11004 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtTSession11004 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
//
//
//
class PPCvtBill11112 : public PPTableConversion {
	enum {
		billrecfmtCurrent = 0,
		billrecfmtBefore4108,
		billrecfmtBefore4911,
		billrecfmtBefore11112,
	};
	int   RecFmt;
	Reference * P_Ref;
	struct BillRec_Before4108 { // Size = 76+160
		long   ID;       // Ид. документа
		char   Code[10];    // Код документа
		LDATE  Dt;          // Дата документа
		long   BillNo;      // Номер документа за день
		long   OprKind;     // Вид операции          ->Ref(PPOBJ_OPRKIND)
		long   UserID;      // Пользователь          ->Ref(PPOBJ_USR)
		long   Location;    // Позиция               ->Location.ID
		long   Object;      // Контрагент            ->Article.ID
		long   Object2;     // Дополнительный объект ->Article.ID
		long   CurID;       // Валюта (0 - базовая)  ->Ref(PPOBJ_CURRENCY)
		double CRate;       // Курс валюты для пересчета в базовую валюту
		char   Amount[8];   // Номинальная сумма (в единицах CurID)
		long   LinkBillID;  // Связанный документ    ->Bill.ID
		long   Flags;       // Флаги
		int16  AccessLevel; // Уровень доступа к документу
		char   Memo[160];   // Примечание
	};
	struct BillRec_Before4911 { // Size = 76+160
		long   ID;       // Ид. документа
		char   Code[10];    // Код документа
		LDATE  Dt;          // Дата документа
		long   BillNo;      // Номер документа за день
		long   OprKind;     // Вид операции          ->Ref(PPOBJ_OPRKIND)
		long   UserID;      // Пользователь          ->Ref(PPOBJ_USR)
		long   Location;    // Позиция               ->Location.ID
		long   Object;      // Контрагент            ->Article.ID
		long   Object2;     // Дополнительный объект ->Article.ID
		long   CurID;       // Валюта (0 - базовая)  ->Ref(PPOBJ_CURRENCY)
		double CRate;       // Курс валюты для пересчета в базовую валюту
		char   Amount[8];   // Номинальная сумма (в единицах CurID)
		long   LinkBillID;  // Связанный документ    ->Bill.ID
		long   Flags;       // Флаги
		int16  AccessLevel; // Уровень доступа к документу
		long   SCardID;     // @v4.1.8 ->SCard.ID
		char   Memo[160];   // Примечание
	};
	struct BillRec_Before11112 {
		long   ID;
		char   Code[24];
		LDATE  Dt;
		long   BillNo;
		LDATE  DueDate;
		long   OpID;
		long   StatusID;
		long   UserID;
		long   MainOrgID;
		long   LocID;
		long   Object;
		long   Object2;
		long   CurID;
		double CRate;
		double Amount;
		long   LinkBillID;
		long   Flags;
		long   Flags2;
		long   SCardID;
		LDATE  PeriodLow;
		LDATE  PeriodUpp;
		int16  LastRByBill; 
		int16  EdiOp;
		double PaymAmount;
		long   AgtBillID;
		char   Memo[512];
	};
public:
	PPCvtBill11112() : RecFmt(0), P_Ref(0)
	{
	}
	~PPCvtBill11112()
	{
		RestoreCommonRefFlags();
		ZDELETE(P_Ref);
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = tbl->getRecSize();
			if(recsz < (sizeof(BillRec_Before4108) - sizeof(((BillRec_Before4108 *)0)->Memo)))
				RecFmt = billrecfmtBefore4108;
			else if(recsz < (sizeof(BillRec_Before4911) - sizeof(((BillRec_Before4911 *)0)->Memo)))
				RecFmt = billrecfmtBefore4911;
			else if(recsz < sizeof(BillTbl::Rec))
				RecFmt = billrecfmtBefore11112;
			else
				RecFmt = 0;
			if(RecFmt)
				*pNeedConversion = 1;
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		#define CF(f) p_data->f = p_old_rec->f
		int    ok = 1;
		SString memo_buf;
		SString code_buf;
		BillTbl::Rec * p_data = static_cast<BillTbl::Rec *>(tbl->getDataBuf());
		tbl->clearDataBuf();
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		THROW(p_ref);
		THROW(oneof3(RecFmt, billrecfmtBefore4108, billrecfmtBefore4911, billrecfmtBefore11112));
		const  PPID id = *static_cast<const long *>(rec); // Идент записи в любом случае - самое первое поле 
		if(RecFmt == billrecfmtBefore4108) {
			const BillRec_Before4108 * p_old_rec = static_cast<const BillRec_Before4108 *>(rec);
			CF(ID);
			//char   Code[24];
			CF(Dt);
			CF(BillNo);
			//CF(DueDate);
			//CF(OpID);
			p_data->OpID = p_old_rec->OprKind;
			//CF(StatusID);
			CF(UserID);
			//long   MainOrgID;
			//CF(LocID);
			p_data->LocID = p_old_rec->Location;
			CF(Object);
			CF(Object2);
			CF(CurID);
			CF(CRate);
			//CF(Amount);
			p_data->Amount = MONEYTOLDBL(p_old_rec->Amount);
			CF(LinkBillID);
			CF(Flags);
			//CF(Flags2);
			//CF(SCardID);
			//CF(PeriodLow);
			//CF(PeriodUpp);
			//CF(LastRByBill); 
			//CF(EdiOp);
			//CF(PaymAmount);
			//CF(AgtBillID);
			//char   Memo[512];
			//STRNSCPY(p_data->Code, p_old_rec->Code);
			code_buf = p_old_rec->Code;
			memo_buf = p_old_rec->Memo;
			*pNewRecLen = -1;
		}
		else if(RecFmt == billrecfmtBefore4911) {
			const BillRec_Before4911 * p_old_rec = static_cast<const BillRec_Before4911 *>(rec);
			CF(ID);
			//char   Code[24];
			CF(Dt);
			CF(BillNo);
			//CF(DueDate);
			//CF(OpID);
			p_data->OpID = p_old_rec->OprKind;
			//CF(StatusID);
			CF(UserID);
			//long   MainOrgID;
			//CF(LocID);
			p_data->LocID = p_old_rec->Location;
			CF(Object);
			CF(Object2);
			CF(CurID);
			CF(CRate);
			//CF(Amount);
			p_data->Amount = MONEYTOLDBL(p_old_rec->Amount);
			CF(LinkBillID);
			CF(Flags);
			//CF(Flags2);
			CF(SCardID);
			//CF(PeriodLow);
			//CF(PeriodUpp);
			//CF(LastRByBill); 
			//CF(EdiOp);
			//CF(PaymAmount);
			//CF(AgtBillID);
			//char   Memo[512];
			//STRNSCPY(p_data->Code, p_old_rec->Code);
			code_buf = p_old_rec->Code;
			memo_buf = p_old_rec->Memo;
		}
		else if(RecFmt == billrecfmtBefore11112) {
			const BillRec_Before11112 * p_old_rec = static_cast<const BillRec_Before11112 *>(rec);
			CF(ID);
			//char   Code[24];
			CF(Dt);
			CF(BillNo);
			CF(DueDate);
			CF(OpID);
			CF(StatusID);
			CF(UserID);
			//long   MainOrgID;
			CF(LocID);
			CF(Object);
			CF(Object2);
			CF(CurID);
			CF(CRate);
			CF(Amount);
			CF(LinkBillID);
			CF(Flags);
			CF(Flags2);
			CF(SCardID);
			CF(PeriodLow);
			CF(PeriodUpp);
			CF(LastRByBill); 
			CF(EdiOp);
			CF(PaymAmount);
			CF(AgtBillID);
			//char   Memo[512];
			//STRNSCPY(p_data->Code, p_old_rec->Code);
			code_buf = p_old_rec->Code;
			memo_buf = p_old_rec->Memo;
		}
		else {
			assert(0);
		}
		{
			code_buf.Strip();
			if(code_buf.C(0) == '!' && p_data->Flags & BILLF_WHITELABEL) {
				code_buf.ShiftLeft();
			}
			{
				SString long_code_buf;
				if(p_ref->Ot.GetTagStr(PPOBJ_BILL, id, PPTAG_BILL_LONGCODE, long_code_buf) > 0) {
					if(long_code_buf.HasPrefix(code_buf) && long_code_buf.Len() < sizeof(p_data->Code)) {
						code_buf = long_code_buf;
						THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, id, PPTAG_BILL_LONGCODE, 0));
					}
				}
			}
			STRNSCPY(p_data->Code, code_buf);
			if(memo_buf == "N2") {
				p_data->Flags2 |= BILLF2_FORCEDRECEIPT;
				memo_buf.Z();
			}
			THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_BILL, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		CATCHZOK
		return ok;
		#undef CF
	}
};

class PPCvtTech11112 : public PPTableConversion {
	enum {
		recfmtCurrent = 0,
		recfmtBefore7506,
		recfmtBefore11112,
	};
	int    RecFmt;
	long   OrderN_Counter;
	Reference * P_Ref;
	struct TechRec_Before11112 {
		int32  ID;
		char   Code[24];
		int32  PrcID;
		int32  GoodsID;
		int32  GStrucID;
		int32  Flags;
		int16  Sign;
		int16  Kind;
		int32  PrevGoodsID;
		int32  Duration;
		double Cost;
		double Capacity;
		double Rounding;
		int32  TransClsID;
		int32  TransMask;
		float  InitQtty;
		int32  ParentID;
		int32  OrderN;
		int16  CipMax;
		uint8  Reserve3[2]; // raw
		char   Memo[512];  // note
	};
public:
	PPCvtTech11112() : PPTableConversion(), P_Ref(0), RecFmt(0), OrderN_Counter(0)
	{
	}
	~PPCvtTech11112()
	{
		RestoreCommonRefFlags();
		delete P_Ref;
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		TechTbl * p_tbl = new TechTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys = 0;
			const  RECORDSIZE recsz = p_tbl->getRecSize();
			p_tbl->getNumKeys(&num_keys);
			if(num_keys < 6)
				RecFmt = recfmtBefore7506;
			else if(recsz < sizeof(TechTbl::Rec))
				RecFmt = recfmtBefore11112;
			else
				RecFmt = recfmtCurrent;
			*pNeedConversion = BIN(RecFmt != recfmtCurrent);
		}
		OrderN_Counter = 0;
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		#define CF(f) p_data->f = p_old_rec->f
		int    ok = 1;
		SString memo_buf;
		TechTbl::Rec * p_data = static_cast<TechTbl::Rec *>(pNewTbl->getDataBuf());
		const TechRec_Before11112 * p_old_rec = static_cast<TechRec_Before11112 *>(pOldRec);
		pNewTbl->clearDataBuf();
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		THROW(p_ref);
		assert(oneof2(RecFmt, recfmtBefore7506, recfmtBefore11112));
		THROW(oneof2(RecFmt, recfmtBefore7506, recfmtBefore11112));
		const  PPID id = *static_cast<const long *>(pOldRec); // Идент записи в любом случае - самое первое поле 
		{
			CF(ID);
			CF(PrcID);
			CF(GoodsID);
			CF(GStrucID);
			CF(Flags);
			CF(Sign);
			CF(Kind);
			CF(PrevGoodsID);
			CF(Duration);
			CF(Cost);
			CF(Capacity);
			CF(Rounding);
			CF(TransClsID);
			CF(TransMask);
			CF(InitQtty);
			CF(ParentID);
			CF(OrderN);
			CF(CipMax);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			//uint8  Reserve3[2]; // raw
			//char   Memo[512];  // note
		}
		(memo_buf = p_old_rec->Memo).Strip();
		if(RecFmt == recfmtBefore7506) {
			p_data->ParentID = 0;
			p_data->OrderN = ++OrderN_Counter;
		}
		THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_TECH, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		CATCHZOK
		return ok;
		#undef CF
	}
};

class PPCvtPerson11112 : public PPTableConversion {
	enum {
		recfmtCurrent = 0,
		recfmtBefore6202,
		recfmtBefore11112,
	};
	int    RecFmt;
	Reference * P_Ref;
public:
	struct Person_Before6202 {
		long   ID;
		char   Name[48];
		long   Status;
		long   MainLoc;
		long   Flags;
		long   RLoc;
		long   Division;
		long   Position;
		long   CatID;
		char   Memo[128];
	};
	struct Person_Before11112 {
		int32  ID;
		char   Name[128];
		int32  Status;
		int32  MainLoc;
		int32  RLoc;
		int32  CatID;
		int32  Flags;
		int32  Division;
		int32  Position;
		char   Memo[512];
	};
	PPCvtPerson11112() : P_Ref(0), RecFmt(recfmtCurrent)
	{
	}
	~PPCvtPerson11112()
	{
		RestoreCommonRefFlags();
		delete P_Ref;
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PersonTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			//int16 flags = 0;
			tbl->GetFileStat(-1, &stat);
			//tbl->getTabFlags(&flags);
			//*pNeedConversion = (flags & XTF_VLR) ? 0 : 1;
			if(stat.FixRecSize < offsetof(Person_Before11112, Memo))
				RecFmt = recfmtBefore6202;
			else if(stat.Flags & XTF_VLR) // В версии v11.1.12 убрали поле Memo и таблица более не использует записи переменной длины (VLR)
				RecFmt = recfmtBefore11112;
			else
				RecFmt = 0;
			*pNeedConversion = BIN(RecFmt != 0);
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		#define CF(f) p_data->f = p_old_rec->f
		int    ok = 1;
		SString memo_buf;
		pNewTbl->clearDataBuf();
		PersonTbl::Rec * p_data = static_cast<PersonTbl::Rec *>(pNewTbl->getDataBuf());
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		THROW(p_ref);
		assert(oneof2(RecFmt, recfmtBefore6202, recfmtBefore11112));
		THROW(oneof2(RecFmt, recfmtBefore6202, recfmtBefore11112));
		const  PPID id = *static_cast<const long *>(pOldRec); // Идент записи в любом случае - самое первое поле 
		if(RecFmt == recfmtBefore6202) {
			const Person_Before6202 * p_old_rec = static_cast<const Person_Before6202 *>(pOldRec);
			CF(ID);
			CF(Status);
			CF(MainLoc);
			CF(Flags);
			CF(RLoc);
			CF(Division);
			CF(Position);
			CF(CatID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			//STRNSCPY(p_data->Memo, p_old_rec->Memo);
			(memo_buf = p_old_rec->Memo).Strip();
		}
		else if(RecFmt == recfmtBefore11112) {
			const Person_Before11112 * p_old_rec = static_cast<const Person_Before11112 *>(pOldRec);
			CF(ID);
			CF(Status);
			CF(MainLoc);
			CF(RLoc);
			CF(CatID);
			CF(Flags);
			CF(Division);
			CF(Position);
			//char   Memo[512];
			STRNSCPY(p_data->Name, p_old_rec->Name);
			(memo_buf = p_old_rec->Memo).Strip();
		}
		THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PERSON, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		CATCHZOK
		return ok;
		#define CF(f) p_data->f = p_old_rec->f
	}
};


class PPCvtPersonEvent11112 : public PPTableConversion {
	Reference * P_Ref;
public:
	struct PersonEvent_Before11112 {
		int32  ID;
		LDATE  Dt;
		int32  OprNo;
		int32  OpID;
		int32  PersonID;
		int32  SecondID;
		int32  LocationID;
		int32  Extra;
		int32  Flags;
		int32  LinkBillID;
		LTIME  Tm;
		int16  EstDuration;
		uint8  Reserve[18]; // raw
		int32  PrmrSCardID;
		int32  ScndSCardID;
		char   Memo[512];  // note
	};
	PPCvtPersonEvent11112() : P_Ref(0)
	{
	}
	~PPCvtPersonEvent11112()
	{
		RestoreCommonRefFlags();
		delete P_Ref;
	}
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PersonEventTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			tbl->GetFileStat(-1, &stat);
			*pNeedConversion = (stat.Flags & XTF_VLR) ? 1 : 0; // В версии v11.1.12 убрали поле Memo и таблица более не использует записи переменной длины (VLR)
		}
		return tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		#define CF(f) p_data->f = p_old_rec->f
		int    ok = 1;
		SString memo_buf;
		pNewTbl->clearDataBuf();
		PersonEventTbl::Rec * p_data = static_cast<PersonEventTbl::Rec *>(pNewTbl->getDataBuf());
		Reference * p_ref = GetReferenceInstance(&P_Ref);
		THROW(p_ref);
		const  PPID id = *static_cast<const long *>(pOldRec); // Идент записи в любом случае - самое первое поле 
		{
			const PersonEvent_Before11112 * p_old_rec = static_cast<const PersonEvent_Before11112 *>(pOldRec);
			CF(ID);
			CF(Dt);
			CF(OprNo);
			CF(OpID);
			CF(PersonID);
			CF(SecondID);
			CF(LocationID);
			CF(Extra);
			CF(Flags);
			CF(LinkBillID);
			CF(Tm);
			CF(EstDuration);
			//uint8  Reserve[18]; // raw
			CF(PrmrSCardID);
			CF(ScndSCardID);
			(memo_buf = p_old_rec->Memo).Strip();
		}
		THROW(p_ref->UtrC.SetTextUtf8(TextRefIdent(PPOBJ_PERSONEVENT, id, PPTRPROP_MEMO), memo_buf.Transf(CTRANSF_INNER_TO_UTF8), 0));
		CATCHZOK
		return ok;
		#define CF(f) p_data->f = p_old_rec->f
	}
};

int Convert11112()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtBill11112 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtTech11112 cvt02;
		THROW(cvt02.Convert());
	}
	{
		PPCvtPerson11112 cvt03;
		THROW(cvt03.Convert());
	}
	{
		PPCvtPersonEvent11112 cvt04;
		THROW(cvt04.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}

int Convert11200()
{
	int    ok = 1;
	SysJournal * p_sj = 0;
	Reference * p_ref = 0;
	bool   ref_is_common = false;
	PPWaitStart();
	{
		//
		// Необходимо зафиксировать в системном журнале событие,
		// которое будет разграничивать применении старой
		// и новой схемы хранения соглашений
		//
		LDATETIME moment;
		PPIDArray acn_list;
		acn_list.add(PPACN_EVENTTOKEN);
		THROW(p_sj = new SysJournal);
		if(p_sj->GetLastObjEvent(PPOBJ_EVENTTOKEN, PPEVTOK_CVTCLIAGT11200, &acn_list, &moment) > 0)
			ok = -1; // Событие уже установлено
		else {
			p_ref = PPRef;
			if(p_ref) {
				ref_is_common = true;
			}
			else {
				p_ref = new Reference;
				ref_is_common = false;
			}
			if(p_ref) {
				PPTransaction tra(1);
				THROW(tra);
				THROW(PPObjArticle::ConvertClientAgreements_11200(p_ref, 0));
				THROW(p_sj->LogEvent(PPACN_EVENTTOKEN, PPOBJ_EVENTTOKEN, PPEVTOK_CVTCLIAGT11200, 0, 0/*use_ta*/));
				THROW(tra.Commit());
			}
		}
	}
	PPWaitStop();
	CATCHZOKPPERR
	ZDELETE(p_sj);
	if(p_ref && !ref_is_common) {
		delete p_ref;
	}
	return ok;
}
//
//
//
#if 0 // Конвертация совмещена с v12.0.0 {
class PPCvtRegister8306 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(RegisterTbl::Rec) || num_keys < 5);
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		#pragma pack(push, 1)
		struct RegisterTblRec_Before8306 {
			long   ID;
			long   PsnID;
			long   PsnEventID;
			long   RegTypeID;
			LDATE  Dt;
			long   RegOrgID;
			char   Serial[12];
			char   Num[32];
			LDATE  Expiry;
			long   UniqCntr;
			long   Flags;
			long   ExtID;
		};
		#pragma pack(pop)
		RegisterTbl::Rec * p_data = static_cast<RegisterTbl::Rec *>(pNewTbl->getDataBuf());
		RegisterTblRec_Before8306 * p_old_rec = static_cast<RegisterTblRec_Before8306 *>(pOldRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(ID);
		CPYFLD(PsnEventID);
		CPYFLD(RegTypeID);
		CPYFLD(Dt);
		CPYFLD(RegOrgID);
		CPYFLD(Expiry);
		CPYFLD(UniqCntr);
		CPYFLD(Flags);
		CPYFLD(ExtID); // @v9.0.4 Reserve-->ExtID
#undef CPYFLD
		if(p_old_rec->PsnID) {
			p_data->ObjType = PPOBJ_PERSON;
			p_data->ObjID = p_old_rec->PsnID;
		}
		STRNSCPY(p_data->Serial, p_old_rec->Serial);
		STRNSCPY(p_data->Num, p_old_rec->Num);
		return 1;
	}
};

CONVERT_PROC(Convert8306, PPCvtRegister8306);
#endif // } 0

class PPCvtRegister12000 : public PPTableConversion {
	enum {
		recfmtCurrent = 0,
		recfmtBefore8306,
		recfmtBefore12000,
	};
	int    RecFmt;
public:
	PPCvtRegister12000() : PPTableConversion(), RecFmt(recfmtCurrent)
	{
	}

	#pragma pack(push, 1)
	struct RegisterTblRec_Before8306 {
		long   ID;
		long   PsnID;
		long   PsnEventID;
		long   RegTypeID;
		LDATE  Dt;
		long   RegOrgID;
		char   Serial[12];
		char   Num[32];
		LDATE  Expiry;
		long   UniqCntr;
		long   Flags;
		long   ExtID;
	};

	struct RegisterTblRec_Before12000 {
		long   ID;
		long   ObjType;
		long   ObjID;
		long   PsnEventID;
		long   RegTypeID;
		LDATE  Dt;
		long   RegOrgID;
		char   Serial[12];
		char   Num[32];
		LDATE  Expiry;
		long   UniqCntr;
		long   Flags;
		long   ExtID;
	};
	#pragma pack(pop)
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			const RECORDSIZE recsz = p_tbl->getRecSize();
			if(recsz < sizeof(RegisterTblRec_Before12000) || num_keys < 5) {
				RecFmt = recfmtBefore8306;
				*pNeedConversion = 1;
			}
			else if(recsz < sizeof(RegisterTbl::Rec)) {
				RecFmt = recfmtBefore12000;
				*pNeedConversion = 1;
			}
			else
				*pNeedConversion = 0;
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		RegisterTbl::Rec * p_data = static_cast<RegisterTbl::Rec *>(pNewTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
		if(RecFmt = recfmtBefore12000) {
			RegisterTblRec_Before12000 * p_old_rec = static_cast<RegisterTblRec_Before12000 *>(pOldRec);
	#define CPYFLD(f) p_data->f = p_old_rec->f
			CPYFLD(ID);
			CPYFLD(ObjType);
			CPYFLD(ObjID);
			CPYFLD(PsnEventID);
			CPYFLD(RegTypeID);
			CPYFLD(Dt);
			CPYFLD(RegOrgID);
			CPYFLD(Expiry);
			CPYFLD(UniqCntr);
			CPYFLD(Flags);
			CPYFLD(ExtID);
	#undef CPYFLD
			STRNSCPY(p_data->Serial, p_old_rec->Serial);
			STRNSCPY(p_data->Num, p_old_rec->Num);
		}
		else if(RecFmt = recfmtBefore8306) {
			RegisterTblRec_Before8306 * p_old_rec = static_cast<RegisterTblRec_Before8306 *>(pOldRec);
	#define CPYFLD(f) p_data->f = p_old_rec->f
			CPYFLD(ID);
			CPYFLD(PsnEventID);
			CPYFLD(RegTypeID);
			CPYFLD(Dt);
			CPYFLD(RegOrgID);
			CPYFLD(Expiry);
			CPYFLD(UniqCntr);
			CPYFLD(Flags);
			CPYFLD(ExtID);
	#undef CPYFLD
			if(p_old_rec->PsnID) {
				p_data->ObjType = PPOBJ_PERSON;
				p_data->ObjID = p_old_rec->PsnID;
			}
			STRNSCPY(p_data->Serial, p_old_rec->Serial);
			STRNSCPY(p_data->Num, p_old_rec->Num);			
		}
		return 1;
	}
};

CONVERT_PROC(Convert12000, PPCvtRegister12000);
//
//
//
class PPCvtSCardOp12005 : public PPTableConversion {
public:
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SCardOpTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys > 2 || recsz < sizeof(SCardOpTbl::Rec)); // @v12.0.8 Совмещена конвертация 7.7.12 с этой // @v12.2.0 @fix (num_keys < 5)-->(num_keys > 2)
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct SCardOpRec_BeforePPCvtSCardOp12005 { // size=60
			int32  SCardID;
			LDATE  Dt;
			LTIME  Tm;
			int32  UserID;
			int32  Flags;
			double Amount;
			double Rest;
			int32  DestSCardID;
			int32  LinkObjType;
			int32  LinkObjID;
			LDATE  FreezingStart;
			LDATE  FreezingEnd;
			uint8  Reserve[4]; // raw
		};
		SCardOpTbl::Rec * p_data = static_cast<SCardOpTbl::Rec *>(pTbl->getDataBuf());
		const SCardOpRec_BeforePPCvtSCardOp12005 * p_old_rec = static_cast<const SCardOpRec_BeforePPCvtSCardOp12005 *>(pRec);
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f) p_data->f = p_old_rec->f
		CPYFLD(SCardID);
		CPYFLD(Dt);
		CPYFLD(Tm);
		CPYFLD(UserID);
		CPYFLD(Flags);
		CPYFLD(Amount);
		CPYFLD(Rest);
		CPYFLD(DestSCardID);
		CPYFLD(LinkObjType);
		CPYFLD(LinkObjID);
		CPYFLD(FreezingStart);
		CPYFLD(FreezingEnd);
#undef CPYFLD
		p_data->CtAmount = 0;
		p_data->CtRest = 0;
		p_data->CtGoodsID = 0; // @v12.0.6
		memzero(p_data->Reserve, sizeof(p_data->Reserve));
		return 1;
	}
};

CONVERT_PROC(Convert12005, PPCvtSCardOp12005);
//
//
//
class PPCvtVATBook12207 : public PPTableConversion {
public:
	struct VATBookRec_Before12207 {
		int32  ID;
		char   Code[24];
		int16  LineType_;
		int16  LineSubType;
		int16  Excluded;
		uint16 Reserve;
		LDATE  Dt;
		int32  LineNo;
		LDATE  InvcDt;
		LDATE  PaymDt;
		LDATE  RcptDt;
		int32  Object;
		int32  Object2;
		int32  Link;
		int32  OpID;
		int32  LocID;
		int32  Flags;
		char   Amount[8];  // money[8.2]
		char   Excise[8];  // money[8.2]
		char   VAT0[8];    // money[8.2]
		char   Export[8];  // money[8.2]
		char   VAT1[8];    // money[8.2]
		char   SVAT1[8];   // money[8.2]
		char   VAT2[8];    // money[8.2]
		char   SVAT2[8];   // money[8.2]
		char   VAT3[8];    // money[8.2]
		char   SVAT3[8];   // money[8.2]
		LDATE  CBillDt;
		char   CBillCode[24];
		char   TaxOpCode[8];
		uint8  Reserve2[72]; // raw
	};
	DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new VATBookTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(VATBookTbl::Rec)) {
				*pNeedConversion = 1;
			}
			else
				*pNeedConversion = 0;
		}
		return p_tbl;
	}
	int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const VATBookRec_Before12207 * p_old_rec = static_cast<const VATBookRec_Before12207 *>(pRec);
		VATBookTbl::Rec * p_data = static_cast<VATBookTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f)    p_data->f = p_old_rec->f
		CPYFLD(ID);
		CPYFLD(LineType_);
		CPYFLD(LineSubType);
		CPYFLD(Excluded);
		CPYFLD(Dt);
		CPYFLD(LineNo);
		CPYFLD(InvcDt);
		CPYFLD(PaymDt);
		CPYFLD(RcptDt);
		CPYFLD(OpID);
		CPYFLD(LocID);
		CPYFLD(Flags);
		CPYFLD(CBillDt);
#undef CPYFLD
		p_data->ArID = p_old_rec->Object;
		p_data->Ar2ID = p_old_rec->Object2;
		p_data->LinkBillID = p_old_rec->Link;
		STRNSCPY(p_data->Code, p_old_rec->Code);
		STRNSCPY(p_data->CBillCode, p_old_rec->CBillCode);
		STRNSCPY(p_data->TaxOpCode, p_old_rec->TaxOpCode);
		p_data->Amount = MONEYTOLDBL(p_old_rec->Amount);
		p_data->Excise = MONEYTOLDBL(p_old_rec->Excise);
		p_data->VAT0 = MONEYTOLDBL(p_old_rec->VAT0);
		p_data->Export = MONEYTOLDBL(p_old_rec->Export);
		p_data->VAT1 = MONEYTOLDBL(p_old_rec->VAT1);
		p_data->SVAT1 = MONEYTOLDBL(p_old_rec->SVAT1);
		p_data->VAT2 = MONEYTOLDBL(p_old_rec->VAT2);
		p_data->SVAT2 = MONEYTOLDBL(p_old_rec->SVAT2);
		p_data->VAT3 = MONEYTOLDBL(p_old_rec->VAT3);
		p_data->SVAT3 = MONEYTOLDBL(p_old_rec->SVAT3);
		p_old_rec->Reserve;
		p_old_rec->Reserve2;
		return 1;
	}
};

CONVERT_PROC(Convert12207, PPCvtVATBook12207);
//
// @v12.4.1 LocTransf
//
class PPCvtLocTransf12401 : public PPTableConversion {
public:
	struct LocTransfRec_Before12401 {
		int32  LocID;
		int32  RByLoc;
		LDATE  Dt;
		LTIME  Tm;
		int32  UserID;
		int32  BillID;
		int16  RByBillLT;
		int16  LTOp;
		int32  Flags;
		int32  GoodsID;
		int32  LotID;
		double Qtty;
		double RestByGoods;
		double RestByLot;
		int32  LinkLocID;
		int32  LinkRByLoc;
		int32  PalletTypeID;
		int16  PalletCount;
		int16  Reserve1;
		uint8  Reserve2[8]; // raw
	};
	DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new LocTransfTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(LocTransfTbl::Rec)) {
				*pNeedConversion = 1;
			}
			else
				*pNeedConversion = 0;
		}
		return p_tbl;
	}
	int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const LocTransfRec_Before12401 * p_old_rec = static_cast<const LocTransfRec_Before12401 *>(pRec);
		LocTransfTbl::Rec * p_data = static_cast<LocTransfTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f)    p_data->f = p_old_rec->f
		CPYFLD(LocID);
		CPYFLD(RByLoc);
		CPYFLD(Dt);
		CPYFLD(Tm);
		CPYFLD(UserID);
		CPYFLD(BillID);
		CPYFLD(RByBillLT);
		CPYFLD(LTOp);
		CPYFLD(Flags);
		CPYFLD(GoodsID);
		CPYFLD(LotID);
		CPYFLD(Qtty);
		CPYFLD(RestByGoods);
		CPYFLD(RestByLot);
		CPYFLD(LinkLocID);
		CPYFLD(LinkRByLoc);
		CPYFLD(PalletTypeID);
		CPYFLD(PalletCount);
#undef CPYFLD
		return 1;
	}
};

CONVERT_PROC(Convert12401, PPCvtLocTransf12401);
//
//
//
class PPCvtPrjTask12407 : public PPTableConversion {
public:
	struct PrjTaskRec_Before12407 {
		int32  ID;
		int32  ProjectID;
		int32  Kind;
		char   Code[24];
		int32  CreatorID;
		int32  GroupID;
		int32  EmployerID;
		int32  ClientID;
		int32  TemplateID;
		LDATE  Dt;
		LTIME  Tm;
		LDATE  StartDt;
		LTIME  StartTm;
		LDATE  EstFinishDt;
		LTIME  EstFinishTm;
		LDATE  FinishDt;
		LTIME  FinishTm;
		int16  Priority;
		int16  Status;
		int16  DrPrd;
		int16  DrKind;
		int32  DrDetail;
		int32  Flags;
		int32  DlvrAddrID;
		int32  LinkTaskID;
		double Amount;
		int32  OpenCount;
		int32  BillArID;
		uint8  Reserve[16]; // raw
	};
	DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PrjTaskTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			//const RECORDSIZE recsz = p_tbl->getRecSize();
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 8);
		}
		return p_tbl;
	}
	int ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		const PrjTaskRec_Before12407 * p_old_rec = static_cast<const PrjTaskRec_Before12407 *>(pRec);
		PrjTaskTbl::Rec * p_data = static_cast<PrjTaskTbl::Rec *>(pTbl->getDataBuf());
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f)    p_data->f = p_old_rec->f
		CPYFLD(ID);
		CPYFLD(ProjectID);
		CPYFLD(Kind);
		CPYFLD(CreatorID);
		CPYFLD(GroupID);
		CPYFLD(EmployerID);
		CPYFLD(ClientID);
		CPYFLD(TemplateID);
		CPYFLD(Dt);
		CPYFLD(Tm);
		CPYFLD(StartDt);
		CPYFLD(StartTm);
		CPYFLD(EstFinishDt);
		CPYFLD(EstFinishTm);
		CPYFLD(FinishDt);
		CPYFLD(FinishTm);
		CPYFLD(Priority);
		CPYFLD(Status);
		CPYFLD(DrPrd);
		CPYFLD(DrKind);
		CPYFLD(DrDetail);
		CPYFLD(Flags);
		CPYFLD(DlvrAddrID);
		CPYFLD(LinkTaskID);
		CPYFLD(Amount);
		CPYFLD(OpenCount);
		CPYFLD(BillArID);
#undef CPYFLD
		STRNSCPY(p_data->Code, p_old_rec->Code);
		return 1;
	}
};

class PPCvtPersonPost12407 : public PPTableConversion { // @construction
public:
	struct PersonPostRec_Before12407 {
		int32  ID;
		char   Code[16];
		int32  StaffID;
		int32  PersonID;
		LDATE  Dt;
		LDATE  Finish;
		int32  ChargeGrpID;
		int32  Flags;
		int16  Closed;
		int16  Reserve1;
		int32  PsnEventID;
		uint8  Reserve2[28]; // raw
	};
	virtual DBTable * CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PersonPostTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			const RECORDSIZE recsz = p_tbl->getRecSize();
			*pNeedConversion = BIN(recsz < sizeof(PersonPostTbl::Rec)); // Новый размер БОЛЬШЕ предыдущего
		}
		return p_tbl;
	}
	virtual int ConvertRec(DBTable * newTbl, void * pRec, int * pNewRecLen)
	{
		const PersonPostRec_Before12407 * p_old_rec = static_cast<const PersonPostRec_Before12407 *>(pRec);
		PersonPostTbl::Rec * p_data = static_cast<PersonPostTbl::Rec *>(newTbl->getDataBuf());
		newTbl->clearDataBuf();
		#define CPYFLD(f)    p_data->f = p_old_rec->f
			CPYFLD(ID);
			CPYFLD(StaffID);
			CPYFLD(PersonID);
			CPYFLD(Dt);
			CPYFLD(Finish);
			CPYFLD(ChargeGrpID);
			CPYFLD(Flags);
			CPYFLD(Closed);
			CPYFLD(PsnEventID);
		#undef CPYFLD
		STRNSCPY(p_data->Code, p_old_rec->Code);
		*pNewRecLen = sizeof(PersonPostTbl::Rec);
		return 1;
	}
};

int Convert12407()
{
	int    ok = 1;
	PPWaitStart();
	{
		PPCvtPrjTask12407 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtPersonPost12407 cvt02;
		THROW(cvt02.Convert());
	}
	PPWaitStop();
	CATCHZOK
	return ok;
}
