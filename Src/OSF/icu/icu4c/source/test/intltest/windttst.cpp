// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 ********************************************************************************
 *   Copyright (C) 2005-2016, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 ********************************************************************************
 *
 * File WINDTTST.CPP
 *
 ********************************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop

#if U_PLATFORM_USES_ONLY_WIN32_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/testlog.h"
#include "unicode/utmscale.h"
#include "windtfmt.h"
#include "winutil.h"
#include "windttst.h"
#include "dtfmttst.h"
#include "locmap.h"
#include "wintzimpl.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOUSER
#define NOSERVICE
#define NOIME
#define NOMCX

static const char * getCalendarType(int32_t type)
{
	switch(type) {
		case 1:
		case 2: return "@calendar=gregorian";
		case 3: return "@calendar=japanese";
		case 6: return "@calendar=islamic";
		case 7: return "@calendar=buddhist";
		case 8: return "@calendar=hebrew";
		default: return "";
	}
}

void Win32DateTimeTest::testLocales(DateFormatTest * log)
{
	SYSTEMTIME winNow;
	UDate icuNow = 0;
	SYSTEMTIME st;
	FILETIME ft;
	UnicodeString zoneID;
	const TimeZone * tz = TimeZone::createDefault();
	TIME_ZONE_INFORMATION tzi;

	tz->getID(zoneID);
	if(!uprv_getWindowsTimeZoneInfo(&tzi, zoneID.getBuffer(), zoneID.length())) {
		bool found = FALSE;
		int32_t ec = TimeZone::countEquivalentIDs(zoneID);

		for(int z = 0; z < ec; z += 1) {
			UnicodeString equiv = TimeZone::getEquivalentID(zoneID, z);

			found = uprv_getWindowsTimeZoneInfo(&tzi, equiv.getBuffer(), equiv.length());
			if(found) {
				break;
			}
		}

		if(!found) {
			GetTimeZoneInformation(&tzi);
		}
	}

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	SystemTimeToTzSpecificLocalTime(&tzi, &st, &winNow);

	int64_t wftNow = ((int64_t)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
	UErrorCode status = U_ZERO_ERROR;

	int64_t udtsNow = utmscale_fromInt64(wftNow, UDTS_WINDOWS_FILE_TIME, &status);

	icuNow = (UDate)utmscale_toInt64(udtsNow, UDTS_ICU4C_TIME, &status);

	int32_t lcidCount = 0;
	Win32Utilities::LCIDRecord * lcidRecords = Win32Utilities::getLocales(lcidCount);

	for(int i = 0; i < lcidCount; i += 1) {
		UErrorCode status = U_ZERO_ERROR;
		WCHAR longDateFormat[81], longTimeFormat[81], wdBuffer[256], wtBuffer[256];
		DWORD value = 0;
		int32_t calType = 0;

		// NULL localeID means ICU didn't recognize this locale
		if(lcidRecords[i].localeID == NULL) {
			continue;
		}

		// Some locales have had their names change over various OS releases; skip them in the test for now.
		int32_t failingLocaleLCIDs[] = {
			0x040a, /* es-ES_tradnl;es-ES-u-co-trad; */
			0x048c, /* fa-AF;prs-AF;prs-Arab-AF; */
			0x046b, /* qu-BO;quz-BO;quz-Latn-BO; */
			0x086b, /* qu-EC;quz-EC;quz-Latn-EC; */
			0x0c6b, /* qu-PE;quz-PE;quz-Latn-PE; */
			0x0492 /* ckb-IQ;ku-Arab-IQ; */
		};
		bool skip =
		    (std::find(std::begin(failingLocaleLCIDs), std::end(failingLocaleLCIDs),
		    lcidRecords[i].lcid) != std::end(failingLocaleLCIDs));
		if(skip && log->logKnownIssue("13119", "Windows '@compat=host' fails on down-level versions of the OS")) {
			log->logln("ticket:13119 - Skipping LCID = 0x%04x", lcidRecords[i].lcid);
			continue;
		}

		GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_SLONGDATE,   longDateFormat, 81);
		GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_STIMEFORMAT, longTimeFormat, 81);
		GetLocaleInfoW(lcidRecords[i].lcid, LOCALE_RETURN_NUMBER|LOCALE_ICALENDARTYPE, (LPWSTR)&value, sizeof(value)/sizeof(WCHAR));
		calType = value;
		char localeID[64];
		strcpy(localeID, lcidRecords[i].localeID);
		uprv_strcat(localeID, getCalendarType(calType));

		UnicodeString ubBuffer, udBuffer, utBuffer;
		Locale ulocale(localeID);
		int32_t wdLength, wtLength;

		wdLength = GetDateFormatW(lcidRecords[i].lcid, DATE_LONGDATE, &winNow, NULL, wdBuffer, SIZEOFARRAYi(wdBuffer));
		wtLength = GetTimeFormatW(lcidRecords[i].lcid, 0, &winNow, NULL, wtBuffer, SIZEOFARRAYi(wtBuffer));
		if(sstrchr(localeID, '@') > 0) {
			uprv_strcat(localeID, ";");
		}
		else {
			uprv_strcat(localeID, "@");
		}

		uprv_strcat(localeID, "compat=host");

		Locale wlocale(localeID);
		DateFormat * wbf = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, wlocale);
		DateFormat * wdf = DateFormat::createDateInstance(DateFormat::kFull, wlocale);
		DateFormat * wtf = DateFormat::createTimeInstance(DateFormat::kFull, wlocale);

		wbf->format(icuNow, ubBuffer);
		wdf->format(icuNow, udBuffer);
		wtf->format(icuNow, utBuffer);

		if(ubBuffer.indexOf((const char16_t*)wdBuffer, wdLength - 1, 0) < 0) {
			UnicodeString baseName(wlocale.getBaseName());
			UnicodeString expected((const char16_t*)wdBuffer);

			log->errln("DateTime format error for locale " + baseName + ": expected date \"" + expected +
			    "\" got \"" + ubBuffer + "\"");
		}

		if(ubBuffer.indexOf((const char16_t*)wtBuffer, wtLength - 1, 0) < 0) {
			UnicodeString baseName(wlocale.getBaseName());
			UnicodeString expected((const char16_t*)wtBuffer);

			log->errln("DateTime format error for locale " + baseName + ": expected time \"" + expected +
			    "\" got \"" + ubBuffer + "\"");
		}

		if(udBuffer.compare((const char16_t*)wdBuffer) != 0) {
			UnicodeString baseName(wlocale.getBaseName());
			UnicodeString expected((const char16_t*)wdBuffer);

			log->errln("Date format error for locale " + baseName + ": expected \"" + expected +
			    "\" got \"" + udBuffer + "\"");
		}

		if(utBuffer.compare((const char16_t*)wtBuffer) != 0) {
			UnicodeString baseName(wlocale.getBaseName());
			UnicodeString expected((const char16_t*)wtBuffer);

			log->errln("Time format error for locale " + baseName + ": expected \"" + expected +
			    "\" got \"" + utBuffer + "\"");
		}
		delete wbf;
		delete wdf;
		delete wtf;
	}

	Win32Utilities::freeLocales(lcidRecords);
	delete tz;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_PLATFORM_USES_ONLY_WIN32_API */
