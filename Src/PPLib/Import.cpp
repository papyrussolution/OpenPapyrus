// IMPORT.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage windows-1251
//
// Функции импорта справочников
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <zlib.h>
/*
import.ini

[quot]
file=filename      ; * DBF file name for importing
quotname=string    ; * QuotKind Name for importing
goodsid=fld_name   ;   goods id
goodscode=fld_name ;   goods code
				   ; *(goodsid | goodscode)
goodsname=fld_name ; * goods name
price=fld_name     ; * price
cursymb=fld_name   ; currency symbol

[country]
file=filename
setcode={1|0}  ; Если значение !0, то при нахождении государства по имени и при
	; пустом поле Abbr это поле заполняется значением code.
name=fld_name
code=fld_name   ; Цифровой код государства ISO 3166
codea2=fld_name ; A2-код по стандарту ISO 3166

[scard]
file=filename
personcodereg = reg_symb ; Символ вида регистрационного документа, испольуземого в качестве кода владельца карты
seriesid     = scser_id ; Идентификатор серии, к которой должны относится импортируемые карты.
	; Если серия не определена, то система использует серию с именем DEFAULT. Если такая серия не существует,
	; то создает новую серию с таким именем
code         = fld_name ; Текстовый код карты
codenum      = fld_name ; Числовой код карты
	; поле codenum имеет приоритет перед code. То есть, если значения есть и в поле codenum и в поле code,
	; то система использует поле codenum и преобразует его в текст.
personcode   = fld_name ; Код владельца карты
pctdis       = fld_name ; Процент скидки по карте
serial       = fld_name ; Наименование серии, к которой относится карта.
turnover     = fld_name ; Начальный оборот по карте. Формируется фиктивными чеками на текущую операционную дату.

; Поля идентичны полям в зоне [person], но по умолчанию (если не определено) значение kind
; берется из группы ассоциаций таблицы GetSupplAccSheet()
; От туда же берется код клиента (personcodereg). Если personcodereg определен, то он
; имеет более высокий приоритет
[suppl]

[person]
file=filename       ; * DBF file name for importing
personcodereg=reg_symb ; Символ вида регистрационного документа, испольуземого в качестве кода персоналии
idbias=bias         ; bias added to key to avoid person id duplicating
codetohex={1|0}     ; Если !0, то код персоналии преобразуется к строке, каждый символ кода преобразуется //
	; в свое беззнаковое шестнадцатиричное представление (два символа). Буквы в этой строке находятся //
	; в верхнем регистре.
kind=person_kind_list; list of kind's names of persons (divisor ',')
status=fld_name     ; Символ юридического статуса
code=fld_name       ; unique code, identified person in enterprise domain
name=fld_name       ; * person name (imported into PersonTbl::Rec.Name)
city=fld_name       ; city name
zip=fld_name        ; ZIP-code
addr=fld_name       ; address
phone=fld_name      ; phone string
inn=fld_name        ; ИНН
okpo=fld_name       ; ОКПО
okonh=fld_name      ; ОКОНх
bankacc=fld_name    ; Номер банковского счета
bankname=fld_name   ; Наименование банка
bic=fld_name        ; БИК банка
corracc=fld_name    ; Корр счет банка
memo=fld_name       ; Примечание персоналии

[goodsgroup]
file=filename ; * DBF file name for importing
code=fld_name ; * group code (imported into Code (not ID))
name=fld_name ; * group name (imported into Name)

[goods]
file=filename     ; * DBF file name for importing

hierarchy=obj_code,parent_code ; Иерархический справочник: obj_code - код объекта, parent_code - код родительского объекта
code=fld_name             ; goods code (imported into Code (not ID))
addedcode=fld_name        ; Дополнительный код товара. Если существует запись с кодом (code), который
	;  уже есть в справочнике, но addedcode при этом уникальный, то в товар с кодом code добавляется //
	;  дополнительный код addedcode
addedcodeqtty=fld_name    ; Количество товара, ассоциированное с addedcode (по умолчанию - 1)
supplid=fld_name          ; supplier id (used for linking to suppliers)
personcodereg=reg_symb    ; Символ вида регистрационного документа, испольуземого в качестве кода поставщика
	; Если это поле не определено, но поле supplcode определено, то предпринимается попытка использовать
	; CodeRegTypeID из таблицы статей GetSupplAccSheet()
personcode=fld_name       ; Код поставщика
	; Поиск поставщика по supplcode имеет более высокий приоритет, чем supplid.
codetohex={1|0}           ; Если !0, то код поставщика преобразуется к строке, каждый символ кода преобразуется //
	; в свое беззнаковое шестнадцатиричное представление (два символа). Буквы в этой строке находятся //
	; в верхнем регистре.
billdate=fld_name         ; Дата поступления товара
billcode=fld_name         ; Номер документа прихода
	; Если определены поля lotdate, billcode, то документы прихода формируются в соответствии со значениями
	; этих полей. Группировка реализуется по сочетанию {Поставщик, Дата, Номер документа}
groupcode=fld_name        ; parent group code
groupname=fld_name        ; parent group name (used only if grpcode undefined)
name=fld_name             ; * goods name
unitname=fld_name         ; unit name
phperu=fld_name           ; physical units per unit

manufname=fld_name        ; Производитель
brand=fld_name            ; Торговая марка
ext_a=fld_name            ; Дополнительное поле A
ext_b=fld_name            ; Дополнительное поле B
ext_c=fld_name            ; Дополнительное поле C
ext_d=fld_name            ; Дополнительное поле D
ext_e=fld_name            ; Дополнительное поле E

packqtty=fld_name         ; quantity of trade units in one package
cost=fld_name             ; cost of one trade unit
price=fld_name            ; price of one trade unit
rest=fld_name             ; rest (trade units)

qcnmb=fld_name            ; quality cert name
qcblank=fld_name          ; quality cert blank code
qcdate=fld_name           ; quality cert issue date
qcexpiry=fld_name         ; quality cert expiry date
qcmanufname=fld_name      ; goods manufacturer name (as marked in quality cert)
qcorgname=fld_name        ; quality cert issue organization name
qcmanufdate=fld_name      ; goods produce date string (as marked in quality cert)
qcetc=fld_name            ; quality cert extra string (imported into QalityCert.Etc

salestaxrate=fld_name     ; rate of sales tax
vatrate=fld_name          ; rate of vat
manufcountryname=fld_name ; goods manufacturer country name
manufcountrycode=fld_name ; goods manufacturer country code
	; Если задан manufcountrycode, и поле manufcountry пустое, то
	; система ищет государство (не персоналию-государство, а именно государство) с аббревиатурой
	; manufcountrycode. Если такое государство существует, то создается персоналия-государство,
	; соответствующая найденному государству и ее идентификатор принимается в качестве
	; производителя товара
clb=fld_name              ; Custom Lading Bill number

*/
//
//
//
static int SLAPI get_fld_number(PPIniFile * pIniFile, DbfTable * pTbl, uint sectId, uint aliasId, int * pFldN)
{
	SString fld_name;
	return (pIniFile->Get(sectId, aliasId, fld_name) > 0) ? pTbl->getFieldNumber(fld_name, pFldN) : -1;
}
//
//
//
int SLAPI PPObjWorld::ImportCountry(int use_ta)
{
	int    ok = 1;
	int    setcode = 0;
	const  uint sect = PPINISECT_IMP_COUNTRY;
	SString file_name, code, name, code_a2;
	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	THROW_SL(fileExists(file_name));
	{
		PPIniFile ini_file(file_name);
		ini_file.Get(sect, PPINIPARAM_FILE, file_name);
		ini_file.GetInt(sect, PPINIPARAM_SETCODE, &setcode);
		THROW_SL(fileExists(file_name));
		{
			DbfTable in_tbl(file_name);
			int    fldn_code = 0;
			int    fldn_name = 0;
			int    fldn_codea2 = 0;
			THROW_PP(in_tbl.isOpened(), PPERR_DBFOPFAULT);
			get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODE, &fldn_code);
			get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_NAME, &fldn_name);
			get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODEA2, &fldn_codea2);
			THROW_PP(fldn_name, PPERR_IMPORTUNDEFFLD);
			{
				PPTransaction tra(use_ta);
				THROW(tra);
				if(in_tbl.top()) {
					do {
						if(in_tbl.isDeletedRec() <= 0) {
							int    r;
							PPID   id = 0;
							WorldTbl::Rec w_rec;
							DbfRecord rec(&in_tbl);
							THROW(in_tbl.getRec(&rec));
							rec.get(fldn_code, code);
							rec.get(fldn_name, name);
							rec.get(fldn_codea2, code_a2);
							code.Strip();
							name.Strip();
							code_a2.Strip();
							if(name.NotEmpty()) {
								r = SearchCountry(name, code, code_a2, &w_rec);
								if(r > 0) {
									int    upd = 0;
									if(setcode) {
										if(code.NotEmpty()) {
											code.CopyTo(w_rec.Code, sizeof(w_rec.Code));
											upd = 1;
										}
										if(code_a2.NotEmpty()) {
											code_a2.CopyTo(w_rec.Abbr, sizeof(w_rec.Abbr));
											upd = 1;
										}
									}
									if(upd) {
										PPWorldPacket pack;
										pack.Rec = w_rec;
										id = w_rec.ID;
										THROW(PutPacket(&id, &pack, 0));
									}
								}
								else {
									PPWorldPacket pack;
									pack.Rec.Kind = WORLDOBJ_COUNTRY;
									name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
									code.CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
									code_a2.CopyTo(pack.Rec.Abbr, sizeof(pack.Rec.Abbr));
									THROW(PutPacket(&id, &pack, 0));
								}
							}
						}
					} while(in_tbl.next());
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPObjSCard::Import(int use_ta)
{
	int    ok = 1;
	const  uint sect = PPINISECT_IMP_SCARD;
	SString file_name, wait_msg;
	IterCounter cntr;

	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	if(!fileExists(file_name))
		return -1;
	PPIniFile ini_file(file_name);
	ini_file.Get(sect, PPINIPARAM_FILE, file_name);

	//THROW(fileExists(file_name));

	DbfTable in_tbl(file_name);
	int fldn_code       = 0;
	int fldn_codenum    = 0;
	int fldn_personcode = 0;
	int fldn_pctdis     = 0;
	int fldn_serial     = 0; // Поле наименования серии карты
	int fldn_turnover   = 0; // Начальный оборот по карте

	PPIDArray psn_list;
	PPObjPerson psn_obj;
	PPObjSCardSeries scs_obj;

	PPWait(1);
	THROW_PP(in_tbl.isOpened(), PPERR_DBFOPFAULT);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODE,       &fldn_code);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODENUM,    &fldn_codenum);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PERSONCODE, &fldn_personcode);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PCTDIS,     &fldn_pctdis);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_SERIAL,     &fldn_serial);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_TURNOVER,   &fldn_turnover);
	THROW_PP(fldn_code || fldn_codenum, PPERR_IMPORTUNDEFFLD);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		PPLoadText(PPTXT_IMPSCARD, wait_msg);
		cntr.Init(in_tbl.getNumRecs());
		if(in_tbl.top()) {
			int    temp_id = 0;
			PPID   scser_id = 0, reg_type_id = 0;
			long   cc_code = 100000; // Код чека для внесения начальных оборотов по карте.
			LTIME  cc_time = ZEROTIME;
			SString temp_buf;
			PPSCardSeries scs_rec_;
			PPSCardSerPacket scs_pack;
			ini_file.GetInt(sect, PPINIPARAM_SERIESID, &temp_id);
			ini_file.Get(sect, PPINIPARAM_PERSONCODEREG, temp_buf);
			PPObjRegisterType::GetByCode(temp_buf.Strip(), &reg_type_id);
			if(temp_id && scs_obj.GetPacket(temp_id, &scs_pack) > 0) {
				scser_id = scs_pack.Rec.ID;
			}
			else if(scs_obj.SearchByName("default", &scser_id, &scs_rec_) > 0) {
				THROW(scs_obj.GetPacket(scs_rec_.ID, &scs_pack) > 0);
			}
			else {
				scs_pack.Init();
				STRNSCPY(scs_pack.Rec.Name, "default");
				scser_id = 0;
				THROW(scs_obj.PutPacket(&scser_id, &scs_pack, 0))
			}
			do {
				if(in_tbl.isDeletedRec() <= 0) {
					int64  codenum = 0;
					double pctdis = 0.0;
					SCardTbl::Rec sc_rec;
					DbfRecord rec(&in_tbl);
					THROW(in_tbl.getRec(&rec));
					// @v10.6.4 MEMSZERO(sc_rec);
					sc_rec.SeriesID = scser_id;
					rec.get(fldn_codenum, codenum);
					if(codenum) {
						_i64toa(codenum, sc_rec.Code, 10);
					}
					else {
						rec.get(fldn_code, temp_buf);
						if(temp_buf.NotEmptyS())
							STRNSCPY(sc_rec.Code, temp_buf);
					}
					if(sc_rec.Code[0]) {
						PPID   scard_id = 0;
						double turnover = 0.0;
						rec.get(fldn_turnover, turnover);
						if(P_Tbl->SearchCode(0, sc_rec.Code, 0) > 0) {
							scard_id = P_Tbl->data.ID;
						}
						else {
							if(rec.get(fldn_serial, temp_buf, 1)) {
								PPID   s_id = 0;
								if(scs_obj.SearchByName(temp_buf, &s_id, &scs_rec_) > 0) {
								}
								else {
									scs_pack.Init();
									STRNSCPY(scs_pack.Rec.Name, temp_buf);
									s_id = 0;
									THROW(scs_obj.PutPacket(&s_id, &scs_pack, 0))
								}
								if(s_id)
									sc_rec.SeriesID = s_id;
							}
							if(reg_type_id) {
								rec.get(fldn_personcode, temp_buf);
								if(temp_buf.NotEmptyS()) {
									psn_list.clear();
									int64  icode = temp_buf.ToInt64();
									if(icode) {
										temp_buf.Z().Cat(icode);
										if(psn_obj.GetListByRegNumber(reg_type_id, 0, temp_buf, psn_list) > 0)
											sc_rec.PersonID = psn_list.getSingle();
									}
								}
							}
							SCardTbl::Key0 k0;
							rec.get(fldn_pctdis, pctdis);
							sc_rec.PDis = fmul100i(pctdis); // @v10.2.9 @fix (long)(pctdis * 100L)-->fmul100i(pctdis)
							P_Tbl->copyBufFrom(&sc_rec);
							THROW_DB(P_Tbl->insertRec(0, &k0));
							scard_id = k0.ID;
						}
						if(scard_id && turnover > 0) {
							PPID cc_id = 0;
							CCheckTbl::Rec cc_rec;
							// @v10.6.4 MEMSZERO(cc_rec);
							cc_rec.Code = ++cc_code;
							encodedate(1, 1, 2001, &cc_rec.Dt);
							cc_time.v++;
							cc_rec.Tm = cc_time;
							cc_rec.SCardID = scard_id;
							LDBLTOMONEY(turnover, cc_rec.Amount);
							THROW(P_CcTbl->Add(&cc_id, &cc_rec, 0));
						}
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			} while(in_tbl.next());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}
//
//
//
struct CodeToPersonTabEntry {
	long   Code;
	long   PersonID;
};

static SArray * SLAPI ReadCodeToPersonTab()
{
	SArray * p_list = new SArray(sizeof(CodeToPersonTabEntry));
	SString file_name;
	PPGetFilePath(PPPATH_OUT, PPFILNAM_PSNKEYS_, file_name);
	FILE * rel_file = fopen(file_name, "rb");
	if(rel_file) {
		CodeToPersonTabEntry entry;
		while(fread(&entry, sizeof(entry), 1, rel_file) == 1)
			p_list->insert(&entry);
		SFile::ZClose(&rel_file);
	}
	return p_list;
}

static int SLAPI ConvertCodeToArticle(PPID accSheetID, SArray * pTab, long code, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	if(pTab) {
		PPID psn_id = 0;
		uint p = 0;
		if(pTab->lsearch(&code, &p, CMPF_LONG))
			psn_id = ((CodeToPersonTabEntry *)pTab->at(p))->PersonID;
		if(psn_id) {
			PPObjArticle ar_obj;
			ArticleTbl::Rec ar_rec;
			if(ar_obj.P_Tbl->SearchObjRef(accSheetID, psn_id, &ar_rec) > 0) {
				ar_id = ar_rec.ID;
				ok = 1;
			}
			else
				ok = ar_obj.CreateObjRef(&ar_id, accSheetID, psn_id, 0, 0);
		}
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int SLAPI PPObjGoodsGroup::Import(int use_ta)
{
	int    ok = 1;
	const  uint sect = PPINISECT_IMP_GOODSGROUP;
	SString file_name;
	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	if(fileExists(file_name)) {
		SString temp_buf;
		PPIniFile ini_file(file_name);
		ini_file.Get(sect, PPINIPARAM_FILE, file_name);

		//THROW(fileExists(file_name));

		PPGoodsConfig goods_cfg;
		DbfTable in_tbl(file_name);
		int    fldn_code = 0;
		int    fldn_name = 0;
		uint   num_par = 0;        // Количество уровней родительских
		int    fldn_par_name[32];
		int    fldn_par_code[32];
		ReadConfig(&goods_cfg);
		THROW_PP_S(in_tbl.isOpened(), PPERR_DBFOPFAULT, file_name);
		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODE, &fldn_code);
		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_NAME, &fldn_name);
		ini_file.Get(sect, PPINIPARAM_PARENTSEQ, temp_buf);
		if(temp_buf.NotEmptyS()) {
			SString par_code, par_name;
			StringSet ss(';', temp_buf);
			for(uint p = 0; ss.get(&p, temp_buf);) {
				temp_buf.Divide(',', par_name, par_code);
				in_tbl.getFieldNumber(par_name, &fldn_par_name[num_par]);
				in_tbl.getFieldNumber(par_code, &fldn_par_code[num_par]);
				if(fldn_par_name[num_par])
					num_par++;
			}
		}
		THROW_PP(fldn_code && fldn_name, PPERR_IMPORTUNDEFFLD);
		PPWaitMsg(PPSTR_TEXT, PPTXT_IMPGOODSGRP, 0);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(in_tbl.top()) {
				do {
					if(in_tbl.isDeletedRec() <= 0) {
						PPID   id = 0, parent_id = 0;
						int    uniq_cntr = 0;
						char   code[32], name[512], org_name[512];
						Goods2Tbl::Rec goods_rec;
						DbfRecord rec(&in_tbl);
						THROW(in_tbl.getRec(&rec));
						for(uint i = 0; i < num_par; i++) {
							rec.get(fldn_par_code[i], code);
							rec.get(fldn_par_name[i], name);
							strip(code);
							strip(name);
							STRNSCPY(org_name, name);
							uniq_cntr = 0;
							while(SearchByName(name, &(id = 0), &goods_rec) > 0 && PPObjGoods::GetRecKind(&goods_rec) != gpkndFolderGroup) {
								uniq_cntr++;
								sprintf(name, "%s #%i", org_name, uniq_cntr);
							}
							THROW(AddSimple(&id, gpkndFolderGroup, parent_id, name, code, 0, 0));
							parent_id = id;
						}
						rec.get(fldn_code, code);
						rec.get(fldn_name, name);
						strip(code);
						strip(name);
						STRNSCPY(org_name, name);
						uniq_cntr = 0;
						while(SearchByName(name, &(id = 0), &goods_rec) > 0 && PPObjGoods::GetRecKind(&goods_rec) != gpkndOrdinaryGroup) {
							uniq_cntr++;
							sprintf(name, "%s #%i", org_name, uniq_cntr);
						}
						THROW(AddSimple(&(id = 0), gpkndOrdinaryGroup, parent_id, name, code, goods_cfg.DefUnitID, 0));
					}
				} while(in_tbl.next());
			}
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::ImportQuotOld(int use_ta)
{
	int    ok = 1;
	SString file_name, quotname, temp_buf2;
	const  uint sect = PPINISECT_IMP_QUOT;
	IterCounter cntr;

	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	if(!fileExists(file_name))
		return -1;
	PPIniFile ini_file(file_name);
	ini_file.Get(sect, PPINIPARAM_FILE, file_name);

	//THROW(fileExists(file_name));

	PPGoodsConfig goods_cfg;
	DbfTable in_tbl(file_name);
	int    fldn_goodscode = 0;
	int    fldn_goodsname = 0;
	int    fldn_price     = 0;
	PPID   quot_kind_id = 0;
	PPID   loc_id = 0;

	ReadConfig(&goods_cfg);

	THROW_PP(in_tbl.isOpened(), PPERR_DBFOPFAULT);
	ini_file.Get(sect, PPINIPARAM_QUOTNAME, quotname);
	if(PPRef->SearchSymb(PPOBJ_QUOTKIND, &quot_kind_id, quotname, offsetof(PPQuotKind, Symb)) <= 0)
		THROW_PP(PPRef->SearchName(PPOBJ_QUOTKIND, &quot_kind_id, quotname, 0) > 0, PPERR_IMPORTUNDEFFLD);
	if(ini_file.Get(sect, PPINIPARAM_WAREHOUSE, temp_buf2) > 0) {
		PPObjLocation loc_obj;
		if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, temp_buf2, &loc_id, 0) <= 0)
			loc_id = 0;
	}
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_GOODSCODE, &fldn_goodscode);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_GOODSNAME, &fldn_goodsname);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PRICE,     &fldn_price);
	THROW_PP((fldn_goodscode || fldn_goodsname) && fldn_price, PPERR_IMPORTUNDEFFLD);

	PPWait(1);
	PPWaitMsg(PPSTR_TEXT, PPTXT_IMPGOODS, 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		cntr.Init(in_tbl.getNumRecs());
		if(in_tbl.top()) {
			do {
				if(in_tbl.isDeletedRec() <= 0) {
					PPID   goods_id = 0;
					char   code[256];
					double price = 0.0;
					BarcodeTbl::Rec barcode_rec;
					DbfRecord rec(&in_tbl);
					THROW(in_tbl.getRec(&rec));
					rec.get(fldn_goodscode, code);
					rec.get(fldn_goodsname, temp_buf2.Trim(63));
					rec.get(fldn_price, price);

					if(SearchByName(temp_buf2, &goods_id, 0) > 0) {
					}
					else if(code[0] && SearchByBarcode(code, &barcode_rec, 0, 0) > 0) {
						goods_id = barcode_rec.GoodsID;
					}
					if(goods_id && price > 0) {
						PPQuot quot(goods_id);
						quot.Kind = quot_kind_id;
						quot.LocID = loc_id;
						quot.Quot = price;
						THROW(P_Tbl->SetQuot(quot, 0));
					}
				}
				PPWaitPercent(cntr.Increment());
			} while(in_tbl.next());
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	CATCHZOK
	return ok;
}

struct ImportGoodsParam {
	enum {
		fSkipZeroQtty = 0x0001
	};
	PPID   DefParentID;
	PPID   RcptOpID;
	PPID   SupplID;
	PPID   DefUnitID;
	PPID   PhUnitID;
	long   Flags;
};

static int SLAPI RequestImportGoodsParam(ImportGoodsParam * pData)
{
	int    ok = -1, valid_data = 0;
	PPGoodsConfig goods_cfg;
	PPObjGoods::ReadConfig(&goods_cfg);
	SETIFZ(pData->RcptOpID, CConfig.ReceiptOp);
	SETIFZ(pData->DefUnitID, goods_cfg.DefUnitID);
	TDialog * dlg = new TDialog(DLG_IMPGOODS);
	if(CheckDialogPtrErr(&dlg)) {
		SetupPPObjCombo(dlg, CTLSEL_IMPGOODS_GRP, PPOBJ_GOODSGROUP, pData->DefParentID, OLW_CANINSERT);
		SetupPPObjCombo(dlg, CTLSEL_IMPGOODS_OP,  PPOBJ_OPRKIND,    pData->RcptOpID, 0, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
		SetupArCombo(dlg, CTLSEL_IMPGOODS_SUPPL, pData->SupplID, 0, GetSupplAccSheet(), sacfDisableIfZeroSheet);
		SetupPPObjCombo(dlg, CTLSEL_IMPGOODS_UNIT,   PPOBJ_UNIT, pData->DefUnitID, 0);
		SetupPPObjCombo(dlg, CTLSEL_IMPGOODS_PHUNIT, PPOBJ_UNIT, pData->PhUnitID,  0);
		dlg->AddClusterAssoc(CTL_IMPGOODS_FLAGS, 0, ImportGoodsParam::fSkipZeroQtty);
		dlg->SetClusterData(CTL_IMPGOODS_FLAGS, pData->Flags);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_IMPGOODS_GRP,    &pData->DefParentID);
			dlg->getCtrlData(CTLSEL_IMPGOODS_OP,     &pData->RcptOpID);
			dlg->getCtrlData(CTLSEL_IMPGOODS_SUPPL,  &pData->SupplID);
			dlg->getCtrlData(CTLSEL_IMPGOODS_UNIT,   &pData->DefUnitID);
			dlg->getCtrlData(CTLSEL_IMPGOODS_PHUNIT, &pData->PhUnitID);
			dlg->GetClusterData(CTL_IMPGOODS_FLAGS,  &pData->Flags);
			valid_data = ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

static int SLAPI GetHierarchyFields(PPIniFile * pIniFile, DbfTable * pTbl, uint sectId, uint aliasId, int * pCodeFldN, int * pParentFldN)
{
	int    ok = -1;
	SString fld_name, fld_name_code, fld_name_parent;
	if(pIniFile->Get(sectId, aliasId, fld_name) > 0)
		if(fld_name.Divide(',', fld_name_code, fld_name_parent) > 0) {
			int r1 = pTbl->getFieldNumber(fld_name_code.Strip(), pCodeFldN);
			int r2 = pTbl->getFieldNumber(fld_name_parent.Strip(), pParentFldN);
			if(r1 && r2)
				ok = 1;
		}
	return ok;
}

SLAPI HierArray::HierArray() : SVector(sizeof(HierArray::Item)) // @v9.9.3 SArray-->SVector
{
}

const HierArray::Item & SLAPI HierArray::at(uint i) const
{
	return *(HierArray::Item *)SVector::at(i); // @v9.9.3 SArray-->SVector
}

int SLAPI HierArray::Add(const char * pCode, const char * pParentCode)
{
	Item item;
	STRNSCPY(item.Code, pCode);
	STRNSCPY(item.ParentCode, pParentCode);
	return insert(&item) ? 1 : PPSetErrorSLib();
}

int SLAPI HierArray::SearchParentOf(const char * pCode, char * pBuf, size_t bufLen) const
{
	uint   pos = 0;
	if(lsearch(pCode, &pos, PTR_CMPFUNC(Pchar))) {
		strnzcpy(pBuf, at(pos).ParentCode,  bufLen);
		return 1;
	}
	return 0;
}

int SLAPI HierArray::IsThereChildOf(const char * pParentCode) const
{
	uint   pos = 0;
	return lsearch(pParentCode, &pos, PTR_CMPFUNC(Pchar), offsetof(HierArray::Item, ParentCode));
}

static int SLAPI LoadHierList(DbfTable * pTbl, int fldnCode, int fldnParent, HierArray * pList)
{
	int    ok = 1;
	SString code_buf, parent_code_buf;
	if(pTbl->top()) {
		do {
			if(pTbl->isDeletedRec() <= 0) {
				DbfRecord rec(pTbl);
				pTbl->getRec(&rec);
				rec.get(fldnCode, code_buf);
				rec.get(fldnParent, parent_code_buf);
				pList->Add(code_buf.Strip(), parent_code_buf.Strip());
			}
		} while(pTbl->next());
	}
	return ok;
}

int SLAPI PPObjGoods::Helper_ImportHier(PPIniFile * pIniFile, DbfTable * pTbl, PPID defUnitID, HierArray * pHierList)
{
	int    ok = 1, is_hier = 0;
	uint   i;
	PPIDArray id_list;
	PPObjGoodsGroup gg_obj;
	int    fldn_name  = 0;
	int    fldn_hier_objcode = 0;
	int    fldn_hier_parentcode = 0;
	get_fld_number(pIniFile, pTbl, PPINISECT_IMP_GOODS, PPINIPARAM_NAME, &fldn_name);
	if(GetHierarchyFields(pIniFile, pTbl, PPINISECT_IMP_GOODS, PPINIPARAM_HIERARCHY,
		&fldn_hier_objcode, &fldn_hier_parentcode) > 0)
		is_hier = 1;
	if(is_hier && pTbl->top()) {
		SString wait_msg;
		IterCounter cntr;
		cntr.Init(pTbl->getNumRecs());
		THROW(LoadHierList(pTbl, fldn_hier_objcode, fldn_hier_parentcode, pHierList));
		PPLoadText(PPTXT_IMPGOODSGRP, wait_msg);
		pTbl->top();
		do {
			if(pTbl->isDeletedRec() <= 0) {
				PPID   id;
				char   name[128], obj_code[128];
				DbfRecord rec(pTbl);
				THROW(pTbl->getRec(&rec));
				rec.get(fldn_hier_objcode, obj_code);
				strip(obj_code);
				if(pHierList->IsThereChildOf(obj_code)) {
					rec.get(fldn_name, name);
					THROW(gg_obj.AddSimple(&id, gpkndOrdinaryGroup, 0, name, obj_code, defUnitID, 0));
					THROW(id_list.addUnique(id));
				}
			}
			PPWaitPercent(cntr.Increment(), wait_msg);
		} while(pTbl->next());
	}
	for(i = 0; i < id_list.getCount(); i++) {
		char parent_code[128];
		Goods2Tbl::Rec gg_rec;
		if(gg_obj.Search(id_list.at(i), &gg_rec) > 0) {
			BarcodeArray bc_list;
			THROW(gg_obj.ReadBarcodes(id_list.at(i), bc_list));
			if(bc_list.getCount()) {
				if(pHierList->SearchParentOf(bc_list.at(0).Code+1, parent_code, sizeof(parent_code))) {
					BarcodeTbl::Rec barcode_rec;
					if(gg_obj.SearchCode(parent_code, &barcode_rec) > 0) {
						Goods2Tbl::Rec parent_rec;
						if(gg_obj.Search(barcode_rec.GoodsID, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP) {
							PPID temp_id = 0;
							if(!(parent_rec.Flags & GF_FOLDER)) {
								temp_id = parent_rec.ID;
								parent_rec.Flags |= GF_FOLDER;
								THROW(gg_obj.P_Tbl->Update(&temp_id, &parent_rec, 0));
							}
							gg_rec.ParentID = barcode_rec.GoodsID;
							temp_id = id_list.at(i);
							THROW(gg_obj.P_Tbl->Update(&temp_id, &gg_rec, 0));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static SString & SLAPI CodeToHex(SString & rCode)
{
	SString temp_buf;
	for(size_t i = 0; i < rCode.Len(); i++) {
		uint8 c = (uint8)rCode.C(i);
		uint8 lo = (c >> 4);
		uint8 hi = (c & 0x0f);
		temp_buf.CatChar(lo + ((lo > 9) ? ('A'-10) : '0'));
		temp_buf.CatChar(hi + ((hi > 9) ? ('A'-10) : '0'));
	}
	rCode = temp_buf;
	return rCode;
}

SLAPI GoodsImportBillIdent::GoodsImportBillIdent(PPObjPerson * pPsnObj, PPID defSupplID)
{
	THISZERO();
	RegTypeID = -1;
	P_PsnObj = pPsnObj;
	AccSheetID = GetSupplAccSheet();
}

SLAPI GoodsImportBillIdent::~GoodsImportBillIdent()
{
	delete P_ArObj;
	ZDELETE(P_PackList);
	delete P_CodeToPersonTab; // @v10.3.0 @fix
}

PPBillPacket * SLAPI GoodsImportBillIdent::GetPacket(PPID opID, PPID locID)
{
	PPBillPacket * p_pack = 0;
	if(P_PackList) {
		for(uint i = 0; i < P_PackList->getCount(); i++) {
			PPBillPacket * p_temp_pack = P_PackList->at(i);
			if(p_temp_pack->Rec.Dt == BillDate && p_temp_pack->Rec.Object == SupplID &&
				stricmp(p_temp_pack->Rec.Code, BillCode) == 0) {
				if(p_temp_pack->CheckLargeBill(0)) {
					THROW(BillObj->__TurnPacket(p_temp_pack, 0, 1, 0));
					THROW(p_temp_pack->CreateBlank(opID, 0, locID, 0));
					if(locID)
						p_temp_pack->Rec.LocID = locID;
					p_temp_pack->Rec.Object = SupplID;
					p_temp_pack->Rec.Dt = BillDate;
					STRNSCPY(p_temp_pack->Rec.Code, BillCode);
				}
				p_pack = p_temp_pack;
				break;
			}
		}
	}
	if(p_pack == 0) {
		if(P_PackList == 0)
			THROW_MEM(P_PackList = new TSCollection <PPBillPacket>);
		THROW_MEM(p_pack = new PPBillPacket);
		THROW(p_pack->CreateBlank(opID, 0, locID, 0));
		if(locID)
			p_pack->Rec.LocID = locID;
		p_pack->Rec.Object = SupplID;
		p_pack->Rec.Dt = BillDate;
		STRNSCPY(p_pack->Rec.Code, BillCode);
		P_PackList->insert(p_pack);
	}
	CATCH
		p_pack = 0;
	ENDCATCH
	return p_pack;
}

int  SLAPI GoodsImportBillIdent::FinishPackets()
{
	int    ok = 1;
	if(P_PackList) {
		SString msg_buf;
		PPLoadText(PPTXT_TURNBILLPACKETS, msg_buf);
		for(uint i = 0; i < P_PackList->getCount(); i++) {
			PPBillPacket * p_pack = P_PackList->at(i);
			THROW(BillObj->__TurnPacket(p_pack, 0, 1, 0));
			PPWaitPercent(i+1, P_PackList->getCount(), msg_buf);
		}
	}
	CATCHZOK
	return ok;
}

void SLAPI GoodsImportBillIdent::GetFldSet(PPIniFile * pIniFile, uint sect, DbfTable * pTbl)
{
	get_fld_number(pIniFile, pTbl, sect, PPINIPARAM_SUPPLID,       &fldn_suppl);
	get_fld_number(pIniFile, pTbl, sect, PPINIPARAM_PERSONCODE,    &fldn_supplcode);
	get_fld_number(pIniFile, pTbl, sect, PPINIPARAM_BILLCODE,      &fldn_billcode);
	get_fld_number(pIniFile, pTbl, sect, PPINIPARAM_BILLDATE,      &fldn_billdate);
	pIniFile->Get(sect, PPINIPARAM_PERSONCODEREG, SupplCodeRegSymb);
	SupplCodeRegSymb.Strip();
	pIniFile->GetInt(sect, PPINIPARAM_CODETOHEX, &CvtCodeToHex);
	P_CodeToPersonTab = ReadCodeToPersonTab();
}

int SLAPI GoodsImportBillIdent::Get(Sdr_Goods2 * pRec, PPID supplID)
{
	int    ok = 1;
	SString suppl_code;
	SupplID = 0;
	BillDate = ZERODATE;
	BillCode.Z();
	/*
	if(*strip(pRec->BillSupplCode) && AccSheetID) {
		if(CvtCodeToHex)
			CodeToHex(suppl_code = pRec->BillSupplCode);
		if(RegTypeID < 0)
			PPObjArticle::GetSearchingRegTypeID(AccSheetID, SupplCodeRegSymb, 0, &RegTypeID);
		if(RegTypeID > 0) {
			PPIDArray id_list, ar_id_list;
			P_PsnObj->GetListByRegNumber(RegTypeID, 0, pRec->BillSupplCode, id_list);
			if(id_list.getCount()) {
				SETIFZ(P_ArObj, new PPObjArticle);
				P_ArObj->GetByPersonList(AccSheetID, &id_list, &ar_id_list);
				if(ar_id_list.getCount())
					SupplID = ar_id_list.at(0);
			}
		}

	}
	if(!SupplID && pRec->BillSupplID) {
		THROW(ConvertCodeToArticle(AccSheetID, P_CodeToPersonTab, pRec->BillSupplID, &SupplID));
	}
	SETIFZ(SupplID, DefSupplID);
	*/
	if(*strip(pRec->BillCode) == 0)
		BillCode = "@INIT";
	BillDate = pRec->BillDate;
	SETIFZ(BillDate, LConfig.OperDate);
	SupplID = NZOR(supplID, DefSupplID);
	/*
	CATCHZOK
	*/
	return ok;
}

int SLAPI GoodsImportBillIdent::Get(DbfRecord * pRec)
{
	int    ok = 1;
	SString suppl_code;
	SupplID = 0;
	BillDate = ZERODATE;
	BillCode = 0;
	if(pRec->get(fldn_supplcode, suppl_code, 1) && AccSheetID) {
		if(CvtCodeToHex)
			CodeToHex(suppl_code);
		if(RegTypeID < 0)
			PPObjArticle::GetSearchingRegTypeID(AccSheetID, SupplCodeRegSymb, 0, &RegTypeID);
		if(RegTypeID > 0) {
			PPIDArray id_list, ar_id_list;
			P_PsnObj->GetListByRegNumber(RegTypeID, 0, suppl_code, id_list);
			if(id_list.getCount()) {
				SETIFZ(P_ArObj, new PPObjArticle);
				P_ArObj->GetByPersonList(AccSheetID, &id_list, &ar_id_list);
				if(ar_id_list.getCount())
					SupplID = ar_id_list.at(0);
			}
		}

	}
	if(!SupplID && fldn_suppl) {
		long   code = 0;
		pRec->get(fldn_suppl, code);
		THROW(ConvertCodeToArticle(AccSheetID, P_CodeToPersonTab, code, &SupplID));
	}
	SETIFZ(SupplID, DefSupplID);
	if(!pRec->get(fldn_billcode, BillCode, 1))
		BillCode = "@INIT";
	pRec->get(fldn_billdate, BillDate);
	SETIFZ(BillDate, LConfig.OperDate);
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoods::ImportOld(int use_ta)
{
	int    ok = 1;
	IterCounter cntr;
	SString file_name, wait_msg, temp_buf2;
	SString err_msg_buf, fmt_buf, msg_buf;
	const  uint sect = PPINISECT_IMP_GOODS;
	ImportGoodsParam igp;
	PPObjGoodsGroup gg_obj;
	PPObjBrand      brand_obj;
	PPObjQCert      qc_obj;
	PPObjPerson     psn_obj;
	PPObjWorld      w_obj;
	PPObjUnit       unit_obj;
	BarcodeTbl::Rec barcode_rec;
	HierArray hier_list;
	PPLogger logger;

	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	if(!fileExists(file_name))
		return -1;
	PPIniFile ini_file(file_name);
	ini_file.Get(sect, PPINIPARAM_FILE, file_name);
	DbfTable in_tbl(file_name);
	int    fldn_code  = 0;
	int    fldn_addedcode = 0;
	int    fldn_addedcodeqtty = 0;
	int    fldn_grpcode  = 0;
	int    fldn_grpname  = 0;
	int    fldn_grp2name = 0;
	int    fldn_name  = 0;
	int    fldn_unitname = 0;
	int    fldn_phperu  = 0;
	int    fldn_unitsperpack = 0;
	int    fldn_price = 0;
	int    fldn_cost = 0;
	int    fldn_rest = 0;
	int    fldn_qc_number = 0;
	int    fldn_qc_blank  = 0;
	int    fldn_qc_date = 0;
	int    fldn_qc_expiry = 0;
	int    fldn_qc_manuf = 0;
	int    fldn_qc_org = 0;
	int    fldn_qc_manufdate = 0;
	int    fldn_qc_etc = 0;
	int    fldn_salestax = 0;
	int    fldn_vat = 0;
	int    fldn_country = 0;
	int    fldn_countrycode = 0;
	int    fldn_clb = 0;
	int    fldn_serial = 0;
	int    fldn_expiry = 0;
	int    fldn_manufname = 0;
	int    fldn_brand = 0;
	int    fldn_ext_a = 0;
	int    fldn_ext_b = 0;
	int    fldn_ext_c = 0;
	int    fldn_ext_d = 0;
	int    fldn_ext_e = 0;
	int    fldn_article = 0;
	int    fldn_hier_objcode = 0;
	int    fldn_hier_parentcode = 0;
	int    is_hier = 0;
	int    codetohex = 0;
	PPID   warehouse_id = 0;
	int    matrix_action = -1;
	LAssoc subcode;
	PPLoadText(PPTXT_IMPGOODS, wait_msg);
	THROW_PP(in_tbl.isOpened(), PPERR_DBFOPFAULT);
	if(GetHierarchyFields(&ini_file, &in_tbl, sect, PPINIPARAM_HIERARCHY, &fldn_hier_objcode, &fldn_hier_parentcode) > 0)
		is_hier = 1;
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODE,      &fldn_code);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_ADDEDCODE, &fldn_addedcode);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_ADDEDCODEQTTY, &fldn_addedcodeqtty);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_GROUPCODE,  &fldn_grpcode);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_GROUPNAME,  &fldn_grpname);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_GROUP2NAME, &fldn_grp2name);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_NAME,     &fldn_name);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_UNITNAME, &fldn_unitname);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_IMPGOODS_PHPERU, &fldn_phperu);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PACKQTTY, &fldn_unitsperpack);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_COST,     &fldn_cost);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PRICE,    &fldn_price);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_REST,     &fldn_rest);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_MANUFNAME, &fldn_manufname);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_BRAND,     &fldn_brand);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXT_A,     &fldn_ext_a);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXT_B,     &fldn_ext_b);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXT_C,     &fldn_ext_c);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXT_D,     &fldn_ext_d);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXT_E,     &fldn_ext_e);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCNMB,    &fldn_qc_number);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCBLANK,  &fldn_qc_blank);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCDATE,   &fldn_qc_date);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCEXPIRY,    &fldn_qc_expiry);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCMANUFNAME, &fldn_qc_manuf);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCORGNAME,   &fldn_qc_org);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCMANUFDATE, &fldn_qc_manufdate);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_QCETC,       &fldn_qc_etc);

	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_SALESTAXRATE, &fldn_salestax);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_VATRATE,      &fldn_vat);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_MANUFCOUNTRYNAME, &fldn_country);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_MANUFCOUNTRYCODE, &fldn_countrycode);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CLB,          &fldn_clb);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_SERIAL,       &fldn_serial);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXPIRY,       &fldn_expiry);

	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_ARTICLE,      &fldn_article);

	THROW_PP(fldn_name /*&& (fldn_grpcode || fldn_grpname)*/, PPERR_IMPORTUNDEFFLD);
	{
		if(ini_file.Get(sect, PPINIPARAM_WAREHOUSE, temp_buf2) > 0) {
			PPObjLocation loc_obj;
			if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, temp_buf2, &warehouse_id, 0) <= 0)
				warehouse_id = 0;
		}
		if(ini_file.GetInt(sect, PPINIPARAM_MATRIX, &matrix_action) <= 0)
			matrix_action = -1;
		if(!GetConfig().MtxQkID) {
			THROW_PP(matrix_action < 1000, PPERR_UNDEFGOODSMATRIX);
			matrix_action = -1;
		}
	}
	if(ini_file.Get(sect, PPINIPARAM_SUBCODE, temp_buf2) > 0) {
		SString o, s;
		temp_buf2.Divide(',', o, s);
		subcode.Key = o.ToLong();
		subcode.Val = s.ToLong();
		if((subcode.Key < 0 || subcode.Key > 32) || (subcode.Val < 1 || subcode.Val > 32))
			subcode.Key = subcode.Val = 0; // @error
	}
	MEMSZERO(igp);
	if(RequestImportGoodsParam(&igp) > 0) {
		PPWait(1);
		PPWaitMsg(wait_msg);
		GoodsImportBillIdent bill_ident(&psn_obj, igp.SupplID);
		bill_ident.GetFldSet(&ini_file, sect, &in_tbl);
		PPTransaction tra(use_ta);
		THROW(tra);
		if(is_hier)
			THROW(Helper_ImportHier(&ini_file, &in_tbl, igp.DefUnitID, &hier_list));
		PPLoadText(PPTXT_IMPGOODS, wait_msg);
		cntr.Init(in_tbl.getNumRecs());
		if(in_tbl.top()) {
			do {
				if(in_tbl.isDeletedRec() <= 0) {
					int    skip = 0;
					int    is_found = 0;
					PPID   goods_id = 0, parent_id = 0;
					char   added_code[32];
					char   temp_buf[256];
					char   goods_name[128];
					char   barcode[32];
					char   subc[32];
					SString obj_code, parent_code, ar_code;
					double added_code_qtty = 1.0, rest = 0.0;
					PPGoodsPacket  pack;

					added_code[0] = 0;
					DbfRecord rec(&in_tbl);
					THROW(in_tbl.getRec(&rec));
					rec.get(fldn_rest, rest);
					rec.get(fldn_name, temp_buf);
					if(is_hier) {
						rec.get(fldn_hier_objcode, obj_code);
						rec.get(fldn_hier_parentcode, parent_code);
						if(hier_list.IsThereChildOf(obj_code) || gg_obj.SearchCode(obj_code, &barcode_rec) > 0)
							skip = 1;
					}
					if(!skip && *strip(temp_buf)) {
						STRNSCPY(goods_name, temp_buf);
						memzero(barcode, sizeof(barcode));
						added_code[0] = 0;
						subc[0] = 0;
						if(rec.get(fldn_code, temp_buf, 1)) {
							STRNSCPY(barcode, strupr(temp_buf));
							if(subcode.Key >= 0 && subcode.Val > 1) {
								size_t bclen = sstrlen(barcode);
								if((size_t)subcode.Key < bclen) {
									bclen = MIN((size_t)subcode.Val, bclen-subcode.Key);
									memcpy(subc, barcode+subcode.Key, bclen);
									subc[bclen] = 0;
								}
							}
						}
						if(rec.get(fldn_addedcode, temp_buf, 1))
							STRNSCPY(added_code, strupr(temp_buf));
						if(rec.get(fldn_addedcodeqtty, added_code_qtty)) {
							if(added_code_qtty <= 0.0)
								added_code_qtty = 1.0;
						}
						rec.get(fldn_article, ar_code, 1);
						if(SearchByName(goods_name, &goods_id, 0) > 0) {
							if(added_code[0] && SearchByBarcode(added_code, &barcode_rec, 0, 0) > 0)
								added_code[0] = 0;
							is_found = 1;
						}
						else if(barcode[0] && SearchByBarcode(barcode, &barcode_rec, 0, 0) > 0) {
							goods_id = barcode_rec.GoodsID;
							if(added_code[0] && SearchByBarcode(added_code, &barcode_rec, 0, 0) > 0)
								added_code[0] = 0;
							is_found = 1;
						}
						else if(subc[0] && SearchByBarcode(subc, &barcode_rec, 0, 0) > 0) {
							goods_id = barcode_rec.GoodsID;
							if(added_code[0] && SearchByBarcode(added_code, &barcode_rec, 0, 0) > 0)
								added_code[0] = 0;
							is_found = 1;
						}
						if(is_found) {
							int do_update = 0;
							if(subc[0] || ar_code.NotEmpty()) {
								THROW(GetPacket(goods_id, &pack, 0) > 0);
								if(subc[0] && pack.Codes.Replace(subc, barcode) > 0)
									do_update = 1;
								if(ar_code.NotEmpty()) {
									ArGoodsCodeTbl::Rec ar_code_rec;
									// @v10.6.4 MEMSZERO(ar_code_rec);
									ar_code_rec.GoodsID = pack.Rec.ID;
									ar_code_rec.ArID = 0; // ?
									ar_code_rec.Pack = 1000; // 1.0
									ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
									pack.ArCodes.insert(&ar_code_rec);
									do_update = 1;
								}
							}
							if(do_update) {
								if(!PutPacket(&goods_id, &pack, 0)) {
									PPGetLastErrorMessage(1, err_msg_buf);
									PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
									msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack.Rec.Name, err_msg_buf.cptr());
									logger.Log(msg_buf);
								}
							}
						}
						else if(matrix_action < 1000) {
							if(is_hier || fldn_grpcode) {
								if(is_hier)
									STRNSCPY(temp_buf, parent_code);
								else
									rec.get(fldn_grpcode, temp_buf);
								if(*strip(temp_buf) && gg_obj.SearchCode(temp_buf, &barcode_rec) > 0)
									parent_id = barcode_rec.GoodsID;
							}
							else if(rec.get(fldn_grpname, temp_buf, 1)) {
								PPID   folder_id = 0;
								if(rec.get(fldn_grp2name, temp_buf2, 1))
									THROW(gg_obj.AddSimple(&folder_id, gpkndFolderGroup, 0, temp_buf2, 0, 0, 0));
								THROW(gg_obj.AddSimple(&parent_id, gpkndOrdinaryGroup, folder_id, temp_buf, 0, igp.DefUnitID, 0));
							}
							if(!parent_id) {
								if(igp.DefParentID == 0)
									THROW(gg_obj.AddSimple(&igp.DefParentID, gpkndOrdinaryGroup, 0, "0", 0, igp.DefUnitID, 0));
								parent_id = igp.DefParentID;
							}
							THROW(InitPacket(&pack, gpkndGoods, parent_id, 0, barcode));
							STRNSCPY(pack.Rec.Name, goods_name);
							STRNSCPY(pack.Rec.Abbr, goods_name);
							if(rec.get(fldn_unitname, temp_buf, 1)) {
								THROW(unit_obj.AddSimple(&pack.Rec.UnitID, temp_buf, 0, 0));
							}
							else
								pack.Rec.UnitID = igp.DefUnitID;
							{ // Журавлев
								if(fldn_phperu) {
									char   phperu_buf[48], val_buf[48];
									rec.get(fldn_phperu, phperu_buf, 1);
									char * p = phperu_buf;
									char * p_v = val_buf;
									while((*p >= '0' && *p <= '9') || *p == '.' || *p == ',') {
										if(*p == ',')
											*p = '.';
										*p_v++ = *p++;
									}
									*p_v = 0;
									double phperu = satof(val_buf); // @v10.7.9 atof-->satof
									while(*p == ' ')
										p++;
									if(*p == '*') {
										p++;
										while(*p == ' ')
											p++;
										p_v = val_buf;
										while((*p >= '0' && *p <= '9') || *p == '.' || *p == ',') {
											if(*p == ',')
												*p = '.';
											*p_v++ = *p++;
										}
										*p_v = 0;
										phperu *= satof(val_buf); // @v10.7.9 atof-->satof
									}
									size_t len = sstrlen(STRNSCPY(val_buf, strip(p)));
									if(val_buf[len] == '.') {
										val_buf[len] = 0;
										strip(val_buf);
									}
									if(val_buf[0]) {
										if(stricmp1251(val_buf, "г") == 0 || stricmp1251(val_buf, "гр") == 0) {
											PPLoadString("munit_kg", temp_buf2);
											STRNSCPY(val_buf, temp_buf2);
											phperu /= 1000L;
										}
										else if(stricmp1251(val_buf, "л") == 0) {
											PPLoadString("munit_liter", temp_buf2);
											STRNSCPY(val_buf, temp_buf2);
										}
										else if(stricmp1251(val_buf, "мл") == 0) {
											PPLoadString("munit_liter", temp_buf2);
											STRNSCPY(val_buf, temp_buf2);
											phperu /= 1000L;
										}
										THROW(unit_obj.AddSimple(&pack.Rec.PhUnitID, val_buf, PPUnit::Trade|PPUnit::Physical, 0));
									}
									else
										pack.Rec.PhUnitID = igp.PhUnitID;
									pack.Rec.PhUPerU = phperu;
								}
							}
							rec.get(fldn_country, temp_buf);
							if(*strip(temp_buf)) {
								THROW(psn_obj.AddSimple(&pack.Rec.ManufID, temp_buf, PPPRK_MANUF, PPPRS_COUNTRY, 0));
							}
							else if(rec.get(fldn_countrycode, temp_buf, 1)) {
								PPID   cntry_id = 0;
								WorldTbl::Rec w_rec;
								if(w_obj.SearchByName(WORLDOBJ_COUNTRY, temp_buf, &w_rec) > 0)
									THROW(psn_obj.AddSimple(&pack.Rec.ManufID, w_rec.Name, PPPRK_MANUF, PPPRS_COUNTRY, 0));
							}
							else if(rec.get(fldn_manufname, temp_buf, 1)) {
								THROW(psn_obj.AddSimple(&pack.Rec.ManufID, temp_buf, PPPRK_MANUF, PPPRS_LEGAL, 0));
							}
							if(rec.get(fldn_brand, temp_buf, 1))
								THROW(brand_obj.AddSimple(&pack.Rec.BrandID, temp_buf, 0, 0));
							if(rec.get(fldn_ext_a, temp_buf, 1)) pack.PutExtStrData(GDSEXSTR_A, temp_buf);
							if(rec.get(fldn_ext_b, temp_buf, 1)) pack.PutExtStrData(GDSEXSTR_B, temp_buf);
							if(rec.get(fldn_ext_c, temp_buf, 1)) pack.PutExtStrData(GDSEXSTR_C, temp_buf);
							if(rec.get(fldn_ext_d, temp_buf, 1)) pack.PutExtStrData(GDSEXSTR_D, temp_buf);
							if(rec.get(fldn_ext_e, temp_buf, 1)) pack.PutExtStrData(GDSEXSTR_E, temp_buf);
							if(fldn_salestax || fldn_vat) {
								PPID   tax_grp_id = 0;
								double vat_rate = 0.0, stax_rate = 0.0;
								rec.get(fldn_salestax, stax_rate);
								rec.get(fldn_vat, vat_rate);
								/*gt_obj*/GTxObj.GetByScheme(&tax_grp_id, vat_rate, 0, stax_rate, 0, 0/*use_ta*/);
								pack.Rec.TaxGrpID = tax_grp_id;
							}
							if(PutPacket(&goods_id, &pack, 0)) {
								//logger.LogAcceptMsg(PPOBJ_GOODS, goods_id, 0);
								if(ar_code.NotEmpty()) {
									//
									// Артикулы товара меняем уже после проведения товара
									// для того, чтобы вероятное дублировании артикулов
									// не помешало бы нам сохранить весь пакет товара в целом.
									//
									ArGoodsCodeArray arcode_list;
									ArGoodsCodeTbl::Rec ar_code_rec;
									arcode_list = pack.ArCodes;
									// @v10.6.4 MEMSZERO(ar_code_rec);
									ar_code_rec.GoodsID = goods_id;
									ar_code_rec.ArID = 0; // ?
									ar_code_rec.Pack = 1000; // 1.0
									ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
									arcode_list.insert(&ar_code_rec);
									if(!P_Tbl->UpdateArCodes(goods_id, &arcode_list, 0)) {
										PPGetLastErrorMessage(1, err_msg_buf);
										logger.Log(err_msg_buf);
									}
								}
							}
							else {
								int ok_2 = 0;
								if(PPErrCode == PPERR_INVGOODSPARENT && igp.DefParentID) {
									pack.Rec.ParentID = igp.DefParentID;
									if(PutPacket(&goods_id, &pack, 0))
										ok_2 = 1;
								}
								if(!ok_2) {
									PPGetLastErrorMessage(1, err_msg_buf);
									PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
									msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack.Rec.Name, err_msg_buf.cptr());
									logger.Log(msg_buf);
								}
							}
						}
					}
					if(goods_id) {
						PPID   qcert_id = 0;
						if(added_code[0] && P_Tbl->SearchBarcode(added_code, 0) < 0)
							THROW(P_Tbl->AddBarcode(goods_id, added_code, added_code_qtty, 0));
						rec.get(fldn_qc_number, temp_buf);
						if(*strip(temp_buf) && qc_obj.SearchByCode(temp_buf, &qcert_id, 0) <= 0) {
							QualityCertTbl::Rec qc_rec;
							// @v10.6.4 MEMSZERO(qc_rec);
							STRNSCPY(qc_rec.Code, temp_buf);
							rec.get(fldn_qc_blank, temp_buf);
							STRNSCPY(qc_rec.BlankCode, strip(temp_buf));
							rec.get(fldn_qc_date, qc_rec.InitDate);
							rec.get(fldn_qc_expiry, qc_rec.Expiry);
							rec.get(fldn_qc_manuf, temp_buf);
							// Журавлев STRNSCPY(qc_rec.Manuf, strip(temp_buf));
							STRNSCPY(qc_rec.GoodsName, strip(temp_buf)); // Журавлев
							// Журавлев {
							{
								int fld_no = 0;
								rec.getFieldNumber("INDSER", &fld_no);
								rec.get(fld_no, qc_rec.InnerCode, 1);
							}
							// } Журавлев
							rec.get(fldn_qc_etc, temp_buf);
							STRNSCPY(qc_rec.Etc, strip(temp_buf));
							rec.get(fldn_qc_manufdate, temp_buf);
							STRNSCPY(qc_rec.SPrDate, strip(temp_buf));
							if(rec.get(fldn_qc_org, temp_buf, 1))
								THROW(psn_obj.AddSimple(&qc_rec.RegOrgan, temp_buf, PPPRK_BUSADMIN, PPPRS_LEGAL, 0));
							THROW(qc_obj.PutPacket(&(qcert_id = 0), &qc_rec, 0));
						}
						//
						//
						//
						if(igp.RcptOpID && igp.SupplID) {
							PPBillPacket * p_pack = 0;
							PPTransferItem ti;
							THROW(bill_ident.Get(&rec));
							THROW(p_pack = bill_ident.GetPacket(igp.RcptOpID, warehouse_id));
							ti.Init(&p_pack->Rec);
							THROW(ti.SetupGoods(goods_id));
							rec.get(fldn_unitsperpack, ti.UnitPerPack);
							rec.get(fldn_rest, ti.Quantity_);
							ti.Quantity_ = R6(fabs(ti.Quantity_));
							rec.get(fldn_cost, ti.Cost);
							ti.Cost  = R2(fabs(ti.Cost));
							rec.get(fldn_price, ti.Price);
							ti.Price = R2(fabs(ti.Price));
							ti.QCert = qcert_id;
							rec.get(fldn_expiry, ti.Expiry);
							if(ti.QCert || ti.Price || ti.Rest_ || ti.UnitPerPack) {
								if(ti.Quantity_ > 0.0 || !(igp.Flags & ImportGoodsParam::fSkipZeroQtty)) {
									THROW(p_pack->InsertRow(&ti, 0));
									if(rec.get(fldn_clb, temp_buf2, 1)) {
										temp_buf2.ReplaceChar('\\', '/').ReplaceChar('-', ' ');
										// @v9.8.11 THROW(p_pack->ClbL.AddNumber(p_pack->GetTCount()-1, temp_buf2));
										THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_CLB, p_pack->GetTCount()-1, temp_buf2)); // @v9.8.11
									}
									if(rec.get(fldn_serial, temp_buf2, 1)) {
										temp_buf2.ReplaceChar('\\', '/').ReplaceChar('-', ' ');
										// @v9.8.11 THROW(p_pack->SnL.AddNumber(p_pack->GetTCount()-1, temp_buf2));
										THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_SN, p_pack->GetTCount()-1, temp_buf2)); // @v9.8.11
									}
								}
							}
						}
						if(oneof4(matrix_action, 0, 1, 1000, 1001)) {
							PPQuot quot(goods_id);
							quot.Kind = GetConfig().MtxQkID;
							quot.LocID = warehouse_id;
							quot.Quot = oneof2(matrix_action, 0, 1000) ? 0.0 : 1.0;
							THROW(P_Tbl->SetQuot(quot, 0));
						}
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			} while(in_tbl.next());
			THROW(bill_ident.FinishPackets());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PPObjPerson::Import(int specKind, int use_ta)
{
	int    ok = 1;
	IterCounter cntr;
	SString in_file_name, file_name, temp_buf, wait_msg;
	int    codetohex = 0;
	PPIDArray kind_list;
	PPID   status_id = 0;
	uint   sect = 0;
	if(specKind == PPPRK_SUPPL) {
		sect = PPINISECT_IMP_SUPPL;
		kind_list.addUnique(specKind);
	}
	else
		sect = PPINISECT_IMP_PERSON;
	FILE * rel_file = 0;

	PPObjArticle arobj;
	PPObjWorld w_obj;
	PPObjPersonStatus ps_obj;
	PPObjRegisterType rt_obj;
	PPRegisterType rt_rec;

	PPLoadText(PPTXT_IMPPERSON, wait_msg);
	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	if(!fileExists(file_name))
		return -1;
	PPIniFile ini_file(file_name);
	PPGetFilePath(PPPATH_OUT, PPFILNAM_PSNKEYS_, file_name);
	rel_file = fopen(file_name, "wb");

	ini_file.Get(sect, PPINIPARAM_FILE, in_file_name);
	ini_file.Get(sect, PPINIPARAM_PSNKINDS, temp_buf);
	ini_file.GetInt(sect, PPINIPARAM_CODETOHEX, &codetohex);

	if(temp_buf.NotEmptyS()) {
		SString kind_name;
		StringSet ss(',', temp_buf);
		for(uint pos = 0; ss.get(&pos, kind_name);) {
			PPID   kind_id = 0;
			if(PPRef->SearchName(PPOBJ_PRSNKIND, &kind_id, kind_name) > 0)
				kind_list.addUnique(kind_id);
			else {
				kind_name.Transf(CTRANSF_OUTER_TO_INNER);
				if(PPRef->SearchName(PPOBJ_PRSNKIND, &kind_id, kind_name) > 0)
					kind_list.addUnique(kind_id);
			}
		}
	}
	if(kind_list.getCount() == 0)
		kind_list.add(PPPRK_UNKNOWN);
	{
		PPObjPersonKind pk_obj;
		PPPersonKind pk_rec;
		for(uint i = 0; i < kind_list.getCount(); i++) {
			if(pk_obj.Fetch(kind_list.get(i), &pk_rec) > 0 && pk_rec.DefStatusID) {
				status_id = pk_rec.DefStatusID;
				break;
			}
		}
		SETIFZ(status_id, PPPRS_LEGAL);
	}
	DbfTable in_tbl(in_file_name);

	int fldn_code  = 0;
	int fldn_name  = 0;
	int fldn_city  = 0;
	int fldn_index = 0;
	int fldn_address  = 0;
	int fldn_phone    = 0;
	int fldn_bankacc  = 0;
	int fldn_bankname = 0;
	int fldn_bic      = 0;
	int fldn_corracc  = 0;
	int fldn_inn   = 0;
	int fldn_okonh = 0;
	int fldn_okpo  = 0;
	int fldn_memo  = 0;
	int fldn_status = 0;
	int fldn_vatfree = 0;
	int fldn_kpp     = 0;
	int fldn_extname = 0;

	PPID   key_reg_type_id = PPREGT_TPID; // Ид регистрационного документа, определяющего уникальность персоналии
		// при вызове PPObjPerson::SearchMaxLike
	if(rt_obj.Fetch(key_reg_type_id, &rt_rec) > 0) {
		if(rt_rec.Flags & REGTF_DUPNUMBER) {
			key_reg_type_id = 0;
		}
	}

	THROW_PP_S(in_tbl.isOpened(), PPERR_DBFOPFAULT, file_name);

	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CODE,      &fldn_code);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_NAME,      &fldn_name);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CITY,      &fldn_city);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_ZIP,       &fldn_index);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_ADDR,      &fldn_address);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_PHONE,     &fldn_phone);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_INN,       &fldn_inn);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_OKONH,     &fldn_okonh);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_OKPO,      &fldn_okpo);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_MEMO,      &fldn_memo);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_BANKACC,   &fldn_bankacc);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_BANKNAME,  &fldn_bankname);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_BIC,       &fldn_bic);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CORRACC,   &fldn_corracc);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_STATUS,    &fldn_status);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_VATFREE,   &fldn_vatfree);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_KPP,       &fldn_kpp);
	get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_EXTNAME,   &fldn_extname);

	PPWait(1);
	PPWaitMsg(wait_msg);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		cntr.Init(in_tbl.getNumRecs());
		if(in_tbl.top()) {
			PPID   reg_type_id = 0;
			ini_file.Get(sect, PPINIPARAM_PERSONCODEREG, temp_buf);
			PPObjRegisterType::GetByCode(temp_buf.Strip(), &reg_type_id);
			if(reg_type_id == 0) {
				if(specKind == PPPRK_SUPPL) {
					PPID acc_sheet_id = GetSupplAccSheet();
					PPAccSheet acs_rec;
					if(SearchObject(PPOBJ_ACCSHEET, acc_sheet_id, &acs_rec) > 0)
						reg_type_id = acs_rec.CodeRegTypeID;
				}
			}
			do {
				if(in_tbl.isDeletedRec() <= 0) {
					PPID   psn_id = 0, ps_id = 0;
					int64  code = 0;
					PPPersonPacket pack;
					DbfRecord rec(&in_tbl);
					THROW(in_tbl.getRec(&rec));
					rec.get(fldn_name, temp_buf);
					if(temp_buf.NotEmptyS()) {
						STRNSCPY(pack.Rec.Name, temp_buf);
						if(rec.get(fldn_extname, temp_buf, 1) && temp_buf.CmpNC(pack.Rec.Name) != 0)
							pack.SetExtName(temp_buf);
						if(rec.get(fldn_status, temp_buf, 1) && ps_obj.SearchBySymb(temp_buf, &ps_id, 0) > 0)
							pack.Rec.Status = ps_id;
						else
							pack.Rec.Status = NZOR(status_id, PPPRS_LEGAL);
						pack.Kinds.copy(kind_list);
						if(rec.get(fldn_vatfree, temp_buf, 1))
							if(temp_buf.ToLong() == 1 || temp_buf.CmpNC("Yes") == 0 || temp_buf.CmpNC("Y") == 0)
								pack.Rec.Flags |= PSNF_NOVATAX;
						rec.get(fldn_memo, temp_buf);
						temp_buf.Strip().CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
						if(reg_type_id) {
							rec.get(fldn_code, temp_buf);
							if(codetohex) {
								pack.AddRegister(reg_type_id, CodeToHex(temp_buf), 0);
							}
							else {
								code = temp_buf.Strip().ToInt64();
								if(code) {
									temp_buf.Z();
									temp_buf.Cat(code);
									pack.AddRegister(reg_type_id, temp_buf, 0);
								}
								else if(temp_buf.NotEmpty()) {
									pack.AddRegister(reg_type_id, temp_buf, 0);
								}
							}
						}
						rec.get(fldn_city, temp_buf);
						if(temp_buf.NotEmptyS()) {
							temp_buf.ShiftLeftChr('г').Strip().ShiftLeftChr('.').Strip();
							THROW(w_obj.AddSimple(&pack.Loc.CityID, WORLDOBJ_CITY, temp_buf, 0, 0));
						}
						rec.get(fldn_index, temp_buf);
						LocationCore::SetExField(&pack.Loc, LOCEXSTR_ZIP, temp_buf);
						rec.get(fldn_address, temp_buf);
						LocationCore::SetExField(&pack.Loc, LOCEXSTR_SHORTADDR, temp_buf);
						rec.get(fldn_inn, temp_buf);
						THROW(pack.AddRegister(PPREGT_TPID, temp_buf, 0));
						rec.get(fldn_okonh, temp_buf);
						THROW(pack.AddRegister(PPREGT_OKONH, temp_buf, 0));
						rec.get(fldn_okpo, temp_buf);
						THROW(pack.AddRegister(PPREGT_OKPO, temp_buf, 0));
						rec.get(fldn_kpp, temp_buf);
						THROW(pack.AddRegister(PPREGT_KPP, temp_buf, 0));
						rec.get(fldn_phone, temp_buf);
						if(temp_buf.NotEmptyS())
							THROW(pack.ELA.AddItem(PPELK_WORKPHONE, temp_buf));
						{
							SString accno;
							PPBank bnk_rec;
							rec.get(fldn_bankname, temp_buf);
							STRNSCPY(bnk_rec.Name, temp_buf.Strip());
							rec.get(fldn_bic, temp_buf);
							STRNSCPY(bnk_rec.BIC, temp_buf.Strip());
							rec.get(fldn_corracc, temp_buf);
							STRNSCPY(bnk_rec.CorrAcc, temp_buf.Strip());
							rec.get(fldn_bankacc, accno);
							if(bnk_rec.Name[0] && accno.NotEmptyS()) {
								PPID   bnk_id = 0;
								/* @v9.0.4
								BankAccountTbl::Rec bnkacc_rec;
								MEMSZERO(bnkacc_rec);
								THROW(AddBankSimple(&bnk_id, &bnk_rec, 0));
								bnkacc_rec.BankID  = bnk_id;
								bnkacc_rec.AccType = PPBAC_CURRENT;
								STRNSCPY(bnkacc_rec.Acct, accno);
								THROW_SL(pack.BAA.insert(&bnkacc_rec));
								*/
								// @v9.0.4 {
								PPBankAccount ba;
								THROW(AddBankSimple(&bnk_id, &bnk_rec, 0));
								ba.BankID = bnk_id;
								ba.AccType = PPBAC_CURRENT;
								STRNSCPY(ba.Acct, accno);
								THROW(pack.Regs.SetBankAccount(&ba, static_cast<uint>(-1)));
								// } @v9.0.4
							}
						}
						if(SearchMaxLike(&pack, &psn_id, 0, key_reg_type_id /*PPREGT_TPID*/) > 0)
							pack.Rec.ID = psn_id;
						else
							THROW(PutPacket(&(psn_id = 0), &pack, 0));
						if(code && pack.Rec.ID && rel_file) {
							fwrite(&code, sizeof(code), 1, rel_file);
							fwrite(&pack.Rec.ID, sizeof(pack.Rec.ID), 1, rel_file);
						}
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			} while(in_tbl.next());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWait(0);
	SFile::ZClose(&rel_file);
	return ok;
}

int SLAPI Import(PPID objType, long extraParam)
{
	int    ok = 1;
	if(objType == PPOBJ_GOODSGROUP) {
		PPObjGoodsGroup gg_obj;
		PPWait(1);
		THROW(gg_obj.Import(1));
		PPWait(0);
	}
	else if(objType == PPOBJ_GOODS) {
		PPObjGoods g_obj;
		if(extraParam == 1) {
			THROW(g_obj.ImportOld(1));
		}
		else {
			THROW(g_obj.Import(0, 0, 1));
		}
	}
	else if(objType == PPOBJ_QUOT) {
		PPObjGoods g_obj;
		if(extraParam == 1) {
			THROW(g_obj.ImportQuotOld(1));
		}
		else {
			THROW(g_obj.ImportQuot(0, 1));
		}
	}
	else if(objType == PPOBJ_PERSON) {
		PPObjPerson p_obj;
		if(extraParam == PPPRK_SUPPL) {
			THROW(p_obj.Import(extraParam, 1));
		}
		else
			THROW(p_obj.Import(0, 1));
	}
	else if(objType == PPOBJ_COUNTRY) {
		PPObjWorld w_obj;
		THROW(w_obj.ImportCountry(1));
	}
	else if(objType == PPOBJ_SCARD) {
		PPObjSCard sc_obj;
		THROW(sc_obj.Import(1));
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}
//
//
//
int SLAPI EditSpecSeriesFormatDescription()
{
	int    ok = -1;
	ImpExpParamDialog * dlg = 0;
	PPImpExpParam param;
	SString ini_file_name, sect_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, ini_file_name));
	{
		PPIniFile::GetSectSymb(PPINISECT_IMP_SPECSER, sect_name);
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_SPECSERIES, &param.InrRec));
		THROW(param.ReadIni(&ini_file, sect_name, 0));
		THROW(CheckDialogPtr(&(dlg = new ImpExpParamDialog(DLG_IMPEXP, ImpExpParamDialog::fDisableExport))));
		dlg->setDTS(&param);
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&param)) {
				ini_file.ClearSection(sect_name);
				if(param.WriteIni(&ini_file, sect_name) && ini_file.FlashIniBuf()) {
					ok = 1;
				}
				else
					PPError();
			}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI ImportSpecSeries()
{
	int    ok = 1, r;
	PPLogger logger;
	PPImpExp * p_impexp = 0;
	PPImpExpParam param;
	SString ini_file_name, sect_name, wait_msg;
	PPIniFile::GetSectSymb(PPINISECT_IMP_SPECSER, sect_name);
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name);
		THROW(LoadSdRecord(PPREC_SPECSERIES, &param.InrRec));
		THROW(param.ReadIni(&ini_file, sect_name, 0));
		THROW_MEM(p_impexp = new PPImpExp(&param, 0));
		THROW(p_impexp->OpenFileForReading(0));
		{
			int    skip = 1;
			int    clear_data = 0;
			{
				TDialog * dlg = new TDialog(DLG_IMPSPOIL);
				THROW(CheckDialogPtr(&dlg));
				dlg->setCtrlUInt16(CTL_IMPSPOIL_FLAGS, BIN(clear_data));
				if(ExecView(dlg) == cmOK) {
					clear_data = BIN(dlg->getCtrlUInt16(CTL_IMPSPOIL_FLAGS));
					skip = 0;
				}
				delete dlg;
				dlg = 0;
			}
			if(!skip) {
				SString temp_buf, name1_buf, name2_buf;
				long   numrecs = 0;
				long   accepted_count = 0;
				p_impexp->GetNumRecs(&numrecs);
				IterCounter cntr;
				cntr.Init(numrecs);
				Sdr_SpecSeries src_rec;
				SpecSeriesCore ss_tbl;
				PPLoadText(PPTXT_IMPSPECSER, wait_msg);
				PPWait(1);
				PPWaitMsg(wait_msg);
				PPTransaction tra(1);
				THROW(tra);
				/*
				if(clear_data)
					THROW(ss_tbl.ClearAll());
				*/
				if(clear_data) {
					logger.LogString(PPTXT_IMPSPOIL_CLEAR, 0);
					THROW(deleteFrom(&ss_tbl, 0, *reinterpret_cast<DBQ *>(0)));
				}
				MEMSZERO(src_rec);
				while((r = p_impexp->ReadRecord(&src_rec, sizeof(src_rec))) > 0) {
					int    dup = 0;
					SpecSeries2Tbl::Rec ss_rec;
					// @v10.6.4 MEMSZERO(ss_rec);
					ss_rec.InfoKind = SPCSERIK_SPOILAGE;
					STRNSCPY(ss_rec.Serial, src_rec.Serial);
					SpecSeriesCore::SetExField(&ss_rec, SPCSNEXSTR_GOODSNAME, (temp_buf = src_rec.GoodsName).Transf(CTRANSF_OUTER_TO_INNER));
					SpecSeriesCore::SetExField(&ss_rec, SPCSNEXSTR_MANUFNAME, (temp_buf = src_rec.ManufName).Transf(CTRANSF_OUTER_TO_INNER));
					ss_rec.InfoDate = src_rec.InfoDate;
					//
					STRNSCPY(ss_rec.InfoIdent, src_rec.InfoIdent);
					SpecSeriesCore::SetExField(&ss_rec, SPCSNEXSTR_LABNAME, (temp_buf = src_rec.LabName).Transf(CTRANSF_OUTER_TO_INNER));
					SpecSeriesCore::SetExField(&ss_rec, SPCSNEXSTR_MANUFCOUNTRYNAME, (temp_buf = src_rec.ManufCountryName).Transf(CTRANSF_OUTER_TO_INNER));
					ss_rec.AllowDate = src_rec.AllowDate;
					STRNSCPY(ss_rec.AllowNumber, src_rec.AllowNumber);
					SpecSeriesCore::SetExField(&ss_rec, SPCSNEXSTR_DESCRIPTION, (temp_buf = src_rec.SpecName).Transf(CTRANSF_OUTER_TO_INNER));
					STRNSCPY(ss_rec.LetterType, src_rec.LetterType);
					SETFLAG(ss_rec.Flags, SPCSELIF_FALSIFICATION, src_rec.FalsificationFlag);
					SETFLAG(ss_rec.Flags, SPCSELIF_ALLOW, src_rec.AllowFlag);
					//
					if(ss_rec.Serial[0]) {
						SpecSeries2Tbl::Key1 k1;
						MEMSZERO(k1);
						k1.InfoKind = ss_rec.InfoKind;
						memcpy(k1.Serial, ss_rec.Serial, sizeof(k1.Serial));
						if(ss_tbl.search(1, &k1, spEq))
							do {
								SpecSeriesCore::GetExField(&ss_tbl.data, SPCSNEXSTR_GOODSNAME, name1_buf);
								SpecSeriesCore::GetExField(&ss_rec, SPCSNEXSTR_GOODSNAME, name2_buf);
								if(name1_buf.CmpNC(name2_buf) == 0 && ss_rec.InfoDate == ss_tbl.data.InfoDate) {
									temp_buf.Z().Cat(name2_buf).CatDiv(':', 1).Cat(ss_rec.Serial).CatDiv(':', 1).Cat(ss_rec.InfoDate);
									logger.LogString(PPTXT_IMPSPOIL_DUP, temp_buf);
									dup = 1;
								}
							} while(!dup && ss_tbl.search(1, &k1, spNext) && ss_rec.InfoKind == ss_tbl.data.InfoKind && stricmp866(ss_rec.Serial, ss_tbl.data.Serial) == 0);
						if(!dup) {
							THROW_DB(ss_tbl.insertRecBuf(&ss_rec));
							SpecSeriesCore::GetExField(&ss_rec, SPCSNEXSTR_GOODSNAME, name2_buf);
							temp_buf.Z().Cat(name2_buf).CatDiv(':', 1).Cat(ss_rec.Serial).CatDiv(':', 1).Cat(ss_rec.InfoDate);
							logger.LogString(PPTXT_IMPSPOIL_ACCEPT, temp_buf);
							++accepted_count;
						}
					}
					MEMSZERO(src_rec);
					PPWaitPercent(cntr.Increment(), wait_msg);
				}
				THROW(r);
				THROW(tra.Commit());
				PPMessage(mfInfo|mfOK, PPINF_RCVCURRSCOUNT, wait_msg.Z().Cat(accepted_count));
			}
		}
	}
	CATCH
		ok = PPErrorZ();
		logger.LogLastError();
	ENDCATCH
	delete p_impexp;
	PPWait(0);
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	return ok;
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(PHONELIST, PPPhoneListImpExpParam);

PPPhoneListImpExpParam::PPPhoneListImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
}

#define IMPEXPPARAM_PHONELIST_DEFCITYPHONEPREFIX 1

//virtual
int PPPhoneListImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(DefCityPhonePrefix.NotEmpty())
			param_list.Add(IMPEXPPARAM_PHONELIST_DEFCITYPHONEPREFIX, temp_buf = DefCityPhonePrefix);
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		DefCityPhonePrefix = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case IMPEXPPARAM_PHONELIST_DEFCITYPHONEPREFIX:
					DefCityPhonePrefix = temp_buf;
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPPhoneListImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name, param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	if(Direction != 0) {
		if(DefCityPhonePrefix.NotEmpty())
			pFile->AppendParam(pSect, "DefCityPhonePrefix", DefCityPhonePrefix, 1);
	}
	CATCHZOK
	return ok;
}

int PPPhoneListImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	excl.add("DefCityPhonePrefix");
	if(pFile->GetParam(pSect, "DefCityPhonePrefix", param_val) > 0)
		DefCityPhonePrefix = param_val;
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

PhoneListImpExpDialog::PhoneListImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPPHONE, 0)
{
}

int PhoneListImpExpDialog::setDTS(const PPPhoneListImpExpParam * pData)
{
	int    ok = 1;
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);
	setCtrlString(CTL_IMPEXPPHONE_DEFPFX, Data.DefCityPhonePrefix);
	return ok;
}

int PhoneListImpExpDialog::getDTS(PPPhoneListImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	getCtrlString(sel = CTL_IMPEXPPHONE_DEFPFX, Data.DefCityPhonePrefix);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int SLAPI EditPhoneListParam(const char * pIniSection)
{
	int    ok = -1;
	PhoneListImpExpDialog * dlg = 0;
	PPPhoneListImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
   		THROW(CheckDialogPtr(&(dlg = new PhoneListImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_PHONELIST, &param.InrRec));
   		direction = param.Direction;
   		if(!isempty(pIniSection))
   			THROW(param.ReadIni(&ini_file, pIniSection, 0));
   		dlg->setDTS(&param);
   		while(ok <= 0 && ExecView(dlg) == cmOK)
   			if(dlg->getDTS(&param)) {
   				int is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
   				if(!isempty(pIniSection))
   					if(is_new)
   						ini_file.RemoveSection(pIniSection);
   					else
   						ini_file.ClearSection(pIniSection);
   				PPSetError(PPERR_DUPOBJNAME);
   				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
   					ok = 1;
   				else
   					PPError();
   			}
   			else
   				PPError();
   	}
   	CATCH
   		ok = PPErrorZ();
   	ENDCATCH
   	delete dlg;
   	return ok;
}

class PhoneListImpExpCfgListDialog : public ImpExpCfgListDialog {
public:
	PhoneListImpExpCfgListDialog() : ImpExpCfgListDialog()
	{
		SetParams(PPFILNAM_IMPEXP_INI, PPREC_PHONELIST, &Param, 0);
	}
private:
	virtual int EditParam(const char * pIniSection)
	{
		return EditPhoneListParam(pIniSection);
	}
	PPPhoneListImpExpParam Param;
};

class PrcssrPhoneListImport {
public:
	struct Param {
		Param() : DefCityID(0)
		{
		}
		PPID   DefCityID;
		SString CfgName;
	};
	SLAPI  PrcssrPhoneListImport()
	{
	}
	int    SLAPI InitParam(Param *);
	int    SLAPI EditParam(Param *);
	int    SLAPI Init(const Param *);
	int    SLAPI Run();
private:
	PPObjLocation LocObj;
	Param  P;
	PPPhoneListImpExpParam IeParam;
};

int SLAPI PrcssrPhoneListImport::InitParam(Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		pParam->DefCityID = 0;
		pParam->CfgName = 0;
		ok = 1;
	}
	return ok;
}

int PrcssrPhoneListImport::EditParam(Param * pParam)
{
	class PhoneListImportDialog : public TDialog {
	public:
		PhoneListImportDialog() : TDialog(DLG_IEPHONE)
		{
			PPPhoneListImpExpParam param;
			GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_PHONELIST, &param, &CfgList, 2 /* import */);
		}
		int    setDTS(const PrcssrPhoneListImport::Param * pData)
		{
			Data = *pData;
			int    ok = 1;
			long   cfg_id = 0;
			if(CfgList.getCount() == 1)
				cfg_id = CfgList.Get(0).Id;
			SetupStrAssocCombo(this, CTLSEL_IEPHONE_CFG, &CfgList, cfg_id, 0, 0, 0);
			SetupPPObjCombo(this, CTLSEL_IEPHONE_DEFCITY, PPOBJ_WORLD, Data.DefCityID, 0, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
			return ok;
		}
		int    getDTS(PrcssrPhoneListImport::Param * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			long   cfg_id = getCtrlLong(sel = CTLSEL_IEPHONE_CFG);
			SString temp_buf;
			THROW_PP(cfg_id, PPERR_CFGNEEDED);
			THROW_PP(CfgList.GetText(cfg_id, Data.CfgName) > 0, PPERR_CFGNEEDED);
			Data.DefCityID = getCtrlLong(sel = CTLSEL_IEPHONE_DEFCITY);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		PrcssrPhoneListImport::Param Data;
		StrAssocArray CfgList;
	};
	DIALOG_PROC_BODY(PhoneListImportDialog, pParam);
}

int PrcssrPhoneListImport::Init(const Param * pParam)
{
	P = *pParam;
	int    ok = 1;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_PHONELIST, &IeParam.InrRec));
		IeParam.Direction = 1; // import
		THROW(IeParam.ProcessName(1, P.CfgName));
		THROW(IeParam.ReadIni(&ini_file, P.CfgName, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPhoneListImport::Run()
{
	int    ok = 1;
	long   numrecs = 0;
	PPID   def_city_id = 0;
	SString log_msg, fmt_buf, temp_buf, temp_buf2;
	SString phone, city_prefix, address, contact;
	SString f_addr, f_cont;
	PPIDArray phone_idx_list;
	PPLogger logger;
	PPImpExp ie(&IeParam, 0);
	PPWait(1);
	if(P.DefCityID) {
		PPObjWorld w_obj;
		WorldTbl::Rec w_rec;
		if(w_obj.Search(P.DefCityID, &w_rec) > 0 && w_rec.Kind == WORLDOBJ_CITY)
			def_city_id = P.DefCityID;
	}
	THROW_PP(CConfig.Flags2 & CCFLG2_INDEXEADDR, PPERR_CCFG_INDEXEADDRNEEDED);
	THROW(ie.OpenFileForReading(0));
	THROW(ie.GetNumRecs(&numrecs));
	if(numrecs) {
		int    r;
		IterCounter cntr;
		Sdr_PhoneList rec;
		PPEAddr::Phone::NormalizeStr(IeParam.DefCityPhonePrefix, 0, city_prefix);
		PPTransaction tra(1);
		THROW(tra);
		{
			cntr.Init(numrecs);
			MEMSZERO(rec);
			while((r = ie.ReadRecord(&rec, sizeof(rec))) > 0) {
				int    found = 0;
				PPEAddr::Phone::NormalizeStr(rec.Phone, 0, phone);
				if(phone.Len() >= 5) {
					if(city_prefix.Len()) {
						size_t sl = phone.Len() + city_prefix.Len();
						if(oneof2(sl, 10, 11)) {
							phone = (temp_buf = city_prefix).Cat(phone);
						}
					}
					(address = rec.Address).Strip().ReplaceStr("  ", " ", 0).Transf(CTRANSF_OUTER_TO_INNER);
					(contact = rec.Contact).Strip().ReplaceStr("  ", " ", 0).Transf(CTRANSF_OUTER_TO_INNER);
					if(address.NotEmpty() || contact.NotEmpty()) {
						phone_idx_list.clear();
						if(LocObj.P_Tbl->SearchPhoneIndex(phone, 0, phone_idx_list) > 0) {
							for(uint i = 0; !found && i < phone_idx_list.getCount(); i++) {
								EAddrTbl::Rec eaddr_rec;
								LocationTbl::Rec ea_loc_rec;
								if(LocObj.P_Tbl->GetEAddr(phone_idx_list.get(i), &eaddr_rec) > 0) {
									if(eaddr_rec.LinkObjType == PPOBJ_LOCATION && LocObj.Search(eaddr_rec.LinkObjID, &ea_loc_rec) > 0) {
										LocationCore::GetExField(&ea_loc_rec, LOCEXSTR_SHORTADDR, f_addr);
										LocationCore::GetExField(&ea_loc_rec, LOCEXSTR_CONTACT,   f_cont);
										if(f_addr.CmpNC(address) == 0 && f_cont.CmpNC(contact) == 0) {
											found = 1;
											// PPTXT_IMPPHONE_ANALOGFOUND "Найден аналог для телефона: @zstr; @zstr; @zstr"
											PPFormatT(PPTXT_IMPPHONE_ANALOGFOUND, &log_msg, phone.cptr(), address.cptr(), contact.cptr());
											logger.Log(log_msg);
										}
									}
								}
							}
						}
						if(!found) {
							PPID   loc_id = 0;
							LocationTbl::Rec loc_rec;
							// @v10.6.4 MEMSZERO(loc_rec);
							LocationCore::SetExField(&loc_rec, LOCEXSTR_PHONE, phone);
							if(address.NotEmpty())
								LocationCore::SetExField(&loc_rec, LOCEXSTR_SHORTADDR, address);
							if(contact.NotEmpty())
								LocationCore::SetExField(&loc_rec, LOCEXSTR_CONTACT, contact);
							loc_rec.Type = LOCTYP_ADDRESS;
							loc_rec.Flags |= LOCF_STANDALONE;
							loc_rec.CityID = def_city_id;
							THROW(LocObj.P_Tbl->Add(&loc_id, &loc_rec, 0));
							// PPTXT_IMPPHONE_CREATED "Создана запись автономного адреса: @zstr; @zstr; @zstr"
							logger.Log(PPFormatT(PPTXT_IMPPHONE_CREATED, &log_msg, phone.cptr(), address.cptr(), contact.cptr()));
						}
					}
				}
				else {
					// PPTXT_IMPPHONE_INVPHONE "Минимальная длина телефона должна быть 5 цифр: @zstr; @zstr; @zstr"
					logger.Log(PPFormatT(PPTXT_IMPPHONE_INVPHONE, &log_msg, phone.cptr(), address.cptr(), contact.cptr()));
				}
				MEMSZERO(rec);
				PPWaitPercent(cntr.Increment());
			}
			THROW(r);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	PPWait(0);
	return ok;
}

int SLAPI ImportPhoneList()
{
	int    ok = -1;
	PrcssrPhoneListImport prcssr;
	PrcssrPhoneListImport::Param param;
	prcssr.InitParam(&param);
	while(ok < 0 && prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			PPError();
	return ok;
}
//
//
//
int SLAPI ImportBanks()
{
	int    ok = 1, import = 0;
	PPLogger logger;
	IterCounter cntr;
	PPID   srch_region = 0;
	char   srch_city[256];
	SString file_name, name, wait_msg, temp_buf;
	const  uint sect = PPINISECT_IMP_BANK;
	TDialog * p_dlg = 0;

	PPLoadText(PPTXT_IMPBANK, wait_msg);
	PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, file_name);
	THROW_SL(fileExists(file_name));
	{
		PPIniFile ini_file(file_name);

		int fldn_regionid = 0;
		int fldn_region   = 0;
		int fldn_city     = 0;
		int fldn_name     = 0;
		int fldn_bic      = 0;
		int fldn_corracc  = 0;

		memzero(srch_city, sizeof(srch_city));

		ini_file.Get(sect, PPINIPARAM_REGIONFILE, file_name);
		DbfTable region_tbl(file_name);
		SCollection region_sc, city_sc;

		ini_file.Get(sect, PPINIPARAM_FILE, file_name);
		DbfTable in_tbl(file_name);

		THROW_PP(region_tbl.isOpened() && in_tbl.isOpened(), PPERR_DBFOPFAULT);

		get_fld_number(&ini_file, &region_tbl, sect, PPINIPARAM_REGIONID, &fldn_regionid);
		get_fld_number(&ini_file, &region_tbl, sect, PPINIPARAM_REGIONNAME, &fldn_region);

		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CITY,      &fldn_city);
		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_NAME,      &fldn_name);
		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_BIC,       &fldn_bic);
		get_fld_number(&ini_file, &in_tbl, sect, PPINIPARAM_CORRACC,   &fldn_corracc);

		PPWait(1);
		if(region_tbl.top()) {
			StringSet ss(",");
			do {
				long   id = 0;
				if(region_tbl.isDeletedRec() <= 0) {
					DbfRecord rec(&region_tbl);
					THROW(region_tbl.getRec(&rec));
					rec.get(fldn_regionid, id);
					rec.get(fldn_region, name);
					if(name.NotEmptyS()) {
						char idstr[128];
						ss.clear();
						ss.add(ltoa(id, idstr, 10));
						ss.add(name);
						region_sc.insert(newStr(ss.getBuf()));
					}
				}
			} while(region_tbl.next());
		}
		if(in_tbl.top()) {
			long   i = 1;
			do {
				if(in_tbl.isDeletedRec() <= 0) {
					DbfRecord rec(&in_tbl);
					THROW(in_tbl.getRec(&rec));
					rec.get(fldn_city, name);
					if(name.NotEmptyS() && !city_sc.lsearch(name, 0, PTR_CMPFUNC(Pchar))) {
						city_sc.insert(newStr(name));
						i++;
					}
				}
			} while(in_tbl.next());
		}
		PPWait(0);

		p_dlg = new TDialog(DLG_REGIONSEL);
		THROW(CheckDialogPtr(&p_dlg));
		city_sc.sort(PTR_CMPFUNC(Pchar));
		SetupSCollectionComboBox(p_dlg, CTLSEL_REGIONSEL_REGION, &region_sc, 0);
		SetupSCollectionComboBox(p_dlg, CTLSEL_REGIONSEL_CITY, &city_sc, 0);
		if(ExecView(p_dlg) == cmOK) {
			srch_region = p_dlg->getCtrlLong(CTLSEL_REGIONSEL_REGION);
			const long pos = p_dlg->getCtrlLong(CTLSEL_REGIONSEL_CITY);
			if(pos > 0) {
				const char * p_buf = (const char *)city_sc.at(pos-1);
				STRNSCPY(srch_city, p_buf);
			}
			import = 1;
		}
		if(import) {
			int    accepted_recs_count = 0;
			SString fmt_buf, msg_buf;
			SString city, name, bic, corracc, addr;
			PPObjPerson psn_obj;
			PPWaitMsg(wait_msg);
			{
				PPTransaction tra(1);
				THROW(tra);
				cntr.Init(in_tbl.getNumRecs());
				PPWait(1);
				if(in_tbl.top()) {
					PPObjWorld w_obj;
					do {
						int    accept_rec = 1;
						if(in_tbl.isDeletedRec() <= 0) {
							PPID   psn_id = 0, region = 0;
							PPPersonPacket pack;
							DbfRecord rec(&in_tbl);
							THROW(in_tbl.getRec(&rec));
							pack.Rec.Status = PPPRS_LEGAL;
							pack.Kinds.add(PPPRK_BANK);
							rec.get(fldn_city, city);
							city.Strip();
							rec.get(fldn_name, name);
							if(name.NotEmptyS())
								name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
							rec.get(fldn_corracc, corracc);
							if(corracc.NotEmptyS())
								pack.AddRegister(PPREGT_BNKCORRACC, corracc, 1);
							rec.get(fldn_bic, bic);
							if(bic.NotEmptyS())
								pack.AddRegister(PPREGT_BIC, bic, 1);
							region = bic.Sub(2, 3, temp_buf).ToLong();
							accept_rec = BIN((!srch_city[0] || stricmp866(srch_city, city) == 0) && (!srch_region || srch_region == region));
							if(accept_rec) {
								if(psn_obj.SearchMaxLike(&pack, &psn_id, 0, PPREGT_BIC) > 0) {
									THROW(psn_obj.GetPacket(psn_id, &pack, 0) > 0);
									LocationCore::GetExField(&pack.Loc, LOCEXSTR_SHORTADDR, addr);
									if(addr.Empty() || addr.CmpNC(city) == 0) {
										THROW(w_obj.AddSimple(&pack.Loc.CityID, WORLDOBJ_CITY, city, 0, 0));
										LocationCore::SetExField(&pack.Loc, LOCEXSTR_SHORTADDR, addr.Z());
										name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
										if(corracc.NotEmpty()) {
											if(!pack.AddRegister(PPREGT_BNKCORRACC, corracc, 1)) {
												logger.LogLastError();
												accept_rec = 0;
											}
										}
										if(bic.NotEmpty()) {
											if(!pack.AddRegister(PPREGT_BIC, bic, 1)) {
												logger.LogLastError();
												accept_rec = 0;
											}
										}
										if(accept_rec) {
											THROW(psn_obj.PutPacket(&pack.Rec.ID, &pack, 0));
											PPFormatT(PPTXT_LOG_IMPBNK_UPD, &msg_buf, name.cptr(), city.cptr(), bic.cptr(), corracc.cptr());
											logger.Log(msg_buf);
										}
									}
								}
								else {
									THROW(w_obj.AddSimple(&pack.Loc.CityID, WORLDOBJ_CITY, city, 0, 0));
									THROW(psn_obj.PutPacket(&(psn_id = 0), &pack, 0));
									PPFormatT(PPTXT_LOG_IMPBNK_ADD, &msg_buf, name.cptr(), city.cptr(), bic.cptr(), corracc.cptr());
									logger.Log(msg_buf);
									accepted_recs_count++;
								}
							}
						}
						PPWaitPercent(cntr.Increment(), wait_msg);
					} while(in_tbl.next());
				}
				PPWait(0);
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	delete p_dlg;
	return ok;
}
//
//
//
class PrcssrImportKLADR {
public:
	struct Param {
		enum {
			fSkipSmallItems = 0x0001, // Не импортировать подчиненные населенные пункты (код населенного пункта !=0)
			fStreet         = 0x0002  // Импортировать улицы
		};
		long   Flags;
		SString ParentCodeString;
	};
	SLAPI  PrcssrImportKLADR() : StatusList(sizeof(Status)), P_TempTbl(0)
	{
		MEMSZERO(P);
	}
	SLAPI ~PrcssrImportKLADR()
	{
		ZDELETE(P_TempTbl);
	}
	int    SLAPI EditParam(Param *);
	int    SLAPI Run();
private:
	int    SLAPI PreImport();
	int    SLAPI Import();
	int    SLAPI ProcessTempTable();
	int    SLAPI SearchStatus(const char * pName, int abbr, long * pCode) const;
	struct Status {
		long   Code;
		long   ID;       // Идентификатор в базе данных
		char   Name[32];
		char   Abbr[16];
	};
	Param P;
	SArray StatusList; // Импортированный список статусов объектов
	TempKLADRTbl * P_TempTbl; // Импортированная таблица объектов
	PPObjWorld WObj;
	PPObjWorldObjStatus WosObj;
};

PP_CREATE_TEMP_FILE_PROC(CreateTempKLADR_File, TempKLADR);

int SLAPI PrcssrImportKLADR::EditParam(Param * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_IMPKLADR);
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlString(CTL_IMPKLADR_PARENT, pData->ParentCodeString);
	dlg->AddClusterAssoc(CTL_IMPKLADR_FLAGS, 0, Param::fSkipSmallItems);
	dlg->AddClusterAssoc(CTL_IMPKLADR_FLAGS, 1, Param::fStreet);
	dlg->SetClusterData(CTL_IMPKLADR_FLAGS, pData->Flags);
	if(ExecView(dlg) == cmOK) {
		dlg->getCtrlString(CTL_IMPKLADR_PARENT, pData->ParentCodeString);
		dlg->GetClusterData(CTL_IMPKLADR_FLAGS, &pData->Flags);
		ok = 1;
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int SLAPI PrcssrImportKLADR::SearchStatus(const char * pName, int abbr, long * pCode) const
{
	Status * p_item;
	for(uint i = 0; StatusList.enumItems(&i, (void **)&p_item);) {
		if(abbr) {
			if(stricmp866(p_item->Abbr, pName) == 0) {
				ASSIGN_PTR(pCode, p_item->Code);
				return static_cast<int>(i);
			}
		}
		else {
			if(stricmp866(p_item->Name, pName) == 0) {
				ASSIGN_PTR(pCode, p_item->Code);
				return static_cast<int>(i);
			}
		}
	}
	ASSIGN_PTR(pCode, 0);
	return 0;
}

struct KLADRCODE {
	KLADRCODE()
	{
		Init();
	}
	void   Init()
	{
		Sub = Reg = Mr = Cit = Str = ID = 0;
	}
	int    FromStr(const SString & rBuf, SString & rTemp)
	{
		Sub = rBuf.Sub( 0, 2, rTemp).ToLong();
		Reg = rBuf.Sub( 2, 3, rTemp).ToLong();
		Mr  = rBuf.Sub( 5, 3, rTemp).ToLong();
		Cit = rBuf.Sub( 8, 3, rTemp).ToLong();
		Str = rBuf.Sub(11, 4, rTemp).ToLong();
		return 1;
	}
	uint   IsParent() const
	{
		if(Str)
			return 0;
		else if(Cit)
			return 4;
		else if(Mr)
			return 3;
		else if(Reg)
			return 2;
		else if(Sub)
			return 1;
		else
			return 0;
	}
	uint   GetParentIdx() const
	{
		if(Str) {
			if(Cit)
				return 4;
			else if(Mr)
				return 3;
			else if(Reg)
				return 2;
			else if(Sub)
				return 1;
		}
		else if(Cit) {
			if(Mr)
				return 3;
			else if(Reg)
				return 2;
			else if(Sub)
				return 1;
		}
		else if(Mr) {
			if(Reg)
				return 2;
			else if(Sub)
				return 1;
		}
		else if(Reg) {
			if(Sub)
				return 1;
		}
		return 0;
	}
	long   Sub; // Субъект государства
	long   Reg; // Регион
	long   Mr;  // Район
	long   Cit; // Город
	long   Str; // Улица
	long   ID;
};

int SLAPI PrcssrImportKLADR::ProcessTempTable()
{
	int    ok = 1;
	if(P_TempTbl) {
		SString buf, wait_msg, temp_buf;
		PPLoadText(PPTXT_IMPKLADR_PREPROCESS, wait_msg);
		//
		KLADRCODE code;
		KLADRCODE parent_code[4];
		// idx:
		// 0 - регион
		// 1 - район
		// 2 - город
		// 3 - населенный пункт (для улиц)
		// -------------------------
		// 01 234 567 890 [1234][56]
		//                 0000  12
		// СС РРР ГГГ ППП [УУУУ] АА
		//
		IterCounter cntr;
		PPInitIterCounter(cntr, P_TempTbl);
		TempKLADRTbl::Key1 k1;
		MEMSZERO(k1);
		if(P_TempTbl->search(1, &k1, spFirst))
			do {
				buf = P_TempTbl->data.Code;
				if(buf.Len() >= 13) {
					code.FromStr(buf, temp_buf);
					code.ID = P_TempTbl->data.ID__;
					uint   parent_idx = code.GetParentIdx();
					if(parent_idx) {
						P_TempTbl->data.ParentID = parent_code[parent_idx-1].ID;
						THROW_DB(P_TempTbl->updateRec());
					}
					parent_idx = code.IsParent();
					if(parent_idx) {
						parent_code[parent_idx-1] = code;
						for(int i = parent_idx; i < 4; i++)
							parent_code[i].Init();
					}
				}
				PPWaitPercent(cntr.Increment(), wait_msg);
			} while(P_TempTbl->search(1, &k1, spNext));
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrImportKLADR::PreImport()
{
	ZDELETE(P_TempTbl);

	int    ok = 1, ta = 0, r;
	PPID   native_country_id = 0;
	PPImpExp * p_impexp = 0;
	SString ini_file_name, sect_name, wait_msg;
	SString temp_buf;
	PPWait(1);
	THROW(WObj.GetNativeCountry(&native_country_id) > 0);
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPORT_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name);
		//
		// Импорт статусов
		//
		PPImpExpParam param;
		PPIniFile::GetSectSymb(PPINISECT_IMPKLADR_STATUS, sect_name);

		THROW(LoadSdRecord(PPREC_WORLDOBJSTATUS, &param.InrRec));
		THROW(param.ReadIni(&ini_file, sect_name, 0));
		THROW_MEM(p_impexp = new PPImpExp(&param, 0));
		THROW(p_impexp->OpenFileForReading(0));
		{
			Sdr_WorldObjStatus src_rec;
			while((r = p_impexp->ReadRecord(&src_rec, sizeof(src_rec))) > 0) {
				//p_impexp->GetParam().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &src_rec);
				int    do_add = 1;
				Status rec;
				MEMSZERO(rec);
				rec.Code = src_rec.Code;
				(temp_buf = src_rec.Name).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(rec.Name, temp_buf);
				(temp_buf = src_rec.Abbr).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(rec.Abbr, temp_buf);
				long fcode = 0;
				int  fpos = SearchStatus(rec.Abbr, 1, &fcode);
				if(fpos) {
					PPID   fid = 0;
					if(WosObj.SearchByCode(fcode, 0, &fid, 0) > 0)
						do_add = 0;
					else if(WosObj.SearchByCode(rec.Code, 0, &fid, 0) > 0)
						StatusList.atFree(fpos-1);
				}
				if(do_add)
					THROW_SL(StatusList.insert(&rec));
			}
			THROW(r);
		}
		ZDELETE(p_impexp);
		//
		// Создание новой временной таблицы
		//
		THROW(P_TempTbl = CreateTempKLADR_File());
		//
		// Импорт регионов и городов
		//
		PPLoadText(PPTXT_IMPKLADR_KLADR, wait_msg);
		PPIniFile::GetSectSymb(PPINISECT_IMPKLADR, sect_name);
		param.Init();
		THROW(LoadSdRecord(PPREC_WORLDOBJ, &param.InrRec));
		THROW(param.ReadIni(&ini_file, sect_name, 0));
		THROW_MEM(p_impexp = new PPImpExp(&param, 0));
		THROW(p_impexp->OpenFileForReading(0));
		{
			Sdr_WorldObj src_rec;
			long   numrecs = 0;
			p_impexp->GetNumRecs(&numrecs);
			IterCounter cntr;
			cntr.Init(numrecs);
			BExtInsert bei(P_TempTbl);
			while((r = p_impexp->ReadRecord(&src_rec, sizeof(src_rec))) > 0) {
				char status_text[64];
				TempKLADRTbl::Rec rec;
				// @v10.6.4 MEMSZERO(rec);
				(temp_buf = src_rec.Name).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(rec.Name, temp_buf);
				(temp_buf = src_rec.StatusAbbr).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(status_text, temp_buf);
				SearchStatus(status_text, 1, &rec.StatusCode);
				STRNSCPY(rec.Code, src_rec.Code);
				STRNSCPY(rec.ZIP, src_rec.ZIP);
				THROW_DB(bei.insert(&rec));
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
			THROW(r);
			THROW_DB(bei.flash());
		}
		//
		// Импорт улиц
		//
		PPLoadText(PPTXT_IMPKLADR_STREET, wait_msg);
		PPIniFile::GetSectSymb(PPINISECT_IMPKLADR_STREET, sect_name);
		param.Init();
		THROW(LoadSdRecord(PPREC_WORLDOBJ, &param.InrRec));
		THROW(param.ReadIni(&ini_file, sect_name, 0));
		THROW_MEM(p_impexp = new PPImpExp(&param, 0));
		THROW(p_impexp->OpenFileForReading(0));
		{
			Sdr_WorldObj src_rec;
			long   numrecs = 0;
			p_impexp->GetNumRecs(&numrecs);
			IterCounter cntr;
			cntr.Init(numrecs);
			BExtInsert bei(P_TempTbl);
			while((r = p_impexp->ReadRecord(&src_rec, sizeof(src_rec))) > 0) {
				char status_text[64];
				TempKLADRTbl::Rec rec;
				// @v10.6.4 MEMSZERO(rec);
				(temp_buf = src_rec.Name).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(rec.Name, temp_buf);
				(temp_buf = src_rec.StatusAbbr).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(status_text, temp_buf);
				SearchStatus(status_text, 1, &rec.StatusCode);
				STRNSCPY(rec.Code, src_rec.Code);
				STRNSCPY(rec.ZIP, src_rec.ZIP);
				THROW_DB(bei.insert(&rec));
				PPWaitPercent(cntr.Increment(), wait_msg);
			}
			THROW(r);
			THROW_DB(bei.flash());
		}
		THROW(ProcessTempTable());
	}
	CATCHZOK
	delete p_impexp;
	PPWait(0);
	return ok;
}

int SLAPI PrcssrImportKLADR::Import()
{
	int    ok = 1;
	PPID   native_country_id = 0;
	SString temp_buf, code_buf;
	StringSet parent_list;
	PPWait(1);
	THROW(WObj.GetNativeCountry(&native_country_id) > 0);
	StatusList.sort(CMPF_LONG);
	if(P.ParentCodeString.NotEmptyS()) {
		StringSet ss_src(',', P.ParentCodeString);
		for(uint i = 0; ss_src.get(&i, temp_buf);) {
			if(temp_buf.NotEmptyS())
				parent_list.add(temp_buf);
		}
	}
	{
		IterCounter cntr;
		TempKLADRTbl::Key1 k1, k1_;
		DBQ * dbq = 0;
		BExtQuery q(P_TempTbl, 1);
		{
			PPTransaction tra(1);
			THROW(tra);
			if(!(P.Flags & Param::fStreet))
				dbq = &(*dbq && P_TempTbl->StatusCode < 500L);
			q.selectAll().where(*dbq);
			MEMSZERO(k1);
			k1_ = k1;
			cntr.Init(q.countIterations(0, &k1_, spFirst));
			for(q.initIteration(0, &k1, spFirst); q.nextIteration() > 0;) {
				const TempKLADRTbl::Rec kladr = P_TempTbl->data;
				int do_process = 0;
				code_buf = kladr.Code;
				for(uint i = 0; !do_process && parent_list.get(&i, temp_buf);) {
					if(code_buf.CmpPrefix(temp_buf, 0) == 0) {
						if(!(P.Flags & Param::fSkipSmallItems))
							do_process = 1;
						else {
							KLADRCODE kc;
							kc.FromStr(code_buf, temp_buf.Z());
							if(kc.Cit == 0)
								do_process = 1;
							else
								break;
						}
					}
				}
				if(do_process) {
					PPID   id = 0;
					uint   status_pos = 0;
					long   status_code = kladr.StatusCode;
					PPWorldPacket pack;
					if(StatusList.bsearch(&status_code, &status_pos, CMPF_LONG)) {
						long kind = 0;
						Status * p_status = (Status *)StatusList.at(status_pos);
						if(code_buf.Len() > 13)
							kind = WORLDOBJ_STREET;
						else if(status_code == 103)
							kind = WORLDOBJ_CITY;
						else if(status_code < 300)
							kind = WORLDOBJ_REGION;
						else if(status_code < 500)
							kind = WORLDOBJ_CITY;
						else
							kind = WORLDOBJ_STREET;
						if(p_status->ID == 0) {
							THROW(WosObj.AddSimple(&p_status->ID, p_status->Name, p_status->Abbr, kind, p_status->Code, 0));
						}
						pack.Rec.Kind      = kind;
						pack.Rec.CountryID = native_country_id;
						if(kladr.ParentID) {
							TempKLADRTbl::Rec prec;
							if(SearchByID(P_TempTbl, 0, kladr.ParentID, &prec) > 0) {
								WorldTbl::Rec pwrec;
								if(WObj.SearchByCode(prec.Code, &pwrec) > 0)
									pack.Rec.ParentID = pwrec.ID;
							}
						}
						SETIFZ(pack.Rec.ParentID, native_country_id);
						pack.Rec.Status = p_status->ID;
						STRNSCPY(pack.Rec.Name, kladr.Name);
						STRNSCPY(pack.Rec.Code, kladr.Code);
						STRNSCPY(pack.Rec.ZIP,  kladr.ZIP);
						int    r = WObj.SearchMaxLike(&pack, PPObjWorld::smlCode|PPObjWorld::smlName|PPObjWorld::smlCheckCountry, &id);
						THROW(r);
						if(r < 0) {
							THROW(WObj.PutPacket(&(id = 0), &pack, 0));
						}
						else {
							PPWorldPacket ex_pack;
							THROW(WObj.GetPacket(id, &ex_pack) > 0);
							ex_pack.Rec.ParentID = pack.Rec.ParentID;
							ex_pack.Rec.Status   = pack.Rec.Status;
							STRNSCPY(ex_pack.Rec.ZIP, pack.Rec.ZIP);
							STRNSCPY(ex_pack.Rec.Code, pack.Rec.Code);
							THROW(WObj.PutPacket(&id, &ex_pack, 0));
						}
					}
				}
				PPWaitPercent(cntr.Increment());
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PrcssrImportKLADR::Run()
{
	int    ok = 1;
	P.Flags = 0;
	P.ParentCodeString = 0;
	THROW(PreImport());
	THROW(EditParam(&P));
	THROW(Import());
	CATCHZOKPPERR
	return ok;
}

int SLAPI ImportKLADR()
{
	PrcssrImportKLADR prcssr;
	return prcssr.Run();
}
//
// PersonImport
//
IMPLEMENT_IMPEXP_HDL_FACTORY(PERSON, PPPersonImpExpParam);

PPPersonImpExpParam::PPPersonImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags),
	DefKindID(0), DefCategoryID(0), DefCityID(0), SrchRegTypeID(0), Flags(0)
{
}

#define IMPEXPPARAM_PERSON_DEFKINDID     1
#define IMPEXPPARAM_PERSON_DEFCITYID     2
#define IMPEXPPARAM_PERSON_SRCHREGTYPEID 3
#define IMPEXPPARAM_PERSON_FLAGS         4
#define IMPEXPPARAM_PERSON_DEFCATEGORYID 5

struct IeCfgParamItem {
	uint   Id;
	const  char * P_Txt;
};

static IeCfgParamItem PersonIeCfgParamList[] = {
	{ IMPEXPPARAM_PERSON_DEFKINDID, "DefKindID" },
	{ IMPEXPPARAM_PERSON_DEFCITYID, "DefCityID" },
	{ IMPEXPPARAM_PERSON_SRCHREGTYPEID, "SrchRegTypeID" },
	{ IMPEXPPARAM_PERSON_FLAGS, "Flags" }
};

int PPPersonImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(DefKindID)
			param_list.Add(IMPEXPPARAM_PERSON_DEFKINDID, temp_buf.Z().Cat(DefKindID));
		if(DefCategoryID)
			param_list.Add(IMPEXPPARAM_PERSON_DEFCATEGORYID, temp_buf.Z().Cat(DefCategoryID));
		if(DefCityID)
			param_list.Add(IMPEXPPARAM_PERSON_DEFCITYID, temp_buf.Z().Cat(DefCityID));
		if(SrchRegTypeID)
			param_list.Add(IMPEXPPARAM_PERSON_SRCHREGTYPEID, temp_buf.Z().Cat(SrchRegTypeID));
		if(Flags)
			param_list.Add(IMPEXPPARAM_PERSON_FLAGS, temp_buf.Z().Cat(Flags));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		DefKindID = 0;
		DefCategoryID = 0;
		DefCityID = 0;
		SrchRegTypeID = 0;
		Flags = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case IMPEXPPARAM_PERSON_DEFKINDID: DefKindID = temp_buf.ToLong(); break;
				case IMPEXPPARAM_PERSON_DEFCATEGORYID: DefCategoryID = temp_buf.ToLong(); break;
				case IMPEXPPARAM_PERSON_DEFCITYID: DefCityID = temp_buf.ToLong(); break;
				case IMPEXPPARAM_PERSON_SRCHREGTYPEID: SrchRegTypeID = temp_buf.ToLong(); break;
				case IMPEXPPARAM_PERSON_FLAGS: Flags = temp_buf.ToLong(); break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPPersonImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	SString param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	if(Direction != 0) {
		for(uint i = 0; i < SIZEOFARRAY(PersonIeCfgParamList); i++) {
			IeCfgParamItem & r_item = PersonIeCfgParamList[i];
			switch(r_item.Id) {
				case IMPEXPPARAM_PERSON_DEFKINDID:
					if(DefKindID)
						pFile->AppendParam(pSect, r_item.P_Txt, param_val.Z().Cat(DefKindID), 1);
					break;
				case IMPEXPPARAM_PERSON_DEFCATEGORYID:
					if(DefKindID)
						pFile->AppendParam(pSect, r_item.P_Txt, param_val.Z().Cat(DefCategoryID), 1);
					break;
				case IMPEXPPARAM_PERSON_DEFCITYID:
					if(DefCityID)
						pFile->AppendParam(pSect, r_item.P_Txt, param_val.Z().Cat(DefCityID), 1);
					break;
				case IMPEXPPARAM_PERSON_SRCHREGTYPEID:
					if(SrchRegTypeID)
						pFile->AppendParam(pSect, r_item.P_Txt, param_val.Z().Cat(SrchRegTypeID), 1);
					break;
				case IMPEXPPARAM_PERSON_FLAGS:
					if(Flags)
						pFile->AppendParam(pSect, r_item.P_Txt, param_val.Z().Cat(Flags), 1);
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPPersonImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	DefKindID = 0;
	DefCityID = 0;
	SrchRegTypeID = 0;
	Flags = 0;
	RVALUEPTR(excl, pExclParamList);
	for(uint i = 0; i < SIZEOFARRAY(PersonIeCfgParamList); i++) {
		IeCfgParamItem & r_item = PersonIeCfgParamList[i];
		excl.add(r_item.P_Txt);
		if(pFile->GetParam(pSect, r_item.P_Txt, param_val) > 0) {
			switch(r_item.Id) {
				case IMPEXPPARAM_PERSON_DEFKINDID: DefKindID = param_val.ToLong(); break;
				case IMPEXPPARAM_PERSON_DEFCATEGORYID: DefCategoryID = param_val.ToLong(); break;
				case IMPEXPPARAM_PERSON_DEFCITYID: DefCityID = param_val.ToLong(); break;
				case IMPEXPPARAM_PERSON_SRCHREGTYPEID: SrchRegTypeID = param_val.ToLong(); break;
				case IMPEXPPARAM_PERSON_FLAGS: Flags = param_val.ToLong(); break;
			}
		}
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}
//
//
//
PersonImpExpDialog::PersonImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPPERSON, 0)
{
}

int PersonImpExpDialog::setDTS(const PPPersonImpExpParam * pData)
{
	int    ok = 1;
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);
	{
		SetupPPObjCombo(this, CTLSEL_IMPEXPPSN_KIND, PPOBJ_PRSNKIND,     Data.DefKindID, 0);
		SetupPPObjCombo(this, CTLSEL_IMPEXPPSN_CAT,  PPOBJ_PRSNCATEGORY, Data.DefCategoryID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_IMPEXPPSN_SREG, PPOBJ_REGISTERTYPE, Data.SrchRegTypeID, 0);
		SetupPPObjCombo(this, CTLSEL_IMPEXPPSN_CITY, PPOBJ_WORLD, Data.DefCityID, 0, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
		AddClusterAssoc(CTL_IMPEXPPSN_FLAGS, 0, PPPersonImpExpParam::f2GIS);
		AddClusterAssoc(CTL_IMPEXPPSN_FLAGS, 1, PPPersonImpExpParam::fCodeToHex);
		SetClusterData(CTL_IMPEXPPSN_FLAGS, Data.Flags);
	}
	return ok;
}

int PersonImpExpDialog::getDTS(PPPersonImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	if(Data.Direction != 0) {
		getCtrlData(CTLSEL_IMPEXPPSN_KIND, &Data.DefKindID);
		getCtrlData(CTLSEL_IMPEXPPSN_CAT,  &Data.DefCategoryID);
		getCtrlData(CTLSEL_IMPEXPPSN_SREG, &Data.SrchRegTypeID);
		getCtrlData(CTLSEL_IMPEXPPSN_CITY, &Data.DefCityID);
		GetClusterData(CTL_IMPEXPPSN_FLAGS, &Data.Flags);
	}
	else {
		Data.DefKindID = 0;
		Data.SrchRegTypeID = 0;
		Data.DefCityID = 0;
		Data.Flags = 0;
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

class PrcssrPersonImport {
public:
	struct Param {
		SLAPI  Param() : CategoryID(0)
		{
		}
		PPID   CategoryID;
		SString CfgName;
	};
	SLAPI  PrcssrPersonImport();
	int    SLAPI InitParam(Param *);
	int    SLAPI EditParam(Param *);
	int    SLAPI Init(const Param *);
	int    SLAPI Run();
private:
	struct AddrEntry {
		double Longitude;
		double Latitude;
		SString Code;
		SString ZIP;
		SString CityName;
		SString SubCityName; // @v8.2.3
		SString Addr;
	};
	int    SLAPI ResolveAddr(AddrEntry & rEntry, LocationTbl::Rec & rAddr);
	int    SLAPI ProcessComplexELinkText(const char * pText, PPPersonPacket * pPack);
	int    SLAPI ProcessComplexEMailText(const char * pText, PPPersonPacket * pPack);

	PPObjPerson PsnObj;
	PPObjWorld WObj;
	Param  P;
	PPPersonImpExpParam IeParam;
};

SLAPI PrcssrPersonImport::PrcssrPersonImport()
{
}

int SLAPI PrcssrPersonImport::InitParam(Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		pParam->CfgName = 0;
		ok = 1;
	}
	return ok;
}

class PersonImportDialog : public TDialog {
public:
	PersonImportDialog() : TDialog(DLG_IEPERSON)
	{
		PPPersonImpExpParam param;
		GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_PERSON, &param, &CfgList, 2 /* import */);
	}
	int    setDTS(const PrcssrPersonImport::Param * pData);
	int    getDTS(PrcssrPersonImport::Param * pData);
private:
	PrcssrPersonImport::Param Data;
	StrAssocArray CfgList;
};

int PersonImportDialog::setDTS(const PrcssrPersonImport::Param * pData)
{
	Data = *pData;
	int    ok = 1;
	long   cfg_id = 0;
	if(CfgList.getCount() == 1)
		cfg_id = CfgList.Get(0).Id;
	SetupStrAssocCombo(this, CTLSEL_IEPERSON_CFG, &CfgList, cfg_id, 0, 0, 0);
	SetupPPObjCombo(this, CTLSEL_IEPERSON_CAT, PPOBJ_PRSNCATEGORY, Data.CategoryID, OLW_CANINSERT, 0);
	return ok;
}

int PersonImportDialog::getDTS(PrcssrPersonImport::Param * pData)
{
	int    ok = 1;
	uint   sel = 0;
	long   cfg_id = getCtrlLong(sel = CTLSEL_IEPERSON_CFG);
	SString temp_buf;
	THROW_PP(cfg_id, PPERR_CFGNEEDED);
	THROW_PP(CfgList.GetText(cfg_id, Data.CfgName) > 0, PPERR_CFGNEEDED);
	getCtrlData(CTLSEL_IEPERSON_CAT, &Data.CategoryID);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int PrcssrPersonImport::EditParam(Param * pParam) { DIALOG_PROC_BODY(PersonImportDialog, pParam); }

int PrcssrPersonImport::Init(const Param * pParam)
{
	P = *pParam;
	int    ok = 1;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_PERSON, &IeParam.InrRec));
		IeParam.Direction = 1; // import
		THROW(IeParam.ProcessName(1, P.CfgName));
		THROW(IeParam.ReadIni(&ini_file, P.CfgName, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPersonImport::ResolveAddr(AddrEntry & rEntry, LocationTbl::Rec & rAddr)
{
	int    ok = -1;
	rEntry.Code.Strip();
	rEntry.ZIP.Strip();
	rEntry.Addr.Strip();
	MEMSZERO(rAddr);
	SString temp_buf = rEntry.CityName;
	if(temp_buf.NotEmptyS()) {
		temp_buf.ShiftLeftChr('г').Strip().ShiftLeftChr('.').Strip();
		THROW(WObj.AddSimple(&rAddr.CityID, WORLDOBJ_CITY, temp_buf, 0, 0));
	}
	else
		rAddr.CityID = IeParam.DefCityID;
	rAddr.Latitude = rEntry.Latitude;
	rAddr.Longitude = rEntry.Longitude;
	rEntry.Code.CopyTo(rAddr.Code, sizeof(rAddr.Code));
	LocationCore::SetExField(&rAddr, LOCEXSTR_ZIP, rEntry.ZIP);
	LocationCore::SetExField(&rAddr, LOCEXSTR_SHORTADDR, rEntry.Addr);
	if(!LocationCore::IsEmptyAddressRec(rAddr))
		ok = 1;
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPersonImport::ProcessComplexELinkText(const char * pText, PPPersonPacket * pPack)
{
	struct PrefixEntry {
		const char * P_Prefix;
		int    ELinkType;
	};
	PrefixEntry pe[] = {
		{ "телефон", ELNKRT_PHONE },
		{ "тел", ELNKRT_PHONE },
		{ "phone", ELNKRT_PHONE },
		{ "fax", ELNKRT_FAX },
		{ "факс", ELNKRT_FAX },
		{ "телефакс", ELNKRT_FAX },
		{ "website", ELNKRT_WEBADDR },
		{ "site", ELNKRT_WEBADDR },
		{ "http", ELNKRT_WEBADDR },
		{ "сайт", ELNKRT_WEBADDR },
		{ "email", ELNKRT_EMAIL },
		{ "e-mail", ELNKRT_EMAIL },
		{ "email", ELNKRT_EMAIL },
		{ "mail", ELNKRT_EMAIL }
	};
	int    ok = 1;
	SString temp_buf;
	SStrScan scan(pText);
	do {
		scan.Skip();
		size_t sc_offs = scan.Offs;
		int    elinktype = 0;
		for(uint i = 0; !elinktype && i < SIZEOFARRAY(pe); i++) {
			(temp_buf = pe[i].P_Prefix).Transf(CTRANSF_OUTER_TO_INNER);
			if(strnicmp866(temp_buf, scan, temp_buf.Len()) == 0) {
				elinktype = pe[i].ELinkType;
				scan.Incr(temp_buf.Len());
			}
		}
		if(elinktype) {
			scan.Skip();
			scan.IncrChr('.');
			scan.Skip();
			scan.IncrChr(':');
			scan.Skip();
			temp_buf.Z();
			while(scan[0] != ';' && scan[0] != ',' && scan[0] != 0) {
				temp_buf.CatChar(scan[0]);
				scan.Incr();
			}
			if(scan[0])
				scan.Incr();
			if(temp_buf.NotEmptyS()) {
				if(elinktype == ELNKRT_PHONE) {
					if(!pPack->ELA.SearchByText(temp_buf, 0))
						pPack->ELA.AddItem(PPELK_WORKPHONE, temp_buf);
				}
				else if(elinktype == ELNKRT_FAX) {
					if(!pPack->ELA.SearchByText(temp_buf, 0))
						pPack->ELA.AddItem(PPELK_FAX, temp_buf);
				}
				else if(elinktype == ELNKRT_EMAIL) {
					if(!pPack->ELA.SearchByText(temp_buf, 0))
						pPack->ELA.AddItem(PPELK_EMAIL, temp_buf);
				}
				else if(elinktype == ELNKRT_WEBADDR) {
					if(IeParam.Flags & PPPersonImpExpParam::f2GIS) {
						if(temp_buf.CmpPrefix("http://link.2gis.ru/", 1) == 0) {
							size_t p_ = 0;
							if(temp_buf.SearchChar('?', &p_)) {
								temp_buf.ShiftLeft(p_+1);
							}
						}
					}
					if(temp_buf.NotEmptyS())
						if(!pPack->ELA.SearchByText(temp_buf, 0))
							pPack->ELA.AddItem(PPELK_WWW, temp_buf);
				}
			}
		}
		if(scan.Offs == sc_offs && scan[0])
			scan.Incr();
	} while(scan[0]);
	return ok;
}

int SLAPI PrcssrPersonImport::ProcessComplexEMailText(const char * pText, PPPersonPacket * pPack)
{
	int    ok = 1;
	SString temp_buf;
	SStrScan scan(pText);
	do {
		scan.Skip();
		if(scan.GetEMail(temp_buf.Z())) {
			if(!pPack->ELA.SearchByText(temp_buf, 0))
				pPack->ELA.AddItem(PPELK_EMAIL, temp_buf);
			scan.Skip();
			while(oneof2(scan[0], ',', ';')) {
				scan.Incr();
				scan.Skip();
			}
		}
		else
			break;
	} while(scan[0]);
	return ok;
}

static uint FASTCALL MakeTextHash(const SString & rText)
{
	return SlHash::BobJenc(rText.cptr(), rText.Len());
}

int SLAPI PrcssrPersonImport::Run()
{
	int    ok = -1;
	long   numrecs = 0;
	SString log_msg, fmt_buf, temp_buf, temp_buf2, dblgis_code;
	SString name;
	PPLogger logger;
	PPObjPersonCat pc_obj;
	PPObjPersonKind pk_obj;
	PPObjPersonStatus ps_obj;
	PPImpExp ie(&IeParam, 0);
	PPWait(1);
	THROW(ie.OpenFileForReading(0));
	THROW(ie.GetNumRecs(&numrecs));
	if(numrecs) {
		int    r;
		long   accepted_count = 0;
		IterCounter cntr;
		Sdr_Person rec;
		PPTransaction tra(1);
		THROW(tra);
		cntr.Init(numrecs);
		MEMSZERO(rec);
		while((r = ie.ReadRecord(&rec, sizeof(rec))) > 0) {
			IeParam.InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &rec);
			int    do_turn = 1; // Импортированный пакет следует сохранить в БД
			int    is_office = 0;
			PPID   psn_id = 0, ps_id = 0;
			int64  code64 = 0;
			PPPersonPacket pack;
			name = rec.Name;
			if(!name.NotEmptyS()) {
				name = rec.ExtName;
			}
			if(name.NotEmptyS()) {
				if(IeParam.Flags & PPPersonImpExpParam::f2GIS) {
#if 0 // {
					//
					// Для наименований импортированных из 2GIS суффикс ",офис"
					// удаляем и далее трактуем эту запись как офисное подразделение торговой сети.
					//
					(temp_buf2 = "офис").ToOem();
					if(name.CmpSuffix(temp_buf2, 1) == 0) {
						(temp_buf = name).Trim(name.Len()-temp_buf2.Len()).Strip();
						if(temp_buf.Last() == ',') {
							temp_buf.TrimRight().Strip();
							name = temp_buf;
							is_office = 1;
						}
					}
#endif // } 0
				}
				name.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				temp_buf = rec.ExtName;
				if(temp_buf.NotEmptyS() && temp_buf.CmpNC(pack.Rec.Name) != 0)
					pack.SetExtName(temp_buf);
				{
					PPID   ps_id = 0;
					PPPersonStatus ps_rec;
					if(rec.StatusCode[0] && ps_obj.SearchBySymb(rec.StatusCode, &ps_id, &ps_rec) > 0) {
					}
					else if(rec.StatusName[0] && ps_obj.SearchByName(rec.StatusName, &ps_id, &ps_rec) > 0) {
					}
					else
						ps_id = 0;
					pack.Rec.Status = NZOR(ps_id, PPPRS_LEGAL);
				}
				{
					PPID   kind_id = 0;
					PPPersonKind pk_rec;
					if(rec.KindCode[0] && pk_obj.SearchBySymb(rec.KindCode, &kind_id, &pk_rec) > 0) {
					}
					else if(rec.KindName[0] && pk_obj.SearchByName(rec.KindName, &kind_id, &pk_rec) > 0) {
					}
					else
						kind_id = 0;
					SETIFZ(kind_id, IeParam.DefKindID);
					SETIFZ(kind_id, PPPRK_UNKNOWN);
					pack.Kinds.addUnique(kind_id);
				}
				{
					PPID   cat_id = 0;
					PPPersonCat cat_rec;
					if(rec.CategoryCode[0] && pc_obj.SearchBySymb(rec.CategoryCode, &cat_id, &cat_rec) > 0) {
					}
					else if(rec.CategoryName[0] && pc_obj.SearchByName(rec.CategoryName, &cat_id, &cat_rec) > 0) {
					}
					else
						cat_id = 0;
					if(cat_id)
						pack.Rec.CatID = cat_id;
				}
				{
					dblgis_code = 0;
					if(rec.DblGisCode && rec.MainAddrCityName[0]) {
						(temp_buf = rec.MainAddrCityName).Strip().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
						uint city_hash = MakeTextHash(temp_buf);
						dblgis_code.Cat(city_hash).CatChar('-').Cat(rec.DblGisCode);
					}
				}
				if(IeParam.SrchRegTypeID) {
					(temp_buf = rec.Code).Strip();
					temp_buf.SetIfEmpty(dblgis_code);
					if(IeParam.Flags & IeParam.fCodeToHex) {
						PsnObj.AddRegisterToPacket(pack, IeParam.SrchRegTypeID, CodeToHex(temp_buf), 0);
					}
					else if(dblgis_code.NotEmpty() && temp_buf == dblgis_code) {
						PsnObj.AddRegisterToPacket(pack, IeParam.SrchRegTypeID, temp_buf, 0);
					}
					else {
						code64 = temp_buf.Strip().ToInt64();
						if(code64) {
							temp_buf.Z();
							temp_buf.Cat(code64);
							PsnObj.AddRegisterToPacket(pack, IeParam.SrchRegTypeID, temp_buf, 0);
						}
						else if(temp_buf.NotEmpty()) {
							PsnObj.AddRegisterToPacket(pack, IeParam.SrchRegTypeID, temp_buf, 0);
						}
					}
				}
				THROW(PsnObj.AddRegisterToPacket(pack, PPREGT_TPID, (temp_buf = rec.INN).Strip(), 0));
				THROW(PsnObj.AddRegisterToPacket(pack, PPREGT_OKONH, (temp_buf = rec.OKONH).Strip(), 0));
				THROW(PsnObj.AddRegisterToPacket(pack, PPREGT_OKPO, (temp_buf = rec.OKPO).Strip(), 0));
				THROW(PsnObj.AddRegisterToPacket(pack, PPREGT_KPP, (temp_buf = rec.KPP).Strip(), 0));
				{
					const long sml_flags = (PsnObj.GetConfig().Flags & PPPersonConfig::fSyncByName) ? 0 : PPObjPerson::smlRegisterOnly;
					if(PsnObj.SearchMaxLike(&pack, &psn_id, sml_flags, NZOR(IeParam.SrchRegTypeID, PPREGT_TPID)) > 0) {
						PPPersonPacket inner_pack;
						THROW(PsnObj.GetPacket(psn_id, &inner_pack, 0) > 0);
						pack = inner_pack;
						do_turn = 0;
					}
					else
						psn_id = 0;
					{
						AddrEntry addr_entry;
						PPLocationPacket addr_pack;
						if(pack.Loc.IsEmptyAddress()) {
							addr_entry.Latitude = 0.0;
							addr_entry.Longitude = 0.0;
							addr_entry.Code = rec.MainAddrCode;
							addr_entry.CityName = rec.MainAddrCityName;
							addr_entry.ZIP = rec.MainAddrZIP;
							addr_entry.Addr = rec.MainAddrText;
							if(ResolveAddr(addr_entry, addr_pack) > 0) {
								pack.Loc = addr_pack;
								do_turn = 1;
							}
						}
						if(pack.RLoc.IsEmptyAddress()) {
							addr_entry.Latitude = 0.0;
							addr_entry.Longitude = 0.0;
							addr_entry.Code = rec.RAddrCode;
							addr_entry.CityName = rec.RAddrCityName;
							addr_entry.ZIP = rec.RAddrZIP;
							addr_entry.Addr = rec.RAddrText;
							if(ResolveAddr(addr_entry, addr_pack) > 0) {
								pack.RLoc = addr_pack;
								do_turn = 1;
							}
						}
						{
							addr_entry.Latitude = rec.DlvrAddrLatitude;
							addr_entry.Longitude = rec.DlvrAddrLongitude;
							addr_entry.Code = rec.DlvrAddrCode;
							addr_entry.CityName = rec.DlvrAddrCityName;
							addr_entry.ZIP = rec.DlvrAddrZIP;
							addr_entry.Addr = rec.DlvrAddrText;
							if(ResolveAddr(addr_entry, addr_pack) > 0) {
								//
								// Так как пакет может быть извлечен из БД, проверяем нет ли в нем уже импортируемого адреса
								//
								int    addr_found = 0;
								PPLocationPacket dlvr_loc;
								for(uint i = 0; !addr_found && pack.EnumDlvrLoc(&i, &dlvr_loc);) {
									if(dlvr_loc.CityID != addr_pack.CityID)
										continue;
									else if(strcmp(dlvr_loc.Code, addr_pack.Code) != 0)
										continue;
									else if(!feqeps(dlvr_loc.Latitude, addr_pack.Latitude, 1.0E-14))
										continue;
									else if(!feqeps(dlvr_loc.Longitude, addr_pack.Longitude, 1.0E-14))
										continue;
									else {
										LocationCore::GetExField(&dlvr_loc, LOCEXSTR_ZIP, temp_buf);
										LocationCore::GetExField(&addr_pack, LOCEXSTR_ZIP, temp_buf2);
										if(temp_buf != temp_buf2)
											continue;
										else {
											LocationCore::GetExField(&dlvr_loc, LOCEXSTR_SHORTADDR, temp_buf);
											LocationCore::GetExField(&addr_pack, LOCEXSTR_SHORTADDR, temp_buf2);
											if(temp_buf != temp_buf2)
												continue;
											else
												addr_found = 1;
										}
									}
								}
								if(!addr_found) {
									if(is_office && pack.RLoc.IsEmptyAddress())
										pack.RLoc = addr_pack;
									else
										pack.AddDlvrLoc(addr_pack);
									do_turn = 1;
								}
							}
						}
					}
				}
				// @v9.8.0 {
				if(checkdate(rec.DOB)) {
					const ObjTagItem * p_tag_item = pack.TagL.GetItem(PPTAG_PERSON_DOB);
					LDATE   ex_dob = ZERODATE;
					if(!p_tag_item || !p_tag_item->GetDate(&ex_dob) || !checkdate(ex_dob)) {
						ObjTagItem new_dob_tag_item;
						if(new_dob_tag_item.SetDate(PPTAG_PERSON_DOB, rec.DOB) && pack.TagL.PutItem(PPTAG_PERSON_DOB, &new_dob_tag_item)) {
							do_turn = 1;
						}
					}
				}
				// } @v9.8.0
				{
					if((temp_buf = rec.Phone).NotEmptyS() && !pack.ELA.SearchByText(temp_buf, 0))
						pack.ELA.AddItem(PPELK_WORKPHONE, temp_buf);
					if((temp_buf = rec.Phone2).NotEmptyS() && !pack.ELA.SearchByText(temp_buf, 0))
						pack.ELA.AddItem(PPELK_ALTPHONE, temp_buf);
					if((temp_buf = rec.PhoneMobile).NotEmptyS() && !pack.ELA.SearchByText(temp_buf, 0))
						pack.ELA.AddItem(PPELK_MOBILE, temp_buf);
					if((temp_buf = rec.EMail).NotEmptyS())
						ProcessComplexEMailText(temp_buf, &pack);
					if((temp_buf = rec.HTTP).NotEmptyS() && !pack.ELA.SearchByText(temp_buf, 0))
						pack.ELA.AddItem(PPELK_WWW, temp_buf);
					if(rec.ComplELink[0])
						ProcessComplexELinkText(rec.ComplELink, &pack);
				}
				if(!pack.Rec.ID) {
					if(rec.VatFree > 0) {
						pack.Rec.Flags |= PSNF_NOVATAX;
					}
					if(!pack.Rec.CatID) {
						PPPersonCat pc_rec;
						if(P.CategoryID && pc_obj.Fetch(P.CategoryID, &pc_rec) > 0)
							pack.Rec.CatID = P.CategoryID;
						else if(IeParam.DefCategoryID && pc_obj.Fetch(IeParam.DefCategoryID, &pc_rec) > 0)
							pack.Rec.CatID = IeParam.DefCategoryID;
					}
					(temp_buf = rec.Memo).Strip().CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
					{
						SString accno;
						PPBank bnk_rec;
						(temp_buf = rec.BankName).Strip().CopyTo(bnk_rec.Name, sizeof(bnk_rec.Name));
						(temp_buf = rec.BIC).Strip().CopyTo(bnk_rec.BIC, sizeof(bnk_rec.BIC));
						(temp_buf = rec.CorrAcc).Strip().CopyTo(bnk_rec.CorrAcc, sizeof(bnk_rec.CorrAcc));
						accno = rec.BankAcc;
						if(bnk_rec.Name[0] && accno.NotEmptyS()) {
							PPID   bnk_id = 0;
							PPBankAccount ba;
							THROW(PsnObj.AddBankSimple(&bnk_id, &bnk_rec, 0));
							ba.BankID = bnk_id;
							ba.AccType = PPBAC_CURRENT;
							STRNSCPY(ba.Acct, accno);
							THROW(pack.Regs.SetBankAccount(&ba, static_cast<uint>(-1)));
						}
					}
				}
				if(do_turn) {
					THROW(PsnObj.PutPacket(&psn_id, &pack, 0));
					accepted_count++;
					if((accepted_count % 10000) == 0) {
						THROW(tra.Commit());
						THROW(tra.Start(1));
					}
				}
			}
			MEMSZERO(rec);
			PPWaitPercent(cntr.Increment());
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	PPWait(0);
	return ok;
}

int SLAPI PPObjPerson::Import()
{
	int    ok = -1;
	PrcssrPersonImport prcssr;
	PrcssrPersonImport::Param param;
	prcssr.InitParam(&param);
	while(ok < 0 && prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			PPError();
	return ok;
}

int SLAPI EditPersonImpExpParams()
{
	int    ok = -1;
	PPPersonImpExpParam param;
	ImpExpParamDialog * p_dlg = new ImpExpParamDialog(DLG_IMPEXP, 0);
	THROW(CheckDialogPtr(&p_dlg));
	THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_PERSON, &param, p_dlg));
	CATCHZOK
	delete p_dlg;
	return ok;
}
//
// @vmiller {
//
// Вспомогательный класс для ненастраиваемого импорта для продуктовой базы
// "Composition of Foods Raw, Processed, Prepared USDA National Nutrient Database for Standard Reference, Release 25"
// или коротко SR25
//
// Формат хранения данных в базе RS25:
// 1) все поля разделены символом ^
// 2) для текстовых полей дополнительно с двух сторон ставится символ ~ (хотя похоже, что не только для текстовых)
//
struct FoodDescr {
	void Clear()
	{
		THISZERO();
	}
	long  ID;           // ИД продукта/блюда в БД
	char  Descr[200];	// Описание блюда, название
};

struct NutrDescr {
	void Clear()
	{
		THISZERO();
	}
	long  ID;        // ИД компонента в БД
	char  Unit[7];   // Единицы измерения
	char  Descr[60]; // Название компонента
};

struct FoodWithNutr {
	void Clear()
	{
		THISZERO();
	}
	PPID   FoodID;
	PPID   NutrID;
	char   Units[7]; // Единицы измерения компонентов
	double Qtty;     // Содержание в 100 граммах продукта
};

struct PPComps {
	void Clear()
	{
		THISZERO();
	}
	PPID   CompID;       // В файле импорта это ИД элемента, а в Papyrus это будет штрихкод
	char   CompName[128];
	double Qtty;
	char   UnitAbbr[20]; // Символьное обозначение единицы измерения
};
//
// Ненастравиваемый импорт базы SR25 с продуктами и их составом
//
class ReadDBSR25 {
public:
	ReadDBSR25()
	{
	}
	int    InitIteration();
	int    FASTCALL NextIteration(PPSuprWareAssoc & rSuprWare);
	int    GetNutrArr(TSArray <PPComps> & rArr);
	int    GetFoodArr(StrAssocArray & rArr);

	IterCounter Cntr;
private:
	SFile  NutrGoodsFile;
};

int ReadDBSR25::GetNutrArr(TSArray <PPComps> & rArr)
{
	int    ok = 1;
	SFile  file;
	SString str, file_path;
	SString left, right;
	NutrDescr nutr;
	PPComps comp;

	PPGetFilePath(PPPATH_IN, "NUTR_DEF.txt", file_path);
	THROW_SL(file.Open(file_path, SFile::mRead));
	rArr.freeAll();
	while(file.ReadLine(str)) {
		MEMSZERO(comp);
		{
			nutr.Clear();
			for(uint pos = 0; (pos < 4) && (str.Divide('^', left, right) > 0);) {
				if(left.Len()) {
					// Обрезаем символ ~ с правой и с левой стороны строки, если он есть
					left.ShiftLeftChr('~');
					left.TrimRightChr('~');
					pos++;
					switch(pos) {
						case 1: nutr.ID = left.ToLong(); break; // ИД компонента
						case 2: left.CopyTo(nutr.Unit, sizeof(nutr.Unit)); break; // Единицы измерения
						case 4: left.CopyTo(nutr.Descr, sizeof(nutr.Descr)); break; // Название компонента
					}
				}
				str = right;
			}
		}
		comp.CompID = nutr.ID;
		STRNSCPY(comp.CompName, nutr.Descr);
		STRNSCPY(comp.UnitAbbr, nutr.Unit);
		THROW_SL(rArr.insert(&comp));
	}
	CATCHZOK
	return ok;
}

int ReadDBSR25::GetFoodArr(StrAssocArray & rArr)
{
	int    ok = 1;
	SFile  file;
	SString str, file_path;
	SString left, right;
	FoodDescr food;

	PPGetFilePath(PPPATH_IN, "FOOD_DES.txt", file_path);
	THROW_SL(file.Open(file_path, SFile::mRead));
	rArr.Z();
	while(file.ReadLine(str)) {
		food.Clear();
		for(uint pos = 0; (pos < 3) && (str.Divide('^', left, right) > 0);) {
			if(left.Len()) {
				// Обрезаем символ ~ с правой и с левой стороны строки, если он есть
				left.ShiftLeftChr('~');
				left.TrimRightChr('~');
				pos++;
				switch(pos) {
					case 1: // ИД продукта
						food.ID = left.ToLong();
						break;
					case 3: // Название продукта
						left.CopyTo(food.Descr, sizeof(food.Descr));
						break;
				}
			}
			str = right;
		}
		THROW_SL(rArr.Add(food.ID, food.Descr));
	}
	CATCHZOK
	return ok;
}

int ReadDBSR25::InitIteration()
{
	int    ok = 1;
	long   count = 0;
	SString temp_buf;
	PPGetFilePath(PPPATH_IN, "NUT_DATA.txt", temp_buf);
	THROW_SL(NutrGoodsFile.Open(temp_buf, SFile::mRead));
	while(NutrGoodsFile.ReadLine(temp_buf)) {
		count++;
	}
	Cntr.Init(count);
	NutrGoodsFile.Seek(0);
	CATCHZOK
	return ok;
}

int FASTCALL ReadDBSR25::NextIteration(PPSuprWareAssoc & rGoodComp)
{
	int    ok = 1;
	SString str;
	SString left, right;
	FoodWithNutr food_nutr;
	MEMSZERO(rGoodComp);
	THROW_SL(NutrGoodsFile.ReadLine(str));
	{
		Cntr.Increment();
		food_nutr.Clear();
		for(uint pos = 0; (pos < 3) && (str.Divide('^', left, right) > 0);) {
			if(left.Len() && left.CmpNC("~~") != 0) {
				// Обрезаем символ ~ с правой и с левой стороны строки, если он есть
				left.ShiftLeftChr('~');
				left.TrimRightChr('~');
				pos++;
				switch(pos) {
					case 1: food_nutr.FoodID = left.ToLong(); break; // ИД продукта
					case 2: food_nutr.NutrID = left.ToLong(); break; // ИД компонента
					case 3: food_nutr.Qtty = left.ToReal(); break; // Содержание компонента в 100 граммах продукта
				}
			}
			str = right;
		}
	}
	rGoodComp.GoodsID = food_nutr.FoodID;
	rGoodComp.CompID  = food_nutr.NutrID;
	rGoodComp.Qtty    = food_nutr.Qtty;
	CATCHZOK
	return ok;
}

int SLAPI ImportSR25()
{
	int    ok = 1, r = 1;
	uint   count = 0, i = 0, pos = 0;
	SString wait_msg, brc_str, msg_buf;
	SString temp_buf;
	SString unit_abbr;
	ReadDBSR25 sr25;
	PPObjSuprWare sw_obj;
	PPSuprWarePacket sw_pack;
	PPSuprWareAssoc sw_item;
	PPObjUnit unit_obj;
	PPSuprWarePacket comp_goods_pack;
	StrAssocArray goods_arr;
	PPLogger logger;
	LAssocArray goods_ids_assoc, nutrs_ids_assoc; // Это ассоциации ИД товаров/компонентов, указанные в файле импорта, и их ИД в Papyrus
	TSArray <PPComps> nutr_arr;

	PPWait(1);
	PPLoadText(PPTXT_IMPGOODS, wait_msg);

	// Загрузим продукты
	THROW(sr25.GetFoodArr(goods_arr));
	// Загрузим компоненты
	THROW(sr25.GetNutrArr(nutr_arr));
	{
		BarcodeTbl::Rec bc_rec;
		PPTransaction tra(1);
		THROW(tra);
		for(i = 0, count = 0; i < goods_arr.getCount(); i++) {
			PPID   goods_id = 0;
			StrAssocArray::Item _item = goods_arr.at_WithoutParent(i);
			const  long   sr25_id = _item.Id;
			if(sr25_id) {
				PPID   sw_id = 0;
				brc_str.Z().Cat(sr25_id);
				if(sw_obj.SearchByBarcode(brc_str, &bc_rec) > 0) {
					sw_id = bc_rec.GoodsID;
					THROW(sw_obj.Get(sw_id, &sw_pack) > 0);
				}
				else {
					sw_pack.Init();
					(temp_buf = _item.Txt).Transf(CTRANSF_OUTER_TO_INNER);
					STRNSCPY(sw_pack.Rec.Name, temp_buf);
					brc_str.CopyTo(sw_pack.Rec.Code, sizeof(sw_pack.Rec.Code));
					sw_pack.Rec.SuprWareType = SUPRWARETYPE_GOODS;
					sw_pack.Rec.SuprWareCat = SUPRWARECLASS_NUTRITION;
					THROW(sw_obj.Put(&sw_id, &sw_pack, 0));
				}
				goods_ids_assoc.Add(sr25_id, sw_id, &(pos = 0));
			}
		}
		// @v9.2.4 PPLoadText(PPTXT_GOODS, msg_buf);
		PPLoadString("ware_pl", msg_buf); // @v9.2.4
		logger.Log(msg_buf);
		PPLoadText(PPTXT_IMPORTEDRECS, temp_buf);
		logger.Log(msg_buf.Printf(temp_buf, count, goods_arr.getCount()));
		for(i = 0, count = 0; i < nutr_arr.getCount(); i++) {
			const  PPComps & r_nutr_item = nutr_arr.at(i);
			const  long sr25_id = r_nutr_item.CompID;
			PPID   sw_id = 0;
			brc_str.Z().Cat(sr25_id);
			if(sw_obj.SearchByBarcode(brc_str, &bc_rec) > 0) {
				sw_id = bc_rec.GoodsID;
				THROW(sw_obj.Get(sw_id, &sw_pack) > 0);
			}
			else {
				sw_pack.Init();
				(temp_buf = r_nutr_item.CompName).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(sw_pack.Rec.Name, temp_buf);
				if(sw_obj.P_Tbl->SearchByName(PPGDSK_SUPRWARE, sw_pack.Rec.Name, &sw_id) > 0) {
				}
				else {
					brc_str.CopyTo(sw_pack.Rec.Code, sizeof(sw_pack.Rec.Code));
					sw_pack.Rec.SuprWareType = SUPRWARETYPE_COMPONENT;
					sw_pack.Rec.SuprWareCat = SUPRWARECLASS_NUTRITION;
					THROW(sw_obj.Put(&sw_id, &sw_pack, 0));
				}
			}
			nutrs_ids_assoc.Add(sr25_id, sw_id, &(pos = 0));
		}

		goods_ids_assoc.Sort();
		nutrs_ids_assoc.Sort();

		PPLoadText(PPTXT_COMPONENTS, temp_buf);
		logger.Log(temp_buf);
		PPLoadText(PPTXT_IMPORTEDRECS, temp_buf);
		logger.Log(msg_buf.Printf(temp_buf, count, nutr_arr.getCount()));
		//
		// Загрузим ассоциации товар - компонент
		//
		THROW(sr25.InitIteration());
		while(sr25.NextIteration(sw_item)) {
			//
			// Сейчас в sw_item в полях GoodID и CompID хранятся внутренние идентификаторы базы, используемые
			// здесь как штрихкоды. Так что теперь по этим штрихкодам надо найти ИД товаров и компонентов в Papyrus
			// Ищем соответствие в массиве ассоциаций goods_ids_assoc и nutrs_ids_assoc
			//
			brc_str.Z().Cat(sw_item.GoodsID);
			PPID   sw_id = 0;
			PPID   sw_comp_id = 0;
			if(goods_ids_assoc.BSearch(sw_item.GoodsID, &sw_id, 0)) {
				brc_str.Z().Cat(sw_item.CompID);
				uint   key_pos = 0;
				if(nutrs_ids_assoc.BSearch(sw_item.CompID, &sw_comp_id, &key_pos)) {
					//
					// Теперь в массиве nutr_arr ищем запись с таким ИД и вытаскиваем единицы измерения //
					//
					uint   comp_pos = 0;
					if(nutr_arr.lsearch(&sw_item.CompID, &comp_pos, CMPF_LONG)) {
						PPID   unit_id = 0;
						const  PPComps & r_comp = nutr_arr.at(comp_pos);
						unit_abbr = r_comp.UnitAbbr;
						if(unit_abbr.NotEmptyS()) {
							PPUnit unit_rec;
							const char micro_prefix[] = {(char)181, 0};
							uint pos = 0;
							// @v10.6.8 @ctr MEMSZERO(unit_rec);
							unit_rec.Tag = PPOBJ_UNIT;
							if((temp_buf = r_comp.UnitAbbr).HasChr(181) || temp_buf.HasChr(-75)) // Если есть приставка "микро" (проверяем по двум кодам)
								temp_buf.ReplaceStr(micro_prefix, "mk", 1);
							temp_buf.CopyTo(unit_rec.Name, sizeof(unit_rec.Name));
							temp_buf.CopyTo(unit_rec.Abbr, sizeof(unit_rec.Abbr));
							unit_rec.Flags = unit_rec.Physical;
							//
							// Сначала посмотрим, есть ли такая единица измерения
							//
							if(unit_obj.SearchMaxLike(&unit_rec, &unit_id) > 0)
								sw_item.UnitID = unit_id;
							else
								THROW(unit_obj.AddItem(&unit_id, &unit_rec, 0));
							sw_item.UnitID = unit_id;
						}
						// если единица измерения энергии - kcal, то не сохраняем. Для всех продуктов есть и kcal и kJ. Записывать будем kJ
						if(unit_abbr.CmpNC("kcal") != 0 && sw_item.Qtty != 0.0) {
							sw_item.GoodsID = sw_id;
							sw_item.CompID = sw_comp_id;
							THROW(sw_obj.PutAssoc(sw_item, 0));
						}
					}
				}
			}
			PPWaitPercent(sr25.Cntr);
		}
		THROW(tra.Commit());
	}
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	logger.Save(PPFILNAM_COMPGOODS_LOG, 0);
	PPWait(0);
	return ok;
}
//
// Ненастраиваемый импорт базы с товарами и их составом (файлы для импорта готовим сами)
//
//
// Вспомогательный класс для универсального ненастраиваемого импорта товаров и компонентов
// из текстового файла
// Формат хранения данных в файле:
//		товар1:компонент1=количество<единица_измерения>;компонент2;компонент3=количество...
//		товар2:компонент1=количество;компонент4=количество<единица_измерения>...
// Количество и единица измерения опциональны. Дробное число указывается через точку
//
class SuprWareDB {
public:
	SuprWareDB(const char * pFileName) : FileName(pFileName)
	{
	}
	int    InitIteration(const char * pFileName = 0);
	int    NextIteration(SString & rName, TSArray <PPComps> & rArr);
	const  IterCounter & GetCounter() const
	{
		return Cntr;
	}
private:
	SString FileName;
	SFile  File;
	IterCounter Cntr;
};

int SuprWareDB::InitIteration(const char * pFileName)
{
	int    ok = pFileName ? File.Open(pFileName, SFile::mRead) : File.Open(FileName, SFile::mRead);
	if(ok) {
		ulong  line_count = 0;
		SString line_buf;
		while(File.ReadLine(line_buf)) {
			line_count++;
		}
		Cntr.Init(line_count);
		File.Seek(0);
	}
	return ok;
}

int SuprWareDB::NextIteration(SString & rName, TSArray <PPComps> & rArr)
{
	int    ok = -1;
	SString str, r_str, l_str, qtty_unit_str, comp_name, qtty, unit;
	PPComps comps;
	rArr.freeAll();
	if(File.ReadLine(str)) {
		str.Divide(':', rName, r_str);
		str = r_str;
		while(str.Len()) {
			str.Divide(';', l_str, r_str);
			l_str.Divide('=', comp_name, qtty_unit_str);
			STRNSCPY(comps.CompName, comp_name.Chomp());
			qtty_unit_str.Divide('<', qtty, unit);
			comps.Qtty = qtty.ToReal();
			STRNSCPY(comps.UnitAbbr, unit.Chomp().TrimRightChr('>'));
			rArr.insert(&comps);
			ok = 1;
			str = r_str;
		}
		Cntr.Increment();
	}
	return ok;
}

int SLAPI ImportCompGS()
{
	int    ok = 1;
	uint   total_gds_count = 0, gds_count = 0, total_comps_count = 0, comps_count = 0, i = 0;
	PPLogger logger;
	SString file_name, wait_msg, msg, buf, goods_name, comp_name, unit_abbr;
	PPObjSuprWare sw_obj;
    TSArray <PPComps> comps_arr;
	PPObjUnit unit_obj;

	PPWait(1);
	PPLoadText(PPTXT_IMPGOODS, wait_msg);

	// Закладываемся на имя файла GoodsCompImport.txt и на его расположение в PPY\IN
	PPGetFilePath(PPPATH_IN, "SuprWareImport.txt", file_name);
	THROW_SL(fileExists(file_name));
	{
		SuprWareDB goods_comp(file_name);
		PPTransaction tra(1);
		THROW(tra);
		THROW(goods_comp.InitIteration());
		while(goods_comp.NextIteration(goods_name, comps_arr) > 0) {
			int    r;
			PPID   sw_id = 0;
			PPSuprWarePacket sw_pack;
			for(uint i = 0; i < comps_arr.getCount(); i++) {
				const  PPComps & r_item = comps_arr.at(i);
				PPSuprWareAssoc sw_item;
				PPID   comp_id = 0;
				PPID   unit_id = 0;
				(comp_name = r_item.CompName).Transf(CTRANSF_OUTER_TO_INNER);
				THROW(r = sw_obj.P_Tbl->SearchByName(PPGDSK_SUPRWARE, comp_name, &comp_id));
				if(r < 0) {
					PPSuprWarePacket comp_pack;
					comp_name.CopyTo(comp_pack.Rec.Name, sizeof(comp_pack.Rec.Name));
					comp_pack.Rec.SuprWareType = SUPRWARETYPE_COMPONENT;
					comp_pack.Rec.SuprWareCat = SUPRWARECLASS_MEDICAL;
					THROW(sw_obj.Put(&comp_id, &comp_pack, 0));
				}
				unit_abbr = r_item.UnitAbbr;
				if(unit_abbr.NotEmptyS()) {
					PPUnit unit_rec;
					char micro_prefix[] = {(char)181, 0};
					uint pos = 0;
					// @v10.6.8 @ctr MEMSZERO(unit_rec);
					unit_rec.Tag = PPOBJ_UNIT;
					if((buf = r_item.UnitAbbr).HasChr(181) || buf.HasChr(-75)) // Если есть приставка "микро" (проверяем по двум кодам)
						buf.ReplaceStr(micro_prefix, "mk", 1);
					buf.CopyTo(unit_rec.Name, sizeof(unit_rec.Name));
					buf.CopyTo(unit_rec.Abbr, sizeof(unit_rec.Abbr));
					unit_rec.Flags = unit_rec.Physical;
					//
					// Сначала посмотрим, есть ли такая единица измерения
					//
					if(unit_obj.SearchMaxLike(&unit_rec, &unit_id) > 0)
						sw_item.UnitID = unit_id;
					else
						THROW(unit_obj.AddItem(&unit_id, &unit_rec, 0));
					sw_item.UnitID = unit_id;
				}
				sw_item.GoodsID = sw_id;
				sw_item.CompID = comp_id;
				sw_item.UnitID = unit_id;
				sw_item.Qtty = r_item.Qtty;
				THROW_SL(sw_pack.Items.insert(&sw_item));
			}
			goods_name.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(sw_pack.Rec.Name, sizeof(sw_pack.Rec.Name));
			sw_pack.Rec.SuprWareType = SUPRWARETYPE_GOODS;
			sw_pack.Rec.SuprWareCat = SUPRWARECLASS_MEDICAL;
			THROW(r = sw_obj.P_Tbl->SearchByName(PPGDSK_SUPRWARE, goods_name, &sw_id));
			if(r > 0)
				sw_pack.Rec.ID = sw_id;
			THROW(r = sw_obj.Put(&sw_id, &sw_pack, 0));
			if(r > 0)
				gds_count++;
			PPWaitPercent(goods_comp.GetCounter());
			total_gds_count++;
		}
		// @v9.2.4 PPLoadText(PPTXT_GOODS, buf);
		PPLoadString("ware_pl", buf); // @v9.2.4
		logger.Log(buf);
		PPLoadText(PPTXT_IMPORTEDRECS, buf);
		logger.Log(msg.Printf(buf, gds_count, total_gds_count));
		/* @v8.2.3
		PPLoadText(PPTXT_COMPONENTS, buf);
		logger.Log(buf);
		PPLoadText(PPTXT_IMPORTEDRECS, buf);
		logger.Log(msg.Printf(buf, comps_count, total_comps_count));
		*/
		THROW(tra.Commit());
	}
	CATCH
		ok = PPErrorZ();
		logger.LogLastError();
	ENDCATCH
	logger.Save(PPFILNAM_COMPGOODS_LOG, 0);
	PPWait(0);
	return ok;
}
// } @vmiller
//
//
// Import FIAS
//
//xmlParserCtxt * xmlCreateFileParserCtxt(const char * pFileName); // @prototype
extern "C" xmlParserCtxt * xmlCreateURLParserCtxt(const char * filename, int options);
void FASTCALL xmlDetectSAX2(xmlParserCtxt * ctxt); // @prototype

FiasImporter::Param::Param() : Flags(0)
{
}

int SLAPI FiasImporter::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Path, rBuf));
	CATCHZOK
	return ok;
}

SLAPI FiasImporter::ProcessState::Item::Item()
{
	THISZERO();
}

int SLAPI FiasImporter::ProcessState::Item::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, InputFileName, sizeof(InputFileName), rBuf));
	THROW_SL(pSCtx->Serialize(dir, Phase, rBuf)); // @v8.9.4
	THROW_SL(pSCtx->Serialize(dir, LastProcessedRecN, rBuf));
	THROW_SL(pSCtx->Serialize(dir, TotalMks, rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Reserve), Reserve, rBuf, 1)); // @v8.9.4
	CATCHZOK
	return ok;
}

SLAPI FiasImporter::ProcessState::ProcessState()
{
}

int SLAPI FiasImporter::ProcessState::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	return TSVector_Serialize <Item> (L, dir, rBuf, pSCtx) ? 1 : PPSetErrorSLib();
}

int SLAPI FiasImporter::ProcessState::Store(int use_ta)
{
	int    ok = 1;
	SBuffer buffer;
	SSerializeContext sctx;
	THROW(Serialize(+1, buffer, &sctx));
	THROW(PPRef->PutPropSBuffer(PPOBJ_FIAS, 1, FIASPRP_IMPORTSTATE2, buffer, use_ta));
	CATCHZOK
	return ok;
}

int SLAPI FiasImporter::ProcessState::Restore()
{
	int    ok = 1;
	SBuffer buffer;
	THROW(ok = PPRef->GetPropSBuffer(PPOBJ_FIAS, 1, FIASPRP_IMPORTSTATE2, buffer));
	if(ok > 0) {
		SSerializeContext sctx;
		THROW(Serialize(-1, buffer, &sctx));
	}
	else
		L.clear();
	CATCHZOK
	return ok;
}

int SLAPI FiasImporter::ProcessState::SearchItem(const char * pPath, int phase, uint * pPos, SString & rNormalizedName) const
{
	int    ok = 0;
	uint   pos = 0;
	SPathStruc ps(pPath);
	SPathStruc::NormalizePath(ps.Nam, 0, rNormalizedName);
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const Item & r_item = L.at(i);
		if(r_item.Phase == phase && rNormalizedName == r_item.InputFileName) {
			pos = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI FiasImporter::ProcessState::SetItem(const char * pPath, int phase, uint * pPos, SString & rNormalizedName)
{
	int    ok = 1;
	uint   pos = 0;
	if(SearchItem(pPath, phase, &pos, rNormalizedName)) {
		Item & r_item = L.at(pos);
		ok = 1;
	}
	else {
		Item new_item;
		new_item.Phase = phase;
		STRNSCPY(new_item.InputFileName, rNormalizedName);
		L.insert(&new_item);
		pos = L.getCount()-1;
		ok = 2;
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

//static {
void FiasImporter::Scb_StartDocument(void * ptr) { CALLTYPEPTRMEMB(FiasImporter, ptr, StartDocument()); }
void FiasImporter::Scb_EndDocument(void * ptr) { CALLTYPEPTRMEMB(FiasImporter, ptr, EndDocument()); }
void FiasImporter::Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList) { CALLTYPEPTRMEMB(FiasImporter, ptr, StartElement((const char *)pName, (const char **)ppAttrList)); }
void FiasImporter::Scb_EndElement(void * ptr, const xmlChar * pName) { CALLTYPEPTRMEMB(FiasImporter, ptr, EndElement((const char *)pName)); }
//} static

FiasImporter::FiasImporter() : Tra(0), TextCache(4 * 1024 * 1024),
	P_Sdr(0), P_DebugOutput(0), InputObject(0), RawRecN(0), P_SaxCtx(0), State(0), CurPsPos(-1), P_SrDb(0), P_SrStoreFiasAddrBlock(0)
{
}

FiasImporter::~FiasImporter()
{
	delete P_Sdr;
	delete P_DebugOutput;
	// @v9.9.0 {
	if(P_SrDb) {
		P_SrDb->DestroyStoreFiasAddrBlock(P_SrStoreFiasAddrBlock);
		delete P_SrDb;
	}
	// } @v9.9.0
}

int FiasImporter::SaxParseFile(xmlSAXHandler *sax, const char * pFileName)
{
	int    ret = 0;
	xmlFreeParserCtxt(P_SaxCtx);
	//P_SaxCtx = xmlCreateFileParserCtxt(pFileName);
	P_SaxCtx = xmlCreateURLParserCtxt(pFileName, 0);
	if(!P_SaxCtx)
		ret = -1;
	else {
		if(P_SaxCtx->sax != (xmlSAXHandler *)&xmlDefaultSAXHandler)
			SAlloc::F(P_SaxCtx->sax);
		P_SaxCtx->sax = sax;
		xmlDetectSAX2(P_SaxCtx);
		P_SaxCtx->userData = this;
		xmlParseDocument(P_SaxCtx);
		ret = P_SaxCtx->wellFormed ? 0 : NZOR(P_SaxCtx->errNo, -1);
		if(sax)
			P_SaxCtx->sax = 0;
		if(P_SaxCtx->myDoc) {
			xmlFreeDoc(P_SaxCtx->myDoc);
			P_SaxCtx->myDoc = NULL;
		}
		xmlFreeParserCtxt(P_SaxCtx);
		P_SaxCtx = 0;
	}
	return ret;
}

void FiasImporter::SaxStop()
{
	xmlStopParser(P_SaxCtx);
}

// static
int FiasImporter::ParseFiasFileName(const char * pFileName, SString & rObjName, LDATE & rDt, S_GUID & rUuid)
{
	int   ok = 1;
	long  status = 0;
	rObjName.Z();
	rDt = ZERODATE;
	rUuid.Z();
	SPathStruc ps(pFileName);
	if(ps.Ext.CmpNC("xml") == 0) {
		StringSet ss('_', ps.Nam);
		SString temp_buf;
		uint   n = 0;
		for(uint p = 0; ok && ss.get(&p, temp_buf); n++) {
			if(n == 0) {
				if(temp_buf.Strip().CmpNC("AS") != 0)
					ok = 0;
			}
			else if(n == 1) {
				if(temp_buf.Strip().CmpNC("DEL") == 0) {
					status |= 0x0001;
					ok |= 0x08000000;
					n--; // DEL - дополнительный компонент
				}
				else {
					if(status & 0x0001)
						(rObjName = "DEL").CatChar('_');
					rObjName.Cat(temp_buf);
				}
			}
			else if(n == 2) {
				strtodate(temp_buf.Strip().Trim(8), DATF_YMD|DATF_CENTURY|DATF_NODIV, &rDt);
			}
			else if(n == 3) {
				if(!rUuid.FromStr(temp_buf))
					ok = 0;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int FiasImporter::ReadRecordFromXmlAttrList(const char ** ppAttrList)
{
	int    ok = 1;
	if(P_Sdr) {
		SdbField fld;
		SFormatParam fp;
		SString temp_buf;
		fp.FDate = DATF_YMD|DATF_CENTURY;
		fp.FTime = TIMF_HMS;
		P_Sdr->ClearDataBuf();
		void * p_data_buf = P_Sdr->GetData();
		THROW(p_data_buf);
		for(uint i = 0; ppAttrList[i] != 0; i += 2) {
			const char * p_text_data = ppAttrList[i+1];
			if(p_text_data != 0) {
				if(P_Sdr->GetFieldByName_Fast(ppAttrList[i], &fld) > 0) {
					fld.PutFieldDataToBuf((temp_buf = p_text_data), p_data_buf, fp);
				}
			}
		}
		P_Sdr->ConvertDataFields(CTRANSF_UTF8_TO_OUTER, p_data_buf);
	}
	CATCHZOK
	return ok;
}

int FiasImporter::StartDocument()
{
	int    ok = -1;
	int    rec_id = 0;
	SString debug_file_name;
	if(InputObject == inpAddrObj) {
		rec_id = PPREC_FIASRAWADDROBJ;
		debug_file_name = "debug-addrobj";
		ok = 1;
	}
	else if(InputObject == inpHouse) {
		rec_id = PPREC_FIASRAWHOUSEOBJ;
		debug_file_name = "debug-houseobj";
		ok = 1;
	}
	if(ok > 0) {
		RawRecN = 0;
		ZDELETE(P_Sdr);
		THROW_MEM(P_Sdr = new SdRecord);
		THROW(LoadSdRecord(rec_id, P_Sdr));
		P_Sdr->AllocDataBuf();
		if(P.Flags & fDoDebugOutput) {
			if(CurPsPos >= 0) {
				const ProcessState::Item & r_state = Ps.L.at(CurPsPos);
				if(r_state.Phase == phaseCount) {
					SString debug_output_fname;
					(debug_output_fname = P.Path).SetLastSlash().Cat(debug_file_name).Dot().Cat("txt");
					P_DebugOutput = new SFile(debug_output_fname, SFile::mWrite);
				}
			}
		}
	}
	CATCH
		SaxStop();
		ok = 0;
	ENDCATCH
	return ok;
}

int FiasImporter::EndDocument()
{
	int    ok = 1;
	ZDELETE(P_Sdr);
	ZDELETE(P_DebugOutput);
	return ok;
}

int FASTCALL FiasImporter::ToggleTransaction(ulong threshold)
{
	int    ok = 1;
	if((RawRecN % threshold) == 0) {
		Ps.L.at(CurPsPos).LastProcessedRecN = RawRecN;
		Ps.Store(0);
		THROW(Tra.Commit());
		THROW(Tra.Start(1));
	}
	CATCHZOK
	return ok;
}

int FiasImporter::ProcessString(const char * pRawText, long * pRefId, SString & rTempBuf, SStringU & rTempBufU)
{
	int    ok = 1;
	long   _id = 0;
	if(pRawText)
		rTempBuf = pRawText;
	if(rTempBuf.Len()) {
		rTempBuf.ToLower1251();
		uint   hval = 0;
		uint   hpos = 0;
		if(TextCache.Search(rTempBuf, &hval, &hpos)) {
			_id = (long)hval;
			ok = 1;
		}
		else {
			SString utf_buf = rTempBuf;
			rTempBufU.CopyFromUtf8(utf_buf.ToUtf8());
			ok = PPRef->TrT.GetSelfRefText(rTempBufU, &_id, 0);
			if(ok > 0 && _id) {
				TextCache.Add(rTempBuf, (uint)_id);
			}
		}
	}
    ASSIGN_PTR(pRefId, _id);
	return ok;
}

int FiasImporter::CollectUuid(const S_GUID & rUuid)
{
	int    ok = -1;
	int    sr = FT.UrT.SearchUuid(rUuid, 0, 0);
	THROW(sr);
	if(sr < 0) {
		if(!PreprocessUuidChunk.lsearch(&rUuid, 0, PTR_CMPFUNC(S_GUID))) {
			THROW_SL(PreprocessUuidChunk.insert(&rUuid));
			ok = 1;
		}
		else
			ok = -2;
	}
	CATCHZOK
	return ok;
}

int FiasImporter::FlashUuidChunk(uint maxCount, int use_ta)
{
	int    ok = FT.UrT.PutChunk(PreprocessUuidChunk, maxCount, use_ta);
	if(ok > 0) {
		Ps.L.at(CurPsPos).LastProcessedRecN = RawRecN;
		if(Ps.Store(use_ta))
			PreprocessUuidChunk.clear();
		else
			ok = 0;
	}
	return ok;
}

int FiasImporter::FlashAddrChunk(uint maxCount, int use_ta)
{
	int    ok = 1;
	const  uint cc = AddrRecChunk.getCount();
	if(cc > maxCount) {
        PPTransaction tra(use_ta);
        THROW(tra);
        {
        	BExtInsert bei(&FT.AdrT);
        	for(uint i = 0; i < cc; i++) {
				const FiasAddrObjTbl::Rec & r_rec = AddrRecChunk.at(i);
				THROW_DB(bei.insert(&r_rec));
        	}
        	THROW_DB(bei.flash());
        }
		Ps.L.at(CurPsPos).LastProcessedRecN = RawRecN;
		THROW(Ps.Store(0));
        THROW(tra.Commit());
        AddrRecChunk.clear();
        ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int FiasImporter::FlashHouseChunk(uint maxCount, int use_ta)
{
	int    ok = 1;
	const  uint cc = HouseRecChunk.getCount();
	if(cc > maxCount) {
        PPTransaction tra(use_ta);
        THROW(tra);
        {
        	BExtInsert bei(&FT.HseT);
        	for(uint i = 0; i < cc; i++) {
				const FiasHouseObjTbl::Rec & r_rec = HouseRecChunk.at(i);
				THROW_DB(bei.insert(&r_rec));
        	}
        	THROW_DB(bei.flash());
        }
		Ps.L.at(CurPsPos).LastProcessedRecN = RawRecN;
		THROW(Ps.Store(0));
        THROW(tra.Commit());
        HouseRecChunk.clear();
        ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int FiasImporter::StartElement(const char * pName, const char ** ppAttrList)
{
	int    ok = 1;
	SString line_buf;
	if(InputObject == inpAddrObj && sstreqi_ascii(pName, "object")) {
		RawRecN++;
		if(CurPsPos >= 0) {
			const ProcessState::Item & r_state = Ps.L.at(CurPsPos);
			if(r_state.LastProcessedRecN < RawRecN) {
				THROW(ReadRecordFromXmlAttrList(ppAttrList));
				const Sdr_FiasRawAddrObj * p_data = (const Sdr_FiasRawAddrObj *)P_Sdr->GetDataC();
				assert(p_data);
				if(p_data) {
					line_buf.Z();
					if(r_state.Phase == phaseCount) {
						if(P_DebugOutput && P_DebugOutput->IsValid()) {
							if(RawRecN == 1) {
								static const char * p_titles[] = {
									"AOID", "PREVID", "NEXTID", "AOGUID", "PARENTGUID", "POSTALCODE",
									"SHORTNAME", "FORMALNAME", "OFFNAME", "REGIONCODE", "AUTOCODE",
									"AREACODE", "CITYCODE", "CTARCODE", "PLACECODE", "STREETCODE",
									"EXTRCODE", "SEXTCODE", "IFNSFL", "TERRIFNSFL", "IFNSUL",
									"TERRIFNSUL", "OKATO", "OKTMO", "UPDATEDATE", "AOLEVEL",
									"CODE", "PLAINCODE", "ACTSTATUS", "CENTSTATUS", "OPERSTATUS",
									"CURRSTATUS", "STARTDATE", "ENDDATE", "NORMDOC", "LIVESTATUS" };
								for(uint tidx = 0; tidx < SIZEOFARRAY(p_titles); tidx++)
									line_buf.Cat(p_titles[tidx]).Semicol();
								line_buf.CR();
								P_DebugOutput->WriteLine(line_buf);
							}
							line_buf.Z();
							line_buf.Cat(p_data->AOID).Semicol();
							line_buf.Cat(p_data->PREVID).Semicol();
							line_buf.Cat(p_data->NEXTID).Semicol();
							line_buf.Cat(p_data->AOGUID).Semicol();
							line_buf.Cat(p_data->PARENTGUID).Semicol();
							line_buf.Cat(p_data->POSTALCODE).Semicol();
							line_buf.Cat(p_data->SHORTNAME).Semicol();
							line_buf.Cat(p_data->FORMALNAME).Semicol();
							line_buf.Cat(p_data->OFFNAME).Semicol();
							line_buf.Cat(p_data->REGIONCODE).Semicol();
							line_buf.Cat(p_data->AUTOCODE).Semicol();
							line_buf.Cat(p_data->AREACODE).Semicol();
							line_buf.Cat(p_data->CITYCODE).Semicol();
							line_buf.Cat(p_data->CTARCODE).Semicol();
							line_buf.Cat(p_data->PLACECODE).Semicol();
							line_buf.Cat(p_data->STREETCODE).Semicol();
							line_buf.Cat(p_data->EXTRCODE).Semicol();
							line_buf.Cat(p_data->SEXTCODE).Semicol();
							line_buf.Cat(p_data->IFNSFL).Semicol();
							line_buf.Cat(p_data->TERRIFNSFL).Semicol();
							line_buf.Cat(p_data->IFNSUL).Semicol();
							line_buf.Cat(p_data->TERRIFNSUL).Semicol();
							line_buf.Cat(p_data->OKATO).Semicol();
							line_buf.Cat(p_data->OKTMO).Semicol();
							line_buf.Cat(p_data->UPDATEDATE).Semicol();
							line_buf.Cat(p_data->AOLEVEL).Semicol();
							line_buf.Cat(p_data->CODE).Semicol();
							line_buf.Cat(p_data->PLAINCODE).Semicol();
							line_buf.Cat(p_data->ACTSTATUS).Semicol();
							line_buf.Cat(p_data->CENTSTATUS).Semicol();
							line_buf.Cat(p_data->OPERSTATUS).Semicol();
							line_buf.Cat(p_data->CURRSTATUS).Semicol();
							line_buf.Cat(p_data->STARTDATE).Semicol();
							line_buf.Cat(p_data->ENDDATE).Semicol();
							line_buf.Cat(p_data->NORMDOC).Semicol();
							line_buf.Cat(p_data->LIVESTATUS).Semicol();
							line_buf.CR();
							P_DebugOutput->WriteLine(line_buf);
						}
					}
					else {
						FiasAddrObjTbl::Rec rec, org_rec;
						MEMSZERO(rec);
						SStringU utext;
						int    sr = 0;
						if(r_state.Phase == phaseUUID) {
							if(p_data->LIVESTATUS) {
								THROW(CollectUuid(p_data->AOID));
								THROW(CollectUuid(p_data->PREVID));
								THROW(CollectUuid(p_data->NEXTID));
								THROW(CollectUuid(p_data->AOGUID));
								THROW(CollectUuid(p_data->PARENTGUID));
								THROW(FlashUuidChunk(1024, 1));
							}
						}
						else if(r_state.Phase == phaseText) {
							if(p_data->LIVESTATUS) {
								THROW(ProcessString(p_data->FORMALNAME, &rec.NameTRef, line_buf, utext));
								THROW(ProcessString(p_data->OFFNAME,    &rec.OfcNameTRef, line_buf, utext));
								THROW(ProcessString(p_data->SHORTNAME,  &rec.SnTRef, line_buf, utext));
								THROW(ProcessString(p_data->OKATO,      &rec.OkatoTRef, line_buf, utext));
								THROW(ProcessString(p_data->OKTMO,      &rec.OktmoTRef, line_buf, utext));
								THROW(ProcessString(p_data->PLAINCODE,  &rec.KladrCodeTRef, line_buf, utext));
								THROW(ToggleTransaction(10000));
							}
						}
						// Следующие 2 фазы перебирают не только актуальные записи {
						else if(r_state.Phase == phaseSartrePass1) {
							if(P_SrDb) {
								THROW(P_SrDb->StoreFiasAddr(P_SrStoreFiasAddrBlock, 1, p_data));
							}
						}
						else if(r_state.Phase == phaseSartrePass2) {
							if(P_SrDb) {
								THROW(P_SrDb->StoreFiasAddr(P_SrStoreFiasAddrBlock, 2, p_data));
							}
						}
						// }
						else {
							if(p_data->LIVESTATUS) {
								THROW(FT.UrT.GetUuid(p_data->AOID, &rec.RecUuID, 0, 0));
								THROW(sr = FT.SearchAddr(rec.RecUuID, &org_rec));
								if(sr < 0) {
									THROW(FT.UrT.GetUuid(p_data->PREVID, &rec.PrevRecUuRef, 0, 0));
									THROW(FT.UrT.GetUuid(p_data->NEXTID, &rec.NextRecUuRef, 0, 0));
									THROW(FT.UrT.GetUuid(p_data->AOGUID, &rec.IdUuRef, UuidRefCore::sgoHash, 0));
									THROW(FT.UrT.GetUuid(p_data->PARENTGUID, &rec.ParentUuRef, UuidRefCore::sgoHash, 0));
									//
									THROW(ProcessString(p_data->FORMALNAME, &rec.NameTRef, line_buf, utext));
									THROW(ProcessString(p_data->OFFNAME,    &rec.OfcNameTRef, line_buf, utext));
									THROW(ProcessString(p_data->SHORTNAME,  &rec.SnTRef, line_buf, utext));
									THROW(ProcessString(p_data->OKATO,      &rec.OkatoTRef, line_buf, utext));
									THROW(ProcessString(p_data->OKTMO,      &rec.OktmoTRef, line_buf, utext));
									THROW(ProcessString(p_data->PLAINCODE,  &rec.KladrCodeTRef, line_buf, utext));
									rec.PostalCode = atoi(p_data->POSTALCODE);
									rec.IfnsJ = atoi(p_data->IFNSUL);
									rec.IfnsI = atoi(p_data->IFNSFL);
									rec.TerrIfnsJ = atoi(p_data->TERRIFNSUL);
									rec.TerrIfnsI = atoi(p_data->TERRIFNSFL);

									rec.LevelStatus = atoi(p_data->AOLEVEL);
									rec.CenterStatus = (int16)p_data->CENTSTATUS;
									rec.ActionStatus = (int16)p_data->ACTSTATUS;
									rec.KladrCurStatus = (int16)p_data->CURRSTATUS;

									rec.StartDt = p_data->STARTDATE;
									rec.EndDt = p_data->ENDDATE;
									rec.UpdateDt = p_data->UPDATEDATE;

									SETFLAG(rec.Flags, FIASADRF_ACTUAL, p_data->LIVESTATUS);
									if(r_state.Phase == phaseData) {
										THROW_SL(AddrRecChunk.insert(&rec));
										THROW(FlashAddrChunk(1024, 1));
									}
									else {
										THROW_DB(FT.AdrT.insertRecBuf(&rec));
									}
								}
							}
							if(r_state.Phase != phaseData) {
								if(p_data->LIVESTATUS) {
									THROW(ToggleTransaction(1000));
								}
							}
						}
					}
				}
			}
			if((RawRecN % 1000) == 0) {
				line_buf = "Address";
				switch(r_state.Phase) {
					case phaseCount: line_buf.Space().Cat("phaseCOUNT"); break;
					case phaseUUID: line_buf.Space().Cat("phaseUUID"); break;
					case phaseText: line_buf.Space().Cat("phaseTEXT"); break;
					case phaseData: line_buf.Space().Cat("phaseDATA"); break;
					case phaseSartrePass1: line_buf.Space().Cat("phaseSartrePass1"); break;
					case phaseSartrePass2: line_buf.Space().Cat("phaseSartrePass2"); break;
				}
				line_buf.Space().Cat(RawRecN);
				PPWaitMsg(line_buf);
			}
		}
	}
	else if(InputObject == inpHouse && sstreqi_ascii(pName, "house")) {
		RawRecN++;
		if(CurPsPos >= 0) {
			const ProcessState::Item & r_state = Ps.L.at(CurPsPos);
			if(r_state.LastProcessedRecN < RawRecN) {
				THROW(ReadRecordFromXmlAttrList(ppAttrList));
				const Sdr_FiasRawHouseObj * p_data = (const Sdr_FiasRawHouseObj *)P_Sdr->GetDataC();
				if(p_data) {
					line_buf.Z();
					if(r_state.Phase == phaseCount) {
						if(P_DebugOutput && P_DebugOutput->IsValid()) {
							if(RawRecN == 1) {
								static const char * p_titles[] = {
									"HOUSENUM", "BUILDNUM", "STRUCNUM", "POSTALCODE", "OKATO",
									"OKTMO", "ESTSTATUS", "STRSTATUS", "HOUSEID", "HOUSEGUID",
									"AOGUID", "IFNSFL", "TERRIFNSFL", "IFNSUL", "TERRIFNSUL",
									"UPDATEDATE", "STARTDATE", "ENDDATE", "STATSTATUS", "NORMDOC", "COUNTER" };
								for(uint tidx = 0; tidx < SIZEOFARRAY(p_titles); tidx++)
									line_buf.Cat(p_titles[tidx]).Semicol();
								line_buf.CR();
								P_DebugOutput->WriteLine(line_buf);
							}
							line_buf.Z();
							line_buf.Cat(p_data->HOUSENUM).Semicol();
							line_buf.Cat(p_data->BUILDNUM).Semicol();
							line_buf.Cat(p_data->STRUCNUM).Semicol();
							line_buf.Cat(p_data->POSTALCODE).Semicol();
							line_buf.Cat(p_data->OKATO).Semicol();
							line_buf.Cat(p_data->OKTMO).Semicol();
							line_buf.Cat(p_data->ESTSTATUS).Semicol();
							line_buf.Cat(p_data->STRSTATUS).Semicol();
							line_buf.Cat(p_data->HOUSEID).Semicol();
							line_buf.Cat(p_data->HOUSEGUID).Semicol();
							line_buf.Cat(p_data->AOGUID).Semicol();
							line_buf.Cat(p_data->IFNSFL).Semicol();
							line_buf.Cat(p_data->TERRIFNSFL).Semicol();
							line_buf.Cat(p_data->IFNSUL).Semicol();
							line_buf.Cat(p_data->TERRIFNSUL).Semicol();
							line_buf.Cat(p_data->UPDATEDATE).Semicol();
							line_buf.Cat(p_data->STARTDATE).Semicol();
							line_buf.Cat(p_data->ENDDATE).Semicol();
							line_buf.Cat(p_data->STATSTATUS).Semicol();
							line_buf.Cat(p_data->NORMDOC).Semicol();
							line_buf.Cat(p_data->COUNTER).Semicol();
							line_buf.CR();
							P_DebugOutput->WriteLine(line_buf);
						}
					}
					else {
						FiasHouseObjTbl::Rec rec, org_rec;
						MEMSZERO(rec);
						SStringU utext;
						int    sr = 0;
						if(r_state.Phase == phaseUUID) {
							THROW(CollectUuid(p_data->HOUSEGUID));
							THROW(CollectUuid(p_data->AOGUID));
							THROW(FlashUuidChunk(1024, 1));
						}
						else if(r_state.Phase == phaseText) {
							THROW(ProcessString(0, &rec.NumTRef, line_buf, utext));
							//THROW(ProcessString(p_data->OKATO, &rec.OkatoTRef, line_buf, utext));
							//THROW(ProcessString(p_data->OKTMO, &rec.OktmoTRef, line_buf, utext));
							THROW(ToggleTransaction(1000000));
						}
						else {
							THROW(FT.UrT.GetUuid(p_data->HOUSEGUID, &rec.IdUuRef, 0, 0));
							THROW(sr = FT.SearchHouse(rec.IdUuRef, &org_rec));
							if(sr < 0 && r_state.Phase == phaseData && HouseRecChunk.lsearch(&rec.IdUuRef, 0, CMPF_LONG)) {
								sr = 1;
							}
							if(sr < 0) {
								FiasObjCore::HouseCode hse_code(0);
								THROW(FT.UrT.GetUuid(p_data->AOGUID, &rec.ParentUuRef, UuidRefCore::sgoHash, 0));
								//
								hse_code.HseNum = p_data->HOUSENUM;
								hse_code.BldNum = p_data->BUILDNUM;
								hse_code.StrNum = p_data->STRUCNUM;
								hse_code.Encode(line_buf.Z());
								THROW(ProcessString(0, &rec.NumTRef, line_buf, utext));
								//
								//THROW(ProcessString(p_data->OKATO, &rec.OkatoTRef, line_buf, utext));
								//THROW(ProcessString(p_data->OKTMO, &rec.OktmoTRef, line_buf, utext));
								rec.PostalCode = atoi(p_data->POSTALCODE);
								rec.IfnsJ = atoi(p_data->IFNSUL);
								rec.IfnsI = atoi(p_data->IFNSFL);
								rec.TerrIfnsJ = atoi(p_data->TERRIFNSUL);
								rec.TerrIfnsI = atoi(p_data->TERRIFNSFL);

								rec.IntStatus = 0;
								rec.BuildStatus = (int16)p_data->STRSTATUS;
								rec.EstStatus   = (int16)p_data->ESTSTATUS;

								rec.StartDt = p_data->STARTDATE;
								rec.EndDt = p_data->ENDDATE;
								rec.UpdateDt = p_data->UPDATEDATE;
								if(r_state.Phase == phaseData) {
									THROW_SL(HouseRecChunk.insert(&rec));
									THROW(FlashHouseChunk(1024, 1));
								}
								else {
									THROW_DB(FT.HseT.insertRecBuf(&rec));
								}
							}
							if(r_state.Phase != phaseData) {
								THROW(ToggleTransaction(1000));
							}
						}
					}
					if((RawRecN % 1000) == 0) {
						line_buf = "Houses";
						switch(r_state.Phase) {
							case phaseCount: line_buf.Space().Cat("phaseCOUNT"); break;
							case phaseUUID: line_buf.Space().Cat("phaseUUID"); break;
							case phaseText: line_buf.Space().Cat("phaseTEXT"); break;
							case phaseData: line_buf.Space().Cat("phaseDATA"); break;
						}
						line_buf.Space().Cat(RawRecN);
						PPWaitMsg(line_buf);
					}
				}
			}
		}
	}
	CATCH
		SaxStop();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

int FiasImporter::EndElement(const char * pName)
{
	int    ok = 1;
	return ok;
}

int FiasImporter::DoPhase(int inpObject, int phase, const SString & rFileName, xmlSAXHandler & rSaxH)
{
	int    ok = 1;
	SString nfn;
	uint   ps_pos = 0;
	PPWait(1);
	if(!(P.Flags & Param::fIgnoreSavedState)) {
		THROW(Ps.Restore());
	}
	THROW(Ps.SetItem(rFileName, phase, &ps_pos, nfn));
	CurPsPos = (int)ps_pos;
	if(!oneof4(phase, phaseUUID, phaseData, phaseSartrePass1, phaseSartrePass2)) {
		THROW(Tra.Start(1));
	}
	InputObject = inpObject;
	SaxParseFile(&rSaxH, rFileName);
	THROW(Ps.Store(-1));
	if(phase == phaseUUID) {
		THROW(FlashUuidChunk(0, 1));
	}
	else if(phase == phaseData) {
		THROW(FlashAddrChunk(0, 1));
		THROW(FlashHouseChunk(0, 1));
	}
	else if(oneof2(phase, phaseSartrePass1, phaseSartrePass2)) {
		if(P_SrDb) {
			THROW(P_SrDb->StoreFiasAddr(P_SrStoreFiasAddrBlock, (phase - phaseSartrePass1 + 1), 0));
		}
	}
	else {
		THROW(Tra.Commit());
	}
	THROW(!(State & stError));
	CATCHZOK
	PPWait(0);
	return ok;
}

int FiasImporter::Import(int inpObject)
{
	int    ok = 1;
	SString file_name, wildcard;
	SString dest_file_obj_name;
	if(inpObject == inpAddrObj) {
		// 	RawRecN	2489624
		(wildcard = P.Path).Strip().SetLastSlash().Cat("AS_ADDROBJ").CatChar('*').Dot().Cat("xml");
		dest_file_obj_name = "ADDROBJ";
	}
	else if(inpObject == inpHouse) {
		// RawRecN 35979557
		(wildcard = P.Path).Strip().SetLastSlash().Cat("AS_HOUSE").CatChar('*').Dot().Cat("xml");
		dest_file_obj_name = "HOUSE";
	}
	if(wildcard.NotEmpty()) {
		LDATE   max_date = ZERODATE;
		SDirEntry de;
		SString file_obj_name;
		LDATE  file_dt;
		S_GUID file_uuid;
		for(SDirec dir(wildcard, 0); dir.Next(&de) > 0;) {
			int pr = ParseFiasFileName(de.FileName, file_obj_name, file_dt, file_uuid);
			if(pr == 1) {
				if(file_obj_name.CmpNC(dest_file_obj_name) == 0 && file_dt > max_date) {
					(file_name = P.Path).SetLastSlash().Cat(de.FileName);
					max_date = file_dt;
				}
			}
		}
		if(file_name.NotEmpty() && checkdate(max_date)) {
			xmlSAXHandler saxh_addr_obj;
			MEMSZERO(saxh_addr_obj);
			saxh_addr_obj.startDocument = Scb_StartDocument;
			saxh_addr_obj.endDocument = Scb_EndDocument;
			saxh_addr_obj.startElement = Scb_StartElement;
			saxh_addr_obj.endElement = Scb_EndElement;
			if(P.Flags & P.fAcceptToSartreDb) {
				const  int  phase_list[] = { /*phaseCount,*/ phaseSartrePass1, phaseSartrePass2 };
				for(uint phn = 0; phn < SIZEOFARRAY(phase_list); phn++) {
					THROW(DoPhase(inpObject, phase_list[phn], file_name, saxh_addr_obj));
				}
			}
			else {
				const  int  phase_list[] = { phaseCount, phaseUUID, phaseText, phaseData };
				for(uint phn = 0; phn < SIZEOFARRAY(phase_list); phn++) {
					THROW(DoPhase(inpObject, phase_list[phn], file_name, saxh_addr_obj));
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int FiasImporter::EditParam(Param & rP)
{
    class FiasImpParamDialog : public TDialog {
	public:
		FiasImpParamDialog() : TDialog(DLG_FIASIMP)
		{
			FileBrowseCtrlGroup::Setup(this, CTLBRW_FIASIMP_PATH, CTL_FIASIMP_PATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		}
		int    setDTS(const FiasImporter::Param * pData)
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setCtrlString(CTL_FIASIMP_PATH, Data.Path);
			AddClusterAssoc(CTL_FIASIMP_FLAGS, 0, FiasImporter::Param::fImportAddrObj);
			AddClusterAssoc(CTL_FIASIMP_FLAGS, 1, FiasImporter::Param::fImportHouseObj);
			AddClusterAssoc(CTL_FIASIMP_FLAGS, 2, FiasImporter::Param::fIgnoreSavedState);
			AddClusterAssoc(CTL_FIASIMP_FLAGS, 3, FiasImporter::Param::fAcceptToSartreDb);
			SetClusterData(CTL_FIASIMP_FLAGS, Data.Flags);
			return ok;
		}
		int    getDTS(FiasImporter::Param * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlString(CTL_FIASIMP_PATH, Data.Path);
			THROW(pathValid(Data.Path, 1));
			GetClusterData(CTL_FIASIMP_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		FiasImporter::Param Data;
    };
    DIALOG_PROC_BODY(FiasImpParamDialog, &rP);
}

/*
int SLAPI ImportFias()
{
	int    ok = 1;
	FiasImporter prcssr("D:\\Papyrus\\Universe-HTT\\DATA\\KLADR\\FIAS");
	prcssr.Import(FiasImporter::inpAddrObj, 0);
	prcssr.Import(FiasImporter::inpHouse, 0);
	return ok;
}
*/

int SLAPI FiasImporter::Run(const FiasImporter::Param & rP)
{
	int    ok = 1;
	P = rP;
	if(P.Flags & P.fAcceptToSartreDb) {
		THROW_MEM(SETIFZ(P_SrDb, new SrDatabase()));
		THROW(P_SrDb->Open(0, SrDatabase::oWriteStatOnClose));
		THROW(P_SrStoreFiasAddrBlock = P_SrDb->CreateStoreFiasAddrBlock());
	}
	else {
		if(P_SrDb) {
			P_SrDb->DestroyStoreFiasAddrBlock(P_SrStoreFiasAddrBlock);
			P_SrStoreFiasAddrBlock = 0;
			ZDELETE(P_SrDb);
		}
	}
	if(P.Flags & P.fImportAddrObj) {
		THROW(Import(FiasImporter::inpAddrObj));
	}
	if(P.Flags & P.fImportHouseObj) {
		THROW(Import(FiasImporter::inpHouse));
	}
	CATCHZOK
	if(P_SrDb) {
		P_SrDb->DestroyStoreFiasAddrBlock(P_SrStoreFiasAddrBlock);
		P_SrStoreFiasAddrBlock = 0;
		ZDELETE(P_SrDb);
	}
	return ok;
}
//
//
//
SLAPI PrcssrOsm::StatBlock::StatBlock()
{
	Clear();
}

void SLAPI PrcssrOsm::StatBlock::Clear()
{
	NodeCount = 0;
	NakedNodeCount = 0;
	WayCount = 0;
	RelationCount = 0;
	TagNodeCount = 0;
	TagWayCount = 0;
	TagRelCount = 0;
	NcList.clear();
	WayList.clear();
}

int SLAPI PrcssrOsm::StatBlock::Serialize(int dir, SBuffer & rBuffer, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, NodeCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, NakedNodeCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, WayCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, RelationCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, TagNodeCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, TagWayCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, TagRelCount, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, &NcList, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, &WayList, rBuffer));
	CATCHZOK
	return ok;
}

#define _SUM_OF_VECTORMEMBER(_type, _vect, _memb) _type t = 0; for(uint i = 0; i < _vect.getCount(); i++) { t += _vect.at(i)._memb; } return t
	uint64 SLAPI PrcssrOsm::StatBlock::GetNcActualCount() const { _SUM_OF_VECTORMEMBER(uint64, NcList, ActualCount); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetNcProcessedCount() const { _SUM_OF_VECTORMEMBER(uint64, NcList, ProcessedCount); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetNcClusterCount() const { _SUM_OF_VECTORMEMBER(uint64, NcList, ClusterCount); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetNcSize() const { _SUM_OF_VECTORMEMBER(uint64, NcList, Size); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetWsCount() const { _SUM_OF_VECTORMEMBER(uint64, WayList, WayCount); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetWsProcessedCount() const { _SUM_OF_VECTORMEMBER(uint64, WayList, ProcessedCount); }
	uint64 SLAPI PrcssrOsm::StatBlock::GetWsSize() const { _SUM_OF_VECTORMEMBER(uint64, WayList, Size); }
#undef _SUM_OF_VECTORMEMBER

SLAPI PrcssrOsm::RoadStone::RoadStone() : Phase(phaseUnkn)
{
}

int SLAPI PrcssrOsm::RoadStone::Serialize(int dir, SBuffer & rBuffer, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(pSCtx->Serialize(dir, SrcFileName, rBuffer));
	THROW_SL(pSCtx->Serialize(dir, Phase, rBuffer));
	THROW(Stat.Serialize(dir, rBuffer, pSCtx));
	CATCHZOK
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(PrcssrOsm); SLAPI PrcssrOsmFilt::PrcssrOsmFilt() : PPBaseFilt(PPFILT_PRCSSROSMPARAM, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrOsmFilt, ReserveStart),
		offsetof(PrcssrOsmFilt, SrcFileName)-offsetof(PrcssrOsmFilt, ReserveStart));
	SetBranchSString(offsetof(PrcssrOsmFilt, SrcFileName));
	Init(1, 0);
}

PrcssrOsmFilt & FASTCALL PrcssrOsmFilt::operator = (const PrcssrOsmFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

int SLAPI PrcssrOsmFilt::IsEmpty() const
{
	if(Flags)
		return 0;
	else if(SrcFileName.NotEmpty())
		return 0;
	else
		return 1;
}

SLAPI PrcssrOsm::CommonAttrSet::CommonAttrSet()
{
	Reset();
}

void SLAPI PrcssrOsm::CommonAttrSet::Reset()
{
	ID = 0;
	Lat = 0.0;
	Lon = 0.0;
	T = ZERODATETIME;
	Ver = 0;
	Visible = 1;
	ChangeSet = 0;
	UserID = 0;
	TypeSymbID = 0;
	RoleSymbID = 0;
	RefID = 0;
	User.Z();
}

SLAPI PrcssrOsm::PrcssrOsm(const char * pDbPath) : O(pDbPath), GgtFinder(O.GetGrid()),
	P_SaxCtx(0), State(0), P_RoadStoneStat(0), P_LatOutF(0), P_LonOutF(0),
	P_NodeToWayAssocOutF(0), P_NodeToWayAssocInF(0), P_TagOutF(0), P_TagNodeOutF(0),
	P_TagWayOutF(0), P_TagRelOutF(0), P_SizeOutF(0), P_TestNodeBinF(0), P_TestNodeF(0),
	P_Ufp(0), P_ShT(PPGetStringHash(PPSTR_HASHTOKEN))
{
	Reset();
}

SLAPI PrcssrOsm::~PrcssrOsm()
{
	P_ShT = 0;
	Reset();
}

int SLAPI PrcssrOsm::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrOsmFilt * p_filt = static_cast<PrcssrOsmFilt *>(pBaseFilt);
		if(p_filt->IsEmpty()) {
		}
	}
	else
		ok = 0;
	return ok;
}

class PrcssrOsmFiltDialog : public TDialog {
public:
	PrcssrOsmFiltDialog() : TDialog(DLG_PRCROSM)
	{
	}
	int    setDTS(const PrcssrOsmFilt * pData)
	{
		int    ok = 1;
		Data = *pData;
		AddClusterAssoc(CTL_PRCROSM_FLAGS, 0, Data.fPreprocess);
		AddClusterAssoc(CTL_PRCROSM_FLAGS, 1, Data.fSortPreprcResults);
		AddClusterAssoc(CTL_PRCROSM_FLAGS, 2, Data.fAnlzPreprcResults);
		AddClusterAssoc(CTL_PRCROSM_FLAGS, 3, Data.fImport);
		AddClusterAssoc(CTL_PRCROSM_FLAGS, 4, Data.fExtractSizes);
		SetClusterData(CTL_PRCROSM_FLAGS, Data.Flags);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_PRCROSM_PATH, CTL_PRCROSM_PATH, 1, 0, PPTXT_FILPAT_OSM, FileBrowseCtrlGroup::fbcgfFile);
		setCtrlString(CTL_PRCROSM_PATH, Data.SrcFileName);
		return ok;
	}
	int    getDTS(PrcssrOsmFilt * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_PRCROSM_FLAGS, &Data.Flags);
		getCtrlString(sel = CTL_PRCROSM_PATH, Data.SrcFileName);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	PrcssrOsmFilt Data;
};

int SLAPI PrcssrOsm::EditParam(PPBaseFilt * pBaseFilt)
{
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrOsmFilt * p_filt = static_cast<PrcssrOsmFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PrcssrOsmFiltDialog, p_filt);
}

int SLAPI PrcssrOsm::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *static_cast<const PrcssrOsmFilt *>(pBaseFilt);
	CATCHZOK
	return ok;
}

void SLAPI PrcssrOsm::Reset()
{
	State = 0;
	Phase = phaseUnkn;
	TokPath.clear();
	TempCaSet.Reset();
	CurrentTagList.clear();
	TokPath.clear();
	Stat.Clear();
	TempTagKeyList.clear();
	TagKeyList.clear();
	LatAccum.clear();
	LonAccum.clear();
	NodeAccum.clear();
	NodeWayAssocAccum.clear();
	LastNodeToWayAssoc.Key = 0;
	LastNodeToWayAssoc.Val = 0;
	ZDELETE(P_RoadStoneStat);
	ZDELETE(P_LatOutF);
	ZDELETE(P_LonOutF);
	ZDELETE(P_NodeToWayAssocOutF);
	ZDELETE(P_NodeToWayAssocInF);
	ZDELETE(P_TagOutF);
	ZDELETE(P_TagNodeOutF);
	ZDELETE(P_TagWayOutF);
	ZDELETE(P_TagRelOutF);
	ZDELETE(P_SizeOutF);
	ZDELETE(P_TestNodeBinF);
	ZDELETE(P_TestNodeF);
	ZDELETE(P_Ufp);
}

int PrcssrOsm::SaxParseFile(xmlSAXHandler * sax, const char * pFileName)
{
	int    ret = 0;
	xmlFreeParserCtxt(P_SaxCtx);
	//P_SaxCtx = xmlCreateFileParserCtxt(pFileName);
	P_SaxCtx = xmlCreateURLParserCtxt(pFileName, 0);
	if(!P_SaxCtx)
		ret = -1;
	else {
		if(P_SaxCtx->sax != (xmlSAXHandler *)&xmlDefaultSAXHandler)
			SAlloc::F(P_SaxCtx->sax);
		P_SaxCtx->sax = sax;
		xmlDetectSAX2(P_SaxCtx);
		P_SaxCtx->userData = this;
		xmlParseDocument(P_SaxCtx);
		ret = P_SaxCtx->wellFormed ? 0 : NZOR(P_SaxCtx->errNo, -1);
		if(sax)
			P_SaxCtx->sax = 0;
		if(P_SaxCtx->myDoc) {
			xmlFreeDoc(P_SaxCtx->myDoc);
			P_SaxCtx->myDoc = NULL;
		}
		xmlFreeParserCtxt(P_SaxCtx);
		P_SaxCtx = 0;
	}
	return ret;
}

void PrcssrOsm::SaxStop()
{
	xmlStopParser(P_SaxCtx);
}

int PrcssrOsm::StartDocument()
{
	int    ok = -1;
	return ok;
}

int PrcssrOsm::EndDocument()
{
	int    ok = 1;
	//if(InputObject == inpAddrObj) {
	/*
		ZDELETE(P_Sdr);
		ZDELETE(P_DebugOutput);
	*/
	//}
	return ok;
}

int PrcssrOsm::ReadCommonAttrSet(const char ** ppAttrList, CommonAttrSet & rSet)
{
	int    result = 1;
	rSet.Reset();
	for(uint i = 0; ppAttrList[i] != 0; i += 2) {
		Pb.TempBuf = ppAttrList[i+1];
		if(Pb.TempBuf.NotEmptyS()) {
			int    tok = 0;
			(Pb.AttrBuf = ppAttrList[i]).ToLower();
			if(P_ShT) {
				uint _ut = 0;
				P_ShT->Search(Pb.AttrBuf, &_ut, 0);
				tok = _ut;
			}
			switch(tok) {
				case PPHS_ID:        rSet.ID = Pb.TempBuf.ToInt64(); break;
				case PPHS_LAT:       rSet.Lat = Pb.TempBuf.ToReal(); break;
				case PPHS_LON:       rSet.Lon = Pb.TempBuf.ToReal(); break;
				case PPHS_VERSION:   rSet.Ver = Pb.TempBuf.ToLong(); break;
				case PPHS_TIMESTAMP: rSet.T.Set(Pb.TempBuf, DATF_ISO8601, 0); break;
				case PPHS_CHANGESET: rSet.ChangeSet = Pb.TempBuf.ToInt64(); break;
				case PPHS_USER:      rSet.User = Pb.TempBuf; break;
				case PPHS_UID:       rSet.UserID = Pb.TempBuf.ToInt64(); break;
				case PPHS_TYPE:      rSet.TypeSymbID = O.CreateSymb(Pb.TempBuf); break;
				case PPHS_REF:       rSet.RefID = Pb.TempBuf.ToInt64(); break;
				case PPHS_ROLE:      rSet.RoleSymbID = O.CreateSymb(Pb.TempBuf); break;
				case PPHS_VISIBLE:
					if(Pb.TempBuf == "true")
						rSet.Visible = 1;
					else if(Pb.TempBuf == "false")
						rSet.Visible = 0;
					break;
			}
		}
	}
	return result;
}

int PrcssrOsm::StartElement(const char * pName, const char ** ppAttrList)
{
	int    ok = 1;
	int    tok = 0;
    (Pb.TempBuf = pName).ToLower();
    if(P_ShT) {
		uint _ut = 0;
		P_ShT->Search(Pb.TempBuf, &_ut, 0);
		tok = _ut;
    }
    switch(tok) {
    	case PPHS_OSM:    CurrentTagList.clear(); break;
		case PPHS_BOUNDS: CurrentTagList.clear(); break;
		case PPHS_NODE: // osm/node
			{
				Stat.NodeCount++;
				if((Stat.NodeCount % 1000000) == 0)
					if(RestoredStat.NodeCount > 0)
						PPWaitPercent((ulong)(((double)Stat.NodeCount / (double)RestoredStat.NodeCount) * 100.0), "Node");
					else
						PPWaitMsg(Pb.TempBuf.Z().Cat("Node").Space().Cat(Stat.NodeCount));
				PPOsm::Node new_node;
				ReadCommonAttrSet(ppAttrList, TempCaSet);
				new_node.ID = TempCaSet.ID;
				new_node.C.Set(TempCaSet.Lat, TempCaSet.Lon);
				if(!TempCaSet.Visible)
					new_node.T.SetInvisible();
				LastNode = new_node;
				if(Phase == phasePreprocess) {
					LatAccum.add(new_node.C.GetIntLat());
					LonAccum.add(new_node.C.GetIntLon());
				}
				else if(Phase == phaseImport) {
					new_node.T.V = GgtFinder.GetZIdx32(new_node.C);
					THROW_SL(NodeAccum.insert(&new_node));
				}
				CurrentTagList.clear();
			}
			break;
		case PPHS_WAY: // osm/way
			{
				if(Stat.WayCount == 0) {
					THROW(FlashNodeAccum(1));
				}
				Stat.WayCount++;
				if((Stat.WayCount % 100000) == 0)
					if(RestoredStat.WayCount > 0)
						PPWaitPercent((ulong)(((double)Stat.WayCount / (double)RestoredStat.WayCount) * 100.0), "Way");
					else
						PPWaitMsg(Pb.TempBuf.Z().Cat("Way").Space().Cat(Stat.WayCount));
				PPOsm::Way new_way;
				ReadCommonAttrSet(ppAttrList, TempCaSet);
				new_way.ID = TempCaSet.ID;
				if(!TempCaSet.Visible)
					new_way.T.SetInvisible();
				LastWay = new_way;
				CurrentTagList.clear();
			}
			break;
		case PPHS_ND: // osm/way/nd
			{
				const int upper_tok = TokPath.peek();
				if(upper_tok == PPHS_WAY) {
					ReadCommonAttrSet(ppAttrList, TempCaSet);
					if(TempCaSet.RefID)
						LastWay.NodeRefs.add(TempCaSet.RefID);
				}
				else {
					; // @error
				}
			}
			break;
		case PPHS_RELATION: // osm/relation
			{
				if(Stat.RelationCount == 0) {
					THROW(FlashNodeAccum(1));
				}
				Stat.RelationCount++;
				if((Stat.RelationCount % 100000) == 0)
					if(RestoredStat.RelationCount > 0)
						PPWaitPercent((ulong)(((double)Stat.RelationCount / (double)RestoredStat.RelationCount) * 100.0), "Relation");
					else
						PPWaitMsg(Pb.TempBuf.Z().Cat("Relation").Space().Cat(Stat.RelationCount));
				PPOsm::Relation new_rel;
				ReadCommonAttrSet(ppAttrList, TempCaSet);
				new_rel.ID = TempCaSet.ID;
				if(!TempCaSet.Visible)
					new_rel.T.SetInvisible();
				LastRel = new_rel;
				CurrentTagList.clear();
			}
			break;
		case PPHS_MEMBER: // osm/relation/member
			{
				const int upper_tok = TokPath.peek();
				if(upper_tok == PPHS_RELATION) {
					PPOsm::RelMember new_memb;
					ReadCommonAttrSet(ppAttrList, TempCaSet);
					new_memb.RefID = TempCaSet.RefID;
					new_memb.RoleSymbID = TempCaSet.RoleSymbID;
					new_memb.TypeSymbID = TempCaSet.TypeSymbID;
					THROW_SL(LastRel.MembList.insert(&new_memb));
				}
				else {
					; // @error
				}
			}
			break;
		case PPHS_TAG: // osm/node/tag || osm/relation/tag
			{
				const int parent_tok = TokPath.peek();
				int    tag_err = 0;
				Pb.TagKeyBuf = 0;
				Pb.TagValBuf = 0;
				for(uint i = 0; ppAttrList[i] != 0; i += 2) {
					const char * p_text_data = ppAttrList[i+1];
					if(p_text_data != 0) {
						if(sstreqi_ascii(ppAttrList[i], "k")) {
							if(Pb.TagKeyBuf.Empty())
								(Pb.TagKeyBuf = p_text_data).Strip();
							else
								tag_err = 1;
						}
						else if(sstreqi_ascii(ppAttrList[i], "v")) {
							if(Pb.TagValBuf.Empty())
								(Pb.TagValBuf = p_text_data).Strip();
							else
								tag_err = 2;
						}
					}
				}
				if(P_TagOutF || P_TagNodeOutF || P_TagWayOutF || P_TagRelOutF) {
					Pb.LineBuf.Z().Cat(Pb.TagKeyBuf).Tab().Cat(Pb.TagValBuf).CR();
					CALLPTRMEMB(P_TagOutF, WriteLine(Pb.LineBuf));
					if(parent_tok == PPHS_NODE) {
						CALLPTRMEMB(P_TagNodeOutF, WriteLine(Pb.LineBuf));
					}
					else if(parent_tok == PPHS_WAY) {
						CALLPTRMEMB(P_TagWayOutF, WriteLine(Pb.LineBuf));
					}
					else if(parent_tok == PPHS_RELATION) {
						CALLPTRMEMB(P_TagRelOutF, WriteLine(Pb.LineBuf));
					}
				}
				if(Pb.TagKeyBuf.Empty())
					tag_err = 3;
				else if(!tag_err) {
					if(0) { // На этапе предварительного анализа данных этот блок не нужен
						PPOsm::Tag new_tag;
						new_tag.KeySymbID = O.CreateSymb(Pb.TagKeyBuf);
						TempTagKeyList.add((long)new_tag.KeySymbID);
						if(TempTagKeyList.getCount() >= 1000) {
							TempTagKeyList.sortAndUndup();
							TagKeyList.add(&TempTagKeyList);
							TagKeyList.sortAndUndup();
							TempTagKeyList.clear();
						}
						if(Pb.TagValBuf.NotEmpty()) {
							new_tag.ValID = O.CreateSymb(Pb.TagValBuf);
						}
						else
							new_tag.ValID = 0;
						CurrentTagList.insert(&new_tag);
					}
				}
			}
			break;
		default:
			tok = 0;
			break;
    }
	TokPath.push(tok);
	CATCH
		Logger.LogLastError();
		SaxStop();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PrcssrOsm::GetPhaseSymb(long phase, SString & rSymb) const
{
	rSymb.Z();
	int    ok = 1;
	switch(phase) {
		case phasePreprocess: rSymb = "preprocess"; break;
		case phaseSortPreprcResults: rSymb = "SortPreprcResults"; break;
		case phaseAnlzPreprcResults: rSymb = "AnlzPreprcResults"; break;
		case phaseImport: rSymb = "import"; break;
		case phaseExtractSizes: rSymb = "extractsizes"; break;
		default: ok = 0;
	}
	return ok;
}

int SLAPI PrcssrOsm::WriteRoadStone(RoadStone & rRs)
{
	int    ok = 1;
	SString temp_buf;
	SString line_buf;
	SString file_name;
	SPathStruc ps(rRs.SrcFileName);
	ps.Nam.CatChar('-');
	THROW(GetPhaseSymb(rRs.Phase, temp_buf));
    ps.Nam.Cat(temp_buf);
    ps.Ext = "roadstone";
    ps.Merge(file_name);
	{
		SFile f_out(file_name, SFile::mWrite|SFile::mBinary);
		THROW_SL(f_out.IsValid());
		{
			SSerializeContext sctx;
			SBuffer buffer;
			THROW(rRs.Serialize(+1, buffer, &sctx));
			THROW_SL(f_out.Write(buffer));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrOsm::ReadRoadStone(long phase, RoadStone & rRs)
{
	int    ok = 1;
	SString temp_buf;
	SString file_name;
	SPathStruc ps(P.SrcFileName);
	ps.Nam.CatChar('-');
	THROW(GetPhaseSymb(phase, temp_buf));
    ps.Nam.Cat(temp_buf);
    ps.Ext = "roadstone";
    ps.Merge(file_name);
    if(fileExists(file_name)) {
		SFile f_in(file_name, SFile::mRead|SFile::mBinary);
		THROW_SL(f_in.IsValid());
		{
			SSerializeContext sctx;
			SBuffer buffer;
			THROW_SL(f_in.Read(buffer));
			THROW(rRs.Serialize(-1, buffer, &sctx));
		}
    }
    else
		ok = -1;
	CATCHZOK
	return ok;
}

int FASTCALL PrcssrOsm::FlashNodeAccum(int force)
{
	int    ok = 1;
	int    do_store_roadstone = 0;
	static const uint nodewayref_accum_limit = 1024*1024;
	static const uint way_accum_limit = 256 * 1024;
	if(Phase == phasePreprocess) {
		static const uint pt_accum_limit = 8192;
		{
			const uint _count = LatAccum.getCount();
			if(_count && (force || _count >= pt_accum_limit)) {
				if(P_LatOutF) {
					LatAccum.sort();
					Pb.LineBuf.Z();
					for(uint i = 0; i < _count; i++) {
						Pb.LineBuf.Cat(LatAccum.get(i)).CR();
					}
					P_LatOutF->WriteLine(Pb.LineBuf);
				}
				if(force)
					LatAccum.freeAll();
				else
					LatAccum.clear();
			}
		}
		{
			const uint _count = LonAccum.getCount();
			if(_count && (force || _count >= pt_accum_limit)) {
				if(P_LonOutF) {
					LonAccum.sort();
					Pb.LineBuf.Z();
					for(uint i = 0; i < _count; i++) {
						Pb.LineBuf.Cat(LonAccum.get(i)).CR();
					}
					P_LonOutF->WriteLine(Pb.LineBuf);
				}
				if(force)
					LonAccum.freeAll();
				else
					LonAccum.clear();
			}
		}
		{
			const uint _count = NodeWayAssocAccum.getCount();
			if(_count && (force || _count >= nodewayref_accum_limit)) {
				if(P_NodeToWayAssocOutF) {
					NodeWayAssocAccum.Sort();
					Pb.LineBuf.Z();
					for(uint i = 0; i < _count; i++) {
						const LLAssoc & r_assoc = NodeWayAssocAccum.at(i);
						Pb.LineBuf.Cat(r_assoc.Key).Tab().Cat(r_assoc.Val).CR();
					}
					P_NodeToWayAssocOutF->WriteLine(Pb.LineBuf);
				}
				if(force)
					NodeWayAssocAccum.freeAll();
				else
					NodeWayAssocAccum.clear();
			}
		}
		{
			const uint _count = WayAccum.getCount();
			if(_count && (force || _count >= way_accum_limit)) {
				WayAccum.freeAll();
			}
		}
	}
	else if(Phase == phaseImport) {
		static const uint node_accum_limit = 1024*1024;
		{
			const uint _count = NodeAccum.getCount();
			if(_count && (force || _count >= node_accum_limit)) {
				const PPOsm::Node & r_last_node = NodeAccum.at(_count-1);
				if(force || (r_last_node.ID & 0x7f) == 0x7f) {
					if(!P_RoadStoneStat || Stat.NodeCount > P_RoadStoneStat->Stat.NodeCount) {
						if(P_RoadStoneStat) {
							Stat.WayList = P_RoadStoneStat->Stat.WayList;
							Stat.NcList = P_RoadStoneStat->Stat.NcList;
							ZDELETE(P_RoadStoneStat);
						}
						SrDatabase * p_db = O.GetDb();
						if(p_db) {
							PROFILE_START
							const int dont_check_existance = 1;
							//LLAssocArray node_to_way_assc_list;
							Pb.NodeToWayAsscList.clear();
							LLAssocArray * p_node_to_way_assc_list = 0;
							if(P_NodeToWayAssocInF) {
								SString _key_buf, _val_buf;
								int64 first_node_id = (int64)NodeAccum.at(0).ID;
								int64 last_node_id = (int64)NodeAccum.at(_count-1).ID;
								PROFILE_START
								if(LastNodeToWayAssoc.Key && LastNodeToWayAssoc.Val) {
									if(LastNodeToWayAssoc.Key >= first_node_id && LastNodeToWayAssoc.Key <= last_node_id) {
										THROW_SL(Pb.NodeToWayAsscList.insert(&LastNodeToWayAssoc));
									}
								}
								PROFILE_END
								PROFILE_START
								while(P_NodeToWayAssocInF->ReadLine(Pb.LineBuf)) {
									Pb.LineBuf.Chomp().Strip();
									if(Pb.LineBuf.Divide('\t', _key_buf, _val_buf) > 0) {
										LastNodeToWayAssoc.Key = _key_buf.ToInt64();
										LastNodeToWayAssoc.Val = _val_buf.ToInt64();
										if(NodeAccum.bsearch(&LastNodeToWayAssoc.Key, 0, CMPF_INT64)) {
											THROW_SL(Pb.NodeToWayAsscList.insert(&LastNodeToWayAssoc));
										}
										else if(LastNodeToWayAssoc.Key > last_node_id) {
											break;
										}
									}
								}
								PROFILE_END
							}
							PROFILE_START
							THROW(p_db->StoreGeoNodeList(NodeAccum, &Pb.NodeToWayAsscList, dont_check_existance, &Stat.NcList));
							PROFILE_END
							PROFILE_END
						}
						OutputStat(0);
						// (При работе с RoadSton'ом - не катит) assert((Stat.GetNcActualCount() + Stat.GetNcProcessedCount()) == Stat.NodeCount);
						do_store_roadstone = 1;
					}
					if(force)
						NodeAccum.freeAll();
					else
						NodeAccum.clear();
				}
			}
		}
		{
			const uint _count = WayAccum.getCount();
			if(_count && (force || _count >= way_accum_limit)) {
				if(!P_RoadStoneStat || Stat.WayCount > P_RoadStoneStat->Stat.WayCount) {
					if(P_RoadStoneStat) {
						Stat.WayList = P_RoadStoneStat->Stat.WayList;
						Stat.NcList = P_RoadStoneStat->Stat.NcList;
						ZDELETE(P_RoadStoneStat);
					}
					size_t offs = 0;
					SrDatabase * p_db = O.GetDb();
					if(p_db) {
						THROW(p_db->StoreGeoWayList(WayAccum, &Stat.WayList));
					}
					OutputStat(0);
					assert((Stat.GetNcActualCount() + Stat.GetNcProcessedCount()) == Stat.NodeCount);
					assert(Stat.GetWsCount() + Stat.GetWsProcessedCount() == Stat.WayCount);
					do_store_roadstone = 1;
				}
				WayAccum.freeAll();
			}
		}
#if 0 // Очень медленный участок кода - ищем другие подходы {
		{
			const uint _count = NodeWayAssocAccum.getCount();
			if(_count && (force || _count >= nodewayref_accum_limit)) {
				NodeWayAssocAccum.Sort();
				SrDatabase * p_db = O.GetDb();
				if(p_db) {
					THROW(p_db->StoreGeoNodeWayRefList(NodeWayAssocAccum));
				}
				if(force)
					NodeWayAssocAccum.freeAll();
				else
					NodeWayAssocAccum.clear();
				do_store_roadstone = 1;
			}
		}
#else
		{
			const uint _count = NodeWayAssocAccum.getCount();
			if(_count && (force || _count >= nodewayref_accum_limit)) {
				if(force)
					NodeWayAssocAccum.freeAll();
				else
					NodeWayAssocAccum.clear();
			}
		}
#endif
		if(do_store_roadstone) {
			ZDELETE(P_RoadStoneStat);
			RoadStone rs;
			rs.SrcFileName = P.SrcFileName;
			rs.Phase = Phase;
			rs.Stat = Stat;
			THROW(WriteRoadStone(rs));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrOsm::ProcessWaySizes()
{
	int    ok = 1;
	SrDatabase * p_db = O.GetDb();
	if(p_db && P_SizeOutF) {
		const uint max_out_accum_count = 1000;
		const uint64 stat_way_count = NZOR(Stat.WayCount, RestoredStat.WayCount);
		uint  out_accum_count = 0;
		SString out_file_name;
		uint64 way_count = 0;
		SString line_buf;
		PPOsm::Way way;
		PPOsm::WayBuffer way_buf;
		TSVector <PPOsm::Node> way_node_list; // @v9.8.6 TSArray-->TSVector
		BDbCursor curs(*p_db->P_GwT, 0);
		BDbTable::Buffer key_buf, data_buf;
		key_buf.Alloc(32);
		data_buf.Alloc(8192);

		long   prev_file_no = -1;
		int    skip_this_file_no = 0;
		SString preserve_filnam;
		SPathStruc ps_out_filename(P_SizeOutF->GetName());
		preserve_filnam = ps_out_filename.Nam;

		if(curs.Search(key_buf, data_buf, spFirst)) do {
			way_count++;
			const long file_no = (long)(way_count / 1000000);
            if(file_no != prev_file_no) {
				(ps_out_filename.Nam = preserve_filnam).CatChar('-').Cat(file_no+1);
                ps_out_filename.Merge(out_file_name);
                if(fileExists(out_file_name)) {
					skip_this_file_no = 1;
                }
                else {
					skip_this_file_no = 0;
					if(out_accum_count) {
						P_SizeOutF->WriteLine(line_buf);
						line_buf.Z();
						out_accum_count = 0;
					}
					P_SizeOutF->Close();
					THROW_SL(P_SizeOutF->Open(out_file_name, SFile::mWrite));
                }
				prev_file_no = file_no;
            }
			if(!skip_this_file_no) {
				uint64 way_id = 0;
				{
					size_t dbsz = 0;
					const void * p_dbptr = data_buf.GetPtr(&dbsz);
					way_buf.SetBuffer(p_dbptr, dbsz);
				}
				{
					size_t dbsz = 0;
					const void * p_dbptr = key_buf.GetPtr(&dbsz);
					way_id = sexpanduint64(p_dbptr, dbsz);
				}
				if(way_buf.Get(way_id, &way)) {
					p_db->P_GnT->GetWayNodes(way, way_node_list);
					uint wn_count = way_node_list.getCount();
					double max_distance = 0.0;
					if(wn_count > 1) {
						if(way_node_list.at(0).ID == way_node_list.at(wn_count-1).ID) {
							//
							// Для замкнутого контура исключим последнюю точку (равную первой) из расчета расстояний - сэкономим время.
							//
							wn_count--;
						}
						for(uint j = 0; j < wn_count; j++) {
							const PPOsm::Node & r_node = way_node_list.at(j);
							SGeoPosLL p1(r_node.C.GetLat(), r_node.C.GetLon());
							for(uint j2 = j+1; j2 < wn_count; j2++) {
								const PPOsm::Node & r_node2 = way_node_list.at(j2);
								SGeoPosLL p2(r_node2.C.GetLat(), r_node2.C.GetLon());
								double distance = 0.0;
								G.Inverse(p1, p2, &distance, 0, 0, 0, 0, 0, 0);
								SETMAX(max_distance, distance);
							}
						}
					}
					line_buf./*Cat(p_way->ID).Tab().*/Cat((uint)R0(max_distance)).CR();
					out_accum_count++;
					if(out_accum_count >= max_out_accum_count) {
						P_SizeOutF->WriteLine(line_buf);
						line_buf.Z();
						out_accum_count = 0;
					}
				}
			}
			PPWaitPercent((ulong)(((double)way_count / (double)stat_way_count) * 100.0), "WaySizes");
		} while(curs.Search(key_buf, data_buf, spNext));
		P_SizeOutF->WriteLine(line_buf);
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrOsm::OutputStat(int detail)
{
	SString stat_info_buf;
	stat_info_buf.CatEq("Node", Stat.NodeCount).CatDiv(';', 2).
		CatEq("Way", Stat.WayCount).CatDiv(';', 2).
		CatEq("Relation", Stat.RelationCount).CatDiv(';', 2);
	PPLogMessage(PPFILNAM_INFO_LOG, stat_info_buf, LOGMSGF_TIME);
	if(detail) {
		if(Stat.NcList.getCount()) {
			for(uint i = 0; i < Stat.NcList.getCount(); i++) {
				const PPOsm::NodeClusterStatEntry & r_entry = Stat.NcList.at(i);
				stat_info_buf.Z().CatEq("NodeLogicalCount", r_entry.LogicalCount).CatDiv(';', 2).
					CatEq("NodeClusterCount", r_entry.ClusterCount).CatDiv(';', 2).
					CatEq("NodeProcessesCount", r_entry.ProcessedCount).CatDiv(';', 2).
					CatEq("NodeActualCount", r_entry.ActualCount).CatDiv(';', 2).
					CatEq("NodeSize", r_entry.Size).CatDiv(';', 2);
				PPLogMessage(PPFILNAM_INFO_LOG, stat_info_buf, 0);
			}
		}
		if(Stat.WayList.getCount()) {
			for(uint i = 0; i < Stat.WayList.getCount(); i++) {
				const PPOsm::WayStatEntry & r_entry = Stat.WayList.at(i);
				stat_info_buf.Z().CatEq("WayRefCount", r_entry.RefCount).CatDiv(';', 2).
					CatEq("WayProcessesCount", r_entry.ProcessedCount).CatDiv(';', 2).
					CatEq("WayCount", r_entry.WayCount).CatDiv(';', 2).
					CatEq("WaySize", r_entry.Size).CatDiv(';', 2);
				PPLogMessage(PPFILNAM_INFO_LOG, stat_info_buf, 0);
			}
		}
	}
	return 1;
}

int PrcssrOsm::EndElement(const char * pName)
{
	int    tok = 0;
	int    ok = TokPath.pop(tok);
	if(tok == PPHS_NODE) {
		if(CurrentTagList.getCount() == 0)
			Stat.NakedNodeCount++;
	}
	else if(tok == PPHS_WAY) {
        PPOsm::Way * p_new_way = WayAccum.CreateNewItem();
		THROW_SL(p_new_way);
		*p_new_way = LastWay;
		{
			const int64 way_id = p_new_way->ID;
			const uint nrc = p_new_way->NodeRefs.getCount();
			for(uint i = 0; i < nrc; i++) {
				THROW_SL(NodeWayAssocAccum.Add(p_new_way->NodeRefs.get(i), way_id, 0));
			}
		}
		LastWay.Clear();
	}
	THROW(FlashNodeAccum(BIN(tok == PPHS_OSM)));
	if(oneof3(tok, PPHS_NODE, PPHS_WAY, PPHS_RELATION)) {
		//PPUPRF_OSMXMLPARSETAG
        CALLPTRMEMB(P_Ufp, CommitAndRestart());
	}
	{
		const uint64 total_count = Stat.NodeCount + Stat.WayCount + Stat.RelationCount;
		if(tok == PPHS_OSM/* || (total_count % 1000000) == 0*/) {
			OutputStat(1);
		}
	}
	CATCH
		Logger.LogLastError();
		SaxStop();
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}

//static
void PrcssrOsm::Scb_StartDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PrcssrOsm, ptr, StartDocument());
}

//static
void PrcssrOsm::Scb_EndDocument(void * ptr)
{
	CALLTYPEPTRMEMB(PrcssrOsm, ptr, EndDocument());
}

//static
void PrcssrOsm::Scb_StartElement(void * ptr, const xmlChar * pName, const xmlChar ** ppAttrList)
{
	CALLTYPEPTRMEMB(PrcssrOsm, ptr, StartElement((const char *)pName, (const char **)ppAttrList));
}

//static
void PrcssrOsm::Scb_EndElement(void * ptr, const xmlChar * pName)
{
	CALLTYPEPTRMEMB(PrcssrOsm, ptr, EndElement((const char *)pName));
}

IMPL_CMPCFUNC(STRINT64, p1, p2)
{
	int64 v1 = _atoi64(static_cast<const char *>(p1));
	int64 v2 = _atoi64(static_cast<const char *>(p2));
	return CMPSIGN(v1, v2);
}

IMPL_CMPCFUNC(STRUTF8NOCASE, p1, p2)
{
	int   si = -1;
	const size_t len1 = sstrlen(static_cast<const char *>(p1));
	const size_t len2 = sstrlen(static_cast<const char *>(p2));
	if(len1 == len2) {
		//
		// Простые случая без перевода строк в unicode
		//
		if(len1 == 0)
			si = 0;
		else if(memcmp(p1, p2, len1) == 0)
			si = 0;
	}
	if(si != 0) {
		const int a1 = sisascii(static_cast<const char *>(p1), len1);
		const int a2 = sisascii(static_cast<const char *>(p2), len2);
		if(a1 && a2) {
			si = strcmp(static_cast<const char *>(p1), static_cast<const char *>(p2));
		}
		else {
			SStringU s1;
			SStringU s2;
			s1.CopyFromUtf8(static_cast<const char *>(p1), len1);
			s2.CopyFromUtf8(static_cast<const char *>(p2), len2);
			//s1.ToLower();
			//s2.ToLower();
			si = s1.Cmp(s2);
		}
	}
	return si;
}
//
// Descr: Построение пропорциональной гео-координатной решетки
//
int SLAPI PrcssrOsm::CreateGeoGridTab(const char * pSrcFileName, uint lowDim, uint uppDim, TSCollection <SGeoGridTab> & rGridList)
{
	rGridList.freeAll();

	int    ok = -1;
	uint * p_last_count = 0;
	long * p_last_coord = 0;
	SString lat_file_name;
	SString lon_file_name;
	{
		SPathStruc ps(pSrcFileName);
		{
			ps.Split(pSrcFileName);
			ps.Nam.CatChar('-').Cat("lat").CatChar('-').Cat("sorted");
			ps.Ext = "txt";
			ps.Merge(lat_file_name);
		}
		{
			ps.Split(pSrcFileName);
			ps.Nam.CatChar('-').Cat("lon").CatChar('-').Cat("sorted");
			ps.Ext = "txt";
			ps.Merge(lon_file_name);
		}
	}
	if(fileExists(lat_file_name) && fileExists(lon_file_name)) {
		SString fmt_buf, msg_buf;
		SString line_buf;
        uint64 lat_count = 0;
        uint64 lon_count = 0;
        int64  f_size_lat = 0;
        int64  f_size_lon = 0;
        uint64 line_count_lat = 0;
        uint64 line_count_lon = 0;
        SFile f_lat(lat_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
        SFile f_lon(lon_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
        THROW_SL(f_lat.IsValid());
        THROW_SL(f_lon.IsValid());
		f_lat.CalcSize(&f_size_lat);
		f_lon.CalcSize(&f_size_lon);
		//PPTXT_FILELINECOUNTING            "Подсчет количества записей в файле '%s'"
		PPLoadText(PPTXT_FILELINECOUNTING, fmt_buf);
        {
        	msg_buf.Printf(fmt_buf, "lat");
			while(f_lat.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				const char firstc = line_buf.C(0);
				if(firstc == '-' || (firstc >= '0' && firstc <= '9'))
					lat_count++;
				line_count_lat++;
				if((line_count_lat % 1000000) == 0) {
					const int64 fpos = f_lat.Tell64();
                    PPWaitPercent((ulong)(100 * fpos / f_size_lat), msg_buf);
				}
			}
			f_lat.Seek64(0);
        }
        {
        	msg_buf.Printf(fmt_buf, "lon");
			while(f_lon.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				const char firstc = line_buf.C(0);
				if(firstc == '-' || (firstc >= '0' && firstc <= '9'))
					lon_count++;
				line_count_lon++;
				if((line_count_lon % 1000000) == 0) {
					const int64 fpos = f_lon.Tell64();
                    PPWaitPercent((ulong)(100 * fpos / f_size_lon), msg_buf);
				}
			}
			f_lon.Seek64(0);
        }
       	PPLoadText(PPTXT_BUILDINGGEOGRID, fmt_buf);
        const uint grid_count = (uppDim - lowDim) + 1;
        {
			for(uint i = lowDim; i <= uppDim; i++) {
				SGeoGridTab * p_new_grid = new SGeoGridTab(i);
				THROW_MEM(p_new_grid);
				p_new_grid->SetSrcCountLat(lat_count);
				p_new_grid->SetSrcCountLon(lon_count);
				THROW_SL(rGridList.insert(p_new_grid));
			}
        }
        assert(rGridList.getCount() == grid_count);
        THROW_MEM(p_last_count = new uint[grid_count]);
        THROW_MEM(p_last_coord = new long[grid_count]);
        {
        	uint64 line_no = 0;
			msg_buf.Printf(fmt_buf, "lat");
			//
        	memzero(p_last_count, sizeof(p_last_count[0]) * grid_count);
			memzero(p_last_coord, sizeof(p_last_coord[0]) * grid_count);
			while(f_lat.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				const char firstc = line_buf.C(0);
				if(firstc == '-' || (firstc >= '0' && firstc <= '9')) {
                    const long c = line_buf.ToLong();
                    for(uint i = 0; i < grid_count; i++) {
						SGeoGridTab * p_grid = rGridList.at(i);
						assert(p_grid);
                        if(p_last_count[i] >= p_grid->GetDensityLat() && p_last_coord[i] != c) {
                            THROW_SL(p_grid->AddThresholdLat(p_last_coord[i]));
                            p_last_count[i] = 0;
                        }
                        p_last_count[i]++;
						p_last_coord[i] = c;
                    }
				}
				line_no++;
				if((line_no % 1000000) == 0) {
					PPWaitPercent((ulong)(100 * line_no / line_count_lat), msg_buf);
				}
			}
			{
				//
				// Финишная вставка последнего порога
				//
				for(uint i = 0; i < grid_count; i++) {
					SGeoGridTab * p_grid = rGridList.at(i);
					assert(p_grid);
					THROW_SL(p_grid->AddThresholdLat(p_last_coord[i]));
					// Финишная проверка правильности
					// !При построении решетки по ограниченному ареалу может возникнуть ситуация, когда p_grid->GetCountLat() < (1UL << p_grid->GetDim())
					// assert(p_grid->GetCountLat() == (1UL << p_grid->GetDim()));
				}
			}
			f_lat.Seek64(0);
        }
        {
        	uint64 line_no = 0;
			msg_buf.Printf(fmt_buf, "lon");
			//
        	memzero(p_last_count, sizeof(p_last_count[0]) * grid_count);
			memzero(p_last_coord, sizeof(p_last_coord[0]) * grid_count);
			const long m180val = -1800000000;
			const long p180val =  1800000000;
            uint minus_180_count = 0;
			while(f_lon.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				const char firstc = line_buf.C(0);
				if(firstc == '-' || (firstc >= '0' && firstc <= '9')) {
                    const long c = line_buf.ToLong();
                    if(c == m180val) {
						minus_180_count++;
                    }
                    else {
						for(uint i = 0; i < grid_count; i++) {
							SGeoGridTab * p_grid = rGridList.at(i);
							assert(p_grid);
							if(p_last_count[i] >= p_grid->GetDensityLon() && p_last_coord[i] != c) {
								THROW_SL(p_grid->AddThresholdLon(p_last_coord[i]));
								p_last_count[i] = 0;
							}
							p_last_count[i]++;
							p_last_coord[i] = c;
						}
                    }
				}
				line_no++;
				if((line_no % 1000000) == 0) {
					PPWaitPercent((ulong)(100 * line_no / line_count_lon), msg_buf);
				}
			}
			{
				//
				// Перенос долготы -180deg в конец списка как 180deg
				//
				for(uint m180idx = 0; m180idx < minus_180_count; m180idx++) {
					const long c = p180val;
					for(uint i = 0; i < grid_count; i++) {
						SGeoGridTab * p_grid = rGridList.at(i);
						assert(p_grid);
						if(p_last_count[i] >= p_grid->GetDensityLon() && p_last_coord[i] != c) {
							THROW_SL(p_grid->AddThresholdLon(p_last_coord[i]));
							p_last_count[i] = 0;
						}
						p_last_count[i]++;
						p_last_coord[i] = c;
					}
				}
			}
			{
				//
				// Финишная вставка последнего порога
				//
				for(uint i = 0; i < grid_count; i++) {
					SGeoGridTab * p_grid = rGridList.at(i);
					assert(p_grid);
					THROW_SL(p_grid->AddThresholdLon(p_last_coord[i]));
					// Финишная проверка правильности
					// !При построении решетки по ограниченному ареалу может возникнуть ситуация, когда p_grid->GetCountLat() < (1UL << p_grid->GetDim())
					// assert(p_grid->GetCountLon() == (1UL << p_grid->GetDim()));
				}
			}
			f_lat.Seek64(0);
        }
	}
	CATCHZOK
	delete [] p_last_count;
	delete [] p_last_coord;
	return ok;
}

//static
int PrcssrOsm::SortCbProc(const SFileSortProgressData * pInfo)
{
	if(pInfo) {
		SString msg_buf;
		const PrcssrOsm * p_prcr = (const PrcssrOsm *)pInfo->ExtraPtr;
		if(p_prcr) {
			SString file_name;
			SPathStruc ps(pInfo->P_SrcFileName);
			ps.Merge(SPathStruc::fNam|SPathStruc::fExt, file_name);
			if(pInfo->Phase == 1) {
				msg_buf.Printf(p_prcr->FmtMsg_SortSplit, file_name.cptr());
				msg_buf.Space().CatParStr(pInfo->SplitThreadCount);
				PPWaitPercent((ulong)(100LL * pInfo->SplitBytesRead / pInfo->TotalFileSize), msg_buf);
			}
			else if(pInfo->Phase == 2) {
				msg_buf.Printf(p_prcr->FmtMsg_SortMerge, file_name.cptr());
				PPWaitMsg(msg_buf);
			}
		}
	}
	return 1;
}

static SString & MakeSuffixedTxtFileName(const SString & rSrcFileName, const char * pSuffix, SPathStruc & rPs, SString & rResult)
{
	rPs.Split(rSrcFileName);
	rPs.Nam.CatChar('-').Cat(pSuffix);
	rPs.Ext = "txt";
	rPs.Merge(rResult);
	return rResult;
}

int SLAPI PrcssrOsm::SortFile(const char * pSrcFileName, const char * pSuffix, CompFunc fcmp)
{
	int    ok = -1;
	//const size_t sort_max_chunk = 32 * 1024 * 1024;
	//const uint sort_max_chunk_count = 16;
	SFile::SortParam sp;
	sp.MaxChunkSize = 32 * 1024 * 1024;
	sp.MaxChunkCount = 16;
	sp.MaxThread = 6;
	sp.ProgressCbProc = PrcssrOsm::SortCbProc;
	sp.ProgressCbExtraPtr = this;

	SString in_file_name;
	SString out_file_name;
	SPathStruc ps;
	{
		ps.Split(pSrcFileName);
		ps.Nam.CatChar('-').Cat(pSuffix);
		ps.Ext = "txt";
		ps.Merge(in_file_name);
	}
	if(fileExists(in_file_name)) {
		{
			ps.Split(pSrcFileName);
			ps.Nam.CatChar('-').Cat(pSuffix).CatChar('-').Cat("sorted");
			ps.Ext = "txt";
			ps.Merge(out_file_name);
		}
		if(!fileExists(out_file_name)) {
			PROFILE_START
			THROW_SL(SFile::Sort(in_file_name, out_file_name, fcmp, &sp/*sort_max_chunk, sort_max_chunk_count*/));
			PROFILE_END
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrOsm::Run()
{
	int    ok = 1;
	const  SString file_name = P.SrcFileName;
	SString log_file_name;
	SString in_file_name;
	SString out_file_name;
	SString temp_buf;
	SPathStruc ps;
	const  char * p_db_path = 0;
	PPWait(1);
	RestoredStat.Clear();
	{
		ps.Split(file_name);
		ps.Ext = "log";
		ps.Merge(log_file_name);
		if(fileExists(log_file_name)) {
			SFile f_log_pre(log_file_name, SFile::mRead);
			if(f_log_pre.IsValid()) {
				uint line_no = 0;
				SString key_buf, val_buf;
				int  all_done = 0;
				while(!all_done && f_log_pre.ReadLine(temp_buf)) {
					line_no++;
					temp_buf.Chomp().Strip();
					if(line_no == 1) {
						StringSet ss;
						temp_buf.Tokenize(";", ss);
						if(ss.getCount() > 1) {
							for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
								if(temp_buf.Strip().Divide('=', key_buf, val_buf) > 0) {
									key_buf.Strip();
									val_buf.Strip();
									if(key_buf.CmpNC("NodeCount") == 0)
										RestoredStat.NodeCount = val_buf.ToInt64();
									else if(key_buf.CmpNC("NakedNodeCount") == 0)
										RestoredStat.NakedNodeCount = val_buf.ToInt64();
									else if(key_buf.CmpNC("WayCount") == 0)
										RestoredStat.WayCount = val_buf.ToInt64();
									else if(key_buf.CmpNC("RelationCount") == 0)
										RestoredStat.RelationCount = val_buf.ToInt64();
								}
							}
							all_done = 1;
						}
					}
					if(!all_done) {
						if(temp_buf.Strip().Divide('=', key_buf, val_buf) > 0) {
							key_buf.Strip();
							val_buf.Strip();
							if(key_buf.CmpNC("NodeCount") == 0)
								RestoredStat.NodeCount = val_buf.ToInt64();
							else if(key_buf.CmpNC("NakedNodeCount") == 0)
								RestoredStat.NakedNodeCount = val_buf.ToInt64();
							else if(key_buf.CmpNC("WayCount") == 0)
								RestoredStat.WayCount = val_buf.ToInt64();
							else if(key_buf.CmpNC("RelationCount") == 0)
								RestoredStat.RelationCount = val_buf.ToInt64();
						}
					}
				}
			}
		}
	}
    O.LoadGeoGrid();
	if(P.Flags & PrcssrOsmFilt::fPreprocess) {
		Phase = phasePreprocess;
		xmlSAXHandler saxh_addr_obj;
		MEMSZERO(saxh_addr_obj);
		saxh_addr_obj.startDocument = Scb_StartDocument;
		saxh_addr_obj.endDocument = Scb_EndDocument;
		saxh_addr_obj.startElement = Scb_StartElement;
		saxh_addr_obj.endElement = Scb_EndElement;
		{
			{
				struct __OFE { SFile ** PP_F; const char * P_Sfx; } __OFE_list[] = {
					{ &P_LatOutF, "lat" }, { &P_LonOutF, "lon" }, { &P_TagOutF, "tag" }, { &P_TagNodeOutF, "tagnode" },
					{ &P_TagWayOutF, "tagway" }, { &P_TagRelOutF, "tagrel" }/*, { &P_SizeOutF, "distance" }*/, { &P_NodeToWayAssocOutF, "nodewayassoc" }
				};
				for(uint i = 0; i < SIZEOFARRAY(__OFE_list); i++) {
					THROW_MEM(*__OFE_list[i].PP_F = new SFile(MakeSuffixedTxtFileName(file_name, __OFE_list[i].P_Sfx, ps, out_file_name), SFile::mWrite));
					THROW_SL((*__OFE_list[i].PP_F)->IsValid());
				}
			}
			THROW_MEM(SETIFZ(P_Ufp, new PPUserFuncProfiler(PPUPRF_OSMXMLPARSETAG)));
			PROFILE_START
			THROW(SaxParseFile(&saxh_addr_obj, file_name) == 0);
			PROFILE_END
			{
				ZDELETE(P_LatOutF);
				ZDELETE(P_LonOutF);
				ZDELETE(P_TagOutF);
				ZDELETE(P_NodeToWayAssocOutF);
				ZDELETE(P_TagNodeOutF);
				ZDELETE(P_TagWayOutF);
				ZDELETE(P_TagRelOutF);
				ZDELETE(P_SizeOutF);
			}
			{
				if(0) {
					TempTagKeyList.sortAndUndup();
					TagKeyList.add(&TempTagKeyList);
					TagKeyList.sortAndUndup();
					TempTagKeyList.clear();
					O.BuildHashAssoc();
				}
				{
					SFile f_log(log_file_name, SFile::mWrite);
					temp_buf.Z();
					temp_buf.
						CatEq("NodeCount", Stat.NodeCount).CR().
						CatEq("NakedNodeCount", Stat.NakedNodeCount).CR().
						CatEq("WayCount", Stat.WayCount).CR().
						CatEq("RelationCount", Stat.RelationCount).CR();
					f_log.WriteLine(temp_buf);
					if(0) {
						for(uint i = 0; i < TagKeyList.getCount(); i++) {
							const long tag_symb_id = TagKeyList.get(i);
							if(O.GetSymbByID(tag_symb_id, temp_buf)) {
								f_log.WriteLine(temp_buf.CR());
							}
						}
					}
				}
			}
		}
	}
	if(P.Flags & PrcssrOsmFilt::fSortPreprcResults) {
		Phase = phaseSortPreprcResults;
        PPLoadText(PPTXT_SORTSPLIT, FmtMsg_SortSplit);
		PPLoadText(PPTXT_SORTMERGE, FmtMsg_SortMerge);
		THROW(SortFile(file_name, "distance", PTR_CMPCFUNC(STRINT64))); // @v9.8.3
		THROW(SortFile(file_name, "lat", PTR_CMPCFUNC(STRINT64)));
		THROW(SortFile(file_name, "lon", PTR_CMPCFUNC(STRINT64)));
		THROW(SortFile(file_name, "nodewayassoc", PTR_CMPCFUNC(STRINT64)));
		THROW(SortFile(file_name, "tagrel", PTR_CMPCFUNC(STRUTF8NOCASE)));
		THROW(SortFile(file_name, "tagway", PTR_CMPCFUNC(STRUTF8NOCASE)));
		THROW(SortFile(file_name, "tagnode", PTR_CMPCFUNC(STRUTF8NOCASE)));
		THROW(SortFile(file_name, "tag", PTR_CMPCFUNC(STRUTF8NOCASE)));
	}
	if(P.Flags & PrcssrOsmFilt::fAnlzPreprcResults) {
		{
			int   test_result = 1;
			TSCollection <SGeoGridTab> grid_list;
			Phase = phaseAnlzPreprcResults;
			THROW(CreateGeoGridTab(file_name, 8, 16, grid_list));
			for(uint i = 0; i < grid_list.getCount(); i++) {
				SGeoGridTab * p_grid = grid_list.at(i);
				if(p_grid) {
					{
						ps.Split(file_name);
						ps.Nam.CatChar('-').Cat("grid").CatChar('-').Cat(p_grid->GetDim());
						ps.Ext = "txt";
						ps.Merge(out_file_name);
					}
					p_grid->Save(out_file_name);
					{
						SGeoGridTab test_tab(16);
						int lr = test_tab.Load(out_file_name);
						if(!lr)
							test_result = 0;
						else {
							int cr = test_tab.IsEqual(*p_grid);
							if(!cr)
								test_result = 0;
							else {
								//PPTXT_GEOGRIDTABWRRDSUCC          "Тест записи/чтения таблицы пропорциональное гео-решетки '@zstr' прошел успешно"
								PPFormatT(PPTXT_GEOGRIDTABWRRDSUCC, &temp_buf, out_file_name.cptr());
								PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
							}
						}
					}
				}
			}
		}
		{
			{
				ps.Split(file_name);
				ps.Nam.CatChar('-').Cat("tag").CatChar('-').Cat("sorted");
				ps.Ext = "txt";
				ps.Merge(in_file_name);
			}
			if(fileExists(in_file_name)) {
				{
					ps.Split(file_name);
					ps.Nam.CatChar('-').Cat("tag").CatChar('-').Cat("processed");
					ps.Ext = "txt";
					ps.Merge(out_file_name);
				}
				if(!fileExists(out_file_name)) {
					const size_t max_out_len = 1024 * 1024;
                    SFile f_in(in_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
                    SFile f_out(out_file_name, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
                    SString line_buf;
                    SString out_buf;
                    SString tag_key;
                    SString tag_val;
                    SString prev_tag_key;
                    SString prev_tag_val;
                    uint   key_count = 0;
                    uint   val_count = 0;
                    uint64 line_no = 0;
                    while(f_in.ReadLine(line_buf)) {
                        line_buf.Chomp().Divide('\t', tag_key, tag_val);
                        if(line_no) {
							if(tag_key != prev_tag_key) {
								out_buf.Cat(prev_tag_key).Tab().Cat(prev_tag_val).Tab().Cat(val_count).CRB();
								out_buf.Cat(prev_tag_key).Tab(2).Cat(key_count).CRB();
								key_count = 0;
								val_count = 0;
							}
							else if(tag_val != prev_tag_val) {
								out_buf.Cat(prev_tag_key).Tab().Cat(prev_tag_val).Tab().Cat(val_count).CRB();
								val_count = 0;
							}
							if(out_buf.Len() >= max_out_len) {
								f_out.WriteLine(out_buf);
								out_buf.Z();
							}
						}
						key_count++;
						val_count++;
						prev_tag_key = tag_key;
						prev_tag_val = tag_val;
						line_no++;
                    }
					if(line_no) {
						out_buf.Cat(prev_tag_key).Tab().Cat(prev_tag_val).Tab().Cat(val_count).CRB();
						out_buf.Cat(prev_tag_key).Tab(2).Cat(key_count).CRB();
						f_out.WriteLine(out_buf);
						out_buf.Z();
					}
				}
			}
		}
	}
	if(P.Flags & PrcssrOsmFilt::fImport) {
		Phase = phaseImport;
		if(O.CheckStatus(O.stGridLoaded)) {
			{
				int   rrsr = 0;
				ZDELETE(P_RoadStoneStat);
				THROW_MEM(P_RoadStoneStat = new RoadStone);
				THROW(rrsr = ReadRoadStone(Phase, *P_RoadStoneStat));
				if(rrsr < 0) {
					ZDELETE(P_RoadStoneStat);
				}
			}
			{
				MakeSuffixedTxtFileName(file_name, "nodewayassoc-sorted", ps, out_file_name);
				if(fileExists(out_file_name)) {
					THROW_MEM(P_NodeToWayAssocInF = new SFile(out_file_name, SFile::mRead|SFile::mNoStd|SFile::mBinary|SFile::mBuffRd));
					THROW_SL(P_NodeToWayAssocInF->IsValid());
				}
				else {
					ZDELETE(P_NodeToWayAssocInF);
				}
			}
			THROW(O.OpenDatabase(p_db_path));
			{
				xmlSAXHandler saxh_addr_obj;
				MEMSZERO(saxh_addr_obj);
				saxh_addr_obj.startDocument = Scb_StartDocument;
				saxh_addr_obj.endDocument = Scb_EndDocument;
				saxh_addr_obj.startElement = Scb_StartElement;
				saxh_addr_obj.endElement = Scb_EndElement;
				//
				THROW_MEM(SETIFZ(P_Ufp, new PPUserFuncProfiler(PPUPRF_OSMXMLPARSETAG)));
				PROFILE_START
				THROW(SaxParseFile(&saxh_addr_obj, file_name) == 0);
				THROW(!(State & stError));
				PROFILE_END
			}
		}
	}
	if(P.Flags & PrcssrOsmFilt::fExtractSizes) {
		Phase = phaseExtractSizes;
		if(O.CheckStatus(O.stGridLoaded)) {
			{
				struct __OFE { SFile ** PP_F; const char * P_Sfx; } __OFE_list[] = {
					{ &P_SizeOutF, "distance" }
				};
				for(uint i = 0; i < SIZEOFARRAY(__OFE_list); i++) {
					THROW_MEM(*__OFE_list[i].PP_F = new SFile(MakeSuffixedTxtFileName(file_name, __OFE_list[i].P_Sfx, ps, out_file_name), SFile::mWrite));
					THROW_SL((*__OFE_list[i].PP_F)->IsValid());
				}
			}
			PROFILE(THROW(O.OpenDatabase(p_db_path)));
			PROFILE(ProcessWaySizes());
		}
	}
	PPWait(0);
	CATCHZOK
	{
		ZDELETE(P_LatOutF);
		ZDELETE(P_LonOutF);
		ZDELETE(P_NodeToWayAssocOutF);
		ZDELETE(P_NodeToWayAssocInF);
		ZDELETE(P_TagOutF);
		ZDELETE(P_TagNodeOutF);
		ZDELETE(P_TagWayOutF);
		ZDELETE(P_TagRelOutF);
		ZDELETE(P_SizeOutF);
	}
	Phase = phaseUnkn;
	return ok;
}

int SLAPI DoProcessOsm(PrcssrOsmFilt * pFilt)
{
	int    ok = -1;
	PrcssrOsm prcssr(0);
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		PrcssrOsmFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}
//
//
//
#if 0 // @experimental {
class PPHistoricalTimeSeries {
public:
	/*enum {
		rsrvid
	};*/
	SLAPI  PPHistoricalTimeSeries();
	int    SLAPI Add(LDATETIME dtm, double value);
    int    SLAPI GetDateRange(DateRange & rRange) const;
    int    SLAPI GetLastValue(LDATETIME dtm, double * pValue) const;
private:
	struct Entry {
		LDATETIME Dtm;
		double Value;
	};
	int    ReservedId;
	SString Source;
	SString Title;
	TSVector <Entry> List;
};

int SLAPI Import_Macrotrends(const char * pPath, TSCollection <PPHistoricalTimeSeries> & rDataList)
{
	int    ok = -1;
	SDirEntry de;
	SString file_name;
    SString wildcard;
    SString path;
    SString line_buf;
    SString date_buf, value_buf;
    (path = pPath).Strip().SetLastSlash();
    (wildcard = path).Cat("*.csv");
	for(SDirec dir(wildcard, 0); dir.Next(&de) > 0;) {
		if(de.IsFile()) {
			(file_name = path).Cat(de.FileName);
            SFile f_in(file_name, SFile::mRead);
			if(f_in.IsValid()) {
				uint   line_no = 0;
				int    data_started = 0;
				while(f_in.ReadLine(line_buf)) {
					line_no++;
					line_buf.Chomp();
					if(line_no == 1) {
						if(!line_buf.IsEqiAscii("Macrotrends Data Download"))
							break;
					}
					else {
						if(data_started) {
							if(line_buf.Divide(',', date_buf, date_buf) > 0) {

							}
						}
					}
				}
			}
		}
	}
	return ok;
}
#endif // } 0 @experimental
