// TEST-TIME.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование реализаций времени и даты
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(LDATE)
{
	struct Pair {
		char   In[64];
		char   Out[64];
	} pair_list[] = {
		{"1/1/2008",          "1/1/2008"},
		{"1 . 1 - 2008",      "1/1/2008"},
		{" 20-9",             "20/9/getcurdate_.year()"},                    // *[2]
		{"31.12.201",         "31/12/2001"},
		{"12",                "12/getcurdate_.month()/getcurdate_.year()"},  // *[4]
		{"@",                 "7/11/2007"},
		{"@-10",              "28/10/2007"},
		{"@+365",             "6/11/2008"},
		{"11 .9.@",           "11/9/2007"},
		{"11. @-1",           "11/10/getcurdate_().year()"}, // *[9] месяц берется относительно даты rel, год - из текущей системной даты
		{"@-1.@-1.@-10",      "6/10/1997"},
		{"@-3.@+2.@+2",       "4/1/2010"},
		{"^10",               "1/10/2007"},
		{"^4",                "1/11/2007"}
	};

	struct PairDateTime {
		char   In[128];
		char   Out[128];
	} pair_list_dtm[] = {
		{"Sun, 06 Nov 1994 08:49:37 GMT",   "6/11/1994 8:49:37"},
		{"Sunday, 06-Nov-94 08:49:37 GMT",  "6/11/1994 8:49:37"},
		{"Sun Nov  6 08:49:37 1994",        "6/11/1994 8:49:37"},
		{"Nov  6 08:49:37 1994",            "6/11/1994 8:49:37"},
		{"GMT 08:49:37 06-Nov-94",          "6/11/1994 8:49:37"},
		{"Sun Nov 6 94",                    "6/11/1994"},
		{"Sun/Nov/6/94/GMT",                "6/11/1994"},
		{"Sun, 06 Nov 1994 08:49:37 CET",   "6/11/1994 8:49:37"},
		{"Sun, 12 Sep 2004 15:05:58 -0700", "12/09/2004 15:05:58"},
		{"Tue, 30 Dec 2008 21:32:11 +0200", "30/12/2008 21:32:11"},
		{"Wed, 11 Dec 2008 01:17:02 +0200", "31/12/2008 01:17:02"}
	};
	uint i;
	_datefmt(20, 9, getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[2].Out);
	_datefmt(12, getcurdate_().month(), getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[4].Out);
	_datefmt(11, 10, getcurdate_().year(), DATF_DMY | DATF_CENTURY, pair_list[9].Out);
	char   cvt_buf[128];
	for(i = 0; i < SIZEOFARRAY(pair_list_dtm); i++) {
		const PairDateTime & r_item = pair_list_dtm[i];
		LDATETIME test_val, cvt_val;
		strtodatetime(r_item.In, &test_val, DATF_DMY, TIMF_HMS);
		//
		// Проверяем конвертацию из даты в строку
		//
		datetimefmt(test_val, DATF_DMY, TIMF_HMS, cvt_buf, sizeof(cvt_buf));
		strtodatetime(cvt_buf, &cvt_val, DATF_DMY, TIMF_HMS);
		SLTEST_CHECK_EQ(test_val, cvt_val);
		//
		{
			SUniTime_Inner uti;
			__time64_t tt = test_val.GetTimeT();
			const struct tm * p_tm = _gmtime64(&tt);
			__EpochTimeToTimeFields(tt, &uti);
			SLTEST_CHECK_EQ((long)p_tm->tm_year, (long)(uti.Y-1900));
			SLTEST_CHECK_EQ((long)p_tm->tm_mon+1, (long)uti.M);
			SLTEST_CHECK_EQ((long)p_tm->tm_mday, (long)uti.D);
			SLTEST_CHECK_EQ((long)p_tm->tm_hour, (long)uti.Hr);
			SLTEST_CHECK_EQ((long)p_tm->tm_min, (long)uti.Mn);
			SLTEST_CHECK_EQ((long)p_tm->tm_sec, (long)uti.Sc);
			SLTEST_CHECK_EQ((long)p_tm->tm_wday, (long)uti.Weekday);
			uint64 epoch_tm = 0;
			__TimeFieldsToEpochTime(&uti, &epoch_tm);
			SLTEST_CHECK_EQ(epoch_tm, (uint64)tt);
		}
	}
	{
		const  LDATE rel = encodedate(7, 11, 2007);
		for(i = 0; i < SIZEOFARRAY(pair_list); i++) {
			const Pair & r_item = pair_list[i];
			LDATE test_val, pattern_val, cvt_val;
			strtodate(r_item.In,  DATF_DMY, &test_val);
			//
			// Проверяем конвертацию из даты в строку
			//
			datefmt(&test_val, DATF_DMY, cvt_buf);
			strtodate(cvt_buf,  DATF_DMY, &cvt_val);
			SLTEST_CHECK_EQ(test_val, cvt_val);
			//
			test_val = test_val.getactual(rel);
			strtodate(r_item.Out, DATF_DMY, &pattern_val);
			SLTEST_CHECK_EQ(test_val, pattern_val);
		}
	}
	{
		int y2, m2, d2;
		{
			int y = 2008, m = 12, d = 30;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2008, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1600, m = 12, d = 30;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1600, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2001, m = 1, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 2004, m = 2, d = 29;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1991, m = 3, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1879, m = 12, d = 31;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1, m = 1, d = 1;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			int y = 1582, m = 10, d = 15;
			int dc = DateToDaysSinceChristmas(y, m, d);
			DaysSinceChristmasToDate(dc, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)y);
			SLTEST_CHECK_EQ((long)m2, (long)m);
			SLTEST_CHECK_EQ((long)d2, (long)d);
		}
		{
			// /*719527*/719162 days were between March 1, 1 BC and March 1, 1970,
			int dc1 = DateToDaysSinceChristmas(1, 3, 1);
			int dc2 = DateToDaysSinceChristmas(1970, 3, 1);
			SLTEST_CHECK_EQ((dc2-dc1), /*719527*/719162);
			DaysSinceChristmasToDate(dc1, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1);
			SLTEST_CHECK_EQ((long)m2, (long)3);
			SLTEST_CHECK_EQ((long)d2, (long)1);
			DaysSinceChristmasToDate(dc2, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1970);
			SLTEST_CHECK_EQ((long)m2, (long)3);
			SLTEST_CHECK_EQ((long)d2, (long)1);
		}
		{
			int dc1 = DateToDaysSinceChristmas(1996, 12, 31);
			int dc2 = DateToDaysSinceChristmas(1997, 1, 1);
			SLTEST_CHECK_EQ((dc2-dc1), 1);
			DaysSinceChristmasToDate(dc1, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1996);
			SLTEST_CHECK_EQ((long)m2, (long)12);
			SLTEST_CHECK_EQ((long)d2, (long)31);
			DaysSinceChristmasToDate(dc2, &y2, &m2, &d2);
			SLTEST_CHECK_EQ((long)y2, (long)1997);
			SLTEST_CHECK_EQ((long)m2, (long)1);
			SLTEST_CHECK_EQ((long)d2, (long)1);
		}
	}
	{
		//const  LDATE rel = encodedate(7, 11, 2007);
		const  LDATE rel = encodedate(15, 10, 1582);
		for(i = 1; i < 200000; i++) {
			LDATE test = plusdate(rel, i);
			SLTEST_CHECK_EQ((long)diffdate(test, rel), (long)i);
			SLTEST_CHECK_EQ((long)diffdate(rel, test), -(long)i);
		}
	}
	{
		SString mon_buf;
		for(i = 1; i <= 12; i++) {
			/*
				#define MONF_SHORT     0x0001 // Сокращенная форма
				#define MONF_CASENOM   0x0002 // Полная форма (именительный падеж)
				#define MONF_CASEGEN   0x0004 // Полная форма (родительный падеж)
				#define MONF_OEM       0x0080 // OEM-coding
			*/
#if 0 // @v10.4.5 {
			char  txt_mon[128];
			{
				long fmt = MONF_SHORT;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASEGEN;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM|MONF_OEM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLTEST_CHECK_EQ(mon_buf, txt_mon);
			}
#endif // } 0 @v10.4.5
		}
	}
	{
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 1), 0L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 2), 0L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 3), 0L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 4), 0L);

		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(10, 1, 10, 17), 1), 2L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 7, 10, 17), encodetime(12, 1, 10, 17), 2), 6L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 21, 17), encodetime(12, 1, 10, 17), 3), 11L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 10, 17), 4), 100L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(12, 1, 10, 27), 4), -100L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 10, 27), 4), 1000L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 11, 27), 4), -1000L);
		SLTEST_CHECK_EQ(DiffTime(encodetime(12, 2, 11,  7), encodetime(12, 1, 10, 17), 4), 60900L);
	}
	{
		SString temp_buf;
		LDATETIME dtm;
		const long datf = DATF_DMY|DATF_CENTURY;
		const long timf = TIMF_HMS;
		strtodatetime("31/12/2008 01:17:02", &dtm, datf, timf);
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(0), datf, timf), "31/12/2008 01:17:02");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(1), datf, timf), "31/12/2008 01:17:03");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(60), datf, timf), "31/12/2008 01:18:03");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(3600), datf, timf), "31/12/2008 02:18:03");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(24*3600), datf, timf), "01/01/2009 02:18:03");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-1), datf, timf), "01/01/2009 02:18:02");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-60), datf, timf), "01/01/2009 02:17:02");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-3600), datf, timf), "01/01/2009 01:17:02");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-24*3600), datf, timf), "31/12/2008 01:17:02");
		SLTEST_CHECK_EQ(temp_buf.Z().Cat(dtm.addsec(0), datf, timf), "31/12/2008 01:17:02");
	}
	return CurrentStatus;
}
