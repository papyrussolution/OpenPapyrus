// PPDS_I.CPP
// Copyright (c) A.Sobolev 2001, 2009
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <ppds.h>
//
//
//
class PPDS_AccturnTempl : public PPDeclStruc {
public:
	SLAPI  PPDS_AccturnTempl();

	PPAccTurnTempl Data;
protected:
	virtual int SLAPI InitData(Ido op, long dataParam, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);

	char PSubst[64], FSubst[64];
};

SLAPI PPDS_AccturnTempl::PPDS_AccturnTempl() : PPDeclStruc()
{
}

int SLAPI PPDS_AccturnTempl::InitData(Ido op, long dataParam, long /*addedParam*/)
{
	int ok = 1;
	if(op == idoAlloc) {
		memset(&Data, 0, sizeof(Data));
		PSubst[0] = FSubst[0] = 0;
		ok = 1;
	}
	else if(op == idoDestroy) {
		PSubst[0] = FSubst[0] = 0;
		ok = 1;
	}
	else if(op == idoExtract) {
		if(dataParam) {
			Data = *(PPAccTurnTempl *)dataParam;
		}
		ok = 1;
	}
	else if(op == idoAccept) {
		Data.SetupSubst(PSubst, FSubst);
		ok = 1;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_AccturnTempl::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int  ok = -1;
	long flag = 0;
	if(dir != tfdDataToBuf && dir != tfdBufToData)
		return (PPErrCode = PPERR_INVPARAM, 0);
	switch(fldID) {
		case DSF_ACCTURNTEMPL_OPID:
			ok = TransferData(&Data.ObjID, dir, rBuf);
			break;
		//case DSF_ACCTURNTEMPL_OPSYMB:
		case DSF_ACCTURNTEMPL_TEMPLNO:
			ok = TransferData(&Data.ID, dir, rBuf);
			break;
		case DSF_ACCTURNTEMPL_DEBITACC:
		case DSF_ACCTURNTEMPL_CREDITACC:
			{
				int side = (fldID == DSF_ACCTURNTEMPL_DEBITACC) ?
					PPDEBIT : PPCREDIT;
				if(dir == tfdDataToBuf)
					ok = Data.AccTemplToStr(side, rBuf);
				else if(dir == tfdBufToData)
					ok = Data.AccTemplFromStr(side, rBuf);
				else
					ok = (PPErrCode = PPERR_INVPARAM, 0);
			}
			break;
		case DSF_ACCTURNTEMPL_PRIMSUBST:
			ok = TransferData(PSubst, sizeof(PSubst), dir, rBuf, 1);
			break;
		case DSF_ACCTURNTEMPL_SECSUBST:
			ok = TransferData(FSubst, sizeof(FSubst), dir, rBuf, 1);
			break;
		case DSF_ACCTURNTEMPL_EXPRESSION:
			ok = TransferData(Data.Expr, sizeof(Data.Expr), dir, rBuf);
			break;
		case DSF_ACCTURNTEMPL_FPRIMONCREDIT:   flag = ATTF_PRIMONCREDIT;   break;
		case DSF_ACCTURNTEMPL_FSKIPNEG:        flag = ATTF_SKIPNEG;        break;
		case DSF_ACCTURNTEMPL_FINVERTNEG:      flag = ATTF_INVERTNEG;      break;
		case DSF_ACCTURNTEMPL_FBASEPROJECTION: flag = ATTF_BASEPROJECTION; break;
		case DSF_ACCTURNTEMPL_FINTROUNDING:    flag = ATTF_INTROUNDING;    break;
		case DSF_ACCTURNTEMPL_FPASSIVE:        flag = ATTF_PASSIVE;        break;
	}
	if(flag)
		ok = TransferData(&Data.Flags, flag, dir, rBuf, 1);
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
class PPDS_OprKind : public PPDeclStruc {
public:
	SLAPI  PPDS_OprKind();

	virtual int SLAPI InitData(Ido op, long dataParam, long addedParam);
	virtual int SLAPI TransferField(long fldID, Tfd dir, uint *, SString & rBuf);
	virtual int SLAPI AcceptListItem(long fldID, PPDeclStruc *, ObjTransmContext * pCtx);
	virtual int SLAPI CreateListItem(long fldID, uint *, PPDeclStruc *);

	PPOprKindPacket Data;
private:
	int    SLAPI TransferExtStrData(int, Tfd, char *, size_t bufLen);

	PPReckonOpEx ROX;
};

SLAPI PPDS_OprKind::PPDS_OprKind() : PPDeclStruc()
{
}

int SLAPI PPDS_OprKind::TransferExtStrData(int esd, Tfd dir, char * pBuf, size_t bufLen)
{
	int    ok = -1;
	if(dir == tfdDataToBuf) {
		SString temp_buf;
		if(Data.GetExtStrData(esd, temp_buf) > 0 && temp_buf.NotEmpty()) {
			temp_buf.CopyTo(pBuf, bufLen);
			ok = 1;
		}
	}
	else if(dir == tfdBufToData) {
		if(Data.PutExtStrData(esd, pBuf) > 0)
			ok = 1;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_OprKind::InitData(Ido op, long /*dataParam*/, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		Data.Init();
		ROX.Init();
		ok = 1;
	}
	else if(op == idoDestroy) {
		Data.Init();
		ROX.Init();
		ok = 1;
	}
	else if(op == idoExtract) {
		if(addedParam) {
			PPObjOprKind op_obj;
			Data.Init();
			ok = op_obj.GetPacket(addedParam, &Data);
			if(Data.P_ReckonData)
				ROX = *Data.P_ReckonData;
			else
				ROX.Init();
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		ok = 1;
		if(Data.Rec.Flags & OPKF_RECKON) {
			ZDELETE(Data.P_ReckonData);
			Data.P_ReckonData = new PPReckonOpEx;
			if(Data.P_ReckonData) {
				*Data.P_ReckonData = ROX;
			}
			else
				ok = (PPSetErrorNoMem(), 0);
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_OprKind::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	long   flag = 0, prn_flag = 0, reckon_flag = 0;
	if(dir != tfdDataToBuf && dir != tfdBufToData)
		return (PPErrCode = PPERR_INVPARAM, 0);
	switch(fldID) {
		case DSF_OPRKIND_ID:
			ok = TransferData(&Data.Rec.ID, dir, rBuf);
			break;
		case DSF_OPRKIND_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_OPRKIND_OPTYPEID:
			ok = TransferData(&Data.Rec.OpTypeID, dir, rBuf);
			break;
		case DSF_OPRKIND_ACCSHEETID:
			ok = TransferData(&Data.Rec.AccSheetID, dir, rBuf);
			break;
		case DSF_OPRKIND_ACCSHEET2ID:
			ok = TransferData(&Data.Rec.AccSheet2ID, dir, rBuf, 1);
			break;
		case DSF_OPRKIND_LINKOPID:
			ok = TransferData(&Data.Rec.LinkOpID, dir, rBuf, 1);
			break;
		case DSF_OPRKIND_RANK:
			ok = TransferData(&Data.Rec.Rank, dir, rBuf, 1);
			break;
		case DSF_OPRKIND_FNEEDPAYMENT: flag = OPKF_NEEDPAYMENT; break;
		case DSF_OPRKIND_FBUYING:      flag = OPKF_BUYING;      break;
		case DSF_OPRKIND_FSELLING:     flag = OPKF_SELLING;     break;
		case DSF_OPRKIND_FPROFITABLE:  flag = OPKF_PROFITABLE;  break;
		case DSF_OPRKIND_FONORDER:     flag = OPKF_ONORDER;     break;
		case DSF_OPRKIND_FFREIGHT:     flag = OPKF_FREIGHT;     break;
		case DSF_OPRKIND_FCALCSTAXES:  flag = OPKF_CALCSTAXES;  break;
		case DSF_OPRKIND_FAUTOWL:      flag = OPKF_AUTOWL;      break;
		case DSF_OPRKIND_FUSEEXT:      flag = OPKF_USEPAYER;    break;
		case DSF_OPRKIND_FEXTACCTURN:  flag = OPKF_EXTACCTURN;  break;
		case DSF_OPRKIND_FEXTAMTLIST:  flag = OPKF_EXTAMTLIST;  break;
		case DSF_OPRKIND_FRENT:        flag = OPKF_RENT;        break;
		case DSF_OPRKIND_FNEEDACK:     flag = OPKF_NEEDACK;     break;
		case DSF_OPRKIND_FNOCALCTIORD: flag = OPKF_NOCALCTIORD; break;
		case DSF_OPRKIND_FRECKON:      flag = OPKF_RECKON;      break;
		case DSF_OPRKIND_FBANKING:     flag = OPKF_BANKING;     break;
		case DSF_OPRKIND_FPASSIVE:     flag = OPKF_PASSIVE;     break;
		case DSF_OPRKIND_FCURTRANSIT:  flag = OPKF_CURTRANSIT;  break;

		case DSF_OPRKIND_FPRTINCINVOICE: prn_flag = OPKF_PRT_INCINVC; break;
		case DSF_OPRKIND_FPRTNEGINVOICE: prn_flag = OPKF_PRT_NEGINVC; break;
		case DSF_OPRKIND_FPRTBUYING:     prn_flag = OPKF_PRT_BUYING;  break;
		case DSF_OPRKIND_FPRTSELLING:    prn_flag = OPKF_PRT_SELLING; break;
		case DSF_OPRKIND_FPRTQCERT:      prn_flag = OPKF_PRT_QCERT;   break;
		case DSF_OPRKIND_FPRTSKIPBILLNO: prn_flag = OPKF_PRT_NBILLN;  break;
		case DSF_OPRKIND_FPRTVATCOLUMNS: prn_flag = OPKF_PRT_VATAX;   break;
		case DSF_OPRKIND_FPRTINVOICE:    prn_flag = OPKF_PRT_INVOICE; break;
		case DSF_OPRKIND_FPRTQCERTWITHGOODS:  prn_flag = OPKF_PRT_QCG;      break;
		case DSF_OPRKIND_FPRTSHORTORGNAME:    prn_flag = OPKF_PRT_SHRTORG;  break;
		case DSF_OPRKIND_FPRTCASHORDER:       prn_flag = OPKF_PRT_CASHORD;  break;
		case DSF_OPRKIND_FPRTSELECTPRICETYPE: prn_flag = OPKF_PRT_SELPRICE; break;
		case DSF_OPRKIND_FPRTNODISCOUNT:      prn_flag = OPKF_PRT_NDISCNT;  break;
		case DSF_OPRKIND_FPRTLADINGBILL:      prn_flag = OPKF_PRT_LADING;   break;
		case DSF_OPRKIND_FPRTMERGETI:         prn_flag = OPKF_PRT_MERGETI;  break;

		case DSF_OPRKIND_PRNORDER:
			ok = TransferData(&Data.Rec.PrnOrder, dir, rBuf, 1);
			break;
		case DSF_OPRKIND_DEFLOCID:
			ok = TransferData(&Data.Rec.DefLocID, dir, rBuf, 1);
			break;
		case DSF_OPRKIND_COUNTER:
			ok = TransferData(&Data.Counter, dir, rBuf);
			break;
		case DSF_OPRKIND_CODETEMPLATE:
			ok = TransferData(Data.CodeTemplate, sizeof(Data.CodeTemplate), dir, rBuf);
			break;
		case DSF_OPRKIND_OPCOUNTERNAME:
			{
				PPOpCounter cntr_rec;
				PPID cntr_id = Data.Rec.OpCounterID;
				if(cntr_id && SearchObject(PPOBJ_OPCOUNTER, cntr_id, &cntr_rec) > 0 &&
					cntr_rec.OwnerObjID == 0) {
					ok = TransferData(cntr_rec.Name, sizeof(cntr_rec.Name), dir, rBuf);
				}
				else
					ok = -1;
			}
			break;
		case DSF_OPRKIND_MEMOTEMPL:
			ok = TransferExtStrData(OPKEXSTR_MEMO, dir, rBuf);
			break;
		case DSF_OPRKIND_OBJ2NAME:
			ok = TransferExtStrData(OPKEXSTR_OBJ2NAME, dir, rBuf);
			break;
		case DSF_OPRKIND_DEFPRNFORM:
			ok = TransferExtStrData(OPKEXSTR_DEFPRNFORM, dir, rBuf);
			break;
		case DSF_OPRKIND_RECKONPERIOD:
			{
				if(dir == tfdDataToBuf) {
					if(Data.Rec.Flags & OPKF_RECKON)
						ok = ROX.PeriodToStr(rBuf);
					else
						ok = -1;
				}
				else if(dir == tfdBufToData)
					ok = ROX.StrToPeriod(rBuf);
				else
					ok = (PPErrCode = PPERR_INVPARAM, 0);
			}
			break;
		case DSF_OPRKIND_FRECKONAUTOPAYM: reckon_flag = ROXF_AUTOPAYM; break;
		case DSF_OPRKIND_FRECKONAUTODEBT: reckon_flag = ROXF_AUTODEBT; break;
		case DSF_OPRKIND_FRECKONCFMPAYM:  reckon_flag = ROXF_CFM_PAYM; break;
		case DSF_OPRKIND_FRECKONCFMDEBT:  reckon_flag = ROXF_CFM_DEBT; break;
		case DSF_OPRKIND_RECKONOPLIST:
			ok = TransferData(&ROX.OpList, dir, *pIter, rBuf);
			break;
		case DSF_OPRKIND_AMOUNTS:
			ok = TransferData(&Data.Amounts, dir, *pIter, rBuf);
			break;
	}
	if(flag)
		ok = TransferData(&Data.Rec.Flags, flag, dir, rBuf, 1);
	else if(prn_flag)
		ok = TransferData(&Data.Rec.PrnFlags, prn_flag, dir, rBuf, 1);
	else if(reckon_flag)
		ok = TransferData(&ROX.Flags, reckon_flag, dir, rBuf, 1);
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_OprKind::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(fldID == DSF_OPRKIND_ACCTURNLIST && pData) {
		PPDS_AccturnTempl * p_data = (PPDS_AccturnTempl *)pData;
		ok = Data.ATTmpls.insert(&p_data->Data) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int SLAPI PPDS_OprKind::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int ok = -1;
	if(fldID == DSF_OPRKIND_ACCTURNLIST) {
		if(*pIter < Data.ATTmpls.getCount()) {
			PPAccTurnTempl & r_item = Data.ATTmpls.at(*pIter);
			int r = pData->InitData(idoExtract, (long)p_item, 0);
			(*pIter)++;
			if(r > 0)
				ok = 1;
			else if(r == 0)
				ok = 0;
		}
	}
	return ok;
}
