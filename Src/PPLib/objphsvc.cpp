// OBJPHSVC.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
#include <idea.h>

int SLAPI TestAsteriskAmiClient(PPID phnSvcID);
//
//
//
/*
ListCommands:

WaitEvent: Wait for an event to occur (Priv: <none>)
SIPnotify: Send a SIP notify (Priv: system,all)
SIPshowregistry: Show SIP registrations (text format) (Priv: system,reporting,all)
SIPqualifypeer: Show SIP peer (text format) (Priv: system,reporting,all)
SIPshowpeer: Show SIP peer (text format) (Priv: system,reporting,all)
SIPpeers: List SIP peers (text format) (Priv: system,reporting,all)
SKINNYshowline: Show SKINNY line (text format) (Priv: system,reporting,all)
SKINNYlines: List SKINNY lines (text format) (Priv: system,reporting,all)
SKINNYshowdevice: Show SKINNY device (text format) (Priv: system,reporting,all)
SKINNYdevices: List SKINNY devices (text format) (Priv: system,reporting,all)
IAXregistry: Show IAX registrations (Priv: system,reporting,all)
IAXnetstats: Show IAX Netstats (Priv: system,reporting,all)
IAXpeerlist: List IAX Peers (Priv: system,reporting,all)
IAXpeers: List IAX Peers (Priv: system,reporting,all)
QueueReset: Reset queue statistics (Priv: <none>)
QueueReload: Reload a queue, queues, or any sub-section of a queue or queues (Priv: <none>)
QueueRule: Queue Rules (Priv: <none>)
QueueSummary: Queue Summary (Priv: <none>)
QueueStatus: Queue Status (Priv: <none>)
Queues: Queues (Priv: <none>)
DAHDIRestart: Fully Restart DAHDI channels (terminates calls) (Priv: <none>)
DAHDIShowChannels: Show status DAHDI channels (Priv: <none>)
DAHDIDNDoff: Toggle DAHDI channel Do Not Disturb status OFF (Priv: <none>)
DAHDIDNDon: Toggle DAHDI channel Do Not Disturb status ON (Priv: <none>)
DAHDIDialOffhook: Dial over DAHDI channel while offhook (Priv: <none>)
DAHDIHangup: Hangup DAHDI Channel (Priv: <none>)
DAHDITransfer: Transfer DAHDI Channel (Priv: <none>)
DBDelTree: Delete DB Tree (Priv: system,all)
DBDel: Delete DB Entry (Priv: system,all)
DBPut: Put DB Entry (Priv: system,all)
DBGet: Get DB Entry (Priv: system,reporting,all)
ParkedCalls: List parked calls (Priv: <none>)
ModuleCheck: Check if module is loaded (Priv: system,all)
ModuleLoad: Module management (Priv: system,all)
CoreShowChannels: List currently active channels (Priv: system,reporting,all)
Reload: Send a reload event (Priv: system,config,all)
CoreStatus: Show PBX core status variables (Priv: system,reporting,all)
CoreSettings: Show PBX core settings (version etc) (Priv: system,reporting,all)
ListCommands: List available manager commands (Priv: <none>)
AbsoluteTimeout: Set Absolute Timeout (Priv: system,call,all)
Status: Lists channel status (Priv: system,call,reporting,all)
GetConfigJSON: Retrieve configuration (JSON format) (Priv: system,config,all)
GetConfig: Retrieve configuration (Priv: system,config,all)
Ping: Keepalive command (Priv: <none>)
Hangup: Hangup Channel (Priv: system,call,all)
Challenge: Generate Challenge for MD5 Auth (Priv: <none>)
Login: Login Manager (Priv: <none>)
Logoff: Logoff Manager (Priv: <none>)
Events: Control Event Flow (Priv: <none>)
*/
/*
Some standard AMI headers

	Account:                 -- Account Code (Status)
	AccountCode:             -- Account Code (cdr_manager)
	ACL: <Y | N>             -- Does ACL exist for object ?
	Action: <action>         -- Request or notification of a particular action
	Address-IP:              -- IPaddress
	Address-Port:            -- IP port number
	Agent: <string>          -- Agent name
	AMAflags:                -- AMA flag (cdr_manager, sippeers)
	AnswerTime:              -- Time of answer (cdr_manager)
	Append: <bool>           -- CDR userfield Append flag
	Application:             -- Application to use
	Async:                   -- Whether or not to use fast setup
	AuthType:                -- Authentication type (for login or challenge) "md5"
	BillableSeconds:         -- Billable seconds for call (cdr_manager)
	CallerID:                -- Caller id (name and number in Originate & cdr_manager)
	CallerID:                -- CallerID number Number or "<unknown>" or "unknown" (should change to "<unknown>" in app_queue)
	CallerID1:               -- Channel 1 CallerID (Link event)
	CallerID2:               -- Channel 2 CallerID (Link event)
	CallerIDName:            -- CallerID name Name or "<unknown>" or "unknown" (should change to "<unknown>" in app_queue)
	Callgroup:               -- Call group for peer/user
	CallsTaken: <num>        -- Queue status variable
	Cause: <value>           -- Event change cause - "Expired"
	Cause: <value>           -- Hangupcause (channel.c)
	CID-CallingPres:         -- Caller ID calling presentation
	Channel: <channel>       -- Channel specifier
	Channel: <dialstring>    -- Dialstring in Originate
	Channel: <tech/[peer/username]> -- Channel in Registry events (SIP, IAX2)
	Channel: <tech>          -- Technology (SIP/IAX2 etc) in Registry events
	ChannelType:             -- Tech: SIP, IAX2, DAHDI, MGCP etc
	Channel1:                -- Link channel 1
	Channel2:                -- Link channel 2
	ChanObjectType:          -- "peer", "user"
	Codecs:                  -- Codec list
	CodecOrder:              -- Codec order, separated with comma ","
	Command:                 -- Cli command to run
	Context:                 -- Context
	Count: <num>             -- Number of callers in queue
	Data:                    -- Application data
	Default-addr-IP:         -- IP address to use before registration
	Default-Username:        -- Username part of URI to use before registration
	Destination:             -- Destination for call (Dialstring ) (dial, cdr_manager)
	DestinationContext:      -- Destination context (cdr_manager)
	DestinationChannel:      -- Destination channel (cdr_manager)
	DestUniqueID:            -- UniqueID of destination (dial event)
	Disposition:             -- Call disposition (CDR manager)
	Domain: <domain>         -- DNS domain
	Duration: <secs>         -- Duration of call (cdr_manager)
	Dynamic: <Y |  N>        -- Device registration supported?
	Endtime:                 -- End time stamp of call (cdr_manager)
	EventList: <flag>        -- Flag being "Start", "End", "Cancelled" or "ListObject"
	Events: <eventmask>      -- Eventmask filter ("on", "off", "system", "call", "log")
	Exten:                   -- Extension (Redirect command)
	Extension:               -- Extension (Status)
	Family: <string>         -- ASTdb key family
	File: <filename>         -- Filename (monitor)
	Format: <format>         -- Format of sound file (monitor)
	From: <time>             --  Parking time (ParkedCall event)
	Hint:                    -- Extension hint
	Incominglimit:           -- SIP Peer incoming limit
	Key:
	Key:                     -- ASTdb Database key
	LastApplication:         -- Last application executed (cdr_manager)
	LastCall: <num>          -- Last call in queue
	LastData:                -- Data for last application (cdr_manager)
	Link:                    -- (Status)
	ListItems: <number>      -- Number of items in Eventlist (Optionally sent in "end" packet)
	Location:                -- Interface (whatever that is -maybe tech/name in app_queue )
	Loginchan:               -- Login channel for agent
	Logintime: <number>      -- Login time for agent
	Mailbox:                 -- VM Mailbox (id@vmcontext) (mailboxstatus, mailboxcount)
	MD5SecretExist: <Y | N>  -- Whether secret exists in MD5 format
	Membership: <string>     -- "Dynamic" or "static" member in queue
	Message: <text>          -- Text message in ACKs, errors (explanation)
	Mix: <bool>              -- Boolean parameter (monitor)
	NewMessages: <count>     -- Count of new Mailbox messages (mailboxcount)
	Newname:
	ObjectName:              -- Name of object in list
	OldName:                 -- Something in Rename (channel.c)
	OldMessages: <count>     -- Count of old mailbox messages (mailboxcount)
	Outgoinglimit:           -- SIP Peer outgoing limit
	Paused: <num>            -- Queue member paused status
	Peer: <tech/name>        -- "channel" specifier :-)
	PeerStatus: <tech/name>  -- Peer status code "Unregistered", "Registered", "Lagged", "Reachable"
	Penalty: <num>           -- Queue penalty
	Priority:                -- Extension priority
	Privilege: <privilege>   -- AMI authorization class (system, call, log, verbose, command, agent, user)
	Pickupgroup:             -- Pickup group for peer
	Position: <num>          -- Position in Queue
	Queue:                   -- Queue name
	Reason:                  -- "Autologoff"
	Reason:                  -- "Chanunavail"
	Response: <response>     -- response code, like "200 OK" "Success", "Error", "Follows"
	Restart:                 -- "True", "False"
	RegExpire:               -- SIP registry expire
	RegExpiry:               -- SIP registry expiry
	Reason:                  -- Originate reason code
	Seconds:                 -- Seconds (Status)
	Secret: <password>       -- Authentication secret (for login)
	SecretExist: <Y | N>     -- Whether secret exists
	Shutdown:                -- "Uncleanly", "Cleanly"
	SIP-AuthInsecure:
	SIP-FromDomain:          -- Peer FromDomain
	SIP-FromUser:            -- Peer FromUser
	SIP-NatSupport:
	SIPLastMsg:
	Source:                  -- Source of call (dial event, cdr_manager)
	SrcUniqueID:             -- UniqueID of source (dial event)
	StartTime:               -- Start time of call (cdr_manager)
	State:                   -- Channel state
	Status:                  -- Registration status (Registry events SIP)
	Status:                  -- Extension status (Extensionstate)
	Status:                  -- Peer status (if monitored)  ** Will change name ** "unknown", "lagged", "ok"
	Status: <num>            -- Queue Status
	Status:                  -- DND status (DNDState)
	Time: <sec>              -- Roundtrip time (latency)
	Timeout:                 -- Parking timeout time
	Timeout:                 -- Timeout for call setup (Originate)
	Timeout: <seconds>       -- Timeout for call
	Uniqueid:                -- Channel Unique ID
	Uniqueid1:               -- Channel 1 Unique ID (Link event)
	Uniqueid2:               -- Channel 2 Unique ID (Link event)
	User:                    -- Username (SIP registry)
	UserField:               -- CDR userfield (cdr_manager)
	Val:                     -- Value to set/read in ASTdb
	Variable:                -- Variable AND value to set (multiple separated with | in Originate)
	Variable: <name>         -- For channel variables
	Value: <value>           -- Value to set
	VoiceMailbox:            -- VM Mailbox in SIPpeers
	Waiting:                 -- Count of mailbox messages (mailboxstatus)
*/
//
//
//
SLAPI PPPhoneService::PPPhoneService()
{
	THISZERO();
}

SLAPI PPPhoneServicePacket::PPPhoneServicePacket()
{
}

int SLAPI PPPhoneServicePacket::GetExField(int fldId, SString & rBuf) const
{
	int    ok = -1;
	rBuf = 0;
	if(oneof4(fldId, PHNSVCEXSTR_ADDR, PHNSVCEXSTR_PORT, PHNSVCEXSTR_USER, PHNSVCEXSTR_PASSWORD)) {
		ok = PPGetExtStrData(fldId, Tail, rBuf);
	}
	return ok;
}

int SLAPI PPPhoneServicePacket::SetExField(int fldId, const char * pBuf)
{
	int    ok = -1;
	if(oneof4(fldId, PHNSVCEXSTR_ADDR, PHNSVCEXSTR_PORT, PHNSVCEXSTR_USER, PHNSVCEXSTR_PASSWORD)) {
		ok = PPPutExtStrData(fldId, Tail, pBuf);
	}
	return ok;
}

#define PHNSVC_PW_SIZE 20

int SLAPI PPPhoneServicePacket::GetPassword(SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	GetExField(PHNSVCEXSTR_PASSWORD, temp_buf);
	Reference::Helper_DecodeOtherPw(0, temp_buf, PHNSVC_PW_SIZE, rBuf);
	/*
	rBuf = 0;
	char   temp_pw[PHNSVC_PW_SIZE], temp_str[PHNSVC_PW_SIZE*3+8];
	temp_buf.CopyTo(temp_str, sizeof(temp_str));
	if(strlen(temp_str) == (PHNSVC_PW_SIZE*3)) {
		for(size_t i = 0, p = 0; i < PHNSVC_PW_SIZE; i++) {
			char   nmb[16];
			nmb[0] = temp_str[p];
			nmb[1] = temp_str[p+1];
			nmb[2] = temp_str[p+2];
			nmb[3] = 0;
			temp_pw[i] = atoi(nmb);
			p += 3;
		}
		IdeaDecrypt(0, temp_pw, sizeof(temp_pw));
	}
	else
		temp_pw[0] = 0;
	rBuf = temp_pw;
	IdeaRandMem(temp_pw, sizeof(temp_pw));
	*/
	return ok;
}

int SLAPI PPPhoneServicePacket::SetPassword(const char * pPassword)
{
	int    ok = 1;
	/*
	char   temp_pw[PHNSVC_PW_SIZE], temp_str[PHNSVC_PW_SIZE*3+8];
	STRNSCPY(temp_pw, pPassword);
	IdeaEncrypt(0, temp_pw, sizeof(temp_pw));
	size_t i = 0, p = 0;
	for(; i < PHNSVC_PW_SIZE; i++) {
		sprintf(temp_str+p, "%03u", (uint8)temp_pw[i]);
		p += 3;
	}
	temp_str[p] = 0;
	SString temp_buf = temp_str;
	*/
	SString temp_buf;
	Reference::Helper_EncodeOtherPw(0, pPassword, PHNSVC_PW_SIZE, temp_buf);
	SetExField(PHNSVCEXSTR_PASSWORD, temp_buf);
	return ok;
}

SLAPI PPObjPhoneService::PPObjPhoneService(void * extraPtr) : PPObjReference(PPOBJ_PHONESERVICE, extraPtr)
{
}

class PhoneServiceDialog : public TDialog {
public:
	PhoneServiceDialog() : TDialog(DLG_PHNSVC)
	{
	}
	int    setDTS(const PPPhoneServicePacket * pData)
	{
		int    ok = 1;
		SString temp_buf;
		Data = *pData;
		setCtrlData(CTL_PHNSVC_NAME, Data.Rec.Name);
		setCtrlData(CTL_PHNSVC_SYMB, Data.Rec.Symb);
		setCtrlLong(CTL_PHNSVC_ID, Data.Rec.ID);
		Data.GetExField(PHNSVCEXSTR_ADDR, temp_buf);
		setCtrlString(CTL_PHNSVC_ADDR, temp_buf);
		Data.GetExField(PHNSVCEXSTR_PORT, temp_buf);
		setCtrlLong(CTL_PHNSVC_PORT, temp_buf.ToLong());
		Data.GetExField(PHNSVCEXSTR_USER, temp_buf);
		setCtrlString(CTL_PHNSVC_USERNAME, temp_buf);
		Data.GetPassword(temp_buf);
		setCtrlString(CTL_PHNSVC_PASSWORD, temp_buf);
		setCtrlString(CTL_PHNSVC_LOCALCHNLSYMB, Data.LocalChannelSymb);
		return ok;
	}
	int    getDTS(PPPhoneServicePacket * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTL_PHNSVC_NAME, Data.Rec.Name);
		THROW_PP(strip(Data.Rec.Name)[0], PPERR_NAMENEEDED);
		getCtrlData(CTL_PHNSVC_SYMB, Data.Rec.Symb);
		getCtrlString(CTL_PHNSVC_ADDR, temp_buf);
		Data.SetExField(PHNSVCEXSTR_ADDR, temp_buf);
		long   port = getCtrlLong(CTL_PHNSVC_PORT);
		Data.SetExField(PHNSVCEXSTR_PORT, (temp_buf = 0).Cat(port));
		getCtrlString(CTL_PHNSVC_USERNAME, temp_buf);
		Data.SetExField(PHNSVCEXSTR_USER, temp_buf);
		getCtrlString(CTL_PHNSVC_PASSWORD, temp_buf);
		Data.SetPassword(temp_buf);
		getCtrlString(CTL_PHNSVC_LOCALCHNLSYMB, Data.LocalChannelSymb);
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
		return ok;
	}
private:
	PPPhoneServicePacket Data;
};

int SLAPI PPObjPhoneService::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1, r = cmCancel, valid_data = 0, is_new = 0;
	PhoneServiceDialog * dlg = 0;
	PPPhoneServicePacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(is_new) {
		pack.Rec.DvcType = PHNSVCTYPE_ASTERISK;
	}
	else {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	THROW(CheckDialogPtr(&(dlg = new PhoneServiceDialog)));
	dlg->setDTS(&pack);
	THROW(EditPrereq(pID, dlg, 0));
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		if(dlg->getDTS(&pack)) {
			valid_data = 1;
			THROW(PutPacket(pID, &pack, 1));
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

int SLAPI PPObjPhoneService::Browse(void * extraPtr)
{
	class PhoneServiceView : public ObjViewDialog {
	public:
		PhoneServiceView(PPObjPhoneService * _ppobj) : ObjViewDialog(DLG_PHNSVCVIEW, _ppobj, 0)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			ObjViewDialog::handleEvent(event);
			if(event.isCmd(cmTest)) {
				PPID   phnsvc_id = getCurrID();
				if(phnsvc_id)
					TestAsteriskAmiClient(phnsvc_id);
				clearEvent(event);
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new PhoneServiceView(this);
		if(CheckDialogPtr(&dlg, 1))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

static const char * RpPhnSvcLocalChannelSymbol = "PhnSvcLocalChannelSymbol";

int SLAPI PPObjPhoneService::PutPacket(PPID * pID, PPPhoneServicePacket * pPack, int use_ta)
{
	int    ok = 1;
	SString tail;
	if(pPack) {
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
		reg_key.PutString(RpPhnSvcLocalChannelSymbol, pPack->LocalChannelSymb);
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				tail = pPack->Tail;
			}
			else {
				THROW(ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
			tail = pPack->Tail;
		}
		THROW(ref->PutPropVlrString(Obj, *pID, PHNSVCPRP_TAIL, tail));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjPhoneService::GetPacket(PPID id, PPPhoneServicePacket * pPack)
{
	int    ok = 1, r;
	THROW(r = Search(id, &pPack->Rec));
	if(r > 0) {
		THROW(ref->GetPropVlrString(Obj, id, PHNSVCPRP_TAIL, pPack->Tail));
		{
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 1); // @v9.2.0 readonly 0-->1
			reg_key.GetString(RpPhnSvcLocalChannelSymbol, pPack->LocalChannelSymb);
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
PhnSvcChannelStatus::PhnSvcChannelStatus()
{
	Clear();
}

PhnSvcChannelStatus & PhnSvcChannelStatus::Clear()
{
	State = 0;
	Flags = 0;
	Priority = 0;
	Seconds = 0;
	Channel = 0;
	CallerId = 0;
	ConnectedLineNum = 0;
	return *this;
}

PhnSvcChannelStatusPool::PhnSvcChannelStatusPool() : SArray(sizeof(Item_))
{
	Pool.add("$", 0); // zero index - is empty string
}

PhnSvcChannelStatusPool & PhnSvcChannelStatusPool::Clear()
{
	Pool.clear(1);
	Pool.add("$", 0); // zero index - is empty string
	SArray::clear();
	return *this;
}

uint PhnSvcChannelStatusPool::GetCount() const
{
	return getCount();
}

int FASTCALL PhnSvcChannelStatusPool::Add(const PhnSvcChannelStatus & rStatus)
{
	int    ok = 1;
	Item_ item;
	MEMSZERO(item);
	item.State = rStatus.State;
	item.Flags = rStatus.Flags;
	item.Priority = rStatus.Priority;
	item.Seconds = rStatus.Seconds;
	if(rStatus.Channel.NotEmpty())
		Pool.add(rStatus.Channel, &item.ChannelPos);
	if(rStatus.CallerId.NotEmpty())
		Pool.add(rStatus.CallerId, &item.CallerIdPos);
	// @v7.8.0 {
	if(rStatus.ConnectedLineNum.NotEmpty())
		Pool.add(rStatus.ConnectedLineNum, &item.ConnectedLineNumPos);
	// } @v7.8.0
	insert(&item);
	return ok;
}

int FASTCALL PhnSvcChannelStatusPool::Get(uint idx, PhnSvcChannelStatus & rStatus) const
{
	int    ok = 1;
	rStatus.Clear();
	if(idx < getCount()) {
		const Item_ & r_item = *(const Item_ *)at(idx);
		rStatus.State = r_item.State;
		rStatus.Flags = r_item.Flags;
		rStatus.Priority = r_item.Priority;
		rStatus.Seconds = r_item.Seconds;
		Pool.getnz(r_item.ChannelPos, rStatus.Channel);
		Pool.getnz(r_item.CallerIdPos, rStatus.CallerId);
		Pool.getnz(r_item.ConnectedLineNumPos, rStatus.ConnectedLineNum); // @v7.8.0
	}
	else
		ok = 0;
	return ok;
}

//static
AsteriskAmiClient::AsteriskAmiStateStr AsteriskAmiClient::StateList[] = {
	{ PhnSvcChannelStatus::stDown,      "Down" },
	{ PhnSvcChannelStatus::stReserved,  "Rsrvd" },
	{ PhnSvcChannelStatus::stOffHook,   "OffHook" },
	{ PhnSvcChannelStatus::stDialing,   "Dialing" },
	{ PhnSvcChannelStatus::stRing,      "Ring" },
	{ PhnSvcChannelStatus::stRinging,   "Ringing" },
	{ PhnSvcChannelStatus::stUp,        "Up" },
	{ PhnSvcChannelStatus::stBusy,      "Busy" },
	{ PhnSvcChannelStatus::stDialingOffHook, "Dialing Offhook" },
	{ PhnSvcChannelStatus::stPreRing,   "Pre-ring" }
};

//static
int AsteriskAmiClient::GetStateText(long state, SString & rBuf)
{
	const long _st = (state & 0xffff);
	rBuf = 0;
	for(uint i = 0; i < SIZEOFARRAY(AsteriskAmiClient::StateList); i++) {
		if(AsteriskAmiClient::StateList[i].State == _st) {
			rBuf = AsteriskAmiClient::StateList[i].P_Str;
			break;
		}
	}
	return BIN(rBuf.NotEmpty());
}

//static
int AsteriskAmiClient::GetStateVal(const char * pText, long * pState)
{
	int    ok = 0;
	long   _st = 0;
	for(uint i = 0; !ok && i < SIZEOFARRAY(AsteriskAmiClient::StateList); i++) {
		if(stricmp(AsteriskAmiClient::StateList[i].P_Str, pText) == 0) {
			_st = AsteriskAmiClient::StateList[i].State;
			ok = 1;
		}
	}
	ASSIGN_PTR(pState, _st);
	return ok;
}

int AsteriskAmiClient::GetChannelList(const char * pChannelName, PhnSvcChannelStatusPool & rList)
{
	int    ok = 1;
	PhnSvcChannelStatus cnl_status;
	Message msg, reply;
	Message::ReplyStatus rs;
	SString temp_buf;
	rList.Clear();
	THROW_PP(State & stLoggedOn, PPERR_PHNSVC_NOTAUTH);
	msg.AddAction("CoreShowChannels");
	THROW(ExecCommand(msg, &reply));
	THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
	if(rs.EventListFlag == 0) {
		do {
			THROW(ReadReply(reply.Clear()));
			THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
			if(reply.GetTag("Event", temp_buf) && temp_buf.CmpNC("CoreShowChannel") == 0) {
				cnl_status.Clear();
				int    do_insert = 0;
				if(reply.GetTag("State", temp_buf) || reply.GetTag("ChannelState", temp_buf)) {
					do_insert = 1;
					if(!GetStateVal(temp_buf, &cnl_status.State)) {
						cnl_status.State = temp_buf.ToLong();
					}
				}
				if(reply.GetTag("Priority", temp_buf)) {
					cnl_status.Priority = temp_buf.ToLong();
				}
				if(reply.GetTag("Seconds", temp_buf))
					cnl_status.Seconds = temp_buf.ToLong();
				else if(reply.GetTag("Duration", temp_buf)) {
					LTIME t;
					strtotime(temp_buf, TIMF_HMS, &t);
					cnl_status.Seconds = t.totalsec();
				}
				if(reply.GetTag("Channel", temp_buf)) {
					do_insert = 1;
					cnl_status.Channel = temp_buf;
				}
				if(reply.GetTag("CallerIDNum", temp_buf)) {
					do_insert = 1;
					cnl_status.CallerId = temp_buf;
				}
				// @v7.8.0 {
				if(reply.GetTag("ConnectedLineNum", temp_buf)) {
					do_insert = 1;
					cnl_status.ConnectedLineNum = temp_buf;
				}
				// } @v7.8.0
				if(do_insert)
					rList.Add(cnl_status);
			}
		} while(rs.EventListFlag <= 0);
	}
	CATCHZOK
	return ok;
}

int AsteriskAmiClient::GetChannelStatus(const char * pChannelName, PhnSvcChannelStatusPool & rList)
{
	int    ok = 1;
	PhnSvcChannelStatus cnl_status;
	Message msg, reply;
	Message::ReplyStatus rs;
	SString temp_buf;
	rList.Clear();
	THROW_PP(State & stLoggedOn, PPERR_PHNSVC_NOTAUTH);
	msg.AddAction("Status");
	if(pChannelName)
		msg.Add("Channel", pChannelName);
	THROW(ExecCommand(msg, &reply));
	THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
	do {
		THROW(ReadReply(reply.Clear()));
		THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
		if(reply.GetTag("Event", temp_buf) && temp_buf.CmpNC("Status") == 0) {
			cnl_status.Clear();
			int    do_insert = 0;
			if(reply.GetTag("State", temp_buf) || reply.GetTag("ChannelState", temp_buf) || reply.GetTag("ChannelStateDesc", temp_buf)) {
				do_insert = 1;
				if(!GetStateVal(temp_buf, &cnl_status.State)) {
					cnl_status.State = temp_buf.ToLong();
				}
			}
			if(reply.GetTag("Priority", temp_buf)) {
				cnl_status.Priority = temp_buf.ToLong();
			}
			if(reply.GetTag("Seconds", temp_buf))
				cnl_status.Seconds = temp_buf.ToLong();
			else if(reply.GetTag("Duration", temp_buf)) {
				LTIME t;
				strtotime(temp_buf, TIMF_HMS, &t);
				cnl_status.Seconds = t.totalsec();
			}
			if(reply.GetTag("Channel", temp_buf)) {
				do_insert = 1;
				cnl_status.Channel = temp_buf;
			}
			if(reply.GetTag("CallerIDNum", temp_buf)) {
				do_insert = 1;
				cnl_status.CallerId = temp_buf;
			}
			// @v7.8.0 {
			if(reply.GetTag("ConnectedLineNum", temp_buf)) {
				do_insert = 1;
				cnl_status.ConnectedLineNum = temp_buf;
			}
			// } @v7.8.0
			if(do_insert)
				rList.Add(cnl_status);
		}
	} while(!(reply.GetTag("Event", temp_buf) && temp_buf.CmpNC("StatusComplete") == 0));
	CATCHZOK
	return ok;
}

AsteriskAmiClient::Message::Message() : StringSet()
{
}

AsteriskAmiClient::Message::Message(const char * pReply) : StringSet()
{
	ParseReply(pReply);
}

AsteriskAmiClient::Message & AsteriskAmiClient::Message::Clear()
{
	StringSet::clear(1);
	return *this;
}

int AsteriskAmiClient::Message::Add(const char * pTag, const char * pValue)
{
	SString temp_buf;
	return add((temp_buf = pTag).CatChar(':').Cat(pValue)) ? 1 : PPSetErrorSLib();
}

int AsteriskAmiClient::Message::AddAction(const char * pValue)
{
	SString temp_buf;
	return add((temp_buf = "Action").CatChar(':').Cat(pValue)) ? 1 : PPSetErrorSLib();
}

int AsteriskAmiClient::Message::Convert(SString & rBuf) const
{
	int    ok = 1;
	uint   c = 0;
	rBuf = 0;
	SString temp_buf, tag, value;
	for(uint p = 0; get(&p, temp_buf);) {
		temp_buf.Divide(':', tag, value);
		rBuf.Cat(tag.Strip()).CatDiv(':', 2).Cat(value.Strip()).CRB();
		c++;
	}
	if(!c)
		rBuf.CRB();
	rBuf.CRB();
	return ok;
}

int AsteriskAmiClient::Message::Get(uint * pPos, SString & rTag, SString & rValue) const
{
	int    ok = 0;
	rTag = 0;
	rValue = 0;
	SString temp_buf;
	if(get(pPos, temp_buf)) {
		temp_buf.Divide(':', rTag, rValue);
		rTag.Strip();
		rValue.Strip();
		ok = 1;
	}
	return ok;
}

int AsteriskAmiClient::Message::GetTag(const char * pTag, SString & rValue) const
{
	int    ok = 0;
	rValue = 0;
	SString temp_buf, tag, value;
	for(uint p = 0; !ok && get(&p, temp_buf);) {
		temp_buf.Divide(':', tag, value);
		if(tag.Strip().CmpNC(pTag) == 0) {
			rValue = value;
			ok = 1;
		}
	}
	return ok;
}

int AsteriskAmiClient::Message::GetReplyStatus(ReplyStatus & rS) const
{
	rS.Code = -1;
	rS.EventListFlag = -1;
	rS.Message = 0;
	SString temp_buf, tag, value;
	for(uint p = 0; get(&p, temp_buf);) {
		temp_buf.Divide(':', tag, value);
		tag.Strip();
		value.Strip();
		if(tag.CmpNC("Response") == 0) {
			if(value.CmpNC("SUCCESS") == 0) {
				rS.Code = 1;
			}
			else if(value.CmpNC("ERROR") == 0) {
				rS.Code = 0;
			}
		}
		else if(tag.CmpNC("Message") == 0) {
			rS.Message = value;
		}
		else if(tag.CmpNC("EventList") == 0) {
			if(value.CmpNC("START") == 0)
				rS.EventListFlag = 0;
			else if(value.CmpNC("END") == 0 || value.CmpNC("COMPLETE") == 0)
				rS.EventListFlag = 1;
			else if(value.CmpNC("CANCELLED") == 0)
				rS.EventListFlag = 2;
		}
	}
	return rS.Code;
}

int AsteriskAmiClient::Message::ParseReply(const char * pReply)
{
	int    ok = 1;
	clear(1);
	SStrScan scan(pReply);
	SString temp_buf, left, right;
	while(scan.Search("\xD\xA")) {
		scan.Get(temp_buf);
		scan.IncrLen(2);
		if(temp_buf.Len()) {
			if(temp_buf.Strip().Divide(':', left, right) > 0) {
				(temp_buf = 0).Cat(left.Strip()).CatChar(':').Cat(right.Strip());
				add(temp_buf);
			}
		}
		else
			break;
	}
	return ok;
}

AsteriskAmiClient::AsteriskAmiClient() : S(1000)
{
	State = 0;
}

AsteriskAmiClient::~AsteriskAmiClient()
{
	Logout();
}

int AsteriskAmiClient::Connect(const char * pServerAddr, int serverPort)
{
	int    ok = 1;
	InetAddr  addr;
	addr.Set(pServerAddr, NZOR(serverPort, 5038));
	S.Disconnect();
	THROW_SL(S.Connect(addr) > 0);
	State |= stConnected;
	CATCHZOK
	return ok;
}

int AsteriskAmiClient::Log(const char * pText)
{
// @v7.9.9 #ifndef NDEBUG // {
	if(CConfig.Flags & CCFLG_DEBUG) { // @v7.9.9
		SString temp_buf;
		(temp_buf = pText).ReplaceStr("\xD\xA", ";", 0);
		PPLogMessage(PPFILNAM_PHNSVC_LOG, temp_buf, LOGMSGF_TIME);
	}
// @v7.9.9 #endif // } !NDEBUG
	return 1;
}

int AsteriskAmiClient::ReadReply(Message & rOut)
{
	int    ok = 1;
	size_t rcvd_size = 0;
	SString temp_buf;
	SBuffer out_buf;
	THROW_PP(S.IsValid(), PPERR_PHNSVC_NOTCONNECTED);
	THROW(S.RecvUntil(out_buf, "\xD\xA\xD\xA", &rcvd_size) > 0);
	{
		const char * p_reply = (const char *)out_buf.GetBuf(out_buf.GetRdOffs());
		(temp_buf = 0).CatN(p_reply, rcvd_size);
		Log(temp_buf);
		rOut.ParseReply(temp_buf);
	}
	CATCHZOK
	return ok;
}

int AsteriskAmiClient::ExecCommand(const Message & rIn, Message * pOut)
{
	int    ok = 1;
	size_t sended_size = 0, rcvd_size = 0;
	SString in_buf, temp_buf;
	SBuffer out_buf;
	THROW_PP(S.IsValid(), PPERR_PHNSVC_NOTCONNECTED);
	THROW(rIn.Convert(in_buf));
	Log(in_buf);
	THROW_SL(S.Send(in_buf, in_buf.Len(), &sended_size));
	if(pOut) {
		THROW(ReadReply(*pOut));
	}
	CATCHZOK
	return ok;
}

int AsteriskAmiClient::Login(const char * pUserName, const char * pPassword)
{
	int    ok = 1;
	SString temp_buf;
	Message msg, reply;
	Message::ReplyStatus rs;
	THROW_PP(S.IsValid(), PPERR_PHNSVC_NOTCONNECTED);
	Logout();
	msg.AddAction("Login");
	msg.Add("Username", pUserName);
	msg.Add("Secret", pPassword);
	THROW(ExecCommand(msg, &reply));
	THROW_PP_S(reply.GetReplyStatus(rs) > 0, PPERR_PHNSVC_LOGIN, rs.Message);
	State |= stLoggedOn;
	CATCHZOK
	return ok;
}

int AsteriskAmiClient::Logout()
{
	int    ok = 1;
	if(State & stLoggedOn) {
		Message msg, reply;
		msg.AddAction("Logoff");
		THROW(ExecCommand(msg, &reply));
		State &= ~stLoggedOn;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}


int SLAPI TestAsteriskAmiClient(PPID phnSvcID)
{
	int    ok = 1;
#ifndef NDEBUG // {
	if(phnSvcID) {
		SString addr_buf, user_buf, secret_buf, temp_buf;
		int    port = 0;
		PPObjPhoneService ps_obj(0);
		PPPhoneServicePacket ps_pack;
		THROW(ps_obj.GetPacket(phnSvcID, &ps_pack) > 0);
		ps_pack.GetExField(PHNSVCEXSTR_ADDR, addr_buf);
		ps_pack.GetExField(PHNSVCEXSTR_PORT, temp_buf);
		port = temp_buf.ToLong();
		ps_pack.GetExField(PHNSVCEXSTR_USER, user_buf);
		ps_pack.GetPassword(secret_buf);
		{
			PhnSvcChannelStatusPool status_list;
			PhnSvcChannelStatus cnl_status;
			StringSet channel_list;
			AsteriskAmiClient client;
			THROW(client.Connect(addr_buf, port));
			THROW(client.Login(user_buf, secret_buf));
			for(uint i = 0; i < 100; i++) {
				AsteriskAmiClient::Message msg, reply;
				THROW(client.GetChannelList(0, status_list));
				/*
				THROW(client.GetChannelStatus(0, status_list));
				if(status_list.GetCount()) {
					for(uint i = 0; i < status_list.GetCount(); i++) {
						if(status_list.Get(i, cnl_status)) {
							(temp_buf = 0).Cat(cnl_status.Channel).CatDiv(';', 2).
								Cat(cnl_status.State).CatDiv(';', 2).
								Cat(cnl_status.Priority).CatDiv(';', 2).
								Cat(cnl_status.Seconds).CatDiv(';', 2).
								Cat(cnl_status.CallerId);
							PPLogMessage(PPFILNAM_PHNSVC_LOG, temp_buf, LOGMSGF_TIME);
						}
					}
				}
				*/
				delay(1000);
			}
			// ...
			THROW(client.Logout());
		}
	}
	CATCHZOKPPERR
#endif // } !NDEBUG
	return ok;
}

