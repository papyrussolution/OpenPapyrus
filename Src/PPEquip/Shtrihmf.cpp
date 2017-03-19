// SHTRIHMF.CPP
// Copyright (c) A.Starodub 2009, 2010, 2011, 2013, 2015, 2016
// @codepage windows-1251
// Интерфейс (асинхронный) к драйверу "Штрих-М-ФР-К"
//
#include <pp.h>
#pragma hdrstop
#include <comdisp.h>
#include <ppidata.h>
//
//   Интерфейс к драйверу "Штрих-М-ФР-К"
//
//   Схемы в SHTRIH для PAPYRUS
#define SHTRIHMFRK_INNER_SCHEME           1
#define SHTRIHMFRK_OUTER_SCHEME        1001
//   Длина поля кода карты
#define AC_DEF_CARD_CODE_LEN       24
// Скидка по дисконтной карте
#define DISCOUNT_BYSCARD    3
//   Тип чека
#define CHK_SALE            1   // Продажа
#define CHK_RETURN          2   // Возврат
//   Тип оплаты
#define PAYMENT_CASH        1   // Наличными
//   Тип операции (транзакции) с чеком
#define OPTYPE_CHKLINE        11   // Строка чека
#define OPTYPE_STORNO         12   // Сторно строки
#define OPTYPE_RETURN         13   // Возврат
#define OPTYPE_CHKDISCOUNT    37   // Скидка на чек
#define OPTYPE_CHKEXTRACHARGE 38 // Надбавка на чек
#define OPTYPE_PAYM1          40   // Оплата с вводом суммы клиента
#define OPTYPE_PAYM2          41   // Оплата без ввода суммы клиента
#define OPTYPE_CHKCLOSED      55   // Закрытие чека
#define OPTYPE_CANCEL         56   // Отмена чека
#define OPTYPE_ZREPORT        61   // Z-отчет
#define OPTYPE_POSDISDETAIL   70   // Детализация скидки на позицию
#define OPTYPE_CHKDISDETAIL   71   // Детализация скидки на чек

class ACS_SHTRIHMFRK : public PPAsyncCashSession {
public:
	SLAPI  ACS_SHTRIHMFRK(PPID id);
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);
	virtual int SLAPI SetGoodsRestLoadFlag(int updOnly);
protected:
	virtual int SLAPI ExportSCard(FILE * pFile, int updOnly);
	int    SLAPI GetZRepList(LAssocArray *);
	int    SLAPI ImportFiles();

	int    UseInnerAutoDscnt;
private:
	int    SLAPI ConvertWareList(const char * pImpPath, int numSmena);
	DateRange    ChkRepPeriod;
	PPIDArray    LogNumList;
	SString      PathRpt;
	SString      PathFlag;
	int          ImpExpTimeout;
	StringSet    ImpPaths;
	StringSet    ExpPaths;
	SString      ImportedFiles;
	LAssocArray  ZRepList;
};

class CM_SHTRIHMFRK : public PPCashMachine {
public:
	SLAPI CM_SHTRIHMFRK(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_SHTRIHMFRK(NodeID); }
};

REGISTER_CMT(SHTRIHMFRK, 0, 1);

SLAPI ACS_SHTRIHMFRK::ACS_SHTRIHMFRK(PPID id) : PPAsyncCashSession(id)
{
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHMFRK_TIMEOUT, &ImpExpTimeout);
	UseInnerAutoDscnt = 0;
	ChkRepPeriod.SetZero();
}

int SLAPI ACS_SHTRIHMFRK::ExportSCard(FILE * pFile, int updOnly)
{
	int   ok = 1, r = 0;
	PPWait(1);
	long     scheme_id = 0;
	PPID     ser_id = 0;
	char   * p_format = "%s\n";
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPObjSCard   s_crd_obj;
	PPObjPerson  psn_obj;

	MEMSZERO(ser_rec);
	for(ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
		if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
			PPSCardSerPacket scs_pack;
			SCardTbl::Key2 k;
			MEMSZERO(k);
			k.SeriesID = ser_id;
			THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
			while(s_crd_obj.P_Tbl->search(2, &k, spGt) && k.SeriesID == ser_id) {
				int denied_card = 0;
				SString  temp_buf, buf, psn_name;
				SCardTbl::Rec card_rec;
				s_crd_obj.P_Tbl->copyBufTo(&card_rec);
				s_crd_obj.SetInheritance(&scs_pack, &card_rec);
				uint  pos = 0;
				PersonTbl::Rec psn_rec;
				denied_card = (card_rec.PDis && !(card_rec.Flags & SCRDF_CLOSED) && !(card_rec.Expiry && card_rec.Expiry < LConfig.OperDate)) ? 0 : 1;
				if(psn_obj.Search(card_rec.PersonID, &psn_rec) > 0) {
					(psn_name = psn_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
				}
				else
					psn_name = 0;
				scheme_id = card_rec.ID;
				//
				// Эскпорт схем автоматических скидок/надбавок
				//
				{
					(temp_buf = 0).CatChar('!');
					temp_buf.Cat(scheme_id).Semicol();                     // #1 Код схемы
					temp_buf.Cat(card_rec.Code).Cat(psn_name).Semicol();   // #2 Название схемы
					temp_buf.Cat(1L);                                         // #3 Применяется по карте
					THROW_PP(fprintf(pFile, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
				}
				//
				// Эскпорт автоматических скидок/надбавок
				//
				{
					(temp_buf = 0).CatChar('@');
					temp_buf.Cat(scheme_id).Semicol();                                  // #1  Код схемы
					temp_buf.Cat(card_rec.ID).Semicol();                                // #2  Код
					temp_buf.Cat(psn_name).Semicol();                                   // #3  Наименование
					temp_buf.Cat(1L).Cat(psn_name).Semicol();                           // #4  Тип скидки
					temp_buf.Cat(1L).Semicol();                                         // #5  Вид скидки
					temp_buf.Cat(fdiv100i(card_rec.PDis)).Semicol();					// #6  Размер скидки
					temp_buf.Cat(psn_name).Semicol();                                   // #7  Текст для чека
					temp_buf.Cat(card_rec.Dt, DATF_GERMAN|DATF_CENTURY).Semicol();      // #8  Начальная дата
					temp_buf.Cat(card_rec.Expiry, DATF_GERMAN|DATF_CENTURY).Semicol();  // #9  Конечная дата
					temp_buf.Cat("00:00:00").Semicol();                                 // #10 Начальное время //
					temp_buf.Cat("24:00:00").Cat(psn_name).Semicol();                   // #11 Конечное время //
					temp_buf.Cat(0L).Semicol();                                         // #12 Номер начального дня недели
					temp_buf.Cat(7L).Semicol();                                         // #13 Номер конечного дня недели
					temp_buf.Cat(0L).Semicol();                                         // #14 Начальное количество
					temp_buf.Cat(0L).Semicol();                                         // #15 Конечное количество
					temp_buf.Cat(0L).Cat(psn_name).Semicol();                           // #16 Начальная сумма
					temp_buf.Cat(0L);                                                   // #17 Конечная сумма
					THROW_PP(fprintf(pFile, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
				}
				//
				// Экспорт дисконтных карт
				//
				{
					(temp_buf = 0).CatChar('%');
					// (buf = 0).CatCharN(' ', AC_DEF_CARD_CODE_LEN - strlen(card_rec.Code)).Cat(card_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
					temp_buf.Cat(card_rec.Code).Semicol();            // #1 Номер карты
					temp_buf.Cat(psn_name).Semicol();                 // #2 Владелец карты
					temp_buf.Cat(psn_name).Semicol();                 // #3 Текст для чека
					temp_buf.Cat(0L/*card_rec.Turnover*/).Semicol();  // #4 Сумма накопления //
					temp_buf.Cat(scheme_id).Semicol();                // #5 Код схемы автоматических скидок
					temp_buf.Cat(denied_card).Semicol();              // #6 Карта запрещена
					temp_buf.Cat(1L).Semicol();                       // #7 Не вести накоплений по карте
					temp_buf.Semicol();                               // #8 Не используется //
					temp_buf.Cat(0L).Semicol();                       // #9 Использовать как платежную
					temp_buf.CatCharN(';', 3);                           // #10-#12 Не используется //
					temp_buf.Cat(0L).Semicol();                       // #13 Код схемы накопительных скидок
					temp_buf.CatCharN(';', 4);                           // #14-#18 Не используется //
					THROW_PP(fprintf(pFile, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
				}
			}
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::SetGoodsRestLoadFlag(int updOnly)
{
	int    ok = 1;
	Flags &= ~PPACSF_LOADRESTWOSALES;
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::ExportData(int updOnly)
{
	int    ok = 1, load_groups = 0;
	char   load_symb = '#', * p_format = "%s\n";
	PPID   prev_goods_id = 0, stat_id = 0;
	LAssocArray  grp_n_level_ary;
	SString   f_str, name;
	SString   path_goods;
	PPUnit    unit_rec;
	PPObjUnit unit_obj;
	PPAsyncCashNode               cn_data;
	AsyncCashGoodsInfo            gds_info;
	AsyncCashGoodsIterator      * p_gds_iter = 0;
	AsyncCashGoodsGroupInfo       grp_info;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	PPGoodsConfig goods_cfg = GetGoodsCfg();
	PPIDArray quot_list, goods_list;
	FILE * p_file = 0;
	const  int check_dig = BIN(goods_cfg.Flags & GCF_BCCHKDIG);

	THROW(GetNodeData(&cn_data) > 0);
	if(!P_Dls)
		THROW_MEM(P_Dls = new DeviceLoadingStat);
	P_Dls->StartLoading(&stat_id, dvctCashs, NodeID, 1);
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_SHTRIHMFRK_IMP_TXT,  path_goods));
	PPSetAddedMsgString(path_goods);
	THROW_PP(p_file = fopen(path_goods, onecstr('w')), PPERR_CANTOPENFILE);
	THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, P_Dls));
	THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_INITFILE, f_str));
	THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	THROW_PP(fprintf(p_file, p_format, onecstr(load_symb)) > 0,  PPERR_EXPFILEWRITEFAULT);
	load_groups = (cn_data.Flags & CASHF_EXPGOODSGROUPS) ? 1 : 0;
	if(load_groups) {
		THROW_MEM(p_grp_iter = new AsyncCashGoodsGroupIterator(NodeID, 0, P_Dls));
		while(p_grp_iter->Next(&grp_info) > 0) {
			f_str = 0;
			f_str.Cat(grp_info.ID).CatCharN(';', 2);      // #1 - ИД группы товаров, #2 - не используется //
			name = grp_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                 // #3 - Наименование группы товаров
			f_str.Cat(name).Semicol();                 // #4 - Текст для чека (?)
			f_str.CatCharN(';', 11);                      // #5-#15 - Не используем
			f_str.Cat(grp_info.ParentID).Semicol();    // #16 - ИД родительской группы
			f_str.CatChar('0');                           // #17 - Признак товарной группы (0)
			THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			grp_n_level_ary.Add(grp_info.ID, grp_info.Level, 0);
		}
	}
	while(p_gds_iter->Next(&gds_info) > 0) {
		size_t bclen;
	   	if(gds_info.ID != prev_goods_id) {
			long level = 0;
			f_str = 0;
			f_str.Cat(gds_info.ID).Semicol();                                 // #1 - ИД товара
			if((bclen = strlen(gds_info.BarCode)) != 0) {                     // #2 - Основной штрихкод
				gds_info.AdjustBarcode(check_dig);
				const int wp = goods_cfg.IsWghtPrefix(gds_info.BarCode);
				if(wp == 1)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(goods_cfg.WghtPrefix));
				else if(wp == 2)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(goods_cfg.WghtCntPrefix));
				else
					AddCheckDigToBarcode(gds_info.BarCode);
				f_str.Cat(gds_info.BarCode);
			}
			f_str.Semicol();
			name = gds_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                                     // #3 - Наименование товара
			f_str.Cat(name).Semicol();                                     // #4 - Текст для чека
			f_str.Cat(gds_info.Price, SFMT_MONEY).Semicol();               // #5 - Цена товара
			if(CheckCnFlag(CASHF_EXPGOODSREST))
				f_str.Cat(gds_info.Rest, SFMT_QTTY);                       // #6 - Остаток
			f_str.Semicol();
			/*
			if(UseInnerAutoDscnt && gds_info.NoDis <= 0)
				f_str.Cat(SHTRIHMFRK_INNER_SCHEME);                        // #7 - Код схемы внутренней автоматической скидки
			*/
			f_str.Semicol();
		   	unit_obj.Fetch(gds_info.UnitID, &unit_rec);
			f_str.CatChar((unit_rec.Flags & PPUnit::IntVal) ? '0' : '1');  // #8 - Разрешить дробное кол-во
			f_str.Semicol();
			f_str.Cat((gds_info.DivN == 0) ? 1 : gds_info.DivN).Semicol(); // #9 - Номер секции
			f_str.Cat((gds_info.NoDis <= 0) ? 99L : -1L).Semicol();       // #10 - Максимальный процент скидки
			f_str.Semicol();											  // #11 - Код налоговой схемы (пока не используем)
			f_str.Semicol();                                              // #12 - Артикул
			f_str.CatCharN(';', 3);                                       // #13-#15 - Не используем
			if(load_groups && gds_info.ParentID &&
				grp_n_level_ary.Search(gds_info.ParentID, &level, 0) > 0)
				f_str.Cat(gds_info.ParentID).Semicol();                   // #16 - ИД группы товаров
			else
				f_str.CatChar('0').Semicol();
			f_str.CatChar('1');                                           // #17 - Признак товара (1)
			THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
			goods_list.add(gds_info.ID);
		}
		else {
			(f_str = 0).CatChar('#').Cat(gds_info.ID).Semicol();          // #1 - ИД товара
			if((bclen = strlen(gds_info.BarCode)) != 0) {
				gds_info.AdjustBarcode(check_dig);
				const int wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
				if(wp == 1)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(GetGoodsCfg().WghtPrefix));
				else if(wp == 2)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+strlen(GetGoodsCfg().WghtCntPrefix));
				else
					AddCheckDigToBarcode(gds_info.BarCode);
				f_str.Cat(gds_info.BarCode);                              // #2 - Дополнительный штрихкод
			}
			f_str.Semicol();
			name = gds_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                                    // #3 - Наименование товара
			f_str.Cat(name).Semicol();                                    // #4 - Наименование для кассы
			f_str.Cat(gds_info.Price, SFMT_MONEY).Semicol();              // #5 - Цена товара
			f_str.CatCharN(';', 4);                                       // #6-#9 Не используем
			f_str.Cat((gds_info.UnitPerPack) ? gds_info.UnitPerPack : 1); // #10 - Коэффициент
			THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		}
	   	prev_goods_id = gds_info.ID;
		PPWaitPercent(p_gds_iter->GetIterCounter());
	}
	//
	// Экспорт формата весовых штрих-кодов
	//
	/* @v6.3.x Структура весового штрихкода будет настраиваться на кассе
	if((goods_cfg.Flags & GCF_ENABLEWP) && strlen(goods_cfg.WghtPrefix)) {
		(f_str = 0).CatChar('(').Cat(goods_cfg.WghtPrefix).Semicol();       // #1 - Префикс весового штрихкода
		f_str.Cat(1L).Semicol();                                            // #2 - Сначала код товара затем вес вес товара (1)
		f_str.Cat(5L).Semicol();                                            // #3 - Число знаков под код (5)
		f_str.Cat(0.001).Semicol();                                         // #4 - Коэффициент веса (0.001)
		f_str.Cat(0.001);                                                      // #5 - Множитель цены (0.001)
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	*/
	//THROW(ExportSCard(p_file, updOnly));
	//int SLAPI ACS_SHTRIHMFRK::ExportSCard(FILE * pFile, int updOnly)
	{
		int    r = 0;
		long   scheme_id = 0;
		PPID   ser_id = 0;
		char * p_format = "%s\n";
		PPObjSCardSeries scs_obj;
		PPSCardSeries ser_rec;
		PPObjSCard   s_crd_obj;
		LAssocArray  scard_quot_ary;
		SString  temp_buf, buf, psn_name;
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, stat_id);
		scard_quot_ary.freeAll();
		MEMSZERO(ser_rec);
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
				PPSCardSerPacket scs_pack;
				AsyncCashSCardInfo info;
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
				if(ser_rec.QuotKindID_s > 0)
					quot_list.addUnique(ser_rec.QuotKindID_s);
				for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
					int    denied_card = 0;
					s_crd_obj.SetInheritance(&scs_pack, &info.Rec);
					uint  pos = 0;
					denied_card = (info.Rec.PDis && !(info.Rec.Flags & SCRDF_CLOSED) && !(info.Rec.Expiry && info.Rec.Expiry < LConfig.OperDate)) ? 0 : 1;
					(psn_name = info.PsnName).Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
					scheme_id = info.Rec.ID;
					//
					// Эскпорт схем автоматических скидок/надбавок
					//
					{
						(temp_buf = 0).CatChar('!');
						temp_buf.Cat(scheme_id).Semicol();                         // #1 Код схемы
						temp_buf.Cat(info.Rec.Code).Cat(psn_name).Semicol();       // #2 Название схемы
						temp_buf.Cat(1L);                                          // #3 Применяется по карте
						THROW_PP(fprintf(p_file, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					//
					// Эскпорт автоматических скидок/надбавок
					//
					{
						(temp_buf = 0).CatChar('@');
						temp_buf.Cat(scheme_id).Semicol();                                  // #1  Код схемы
						temp_buf.Cat(info.Rec.ID).Semicol();                                // #2  Код
						temp_buf.Cat(psn_name).Semicol();                                   // #3  Наименование
						temp_buf.Cat(1L).Cat(psn_name).Semicol();                           // #4  Тип скидки
						temp_buf.Cat(1L).Semicol();                                         // #5  Вид скидки
						temp_buf.Cat(fdiv100i(info.Rec.PDis)).Semicol();                    // #6  Размер скидки
						temp_buf.Cat(psn_name).Semicol();                                   // #7  Текст для чека
						temp_buf.Cat(info.Rec.Dt, DATF_GERMAN|DATF_CENTURY).Semicol();      // #8  Начальная дата
						temp_buf.Cat(info.Rec.Expiry, DATF_GERMAN|DATF_CENTURY).Semicol();  // #9  Конечная дата
						temp_buf.Cat("00:00:00").Semicol();                                 // #10 Начальное время //
						temp_buf.Cat("24:00:00").Cat(psn_name).Semicol();                   // #11 Конечное время //
						temp_buf.Cat(0L).Semicol();                                         // #12 Номер начального дня недели
						temp_buf.Cat(7L).Semicol();                                         // #13 Номер конечного дня недели
						temp_buf.Cat(0L).Semicol();                                         // #14 Начальное количество
						temp_buf.Cat(0L).Semicol();                                         // #15 Конечное количество
						temp_buf.Cat(0L).Cat(psn_name).Semicol();                           // #16 Начальная сумма
						temp_buf.Cat(0L);                                                   // #17 Конечная сумма
						temp_buf.Cat(ser_rec.QuotKindID_s);                                 // #18 Код дополнительной цены
						THROW_PP(fprintf(p_file, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					//
					// Экспорт дисконтных карт
					//
					{
						(temp_buf = 0).CatChar('%');
						// (buf = 0).CatCharN(' ', AC_DEF_CARD_CODE_LEN - strlen(info.Rec.Code)).Cat(info.Rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
						temp_buf.Cat(info.Rec.Code).Semicol();            // #1 Номер карты
						temp_buf.Cat(psn_name).Semicol();                 // #2 Владелец карты
						temp_buf.Cat(psn_name).Semicol();                 // #3 Текст для чека
						temp_buf.Cat(0L/*info.Rec.Turnover*/).Semicol();  // #4 Сумма накопления //
						temp_buf.Cat(scheme_id).Semicol();                // #5 Код схемы автоматических скидок
						temp_buf.Cat(denied_card).Semicol();              // #6 Карта запрещена
						temp_buf.Cat(1L).Semicol();                       // #7 Не вести накоплений по карте
						temp_buf.Semicol();                               // #8 Не используется //
						temp_buf.Cat(0L).Semicol();                       // #9 Использовать как платежную
						temp_buf.CatCharN(';', 3);                        // #10-#12 Не используется //
						temp_buf.Cat(0L).Semicol();                       // #13 Код схемы накопительных скидок
						temp_buf.CatCharN(';', 4);                        // #14-#18 Не используется //
						THROW_PP(fprintf(p_file, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					iter.SetStat();
				}
			}
		}
	}
	//
	// Загрузка дополнительных цен/котировок
	//
	if(goods_list.getCount() > 0 && quot_list.getCount() > 0) {
		LDATE cur_dt;
		uint quot_count = quot_list.getCount();
		uint goods_count = goods_list.getCount();

		getcurdate(&cur_dt);
		for(uint i = 0; i < quot_count; i++) {
			PPID quot_kind_id = quot_list.at(i);
			SString quot_name;

			GetObjectName(PPOBJ_QUOTKIND, quot_kind_id, quot_name);
			quot_name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			for(uint j = 0; j < goods_count; j++) {
				PPID goods_id = goods_list.at(j);
				RetailPriceExtractor::ExtQuotBlock ext_block(quot_kind_id);
				RetailPriceExtractor rtl_extr(0, &ext_block, 0, ZERODATETIME, 0);
				RetailExtrItem price_item;

				if(rtl_extr.GetPrice(goods_id, 0, 1, &price_item) > 0 && price_item.Price > 0.0) {
					SString temp_buf;
					(temp_buf = 0).CatChar('?');
					temp_buf.Cat(goods_id).Semicol();         // #1 Код
					temp_buf.Cat(quot_kind_id).Semicol();     // #2 Код доп цены.
					temp_buf.Cat(quot_name).Semicol();        // #3 Наименование
					temp_buf.Cat(price_item.Price);           // #4 Цена
					THROW_PP(fprintf(p_file, p_format, (const char *)temp_buf) > 0, PPERR_EXPFILEWRITEFAULT);
				}
			}
		}
	}
	//
	if(updOnly) {
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_REPLACEQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	else {
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_CLEAR, f_str));
		f_str.Cat("{AUT_S}").Cat("{DIS_C}");
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_ADDQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, (const char *)f_str) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	SFile::ZClose(&p_file);
	PPWait(0);
	PPWait(1);
	THROW(DistributeFile(path_goods, 0));
	if(stat_id)
		P_Dls->FinishLoading(stat_id, 1, 1);
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	delete p_gds_iter;
	delete p_grp_iter;
	PPWait(0);
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	ZRepList.freeAll();
	ImportedFiles = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtr(&dlg, 1)) {
			SString dt_buf;
			ChkRepPeriod.low = LConfig.OperDate;
			ChkRepPeriod.upp = LConfig.OperDate;
			SetupCalCtrl(CTLCAL_DATERNG_PERIOD, dlg, CTL_DATERNG_PERIOD, 1);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, &ChkRepPeriod);
			PPWait(0);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && strtoperiod(dt_buf, &ChkRepPeriod, 0) && !ChkRepPeriod.IsZero()) {
					SETIFZ(ChkRepPeriod.upp, plusdate(LConfig.OperDate, 2));
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWait(1);
		}
	}
	else {
		ChkRepPeriod = *pPrd;
		ok = 1;
	}
	if(ok > 0) {
		PPAsyncCashNode acn;
		THROW(GetNodeData(&acn) > 0);
		acn.GetLogNumList(&LogNumList);
		{
			SString  alt_imp_params;
			PPIniFile  ini_file;
			if(!PathRpt.NotEmptyS())
				THROW(PPGetFilePath(PPPATH_IN, PPFILNAM_SHTRIHMFRK_EXP_TXT,  PathRpt));
			if(!PathFlag.NotEmptyS())
				THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_SHTRIHMFRK_EXP_FLAG, PathFlag));
		}
		THROW_PP(acn.ExpPaths.NotEmptyS() || acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
		ImpPaths.clear();
		ImpPaths.setDelim(onecstr(';'));
		{
			SString & r_list = (acn.ImpFiles.NotEmpty()) ? acn.ImpFiles : acn.ExpPaths;
			ImpPaths.setBuf(r_list, r_list.Len() + 1);
		}
		ExpPaths.clear();
		ExpPaths.setDelim(onecstr(';'));
		{
			SString & r_list = (acn.ExpPaths.NotEmpty()) ? acn.ExpPaths : acn.ImpFiles;
			ExpPaths.setBuf(r_list, r_list.Len() + 1);
		}
		THROW(ImportFiles());
		THROW(GetZRepList(&ZRepList));
	}
	CATCHZOK
	ASSIGN_PTR(pSessCount, (ok > 0) ? ZRepList.getCount() : 0);
	ASSIGN_PTR(pIsForwardSess, (ok > 0) ? 1 : 0);
	delete dlg;
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::ImportFiles()
{
	long   delay_quant = 5 * 60 * 1000; // 5 мин
	const  char * p_ftp_flag = "ftp:";
	int    ok = 1, ftp_connected = 0, notify_timeout = (ImpExpTimeout) ? ImpExpTimeout : (1 * 60 * 60 * 1000); // таймаут по умолчанию - 1 час.
	double timeouts_c = (double)notify_timeout / (double)delay_quant;
	uint   set_no = 0;
	SString imp_path, exp_path, path_rpt, path_flag, str_imp_paths;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	StringSet imp_paths(";");
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	WinInetFTP ftp;

	ImportedFiles = 0;
	SETIFZ(last_date, plusdate(LConfig.OperDate, 2));
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	if(EqCfg.FtpAcctID)
		THROW(obj_acct.Get(EqCfg.FtpAcctID, &acct));
	for(uint i = 0; ImpPaths.get(&i, imp_path);) {
		if(imp_path.CmpPrefix(p_ftp_flag, 1) == 0) {
			SString ftp_path, ftp_path_flag, ftp_dir, file_name;
			SPathStruc sp;
			imp_path.ShiftLeft(strlen(p_ftp_flag));
			if(!ftp_connected) {
				THROW(ftp.Init());
				THROW(ftp.Connect(&acct));
				ftp_connected = 1;
			}
			{
				SPathStruc sp;
				sp.Split(imp_path);
				sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, ftp_dir);
				sp.Split(PathRpt);
				sp.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, file_name);
				(ftp_path = ftp_dir).SetLastSlash().Cat(file_name);
				sp.Split(PathFlag);
				sp.Merge(0, SPathStruc::fDrv|SPathStruc::fDir|SPathStruc::fExt, file_name);
				(ftp_path_flag = ftp_dir).SetLastSlash().Cat(file_name);
			}
			/* Сессии будут выгружаться при закрытии Z отчета
			SFile query_file(PathFlag, SFile::mWrite);
			query_file.Close();
			THROW(ftp.SafePut(PathFlag, ftp_path_flag, 0, 0, 0));
			delay(60 * 1000);
			*/
			path_rpt = PathRpt;
			for(double j = 0; j < timeouts_c; j++) {
				if(ftp.SafeGet(path_rpt, ftp_path, 0, 0, 0) > 0) {
					ftp.SafeDelete(ftp_path, 0);
					if(ImportedFiles.Len())
						ImportedFiles.Semicol();
					ImportedFiles.Cat(path_rpt);
					ok = 1;
					break;
				}
				else
					delay(delay_quant);
			}
		}
		else {
			if(str_imp_paths.NotEmptyS())
				str_imp_paths.Semicol();
			str_imp_paths.Cat(imp_path);
		}
	}
	if(str_imp_paths.Len())
		imp_paths.setBuf(str_imp_paths, str_imp_paths.Len() + 1);
	for(uint i = 0; imp_paths.get(&i, imp_path);) {
		set_no++;
		{
			int exp_path_found = 0;
			for(uint j = 0, n = 0; !exp_path_found && ExpPaths.get(&j, exp_path);)
				if(++n == set_no)
					exp_path_found = 1;
			if(!exp_path_found)
				exp_path = imp_path;
		}
		if(::access(imp_path, 0) == 0) {
			path_rpt  = PathRpt;
			path_flag = PathFlag;
			SPathStruc::ReplacePath(path_rpt,   imp_path, 1);
			SPathStruc::ReplacePath(path_flag,  exp_path, 1);
			if(ok > 0) {
				for(double j = 0; j < timeouts_c; j++) {
					if(fileExists(path_rpt)) {
						if(ImportedFiles.Len())
							ImportedFiles.Semicol();
						ImportedFiles.Cat(path_rpt);
						ok = 1;
						break;
					}
					else
						delay(delay_quant);
				}
			}
		}
		else {
			// log message
		}
	}
	{
		const char * p_prefix = "slp";
		SString path, temp_path;
		SString temp_dir;
		for(uint file_no = 0; path.GetSubFrom(ImportedFiles, ';', file_no) > 0; file_no++) {
			if(fileExists(path)) {
				SPathStruc sp;
				sp.Split(path);
				sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, temp_dir);
				// AHTOXA удаление временных файлов, если их кол-во стало больше 30 {
				{
					uint   files_count = 0;
					LDATETIME dtm;
					dtm.SetFar();
					SString firstf_path;
					SDirEntry sde;
					dtm.d = MAXLONG;
					dtm.t = MAXLONG;
					(temp_path = temp_dir.SetLastSlash()).Cat(p_prefix).Cat("?????.txt");
					for(SDirec sd(temp_path); sd.Next(&sde) > 0;) {
						if(dtm.IsFar() || cmp(sde.WriteTime, dtm) < 0) {
							dtm = sde.WriteTime;
							(firstf_path = temp_dir).Cat(sde.FileName);
						}
						files_count++;
					}
					if(files_count > 29)
						SFile::Remove(firstf_path);
				}
                MakeTempFileName(temp_dir.SetLastSlash(), p_prefix, "txt", 0, temp_path);
				SCopyFile(path, temp_path, 0, 0, 0);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::GetZRepList(LAssocArray * pZRepList)
{
	int    ok = 1;
	SString path, buf;
	StringSet ss(';', ImportedFiles);
	LAssocArray zrep_list;
	for(uint i = 0, file_no = 0; ss.get(&i, path); file_no++) {
		uint   pos = 0;
		long   op_type = 0, nsmena = 0, cash_no = 0;
		LAssocArray zrep_ary; // Пара {номер_файла; номер_смены}
		SString imp_file_name = path;
		SFile  imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name
		PPSetAddedMsgString(imp_file_name);
		THROW_SL(imp_file.IsValid());
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		//
		// Собираем список Z-отчетов в массив zrep_list.
		//
		while(imp_file.ReadLine(buf) > 0) {
			StringSet ss1(';', buf);
			ss1.get(&(pos = 0), buf);                                                      // #1 Код транзакции (не используем)
			ss1.get(&pos, buf);                                                            // #2 Дата транзакции
 			ss1.get(&pos, buf);                                                            // #3 Время транзакции
			ss1.get(&pos, buf);                                                            // #4 Тип транзакции (операции)
			op_type = buf.ToLong();
			if(op_type == OPTYPE_ZREPORT) {
				ss1.get(&pos, buf);
				cash_no = buf.ToLong();													   // #5 Номер ККМ
				ss1.get(&pos, buf);                                                        // #6 Пропускаем
				ss1.get(&pos, buf);                                                        // #7 Пропускаем
				ss1.get(&pos, buf);                                                        // #8 Номер смены
				nsmena = buf.ToLong();
				if(LogNumList.lsearch(cash_no))
					zrep_list.Add(file_no, nsmena, 0);
			}
		}
	}
	ASSIGN_PTR(pZRepList, zrep_list);
	CATCHZOK
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::ConvertWareList(const char * pImpPath, int numSmena)
{
	int    ok = 1, smena_found = 0;
	uint   pos = 0;
	long   op_type = 0;
	long   count = 0, prev_smena_pos = 0;
	PPID   grp_id = 0, goods_id = 0;
	LDATE  dt = ZERODATE;
	LTIME  tm = ZEROTIME;
	SString   buf, wait_msg;
	StringSet ss(';', 0);
	SCardCore     sc_core;
	PPGoodsPacket gds_pack;
	PPObjGoods    goods_obj;

	SString   imp_file_name = pImpPath;
	SFile     imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name
	PPSetAddedMsgString(imp_file_name);
	THROW_SL(imp_file.IsValid());
	{
		PPTransaction tra(1);
		THROW(tra);
		prev_smena_pos = 0;
		//
		// Находим в файле смещение нужного нам z отчета
		//
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		prev_smena_pos = imp_file.Tell();
		while(imp_file.ReadLine(buf) > 0) {
			ss.clear();
			ss.add(buf);
			ss.get(&(pos = 0), buf);                                                      // #1 Код транзакции (не используем)
			ss.get(&pos, buf);                                                            // #2 Дата транзакции
			ss.get(&pos, buf);                                                            // #3 Время транзакции
			ss.get(&pos, buf);                                                            // #4 Тип транзакции (операции)
			op_type = buf.ToLong();
			if(op_type == OPTYPE_ZREPORT) {
				ss.get(&pos, buf);                                                        // #5 Номер ККМ - пропускаем
				ss.get(&pos, buf);                                                        // #6 Пропускаем
				ss.get(&pos, buf);                                                        // #7 Пропускаем
				ss.get(&pos, buf);                                                        // #8 Номер смены
				if(buf.ToLong() == numSmena) {
					 smena_found = 1;
					 break;
				}
				else
					prev_smena_pos = imp_file.Tell();
			}
		}
		PPLoadText(PPTXT_ACSCLS_IMPORT, wait_msg);
		PPWaitPercent(numSmena, wait_msg);
		//
		// Закладываемся на то, что чек заканчивается либо закрытием, либо отменой
		//
		if(smena_found) {
			int    chk_type = 0;
			double total_discount = 0.0;
			double total_amount = 0.0;
			CCheckPacket check_pack;
			BExtInsert bei(P_TmpCclTbl);
			imp_file.Seek(prev_smena_pos);
			while(imp_file.ReadLine(buf) > 0) {
				long   chk_no = 0, cash_no = 0;
				long   line_no = 0; // @debug
				ss.clear();
				ss.add(buf);
				ss.get(&(pos = 0), buf);            // #1 Код транзакции (не используем)
				line_no = buf.ToLong();             // @v6.6.3 @debug
				ss.get(&pos, buf);                  // #2 Дата транзакции
				strtodate(buf.Strip(), DATF_DMY, &dt);
				ss.get(&pos, buf);                  // #3 Время транзакции
				strtotime(buf.Strip(), TIMF_HMS, &tm);
				ss.get(&pos, buf);                  // #4 Тип транзакции (операции)
				op_type = buf.ToLong();
				ss.get(&pos, buf);                  // #5 Номер ККМ
				cash_no = buf.ToLong();
				ss.get(&pos, buf);                  // #6 Номер чека
				chk_no = buf.ToLong();
				ss.get(&pos, buf);                  // #7 Код кассира - пропускаем
				if(oneof3(op_type, OPTYPE_CHKLINE, OPTYPE_STORNO, OPTYPE_RETURN)) { // Строка чека, сторно строки или возврат
					double price = 0.0, qtty = 0.0;
					double line_sum = 0.0;
					ss.get(&pos, buf);
					goods_id = buf.ToLong();        // #8 Код товара
					ss.get(&pos, buf);              // #9 номер секции - пропускаем
					ss.get(&pos, buf);
					price = R5(buf.ToReal());       // #10 Цена
					ss.get(&pos, buf);
					qtty = R6(buf.ToReal());        // #11 Количество
					ss.get(&pos, buf);
					line_sum = buf.ToReal();        // #12 Сумма по строке
					if(qtty != 0.0 && line_sum != 0.0)
						price = line_sum / qtty;
					if(op_type == OPTYPE_STORNO) {
						//
						// Для операции сторно удаляем предшествующую строку, имеющую тот же товар и то же количество.
						//
						uint   c = check_pack.GetCount();
						if(c)
							do {
								const CCheckLineTbl::Rec & r_line = check_pack.GetLine(--c);
								if(r_line.GoodsID == labs(goods_id) && fabs(r_line.Quantity) == fabs(qtty)) {
									check_pack.RemoveLine_(c);
									break;
								}
							} while(c);
					}
					else {
						check_pack.InsertItem(labs(goods_id), qtty, price, 0, 0);
						if(op_type == OPTYPE_RETURN)
							chk_type = CHK_RETURN;
						else if(oneof2(op_type, OPTYPE_CHKLINE, OPTYPE_STORNO) && chk_type != CHK_RETURN)
							chk_type = CHK_SALE;
					}
				}
				else if(oneof2(op_type, OPTYPE_CHKEXTRACHARGE, OPTYPE_CHKDISDETAIL)) {
					long   dis_kind = 0;
					SCardTbl::Rec sc_rec;
					SString card_code;
					ss.get(&pos, buf);
					card_code = buf;                // #8  Номер дисконтной карты, если 9-ый параметр == DISCOUNT_BYSCARD
					ss.get(&pos, buf);
					dis_kind = buf.ToLong();        // #9  Вид скидки
					ss.get(&pos, buf);              // #10 Пропускаем
					ss.get(&pos, buf);              // #11 Процент скидки - пропускаем
					ss.get(&pos, buf);              // #12 Сумма скидки
					total_discount = fabs(R5(buf.ToReal()));
					if(op_type == OPTYPE_CHKEXTRACHARGE)
						total_discount = -total_discount;
					//check_pack.SetTotalDiscount(total_discount, (op_type == OPTYPE_CHKEXTRACHARGE) ? CCheckPacket::stdfPlus : 0);
					if(op_type == OPTYPE_CHKDISDETAIL && dis_kind == DISCOUNT_BYSCARD && card_code.NotEmptyS() && sc_core.SearchCode(0, card_code, &sc_rec) > 0)
						check_pack.Rec.SCardID = sc_rec.ID;
					else
						check_pack.Rec.SCardID = 0;
				}
				else if(oneof2(op_type, OPTYPE_PAYM1, OPTYPE_PAYM2)) { // Оплата
					long   paym_type;
					double submit = 0.0; // Сдача
					ss.get(&pos, buf);                // #8  - Код платежной карты
					ss.get(&pos, buf);                // #9  - Пропускаем
					ss.get(&pos, buf);                // #10 - Сумма сдачи
					submit = buf.ToReal();
					ss.get(&pos, buf);
					paym_type = buf.ToLong();         // #11 - Номер вида оплаты
					ss.get(&pos, buf);                // #12 - Сумма чека
					if(chk_type == CHK_RETURN)
						total_amount = -R2(-buf.ToReal()-submit);
					else
						total_amount = R2(buf.ToReal()-submit);
					if(paym_type != PAYMENT_CASH)
						check_pack.Rec.Flags |= CCHKF_BANKING;
				}
				else if(op_type == OPTYPE_CHKCLOSED) {
					if(LogNumList.lsearch(cash_no)) {
						int    hour = 0, min = 0;
						PPID   id = 0;
						LDATETIME  dttm;
						CCheckLineTbl::Rec cchkl_rec;
						dttm.Set(dt, tm);
						if(oneof2(chk_type, CHK_SALE, CHK_RETURN)) {
							long   fl  = (chk_type == CHK_RETURN) ? CCHKF_RETURN : 0;
							fl |= CCHKF_TEMPSESS;
							SETFLAG(fl, CCHKF_BANKING, check_pack.Rec.Flags & CCHKF_BANKING);
							THROW(AddTempCheck(&id, numSmena, fl, cash_no, chk_no, 0, 0, &dttm, 0, 0));
						}
						if(check_pack.Rec.SCardID)
							THROW(AddTempCheckSCardID(id, check_pack.Rec.SCardID));
						PPID   chk_id = P_TmpCcTbl->data.ID;
						{
							//
							// Установка скидки на чек и выравнивание суммы чека до величины, заданной
							// во входном файле (total_amount)
							//
							double chk_amt = 0.0;
							double chk_dis = 0.0;
							check_pack.CalcAmount(&chk_amt, &chk_dis);
							if(chk_amt != total_amount || total_discount != 0.0) {
								double new_discount = chk_dis + (fabs(chk_amt) - fabs(total_amount)); // @v6.9.1 fabs()
								check_pack.SetTotalDiscount(fabs(new_discount), (new_discount < 0.0) ? CCheckPacket::stdfPlus : 0);
								check_pack.CalcAmount(&chk_amt, &chk_dis);
							}
							THROW(SetTempCheckAmounts(chk_id, /*chk_amt*/total_amount, chk_dis));
						}
						for(uint chk_pos = 0; check_pack.EnumLines(&chk_pos, &cchkl_rec) > 0;) {
							TempCCheckLineTbl::Rec ccl_rec;
							MEMSZERO(ccl_rec);
							ccl_rec.CheckID   = chk_id;
							ccl_rec.CheckCode = chk_no;
							ccl_rec.Dt        = P_TmpCcTbl->data.Dt;
							ccl_rec.DivID     = 0;
							ccl_rec.GoodsID   = cchkl_rec.GoodsID;
							ccl_rec.Quantity  = cchkl_rec.Quantity;
							ccl_rec.Price     = cchkl_rec.Price;
							ccl_rec.Dscnt     = cchkl_rec.Dscnt;
							THROW_DB(bei.insert(&ccl_rec));
							/*
							{
								double price = intmnytodbl(cchkl_rec.Price);
								double line_amount = R2(cchkl_rec.Quantity * price);
								double line_discount = R2(cchkl_rec.Quantity * cchkl_rec.Dscnt);
								THROW(AddTempCheckAmounts(chk_id, line_amount - line_discount, line_discount));
							}
							*/
						}
					}
					check_pack.Init();
					chk_type = 0;
					total_amount = total_discount = 0.0;
				}
				else if(op_type == OPTYPE_CANCEL) { // Отмена чека
					check_pack.Init();
					chk_type = 0;
					total_amount = total_discount = 0.0;
				}
				else if(op_type == OPTYPE_ZREPORT) {// Найдена запись z отчета (т.е. смена загружена в бд)
					int    r = 0, h = 0, m = 0;
					PPID   chk_id = 0;
					double amount = 0.0;
					LDATETIME dttm;
					dttm.Set(dt, tm);
					ss.get(&pos, buf);                // #8  - Пропускаем
					ss.get(&pos, buf);                // #9  - Пропускаем
					ss.get(&pos, buf);                // #10 - Пропускаем
					ss.get(&pos, buf);                // #11 - Пропускаем
					ss.get(&pos, buf);                // #12 - Сумма чека
					amount = R2(buf.ToReal());
					r = AddTempCheck(&chk_id, numSmena, CCHKF_ZCHECK, cash_no, chk_no, 0, 0, &dttm, amount, 0);
					THROW(r);
					break;
				}
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::ImportSession(int idx)
{
	int    ok = 1;
	SString path;
	if(idx >= 0 && (uint)idx < ZRepList.getCount()) {
		long file_no   = ZRepList.at(idx).Key;
		long num_smena = ZRepList.at(idx).Val;
		if(path.GetSubFrom(ImportedFiles, ';', file_no) > 0 && fileExists(path)) {
			THROW(CreateTables());
			THROW(ConvertWareList(path, num_smena));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SHTRIHMFRK::FinishImportSession(PPIDArray * pSessList)
{
	//
	// @v6.6.5 AHTOXA Удалим файлы импорта.
	// Так как Minipos дописывает инфо о сессиях в эти файлы, из-за чего они постоянно растут
	//
	StringSet ss(';', ImportedFiles);
	SString path;
	for(uint i = 0; ss.get(&i, path);)
		SFile::Remove(path);
	//
	return 1; // pSessList->addUnique(&SessAry);
}
