// PALM.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <stylopalm.h>

PalmBillQueue::IdAssocItem::IdAssocItem(PPID dvcID, PPID outerBillID, PPID innerBillID) : DvcID(dvcID), OuterBillID(outerBillID), InnerBillID(innerBillID)
{
}

PalmBillQueue::PalmBillQueue() : SArray(sizeof(PalmBillPacket *), aryPtrContainer)
{
}

uint PalmBillQueue::GetItemsCount() const { return getCount(); }
bool PalmBillQueue::IsEmpty() const { return !getCount(); }
int  PalmBillQueue::Push(PalmBillPacket * pPack) { return insert(pPack) ? 1 : PPSetErrorSLib(); }

// @v10.7.2 #define COMPARE(a,b) ((a)>(b)) ? 1 : (((a)<(b)) ? -1 : 0)

IMPL_CMPFUNC(PalmBillPacket, i1, i2)
{
	const PalmBillPacket * p_i1 = static_cast<const PalmBillPacket *>(i1);
	const PalmBillPacket * p_i2 = static_cast<const PalmBillPacket *>(i2);
	// @v10.7.2 int    r = COMPARE(p_i1->Hdr.PalmID, p_i2->Hdr.PalmID);
	// @v10.7.2 return r ? r : COMPARE(p_i1->Hdr.ID, p_i2->Hdr.ID);
	int    si = 0; // @v10.7.2 
	CMPCASCADE2(si, p_i1, p_i2, Hdr.PalmID, Hdr.ID); // @v10.7.2 
	return si; // @v10.7.2 
}

int PalmBillQueue::PushUnique(PalmBillPacket * pPack)
{
	int    ok = -1;
	uint pos = 0;
	if(lsearch(pPack, &pos, PTR_CMPFUNC(PalmBillPacket))) {
		PalmBillPacket * p_pack = static_cast<PalmBillPacket *>(at(pos));
		ZDELETE(p_pack);
		atFree(pos);
		atInsert(pos, pPack);
		ok = 1;
	}
	else
		ok = Push(pPack);
	return ok;
}

PalmBillPacket * PalmBillQueue::Pop()
{
	PalmBillPacket * p_pack = 0;
	if(getCount()) {
		p_pack = static_cast<PalmBillPacket *>(at(0));
		atFree(0);
	}
	return p_pack;
}

PalmBillPacket * PalmBillQueue::Peek()
{
	return getCount() ? static_cast<PalmBillPacket *>(at(0)) : 0;
}

void PalmBillQueue::Destroy()
{
	while(!IsEmpty()) {
		PalmBillPacket * p_pack = Pop();
		delete p_pack;
	}
}

long PalmBillQueue::GetBillIdBias() const
{
	long   bias = 0;
	for(uint i = 0; i < getCount(); i++) {
		const PalmBillPacket * p_item = static_cast<const PalmBillPacket *>(at(i));
		if(p_item->Hdr.ID > bias)
			bias = p_item->Hdr.ID;
	}
	return bias;
}

int PalmBillQueue::AddItem(PPID billID, const PalmBillItem * pItem, uint * pPos)
{
	int    ok = -1;
	uint   pos = 0;
	PalmBillPacket * p_pack = 0;
	if(pPos && *pPos < getCount() && static_cast<const PalmBillPacket *>(at(*pPos))->Hdr.ID == billID) {
		pos = *pPos;
		ok = 1;
	}
	else {
		for(uint i = 0; i < getCount(); i++) {
			if(static_cast<const PalmBillPacket *>(at(i))->Hdr.ID == billID) {
				pos = i;
				ok = 1;
				break;
			}
		}
	}
	if(ok > 0) {
		p_pack = static_cast<PalmBillPacket *>(at(pos));
		if(p_pack->Hdr.ID == billID) {
			ok = p_pack->AddItem(pItem);
			ASSIGN_PTR(pPos, pos);
		}
		else
			ok = -1;
	}
	return ok;
}

PPID PalmBillQueue::GetInnerIdByOuter(PPID dvcID, PPID outerID) const
{
	uint   pos = 0;
	const  IdAssocItem key(dvcID, outerID);
	return IdAssocList.lsearch(&key, &pos, PTR_CMPFUNC(_2long)) ? IdAssocList.at(pos).InnerBillID : 0;
}

int PalmBillQueue::SetIdAssoc(PPID dvcID, PPID outerID, PPID innerID)
{
	int    ok = -1;
	uint   pos = 0;
	const  IdAssocItem key(dvcID, outerID);
	if(IdAssocList.lsearch(&key, &pos, PTR_CMPFUNC(_2long))) {
		IdAssocItem & r_item = IdAssocList.at(pos);
		if(r_item.InnerBillID == innerID)
			ok = -1;
		else if(r_item.InnerBillID == 0) {
			r_item.InnerBillID = innerID;
			ok = 1;
		}
		else {
			r_item.InnerBillID = innerID;
			ok = 100; // conflict
		}
	}
	else {
		IdAssocItem new_item(dvcID, outerID, innerID);
		IdAssocList.insert(&new_item);
		ok = 1;
	}
	return ok;
}
//
//
//
PalmBillItem::PalmBillItem() : GoodsID(0), Qtty(0.0), Price(0.0)
{
}

PalmBillPacket::PalmBillPacket() : SVector(sizeof(PalmBillItem))
{
	Init();
}

void PalmBillPacket::Init()
{
	MEMSZERO(Hdr);
	freeAll();
}

uint PalmBillPacket::GetItemsCount() const { return getCount(); }
int  PalmBillPacket::AddItem(const PalmBillItem * pItem) { return insert(pItem) ? 1 : PPSetErrorSLib(); }
int  PalmBillPacket::RemoveItem(uint idx) { return atFree(idx) ? 1 : PPSetErrorSLib(); }

int PalmBillPacket::EnumItems(uint * pIdx, PalmBillItem * pItem) const
{
	PalmBillItem * p_item;
	if(SVector::enumItems(pIdx, (void **)&p_item) > 0) {
		ASSIGN_PTR(pItem, *p_item);
		return 1;
	}
	return 0;
}
//
//
//
PPGeoTrackingMode::PPGeoTrackingMode() : Mode(0), Cycle(0)
{
}

PPGeoTrackingMode & PPGeoTrackingMode::Z()
{
	Mode = 0;
	Cycle = 0;
	return *this;
}

bool FASTCALL PPGeoTrackingMode::operator == (const PPGeoTrackingMode & rS) const { return (Mode == rS.Mode && Cycle == rS.Cycle); }
bool FASTCALL PPGeoTrackingMode::operator != (const PPGeoTrackingMode & rS) const { return (Mode != rS.Mode || Cycle != rS.Cycle); }

PPStyloPalmPacket::PPStyloPalmPacket() : P_Path(0), P_FTPPath(0)
{
	destroy();
}

PPStyloPalmPacket::~PPStyloPalmPacket()
{
	destroy();
}

void PPStyloPalmPacket::destroy()
{
	MEMSZERO(Rec);
	ZDELETE(P_Path);
	ZDELETE(P_FTPPath);
	LocList.Set(0);
	QkList__.Set(0);
}

PPStyloPalmPacket & FASTCALL PPStyloPalmPacket::operator = (const PPStyloPalmPacket & s)
{
	ZDELETE(P_Path);
	ZDELETE(P_FTPPath);
	Rec = s.Rec;
	if(s.P_Path)
		P_Path = newStr(s.P_Path);
	if(s.P_FTPPath)
		P_FTPPath = newStr(s.P_FTPPath);
	LocList = s.LocList;
	QkList__ = s.QkList__;
	return *this;
}

void PPStyloPalmPacket::Setup()
{
	if(P_Path && *strip(P_Path) == 0)
		ZDELETE(P_Path);
	if(P_FTPPath && *strip(P_FTPPath) == 0)
		ZDELETE(P_FTPPath);
}

int PPStyloPalmPacket::MakePath(const char * pSuffix, SString & rBuf) const
{
	rBuf = P_Path;
	if(rBuf.NotEmptyS()) {
		if(pSuffix)
			rBuf.SetLastSlash().Cat(pSuffix);
		SFile::CreateDir(rBuf);
		return 1;
	}
	else
		return 0;
}

int PPStyloPalmPacket::MakeOutputPath(SString & rBuf) const { return MakePath("IN", rBuf); }
int PPStyloPalmPacket::MakeInputPath(SString & rBuf) const { return (Rec.Flags & PLMF_ANDROID) ? MakePath(0, rBuf) : MakePath("OUT", rBuf); }
//
//
//
PalmPaneData::PalmPaneData()
{
	THISZERO();
}
//
//
//
static int StyloPalmListFilt(void * pRec, void * extraPtr)
{
	const long extra_param = reinterpret_cast<long>(extraPtr);
	const PPStyloPalm * p_rec = static_cast<const PPStyloPalm *>(pRec);
	return extra_param ? ((p_rec->Flags & extra_param) == extra_param) : 1;
}

PPObjStyloPalm::PPObjStyloPalm(void * extraPtr) : PPObjReference(PPOBJ_STYLOPALM, extraPtr)
{
	FiltProc = StyloPalmListFilt;
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

StrAssocArray * PPObjStyloPalm::MakeStrAssocList(void * extraPtr)
{
	StrAssocArray * p_list = new StrAssocArray();
	THROW_MEM(p_list);
	{
		const long extra_param = reinterpret_cast<long>(extraPtr);
		PPStyloPalm rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
			if((rec.Flags & extra_param) == extra_param) {
				if(*strip(rec.Name) == 0)
					ideqvalstr(rec.ID, rec.Name, sizeof(rec.Name));
				THROW_SL(p_list->Add(rec.ID, rec.GroupID, rec.Name));
			}
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjStyloPalm::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	PPID   k = 0;
	PPID   bill_id = 0;
	PPStyloPalm rec;
	if(msg == DBMSG_OBJDELETE) {
		switch(_obj) {
			case PPOBJ_GOODS:
			case PPOBJ_GOODSGROUP:
			case PPOBJ_LOCATION:
			case PPOBJ_OPRKIND:
			case PPOBJ_ARTICLE:
			case PPOBJ_INTERNETACCOUNT:
			case PPOBJ_STYLOPALM:
				{
					PPIDArray loc_list;
					for(SEnum en = P_Ref->Enum(Obj, 0); ok == DBRPL_OK && en.Next(&rec) > 0;) {
						switch(_obj) {
							case PPOBJ_GOODS:
							case PPOBJ_GOODSGROUP:
								if(rec.GoodsGrpID == _id)
									ok = RetRefsExistsErr(Obj, rec.ID);
								break;
							case PPOBJ_LOCATION:
								{
									loc_list.clear();
									P_Ref->GetPropArray(Obj, rec.ID, PLMPRP_LOCLIST, &loc_list);
									for(uint i = 0; i < loc_list.getCount(); i++)
										if(loc_list.get(i) == _id) {
											ok = RetRefsExistsErr(Obj, rec.ID);
											break;
										}
								}
								break;
							case PPOBJ_OPRKIND:
								if(rec.OrderOpID == _id)
									ok = RetRefsExistsErr(Obj, rec.ID);
								break;
							case PPOBJ_ARTICLE:
								if(rec.AgentID == _id)
									ok = RetRefsExistsErr(Obj, rec.ID);
								break;
							case PPOBJ_INTERNETACCOUNT:
								if(rec.FTPAcctID == _id)
									ok = RetRefsExistsErr(Obj, rec.ID);
								break;
							case PPOBJ_STYLOPALM:
								if(rec.GroupID == _id)
									ok = RetRefsExistsErr(Obj, rec.ID);
								break;
						}
					}
				}
				break;
		}
	}
	else if(msg == DBMSG_OBJREPLACE) {
		if(_obj == PPOBJ_ARTICLE) {
			for(SEnum en = P_Ref->Enum(Obj, 0); ok == DBRPL_OK && en.Next(&rec) > 0;) {
				if(rec.AgentID == _id) {
					Reference2Tbl::Key0 k0;
					k0.ObjType = Obj;
					k0.ObjID = rec.ID;
					if(SearchByKey_ForUpdate(P_Ref, 0, &k0, &rec) > 0) {
						rec.AgentID = reinterpret_cast<long>(extraPtr);
						if(!P_Ref->updateRecBuf(&rec))
							ok = DBRPL_ERROR;
					}
				}
			}
		}
	}
	return ok;
}

/*static*/int FASTCALL PPObjStyloPalm::ReadConfig(PPStyloPalmConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_STYLOPALMCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

/*static*/int PPObjStyloPalm::EditConfig()
{
	int    ok = -1;
	int    valid_data = 0;
	int    is_new = 0;
	PPStyloPalmConfig cfg;
	TDialog * dlg = new TDialog(DLG_SPIICFG);
	PPIDArray  op_list;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	THROW(CheckCfgRights(PPCFGOBJ_STYLOPALM, PPR_READ, 0));
	for(PPID op_id = 0; EnumOperations(PPOPT_INVENTORY, &op_id, &op_rec) > 0;) {
		PPInventoryOpEx  inv_op_ex;
		op_obj.FetchInventoryData(op_id, &inv_op_ex);
		if(inv_op_ex.Flags & INVOPF_INVBYCLIENT)
			op_list.add(op_id);
	}
	THROW(is_new = ReadConfig(&cfg));
	THROW(CheckDialogPtr(&dlg));
	SetupPPObjCombo(dlg, CTLSEL_SPIICFG_SELLOP, PPOBJ_OPRKIND, cfg.SellOpID, 0);
	dlg->setCtrlData(CTL_SPIICFG_SELLTERM, &cfg.SellAnlzTerm);
	SetupOprKindCombo(dlg, CTLSEL_SPIICFG_CLIINVOP, cfg.CliInvOpID, 0, &op_list, OPKLF_OPLIST);
	{
		ObjTagFilt ot_filt(PPOBJ_BILL);
		ot_filt.Flags |= ObjTagFilt::fOnlyTags;
		SetupObjTagCombo(dlg, CTLSEL_SPIICFG_INHBTAG, cfg.InhBillTagID, 0, &ot_filt);
	}
	dlg->AddClusterAssocDef(CTL_SPIICFG_ORDCFMTTYP,  0, PPStyloPalmConfig::ordercodeIdHash);
	dlg->AddClusterAssoc(CTL_SPIICFG_ORDCFMTTYP,  1, PPStyloPalmConfig::ordercodeSymbName);
	dlg->AddClusterAssoc(CTL_SPIICFG_ORDCFMTTYP,  2, PPStyloPalmConfig::ordercodeNameSymb);
	dlg->SetClusterData(CTL_SPIICFG_ORDCFMTTYP, cfg.OrderCodeFormatType);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		THROW(CheckCfgRights(PPCFGOBJ_STYLOPALM, PPR_MOD, 0));
		dlg->getCtrlData(CTLSEL_SPIICFG_SELLOP, &cfg.SellOpID);
		dlg->getCtrlData(CTL_SPIICFG_SELLTERM, &cfg.SellAnlzTerm);
		dlg->getCtrlData(CTLSEL_SPIICFG_CLIINVOP, &cfg.CliInvOpID);
		dlg->GetClusterData(CTL_SPIICFG_ORDCFMTTYP, &cfg.OrderCodeFormatType);
		dlg->getCtrlData(CTLSEL_SPIICFG_INHBTAG, &cfg.InhBillTagID);
		{
			PPTransaction tra(1);
			THROW(tra);
			THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_STYLOPALMCFG, &cfg, sizeof(cfg), 0));
			DS.LogAction(is_new == -1 ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_STYLOPALM, 0, 0, 0);
			THROW(tra.Commit());
		}
		ok = valid_data = 1;
	}
	CATCHZOKPPERR
//turistti
	delete dlg;
	return ok;
}

static int EditGeoTrackingMode(PPGeoTrackingMode * pData)
{
	class GeoTrackingModeDialog : public TDialog {
		DECL_DIALOG_DATA(PPGeoTrackingMode);
	public:
		GeoTrackingModeDialog() : TDialog(DLG_GEOTRACKMODE)
		{
		}
		DECL_DIALOG_SETDTS()
		{
            int    ok = 1;
            RVALUEPTR(Data, pData);
            AddClusterAssoc(CTL_GEOTRACKMODE_FLAGS, 0, GTMF_AUTO);
            AddClusterAssoc(CTL_GEOTRACKMODE_FLAGS, 1, GTMF_CHECKIN);
            AddClusterAssoc(CTL_GEOTRACKMODE_FLAGS, 2, GTMF_EVNT_BILL);
            AddClusterAssoc(CTL_GEOTRACKMODE_FLAGS, 3, GTMF_FORCEGPSON);
            SetClusterData(CTL_GEOTRACKMODE_FLAGS, Data.Mode);
            setCtrlLong(CTL_GEOTRACKMODE_CYCLE, Data.Cycle / 1000);
            disableCtrl(CTL_GEOTRACKMODE_CYCLE, !(Data.Mode & GTMF_AUTO));
            return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_GEOTRACKMODE_FLAGS, &Data.Mode);
			Data.Cycle = getCtrlLong(CTL_GEOTRACKMODE_CYCLE) * 1000;
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_GEOTRACKMODE_FLAGS)) {
				GetClusterData(CTL_GEOTRACKMODE_FLAGS, &Data.Mode);
				disableCtrl(CTL_GEOTRACKMODE_CYCLE, !(Data.Mode & GTMF_AUTO));
			}
			else
				return;
			clearEvent(event);
		}
	};
    DIALOG_PROC_BODY(GeoTrackingModeDialog, pData);
}
//
// @todo @v4.6.8 Если запись PPStyloPalmPacket имеет ненулевую группу, то в диалоге
// необходимо запретить редактирование наследуемых параметров, а также предусмотреть
// изменение наследуемых параметров при изменении принадлежности к группе.
//
class StyloPalmDialog : public TDialog {
	DECL_DIALOG_DATA(PPStyloPalmPacket);
	enum {
		ctlgroupLoc      = 1,
		ctlgroupQuotKind = 2
	};
public:
	StyloPalmDialog(uint dlgID) : TDialog(dlgID)
	{
		PPObjStyloPalm::ReadConfig(&SpCfg);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_PALM_PATH, CTL_PALM_PATH, 1, 0, 0, FileBrowseCtrlGroup::fbcgfPath);
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_PALM_LOC, 0, 0, cmLocList, 0, 0, 0));
		addGroup(ctlgroupQuotKind, new QuotKindCtrlGroup(CTLSEL_PALM_QUOTKIND, cmQuotKindList));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		const   PPID   agent_accsheet_id = GetAgentAccSheet();
		setCtrlData(CTL_PALM_NAME, Data.Rec.Name);
		setCtrlData(CTL_PALM_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_PALM_ID,   &Data.Rec.ID);
		disableCtrl(CTL_PALM_ID, (!PPMaster || Data.Rec.ID));
		{
			const long dvc_type = (Data.Rec.Flags & PLMF_ANDROID) ? 2 : 1;
			AddClusterAssoc(CTL_PALM_DVCTYPE, 0,  1);
			AddClusterAssocDef(CTL_PALM_DVCTYPE, 1,  2);
			SetClusterData(CTL_PALM_DVCTYPE, dvc_type);
		}
		SetupPPObjCombo(this, CTLSEL_PALM_GROUP, PPOBJ_STYLOPALM, Data.Rec.GroupID, OLW_CANSELUPLEVEL, reinterpret_cast<void *>(PLMF_GENERIC));
		SetupArCombo(this, CTLSEL_PALM_AGENT, Data.Rec.AgentID, OLW_CANINSERT, agent_accsheet_id, sacfDisableIfZeroSheet);
		{
			LocationCtrlGroup::Rec l_rec(&Data.LocList);
			setGroupData(ctlgroupLoc, &l_rec);
		}
		{
			QuotKindCtrlGroup::Rec qk_rec(&Data.QkList__);
			setGroupData(ctlgroupQuotKind, &qk_rec);
			// @v11.7.1 {
			AddClusterAssocDef(CTL_PALM_QUOTKINDOPTIONS, 0, PPStyloPalm2::qkoNone);
			AddClusterAssoc(CTL_PALM_QUOTKINDOPTIONS, 1, PPStyloPalm2::qkoAuto);
			AddClusterAssoc(CTL_PALM_QUOTKINDOPTIONS, 2, PPStyloPalm2::qkoManual);
			SetClusterData(CTL_PALM_QUOTKINDOPTIONS, Data.Rec.QuotKindOptions);
			// } @v11.7.1 
		}
		SetupPPObjCombo(this, CTLSEL_PALM_GGRP, PPOBJ_GOODSGROUP, Data.Rec.GoodsGrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN);
		SetupPPObjCombo(this, CTLSEL_PALM_FTPACCT, PPOBJ_INTERNETACCOUNT, Data.Rec.FTPAcctID, 0, reinterpret_cast<void *>(PPObjInternetAccount::filtfFtp));
		PPIDArray op_type_list;
		op_type_list.addzlist(PPOPT_GOODSORDER, PPOPT_GOODSEXPEND, 0L);
		SetupOprKindCombo(this, CTLSEL_PALM_OP, Data.Rec.OrderOpID, 0, &op_type_list, 0);
		if(getCtrlView(CTLSEL_PALM_INHBTAGVAL)) {
			int    disable_inhtagval = 1;
			if(SpCfg.InhBillTagID) {
				PPObjectTag tag_rec;
				if(TagObj.Fetch(SpCfg.InhBillTagID, &tag_rec) > 0 && oneof2(tag_rec.TagDataType, OTTYP_OBJLINK, OTTYP_ENUM)) {
					SetupPPObjCombo(this, CTLSEL_PALM_INHBTAGVAL, tag_rec.TagEnumID, Data.Rec.InhBillTagVal, OLW_CANINSERT, reinterpret_cast<void *>(tag_rec.LinkObjGrp));
					disable_inhtagval = 0;
				}
			}
			disableCtrl(CTLSEL_PALM_INHBTAGVAL, disable_inhtagval);
		}
		setCtrlString(CTL_PALM_PATH, temp_buf = Data.P_Path);
		setCtrlString(CTL_PALM_FTPPATH, temp_buf = Data.P_FTPPath);
		setCtrlData(CTL_PALM_MAXNOTSENTORD, &Data.Rec.MaxUnsentOrders);
		setCtrlData(CTL_PALM_MAXSENTDAYS, &Data.Rec.TransfDaysAgo);
		AddClusterAssoc(CTL_PALM_FLAGS, 0, PLMF_INHFLAGS);
		AddClusterAssoc(CTL_PALM_FLAGS, 1, PLMF_IMPASCHECKS);
		AddClusterAssoc(CTL_PALM_FLAGS, 2, PLMF_EXPCLIDEBT);
		AddClusterAssoc(CTL_PALM_FLAGS, 3, PLMF_EXPSELL);
		AddClusterAssoc(CTL_PALM_FLAGS, 4, PLMF_EXPBRAND);
		AddClusterAssoc(CTL_PALM_FLAGS, 5, PLMF_EXPLOC);
		AddClusterAssoc(CTL_PALM_FLAGS, 6, PLMF_EXPSTOPFLAG);
		AddClusterAssoc(CTL_PALM_FLAGS, 7, PLMF_DISABLCEDISCOUNT);
		if(!(Data.Rec.Flags & PLMF_GENERIC)) {
			AddClusterAssoc(CTL_PALM_FLAGS, 8, PLMF_BLOCKED);
			AddClusterAssoc(CTL_PALM_FLAGS, 9, PLMF_TREATDUEDATEASDATE); // @v11.4.3 @fix
			AddClusterAssoc(CTL_PALM_FLAGS, 10, PLMF_EXPZSTOCK); // @v11.4.3
			AddClusterAssoc(CTL_PALM_FLAGS, 11, PLMF_EXPGOODSEXPIRYTAGS); // @v11.6.2
		}
		else {
			AddClusterAssoc(CTL_PALM_FLAGS, 8, PLMF_TREATDUEDATEASDATE); // @v10.8.11 // @v11.4.3 @fix
			AddClusterAssoc(CTL_PALM_FLAGS, 9, PLMF_EXPZSTOCK); // @v11.4.3
			AddClusterAssoc(CTL_PALM_FLAGS, 10, PLMF_EXPGOODSEXPIRYTAGS); // @v11.6.2
		}
		SetClusterData(CTL_PALM_FLAGS, Data.Rec.Flags);
		{
			temp_buf.Z();
			if(!(Data.Rec.Flags & PLMF_GENERIC)) {
				temp_buf.Cat(Data.Rec.RegisterTime, DATF_DMY|DATF_CENTURY, TIMF_HMS);
				setCtrlString(CTL_PALM_REGINFO, temp_buf);
				AddClusterAssoc(CTL_PALM_REGISTERED, 0, PLMF_REGISTERED);
				SetClusterData(CTL_PALM_REGISTERED, Data.Rec.Flags);
			}
		}
		SetupInheritance();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		int    sel = 0;
		SString path;
		PPStyloPalm sp_rec;
		{
			long   dvc_type = 0;
			GetClusterData(CTL_PALM_DVCTYPE, &dvc_type);
			SETFLAG(Data.Rec.Flags, PLMF_ANDROID, dvc_type == 2);
		}
		getCtrlData(sel = CTL_PALM_NAME, Data.Rec.Name);
		THROW(SpObj.CheckName(Data.Rec.ID, strip(Data.Rec.Name), 1));
		getCtrlData(sel = CTL_PALM_SYMB, Data.Rec.Symb);
		THROW(PPRef->CheckUniqueSymb(PPOBJ_STYLOPALM, Data.Rec.ID, strip(Data.Rec.Symb), offsetof(ReferenceTbl::Rec, Symb)));
		getCtrlData(CTL_PALM_ID,  &Data.Rec.ID);
		getCtrlData(sel = CTLSEL_PALM_GROUP, &Data.Rec.GroupID);
		if(Data.Rec.GroupID) {
			PPIDArray inner_stack;
			THROW(SpObj.Search(Data.Rec.GroupID, &sp_rec) > 0);
			THROW_PP_S(sp_rec.Flags & PLMF_GENERIC, PPERR_STYLOPALMPARENTNOTGENERIC, sp_rec.Name);
			THROW_PP(Data.Rec.GroupID != Data.Rec.ID, PPERR_STYLOPALMLOOP);
			inner_stack.add(Data.Rec.ID);
			inner_stack.add(sp_rec.ID);
			while(sp_rec.GroupID && SpObj.Search(sp_rec.GroupID, &sp_rec) > 0) {
				int r = inner_stack.addUnique(sp_rec.ID);
				THROW_SL(r);
				THROW_PP_S(r > 0, PPERR_STYLOPALMCYCLE, Data.Rec.Name);
			}
		}
		getCtrlData(CTLSEL_PALM_AGENT, &Data.Rec.AgentID);
		{
			LocationCtrlGroup::Rec l_rec;
			getGroupData(ctlgroupLoc, &l_rec);
			Data.LocList = l_rec.LocList;
			THROW_PP(Data.LocList.GetCount(), PPERR_STYLOPALMEMPTYWHLIST); // @v11.6.2
		}
		{
			QuotKindCtrlGroup::Rec qk_rec;
			getGroupData(ctlgroupQuotKind, &qk_rec);
			Data.QkList__ = qk_rec.List;
			GetClusterData(CTL_PALM_QUOTKINDOPTIONS, &Data.Rec.QuotKindOptions); // @v11.7.1
		}
		getCtrlData(CTLSEL_PALM_GGRP,  &Data.Rec.GoodsGrpID);
		getCtrlData(CTLSEL_PALM_OP,    &Data.Rec.OrderOpID);
		if(getCtrlView(CTLSEL_PALM_INHBTAGVAL) && SpCfg.InhBillTagID) {
			PPObjectTag tag_rec;
			if(TagObj.Fetch(SpCfg.InhBillTagID, &tag_rec) > 0 && oneof2(tag_rec.TagDataType, OTTYP_OBJLINK, OTTYP_ENUM))
				Data.Rec.InhBillTagVal = getCtrlLong(CTLSEL_PALM_INHBTAGVAL);
		}
		getCtrlData(CTLSEL_PALM_FTPACCT, &Data.Rec.FTPAcctID);
		getCtrlString(CTL_PALM_PATH,     path);
		getCtrlData(CTL_PALM_MAXNOTSENTORD, &Data.Rec.MaxUnsentOrders);
		getCtrlData(CTL_PALM_MAXSENTDAYS, &Data.Rec.TransfDaysAgo);
		ZDELETE(Data.P_Path);
		if(path.NotEmptyS()) {
			THROW_MEM(Data.P_Path = newStr(path));
		}
		getCtrlString(CTL_PALM_FTPPATH,  path);
		ZDELETE(Data.P_FTPPath);
		if(path.NotEmptyS()) {
			THROW_MEM(Data.P_FTPPath = newStr(path));
		}
		GetClusterData(CTL_PALM_FLAGS, &Data.Rec.Flags);
		if(!(Data.Rec.Flags & PLMF_GENERIC)) {
			GetClusterData(CTL_PALM_REGISTERED, &Data.Rec.Flags);
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PALM_GROUP))
			SetupInheritance();
		else if(event.isClusterClk(CTL_PALM_FLAGS))
			SetupInheritance();
		else if(event.isCmd(cmGeoTracking))
			EditGeoTrackingMode(&Data.Rec.Gtm);
		else
			return;
		clearEvent(event);
	}
	void   SetupInheritance()
	{
		PPStyloPalm sp_rec;
		Data.Rec.GroupID = getCtrlLong(CTLSEL_PALM_GROUP);
		GetClusterData(CTL_PALM_FLAGS, &Data.Rec.Flags);
		long   flags2 = Data.Rec.Flags;
		if(Data.Rec.GroupID) {
			if(SpObj.Search(Data.Rec.GroupID, &sp_rec) > 0) {
				if(flags2 & PLMF_INHFLAGS) {
					flags2 &= ~PLMF_INHMASK;
					flags2 |= (sp_rec.Flags & PLMF_INHMASK);
					// @v11.7.1 {
					Data.Rec.QuotKindOptions = sp_rec.QuotKindOptions; 
					SetClusterData(CTL_PALM_QUOTKINDOPTIONS, Data.Rec.QuotKindOptions);
					// } @v11.7.1
				}
			}
			else
				setCtrlLong(CTLSEL_PALM_GROUP, Data.Rec.GroupID = 0);
		}
		if(!Data.Rec.GroupID)
			flags2 &= ~PLMF_INHFLAGS;
		if(flags2 != Data.Rec.Flags)
			SetClusterData(CTL_PALM_FLAGS, Data.Rec.Flags = flags2);
		const bool dsbl_flags = (Data.Rec.GroupID && flags2 & PLMF_INHFLAGS);
		DisableClusterItem(CTL_PALM_FLAGS, 0, !Data.Rec.GroupID);
		// @v11.6.2 {
		const long idx_list[] = {1L, 2L, 3L, 4L, 5L, 6L, 7L};
		LongArray items_to_disable(idx_list, SIZEOFARRAY(idx_list));
		if(!(Data.Rec.Flags & PLMF_GENERIC))
			items_to_disable.addr(9).addr(10).addr(11);
		else
			items_to_disable.addr(8).addr(9).addr(10);
		DisableClusterItems(CTL_PALM_FLAGS, items_to_disable, dsbl_flags);
		disableCtrl(CTL_PALM_QUOTKINDOPTIONS, dsbl_flags); // @v11.7.1
		// } @v11.6.2 
		/* @v11.6.2 {
		DisableClusterItem(CTL_PALM_FLAGS, 1, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 2, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 3, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 4, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 5, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 6, dsbl_flags);
		DisableClusterItem(CTL_PALM_FLAGS, 7, dsbl_flags);
		if(!(Data.Rec.Flags & PLMF_GENERIC)) {
			DisableClusterItem(CTL_PALM_FLAGS, 9, dsbl_flags); // @v11.4.3
			DisableClusterItem(CTL_PALM_FLAGS, 10, dsbl_flags); // @v11.4.3
			DisableClusterItem(CTL_PALM_FLAGS, 11, dsbl_flags); // @v11.6.2
		}
		else {
			DisableClusterItem(CTL_PALM_FLAGS, 8, dsbl_flags); // @v11.4.3
			DisableClusterItem(CTL_PALM_FLAGS, 9, dsbl_flags); // @v11.4.3
			DisableClusterItem(CTL_PALM_FLAGS, 10, dsbl_flags); // @v11.6.2
		} */
	}
	PPObjStyloPalm SpObj;
	PPObjTag TagObj;
	PPStyloPalmConfig SpCfg;
};

int PPObjStyloPalm::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	bool   is_new = false;
	uint   dlg_id = 0;
	PPStyloPalmPacket pack;
	StyloPalmDialog * dlg = 0;
	THROW(EditPrereq(pID, 0, &is_new));
	if(is_new) {
		uint   s = 0;
		if(SelectorDialog(DLG_SELNEWPALM, CTL_SELNEWPALM_WHAT, &s) > 0)
			if(s == 0)
				dlg_id = DLG_PALM;
			else if(s == 1) {
				pack.Rec.Flags |= PLMF_ANDROID;
				dlg_id = DLG_PALM;
			}
			else {
				pack.Rec.Flags |= PLMF_GENERIC;
				dlg_id = DLG_PALMGROUP;
			}
	}
	else {
		THROW(GetPacket(*pID, &pack) > 0);
		dlg_id = (pack.Rec.Flags & PLMF_GENERIC) ? DLG_PALMGROUP : DLG_PALM;
	}
	if(dlg_id) {
		THROW(CheckDialogPtr(&(dlg = new StyloPalmDialog(dlg_id))));
		dlg->setDTS(&pack);
		THROW(EditPrereq(pID, dlg, 0));
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(dlg->getDTS(&pack)) {
				valid_data = 1;
				THROW(PutPacket(pID, &pack, 1));
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int PPObjStyloPalm::PutPacket(PPID * pID, PPStyloPalmPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPStyloPalm parent_rec;
		const    PPID parent_id = pPack->Rec.GroupID;
		THROW_PP(!pPack || pPack->Rec.Name[0], PPERR_NAMENEEDED); // @v10.1.6 @fix (!pPack ||)
        THROW_PP_S(!parent_id || (Search(parent_id, &parent_rec) > 0 && parent_rec.Flags & PLMF_GENERIC), PPERR_INVSTYLOPARENT, parent_id);
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				PPStyloPalmPacket org_pack;
				THROW(GetPacket(*pID, &org_pack) > 0);
				if(!IsPacketEq(*pPack, org_pack, 0)) {
					THROW(CheckDupName(*pID, pPack->Rec.Name));
					THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
					THROW(CheckRights(PPR_MOD));
					THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				}
				else
					ok = -1;
			}
			else {
				THROW(CheckRights(PPR_DEL));
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			THROW(CheckRights(PPR_INS));
			THROW(CheckDupName(*pID, pPack->Rec.Name));
			THROW(CheckDupSymb(*pID, pPack->Rec.Symb));
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		if(ok > 0) {
			THROW(P_Ref->PutPropVlrString(Obj, *pID, PLMPRP_PATH, pPack ? pPack->P_Path : 0));
			THROW(P_Ref->PutPropVlrString(Obj, *pID, PLMPRP_FTPPATH, pPack ? pPack->P_FTPPath : 0));
			THROW(P_Ref->PutPropArray(Obj, *pID, PLMPRP_LOCLIST, pPack ? &pPack->LocList.Get() : 0, 0));
			THROW(P_Ref->PutPropArray(Obj, *pID, PLMPRP_QUOTKINDLIST, pPack ? &pPack->QkList__.Get() : 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjStyloPalm::GetLocList(PPID id, ObjIdListFilt & rLocList)
{
	PPIDArray list;
	int    ok = P_Ref->GetPropArray(Obj, id, PLMPRP_LOCLIST, &list);
	rLocList.Set((ok > 0) ? &list : 0);
	return ok;
}

int PPObjStyloPalm::GetQuotKindList(PPID id, ObjIdListFilt & rQuotKindList)
{
	PPIDArray list;
	int    ok = P_Ref->GetPropArray(Obj, id, PLMPRP_QUOTKINDLIST, &list);
	rQuotKindList.Set((ok > 0) ? &list : 0);
	return ok;
}

int PPObjStyloPalm::Helper_GetPacket(PPID id, PPStyloPalmPacket * pPack, PPIDArray * pStack)
{
	int    ok = -1;
	PPIDArray inner_stack;
	ZDELETE(pPack->P_Path);
	ZDELETE(pPack->P_FTPPath);
	pPack->LocList.Set(0);
	if(Search(id, &pPack->Rec) > 0) {
		SString path;
		THROW(P_Ref->GetPropVlrString(Obj, id, PLMPRP_PATH, path));
		THROW(GetLocList(id, pPack->LocList));
		THROW(GetQuotKindList(id, pPack->QkList__));
		THROW_MEM(pPack->P_Path = newStr(path));
		THROW(P_Ref->GetPropVlrString(Obj, id, PLMPRP_FTPPATH, path));
		THROW_MEM(pPack->P_FTPPath = newStr(path));
		if(pPack->Rec.GroupID) {
			PPStyloPalmPacket group_pack;
			SETIFZ(pStack, &inner_stack);
			if(pStack->addUnique(pPack->Rec.GroupID) < 0) {
				//
				// Не завершаем функцию, дабы не смотря на рекурсию она могла работать.
				// В журнале pperror.log появится информация о проблеме.
				//
				PPSetError(PPERR_STYLOPALMCYCLE, pPack->Rec.Name);
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			}
			else if(Helper_GetPacket(pPack->Rec.GroupID, &group_pack, pStack) > 0) { // @recursion
				SETIFZ(pPack->Rec.OrderOpID, group_pack.Rec.OrderOpID);
				if(pPack->Rec.Flags & PLMF_INHFLAGS) {
					pPack->Rec.Flags &= ~PLMF_INHMASK;
					pPack->Rec.Flags |= (group_pack.Rec.Flags & PLMF_INHMASK);
					pPack->Rec.QuotKindOptions = group_pack.Rec.QuotKindOptions; // @v11.7.1
				}
				if(!pPack->LocList.GetCount() && group_pack.LocList.GetCount())
					pPack->LocList = group_pack.LocList;
				SETIFZ(pPack->Rec.FTPAcctID, group_pack.Rec.FTPAcctID);
				SETIFZ(pPack->Rec.GoodsGrpID, group_pack.Rec.GoodsGrpID);
				SETIFZ(pPack->Rec.MaxUnsentOrders, group_pack.Rec.MaxUnsentOrders);
				SETIFZ(pPack->Rec.TransfDaysAgo,   group_pack.Rec.TransfDaysAgo);
			}
			else {
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			}
		}
		pPack->Setup();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjStyloPalm::GetPacket(PPID id, PPStyloPalmPacket * pPack)
{
	return Helper_GetPacket(id, pPack, 0);
}

struct PdaClientRec {
	long   ID;
	char   Name[48];
	char   Code[16];
};

static int SearchClientInPdaTbl(const char * pInTblPath, PPID id, PdaClientRec * pRec)
{
	int    ok = -1;
	DbfTable * p_cli_tbl = PPOpenDbfTable(pInTblPath);
	THROW(p_cli_tbl);
	if(p_cli_tbl->top())
		do {
			PPID   client_id = 0;
			DbfRecord rec(p_cli_tbl);
			rec.get(1, client_id);
			if(client_id == id) {
				pRec->ID = client_id;
				rec.get(2, pRec->Name, sizeof(pRec->Name));
				rec.get(3, pRec->Code, sizeof(pRec->Code));
				ok = 1;
			}
		} while(ok < 0 && p_cli_tbl->next());
	CATCHZOK
	delete p_cli_tbl;
	return ok;
}

int PPObjStyloPalm::ResolveClientID(const char * pInTblPath, PPID inID, PPID * pOutID)
{
	int    ok = -1;
	PPID   acsh_id = GetSellAccSheet();
	PPAccSheet acs_rec;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	if(inID > 0) {
		if(ar_obj.Fetch(inID, &ar_rec) > 0) {
			ASSIGN_PTR(pOutID, ar_rec.ID);
			ok = 1;
		}
	}
	else if(inID < 0) {
		PdaClientRec pda_rec;
		if(acsh_id && SearchClientInPdaTbl(pInTblPath, inID, &pda_rec) > 0) {
			if(ar_obj.P_Tbl->SearchName(acsh_id, pda_rec.Name, &ar_rec) > 0) {
				ASSIGN_PTR(pOutID, ar_rec.ID);
				ok = 1;
			}
			else if(SearchObject(PPOBJ_ACCSHEET, acsh_id, &acs_rec) > 0)
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					PPID   psn_id = 0;
					PPID   ar_id = 0;
					PPObjPerson psn_obj;
					THROW(psn_obj.AddSimple(&psn_id, pda_rec.Name, acs_rec.ObjGroup, PPPRS_LEGAL, 1));
					if(ar_obj.P_Tbl->SearchObjRef(acsh_id, psn_id) < 0) {
						THROW(ar_obj.CreateObjRef(&ar_id, acsh_id, psn_id, 0, 1));
						ASSIGN_PTR(pOutID, ar_id);
						ok = 1;
					}
				}
				else if(acs_rec.Assoc == 0) {
					PPID   ar_id = 0;
					THROW(ar_obj.AddSimple(&ar_id, acsh_id, pda_rec.Name, 0, 1));
					ASSIGN_PTR(pOutID, ar_id);
					ok = 1;
				}
		}
	}
	CATCHZOK
	return ok;
}

bool PPObjStyloPalm::CheckSignalForInput(const char * pPath)
{
	SString sig_file_name;
	if(fileExists((sig_file_name = pPath).SetLastSlash().Cat("sp_ready"))) {
		//SFile::Remove(sig_file_name);
		return true;
	}
	else
		return false;
}

void PPObjStyloPalm::ClearInputSemaphore(const char * pPath)
{
	SString sig_file_name;
	SFile::Remove((sig_file_name = pPath).SetLastSlash().Cat("sp_ready"));
}

/*static char * GetImpFileName__(const char * pPath, uint fnameID, char * pBuf, size_t bufLen)
{
	SString fname;
	PPGetFileName(fnameID, fname);
	return strcat(setLastSlash(strnzcpy(pBuf, pPath, bufLen)), fname);
}*/

static SString & GetImpFileName(const char * pPath, uint fnameID, SString & rBuf)
{
	SString fname;
	PPGetFileName(fnameID, fname);
	return (rBuf = pPath).SetLastSlash().Cat(fname);
	//return strcat(setLastSlash(strnzcpy(pBuf, pPath, bufLen)), fname);
}

int PPObjStyloPalm::ReadInputToDo(PPStyloPalm * pRec, const char * pPath, PalmInputParam * pParam, long /*flags*/, PPLogger * pLogger)
{
	int    ok = 1;
	DbfTable * p_todo_tbl = 0;
	if(pParam && pParam->P_ToDoQueue) {
		SString path;
		if(pRec->Flags & PLMF_ANDROID) {
		}
		else {
			GetImpFileName(pPath, PPFILNAM_PALM_TODO, path);
			THROW(p_todo_tbl = PPOpenDbfTable(path));
			if(p_todo_tbl->getNumRecs() && p_todo_tbl->top()) {
				int fldn_id = 0;
				int fldn_completed = 0;
				int fldn_prior = 0;
				int fldn_duedate = 0;
				int fldn_compldate = 0;
				int fldn_descr = 0;
				int fldn_memo = 0;
				p_todo_tbl->getFieldNumber("ID", &fldn_id);
				p_todo_tbl->getFieldNumber("PRIOR", &fldn_prior);
				p_todo_tbl->getFieldNumber("COMPLETED", &fldn_completed);
				p_todo_tbl->getFieldNumber("DUEDATE", &fldn_duedate);
				p_todo_tbl->getFieldNumber("COMPLDATE", &fldn_compldate);
				p_todo_tbl->getFieldNumber("DESCRIPT", &fldn_descr);
				p_todo_tbl->getFieldNumber("MEMO",  &fldn_memo);
				do {
					PPID   id = 0;
					int    completed = 0;
					PalmToDoItem item;
					MEMSZERO(item);
					DbfRecord rec(p_todo_tbl);
					THROW(p_todo_tbl->getRec(&rec));
					rec.get(fldn_id, item.ID);
					rec.get(fldn_completed, item.Completed);
					rec.get(fldn_prior, item.Priority);
					rec.get(fldn_duedate, item.DueDate);
					rec.get(fldn_compldate, item.ComplDate);
					rec.get(fldn_descr, item.Descript, sizeof(item.Descript));
					rec.get(fldn_memo, item.Memo, sizeof(item.Memo));
					item.AgentID = pRec->AgentID;
					THROW_SL(pParam->P_ToDoQueue->push(&item));
				} while(p_todo_tbl->next());
			}
		}
	}
	else
		return -1;
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	delete p_todo_tbl;
	return ok;
}

int PPObjStyloPalm::ReadInputDebtMemo(PPStyloPalm * pRec, const char * pPath, PalmInputParam * pParam, long /*flags*/)
{
	int    ok = 1;
	DbfTable * p_tbl = 0;
	if(pParam && pParam->P_DebtMemoQueue) {
		SString path;
		GetImpFileName(pPath, PPFILNAM_PALM_DEBTMEMO, path);
		THROW(p_tbl = PPOpenDbfTable(path));
		if(p_tbl->getNumRecs() && p_tbl->top()) {
			int fldn_id = 0;
			int fldn_memo = 0;
			p_tbl->getFieldNumber("BILLID", &fldn_id);
			p_tbl->getFieldNumber("MEMO",   &fldn_memo);
			do {
				PalmDebtMemoItem item;
				MEMSZERO(item);
				DbfRecord rec(p_tbl);
				THROW(p_tbl->getRec(&rec));
				rec.get(fldn_id,   item.BillID);
				rec.get(fldn_memo, item.Memo, sizeof(item.Memo));
				THROW_SL(pParam->P_DebtMemoQueue->push(&item));
			} while(p_tbl->next());
		}
	}
	else
		return -1;
	CATCHZOK
	delete p_tbl;
	return ok;
}

class AndroidReader {
public:
	AndroidReader(const char * pPath, PPStyloPalm * pRec) : P_Doc(0)
	{
		if(!RVALUEPTR(PalmRec, pRec))
			MEMSZERO(PalmRec);
		if(pPath) {
			P_Doc = xmlReadFile(pPath, "UTF-8", XML_PARSE_NOENT);
		}
	}
	~AndroidReader()
	{
		xmlFreeDoc(P_Doc);
	}
	const xmlNode * FindFirstRec(const xmlNode * pChild, const char * pTag)
	{
		const xmlNode * p_result = 0;
		for(const xmlNode * p_rec = pChild; p_rec && !p_result; p_rec = p_rec->next) {
			p_result = SXml::IsName(p_rec, pTag) ? p_rec : FindFirstRec(p_rec->children, pTag); // @recursion
		}
		return p_result;
	}
	int ReadBills(PalmInputParam * pParam, long billIdBias, long * pOrdCount);
	int ReadGeoTracks(PalmInputParam * pParam);
private:
	PPStyloPalm PalmRec;
	xmlDoc * P_Doc;
};

int AndroidReader::ReadGeoTracks(PalmInputParam * pParam)
{
	const char * p_hdr_tag = "GeoTrackTable";
	const char * p_item_tag = "Item";
	int    ok = 1;
	xmlDoc * p_doc = P_Doc;
	SString wait_msg_buf;
	SString val;
	PPLoadText(PPTXT_IMPGEOTRACK, wait_msg_buf);
	if(pParam && pParam->P_GtList && p_doc) {
		long   _count = 0;
		IterCounter cntr;
		const xmlNode * p_first_rec = 0;
		const xmlNode * p_node = 0;
		xmlNode * p_root = xmlDocGetRootElement(p_doc);
		{
			p_node = FindFirstRec(p_root, p_hdr_tag);
			if(SXml::IsName(p_node, p_hdr_tag)) {
				p_first_rec = FindFirstRec(p_node, p_item_tag);
				if(SXml::IsName(p_first_rec, p_item_tag)) {
					p_node = p_first_rec;
					do {
						_count++;
						p_node = p_node->next;
					} while(SXml::IsName(p_node, p_item_tag));
				}
			}
		}
		cntr.Init(_count);
		for(p_node = p_first_rec; p_node; p_node = p_node->next) {
			const xmlNode * p_fld = p_node->children;
			if(p_fld) {
				PPGeoTrackItem gt_item;
				PPID line_order_id = 0;
				for(; p_fld; p_fld = p_fld->next) {
					if(p_fld->children && p_fld->children->content) {
						val.Set(p_fld->children->content);
						if(SXml::IsName(p_fld, "_id")) {
						}
						else if(SXml::IsName(p_fld, "Dt"))
							strtodate(val, DATF_DMY, &gt_item.Dtm.d);
						else if(SXml::IsName(p_fld, "Tm"))
							strtotime(val, TIMF_HMS, &gt_item.Dtm.t);
						else if(SXml::IsName(p_fld, "Latitude"))
							gt_item.Latitude = val.ToReal();
						else if(SXml::IsName(p_fld, "Longitude"))
							gt_item.Longitude = val.ToReal();
						else if(SXml::IsName(p_fld, "Altitude"))
							gt_item.Altitude = val.ToReal();
						else if(SXml::IsName(p_fld, "Speed"))
							gt_item.Speed = val.ToReal();
						else if(SXml::IsName(p_fld, "Flags"))
							gt_item.Flags = val.ToLong();
						else if(SXml::IsName(p_fld, "ExtObjType"))
							gt_item.ExtOid.Obj = val.ToLong();
						else if(SXml::IsName(p_fld, "ExtObjID"))
							gt_item.ExtOid.Id = val.ToLong();
					}
				}
				gt_item.Oid.Set(PPOBJ_STYLOPALM, PalmRec.ID);
				THROW_SL(pParam->P_GtList->insert(&gt_item));
			}
			PPWaitPercent(cntr.Increment(), wait_msg_buf);
		}
	}
	CATCHZOK
	return ok;
}

int AndroidReader::ReadBills(PalmInputParam * pParam, long billIdBias, long * pOrdCount)
{
	const char * p_hdr_tag = "OrderHeaderTable";
	const char * p_item_tag = "Item";
	const char * p_bill_tags = "_id;Dt;CreateDt;CreateTm;LocID;Code;ClientID;DlvrAddrID;QuotKindID;Amount;PctDis;Flags;Memo;CreateDtEnd;CreateTmEnd;Latitude;Longitude;LatitudeEnd;LongitudeEnd";
	const char * p_brow_tags = "_id;OrderID;GoodsID;Price;Quantity";
	int    ok = 1;
	uint   queue_pos = UINT_MAX;
	xmlDoc * p_doc = P_Doc;
	SString wait_msg_buf;
	SString val;
	PPLoadText(PPTXT_IMPBILL, wait_msg_buf);
	if(pParam && pParam->P_BillQueue && p_doc) {
		long   _count = 0;
		IterCounter cntr;
		const xmlNode * p_first_rec = 0;
		const xmlNode * p_node = 0;
		const xmlNode * p_root = xmlDocGetRootElement(p_doc);
		{
			p_node = FindFirstRec(p_root, p_hdr_tag);
			if(SXml::IsName(p_node, p_hdr_tag)) {
				p_first_rec = FindFirstRec(p_node, p_item_tag);
				if(SXml::IsName(p_first_rec, p_item_tag)) {
					p_node = p_first_rec;
					do {
						_count++;
						p_node = p_node->next;
					} while(SXml::IsName(p_node, p_item_tag));
				}
			}
		}
		cntr.Init(_count);
		for(p_node = p_first_rec; p_node; p_node = p_node->next) {
			const xmlNode * p_fld = p_node->children;
			if(p_fld) {
				PalmBillPacket * p_pack = new PalmBillPacket;
				PPID line_order_id = 0;
				for(; p_fld; p_fld = p_fld->next) {
					if(p_fld->children && p_fld->children->content) {
						int idx = 0;
						val = (const char *)p_fld->children->content;
						if(PPSearchSubStr(p_bill_tags, &(idx = 0), (const char *)p_fld->name, 1) > 0) {
							switch(idx) {
								case 0:   // _id
									p_pack->Hdr.OrgID = val.ToLong();
									p_pack->Hdr.ID = p_pack->Hdr.OrgID + billIdBias;
									break;
								case 1: strtodate(val, DATF_DMY, &p_pack->Hdr.Dt); break; // Dt
								case 2: strtodate(val, DATF_DMY, &p_pack->Hdr.CreateDtm.d); break; // CreateDt
								case 3: strtotime(val, TIMF_HMS, &p_pack->Hdr.CreateDtm.t); break; // CreateTm
								case 4: p_pack->Hdr.LocID = val.ToLong(); break; // LocID
								case 5: val.CopyTo(p_pack->Hdr.Code, sizeof(p_pack->Hdr.Code)); break; // Code
								case 6: p_pack->Hdr.ClientID   = val.ToLong(); break; // ClientID
								case 7: p_pack->Hdr.DlvrAddrID = val.ToLong(); break; // DlvrAddrID
								case 8: p_pack->Hdr.QuotKindID = val.ToLong(); break; // QuotKindID
								case 9: p_pack->Hdr.Amount = val.ToReal(); break; // Amount
								case 10: p_pack->Hdr.PctDis = val.ToReal(); break; // PctDis
								case 11: break; // Flags
								case 12:  // Memo
									val.Transf(CTRANSF_UTF8_TO_INNER);
									val.CopyTo(p_pack->Hdr.Memo, sizeof(p_pack->Hdr.Memo));
									break;
								case 13: strtodate(val, DATF_DMY, &p_pack->Hdr.CreateDtmEnd.d); break; // CreateDtEnd
								case 14: strtotime(val, TIMF_HMS, &p_pack->Hdr.CreateDtmEnd.t); break; // CreateTmEnd
								case 15: p_pack->Hdr.Latitude = val.ToReal(); break; // Latitude
								case 16: p_pack->Hdr.Longitude = val.ToReal(); break; // Longitude
								case 17: p_pack->Hdr.LatitudeEnd = val.ToReal(); break; // LatitudeEnd
								case 18: p_pack->Hdr.LongitudeEnd = val.ToReal(); break; // LongitudeEnd
							}
						}
					}
				}
				p_pack->Hdr.PalmID  = PalmRec.ID;
				p_pack->Hdr.Op      = 1;
				p_pack->Hdr.AgentID = PalmRec.AgentID;
				line_order_id = p_pack->Hdr.ID;
				if(line_order_id) {
					pParam->P_BillQueue->PushUnique(p_pack);
					const xmlNode * p_first_row_rec = 0;
					const xmlNode * p_row_node = 0;
					for(p_fld = p_node->children; p_fld; p_fld = p_fld->next) {
						if(SXml::IsName(p_fld, "OrderRowTable")) {
							for(p_fld = p_fld->children; !p_first_row_rec && p_fld; p_fld = p_fld->next) {
								if(SXml::IsName(p_fld, "Item"))
									p_first_row_rec = p_fld;
							}
							p_fld = 0;
							break;
						}
					}
					for(p_row_node = p_first_row_rec; p_row_node; p_row_node = p_row_node->next) {
						p_fld = p_row_node->children;
						if(p_fld) {
							PalmBillItem item;
							for(; p_fld; p_fld = p_fld->next) {
								if(p_fld->children && p_fld->children->content) {
									int idx = 0;
									val = (const char *)p_fld->children->content;
									if(PPSearchSubStr(p_brow_tags, &(idx = 0), (const char *)p_fld->name, 1) > 0) {
										switch(idx) {
											case 0: break; // _id
											case 1: break; // OrderID
											case 2: item.GoodsID = val.ToLong(); break; // GoodsID
											case 3: item.Price = val.ToReal(); break; // Price
											case 4: item.Qtty = val.ToReal(); break; // Quantity
										}
									}
								}
							}
							THROW(pParam->P_BillQueue->AddItem(line_order_id, &item, &queue_pos));
						}
					}
					queue_pos = UINT_MAX;
				}
			}
			PPWaitPercent(cntr.Increment(), wait_msg_buf);
		}
	}
	CATCHZOK
	return ok;
}

static int GetImportFileList(int isAndr, const char * pPath, WinInetFTP * pFtp, StrAssocArray * pList, PPLogger * pLogger)
{
	int    ok = 0;
	if(sstrlen(pPath) && pList) {
		SString fname;
		pList->Z();
		if(isAndr) {
			SString mask;
			PPGetFileName(PPFILNAM_PALM_OUTXML, fname);
			SFsPath sp(fname);
			mask.CatChar('*').Cat(sp.Nam).CatChar('*').Dot().Cat(sp.Ext);
			if(pFtp) {
				ok = pFtp->SafeGetFileList(pPath, pList, mask.cptr(), pLogger);
			}
			else {
				SString path;
				(path = pPath).SetLastSlash().Cat(mask);
				{
					SDirEntry de;
					SDirec sd(path);
					for(long id = 1; sd.Next(&de) > 0; id++) {
						de.GetNameA(fname);
						pList->Add(id, fname);
					}
					pList->SortByText();
				}
				ok = 1;
			}
		}
		else {
			pList->Add(1, PPGetFileName(PPFILNAM_PALM_BILL,    fname));
			pList->Add(2, PPGetFileName(PPFILNAM_PALM_BLINE,   fname));
			pList->Add(3, PPGetFileName(PPFILNAM_PALM_INVHDR,  fname));
			pList->Add(4, PPGetFileName(PPFILNAM_PALM_INVLINE, fname));
			pList->Add(5, PPGetFileName(PPFILNAM_PALM_TODO,    fname));
			ok = 1;
		}
	}
	return ok;
}

int PPObjStyloPalm::ReadInputBill(PPStyloPalm * pRec, const char * pPath, PalmInputParam * pParam, long /*flags*/, PPLogger * pLogger, long * pOrdCount)
{
	int    ok = 1;
	long   ord_count = 0L;
	IterCounter cntr;
	DbfTable * p_bill_tbl = 0, * p_line_tbl = 0;
	SString wait_msg_buf;
	if(pParam && pParam->P_BillQueue) {
		const  long bill_id_bias = pParam->P_BillQueue->GetBillIdBias();
		if(pRec->Flags & PLMF_ANDROID) {
			SString path, mask, fname;
			StrAssocArray file_list;
			SFsPath sp;
			if(GetImportFileList(1, pPath, 0, &file_list, 0) > 0) {
				for(uint i = 0; i < file_list.getCount(); i++) {
					AndroidReader reader((path = pPath).SetLastSlash().Cat(file_list.Get(i).Txt), pRec);
					if(!(pRec->Flags & PLMF_BLOCKED))
						reader.ReadBills(pParam, bill_id_bias, &ord_count);
					if(DS.CheckExtFlag(ECF_USEGEOTRACKING))
						reader.ReadGeoTracks(pParam);
				}
			}
		}
		else if(!(pRec->Flags & PLMF_BLOCKED)) {
			SString cln_path;
			SString hdr_path;
			SString ln_path;
			GetImpFileName(pPath, PPFILNAM_PALM_CLIENT, cln_path);
			GetImpFileName(pPath, PPFILNAM_PALM_BILL,   hdr_path);
			GetImpFileName(pPath, PPFILNAM_PALM_BLINE,  ln_path);
			THROW(p_bill_tbl = PPOpenDbfTable(hdr_path));
			THROW(p_line_tbl = PPOpenDbfTable(ln_path));
			if(p_bill_tbl->getNumRecs() && p_bill_tbl->top()) {
				int fldn_id = 0;
				int fldn_clientid = 0;
				int fldn_dlvraddrid = 0;
				int fldn_quotkindid = 0;
				int fldn_date = 0;
				int fldn_dscnt = 0;
				int fldn_code = 0;
				int fldn_sum = 0;
				int fldn_memo = 0;
				int fldn_loc  = 0;

				PPLoadText(PPTXT_IMPBILL, wait_msg_buf);
				cntr.Init(p_bill_tbl->getNumRecs());
				p_bill_tbl->getFieldNumber("ID", &fldn_id);
				if(!p_bill_tbl->getFieldNumber("AGENTID", &fldn_clientid))
					p_bill_tbl->getFieldNumber("CLIENTID", &fldn_clientid);
				p_bill_tbl->getFieldNumber("DLVRADRID", &fldn_dlvraddrid);
				p_bill_tbl->getFieldNumber("QKID", &fldn_quotkindid);
				p_bill_tbl->getFieldNumber("DATE", &fldn_date);
				p_bill_tbl->getFieldNumber("DISCOUNT", &fldn_dscnt);
				if(!p_bill_tbl->getFieldNumber("NUMBER", &fldn_code))
					p_bill_tbl->getFieldNumber("CODE", &fldn_code);
				p_bill_tbl->getFieldNumber("SUM", &fldn_sum);
				if(!p_bill_tbl->getFieldNumber("MEMO", &fldn_memo))
					p_bill_tbl->getFieldNumber("COMMENT", &fldn_memo);
				p_bill_tbl->getFieldNumber("LOCID", &fldn_loc);
				do {
					PalmBillPacket * p_pack = new PalmBillPacket;

					PPID   client_id = 0, dlvr_addr_id = 0;
					char   buf[256];
					DbfRecord rec(p_bill_tbl);
					THROW(p_bill_tbl->getRec(&rec));

					p_pack->Hdr.PalmID = pRec->ID;
					p_pack->Hdr.Op = 1; // Заказ
					p_pack->Hdr.AgentID = pRec->AgentID;
					rec.get(fldn_id, p_pack->Hdr.ID);
					p_pack->Hdr.ID += bill_id_bias;
					rec.get(fldn_clientid, client_id);
					rec.get(fldn_dlvraddrid, dlvr_addr_id);

					rec.get(fldn_date,  p_pack->Hdr.Dt);
					rec.get(fldn_dscnt, p_pack->Hdr.PctDis);
					rec.get(fldn_code,  buf, sizeof(buf));
					STRNSCPY(p_pack->Hdr.Code, strip(buf));
					rec.get(fldn_sum,  p_pack->Hdr.Amount);
					rec.get(fldn_memo, buf, sizeof(buf));
					strip(buf);
					SCharToOem(buf);
					STRNSCPY(p_pack->Hdr.Memo, buf);
					THROW(ResolveClientID(cln_path, client_id, &p_pack->Hdr.ClientID));
					p_pack->Hdr.DlvrAddrID = dlvr_addr_id;
					if(fldn_loc)
						rec.get(fldn_loc, p_pack->Hdr.LocID);
					THROW(pParam->P_BillQueue->Push(p_pack));
					ord_count++;
					PPWaitPercent(cntr.Increment(), wait_msg_buf);
				} while(p_bill_tbl->next());
				if(p_line_tbl->getNumRecs() && p_line_tbl->top()) {
					uint queue_pos = UINT_MAX;
					PPID prev_id = 0;

					int fldn_lineid = 0;
					int fldn_billid = 0;
					int fldn_goodsid = 0;
					int fldn_qtty = 0;
					int fldn_price = 0;

					PPLoadText(PPTXT_IMPBILLLINE, wait_msg_buf);
					cntr.Init(p_line_tbl->getNumRecs());
					p_line_tbl->getFieldNumber("ID",      &fldn_lineid);
					p_line_tbl->getFieldNumber("BILLID",  &fldn_billid);
					p_line_tbl->getFieldNumber("GOODSID", &fldn_goodsid);
					if(!p_line_tbl->getFieldNumber("QUANTITY", &fldn_qtty))
						p_line_tbl->getFieldNumber("QTTY", &fldn_qtty);
					p_line_tbl->getFieldNumber("PRICE", &fldn_price);
					do {
	   			    	long   line_order_id = 0;
						PalmBillItem item;
						DbfRecord line_rec(p_line_tbl);
						THROW(p_line_tbl->getRec(&line_rec));
						line_rec.get(fldn_billid, line_order_id);
						line_order_id += bill_id_bias;
						line_rec.get(fldn_goodsid, item.GoodsID);
						line_rec.get(fldn_qtty,    item.Qtty);
						line_rec.get(fldn_price,   item.Price);
						if(line_order_id != prev_id)
							queue_pos = UINT_MAX;
						THROW(pParam->P_BillQueue->AddItem(line_order_id, &item, &queue_pos));
						prev_id = line_order_id;
						PPWaitPercent(cntr.Increment(), wait_msg_buf);
					} while(p_line_tbl->next());
				}
			}
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pOrdCount, ord_count);
	delete p_line_tbl;
	delete p_bill_tbl;
	//::remove(cln_path);
	//::remove(hdr_path);
	//::remove(ln_path);
	return ok;
}

int PPObjStyloPalm::ReadInputInv(PPStyloPalm * pRec, const char * pPath, PalmInputParam * pParam, long /*flags*/, PPLogger * pLogger)
{
	int    ok = 1;
	DbfTable * p_bill_tbl = 0, * p_line_tbl = 0;
	if(pParam && pParam->P_BillQueue) {
		if(pRec->Flags & PLMF_ANDROID) {
		}
		else {
			SString cln_path;
			SString hdr_path;
			SString ln_path;
			GetImpFileName(pPath, PPFILNAM_PALM_CLIENT,   cln_path);
			GetImpFileName(pPath, PPFILNAM_PALM_INVHDR,   hdr_path);
			GetImpFileName(pPath, PPFILNAM_PALM_INVLINE,  ln_path);
			THROW(p_bill_tbl = PPOpenDbfTable(hdr_path));
			THROW(p_line_tbl = PPOpenDbfTable(ln_path));
			if(p_bill_tbl->getNumRecs() && p_bill_tbl->top()) {
				int fldn_id = 0;
				int fldn_clientid = 0;
				int fldn_dlvraddrid = 0;
				int fldn_date = 0;
				int fldn_code = 0;
				int fldn_memo = 0;

				p_bill_tbl->getFieldNumber("ID",        &fldn_id);
				p_bill_tbl->getFieldNumber("CLIENTID",  &fldn_clientid);
				p_bill_tbl->getFieldNumber("DLVRADRID", &fldn_dlvraddrid);
				p_bill_tbl->getFieldNumber("DATE",      &fldn_date);
				if(!p_bill_tbl->getFieldNumber("NUMBER", &fldn_code))
					p_bill_tbl->getFieldNumber("CODE", &fldn_code);
				if(!p_bill_tbl->getFieldNumber("MEMO", &fldn_memo))
					p_bill_tbl->getFieldNumber("COMMENT", &fldn_memo);
				do {
					PalmBillPacket * p_pack = new PalmBillPacket;

					PPID   client_id = 0, dlvr_addr_id = 0;
					char   buf[256];
					DbfRecord rec(p_bill_tbl);
					THROW(p_bill_tbl->getRec(&rec));

					p_pack->Hdr.PalmID = pRec->ID;
					p_pack->Hdr.Op = 2; // Инвентаризация //
					rec.get(fldn_id, p_pack->Hdr.ID);
					rec.get(fldn_clientid, client_id);
					rec.get(fldn_dlvraddrid, dlvr_addr_id);

					rec.get(fldn_date,  p_pack->Hdr.Dt);
					rec.get(fldn_code,  buf, sizeof(buf));
					STRNSCPY(p_pack->Hdr.Code, strip(buf));
					rec.get(fldn_memo, buf, sizeof(buf));
					strip(buf);
					SCharToOem(buf);
					STRNSCPY(p_pack->Hdr.Memo, buf);
					THROW(ResolveClientID(cln_path, client_id, &p_pack->Hdr.ClientID));
					p_pack->Hdr.DlvrAddrID = dlvr_addr_id;
					if(p_line_tbl->getNumRecs() && p_line_tbl->top()) {
						int fldn_billid = 0;
						int fldn_goodsid = 0;
						int fldn_qtty = 0;

						p_line_tbl->getFieldNumber("INVID",   &fldn_billid);
						p_line_tbl->getFieldNumber("GOODSID", &fldn_goodsid);
						if(!p_line_tbl->getFieldNumber("QUANTITY", &fldn_qtty))
							p_line_tbl->getFieldNumber("QTTY", &fldn_qtty);
						do {
	   			    		long   line_order_id = 0;
							DbfRecord line_rec(p_line_tbl);
							THROW(p_line_tbl->getRec(&line_rec));
							line_rec.get(fldn_billid, line_order_id);
							if(line_order_id == p_pack->Hdr.ID) {
								PalmBillItem item;
								line_rec.get(fldn_goodsid, item.GoodsID);
								line_rec.get(fldn_qtty,    item.Qtty);
								THROW(p_pack->AddItem(&item));
							}
						} while(p_line_tbl->next());
					}
					THROW(pParam->P_BillQueue->Push(p_pack));
				} while(p_bill_tbl->next());
			}
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	delete p_line_tbl;
	delete p_bill_tbl;
	//::remove(cln_path);
	//::remove(hdr_path);
	//::remove(ln_path);
	return ok;
}

int PPObjStyloPalm::ReadInput(PPID id, PalmInputParam * pParam, long flags, PPLogger * pLogger, long * pOrdCount)
{
	int    ok = -1;
	if(pParam) {
		SString path;
		PPStyloPalmPacket palm_pack;
		THROW(GetPacket(id, &palm_pack) > 0);
		if(palm_pack.MakeInputPath(path) && fileExists(path) && CheckSignalForInput(path)) {
			if(pParam->P_ToDoQueue && !(palm_pack.Rec.Flags & PLMF_BLOCKED)) {
				THROW(ReadInputToDo(&palm_pack.Rec, path, pParam, flags, pLogger));
			}
			if(pParam->P_BillQueue) {
				THROW(ReadInputBill(&palm_pack.Rec, path, pParam, flags, pLogger, pOrdCount));
				if(!(palm_pack.Rec.Flags & PLMF_BLOCKED)) {
					THROW(ReadInputInv(&palm_pack.Rec, path, pParam, flags, pLogger));
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

static SString & MakeBillCode(const PPStyloPalm & rDvcRec, int orderCodeFormatType, const char * pSrcCode, SString & rBuf)
{
	rBuf.Z();
	const size_t max_len = sizeof(((BillTbl::Rec *)0)->Code)-1;
	/*if(!oneof3(orderCodeFormatType, PPStyloPalmConfig::ordercodeIdHash, PPStyloPalmConfig::ordercodeSymbName, PPStyloPalmConfig::ordercodeNameSymb))
		orderCodeFormatType = PPStyloPalmConfig::ordercodeSymbName;*/
	switch(orderCodeFormatType) {
		case PPStyloPalmConfig::ordercodeIdHash:
			rBuf.CatChar('A' + (char)((rDvcRec.ID > 1000) ? ((rDvcRec.ID - 1000) % 26) : (rDvcRec.ID % 26)));
			break;
		case PPStyloPalmConfig::ordercodeNameSymb:
			if(rDvcRec.Name[0] && (sstrlen(rDvcRec.Name) + sstrlen(pSrcCode) + 1) <= max_len)
				(rBuf = rDvcRec.Name).ToUpper();
			else if(rDvcRec.Symb[0] && (sstrlen(rDvcRec.Symb) + sstrlen(pSrcCode) + 1) <= max_len)
				(rBuf = rDvcRec.Symb).ToUpper();
			else
				rBuf.CatChar('A' + (char)((rDvcRec.ID > 1000) ? ((rDvcRec.ID - 1000) % 26) : (rDvcRec.ID % 26)));
			break;
		case PPStyloPalmConfig::ordercodeSymbName:
		default:
			if(rDvcRec.Symb[0] && (sstrlen(rDvcRec.Symb) + sstrlen(pSrcCode) + 1) <= max_len)
				(rBuf = rDvcRec.Symb).ToUpper();
			else if((sstrlen(rDvcRec.Name) + sstrlen(pSrcCode) + 1) <= max_len)
				(rBuf = rDvcRec.Name).ToUpper();
			else
				rBuf.CatChar('A' + (char)((rDvcRec.ID > 1000) ? ((rDvcRec.ID - 1000) % 26) : (rDvcRec.ID % 26)));
			break;
	}
	rBuf.CatChar('-').Cat(pSrcCode);
	return rBuf;
}

int PPObjStyloPalm::ImportOrder(PalmBillPacket * pSrcPack, PPID opID, PPID locID, const PPStyloPalmConfig & rCfg, PPID * pResultID, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	int    r = 1;
	PPID   result_id = 0;
	PPBillPacket pack;
	const  PPID palm_id = pSrcPack->Hdr.PalmID;
	PPStyloPalm palm_rec;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	SString fmt_buf, msg_buf, temp_buf;
	PPID   analog_bill_id = 0;
	BillTbl::Rec analog_bill_rec;
	THROW(Search(palm_id, &palm_rec) > 0);
	THROW(pack.CreateBlank_WithoutCode(opID, 0L, pSrcPack->Hdr.LocID, use_ta));
	if(pSrcPack->Hdr.AgentID)
		pack.Ext.AgentID = pSrcPack->Hdr.AgentID;
	THROW(pack.SetFreight_DlvrAddrOnly(pSrcPack->Hdr.DlvrAddrID));
	pack.Rec.Dt = pSrcPack->Hdr.Dt;
	if(pSrcPack->Hdr.LocID)
		pack.Rec.LocID = pSrcPack->Hdr.LocID;
	else if(locID)
		pack.Rec.LocID = locID;
	MakeBillCode(palm_rec, rCfg.OrderCodeFormatType, pSrcPack->Hdr.Code, temp_buf);
	STRNSCPY(pack.Rec.Code, temp_buf);
	// @v11.1.12 STRNSCPY(pack.Rec.Memo, pSrcPack->Hdr.Memo);
	pack.SMemo = pSrcPack->Hdr.Memo; // @v11.1.12
	pack.Rec.EdiOp = PPEDIOP_SALESORDER;
	if(ar_obj.Fetch(pSrcPack->Hdr.ClientID, &ar_rec) > 0) {
		PPOprKind op_rec;
		if(GetOpData(opID, &op_rec) > 0 && op_rec.AccSheetID == ar_rec.AccSheetID)
			pack.Rec.Object = pSrcPack->Hdr.ClientID;
	}
	if(p_bobj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &analog_bill_id, &analog_bill_rec) < 0) {
		int    skip = 0;
		PalmBillItem item;
		ObjTagItem tag;
		if(pSrcPack->Hdr.CreateDtm != ZERODATETIME) { // Дата и время начала создания заказа
			pack.BTagL.PutItemStr(PPTAG_BILL_CREATEDTM, temp_buf.Z().Cat(pSrcPack->Hdr.CreateDtm));
		}
		if(pSrcPack->Hdr.CreateDtmEnd != ZERODATETIME) { // Дата и время окончания создания заказа
			pack.BTagL.PutItemStr(PPTAG_BILL_CREATEDTMEND, temp_buf.Z().Cat(pSrcPack->Hdr.CreateDtmEnd));
		}
		if(pSrcPack->Hdr.Latitude != 0.0 || pSrcPack->Hdr.Longitude != 0.0) { // Координаты начала создания заказа
			pack.BTagL.PutItemStr(PPTAG_BILL_GPSCOORD, temp_buf.Z().Cat(pSrcPack->Hdr.Latitude).CatChar(',').Cat(pSrcPack->Hdr.Longitude));
		}
		if(pSrcPack->Hdr.LatitudeEnd != 0.0 || pSrcPack->Hdr.LongitudeEnd != 0.0) { // Координаты окончания создания заказа
			pack.BTagL.PutItemStr(PPTAG_BILL_GPSCOORDEND, temp_buf.Z().Cat(pSrcPack->Hdr.LatitudeEnd).CatChar(',').Cat(pSrcPack->Hdr.LongitudeEnd));
		}
		pack.BTagL.PutItemStr(PPTAG_BILL_EDICHANNEL, "STYLOAGENT");
		if(rCfg.InhBillTagID && palm_rec.InhBillTagVal) {
			if(tag.SetInt(rCfg.InhBillTagID, palm_rec.InhBillTagVal))
				pack.BTagL.PutItem(rCfg.InhBillTagID, &tag);
		}
		for(uint i = 0; r && pSrcPack->EnumItems(&i, &item) > 0;) {
			if(GetOpType(opID) == PPOPT_GOODSORDER) {
				PPTransferItem ti;
				r = ti.Init(&pack.Rec);
				if(r > 0 && goods_obj.Fetch(item.GoodsID, &goods_rec) > 0) {
					ti.SetupGoods(item.GoodsID);
					ti.Quantity_ = fabs(item.Qtty);
					ti.Price = fabs(item.Price);
					//
					// Устанавливаем емкость упаковки лота заказа равной емкости упаковки последнего прихода до даты этого заказа
					//
					{
						LDATE  dt = pack.Rec.Dt;
						long   oprno = MAXLONG;
						ReceiptTbl::Rec temp_rec;
						while(p_bobj->trfr->Rcpt.EnumLastLots(labs(item.GoodsID), pack.Rec.LocID, &dt, &oprno, &temp_rec) > 0) {
							if(temp_rec.UnitPerPack > 0.0) {
								ti.UnitPerPack = temp_rec.UnitPerPack;
								break;
							}
						}
					}
					r = pack.InsertRow(&ti, 0);
				}
			}
			else if(GetOpType(opID) == PPOPT_GOODSEXPEND) {
				ILTI   ilti;
				LongArray rows;
				uint   fl = 0;
				ilti.Setup(item.GoodsID, -1, item.Qtty, 0, item.Price);
				r = p_bobj->ConvertILTI(&ilti, &pack, &rows, fl, 0);
				if(r > 0 && ilti.HasDeficit()) {
					int   r2 = ProcessUnsuffisientGoods(item.GoodsID, pugpNoBalance);
					if(r2 == PCUG_EXCLUDE)
						pack.RemoveRows(&rows);
					else if(r2 == PCUG_CANCEL || r < 0)
						skip = 1;
				}
			}
		}
		if(r > 0 && !skip && pack.GetTCount()) {
			if(pSrcPack->Hdr.PctDis) {
				pack.SetTotalDiscount(pSrcPack->Hdr.PctDis, 1, 0);
				pack.Rec.Flags |= BILLF_TOTALDISCOUNT;
			}
			pack.InitAmounts();
			THROW(p_bobj->FillTurnList(&pack));
			THROW(p_bobj->TurnPacket(&pack, use_ta));
			result_id = pack.Rec.ID;
			ok = 1;
			//accepted_ord_count++;
			if(pLogger) { // Выведем подробную информацию о том, что заказ принят
				msg_buf.Z().Printf(PPLoadTextS(PPTXT_ORDACCEPTED, fmt_buf), pack.Rec.Code, temp_buf.Z().Cat(pack.Rec.Dt).cptr(), palm_rec.Name);
				pLogger->Log(msg_buf);
			}
		}
	}
	else { // выведем подробную информацию о том, что такой документ уже существует
		// @v10.8.3 {
		// Аварийный блок для восстановления потерянных адресов доставки и агентов по заказу
		if(pSrcPack->Hdr.DlvrAddrID) {
			PPFreight ex_freight;
			if(p_bobj->P_Tbl->GetFreight(analog_bill_rec.ID, &ex_freight) <= 0) {
				PPFreight freight;
				freight.SetupDlvrAddr(pSrcPack->Hdr.DlvrAddrID);
				p_bobj->P_Tbl->SetFreight(analog_bill_rec.ID, &freight, use_ta);
			}
			else if(ex_freight.DlvrAddrID == 0) {
				ex_freight.SetupDlvrAddr(pSrcPack->Hdr.DlvrAddrID);
				p_bobj->P_Tbl->SetFreight(analog_bill_rec.ID, &ex_freight, use_ta);
			}
		}
		if(pSrcPack->Hdr.AgentID) {
			PPBillExt ex_ext;
			if(p_bobj->P_Tbl->GetExtraData(analog_bill_rec.ID, &ex_ext) <= 0) {
				PPBillExt ext;
				ext.AgentID = pSrcPack->Hdr.AgentID;
				p_bobj->P_Tbl->PutExtraData(analog_bill_rec.ID, &ext, use_ta);
			}
			else if(ex_ext.AgentID == 0) {
				ex_ext.AgentID = pSrcPack->Hdr.AgentID;
				p_bobj->P_Tbl->PutExtraData(analog_bill_rec.ID, &ex_ext, use_ta);
			}
		}
		// } @v10.8.3
		if(pLogger) {
			PPLoadString("date", temp_buf);
			PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, pack.Rec.Code, 1, msg_buf);
			pLogger->Log(msg_buf.Space().Cat(temp_buf).Cat(pack.Rec.Dt).Dot().Space().Cat(palm_rec.Name));
		}
		//ord_already_ex_count++;
		result_id = analog_bill_id;
		ok = -100;
	}
	CATCH
		ok = 0;
		if(pLogger) {
			PPGetLastErrorMessage(1, temp_buf);
			pLogger->Log((msg_buf = palm_rec.Name).CatDiv(':', 2).Cat(temp_buf));
		}
	ENDCATCH
	ASSIGN_PTR(pResultID, result_id);
	return ok;
}

int PPObjStyloPalm::ImportInventory(PalmBillPacket * pSrcPack, PPID opID, PPID locID, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	const  PPID palm_id = pSrcPack->Hdr.PalmID;
	PPObjStyloPalm palm_obj;
	PPStyloPalm palm_rec;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	PPBillPacket pack;
	SString temp_buf;
	THROW(palm_obj.Search(palm_id, &palm_rec) > 0);
	THROW_PP(/*sp_cfg.CliInvOpID*/opID, PPERR_INVOPNOTDEF);
	THROW(pack.CreateBlank_WithoutCode(/*sp_cfg.CliInvOpID*/opID, 0L, 0, use_ta));
	pack.Rec.Dt = pSrcPack->Hdr.Dt;
	MakeBillCode(palm_rec, PPStyloPalmConfig::ordercodeIdHash, pSrcPack->Hdr.Code, temp_buf);
	STRNSCPY(pack.Rec.Code, temp_buf);
	if(ar_obj.Fetch(pSrcPack->Hdr.ClientID, &ar_rec) > 0) {
		PPOprKind op_rec;
		if(GetOpData(opID, &op_rec) > 0 && op_rec.AccSheetID == ar_rec.AccSheetID)
			pack.Rec.Object = pSrcPack->Hdr.ClientID;
	}
	// @v11.1.12 STRNSCPY(pack.Rec.Memo, pSrcPack->Hdr.Memo);
	pack.SMemo = pSrcPack->Hdr.Memo; // @v11.1.12
	if(p_bobj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, 0, 0) > 0) {
		ok = -100;
	}
	else {
		PPObjBill::InvBlock inv_blk;
		PalmBillItem item;
		THROW(p_bobj->TurnInventory(&pack, use_ta));
		THROW(p_bobj->InitInventoryBlock(pack.Rec.ID, inv_blk));
		for(uint i = 0; pSrcPack->EnumItems(&i, &item) > 0;) {
			PPObjBill::InvItem inv_item;
			inv_item.Init(item.GoodsID, 0);
			inv_item.Qtty = item.Qtty;
			THROW(p_bobj->AcceptInventoryItem(inv_blk, &inv_item, 0));
		}
		ok = 1;
	}
	CATCH
		ok = 0;
		if(pLogger) {
			SString msg;
			PPGetLastErrorMessage(1, temp_buf);
			(msg = palm_rec.Name).CatDiv(':', 2).Cat(temp_buf);
			pLogger->Log(msg);
		}
	ENDCATCH
	return ok;
}

// @<<PPObjStyloPalm::ImportData
int PPObjStyloPalm::ImportBill(PalmBillQueue * pQueue, PPID opID, PPID locID, PPLogger * pLogger, long ordCount, int use_ta)
{
	int    ok = -1;
	long   accepted_ord_count = 0L, ord_already_ex_count = 0L;
	PalmBillPacket * p_src_pack = 0;
	InventoryCore & r_inv_tbl = BillObj->GetInvT();
	PPStyloPalmConfig sp_cfg;
	SString fmt_buf, msg_buf, temp_buf;
	PPObjStyloPalm::ReadConfig(&sp_cfg);
	while(!pQueue->IsEmpty()) {
		p_src_pack = pQueue->Pop();
		if(p_src_pack->Hdr.Op == 1) {
			PPID  result_bill_id = 0;
			int   ior = ImportOrder(p_src_pack, opID, locID, sp_cfg/*.OrderCodeFormatType*/, &result_bill_id, pLogger, 1);
			if(result_bill_id) {
                pQueue->SetIdAssoc(p_src_pack->Hdr.PalmID, p_src_pack->Hdr.OrgID, result_bill_id);
			}
			if(ior > 0)
				accepted_ord_count++;
			else if(ior == -100)
				ord_already_ex_count++;
		}
		else if(p_src_pack->Hdr.Op == 2) { // Инвентаризация //
			ImportInventory(p_src_pack, sp_cfg.CliInvOpID, 0, pLogger, 1);
		}
		ZDELETE(p_src_pack);
	}
	CALLPTRMEMB(pLogger, Log(msg_buf.Printf(PPLoadTextS(PPTXT_PALMACCEPTEDORDERS, temp_buf),
		accepted_ord_count + ord_already_ex_count, ordCount, ord_already_ex_count)));
	delete p_src_pack;
	pQueue->Destroy();
	return ok;
}

const char * PalmMemo = "Palm memo: ";

// @<<PPObjStyloPalm::ImportData
int PPObjStyloPalm::ImportDebtMemo(SQueue * pQueue, PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	/*
	int    ta = 0;
	if(pQueue) {
		THROW(PPStartTransaction(&ta, use_ta));
		while(pQueue->getNumItems()) {
			SString memo, prev_memos;
			DebtMemoItem * p_item = (DebtMemoItem *)pQueue->pop();
			if(p_item->BillID) {
				uint pos = 0;
				BillObj->FetchExtMemo(p_item->BillID, prev_memos);
				StringSet ss(MemosDelim);
				ss.setBuf(prev_memos, prev_memos.Len());
				(memo = PalmMemo).Cat(p_item->Memo);
				if(ss.search(PalmMemo, &pos, 0) <= 0 && ss.search(memo, &pos, 0) <= 0) {
					prev_memos.Cat(MemosDelim).Cat(memo);
					if(!PutMemos(PPOBJ_BILL, PPPRP_BILLMEMO, p_item->BillID, prev_memos)) {
						ok = 0;
						CALLPTRMEMB(pLogger, LogLastError());
					}
		  		}
			}
		}
		THROW(PPCommitWork(&ta));
	}
	CATCH
		PPRollbackWork(&ta);
		ok = 0;
	ENDCATCH
	*/
	return ok;
}

/*
static int DeleteImportFile(int fileID, const char * pFileName, const char * pExpPath, const char * pDeviceName)
{
  	int    ok = 1;
	char   fname[32], path[MAX_PATH];
	PPWaitStart();
	if(pFileName)
		STRNSCPY(fname, pFileName);
	else
		PPGetFileName(fileID, fname, sizeof(fname));
	setLastSlash(strcpy(path, pExpPath));
	PPSetAddedMsgString(path);
	if(::access(path, 0) != 0)
		CALLEXCEPT_PP_S(PPERR_NEXISTPATH, pExpPath);
   	strcat(setLastSlash(strcpy(path, pExpPath)), fname);
	SFile::Remove(path);
	CATCHZOK
	PPWaitStop();
	return ok;
}
*/

static int DeleteImportFiles(const PPStyloPalmPacket * pPack)
{
	int    ok = 1;
	if(pPack->P_Path) {
		const int is_andr = BIN(pPack->Rec.Flags & PLMF_ANDROID);
		SString input_path, file_name, path_;
		StrAssocArray file_list;
		PPWaitStart();
		pPack->MakeInputPath(input_path);
		if(::access(input_path, 0) == 0) {
			GetImportFileList(is_andr, input_path, 0, &file_list, 0);
			for(uint i = 0; i < file_list.getCount(); i++) {
				file_name = file_list.Get(i).Txt;
				if(file_name.CmpSuffix("in.xml", 1) != 0) { // @v11.1.1 @fix не следует удалять этот файл - он входящий для устройства
					(path_ = input_path).SetLastSlash().Cat(file_name.Strip());
					SFile::Remove(path_);
				}
			}
		}
		PPWaitStop();
	}
	else
		ok = -1;
	return ok;
}

int CallbackFTPTransfer(long count, long total, const char * pMsg, int)
{
	PPWaitPercent(count, total, pMsg);
	return 1;
}

static int DelFtpFile(const char * pFileName, const char * pFTPPath, WinInetFTP * pFtp, PPLogger * pLogger)
{
	int    ok = 1;
	if(pFtp) {
		SString ftp_path;
		ftp_path.CopyFrom(pFTPPath).SetLastSlash().Cat(pFileName);
	 	if(pFtp->Exists(ftp_path))
	 		ok = pFtp->SafeDelete(ftp_path, pLogger);
	}
	else
		ok = 0;
	return ok;
}

static int DelFtpFile(int fileID, const char * pFTPPath, WinInetFTP * pFtp, PPLogger * pLogger)
{
	int    ok = 1;
	if(pFtp) {
		SString fname;
		SString ftp_path;
		PPGetFileName(fileID, fname);
		ok = DelFtpFile(fname, pFTPPath, pFtp, pLogger);
	}
	else
		ok = 0;
	return ok;
}

static int DelLocalFile(const char * pPath, const char * pFileName)
{
	int    ok = 1;
	SString path;
	(path = pPath).SetLastSlash();
	THROW_PP_S(::access(path, 0) == 0, PPERR_NEXISTPATH, path);
	SFile::Remove(path.Cat(pFileName));
	CATCHZOK
	return ok;
}

static int DelLocalFile(const char * pPath, int fileID)
{
	SString fname;
	PPGetFileName(fileID, fname);
	return DelLocalFile(pPath, (const char *)fname);
}

static int CopyFileToFtp(int fileID, const char * pFileName, const char * pExpPath, const char * pFTPPath,
	const char * pDeviceName, WinInetFTP * pFtp, int toFtp, PPLogger * pLogger)
{
	int    ok = 1;
	SString file_name, path, ftp_path;
	PPWaitStart();
	if(pFileName)
		file_name = pFileName;
	else
		PPGetFileName(fileID, file_name);
	(path = pExpPath).SetLastSlash();
	THROW_PP_S(::access(path, 0) == 0, PPERR_NEXISTPATH, path);
	path.Cat(file_name);
	(ftp_path = pFTPPath).SetLastSlash().Cat(file_name);
	if(toFtp) {
		THROW(pFtp->SafePut(path, ftp_path, 0, CallbackFTPTransfer, pLogger));
	}
	else {
		THROW(pFtp->SafeGet(path, ftp_path, 0, CallbackFTPTransfer, pLogger));
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

static int CopyFilesToFTP(const PPStyloPalmPacket * pPack, WinInetFTP * pFtp, int toFtp, int delAfterCopy, PPLogger * pLogger = 0)
{
	int    ok = 1;
	int    put_to_log = 0;
	if(pFtp && sstrlen(pPack->P_Path) && sstrlen(pPack->P_FTPPath)) {
		int    r = 0;
		const  int  is_andr = BIN(pPack->Rec.Flags & PLMF_ANDROID);
		SString ftp_path;
		SString path;
		SString temp_buf;
		SString start_msg;
		SString fname;
		StrAssocArray file_list;
		(ftp_path = pPack->P_FTPPath).SetLastSlash();
		if(is_andr)
			pPack->MakePath(0, path);
		else {
			if(toFtp) {
				ftp_path.Cat("IN");
				pPack->MakeOutputPath(path);
			}
			else {
				ftp_path.Cat("OUT");
				pPack->MakeInputPath(path);
			}
		}
		put_to_log = BIN(pLogger);
		if(put_to_log) {
			start_msg.Printf(PPLoadTextS(toFtp ? PPTXT_FTPSPIISENDSTART : PPTXT_FTPSPIIRCVSTART, temp_buf), pPack->Rec.Name);
			pLogger->Log(start_msg);
			pLogger->Log("  ");
		}
		if(toFtp) {
			int    send_ok = 1;
			long   palm_flags = pPack->Rec.Flags;
			if(is_andr)
				r = send_ok = CopyFileToFtp(PPFILNAM_PALM_INXML, 0, path, ftp_path, pPack->Rec.Name, pFtp, 1, pLogger);
			else {
				PalmConfig server_pc, local_pc;
            	SString local_binfile = path;
				SString server_binfile = path;
				SString ftp_bin_path = ftp_path;
				server_binfile.SetLastSlash().Cat(P_PalmConfigFileName).CatChar('_');
				local_binfile.SetLastSlash().Cat(P_PalmConfigFileName);
				ftp_bin_path.SetLastSlash().Cat(P_PalmConfigFileName);
				if(pFtp->SafeGet(server_binfile, ftp_bin_path, 0, CallbackFTPTransfer, pLogger)) {
					server_pc.Read(server_binfile);
					SFile::Remove(server_binfile);
				}
				local_pc.Read(path);
				struct LSDTM {
					PPID   FileID;
					LDATETIME * P_LocalDTMS;
					LDATETIME ServerDTMS;
					long   CopyFile;
				};
				LSDTM files_dtms[] = {
					{PPFILNAM_PALM_GOODS,    &local_pc.TmGoods,   server_pc.TmGoods,  1},
					{PPFILNAM_PALM_BRAND,    &local_pc.TmGoods,   server_pc.TmGoods,  BIN(palm_flags & PLMF_EXPBRAND)},
					{PPFILNAM_PALM_LOC,      &local_pc.TmGoods,   server_pc.TmGoods,  BIN(palm_flags & PLMF_EXPLOC)},
					{PPFILNAM_PALM_GOODSGRP, &local_pc.TmGoods,   server_pc.TmGoods,  1},
					{PPFILNAM_PALM_QUOTKIND, &local_pc.TmGoods,   server_pc.TmGoods,  1},
					{PPFILNAM_PALM_CLIENT,   &local_pc.TmClient,  server_pc.TmClient, 1},
					{PPFILNAM_PALM_CLIADDR,  &local_pc.TmClient,  server_pc.TmClient, 1},
					{PPFILNAM_PALM_CLIDEBT,  &local_pc.TmCliDebt, server_pc.TmCliDebt, BIN(palm_flags & PLMF_EXPCLIDEBT)},
					{PPFILNAM_PALM_CLISELL,  &local_pc.TmCliSell, server_pc.TmCliSell, BIN(palm_flags & PLMF_EXPSELL)},
					{PPFILNAM_PALM_TODO,     &local_pc.TmToDo,    server_pc.TmToDo,    1}
				};
				uint num_files = (sizeof(files_dtms)/sizeof(LSDTM));
				for(uint i = 0; i < num_files; i++) {
					if(files_dtms[i].CopyFile) {
						if(cmp(*files_dtms[i].P_LocalDTMS, files_dtms[i].ServerDTMS) > 0) {
							r = CopyFileToFtp(files_dtms[i].FileID, 0, path, ftp_path, pPack->Rec.Name, pFtp, 1, pLogger);
							if(r < 0 && send_ok > 0)
								send_ok = r;
							else if(!r)
								send_ok = 0;
							if(r <= 0)
								files_dtms[i].P_LocalDTMS->Z();
						}
						else if(pLogger)
							pLogger->LogString(PPTXT_FTPSPIISRVDATAOBSOLETE, PPGetFileName(files_dtms[i].FileID, fname));
					}
					else if(pLogger)
						pLogger->LogString(PPTXT_PALM_BLOCKEDFILECOPY, PPGetFileName(files_dtms[i].FileID, fname));
				}
				SFile::Rename(local_binfile, server_binfile);
				local_pc.Write(local_binfile);
				r = CopyFileToFtp(0, P_PalmConfigFileName, path, ftp_path, pPack->Rec.Name, pFtp, 1, pLogger);
				SFile::Remove(local_binfile);
				SFile::Rename(server_binfile, local_binfile);
			}
			THROW(send_ok);
			THROW(r);
		}
		else {
			const char * p_ready_flag = "sp_ready";
			if(CopyFileToFtp(0, p_ready_flag, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger) > 0) {
				int del_spready = 1;
				if(is_andr) {
					if(GetImportFileList(1, ftp_path, pFtp, &file_list, pLogger) > 0) {
						DeleteImportFiles(pPack);
						for(uint i = 0; i < file_list.getCount(); i++) {
							//const char * p_fname = file_list.Get(i).Txt;
							(temp_buf = file_list.Get(i).Txt).Strip();
							if(temp_buf.CmpSuffix("stylo.log", 1) != 0 && temp_buf.CmpSuffix("in.xml", 1) != 0) { // @v11.1.1-@v11.1.7 Эти файлы нам не нужны
								r = CopyFileToFtp(0, temp_buf, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger);
								if(r <= 0) {
									const SString add_str = DS.GetTLA().AddedMsgString; // preserving before the call of DelLocalFile()
									DelLocalFile(path, p_ready_flag);
									DelLocalFile(path, temp_buf);
									PPSetAddedMsgString(add_str);
									del_spready = 0;
									THROW(r);
								}
							}
						}
					}
				}
				else {
					if((r = CopyFileToFtp(PPFILNAM_PALM_BILL,  0, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger)) > 0)
						r = CopyFileToFtp(PPFILNAM_PALM_BLINE, 0, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger);
					if(r <= 0) {
						SString add_str = DS.GetTLA().AddedMsgString;
						del_spready = 0;
						DelLocalFile(path, PPFILNAM_PALM_BILL);
						DelLocalFile(path, PPFILNAM_PALM_BLINE);
						DelLocalFile(path, p_ready_flag);
						PPSetAddedMsgString(add_str);
						THROW(r);
					}
					if((r = CopyFileToFtp(PPFILNAM_PALM_INVHDR,  0, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger)) > 0)
						r = CopyFileToFtp(PPFILNAM_PALM_INVLINE, 0, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger);
					if(r <= 0) {
						SString add_str = DS.GetTLA().AddedMsgString;
						del_spready = 0;
						DelLocalFile(path, PPFILNAM_PALM_INVHDR);
						DelLocalFile(path, PPFILNAM_PALM_INVLINE);
						DelLocalFile(path, p_ready_flag);
						PPSetAddedMsgString(add_str);
						THROW(r);
					}
					if((r = CopyFileToFtp(PPFILNAM_PALM_TODO, 0, path, ftp_path, pPack->Rec.Name, pFtp, 0, pLogger)) <= 0) {
						DelLocalFile(path, p_ready_flag);
						THROW(r);
					}
				}
				if(del_spready) {
					SString sp_ready_path = ftp_path;
                    sp_ready_path.SetLastSlash().Cat(p_ready_flag);
					pFtp->SafeDelete(sp_ready_path, pLogger);
				}
				else
					delAfterCopy = 0;
			}
			else {
				pLogger->LogString(PPTXT_FTPSPIIDATANOTREADY, 0);
				ok = -1;
			}
		}
		if(delAfterCopy) {
			SString spready_ftp_path;
			(spready_ftp_path = pPack->P_FTPPath).SetLastSlash();
			if(!is_andr)
				spready_ftp_path.Cat("OUT").SetLastSlash();
			ftp_path = spready_ftp_path;
			spready_ftp_path.Cat("sp_ready");
			if(!pFtp->Exists(spready_ftp_path)) {
				if(GetImportFileList(is_andr, ftp_path, pFtp, &file_list, pLogger) > 0)
					for(uint i = 0; i < file_list.getCount(); i++)
						DelFtpFile(file_list.Get(i).Txt, ftp_path, pFtp, pLogger);
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	if(put_to_log)
		pLogger->Log("  ");
	return ok;
}

int PPObjStyloPalm::Helper_GetChildList(PPID id, PPIDArray & rPalmList, PPIDArray * pStack)
{
	int    ok = 1;
	PPIDArray inner_stack;
	PPStyloPalm rec;
	THROW(Search(id, &rec) > 0);
	if(rec.Flags & PLMF_GENERIC) {
		SETIFZ(pStack, &inner_stack);
		if(pStack->addUnique(id) < 0) {
			PPSetError(PPERR_STYLOPALMCYCLE, rec.Name);
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			CALLEXCEPT();
		}
		else {
			PPStyloPalm child_rec;
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&child_rec) > 0;)
				if(child_rec.GroupID == id)
					Helper_GetChildList(child_rec.ID, rPalmList, pStack); // @recursion
		}
	}
	else
		rPalmList.addUnique(id);
	CATCHZOK
	return ok;
}

int PPObjStyloPalm::IsPacketEq(const PPStyloPalmPacket & rS1, const PPStyloPalmPacket & rS2, long flags)
{
#define CMP_MEMB(m)  if(rS1.Rec.m != rS2.Rec.m) return 0;
#define CMP_MEMBS(m) if(!sstreq(rS1.Rec.m, rS2.Rec.m)) return 0;
	CMP_MEMB(ID);
	CMP_MEMBS(Name);
	CMP_MEMBS(Symb);
	CMP_MEMB(RegisterTime);
	CMP_MEMB(LastExchgTime);
	CMP_MEMB(DeviceVer);
	CMP_MEMB(Gtm);
	CMP_MEMB(GoodsGrpID);
	CMP_MEMB(OrderOpID);
	CMP_MEMB(FTPAcctID);
	CMP_MEMB(Flags);
	CMP_MEMB(GroupID);
	CMP_MEMB(AgentID);
	CMP_MEMB(MaxUnsentOrders);
	CMP_MEMB(TransfDaysAgo);
	CMP_MEMB(QuotKindOptions); // @v11.7.1
	CMP_MEMB(InhBillTagVal);
#undef CMP_MEMBS
#undef CMP_MEMB
	if(!rS1.LocList.IsEq(rS2.LocList))
		return 0;
	else if(!rS1.QkList__.IsEq(rS2.QkList__))
		return 0;
	else {
		SString temp_buf1, temp_buf2;
		temp_buf1 = rS1.P_FTPPath;
		temp_buf2 = rS2.P_FTPPath;
		if(temp_buf1 != temp_buf2)
			return 0;
		else {
			temp_buf1 = rS1.P_Path;
			temp_buf2 = rS2.P_Path;
			if(temp_buf1 != temp_buf2)
				return 0;
		}
	}
	return 1;
}

int PPObjStyloPalm::GetChildList(PPID id, PPIDArray & rPalmList)
{
	return Helper_GetChildList(id, rPalmList, 0);
}

int PPObjStyloPalm::GetListByPerson(PPID personID, PPIDArray & rPalmList)
{
	int    ok = -1;
	rPalmList.Z();
	if(personID) {
		PPID   agent_accsheet_id = GetAgentAccSheet();
		if(agent_accsheet_id) {
			PPObjArticle ar_obj;
			PPID   ar_id = 0;
			if(ar_obj.P_Tbl->PersonToArticle(personID, agent_accsheet_id, &ar_id) > 0 && ar_id) {
				PPStyloPalm2 rec;
				for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
					if(rec.AgentID == ar_id) {
						rPalmList.add(rec.ID);
						ok = 1;
					}
				}
			}
		}
	}
	rPalmList.sortAndUndup();
	return ok;
}

int PPObjStyloPalm::CopyFromFTP(PPID id, int delAfterCopy, PPLogger * pLog)
{
	int    ok = 1;
	PPIDArray palm_list;
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	PPStyloPalmPacket palm_pack;
	WinInetFTP ftp;
	THROW(GetPacket(id, &palm_pack) > 0);
	THROW(obj_acct.Get(palm_pack.Rec.FTPAcctID, &acct));
	THROW(GetChildList(id, palm_list));
	THROW(ftp.Init());
	THROW(ftp.Connect(&acct));
	for(uint i = 0; i < palm_list.getCount(); i++) {
		PPStyloPalmPacket child_pack;
		if(GetPacket(palm_list.at(i), &child_pack) > 0) {
			int r = CopyFilesToFTP(&child_pack, &ftp, 0, delAfterCopy, pLog);
			ok = (r == 0) ? 0 : ok;
			if(!ok)
				PPSetError(PPERR_RCVFROMFTP);
			THROW(ftp.ReInit());
		}
	}
	CATCHZOK
	ftp.UnInit();
	return ok;
}

// @<<PPObjStyloPalm::ImpExp
int PPObjStyloPalm::ImportData(PPID id, PPID opID, PPID locID, PPLogger * pLogger)
{
	int    ok = -1, r = 1;
	uint   i = 0;
	long   ord_count = 0L;
	SString path;
	PPStyloPalmPacket palm_pack;
	PPObjGoods goods_obj;
	PPObjArticle ar_obj;
	PPIDArray palm_id_list;

	PalmBillQueue bill_queue;
	SQueue todo_queue(sizeof(PalmToDoItem), 1024);
	SQueue bill_debt_memo_queue(sizeof(PalmDebtMemoItem), 1024);
	TSVector <PPGeoTrackItem> gt_list;
	PalmInputParam input_param;
	input_param.P_BillQueue = &bill_queue;
	input_param.P_ToDoQueue = &todo_queue;
	input_param.P_DebtMemoQueue = &bill_debt_memo_queue;
	input_param.P_GtList = &gt_list;
	THROW(GetPacket(id, &palm_pack) > 0);
	THROW(GetChildList(id, palm_id_list));
	for(i = 0; i < palm_id_list.getCount(); i++) {
		const  PPID child_id = palm_id_list.at(i);
		if(ReadInput(child_id, &input_param, 0, pLogger, &ord_count) == 0)
			r = 0;
	}
	r = (ImportBill(&bill_queue, opID, locID, pLogger, ord_count, 1) == 0) ? 0 : r;
	r = (ImportDebtMemo(&bill_debt_memo_queue, pLogger, 1) == 0) ? 0 : r;
	r = (ImportToDo(&todo_queue, pLogger, 1) == 0) ? 0 : r;
	{
		const uint gtc = gt_list.getCount();
		if(gtc) {
			GeoTrackCore gt_core;
			for(i = 0; i < gtc; i++) {
				PPGeoTrackItem & r_item = gt_list.at(i);
				if(r_item.ExtOid.Obj == PPOBJ_BILL) {
					const  PPID inner_id = bill_queue.GetInnerIdByOuter(r_item.Oid.Id, r_item.ExtOid.Id);
                    r_item.ExtOid.Id = inner_id;
				}
			}
			r = (gt_core.PutChunk(gt_list, 1) == 0) ? 0 : r;
		}
	}
	THROW(r && ok);
	//
	// Очищаем семафор готовности данных только в том случае, если
	// прием объектов завершился успешно.
	//
	for(i = 0; i < palm_id_list.getCount(); i++) {
		PPStyloPalmPacket palm_pack;
		if(GetPacket(palm_id_list.at(i), &palm_pack) > 0 && sstrlen(palm_pack.P_Path)) {
			if(palm_pack.MakeInputPath(path))
				ClearInputSemaphore(path);
		}
	}
	CATCHZOK
	bill_queue.Destroy();
	return ok;
}

static DbfTable * Palm_CreateDbfTable(uint fileNameId, uint dbfsId)
{
	SString path;
	return CreateDbfTable(dbfsId, PPGetFilePathS(PPPATH_OUT, fileNameId, path), 1);
}

class AndroidXmlWriter {
public:
	struct Header {
		Header()
		{
			THISZERO();
		}
		int32 FormatVersion;
		char  ProductName[32];
		int32 ProductVersion;
		int32 Id;
		char  Name[32];
		S_GUID UUID_;
		int32 AgentId;
		LDATETIME CreateDtm;
		int32 MaxNotSentOrd;
		int32 TransfDaysAgo;
		PPGeoTrackingMode Gtm;
		int32 StyloFlags;
	};
	AndroidXmlWriter(const char * pPath, Header * pHdr, const char * pRoot = 0);
	~AndroidXmlWriter();
	int    PutTableInfo(const char * pName, int32 recsCount, LDATETIME crtDtm);
	int    StartElement(const char * pName, const char * pAttribName = 0, const char * pAttribValue = 0);
	int    EndElement();
	int    PutElement(const char * pName, const char * pValue);
	int    PutElement(const char * pName, long v);
	int    PutElement(const char * pName, int v) { return PutElement(pName, static_cast<long>(v)); }
	int    PutElement(const char * pName, double v);
	int    PutElement(const char * pName, LDATE v);
	int    AddAttrib(const char * pName, const char * pAttribValue);
	int    AddAttrib(const char * pName, bool attribValue);
	int    AddAttrib(const char * pName, long attribValue);
	int    AddAttrib(const char * pName, int attribValue) { return AddAttrib(pName, static_cast<long>(attribValue)); }
	int    AddAttrib(const char * pName, double attribValue);
	int    AddAttrib(const char * pName, LDATETIME val);
	int    AddAttrib(const char * pName, LTIME val);
	int    AddAttrib(const char * pName, LDATE val);
	int    AddAttrib(const char * pName, const S_GUID & rUuid);

	int    GetBuffer(SBuffer & rBuf);
private:
	int    Helper_CloseWriter();

	xmlTextWriter * P_Writer;
	xmlBuffer * P_Buffer;
};

#define PALM_FIRST_QUOTFLD 13
#define PALM_MAX_QUOT      20

AndroidXmlWriter::AndroidXmlWriter(const char * pPath, Header * pHdr, const char * pRoot /*=0*/) : P_Buffer(0)
{
	if(!isempty(pPath)) {
		SFile::Remove(pPath);
		if(sstreqi_ascii(pPath, ":buffer:")) {
            P_Buffer = xmlBufferCreate();
            if(P_Buffer)
				P_Writer = xmlNewTextWriterMemory(P_Buffer, 0);
		}
		else
			P_Writer = xmlNewTextWriterFilename(pPath, 0);
		if(P_Writer) {
			SString temp_buf;
			SString uhtt_acc;
			PPAlbatrossConfig cfg;
			DS.FetchAlbatrosConfig(&cfg);
			xmlTextWriterSetIndent(P_Writer, 1);
			xmlTextWriterSetIndentTab(P_Writer);
			xmlTextWriterStartDocument(P_Writer, 0, "utf8", 0);
			StartElement(isempty(pRoot) ? "StyloPalm" : pRoot);
			if(pHdr) {
				AddAttrib("FormatVersion",  pHdr->FormatVersion);
				AddAttrib("ProductName",    pHdr->ProductName);
				AddAttrib("ProductVersion", pHdr->ProductVersion);
				AddAttrib("Id",             pHdr->Id);
				AddAttrib("Name",           pHdr->Name);
				AddAttrib("UUID",           pHdr->UUID_);
				AddAttrib("AgentId",        pHdr->AgentId);
				AddAttrib("CreateDt",       pHdr->CreateDtm.d);
				AddAttrib("CreateTm",       pHdr->CreateDtm.t);
				AddAttrib("MaxNotSentOrd",  pHdr->MaxNotSentOrd);
				cfg.GetExtStrData(ALBATROSEXSTR_UHTTURN_unused, temp_buf);
				AddAttrib("UhttUrn",        /*cfg.UhttUrn*/temp_buf);
				cfg.GetExtStrData(ALBATROSEXSTR_UHTTURLPFX, temp_buf);
				AddAttrib("UhttUrlPrefix",  /*cfg.UhttUrlPrefix*/temp_buf);
				cfg.GetExtStrData(ALBATROSEXSTR_UHTTACC, uhtt_acc);
				AddAttrib("UhttAccount",    /*cfg.UhttAccount*/uhtt_acc);
				AddAttrib("GeoTrackingMode",  pHdr->Gtm.Mode);
				AddAttrib("GeoTrackingCycle", pHdr->Gtm.Cycle);
				AddAttrib("StyloFlags",     pHdr->StyloFlags);
				AddAttrib("TransfDaysAgo",  pHdr->TransfDaysAgo);
			}
			// @v10.2.0 Если аккаунта нет, то и пароль слать не надо (иначе может передаться мусор)
			if(uhtt_acc.NotEmpty()) {
				cfg.GetPassword(ALBATROSEXSTR_UHTTPASSW, temp_buf);
				AddAttrib("UhttPassword", temp_buf.cptr());
				temp_buf.Obfuscate();
			}
		}
	}
}

int AndroidXmlWriter::Helper_CloseWriter()
{
	int    ok = 0;
	if(P_Writer) {
		EndElement();
		xmlTextWriterEndDocument(P_Writer);
		xmlFreeTextWriter(P_Writer);
		P_Writer = 0;
		ok = 1;
	}
	return ok;
}

AndroidXmlWriter::~AndroidXmlWriter()
{
	Helper_CloseWriter();
	xmlBufferFree(P_Buffer);
}

int AndroidXmlWriter::GetBuffer(SBuffer & rBuf)
{
	int    ok = -1;
	if(P_Buffer) {
		Helper_CloseWriter();
        int   sz = xmlBufferLength(P_Buffer);
        const void * p_content = xmlBufferContent(P_Buffer);
        if(sz > 0 && p_content)
            rBuf.Write(p_content, (size_t)sz);
	}
	return ok;
}

int AndroidXmlWriter::PutTableInfo(const char * pName, int32 recsCount, LDATETIME crtDtm)
{
	int    ok = -1;
	if(P_Writer && sstrlen(pName)) {
		SString temp_buf;
		temp_buf.Cat(recsCount);
		StartElement(pName, "RecsCount", temp_buf);
		AddAttrib("CreateDt", crtDtm.d);
		AddAttrib("CreateDm", crtDtm.t);
		ok = 1;
	}
	return ok;
}

int AndroidXmlWriter::StartElement(const char * pName, const char * pAttribName /*=0*/, const char * pAttribValue /*=0*/)
{
	int    ok = 0;
	if(P_Writer && !isempty(pName)) {
		SString name_buf;
		(name_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterStartElement(P_Writer, name_buf.ucptr());
		AddAttrib(pAttribName, pAttribValue);
		ok = 1;
	}
	return ok;
}

int AndroidXmlWriter::EndElement()
{
	int    ok = 0;
	if(P_Writer) {
		xmlTextWriterEndElement(P_Writer);
		ok = 1;
	}
	return ok;
}

int AndroidXmlWriter::PutElement(const char * pName, const char * pValue)
{
	int    ok = 0;
	if(P_Writer && !isempty(pName) && !isempty(pValue)) {
		SString name_buf, val_buf;
		(name_buf = pName).Strip().Transf(CTRANSF_INNER_TO_UTF8);
		(val_buf = pValue).Strip().ReplaceChar('\n', ' ').ReplaceChar('\r', ' ');
		XMLReplaceSpecSymb(val_buf, "&<>\'");
		val_buf.Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterWriteElement(P_Writer, name_buf.ucptr(), val_buf.ucptr());
		ok = 1;
	}
	return ok;
}

int AndroidXmlWriter::PutElement(const char * pName, long v)
{
	SString val;
	return PutElement(pName, val.Cat(v));
}

int AndroidXmlWriter::PutElement(const char * pName, double v)
{
	SString val;
	return PutElement(pName, val.Cat(v));
}

int AndroidXmlWriter::PutElement(const char * pName, LDATE v)
{
	SString val;
	return PutElement(pName, val.Cat(v, DATF_DMY));
}

int AndroidXmlWriter::AddAttrib(const char * pAttribName, const char * pAttribValue)
{
	int    ok = 0;
	if(P_Writer && !isempty(pAttribName) && !isempty(pAttribValue)) {
		SString attrib_name, attrib_value;
		(attrib_name = pAttribName).Transf(CTRANSF_INNER_TO_UTF8);
		(attrib_value = pAttribValue).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ').ReplaceChar(30, ' ');
		XMLReplaceSpecSymb(attrib_value, "&<>\'");
		attrib_value.Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterWriteAttribute(P_Writer, attrib_name.ucptr(), attrib_value.ucptr());
		ok = 1;
	}
	return ok;
}

int AndroidXmlWriter::AddAttrib(const char * pName, const S_GUID & rUuid)
{
	int    ok = 0;
	if(!rUuid.IsZero()) {
		SString temp_buf;
		rUuid.ToStr(S_GUID::fmtIDL, temp_buf);
		ok = AddAttrib(pName, temp_buf);
	}
	return ok;
}

int AndroidXmlWriter::AddAttrib(const char * pAttribName, bool attribValue)
{
	SString attr_val;
	return AddAttrib(pAttribName, attr_val.Cat(STextConst::GetBool(attribValue)));
}

int AndroidXmlWriter::AddAttrib(const char * pAttribName, long attribValue)
{
	SString attr_val;
	return AddAttrib(pAttribName, attr_val.Cat(attribValue));
}

int AndroidXmlWriter::AddAttrib(const char * pAttribName, double attribValue)
{
	SString attr_val;
	return AddAttrib(pAttribName, attr_val.Cat(attribValue, MKSFMTD(0, 2, NMBF_NOTRAILZ)));
}

int AndroidXmlWriter::AddAttrib(const char * pName, LDATETIME val)
{
	SString attr_val;
	return AddAttrib(pName, attr_val.Cat(val));
}

int AndroidXmlWriter::AddAttrib(const char * pName, LTIME val)
{
	SString attr_val;
	return AddAttrib(pName, attr_val.Cat(val));
}

int AndroidXmlWriter::AddAttrib(const char * pName, LDATE val)
{
	SString attr_val;
	return AddAttrib(pName, attr_val.Cat(val));
}

int PutGoods(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		Sdr_PalmGoods rec;
		SString buf;
		pRec->get(1,  rec.ID);
		pRec->get(2,  buf);
		buf.CopyTo(rec.Name, sizeof(rec.Name));
		pRec->get(3,  buf);
		buf.CopyTo(rec.Barcode, sizeof(rec.Barcode));
		pRec->get(4,  rec.Pack);
		pRec->get(5,  rec.Price);
		pRec->get(6,  rec.Rest);
		pRec->get(7,  rec.ParentID);
		pRec->get(8,  rec.Expiry);
		pRec->get(9,  rec.BrandID);
		pRec->get(10, rec.BrandOwnerID);
		pRec->get(11, rec.MinOrd);
		pRec->get(12, rec.MultMinOrd);

		pWriter->PutElement("_id",          rec.ID);
		pWriter->PutElement("Name",         rec.Name);
		pWriter->PutElement("Barcode",      rec.Barcode);
		pWriter->PutElement("Pack",         rec.Pack);
		pWriter->PutElement("Price",        rec.Price);
		pWriter->PutElement("Rest",         rec.Rest);
		pWriter->PutElement("ParentID",     rec.ParentID);
		pWriter->PutElement("Expiry",       rec.Expiry);
		pWriter->PutElement("BrandID",      rec.BrandID);
		pWriter->PutElement("BrandOwnerID", rec.BrandOwnerID);
		pWriter->PutElement("MinOrd",       rec.MinOrd);
		pWriter->PutElement("MultMinOrd",   rec.MultMinOrd);
		ok = 1;
	}
	return ok;
}

int PutQuots(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		Sdr_PalmQuot rec;
		pRec->get(1,  rec.GoodsID);
		pRec->get(2,  rec.QuotKindID);
		pRec->get(3,  rec.ClientID);
		pRec->get(4,  rec.Price);
		pWriter->PutElement("GoodsID",      rec.GoodsID);
		pWriter->PutElement("QuotKindID",   rec.QuotKindID);
		pWriter->PutElement("ClientID",     rec.ClientID);
		pWriter->PutElement("Price",        rec.Price);
		ok = 1;
	}
	return ok;
}

int PutGoodsGrp(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		SString buf;
		Sdr_PalmGoodsGrp rec;
		pRec->get(1,  rec.ID);
		pRec->get(2,  rec.ParentID);
		pRec->get(3,  buf);
		buf.CopyTo(rec.Name, sizeof(rec.Name));
		pWriter->PutElement("_id",      rec.ID);
		pWriter->PutElement("ParentID", rec.ParentID);
		pWriter->PutElement("Name",     rec.Name);
		ok = 1;
	}
	return ok;
}

int PutClients(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long   flags = 0;
		SString buf;
		Sdr_PalmClient rec;
		pRec->get(1,  rec.ID);
		pRec->get(2,  buf);
		buf.CopyTo(rec.Name, sizeof(rec.Name));
		pRec->get(3,  buf);
		buf.CopyTo(rec.Code, sizeof(rec.Code));
		pRec->get(4,  buf);
		buf.CopyTo(rec.Inn, sizeof(rec.Inn));
		pRec->get(5,  rec.QuotKindID);
		pRec->get(6,  rec.Debt);
		pRec->get(7,  flags);
		if(sstrlen(strip(rec.Name)) == 0)
			ltoa(rec.ID, rec.Name, 10);
		pWriter->PutElement("_id",        rec.ID);
		pWriter->PutElement("Name",       rec.Name);
		pWriter->PutElement("Code",       rec.Code);
		pWriter->PutElement("Inn",        rec.Inn);
		pWriter->PutElement("QuotKindID", rec.QuotKindID);
		pWriter->PutElement("Debt",       rec.Debt);
		pWriter->PutElement("Stop",       (long)BIN(flags & CLIENTF_BLOCKED));
		pWriter->PutElement("Flags",      (flags & (CLIENTF_DONTUSEMINSHIPMQTTY|CLIENTF_BLOCKED))); // @v8.4.4
		ok = 1;
	}
	return ok;
}

int PutDlvrAddr(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long flags = 0;
		SString buf;
		Sdr_PalmDlvrAddr rec;
		pRec->get(1,  rec.ID);
		pRec->get(2,  rec.ClientID);
		pRec->get(3,  buf);
		buf.CopyTo(rec.AddrString, sizeof(rec.AddrString));
		pWriter->PutElement("ID",         rec.ID);
		pWriter->PutElement("ClientID",    rec.ClientID);
		pWriter->PutElement("Name",     rec.AddrString);
		ok = 1;
	}
	return ok;
}

int PutQuotKind(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long flags = 0;
		SString buf;
		Sdr_PalmQuotKind rec;
		pRec->get(1,  rec.ID);
		pRec->get(3,  buf);
		buf.CopyTo(rec.Name, sizeof(rec.Name));
		pWriter->PutElement("_id",  rec.ID);
		pWriter->PutElement("Name", rec.Name);
		ok = 1;
	}
	return ok;
}

int PutBrand(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long flags = 0;
		SString buf;
		Sdr_PalmBrand rec;
		pRec->get(1,  rec.BrandID);
		pRec->get(2,  rec.BrandOwnerID);
		pRec->get(3,  buf);
		buf.CopyTo(rec.BrandName, sizeof(rec.BrandName));
		pRec->get(4,  buf);
		buf.CopyTo(rec.BrandOwnerName, sizeof(rec.BrandOwnerName));

		pWriter->PutElement("BrandID",        rec.BrandID);
		pWriter->PutElement("BrandName",      rec.BrandName);
		pWriter->PutElement("BrandOwnerID",   rec.BrandOwnerID);
		pWriter->PutElement("BrandOwnerName", rec.BrandOwnerName);
		ok = 1;
	}
	return ok;
}

int PutDebt(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long flags = 0;
		SString buf;
		Sdr_PalmCliDebt rec;
		pRec->get(1, rec.ClientID);
		pRec->get(2, rec.BillID);
		pRec->get(3, buf);
		STRNSCPY(rec.BillCode, buf);
		pRec->get(4, rec.BillDt);
		pRec->get(5, rec.BillAmt);
		pRec->get(6, rec.BillDebt);
		pRec->get(7, rec.AgentID);
		pWriter->PutElement("ClientID",  rec.ClientID);
		pWriter->PutElement("AgentID",   rec.AgentID);
		pWriter->PutElement("BillID",    rec.BillID);
		pWriter->PutElement("BillCode",  rec.BillCode);
		pWriter->PutElement("BillDt",    rec.BillDt);
		pWriter->PutElement("BillAmt",   rec.BillAmt);
		pWriter->PutElement("BillDebt",  rec.BillDebt);
		ok = 1;
	}
	return ok;
}

int PutWarehouse(DbfRecord * pRec, AndroidXmlWriter * pWriter)
{
	int    ok = -1;
	if(pRec && pWriter) {
		long flags = 0;
		SString buf;
		Sdr_PalmWarehouse rec;
		pRec->get(1,  rec.ID);
		pRec->get(2,  buf);
		buf.CopyTo(rec.Name, sizeof(rec.Name));
		pWriter->PutElement("_id",        rec.ID);
		pWriter->PutElement("Name",       rec.Name);
		pWriter->PutElement("Latitude",   rec.Latitude);
		pWriter->PutElement("Longitude",  rec.Longitude);
		pWriter->PutElement("AddrString", rec.AddrString);
		ok = 1;
	}
	return ok;
}

struct TableNames {
	uint   FileId;
	const char * P_TblName;
	int    (*Func)(DbfRecord * pRec, AndroidXmlWriter * pWriter);
};

static const TableNames TblNames[] = {
	{PPFILNAM_PALM_GOODS,    "GoodsTable",      &PutGoods},
	{PPFILNAM_PALM_QUOTS,    "QuotsTable",      &PutQuots},
	{PPFILNAM_PALM_GOODSGRP, "GoodsGroupTable", &PutGoodsGrp},
	{PPFILNAM_PALM_CLIENT,   "ClientsTable",    &PutClients},
	{PPFILNAM_PALM_QUOTKIND, "QuotKindTable",   &PutQuotKind},
	{PPFILNAM_PALM_CLIADDR,  "DlvrAddrTable",   &PutDlvrAddr},
	{PPFILNAM_PALM_CLIDEBT,  "ClientDebtTable", &PutDebt},
	{PPFILNAM_PALM_CLISELL,  "", 0},
	{PPFILNAM_PALM_BRAND,    "BrandTable",      &PutBrand},
	{PPFILNAM_PALM_LOC,      "LocationTable",   &PutWarehouse},
	{PPFILNAM_PALM_TODO,     "", 0},
};

class AndroidDevs {
public:
	AndroidDevs()
	{
	}
	int Add(const PPStyloPalmPacket * pPack)
	{
		int    ok = -1;
		if(pPack && pPack->Rec.Flags & PLMF_ANDROID) {
			AndroidXmlWriter::Header h;
			SString path, fname;
			pPack->MakePath(0, path);
			PPGetFileName(PPFILNAM_PALM_INXML, fname);
			path.SetLastSlash().Cat(fname);
			h.Id = pPack->Rec.ID;
			STRNSCPY(h.Name, pPack->Rec.Name);
			/*
			{
				SString name_buf;
				(name_buf = pPack->Rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				name_buf.CopyTo(h.Name, sizeof(h.Name));
				memset(h.Name + sstrlen(h.Name), ' ', sizeof(h.Name) - sstrlen(h.Name));
			}
			*/
			h.AgentId = pPack->Rec.AgentID;
			h.MaxNotSentOrd = pPack->Rec.MaxUnsentOrders;
			h.TransfDaysAgo = pPack->Rec.TransfDaysAgo;
			h.FormatVersion  = 1;
			STRNSCPY(h.ProductName, "StyloAndroid");
			h.ProductVersion = 1;
			if(DS.CheckExtFlag(ECF_USEGEOTRACKING)) {
				h.Gtm = pPack->Rec.Gtm;
			}
			else {
				h.Gtm.Z();
			}
			h.StyloFlags = (pPack->Rec.Flags & PLMF_TRANSMITMASK);
			getcurdatetime(&h.CreateDtm);
			{
				AndroidXmlWriter * p_writer = new AndroidXmlWriter(path, &h);
				PalmList.add(pPack->Rec.ID);
				Writers.insert(p_writer);
				ok = 1;
			}
		}
		return ok;
	}
	int    MoveOutFile(PPID palmId, uint fileId, int forceEmpty)
	{
		int    ok = -1;
		uint   pos = 0;
		DbfTable * p_tbl = 0;
		if(PalmList.lsearch(palmId, &pos)) {
			SString path;
			AndroidXmlWriter * p_writer = Writers.at(pos);
			PPGetFilePath(PPPATH_OUT, fileId, path);
			if(fileExists(path)) {
				THROW(p_tbl = PPOpenDbfTable(path));
				int32 recs_count = (int32)p_tbl->getNumRecs();
				if((recs_count || forceEmpty) && PutTableElement(fileId, p_writer, recs_count) > 0) {
					int  (*func)(DbfRecord * pRec, AndroidXmlWriter * pWriter) = 0;
					for(uint i = 0; func == 0 && i < SIZEOFARRAY(TblNames); i++) {
						if(fileId == TblNames[i].FileId)
							func = TblNames[i].Func;
					}
					if(recs_count && p_tbl->top()) do {
						DbfRecord rec(p_tbl);
						p_tbl->getRec(&rec);
						p_writer->StartElement("Item");
						func(&rec, p_writer);
						p_writer->EndElement();
					} while(ok < 0 && p_tbl->next());
					p_writer->EndElement();
					ok = 1;
				}
				else
					ok = -2;
			}
			else
				ok = -2;
		}
		CATCHZOK
		ZDELETE(p_tbl);
		return ok;
	}
	int    PutTableElement(uint fileId, AndroidXmlWriter * pWriter, int32 rowsCount)
	{
		int    ok = -1;
		if(pWriter) {
			SString tbl_name;
			for(uint i = 0; tbl_name.IsEmpty() && i < SIZEOFARRAY(TblNames); i++) {
				if(fileId == TblNames[i].FileId)
					tbl_name = TblNames[i].P_TblName;
			}
			if(tbl_name.Len()) {
				pWriter->PutTableInfo(tbl_name, rowsCount, getcurdatetime_());
				ok = 1;
			}
		}
		return ok;
	}
private:
	PPIDArray PalmList;
	TSCollection <AndroidXmlWriter> Writers;
};

struct PalmCfgItem {
	PPID   ID;
	long   PalmFlags;
	SString Path;
	PalmConfig C;
};

static int MoveOutFilePlain(int fileID, const char * pFileName, const PalmCfgItem * pCfgItem, AndroidDevs * pDevs)
{
	int    ok = -1;
	SString fname, path, out_path;
	if(pFileName)
		fname = pFileName;
	else
		PPGetFileName(fileID, fname);
	PPGetFilePath(PPPATH_OUT, fname, path);
	(out_path = pCfgItem->Path).SetLastSlash();
	PPSetAddedMsgString(out_path);
	if(::access(out_path, 0) != 0) {
		ok = PPSetError(PPERR_NEXISTPATH, pCfgItem->Path);
	}
	else {
		(out_path = pCfgItem->Path).SetLastSlash().Cat(fname);
		ok = SCopyFile(path, out_path, 0, FILE_SHARE_READ, 0) ? 1 : PPSetError(PPERR_SLIB, path);
	}
	return ok;
}

//static int MoveOutFile(int fileID, const char * pFileName, const char * pExpPath, PPID palmId, AndroidDevs * pDevs)
static int MoveOutFile(int fileID, const PalmCfgItem * pCfgItem, AndroidDevs * pDevs)
{
	const  int force_empty = BIN(pCfgItem->PalmFlags & PLMF_BLOCKED);
	int    ok = pDevs ? pDevs->MoveOutFile(pCfgItem->ID, fileID, force_empty) : -1;
	if(ok < 0 && ok != -2) {
		ok = MoveOutFilePlain(fileID, 0, pCfgItem, pDevs);
	}
	return ok;
}

/*static*/int PPObjStyloPalm::XmlCmpDtm(LDATE dt, LTIME tm, const char * pXmlPath)
{
	int    r = -1;
	const  char * p_tag = "StyloPalm";
	LDATETIME dtm = ZERODATETIME;
	xmlDoc * p_doc = 0;
	xmlTextReader * p_reader  = 0;
	if(pXmlPath)
		p_reader = xmlReaderForFile(pXmlPath, NULL, XML_PARSE_NOENT);
	if(p_reader) {
		int r = 0;
		xmlTextReaderPreservePattern(p_reader, reinterpret_cast<const xmlChar *>(p_tag), 0);
		r = xmlTextReaderRead(p_reader);
		/*
		while(r == 1)
			r = xmlTextReaderRead(p_reader);
		*/
		if(r == 1) {
			p_doc = xmlTextReaderCurrentDoc(p_reader);
			if(p_doc) {
				xmlNode * p_root = xmlDocGetRootElement(p_doc);
				if(p_root && /* p_items->type == XML_ELEMENT_NODE && */ p_root->properties) {
					xmlAttr * p_attr = p_root->properties;
					for(; p_attr; p_attr = p_attr->next) {
						if(p_attr->children && p_attr->children->content) {
							if(sstreqi_ascii("CreateDt", (const char *)p_attr->name))
								strtodate((const char *)p_attr->children->content, DATF_DMY, &dtm.d);
							else if(sstreqi_ascii("CreateTm", (const char *)p_attr->name))
								strtotime((const char *)p_attr->children->content, TIMF_HMS, &dtm.t);
						}
					}
				}
			}
		}
	}
	xmlFreeTextReader(p_reader);
	p_reader = 0;
	xmlFreeDoc(p_doc);
	p_doc = 0;
	if(dt < dtm.d)
		r = -1;
	else if(dt > dtm.d)
		r = 1;
	else if(tm < dtm.t)
		r = -1;
	else if(tm > dtm.t)
		r = 1;
	else
		r = 0;
	return r;
}

int PPObjStyloPalm::ImportToDo(SQueue * pQueue, PPLogger * pLogger, int use_ta)
{
	int    ok = 1;
	if(pQueue) {
		PPObjPrjTask todo_obj;
		PPTransaction tra(use_ta);
		THROW(tra);
		while(pQueue->getNumItems()) {
			PalmToDoItem * p_item = (PalmToDoItem *)pQueue->pop();
			PrjTaskTbl::Rec todo_rec;
			if(p_item->Completed && todo_obj.Search(p_item->ID, &todo_rec) > 0) {
				if(todo_rec.Status != TODOSTTS_COMPLETED) {
					todo_rec.Status = TODOSTTS_COMPLETED;
					if(!todo_obj.P_Tbl->Update(p_item->ID, &todo_rec, 0)) {
						ok = 0;
						CALLPTRMEMB(pLogger, LogLastError());
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PPObjStyloPalm::ExportBlock::ExportBlock() : P_DebtView(0), P_GrView(0), P_BrandList(0), P_WhList(0), P_GgList(0)
{
	P_GObj = new PPObjGoods;
	P_BrObj = new PPObjBrand;
	P_PsnObj = new PPObjPerson;
	P_LocObj = new PPObjLocation;
	P_ArObj = new PPObjArticle;
}

PPObjStyloPalm::ExportBlock::~ExportBlock()
{
	delete P_GObj;
	delete P_BrObj;
	delete P_PsnObj;
	delete P_LocObj;
	delete P_ArObj;
	delete P_DebtView;
	delete P_GrView;
	delete P_BrandList;
	delete P_WhList;
	delete P_GgList;
}

int PPObjStyloPalm::CreateWhList(ExportBlock & rBlk)
{
	int    ok = -1;
	if(!rBlk.P_WhList) {
		SString temp_buf;
		PPIDArray loc_list;
		PPObjLocation loc_obj;
		THROW_MEM(rBlk.P_WhList = new TSVector <ExportBlock::WhEntry>);
		loc_obj.GetWarehouseList(&loc_list, 0);
		for(uint i = 0; i < loc_list.getCount(); i++) {
			ExportBlock::WhEntry entry;
			MEMSZERO(entry);
			entry.WhID = loc_list.get(i);
			GetObjectName(PPOBJ_LOCATION, entry.WhID, temp_buf);
			temp_buf.CopyTo(entry.WhName, sizeof(entry.WhName));
			THROW_SL(rBlk.P_WhList->insert(&entry));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjStyloPalm::CreateBrandList(ExportBlock & rBlk)
{
	int    ok = -1;
    if(!rBlk.P_BrandList) {
    	THROW_MEM(rBlk.P_BrandList = new TSVector <ExportBlock::BrandEntry>);
		{
			SString temp_buf;
			Goods2Tbl::Key2 k2;
			GoodsCore * p_gtbl = rBlk.P_GObj->P_Tbl;
			BExtQuery q(p_gtbl, 2);
			MEMSZERO(k2);
			k2.Kind = PPGDSK_BRAND;
			q.select(p_gtbl->ID, 0L).where(p_gtbl->Kind == PPGDSK_BRAND);
			for(q.initIteration(false, &k2); q.nextIteration() > 0;) {
				PPBrand brand_rec;
				if(rBlk.P_BrObj->Fetch(p_gtbl->data.ID, &brand_rec) > 0) {
					ExportBlock::BrandEntry entry;
					MEMSZERO(entry);
                    entry.BrandID = brand_rec.ID;
                    entry.OwnerID = brand_rec.OwnerID;
                    STRNSCPY(entry.BrandName, brand_rec.Name);
					if(brand_rec.OwnerID && GetPersonName(brand_rec.OwnerID, temp_buf) > 0)
						temp_buf.CopyTo(entry.OwnerName, sizeof(entry.OwnerName));
					THROW_SL(rBlk.P_BrandList->insert(&entry));
				}
			}
		}
    	ok = 1;
    }
    CATCHZOK
    return ok;
}

int PPObjStyloPalm::CreateGoodsGrpList(ExportBlock & rBlk)
{
	int    ok = -1;
    if(!rBlk.P_GgList) {
		SString temp_buf;
		PPID   id = 0;
		Goods2Tbl::Rec goods_rec;
    	THROW_MEM(rBlk.P_GgList = new TSVector <ExportBlock::GoodsGrpEntry>);
		for(GoodsGroupIterator ggiter(0); ggiter.Next(&id, temp_buf) > 0;) {
			if(rBlk.P_GObj->Fetch(id, &goods_rec) > 0) {
				ExportBlock::GoodsGrpEntry entry;
				entry.ID = goods_rec.ID;
				entry.ParentID = goods_rec.ParentID;
				STRNSCPY(entry.Name, goods_rec.Name);
				THROW_SL(rBlk.P_GgList->insert(&entry));
			}
		}
    	ok = 1;
    }
    CATCHZOK
    return ok;
}

/* @v10.8.6 int PPObjStyloPalm::CreateQkList(ExportBlock & rBlk)
{
	int    ok = 1;
	rBlk.QkList.Z();
	PPObjQuotKind qk_obj;
	QuotKindFilt qk_filt;
	qk_filt.Flags = (QuotKindFilt::fExclNotForBill|QuotKindFilt::fSortByRankName);
	qk_filt.MaxItems = PALM_MAX_QUOT;
	THROW(qk_obj.MakeList(&qk_filt, &rBlk.QkList));
	CATCHZOK
	return ok;
}*/

struct StyloPalmGoodsEntry {
	StyloPalmGoodsEntry(PPID goodsID) : GoodsID(goodsID), Rest(0.0), Cost(0.0), Price(0.0), UnitPerPack(0.0)
	{
	}
	PPID   GoodsID;
	double Rest;
	double Cost;
	double Price;
	double UnitPerPack;
};

int PPObjStyloPalm::ExportGoods(const PPStyloPalmPacket * pPack, ExportBlock & rBlk)
{
	int    ok = -1;
	uint   i;
	SString wait_msg, temp_buf;
	DbfTable  * p_goods_tbl = 0, * p_grp_tbl = 0, * p_qk_tbl = 0;
	PPIDArray  grp_list;
	PPImpExp * p_ie_brand = 0;
	PPImpExp * p_ie_loc   = 0;
	PPImpExp * p_ie_quots = 0;
	PPObjQuotKind qk_obj;
	Goods2Tbl::Rec goods_rec;
	TSVector <StyloPalmGoodsEntry> goods_list;
	PPIDArray temp_loc_list;
	Transfer * p_trfr = BillObj->trfr;
	SETIFZ(rBlk.P_GrView, new PPViewGoodsRest);
	THROW_MEM(rBlk.P_GrView);
	THROW(p_goods_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_GOODS, DBFS_PALM_GOODS));
	THROW(p_grp_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_GOODSGRP, DBFS_PALM_GOODSGRP));
	THROW(p_qk_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_QUOTKIND, DBFS_PALM_QUOTKIND));
	{
		PPImpExpParam param_quots;
		THROW(InitImpExpDbfParam(PPREC_PALMQUOT, &param_quots, PPGetFilePathS(PPPATH_OUT, PPFILNAM_PALM_QUOTS, temp_buf), 1));
		THROW_MEM(p_ie_quots = new PPImpExp(&param_quots, 0));
		THROW(p_ie_quots->OpenFileForWriting(0, 1));
	}
	if(!(pPack->Rec.Flags & PLMF_BLOCKED)) {
		const  PPID single_loc_id = pPack->LocList.GetSingle();
		{
			PPSetAddedMsgString(p_qk_tbl->getName());
			for(i = 0; i < pPack->QkList__.GetCount(); i++) {
				const  PPID qk_id = pPack->QkList__.Get(i);
				PPQuotKindPacket qk_pack;
				if(qk_obj.Fetch(qk_id, &qk_pack) > 0) {
					DbfRecord drec_qk(p_qk_tbl);
					drec_qk.put(1, qk_pack.Rec.ID);
					drec_qk.put(2, static_cast<long>(i+1));
					drec_qk.put(3, qk_pack.Rec.Name);
					THROW_PP(p_qk_tbl->appendRec(&drec_qk), PPERR_DBFWRFAULT);
				}
			}
		}
		{
			PPIDArray gt_quasi_unlim_list;
			{
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				for(SEnum en = gt_obj.Enum(0); en.Next(&gt_rec) > 0;) {
					if(gt_rec.Flags & (GTF_QUASIUNLIM|GTF_UNLIMITED)) 
						gt_quasi_unlim_list.add(gt_rec.ID);
				}
			}
			GoodsRestFilt gr_filt;
			gr_filt.LocList = pPack->LocList;
			gr_filt.GoodsGrpID = pPack->Rec.GoodsGrpID;
			gr_filt.CalcMethod = GoodsRestParam::pcmLastLot;
			if(gt_quasi_unlim_list.getCount())
				gr_filt.Flags |= GoodsRestFilt::fNullRest;
			gr_filt.WaitMsgID  = PPTXT_WAIT_GOODSREST;
			THROW(rBlk.P_GrView->Init_(&gr_filt));
			PPLoadText(PPTXT_WAIT_PALMEXPGOODS, wait_msg);
			GoodsRestViewItem gr_item;
			for(rBlk.P_GrView->InitIteration(); rBlk.P_GrView->NextIteration(&gr_item) > 0;) {
				PPWaitPercent(rBlk.P_GrView->GetCounter(), wait_msg);
				if(rBlk.P_GObj->Fetch(gr_item.GoodsID, &goods_rec) > 0) {
					if(gr_item.Rest > 0.0 || gt_quasi_unlim_list.lsearch(goods_rec.GoodsTypeID)) {
						StyloPalmGoodsEntry goods_entry(gr_item.GoodsID);
						goods_entry.Rest = gr_item.Rest;
						goods_entry.Cost = gr_item.Cost;
						goods_entry.Price = gr_item.Price;
						goods_entry.UnitPerPack = gr_item.UnitPerPack;
						goods_list.insert(&goods_entry);
					}
				}
			}
		}
		{
			for(uint glidx = 0; glidx < goods_list.getCount(); glidx++) {
				const StyloPalmGoodsEntry & r_goods_entry = goods_list.at(glidx);
				if(rBlk.P_GObj->Fetch(r_goods_entry.GoodsID, &goods_rec) > 0) {
					THROW(grp_list.add(goods_rec.ParentID));
					DbfRecord drec_goods(p_goods_tbl);
					drec_goods.put(1, r_goods_entry.GoodsID);
					drec_goods.put(2, goods_rec.Name);
					rBlk.P_GObj->FetchSingleBarcode(r_goods_entry.GoodsID, temp_buf);
					drec_goods.put(3, temp_buf);
					drec_goods.put(4, r_goods_entry.UnitPerPack);
					drec_goods.put(5, r_goods_entry.Price);
					drec_goods.put(6, r_goods_entry.Rest);
					drec_goods.put(7, goods_rec.ParentID);
					drec_goods.put(9, goods_rec.BrandID);
					if(goods_rec.BrandID && (pPack->Rec.Flags & PLMF_EXPBRAND)) {
						PPBrand brand_rec;
						if(rBlk.P_BrObj->Fetch(goods_rec.BrandID, &brand_rec) > 0)
							drec_goods.put(10, brand_rec.OwnerID);
					}
					{
						//
						// Минимальный заказ
						//
						GoodsStockExt stock;
						if(rBlk.P_GObj->GetStockExt(goods_rec.ID, &stock, 1) > 0) {
							drec_goods.put(11, stock.MinShippmQtty);
							drec_goods.put(12, BIN(stock.GseFlags & GoodsStockExt::fMultMinShipm));
						}
					}
					{
						const LDATE now_date = getcurdate_();
						for(i = 0; i < PALM_MAX_QUOT && i < pPack->QkList__.GetCount(); i++) {
							const  PPID qk_id = pPack->QkList__.Get(i);
							double quot = 0.0;
							QuotIdent qi(QIDATE(now_date), single_loc_id, qk_id);
							if(rBlk.P_GObj->GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
								drec_goods.put(PALM_FIRST_QUOTFLD+i, quot);
								{
									Sdr_PalmQuot quot_rec;
									quot_rec.GoodsID    = r_goods_entry.GoodsID;
									quot_rec.QuotKindID = qk_id;
									quot_rec.ClientID   = 0;
									quot_rec.Price      = quot;
									THROW(p_ie_quots->AppendRecord(&quot_rec, sizeof(quot_rec)));
								}
							}
							// @v10.5.2 {
							else if(single_loc_id == 0 && pPack->LocList.GetCount()) {
								GoodsRestParam grparam;
								grparam.LocList = pPack->LocList.Get();
								grparam.GoodsID = r_goods_entry.GoodsID;
								grparam.DiffParam = GoodsRestParam::_diffLoc;
								p_trfr->GetCurRest(grparam);
								temp_loc_list.clear();
								if(grparam.Total.Rest > 0.0) {
									for(uint grvidx = 0; grvidx < grparam.getCount(); grvidx++) {
										GoodsRestVal & r_grv = grparam.at(grvidx);
										if(r_grv.Rest > 0.0)
											temp_loc_list.add(r_grv.LocID);
									}
								}
								{
									const  PPID target_loc_id = temp_loc_list.getCount() ? temp_loc_list.get(0) : pPack->LocList.Get(0);
									QuotIdent qi(QIDATE(now_date), target_loc_id, qk_id);
									if(rBlk.P_GObj->GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
										drec_goods.put(PALM_FIRST_QUOTFLD+i, quot);
										{
											Sdr_PalmQuot quot_rec;
											quot_rec.GoodsID    = r_goods_entry.GoodsID;
											quot_rec.QuotKindID = qk_id;
											quot_rec.ClientID   = 0;
											quot_rec.Price      = quot;
											THROW(p_ie_quots->AppendRecord(&quot_rec, sizeof(quot_rec)));
										}
									}
								}
							}
							// } @v10.5.2 
						}
					}
					PPSetAddedMsgString(p_goods_tbl->getName());
					THROW_PP(p_goods_tbl->appendRec(&drec_goods), PPERR_DBFWRFAULT);
				}
			}
			grp_list.sortAndUndup();
		}
		{
			struct NameEntry { // @flat
				char   Name[64];
				PPID   ID;
				PPID   ParentID;
			};
			SVector name_list(sizeof(NameEntry));
			if(rBlk.P_GgList) {
				for(uint i = 0; i < rBlk.P_GgList->getCount(); i++) {
					const ExportBlock::GoodsGrpEntry & r_eb_entry = rBlk.P_GgList->at(i);
					if(grp_list.lsearch(r_eb_entry.ID)) {
						NameEntry entry;
						MEMSZERO(entry);
						entry.ID = r_eb_entry.ID;
						entry.ParentID = r_eb_entry.ParentID;
						STRNSCPY(entry.Name, r_eb_entry.Name);
						name_list.insert(&entry);
					}
				}
			}
			name_list.sort(PTR_CMPFUNC(PcharNoCase));
			PPSetAddedMsgString(p_grp_tbl->getName());
			NameEntry * p_entry;
			for(uint i = 0; name_list.enumItems(&i, (void **)&p_entry);) {
				DbfRecord drec_grp(p_grp_tbl);
				drec_grp.put(1, p_entry->ID);
				drec_grp.put(2, p_entry->ParentID);
				drec_grp.put(3, p_entry->Name);
				THROW_PP(p_grp_tbl->appendRec(&drec_grp), PPERR_DBFWRFAULT);
			}
		}
	}
	if(pPack->Rec.Flags & PLMF_EXPBRAND) {
		PPImpExpParam param_brand;
		THROW(CreateBrandList(rBlk));
		THROW(InitImpExpDbfParam(PPREC_SPIIBRAND, &param_brand, PPGetFilePathS(PPPATH_OUT, PPFILNAM_PALM_BRAND, temp_buf), 1));
		THROW_MEM(p_ie_brand = new PPImpExp(&param_brand, 0));
		THROW(p_ie_brand->OpenFileForWriting(0, 1));
		if(!(pPack->Rec.Flags & PLMF_BLOCKED)) {
			if(rBlk.P_BrandList) {
				for(uint i = 0; i < rBlk.P_BrandList->getCount(); i++) {
					const ExportBlock::BrandEntry & r_entry = rBlk.P_BrandList->at(i);
					Sdr_SPIIBrand out_brand_rec;
					out_brand_rec.ID      = r_entry.BrandID;
					out_brand_rec.OwnID   = r_entry.OwnerID;
					STRNSCPY(out_brand_rec.Name, r_entry.BrandName);
					STRNSCPY(out_brand_rec.OwnName, r_entry.OwnerName); 
					THROW(p_ie_brand->AppendRecord(&out_brand_rec, sizeof(out_brand_rec)));
				}
			}
		}
	}
	if(pPack->Rec.Flags & PLMF_EXPLOC) {
		PPImpExpParam param_loc;
		THROW(CreateWhList(rBlk));
		THROW(InitImpExpDbfParam(PPREC_SPIILOC, &param_loc, PPGetFilePathS(PPPATH_OUT, PPFILNAM_PALM_LOC, temp_buf), 1));
		THROW_MEM(p_ie_loc = new PPImpExp(&param_loc, 0));
		THROW(p_ie_loc->OpenFileForWriting(0, 1));
		if(!(pPack->Rec.Flags & PLMF_BLOCKED)) {
			if(rBlk.P_WhList) {
				for(uint i = 0; i < rBlk.P_WhList->getCount(); i++) {
					const ExportBlock::WhEntry & r_entry = rBlk.P_WhList->at(i);
					if(pPack->LocList.CheckID(r_entry.WhID)) {
						Sdr_SPIILoc out_loc_rec;
						out_loc_rec.ID = r_entry.WhID;
						STRNSCPY(out_loc_rec.Name, r_entry.WhName);
						THROW(p_ie_loc->AppendRecord(&out_loc_rec, sizeof(out_loc_rec)));
					}
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_grp_tbl);
	ZDELETE(p_goods_tbl);
	ZDELETE(p_qk_tbl);
	ZDELETE(p_ie_brand);
	ZDELETE(p_ie_loc);
	ZDELETE(p_ie_quots);
	PPSetAddedMsgString(0);
	return ok;
}

int PPObjStyloPalm::CopyToFTP(PPID id, int delAfterCopy, PPLogger * pLog)
{
	int    ok = 1;
	PPIDArray palm_list;
	PPStyloPalmPacket palm_pack;
	WinInetFTP ftp;
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	THROW(GetPacket(id, &palm_pack) > 0);
	THROW(GetChildList(id, palm_list));
	THROW(obj_acct.Get(palm_pack.Rec.FTPAcctID, &acct));
	THROW(ftp.Init());
	THROW(ftp.Connect(&acct));
	for(uint i = 0; i < palm_list.getCount(); i++) {
	  	PPStyloPalmPacket child_pack;
	  	if(GetPacket(palm_list.at(i), &child_pack) > 0) {
	  		int r = CopyFilesToFTP(&child_pack, &ftp, 1, delAfterCopy, pLog);
	  		ok = (r == 0) ? 0 : ok;
	  		if(!ok)
	  			PPSetError(PPERR_SENDTOFTP);
	  		THROW(ftp.ReInit());
	  	}
	}
	CATCHZOK
	ftp.UnInit();
	return ok;
}

int PPObjStyloPalm::DeleteImportData(PPID id)
{
	int    ok = 1;
	PPIDArray palm_list;
	PPStyloPalmPacket palm_pack;
	THROW(GetPacket(id, &palm_pack) > 0);
	THROW(GetChildList(id, palm_list));
	for(uint i = 0; i < palm_list.getCount(); i++) {
		PPStyloPalmPacket child_pack;
		if(GetPacket(palm_list.at(i), &child_pack) > 0)
			THROW(DeleteImportFiles(&child_pack));
	}
	CATCHZOK
	return ok;
}

int PPObjStyloPalm::RegisterDevice(PPID id, RegisterDeviceBlock * pBlk, int use_ta)
{
	int    ok = 1;
	PPStyloPalmPacket pack;
	THROW(GetPacket(id, &pack) > 0)
	THROW_PP_S(!(pack.Rec.Flags & PLMF_REGISTERED), PPERR_STYLOPALMREGISTERED, pack.Rec.Name);
    pack.Rec.Flags |= PLMF_REGISTERED;
    pack.Rec.RegisterTime = getcurdatetime_();
    THROW(PutPacket(&id, &pack, 1));
	CATCHZOK
	return ok;
}

struct PalmExpStruc {
	PalmExpStruc(PPID goodsGrpID, long flags) : GoodsGrpID(goodsGrpID), Flags(flags)
	{
	}
	int    IsAnalog(const PalmExpStruc * pItem) const
	{
		int    yes = 0;
		if(pItem) {
			if(pItem->Flags & PLMF_BLOCKED) {
				if(Flags & PLMF_BLOCKED)
					yes = 1;
				else
					yes = 0;
			}
			else if(pItem->GoodsGrpID == GoodsGrpID && (pItem->Flags & PLMF_EXPBRAND) == (Flags & PLMF_EXPBRAND) &&
				(pItem->Flags & PLMF_EXPCLIDEBT) == (Flags & PLMF_EXPCLIDEBT) &&
				(pItem->Flags & PLMF_EXPLOC) == (Flags & PLMF_EXPLOC) &&
				LocList.IsEq(&pItem->LocList) && QkList.IsEq(&pItem->QkList))
				yes = 1;
		}
		return yes;
	}
	const  PPID   GoodsGrpID;
	long   Flags;
	PPIDArray LocList;
	PPIDArray QkList; // @v10.8.6
	PPIDArray PalmList;
};

struct PalmDebtExpStruc {
	PalmDebtExpStruc(const PPStyloPalmPacket & rPack, int dontPrepareDebtData)
	{
		PPOprKind opk;
		AcsID = GetOpData(rPack.Rec.OrderOpID, &opk) ? opk.AccSheetID : 0;
		SETIFZ(AcsID, GetSellAccSheet());
		Flags = (rPack.Rec.Flags & (PLMF_EXPCLIDEBT|PLMF_EXPSTOPFLAG|PLMF_BLOCKED));
		if(dontPrepareDebtData)
			Flags &= ~PLMF_EXPCLIDEBT;
		PalmList.add(rPack.Rec.ID);
	}
	int    IsAnalog(const PalmDebtExpStruc * pItem, int dontPrepareDebt) const
	{
		if(pItem) {
			const long _fmask = dontPrepareDebt ? PLMF_EXPSTOPFLAG : (PLMF_EXPCLIDEBT|PLMF_EXPSTOPFLAG|PLMF_BLOCKED);
			if(pItem->AcsID == AcsID && (pItem->Flags & _fmask) == (Flags & _fmask))
				return 1;
		}
		return 0;
	}
	PPID   AcsID;
	long   Flags;        // PLMF_EXPCLIDEBT, PLMF_EXPSTOPFLAG
	PPIDArray PalmList;
};

static PalmCfgItem * FASTCALL GetPalmConfigItem(TSCollection <PalmCfgItem> & rList, PPID palmID)
{
	uint   pos = 0;
	return rList.lsearch(&palmID, &pos, CMPF_LONG) ? rList.at(pos) : 0;
}

int PPObjStyloPalm::ExportClients(PPID acsID, long palmFlags, ExportBlock & rBlk)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	const  bool use_omt_paym_amt = LOGIC(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	DbfTable * p_client_tbl = 0, * p_debt_tbl = 0, * p_addr_tbl = 0;
	DebtTrnovrViewItem debt_item;

	THROW(p_client_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_CLIENT, DBFS_PALM_CLIENTS));
	THROW(p_addr_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_CLIADDR, DBFS_PALM_CLIADDR));
	THROW(p_debt_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_CLIDEBT, DBFS_PALM_CLIDEBT));
	if(palmFlags & PLMF_EXPCLIDEBT && !(palmFlags & PLMF_BLOCKED)) {
		DebtTrnovrFilt debt_filt;
		debt_filt.AccSheetID = acsID;
		debt_filt.Flags |= DebtTrnovrFilt::fDebtOnly;
		THROW_MEM(SETIFZ(rBlk.P_DebtView, new PPViewDebtTrnovr));
		THROW(rBlk.P_DebtView->Init_(&debt_filt));
	}
	if(!(palmFlags & PLMF_BLOCKED)) {
		SString wait_msg, addr, inn_buf;
		PPIDArray dlvr_loc_list;
		PPViewArticle ar_view;
		ArticleFilt ar_filt;
		ar_filt.AccSheetID = acsID;
		ArticleViewItem ar_item;
		PPObjAccSheet acc_sheet_obj;
		PPAccSheet acs_rec;

		THROW(acc_sheet_obj.Fetch(acsID, &acs_rec) > 0);
		PPLoadText(PPTXT_WAIT_PALMEXPCLI, wait_msg);
		THROW(ar_view.Init_(&ar_filt));
		for(ar_view.InitIteration(); ar_view.NextIteration(&ar_item) > 0;) {
			PPWaitPercent(ar_view.GetCounter(), wait_msg);
			if(ar_item.Closed == 0) { // Не будем выгружать пассивные статьи
				inn_buf.Z();
				long   _flags = 0;
				PPID   quot_kind_id = 0;
				PPClientAgreement cli_agt;
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					uint   i;
					dlvr_loc_list.clear();
					THROW(rBlk.P_PsnObj->GetDlvrLocList(ar_item.ObjID, &dlvr_loc_list));
					for(i = 0; i < dlvr_loc_list.getCount(); i++) {
						const  PPID dlvr_loc_id = dlvr_loc_list.at(i);
						rBlk.P_LocObj->GetAddress(dlvr_loc_id, 0, addr);
						if(addr.NotEmptyS()) {
							DbfRecord drec_addr(p_addr_tbl);
							drec_addr.put(1, dlvr_loc_id);
							drec_addr.put(2, ar_item.ID);
							drec_addr.put(3, addr);
							PPSetAddedMsgString(p_addr_tbl->getName());
							THROW_PP(p_addr_tbl->appendRec(&drec_addr), PPERR_DBFWRFAULT);
						}
					}
					rBlk.P_PsnObj->GetRegNumber(ar_item.ObjID, PPREGT_TPID, inn_buf);
				}
				if(rBlk.P_ArObj->GetClientAgreement(ar_item.ID, cli_agt, 0) > 0) {
					quot_kind_id = cli_agt.DefQuotKindID;
					if(cli_agt.Flags & AGTF_DONTUSEMINSHIPMQTTY)
						_flags |= CLIENTF_DONTUSEMINSHIPMQTTY;
				}
				if(rBlk.P_DebtView) {
					PayableBillList pb_list;
					PayableBillListItem * p_pb_item;
					rBlk.P_DebtView->GetPayableBillList(ar_item.ID, 0L, &pb_list);
					for(uint i = 0; pb_list.enumItems(&i, (void **)&p_pb_item);) {
						BillTbl::Rec bill_rec;
						if(p_bobj->Search(p_pb_item->ID, &bill_rec) > 0) {
							const double amt = BR2(bill_rec.Amount);
							double paym = 0.0;
							//
							// Извлечение примечания к долговому документу
							//
							/*
							{
								uint pos = 0;
								SString memos;
								StringSet ss(MemosDelim);

								BillObj->FetchExtMemo(p_pb_item->ID, memos);
								ss.setBuf(memos, prev_memos.Len());
								if(ss.search(PalmMemo, &pos, 0) > 0)
									ss.get(&pos, memo);
							}
							*/
							// @v11.1.12 BillCore::GetCode(bill_rec.Code);
							if(use_omt_paym_amt)
								paym = bill_rec.PaymAmount;
							else
								p_bobj->P_Tbl->CalcPayment(p_pb_item->ID, 0, 0, p_pb_item->CurID, &paym);
							const double debt = R2(amt - paym);
							if(debt > 0.0) {
								PPBillExt bill_ext;
								PPFreight freight;
								DbfRecord drec_debt(p_debt_tbl);
								p_bobj->FetchExt(p_pb_item->ID, &bill_ext);
								drec_debt.put(1, ar_item.ID);
								drec_debt.put(2, p_pb_item->ID);
								drec_debt.put(3, bill_rec.Code);
								drec_debt.put(4, bill_rec.Dt);
								drec_debt.put(5, amt);
								drec_debt.put(6, debt);
								drec_debt.put(7, bill_ext.AgentID);
								// drec_debt.put(8, (const char *)memo);
								PPSetAddedMsgString(p_debt_tbl->getName());
								THROW_PP(p_debt_tbl->appendRec(&drec_debt), PPERR_DBFWRFAULT);
							}
						}
					}
				}
				{
					if((palmFlags & PLMF_EXPSTOPFLAG) && (ar_item.Flags & ARTRF_STOPBILL)) {
						_flags |= CLIENTF_BLOCKED;
					}
					int    valid_debt = (rBlk.P_DebtView && rBlk.P_DebtView->GetItem(ar_item.ID, 0L, 0L, &debt_item) > 0) ? 1 : 0;
					DbfRecord drec_ar(p_client_tbl);
					drec_ar.put(1, ar_item.ID);
					drec_ar.put(2, ar_item.Name);
					drec_ar.put(3, ar_item.Article);
					drec_ar.put(4, inn_buf);
					drec_ar.put(5, quot_kind_id);
					if(valid_debt)
						drec_ar.put(6, debt_item.Debt);
					drec_ar.put(7, _flags);
					PPSetAddedMsgString(p_client_tbl->getName());
					THROW_PP(p_client_tbl->appendRec(&drec_ar), PPERR_DBFWRFAULT);
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_client_tbl);
	ZDELETE(p_addr_tbl);
	ZDELETE(p_debt_tbl);
	return ok;
}

/*
int PPObjStyloPalm::UpdateLicense()
{
	struct PalmLic {
		long  AgentID;
		char  AgentName[64];
		char  SubDir[32];
		char  UUID[40];
		long  Linked;
	};
	SFile lic_file;
	//
	// Получаем файл лицензий и проставляем уже использованные
	//

	//
	// Сбрасываем файл лицензии
	//

	//
	// Символ БД
	//
	char bd_symb[32];
	lic_file.Write(
	//
	// Информация о свободных лицензиях
	//
	{

		PPObjStyloPalm obj_palm;
		PPIDArray      palm_id_list;
		StrAssocList * p_palm_list = obj_palm.MakeStrAssocList(0);
		for(uint i = 0; i < p_palm_list->getCount(); i++) {
			SString path;
			PPStyloPalmPacket pack;
			THROW(GetPacket(p_palm_list->at(i).Id, &pack) > 0);
			pack.MakePath(0, path);
			if((pack.Rec.Flags & PLMF_ANDROID) && pack.Rec.AgentID && path.Len())
				palm_id_list.add(pack.Rec.ID;
		}
		for(i = 0; i < palm_id_list.getCount(); i++) {
			PPStyloPalmPacket pack;
			size_t lic_size = sizeof(PalmLic);
			PalmLic lic;
			MEMSZERO(lic);
			THROW(GetPacket(p_palm_list->at(i).Id, &pack) > 0);
			lic.AgentID = pack.Rec.AgentID;
			GetObjectName(PPOBJ_ARTICLE, pack.Rec.AgentID, lic.AgentName, sizeof(lic.AgentName));
			{
				int start_pos = 0;
				SString path, sub_dir;
				pPack->MakePath(0, path);
				path.RmvLastSlash();
				for(int i = path.Len() - 1; start_pos == 0 && i >= 0; i--) {
					char symb = path.C(i);
					if(c == "//" || c = "\\")
						start_pos = i + 1;
				}
				if(start_pos >= 0)
					path.Sub(start_pos, path.Len() - start_pos, sub_dir);
				sub_dir.CopyTo(lic.SubDir, sizeof(lic.SubDir));
			}
			lic.Linked = BIN(pack.Rec.Flags & PLMF_PALMLINKED);
			lic_file.Write(&lic_size, sizeof(lic_size));
			lic_file.Write(&lic, sizeof(lic));
		}
		THROW(MoveOutFile(PPFILNAM_PALM_LIC, 0, path, 0, 0));
	}
}
*/
int PPObjStyloPalm::ExportData(const PalmPaneData & rParam)
{
	int    ok = 1;
	const  int dont_prepare_debt_data = BIN(rParam.Flags & PalmPaneData::fExclExpDebts);
	uint   i, j;
	SString temp_buf;
	DbfTable * p_dbf_tbl = 0;
	PPStyloPalmConfig sp_cfg;
	SString out_path, palm_path;
	TSCollection <PalmExpStruc> exp_list;
	TSCollection <PalmDebtExpStruc> debt_exp_list;
	TSCollection <PalmCfgItem> cfg_list;
	AndroidDevs andr_devs;
	PPViewPrjTask v_todo;
	PPObjPrjTask todo_obj; // @v10.7.2
	PPObjQuotKind qk_obj;
	ExportBlock _blk;
	THROW(ReadConfig(&sp_cfg));
	PPGetPath(PPPATH_OUT, out_path);
	// @v10.8.6 THROW(CreateQkList(_blk));
	THROW(CreateGoodsGrpList(_blk));
	{
		//
		// Создаем искусственные списки групп устройств, разделенных по признакам списков складов, товарных групп, видов котировок
		//
		PPStyloPalmPacket palm_pack;
		PPIDArray palm_list;
		THROW(GetChildList(rParam.PalmID, palm_list));
		for(i = 0; i < palm_list.getCount(); i++) {
			THROW(GetPacket(palm_list.at(i), &palm_pack) > 0);
			if(!isempty(palm_pack.P_Path)) {
				const long dvc_flags = dont_prepare_debt_data ? (palm_pack.Rec.Flags & ~PLMF_EXPCLIDEBT) : palm_pack.Rec.Flags;
				PalmCfgItem * p_cfg_item__ = new PalmCfgItem;
				THROW(andr_devs.Add(&palm_pack)); // Инициализируем writer для андроид устройств
				p_cfg_item__->ID = palm_pack.Rec.ID;
				p_cfg_item__->PalmFlags = dvc_flags;
				p_cfg_item__->C.MaxNotSentOrd = static_cast<uint16>(palm_pack.Rec.MaxUnsentOrders);
				palm_pack.MakeOutputPath(p_cfg_item__->Path);
				cfg_list.insert(p_cfg_item__);
				{
					PalmExpStruc * p_item = new PalmExpStruc(palm_pack.Rec.GoodsGrpID, (dvc_flags & (PLMF_INHMASK|PLMF_BLOCKED)));
					THROW_MEM(p_item);
					if(palm_pack.LocList.IsExists()) {
						p_item->LocList = palm_pack.LocList.Get();
						p_item->LocList.sortAndUndup();
					}
					if(palm_pack.QkList__.IsExists()) {
						p_item->QkList = palm_pack.QkList__.Get();
						p_item->QkList.sortAndUndup();
					}
					else {
						QuotKindFilt qk_filt;
						StrAssocArray temp_qk_list;
						qk_filt.Flags = (QuotKindFilt::fExclNotForBill|QuotKindFilt::fSortByRankName);
						qk_filt.MaxItems = PALM_MAX_QUOT;
						qk_obj.MakeList(&qk_filt, &temp_qk_list);
						temp_qk_list.GetIdList(p_item->QkList);
						p_item->QkList.sortAndUndup();
					}
					p_item->PalmList.addUnique(palm_pack.Rec.ID);
					//
					// Если в списке уже есть аналог нового элемента, то просто изменим в ней
					// список устройсв и флаги, в противном случае добавим новый элемент в список.
					//
					for(j = 0; j < exp_list.getCount(); j++) {
						PalmExpStruc * p_ex_item = exp_list.at(j);
						if(p_item->IsAnalog(p_ex_item)) {
							p_ex_item->Flags |= p_item->Flags;
							p_ex_item->PalmList.addUnique(palm_pack.Rec.ID);
							ZDELETE(p_item);
							break;
						}
					}
					if(p_item) {
						THROW_SL(exp_list.insert(p_item));
					}
				}
				{
					PalmDebtExpStruc * p_item = new PalmDebtExpStruc(palm_pack, dont_prepare_debt_data);
					THROW_MEM(p_item);
					for(j = 0; j < debt_exp_list.getCount(); j++) {
						PalmDebtExpStruc * p_ex_item = debt_exp_list.at(j);
						if(p_item->IsAnalog(p_ex_item, dont_prepare_debt_data)) {
							p_ex_item->Flags |= p_item->Flags;
							p_ex_item->PalmList.addUnique(palm_pack.Rec.ID);
							ZDELETE(p_item);
							break;
						}
					}
					if(p_item) {
						THROW_SL(debt_exp_list.insert(p_item));
					}
				}
			}
		}
	}
	for(i = 0; i < debt_exp_list.getCount(); i++) {
		PalmDebtExpStruc * p_item = debt_exp_list.at(i);
		if(p_item) {
			PPStyloPalmPacket palm_pack;
			PPWaitStart();
			THROW(ExportClients(p_item->AcsID, p_item->Flags, _blk));
			for(j = 0; j < p_item->PalmList.getCount(); j++) {
				PalmCfgItem * p_cfg_item = GetPalmConfigItem(cfg_list, p_item->PalmList.get(j));
				assert(p_cfg_item);
				if(p_cfg_item) {
					const long palm_flags = p_cfg_item->PalmFlags;
					assert((p_item->Flags & PLMF_EXPCLIDEBT) == (palm_flags & PLMF_EXPCLIDEBT));
					p_cfg_item->C.TmClient = getcurdatetime_();
					THROW(MoveOutFile(PPFILNAM_PALM_CLIENT,   p_cfg_item, &andr_devs));
					THROW(MoveOutFile(PPFILNAM_PALM_CLIADDR,  p_cfg_item, &andr_devs));
					if(palm_flags & PLMF_EXPCLIDEBT) {
						THROW(MoveOutFile(PPFILNAM_PALM_CLIDEBT, p_cfg_item, &andr_devs));
						p_cfg_item->C.TmCliDebt = p_cfg_item->C.TmClient;
					}
				}
			}
		}
	}
	for(i = 0; i < exp_list.getCount(); i++) {
		PalmExpStruc * p_item = exp_list.at(i);
		if(p_item) {
			PPStyloPalmPacket palm_pack;
			PPWaitStart();
			palm_pack.Rec.GoodsGrpID = p_item->GoodsGrpID;
			palm_pack.Rec.Flags      = p_item->Flags;
			if(p_item->LocList.getCount()) {
				palm_pack.LocList.Set(&p_item->LocList);
				uint lc = palm_pack.LocList.GetCount();
				if(lc) {
					LongArray * p_inner_list = palm_pack.LocList.GetP();
					do {
						const  PPID loc_id = p_inner_list->get(--lc);
						LocationTbl::Rec loc_rec;
						if(_blk.P_LocObj->Search(loc_id, &loc_rec) <= 0 || loc_rec.Type != LOCTYP_WAREHOUSE)
							p_inner_list->atFree(lc);
					} while(lc);
				}
			}
			palm_pack.QkList__.Set(&p_item->QkList); // @v10.8.6
			// } @v10.8.6
			THROW(ExportGoods(&palm_pack, _blk));
			for(j = 0; j < p_item->PalmList.getCount(); j++) {
				PalmCfgItem * p_cfg_item = GetPalmConfigItem(cfg_list, p_item->PalmList.get(j));
				if(p_cfg_item)
					p_cfg_item->C.TmGoods = getcurdatetime_();
			}
			{
				//
				// Экспорт продаж по клиентам
				//
				PPViewTrfrAnlz ta_view;
				TrfrAnlzFilt ta_filt;
				TrfrAnlzViewItem ta_item;
				ZDELETE(p_dbf_tbl);
				THROW(p_dbf_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_CLISELL, DBFS_PALM_CLISELL));
				if(palm_pack.Rec.Flags & PLMF_EXPSELL && sp_cfg.SellOpID && sp_cfg.SellAnlzTerm > 0) {
					SString wait_msg;
					//
					// Расчет выполняем на cfg.SellAnlzTerm недель до последнего воскресенья включительно
					//
					ta_filt.Period.upp = LConfig.OperDate;
					int dow = dayofweek(&ta_filt.Period.upp, 1);
					if(dow < 7)
						ta_filt.Period.upp = plusdate(ta_filt.Period.upp, -dow);
					ta_filt.Period.low = plusdate(ta_filt.Period.upp, -7*sp_cfg.SellAnlzTerm+1);
					ta_filt.LocList = palm_pack.LocList;
					ta_filt.GoodsGrpID = palm_pack.Rec.GoodsGrpID;
					ta_filt.OpID = sp_cfg.SellOpID;
					ta_filt.Grp = TrfrAnlzFilt::gGoodsCntragentDate;
					ta_filt.Flags |= (TrfrAnlzFilt::fDiffByDlvrAddr | TrfrAnlzFilt::fDontInitSubstNames);
					ta_filt.Sgd = sgdWeek;
					PPLoadText(PPTXT_WAIT_PALMEXPSELL, wait_msg);
					THROW(ta_view.Init_(&ta_filt));
					PPSetAddedMsgString(p_dbf_tbl->getName());
					for(ta_view.InitIteration(PPViewTrfrAnlz::OrdByDefault); ta_view.NextIteration(&ta_item) > 0;) {
						PPWaitPercent(ta_view.GetCounter(), wait_msg);
						DbfRecord rec(p_dbf_tbl);
						rec.put(1, ta_item.ArticleID);
						rec.put(2, ta_item.DlvrLocID);
						rec.put(3, ta_item.GoodsID);
						rec.put(4, ta_item.Dt);
						rec.put(5, fabs(ta_item.Qtty));
						THROW_PP(p_dbf_tbl->appendRec(&rec), PPERR_DBFWRFAULT);
					}
					//
					// В конфигурацию записываем не текущее время, а конечную дату, на которую сформирован отчет.
					// Это позволит сэкономить на том, что данные фактически готовятся на конец прошедшей недели.
					//
					for(j = 0; j < p_item->PalmList.getCount(); j++) {
						PalmCfgItem * p_cfg_item = GetPalmConfigItem(cfg_list, p_item->PalmList.get(j));
						if(p_cfg_item) {
							p_cfg_item->C.TmCliSell.d = ta_filt.Period.upp;
							p_cfg_item->C.TmCliSell.t = ZEROTIME;
							p_cfg_item->C.NumSellWeeks = sp_cfg.SellAnlzTerm;
						}
					}
				}
				ZDELETE(p_dbf_tbl);
			}
			//
			// Задачи формируем для каждого агента в отдельности.
			// В этом же цикле разносим подготовленные файлы по каталогам назначения.
			//
			for(j = 0; j < p_item->PalmList.getCount(); j++) {
				const  PPID palm_id = p_item->PalmList.get(j);
				PPStyloPalmPacket child_pack;
				if(GetPacket(palm_id, &child_pack) > 0) {
					PalmCfgItem * p_cfg_item = GetPalmConfigItem(cfg_list, palm_id);
					{
						PPID   employer_id = ObjectToPerson(child_pack.Rec.AgentID, 0);
						ZDELETE(p_dbf_tbl);
						THROW(p_dbf_tbl = Palm_CreateDbfTable(PPFILNAM_PALM_TODO, DBFS_PALM_TODO));
						if(employer_id) {
							PrjTaskViewItem item;
							PrjTaskFilt todo_filt;
							todo_filt.Flags |= PrjTaskFilt::fNoTempTable;
							todo_filt.Kind = TODOKIND_TASK;
							todo_filt.EmployerID = employer_id;
							todo_filt.IncludeStatus(TODOSTTS_NEW);
							todo_filt.IncludeStatus(TODOSTTS_INPROGRESS);
							todo_filt.IncludeStatus(TODOSTTS_ONHOLD);
							THROW(v_todo.Init_(&todo_filt));
							for(v_todo.InitIteration(); v_todo.NextIteration(&item) > 0;) {
								DbfRecord rec(p_dbf_tbl);
								rec.put(1, item.ID);
								rec.put(2, item.Priority);
								rec.put(3, BIN(item.Status == TODOSTTS_COMPLETED));
								rec.put(4, item.StartDt); // Вариант: item.EstFinishDt
								todo_obj.GetItemDescr(item.ID, temp_buf);
								temp_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER);
								rec.put(6, temp_buf);
								todo_obj.GetItemMemo(item.ID, temp_buf);
								temp_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER);
								rec.put(7, temp_buf);
								THROW_PP_S(p_dbf_tbl->appendRec(&rec), PPERR_DBFWRFAULT, p_dbf_tbl->getName());
							}
						}
						ZDELETE(p_dbf_tbl);
					}
					if(p_cfg_item) {
						p_cfg_item->C.TmToDo = getcurdatetime_();
						if(p_cfg_item->Path.NotEmpty()) {
							//
							// Разноска файлов по каталогам назначения.
							//
							long   palm_flags = child_pack.Rec.Flags;
							THROW(MoveOutFile(PPFILNAM_PALM_GOODS,    p_cfg_item, &andr_devs));
							if(palm_flags & PLMF_EXPBRAND)
								THROW(MoveOutFile(PPFILNAM_PALM_BRAND, p_cfg_item, &andr_devs));
							if(palm_flags & PLMF_EXPLOC)
								THROW(MoveOutFile(PPFILNAM_PALM_LOC,  p_cfg_item, &andr_devs));
							THROW(MoveOutFile(PPFILNAM_PALM_GOODSGRP, p_cfg_item, &andr_devs));
							THROW(MoveOutFile(PPFILNAM_PALM_QUOTKIND, p_cfg_item, &andr_devs));
							THROW(MoveOutFile(PPFILNAM_PALM_QUOTS,    p_cfg_item, &andr_devs));
							if(palm_flags & PLMF_EXPSELL)
								THROW(MoveOutFile(PPFILNAM_PALM_CLISELL, p_cfg_item, &andr_devs));
							THROW(MoveOutFile(PPFILNAM_PALM_TODO, p_cfg_item, &andr_devs));
						}
					}
				}
			}
		}
	}
	//
	// В завершении сбрасываем файлы конфигурации в каталоги каждого из устройств
	//
	for(j = 0; j < cfg_list.getCount(); j++) {
		PalmCfgItem * p_cfg_item = cfg_list.at(j);
		if(p_cfg_item && p_cfg_item->Path.NotEmpty()) {
			THROW_PP_S(p_cfg_item->C.Write(out_path), PPERR_EXPFOPENFAULT, out_path);
			THROW(MoveOutFilePlain(0, P_PalmConfigFileName, p_cfg_item, &andr_devs));
		}
	}
	CATCHZOK
	ZDELETE(p_dbf_tbl);
	return ok;
}

class PalmPaneDialog : public TDialog {
	DECL_DIALOG_DATA(PalmPaneData);
public:
	PalmPaneDialog() : TDialog(DLG_PALMPANE)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 0, PalmPaneData::fUpdateData);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 1, PalmPaneData::fExportFTP);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 2, PalmPaneData::fImportFTP);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 3, PalmPaneData::fDelImpData);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 4, PalmPaneData::fExclExpDebts);
		AddClusterAssoc(CTL_PALMPANE_FLAGS, 5, PalmPaneData::fIgnoreMutex);
		SetClusterData(CTL_PALMPANE_FLAGS, Data.Flags);
		SetupPPObjCombo(this, CTLSEL_PALMPANE_PALM, PPOBJ_STYLOPALM, Data.PalmID, OLW_CANSELUPLEVEL, 0);
		PPIDArray op_type_list;
		op_type_list.addzlist(PPOPT_GOODSORDER, PPOPT_GOODSEXPEND, 0L);
		SetupOprKindCombo(this, CTLSEL_PALMPANE_OP, Data.OpID, 0, &op_type_list, 0);
		SetupPPObjCombo(this, CTLSEL_PALMPANE_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
		setupPalm();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_PALMPANE_FLAGS, &Data.Flags);
		getCtrlData(sel = CTLSEL_PALMPANE_PALM, &Data.PalmID);
		THROW_PP(Data.PalmID, PPERR_USERINPUT);
		getCtrlData(sel = CTLSEL_PALMPANE_OP, &Data.OpID);
		THROW_PP(Data.OpID, PPERR_OPRKINDNEEDED);
		getCtrlData(sel = CTLSEL_PALMPANE_LOC, &Data.LocID);
		THROW_PP(Data.LocID, PPERR_LOCNEEDED);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PALMPANE_PALM)) {
			setupPalm();
			clearEvent(event);
		}
	}
	void   setupPalm()
	{
		getCtrlData(CTLSEL_PALMPANE_PALM, &Data.PalmID);
		if(Data.PalmID) {
			PPStyloPalm palm_rec;
			if(SearchObject(PPOBJ_STYLOPALM, Data.PalmID, &palm_rec) > 0) {
				Data.OpID = palm_rec.OrderOpID;
				setCtrlData(CTLSEL_PALMPANE_OP, &Data.OpID);
			}
		}
	}
};

/*static*/int PPObjStyloPalm::EditImpExpData(PalmPaneData * pData)
{
	int    ok = -1;
	PalmPaneDialog * dlg = 0;
	PalmPaneData data;
	RVALUEPTR(data, pData);
	THROW(CheckDialogPtr(&(dlg = new PalmPaneDialog())));
	dlg->setDTS(&data);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&data)) {
			ASSIGN_PTR(pData, data);
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

/*static*/int PPObjStyloPalm::ImpExp(const PalmPaneData * pData)
{
	int    ok = 1;
	const  PPConfig & r_cfg = LConfig;
	int    edit_param = 1;
	int    r = 0;
	int    locked = 0;
	PPLogger logger;
	long mutex_id = 0;
	PPSyncItem sync_item;
	PalmPaneData data;
	PPObjStyloPalm palm_obj;
	RVALUEPTR(data, pData);
	if(data.PalmID == 0) {
		PPID temp_id = 0;
		if(palm_obj.EnumItems(&temp_id) > 0) {
			data.PalmID = temp_id;
			if(palm_obj.EnumItems(&temp_id) > 0)
				data.PalmID = 0;
		}
		data.LocID = r_cfg.Location;
	}
	else if(!(data.Flags & PalmPaneData::fForceEdit))
		edit_param = 0;
	if(edit_param) {
		THROW(ok = EditImpExpData(&data));
	}
	if(ok > 0) {
		if(!(data.Flags & data.fIgnoreMutex)) {
			if((r = DS.GetSync().CreateMutex_(r_cfg.SessionID, PPOBJ_STYLOPALM, -1, &mutex_id, &sync_item)) < 0) {
				THROW_PP_S(0, PPERR_PALMEXPIMPBLOCKED, sync_item.Name);
			}
			else if(!r) {
				THROW_PP(0, PPERR_LOGICLOCKFAULT);
			}
			locked = 1;
		}
		{
			int    do_remove_imp_data = BIN(data.Flags & PalmPaneData::fDelImpData);
			int    import_from_ftp_ok = 1;
			int    ftp_err_code = 0;
			SString ftp_add_errmsg;
			PPWaitStart();
			if(data.Flags & PalmPaneData::fImportFTP) {
				import_from_ftp_ok = palm_obj.CopyFromFTP(data.PalmID, do_remove_imp_data, &logger);
				ftp_err_code   = PPErrCode;
				ftp_add_errmsg = DS.GetTLA().AddedMsgString;
				PPWaitStart();
			}
			if(!palm_obj.ImportData(data.PalmID, data.OpID, data.LocID, &logger)) {
				logger.LogLastError();
				do_remove_imp_data = 0;
			}
			PPSetAddedMsgString(ftp_add_errmsg);
			THROW_PP(import_from_ftp_ok, ftp_err_code);
			if(data.Flags & PalmPaneData::fUpdateData)
				THROW(palm_obj.ExportData(data));
			if(data.Flags & PalmPaneData::fExportFTP)
				THROW(palm_obj.CopyToFTP(data.PalmID, BIN(data.Flags & PalmPaneData::fDelImpData), &logger));
			if(do_remove_imp_data)
				THROW(palm_obj.DeleteImportData(data.PalmID));
			PPWaitStop();
		}
	}
	CATCHZOKPPERR
	if(locked)
		DS.GetSync().ReleaseMutex(mutex_id);
	{
		SString file_name;
		logger.Save(PPGetFilePathS(PPPATH_LOG, PPFILNAM_INFO_LOG, file_name), 0);
	}
	return ok;
}

PalmImportWaiter::PalmImportWaiter(PalmImportProc proc, void * procExtraPtr) : Semaphore(0), P_DeviceList(0), Proc(proc), ProcExtraPtr(procExtraPtr)
{
	InitDeviceList();
}

PalmImportWaiter::~PalmImportWaiter()
{
	delete P_DeviceList;
	Queue.Destroy();
}

int PalmImportWaiter::InitDeviceList()
{
	P_DeviceList = new SArray(sizeof(DeviceEntry));
	PPStyloPalm rec;
	for(SEnum en = PPRef->Enum(PPOBJ_STYLOPALM, 0); en.Next(&rec) > 0;) {
		if(!(rec.Flags & PLMF_GENERIC) && (rec.Flags & PLMF_IMPASCHECKS)) {
			PPStyloPalmPacket pack;
			if(PalmObj.GetPacket(rec.ID, &pack) > 0 && pack.P_Path) {
				DeviceEntry entry;
				entry.ID = rec.ID;
				STRNSCPY(entry.Path, pack.P_Path);
				strcat(setLastSlash(entry.Path), "OUT");
				P_DeviceList->insert(&entry);
			}
		}
	}
	return 1;
}

int PalmImportWaiter::ProcessQueue()
{
	if(Proc) {
		while(!Queue.IsEmpty()) {
			PalmBillPacket * p_pack = Queue.Peek();
			int    r = Proc(p_pack, ProcExtraPtr);
			if(r == PIPR_ERROR_BREAK)
				break;
			else if(r == PIPR_OK_CONTINUE) {
				p_pack = Queue.Pop();
				delete p_pack;
			}
			else if(r == PIPR_OK_BREAK) {
				p_pack = Queue.Pop();
				delete p_pack;
				break;
			}
			else if(r == PIPR_OK_DESTROY) {
				Queue.Destroy();
				break;
			}
		}
	}
	return 1;
}

int PalmImportWaiter::Activate()
{
	int    ok = 1;
	if(!Semaphore) {
		Semaphore++;
		if(P_DeviceList) {
			DeviceEntry * p_entry;
			for(uint i = 0; P_DeviceList->enumItems(&i, (void **)&p_entry);) {
				PalmInputParam param;
				param.P_BillQueue = &Queue;
				PalmObj.ReadInput(p_entry->ID, &param, 0, 0, 0);
			}
		}
		ProcessQueue();
		Semaphore--;
	}
	return ok;
}
//
//
//
PalmInputParam::PalmInputParam() : P_BillQueue(0), P_ToDoQueue(0), P_DebtMemoQueue(0), P_GtList(0)
{
}
//
//
//
PalmDisplayBlock::PalmDisplayBlock()
{
	Clear();
}

void PalmDisplayBlock::Clear()
{
	Ver = DS.GetVersion();
	DvcID = 0;
	Flags = 0;
	Ctx = 0;
	CtxData.Z();
	DirectMsg.Z();
	QueuePos = 0;
}

class StyloDisplayQueue {
public:
	StyloDisplayQueue() : Q(128), DvcID(0)
	{
	}
	int    Lock()
	{
		Lck.Lock();
		return 1;
	}
	int    Unlock()
	{
		Lck.Unlock();
		return 1;
	}
	int    FASTCALL Put(const PalmDisplayBlock & rBlk)
	{
		int    ok = 1;
		uint   pos = 0;
		assert(!rBlk.DvcID || rBlk.DvcID == DvcID);
		PalmDisplayBlock * p_new_blk = new PalmDisplayBlock(rBlk);
		THROW_MEM(p_new_blk);
		p_new_blk->DvcID = DvcID;
		Lock();
		for(uint i = 0; i < L.getCount(); i++) {
			if(L.at(i) == 0) {
				pos = i+1;
				break;
			}
		}
		if(pos)
			L.atPut(pos-1, p_new_blk);
		else {
			THROW_SL(L.insert(p_new_blk));
			pos = L.getCount();
		}
		THROW_SL(Q.push(pos));
		Unlock();
		CATCHZOK
		return ok;
	}
	uint   Peek(PalmDisplayBlock & rBlk, int doLock)
	{
		int    ok = -1;
		if(doLock)
			Lock();
		if(Q.getNumItems()) {
			uint pos = Q.peek();
			assert(pos > 0 && pos <= L.getCount());
			const PalmDisplayBlock * p_blk = L.at(pos-1);
			assert(p_blk);
			rBlk = *p_blk;
			rBlk.QueuePos = pos;
			ok = 1;
		}
		if(doLock)
			Unlock();
		return ok;
	}
	int    Pop(PalmDisplayBlock * pBlk, int doLock)
	{
		int    ok = -1;
		if(doLock)
			Lock();
		if(Q.getNumItems()) {
			uint pos = Q.pop();
			assert(pos > 0 && pos <= L.getCount());
			const PalmDisplayBlock * p_blk = L.at(pos-1);
			assert(p_blk);
			if(pBlk) {
				*pBlk = *p_blk;
				pBlk->QueuePos = 0;
			}
			L.atPut(pos-1, 0);
			ok = 1;
		}
		if(doLock)
			Unlock();
		return ok;
	}

	PPID   DvcID;
	TSCollection <PalmDisplayBlock> L;
	TSQueue <uint> Q;
	SMtLock Lck;
};

static StyloDisplayQueue * FASTCALL GetStyloDisplayQueue(PPID dvcID)
{
	StyloDisplayQueue * p_queue = 0;
	SString queue_symb, temp_buf;
	(queue_symb = "PALMDISPLAYQUEUE").CatChar('&').Cat(dvcID);
	int    is_new = 0;
	long   symbol_id = SLS.GetGlobalSymbol(queue_symb, -1, 0);
	THROW_SL(symbol_id);
	if(symbol_id < 0) {
		TSClassWrapper <StyloDisplayQueue> cls;
		THROW_SL(symbol_id = SLS.CreateGlobalObject(cls));
		THROW_SL(p_queue = static_cast<StyloDisplayQueue *>(SLS.GetGlobalObject(symbol_id)));
		p_queue->DvcID = dvcID;
		{
			long s = SLS.GetGlobalSymbol(queue_symb, symbol_id, 0);
			assert(symbol_id == s);
		}
	}
	else {
		THROW_SL(p_queue = static_cast<StyloDisplayQueue *>(SLS.GetGlobalObject(symbol_id)));
		assert(p_queue->DvcID == dvcID);
	}
	CATCH
		p_queue = 0;
	ENDCATCH
	return p_queue;
}

//static
int PPObjStyloPalm::PutDisplayBlock(const PalmDisplayBlock & rBlk)
{
	int    ok = 1;
	assert(rBlk.DvcID);
	StyloDisplayQueue * p_queue = GetStyloDisplayQueue(rBlk.DvcID);
	assert(p_queue);
	THROW(p_queue);
	THROW(p_queue->Put(rBlk));
	CATCHZOK
	return ok;
}

//static
int PPObjStyloPalm::LockDisplayQueue(PPID dvcID)
{
	int    ok = 1;
	assert(dvcID);
	StyloDisplayQueue * p_queue = GetStyloDisplayQueue(dvcID);
	assert(p_queue);
	THROW(p_queue);
	THROW(p_queue->Lock());
	CATCHZOK
	return ok;
}

//static
int PPObjStyloPalm::UnlockDisplayQueue(PPID dvcID)
{
	int    ok = 1;
	assert(dvcID);
	StyloDisplayQueue * p_queue = GetStyloDisplayQueue(dvcID);
	assert(p_queue);
	THROW(p_queue);
	THROW(p_queue->Unlock());
	CATCHZOK
	return ok;
}

//static
int PPObjStyloPalm::PeekDisplayBlock(PPID dvcID, PalmDisplayBlock & rBlk, int lock)
{
	int    ok = -1;
	assert(dvcID);
	StyloDisplayQueue * p_queue = GetStyloDisplayQueue(dvcID);
	assert(p_queue);
	THROW(p_queue);
	ok = p_queue->Peek(rBlk, lock);
	CATCHZOK
	return ok;
}

//static
int PPObjStyloPalm::PopDisplayBlock(PPID dvcID, PalmDisplayBlock * pBlk, int lock)
{
	int    ok = 1;
	assert(dvcID);
	StyloDisplayQueue * p_queue = GetStyloDisplayQueue(dvcID);
	assert(p_queue);
	THROW(p_queue);
	THROW(p_queue->Pop(pBlk, lock));
	CATCHZOK
	return ok;
}
//
//
//
//
// Implementation of PPALDD_UhttStyloDevice
//
struct UhttStyloDeviceBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttStyloDeviceBlock()
	{
		Clear();
	}
	void Clear()
	{
		Pack.destroy();
		State = stFetch;
	}
	PPObjStyloPalm SpObj;
	PPStyloPalmPacket Pack;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttStyloDevice)
{
	if(Valid) {
		Extra[0].Ptr = new UhttStyloDeviceBlock();
		InitFixData(rscDefHdr, &H, sizeof(H));
	}
}

PPALDD_DESTRUCTOR(UhttStyloDevice)
{
	Destroy();
	delete static_cast<UhttStyloDeviceBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttStyloDevice::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttStyloDeviceBlock & r_blk = *static_cast<UhttStyloDeviceBlock *>(Extra[0].Ptr);
	r_blk.Clear();
	MEMSZERO(H);
	if(r_blk.SpObj.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
		SString temp_buf;
		H.ID        = r_blk.Pack.Rec.ID;
		H.ParentID  = r_blk.Pack.Rec.GroupID;
		H.Flags     = r_blk.Pack.Rec.Flags;
		H.DeviceVer = r_blk.Pack.Rec.DeviceVer;
		STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
		STRNSCPY(H.Symb, r_blk.Pack.Rec.Symb);
        temp_buf.Z().Cat(r_blk.Pack.Rec.RegisterTime, DATF_ISO8601CENT, 0);
        STRNSCPY(H.RegisterTime, temp_buf);
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttStyloDevice::Set(long iterId, int commit)
{
	int    ok = 1;
	SString temp_buf;
	UhttStyloDeviceBlock & r_blk = *static_cast<UhttStyloDeviceBlock *>(Extra[0].Ptr);
	const  PPID glob_acc_id = DS.GetConstTLA().GlobAccID;
	if(r_blk.State != r_blk.stSet) {
		r_blk.Clear();
		r_blk.State = r_blk.stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			PPStyloPalm parent_rec;
			r_blk.Pack.Rec.ID = H.ID;
			PPID   parent_id = H.ParentID;
			if(r_blk.SpObj.Search(parent_id, &parent_rec) > 0 && parent_rec.Flags & PLMF_GENERIC) {
				r_blk.Pack.Rec.GroupID = parent_id;
			}
			(temp_buf = H.Name).Strip().RevertSpecSymb(SFileFormat::Html);
			STRNSCPY(r_blk.Pack.Rec.Name, temp_buf);
			(temp_buf = H.Symb).Strip().RevertSpecSymb(SFileFormat::Html);
			STRNSCPY(r_blk.Pack.Rec.Symb, temp_buf);
		}
	}
	else {
		if(!r_blk.Pack.Rec.ID) {
			PPID   id_by_name = 0;
			PPID   id_by_symb = 0;
			if(r_blk.Pack.Rec.Name[0] && r_blk.SpObj.SearchByName(r_blk.Pack.Rec.Name, &id_by_name, 0) > 0) {
			}
			if(r_blk.Pack.Rec.Symb[0] && r_blk.SpObj.SearchBySymb(r_blk.Pack.Rec.Symb, &id_by_symb, 0) > 0) {
			}
			if(id_by_name) {
				temp_buf.Z().Cat(r_blk.Pack.Rec.Name).CatDiv(':', 1).Cat(r_blk.Pack.Rec.Symb);
				THROW_PP_S(!id_by_symb || id_by_name == id_by_symb, PPERR_STYLONAMESYMBCONFLICT, temp_buf);
				r_blk.Pack.Rec.ID = id_by_name;
			}
			else if(id_by_symb)
				r_blk.Pack.Rec.ID = id_by_symb;
		}
		PPGlobalAccRights rights_blk(PPTAG_GUA_STYLORIGHTS);
		int    has_rights = (r_blk.Pack.Rec.ID == 0) ? rights_blk.IsAllow(PPGlobalAccRights::fCreate) : rights_blk.IsAllow(PPGlobalAccRights::fEdit);
		if(has_rights) {
			PPID   id = r_blk.Pack.Rec.ID;
			if(id == 0) {
				THROW(r_blk.SpObj.PutPacket(&id, &r_blk.Pack, 1));
			}
			else {
				//
				// Ограниченное изменение записи устройства (только под определенной учетной записью и только некоторые атрибуты)
				//
				if(glob_acc_id) {
					;
				}
			}
			Extra[4].Ptr = reinterpret_cast<void *>(id);
		}
		else {
			PPSetError(PPERR_NORIGHTS);
			DS.GetTLA().AddedMsgStrNoRights = DS.GetTLA().GlobAccName;
			ok = 0;
		}
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
