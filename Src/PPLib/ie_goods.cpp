// IE_GOODS.CPP
// Copyright (c) A.Starodub 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(QUOTVAL, PPQuotImpExpParam);

PPQuotImpExpParam::PPQuotImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags), QuotCls(PPQC_PRICE), QuotKindID(0), CurrID(0), ArID(0), LocID(0)
{
}

PPQuotImpExpParam & PPQuotImpExpParam::Z()
{
	QuotCls = PPQC_PRICE; // @v11.4.2
	QuotKindID = 0;
	CurrID = 0;
	ArID = 0;
	LocID = 0;
	return *this;
}

/*virtual*/int PPQuotImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(QuotKindID)
			param_list.Add(PPQUOTPAR_QUOTKIND, temp_buf.Z().Cat(QuotKindID));
		if(CurrID)
			param_list.Add(PPQUOTPAR_CURRENCY, temp_buf.Z().Cat(CurrID));
		if(ArID)
			param_list.Add(PPQUOTPAR_ARTICLE, temp_buf.Z().Cat(ArID));
		if(LocID)
			param_list.Add(PPQUOTPAR_LOC, temp_buf.Z().Cat(LocID));
		if(Flags)
			param_list.Add(PPQUOTPAR_FLAGS, temp_buf.Z().Cat(Flags));
		if(QuotCls) // @v11.4.2
			param_list.Add(PPQUOTPAR_QUOTCAT, temp_buf.Z().Cat(QuotCls));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Z();
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			const long id_to_assign = temp_buf.ToLong();
			switch(item.Id) {
				case PPQUOTPAR_QUOTKIND: QuotKindID = id_to_assign; break;
				case PPQUOTPAR_CURRENCY: CurrID = id_to_assign; break;
				case PPQUOTPAR_ARTICLE:  ArID = id_to_assign; break;
				case PPQUOTPAR_LOC:      LocID = id_to_assign; break;
				case PPQUOTPAR_FLAGS:    Flags = id_to_assign; break;
				case PPQUOTPAR_QUOTCAT: // @v11.4.2
					if(oneof5(id_to_assign, PPQC_PRICE, PPQC_SUPPLDEAL, PPQC_MATRIX, PPQC_MATRIXRESTR, PPQC_PREDICTCOEFF))
						QuotCls = id_to_assign; 
					else
						QuotCls = PPQC_PRICE;
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPQuotImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name, param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	THROW(PPLoadText(PPTXT_QUOTPARAMS, params));
	const LAssoc int_items[] = {
		{PPQUOTPAR_QUOTKIND, QuotKindID},
		{PPQUOTPAR_CURRENCY, CurrID},
		{PPQUOTPAR_ARTICLE, ArID},
		{PPQUOTPAR_LOC, LocID},
		{PPQUOTPAR_FLAGS, Flags},
		{PPQUOTPAR_QUOTCAT, QuotCls} // @v11.4.2
	};
	for(uint i = 0; i < SIZEOFARRAY(int_items); i++) {
		PPGetSubStr(params, int_items[i].Key, fld_name);
		pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(int_items[i].Val), 1);
	}
	CATCHZOK
	return ok;
}

int PPQuotImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_QUOTPARAMS, params));
	{
		struct I {
			int    ID;
			int32 * P_Val;
		} int_items[] = {
			{PPQUOTPAR_QUOTKIND, &QuotKindID},
			{PPQUOTPAR_CURRENCY, &CurrID},
			{PPQUOTPAR_ARTICLE, &ArID},
			{PPQUOTPAR_LOC, &LocID},
			{PPQUOTPAR_FLAGS, &Flags},
			{PPQUOTPAR_QUOTCAT, &QuotCls} // @v11.4.2
		};
		for(uint i = 0; i < SIZEOFARRAY(int_items); i++) {
			*(int_items[i].P_Val) = 0;
			if(PPGetSubStr(params, int_items[i].ID, fld_name)) {
				excl.add(fld_name);
				pFile->GetParam(pSect, fld_name, param_val);
				*(int_items[i].P_Val) = param_val.ToLong();
			}
		}
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

QuotImpExpDialog::QuotImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPQUOT), QkSpc(QkSpc.ctrInitializeWithCache)
{
}

int QuotImpExpDialog::setDTS(const PPQuotImpExpParam * pData)
{
	RVALUEPTR(Data, pData);
	ImpExpParamDialog::setDTS(&Data);
	{
		// @v11.4.2 {
		long   qk_sel_extra = 1;
		PPID   acs_id = 0;
		PPID   new_qk_id = 0;
		if(Data.QuotKindID)
			QkObj.Classify(Data.QuotKindID, reinterpret_cast<int *>(&Data.QuotCls));
		QkSpc.GetDefaults(Data.QuotCls, Data.QuotKindID, &acs_id, &new_qk_id, &qk_sel_extra);
		AddClusterAssocDef(CTL_IMPEXPQUOT_QKCLS, 0, PPQuot::clsGeneral);
		AddClusterAssoc(CTL_IMPEXPQUOT_QKCLS, 1, PPQuot::clsSupplDeal);
		AddClusterAssoc(CTL_IMPEXPQUOT_QKCLS, 2, PPQuot::clsMtx);
		AddClusterAssoc(CTL_IMPEXPQUOT_QKCLS, 3, PPQuot::clsPredictCoeff);
		SetClusterData(CTL_IMPEXPQUOT_QKCLS, Data.QuotCls);
		// } @v11.4.2 
		SetupPPObjCombo(this, CTLSEL_IMPEXPQUOT_QK, PPOBJ_QUOTKIND, new_qk_id, 0, reinterpret_cast<void *>(qk_sel_extra));
		SetupArCombo(this, CTLSEL_IMPEXPQUOT_AR, Data.ArID, 0, GetSellAccSheet(), sacfDisableIfZeroSheet);
		SetupPPObjCombo(this, CTLSEL_IMPEXPQUOT_CURR,  PPOBJ_CURRENCY, Data.CurrID, 0);
		SetupPPObjCombo(this, CTLSEL_IMPEXPQUOT_LOC,   PPOBJ_LOCATION,   Data.LocID, 0);
		SetupCtrls(Data.Direction);
	}
	return 1;
}

int QuotImpExpDialog::getDTS(PPQuotImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	if(Data.Direction != 0) {
		Data.QuotCls = GetClusterData(CTL_IMPEXPQUOT_QKCLS); // @v11.4.2
		getCtrlData(CTLSEL_IMPEXPQUOT_QK,     &Data.QuotKindID);
		getCtrlData(CTLSEL_IMPEXPQUOT_AR,     &Data.ArID);
		getCtrlData(CTLSEL_IMPEXPQUOT_CURR,   &Data.CurrID);
		getCtrlData(CTLSEL_IMPEXPQUOT_LOC,    &Data.LocID);
	}
	else {
		Data.QuotKindID = 0;
		Data.ArID = 0;
		Data.CurrID = 0;
		Data.LocID = 0;
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

IMPL_HANDLE_EVENT(QuotImpExpDialog)
{
	ImpExpParamDialog::handleEvent(event);
	if(event.isClusterClk(CTL_IMPEXP_DIR)) {
		GetClusterData(CTL_IMPEXP_DIR, &Data.Direction);
		SetupCtrls(Data.Direction);
	}
	else if(event.isClusterClk(CTL_IMPEXPQUOT_QKCLS)) {
		const long   prev_cls = Data.QuotCls;
		Data.QuotCls = GetClusterData(CTL_IMPEXPQUOT_QKCLS);
		if(Data.QuotCls != prev_cls) {
			PPID   acs_id = 0;
			long   qk_sel_extra = 1;
			PPID   new_qk_id = 0;
			QkSpc.GetDefaults(Data.QuotCls, 0, &acs_id, &new_qk_id, &qk_sel_extra);
			Data.QuotKindID = new_qk_id;
			SetupPPObjCombo(this, CTLSEL_IMPEXPQUOT_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0, reinterpret_cast<void *>(qk_sel_extra));
		}		
	}
	else
		return;
	clearEvent(event);
}

void QuotImpExpDialog::SetupCtrls(long direction)
{
	disableCtrls(direction == 0, CTLSEL_IMPEXPQUOT_QK, CTLSEL_IMPEXPQUOT_CURR, CTLSEL_IMPEXPQUOT_AR, CTLSEL_IMPEXPQUOT_LOC, 0L);
}

int EditQuotImpExpParam(const char * pIniSection)
{
	int    ok = -1;
	QuotImpExpDialog * dlg = 0;
	PPQuotImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		//Или здесь??
   		THROW(CheckDialogPtr(&(dlg = new QuotImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_QUOTVAL, &param.InrRec));
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
   				PPSetError(PPERR_DUPOBJNAME);
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
//
// Import
//
class PPQuotImporter {
public:
	PPQuotImporter() : P_IE(0)
	{
	}
	~PPQuotImporter()
	{
		ZDELETE(P_IE);
	}
	int    Run(const char * pCfgName, int use_ta);
private:
	PPID   IdentifyArticle(const Sdr_QuotVal & rRec, ArticleTbl::Rec * pArRec)
	{
		PPID   ar_id = 0;
		ArticleTbl::Rec ar_rec;
		if(rRec.ArID) {
			if(ArObj.Search(rRec.ArID, &ar_rec) > 0) {
				ar_id = ar_rec.ID;
				ASSIGN_PTR(pArRec, ar_rec);
			}
		}
		if(!ar_id && rRec.ArCode[0]) {

		}
		return ar_id;
	}
	PPQuotImpExpParam Param;
	PPImpExp * P_IE;
	PPObjGoods GObj;
	PPObjArticle ArObj;
	PPObjLocation LocObj;
};

static int SelectQuotImportCfgs(PPQuotImpExpParam * pParam, int import)
{
	const uint _rec_ident = PPREC_QUOTVAL;
	int    ok = -1;
	int    valid_data = 0;
	uint   p = 0;
	long   id = 0;
	SString ini_file_name, sect;
	StrAssocArray list;
	PPQuotImpExpParam param;
	TDialog * p_dlg = 0;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, _rec_ident, &param, &list, import ? 2 : 1));
	id = (list.SearchByTextNc(pParam->Name, &p) > 0) ? static_cast<uint>(list.Get(p).Id) : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		#if SLTEST_RUNNING
			//
			// В режиме автоматического тестирования конфигурация выбирается автоматически по имени pParam->Name
			//
			for(int i = 1; ok < 0 && i < static_cast<int>(list.getCount()); i++) {
				list.GetText(i, sect);
				if(strstr(sect, pParam->Name)) {
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					ok = 1;
				}
			}
		#endif
		while(ok < 0 && ListBoxSelDialog::Run(&list, PPTXT_TITLE_QUOTIMPCFG, &id) > 0) {
			if(id) {
				list.GetText(id, sect);
				pParam->ProcessName(1, sect);
				pParam->ReadIni(&ini_file, sect, 0);
				valid_data = ok = 1;
			}
			else
				PPError(PPERR_INVQUOTIMPEXPCFG);
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int PPQuotImporter::Run(const char * pCfgName, int use_ta)
{
	const uint _rec_ident = PPREC_QUOTVAL;
	int    ok = 1, r = 0;
	SString wait_msg, temp_buf, tok_buf;
	ZDELETE(P_IE);
	THROW(LoadSdRecord(_rec_ident, &Param.InrRec));
	if(pCfgName) {
		uint p = 0;
		StrAssocArray list;
		PPSCardImpExpParam param;
		SString ini_file_name;
		Param.Name = pCfgName;
		Param.Direction = 1;
		THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, _rec_ident, &param, &list, 2));
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		for(uint i = 1; i < list.getCount(); i++) {
			list.GetText(i, temp_buf);
			if(temp_buf.CmpPrefix(pCfgName, 1) == 0) {
				Param.ProcessName(1, temp_buf);
				Param.ReadIni(&ini_file, temp_buf, 0);
				r = 1;
				break;
			}
		}
	}
	else if(SelectQuotImportCfgs(&Param, 1) > 0)
		r = 1;
	if(r == 1) {
		THROW_MEM(P_IE = new PPImpExp(&Param, 0));
		{
			PPWaitStart();
			PPLoadText(PPTXT_IMPQUOT, wait_msg);
			PPWaitMsg(wait_msg);
			IterCounter cntr;
			if(P_IE->OpenFileForReading(0)) {
				PPObjQuotKind qk_obj;
				PPQuotKind qk_rec;
				PPObjCurrency cur_obj;
				PPCurrency cur_rec;
				Goods2Tbl::Rec goods_rec;
				ArticleTbl::Rec ar_rec;
				LocationTbl::Rec loc_rec;
				BarcodeTbl::Rec bc_rec;
				long   numrecs = 0;
				P_IE->GetNumRecs(&numrecs);
				cntr.SetTotal(numrecs);
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < static_cast<uint>(numrecs); i++) {
					PPID   qk_id = 0;
					PPID   goods_id = 0;
					PPID   ar_id = 0;
					PPID   cur_id = 0;
					PPID   loc_id = 0;
					double qv = 0.0;
					long   qf = 0;
					PPID   temp_id = 0;
					Sdr_QuotVal sdr_rec;
					THROW(P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec)));
					P_IE->GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &sdr_rec);
					if(sdr_rec.QuotKindID && qk_obj.Search(sdr_rec.QuotKindID, &qk_rec) > 0) {
						qk_id = qk_rec.ID;
					}
					if(!qk_id && sdr_rec.QuotKindSymb[0]) {
						if(qk_obj.SearchBySymb(sdr_rec.QuotKindSymb, &temp_id, &qk_rec) > 0) {
							qk_id = qk_rec.ID;
						}
					}
					if(SETIFZ(qk_id, Param.QuotKindID)) {
						if(sdr_rec.GoodsID) {
							if(GObj.Search(sdr_rec.GoodsID, &goods_rec) > 0) {
								if(goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC)) {
									goods_id = goods_rec.ID;
								}
							}
						}
						if(!goods_id && sdr_rec.GoodsCode[0]) {
							if(GObj.SearchByBarcode(sdr_rec.GoodsCode, &bc_rec, &goods_rec, 1) > 0) {
								if(goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC))
									goods_id = bc_rec.GoodsID;
							}
						}
						// @v11.3.1 {
						if(!goods_id && sdr_rec.GoodsName[0]) { 
							if(GObj.SearchByName(sdr_rec.GoodsName, 0, &goods_rec) > 0) {
								if(goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC))
									goods_id = goods_rec.ID;
							}
						}
						// } @v11.3.1 
						if(goods_id) {
							if(sdr_rec.ArID) {
								if(ArObj.Search(sdr_rec.ArID, &ar_rec) > 0)
									ar_id = ar_rec.ID;
							}
							if(!ar_id && sdr_rec.ArCode[0]) {
								;
							}
							SETIFZ(ar_id, Param.ArID);
							//
							if(sdr_rec.CurrencyID && cur_obj.Fetch(sdr_rec.CurrencyID, &cur_rec) > 0) {
								cur_id = cur_rec.ID;
							}
							if(!cur_id && sdr_rec.CurrencyCode) {
								if(cur_obj.SearchCode(&temp_id, sdr_rec.CurrencyCode) > 0)
									cur_id = temp_id;
							}
							if(!cur_id && sdr_rec.CurrencySymb[0]) {
								if(cur_obj.SearchSymb(&temp_id, sdr_rec.CurrencySymb) > 0)
									cur_id = temp_id;
							}
							SETIFZ(cur_id, Param.CurrID);
							//
							if(sdr_rec.LocID && LocObj.Search(sdr_rec.LocID, &loc_rec) > 0) {
								loc_id = loc_rec.ID;
							}
							if(!loc_id && sdr_rec.LocCode[0]) {
								if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, sdr_rec.LocCode, &temp_id, &loc_rec) > 0) {
									loc_id = loc_rec.ID;
								}
								else if(LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, sdr_rec.LocCode, &temp_id, &loc_rec) > 0) {
									loc_id = loc_rec.ID;
								}
							}
							SETIFZ(loc_id, Param.LocID);
							{
								PPQuot quot(goods_id);
								quot.Kind = qk_id;
								quot.LocID = loc_id;
								quot.ArID = ar_id;
								if(checkdate(sdr_rec.DateLow, 1) && checkdate(sdr_rec.DateUpp, 1) && (!sdr_rec.DateUpp || sdr_rec.DateLow <= sdr_rec.DateUpp)) {
									quot.Period.Set(sdr_rec.DateLow, sdr_rec.DateUpp);
								}
								temp_buf = sdr_rec.ValueText;
								if(temp_buf.NotEmptyS()) {
									quot.GetValFromStr(temp_buf);
								}
								if(quot.IsEmpty()) {
									quot.Quot = sdr_rec.Value;
									quot.Flags = sdr_rec.ValueFlags;
								}
                                THROW(GObj.P_Tbl->SetQuot(quot, 0));
							}
						}
					}
					PPWaitPercent(cntr.Increment(), wait_msg);
				}
				THROW(tra.Commit());
			}
			PPWaitStop();
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoods::ImportQuot(const char * pCfgName, int use_ta)
{
	PPQuotImporter qi;
	return qi.Run(pCfgName, use_ta);
}
//
//
//
/*static*/const uint32 ExportGoodsParam::Signature = 0x49FA91B4UL;

ExportGoodsParam::ExportGoodsParam() : LocID(0)
{
}
	
ExportGoodsParam & ExportGoodsParam::Z()
{
	LocID = 0;
	Filt.Init(1, 0);
	ExpCfg.Z();
	return *this;
}
	
int ExportGoodsParam::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	if(rBuf.GetAvailableSize()) {
		uint32 _signature = 0;
		THROW_SL(rBuf.Read(_signature));
		if(_signature == ExportGoodsParam::Signature) {
			THROW_SL(rBuf.Read(LocID));				
		}
		else {
			rBuf.Unread(sizeof(_signature));
		}
		THROW(Filt.Read(rBuf, 0));
		THROW(rBuf.Read(ExpCfg));
	}
	CATCHZOK
	return ok;
}

int ExportGoodsParam::Write(SBuffer & rBuf, long)
{
	int    ok = 1;
	uint32 _signature = ExportGoodsParam::Signature; 
	THROW_SL(rBuf.Write(_signature));
	THROW_SL(rBuf.Write(LocID));
	THROW(Filt.Write(rBuf, 0));
	THROW(rBuf.Write(ExpCfg));
	CATCHZOK
	return ok;
}

/*static*/int ExportGoodsParam::GetExportParamByName(const char * pParamName, PPGoodsImpExpParam * pParam)
{
	int    ok = 0;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	if(pParamName && pParam) {
		SString sect;
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_GOODS2, &pParam->InrRec));
		pParam->Direction = 0;
		pParam->ProcessName(1, (sect = pParamName));
		THROW_PP_S(pParam->ReadIni(&ini_file, sect, 0) > 0, PPERR_INVGOODSEXPCFG, pParamName);
		pParam->ProcessName(2, (sect = pParamName));
		pParam->Name = sect;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*static*/int ExportGoodsParam::Edit(ExportGoodsParam & rData)
{
	class ExportGoodsFiltDialog : public TDialog {
		DECL_DIALOG_DATA(ExportGoodsParam);
	public:
		ExportGoodsFiltDialog() : TDialog(DLG_GOODSEXPFILT)
		{
			PPGoodsImpExpParam param;
			GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_GOODS2, &param, &CfgList, 1);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			{
				uint    p  = 0;
				PPGoodsImpExpParam param;
				ExportGoodsParam::GetExportParamByName(Data.ExpCfg, &param);
				const uint id = (CfgList.SearchByTextNc(param.Name, &p) > 0) ? static_cast<uint>(CfgList.Get(p).Id) : 0;
				SetupStrAssocCombo(this, CTLSEL_GOODSEXPFILT_CFG, CfgList, static_cast<long>(id), 0);
				SetupLocationCombo(this, CTLSEL_GOODSEXPFILT_LOC, Data.LocID, 0, 0);
			}
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			long   id = 0;
			SString sect;
			getCtrlData(sel = CTLSEL_GOODSEXPFILT_CFG, &id);
			THROW_PP(id, PPERR_INVGOODSIMPEXPCFG);
			CfgList.GetText(id, sect);
			Data.ExpCfg = sect;
			getCtrlData(CTLSEL_GOODSEXPFILT_LOC, &Data.LocID);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmGoodsFilt)) {
				PPViewGoods v_goods;
				v_goods.EditBaseFilt(&Data.Filt);
				clearEvent(event);
			}
		}
		StrAssocArray CfgList;
	};
	DIALOG_PROC_BODYERR(ExportGoodsFiltDialog, &rData);
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(GOODS2, PPGoodsImpExpParam);

PPGoodsImpExpParam::PPGoodsImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags),
	AccSheetID(0), SupplID(0), DefUnitID(0), PhUnitID(0), DefParentID(0), RcptOpID(0),
	Flags(0), LocID(0), MatrixAction(0)
{
}

void PPGoodsImpExpParam::Clear()
{
	AccSheetID    = 0;
	SupplID       = 0;
	DefUnitID     = 0;
	PhUnitID      = 0;
	DefParentID   = 0;
	RcptOpID      = 0;
	Flags = 0;
	LocID = 0;
	MatrixAction  = 0;
	SubCode.Z();
}

/*virtual*/int PPGoodsImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(SubCode.NotEmptyS())
			param_list.Add(PPGOODSPAR_SUBCODE, temp_buf = SubCode);
		const LAssoc int_fld_list[] = {
			{ AccSheetID, PPGOODSPAR_ACCSHEET },
			{ SupplID, PPGOODSPAR_SUPPL },
			{ DefUnitID, PPGOODSPAR_DEFUNIT },
			{ PhUnitID, PPGOODSPAR_PHUNIT },
			{ DefParentID, PPGOODSPAR_DEFPARENT },
			{ RcptOpID, PPGOODSPAR_RCPTOP },
			{ Flags, PPGOODSPAR_FLAGS },
			{ LocID, PPGOODSPAR_LOC },
			{ MatrixAction, PPGOODSPAR_MATRIXACTION },
		};
		for(uint i = 0; i < SIZEOFARRAY(int_fld_list); i++) {
			if(int_fld_list[i].Key)
				param_list.Add(int_fld_list[i].Val, temp_buf.Z().Cat(int_fld_list[i].Key));
		}
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Clear();
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case PPGOODSPAR_SUBCODE: SubCode = temp_buf; break;
				case PPGOODSPAR_ACCSHEET: AccSheetID = temp_buf.ToLong(); break;
				case PPGOODSPAR_SUPPL: SupplID = temp_buf.ToLong(); break;
				case PPGOODSPAR_DEFUNIT: DefUnitID = temp_buf.ToLong(); break;
				case PPGOODSPAR_PHUNIT: PhUnitID = temp_buf.ToLong(); break;
				case PPGOODSPAR_DEFPARENT: DefParentID = temp_buf.ToLong(); break;
				case PPGOODSPAR_RCPTOP: RcptOpID = temp_buf.ToLong(); break;
				case PPGOODSPAR_FLAGS: Flags = temp_buf.ToLong(); break;
				case PPGOODSPAR_LOC: LocID = temp_buf.ToLong(); break;
				case PPGOODSPAR_MATRIXACTION: MatrixAction = temp_buf.ToLong(); break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPGoodsImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name, param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	THROW(PPLoadText(PPTXT_GOODSPARAMS, params));
	if(SubCode.NotEmpty()) {
		PPGetSubStr(params, PPGOODSPAR_SUBCODE, fld_name);
		pFile->AppendParam(pSect, fld_name, SubCode, 1);
	}
	struct I {
		int    ID;
		int32  Val;
	} int_items[] = {
		{PPGOODSPAR_SUPPL,        SupplID},
		{PPGOODSPAR_DEFUNIT,      DefUnitID},
		{PPGOODSPAR_PHUNIT,       PhUnitID},
		{PPGOODSPAR_DEFPARENT,    DefParentID},
		{PPGOODSPAR_RCPTOP,       RcptOpID},
		{PPGOODSPAR_FLAGS,        Flags},
		{PPGOODSPAR_LOC,          LocID},
		{PPGOODSPAR_MATRIXACTION, MatrixAction},
		{PPGOODSPAR_ACCSHEET,     AccSheetID},
	};
	for(uint i = 0; i < SIZEOFARRAY(int_items); i++) {
		PPGetSubStr(params, int_items[i].ID, fld_name);
		pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(int_items[i].Val), 1);
	}
	CATCHZOK
	return ok;
}

int PPGoodsImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_GOODSPARAMS, params));
	if(PPGetSubStr(params, PPGOODSPAR_SUBCODE, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			SubCode = param_val;
	}
	{
		struct I {
			int    ID;
			int32 * P_Val;
		} int_items[] = {
			{PPGOODSPAR_SUPPL,        &SupplID},
			{PPGOODSPAR_DEFUNIT,      &DefUnitID},
			{PPGOODSPAR_PHUNIT,       &PhUnitID},
			{PPGOODSPAR_DEFPARENT,    &DefParentID},
			{PPGOODSPAR_RCPTOP,       &RcptOpID},
			{PPGOODSPAR_FLAGS,        &Flags},
			{PPGOODSPAR_LOC,          &LocID},
			{PPGOODSPAR_MATRIXACTION, &MatrixAction},
			{PPGOODSPAR_ACCSHEET,     &AccSheetID},
		};
		for(uint i = 0; i < SIZEOFARRAY(int_items); i++) {
			*(int_items[i].P_Val) = 0;
			if(PPGetSubStr(params, int_items[i].ID, fld_name)) {
				excl.add(fld_name);
				pFile->GetParam(pSect, fld_name, param_val);
				*(int_items[i].P_Val) = param_val.ToLong();
			}
		}
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

GoodsImpExpDialog::GoodsImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPGOODS)
{
}

int GoodsImpExpDialog::setDTS(const PPGoodsImpExpParam * pData)
{
	RVALUEPTR(Data, pData);
	ImpExpParamDialog::setDTS(&Data);
	{
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		const  PPID acs_id = (acs_obj.Fetch(Data.AccSheetID, &acs_rec) > 0) ? Data.AccSheetID : GetSupplAccSheet();
		setCtrlString(CTL_IMPEXPGOODS_SUBCODE, Data.SubCode);
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_ACS, PPOBJ_ACCSHEET, acs_id, 0, 0);
		SetupArCombo(this, CTLSEL_IMPEXPGOODS_SUPPL, Data.SupplID, 0, acs_id, sacfDisableIfZeroSheet);
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_UNIT,  PPOBJ_UNIT,       Data.DefUnitID,   0);
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_PHUNI, PPOBJ_UNIT,       Data.PhUnitID,    0);
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_GRP,   PPOBJ_GOODSGROUP, Data.DefParentID, OLW_CANINSERT);
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_OP,    PPOBJ_OPRKIND,    Data.RcptOpID,    0, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
		SetupPPObjCombo(this, CTLSEL_IMPEXPGOODS_LOC,   PPOBJ_LOCATION,   Data.LocID,       0);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 0, PPGoodsImpExpParam::fSkipZeroQtty);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 1, PPGoodsImpExpParam::fAnalyzeBarcode);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 2, PPGoodsImpExpParam::fAnalyzeName);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 3, PPGoodsImpExpParam::fAnalyzeOnly);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 4, PPGoodsImpExpParam::fUHTT);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 5, PPGoodsImpExpParam::fForceSnglBarcode);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 6, PPGoodsImpExpParam::fImportImages);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 7, PPGoodsImpExpParam::fForceUpdateManuf);
		AddClusterAssoc(CTL_IMPEXPGOODS_FLAGS, 8, PPGoodsImpExpParam::fForceUpdateBrand); // @v11.6.10
		SetClusterData(CTL_IMPEXPGOODS_FLAGS, Data.Flags);
		setCtrlLong(CTL_IMPEXPGOODS_MXACT, Data.MatrixAction);
		SetupCtrls(Data.Direction);
	}
	return 1;
}

int GoodsImpExpDialog::getDTS(PPGoodsImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	// Может здесь??
	THROW(ImpExpParamDialog::getDTS(&Data));
	if(Data.Direction != 0) {
		getCtrlString(CTL_IMPEXPGOODS_SUBCODE,  Data. SubCode);
		getCtrlData(CTLSEL_IMPEXPGOODS_ACS,    &Data.AccSheetID);
		getCtrlData(CTLSEL_IMPEXPGOODS_SUPPL,  &Data.SupplID);
		getCtrlData(CTLSEL_IMPEXPGOODS_UNIT,   &Data.DefUnitID);
		getCtrlData(CTLSEL_IMPEXPGOODS_PHUNI,  &Data.PhUnitID);
		getCtrlData(CTLSEL_IMPEXPGOODS_GRP,    &Data.DefParentID);
		getCtrlData(CTLSEL_IMPEXPGOODS_OP,     &Data.RcptOpID);
		getCtrlData(CTLSEL_IMPEXPGOODS_LOC,    &Data.LocID);
		Data.MatrixAction = getCtrlLong(CTL_IMPEXPGOODS_MXACT);
	}
	else {
		Data.SubCode.Z();
		Data.AccSheetID   = 0;
		Data.SupplID      = 0;
		Data.DefUnitID    = 0;
		Data.PhUnitID     = 0;
		Data.DefParentID  = 0;
		Data.RcptOpID     = 0;
		Data.LocID        = 0;
		Data.Flags        = 0;
		Data.MatrixAction = 0;
	}
	GetClusterData(CTL_IMPEXPGOODS_FLAGS,  &Data.Flags);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

IMPL_HANDLE_EVENT(GoodsImpExpDialog)
{
	ImpExpParamDialog::handleEvent(event);
	if(event.isClusterClk(CTL_IMPEXP_DIR)) {
		long direction = 0;
		GetClusterData(CTL_IMPEXP_DIR, &direction);
		SetupCtrls(direction);
	}
	else if(event.isCbSelected(CTLSEL_IMPEXPGOODS_ACS)) {
		PPID   prev_acs_id = Data.AccSheetID;
		getCtrlData(CTLSEL_IMPEXPGOODS_ACS, &Data.AccSheetID);
		if(prev_acs_id != Data.AccSheetID)
			SetupArCombo(this, CTLSEL_IMPEXPGOODS_SUPPL, Data.SupplID = 0, 0, Data.AccSheetID, sacfDisableIfZeroSheet);
	}
	else
		return;
	clearEvent(event);
}

void GoodsImpExpDialog::SetupCtrls(long direction)
{
	disableCtrls(direction == 0, CTL_IMPEXPGOODS_SUBCODE, CTLSEL_IMPEXPGOODS_SUPPL, CTLSEL_IMPEXPGOODS_UNIT, CTLSEL_IMPEXPGOODS_PHUNI,
		CTLSEL_IMPEXPGOODS_GRP, CTLSEL_IMPEXPGOODS_OP, CTL_IMPEXPGOODS_MXACT, 0L);
	// @v11.6.2 {
	const long idx_list[] = {0L, 1L, 2L, 3L, 4L, 5L};
	DisableClusterItems(CTL_IMPEXPGOODS_FLAGS, LongArray(idx_list, SIZEOFARRAY(idx_list)), direction == 0);
	// } @v11.6.2 
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 0, direction == 0);
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 1, direction == 0);
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 2, direction == 0);
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 3, direction == 0);
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 4, direction == 0);
	// @v11.6.2 DisableClusterItem(CTL_IMPEXPGOODS_FLAGS, 5, direction == 0);
}

int EditGoodsImpExpParams(const char * pIniSection)
{
	int    ok = -1;
	GoodsImpExpDialog * dlg = 0;
	PPGoodsImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		//Или здесь??
   		THROW(CheckDialogPtr(&(dlg = new GoodsImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_GOODS2, &param.InrRec));
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
   				PPSetError(PPERR_DUPOBJNAME);
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

int EditGoodsImpExpParams()
{
	class GoodsImpExpCfgListDialog : public ImpExpCfgListDialog {
	public:
		GoodsImpExpCfgListDialog() : ImpExpCfgListDialog()
		{
			SetParams(PPFILNAM_IMPEXP_INI, PPREC_GOODS2, &Param, 0);
		}
	private:
		virtual int EditParam(const char * pIniSection)
		{
			return EditGoodsImpExpParams(pIniSection);
		}
		PPGoodsImpExpParam Param;
	};
	int    ok = 1;
	GoodsImpExpCfgListDialog * dlg = new GoodsImpExpCfgListDialog;
	if(CheckDialogPtrErr(&dlg))
		ExecViewAndDestroy(dlg);
	else
		ok = PPErrorZ();
	return ok;
}

int SelectGoodsImportCfgs(PPGoodsImpExpParam * pParam, int import)
{
	int    ok = -1;
	int    valid_data = 0;
	TDialog * dlg = 0;
	uint   p = 0;
	long   id = 0;
	PPID   loc_id = 0;
	SString ini_file_name;
	StrAssocArray list;
	PPGoodsImpExpParam param;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_GOODS2, &param, &list, import ? 2 : 1));
	id = (list.SearchByTextNc(pParam->Name, &p) > 0) ? static_cast<uint>(list.Get(p).Id) : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		SString sect;
		// В режиме тестирования - начало {
		#if SLTEST_RUNNING
			for(int i = 1; i < (int)list.getCount(); i++) {
				list.GetText(i, sect);
				if(strstr(sect, pParam->Name)) {
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					ok = 1;
					return ok;
				}
			}
		#endif
		// } конец
		THROW(CheckDialogPtrErr(&(dlg = new TDialog(DLG_IEGOODS))));
		SetupStrAssocCombo(dlg, CTLSEL_IEGOODS_CFG, list, id, 0, 0, 0);
		SetupPPObjCombo(dlg, CTLSEL_IEGOODS_LOC, PPOBJ_LOCATION, loc_id, 0, 0);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			id = dlg->getCtrlLong(CTLSEL_IEGOODS_CFG);
			if(id) {
				list.GetText(id, sect);
				pParam->ProcessName(1, sect);
				pParam->ReadIni(&ini_file, sect, 0);
				loc_id = dlg->getCtrlLong(CTLSEL_IEGOODS_LOC);
				if(loc_id) {
					pParam->LocID = loc_id;
				}
				ok = 1;
			}
			else
				PPError(PPERR_INVGOODSIMPEXPCFG);
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}
//
// Export
//
PPGoodsExporter::PPGoodsExporter() : P_IEGoods(0)
{
	P_GObj = new PPObjGoods;
	P_PsnObj = new PPObjPerson;
	P_QcObj = new PPObjQCert;
	PPGoodsConfig goods_cfg;
	PPObjGoods::ReadConfig(&goods_cfg);
	memzero(WeightPrefix, sizeof(WeightPrefix));
	STRNSCPY(WeightPrefix, goods_cfg.WghtPrefix);
}

PPGoodsExporter::~PPGoodsExporter()
{
	ZDELETE(P_IEGoods);
	delete P_QcObj;
	delete P_PsnObj;
	delete P_GObj;
}

int PPGoodsExporter::Init(const PPGoodsImpExpParam * pParam, StringSet * pResultFileList)
{
	int    ok = 1;
	if(!RVALUEPTR(Param, pParam)) {
		THROW(LoadSdRecord(PPREC_GOODS2, &Param.InrRec));
		ok = SelectGoodsImportCfgs(&Param, 0);
	}
	if(ok > 0) {
		THROW_MEM(P_IEGoods = new PPImpExp(&Param, 0));
		THROW(P_IEGoods->OpenFileForWriting(0, 1, pResultFileList));
	}
	CATCHZOK
	return ok;
}

static void FASTCALL PreprocessGoodsExtText(SString & rBuf, char * pDestBuf, size_t destBufLen)
{
	rBuf.ReplaceStr("\x0D\x0A", "\x20", 0).ReplaceChar('\x0D', '\x20').
		ReplaceChar('\x0A', '\x20').ReplaceChar('\x09', '\x20').ReplaceChar('\x07', '\x20');
    strnzcpy(pDestBuf, rBuf, destBufLen);
}

int PPGoodsExporter::ExportPacket(PPGoodsPacket * pPack, const char * pBarcode, PPID altGrpID, const GoodsFilt * pFilt)
{
	int    ok = 1;
	SString temp_buf;
	THROW_MEM(P_GObj && P_PsnObj && P_QcObj);
	THROW_INVARG(pPack && P_IEGoods);
	if(Param.Flags & PPGoodsImpExpParam::fImportImages) {
		const uint count = pPack->Codes.getCount();
		if(count) {
			pPack->LinkFiles.Init(PPOBJ_GOODS);
			pPack->LinkFiles.Load(pPack->Rec.ID, 0L);
			if(pPack->LinkFiles.GetCount()) {
				SString img_path;
				SString ext_buf;
				pPack->LinkFiles.At(0, img_path);
				if(fileExists(img_path)) {
					SFileFormat ff;
					const int fir = ff.Identify(img_path, &ext_buf);
					if(oneof2(fir, 2, 3)) { // Принимаем только идентификацию по сигнатуре
						if(SImageBuffer::IsSupportedFormat(ff)) { // @v11.3.7 SFileFormat::Webp
							if(!ext_buf.NotEmptyS()) {
								SFileFormat::GetExt(ff, ext_buf);
							}
							SString dest_fname;
							SFsPath src_ps(img_path);
							SFsPath dest_ps(Param.FileName);
							for(uint i = 0; i < count; i++) {
								dest_ps.Nam = pPack->Codes.at(i).Code;
                                dest_ps.Ext = ext_buf;
                                dest_ps.Merge(dest_fname);
								SCopyFile(img_path, dest_fname, 0, FILE_SHARE_READ, 0);
							}
						}
					}
				}
			}
		}
	}
	else {
		Sdr_Goods2 sdr_goods;
		PreprocessGoodsExtText(temp_buf = pPack->Rec.Name, sdr_goods.Name, sizeof(sdr_goods.Name));
		PreprocessGoodsExtText(temp_buf = pPack->Rec.Abbr, sdr_goods.Abbr, sizeof(sdr_goods.Abbr));
		//
		// Класс товара и классификаторы
		//
		if(pPack->Rec.GdsClsID) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(pPack->Rec.GdsClsID, &gc_pack) > 0) {
				PPID   prop_val_id;
				STRNSCPY(sdr_goods.ClsCode, gc_pack.Rec.Symb);
				gc_pack.GetExtDim(&pPack->ExtRec, PPGdsCls::eX, &sdr_goods.DimX);
				gc_pack.GetExtDim(&pPack->ExtRec, PPGdsCls::eY, &sdr_goods.DimY);
				gc_pack.GetExtDim(&pPack->ExtRec, PPGdsCls::eZ, &sdr_goods.DimZ);
				gc_pack.GetExtDim(&pPack->ExtRec, PPGdsCls::eW, &sdr_goods.DimW);
				gc_pack.GetExtProp(&pPack->ExtRec, PPGdsCls::eKind, &prop_val_id, temp_buf);
				temp_buf.CopyTo(sdr_goods.PropKindName, sizeof(sdr_goods.PropKindName));
				gc_pack.GetExtProp(&pPack->ExtRec, PPGdsCls::eGrade, &prop_val_id, temp_buf);
				temp_buf.CopyTo(sdr_goods.PropGradeName, sizeof(sdr_goods.PropGradeName));
				gc_pack.GetExtProp(&pPack->ExtRec, PPGdsCls::eAdd, &prop_val_id, temp_buf);
				temp_buf.CopyTo(sdr_goods.PropAddName, sizeof(sdr_goods.PropAddName));
				gc_pack.GetExtProp(&pPack->ExtRec, PPGdsCls::eAdd2, &prop_val_id, temp_buf);
				temp_buf.CopyTo(sdr_goods.PropAdd2Name, sizeof(sdr_goods.PropAdd2Name));
			}
		}
		//
		// Экспорт цены поступления, реализации, остатка, грузовой таможенной декларации и серийного номера, сертификата качества
		//
		{
			PPObjBill * p_bobj = BillObj;
			ReceiptTbl::Rec rcpt_rec;
			PPIDArray loc_list;
			PPIDArray * p_loc_list = 0;
			if(Param.LocID) {
				loc_list.addnz(Param.LocID);
				p_loc_list = &loc_list;
			}
			else if(pFilt && pFilt->LocList.GetCount()) {
				pFilt->LocList.Get(loc_list);
				p_loc_list = &loc_list;
			}
			else
				p_loc_list = 0;
			{
				GoodsRestParam rp;
				rp.GoodsID = pPack->Rec.ID;
				if(p_loc_list)
					rp.LocList = *p_loc_list;
				p_bobj->trfr->GetCurRest(rp);
				sdr_goods.Rest = rp.Total.Rest;
			}
			double price_by_open_lot = 0.0;
			p_bobj->trfr->Rcpt.ReceiptCore::GetCurrentGoodsPrice_ByLocList(pPack->Rec.ID, p_loc_list, 0/*flags*/, &price_by_open_lot, 0); // @v12.3.10 
			// @v12.3.10 ::GetCurGoodsPrice(pPack->Rec.ID, Param.LocID, 0/*flags*/, &price_by_open_lot, 0);
			if(p_bobj->trfr->Rcpt.GetLastLot_ByLocList(pPack->Rec.ID, p_loc_list, MAXDATE, &rcpt_rec) > 0) {
				QualityCertTbl::Rec qcert_rec;
				sdr_goods.Price = rcpt_rec.Price;
				sdr_goods.Cost  = rcpt_rec.Cost;
				p_bobj->GetClbNumberByLot(rcpt_rec.ID, 0, temp_buf);
				temp_buf.CopyTo(sdr_goods.Clb, sizeof(sdr_goods.Clb));
				p_bobj->GetSerialNumberByLot(rcpt_rec.ID, temp_buf, 1);
				temp_buf.CopyTo(sdr_goods.Serial, sizeof(sdr_goods.Serial));
				sdr_goods.UnitsPerPack = rcpt_rec.UnitPerPack;
				sdr_goods.Expiry = rcpt_rec.Expiry;
				if(rcpt_rec.QCertID && P_QcObj->Search(rcpt_rec.QCertID, &qcert_rec) > 0) {
					STRNSCPY(sdr_goods.QCNumber, qcert_rec.Code);
					STRNSCPY(sdr_goods.QCBlank,  qcert_rec.BlankCode);
					sdr_goods.QCDate = qcert_rec.InitDate;
					sdr_goods.QCExpiry = qcert_rec.Expiry;
					STRNSCPY(sdr_goods.QCManuf, qcert_rec.Manuf);
					GetPersonName(qcert_rec.RegOrgan, temp_buf);
					temp_buf.CopyTo(sdr_goods.QCOrg, sizeof(sdr_goods.QCOrg));
					STRNSCPY(sdr_goods.QCManufDate, qcert_rec.SPrDate);
					STRNSCPY(sdr_goods.QCEtc, qcert_rec.Etc);
					STRNSCPY(sdr_goods.QCInnerCode, qcert_rec.InnerCode);
				}
			}
			else {
				RetailExtrItem extr_item;
				RetailPriceExtractor rpe(0, 0, 0, ZERODATETIME, RTLPF_GETCURPRICE);
				rpe.GetPrice(pPack->Rec.ID, 0, 0.0, &extr_item);
				sdr_goods.Price = extr_item.Price;
				sdr_goods.Cost  = extr_item.Cost;
			}
			sdr_goods.PriceByOpenLot = (price_by_open_lot > 0.0) ? price_by_open_lot : sdr_goods.Price;
		}
		GetObjectName(PPOBJ_BRAND, pPack->Rec.BrandID, temp_buf);
		temp_buf.CopyTo(sdr_goods.Brand, sizeof(sdr_goods.Brand));
		//
		// Экспорт информации о производителе и стране производителе
		//
		{
			PersonReq psn_req;
			PPCountryBlock country_blk;
			sdr_goods.ManufID = pPack->Rec.ManufID;
			GetObjectName(PPOBJ_PERSON, pPack->Rec.ManufID, temp_buf);
			temp_buf.CopyTo(sdr_goods.ManufName, sizeof(sdr_goods.ManufName));
			P_PsnObj->GetCountry(pPack->Rec.ManufID, 0, &country_blk);
			country_blk.Name.CopyTo(sdr_goods.Country, sizeof(sdr_goods.Country));
		}
		//
		// Экспорт информации о родительских группах
		//
		{
			PPID parent_id = pPack->Rec.ParentID;
			if(parent_id) {
				PPGoodsPacket grp_pack;
				if(P_GObj->GetPacket(parent_id, &grp_pack, PPObjGoods::gpoSkipQuot) > 0) {
					sdr_goods.GrpID = grp_pack.Rec.ID;
					STRNSCPY(sdr_goods.GrpName, grp_pack.Rec.Name);
					grp_pack.GetGroupCode(temp_buf);
					STRNSCPY(sdr_goods.GrpCode, temp_buf);
					while(parent_id) {
						PPID   local_parent_id = 0;
						if(P_GObj->GetParentID(parent_id, &local_parent_id) > 0 && local_parent_id)
							parent_id = local_parent_id;
						else
							break;
					}
					if(parent_id && parent_id != pPack->Rec.ParentID) {
						GetObjectName(PPOBJ_GOODSGROUP, parent_id, temp_buf);
						STRNSCPY(sdr_goods.FolderGrpName, temp_buf);
					}
				}
			}
		}
		//
		// Экспорт информации о торговых и физических единицах, их соотношении и емкости упаковки
		//
		{
			PPUnit u_rec;
			if(P_GObj->FetchUnit(pPack->Rec.UnitID, &u_rec) > 0) {
				(temp_buf = u_rec.Name).CopyTo(sdr_goods.UnitName, sizeof(sdr_goods.UnitName));
				(temp_buf = u_rec.Code).CopyTo(sdr_goods.UnitCode, sizeof(sdr_goods.UnitCode));
			}
			if(P_GObj->FetchUnit(pPack->Rec.PhUnitID, &u_rec) > 0)
				(temp_buf = u_rec.Name).CopyTo(sdr_goods.PhUnitName, sizeof(sdr_goods.PhUnitName));
			temp_buf.Z().Cat(pPack->Rec.PhUPerU).CopyTo(sdr_goods.PhUPerU, sizeof(sdr_goods.PhUPerU));
		}
		//
		// Экспорт информации о штрихкодах
		//
		{
			const uint count = pPack->Codes.getCount();
			if(!isempty(pBarcode))
				STRNSCPY(sdr_goods.Code, pBarcode);
			else {
				pPack->Codes.GetSingle(0, temp_buf);
				temp_buf.CopyTo(sdr_goods.Code, sizeof(sdr_goods.Code));
			}
			{
				//
				// Штрихкод с форсированной контрольной цифрой
				//
				STRNSCPY(sdr_goods.CodeCD, sdr_goods.Code);
				const size_t len = sstrlen(sdr_goods.CodeCD);
				const PPGoodsConfig & r_gcfg = P_GObj->GetConfig();
				if(len > 3 && !(r_gcfg.Flags & GCF_BCCHKDIG) && !r_gcfg.IsWghtPrefix(sdr_goods.CodeCD)) {
					int diag = 0;
					int std = 0;
					int dr = P_GObj->DiagBarcode(sdr_goods.Code, &diag, &std, &temp_buf);
					temp_buf.CopyTo(sdr_goods.CodeCD, sizeof(sdr_goods.CodeCD));
				}
			}
			for(uint i = 0; i < count; i++) {
				const BarcodeTbl::Rec & r_bc_rec = pPack->Codes.at(i);
				if(sdr_goods.AddedCode[0] == 0 && stricmp866(sdr_goods.Code, r_bc_rec.Code) != 0 && r_bc_rec.Qtty > 0.0) {
					STRNSCPY(sdr_goods.AddedCode, r_bc_rec.Code);
					sdr_goods.AddedCodeQtty = r_bc_rec.Qtty;
				}
				if(sdr_goods.PreferredCode[0] == 0 && IsInnerBarcodeType(r_bc_rec.BarcodeType, BARCODE_TYPE_PREFERRED)) {
					STRNSCPY(sdr_goods.PreferredCode, r_bc_rec.Code);
				}
			}
		}
		{
			//
			// Экспорт артикула
			//
			P_GObj->P_Tbl->GetArCode(Param.SupplID, pPack->Rec.ID, temp_buf.Z(), 0);
			STRNSCPY(sdr_goods.ArCode, temp_buf);
		}
		memzero(sdr_goods.WeightPrefix, sizeof(sdr_goods.WeightPrefix));
		STRNSCPY(sdr_goods.WeightPrefix, WeightPrefix);
		//
		// Экспорт информации о налоговых ставках
		//
		{
			PPGoodsTaxEntry gtx;
			if(P_GObj->FetchTaxEntry2(pPack->Rec.ID, 0/*lotID*/, 0/*taxPayerID*/, ZERODATE, 0, &gtx) > 0) {
				sdr_goods.SalesTax = fdiv100i(gtx.SalesTax);
				sdr_goods.Vat      = fdiv100i(gtx.VAT);
			}
		}
		sdr_goods.PckgQtty   = pPack->Stock.Package;
		sdr_goods.PckgBrutto = pPack->Stock.Brutto;
		sdr_goods.PckgLength = pPack->Stock.PckgDim.Length;
		sdr_goods.PckgWidth  = pPack->Stock.PckgDim.Width;
		sdr_goods.PckgHeight = pPack->Stock.PckgDim.Height;
		sdr_goods.MinShippmQtty = pPack->Stock.MinShippmQtty;
		sdr_goods.ExpiryPeriod = static_cast<long>(pPack->Stock.ExpiryPeriod); // Срок годности в днях
		//
		// Экспорт информации о дополнительных полях
		//
		pPack->GetExtStrData(GDSEXSTR_A, temp_buf); PreprocessGoodsExtText(temp_buf, sdr_goods.ExtA, sizeof(sdr_goods.ExtA));
		pPack->GetExtStrData(GDSEXSTR_B, temp_buf); PreprocessGoodsExtText(temp_buf, sdr_goods.ExtB, sizeof(sdr_goods.ExtB));
		pPack->GetExtStrData(GDSEXSTR_C, temp_buf); PreprocessGoodsExtText(temp_buf, sdr_goods.ExtC, sizeof(sdr_goods.ExtC));
		pPack->GetExtStrData(GDSEXSTR_D, temp_buf); PreprocessGoodsExtText(temp_buf, sdr_goods.ExtD, sizeof(sdr_goods.ExtD));
		pPack->GetExtStrData(GDSEXSTR_E, temp_buf); PreprocessGoodsExtText(temp_buf, sdr_goods.ExtE, sizeof(sdr_goods.ExtE));
		//
		// Информация о альтернативной товарной группе
		//
		if(altGrpID) {
			long plu = 0L;
			Reference * p_ref = PPRef;
			if(P_GObj->IsAltGroup(altGrpID) > 0 && p_ref->Assc.Search(PPASS_ALTGOODSGRP, altGrpID, pPack->Rec.ID) > 0)
				plu = p_ref->Assc.data.InnerNum;
			sdr_goods.AltGrpPLU = plu;
			GetObjectName(PPOBJ_GOODSGROUP, altGrpID, temp_buf);
			temp_buf.CopyTo(sdr_goods.GrpName, sizeof(sdr_goods.GrpName));
			(temp_buf.Z().Cat(altGrpID)).CopyTo(sdr_goods.GrpCode, sizeof(sdr_goods.GrpCode));
		}
		//
		// Идентификатор товара
		//
		sdr_goods.GoodsID = pPack->Rec.ID;
		//
		// Срок годности в днях от текущей даты
		//
		{
			const LDATE cur_dt = getcurdate_();
			sdr_goods.ExpiryFromCurDt = (sdr_goods.Expiry > cur_dt) ? diffdate(sdr_goods.Expiry, cur_dt) : 0;
		}
		STRNSCPY(sdr_goods.FlgPassive, STextConst::GetBool(pPack->Rec.Flags & GF_PASSIV));
		STRNSCPY(sdr_goods.FlgNoDiscount, STextConst::GetBool(pPack->Rec.Flags & GF_NODISCOUNT));
		STRNSCPY(sdr_goods.FlgWantVetisCert, STextConst::GetBool(pPack->Rec.Flags & GF_WANTVETISCERT));
		P_IEGoods->GetParamConst().InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr_goods);
		{
			GoodsContext::Param gcp;
            gcp.GoodsID = pPack->Rec.ID;
			gcp.LocID = Param.LocID;
			gcp.Qtty = 1.0;
			{
				gcp.Price = sdr_goods.Price;
				gcp.Cost = sdr_goods.Cost;
				gcp.Flags |= (gcp.fCostSettled|gcp.fPriceSettled);
			}
			GoodsContext ctx(gcp);
			P_IEGoods->SetExprContext(&ctx);
			THROW(P_IEGoods->AppendRecord(&sdr_goods, sizeof(sdr_goods)));
			P_IEGoods->SetExprContext(0);
		}
	}
	CATCHZOK
	return ok;
}
//
// Import
//
class PPGoodsImporter {
public:
	PPGoodsImporter() : P_IE(0), IsHier(0), UseTaxes(1)
	{
	}
	~PPGoodsImporter()
	{
		ZDELETE(P_IE);
	}
	int    Run(const char * pCfgName, int use_ta);
private:
	class ImageFileBlock : public SStrGroup {
	public:
		ImageFileBlock(const char * pSetPath);
		int    SetFile(const char * pFileName, PPID goodsID);

		struct Entry { // @flat
			PPID   GoodsID;
			uint   H;
			uint   W;
			int64  Sz;
			uint   FnP;
		};
		TSVector <Entry> List;
		const SString SetPath;
	private:
		SFsPath Ps;
		SString TempBuf; // @allocreuse
	};
	int    Helper_ImportHier(PPID defUnitID, HierArray * pHierList);
	int    LoadHierList(HierArray * pList);
	int    PutQCert(Sdr_Goods2 * pRec, PPID * pQcertID);
	int    PutTax(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack);
	int    PutExtStrData(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack);
	int    PutUnit(const Sdr_Goods2 & pRec, PPID defPhUnitID, PPID defUnitID, PPGoodsPacket * pPack);
	int    Resolve_Group(const Sdr_Goods2 & rRec, PPID * pGrpID);
	int    Resolve_Manuf(const Sdr_Goods2 & rRec, PPID * pManufID);
	int    CreateGoodsPacket(const Sdr_Goods2 & rRec, const char * pBarcode, PPGoodsPacket * pPack, PPLogger & rLogger);
	int    AssignClassif(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack);
	int    AssignEgaisCode(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack, PPLogger & rLogger);
	int    AssignFlags(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack);
	int    Helper_ProcessDirForImages(const char * pPath, ImageFileBlock & rBlk);
	void   SetupNameExt(PPGoodsPacket & rPack, const SString & rGoodsNameExt, const char * pExtLongNameLetter);

	int    IsHier;
	int    UseTaxes;
	PPGoodsImpExpParam Param;
	PPImpExp * P_IE;
	PPObjGoodsGroup GGObj;
	PPObjGoods GObj;
	PPObjBrand BrObj;
	PPObjQCert QcObj;
	PPObjPerson PsnObj;
	PPObjUnit UnitObj;
	PPObjWorld WObj;
};

struct CommonUnit { // @flat
	char   Abbr[128];
	uint   BaseId;   // SUOM_XXX
	double Rate;     // Отношение base / this
};

int PPLoadCommonUnits(TSVector <CommonUnit> * pList)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	CALLPTRMEMB(pList, freeAll());
	if(p_rez) {
		uint   num_items = 0;
		SString temp_buf;
		THROW_PP(p_rez->findResource(TAB_UNITS, PP_RCDATA), PPERR_RESFAULT);
		THROW_PP(num_items = p_rez->getUINT(), PPERR_RESFAULT);
		for(uint i = 0; i < num_items; i++) {
			CommonUnit entry;
			p_rez->getString(temp_buf, 2);
			temp_buf.CopyTo(entry.Abbr, sizeof(entry.Abbr));
			entry.BaseId = p_rez->getUINT();
			p_rez->getString(temp_buf, 0);
			entry.Rate = temp_buf.ToReal();
			if(pList)
				THROW_SL(pList->insert(&entry));
		}
	}
	CATCH
		CALLPTRMEMB(pList, freeAll());
		ok = 0;
	ENDCATCH
	return ok;
}

class TextFieldAnalyzer {
public:
	struct ResultBlock {
		uint   LexCount;
		SString ProcessedText;
	};
	TextFieldAnalyzer();
	~TextFieldAnalyzer();
	int    Process(const char * pText, ResultBlock * pBlk);

	SymbHashTable Words;
	RAssocArray WordCounter;
private:
	int    FASTCALL ScanLex(SString & rBuf);
	enum {
		reNumber = 1,
		reScale,
		reWord,
		reDiv,
		rePctRange,
		rePctNum,
		reManyInOne,
		reFrac,
		rePaperFmt,
		lexSymb
	};
	SStrScan Scan;
	int    ReInit;
	long   LastWordId;
	long   ReH_Word;
	long   ReH_Num;
	long   ReH_Div;
	long   ReH_Scale;
	long   ReH_PctNum;
	long   ReH_PctRange;
	long   ReH_ManyInOne;
	long   ReH_Frac;
	long   ReH_PaperFmt;
	TSVector <CommonUnit> Units;
	SString Prefixes;
	SString TempBuf; // @allocreuse
};

TextFieldAnalyzer::TextFieldAnalyzer() : Words(100000, 1), ReInit(0), LastWordId(0)
{
	PPLoadCommonUnits(&Units);
	PPLoadText(PPTXT_GOODSPREFIXES_RUS, Prefixes);
}

TextFieldAnalyzer::~TextFieldAnalyzer()
{
}

int FASTCALL TextFieldAnalyzer::ScanLex(SString & rBuf)
{
	if(!ReInit) {
		SString temp_buf;
		//Scan.RegisterRe("^(\\()?[^ \t\\;\\,\\.\\(\\)\\*\\+0-9]+[\\.\\)]?",    &ReH_Word);
		Scan.RegisterRe("^[^ \t\\;\\,\\.\\(\\)\\*\\+0-9]+[\\.]?",    &ReH_Word);
		Scan.RegisterRe("^[0-9]+([\\.,\\-][0-9]*)?", &ReH_Num);
		Scan.RegisterRe("^[\\;\\,\\.\\-\\*\\+\\/]",                  &ReH_Div);
		Scan.RegisterRe("^[m]?[0-9]+[ ]*:[ ]*[0-9]+",             &ReH_Scale); // Масштаб
		Scan.RegisterRe("^[0-9]+([\\.,][0-9]*)\\-[0-9]+([\\.,][0-9]*)?[ ]*%", &ReH_PctRange);
		Scan.RegisterRe("^[0-9]+([\\.,\\-][0-9]*)?[ ]*%", &ReH_PctNum);
		(temp_buf = "^[2-6][ ]*((in)|[вВ]|(\\/))[ ]*1").Transf(CTRANSF_OUTER_TO_INNER);
		Scan.RegisterRe(temp_buf, &ReH_ManyInOne);
		Scan.RegisterRe("^[0-9]+(\\/[0-9]+)+", &ReH_Frac);
		(temp_buf = "^[AaАа][1-6][ ]").Transf(CTRANSF_OUTER_TO_INNER); // Вторая пара Aa - русские буквы //
		Scan.RegisterRe(temp_buf, &ReH_PaperFmt);
		ReInit = 1;
	}
	int    lex = 0;
	if(Scan.GetRe(ReH_ManyInOne, rBuf))
		lex = reManyInOne;
	else if(Scan.GetRe(ReH_Frac, rBuf))
		lex = reFrac;
	else if(Scan.GetRe(ReH_PctRange, rBuf))
		lex = rePctRange;
	else if(Scan.GetRe(ReH_PctNum, rBuf))
		lex = rePctNum;
	else if(Scan.GetRe(ReH_Num, rBuf))
		lex = reNumber;
	else if(Scan.GetRe(ReH_Scale, rBuf))
		lex = reScale;
	else if(Scan.GetRe(ReH_PaperFmt, rBuf)) {
		SString temp_buf;
		lex = rePaperFmt;
		temp_buf.CatChar('A').CatChar(rBuf[1]);
		rBuf = temp_buf;
	}
	else if(Scan.GetRe(ReH_Word, rBuf))
		lex = reWord;
	else if(Scan.GetRe(ReH_Div, rBuf))
		lex = reDiv;
	else if(Scan[0] == 0) {
		lex = 0;
		rBuf.Z();
	}
	else {
		lex = lexSymb;
		rBuf.Z().CatChar(Scan[0]);
		Scan.Incr();
	}
	return lex;
}

int TextFieldAnalyzer::Process(const char * pText, ResultBlock * pRetBlk)
{
	// "бесплатно!,бесплатно,беспл"
	int    ok = 1;
	uint   i;
	SString text; // don't modify after Scan.Set(text, 0)
	SString next_buf, temp_buf, new_text;
	SString single_measure, single_pct_measure;
	uint   single_measure_idx = 0;
	uint   single_pct_measure_idx = 0;
	StringSet word_set;
	uint   word_count = 0;
	(text = pText).Strip();
	while(text.Last() == '*')
		text.TrimRight();
	Scan.Set(text, 0);
	Scan.Skip();
	int    lex = 0;
	int    lex_next = 0;
	uint   c = 0; // Счетчик слов
	do {
		int    unit_id = 0;
		double unit_meas = 0.0;
		if(lex_next) {
			lex = lex_next;
			TempBuf = next_buf;
			lex_next = 0;
		}
		else {
			lex = ScanLex(TempBuf);
		}
		if(lex) {
			Scan.Skip();
			lex_next = ScanLex(next_buf);
			if(lex == reWord) {
				if(lex_next == reWord && PPSearchSubStr(Prefixes, 0, TempBuf, 1)) {
					TempBuf.ToLower().Space().Cat(next_buf);
					lex_next = 0;
					next_buf.Z();
				}
				if(lex_next == reWord && next_buf.C(0) == '-') {
					TempBuf.Cat(next_buf);
					lex_next = 0;
					next_buf.Z();
				}
				if(lex_next == reWord && TempBuf.Last() == '-') {
					TempBuf.Cat(next_buf);
					lex_next = 0;
					next_buf.Z();
				}
				TempBuf.TrimRightChr('.');
			}
			else if(lex == rePctNum || lex == rePctRange) {
				if(single_pct_measure.IsEmpty()) {
					single_pct_measure = TempBuf;
					single_pct_measure_idx = word_count;
				}
				else
					single_pct_measure.Z();
				//
				// Для алкоголя после процентов иногда следует "vol" - это опускаем.
				//
				if(lex_next == reWord && next_buf.IsEqiAscii("vol")) {
					lex_next = 0;
					next_buf.Z();
				}
			}
			else if(lex == reNumber && !TempBuf.HasChr('-') && TempBuf.Last() != '%') {
				int normalize_val = 1;
				TempBuf.ReplaceChar(',', '.');
				double val = R6(TempBuf.ToReal());
				if(lex_next == reWord) {
					StringSet uss(',', 0);
					SString unit_abbr, first_unit_abbr;
					(temp_buf = next_buf).ToLower().TrimRightChr('.').Strip();
					if(temp_buf.IsEqiAscii("cl")) {
						temp_buf = "ml";
						val *= 10.0;
					}
					for(i = 0; i < Units.getCount(); i++) {
						const char * p_abbr_set = Units.at(i).Abbr;
						uss.setBuf(p_abbr_set, sstrlen(p_abbr_set)+1);
						for(uint j = 0, idx = 0; uss.get(&j, unit_abbr); idx++) {
							if(idx == 0)
								first_unit_abbr = unit_abbr;
							if(temp_buf.CmpNC(unit_abbr) == 0) {
								unit_id = Units.at(i).BaseId;
								TempBuf.Z().Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ)).Cat(first_unit_abbr);
								normalize_val = 0;
								lex_next = 0;
								next_buf.Z();
								if(single_measure.IsEmpty() && unit_id != SUOM_COLOR) {
									single_measure = TempBuf;
									single_measure_idx = word_count;
								}
								else
									single_measure.Z();
								break;
							}
						}
					}
					if(normalize_val)
						TempBuf.Z().Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				}
			}
			else if(lex == reDiv) {
				if(TempBuf.Len() == 1 && TempBuf[0] == '*' && lex_next == reNumber) {
					TempBuf = "x";
				}
			}
			else if(lex == reManyInOne) {
				TempBuf.Trim(1).Cat("in1");
			}
			uint   word_id = 0;
			TempBuf.Transf(CTRANSF_INNER_TO_OUTER);
			TempBuf.ReplaceChar('ё', 'е');
			TempBuf.ReplaceChar('Ё', 'Е');
			if(Words.Search(TempBuf, &word_id, 0)) {
				THROW_SL(WordCounter.Add(static_cast<long>(word_id), 1.0));
			}
			else {
				++LastWordId;
				word_id = static_cast<uint>(LastWordId);
				THROW_SL(Words.Add(TempBuf, word_id, 0));
				THROW_SL(WordCounter.Add(static_cast<long>(word_id), 1.0));
			}
			{
				word_set.add(TempBuf.Transf(CTRANSF_OUTER_TO_INNER));
				word_count++;
			}
			Scan.Skip();
		}
		c++;
	} while(lex);
	{
		uint   idx;
		int    prev_is_mult = 0;
		for(i = idx = 0; word_set.get(&i, TempBuf); idx++) {
			/*
			if(single_pct_measure.NotEmpty() && single_pct_measure_idx == idx)
				continue;
			*/
			if(single_measure.NotEmpty() && single_measure_idx == idx)
				continue;
			int this_is_mult = BIN(TempBuf.IsEqiAscii("x") || TempBuf == "+");
			const uchar last = static_cast<uchar>(new_text.Last());
			const uchar first = static_cast<uchar>(TempBuf[0]);
			if(last == '(' && first == ')') {
				new_text.TrimRight();
				TempBuf.ShiftLeft(1);
			}
			if(new_text.NotEmpty() && !(TempBuf.Len() == 1 && oneof2(first, '.', ',')) && !(prev_is_mult && isdec(first)) && !(this_is_mult && isdec(last)) &&
				!oneof4(last, ' ', '/', '-', '(') && !oneof2(first, '/', ')'))
				new_text.Space();
			new_text.Cat(TempBuf);
			prev_is_mult = this_is_mult;
		}
		if(single_measure.NotEmpty())
			new_text.Space().Cat(single_measure);
	}
	CATCHZOK
	if(pRetBlk)
		pRetBlk->ProcessedText = new_text;
	return ok;
}

int PPGoodsImporter::LoadHierList(HierArray * pList)
{
	int    ok = 1;
	P_IE->CloseFile();
	if(P_IE->OpenFileForReading(0)) {
		long   numrecs = 0;
		P_IE->GetNumRecs(&numrecs);
		for(uint i = 0; i < static_cast<uint>(numrecs); i++) {
			Sdr_Goods2 sdr_rec;
			P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec));
			pList->Add(strip(sdr_rec.HierObjCode), strip(sdr_rec.HierParentCode));
		}
	}
	return ok;
}

int PPGoodsImporter::Helper_ImportHier(PPID defUnitID, HierArray * pHierList)
{
	int    ok = 1;
	uint   i = 0;
	PPIDArray id_list;
	if(IsHier) {
		long numrecs = 0;
		SString wait_msg;
		IterCounter cntr;
		P_IE->GetNumRecs(&numrecs);
		cntr.SetTotal(numrecs);
		THROW(LoadHierList(pHierList));
		P_IE->CloseFile();
		THROW(P_IE->OpenFileForReading(0));
		PPLoadText(PPTXT_IMPGOODSGRP, wait_msg);
		for(i = 0; i < static_cast<uint>(numrecs); i++) {
			PPID   id = 0;
			Sdr_Goods2 sdr_rec;
			P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec));
			if(strip(sdr_rec.HierObjCode) && pHierList->IsThereChildOf(sdr_rec.HierObjCode)) {
				THROW(GGObj.AddSimple(&id, gpkndOrdinaryGroup, 0, strip(sdr_rec.Name), sdr_rec.HierObjCode, defUnitID, 0));
				THROW(id_list.addUnique(id));
			}
			PPWaitPercent(cntr.Increment(), wait_msg);
		}
	}
	for(i = 0; i < id_list.getCount(); i++) {
		char parent_code[64];
		Goods2Tbl::Rec gg_rec;
		if(GGObj.Search(id_list.at(i), &gg_rec) > 0) {
			BarcodeArray bc_list;
			THROW(GGObj.ReadBarcodes(id_list.at(i), bc_list));
			if(bc_list.getCount()) {
				if(pHierList->SearchParentOf(bc_list.at(0).Code+1, parent_code, sizeof(parent_code))) {
					BarcodeTbl::Rec barcode_rec;
					if(GGObj.SearchCode(parent_code, &barcode_rec) > 0) {
						Goods2Tbl::Rec parent_rec;
						if(GGObj.Search(barcode_rec.GoodsID, &parent_rec) > 0 && parent_rec.Kind == PPGDSK_GROUP) {
							PPID temp_id = 0;
							if(!(parent_rec.Flags & GF_FOLDER)) {
								temp_id = parent_rec.ID;
								parent_rec.Flags |= GF_FOLDER;
								THROW(GGObj.P_Tbl->Update(&temp_id, &parent_rec, 0));
							}
							gg_rec.ParentID = barcode_rec.GoodsID;
							temp_id = id_list.at(i);
							THROW(GGObj.P_Tbl->Update(&temp_id, &gg_rec, 0));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	P_IE->CloseFile();
	return ok;
}

int PPGoodsImporter::PutQCert(Sdr_Goods2 * pRec, PPID * pQcertID)
{
	int    ok = 1;
	PPID   qcert_id = 0;
	if(pRec && *strip(pRec->QCNumber) && QcObj.SearchByCode(pRec->QCNumber, &qcert_id, 0) <= 0) {
		QualityCertTbl::Rec qc_rec;
		STRNSCPY(qc_rec.Code,      strip(pRec->QCNumber));
		STRNSCPY(qc_rec.BlankCode, strip(pRec->QCBlank));
		if(checkdate(pRec->QCDate, 1))
			qc_rec.InitDate = pRec->QCDate;
		if(checkdate(pRec->QCExpiry, 1))
			qc_rec.Expiry   = pRec->QCExpiry;
		STRNSCPY(qc_rec.GoodsName, strip(pRec->QCManuf));
		STRNSCPY(qc_rec.InnerCode, strip(pRec->QCInnerCode));
		STRNSCPY(qc_rec.Etc,       strip(pRec->QCEtc));
		STRNSCPY(qc_rec.SPrDate, strip(pRec->QCManufDate));
		if(*strip(pRec->QCOrg))
			THROW(PsnObj.AddSimple(&qc_rec.RegOrgan, pRec->QCOrg, PPPRK_BUSADMIN, PPPRS_LEGAL, 0));
		THROW(QcObj.PutPacket(&qcert_id, &qc_rec, 0));
	}
	CATCHZOK
	ASSIGN_PTR(pQcertID, qcert_id);
	return ok;
}

int PPGoodsImporter::PutExtStrData(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack)
{
	int    ok = -1;
	if(pPack) {
		SString temp_buf;
		pPack->PutExtStrData(GDSEXSTR_A, (temp_buf = rRec.ExtA).Strip());
		pPack->PutExtStrData(GDSEXSTR_B, (temp_buf = rRec.ExtB).Strip());
		pPack->PutExtStrData(GDSEXSTR_C, (temp_buf = rRec.ExtC).Strip());
		pPack->PutExtStrData(GDSEXSTR_D, (temp_buf = rRec.ExtD).Strip());
		pPack->PutExtStrData(GDSEXSTR_E, (temp_buf = rRec.ExtE).Strip());
		ok = 1;
	}
	return ok;
}

int PPGoodsImporter::PutTax(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack)
{
	int    ok = -1;
	if(UseTaxes && pPack) {
		PPID   tax_grp_id = 0;
		double vat_rate = rRec.Vat;
		// @v11.5.5 {
		// Иногда бывает ставка НДС указана в долях от единицы. Так как ставки НДС меньше 1%, по-моему, не бывает, то 
		// именно так и трактуем это значение.
		if(vat_rate > 0.0 && vat_rate <= 0.4) {
			vat_rate *= 100.0;
		}
		// } @v11.5.5 
		GObj.GTxObj.GetByScheme(&pPack->Rec.TaxGrpID, vat_rate, 0, rRec.SalesTax, 0, 0/*use_ta*/);
		ok = 1;
	}
	return ok;
}

int PPGoodsImporter::PutUnit(const Sdr_Goods2 & rRec, PPID defPhUnitID, PPID defUnitID, PPGoodsPacket * pPack)
{
	int    ok = 1;
	if(pPack) {
		SString temp_buf(rRec.UnitName);
		if(temp_buf.NotEmptyS()) {
			THROW(UnitObj.AddSimple(&pPack->Rec.UnitID, temp_buf, 0, 0));
		}
		else if(defUnitID)
			pPack->Rec.UnitID = defUnitID;
		temp_buf = rRec.PhUPerU;
		if(temp_buf.NotEmptyS()) {
			char   phperu_buf[64]/*, val_buf[64]*/;
			SString val_buf;
			STRNSCPY(phperu_buf, temp_buf);
			char * p = phperu_buf;
			while(isdec(*p) || *p == '.' || *p == ',') {
				if(*p == ',')
					*p = '.';
				val_buf.CatChar(*p++);
			}
			double phperu = satof(val_buf);
			if(phperu > 0.0) {
				temp_buf = rRec.PhUnitName;
				if(temp_buf.NotEmptyS()) {
					val_buf = temp_buf;
					if(val_buf.IsEqiAscii("g") || stricmp1251(val_buf, "г") == 0 || stricmp1251(val_buf, "гр") == 0) {
						PPLoadString("munit_kg", val_buf);
						phperu /= 1000L;
					}
					else if(val_buf.IsEqiAscii("kg"))
						PPLoadString("munit_kg", val_buf);
					else if(stricmp1251(val_buf, "л") == 0)
						PPLoadString("munit_liter", val_buf);
					else if(stricmp1251(val_buf, "мл") == 0) {
						PPLoadString("munit_liter", val_buf);
						phperu /= 1000L;
					}
					THROW(UnitObj.AddSimple(&pPack->Rec.PhUnitID, val_buf, PPUnit::Trade|PPUnit::Physical, 0));
				}
				else
					pPack->Rec.PhUnitID = defPhUnitID;
				if(pPack->Rec.PhUnitID)
					pPack->Rec.PhUPerU = phperu;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPGoodsImporter::Resolve_Group(const Sdr_Goods2 & rRec, PPID * pGrpID)
{
	int    ok = 1;
	PPID   parent_id = 0;
	SString grpcode(rRec.GrpCode);
	SString temp_buf;
	if(IsHier || grpcode.NotEmptyS()) {
		BarcodeTbl::Rec barcode_rec;
		temp_buf = IsHier ? rRec.HierParentCode : rRec.GrpCode;
		if(temp_buf.NotEmptyS() && GGObj.SearchCode(temp_buf, &barcode_rec) > 0)
			parent_id = barcode_rec.GoodsID;
	}
	temp_buf = rRec.GrpName;
	if(!parent_id && temp_buf.NotEmptyS()) {
		PPID   folder_id = 0;
		SString folder_name(rRec.FolderGrpName);
		if(folder_name.NotEmptyS())
			THROW(GGObj.AddSimple(&folder_id, gpkndFolderGroup, 0, folder_name, 0, 0, 0));
		THROW(GGObj.AddSimple(&parent_id, gpkndOrdinaryGroup, folder_id, temp_buf /*grpname*/, rRec.GrpCode, Param.DefUnitID, 0));
	}
	if(!parent_id) {
		if(Param.DefParentID == 0)
			THROW(GGObj.AddSimple(&Param.DefParentID, gpkndOrdinaryGroup, 0, "0", 0, Param.DefUnitID, 0));
		parent_id = Param.DefParentID;
		ok = -1;
	}
	CATCHZOK
	ASSIGN_PTR(pGrpID, parent_id);
	return ok;
}

int PPGoodsImporter::Resolve_Manuf(const Sdr_Goods2 & rRec, PPID * pManufID)
{
	int    ok = 1;
	PPID   manuf_id = 0;
	SString temp_buf(rRec.ManufName);
	if(temp_buf.NotEmptyS()) {
		THROW(PsnObj.AddSimple(&manuf_id, temp_buf, PPPRK_MANUF, PPPRS_LEGAL, 0));
	}
	if(!manuf_id) {
		temp_buf = rRec.Country;
		if(temp_buf.NotEmptyS()) {
			THROW(PsnObj.AddSimple(&manuf_id, temp_buf, PPPRK_MANUF, PPPRS_COUNTRY, 0));
		}
		else {
			temp_buf = rRec.CountryCode;
			if(temp_buf.NotEmptyS()) {
				PPID   cntry_id = 0;
				WorldTbl::Rec w_rec;
				if(WObj.SearchByName(WORLDOBJ_COUNTRY, temp_buf, &w_rec) > 0)
					THROW(PsnObj.AddSimple(&manuf_id, w_rec.Name, PPPRK_MANUF, PPPRS_COUNTRY, 0));
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pManufID, manuf_id);
	return ok;
}

int PPGoodsImporter::AssignClassif(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack)
{
	int    ok = -1;
	if(pPack->Rec.GdsClsID) {
		if(rRec.DimX != 0.0 || rRec.DimY != 0.0 || rRec.DimZ != 0.0 || rRec.DimW != 0.0 ||
			rRec.PropKindName[0] || rRec.PropGradeName[0] || rRec.PropAddName[0] || rRec.PropAdd2Name[0]) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack; // PPGdsCls2 GoodsExtTbl
			if(gc_obj.Fetch(pPack->Rec.GdsClsID, &gc_pack) > 0) {
				if(rRec.DimX != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimX) {
					gc_pack.RealToExtDim(rRec.DimX, PPGdsCls::eX, &pPack->ExtRec.X);
					ok = 1;
				}
				if(rRec.DimY != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimY) {
					gc_pack.RealToExtDim(rRec.DimY, PPGdsCls::eY, &pPack->ExtRec.Y);
					ok = 1;
				}
				if(rRec.DimZ != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimZ) {
					gc_pack.RealToExtDim(rRec.DimZ, PPGdsCls::eZ, &pPack->ExtRec.Z);
					ok = 1;
				}
				if(rRec.DimW != 0.0 && gc_pack.Rec.Flags & PPGdsCls::fUseDimW) {
					gc_pack.RealToExtDim(rRec.DimW, PPGdsCls::eW, &pPack->ExtRec.W);
					ok = 1;
				}
				if(rRec.PropKindName[0] && gc_pack.Rec.Flags & PPGdsCls::fUsePropKind) {
					THROW(gc_pack.PropNameToID(PPGdsCls::eKind, rRec.PropKindName, &pPack->ExtRec.KindID, 1, 0));
				}
				if(rRec.PropGradeName[0] && gc_pack.Rec.Flags & PPGdsCls::fUsePropGrade) {
					THROW(gc_pack.PropNameToID(PPGdsCls::eGrade, rRec.PropGradeName, &pPack->ExtRec.GradeID, 1, 0));
				}
				if(rRec.PropAddName[0] && gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd) {
					THROW(gc_pack.PropNameToID(PPGdsCls::eAdd, rRec.PropAddName, &pPack->ExtRec.AddObjID, 1, 0));
				}
				if(rRec.PropAdd2Name[0] && gc_pack.Rec.Flags & PPGdsCls::fUsePropAdd2) {
					THROW(gc_pack.PropNameToID(PPGdsCls::eAdd2, rRec.PropAdd2Name, &pPack->ExtRec.AddObj2ID, 1, 0));
				}
				gc_pack.CompleteGoodsPacket(pPack);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPGoodsImporter::CreateGoodsPacket(const Sdr_Goods2 & rRec, const char * pBarcode, PPGoodsPacket * pPack, PPLogger & rLogger)
{
	int    ok = 1;
	PPID   parent_id = 0;
	char   goods_name[256];
	PPID   def_unit_id = 0;
	SString temp_buf;
	PPUnit u_rec;
	if(Param.DefUnitID && UnitObj.Fetch(Param.DefUnitID, &u_rec) > 0 && u_rec.Flags & PPUnit::Trade)
		def_unit_id = Param.DefUnitID;
	STRNSCPY(goods_name, rRec.Name);
	strip(goods_name);
	THROW(Resolve_Group(rRec, &parent_id));
	THROW(GObj.InitPacket(pPack, gpkndGoods, parent_id, 0, pBarcode));
	if(rRec.ClsCode[0]) {
		PPID   cls_id = 0;
		PPObjGoodsClass gc_obj;
		if(gc_obj.SearchBySymb(rRec.ClsCode, &cls_id, 0) > 0)
			pPack->Rec.GdsClsID = cls_id;
	}
	STRNSCPY(pPack->Rec.Name, goods_name);
	temp_buf = rRec.Abbr;
	if(temp_buf.NotEmptyS())
		STRNSCPY(pPack->Rec.Abbr, temp_buf);
	else
		STRNSCPY(pPack->Rec.Abbr, goods_name);
	THROW(PutUnit(rRec, Param.PhUnitID, def_unit_id, pPack));
	THROW(Resolve_Manuf(rRec, &pPack->Rec.ManufID));
	temp_buf = rRec.Brand;
	if(temp_buf.NotEmptyS())
		THROW(BrObj.AddSimple(&pPack->Rec.BrandID, temp_buf, 0, 0));
	THROW(PutExtStrData(rRec, pPack));
	THROW(PutTax(rRec, pPack));
	if(rRec.ExpiryPeriod > 0 && rRec.ExpiryPeriod < 365*20)
		pPack->Stock.ExpiryPeriod = static_cast<int16>(rRec.ExpiryPeriod);
	if(rRec.PckgQtty > 0.0)
		pPack->Stock.Package = rRec.PckgQtty;
	else if(rRec.UnitsPerPack > 0.0)
		pPack->Stock.Package = rRec.UnitsPerPack;
	if(rRec.MinShippmQtty > 0.0) {
		pPack->Stock.MinShippmQtty = rRec.MinShippmQtty;
		if(rRec.MultMinShippm)
			pPack->Stock.GseFlags |= GoodsStockExt::fMultMinShipm;
	}
	THROW(AssignClassif(rRec, pPack));
	AssignEgaisCode(rRec, pPack, rLogger);
	CATCHZOK
	return ok;
}

static int _IsTrueString(const char * pStr)
{
	if(!isempty(pStr) && (sstreq(pStr, "1") || sstreqi_ascii(pStr, "yes") || sstreqi_ascii(pStr, "Y") ||
		sstreqi_ascii(pStr, "true") || sstreqi_ascii(pStr, "T") || sstreqi_ascii(pStr, ".T.")))
		return 1;
	else
		return 0;
}

PPGoodsImporter::ImageFileBlock::ImageFileBlock(const char * pSetPath) : SetPath(pSetPath)
{
}

int PPGoodsImporter::ImageFileBlock::SetFile(const char * pFileName, PPID goodsID)
{
	int    ok = 1;
	SFile::Stat fs;
	if(SFile::GetStat(pFileName, 0, &fs, 0)) {
		{
			long   c = 0;
			Ps.Split(pFileName);
			(TempBuf = SetPath).SetLastSlash().Cat(Ps.Nam).Dot().Cat(Ps.Ext);
			while(fileExists(TempBuf)) {
				(TempBuf = SetPath).SetLastSlash().Cat(Ps.Nam).CatChar('#').Cat(++c).Dot().Cat(Ps.Ext);
			}
			SCopyFile(pFileName, TempBuf, 0, FILE_SHARE_READ, 0);
		}
		if(goodsID) {
			Entry entry;
			MEMSZERO(entry);
			entry.GoodsID = goodsID;
			entry.Sz = fs.Size;
			uint pos = 0;
			if(List.lsearch(&goodsID, &pos, CMPF_LONG)) {
				Entry & r_entry = List.at(pos);
				if(entry.Sz > r_entry.Sz) {
					r_entry.Sz = entry.Sz;
					AddS(pFileName, &r_entry.FnP);
				}
			}
			else {
				AddS(pFileName, &entry.FnP);
				List.insert(&entry);
			}
		}
	}
	return ok;
}

int PPGoodsImporter::Helper_ProcessDirForImages(const char * pPath, ImageFileBlock & rBlk)
{
	int    ok = 1;
	SString temp_buf;
	SString code_buf;
	SString file_name;
	SFsPath ps;
	(temp_buf = pPath).SetLastSlash().Cat("*.*");
	SDirEntry de;
	for(SDirec direc(temp_buf); direc.Next(&de) > 0;) {
		de.GetNameA(file_name);
		if(de.IsFolder()) {
			if(!de.IsSelf() && !de.IsUpFolder() && !sstreqi_ascii(file_name, "__SET__")) {
				(temp_buf = pPath).SetLastSlash().Cat(file_name);
				THROW(Helper_ProcessDirForImages(temp_buf, rBlk)); // @recursion
			}
		}
		else {
			(temp_buf = pPath).SetLastSlash().Cat(file_name);
			PPWaitMsg(temp_buf);
			SFileFormat ff;
			const int fir = ff.Identify((temp_buf = pPath).SetLastSlash().Cat(file_name));
			if(oneof2(fir, 2, 3) && SImageBuffer::IsSupportedFormat(ff)) { // Принимаем только идентификацию по сигнатуре // @v11.3.7 SFileFormat::Webp
				BarcodeTbl::Rec bc_rec;
				ps.Split(file_name);
				code_buf = ps.Nam;
				if(code_buf.Len() > 6) {
					//
					// Сначала пытаемся найти код, соответствующий полному имени файла.
					// Эта попытка нужна для того, чтобы импортировать изображения, экспортированные Papyrus'ом же
					// и имеющие нецифровые символы в коде.
					//
					if(GObj.SearchByBarcode(code_buf.ToUpper(), &bc_rec, 0, 1) > 0) {
						THROW(rBlk.SetFile(temp_buf, bc_rec.GoodsID));
					}
					else {
						//
						// Если не получилось по полному имени, то извлекаем из имени максимально длинные цифровые последовательности
						// и пытаемся рассматривать их как коды.
						//
						size_t _pos = 0;
						while(ps.Nam[_pos]) {
							if(isdec(ps.Nam[_pos])) {
								code_buf.Z();
								const size_t _start = _pos;
								do {
									code_buf.CatChar(ps.Nam[_pos++]);
									if(code_buf.Len() > 6) {
										if(GObj.SearchByBarcode(code_buf, &bc_rec, 0, 0) > 0) {
											THROW(rBlk.SetFile(temp_buf, bc_rec.GoodsID));
										}
										else {
											int dr = PPObjGoods::DiagBarcode(code_buf, 0, 0, 0);
											if(dr > 0) {
												THROW(rBlk.SetFile(temp_buf, 0));
											}
										}
									}
								} while(isdec(ps.Nam[_pos]));
                                if(code_buf.Len() > 6) {
									_pos = _start+1;
                                }
							}
							else
								_pos++;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void PPGoodsImporter::SetupNameExt(PPGoodsPacket & rPack, const SString & rGoodsNameExt, const char * pExtLongNameLetter)
{
	const char ext_letter = pExtLongNameLetter ? toupper(pExtLongNameLetter[0]) : 0;
	if(ext_letter && rGoodsNameExt.NotEmpty()) {
		SString temp_buf;
		struct LToI { char L; int I; } ltoi_list[] = { { 'A', GDSEXSTR_A }, { 'B', GDSEXSTR_B }, { 'C', GDSEXSTR_C }, { 'D', GDSEXSTR_D }, { 'E', GDSEXSTR_E } };
		for(uint i = 0; i < SIZEOFARRAY(ltoi_list); i++) {
			if(ext_letter == ltoi_list[i].L) {
				if(rPack.GetExtStrData(ltoi_list[i].I, temp_buf) < 0 || !temp_buf.NotEmptyS())
					rPack.PutExtStrData(ltoi_list[i].I, rGoodsNameExt);
				break;
			}
		}
	}
}

int PPGoodsImporter::AssignFlags(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack)
{
	const  long org_flags = pPack->Rec.Flags;
	if(!isempty(rRec.FlgPassive)) {
		SETFLAG(pPack->Rec.Flags, GF_PASSIV, _IsTrueString(rRec.FlgPassive));
	}
	if(!isempty(rRec.FlgNoDiscount)) {
		SETFLAG(pPack->Rec.Flags, GF_NODISCOUNT, _IsTrueString(rRec.FlgNoDiscount));
	}
	if(!isempty(rRec.FlgWantVetisCert)) {
		SETFLAG(pPack->Rec.Flags, GF_WANTVETISCERT, _IsTrueString(rRec.FlgWantVetisCert));
	}
	return (pPack->Rec.Flags != org_flags) ? 1 : -1;
}

int PPGoodsImporter::AssignEgaisCode(const Sdr_Goods2 & rRec, PPGoodsPacket * pPack, PPLogger & rLogger)
{
	int    ok = -1;
	if(rRec.EgaisCode[0]) {
		SString temp_buf, msg_buf, fmt_buf;
		(temp_buf = rRec.EgaisCode).Strip();
		if(temp_buf.Len() == 19 && temp_buf.IsDec()) {
			BarcodeTbl::Rec egais_code_rec;
			if(GObj.SearchByBarcode(temp_buf, &egais_code_rec, 0, 0) > 0) {
				PPLoadText(PPTXT_IMPEGAISCODEXISTS, fmt_buf);
				rLogger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
			}
			else {
				pPack->Codes.Add(temp_buf, 0, 1.0);
				PPLoadText(PPTXT_IMPEGAISCODACCEPTED, fmt_buf);
				rLogger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
				ok = 1;
			}
		}
		else {
			PPLoadText(PPTXT_IMPEGAISCODEINV, fmt_buf);
			rLogger.Log(msg_buf.Printf(fmt_buf, temp_buf.cptr()));
		}
	}
	return ok;
}

int PPGoodsImporter::Run(const char * pCfgName, int use_ta)
{
	const  size_t max_nm_len = sizeof(static_cast<const Goods2Tbl::Rec *>(0)->Name)-1;

	int    ok = 1;
	int    r = 0;
	int    codetohex = 0;
	int    err_barcode = 0;
	IterCounter cntr;
	SString file_name;
	SString wait_msg;
	SString temp_buf2;
	SString ext_fld_buf;
	SString err_msg_buf;
	SString fmt_buf;
	SString msg_buf;
	SString ini_file_name;
	SString goods_name;
	TextFieldAnalyzer::ResultBlock txt_blk[2];
	const  uint sect = PPINISECT_IMP_GOODS;
	BarcodeTbl::Rec barcode_rec;
	PPObjGoodsClass gc_obj;
	PPObjArticle ar_obj;
	PPObjTag tag_obj;
	ArticleTbl::Rec ar_rec;
	HierArray hier_list;
	PPLogger  logger;
	PPID   suppl_acs_id = GetSupplAccSheet();
	PPID   suppl_srch_reg_id = 0;
	PPObjArticle::GetSearchingRegTypeID(suppl_acs_id, 0, 1, &suppl_srch_reg_id);
	ZDELETE(P_IE);
	THROW(LoadSdRecord(PPREC_GOODS2, &Param.InrRec));
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		if(pCfgName) {
			StrAssocArray list;
			PPGoodsImpExpParam param;
			Param.Name = pCfgName;
			Param.Direction = 1;
			THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_GOODS2, &param, &list, 2));
			for(uint i = 1; i < list.getCount(); i++) {
				list.GetText(i, temp_buf2);
				if(strstr(temp_buf2, pCfgName)) {
					Param.ProcessName(1, temp_buf2);
					Param.ReadIni(&ini_file, temp_buf2, 0);
					r = 1;
					break;
				}
			}
		}
		else if(SelectGoodsImportCfgs(&Param, 1) > 0) {
			r = 1;
		}
		if(r == 1) {
			PPID   warehouse_id = 0;
			int    matrix_action = -1;
			LAssoc subcode;
			{
				warehouse_id = Param.LocID;
				matrix_action = Param.MatrixAction;
				if(!GObj.GetConfig().MtxQkID) {
					THROW_PP(matrix_action < 1000, PPERR_UNDEFGOODSMATRIX);
					matrix_action = -1;
				}
			}
			{
				SString o, s;
				Param.SubCode.Divide(',', o, s);
				subcode.Key = o.ToLong();
				subcode.Val = s.ToLong();
				if((subcode.Key < 0 || subcode.Key > 32) || (subcode.Val < 1 || subcode.Val > 32))
					subcode.Key = subcode.Val = 0; // @error
			}
			TextFieldAnalyzer txt_anlzr;
			Param.FileName.Transf(CTRANSF_INNER_TO_OUTER);
			THROW_MEM(P_IE = new PPImpExp(&Param, 0));
			PPWaitStart();
			PPLoadText(PPTXT_IMPGOODS, wait_msg);
			PPWaitMsg(wait_msg);
			if(Param.Flags & PPGoodsImpExpParam::fImportImages) {
				file_name = Param.FileName;
				SString set_path, line_buf;
				SFsPath ps(Param.FileName);
				ps.Merge(SFsPath::fDrv|SFsPath::fDir, file_name);
				THROW_SL(SFile::IsDir(file_name.RmvLastSlash()));
				(set_path = file_name).SetLastSlash().Cat("__SET__");
				if(!fileExists(set_path)) {
					THROW_SL(SFile::CreateDir(set_path));
				}
				{
					ImageFileBlock blk(set_path);
					THROW(Helper_ProcessDirForImages(file_name, blk));
					{
						set_path.SetLastSlash().Cat("imagelist.txt");
						SFile listf(set_path, SFile::mWrite);
						Goods2Tbl::Rec goods_rec;
						blk.List.sort(CMPF_LONG);
						{
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < blk.List.getCount(); i++) {
								const ImageFileBlock::Entry & r_entry = blk.List.at(i);
								line_buf.Z().Cat(GetGoodsName(r_entry.GoodsID, temp_buf2)).Tab();
								blk.GetS(r_entry.FnP, temp_buf2);
								line_buf.Cat(temp_buf2).CR();
								listf.WriteLine(line_buf);
								if(GObj.Search(r_entry.GoodsID, &goods_rec) > 0) {
									ObjLinkFiles _lf(PPOBJ_GOODS);
									THROW(_lf.Load(r_entry.GoodsID, 0L));
									if(!_lf.At(0, temp_buf2) || !fileExists(temp_buf2)) {
										blk.GetS(r_entry.FnP, temp_buf2);
										THROW(_lf.Replace(0, temp_buf2));
										THROW(_lf.Save(r_entry.GoodsID, 0L));
										THROW(GObj.UpdateFlags(r_entry.GoodsID, GF_HASIMAGES, 0, 0));
									}
								}
							}
							THROW(tra.Commit());
						}
					}
				}
			}
			else if(Param.Flags & PPGoodsImpExpParam::fAnalyzeOnly) {
				if(P_IE->OpenFileForReading(0)) {
					PPGoodsImpExpParam exp_param; // PPImpExpParam
					exp_param = Param;
					exp_param.Direction = 0; // export
					SFsPath::ReplaceExt((temp_buf2 = Param.FileName), "out", 1);
					exp_param.FileName = temp_buf2;
					PPImpExp exp_blk(&exp_param, 0);
					exp_blk.OpenFileForWriting(0, 1);

					SFsPath::ReplaceExt((temp_buf2 = Param.FileName), "bcd", 1);
					SFile f_out_bcd(temp_buf2, SFile::mWrite);
					SFsPath::ReplaceExt((temp_buf2 = Param.FileName), "word", 1);
					SFile f_out_word(temp_buf2, SFile::mWrite);
					SFsPath::ReplaceExt((temp_buf2 = Param.FileName), "name", 1);
					SFile f_out_name(temp_buf2, SFile::mWrite);

					PPTextAnalyzer text_analyzer2;
					PPTextAnalyzer::Item text_analyzer_item;

					int    err_txt_anlz = 0;
					long   numrecs = 0;
					P_IE->GetNumRecs(&numrecs);
					cntr.SetTotal(numrecs);
					for(uint i = 0; i < static_cast<uint>(numrecs); i++) {
						Sdr_Goods2 sdr_rec;
						THROW(P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec)));
						P_IE->GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &sdr_rec);
						(temp_buf2 = Param.FileName).CatChar('#').Cat(i+1);
						(goods_name = sdr_rec.Name).Strip().ToLower().Transf(CTRANSF_INNER_TO_OUTER);
						//
						// Препроцессинг строки, обрамленной кавычками и содержащей замену кавычек спаренными кавычками
						// (часто встречается в CSV-файлах)
						//
						if(goods_name.C(0) == '\"' && goods_name.Last() == '\"') {
							goods_name.TrimRightChr('\"');
							goods_name.ShiftLeftChr('\"');
							goods_name.ReplaceStr("\"\"", "\"", 0);
						}
						{
							uint   j;
							uint   idx_first = 0, idx_count = 0;
							PROFILE(THROW_SL(text_analyzer2.Write(temp_buf2, 0, goods_name, goods_name.Len()+1)));
							PROFILE(THROW_SL(text_analyzer2.Run(&idx_first, &idx_count)));
							(msg_buf = goods_name).Tab();
							for(j = 0; j < idx_count; j++) {
								if(text_analyzer2.Get(idx_first+j, text_analyzer_item)) {
									msg_buf.Cat(text_analyzer_item.Text);
								}
							}
							for(j = 0; j < idx_count; j++) {
								if(text_analyzer2.Get(idx_first+j, text_analyzer_item)) {
									msg_buf.Tab().Cat(text_analyzer_item.Text);
								}
							}
							f_out_name.WriteLine(msg_buf.CR());
							PPWaitPercent(cntr.Increment(), wait_msg);
						}
					}
					{
						SymbHashTable::Stat stat;
						text_analyzer2.GetSymbHashStat(stat);
						(msg_buf = "SymbHashTable Statistics").CatDiv(':', 2);
	#define CATSTATFLD(f) msg_buf.CatEq(#f, (ulong)stat.f).Tab()
	#define CATSTATFLD_REAL(f) msg_buf.CatEq(#f, stat.f, MKSFMTD(0, 1, 0)).Tab()
						CATSTATFLD(NumEntries);
						CATSTATFLD(CountEmpty);
						CATSTATFLD(CountSingle);
						CATSTATFLD(CountMult);
						CATSTATFLD(CountItems);
						CATSTATFLD(Min);
						CATSTATFLD(Max);
						CATSTATFLD_REAL(Average);
						CATSTATFLD_REAL(StdDev);
	#undef CATSTATFLD
						PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME);
					}
				}
			}
			else {
				GoodsImportBillIdent bill_ident(&PsnObj, Param.SupplID);
				bill_ident.GetFldSet(&ini_file, sect, 0);
				GoodsImportBillIdent bill_ident_negop(&PsnObj, 0); // экземпляр GoodsImportBillIdent для отрицательных остатков
				bill_ident_negop.GetFldSet(&ini_file, sect, 0);
				PPEquipConfig eq_cfg;
				ReadEquipConfig(&eq_cfg);
				const  PPID op_for_neg_rest_id = NZOR(eq_cfg.OpOnDfctThisLoc, eq_cfg.OpOnTempSess);
				PPTransaction tra(use_ta);
				THROW(tra);
				if(IsHier)
					THROW(Helper_ImportHier(Param.DefUnitID, &hier_list));
				if(P_IE->OpenFileForReading(0)) {
					BarcodeArray added_code_list;
					SString obj_code;
					SString ar_code;
					SString suffix;
					SString goods_name_ext;
					long   numrecs;
					SdRecord dyn_rec;
					SdbField dyn_fld;
					THROW(P_IE->InitDynRec(&dyn_rec));
					P_IE->GetNumRecs(&numrecs);
					cntr.SetTotal(numrecs);
					for(uint i = 0; i < static_cast<uint>(numrecs); i++) {
						added_code_list.clear();
						obj_code.Z();
						ar_code.Z();
						goods_name_ext.Z();
						int    skip = 0;
						int    force_update = 0;
						int    is_found = 0;
						int    try_cls_name = 0; // Если !0 то при отсутствии имени товара попытаемся сформировать имя по импортированным атрибутам класса
						int    uhtt_update = 0;
						int    add_next_code = 0;
						PPID   goods_id = 0, parent_id = 0;
						PPID   nm_goods_id = 0; // Идентификатор товара, найденный по имени импортировуемой записи.
						PPID   suppl_id = Param.SupplID;
						char   barcode[32];
						char   subc[32];
						double rest = 0.0;
						Sdr_Goods2 sdr_rec;
						Goods2Tbl::Rec par_rec;
						PPGoodsPacket pack;
						ObjTagList tag_list;
						THROW(P_IE->ReadRecord(&sdr_rec, sizeof(sdr_rec), &dyn_rec));
						P_IE->GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, &sdr_rec);
						if(dyn_rec.GetCount()) {
							SStrScan scan;
							PPObjectTag tag_rec;
							PPID   tag_id = 0;
							for(uint j = 0; j < dyn_rec.GetCount(); j++) {
								dyn_rec.GetFieldByPos(j, &dyn_fld);
								if(dyn_fld.InnerFormula.NotEmptyS()) {
									scan.Set(dyn_fld.InnerFormula, 0);
									if(scan.GetIdent(temp_buf2.Z())) {
										if(temp_buf2.IsEqiAscii("tag")) {
											scan.Skip();
											if(scan[0] == '.') {
												scan.Incr(1);
												(temp_buf2 = scan).Strip();
												if(tag_obj.SearchBySymb(temp_buf2, &tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_GOODS) {
													int   r = 0;
													const TYPEID typ = dyn_fld.T.Typ;
													const int    base_typ = stbase(typ);
													ObjTagItem tag_item;
													switch(base_typ) {
														case BTS_INT:
															{
																long ival = 0;
																sttobase(typ, dyn_rec.GetDataC(j), &ival);
																if(ival) {
																	tag_item.SetInt(tag_id, ival);
																	r = 1;
																}
															}
															break;
														case BTS_REAL:
															{
																double rval = 0.0;
																sttobase(typ, dyn_rec.GetDataC(j), &rval);
																if(rval != 0.0) {
																	tag_item.SetReal(tag_id, rval);
																	r = 1;
																}
															}
															break;
														case BTS_STRING:
															{
																char strval[1024];
																sttobase(typ, dyn_rec.GetDataC(j), strval);
																if(strval[0]) {
																	(temp_buf2 = strval).Strip().Transf(CTRANSF_OUTER_TO_INNER);
																	tag_item.SetStr(tag_id, temp_buf2);
																	r = 1;
																}
															}
															break;
														case BTS_DATE:
															{
																LDATE dval = ZERODATE;
																sttobase(typ, dyn_rec.GetDataC(j), &dval);
																if(checkdate(dval)) {
																	tag_item.SetDate(tag_id, dval);
																	r = 1;
																}
															}
															break;
													}
													if(r)
														tag_list.PutItem(tag_id, &tag_item);
												}
											}
										}
									}
								}
							}
						}
						//
						// Уточняем ИД поставщика по полям записи BillSupplID или BillSupplCode
						//
						if(sdr_rec.BillSupplID && ar_obj.Fetch(sdr_rec.BillSupplID, &ar_rec) > 0)
							suppl_id = sdr_rec.BillSupplID;
						else if(sdr_rec.BillSupplCode[0] && suppl_srch_reg_id) {
							PPID   temp_id = 0;
							if(ar_obj.SearchByRegCode(suppl_acs_id, suppl_srch_reg_id, sdr_rec.BillSupplCode, &temp_id, 0) > 0)
								suppl_id = temp_id;
						}
						//
						rest = sdr_rec.Rest;
						(goods_name = sdr_rec.Name).Strip();
						//
						// Препроцессинг строки, обрамленной кавычками и содержащей замену кавычек спаренными кавычками
						// (часто встречается в CSV-файлах)
						//
						if(goods_name.C(0) == '\"' && goods_name.Last() == '\"') {
							goods_name.TrimRightChr('\"');
							goods_name.ShiftLeftChr('\"');
							goods_name.ReplaceStr("\"\"", "\"", 0);
						}
						if(goods_name.Strip().Len() >= sizeof(pack.Rec.Name)) {
							goods_name_ext = goods_name;
							goods_name.Trim(sizeof(pack.Rec.Name)-1);
						}
						if(IsHier) {
							obj_code.CopyFrom(strip(sdr_rec.HierObjCode));
							if(hier_list.IsThereChildOf(obj_code) || GGObj.SearchCode(obj_code, &barcode_rec) > 0)
								skip = 1;
						}
						if(!skip && goods_name.IsEmpty() && Param.DefParentID) {
							if(GObj.Fetch(Param.DefParentID, &par_rec) > 0 && par_rec.GdsClsID) {
								PPGdsClsPacket gc_pack;
								if(gc_obj.Fetch(par_rec.GdsClsID, &gc_pack) > 0 && gc_pack.NameConv.NotEmpty())
									try_cls_name = 1;
							}
						}
						if(!skip && (goods_name.NotEmpty() || sdr_rec.Code[0])) {
							memzero(barcode, sizeof(barcode));
							subc[0] = 0;
							if(*strip(sdr_rec.Code)) {
								STRNSCPY(barcode, strupr(sdr_rec.Code));
								if(subcode.Key >= 0 && subcode.Val > 1) {
									size_t bclen = sstrlen(barcode);
									if(subcode.Key < static_cast<int32>(bclen)) {
										bclen = MIN(static_cast<size_t>(subcode.Val), bclen-subcode.Key);
										memcpy(subc, barcode+subcode.Key, bclen);
										subc[bclen] = 0;
									}
								}
								{
									int    diag = 0, std = 0;
									int    r = PPObjGoods::DiagBarcode(barcode, &diag, &std, &temp_buf2);
									if(r <= 0) {
										PPObjGoods::GetBarcodeDiagText(diag, err_msg_buf);
										msg_buf.Z().Cat(std).Semicol().
											Cat(temp_buf2).Semicol().Cat(barcode).Semicol().Cat(err_msg_buf/*.ToOem()*/).CR();
										logger.Log(msg_buf);
										if(Param.Flags & PPGoodsImpExpParam::fUHTT) {
											temp_buf2.CopyTo(barcode, sizeof(barcode));
											if(!r)
												continue;
										}
									}
								}
							}
							if(*strip(sdr_rec.AddedCode)) {
								{
									int    diag = 0, std = 0;
									int    r = PPObjGoods::DiagBarcode(sdr_rec.AddedCode, &diag, &std, &temp_buf2);
									if(r <= 0) {
										PPObjGoods::GetBarcodeDiagText(diag, err_msg_buf);
										msg_buf.Z().Cat(std).Semicol().
											Cat(temp_buf2).Semicol().Cat(sdr_rec.AddedCode).Semicol().Cat(err_msg_buf/*.ToOem()*/).CR();
										logger.Log(msg_buf);
										if(Param.Flags & PPGoodsImpExpParam::fUHTT)
											if(r)
												temp_buf2.CopyTo(sdr_rec.AddedCode, sizeof(sdr_rec.AddedCode));
											else
												sdr_rec.AddedCode[0] = 0;
									}
								}
								if(sdr_rec.AddedCode[0]) {
									BarcodeTbl::Rec bc_entry;
									STRNSCPY(bc_entry.Code, strupr(sdr_rec.AddedCode));
									bc_entry.Qtty = (sdr_rec.AddedCodeQtty > 0.0) ? sdr_rec.AddedCodeQtty : 1.0;
									added_code_list.insert(&bc_entry);
								}
							}
							(ar_code = sdr_rec.ArCode).Strip();
							if(goods_name.NotEmpty()) {
								if(GObj.SearchByName(goods_name, &nm_goods_id, 0) <= 0)
									nm_goods_id = 0;
							}
							if(barcode[0] && GObj.SearchByBarcode(barcode, &barcode_rec, 0, 0) > 0) {
								goods_id = barcode_rec.GoodsID;
								is_found = 1;
								if(Param.Flags & PPGoodsImpExpParam::fUHTT) {
									BarcodeArray bc_list;
									GObj.ReadBarcodes(goods_id, bc_list);
									//
									// Если найденный по штрихкоду товар имеет более одного кода, то
									// отнимаем у него этот код в пользу нового товара.
									// Таким образом, мы автоматически (с некоторой вероятностью) сможем
									// уточнить значение конкретного штрихкода.
									//
									if(bc_list.getCount() > 1 && bc_list.Replace(barcode, 0) > 0) {
										if(!nm_goods_id) {
											THROW(GObj.P_Tbl->UpdateBarcodes(goods_id, &bc_list, 0));
											is_found = 0;
											goods_id = 0;
										}
									}
									else if(_IsTrueString(sdr_rec.ForceReplace))
										force_update = 1;
									uhtt_update = 1;
								}
							}
							else if(nm_goods_id) {
								if(barcode[0]) {
									if(Param.Flags & PPGoodsImpExpParam::fUHTT) {
										PPID   temp_nm_goods_id = 0;
										long   uc = 1;
										do {
											suffix.Z().Space().CatChar('#').Cat(++uc);
											temp_buf2 = goods_name;
											const size_t sum_len = temp_buf2.Len() + suffix.Len();
											if(sum_len > max_nm_len)
												temp_buf2.Trim(max_nm_len-suffix.Len());
											temp_buf2.Cat(suffix);
										} while(GObj.SearchByName(temp_buf2, &temp_nm_goods_id, 0) > 0);
										goods_name = temp_buf2;
									}
									else {
										BarcodeTbl::Rec bc_entry;
										STRNSCPY(bc_entry.Code, barcode);
										bc_entry.Qtty = 1.0;
										added_code_list.insert(&bc_entry);
										goods_id = nm_goods_id;
										is_found = 1;
									}
								}
								else {
									goods_id = nm_goods_id;
									is_found = 1;
								}
							}
							else if(subc[0] && GObj.SearchByBarcode(subc, &barcode_rec, 0, 0) > 0) {
								goods_id = barcode_rec.GoodsID;
								is_found = 1;
								if(_IsTrueString(sdr_rec.ForceReplace))
									force_update = 1;
							}
							if(is_found) {
								int    do_update = 0;
								int    do_add = 0;
								PPGoodsPacket pack2;
								THROW(GObj.GetPacket(goods_id, &pack, 0) > 0);
								if(subc[0] || ar_code.NotEmpty() || uhtt_update || sdr_rec.MinShippmQtty != 0.0) {
									if(subc[0] && pack.Codes.Replace(subc, barcode) > 0)
										do_update = 1;
									if(ar_code.NotEmpty()) {
										ArGoodsCodeTbl::Rec ar_code_rec;
										ar_code_rec.GoodsID = pack.Rec.ID;
										ar_code_rec.ArID = Param.SupplID;
										ar_code_rec.Pack = 1000; // 1.0
										ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
										{
											// Удаляем из товара старые коды этого же контрагента (рискованно, но пока так)
											// @todo - сделать в конфигурации импорта опцию, регулирующую вопрос замещения существующих кодов
											uint _ac = pack.ArCodes.getCount();
											if(_ac) do {
												ArGoodsCodeTbl::Rec & r_ac_rec = pack.ArCodes.at(--_ac);
												if(r_ac_rec.ArID == ar_code_rec.ArID)
													pack.ArCodes.atFree(_ac);
											} while(_ac);
										}
										pack.ArCodes.insert(&ar_code_rec);
										do_update = 1;
									}
									if(AssignClassif(sdr_rec, &pack) > 0)
										do_update = 1;
									if(uhtt_update) {
										goods_name.CopyTo(sdr_rec.Name, sizeof(sdr_rec.Name));
										THROW(CreateGoodsPacket(sdr_rec, barcode, &pack2, logger));
										SetupNameExt(pack2, goods_name_ext, sdr_rec.ExtLongNameTo);
										if(pack.Codes.getCount() > 1 && pack.Codes.Replace(barcode, 0) > 0) {
											if(nm_goods_id) {
												PPID   temp_nm_goods_id = 0;
												long   uc = 1;
												do {
													suffix.Z().Space().CatChar('#').Cat(++uc);
													(temp_buf2 = pack2.Rec.Name).Strip();
													size_t sum_len = temp_buf2.Len() + suffix.Len();
													if(sum_len > max_nm_len)
														temp_buf2.Trim(max_nm_len-suffix.Len());
													temp_buf2.Cat(suffix);
												} while(GObj.SearchByName(temp_buf2, &temp_nm_goods_id, 0) > 0);
												temp_buf2.CopyTo(pack2.Rec.Name, sizeof(pack2.Rec.Name));
											}
											do_add = 1;
											do_update = 1;
										}
										else {
											txt_anlzr.Process(pack.Rec.Name, &txt_blk[0]);
											txt_anlzr.Process(pack2.Rec.Name, &txt_blk[1]);
											if(force_update) {
												STRNSCPY(pack.Rec.Name, pack2.Rec.Name);
												STRNSCPY(pack.Rec.Abbr, pack2.Rec.Abbr);
												do_update = 1;
											}
											else if(txt_blk[1].ProcessedText.Len() > txt_blk[0].ProcessedText.Len()) {
												PPID   temp_nm_goods_id = 0;
												if(GObj.SearchByName(pack2.Rec.Name, &temp_nm_goods_id, 0) > 0) {
												}
												else {
													STRNSCPY(pack.Rec.Name, pack2.Rec.Name);
													STRNSCPY(pack.Rec.Abbr, pack2.Rec.Abbr);
													do_update = 1;
												}
											}
											if(pack2.Rec.BrandID && !pack.Rec.BrandID) {
												pack.Rec.BrandID = pack2.Rec.BrandID;
												do_update = 1;
											}
											if(pack2.Rec.PhUPerU != 0.0 && pack2.Rec.PhUnitID && pack.Rec.PhUPerU == 0.0) {
												pack.Rec.PhUnitID = pack2.Rec.PhUnitID;
												pack.Rec.PhUPerU = pack2.Rec.PhUPerU;
												do_update = 1;
											}
											if(pack2.Stock.Package != 0.0 && pack.Stock.Package == 0.0) {
												pack.Stock.Package = pack2.Stock.Package;
												do_update = 1;
											}
										}
									}
								}
								if(sdr_rec.MinShippmQtty != pack.Stock.MinShippmQtty || (sdr_rec.MultMinShippm && !(pack.Stock.GseFlags & GoodsStockExt::fMultMinShipm))) {
									pack.Stock.MinShippmQtty = sdr_rec.MinShippmQtty;
									if(sdr_rec.MultMinShippm)
										pack.Stock.GseFlags |= GoodsStockExt::fMultMinShipm;
									do_update = 1;
								}
								{
									static const uint8 ext_id_list[] = { GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E };
									for(uint eilidx = 0; eilidx < SIZEOFARRAY(ext_id_list); eilidx++) {
										if(PPGetExtStrData(ext_id_list[eilidx], pack2.ExtString, temp_buf2) > 0 && temp_buf2.NotEmptyS()) {
											PPGetExtStrData(ext_id_list[eilidx], pack.ExtString, ext_fld_buf);
											if(ext_fld_buf != temp_buf2) {
												PPPutExtStrData(ext_id_list[eilidx], pack.ExtString, temp_buf2);
												do_update = 1;
											}
										}
									}
								}
								if(tag_list.GetCount()) {
									pack.TagL.Merge(tag_list, ObjTagList::mumUpdate|ObjTagList::mumAdd);
									do_update = 1;
								}
								if(AssignEgaisCode(sdr_rec, &pack, logger) > 0)
									do_update = 1;
								if(AssignFlags(sdr_rec, &pack) > 0)
									do_update = 1;
								// @v11.6.10 {
								{
									temp_buf2 = sdr_rec.Brand;
									if(temp_buf2.NotEmptyS()) {
										const  PPID org_brand_id = pack.Rec.BrandID;
										if(!pack.Rec.BrandID || Param.Flags & PPGoodsImpExpParam::fForceUpdateBrand) {
											THROW(BrObj.AddSimple(&pack.Rec.BrandID, temp_buf2, 0, 0));
											if(pack.Rec.BrandID != org_brand_id)
												do_update = 1;
										}
									}
								}
								// } @v11.6.10 
								{
									const  PPID org_manuf_id = pack.Rec.ManufID;
									if(!pack.Rec.ManufID || Param.Flags & PPGoodsImpExpParam::fForceUpdateManuf) {
										THROW(Resolve_Manuf(sdr_rec, &pack.Rec.ManufID));
										if(pack.Rec.ManufID != org_manuf_id)
											do_update = 1;
									}
								}
								if(sdr_rec.MinStock > 0.0) {
									pack.Stock.SetMinStock(Param.LocID, sdr_rec.MinStock);
									do_update = 1;
								}
								if(do_update) {
									if(!GObj.PutPacket(&goods_id, &pack, 0)) {
										PPGetLastErrorMessage(1, err_msg_buf);
										PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
										logger.Log(msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack.Rec.Name, err_msg_buf.cptr()));
									}
								}
								if(do_add) {
									goods_id = 0;
									if(!GObj.PutPacket(&goods_id, &pack2, 0)) {
										PPGetLastErrorMessage(1, err_msg_buf);
										PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
										logger.Log(msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack2.Rec.Name, err_msg_buf.cptr()));
									}
								}
							}
							else if((goods_name.NotEmpty() || try_cls_name) && matrix_action < 1000) {
								goods_name.CopyTo(sdr_rec.Name, sizeof(sdr_rec.Name));
								THROW(CreateGoodsPacket(sdr_rec, barcode, &pack, logger));
								SetupNameExt(pack, goods_name_ext, sdr_rec.ExtLongNameTo);
								if(pack.Rec.Name[0] == 0) {
									PPLoadText(PPTXT_UNDEFIMPGOODSNAME, err_msg_buf);
									PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
									msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack.Rec.Name, err_msg_buf.cptr());
									logger.Log(msg_buf);
								}
								else {
									if(sdr_rec.MinStock > 0.0)
										pack.Stock.SetMinStock(Param.LocID, sdr_rec.MinStock);
									AssignFlags(sdr_rec, &pack);
									if(GObj.PutPacket(&goods_id, &pack, 0)) {
										if(ar_code.NotEmpty()) {
											//
											// Артикулы товара меняем уже после проведения товара
											// для того, чтобы вероятное дублировании артикулов
											// не помешало бы нам сохранить весь пакет товара в целом.
											//
											ArGoodsCodeArray arcode_list;
											ArGoodsCodeTbl::Rec ar_code_rec;
											arcode_list = pack.ArCodes;
											ar_code_rec.GoodsID = goods_id;
											ar_code_rec.ArID = suppl_id; //
											ar_code_rec.Pack = 1000; // 1.0
											ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
											arcode_list.insert(&ar_code_rec);
											if(!GObj.P_Tbl->UpdateArCodes(goods_id, &arcode_list, 0)) {
												PPGetLastErrorMessage(1, err_msg_buf);
												logger.Log(err_msg_buf);
											}
										}
									}
									else {
										int ok_2 = 0;
										if(PPErrCode == PPERR_INVGOODSPARENT && Param.DefParentID) {
											pack.Rec.ParentID = Param.DefParentID;
											if(GObj.PutPacket(&goods_id, &pack, 0))
												ok_2 = 1;
										}
										if(!ok_2) {
											PPGetLastErrorMessage(1, err_msg_buf);
											PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
											msg_buf.Printf(fmt_buf, PPOBJ_GOODS, pack.Rec.Name, err_msg_buf.cptr());
											logger.Log(msg_buf);
										}
									}
								}
							}
						}
						if(goods_id) {
							PPID   qcert_id = 0;
							for(uint j = 0; j < added_code_list.getCount(); j++) {
								const BarcodeTbl::Rec & r_bc_rec = added_code_list.at(j);
								if(r_bc_rec.Code[0] && GObj.P_Tbl->SearchBarcode(r_bc_rec.Code, 0) < 0) {
									THROW(GObj.P_Tbl->AddBarcode(goods_id, r_bc_rec.Code, r_bc_rec.Qtty, 0));
								}
							}
							THROW(PutQCert(&sdr_rec, &qcert_id));
							//
							//
							//
							{
								const double __rest = R6(sdr_rec.Rest);
								PPBillPacket * p_pack = 0;
								if((__rest > 0.0 || (__rest == 0.0 && !(Param.Flags & PPGoodsImpExpParam::fSkipZeroQtty))) && (Param.RcptOpID && suppl_id)) {
									PPTransferItem ti;
									THROW(bill_ident.Get(&sdr_rec, suppl_id));
									THROW(p_pack = bill_ident.GetPacket(Param.RcptOpID, warehouse_id));
									ti.Init(&p_pack->Rec);
									THROW(ti.SetupGoods(goods_id));
									ti.UnitPerPack = sdr_rec.UnitsPerPack;
									ti.Quantity_   = __rest;
									ti.Cost        = R2(fabs(sdr_rec.Cost));
									ti.Price       = R2(fabs(sdr_rec.Price));
									ti.QCert       = qcert_id;
									ti.Expiry      = sdr_rec.Expiry;
									if(ti.QCert || ti.Price != 0.0 || ti.Quantity_ != 0.0 || ti.UnitPerPack != 0.0) {
										THROW(p_pack->InsertRow(&ti, 0));
										(temp_buf2 = sdr_rec.Clb).Strip();
										if(temp_buf2.Len()) {
											temp_buf2.ReplaceChar('\\', '/').ReplaceChar('-', ' ');
											THROW(p_pack->LTagL.SetString(PPTAG_LOT_CLB, p_pack->GetTCount()-1, temp_buf2));
										}
										(temp_buf2 = sdr_rec.Serial).Strip();
										if(temp_buf2.Len()) {
											temp_buf2.ReplaceChar('\\', '/').ReplaceChar('-', ' ');
											THROW(p_pack->LTagL.SetString(PPTAG_LOT_SN, p_pack->GetTCount()-1, temp_buf2));
										}
									}
								}
								else if(__rest < 0.0 && op_for_neg_rest_id) {
									PPTransferItem ti;
									THROW(bill_ident_negop.Get(&sdr_rec, 0));
									THROW(p_pack = bill_ident_negop.GetPacket(op_for_neg_rest_id, warehouse_id));
									ti.Init(&p_pack->Rec);
									THROW(ti.SetupGoods(goods_id));
									ti.Quantity_   = fabs(__rest);
									ti.Cost        = R2(fabs(sdr_rec.Cost));
									ti.Price       = R2(fabs(sdr_rec.Price));
									THROW(p_pack->InsertRow(&ti, 0));
								}
							}
							if(oneof4(matrix_action, 0, 1, 1000, 1001)) {
								PPQuot quot(goods_id);
								quot.Kind = GObj.GetConfig().MtxQkID;
								quot.LocID = warehouse_id;
								quot.Quot = oneof2(matrix_action, 0, 1000) ? 0.0 : 1.0;
								THROW(GObj.P_Tbl->SetQuot(quot, 0));
							}
							if(sdr_rec.AltGrpCode[0]) {
								BarcodeTbl::Rec grp_code_rec;
								Goods2Tbl::Rec grp_rec;
								if(GGObj.SearchCode(sdr_rec.AltGrpCode, &grp_code_rec) > 0) {
									if(GGObj.Fetch(grp_code_rec.GoodsID, &grp_rec) > 0) {
										if(grp_rec.Kind == PPGDSK_GROUP && grp_rec.Flags & GF_ALTGROUP && !(grp_rec.Flags & GF_DYNAMIC)) {
											if(!GGObj.AssignGoodsToAltGrp(goods_id, grp_rec.ID, sdr_rec.AltGrpPLU, 0)) {
												// В случае ошибки выдаем сообщение в журнал и позволяем процессу работать дальше.
												logger.LogLastError();
											}
										}
									}
								}
							}
						}
						PPWaitPercent(cntr.Increment(), wait_msg);
					}
					THROW(bill_ident.FinishPackets());
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPObjGoods::Import(const char * pCfgName, int analyze, int use_ta)
{
	PPGoodsImporter gi;
	return gi.Run(pCfgName, use_ta);
}
//
//
//
static IMPL_CMPFUNC(WordConcordAssoc_ByText, p1, p2)
{
	const LAssoc * p_a1 = static_cast<const LAssoc *>(p1);
	const LAssoc * p_a2 = static_cast<const LAssoc *>(p2);
	PPTextAnalyzer * p_ta = static_cast<PPTextAnalyzer *>(pExtraData);
	if(p_ta) {
		SString & r_temp_buf1 = SLS.AcquireRvlStr();
		SString & r_temp_buf2 = SLS.AcquireRvlStr();
		p_ta->GetTextById(p_a1->Key, r_temp_buf1);
		p_ta->GetTextById(p_a2->Key, r_temp_buf2);
		return r_temp_buf1.CmpNC(r_temp_buf2);
	}
	else
		return 0;
}

static IMPL_CMPFUNC(WordConcordAssoc_ByFreq, p1, p2)
{
	const LAssoc * p_a1 = static_cast<const LAssoc *>(p1);
	const LAssoc * p_a2 = static_cast<const LAssoc *>(p2);
	int si = (p_a1->Val < p_a2->Val) ? -1 : ((p_a1->Val > p_a2->Val) ? +1 : 0);
	if(si == 0) {
		PPTextAnalyzer * p_ta = static_cast<PPTextAnalyzer *>(pExtraData);
		if(p_ta) {
			SString & r_temp_buf1 = SLS.AcquireRvlStr();
			SString & r_temp_buf2 = SLS.AcquireRvlStr();
			p_ta->GetTextById(p_a1->Key, r_temp_buf1);
			p_ta->GetTextById(p_a2->Key, r_temp_buf2);
			si = r_temp_buf1.CmpNC(r_temp_buf2);
		}
	}
	return si;
}

static IMPL_CMPFUNC(BrandConcordAssoc_ByText, p1, p2)
{
	const LAssoc * p_a1 = static_cast<const LAssoc *>(p1);
	const LAssoc * p_a2 = static_cast<const LAssoc *>(p2);
	PPObjBrand * p_brand_obj = static_cast<PPObjBrand *>(pExtraData);
	if(p_brand_obj) {
		SString & r_temp_buf1 = SLS.AcquireRvlStr();
		SString & r_temp_buf2 = SLS.AcquireRvlStr();
		PPBrand brand_rec;
		if(p_a1->Key && p_brand_obj->Fetch(p_a1->Key, &brand_rec) > 0)
			(r_temp_buf1 = brand_rec.Name).Strip();
		if(p_a2->Key && p_brand_obj->Fetch(p_a2->Key, &brand_rec) > 0)
			(r_temp_buf2 = brand_rec.Name).Strip();
		return r_temp_buf1.CmpNC(r_temp_buf2);
	}
	else
		return 0;
}

static IMPL_CMPFUNC(CategoryConcordAssoc_ByText, p1, p2)
{
	const LAssoc * p_a1 = static_cast<const LAssoc *>(p1);
	const LAssoc * p_a2 = static_cast<const LAssoc *>(p2);
	PPObjGoods * p_categ_obj = static_cast<PPObjGoods *>(pExtraData);
	if(p_categ_obj) {
		SString & r_temp_buf1 = SLS.AcquireRvlStr();
		SString & r_temp_buf2 = SLS.AcquireRvlStr();
		Goods2Tbl::Rec group_rec;
		if(p_a1->Key && p_categ_obj->Fetch(p_a1->Key, &group_rec) > 0)
			(r_temp_buf1 = group_rec.Name).Strip();
		if(p_a2->Key && p_categ_obj->Fetch(p_a2->Key, &group_rec) > 0)
			(r_temp_buf2 = group_rec.Name).Strip();
		return r_temp_buf1.CmpNC(r_temp_buf2);
	}
	else
		return 0;
}

int ExportUhttForGitHub()
{
	int    ok = 1;
	struct Stat {
		Stat() : BarcodeCount(0), DistingWordCount(0), BrandCount(0), CategCount(0)
		{
		}
		uint   BarcodeCount;
		uint   DistingWordCount;
		uint   BrandCount;
		uint   CategCount;
	} stat;

	const  uint lines_per_file = 65000;
	uint   line_count = 0;
	uint   line_total = 0;
	uint64 bytes_total = 0;
	uint64 bytes_part = 0;

	SString temp_buf;
	SString text_ident;

	SString goods_name_utf;
	SString goods_name_ascii;
	SString group_name;
	SString brand_name;

	SString out_file_name;
	SString result_barcode;
	SString line_buf;

	LAssocArray brand_concord;
	LAssocArray categ_concord;
	LAssocHashTable word_concord(SMEGABYTE(4));
	PPTextAnalyzer text_analyzer2;
	PPTextAnalyzer::Item text_analyzer_item;
	StrAssocArray goods_grp_hier;

	PPObjGoods goods_obj;
	PPObjBrand brand_obj;
	PPBrand brand_rec;
	GoodsFilt goods_filt;
	goods_filt.Flags &= ~GoodsFilt::fHidePassive;
    GoodsIterator giter(&goods_filt, GoodsIterator::ordByName);
    Goods2Tbl::Rec goods_rec;
    BarcodeArray codes;
	const SString title_line("ID\tUPCEAN\tName\tCategoryID\tCategoryName\tBrandID\tBrandName\n");
	long   file_no = 1;
	(temp_buf = "uhtt_barcode_ref").CatChar('_').CatLongZ(file_no, 4).DotCat("csv");
    PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
    SFile f_out(out_file_name, SFile::mWrite);
	(temp_buf = "uhtt_barcode_ref_all").DotCat("csv");
    PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
	SFile f_out_all(out_file_name, SFile::mWrite);
	f_out.WriteLine(title_line);
	f_out_all.WriteLine(title_line);
	PPWaitStart();
    while(giter.Next(&goods_rec) > 0) {
		int is_there_valid_codes = 0;
		uint i;
		goods_obj.P_Tbl->ReadBarcodes(goods_rec.ID, codes);
		for(i = 0; !is_there_valid_codes && i < codes.getCount(); i++) {
			temp_buf = codes.at(i).Code;
			int    diag = 0, std = 0;
			const  int  dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
			if(dbr > 0)
				is_there_valid_codes = 1;
		}
		if(is_there_valid_codes) {
			goods_name_utf = goods_rec.Name;
			goods_name_utf.ReplaceChar('\t', ' ');
			(goods_name_ascii = goods_name_utf).ToLower().Transf(CTRANSF_INNER_TO_OUTER);
			goods_name_utf.Transf(CTRANSF_INNER_TO_UTF8);
			goods_obj.GetHierarchy(goods_rec.ID, &goods_grp_hier);
			group_name.Z();
			if(goods_grp_hier.getCount() > 1) {
				for(uint j = 0; j < (goods_grp_hier.getCount()-1); j++)	{
					StrAssocArray::Item item = goods_grp_hier.at_WithoutParent(j);
					group_name.CatDivIfNotEmpty('/', 0);
					group_name.Cat(item.Txt);
				}
				group_name.Transf(CTRANSF_INNER_TO_UTF8);
			}
			categ_concord.IncrementValueByKey(goods_rec.ParentID);
			group_name.ReplaceChar('\t', ' ');
			brand_name.Z();
			if(goods_rec.BrandID && brand_obj.Fetch(goods_rec.BrandID, &brand_rec) > 0) {
				(brand_name = brand_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				brand_name.ReplaceChar('\t', ' ');
				brand_concord.IncrementValueByKey(goods_rec.BrandID);
			}
			{
				uint   j;
				uint   idx_first = 0, idx_count = 0;
				text_ident.Z().CatChar('#').Cat(goods_rec.ID);
				text_analyzer2.Reset(0);
				text_analyzer2.Write(text_ident, 0, goods_name_ascii, goods_name_ascii.Len()+1);
				text_analyzer2.Run(&idx_first, &idx_count);
				for(j = 0; j < idx_count; j++) {
					if(text_analyzer2.Get(idx_first+j, text_analyzer_item) && text_analyzer_item.Token == STokenizer::tokWord && text_analyzer_item.Text.NotEmpty()) {
						long cc = 0;
						if(word_concord.Search(static_cast<long>(text_analyzer_item.TextId), &cc))
							word_concord.Set(static_cast<long>(text_analyzer_item.TextId), cc+1); 
						else
							word_concord.Set(static_cast<long>(text_analyzer_item.TextId), 1); 
					}
				}
			}
			for(i = 0; i < codes.getCount(); i++) {
				temp_buf = codes.at(i).Code;
				int    diag = 0, std = 0;
				const  int  dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
				if(dbr > 0) {
					line_buf.Z().Cat(goods_rec.ID).Tab().Cat(result_barcode).Tab().Cat(goods_name_utf).Tab().Cat(goods_rec.ParentID).Tab().
						Cat(group_name).Tab().Cat(goods_rec.BrandID).Tab().Cat(brand_name).CR();
					//assert(line_buf.IsLegalUtf8()); // @debug
					f_out.WriteLine(line_buf);
					f_out_all.WriteLine(line_buf);
					bytes_total += line_buf.Len();
					bytes_part  += line_buf.Len();
					line_count++;
					line_total++;
					stat.BarcodeCount++;
					//if(line_count >= lines_per_file) {
					if(bytes_part >= SKILOBYTE(1024-1)) {
						f_out.Close();
						file_no++;
						(temp_buf = "uhtt_barcode_ref").CatChar('_').CatLongZ(file_no, 4).DotCat("csv");
						PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
						f_out.Open(out_file_name, SFile::mWrite);
						f_out.WriteLine(title_line);
						line_count = 0;
						bytes_part = 0;
					}
				}
			}
		}
		PPWaitPercent(giter.GetIterCounter());
    }
	if(brand_concord.getCount()) {
		brand_concord.sort(PTR_CMPFUNC(CategoryConcordAssoc_ByText), &brand_obj);
		(temp_buf = "uhtt_barcode_ref_brand_concord").DotCat("csv");
		PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
		SFile f_out_brand(out_file_name, SFile::mWrite);
		line_buf.Z().Cat("BrandID").Tab().Cat("BrandName").Tab().Cat("Count").CR();
		f_out_brand.WriteLine(line_buf);
		for(uint i = 0; i < brand_concord.getCount(); i++) {
			const  PPID brand_id = brand_concord.at(i).Key;
			if(brand_id && brand_obj.Fetch(brand_id, &brand_rec) > 0) {
				(brand_name = brand_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				brand_name.ReplaceChar('\t', ' ');
				line_buf.Z().Cat(brand_id).Tab().Cat(brand_name).Tab().Cat(brand_concord.at(i).Val).CR();
				f_out_brand.WriteLine(line_buf);
				stat.BrandCount++;
			}
		}
	}
	if(categ_concord.getCount()) {
		categ_concord.sort(PTR_CMPFUNC(CategoryConcordAssoc_ByText), &goods_obj);
		(temp_buf = "uhtt_barcode_ref_category_concord").DotCat("csv");
		PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
		SFile f_out_categ(out_file_name, SFile::mWrite);
		line_buf.Z().Cat("CategoryID").Tab().Cat("CategoryName").Tab().Cat("Count").CR();
		f_out_categ.WriteLine(line_buf);
		for(uint i = 0; i < categ_concord.getCount(); i++) {
			const  PPID categ_id = categ_concord.at(i).Key;
			Goods2Tbl::Rec group_rec;
			if(categ_id && goods_obj.Fetch(categ_id, &group_rec) > 0) {
				(group_name = group_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				group_name.ReplaceChar('\t', ' ');
				line_buf.Z().Cat(categ_id).Tab().Cat(group_name).Tab().Cat(categ_concord.at(i).Val).CR();
				f_out_categ.WriteLine(line_buf);
				stat.CategCount++;
			}
		}
	}
	{
		LAssocArray word_concord_assoc_list;
		{
			HashTableBase::Iter wciter;
			long   key = 0;
			long   val = 0;
			for(word_concord.InitIteration(&wciter); word_concord.NextIteration(&wciter, &key, &val) > 0;) {
				word_concord_assoc_list.Add(key, val);
			}
		}
		{
			(temp_buf = "uhtt_barcode_ref_word_concord_bytext").DotCat("csv");
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out_word(out_file_name, SFile::mWrite);
			line_buf.Z().Cat("Word").Tab().Cat("Count").CR();
			f_out_word.WriteLine(line_buf);
			word_concord_assoc_list.sort(PTR_CMPFUNC(WordConcordAssoc_ByText), &text_analyzer2);
			for(uint i = 0; i < word_concord_assoc_list.getCount(); i++) {
				const LAssoc & r_item = word_concord_assoc_list.at(i);
				const uint text_id = static_cast<uint>(r_item.Key);
				text_analyzer2.GetTextById(text_id, temp_buf);
				temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
				line_buf.Z().Cat(temp_buf).Tab().Cat(r_item.Val).CR();
				f_out_word.WriteLine(line_buf);
				stat.DistingWordCount++;
			}
		}
		{
			(temp_buf = "uhtt_barcode_ref_word_concord_byfreq").DotCat("csv");
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out_word(out_file_name, SFile::mWrite);
			line_buf.Z().Cat("Word").Tab().Cat("Count").CR();
			f_out_word.WriteLine(line_buf);
			//word_concord_assoc_list.SortByVal();
			word_concord_assoc_list.sort(PTR_CMPFUNC(WordConcordAssoc_ByFreq), &text_analyzer2);
			for(uint i = 0; i < word_concord_assoc_list.getCount(); i++) {
				const LAssoc & r_item = word_concord_assoc_list.at(i);
				const uint text_id = static_cast<uint>(r_item.Key);
				text_analyzer2.GetTextById(text_id, temp_buf);
				temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
				line_buf.Z().Cat(temp_buf).Tab().Cat(r_item.Val).CR();
				f_out_word.WriteLine(line_buf);
			}
		}
		/*for(word_concord.InitIteration(&wciter); word_concord.NextIteration(&wciter, &key, &val) > 0;) {
			const uint text_id = (uint)key;
			text_analyzer2.GetTextById(text_id, temp_buf);
			temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
			line_buf.Z().Cat(temp_buf).Tab().Cat(val).CR();
			f_out_word.WriteLine(line_buf);
		}*/
		{
			(temp_buf = "uhtt_barcode_ref_stat").DotCat("txt");
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
			SFile f_out_stat(out_file_name, SFile::mWrite);
			f_out_stat.WriteLine(line_buf.Z().CatEq("barcode-count", stat.BarcodeCount).CR());
			f_out_stat.WriteLine(line_buf.Z().CatEq("brand-count", stat.BrandCount).CR());
			f_out_stat.WriteLine(line_buf.Z().CatEq("categ-count", stat.CategCount).CR());
			f_out_stat.WriteLine(line_buf.Z().CatEq("word-count", stat.DistingWordCount).CR());
			f_out_stat.WriteLine(line_buf.Z().CatEq("barcode-ref-bytes-count", bytes_total).CR());
		}
	}
	PPWaitStop();
	return ok;
}

int ReformatIceCat(const char * pFileName)
{
	int    ok = 1;
	SString line_buf;
	SString temp_buf;
	SFile f_in(pFileName, SFile::mRead);
	if(f_in.IsValid()) {
		SFsPath ps(pFileName);
		ps.Nam.CatChar('-').Cat("out");
		ps.Merge(temp_buf);
		StringSet ss("\t\t\t");
		StringSet ean_ss(";");
		SString part_num;
		SString brand;
		SString category;
		SString model;
		SString ean;
		SString family;
		SString title;
		SString compose_buf;
		uint   line_no = 0;
		SFile f_out(temp_buf, SFile::mWrite);
		while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			line_no++;
			if(line_no > 1) {
				ss.setBuf(line_buf);
				// PartNumber; Brand; Quality; Category; ModelName; EAN; MarketPresence; Family; Title; 
				line_buf.Z();
				for(uint ssp = 0, fld_no = 0; ss.get(&ssp, temp_buf); fld_no++) {
					temp_buf.Strip();
					switch(fld_no) {
						case 0:	part_num = temp_buf; break;
						case 1:	brand = temp_buf; break;
						//case 2: break; // quality
						case 3:	category = temp_buf; break;
						case 4:	model = temp_buf; break;
						case 5:	ean = temp_buf; break;
						// case 6: break; // marketpresence
						case 7:	family = temp_buf; break;
						case 8:	title = temp_buf; break;
					}
				}
				if(ean.NotEmpty()) {
					ean_ss.setBuf(ean);
					for(uint ep = 0; ean_ss.get(&ep, temp_buf);) {
						if(temp_buf.NotEmptyS()) {
							int    diag = 0, std = 0;
							int    r = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, 0);
							if(r > 0) {
								int    skip = 0;
								if(title.HasPrefixIAscii("ISBN")) {
									(compose_buf = "ISBN").Space().Cat(temp_buf);
									if(title.HasPrefixNC(compose_buf))
										skip = 1;
								}
								if(!skip) {
									line_buf.Z().Cat(temp_buf).Tab().Cat(title).Tab().Cat(brand).Tab().Cat(category).Tab().Cat(family).Tab().Cat(model).CR();
									f_out.WriteLine(line_buf);
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}
//
//
//
IntermediateImportedGoodsCollection::InnerEntry::InnerEntry()
{
	THISZERO();
}

IntermediateImportedGoodsCollection::Entry::Entry() : Ident(0), ProcessFlags(0)
{
}
		
IntermediateImportedGoodsCollection::Entry & IntermediateImportedGoodsCollection::Entry::Z()
{
	Ident = 0;
	ProcessFlags = 0;
	Name.Z();
	Code.Z();
	GrpName.Z();
	BrandName.Z();
	return *this;
}

static IMPL_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByName, i1, i2)
{
	int    r = 0;
	if(pExtraData) {
		IntermediateImportedGoodsCollection * p_this = static_cast<IntermediateImportedGoodsCollection *>(pExtraData);
		const IntermediateImportedGoodsCollection::InnerEntry * p1 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i1);
		const IntermediateImportedGoodsCollection::InnerEntry * p2 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i2);
		SStringU & r_textu1 = SLS.AcquireRvlStrU();
		SStringU & r_textu2 = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		p_this->GetS(p1->NameP, r_temp_buf);
		r_textu1.CopyFromUtf8(r_temp_buf);
		p_this->GetS(p2->NameP, r_temp_buf);
		r_textu2.CopyFromUtf8(r_temp_buf);
		r = r_textu1.Cmp(r_textu2);
	}
	return r;
}
	
static IMPL_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByGroup, i1, i2)
{
	int    r = 0;
	if(pExtraData) {
		IntermediateImportedGoodsCollection * p_this = static_cast<IntermediateImportedGoodsCollection *>(pExtraData);
		const IntermediateImportedGoodsCollection::InnerEntry * p1 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i1);
		const IntermediateImportedGoodsCollection::InnerEntry * p2 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i2);
		SStringU & r_textu1 = SLS.AcquireRvlStrU();
		SStringU & r_textu2 = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		p_this->Ht.Get(p1->GrpNameP, r_temp_buf);
		r_textu1.CopyFromUtf8(r_temp_buf);
		p_this->Ht.Get(p2->GrpNameP, r_temp_buf);
		r_textu2.CopyFromUtf8(r_temp_buf);
		r = r_textu1.Cmp(r_textu2);
		if(r == 0) {
			p_this->GetS(p1->NameP, r_temp_buf);
			r_textu1.CopyFromUtf8(r_temp_buf);
			p_this->GetS(p2->NameP, r_temp_buf);
			r_textu2.CopyFromUtf8(r_temp_buf);
			r = r_textu1.Cmp(r_textu2);
		}
	}
	return r;
}
	
static IMPL_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByBrand, i1, i2)
{
	int    r = 0;
	if(pExtraData) {
		IntermediateImportedGoodsCollection * p_this = static_cast<IntermediateImportedGoodsCollection *>(pExtraData);
		const IntermediateImportedGoodsCollection::InnerEntry * p1 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i1);
		const IntermediateImportedGoodsCollection::InnerEntry * p2 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i2);
		SStringU & r_textu1 = SLS.AcquireRvlStrU();
		SStringU & r_textu2 = SLS.AcquireRvlStrU();
		SString & r_temp_buf = SLS.AcquireRvlStr();
		p_this->Ht.Get(p1->BrandNameP, r_temp_buf);
		r_textu1.CopyFromUtf8(r_temp_buf);
		p_this->Ht.Get(p2->BrandNameP, r_temp_buf);
		r_textu2.CopyFromUtf8(r_temp_buf);
		r = r_textu1.Cmp(r_textu2);
		if(r == 0) {
			p_this->GetS(p1->NameP, r_temp_buf);
			r_textu1.CopyFromUtf8(r_temp_buf);
			p_this->GetS(p2->NameP, r_temp_buf);
			r_textu2.CopyFromUtf8(r_temp_buf);
			r = r_textu1.Cmp(r_textu2);
		}
	}
	return r;
}
	
static IMPL_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByCode, i1, i2)
{
	int    r = 0;
	if(pExtraData) {
		IntermediateImportedGoodsCollection * p_this = static_cast<IntermediateImportedGoodsCollection *>(pExtraData);
		const IntermediateImportedGoodsCollection::InnerEntry * p1 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i1);
		const IntermediateImportedGoodsCollection::InnerEntry * p2 = static_cast<const IntermediateImportedGoodsCollection::InnerEntry *>(i2);
		r = strcmp(p1->Code, p2->Code);
		if(r == 0) {
			SStringU & r_textu1 = SLS.AcquireRvlStrU();
			SStringU & r_textu2 = SLS.AcquireRvlStrU();
			SString & r_temp_buf = SLS.AcquireRvlStr();
			p_this->GetS(p1->NameP, r_temp_buf);
			r_textu1.CopyFromUtf8(r_temp_buf);
			p_this->GetS(p2->NameP, r_temp_buf);
			r_textu2.CopyFromUtf8(r_temp_buf);
			r = r_textu1.Cmp(r_textu2);
		}
	}
	return r;
}

IntermediateImportedGoodsCollection::IntermediateImportedGoodsCollection() : Ht(20 * 1024, 0)
{
}
	
uint  IntermediateImportedGoodsCollection::GetCount() const { return L.getCount(); }
const LongArray & IntermediateImportedGoodsCollection::GetGroupList() { return GroupList; }
const LongArray & IntermediateImportedGoodsCollection::GetBrandList() { return BrandList; }
int   IntermediateImportedGoodsCollection::GetTextById(long id, SString & rBuf) const { return Ht.GetByAssoc(id, rBuf); }

int  IntermediateImportedGoodsCollection::Get(uint idx, Entry & rEntry) const
{
	rEntry.Z();
	int    ok = 1;
	if(idx < L.getCount()) {
		const InnerEntry & r_entry = L.at(idx);
		SStrGroup::GetS(r_entry.NameP, rEntry.Name);
		rEntry.Ident = r_entry.Ident; // @v11.5.11
		rEntry.ProcessFlags = r_entry.ProcessFlags; // @v11.5.11
		rEntry.Code = r_entry.Code;
		if(r_entry.GrpNameP)
			Ht.Get(r_entry.GrpNameP, rEntry.GrpName);
		if(r_entry.BrandNameP)
			Ht.Get(r_entry.BrandNameP, rEntry.BrandName);
	}
	else
		ok = 0;
	return ok;
}

int IntermediateImportedGoodsCollection::Add(const Entry & rEntry)
{
	int    ok = 1;
	if(rEntry.Name.NotEmpty() && rEntry.Code.NotEmpty()) {
		SString temp_buf;
		(temp_buf = rEntry.Name).Utf8ToLower();
		temp_buf.ElimDblSpaces();
		InnerEntry new_entry;
		SStrGroup::AddS(temp_buf, &new_entry.NameP);
		//
		temp_buf = rEntry.GrpName;
		temp_buf.ElimDblSpaces();
		if(temp_buf.NotEmptyS()) {
			temp_buf.Utf8ToLower();
			uint   v = 0;
			uint   p = 0;
			if(Ht.Search(temp_buf, &v, &p)) {
				new_entry.GrpNameP = p;
			}
			else {
				LastId++;
				THROW_SL(Ht.Add(temp_buf, LastId, &p));
				GroupList.add(LastId);
				new_entry.GrpNameP = p;
			}
		}
		//
		temp_buf = rEntry.BrandName;
		temp_buf.ElimDblSpaces();
		if(temp_buf.NotEmptyS()) {
			temp_buf.Utf8ToLower();
			uint   v = 0;
			uint   p = 0;
			if(Ht.Search(temp_buf, &v, &p)) {
				new_entry.BrandNameP = p;
			}
			else {
				LastId++;
				THROW_SL(Ht.Add(temp_buf, LastId, &p));
				BrandList.add(LastId);
				new_entry.BrandNameP = p;
			}
		}
		STRNSCPY(new_entry.Code, rEntry.Code);
		new_entry.Ident = rEntry.Ident;
		THROW_SL(L.insert(&new_entry));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
	
void IntermediateImportedGoodsCollection::UpdateProcessFlags(uint idx, uint flags)
{
	if(idx < L.getCount()) {
		InnerEntry & r_entry = L.at(idx);
		r_entry.ProcessFlags = flags;
	}
}
	
int IntermediateImportedGoodsCollection::UpdateCategory(uint idx, const SString & rText)
{
	int    ok = 1;
	if(idx < L.getCount() && rText.NotEmpty()) {
		InnerEntry & r_entry = L.at(idx);
		uint   v = 0;
		uint   p = 0;
		if(Ht.Search(rText, &v, &p)) {
			r_entry.GrpNameP = p;
		}
		else {
			LastId++;
			THROW_SL(Ht.Add(rText, LastId, &p));
			GroupList.add(LastId);
			r_entry.GrpNameP = p;
		}
	}
	CATCHZOK
	return ok;
}

int IntermediateImportedGoodsCollection::UpdateBrand(uint idx, const SString & rText)
{
	int    ok = 1;
	if(idx < L.getCount() && rText.NotEmpty()) {
		InnerEntry & r_entry = L.at(idx);
		uint   v = 0;
		uint   p = 0;
		if(Ht.Search(rText, &v, &p)) {
			r_entry.BrandNameP = p;
		}
		else {
			LastId++;
			THROW_SL(Ht.Add(rText, LastId, &p));
			BrandList.add(LastId);
			r_entry.BrandNameP = p;
		}
	}
	CATCHZOK
	return ok;
}
	
void IntermediateImportedGoodsCollection::FinalizeImport()
{
	GroupList.sortAndUndup();
	BrandList.sortAndUndup();
	Ht.BuildAssoc();
}

void IntermediateImportedGoodsCollection::Sort(int ord)
{
	switch(ord) {
		case ordByName: L.sort(PTR_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByName), this); break;
		case ordByGroup: L.sort(PTR_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByGroup), this); break;
		case ordByBrand: L.sort(PTR_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByBrand), this); break;
		case ordByCode: L.sort(PTR_CMPFUNC(IntermediateImportedGoodsCollection_Entry_ByCode), this); break;
	}
}
//
//
//
UhttGoodsProcessor::UhttGoodsProcessor()
{
}

int UhttGoodsProcessor::Init()
{
	return 0; // @construction
}
	
int UhttGoodsProcessor::AddEntry(const char * pCode, const char * pName, const char * pCategory, const char * pBrand)
{
	int    ok = -1;
	if(!isempty(pCode) && !isempty(pName)) {
		IntermediateImportedGoodsCollection::Entry entry;
		entry.Code = pCode;
		entry.Name = pName;
		if(entry.Code.NotEmptyS() && entry.Name.NotEmptyS()) {
			(entry.GrpName = pCategory).Strip();
			(entry.BrandName = pBrand).Strip();
			ok = L.Add(entry);
		}
	}
	return ok;
}

void UhttGoodsProcessor::ProcessDupCodeList(const LongArray & rDupCodeIdxList)
{
	IntermediateImportedGoodsCollection::Entry entry;
	IntermediateImportedGoodsCollection::Entry entry2;
	SString name;
	SString name2;
	SString temp_buf;
	const uint _c = rDupCodeIdxList.getCount();
	StringSet ss_groups;
	StringSet ss_brands;
	{
		for(uint i = 0; i < _c; i++) {
			const uint idx = rDupCodeIdxList.get(i)-1;
			assert(idx < L.GetCount());
			L.Get(idx, entry);
			if(entry.GrpName.NotEmpty())
				ss_groups.add(entry.GrpName);
			if(entry.BrandName.NotEmpty())
				ss_brands.add(entry.BrandName);
		}
		ss_groups.sortAndUndup();
		ss_brands.sortAndUndup();
	}
	{
		SString upd_grp_name;
		SString upd_brand_name;
		for(uint i = 0; i < _c; i++) {
			const uint idx = rDupCodeIdxList.get(i)-1;
			assert(idx < L.GetCount());
			L.Get(idx, entry);
			upd_grp_name.Z();
			upd_brand_name.Z();
			(name = entry.Name).Utf8ToLower();
			uint   process_flags = entry.ProcessFlags;
			if(entry.GrpName.IsEmpty()) {
				if(ss_groups.getCount()) {
					ss_groups.get(0U, upd_grp_name);
					process_flags |= IntermediateImportedGoodsCollection::Entry::fUpdCategory;
				}
			}
			if(entry.BrandName.IsEmpty()) {
				if(ss_brands.getCount()) {
					ss_brands.get(0U, upd_brand_name);
					process_flags |= IntermediateImportedGoodsCollection::Entry::fUpdBrand;
				}
			}
			for(uint j = 0; j < _c; j++) {
				if(j != i) {
					const uint idx2 = rDupCodeIdxList.get(j)-1;
					assert(idx2 < L.GetCount());
					L.Get(idx2, entry2);
					if(!(entry2.ProcessFlags & IntermediateImportedGoodsCollection::Entry::fRemove)) {
						(name2 = entry2.Name).Utf8ToLower();
						if(name2.Search(name, 0, 0, 0)) {
							// Имя элемента idx полностью содержится в имени другого элемента: стало быть элемент[idx] удаляем
							process_flags |= IntermediateImportedGoodsCollection::Entry::fRemove;
						}
					}
				}
			}
			if(process_flags != entry.ProcessFlags) {
				IntermediateImportedGoodsCollection::Entry result_entry;
				result_entry.Ident = entry.Ident;
				result_entry.Code = entry.Code;
				result_entry.ProcessFlags = process_flags;
				if(upd_grp_name.NotEmpty())
					result_entry.GrpName = upd_grp_name;
				else
					result_entry.GrpName = entry.GrpName;
				if(upd_brand_name.NotEmpty())
					result_entry.BrandName = upd_brand_name;
				else
					result_entry.BrandName = entry.BrandName;
				result_entry.Name = entry.Name;
				Result.Add(result_entry);
				//L.UpdateProcessFlags(idx, process_flags);
			}
		}
	}
}

uint UhttGoodsProcessor::GetResultCount() const
{
	return Result.GetCount();
}

int UhttGoodsProcessor::GetResult(uint idx, IntermediateImportedGoodsCollection::Entry & rEntry)
{
	int    ok = 1;
	if(idx < Result.GetCount()) {
		Result.Get(idx, rEntry);
	}
	else
		ok = 0;
	return ok;
}
	
int UhttGoodsProcessor::Run()
{
	int    ok = 1;
	L.FinalizeImport();
	if(L.GetCount()) {
		L.Sort(IntermediateImportedGoodsCollection::ordByCode);
		LongArray dup_code_idx_list;
		SString prev_code;
		SString code_pattern_buf;
		SString new_name_buf;
		TSCollection <IntermediateImportedGoodsCollection::Entry> dup_code_entry_list;
		IntermediateImportedGoodsCollection::Entry entry;
		{
			for(uint i = 0; i < L.GetCount(); i++) {
				L.Get(i, entry);
				//
				// Если в конце имени содержится '{штрихкод}' или '({штрихкод})' при этом имя не эквивалентно {штрихкод} (то есть, там еще что-то есть кроме кода),
				// то убираем {штрихкод} из наименования.
				//
				uint   code_pos = 0;
				code_pattern_buf = entry.Code;
				if(entry.Name.CmpSuffix(code_pattern_buf, 0)) {
					(new_name_buf = entry.Name).Trim(code_pattern_buf.Len()).Strip();
				}
				else {
					code_pattern_buf.Z().CatParStr(entry.Code);
					if(entry.Name.CmpSuffix(code_pattern_buf, 0)) {
						(new_name_buf = entry.Name).Trim(code_pattern_buf.Len()).Strip();
					}
				}
				if(new_name_buf.NotEmpty()) {

				}
				if(i > 0) {
					if(entry.Code == prev_code) {
						if(dup_code_idx_list.getCount() == 0)
							dup_code_idx_list.add((i-1)+1);
						dup_code_idx_list.add(i+1);
					}
					else {
						if(dup_code_idx_list.getCount()) {
							assert(dup_code_idx_list.getCount() > 1);
							ProcessDupCodeList(dup_code_idx_list);
						}
					}
				}
				//
				prev_code = entry.Code;
			}
			if(dup_code_idx_list.getCount()) {
				assert(dup_code_idx_list.getCount() > 1);
				ProcessDupCodeList(dup_code_idx_list);
			}
		}
	}
	return ok;
}

int ReformatRazoomnick(const char * pFileName)
{
	int    ok = 1;
	SString line_buf;
	SString temp_buf;
	SFile f_in(pFileName, SFile::mRead);
	uint   bad_lines_count = 0;
	if(f_in.IsValid()) {
		PPTextAnalyzerWrapper ta_category;
		StringSet ss;
		StringSet ean_ss(",");
		SString final_barcode;
		StringSet final_barcode_list;
		SString ident;
		SString brand;
		SString category;
		SString name;
		SString ean_list;
		SString article_text;
		SString compose_buf;
		IntermediateImportedGoodsCollection::Entry entry;
		uint   line_no = 0;
		IntermediateImportedGoodsCollection list;
		SFile::ReadLineCsvContext csv_ctx(';');
		PPGetFilePath(PPPATH_DD, "replacer-rule-goodsgroup.sr", temp_buf);
		THROW(ta_category.Init(temp_buf, 0));
		while(f_in.ReadLineCsv(csv_ctx, ss)) {
			line_no++;
			if(line_no > 1) {
				// Id;Category;Vendor;Name;Article;Barcode
				const uint _fc = ss.getCount();
				if(_fc != 6) {
					bad_lines_count++;
				}
				else {
					for(uint ssp = 0, fld_no = 0; ss.get(&ssp, temp_buf); fld_no++) {
						temp_buf.Strip();
						switch(fld_no) {
							case 0:	ident = temp_buf; break;
							case 1:	category = temp_buf; break;
							case 2:	brand = temp_buf; break;
							case 3:	name = temp_buf; break;
							case 4:	article_text = temp_buf; break;
							case 5:	ean_list = temp_buf; break;
						}
					}
					if(name.NotEmptyS() && ean_list.NotEmpty()) {
						{
							category.Transf(CTRANSF_UTF8_TO_OUTER);
							ta_category.ReplaceString(category, temp_buf);
							(category = temp_buf).Transf(CTRANSF_OUTER_TO_UTF8);
						}
						name.StripQuotes();
						final_barcode_list.Z();
						ean_ss.setBuf(ean_list);
						for(uint ep = 0; ean_ss.get(&ep, temp_buf);) {
							if(temp_buf.NotEmptyS()) {
								int    diag = 0, std = 0;
								int    r = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &final_barcode);
								if(r > 0) {
									entry.Z();
									entry.Name = name.ReplaceChar(';', ',');
									entry.GrpName = category.ReplaceChar(';', ',');
									entry.BrandName = brand.ReplaceChar(';', ',');
									entry.Code = final_barcode;
									list.Add(entry);

									final_barcode_list.add(final_barcode);
								}
							}
						}
					}
				}
			}
		}
		list.FinalizeImport();
		{
			{
				SFsPath ps(pFileName);
				ps.Nam.CatChar('-').Cat("out-01");
				ps.Merge(temp_buf);
			}
			SFile f_out_01(temp_buf, SFile::mWrite);
			{
				SFsPath ps(pFileName);
				ps.Nam.CatChar('-').Cat("out-02");
				ps.Merge(temp_buf);
			}
			SFile f_out_02(temp_buf, SFile::mWrite);
			list.Sort(list.ordByGroup);
			for(uint i = 0; i < list.GetCount(); i++) {
				if(list.Get(i, entry)) {
					line_buf.Z().Cat(entry.GrpName).Semicol().Cat(entry.BrandName).Semicol().Cat(entry.Name).Semicol().Cat(entry.Code).CR();
					if(entry.GrpName.NotEmpty()) {
						f_out_01.WriteLine(line_buf);
					}
					else {
						f_out_02.WriteLine(line_buf);
					}
				}
			}
		}
		{
			SFsPath ps(pFileName);
			ps.Nam.CatChar('-').Cat("category");
			ps.Merge(temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);			
			const LongArray & r_id_list = list.GetGroupList();
			for(uint i = 0; i < r_id_list.getCount(); i++) {
				if(list.GetTextById(r_id_list.get(i), temp_buf)) {
					f_out.WriteLine(temp_buf.CR());
				}
			}
		}
		{
			SFsPath ps(pFileName);
			ps.Nam.CatChar('-').Cat("brand");
			ps.Merge(temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);			
			const LongArray & r_id_list = list.GetBrandList();
			for(uint i = 0; i < r_id_list.getCount(); i++) {
				if(list.GetTextById(r_id_list.get(i), temp_buf)) {
					f_out.WriteLine(temp_buf.CR());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}