// OBJEVENT.CPP
// Copyright (c) A.Sobolev 2020
//
#include <pp.h>
#pragma hdrstop

struct PPEventSubscription {
	SLAPI  PPEventSubscription();
	int    FASTCALL IsEqual(const PPEventSubscription & rS) const;
	long   Tag;            // Const=PPOBJ_EVENTSUBSCRIPTION
	long   ID;             // @id
	char   Name[48];       // @name @!refname
	char   Symb[20];       // 
	long   Flags;          //
	char   Reserve[60];    // @reserve
	PPID   EventType;      //
	PPID   UserID;         // ->Ref(PPOBJ_USER)
};

class PPEventSubscriptionPacket {
public:
	SLAPI  PPEventSubscriptionPacket();
	SLAPI ~PPEventSubscriptionPacket();
	int    FASTCALL IsEqual(const PPEventSubscriptionPacket & rS) const;
	PPEventSubscription Rec;
	struct FlatBlock {
		SLAPI  FlatBlock();
		int    SLAPI IsEmpty() const;
		int    FASTCALL IsEqual(const FlatBlock & rS) const;
		uint   Reserve[64];
	} Fb;
	PPBaseFilt * P_Filt;
};

class PPObjEventSuscription : public PPObjReference { // PPOBJ_EVENTSUBSCRIPTION
public:
	SLAPI  PPObjEventSuscription();
	virtual int  SLAPI Edit(PPID * pID, void * extraPtr);
	int    SLAPI PutPacket(PPID * pID, PPEventSubscriptionPacket * pPack, int use_ta);
	int    SLAPI GetPacket(PPID id, PPEventSubscriptionPacket * pPack);
};


SLAPI PPEventSubscription::PPEventSubscription() : Tag(PPOBJ_EVENTSUBSCRIPTION), ID(0), Flags(0), EventType(0), UserID(0)
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
	else if(UserID != rS.UserID)
		eq = 0;
	return eq;
}

SLAPI PPEventSubscriptionPacket::FlatBlock::FlatBlock()
{
	MEMSZERO(Reserve);
}

int SLAPI PPEventSubscriptionPacket::FlatBlock::IsEmpty() const
{
	return 1;
}

int FASTCALL PPEventSubscriptionPacket::FlatBlock::IsEqual(const PPEventSubscriptionPacket::FlatBlock & rS) const
{
	int    eq = 1;
	return eq;
}

SLAPI PPEventSubscriptionPacket::PPEventSubscriptionPacket() : P_Filt(0)
{
	MEMSZERO(Fb);
}

SLAPI PPEventSubscriptionPacket::~PPEventSubscriptionPacket()
{
	ZDELETE(P_Filt);
}

int FASTCALL PPEventSubscriptionPacket::IsEqual(const PPEventSubscriptionPacket & rS) const
{
	int    eq = 1;
	if(!Rec.IsEqual(rS.Rec))
		eq = 0;
	else if(!Fb.IsEqual(rS.Fb))
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

SLAPI PPObjEventSuscription::PPObjEventSuscription() : PPObjReference(PPOBJ_EVENTSUBSCRIPTION, 0)
{
}

int SLAPI PPObjEventSuscription::PutPacket(PPID * pID, PPEventSubscriptionPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPID   _id = DEREFPTRORZ(pID);
	const  int is_new = (_id == 0);
	const  int is_removing = BIN(*pID != 0 && pPack == 0);
	PPID   hid = 0;
	SBuffer ext_buf;
	PPEventSubscriptionPacket org_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(_id) {
			THROW(GetPacket(_id, &org_pack) > 0);
		}
		if(pPack == 0) {
			if(*pID) {
				TextRefIdent tri(Obj, _id, PPTRPROP_TIMESERIES);
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
					THROW(ref->UpdateItem(Obj, _id, &pPack->Rec, 1, 0));
					DS.LogAction(PPACN_OBJUPD, Obj, _id, 0, 0);
				}
			}
			else {
				THROW(ref->AddItem(Obj, &_id, &pPack->Rec, 0));
				pPack->Rec.ID = _id;
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

int SLAPI PPObjEventSuscription::GetPacket(PPID id, PPEventSubscriptionPacket * pPack)
{
	int    ok = -1;
	return ok;
}

/*virtual*/int SLAPI PPObjEventSuscription::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1;
	return ok;
}
//
//
//
SLAPI PPEventCore::Packet::Packet() : ID(0), Dtm(ZERODATETIME), TypeID(0), Status(0), UserID(0), GlobalUserID(0), Flags(0)
{
	Oid.Z();
}

PPEventCore::Packet & PPEventCore::Packet::Z()
{
	ID = 0;
	Dtm.Z();
	TypeID = 0;
	Status = 0;
	UserID = 0;
	GlobalUserID = 0;
	Oid.Z();
	Flags = 0;
	Text.Z();
	ExtData.Z();
	return *this;
}

SLAPI PPEventCore::PPEventCore() : EventTbl()
{
}

void SLAPI PPEventCore::PacketToRec(const Packet & rPack, EventTbl::Rec & rRec) const
{
#define F(f) rRec.f = rPack.f
	F(ID);
	F(TypeID);
	F(Status);
	F(UserID);
	F(GlobalUserID);
	F(Flags);
#undef F
	rRec.Dt = rPack.Dtm.d;
	rRec.Tm = rPack.Dtm.t;
	rRec.ObjType = rPack.Oid.Obj;
	rRec.ObjID = rPack.Oid.Id;
}

void SLAPI PPEventCore::RecToPacket(const EventTbl::Rec & rRec, Packet & rPack) const
{
#define F(f) rPack.f = rRec.f
	F(ID);
	F(TypeID);
	F(Status);
	F(UserID);
	F(GlobalUserID);
	F(Flags);
#undef F
	rPack.Dtm.d = rRec.Dt;
	rPack.Dtm.t = rRec.Tm;
	rPack.Oid.Obj = rRec.ObjType;
	rPack.Oid.Id = rRec.ObjID;
}

int SLAPI PPEventCore::Put(PPID * pID, const Packet * pPack, int use_ta)
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
				copyBufTo(&rec);
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
					copyBufTo(&rec);
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

int SLAPI PPEventCore::Get(PPID id, Packet * pPack)
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

IMPLEMENT_PPFILT_FACTORY(Event); SLAPI EventFilt::EventFilt() : PPBaseFilt(PPFILT_EVENT, 0, 0)
{
	SetFlatChunk(offsetof(EventFilt, ReserveStart),
		offsetof(EventFilt, Reserve)-offsetof(EventFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

SLAPI PPViewEvent::PPViewEvent() : PPView(0, &Filt, PPVIEW_EVENT), P_DsList(0)
{
	ImplementFlags |= (implBrowseArray|implOnAddSetupPos|implDontEditNullFilter);
}

SLAPI PPViewEvent::~PPViewEvent()
{
	ZDELETE(P_DsList);
}

int SLAPI PPViewEvent::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	CATCHZOK
	return ok;
}

int SLAPI PPViewEvent::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	return ok;
}

int SLAPI PPViewEvent::CheckForFilt(const EventFilt * pFilt, EventTbl::Rec * pRec)
{
	int    ok = 0;
	if(pFilt) {
		if(pRec) {
			if(!CheckFiltID(pFilt->TypeID, pRec->TypeID))
				ok = 0;
			else if(!CheckFiltID(pFilt->UserID, pRec->UserID))
				ok = 0;
			else if(!CheckFiltID(pFilt->GlobalUserID, pRec->GlobalUserID))
				ok = 0;
			else if(!CheckFiltID(pFilt->ObjType, pRec->ObjType))
				ok = 0;
			else if(!pFilt->Period.CheckDate(pRec->Dt))
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

int SLAPI PPViewEvent::InitIteration()
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

int SLAPI PPViewEvent::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	PPTimeSeries item;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount());
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new SArray(sizeof(BrwItem));
	StrPool.ClearS();
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
		if(Filt.TypeID) {
			idx = 2;
			k.k2.TypeID = Filt.TypeID;
			k.k2.Dt = Filt.Period.low;
		}
		else if(Filt.UserID) {
			idx = 3;
			k.k3.UserID = Filt.UserID;
			k.k3.Dt = Filt.Period.low;
		}
		else if(Filt.GlobalUserID) {
			idx = 4;
			k.k4.GlobalUserID = Filt.GlobalUserID;
			k.k4.Dt = Filt.Period.low;
		}
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
		dbq = ppcheckfiltid(dbq, T.TypeID, Filt.TypeID);
		dbq = ppcheckfiltid(dbq, T.UserID, Filt.UserID);
		dbq = ppcheckfiltid(dbq, T.GlobalUserID, Filt.GlobalUserID);
		dbq = ppcheckfiltid(dbq, T.ObjType, Filt.ObjType);
		dbq = & (*dbq && daterange(T.Dt, &Filt.Period));
		q.select(T.ID, T.Dt, T.Tm, T.TypeID, T.Status, T.UserID, T.GlobalUserID, T.ObjType, T.ObjID, T.Flags, 0).where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(CheckForFilt(&Filt, &T.data)) {
				if(T.Get(T.data.ID, &pack)) {
					BrwItem new_item;
					MEMSZERO(new_item);
					new_item.ID = pack.ID;
					new_item.Dtm = pack.Dtm;
					new_item.TypeID = pack.TypeID;
					new_item.Status = pack.Status;
					new_item.UserID = pack.UserID;
					new_item.GlobalUserID = pack.GlobalUserID;
					new_item.Oid = pack.Oid;
					new_item.Flags = pack.Flags;
					StrPool.AddS(pack.Text, &new_item.TextP);
					THROW_SL(P_DsList->insert(&new_item));
				}
			}
		}
	}
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

/*static*/int SLAPI PPViewEvent::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			const PPViewEvent::BrwItem * p_hdr = static_cast<const PPViewEvent::BrwItem *>(pData);
			/*if(r_col.OrgOffs == 5) { // OprKind
				if(p_hdr->Flags & PSNEVF_FORCEPAIR) {
					pStyle->Color2 = GetColorRef(SClrViolet);
					pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
					ok = 1;
				}
			}*/
		}
	}
	return ok;
}


void SLAPI PPViewEvent::PreprocessBrowser(PPViewBrowser * pBrw)
{
}

int SLAPI PPViewEvent::OnExecBrowser(PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewEvent::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
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
				pBlk->SetZero(); 
				break; 
			case 3: // status
				pBlk->SetZero(); 
				break; 
			case 4: // object
				pBlk->SetZero(); 
				break; 
			case 5: // text
				StrPool.GetS(p_item->TextP, pBlk->TempBuf);
				pBlk->Set(pBlk->TempBuf); 
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

SArray * SLAPI PPViewEvent::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_EVENT;
	SArray * p_array = 0;
	PPTimeSeries ds_item;
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewEvent::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int   ok = -1;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	return ok;
}