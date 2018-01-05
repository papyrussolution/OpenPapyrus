// PHNPAN.CPP
// Copyright (c) A.Sobolev 2018
//
#include <pp.h>
#pragma hdrstop

class PhonePaneDialog : public TDialog {
public:
	struct State {
		enum {
			lmNone = 0,
			lmBill,
			lmTask,
			lmPersonEvent,
			lmScOp,
			lmScCCheck,  // Чеки по карте
			lmLocCCheck, // Чеки по автономному адресу
			lmSwitchTo
		};
		SLAPI  State() : Mode(lmNone)
		{
		}
		long   Mode;
		SString Channel;
		SString CallerID;
		SString ConnectedLine;
		//
		PPObjIDArray RelEntries;
	};
	PhonePaneDialog(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt);
	static PhonePaneDialog * FindAnalogue(const char * pChannel)
	{
		const long res_id = DLG_PHNCPANE;
		for(TView * p = APPL->P_DeskTop->first(); p != 0; p = p->nextView()) {
			if(p->IsConsistent() && p->GetSubSign() == TV_SUBSIGN_DIALOG && ((TDialog *)p)->resourceID == res_id)
				return (PhonePaneDialog *)p;
		}
		return 0;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
	}
	State  S;
	PhoneServiceEventResponder * P_PSER;
};

PhonePaneDialog::PhonePaneDialog(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt) : TDialog(DLG_PHNCPANE), P_PSER(pPSER)
{
	RVALUEPTR(S, pSt);
}

int SLAPI ShowPhoneCallPane(PhoneServiceEventResponder * pPSER, const PhonePaneDialog::State * pSt)
{
	int    ok = 1;
	PhonePaneDialog * p_prev_dlg = PhonePaneDialog::FindAnalogue("");
	if(p_prev_dlg) {
		ok = -1;
	}
	else {
		PhonePaneDialog * p_dlg = new PhonePaneDialog(pPSER, pSt);
		if(CheckDialogPtr(&p_dlg)) {
			APPL->P_DeskTop->Insert_(p_dlg);
			p_dlg->Insert();
		}
	}
	return ok;
}

SLAPI PhoneServiceEventResponder::PhoneServiceEventResponder() : AdvCookie_Ringing(0), AdvCookie_Up(0)
{
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evPhoneRinging;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = PhoneServiceEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie_Ringing, &adv_blk);
	}
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evPhoneUp;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = PhoneServiceEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie_Up, &adv_blk);
	}
}

SLAPI PhoneServiceEventResponder::~PhoneServiceEventResponder()
{
	DS.Unadvise(AdvCookie_Ringing);
	DS.Unadvise(AdvCookie_Up);
}

int SLAPI PhoneServiceEventResponder::IdentifyCaller(const char * pCaller, PPObjIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	SString caller_buf;
	PPObjLocation loc_obj;
	PPIDArray ea_id_list;
	PPEAddr::Phone::NormalizeStr(pCaller, caller_buf);
	loc_obj.P_Tbl->SearchPhoneIndex(caller_buf, 0, ea_id_list);
	for(uint i = 0; i < ea_id_list.getCount(); i++) {
		EAddrTbl::Rec ea_rec;
		if(loc_obj.P_Tbl->GetEAddr(ea_id_list.get(i), &ea_rec) > 0) {
			rList.Add(ea_rec.LinkObjType, ea_rec.LinkObjID);
			ok = 1;
		}
	}
	return ok;
}

//static 
int PhoneServiceEventResponder::AdviseCallback(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = -1;
	SString msg_buf;
	SString temp_buf;
	SString caller;
	SString channel;
	SString connected_line;
	if(kind == PPAdviseBlock::evPhoneRinging) {
		PhoneServiceEventResponder * p_self = (PhoneServiceEventResponder *)procExtPtr;
		if(p_self) {
			(msg_buf = "PhoneRinging").CatDiv(':', 2);
			pEv->GetExtStrData(pEv->extssChannel, channel);
			msg_buf.CatEq("channel", channel).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssCallerId, caller);
			msg_buf.CatEq("callerid", caller).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssConnectedLineNum, connected_line);
			msg_buf.CatEq("connectedline", connected_line);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME);
			ok = 1;
		}
	}
	else if(kind == PPAdviseBlock::evPhoneUp) {
		PhoneServiceEventResponder * p_self = (PhoneServiceEventResponder *)procExtPtr;
		if(p_self) {
			(msg_buf = "PhoneUp").CatDiv(':', 2);
			pEv->GetExtStrData(pEv->extssChannel, channel);
			msg_buf.CatEq("channel", channel).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssCallerId, caller);
			msg_buf.CatEq("callerid", caller).CatDiv(';', 2);
			pEv->GetExtStrData(pEv->extssConnectedLineNum, connected_line);
			msg_buf.CatEq("connectedline", connected_line);
			PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_TIME);
			{
				PhonePaneDialog::State state;
				state.CallerID = caller;
				state.Channel = channel;
				state.ConnectedLine = connected_line;
				p_self->IdentifyCaller(caller, state.RelEntries);
				ShowPhoneCallPane(p_self, &state);
			}
			ok = 1;
		}
	}
	return ok;
}
