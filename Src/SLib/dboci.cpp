// DBOCI.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2018, 2020, 2022
//
#include <slib-internal.h>
#pragma hdrstop

OCIENVCREATE                 Ocif::OCIEnvCreate = 0;
OCISERVERATTACH              Ocif::OCIServerAttach = 0;
OCISERVERDETACH              Ocif::OCIServerDetach = 0;
OCIHANDLEALLOC               Ocif::OCIHandleAlloc = 0;
OCIHANDLEFREE                Ocif::OCIHandleFree = 0;
OCIDESCRIPTORALLOC           Ocif::OCIDescriptorAlloc = 0;
OCIDESCRIPTORFREE            Ocif::OCIDescriptorFree = 0;
OCISESSIONBEGIN              Ocif::OCISessionBegin = 0;
OCISESSIONEND                Ocif::OCISessionEnd = 0;
OCIBINDBYPOS                 Ocif::OCIBindByPos = 0;
OCIBINDBYNAME                Ocif::OCIBindByName = 0;
OCIDEFINEBYPOS               Ocif::OCIDefineByPos = 0;
OCISTMTPREPARE               Ocif::OCIStmtPrepare = 0;
OCISTMTEXECUTE               Ocif::OCIStmtExecute = 0;
OCISTMTFETCH                 Ocif::OCIStmtFetch = 0;
OCISTMTFETCH2                Ocif::OCIStmtFetch2 = 0;
OCISTMTGETPIECEINFO          Ocif::OCIStmtGetPieceInfo = 0;
OCISTMTSETPIECEINFO          Ocif::OCIStmtSetPieceInfo = 0;
OCIPARAMGET                  Ocif::OCIParamGet = 0;
OCIPARAMSET                  Ocif::OCIParamSet = 0;
OCITRANSSTART                Ocif::OCITransStart = 0;
OCITRANSDETACH               Ocif::OCITransDetach = 0;
OCITRANSPREPARE              Ocif::OCITransPrepare = 0;
OCITRANSFORGET               Ocif::OCITransForget = 0;
OCITRANSCOMMIT               Ocif::OCITransCommit = 0;
OCITRANSROLLBACK             Ocif::OCITransRollback = 0;
OCIERRORGET                  Ocif::OCIErrorGet = 0;
OCILOBCREATETEMPORARY        Ocif::OCILobCreateTemporary = 0;
OCILOBFREETEMPORARY          Ocif::OCILobFreeTemporary = 0;
OCILOBISTEMPORARY            Ocif::OCILobIsTemporary = 0;
OCILOBAPPEND                 Ocif::OCILobAppend = 0;
OCILOBCOPY                   Ocif::OCILobCopy = 0;
OCILOBGETLENGTH              Ocif::OCILobGetLength = 0;
OCILOBREAD                   Ocif::OCILobRead = 0;
OCILOBWRITE                  Ocif::OCILobWrite = 0;
OCILOBTRIM                   Ocif::OCILobTrim = 0;
OCILOBERASE                  Ocif::OCILobErase = 0;
OCILOBOPEN                   Ocif::OCILobOpen = 0;
OCILOBCLOSE                  Ocif::OCILobClose = 0;
OCILOBGETCHUNKSIZE           Ocif::OCILobGetChunkSize = 0; // @v6.1.5
OCILOBFILEOPEN               Ocif::OCILobFileOpen = 0;
OCILOBFILECLOSE              Ocif::OCILobFileClose = 0;
OCILOBFILECLOSEALL           Ocif::OCILobFileCloseAll = 0;
OCILOBFILEISOPEN             Ocif::OCILobFileIsOpen = 0;
OCILOBFILEEXISTS             Ocif::OCILobFileExists = 0;
OCILOBFIELGETNAME            Ocif::OCILobFileGetName = 0;
OCILOBFILESETNAME            Ocif::OCILobFileSetName = 0;
OCILOBLOADFROMFILE           Ocif::OCILobLoadFromFile = 0;
OCILOBWRITEAPPEND            Ocif::OCILobWriteAppend = 0;
OCILOBISEQUAL                Ocif::OCILobIsEqual = 0;
OCILOBASSIGN                 Ocif::OCILobAssign = 0;       // @v6.1.6
OCISERVERVERSION             Ocif::OCIServerVersion = 0;
OCIATTRGET                   Ocif::OCIAttrGet = 0;
OCIATTRSET                   Ocif::OCIAttrSet = 0;
OCIDATEASSIGN                Ocif::OCIDateAssign = 0;
OCIDATETOTEXT                Ocif::OCIDateToText = 0;
OCIDATEFROMTEXT              Ocif::OCIDateFromText = 0;
OCIDATECOMPARE               Ocif::OCIDateCompare = 0;
OCIDATEADDMONTHS             Ocif::OCIDateAddMonths = 0;
OCIDATEADDDAYS               Ocif::OCIDateAddDays = 0;
OCIDATELASTDAY               Ocif::OCIDateLastDay = 0;
OCIDATEDAYSBETWEEN           Ocif::OCIDateDaysBetween = 0;
OCIDATEZONETOZONE            Ocif::OCIDateZoneToZone = 0;
OCIDATENEXTDAY               Ocif::OCIDateNextDay = 0;
OCIDATECHECK                 Ocif::OCIDateCheck = 0;
OCIDATESYSDATE               Ocif::OCIDateSysDate = 0;
OCIDESCRIBEANY               Ocif::OCIDescribeAny = 0;
OCIINTERVALASSIGN            Ocif::OCIIntervalAssign = 0;
OCIINTERVALCHECK             Ocif::OCIIntervalCheck = 0;
OCIINTERVALCOMPARE           Ocif::OCIIntervalCompare = 0;
OCIINTERVALFROMTEXT          Ocif::OCIIntervalFromText = 0;
OCIINTERVALTOTEXT            Ocif::OCIIntervalToText = 0;
OCIINTERVALFROMTZ            Ocif::OCIIntervalFromTZ = 0;
OCIINTERVALGETDAYSECOND      Ocif::OCIIntervalGetDaySecond = 0;
OCIINTERVALGETYEARMONTH      Ocif::OCIIntervalGetYearMonth = 0;
OCIINTERVALSETDAYSECOND      Ocif::OCIIntervalSetDaySecond = 0;
OCIINTERVALSETYEARMONTH      Ocif::OCIIntervalSetYearMonth = 0;
OCIINTERVALSUBTRACT          Ocif::OCIIntervalSubtract = 0;
OCIINTERVALADD               Ocif::OCIIntervalAdd = 0;
OCIDATETIMEASSIGN            Ocif::OCIDateTimeAssign = 0;
OCIDATETIMECHECK             Ocif::OCIDateTimeCheck = 0;
OCIDATETIMECOMPARE           Ocif::OCIDateTimeCompare = 0;
OCIDATETIMECONSTRUCT         Ocif::OCIDateTimeConstruct = 0;
OCIDATETIMECONVERT           Ocif::OCIDateTimeConvert = 0;
OCIDATETIMEFROMARRAY         Ocif::OCIDateTimeFromArray = 0;
OCIDATETIMETOARRAY           Ocif::OCIDateTimeToArray = 0;
OCIDATETIMEFROMTEXT          Ocif::OCIDateTimeFromText = 0;
OCIDATETIMETOTEXT            Ocif::OCIDateTimeToText = 0;
OCIDATETIMEGETDATE           Ocif::OCIDateTimeGetDate = 0;
OCIDATETIMEGETTIME           Ocif::OCIDateTimeGetTime = 0;
OCIDATETIMEGETTIMEZONENAME   Ocif::OCIDateTimeGetTimeZoneName = 0;
OCIDATETIMEGETTIMEZONEOFFSET Ocif::OCIDateTimeGetTimeZoneOffset = 0;
OCIDATETIMEINTERVALADD       Ocif::OCIDateTimeIntervalAdd = 0;
OCIDATETIMEINTERVALSUB       Ocif::OCIDateTimeIntervalSub = 0;
OCIDATETIMESUBTRACT          Ocif::OCIDateTimeSubtract = 0;
OCIDATETIMESYSTIMESTAMP      Ocif::OCIDateTimeSysTimeStamp = 0;
OCIARRAYDESCRIPTORFREE       Ocif::OCIArrayDescriptorFree = 0;
OCICLIENTVERSION             Ocif::OCIClientVersion = 0;
OCISTRINGPTR                 Ocif::OCIStringPtr = 0;
OCISTRINGASSIGNTEXT          Ocif::OCIStringAssignText = 0;
OCIRAWPTR                    Ocif::OCIRawPtr = 0;
OCIRAWASSIGNBYTES            Ocif::OCIRawAssignBytes = 0;
OCIRAWALLOCSIZE              Ocif::OCIRawAllocSize = 0;
OCIRAWRESIZE                 Ocif::OCIRawResize = 0;
OCIROWIDTOCHAR               Ocif::OCIRowidToChar = 0;
OCIBINDARRAYOFSTRUCT         Ocif::OCIBindArrayOfStruct = 0;
OCIDEFINEARRAYOFSTRUCT       Ocif::OCIDefineArrayOfStruct = 0;

SDynLibrary * Ocif::P_Lib = 0; // @global @threadsafe

static int oci_stub() { return -1; }

int Ocif::Release()
{
	if(P_Lib) {
		ZDELETE(P_Lib);
		return 1;
	}
	else
		return -1;
}

int Ocif::Load()
{
	if(P_Lib) {
		return P_Lib->IsValid() ? -1 : 0;
	}
	else {
		ENTER_CRITICAL_SECTION
		if(!P_Lib) {
			P_Lib = new SDynLibrary("OCI.DLL");
	#define LIB_SYMB(symb,typ) symb = (typ)P_Lib->GetProcAddr(#symb, 0); if(!symb) symb = (typ)oci_stub;
    		LIB_SYMB(OCIEnvCreate, OCIENVCREATE);
    		LIB_SYMB(OCIServerAttach, OCISERVERATTACH);
    		LIB_SYMB(OCIServerDetach, OCISERVERDETACH);
    		LIB_SYMB(OCIHandleAlloc, OCIHANDLEALLOC);
    		LIB_SYMB(OCIHandleFree, OCIHANDLEFREE);
    		LIB_SYMB(OCIDescriptorAlloc, OCIDESCRIPTORALLOC);
    		LIB_SYMB(OCIDescriptorFree, OCIDESCRIPTORFREE);
    		LIB_SYMB(OCIAttrSet, OCIATTRSET);
    		LIB_SYMB(OCIAttrGet, OCIATTRGET);
    		LIB_SYMB(OCIParamSet, OCIPARAMSET);
    		LIB_SYMB(OCIParamGet, OCIPARAMGET);
    		LIB_SYMB(OCISessionBegin, OCISESSIONBEGIN);
    		LIB_SYMB(OCISessionEnd, OCISESSIONEND);
    		LIB_SYMB(OCITransStart, OCITRANSSTART);
    		LIB_SYMB(OCITransDetach, OCITRANSDETACH);
    		LIB_SYMB(OCITransPrepare, OCITRANSPREPARE);
    		LIB_SYMB(OCITransForget, OCITRANSFORGET);
    		LIB_SYMB(OCITransCommit, OCITRANSCOMMIT);
    		LIB_SYMB(OCITransRollback, OCITRANSROLLBACK);
    		LIB_SYMB(OCIErrorGet, OCIERRORGET);
    		LIB_SYMB(OCIServerVersion, OCISERVERVERSION);
    		LIB_SYMB(OCIBindByPos, OCIBINDBYPOS);
    		LIB_SYMB(OCIBindByName, OCIBINDBYNAME);
    		LIB_SYMB(OCIDefineByPos, OCIDEFINEBYPOS);
    		LIB_SYMB(OCIStmtPrepare, OCISTMTPREPARE);
    		LIB_SYMB(OCIStmtExecute, OCISTMTEXECUTE);
    		LIB_SYMB(OCIStmtFetch, OCISTMTFETCH);
			LIB_SYMB(OCIStmtFetch2, OCISTMTFETCH2);
    		LIB_SYMB(OCIStmtGetPieceInfo, OCISTMTGETPIECEINFO);
    		LIB_SYMB(OCIStmtSetPieceInfo, OCISTMTSETPIECEINFO);
    		LIB_SYMB(OCILobCreateTemporary, OCILOBCREATETEMPORARY);
    		LIB_SYMB(OCILobFreeTemporary, OCILOBFREETEMPORARY);
    		LIB_SYMB(OCILobIsTemporary, OCILOBISTEMPORARY);
    		LIB_SYMB(OCILobRead, OCILOBREAD);
    		LIB_SYMB(OCILobWrite, OCILOBWRITE);
    		LIB_SYMB(OCILobCopy, OCILOBCOPY);
    		LIB_SYMB(OCILobTrim, OCILOBTRIM);
    		LIB_SYMB(OCILobErase, OCILOBERASE);
    		LIB_SYMB(OCILobAppend, OCILOBAPPEND);
    		LIB_SYMB(OCILobGetLength, OCILOBGETLENGTH);
    		LIB_SYMB(OCILobOpen, OCILOBOPEN);
    		LIB_SYMB(OCILobClose, OCILOBCLOSE);
			LIB_SYMB(OCILobGetChunkSize, OCILOBGETCHUNKSIZE);
    		LIB_SYMB(OCILobFileOpen, OCILOBFILEOPEN);
    		LIB_SYMB(OCILobFileClose, OCILOBFILECLOSE);
    		LIB_SYMB(OCILobFileCloseAll, OCILOBFILECLOSEALL);
    		LIB_SYMB(OCILobFileIsOpen, OCILOBFILEISOPEN);
    		LIB_SYMB(OCILobFileExists, OCILOBFILEEXISTS);
    		LIB_SYMB(OCILobFileGetName, OCILOBFIELGETNAME);
    		LIB_SYMB(OCILobFileSetName, OCILOBFILESETNAME);
    		LIB_SYMB(OCILobLoadFromFile, OCILOBLOADFROMFILE);
    		LIB_SYMB(OCILobWriteAppend, OCILOBWRITEAPPEND);
    		LIB_SYMB(OCILobIsEqual, OCILOBISEQUAL);
			LIB_SYMB(OCILobAssign,  OCILOBASSIGN);
    		LIB_SYMB(OCIDateAssign, OCIDATEASSIGN);
    		LIB_SYMB(OCIDateToText, OCIDATETOTEXT);
    		LIB_SYMB(OCIDateFromText, OCIDATEFROMTEXT);
    		LIB_SYMB(OCIDateCompare, OCIDATECOMPARE);
    		LIB_SYMB(OCIDateAddMonths, OCIDATEADDMONTHS);
    		LIB_SYMB(OCIDateAddDays, OCIDATEADDDAYS);
    		LIB_SYMB(OCIDateLastDay, OCIDATELASTDAY);
    		LIB_SYMB(OCIDateDaysBetween, OCIDATEDAYSBETWEEN);
    		LIB_SYMB(OCIDateZoneToZone, OCIDATEZONETOZONE);
    		LIB_SYMB(OCIDateNextDay, OCIDATENEXTDAY);
    		LIB_SYMB(OCIDateCheck, OCIDATECHECK);
    		LIB_SYMB(OCIDateSysDate, OCIDATESYSDATE);
    		LIB_SYMB(OCIDescribeAny, OCIDESCRIBEANY);
    		LIB_SYMB(OCIIntervalAssign, OCIINTERVALASSIGN);
    		LIB_SYMB(OCIIntervalCheck, OCIINTERVALCHECK);
    		LIB_SYMB(OCIIntervalCompare, OCIINTERVALCOMPARE);
    		LIB_SYMB(OCIIntervalFromText, OCIINTERVALFROMTEXT);
    		LIB_SYMB(OCIIntervalToText, OCIINTERVALTOTEXT);
    		LIB_SYMB(OCIIntervalFromTZ, OCIINTERVALFROMTZ);
    		LIB_SYMB(OCIIntervalGetDaySecond, OCIINTERVALGETDAYSECOND);
    		LIB_SYMB(OCIIntervalGetYearMonth, OCIINTERVALGETYEARMONTH);
    		LIB_SYMB(OCIIntervalSetDaySecond, OCIINTERVALSETDAYSECOND);
    		LIB_SYMB(OCIIntervalSetYearMonth, OCIINTERVALSETYEARMONTH);
    		LIB_SYMB(OCIIntervalSubtract, OCIINTERVALSUBTRACT);
    		LIB_SYMB(OCIIntervalAdd, OCIINTERVALADD);
    		LIB_SYMB(OCIDateTimeAssign, OCIDATETIMEASSIGN);
    		LIB_SYMB(OCIDateTimeCheck, OCIDATETIMECHECK);
    		LIB_SYMB(OCIDateTimeCompare, OCIDATETIMECOMPARE);
    		LIB_SYMB(OCIDateTimeConstruct, OCIDATETIMECONSTRUCT);
    		LIB_SYMB(OCIDateTimeConvert, OCIDATETIMECONVERT);
    		LIB_SYMB(OCIDateTimeFromArray, OCIDATETIMEFROMARRAY);
    		LIB_SYMB(OCIDateTimeToArray, OCIDATETIMETOARRAY);
    		LIB_SYMB(OCIDateTimeFromText, OCIDATETIMEFROMTEXT);
    		LIB_SYMB(OCIDateTimeToText, OCIDATETIMETOTEXT);
    		LIB_SYMB(OCIDateTimeGetDate, OCIDATETIMEGETDATE);
    		LIB_SYMB(OCIDateTimeGetTime, OCIDATETIMEGETTIME);
    		LIB_SYMB(OCIDateTimeGetTimeZoneName, OCIDATETIMEGETTIMEZONENAME);
    		LIB_SYMB(OCIDateTimeGetTimeZoneOffset, OCIDATETIMEGETTIMEZONEOFFSET);
    		LIB_SYMB(OCIDateTimeIntervalAdd, OCIDATETIMEINTERVALADD);
    		LIB_SYMB(OCIDateTimeIntervalSub, OCIDATETIMEINTERVALSUB);
    		LIB_SYMB(OCIDateTimeSubtract, OCIDATETIMESUBTRACT);
    		LIB_SYMB(OCIDateTimeSysTimeStamp, OCIDATETIMESYSTIMESTAMP);
    		LIB_SYMB(OCIArrayDescriptorFree, OCIARRAYDESCRIPTORFREE);
    		LIB_SYMB(OCIClientVersion, OCICLIENTVERSION);

			LIB_SYMB(OCIStringPtr, OCISTRINGPTR);
			LIB_SYMB(OCIStringAssignText, OCISTRINGASSIGNTEXT);
			LIB_SYMB(OCIRawPtr, OCIRAWPTR);
			LIB_SYMB(OCIRawAssignBytes, OCIRAWASSIGNBYTES);
			LIB_SYMB(OCIRawAllocSize, OCIRAWALLOCSIZE);
			LIB_SYMB(OCIRawResize, OCIRAWRESIZE);
			LIB_SYMB(OCIRowidToChar, OCIROWIDTOCHAR);

			LIB_SYMB(OCIBindArrayOfStruct, OCIBINDARRAYOFSTRUCT);
			LIB_SYMB(OCIDefineArrayOfStruct, OCIDEFINEARRAYOFSTRUCT);
	#undef LIB_SYMB
		}
		LEAVE_CRITICAL_SECTION
		return BIN(P_Lib->IsValid());
	}
}
