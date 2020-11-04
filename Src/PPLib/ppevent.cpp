// OBJEVENT.CPP
// Copyright (c) A.Sobolev 2020
//
#include <pp.h>
#pragma hdrstop

PPEventSubscription::PPEventSubscription() : Tag(PPOBJ_EVENTSUBSCRIPTION), ID(0), Flags(0), EventType(0), ObjType(0), MinDetectionInterval(0), Reserve2(0)
{
	PTR32(Name)[0] = 0;
	PTR32(Symb)[0] = 0;
	memzero(Reserve, sizeof(Reserve));
}

int FASTCALL PPEventSubscription::IsEqual(const PPEventSubscription & rS) const
{
	int    eq = 1;
	if(!sstreq(Name, rS.Name))
		eq = 0;
	else if(!sstreq(Symb, rS.Symb))
		eq = 0;
	else if(Flags != rS.Flags)
		eq = 0;
	else if(EventType != rS.EventType)
		eq = 0;
	else if(ObjType != rS.ObjType)
		eq = 0;
	else if(NotifColor != rS.NotifColor)
		eq = 0;
	else if(MinDetectionInterval != rS.MinDetectionInterval)
		eq = 0;
	return eq;
}

PPEventSubscriptionPacket::PPEventSubscriptionPacket() : P_Filt(0)
{
}

PPEventSubscriptionPacket::PPEventSubscriptionPacket(const PPEventSubscriptionPacket & rS) : P_Filt(0)
{
	Copy(rS);
}

PPEventSubscriptionPacket::~PPEventSubscriptionPacket()
{
	ZDELETE(P_Filt);
}

PPEventSubscriptionPacket & FASTCALL PPEventSubscriptionPacket::operator = (const PPEventSubscriptionPacket & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPEventSubscriptionPacket::Copy(const PPEventSubscriptionPacket & rS)
{
	int    ok = 1;
	PPExtStrContainer::operator = (rS);
	Rec = rS.Rec;
	GuaList = rS.GuaList;
	UserList = rS.UserList;
	ZDELETE(P_Filt);
	if(rS.P_Filt) {
		PPBaseFilt::CopyBaseFiltPtr(rS.P_Filt->GetSignature(), rS.P_Filt, &P_Filt);
	}
	return ok;
}

int FASTCALL PPEventSubscriptionPacket::IsEqual(const PPEventSubscriptionPacket & rS) const
{
	int    eq = 1;
	const  int exfl[] = { extssMessage };
	if(!Rec.IsEqual(rS.Rec))
		eq = 0;
	else if(!PPExtStrContainer::IsEqual(rS, SIZEOFARRAY(exfl), exfl))
		eq = 0;
	else if(!GuaList.IsEqual(rS.GuaList))
		eq = 0;
	else if(!UserList.IsEqual(rS.UserList))
		eq = 0;
	else if(P_Filt && rS.P_Filt) {
		if(!P_Filt->IsEqual(rS.P_Filt, 0))
			eq = 0;
	}
	else if(P_Filt && !rS.P_Filt)
		eq = 0;
	else if(!P_Filt && rS.P_Filt)
		eq = 0;
	return eq;
}

PPObjEventSubscription::PPObjEventSubscription(void * extraPtr) : PPObjReference(PPOBJ_EVENTSUBSCRIPTION, extraPtr)
{
}

int PPObjEventSubscription::SerializePacket_WithoutRec(int dir, PPEventSubscriptionPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pPack->SerializeB(dir, rBuf, pSCtx));
	THROW(pPack->UserList.Serialize(dir, rBuf, pSCtx));
	THROW(pPack->GuaList.Serialize(dir, rBuf, pSCtx));
	if(dir > 0) {
		THROW(PPView::WriteFiltPtr(rBuf, pPack->P_Filt));
	}
	else if(dir < 0) {
		THROW(PPView::ReadFiltPtr(rBuf, &pPack->P_Filt));
	}
	CATCHZOK
	return ok;
}

int PPObjEventSubscription::SerializePacket(int dir, PPEventSubscriptionPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW(SerializePacket_WithoutRec(dir, pPack, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjEventSubscription::PutPacket(PPID * pID, PPEventSubscriptionPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	SBuffer sbuf;
	SSerializeContext sctx;
	PPEventSubscriptionPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				THROW(CheckRights(PPR_DEL));
				THROW(ref->RemoveItem(Obj, _id, 0));
				THROW(ref->RemoveProperty(Obj, _id, 0, 0));
				//THROW(RemoveSync(_id));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, hid, 0);
			}
		}
		else {
			THROW(CheckRightsModByID(pID));
			if(_id) {
				if(pPack->IsEqual(org_pack))
					ok = -1;
				else {
					THROW(SerializePacket_WithoutRec(+1, pPack, sbuf, &sctx));
					THROW(ref->UpdateItem(Obj, _id, &pPack->Rec, 1, 0));
					THROW(ref->PutPropSBuffer(Obj, _id, EVNTSUBSCRPRP_EXTENSION, sbuf, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(SerializePacket_WithoutRec(+1, pPack, sbuf, &sctx));
				THROW(ref->AddItem(Obj, &_id, &pPack->Rec, 0));
				pPack->Rec.ID = _id;
				THROW(ref->PutPropSBuffer(Obj, _id, EVNTSUBSCRPRP_EXTENSION, sbuf, 0));
				DS.LogAction(PPACN_OBJADD, Obj, _id, 0, 0);
				ASSIGN_PTR(pID, _id);
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		if(is_new) {
			*pID = 0;
			if(pPack)
				pPack->Rec.ID = 0;
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PPObjEventSubscription::GetPacket(PPID id, PPEventSubscriptionPacket * pPack)
{
	int    ok = Search(id, &pPack->Rec);
	if(ok > 0) {
		SBuffer sbuf;
		SSerializeContext sctx;
		THROW(ref->GetPropSBuffer(Obj, id, EVNTSUBSCRPRP_EXTENSION, sbuf));
		THROW(SerializePacket_WithoutRec(-1, pPack, sbuf, &sctx));
	}
	CATCHZOK
	return ok;
}

static void SetupEventTypeCombo(TDialog * pDlg, uint ctlselId, long initId)
{
	SString temp_buf;
	StrAssocArray type_list;
	type_list.Add(PPEVENTTYPE_OBJCREATED, PPLoadStringS("eventtype_objcreated", temp_buf));
	type_list.Add(PPEVENTTYPE_OUTER, PPLoadStringS("eventtype_outer", temp_buf));
	type_list.Add(PPEVENTTYPE_SPCBILLCHANGE, PPLoadStringS("eventtype_spcbillchange", temp_buf));
	type_list.Add(PPEVENTTYPE_SYSJOURNAL, PPLoadStringS("eventtype_sysjournal", temp_buf));
	type_list.Add(PPEVENTTYPE_LOTEXPIRATION, PPLoadStringS("eventtype_lotexpiration", temp_buf));
	SetupStrAssocCombo(pDlg, ctlselId, &type_list, initId, 0);
}

int PPObjEventSubscription::EditDialog(PPEventSubscriptionPacket * pPack)
{
	class EventSubscriptionDialog : public TDialog {
		DECL_DIALOG_DATA(PPEventSubscriptionPacket);
		enum {
			ctrgroupColor = 1
		};
	public:
		EventSubscriptionDialog() : TDialog(DLG_EVNTSUBSCR)
		{
			addGroup(ctrgroupColor, new ColorCtrlGroup(CTL_EVNTSUBSCR_COLOR, CTLSEL_EVNTSUBSCR_COLOR, cmSelColor, CTL_EVNTSUBSCR_SELCOLOR));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			RVALUEPTR(Data, pData);
			setCtrlData(CTL_EVNTSUBSCR_NAME, Data.Rec.Name);
			setCtrlData(CTL_EVNTSUBSCR_SYMB, Data.Rec.Symb);
			setCtrlLong(CTL_EVNTSUBSCR_ID, Data.Rec.ID);
			SetupEventTypeCombo(this, CTLSEL_EVNTSUBSCR_TYPE, Data.Rec.EventType);
			SetupObjListCombo(this, CTLSEL_EVNTSUBSCR_OBJ, Data.Rec.ObjType, 0);
			Data.GetExtStrData(Data.extssMessage, temp_buf);
			setCtrlString(CTL_EVNTSUBSCR_MSG, temp_buf);
			{
				ColorCtrlGroup::Rec color_rec;
				color_rec.SetupStdColorList();
				color_rec.C = NZOR(Data.Rec.NotifColor, GetColorRef(SClrLightskyblue));
				setGroupData(ctrgroupColor, &color_rec);
			}
			SetupType();
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			SString temp_buf;
			getCtrlData(CTL_EVNTSUBSCR_NAME, Data.Rec.Name);
			getCtrlData(CTL_EVNTSUBSCR_SYMB, Data.Rec.Symb);
			getCtrlData(CTLSEL_EVNTSUBSCR_TYPE, &Data.Rec.EventType);
			getCtrlData(CTLSEL_EVNTSUBSCR_OBJ, &Data.Rec.ObjType);
			getCtrlString(CTL_EVNTSUBSCR_MSG, temp_buf);
			Data.PutExtStrData(Data.extssMessage, temp_buf);
			{
				ColorCtrlGroup::Rec color_rec;
				getGroupData(ctrgroupColor, &color_rec);
				Data.Rec.NotifColor = color_rec.C;
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_EVNTSUBSCR_TYPE)) {
				SetupType();
			}
			else if(event.isCmd(cmGuaList)) {
				PPIDArray id_list;
				Data.GuaList.Get(id_list);
				ListToListData ltld(PPOBJ_GLOBALUSERACC, 0, &id_list);
				ltld.TitleStrID = 0; // PPTXT_XXX;
				if(ListToListDialog(&ltld) > 0)
					Data.GuaList.Set(&id_list);
			}
			else if(event.isCmd(cmUserList)) {
				PPIDArray id_list;
				Data.UserList.Get(id_list);
				ListToListData ltld(PPOBJ_USR, 0, &id_list);
				ltld.Flags |= ListToListData::fIsTreeList;
				ltld.TitleStrID = 0; // PPTXT_XXX;
				if(ListToListDialog(&ltld) > 0)
					Data.UserList.Set(&id_list);
			}
			else if(event.isCmd(cmFilter)) {
				int    filt_id = 0;
				int    view_id = 0;
				getCtrlData(CTLSEL_EVNTSUBSCR_TYPE, &Data.Rec.EventType);
				switch(Data.Rec.EventType) {
					case PPEVENTTYPE_OBJCREATED:
						getCtrlData(CTLSEL_EVNTSUBSCR_OBJ, &Data.Rec.ObjType);
						PPGetObjViewFiltMapping_Obj(Data.Rec.ObjType, &view_id, &filt_id);
						break;
					case PPEVENTTYPE_OUTER:
						break;
					case PPEVENTTYPE_SPCBILLCHANGE:
						PPGetObjViewFiltMapping_Obj(PPOBJ_BILL, &view_id, &filt_id);
						break;
					case PPEVENTTYPE_SYSJOURNAL:
						view_id = PPVIEW_SYSJOURNAL;
						filt_id = PPFILT_SYSJOURNAL;
						break;
					case PPEVENTTYPE_LOTEXPIRATION:
						PPGetObjViewFiltMapping_Obj(PPOBJ_LOT, &view_id, &filt_id);
						break;
				}
				if(filt_id && view_id) {
					if(Data.P_Filt && Data.P_Filt->GetSignature() != filt_id) {
					}
					else {
						PPBaseFilt * p_filt = 0;
						if(Data.P_Filt)
							p_filt = Data.P_Filt;
						else if(PPView::CreateFiltInstance(filt_id, &p_filt)) {
							;
						}
						if(p_filt) {
							PPView * p_view = 0;
							if(PPView::CreateInstance(view_id, &p_view)) {
								if(p_view->EditBaseFilt(p_filt) > 0) {
									if(!Data.P_Filt) {
										Data.P_Filt = p_filt;
										p_filt = 0;
									}
								}
								else {
									if(!Data.P_Filt)
										ZDELETE(p_filt);
								}
							}
							else {
								if(!Data.P_Filt)
									ZDELETE(p_filt);
							}
						}
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		void SetupType()
		{
			getCtrlData(CTLSEL_EVNTSUBSCR_TYPE, &Data.Rec.EventType);
			switch(Data.Rec.EventType) {
				case PPEVENTTYPE_OBJCREATED:
					disableCtrl(CTLSEL_EVNTSUBSCR_OBJ, 0);
					enableCommand(cmFilter, 1);
					break;
				case PPEVENTTYPE_OUTER:
					disableCtrl(CTLSEL_EVNTSUBSCR_OBJ, 1);
					enableCommand(cmFilter, 0);
					break;
				case PPEVENTTYPE_SPCBILLCHANGE:
					Data.Rec.ObjType = PPOBJ_BILL;
					setCtrlLong(CTLSEL_EVNTSUBSCR_OBJ, Data.Rec.ObjType);
					disableCtrl(CTLSEL_EVNTSUBSCR_OBJ, 1);
					enableCommand(cmFilter, 1);
					break;
				case PPEVENTTYPE_SYSJOURNAL:
					Data.Rec.ObjType = 0;
					setCtrlLong(CTLSEL_EVNTSUBSCR_OBJ, Data.Rec.ObjType);
					disableCtrl(CTLSEL_EVNTSUBSCR_OBJ, 1);
					enableCommand(cmFilter, 1);
					break;
				case PPEVENTTYPE_LOTEXPIRATION:
					Data.Rec.ObjType = 0;
					setCtrlLong(CTLSEL_EVNTSUBSCR_OBJ, Data.Rec.ObjType);
					disableCtrl(CTLSEL_EVNTSUBSCR_OBJ, 1);
					enableCommand(cmFilter, 1);
					break;
			}
		}
	};
	int    ok = -1;
	EventSubscriptionDialog * p_dlg = 0;
	if(pPack) {
		SString obj_title;
		THROW(CheckDialogPtr(&(p_dlg = new EventSubscriptionDialog())));
		THROW(EditPrereq(&pPack->Rec.ID, p_dlg, 0));
		p_dlg->setTitle(GetObjectTitle(Obj, obj_title));
		p_dlg->setDTS(pPack);
		while(ok < 0 && ExecView(p_dlg) == cmOK) {
			if(p_dlg->getDTS(pPack)) {
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

/*virtual*/int PPObjEventSubscription::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    is_new = 0;
	PPEventSubscriptionPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	{
		THROW(ok = EditDialog(&pack));
		if(ok > 0) {
			THROW(is_new || CheckRights(PPR_MOD));
			if(*pID)
				*pID = pack.Rec.ID;
			THROW(PutPacket(pID, &pack, 1));
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}

class EventSubscriptionCache : public ObjCache {
public:
	EventSubscriptionCache() : ObjCache(PPOBJ_EVENTSUBSCRIPTION, sizeof(EventSubscriptionData)) {}
private:
	struct EventSubscriptionData : public ObjCacheEntry {
		long   Flags;          //
		PPID   ObjType;        // Тип объекта, ассоциированный с событием (для некоторых типов событий)
		PPID   EventType;      //
		SColor NotifColor;
		long   MinDetectionInterval;
	};
	virtual int FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
	{
		int    ok = 1;
		EventSubscriptionData * p_cache_rec = static_cast<EventSubscriptionData *>(pEntry);
		PPObjEventSubscription _obj(0);
		PPEventSubscriptionPacket pack;
		if(_obj.GetPacket(id, &pack) > 0) {
			#define FLD(f) p_cache_rec->f = pack.Rec.f
			FLD(Flags);
			FLD(ObjType);
			FLD(EventType);
			FLD(NotifColor);
			FLD(MinDetectionInterval);
			#undef FLD
			MultTextBlock b;
			b.Add(pack.Rec.Name);
			b.Add(pack.Rec.Symb);
			{
				SString & r_temp_buf = SLS.AcquireRvlStr();
				pack.GetExtStrData(PPEventSubscriptionPacket::extssMessage, r_temp_buf);
				b.Add(r_temp_buf);
			}
			PutTextBlock(b, p_cache_rec);
			//
			{
				{
					long   user_id = 0;
					uint   pos = 0;
					while(IdToUserList.Search(id, &user_id, &pos)) {
						IdToUserList.atFree(pos);
					}
				}
				PPIDArray user_list;
				pack.UserList.Get(user_list);
				for(uint i = 0; i < user_list.getCount(); i++) {
					IdToUserList.Add(id, user_list.get(i));
				}
			}
		}
		else
			ok = -1;
		return ok;
	}
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
	{
		PPEventSubscriptionPacket * p_data_pack = static_cast<PPEventSubscriptionPacket *>(pDataRec);
		const EventSubscriptionData * p_cache_rec = static_cast<const EventSubscriptionData *>(pEntry);
		p_data_pack->Z();
		p_data_pack->Rec.Tag = PPOBJ_EVENTSUBSCRIPTION;
		#define FLD(f) p_data_pack->Rec.f = p_cache_rec->f
		FLD(ID);
		FLD(Flags);
		FLD(ObjType);
		FLD(EventType);
		FLD(NotifColor);
		FLD(MinDetectionInterval);
		#undef FLD
		MultTextBlock b(this, pEntry);
		b.Get(p_data_pack->Rec.Name, sizeof(p_data_pack->Rec.Name));
		b.Get(p_data_pack->Rec.Symb, sizeof(p_data_pack->Rec.Symb));
		{
			SString & r_temp_buf = SLS.AcquireRvlStr();
			b.Get(r_temp_buf);
			p_data_pack->PutExtStrData(PPEventSubscriptionPacket::extssMessage, r_temp_buf);
		}
		{
			PPIDArray user_list;
			IdToUserList.GetListByKey(p_cache_rec->ID, user_list);
			p_data_pack->UserList.Set(&user_list);
		}
	}
	LAssocArray IdToUserList; // Список ассоциаций EventSubscriptionID-->UserID
};

IMPL_OBJ_FETCH(PPObjEventSubscription, PPEventSubscriptionPacket, EventSubscriptionCache);
//
//
//
PPEventCore::Packet::Packet() : ID(0), Dtm(ZERODATETIME), EventType(0), Status(0), UserID(0), GlobalUserID(0), Flags(0), EvSubscrID(0)
{
	Oid.Z();
}

PPEventCore::Packet & PPEventCore::Packet::Z()
{
	ID = 0;
	Dtm.Z();
	EventType = 0;
	Status = 0;
	UserID = 0;
	GlobalUserID = 0;
	Oid.Z();
	Flags = 0;
	EvSubscrID = 0;
	Text.Z();
	ExtData.Z();
	return *this;
}

PPEventCore::PPEventCore() : EventTbl()
{
}

void PPEventCore::PacketToRec(const Packet & rPack, EventTbl::Rec & rRec) const
{
#define F(f) rRec.f = rPack.f
	F(ID);
	F(EventType);
	F(Status);
	F(UserID);
	F(GlobalUserID);
	F(Flags);
	F(EvSubscrID);
#undef F
	rRec.Dt = rPack.Dtm.d;
	rRec.Tm = rPack.Dtm.t;
	rRec.ObjType = rPack.Oid.Obj;
	rRec.ObjID = rPack.Oid.Id;
}

void PPEventCore::RecToPacket(const EventTbl::Rec & rRec, Packet & rPack) const
{
#define F(f) rPack.f = rRec.f
	F(ID);
	F(EventType);
	F(Status);
	F(UserID);
	F(GlobalUserID);
	F(Flags);
	F(EvSubscrID);
#undef F
	rPack.Dtm.d = rRec.Dt;
	rPack.Dtm.t = rRec.Tm;
	rPack.Oid.Obj = rRec.ObjType;
	rPack.Oid.Id = rRec.ObjID;
}

int PPEventCore::Put(PPID * pID, const Packet * pPack, int use_ta)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	EventTbl::Rec rec;
	SString temp_buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!pID || !*pID) {
			if(pPack) {
				PPID   id = 0;
				PacketToRec(*pPack, rec);
				rec.ID = 0;
				copyBufFrom(&rec);
				if(pPack->ExtData.GetAvailableSize()) {
					THROW(writeLobData(VT, pPack->ExtData.constptr(), pPack->ExtData.GetAvailableSize()));
				}
				THROW_DB(insertRec(0, &id));
				destroyLobData(VT);
				{
					(temp_buf = pPack->Text).Strip().Transf(CTRANSF_INNER_TO_UTF8);
					THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EVENT, id, PPTRPROP_DESCR), temp_buf, 0));
				}
				ASSIGN_PTR(pID, id);
			}
		}
		else if(pID) {
			if(*pID) {
				if(pPack) {
					assert(pPack->ID == *pID);
					PacketToRec(*pPack, rec);
					copyBufFrom(&rec);
					if(pPack->ExtData.GetAvailableSize()) {
						THROW(writeLobData(VT, pPack->ExtData.constptr(), pPack->ExtData.GetAvailableSize()));
					}
					THROW_DB(updateRec());
					destroyLobData(VT);
					{
						(temp_buf = pPack->Text).Strip().Transf(CTRANSF_INNER_TO_UTF8);
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EVENT, *pID, PPTRPROP_DESCR), temp_buf, 0));
					}
				}
				else {
					if(SearchByID_ForUpdate(this, PPOBJ_EVENT, *pID, 0) > 0) {
						THROW_DB(deleteRec());
						THROW(p_ref->UtrC.SetText(TextRefIdent(PPOBJ_EVENT, *pID, PPTRPROP_DESCR), temp_buf.Z(), 0));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPEventCore::Get(PPID id, Packet * pPack)
{
	int    ok = -1;
	if(SearchByID(this, PPOBJ_EVENT, id, 0) > 0) {
		if(pPack) {
			Reference * p_ref = PPRef;
			RecToPacket(data, *pPack);
			readLobData(this->VT, pPack->ExtData);
			destroyLobData(this->VT);
			p_ref->UtrC.GetText(TextRefIdent(PPOBJ_EVENT, id, PPTRPROP_DESCR), pPack->Text);
			pPack->Text.Transf(CTRANSF_UTF8_TO_INNER);
		}
		ok = 1;
	}
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(Event); EventFilt::EventFilt() : PPBaseFilt(PPFILT_EVENT, 0, 0)
{
	SetFlatChunk(offsetof(EventFilt, ReserveStart),
		offsetof(EventFilt, Reserve)-offsetof(EventFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

PPViewEvent::PPViewEvent() : 
	PPView(0, &Filt, PPVIEW_EVENT, (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter), 0), P_DsList(0), 
	P_ObjColl(new ObjCollection), EsObj(0)
{
}

PPViewEvent::~PPViewEvent()
{
	ZDELETE(P_DsList);
	ZDELETE(P_ObjColl);
}

int PPViewEvent::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	ObjNameList.clear();
	SubscrList.Set(0);
	if(Filt.UserID) {
		PPObjEventSubscription es_obj(0);
		PPEventSubscription es_rec;
		PPEventSubscriptionPacket es_pack;
		PPIDArray es_list;
		for(SEnum en = es_obj.Enum(0); en.Next(&es_rec) > 0;) {
			if(es_obj.Fetch(es_rec.ID, &es_pack) > 0 && es_pack.UserList.Search(Filt.UserID, 0, 0))
				es_list.add(es_rec.ID);
		}
		SubscrList.Set(&es_list);
	}
	CATCHZOK
	return ok;
}

class EventFiltDialog : public TDialog {
	DECL_DIALOG_DATA(EventFilt);
public:
	EventFiltDialog() : TDialog(DLG_EVNTFILT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_EVNTFILT_PERIOD, &Data.Period);
		SetupEventTypeCombo(this, CTLSEL_EVNTFILT_TYPE, Data.EventType);
		SetupPPObjCombo(this, CTLSEL_EVNTFILT_SUBSCR, PPOBJ_EVENTSUBSCRIPTION, Data.EventSubscrID, 0);
		SetupPPObjCombo(this, CTLSEL_EVNTFILT_USER, PPOBJ_USR, Data.UserID, 0);
		AddClusterAssoc(CTL_EVNTFILT_STATUS, 0, (1 << PPEventCore::statusActual));
		AddClusterAssoc(CTL_EVNTFILT_STATUS, 1, (1 << PPEventCore::statusViewed));
		AddClusterAssoc(CTL_EVNTFILT_STATUS, 2, (1 << PPEventCore::statusArchived));
		SetClusterData(CTL_EVNTFILT_STATUS, Data.StatusFlags);
		//
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GetPeriodInput(this, CTL_EVNTFILT_PERIOD, &Data.Period);
		getCtrlData(CTLSEL_EVNTFILT_TYPE, &Data.EventType);
		getCtrlData(CTLSEL_EVNTFILT_SUBSCR, &Data.EventSubscrID);
		getCtrlData(CTLSEL_EVNTFILT_USER, &Data.UserID);
		GetClusterData(CTL_EVNTFILT_STATUS, &Data.StatusFlags);
		//
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int PPViewEvent::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	EventFilt * p_filt = static_cast<EventFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(EventFiltDialog, p_filt);
}

int PPViewEvent::CheckForFilt(const EventFilt * pFilt, EventTbl::Rec * pRec)
{
	int    ok = 0;
	if(pFilt) {
		if(pRec) {
			if(!CheckFiltID(pFilt->EventType, pRec->EventType))
				ok = 0;
			else if(pFilt->StatusFlags && !(pFilt->StatusFlags & (1 << pRec->Status)))
				ok = 0;
			/*else if(!CheckFiltID(pFilt->UserID, pRec->UserID))
				ok = 0;
			else if(!CheckFiltID(pFilt->GlobalUserID, pRec->GlobalUserID))
				ok = 0;*/
			else if(!CheckFiltID(pFilt->ObjType, pRec->ObjType))
				ok = 0;
			else if(!CheckFiltID(pFilt->EventSubscrID, pRec->EvSubscrID))
				ok = 0;
			else if(!pFilt->Period.CheckDate(pRec->Dt))
				ok = 0;
			else if(!SubscrList.CheckID(pRec->EvSubscrID))
				ok = 0;
			else
				ok = 1;
		}
		else
			ok = 0;
	}
	else
		ok = 1;
	return ok;
}

int PPViewEvent::InitIteration()
{
	Counter.Init();
	return 1;
}

int FASTCALL PPViewEvent::NextIteration(EventViewItem * pItem)
{
	if(static_cast<ulong>(Counter) < SVector::GetCount(P_DsList)) {
		if(pItem) {
			const BrwItem * p_inner_item = static_cast<const BrwItem *>(P_DsList->at(static_cast<ulong>(Counter)));
			*pItem = *p_inner_item;
		}
		Counter.Increment();
		return 1;
	}
	else
		return 0;
}

int PPViewEvent::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	PPTimeSeries item;
	SString temp_buf;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount());
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	StrPool.ClearS();
	ObjNameList.clear();
	{
		PPEventCore::Packet pack;
		union {
			EventTbl::Key1 k1; // tm
			EventTbl::Key2 k2; // type, tm
			EventTbl::Key3 k3; // user, tm
			EventTbl::Key4 k4; // globaluser, tm
			EventTbl::Key5 k5; // objtype, tm
		} k;
		int    idx = 1;
		MEMSZERO(k);
		DBQ * dbq = 0;//&daterange(T.Dt, &Filt.Period);
		if(Filt.EventType) {
			idx = 2;
			k.k2.EventType = Filt.EventType;
			k.k2.Dt = Filt.Period.low;
		}
		/*else if(Filt.UserID) {
			idx = 3;
			k.k3.UserID = Filt.UserID;
			k.k3.Dt = Filt.Period.low;
		}
		else if(Filt.GlobalUserID) {
			idx = 4;
			k.k4.GlobalUserID = Filt.GlobalUserID;
			k.k4.Dt = Filt.Period.low;
		}*/
		else if(Filt.ObjType) {
			idx = 5;
			k.k5.ObjType = Filt.ObjType;
			k.k5.Dt = Filt.Period.low;
		}
		else {
			idx = 1;
			k.k1.Dt = Filt.Period.low;
		}
		BExtQuery q(&T, idx);
		dbq = ppcheckfiltid(dbq, T.EventType, Filt.EventType);
		//dbq = ppcheckfiltid(dbq, T.UserID, Filt.UserID);
		//dbq = ppcheckfiltid(dbq, T.GlobalUserID, Filt.GlobalUserID);
		dbq = ppcheckfiltid(dbq, T.EvSubscrID, Filt.EventSubscrID);
		dbq = ppcheckfiltid(dbq, T.ObjType, Filt.ObjType);
		dbq = & (*dbq && daterange(T.Dt, &Filt.Period));
		q.select(T.ID, T.Dt, T.Tm, T.EventType, T.Status, T.UserID, T.GlobalUserID, T.EvSubscrID, T.ObjType, T.ObjID, T.Flags, 0).where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(CheckForFilt(&Filt, &T.data)) {
				if(T.Get(T.data.ID, &pack)) {
					BrwItem new_item;
					MEMSZERO(new_item);
					new_item.ID = pack.ID;
					new_item.Dtm = pack.Dtm;
					new_item.EventType = pack.EventType;
					new_item.Status = pack.Status;
					new_item.UserID = pack.UserID;
					new_item.GlobalUserID = pack.GlobalUserID;
					new_item.EventSubscrID = pack.EvSubscrID; // @v10.9.1
					new_item.Oid = pack.Oid;
					new_item.Flags = pack.Flags;
					StrPool.AddS(pack.Text, &new_item.TextP);
					if(new_item.Oid.Obj && new_item.Oid.Id) {
						PPObjNamePEntry objn_entry(new_item.Oid.Obj, new_item.Oid.Id);
						objn_entry.NameP = 0;
						uint   objn_pos = 0;
						if(!ObjNameList.bsearch(&objn_entry, &objn_pos, PTR_CMPFUNC(PPObjID))) {
							char   name_buf[256];
							PPObject * ppobj = P_ObjColl->GetObjectPtr(objn_entry.Obj);
							if(ppobj && ppobj->GetName(objn_entry.Id, name_buf, sizeof(name_buf)) > 0) {
								temp_buf = name_buf;
								StrPool.AddS(temp_buf, &objn_entry.NameP);
								ObjNameList.ordInsert(&objn_entry, 0, PTR_CMPFUNC(PPObjID));
							}
						}
						else
							objn_entry = ObjNameList.at(objn_pos);
						new_item.ObjNameP = objn_entry.NameP;
					}
					THROW_SL(P_DsList->insert(&new_item));
				}
			}
		}
	}
	ObjNameList.sort(PTR_CMPFUNC(PPObjID));
	if(pBrw) {
		pBrw->Helper_SetAllColumnsSortable();
		if(is_sorting_needed) {
			//P_DsList->sort(PTR_CMPFUNC(PPViewTimeSeriesBrwItem), pBrw);
		}
	}
	CATCHZOK
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewEvent * p_view = static_cast<PPViewEvent *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

/*static*/int PPViewEvent::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewEvent::BrwItem * p_hdr = static_cast<const PPViewEvent::BrwItem *>(pData);
			if(r_col.OrgOffs == 3) { // Status
				switch(p_hdr->Status) {
					case PPEventCore::statusActual:   pStyle->Color = GetColorRef(SClrLightgreen); break;
					case PPEventCore::statusViewed:   pStyle->Color = GetColorRef(SClrOrange); break;
					case PPEventCore::statusArchived: pStyle->Color = GetColorRef(SClrSnow); break;
					default: pStyle->Color = GetColorRef(SClrGrey); break;
				}
				ok = 1;
			}
			else if(r_col.OrgOffs == 6) { // Subscription
				if(p_hdr->EventSubscrID) {
					PPObjEventSubscription es_obj(0);
					PPEventSubscriptionPacket es_pack;
					if(es_obj.Fetch(p_hdr->EventSubscrID, &es_pack) && !es_pack.Rec.NotifColor.IsEmpty())
						ok = pStyle->SetRightFigCircleColor(static_cast<COLORREF>(es_pack.Rec.NotifColor));
				}
			}
		}
	}
	return ok;
}

/*virtual*/int PPViewEvent::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1;
	LDATETIME last_dtm;
	if(pEv) {
		if(pEv->IsFinish() && kind == PPAdviseBlock::evSysJournalChanged) {
			last_dtm = pEv->ExtDtm;
			ok = ProcessCommand(PPVCMD_REFRESH, 0, pBrw);
			if(pBrw && ok > 0)
				pBrw->Update();
		}
	}
	return ok;
}

void PPViewEvent::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewEvent::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
	pBrw->Advise(PPAdviseBlock::evSysJournalChanged, PPACN_EVENTDETECTION, PPOBJ_EVENTSUBSCRIPTION, 0);
}

int PPViewEvent::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int PPViewEvent::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const  BrwItem * p_item = static_cast<const BrwItem *>(pBlk->P_SrcData);
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->ID); break; // @id
			case 1: // @time
				pBlk->TempBuf.Z().Cat(p_item->Dtm, DATF_DMY, TIMF_HMS);
				pBlk->Set(pBlk->TempBuf);
				break; 
			case 2: // @type
				{
					const char * p_tsign = 0;
					switch(p_item->EventType) {
						case PPEVENTTYPE_OBJCREATED: p_tsign = "eventtype_objcreated"; break;
						case PPEVENTTYPE_OUTER: p_tsign = "eventtype_outer"; break;
						case PPEVENTTYPE_SPCBILLCHANGE: p_tsign = "eventtype_spcbillchange"; break;
						case PPEVENTTYPE_SYSJOURNAL: p_tsign = "eventtype_sysjournal"; break;
						case PPEVENTTYPE_LOTEXPIRATION: p_tsign = "eventtype_lotexpiration"; break;
					}
					if(p_tsign)
						PPLoadString(p_tsign, pBlk->TempBuf);
					else
						pBlk->TempBuf.Z();
					pBlk->Set(pBlk->TempBuf);
				}
				break; 
			case 3: // status
				pBlk->TempBuf.Z();
				pBlk->Set(pBlk->TempBuf);
				break; 
			case 4: // object
				StrPool.GetS(p_item->ObjNameP, pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf);
				break; 
			case 5: // text
				StrPool.GetS(p_item->TextP, pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf); 
				break; 
			case 6: // subscription
				{
					PPEventSubscriptionPacket es_pack;
					if(EsObj.Fetch(p_item->EventSubscrID, &es_pack) > 0)
						pBlk->TempBuf = es_pack.Rec.Name;
					else
						pBlk->TempBuf.Z();
					pBlk->Set(pBlk->TempBuf);
				}
				break;
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewEvent::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewEvent * p_v = static_cast<PPViewEvent *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

SArray * PPViewEvent::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_EVENT;
	SArray * p_array = 0;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int PPViewEvent::EditObj(const PPObjID * pObjID)
{
	return pObjID ? EditPPObj(pObjID->Obj, pObjID->Id) : -1;
}

int PPViewEvent::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	BrwItem item;
	if(!RVALUEPTR(item, static_cast<const BrwItem *>(pHdr)))
		MEMSZERO(item);
	if(item.Oid.Obj && item.Oid.Id) {
		EditObj(&item.Oid);
	}
	return -1;
}

int PPViewEvent::SelectStatus(PPID evID)
{
	int    ok = -1;
	TDialog * dlg = 0;
	PPEventCore::Packet pack;
	if(evID && T.Get(evID, &pack) > 0) {
		const long org_status = pack.Status;
		SString temp_buf;
		dlg = new TDialog(DLG_SELEVNTSTATUS);
		THROW(CheckDialogPtr(&dlg));
		dlg->AddClusterAssocDef(CTL_SELEVNTSTATUS_ST, 0, PPEventCore::statusUndef);
		dlg->AddClusterAssoc(CTL_SELEVNTSTATUS_ST, 1, PPEventCore::statusActual);
		dlg->AddClusterAssoc(CTL_SELEVNTSTATUS_ST, 2, PPEventCore::statusViewed);
		dlg->AddClusterAssoc(CTL_SELEVNTSTATUS_ST, 3, PPEventCore::statusArchived);
		dlg->SetClusterData(CTL_SELEVNTSTATUS_ST, pack.Status);
		if(ExecView(dlg) == cmOK) {
			dlg->GetClusterData(CTL_SELEVNTSTATUS_ST, &pack.Status);
			if(pack.Status != org_status && oneof3(pack.Status, PPEventCore::statusActual, PPEventCore::statusViewed, PPEventCore::statusArchived)) {
				THROW(T.Put(&evID, &pack, 1));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewEvent::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int   ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		if(ppvCmd == PPVCMD_CHANGESTATUS) {
			if(pHdr) {
				const BrwItem * p_item = static_cast<const BrwItem *>(pHdr);
				if(SelectStatus(p_item->ID) > 0) {
					AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
					if(p_def && MakeList(pBrw)) {
						SArray * p_array = new SArray(*P_DsList);
						p_def->setArray(p_array, 0, 0);
						ok = 1;
					}
				}
			}
		}
		else if(ppvCmd == PPVCMD_REFRESH) {
			AryBrowserDef * p_def = pBrw ? static_cast<AryBrowserDef *>(pBrw->getDef()) : 0;
			if(p_def && MakeList(pBrw)) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 0);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPObjEventSubscription::Run()
{
	int    ok = -1;
	TSCollection <PPEventCore::Packet> evp_list;
	PPEventSubscription evs_rec;
	PPIDArray id_list;
	for(SEnum en = Enum(0); en.Next(&evs_rec) > 0;) {
		id_list.add(evs_rec.ID);
	}
	for(uint i = 0; i < id_list.getCount(); i++) {
		const PPID evs_id = id_list.get(i);
		int dr = Detect(evs_id, evp_list);
		/*if(dr > 0) {
			detected_id_list.add(evs_id);
		}*/
	}
	if(evp_list.getCount()) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		PPEventCore evc;
		LAssocArray detected_id_list;
		PPTransaction tra(1);
		THROW(tra);
		for(uint j = 0; j < evp_list.getCount(); j++) {
			PPID ev_id = 0;
			const PPEventCore::Packet * p_pack = evp_list.at(j);
			if(p_pack) {
				if(evc.Put(&ev_id, p_pack, 0)) {
					detected_id_list.Add(p_pack->EvSubscrID, ev_id);
				}
				else
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
			}
		}
		if(p_sj) {
			for(uint dlidx = 0; dlidx < detected_id_list.getCount(); dlidx++) {
				const LAssoc & r_item = detected_id_list.at(dlidx);
				THROW(DS.LogAction(PPACN_EVENTDETECTION, PPOBJ_EVENTSUBSCRIPTION, r_item.Key, r_item.Val, 0));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjEventSubscription::Detect(PPID id, TSCollection <PPEventCore::Packet> & rEvpList)
{
	int    ok = -1;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	if(p_sj) {
		PPEventSubscriptionPacket pack;
		if(GetPacket(id, &pack) > 0) {
			LDATETIME last_det_dtm = ZERODATETIME;
			LDATETIME new_last_det_dtm = ZERODATETIME;
			SysJournalTbl::Rec sj_rec;
			PPIDArray acn_list;
			acn_list.add(PPACN_EVENTDETECTION);
			if(p_sj->GetLastObjEvent(Obj, id, &acn_list, &last_det_dtm, &sj_rec) > 0) {
				;
			}
			else {
				int    creation = 0;
				if(p_sj->GetLastObjModifEvent(Obj, id, &last_det_dtm, &creation, &sj_rec) > 0) {
				}
			}
			if(!last_det_dtm)
				last_det_dtm.Set(getcurdate_(), ZEROTIME);
			//
			if(pack.Rec.EventType == PPEVENTTYPE_OBJCREATED) {
				PPIDArray acn_list;
				if(pack.Rec.ObjType == PPOBJ_BILL)
					acn_list.add(PPACN_TURNBILL);
				else
					acn_list.add(PPACN_OBJADD);
				{
					SysJournalTbl::Key0 k;
					k.Dt = last_det_dtm.d;
					k.Tm = last_det_dtm.t;
					BExtQuery q(p_sj, 0, 128);
					DBQ * dbq = &(p_sj->Dt >= last_det_dtm.d && p_sj->ObjType == pack.Rec.ObjType);
					q.select(p_sj->Dt, p_sj->Tm, p_sj->ObjType, p_sj->ObjID, p_sj->Action, 0L).where(*dbq);
					for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
						SysJournalTbl::Rec sj_rec;
						p_sj->copyBufTo(&sj_rec);
						if(cmp(last_det_dtm, sj_rec.Dt, sj_rec.Tm) < 0 && acn_list.lsearch(sj_rec.Action)) {
							int    skip = 0;
							if(pack.P_Filt) {
								skip = 1;
								int   view_id = 0;
								int   filt_id = 0;
								if(PPGetObjViewFiltMapping_Obj(sj_rec.ObjType, &view_id, &filt_id) && pack.P_Filt->GetSignature() == filt_id) {
									switch(sj_rec.ObjType) {
										case PPOBJ_BILL:
											{
												PPViewBill view;
												static_cast<BillFilt *>(pack.P_Filt)->Flags |= BillFilt::fNoTempTable;
												if(view.Init_(pack.P_Filt) && view.CheckIDForFilt(sj_rec.ObjID, 0))
													skip = 0;
											}
											break;
										case PPOBJ_PRJTASK:
											{
												PPViewPrjTask view;
												static_cast<PrjTaskFilt *>(pack.P_Filt)->Flags |= (PrjTaskFilt::fNoTempTable|PrjTaskFilt::fNotShowPPWaitOnInit);
												if(view.Init_(pack.P_Filt) && view.CheckIDForFilt(sj_rec.ObjID, 0))
													skip = 0;
											}
											break;
										case PPOBJ_PERSON:
											{
												PPViewPerson view;
												if(view.Init_(pack.P_Filt) && view.CheckIDForFilt(sj_rec.ObjID, 0))
													skip = 0;
											}
											break;
										case PPOBJ_GOODS:
											{
												PPObjGoods obj;
												if(obj.CheckForFilt(static_cast<GoodsFilt *>(pack.P_Filt), sj_rec.ObjID, 0))
													skip = 0;
											}
											break;
									}
								}
							}
							if(!skip) {
								PPEventCore::Packet * p_evp = rEvpList.CreateNewItem();
								THROW_SL(p_evp);
								p_evp->Dtm.Set(sj_rec.Dt, sj_rec.Tm);
								p_evp->EvSubscrID = pack.Rec.ID;
								p_evp->Oid.Set(sj_rec.ObjType, sj_rec.ObjID);
								p_evp->EventType = pack.Rec.EventType;
								p_evp->Status = PPEventCore::statusActual;
								pack.GetExtStrData(pack.extssMessage, p_evp->Text);
								new_last_det_dtm = p_evp->Dtm;
								ok = 1;
							}
						}
					}
				}
			}
			else if(pack.Rec.EventType == PPEVENTTYPE_SPCBILLCHANGE) {
			}
			else if(pack.Rec.EventType == PPEVENTTYPE_SYSJOURNAL) {
				if(pack.P_Filt && pack.P_Filt->GetSignature() == PPFILT_SYSJOURNAL) {
					SysJournalFilt temp_filt;
					if(temp_filt.Copy(pack.P_Filt, 0)) {
						temp_filt.Period.low = last_det_dtm.d;
						temp_filt.Period.upp = ZERODATE;
						temp_filt.BegTm = last_det_dtm.t;
						temp_filt.Flags &= ~(SysJournalFilt::fShowHistoryObj|SysJournalFilt::fShowObjects);
						temp_filt.Sgsj = sgsjNone;
						temp_filt.Sgd = sgdNone;
						PPViewSysJournal sj_view;
						if(sj_view.Init_(&temp_filt)) {
							SysJournalViewItem view_item;
							for(sj_view.InitIteration(); sj_view.NextIteration(&view_item) > 0;) {
								PPEventCore::Packet * p_evp = rEvpList.CreateNewItem();
								THROW_SL(p_evp);
								p_evp->Dtm.Set(p_sj->data.Dt, p_sj->data.Tm);
								p_evp->EvSubscrID = pack.Rec.ID;
								p_evp->Oid.Set(p_sj->data.ObjType, p_sj->data.ObjID);
								p_evp->EventType = pack.Rec.EventType;
								p_evp->Status = PPEventCore::statusActual;
								pack.GetExtStrData(pack.extssMessage, p_evp->Text);
								new_last_det_dtm = p_evp->Dtm;
								ok = 1;
							}
						}
					}
				}
			}
			else if(pack.Rec.EventType == PPEVENTTYPE_LOTEXPIRATION) {
			}
			else if(pack.Rec.EventType == PPEVENTTYPE_OUTER) {
			}
		}
	}
	CATCHZOK
	return ok;
}