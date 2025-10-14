// OBJARTCL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
int STDCALL SetupArCombo(TDialog * dlg, uint ctlID, PPID id, uint flags, PPID _accSheetID, /*int disableIfZeroSheet*/long sacf)
{
	int    ok = 1;
	int    create_ctl_grp = 0;
	ArticleFilt filt;
	filt.AccSheetID = _accSheetID;
	filt.Ft_Closed = -1;
	if(_accSheetID > 0 && id) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec ar_rec;
		if(ar_obj.Fetch(id, &ar_rec) > 0 && ar_rec.Closed) {
			filt.Ft_Closed = 0;
		}
	}
	if(sacf & sacfNonEmptyExchageParam)
		filt.Flags |= ArticleFilt::fWithIxParamOnly;
	if(sacf & sacfNonGeneric)
		filt.Flags |= ArticleFilt::fNonGenericOnly;
	if(/*disableIfZeroSheet*/sacf & sacfDisableIfZeroSheet)
		dlg->disableCtrl(ctlID, _accSheetID == 0);
	if(_accSheetID) {
		ArticleCtrlGroup * p_grp = static_cast<ArticleCtrlGroup *>(dlg->getGroup(ctlID));
		if(p_grp) {
			p_grp->SetAccSheet(_accSheetID);
		}
		else {
			p_grp = new ArticleCtrlGroup(0, 0, ctlID, 0, _accSheetID);
			dlg->addGroup(ctlID, p_grp);
		}
		ok = SetupPPObjCombo(dlg, ctlID, PPOBJ_ARTICLE, id, flags, /*(void *)accSheetID*/&filt);
		if(ok)
			dlg->SetupWordSelector(ctlID, new PersonSelExtra(_accSheetID, 0), id, 2, 0);
	}
	else
		dlg->setCtrlLong(ctlID, 0);
	return ok;
}

int FASTCALL GetArticleSheetID(PPID arID, PPID * pAccSheetID, PPID * pLnkObjID)
{
	int    ok = -1;
	PPID   acc_sheet_id = 0, lnk_obj_id = 0;
	PPObjArticle ar_obj;
	ArticleTbl::Rec rec;
	if(arID && (ok = ar_obj.Fetch(arID, &rec)) > 0) {
		acc_sheet_id = rec.AccSheetID;
		lnk_obj_id = rec.ObjID;
	}
	ASSIGN_PTR(pAccSheetID, acc_sheet_id);
	ASSIGN_PTR(pLnkObjID, lnk_obj_id);
	return ok;
}

PPID FASTCALL ObjectToPerson(PPID objID, PPID * pAccSheetID)
{
	PPID   result_id = 0;
	PPID   acc_sheet_id = 0;
	PPID   lnk_obj_id = 0;
	if(objID && GetArticleSheetID(objID, &acc_sheet_id, &lnk_obj_id) > 0) {
		PPObjAccSheet acc_sheet_obj;
		PPAccSheet acs_rec;
		result_id = (acc_sheet_obj.Fetch(acc_sheet_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) ? lnk_obj_id : 0;
	}
	ASSIGN_PTR(pAccSheetID, acc_sheet_id);
	return result_id;
}

int FASTCALL GetArticleName(PPID arID, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(arID) {
		PPObjArticle ar_obj;
		ArticleTbl::Rec rec;
		if((ok = ar_obj.Fetch(arID, &rec)) > 0) {
			rBuf = rec.Name;
			ok = 1;
		}
		else
			ideqvalstr(arID, rBuf);
	}
	return ok;
}

int GetArticleText(PPID arID, PPArticleType artyp, SString & rBuf)
{
	int    ok = -1;
	if(arID) {
		SString ar_name;
		PPGetSubStr(PPTXT_ARTYPE, ((int)artyp) - 1, rBuf);
		GetArticleName(arID, ar_name);
		rBuf.CatDivIfNotEmpty(':', 2).Cat(ar_name);
		ok = 1;
	}
	else
		rBuf.Z();
	return ok;
}

int GetSupplText(PPID supplID, SString & rBuf)
{
	return GetArticleText(supplID, artypSuppl, rBuf);
}
//
//
//
PPArticlePacket::PPArticlePacket()
{
	THISZERO();
}

PPArticlePacket::~PPArticlePacket()
{
	delete P_CliAgt;
	delete P_SupplAgt;
	delete P_AliasSubst;
}

PPArticlePacket & FASTCALL PPArticlePacket::operator = (const PPArticlePacket & s)
{
	Rec = s.Rec;
	Assoc = s.Assoc;
	DontUpdateAliasSubst = s.DontUpdateAliasSubst;
	SetClientAgreement(s.P_CliAgt, 0);
	SetSupplAgreement(s.P_SupplAgt);
	ZDELETE(P_AliasSubst);
	if(s.P_AliasSubst)
		P_AliasSubst = new LAssocArray(*s.P_AliasSubst);
	return *this;
}

int PPObjArticle::IsPacketEq(const PPArticlePacket & rS1, const PPArticlePacket & rS2, long options)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rS1.Rec.m, rS2.Rec.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMB(AccSheetID);
	CMP_MEMB(Article);
	CMP_MEMB(ObjID);
	CMP_MEMBS(Name);
	CMP_MEMB(AccessLevel);
	CMP_MEMB(Closed);
	CMP_MEMB(Flags);
#undef CMP_MEMBS
#undef CMP_MEMB
	if(rS1.Assoc != rS2.Assoc)
		return 0;
	if(!BOOLXOR(rS1.P_CliAgt, rS2.P_CliAgt)) return 0;
	if(rS1.P_CliAgt && rS2.P_CliAgt && !rS1.P_CliAgt->IsEq(*rS2.P_CliAgt)) return 0;
	if(!BOOLXOR(rS1.P_SupplAgt, rS2.P_SupplAgt)) return 0;
	if(rS1.P_SupplAgt && rS2.P_SupplAgt && !rS1.P_SupplAgt->IsEq(*rS2.P_SupplAgt)) return 0;
	if(!(options & peoDontCmpAliasSubst)) { // @v8.1.3
		if(!BOOLXOR(rS1.P_AliasSubst, rS2.P_AliasSubst)) return 0;
		if(rS1.P_AliasSubst && rS2.P_AliasSubst && !(*rS1.P_AliasSubst == *rS2.P_AliasSubst)) return 0;
	}
	return 1;
}

void PPArticlePacket::Init()
{
	ZDELETE(P_CliAgt);
	ZDELETE(P_SupplAgt);
	ZDELETE(P_AliasSubst);
	THISZERO();
}

int PPArticlePacket::SetClientAgreement(const PPClientAgreement * pAgt, int ignoreEmpty)
{
	int    ok = 1;
	ZDELETE(P_CliAgt);
	if(pAgt && (!ignoreEmpty || !pAgt->IsEmpty())) {
		P_CliAgt = new PPClientAgreement(*pAgt);
		if(!P_CliAgt)
			ok = PPSetErrorNoMem();
	}
	return ok;
}

int PPArticlePacket::SetSupplAgreement(const PPSupplAgreement * pAgt, int ignoreEmpty)
{
	int    ok = 1;
	ZDELETE(P_SupplAgt);
	if(pAgt && (!ignoreEmpty || !pAgt->IsEmpty())) {
		P_SupplAgt = new PPSupplAgreement(*pAgt);
		if(!P_SupplAgt)
			ok = PPSetErrorNoMem();
	}
	return ok;
}

const LAssocArray * PPArticlePacket::GetAliasSubst() const
{
	return P_AliasSubst;
}

int PPArticlePacket::EnumAliasSubst(uint * pPos, PPID * pAliasID, PPID * pAccID) const
{
	int    ok = 0;
	if(P_AliasSubst) {
		LAssoc * p_item;
		if(P_AliasSubst->enumItems(pPos, (void **)&p_item)) {
			ASSIGN_PTR(pAliasID, p_item->Key);
			ASSIGN_PTR(pAccID, p_item->Val);
			ok = 1;
		}
	}
	return ok;
}

int PPArticlePacket::AddAliasSubst(PPID accAliasID, PPID accID)
{
	SETIFZ(P_AliasSubst, new LAssocArray);
	return P_AliasSubst->AddUnique(accAliasID, accID, 0) ? 1 : PPSetError(PPERR_DUPSUBSTALIASONARTICLE);
}

int PPArticlePacket::UpdateAliasSubst(PPID accAliasID, PPID accID)
{
	if(P_AliasSubst && P_AliasSubst->Search(accAliasID, 0)) {
		uint   cnt = 0;
		for(uint i = 0; i < P_AliasSubst->getCount(); i++)
			if(P_AliasSubst->at(i).Key == accAliasID)
				cnt++;
		if(cnt > 1)
			return PPSetError(PPERR_DUPSUBSTALIASONARTICLE);
		else {
			P_AliasSubst->Update(accAliasID, accID);
			return 1;
		}
	}
	else
		return -1;
}

int PPArticlePacket::RemoveAliasSubst(PPID accAliasID)
{
	return (P_AliasSubst && P_AliasSubst->Remove(accAliasID)) ? 1 : -1;
}
//
//
//
struct ArticleDlgData : public PPArticlePacket {
	ArticleDlgData() : PPArticlePacket(), Options(0)
	{
	}
	enum {
		fDisableName     = 0x0001,
		fNewAgreement    = 0x0002,
		fAssocAccnt      = 0x0004,
		fAllowUpdLinkObj = 0x0008
	};
	long   Options;
};

class ArticleAutoAddDialog : public TDialog {
public:
	ArticleAutoAddDialog(long _sheetID) : TDialog(DLG_ARTICLEAUTO), P_Query(0), P_Buf(0), IsFound(1), Ta(0), OnExitResult(0)
	{
		init(_sheetID);
	}
	~ArticleAutoAddDialog()
	{
		delete P_Buf;
		delete P_Query;
	}
	int    GetResult() const { return OnExitResult; }
private:
	void   init(PPID sheetID);
	DECL_HANDLE_EVENT;
	int    fetch(int);
	int    GetNext();
	int    save();
	int    makeQuery();
	int    extractFromQuery();
	int    MakeCadidateList(StrAssocArray & rList);

	int    Ta;
	int    OnExitResult;
	int    IsFound;
	char * P_Buf;
	//PPID   Assoc;
	//PPID   GroupID;
	DBQuery * P_Query;
	PPAccSheet AcsRec;
 	PPObjArticle ArObj;
	ArticleTbl::Rec Rec;
	StrAssocArray CandidateList;
};

int ArticleAutoAddDialog::MakeCadidateList(StrAssocArray & rList)
{
	rList.Z();
	int    ok = 1;
	switch(AcsRec.Assoc) {
		case PPOBJ_PERSON:
			if(AcsRec.ObjGroup) {
				PPObjPerson psn_obj;
				psn_obj.GetListByKind(AcsRec.ObjGroup, 0, &rList);
			}
			break;
		case PPOBJ_LOCATION:
			{
				LocationFilt filt;
				filt.LocType = LOCTYP_WAREHOUSE;
				//if(AcsRec.WarehouseGroup)
					//filt.Parent = AcsRec.WarehouseGroup;
				PPObjLocation loc_obj;
				loc_obj.GetList(&filt, 0, rList);
				if(AcsRec.WarehouseGroup) {
					uint i = rList.getCount();
					if(i) do {
						StrAssocArray::Item item = rList.Get(--i);
						if(loc_obj.IsMemberOfGroup(item.Id, AcsRec.WarehouseGroup) > 0) {
							;
						}
						else {
							rList.AtFree(i);
						}
					} while(i);
				}
			}
			break;
		case PPOBJ_PROCESSOR:
			{
				PPObjProcessor prc_obj;
				prc_obj.GetList(0, rList);
			}
			break;
		case PPOBJ_ACCOUNT2:
			{
				PPObjAccount acc_obj;
				StrAssocArray * p_temp_list = acc_obj.MakeStrAssocList(0);
				if(p_temp_list) {
					rList = *p_temp_list;
					ZDELETE(p_temp_list);
				}
			}
			break;
		case PPOBJ_GLOBALUSERACC:
			{
				PPObjGlobalUserAcc gua_obj;
				StrAssocArray * p_temp_list = gua_obj.MakeStrAssocList(0);
				if(p_temp_list) {
					rList = *p_temp_list;
					ZDELETE(p_temp_list);
				}
			}
			break;
	}
	{
		uint i = rList.getCount();
		if(i) do {
			StrAssocArray::Item item = rList.Get(--i);
			if(ArObj.P_Tbl->SearchObjRef(Rec.AccSheetID, item.Id) > 0) {
				rList.AtFree(i);
			}
			else {
				;
			}
		} while(i);
		rList.setPointer(0);
	}
	return ok;
}

int ArticleAutoAddDialog::makeQuery()
{
	int    ok = 1;
	LocationTbl  * loc_tbl  = 0;
	ProcessorTbl * prc_tbl = 0;
	PersonKindTbl * k  = 0;
	if(AcsRec.Assoc == PPOBJ_PERSON) {
		THROW(CheckTblPtr(k = new PersonKindTbl));
		P_Query = &::select(k->PersonID, k->Name, 0L).from(k, 0L).
			where(k->KindID == AcsRec.ObjGroup).orderBy(k->Name, 0L);
	}
	else if(AcsRec.Assoc == PPOBJ_LOCATION) {
		THROW(CheckTblPtr(loc_tbl = new LocationTbl));
		P_Query = &::select(loc_tbl->ID, loc_tbl->Name, 0L).from(loc_tbl, 0L).
			where(loc_tbl->Type == LOCTYP_WAREHOUSE).orderBy(loc_tbl->ParentID, loc_tbl->Name, 0L);
	}
	else if(AcsRec.Assoc == PPOBJ_PROCESSOR) { // @v11.3.12
		THROW(CheckTblPtr(prc_tbl = new ProcessorTbl));
		P_Query = &::select(prc_tbl->ID, prc_tbl->Name, 0L).from(prc_tbl, 0L).
			where(prc_tbl->Kind == static_cast<long>(PPPRCK_PROCESSOR)).orderBy(prc_tbl->Name, 0L);
	}
	else
		CALLEXCEPT_PP(PPERR_INVACCSHEETASSOC);
	THROW_MEM(P_Query);
	THROW_PP(!P_Query->error, PPERR_DBQUERY);
	THROW_MEM(P_Buf = new char[sizeof(PPID) + 128]);
	CATCH
		ok = 0;
		if(P_Query)
			ZDELETE(P_Query);
		else {
			delete k;
			delete loc_tbl;
			delete prc_tbl;
		}
	ENDCATCH
	return ok;
}

int ArticleAutoAddDialog::extractFromQuery()
{
	int    ok = 1;
	STRNSCPY(Rec.Name, P_Buf + sizeof(PPID));
	Rec.ObjID = *reinterpret_cast<const  PPID *>(P_Buf);
	int    r = ArObj.P_Tbl->SearchObjRef(Rec.AccSheetID, Rec.ObjID);
	THROW(r);
	if(r < 0) {
		Rec.ID = 0;
		Rec.Article = 0;
		THROW(ArObj.GetFreeArticle(&Rec.Article, Rec.AccSheetID));
		setCtrlData(CTL_ARTICLE_NAME, P_Buf + sizeof(PPID));
		getCtrlView(CTL_ARTICLE_NAME)->Draw_();
		setCtrlData(CTL_ARTICLE_NUMBER, &Rec.Article);
		getCtrlView(CTL_ARTICLE_NUMBER)->Draw_();
	}
	CATCHZOK
	return ok ? -r : 0;
}

int ArticleAutoAddDialog::fetch(int sp)
{
	int    r = -1;
	if(P_Query->fetch(1, P_Buf, sp)) {
		while((r = extractFromQuery()) < 0 && P_Query->fetch(1, P_Buf, spNext))
			;
	}
	if(P_Query->error)
		r = PPSetError(PPERR_DBQUERY);
	if(r <= 0 && sp == spNext)
		endModal(r ? cmOK : cmError);
	return r;
}

int ArticleAutoAddDialog::GetNext()
{
	int    ok = -1;
	if(CandidateList.getPointer() < CandidateList.getCount()) {
		//int    ok = 1;
		StrAssocArray::Item item = CandidateList.Get(CandidateList.getPointer());
		CandidateList.incPointer();
		PPID   obj_id = item.Id;
		SString obj_name(item.Txt);
		STRNSCPY(Rec.Name, obj_name);
		Rec.ObjID = obj_id;
		int    r = ArObj.P_Tbl->SearchObjRef(Rec.AccSheetID, Rec.ObjID);
		THROW(r);
		if(r < 0) {
			Rec.ID = 0;
			Rec.Article = 0;
			THROW(ArObj.GetFreeArticle(&Rec.Article, Rec.AccSheetID));
			setCtrlString(CTL_ARTICLE_NAME, obj_name);
			getCtrlView(CTL_ARTICLE_NAME)->Draw_();
			setCtrlData(CTL_ARTICLE_NUMBER, &Rec.Article);
			getCtrlView(CTL_ARTICLE_NUMBER)->Draw_();
			ok = 1;
		}
		//CATCHZOK
		//return ok ? -r : 0;
	}
	else {
		endModal(cmOK);
	}
	CATCHZOK
	return ok;
}

int ArticleAutoAddDialog::save()
{
	long   n = getCtrlLong(CTL_ARTICLE_NUMBER);
	const  int  r = ArObj.GetFreeArticle(&n, Rec.AccSheetID);
	if(r > 0)
		Rec.Article = n;
	else if(!r)
		return (endModal(cmError), 0);
	return ArObj.P_Tbl->insertRecBuf(&Rec) ? 1 : (endModal(cmError), 0);
}

void ArticleAutoAddDialog::init(PPID sheetID)
{
	//int    r;
	//PPAccSheet acs_rec;
	THROW(SearchObject(PPOBJ_ACCSHEET, sheetID, &AcsRec) > 0);
	Rec.Clear();
	Rec.AccSheetID = sheetID;
	THROW(ArObj.GetFreeArticle(&Rec.Article, sheetID));
	//Assoc = acs_rec.Assoc;
	//GroupID = acs_rec.ObjGroup;
	{
		MakeCadidateList(CandidateList);
		if(GetNext()) {
			THROW(PPStartTransaction(&Ta, 1));
			OnExitResult = ExecView(this);
			SETIFZ(OnExitResult, cmCancel); // @v12.1.7
			THROW(OnExitResult != cmError);
			THROW(PPCommitWork(&Ta));
		}
		else {
			IsFound = 0;
			OnExitResult = cmCancel;
		}
	}
	/*
	THROW(makeQuery());
	THROW((r = fetch(spFirst)) != 0);
	if(r > 0) {
		THROW(PPStartTransaction(&Ta, 1));
		THROW((Ret = ExecView(this)) != cmError);
		THROW(PPCommitWork(&Ta));
	}
	else {
		IsFound = 0;
		Ret = cmCancel;
	}
	*/
	CATCH
		PPRollbackWork(&Ta);
		IsFound = 0;
		PPError();
	ENDCATCH
}

IMPL_HANDLE_EVENT(ArticleAutoAddDialog)
{
	int    clear = 1;
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCancel:
				endModal(cmCancel);
				return; // После endModal не следует обращаться к this
			case cmOK:
				if(save() <= 0)
					break;
			case cmaSkip:
				//fetch(spNext);
				GetNext();
				break;
			case cmaAll:
				while(save() > 0 && /*fetch(spNext)*/GetNext() > 0)
					;
				break;
			default:
				clear = 0;
				break;
		}
	}
	else
		clear = 0;
	if(clear)
		clearEvent(event);
	else
		TDialog::handleEvent(event);
}
//
//
//
static int EditAliasSubst(const PPArticlePacket * pPack, LAssoc * pData)
{
	#define GRP_ALS 1
	#define GRP_ACC 2

	class SubstAliasDialog : public TDialog {
		DECL_DIALOG_DATA(LAssoc);
	public:
		SubstAliasDialog(const PPArticlePacket * pPack) : TDialog(DLG_ALSSUBST), P_Pack(pPack)
		{
			addGroup(GRP_ALS, new AcctCtrlGroup(CTL_ALSSUBST_ALS, 0, CTLSEL_ALSSUBST_ALSNAME, 0));
			addGroup(GRP_ACC, new AcctCtrlGroup(CTL_ALSSUBST_ACC, 0, CTLSEL_ALSSUBST_ACCNAME, 0));
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			{
				AcctCtrlGroup::Rec acc_rec;
				acc_rec.AcctId.ac = Data.Key;
				acc_rec.AccSheetID = P_Pack->Rec.AccSheetID;
				acc_rec.AccSelParam = ACY_SEL_ALIAS;
				setGroupData(GRP_ALS, &acc_rec);
			}
			{
				AcctCtrlGroup::Rec acc_rec;
				acc_rec.AcctId.ac = Data.Val;
				acc_rec.AccSheetID = P_Pack->Rec.AccSheetID;
				//
				// Если необходимо, чтобы в качестве подставляемого счета можно было выбрать только
				// счет, имеющий таблицу статей, совпадающую с той, которой принадлежит эта статья,
				// то следует использовать конструкцию (1000+P_Pack->Rec.AccSheetID) вместо ACY_SEL_BALOBAL.
				//
				acc_rec.AccSelParam = ACY_SEL_BALOBAL;
				setGroupData(GRP_ACC, &acc_rec);
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			AcctCtrlGroup::Rec acc_rec;
			getGroupData(GRP_ALS, &acc_rec);
			Data.Key = acc_rec.AcctId.ac;
			sel = CTL_ALSSUBST_ALS;
			THROW_PP(Data.Key, PPERR_ACCALIASNEEDED);
			getGroupData(GRP_ACC, &acc_rec);
			Data.Val = acc_rec.AcctId.ac;
			sel = CTL_ALSSUBST_ACC;
			THROW_PP(Data.Val, PPERR_ACCNEEDED);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		const PPArticlePacket * P_Pack;
	};
	DIALOG_PROC_BODY_P1(SubstAliasDialog, pPack, pData);
}
//
//
//
#define GRP_ASSCACC 1

class ArticleDialog : public PPListDialog {
public:
	ArticleDialog(uint rezID, ArticleDlgData * aData) : PPListDialog(rezID, CTL_ARTICLE_ALIASSUBST), P_Data(aData), AccSheetFounded(0), AgtFlags(0)
	{
		PPObjArticle arobj;
		SetEmptyAgreementInd();
		if(aData->Options & ArticleDlgData::fAssocAccnt) {
			AcctCtrlGroup::Rec acc_rec;
			AcctCtrlGroup * p_ac_grp = new AcctCtrlGroup(CTL_ARTICLE_ACC, 0, CTLSEL_ARTICLE_ACCNAME, 0);
			addGroup(GRP_ASSCACC, p_ac_grp);
			acc_rec.AcctId.ac   = P_Data->Rec.ObjID;
			setGroupData(GRP_ASSCACC, &acc_rec);
		}
		if(SearchObject(PPOBJ_ACCSHEET, P_Data->Rec.AccSheetID, &AccSheetRec) > 0) {
			if(P_Data->Rec.AccSheetID == GetSupplAccSheet() && AccSheetRec.Flags & ACSHF_USESUPPLAGT)
				AgtFlags |= ACSHF_USESUPPLAGT;
			else if(P_Data->Rec.AccSheetID == GetSellAccSheet() || AccSheetRec.Flags & ACSHF_USECLIAGT)
				AgtFlags |= ACSHF_USECLIAGT;
			AccSheetFounded = 1;
		}
		setCtrlData(CTL_ARTICLE_NUMBER, &P_Data->Rec.Article);
		setCtrlData(CTL_ARTICLE_ACCESS, &P_Data->Rec.AccessLevel);
		setCtrlUInt16(CTL_ARTICLE_CLOSED, BIN(P_Data->Rec.Closed));

		AddClusterAssoc(CTL_ARTICLE_FLAGS, 0, ARTRF_STOPBILL);
		SetClusterData(CTL_ARTICLE_FLAGS, P_Data->Rec.Flags);

		setCtrlData(CTL_ARTICLE_NAME, P_Data->Rec.Name);
		setCtrlReadOnly(CTL_ARTICLE_NAME, LOGIC(P_Data->Options & ArticleDlgData::fDisableName));
		if(!P_Data->Rec.ObjID || !P_Data->Rec.AccSheetID || !AccSheetFounded)
			enableCommand(cmaMore, 0);
		enableCommand(cmAgreement, AgtFlags & (ACSHF_USESUPPLAGT|ACSHF_USECLIAGT));
		enableCommand(cmClearAgreement, (AgtFlags & (ACSHF_USESUPPLAGT|ACSHF_USECLIAGT)) && arobj.CheckRights(ARTRT_CLIAGT));
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmaMore))
			editObject();
		else if(event.isCmd(cmAgreement))
			editClientAgreement();
		else if(event.isCmd(cmClearAgreement) && P_Data && CONFIRMCRIT(PPCFM_DELARTICLEAGREEMENT)) {
			if(P_Data->P_CliAgt && P_Data->P_CliAgt->ClientID)
				ArObj.PutClientAgreement(P_Data->P_CliAgt->ClientID, 0, 1);
			ZDELETE(P_Data->P_CliAgt);
			if(P_Data->P_SupplAgt && P_Data->P_SupplAgt->SupplID)
				ArObj.PutSupplAgreement(P_Data->P_SupplAgt->SupplID, 0, 1);
			ZDELETE(P_Data->P_SupplAgt);
			SetEmptyAgreementInd();
		}
		else if(event.isCbSelected(CTLSEL_ARTICLE_LINKOBJ)) {
			PPID    obj_id = getCtrlLong(CTLSEL_ARTICLE_LINKOBJ);
			SString name;
			if(obj_id) {
				getCtrlString(CTL_ARTICLE_NAME, name);
				if(name.Strip().IsEmpty()) {
					if(GetObjectName(AccSheetRec.Assoc, obj_id, name) > 0)
						setCtrlString(CTL_ARTICLE_NAME, name);
				}
			}
		}
		else
			return;
		clearEvent(event);
	}
	virtual int setupList();
	virtual int addItem(long * pos, long * id);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	void   editObject();
	void   editClientAgreement();
	void   SetEmptyAgreementInd();

	ArticleDlgData * P_Data;
	int    AccSheetFounded;
	long   AgtFlags;
	PPAccSheet AccSheetRec;
	PPObjArticle ArObj;
};

int ArticleDialog::setupList()
{
	PPID   alias_id = 0, acc_id = 0;
	SString sub;
	PPObjAccount acc_obj;
	for(uint i = 0; P_Data->EnumAliasSubst(&i, &alias_id, &acc_id) > 0;) {
		PPAccount acc_rec;
		StringSet ss(SLBColumnDelim);
		if(acc_obj.Search(alias_id, &acc_rec) > 0)
			sub = acc_rec.Name;
		else
			ideqvalstr(alias_id, sub);
		ss.add(sub);
		if(acc_obj.Search(acc_id, &acc_rec) > 0)
			(sub = acc_rec.Code).CatDiv('-', 1).Cat(acc_rec.Name);
		else
			ideqvalstr(acc_id, sub);
		ss.add(sub);
		if(!addStringToList(alias_id, ss.getBuf()))
			return 0;
	}
	return 1;
}

int ArticleDialog::addItem(long * /*pPos*/, long * pID)
{
	LAssoc alias_subst;
	if(EditAliasSubst(P_Data, &alias_subst) > 0) {
		if(P_Data->AddAliasSubst(alias_subst.Key, alias_subst.Val)) {
			*pID = alias_subst.Key;
			return 1;
		}
		else
			return PPErrorZ();
	}
	else
		return -1;
}

int ArticleDialog::editItem(long pos, long /*id*/)
{
	LAssoc alias_subst;
	uint   p = static_cast<uint>(pos);
	if(P_Data->EnumAliasSubst(&p, &alias_subst.Key, &alias_subst.Val) > 0)
		if(EditAliasSubst(P_Data, &alias_subst) > 0)
			return P_Data->UpdateAliasSubst(alias_subst.Key, alias_subst.Val) ? 1 : PPErrorZ();
	return -1;
}

int ArticleDialog::delItem(long pos, long id)
{
	P_Data->RemoveAliasSubst(id);
	return 1;
}

void ArticleDialog::editObject()
{
	if(P_Data->Rec.AccSheetID && AccSheetFounded && AccSheetRec.Assoc) {
		if(EditPPObj(AccSheetRec.Assoc, P_Data->Rec.ObjID) > 0) {
			SString obj_name;
			if(GetObjectName(AccSheetRec.Assoc, P_Data->Rec.ObjID, obj_name) > 0)
				setCtrlString(CTL_ARTICLE_NAME, obj_name);
		}
	}
}

void ArticleDialog::editClientAgreement()
{
	int    ok = -1;
	int    agt_kind = -1;
	THROW(agt_kind = PPObjArticle::GetAgreementKind(&P_Data->Rec));
	if(agt_kind > 0) {
		if(agt_kind == 1) {
			PPClientAgreement cli_agt_rec;
			if(P_Data->P_CliAgt)
				cli_agt_rec = *P_Data->P_CliAgt;
			else {
				THROW(ArObj.CheckRights(ARTRT_CLIAGT));
			}
			cli_agt_rec.ClientID = P_Data->Rec.ID;
			if(ArObj.EditClientAgreement(&cli_agt_rec) > 0) {
				if(ArObj.CheckRights(ARTRT_CLIAGT)) {
					P_Data->SetClientAgreement(&cli_agt_rec, 1);
					ok = 1;
				}
			}
		}
		else if(agt_kind == 2) {
			PPSupplAgreement spl_agt_rec;
			if(P_Data->P_SupplAgt)
				spl_agt_rec = *P_Data->P_SupplAgt;
			else {
				THROW(ArObj.CheckRights(ARTRT_CLIAGT));
			}
			spl_agt_rec.SupplID = P_Data->Rec.ID;
			if(ArObj.EditSupplAgreement(&spl_agt_rec) > 0) {
				if(ArObj.CheckRights(ARTRT_CLIAGT)) {
					P_Data->SetSupplAgreement(&spl_agt_rec);
					ok = 1;
				}
			}
		}
		if(ok > 0)
			SetEmptyAgreementInd();
	}
	CATCHZOKPPERR
}

void ArticleDialog::SetEmptyAgreementInd()
{
	uint   bmp_id = IDB_RED;
	if(P_Data) {
		const  PPID ar_id = P_Data->Rec.ID;
		if((P_Data->P_CliAgt && ArObj.HasClientAgreement(ar_id)) || (P_Data->P_SupplAgt && ArObj.HasSupplAgreement(ar_id)))
			bmp_id = IDB_GREEN;
	}
	SetCtrlBitmap(CTL_ARTICLE_AGTISEMPTY, bmp_id);
}
//
//
//
int PPObjArticle::EditGrpArticle(PPID * pID, PPID sheetID)
{
	class GrpArticleDialog : public TDialog {
	public:
		GrpArticleDialog() : TDialog(DLG_ARTICLEGROUP), Data(0L, (void *)0, 0)
		{
		}
		~GrpArticleDialog()
		{
			ZDELETE(Data.P_List);
		}
		int    setDTS(const ArticleTbl::Rec * pRec, const PPIDArray * pAry)
		{
			int    ok = 1;
			Rec = *pRec;
			Data.P_List = pAry ? new PPIDArray(*pAry) : new PPIDArray;
			THROW_MEM(Data.P_List);
			setCtrlData(CTL_ARTICLE_NUMBER, &Rec.Article);
			setCtrlData(CTL_ARTICLE_ACCESS, &Rec.AccessLevel);
			setCtrlUInt16(CTL_ARTICLE_CLOSED, BIN(Rec.Closed));
			setCtrlData(CTL_ARTICLE_NAME,   Rec.Name);
			enableCommand(cmAgreement, 0);
			enableCommand(cmaMore,  1);
			CATCHZOK
			return ok;
		}
		int    getDTS(ArticleTbl::Rec * pRec, PPIDArray * pAry)
		{
			int    ok = 0, r = 1;
			PPObjArticle arobj;
			long   new_art_no = Rec.Article;
			getCtrlData(CTL_ARTICLE_NUMBER, &new_art_no);
			getCtrlData(CTL_ARTICLE_ACCESS, &Rec.AccessLevel);
			Rec.Closed = BIN(getCtrlUInt16(CTL_ARTICLE_CLOSED));
			getCtrlData(CTL_ARTICLE_NAME,   Rec.Name);
			if(*strip(Rec.Name) == 0)
				PPErrorByDialog(this, CTL_ARTICLE_NAME, PPERR_NAMENEEDED);
			else if(new_art_no != Rec.Article && (r = arobj.GetFreeArticle(&new_art_no, Rec.AccSheetID)) > 0) {
				setCtrlData(CTL_ARTICLE_NUMBER, &Rec.Article);
				PPErrorByDialog(this, CTL_ARTICLE_NUMBER, PPERR_DUPARTICLE);
			}
			else if(!r) {
				CALLEXCEPT();
			}
			else if(Rec.AccessLevel < LConfig.AccessLevel)
				PPErrorByDialog(this, CTL_ARTICLE_ACCESS, PPERR_TOOHIGHACCS);
			else {
				Rec.Article = new_art_no;
				ok = 1;
			}
			CATCHZOK
			if(ok) {
				ASSIGN_PTR(pRec, Rec);
				if(pAry && Data.P_List)
					pAry->copy(*Data.P_List);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmaMore)) {
				ArticleFilt ar_filt;
				ar_filt.AccSheetID = Rec.AccSheetID;
				ar_filt.Ft_Closed = -1;
				Data.ObjType = PPOBJ_ARTICLE;
				Data.ExtraPtr = &ar_filt;
				Data.TitleStrID = PPTXT_SELARTICLES;
				if(!ListToListDialog(&Data))
					PPErrorZ();
				enableCommand(cmAgreement, 0);
				clearEvent(event);
			}
		}
		ArticleTbl::Rec Rec;
		ListToListData  Data;
	};
	int    ok = -1, valid = 0;
	Reference * p_ref = PPRef;
	uint   i;
	GrpArticleDialog * dlg = 0;
	ArticleTbl::Rec    ar_rec;
	ObjAssocTbl::Rec   oa_rec;
	PPIDArray oa_ary;
	PPID   id;
	THROW_INVARG(pID);
	THROW(CheckRightsModByID(pID));
	if(*pID == 0) {
		THROW(GetFreeArticle(&ar_rec.Article, sheetID));
		ar_rec.AccSheetID = sheetID;
		ar_rec.ObjID = ar_rec.Article;
		ar_rec.Flags |= ARTRF_GROUP;
	}
	else {
		THROW(Search(*pID, &ar_rec) > 0);
		THROW(P_Tbl->GetListByGroup(ar_rec.ID, &oa_ary));
		oa_ary.sort();
	}
	THROW(CheckDialogPtr(&(dlg = new GrpArticleDialog())));
	THROW(dlg->setDTS(&ar_rec, &oa_ary));
	while(!valid && ExecView(dlg) == cmOK)
		valid = dlg->getDTS(&ar_rec, &oa_ary);
	if(valid) {
		ar_rec.ObjID = -ar_rec.Article;
		PPTransaction tra(1);
		THROW(tra);
		if(*pID) {
			THROW(UpdateByID(P_Tbl, Obj, *pID, &ar_rec, 0));
			THROW(p_ref->Assc.Remove(PPASS_GROUPARTICLE, ar_rec.ID, 0, 0));
		}
		else {
			THROW(AddObjRecByID(P_Tbl, Obj, pID, &ar_rec, 0));
			ar_rec.ID = *pID;
		}
		for(i = 0; i < oa_ary.getCount(); i++) {
			MEMSZERO(oa_rec);
			oa_rec.AsscType  = PPASS_GROUPARTICLE;
			oa_rec.PrmrObjID = ar_rec.ID;
			oa_rec.ScndObjID = oa_ary.at(i);
			THROW(p_ref->Assc.SearchFreeNum(PPASS_GROUPARTICLE, ar_rec.ID, &oa_rec.InnerNum));
			THROW(p_ref->Assc.Add(&(id = 0), &oa_rec, 0));
		}
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
int PPObjArticle::EditDialog(ArticleDlgData * pData)
{
	int    ok = 1;
	int    r = 1;
	int    cm = cmCancel;
	int    valid_data = 0;
	int    sel_linkobj = 0;
	uint   dlg_id = 0;
	long   old_ar_no = pData->Rec.Article;
	PPAccSheet acs_rec;
	ArticleDialog * dlg = 0;
	PPObjAccSheet acs_obj;
	THROW(acs_obj.Search(pData->Rec.AccSheetID, &acs_rec) > 0);
	if(pData->Options & ArticleDlgData::fAssocAccnt)
		dlg_id = DLG_ARTICLEACC;
	else if(acs_rec.Flags & ACSHF_USEALIASSUBST)
		dlg_id = DLG_ARTICLE2;
	else if(pData->Options & ArticleDlgData::fAllowUpdLinkObj && acs_rec.Assoc && PPMaster) {
		dlg_id = DLG_ARTICLE_RL;
		sel_linkobj = 1;
	}
	else
		dlg_id = DLG_ARTICLE;
	THROW(CheckDialogPtr(&(dlg = new ArticleDialog(dlg_id, pData))));
	dlg->enableCommand(cmOK, CheckRights(PPR_MOD));
	dlg->setCtrlLong(CTL_ARTICLE_ID, pData->Rec.ID);
	dlg->setCtrlData(CTL_ARTICLE_SHEETNAME, acs_rec.Name);
	dlg->setCtrlReadOnly(CTL_ARTICLE_SHEETNAME, true);
	if(sel_linkobj) {
		long   sel_extra = 0;
		if(acs_rec.Assoc == PPOBJ_LOCATION)
			sel_extra = 0;
		else
			sel_extra = acs_rec.ObjGroup;
		SetupPPObjCombo(dlg, CTLSEL_ARTICLE_LINKOBJ, acs_rec.Assoc, pData->Rec.ObjID, OLW_CANINSERT, reinterpret_cast<void *>(sel_extra));
	}
	while(!valid_data && (cm = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_ARTICLE_NUMBER, &pData->Rec.Article);
		dlg->getCtrlData(CTL_ARTICLE_ACCESS, &pData->Rec.AccessLevel);
		pData->Rec.Closed = BIN(dlg->getCtrlUInt16(CTL_ARTICLE_CLOSED));
		dlg->GetClusterData(CTL_ARTICLE_FLAGS, &pData->Rec.Flags);
		if(pData->Options & ArticleDlgData::fAssocAccnt) {
			AcctCtrlGroup::Rec acc_rec;
			dlg->getGroupData(GRP_ASSCACC, &acc_rec);
			pData->Rec.ObjID = acc_rec.AcctId.ac;
		}
		dlg->getCtrlData(CTL_ARTICLE_NAME,   pData->Rec.Name);
		if(*strip(pData->Rec.Name) == 0)
			PPErrorByDialog(dlg, CTL_ARTICLE_NAME, PPERR_NAMENEEDED);
		else if(old_ar_no != pData->Rec.Article && (r = GetFreeArticle(&pData->Rec.Article, pData->Rec.AccSheetID)) > 0) {
			dlg->setCtrlData(CTL_ARTICLE_NUMBER, &old_ar_no);
			PPErrorByDialog(dlg, CTL_ARTICLE_NUMBER, PPERR_DUPARTICLE);
		}
		else if(!r)
			PPError();
		else if(pData->Rec.AccessLevel < LConfig.AccessLevel)
			PPErrorByDialog(dlg, CTL_ARTICLE_ACCESS, PPERR_TOOHIGHACCS);
		else {
			if(sel_linkobj) {
				pData->Rec.ObjID = dlg->getCtrlLong(CTLSEL_ARTICLE_LINKOBJ);
				SString obj_name;
				GetObjectName(acs_rec.Assoc, pData->Rec.ObjID, obj_name);
				obj_name.CopyTo(pData->Rec.Name, sizeof(pData->Rec.Name));
				if(!CheckObject(&pData->Rec, 0))
					PPErrorByDialog(dlg, CTL_ARTICLE_LINKOBJ);
				else
					valid_data = 1;
			}
			else
				valid_data = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok ? cm : 0;
}

int PPObjArticle::Edit(PPID * pID, void * extraPtr /*sheetID*/)
{
	const ArticleFilt * p_filt = static_cast<const ArticleFilt *>(extraPtr);
	const  PPID extra_acs_id = NZOR(CurrFilt.AccSheetID, (p_filt ? p_filt->AccSheetID : 0));
	int    ok = 1;
	int    r;
	SString temp_buf;
	ArticleDlgData pack;
	if(*pID == 0) {
		THROW(CheckRights(PPR_INS));
		r = NewArticle(pID, extra_acs_id);
	}
	else {
		THROW(GetPacket(*pID, &pack) > 0);
		if(pack.Rec.Flags & ARTRF_GROUP) {
			THROW(EditGrpArticle(pID, extra_acs_id));
		}
		else {
			if(pack.Assoc) {
				THROW_PP(oneof6(pack.Assoc, PPOBJ_PERSON, PPOBJ_LOCATION,
					PPOBJ_ACCOUNT_PRE9004, PPOBJ_ACCOUNT2, PPOBJ_GLOBALUSERACC, PPOBJ_PROCESSOR), PPERR_INVACCSHEETASSOC); // @v11.3.12 PPOBJ_PROCESSOR
				if(!oneof2(pack.Assoc, PPOBJ_ACCOUNT_PRE9004, PPOBJ_ACCOUNT2)) {
					GetObjectName(pack.Assoc, pack.Rec.ObjID, temp_buf.Z());
					STRNSCPY(pack.Rec.Name, temp_buf);
					pack.Options |= ArticleDlgData::fDisableName;
				}
			}
			SETFLAG(pack.Options, ArticleDlgData::fAssocAccnt, oneof2(pack.Assoc, PPOBJ_ACCOUNT_PRE9004, PPOBJ_ACCOUNT2));
			if(!CheckObject(&pack.Rec, 0))
				pack.Options |= ArticleDlgData::fAllowUpdLinkObj;
			THROW(r = EditDialog(&pack));
			if(r == cmOK) {
				THROW(PutPacket(pID, &pack, 1));
				Dirty(*pID);
			}
		}
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjArticle::AutoFill(const PPAccSheet * pAccSheetRec)
{
	ArticleAutoAddDialog * dlg = new ArticleAutoAddDialog(pAccSheetRec->ID);
	int    r = dlg->GetResult();
	delete dlg;
	return r;
}

int PPObjArticle::NewArticle(PPID * pID, long sheetID)
{
	int    ok = 1;
	int    auto_fill_result = 0; // @v12.2.4
	int    cm = cmCancel;
	bool   done = false;
	PPID   obj_id = 0;
	PPObject * ppobj = 0;
	PPAccSheet acs_rec;
	ArticleDlgData  pack;
	THROW(SearchObject(PPOBJ_ACCSHEET, sheetID, &acs_rec) > 0);
	pack.Rec.AccSheetID = sheetID;
	pack.Assoc = acs_rec.Assoc;
	THROW(GetFreeArticle(&pack.Rec.Article, sheetID));
	if(oneof3(pack.Assoc, 0, PPOBJ_ACCOUNT_PRE9004, PPOBJ_ACCOUNT2)) {
		pack.Options &= ~ArticleDlgData::fDisableName;
		pack.Options |= ArticleDlgData::fAssocAccnt;
		THROW(cm = EditDialog(&pack));
		if(pack.Assoc == 0)
			pack.Rec.ObjID = pack.Rec.Article;
	}
	else {
		void * extra_ptr = 0;
		LocationFilt loc_filt;
		THROW(auto_fill_result = AutoFill(&acs_rec));
		if(pack.Assoc == PPOBJ_PERSON)
			extra_ptr = reinterpret_cast<void *>(acs_rec.ObjGroup);
		else if(pack.Assoc == PPOBJ_LOCATION) {
			loc_filt.LocType = LOCTYP_WAREHOUSE;
			if(acs_rec.WarehouseGroup) {
				loc_filt.Parent = acs_rec.WarehouseGroup;
			}
			extra_ptr = &loc_filt;
		}
		THROW(ppobj = GetPPObject(pack.Assoc, extra_ptr));
		obj_id = 0;
		THROW(cm = ppobj->Edit(&obj_id, extra_ptr));
		if(cm == cmOK) {
			if(P_Tbl->SearchObjRef(sheetID, obj_id) > 0) {
				ASSIGN_PTR(pID, P_Tbl->data.ID);
				cm = cmOK;
				done = true;
			}
			else {
				PersonTbl::Rec   psn_rec;
				LocationTbl::Rec loc_rec;
				PPAccount  acc_rec;
				PPGlobalUserAcc gua_rec;
				ProcessorTbl::Rec prc_rec; // @v11.3.12
				//THROW(ppobj->Search(obj_id, &assoc_obj_rec) > 0);
				switch(acs_rec.Assoc) {
					case PPOBJ_PERSON:
						THROW(ppobj->Search(obj_id, &psn_rec) > 0);
						STRNSCPY(pack.Rec.Name, psn_rec.Name);
						break;
					case PPOBJ_LOCATION:
						THROW(ppobj->Search(obj_id, &loc_rec) > 0);
						STRNSCPY(pack.Rec.Name, loc_rec.Name);
						break;
					case PPOBJ_GLOBALUSERACC:
						THROW(ppobj->Search(obj_id, &gua_rec) > 0);
						STRNSCPY(pack.Rec.Name, gua_rec.Name);
						break;
					case PPOBJ_ACCOUNT2:
						THROW(ppobj->Search(obj_id, &acc_rec) > 0);
						STRNSCPY(pack.Rec.Name, acc_rec.Name);
						break;
					case PPOBJ_PROCESSOR: // @v11.3.12
						THROW(ppobj->Search(obj_id, &prc_rec) > 0);
						STRNSCPY(pack.Rec.Name, prc_rec.Name);
						break;
				}
				pack.Rec.ObjID = obj_id;
				if(acs_rec.Flags & ACSHF_AUTOCREATART)
					cm = cmOK;
				else {
					pack.Options |= ArticleDlgData::fDisableName;
					THROW(cm = EditDialog(&pack));
				}
			}
		}
	}
	if(!done && cm == cmOK) {
		THROW(PutPacket(pID, &pack, 1));
	}
	CATCHZOKPPERR
	delete ppobj;
	// @v12.2.4 return ok ? cm : 0; 
	return ok ? ((cm == cmOK) ? cm : auto_fill_result) : 0; // @v12.2.4
}

TLP_IMPL(PPObjArticle, ArticleCore, P_Tbl);

PPObjArticle::PPObjArticle(void * extraPtr) : PPObject(PPOBJ_ARTICLE), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= implStrAssocMakeList;
	RVALUEPTR(CurrFilt, static_cast<ArticleFilt *>(ExtraPtr));
}

PPObjArticle::~PPObjArticle()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjArticle::Search(PPID id, void * b) { return SearchByID(P_Tbl, Obj, id, b); }

int PPObjArticle::GetFreeArticle(long * pID, long accSheetID)
{ 
	assert(pID != 0);
	// @v11.2.4 {
	// Если статья не ассоциирована с объектом, то в новой записи ObjID будет равно Article по-этому надо
	// проверить свободен ли соответствующий номер по ключу ArticleTbl::Key3
	bool check_surr_obj_ref = false;
	if(*pID == 0) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Search(accSheetID, &acs_rec) > 0 && acs_rec.Assoc == 0)
			check_surr_obj_ref = true;
	}
	// } @v11.2.4 
	int r = P_Tbl->SearchFreeNum(accSheetID, pID); 
	// @v11.2.4 {
	if(r) {
		while(check_surr_obj_ref) { 
			ArticleTbl::Key3 k3;
			k3.AccSheetID = accSheetID;
			k3.ObjID = *pID;
			if(P_Tbl->search(3, &k3, spEq)) {
				// Проверять на существование записи с ключом {accsheet; article} нет смысла поскольку мы вызовом SearchFreeNum получили максимальный номер
				(*pID)++;
			}
			else
				check_surr_obj_ref = false;
		}
	}
	// } @v11.2.4
	return r;
}

/*static*/int PPObjArticle::GetSearchingRegTypeID(PPID accSheetID, const char * pRegTypeCode, int useBillConfig, PPID * pRegTypeID)
{
	int    ok = -1;
	PPID   reg_type_id = 0;
	if(pRegTypeCode && pRegTypeCode[0]) {
		if(PPObjRegisterType::GetByCode(pRegTypeCode, &reg_type_id) > 0)
			ok = 1;
	}
	if(ok < 0) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(acs_obj.Fetch(accSheetID, &acs_rec) > 0) {
			reg_type_id = acs_rec.CodeRegTypeID;
			if(reg_type_id > 0)
				ok = 1;
		}
	}
	if(ok < 0 && useBillConfig) {
		reg_type_id = BillObj->Cfg.ClCodeRegTypeID;
		if(reg_type_id > 0)
			ok = 2;
	}
	ASSIGN_PTR(pRegTypeID, reg_type_id);
	return ok;
}

int PPObjArticle::SearchByRegCode(PPID accSheetID, PPID regTypeID, const char * pRegCode, PPID * pID, ArticleTbl::Rec * pRec)
{
	int    ok = -1;
	PPIDArray psn_list;
	PPObjPerson psn_obj;
	ArticleTbl::Rec ar_rec;
	if(psn_obj.GetListByRegNumber(regTypeID, 0, pRegCode, psn_list) > 0)
		for(uint i = 0; ok < 0 && i < psn_list.getCount(); i++)
			if(P_Tbl->SearchObjRef(accSheetID, psn_list.get(i), &ar_rec) > 0) {
				ASSIGN_PTR(pID, ar_rec.ID);
				ASSIGN_PTR(pRec, ar_rec);
				ok = 1;
			}
	return ok;
}

int PPObjArticle::GetByLocationList(PPID accSheetID, const PPIDArray * pLocList, PPIDArray * pArList)
{
	int    ok = 1;
	const  PPID link_obj_type = PPOBJ_LOCATION;
	const  uint linkobj_list_count = SVectorBase::GetCount(pLocList);
	if(linkobj_list_count) {
		if(accSheetID) {
			for(uint i = 0; i < linkobj_list_count; i++) {
				PPID   ar_id = 0;
				if(P_Tbl->LocationToArticle(pLocList->at(i), accSheetID, &ar_id) > 0)
					if(pArList && ar_id)
						THROW(pArList->addUnique(ar_id));
			}
		}
		else {
			PPAccSheet acs_rec;
			PPObjAccSheet acs_obj;
			for(SEnum en = acs_obj.P_Ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, link_obj_type); en.Next(&acs_rec) > 0;) {
				if(acs_rec.Assoc == link_obj_type)
					THROW(GetByLocationList(acs_rec.ID, pLocList, pArList)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::GetByPersonList(PPID accSheetID, const PPIDArray * pPsnList, PPIDArray * pArList)
{
	int    ok = 1;
	const  PPID link_obj_type = PPOBJ_PERSON;
	const  uint linkobj_list_count = SVectorBase::GetCount(pPsnList);
	if(linkobj_list_count) {
		if(accSheetID) {
			for(uint i = 0; i < linkobj_list_count; i++) {
				PPID   ar_id = 0;
				if(P_Tbl->PersonToArticle(pPsnList->at(i), accSheetID, &ar_id))
					if(pArList && ar_id)
						THROW(pArList->addUnique(ar_id));
			}
		}
		else {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			for(SEnum en = acs_obj.P_Ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, link_obj_type); en.Next(&acs_rec) > 0;) {
				if(acs_rec.Assoc == link_obj_type)
					THROW(GetByPersonList(acs_rec.ID, pPsnList, pArList)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::GetByPerson(PPID accSheetID, PPID psnID, PPID * pArID)
{
	int    ok = -1;
	PPID   ar_id = 0;
	if(accSheetID && psnID) {
		PPIDArray psn_list;
		PPIDArray ar_list;
		psn_list.add(psnID);
		if(GetByPersonList(accSheetID, &psn_list, &ar_list) && ar_list.getCount()) {
			ar_id = ar_list.at(0);
			ok = 1;
		}
	}
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int PPObjArticle::GetByProcessor(PPID accSheetID, PPID prcID, PPIDArray * pArList)
{
	int    ok = -1;
	const  PPID link_obj_type = PPOBJ_PROCESSOR;
	CALLPTRMEMB(pArList, Z());
	if(prcID) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		if(accSheetID) {
			if(acs_obj.Fetch(accSheetID, &acs_rec) > 0 && acs_rec.Assoc == link_obj_type) {
				ArticleTbl::Rec ar_rec;
				if(P_Tbl->SearchObjRef(accSheetID, prcID, &ar_rec) > 0) {
					CALLPTRMEMB(pArList, add(ar_rec.ID));
					ok = 1;
				}
			}
		}
		else {
			PPIDArray temp_list;
			for(SEnum en = acs_obj.P_Ref->EnumByIdxVal(PPOBJ_ACCSHEET, 1, link_obj_type); en.Next(&acs_rec) > 0;) {
				if(acs_rec.Assoc == link_obj_type) {
					temp_list.Z();
					if(GetByProcessor(acs_rec.ID, prcID, &temp_list) > 0) { // 
						assert(temp_list.getCount());
						CALLPTRMEMB(pArList, add(&temp_list));
						ok = 1;
					}
					
				}
			}
		}
	}
	if(pArList) {
		if(ok > 0) {
			assert(pArList->getCount());
			pArList->sortAndUndup();
		}
		else {
			assert(pArList->getCount() == 0);
		}
	}
	return ok;
}

int PPObjArticle::DeleteObj(PPID id)
{
	int    ok = 1;
	THROW(Search(id) > 0);
	if(P_Tbl->data.Flags & ARTRF_GROUP) {
		THROW(PPRef->Assc.Remove(PPASS_GROUPARTICLE, id, 0, 0));
	}
	else
		THROW(PPRef->Assc.Remove(PPASS_GROUPARTICLE, 0, id, 0));
	THROW(P_Tbl->Remove(id, 0));
	Dirty(id);
	CATCHZOK
	return ok;
}

/*static*/int PPObjArticle::PutAliasSubst(PPID arID, const LAssocArray * pList, int use_ta)
	{ return PPRef->PutPropArray(PPOBJ_ARTICLE, arID, ARTPRP_ALIASSUBST, pList, use_ta); }
/*static*/int PPObjArticle::GetAliasSubst(PPID arID, LAssocArray * pList)
	{ return PPRef->GetPropArray(PPOBJ_ARTICLE, arID, ARTPRP_ALIASSUBST, pList); }

int PPObjArticle::Helper_PutAgreement(PPID id, PPArticlePacket * pPack)
{
	int    ok = -1;
	SString fmt_buf, msg_buf;
	if(pPack->P_CliAgt) {
		if(CheckRights(ARTRT_CLIAGT)) {
			THROW(PutClientAgreement(id, pPack->P_CliAgt, 0));
			ok = 1;
		}
		else {
			// Нет прав на изменение клиентского соглашения (статья '%s')
			PPLoadText(PPTXT_LOG_NORT_CLIAGTUPD, fmt_buf);
			msg_buf.Printf(fmt_buf, pPack->Rec.Name);
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
		}
	}
	if(pPack->P_SupplAgt) {
		if(CheckRights(ARTRT_CLIAGT)) {
			THROW(PutSupplAgreement(id, pPack->P_SupplAgt, 0));
			ok = 1;
		}
		else {
			// Нет прав на изменение соглашения с поставщиком (статья '%s')
			PPLoadText(PPTXT_LOG_NORT_SPLAGTUPD, fmt_buf);
			msg_buf.Printf(fmt_buf, pPack->Rec.Name);
			PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::PutPacket(PPID * pID, PPArticlePacket * pPack, int use_ta)
{
	int    ok = 1;
	PPObjAccTurn at_obj;
	PPObjLocation loc_obj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID == 0) {
			if(pPack) {
				//
				// @todo
				// Здесь следует проверить право на создание записи.
				// Однако, есть опасность, что при проверке этого флага, возникнет ситуация когда
				// из другого раздела не удасться принять новую статью. По этому, пока оставляем как есть.
				//
				pPack->Rec.ID = 0;
				pPack->Rec.Article = 0;
				P_Tbl->SearchFreeNum(pPack->Rec.AccSheetID, &pPack->Rec.Article, 0);
				THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0));
				THROW(Helper_PutAgreement(*pID, pPack));
				if(!pPack->DontUpdateAliasSubst)
					THROW(PutAliasSubst(*pID, pPack->GetAliasSubst(), 0));
				DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
				if(pPack->Assoc == PPOBJ_LOCATION) {
					if(pPack->Rec.ObjID)
						loc_obj.Dirty(pPack->Rec.ObjID);
				}
			}
		}
		else if(pPack) {
			int    prev_stop_flag = BIN(pPack->Rec.Flags & ARTRF_STOPBILL);
			PPArticlePacket org_pack;
			THROW(GetPacket(*pID, &org_pack) > 0);
			pPack->Rec.ID = *pID;
			if(pPack->P_CliAgt) {
				pPack->P_CliAgt->ClientID = *pID;
			}
			if(pPack->P_SupplAgt) {
				pPack->P_SupplAgt->SupplID = *pID;
			}
			if(!IsPacketEq(*pPack, org_pack, pPack->DontUpdateAliasSubst ? peoDontCmpAliasSubst : 0)) {
				THROW(CheckRights(PPR_MOD));
				THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0));
				if(pPack->Rec.Article != org_pack.Rec.Article)
					THROW(at_obj.P_Tbl->UpdateRelsArRef(*pID, pPack->Rec.Article, 0));
				THROW(Helper_PutAgreement(*pID, pPack));
				if(!pPack->DontUpdateAliasSubst)
					THROW(PutAliasSubst(*pID, pPack->GetAliasSubst(), 0));
				Dirty(*pID);
				DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				if((org_pack.Rec.Flags & ARTRF_STOPBILL) != (pPack->Rec.Flags & ARTRF_STOPBILL)) {
					DS.LogAction((pPack->Rec.Flags & ARTRF_STOPBILL) ? PPACN_ARSTOPSET : PPACN_ARSTOPRESET, Obj, *pID, 0, 0);
				}
			}
			else
				ok = -1;
		}
		else {
			THROW(CheckRights(PPR_DEL));
			THROW(P_Tbl->Remove(*pID, 0));
			THROW(PPRef->Assc.Remove(PPASS_GROUPARTICLE, *pID, 0, 0));
			THROW(PPRef->Assc.Remove(PPASS_GROUPARTICLE, 0, *pID, 0));
			THROW(PutClientAgreement(*pID, 0, 0));
			THROW(PutSupplAgreement(*pID, 0, 0));
			THROW(PutAliasSubst(*pID, 0, 0));
			Dirty(*pID);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::GetPacket(PPID id, PPArticlePacket * pPack)
{
	int    ok = 1, r;
	pPack->Init();
	if(PPCheckGetObjPacketID(Obj, id)) {
		ok = Search(id, &pPack->Rec);
		if(ok > 0) {
			LAssocArray alias_subst;
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			if(acs_obj.Fetch(pPack->Rec.AccSheetID, &acs_rec) > 0) {
				pPack->Assoc = acs_rec.Assoc;
				if(acs_rec.Flags & ACSHF_USESUPPLAGT || pPack->Rec.AccSheetID == GetSupplAccSheet()) {
					PPSupplAgreement agt;
					THROW(r = GetSupplAgreement(id, &agt, 0));
					if(r > 0)
						THROW(pPack->SetSupplAgreement(&agt));
				}
				else if(acs_rec.Flags & ACSHF_USECLIAGT || pPack->Rec.AccSheetID == GetSellAccSheet()) {
					PPClientAgreement agt;
					THROW(r = GetClientAgreement(id, agt, 0));
					if(r > 0)
						THROW(pPack->SetClientAgreement(&agt, 0));
				}
				if(GetAliasSubst(id, &alias_subst) > 0)
					for(uint i = 0; i < alias_subst.getCount(); i++)
						pPack->AddAliasSubst(alias_subst.at(i).Key, alias_subst.at(i).Val);
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

StrAssocArray * PPObjArticle::MakeStrAssocList(void * extraPtr /*accSheetID-->(ArticleFilt*)*/ )
{
//#define DO_GET_NAME_FROM_CACHE // Вариант с раздельным извлечением строк (значительно медленнее прямого метода)q
	StrAssocArray * p_list = 0;
	PROFILE_START
	const  ArticleFilt * p_filt = &CurrFilt;
	const  PPID acs_id = p_filt ? labs(p_filt->AccSheetID) : 0;
	ArticleTbl::Key2 k2;
	PPObjAccSheet acc_sheet_obj;
	PPAccSheet acs_rec;
	ArticleTbl::Rec ar_rec;
	PPSupplAgreement suppl_agt;
	DBQ  * dbq = 0;
	ArticleTbl * p_tbl = P_Tbl;
	BExtQuery q(p_tbl, 2);
	THROW_MEM(p_list = new StrAssocArray);
	if(acs_id)
		THROW(acc_sheet_obj.Fetch(acs_id, &acs_rec) > 0);
#ifdef DO_GET_NAME_FROM_CACHE
	q.select(p_tbl->ID, p_tbl->ObjID, 0L);
#else
	q.select(p_tbl->ID, p_tbl->Name, p_tbl->ObjID, p_tbl->Flags, 0L);
#endif // DO_GET_NAME_FROM_CACHE
	if(acs_id)
		dbq = & (p_tbl->AccSheetID == acs_id);
	if(!p_filt || p_filt->Ft_Closed < 0)
		dbq = & (*dbq && (p_tbl->Closed == 0L));
	else if(p_filt && p_filt->Ft_Closed > 0)
		dbq = & (*dbq && (p_tbl->Closed > 0L));
	q.where(*dbq);
	MEMSZERO(k2);
	k2.AccSheetID = acs_id;
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		const  PPID ar_id = p_tbl->data.ID;
		p_tbl->CopyBufTo(&ar_rec);
		int   do_skip = 0;
		if(p_filt) {
			if(p_filt->Flags & ArticleFilt::fNonGenericOnly && ar_rec.Flags & ARTRF_GROUP)
				do_skip = 1;
			else if(p_filt->Flags & ArticleFilt::fWithIxParamOnly)
				do_skip = (GetSupplAgreement(ar_rec.ID, &suppl_agt, 0) > 0 && !suppl_agt.Ep.IsEmpty()) ? 0 : 1;
		}
		if(!do_skip) {
#ifdef DO_GET_NAME_FROM_CACHE
			if(Fetch(ar_id, &ar_rec) > 0) {
				THROW_SL(p_list->AddFast(ar_id, ar_rec.Name));
			}
#else
			THROW_SL(p_list->AddFast(ar_id, ar_rec.Name));
#endif // DO_GET_NAME_FROM_CACHE
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	PROFILE_END
	return p_list;
#undef DO_GET_NAME_FROM_CACHE
}

int PPObjArticle::Browse(void * extraPtr /*(ArticleFilt *)*/)
{
	if(extraPtr) {
		ArticleFilt filt;
		if(filt.IsA(static_cast<PPBaseFilt *>(extraPtr))) {
			filt = *static_cast<const ArticleFilt *>(extraPtr);
			return ViewArticle(&filt);
		}
		else
			return ViewArticle(0);
	}
	else
		return ViewArticle(0);
}
//
//
//
const char * PPObjArticle::GetNamePtr()
{
	return P_Tbl->data.Name;
}

int PPObjArticle::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	switch(msg) {
		case DBMSG_OBJDELETE:
			switch(_obj) {
				case PPOBJ_ACCSHEET:
					{
						ArticleTbl::Key1 k;
						k.AccSheetID = _id;
						k.Article = 0;
						if(P_Tbl->search(1, &k, spGe) && k.AccSheetID == _id)
							ok = RetRefsExistsErr(Obj, P_Tbl->data.ID);
						else
							ok = (BTROKORNFOUND) ? DBRPL_OK : (PPSetErrorDB(), DBRPL_ERROR);
					}
					break;
				case PPOBJ_PERSON:
				case PPOBJ_LOCATION:
				case PPOBJ_GLOBALUSERACC: // @v11.3.12 @fix
				case PPOBJ_PROCESSOR: // @v11.3.12
					{
						PPID   ar_id = 0;
						int    r = SearchAssocObjRef(_obj, _id, 0, 0, &ar_id);
						ok = _ProcessSearch(r, ar_id);
					}
					break;
				case PPOBJ_DEBTDIM:
					{
						Reference * p_ref = PPRef;
						PropertyTbl::Key1 k1;
						{
							const  PPID prop_id = ARTPRP_DEBTLIMLIST2;
							PPIDArray ar_list;
							TSVector <PPClientAgreement::DebtLimit> debt_lim_list;
							MEMSZERO(k1);
							k1.ObjType = PPOBJ_ARTICLE;
							k1.Prop = prop_id;
							if(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id) do {
								ar_list.add(p_ref->Prop.data.ObjID);
							} while(p_ref->Prop.search(1, &k1, spNext) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id);
							ar_list.sortAndUndup();
							for(uint i = 0; i < ar_list.getCount(); i++) {
								if(p_ref->GetPropArray(PPOBJ_ARTICLE, ar_list.get(i), prop_id, &debt_lim_list) > 0) {
									for(uint j = 0; j < debt_lim_list.getCount(); j++) {
										if(debt_lim_list.at(j).DebtDimID == _id)
											return RetRefsExistsErr(Obj, ar_list.get(i));
									}
								}
							}
						}
						{
							const  PPID prop_id = ARTPRP_SUPPLAGT2;
							PPIDArray ar_list;
							SBuffer _buf;
							MEMSZERO(k1);
							k1.ObjType = PPOBJ_ARTICLE;
							k1.Prop = prop_id;
							if(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id) do {
								ar_list.add(p_ref->Prop.data.ObjID);
							} while(p_ref->Prop.search(1, &k1, spNext) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id);
							ar_list.sortAndUndup();
							for(uint i = 0; i < ar_list.getCount(); i++) {
								if(p_ref->GetPropSBuffer(PPOBJ_ARTICLE, ar_list.get(i), prop_id, _buf) > 0) {
									SSerializeContext ctx;
									PPSupplAgreement agt;
									if(agt.Serialize(-1, _buf, &ctx) && agt.Ep.DebtDimList.Search(_id, 0, 0))
										return RetRefsExistsErr(Obj, ar_list.get(i));
								}
							}
						}
					}
					break;
			}
			break;
		case DBMSG_PERSONLOSEKIND:
			{
				const  PPID person_id = _obj;
				const  PPID kind_id = _id;
				PPID   ar_id = 0;
				int    r = SearchAssocObjRef(PPOBJ_PERSON, person_id, 0, kind_id, &ar_id);
				ok = _ProcessSearch(r, ar_id);
			}
			break;
		case DBMSG_PERSONACQUIREKIND:
			{
				const  PPID person_id = _id;
				const  PPID kind_id = reinterpret_cast<long>(extraPtr);

				PPID   sheet_id = 0;
				PPAccSheet acs_rec;
				PPObjAccSheet as_obj;
				while(ok == DBRPL_OK && as_obj.EnumItems(&sheet_id, &acs_rec) > 0) {
					if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == kind_id && acs_rec.Flags & ACSHF_AUTOCREATART) {
						if(P_Tbl->SearchObjRef(sheet_id, person_id) < 0) {
							PPID   ar_id = 0;
							if(!CreateObjRef(&ar_id, sheet_id, person_id, 0, 0))
								ok = DBRPL_ERROR;
						}
					}
				}
			}
			break;
		case DBMSG_OBJNAMEUPDATE:
			if(oneof3(_obj, PPOBJ_PERSON, PPOBJ_LOCATION, PPOBJ_PROCESSOR)) { // @v11.3.12 PPOBJ_PROCESSOR
				int    r;
				PPID   acs_id = 0;
				while((r = SearchAssocObjRef(_obj, _id, &acs_id, 0, 0)) > 0 && (r = _UpdateName(static_cast<const char *>(extraPtr))) != 0)
					;
				ok = r ? DBRPL_OK : DBRPL_ERROR;
			}
			break;
		case DBMSG_OBJREPLACE:
			if(_obj == PPOBJ_ARTICLE)
				ok = ReplyArticleReplace(_id, reinterpret_cast<long>(extraPtr));
			else if(_obj == PPOBJ_PERSON)
				ok = ReplyPersonReplace(_id, reinterpret_cast<long>(extraPtr));
			break;
		case DBMSG_WAREHOUSEADDED:
			if(_obj == PPOBJ_LOCATION) {
				ok = ReplyObjectCreated(_obj, _id);
			}
			break;
		case DBMSG_GLOBALACCADDED:
			if(_obj == PPOBJ_GLOBALUSERACC) {
				ok = ReplyObjectCreated(_obj, _id);
			}
			break;
		case DBMSG_PROCESSORADDED: // @v11.3.12
			if(_obj == PPOBJ_PROCESSOR) {
				ok = ReplyObjectCreated(_obj, _id);
			}
			break;
	}
	return ok;
}

int PPObjArticle::SearchAssocObjRef(PPID _obj, PPID _id, PPID * pAccSheetID, PPID kind, PPID * pID)
{
	int    r;
	PPID   acc_sheet_id = DEREFPTRORZ(pAccSheetID);
	PPAccSheet acs_rec;
	PPObjAccSheet as_obj;
	ASSIGN_PTR(pID, 0);
	while((r = as_obj.EnumItems(&acc_sheet_id, &acs_rec)) > 0)
		if(acs_rec.Assoc == _obj && (!kind || acs_rec.ObjGroup == kind)) {
			ArticleTbl::Rec rec;
			if((r = P_Tbl->SearchObjRef(acc_sheet_id, _id, &rec)) >= 0) {
				if(r > 0)
					ASSIGN_PTR(pID, rec.ID);
				break;
			}
		}
	ASSIGN_PTR(pAccSheetID, acc_sheet_id);
	return r;
}

int PPObjArticle::AddSimple(PPID * pID, PPID accSheetID, const char * pName, long ar, int use_ta)
{
	int    ok = 1;
	ArticleTbl::Rec rec;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(acs_obj.Fetch(accSheetID, &acs_rec) > 0);
		THROW_PP(*strip(STRNSCPY(rec.Name, pName)), PPERR_NAMENEEDED);
		if(ar == 0) {
			THROW(P_Tbl->SearchFreeNum(accSheetID, &ar));
		}
		else {
			THROW(P_Tbl->SearchFreeNum(accSheetID, &ar) < 0);
		}
		rec.AccSheetID = accSheetID;
		rec.Article = ar;
		THROW(AddObjRecByID(P_Tbl, Obj, pID, &rec, 0));
		DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::CreateObjRef(PPID * pID, PPID accSheetID, PPID objID, long ar, int use_ta)
{
	int    ok = 1;
	SString temp_buf;
	ArticleTbl::Rec rec;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(acs_obj.Fetch(accSheetID, &acs_rec) > 0);
		THROW(GetObjectName(acs_rec.Assoc, objID, temp_buf.Z()) > 0);
		temp_buf.Strip();
		STRNSCPY(rec.Name, temp_buf);
		if(ar == 0) {
			THROW(P_Tbl->SearchFreeNum(accSheetID, &ar));
		}
		else {
			THROW(P_Tbl->SearchFreeNum(accSheetID, &ar) < 0);
		}
		rec.AccSheetID = accSheetID;
		rec.ObjID   = objID;
		rec.Article = ar;
		THROW(AddObjRecByID(P_Tbl, Obj, pID, &rec, 0));
		DS.LogAction(PPACN_OBJADD, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::_ProcessSearch(int r, PPID id)
{
	return (r > 0) ? RetRefsExistsErr(Obj, id) : (r ? DBRPL_OK : DBRPL_ERROR);
}

int PPObjArticle::ReplyObjectCreated(PPID objType, PPID objID)
{
	PPAccSheet acs_rec;
	PPObjAccSheet acs_obj;
	for(PPID acs_id = 0; acs_obj.EnumItems(&acs_id, &acs_rec) > 0;) {
		if(acs_rec.Assoc == objType && acs_rec.Flags & ACSHF_AUTOCREATART) {
			if(P_Tbl->SearchObjRef(acs_id, objID) < 0) {
				PPID ar_id = 0;
				if(!CreateObjRef(&ar_id, acs_id, objID, 0, 0))
					return DBRPL_ERROR;
			}
		}
	}
	return DBRPL_OK;
}

int PPObjArticle::_UpdateName(const char * pNewName)
{
	int    ok = 1;
	const  char * p_newname = pNewName;
	if(p_newname == 0)
		ok = (PPSetObjError(PPERR_REFSEXISTS, Obj, 0), 0);
	else {
		ArticleTbl::Rec rec;
		P_Tbl->CopyBufTo(&rec);
		if(!sstreq(rec.Name, p_newname)) {
			memzero(rec.Name, sizeof(rec.Name));
			STRNSCPY(rec.Name, p_newname);
			if(P_Tbl->Update(rec.ID, &rec, 0) == 0)
				ok = 0;
		}
	}
	return ok;
}

int PPObjArticle::ReplyPersonReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	int    r;
	PPID   acc_sheet_id = 0;
	PPID   dest_ar_id = 0;
	PersonTbl::Rec psnr;
	while((r = SearchAssocObjRef(PPOBJ_PERSON, dest, &acc_sheet_id, 0, &dest_ar_id)) > 0) {
		if((r = P_Tbl->SearchObjRef(acc_sheet_id, src)) > 0) {
			THROW(ReplyArticleReplace(dest_ar_id, P_Tbl->data.ID) != DBRPL_ERROR);
		}
		else if(r < 0) {
			THROW(SearchObject(PPOBJ_PERSON, src, &psnr) > 0);
			THROW_DB(updateFor(P_Tbl, 0, (P_Tbl->ID == dest_ar_id),
				set(P_Tbl->ObjID, dbconst(src)).
				set(P_Tbl->Name, dbconst(psnr.Name))));
		}
		else
			break;
	}
	THROW(r);
	CATCHZOK
	return ok;
}

int PPObjArticle::ReplyArticleReplace(PPID dest, PPID src)
{
	int    ok = DBRPL_OK;
	PPID   dst_sheet, src_sheet;
	THROW(Search(dest) > 0);
	dst_sheet = P_Tbl->data.AccSheetID;
	THROW(Search(src) > 0);
	src_sheet = P_Tbl->data.AccSheetID;
	THROW_PP(dst_sheet == src_sheet, PPERR_REPLARTNEQSHEET);
	THROW(BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, dest, reinterpret_cast<void *>(src)));
	THROW(RemoveByID(P_Tbl, dest, 0));
	CATCHZOK
	return ok;
}

int PPObjArticle::SerializePacket(int dir, PPArticlePacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint8  cli_agt_ind   = 0; // 1 - соглашение отсутствует
	uint8  suppl_agt_ind = 0; // 1 - соглашение отсутствует
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->Assoc, rBuf));
	if(dir > 0) {
		if(pPack->P_CliAgt) {
			THROW_SL(rBuf.Write(cli_agt_ind = 0));
			THROW(pPack->P_CliAgt->Serialize(dir, rBuf, pSCtx));
		}
		else {
			THROW_SL(rBuf.Write(cli_agt_ind = 1));
		}
		if(pPack->P_SupplAgt) {
			THROW_SL(rBuf.Write(suppl_agt_ind = 0));
			THROW(pPack->P_SupplAgt->Serialize(dir, rBuf, pSCtx));
		}
		else {
			THROW_SL(rBuf.Write(suppl_agt_ind = 1));
		}
	}
	else if(dir < 0) {
		THROW_SL(rBuf.Read(cli_agt_ind));
		ZDELETE(pPack->P_CliAgt);
		if(cli_agt_ind == 0) {
			THROW_MEM(pPack->P_CliAgt = new PPClientAgreement);
			THROW(pPack->P_CliAgt->Serialize(dir, rBuf, pSCtx));
		}
		//
		THROW_SL(rBuf.Read(suppl_agt_ind));
		ZDELETE(pPack->P_SupplAgt);
		if(suppl_agt_ind == 0) {
			THROW_MEM(pPack->P_SupplAgt = new PPSupplAgreement);
			THROW(pPack->P_SupplAgt->Serialize(dir, rBuf, pSCtx));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPArticlePacket * p_pack = static_cast<PPArticlePacket *>(p->Data);
	THROW(p && p->Data);
	if(stream == 0) {
		assert(pID); // @v11.0.10
		int    is_new = 0;
		ArticleTbl::Rec rec;
		if(P_Tbl->SearchObjRef(p_pack->Rec.AccSheetID, p_pack->Rec.ObjID, &rec) > 0 && (!*pID || rec.ID != *pID))
			*pID = rec.ID;
		{
			//
			// Запрещаем изменение номера статьи при изменении записи
			//
			if(*pID) {
				if(Search(*pID, &rec) > 0)
					p_pack->Rec.Article = rec.Article;
				else
					*pID = 0;
			}
			else
				is_new = 1;
			p_pack->DontUpdateAliasSubst = 1;
			//
			// Запрещаем изменение параметров формирования автозаказа при изменении записи
			//
			if(*pID && p_pack && p_pack->P_SupplAgt) { // @v11.0.10 (pID-->*pID)
				PPSupplAgreement spl_agt;
				if(GetSupplAgreement(*pID, &spl_agt, 0) > 0) {
					PPSupplAgreement spl_agt_general;
					p_pack->P_SupplAgt->RestoreAutoOrderParams(spl_agt);
					// @v11.0.10 {
					if(GetSupplAgreement(0, &spl_agt_general, 0) > 0 && spl_agt_general.Flags & AGTF_DEFAGENTLOCTODBDIV)
						p_pack->P_SupplAgt->DefAgentID = spl_agt.DefAgentID;
					// } @v11.0.10 
				}
			}
			int    r = PutPacket(pID, p_pack, 1);
			if(!r) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTARTICLE, p_pack->Rec.ID, p_pack->Rec.Name);
				ok = -1;
			}
			else if(r > 0)
				ok = is_new ? 101 : 102; // @ObjectCreated : @ObjectUpdated
		}
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjArticle, PPArticlePacket>(this, p, id, stream, pCtx); }

int PPObjArticle::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPArticlePacket * ap = static_cast<PPArticlePacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_ACCSHEET, &ap->Rec.AccSheetID, ary, replace));
		THROW(ProcessObjRefInArray(ap->Assoc, &ap->Rec.ObjID, ary, replace));
		if(ap->P_CliAgt) {
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE,  &ap->P_CliAgt->DefAgentID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_QUOTKIND, &ap->P_CliAgt->DefQuotKindID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE,  &ap->P_CliAgt->ExtObjectID, ary, replace));
			for(uint i = 0; i < ap->P_CliAgt->DebtLimList.getCount(); i++) {
				PPClientAgreement::DebtLimit & r_item = ap->P_CliAgt->DebtLimList.at(i);
				THROW(ProcessObjRefInArray(PPOBJ_DEBTDIM,  &r_item.DebtDimID, ary, replace));
			}
		}
		if(ap->P_SupplAgt) {
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &ap->P_SupplAgt->DefAgentID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &ap->P_SupplAgt->Ep.GoodsGrpID, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.ExpendOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.RcptOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.SupplRetOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.RetOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.MovInOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_OPRKIND,    &ap->P_SupplAgt->Ep.MovOutOp, ary, replace));
			THROW(ProcessObjRefInArray(PPOBJ_QUOTKIND,   &ap->P_SupplAgt->Ep.PriceQuotID, ary, replace));
			for(uint i = 0; i < ap->P_SupplAgt->OrderParamList.getCount(); i++) {
				PPSupplAgreement::OrderParamEntry & r_entry = ap->P_SupplAgt->OrderParamList.at(i);
				THROW(ProcessObjRefInArray(PPOBJ_GOODSGROUP, &r_entry.GoodsGrpID, ary, replace));
				THROW(ProcessObjRefInArray(PPOBJ_LOCATION,   &r_entry.LocID, ary, replace));
				THROW(ProcessObjRefInArray(PPOBJ_PERSON,     &r_entry.MngrID, ary, replace));
			}
		}
	}
	else
		ok = 0;
	CATCHZOK
	return ok;
}

int PPObjArticle::GetMainOrgAsSuppl(PPID * pID, int processAbsense, int use_ta)
{
	int    ok = 1;
	PPID   i;
	const  PPID   acs_id = GetSupplAccSheet();
	THROW(acs_id);
	THROW(GetMainOrgID(&i) > 0);
	{
		int    r = P_Tbl->SearchObjRef(acs_id, i);
		THROW(r);
		if(r > 0)
			*pID = P_Tbl->data.ID;
		else if(r) {
			if(processAbsense) {
				THROW(CreateObjRef(pID, acs_id, i, 0, use_ta));
			}
			else {
				CALLEXCEPT_PP(PPERR_MAINORGASSUPPL);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTART, CTL_RTART_FLAGS, CTL_RTART_SFLAGS, bufSize, rt, pDlg);
}

int PPObjArticle::GetRelPersonList(PPID arID, PPID relTypeID, int reverse, PPIDArray * pList)
{
	int    ok = -1;
	PPID   acs_id = 0;
	PPID   psn_id = ObjectToPerson(arID, &acs_id);
	if(psn_id) {
		PPIDArray rel_list;
		PPObjPerson psn_obj;
		if(psn_obj.GetRelPersonList(psn_id, relTypeID, reverse, &rel_list) > 0) {
			if(pList) {
				GetByPersonList(acs_id, &rel_list, pList);
				if(pList->getCount())
					ok = 1;
			}
		}
	}
	return ok;
}

int PPObjArticle::GetRelPersonSingle(PPID arID, PPID relTypeID, int reverse, PPID * pRelID)
{
	PPID   rel_id = 0;
	PPIDArray ar_list;
	GetRelPersonList(arID, relTypeID, reverse, &ar_list);
	if(ar_list.getCount())
		rel_id = ar_list.at(0);
	ASSIGN_PTR(pRelID, rel_id);
	return rel_id ? 1 : -1;
}

SString & PPObjArticle::MakeCodeString(const ArticleTbl::Rec * pRec, long options, SString & rBuf)
{
	rBuf.Z();
	rBuf.Cat(pRec->Name);
	if(options & 0x0001) {
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		rBuf.CatDiv('-', 1);
		if(acs_obj.Fetch(pRec->AccSheetID, &acs_rec) > 0)
			rBuf.Cat(acs_rec.Name);
		else
			ideqvalstr(pRec->AccSheetID, rBuf);
	}
	return rBuf;
}

int PPObjArticle::CheckPersonPacket(const PPPersonPacket * pPack, PPIDArray * pAbsentKinds)
{
	int    ok = 1;
	if(pPack && pPack->Rec.ID) {
		PPIDArray id_list, ar_id_list;
		id_list.add(pPack->Rec.ID);
		GetByPersonList(0, &id_list, &ar_id_list);
		if(ar_id_list.getCount()) {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			for(uint i = 0; i < ar_id_list.getCount(); i++) {
				ArticleTbl::Rec ar_rec;
				const  PPID id = ar_id_list.get(i);
				if(Search(id, &ar_rec) > 0) {
					THROW(acs_obj.Fetch(ar_rec.AccSheetID, &acs_rec) > 0);
					const int exists = pPack->Kinds.lsearch(acs_rec.ObjGroup);
					if(!exists) {
						if(pAbsentKinds) {
							pAbsentKinds->add(acs_rec.ObjGroup);
						}
						else {
							CALLEXCEPT_PP_S(PPERR_AR_INVLINKPERSONKIND, acs_rec.Name);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjArticle::CheckObject(const ArticleTbl::Rec * pRec, SString * pMsgBuf)
{
	int    ok = 1;
	SString ar_buf;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	THROW_INVARG(pRec);
	MakeCodeString(pRec, 0x0001, ar_buf);
	THROW_PP(acs_obj.Fetch(pRec->AccSheetID, &acs_rec) > 0, PPERR_AR_INVACCSHEET);
	if(acs_rec.Assoc == PPOBJ_PERSON) {
		PPObjPerson psn_obj;
		PersonTbl::Rec psn_rec;
		THROW_PP(acs_rec.ObjGroup, PPERR_ACS_NOTDEFPERSONKIND);
		THROW_PP(pRec->ObjID, PPERR_AR_ZEROLINK_PSN);
		THROW_PP(psn_obj.Search(pRec->ObjID, &psn_rec) > 0, PPERR_AR_HANGLINK_PSN);
		if(acs_rec.ObjGroup) {
			THROW_PP_S(psn_obj.P_Tbl->IsBelongsToKind(psn_rec.ID, acs_rec.ObjGroup) > 0, PPERR_AR_INVLINKPERSONKIND, acs_rec.Name);
			THROW_PP(sstreq(psn_rec.Name, pRec->Name), PPERR_AR_UNEQNAME_PSN);
		}
	}
	else if(acs_rec.Assoc == PPOBJ_LOCATION) {
		PPObjLocation loc_obj;
		LocationTbl::Rec loc_rec;
		THROW_PP(loc_obj.Search(pRec->ObjID, &loc_rec) > 0, PPERR_AR_HANGLINK_LOC);
		THROW_PP(loc_rec.Type == LOCTYP_WAREHOUSE, PPERR_AR_INVLINKLOC);
		THROW_PP(sstreq(loc_rec.Name, pRec->Name), PPERR_AR_UNEQNAME_LOC);
	}
	else if(oneof2(acs_rec.Assoc, PPOBJ_ACCOUNT_PRE9004, PPOBJ_ACCOUNT2)) {
		PPObjAccount acc_obj;
		PPAccount acc_rec;
		THROW_PP(acc_obj.Search(pRec->ObjID, &acc_rec) > 0, PPERR_AR_HANGLINK_ACC);
		THROW_PP(sstreq(acc_rec.Name, pRec->Name), PPERR_AR_UNEQNAME_ACC);
	}
	else if(acs_rec.Assoc == PPOBJ_GLOBALUSERACC) {
		PPObjGlobalUserAcc gua_obj;
		PPGlobalUserAcc gua_rec;
		THROW_PP(gua_obj.Search(pRec->ObjID, &gua_rec) > 0, PPERR_AR_HANGLINK_GUA);
		THROW_PP(sstreq(gua_rec.Name, pRec->Name), PPERR_AR_UNEQNAME_GUA);
	}
	else if(acs_rec.Assoc == PPOBJ_PROCESSOR) { // @v11.3.12
		PPObjProcessor prc_obj;
		ProcessorTbl::Rec prc_rec;
		THROW_PP(prc_obj.Search(pRec->ObjID, &prc_rec) > 0, PPERR_AR_HANGLINK_PRC);
		THROW_PP(sstreq(prc_rec.Name, pRec->Name), PPERR_AR_UNEQNAME_PRC);
	}
	else {
		THROW_PP(acs_rec.Assoc == 0, PPERR_ACS_INVLINKOBJ);
		THROW_PP(pRec->ObjID == 0, PPERR_AR_INVLINKOBJ);
	}
	CATCHZOK
	if(ok == 0) {
		if(pMsgBuf) {
			*pMsgBuf = 0;
			PPGetLastErrorMessage(1, *pMsgBuf);
		}
	}
	return ok;
}
//
//
//
class ArticleCache : public ObjCacheHash {
public:
	ArticleCache() : ObjCacheHash(PPOBJ_ARTICLE, sizeof(Data), 1024*1024, 4), IsVatFreeListInited(0)
	{
	}
	virtual void FASTCALL Dirty(PPID id); // @sync_w
	int    IsSupplVatFree(PPID supplID); // @sync_rw
private:
	struct Data : public ObjCacheEntry {
		PPID   AccSheetID;
		long   Article;
		PPID   ObjID;
		long   Flags;
	};
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	PPIDArray VatFreeSupplList;
	int    IsVatFreeListInited;
};

int ArticleCache::IsSupplVatFree(PPID supplID)
{
	int    ok = -1;
	if(supplID) {
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(IsVatFreeListInited) {
			ok = VatFreeSupplList.bsearch(supplID) ? 1 : -1;
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!IsVatFreeListInited) {
				PPObjPerson  psn_obj;
				PPObjArticle ar_obj;
				PPIDArray    psn_list;
				VatFreeSupplList.clear();
				if(psn_obj.P_Tbl->GetVATFreePersonList(&psn_list) && ar_obj.GetByPersonList(0, &psn_list, &VatFreeSupplList)) {
					VatFreeSupplList.sort();
					IsVatFreeListInited = 1;
					ok = VatFreeSupplList.bsearch(supplID) ? 1 : -1;
				}
				else
					ok = 0;
			}
		}
	}
	return ok;
}

void FASTCALL ArticleCache::Dirty(PPID id)
{
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		Helper_Dirty(id);
		// @todo process VatFreeSupplList
	}
}

int ArticleCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjArticle ar_obj;
	ArticleTbl::Rec rec;
	if(ar_obj.Search(id, &rec) > 0) {
	   	p_cache_rec->AccSheetID = rec.AccSheetID;
		p_cache_rec->Article = rec.Article;
		p_cache_rec->ObjID   = rec.ObjID;
		p_cache_rec->Flags   = rec.Flags;
		if(rec.Closed)
			p_cache_rec->Flags |= ARTRF_CLOSED;
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void ArticleCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	ArticleTbl::Rec * p_data_rec = static_cast<ArticleTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->ID       = p_cache_rec->ID;
	p_data_rec->AccSheetID = p_cache_rec->AccSheetID;
	p_data_rec->Article  = p_cache_rec->Article;
	p_data_rec->ObjID    = p_cache_rec->ObjID;
	p_data_rec->Closed   = BIN(p_cache_rec->Flags & ARTRF_CLOSED);
	p_data_rec->Flags    = p_cache_rec->Flags;
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}

IMPL_OBJ_FETCH(PPObjArticle, ArticleTbl::Rec, ArticleCache);

int FASTCALL IsSupplVATFree(PPID supplID)
{
	ArticleCache * p_cache = GetDbLocalCachePtr <ArticleCache> (PPOBJ_ARTICLE);
	return p_cache ? p_cache->IsSupplVatFree(supplID) : 0;
}

void FASTCALL PPObjArticle::Dirty(PPID id)
{
	ArticleCache * p_cache = GetDbLocalCachePtr <ArticleCache> (Obj, 0);
	CALLPTRMEMB(p_cache, Dirty(id));
	ArticleTbl::Rec rec;
	if(Fetch(id, &rec) > 0 && rec.ObjID) {
		PPObjAccSheet acc_sheet_obj;
		PPAccSheet acs_rec;
		if(acc_sheet_obj.Fetch(rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_LOCATION) {
			PPObjLocation loc_obj;
			loc_obj.Dirty(rec.ObjID);
		}
	}
}

const ArticleFilt * PPObjArticle::GetCurrFilt() const { return &CurrFilt; }

void PPObjArticle::SetCurrFilt(const ArticleFilt * pFilt)
{
	if(!RVALUEPTR(CurrFilt, pFilt)) {
		CurrFilt.Init(1, 0);
	}
}
//
//
//
PPDebtDim::PPDebtDim() // @v11.9.6
{
	THISZERO();
}

PPDebtDimPacket::PPDebtDimPacket()
{
}
//
// PPObjDebtLimit
//
PPObjDebtDim::PPObjDebtDim() : PPObjReference(PPOBJ_DEBTDIM, 0)
{
}

/*static*/PPID PPObjDebtDim::Select()
{
	PPID   id = 0;
	SString sub_title;
	ListWindow * p_lw = 0;
	PPObjDebtDim dd_obj;
	StrAssocArray * p_ary = dd_obj.MakeStrAssocList(0);
	THROW(p_ary);
	THROW(p_lw = CreateListWindow(p_ary, lbtDisposeData|lbtDblClkNotify));
	THROW(PPLoadText(PPTXT_SELECTDEBTDIM, sub_title));
	p_lw->setTitle(sub_title);
	p_lw->ViewOptions |= (ofCenterX | ofCenterY);
	if(ExecView(p_lw) == cmOK)
		p_lw->getResult(&id);
	else
		id = -1;
	CATCH
		id = 0;
		PPError();
	ENDCATCH
	delete p_lw;
	return id;
}

int PPObjDebtDim::Browse(void * extraPtr) { return RefObjView(this, 0, 0); }

class DebtDimDialog : public PPListDialog {
public:
	DebtDimDialog() : PPListDialog(DLG_DEBTDIM, CTL_DEBTDIM_AGENTS)
	{
	}
	int    setDTS(const PPDebtDimPacket *);
	int    getDTS(PPDebtDimPacket *);
public:
	virtual int setupList();
	virtual int delItem(long pos, long id);
	virtual int editItem(long pos, long id);

	PPDebtDimPacket Data;
};

int DebtDimDialog::setDTS(const PPDebtDimPacket * pData)
{
	RVALUEPTR(Data, pData);
	setCtrlData(CTL_DEBTDIM_ID, &Data.Rec.ID);
	setCtrlData(CTL_DEBTDIM_SYMB, Data.Rec.Symb);
	setCtrlData(CTL_DEBTDIM_NAME, Data.Rec.Name);
	disableCtrl(CTL_DEBTDIM_ID, LOGIC(Data.Rec.ID));
	updateList(-1);
	return 1;
}

int DebtDimDialog::getDTS(PPDebtDimPacket * pData)
{
	int    ok = 1;
	getCtrlData(CTL_DEBTDIM_ID, &Data.Rec.ID);
	getCtrlData(CTL_DEBTDIM_SYMB, Data.Rec.Symb);
	getCtrlData(CTL_DEBTDIM_NAME, Data.Rec.Name);
	THROW_PP(sstrlen(Data.Rec.Name) > 0, PPERR_NAMENEEDED);
	ASSIGN_PTR(pData, Data);
	CATCHZOK
	return ok;
}

int DebtDimDialog::setupList()
{
	int    ok = 1;
	SString agent_name;
	for(uint i = 0; i < Data.AgentList.GetCount(); i++) {
		PPID   agent_id = Data.AgentList.Get(i);
		GetArticleName(agent_id, agent_name.Z());
		if(!addStringToList(agent_id, agent_name))
			ok = PPErrorZ();
	}
	return ok;
}

int DebtDimDialog::editItem(long pos, long id)
{
	PPIDArray agent_list;
	Data.AgentList.CopyTo(&agent_list);
	{
		SString agent_name;
		ArticleFilt ar_filt;
		ar_filt.AccSheetID = GetAgentAccSheet();
		ar_filt.Ft_Closed = -1;
		ListToListData list_data(PPOBJ_ARTICLE, &ar_filt, &agent_list);
		for(int valid_data = 0; !valid_data && ListToListDialog(&list_data) > 0;) {
			int    intersect = 0;
			PPDebtDim dd_rec;
			PPObjDebtDim obj;
			valid_data = 1;
			for(PPID id = 0; valid_data && obj.EnumItems(&id, &dd_rec) > 0;) {
				PPDebtDimPacket dd_pack;
				if(id != Data.Rec.ID && obj.GetPacket(id, &dd_pack) > 0) {
					PPIDArray agent_list2;
					dd_pack.AgentList.CopyTo(&agent_list2);
					agent_list2.intersect(&agent_list);
					if(agent_list2.getCount()) {
						GetObjectName(PPOBJ_ARTICLE, agent_list2.at(0), agent_name);
						valid_data = 0;
					}
				}
			}
			if(!valid_data) {
				SString temp_buf, msg;
				PPLoadString(PPMSG_CONFIRMATION, PPCFM_DEBTDIMINTERSECTAGENTS, temp_buf);
				msg.Printf(temp_buf.cptr(), agent_name.cptr(), dd_rec.Name);
				if(PPOutputMessage(msg, mfConf|mfYes|mfNo) == cmYes)
					valid_data = 1;
			}
			if(valid_data) {
				Data.AgentList.Set(&agent_list);
				updateList(-1);
			}
		}
	}
	return -1;
}

int DebtDimDialog::delItem(long pos, long id) { return Data.AgentList.Remove(id, 0); }

int PPObjDebtDim::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	PPDebtDimPacket pack;
	DebtDimDialog * p_dlg = 0;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else
		pack.Rec.Tag = PPOBJ_DEBTDIM;
	if(r > 0) {
		THROW(CheckDialogPtr(&(p_dlg = new DebtDimDialog)));
		THROW(EditPrereq(pID, p_dlg, 0));
		p_dlg->setDTS(&pack);
		while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(p_dlg->getDTS(&pack) && CheckDupName(pack.Rec.ID, pack.Rec.Name)) {
				valid_data = 1;
				if(*pID)
					*pID = pack.Rec.ID;
				THROW(PutPacket(pID, &pack, 1));
			}
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

int PPObjDebtDim::GetPacket(PPID id, PPDebtDimPacket * pPack)
{
	int    ok = -1;
	if(PPCheckGetObjPacketID(Obj, id)) {
		PPIDArray agent_list;
		PPDebtDimPacket pack;
		THROW(Search(id, &pack.Rec) > 0);
		THROW(P_Ref->GetPropArray(PPOBJ_DEBTDIM, id, DBTDIMPRP_AGENTLIST, &agent_list));
		pack.AgentList.Set(&agent_list);
		ASSIGN_PTR(pPack, pack);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjDebtDim::PutPacket(PPID * pID, PPDebtDimPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			PPIDArray agent_list;
			pPack->AgentList.CopyTo(&agent_list);
			pPack->Rec.Tag = PPOBJ_DEBTDIM;
			if(*pID) {
				THROW(UpdateItem(*pID, &pPack->Rec, 0));
			}
			else {
				THROW(AddItem(pID, &pPack->Rec, 0));
			}
			THROW(P_Ref->PutPropArray(PPOBJ_DEBTDIM, *pID, DBTDIMPRP_AGENTLIST, &agent_list, 0));
		}
		else if(*pID) {
			THROW(RemoveObjV(*pID, 0, 0, 0));
			THROW(P_Ref->PutPropArray(PPOBJ_DEBTDIM, *pID, DBTDIMPRP_AGENTLIST, 0, 0));
		}
		THROW(tra.Commit());
		Dirty(*pID);
	}
	CATCHZOK
	return ok;
}

int PPObjDebtDim::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		if(_obj == PPOBJ_ARTICLE) {
			StrAssocArray * p_dd_list = MakeStrAssocList(0);
			if(p_dd_list) {
				uint count = p_dd_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					PPDebtDimPacket dd_pack;
					if(GetPacket(p_dd_list->Get(i).Id, &dd_pack) > 0 && dd_pack.AgentList.CheckID(_id))
						ok = RetRefsExistsErr(Obj, _id);
				}
			}
			ZDELETE(p_dd_list);
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_ARTICLE) {
			StrAssocArray * p_dd_list = MakeStrAssocList(0);
			if(p_dd_list) {
				uint count = p_dd_list->getCount();
				for(uint i = 0; ok == DBRPL_OK && i < count; i++) {
					uint pos = 0;
					PPDebtDimPacket dd_pack;
					if(GetPacket(p_dd_list->Get(i).Id, &dd_pack) > 0 && dd_pack.AgentList.Search(_id, &pos) > 0) {
						if(!dd_pack.AgentList.Update(pos, reinterpret_cast<long>(extraPtr)) || !PutPacket(&dd_pack.Rec.ID, &dd_pack, 0))
							ok = DBRPL_ERROR;
					}
				}
			}
			ZDELETE(p_dd_list);
			if(ok && !BroadcastObjMessage(DBMSG_OBJREPLACE, Obj, _id, extraPtr))
				ok = DBRPL_ERROR;
		}
	}
	return ok;
}

int PPObjDebtDim::Read(PPObjPack *p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjDebtDim, PPDebtDimPacket>(this, p, id, stream, pCtx); }

int  PPObjDebtDim::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPDebtDimPacket * p_pack = static_cast<PPDebtDimPacket *>(p->Data);
	THROW(p && p->Data);
	if(stream == 0) {
		int    is_new = 0;
		int    r = PutPacket(pID, p_pack, 1);
		if(!r) {
			pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTDEBTDIM, p_pack->Rec.ID, p_pack->Rec.Name);
			ok = -1;
		}
		else if(r > 0)
			ok = is_new ? 101 : 102; // @ObjectCreated : @ObjectUpdated
	}
	else {
		SBuffer buffer;
		THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
		THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
	}
	CATCHZOK
	return ok;
}

int  PPObjDebtDim::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPDebtDimPacket * p_pack = static_cast<PPDebtDimPacket *>(p->Data);
		int    do_replace_list = 0;
		PPIDArray temp_list;
		for(uint i = 0; i < p_pack->AgentList.GetCount(); i++) {
			PPID id = p_pack->AgentList.Get(i);
			THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &id, ary, replace));
			if(replace) {
				temp_list.add(id);
				do_replace_list = 1;
			}
		}
		if(do_replace_list)
			p_pack->AgentList.Set(&temp_list);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjDebtDim::SerializePacket(int dir, PPDebtDimPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	if(dir > 0) {
		PPIDArray agent_list;
		pPack->AgentList.CopyTo(&agent_list);
		THROW_SL(pSCtx->Serialize(dir, &agent_list, rBuf));
	}
	else if(dir < 0) {
		PPIDArray agent_list;
		THROW_SL(ok = pSCtx->Serialize(dir, &agent_list, rBuf));
		pPack->AgentList.Set(&agent_list);
	}
	CATCHZOK
	return ok;
}

int CorrectZeroDebtDimRefs()
{
	int    ok = -1;
	{
		Reference * p_ref = PPRef;
		PPObjDebtDim dd_obj;
		PPDebtDim dd_rec;
		PropertyTbl::Key1 k1;
		{
			const  PPID prop_id = ARTPRP_DEBTLIMLIST2;
			PPIDArray ar_list;
			TSVector <PPClientAgreement::DebtLimit> debt_lim_list;
			MEMSZERO(k1);
			k1.ObjType = PPOBJ_ARTICLE;
			k1.Prop = prop_id;
			if(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id) do {
				ar_list.add(p_ref->Prop.data.ObjID);
			} while(p_ref->Prop.search(1, &k1, spNext) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id);
			ar_list.sortAndUndup();
			for(uint i = 0; i < ar_list.getCount(); i++) {
				const  PPID ar_id = ar_list.get(i);
				debt_lim_list.clear();
				if(p_ref->GetPropArray(PPOBJ_ARTICLE, ar_id, prop_id, &debt_lim_list) > 0) {
					int    do_update = 0;
					uint   ri = debt_lim_list.getCount();
					if(ri) do {
						const  PPID dd_id = debt_lim_list.at(--ri).DebtDimID;
						if(dd_obj.Search(dd_id, &dd_rec) > 0) {
							;
						}
						else {
							debt_lim_list.atFree(ri);
							do_update = 1;
						}
					} while(ri);
					if(do_update) {
						THROW(p_ref->PutPropArray(PPOBJ_ARTICLE, ar_id, prop_id, &debt_lim_list, 1));
					}
				}
			}
		}
		{
			const  PPID prop_id = ARTPRP_SUPPLAGT2;
			PPIDArray ar_list;
			SBuffer _buf;
			MEMSZERO(k1);
			k1.ObjType = PPOBJ_ARTICLE;
			k1.Prop = prop_id;
			if(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id) do {
				ar_list.add(p_ref->Prop.data.ObjID);
			} while(p_ref->Prop.search(1, &k1, spNext) && k1.ObjType == PPOBJ_ARTICLE && k1.Prop == prop_id);
			ar_list.sortAndUndup();
			for(uint i = 0; i < ar_list.getCount(); i++) {
				const  PPID ar_id = ar_list.get(i);
				if(p_ref->GetPropSBuffer(PPOBJ_ARTICLE, ar_id, prop_id, _buf) > 0) {
					SSerializeContext ctx;
					PPSupplAgreement agt;
					if(agt.Serialize(-1, _buf, &ctx)) {
						int    do_update = 0;
						uint   ri = agt.Ep.DebtDimList.GetCount();
						if(ri) do {
							const  PPID dd_id = agt.Ep.DebtDimList.Get(--ri);
							if(dd_obj.Search(dd_id, &dd_rec) > 0) {
								;
							}
							else {
								agt.Ep.DebtDimList.RemoveByIdx(ri);
								do_update = 1;
							}
						} while(ri);
						if(do_update) {
							if(agt.Ep.DebtDimList.GetCount() == 0)
								agt.Ep.DebtDimList.Set(0);

							_buf.Z();
							if(agt.Serialize(+1, _buf, &ctx)) {
								THROW(p_ref->PutPropSBuffer(PPOBJ_ARTICLE, ar_id, prop_id, _buf, 1));
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class DebtDimCache : public ObjCache {
public:
	DebtDimCache() : ObjCache(PPOBJ_DEBTDIM, sizeof(DebtDimData)), P_AgentList(0)
	{
	}
	~DebtDimCache()
	{
		ZDELETE(P_AgentList);
	}
	virtual void FASTCALL Dirty(PPID id);
	int    FetchAgentList(LAssocArray * pAgentList);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct DebtDimData : public ObjCacheEntry {
		PPID   Reserve; // @reserve
	};
	LAssocArray * P_AgentList;
	ReadWriteLock AlLock;
};

int DebtDimCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	DebtDimData * p_cache_rec = static_cast<DebtDimData *>(pEntry);
	PPObjDebtDim dd_obj;
	PPDebtDim rec;
	if(dd_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
	// reserve
#undef CPY_FLD
		p_cache_rec->Reserve = 0;
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void DebtDimCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPDebtDim * p_data_rec = static_cast<PPDebtDim *>(pDataRec);
	const DebtDimData * p_cache_rec = static_cast<const DebtDimData *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_DEBTDIM;
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

void FASTCALL DebtDimCache::Dirty(PPID id)
{
	ObjCache::Dirty(id);
	{
		SRWLOCKER(AlLock, SReadWriteLocker::Write);
		ZDELETE(P_AgentList);
	}
}

int DebtDimCache::FetchAgentList(LAssocArray * pAgentList)
{
	int    ok = 0;
	{
		SRWLOCKER(AlLock, SReadWriteLocker::Read);
		if(!P_AgentList) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!P_AgentList) {
				P_AgentList = new LAssocArray;
				if(P_AgentList) {
					PPObjDebtDim dd_obj;
					PPDebtDim dd_rec;
					PPDebtDimPacket dd_pack;
					for(SEnum en = PPRef->Enum(PPOBJ_DEBTDIM, 0); en.Next(&dd_rec) > 0;) {
						if(dd_obj.GetPacket(dd_rec.ID, &dd_pack) > 0) {
							for(uint i = 0; i < dd_pack.AgentList.GetCount(); i++)
								P_AgentList->Add(dd_rec.ID, dd_pack.AgentList.Get(i), 0);
						}
					}
					ASSIGN_PTR(pAgentList, *P_AgentList);
					ok = 1;
				}
				else
					PPSetErrorNoMem();
			}
		}
		if(!ok && P_AgentList) {
			ASSIGN_PTR(pAgentList, *P_AgentList);
			ok = 1;
		}
	}
	return ok;
}

IMPL_OBJ_FETCH(PPObjDebtDim, PPDebtDim, DebtDimCache);
IMPL_OBJ_DIRTY(PPObjDebtDim, DebtDimCache);

int  PPObjDebtDim::FetchAgentList(LAssocArray * pList)
{
	DebtDimCache * p_cache = GetDbLocalCachePtr <DebtDimCache> (Obj);
	return p_cache ? p_cache->FetchAgentList(pList) : -1;
}
//
// Implementation of PPALDD_DebtDim
//
PPALDD_CONSTRUCTOR(DebtDim)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(DebtDim) { Destroy(); }

int PPALDD_DebtDim::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjDebtDim dd_obj;
		PPDebtDim rec;
		if(dd_obj.Fetch(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_Article::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))

	if(pF->Name == "?GetDebtDim") {
		long   dd_id = 0;
		if(H.ID) {
			PPObjDebtDim dd_obj;
			LAssocArray debt_dim_agent_list;
			PPIDArray dim_list;
			dd_obj.FetchAgentList(&debt_dim_agent_list);
			debt_dim_agent_list.GetListByVal(H.ID, dim_list);
			if(dim_list.getCount())
				dd_id = dim_list.get(0);
		}
		_RET_INT = dd_id;
	}
}
