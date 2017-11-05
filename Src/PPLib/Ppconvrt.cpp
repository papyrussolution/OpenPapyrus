// PPCONVRT.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
// Конвертация файлов данных при изменениях версий
//
#include <pp.h>
#pragma hdrstop
#include <private\_ppo.h>
// @v9.6.3 #include <idea.h>
//
//
//
int SLAPI PPReEncryptDatabaseChain(PPObjBill * pBObj, Reference * pRef, const char * pSrcEncPw, const char * pDestEncPw, int use_ta);

int SLAPI ConvertCipher(const char * pDbSymb, const char * pMasterPassword, const char * pSrcIniFileName, const char * pDestIniFileName)
{
	int    ok = 1;
	int    is_dict_opened = 0;
	SString temp_buf;
	SString src_pw_buf, dest_pw_buf;
	THROW(!isempty(pDbSymb));
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
					const PPID secur_obj_list[] = { PPOBJ_CONFIG, PPOBJ_USRGRP, PPOBJ_USR };
					PPTransaction tra(1);
					THROW(tra);
					THROW(PPReEncryptDatabaseChain(p_bobj, p_ref, ppb_src.DefPassword, ppb_dest.DefPassword, 0)); // Собсвенная транзакция
					for(uint si = 0; si < SIZEOFARRAY(secur_obj_list); si++) {
						const PPID sec_obj_type = secur_obj_list[si];
						for(PPID sec_id = 0; p_ref->EnumItems(sec_obj_type, &sec_id, &sec_rec) > 0;) {
							if(strlen(sec_rec.Password)) {
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
						PPAlbatrosConfig acfg;
						if(PPAlbatrosCfgMngr::Helper_Get(p_ref, &acfg) > 0) {
							Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, acfg.UhttPassword, /*UHTT_PW_SIZE*/20, temp_buf);
							Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, temp_buf, /*UHTT_PW_SIZE*/20, acfg.UhttPassword);
							THROW(PPAlbatrosCfgMngr::Helper_Put(p_ref, &acfg, 0));
						}
					}
					{
						PPPhoneService phs_rec;
						for(PPID phs_id = 0; p_ref->EnumItems(PPOBJ_PHONESERVICE, &phs_id, &phs_rec) > 0;) {
							if(p_ref->GetPropVlrString(PPOBJ_PHONESERVICE, phs_id, PHNSVCPRP_TAIL, temp_buf) > 0) {
								PPGetExtStrData(PHNSVCEXSTR_PASSWORD, temp_buf, src_pw_buf);
								Reference::Helper_DecodeOtherPw(ppb_src.DefPassword, src_pw_buf, /*PHNSVC_PW_SIZE*/20, dest_pw_buf);
								Reference::Helper_EncodeOtherPw(ppb_dest.DefPassword, dest_pw_buf, /*PHNSVC_PW_SIZE*/20, src_pw_buf);
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
int SLAPI proc_name() \
{                     \
	PPWait(1);        \
	class_name cvt;   \
	int    ok = BIN(cvt.Convert()); \
	PPWait(0);                         \
	return ok;                         \
}

class PPTableConversion {
public:
	int    SLAPI Convert();
protected:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion) = 0;
	virtual int SLAPI DestroyTable(DBTable * pTbl);
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen) = 0;
	virtual int SLAPI Final(DBTable * pTbl)
	{
		return -1;
	}
	PPLogger Logger;
};

int SLAPI PPTableConversion::DestroyTable(DBTable * pTbl)
{
	delete pTbl;
	return 1;
}

int SLAPI PPTableConversion::Convert()
{
	//
	// Так как сеанс конвертации может состоять из нескольких конвертаций отдельных таблиц
	// необходимо иметь общий каталог, в который будут сбрасываться резервные копии всех конвертируемых в
	// данном сеансе файлов.
	//
	static SString _BakPath;
	//
	int    ok = 1, need_conversion = 0;
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
		SPathStruc path_struc;
		THROW(DS.GetSync().LockDB());
		db_locked = 1;
		SString tbl_name = p_tbl->GetTableName();
		const SString file_name = p_tbl->GetName();
		DestroyTable(p_tbl);
		p_tbl = 0;

		path_struc.Split(file_name);
		if(_BakPath.Empty() || !::fileExists(_BakPath)) {
			for(i = 1; i < 100000; i++) {
				SPathStruc ps2;
				ps2.Copy(&path_struc, SPathStruc::fDrv|SPathStruc::fDir);
				ps2.Dir.SetLastSlash().Cat("CVT").CatLongZ((long)i, 5);
				ps2.Merge(temp_buf);
				if(!fileExists(temp_buf)) {
					THROW_SL(::createDir(temp_buf));
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
				// Если существует файл continuous-состояния (^^^) то дадим ему несколько секунд
				// на исчезновение...
				//
				SDelay(5000);
				// ... и снова инициализируем tpl
				THROW_DB(tpl.Init(file_name, 0, 0));
			}
			for(i = 0; i < tpl.GetCount(); i++) {
				THROW_SL(tpl.Get(i, tpe));
				if((tpe.Flags && !(tpe.Flags & tpl.fBu)) && (tpe.Flags & (tpl.fMain|tpl.fExt|tpl.fCon))) {
					THROW_PP_S(SFile::IsOpenedForWrite(tpe.Path) == 0, /*PPERR_CVT_FILEWROPENED*/1, tpe.Path);
					to_copy_pos_list.add((long)i);
				}
			}
			for(i = 0; i < to_copy_pos_list.getCount(); i++) {
				THROW_SL(tpl.Get(to_copy_pos_list.get(i), tpe));
				SPathStruc::ReplacePath(temp_buf = tpe.Path, _BakPath, 1);
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
			p_tbl->SetFlag(XTF_DISABLEOUTOFTAMSG); // @v8.9.10
			THROW_MEM(p_old_tbl = new DBTable(0, old_fname_to_convert, omNormal/*, p*/));
			rec_size = 4096;
			p_old_tbl->getNumRecs(&num_recs);
			THROW_MEM(p_old_buf = new char[rec_size]);
			memzero(p_old_buf, rec_size);
			p_old_tbl->setDataBuf(p_old_buf, rec_size);
			if(p_old_tbl->step(spFirst)) {
				do {
					int    new_rec_len = p_old_tbl->getRetBufLen();
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
			p_tbl->ResetFlag(XTF_DISABLEOUTOFTAMSG); // @v8.9.10
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
			SPathStruc::ReplacePath(temp_buf = tpe.Path, _BakPath, 1);
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
class PPCvtReceipt229 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new ReceiptTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				if(recsz < sizeof(ReceiptTbl::Rec))
					*needConversion = 1;
				else
					*needConversion = 0;
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		ReceiptTbl::Rec * data = (ReceiptTbl::Rec*)tbl->getDataBuf();
		memcpy(data, rec, sizeof(ReceiptTbl::Rec) - sizeof(LDATE));
		data->Expiry = ZERODATE;
		return 1;
	}
};

int SLAPI Convert229()
{
	int ok = 1;
	PPWait(1);
	if(BillObj) {
		ZDELETE(BillObj);
	}
	PPCvtReceipt229 cvt;
	ok = cvt.Convert() ? 1 : PPErrorZ();
	BillObj = new PPObjBill(0);
	PPWait(0);
	return ok;
}
//
//
//
class PPCvtVATBook253 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new VATBookTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			int16  num_keys;
			tbl->getNumKeys(&num_keys);
			*needConversion = BIN(num_keys < 7);
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		VATBookTbl::Rec * data = (VATBookTbl::Rec*)tbl->getDataBuf();
		memcpy(data, rec, sizeof(VATBookTbl::Rec));
		return 1;
	}
};

int SLAPI Convert253()
{
	int ok = 1;
	PPWait(1);
	PPCvtVATBook253 cvt;
	ok = cvt.Convert() ? 1 : PPErrorZ();
	PPWait(0);
	return ok;
}
//
// GoodsConvertion 2.7.0
//
class GoodsConvertion270 {
public:
	SLAPI GoodsConvertion270()
	{
		GrpIdBias = 1000L;
	}
	int SLAPI Convert();
private:
	int SLAPI ConvertGroups();
	int SLAPI ConvertGoods();
	int SLAPI ConvertGoodsRec(GoodsTbl::Rec *, Goods2Tbl::Rec *);
	int SLAPI ConvertGroupRec(GoodsGroupTbl::Rec *, Goods2Tbl::Rec *, BarcodeTbl::Rec *);
	int SLAPI CheckDupName(PPID gknd, char * name, size_t buflen);

	long          GrpIdBias;
	GoodsCore     G2Tbl;
	GoodsTbl      GTbl;
	GoodsGroupTbl GGTbl;
	PPObjGoodsTax GTObj;
	ObjSyncTbl    SyncTbl;
};

int SLAPI GoodsConvertion270::CheckDupName(PPID gknd, char * name, size_t buflen)
{
	char name_buf[64];
	STRNSCPY(name_buf, name);
	for(int i = 0; G2Tbl.SearchByName(gknd, name_buf, 0, 0) > 0;)
		sprintf(name_buf, "%s #%d", name, ++i);
	strnzcpy(name, name_buf, buflen);
	return 1;
}

int SLAPI GoodsConvertion270::ConvertGroups()
{
	int ok = 1;
	RECORDNUMBER num_recs = 0, count = 0;
	GGTbl.getNumRecs(&num_recs);
	if(GGTbl.step(spFirst))
		do {
			Goods2Tbl::Rec g2rec;
			BarcodeTbl::Rec bcrec;
			PPID old_id = GGTbl.data.ID;
			PPID new_id = 0;
			if(ConvertGroupRec(&GGTbl.data, &g2rec, &bcrec) > 0) {
				new_id = g2rec.ID;
				CheckDupName(PPGDSK_GROUP, g2rec.Name, sizeof(g2rec.Name));
				THROW_DB(G2Tbl.insertRecBuf(&g2rec));
				if(bcrec.Code[0]) {
					BarcodeArray bclist;
					THROW_SL(bclist.insert(&bcrec));
					THROW(G2Tbl.UpdateBarcodes(new_id, &bclist, 0));
				}
				if(g2rec.Flags & GF_ALTGROUP) {
					PPID   scnd = 0;
					GoodsFilt gfilt;
					ObjAssocTbl::Rec assc_rec;
					while(PPRef->Assc.EnumByPrmr(PPASS_ALTGOODSGRP, old_id, &scnd, &assc_rec) > 0) {
						assc_rec.PrmrObjID = new_id;
						THROW(PPRef->Assc.Update(assc_rec.ID, &assc_rec, 0));
					}
					if(gfilt.ReadFromProp(PPOBJ_GOODSGROUP, old_id, GGPRP_GOODSFLT) > 0) {
						gfilt.WriteToProp(PPOBJ_GOODSGROUP, new_id, GGPRP_GOODSFLT);
						gfilt.Clear();
						gfilt.WriteToProp(PPOBJ_GOODSGROUP, old_id, GGPRP_GOODSFLT);
					}
				}
			}
			PPWaitPercent(++count, num_recs);
		} while(GGTbl.step(spNext));
	{
		PPScale scale_rec;
		PPObjScale scale_obj;
		for(PPID scale_id = 0; scale_obj.EnumItems(&scale_id, &scale_rec) > 0;)
			if(scale_rec.AltGoodsGrp) {
				scale_rec.AltGoodsGrp += GrpIdBias;
				THROW(scale_obj.UpdateItem(scale_id, &scale_rec));
			}
	}
	{
		// Удаление синхронизации групп товаров
		ObjSyncTbl::Key0 k;
		MEMSZERO(k);
		k.ObjType = PPOBJ_GOODSGROUP;
		while(SyncTbl.search(0, &k, spGt) && SyncTbl.data.ObjType == PPOBJ_GOODSGROUP) {
			THROW_DB(SyncTbl.deleteRec());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI GoodsConvertion270::ConvertGoods()
{
	int ok = 1;
	RECORDNUMBER num_recs = 0, count = 0;
	GTbl.getNumRecs(&num_recs);
	if(GTbl.step(spFirst))
		do {
			Goods2Tbl::Rec g2rec;
			if(ConvertGoodsRec(&GTbl.data, &g2rec) > 0) {
				CheckDupName(PPGDSK_GOODS, g2rec.Name, sizeof(g2rec.Name));
				THROW_DB(G2Tbl.insertRecBuf(&g2rec));
			}
			PPWaitPercent(++count, num_recs);
		} while(GTbl.step(spNext));
	THROW_DB(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int SLAPI GoodsConvertion270::ConvertGroupRec(GoodsGroupTbl::Rec * grec, Goods2Tbl::Rec * g2rec, BarcodeTbl::Rec * bcrec)
{
	int    ok = 1;
	char   buf[64];
	memzero(g2rec, sizeof(Goods2Tbl::Rec));
	g2rec->ID          = grec->ID + GrpIdBias;
	g2rec->Kind        = PPGDSK_GROUP;
	g2rec->ParentID    = grec->PrevLevelID ? (grec->PrevLevelID + GrpIdBias) : 0;
	g2rec->GoodsTypeID = grec->GoodsType;
	g2rec->UnitID      = grec->Unit;
	g2rec->PhUnitID    = grec->PhUnit;
	if(grec->AltGrp)
		g2rec->Flags |= GF_ALTGROUP;
	if(grec->HasBranch)
		g2rec->Flags |= GF_FOLDER;
	STRNSCPY(buf, grec->Name);
	buf[sizeof(grec->Name)-1] = 0;
	STRNSCPY(g2rec->Name, strip(buf));
	memzero(bcrec, sizeof(BarcodeTbl::Rec));
	bcrec->GoodsID = g2rec->ID;
	if(*strip(grec->Code)) {
		bcrec->Qtty    = 1;
		bcrec->Code[0] = '@';
		strnzcpy(bcrec->Code + 1, grec->Code, sizeof(bcrec->Code) - 1);
	}
	if(grec->VATax != 0 || grec->Excise != 0) {
   	    PPID gtax_id = 0;
		GTObj.GetBySheme(&gtax_id, grec->VATax, 0, grec->Excise, 0);
	   	g2rec->TaxGrpID = gtax_id;
	}
	return ok;
}

int SLAPI GoodsConvertion270::ConvertGoodsRec(GoodsTbl::Rec * grec, Goods2Tbl::Rec * g2rec)
{
	int    ok = 1;
	char   buf[64];
	memzero(g2rec, sizeof(Goods2Tbl::Rec));
	g2rec->ID          = grec->ID;
	g2rec->Kind        = PPGDSK_GOODS;
	g2rec->ParentID    = grec->Grp ? (grec->Grp + GrpIdBias) : 0;
	g2rec->GoodsTypeID = grec->GoodsType;
	g2rec->UnitID      = grec->Unit;
	g2rec->PhUnitID    = grec->PhUnit;
	g2rec->PhUPerU     = grec->PhUPerU;
	g2rec->ManufID     = grec->Producer;
	g2rec->StrucID     = grec->StrucID;
	g2rec->Flags       = (grec->Flags & (GF_GENERIC));
	STRNSCPY(buf, grec->Name);
	buf[sizeof(grec->Name)-1] = 0;
	STRNSCPY(g2rec->Name, strip(buf));
	STRNSCPY(buf, grec->Abbr);
	buf[sizeof(grec->Abbr)-1] = 0;
	STRNSCPY(g2rec->Abbr, strip(buf));
	if((grec->VATax != 0 || (grec->Flags & GF_ZVAT)) ||
		(grec->Excise != 0 || (grec->Flags & GF_ZEXCISE))) {
		PPID gtax_id = 0;
		int  abs_excise = 0;
		double excise = 0, sales_tax = 0;
		if(grec->Flags & GF_ABSEXCISE) {
			abs_excise = 1;
			excise = grec->Excise;
		}
		else
			sales_tax = grec->Excise;
		GTObj.GetBySheme(&gtax_id, grec->VATax, excise, sales_tax, (abs_excise ? GTAXF_ABSEXCISE : 0));
		g2rec->TaxGrpID = gtax_id;
	}
	return ok;
}

int SLAPI GoodsConvertion270::Convert()
{
	int    ok = 1, ta = 0;
	PPID   k = MAXLONG;
	PPWait(1);
	THROW(PPStartTransaction(&ta, 1));
	if(GTbl.search(0, &k, spLt))
		GrpIdBias = (k + 1000L) - (k % 1000L);
	else
		GrpIdBias = 1000L;
	THROW_DB(BTROKORNFOUND);
	THROW(ConvertGroups());
	THROW(ConvertGoods());
	THROW(PPCommitWork(&ta));
	PPWait(0);
	CATCH
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI Convert270()
{
	GoodsConvertion270 gc270;
	return gc270.Convert();
}
//
//
//
class PPCvtAccount290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new AccountTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz > sizeof(PPAccount));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		PPAccount * data = (PPAccount*)tbl->getDataBuf();
		OldAccountTbl::Rec * old_rec = (OldAccountTbl::Rec *)rec;
		tbl->clearDataBuf();
		data->ID    = old_rec->ID;
		data->Ac    = old_rec->Ac;
		data->Sb    = old_rec->Sb;
		data->CurID = 0;
		data->AccSheetID = old_rec->AccSheet;
		data->OpenDate   = old_rec->OpenDate;
		// @v3.1.11 data->User       = old_rec->User;
		data->Kind       = old_rec->Kind;
		data->Flags      = old_rec->Flags;
		STRNSCPY(data->Name, old_rec->Name);
		data->Limit = MONEYTOLDBL(old_rec->Limit);
		data->Overdraft = MONEYTOLDBL(old_rec->Overdraft);
		data->AccessLevel = old_rec->AccessLevel;
		return 1;
	}
};
//
//
//
class PPCvtAcctRel290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new AcctRelTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz > sizeof(AcctRelTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		AcctRelTbl::Rec * data = (AcctRelTbl::Rec*)tbl->getDataBuf();
		OldAcctRelTbl::Rec * old_rec = (OldAcctRelTbl::Rec *)rec;
		tbl->clearDataBuf();
		data->ID          = old_rec->ID;
		data->AccID       = old_rec->AccID;
		data->ArticleID   = old_rec->Article;
		data->CurID       = 0;
		data->Ac          = old_rec->Ac;
		data->Sb          = old_rec->Sb;
		data->Ar          = old_rec->Ar;
		data->Kind        = old_rec->Kind;
		data->Closed      = old_rec->Closed;
		data->Flags       = old_rec->Flags;
		data->BankAccID   = old_rec->BankAcc;
		data->AccessLevel = old_rec->AccessLevel;
		return 1;
	}
};

class PPCvtBalance290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new BalanceTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz > sizeof(BalanceTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		BalanceTbl::Rec * data = (BalanceTbl::Rec*)tbl->getDataBuf();
		OldBalanceTbl::Rec * old_rec = (OldBalanceTbl::Rec *)rec;
		tbl->clearDataBuf();
		data->Dt    = old_rec->Dt;
		data->AccID = old_rec->Bal;
		MONEYTOMONEY(old_rec->DbtRest, data->DbtRest);
		MONEYTOMONEY(old_rec->CrdRest, data->CrdRest);
		return 1;
	}
};

class PPCvtBill290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz < (sizeof(BillTbl::Rec) - sizeof( ((BillTbl::Rec*)0)->Memo)));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		BillTbl::Rec    * data = (BillTbl::Rec*)tbl->getDataBuf();
		OldBillTbl::Rec * old_rec = (OldBillTbl::Rec *)rec;
		tbl->clearDataBuf();
		data->ID = old_rec->ID;
		memcpy(data->Code, old_rec->Code, sizeof(data->Code));
		data->Dt       = old_rec->Dt;
		data->BillNo   = old_rec->BillNo;
		data->OprKind  = old_rec->OprKind;
		data->UserID   = old_rec->User;
		data->Location = old_rec->Location;
		data->Object   = old_rec->Object;
		data->Object2  = 0;
		data->CurID    = 0;
		data->CRate    = 1;
		memcpy(data->Amount, old_rec->Amount, sizeof(data->Amount));
		data->LinkBillID  = old_rec->LinkBill;
		data->Flags       = old_rec->Flags;
		data->AccessLevel = old_rec->AccessLevel;
		STRNSCPY(data->Memo, strip(old_rec->Memo));
		*pNewRecLen = -1;
		return 1;
	}
};

class PPCvtBillAmount290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new BillAmountTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz < sizeof(BillAmountTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		BillAmountTbl::Rec * data = (BillAmountTbl::Rec*)tbl->getDataBuf();
		OldBillAmountTbl::Rec * old_rec = (OldBillAmountTbl::Rec *)rec;
		tbl->clearDataBuf();
		data->BillID    = old_rec->BillID;
		data->AmtTypeID = old_rec->AmtType;
		data->CurID     = 0;
		//memcpy(data->Amount, old_rec->Amount, sizeof(data->Amount));
		data->Amount = MONEYTOLDBL(old_rec->Amount);
		return 1;
	}
};

class PPCvtArticle290 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new ArticleTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz < sizeof(ArticleTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
	{
		ArticleTbl::Rec * p_data = (ArticleTbl::Rec*)tbl->getDataBuf();
		memcpy(p_data, rec, sizeof(ArticleTbl::Rec) - 14);
		p_data->Closed = 0;
		p_data->Flags = 0;
		memzero(p_data->Reserve2, sizeof(p_data->Reserve2));
		return 1;
	}
};
//
//
//
class PPCvtReceipt300 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new ReceiptTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz < sizeof(ReceiptTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		ReceiptTbl::Rec * data = (ReceiptTbl::Rec*)tbl->getDataBuf();
		ReceiptTbl::Rec * old_rec = (ReceiptTbl::Rec *)rec;
		tbl->clearDataBuf();
		memcpy(data, old_rec, sizeof(ReceiptTbl::Rec) - 8);
		return 1;
	}
};
//
//
//
class PPCvtTransfer300 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion)
	{
		DBTable * tbl = new TransferTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(needConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*needConversion = BIN(recsz < sizeof(TransferTbl::Rec));
			else {
				*needConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int *)
	{
		TransferTbl::Rec * data = (TransferTbl::Rec*)tbl->getDataBuf();
		TransferTbl::Rec * old_rec = (TransferTbl::Rec *)rec;
		tbl->clearDataBuf();
		memcpy(data, old_rec, sizeof(TransferTbl::Rec) - 12);
		return 1;
	}
};
//
//
//
int SLAPI PPObjFormula::GetBefore290(PPID id, char * pName, char * pBuf, size_t buflen)
{
	struct _Formula {
		PPID Tag;      // Const = PPOBJ_FORMULA
		PPID ID;       // -> Ref (ObjType = PPOBJ_FORMULA)
		PPID PropID;   // Const = 1
		long TailSize;
		// char [PROPRECFIXSIZE - 16 + TailSize]
	};
	int ok = 1;
	union {
		_Formula * form;
		char     * bform;
	};
	size_t sz = PROPRECFIXSIZE;
	ReferenceTbl::Rec ref_rec;
	pBuf[0] = 0;
	if(id == 0) {
		THROW(ref->SearchName(Obj, &id, pName, &ref_rec) > 0);
	}
	else {
		THROW(Search(id, &ref_rec) > 0);
	}
	strnzcpy(pName, strip(ref_rec.ObjName), PP_SYMBLEN);
	THROW_MEM(bform = (char*)SAlloc::C(1, sz));
	THROW(ref->GetProp(Obj, id, 1, form, sz) > 0);
	if((PROPRECFIXSIZE + form->TailSize) > sz) {
		sz = PROPRECFIXSIZE + (size_t)form->TailSize;
		THROW_MEM(bform = (char*)SAlloc::R(bform, sz));
		THROW(ref->GetProp(PPOBJ_FORMULA, id, 1, form, sz));
	}
	strnzcpy(pBuf, (char*)(form + 1), buflen);
	strip(pBuf);
	CATCHZOK
	SAlloc::F(bform);
	return ok;
}

static int SLAPI ConvertFormula290()
{
	int ok = 1;
	PPObjFormula frmobj;
	for(PPID id = 0; frmobj.EnumItems(&id) > 0;) {
		char name[48], form[256];
		THROW(frmobj.GetBefore290(id, name, form, sizeof(form)) > 0);
		THROW(frmobj.Put(&id, name, form, 1));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjDBDiv::GetBefore290(PPID id, DBDivPack * pack)
{
	const size_t LLC_LIMIT ((PROPRECFIXSIZE - 14) / sizeof(PPID)); // 17
	const size_t ALS_LIMIT ((PROPRECFIXSIZE - 14) / sizeof(char)); // 70

	struct LocListBuf { // 84
		PPID   Tag;          //
		PPID   ID;           //
		PPID   Prop;         //
		int16  Count;        // Количество элементов в кластере
		PPID   Items[LLC_LIMIT];
	};
	struct AccListBuf { // 84
		PPID   Tag;          //
		PPID   ID;           //
		PPID   Prop;         //
		int16  Size;         // Длина строки в байтах
		char   Str[ALS_LIMIT];
	};
	int ok = 1, r;
	uint i, sz;
	union {
		LocListBuf * buf;
		AccListBuf * alb;
	};
	buf = 0;
	THROW(PPRef->GetItem(PPOBJ_DBDIV, id, &pack->Rec) > 0);
	pack->LocList.freeAll();
	sz = PROPRECFIXSIZE;
	THROW_MEM(buf = (LocListBuf*)SAlloc::M(sz));
	if((r = PPRef->GetProp(PPOBJ_DBDIV, id, DBDPRP_LOCLIST, buf, sz)) > 0) {
		if(buf->Count > LLC_LIMIT) {
			sz += (buf->Count - LLC_LIMIT) * sizeof(PPID);
			THROW_MEM(buf = (LocListBuf*)SAlloc::R(buf, sz));
			THROW(PPRef->GetProp(PPOBJ_DBDIV, id, DBDPRP_LOCLIST, buf, sz) > 0);
		}
		for(i = 0; i < (uint)buf->Count; i++) {
			THROW_SL(pack->LocList.insert(&buf->Items[i]));
		}
	}
	ZFREE(buf);
	THROW(r);
	memzero(pack->AccList, sizeof(pack->AccList));
	sz = PROPRECFIXSIZE;
	THROW_MEM(alb = (AccListBuf*)SAlloc::M(sz));
	if((r = PPRef->GetProp(PPOBJ_DBDIV, id, DBDPRP_ACCLIST, alb, sz)) > 0) {
		if(alb->Size > ALS_LIMIT) {
			sz += (alb->Size - ALS_LIMIT);
			THROW_MEM(alb = (AccListBuf*)SAlloc::R(alb, sz));
			THROW(PPRef->GetProp(PPOBJ_DBDIV, id, DBDPRP_ACCLIST, alb, sz) > 0);
		}
		STRNSCPY(pack->AccList, alb->Str);
	}
	ZFREE(alb);
	THROW(r);
	CATCH
		ok = 0;
		pack->LocList.freeAll();
		pack->AccList[0] = 0;
	ENDCATCH
	SAlloc::F(buf);
	return ok;
}

static int SLAPI ConvertDBDiv290()
{
	int ok = 1;
	PPObjDBDiv dbdivobj;
	for(PPID id = 0; dbdivobj.EnumItems(&id) > 0;) {
		DBDivPack pack;
		THROW(dbdivobj.GetBefore290(id, &pack) > 0);
		THROW(dbdivobj.Put(&id, &pack, 1));
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI Convert300()
{
	int ok = 1;
	PPWait(1);
	if(BillObj) {
		ZDELETE(BillObj);
	}
#if 1
	{
		PPCvtAccount290 cvt_acct;
		THROW(cvt_acct.Convert());
	}
	{
		PPCvtArticle290 cvt_art;
		THROW(cvt_art.Convert());
	}
	{
		PPCvtAcctRel290 cvt_arel;
		THROW(cvt_arel.Convert());
	}
	{
		PPCvtBalance290 cvt_bal;
		THROW(cvt_bal.Convert());
	}
	{
		PPCvtBill290       cvt_bill;
		PPCvtBillAmount290 cvt_ba;
		THROW(cvt_bill.Convert());
		THROW(cvt_ba.Convert());
	}
#endif
	{
		PPCvtReceipt300 cvt_rcpt;
		THROW(cvt_rcpt.Convert());
	}
	{
		PPCvtTransfer300 cvt_trfr;
		THROW(cvt_trfr.Convert());
	}
#if 1
	THROW(ConvertDBDiv290());
	THROW(ConvertFormula290());
#endif
	PPWait(0);
	CATCHZOKPPERR
	BillObj = new PPObjBill(0);
	return ok;
}
//
//
//
class CounterGrpList {
public:
	SLAPI  CounterGrpList() { count = 0; ptr = 0; }
	SLAPI ~CounterGrpList() { SAlloc::F(ptr); }
	int    SLAPI SearchID(long id, int * pos);
	int    SLAPI Load();
private:
	int    SLAPI GetNumGroups();
	int    SLAPI GetGroup(int);

	int    count;
	long * ptr;
};
//
// Структура списка групп операций по счетчику:
//
//    Variable Part:
//        long Count;
//        long Items[Count];
//
// Группы неявно идентифицируются порядковым номером в списке (0..)
//
// sizeof(CntGrpCluster) == PROPRECFIXSIZE
//
#define ITEMS_PER_CLUSTER (PROPRECFIXSIZE / sizeof(long) - 4)

struct CntGrpCluster {
	PPID   Obj;
	PPID   ObjID;
	PPID   Prop;
	long   Count;
	long   Items[ITEMS_PER_CLUSTER];
};

int SLAPI CounterGrpList::GetNumGroups()
{
	int c = 0;
	if(ptr)
		for(int i = 0; i < count; i += (int)(ptr[i] + 1), c++);
	return c;
}

int SLAPI CounterGrpList::GetGroup(int g)
{
	int i, c = 0;
	if(ptr && count) {
		for(i = 0; i < count && c < g; i += (int)(ptr[i] + 1), c++);
		return (c == g) ? i : -1;
	}
	return -1;
}

int SLAPI CounterGrpList::SearchID(long id, int * pos)
{
	for(int i = 0, ng = GetNumGroups(); i < ng; i++)
		for(int j = 0, p = GetGroup(i); j < ptr[p]; j++)
			if(ptr[p+j+1] == id) {
				ASSIGN_PTR(pos, (p+j+1));
				return i;
			}
	return -1;
}

int SLAPI CounterGrpList::Load()
{
	PPID   p = 0;
	CntGrpCluster clu;
	count = 0;
	while(PPRef->EnumProps(PPOBJ_COUNTGRP, PPCNT_ONE, &p, &clu, sizeof(clu)) > 0 && p <= 1)
		if(p == 1 && clu.Count)
			if((ptr = (long*)SAlloc::R(ptr, sizeof(long) * (count + (uint)clu.Count))) != 0) {
				memcpy(ptr + count, clu.Items, sizeof(long) * (uint)clu.Count);
				count += (int)clu.Count;
			}
			else
				return PPSetErrorNoMem();
	return 1;
}

struct PPOprKind_Before301 {
	long   Tag;              // Const PPOBJ_OPRKIND
	long   ID;
	char   Name[42];
	int16  Rank;             // @v2.9.10 Ранг операции (порядок сортировки)
	long   Link;             // Связанный вид операции
	char   CodeTemplate[16]; // Шаблон генерации кода документа
	long   Counter;          // Номер последней операции по кодировке
	long   Flags;            // OPKF_XXX
	long   OprType;          // Тип операции PPOprType::ID
	long   AccSheet;         // Связанная таблица аналитических статей
};

int SLAPI Convert301()
{
	int    ok = 1, ta = 0;
	PPObjOprKind opobj;
	PPOprKind_Before301 op_rec_b301;
	PPOprKind op_rec;
	PPID   id;
	int    need_conversion = 0;
	for(id = 0; !need_conversion && opobj.EnumItems(&id, &op_rec) > 0;) {
		// @v3.3.14 for(size_t i = 0; !need_conversion && i < sizeof(op_rec.Reserve); i++)
		// @v3.9.0 if(op_rec.Reserve/* @v3.3.14 [i]*/ != 0)
		if(op_rec.SubType != 0 /* @v3.9.0 */ && op_rec.OpTypeID != PPOPT_ACCTURN)
			need_conversion = 1;
	}
	if(need_conversion) {
		PPObjOpCounter opc_obj;
		PPID   named_counter_array[64];
		CounterGrpList grp_list;
		memzero(named_counter_array, sizeof(named_counter_array));
		THROW(grp_list.Load());
		THROW(PPStartTransaction(&ta, 1));
		for(id = 0; opobj.EnumItems(&id, &op_rec_b301) > 0;) {
			PPID   cntr_id = 0;
			char   memo_templ[128];
			PropertyTbl::Rec prop_rec;

			MEMSZERO(op_rec);
			STRNSCPY(op_rec.Name, op_rec_b301.Name);
			op_rec.Tag        = op_rec_b301.Tag;
			op_rec.ID         = op_rec_b301.ID;
			op_rec.Rank       = op_rec_b301.Rank;
			op_rec.LinkOpID   = op_rec_b301.Link;
			op_rec.OpTypeID   = op_rec_b301.OprType;
			op_rec.AccSheetID = op_rec_b301.AccSheet;

			op_rec.Flags = op_rec_b301.Flags & (
				OPKF_NEEDPAYMENT | OPKF_GRECEIPT | OPKF_GEXPEND | OPKF_BUYING |
				OPKF_SELLING | OPKF_PROFITABLE | OPKF_ONORDER | OPKF_CALCSTAXES |
				OPKF_AUTOWL | OPKF_USEPAYER | OPKF_EXTACCTURN | OPKF_EXTAMTLIST |
				OPKF_RENT | OPKF_NEEDACK | OPKF_RECKON | OPKF_BANKING |
				OPKF_PASSIVE | OPKF_CURTRANSIT);
			op_rec.PrnFlags = op_rec_b301.Flags & (
				OPKF_PRT_BUYING | OPKF_PRT_SELLING | OPKF_PRT_QCERT |
				OPKF_PRT_NBILLN | OPKF_PRT_VATAX | OPKF_PRT_INVOICE |
				OPKF_PRT_QCG | OPKF_PRT_SHRTORG | OPKF_PRT_CASHORD |
				OPKF_PRT_SELPRICE | OPKF_PRT_NDISCNT | OPKF_PRT_LADING);
			int cntr_grp_id = grp_list.SearchID(id, 0);
			PPOpCounter opc_rec;
			PPOpCounterPacket opc_pack;
			if(cntr_grp_id >= 0) {
				if(named_counter_array[cntr_grp_id]) {
					cntr_id = named_counter_array[cntr_grp_id];
				}
				else {
					MEMSZERO(opc_rec);
					STRNSCPY(opc_rec.Name, op_rec_b301.Name);
					STRNSCPY(opc_rec.CodeTemplate, op_rec_b301.CodeTemplate);
					opc_rec.Counter = op_rec_b301.Counter;
					opc_pack.Init(&opc_rec, 0);
					THROW(opc_obj.SetItem(&cntr_id, &opc_pack, 0));
					named_counter_array[cntr_grp_id] = cntr_id;
				}
			}
			else {
				MEMSZERO(opc_rec);
				STRNSCPY(opc_rec.CodeTemplate, op_rec_b301.CodeTemplate);
				opc_rec.Counter = op_rec_b301.Counter;
				opc_rec.OwnerObjID = op_rec_b301.ID;
				opc_pack.Init(&opc_rec, 0);
				THROW(opc_obj.SetItem(&cntr_id, &opc_pack, 0));
			}
			op_rec.OpCounterID = cntr_id;
			THROW(PPRef->UpdateItem(PPOBJ_OPRKIND, id, &op_rec));
			if(PPRef->GetProp(PPOBJ_OPRKIND, id, OPKPRP_BILLMEMO, &prop_rec, sizeof(prop_rec)) > 0) {
				STRNSCPY(memo_templ, prop_rec.Text);
				THROW(PPRef->PutPropVlrString(PPOBJ_OPRKIND, id, OPKPRP_BILLMEMO, memo_templ));
			}
		}
		//
		// Removing old Counter Groups
		//
	   	THROW(PPRef->RemoveProp(PPOBJ_COUNTGRP, PPCNT_ONE, 0, 0));
	   	THROW(PPRef->RemoveProp(PPOBJ_COUNTGRP, PPCNT_ONE, 1, 0));

		THROW(PPCommitWork(&ta));
	}
	CATCH
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
//
//
//
class PPCvtSJ329 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SysJournalTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = (recsz == sizeof(SysJournalTbl::Rec)) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
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
			int16 e = *(int16*)p_old_sj_rec->Info;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = (recsz == sizeof(RegisterTbl::Rec)) ? 0 : 1;
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
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
				pTbl->copyBufFrom(&tmp_buf);
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CCheckTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(CCheckTbl::Rec));
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct OldCCheckRec {
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
		} * p_old_data = (OldCCheckRec*)pRec;

		CCheckTbl::Rec * p_data = (CCheckTbl::Rec*)pTbl->getDataBuf();
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new AccountTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(PPAccount));
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct OldAccRec {
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
		} * p_old_data = (OldAccRec*)pRec;

		PPAccount * p_data = (PPAccount*)pTbl->getDataBuf();
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
		AccountCore::GenerateCode(p_data);
		return 1;
	}
};

CONVERT_PROC(Convert400, PPCvtAccount400);
//
//
//
class PPCvtObjTag31102 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		ObjTagTbl::Rec * p_data = (ObjTagTbl::Rec*)pTbl->getDataBuf();
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		CCheckTbl::Rec * p_data = (CCheckTbl::Rec*)pTbl->getDataBuf();
		memcpy(p_data, pRec, sizeof(CCheckTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert31110, PPCvtCCheck31110);

#endif // } 0 @v9.0.4 (снята поддержка Convert31110, Convert31102, Convert400, Convert372)
//
//
//
class PPCvtBill4108 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				if(recsz < (sizeof(BillTbl::Rec) - sizeof( ((BillTbl::Rec*)0)->Memo)))
					*pNeedConversion = 1;
				else
					*pNeedConversion = 0;
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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

#if 0 // { Перенесено в PPCvtCCheckLine5207

class PPCvtCCheckLine4108 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtCCheckLine4108::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new CCheckLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz)) {
			//
			// В новом формате запись короче (28 байтов вместо 40)
			//
			*pNeedConversion = BIN(recsz > sizeof(CCheckLineTbl::Rec));
		}
	}
	return p_tbl;
}

int SLAPI PPCvtCCheckLine4108::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldCCheckLineRec {
		long   CheckID;      // -> CCheck.ID
		long   RByCheck;     // Счетчик строк по чеку
		long   DivID;        // Отдел магазина
		long   GoodsID;      // -> Goods.ID
		double Quantity;     // Количество товара
		char   Price[8];     // Цена
		char   Discount[8];  // Скидка
	} * p_old_data = (OldCCheckLineRec*)pRec;
	CCheckLineTbl::Rec * p_data = (CCheckLineTbl::Rec*)pTbl->getDataBuf();
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
class PPCvtSCardOp4108 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SCardOpTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				//
				// В новом формате запись короче (48 байтов вместо 56)
				//
				*pNeedConversion = BIN(recsz > sizeof(SCardOpTbl::Rec));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		SCardOpTbl::Rec * p_data = (SCardOpTbl::Rec*)pTbl->getDataBuf();
		memcpy(p_data, pRec, sizeof(SCardOpTbl::Rec));
		return 1;
	}
};

int SLAPI Convert4108()
{
	int    ok = 1;
	PPWait(1);
	if(ok) {
		PPCvtBill4108 cvt;
		ok = cvt.Convert() ? 1 : PPErrorZ();
	}
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
	PPWait(0);
	return ok;
}
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtAdvBillItem6202 {
class PPCvtAdvBillItem4208 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
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
class PPCvtVatBook4402 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtVatBook4402::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new VATBookTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		//
		// Сюда же перенесена проверка на конвертацию VATBook 3.11.2, которая упразднена
		//
		int    num_seg = 0;
		DBIdxSpec * p_is = p_tbl->getIndexSpec(1, &num_seg);
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(VATBookTbl::Rec) || (p_is && num_seg == 1));
		SAlloc::F(p_is);
	}
	return p_tbl;
}

int SLAPI PPCvtVatBook4402::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldRec {
		long    ID;
		char    Code[10];
		long    LineType;
		LDATE   Dt;
		long    LineNo;
		LDATE   InvcDt;
		LDATE   PaymDt;
		LDATE   RcptDt;
		long    Object;
		long    Link;
		long    Flags;
		char    Amount[8];
		char    Excise[8];
		char    VAT0[8];
		char    Export[8];
		char    VAT1[8];
		char    SVAT1[8];
		char    VAT2[8];
		char    SVAT2[8];
		short   Excluded;
		long    OpID;
		char    VAT3[8];
	} * p_old_rec = (OldRec *)pRec;

	VATBookTbl::Rec * p_data = (VATBookTbl::Rec*)pTbl->getDataBuf();
	memcpy(p_data, pRec, sizeof(VATBookTbl::Rec));

	p_data->ID = p_old_rec->ID;
	STRNSCPY(p_data->Code, p_old_rec->Code);
	p_data->LineType_ = (int16)p_old_rec->LineType;
	p_data->LineSubType = 0;
	p_data->Dt = p_old_rec->Dt;
	p_data->LineNo = p_old_rec->LineNo;
	p_data->InvcDt = p_old_rec->InvcDt;
	p_data->PaymDt = p_old_rec->PaymDt;
	p_data->RcptDt = p_old_rec->RcptDt;
	p_data->Object = p_old_rec->Object;
	p_data->Link   = p_old_rec->Link;
	p_data->Flags  = p_old_rec->Flags;
	p_data->OpID   = p_old_rec->OpID;
	p_data->Excluded  = p_old_rec->Excluded;
	MONEYTOMONEY(p_old_rec->Amount, p_data->Amount);
	MONEYTOMONEY(p_old_rec->Excise, p_data->Excise);
	MONEYTOMONEY(p_old_rec->Export, p_data->Export);
	MONEYTOMONEY(p_old_rec->VAT0, p_data->VAT0);
	MONEYTOMONEY(p_old_rec->VAT1, p_data->VAT1);
	MONEYTOMONEY(p_old_rec->SVAT1, p_data->SVAT1);
	MONEYTOMONEY(p_old_rec->VAT2, p_data->VAT2);
	MONEYTOMONEY(p_old_rec->SVAT2, p_data->SVAT2);
	MONEYTOMONEY(p_old_rec->VAT3, p_data->VAT3);
	return 1;
}

CONVERT_PROC(Convert4402, PPCvtVatBook4402);
//
//
//
#if 0 // Перенесено в Convert5200 {

class PPCvtGoods4405 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtGoods4405::CreateTableInstance(int * pNeedConversion)
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

int SLAPI PPCvtGoods4405::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldRec {
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
	} * p_old_rec = (OldRec *)pRec;

	Goods2Tbl::Rec * p_data = (Goods2Tbl::Rec*)pTbl->getDataBuf();
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

class PPCvtQCert4405 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		QualityCertTbl::Rec * p_data = (QualityCertTbl::Rec*)pTbl->getDataBuf();
		memcpy(p_data, pRec, sizeof(QualityCertTbl::Rec));
		return 1;
	}
};
//
//
//
class PPCvtPriceLine4405 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
private:
	int    _pre380format;
};

DBTable * SLAPI PPCvtPriceLine4405::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new PriceLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			if(recsz < (sizeof(PriceLineTbl::Rec)-sizeof(((PriceLineTbl::Rec*)0)->Memo))) {
				*pNeedConversion = 1;
				_pre380format = BIN(recsz < 152);
			}
			else
				*pNeedConversion = 0;
	}
	return p_tbl;
}

int SLAPI PPCvtPriceLine4405::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldPriceLineRec1 {
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
	} * p_old_data1 = (OldPriceLineRec1*)pRec;
	struct OldPriceLineRec2 {
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
	} * p_old_data2 = (OldPriceLineRec2*)pRec;

	PriceLineTbl::Rec * p_data = (PriceLineTbl::Rec*)pTbl->getDataBuf();
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		pTbl->clearDataBuf();
		RegisterTbl::Rec tmp_buf;
		MEMSZERO(tmp_buf);
		memcpy(&tmp_buf, pRec, sizeof(RegisterTbl::Rec));
		tmp_buf.UniqCntr = 0;
		tmp_buf.Flags = 0;

		char   serial_buf[48];
		int    c = 0;
		STRNSCPY(serial_buf, tmp_buf.Serial);
		while(1) {
			RegisterTbl::Key3 k3;
			MEMSZERO(k3);
			k3.RegTypeID = tmp_buf.RegTypeID;
			STRNSCPY(k3.Serial, tmp_buf.Serial);
			STRNSCPY(k3.Num, tmp_buf.Num);
			k3.UniqCntr = 0;
			if(pTbl->search(3, &k3, spEq)) {
				sprintf(tmp_buf.Serial, "%s #%d", serial_buf, ++c);
				strip(tmp_buf.Serial);
			}
			else {
				pTbl->copyBufFrom(&tmp_buf);
				break;
			}
		}
		return 1;
	}
};

int SLAPI Convert4405()
{
	int    ok = 1;
	PPWait(1);
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
	PPWait(0);
	return ok;
}
//
//
//
class PPCvtHistBill4515 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtHistBill4515::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new HistBillTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(HistBillTbl::Rec));
	}
	return p_tbl;
}

int SLAPI PPCvtHistBill4515::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldRec {
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
	} * p_old_data = (OldRec*)pRec;
	HistBillTbl::Rec * p_data = (HistBillTbl::Rec*)pTbl->getDataBuf();
	memzero(p_data, sizeof(HistBillTbl::Rec));
	p_data->ID         = p_old_data->ID;
	p_data->BillID     = p_old_data->BillID;
	p_data->InnerID    = p_old_data->InnerID;
	p_data->Ver        = p_old_data->Ver;
	STRNSCPY(p_data->Code, p_old_data->Code);
	p_data->Dt         = p_old_data->Dt;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ReceiptTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(ReceiptTbl::Rec));
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
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
		OldRec * p_old_rec = (OldRec *)rec;
		ReceiptTbl::Rec * data = (ReceiptTbl::Rec*)tbl->getDataBuf();

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

int SLAPI Convert4707()
{
	int    ok = 1, is_billobj_existed = 0;
	PPWait(1);
	if(BillObj) {
		ZDELETE(BillObj);
		is_billobj_existed = 1;
	}
	PPCvtReceipt477 cvt;
	PROFILE(ok = BIN(cvt.Convert()));
	if(is_billobj_existed)
		BillObj = new PPObjBill(0);
	PPWait(0);
	return ok;
}
//
//
//
// AHTOXA {
class PPPriceList4802 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		PriceListTbl::Rec * p_data = (PriceListTbl::Rec*)pTbl->getDataBuf();
		memcpy(p_data, pRec, sizeof(PriceListTbl::Rec));
		p_data->UserID  = -1;
		return 1;
	}
};

CONVERT_PROC(Convert4802, PPPriceList4802);

// } AHTOXA

class PPCvtDlsObj4805 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new DlsObjTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(DlsObjTbl::Rec));
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * /*pNewRecLen*/)
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
//
// Conversion 4.9.11
// Bill, PayPlan, Transfer
//
class PPCvtBill4911 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new BillTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < (sizeof(BillTbl::Rec) - sizeof( ((BillTbl::Rec*)0)->Memo)));
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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

class PPCvtPayPlan4911 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new PayPlanTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(PayPlanTbl::Rec));
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion);
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtTransfer4911::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new TransferTbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(TransferTbl::Rec));
		else {
			*pNeedConversion = 0;
			ZDELETE(tbl);
			PPSetErrorDB();
		}
	}
	return tbl;
}

int SLAPI PPCvtTransfer4911::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		CpTransfTbl::Rec * data = (CpTransfTbl::Rec*)tbl->getDataBuf();
		memcpy(data, rec, sizeof(CpTransfTbl::Rec));
		return 1;
	}
};

class PPCvtTSession4911 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		TSessionTbl::Rec * p_data = (TSessionTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();
		memcpy(p_data, rec, sizeof(TSessionTbl::Rec));
		*pNewRecLen = -1;
		return 1;
	}
};

//
#if 0 // Перенесено в Convert5810 {

class PPCvtCGoodsLine4911 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion);
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtCGoodsLine4911::CreateTableInstance(int * pNeedConversion)
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

int SLAPI PPCvtCGoodsLine4911::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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

static CONVERT_PROC(_ConvertBill4911,       PPCvtBill4911);
static CONVERT_PROC(_ConvertPayPlan4911,    PPCvtPayPlan4911);
static CONVERT_PROC(_ConvertTransfer4911,   PPCvtTransfer4911);
static CONVERT_PROC(_ConvertCpTransf4911,   PPCvtCpTransf4911);
static CONVERT_PROC(_ConvertTSession4911,   PPCvtTSession4911);

int SLAPI Convert4911()
{
	return (_ConvertBill4911() && _ConvertPayPlan4911() && _ConvertCpTransf4911() &&
		_ConvertTransfer4911() && _ConvertTSession4911() /*&& _ConvertCGoodsLine4911()*/);
}
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtPrjTask6202 {
class PPCvtPrjTask5006 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * pRec, int * pNewRecLen)
	{
		struct OldRec { // size=92
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
		} * p_old_rec = (OldRec *)pRec;
		InventoryTbl::Rec * p_rec = (InventoryTbl::Rec*)tbl->getDataBuf();
		tbl->clearDataBuf();
		p_rec->BillID        = p_old_rec->BillID;
		p_rec->OprNo         = p_old_rec->OprNo;
		p_rec->GoodsID       = p_old_rec->GoodsID;
		p_rec->Flags         = p_old_rec->Flags;
    	INVENT_SETDIFFSIGN(p_rec->Flags, p_old_rec->DiffSign);
		INVENT_SETAUTOLINE(p_rec->Flags, p_old_rec->AutoLine);
		SETFLAG(p_rec->Flags, INVENTF_WRITEDOFF, p_old_rec->WritedOff);
		p_rec->UnitPerPack   = p_old_rec->UnitPerPack;
		p_rec->Quantity      = p_old_rec->Quantity;
		p_rec->StockRest     = p_old_rec->StockRest;
		p_rec->Price         = MONEYTOLDBL(p_old_rec->Price);
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
class PPCvtGoodsExt5109 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		GoodsExtTbl::Rec * data = (GoodsExtTbl::Rec*)tbl->getDataBuf();
		memcpy(data, rec, sizeof(GoodsExtTbl::Rec));
		return 1;
	}
};

#if 0 // @v6.2.2 Moved to PPCvtGoods6202 {
class PPCvtGoods5200 : public PPTableConversion {
public:
	int    RecSizeChanged;

	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * pNewRecLen)
	{
		Goods2Tbl::Rec * p_data = (Goods2Tbl::Rec*)pTbl->getDataBuf();
		if(RecSizeChanged) {
			struct OldRec {
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
			} * p_old_rec = (OldRec *)pRec;

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

int SLAPI Convert5200()
{
	int    ok = 1;
	PPWait(1);
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
	PPWait(0);
	return ok;
}
//
// Convert5207
//
class PPCvtCCheckLine5207 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
private:
	int    ver;
};

DBTable * SLAPI PPCvtCCheckLine5207::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new CCheckLineTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz)) {
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
	}
	return p_tbl;
}

int SLAPI PPCvtCCheckLine5207::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldCCheckLineRec {
		long   CheckID;      // -> CCheck.ID
		long   RByCheck;     // Счетчик строк по чеку
		long   DivID;        // Отдел магазина
		long   GoodsID;      // -> Goods.ID
		double Quantity;     // Количество товара
		char   Price[8];     // Цена
		char   Discount[8];  // Скидка
	} * p_4108_data = (OldCCheckLineRec*)pRec;
	struct CCheckLineBefore5207 { // Size = 28 /* before @v4.1.8 - 40 bytes */
		long   CheckID;      // ->CCheck.ID
		int16  RByCheck;     // Счетчик строк по чеку
		int16  DivID;        // Отдел магазина
		long   GoodsID;      // ->Goods.ID
		double Quantity;     // Количество товара
		long   Price;        // Цена 0.01
		long   Discount;     // Скидка 0.01
	} * p_5207_data = (CCheckLineBefore5207*)pRec;
	CCheckLineTbl::Rec * p_data = (CCheckLineTbl::Rec*)pTbl->getDataBuf();
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
	else {
		*p_data = *(CCheckLineTbl::Rec *)pRec;
	}
	return 1;
}
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtLocation6202 {

class PPCvtLocation5207 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
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
		} * p_old_data = (OldLocationRec*)pRec;
		LocationTbl::Rec * p_data = (LocationTbl::Rec*)pTbl->getDataBuf();
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

int SLAPI Convert5207()
{
	int    ok = 1;
	PPWait(1);
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
	PPWait(0);
	return ok;
}
//
//
//
#if 0 // {

class PPCvtBarcode5305 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtBarcode5305::CreateTableInstance(int * pNeedConversion)
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

int SLAPI PPCvtBarcode5305::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldBarcodeRec {
		long   GoodsID;     // Ид товара
		double Qtty;        // Количество единиц товара в упаковке
		long   BarcodeType; // Тип кодировки
		char   Code[16];    // Штрихкод
	} * p_old_data = (OldBarcodeRec *)pRec;
	BarcodeTbl::Rec * p_data = (BarcodeTbl::Rec*)pTbl->getDataBuf();
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
class PPCvtStaffPost5501 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new PersonPostTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz > sizeof(PersonPostTbl::Rec)); // Новый размер меньше предыдущего
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * pRec, int * pNewRecLen)
	{
		struct OldRec {
			long    ID;
			char    Code[12];
			long    PostID;
			long    PersonID;
			LDATE   Dt;
			char    Name[48];
			long    OrgID;
			long    DivID;
		} * p_old_rec = (OldRec *)pRec;
		PersonPostTbl::Rec * p_data = (PersonPostTbl::Rec *)newTbl->getDataBuf();
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * pNewRecLen)
	{
		StaffCalendarTbl::Rec * p_data = (StaffCalendarTbl::Rec*)pTbl->getDataBuf();
		memcpy(p_data, pRec, sizeof(StaffCalendarTbl::Rec));
		return 1;
	}
};

int SLAPI Convert5501()
{
	int    ok = 1;
	PPWait(1);
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
	PPWait(0);
	return ok;
}
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtProp6202 {

class PPCvtProperty5506 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * needConversion);
	int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtProperty5506::CreateTableInstance(int * needConversion)
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

int SLAPI PPCvtProperty5506::ConvertRec(DBTable * /*tbl*/, void * /*rec*/, int * /*pNewRecLen*/)
{
	return -1;
}

int SLAPI Convert5506()
{
	int ok = 1;
	PPWait(1);
	PPCvtProperty5506 cvt;
	ok = cvt.Convert() ? 1 : PPErrorZ();
	if(!ok)
		PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME);
	PPWait(0);
	return ok;
}
#endif // } 0 @v6.2.2 Moved to PPCvtProp6202
//
//
//
#if 0 // @v6.2.2 Moved to PPCvtWorld6202 {

class PPCvtWorld5512 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
class PPCvtObjSync5608 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * tbl = new ObjSyncTbl;
		if(!tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(tbl->getRecSize(&recsz))
				if(recsz > sizeof(ObjSyncTbl::Rec))
					*pNeedConversion = 1;
				else
					*pNeedConversion = 0;
			else {
				*pNeedConversion = 0;
				ZDELETE(tbl);
				PPSetErrorDB();
			}
		}
		return tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
		ObjSyncTbl::Rec * p_data = (ObjSyncTbl::Rec *)tbl->getDataBuf();
		tbl->clearDataBuf();
		const OldObjSyncRec * p_old_rec = (OldObjSyncRec *)rec;
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
#define IMPL_REF2CVT_FUNC(rec) int ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\
	assert(sizeof(rRec) == sizeof(Reference_Tbl::Rec)); \
	assert(sizeof(rRec2) == sizeof(Reference2Tbl::Rec)); \
	MEMSZERO(rRec2); \
	rRec2.Tag = rRec.Tag; \
	rRec2.ID = rRec.ID; \
	STRNSCPY(rRec2.Name, rRec.Name);

#define IMPL_REF2CVT_FUNC_NA(rec) int ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\
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
	return 1;
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
	return ok;
}

IMPL_REF2CVT_FUNC(PPBarcodeStruc) // {
	STRNSCPY(rRec2.Templ, rRec.Templ);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPUnit) // {
	STRNSCPY(rRec2.Abbr, rRec.Abbr);
	REF2CVT_ASSIGN(BaseUnitID);
	REF2CVT_ASSIGN(BaseRatio);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPNamedObjAssoc) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(ScndObjGrp);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(PrmrObjType);
	REF2CVT_ASSIGN(ScndObjType);
	return 1;
}

IMPL_REF2CVT_FUNC(PPPersonKind) // {
	REF2CVT_ASSIGN(CodeRegTypeID);
	REF2CVT_ASSIGN(FolderRegTypeID);
	return 1;
}

IMPL_REF2CVT_FUNC(PPPersonStatus) // {
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPELinkKind) // {
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Type);
	return 1;
}

IMPL_REF2CVT_FUNC(PPCurrency) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Code);
	return 1;
}

IMPL_REF2CVT_FUNC(PPCurRateType) // {
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPAmountType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Tax);
	REF2CVT_ASSIGN(TaxRate);
	return 1;
}

IMPL_REF2CVT_FUNC(PPOprType) // {
	STRNSCPY(rRec2.Pict, rRec.Pict);
	return 1;
}

IMPL_REF2CVT_FUNC(PPOpCounter) // {
	STRNSCPY(rRec2.CodeTemplate, rRec.CodeTemplate);
	REF2CVT_ASSIGN(ObjType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Counter);
	REF2CVT_ASSIGN(OwnerObjID);
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPAssetWrOffGrp) // {
	STRNSCPY(rRec2.Code, rRec.Code);
	REF2CVT_ASSIGN(WrOffType);
	REF2CVT_ASSIGN(WrOffTerm);
	REF2CVT_ASSIGN(Limit);
	REF2CVT_ASSIGN(Flags);
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPBillStatus) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(Rank);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPAccSheet) // {
	REF2CVT_ASSIGN(BinArID);
	REF2CVT_ASSIGN(CodeRegTypeID);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Assoc);
	REF2CVT_ASSIGN(ObjGroup);
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPLocPrinter) // {
	STRNSCPY(rRec2.Port, rRec.Port);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LocID);
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPTouchScreen) // {
	STRNSCPY(rRec2.GdsListFontName, rRec.GdsListFontName);
	REF2CVT_ASSIGN(TouchScreenType);
	REF2CVT_ASSIGN(AltGdsGrpID);
	REF2CVT_ASSIGN(GdsListFontHight);
	REF2CVT_ASSIGN(GdsListEntryGap);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC_NA(PPBarcodePrinter) // {
	STRNSCPY(rRec2.Port, rRec.Port);
	STRNSCPY(rRec2.LabelName, rRec.LabelName);
	REF2CVT_ASSIGN(Cpp);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(PrinterType);
	return 1;
}

IMPL_REF2CVT_FUNC_NA(PPInternetAccount) // {
	REF2CVT_ASSIGN(SmtpAuthType);
	REF2CVT_ASSIGN(Timeout);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPDBDiv) // {
	STRNSCPY(rRec2.Addr, rRec.Addr);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(IntrRcptOpr);
	REF2CVT_ASSIGN(OutCounter);
	return 1;
}

IMPL_REF2CVT_FUNC(PPGoodsType) // {
	REF2CVT_ASSIGN(WrOffGrpID);
	REF2CVT_ASSIGN(AmtCVat);
	REF2CVT_ASSIGN(AmtCost);
	REF2CVT_ASSIGN(AmtPrice);
	REF2CVT_ASSIGN(AmtDscnt);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPGoodsStrucHeader) // {
	REF2CVT_ASSIGN(VariedPropObjType);
	REF2CVT_ASSIGN(Period);
	REF2CVT_ASSIGN(CommDenom);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(ParentID);
	return 1;
}

IMPL_REF2CVT_FUNC(PPGoodsTax) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(VAT);
	REF2CVT_ASSIGN(Excise);
	REF2CVT_ASSIGN(SalesTax);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Order);
	REF2CVT_ASSIGN(UnionVect);
	return 1;
}

IMPL_REF2CVT_FUNC(PPRegisterType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(PersonKindID);
	REF2CVT_ASSIGN(RegOrgKind);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(CounterID);
	return 1;
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
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPWorldObjStatus) // {
	STRNSCPY(rRec2.Abbr, rRec.Abbr);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Kind);
	REF2CVT_ASSIGN(Code);
	return 1;
}

IMPL_REF2CVT_FUNC(PPPersonRelType) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(StatusRestriction);
	REF2CVT_ASSIGN(Cardinality);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPSalCharge) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(AmtID);
	REF2CVT_ASSIGN(CalID);
	REF2CVT_ASSIGN(WrOffOpID);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPDateTimeRep) // {
	REF2CVT_ASSIGN(Dtr);
	REF2CVT_ASSIGN(Duration);
	return 1;
}

IMPL_REF2CVT_FUNC(PPDutySched) // {
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(ObjType);
	REF2CVT_ASSIGN(ObjGroup);
	return 1;
}

IMPL_REF2CVT_FUNC(PPStaffCal) // {
	STRNSCPY(rRec2.Symb, rRec.Symb);
	REF2CVT_ASSIGN(PersonKind);
	REF2CVT_ASSIGN(SubstCalID);
	REF2CVT_ASSIGN(LinkObjType);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(LinkCalID);
	REF2CVT_ASSIGN(LinkObjID);
	return 1;
}

IMPL_REF2CVT_FUNC(PPScale) // {
	memcpy(rRec2.Port, rRec.Port, sizeof(rRec.Port));
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
	return 1;
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
	return 1;
}

IMPL_REF2CVT_FUNC(PPSCardSeries) // {
	STRNSCPY(rRec2.CodeTempl, rRec.CodeTempl);
	rRec2.QuotKindID_s = rRec.QuotKindID;
	REF2CVT_ASSIGN(PersonKindID);
	REF2CVT_ASSIGN(PDis);
	REF2CVT_ASSIGN(MaxCredit);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(Issue);
	REF2CVT_ASSIGN(Expiry);
	return 1;
}

IMPL_REF2CVT_FUNC(PPDraftWrOff) // {
	REF2CVT_ASSIGN(PoolOpID);
	REF2CVT_ASSIGN(DfctCompensOpID);
	REF2CVT_ASSIGN(DfctCompensArID);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPAdvBillKind) // {
	REF2CVT_ASSIGN(LinkOpID);
	REF2CVT_ASSIGN(Flags);
	return 1;
}

IMPL_REF2CVT_FUNC(PPGoodsBasket) // {
	REF2CVT_ASSIGN(Num);
	REF2CVT_ASSIGN(Flags);
	REF2CVT_ASSIGN(User);
	REF2CVT_ASSIGN(SupplID);
	return 1;
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
	return 1;
}

//#define IMPL_REF2CVT_FUNC(rec) int ConvertRef(const rec##_ & rRec, rec##2 & rRec2) {\

#define CONVERT_REF2_OBJ(obj, rec) case obj: THROW(ConvertRef(*(rec##_ *)&ref.data, *(rec##2 *)&ref2.data)); break

static int SLAPI ConvertRef2()
{
	int    ok = 1, ta = 0, r;
	RECORDNUMBER numrec2 = 0;
	Reference2Tbl ref2;
	ref2.getNumRecs(&numrec2);
	if(numrec2 == 0) {
		Reference_Tbl ref;
		Reference_Tbl::Key0 k0;
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
					CONVERT_REF2_OBJ(PPOBJ_PRSNKIND,   PPPersonKind);
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
	PPCvtCGoodsLine5810() : PPTableConversion()
	{
		C4911 = -1;
	}
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion);
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
private:
	int    C4911; // Если !0, то исходный файл находится в состоянии, предшествующем v4.9.11
};

DBTable * SLAPI PPCvtCGoodsLine5810::CreateTableInstance(int * pNeedConversion)
{
	if(C4911 < 0)
		C4911 = 0;
	DBTable * tbl = new CGoodsLineTbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(tbl->getRecSize(&recsz)) {
			if(recsz < sizeof(CGoodsLineTbl::Rec)) {
				C4911 = BIN(recsz < 48);
				*pNeedConversion = 1;
			}
			else
				*pNeedConversion = 0;
		}
		else {
			*pNeedConversion = 0;
			ZDELETE(tbl);
			PPSetErrorDB();
		}
	}
	return tbl;
}

int SLAPI PPCvtCGoodsLine5810::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	struct OldCGoodsLineRec4911 { //
		LDATE  Dt;
		long   SessID;
		long   GoodsID;
		double Qtty;
		double Rest;
		char   Sum[8];
		long   AltGoodsID;
		float  AltGoodsQtty;
	} * p_old_rec_4911 = (OldCGoodsLineRec4911 *)rec;
	struct OldCGoodsLineRec { //
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
	} * p_old_rec = (OldCGoodsLineRec *)rec;

	CGoodsLineTbl::Rec * p_data = (CGoodsLineTbl::Rec *)tbl->getDataBuf();
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
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion);
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtBankAccount5810::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new BankAccount_Pre9004Tbl;
	if(!tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(tbl->getRecSize(&recsz)) {
			*pNeedConversion = BIN(recsz < sizeof(BankAccount_Pre9004Tbl::Rec));
		}
		else {
			*pNeedConversion = 0;
			ZDELETE(tbl);
			PPSetErrorDB();
		}
	}
	return tbl;
}

int SLAPI PPCvtBankAccount5810::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	struct OldBankAccountRec {
		long   ID;
		long   PersonID;
		long   BankID;
		long   AccType;
		char   Acct[24];
		LDATE  OpenDate;
		long   Flags;
		long   CorrAcc;
		long   CorrArt;
	} * p_old_rec = (OldBankAccountRec *)rec;
	BankAccount_Pre9004Tbl::Rec * p_data = (BankAccount_Pre9004Tbl::Rec *)tbl->getDataBuf();
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

int SLAPI Convert5810()
{
	int    ok = 1;
	PPWait(1);
	THROW(ConvertRef2());
	{
		PPCvtCGoodsLine5810 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtBankAccount5810 cvt02;
		THROW(cvt02.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}

class PPCvtSpecSeries6109 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * needConversion);
	virtual int SLAPI ConvertRec(DBTable * newTbl, void * oldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtSpecSeries6109::CreateTableInstance(int * pNeedConversion)
{
	DBTable * tbl = new SpecSeriesTbl;
	if(!tbl)
		PPSetError(PPERR_NOMEM);
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(tbl->getRecSize(&recsz)) {
			*pNeedConversion = BIN(recsz < sizeof(SpecSeriesTbl::Rec));
		}
		else {
			*pNeedConversion = 0;
			ZDELETE(tbl);
			PPSetErrorDB();
		}
	}
	return tbl;
}

int SLAPI PPCvtSpecSeries6109::ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
{
	struct OldSpecSeriesRec {
		char   Serial[24];
		char   Barcode[24];
		char   GoodsName[64];
		char   ManufName[48];
		LDATE  InfoDate;
		int32  InfoKind;
		char   InfoIdent[24];
		uint8  Reserve[48];
	} * p_old_rec = (OldSpecSeriesRec *)rec;
	SpecSeriesTbl::Rec * p_data = (SpecSeriesTbl::Rec *)tbl->getDataBuf();
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
int SLAPI Convert6109()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtSpecSeries6109 cvt01;
		THROW(cvt01.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
*/
//
// @v6.2.2 {
//
class PPCvtObjAssoc6202 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		ObjAssocTbl::Rec * p_data = (ObjAssocTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(ObjAssocTbl::Rec));
		return 1;
	}
};

class PPCvtObjProp6202 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
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
		pNewTbl->copyBufLobFrom(pOldRec, *pNewRecLen);
		return 1;
	}
};

class PPCvtLocation6202 : public PPTableConversion {
public:
	PPCvtLocation6202() : PPTableConversion()
	{
		Before5207 = 0;
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

	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		pNewTbl->clearDataBuf();
		LocationTbl::Rec * p_data = (LocationTbl::Rec *)pNewTbl->getDataBuf();
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
			} * p_old_rec = (OldLocationRec*)pOldRec;
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Counter);
			FLD_ASSIGN(ParentID);
			FLD_ASSIGN(OwnerID);
			p_data->Type  = (int16)p_old_rec->Type;
			p_data->Flags = (int16)p_old_rec->Flags;
			FLD_ASSIGN(CityID);
			FLD_ASSIGN(RspnsPersonID);
			STRNSCPY(p_data->Name, p_old_rec->Name);
			STRNSCPY(p_data->Code, p_old_rec->Code);
			LocationCore::SetExField(p_data, LOCEXSTR_ZIP, p_old_rec->ZIP);
			LocationCore::SetExField(p_data, LOCEXSTR_SHORTADDR, p_old_rec->Address);
			return 1;
		}
		else {
			Location_Before6202 * p_old_rec = (Location_Before6202 *)pOldRec;
			FLD_ASSIGN(ID);
			FLD_ASSIGN(Counter);
			FLD_ASSIGN(ParentID);
			FLD_ASSIGN(OwnerID);
			p_data->Type  = (int16)p_old_rec->Type;
			p_data->Flags = (int16)p_old_rec->Flags;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		WorldTbl::Rec * p_data = (WorldTbl::Rec *)pNewTbl->getDataBuf();
		World_Before6202 * p_old_rec = (World_Before6202 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PersonTbl::Rec * p_data = (PersonTbl::Rec *)pNewTbl->getDataBuf();
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

class PPCvtPersonKind6202 : public PPTableConversion {
public:
	struct PersonKind_Before6202 {
		long   KindID;
		long   PersonID;
		char   Name[128];
	};
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PersonKindTbl::Rec * p_data = (PersonKindTbl::Rec *)pNewTbl->getDataBuf();
		PersonKind_Before6202 * p_old_rec = (PersonKind_Before6202 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ArticleTbl::Rec * p_data = (ArticleTbl::Rec *)pNewTbl->getDataBuf();
		Article_Before6202 * p_old_rec = (Article_Before6202 *)pOldRec;
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

static int SLAPI GetTransport_Before6202(PPID id, const Goods2_Before6202 * pOldRec, PPTransport * pRec)
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

	PPCvtGoods6202() : PPTableConversion()
	{
		Before4405 = 0;
	}
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI DestroyTable(DBTable * pTbl)
	{
		if(pTbl)
			delete (GoodsCore *)pTbl;
		return 1;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pOldRec, int * pNewRecLen)
	{
		int    ok = 1;
#define FLD_ASSIGN(f) p_data->f = p_old_rec->f
		pTbl->clearDataBuf();
		Goods2Tbl::Rec * p_data = (Goods2Tbl::Rec *)pTbl->getDataBuf();
		if(Before4405) {
			Goods2_Before4405 * p_old_rec = (Goods2_Before4405 *)pOldRec;
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
			Goods2_Before6202 * p_old_rec = (Goods2_Before6202 *)pOldRec;
			if(p_old_rec->Kind == PPGDSK_TRANSPORT) {
				PPTransport tr_rec;
				BarcodeArray bc_list;
				GetTransport_Before6202(p_old_rec->ID, p_old_rec, &tr_rec);
				THROW(PPObjTransport::MakeStorage(p_old_rec->ID, &tr_rec, p_data, &bc_list));
				THROW(((GoodsCore *)pTbl)->UpdateBarcodes(p_old_rec->ID, &bc_list, 0));
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
		CATCH
			ok = 0;
		ENDCATCH
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		AdvBillItemTbl::Rec * p_data = (AdvBillItemTbl::Rec *)pNewTbl->getDataBuf();
		AdvBillItem_Before6202 * p_old_rec = (AdvBillItem_Before6202 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		PrjTaskTbl::Rec * p_data = (PrjTaskTbl::Rec *)pNewTbl->getDataBuf();
		PrjTask_Before6202 * p_old_rec = (PrjTask_Before6202 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ProjectTbl::Rec * p_data = (ProjectTbl::Rec *)pNewTbl->getDataBuf();
		Project_Before6202 * p_old_rec = (Project_Before6202 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		pNewTbl->clearDataBuf();
		ObjSyncQueueTbl::Rec * p_data = (ObjSyncQueueTbl::Rec *)pNewTbl->getDataBuf();
		ObjSyncQueue_Before6202 * p_old_rec = (ObjSyncQueue_Before6202 *)pOldRec;
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

int SLAPI Convert6202()
{
	int    ok = 1;
	PPWait(1);
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
		PPCvtPerson6202 cvt06;
		THROW(cvt06.Convert());
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
		PPCvtPrjTask6202 cvt11;
		THROW(cvt11.Convert());
	}
	{
		PPCvtProject6202 cvt12;
		THROW(cvt12.Convert());
	}
	{
		PPCvtObjSyncQueue6202 cvt13;
		THROW(cvt13.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
//
// } @v6.2.2
//
class PPCvtSalary6303 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
	{
		SalaryTbl::Rec * p_data = (SalaryTbl::Rec *)tbl->getDataBuf();
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
	PPCvtInventory6407() : PPTableConversion()
	{
		Before5009 = 0;
	}
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * tbl, void * pRec, int * pNewRecLen)
	{
		InventoryTbl::Rec * p_rec = (InventoryTbl::Rec*)tbl->getDataBuf();
		tbl->clearDataBuf();
		if(Before5009) {
			struct Rec_Before5009 { // size=92
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
			} * p_old_rec = (Rec_Before5009 *)pRec;
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
			p_rec->Price         = MONEYTOLDBL(p_old_rec->Price);
			p_rec->StockPrice    = MONEYTOLDBL(p_old_rec->StockPrice);
			p_rec->CSesDfctQtty  = 0;
			p_rec->CSesDfctPrice = 0;
			p_rec->WrOffPrice    = p_rec->StockPrice;
		}
		else {
			struct Rec_Before6407 {
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
			} * p_old_rec = (Rec_Before6407 *)pRec;
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

int SLAPI ConvertBillExtRec_6407(PropertyTbl::Rec * pRec); // @prototype @defined(bill.cpp)

class PPCvtProperty6407 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		int    ok = 1;
		PropertyTbl * p_tbl = (PropertyTbl *)pNewTbl;
		RECORDSIZE fix_rec_size = 0;
		p_tbl->getRecSize(&fix_rec_size);
		PropertyTbl::Rec * p_rec = (PropertyTbl::Rec*)p_tbl->getDataBuf();
		PropertyTbl::Rec * p_rec_old = (PropertyTbl::Rec*)pOldRec;
		PropertyTbl::Rec temp_rec = *p_rec_old;
		if(p_rec_old->ObjType == PPOBJ_OPRKIND && p_rec_old->Prop > 0 && p_rec_old->Prop <= PP_MAXATURNTEMPLATES) {
			PPAccTurnTempl::Convert_6407(&temp_rec);
			uint s = sizeof(PPAccTurnTempl);
			p_tbl->copyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		else if(p_rec_old->ObjType == PPOBJ_BILL && p_rec_old->Prop == BILLPRP_EXTRA) {
			*p_rec = *p_rec_old;
			ConvertBillExtRec_6407(&temp_rec);
			uint s = sizeof(BillCore::Extra_Strg);
			p_tbl->copyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		else {
			uint s = *pNewRecLen;
			p_tbl->copyBufFrom(&temp_rec, fix_rec_size);
			p_tbl->writeLobData(p_tbl->VT, PTR8(&temp_rec)+fix_rec_size, (s > fix_rec_size) ? (s-fix_rec_size) : 0);
		}
		return ok;
	}
	virtual int SLAPI Final(DBTable * pNewTbl)
	{
		int    ok = -1;
		if(pNewTbl) {
			PropertyTbl::Rec rec;
			MEMSZERO(rec);
			rec.ObjType = PPOBJ_DBCONVERT;
			rec.ObjID = 0x060407;
			rec.Prop = 1;
			ok = pNewTbl->insertRecBuf(&rec) ? 1 : PPSetErrorDB();
		}
		return ok;
	}
};

int SLAPI Convert6407()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtInventory6407 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtProperty6407 cvt02;
		THROW(cvt02.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
//
//
//
class PPCvtCurRest6611 : public PPTableConversion {
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CurRestTbl::Rec * p_data = (CurRestTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(CurRestTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert6611, PPCvtCurRest6611);
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
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
int SLAPI ConvertQuot720()
{
	int    ok = 1;
	IterCounter cntr;
	PPObjQuotKind qk_obj;
	PPQuotKind qk_rec;
	QuotationCore qc;
	Quotation2Core qc2;
	QuotationTbl::Key0 k0;
	MEMSZERO(k0);
	{
		PPTransaction tra(1);
		THROW(tra);
		PPInitIterCounter(cntr, &qc);
		PPWait(1);
		if(qc.search(0, &k0, spFirst)) {
			do {
				if(qc.data.Actual > 0) {
					if(qk_obj.Fetch(qc.data.Kind, &qk_rec) > 0) {
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
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
#if 0 // { Перенесено в PPCvtVatBook7311

class PPCvtVatBook7208 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new VATBookTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int    num_seg = 0;
			DBIdxSpec * p_is = p_tbl->getIndexSpec(1, &num_seg);
			*pNeedConversion = BIN(num_seg < 5);
			SAlloc::F(p_is);
		}
		return p_tbl;
	}
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		VATBookTbl::Rec * p_data = (VATBookTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(VATBookTbl::Rec));
		long * p_old_line_type = (long *)(&((VATBookTbl::Rec *)pOldRec)->LineType_);
		p_data->LineType_ = (int16)*p_old_line_type;
		p_data->LineSubType = 0;
		return 1;
	}
};

CONVERT_PROC(Convert7208, PPCvtVatBook7208);

#endif // } 0 Перенесено в PPCvtVatBook7311
//
//
//
class PPCvtQuot2Rel7305 : public PPTableConversion {
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		Quot2RelTbl::Rec * p_data = (Quot2RelTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(Quot2RelTbl::Rec));
		return 1;
	}
};

class PPCvtObjTag7305 : public PPTableConversion {
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		ObjTagTbl::Rec * p_data = (ObjTagTbl::Rec*)pNewTbl->getDataBuf();
		ObjTagTbl::Rec * p_old_rec = (ObjTagTbl::Rec *)pOldRec;
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

int SLAPI Convert7305()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtQuot2Rel7305 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtObjTag7305 cvt02;
		THROW(cvt02.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
//
//
//
class PPCvtVatBook7311 : public PPTableConversion {
public:
	int    Pre7208;
	PPCvtVatBook7311() : PPTableConversion()
	{
		Pre7208 = 0;
	}
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	int SLAPI PPCvtVatBook7311::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct OldRec {
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
		} * p_old_rec = (OldRec *)pRec;

		VATBookTbl::Rec * p_data = (VATBookTbl::Rec*)pTbl->getDataBuf();
		memzero(p_data, sizeof(*p_data));
#define CPYFLD(f)    p_data->f = p_old_rec->f
#define CPYBUFFLD(f) memcpy(p_data->f, p_old_rec->f, sizeof(p_data->f))
			CPYFLD(ID);
			if(Pre7208) {
				long * p_old_line_type = (long *)(&p_old_rec->LineType_);
				p_data->LineType_ = (int16)*p_old_line_type;
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
//
//
//
class PPCvtTech7506 : public PPTableConversion {
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		TechTbl::Rec * p_data = (TechTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(TechTbl::Rec));
		p_data->ParentID = 0;
		p_data->OrderN = ++OrderN_Counter;
		return 1;
	}
	long   OrderN_Counter;
};

CONVERT_PROC(Convert7506, PPCvtTech7506);
//
//
//
class PPCvtCCheckExt7601 : public PPTableConversion {
public:
	PPCvtCCheckExt7601()
	{
		P_CcT = 0;
		Pre6708 = 0;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CCheckExtTbl::Rec * p_data = (CCheckExtTbl::Rec *)pNewTbl->getDataBuf();
		CCheckExt_Before6708 * p_old_rec_6708 = (CCheckExt_Before6708 *)pOldRec;
		CCheckExt_Before7601 * p_old_rec_7601 = (CCheckExt_Before7601 *)pOldRec;
		memzero(p_data, sizeof(*p_data));
		if(Pre6708) {
			p_data->CheckID = p_old_rec_6708->CheckID;
			p_data->SalerID = p_old_rec_6708->SalerID;
			p_data->TableNo = p_old_rec_6708->TableNo;
			p_data->AddPaym_unused = p_old_rec_6708->AddPaym; // @v9.0.4 _unused
			p_data->GuestCount = p_old_rec_6708->GuestCount;

			p_data->StartOrdDtm.SetZero();
			p_data->EndOrdDtm.SetZero();
			p_data->CreationDtm.SetZero();
		}
		else {
			p_data->CheckID = p_old_rec_7601->CheckID;
			p_data->SalerID = p_old_rec_7601->SalerID;
			p_data->TableNo = p_old_rec_7601->TableNo;
			p_data->AddPaym_unused = p_old_rec_7601->AddPaym; // @v9.0.4 _unused
			p_data->GuestCount = p_old_rec_7601->GuestCount;

			p_data->AddrID = p_old_rec_7601->AddrID;
			p_data->AddCrdCardID_unused = p_old_rec_7601->AddCrdCardID; // @v9.0.4 _unused
			p_data->AddCrdCardPaym_unused = p_old_rec_7601->AddCrdCardPaym; // @v9.0.4 _unused
			p_data->LinkCheckID = p_old_rec_7601->LinkCheckID;
			p_data->EndOrdDtm = p_old_rec_7601->EndOrdDtm;
			if(!p_old_rec_7601->StartOrdDtm) {
				p_data->StartOrdDtm.SetZero();
				p_data->CreationDtm.SetZero();
			}
			else {
				CCheckTbl::Key0 k0;
				k0.ID = p_data->CheckID;
				if(P_CcT && P_CcT->search(0, &k0, spEq)) {
					if(P_CcT->data.Flags & CCHKF_DELIVERY) {
						p_data->StartOrdDtm = p_old_rec_7601->StartOrdDtm;
						p_data->CreationDtm.SetZero();
					}
					else {
						p_data->StartOrdDtm.SetZero();
						p_data->CreationDtm = p_old_rec_7601->StartOrdDtm;
					}
				}
				else {
					p_data->StartOrdDtm.SetZero();
					p_data->CreationDtm.SetZero();
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
#if 0 // @v9.4.0 перенесено в PPCvtSCard9400 {

class PPCvtSCard7702 : public PPTableConversion {
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		SCardTbl * p_tbl = new SCardTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			DbTableStat stat;
			p_tbl->GetFileStat(-1, &stat);
			if(stat.FixRecSize < sizeof(SCardTbl::Rec)) {
				*pNeedConversion = 1;
			}
			else {
				*pNeedConversion = 0;
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
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
		assert(sizeof(SCard_Before7702)==104);
		SCardTbl::Rec * p_data = (SCardTbl::Rec*)pNewTbl->getDataBuf();
		SCard_Before7702 * p_old_rec = (SCard_Before7702 *)pOldRec;
		memzero(p_data, sizeof(*p_data));
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
		STRNSCPY(p_data->Password, p_old_rec->Password);
		return 1;
	}
};

CONVERT_PROC(Convert7702, PPCvtSCard7702);

#endif // } @v9.4.0
//
//
//
class PPCvtSJ7708 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		SysJournalTbl::Rec * p_data = (SysJournalTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

class PPCvtSJR7708 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		SjRsrvTbl::Rec * p_data = (SjRsrvTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

int SLAPI Convert7708()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtSJ7708 cvt01;
		THROW(cvt01.Convert());
	}
	{
		PPCvtSJR7708 cvt02;
		THROW(cvt02.Convert());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}
//
//
//
class PPCvtSCardOp7712 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new SCardOpTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz < sizeof(SCardOpTbl::Rec));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
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

		SCardOpTbl::Rec * p_data = (SCardOpTbl::Rec*)pTbl->getDataBuf();
		SCardOpRec_Before7712 * p_old_rec = (SCardOpRec_Before7712 *)pRec;
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

class PPCvtCSession7712 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
	{
		CSessionTbl::Rec * p_data = (CSessionTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(*p_data));
		return 1;
	}
};

int SLAPI Convert7712()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtSCardOp7712 cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}
	{
		PPCvtCSession7712 cvt2;
		ok = cvt2.Convert() ? 1 : PPErrorZ();
	}
	PPWait(0);
	return ok;
}
//
// @vmiller
//
class PPCvtChkOpJrnl : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CheckOpJrnlTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz < sizeof(CheckOpJrnlTbl::Rec));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
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

		CheckOpJrnlTbl::Rec * p_data = (CheckOpJrnlTbl::Rec*)pTbl->getDataBuf();
		CheckOpJrnlRec_Before7909 * p_old_rec = (CheckOpJrnlRec_Before7909 *)pRec;
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
int SLAPI Convert7907()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtChkOpJrnl cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}
	PPWait(0);
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
	SLAPI PPWorkbookPacket_Pre813()
	{
		destroy();
	}
	SLAPI ~PPWorkbookPacket_Pre813()
	{
		destroy();
	}
	void SLAPI destroy()
	{
		MEMSZERO(Rec);
		TagL.Destroy();
		F.Clear();
	}

	PPWorkbook Rec;
	ObjTagList TagL;        // Список тегов
	ObjLinkFiles F;
};

class PPObjWorkbook_Pre813 : public PPObjReference {
public:
	SLAPI  PPObjWorkbook_Pre813(void * extraPtr = 0) : PPObjReference(PPOBJ_WORKBOOK_PRE813, extraPtr)
	{
		ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
	}
	int    SLAPI GetPacket(PPID id, PPWorkbookPacket_Pre813 * pPack);
	int    SLAPI RemovePacket(PPID id, int use_ta);
private:
	virtual void FASTCALL Destroy(PPObjPack * pPack);
	int    SLAPI CheckParent(PPID itemID, PPID parentID);
	int    SLAPI GetItemPath(PPID itemID, SString & rPath);
};

IMPL_DESTROY_OBJ_PACK(PPObjWorkbook_Pre813, PPWorkbookPacket_Pre813);

int SLAPI PPObjWorkbook_Pre813::RemovePacket(PPID id, int use_ta)
{
	int    ok = 1;
	PPID   hid = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(CheckRights(PPR_DEL));
		THROW(ref->RemoveItem(Obj, id, 0));
		THROW(ref->RemoveProp(Obj, id, 0, 0));
		THROW(ref->Ot.PutList(Obj, id, 0, 0));
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

int SLAPI PPObjWorkbook_Pre813::GetPacket(PPID id, PPWorkbookPacket_Pre813 * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		pPack->TagL.Destroy();
		pPack->F.Clear();
		THROW(ref->Ot.GetList(Obj, id, &pPack->TagL));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjWorkbook_Pre813::CheckParent(PPID itemID, PPID parentID)
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

int SLAPI PPObjWorkbook_Pre813::GetItemPath(PPID itemID, SString & rPath)
{
	int   ok = 0;
	PPWorkbook rec;
	SString temp_buf;
	rPath = 0;
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

int SLAPI ConvertWorkbook813()
{
	int    ok = 1;
	{
		PPObjWorkbook wb_obj;
		PPObjWorkbook_Pre813 wb_obj_pre;
		PPWorkbook rec_pre;
		SString file_name, temp_file_name; //, temp_path;
		PPIDArray id_list;
		SPathStruc ps;
		PPWait(1);
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
					ps.Clear().Split(file_name);
					PPMakeTempFileName("wbc813", ps.Ext, 0, temp_file_name);
					THROW_SL(SCopyFile(file_name, temp_file_name, 0, FILE_SHARE_READ, 0));
					pack.F.Replace(0, temp_file_name);
				}
				{
					wb_obj.P_Tbl->copyBufFrom(&pack.Rec);
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
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
#if 0 // @v8.3.6 Конвертация совмещена с PPCvtRegister8306 {
class PPCvtRegister8203 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			*pNeedConversion = BIN(num_keys < 5);
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		RegisterTbl::Rec * p_data = (RegisterTbl::Rec*)pNewTbl->getDataBuf();
		memcpy(p_data, pOldRec, sizeof(RegisterTbl::Rec));
		return 1;
	}
};

CONVERT_PROC(Convert8203, PPCvtRegister8203);
#endif // } 0 @v8.3.6 Конвертация совмещена с PPCvtRegister8306
//
//
//
class PPCvtRegister8306 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new RegisterTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			int16  num_keys;
			p_tbl->getNumKeys(&num_keys);
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz < sizeof(RegisterTbl::Rec) || num_keys < 5);
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
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
			long   ExtID; // @v9.0.4 Reserve-->ExtID
		};
		RegisterTbl::Rec * p_data = (RegisterTbl::Rec*)pNewTbl->getDataBuf();
		RegisterTblRec_Before8306 * p_old_rec = (RegisterTblRec_Before8306 *)pOldRec;
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
//
//
//
class PPCvtBarcode8800 : public PPTableConversion {
public:
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtBarcode8800::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new BarcodeTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(BarcodeTbl::Rec)); // Новый размер =38 bytes
	}
	return p_tbl;
}

int SLAPI PPCvtBarcode8800::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldBarcodeRec {
		long   GoodsID;
		double Qtty;
		long   BarcodeType;
		char   Code[16];
	} * p_old_data = (OldBarcodeRec *)pRec;
	BarcodeTbl::Rec * p_data = (BarcodeTbl::Rec*)pTbl->getDataBuf();
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
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion);
	int    SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen);
};

DBTable * SLAPI PPCvtArGoodsCode8800::CreateTableInstance(int * pNeedConversion)
{
	DBTable * p_tbl = new ArGoodsCodeTbl;
	if(!p_tbl)
		PPSetErrorNoMem();
	else if(pNeedConversion) {
		RECORDSIZE recsz = 0;
		if(p_tbl->getRecSize(&recsz))
			*pNeedConversion = BIN(recsz < sizeof(ArGoodsCodeTbl::Rec)); // Новый размер =38 bytes
	}
	return p_tbl;
}

int SLAPI PPCvtArGoodsCode8800::ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
{
	struct OldArGoodsCodeRec {
		long   GoodsID;
		long   ArID;
		long   Pack;
		char   Code[16];
	} * p_old_data = (OldArGoodsCodeRec *)pRec;
	ArGoodsCodeTbl::Rec * p_data = (ArGoodsCodeTbl::Rec*)pTbl->getDataBuf();
	memzero(p_data, sizeof(ArGoodsCodeTbl::Rec));
	p_data->GoodsID = p_old_data->GoodsID;
	p_data->ArID = p_old_data->ArID;
	p_data->Pack = p_old_data->Pack;
	STRNSCPY(p_data->Code, p_old_data->Code);
	return 1;
}

int SLAPI Convert8800()
{
	int    ok = 1;
	PPWait(1);
	{
		PPCvtBarcode8800 cvt1;
		ok = cvt1.Convert() ? 1 : PPErrorZ();
	}
	{
		PPCvtArGoodsCode8800 cvt2;
		ok = cvt2.Convert() ? 1 : PPErrorZ();
	}
	PPWait(0);
	return ok;
}
//
//
//
class PPCvtCpTransf8910 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CpTransfTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz > (sizeof(CpTransfTbl::Rec) - sizeof(((CpTransfTbl::Rec*)0)->Tail)));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * tbl, void * rec, int * pNewRecLen)
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
		CpTransfRec_Before8910 * p_old_data = (CpTransfRec_Before8910 *)rec;
		CpTransfTbl::Rec * data = (CpTransfTbl::Rec*)tbl->getDataBuf();
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
		cpext.PartNo[0] = 0;
		cpext.Clb[0] = 0;
		STRNSCPY(cpext.PartNo, p_old_data->PartNo);
		STRNSCPY(cpext.Clb, p_old_data->Clb);
        CpTransfCore::PutExt(*data, &cpext);
		return 1;
	}
};

CONVERT_PROC(Convert8910, PPCvtCpTransf8910);
//
//
//
#define STAFFLIST_EXCL_ID 1000000

static int SLAPI ConvertStaffList9003()
{
	int    ok = 1;
	int    db_locked = 0;
	int    need_conversion = 0;
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
								p_tbl->copyBufTo(&rec);
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
								MEMSZERO(spec_rec);
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
int SLAPI Convert9003()
{
	int    ok = 1;
	PPWait(1);
	{
		ok = ConvertStaffList9003() ? 1 : PPErrorZ();
	}
	PPWait(0);
	return ok;
}
*/
//
//
//
#define ACCOUNT_EXCL_ID   1000000
#define ACCOUNT_EXCL2_ID  1000001

static int SLAPI ConvertAccount9004()
{
	int    ok = 1;
	int    db_locked = 0;
	int    need_conversion = 0;
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
										ref_t.copyBufTo(&acc_rec);
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
									p_tbl->copyBufTo(&rec);
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
									new_item.Limit = rec.Limit;
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
							MEMSZERO(spec_rec);
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

static int SLAPI ConvertBankAccount9004()
{
	int    ok = 1;
	int    db_locked = 0;
	int    need_conversion = 0;
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
								p_tbl->copyBufTo(&rec);
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
								MEMSZERO(spec_rec);
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
	DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new CCheckPaymTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz))
				*pNeedConversion = BIN(recsz < sizeof(CCheckPaymTbl::Rec)); // Новый размер =38 bytes
		}
		return p_tbl;
	}
	int    SLAPI ConvertRec(DBTable * pTbl, void * pRec, int * /*pNewRecLen*/)
	{
		struct OldCCheckPaymRec {
			long   CheckID;
			int16  RByCheck;
			int16  PaymType;
			long   Amount;
			long   SCardID;
		} * p_old_data = (OldCCheckPaymRec *)pRec;
		CCheckPaymTbl::Rec * p_data = (CCheckPaymTbl::Rec *)pTbl->getDataBuf();
		memzero(p_data, sizeof(CCheckPaymTbl::Rec));
		p_data->CheckID  = p_old_data->CheckID;
		p_data->RByCheck = p_old_data->RByCheck;
		p_data->PaymType = p_old_data->PaymType;
		p_data->Amount   = p_old_data->Amount;
		p_data->SCardID  = p_old_data->SCardID;
		return 1;
	}
};

int SLAPI Convert9004()
{
	int    ok = 1;
	PPWait(1);
	{
		THROW(ConvertStaffList9003());
		THROW(ConvertAccount9004());
		THROW(ConvertBankAccount9004());
	}
	{
		PPCvtCCheckPaym9004 cvt2;
		THROW(cvt2.Convert());
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}
//
//
//
class PPCvtGoodsDebt9108 : public PPTableConversion {
public:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new GoodsDebtTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz != sizeof(GoodsDebtTbl::Rec));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
	{
		struct GoodsDebtTblRec_Before9108 {
			long   GoodsID;
			long   ArID;
			LDATE  Dt;
			double SaldoQtty;
			double SaldoAmount;
			uint8  Reserve[20];
		};
		GoodsDebtTbl::Rec * p_data = (GoodsDebtTbl::Rec*)pNewTbl->getDataBuf();
		GoodsDebtTblRec_Before9108 * p_old_rec = (GoodsDebtTblRec_Before9108 *)pOldRec;
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
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
	{
		DBTable * p_tbl = new EgaisProductTbl;
		if(!p_tbl)
			PPSetErrorNoMem();
		else if(pNeedConversion) {
			RECORDSIZE recsz = 0;
			if(p_tbl->getRecSize(&recsz)) {
				*pNeedConversion = BIN(recsz < sizeof(EgaisProductTbl::Rec));
			}
		}
		return p_tbl;
	}
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * /*pNewRecLen*/)
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
			//long   Flags; // @v9.2.12
		};
		EgaisProductTbl::Rec * p_data = (EgaisProductTbl::Rec*)pNewTbl->getDataBuf();
		EgaisProductTblRec_Before9214 * p_old_rec = (EgaisProductTblRec_Before9214 *)pOldRec;
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
	PPCvtSCard9400() : PPTableConversion()
	{
		Stage = 0;
	}
private:
	virtual DBTable * SLAPI CreateTableInstance(int * pNeedConversion)
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
	virtual int SLAPI ConvertRec(DBTable * pNewTbl, void * pOldRec, int * pNewRecLen)
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
		SCardTbl::Rec * p_data = (SCardTbl::Rec*)pNewTbl->getDataBuf();
		memzero(p_data, sizeof(*p_data));
		SString pw_buf;
		if(Stage == 1) {
			assert(sizeof(SCard_Before7702)==104);
			SCard_Before7702 * p_old_rec = (SCard_Before7702 *)pOldRec;
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
			assert(sizeof(SCard_Before9400)==128);
			SCard_Before9400 * p_old_rec = (SCard_Before9400 *)pOldRec;
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
			THROW(UtrC.SetText(TextRefIdent(PPOBJ_SCARD, p_data->ID, PPTRPROP_SCARDEXT), ext_buffer.Transf(CTRANSF_INNER_TO_UTF8), 0));
		}
		CATCHZOK
		return ok;
	}
private:
	int    Stage; // 1 - конвертация из версии менее 7.7.2, 0 - из более новой
	UnxTextRefCore UtrC;
};

CONVERT_PROC(Convert9400, PPCvtSCard9400);
