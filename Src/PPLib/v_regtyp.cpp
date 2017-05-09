// V_REGTYP.CPP
// Copyright (c) A.Starodub 2010, 2013, 2015, 2016
// @codepage windows-1251
//
// Типы регистрационных документов 
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

IMPLEMENT_PPFILT_FACTORY(RegisterType); SLAPI RegisterTypeFilt::RegisterTypeFilt() : PPBaseFilt(PPFILT_REGISTERTYPE, 0, 0)
{
	SetFlatChunk(offsetof(RegisterTypeFilt, ReserveStart), offsetof(RegisterTypeFilt, ReserveEnd) - offsetof(RegisterTypeFilt, ReserveStart));
	Init(1, 0);
}

RegisterTypeFilt & FASTCALL RegisterTypeFilt::operator = (const RegisterTypeFilt & s)
{
	Copy(&s, 0);
	return *this;
}

SLAPI PPViewRegisterType::PPViewRegisterType() : PPView(&ObjRegT, &Filt, PPVIEW_REGISTERTYPE), Data(sizeof(RegTypeViewItem))
{
	ImplementFlags |= (implBrowseArray|implDontEditNullFilter);
}

SLAPI PPViewRegisterType::~PPViewRegisterType()
{
}

int SLAPI PPViewRegisterType::CheckForFilt(const PPRegisterTypePacket * pPack) const
{
	if(pPack) {
		long st = (pPack->Rec.Flags & (REGTF_LEGAL|REGTF_PRIVATE));
		if(Filt.RegOrgKind && Filt.RegOrgKind != pPack->Rec.RegOrgKind)
			return 0;
		if(Filt.PersonKindID && Filt.PersonKindID != pPack->Rec.PersonKindID)
			return 0;
		if(Filt.St && Filt.St != st)
			return 0;
	}
	return 1;
}

// static
int PPViewRegisterType::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewRegisterType * p_v = (PPViewRegisterType *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI PPViewRegisterType::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		RegTypeViewItem * p_item = (RegTypeViewItem *)pBlk->P_SrcData;
		switch(pBlk->ColumnN) {
			case 0: // ИД
				pBlk->Set(p_item->ID);
				break;
			case 1: // Наименование
				GetRegisterTypeName(p_item->ID, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 2: // Вид регистрирующей организации
				GetObjectName(PPOBJ_PRSNKIND, p_item->RegOrgKind, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 3: // Вид разрешенной персоналии
				GetObjectName(PPOBJ_PRSNKIND, p_item->PersonKindID, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 4: // Период действия в днях
				pBlk->Set((int32)p_item->ExpiryPrd);
				break;
			case 5: // Формат
				pBlk->Set(p_item->Format);
				break;
		}
	}
	return ok;
}

int SLAPI PPViewRegisterType::MakeListEntry(const PPRegisterTypePacket * pPack, RegTypeViewItem * pItem)
{
	int    ok = -1;
	if(pPack && pItem) {
		pItem->ID           = pPack->Rec.ID;
		pItem->RegOrgKind   = pPack->Rec.RegOrgKind;
		pItem->PersonKindID = pPack->Rec.PersonKindID;
		pItem->ExpiryPrd    = (long)pPack->Rec.ExpiryPeriod;
		pPack->Format.CopyTo(pItem->Format, sizeof(pItem->Format));
		ok = 1;
	}
	return ok;
}
//
//
//
int SLAPI PPViewRegisterType::EditBaseFilt(PPBaseFilt * pFilt)
{
	class RegTypeFiltDialog : public TDialog {
	public:
		RegTypeFiltDialog() : TDialog(DLG_FLTREGT)
		{
		}
		int    setDTS(const RegisterTypeFilt * pData)
		{
			Data = *pData;
			AddClusterAssocDef(CTL_FLTREGT_ST,  0, 0);
			AddClusterAssoc(CTL_FLTREGT_ST,  1, REGTF_PRIVATE);
			AddClusterAssoc(CTL_FLTREGT_ST,  2, REGTF_LEGAL);
			SetClusterData(CTL_FLTREGT_ST, Data.St);
			SetupPPObjCombo(this, CTLSEL_FLTREGT_REGPKIND, PPOBJ_PRSNKIND, Data.RegOrgKind, OLW_CANINSERT, 0);
			SetupPPObjCombo(this, CTLSEL_FLTREGT_PKIND, PPOBJ_PRSNKIND, Data.PersonKindID, 0, 0);
			return 1;
		}
		int    getDTS(RegisterTypeFilt * pData)
		{
			int ok = 1, sel = 0;
			GetClusterData(CTL_FLTREGT_ST, &Data.St);
			getCtrlData(CTLSEL_FLTREGT_REGPKIND,  &Data.RegOrgKind);
			getCtrlData(CTLSEL_FLTREGT_PKIND, &Data.PersonKindID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		RegisterTypeFilt Data;
	};
	int    ok = -1;
	RegisterTypeFilt filt;
	RegTypeFiltDialog * p_dlg = new RegTypeFiltDialog;
	filt.Copy(pFilt, 0);
	THROW(CheckDialogPtr(&p_dlg));
	p_dlg->setDTS(&filt);
	while(ok < 0 && ExecView(p_dlg) == cmOK) {
		if(p_dlg->getDTS(&filt) > 0) {
			CALLPTRMEMB(pFilt, Copy(&filt, 0));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}

int SLAPI PPViewRegisterType::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(FetchData(0));
	CATCHZOK
	return ok;
}

int SLAPI PPViewRegisterType::InitIteration()
{
	int    ok = 1;
	Counter.Init(Data.getCount());
	return ok;
}

int SLAPI PPViewRegisterType::NextIteration(RegTypeViewItem * pItem)
{
	int    ok = -1;
	if(pItem && Counter < Data.getCount()) {
		ASSIGN_PTR(pItem, *((RegTypeViewItem*)Data.at(Counter)));
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewRegisterType::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewRegisterType::GetDataForBrowser, this);
		ok = 1;
	}
	return ok;
}

SArray * SLAPI PPViewRegisterType::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(Data);
	uint   brw_id = BROWSER_REGISTERTYPE;
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewRegisterType::Transmit(int isCharry)
{
	int    ok = -1;
	PPWait(1);
	if(isCharry) {
		RegTypeViewItem item;
		PPIDArray id_list;
		for(InitIteration(); NextIteration(&item) > 0;)
			id_list.add(item.ID);
		THROW(SendCharryObject(PPDS_CRRREGISTERTYPE, id_list));
		ok = 1;
	}
	else {
		ObjTransmitParam param;
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			RegTypeViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			for(InitIteration(); NextIteration(&item) > 0;)
				objid_ary.Add(PPOBJ_REGISTERTYPE, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	PPWait(0);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewRegisterType::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_ADDITEM) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	PPIDArray  id_list;
	PPID   id = (pHdr) ? *(PPID *)pHdr : 0;
 	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (ObjRegT.Edit(&(id = 0), 0) == cmOK) ? 1 : -1;
				break;
			case PPVCMD_VIEWPERSONS:
				ok = -1;
				{
					PPRegisterType rt_rec;
					if(ObjRegT.Fetch(id, &rt_rec) > 0) {
						PersonFilt filt;
						filt.AttribType = PPPSNATTR_REGISTER;
						filt.RegTypeID = id;
						filt.EmptyAttrib = EA_NOEMPTY;
						if(rt_rec.PersonKindID)
							filt.Kind = rt_rec.PersonKindID;
						ViewPerson(&filt);
					}
				}
				break;
			case PPVCMD_TRANSMIT:
			case PPVCMD_TRANSMITCHARRY:
				ok = Transmit(BIN(ppvCmd == PPVCMD_TRANSMITCHARRY));
				break;
			case PPVCMD_REFRESHBYPERIOD:
			case PPVCMD_REFRESH:
				ok = 1;
				break;
		}
	}
	if(ok > 0 && oneof5(ppvCmd, PPVCMD_ADDITEM, PPVCMD_EDITITEM, PPVCMD_DELETEITEM, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		FetchData(id);
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			long   c = p_def->_curItem(); // @v8.0.5
			p_def->setArray(new SArray(Data), 0, 1);
			pBrw->go(c); // @v8.0.5
		}
		ok = 1;
	}
	return ok;
}

int SLAPI PPViewRegisterType::FetchData(long id)
{
	int    ok = 1;
	RegTypeViewItem item;
	if(id == 0) {
		Data.freeAll();
		for(PPID id = 0; ObjRegT.EnumItems(&id, 0) > 0;) {
			PPRegisterTypePacket pack;
			if(ObjRegT.GetPacket(id, &pack) > 0 && CheckForFilt(&pack) > 0) {
				MEMSZERO(item);
				MakeListEntry(&pack, &item);
				THROW_SL(Data.insert(&item));
			}
		}
	}
	else {
		uint pos = 0;
		int found = BIN(Data.lsearch(&id, &pos, CMPF_LONG) > 0);
		PPRegisterTypePacket pack;
		if(ObjRegT.GetPacket(id, &pack) > 0 && CheckForFilt(&pack) > 0) {
			MEMSZERO(item);
			MakeListEntry(&pack, &item);
			if(found)
				*((RegTypeViewItem*)Data.at(pos)) = item;
			else
				THROW_SL(Data.insert(&item));
		}
		else if(found)
			THROW_SL(Data.atFree(pos));
	}
	CATCHZOK
	return ok;
}
