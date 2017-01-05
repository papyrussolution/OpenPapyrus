// PETROVI.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2008, 2010, 2011, 2013, 2015, 2016
// @codepage windows-1251
//
// Интерфейс (асинхронный) с ККМ через драйвер PETROVICH (Ока)
// Ладога-Ф, Электроника-92-Аквариус, ЭКР-3102Ф
//
// Since @v3.2.0 under construction
// Modified @v3.9.16
//
#include <pp.h>
#pragma hdrstop

#define DEFAULT_PETROVICH_GOODS_ID_BIAS 0 // @v6.3.4 200-->0

class ACS_PETROVICH : public CS_1 {
public:
	SLAPI  ACS_PETROVICH(PPID n) : CS_1(n)
	{
		P_SessList = 0;
		PPIniFile ini_file;
		int    bias = 0;
		if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_PETROVI4BIAS, &bias) > 0)
			GoodsIdBias = bias;
		else
			GoodsIdBias = DEFAULT_PETROVICH_GOODS_ID_BIAS;
	}
	SLAPI ~ACS_PETROVICH()
	{
		delete P_SessList;
	}
	virtual int SLAPI ExportData(int);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
private:
	int    SLAPI ConvertWareList(int);
	int    SLAPI UpdateSessions();

	struct SessEntry {
		char InFilePath[MAXPATH];
	};
	SArray * P_SessList;
	long   GoodsIdBias;
};

class CM_PETROVICH : public PPCashMachine {
public:
	SLAPI CM_PETROVICH(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_PETROVICH(NodeID); }
};

REGISTER_CMT(PETROVICH,0,1);

int SLAPI ACS_PETROVICH::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	ZDELETE(P_SessList);

	int    ok = 1;
	PPAsyncCashNode acn;
	PPObjCashNode cn_obj;
	THROW(cn_obj.GetAsync(NodeID, &acn) > 0);
	if(acn.ImpFiles.NotEmpty()) {
		char   buf[MAXPATH], spc_fname[MAXPATH];
		SString path, fname;
		StringSet ss(';', acn.ImpFiles);
		THROW(PPGetFileName(PPFILNAM_OTCHET2_DBF, fname));
		STRNSCPY(spc_fname, "PETROVI4.DBF");
		for(uint i = 0; ss.get(&i, path) > 0;) {
			replacePath(STRNSCPY(buf, fname), path, 1);
			if(fileExists(buf))
				SFile::Remove(buf);
			replacePath(spc_fname, path, 1);
			if(fileExists(spc_fname)) {
				::rename(spc_fname, buf);
			}
			else {
				FILE * p_crf = 0;
				SFile::Remove(replacePath(strcpy(buf, "ERRKKM.DBF"), path, 1));
				SFile::Remove(replacePath(strcpy(buf, "OTCHET.EXP"), path, 1));
				p_crf = fopen(buf, "w");
				SFile::ZClose(&p_crf);
				replacePath(STRNSCPY(buf, fname), path, 1);
				PPWaitMsg(PPSTR_TEXT, PPTXT_WAITONDATAFROMCASH, buf);
				while(!fileExists(buf))
					if(!PPCheckUserBreak()) {
						ok = -1;
						break;
					}
			}
			if(ok > 0) {
				SessEntry entry;
				THROW_MEM(SETIFZ(P_SessList, new SArray(sizeof(SessEntry))));
				STRNSCPY(entry.InFilePath, buf);
				THROW_SL(P_SessList->insert(&entry));
			}
		}
	}
	else
		ok = -1;
	CATCH
		ZDELETE(P_SessList);
		ok = 0;
	ENDCATCH
	*pSessCount = P_SessList ? P_SessList->getCount() : 0;
	ASSIGN_PTR(pIsForwardSess, 1);
	return ok;
}

static void SLAPI WaitMsgByCheck(LDATE dt, long cashCode, long chkNumber)
{
	SString msg_buf;
	PPWaitMsg(msg_buf.Cat(dt, DATF_DMY).CatDiv(':', 1).Cat(cashCode).CatDiv(':', 1).Cat(chkNumber));
}

int SLAPI ACS_PETROVICH::ConvertWareList(int num)
{
	int    ok = 1, ta = 0;
	long   recno = 0;
	int    get_goods_by_barcode = 0;
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_PETROVI4BARCODE, &get_goods_by_barcode);
	PPObjGoods goods_obj;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	DbfTable * dbft = new DbfTable(((SessEntry *)P_SessList->at(num))->InFilePath);
	THROW_MEM(dbft);
	THROW(cn_obj.Search(NodeID, &cn_rec) > 0);
	THROW_PP(dbft->isOpened(), PPERR_DBFOPFAULT);
	THROW(PPStartTransaction(&ta, 1));
	if(dbft->top())
		do {
			char   lot_code[32], cash_code[32], chk_code[16], tm_str[16];
			char   op_code[8];
			int    division, r;
			int    h = 0, m = 0, s = 0, hs = 0;
			int    day, mon, year;
			PPID   goods_id = 0, chk_id = 0;
			long   csh, chk, delta;
			LDATE  beg_of_year;
			LDATETIME dm;
			double qtty, price, amount, discount, line_amount;
			TempCCheckLineTbl::Rec line_rec;
			DbfRecord dbfr(dbft);
			THROW(dbft->getRec(&dbfr));
			recno++;
			dbfr.get(1, lot_code);
			dbfr.get(2, cash_code);
			dbfr.get(4, chk_code);
			dbfr.get(5, dm.d);
			dbfr.get(6, tm_str);
			dbfr.get(7, op_code);
			if(get_goods_by_barcode) {
				char   barcode[32];
				dbfr.get(8, barcode);
				strip(barcode);
				Goods2Tbl::Rec goods_rec;
				BarcodeTbl::Rec bc_rec;
				if(goods_obj.SearchByBarcode(barcode, &bc_rec, &goods_rec, 0) > 0)
					goods_id = goods_rec.ID;
			}
			else {
				dbfr.get(8, goods_id);
				if(goods_id)
					goods_id -= GoodsIdBias;
			}
			dbfr.get(10, price);
			dbfr.get(11, qtty);
			dbfr.get(12, amount);
			dbfr.get(17, discount);
			dbfr.get(18, division);

			decodedate(&day, &mon, &year, &dm.d);
			if(op_code[0] == 'Z' && cn_rec.DrvVerMinor == 13) {
				char   temp_buf[32], temp_buf2[32];
				dbfr.get(5, temp_buf);
				char * p = temp_buf2;
				*p++ = temp_buf[0];
				*p++ = temp_buf[1];
				*p++ = temp_buf[6];
				*p++ = temp_buf[7];
				*p = 0;
				year = atoi(temp_buf2);

				p = temp_buf2;
				*p++ = temp_buf[2];
				*p++ = temp_buf[3];
				*p = 0;
				day = atoi(temp_buf2);

				p = temp_buf2;
				*p++ = temp_buf[4];
				*p++ = temp_buf[5];
				*p = 0;
				mon = atoi(temp_buf2);

				dm.d = encodedate(day, mon, year);
			}
			encodedate(1, 1, year, &beg_of_year);

			strtotime(tm_str, 0, &dm.t);
			decodetime(&h, &m, &s, &hs, &dm.t);

			delta = diffdate(&dm.d, &beg_of_year, 0) * 24 + h;

			strip(chk_code);
			strip(cash_code);
			chk = delta * 10000L + atol(chk_code);
			csh = atol(cash_code) % 10000L;
			//
			// Так как в поле времени чека, передаваемом драйвером PETROVICH нет
			// секунд, то искусственно установим их равным сумме цифр номера чека
			// и номера кассы по модулю 60.
			//
			{
				long   sec = 0;
				char * p;
				for(p = chk_code; *p; p++)
					sec += (*p - '0');
				for(p = cash_code; *p; p++)
					sec += (*p - '0');
				sec += (day*30 + mon);
				s = (int)(sec % 60);
				dm.t = encodetime(h, m, s, hs);
			}
			if(op_code[0] == '1' || op_code[0] == '2' || op_code[0] == '3') {
				qtty = (op_code[0] == '3') ? -fabs(qtty) : fabs(qtty);
				THROW(r = SearchTempCheckByCode(csh, chk));
				if(r > 0)
					chk_id = P_TmpCcTbl->data.ID;
				else {
					PPID   card_id = 0;
					WaitMsgByCheck(dm.d, csh, atol(chk_code));
					// @debug THROW(AddTempCheck(&chk_id, 0, 0, csh, chk, 0, card_id, &dm, 0, 0));
					int    r = AddTempCheck(&chk_id, 0, 0, csh, chk, 0, card_id, &dm, 0, 0);
					THROW(r);
				}
				if(chk_id) {
					MEMSZERO(line_rec);
					line_rec.CheckID   = chk_id;
					line_rec.CheckCode = chk;
					line_rec.Dt        = dm.d;
					line_rec.DivID     = 0L;
					line_rec.GoodsID   = goods_id;
					line_rec.Quantity  = qtty;
					line_amount = line_rec.GoodsID ? (price * qtty + discount) : amount;
					price = R2((qtty != 0) ? (line_amount / qtty) : line_amount);
					line_amount = R2(line_amount);
					line_rec.Price = dbltointmny(price);
					// @v5.2.7 line_rec.Discount = 0;
					line_rec.Dscnt = 0; // @v5.2.7
					P_TmpCclTbl->copyBufFrom(&line_rec);
					THROW_DB(P_TmpCclTbl->insertRec());
					THROW(AddTempCheckAmounts(chk_id, line_amount, -discount));
				}
			}
			else if(op_code[0] == 'Z') {
				// @debug THROW(AddTempCheck(&chk_id, atol(chk_code), CCHKF_ZCHECK, csh, chk, 0, 0, &dm, amount, 0));
				int r = AddTempCheck(&chk_id, atol(chk_code), CCHKF_ZCHECK, csh, chk, 0, 0, &dm, amount, 0);
				THROW(r);
			}
		} while(dbft->next());
	THROW(PPCommitWork(&ta));
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	delete dbft;
	return ok;
}

int SLAPI ACS_PETROVICH::ImportSession(int num)
{
	int    ok = 1;
	if(fileExists(((SessEntry *)P_SessList->at(num))->InFilePath)) {
		THROW(CreateTables());
		THROW(ConvertWareList(num));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI ACS_PETROVICH::UpdateSessions()
{
	int    ok = 1;
	PPAsyncCashNode acn;
	PPObjCashNode cn_obj;
	THROW(cn_obj.GetAsync(NodeID, &acn) > 0);
	if(acn.ExpPaths.NotEmpty()) {
		char   buf[MAXPATH];
		SString fname, path;
		StringSet ss(';', acn.ExpPaths);
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_TOVARS2_DBF, fname));
		for(uint i = 0; ss.get(&i, path) > 0;) {
			FILE * p_crf = 0;
			STRNSCPY(buf, fname);
			replacePath(buf, path, 1);
			if(fileExists(buf))
				SFile::Remove(buf);
			THROW_SL(copyFileByName(fname, buf));
			strcpy(buf, "ERRKKM.DBF");
			replacePath(buf, path, 1);
			SFile::Remove(buf);
			strcpy(buf, "TOVARS.EXP");
			replacePath(buf, path, 1);
			SFile::Remove(buf);
			p_crf = fopen(buf, onecstr('w'));
			SFile::ZClose(&p_crf);
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI ACS_PETROVICH::ExportData(int)
{
	int    ok = 1;
	DbfTable * p_out_tbl = 0;
	SString temp_buf, path;
	AsyncCashGoodsInfo info;
	AsyncCashGoodsIterator * p_iter = 0;
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_TOVARS2_DBF, path));
	THROW(p_out_tbl = CreateDbfTable(DBFS_PETROVICHEXPORT, path, 1));
	THROW_MEM(p_iter = new AsyncCashGoodsIterator(NodeID, 0, SinceDlsID, 0));
	while(p_iter->Next(&info) > 0) {
		DbfRecord dbfr(p_out_tbl);
		dbfr.empty();
		//dbfr.put(1, info.ID);
		(temp_buf = 0).Cat(info.ID+GoodsIdBias).Align(13, ALIGN_LEFT); // @v5.3.5
		// @v5.3.5 intfmt(info.ID+GoodsIdBias, MKSFMT(13, ALIGN_LEFT), temp);
		dbfr.put(2, temp_buf);
		dbfr.put(3, info.Name);
		dbfr.put(4, (double)(info.Price * info.UnitPerPack));
		dbfr.put(5, (int)1);    // Номер отдела
		if(info.UnitID == PPUNT_KILOGRAM)
			(temp_buf = GetGoodsCfg().WghtPrefix).CatLongZ(info.ID+GoodsIdBias, 5);
		else {
			AddCheckDigToBarcode(info.BarCode);
			(temp_buf = info.BarCode).Align(13, ALIGN_LEFT); // @v5.3.5
			// @v5.3.5 strfmt(info.BarCode, MKSFMT(13, ALIGN_LEFT), temp);
		}
		dbfr.put(6, temp_buf);
		dbfr.put(7, 1000L /*info.Rest*/); // Остаток
		THROW_PP(p_out_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
		PPWaitPercent(p_iter->GetIterCounter());
	}
	ZDELETE(p_out_tbl);
	THROW(UpdateSessions());
	PPWait(0);
	CATCHZOK
	delete p_out_tbl;
	return ok;
}

/*

int SLAPI ACS_PETROVICH::StoreCashOutput(char * pFileName, long maxCopies)
{
	int  ok = 1;
	char path[MAXPATH], temp_pat[MAXPATH];
	char drv[MAXDRIVE], dir[MAXDIR], fil[MAXFILE], ext[MAXEXT];
	char oldest_fname[MAXPATH];
	char new_fname[32];
	long fcount = 0, long last_count = 0;
	SDirec sdir;
	SDirEntry sdir_entry;
	LDATETIME oldest_time;

	STRNSCPY(path, pFileName);
	fnsplit(path, drv, dir, fil, ext);
	fnmerge(path, drv, dir, 0, 0);

	oldest_time.d = MAXDATE;
	oldest_time.t = MAXTIME;
	for(sdir.Init(path); sdir.Next(&sdir_entry) > 0;) {
		fnsplit(sdir_entry.FileName, 0, 0, fil, ext);
		if(fil[0] == '@' && fil[1] == '@' && atoi(ext+1) == 1) {
			last_count = MAX(atol(fil+2), last_count);
			fcount++;
			if(sdir_entry.WriteTime.d < oldest_time.d ||
				(sdir_entry.WriteTime.d == oldest_time.d &&
				sdir_entry.WriteTime.t < oldest_time.t)) {
				oldest_time = sdir_entry.WriteTime;
				STRNSCPY(oldest_fname, sditr_entry.FileName);
			}
		}
	}
	if(fcount >= maxCopies) {
		strcat(setLastSlash(strcpy(temp_path, path)), oldest_fname);
		SFile::Remove(temp_path);
	}
	sprintf(new_fname, "@@%06ld.001", last_count+1);
	strcat(setLastSlash(strcpy(temp_path, path)), new_fname);
	SCopyFile(pFileName, temp_path, 0, 0);
	return ok;
}

*/
