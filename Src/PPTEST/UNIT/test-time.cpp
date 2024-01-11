// TEST-TIME.CPP
// Copyright (c) A.Sobolev 2023, 2024
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
		SLCHECK_EQ(test_val, cvt_val);
		//
		{
			SUniTime_Internal uti;
			__time64_t tt = test_val.GetTimeT();
			const struct tm * p_tm = _gmtime64(&tt);
			__EpochTimeToTimeFields(tt, &uti);
			SLCHECK_EQ((long)p_tm->tm_year, (long)(uti.Y-1900));
			SLCHECK_EQ((long)p_tm->tm_mon+1, (long)uti.M);
			SLCHECK_EQ((long)p_tm->tm_mday, (long)uti.D);
			SLCHECK_EQ((long)p_tm->tm_hour, (long)uti.Hr);
			SLCHECK_EQ((long)p_tm->tm_min, (long)uti.Mn);
			SLCHECK_EQ((long)p_tm->tm_sec, (long)uti.Sc);
			SLCHECK_EQ((long)p_tm->tm_wday, (long)uti.Weekday);
			uint64 epoch_tm = 0;
			__TimeFieldsToEpochTime(&uti, &epoch_tm);
			SLCHECK_EQ(epoch_tm, (uint64)tt);
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
			SLCHECK_EQ(test_val, cvt_val);
			//
			test_val = test_val.getactual(rel);
			strtodate(r_item.Out, DATF_DMY, &pattern_val);
			SLCHECK_EQ(test_val, pattern_val);
		}
	}
	{
		{
			static const SUniDate_Internal _ymd_list[] = {
				SUniDate_Internal(2008, 12, 30),
				SUniDate_Internal(2008, 12, 31),
				SUniDate_Internal(1600, 12, 30),
				SUniDate_Internal(1600, 12, 31),
				SUniDate_Internal(2001, 1, 1),
				SUniDate_Internal(2004, 2, 29),
				SUniDate_Internal(1991, 3, 1),
				SUniDate_Internal(1879, 12, 31),
				SUniDate_Internal(1, 1, 1),
				SUniDate_Internal(1582, 10, 15),
			};
			for(uint i = 0; i < SIZEOFARRAY(_ymd_list); i++) {
				const int dc = _ymd_list[i].GetDaysSinceChristmas();
				SUniDate_Internal ymd2;
				ymd2.SetDaysSinceChristmas(dc);
				SLCHECK_NZ(ymd2 == _ymd_list[i]);
			}
		}

		struct t_YMD_pair {
			SUniDate_Internal D1;
			SUniDate_Internal D2;
			int     Diff;
		};

		static const t_YMD_pair _ymd_pair_list[] = {
			{ SUniDate_Internal(1, 3, 1), SUniDate_Internal(1970, 3, 1), /*719527*/719162 }, // /*719527*/719162 days were between March 1, 1 BC and March 1, 1970,
			{ SUniDate_Internal(1996, 12, 31), SUniDate_Internal(1997, 1, 1), 1 },
			{ SUniDate_Internal(2000, 2, 28), SUniDate_Internal(2000, 3, 1), 2 },
		};
		{
			for(uint i = 0; i < SIZEOFARRAY(_ymd_pair_list); i++) {
				int dc1 = _ymd_pair_list[i].D1.GetDaysSinceChristmas();
				int dc2 = _ymd_pair_list[i].D2.GetDaysSinceChristmas();
				SLCHECK_EQ((uint)(dc2-dc1), (uint)_ymd_pair_list[i].Diff);
				SUniDate_Internal ymd2;
				ymd2.SetDaysSinceChristmas(dc1);
				SLCHECK_NZ(ymd2 == _ymd_pair_list[i].D1);
				ymd2.SetDaysSinceChristmas(dc2);
				SLCHECK_NZ(ymd2 == _ymd_pair_list[i].D2);
			}
		}
	}
	{
		//const  LDATE rel = encodedate(7, 11, 2007);
		const  LDATE rel = encodedate(15, 10, 1582);
		for(i = 1; i < 200000; i++) {
			LDATE test = plusdate(rel, i);
			SLCHECK_EQ((long)diffdate(test, rel), (long)i);
			SLCHECK_EQ((long)diffdate(rel, test), -(long)i);
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
				SLCHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLCHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASEGEN;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLCHECK_EQ(mon_buf, txt_mon);
			}
			{
				long fmt = MONF_CASENOM|MONF_OEM;
				getMonthText(i, fmt, txt_mon);
				SGetMonthText(i, fmt, mon_buf);
				SLCHECK_EQ(mon_buf, txt_mon);
			}
#endif // } 0 @v10.4.5
		}
	}
	{
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 1), 0L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 2), 0L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 3), 0L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 11, 27), 4), 0L);

		SLCHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(10, 1, 10, 17), 1), 2L);
		SLCHECK_EQ(DiffTime(encodetime(12, 7, 10, 17), encodetime(12, 1, 10, 17), 2), 6L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 21, 17), encodetime(12, 1, 10, 17), 3), 11L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 10, 17), 4), 100L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 10, 17), encodetime(12, 1, 10, 27), 4), -100L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 11, 27), encodetime(12, 1, 10, 27), 4), 1000L);
		SLCHECK_EQ(DiffTime(encodetime(12, 1, 10, 27), encodetime(12, 1, 11, 27), 4), -1000L);
		SLCHECK_EQ(DiffTime(encodetime(12, 2, 11,  7), encodetime(12, 1, 10, 17), 4), 60900L);
	}
	{
		SString temp_buf;
		LDATETIME dtm;
		const long datf = DATF_DMY|DATF_CENTURY;
		const long timf = TIMF_HMS;
		strtodatetime("31/12/2008 01:17:02", &dtm, datf, timf);
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(0), datf, timf), "31/12/2008 01:17:02");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(1), datf, timf), "31/12/2008 01:17:03");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(60), datf, timf), "31/12/2008 01:18:03");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(3600), datf, timf), "31/12/2008 02:18:03");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(24*3600), datf, timf), "01/01/2009 02:18:03");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-1), datf, timf), "01/01/2009 02:18:02");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-60), datf, timf), "01/01/2009 02:17:02");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-3600), datf, timf), "01/01/2009 01:17:02");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(-24*3600), datf, timf), "31/12/2008 01:17:02");
		SLCHECK_EQ(temp_buf.Z().Cat(dtm.addsec(0), datf, timf), "31/12/2008 01:17:02");
	}
	{
		//
		// SDN date
		//
		struct SdnCalendarCvtCase {
			uint   Sdn;
			SUniDate_Internal Dt;
		};
		{
			static const SdnCalendarCvtCase TestSet_Julian[] = {
				{ 2298884,  { 1582, 1, 1 } },
				{ 2299161,  { 1582, 10, 5, } },
				{ 2440601,  { 1970, 1, 1 } },
				{ 2816443,  { 2999 , 1, 1 } },
				{ 1,		  { -4713, 1, 2 } },
				{ 2298874,	  { 1581, 12, 22 } },
				{ 2299151,	  { 1582, 9, 25 } },
				{ 2440588,	  { 1969, 12, 19 } },
				{ 2816423,	  { 2998, 12, 12 } },
				{ 0,          { -4713, 1, 1 } },
				{ 0,        { 0, 0, 0 } },  
			};
			SUniDate_Internal dt;
			SLCHECK_Z(dt.SetSdnJulian(0));
			for(uint i = 0; i < SIZEOFARRAY(TestSet_Julian); i++) {
				//SLCHECK_Z(SUniDate_Internal::SdnToJulian(0, &y, &m, &d));
				if(TestSet_Julian[i].Sdn) {
					int y;
					int m;
					int d;
					SLCHECK_NZ(SUniDate_Internal::SdnToJulian(TestSet_Julian[i].Sdn, &y, &m, &d));
					SLCHECK_NZ(y = TestSet_Julian[i].Dt.Y && m == TestSet_Julian[i].Dt.M && d == TestSet_Julian[i].Dt.D);
					SLCHECK_NZ(dt.SetSdnJulian(TestSet_Julian[i].Sdn));
					SLCHECK_NZ(dt == TestSet_Julian[i].Dt);
				}
				uint sdn = SUniDate_Internal::JulianToSdn(TestSet_Julian[i].Dt.Y, TestSet_Julian[i].Dt.M, TestSet_Julian[i].Dt.D);
				SLCHECK_EQ(sdn, TestSet_Julian[i].Sdn);
			}
		}
		{
			static const SdnCalendarCvtCase TestSet_Gregorian[] = {
				{ 2298874,	  { 1582, 1, 1 } },
				{ 2299151,	  { 1582, 10, 5 } },
				{ 2440588,	  { 1970, 1, 1 } },
				{ 2816423,	  { 2999, 1, 1 } },
				{ 1,		  { -4714, 11,25 } },
				{ 0,          { 0, 0,    0 } },
				{ 0,		  { -4714, 1, 1 } },
				{ 0,		  { -4714, 11, 24 } },
			};
			for(uint i = 0; i < SIZEOFARRAY(TestSet_Gregorian); i++) {
				//int y;
				//int m;
				//int d;
				SUniDate_Internal dt;
				SLCHECK_Z(dt.SetSdnGregorian(0));
				//SLCHECK_Z(SUniDate_Internal::SdnToGregorian(0, &y, &m, &d));
				if(TestSet_Gregorian[i].Sdn) {
					//SLCHECK_NZ(SUniDate_Internal::SdnToGregorian(TestSet_Gregorian[i].Sdn, &y, &m, &d));
					//SLCHECK_NZ(y = TestSet_Gregorian[i].Dt.Y && m == TestSet_Gregorian[i].Dt.M && d == TestSet_Gregorian[i].Dt.D);
					SLCHECK_NZ(dt.SetSdnGregorian(TestSet_Gregorian[i].Sdn));
					SLCHECK_NZ(dt == TestSet_Gregorian[i].Dt);
				}
				uint sdn = SUniDate_Internal::GregorianToSdn(TestSet_Gregorian[i].Dt.Y, TestSet_Gregorian[i].Dt.M, TestSet_Gregorian[i].Dt.D);
				SLCHECK_EQ(sdn, TestSet_Gregorian[i].Sdn);
			}
		}
		{
			struct SdnCalendarDowCase {
				long    Sdn;
				int     Dow[8];
			};

			static const SdnCalendarDowCase TestSet_SdnDayOfWeek[] = {
				{ 2440588, { 4, 5, 6, 0, 1, 2, 3, 4 } },
				{ 2452162, { 0, 1, 2, 3, 4, 5, 6, 0 } }, 
				{ 2453926, { 0, 1, 2, 3, 4, 5, 6, 0 } },
				{ -1000, { 2, 3, 4, 5, 6, 0, 1, 2 } }
			};
			for(uint i = 0; i < SIZEOFARRAY(TestSet_SdnDayOfWeek); i++) {
				for(uint j = 0; j < SIZEOFARRAY(TestSet_SdnDayOfWeek[i].Dow); j++) {
					SLCHECK_EQ(SUniDate_Internal::SdnDayOfWeek(TestSet_SdnDayOfWeek[i].Sdn + j), TestSet_SdnDayOfWeek[i].Dow[j]);
				}
			}
		}
	}
	{
		SString temp_file_path;
		(temp_file_path = GetSuiteEntry()->OutPath).SetLastSlash().Cat("temporary-file.tmp");
		SFile f_temp(temp_file_path, SFile::mWrite);
		if(f_temp.IsValid()) {
			f_temp.WriteLine("any text\n");
			f_temp.Close();
			const LDATETIME now_dtm = getcurdatetime_();
			//
			SFile::Stat fs;
			if(SFile::GetStat(temp_file_path, 0, &fs, 0)) {
				LDATETIME dtm;
				//int tz = gettimezone(); 

				TIME_ZONE_INFORMATION tz;
				::GetTimeZoneInformation(&tz);

				dtm.SetNs100(fs.ModTm_);
				long tm_diff_sec = labs(diffdatetimesec(dtm, now_dtm));
				SLCHECK_NZ((tm_diff_sec + (tz.Bias * 60)) <= 1);
			}
		}
	}
	return CurrentStatus;
}
