// OBJPHSVC.CPP
// Copyright (c) A.Sobolev 2012, 2013, 2015, 2016, 2017, 2018
//
#include <pp.h>
#pragma hdrstop
// @v9.6.3 #include <idea.h>

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
	rBuf.Z();
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

#define PHNSVC_PW_SIZE 64 // @v9.8.11 20-->64 // @attention изменение значения требует конвертации хранимого пароля

int SLAPI PPPhoneServicePacket::GetPassword(SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	GetExField(PHNSVCEXSTR_PASSWORD, temp_buf);
	Reference::Helper_DecodeOtherPw(0, temp_buf, PHNSVC_PW_SIZE, rBuf);
	return ok;
}

int SLAPI PPPhoneServicePacket::SetPassword(const char * pPassword)
{
	int    ok = 1;
	SString temp_buf;
	Reference::Helper_EncodeOtherPw(0, pPassword, PHNSVC_PW_SIZE, temp_buf);
	SetExField(PHNSVCEXSTR_PASSWORD, temp_buf);
	return ok;
}

//static 
int FASTCALL PPObjPhoneService::IsPhnChannelAcceptable(const SString & rFilter, const SString & rChannel)
{
	int    ok = 0;
	if(rFilter.NotEmpty() && rChannel.NotEmpty()) {
		if(rFilter.IsEqiAscii("@all")) {
			ok = 1;
		}
		else if(rFilter.HasChr(';') || rFilter.HasChr(',') || rFilter.HasChr(' ')) {
			SString temp_buf;
            StringSet ss;
            rFilter.Tokenize(" ;,", ss);
            for(uint ssp = 0; !ok && ss.get(&ssp, temp_buf);) {
				if(rChannel.CmpPrefix(temp_buf, 1) == 0)
					ok = 1;
            }
		}
		else if(rChannel.CmpPrefix(rFilter, 1) == 0) {
			ok = 1;
		}
	}
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
		setCtrlString(CTL_PHNSVC_SCANCHNL, Data.ScanChannelSymb); // @v9.9.11
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
		Data.SetExField(PHNSVCEXSTR_PORT, temp_buf.Z().Cat(port));
		getCtrlString(CTL_PHNSVC_USERNAME, temp_buf);
		Data.SetExField(PHNSVCEXSTR_USER, temp_buf);
		getCtrlString(CTL_PHNSVC_PASSWORD, temp_buf);
		Data.SetPassword(temp_buf);
		getCtrlString(CTL_PHNSVC_LOCALCHNLSYMB, Data.LocalChannelSymb);
		getCtrlString(CTL_PHNSVC_SCANCHNL, Data.ScanChannelSymb); // @v9.9.11
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
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

static const char * RpPhnSvcLocalUpChannelSymbol = "PhnSvcLocalChannelSymbol";
static const char * RpPhnSvcLocalScanChannelSymbol = "PhnSvcLocalScanChannelSymbol";

int SLAPI PPObjPhoneService::PutPacket(PPID * pID, PPPhoneServicePacket * pPack, int use_ta)
{
	int    ok = 1;
	SString tail;
	if(pPack) {
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::SysSettings, 0);
		reg_key.PutString(RpPhnSvcLocalUpChannelSymbol, pPack->LocalChannelSymb);
		reg_key.PutString(RpPhnSvcLocalScanChannelSymbol, pPack->ScanChannelSymb); // @v9.9.11
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
			reg_key.GetString(RpPhnSvcLocalUpChannelSymbol, pPack->LocalChannelSymb);
			reg_key.GetString(RpPhnSvcLocalScanChannelSymbol, pPack->ScanChannelSymb); // @v9.9.11
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
PhnSvcChannelStatus::PhnSvcChannelStatus() : State(stUndef), Flags(0), Priority(0), Type(0), TimeToHungUp(0), Seconds(0), CallGroup(0), PickUpGroup(0)
{
}

PhnSvcChannelStatus & PhnSvcChannelStatus::Clear()
{
	State = stUndef;
	Flags = 0;
	Priority = 0;
	Type = 0;
	TimeToHungUp = 0;
	Seconds = 0;
	CallGroup = 0;
	PickUpGroup = 0;
	Channel.Z();
	CallerId.Z();
	CallerIdName.Z();
	ConnectedLineNum.Z();
	ConnectedLineName.Z();
	AccountCode.Z();
	Context.Z();
	Exten.Z();
	DnId.Z();
	EffConnectedLineNum.Z();
	EffConnectedLineName.Z();
	Application.Z();
	Data.Z();
	return *this;
}

PhnSvcChannelStatusPool::PhnSvcChannelStatusPool() : SVector(sizeof(Item_)), SStrGroup() // @v9.8.11 SArray-->SVector
{
	//Pool.add("$"); // zero index - is empty string
}

PhnSvcChannelStatusPool & PhnSvcChannelStatusPool::Clear()
{
	//Pool.clear();
	//Pool.add("$"); // zero index - is empty string
	ClearS();
	SVector::clear(); // @v9.8.11 SArray-->SVector
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
	item.Type = rStatus.Type;
	item.TimeToHungUp = rStatus.TimeToHungUp;
	item.Seconds = rStatus.Seconds;
	item.CallGroup = rStatus.CallGroup;
	item.PickUpGroup = rStatus.PickUpGroup;
	AddS(rStatus.Channel, &item.ChannelPos);
	AddS(rStatus.CallerId, &item.CallerIdPos);
	AddS(rStatus.CallerIdName, &item.CallerIdNameP);
	AddS(rStatus.ConnectedLineNum, &item.ConnectedLineNumPos);
	AddS(rStatus.ConnectedLineName, &item.ConnectedLineNameP);
	AddS(rStatus.AccountCode, &item.AccountCodeP);
	AddS(rStatus.Context, &item.ContextP);
	AddS(rStatus.Exten, &item.ExtenP);
	AddS(rStatus.DnId, &item.DnIdP);
	AddS(rStatus.EffConnectedLineNum, &item.EffConnectedLineNumP);
	AddS(rStatus.EffConnectedLineName, &item.EffConnectedLineNameP);
	AddS(rStatus.Application, &item.ApplicationP);
	AddS(rStatus.Data, &item.DataP);
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
		rStatus.Type = r_item.Type;
		rStatus.TimeToHungUp = r_item.TimeToHungUp;
		rStatus.Seconds = r_item.Seconds;
		rStatus.CallGroup = r_item.CallGroup;
		rStatus.PickUpGroup = r_item.PickUpGroup;
		GetS(r_item.ChannelPos, rStatus.Channel);
		GetS(r_item.CallerIdPos, rStatus.CallerId);
		GetS(r_item.CallerIdNameP, rStatus.CallerIdName);
		GetS(r_item.ConnectedLineNumPos, rStatus.ConnectedLineNum);
		GetS(r_item.ConnectedLineNameP, rStatus.ConnectedLineName);
		GetS(r_item.AccountCodeP, rStatus.AccountCode);
		GetS(r_item.ContextP, rStatus.Context);
		GetS(r_item.ExtenP, rStatus.Exten);
		GetS(r_item.DnIdP, rStatus.DnId);
		GetS(r_item.EffConnectedLineNumP, rStatus.EffConnectedLineNum);
		GetS(r_item.EffConnectedLineNameP, rStatus.EffConnectedLineName);
		GetS(r_item.ApplicationP, rStatus.Application);
		GetS(r_item.DataP, rStatus.Data);
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
	int    ok = 0;
	const  long _st = (state & 0xffff);
	rBuf.Z();
	for(uint i = 0; !ok && i < SIZEOFARRAY(AsteriskAmiClient::StateList); i++) {
		if(AsteriskAmiClient::StateList[i].State == _st) {
			rBuf = AsteriskAmiClient::StateList[i].P_Str;
			ok = 1;
		}
	}
	if(!ok)
		rBuf = "Undef";
	return ok;
}

//static
int AsteriskAmiClient::GetStateVal(const char * pText, long * pState)
{
	int    ok = 0;
	long   _st = 0;
	for(uint i = 0; !ok && i < SIZEOFARRAY(AsteriskAmiClient::StateList); i++) {
		if(sstreqi_ascii(AsteriskAmiClient::StateList[i].P_Str, pText)) {
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
	SString temp_buf, key_buf, val_buf;
	rList.Clear();
	THROW_PP(State & stLoggedOn, PPERR_PHNSVC_NOTAUTH);
	msg.AddAction("CoreShowChannels");
	const int64 action_id = ++LastActionId; // @v9.9.9
	msg.Add("ActionID", temp_buf.Z().Cat(action_id)); // @v9.9.9
	THROW(ExecCommand(msg, &reply));
	THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
	if(rs.EventListFlag == 0) {
		do {
			THROW(ReadReply(reply.Clear()));
			THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
			if(reply.GetTag("Event", temp_buf)) {
				if(temp_buf.IsEqiAscii("CoreShowChannel")) {
					cnl_status.Clear();
					int    do_insert = 0;
					for(uint p = 0; reply.get(&p, temp_buf);) {
						temp_buf.Divide(':', key_buf, val_buf);
						key_buf.Strip();
						val_buf.Strip();
						if(key_buf.IsEqiAscii("State") || key_buf.IsEqiAscii("ChannelState") || key_buf.IsEqiAscii("ChannelStateDesc")) {
							if(cnl_status.State == cnl_status.stUndef) {
								do_insert = 1;
								if(!GetStateVal(val_buf, &cnl_status.State))
									cnl_status.State = val_buf.ToLong();
							}
						}
						else if(key_buf.IsEqiAscii("Type")) {
							if(val_buf.IsEqiAscii("SIP"))
								cnl_status.Type = cnl_status.typSip;
						}
						else if(key_buf.IsEqiAscii("Priority"))
							cnl_status.Priority = val_buf.ToLong();
						else if(key_buf.IsEqiAscii("Seconds"))
							cnl_status.Seconds = val_buf.ToLong();
						else if(key_buf.IsEqiAscii("Duration")) {
							LTIME t;
							strtotime(val_buf, TIMF_HMS, &t);
							cnl_status.Seconds = t.totalsec();
						}
						else if(key_buf.IsEqiAscii("TimeToHangup"))
							cnl_status.TimeToHungUp = val_buf.ToLong();
						else if(key_buf.IsEqiAscii("Callgroup"))
							cnl_status.CallGroup = val_buf.ToInt64();
						else if(key_buf.IsEqiAscii("Pickupgroup"))
							cnl_status.PickUpGroup = val_buf.ToInt64();
						else if(key_buf.IsEqiAscii("Channel")) {
							do_insert = 1;
							cnl_status.Channel = val_buf;
						}
						else if(key_buf.IsEqiAscii("CallerIDNum")) {
							do_insert = 1;
							cnl_status.CallerId = val_buf;
						}
						else if(key_buf.IsEqiAscii("ConnectedLineNum")) {
							do_insert = 1;
							cnl_status.ConnectedLineNum = val_buf;
						}
						else if(key_buf.IsEqiAscii("ConnectedLineName")) {
							do_insert = 1;
							cnl_status.ConnectedLineName = val_buf;
						}
						else if(key_buf.IsEqiAscii("EffectiveConnectedLineNum")) {
							do_insert = 1;
							cnl_status.EffConnectedLineNum = val_buf;
						}
						else if(key_buf.IsEqiAscii("EffectiveConnectedLineName")) {
							do_insert = 1;
							cnl_status.EffConnectedLineName = val_buf;
						}
						else if(key_buf.IsEqiAscii("Accountcode"))
							cnl_status.AccountCode = val_buf;
						else if(key_buf.IsEqiAscii("Context"))
							cnl_status.Context = val_buf;
						else if(key_buf.IsEqiAscii("Exten"))
							cnl_status.Exten = val_buf;
						else if(key_buf.IsEqiAscii("DNID"))
							cnl_status.DnId = val_buf;
						else if(key_buf.IsEqiAscii("Application"))
							cnl_status.Application = val_buf;
						else if(key_buf.IsEqiAscii("Data"))
							cnl_status.Data = val_buf;
					}
					if(do_insert)
						rList.Add(cnl_status);
				}
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
	SString key_buf, val_buf;
	rList.Clear();
	THROW_PP(State & stLoggedOn, PPERR_PHNSVC_NOTAUTH);
	msg.AddAction("Status");
	const int64 action_id = ++LastActionId; // @v9.9.9
	msg.Add("ActionID", temp_buf.Z().Cat(action_id)); // @v9.9.9
	if(pChannelName)
		msg.Add("Channel", pChannelName);
	THROW(ExecCommand(msg, &reply));
	THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
	do {
		THROW(ReadReply(reply.Clear()));
		THROW_PP_S(reply.GetReplyStatus(rs) != 0, PPERR_PHNSVC_ERROR, rs.Message);
		if(reply.GetTag("Event", temp_buf) && temp_buf.IsEqiAscii("Status")) {
			if(!reply.GetTag("ActionID", temp_buf) || temp_buf.ToInt64() == action_id) { // @v9.9.9
				cnl_status.Clear();
				int    do_insert = 0;
				for(uint p = 0; reply.get(&p, temp_buf);) {
					temp_buf.Divide(':', key_buf, val_buf);
					key_buf.Strip();
					val_buf.Strip();
					if(key_buf.IsEqiAscii("State") || key_buf.IsEqiAscii("ChannelState") || key_buf.IsEqiAscii("ChannelStateDesc")) {
						if(cnl_status.State == cnl_status.stUndef) {
							do_insert = 1;
							if(!GetStateVal(val_buf, &cnl_status.State))
								cnl_status.State = val_buf.ToLong();
						}
					}
					else if(key_buf.IsEqiAscii("Type")) {
						if(val_buf.IsEqiAscii("SIP"))
							cnl_status.Type = cnl_status.typSip;
					}
					else if(key_buf.IsEqiAscii("Priority"))
						cnl_status.Priority = val_buf.ToLong();
					else if(key_buf.IsEqiAscii("Seconds"))
						cnl_status.Seconds = val_buf.ToLong();
					else if(key_buf.IsEqiAscii("Duration")) {
						LTIME t;
						strtotime(val_buf, TIMF_HMS, &t);
						cnl_status.Seconds = t.totalsec();
					}
					else if(key_buf.IsEqiAscii("TimeToHangup"))
						cnl_status.TimeToHungUp = val_buf.ToLong();
					else if(key_buf.IsEqiAscii("Callgroup"))
						cnl_status.CallGroup = val_buf.ToInt64();
					else if(key_buf.IsEqiAscii("Pickupgroup"))
						cnl_status.PickUpGroup = val_buf.ToInt64();
					else if(key_buf.IsEqiAscii("Channel")) {
						do_insert = 1;
						cnl_status.Channel = val_buf;
					}
					else if(key_buf.IsEqiAscii("CallerIDNum")) {
						do_insert = 1;
						cnl_status.CallerId = val_buf;
					}
					else if(key_buf.IsEqiAscii("ConnectedLineNum")) {
						do_insert = 1;
						cnl_status.ConnectedLineNum = val_buf;
					}
					else if(key_buf.IsEqiAscii("ConnectedLineName")) {
						do_insert = 1;
						cnl_status.ConnectedLineName = val_buf;
					}
					else if(key_buf.IsEqiAscii("EffectiveConnectedLineNum")) {
						do_insert = 1;
						cnl_status.EffConnectedLineNum = val_buf;
					}
					else if(key_buf.IsEqiAscii("EffectiveConnectedLineName")) {
						do_insert = 1;
						cnl_status.EffConnectedLineName = val_buf;
					}
					else if(key_buf.IsEqiAscii("Accountcode"))
						cnl_status.AccountCode = val_buf;
					else if(key_buf.IsEqiAscii("Context"))
						cnl_status.Context = val_buf;
					else if(key_buf.IsEqiAscii("Exten"))
						cnl_status.Exten = val_buf;
					else if(key_buf.IsEqiAscii("DNID"))
						cnl_status.DnId = val_buf;
					else if(key_buf.IsEqiAscii("Application"))
						cnl_status.Application = val_buf;
					else if(key_buf.IsEqiAscii("Data"))
						cnl_status.Data = val_buf;
				}
				if(do_insert)
					rList.Add(cnl_status);
			}
		}
	} while(!(reply.GetTag("Event", temp_buf) && temp_buf.IsEqiAscii("StatusComplete")));
	CATCH
		PPLogMessage(PPFILNAM_PHNSVC_LOG, 0, LOGMSGF_TIME|LOGMSGF_LASTERR);
		ok = 0;
	ENDCATCH
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
	StringSet::clear();
	return *this;
}

int AsteriskAmiClient::Message::Add(const char * pTag, const char * pValue)
{
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.9
	return add((r_temp_buf = pTag).CatChar(':').Cat(pValue)) ? 1 : PPSetErrorSLib();
}

int AsteriskAmiClient::Message::AddAction(const char * pValue)
{
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.9
	return add((r_temp_buf = "Action").CatChar(':').Cat(pValue)) ? 1 : PPSetErrorSLib();
}

int AsteriskAmiClient::Message::Convert(SString & rBuf) const
{
	int    ok = 1;
	uint   c = 0;
	rBuf.Z();
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
	rTag.Z();
	rValue.Z();
	SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.9
	if(get(pPos, r_temp_buf)) {
		r_temp_buf.Divide(':', rTag, rValue);
		rTag.Strip();
		rValue.Strip();
		ok = 1;
	}
	return ok;
}

int AsteriskAmiClient::Message::Parse(StrStrAssocArray & rTags) const
{
	int    ok = 0;
	rTags.Clear();
	SString temp_buf, tag, value;
	for(uint p = 0; !ok && get(&p, temp_buf);) {
		if(temp_buf.Divide(':', tag, value) > 0) {
			rTags.Add(tag.Strip(), value.Strip(), 0);
		}
	}
	return ok;
}

int AsteriskAmiClient::Message::GetTag(const char * pTag, SString & rValue) const
{
	int    ok = 0;
	rValue.Z();
	SString temp_buf, tag;
	for(uint p = 0; !ok && get(&p, temp_buf);) {
		temp_buf.Divide(':', tag, rValue);
		if(tag.Strip().CmpNC(pTag) == 0)
			ok = 1;
		else
			rValue.Z();
	}
	return ok;
}

int AsteriskAmiClient::Message::GetReplyStatus(ReplyStatus & rS) const
{
	rS.Code = -1;
	rS.EventListFlag = -1;
	rS.Message.Z();
	SString temp_buf, tag, value;
	for(uint p = 0; get(&p, temp_buf);) {
		temp_buf.Divide(':', tag, value);
		tag.Strip();
		value.Strip();
		if(tag.IsEqiAscii("Response")) {
			if(value.IsEqiAscii("SUCCESS")) {
				rS.Code = 1;
			}
			else if(value.IsEqiAscii("ERROR")) {
				rS.Code = 0;
			}
		}
		else if(tag.IsEqiAscii("Message")) {
			rS.Message = value;
		}
		else if(tag.IsEqiAscii("EventList")) {
			if(value.IsEqiAscii("START"))
				rS.EventListFlag = 0;
			else if(value.IsEqiAscii("END") || value.IsEqiAscii("COMPLETE"))
				rS.EventListFlag = 1;
			else if(value.IsEqiAscii("CANCELLED"))
				rS.EventListFlag = 2;
		}
	}
	return rS.Code;
}

int AsteriskAmiClient::Message::ParseReply(const char * pReply)
{
	int    ok = 1;
	clear();
	SStrScan scan(pReply);
	SString temp_buf, left, right;
	while(scan.Search("\xD\xA")) {
		scan.Get(temp_buf);
		scan.IncrLen(2);
		if(temp_buf.Len()) {
			if(temp_buf.Strip().Divide(':', left, right) > 0) {
				temp_buf.Z().Cat(left.Strip()).CatChar(':').Cat(right.Strip());
				add(temp_buf);
			}
		}
		else
			break;
	}
	return ok;
}

AsteriskAmiClient::AsteriskAmiClient(long flags) : S(1000), Flags(flags), State(0), LastActionId(clock())
{
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
	if(Flags & fDoLog) {
		SString & r_temp_buf = SLS.AcquireRvlStr(); // @v9.9.10
		(r_temp_buf = pText).ReplaceStr("\xD\xA", ";", 0).Transf(CTRANSF_UTF8_TO_INNER);
		PPLogMessage(PPFILNAM_PHNSVC_LOG, r_temp_buf, LOGMSGF_TIME);
	}
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
		temp_buf.Z().CatN(p_reply, rcvd_size);
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
			AsteriskAmiClient client(AsteriskAmiClient::fDoLog);
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
							temp_buf.Z().Cat(cnl_status.Channel).CatDiv(';', 2).
								Cat(cnl_status.State).CatDiv(';', 2).
								Cat(cnl_status.Priority).CatDiv(';', 2).
								Cat(cnl_status.Seconds).CatDiv(';', 2).
								Cat(cnl_status.CallerId);
							PPLogMessage(PPFILNAM_PHNSVC_LOG, temp_buf, LOGMSGF_TIME);
						}
					}
				}
				*/
				SDelay(1000);
			}
			// ...
			THROW(client.Logout());
		}
	}
	CATCHZOKPPERR
#endif // } !NDEBUG
	return ok;
}

