// SETSTART.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019
// @codepage UTF-8
// Интерфейс (асинхронный) к драйверу SetStart (аналогичен ФРОНТОЛ'у)
//
#include <pp.h>
#pragma hdrstop
#include <frontol.h>
// @v9.6.2 (moved to pp.h) #include <ppidata.h>

static void RcvMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_RCVMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).CatChar('/').Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

class ACS_SETSTART : public PPAsyncCashSession {
public:
	SLAPI  ACS_SETSTART(PPID id);
	virtual int SLAPI ExportData(int updOnly);
	virtual int SLAPI GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int SLAPI ImportSession(int);
	virtual int SLAPI FinishImportSession(PPIDArray *);
	virtual int SLAPI SetGoodsRestLoadFlag(int updOnly);
protected:
	PPID   StatID;
private:
	int    SLAPI ConvertWareList(const char * pImpPath);
	long   SLAPI ModifDup(long cashNo, long chkNo);
	int    SLAPI ImportFiles();
	int    SLAPI GetZRepList(const char * pPath, _FrontolZRepArray * pZRepList);
	int    SLAPI QueryFile(uint setNo, const char * pImpPath);

	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	PPIDArray SessAry;
	SString PathRpt;
	SString PathFlag;
	SString PathGoods;
	SString PathGoodsFlag;
	int    UseAltImport;
	int    CrdCardAsDsc;
	int    SkipExportingDiscountSchemes;
	int    ImpExpTimeout;
	int    ImportDelay;
	StringSet ImpPaths;
	StringSet ExpPaths;
	PPAsyncCashNode Acn;
	SString   ImportedFiles;
	_FrontolZRepArray ZRepList;
};

class CM_SETSTART : public PPCashMachine {
public:
	SLAPI CM_SETSTART(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * SLAPI AsyncInterface() { return new ACS_SETSTART(NodeID); }
};

#define CM_ATOLWOATOLCARD  CM_ATOL
#define ACS_ATOLWOATOLCARD ACS_SETSTART

REGISTER_CMT(SETSTART, 0, 1);

SLAPI ACS_SETSTART::ACS_SETSTART(PPID id) : PPAsyncCashSession(id), ImpExpTimeout(0), ImportDelay(0), CrdCardAsDsc(0)
{
	SkipExportingDiscountSchemes = 0;
	int    v = 0;
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ATOL_TIMEOUT, &ImpExpTimeout);
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_FRONTOL_IMPORTDELAY, &ImportDelay);
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_FRONTOLCRDCARDASDSC, &CrdCardAsDsc);
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_FRONTOLSKIPDISCOUNTSCHEMES, &(v = 0)) > 0 && v == 1) {
		SkipExportingDiscountSchemes = 1;
	}
	UseAltImport = 0;
	StatID = 0;
	ChkRepPeriod.Z();
}

int SLAPI ACS_SETSTART::SetGoodsRestLoadFlag(int updOnly)
{
	int    ok = -1, use_replace_qtty_wosale = 0;
	PPIniFile  ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ATOL_QTTYWOSALE, &use_replace_qtty_wosale);
	if(use_replace_qtty_wosale && (updOnly || PPMessage(mfConf|mfYesNo, PPCFM_LOADGDSRESTWOSALES) == cmYes)) {
		Flags |= PPACSF_LOADRESTWOSALES;
		ok = 1;
	}
	else
		Flags &= ~PPACSF_LOADRESTWOSALES;
	return ok;
}

int SLAPI ACS_SETSTART::ExportData(int updOnly)
{
	int    ok = 1, next_barcode = 0;
	uint   i;
	//char   load_symb = '$';
	const  char * p_load_symb = "$";
	const  char * p_format = "%s\n";
	PPID   prev_goods_id = 0 /*, stat_id = 0*/;
	LAssocArray  grp_n_level_ary;
	SString   f_str, tail, temp_buf, email_subj;
	SString   path_goods, path_flag;
	//
	PPID      gc_alc_id = 0;
	SString   gc_alc_code; // Код класса товаров, относящихся к алкоголю
	PPIDArray alc_goods_list;
	//
	PPUnit    unit_rec;
	PPObjUnit unit_obj;
	PPObjQuotKind qk_obj;
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPSCardSerPacket scs_pack;
	PPObjGoods goods_obj;
	PPObjGoodsClass gc_obj;
	PPAsyncCashNode cn_data;
	LAssocArray  scard_quot_list;
	PPIDArray retail_quot_list;
	PPIDArray scard_series_list;
	BitArray used_retail_quot;
	PPIniFile ini_file;
	FILE * p_file = 0;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);

	THROW(GetNodeData(&cn_data) > 0);
	if(cn_data.DrvVerMajor > 3 || (cn_data.DrvVerMajor == 3 && cn_data.DrvVerMinor >= 4))
		p_load_symb = "#";
	//
	// Извлечем информацию о классе алкогольного товара
	//
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_GOODSCLASSALC, temp_buf) > 0 && temp_buf.NotEmptyS()) {
		StringSet ss(',', temp_buf);
		ss.get(&(i = 0), temp_buf.Z());
		if(gc_obj.SearchBySymb(temp_buf, &gc_alc_id) > 0) {
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(gc_alc_id, &gc_pack) > 0) {
				gc_alc_id = gc_pack.Rec.ID;
				(temp_buf = gc_pack.Rec.Symb).Strip();
				//
				// Если код товарного класса числовой, то используем для загрузки его, иначе - идентификатор
				// (фронтол требует цифрового кода, но использование идентификатора не желательно из-за возможного разнобоя между разделами БД).
				//
				if(temp_buf.ToLong() > 0)
					gc_alc_code = temp_buf;
				else
					gc_alc_code.Z().Cat(gc_alc_id);
			}
		}
	}
	// }
	THROW_MEM(SETIFZ(P_Dls, new DeviceLoadingStat));
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	PPWait(1);
	THROW(PPGetFilePath(PPPATH_OUT, "goods.txt",  path_goods));
	THROW(PPGetFilePath(PPPATH_OUT, "goods_flag.txt", path_flag));
	THROW_PP_S(p_file = fopen(path_flag, "w"), PPERR_CANTOPENFILE, path_flag);
	SFile::ZClose(&p_file);
	THROW_PP_S(p_file = fopen(path_goods, "w"), PPERR_CANTOPENFILE, path_goods);
	f_str = "##@@&&";
	THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	THROW_PP(fprintf(p_file, p_format, p_load_symb) > 0, PPERR_EXPFILEWRITEFAULT);
	{
		qk_obj.GetRetailQuotList(ZERODATETIME, &retail_quot_list, 0);
		used_retail_quot.insertN(0, retail_quot_list.getCount());
	}
	{
		//
		// Экспорт дисконтных карт
		//
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
		scard_quot_list.freeAll();
		MEMSZERO(ser_rec);
		if(!updOnly) {
			fputs((f_str = "$$$DELETEALLCCARDDISCS").CR(), p_file);
		}
		fputs((f_str = "$$$ADDCCARDDISCS").CR(), p_file);
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(ser_rec.GetType() == scstDiscount || (ser_rec.GetType() == scstCredit && CrdCardAsDsc)) {
				AsyncCashSCardInfo info;
				scard_series_list.add(ser_id); // @v9.4.1
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				THROW_SL(scard_quot_list.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
				for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
					f_str.Z();
					f_str.Cat(info.Rec.ID).Semicol();   // Card ID
					f_str.Cat(ser_rec.ID).Semicol();    // Series ID
					f_str.Cat(info.Rec.Code).Semicol(); // Code
					f_str.Cat(NZOR(info.Rec.Dt, encodedate(1, 1, 2000)), DATF_GERMAN|DATF_CENTURY).Semicol();
					f_str.Cat(NZOR(info.Rec.Expiry, encodedate(1, 1, 3000)), DATF_GERMAN|DATF_CENTURY).Semicol();
					f_str.Cat(info.IsClosed ? 0 : 1).Semicol();  // Passive | Active
					f_str.Cat(0L).Semicol();                     // Скидка уменьшающая цену в процентах (0)
					f_str.Cat(fdiv100i(info.Rec.PDis), MKSFMTD(0, 2, NMBF_NOTRAILZ)).Semicol();
					f_str.Cat(1L).Semicol();                     // Скидка на сумму чека
					f_str.CatCharN(';', 7);
					f_str.CR();
					fputs(f_str, p_file);
					iter.SetStat();
				}
			}
		}
		scard_series_list.sortAndUndup();
	}
	if(!CrdCardAsDsc) {
		PPIDArray scs_list;
		//
		// Экспорт серий кредитных карт
		//
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(ser_rec.GetType() == scstCredit) {
				/*
					№ поля Тип поля Назначение
					1 Целое Код
					2 Строка Наименование
					3 Строка Текст для чека
					4 Целое
						Ввод сертификата:
						0 — вручную;
						1 — визуально;
						2 — ридером магнитных карт;
						3 — сканером штрихкода
					5 Целое
						Сумма к расчету:
						1 — номинал;
						2 — номинал с количеством
					6 Целое Не используетс
				*/
				if(scs_list.getCount() == 0) {
					if(!updOnly) {
						fputs((f_str = "$$$DELETEALLCARDGROUPS").CR(), p_file);
					}
					fputs((f_str = "$$$ADDCARDGROUPS").CR(), p_file);
				}
				f_str.Z();
				f_str.Cat(ser_rec.ID).Semicol();   // #1 Series ID
				f_str.Cat(ser_rec.Name).Semicol(); // #2 Name
				f_str.Cat(ser_rec.Name).Semicol(); // #3 Текст для чека
				f_str.Cat(0L).Semicol();           // #4 Ввод сертификата: 0 — вручную; 1 — визуально; 2 — ридером магнитных карт; 3 — сканером штрихкода
				f_str.Cat(1L).Semicol();           // #5 Сумма к расчету: 1 — номинал; 2 — номинал с количеством
				f_str.Cat(0L).Semicol();           // #6 Reserved
				f_str.Transf(CTRANSF_INNER_TO_OUTER).CR();
				fputs(f_str, p_file);
				//
				scs_list.add(ser_rec.ID);
			}
		}
		if(scs_list.getCount()) {
			//
			// Экспорт кредитных карт (сертификатов)
			//
			AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
			scard_quot_list.freeAll();
			MEMSZERO(ser_rec);
			if(!updOnly) {
				fputs((f_str = "$$$DELETEALLCARDS").CR(), p_file);
			}
			fputs((f_str = "$$$ADDCARDS").CR(), p_file);
			for(uint i = 0; i < scs_list.getCount(); i++) {
				const PPID ser_id = scs_list.get(i);
				PPSCardSerPacket scs_pack;
				if(scs_obj.GetPacket(ser_id, &scs_pack) > 0) {
					AsyncCashSCardInfo info;
					for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
						/*
							№ поля Тип поля Назначение
							1 Целое Код группы сертификатов
							2 Целое Код сертификата
							3 Строка Наименование
							4 Строка Текст для чека
							5 Целое Начало диапазона длин сертификатов
							6 Целое Конец диапазона длин сертификатов
							7 Строка Начало диапазона префиксов
							8 Строка Конец диапазона префиксов
							9 Дата Начальная дата действия сертификата
							10 Дата Конечная дата действия сертификата
							11 Целое Не используетс
							12 Целое Номинал
							13 Целое
							Состояние:
							0 — неактивный;
							1 — активный
						*/
						f_str.Z();
						f_str.Cat(ser_rec.ID).Semicol();    // #1 Series ID
						f_str.Cat(info.Rec.ID).Semicol();   // #2 Card ID
						f_str.Cat(info.Rec.Code).Semicol(); // #3 Code
						f_str.Cat(info.Rec.Code).Semicol(); // #4 Текст для чека
						const long len = (long)sstrlen(info.Rec.Code);
						f_str.Cat(len).Semicol();           // #5 Начало диапазона длин сертификатов
						f_str.Cat(len).Semicol();           // #6 Конец диапазона длин сертификатов
						f_str.Cat(info.Rec.Code).Semicol(); // #7 Начало диапазона префиксов
						f_str.Cat(info.Rec.Code).Semicol(); // #8 Конец диапазона префиксов
						f_str.Cat(NZOR(info.Rec.Dt, encodedate(1, 1, 2000)), DATF_GERMAN | DATF_CENTURY).Semicol();     // #09 Начальная дата действия сертификата
						f_str.Cat(NZOR(info.Rec.Expiry, encodedate(1, 1, 3000)), DATF_GERMAN | DATF_CENTURY).Semicol(); // #10 Конечная дата действия сертификата
						f_str.Semicol();                       // #11 Не используется //
						f_str.Cat(R0i(info.Rest)).Semicol(); // #12 Конец диапазона длин сертификатов
						f_str.Cat(info.IsClosed ? 0 : 1).Semicol();  // #13 Passive | Active
						f_str.CR();
						fputs(f_str, p_file);
						iter.SetStat();
					}
				}
			}
		}
	}
	if(updOnly || (Flags & PPACSF_LOADRESTWOSALES)) {
		f_str = (Flags & PPACSF_LOADRESTWOSALES) ? "$$$REPLACEQUANTITYWITHOUTSALE" : "$$$REPLACEQUANTITY";
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	else {
		f_str = "$$$REPLACEQUANTITY";
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	{
		//long   iter_flags = updOnly ? ACGIF_UPDATEDONLY : 0;
		long   acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		if(cn_data.ExtFlags & CASHFX_EXPLOCPRNASSOC)
			acgif |= ACGIF_INITLOCPRN;
		AsyncCashGoodsIterator goods_iter(NodeID, acgif, SinceDlsID, P_Dls);
		if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
			AsyncCashGoodsGroupInfo grp_info;
			AsyncCashGoodsGroupIterator * p_group_iter = goods_iter.GetGroupIterator();
			if(p_group_iter) {
				while(p_group_iter->Next(&grp_info) > 0) {
					f_str.Z();
					f_str.Cat(grp_info.ID).CatCharN(';', 2);      // #1 - ИД группы товаров, #2 - не используется //
					tail = grp_info.Name;
					tail.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4).Semicol();
					f_str.Cat(tail);                              // #3 - Наименование группы товаров
					f_str.Cat(tail);                              // #4 - Текст для чека (?)
					f_str.CatCharN(';', 8);                       // #5-#12 - Не используем
					f_str.Cat(ATOL_OUTER_SCHEME).Semicol();       // #13 - Код схемы внешней автоматической скидки
					f_str.CatCharN(';', 2);                       // #14-#15 - Не используем
					f_str.Cat(grp_info.ParentID).Semicol();       // #16 - ИД родительской группы
					f_str.CatChar('0').Semicol();                 // #17 - Признак товарной группы (0)
					f_str.Cat((long)grp_info.Level).Semicol();    // #18 - Номер уровня иерархического списка
					f_str.CatCharN(';', 4);                       // #19-#22 - Не используем
					f_str.Semicol();                              // #23 - Налоговую группу не грузим
					THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
					grp_n_level_ary.Add(grp_info.ID, grp_info.Level, 0);
				}
			}
		}
		{
			AsyncCashGoodsInfo gds_info;
			TSArray <AtolGoodsDiscountEntry> goods_dis_list;
			PPIDArray rmv_goods_list;
			PrcssrAlcReport::GoodsItem agi;
			for(i = 0; i < retail_quot_list.getCount(); i++) {
				gds_info.QuotList.Add(retail_quot_list.get(i), 0, 1);
			}
			while(goods_iter.Next(&gds_info) > 0) {
				if(gds_info.GoodsFlags & GF_PASSIV && cn_data.ExtFlags & CASHFX_RMVPASSIVEGOODS && gds_info.Rest <= 0.0) {
					rmv_goods_list.addUnique(gds_info.ID);
				}
				else {
					size_t bclen;
	   				if(gds_info.ID != prev_goods_id) {
						long   level = 0;
						PPID   dscnt_scheme_id = 0;
						if(prev_goods_id) {
							f_str.Transf(CTRANSF_INNER_TO_OUTER).Semicol().Cat(tail);
							THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
						}
						if(gc_alc_id && gc_alc_code.NotEmpty() && gds_info.GdsClsID == gc_alc_id) {
							alc_goods_list.add(gds_info.ID);
						}
						//
						// Формирование массива скидок по котировкам
						//
						{
							AtolGoodsDiscountEntry ent;
							for(i = 0; i < retail_quot_list.getCount(); i++) {
								const  PPID qk_id = retail_quot_list.get(i);
								if(qk_id) {
									double quot = gds_info.QuotList.Get(qk_id);
									if(quot > 0.0 && quot != gds_info.Price) {
										MEMSZERO(ent);
										ent.GoodsID = gds_info.ID;
										ent.QuotKindID = qk_id;
										//ent.SCardSerID = scard_quot_list.at(i).Key;
										ent.AbsDiscount = gds_info.Price - quot;
										goods_dis_list.insert(&ent);
										used_retail_quot.set(i, 1);
									}
								}
							}
							if(gds_info.ExtQuot > 0.0) {
								MEMSZERO(ent);
								ent.GoodsID = gds_info.ID;
								ent.AbsDiscount = gds_info.Price - gds_info.ExtQuot;
								goods_dis_list.insert(&ent);
							}
						}
						next_barcode = 0;
						f_str.Z();
						f_str.Cat(gds_info.ID).Semicol();               // #1 - ИД товара
						tail = gds_info.Name;
						tail.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4).Semicol(); // #3 - Наименование товара
						tail.Cat(tail);                                 // #4 - Текст для чека
						tail.Cat(gds_info.Price, SFMT_MONEY).Semicol(); // #5 - Цена товара
						if(CheckCnFlag(CASHF_EXPGOODSREST))
							tail.Cat(gds_info.Rest, SFMT_QTTY);
						tail.Semicol();                                 // #6 - Остаток
						if(dscnt_scheme_id && gds_info.NoDis <= 0)      // && gds_info.NoDis <= 0
							tail.Cat(dscnt_scheme_id);
						tail.Semicol();                                 // #7 - Код схемы внутренней автоматической скидки
						// #8 - Флаги {
		   				if(unit_obj.Fetch(gds_info.UnitID, &unit_rec) > 0)
							tail.CatChar((unit_rec.Flags & PPUnit::IntVal) ? '0' : '1'); // Разрешить дробное кол-во
						else {
							//
							// Если для товара не определена единица измерения (правда, так быть не должно),
							// то НЕ разрешаем дробное количество
							//
							tail.CatChar('0');
						}
						for(i = 0; i < 4; i++)
							tail.Comma().CatChar('1'); // Разрешить продажи, возврат, отриц.остатки, регистрацию без указания кол-ва
						tail.Semicol();
						// }
						if(gds_info.NoDis > 0)
							tail.Cat(gds_info.Price, SFMT_MONEY); // #9 - Min цена товара
						tail.Semicol();
						tail.CatCharN(';', 3);                          // #10-#12 - Не используем
						if(gds_info.NoDis <= 0)                         //
							tail.Cat(ATOL_OUTER_SCHEME);                // #13 - Код схемы внешней автоматической скидки
						tail.Semicol();
						tail.CatCharN(';', 2);                          // #14-#15 - Не используем
						if(cn_data.Flags & CASHF_EXPGOODSGROUPS && gds_info.ParentID && grp_n_level_ary.Search(gds_info.ParentID, &level, 0) > 0)
							tail.Cat(gds_info.ParentID);               // #16 - ИД группы товаров
						else
							tail.CatChar('0');
						tail.Semicol();
						tail.CatChar('1').Semicol();                    // #17 - Признак товара (1)
						tail.Cat(level + 1).Semicol();                  // #18 - Номер уровня иерархического списка
						tail.CatCharN(';', 3);                          // #19-#21 - Не используем
						tail.Cat(gds_info.AsscPosNodeSymb).Semicol();   // #22 - Символ кассового аппарата, ассоциированного с товаром
						tail.Semicol();                                 // #23 Налоговую группу не грузим
						tail.CatCharN(';', 6);                          // #24-#29 - Не используем
						tail.Cat(strip(gds_info.LocPrnSymb)).Semicol(); // #30 - Символ локального принтера, ассоциированного с товаром
						tail.CatCharN(';', 22);                         // #31-#52 - Не используем
						if(goods_iter.GetAlcoGoodsExtension(gds_info.ID, 0, agi) > 0) {
							tail.Cat(agi.CategoryCode).Semicol();                               // #53 Код вида алкогольной продукции
                            tail.Cat(agi.Volume, MKSFMTD(0, 3, NMBF_NOZERO)).Semicol();         // #54 Емкость тары
                            tail.Cat(1L).Semicol();                                             // #55 Признак алкогольной продукции
                            tail.Cat((agi.StatusFlags & agi.stMarkWanted) ? 0L : 1L).Semicol(); // #56 Признак маркированной алкогольной продукции (0 - маркированная)
                            tail.Cat(agi.Proof, MKSFMTD(0, 1, NMBF_NOZERO)).Semicol();          // #57 Крепость алкогольной продукции
						}
						else {
							tail.Semicol();         // #53 Код вида алкогольной продукции
							tail.Semicol();         // #54 Емкость тары
							tail.Semicol();         // #55 Признак алкогольной продукции
							tail.Semicol();         // #56 Признак маркированной алкогольной продукции (пока НЕТ)
							tail.Semicol();         // #57 Крепость алкогольной продукции
						}
					}
					if((bclen = sstrlen(gds_info.BarCode)) != 0) {
						gds_info.AdjustBarcode(check_dig);
						int    wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
						if(wp == 1) {
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtPrefix));
						}
						else if(wp == 2) {
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtCntPrefix));
						}
						else
							AddCheckDigToBarcode(gds_info.BarCode);
						if(next_barcode)
							f_str.Comma();
						next_barcode = 1;
						f_str.Cat(gds_info.BarCode); // #2 - Список штрихкодов через запятую
					}
				}
	   			prev_goods_id = gds_info.ID;
				PPWaitPercent(goods_iter.GetIterCounter());
			}
			if(prev_goods_id) {
				f_str.Transf(CTRANSF_INNER_TO_OUTER).Semicol().Cat(tail);
				THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
			}
			//
			// Список товаров на удаление.
			//
			if(rmv_goods_list.getCount()) {
				fputs((f_str = "$$$DELETEWARESBYWARECODE").CR(), p_file);
				for(i = 0; i < rmv_goods_list.getCount(); i++) {
					f_str.Z().Cat(rmv_goods_list.get(i));  // #1 - ИД товара
					fputs(f_str.CR(), p_file);
				}
			}
			//
			// Список алкогольных товаров
			//
			if(alc_goods_list.getCount()) {
				alc_goods_list.sortAndUndup();
				fputs((f_str = "$$$ADDCLASSIFIERLINKS").CR(), p_file);
				for(i = 0; i < alc_goods_list.getCount(); i++) {
					f_str.Z().Cat(gc_alc_code).Semicol().Cat(1L).Semicol().Cat(alc_goods_list.get(i)).CR();
					fputs(f_str, p_file);
				}
			}
			// @9.4.1 {
			const long default_scheme_id = 999999L;
			fputs((f_str = "$$$ADDSETTINGS").CR(), p_file);
			fputs((f_str = "InternalDefaultSchmRecID").Semicol().Cat(default_scheme_id).CR(), p_file);
			if(scard_series_list.getCount()) {
				long   dscnt_code = 0;
				PPQuotKind qk_rec;
				SString card_code_low, card_code_upp;
				if(!updOnly) {
					fputs((f_str = "$$$DELETEALLAUTODISCCONDS").CR(), p_file);
				}
				int    is_first = 1;
				for(uint scsidx = 0; scsidx < scard_series_list.getCount(); scsidx++) {
					const PPID scs_id = scard_series_list.get(scsidx);
					if(scs_obj.GetPacket(scs_id, &scs_pack) > 0 && scs_pack.Rec.PDis != 0) {
						//int SLAPI SCardCore::GetPrefixRanges(PPID seriesID, uint maxLen, TSCollection <PrefixRange> & rRanges)
						TSCollection <SCardCore::PrefixRange> prefix_ranges;
						CC.Cards.GetPrefixRanges(scs_id, 10, prefix_ranges);
						for(uint pridx = 0; pridx < prefix_ranges.getCount(); pridx++) {
							const SCardCore::PrefixRange * p_prefix_range = prefix_ranges.at(pridx);
							if(is_first) {
								fputs((f_str = "$$$ADDAUTODISCCONDS").CR(), p_file);
								is_first = 0;
							}
							f_str.Z();
							// Для Сет-Старт default-scheme = 999999
							f_str.Cat(default_scheme_id).Semicol();               // #1 - код схемы внутренней авт.скидки
							f_str.Cat(scs_id).Semicol();                          // #2 - код скидки
							f_str.Semicol();                                      // #3 - unused
							(temp_buf = ser_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
							f_str.Cat(temp_buf).Semicol();                        // #4 - наименование скидки (код карты)
							f_str.Cat(0L).Semicol();                              // #5 - тип скидки (0 - percent, 1 - absolute)
							f_str.Cat(fdiv100i(scs_pack.Rec.PDis)).Semicol();     // #6 - значение скидки
							//
							// #7-#8 Период действия цены
							//
							f_str.CatCharN(';', 2);
							f_str.Semicol();                                      // #9 reserve
							f_str.Cat(scs_pack.Eb.UsageTmStart, TIMF_HMS).Semicol(); // #10 время начала действия скидки
							if(scs_pack.Eb.UsageTmEnd) {                             // #11 время окончания действия скидки
								f_str.Cat(scs_pack.Eb.UsageTmEnd, TIMF_HMS).Semicol();
							}
							else {
								f_str.Semicol().Semicol();
							}
							//f_str.Semicol();                                          // #12 reserve
							//
							//                                                        #12 - #24 - не используем
							//
							temp_buf.Z().CatCharN(';', 2).CatChar('0').Semicol();
							f_str.Cat(temp_buf);
							f_str.Cat(temp_buf);
							f_str.Cat(temp_buf);
							f_str.Cat(temp_buf);
							f_str.Cat(p_prefix_range->Low).Semicol();              // #25, #26 Диапазон номеров карт
							f_str.Cat(p_prefix_range->Upp).Semicol();
							f_str.CatChar('0').Semicol();                         // #27 - reserve
							//
							//
							//
							f_str.Semicol();                                      // #28
							f_str.Semicol();                                      // #29
							f_str.Semicol();                                      // #30
							f_str.Semicol();                                      // #31
							f_str.Semicol();                                      // #32
							f_str.Semicol();                                      // #33
							f_str.Semicol();                                      // #34 - ИД товара
							f_str.Semicol();                                      // #35
							f_str.Semicol();                                      // #36
							f_str.Semicol();                                      // #37
							f_str.Semicol();                                      // #38
							f_str.Semicol();                                      // #39
							f_str.Semicol();                                      // #40
							f_str.Semicol();                                      // #41
							f_str.Cat(p_prefix_range->Len).Semicol();             // #42
							f_str.Semicol();                                      // #43
							f_str.Semicol();                                      // #44
							THROW_PP(fprintf(p_file, p_format, f_str.Transf(CTRANSF_INNER_TO_OUTER).cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
						}
					}
				}
			}
			// } @9.4.1
#if 0 // @v9.4.1 {
			if(!SkipExportingDiscountSchemes && used_retail_quot.getCountVal(1)) {
				long   dscnt_code = 0;
				PPQuotKind qk_rec;
				SString card_code_low, card_code_upp;
				if(!updOnly) {
					fputs((f_str = "$$$DELETEALLAUTODISCSCHMS").CR(), p_file);
				}
				fputs((f_str = "$$$ADDAUTODISCSCHMS").CR(), p_file);
				for(i = 0; i < retail_quot_list.getCount(); i++) {
					const PPID qk_id = retail_quot_list.get(i);
					if(used_retail_quot.get(i) && qk_obj.Fetch(qk_id, &qk_rec) > 0) {
						f_str.Z().Cat(qk_id).Semicol();                     // #1 - код схемы внутренней авт.скидки
						f_str.Cat(qk_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CatCharN(';', 2);     // #2 - наименование схемы, #3 - не используется //
						f_str.CatChar('0').Semicol();                         // #4 - тип операции объединения (0 - не объединять)
						THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
					}
				}
				if(!updOnly) {
					fputs((f_str = "$$$DELETEALLAUTODISCCONDS").CR(), p_file);
				}
				fputs((f_str = "$$$ADDAUTODISCCONDS").CR(), p_file);
				for(i = 0; i < goods_dis_list.getCount(); i++) {
					const AtolGoodsDiscountEntry & r_ent = goods_dis_list.at(i);
					PPID   sc_ser_id = 0;
					if(r_ent.QuotKindID)
						scard_quot_list.SearchByVal(r_ent.QuotKindID, &sc_ser_id, 0);
					TimeRange tmr;
					f_str.Z();
					// Для Сет-Старт default-scheme = default_scheme_id
					f_str.Cat(NZOR(r_ent.QuotKindID, default_scheme_id)).Semicol(); // #1 - код схемы внутренней авт.скидки
					f_str.Cat(++dscnt_code).Semicol();                    // #2 - код скидки
					                                                      // #3 - наименование скидки (код карты) {
					temp_buf.Z();
					if(qk_obj.Fetch(r_ent.QuotKindID, &qk_rec) > 0)
						temp_buf.Cat(qk_rec.Name);
					else {
						qk_rec.ID = 0;
						temp_buf.Cat("EXT DISCOUNT");
					}
					f_str.Cat(temp_buf).Semicol();
					// } #3 - наименование скидки (код карты)
					f_str.Cat(temp_buf).Semicol();                           // #4 - текст для чека
					f_str.Cat((r_ent.AbsDiscount > 0.0) ? 1 : 3).Semicol();  // #5 - тип скидки
					f_str.Cat(fabs(r_ent.AbsDiscount)).Semicol();            // #6 - значение скидки
					//
					// #7-#8 Период действия цены
					//
					if(qk_rec.ID && !qk_rec.Period.IsZero()) {
						f_str.Cat(qk_rec.Period.low, DATF_GERMAN|DATF_CENTURY).Semicol();
						f_str.Cat(qk_rec.Period.upp, DATF_GERMAN|DATF_CENTURY).Semicol();
					}
					else
						f_str.CatCharN(';', 2);
					f_str.Semicol();                                         // #9 reserve
					//
					//                                                       #10-#11 время начала и окончания действия скидки
					//
					if(qk_rec.ID && qk_rec.GetTimeRange(tmr) > 0)
						f_str.Cat(tmr.low, TIMF_HMS).Semicol().Cat(tmr.upp, TIMF_HMS).Semicol();
					else
						f_str.CatCharN(';', 2);
					f_str.Semicol(); // #12 reserve
					//
					//                                                       #13 - #24 - не используем
					//
					temp_buf.Z().CatCharN(';', 2).CatChar('0').Semicol();
					f_str.Cat(temp_buf);
					f_str.Cat(temp_buf);
					f_str.Cat(temp_buf);
					f_str.Cat(temp_buf);
					//
					//                                                       #24, #26 Диапазон номеров карт
					//
					if(sc_ser_id) {
						if(scs_obj.GetCodeRange(sc_ser_id, card_code_low, card_code_upp) > 0) {
							f_str.Cat(card_code_low).Semicol();
							f_str.Cat(card_code_upp).Semicol();
						}
						else {
							f_str.Cat(1L).Semicol();
							f_str.Cat(9L).Semicol();
						}
					}
					else
						f_str.Semicol().Semicol();
					f_str.CatChar('0').Semicol();                         // #27 - reserve
					//
					//
					//
					f_str.Semicol();                                      // #28
					f_str.Semicol();                                      // #29
					f_str.Semicol();                                      // #30
					f_str.Semicol();                                      // #31
					f_str.Semicol();                                      // #32
					f_str.Semicol();                                      // #33
					f_str.Cat(r_ent.GoodsID).Semicol();                   // #34 - ИД товара
					f_str.Semicol();                                      // #35
					f_str.Semicol();                                      // #36
					f_str.Semicol();                                      // #37
					f_str.Semicol();                                      // #38
					f_str.Semicol();                                      // #39
					f_str.Semicol();                                      // #40
					f_str.Semicol();                                      // #41
					f_str.Semicol();                                      // #42
					f_str.Semicol();                                      // #43
					f_str.Semicol();                                      // #44
					THROW_PP(fprintf(p_file, p_format, f_str.Transf(CTRANSF_INNER_TO_OUTER).cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
				}
			}
#endif // } @v9.4.1
		}
	}
	SFile::ZClose(&p_file);
	PPWait(0);
	PPWait(1);
	(email_subj = SUBJECTFRONTOL).Cat("001"); // Пока только для кассы с номером 1
	THROW(DistributeFile(path_goods, 0, 0, email_subj));
	THROW(DistributeFile(path_flag,  0));
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI ACS_SETSTART::ImportFiles()
{
	const  long   delay_quant = 5 * 60 * 1000; // 5 мин
	const  int    notify_timeout = (ImpExpTimeout) ? ImpExpTimeout : (1 * 60 * 60 * 1000); // таймаут по умолчанию - 1 час.
	const  double timeouts_c = (double)notify_timeout / (double)delay_quant;
	const  char * p_ftp_flag = "ftp:";
	const  char * p_email_flag = "email";
	int    ok = 1;
	int    ftp_connected = 0;
	int    mail_connected = 0;
	uint   set_no = 0;
	SString imp_path, exp_path, path_rpt, path_flag, str_imp_paths;
	SString dir_in;
	LDATE  first_date = ChkRepPeriod.low;
	LDATE  last_date = ChkRepPeriod.upp;
	StringSet imp_paths(";");
	PPInternetAccount mac_rec;
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	WinInetFTP ftp;
	PPMailPop3 mail(0);

	PPGetPath(PPPATH_IN, dir_in);
	ImportedFiles.Z();
	SETIFZ(last_date, plusdate(LConfig.OperDate, 2));
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	if(EqCfg.FtpAcctID)
		THROW(obj_acct.Get(EqCfg.FtpAcctID, &acct));
	{
		PPAlbatrosConfig alb_cfg;
		MEMSZERO(mac_rec);
		if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0 && alb_cfg.Hdr.MailAccID) {
			THROW_PP(obj_acct.Get(alb_cfg.Hdr.MailAccID, &mac_rec) > 0, PPERR_UNDEFMAILACC);
		}
	}
	for(uint i = 0, set_no = 0; ImpPaths.get(&i, imp_path); set_no++) {
		if(imp_path.CmpPrefix(p_ftp_flag, 1) == 0) {
			SString ftp_path, ftp_path_flag, ftp_dir, file_name;
			SPathStruc sp;
			imp_path.ShiftLeft(sstrlen(p_ftp_flag));
			if(!ftp_connected) {
				THROW(ftp.Init());
				THROW(ftp.Connect(&acct));
				ftp_connected = 1;
			}
			{
				SPathStruc sp(imp_path);
				sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, ftp_dir);
				sp.Split(PathRpt);
				sp.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, file_name);
				(ftp_path = ftp_dir).SetLastSlash().Cat(file_name);
				sp.Split(PathFlag);
				sp.Merge(0, SPathStruc::fDrv|SPathStruc::fDir|SPathStruc::fExt, file_name);
				(ftp_path_flag = ftp_dir).SetLastSlash().Cat(file_name);
			}
			MakeTempFileName(dir_in, "front", "txt", 0, path_rpt.Z());
			// path_rpt = PathRpt;
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
					SDelay(delay_quant);
			}
		}
		else if(imp_path.CmpPrefix(p_email_flag, 1) == 0) {
			if(mac_rec.ID) {
				uint   i;
				PPIDArray msg_list;
				long   mailbox_count = 0, mailbox_size = 0;
				int    msg_n = 0;
				IterCounter msg_counter;
				SString wait_msg;
				SString temp_fname;
				SString dest_fname;

				mail.Init(&mac_rec);
				mac_rec.GetExtField(MAEXSTR_RCVSERVER, wait_msg);
				PPWaitMsg(PPSTR_TEXT, PPTXT_WTMAILCONNECTION, wait_msg);
				if(!mail_connected) {
					THROW(mail.Connect());
					THROW(mail.Login());
					mail_connected = 1;
				}
				THROW(mail.GetStat(&mailbox_count, &mailbox_size));
				PPLoadText(PPTXT_CHECKINMAILFORPPY, wait_msg);
				for(msg_n = 1; msg_n <= mailbox_count; msg_n++) {
					SMailMessage msg;
					if(mail.GetMsgInfo(msg_n, &msg) > 0)
						if(msg.Flags & SMailMessage::fFrontol)
							msg_list.add(msg_n);
					PPWaitPercent(msg_n, mailbox_count, wait_msg);
				}
				msg_counter.Init(msg_list.getCount());
				for(i = 0; i < msg_list.getCount(); i++) {
					SMailMessage msg;
					PPMakeTempFileName(0, "msg", 0, temp_fname.Z());
					msg_counter.Increment();
					THROW(mail.GetMsg(msg_list.at(i), &msg, temp_fname, RcvMailCallback, msg_counter));
					{
						MakeTempFileName(dir_in, "front", "txt", 0, path_rpt.Z());
						THROW(mail.SaveAttachment(temp_fname, PathRpt, dir_in));
						(temp_fname = dir_in).SetLastSlash().Cat(PathRpt);
						rename(temp_fname, path_rpt);
						ImportedFiles.Cat(path_rpt);
						THROW(mail.DeleteMsg(msg_list.at(i)));
					}
				}
			}
		}
		else
			THROW(QueryFile(set_no, imp_path));
	}
	{
		const char * p_prefix = "slp";
		SString path;
		SString temp_path;
		SString temp_dir = dir_in;
		for(uint file_no = 0; path.GetSubFrom(ImportedFiles, ';', file_no) > 0; file_no++) {
			if(fileExists(path)) {
				SPathStruc sp(path);
				// sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, temp_dir);
				// удаление временных файлов, если их кол-во стало больше 30 {
				{
					uint   files_count = 0;
					LDATETIME dtm;
					dtm.SetFar();
					SString firstf_path;
					SDirEntry sde;
					// @v9.4.10 dtm.d = MAXLONG;
					// @v9.4.10 dtm.t = MAXLONG;
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
				// }
                MakeTempFileName(temp_dir.SetLastSlash(), p_prefix, "txt", 0, temp_path);
				SCopyFile(path, temp_path, 0, 0, 0);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SETSTART::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtrErr(&dlg)) {
			SString dt_buf;
			ChkRepPeriod.SetDate(LConfig.OperDate);
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
		THROW(GetNodeData(&Acn) > 0);
		Acn.GetLogNumList(LogNumList);
		{
			SString  alt_imp_params;
			PPIniFile  ini_file;
			if(Acn.ExtFlags & CASHFX_CREATEOBJSONIMP) {
				UseAltImport = 1;
				if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_ACSCLOSE_USEALTIMPORT, alt_imp_params) > 0) {
					uint    pos = 0;
					SString param;
					StringSet params(';', alt_imp_params);
					params.get(&pos, param);
					// UseAltImport = BIN(param.ToLong());
					if(UseAltImport) {
						params.get(&pos, param);
						if(param.NotEmptyS()) {
							uint  pos = 0;
							StringSet  file_names(',', param);
							file_names.get(&pos, PathRpt);
							file_names.get(&pos, PathFlag);
							file_names.get(&pos, PathGoods);
							file_names.get(&pos, PathGoodsFlag);
						}
					}
				}
			}
			if(!PathRpt.NotEmptyS()) {
				//THROW(PPGetFileName(PPFILNAM_ATOL_EXP_TXT,   PathRpt));
				PathRpt = "report.txt";
			}
			if(!PathFlag.NotEmptyS()) {
				//THROW(PPGetFileName(PPFILNAM_ATOL_EXP_FLAG,  PathFlag));
				PathFlag.Z(); // "atolexp.flg";
			}
			if(!PathGoods.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_TXT, PathGoods));
			if(!PathGoodsFlag.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_FLG, PathGoodsFlag));
		}
		THROW_PP(Acn.ExpPaths.NotEmptyS() || Acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
		ImpPaths.clear();
		ImpPaths.setDelim(";");
		{
			SString & r_list = Acn.ImpFiles.NotEmpty() ? Acn.ImpFiles : Acn.ExpPaths;
			ImpPaths.setBuf(r_list, r_list.Len()+1);
		}
		ExpPaths.clear();
		ExpPaths.setDelim(";");
		{
			SString & r_list = Acn.ExpPaths.NotEmpty() ? Acn.ExpPaths : Acn.ImpFiles;
			ExpPaths.setBuf(r_list, r_list.Len()+1);
		}
		THROW(ImportFiles());
	}
	CATCHZOK
	*pSessCount = BIN(ok > 0);
	*pIsForwardSess = 0;
	delete dlg;
	return ok;
}

#define FRONTOL_DUPCHECK_OFFS 0x3FFF8000

long SLAPI ACS_SETSTART::ModifDup(long cashNo, long chkNo)
{
	/*
	if(Acn.ExtFlags & CASHFX_CREATEOBJSONIMP)
		if(!SearchTempCheckByCode(cashNo, chkNo))
			chkNo += FRONTOL_DUPCHECK_OFFS;
	*/
	return chkNo;
}

int SLAPI ACS_SETSTART::GetZRepList(const char * pPath, _FrontolZRepArray * pZRepList)
{
	int    ok = 1, field_no = 0;
	SString path, buf;
	_FrontolZRepArray zrep_list;
	uint   pos = 0;
	long   op_type = 0, nsmena = 0, cash_no = 0;
	LAssocArray zrep_ary; // Пара {номер_файла; номер_смены}
	SString imp_file_name = pPath;
	SFile  imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name

	PPSetAddedMsgString(imp_file_name);
	THROW_SL(imp_file.IsValid());
	for(pos = 0; pos < 3; pos++)
		imp_file.ReadLine(buf);
	//
	// #loop01
	// Собираем список Z-отчетов в массив zrep_ary.
	// Для каждого найденного Z-отчета создаем запись в таблице кассовых сессий (CS)
	// и заносим идентификатор сессии в массив ACS_ATOS::SessAry.
	//
	while(imp_file.ReadLine(buf) > 0) {
		LDATETIME dtm;
		StringSet ss(';', 0);

		ss.clear();
		ss.add(buf);
		ss.get(&(pos = 0), buf);     // #01 Код транзакции (не используем)
		ss.get(&pos, buf);           // #02 Дата транзакции
		strtodate(buf.Strip(), DATF_DMY, &dtm.d);
		ss.get(&pos, buf);           // #03 Время транзакции
		strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
		ss.get(&pos, buf);           // #04 Тип транзакции (операции)
		op_type = buf.ToLong();
		if(op_type == FRONTOL_OPTYPE_ZREPORT) {
			ss.get(&pos, buf);       // #05 Номер ККМ
			cash_no = buf.ToLong();
			if(LogNumList.lsearch(cash_no)) {
				PPID   sess_id;
				ss.get(&pos, buf);           // #06
				ss.get(&pos, buf);           // #07
				ss.get(&pos, buf);           // #08
				ss.get(&pos, buf);           // #09
				ss.get(&pos, buf);           // #10
				ss.get(&pos, buf);           // #11
				ss.get(&pos, buf);           // #12
				ss.get(&pos, buf);           // #13
				ss.get(&pos, buf);           // #14 Номер смены
				nsmena = buf.ToLong(); //
				if(CS.SearchByNumber(&sess_id, NodeID, cash_no, nsmena, dtm) > 0) {
					if(CS.data.Temporary)
						THROW(CS.ResetTempSessTag(sess_id, 0));
				}
				else
					THROW(CS.CreateSess(&sess_id, NodeID, cash_no, nsmena, dtm, 0));
				SessAry.addUnique(sess_id);
				{
					_FrontolZRepEntry z_entry;
					z_entry.PosN = cash_no;
					z_entry.ZRepN = nsmena;
					z_entry.SessID = sess_id;
					zrep_list.insert(&z_entry);
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pZRepList, zrep_list);
	return ok;
}

int SLAPI ACS_SETSTART::ConvertWareList(const char * pImpPath)
{
	int    ok = 1, field_no;
	uint   pos;
	long   op_type, nsmena, cash_no, chk_no;
	long   count = 0;
	PPID   grp_id = 0, goods_id = 0;
	PPIDArray   new_goods;
	//LAssocArray zrep_ary; // Пара {номер_кассы; номер_смены}
	LDATETIME dtm;
	SString   buf, card_code, wait_msg;
	SString   barcode, goods_name, arcode;
	SString   goods_ident_txt; // Текстовый идентификатор товара
	StringSet ss(';', 0);
	IterCounter   cntr;
	PPObjSCard sc_obj;
	SCardTbl::Rec sc_rec;
	PPGoodsPacket gds_pack;
	PPObjGoods    goods_obj;
	PPGoodsConfig goods_cfg;
	UintHashTable closed_check_list; // Список идентификаторов закрытых чеков.
		// Необходим для предотвращения повторного сброса строк одного чека, если
		// во входном файле дублируются чеки.
	FrontolCcPayment cc_payment;
	SString   imp_file_name = PathRpt;
	// SPathStruc::ReplacePath(imp_file_name, pImpPath, 1);
	SFile     imp_file(pImpPath, SFile::mRead); // PathRpt-->imp_file_name

	PPObjGoods::ReadConfig(&goods_cfg);
	const PPID def_goods_id = (goods_cfg.DefGoodsID && goods_obj.Fetch(goods_cfg.DefGoodsID, 0) > 0) ? goods_cfg.DefGoodsID : 0;
	THROW(imp_file.IsValid());
	for(pos = 0; pos < 3; pos++)
		imp_file.ReadLine(buf);
	{
		PPTransaction tra(1);
		THROW(tra);
		//
		imp_file.Seek(0);
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		while(imp_file.ReadLine(buf) > 0) {
			ss.clear();
			ss.add(buf);
			ss.get(&(pos = 0), buf);     // #01 Код транзакции (не используем)
			ss.get(&pos, buf);           // #02 Дата транзакции
			strtodate(buf.Strip(), DATF_DMY, &dtm.d);
			ss.get(&pos, buf);           // #03 Время транзакции
			strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
			ss.get(&pos, buf);           // #04 Тип транзакции (операции)
			op_type = buf.ToLong();
			if(op_type == FRONTOL_OPTYPE_CHKCLOSED) {
				ss.get(&pos, buf);       // #05 Номер ККМ
				cash_no = buf.ToLong();
				if(LogNumList.lsearch(cash_no)) {
					int    hour, min;
					long   chk_type;
					PPID   id = 0;
					LDATETIME dttm2;
					ss.get(&pos, buf);   // #06 Номер чека
					chk_no = buf.ToLong();
					chk_no = ModifDup(cash_no, chk_no);
					decodetime(&hour, &min, 0, 0, &dtm.t);
					dtm.t = encodetime(hour, min, (int)(chk_no % 60), 0);
					dttm2 = dtm;
					ss.get(&pos, buf);       // #07 пропускаем
					ss.get(&pos, card_code); // #08 Код дисконтной карты
					for(field_no = 8; field_no < 13 && ss.get(&pos, buf) > 0; field_no++); // #9-12 пропускаем
					chk_type = buf.ToLong(); // #13 Тип чека
					ss.get(&pos, buf);       // #14 Номер смены
					nsmena = buf.ToLong();
					if(oneof2(chk_type, FRONTOL_CHK_SALE, FRONTOL_CHK_RETURN)) {
						long   fl  = (chk_type == FRONTOL_CHK_RETURN) ? CCHKF_RETURN : 0;
						if(!ZRepList.Search(cash_no, nsmena, &(pos = 0)))
							fl |= CCHKF_TEMPSESS;
						THROW(AddTempCheck(&id, nsmena, fl, cash_no, chk_no, 0, 0, dttm2, 0, 0));
					}
					if(card_code.NotEmptyS() && sc_obj.P_Tbl->SearchCode(0, card_code, &sc_rec) > 0)
						THROW(AddTempCheckSCardID(id, sc_rec.ID));
				}
			}
		}
		imp_file.Seek(0);
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		cntr.Init(ZRepList.getCount());
		PPLoadText(PPTXT_ACSCLS_IMPORT, wait_msg);
		//
		// Значение cur_zrep_list_pos соответствует текущему Z-отчету. Позиция смещается аналогично
		// порядку считывания записей о Z-отчетах в цикле выше (#loop01) при обнаружении записи с типом
		// транзакции FRONTOL_OPTYPE_ZREPORT
		//
		for(uint cur_zrep_list_pos = 0; imp_file.ReadLine(buf) > 0;) {
			int    r;
			long   op_type, cash_no, chk_no;
			const  long cur_zrep_n = (cur_zrep_list_pos < ZRepList.getCount()) ? ZRepList.at(cur_zrep_list_pos).ZRepN : 0;
			ss.clear();
			ss.add(buf);
			pos = 0;
			//   № транзакции, дата транзакции, время транзакции - пропускаем, выбираем тип транзакции (операции)
			for(field_no = 0; field_no < 4 && ss.get(&pos, buf) > 0; field_no++);
										 // #01 Номер транзакции
										 // #02 Дата транзакции
										 // #03 Время транзакции
			op_type = buf.ToLong();      // #04 Тип операции
			ss.get(&pos, buf);           // #05 Номер ККМ
			cash_no = buf.ToLong();
			ss.get(&pos, buf);           // #06 Номер чека
			chk_no = buf.ToLong();
			chk_no = ModifDup(cash_no, chk_no);
			ss.get(&pos, buf);           // #07 Код кассира - пропускаем
			if(op_type == FRONTOL_OPTYPE_ZREPORT) {
				if(LogNumList.lsearch(cash_no)) {
					const  uint preserve_pos = pos;
					ss.get(&pos, buf);   // #08 skip
					ss.get(&pos, buf);   // #09 Номер смены (no)

					ss.get(&pos, buf);   // #10
					ss.get(&pos, buf);   // #11
					ss.get(&pos, buf);   // #12
					ss.get(&pos, buf);   // #13
					ss.get(&pos, buf);   // #14
					nsmena = buf.ToLong();

					pos = preserve_pos;
					uint zrep_pos = 0;
					if(ZRepList.Search(cash_no, nsmena, &zrep_pos)) {
						assert(zrep_pos == cur_zrep_list_pos);
						cur_zrep_list_pos++;
					}
					else {
						const int FRONTOL_CURRENT_ZREP_POSITION_FAULT = 0;
						assert(FRONTOL_CURRENT_ZREP_POSITION_FAULT);
					}
				}
			}
			else if(oneof4(op_type, FRONTOL_OPTYPE_CHKLINE, FRONTOL_OPTYPE_STORNO, FRONTOL_OPTYPE_CHKLINEFREE, FRONTOL_OPTYPE_STORNOFREE)) { // Строка чека или сторно строки
				double qtty = 0.0;
				double price = 0.0;
				double dscnt_price = 0.0;
				double dscnt = 0.0;
				int    div = 0; // Номер отдела
				const  int is_free_price = oneof2(op_type, FRONTOL_OPTYPE_CHKLINEFREE, FRONTOL_OPTYPE_STORNOFREE);
				ss.get(&pos, goods_ident_txt); // #08 ID товара
				ss.get(&pos, buf);          // #09 Коды значений разрезов
				ss.get(&pos, buf);          // #10 Цена
				price = buf.ToReal();
				ss.get(&pos, buf);          // #11 Количество
				qtty = buf.ToReal();
				ss.get(&pos, buf);          // #12 Сумма товара + сумма округления (пропускаем)
				ss.get(&pos, buf);          // #13 Операция (пропускаем)
				ss.get(&pos, buf);          // #14 Номер смены (пропускаем)
				ss.get(&pos, buf);          // #15 Цена  со скидками
				dscnt_price = buf.ToReal(); //
				ss.get(&pos, buf);          // #16 Сумма со скидками
				dscnt = buf.ToReal();
				ss.get(&pos, buf);          // #17 пропускаем
				ss.get(&pos, buf);          // #18 пропускаем
				ss.get(&pos, barcode);      // #19 barcode
				ss.get(&pos, buf);          // #20 пропускаем
				ss.get(&pos, buf.Z());      // #21 Номер отдела
				div = buf.ToLong();
				THROW(r = SearchTempCheckByCode(cash_no, chk_no, cur_zrep_n));
				if(r > 0) {
					double line_amount;
					const  PPID chk_id = P_TmpCcTbl->data.ID;
					if(!closed_check_list.Has((ulong)chk_id)) {
						if(is_free_price) {
							goods_id = def_goods_id;
						}
						else {
							Goods2Tbl::Rec goods_rec;
							goods_id = goods_ident_txt.ToLong();
							if(UseAltImport) {
								goods_ident_txt.Divide('|', arcode, goods_name);
								if(!goods_name.NotEmptyS())
									goods_name.Z().CatEq("ID", goods_id);
								else
									goods_name.Transf(CTRANSF_OUTER_TO_INNER);
								if(goods_obj.P_Tbl->SearchByArCode(0, arcode, 0, &goods_rec) > 0)
									goods_id = goods_rec.ID;
								else {
									buf = "ATOL";
									if(!grp_id && goods_obj.P_Tbl->SearchByName(PPGDSK_GROUP, buf, &grp_id, &gds_pack.Rec) <= 0) {
										gds_pack.destroy();
										gds_pack.Rec.Kind = PPGDSK_GROUP;
										buf.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
										THROW(goods_obj.PutPacket(&grp_id, &gds_pack, 0));
									}
									gds_pack.destroy();
									gds_pack.Rec.Kind = PPGDSK_GOODS;
									gds_pack.Rec.ParentID = grp_id;

									PPID   dup_goods_id = 0L;
									if(goods_obj.P_Tbl->SearchByName(PPGDSK_GOODS, goods_name, &dup_goods_id, &goods_rec) > 0)
										goods_name.CatChar('_').Cat(buf);
									goods_name.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
									STRNSCPY(gds_pack.Rec.Abbr, gds_pack.Rec.Name);
									gds_pack.Rec.UnitID   = goods_cfg.DefUnitID;
									// gds_pack.Codes.Add((buf = "$").Cat(goods_id), -1, 1.0);
									THROW(new_goods.add(goods_id));
									THROW(goods_obj.PutPacket(&(goods_id = 0), &gds_pack, 0));
									THROW(goods_obj.P_Tbl->SetArCode(goods_id, 0, arcode, 0));
								}
							}
							// @v9.4.10 {
							if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
								; // ok
							}
							else {
								if(barcode.NotEmptyS() && goods_obj.SearchByBarcode(barcode, 0, &goods_rec) > 0) {
									goods_id = goods_rec.ID;
								}
								// @v10.1.7 В некоторых ситуациях на месте идентификатора может оказаться штрихкод товара: проверим эту гипотезу {
								else if(goods_ident_txt.NotEmptyS() && goods_obj.SearchByBarcode(goods_ident_txt, 0, &goods_rec) > 0) {
									goods_id = goods_rec.ID;
								}
								// } @v10.1.7
								else
									goods_id = 0;
							}
							// @v9.4.10 {
						}
						qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(qtty) : fabs(qtty);
						if(op_type == FRONTOL_OPTYPE_STORNO) {
							TempCCheckLineTbl::Key2 k2;
							k2.CheckID = chk_id;
							BExtQuery cclq(P_TmpCclTbl, 2);
							cclq.selectAll().where(P_TmpCclTbl->CheckID == chk_id);
							for(cclq.initIteration(0, &k2, spGe); cclq.nextIteration() > 0;)
								if(P_TmpCclTbl->data.GoodsID == goods_id && P_TmpCclTbl->data.Quantity == qtty) {
									THROW_DB(P_TmpCclTbl->deleteRec());
									break;
								}
							qtty = -qtty;
						}
						else {
							SetupTempCcLineRec(0, chk_id, chk_no, P_TmpCcTbl->data.Dt, div, goods_id);
							if(is_free_price)
								SetTempCcLineValues(0, qtty, dscnt_price, 0.0);
							else
								SetTempCcLineValues(0, qtty, price, price - dscnt_price);
							THROW_DB(P_TmpCclTbl->insertRec());
						}
						if(is_free_price) {
							line_amount = R2(qtty * dscnt_price);
							dscnt       = 0.0;
						}
						else {
							line_amount = R2(qtty * dscnt_price);
							dscnt       = R2(qtty * price - dscnt);
						}
						THROW(AddTempCheckAmounts(chk_id, line_amount, dscnt));
					}
				}
			}
			else if(oneof2(op_type, FRONTOL_OPTYPE_PAYM1, FRONTOL_OPTYPE_PAYM2)) { // Оплата
				long   paym_type;
				PPID   crd_card_id = 0;
				double _amount = 0.0;
				ss.get(&pos, buf);        // #8 - пропускаем
				ss.get(&pos, buf);        // #9 - тип оплаты
				paym_type = buf.ToLong(); // Тип оплаты (1 - наличные)
				ss.get(&pos, buf);        // #10
				ss.get(&pos, buf);        // #11
				ss.get(&pos, buf);        // #12
				_amount = buf.ToReal();
				ss.get(&pos, buf);        // #13
				ss.get(&pos, buf);        // #14
				ss.get(&pos, buf);        // #15 Credit card series ID
				ss.get(&pos, buf);        // #16 Credit card ID
				crd_card_id = buf.ToLong();
				/*
				if(paym_type != PAYMENT_CASH) {
					THROW(r = SearchTempCheckByCode(cash_no, chk_no));
					if(r > 0 && !(P_TmpCcTbl->data.Flags & CCHKF_BANKING)) {
						P_TmpCcTbl->data.Flags |= CCHKF_BANKING;
						THROW_DB(P_TmpCcTbl->updateRec());
					}
				}
				*/
				if(crd_card_id) {
					cc_payment.CrdSCardList.Add(crd_card_id, _amount);
				}
				else if(paym_type == FRONTOL_PAYMENT_CASH) {
					cc_payment.CashAmt += _amount;
				}
				else {
					cc_payment.BankAmt += _amount;
				}
				cc_payment.Amount += _amount;
			}
			else if(op_type == FRONTOL_OPTYPE_CANCEL) { // Отмена чека
				THROW(r = SearchTempCheckByCode(cash_no, chk_no, cur_zrep_n));
				if(r > 0) {
					TempCCheckLineTbl::Key2 ccl_k2;
					ccl_k2.CheckID = P_TmpCcTbl->data.ID;
					P_TmpCcTbl->data.Flags &= ~CCHKF_BANKING;
					LDBLTOMONEY(0.0, P_TmpCcTbl->data.Amount);
					LDBLTOMONEY(0.0, P_TmpCcTbl->data.Discount);
					THROW_DB(P_TmpCcTbl->updateRec());
					while(P_TmpCclTbl->searchForUpdate(2, &ccl_k2, spEq))
						THROW_DB(P_TmpCclTbl->deleteRec()); // @sfu
				}
			}
			else if(op_type == FRONTOL_OPTYPE_CHKCLOSED) {
				THROW(r = SearchTempCheckByCode(cash_no, chk_no, cur_zrep_n));
				if(r > 0) {
					PPID   chk_id = P_TmpCcTbl->data.ID;
					PPID   scard_id = P_TmpCcTbl->data.SCardID;
					int    do_upd_rec = 0;
					if(cc_payment.CashAmt == cc_payment.Amount) {
					}
					else if(cc_payment.BankAmt == cc_payment.Amount) {
						if(!(P_TmpCcTbl->data.Flags & CCHKF_BANKING)) {
							P_TmpCcTbl->data.Flags |= CCHKF_BANKING;
							do_upd_rec = 1;
						}
					}
					else {
						if(cc_payment.BankAmt > 0.0) {
							if(cc_payment.BankAmt < cc_payment.Amount)
								THROW(AddTempCheckPaym(chk_id, CCAMTTYP_BANK, cc_payment.BankAmt, 0));
							if(!(P_TmpCcTbl->data.Flags & CCHKF_BANKING)) {
								P_TmpCcTbl->data.Flags |= CCHKF_BANKING;
								do_upd_rec = 1;
							}
						}
						if(cc_payment.CashAmt > 0.0) {
							if(cc_payment.CashAmt < cc_payment.Amount)
								THROW(AddTempCheckPaym(chk_id, CCAMTTYP_CASH, cc_payment.CashAmt, 0));
						}
						if(cc_payment.CrdSCardList.getCount()) {
							for(uint i = 0; i < cc_payment.CrdSCardList.getCount(); i++) {
								const RAssoc & r_item = cc_payment.CrdSCardList.at(i);
								THROW(AddTempCheckPaym(chk_id, CCAMTTYP_CRDCARD, r_item.Val, r_item.Key));
							}
						}
					}
					if(do_upd_rec) {
						THROW_DB(P_TmpCcTbl->updateRec());
					}
					closed_check_list.Add((ulong)chk_id);
				}
				cc_payment.Reset();
			}
			PPWaitPercent(cntr.Increment(), wait_msg);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SETSTART::QueryFile(uint setNo, const char * pImpPath)
{
	const  int  notify_timeout = NZOR(ImpExpTimeout, 5000);

	int    ok = 1;
	SString imp_path = pImpPath, exp_path;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	SETIFZ(last_date, plusdate(LConfig.OperDate, 2));
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	THROW(CreateTables());
	{
		{
			int exp_path_found = 0;
			for(uint j = 0, n = 0; !exp_path_found && ExpPaths.get(&j, exp_path); n++)
				if(n == setNo)
					exp_path_found = 1;
			if(!exp_path_found)
				exp_path = imp_path;
		}
		if(fileExists(imp_path)) {
			SString path_rpt = PathRpt;
			SString path_flag = PathFlag;
			SPathStruc::ReplacePath(path_rpt,  imp_path, 1);
			if(path_flag.NotEmpty()) {
				SPathStruc::ReplacePath(path_flag, exp_path, 1);
				THROW_PP(ok = WaitForExists(path_flag, 1, notify_timeout), PPERR_ATOL_IMPCHECKS);
				if(ok > 0) {
					int     y, m, d;
					SString buf, tmp_buf, tmp_name;
					SString date_mask = "%02d.%02d.%04d";
					SFile::Remove(path_rpt);
					tmp_name = path_flag;
					SPathStruc::ReplaceExt(tmp_name, "tmp", 1);
					SFile  query_file(tmp_name, SFile::mWrite);
					buf = "$$$TRANSACTIONSBYDATETIMERANGE";
					query_file.WriteLine(buf.CR());
					decodedate(&d, &m, &y, &first_date);
					buf.Z().Printf(date_mask, d, m, y).Semicol();
					decodedate(&d, &m, &y, &last_date);
					buf.Cat(tmp_buf.Printf(date_mask, d, m, y)).CR();
					query_file.WriteLine(buf);
					query_file.Close();
					//
					// Задержка для Windows Vista (and above) чтобы убедиться что файл на сетевом диске виден
					//
					while(!fileExists(tmp_name)) {
						SDelay(50);
					}
					SFile::Rename(tmp_name, path_flag);
					//
					// Задержка для Windows Vist (and above) чтобы убедиться что файл на сетевом диске виден
					//
					while(!fileExists(path_flag)) {
						SDelay(50);
					}
					THROW_PP(ok = WaitForExists(path_flag, 1, notify_timeout), PPERR_ATOL_IMPCHECKS);
				}
			}
			if(ok > 0) {
				if(ImportDelay >= 0)
					SDelay((ImportDelay > 0) ? ImportDelay : 1000);
				if(ImportedFiles.Len())
					ImportedFiles.Semicol();
				imp_path.SetLastSlash().Cat(PathRpt);
				ImportedFiles.Cat(imp_path);
			}
		}
		else {
			// log message
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SETSTART::ImportSession(int)
{
	int    ok = 1;
	StringSet ss(";");
	SString path;
	THROW(CreateTables());
	ss.setBuf(ImportedFiles, ImportedFiles.Len() + 1);
	for(uint i = 0; ss.get(&i, path.Z());) {
		ZRepList.freeAll();
		if(fileExists(path)) {
			THROW(GetZRepList(path, &ZRepList));
			THROW(ConvertWareList(path));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI ACS_SETSTART::FinishImportSession(PPIDArray * pSessList)
{
	//
	// Удалим файлы импорта.
	//
	StringSet ss(';', ImportedFiles);
	SString path, backup_path, backup_file_name;
	SPathStruc ps;
	for(uint i = 0; ss.get(&i, path);) {
		if(fileExists(path)) {
			ps.Split(path);
			ps.Nam.Z();
			ps.Ext.Z();
			ps.Merge(backup_path);
			backup_path.SetLastSlash().Cat("backup");
			if(::createDir(backup_path)) {
				MakeTempFileName(backup_path, "ssr", "txt", 0, backup_file_name);
				SCopyFile(path, backup_file_name, 0, FILE_SHARE_READ, 0);
			}
			SFile::Remove(path);
		}
	}
	return pSessList->addUnique(&SessAry);
}
