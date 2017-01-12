// IE_BILL.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
//
//
IMPL_CMPFUNC(Sdr_Bill, i1, i2)
{
	const Sdr_Bill * p1 = (const Sdr_Bill *)i1;
	const Sdr_Bill * p2 = (const Sdr_Bill *)i2;
	int    si = CMPSIGN(p1->Date, p2->Date);
	SETIFZ(si, strcmp(p1->Code, p2->Code));
	SETIFZ(si, strcmp(p1->ID, p2->ID));
	return si;
}

IMPL_CMPFUNC(Sdr_BRow, i1, i2)
{
	const Sdr_BRow * p1 = (const Sdr_BRow *)i1;
	const Sdr_BRow * p2 = (const Sdr_BRow *)i2;
    int    si = CMPSIGN(p1->BillDate, p2->BillDate);
	SETIFZ(si, strcmp(p1->BillCode, p2->BillCode));
	SETIFZ(si, strcmp(p1->BillID, p2->BillID));
    return si;
}

IMPL_CMPFUNC(Sdr_BRow_ID, i1, i2)
{
	const Sdr_BRow * p1 = (const Sdr_BRow *)i1;
	const Sdr_BRow * p2 = (const Sdr_BRow *)i2;
	int    si = strcmp(p1->BillID, p2->BillID);
	SETIFZ(si, CMPSIGN(p1->BillDate, p2->BillDate));
	SETIFZ(si, strcmp(p1->INN, p2->INN));
    return si;
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(BILL, PPBillImpExpParam);
IMPLEMENT_IMPEXP_HDL_FACTORY(BROW, PPBillImpExpParam);

PPBillImpExpParam::PPBillImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
	Flags = 0;
	ImpOpID = 0;
	Object1SrchCode = 0;
	Object2SrchCode = 0;
}

//virtual
int PPBillImpExpParam::MakeExportFileName(const void * extraPtr, SString & rResult) const
{
	rResult = 0;
	int    ok = 1;
	int    use_ps = 0;
	int    _is_subst = 0;
	if(FileName.CmpNC(":buffer:") == 0) {
		rResult = FileName;
	}
	else {
		SString temp_buf;
		SPathStruc ps, ps_temp;
		ps.Split(FileName);
		if(ps.Drv.Empty() && ps.Dir.Empty()) {
			PPGetPath(PPPATH_OUT, temp_buf);
			ps_temp.Split(temp_buf);
			ps.Drv = ps_temp.Drv;
			ps.Dir = ps_temp.Dir;
			use_ps = 1;
		}
		{
			uint gp = 0;
			if(ps.Nam.Search("#guid", 0, 1, &gp)) {
				S_GUID guid;
				guid.Generate();
				guid.ToStr(S_GUID::fmtIDL, temp_buf = 0);
				ps.Nam.ReplaceStr("#guid", temp_buf, 0);
				use_ps = 1;
			}
		}
		if(extraPtr) {
			const PPBillPacket * p_pack = (const PPBillPacket *)extraPtr;
			PPObjBill * p_bobj = BillObj;
			if(p_bobj && p_bobj->SubstText(p_pack, ps.Nam, temp_buf) == 2) {
				ps.Nam = temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				use_ps = 1;
				ok = 100;
			}
		}
		char   cntr[128];
		uint   cn = 0;
		const uint fnl = (const uint)ps.Nam.Len();
		for(uint i = 0; i < fnl; i++)
			if(ps.Nam.C(i) == '?')
				cntr[cn++] = '0';
		if(cn) {
			SString nam;
			int    overflow = 1;
			do {
				overflow = 1;
				for(int j = cn-1; overflow && j >= 0; j--) {
					if(cntr[j] < '9') {
						cntr[j]++;
						// @v8.6.6 @fix {
						for(int z = j+1; z < (int)cn; z++)
							cntr[z] = '0';
						// } @v8.6.6 @fix
						overflow = 0;
					}
				}
				nam = 0;
				for(uint i = 0, k = 0; i < fnl; i++) {
					if(ps.Nam.C(i) == '?')
						nam.CatChar(cntr[k++]);
					else
						nam.CatChar(ps.Nam.C(i));
				}
				ps_temp.Copy(&ps, 0xffff);
				ps_temp.Nam = nam;
				ps_temp.Merge(rResult);
			} while(!overflow && fileExists(rResult));
			if(overflow)
				ok = PPSetError(PPERR_EXPFNTEMPLATEOVERFLOW, FileName);
			else
				ok = 100; // Имя создано по шаблону
		}
		else {
			if(use_ps)
				ps.Merge(rResult);
			else
				rResult = FileName;
		}
	}
	return ok;
}

//virtual
int PPBillImpExpParam::PreprocessImportFileSpec(const SString & rFileSpec, PPFileNameArray & rList)
{
	int    ok = -1;
	SPathStruc ps;
	SString wildcard, path;
	{
		SString templ, name;
		ps.Split(FileName);
		templ = ps.Nam;
		ps.Split(rFileSpec);
		name = ps.Nam;
		StrAssocArray result_list;
		if(PPObjBill::ParseText(name, templ, result_list, &wildcard) > 0) {
			wildcard.Dot().Cat(ps.Ext);
		}
		else {
			ps.Split(rFileSpec);
			(wildcard = ps.Nam).Dot().Cat(ps.Ext);
		}
		THROW(GetFilesFromSource(wildcard, 0));
		ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, path);
	}
	rList.Scan(path, wildcard);
    if(rList.getCount())
		ok = 1;
	CATCHZOK
	return ok;
}

//virtual
int PPBillImpExpParam::PreprocessImportFileName(const SString & rFileName, StrAssocArray & rResultList)
{
	rResultList.Clear();
	int    ok = 1;
	SString templ, name;
	SPathStruc ps;
	ps.Split(FileName);
	templ = ps.Nam;
	ps.Split(rFileName);
	name = ps.Nam;
	StrAssocArray result_list;
	ok = PPObjBill::ParseText(name, templ, rResultList, 0);
	return ok;
}

//virtual
int PPBillImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(ImpOpID && op_obj.Search(ImpOpID, &op_rec) > 0) {
			if(op_rec.Symb[0])
				temp_buf = op_rec.Symb;
			else
				(temp_buf = 0).Cat(ImpOpID);
		}
		else
			temp_buf = 0;
		if(temp_buf.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_IMPOPSYMB, temp_buf);
		if(Flags)
			param_list.Add(IMPEXPPARAM_BILH_FLAGS, (temp_buf = 0).Cat(Flags));
		if(Object1SrchCode.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_SRCHCODE1, (temp_buf = Object1SrchCode).Strip());
		if(Object2SrchCode.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_SRCHCODE2, (temp_buf = Object2SrchCode).Strip());
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Flags = 0;
		ImpOpID = 0;
		Object1SrchCode = 0;
		Object2SrchCode = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case IMPEXPPARAM_BILH_IMPOPSYMB:
					{
						PPID   op_id = 0;
						if(op_obj.SearchBySymb(temp_buf, &op_id) > 0)
							ImpOpID = op_id;
						else if(op_obj.Search(temp_buf.ToLong(), &op_rec) > 0)
							ImpOpID = op_rec.ID;
					}
					break;
				case IMPEXPPARAM_BILH_FLAGS:
					Flags = temp_buf.ToLong();
					break;
				case IMPEXPPARAM_BILH_SRCHCODE1:
					Object1SrchCode = temp_buf;
					break;
				case IMPEXPPARAM_BILH_SRCHCODE2:
					Object2SrchCode = temp_buf;
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	SString params;
	SString fld_name;
	SString param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_BILH, params));
	if(Direction != 0) {
		PPObjOprKind op_obj;
		PPOprKind op_rec;
		PPGetSubStr(params, IMPEXPPARAM_BILH_IMPOPSYMB, fld_name);
		if(ImpOpID && op_obj.Search(ImpOpID, &op_rec) > 0) {
			if(op_rec.Symb[0])
				param_val = op_rec.Symb;
			else {
				(param_val = 0).Cat(ImpOpID);
			}
		}
		else
			param_val = 0;
		pFile->AppendParam(pSect, fld_name, param_val, 1);

		PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE1, fld_name);
		pFile->AppendParam(pSect, fld_name, (param_val = 0).Cat(Object1SrchCode), 1);
		PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE2, fld_name);
		pFile->AppendParam(pSect, fld_name, (param_val = 0).Cat(Object2SrchCode), 1);
	}
	PPGetSubStr(params, IMPEXPPARAM_BILH_FLAGS, fld_name);
	pFile->AppendParam(pSect, fld_name, (param_val = 0).Cat(Flags), 1);
	CATCHZOK
	return ok;
}

int PPBillImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	ImpOpID = 0;
	Flags = 0;

	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_BILH, params));
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_IMPOPSYMB, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0) {
			PPObjOprKind op_obj;
			PPOprKind op_rec;
			PPID   op_id = 0;
			if(op_obj.SearchBySymb(param_val, &op_id) > 0)
				ImpOpID = op_id;
			else if(op_obj.Search(param_val.ToLong(), &op_rec) > 0)
				ImpOpID = op_rec.ID;
		}
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_FLAGS, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0) {
			Flags = param_val.ToLong();
		}
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE1, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Object1SrchCode = param_val;
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE2, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Object2SrchCode = param_val;
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}
//
//
//
BillHdrImpExpDialog::BillHdrImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPBILH)
{
}

IMPL_HANDLE_EVENT(BillHdrImpExpDialog)
{
	ImpExpParamDialog::handleEvent(event);
	if(event.isClusterClk(CTL_IMPEXP_DIR)) {
		SetupCtrls(GetClusterData(CTL_IMPEXP_DIR));
	}
	else if(event.isClusterClk(CTL_IMPEXPBILH_FLAGS)) {
		GetClusterData(CTL_IMPEXPBILH_FLAGS, &Data.Flags);
	}
	else
		return;
	clearEvent(event);
}

int BillHdrImpExpDialog::SetupCtrls(long direction)
{
	disableCtrls(direction == 0, CTL_IMPEXPBILH_FLAGS, CTLSEL_IMPEXPBILH_IMPOP, 0);
	DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 0, direction == 0);
	DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 1, direction == 0 && !(Data.Flags & PPBillImpExpParam::fImpRowsFromSameFile));
	DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 3, direction);
	return 1;
}

int BillHdrImpExpDialog::setDTS(const PPBillImpExpParam * pData)
{
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);

	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 0, PPBillImpExpParam::fImpRowsFromSameFile);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 1, PPBillImpExpParam::fImpRowsOnly); // @v8.4.8
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 2, PPBillImpExpParam::fRestrictByMatrix); // @v9.0.4
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 3, PPBillImpExpParam::fExpOneByOne); // @v9.3.10
	SetClusterData(CTL_IMPEXPBILH_FLAGS, Data.Flags);

	PPIDArray op_types;
	op_types.addzlist(PPOPT_GOODSRECEIPT, /* @v8.4.6 {*/ PPOPT_GOODSEXPEND /*}*/,
		PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_ACCTURN, PPOPT_GOODSORDER, 0L);
	SetupOprKindCombo(this, CTLSEL_IMPEXPBILH_IMPOP, Data.ImpOpID, 0, &op_types, 0);
	setCtrlString(CTL_IMPEXPBILH_SRCHCODE1, Data.Object1SrchCode);
	setCtrlString(CTL_IMPEXPBILH_SRCHCODE2, Data.Object2SrchCode);
	SetupCtrls(Data.Direction);
	return 1;
}

int BillHdrImpExpDialog::getDTS(PPBillImpExpParam * pData)
{
	int    ok = 1;
	THROW(ImpExpParamDialog::getDTS(&Data));
	GetClusterData(CTL_IMPEXPBILH_FLAGS, &Data.Flags);
	getCtrlData(CTLSEL_IMPEXPBILH_IMPOP, &Data.ImpOpID);
	getCtrlString(CTL_IMPEXPBILH_SRCHCODE1, Data.Object1SrchCode);
	getCtrlString(CTL_IMPEXPBILH_SRCHCODE2, Data.Object2SrchCode);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, 0, -1);
	ENDCATCH
	return ok;
}

class BillHdrImpExpCfgListDialog : public ImpExpCfgListDialog {
public:
	BillHdrImpExpCfgListDialog() : ImpExpCfgListDialog()
	{
		SetParams(PPFILNAM_IMPEXP_INI, PPREC_BILL, &Param, 0);
	}
private:
	virtual int EditParam(const char * pIniSection);
	PPBillImpExpParam Param;
};

int BillHdrImpExpCfgListDialog::EditParam(const char * pIniSection)
{
	int    ok = -1;
	BillHdrImpExpDialog * dlg = 0;
	PPBillImpExpParam param;
	SString ini_file_name, sect;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		int    direction = 0;
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(CheckDialogPtr(&(dlg = new BillHdrImpExpDialog()), 0));
		THROW(LoadSdRecord(PPREC_BILL, &param.InrRec));
		direction = param.Direction;
		if(!isempty(pIniSection))
			THROW(param.ReadIni(&ini_file, pIniSection, 0));
		dlg->setDTS(&param);
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&param)) {
				int is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
				if(!isempty(pIniSection))
					if(is_new)
						ini_file.RemoveSection(pIniSection);
					else
						ini_file.ClearSection(pIniSection);
				PPErrCode = PPERR_DUPOBJNAME;
				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
					ok = 1;
				else
					PPError();
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI EditBillImpExpParams(int editBill)
{
	int    ok = -1;
	PPBillImpExpParam param;
	ImpExpParamDialog * dlg = 0;
	if(editBill) {
		BillHdrImpExpCfgListDialog * p_list_dlg = new BillHdrImpExpCfgListDialog();
		THROW(CheckDialogPtr(&p_list_dlg, 1));
		ok = (ExecViewAndDestroy(p_list_dlg) == cmOK) ? 1 : -1;
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new ImpExpParamDialog(DLG_IMPEXP, 0)), 0));
		THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, dlg));
	}
	CATCHZOK
	delete dlg;
	return ok;
}
//
// Import/Export Bills
//
int SLAPI GetCliBnkSections(StringSet * pSectNames, int kind, PPCliBnkImpExpParam * pParam, uint maxBackup); // @prototype

#define GRP_EMAILLIST 1

class SelectBillImpCfgDialog : public TDialog {
public:
	SelectBillImpCfgDialog(uint dlgId, const char * pIniFileName, int import) : TDialog(dlgId)
	{
		P_IniFile = new PPIniFile(pIniFileName, 0, 1, 1);
		P_Data = 0;
		Import = import;
		SetupCalPeriod(CTLCAL_IEBILLSEL_PERIOD, CTL_IEBILLSEL_PERIOD);
		if(!Import) {
			addGroup(GRP_EMAILLIST, new EmailCtrlGroup(CTL_IEBILLSEL_MAILADR, cmEMailList));
		}
	}
	~SelectBillImpCfgDialog()
	{
		delete P_IniFile;
	}
	int    setDTS(PPBillImpExpBaseProcessBlock * pData)
	{
		P_Data = pData;
		HdrList.Clear();
		LineList.Clear();

		int    ok = 1;
		uint   id = 0;
		uint   p = 0;
		SString sect; // @vmiller
		PPBillImpExpParam param;
		THROW_PP(P_Data, PPERR_INVPARAM);
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &HdrList, Import ? 2 : 1));
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, &LineList, Import ? 2 : 1));
		THROW(LoadSdRecord(PPREC_BILL, &P_Data->BillParam.InrRec)); // @vmiller (для отображения в фильтре иконки)
		THROW(LoadSdRecord(PPREC_BROW, &P_Data->BRowParam.InrRec)); // @vmiller (для отображения в фильтре иконки)
		P_Data->BillParam.ProcessName(2, sect = P_Data->CfgNameBill); // @vmiller (для отображения в фильтре иконки)
		id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)HdrList.at(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
		SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, &HdrList, (long)id, 0);
		P_Data->BRowParam.ProcessName(2, sect = P_Data->CfgNameBRow); // @vmiller (для отображения в фильтре иконки)
		id = (LineList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)LineList.at(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
		SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BROW, &LineList, (long)id, 0);
		SetTech(); // @v9.2.10
		if(Import) {
			PPIDArray op_types;
			op_types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_ACCTURN, PPOPT_GOODSORDER, 0L);
			PPID   init_op_id = NZOR(P_Data->OpID, P_Data->BillParam.ImpOpID);
			SetupOprKindCombo(this, CTLSEL_IEBILLSEL_OP, init_op_id, 0, &op_types, 0);
			SetupPPObjCombo(this, CTLSEL_IEBILLSEL_LOC, PPOBJ_LOCATION, P_Data->LocID, 0, 0);
			// @v9.2.10 SetTech();
			// @vmiller {
			if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
				// Заполняем списком типов документов
				HdrList.Clear();
				SString buf;
				if(Import)
					PPLoadText(PPTXT_EDIIMPCMD, buf); // для импорта
				else
					PPLoadText(PPTXT_EDIEXPCMD, buf); // для экспорта
				StringSet ss(';', buf);
				for(uint i = 0; ss.get(&i, buf);) {
					uint j = 0;
					StringSet ss1(',', buf);
					ss1.get(&j, (buf = 0));
					id = buf.ToLong();
					ss1.get(&j, (buf = 0));
					ss1.get(&j, (buf = 0));
					HdrList.Add(id, buf);
				}
				P_Data->BillParam.ProcessName(2, sect = P_Data->CfgNameBill); // @vmiller (для отображения в фильтре иконки)
				id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)HdrList.at(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
				SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, &HdrList, (long)id, 0);
			}
			// } @vmiller
			SetPeriodInput(this, CTL_IEBILLSEL_PERIOD, &P_Data->Period);
			disableCtrls((P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|PPBillImpExpBaseProcessBlock::fEgaisImpExp)), CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0);
			disableCtrls((P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp), CTLSEL_IEBILLSEL_OP, 0L);
			//
			// @v8.9.0 {
			AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 0, PPBillImpExpBaseProcessBlock::fTestMode);
			AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 1, PPBillImpExpBaseProcessBlock::fDontRemoveTags);
			SetClusterData(CTL_IEBILLSEL_FLAGS, P_Data->Flags);
			// } @v8.9.0
		}
		else {
			disableCtrls(1, CTLSEL_IEBILLSEL_OP, CTLSEL_IEBILLSEL_LOC, CTL_IEBILLSEL_PERIOD, 0L);
			AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 0, PPBillImpExpBaseProcessBlock::fSignExport);
			AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 1, PPBillImpExpBaseProcessBlock::fTestMode);
			SetClusterData(CTL_IEBILLSEL_FLAGS, P_Data->Flags);
			SetupPPObjCombo(this, CTLSEL_IEBILLSEL_MAILACC, PPOBJ_INTERNETACCOUNT, P_Data->Tp.InetAccID, 0, INETACCT_ONLYMAIL);
			{
				EmailCtrlGroup::Rec grp_rec(&P_Data->Tp.AddrList);
				setGroupData(GRP_EMAILLIST, &grp_rec);
			}
			setCtrlString(CTL_IEBILLSEL_MAILSUBJ, P_Data->Tp.Subject);
		}
		CATCHZOKPPERR
		return ok;
	}
	int    getDTS()
	{
		int    ok = 1;
		uint   id = 0;
		SString sect;
		THROW_PP(P_IniFile, PPERR_INVPARAM);
		THROW_PP(P_Data, PPERR_INVPARAM);
		GetTech();
		if(Import) {
			P_Data->OpID = getCtrlLong(CTLSEL_IEBILLSEL_OP);
			P_Data->LocID = getCtrlLong(CTLSEL_IEBILLSEL_LOC);
			SETIFZ(P_Data->LocID, LConfig.Location);
			THROW_PP(P_Data->OpID || (P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp), PPERR_INVOPRKIND);
			GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags);
			THROW(GetPeriodInput(this, CTL_IEBILLSEL_PERIOD, &P_Data->Period));
			GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags); // @v8.9.0
		}
		else {
			GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags);
			getCtrlData(CTLSEL_IEBILLSEL_MAILACC, &P_Data->Tp.InetAccID);
			{
				EmailCtrlGroup::Rec grp_rec;
				getGroupData(GRP_EMAILLIST, &grp_rec);
				P_Data->Tp.AddrList = grp_rec.AddrList;
			}
			getCtrlString(CTL_IEBILLSEL_MAILSUBJ, P_Data->Tp.Subject);
		}
		if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
			PPCliBnkImpExpParam cb_param;
			LoadSdRecord(PPREC_CLIBNKDATA, &cb_param.InrRec);
			getCtrlData(CTLSEL_IEBILLSEL_BILL, &(id = 0));
			THROW_PP(id, PPERR_INVBILLIMPEXPCFG);
			HdrList.Get(id, sect);
			cb_param.ProcessName(1, sect);
			P_Data->CfgNameBill = sect;
		}
		else if(!Import || !(P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|PPBillImpExpBaseProcessBlock::fEgaisImpExp))) {
			P_Data->BillParam.Init(Import);
			P_Data->BRowParam.Init(Import);
			THROW(LoadSdRecord(PPREC_BILL, &P_Data->BillParam.InrRec));
			THROW(LoadSdRecord(PPREC_BROW, &P_Data->BRowParam.InrRec));
			if(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)) {
				getCtrlData(CTLSEL_IEBILLSEL_BILL, &(id = 0));
				THROW_PP(id, PPERR_INVBILLIMPEXPCFG);
				HdrList.Get(id, sect);
				if(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)) {
					if(!sect.CmpPrefix("DLL_", 1))
						P_Data->BillParam.BaseFlags |= PPImpExpParam::bfDLL;
					P_Data->BillParam.ProcessName(1, sect);
					THROW(P_Data->BillParam.ReadIni(P_IniFile, sect, 0));
					id = getCtrlLong(CTLSEL_IEBILLSEL_BROW);
					if(!(P_Data->BillParam.BaseFlags & PPImpExpParam::bfDLL)) {
						if(id || GetOpType(P_Data->OpID) != PPOPT_ACCTURN) { // @v8.4.10
							THROW_PP(id, PPERR_INVBILLIMPEXPCFG);
							LineList.Get(id, sect);
							P_Data->BRowParam.ProcessName(1, sect);
							THROW(P_Data->BRowParam.ReadIni(P_IniFile, sect, 0));
						}
					}
				}
				else {
					P_Data->BillParam.BaseFlags |= PPImpExpParam::bfDLL;
					P_Data->BillParam.EDIDocType = id;
					P_Data->BillParam.Name = sect;
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		// @vmiller {
		if(event.isClusterClk(CTL_IEBILLSEL_TECH)) {
			if(P_Data) {
				uint   id = 0, p = 0;
				SString sect;
				SString buf;
				PPBillImpExpParam param;
				GetTech();
				disableCtrls(0, CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0L);
				disableCtrls((P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|PPBillImpExpBaseProcessBlock::fEgaisImpExp)), CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0L);
				if(P_Data->Flags & (PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fPaymOrdersExp))
					disableCtrl(CTLSEL_IEBILLSEL_BROW, 1);
				disableCtrls((P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp), CTLSEL_IEBILLSEL_OP, 0L);
				if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
					// Заполняем списком типов документов
					HdrList.Clear();
					PPLoadText((Import ? PPTXT_EDIIMPCMD : PPTXT_EDIEXPCMD), buf);
					StringSet ss(';', buf);
					for(uint i = 0; ss.get(&i, buf);) {
						uint j = 0;
						StringSet ss1(',', buf);
						ss1.get(&j, (buf = 0));
						id = buf.ToLong();
						ss1.get(&j, (buf = 0));
						ss1.get(&j, (buf = 0));
						HdrList.Add(id, buf);
					}
					P_Data->BillParam.ProcessName(2, sect = P_Data->CfgNameBill); // @vmiller (для отображения в фильтре иконки)
					id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)HdrList.at(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
					SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, &HdrList, (long)id, 0);
				}
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp) {
					;
				}
				// @v9.2.10 {
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
					HdrList.Clear();
					PPCliBnkImpExpParam cb_param;
					StringSet sections;
					GetCliBnkSections(&sections, Import ? 2 : 1, &cb_param, 0);
					for(uint i = 0; sections.get(&i, buf);) {
						cb_param.ProcessName(2, sect = buf);
						HdrList.Add(i+1, sect);
					}
					SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, &HdrList, (long)id, 0);
				}
				// } @v9.2.10
				else {
					HdrList.Clear();
					if(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &HdrList, Import ? 2 : 1) > 0) {
						id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)HdrList.at(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
						SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, &HdrList, (long)id, 0);
					}
				}
			}
		}
		// } @vmiller
		else if(event.isCbSelected(CTLSEL_IEBILLSEL_BILL)) {
			long   hdr_id = getCtrlLong(CTLSEL_IEBILLSEL_BILL);
			SString sect;
			HdrList.Get(hdr_id, sect);
			if(sect.NotEmpty()) {
				uint p = 0;
				if(LineList.SearchByText(sect, 1, &p) > 0)
					setCtrlLong(CTLSEL_IEBILLSEL_BROW, LineList.at(p).Id);
				if(P_Data && P_IniFile) {
					P_Data->BillParam.ProcessName(1, sect);
					P_Data->BillParam.ReadIni(P_IniFile, sect, 0);
					if(P_Data->BillParam.ImpOpID)
						setCtrlLong(CTLSEL_IEBILLSEL_OP, P_Data->BillParam.ImpOpID);
				}
			}
		}
		else if(event.isCbSelected(CTLSEL_IEBILLSEL_BROW)) {
			long row_id = getCtrlLong(CTLSEL_IEBILLSEL_BROW);
			if(!getCtrlLong(CTLSEL_IEBILLSEL_BILL)) {
				SString sect;
				LineList.Get(row_id, sect);
				if(sect.NotEmpty()) {
					uint p = 0;
					if(HdrList.SearchByText(sect, 1, &p) > 0)
						setCtrlLong(CTLSEL_IEBILLSEL_BILL, HdrList.at(p).Id);
				}
			}
		}
		else
			return;
		clearEvent(event);
	}
	void SetTech()
	{
		ushort v = 0;
		if(P_Data) {
			if(Import) {
				if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fUhttImport)
					v = 1;
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)
					v = 2;
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)
					v = 3;
				else
					v = 0;
			}
			else {
				// @v9.2.10 {
				if(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fEdiImpExp)
					DisableClusterItem(CTL_IEBILLSEL_TECH, 1, 1);
				if(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fEgaisImpExp)
					DisableClusterItem(CTL_IEBILLSEL_TECH, 2, 1);
				if(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fPaymOrdersExp)
					DisableClusterItem(CTL_IEBILLSEL_TECH, 3, 1);
				// } @v9.2.10
				if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)
					v = 1;
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)
					v = 2;
				// @v9.2.10 {
				else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp && !(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fPaymOrdersExp))
					v = 3;
				// } @v9.2.10
				else
					v = 0;
			}
		}
		setCtrlUInt16(CTL_IEBILLSEL_TECH, v);
	}
	void GetTech()
	{
		if(P_Data) {
			ushort v = getCtrlUInt16(CTL_IEBILLSEL_TECH);
			if(Import) {
				P_Data->Flags &= ~(PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fEgaisImpExp|
					PPBillImpExpBaseProcessBlock::fUhttImport|PPBillImpExpBaseProcessBlock::fPaymOrdersExp);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fUhttImport,   v == 1);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fEdiImpExp,    v == 2);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fEgaisImpExp,  v == 3);
			}
			else {
				P_Data->Flags &= ~(PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fPaymOrdersExp);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fEdiImpExp, v == 1);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fEgaisImpExp, v == 2);
				SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fPaymOrdersExp, v == 3);
			}
		}
	}
	PPIniFile * P_IniFile;
	int    Import;
	PPBillImpExpBaseProcessBlock * P_Data; // @notowned
	StrAssocArray HdrList;
	StrAssocArray LineList;
};
//
//
//
PPBillImpExpBaseProcessBlock::TransmitParam::TransmitParam()
{
	InetAccID = 0;
}

void PPBillImpExpBaseProcessBlock::TransmitParam::Reset()
{
	InetAccID = 0;
	AddrList.Clear();
	Subject = 0;
}

PPBillImpExpBaseProcessBlock::SearchBlock::SearchBlock()
{
	Dt = ZERODATE;
	SurveyDays = 7;
}

PPBillImpExpBaseProcessBlock::PPBillImpExpBaseProcessBlock()
{
	P_BObj = BillObj;
	DisabledOptions = 0;
	Reset();
}

PPBillImpExpBaseProcessBlock & PPBillImpExpBaseProcessBlock::Reset()
{
	// Эта функция не сбрасывает DisabledOptions поскольку поле транзиентное и устанавливается
	// на прикладном уровне выше вызова Reset()
	Flags = 0;
	OpID = 0;
	LocID = 0;
	PosNodeID = 0;
	Period.SetZero();
	Tp.Reset();
	return *this;
}

int SLAPI PPBillImpExpBaseProcessBlock::SerializeParam(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	int    is_ver_ok = 0;
	PPVersionInfo vi = DS.GetVersionInfo();
	SVerT cur_ver = vi.GetVersion();
	SVerT ser_ver;
	if(dir > 0) {
		if(BillParam.Name.NotEmpty())
			CfgNameBill = BillParam.Name;
		if(BRowParam.Name.NotEmpty())
			CfgNameBRow = BRowParam.Name;
		THROW_SL(cur_ver.Serialize(dir, rBuf, pCtx));
		is_ver_ok = 1;
	}
	else if(dir < 0) {
		const size_t preserve_offs = rBuf.GetRdOffs();
		int    mj = 0, mn = 0, rz = 0;
		THROW_SL(ser_ver.Serialize(dir, rBuf, pCtx));
		ser_ver.Get(&mj, &mn, &rz);
		if((mj >= 8 && mj <= 20) && (mn >= 0 && mn <= 12) && (rz >= 0 && rz <= 16)) {
			is_ver_ok = 1;
		}
		else {
            rBuf.SetRdOffs(preserve_offs);
		}
	}
	THROW_SL(pCtx->Serialize(dir, CfgNameBill, rBuf));
	THROW_SL(pCtx->Serialize(dir, CfgNameBRow, rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pCtx->Serialize(dir, OpID, rBuf));
	THROW_SL(pCtx->Serialize(dir, LocID, rBuf));
	THROW_SL(pCtx->Serialize(dir, Period.low, rBuf));
	THROW_SL(pCtx->Serialize(dir, Period.upp, rBuf));
	if(dir > 0 || is_ver_ok) {
        THROW_SL(pCtx->Serialize(dir, Tp.InetAccID, rBuf));
        THROW_SL(pCtx->Serialize(dir, Tp.AddrList, rBuf));
        THROW_SL(pCtx->Serialize(dir, Tp.Subject, rBuf));
	}
	CATCHZOK
	return ok;
}

int PPBillImpExpBaseProcessBlock::Select(int import)
{
	int    ok = -1;
	SString ini_file_name;
	SelectBillImpCfgDialog * dlg = 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		//PPIniFile ini_file(ini_file_name, 0, 1, 1);
		BillParam.Direction = BIN(import);
		BRowParam.Direction = BIN(import);
		THROW(CheckDialogPtr(&(dlg = new SelectBillImpCfgDialog((import ? DLG_RUNIE_BILL_IMP : DLG_RUNIE_BILL_EXP), ini_file_name, import)), 0));
		THROW(dlg->setDTS(this));
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS())
				ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

#if 0 // {

int SLAPI SelectBillImpExpCfgs(PPBillImpExpParam * pBillParam, PPBillImpExpParam * pBRowParam,
	PPID * pOpID, PPID * pLocID, int import)
{
	int    ok = -1, valid_data = 0;
	uint   p = 0;
	uint   id = 0;
	SString ini_file_name;
	PPIDArray op_types;
	StrAssocArray list1, list2;
	PPBillImpExpParam param;
	TDialog * p_dlg = 0;
	THROW_PP(pBillParam && pBRowParam, PPERR_INVPARAM);
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		pBillParam->Direction = BIN(import);
		pBRowParam->Direction = BIN(import);
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &list1, import ? 2 : 1));
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, &list2, import ? 2 : 1));
		THROW(CheckDialogPtr(&(p_dlg = new SelectBillImpCfgDialog(list1, list2, pBillParam, &ini_file)), 0));
		id = (list1.SearchByText(pBillParam->Name, 1, &p) > 0) ? (uint)list1.at(p).Id : 0;
		SetupStrAssocCombo(p_dlg, CTLSEL_IEBILLSEL_BILL, &list1, (long)id, 0);
		id = (list2.SearchByText(pBRowParam->Name, 1, &p) > 0) ? (uint)list2.at(p).Id : 0;
		SetupStrAssocCombo(p_dlg, CTLSEL_IEBILLSEL_BROW, &list2, (long)id, 0);
		if(import) {
			op_types.addzlist(PPOPT_GOODSRECEIPT, /* @v8.4.6 {*/ PPOPT_GOODSEXPEND /*}*/,
				PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_ACCTURN, PPOPT_GOODSORDER, 0L);
			PPID   init_op_id = (pOpID && *pOpID) ? *pOpID : param.ImpOpID;
			SetupOprKindCombo(p_dlg, CTLSEL_IEBILLSEL_OP, init_op_id, 0, &op_types, 0);
			SetupPPObjCombo(p_dlg, CTLSEL_IEBILLSEL_LOC, PPOBJ_LOCATION, pLocID ? *pLocID : 0, 0, 0);
		}
		else
			p_dlg->disableCtrls(1, CTLSEL_IEBILLSEL_OP, CTLSEL_IEBILLSEL_LOC, 0L);
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			PPID   op_id  = import ? p_dlg->getCtrlLong(CTLSEL_IEBILLSEL_OP)  : 0;
			PPID   loc_id = import ? p_dlg->getCtrlLong(CTLSEL_IEBILLSEL_LOC) : 0;
			SString sect;
			if(!import || op_id) {
				p_dlg->getCtrlData(CTLSEL_IEBILLSEL_BILL, &id);
				if(id) {
					list1.Get(id, sect);
					pBillParam->ProcessName(1, sect);
					pBillParam->ReadIni(&ini_file, sect, 0);
					p_dlg->getCtrlData(CTLSEL_IEBILLSEL_BROW, &id);
					if(id) {
						list2.Get(id, sect);
						pBRowParam->ProcessName(1, sect);
						pBRowParam->ReadIni(&ini_file, sect, 0);
						ASSIGN_PTR(pOpID, op_id);
						ASSIGN_PTR(pLocID, loc_id ? loc_id : LConfig.Location);
						ok = valid_data = 1;
					}
					else
						PPError(PPERR_INVBILLIMPEXPCFG);
				}
				else
					PPError(PPERR_INVBILLIMPEXPCFG);
				// @vmiller {
				if(p_dlg->getCtrlLong(CTL_IEBILLSEL_SIGN))
					pBillParam->Flags |= PPBillImpExpParam::fSignBill;
				// } @vmiller
			}
			else
				PPError(PPERR_INVOPRKIND);
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

#endif // } 0

#if 0 // @v9.1.1 @unused {

class UnknownGoodsSubstDialog : public PPListDialog {
public:
	UnknownGoodsSubstDialog() : PPListDialog(DLG_LBXSEL, CTL_LBXSEL_LIST)
	{
		SString subtitle;
		PPLoadText(PPTXT_NOTIMPORTEDGOODS, subtitle);
		setSubTitle(subtitle);
		showCtrl(STDCTL_INSBUTTON,  0);
		showCtrl(STDCTL_DELBUTTON,  0);
		showCtrl(CTL_LBXSEL_UPBTN,   0);
		showCtrl(CTL_LBXSEL_DOWNBTN, 0);
		updateList(-1);
	}
	int    setDTS(SdrBillRowArray * pData);
	int    getDTS(SdrBillRowArray * pRowsAry, PPIDArray * pSubsGoodsList);
private:
	virtual int editItem(long pos, long id);
	virtual int setupList();
	LAssocArray     SubstGoodsList;
	SdrBillRowArray Data;
};

int UnknownGoodsSubstDialog::setupList()
{
	int    ok = -1;
	SString goods_name;
	for(uint i = 0; i < Data.getCount(); i++) {
		if(SubstGoodsList.Search(i + 1, 0, 0) <= 0) {
			const Sdr_BRow * p_brow = &Data.at(i);
			goods_name = p_brow->GoodsName;
			if(!goods_name.NotEmptyS())
				(goods_name = p_brow->Barcode).CatChar(' ').Cat(p_brow->Quantity, MKSFMTD(10, 2, 0));
			THROW(addStringToList(i + 1, (const char*)goods_name));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int UnknownGoodsSubstDialog::setDTS(SdrBillRowArray * pData)
{
	int    ok = 1;
	if(pData)
		Data.copy(*pData);
	updateList(-1);
	return ok;
}

int UnknownGoodsSubstDialog::getDTS(SdrBillRowArray * pData, PPIDArray * pSubstGoodsList)
{
	int    ok = 1;
	THROW_PP(pData && pSubstGoodsList, PPERR_INVPARAM);
	pData->freeAll();
	pSubstGoodsList->freeAll();
	for(uint i = 0; i < SubstGoodsList.getCount(); i++) {
		const  LAssoc & r_item = SubstGoodsList.at(i);
		THROW_SL(pSubstGoodsList->add(r_item.Val /* goods_id */));
		THROW_SL(pData->insert(&Data.at(r_item.Key - 1)));
	}
	CATCHZOK
	return ok;
}

int UnknownGoodsSubstDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPID   goods_id = 0;
	if(id && SelectGoods(goods_id) > 0) {
		char   goods_name[128];
		SubstGoodsList.Add(id, goods_id, 0);
		STRNSCPY(goods_name, Data.at((uint)id - 1).GoodsName);
		for(uint p = 0; Data.lsearch(goods_name, &p, PTR_CMPFUNC(Pchar), offsetof(Sdr_BRow, GoodsName)) > 0; p++) {
			if((p + 1) != id)
				SubstGoodsList.Add(p + 1, goods_id, 0);
		}
		ok = 1;
	}
	return ok;
}

#endif // } 0 @v9.1.1 @unused
//
//
//
PPBillImporter::PPBillImporter()
{
	P_Cc = 0;
	P_Btd = 0;
	Init();
}

PPBillImporter::~PPBillImporter()
{
	SString path;
	PPGetFilePath(PPPATH_LOG, PPFILNAM_IMPEXP_LOG, path);
	Logger.Save(path, 0);
	delete P_Btd;
	delete P_Cc;
}

int SLAPI PPBillImporter::Init()
{
	PPBillImpExpBaseProcessBlock::Reset();
	AccSheetID = 0;
	LineIdSeq = 0;
	Bills.freeAll();
	BillsRows.freeAll();
	Logger.Clear();
	return 1;
}

int SLAPI PPBillImporter::LoadConfig(int import)
{
	int    ok = 1;
	SString name;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		BillParam.Init(import);
		BRowParam.Init(import);
		THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		name = CfgNameBill;
		if(Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
			BillParam.BaseFlags |= PPImpExpParam::bfDLL;
			BillParam.Name = CfgNameBill;
		}
		else {
			BillParam.ProcessName(2, name); // @vmiller
			// @vmiller (impexp) {
			if(name.CmpPrefix("DLL_", 1) == 0) {
				BillParam.BaseFlags |= PPImpExpParam::bfDLL;
			}
			// } @vmiller (impexp)
			BillParam.ProcessName(1, name);
			THROW(BillParam.ReadIni(&ini_file, name, 0));
			name = CfgNameBRow;
			BRowParam.ProcessName(2, name);
			BRowParam.ProcessName(1, name);
			THROW(BRowParam.ReadIni(&ini_file, name, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::InitUhttImport(PPID opID, PPID locID, PPID posNodeID)
{
	int    ok = 1;
	Init();
	OpID = opID;
	LocID = NZOR(locID, LConfig.Location);
	PosNodeID = posNodeID;
	Flags |= fUhttImport;
	SETFLAG(Flags, fCreateCc, PosNodeID);
	if(Flags & fCreateCc) {
		SETIFZ(P_Cc, new CCheckCore);
	}
	THROW(OpID && LocID);
	THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
	THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
	{
		PPOprKind op_rec;
		if(GetOpData(OpID, &op_rec) > 0)
			AccSheetID = op_rec.AccSheetID;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::Init(PPBillImpExpParam * pBillParam, PPBillImpExpParam * pBRowParam, PPID opID, PPID locID)
{
	//int    ok = -1;
	int    ok = 1; //@vmiller
	Init();
	OpID = opID;
	LocID = NZOR(locID, LConfig.Location);
	RVALUEPTR(BillParam, pBillParam);
	RVALUEPTR(BRowParam, pBRowParam);
	if(!pBillParam || !pBRowParam || !OpID) {
		if(!pBillParam)
			THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		if(!pBRowParam)
			THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		THROW(ok = Select(1));
		if(ok > 0) {
			PPOprKind op_rec;
			if(GetOpData(OpID, &op_rec) > 0)
				AccSheetID = op_rec.AccSheetID;
		}
	}
	// BillParam.FileName.Transf(CTRANSF_INNER_TO_OUTER); @Данная операция производится в PPImpExp::OpenFile
	// BRowParam.FileName.Transf(CTRANSF_INNER_TO_OUTER); @Данная операция производится в PPImpExp::OpenFile
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::Helper_EnsurePersonArticle(PPID psnID, PPID accSheetID, PPID psnKindID, PPID * pArID)
{
	int    ok = 1;
	PPID   ar_id = 0;
	ArticleTbl::Rec ar_rec;
	if(ArObj.P_Tbl->SearchObjRef(accSheetID, psnID, &ar_rec) > 0) {
		ar_id = ar_rec.ID;
	}
	else {
		if(PsnObj.P_Tbl->IsBelongToKind(psnID, psnKindID) <= 0)
			THROW(PsnObj.P_Tbl->AddKind(psnID, psnKindID, 0) > 0);
		THROW(ArObj.CreateObjRef(&(ar_id = 0), accSheetID, psnID, 0, 0) > 0);
	}
	ASSIGN_PTR(pArID, ar_id);
	CATCHZOK
	return ar_id;
}

int SLAPI PPBillImporter::RunUhttImport()
{
	int    ok = -1;
	const  PPID loc_id = NZOR(LocID, LConfig.Location);
	int32  uhtt_loc_id = 0;
	SString clb_buf, serial_buf;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	PPOprKind op_rec;
	PPObjAccSheet acs_obj;
	LocationTbl::Rec loc_rec;
	UhttLocationPacket uhtt_loc_pack;
	PPUhttClient uhtt_cli;
	THROW(GetOpData(OpID, &op_rec) > 0);
	THROW(uhtt_cli.Auth());
	THROW_PP(LocObj.Fetch(loc_id, &loc_rec) > 0, PPERR_LOCSYMBUNDEF);
	THROW_PP_S(loc_rec.Code[0], PPERR_LOCSYMBUNDEF, loc_rec.Name);
	THROW(uhtt_cli.GetLocationByCode(loc_rec.Code, uhtt_loc_pack) > 0);
	uhtt_loc_id = uhtt_loc_pack.ID;
	{
		TSCollection <UhttBillPacket> result_list;
		UhttBillFilter uhtt_filt;
		PPIDArray psn_list, ar_list, kind_list;
		//DateRange period;
		//period.Set(plusdate(getcurdate_(), -1), ZERODATE);
		//uhtt_filt.Period = period;
		uhtt_filt.Since = plusdate(getcurdate_(), -1);
		uhtt_filt.OpSymb = "DRAFTORDER";
		uhtt_filt.LocID = uhtt_loc_id;
		THROW(uhtt_cli.GetBill(uhtt_filt, result_list));
		for(uint i = 0; i < result_list.getCount(); i++) {
			const UhttBillPacket * p_uhtt_pack = result_list.at(i);
			if(p_uhtt_pack) {
				BillTbl::Rec same_bill_rec;
				if(!p_uhtt_pack->Uuid.IsZero() && P_BObj->SearchByGuid(p_uhtt_pack->Uuid, &same_bill_rec) > 0) {
					// PPTXT_LOG_IMPBILLFOUND "Документ '@bill' уже принят в базу данных (идентифицирован по UUID)"
					PPFormatT(PPTXT_LOG_IMPBILLFOUND, &msg_buf, same_bill_rec.ID);
					Logger.Log(msg_buf);
				}
				else {
					PPBillPacket pack;
					PPPersonPacket psn_pack;
					PPID   dlvr_loc_id = 0;
					PPLocationPacket dlvr_loc_pack;
					PPTransaction tra(1);
					THROW(tra);
					THROW(pack.CreateBlank(OpID, 0, loc_id, 0));
					pack.Rec.Dt = p_uhtt_pack->Dtm;
					if(!p_uhtt_pack->Uuid.IsZero()) {
						pack.SetGuid(p_uhtt_pack->Uuid);
					}
					if(p_uhtt_pack->DlvrLocID) {
						THROW(uhtt_cli.GetLocationByID(p_uhtt_pack->DlvrLocID, uhtt_loc_pack));
						uhtt_cli.ConvertLocationPacket(uhtt_loc_pack, dlvr_loc_pack);
						LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, dlvr_loc_pack.Code, &dlvr_loc_id, &dlvr_loc_pack);
					}
					if(op_rec.AccSheetID) {
   						PPAccSheet acs_rec;
						PPID   ar_id = 0;
						PPID   psn_id = 0;
						THROW(acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0);
						if(acs_rec.Assoc == PPOBJ_PERSON) {
							const  PPID psn_kind_id = acs_rec.ObjGroup;
							if(p_uhtt_pack->Contractor.ID) {
								//
								// С пакетом документа пришла информация о контрагенте
								//
								ar_list.clear();
								//
								// Попытка идентификации контрагента по ИНН
								//
								(temp_buf = p_uhtt_pack->Contractor.INN).Strip();
								THROW(PsnObj.GetListByRegNumber(PPREGT_TPID, 0, temp_buf, psn_list));
								if(psn_list.getCount()) {
									psn_id = psn_list.get(0);
									THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
								}
								if(!ar_id) {
									//
									// Попытка идентификации контрагента по коду персоналии Universe-HTT
									//
									p_uhtt_pack->Contractor.GetUhttContragentCode(temp_buf);
									THROW(PsnObj.GetListByRegNumber(PPREGT_UHTTCLID, 0, temp_buf, psn_list));
									if(psn_list.getCount()) {
										psn_id = psn_list.get(0);
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
								if(!ar_id) {
									//
									// Не нашли аналога персоналии в БД.
									// Создаем новую персоналию.
									//
									psn_pack.destroy();
									if(uhtt_cli.ConvertPersonPacket(p_uhtt_pack->Contractor, psn_kind_id, psn_pack) > 0) {
										THROW(PsnObj.PutPacket(&psn_id, &psn_pack, 0));
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
							}
							else {
								if(dlvr_loc_id) {
									PersonTbl::Rec dlvr_loc_owner_rec;
									if(dlvr_loc_pack.OwnerID && PsnObj.Search(dlvr_loc_pack.OwnerID, &dlvr_loc_owner_rec) > 0) {
										psn_id = dlvr_loc_owner_rec.ID;
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
								if(!psn_id) {
									if(!dlvr_loc_pack.IsEmptyAddress()) {
										PersonTbl::Rec temp_psn_rec;
										temp_buf = "Anonymous client Universe-HTT";
										kind_list.clear();
										psn_pack.destroy();
										kind_list.add(psn_kind_id);
										if(PsnObj.SearchFirstByName(temp_buf, &kind_list, 0, &temp_psn_rec) > 0) {
											psn_id = temp_psn_rec.ID;
											THROW(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0);
											dlvr_loc_pack.OwnerID = psn_id;
											psn_pack.AddDlvrLoc(dlvr_loc_pack);
										}
										else {
											temp_buf.CopyTo(psn_pack.Rec.Name, sizeof(psn_pack.Rec.Name));
											psn_pack.Rec.Status = PPPRS_LEGAL;
											psn_pack.Kinds.addUnique(psn_kind_id);
											psn_pack.AddDlvrLoc(dlvr_loc_pack);
										}
										THROW(PsnObj.PutPacket(&psn_id, &psn_pack, 0));
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
							}
							if(ar_id) {
								PPBillPacket::SetupObjectBlock sob;
								THROW(pack.SetupObject(ar_id, sob));
							}
							if(psn_id && !dlvr_loc_pack.IsEmptyAddress()) {
								//
								// Если с сервера был получен адрес доставки, то необходимо убедиться, что
								// адрес включен в список адресов персоналии и установить этот адрес во фрахт документа.
								//
								uint   dlvr_loc_idx = 0;
								PPLocationPacket _loc_pack;
								psn_pack.destroy();
								THROW(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0);
								for(uint j = 0; !dlvr_loc_idx && psn_pack.EnumDlvrLoc(&j, &_loc_pack);) {
									if(strcmp(_loc_pack.Code, dlvr_loc_pack.Code) == 0) {
										dlvr_loc_idx = j;
									}
								}
								if(!dlvr_loc_idx) {
									PPID   _psn_id = psn_pack.Rec.ID;
									dlvr_loc_pack.ID = 0;
									psn_pack.AddDlvrLoc(dlvr_loc_pack);
									THROW(PsnObj.PutPacket(&_psn_id, &psn_pack, 0));
									if(LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, dlvr_loc_pack.Code, &dlvr_loc_id, &dlvr_loc_pack) > 0) {
										PPFreight freight;
										freight.DlvrAddrID = dlvr_loc_id;
										pack.SetFreight(&freight);
									}
								}
								else {
									PPFreight freight;
									freight.DlvrAddrID = dlvr_loc_id;
									pack.SetFreight(&freight);
								}
							}
						}
					}
					if(!dlvr_loc_id && !dlvr_loc_pack.IsEmptyAddress()) {
						dlvr_loc_pack.Flags |= LOCF_STANDALONE;
						THROW(LocObj.PutPacket(&dlvr_loc_id, &dlvr_loc_pack, 0));
						{
							PPFreight freight;
							freight.DlvrAddrID = dlvr_loc_id;
							pack.SetFreight(&freight);
						}
					}
					for(uint j = 0; j < p_uhtt_pack->Items.getCount(); j++) {
						const UhttBillPacket::BillItem & r_item = p_uhtt_pack->Items.at(j);
						UhttGoodsPacket uhtt_goods_pack;
						PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
						clb_buf = 0;
						serial_buf = 0;
						if(uhtt_cli.ResolveGoodsByUhttID(r_item.GoodsID, &uhtt_goods_pack, &ti.GoodsID, 0) > 0) {
							ti.Quantity_ = fabs(r_item.Quantity);
							ti.Price = fabs(r_item.Price);
							P_BObj->SetupImportedPrice(&pack, &ti, PPObjBill::sipfAllowZeroPrice);
							THROW(pack.LoadTItem(&ti, clb_buf, serial_buf));
						}
						else {
							// @log
						}
					}
					pack.SetQuantitySign(-1);
					THROW(P_BObj->__TurnPacket(&pack, 0, 0, 0));
					ok = 1;
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					if(Flags & fCreateCc && P_Cc && pack.GetTCount()) {
						PPObjCashNode cn_obj;
						PPCashNode cn_rec;
						if(cn_obj.Search(PosNodeID, &cn_rec) > 0) {
							CCheckPacket cc_pack;
							cc_pack.Init();
							cc_pack.Rec.CashID = PosNodeID;
							// @v8.4.7 @fix (отложенный чек не должен быть привязан к сессии) cc_pack.Rec.SessID = cn_rec.CurSessID;
							cc_pack.Rec.Dt = p_uhtt_pack->Dtm;
							cc_pack.Rec.Tm = p_uhtt_pack->Dtm;
							SETIFZ(cc_pack.Rec.Dt, getcurdate_());
							SETIFZ(cc_pack.Rec.Tm, getcurtime_());
							GetCurUserPerson(&cc_pack.Rec.UserID, 0);
							cc_pack.Rec.Flags |= (CCHKF_DELIVERY|CCHKF_SYNC|CCHKF_SUSPENDED|CCHKF_IMPORTED);
							{
								CCheckTbl::Rec last_chk_rec;
								if(P_Cc->GetLastCheckByCode(PosNodeID, &last_chk_rec) > 0)
									cc_pack.Rec.Code = last_chk_rec.Code + 1;
							}
							if(dlvr_loc_id) {
								cc_pack.Ext.AddrID = dlvr_loc_id;
							}
							{
								temp_buf = "UHTT";
								if(pack.Rec.Memo[0])
									temp_buf.Space().Cat(pack.Rec.Memo);
								temp_buf.CopyTo(cc_pack.Ext.Memo, sizeof(cc_pack.Ext.Memo));
							}
							PPTransferItem * p_ti;
							for(uint ln = 0; pack.EnumTItems(&ln, &p_ti);) {
								const double _qtty = fabs(p_ti->Quantity_);
								THROW(cc_pack.InsertItem(p_ti->GoodsID, _qtty, p_ti->NetPrice(), 0));
							}
							cc_pack.SetupAmount(0, 0);
							THROW(P_Cc->TurnCheck(&cc_pack, 0));
							ok += 100;
							//BillTbl::Rec CCheckExtTbl::Rec
						}
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

// @vmiller
int SLAPI PPBillImporter::RunEDIImport()
{
	int    ok = 1, r = 1;
	//
	// Если импортируем через EDI, то необходимо импортировать данные для всех провайдеров, указанных
	// в impexp.ini для выбранного типа документов
	//
	PPBillImpExpParam bill_param;
	SString ini_file_name, temp_buf;
	StringSet sects;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file (ini_file_name, 0, 1, 1);
		bill_param = BillParam; // Запомним начальные настройки конфигурации
		if(ini_file.GetSections(&sects) > 0) {
			uint sect_pos = 0;
			SString str, type_str;
			// Импортируем документы, пока получаем настройки для импорта данного типа документа (BillParam.Name) (перечисление в PPTXT_EDIIMPCMD)
			while(sects.get(&sect_pos, str) > 0) {
				uint   start_pos = 0;
				uint   end_pos = 0;
				(type_str = BillParam.Name).Transf(CTRANSF_INNER_TO_OUTER);
				if(str.Search("IMP@BILL@DLL_", 0, 1, &(start_pos = 0)) > 0 && str.Search(BillParam.Name, 0, 1, &(start_pos = 0)) > 0) {
					if(!BillParam.ReadIni(&ini_file, str, 0)) {
						PPSetError(PPERR_IMPEXPCFGRDFAULT, str);
						Logger.LogLastError();
					}
					else {
						r = ReadData();
						if(r > 0)
							Import(1);
						else if(!r) {
							Logger.LogLastError();
						}
						BillParam = bill_param;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::RunEDIImport2()
{
	int    ok = 1, r = 1;
	//
	// Если импортируем через EDI, то необходимо импортировать данные для всех провайдеров, указанных
	// в impexp.ini для выбранного типа документов
	//
	PPBillImpExpParam bill_param;
	SString temp_buf;
	{
        PPObjEdiProvider ep_obj;
        PPEdiProvider ep_rec;
        for(SEnum en = ep_obj.Enum(0); en.Next(&ep_rec) > 0;) {

        }
#if 0 // {
		uint sect_pos = 0;
		SString str, type_str;
		// Импортируем документы, пока получаем настройки для импорта данного типа документа (BillParam.Name) (перечисление в PPTXT_EDIIMPCMD)
		while(sects.get(&sect_pos, str) > 0) {
			uint   start_pos = 0;
			uint   end_pos = 0;
			(type_str = BillParam.Name).Transf(CTRANSF_INNER_TO_OUTER);
			if((str.Search("IMP@BILL@DLL_", 0, 1, &(start_pos = 0)) > 0) && (str.Search(BillParam.Name, 0, 1, &(start_pos = 0)) > 0)) {
				if(!BillParam.ReadIni(&ini_file, str, 0)) {
					PPSetError(PPERR_IMPEXPCFGRDFAULT, str);
					Logger.LogLastError();
				}
				else {
					r = ReadData();
					if(r > 0)
						Import(1);
					else if(!r) {
						Logger.LogLastError();
					}
					BillParam = bill_param;
				}
			}
		}
#endif // } 0
	}
	// CATCHZOK
	return ok;
}

const Sdr_Bill * SLAPI PPBillImporter::SearchBillForRow(const SString & rBillIdent, const Sdr_BRow & rRow) const
{
	uint   p = 0;
	while(Bills.lsearch(rBillIdent, &p, PTR_CMPFUNC(Pchar)) > 0) {
		const long _int_bill_id = rBillIdent.ToLong(); // @debug
		const Sdr_Bill * p_bill = &Bills.at(p);
		if((!rRow.BillDate || rRow.BillDate == p_bill->Date) && (!rRow.INN[0] || strcmp(rRow.INN, p_bill->INN) == 0)) {
			return p_bill;
		}
		else
			p++;
	}
	return 0;
}

int SLAPI PPBillImporter::SearchNextRowForBill(const Sdr_Bill & rBill, uint * pPos) const
{
    uint p = *pPos;
	while(BillsRows.lsearch(rBill.ID, &p, PTR_CMPFUNC(Pchar))) {
		const Sdr_BRow * p_row = &BillsRows.at(p);
		if((!p_row->BillDate || p_row->BillDate == rBill.Date) && (!p_row->INN[0] || strcmp(p_row->INN, rBill.INN) == 0)) {
			ASSIGN_PTR(pPos, p);
			return 1;
		}
		else
			p++;
	}
	return 0;
}

int SLAPI PPBillImporter::AddBRow(Sdr_BRow & rRow, uint * pRowId)
{
	LineIdSeq++;
	rRow.LineId = LineIdSeq;
	ASSIGN_PTR(pRowId, rRow.LineId);
	return BillsRows.insert(&rRow) ? 1 : PPSetErrorSLib();
}

int SLAPI PPBillImporter::ReadRows(PPImpExp * pImpExp, int mode/*linkByLastInsBill*/, const StrAssocArray * pFnFldList)
{
	int    ok = 1;
	long   count = 0;
	const  LDATE cdate = getcurdate_();
	SString def_inn;
	SString temp_buf;
	SString inn;
	SString bill_ident;
	StrAssocArray articles;
	PPObjGoods gobj;
	PPObjTag tag_obj;
	SdRecord dyn_rec;
	SdbField dyn_fld, inner_fld;
	THROW_PP(pImpExp, PPERR_INVPARAM);
	THROW(pImpExp->InitDynRec(&dyn_rec));
	pImpExp->GetNumRecs(&count);
	def_inn = BillParam.Object1SrchCode;
	for(long i = 0; i < count; i++) {
		uint   p = 0;
		PPID   id = 0;
		Sdr_BRow brow_;
		BarcodeTbl::Rec bcrec;
		MEMSZERO(brow_);
		MEMSZERO(bcrec);
		if(pFnFldList) {
			AssignFnFieldToRecord(*pFnFldList, 0, &brow_);
		}
		THROW(pImpExp->ReadRecord(&brow_, sizeof(brow_), &dyn_rec));
		(temp_buf = brow_.Barcode).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.Barcode, temp_buf);
		(temp_buf = brow_.GoodsName).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.GoodsName, temp_buf);
		if(mode == 1/*linkByLastInsBill*/)
			STRNSCPY(brow_.BillID, Bills.at(Bills.getCount() - 1).ID);
		else if(mode == 2) {
			// @v8.7.1 {
			if(brow_.BillID[0] == 0)
				STRNSCPY(brow_.BillID, brow_.BillCode);
			// } @v8.7.1
			SETIFZ(brow_.BillDate, cdate);
			if(!Period.CheckDate(brow_.BillDate))
				continue;
		}
		(bill_ident = brow_.BillID).Strip();
		if(mode != 2 && bill_ident.Empty()) { // нет сопоставления для строки документа
			SString msg, file_name, goods_info, word;
			SPathStruc sp;
			pImpExp->GetFileName(file_name);
			sp.Split(file_name);
			(file_name = sp.Nam).Dot().Cat(sp.Ext);
			PPLoadText(PPTXT_BROWLINKNOTFOUND, temp_buf);
			if(brow_.Barcode[0]) {
				// @v9.0.2 PPGetWord(PPWORD_BARCODE, 0, goods_info);
				PPLoadString("barcode", goods_info); // @v9.0.2
				goods_info.Space().Cat(brow_.Barcode).Space();
			}
			// @v9.0.2 PPGetWord(PPWORD_GOODS, 0, (word = 0));
			PPLoadString("ware", word); // @v9.0.2
			goods_info.Cat(word).Space().Cat(brow_.GoodsName);
			msg.Printf(temp_buf, (const char*)file_name, (const char*)goods_info);
			Logger.Log(msg);
		}
		else { //if(mode == 2 || Bills.lsearch(bill_ident, &(p = 0), PTR_CMPFUNC(Pchar)) > 0) { // bsearch->lsearch
			const Sdr_Bill * p_bill = (mode == 2) ? 0 : SearchBillForRow(bill_ident, brow_);
			if(mode == 2 || p_bill) {
				uint   pos = 0;
				PPID   goods_id = 0;
				ObjTagList tag_list;
				Goods2Tbl::Rec goods_rec;
				if(brow_.GoodsID && gobj.Fetch(brow_.GoodsID, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
					goods_id = goods_rec.ID;
				}
				else if(CConfig.Flags & CCFLG_USEARGOODSCODE && (brow_.Barcode[0] || brow_.ArCode[0])) {
					PPID   ar_id = 0;
					ArticleTbl::Rec ar_rec;
					ArGoodsCodeTbl::Rec code_rec;
					MEMSZERO(code_rec);
					if(brow_.CntragID && ArObj.Fetch(brow_.CntragID, &ar_rec) > 0) {
						ar_id = brow_.CntragID;
					}
					else {
						inn = p_bill ? p_bill->INN : brow_.INN;
						if(!inn.NotEmptyS())
							inn = def_inn;
						if(articles.SearchByText(inn, 0, &pos) > 0)
							ar_id = articles.at(pos).Id;
						else if(ResolveINN(inn, (p_bill ? p_bill->DlvrAddrID : 0), (p_bill ? p_bill->DlvrAddrCode : 0),
							bill_ident, AccSheetID, &ar_id, 0) > 0 && ar_id > 0) {
							THROW_SL(articles.Add(ar_id, inn, 0));
						}
					}
					if(ar_id > 0) {
						brow_.CntragID = ar_id;
						if(brow_.ArCode[0] && gobj.P_Tbl->SearchByArCode(ar_id, brow_.ArCode, &code_rec) > 0)
							goods_id = code_rec.GoodsID;
						else if(brow_.Barcode[0] && gobj.P_Tbl->SearchByArCode(ar_id, brow_.Barcode, &code_rec) > 0)
							goods_id = code_rec.GoodsID;
					}
				}
				if(!goods_id) {
					if(brow_.Barcode[0] && gobj.SearchByBarcode(brow_.Barcode, &bcrec, 0, 1) > 0) // @v7.1.5 adoptSearching=1
						goods_id = bcrec.GoodsID;
					else if(gobj.SearchByName(brow_.GoodsName, &id) > 0)
						goods_id = id;
				}
				brow_.GoodsID = goods_id;
				if(dyn_rec.GetCount()) {
					const PPImpExpParam & r_iep = pImpExp->GetParam();
					SStrScan scan;
					PPID   tag_id = 0;
					PPObjectTag tag_rec;
					tag_list.ObjType = PPOBJ_LOT;
					for(uint j = 0; j < dyn_rec.GetCount(); j++) {
						dyn_rec.GetFieldByPos(j, &dyn_fld);
						if(dyn_fld.Formula.NotEmptyS()) {
							scan.Set(dyn_fld.Formula, 0);
							if(scan.GetIdent((temp_buf = 0))) {
								if(temp_buf.CmpNC("lottag") == 0) {
									scan.Skip();
									if(scan[0] == '.') {
										scan.Incr(1);
										(temp_buf = scan).Strip();
										if(tag_obj.SearchBySymb(temp_buf, &tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT) {
											int   r = 0;
											const TYPEID typ = dyn_fld.T.Typ;
											const int    base_typ = stbase(typ);
											ObjTagItem tag_item;
											if(base_typ == BTS_INT) {
												long ival = 0;
												sttobase(typ, dyn_rec.GetDataC(j), &ival);
												if(ival) {
													tag_item.SetInt(tag_id, ival);
													r = 1;
												}
											}
											else if(base_typ == BTS_REAL) {
												double rval = 0.0;
												sttobase(typ, dyn_rec.GetDataC(j), &rval);
												if(rval != 0.0) {
													tag_item.SetReal(tag_id, rval);
													r = 1;
												}
											}
											else if(base_typ == BTS_STRING) {
												char strval[1024];
												sttobase(typ, dyn_rec.GetDataC(j), strval);
												if(strval[0]) {
													(temp_buf = strval).Strip().Transf(CTRANSF_OUTER_TO_INNER);
													tag_item.SetStr(tag_id, temp_buf);
													r = 1;
												}
											}
											else if(base_typ == BTS_DATE) {
												LDATE dval = ZERODATE;
												sttobase(typ, dyn_rec.GetDataC(j), &dval);
												if(checkdate(dval, 0)) {
													tag_item.SetDate(tag_id, dval);
													r = 1;
												}
											}
											if(r) {
												tag_list.PutItem(tag_id, &tag_item);
											}
										}
									}
								}
								// @v7.4.10 {
								else if(temp_buf.CmpNC("costformula") == 0) {
									scan.Skip();
									if(scan[0] == '.') {
										scan.Incr(1);
										(temp_buf = scan).Strip();
										if(temp_buf.NotEmpty()) {
											ImpExpExprEvalContext expr_ctx(r_iep.InrRec);
											double val = 0.0;
											if(PPExprParser::CalcExpression(temp_buf, &val, 0, &expr_ctx) > 0) {
												if(val > 0.0)
													brow_.Cost = val;
												else
													Logger.LogString(PPTXT_BROWCOSTFORMINVRES, temp_buf);
											}
											else
												Logger.LogString(PPTXT_BROWCOSTFORMINV, temp_buf);
										}
									}
								}
								// } @v7.4.10
								else if(temp_buf.CmpNC("priceformula") == 0) {
									scan.Skip();
									if(scan[0] == '.') {
										scan.Incr(1);
										(temp_buf = scan).Strip();
										if(temp_buf.NotEmpty()) {
											ImpExpExprEvalContext expr_ctx(r_iep.InrRec);
											double val = 0.0;
											if(PPExprParser::CalcExpression(temp_buf, &val, 0, &expr_ctx) > 0) {
												if(val > 0.0)
													brow_.Price = val;
												else
													Logger.LogString(PPTXT_BROWPRICEFORMINVRES, temp_buf);
											}
											else
												Logger.LogString(PPTXT_BROWPRICEFORMINV, temp_buf);
										}
									}
								}
								else if(temp_buf.CmpNC("formula") == 0) {
									if(scan.Skip()[0] == '[') {
										scan.Incr(1);
										if(scan.Skip().GetIdent(temp_buf) && scan.Skip()[0] == ']' && scan.Skip()[0] == '.') {
											SString in_fld_name = temp_buf.Strip();
											scan.Incr(2);
											(temp_buf = scan).Strip();
											if(temp_buf.NotEmpty()) {
												uint   fld_pos = 0;
												if(r_iep.InrRec.SearchName(in_fld_name, &fld_pos)) {
													r_iep.InrRec.GetFieldByPos(fld_pos, &inner_fld);
													if(btnumber(stbase(inner_fld.T.Typ))) {
														ImpExpExprEvalContext expr_ctx(r_iep.InrRec);
														double val = 0.0;
														if(PPExprParser::CalcExpression(temp_buf, &val, 0, &expr_ctx) > 0) {
															stcast(T_DOUBLE, inner_fld.T.Typ, &val, r_iep.InrRec.GetData(fld_pos), 0);
														}
														else
															Logger.LogString(PPTXT_BROWFORMINVRES, temp_buf);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				{
					uint    row_id = 0;
					THROW(AddBRow(brow_, &row_id));
					// @v9.2.0 if(mode != 2) {
						if(tag_list.GetCount())
							TagC.Set(row_id, &tag_list);
					// @v9.2.0 }
				}
			}
		}
		if(mode != 1 /* !linkByLastInsBill*/)
			PPWaitPercent(i + 1, count);
	}
	if(mode == 2) {
		// @v8.7.1 BillsRows.sort(PTR_CMPFUNC(Sdr_BRow));
	}
	else {
		BillsRows.sort(PTR_CMPFUNC(Sdr_BRow_ID));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::ReadSpecXmlData()
{
	int    ok = 1;
	uint   p = 0;
	long   count = 0;
	long   row_rec_count = 0;
	StrAssocArray articles;
	PPImpExp ie(&BillParam, 0);
	PPImpExp * p_ie_row = 0;
	PPObjGoods gobj;
	THROW(ie.OpenFileForReading(0));
	p_ie_row = &ie;
	ie.GetNumRecs(&count);
	for(long i = 0; i < count; i++) {
		Sdr_Bill bill;
		MEMSZERO(bill);
		THROW(ie.ReadRecord(&bill, sizeof(bill)));
		SETIFZ(bill.Date, getcurdate_());
		if(CheckBill(&bill)) {
            int    r = AddBillToList(&bill, 0);
            THROW(r);
            if(r > 0) {
				ie.Push(&BRowParam);
				THROW(ReadRows(&ie, 1, 0));
				ie.Pop();
			}
		}
		PPWaitPercent(i + 1, count);
	}
	THROW(ie.CloseFile());
	CATCHZOK
	return ok;
}

/*
InitImport
GetErrorMessage
GetImportObj
ReplyImportObjStatus
InitImportIter
NextImportIter
FinishImpExp
*/

int SLAPI PPBillImporter::AddBillToList(Sdr_Bill * pBill, long extraBillId)
{
	int    ok = 1;
	int    found = 0;
	assert(pBill);
	THROW_PP(pBill, PPERR_INVPARAM);
	for(uint j = 0; ok > 0 && j < Bills.getCount(); j++) {
		const Sdr_Bill & r_rec = Bills.at(j);
		if(pBill->Date == r_rec.Date && (!pBill->Code[0] || stricmp(pBill->Code, r_rec.Code) == 0) && (!pBill->ID[0] || stricmp(pBill->ID, r_rec.ID) == 0))
			ok = -1;
	}
	if(ok > 0) {
		SString bid = pBill->ID;
		if(!bid.NotEmptyS() && pBill->Code[0] != 0)
			(bid = pBill->Code).Strip();
		else if(pBill->Code[0] == 0 && bid.NotEmptyS()) {
			STRNSCPY(pBill->Code, bid);
		}
		else if(!bid.NotEmptyS() && extraBillId)
			(bid = "@docimp").Cat(extraBillId);
		STRNSCPY(pBill->ID, bid);
		SETIFZ(pBill->Date, getcurdate_());
		THROW_SL(Bills.insert(pBill));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::AssignFnFieldToRecord(const StrAssocArray & rFldList, Sdr_Bill * pRecHdr, Sdr_BRow * pRecRow)
{
	int    ok = 1;
	if(pRecHdr || pRecRow) {
		//
		// Гарантируем не нулевые указатели pRecHdr и pRecRow (дабы упростить последующий код)
		//
		Sdr_Bill hdr_stub;
		Sdr_BRow row_stub;
        SETIFZ(pRecHdr, &hdr_stub);
        SETIFZ(pRecRow, &row_stub);
		//
		for(uint i = 0; i < rFldList.getCount(); i++) {
			StrAssocArray::Item item = rFldList.at_WithoutParent(i);
			LDATE dt = ZERODATE;
			switch(item.Id) {
				case PPSYM_BILLNO:
                    STRNSCPY(pRecHdr->Code, item.Txt);
                    STRNSCPY(pRecRow->BillCode, item.Txt);
					break;
				case PPSYM_DATE:
					{
                        strtodate(item.Txt, DATF_DMY, &dt);
                        pRecHdr->Date = dt;
                        pRecRow->BillDate = dt;
					}
					break;
				case PPSYM_FGDATE:
					{
                        strtodate(item.Txt, DATF_DMY|DATF_CENTURY|DATF_NODIV, &dt);
                        pRecHdr->Date = dt;
                        pRecRow->BillDate = dt;
					}
					break;
				case PPSYM_PAYDATE:
					{
                        strtodate(item.Txt, DATF_DMY, &dt);
                        pRecHdr->PaymDate = dt;
                        pRecRow->PaymDate = dt;
					}
					break;
				case PPSYM_INVOICEDATE:
					{
                        strtodate(item.Txt, DATF_DMY, &dt);
                        pRecHdr->InvoiceDate = dt;
                        pRecRow->InvcDate = dt;
					}
					break;
				case PPSYM_INVOICENO:
					STRNSCPY(pRecHdr->InvoiceCode, item.Txt);
					STRNSCPY(pRecRow->InvcCode, item.Txt);
					break;
				case PPSYM_LOCCODE:
					STRNSCPY(pRecHdr->LocCode, item.Txt);
					STRNSCPY(pRecRow->LocCode, item.Txt);
					break;
				case PPSYM_DLVRLOCCODE:
					STRNSCPY(pRecHdr->DlvrAddrCode, item.Txt);
					STRNSCPY(pRecRow->DlvrAddrCode, item.Txt);
					break;
				case PPSYM_DLVRLOCID:
					pRecHdr->DlvrAddrID = atol(item.Txt);
					pRecRow->DlvrAddrID = atol(item.Txt);
					break;
				case PPSYM_INN:
					STRNSCPY(pRecHdr->INN, item.Txt);
					STRNSCPY(pRecRow->INN, item.Txt);
					break;
				case PPSYM_DUMMY:
					break;
				default:
					break;
			}
		}
	}
	return ok;
}

int SLAPI PPBillImporter::ReadData()
{
	int    ok = -1, r = -1;
	char   _err_buf[1024];
	_err_buf[0] = 0;
	ImpExpDll imp_dll;
	SString err_msg, path, wildcard, filename, temp_buf;
	PPObjGoods gobj;
	PPFileNameArray file_list;
	Sdr_Bill bill;
	SPathStruc ps;
	const int h_r_eq_f = (BillParam.DataFormat == BRowParam.DataFormat) ? BillParam.DataFormat : -1;
	const int imp_rows_from_same_file = BIN(BillParam.Flags & PPBillImpExpParam::fImpRowsFromSameFile &&
		oneof4(h_r_eq_f, PPImpExpParam::dfText, PPImpExpParam::dfDbf, PPImpExpParam::dfExcel, PPImpExpParam::dfXml));
	const  int imp_rows_only = BIN(imp_rows_from_same_file && BillParam.Flags & PPBillImpExpParam::fImpRowsOnly);
	if(BillParam.BaseFlags & PPImpExpParam::bfDLL) {
		int    obj_id = 0, sess_id = 0;
		Sdr_BRow brow;
		BillParam.ImpExpParamDll.FileName = BillParam.FileName;
		THROW_SL(imp_dll.InitLibrary(BillParam.ImpExpParamDll.DllPath, 2));
		Sdr_ImpExpHeader hdr;
		MEMSZERO(hdr);
		hdr.PeriodLow = Period.low;
		hdr.PeriodUpp = Period.upp;
		BillParam.ImpExpParamDll.Login.CopyTo(hdr.EdiLogin, sizeof(hdr.EdiLogin));
		BillParam.ImpExpParamDll.Password.CopyTo(hdr.EdiPassword, sizeof(hdr.EdiPassword));
		BillParam.ImpExpParamDll.OperType.CopyTo(hdr.EdiDocType, sizeof(hdr.EdiDocType));
		if(!imp_dll.InitImport(&hdr, BillParam.ImpExpParamDll.FileName, &sess_id)) {
			imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
			CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
		}
		//
		// Выполняется, пока есть документы для импорта
		//
		while((r = imp_dll.GetImportObj(sess_id, "BILLS", &bill, &obj_id, BillParam.ImpExpParamDll.OperType)) == 1) {
			Sdr_DllImpObjStatus imp_obj_stat;
			MEMSZERO(imp_obj_stat);
			// Проверка документа и заполнение Sdr_ImpObjStatus
			GetDocImpStatus(&bill, imp_obj_stat);
			imp_dll.ReplyImportObjStatus(sess_id, obj_id, &imp_obj_stat);
			if(imp_obj_stat.DocStatus == statIsSuchDoc) {
				int    edi_op = 0;
				SString ordresp_cmd, desadv_cmd;
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_ORDRSP, ordresp_cmd));
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_DESADV, desadv_cmd));
				if(BillParam.ImpExpParamDll.OperType.CmpNC("ALCODESADV") == 0)
					edi_op = PPEDIOP_ALCODESADV;
				else if(ordresp_cmd.CmpNC(BillParam.ImpExpParamDll.OperType) == 0)
					edi_op = PPEDIOP_ORDERRSP;
				else if(desadv_cmd.CmpNC(BillParam.ImpExpParamDll.OperType) == 0)
					edi_op = PPEDIOP_DESADV;
				if(edi_op == PPEDIOP_ALCODESADV) {
					THROW(AddBillToList(&bill, obj_id));
					if(imp_dll.InitImportIter(sess_id, obj_id)) {
						while(imp_dll.NextImportIter(sess_id, obj_id, &brow) == 1) {
							uint   p = 0;
							PPID   id = 0;
							BarcodeTbl::Rec bcrec;
							if(brow.Barcode[0] && gobj.SearchByBarcode(brow.Barcode, &bcrec, 0, 1) > 0)
								brow.GoodsID = bcrec.GoodsID;
							else if(gobj.SearchByName(brow.GoodsName, &id) > 0)
								brow.GoodsID = id;
							STRNSCPY(brow.BillID, bill.ID);
							STRNSCPY(brow.BillCode, bill.Code);
							{
								uint   row_id = 0;
								THROW(AddBRow(brow, &row_id));
								ok = 1;
							}
						}
					}
					else {
						imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
						CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
					}
				}
				else if(oneof2(edi_op, PPEDIOP_ORDERRSP, PPEDIOP_DESADV)) {
					if(CheckBill(&bill)) {
						bill.EdiOp = edi_op;
						THROW(AddBillToList(&bill, obj_id));
					}
					if(imp_dll.InitImportIter(sess_id, obj_id)) {
						while(imp_dll.NextImportIter(sess_id, obj_id, &brow) == 1) {
							uint   p = 0;
							PPID   id = 0;
							BarcodeTbl::Rec bcrec;
							if(brow.Barcode[0] && gobj.SearchByBarcode(brow.Barcode, &bcrec, 0, 1) > 0)
								brow.GoodsID = bcrec.GoodsID;
							else if(gobj.SearchByName(brow.GoodsName, &id) > 0)
								brow.GoodsID = id;
							STRNSCPY(brow.BillID, bill.ID);
							STRNSCPY(brow.BillCode, bill.Code);
							{
								uint   row_id = 0;
								THROW(AddBRow(brow, &row_id));
								ok = 1;
							}
						}
					}
					else {
						imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
						CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
					}
				}
			}
		}
		if(!r) {
			imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
			CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
		}
		THROW(r);
		THROW(imp_dll.FinishImpExp());
	}
	else if(h_r_eq_f == PPImpExpParam::dfXml && BillParam.FileName.CmpNC(BRowParam.FileName) == 0 && !imp_rows_only) {
		// @v8.6.4 const  int imp_rows_only = BIN(BillParam.Flags & PPBillImpExpParam::fImpRowsOnly);
		THROW(BillParam.PreprocessImportFileSpec((path = BillParam.FileName).Transf(CTRANSF_INNER_TO_OUTER), file_list));
		for(uint fi = 0; file_list.Enum(&fi, 0, &(filename = 0));) {
			uint   p = 0;
			long   count = 0;
			long   row_rec_count = 0;
			StrAssocArray articles;
			PPImpExp ie(&BillParam, 0);
			PPImpExp * p_ie_row = 0;
			StrAssocArray fn_fld_list;
			BillParam.PreprocessImportFileName(filename, fn_fld_list);
			THROW(ie.OpenFileForReading(filename));
			p_ie_row = &ie;
			ie.GetNumRecs(&count);
			for(long i = 0; i < count; i++) {
				MEMSZERO(bill);
				AssignFnFieldToRecord(fn_fld_list, &bill, 0);
				THROW(ie.ReadRecord(&bill, sizeof(bill)));
				SETIFZ(bill.Date, getcurdate_());
				if(CheckBill(&bill)) {
					int   ir = AddBillToList(&bill, 0);
					THROW(ir);
					if(ir > 0) {
						ie.Push(&BRowParam);
						THROW(ReadRows(&ie, 1, &fn_fld_list));
						ie.Pop();
					}
				}
				PPWaitPercent(i + 1, count);
			}
			THROW(ie.CloseFile());
			// @v8.4.7 {
			if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
				ToRemoveFiles.add(filename);
			}
			// } @v8.4.7
			ok = 1;
		}
	}
	else {
		uint   p = 0;
		long   count = 0;
		PPImpExp ie(&BillParam, 0);
		PPImpExp ie_row(&BRowParam, 0);
		SString bid;
		BillsRows.freeAll(); // @v8.7.1
		THROW(BillParam.PreprocessImportFileSpec((path = BillParam.FileName).Transf(CTRANSF_INNER_TO_OUTER), file_list));
		for(uint fi = 0; file_list.Enum(&fi, 0, &(filename = 0));) {
			StrAssocArray fn_fld_list;
			BillParam.PreprocessImportFileName(filename, fn_fld_list);
			if(imp_rows_only) {
				// @v8.7.1 BillsRows.freeAll();
				SdrBillRowArray preserve_rows = BillsRows;
				BillsRows.freeAll();
				THROW(ie_row.OpenFileForReading(filename));
				THROW(ReadRows(&ie_row, 2, &fn_fld_list));
				ie_row.CloseFile();
				{
					SString fn_for_hash;
					ps.Split(filename);
					ps.Drv = 0;
					ps.Dir = 0;
					ps.Merge(fn_for_hash);
					fn_for_hash.Strip().ToLower();
					LDATE last_date = ZERODATE;
					SString last_code, bill_code;
					SString last_ident, bill_ident;
					long   cc_ = 0;
					for(uint ln = 0; ln < BillsRows.getCount(); ln++) {
						Sdr_BRow & r_row = BillsRows.at(ln);
						(bill_ident = r_row.BillID).Strip();
						(bill_code = r_row.BillCode).Strip();
						const int new_bill = BIN(r_row.BillDate != last_date || last_code != bill_code || last_ident != bill_ident);
						if(new_bill) {
							cc_++;
							last_date = r_row.BillDate;
							last_code = bill_code;
							last_ident = bill_ident;
						}
						if(bill_ident.Empty()) {
							(bill_ident = 0).CatLongZ(cc_, 6);
						}
						bill_ident.CopyTo(r_row.BillID, sizeof(r_row.BillID));
						//
						if(bill_code.Empty()) {
							(bill_code = 0).Cat(fn_for_hash).Cat(bill.Date, MKSFMT(0, DATF_YMD|DATF_CENTURY|DATF_NODIV));
							const uint32 _h = BobJencHash(bill_code, bill_code.Len());
							(bill_code = 0).Cat("H-").Cat(_h).CatChar('-').CatLongZ(cc_, 6);
						}
						bill_code.CopyTo(r_row.BillCode, sizeof(r_row.BillCode));
						if(new_bill) {
							MEMSZERO(bill);
							bill.Date = r_row.BillDate;
							bill_code.CopyTo(bill.Code, sizeof(bill.Code));
							bill_ident.CopyTo(bill.ID, sizeof(bill.ID));
                            bill.CntragID = r_row.CntragID;
							bill.CntragNo = r_row.CntragNo; // @v9.0.9
                            STRNSCPY(bill.CntragName, r_row.CntragName);
                            bill.DueDate = r_row.DueDate;
                            STRNSCPY(bill.INN, r_row.INN);
                            STRNSCPY(bill.LocCode, r_row.LocCode);
                            STRNSCPY(bill.LocID, r_row.LocID);
                            STRNSCPY(bill.LocName, r_row.LocName);
                            bill.Obj2ID = r_row.Obj2ID;
                            STRNSCPY(bill.Obj2INN, r_row.Obj2INN);
                            STRNSCPY(bill.Obj2Name, r_row.Obj2Name);
                            bill.Obj2No = r_row.Obj2No;
                            STRNSCPY(bill.AgentINN, r_row.AgentINN);
							bill.DlvrAddrID = r_row.DlvrAddrID; // @v8.7.1
							STRNSCPY(bill.DlvrAddrCode, r_row.DlvrAddrCode); // @v8.7.1
                            STRNSCPY(bill.Memo, r_row.BillMemo); // @v8.6.0
                            // @todo не все поля перенесены из r_row в bill
                            if(CheckBill(&bill)) {
								THROW_SL(Bills.insert(&bill));
                            }
						}
					}
				}
				{
					for(uint i = 0; i < BillsRows.getCount(); i++) {
						THROW_SL(preserve_rows.insert(&BillsRows.at(i)));
					}
					BillsRows = preserve_rows;
				}
			}
			else {
				long   bill_line_no = 0;
				int    accept_rows = 0;
				THROW(ie.OpenFileForReading(filename));
				ie.GetNumRecs(&count);
				for(bill_line_no = 0; bill_line_no < count; bill_line_no++) {
					MEMSZERO(bill);
					AssignFnFieldToRecord(fn_fld_list, &bill, 0);
					THROW(ie.ReadRecord(&bill, sizeof(bill)));
					if(bill.Date == ZERODATE)
						getcurdate(&bill.Date);
					if(CheckBill(&bill)) {
						int    ir = AddBillToList(&bill, fi);
						THROW(ir);
						if(ir > 0) {
							accept_rows = 1;
						}
					}
					PPWaitPercent(bill_line_no + 1, count);
					if(imp_rows_from_same_file)
						break;
				}
				THROW(ie.CloseFile());
				if(imp_rows_from_same_file && accept_rows) {
					THROW(ie_row.OpenFileForReading(filename));
					THROW(ReadRows(&ie_row, 1, &fn_fld_list));
					ie_row.CloseFile();
				}
				THROW_SL(Bills.sort(PTR_CMPFUNC(Pchar)));
				// @v8.4.7 {
				if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
					ToRemoveFiles.add(filename);
				}
				// } @v8.4.7
			}
			ok = 1;
		}
		if(!imp_rows_from_same_file) {
			if(GetOpType(BillParam.ImpOpID) != PPOPT_ACCTURN) { // @v8.4.10
				THROW(BRowParam.PreprocessImportFileSpec((path = BRowParam.FileName).Transf(CTRANSF_INNER_TO_OUTER), file_list));
				for(uint fi = 0; file_list.Enum(&fi, 0, &filename);) {
					StrAssocArray fn_fld_list;
					BillParam.PreprocessImportFileName(filename, fn_fld_list);
					THROW(ie_row.OpenFileForReading(filename));
					THROW(ReadRows(&ie_row, 0, &fn_fld_list));
					ie_row.CloseFile();
					// @v8.4.7 {
					if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
						ToRemoveFiles.add(filename);
					}
					// } @v8.4.7
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::ResolveUnitPerPack(PPID goodsID, PPID locID, LDATE dt, double * pUpp)
{
	double upp = 0.0;
	ReceiptCore * p_rcpt = (P_BObj && P_BObj->trfr) ? &P_BObj->trfr->Rcpt : 0;
	if(p_rcpt) {
		ReceiptTbl::Rec lot_rec;
		MEMSZERO(lot_rec);
		if(p_rcpt->GetLastLot(goodsID, locID, dt, &lot_rec) > 0)
			upp = lot_rec.UnitPerPack;
		if(upp <= 0.0) {
			LDATE  iter_dt = dt;
			long   iter_oprno = MAXLONG;
			while(p_rcpt->EnumLastLots(goodsID, locID, &iter_dt, &iter_oprno, &lot_rec) > 0) {
				if(lot_rec.UnitPerPack > 0.0 || diffdate(dt, iter_dt) > 180) {
					break;
				}
			}
			upp = lot_rec.UnitPerPack;
		}
		SETMAX(upp, 0.0);
	}
	ASSIGN_PTR(pUpp, upp);
	return (upp > 0.0) ? 1 : -1;
}

int SLAPI PPBillImporter::Import(int useTa)
{
	int    ok = 1;
	SString temp_buf, qc_code, TTN;
	SString fmt_buf, msg_buf;
	SString serial, sn_templt;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	PPObjGoods goods_obj;
	ObjTransmContext ot_ctx(&Logger); // Контекст, необходимый для автоматического приходования дефицита классом BillTransmDeficit
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	ObjTagItem tag_item;
	ObjTagList tag_list;
	RegisterTbl::Rec reg_rec;
	PPID   mnf_lot_tag_id = 0;
	PPOprKind op_rec;
	GetOpData(OpID, &op_rec);
	const int restrict_by_matrix = BIN(BillParam.Flags & PPBillImpExpParam::fRestrictByMatrix);
	const int need_price_restrict = BIN(op_rec.ExtFlags & OPKFX_RESTRICTPRICE);
	PPWait(0);
	{
		const  PPID temp_tag_id_1 = BillParam.ImpExpParamDll.ManufTagID;
		const  PPID temp_tag_id_2 = P_BObj->GetConfig().MnfCountryLotTagID;
		if(temp_tag_id_1 || temp_tag_id_2) {
			if(temp_tag_id_1 && tag_obj.Fetch(temp_tag_id_1, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
				mnf_lot_tag_id = temp_tag_id_1;
			}
			else if(temp_tag_id_2 && tag_obj.Fetch(temp_tag_id_2, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
				mnf_lot_tag_id = temp_tag_id_2;
			}
		}
	}
	{
		//
		// Обработка неразрешенных товаров
		//
		int    r = -1;
		ResolveGoodsItemList goods_list;
		LongArray unres_pos_list;
		for(uint i = 0; i < BillsRows.getCount(); i++) {
			const Sdr_BRow & r_row = BillsRows.at(i);
			if(!r_row.GoodsID) {
				ResolveGoodsItem item;
				STRNSCPY(item.GoodsName, r_row.GoodsName);
				STRNSCPY(item.Barcode, r_row.Barcode);
				item.Quantity = r_row.Quantity;
				if(r_row.CntragID && r_row.ArCode[0]) {
					item.ArID = r_row.CntragID;
					STRNSCPY(item.ArCode, r_row.ArCode);
				}
				THROW_SL(goods_list.insert(&item));
				unres_pos_list.add(i);
			}
		}
		if(goods_list.getCount()) {
			if(ResolveGoodsDlg(&goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG) > 0) {
				for(uint i = 0; i < goods_list.getCount(); i++)
					BillsRows.at(unres_pos_list.get(i)).GoodsID = goods_list.at(i).ResolvedGoodsID;
				r = 1;
			}
			else
				ok = -1;
		}
	}
	if(ok > 0) {
		PrcssrAlcReport::Config alc_rep_cfg;
		int    alc_rep_cfg_inited = 0;
		{
			ZDELETE(P_Btd);
			if(GetOpType(OpID) == PPOPT_GOODSEXPEND) {
				THROW_MEM(P_Btd = new BillTransmDeficit);
				PPObjectTransmit::ReadConfig(&ot_ctx.Cfg);
			}
		}
		Bills.sort(PTR_CMPFUNC(Sdr_Bill)); // @v8.4.8
		PPTransaction tra(useTa);
		THROW(tra);
		PPWait(1);
		for(long i = 0; i < (long)Bills.getCount(); i++) {
			uint   pos = 0;
			int    is_draft_rcpt = BIN(GetOpType(OpID) == PPOPT_DRAFTRECEIPT);
			PPBillPacket pack;
			Sdr_Bill bill = Bills.at(i);
			if(bill.EdiOp == PPEDIOP_ALCODESADV) {
				PPID   alc_manuf_lot_tag_id = 0;
				PPID   alc_manuf_kind_id = 0;
				PPID   alc_cat_lot_tag_id = 0;
				SETIFZ(alc_rep_cfg_inited, (!alc_rep_cfg_inited && PrcssrAlcReport::ReadConfig(&alc_rep_cfg) > 0) ? 1 : -1);
				if(alc_rep_cfg_inited < 0) {
				}
				else if(BillToBillRec(&bill, &pack)) {
					PPID   main_bill_id = 0;
					BillTbl::Rec main_bill_rec;
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &main_bill_id, &main_bill_rec) > 0) {
						int    pack_updated = 0;
						THROW(P_BObj->ExtractPacket(main_bill_id, &pack) > 0);
						{
							if(alc_rep_cfg.CategoryTagID && tag_obj.Fetch(alc_rep_cfg.CategoryTagID, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT)
								alc_cat_lot_tag_id = alc_rep_cfg.CategoryTagID;
							if(alc_rep_cfg.LotManufTagList.getCount()) {
								for(uint k = 0; !alc_manuf_lot_tag_id && k < alc_rep_cfg.LotManufTagList.getCount(); k++) {
									PPID tag_id = alc_rep_cfg.LotManufTagList.get(k);
									if(tag_id && tag_obj.Fetch(tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
										alc_manuf_lot_tag_id = tag_id;
										alc_manuf_kind_id = tag_rec.LinkObjGrp;
									}
								}
							}
							for(pos = 0; SearchNextRowForBill(bill, &pos); pos++) {
								const  Sdr_BRow & r_row = BillsRows.at(pos);
								if(r_row.LineNo > 0 && r_row.LineNo <= (int)pack.GetTCount()) {
									const uint ti_idx = r_row.LineNo-1;
									int    tag_list_updated = 0;
									ObjTagList * p_tag_list = pack.LTagL.Get(ti_idx);
									if(!RVALUEPTR(tag_list, p_tag_list))
										tag_list.Destroy();
									PPTransferItem & r_ti = pack.TI(ti_idx);
									if(r_ti.GoodsID == r_row.GoodsID) {
										if(alc_cat_lot_tag_id && r_row.AlcoCatCode[0]) {
											if(tag_item.SetStr(alc_cat_lot_tag_id, r_row.AlcoCatCode)) {
												tag_list.PutItem(alc_cat_lot_tag_id, &tag_item);
												tag_list_updated = 1;
											}
										}
										if(alc_manuf_lot_tag_id && r_row.ManufINN[0]) {
											PPID   manuf_id = 0;
											PPIDArray psn_list;
                                            PsnObj.GetListByRegNumber(PPREGT_TPID, alc_manuf_kind_id, r_row.ManufINN, psn_list);
											if(psn_list.getCount()) {
												if(r_row.ManufKPP[0]) {
													for(uint k = 0; !manuf_id && k < psn_list.getCount(); k++) {
														const PPID psn_id = psn_list.get(k);
														if(PsnObj.GetRegNumber(psn_id, PPREGT_KPP, ZERODATE, temp_buf = 0) > 0 && temp_buf.CmpNC(r_row.ManufKPP) == 0)
															manuf_id = psn_id;
													}
												}
												else
													manuf_id = psn_list.get(0);
											}
											else if((temp_buf = r_row.LotManuf).NotEmptyS()) {
												PPPersonPacket psn_pack;
                                                STRNSCPY(psn_pack.Rec.Name, temp_buf);
                                                psn_pack.Kinds.add(alc_manuf_kind_id);
                                                {
                                                	MEMSZERO(reg_rec);
                                                	reg_rec.ObjType = PPOBJ_PERSON;
                                                	reg_rec.RegTypeID = PPREGT_TPID;
                                                	STRNSCPY(reg_rec.Num, r_row.ManufINN);
													psn_pack.Regs.insert(&reg_rec);
                                                }
                                                if(r_row.ManufKPP[0]) {
                                                	MEMSZERO(reg_rec);
                                                	reg_rec.ObjType = PPOBJ_PERSON;
													reg_rec.RegTypeID = PPREGT_KPP;
                                                	STRNSCPY(reg_rec.Num, r_row.ManufKPP);
													psn_pack.Regs.insert(&reg_rec);
                                                }
                                                THROW(PsnObj.PutPacket(&manuf_id, &psn_pack, 0));
											}
											if(manuf_id) {
												if(tag_item.SetInt(alc_manuf_lot_tag_id, manuf_id)) {
													tag_list.PutItem(alc_manuf_lot_tag_id, &tag_item);
													tag_list_updated = 1;
												}
											}
										}
									}
									if(tag_list_updated) {
										pack.LTagL.Set(ti_idx, &tag_list);
										pack_updated = 1;
									}
								}
							}
						}
						if(pack_updated) {
							THROW(P_BObj->UpdatePacket(&pack, 0));
						}
					}
					else {
						PPLoadText(PPTXT_EDIRELMAINNFOUND, temp_buf);
					}
				}
			}
			else {
				THROW(pack.CreateBlank2(OpID, bill.Date, LocID, 0));
				if(BillToBillRec(&bill, &pack) > 0) {
					int    is_bad_packet = 0; // Признак того, что документ не удалось правильно преобразовать
					// @v8.4.10 for(pos = 0; !is_bad_packet && BillsRows.lsearch(bill.ID, &pos, PTR_CMPFUNC(Pchar)); pos++) {
					for(pos = 0; !is_bad_packet && SearchNextRowForBill(bill, &pos); pos++) { // @v8.4.10
						const  Sdr_BRow & r_row = BillsRows.at(pos);
						(serial = r_row.Serial).Strip().Transf(CTRANSF_OUTER_TO_INNER);
						if(!r_row.GoodsID) {
							;
						}
						else if(restrict_by_matrix && !goods_obj.BelongToMatrix(r_row.GoodsID, pack.Rec.LocID)) {
							Logger.LogLastError();
						}
						else if(pack.OprType == PPOPT_GOODSEXPEND) {
							ILTI   ilti;
							long   ciltif_ = CILTIF_OPTMZLOTS|CILTIF_SUBSTSERIAL|CILTIF_ALLOWZPRICE/*|CILTIF_SYNC*/;
							int    lot_id_exists = 0;
							if(r_row.LotID > 0) {
								ReceiptTbl::Rec temp_lot_rec;
								lot_id_exists = BIN(P_BObj->trfr->Rcpt.Search(r_row.LotID, &temp_lot_rec) > 0);
								if(lot_id_exists && temp_lot_rec.GoodsID == r_row.GoodsID && temp_lot_rec.LocID == LocID) {
									ilti.LotSyncID = r_row.LotID;
									ciltif_ |= CILTIF_USESYNCLOT;
								}
							}
							ilti.Setup(r_row.GoodsID, -1, r_row.Quantity, r_row.Cost, r_row.Price);
							const int rconv = P_BObj->ConvertILTI(&ilti, &pack, 0, ciltif_, serial.NotEmpty() ? (const char *)serial : 0, 0);
							if(!rconv) {
								Logger.LogLastError();
								SETIFZ(is_bad_packet, 1);
							}
							else if(ilti.HasDeficit()) {
								if(P_Btd) {
									THROW(P_Btd->AddItem(&ilti, serial, &pack.Rec, 1));
								}
								// @v9.1.11 PPGetWord(PPWORD_DEFICIT, 0, msg_buf);
								PPLoadString("deficit", msg_buf); // @v9.1.11
								msg_buf.Space().CatChar('[').Cat(GetGoodsName(r_row.GoodsID, temp_buf));
								if(serial.NotEmpty())
									msg_buf.CatDiv(';', 2).CatEq("sn", serial);
								if(lot_id_exists)
									msg_buf.CatDiv(';', 2).CatEq("lotid", r_row.LotID);
								msg_buf.CatChar(']');
								msg_buf.CatDiv('=', 1).Cat(ilti.Rest, MKSFMTD(0, 6, 0));
								Logger.Log(msg_buf);
								is_bad_packet = 100;
							}
						}
						else {
							PPID   qcert_id = 0;
							double upp = 0.0;
							int    upp_inited = 0;
							PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
							ti.GoodsID  = r_row.GoodsID;
							// @v8.4.6 { перенесено из @01
							ti.Cost     = r_row.Cost;
							ti.Price    = r_row.Price;
							// } @v8.4.6
							// @v8.4.6 {
							if(r_row.LotID > 0) {
								ReceiptTbl::Rec temp_lot_rec;
								const int lot_id_exists = BIN(P_BObj->trfr->Rcpt.Search(r_row.LotID, &temp_lot_rec) > 0);
								if(ti.Flags & PPTFR_RECEIPT) {
									if(!lot_id_exists && !pack.SearchLot(r_row.LotID, 0)) {
										ti.LotID = r_row.LotID;
										ti.TFlags |= PPTransferItem::tfForceLotID;
									}
								}
								else {
									if(lot_id_exists && temp_lot_rec.GoodsID == ti.GoodsID && temp_lot_rec.LocID == LocID) {
										THROW(ti.SetupLot(r_row.LotID, &temp_lot_rec, 0));
									}
								}
							}
							// } @v8.4.6
							if(checkdate(r_row.Expiry, 0))
								ti.Expiry = r_row.Expiry;
							if(is_draft_rcpt || (ti.Flags & PPTFR_RECEIPT)) {
								if(r_row.UnitPerPack > 0.0)
									upp = r_row.UnitPerPack;
								if(upp <= 0.0)
									ResolveUnitPerPack(ti.GoodsID, pack.Rec.LocID, pack.Rec.Dt, &upp);
								upp_inited = 0;
								ti.UnitPerPack = upp;
							}
							if(r_row.Quantity != 0.0)
								ti.Quantity_ = fabs(r_row.Quantity);
							else if(r_row.PckgQtty != 0.0) {
								if(upp <= 0.0) {
									GoodsStockExt gse;
									if(goods_obj.GetStockExt(ti.GoodsID, &gse, 1) > 0)
										upp = gse.Package;
								}
								if(upp <= 0.0 && !upp_inited) {
									ResolveUnitPerPack(ti.GoodsID, pack.Rec.LocID, pack.Rec.Dt, &upp);
									upp_inited = 0;
								}
								if(upp > 0.0)
									ti.Quantity_ = fabs(r_row.PckgQtty * upp);
							}
							// @v8.4.6 (@01 перенесено выше) ti.Cost     = r_row.Cost;
							// @v8.4.6 (@01 перенесено выше) ti.Price    = r_row.Price;
							THROW(P_BObj->SetupImportedPrice(&pack, &ti, 0));
							//
							// Обработка сертификата качества.
							// Если код сертификата пустой, то считаем, что сертификата нет
							//
							(qc_code = r_row.QcCode).Strip().Transf(CTRANSF_OUTER_TO_INNER);
							if(qc_code.NotEmpty() && QcObj.SearchByCode(qc_code, &qcert_id, 0) <= 0) {
								qcert_id = 0;
								QualityCertTbl::Rec qc_rec;
								MEMSZERO(qc_rec);
								qc_code.CopyTo(qc_rec.Code, sizeof(qc_rec.Code));
								(temp_buf = r_row.QcBc).Transf(CTRANSF_OUTER_TO_INNER);
								STRNSCPY(qc_rec.BlankCode, temp_buf);
								(temp_buf = r_row.QcPrDate).Transf(CTRANSF_OUTER_TO_INNER);
								STRNSCPY(qc_rec.SPrDate, temp_buf);
								(temp_buf = r_row.QcManuf).Transf(CTRANSF_OUTER_TO_INNER);
								STRNSCPY(qc_rec.Manuf, temp_buf);
								qc_rec.InitDate = r_row.QcInitDate;
								qc_rec.Expiry = r_row.QcExpiry;
								(temp_buf = r_row.QcEtc).Transf(CTRANSF_OUTER_TO_INNER);
								STRNSCPY(qc_rec.Etc, temp_buf);
								if(r_row.QcRegOrg[0]) {
									(temp_buf = r_row.QcRegOrg).Transf(CTRANSF_OUTER_TO_INNER);
									THROW(PsnObj.AddSimple(&qc_rec.RegOrgan, temp_buf, PPPRK_BUSADMIN, PPPRS_LEGAL, 0));
								}
								THROW(QcObj.PutPacket(&qcert_id, &qc_rec, 0));
							}
							ti.QCert = qcert_id;
							(temp_buf = r_row.CLB).Strip().Transf(CTRANSF_OUTER_TO_INNER);
							if(oneof3(pack.OprType, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, PPOPT_GOODSORDER)) {
								if(serial.NotEmpty()) {
									P_BObj->AdjustSerialForUniq(ti.GoodsID, ti.LotID, 0, serial);
								}
								else if(P_BObj->GetConfig().Flags & BCF_AUTOSERIAL) {
									sn_templt = (goods_obj.IsAsset(ti.GoodsID) > 0) ? P_BObj->Cfg.InvSnTemplt : P_BObj->Cfg.SnTemplt;
									P_BObj->GetSnByTemplate(pack.Rec.Code, labs(ti.GoodsID), &pack.SnL, sn_templt, serial);
								}
							}
							{
								int    new_item_pos = -1;
								THROW(pack.LoadTItem(&ti, temp_buf, serial));
								new_item_pos = pack.GetTCount()-1;
								if(pack.OprType == PPOPT_GOODSRECEIPT) {
									ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
									if(p_tag_list && p_tag_list->GetCount())
										pack.LTagL.Set(new_item_pos, p_tag_list);
								}
								// @v7.6.8 {
								if(need_price_restrict) {
									RealRange price_range;
									if(P_BObj->GetPriceRestrictions(pack, ti, new_item_pos, &price_range) > 0) {
										PPTransferItem & r_ti = pack.TI(new_item_pos);
										if(price_range.low > 0.0 && r_ti.Price < price_range.low) {
											// PPTXT_LOG_IMPBILLPRICEBOUNDLO "Цена на товар @goods ограничена нижним пределом @real --> @real"
											PPFormatT(PPTXT_LOG_IMPBILLPRICEBOUNDLO, &msg_buf, ti.GoodsID, r_ti.Price, price_range.low);
											Logger.Log(msg_buf);
											//
											r_ti.Price = price_range.low;
										}
										else if(price_range.upp > 0.0 && r_ti.Price > price_range.upp) {
											// PPTXT_LOG_IMPBILLPRICEBOUNDUP "Цена на товар @goods ограничена верхним пределом @real --> @real"
											PPFormatT(PPTXT_LOG_IMPBILLPRICEBOUNDUP, &msg_buf, ti.GoodsID, r_ti.Price, price_range.upp);
											Logger.Log(msg_buf);
											//
											r_ti.Price = price_range.upp;
										}
									}
								}
								// } @v7.6.8
								TTN = r_row.TTN;
								// Запоминамем в теге строки имя производителя/импортера
								if(pack.OprType == PPOPT_GOODSRECEIPT && mnf_lot_tag_id) {
									(temp_buf = r_row.LotManuf).Transf(CTRANSF_OUTER_TO_INNER);
									PPID   manuf_id = 0;
									if(temp_buf.NotEmptyS()) {
										THROW(PsnObj.AddSimple(&manuf_id, temp_buf, PPPRK_MANUF, PPPRS_LEGAL, 0));
									}
									if(manuf_id) {
										int    new_item_pos = -1;
										new_item_pos = pack.GetTCount()-1;
										ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
										if(!RVALUEPTR(tag_list, p_tag_list))
											tag_list.Destroy();
										tag_item.Init(mnf_lot_tag_id);
										//tag_item.TagDataType = OTTYP_OBJLINK;
										tag_item.SetInt(tag_item.TagID, manuf_id);
										tag_list.PutItem(tag_item.TagID, &tag_item);
										pack.LTagL.Set(new_item_pos, &tag_list);
										TagC.Set(new_item_pos, &tag_list);
									}
								}
								// Запомним код вид товара
								if(pack.OprType == PPOPT_GOODSRECEIPT && BillParam.ImpExpParamDll.GoodsKindTagID && r_row.GoodKindCode) {
									int    new_item_pos = -1;
									new_item_pos = pack.GetTCount()-1;
									ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
									if(!RVALUEPTR(tag_list, p_tag_list))
										tag_list.Destroy();
									tag_item.Init(BillParam.ImpExpParamDll.GoodsKindTagID);
									tag_item.TagDataType = OTTYP_NUMBER;
									tag_item.SetInt(tag_item.TagID, r_row.GoodKindCode);
									tag_list.PutItem(tag_item.TagID, &tag_item);
									pack.LTagL.Set(new_item_pos, &tag_list);
									TagC.Set(new_item_pos, &tag_list);
								}
							}
						}
					}
					if(is_bad_packet == 100)
						Logger.LogMsgCode(mfError, PPERR_BILLNOTIMPORTED, bill.ID);
					else if(is_bad_packet)
						Logger.LogMsgCode(mfError, PPERR_BILLNOTIMPORTED, bill.ID);
					else {
						if(acs_obj.Fetch(pack.AccSheet, &acs_rec) > 0) {
							if((acs_rec.Flags & ACSHF_USECLIAGT) || pack.AccSheet == GetSellAccSheet()) {
								PPClientAgreement ca_rec;
								if(ArObj.GetClientAgreement(pack.Rec.Object, &ca_rec, 1) > 0) {
									if(!(ca_rec.Flags & AGTF_DEFAULT))
										if(GetAgentAccSheet() && ca_rec.DefAgentID && !pack.Ext.AgentID)
											pack.Ext.AgentID = ca_rec.DefAgentID;
									if(ca_rec.DefPayPeriod >= 0 && pack.Rec.Flags & BILLF_NEEDPAYMENT)
										pack.SetPayDate(plusdate(pack.Rec.Dt, ca_rec.DefPayPeriod), 0);
								}
							}
							else if((acs_rec.Flags & ACSHF_USESUPPLAGT) || pack.AccSheet == GetSupplAccSheet()) {
								PPSupplAgreement sa_rec;
								if(ArObj.GetSupplAgreement(pack.Rec.Object, &sa_rec, 1) > 0) {
									if(!(sa_rec.Flags & AGTF_DEFAULT))
										if(GetAgentAccSheet() && sa_rec.DefAgentID && !pack.Ext.AgentID)
											pack.Ext.AgentID = sa_rec.DefAgentID;
									if(sa_rec.DefPayPeriod >= 0 && pack.Rec.Flags & BILLF_NEEDPAYMENT)
										pack.SetPayDate(plusdate(pack.Rec.Dt, sa_rec.DefPayPeriod), 0);
								}
							}
						}
						if(BillParam.ImpExpParamDll.TTNTagID) {
							tag_item.Init(BillParam.ImpExpParamDll.TTNTagID);
							tag_item.TagDataType = OTTYP_STRING;
							tag_item.SetStr(tag_item.TagID, TTN);
							pack.BTagL.PutItem(tag_item.TagID, &tag_item);
						}
						pack.SetQuantitySign(-1);
						// @v9.1.11 {
						if(CheckOpFlags(pack.Rec.OpID, OPKF_NEEDVALUATION)) {
							int    is_valuation_modif = 0;
							P_BObj->AutoCalcPrices(&pack, 0, &is_valuation_modif);
						}
						// } @v9.1.11
						THROW(P_BObj->__TurnPacket(&pack, 0, 0, 0));
						Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					}
				}
			}
			PPWaitPercent(i + 1, Bills.getCount());
		}
		THROW(tra.Commit());
		if(P_Btd) {
			ot_ctx.Cfg.Flags |= (DBDXF_TURNTOTALDEFICITE|DBDXF_CALCTOTALDEFICITE);
			ot_ctx.Cfg.Flags &= ~(DBDXF_SUBSTDEFICITGOODS|DBDXF_TWOPASSRCV);
			THROW(P_Btd->ProcessDeficit(&ot_ctx, 0));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::ResolveINN(const char * pINN, PPID dlvrLocID, const char * pDlvrLocCode,
	const char * pBillId, PPID accSheetID, PPID * pArID, int logErr /*=1*/)
{
	int    ok = -1;
	PPID   ar_id = 0;
	PPID   reg_type_id = PPREGT_TPID;
	SString code = pINN;
	if(!code.NotEmptyS()) {
		if(PPObjArticle::GetSearchingRegTypeID(accSheetID, 0, 1, &reg_type_id) > 0)
			code = BillParam.Object1SrchCode;
	}
	if(code.NotEmptyS()) {
		PPIDArray psn_list, ar_list;
		THROW(PsnObj.GetListByRegNumber(reg_type_id, 0, code, psn_list));
		THROW(ArObj.GetByPersonList(accSheetID, &psn_list, &ar_list));
		if(ar_list.getCount()) {
			// @v9.1.1 {
			if(ar_list.getCount() > 1) {
				PPID   dlvr_loc_id = 0;
				LocationTbl::Rec loc_rec;
				if(dlvrLocID && PsnObj.LocObj.Search(dlvrLocID, &loc_rec) > 0) {
					uint   _ppos = 0;
                    if(loc_rec.OwnerID && psn_list.lsearch(loc_rec.OwnerID, &_ppos)) {
						ArObj.P_Tbl->PersonToArticle(loc_rec.OwnerID, accSheetID, &ar_id);
                    }
				}
				else if(!isempty(pDlvrLocCode)) {
                    PPIDArray loc_list;
                    PsnObj.LocObj.P_Tbl->GetListByCode(LOCTYP_ADDRESS, pDlvrLocCode, &loc_list);
                    for(uint loc_idx = 0; !ar_id && loc_idx < loc_list.getCount(); loc_idx++) {
						const PPID loc_id = loc_list.get(loc_idx);
                        if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0) {
							uint   _ppos = 0;
							if(loc_rec.OwnerID && psn_list.lsearch(loc_rec.OwnerID, &_ppos))
								ArObj.P_Tbl->PersonToArticle(loc_rec.OwnerID, accSheetID, &ar_id);
                        }
                    }
				}
			}
			// } @v9.1.1
			SETIFZ(ar_id, ar_list.get(0));
			ASSIGN_PTR(pArID, ar_id);
			ok = 1;
		}
	}
	if(ok < 0 && logErr) {
		char   stub[32];
		SString msg, err;
		PPLoadString(PPMSG_ERROR, PPERR_BILLNOTIMPORTED2, msg);
		if(pBillId == 0) {
			stub[0] = 0;
			pBillId = stub;
		}
		Logger.Log(err.Printf((const char*)msg, pBillId, pINN));
	}
	CATCHZOK
	return ok;
}

// @vmiller
int SLAPI PPBillImporter::ResolveGLN(const char * pGLN, const char * pLocCode, const char * pBillId, PPID accSheetID, PPID * pArID, int logErr /*=1*/)
{
	int    ok = -1, r = 0;
	PPID   reg_type_id = 0, ar_id = 0;
	SString code;
	assert(pGLN || pLocCode);
	THROW_PP(pGLN || pLocCode, PPERR_INVPARAM);
	//
	if((code = pGLN).NotEmptyS()) {
		reg_type_id = PPREGT_GLN;
		PPIDArray psn_list, ar_list;
		THROW(PsnObj.GetListByRegNumber(reg_type_id, 0, code, psn_list));
		THROW(ArObj.GetByPersonList(accSheetID, &psn_list, &ar_list));
		if(ar_list.getCount()) {
			ASSIGN_PTR(pArID, ar_list.at(0));
			r = 1;
			ok = 1;
		}
	}
	//
	// Если персоналия не найдена по ее GLN или ее GLN вообще не определена, то ищем по GLN склада
	//
	if(ok < 0) {
		if((code = pLocCode).NotEmptyS()) {
			PPIDArray loc_list;
			LocationTbl::Rec loc_rec;
			LocObj.P_Tbl->GetListByCode(LOCTYP_WAREHOUSE, code, &loc_list);
			if(loc_list.getCount()) {
				if(LocObj.Fetch(loc_list.at(0), &loc_rec)) {
					if(ArObj.GetByPerson(accSheetID, loc_rec.OwnerID, &ar_id) > 0) { // @v9.3.5 @fix (>0)
						ASSIGN_PTR(pArID, ar_id);
						ok = 1;
					}
				}
			}
		}
	}
	if(ok < 0 && logErr) {
		char   stub[32];
		SString msg, err;
		PPLoadString(PPMSG_ERROR, PPERR_BILLNOTIMPORTED2, msg);
		if(pBillId == 0) {
			stub[0] = 0;
			pBillId = stub;
		}
		Logger.Log(err.Printf((const char*)msg, pBillId, ""));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImpExpBaseProcessBlock::SearchEdiOrder(const SearchBlock & rBlk, BillTbl::Rec * pOrderRec)
{
	int    ok = -1;
	PPID   order_bill_id = 0;
	SString order_bill_code = rBlk.Code; // pBill->OrderBillNo;
	SString dlvr_loc_code = rBlk.DlvrLocCode; // pBill->DlvrAddrCode;
	if(P_BObj) {
		if(order_bill_code.NotEmptyS() && dlvr_loc_code.NotEmptyS()) {
			DateRange period;
			period.SetZero();
			if(/*pBill->OrderDate*/rBlk.Dt) {
				period.upp = rBlk.Dt; // pBill->OrderDate;
				period.low = rBlk.Dt; // pBill->OrderDate;
			}
			else {
				// Ограничим поиск документов последней неделей от текущего дня
				period.upp = getcurdate_();
				period.low = plusdate(period.upp, -rBlk.SurveyDays);
			}
			BillCore * t = P_BObj->P_Tbl;
			BillTbl::Key1 k1;
			BillTbl::Rec bill_rec;
			BExtQuery q(t, 1);
			q.select(t->ID, t->Code, t->Object, t->LocID, 0).where(daterange(t->Dt, &period));
			MEMSZERO(k1);
			k1.Dt = period.low;
			for(q.initIteration(0, &k1, spGe); ok < 0 && q.nextIteration() > 0;) {
				t->copyBufTo(&bill_rec);
				if(order_bill_code.CmpNC(bill_rec.Code) == 0) {
					PPFreight freight;
					LocationTbl::Rec loc_rec;
					// Смотрим адрес доставки. Он может быть в DlvrAddrCode и в LocCode
					if(t->GetFreight(bill_rec.ID, &freight) && LocObj.Fetch(freight.DlvrAddrID, &loc_rec) > 0 && dlvr_loc_code.CmpNC(loc_rec.Code) == 0) {
						order_bill_id = bill_rec.ID;
						ok = 1;
					}
					else if(LocObj.Fetch(bill_rec.LocID, &loc_rec) > 0 && dlvr_loc_code.CmpNC(loc_rec.Code) == 0) {
						order_bill_id = bill_rec.ID;
						ok = 1;
					}
				}
			}
			if(ok > 0) {
				if(P_BObj->Search(order_bill_id, &bill_rec) > 0) {
					ASSIGN_PTR(pOrderRec, bill_rec);
				}
				else
					ok = -1;
			}
		}
		else
			ok = -2;
	}
	return ok;
}
//
// @vmiller
// Descr: На данный момент проверяет наличие документа заказа (только драфт-документы), чьи параметры переданы в pBill
//
int SLAPI PPBillImporter::GetDocImpStatus(Sdr_Bill * pBill, Sdr_DllImpObjStatus & rStatus)
{
	int    ok = 1;
	rStatus.DocStatus = statNoSuchDoc;
	if(pBill) {
		SString msg_buf, temp_buf;
		PPLoadText(PPTXT_EDIBILLRCVD, msg_buf);
		msg_buf.Space().CatChar('[').Cat(pBill->EdiOpSymb).CatChar(']');
		msg_buf.CatDiv(':', 2).CatEq("CODE", (temp_buf = pBill->Code).Transf(CTRANSF_OUTER_TO_INNER)).CatDiv(',', 2).CatEq("DATE", pBill->Date);
		if(pBill->EdiOp == PPEDIOP_ALCODESADV) {
			PPBillPacket pack;
			if(BillToBillRec(pBill, &pack)) {
				PPID   main_bill_id = 0;
				BillTbl::Rec main_bill_rec;
				if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &main_bill_id, &main_bill_rec) > 0) {
					PPLoadText(PPTXT_EDIRELMAINFOUND, temp_buf);
					temp_buf.Space().CatChar('(').CatEq("ID", main_bill_id).CatChar(')');
					rStatus.DocStatus = statIsSuchDoc;
				}
				else {
					PPLoadText(PPTXT_EDIRELMAINNFOUND, temp_buf);
				}
			}
		}
		else {
			msg_buf.CatDiv(',', 2).CatEq("ORDERCODE", (temp_buf = pBill->OrderBillNo).Transf(CTRANSF_OUTER_TO_INNER)).
				CatDiv(',', 2).CatEq("ORDERDATE", pBill->OrderDate);
			BillTbl::Rec order_bill_rec;
			SearchBlock sblk;
			sblk.Code = pBill->OrderBillNo;
			sblk.DlvrLocCode = pBill->DlvrAddrCode;
			sblk.Dt = pBill->OrderDate;
			sblk.SurveyDays = 7;
			int   sr = SearchEdiOrder(/*pBill*/sblk, &order_bill_rec);
			if(sr > 0) {
				ltoa(order_bill_rec.ID, pBill->OrderBillID, 10);
				PPLoadText(PPTXT_EDIRELORDERFOUNT, temp_buf);
				temp_buf.Space().CatChar('(').CatEq("ID", pBill->OrderBillID).CatChar(')');
				rStatus.DocStatus = statIsSuchDoc;
			}
			else {
				PPLoadText(PPTXT_EDIRELORDERNFOUNT, temp_buf);
			}
		}
        msg_buf.CatDiv(';', 2).Cat(temp_buf);
		Logger.Log(msg_buf);
	}
	return ok;
}

int SLAPI PPBillImporter::CheckBill(const Sdr_Bill * pBill)
{
	int    ok = 0;
	if(pBill) {
		PPBillPacket pack;
		if(BillToBillRec(pBill, &pack) /* @v8.4.8{*/ && Period.CheckDate(pack.Rec.Dt) /*}*/) {
			if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, 0, 0) <= 0)
				ok = 1;
			else
				Logger.LogMsgCode(mfError, PPERR_DOC_ALREADY_EXISTS, pBill->ID);
		}
	}
	return ok;
}

int SLAPI PPBillImporter::BillToBillRec(const Sdr_Bill * pBill, PPBillPacket * pPack)
{
	int    ok = 0;
	if(pBill && pPack) {
		SString temp_buf;
		PPID   ar_id = 0;
		ArticleTbl::Rec ar_rec;
		PPOprKind op_rec;
		THROW(GetOpData(OpID, &op_rec) > 0);
		const PPID acs_id = op_rec.AccSheetID;
		if(acs_id) {
			if(pBill->CntragID && ArObj.Search(pBill->CntragID, &ar_rec) > 0 && ar_rec.AccSheetID == acs_id)
				ar_id = pBill->CntragID;
			// @v9.0.9 {
			else if(pBill->CntragNo && acs_id && ArObj.P_Tbl->SearchNum(acs_id, pBill->CntragNo, &ar_rec) > 0)
				ar_id = ar_rec.ID;
			// } @v9.0.9
			else if(pBill->RegistryCode[0]) {
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(acs_rec.ObjGroup, &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
						ArObj.SearchByRegCode(acs_id, pk_rec.CodeRegTypeID, pBill->RegistryCode, &ar_id, 0);
					}
				}
			}
			if(!ar_id) {
				// Так, даже если персоналия найдена по GLN, выведет ошибку, что она не найдена по INN
				//if(!ar_id)
				//	ResolveINN(pBill->INN, pBill->ID, acs_id, &ar_id);
				//if(!ar_id /*&& (!isempty(pBill->GLN) || !isempty(pBill->LocCode))*/)
				//	ResolveGLN(pBill->GLN, pBill->LocCode, pBill->ID, acs_id, &ar_id);
				//
				// А вот так ошибка выведется, только если она есть
				// Если INN пустое, то ищем по GLN
				if(!ar_id && isempty(pBill->INN) && (!isempty(pBill->GLN) || !isempty(pBill->LocCode)))
					ResolveGLN(pBill->GLN, pBill->LocCode, pBill->ID, acs_id, &ar_id);
				// Иначе ищем по INN
				else if(!ar_id)
					ResolveINN(pBill->INN, pBill->DlvrAddrID, pBill->DlvrAddrCode, pBill->ID, acs_id, &ar_id);
			}
			// @v9.3.5 {
			if(!ar_id && pBill->DlvrAddrID) {
				LocationTbl::Rec loc_rec;
				PPID   temp_ar_id = 0;
				if(LocObj.Fetch(pBill->DlvrAddrID, &loc_rec) && ArObj.GetByPerson(acs_id, loc_rec.OwnerID, &temp_ar_id) > 0)
					ar_id = temp_ar_id;
			}
			// } @v9.3.5
		}
		if(ar_id || !acs_id) {
			pPack->Rec.Object = ar_id;
			const PPID psn_id = ObjectToPerson(ar_id, 0); // @v8.6.11
			if(op_rec.AccSheet2ID) {
				ArticleTbl::Rec ar_rec;
				if(pBill->Obj2No && ArObj.P_Tbl->SearchNum(op_rec.AccSheet2ID, pBill->Obj2No, &ar_rec) > 0) {
					pPack->Rec.Object2 = ar_rec.ID;
				}
				else if(pBill->Obj2INN[0]) {
					PPID   obj2id = 0;
					if(ResolveINN(pBill->Obj2INN, 0, 0, pBill->ID, op_rec.AccSheet2ID, &obj2id, 0) > 0)
						pPack->Rec.Object2 = obj2id;
				}
				// @vmiller {
				else if(pBill->Obj2GLN[0]) {
					PPID   obj2id = 0;
					if(ResolveGLN(pBill->Obj2GLN, 0, pBill->ID, op_rec.AccSheet2ID, &obj2id, 0) > 0)
						pPack->Rec.Object2 = obj2id;
				}
				// } @vmiller
			}
			SETIFZ(pPack->Rec.Dt, pBill->Date);
			SETIFZ(pPack->Rec.OpID, OpID);
			SETIFZ(pPack->Rec.LocID, LocID);
			// @vmiller {
			if(isempty(pBill->LocID) && !isempty(pBill->LocCode)) {
				PPID loc_id = 0;
				if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, pBill->LocCode, &loc_id) > 0) // @v8.6.11 @fix (!=0)-->(>0)
					pPack->Rec.LocID = loc_id;
			}
			if(!isempty(pBill->OrderBillID)) {
				PPID   link_bill_id = atol(pBill->OrderBillID);
				BillTbl::Rec link_bill_rec;
				if(P_BObj->Search(link_bill_id, &link_bill_rec) > 0 && GetOpType(link_bill_rec.OpID) == PPOPT_DRAFTRECEIPT)
					pPack->Rec.LinkBillID = link_bill_id;
			}
			// } @vmiller
			pPack->Rec.DueDate = pBill->DueDate;
			STRNSCPY(pPack->Rec.Code, pBill->Code);
			if(pBill->InvoiceCode[0]) {
				STRNSCPY(pPack->Ext.InvoiceCode, pBill->InvoiceCode);
			}
			if(pBill->InvoiceDate)
				pPack->Ext.InvoiceDate = pBill->InvoiceDate;
			pPack->Rec.Amount  = pBill->Amount;
			pPack->Rec.CurID   = pBill->CurID;
			pPack->Rec.CRate   = pBill->CRate;
			if(checkdate(pBill->PaymDate, 0) && pBill->Amount)
				pPack->SetPayDate(pBill->PaymDate, pBill->Amount);
			STRNSCPY(pPack->Rec.Memo, (temp_buf = pBill->Memo).Transf(CTRANSF_OUTER_TO_INNER));
			(temp_buf = pPack->Rec.Code).Transf(CTRANSF_OUTER_TO_INNER);
			STRNSCPY(pPack->Rec.Code, temp_buf);
			(temp_buf = pPack->Ext.InvoiceCode).Transf(CTRANSF_OUTER_TO_INNER);
			STRNSCPY(pPack->Ext.InvoiceCode, temp_buf);
			if(pBill->AgentINN[0]) {
				PPID   agent_id = 0;
				if(ResolveINN(pBill->AgentINN, 0, 0, pBill->ID, GetAgentAccSheet(), &agent_id, 0) > 0)
					pPack->Ext.AgentID = agent_id;
			}
			// @vmiller {
			else if(pBill->AgentGLN[0]) {
				PPID	agent_id = 0;
				if(ResolveGLN(pBill->AgentGLN, 0, pBill->ID, GetAgentAccSheet(), &agent_id, 0) > 0)
					pPack->Ext.AgentID = agent_id;
			}
			// @v8.6.11 {
			{
				int    dlvr_loc_id_proof = 0;
				if(psn_id && (pBill->DlvrAddrID || pBill->DlvrAddrCode[0])) {
					LocationTbl::Rec dlvr_loc_rec;
					if(pBill->DlvrAddrID && LocObj.Search(pBill->DlvrAddrID, &dlvr_loc_rec) > 0) {
						if(dlvr_loc_rec.OwnerID == psn_id)
							dlvr_loc_id_proof = dlvr_loc_rec.ID;
					}
					if(!dlvr_loc_id_proof && pBill->DlvrAddrCode[0]) {
						PPID loc_id = 0;
						if(LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, pBill->DlvrAddrCode, &loc_id) > 0) {
							if(LocObj.Search(loc_id, &dlvr_loc_rec) > 0) {
								if(dlvr_loc_rec.OwnerID == psn_id)
									dlvr_loc_id_proof = dlvr_loc_rec.ID;
							}
						}
					}
				}
				if(dlvr_loc_id_proof) {
					if(op_rec.Flags & OPKF_FREIGHT) {
						PPFreight freight;
						freight.DlvrAddrID = dlvr_loc_id_proof;
						pPack->SetFreight(&freight);
					}
				}
			}
			// } @v8.6.11
			if(Flags & PPBillImporter::fEdiImpExp)
				pPack->Rec.Flags2 |= BILLF2_DONTCLOSDRAFT;
			// } @vmiller
			pPack->Rec.EdiOp = (int16)pBill->EdiOp; // @v8.6.12
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::BillRowToBillRec(const Sdr_BRow * pRow, PPBillPacket * pPack)
{
	int    ok = 0;
	if(pRow && pPack) {
		PPID   ar_id = 0;
		ArticleTbl::Rec ar_rec;
		PPOprKind op_rec;
		THROW(GetOpData(OpID, &op_rec) > 0);
		const PPID acs_id = op_rec.AccSheetID;
		if(acs_id) {
			if(pRow->CntragID && ArObj.Search(pRow->CntragID, &ar_rec) > 0 && ar_rec.AccSheetID == acs_id)
				ar_id = pRow->CntragID;
			else if(pRow->RegistryCode[0]) {
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				if(acs_obj.Fetch(acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(acs_rec.ObjGroup, &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
						ArObj.SearchByRegCode(acs_id, pk_rec.CodeRegTypeID, pRow->RegistryCode, &ar_id, 0);
					}
				}
			}
			if(!ar_id)
				ResolveINN(pRow->INN, pRow->DlvrAddrID, pRow->DlvrAddrCode, pRow->BillID, acs_id, &ar_id);
		}
		if(ar_id || !acs_id) {
			pPack->Rec.Object = ar_id;
			if(op_rec.AccSheet2ID) {
				ArticleTbl::Rec ar_rec;
				if(pRow->Obj2No && ArObj.P_Tbl->SearchNum(op_rec.AccSheet2ID, pRow->Obj2No, &ar_rec) > 0) {
					pPack->Rec.Object2 = ar_rec.ID;
				}
				else if(pRow->Obj2INN[0]) {
					PPID   obj2id = 0;
					if(ResolveINN(pRow->Obj2INN, 0, 0, pRow->BillID, op_rec.AccSheet2ID, &obj2id, 0) > 0)
						pPack->Rec.Object2 = obj2id;
				}
			}
			SETIFZ(pPack->Rec.Dt, pRow->BillDate);
			SETIFZ(pPack->Rec.OpID, OpID);
			SETIFZ(pPack->Rec.LocID, LocID);
			// @vmiller {
			if(isempty(pRow->LocID) && !isempty(pRow->LocCode)) {
				PPID loc_id = 0;
				if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, pRow->LocCode, &loc_id))
					pPack->Rec.LocID = loc_id;
			}
			// } @vmiller
			pPack->Rec.DueDate = pRow->DueDate;
			STRNSCPY(pPack->Rec.Code, pRow->BillCode);
			SCharToOem(pPack->Rec.Memo);
			SCharToOem(pPack->Rec.Code);
			SCharToOem(pPack->Ext.InvoiceCode);
			if(pRow->AgentINN[0]) {
				PPID   agent_id = 0;
				if(ResolveINN(pRow->AgentINN, 0, 0, pRow->BillID, GetAgentAccSheet(), &agent_id, 0) > 0)
					pPack->Ext.AgentID = agent_id;
			}
			// @vmiller {
			if(Flags & PPBillImporter::fEdiImpExp)
				pPack->Rec.Flags2 |= BILLF2_DONTCLOSDRAFT;
			// } @vmiller
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillImporter::Run()
{
	int    ok = 1;
	LineIdSeq = 0;
	PPWait(1);
	Period.Actualize(ZERODATE);
	ToRemoveFiles.clear();
	if(Flags & PPBillImporter::fUhttImport) {
		THROW(RunUhttImport());
	}
	else if(Flags & PPBillImporter::fEdiImpExp) {
		THROW(RunEDIImport());
	}
	else if(Flags & PPBillImporter::fEgaisImpExp) {
		PPEgaisProcessor ep(((Flags & PPBillImporter::fTestMode) ? PPEgaisProcessor::cfDebugMode : 0), &Logger);
		THROW(ep);
		if(Flags & PPBillImporter::fDontRemoveTags)
			ep.SetNonRvmTagMode(1);
		THROW(ep.CheckLic());
		{
			PPEgaisProcessor::SendBillsParam sbp;
			sbp.LocID = LocID;
			sbp.Period = Period;
			TSArray <PPEgaisProcessor::UtmEntry> utm_list;
			THROW(ep.GetUtmList(LocID, utm_list));
			PPWait(1);
			for(uint i = 0; i < utm_list.getCount(); i++) {
                ep.SetUtmEntry(LocID, &utm_list.at(i), &Period);
				ep.SendBillActs(sbp);
				ep.SendBillRepeals(sbp); // @v9.2.8
				ep.SendBills(sbp);
				ep.ReadInput(LocID, &Period, 0);
				ep.SetUtmEntry(0, 0, 0);
			}
			PPWait(0);
		}
	}
	else {
		THROW(ReadData());
		THROW(Import(1));
		// @v8.4.7 {
		{
			SString file_name;
            for(uint i = 0; ToRemoveFiles.get(&i, file_name);) {
				if(fileExists(file_name))
					SFile::Remove(file_name);
            }
		}
		// } @v8.4.7
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI ImportBills(PPBillImpExpParam * pBillParam, PPBillImpExpParam * pBRowParam, PPID opID, PPID locID)
{
	int    ok = -1, r = 1;
	PPBillImporter b_i;
	THROW(r = b_i.Init(pBillParam, pBRowParam, opID, locID));
	if(r > 0) {
		THROW(b_i.Run());
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
// Export
//
class BillExpExprEvalContext : public BillContext {
public:
	SLAPI  BillExpExprEvalContext(const PPBillPacket * pPack, const SdRecord & rRec) :
		BillContext(pPack, pPack->Rec.CurID, 0), R_Rec(rRec)
	{
	}
	virtual int SLAPI Resolve(const char * pSymb, double * pVal)
	{
		if(PPImpExp::ResolveVarName(pSymb, R_Rec, pVal) > 0)
			return 1;
		else {
			TempBuf = pSymb;
			TempBuf.ReplaceChar('@', ' ').Strip();
			return BillContext::Resolve(TempBuf, pVal);
		}
	}
private:
	const SdRecord & R_Rec;
	SString TempBuf;
};

// @vmiller
class SignBillDialog : public TDialog {
public:
	SignBillDialog(const char * pFileName) : TDialog(DLG_SIGNBILL)
	{
		CurPosInList = 0;
		FileName = pFileName;
		SignFileName = 0;
		SignerName = 0;
		SetupStrListBox(this, CTL_SIGNBILL_SIGNER);
		SetupStrListBox(this, CTL_SIGNBILL_SIGNFILE);
		DrawList();
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCtlEvent(CTL_SIGNBILL_SIGNER) && event.isCmd(cmLBItemFocused)) {
			SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_SIGNBILL_SIGNER);
			if(p_list) {
				p_list->getCurID(&CurPosInList);
				p_list->getCurString(SignerName = 0);
			}
		}
		else if(event.isCtlEvent(CTL_SIGNBILL_SIGNFILE) && event.isCmd(cmLBItemFocused)) {
			SmartListBox * p_list = (SmartListBox*)getCtrlView(CTL_SIGNBILL_SIGNFILE);
			if(p_list) {
				p_list->getCurString(SignFileName = 0);
				SPathStruc spath, sign_spath;
				spath.Split(FileName);
				sign_spath.Split(SignFileName);
				spath.Nam = sign_spath.Nam;
				spath.Ext = sign_spath.Ext;
				spath.Merge(SignFileName = 0);
			}
		}
		else if(event.isCmd(cmSignBill)) {
			SString msg_buf, fmt_buf;
			if(SignerName.NotEmpty() && FileName.NotEmpty()) {
				if(Eds.SignData(SignerName.Transf(CTRANSF_INNER_TO_OUTER), FileName, SignFileName)) {
					PPLoadText(PPTXT_EDS_FILESIGNED, fmt_buf);
					msg_buf.Printf(fmt_buf, (const char *)FileName, (const char *)SignFileName);
					PPOutputMessage(msg_buf, mfYes | mfLargeBox);
				}
				else
					PPError();
			}
			else {
				PPLoadText(PPTXT_EDS_NOFILEORSIGNOWNER, msg_buf);
				PPOutputMessage(msg_buf, mfYes | mfLargeBox);
			}

		}
		clearEvent(event);
	}
	int    DrawList();

	long   CurPosInList;
	SString FileName;
	SString SignFileName;
	SString SignerName;
	PPEds Eds;
};

// @vmiller
int SignBillDialog::DrawList()
{
	int    ok = 1;
	StrAssocArray names_arr, files_arr;
	SString signer_name, file_name;
	SmartListBox * p_list = 0;
	{
		p_list = (SmartListBox*)getCtrlView(CTL_SIGNBILL_SIGNER);
		if(p_list) {
			THROW(Eds.GetSignerNamesInStore(names_arr));
			if(p_list) {
				for(uint i = 0; i < names_arr.getCount(); i++) {
					names_arr.Get(i, signer_name = 0);
					THROW_SL(p_list->addItem(i, signer_name.Transf(CTRANSF_OUTER_TO_INNER)));
				}
			}
			p_list->draw();
		}
	}
	{
		p_list = (SmartListBox*)getCtrlView(CTL_SIGNBILL_SIGNFILE);
		if(p_list) {
			THROW(Eds.GetSignFilesForDoc(FileName, files_arr));
			if(p_list) {
				for(uint i = 0; i < files_arr.getCount(); i++) {
					files_arr.Get(i, file_name = 0);
					THROW_SL(p_list->addItem(i, file_name.Transf(CTRANSF_OUTER_TO_INNER)));
				}
			}
			p_list->draw();
		}
	}
	CATCHZOKPPERR;
	return ok;
}

SLAPI PPBillExporter::PPBillExporter()
{
	P_IEBill = 0;
	P_IEBRow = 0;
}

SLAPI PPBillExporter::~PPBillExporter()
{
	Destroy();
}

void SLAPI PPBillExporter::Destroy()
{
	const int del_row = BIN(P_IEBill != P_IEBRow);
	ZDELETE(P_IEBill);
	if(del_row)
		ZDELETE(P_IEBRow);
}

int SLAPI PPBillExporter::Init(const PPBillImpExpParam * pBillParam, const PPBillImpExpParam * pBRowParam,
	const PPBillPacket * pFirstPack, StringSet * pResultFileList)
{
	Destroy();

	int    ok = 1;
	SString temp_buf;
	// Init();
	PPBillImpExpBaseProcessBlock::Reset();
	RVALUEPTR(BillParam, pBillParam);
	RVALUEPTR(BRowParam, pBRowParam);
	if(!pBillParam || !pBRowParam) {
		if(!pBillParam)
			THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		if(!pBRowParam)
			THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		if(pFirstPack && !Tp.AddrList.getCount()) {
			if(pFirstPack->GetContextEmailAddr(temp_buf) > 0)
				Tp.AddrList.Add(1, temp_buf);
		}
		THROW(ok = Select(0));
	}
	if(ok > 0) {
		if(Flags & fPaymOrdersExp) {
		}
		else {
			THROW_MEM(P_IEBill = new PPImpExp(&BillParam, pFirstPack));
			THROW(!P_IEBill->IsCtrError());
			if(!(BillParam.BaseFlags & PPImpExpParam::bfDLL) && !(Flags & fEgaisImpExp)) {
				if(BillParam.DataFormat == PPImpExpParam::dfXml && BRowParam.DataFormat == PPImpExpParam::dfXml && BillParam.FileName.CmpNC(BRowParam.FileName) == 0) {
					P_IEBRow = P_IEBill;
				}
				else {
					THROW_MEM(P_IEBRow = new PPImpExp(&BRowParam, pFirstPack));
					THROW(!P_IEBRow->IsCtrError());
					THROW(P_IEBRow->OpenFileForWriting(0, 1, pResultFileList));
				}
				// @v7.1.10 Если имя файла задано шаблоном, то оно могло измениться { //
				BillParam.FileName = P_IEBill->GetParam().FileName;
				BRowParam.FileName = P_IEBRow->GetParam().FileName;
				// }
				THROW(P_IEBill->OpenFileForWriting(0, 1, pResultFileList));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPBillExporter::SignBill()
{
	int    ok = 1;
	TDialog * p_sign_dlg = 0;
	if(Flags & fSignExport) {
		SString file_name = BillParam.FileName;
		THROW(CheckDialogPtr(&(p_sign_dlg = new SignBillDialog(file_name)), 0));
		ExecView(p_sign_dlg);
	}
	CATCHZOK
	delete p_sign_dlg;
	return ok;
}

int SLAPI PPBillExporter::PutPacket(PPBillPacket * pPack, int sessId /*=0*/, ImpExpDll * pImpExpDll /*=0*/)
{
	int    ok = -1, obj_id = 0, use_dll = 0;
	const  int use_ar_code = BIN(pPack->Rec.Object && (CConfig.Flags & CCFLG_USEARGOODSCODE));
	PPObjWorld world_obj;
	SString temp_buf, err_msg;
	Sdr_Bill bill;
	PPTransferItem * p_ti = 0;
	ReceiptCore * p_rcpt = (P_BObj && P_BObj->trfr) ? &P_BObj->trfr->Rcpt : 0;
	GTaxVect vect;
	THROW_PP(pPack && P_IEBill && (P_IEBRow || (sessId && pImpExpDll)), PPERR_INVPARAM);
	{
		const SString edi_op_symb = BillParam.ImpExpParamDll.OperType;
		SString ttn_tag_value;
		if(BillParam.ImpExpParamDll.TTNTagID) {
			ObjTagItem tag_item;
			if(PPRef->Ot.GetTag(PPOBJ_BILL, pPack->Rec.ID, BillParam.ImpExpParamDll.TTNTagID, &tag_item) > 0)
				tag_item.GetStr(ttn_tag_value);
		}
		if(sessId && pImpExpDll) {
            int    edi_op = 0;
            if(edi_op_symb.CmpNC("RECADV") == 0)
				edi_op = PPEDIOP_RECADV;
			else if(edi_op_symb.CmpNC("ORDER") == 0)
				edi_op = PPEDIOP_ORDER;
			PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, temp_buf = 0);
            pPack->Rec.EdiOp = edi_op; // @v8.6.12
			THROW(BillRecToBill(pPack, &bill));
			THROW_PP_S(edi_op != PPEDIOP_RECADV || bill.DesadvBillNo[0], PPERR_EDI_SPRRECADVWOTTN, temp_buf);
			// Если отклоненный заказ
			if(pPack->Rec.Flags2 & BILLF2_DECLINED) {
				SString decline_order;
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_DECLNORDER, decline_order));
				THROW(pImpExpDll->SetExportObj(sessId, "BILLS", &bill, &obj_id, decline_order));
			}
			else {
				THROW(pImpExpDll->SetExportObj(sessId, "BILLS", &bill, &obj_id, edi_op_symb));
			}
			THROW(pImpExpDll->InitExportIter(sessId, obj_id));
		}
		else {
			BillExpExprEvalContext expr_ctx(pPack, P_IEBill->GetParam().InrRec);
			P_IEBill->SetExprContext(&expr_ctx);
			THROW(BillRecToBill(pPack, &bill));
			THROW(P_IEBill->AppendRecord(&bill, sizeof(bill)));
			P_IEBRow->Push(&BRowParam);
		}
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti) > 0;) {
			const PPID goods_id = labs(p_ti->GoodsID);
			Goods2Tbl::Rec goods_rec;
			Sdr_BRow brow;
			BarcodeArray bcd_ary;
			MEMSZERO(brow);
			STRNSCPY(brow.BillID, bill.ID);
			brow.LineNo = i; // @v8.1.2 // @v8.1.12 @fix (i+1)-->i
			brow.GoodsID = goods_id;
			brow.LotID = p_ti->LotID; // @v7.4.11
			if(GObj.Fetch(goods_id, &goods_rec) <= 0)
				MEMSZERO(goods_rec);
			(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.GoodsName, sizeof(brow.GoodsName));
			if(use_ar_code) {
				if(GObj.P_Tbl->GetArCode(pPack->Rec.Object, goods_id, temp_buf, 0) > 0)
					temp_buf.CopyTo(brow.ArCode, sizeof(brow.ArCode));
				// @v9.1.11 {
				if(GObj.P_Tbl->GetArCode(0, goods_id, temp_buf, 0) > 0)
					temp_buf.CopyTo(brow.ArCodeOwn, sizeof(brow.ArCodeOwn));
				// } @v9.1.11
			}
			if(GObj.ReadBarcodes(goods_id, bcd_ary) > 0) {
				bcd_ary.GetSingle(temp_buf);
				temp_buf.CopyTo(brow.Barcode, sizeof(brow.Barcode));
			}
			// @v8.2.3 {
			if(goods_rec.ManufID) {
				PPID   country_id = 0;
				PPCountryBlock mcb;
				PsnObj.GetCountry(goods_rec.ManufID, &country_id, &mcb);
				mcb.Name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.ManufCountryName, sizeof(brow.ManufCountryName)); // @v8.2.4 ToChar()
				mcb.Code.CopyTo(brow.ManufCountryOKSM, sizeof(brow.ManufCountryOKSM));
			}
			// } @v8.2.3
			{
				PPID   org_lot_id = 0;
				ReceiptTbl::Rec org_lot_rec;
				if(p_rcpt->SearchOrigin(brow.LotID, &org_lot_id, 0, &org_lot_rec) > 0) {
					//
					// Ищем дату документа из лота
					//
					BillTbl::Rec bill_rec;
					ObjTagList org_lot_tag_list;
					//
					// В дальнейшем нам понадобится список тегов лота
					//
					P_BObj->GetTagListByLot(org_lot_id, 0, &org_lot_tag_list);
					if(P_BObj->Search(org_lot_rec.BillID, &bill_rec) > 0)
						brow.LotDocDate = bill_rec.Dt;
					//
					// Идентифицируем вид товарной продукции по тегу оригинального лота #1
					//
					if(BillParam.ImpExpParamDll.GoodsKindTagID) {
						PPID   gds_kind_tag_id = BillParam.ImpExpParamDll.GoodsKindTagID;
						const  ObjTagItem * p_goods_kind_tag = org_lot_tag_list.GetItem(gds_kind_tag_id);
						if(p_goods_kind_tag) {
							long   kind_code = 0;
							p_goods_kind_tag->GetInt(&kind_code);
							brow.GoodKindCode = kind_code; // Код вида товара
						}
					}
					{
						//
						// Смотрим ИД производителя/импортера, его имя, ИНН и КПП
						//
						PersonTbl::Rec pers_rec;
						PersonReq persrec;
						RegisterTbl::Rec reg_rec;
						const  ObjTagItem * p_manuf_tag = org_lot_tag_list.GetItem(BillParam.ImpExpParamDll.ManufTagID);
						if(p_manuf_tag) {
							long   manuf_id = 0;
							int r = 0;
							p_manuf_tag->GetInt(&manuf_id);
							if(PsnObj.Search(manuf_id, &pers_rec) > 0)
								(temp_buf = pers_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.LotManuf, sizeof(brow.LotManuf));
							if(PsnObj.GetPersonReq(manuf_id, &persrec) > 0) {
								STRNSCPY(brow.ManufINN, persrec.TPID);
								STRNSCPY(brow.ManufKPP, persrec.KPP);
							}
							// Если не нашли ИНН здесь, то, скорее всего это филиал. Тогда смотрим ИНН у головной организации
							if(isempty(persrec.TPID)) {
								PPIDArray id_list;
								if(PsnObj.GetRelPersonList(manuf_id, PPPRK_MAIN, 0, &id_list) > 0) {
									// Ищем по первой записи в отношениях
									if(PsnObj.Search(id_list.get(0), &pers_rec) > 0)
										if(PsnObj.GetPersonReq(pers_rec.ID, &persrec) > 0)
											STRNSCPY(brow.ManufINN, persrec.TPID);
								}
							}
							//
							// Смотрим, является ли персоналия производителем
							//
							if(BillParam.ImpExpParamDll.IsManufTagID) {
								ObjTagItem tag_item;
								if(PPRef->Ot.GetTag(PPOBJ_PERSON, manuf_id, BillParam.ImpExpParamDll.IsManufTagID, &tag_item) > 0) {
									long   is_manuf = 0;
									tag_item.GetInt(&is_manuf);
									brow.IsManuf = is_manuf;
								}
							}
							//
							// Смотрим адрес производителя
							//
							LocationTbl::Rec loc_rec;
							WorldTbl::Rec world_rec;
							PPLocAddrStruc loc_addr_st;
							SString address, city_name, region_name;
							MEMSZERO(world_rec);
							MEMSZERO(loc_rec);
							if(pers_rec.MainLoc)
								r = LocObj.Fetch(pers_rec.MainLoc, &loc_rec);
							else if(pers_rec.RLoc)
								r = LocObj.Fetch(pers_rec.RLoc, &loc_rec);
							if(r && loc_rec.CityID && world_obj.Fetch(loc_rec.CityID, &world_rec) > 0) {
								city_name = world_rec.Name;
								while(world_rec.ParentID > 0) {
									if(world_obj.Search(world_rec.ParentID, &world_rec) > 0) {
										if(world_rec.ZIP[0]) {
											region_name = world_rec.Name;
											world_rec.ParentID = 0;
										}
									}
									else
										world_rec.ParentID = 0;
								}
							}
							PsnObj.GetAddress(manuf_id, address);
							loc_addr_st.Recognize(address.Transf(CTRANSF_INNER_TO_OUTER));
							if(world_rec.ZIP[0] == 0)
								if(loc_addr_st.Get(PPLocAddrStruc::tZip, temp_buf = 0))
									temp_buf.CopyTo(world_rec.ZIP, sizeof(world_rec.ZIP));
							STRNSCPY(brow.ManufIndex, world_rec.ZIP);
							brow.ManufRegionCode = BillParam.ImpExpParamDll.ManufRegionCode; // Вытаскиваем из ini-файла
							if(region_name.Empty())
								if(loc_addr_st.Get(PPLocAddrStruc::tLocalArea, temp_buf))
									temp_buf.CopyTo(brow.ManufDistrict, sizeof(brow.ManufDistrict));
							if(city_name.Empty()) {
								if(loc_addr_st.Get(PPLocAddrStruc::tCity, temp_buf = 0))
									temp_buf.CopyTo(brow.ManufCityName, sizeof(brow.ManufCityName));
							}
							else
								city_name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.ManufCityName, sizeof(brow.ManufCityName));
							if(loc_addr_st.Get(PPLocAddrStruc::tStreet, temp_buf = 0))
								temp_buf.CopyTo(brow.ManufStreet, sizeof(brow.ManufStreet));
							if(loc_addr_st.Get(PPLocAddrStruc::tHouse, temp_buf = 0))
								brow.ManufHouse = temp_buf.ToLong();
							if(loc_addr_st.Get(PPLocAddrStruc::tCorp, temp_buf = 0))
								brow.ManufHousing = temp_buf.ToLong();
							//
							// Смотрим инфу о лицензии
							//
							if(PsnObj.GetRegister(manuf_id, BillParam.ImpExpParamDll.AlcoLicenseRegID, &reg_rec) > 0) {
								brow.LicenseID = reg_rec.ID;
								STRNSCPY(brow.LicenseNum, reg_rec.Num);
								brow.LicenseDate = reg_rec.Dt;
								brow.LicenseExpiry = reg_rec.Expiry;
								(temp_buf = reg_rec.Serial).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.LicenseSerial, sizeof(brow.LicenseSerial));
								STRNSCPY(brow.LicenseNum, reg_rec.Num);
								if(PsnObj.Search(reg_rec.RegOrgID, &pers_rec) > 0)
									(temp_buf = pers_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.RegAuthority, sizeof(brow.RegAuthority));
							}
						}
					}
				}
				//
				// Если в точке #1 не удалось идентифицировать вид товарной продукции, то определяем ее из классификатора товара
				//
				if(!brow.GoodKindCode) {
					if(BillParam.ImpExpParamDll.GoodsKindSymb.NotEmpty()) {
						SString goods_kind_symb = BillParam.ImpExpParamDll.GoodsKindSymb;
						if(goods_kind_symb.NotEmpty()) {
							GoodsExtTbl::Rec goods_ext_rec;
							if(GObj.P_Tbl->GetExt(goods_rec.ID, &goods_ext_rec)) {
								if(goods_kind_symb.CmpNC("X") == 0)
									brow.GoodKindCode = goods_ext_rec.X;
								else if(goods_kind_symb.CmpNC("Y") == 0)
									brow.GoodKindCode = goods_ext_rec.Y;
								else if(goods_kind_symb.CmpNC("Z") == 0)
									brow.GoodKindCode = goods_ext_rec.Z;
								else if(goods_kind_symb.CmpNC("W") == 0)
									brow.GoodKindCode = goods_ext_rec.W;
							}
						}
					}
				}
				//
				// Проверяем, принадлежит ли товар альтернативной группе "Пиво"
				//
				if(BillParam.ImpExpParamDll.BeerGrpID) {
					if(GObj.BelongToGroup(goods_rec.ID, BillParam.ImpExpParamDll.BeerGrpID))
						brow.IsBeer = 1;
				}
				//
				// Проверяем, принадлежит ли товар альтернативной группе "Алкоголь"
				//
				if(BillParam.ImpExpParamDll.AlcoGrpID) {
					if(GObj.BelongToGroup(goods_rec.ID, BillParam.ImpExpParamDll.AlcoGrpID))
						brow.IsAlco = 1;
					brow.GoodGrpID = goods_rec.ParentID; // Тип товара
				}
				//
				// Запишем номер ТТН
				//
				if(ttn_tag_value.NotEmpty()) {
					STRNSCPY(brow.TTN, ttn_tag_value);
				}
				else if(!isempty(bill.Memo)) {
					STRNSCPY(brow.TTN, bill.Memo);
				}
				//
				// Запишем ГТД
				//
				if(P_BObj->GetClbNumberByLot(org_lot_id, 0, temp_buf) > 0)
					temp_buf.CopyTo(brow.GTD, sizeof(brow.GTD));
			}
			if(p_ti->Flags & PPTFR_REVAL) {
				brow.Quantity = p_ti->Rest_;
				brow.Cost     = p_ti->Cost;
				brow.Price    = p_ti->Price;
				brow.Discount = 0.0;
			}
			else {
				double upp = 0.0;
				brow.Quantity = p_ti->Quantity_;
				{
					upp = p_ti->UnitPerPack;
					if(upp <= 0.0) {
						GoodsStockExt gse;
						if(GObj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
							upp = gse.Package;
					}
					if(upp <= 0.0) {
						if(p_rcpt) {
							ReceiptTbl::Rec lot_rec;
							if(p_rcpt->GetLastLot(p_ti->GoodsID, pPack->Rec.LocID, pPack->Rec.Dt, &lot_rec) > 0)
								upp = lot_rec.UnitPerPack;
						}
					}
					if(upp > 0.0) {
						brow.UnitPerPack = upp;
						brow.PckgQtty = (double)(long)(brow.Quantity / upp);
					}
				}
				brow.Cost     = p_ti->Cost;
				brow.Price    = p_ti->Price;
				brow.Discount = p_ti->Discount;
				if(p_ti->Flags & PPTFR_INDEPPHQTTY)
					brow.PhQtty = p_ti->WtQtty;
				else {
					double phuperu = 0.0;
					//
					// Поиск объема единицы в полях расширения
					//
					if(BillParam.ImpExpParamDll.GoodsVolSymb.NotEmpty()) {
						SString goods_vol_symb = BillParam.ImpExpParamDll.GoodsVolSymb;
						if(goods_vol_symb.NotEmpty()) {
							PPGdsClsPacket gds_cls_pack;
							GoodsExtTbl::Rec goods_ext_rec;
							GObj.FetchCls(goods_rec.ID, 0, &gds_cls_pack);
							if(GObj.P_Tbl->GetExt(goods_rec.ID, &goods_ext_rec)) {
								int    dim = 0;
								if(goods_vol_symb.CmpNC("X") == 0)
									dim = PPGdsCls::eX;
								else if(goods_vol_symb.CmpNC("Y") == 0)
									dim = PPGdsCls::eY;
								else if(goods_vol_symb.CmpNC("Z") == 0)
									dim = PPGdsCls::eZ;
								else if(goods_vol_symb.CmpNC("W") == 0)
									dim = PPGdsCls::eW;
								if(dim)
									gds_cls_pack.GetExtDim(&goods_ext_rec, dim, &phuperu);
							}
						}
					}
					if(phuperu <= 0.0)
						GObj.GetPhUPerU(p_ti->GoodsID, 0, &phuperu);
					brow.PhQtty = p_ti->Quantity_ * phuperu;
				}
			}
			{
				PPUnit u_rec;
				if(GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
					(temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.UnitName, sizeof(brow.UnitName));
					(temp_buf = u_rec.Code).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.UnitCode, sizeof(brow.UnitCode)); // @v8.1.1
				}
				if(GObj.FetchUnit(goods_rec.PhUnitID, &u_rec) > 0)
					(temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.PhUnitName, sizeof(brow.PhUnitName));
			}
			{
				vect.CalcTI(p_ti, pPack->Rec.OpID, TIAMT_PRICE, 0);
				brow.VatRate = vect.GetTaxRate(GTAX_VAT, 0);
				brow.VatSum  = vect.GetValue(GTAXVF_VAT);
				vect.CalcTI(p_ti, pPack->Rec.OpID, TIAMT_COST, 0);
				brow.CVatRate = vect.GetTaxRate(GTAX_VAT, 0);
				brow.CVatSum  = vect.GetValue(GTAXVF_VAT);
			}
			brow.Expiry = p_ti->Expiry;
			pPack->SnL.GetNumber(i-1, &temp_buf);
			temp_buf.CopyTo(brow.Serial, sizeof(brow.Serial));
			pPack->ClbL.GetNumber(i-1, &temp_buf);
			temp_buf.CopyTo(brow.CLB, sizeof(brow.CLB));
			if(p_ti->QCert) {
				QualityCertTbl::Rec qc_rec;
				if(QcObj.Search(p_ti->QCert, &qc_rec) > 0) {
					STRNSCPY(brow.QcCode, (temp_buf = qc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcBc, (temp_buf = qc_rec.BlankCode).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcPrDate, (temp_buf = qc_rec.SPrDate).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcManuf, (temp_buf = qc_rec.Manuf).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcEtc, (temp_buf = qc_rec.Etc).Transf(CTRANSF_INNER_TO_OUTER));
					brow.QcInitDate = qc_rec.InitDate;
					brow.QcExpiry = qc_rec.Expiry;
					if(GetPersonName(qc_rec.RegOrgan, temp_buf) > 0)
						temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.QcRegOrg, sizeof(brow.QcRegOrg));
				}
			}
			{
				//
				// Переносим значения полей шапки документа в строку
				//
				STRNSCPY(brow.BillCode, bill.Code);
				brow.BillDate = bill.Date;
				brow.OpID = bill.OpID;
				STRNSCPY(brow.OpSymb, bill.OpSymb);
				STRNSCPY(brow.InvcCode, bill.InvoiceCode);
				brow.InvcDate = bill.InvoiceDate;
				STRNSCPY(brow.INN, bill.INN);
				brow.CntragID = bill.CntragID;
				STRNSCPY(brow.CntragName, bill.CntragName);
				brow.CntragNo = bill.CntragNo;
				STRNSCPY(brow.RegistryCode, bill.RegistryCode);
				STRNSCPY(brow.Obj2Name, bill.Obj2Name);
				STRNSCPY(brow.Obj2INN, bill.Obj2INN);
				brow.Obj2ID = bill.Obj2ID; // @v8.1.3
				brow.Obj2No = bill.Obj2No;
				brow.DueDate = bill.DueDate;
				brow.PaymDate = bill.PaymDate;
				brow.BillAmt = bill.Amount;
				brow.CRate = bill.CRate;
				brow.CurID = bill.CurID;
				brow.BaseAmount = bill.BaseAmount;
				brow.CostWithSTax = bill.CostWithSTax;
				brow.BillVatSum = bill.VatSum;
				STRNSCPY(brow.BillMemo, bill.Memo);
				STRNSCPY(brow.LocID, bill.LocID);
				STRNSCPY(brow.LocName, bill.LocName);
				STRNSCPY(brow.LocCode, bill.LocCode);
				STRNSCPY(brow.City, bill.City);
				STRNSCPY(brow.Addr, bill.Addr);
				STRNSCPY(brow.OrderBillID, bill.OrderBillID);
				STRNSCPY(brow.OrderBillNo, bill.OrderBillNo);
				brow.DlvrAddrID = bill.DlvrAddrID;
				STRNSCPY(brow.DlvrAddrCode, bill.DlvrAddrCode);
				STRNSCPY(brow.DlvrAddr, bill.DlvrAddr);
				STRNSCPY(brow.AgentName, bill.AgentName);
				STRNSCPY(brow.AgentINN, bill.AgentINN);
			}
			{
				if(sessId && pImpExpDll) {
					THROW(pImpExpDll->NextExportIter(sessId, obj_id, &brow));
				}
				else {
					pPack->SetTPointer(i-1);
					GoodsContext ctx(p_ti, pPack);
					P_IEBRow->SetExprContext(&ctx);
					THROW(P_IEBRow->AppendRecord(&brow, sizeof(brow)));
					P_IEBRow->SetExprContext(0);
				}
			}
		}
		if(!(sessId && pImpExpDll))
			P_IEBRow->Pop();
	}
	CATCHZOK
	if(P_IEBill) {
		//
		// Необходимо обнулить контекст выражений, так как мы передали объект со стека.
		//
		P_IEBill->SetExprContext(0);
	}
	return ok;
}

int SLAPI PPBillExporter::BillRecToBill(const PPBillPacket * pPack, Sdr_Bill * pBill)
{
	int    ok = 0;
	SString temp_buf, str_city;
	if(pBill && pPack) {
		ArticleTbl::Rec ar_rec;
		LocationTbl::Rec loc_rec;
		PPOprKind op_rec;
		memzero(pBill, sizeof(*pBill));
		ltoa(pPack->Rec.ID, pBill->ID, 10);
		GetReg(pPack->Rec.Object, PPREGT_TPID, temp_buf);
		STRNSCPY(pBill->INN, temp_buf);
		GetReg(pPack->Rec.Object, PPREGT_GLN, temp_buf);
		STRNSCPY(pBill->GLN, temp_buf);
		pBill->Date = pPack->Rec.Dt;
		pBill->DueDate = pPack->Rec.DueDate;
		pPack->GetLastPayDate(&pBill->PaymDate);
		(temp_buf = pPack->Rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
		STRNSCPY(pBill->Code, temp_buf);
		temp_buf = pPack->Ext.InvoiceCode[0] ? pPack->Ext.InvoiceCode : pPack->Rec.Code;
		STRNSCPY(pBill->InvoiceCode, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		pBill->InvoiceDate = NZOR(pPack->Ext.InvoiceDate, pPack->Rec.Dt);
		if(GetOpData(pPack->Rec.OpID, &op_rec) > 0) {
			pBill->OpID = op_rec.ID;
			STRNSCPY(pBill->OpSymb, op_rec.Symb);
		}
		pBill->CurID        = pPack->Rec.CurID;
		pBill->CRate        = pPack->Rec.CRate;
		pBill->Amount       = pPack->Rec.Amount;
		pBill->CostWithSTax = BIN(pPack->Rec.Flags & BILLF_RMVEXCISE);
		pBill->VatSum = pPack->Amounts.Get(PPAMT_VATAX, 0L);
		if(pPack->Rec.Object) {
			pBill->CntragID = pPack->Rec.Object;
			if(ArObj.Fetch(pPack->Rec.Object, &ar_rec) > 0) {
				pBill->CntragNo = ar_rec.Article;
				(temp_buf = ar_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->CntragName, sizeof(pBill->CntragName));
				{
					PPID   psn_id = ObjectToPerson(pPack->Rec.Object, 0);
					PPPersonPacket psn_pack;
					if(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0 && psn_pack.GetSrchRegNumber(0, temp_buf) > 0)
						temp_buf.CopyTo(pBill->RegistryCode, sizeof(pBill->RegistryCode));
				}
			}
		}
		if(pPack->Rec.Object2) {
			if(ArObj.Fetch(pPack->Rec.Object2, &ar_rec) > 0) {
				pBill->Obj2ID = ar_rec.ID; // @v8.1.3
				pBill->Obj2No = ar_rec.Article;
				STRNSCPY(pBill->Obj2Name, (temp_buf = ar_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
			}
			GetReg(pPack->Rec.Object2, PPREGT_TPID, temp_buf);
			temp_buf.CopyTo(pBill->Obj2INN, sizeof(pBill->Obj2INN));
			GetReg(pPack->Rec.Object2, PPREGT_GLN, temp_buf);
			temp_buf.CopyTo(pBill->Obj2GLN, sizeof(pBill->Obj2GLN));
		}
		(temp_buf = pPack->Rec.Memo).Transf(CTRANSF_INNER_TO_OUTER);
		STRNSCPY(pBill->Memo, temp_buf);
		if(LocObj.Search(pPack->Rec.LocID, &loc_rec) > 0) {
			ltoa(loc_rec.ID, pBill->LocID, 10);
			(temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->LocName, sizeof(pBill->LocName));
			(temp_buf = loc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->LocCode, sizeof(pBill->LocCode));
			LocObj.GetCity(loc_rec.ID, 0, &str_city, 1); // @v8.7.12 @fix loc_rec.CityID-->loc_rec.ID; useCache=1
			str_city.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->City, sizeof(pBill->City));
			LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
			if(temp_buf.NotEmptyS())
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->Addr, sizeof(pBill->Addr));
			else {
				LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->Addr, sizeof(pBill->Addr));
			}
		}
		{
			PPIDArray ord_bill_list;
			BillTbl::Rec ord_bill_rec;
			const PPID org_link_bill_id = pPack->Rec.LinkBillID;
			// @v8.6.12 {
			if(pPack->Rec.EdiOp == PPEDIOP_RECADV) {
				BillTbl::Rec desadv_bill_rec;
				PPID   desadv_bill_id = 0;
				for(PPID link_bill_id = org_link_bill_id; !desadv_bill_id && P_BObj->Search(link_bill_id, &desadv_bill_rec) > 0; link_bill_id = desadv_bill_rec.LinkBillID) {
					if(desadv_bill_rec.EdiOp == PPEDIOP_DESADV) {
						desadv_bill_id = desadv_bill_rec.ID;
						(temp_buf = desadv_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
						STRNSCPY(pBill->DesadvBillNo, temp_buf);
						STRNSCPY(pBill->DesadvBillID, (temp_buf = 0).Cat(desadv_bill_rec.ID));
						pBill->DesadvBillDt = desadv_bill_rec.Dt;
						if(desadv_bill_rec.LinkBillID && P_BObj->Search(desadv_bill_rec.LinkBillID, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, (temp_buf = 0).Cat(ord_bill_rec.ID));
							pBill->OrderDate = ord_bill_rec.Dt;
						}
					}
				}
			}
			else { // } @v8.6.12
				if(P_BObj->P_Tbl->GetListOfOrdersByLading(pPack->Rec.ID, &ord_bill_list) > 0) {
					for(uint j = 0; j < ord_bill_list.getCount(); j++) {
						PPID  ord_bill_id = ord_bill_list.get(j);
						ltoa(ord_bill_id, pBill->OrderBillID, 10);
						if(P_BObj->Search(ord_bill_id, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, (temp_buf = 0).Cat(ord_bill_rec.ID)); // @v8.6.9
							pBill->OrderDate = ord_bill_rec.Dt; // @v8.6.9
							break; // Даже если с отгрузкой связано более одного заказа, здесь мы можем сохранить ссылку лишь на один
						}
					}
				}
				// @vmiller {
				if(isempty(pBill->OrderBillNo)) {
					if(pPack->Rec.LinkBillID) {
						BillTbl::Rec ord_bill_rec;
						ltoa(pPack->Rec.LinkBillID, pBill->OrderBillID, 10);
						if(P_BObj->Search(pPack->Rec.LinkBillID, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, (temp_buf = 0).Cat(ord_bill_rec.ID)); // @v8.6.9
							pBill->OrderDate = ord_bill_rec.Dt; // @v8.6.9
						}
					}
				}
				// } @vmiller
			}
		}
		if(pPack->P_Freight) {
			const PPID dlvr_addr_id = pPack->P_Freight->DlvrAddrID;
			if(dlvr_addr_id && LocObj.Search(dlvr_addr_id, &loc_rec) > 0) {
				pBill->DlvrAddrID = dlvr_addr_id;
				(temp_buf = loc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
				STRNSCPY(pBill->DlvrAddrCode, temp_buf);
				LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
				if(temp_buf.NotEmptyS())
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->DlvrAddr, sizeof(pBill->DlvrAddr));
				else {
					LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->DlvrAddr, sizeof(pBill->DlvrAddr));
				}
			}
		}
		if(pPack->Ext.AgentID) {
			GetArticleName(pPack->Ext.AgentID, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->AgentName, sizeof(pBill->AgentName));
			GetReg(pPack->Ext.AgentID, PPREGT_TPID, temp_buf);
			temp_buf.CopyTo(pBill->AgentINN, sizeof(pBill->AgentINN));
			GetReg(pPack->Ext.AgentID, PPREGT_GLN, temp_buf);
			temp_buf.CopyTo(pBill->AgentGLN, sizeof(pBill->AgentGLN));
		}
		{
			PPID   main_org_id = 0;
			if(GetMainOrgID(&main_org_id)) {
				RegisterTbl::Rec reg_rec;
				if(PsnObj.GetRegister(main_org_id, PPREGT_GLN, &reg_rec) > 0)
					STRNSCPY(pBill->MainGLN, reg_rec.Num);
				// @vmiller {
				if(PsnObj.GetRegister(main_org_id, PPREGT_TPID, &reg_rec) > 0)
					STRNSCPY(pBill->MainINN, reg_rec.Num);
				// } @vmiller
			}
		}
		// @vmiller {
		// Запишем префикс документа
		if(!isempty(BillParam.ImpExpParamDll.XmlPrefix))
			STRNSCPY(pBill->XmlPrefix, BillParam.ImpExpParamDll.XmlPrefix);
		//
		// Запишем ИД тега типа товара "Пиво"
		//
		if(BillParam.ImpExpParamDll.BeerGrpID)
			pBill->BeerGrpID = BillParam.ImpExpParamDll.BeerGrpID;
		{
			//
			// Запишем GUID документа
			//
			S_GUID guid;
			guid.SetZero();
			if(P_BObj->GetGuid((temp_buf = pBill->ID).ToLong(), &guid) > 0) {
				STRNSCPY(pBill->GUID, guid.ToStr(S_GUID::fmtIDL, temp_buf));
			}
		}
		//
		// Запишем номер и дату регистра контрагента
		{
			const  PPBillConfig & cfg = DS.GetTLA().P_BObj->GetConfig(); //P_BObj->GetConfig();
			if(cfg.ContractRegTypeID) {
				RegisterTbl::Rec reg_rec;
				if(PsnObj.GetRegister(ObjectToPerson(pPack->Rec.Object), cfg.ContractRegTypeID, &reg_rec) > 0) {
					STRNSCPY(pBill->CntractCode, reg_rec.Num);
					pBill->CntractDt = reg_rec.Dt;
					pBill->CntractExpry = reg_rec.Expiry;
				}
			}
		}

		// } @vmiller
		ok = 1;
	}
	return ok;
}

int SLAPI PPBillExporter::GetReg(PPID arID, PPID regTypeID, SString & rRegNum)
{
	int    ok = -1;
	rRegNum = 0;
	if(arID && regTypeID) {
		RegisterTbl::Rec reg_rec;
		if(PsnObj.GetRegister(ObjectToPerson(arID), regTypeID, &reg_rec) > 0) {
			rRegNum = reg_rec.Num;
			ok = 1;
		}
	}
	return ok;
}
//
// @vmiller
// Descr: Получает список документов, которые были успешно отправлены и ставит в тегах этих документов пометку
//
int SLAPI PPBillExporter::CheckBillsWasExported(ImpExpDll * pExpDll)
{
	int    ok = 1, r = 1;
	{
		Sdr_DllImpExpReceipt exp_rcpt;
		MEMSZERO(exp_rcpt);
		PPTransaction tra(1);
		THROW(tra);
		while((r = pExpDll->EnumExpReceipt(&exp_rcpt)) == 1) {
			if(exp_rcpt.ID) {
				PPBillPacket bill_pack;
				uint declined = 0;
				if(P_BObj->ExtractPacket(exp_rcpt.ID, &bill_pack))
					declined = bill_pack.Rec.Flags2 & BILLF2_DECLINED;
				if(BillParam.ImpExpParamDll.RcptTagID && (!isempty(exp_rcpt.ReceiptNumber) || declined)) { // Если отмена заказа, то ReceiptNumber не обязательно должен быть непустым
					ObjTagList tag_list;
					ObjTagItem tag_item;
					P_BObj->GetTagList(exp_rcpt.ID, &tag_list);
					tag_item.Init(BillParam.ImpExpParamDll.RcptTagID);
					tag_item.TagDataType = OTTYP_STRING;
					tag_item.SetStr(tag_item.TagID, declined ? "" : exp_rcpt.ReceiptNumber); // Если отмена заказа, то очищаем тег
					tag_list.PutItem(tag_item.TagID, &tag_item);
					THROW(P_BObj->SetTagList(exp_rcpt.ID, &tag_list, 0));
				}
			}
			//if(!isempty(exp_rcpt.ReceiptNumber) && exp_rcpt.ID && /*P_IEBill->.GetParam()*/BillParam.ImpExpParamDll.RcptTagID) {
			//	// Записываем номер квитанции об отправке в тег документа
			//	ObjTagList tag_list;
			//	ObjTagItem tag_item;
			//	BillObj->GetTagList(exp_rcpt.ID, &tag_list);
			//	tag_item.Init(/*P_IEBill->.GetParam()*/BillParam.ImpExpParamDll.RcptTagID);
			//	tag_item.TagDataType = OTTYP_STRING;
			//	tag_item.SetStr(tag_item.TagID, exp_rcpt.ReceiptNumber);
			//	tag_list.PutItem(tag_item.TagID, &tag_item);
			//	THROW(BillObj->SetTagList(exp_rcpt.ID, &tag_list, 0));
			//}
			MEMSZERO(exp_rcpt);
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
