// CLIBNK2.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
// Модуль формирования данных для передачи в системы клиент-банк
//
#include <pp.h>
#pragma hdrstop
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(CLIBNKDATA, PPCliBnkImpExpParam);

PPCliBnkImpExpParam::PPCliBnkImpExpParam(uint recId, long flags) : PPImpExpParam(NZOR(recId, PPREC_CLIBNKDATA), flags), DefPayerByAmtSign(0)
{
}

/*virtual*/int PPCliBnkImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   flags = 0;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(BnkCode.NotEmpty())
			param_list.Add(PPCLBNKPAR_BANKCODE, temp_buf = BnkCode);
		if(PaymMethodTransl.NotEmpty())
			param_list.Add(PPCLBNKPAR_PAYMMETHODTRANSL, (temp_buf = PaymMethodTransl));
		SETFLAG(flags, 0x0001, DefPayerByAmtSign);
		if(flags)
			param_list.Add(PPCLBNKPAR_FLAGS, temp_buf.Z().Cat(flags));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		BnkCode.Z();
		PaymMethodTransl.Z();
		DefPayerByAmtSign = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case PPCLBNKPAR_BANKCODE: BnkCode = temp_buf; break;
				case PPCLBNKPAR_PAYMMETHODTRANSL: PaymMethodTransl = temp_buf; break;
				case PPCLBNKPAR_FLAGS: 
					flags = temp_buf.ToLong();
					DefPayerByAmtSign = BIN(flags & 0x0001);
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPCliBnkImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name;
	THROW(PPLoadText(PPTXT_CLIENTBANKPARAMS, params));
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	if(BnkCode.NotEmpty()) {
		PPGetSubStr(params, PPCLBNKPAR_BANKCODE, fld_name);
		pFile->AppendParam(pSect, fld_name, BnkCode, 1);
	}
	if(PaymMethodTransl.NotEmpty()) {
		PPGetSubStr(params, PPCLBNKPAR_PAYMMETHODTRANSL, fld_name);
		pFile->AppendParam(pSect, fld_name, PaymMethodTransl, 1);
	}
	SETFLAG(flags, 0x0001, DefPayerByAmtSign);
	PPGetSubStr(params, PPCLBNKPAR_FLAGS, fld_name);
	params.Z().Cat(flags);
	pFile->AppendParam(pSect, fld_name, params, 1);
	CATCHZOK
	return ok;
}

int PPCliBnkImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	BnkCode.Z();
	PaymMethodTransl.Z();
	THROW(PPLoadText(PPTXT_CLIENTBANKPARAMS, params));
	if(PPGetSubStr(params, PPCLBNKPAR_BANKCODE, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			BnkCode = param_val;
	}
	if(PPGetSubStr(params, PPCLBNKPAR_PAYMMETHODTRANSL, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			PaymMethodTransl = param_val;
	}
	DefPayerByAmtSign = 0;
	if(PPGetSubStr(params, PPCLBNKPAR_FLAGS, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0) {
			long   flags = param_val.ToLong();
			DefPayerByAmtSign = BIN(flags & 0x0001);
		}
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}
//
// ARG(kind IN):
//   0 - all, 1 - export, 2 - import
// ARG(pParam   OUT): @#{vptr0}
// ARG(maxBackup IN): Максимальное количество копий, которые необходимо оставить при резервном
//   копировании INI-файла. 0 - не делать копию.
//
int FASTCALL GetCliBnkSections(StringSet * pSectNames, int kind, PPCliBnkImpExpParam * pParam, uint maxBackup, PPLogger * pLogger)
{
	int    ok = 1;
	SString ini_file_name, section, all_fields_name, temp_buf;
	StringSet all_sections;
	PPCliBnkImpExpParam param;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_CLIBNK_INI, ini_file_name));
	THROW_SL(fileExists(ini_file_name));
	THROW(PPLoadText(PPTXT_CLIBNK_SECTION_WITHNAMES, all_fields_name));
	THROW(LoadSdRecord(PPREC_CLIBNKDATA, &param.InrRec));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(ini_file.IsValid());
		if(maxBackup)
			ini_file.Backup(maxBackup);
		THROW(ini_file.GetSections(&all_sections));
		for(uint p = 0; all_sections.get(&p, section);)
			if(!section.IsEqiAscii(all_fields_name)) {
				param.OtrRec.Clear();
				param.HdrOtrRec.Clear(); // @v10.1.11 @fix
				if(param.ReadIni(&ini_file, section, 0)) {
					if((kind == 1 && param.Direction == 0) || (kind == 2 && param.Direction == 1) || kind == 0)
						pSectNames->add(section);
				}
				else {
					if(pLogger)
						pLogger->LogLastError();
					else
						PPError();
				}
			}
	}
	ASSIGN_PTR(pParam, param);
	CATCHZOKPPERR
	return ok;
}
//
// Descr: Элемент ассоциации строки банковской выписки с видом операции.
//   Вид операции идентифицируется по таблице статей, к которой относится контрагент операции
//   и по знаку суммы операции.
//
struct BankStmntAssocItem {  // @persistent @store(PropertyTbl)[as item of array] @flat
	PPID   AccSheetID; //
	int16  Sign;       // -1, 1, 0 (undefined)
	int16  AddedTag;   // Дополнительный тег для установки соотвествия вида операции
	PPID   OpID;       //
};
//
//
//
struct BankStmntItem : public Sdr_CliBnkData { // @flat
	BankStmntItem() : Sdr_CliBnkData(), WeArePayer(0), PayerPersonID(0), ReceiverPersonID(0)
	{
		PTR32(WhKPP)[0] = 0;
	}
	void PutPayerRcvrInfo(const BankStmntItem * pItem)
	{
		BankStmntItem temp = *this;
		WeArePayer = BIN(Amount < 0.0);
		if(!WeArePayer) {
			STRNSCPY(PayerINN, temp.PayerINN[0] ? temp.PayerINN : temp.ReceiverINN);
			STRNSCPY(ReceiverINN, pItem->PayerINN);
			STRNSCPY(PayerKPP, temp.PayerKPP[0] ? temp.PayerKPP : temp.ReceiverKPP);
			STRNSCPY(ReceiverKPP, pItem->PayerKPP);
			STRNSCPY(PayerName, temp.PayerName[0] ? temp.PayerName : temp.ReceiverName);
			STRNSCPY(ReceiverName, pItem->PayerName);
			STRNSCPY(PayerBankCode, temp.PayerBankCode[0] ? temp.PayerBankCode : temp.ReceiverBankCode);
			STRNSCPY(ReceiverBankCode, pItem->PayerBankCode);
			STRNSCPY(PayerBankAcc, temp.PayerBankAcc[0] ? temp.PayerBankAcc : temp.ReceiverBankAcc);
			STRNSCPY(ReceiverBankAcc, pItem->PayerBankAcc);
			STRNSCPY(PayerBankName, temp.PayerBankName[0] ? temp.PayerBankName : temp.ReceiverBankName);
			STRNSCPY(ReceiverBankName, pItem->PayerBankName);
			STRNSCPY(PayerBankCorr, temp.PayerBankCorr[0] ? temp.PayerBankCorr : temp.ReceiverBankCorr);
			STRNSCPY(ReceiverBankCorr, pItem->PayerBankCorr);
			PayerPersonID = temp.PayerPersonID ? temp.PayerPersonID : temp.ReceiverPersonID;
			ReceiverPersonID = pItem->PayerPersonID;
			STRNSCPY(TaxPayerKPP, temp.TaxPayerKPP[0] ? temp.TaxPayerKPP : temp.TaxReceiverKPP);
			STRNSCPY(TaxReceiverKPP, pItem->TaxPayerKPP);
		}
		else {
			STRNSCPY(ReceiverINN, temp.ReceiverINN[0] ? temp.ReceiverINN : temp.PayerINN);
			STRNSCPY(PayerINN, pItem->PayerINN);
			STRNSCPY(ReceiverKPP, temp.ReceiverKPP[0] ? temp.ReceiverKPP : temp.PayerKPP);
			STRNSCPY(PayerKPP, pItem->PayerKPP);
			STRNSCPY(ReceiverName, temp.ReceiverName[0] ? temp.ReceiverName : temp.PayerName);
			STRNSCPY(PayerName, pItem->PayerName);
			STRNSCPY(ReceiverBankCode, temp.ReceiverBankCode[0] ? temp.ReceiverBankCode : temp.PayerBankCode);
			STRNSCPY(PayerBankCode, pItem->PayerBankCode);
			STRNSCPY(ReceiverBankAcc, temp.ReceiverBankAcc[0] ? temp.ReceiverBankAcc : temp.PayerBankAcc);
			STRNSCPY(PayerBankAcc, pItem->PayerBankAcc);
			STRNSCPY(ReceiverBankName, temp.ReceiverBankName[0] ? temp.ReceiverBankName : temp.PayerBankName);
			STRNSCPY(PayerBankName, pItem->PayerBankName);
			STRNSCPY(ReceiverBankCorr, temp.ReceiverBankCorr[0] ? temp.ReceiverBankCorr : temp.PayerBankCorr);
			STRNSCPY(PayerBankCorr, pItem->PayerBankCorr);
			ReceiverPersonID = temp.ReceiverPersonID ? temp.ReceiverPersonID : temp.PayerPersonID;
			PayerPersonID = pItem->PayerPersonID;
			STRNSCPY(TaxReceiverKPP, temp.TaxReceiverKPP[0] ? temp.TaxReceiverKPP : temp.TaxPayerKPP);
			STRNSCPY(TaxPayerKPP, pItem->TaxPayerKPP);
		}
	}
	const char * GetContragentName() const { return WeArePayer ? ReceiverName : PayerName; }
	const char * GetContragentINN()  const { return WeArePayer ? ReceiverINN  : PayerINN;  }
	const char * GetContragentBnkAcc() const { return WeArePayer ? ReceiverBankAcc  : PayerBankAcc;  } // @v10.0.03
	const char * GetOurBIC() const { return WeArePayer ? PayerBankCode : ReceiverBankCode; }
	SString & MakeDescrText(SString &) const;

	int    WeArePayer;              // !0 - главная организация является плательщиком
	PPID   PayerPersonID;			// ИД персоналии плательщика //
	PPID   ReceiverPersonID;		// ИД персоналии получателя  //
	char   WhKPP[24];               // @v9.1.1 Кпп склада, к которому привязан документ
};

SString & BankStmntItem::MakeDescrText(SString & rBuf) const
{
	return rBuf.Z().Cat(Date, DATF_DMY|DATF_CENTURY).CatDiv('-', 1).Cat(Code).CatDiv('-', 1).
		Cat(GetContragentName()).Space().Eq().Cat(Amount, SFMT_MONEY);
}

class Helper_ClientBank2 {
public:
	Helper_ClientBank2(const DateRange * pPeriod);
	~Helper_ClientBank2();
	int    ReadDefinition(const char * pIniSection); // *
	int    GetRecord(BankStmntItem * pItem)
	{
		PPSetAddedMsgString(P.FileName);
		memzero(pItem, sizeof(*pItem));
		pItem->AddedAssocTag = -1;
		int    ok = P_ImEx ? P_ImEx->ReadRecord(pItem, sizeof(*pItem)) : 0;
		if(ok > 0) {
			P_ImEx->GetParamConst().InrRec.ConvertDataFields(CTRANSF_OUTER_TO_INNER, pItem);
		}
		return ok;
	}
	int    OpenInputFile()
	{
		return P_ImEx ? P_ImEx->OpenFileForReading(0) : 0;
	}
	int    CreateOutputFile(StringSet * pResultFileList)
	{
		int    ok = 1;
		if(P_ImEx) {
			if(!Period.IsZero()) {
				Sdr_ImpExpHeader hdr_data;
				// @v10.7.9 @ctr MEMSZERO(hdr_data);
				hdr_data.PeriodLow = Period.low;
				hdr_data.PeriodUpp = Period.upp;
				P_ImEx->SetHeaderData(&hdr_data);
			}
			else
				P_ImEx->SetHeaderData(0);
			P_ImEx->OpenFileForWriting(0, 1, pResultFileList);
		}
		else
			ok = 0;
		return ok;
		// @v9.6.3 @fix (не понятная лишняя строка) return P_ImEx ? P_ImEx->OpenFileForWriting(0, 1) : 0;
	}
	int    PutRecord(const PPBillPacket * pPack, PPID debtBillID, PPLogger * pLogger);
	int    PutHeader()
	{
		return -1;
	}
	int    PutEnd()
	{
		return -1;
	}
	int    GetStat(long * pAcceptedCount, long * pRejectedCount, double * pAmount) const
	{
		ASSIGN_PTR(pAcceptedCount, AcceptedCount);
		ASSIGN_PTR(pRejectedCount, RejectedCount);
		ASSIGN_PTR(pAmount, AcceptedAmount);
		return 1;
	}
	PPCliBnkImpExpParam & GetParam() { return P; }
private:
	static SString & MakeVatText(const PPBillPacket *, SString & rBuf);
	PPCliBnkImpExpParam P;
	PPImpExp * P_ImEx;
	BankStmntItem Buf;
	SArray * P_FldList;
	PPObjPerson PsnObj;
	long   AcceptedCount;
	long   RejectedCount;
	double AcceptedAmount;
	DateRange Period;
};

ClientBankImportDef::ClientBankImportDef()
	{ P_Helper = new Helper_ClientBank2(0); }
ClientBankImportDef::~ClientBankImportDef()
	{ delete static_cast<Helper_ClientBank2 *>(P_Helper); }
PPImpExpParam & ClientBankImportDef::GetParam() const
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->GetParam(); }
int ClientBankImportDef::ReadDefinition(const char * pIniSection)
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->ReadDefinition(pIniSection); }

// AHTOXA {
static int GetOurInfo(BankStmntItem * pItem)
{
	int    ok = 0;
	if(pItem) {
		SString temp_buf;
		PPObjPerson psn_obj;
		PPPersonPacket org_pack;
		PPID   org_id = GetMainOrgID();
		THROW(psn_obj.GetPacket(org_id, &org_pack, 0));
		org_pack.GetRegNumber(PPREGT_TPID, temp_buf); temp_buf.CopyTo(pItem->PayerINN, sizeof(pItem->PayerINN));
		org_pack.GetRegNumber(PPREGT_KPP,  temp_buf); temp_buf.CopyTo(pItem->PayerKPP, sizeof(pItem->PayerKPP)); // @v7.4.5
		org_pack.GetRegNumber(PPREGT_BIC, temp_buf); temp_buf.CopyTo(pItem->PayerBankCode, sizeof(pItem->PayerBankCode));
		org_pack.GetRegNumber(PPREGT_BNKCORRACC, temp_buf); temp_buf.CopyTo(pItem->PayerBankCorr, sizeof(pItem->PayerBankCorr));
		STRNSCPY(pItem->PayerName, org_pack.Rec.Name);
		pItem->PayerPersonID = org_pack.Rec.ID;
		/*
		pItem->PayerBankAcc = ?;
		pItem->PayerBankName = ?;
		*/
		ok = 1;
	}
	CATCHZOK
	return ok;
}
// } AHTOXA

static void LogError(PPLogger & rLogger, long err, const BankStmntItem * pItem)
{
	SString msg_buf, log_buf;
	pItem->MakeDescrText(msg_buf).Quot('(', ')');
	if(PPGetMessage(mfError, err, msg_buf, 1, log_buf))
		rLogger.Log(log_buf);
}

struct Assoc {
	Assoc(const BankStmntAssocItem * pItem, PPID arID, PPID psnID) : P_Item(pItem), ArticleID(arID), PersonID(psnID)
	{
	}
	const BankStmntAssocItem * P_Item;
	PPID   ArticleID;
	PPID   PersonID;
};

class ResolveAssocCollisionDialog : public TDialog {
public:
	ResolveAssocCollisionDialog() : TDialog(DLG_ASC_RESOLVE), P_A(0)
	{
	}
	int    setDTS(const SArray * pA)
	{
		P_A = pA;
		PPIDArray op_list;
		const uint _c = SVectorBase::GetCount(P_A);
		PPID   op_id = 0;
		if(_c) {
			for(uint i = 0; i < pA->getCount(); i++) {
				op_list.add(static_cast<const Assoc *>(pA->at(i))->P_Item->OpID);
			}
			op_id = static_cast<Assoc *>(pA->at(0))->P_Item->OpID;
		}
		SetupOprKindCombo(this, CTLSEL_ASCRES_OPRKIND, op_id, 0, &op_list, OPKLF_OPLIST);
		SetupPersonsListByOprKind(op_id);
		return 1;
	}
	int    getDTS(SArray * pA)
	{
		int    ok = 1;
		Assoc  a(0, 0, 0);
		PPID   op_id = 0;
		getCtrlData(CTLSEL_ASCRES_OPRKIND, &op_id);
		getCtrlData(CTLSEL_ASCRES_PERSON, &a.PersonID);
		for(uint i = 0; i < pA->getCount(); i++) {
			const Assoc * p_assoc = static_cast<const Assoc *>(pA->at(i));
			if(p_assoc->P_Item->OpID == op_id && p_assoc->PersonID == a.PersonID)
				a = *p_assoc;
		}
		if(a.P_Item) {
			pA->freeAll();
			pA->insert(&a);
		}
		else
			ok = 0;
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_ASCRES_OPRKIND)) {
			SetupPersonsListByOprKind(getCtrlLong(CTLSEL_ASCRES_OPRKIND));
			clearEvent(event);
		}
	}
	void SetupPersonsListByOprKind(PPID opID)
	{
		Assoc * p_a;
		StrAssocArray persons;
		PPObjPerson psn_obj;
		PersonTbl::Rec r;
		for(uint i = 0; P_A->enumItems(&i, (void **)&p_a);) {
			if(p_a->P_Item->OpID == opID && psn_obj.Fetch(p_a->PersonID, &r) > 0) { // @v10.3.12 Search-->Fetch
				persons.Add(p_a->PersonID, r.Name);
			}
		}
		PPID   s = persons.getCount() ? persons.Get(0).Id : 0;
		SetupStrAssocCombo(this, CTLSEL_ASCRES_PERSON, persons, s, 0);
	}
	const  SArray * P_A;
};

static int ResolveAssocCollision(SArray *pA, const BankStmntItem * pItem)
{
	int    ok = 1;
	SString temp_buf;
	SString msg_buf;
	PPWaitStop();
	pItem->MakeDescrText(msg_buf).Quot('(', ')');
	PPGetMessage(mfError, PPERR_ASSOCCOLLISION, msg_buf, 1, temp_buf);
	ResolveAssocCollisionDialog * p_dlg = new ResolveAssocCollisionDialog();
	p_dlg->setStaticText(CTL_ASCRES_TXT, temp_buf);
	p_dlg->setDTS(pA);
	if(ExecView(p_dlg) == cmOK)
		ok = p_dlg->getDTS(pA);
	else
		ok = 0;
	delete p_dlg;
	PPWaitStart();
	return ok;
}

static int TurnBankImportPacket(const Assoc * pAssoc, BankStmntItem * pItem, PPID obj2ID, PPID agentID, PPLogger & rLogger)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	SString msg_buf;
	SString log_buf;
	PPID   loc_id = 0;
	PPBillPacket  pack;
	AmtList amt_list;
	if(pItem->LocSymb[0]) {
		PPObjLocation loc_obj;
		if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, pItem->LocSymb, &loc_id) <= 0)
			loc_id = LConfig.Location;
	}
	THROW(pack.CreateBlank_WithoutCode(pAssoc->P_Item->OpID, 0, loc_id, 1));
	pack.Rec.Dt = pItem->Date;
	pack.Rec.Object = pAssoc->ArticleID;
	pack.Rec.Object2 = obj2ID;
	pack.Ext.AgentID = agentID;
	STRNSCPY(pack.Rec.Code, pItem->Code);
	// @v11.1.12 STRNSCPY(pack.Rec.Memo, pItem->Purpose);
	pack.SMemo = pItem->Purpose; // @v11.1.12
	const int sar = p_bobj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, 0, 0);
	THROW(sar);
	if(sar > 0) {
		LogError(rLogger, PPERR_DOC_ALREADY_EXISTS, pItem);
	}
	else {
		PPTransaction tra(1);
		THROW(tra);
		amt_list.Put(PPAMT_MAIN, pack.Rec.CurID, fabs(pItem->Amount), 0, 1);
		pack.InitAmounts(&amt_list);
		THROW(p_bobj->FillTurnList(&pack));
		THROW(p_bobj->TurnPacket(&pack, 0));
		pItem->MakeDescrText(temp_buf).Quot('(', ')');
		if(PPLoadText(PPTXT_CLIBNK_BILLTURNED, msg_buf))
			rLogger.Log(log_buf.Printf(msg_buf, temp_buf.cptr()));
		if(pItem->DebtBillID || (!isempty(pItem->DebtBillCode) && checkdate(pItem->DebtBillDate))) {
			//
			// Если идент связанного документа не нулевой либо определены номер и дата связанного документа,
			// то пытаемся зачесть принятый документ.
			//
			msg_buf.Z().CatEq("ID", pItem->DebtBillID).CatDiv(';', 2).
				CatEq("Date", pItem->DebtBillDate).CatDiv(';', 2).CatEq("Code", pItem->DebtBillCode);
			if(CheckOpFlags(pack.Rec.OpID, OPKF_RECKON)) {
				PPObjBill::ReckonParam rp(1, 1);
				rp.ForceBillID = pItem->DebtBillID;
				rp.ForceBillDate = pItem->DebtBillDate;
				STRNSCPY(rp.ForceBillCode, pItem->DebtBillCode);
				int r = p_bobj->ReckoningPaym(pack.Rec.ID, rp, 0);
				if(!r)
					LogError(rLogger, -1, pItem);
				else if(r > 0)
					rLogger.LogString(PPTXT_CLIBNK_IMPRECKONED, msg_buf);
				else
					rLogger.LogString(PPTXT_CLIBNK_IMPNRECKONED, msg_buf);
			}
			else
				rLogger.LogString(PPTXT_CLIBNK_OPNRECKON, msg_buf);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

static int IsOurINN(const char * pINN)
{
	int    yes = 0;
	if(!isempty(pINN)) {
		PPIDArray psn_list;
		PPObjPerson psn_obj;
		yes = BIN(psn_obj.GetListByRegNumber(PPREGT_TPID, PPPRK_MAIN, pINN, psn_list) > 0);
	}
	return yes;
}

/*static*/int ClientBankImportDef::ReadAssocList(SVector * pList)
{
	struct BankStmntAssocItem_Pre578 { // @persistent @store(PropertyTbl)[as item of array] @flat
		PPID   AccSheetID;       //
		int16  Sign;             // -1, 1, 0 (undefined)
		PPID   OpID;             //
	};
	int    ok = -1, r;
	Reference * p_ref = PPRef;
	SVector prev_list(sizeof(BankStmntAssocItem_Pre578));
	SVector temp_list(sizeof(BankStmntAssocItem));
	THROW(r = p_ref->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CLIBNKASSCCFG2, &temp_list));
	if(r > 0) {
		THROW_SL(pList->copy(temp_list));
	}
	else {
		THROW(r = p_ref->GetPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CLIBNKASSCCFG, &prev_list));
		if(r > 0) {
			BankStmntAssocItem_Pre578 * p_item = 0;
			for(uint i = 0; prev_list.enumItems(&i, (void **)&p_item);) {
				BankStmntAssocItem item;
				MEMSZERO(item);
				item.AccSheetID = p_item->AccSheetID;
				item.Sign = p_item->Sign;
				item.AddedTag = -1;
				item.OpID = p_item->OpID;
				THROW_SL(temp_list.insert(&item));
			}
			ok = 1;
		}
		else
			ok = -1;
		THROW_SL(pList->copy(temp_list));
	}
	CATCHZOK
	return ok;
}

/*static*/int ClientBankImportDef::WriteAssocList(const SVector * pList, int use_ta) // @v9.8.8 SArray-->SVector
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CLIBNKASSCCFG2, pList, 0));
		if(pList) {
			// Удаляем старую версию ассоциаций
			THROW(p_ref->PutPropArray(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CLIBNKASSCCFG, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ClientBankImportDef::ImportAll()
{
	int    ok = 1;
	int    status = 1;
	SString wait_msg;
	SString contragent_inn;
	Helper_ClientBank2 * p_helper = static_cast<Helper_ClientBank2 *>(P_Helper);
	const  PPCliBnkImpExpParam & r_params = p_helper->GetParam();
	int    payer_by_sign = BIN(r_params.DefPayerByAmtSign);
	PPObjArticle ar_obj;
	PPObjPerson psn_obj;
	PPObjAccSheet acc_sheet_obj;
	PPObjOprKind op_obj;
	SVector cfg(sizeof(BankStmntAssocItem));
	BankStmntItem item, our_item;
	PPLogger logger;
	PPWaitStart();
	THROW(ClientBankImportDef::ReadAssocList(&cfg));
	if(payer_by_sign) {
		THROW(GetOurInfo(&our_item));
		if(r_params.BnkCode[0])
			STRNSCPY(our_item.PayerBankCode, r_params.BnkCode);
	}
	THROW(p_helper->OpenInputFile());
	do {
		int    r = p_helper->GetRecord(&item);
		THROW(r);
		if(r > 0) {
			if(payer_by_sign)
                item.PutPayerRcvrInfo(&our_item);
			if(IsOurINN(item.PayerINN))
				item.WeArePayer = 1;
			else {
				item.WeArePayer = 0;
				if(!IsOurINN(item.ReceiverINN)) {
					LogError(logger, PPERR_INNNOTFOUND, &item);
					continue;
				}
			}
			uint   i, j;
			SArray best_assoc(sizeof(Assoc));
			PPIDArray psn_list, ar_list;
			PPID * p_psn_id;
			PPWaitMsg(item.MakeDescrText(wait_msg));
			contragent_inn = item.GetContragentINN();
			if(contragent_inn.NotEmptyS()) {
				THROW(psn_obj.GetListByRegNumber(PPREGT_TPID, 0, contragent_inn, psn_list));
				THROW(ar_obj.GetByPersonList(0, &psn_list, &ar_list));
			}
			else {
				contragent_inn = item.GetContragentBnkAcc();
				if(contragent_inn.NotEmptyS()) {
					THROW(psn_obj.GetListByRegNumber(PPREGT_BANKACCOUNT, 0, contragent_inn, psn_list));
					THROW(ar_obj.GetByPersonList(0, &psn_list, &ar_list));
				}
				else
					ar_list.add(0L);
			}
			for(i = 0; i < ar_list.getCount(); i++) {
				const PPID ar_id = ar_list.get(i);
				ArticleTbl::Rec ar_rec;
				// @v10.6.4 MEMSZERO(ar_rec);
				if(ar_id)
					THROW(ar_obj.Fetch(ar_id, &ar_rec) > 0);
				//for(j = 0; cfg.enumItems(&j, (void **)&p_assoc_item);) {
				for(uint j = 0; j < cfg.getCount(); j++) {
					const BankStmntAssocItem * p_assoc_item = static_cast<const BankStmntAssocItem *>(cfg.at(j));
					if(p_assoc_item->AccSheetID == ar_rec.AccSheetID && (p_assoc_item->AddedTag < 0 || p_assoc_item->AddedTag == item.AddedAssocTag))
						if(item.WeArePayer ? (p_assoc_item->Sign < 0) : (p_assoc_item->Sign > 0)) {
							Assoc a(p_assoc_item, ar_id, (ar_id ? ObjectToPerson(ar_id) : 0));
							best_assoc.insert(&a);
						}
				}
			}
			if(best_assoc.getCount() > 1) { // collision
				LogError(logger, PPERR_ASSOCCOLLISION, &item);
				if(ResolveAssocCollision(&best_assoc, &item) <= 0)
					continue;
			}
			if(best_assoc.getCount() == 0)
				for(i = 0; i < ar_list.getCount(); i++) {
					const PPID ar_id = ar_list.get(i);
					ArticleTbl::Rec ar_rec;
					// @v10.6.4 MEMSZERO(ar_rec);
					if(ar_id)
						THROW(ar_obj.Fetch(ar_id, &ar_rec) > 0);
					//for(j = 0; cfg.enumItems(&j, (void **)&p_assoc_item);) {
					for(j = 0; j < cfg.getCount(); j++) {
						const BankStmntAssocItem * p_assoc_item = static_cast<const BankStmntAssocItem *>(cfg.at(j));
						if(p_assoc_item->AccSheetID == ar_rec.AccSheetID && p_assoc_item->Sign == 0 && (p_assoc_item->AddedTag < 0 || p_assoc_item->AddedTag == item.AddedAssocTag)) {
							Assoc a(p_assoc_item, ar_id, (ar_id ? ObjectToPerson(ar_id) : 0));
							best_assoc.insert(&a);
						}
					}
				}
			if(best_assoc.getCount() > 1) { // collision
				LogError(logger, PPERR_ASSOCCOLLISION, &item);
				if(ResolveAssocCollision(&best_assoc, &item) <= 0)
					continue;
			}
			if(best_assoc.getCount() != 1) {
				LogError(logger, PPERR_ASSOCNOTFOUND, &item);
				continue;
			}
			//we know best association - this is a best_assoc[0]
			{
				const  Assoc * p_assoc = static_cast<const Assoc *>(best_assoc.at(0));
				PPID   obj2_ar_id = 0, agent_id = 0;
				PPAccSheet acs_rec2;
				PPOprKind op_rec;
				if(op_obj.Search(p_assoc->P_Item->OpID, &op_rec) > 0 && op_rec.AccSheet2ID && acc_sheet_obj.Fetch(op_rec.AccSheet2ID, &acs_rec2) > 0) {
					if(acs_rec2.Assoc == PPOBJ_PERSON) {
						if(item.Obj2INN[0]) {
							THROW(psn_obj.GetListByRegNumber(PPREGT_TPID, 0, item.Obj2INN, psn_list));
							for(j = 0; psn_list.enumItems(&j, (void **)&p_psn_id);)
								if(ar_obj.GetByPerson(op_rec.AccSheet2ID, *p_psn_id, &obj2_ar_id) > 0)
									break;
						}
						else {
							if(!obj2_ar_id ) {
								THROW(psn_obj.GetListByRegNumber(PPREGT_BIC, 0, item.GetOurBIC(), psn_list));
								for(j = 0; psn_list.enumItems(&j, (void **)&p_psn_id);)
									if(ar_obj.GetByPerson(op_rec.AccSheet2ID, *p_psn_id, &obj2_ar_id) > 0)
										break;
							}
						}
					}
					if(!obj2_ar_id && p_assoc->ArticleID) {
						PPClientAgreement cli_agt;
						if(ar_obj.GetClientAgreement(p_assoc->ArticleID, cli_agt, 0) > 0) {
							ArticleTbl::Rec ar2_rec;
							if(cli_agt.ExtObjectID && ar_obj.Fetch(cli_agt.ExtObjectID, &ar2_rec) > 0 && ar2_rec.AccSheetID == op_rec.AccSheet2ID)
								obj2_ar_id = cli_agt.ExtObjectID;
						}
					}
				}
				if(item.AgentINN[0]) {
					PPID   agent_acs_id = GetAgentAccSheet();
					if(agent_acs_id) {
						THROW(psn_obj.GetListByRegNumber(PPREGT_TPID, 0, item.AgentINN, psn_list));
						for(j = 0; psn_list.enumItems(&j, (void **)&p_psn_id);)
							if(ar_obj.GetByPerson(agent_acs_id, *p_psn_id, &agent_id) > 0)
								break;
					}
				}
				THROW(TurnBankImportPacket(p_assoc, &item, obj2_ar_id, agent_id, logger));
			}
		}
		else
			status = 0;
	} while(status > 0);
	CATCHZOK
	PPWaitStop();
	return ok;
}

Helper_ClientBank2::Helper_ClientBank2(const DateRange * pPeriod) : AcceptedCount(0), RejectedCount(0), AcceptedAmount(0.0), P_ImEx(0), P_FldList(0)
{
	Period.Set(pPeriod);
}

Helper_ClientBank2::~Helper_ClientBank2()
{
	delete P_ImEx;
}

int Helper_ClientBank2::ReadDefinition(const char * pIniSection)
{
	int    ok = 1;
	SString ini_file_name/*, sect*/;
	ZDELETE(P_ImEx);
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_CLIBNK_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name/*, 0, 1, 1*/);
		THROW(LoadSdRecord(PPREC_CLIBNKDATA, &P.InrRec));
		THROW(P.ReadIni(&ini_file, pIniSection, 0));
		P_ImEx = new PPImpExp(&P, 0);
	}
	CATCHZOK
	return ok;
}
//
//
//
ClientBankExportDef::ClientBankExportDef(const DateRange * pPeriod) : P_Helper(new Helper_ClientBank2(pPeriod))
	{}
ClientBankExportDef::~ClientBankExportDef()
	{ delete static_cast<Helper_ClientBank2 *>(P_Helper); }
PPImpExpParam & ClientBankExportDef::GetParam() const
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->GetParam(); }
int ClientBankExportDef::ReadDefinition(const char * pIniSection)
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->ReadDefinition(pIniSection); }
int ClientBankExportDef::CreateOutputFile(StringSet * pResultFileList)
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->CreateOutputFile(pResultFileList); }
int ClientBankExportDef::CloseOutputFile()
	{ return 1; }
int ClientBankExportDef::PutRecord(const PPBillPacket * pPack, PPID debtBillID, PPLogger * pLogger)
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->PutRecord(pPack, debtBillID, pLogger); }
int ClientBankExportDef::PutHeader()
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->PutHeader(); }
int ClientBankExportDef::PutEnd()
	{ return static_cast<Helper_ClientBank2 *>(P_Helper)->PutEnd(); }
int ClientBankExportDef::GetStat(long * pAcceptedCount, long * pRejectedCount, double * pAmount)
	{ return static_cast<const Helper_ClientBank2 *>(P_Helper)->GetStat(pAcceptedCount, pRejectedCount, pAmount); }

/*static*/SString & Helper_ClientBank2::MakeVatText(const PPBillPacket * pPack, SString & rBuf)
{
	PPLoadString("vat", rBuf);
	rBuf.Space();
	if(pPack->P_PaymOrder->VATSum > 0) {
		if(pPack->P_PaymOrder->VATRate)
			rBuf.Space().Cat(pPack->P_PaymOrder->VATRate).CatChar('%').CatDiv('-', 1).Cat(pPack->P_PaymOrder->VATSum, SFMT_MONEY);
		else
			rBuf.CatDiv('-', 1).Cat(pPack->P_PaymOrder->VATSum, SFMT_MONEY);
	}
	else {
		SString n;
		rBuf.CatDiv('-', 1).Cat(PPGetWord(PPWORD_NOTAX_VERB, 0, n));
	}
	return rBuf;
}

static SString & FASTCALL _EncodeStr(const char * pStr, SString & rBuf)
	{ return (rBuf = pStr).Transf(CTRANSF_INNER_TO_OUTER); }

int Helper_ClientBank2::PutRecord(const PPBillPacket * pPack, PPID debtBillID, PPLogger * pLogger)
{
	int    ok = -1;
	const  PPBankingOrder * p_order = pPack ? pPack->P_PaymOrder : 0;
	if(p_order) {
		PPObjLocation loc_obj;
		BankStmntItem data_buf;
		BnkAcctData payer_ba, rcvr_ba;
		PersonReq payer_req, rcvr_req;
		int    r1 = PsnObj.GetBnkAcctData(p_order->PayerBnkAccID, static_cast<const PPBankAccount *>(0), &payer_ba);
		int    r2 = PsnObj.GetBnkAcctData(p_order->RcvrBnkAccID, static_cast<const PPBankAccount *>(0), &rcvr_ba);
		SString buf, temp_buf;
		if(r1 <= 0 || r2 <= 0) {
			if(pLogger) {
				uint msg_code = 0;
				SString str_fmt, msg, buf, str_dt;
				if(r1 <= 0)
					msg_code = p_order->PayerBnkAccID ? PPERR_PAYERBNKACCNOTFOUND : PPERR_PAYERBNKACCEMPTY;
				else
					msg_code = p_order->RcvrBnkAccID ? PPERR_RCVRBNKACCNOTFOUND : PPERR_RCVRBNKACCEMPTY;
				PPLoadString(PPMSG_ERROR, PPERR_EXPORTBNKORDER, str_fmt);
				PPLoadString(PPMSG_ERROR, msg_code, buf);
				str_dt.Cat(pPack->Rec.Dt);
				msg.Printf(str_fmt, pPack->Rec.Code, str_dt.cptr(), buf.cptr());
				pLogger->Log(msg);
			}
			RejectedCount++;
			return -1;
		}
		THROW(P_ImEx);
		PsnObj.GetPersonReq(p_order->PayerID, &payer_req);
		PsnObj.GetPersonReq(p_order->RcvrID,  &rcvr_req);
		{
			data_buf.BillID   = pPack->Rec.ID;
			data_buf.Date     = pPack->Rec.Dt;
			data_buf.Amount   = pPack->GetAmount();
			data_buf.PayerPersonID    = p_order->PayerID;
			data_buf.ReceiverPersonID = p_order->RcvrID;
			data_buf.Sequence = p_order->BnkQueueing;
			data_buf.VatSum   = p_order->VATSum;
			STRNSCPY(data_buf.Code, _EncodeStr(pPack->Rec.Code, temp_buf));
			// @v11.1.12 STRNSCPY(data_buf.Purpose, _EncodeStr(pPack->Rec.Memo, temp_buf));
			STRNSCPY(data_buf.Purpose, _EncodeStr(pPack->SMemo, temp_buf)); // @v11.1.12
			(buf = pPack->SMemo).Space().Cat(MakeVatText(pPack, temp_buf)). // @v11.1.12 pPack->Rec.Memo-->pPack->SMemo
				Transf(CTRANSF_INNER_TO_OUTER).CopyTo(data_buf.PurposePlusVat, sizeof(data_buf.PurposePlusVat));
			{
				long   paym_method = 0;
				StringSet ss(';', P.PaymMethodTransl);
				for(uint i = 0; ss.get(&i, buf);) {
					uint j = 0;
					StringSet ss1(',', buf);
					ss1.get(&j, temp_buf);
					if(temp_buf.ToLong() == p_order->BnkPaymMethod) {
						ss1.get(&j, temp_buf);
						paym_method = temp_buf.ToLong();
					}
				}
				data_buf.PaymMethod = static_cast<int16>(paym_method);
			}
			STRNSCPY(data_buf.PayerName,     _EncodeStr(payer_req.ExtName[0] ? payer_req.ExtName : payer_req.Name, temp_buf));
			STRNSCPY(data_buf.PayerINN,      _EncodeStr(payer_req.TPID, temp_buf));
			STRNSCPY(data_buf.PayerKPP,      _EncodeStr(payer_req.KPP, temp_buf));
			STRNSCPY(data_buf.PayerBankAcc,  _EncodeStr(payer_ba.Acct, temp_buf));
			STRNSCPY(data_buf.PayerBankName, _EncodeStr(payer_ba.Bnk.ExtName[0] ? payer_ba.Bnk.ExtName : payer_ba.Bnk.Name, temp_buf));
			STRNSCPY(data_buf.PayerBankCode, _EncodeStr(payer_ba.Bnk.BIC, temp_buf));
			STRNSCPY(data_buf.PayerBankCorr, _EncodeStr(payer_ba.Bnk.CorrAcc, temp_buf));
			STRNSCPY(data_buf.ReceiverName,     _EncodeStr(rcvr_req.ExtName[0] ? rcvr_req.ExtName : rcvr_req.Name, temp_buf));
			STRNSCPY(data_buf.ReceiverINN,      _EncodeStr(rcvr_req.TPID, temp_buf));
			STRNSCPY(data_buf.ReceiverKPP,      _EncodeStr(rcvr_req.KPP, temp_buf));
			STRNSCPY(data_buf.ReceiverBankAcc,  _EncodeStr(rcvr_ba.Acct, temp_buf));
			STRNSCPY(data_buf.ReceiverBankName, _EncodeStr(rcvr_ba.Bnk.ExtName[0] ? rcvr_ba.Bnk.ExtName : rcvr_ba.Bnk.Name, temp_buf));
			STRNSCPY(data_buf.ReceiverBankCode, _EncodeStr(rcvr_ba.Bnk.BIC, temp_buf));
			STRNSCPY(data_buf.ReceiverBankCorr, _EncodeStr(rcvr_ba.Bnk.CorrAcc, temp_buf));
			if(pPack->Rec.Object2) {
				PersonReq req2;
				GetArticleName(pPack->Rec.Object2, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(data_buf.Obj2Name, sizeof(data_buf.Obj2Name));
				if(PsnObj.GetPersonReq(pPack->Rec.Object2, &req2) > 0)
					STRNSCPY(data_buf.Obj2INN, _EncodeStr(req2.TPID, temp_buf));
			}
			{
				LocationTbl::Rec loc_rec;
				RegisterTbl::Rec reg_rec;
				if(loc_obj.Search(pPack->Rec.LocID, &loc_rec) > 0) {
					STRNSCPY(data_buf.LocSymb, _EncodeStr(loc_rec.Code, temp_buf));
					if(loc_obj.GetRegister(pPack->Rec.LocID, PPREGT_KPP, pPack->Rec.Dt, 0, &reg_rec) > 0 && reg_rec.Num[0])
						STRNSCPY(data_buf.WhKPP, reg_rec.Num);
				}
			}
			STRNSCPY(data_buf.TaxPayerKPP, _EncodeStr(payer_req.KPP, temp_buf));
			STRNSCPY(data_buf.TaxReceiverKPP, _EncodeStr(rcvr_req.KPP, temp_buf));
			buf.Z();
			if(p_order->PayerStatus)
				buf.CatLongZ(p_order->PayerStatus, 2);
			STRNSCPY(data_buf.TaxPayerStatus, _EncodeStr(buf, temp_buf));
			STRNSCPY(data_buf.TaxClass,       _EncodeStr(p_order->Txm.TaxClass2, temp_buf));
			STRNSCPY(data_buf.OKATO,          _EncodeStr(p_order->Txm.OKATO, temp_buf));
			STRNSCPY(data_buf.TaxReason,      _EncodeStr(p_order->Txm.Reason, temp_buf));
			STRNSCPY(data_buf.TaxPeriod,      _EncodeStr(p_order->Txm.Period.Format(buf), temp_buf));
			STRNSCPY(data_buf.TaxDocNumber,   _EncodeStr(p_order->Txm.DocNumber, temp_buf));
			STRNSCPY(data_buf.UIN,            _EncodeStr(p_order->Txm.UIN, temp_buf));
			buf.Z().Cat(p_order->Txm.DocDate, MKSFMT(0, DATF_GERMAN/*|DATF_CENTURY*/)).
				CopyTo(data_buf.TaxDocDate, sizeof(data_buf.TaxDocDate));
			STRNSCPY(data_buf.TaxPaymType,    _EncodeStr(p_order->Txm.PaymType, temp_buf));
		}
		{
			BillTbl::Rec debt_rec;
			if(debtBillID && BillObj->Search(debtBillID, &debt_rec) > 0) {
				data_buf.DebtBillID = debt_rec.ID;
				data_buf.DebtBillDate = debt_rec.Dt;
				STRNSCPY(data_buf.DebtBillCode, debt_rec.Code);
			}
		}
		data_buf.FormalPurpose = p_order->FormalPurpose; // @v10.7.11
		PPSetAddedMsgString(P.FileName);
		THROW(P_ImEx->AppendRecord(&data_buf, sizeof(data_buf)));
		AcceptedCount++;
		AcceptedAmount += pPack->GetAmount();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int EditClientBankFormatDescription(const char * pIniSection)
{
	class CliBnkImpExpDialog : public ImpExpParamDialog {
	public:
		CliBnkImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPCLIBNK)
		{
		}
		int    setDTS(const PPCliBnkImpExpParam * pData)
		{
			Data = *pData;
			ImpExpParamDialog::setDTS(&Data);
			setCtrlString(CTL_IMPEXPCLIBNK_BNKCODE, Data.BnkCode);
			setCtrlString(CTL_IMPEXPCLIBNK_PMCODES, Data.PaymMethodTransl);
			setCtrlUInt16(CTL_IMPEXPCLIBNK_FLAGS, BIN(Data.DefPayerByAmtSign));
			return 1;
		}
		int    getDTS(PPCliBnkImpExpParam * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			THROW(ImpExpParamDialog::getDTS(&Data));
			getCtrlString(CTL_IMPEXPCLIBNK_BNKCODE, Data.BnkCode);
			getCtrlString(CTL_IMPEXPCLIBNK_PMCODES, Data.PaymMethodTransl);
			Data.DefPayerByAmtSign = BIN(getCtrlUInt16(CTL_IMPEXPCLIBNK_FLAGS));
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		PPCliBnkImpExpParam Data;
	};
	int    ok = -1;
	int    undecorated = 0; // Ддя обратной совместимости со старыми настройками
		// этот признак определяет, что в ini-файле имя секции задано без декорации
	CliBnkImpExpDialog * dlg = 0;
	PPCliBnkImpExpParam param, param1;
	SString ini_file_name, sect;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_CLIBNK_INI, ini_file_name));
	{
		int    direction = 0;
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_CLIBNKDATA, &param.InrRec));
		if(!isempty(pIniSection)) {
			sect = pIniSection;
			if(!param.ProcessName(3, sect))
				undecorated = 1;
			THROW(param.ReadIni(&ini_file, sect, 0));
			if(undecorated)
				param.ProcessName(1, sect);
		}
		THROW(CheckDialogPtr(&(dlg = new CliBnkImpExpDialog())));
		dlg->setDTS(&param);
		direction = param.Direction;
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&param)) {
				int    is_new = BIN(isempty(pIniSection) || param.Direction != direction);
				if(!is_new && undecorated)
					ini_file.RemoveSection(pIniSection);
				else if(!isempty(pIniSection))
					if(is_new)
						ini_file.RemoveSection(sect);
					else
						ini_file.ClearSection(sect);
				if(is_new && ini_file.IsSectExists(param.Name))
					PPError(PPERR_DUPOBJNAME);
				else if(!param.WriteIni(&ini_file, param.Name))
					PPError();
				else if(!ini_file.FlashIniBuf())
					PPError();
				else
					ok = 1;
			}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

class SetupCliBnkFormatsDialog : public PPListDialog {
public:
	SetupCliBnkFormatsDialog() : PPListDialog(DLG_CLIBNKFMTLIST, CTL_CLIBNKFMTLIST_LIST), BackupExecuted(0)
	{
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmCliBnkFmtListAssoc)) {
			SetupCliBnkAssoc();
			clearEvent(event);
		}
	}
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	StringSet Sections;
	PPCliBnkImpExpParam P;
	int    BackupExecuted;
};

int SetupCliBnkFormatsDialog::setupList()
{
	int    ok = 1;
	uint   p;
	SString section, all_fields_name, sect;
	StringSet all_sections;
	PPLogger logger;
	Sections.clear();
	GetCliBnkSections(&all_sections, 0, &P, BackupExecuted ? 0 : 5, &logger);
	BackupExecuted = 1;
	all_sections.sort();
	THROW(PPLoadText(PPTXT_CLIBNK_SECTION_WITHNAMES, all_fields_name));
	for(p = 0; all_sections.get(&p, section);)
		if(section.CmpNC(all_fields_name) != 0) {
			uint   id = 0;
			P.ProcessName(2, sect = section);
			Sections.add(section, &id);
			THROW(addStringToList(id, sect));
		}
	CATCHZOK
	return ok;
}

int SetupCliBnkFormatsDialog::addItem(long * pPos, long * pID)
{
	if(EditClientBankFormatDescription(0)) {
		updateList(-1);
		*pPos = *pID = 0;
		return 1;
	}
	else
		return 0;
}

int SetupCliBnkFormatsDialog::editItem(long pos, long id)
{
	SString section;
	return Sections.get(reinterpret_cast<uint *>(&id), section) ? EditClientBankFormatDescription(section) : -1;
}

int SetupCliBnkFormatsDialog::delItem(long pos, long id)
{
	int    ok = 1;
	SString ini_file_name, section;
	if(Sections.get(reinterpret_cast<uint *>(&id), section)) {
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_CLIBNK_INI, ini_file_name));
		{
			PPIniFile ini_file(ini_file_name, 0, 1, 1);
			THROW(ini_file.RemoveSection(section));
			THROW(ini_file.FlashIniBuf());
			updateList(-1);
		}
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int SetupCliBnkFormats()
{
	MemLeakTracer mlt;
	SetupCliBnkFormatsDialog * dlg = new SetupCliBnkFormatsDialog();
	return CheckDialogPtrErr(&dlg) ? ((ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1) : 0;
}
//
// @todo Сохранение конфигурации должно осуществляться единой транзакцией, а не отдельными
//   методами диалога.
//
int SetupCliBnkAssoc()
{
	class SetupCliBnkAssocDialog : public PPListDialog {
	public:
		SetupCliBnkAssocDialog() : PPListDialog(DLG_CBASCFG, CTL_CBASCFG_LIST)
		{
			updateList(-1);
		}
	private:
		static int CliBnkAssocItemDialog(BankStmntAssocItem * pItem)
		{
			class SetupCliBnkAssocItemDialog : public TDialog {
			public:
				SetupCliBnkAssocItemDialog () : TDialog(DLG_CBASITM)
				{
				}
				int    setDTS(const BankStmntAssocItem * pItem)
				{
					SetupPPObjCombo(this,  CTLSEL_CBASITM_ACC, PPOBJ_ACCSHEET, pItem->AccSheetID, 0, 0);
					SetupStringCombo(this, CTLSEL_CBASITM_SIGN, PPTXT_SIGN, 0);
					SetupPPObjCombo(this,  CTLSEL_CBASITM_OPRKIND, PPOBJ_OPRKIND, pItem->OpID, 0, 0);
					int    k = SIGN_NO_MATTER;
					switch(pItem->Sign) {
						case -1: k = SIGN_LT; break;
						case  1: k = SIGN_GT; break;
						case  0: k = SIGN_NO_MATTER; break;
					}
					setCtrlData(CTLSEL_CBASITM_SIGN, &k);
					setCtrlUInt16(CTL_CBASITM_ADDEDTAG, pItem->AddedTag);
					return 1;
				}
				int    getDTS(BankStmntAssocItem * pItem)
				{
					getCtrlData(CTL_CBASITM_ACC, &pItem->AccSheetID);
					getCtrlData(CTL_CBASITM_OPRKIND, &pItem->OpID);
					switch(getCtrlLong(CTL_CBASITM_SIGN)) {
						case SIGN_LT: pItem->Sign = -1; break;
						case SIGN_GT: pItem->Sign =  1; break;
						case SIGN_NO_MATTER: pItem->Sign = 0; break;
					}
					pItem->AddedTag = getCtrlUInt16(CTL_CBASITM_ADDEDTAG);
					return 1;
				}
			};
			DIALOG_PROC_BODY(SetupCliBnkAssocItemDialog, pItem);
		}
		virtual int setupList()
		{
			int    ok = 1;
			uint   i;
			PPOprKind op_rec;
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			SString temp_buf;
			SVector cfg(sizeof(BankStmntAssocItem));
			BankStmntAssocItem * p_assoc_item;
			THROW(ClientBankImportDef::ReadAssocList(&cfg));
			for(i = 0; cfg.enumItems(&i, (void **)&p_assoc_item);) {
				StringSet ss(SLBColumnDelim);
				if(acs_obj.Fetch(p_assoc_item->AccSheetID, &acs_rec) <= 0)
					ideqvalstr(p_assoc_item->AccSheetID, acs_rec.Name, sizeof(acs_rec.Name));
				ss.add(acs_rec.Name);
				int    k = -1;
				switch(p_assoc_item->Sign) {
					case -1: k = SIGN_LT - 1; break;
					case  1: k = SIGN_GT - 1; break;
					case  0: k = SIGN_NO_MATTER - 1; break;
				}
				THROW(PPGetSubStr(PPTXT_SIGN, k, temp_buf));
				ss.add(temp_buf + 2);
				ss.add(temp_buf.Z().Cat(p_assoc_item->AddedTag));
				GetOpData(p_assoc_item->OpID, &op_rec);
				ss.add(op_rec.Name);
				THROW(addStringToList(i - 1, ss.getBuf()));
			}
			CATCHZOKPPERR
			return ok;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = 1;
			*pPos = *pID = 0;
			SVector cfg(sizeof(BankStmntAssocItem));
			BankStmntAssocItem assoc_item;
			THROW(CheckCfgRights(PPCFGOBJ_CLIBNKAS, PPR_INS, 0));
			MEMSZERO(assoc_item);
			assoc_item.AddedTag = -1;
			if(CliBnkAssocItemDialog(&assoc_item) > 0) {
				THROW(ClientBankImportDef::ReadAssocList(&cfg));
				THROW_SL(cfg.insert(&assoc_item));
				THROW(ClientBankImportDef::WriteAssocList(&cfg, 1));
				*pPos = *pID = cfg.getCount();
				updateList(-1);
			}
			CATCHZOKPPERR
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = 1;
			SVector cfg(sizeof(BankStmntAssocItem));
			BankStmntAssocItem * p_assoc_item;
			THROW(CheckCfgRights(PPCFGOBJ_CLIBNKAS, PPR_MOD, 0));
			THROW(ClientBankImportDef::ReadAssocList(&cfg));
			if(pos < cfg.getCountI()) {
				THROW(p_assoc_item = static_cast<BankStmntAssocItem *>(cfg.at(id)));
				if(CliBnkAssocItemDialog(p_assoc_item) > 0) {
					THROW(ClientBankImportDef::WriteAssocList(&cfg, 1));
					updateList(-1);
				}
			}
			else
				ok = -1;
			CATCHZOKPPERR
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = 1;
			SVector cfg(sizeof(BankStmntAssocItem));
			THROW(CheckCfgRights(PPCFGOBJ_CLIBNKAS, PPR_DEL, 0));
			THROW(ClientBankImportDef::ReadAssocList(&cfg));
			if(pos < cfg.getCountI()) {
				THROW_SL(cfg.atFree(id));
				THROW(ClientBankImportDef::WriteAssocList(&cfg, 1));
				updateList(-1);
			}
			else
				ok = -1;
			CATCHZOKPPERR
			return ok;
		}
	};
	int    ok = 0;
	SetupCliBnkAssocDialog * dlg = new SetupCliBnkAssocDialog();
	THROW(CheckCfgRights(PPCFGOBJ_CLIBNKAS, PPR_READ, 0));
	THROW(CheckDialogPtr(&dlg));
	ok = (ExecView(dlg) == cmOK) ? 1 : -1;
	if(ok > 0) {
		THROW(CheckCfgRights(PPCFGOBJ_CLIBNKAS, PPR_MOD, 0));
		DS.LogAction(PPACN_CONFIGUPDATED, PPCFGOBJ_CLIBNKAS, 0, 0, 1/*use_ta*/);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int CliBnkSelectCfgDialog(int kind/*1 - export, 2 - import*/, SString & rSection)
{
	int    ok = -1;
	uint   id = 0;
	StringSet sections;
	StrAssocArray show_sections;
	SString buf, sect;
	PPLogger logger;
	TDialog * dlg = 0;
	PPCliBnkImpExpParam param;
	GetCliBnkSections(&sections, kind, &param, 0, &logger);
	THROW_PP(sections.getCount(), PPERR_CLIBNKCFG_NOTFOUND);
	if(sections.getCount() == 1) {
		sections.get(&id, rSection);
		if(rSection.NotEmptyS())
			ok = 1;
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GRPSEL))));
		THROW(PPLoadText(PPTXT_SELECT_CONFIGURATION, buf));
		dlg->setTitle(buf);
		{
			sections.sort();
			long str_id = 0;
			for(uint p = 0; sections.get(&p, sect);) {
				param.ProcessName(2, sect);
				show_sections.Add(++str_id, sect);
			}
		}
		SetupStrAssocCombo(dlg, CTLSEL_GRPSEL_GROUP, show_sections, 0, 0, 0, 0);
		if(ExecView(dlg) == cmOK) {
			long   id = dlg->getCtrlLong(CTLSEL_GRPSEL_GROUP);
			long   i = 1;
			for(uint p = 0; ok < 0 && sections.get(&p, sect); i++) {
				if(i == id) {
					rSection = sect;
					ok = 1;
				}
			}
		}
		else
			ok = -1;
	}
	CATCHZOK
	delete dlg;
	return ok;
}

int CliBnkImport()
{
	int    ok = -1;
	SString section;
	ClientBankImportDef def;
	if(CliBnkSelectCfgDialog(2, section) > 0) {
		THROW(def.ReadDefinition(section));
		THROW(def.ImportAll());
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
/*
http://cbrates.rbc.ru/bnk/bnk.exe

bnkseek.txt
reg.txt

30	КУРГАН	1	ФИЛИАЛ "КУРГАНСКИЙ" ЗАО АКБ "АЛЕФ-БАНК"		043735853	30101810800000000853
23	МОСКВА	1	АКБ "ЗЕМСКИЙ ЗЕМЕЛЬНЫЙ БАНК" ЗАО		044579161	30101810800000000161
36	ГОРНЯК	1	ОТД. "АГРОПРОМБАНКА"		040137701
20	МОСКВА	1	ЗАО "НОВЫЙ ПРОМЫШЛЕННЫЙ БАНК"		044599749	30101810000000000749
20	МОСКВА	1	АКБ "БАЛТИЙСКИЙ БАНК РАЗВИТИЯ" (ЗАО)		044579769	30101810600000000769
*/

int ConvertRbcBnk(const char * pPath)
{
	int    ok = 1, r;
	SString path(pPath);
	SString file_name;
	SString dbf_file_name;
	SString line_buf;
	{
		PPImpExpParam in_par, out_par;
		in_par.Direction = 1;
		in_par.DataFormat = PPImpExpParam::dfText;
		in_par.TdfParam.FldDiv = "\t";
		THROW(LoadSdRecord(PPREC_RBCBNKREG, &in_par.OtrRec));
		in_par.InrRec = in_par.OtrRec;
		(in_par.FileName = path).SetLastSlash().Cat("reg.txt");

		out_par.Direction = 0;
		out_par.DataFormat = PPImpExpParam::dfDbf;
		out_par.TdfParam.Flags |= TextDbFile::fCpOem;
		THROW(LoadSdRecord(PPREC_RBCBNKREG, &out_par.OtrRec));
		out_par.InrRec = out_par.OtrRec;
		(out_par.FileName = path).SetLastSlash().Cat("reg.dbf");
		{
			PPImpExp in_file(&in_par, 0);
			PPImpExp out_file(&out_par, 0);
			Sdr_RbcBnkReg rec;
			THROW(in_file.OpenFileForReading(0));
			THROW(out_file.OpenFileForWriting(0, 1));
			// @v10.7.9 @ctr MEMSZERO(rec);
			while((r = in_file.ReadRecord(&rec, sizeof(rec))) > 0) {
				THROW(out_file.AppendRecord(&rec, sizeof(rec)));
				MEMSZERO(rec);
			}
			THROW(r);
		}
	}
	{
		PPImpExpParam in_par, out_par;
		in_par.Direction = 1;
		in_par.DataFormat = PPImpExpParam::dfText;
		in_par.TdfParam.FldDiv = "\t";
		THROW(LoadSdRecord(PPREC_RBCBNKSEEK, &in_par.OtrRec));
		in_par.InrRec = in_par.OtrRec;
		(in_par.FileName = path).SetLastSlash().Cat("bnkseek.txt");
		out_par.Direction = 0;
		out_par.DataFormat = PPImpExpParam::dfDbf;
		out_par.TdfParam.Flags |= TextDbFile::fCpOem;
		THROW(LoadSdRecord(PPREC_RBCBNKSEEK, &out_par.OtrRec));
		out_par.InrRec = out_par.OtrRec;
		(out_par.FileName = path).SetLastSlash().Cat("bnkseek.dbf");
		{
			PPImpExp in_file(&in_par, 0);
			PPImpExp out_file(&out_par, 0);
			Sdr_RbcBnkSeek rec;
			THROW(in_file.OpenFileForReading(0));
			THROW(out_file.OpenFileForWriting(0, 1));
			// @v10.7.9 @ctr MEMSZERO(rec);
			while((r = in_file.ReadRecord(&rec, sizeof(rec))) > 0) {
				THROW(out_file.AppendRecord(&rec, sizeof(rec)));
				MEMSZERO(rec);
			}
			THROW(r);
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int GenerateCliBnkImpData()
{
	struct BillEntry {
		enum {
			fDirty = 0x0001 // Элемент уже использован
		};
		LDATE  Dt;        // @anchor По этому полю элементы сортируются //
		PPID   ID;
		PPID   ArID;
		long   Flags;
		char   Code[48];  // @v11.1.12 [24]-->[48]
		double Amount;
	};
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	const  LDATE cd = getcurdate_();
	SString temp_buf;
	SString imp_cfg_name;
	int    max_items = 0;
	double mean = 0.0;
	double stddev = 0.0;
	ClientBankExportDef cbed(0);
	// CliBnkGenParam=cfg_name[,max_items[,mean[,stddev]]]
	// cfg_name - наименование конфигурации импорта клиент-банка
	// max_items - @def=10 максимальное количество генерируемых платежей
	// mean - @def = 25 среднее значение задержки платежа (для генерации статистической задержки от даты документа до сегодня)
	// stddev - @def = 50 стандартное отклонение значений задержки платежа (для генерации статистической задержки от даты документа до сегодня)
	PPObjOprKind op_obj;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	PPIDArray op_list;
	SArray bill_list(sizeof(BillEntry));
	op_obj.GetPayableOpList(-1, &op_list);
	PPViewBill bill_view;
	PPIniFile ini_file;
	int    enbl = 0;
	THROW_PP(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ENABLEBILLGENERATOR, &enbl) && enbl, PPERR_BILLGEN_NOTALLOWED);
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_CLIBNKGENPARAM, temp_buf)) {
		StringSet ss(',', temp_buf);
		uint pos = 0;
		if(ss.get(&pos, temp_buf)) {
			imp_cfg_name = temp_buf;
			if(ss.get(&pos, temp_buf)) {
				max_items = temp_buf.ToLong();
				if(ss.get(&pos, temp_buf)) {
					mean = temp_buf.ToReal();
					if(ss.get(&pos, temp_buf))
						stddev = temp_buf.ToReal();
				}
			}
		}
		SETIFZ(max_items, 10);
		if(mean == 0.0)
			mean = 25.0;
		if(stddev == 0.0)
			stddev = 50.0;
	}
	THROW_PP(imp_cfg_name.NotEmptyS(), PPERR_CLIBNKGEN_CFGNDEF);
	{
		PPImpExpParam & r_param = cbed.GetParam();
		r_param.Direction = 1; // import
		r_param.InrRec.Name = "CLIBNKDATA";
		r_param.ProcessName(1, imp_cfg_name);
	}
	THROW(cbed.ReadDefinition(imp_cfg_name));
	for(uint i = 0; i < op_list.getCount(); i++) {
		BillViewItem bill_item;
		BillFilt bill_filt;
		bill_filt.OpID = op_list.get(i);
		bill_filt.Period.upp = cd;
		bill_filt.Period.low = plusdate(bill_filt.Period.upp, -365*2);
		bill_filt.Flags |= BillFilt::fDebtOnly;
		THROW(bill_view.Init_(&bill_filt));
		for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&bill_item) > 0;) {
			BillEntry entry;
			MEMSZERO(entry);
			entry.Dt = bill_item.Dt;
			entry.ID = bill_item.ID;
			entry.ArID = bill_item.Object;
			STRNSCPY(entry.Code, bill_item.Code);
			entry.Amount = bill_item.Amount;
			THROW_SL(bill_list.insert(&entry));
		}
	}
	bill_list.sort(CMPF_LONG);
	//
	// Список долговых документов составили.
	// Теперь наша задача - сделать случайную выборку с гамма-распределением
	// по периодам от текущего дня до даты документа.
	//
	{
		uint   _count = bill_list.getCount(); // Количество оставшихся не обработанных долговых документов
		PPGPaymentOrderList order_list;
		PPIDArray in_paym_list;
		PPIDArray out_paym_list;
		const  PPID sell_acs_id = GetSellAccSheet();
		const  PPID suppl_acs_id = GetSupplAccSheet();
		double beta = stddev*stddev / mean;
		double alpha = mean / beta;
		SRandGenerator rg;
		rg.Set(getcurtime_().v);
		for(uint i = 0, c = 0; _count > 0 && c < (uint)max_items; i++) {
			long d = R0i(rg.GetGamma(alpha, beta));
			_count = 0;
			int checked = 0;
			for(uint j = 0; j < bill_list.getCount(); j++) {
				BillEntry * p_entry = static_cast<BillEntry *>(bill_list.at(j));
				if(!(p_entry->Flags & BillEntry::fDirty)) {
					_count++;
					if(!checked && diffdate(cd, p_entry->Dt) <= d) {
						double paym = 0.0;
						p_bobj->P_Tbl->CalcPayment(p_entry->ID, 0, 0, 0, &paym);
						if(paym < p_entry->Amount && ar_obj.Fetch(p_entry->ArID, &ar_rec) > 0) {
							if(ar_rec.AccSheetID == sell_acs_id) {
								in_paym_list.add(p_entry->ID);
								c++;
							}
							else if(ar_rec.AccSheetID == suppl_acs_id) {
								out_paym_list.add(p_entry->ID);
								c++;
							}
							checked = 1;
						}
						p_entry->Flags |= BillEntry::fDirty;
					}
				}
			}
		}
		THROW(p_bobj->CreateBankingOrders(in_paym_list, PPObjBill::cboIn, order_list));
		THROW(p_bobj->CreateBankingOrders(out_paym_list, 0, order_list));
		THROW(cbed.CreateOutputFile(0/*pResultFileList*/));
		THROW(cbed.PutHeader());
		for(uint i = 0; i < order_list.getCount(); i++) {
			PPGPaymentOrder * p_order = order_list.at(i);
			PPBillPacket pack;
			(temp_buf = "TEST").Cat(i+1).CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
			pack.Rec.Dt = p_order->Dt;
			pack.Rec.Object = p_order->ArID;
			pack.Rec.Object2 = p_order->Ar2ID;
			pack.Rec.Amount = p_order->Amount;
			pack.P_PaymOrder = new PPBankingOrder;
			*pack.P_PaymOrder = *p_order;
			temp_buf.Z();
			PPID   debt_bill_id = 0;
			if(p_order->LinkBillList.getCount()) {
				debt_bill_id = p_order->LinkBillList.get(0);
				for(uint j = 0; j < p_order->LinkBillList.getCount(); j++) {
					BillTbl::Rec bill_rec;
					if(p_bobj->Search(p_order->LinkBillList.get(j), &bill_rec) > 0) {
						if(temp_buf.IsEmpty())
							temp_buf.Z().Cat("Payment by bill").CatDiv(':', 2);
						else
							temp_buf.CatDiv(',', 2);
						temp_buf.Cat(bill_rec.Code).Space().Cat(bill_rec.Dt);
					}
				}
			}
			// @v11.1.12 temp_buf.CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
			pack.SMemo = temp_buf; // @v11.1.12
			THROW(cbed.PutRecord(&pack, debt_bill_id, 0));
		}
		THROW(cbed.PutEnd());
	}
	CATCHZOKPPERR
	return ok;
}
