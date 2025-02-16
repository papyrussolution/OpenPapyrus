// OBJREG.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2014, 2016, 2017, 2018, 2019, 2020, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
#define USE_EXPERIMENTAL_LAYOUTEDDIALOG  // @v12.2.6 @construction 

#ifdef USE_EXPERIMENTAL_LAYOUTEDDIALOG
class RegisterListDialog2 : public LayoutedListDialog {
	LayoutedListDialog_Base::Param GetListParam()
	{
		LayoutedListDialog_Base::Param p;
		p.Bounds.Set(0, 0, 420, 160);
		p.Title = "@registerdocument_pl";
		p.Flags = LayoutedListDialog_Base::fcedCED|LayoutedListDialog_Base::fHeaderStaticText;
		p.ColumnDescription = "@lbt_reglist";
		p.Symb = "RegisterListDialog2";
		return p;
	}
public:
	RegisterListDialog2(PPPersonPacket * pPsnPack, PPID eventID) :
		LayoutedListDialog(GetListParam(), new StrAssocListBoxDef(new StrAssocArray(), lbtDisposeData|lbtDblClkNotify|lbtFocNotify)), 
		P_PsnPack(0), P_LocPack(0), P_Data(&StubData)
	{
		if(pPsnPack) {
			P_PsnPack = pPsnPack;
			P_Data = &pPsnPack->Regs;
			{
				SString title_buf;
				GetObjectTitle(PPOBJ_PERSON, title_buf).CatDiv(':', 2).Cat(P_PsnPack->Rec.Name);
				setCtrlString(/*CTL_REGLST_LINKOBJTEXT*/STDCTL_HEADERSTATICTEXT, title_buf);
			}
		}
		else
			Oid.Set(PPOBJ_PERSON, 0);
		EventID  = eventID;
		updateList(-1);
	}
	RegisterListDialog2(PPLocationPacket * pLocPack) :
		LayoutedListDialog(GetListParam(), new StrAssocListBoxDef(new StrAssocArray(), lbtDisposeData|lbtDblClkNotify|lbtFocNotify)), 
		P_PsnPack(0), P_LocPack(0), P_Data(&StubData)
	{
		if(pLocPack) {
			Oid.Set(PPOBJ_LOCATION, pLocPack->ID);
			P_LocPack = pLocPack;
			P_Data = &pLocPack->Regs;
			{
				SString title_buf;
				SString name_buf(P_LocPack->Name);
				if(!name_buf.NotEmptyS())
					name_buf = P_LocPack->Code;
				if(!name_buf.NotEmptyS()) {
					LocationCore::GetAddress(*P_LocPack, 0, name_buf);
				}
				GetObjectTitle(PPOBJ_LOCATION, title_buf).CatDiv(':', 2).Cat(name_buf);
				setCtrlString(/*CTL_REGLST_LINKOBJTEXT*/STDCTL_HEADERSTATICTEXT, title_buf);
			}
		}
		else
			Oid.Set(PPOBJ_LOCATION, 0);
		EventID  = 0;
		updateList(-1);
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		PPObjRegisterType rt_obj;
		PPRegisterType    rtrec;
		SString temp_buf;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = P_Data->at(i);
			if(r_reg_rec.RegTypeID != PPREGT_BANKACCOUNT) {
				ss.Z();
				if(rt_obj.Fetch(r_reg_rec.RegTypeID, &rtrec) <= 0)
					ltoa(r_reg_rec.RegTypeID, rtrec.Name, 10);
				ss.add(rtrec.Name);
				ss.add(r_reg_rec.Serial);
				if(r_reg_rec.RegTypeID == PPREGT_TAXSYSTEM) {
					GetObjectName(PPOBJ_TAXSYSTEMKIND, r_reg_rec.ExtID, temp_buf);
					ss.add(temp_buf);
				}
				else
					ss.add(r_reg_rec.Num);
				ss.add(temp_buf.Z().Cat(r_reg_rec.Dt));
				ss.add(temp_buf.Z().Cat(r_reg_rec.Expiry));
				THROW(addStringToList(i+1, ss.getBuf()));
			}
		}
		CATCHZOK
		return ok;
	}
	int Helper_EditItem(RegisterTbl::Rec & rRec)
	{
		return P_LocPack ? RObj.EditDialog(&rRec, P_Data, P_LocPack) : RObj.EditDialog(&rRec, P_Data, P_PsnPack);
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		if(RObj.CheckRights(PPR_INS)) {
			RegisterTbl::Rec rec;
			PPObjRegister::InitPacket(&rec, 0, Oid, 0);
			rec.PsnEventID = EventID;
			while(ok < 0 && Helper_EditItem(rec) > 0)
				if(RObj.CheckUnique(rec.RegTypeID, P_Data))
					if(P_Data->insert(&rec)) {
						const uint new_pos = P_Data->getCount()-1;
						ASSIGN_PTR(pPos, new_pos);
						ASSIGN_PTR(pID, new_pos+1);
						ok = 1;
					}
					else
						ok = PPSetErrorSLib();
				else
					PPError();
		}
		else
			ok = PPErrorZ();
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI())
			ok = Helper_EditItem(P_Data->at((uint)(id-1)));
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			THROW(!P_Data->at((uint)(id-1)).ID || RObj.CheckRights(PPR_DEL));
			if(CONFIRM(PPCFM_DELETE)) {
				P_Data->atFree((uint)(id-1));
				ok = 1;
			}
		}
		CATCHZOKPPERR
		return ok;
	}

	PPObjRegister RObj;
	PPObjID Oid;
	PPID   EventID;
	RegisterArray StubData;
	RegisterArray * P_Data;
	PPPersonPacket * P_PsnPack;
	PPLocationPacket * P_LocPack;
};

#else 

class RegisterListDialog : public PPListDialog {
public:
	RegisterListDialog(PPPersonPacket * pPsnPack, PPID eventID) : PPListDialog(DLG_REGLST, CTL_REGLST_LIST), P_PsnPack(0), P_LocPack(0), P_Data(&StubData)
	{
		if(pPsnPack) {
			P_PsnPack = pPsnPack;
			P_Data = &pPsnPack->Regs;
			{
				SString title_buf;
				GetObjectTitle(PPOBJ_PERSON, title_buf).CatDiv(':', 2).Cat(P_PsnPack->Rec.Name);
				setCtrlString(CTL_REGLST_LINKOBJTEXT, title_buf);
			}
		}
		else
			Oid.Set(PPOBJ_PERSON, 0);
		EventID  = eventID;
		updateList(-1);
	}
	RegisterListDialog(PPLocationPacket * pLocPack) : PPListDialog(DLG_REGLST, CTL_REGLST_LIST), P_PsnPack(0), P_LocPack(0), P_Data(&StubData)
	{
		if(pLocPack) {
			Oid.Set(PPOBJ_LOCATION, pLocPack->ID);
			P_LocPack = pLocPack;
			P_Data = &pLocPack->Regs;
			{
				SString title_buf;
				SString name_buf(P_LocPack->Name);
				if(!name_buf.NotEmptyS())
					name_buf = P_LocPack->Code;
				if(!name_buf.NotEmptyS()) {
					LocationCore::GetAddress(*P_LocPack, 0, name_buf);
				}
				GetObjectTitle(PPOBJ_LOCATION, title_buf).CatDiv(':', 2).Cat(name_buf);
				setCtrlString(CTL_REGLST_LINKOBJTEXT, title_buf);
			}
		}
		else
			Oid.Set(PPOBJ_LOCATION, 0);
		EventID  = 0;
		updateList(-1);
	}
private:
	virtual int setupList()
	{
		int    ok = 1;
		PPObjRegisterType rt_obj;
		PPRegisterType    rtrec;
		SString temp_buf;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = P_Data->at(i);
			if(r_reg_rec.RegTypeID != PPREGT_BANKACCOUNT) {
				ss.Z();
				if(rt_obj.Fetch(r_reg_rec.RegTypeID, &rtrec) <= 0)
					ltoa(r_reg_rec.RegTypeID, rtrec.Name, 10);
				ss.add(rtrec.Name);
				ss.add(r_reg_rec.Serial);
				if(r_reg_rec.RegTypeID == PPREGT_TAXSYSTEM) {
					GetObjectName(PPOBJ_TAXSYSTEMKIND, r_reg_rec.ExtID, temp_buf);
					ss.add(temp_buf);
				}
				else
					ss.add(r_reg_rec.Num);
				ss.add(temp_buf.Z().Cat(r_reg_rec.Dt));
				ss.add(temp_buf.Z().Cat(r_reg_rec.Expiry));
				THROW(addStringToList(i+1, ss.getBuf()));
			}
		}
		CATCHZOK
		return ok;
	}
	int Helper_EditItem(RegisterTbl::Rec & rRec)
	{
		return P_LocPack ? RObj.EditDialog(&rRec, P_Data, P_LocPack) : RObj.EditDialog(&rRec, P_Data, P_PsnPack);
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		if(RObj.CheckRights(PPR_INS)) {
			RegisterTbl::Rec rec;
			PPObjRegister::InitPacket(&rec, 0, Oid, 0);
			rec.PsnEventID = EventID;
			while(ok < 0 && Helper_EditItem(rec) > 0)
				if(RObj.CheckUnique(rec.RegTypeID, P_Data))
					if(P_Data->insert(&rec)) {
						const uint new_pos = P_Data->getCount()-1;
						ASSIGN_PTR(pPos, new_pos);
						ASSIGN_PTR(pID, new_pos+1);
						ok = 1;
					}
					else
						ok = PPSetErrorSLib();
				else
					PPError();
		}
		else
			ok = PPErrorZ();
		return ok;
	}
	virtual int editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI())
			ok = Helper_EditItem(P_Data->at((uint)(id-1)));
		return ok;
	}
	virtual int delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			THROW(!P_Data->at((uint)(id-1)).ID || RObj.CheckRights(PPR_DEL));
			if(CONFIRM(PPCFM_DELETE)) {
				P_Data->atFree((uint)(id-1));
				ok = 1;
			}
		}
		CATCHZOKPPERR
		return ok;
	}

	PPObjRegister RObj;
	PPObjID Oid;
	PPID   EventID;
	RegisterArray StubData;
	RegisterArray * P_Data;
	PPPersonPacket * P_PsnPack;
	PPLocationPacket * P_LocPack;
};
#endif // USE_EXPERIMENTAL_LAYOUTEDDIALOG

TLP_IMPL(PPObjRegister, RegisterCore, P_Tbl);

/*static*/int PPObjRegister::InitPacket(RegisterTbl::Rec * pRec, PPID regTypeID, PPObjID oid, const char * pNumber)
{
	int    ok = 1;
	if(pRec) {
		memzero(pRec, sizeof(*pRec));
		pRec->RegTypeID = regTypeID;
		if(!isempty(pNumber))
			STRNSCPY(pRec->Num, pNumber);
		if(oneof2(oid.Obj, PPOBJ_PERSON, PPOBJ_LOCATION)) {
			pRec->ObjType = oid.Obj;
			pRec->ObjID = oid.Id;
		}
		else if(oid.Obj)
			ok = PPSetError(PPERR_INVREGISTERLINKOBJTYPE);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

PPObjRegister::PPObjRegister(void * extraPtr) : PPObject(PPOBJ_REGISTER), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
}

PPObjRegister::~PPObjRegister()
{
	TLP_CLOSE(P_Tbl);
}

int    PPObjRegister::Search(PPID id, void * b) { return P_Tbl->Search(id, static_cast<RegisterTbl::Rec *>(b)); }
const  char * PPObjRegister::GetNamePtr() { return P_Tbl->data.Num; }
int    PPObjRegister::DeleteObj(PPID id) { return P_Tbl->Remove(id, 0); }

StrAssocArray * PPObjRegister::MakeStrAssocList(void * extraPtr /* (RegisterFilt*) */)
{
	const RegisterFilt * p_filt = static_cast<const RegisterFilt *>(extraPtr);
	StrAssocArray * p_list = new StrAssocArray;
	THROW_MEM(p_list);
	if(p_filt) {
		PPIDArray reg_id_list;
        RegisterTbl::Rec reg_rec;
		P_Tbl->SearchByFilt(p_filt, &reg_id_list, 0);
        for(uint i = 0; i < reg_id_list.getCount(); i++) {
            if(Search(reg_id_list.get(i), &reg_rec) > 0) {
				THROW_SL(p_list->Add(reg_rec.ID, reg_rec.Num));
            }
        }
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjRegister::SearchByNumber(PPID * pID, PPID regTypeID, const char * pSn, const char * pNmbr, RegisterTbl::Rec * pRec)
	{ return P_Tbl->SearchByNumber(pID, regTypeID, pSn, pNmbr, pRec); }
int PPObjRegister::SearchByFilt(const RegisterFilt * pFilt, PPIDArray * pResList, PPIDArray * pObjList)
	{ return P_Tbl->SearchByFilt(pFilt, pResList, pObjList); } // @todo memory leak

int PPObjRegister::CheckUnique(PPID regTypeID, const RegisterArray * pAry) const
{
	int    ok = 1;
	PPRegisterType    rt;
	PPObjRegisterType rt_obj;
	if(rt_obj.Search(regTypeID, &rt) > 0 && rt.Flags & REGTF_UNIQUE)
		for(uint i = 0; ok && i < pAry->getCount(); i++)
			if(pAry->at(i).RegTypeID == regTypeID)
				ok = PPSetError(PPERR_DUPREGISTER);
	return ok;
}

int PPObjRegister::CheckUniqueNumber(const RegisterTbl::Rec * pRec, const RegisterArray * /*pAry*/, PPID objType, PPID objID)
{
	PPID   id = 0;
	RegisterTbl::Rec out_rec;
	if(PPObjRegisterType::IsDupRegType(pRec->RegTypeID))
		return 1;
	else if(P_Tbl->SearchByNumber(&id, pRec->RegTypeID, pRec->Serial, pRec->Num, &out_rec) > 0) {
		if(pRec->ID == 0 || pRec->ID != out_rec.ID) {
			if(out_rec.ObjID) {
				if(objID && out_rec.ObjType == objType && out_rec.ObjID == objID)
					return 1;
				else
					PPSetAddedMsgObjName(out_rec.ObjType, out_rec.ObjID);
			}
			return PPSetErrorPreserveAddendum(PPERR_DUPREGNUMBER);
		}
	}
	return 1;
}

int PPObjRegister::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE)
		if(_obj == PPOBJ_REGISTERTYPE) {
			RegisterTbl::Key3 k3;
			MEMSZERO(k3);
			k3.RegTypeID = _id;
			if(P_Tbl->search(3, &k3, spGe) && P_Tbl->data.RegTypeID == _id)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
			else
				ok = (BTROKORNFOUND) ? DBRPL_OK : PPSetErrorDB();
		}
		else if(_obj == PPOBJ_PERSON) {
			BExtQuery q(P_Tbl, 0, 1);
			q.select(P_Tbl->ID, 0L).where(P_Tbl->RegOrgID == _id);
			if(q.fetchFirst() > 0)
				ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
		}
	return ok;
}

int PPObjRegister::Helper_EditDialog(RegisterTbl::Rec * pRec, const RegisterArray * pRegList, PPID outerObjType, const void * pOuterPacket)
{
	class RegisterDialog : public TDialog {
		DECL_DIALOG_DATA(RegisterTbl::Rec);
	public:
		//
		// ARG(rezID IN): Ид диалога
		// ARG(pRegAry INOUT): @#{vptr0} Список регистров, которому принадлежит редактируемый
		//   (создаваемый) регистр
		//
		RegisterDialog(uint rezID, const RegisterArray * pRegAry) : TDialog(rezID), P_RegAry(pRegAry), P_PsnPack(0), P_LocPack(0)
		{
			Init();
		}
		RegisterDialog(uint rezID, const PPPersonPacket * pPsnPack) : TDialog(rezID), P_RegAry(0), P_PsnPack(pPsnPack), P_LocPack(0)
		{
			if(P_PsnPack)
				P_RegAry = &P_PsnPack->Regs;
			Init();
		}
		RegisterDialog(uint rezID, const PPLocationPacket * pLocPack) : TDialog(rezID), P_RegAry(0), P_PsnPack(0), P_LocPack(pLocPack)
		{
			if(P_LocPack)
				P_RegAry = &P_LocPack->Regs;
			Init();
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			setupRegOrgKind(0);
			SString obj_name;
			if(P_PsnPack)
				obj_name = P_PsnPack->Rec.Name;
			else if(P_LocPack) {
				obj_name = P_LocPack->Name;
				if(!obj_name.NotEmptyS())
					obj_name = P_LocPack->Code;
			}
			else if(Data.ObjID) {
				if(Data.ObjType == PPOBJ_PERSON)
					GetPersonName(Data.ObjID, obj_name);
				else if(Data.ObjType == PPOBJ_LOCATION)
					GetLocationName(Data.ObjID, obj_name);
			}
			setCtrlString(CTL_REG_PNAME, obj_name);
			setCtrlReadOnly(CTL_REG_PNAME, 1);
			SetupPPObjCombo(this, CTLSEL_REG_REGTYP, PPOBJ_REGISTERTYPE, Data.RegTypeID, OLW_CANINSERT, 0);
			if(Data.RegTypeID) {
				PPRegisterType    rt;
				PPObjRegisterType rt_obj;
				if(rt_obj.Fetch(Data.RegTypeID, &rt) > 0 && rt.Flags & REGTF_ONLYNUMBER)
					selectCtrl(CTL_REG_NUMBER);
				else
					selectCtrl(CTL_REG_DATE);
				disableCtrl(CTLSEL_REG_REGTYP, 1);
				ValidateNumber();
			}
			setCtrlData(CTL_REG_DATE,     &Data.Dt);
			setCtrlData(CTL_REG_EXPIRY,   &Data.Expiry);
			setCtrlData(CTL_REG_SERIALNO, Data.Serial);
			setCtrlData(CTL_REG_NUMBER,   Data.Num);
			setCtrlLong(CTL_REG_ID, Data.ID);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_REG_REGTYP, &Data.RegTypeID);
			getCtrlData(CTL_REG_DATE,     &Data.Dt);
			getCtrlData(CTL_REG_EXPIRY,   &Data.Expiry);
			getCtrlData(CTL_REG_SERIALNO, Data.Serial);
			getCtrlData(CTL_REG_NUMBER,   Data.Num);
			if(Data.RegTypeID == PPREGT_TAXSYSTEM)
				getCtrlData(CTLSEL_REG_ORGAN, &Data.ExtID);
			else
				getCtrlData(CTLSEL_REG_ORGAN, &Data.RegOrgID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		void   Init()
		{
			MEMSZERO(Data);
			RegOrgKind = 0;
			ValidCode = -1;
			SetupCalDate(CTLCAL_REG_DATE, CTL_REG_DATE);
			SetupCalDate(CTLCAL_REG_EXPIRY, CTL_REG_EXPIRY);
			Ptb.SetBrush(brushValidNumber,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushInvalidNumber, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
		}
		int  ValidateNumber()
		{
			SString temp_buf;
			getCtrlString(CTL_REG_NUMBER, temp_buf);
			int    prev = ValidCode;
			//
			STokenRecognizer tr;
			SNaturalTokenStat nts;
			SNaturalTokenArray nta;
			tr.Run(temp_buf, nta.Z(), &nts); 
			//
			if(Data.RegTypeID == PPREGT_TPID) {
				ValidCode = (nta.Has(SNTOK_RU_INN) > 0.0f);
			}
			else if(Data.RegTypeID == PPREGT_KPP) {
				ValidCode = (nta.Has(SNTOK_RU_KPP) > 0.0f);
			}
			else if(Data.RegTypeID == PPREGT_OKPO) {
				ValidCode = (nta.Has(SNTOK_RU_OKPO) > 0.0f);
			}
			else if(Data.RegTypeID == PPREGT_BIC) {
				ValidCode = (nta.Has(SNTOK_RU_BIC) > 0.0f); 
			}
			else if(Data.RegTypeID == PPREGT_BNKCORRACC) {
				SString bic;
				if(P_RegAry)
					P_RegAry->GetRegNumber(PPREGT_BIC, bic);
				else if(P_PsnPack)
					P_PsnPack->GetRegNumber(PPREGT_BIC, bic);
				ValidCode = CheckCorrAcc(temp_buf, bic);
			}
			return (ValidCode != prev);
		}
		enum {
			dummyFirst = 1,
			brushValidNumber,
			brushInvalidNumber
		};
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND) {
				if(event.isCbSelected(CTLSEL_REG_REGTYP)) {
					PPID   preserve_reg_type_id = Data.RegTypeID;
					getCtrlData(CTLSEL_REG_REGTYP, &Data.RegTypeID);
					if(Data.RegTypeID && setupRegOrgKind(1) == 0) {
						setCtrlLong(CTLSEL_REG_REGTYP, preserve_reg_type_id);
						PPError();
					}
					else {
						PPRegisterType    rt;
						PPObjRegisterType rt_obj;
						SString numb;
						getCtrlString(CTL_REG_NUMBER, numb);
						if(!numb.NotEmptyS()) {
							PPObjRegisterType rt_obj;
							if(rt_obj.GetCode(Data.RegTypeID, 0, Data.Num, sizeof(Data.Num)) > 0)
								setCtrlData(CTL_REG_NUMBER, Data.Num);
						}
						if(Data.RegTypeID && rt_obj.Fetch(Data.RegTypeID, &rt) > 0) {
							if(P_LocPack && !(rt.Flags & REGTF_LOCATION)) {
								setCtrlLong(CTLSEL_REG_REGTYP, preserve_reg_type_id);
								PPError(PPERR_REGTYPENOTFORLOC, rt.Name);
							}
							else {
								if(rt.ExpiryPeriod > 0 && Data.ID == 0) {
									Data.Dt = getcurdate_();
									Data.Expiry = plusdate(Data.Dt, rt.ExpiryPeriod);
									setCtrlDate(CTL_REG_DATE, Data.Dt);
									setCtrlDate(CTL_REG_EXPIRY, Data.Expiry);
								}
								if(rt.Flags & REGTF_ONLYNUMBER)
									selectCtrl(CTL_REG_NUMBER);
							}
						}
					}
					ValidateNumber();
				}
				else if(TVCMD == cmInputUpdated) {
					uint   ctl_id = event.getCtlID();
					if(oneof2(ctl_id, CTL_REG_NUMBER, CTL_REG_SERIALNO)) {
						SString data_buf, temp_buf;
						getCtrlString(ctl_id, data_buf);
						if(data_buf.NotEmptyS())
							temp_buf.Cat(data_buf.Len());
						setStaticText((ctl_id == CTL_REG_NUMBER) ? CTL_REG_NUMLEN : CTL_REG_SERLEN, temp_buf);
						if(ctl_id == CTL_REG_NUMBER && ValidateNumber())
							drawCtrl(CTL_REG_NUMBER);
					}
					else
						return;
				}
				else if(TVCMD == cmCtlColor) {
					TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
					if(p_dc && ValidCode >= 0 && getCtrlHandle(CTL_REG_NUMBER) == p_dc->H_Ctl) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get((ValidCode > 0) ? brushValidNumber : brushInvalidNumber));
					}
					else
						return;
				}
				else
					return;
				clearEvent(event);
			}
		}
		int    setupRegOrgKind(int checkRegType)
		{
			int    ok = 1;
			PPObjRegisterType rt_obj;
			const bool is_taxsystem = (Data.RegTypeID == PPREGT_TAXSYSTEM);
			SString temp_buf;
			if(Data.RegTypeID) {
				PPRegisterType rt;
				if(rt_obj.Search(Data.RegTypeID, &rt) > 0) {
					RegOrgKind = rt.RegOrgKind;
					disableCtrl(CTL_REG_NUMBER, is_taxsystem);
					disableCtrl(CTL_REG_SERIALNO, is_taxsystem);
					if(checkRegType && Data.ObjType == PPOBJ_PERSON && Data.ObjID) {
						PPObjPerson pobj;
						if(rt.PersonKindID) {
							int r = 1;
							if(P_PsnPack) {
								if(!P_PsnPack->Kinds.lsearch(rt.PersonKindID))
									r = 0;
							}
							else if(!pobj.P_Tbl->IsBelongsToKind(Data.ObjID, rt.PersonKindID))
								r = 0;
							if(!r) {
								ok = PPSetError(PPERR_REGTPERSON);
								Data.RegTypeID = 0;
								RegOrgKind   = 0;
							}
						}
						long f = CheckXORFlags(rt.Flags, REGTF_PRIVATE, REGTF_LEGAL);
						if(ok && f) {
							PPID status = 0;
							int  priv   = -1;
							if(P_PsnPack) {
								PPObjPersonStatus ps_obj;
								PPPersonStatus ps_rec;
								if(ps_obj.Fetch(P_PsnPack->Rec.Status, &ps_rec) > 0)
									priv = BIN(ps_rec.Flags & PSNSTF_PRIVATE);
							}
							else
								pobj.GetStatus(Data.ObjID, &status, &priv);
							if(priv >= 0) {
								if((f == REGTF_PRIVATE && priv == 0) || (f == REGTF_LEGAL && priv > 0)) {
									ok = PPSetError(PPERR_REGTPERSON);
									Data.RegTypeID = 0;
									RegOrgKind   = 0;
								}
							}
						}
						if(ok && P_RegAry) {
							PPObjRegister reg_obj;
							ok = reg_obj.CheckUnique(Data.RegTypeID, P_RegAry);
						}
					}
				}
				else {
					Data.RegTypeID = 0;
					RegOrgKind   = 0;
				}
			}
			temp_buf.Z();
			if(is_taxsystem) {
				PPLoadString("register_taxsystem", temp_buf);
				disableCtrl(CTLSEL_REG_ORGAN, 0);
				SetupPPObjCombo(this, CTLSEL_REG_ORGAN, PPOBJ_TAXSYSTEMKIND, Data.ExtID, OLW_CANINSERT, 0);
			}
			else {
				PPLoadString("register_regorg", temp_buf);
				disableCtrl(CTLSEL_REG_ORGAN, !Data.RegTypeID);
				SetupPPObjCombo(this, CTLSEL_REG_ORGAN, PPOBJ_PERSON, Data.RegOrgID, OLW_CANINSERT, reinterpret_cast<void *>(RegOrgKind));
			}
			setLabelText(CTL_REG_ORGAN, temp_buf);
			return ok;
		}
		const RegisterArray * P_RegAry;
		const PPPersonPacket * P_PsnPack;
		const PPLocationPacket * P_LocPack;
		PPID   RegOrgKind;
		int    ValidCode;
		SPaintToolBox Ptb;
	};
	int    ok = -1;
	RegisterDialog * dlg = 0;
	const PPPersonPacket * p_psn_pack = 0;
	const PPLocationPacket * p_loc_pack = 0;
	if(outerObjType == PPOBJ_PERSON && pOuterPacket) {
		p_psn_pack = static_cast<const PPPersonPacket *>(pOuterPacket);
		pRegList = &p_psn_pack->Regs;
		dlg = new RegisterDialog(DLG_REGISTER, p_psn_pack);
	}
	else if(outerObjType == PPOBJ_LOCATION && pOuterPacket) {
		p_loc_pack = static_cast<const PPLocationPacket *>(pOuterPacket);
		pRegList = &p_loc_pack->Regs;
		dlg = new RegisterDialog(DLG_REGISTER, p_loc_pack);
	}
	else
		dlg = new RegisterDialog(DLG_REGISTER, pRegList);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(pRec);
		if(!CheckRightsModByID(&pRec->ID))
			DisableOKButton(dlg);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(pRec)) {
				if(CheckUniqueNumber(pRec, pRegList, /*NZOR(outerObjType, PPOBJ_PERSON)*/pRec->ObjType, pRec->ObjID))
					ok = valid_data = 1;
				else
					PPError();
			}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjRegister::EditDialog(RegisterTbl::Rec * pRec, const RegisterArray * pRegList, const PPPersonPacket * pOuterPack)
	{ return Helper_EditDialog(pRec, pRegList, PPOBJ_PERSON, pOuterPack); }
int PPObjRegister::EditDialog(RegisterTbl::Rec * pRec, const RegisterArray * pRegList, const PPLocationPacket * pOuterPack)
	{ return Helper_EditDialog(pRec, pRegList, PPOBJ_LOCATION, pOuterPack); }

int PPObjRegister::Edit(PPID * pID, PPID objType, PPID objID, PPID regTypeID)
{
	int    r = cmCancel;
	long   counter = 0;
	PPID   local_obj_type = 0;
	RegisterTbl::Rec rec;
	PPObjRegisterType rt_obj;
	PPPersonPacket psn_pack;
	PPLocationPacket loc_pack;
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
		if(rec.ObjID) {
			local_obj_type = (oneof2(rec.ObjType, PPOBJ_PERSON, PPOBJ_LOCATION)) ? rec.ObjType : PPOBJ_PERSON;
		}
	}
	else {
		PPObjRegister::InitPacket(&rec, regTypeID, PPObjID(objType, objID), 0);
		if(regTypeID)
			rt_obj.GetCode(regTypeID, &counter, rec.Num, sizeof(rec.Num));
		local_obj_type = (oneof2(rec.ObjType, PPOBJ_PERSON, PPOBJ_LOCATION)) ? rec.ObjType : PPOBJ_PERSON;
	}
	if(local_obj_type && objID) {
        if(local_obj_type == PPOBJ_PERSON) {
        	PPObjPerson psn_obj;
			if(psn_obj.GetPacket(objID, &psn_pack, 0) <= 0)
				psn_pack.Rec.ID = 0;
        }
        else if(local_obj_type == PPOBJ_LOCATION) {
        	PPObjLocation loc_obj;
			if(loc_obj.GetPacket(objID, &loc_pack) <= 0)
				loc_pack.ID = 0;
        }
	}
	if(local_obj_type == PPOBJ_LOCATION && loc_pack.ID) {
		r = EditDialog(&rec, 0, &loc_pack);
	}
	else {
		r = EditDialog(&rec, 0, (psn_pack.Rec.ID ? &psn_pack : static_cast<const PPPersonPacket *>(0)));
	}
	if(r > 0) {
		THROW((*pID) ? P_Tbl->Update(*pID, &rec, 1) : P_Tbl->Add(pID, &rec, 1));
	}
	else if(counter)
		rt_obj.UngetCounter(regTypeID, counter);
	CATCH
		r = PPErrorZ();
	ENDCATCH
	return r;
}

int PPObjRegister::Edit(PPID * pID, void * extraPtr /*personID*/)
{
	const  PPID extra_person_id = reinterpret_cast<PPID>(extraPtr);
	return Edit(pID, PPOBJ_PERSON, extra_person_id, 0);
}

int PPObjRegister::EditList(PPPersonPacket * pPsnPack, PPID psnEventID)
{
#ifdef USE_EXPERIMENTAL_LAYOUTEDDIALOG
	RegisterListDialog2 * dlg = new RegisterListDialog2(pPsnPack, psnEventID);
#else
	RegisterListDialog * dlg = new RegisterListDialog(pPsnPack, psnEventID);
#endif
	return CheckDialogPtrErr(&dlg) ? ((ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1) : 0;
}

int PPObjRegister::EditList(PPLocationPacket * pLocPack)
{
#ifdef USE_EXPERIMENTAL_LAYOUTEDDIALOG
	RegisterListDialog2 * dlg = new RegisterListDialog2(pLocPack);
#else
	RegisterListDialog * dlg = new RegisterListDialog(pLocPack);
#endif
	return CheckDialogPtrErr(&dlg) ? ((ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1) : 0;
}

int PPObjRegister::EditBankAccount(PPBankAccount * pRec, PPID psnKindID)
{
	class BankAccountDialog : public TDialog {
	public:
		BankAccountDialog() : TDialog(DLG_BACCT), ValidAcc(-1)
		{
			SetupCalDate(CTLCAL_BACCT_OPENDATE, CTL_BACCT_OPENDATE);
			Ptb.SetBrush(brushValidNumber,   SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushInvalidNumber, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
		}
		void   SetupBIC()
		{
			BIC.Z();
			PPID   bank_id = getCtrlLong(CTLSEL_BACCT_BANK);
			if(bank_id) {
				PPObjPerson psn_obj;
				RegisterTbl::Rec reg_rec;
				if(psn_obj.GetRegister(bank_id, PPREGT_BIC, &reg_rec) > 0)
					BIC = reg_rec.Num;
			}
			setCtrlString(CTL_BACCT_BIC, BIC);
			SString data_buf;
			getCtrlString(CTL_BACCT_ACCT, data_buf);
			ValidAcc = BIN(CheckBnkAcc(data_buf, BIC));
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_BACCT_BANK)) {
				SetupBIC();
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_BACCT_ACCT)) {
				SString data_buf, temp_buf;
				getCtrlString(CTL_BACCT_ACCT, data_buf);
				setStaticText(CTL_BACCT_ACCTLEN, temp_buf.Cat(data_buf.Strip().Len()));
				int    prev_valid = ValidAcc;
				ValidAcc = BIN(CheckBnkAcc(data_buf, BIC));
				if(ValidAcc != prev_valid)
					drawCtrl(CTL_BACCT_ACCT);
			}
			else if(TVCMD == cmCtlColor) {
				TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
				if(p_dc && ValidAcc >= 0 && getCtrlHandle(CTL_BACCT_ACCT) == p_dc->H_Ctl) {
					if(ValidAcc > 0) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushValidNumber));
					}
					else {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushInvalidNumber));
					}
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		enum {
			dummyFirst = 1,
			brushValidNumber,
			brushInvalidNumber
		};
		int    ValidAcc;
		SString BIC;
		SPaintToolBox Ptb;
	};
	int    ok = -1;
	int    valid_data = 0;
	PPBankAccount rec = *pRec;
	BankAccountDialog * dlg = new BankAccountDialog();
	if(CheckDialogPtrErr(&dlg)) {
		SetupPersonCombo(dlg, CTLSEL_BACCT_BANK, rec.BankID, OLW_CANINSERT, (PPID)PPPRK_BANK, 0);
		SetupPPObjCombo(dlg, CTLSEL_BACCT_ACCTYPE, PPOBJ_BNKACCTYPE, rec.AccType, OLW_CANINSERT, 0);
		dlg->setCtrlData(CTL_BACCT_ACCT,     rec.Acct);
		dlg->setCtrlData(CTL_BACCT_OPENDATE, &rec.OpenDate);
		dlg->AddClusterAssoc(CTL_BACCT_FLAGS, 0, PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/);
		dlg->SetClusterData(CTL_BACCT_FLAGS, rec.Flags);
		dlg->SetupBIC();
		{
			if(!CheckRightsModByID(&rec.ID))
				DisableOKButton(dlg);
		}
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTLSEL_BACCT_BANK,    &rec.BankID);
			dlg->getCtrlData(CTLSEL_BACCT_ACCTYPE, &rec.AccType);
			dlg->getCtrlData(CTL_BACCT_ACCT,       rec.Acct);
			dlg->getCtrlData(CTL_BACCT_OPENDATE,   &rec.OpenDate);
			dlg->GetClusterData(CTL_BACCT_FLAGS,   &rec.Flags);
			if(rec.BankID == 0)
				PPErrorByDialog(dlg, CTL_BACCT_BANK, PPERR_BANKNEEDED);
			else if(rec.AccType == 0)
				PPErrorByDialog(dlg, CTL_BACCT_ACCTYPE, PPERR_BACCTYPENEEDED);
			else if((rec.AccType == PPBAC_NOSTRO || rec.AccType == PPBAC_LORO) && psnKindID && psnKindID != PPPRK_BANK)
				PPErrorByDialog(dlg, CTL_BACCT_ACCTYPE, PPERR_INCOMPBACCTYPE);
			else {
				*pRec = rec;
				ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
// BankAccountListDialog
//
class BankAccountListDialog2 : public LayoutedListDialog {
	LayoutedListDialog_Base::Param GetListParam()
	{
		LayoutedListDialog_Base::Param p;
		p.Bounds.Set(0, 0, 420, 160);
		p.Title = "@bankaccount_pl";
		p.Flags = LayoutedListDialog_Base::fcedCED|LayoutedListDialog_Base::fHeaderStaticText;
		p.ColumnDescription = "@lbt_bankacclist";
		p.Symb = "BankAccountListDialog2";
		return p;
	}
public:
	BankAccountListDialog2(PPPersonPacket * pPsnPack) : LayoutedListDialog(GetListParam(), new StrAssocListBoxDef(new StrAssocArray(), lbtDisposeData|lbtDblClkNotify|lbtFocNotify)),
		P_PsnPack(0), P_Data(&StubData)
	{
		if(pPsnPack) {
			P_PsnPack = pPsnPack;
			P_Data = &pPsnPack->Regs;
			setStaticText(STDCTL_HEADERSTATICTEXT, P_PsnPack->Rec.Name);
		}
		// see the comment to the function int moveItem(long, long, int) below
		enableCommand(cmUp, 0);
		enableCommand(cmDown, 0);
		updateList(-1);
	}
private:
	virtual int  setupList()
	{
		int    ok = 1;
		SString sub;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = P_Data->at(i);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba(r_reg_rec);
				ss.Z();
				sub.Z();
				GetPersonName(ba.BankID, sub);
				ss.add(sub);
				ss.add(ba.Acct);
				THROW(addStringToList(i+1, ss.getBuf()));
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		if(RObj.CheckRights(PPR_INS)) {
			PPBankAccount ba_rec;
			PersonTbl::Rec psn_rec;
			ba_rec.AccType = PPBAC_CURRENT;
			while(ok < 0 && RObj.EditBankAccount(&ba_rec, 0) > 0) {
				if(P_Data->CheckDuplicateBankAccount(&ba_rec, -1)) {
					if(P_Data->SetBankAccount(&ba_rec, static_cast<uint>(-1))) {
						ASSIGN_PTR(pID, P_Data->getCount());
						ok = 1;
					}
					else
						PPError();
				}
				else
					PPError();
			}
		}
		else
			ok = PPErrorZ();
		return ok;
	}
	virtual int  editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba_rec(r_reg_rec);
				while(ok < 0 && RObj.EditBankAccount(&ba_rec, 0) > 0) {
					if(P_Data->CheckDuplicateBankAccount(&ba_rec, /*pos*/(id-1))) {
						if(P_Data->SetBankAccount(&ba_rec, (uint)(id-1))) {
							ok = 1;
						}
						else
							PPError();
					}
					else
						PPError();
				}
			}
		}
		return ok;
	}
	virtual int  delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
                PPBankAccount ba_rec(r_reg_rec);
				THROW(!ba_rec.ID || RObj.CheckRights(PPR_DEL));
				if(CONFIRM(PPCFM_DELETE)) {
					THROW(P_Data->SetBankAccount(0, (uint)(id-1)));
					ok = 1;
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
	// @v10.9.9 {
	// Попытка реализовать ручное изменение порядка следования счетов оказалась неудачной:
	// в базе данных регистры, хранящие данные о счетах игнорируют это изменение порядка при обновлении 
	// списка записей. В конструкторе мы запретили кнопки Up и Donw, сам же код пока оставим - может пригодится.
	//
	virtual int moveItem(long pos, long id, int up)
	{
		int    ok = -1;
		if(P_Data) {
			const long _c = P_Data->getCountI();
			if(id > 0 && id <= _c) {
				RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
				if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
					const long this_idx = id-1;
					if(up) {
						long idx = this_idx;
						if(idx > 0) do {
							const RegisterTbl::Rec & r_reg_rec = P_Data->at(--idx);
							if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
								if(P_Data->swap(this_idx, idx)) {
									ok = 1;
								}
								break;
							}
						} while(idx > 0);
					}
					else {
						long idx = this_idx;
						if(idx >= 0 && idx < (_c-1)) do {
							const RegisterTbl::Rec & r_reg_rec = P_Data->at(++idx);
							if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
								if(P_Data->swap(this_idx, idx)) {
									ok = 1;
								}
								break;
							}
						} while(idx < (_c-1));
					}
				}
			}
		}
		return ok;
	}
	// } @v10.9.9 
	PPObjPerson  PsnObj;
	PPObjRegister RObj;
	RegisterArray StubData;
	RegisterArray * P_Data;
	PPPersonPacket * P_PsnPack;
};

class BankAccountListDialog : public PPListDialog {
public:
	BankAccountListDialog(PPPersonPacket * pPsnPack) : PPListDialog(DLG_BACCLST, CTL_BACCLST_LIST), P_PsnPack(0), P_Data(&StubData)
	{
		if(pPsnPack) {
			P_PsnPack = pPsnPack;
			P_Data = &pPsnPack->Regs;
			setStaticText(CTL_BACCLST_NAME, P_PsnPack->Rec.Name);
		}
		// see the comment to the function int moveItem(long, long, int) below
		enableCommand(cmUp, 0);
		enableCommand(cmDown, 0);
		updateList(-1);
	}
private:
	virtual int  setupList()
	{
		int    ok = 1;
		SString sub;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; i < P_Data->getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = P_Data->at(i);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba(r_reg_rec);
				ss.Z();
				sub.Z();
				GetPersonName(ba.BankID, sub);
				ss.add(sub);
				ss.add(ba.Acct);
				THROW(addStringToList(i+1, ss.getBuf()));
			}
		}
		CATCHZOK
		return ok;
	}
	virtual int addItem(long * pPos, long * pID)
	{
		int    ok = -1;
		if(RObj.CheckRights(PPR_INS)) {
			PPBankAccount ba_rec;
			PersonTbl::Rec psn_rec;
			ba_rec.AccType = PPBAC_CURRENT;
			while(ok < 0 && RObj.EditBankAccount(&ba_rec, 0) > 0) {
				if(P_Data->CheckDuplicateBankAccount(&ba_rec, -1)) {
					if(P_Data->SetBankAccount(&ba_rec, static_cast<uint>(-1))) {
						ASSIGN_PTR(pID, P_Data->getCount());
						ok = 1;
					}
					else
						PPError();
				}
				else
					PPError();
			}
		}
		else
			ok = PPErrorZ();
		return ok;
	}
	virtual int  editItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba_rec(r_reg_rec);
				while(ok < 0 && RObj.EditBankAccount(&ba_rec, 0) > 0) {
					if(P_Data->CheckDuplicateBankAccount(&ba_rec, /*pos*/(id-1))) {
						if(P_Data->SetBankAccount(&ba_rec, (uint)(id-1))) {
							ok = 1;
						}
						else
							PPError();
					}
					else
						PPError();
				}
			}
		}
		return ok;
	}
	virtual int  delItem(long pos, long id)
	{
		int    ok = -1;
		if(id > 0 && id <= P_Data->getCountI()) {
			RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
                PPBankAccount ba_rec(r_reg_rec);
				THROW(!ba_rec.ID || RObj.CheckRights(PPR_DEL));
				if(CONFIRM(PPCFM_DELETE)) {
					THROW(P_Data->SetBankAccount(0, (uint)(id-1)));
					ok = 1;
				}
			}
		}
		CATCHZOKPPERR
		return ok;
	}
	// @v10.9.9 {
	// Попытка реализовать ручное изменение порядка следования счетов оказалась неудачной:
	// в базе данных регистры, хранящие данные о счетах игнорируют это изменение порядка при обновлении 
	// списка записей. В конструкторе мы запретили кнопки Up и Donw, сам же код пока оставим - может пригодится.
	//
	virtual int moveItem(long pos, long id, int up)
	{
		int    ok = -1;
		if(P_Data) {
			const long _c = P_Data->getCountI();
			if(id > 0 && id <= _c) {
				RegisterTbl::Rec & r_reg_rec = P_Data->at(id-1);
				if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
					const long this_idx = id-1;
					if(up) {
						long idx = this_idx;
						if(idx > 0) do {
							const RegisterTbl::Rec & r_reg_rec = P_Data->at(--idx);
							if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
								if(P_Data->swap(this_idx, idx)) {
									ok = 1;
								}
								break;
							}
						} while(idx > 0);
					}
					else {
						long idx = this_idx;
						if(idx >= 0 && idx < (_c-1)) do {
							const RegisterTbl::Rec & r_reg_rec = P_Data->at(++idx);
							if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
								if(P_Data->swap(this_idx, idx)) {
									ok = 1;
								}
								break;
							}
						} while(idx < (_c-1));
					}
				}
			}
		}
		return ok;
	}
	// } @v10.9.9 
	PPObjPerson  PsnObj;
	PPObjRegister RObj;
	RegisterArray StubData;
	RegisterArray * P_Data;
	PPPersonPacket * P_PsnPack;
};

int PPObjRegister::EditBankAccountList(PPPersonPacket * pPsnPack)
{
	int    ok = -1;
#ifdef USE_EXPERIMENTAL_LAYOUTEDDIALOG
	BankAccountListDialog2 * dlg = new BankAccountListDialog2(pPsnPack);
#else
	BankAccountListDialog * dlg = new BankAccountListDialog(pPsnPack);
#endif
	if(CheckDialogPtrErr(&dlg))
		ok = (ExecViewAndDestroy(dlg) == cmOK) ? 1 : -1;
	else
		ok = 0;
	return ok;
}

int PPObjRegister::GetBankAccountList(PPID personID, TSVector <PPBankAccount> * pList)
{
	CALLPTRMEMB(pList, clear());
	int   ok = -1;
	RegisterArray  reg_ary;
	if(P_Tbl->GetByPerson(personID, &reg_ary) > 0) {
        for(uint i = 0; i < reg_ary.getCount(); i++) {
			const RegisterTbl::Rec & r_reg_rec = reg_ary.at(i);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba(r_reg_rec);
				ok = 1;
				if(pList) {
					if(ba.Flags & PREGF_BACC_PREFERRED/*BACCTF_PREFERRED*/) {
						THROW_SL(pList->atInsert(0, &ba));
					}
					else {
						THROW_SL(pList->insert(&ba));
					}
				}
				else
					break;
			}
        }
	}
	CATCHZOK
	return ok;
}
/* =====
		 Переменные, используемые в шаблоне наименования регистра

@regname Наименование типа регистра
@regsn   Серия регистра
@regno   Номер регистра
@date    Дата регистра
@expiry  Дата окончания срока действия регистра
@regorg  Регистрирующий орган

===== */
int PPObjRegister::Format(PPID id, const char * pFormat, char * pBuf, size_t bufLen)
{
	SString temp_buf;
	int    ok = Format(id, pFormat, temp_buf);
	temp_buf.CopyTo(pBuf, bufLen);
	return ok;
}

/*static*/int PPObjRegister::Format(const RegisterTbl::Rec & rRec, const char * pFormat, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString intr_fmt, temp_buf;
	PPSymbTranslator st;
	if(pFormat == 0) {
		PPObjRegisterType rt_obj;
		rt_obj.GetFormat(rRec.RegTypeID, intr_fmt);
		pFormat = intr_fmt;
	}
	for(const char * p = pFormat; *p;) {
		if(*p == '@') {
			size_t next = 1;
			long   sym  = st.Translate(p, &next);
			switch(sym) {
				case PPSYM_DATE:    rBuf.Cat(rRec.Dt, DATF_DMY); break;
				case PPSYM_EXPIRY:  rBuf.Cat(rRec.Expiry, DATF_DMY); break;
				case PPSYM_REGSN:   rBuf.Cat(rRec.Serial);       break;
				case PPSYM_REGNO:   rBuf.Cat(rRec.Num);          break;
				case PPSYM_REGNAM:
					GetRegisterTypeName(rRec.RegTypeID, temp_buf);
					rBuf.Cat(temp_buf);
					break;
				case PPSYM_REGORG:
					GetPersonName(rRec.RegOrgID, temp_buf);
					rBuf.Cat(temp_buf);
					break;
			}
			p += next;
		}
		else
			rBuf.CatChar(*p++);
	}
	return ok;
}

int PPObjRegister::Format(PPID id, const char * pFormat, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	RegisterTbl::Rec rec;
	if(id < 0 || (id > 0 && Search(id, &rec) > 0)) {
		ok = PPObjRegister::Format(((id < 0) ? P_Tbl->data : rec), pFormat, rBuf);
	}
	else
		ok = -1;
	return ok;
}

int PPObjRegister::GetTabNumber(PPID personID, SString & rTabNum)
{
	int   ok = -1;
	PPPersonKind    psn_kind;
	PPObjPersonKind pk_obj;
	rTabNum.Z();
	if(pk_obj.Fetch(PPPRK_EMPL, &psn_kind) > 0 && psn_kind.CodeRegTypeID) {
		RegisterArray  reg_ary;
		if(P_Tbl->GetByPerson(personID, &reg_ary) > 0) {
			LDATE  cur_date = getcurdate_();
			RegisterTbl::Rec  reg_rec;
			for(uint pos = 0; ok < 0 && reg_ary.GetRegister(psn_kind.CodeRegTypeID, &pos, &reg_rec) > 0;) {
				DateRange  reg_action;
				reg_action.Set(reg_rec.Dt, reg_rec.Expiry);
				if(reg_action.CheckDate(cur_date)) {
					rTabNum = reg_rec.Num;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPObjRegister::PreventDup(RegisterTbl::Rec & rRec, PPID linkObjType, PPID linkObjID)
{
	int    ok = 1;
	if(!PPObjRegisterType::IsDupRegType(rRec.RegTypeID)) {
		RegisterTbl::Rec rec;
		PPID   reg_id = 0;
		long   c = 0;
		char   serial[32];
		char * p = rRec.Serial;
		if(*p == '#')
			do {
				p++;
			} while(isdec(*p));
		strip(STRNSCPY(serial, p));
		while(SearchByNumber(&reg_id, rRec.RegTypeID, rRec.Serial, rRec.Num, &rec) > 0) {
			int    skip = 0;
			if(rec.ID == rRec.ID)
				skip = 1;
			else if(linkObjID) {
				if(linkObjType == PPOBJ_PERSONEVENT && rec.PsnEventID == linkObjID)
					skip = 1;
				else if(linkObjType == PPOBJ_PERSON && rec.ObjID == linkObjID)
					skip = 1;
				else if(linkObjType == PPOBJ_LOCATION && rec.ObjID == linkObjID)
					skip = 1;
			}
			if(!skip) {
				char   temp_buf[48];
				p = temp_buf;
				*p++ = '#';
				ltoa(++c, p, 10);
				p += sstrlen(p);
				if(serial[0]) {
					*p++ = ' ';
					strcpy(p, serial);
				}
				STRNSCPY(rRec.Serial, temp_buf);
			}
			else
				break;
		}
	}
	return ok;
}
//
//
//
class RegisterCache : public ObjCacheHash {
public:
	struct RegisterData : public ObjCacheEntry {
		long   ObjType;
		long   ObjID;
		long   PsnEventID;
		long   RegTypeID;
		LDATE  Dt;
		long   RegOrgID;
		//string Serial[12];
		//string Num[32];
		LDATE  Expiry;
		long   Flags;
	};
	RegisterCache() : ObjCacheHash(PPOBJ_REGISTER, sizeof(RegisterData), SKILOBYTE(512), 4)
	{
	}
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int RegisterCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	RegisterData * p_cache_rec = static_cast<RegisterData *>(pEntry);
	PPObjRegister reg_obj;
	RegisterTbl::Rec rec;
	if(reg_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(ObjType);
		CPY_FLD(ObjID);
		CPY_FLD(PsnEventID);
		CPY_FLD(RegTypeID);
		CPY_FLD(Dt);
		CPY_FLD(RegOrgID);
		CPY_FLD(Expiry);
		CPY_FLD(Flags);
#undef CPY_FLD
		MultTextBlock b;
		b.Add(rec.Serial);
		b.Add(rec.Num);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void RegisterCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	RegisterTbl::Rec * p_data_rec = static_cast<RegisterTbl::Rec *>(pDataRec);
	const RegisterData * p_cache_rec = static_cast<const RegisterData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	CPY_FLD(ID);
	CPY_FLD(ObjType);
	CPY_FLD(ObjID);
	CPY_FLD(PsnEventID);
	CPY_FLD(RegTypeID);
	CPY_FLD(Dt);
	CPY_FLD(RegOrgID);
	CPY_FLD(Expiry);
	CPY_FLD(Flags);
#undef CPY_FLD
	{
		MultTextBlock b(this, pEntry);
		b.Get(p_data_rec->Serial, sizeof(p_data_rec->Serial));
		b.Get(p_data_rec->Num, sizeof(p_data_rec->Num));
	}
}

int PPObjRegister::Fetch(PPID id, RegisterTbl::Rec * pRec)
{
	RegisterCache * p_cache = GetDbLocalCachePtr <RegisterCache> (PPOBJ_REGISTER);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}
//
// Implementation of PPALDD_PersonRegister
//
struct DlPersonRegisterBlock {
	DlPersonRegisterBlock()
	{
	}
	PPObjRegister RegObj;
    RegisterTbl::Rec Rec;
};

PPALDD_CONSTRUCTOR(PersonRegister)
{
	Extra[0].Ptr = new DlPersonRegisterBlock;
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(PersonRegister)
{
	Destroy();
	delete static_cast<DlPersonRegisterBlock *>(Extra[0].Ptr);
}

int PPALDD_PersonRegister::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		DlPersonRegisterBlock * p_blk = static_cast<DlPersonRegisterBlock *>(Extra[0].Ptr);
		if(p_blk->RegObj.Search(rFilt.ID, &p_blk->Rec) > 0) {
			H.ID = p_blk->Rec.ID;
			if(p_blk->Rec.ObjType == PPOBJ_PERSON)
				H.PersonID  = p_blk->Rec.ObjID;
			else if(p_blk->Rec.ObjType == PPOBJ_LOCATION)
				H.LocID = p_blk->Rec.ObjID;
			H.RegTypeID = p_blk->Rec.RegTypeID;
			H.RegOrgID  = p_blk->Rec.RegOrgID;
			H.EventID   = p_blk->Rec.PsnEventID;
			H.Dt        = p_blk->Rec.Dt;
			H.Expiry    = p_blk->Rec.Expiry;
			H.Flags     = p_blk->Rec.Flags;
			STRNSCPY(H.Serial, p_blk->Rec.Serial);
			STRNSCPY(H.Number, p_blk->Rec.Num);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_PersonRegister::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n)))
	#define _ARG_DATE(n) (*static_cast<const LDATE *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?Format") {
		_RET_STR.Z();
		DlPersonRegisterBlock * p_blk = static_cast<DlPersonRegisterBlock *>(Extra[0].Ptr);
		if(p_blk && p_blk->Rec.ID) {
			PPObjRegister::Format(p_blk->Rec, 0, _RET_STR);
		}
	}
}
