/* attrask.c -- Interrogate attribute type

   (c) 1998-2006 (W3C) MIT, ERCIM, Keio University
   See tidy.h for the copyright notice.

   CVS Info:
    $Author: arnaud02 $
    $Date: 2006/09/12 15:14:44 $
    $Revision: 1.5 $

 */
#include "tidy-int.h"
#pragma hdrstop

bool TIDY_CALL tidyAttrIsHREF(TidyAttr tattr)
{
	return attrIsHREF(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsSRC(TidyAttr tattr)
{
	return attrIsSRC(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsID(TidyAttr tattr)
{
	return attrIsID(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsNAME(TidyAttr tattr)
{
	return attrIsNAME(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsSUMMARY(TidyAttr tattr)
{
	return attrIsSUMMARY(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsALT(TidyAttr tattr)
{
	return attrIsALT(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsLONGDESC(TidyAttr tattr)
{
	return attrIsLONGDESC(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsUSEMAP(TidyAttr tattr)
{
	return attrIsUSEMAP(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsISMAP(TidyAttr tattr)
{
	return attrIsISMAP(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsLANGUAGE(TidyAttr tattr)
{
	return attrIsLANGUAGE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsTYPE(TidyAttr tattr)
{
	return attrIsTYPE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsVALUE(TidyAttr tattr)
{
	return attrIsVALUE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsCONTENT(TidyAttr tattr)
{
	return attrIsCONTENT(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsTITLE(TidyAttr tattr)
{
	return attrIsTITLE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsXMLNS(TidyAttr tattr)
{
	return attrIsXMLNS(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsDATAFLD(TidyAttr tattr)
{
	return attrIsDATAFLD(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsWIDTH(TidyAttr tattr)
{
	return attrIsWIDTH(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsHEIGHT(TidyAttr tattr)
{
	return attrIsHEIGHT(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsFOR(TidyAttr tattr)
{
	return attrIsFOR(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsSELECTED(TidyAttr tattr)
{
	return attrIsSELECTED(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsCHECKED(TidyAttr tattr)
{
	return attrIsCHECKED(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsLANG(TidyAttr tattr)
{
	return attrIsLANG(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsTARGET(TidyAttr tattr)
{
	return attrIsTARGET(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsHTTP_EQUIV(TidyAttr tattr)
{
	return attrIsHTTP_EQUIV(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsREL(TidyAttr tattr)
{
	return attrIsREL(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsEvent(TidyAttr tattr)
{
	return TY_(attrIsEvent) (tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnMOUSEMOVE(TidyAttr tattr)
{
	return attrIsOnMOUSEMOVE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnMOUSEDOWN(TidyAttr tattr)
{
	return attrIsOnMOUSEDOWN(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnMOUSEUP(TidyAttr tattr)
{
	return attrIsOnMOUSEUP(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnCLICK(TidyAttr tattr)
{
	return attrIsOnCLICK(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnMOUSEOVER(TidyAttr tattr)
{
	return attrIsOnMOUSEOVER(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnMOUSEOUT(TidyAttr tattr)
{
	return attrIsOnMOUSEOUT(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnKEYDOWN(TidyAttr tattr)
{
	return attrIsOnKEYDOWN(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnKEYUP(TidyAttr tattr)
{
	return attrIsOnKEYUP(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnKEYPRESS(TidyAttr tattr)
{
	return attrIsOnKEYPRESS(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnFOCUS(TidyAttr tattr)
{
	return attrIsOnFOCUS(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsOnBLUR(TidyAttr tattr)
{
	return attrIsOnBLUR(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsBGCOLOR(TidyAttr tattr)
{
	return attrIsBGCOLOR(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsLINK(TidyAttr tattr)
{
	return attrIsLINK(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsALINK(TidyAttr tattr)
{
	return attrIsALINK(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsVLINK(TidyAttr tattr)
{
	return attrIsVLINK(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsTEXT(TidyAttr tattr)
{
	return attrIsTEXT(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsSTYLE(TidyAttr tattr)
{
	return attrIsSTYLE(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsABBR(TidyAttr tattr)
{
	return attrIsABBR(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsCOLSPAN(TidyAttr tattr)
{
	return attrIsCOLSPAN(tidyAttrToImpl(tattr));
}

bool TIDY_CALL tidyAttrIsROWSPAN(TidyAttr tattr)
{
	return attrIsROWSPAN(tidyAttrToImpl(tattr));
}

/*
 * local variables:
 * mode: c
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * eval: (c-set-offset 'substatement-open 0)
 * end:
 */
