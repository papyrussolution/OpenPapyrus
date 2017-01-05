/* ------------------------------------------------------------------------ *
 *
 * oci_import.h - Part of OCILIB Project
 *
 * OCILIB : IS0 C Oracle Call Interface (OCI) Encapsulation
 *
 * Copyright (C) 2007 Vincent ROGIER <vince_rogier@yahoo.fr>
 *
 * URL : http://orclib.sourceforge.net
 *
 * LICENSE :
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ *
 * $Id: oci_import.h, v 1.5.0 10:24 04/01/2008 Vince $
 * ------------------------------------------------------------------------ */

#ifndef OCILIB_OCI_IMPORT_H_INCLUDED
#define OCILIB_OCI_IMPORT_H_INCLUDED

#ifdef OCI_IMPORT_LINKAGE

#include <oci.h>

#ifdef WIN32
#pragma comment(lib, "oci.lib")
#endif

#else

#include "oci_loader.h"
#include "oci_types.h"
#include "oci_def.h"
#include "oci_api.h"

#ifdef WIN32
#define OCI_DL                   MT("oci.dll")
#else
#define OCI_DL                   MT("libclntsh.so")
#endif

#ifdef WIN32
#pragma comment(lib, "ocilib.lib")
#endif

class Ocif {
public:
	static Init();

	static OCIENVCREATE                 OCIEnvCreate;
	static OCISERVERATTACH              OCIServerAttach;
	static OCISERVERDETACH              OCIServerDetach;
	static OCIHANDLEALLOC               OCIHandleAlloc;
	static OCIHANDLEFREE                OCIHandleFree;
	static OCIDESCRIPTORALLOC           OCIDescriptorAlloc;
	static OCIDESCRIPTORFREE            OCIDescriptorFree;
	static OCISESSIONBEGIN              OCISessionBegin;
	static OCISESSIONEND                OCISessionEnd;
	static OCIBINDBYPOS                 OCIBindByPos;
	static OCIBINDBYNAME                OCIBindByName;
	static OCIDEFINEBYPOS               OCIDefineByPos;
	static OCISTMTPREPARE               OCIStmtPrepare;
	static OCISTMTEXECUTE               OCIStmtExecute;
	static OCISTMTFETCH                 OCIStmtFetch;
	static OCISTMTGETPIECEINFO          OCIStmtGetPieceInfo;
	static OCISTMTSETPIECEINFO          OCIStmtSetPieceInfo;
	static OCIPARAMGET                  OCIParamGet;
	static OCIPARAMSET                  OCIParamSet;
	static OCITRANSSTART                OCITransStart;
	static OCITRANSDETACH               OCITransDetach;
	static OCITRANSPREPARE              OCITransPrepare;
	static OCITRANSFORGET               OCITransForget;
	static OCITRANSCOMMIT               OCITransCommit;
	static OCITRANSROLLBACK             OCITransRollback;
	static OCIERRORGET                  OCIErrorGet;
	static OCILOBCREATETEMPORARY        OCILobCreateTemporary;
	static OCILOBFREETEMPORARY          OCILobFreeTemporary;
	static OCILOBISTEMPORARY            OCILobIsTemporary;
	static OCILOBAPPEND                 OCILobAppend;
	static OCILOBCOPY                   OCILobCopy;
	static OCILOBGETLENGTH              OCILobGetLength;
	static OCILOBREAD                   OCILobRead;
	static OCILOBWRITE                  OCILobWrite;
	static OCILOBTRIM                   OCILobTrim;
	static OCILOBERASE                  OCILobErase;
	static OCILOBOPEN                   OCILobOpen;
	static OCILOBCLOSE                  OCILobClose;
	static OCILOBFILEOPEN               OCILobFileOpen;
	static OCILOBFILECLOSE              OCILobFileClose;
	static OCILOBFILECLOSEALL           OCILobFileCloseAll;
	static OCILOBFILEISOPEN             OCILobFileIsOpen;
	static OCILOBFILEEXISTS             OCILobFileExists;
	static OCILOBFIELGETNAME            OCILobFileGetName;
	static OCILOBFILESETNAME            OCILobFileSetName;
	static OCILOBLOADFROMFILE           OCILobLoadFromFile;
	static OCILOBWRITEAPPEND            OCILobWriteAppend;
	static OCILOBISEQUAL                OCILobIsEqual;
	static OCISERVERVERSION             OCIServerVersion;
	static OCIATTRGET                   OCIAttrGet;
	static OCIATTRSET                   OCIAttrSet;
	static OCIDATEASSIGN                OCIDateAssign;
	static OCIDATETOTEXT                OCIDateToText;
	static OCIDATEFROMTEXT              OCIDateFromText;
	static OCIDATECOMPARE               OCIDateCompare;
	static OCIDATEADDMONTHS             OCIDateAddMonths;
	static OCIDATEADDDAYS               OCIDateAddDays;
	static OCIDATELASTDAY               OCIDateLastDay;
	static OCIDATEDAYSBETWEEN           OCIDateDaysBetween;
	static OCIDATEZONETOZONE            OCIDateZoneToZone;
	static OCIDATENEXTDAY               OCIDateNextDay;
	static OCIDATECHECK                 OCIDateCheck;
	static OCIDATESYSDATE               OCIDateSysDate;
	static OCIDESCRIBEANY               OCIDescribeAny;
	static OCIINTERVALASSIGN            OCIIntervalAssign;
	static OCIINTERVALCHECK             OCIIntervalCheck;
	static OCIINTERVALCOMPARE           OCIIntervalCompare;
	static OCIINTERVALFROMTEXT          OCIIntervalFromText;
	static OCIINTERVALTOTEXT            OCIIntervalToText;
	static OCIINTERVALFROMTZ            OCIIntervalFromTZ;
	static OCIINTERVALGETDAYSECOND      OCIIntervalGetDaySecond;
	static OCIINTERVALGETYEARMONTH      OCIIntervalGetYearMonth;
	static OCIINTERVALSETDAYSECOND      OCIIntervalSetDaySecond;
	static OCIINTERVALSETYEARMONTH      OCIIntervalSetYearMonth;
	static OCIINTERVALSUBTRACT          OCIIntervalSubtract;
	static OCIINTERVALADD               OCIIntervalAdd;
	static OCIDATETIMEASSIGN            OCIDateTimeAssign;
	static OCIDATETIMECHECK             OCIDateTimeCheck;
	static OCIDATETIMECOMPARE           OCIDateTimeCompare;
	static OCIDATETIMECONSTRUCT         OCIDateTimeConstruct;
	static OCIDATETIMECONVERT           OCIDateTimeConvert;
	static OCIDATETIMEFROMARRAY         OCIDateTimeFromArray;
	static OCIDATETIMETOARRAY           OCIDateTimeToArray;
	static OCIDATETIMEFROMTEXT          OCIDateTimeFromText;
	static OCIDATETIMETOTEXT            OCIDateTimeToText;
	static OCIDATETIMEGETDATE           OCIDateTimeGetDate;
	static OCIDATETIMEGETTIME           OCIDateTimeGetTime;
	static OCIDATETIMEGETTIMEZONENAME   OCIDateTimeGetTimeZoneName;
	static OCIDATETIMEGETTIMEZONEOFFSET OCIDateTimeGetTimeZoneOffset;
	static OCIDATETIMEINTERVALADD       OCIDateTimeIntervalAdd;
	static OCIDATETIMEINTERVALSUB       OCIDateTimeIntervalSub;
	static OCIDATETIMESUBTRACT          OCIDateTimeSubtract;
	static OCIDATETIMESYSTIMESTAMP      OCIDateTimeSysTimeStamp;
	static OCIARRAYDESCRIPTORFREE       OCIArrayDescriptorFree;
	static OCICLIENTVERSION             OCIClientVersion;
};

OCIENVCREATE                 Ocif::OCIEnvCreate                 = 0;
OCISERVERATTACH              Ocif::OCIServerAttach              = 0;
OCISERVERDETACH              Ocif::OCIServerDetach              = 0;
OCIHANDLEALLOC               Ocif::OCIHandleAlloc               = 0;
OCIHANDLEFREE                Ocif::OCIHandleFree                = 0;
OCIDESCRIPTORALLOC           Ocif::OCIDescriptorAlloc           = 0;
OCIDESCRIPTORFREE            Ocif::OCIDescriptorFree            = 0;
OCISESSIONBEGIN              Ocif::OCISessionBegin              = 0;
OCISESSIONEND                Ocif::OCISessionEnd                = 0;
OCIBINDBYPOS                 Ocif::OCIBindByPos                 = 0;
OCIBINDBYNAME                Ocif::OCIBindByName                = 0;
OCIDEFINEBYPOS               Ocif::OCIDefineByPos               = 0;
OCISTMTPREPARE               Ocif::OCIStmtPrepare               = 0;
OCISTMTEXECUTE               Ocif::OCIStmtExecute               = 0;
OCISTMTFETCH                 Ocif::OCIStmtFetch                 = 0;
OCISTMTGETPIECEINFO          Ocif::OCIStmtGetPieceInfo          = 0;
OCISTMTSETPIECEINFO          Ocif::OCIStmtSetPieceInfo          = 0;
OCIPARAMGET                  Ocif::OCIParamGet                  = 0;
OCIPARAMSET                  Ocif::OCIParamSet                  = 0;
OCITRANSSTART                Ocif::OCITransStart                = 0;
OCITRANSDETACH               Ocif::OCITransDetach               = 0;
OCITRANSPREPARE              Ocif::OCITransPrepare              = 0;
OCITRANSFORGET               Ocif::OCITransForget               = 0;
OCITRANSCOMMIT               Ocif::OCITransCommit               = 0;
OCITRANSROLLBACK             Ocif::OCITransRollback             = 0;
OCIERRORGET                  Ocif::OCIErrorGet                  = 0;
OCILOBCREATETEMPORARY        Ocif::OCILobCreateTemporary        = 0;
OCILOBFREETEMPORARY          Ocif::OCILobFreeTemporary          = 0;
OCILOBISTEMPORARY            Ocif::OCILobIsTemporary            = 0;
OCILOBAPPEND                 Ocif::OCILobAppend                 = 0;
OCILOBCOPY                   Ocif::OCILobCopy                   = 0;
OCILOBGETLENGTH              Ocif::OCILobGetLength              = 0;
OCILOBREAD                   Ocif::OCILobRead                   = 0;
OCILOBWRITE                  Ocif::OCILobWrite                  = 0;
OCILOBTRIM                   Ocif::OCILobTrim                   = 0;
OCILOBERASE                  Ocif::OCILobErase                  = 0;
OCILOBOPEN                   Ocif::OCILobOpen                   = 0;
OCILOBCLOSE                  Ocif::OCILobClose                  = 0;
OCILOBFILEOPEN               Ocif::OCILobFileOpen               = 0;
OCILOBFILECLOSE              Ocif::OCILobFileClose              = 0;
OCILOBFILECLOSEALL           Ocif::OCILobFileCloseAll           = 0;
OCILOBFILEISOPEN             Ocif::OCILobFileIsOpen             = 0;
OCILOBFILEEXISTS             Ocif::OCILobFileExists             = 0;
OCILOBFIELGETNAME            Ocif::OCILobFileGetName            = 0;
OCILOBFILESETNAME            Ocif::OCILobFileSetName            = 0;
OCILOBLOADFROMFILE           Ocif::OCILobLoadFromFile           = 0;
OCILOBWRITEAPPEND            Ocif::OCILobWriteAppend            = 0;
OCILOBISEQUAL                Ocif::OCILobIsEqual                = 0;
OCISERVERVERSION             Ocif::OCIServerVersion             = 0;
OCIATTRGET                   Ocif::OCIAttrGet                   = 0;
OCIATTRSET                   Ocif::OCIAttrSet                   = 0;
OCIDATEASSIGN                Ocif::OCIDateAssign                = 0;
OCIDATETOTEXT                Ocif::OCIDateToText                = 0;
OCIDATEFROMTEXT              Ocif::OCIDateFromText              = 0;
OCIDATECOMPARE               Ocif::OCIDateCompare               = 0;
OCIDATEADDMONTHS             Ocif::OCIDateAddMonths             = 0;
OCIDATEADDDAYS               Ocif::OCIDateAddDays               = 0;
OCIDATELASTDAY               Ocif::OCIDateLastDay               = 0;
OCIDATEDAYSBETWEEN           Ocif::OCIDateDaysBetween           = 0;
OCIDATEZONETOZONE            Ocif::OCIDateZoneToZone            = 0;
OCIDATENEXTDAY               Ocif::OCIDateNextDay               = 0;
OCIDATECHECK                 Ocif::OCIDateCheck                 = 0;
OCIDATESYSDATE               Ocif::OCIDateSysDate               = 0;
OCIDESCRIBEANY               Ocif::OCIDescribeAny               = 0;
OCIINTERVALASSIGN            Ocif::OCIIntervalAssign            = 0;
OCIINTERVALCHECK             Ocif::OCIIntervalCheck             = 0;
OCIINTERVALCOMPARE           Ocif::OCIIntervalCompare           = 0;
OCIINTERVALFROMTEXT          Ocif::OCIIntervalFromText          = 0;
OCIINTERVALTOTEXT            Ocif::OCIIntervalToText            = 0;
OCIINTERVALFROMTZ            Ocif::OCIIntervalFromTZ            = 0;
OCIINTERVALGETDAYSECOND      Ocif::OCIIntervalGetDaySecond      = 0;
OCIINTERVALGETYEARMONTH      Ocif::OCIIntervalGetYearMonth      = 0;
OCIINTERVALSETDAYSECOND      Ocif::OCIIntervalSetDaySecond      = 0;
OCIINTERVALSETYEARMONTH      Ocif::OCIIntervalSetYearMonth      = 0;
OCIINTERVALSUBTRACT          Ocif::OCIIntervalSubtract          = 0;
OCIINTERVALADD               Ocif::OCIIntervalAdd               = 0;
OCIDATETIMEASSIGN            Ocif::OCIDateTimeAssign            = 0;
OCIDATETIMECHECK             Ocif::OCIDateTimeCheck             = 0;
OCIDATETIMECOMPARE           Ocif::OCIDateTimeCompare           = 0;
OCIDATETIMECONSTRUCT         Ocif::OCIDateTimeConstruct         = 0;
OCIDATETIMECONVERT           Ocif::OCIDateTimeConvert           = 0;
OCIDATETIMEFROMARRAY         Ocif::OCIDateTimeFromArray         = 0;
OCIDATETIMETOARRAY           Ocif::OCIDateTimeToArray           = 0;
OCIDATETIMEFROMTEXT          Ocif::OCIDateTimeFromText          = 0;
OCIDATETIMETOTEXT            Ocif::OCIDateTimeToText            = 0;
OCIDATETIMEGETDATE           Ocif::OCIDateTimeGetDate           = 0;
OCIDATETIMEGETTIME           Ocif::OCIDateTimeGetTime           = 0;
OCIDATETIMEGETTIMEZONENAME   Ocif::OCIDateTimeGetTimeZoneName   = 0;
OCIDATETIMEGETTIMEZONEOFFSET Ocif::OCIDateTimeGetTimeZoneOffset = 0;
OCIDATETIMEINTERVALADD       Ocif::OCIDateTimeIntervalAdd       = 0;
OCIDATETIMEINTERVALSUB       Ocif::OCIDateTimeIntervalSub       = 0;
OCIDATETIMESUBTRACT          Ocif::OCIDateTimeSubtract          = 0;
OCIDATETIMESYSTIMESTAMP      Ocif::OCIDateTimeSysTimeStamp      = 0;
OCIARRAYDESCRIPTORFREE       Ocif::OCIArrayDescriptorFree       = 0;
OCICLIENTVERSION             Ocif::OCIClientVersion             = 0;

#define OCIDateGetTime(date, hour, min, sec) { \
     *hour = (date)->OCIDateTime.OCITimeHH; \
     *min = (date)->OCIDateTime.OCITimeMI; \
     *sec = (date)->OCIDateTime.OCITimeSS; }
#define OCIDateGetDate(date, year, month, day) { \
     *year = (date)->OCIDateYYYY; \
     *month = (date)->OCIDateMM; \
     *day = (date)->OCIDateDD; }
#define OCIDateSetTime(date, hour, min, sec) { \
     (date)->OCIDateTime.OCITimeHH = hour; \
     (date)->OCIDateTime.OCITimeMI = min; \
     (date)->OCIDateTime.OCITimeSS = sec; }
#define OCIDateSetDate(date, year, month, day) { \
     (date)->OCIDateYYYY = year; \
     (date)->OCIDateMM = month; \
     (date)->OCIDateDD = day; }
#endif

#endif    /* OCILIB_OCI_IMPORT_H_INCLUDED */

