/* tagask.c -- Interrogate node type

   (c) 1998-2006 (W3C) MIT, ERCIM, Keio University
   See tidy.h for the copyright notice.

   CVS Info :

    $Author: arnaud02 $
    $Date: 2006/09/12 15:14:44 $
    $Revision: 1.6 $

*/
#include "tidy-int.h"
#pragma hdrstop

bool TIDY_CALL tidyNodeIsText(TidyNode tnod)
{
	return TY_(nodeIsText) (tidyNodeToImpl(tnod));
}

bool tidyNodeCMIsBlock(TidyNode tnod);   /* not exported yet */
bool tidyNodeCMIsBlock(TidyNode tnod)
{
	return TY_(nodeCMIsBlock) (tidyNodeToImpl(tnod));
}

bool tidyNodeCMIsInline(TidyNode tnod);   /* not exported yet */
bool tidyNodeCMIsInline(TidyNode tnod)
{
	return TY_(nodeCMIsInline) (tidyNodeToImpl(tnod));
}

bool tidyNodeCMIsEmpty(TidyNode tnod);   /* not exported yet */
bool tidyNodeCMIsEmpty(TidyNode tnod)
{
	return TY_(nodeCMIsEmpty) (tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsHeader(TidyNode tnod)
{
	return TY_(nodeIsHeader) (tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsHTML(TidyNode tnod)
{
	return nodeIsHTML(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsHEAD(TidyNode tnod)
{
	return nodeIsHEAD(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTITLE(TidyNode tnod)
{
	return nodeIsTITLE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBASE(TidyNode tnod)
{
	return nodeIsBASE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsMETA(TidyNode tnod)
{
	return nodeIsMETA(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBODY(TidyNode tnod)
{
	return nodeIsBODY(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsFRAMESET(TidyNode tnod)
{
	return nodeIsFRAMESET(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsFRAME(TidyNode tnod)
{
	return nodeIsFRAME(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsIFRAME(TidyNode tnod)
{
	return nodeIsIFRAME(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsNOFRAMES(TidyNode tnod)
{
	return nodeIsNOFRAMES(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsHR(TidyNode tnod)
{
	return nodeIsHR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH1(TidyNode tnod)
{
	return nodeIsH1(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH2(TidyNode tnod)
{
	return nodeIsH2(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsPRE(TidyNode tnod)
{
	return nodeIsPRE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsLISTING(TidyNode tnod)
{
	return nodeIsLISTING(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsP(TidyNode tnod)
{
	return nodeIsP(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsUL(TidyNode tnod)
{
	return nodeIsUL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsOL(TidyNode tnod)
{
	return nodeIsOL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsDL(TidyNode tnod)
{
	return nodeIsDL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsDIR(TidyNode tnod)
{
	return nodeIsDIR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsLI(TidyNode tnod)
{
	return nodeIsLI(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsDT(TidyNode tnod)
{
	return nodeIsDT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsDD(TidyNode tnod)
{
	return nodeIsDD(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTABLE(TidyNode tnod)
{
	return nodeIsTABLE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsCAPTION(TidyNode tnod)
{
	return nodeIsCAPTION(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTD(TidyNode tnod)
{
	return nodeIsTD(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTH(TidyNode tnod)
{
	return nodeIsTH(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTR(TidyNode tnod)
{
	return nodeIsTR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsCOL(TidyNode tnod)
{
	return nodeIsCOL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsCOLGROUP(TidyNode tnod)
{
	return nodeIsCOLGROUP(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBR(TidyNode tnod)
{
	return nodeIsBR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsA(TidyNode tnod)
{
	return nodeIsA(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsLINK(TidyNode tnod)
{
	return nodeIsLINK(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsB(TidyNode tnod)
{
	return nodeIsB(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsI(TidyNode tnod)
{
	return nodeIsI(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSTRONG(TidyNode tnod)
{
	return nodeIsSTRONG(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsEM(TidyNode tnod)
{
	return nodeIsEM(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBIG(TidyNode tnod)
{
	return nodeIsBIG(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSMALL(TidyNode tnod)
{
	return nodeIsSMALL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsPARAM(TidyNode tnod)
{
	return nodeIsPARAM(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsOPTION(TidyNode tnod)
{
	return nodeIsOPTION(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsOPTGROUP(TidyNode tnod)
{
	return nodeIsOPTGROUP(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsIMG(TidyNode tnod)
{
	return nodeIsIMG(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsMAP(TidyNode tnod)
{
	return nodeIsMAP(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsAREA(TidyNode tnod)
{
	return nodeIsAREA(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsNOBR(TidyNode tnod)
{
	return nodeIsNOBR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsWBR(TidyNode tnod)
{
	return nodeIsWBR(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsFONT(TidyNode tnod)
{
	return nodeIsFONT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsLAYER(TidyNode tnod)
{
	return nodeIsLAYER(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSPACER(TidyNode tnod)
{
	return nodeIsSPACER(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsCENTER(TidyNode tnod)
{
	return nodeIsCENTER(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSTYLE(TidyNode tnod)
{
	return nodeIsSTYLE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSCRIPT(TidyNode tnod)
{
	return nodeIsSCRIPT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsNOSCRIPT(TidyNode tnod)
{
	return nodeIsNOSCRIPT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsFORM(TidyNode tnod)
{
	return nodeIsFORM(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsTEXTAREA(TidyNode tnod)
{
	return nodeIsTEXTAREA(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBLOCKQUOTE(TidyNode tnod)
{
	return nodeIsBLOCKQUOTE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsAPPLET(TidyNode tnod)
{
	return nodeIsAPPLET(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsOBJECT(TidyNode tnod)
{
	return nodeIsOBJECT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsDIV(TidyNode tnod)
{
	return nodeIsDIV(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSPAN(TidyNode tnod)
{
	return nodeIsSPAN(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsINPUT(TidyNode tnod)
{
	return nodeIsINPUT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsQ(TidyNode tnod)
{
	return nodeIsQ(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsLABEL(TidyNode tnod)
{
	return nodeIsLABEL(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH3(TidyNode tnod)
{
	return nodeIsH3(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH4(TidyNode tnod)
{
	return nodeIsH4(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH5(TidyNode tnod)
{
	return nodeIsH5(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsH6(TidyNode tnod)
{
	return nodeIsH6(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsADDRESS(TidyNode tnod)
{
	return nodeIsADDRESS(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsXMP(TidyNode tnod)
{
	return nodeIsXMP(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSELECT(TidyNode tnod)
{
	return nodeIsSELECT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBLINK(TidyNode tnod)
{
	return nodeIsBLINK(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsMARQUEE(TidyNode tnod)
{
	return nodeIsMARQUEE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsEMBED(TidyNode tnod)
{
	return nodeIsEMBED(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsBASEFONT(TidyNode tnod)
{
	return nodeIsBASEFONT(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsISINDEX(TidyNode tnod)
{
	return nodeIsISINDEX(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsS(TidyNode tnod)
{
	return nodeIsS(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsSTRIKE(TidyNode tnod)
{
	return nodeIsSTRIKE(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsU(TidyNode tnod)
{
	return nodeIsU(tidyNodeToImpl(tnod));
}

bool TIDY_CALL tidyNodeIsMENU(TidyNode tnod)
{
	return nodeIsMENU(tidyNodeToImpl(tnod));
}

/*
 * local variables:
 * mode: c
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * eval: (c-set-offset 'substatement-open 0)
 * end:
 */
