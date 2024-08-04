// FRONTOL.CPP
// Copyright (c) V.Nasonov 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
// Интерфейс (асинхронный) к драйверу "Атол"
//
#include <pp.h>
#pragma hdrstop
#include <frontol.h>

static void RcvMailCallback(const IterCounter & bytesCounter, const IterCounter & msgCounter)
{
	SString msg;
	PPLoadText(PPTXT_RCVMAILWAITMSG, msg);
	if(msgCounter.GetTotal() > 1)
		msg.Space().Cat(msgCounter).Slash().Cat(msgCounter.GetTotal());
	PPWaitPercent(bytesCounter, msg);
}

class ACS_FRONTOL : public PPAsyncCashSession {
public:
	explicit ACS_FRONTOL(PPID id);
	virtual int ExportData(int updOnly);
	virtual int GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int ImportSession(int);
	virtual int FinishImportSession(PPIDArray *);
	virtual int SetGoodsRestLoadFlag(int updOnly);
protected:
	PPID   StatID;
private:
	int    IsXPos(const PPAsyncCashNode & rCn) const { return BIN(rCn.DrvVerMajor >= 10); }
	int    ConvertWareList(const char * pImpPath);
	long   ModifDup(long cashNo, long chkNo);
	int    ImportFiles();
	int    GetZRepList(const char * pPath, _FrontolZRepArray * pZRepList);
	int    QueryFile(uint setNo, const char * pImpPath);
	int    ExportMarketingActions_XPos(int updOnly, StringSet & rSsResult); // @construction

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

class CM_FRONTOL : public PPCashMachine {
public:
	CM_FRONTOL(PPID cashID) : PPCashMachine(cashID)
	{
	}
	PPAsyncCashSession * AsyncInterface() { return new ACS_FRONTOL(NodeID); }
};

#define CM_ATOLWOATOLCARD  CM_ATOL
#define ACS_ATOLWOATOLCARD ACS_FRONTOL

REGISTER_CMT(FRONTOL, false, true);

ACS_FRONTOL::ACS_FRONTOL(PPID id) : PPAsyncCashSession(id), ImpExpTimeout(0), ImportDelay(0), CrdCardAsDsc(0), SkipExportingDiscountSchemes(0)
{
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

int ACS_FRONTOL::SetGoodsRestLoadFlag(int updOnly)
{
	int    ok = -1;
	int    use_replace_qtty_wosale = 0;
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

int ACS_FRONTOL::ExportMarketingActions_XPos(int updOnly, StringSet & rSsResult) // @construction
{
	int    ok = 1;
	// $$$ADDMARKETINGACTIONS (1..12)
	// 
	// 1  Да  Строка 10  Код маркетинговой акции
	// 2  Да  Дата*      Дата начала действия акции
	// 3  Да  Дата*      Дата окончания действия акции
	// 4  Нет Время*     Время начала действия акции
	// 5  Нет Время*     Время окончания действия акции
	// 6  Нет Строка 100 Наименование
	// 7  Нет Строка 100 Текст для чека
	// 8  Нет Целое      Состояние настройки «Активная»: 0 – выключена; 1 – включена (по умолчанию)
	// 9  Нет Целое      Приоритет акции (от 0 до 1млрд, где 0 – наивысший приоритет). Значение по умолчанию = 0
	// 10 Нет Целое      Состояние настройки «Работает всегда»: • 0 – выключена (по умолчанию); • 1 – включена
	// 11 Нет Целое      Состояние настройки «Весь день»: • 0 – выключена (по умолчанию); • 1 – включена
	// 12 Нет Не используется 
	//
	// $$$ADDMARKETINGEVENTS
	//
	// 1 Да Строка 10 Код маркетингового мероприятия
	// 2 Да Строка 10 Код маркетинговой акции
	// 3 Нет Строка 100 Наименование
	// 4 Нет Строка 100 Текст для чека
	// 5 Да  Целое      Действия:
	//   1 – скидка на документ;
	//   2 – товарная скидка;
	//   3 – скидка на набор;
	//   4 – запрет продажи;
	//   5 – ручная скидка;
	//   8 – специальные цены;
	//   15 – оповещение.
	//
	// $$$ADDMARKETINGCONDITIONS
	//
	// 1 Да Строка[10] Код маркетингового мероприятия
	// 2 Да Целое      Тип условия:
	//   1 – сумма документа;
	//   2 – сумма количества всех позиций;
	//   3 – количество позиций;
	//   4 – товар;
	//   5 – диапазон карт;
	//   6 – время;
	//   7 – дни недели
	//   
	PPObjSCardSeries scs_obj;
	PPSCardSeries scs_rec;
	PPIDArray scs_list;
	PPSCardSerPacket scs_pack;
	SString f_str;
	rSsResult.add("$$$DELETEALLMARKETINGACTIONS");
	for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
		if(scs_rec.GetType() == scstDiscount || (scs_rec.GetType() == scstCredit && CrdCardAsDsc)) {
			if(scs_rec.PDis != 0) 
				scs_list.add(ser_id);
		}
	}
	if(scs_list.getCount()) {
		scs_list.sortAndUndup();
		long   dscnt_code = 0;
		PPQuotKind qk_rec;
		SString card_code_low, card_code_upp;

		int    is_first = 1;
		rSsResult.add("$$$ADDMARKETINGACTIONS");
		f_str.Z().
			Cat("PPYRSCS-A").Semicol().
			Cat(encodedate(1, 1, 2018), DATF_GERMANCENT).Semicol().
			Cat(encodedate(31, 12, 2030), DATF_GERMANCENT).Semicol().
			Cat(encodetime(0, 0, 0, 0), TIMF_HMS).Semicol().
			Cat(encodetime(24, 0, 0, 0), TIMF_HMS).Semicol().
			Cat("Card Discount").Semicol().
			Cat("Card Discount").Semicol().
			Cat(1L).Semicol(). // Включена
			Cat(0L).Semicol(). // Priority
			Cat(1L).Semicol(). // Работает всегда
			Cat(1L).Semicol(). // Работает весь день
			Cat("").Semicol(); // @reserve
		rSsResult.add(f_str);
		for(uint scsidx = 0; scsidx < scs_list.getCount(); scsidx++) {
			const  PPID scs_id = scs_list.get(scsidx);
			if(scs_obj.GetPacket(scs_id, &scs_pack) > 0 && scs_pack.Rec.PDis != 0) {
				//int SCardCore::GetPrefixRanges(PPID seriesID, uint maxLen, TSCollection <PrefixRange> & rRanges)
				TSCollection <SCardCore::PrefixRange> prefix_ranges;
				CC.Cards.GetPrefixRanges(scs_id, 10, prefix_ranges);
				if(prefix_ranges.getCount()) {
					rSsResult.add("$$$ADDMARKETINGEVENTS");
					f_str.Z().
						Cat(scs_pack.Rec.ID).Semicol().
						Cat("PPYRSCS-A").Semicol().
						Cat("Card Discount").Semicol().
						Cat("Card Discount").Semicol().
						Cat(1).Semicol(). // Скидка на документ
						Cat(1).Semicol(). // Процент
						Cat(fdiv100i(scs_pack.Rec.PDis), MKSFMTD_020); // Скидка
					rSsResult.add(f_str);
					for(uint pridx = 0; pridx < prefix_ranges.getCount(); pridx++) {
						const SCardCore::PrefixRange * p_pr = prefix_ranges.at(pridx);
						if(p_pr) {
							rSsResult.add("$$$ADDMARKETINGCONDITIONS");
							f_str.Z().
								Cat(scs_pack.Rec.ID).Semicol().
								Cat(5L).Semicol(). // Диапазон карт
								Cat(1L).Semicol(). // Введена
								Cat(p_pr->Low).Semicol().
								Cat(p_pr->Upp).Semicol().
								Cat(p_pr->Low.Len()).Semicol().
								Cat(p_pr->Upp.Len()).Semicol();
							rSsResult.add(f_str);
						}
					}
				}
			}
		}
	}
	return ok;
}

int ACS_FRONTOL::ExportData(int updOnly)
{
	int    ok = 1;
	int    next_barcode = 0;
	uint   i;
	const  char * p_load_symb = "$";
	const  char * p_format = "%s\n";
	PPID   prev_goods_id = 0 /*, stat_id = 0*/;
	LAssocArray  grp_n_level_ary;
	SString f_str;
	SString tail;
	SString temp_buf;
	SString email_subj;
	SString path_goods;
	SString path_flag;
	PPID      gc_alc_id = 0;
	SString   gc_alc_code; // Код класса товаров, относящихся к алкоголю
	PPIDArray alc_goods_list;
	PPUnit    unit_rec;
	PPObjUnit unit_obj;
	PPObjQuotKind qk_obj;
	PPObjSCardSeries scs_obj;
	PPSCardSeries scs_rec;
	PPObjGoods goods_obj;
	PPObjGoodsClass gc_obj;
	PPAsyncCashNode cn_data;
	LAssocArray  scard_quot_list;
	PPIDArray retail_quot_list;
	SBitArray used_retail_quot;
	PPIniFile ini_file;
	FILE * p_file = 0;
	PPObjCashNode cnobj;
	const  int check_dig = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	const  int is_vatfree = (cnobj.IsVatFree(NodeID) > 0);
	THROW(GetNodeData(&cn_data) > 0);
	const int is_xpos = IsXPos(cn_data);
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
	PPWaitStart();
	THROW(PPMakeTempFileName("frol", "txt", 0, path_goods));
	THROW(PPMakeTempFileName("frol", "flg", 0, path_flag));
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
	if(!is_xpos) {
		//
		// Экспорт дисконтных карт
		//
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
		scard_quot_list.freeAll();
		MEMSZERO(scs_rec);
		if(!updOnly) {
			fputs((f_str = "$$$DELETEALLCCARDDISCS").CR(), p_file);
		}
		fputs((f_str = "$$$ADDCCARDDISCS").CR(), p_file);
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
			if(scs_rec.GetType() == scstDiscount || (scs_rec.GetType() == scstCredit && CrdCardAsDsc)) {
				AsyncCashSCardInfo info;
				PPSCardSerPacket scs_pack;
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				THROW_SL(scard_quot_list.Add(scs_rec.ID, scs_rec.QuotKindID_s, 0));
				for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
					f_str.Z();
					f_str.Cat(info.Rec.ID).Semicol();   // Card ID
					f_str.Cat(scs_rec.ID).Semicol();    // Series ID
					f_str.Cat(info.Rec.Code).Semicol(); // Code
					f_str.Cat(NZOR(info.Rec.Dt, encodedate(1, 1, 2000)), DATF_GERMANCENT).Semicol();
					f_str.Cat(NZOR(info.Rec.Expiry, encodedate(1, 1, 3000)), DATF_GERMANCENT).Semicol();
					f_str.Cat((info.Flags & AsyncCashSCardInfo::fClosed) ? 0 : 1).Semicol();  // Passive | Active
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
	}
	if(!CrdCardAsDsc) {
		PPIDArray scs_list;
		//
		// Экспорт серий кредитных карт
		//
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &scs_rec) > 0;) {
			if(scs_rec.GetType() == scstCredit) {
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
				f_str.Cat(scs_rec.ID).Semicol();   // #1 Series ID
				f_str.Cat(scs_rec.Name).Semicol(); // #2 Name
				f_str.Cat(scs_rec.Name).Semicol(); // #3 Текст для чека
				f_str.Cat(0L).Semicol();           // #4 Ввод сертификата: 0 — вручную; 1 — визуально; 2 — ридером магнитных карт; 3 — сканером штрихкода
				f_str.Cat(1L).Semicol();           // #5 Сумма к расчету: 1 — номинал; 2 — номинал с количеством
				f_str.Cat(0L).Semicol();           // #6 Reserved
				f_str.Transf(CTRANSF_INNER_TO_OUTER).CR();
				fputs(f_str, p_file);
				//
				scs_list.add(scs_rec.ID);
			}
		}
		if(scs_list.getCount()) {
			//
			// Экспорт кредитных карт (сертификатов)
			//
			AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
			scard_quot_list.freeAll();
			MEMSZERO(scs_rec);
			if(!updOnly) {
				fputs((f_str = "$$$DELETEALLCARDS").CR(), p_file);
			}
			fputs((f_str = "$$$ADDCARDS").CR(), p_file);
			for(uint i = 0; i < scs_list.getCount(); i++) {
				const  PPID ser_id = scs_list.get(i);
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
						f_str.Cat(scs_rec.ID).Semicol();    // #1 Series ID
						f_str.Cat(info.Rec.ID).Semicol();   // #2 Card ID
						f_str.Cat(info.Rec.Code).Semicol(); // #3 Code
						f_str.Cat(info.Rec.Code).Semicol(); // #4 Текст для чека
						const long len = sstrleni(info.Rec.Code);
						f_str.Cat(len).Semicol();           // #5 Начало диапазона длин сертификатов
						f_str.Cat(len).Semicol();           // #6 Конец диапазона длин сертификатов
						f_str.Cat(info.Rec.Code).Semicol(); // #7 Начало диапазона префиксов
						f_str.Cat(info.Rec.Code).Semicol(); // #8 Конец диапазона префиксов
						f_str.Cat(NZOR(info.Rec.Dt, encodedate(1, 1, 2000)), DATF_GERMAN | DATF_CENTURY).Semicol();     // #09 Начальная дата действия сертификата
						f_str.Cat(NZOR(info.Rec.Expiry, encodedate(1, 1, 3000)), DATF_GERMAN | DATF_CENTURY).Semicol(); // #10 Конечная дата действия сертификата
						f_str.Semicol();                       // #11 Не используется //
						f_str.Cat(R0i(info.Rest)).Semicol(); // #12 Конец диапазона длин сертификатов
						f_str.Cat((info.Flags & AsyncCashSCardInfo::fClosed) ? 0 : 1).Semicol();  // #13 Passive | Active
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
							// @v10.8.2 AtolGoodsDiscountEntry ent;
							for(i = 0; i < retail_quot_list.getCount(); i++) {
								const  PPID qk_id = retail_quot_list.get(i);
								if(qk_id) {
									const double quot = gds_info.QuotList.Get(qk_id);
									if(quot > 0.0 && quot != gds_info.Price) {
										AtolGoodsDiscountEntry ent(gds_info.ID, qk_id, gds_info.Price - quot);
										goods_dis_list.insert(&ent);
										used_retail_quot.set(i, 1);
									}
								}
							}
							if(gds_info.ExtQuot > 0.0) {
								AtolGoodsDiscountEntry ent(gds_info.ID, 0, gds_info.Price - gds_info.ExtQuot);
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
							// Если для товара не определена единица измерения (правда, так быть не должно), то НЕ разрешаем дробное количество
							tail.CatChar('0');
						}
						for(i = 0; i < 3; i++)
							tail.Comma().CatChar('1'); // Разрешить продажи, возврат, отриц.остатки
						/*@v11.9.10{*/if(oneof2(gds_info.ChZnProdType, GTCHZNPT_DRAFTBEER, GTCHZNPT_DRAFTBEER_AWR)) { // @v12.0.5 GTCHZNPT_DRAFTBEER_AWR
							tail.Comma().CatChar('0'); // без указания кол-ва
						}
						else /*}@v11.9.10*/{
							tail.Comma().CatChar('1'); // без указания кол-ва
						}
						tail.Semicol();
						// }
						if(gds_info.NoDis > 0)
							tail.Cat(gds_info.Price, SFMT_MONEY);       // #9 - Min цена товара
						tail.Semicol();
						if(checkdate(gds_info.Expiry))
							tail.Cat(gds_info.Expiry, DATF_GERMANCENT); // #10 - Срок годности @v11.9.5
						tail.Semicol();
						tail.CatCharN(';', 2);                          // #11-#12 - Не используем
						if(cn_data.DrvVerMajor > 5 || (cn_data.DrvVerMajor == 5 && cn_data.DrvVerMinor >= 16)) {
							// Номер поля - 13; Обязательное - нет; Тип поля - целое;
							// Признак предмета расчёта: 0 – товар, кроме подакцизного; 1 – подакцизный товар; 2 – работа;
							// 3 – услуга; 4 – товар, состоящий из нескольких признаков; 5 – иной товар.
							if(cn_data.DrvVerMajor > 5 || (cn_data.DrvVerMajor == 5 && cn_data.DrvVerMinor >= 20)) {
								char _tag = '1';
								if(gds_info.Flags_ & AsyncCashGoodsInfo::fGExciseProForma) // @v11.7.10
									_tag = '2';
								tail.CatChar(_tag); // #13 - Признак предмета расчёта
							}
							else
								tail.CatChar('0'); // #13 - Признак предмета расчёта
						}
						else {
							if(gds_info.NoDis <= 0)                         // 
								tail.Cat(ATOL_OUTER_SCHEME);                // #13 - Код схемы внешней автоматической скидки
						}
						tail.Semicol();
						tail.CatCharN(';', 2);                          // #14-#15 - Не используем
						if(cn_data.Flags & CASHF_EXPGOODSGROUPS && gds_info.ParentID && grp_n_level_ary.Search(gds_info.ParentID, &level, 0))
							tail.Cat(gds_info.ParentID);               // #16 - ИД группы товаров
						else
							tail.CatChar('0');
						tail.Semicol();
						tail.CatChar('1').Semicol();                    // #17 - Признак товара (1)
						tail.Cat(level + 1).Semicol();                  // #18 - Номер уровня иерархического списка
						tail.CatCharN(';', 3);                          // #19-#21 - Не используем
						tail.Cat(gds_info.AsscPosNodeSymb).Semicol();   // #22 - Символ кассового аппарата, ассоциированного с товаром
						// @v10.9.4 tail.Semicol();                     // #23 Налоговую группу не грузим
						// @v10.9.4 {
						{
							/*
								1 - НДС 0%
								2 - НДС 10%
								3 - НДС 20%
								4 - Без НДС
							*/
							long   vat_code = 0;
							if(is_vatfree)
								vat_code = 4;
							else if(feqeps(gds_info.VatRate, 0.0, 1E-6))
								vat_code = 1;
							else if(feqeps(gds_info.VatRate, 10.0, 1E-6))
								vat_code = 2;
							else if(feqeps(gds_info.VatRate, 20.0, 1E-6))
								vat_code = 3;
							if(vat_code)
								tail.Cat(vat_code);                     // #23 Налоговая группа
							tail.Semicol();
						}
						// } @v10.9.4 
						tail.CatCharN(';', 6);                          // #24-#29 - Не используем
						tail.Cat(strip(gds_info.LocPrnSymb)).Semicol(); // #30 - Символ локального принтера, ассоциированного с товаром
						tail.CatCharN(';', 22);                         // #31-#52 - Не используем

						// #55 Признак типа номенклатуры: 0 – обычный товар; 1 – алкогольная продукция; 2 – изделия из меха;
						//    3 – лекарственные препараты; 4 – табачная продукция.
						if(goods_iter.GetAlcoGoodsExtension(gds_info.ID, 0, agi) > 0) {
							tail.Cat(agi.CategoryCode).Semicol();                               // #53 Код вида алкогольной продукции
                            tail.Cat(agi.Volume, MKSFMTD(0, 3, NMBF_NOZERO)).Semicol();         // #54 Емкость тары
							// @v11.9.3 {
							int mark_type = 1;
							if(gds_info.Flags_ & (AsyncCashGoodsInfo::fGMarkedType|AsyncCashGoodsInfo::fGMarkedCode)) {
								if(oneof2(gds_info.ChZnProdType, GTCHZNPT_DRAFTBEER, GTCHZNPT_DRAFTBEER_AWR)) // @v12.0.5 GTCHZNPT_DRAFTBEER_AWR
									mark_type = 18;
								else if(gds_info.ChZnProdType == GTCHZNPT_BEER) // @v12.0.4
									mark_type = 17;
							}
							// } @v11.9.3 
                            tail.Cat(mark_type).Semicol();                                      // #55 Признак алкогольной продукции // @v11.9.3 1L-->mark_type
                            tail.Cat((agi.StatusFlags & agi.stMarkWanted) ? 0L : 1L).Semicol(); // #56 Признак маркированной алкогольной продукции (0 - маркированная)
                            tail.Cat(agi.Proof, MKSFMTD(0, 1, NMBF_NOZERO)).Semicol();          // #57 Крепость алкогольной продукции
							// @v11.9.10 @fix (это поле формируется ниже после if-else) tail.Semicol(); // #58 Признак способа расчета // @v11.9.5
						}
						else {
							tail.Semicol();         // #53 Код вида алкогольной продукции
							tail.Semicol();         // #54 Емкость тары
							if(gds_info.Flags_ & (AsyncCashGoodsInfo::fGMarkedType|AsyncCashGoodsInfo::fGMarkedCode)) {
								/*
									0–товар;
									1–алкогольная продукция;
									4 –табачная продукция;
									5 –обувь;
									6–лотерея;
									7–иная маркированная продукция;
									8 –фототовары;
									9 –парфюмерная продукция;
									10 –шины;
									11–товары легкой промышленности;
									12 –альтернативная табачная продукция
								*/
								// @v11.0.9 {
								int mark_type = 0;
								switch(gds_info.ChZnProdType) {
									case GTCHZNPT_SHOE: mark_type = 5; break;
									case GTCHZNPT_TEXTILE: mark_type = 11; break;
									case GTCHZNPT_CARTIRE: mark_type = 10; break;
									case GTCHZNPT_PERFUMERY: mark_type = 9; break;
									case GTCHZNPT_TOBACCO: mark_type = 4; break; 
									case GTCHZNPT_ALTTOBACCO: mark_type = 12; break; // @v11.9.0 // @v11.9.2 4-->12
									case GTCHZNPT_MEDICINE: mark_type = 3; break;
									case GTCHZNPT_FUR: mark_type = 2; break;
									case GTCHZNPT_MILK: mark_type = 13; break; // @v11.3.5
									case GTCHZNPT_WATER: mark_type = 15; break; // @v11.5.6
									case GTCHZNPT_DRAFTBEER_AWR: mark_type = 18; break; // @v12.0.5
									case GTCHZNPT_DRAFTBEER: mark_type = 18; break; // @v11.9.2
									case GTCHZNPT_BEER: mark_type = 17; break; // @v12.0.3
									case GTCHZNPT_ANTISEPTIC: mark_type = 20; break; // @v12.0.5
									default:
										if(gds_info.ChZnProdType)
											mark_type = 7; // 7–иная маркированная продукция
										break;
								}
								// } @v11.0.9 
								if(mark_type)
									tail.Cat(mark_type); // #55 Признак алкогольной продукции
								tail.Semicol();
							}
							else
								tail.Semicol();     // #55 Признак алкогольной продукции        
							tail.Semicol();         // #56 Признак маркированной алкогольной продукции (пока НЕТ)
							tail.Semicol();         // #57 Крепость алкогольной продукции
						}
						if(cn_data.DrvVerMajor > 5 || (cn_data.DrvVerMajor == 5 && cn_data.DrvVerMinor >= 20))
							tail.CatChar('2').Semicol();  // #58 Признак способа расчета
						else
							tail.Semicol();               // #58 Признак способа расчета // @v11.9.5
					}
					// @v11.9.5 {
					tail.Semicol();               // #59 Код реквизитов агента
					tail.Semicol();               // #60 Сумма акциза
					tail.Semicol();               // #61 Код страны происхождения товара
					tail.Semicol();               // #62 Номер таможенной декларации
					tail.Semicol();               // #63 Наименование лотереи
					tail.Semicol();               // #64 Код вида номенклатурной классификации (EAN-13)
					tail.Semicol();               // #65 Проверка соответствия товара штрихкоду маркировки: 0 – по штрихкодам товара; 1 – по штрихкоду регистрации (def=0)
					{
						// @v11.9.6 {
						/*
							Мера количества предмета расчета:
							• 0 – штука;
							• 1 – грамм;
							• 2 – килограмм;
							• 3 – тонна;
							• 4 – сантиметр;
							• 5 – дециметр;
							• 6 – метр;
							• 7 – квадратный сантиметр;
							• 8 – квадратный дециметр;
							• 9 – квадратный метр;
							• 10 – миллилитр;
							• 11 – литр;
							• 12 – кубический метр;
							• 13 – киловатт час;
							• 14 – гигакалория;
							• 15 – сутки (день);
							• 16 – час;
							• 17 – минута;
							• 18 – секунда;
							• 19 – килобайт;
							• 20 – мегабайт;
							• 21 – гигабайт;
							• 22 – терабайт;
							• 23 – иная единица измерения.
							Значение по умолчанию: 0.
						*/
						int measure = 0;
						//if(goods_iter.IsSimplifiedDraftBeer(gds_info.ID)) {
						if(oneof2(gds_info.ChZnProdType, GTCHZNPT_DRAFTBEER, GTCHZNPT_DRAFTBEER_AWR)) // @v12.0.5 GTCHZNPT_DRAFTBEER_AWR
							measure = 11; // LITER
						// } @v11.9.6 
						tail.Cat(measure).Semicol(); // #66 Мера количества предмета расчета
					}
					tail.Semicol();               // #67 Признак рецептурности лекарственного препарата
					// } @v11.9.5 
					bclen = sstrlen(gds_info.BarCode);
					if(bclen) {
						gds_info.AdjustBarcode(check_dig);
						int    wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
						if(wp == 1)
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtPrefix));
						else if(wp == 2)
							STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtCntPrefix));
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
			if(is_xpos) {
				StringSet ma_ss;
				ExportMarketingActions_XPos(updOnly, ma_ss);
				for(uint massp = 0; ma_ss.get(&massp, f_str);) {
					fputs(f_str.CR(), p_file);
				}
			}
			else {
				if(!SkipExportingDiscountSchemes && used_retail_quot.getCountVal(1)) { 
					long   dscnt_code = 0;
					PPQuotKindPacket qk_pack;
					SString card_code_low, card_code_upp;
					if(!updOnly) {
						fputs((f_str = "$$$DELETEALLAUTODISCSCHMS").CR(), p_file);
					}
					fputs((f_str = "$$$ADDAUTODISCSCHMS").CR(), p_file);
					for(i = 0; i < retail_quot_list.getCount(); i++) {
						const  PPID qk_id = retail_quot_list.get(i);
						if(used_retail_quot.get(i) && qk_obj.Fetch(qk_id, &qk_pack) > 0) {
							f_str.Z().Cat(qk_id).Semicol();                       // #1 - код схемы внутренней авт.скидки
							f_str.Cat(qk_pack.Rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CatCharN(';', 2);     // #2 - наименование схемы, #3 - не используется //
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
						f_str.Cat(NZOR(r_ent.QuotKindID, 100)).Semicol(); // #1 - код схемы внутренней авт.скидки
						f_str.Cat(++dscnt_code).Semicol();                // #2 - код скидки
						// #3 - наименование скидки (код карты) {
						temp_buf.Z();
						if(qk_obj.Fetch(r_ent.QuotKindID, &qk_pack) > 0)
							temp_buf.Cat(qk_pack.Rec.Name);
						else {
							qk_pack.Rec.ID = 0;
							temp_buf.Cat("EXT DISCOUNT");
						}
						f_str.Cat(temp_buf).Semicol();
						// } #3 - наименование скидки (код карты)
						f_str.Cat(temp_buf).Semicol(); // #4 - текст для чека
						f_str.Cat((r_ent.AbsDiscount > 0.0) ? 1 : 3).Semicol();  // #5 - тип скидки
						f_str.Cat(fabs(r_ent.AbsDiscount)).Semicol();            // #6 - значение скидки
						//
						// #7-#8 Период действия цены
						//
						if(qk_pack.Rec.ID && !qk_pack.Rec.Period.IsZero()) {
							f_str.Cat(qk_pack.Rec.Period.low, DATF_GERMANCENT).Semicol();
							f_str.Cat(qk_pack.Rec.Period.upp, DATF_GERMANCENT).Semicol();
						}
						else
							f_str.CatCharN(';', 2);
						f_str.Semicol();                                         // #9 reserve
						//
						//                                                       #10-#11 время начала и окончания действия скидки
						//
						if(qk_pack.Rec.ID && qk_pack.Rec.GetTimeRange(tmr))
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
			}
		}
	}
	SFile::ZClose(&p_file);
	PPWaitStop();
	PPWaitStart();
	(email_subj = SUBJECTFRONTOL).Cat("001"); // Пока только для кассы с номером 1
	THROW(PPGetFileName(PPFILNAM_ATOL_IMP_TXT,  temp_buf)); // @v10.8.4 
	THROW(DistributeFile_(path_goods, temp_buf/*pEndFileName*/, dfactCopy, 0, email_subj));
	THROW(PPGetFileName(PPFILNAM_ATOL_IMP_FLAG, temp_buf)); // @v10.8.4 
	THROW(DistributeFile_(path_flag, temp_buf/*pEndFileName*/, dfactCopy, 0, 0));
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	PPWaitStop();
	return ok;
}

int ACS_FRONTOL::ImportFiles()
{
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	long   delay_quant = 5 * 60 * 1000; // 5 мин
	const  char * p_ftp_flag = "ftp:";
	const  char * p_email_flag = "email";
	int    ok = 1, ftp_connected = 0, notify_timeout = (ImpExpTimeout) ? ImpExpTimeout : (1 * 60 * 60 * 1000); // таймаут по умолчанию - 1 час.
	int    mail_connected = 0;
	double timeouts_c = fdivi(notify_timeout, delay_quant);
	uint   set_no = 0;
	SString imp_path, exp_path, path_rpt, path_flag, str_imp_paths;
	SString dir_in;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	StringSet imp_paths(";");
	PPInternetAccount mac_rec;
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	WinInetFTP ftp;
	PPMailPop3 mail(0);

	PPGetPath(PPPATH_IN, dir_in);
	ImportedFiles.Z();
	SETIFZ(last_date, plusdate(getcurdate_(), 2)); // @v10.8.10 LConfig.OperDate-->getcurdate_()
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	if(r_eq_cfg.FtpAcctID)
		THROW(obj_acct.Get(r_eq_cfg.FtpAcctID, &acct));
	{
		PPAlbatrossConfig alb_cfg;
		if(PPAlbatrosCfgMngr::Get(&alb_cfg) > 0 && alb_cfg.Hdr.MailAccID)
			THROW_PP(obj_acct.Get(alb_cfg.Hdr.MailAccID, &mac_rec) > 0, PPERR_UNDEFMAILACC);
	}
	for(uint i = 0, set_no = 0; ImpPaths.get(&i, imp_path); set_no++) {
		if(imp_path.HasPrefixIAscii(p_ftp_flag)) {
			SString ftp_path, ftp_path_flag, ftp_dir, file_name;
			SFsPath sp;
			imp_path.ShiftLeft(sstrlen(p_ftp_flag));
			if(!ftp_connected) {
				THROW(ftp.Init());
				THROW(ftp.Connect(&acct));
				ftp_connected = 1;
			}
			{
				SFsPath sp(imp_path);
				sp.Merge(0, SFsPath::fNam|SFsPath::fExt, ftp_dir);
				sp.Split(PathRpt);
				sp.Merge(0, SFsPath::fDrv|SFsPath::fDir, file_name);
				(ftp_path = ftp_dir).SetLastSlash().Cat(file_name);
				sp.Split(PathFlag);
				sp.Merge(0, SFsPath::fDrv|SFsPath::fDir|SFsPath::fExt, file_name);
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
		else if(imp_path.HasPrefixIAscii(p_email_flag)) {
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
		SString path, temp_path, temp_dir;
		temp_dir = dir_in;
		for(uint file_no = 0; path.GetSubFrom(ImportedFiles, ';', file_no) > 0; file_no++) {
			if(fileExists(path)) {
				SFsPath sp(path);
				// sp.Merge(0, SFsPath::fNam|SFsPath::fExt, temp_dir);
				// удаление временных файлов, если их кол-во стало больше 30 {
				{
					uint   files_count = 0;
					LDATETIME dtm;
					SString firstf_path;
					SDirEntry sde;
					dtm.SetFar();
					(temp_path = temp_dir.SetLastSlash()).Cat(p_prefix).Cat("?????.txt");
					for(SDirec sd(temp_path); sd.Next(&sde) > 0;) {
						LDATETIME iter_dtm;
						iter_dtm.SetNs100(sde.ModTm_);
						if(dtm.IsFar() || cmp(iter_dtm, dtm) < 0) {
							dtm = iter_dtm;
							sde.GetNameA(temp_dir, firstf_path);
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

int ACS_FRONTOL::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	const  LDATE _cur_date = getcurdate_(); // @v10.8.10 LConfig.OperDate-->getcurdate_()
	TDialog * dlg = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtrErr(&dlg)) {
			SString dt_buf;
			ChkRepPeriod.low = _cur_date;
			ChkRepPeriod.upp = _cur_date;
			dlg->SetupCalPeriod(CTLCAL_DATERNG_PERIOD, CTL_DATERNG_PERIOD);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, &ChkRepPeriod);
			PPWaitStop();
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && strtoperiod(dt_buf, &ChkRepPeriod, 0) && !ChkRepPeriod.IsZero()) {
					SETIFZ(ChkRepPeriod.upp, plusdate(_cur_date, 2));
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWaitStart();
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
			if(!PathRpt.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_EXP_TXT,   PathRpt));
			if(!PathFlag.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_EXP_FLAG,  PathFlag));
			if(!PathGoods.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_TXT, PathGoods));
			if(!PathGoodsFlag.NotEmptyS())
				THROW(PPGetFileName(PPFILNAM_ATOL_GOODS_FLG, PathGoodsFlag));
		}
		THROW_PP(Acn.ExpPaths.NotEmptyS() || Acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
		ImpPaths.Z().setDelim(";");
		{
			SString & r_list = Acn.ImpFiles.NotEmpty() ? Acn.ImpFiles : Acn.ExpPaths;
			ImpPaths.setBuf(r_list, r_list.Len()+1);
		}
		ExpPaths.Z().setDelim(";");
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

long ACS_FRONTOL::ModifDup(long cashNo, long chkNo)
{
	/*
	if(Acn.ExtFlags & CASHFX_CREATEOBJSONIMP)
		if(!SearchTempCheckByCode(cashNo, chkNo))
			chkNo += FRONTOL_DUPCHECK_OFFS;
	*/
	return chkNo;
}

int ACS_FRONTOL::GetZRepList(const char * pPath, _FrontolZRepArray * pZRepList)
{
	int    ok = 1, field_no = 0;
	SString path, buf;
	_FrontolZRepArray zrep_list;
	uint   pos = 0;
	long   op_type = 0;
	long   nsmena = 0;
	long   cash_no = 0;
	LAssocArray zrep_ary; // Пара {номер_файла; номер_смены}
	SString fld_buf_alt[2]; // @v10.8.3 Буферы для альтернативных вариантов значения (такое возможно)
	SString imp_file_name(pPath);
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
		ss.Z();
		ss.add(buf);
		ss.get(&(pos = 0), buf);     // #1 Код транзакции (не используем)
		ss.get(&pos, buf);           // #2 Дата транзакции
		strtodate(buf.Strip(), DATF_DMY, &dtm.d);
		ss.get(&pos, buf);           // #3 Время транзакции
		strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
		ss.get(&pos, buf);           // #4 Тип транзакции (операции)
		op_type = buf.ToLong();
		if(op_type == FRONTOL_OPTYPE_ZREPORT) {
			ss.get(&pos, buf);       // #5 Номер ККМ
			cash_no = buf.ToLong();
			if(LogNumList.lsearch(cash_no)) {
				PPID   sess_id;
				ss.get(&pos, buf);       // #6 
				ss.get(&pos, buf);       // #7 
				ss.get(&pos, buf);       // #8 // Поля 6-8 пропускаем
				ss.get(&pos, buf);       // #9 
				fld_buf_alt[0] = buf; // #9 Номер смены alt[0] // @v10.8.3 
				// @v10.8.3 nsmena = buf.ToLong(); // #9 Номер смены
				// @v10.8.3 {
				ss.get(&pos, buf);       // #10
				ss.get(&pos, buf);       // #11
				ss.get(&pos, buf);       // #12
				ss.get(&pos, buf);       // #13 // Поля 10-13 пропускаем
				ss.get(&pos, buf);       // #14 Номер смены alt[1]
				fld_buf_alt[1] = buf;
				if(fld_buf_alt[0].NotEmptyS())
					nsmena = fld_buf_alt[0].ToLong();
				else if(fld_buf_alt[1].NotEmptyS())
					nsmena = fld_buf_alt[1].ToLong();
				else
					nsmena = 0;
				// } @v10.8.3
				if(CS.SearchByNumber(&sess_id, NodeID, cash_no, nsmena, dtm) > 0) {
					if(CS.data.Temporary)
						THROW(CS.ResetTempSessTag(sess_id, 0));
				}
				else
					THROW(CS.CreateSess(&sess_id, NodeID, cash_no, nsmena, dtm, 0));
				SessAry.addUnique(sess_id);
				//zrep_ary.Add(cash_no, nsmena, &(pos = 0));
				{
					const _FrontolZRepEntry z_entry(cash_no, nsmena, sess_id);
					// @v10.8.2 z_entry.PosN = cash_no;
					// @v10.8.2 z_entry.ZRepN = nsmena;
					// @v10.8.2 z_entry.SessID = sess_id;
					zrep_list.insert(&z_entry);
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pZRepList, zrep_list);
	return ok;
}

int ACS_FRONTOL::ConvertWareList(const char * pImpPath)
{
	int    ok = 1;
	int    field_no;
	uint   pos;
	long   op_type;
	long   nsmena;
	long   cash_no;
	long   chk_no;
	long   count = 0;
	PPID   grp_id = 0;
	PPID   goods_id = 0;
	PPIDArray new_goods;
	LDATETIME dtm;
	SString buf, card_code, wait_msg;
	SString barcode;
	SString goods_name;
	SString arcode;
	SString fld_buf_alt[2]; // @v10.8.3 Буферы для альтернативных вариантов значения (такое возможно)
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
	// SFsPath::ReplacePath(imp_file_name, pImpPath, 1);
	SFile     imp_file(pImpPath, SFile::mRead); // PathRpt-->imp_file_name
	PPObjGoods::ReadConfig(&goods_cfg);
	const  PPID def_goods_id = (goods_cfg.DefGoodsID && goods_obj.Fetch(goods_cfg.DefGoodsID, 0) > 0) ? goods_cfg.DefGoodsID : 0;
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
			ss.Z();
			ss.add(buf);
			ss.get(&(pos = 0), buf);     // Код транзакции (не используем)
			ss.get(&pos, buf);           // Дата транзакции
			strtodate(buf.Strip(), DATF_DMY, &dtm.d);
			ss.get(&pos, buf);           // Время транзакции
			strtotime(buf.Strip(), TIMF_HMS, &dtm.t);
			ss.get(&pos, buf);           // Тип транзакции (операции)
			op_type = buf.ToLong();
			if(op_type == FRONTOL_OPTYPE_CHKCLOSED) {
				ss.get(&pos, buf);       // Номер ККМ
				cash_no = buf.ToLong();
				if(LogNumList.lsearch(cash_no)) {
					int    hour, min;
					long   chk_type;
					PPID   id = 0;
					LDATETIME dttm2;
					ss.get(&pos, buf);
					chk_no = buf.ToLong();
					chk_no = ModifDup(cash_no, chk_no); // Номер чека
					decodetime(&hour, &min, 0, 0, &dtm.t);
					dtm.t = encodetime(hour, min, (int)(chk_no % 60), 0);
					dttm2 = dtm;
					ss.get(&pos, buf);       // #07 пропускаем
					ss.get(&pos, card_code); // #08 Код дисконтной карты
					for(field_no = 8; field_no < 13 && ss.get(&pos, buf); field_no++); // #9-12 пропускаем
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
			long   op_type = 0;
			long   cash_no = 0;
			long   chk_no = 0;
			const  long cur_zrep_n = (cur_zrep_list_pos < ZRepList.getCount()) ? ZRepList.at(cur_zrep_list_pos).ZRepN : 0;
			ss.Z().add(buf);
			pos = 0;
			//   № транзакции, дата транзакции, время транзакции - пропускаем, выбираем тип транзакции (операции)
			for(field_no = 0; field_no < 4 && ss.get(&pos, buf); field_no++);
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
					ss.get(&pos, buf);   // #09 Номер смены
					fld_buf_alt[0] = buf; // alt[0] // @v10.8.3 
					// @v10.8.3 nsmena = buf.ToLong();
					{
						// @v10.8.3 {
						ss.get(&pos, buf);   // #10
						ss.get(&pos, buf);   // #11
						ss.get(&pos, buf);   // #12
						ss.get(&pos, buf);   // #13 // Поля 10-13 пропускаем
						ss.get(&pos, buf);   // #14 // Номер смены в новом варианте протокола frontol (alt[1])
						fld_buf_alt[1] = buf;
						if(fld_buf_alt[0].NotEmptyS())
							nsmena = fld_buf_alt[0].ToLong();
						else if(fld_buf_alt[1].NotEmptyS())
							nsmena = fld_buf_alt[1].ToLong();
						else
							nsmena = 0;
						// } @v10.8.3
					}
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
				double price = 0.0;
				const  int is_free_price = oneof2(op_type, FRONTOL_OPTYPE_CHKLINEFREE, FRONTOL_OPTYPE_STORNOFREE);
				barcode.Z();
				ss.get(&pos, buf);          // #08 ID товара   
				goods_id = buf.ToLong();    
				ss.get(&pos, buf);          // #09 Коды значений разрезов
				ss.get(&pos, buf);          // #10 Цена
				price = buf.ToReal();
				ss.get(&pos, buf);
				const double src_qtty = buf.ToReal();        // #11 Количество
				ss.get(&pos, buf.Z());      // #12 Сумма товара + сумма округления //
				ss.get(&pos, buf.Z());      // #13 Операция //
				ss.get(&pos, buf.Z());      // #14 Номер смены
				ss.get(&pos, buf);          // #15 Цена  со скидками
				const double src_dscnt_price = buf.ToReal(); 
				ss.get(&pos, buf);          // #16 Сумма со скидками
				const double src_dscnt = buf.ToReal();       
				ss.get(&pos, buf.Z());      // #17
				ss.get(&pos, buf.Z());      // #18
				ss.get(&pos, barcode.Z());  // #19 barcode
				ss.get(&pos, buf.Z());      // #20 
				ss.get(&pos, buf.Z());      // #21 Номер отдела
				const int div = buf.ToLong();
				if(is_free_price)
					goods_id = def_goods_id;
				else {
					Goods2Tbl::Rec goods_rec;
					// @v10.8.5 {
					if(!UseAltImport) {
						if(goods_obj.Search(goods_id, &goods_rec) > 0) {
							;
						}
						else {
							if(barcode.NotEmptyS() && goods_obj.SearchByBarcode(barcode, 0, &goods_rec, 0) > 0) {
								goods_id = goods_rec.ID;
							}
							else {
								SysJournal * p_sj = DS.GetTLA().P_SysJ;
								Goods2Tbl::Rec ex_goods_rec;
								PPID   pretend_id = 0;
								do {
									if(goods_obj.Fetch(goods_id, &ex_goods_rec) > 0)
										pretend_id = ex_goods_rec.ID;
								} while(!pretend_id && p_sj && p_sj->GetLastObjUnifyEvent(PPOBJ_GOODS, goods_id, &goods_id, 0) > 0);
								if(pretend_id)
									goods_id = pretend_id;
							}
						}
					}
					// } @v10.8.5
					else { // UseAltImport
						buf.Divide('|', arcode, goods_name);
						if(!goods_name.NotEmptyS())
							goods_name.Z().CatEq("ID", goods_id);
						else
							goods_name.Transf(CTRANSF_OUTER_TO_INNER);
						if(goods_obj.P_Tbl->SearchByArCode(0, arcode, 0, &goods_rec) > 0)
							goods_id = goods_rec.ID;
						else {
							buf = "ATOL";
							if(!grp_id && goods_obj.P_Tbl->SearchByName(PPGDSK_GROUP, buf, &grp_id, &gds_pack.Rec) <= 0) {
								gds_pack.Z();
								gds_pack.Rec.Kind = PPGDSK_GROUP;
								buf.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
								THROW(goods_obj.PutPacket(&grp_id, &gds_pack, 0));
							}
							gds_pack.Z();
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
				}
				THROW(r = SearchTempCheckByCode(cash_no, chk_no, cur_zrep_n));
				if(r > 0) {
					double line_amount;
					PPID   chk_id = P_TmpCcTbl->data.ID;
					if(!closed_check_list.Has(static_cast<ulong>(chk_id))) {
						double qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(src_qtty) : fabs(src_qtty);
						if(op_type == FRONTOL_OPTYPE_STORNO) {
							TempCCheckLineTbl::Key2 k2;
							k2.CheckID = chk_id;
							BExtQuery cclq(P_TmpCclTbl, 2);
							cclq.selectAll().where(P_TmpCclTbl->CheckID == chk_id);
							qtty = -qtty;
							for(cclq.initIteration(false, &k2, spGe); cclq.nextIteration() > 0;) {
								const TempCCheckLineTbl::Rec & r_tccl_rec = P_TmpCclTbl->data;
								if(r_tccl_rec.GoodsID == goods_id && feqeps(fabs(r_tccl_rec.Quantity), fabs(qtty), 1E-7)) {
									double storno_amount = (intmnytodbl(r_tccl_rec.Price) - r_tccl_rec.Dscnt) * r_tccl_rec.Quantity;
									double storno_discount = r_tccl_rec.Dscnt * r_tccl_rec.Quantity;
									THROW(AddTempCheckAmounts(chk_id, -storno_amount, -storno_discount));
									THROW_DB(P_TmpCclTbl->deleteRec());
									break;
								}
							}
						}
						else {
							SetupTempCcLineRec(0, chk_id, chk_no, P_TmpCcTbl->data.Dt, div, goods_id);
							const double ln_price = is_free_price ? src_dscnt_price : price;
							const double ln_discount = is_free_price ? 0.0 : (price - src_dscnt_price);
							// @v10.7.3 SetTempCcLineValues(0, qtty, ln_price, ln_discount, 0/*pLnExtStrings*/);
							// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
							THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, qtty, ln_price, ln_discount, 0/*pLnExtStrings*/)); // @v10.7.3
							{
								double dscnt = 0.0;
								if(is_free_price)
									line_amount = R2(qtty * src_dscnt_price);
								else {
									line_amount = R2(qtty * src_dscnt_price);
									dscnt       = R2(qtty * (price - src_dscnt_price)); // @v10.8.8 @fix (qtty * price - dscnt)-->(qtty * (price - dscnt))
								}
								THROW(AddTempCheckAmounts(chk_id, line_amount, dscnt));
							}
						}
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
				/*
					0 – оплата наличными;
					1 – оплата банковской картой;
					3 – оплата внутренней предоплатой;
					6 – оплата внутренней подарочной картой;
					7 – пользовательская;
					8 – оплата внешней подарочной картой
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
				cc_payment.Z();
			}
			PPWaitPercent(cntr.Increment(), wait_msg);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ACS_FRONTOL::QueryFile(uint setNo, const char * pImpPath)
{
	int    ok = 1, notify_timeout = NZOR(ImpExpTimeout, 5000);
	const  int is_xpos = IsXPos(Acn); // @v10.8.2
	SString imp_path(pImpPath);
	SString exp_path;
	SString path_rpt;
	SString path_flag;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	SETIFZ(last_date, plusdate(getcurdate_(), 2)); // @v10.8.10 LConfig.OperDate-->getcurdate_()
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
			path_rpt = PathRpt;
			path_flag = PathFlag;
			SFsPath::ReplacePath(path_rpt,  imp_path, 1);
			SFsPath::ReplacePath(path_flag, exp_path, 1);
			THROW_PP(ok = WaitForExists(path_flag, 1, notify_timeout), PPERR_ATOL_IMPCHECKS);
			if(ok > 0) {
				// @v10.8.2 int     y, m, d;
				// @v10.8.2 SString date_mask = "%02d.%02d.%04d";
				SString tmp_buf;
				SFile::Remove(path_rpt);
				SString tmp_name = path_flag;
				SFsPath::ReplaceExt(tmp_name, "tmp", 1);
				SFile  query_file(tmp_name, SFile::mWrite);
				SString buf(is_xpos ? "$$$TRANSACTIONSBYDATERANGE" : "$$$TRANSACTIONSBYDATETIMERANGE"); // @v10.8.2 (is_xpos ? "$$$TRANSACTIONSBYDATERANGE")
				query_file.WriteLine(buf.CR());
				// @v10.8.2 decodedate(&d, &m, &y, &first_date);
				// @v10.8.2 buf.Z().Printf(date_mask, d, m, y).Semicol();
				buf.Z().Cat(first_date, DATF_GERMANCENT).Semicol(); // @v10.8.2
				// @v10.8.2 decodedate(&d, &m, &y, &last_date);
				// @v10.8.2 buf.Cat(tmp_buf.Printf(date_mask, d, m, y)).CR(); 
				buf.Cat(last_date, DATF_GERMANCENT).CR(); // @v10.8.2
				query_file.WriteLine(buf);
				query_file.Close();
				//
				// Задержка для Windows Vist (and above) чтобы убедиться что файл на сетевом диске виден
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
				if(ok > 0) {
					if(ImportDelay >= 0)
						SDelay((ImportDelay > 0) ? ImportDelay : 1000);
					if(ImportedFiles.Len())
						ImportedFiles.Semicol();
					imp_path.SetLastSlash().Cat(PathRpt);
					ImportedFiles.Cat(imp_path);
				}
			}
		}
		else {
			// log message
		}
	}
	CATCHZOK
	return ok;
}

int ACS_FRONTOL::ImportSession(int)
{
	int    ok = 1;
	THROW(CreateTables());
	{
		SString path;
		StringSet ss(';', ImportedFiles);
		for(uint i = 0; ss.get(&i, path.Z());) {
			ZRepList.freeAll();
			if(fileExists(path)) {
				THROW(GetZRepList(path, &ZRepList));
				THROW(ConvertWareList(path));
			}
		}
	}
	CATCHZOK
	return ok;
}

int ACS_FRONTOL::FinishImportSession(PPIDArray * pSessList)
{
	//
	// Удалим файлы импорта.
	//
	StringSet ss(';', ImportedFiles);
	SString path;
	for(uint i = 0; ss.get(&i, path);)
		SFile::Remove(path);
	return pSessList->addUnique(&SessAry);
}
