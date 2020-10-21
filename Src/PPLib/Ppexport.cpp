// PPEXPORT.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2010, 2011, 2012, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Импорт/Экспорт данных
//
#include <pp.h>
#pragma hdrstop

//char * _ExportNames[] = {
	//"MONARCH",
	//"LP15",
	//"GLABEL"
//};

#if 0 // @v10.5.3 @obsolete-dialog-form {
DLG_E_MONRCH DIALOGEX 57, 64, 260, 69
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Подготовка данных для принтера Monarch"
FONT 8, "MS Sans Serif", 0, 0, 0x0
BEGIN
    LTEXT           "@goodsgroup",4001,10,20,60,8
    EDITTEXT        CTL_E_MONRCH_GGRP,93,20,139,13,ES_AUTOHSCROLL | ES_READONLY
    PUSHBUTTON      "",CTLSEL_E_MONRCH_GGRP,233,20,12,13,BS_BITMAP
    DEFPUSHBUTTON   "@but_ok",STDCTL_OKBUTTON,140,41,51,14
    PUSHBUTTON      "@but_cancel",STDCTL_CANCELBUTTON,195,41,51,14
END
#endif // } 0

int ProcessExportJob(const char * pJobName)
{
	int    ok = 0;
	PPID   cash_id = 0;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	if(cn_obj.SearchByName(pJobName, &cash_id, &cn_rec) > 0) {
		if(PPCashMachine::IsAsyncCMT(cn_rec.CashType)) {
			PPCashMachine * cm = PPCashMachine::CreateInstance(cash_id);
			if(cm) {
				ok = cm->AsyncOpenSession(0, 0);
				delete cm;
			}
		}
	}
   	return ok;
}

int ImportGeoCity(const char * pPath);

int ProcessImportJob(const char * pJobName)
{
	int    ok = 0;
	const  SString job_name(pJobName);
	if(job_name.HasPrefixIAscii("GEOCITY")) {
		SString left, right;
		if(job_name.Divide('@', left, right) > 0)
			ImportGeoCity(right);
	}
	else {
		PPID   cash_id = 0;
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		if(cn_obj.SearchByName(job_name, &cash_id, &cn_rec) > 0) {
			if(cn_rec.CashType == PPCMT_CASHNGROUP) // @v10.1.1
				PPCashMachine::AsyncCloseSession2(cn_rec.ID, 0); 
			else if(PPCashMachine::IsAsyncCMT(cn_rec.CashType)) {
				PPCashMachine * cm = PPCashMachine::CreateInstance(cash_id);
				if(cm) {
					ok = cm->AsyncCloseSession(0, 0);
					if(!ok)
					   	PPError();
					delete cm;
				}
			}
		}
	}
	return ok;
}
//
//
//
PPDbTableXmlExporter::BaseParam::BaseParam(uint32 sign) : Sign(sign), Flags(0), RefDbID(0)
{
}

int PPDbTableXmlExporter::BaseParam::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Sign, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, RefDbID, rBuf));
	THROW(pSCtx->Serialize(dir, FileName, rBuf));
	CATCHZOK
	return ok;
}

PPDbTableXmlExporter::PPDbTableXmlExporter()
{
}

/*virtual*/PPDbTableXmlExporter::~PPDbTableXmlExporter()
{
}

int PPDbTableXmlExporter::Run(const char * pOutFileName)
{
	int    ok = 1;
	const  long preserve_dt_def_fmt = SLS.GetConstTLA().TxtDateFmt_;
	xmlTextWriter * p_writer = 0;
	PPWait(1);
	DBTable * p_t = Init();
	THROW(p_t);
	THROW(p_writer = xmlNewTextWriterFilename(pOutFileName, 0));
	{
		SString temp_buf, fld_name;
		const BNFieldList & r_fl = p_t->GetFields();
		xmlTextWriterSetIndent(p_writer, 1);
		xmlTextWriterSetIndentTab(p_writer);
		xmlTextWriterStartDocument(p_writer, 0, "utf-8", 0);
		// XMLWriteSpecSymbEntities(writer);
		{
			DbProvider * p_dict = CurDict;
			SLS.GetTLA().TxtDateFmt_ = DATF_DMY|DATF_CENTURY;
			(temp_buf = p_t->GetTableName()).ToUtf8();
			SXml::WNode n_tbl(p_writer, temp_buf);
			DS.GetVersion().ToStr(temp_buf.Z());
			n_tbl.PutAttrib("version", temp_buf);
			{
				p_dict->GetDbSymb(temp_buf);
				temp_buf.ToUtf8();
				n_tbl.PutAttrib("dbsymb", temp_buf);
			}
			{
				S_GUID uuid;
				p_dict->GetDbUUID(&uuid);
				uuid.ToStr(S_GUID::fmtIDL, temp_buf);
				n_tbl.PutAttrib("dbuuid", temp_buf);
			}
			if(Cntr.GetTotal()) {
				temp_buf.Z().Cat(Cntr.GetTotal());
				n_tbl.PutAttrib("count", temp_buf);
			}
			while(Next() > 0) {
				SXml::WNode n_rec(p_writer, "record");
				for(uint i = 0; i < r_fl.getCount(); i++) {
					char   _buf[1024];
					const BNField & f = r_fl[i];
					{
						(fld_name = f.Name).ToUtf8();
						f.putValueToString(p_t->getDataBufConst(), _buf);
						(temp_buf = _buf).Transf(CTRANSF_INNER_TO_UTF8);
						XMLReplaceSpecSymb(temp_buf, "&<>\'");
						SXml::WNode(p_writer, fld_name, temp_buf);
					}
				}
				if(Cntr.GetTotal()) {
					PPWaitPercent(Cntr);
				}
			}
		}
	}
	CATCHZOK
	PPWait(0);
	xmlFreeTextWriter(p_writer);
	SLS.GetTLA().TxtDateFmt_ = preserve_dt_def_fmt;
	return ok;
}
//
//
//
PPDbTableXmlExportParam_TrfrBill::PPDbTableXmlExportParam_TrfrBill() : PPDbTableXmlExporter::BaseParam(0xEF00BC02)
{
	Period.Z();
}

int PPDbTableXmlExportParam_TrfrBill::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(PPDbTableXmlExporter::BaseParam::Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Period.low, rBuf));
	THROW(pSCtx->Serialize(dir, Period.upp, rBuf));
	CATCHZOK
	return ok;
}

#define GRP_BROWSE 1

class DbTableXmlExportParamDialog : public TDialog {
public:
	DbTableXmlExportParamDialog(uint dlgId) : TDialog(dlgId), Data(0)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_DBTEXP_PATH, CTL_DBTEXP_PATH, GRP_BROWSE,
			PPTXT_TITLE_SELDBTXMLEXPPATH, 0, FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfAllowNExists);
	}
	int    setDTS(const PPDbTableXmlExporter::BaseParam * pData)
	{
		int    ok = 1;
		if(pData)
			Data = *pData;
		AddClusterAssoc(CTL_DBTEXP_FLAGS, 0, Data.fReplaceIdsBySync);
		SetClusterData(CTL_DBTEXP_FLAGS, Data.Flags);
		SetupPPObjCombo(this, CTLSEL_DBTEXP_REFDBDIV, PPOBJ_DBDIV, Data.RefDbID, 0, 0);
		setCtrlString(CTL_DBTEXP_PATH, Data.FileName);
		disableCtrl(CTLSEL_DBTEXP_REFDBDIV, !BIN(Data.Flags & Data.fReplaceIdsBySync));
		return ok;
	}
	int    getDTS(PPDbTableXmlExporter::BaseParam * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_DBTEXP_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_DBTEXP_REFDBDIV, &Data.RefDbID);
		getCtrlString(sel = CTL_DBTEXP_PATH, Data.FileName);
		THROW_PP(Data.FileName.NotEmptyS(), PPERR_FILENAMENEEDED);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
protected:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_DBTEXP_FLAGS)) {
			GetClusterData(CTL_DBTEXP_FLAGS, &Data.Flags);
			disableCtrl(CTLSEL_DBTEXP_REFDBDIV, !BIN(Data.Flags & Data.fReplaceIdsBySync));
		}
		else
			return;
		clearEvent(event);
	}

	PPDbTableXmlExporter::BaseParam Data;
};

#undef GRP_BROWSE

int PPDbTableXmlExportParam_TrfrBill::Edit(PPDbTableXmlExportParam_TrfrBill * pData)
{
    int    ok = -1;
    DbTableXmlExportParamDialog * dlg = new DbTableXmlExportParamDialog(DLG_DBTEXPTRFR);
    if(CheckDialogPtrErr(&dlg)) {
		dlg->SetupCalPeriod(CTLCAL_DBTEXP_PERIOD, CTL_DBTEXP_PERIOD);
		dlg->setDTS(pData);
        SetPeriodInput(dlg, CTL_DBTEXP_PERIOD, &pData->Period);
        dlg->setCtrlString(CTL_DBTEXP_PATH, pData->FileName);
        while(ok < 0 && ExecView(dlg) == cmOK) {
			uint   sel = 0;
			if(dlg->getDTS(pData)) {
				if(!GetPeriodInput(dlg, sel = CTL_DBTEXP_PERIOD, &pData->Period)) {
					PPErrorByDialog(dlg, sel);
				}
				else {
					ok = 1;
				}
			}
        }
    }
    else
		ok = 0;
	delete dlg;
    return ok;
}
//
//
//
PPDbTableXmlExporter_Transfer::PPDbTableXmlExporter_Transfer(const PPDbTableXmlExportParam_TrfrBill & rParam) : 
	PPDbTableXmlExporter(), P_Q(0), P(rParam)
{
	P.Period.Actualize(ZERODATE);
	PPObjBill * p_bobj = BillObj;
	P_T = p_bobj ? p_bobj->trfr : 0;
}

/*virtual*/DBTable * PPDbTableXmlExporter_Transfer::Init()
{
	BExtQuery::ZDelete(&P_Q);
	if(P_T) {
		TransferTbl::Key1 k1, k1_;
		MEMSZERO(k1);
		k1.Dt = P.Period.low;
		THROW_MEM(P_Q = new BExtQuery(P_T, 1));
		P_Q->selectAll().where(daterange(P_T->Dt, &P.Period));
		k1_ = k1;
		Cntr.Init(P_Q->countIterations(0, &k1_, spGe));
		THROW(P_Q->initIteration(0, &k1, spGe));
	}
	CATCH
		P_T = 0;
		BExtQuery::ZDelete(&P_Q);
	ENDCATCH
	return P_T;
}

/*virtual*/int PPDbTableXmlExporter_Transfer::Next()
{
	int    ok = P_Q ? P_Q->nextIteration() : 0;
	if(ok > 0)
		Cntr.Increment();
	return ok;
}
//
//
//
PPDbTableXmlExporter_Bill::PPDbTableXmlExporter_Bill(const PPDbTableXmlExportParam_TrfrBill & rParam) : 
	PPDbTableXmlExporter(), P(rParam), P_Q(0)
{
	P.Period.Actualize(ZERODATE);
	PPObjBill * p_bobj = BillObj;
	P_T = p_bobj ? p_bobj->P_Tbl : 0;
}

/*virtual*/DBTable * PPDbTableXmlExporter_Bill::Init()
{
	BExtQuery::ZDelete(&P_Q);
	if(P_T) {
		BillTbl::Key1 k1, k1_;
		MEMSZERO(k1);
		k1.Dt = P.Period.low;
		THROW_MEM(P_Q = new BExtQuery(P_T, 1));
		P_Q->selectAll().where(daterange(P_T->Dt, &P.Period));
		k1_ = k1;
		Cntr.Init(P_Q->countIterations(0, &k1_, spGe));
		THROW(P_Q->initIteration(0, &k1, spGe));
	}
	CATCH
		P_T = 0;
		BExtQuery::ZDelete(&P_Q);
	ENDCATCH
	return P_T;
}

/*virtual*/int PPDbTableXmlExporter_Bill::Next()
{
	int    ok = P_Q ? P_Q->nextIteration() : 0;
	if(ok > 0)
		Cntr.Increment();
	return ok;
}
