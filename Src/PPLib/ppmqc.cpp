// PPMQC.CPP
// Copyright (c) A.Sobolev 2019, 2020, 2021, 2022, 2023, 2024
//
#include <pp.h>
#pragma hdrstop
#include <amqp.h>

static inline amqp_connection_state_t GetNativeConnHandle(void * pConn)
{
	return static_cast<amqp_connection_state_t>(pConn);
}

PPMqbClient::LoginParam::LoginParam() : Method(1)
{
}

int FASTCALL PPMqbClient::LoginParam::Copy(const PPMqbClient::LoginParam & rS)
{
	Method = rS.Method;
	Auth = rS.Auth;
	Secret = rS.Secret;
	VHost = rS.VHost; // @v11.0.9
	return 1;
}

const char * PPMqbClient::LoginParam::GetVHost(const char * pDefault) const
{
	return VHost.NotEmpty() ? VHost.cptr() : (isempty(pDefault) ? "papyrus" : pDefault);
}

int FASTCALL PPMqbClient::LoginParam::IsEq(const LoginParam & rS) const
{
	return (Method == rS.Method && Auth == rS.Auth && Secret == rS.Secret);
}

PPMqbClient::RoutingParamEntry::RoutingParamEntry() : RtRsrv(0), QueueFlags(0), ExchangeType(exgtDefault), ExchangeFlags(0), 
	/*RpcReplyQueueFlags(0), RpcReplyExchangeType(0), RpcReplyExchangeFlags(0),*/PreprocessFlags(0)
{
}

int FASTCALL PPMqbClient::RoutingParamEntry::IsEq(const RoutingParamEntry & rS) const
{
	#define ISEQF(f) ((f)==(rS.f))
	return (ISEQF(RtRsrv) && ISEQF(QueueFlags) && ISEQF(ExchangeType) && ISEQF(ExchangeFlags) && 
		//ISEQF(RpcReplyQueueFlags) && ISEQF(RpcReplyExchangeType) && ISEQF(RpcReplyExchangeFlags) && 
		ISEQF(QueueName) && ISEQF(ExchangeName) && ISEQF(RoutingKey) && ISEQF(CorrelationId) //&& 
		//ISEQF(RpcReplyQueueName) && ISEQF(RpcReplyExchangeName) && ISEQF(RpcReplyRoutingKey)
	);
	#undef ISEQF
}

PPMqbClient::RoutingParamEntry & PPMqbClient::RoutingParamEntry::Z()
{
	RtRsrv = 0;
	PreprocessFlags = 0; // @v11.0.9
	QueueFlags = 0;
	ExchangeType = 0;
	ExchangeFlags = 0;
	//RpcReplyQueueFlags = 0;
	//RpcReplyExchangeType = 0;
	//RpcReplyExchangeFlags = 0;
	QueueName.Z();
	ExchangeName.Z();
	RoutingKey.Z();
	CorrelationId.Z();
	//RpcReplyQueueName.Z();
	//RpcReplyExchangeName.Z();
	//RpcReplyRoutingKey.Z();
	return *this;
}

static const SIntToSymbTabEntry MqbReservedRoutePrefix[] = {
	{ PPMqbClient::rtrsrvPapyrusDbx, "papyrusdbx" },
	{ PPMqbClient::rtrsrvPapyrusPosProtocol, "papyrusposprotocol" },
	{ PPMqbClient::rtrsrvStyloView, "papyrusstyloview" },
	{ PPMqbClient::rtrsrvRpc, "papyrusrpc" },
	{ PPMqbClient::rtrsrvRpcReply, "papyrusrpcreply" },
	{ PPMqbClient::rtrsrvStyloQRpc, "styloqrpc" },
	{ PPMqbClient::rtrsrvStyloQRpcReply, "styloqrpcreply" },
};

int PPMqbClient::RoutingParamEntry::SetupRpcReply(const PPMqbClient::Envelope & rSrcEnv)
{
	Z();
	int    ok = 1;
	RtRsrv = rtrsrvRpcReply;
	QueueName = rSrcEnv.Msg.Props.ReplyTo;
	RoutingKey = rSrcEnv.Msg.Props.ReplyTo;
	CorrelationId = rSrcEnv.Msg.Props.CorrelationId;
	SIntToSymbTab_GetSymb(MqbReservedRoutePrefix, SIZEOFARRAY(MqbReservedRoutePrefix), rtrsrvRpcReply, ExchangeName);
	QueueFlags = 0;
	ExchangeType = exgtDirect;
	ExchangeFlags = 0; 
	//CATCHZOK
	return ok;
}

int PPMqbClient::RoutingParamEntry::SetupStyloQRpcListener(const SBinaryChunk & rIdent)
{
	Z();
	int    ok = 1;
	SString temp_buf;
	THROW(rIdent.Len());
	QueueName.Cat(rIdent.Mime64(temp_buf));
	RoutingKey = temp_buf;
	ExchangeName = "styloqrpc";
	QueueFlags = 0;
	ExchangeType = exgtDirect;
	ExchangeFlags = 0; 
	CATCHZOK
	return ok;
}

int PPMqbClient::RoutingParamEntry::SetupStyloQRpc(const SBinaryChunk & rSrcIdent, const SBinaryChunk & rDestIdent, RoutingParamEntry * pReplyEntry)
{
	Z();
	int    ok = 1;
	SString temp_buf;
	THROW(rSrcIdent.Len());  // PPERR_SQ_MQRPC_SRCIDEMPTY  ��������� MQ-RPC - ������ ��������� ������������� 
	THROW(rDestIdent.Len()); // PPERR_SQ_MQRPC_DESTIDEMPTY ��������� MQ-RPC - ������ ������������� ����������
	QueueName.Cat(rDestIdent.Mime64(temp_buf));
	RtRsrv = rtrsrvRpc;
	RoutingKey = temp_buf;
	ExchangeName = "styloqrpc";
	QueueFlags = 0;
	ExchangeType = exgtDirect;
	ExchangeFlags = 0; 
	if(pReplyEntry) {
		rSrcIdent.Mime64(temp_buf);
		//CorrelationId = temp_buf;
		pReplyEntry->QueueName = temp_buf;
		pReplyEntry->QueueFlags = mqofAutoDelete;
		pReplyEntry->ExchangeName = "styloqrpc" /*"styloqrpcreply"*/;
		pReplyEntry->ExchangeType = exgtDirect;
		pReplyEntry->ExchangeFlags = 0;
		pReplyEntry->RoutingKey = pReplyEntry->QueueName;
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::RoutingParamEntry::SetupReserved(int rsrv, const char * pDomain, const S_GUID * pDestGuid, long destId, RoutingParamEntry * pReplyEntry)
{
	Z();
	int    ok = 1;
	SString symb;
	THROW(SIntToSymbTab_GetSymb(MqbReservedRoutePrefix, SIZEOFARRAY(MqbReservedRoutePrefix), (rsrv & ~0x8000), symb));
	RtRsrv = rsrv;
	switch(rsrv & ~0x8000) {
		case rtrsrvPapyrusDbx:
			{
				THROW_S_S(!isempty(pDomain), SLERR_INVPARAM, __FUNCTION__"/pDomain");
				THROW(destId > 0);
				QueueName.Z().Cat(symb).DotCat(pDomain).Dot().Cat(destId);
				(RoutingKey = pDomain).Dot().Cat(destId);
				ExchangeName = symb;
				QueueFlags = 0;
				ExchangeType = exgtDirect;
				ExchangeFlags = 0; 
			}
			break;
		case rtrsrvPapyrusPosProtocol:
			{
				THROW_S_S(!isempty(pDomain), SLERR_INVPARAM, __FUNCTION__"/pDomain");
				THROW(!S_GUID::IsEmpty(pDestGuid));
				QueueName.Z().Cat(symb).Dot().Cat(pDomain).Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				(RoutingKey = pDomain).Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				ExchangeName = symb;
				QueueFlags = 0;
				ExchangeType = exgtDirect;
				ExchangeFlags = 0; 
			}
			break;
		case rtrsrvStyloView:
			{
				THROW_S_S(!isempty(pDomain), SLERR_INVPARAM, __FUNCTION__"/pDomain");
				THROW(!S_GUID::IsEmpty(pDestGuid));
				QueueName.Z().Cat(symb).Dot().Cat(pDomain).Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				(RoutingKey = pDomain).Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				ExchangeName = symb;
				QueueFlags = 0;
				ExchangeType = exgtDirect;
				ExchangeFlags = 0; 
			}
			break;
		case rtrsrvRpc:
			{
				SString temp_buf;
				THROW_S_S(!isempty(pDomain), SLERR_INVPARAM, __FUNCTION__"/pDomain");
				QueueName.Z().Cat(symb).Dot().Cat(pDomain);
				if(pDestGuid) {
					QueueName.Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				}
				else if(destId > 0) {
					QueueName.Dot().Cat(destId);
				}
				RoutingKey = pDomain;
				if(pDestGuid) {
					RoutingKey.Dot().Cat(*pDestGuid, S_GUID::fmtIDL|S_GUID::fmtLower);
				}
				else if(destId > 0) {
					RoutingKey.Dot().Cat(destId);
				}
				ExchangeName = symb;
				QueueFlags = 0;
				ExchangeType = exgtDirect;
				ExchangeFlags = 0; 
				if(!(rsrv & 0x8000) && pReplyEntry) {
					const S_GUID reply_guid(SCtrGenerate_);
					CorrelationId.Z().Cat(reply_guid, S_GUID::fmtPlain|S_GUID::fmtLower);
					//
					SIntToSymbTab_GetSymb(MqbReservedRoutePrefix, SIZEOFARRAY(MqbReservedRoutePrefix), PPMqbClient::rtrsrvRpcReply, temp_buf);
					(pReplyEntry->QueueName = temp_buf).Dot().Cat(pDomain).Dot().Cat(reply_guid, S_GUID::fmtPlain|S_GUID::fmtLower);
					pReplyEntry->QueueFlags = mqofAutoDelete;
					pReplyEntry->ExchangeName = temp_buf;
					pReplyEntry->ExchangeType = exgtDirect;
					pReplyEntry->ExchangeFlags = 0;
					//RpcReplyRoutingKey.Z().Cat(reply_guid, S_GUID::fmtPlain|S_GUID::fmtLower);
					pReplyEntry->RoutingKey = pReplyEntry->QueueName;
				}
			}
			break;
		case rtrsrvRpcReply:
			{
				// @construction
			}
			break;
		case rtrsrvStyloQRpc:
			{
				SString temp_buf;
				THROW_S_S(!isempty(pDomain), SLERR_INVPARAM, __FUNCTION__"/pDomain");
				QueueName.Z().Cat(symb).Dot().Cat(pDomain);
				RoutingKey = pDomain;
				ExchangeName = symb;
				QueueFlags = 0;
				ExchangeType = exgtDirect;
				ExchangeFlags = 0; 
				if(!(rsrv & 0x8000) && pReplyEntry) {
					const S_GUID reply_guid(SCtrGenerate_);
					CorrelationId.Z().Cat(reply_guid, S_GUID::fmtPlain|S_GUID::fmtLower);
					//
					SIntToSymbTab_GetSymb(MqbReservedRoutePrefix, SIZEOFARRAY(MqbReservedRoutePrefix), PPMqbClient::rtrsrvRpcReply, temp_buf);
					(pReplyEntry->QueueName = temp_buf).Dot().Cat(pDomain).Dot().Cat(reply_guid, S_GUID::fmtPlain|S_GUID::fmtLower);
					pReplyEntry->QueueFlags = mqofAutoDelete;
					pReplyEntry->ExchangeName = temp_buf;
					pReplyEntry->ExchangeType = exgtDirect;
					pReplyEntry->ExchangeFlags = 0;
					pReplyEntry->RoutingKey = pReplyEntry->QueueName;
				}
			}
			break;
		case rtrsrvStyloQRpcReply:
			break;
		default:
			assert(0); // It's impossible to reach this point because of THROW(SIntToSymbTab_GetSymb()) above.
			ok = 0;
			break;
	}
	CATCHZOK
	return ok;
}

PPMqbClient::InitParam::InitParam() : Port(0)
{
}

PPMqbClient::InitParam::InitParam(const InitParam & rS) : LoginParam(rS)
{
	Copy(rS);
}

PPMqbClient::InitParam & FASTCALL PPMqbClient::InitParam::operator = (const PPMqbClient::InitParam & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL PPMqbClient::InitParam::Copy(const PPMqbClient::InitParam & rS)
{
	LoginParam::Copy(rS);
	Host = rS.Host;
	Port = rS.Port;
	TSCollection_Copy(ConsumeParamList, rS.ConsumeParamList);
	return 1;
}

int FASTCALL PPMqbClient::InitParam::IsEq(const InitParam & rS) const
{
	int    eq = 1;
	if(Host == rS.Host && Port == rS.Port && ConsumeParamList.getCount() == rS.ConsumeParamList.getCount()) {
		for(uint i = 0; eq && ConsumeParamList.getCount(); i++) {
			const RoutingParamEntry * p_rp = ConsumeParamList.at(i);
			const RoutingParamEntry * p_rp2 = rS.ConsumeParamList.at(i);
			if(p_rp && p_rp2)
				eq = p_rp->IsEq(*p_rp2);
			else if((!p_rp && p_rp2) || (p_rp && !p_rp2))
				eq = 0;
		}
	}
	else
		eq = 0;
	return eq;
}

int FASTCALL PPMqbClient::InitParam::IsEqualConnection(const InitParam & rS) const
{
	int    eq = 1;
	if(Host == rS.Host && Port == rS.Port) {
		;
	}
	else
		eq = 0;
	return eq;
}

int STDCALL PPMqbClient::InitParam::SearchRoutingEntry(const RoutingParamEntry & rPattern, uint * pPos) const
{
	for(uint i = 0; i < ConsumeParamList.getCount(); i++) {
		const RoutingParamEntry * p_item = ConsumeParamList.at(i);
		if(p_item && p_item->IsEq(rPattern)) {
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	return 0;
}

PPMqbClient::MessageProperties::MessageProperties() : Flags(0), ContentType(0), Encoding(0), DeliveryMode(0), Priority(0), TimeStamp(ZERODATETIME)
{
}

PPMqbClient::MessageProperties & PPMqbClient::MessageProperties::Z()
{
	Flags = 0;
	ContentType = 0;
	Encoding = 0;
	DeliveryMode = 0;
	Priority = 0;
	TimeStamp = ZERODATETIME;
	CorrelationId.Z();
	ReplyTo.Z();
	Expiration.Z();
	MessageId.Z();
	Type.Z();
	UserId.Z();
	AppId.Z();
	ClusterId.Z();
	Headers.Z();
	return *this;
}

PPMqbClient::Message::Message()
{
}

PPMqbClient::Message & PPMqbClient::Message::Z()
{
	Props.Z();
	Body.Z();
	return *this;
}

PPMqbClient::Envelope::Envelope() : ChannelN(0), Reserve(0), DeliveryTag(0), Flags(0)
{
}

PPMqbClient::Envelope & PPMqbClient::Envelope::Z()
{
	ChannelN = 0;
	Reserve = 0;
	DeliveryTag = 0;
	Flags = 0;
	ConsumerTag.Z();
	Exchange.Z();
	RoutingKey.Z();
	Msg.Z();
	return *this;
}

int FASTCALL PPMqbClient::Envelope::IsReservedRoute(int * pRtRsrv) const
{
	int rt = SIntToSymbTab_GetId(MqbReservedRoutePrefix, SIZEOFARRAY(MqbReservedRoutePrefix), Exchange);
	ASSIGN_PTR(pRtRsrv, rt);
	return rt;
}

PPMqbClient::PPMqbClient() : P_Conn(0), P_Sock(0), Port(0), ChannelN(0), ExtStatusFlags(0)
{
}

PPMqbClient::~PPMqbClient()
{
	Disconnect();
}

void PPMqbClient::SetExtStatusFlag(uint f)
{
	if(f == extsfTryToReconnect) {
		ExtStatusFlags |= f;
	}
}

bool PPMqbClient::GetExtStatusFlag(uint f) const
{
	return LOGIC(ExtStatusFlags & f);
}

bool PPMqbClient::IsHostEqual(const char * pHost, int port) const
{
	return (Host.IsEqiAscii(pHost) && Port == port);
}

int PPMqbClient::Connect(const char * pHost, int port)
{
	int    ok = 1;
	int    amqp_status = 0;
	Disconnect();
	THROW(P_Conn = amqp_new_connection());
	THROW(P_Sock = amqp_tcp_socket_new(GetNativeConnHandle(P_Conn)));
	amqp_status = amqp_socket_open(static_cast<amqp_socket_t *>(P_Sock), pHost, port);
	THROW_SL(SlCheckAmqpError(amqp_status, pHost));
	CATCH
		Disconnect();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPMqbClient::Disconnect()
{
	int    ok = -1;
	if(P_Conn) {
		{
			// @v11.2.2 {
			for(uint i = 0; i < ConsumeTagList.getCount(); i++) {
				const char * p_consume_tag = ConsumeTagList.at(i);
				if(!isempty(p_consume_tag)) {
					Cancel(p_consume_tag, 0);
				}
			}
			// } @v11.2.2 
			// @v11.3.10 {
			if(AutoDeleteDeclaredQueues.getCount()) {
				SString temp_buf;
				AutoDeleteDeclaredQueues.sortAndUndup();
				for(uint ssp = 0; AutoDeleteDeclaredQueues.get(&ssp, temp_buf);) {
					QueueDelete(temp_buf, 0);
				}
			}
			// } @v11.3.10
		}
		if(ChannelN) {
			amqp_channel_close(GetNativeConnHandle(P_Conn), ChannelN, AMQP_REPLY_SUCCESS);
			ChannelN = 0;
		}
		amqp_connection_close(GetNativeConnHandle(P_Conn), AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(GetNativeConnHandle(P_Conn));
		P_Conn = 0;
		ok = 1;
	}
	return ok;
}

/*static*/int FASTCALL PPMqbClient::ProcessAmqpRpcReply(const amqp_rpc_reply_t & rR)
{
	int    ok = 1;
	switch(rR.reply_type) {
		case AMQP_RESPONSE_NORMAL: 
			break;
		case AMQP_RESPONSE_NONE: 
			ok = PPSetError(PPERR_MQBC_NORESPONSE);
			break;
		case AMQP_RESPONSE_LIBRARY_EXCEPTION: 
			ok = PPSetError(PPERR_MQBC_LIBRARYEXCEPTION);
			break;
		case AMQP_RESPONSE_SERVER_EXCEPTION: 
			{
				switch(rR.reply.id) {
					case AMQP_CONNECTION_CLOSE_METHOD:
						{
							SString msg_buf;
							const amqp_connection_close_t * p_detail = (const amqp_connection_close_t *)rR.reply.decoded;
							if(p_detail && p_detail->reply_text.len > 0 && p_detail->reply_text.bytes)
								msg_buf.CatN(static_cast<const char *>(p_detail->reply_text.bytes), p_detail->reply_text.len);
							ok = PPSetError(PPERR_MQBC_SERVER, msg_buf);
						}
						break;
					case AMQP_CHANNEL_CLOSE_METHOD: 
						{
							SString msg_buf;
							const amqp_channel_close_t * p_detail = (const amqp_channel_close_t *)rR.reply.decoded;
							if(p_detail && p_detail->reply_text.len > 0 && p_detail->reply_text.bytes)
								msg_buf.CatN(static_cast<const char *>(p_detail->reply_text.bytes), p_detail->reply_text.len);
							ok = PPSetError(PPERR_MQBC_SERVER, msg_buf);
						}
						break;
					default: 
						ok = PPSetError(PPERR_MQBC_SERVER);
						break;
				}
			}
			break;
	}
	return ok;
}

int PPMqbClient::VerifyRpcReply()
{
	amqp_rpc_reply_t amqp_reply = amqp_get_rpc_reply(GetNativeConnHandle(P_Conn));
	return ProcessAmqpRpcReply(amqp_reply);
}

int PPMqbClient::Login(const LoginParam & rP)
{
	int    ok = 1;
	if(P_Conn) {
		const int default_hartbeat = 60; // sec // @erik param heartbeats: 0 ==> 10 // @v11.1.6 10-->60
		amqp_rpc_reply_t amqp_reply = amqp_login(GetNativeConnHandle(P_Conn), 
			rP.GetVHost(), 0, 131072, default_hartbeat, AMQP_SASL_METHOD_PLAIN, rP.Auth.cptr(), rP.Secret.cptr());   
		assert(amqp_reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION || amqp_reply.library_error == 0);
		THROW_SL(SlCheckAmqpError(amqp_reply.library_error, Host));
		THROW(ProcessAmqpRpcReply(amqp_reply));
		ChannelN = 1;
		amqp_channel_open(GetNativeConnHandle(P_Conn), ChannelN);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

/*static*/long PPMqbClient::SetupMessageTtl(long ttlMs, MessageProperties * pProps)
{
	if(ttlMs > 0) {
		if(pProps)
			pProps->Expiration.Z().Cat(ttlMs);
		return ttlMs;
	}
	else
		return 0;
}

int PPMqbClient::Publish(const char * pExchangeName, const char * pRoutingKey, const MessageProperties * pProps, const void * pData, size_t dataLen)
{
	int    ok = 1;
	amqp_table_entry_t * p_amqp_tbl_entries = 0;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		amqp_basic_properties_t * p_local_props = 0;
		amqp_basic_properties_t local_props;
		amqp_bytes_t exchange = amqp_cstring_bytes(/*"amq.direct"*/pExchangeName);
		amqp_bytes_t routing_key = amqp_cstring_bytes(pRoutingKey);
		amqp_bytes_t data;
		data.bytes = const_cast<void *>(pData); // @badcast
		data.len = dataLen;
		if(pProps) {
			MEMSZERO(local_props);
			local_props._flags = 0;
			if(pProps->ContentType) {
				SString & r_rb = SLS.AcquireRvlStr();
				SFileFormat::GetMime(pProps->ContentType, r_rb);
				local_props.content_type = amqp_cstring_bytes(r_rb);
				local_props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
			}
			if(pProps->Encoding) {
				SString & r_rb = SLS.AcquireRvlStr();
				SFileFormat::GetContentTransferEncName(pProps->Encoding, r_rb);
				local_props.content_encoding = amqp_cstring_bytes(r_rb);
				local_props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
			}
			if(!!pProps->TimeStamp) {
				local_props.timestamp = pProps->TimeStamp.GetTimeT();
				local_props._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
			}
			if(oneof2(pProps->DeliveryMode, 1, 2)) {
				local_props.delivery_mode = pProps->DeliveryMode;
				local_props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
			}
			if(pProps->Priority >= 0 && pProps->Priority <= 9) {
				local_props.priority = pProps->Priority;
				local_props._flags |= AMQP_BASIC_PRIORITY_FLAG;
			}
			if(pProps->CorrelationId.NotEmpty()) {
				local_props.correlation_id = amqp_cstring_bytes(pProps->CorrelationId);
				local_props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
			}
			if(pProps->ReplyTo.NotEmpty()) {
				local_props.reply_to = amqp_cstring_bytes(pProps->ReplyTo);
				local_props._flags |= AMQP_BASIC_REPLY_TO_FLAG;
			}
			if(pProps->Expiration.NotEmpty()) {
				local_props.expiration = amqp_cstring_bytes(pProps->Expiration);
				local_props._flags |= AMQP_BASIC_EXPIRATION_FLAG;
			}
			if(pProps->MessageId.NotEmpty()) {
				local_props.message_id = amqp_cstring_bytes(pProps->MessageId);
				local_props._flags |= AMQP_BASIC_MESSAGE_ID_FLAG;
			}
			if(pProps->Type.NotEmpty()) {
				local_props.type = amqp_cstring_bytes(pProps->Type);
				local_props._flags |= AMQP_BASIC_TYPE_FLAG;
			}
			if(pProps->UserId.NotEmpty()) {
				local_props.user_id = amqp_cstring_bytes(pProps->UserId);
				local_props._flags |= AMQP_BASIC_USER_ID_FLAG;
			}
			if(pProps->AppId.NotEmpty()) {
				local_props.app_id = amqp_cstring_bytes(pProps->AppId);
				local_props._flags |= AMQP_BASIC_APP_ID_FLAG;
			}
			if(pProps->ClusterId.NotEmpty()) {
				local_props.cluster_id = amqp_cstring_bytes(pProps->ClusterId);
				local_props._flags |= AMQP_BASIC_CLUSTER_ID_FLAG;
			}
			if(pProps->Headers.getCount()) {
				THROW_SL(p_amqp_tbl_entries = new amqp_table_entry_t[pProps->Headers.getCount()]);
				local_props.headers.entries = p_amqp_tbl_entries;
				local_props.headers.num_entries = pProps->Headers.getCount();
				for(uint pidx = 0; pidx < pProps->Headers.getCount(); pidx++) {
					SStrToStrAssoc pitem = pProps->Headers.at(pidx);
					p_amqp_tbl_entries[pidx].key = amqp_cstring_bytes(pitem.Key);
					p_amqp_tbl_entries[pidx].value.kind = AMQP_FIELD_KIND_UTF8; // AMQP_FIELD_KIND_BYTES
					p_amqp_tbl_entries[pidx].value.value.bytes = amqp_cstring_bytes(pitem.Val);
				}
				local_props._flags |= AMQP_BASIC_HEADERS_FLAG;
			}
			p_local_props = &local_props;
		}
		int pr = amqp_basic_publish(GetNativeConnHandle(P_Conn), ChannelN, exchange, routing_key, 0, 0, p_local_props, data);
		{ // @debug
			SString log_buf;
			log_buf.Cat("amqp-publish").Space().Cat(pExchangeName).Space().Cat(pRoutingKey);
			if(pProps->CorrelationId.NotEmpty())
				log_buf.Space().Cat(pProps->CorrelationId);
			PPLogMessage(PPFILNAM_DEBUG_LOG, log_buf, LOGMSGF_TIME);
		}
		THROW_SL(SlCheckAmqpError(pr, Host));
	}
	CATCHZOK
	delete [] p_amqp_tbl_entries;
	return ok;
}

static void FASTCALL AmpqBytesToSString(const amqp_bytes_t & rS, SString & rDest)
{
	rDest.Z();
	if(rS.len && rS.bytes)
		rDest.CatN(static_cast<const char *>(rS.bytes), rS.len);
}

int PPMqbClient::Consume(const char * pQueue, SString * pConsumerTag, long consumeFlags)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_bytes_t consumer_tag = amqp_cstring_bytes((pConsumerTag && pConsumerTag->Len()) ? pConsumerTag->cptr() : 0);
		amqp_basic_consume_ok_t * p_bco = amqp_basic_consume(GetNativeConnHandle(P_Conn), ChannelN, queue, consumer_tag,
			BIN(consumeFlags & mqofNoLocal), BIN(consumeFlags & mqofNoAck), BIN(consumeFlags & mqofExclusive), amqp_empty_table);
		THROW(VerifyRpcReply());
		{
			SString & r_temp_buf = SLS.AcquireRvlStr();
			AmpqBytesToSString(p_bco->consumer_tag, r_temp_buf);
			if(pConsumerTag) {
				*pConsumerTag = r_temp_buf;
			}
			ConsumeTagList.insert(newStr(r_temp_buf));
		}
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::Cancel(const char * pConsumerTag, long flags)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	THROW(sstrlen(pConsumerTag));
	{
		amqp_bytes_t consumer_tag = amqp_cstring_bytes(pConsumerTag);
		amqp_basic_cancel_ok_t * p_bco = amqp_basic_cancel(GetNativeConnHandle(P_Conn), ChannelN, consumer_tag);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::WaitForMessage(Envelope & rEnv, uint maxTimeMs, uint pollTimeQuantMs)
{
	int    ok = -1;
	const clock_t cs = clock();
	do {
		ok = ConsumeMessage(rEnv, 100);
		if(ok < 0) {
			SDelay(pollTimeQuantMs);
		}
	} while(ok < 0 && (clock() - cs) < static_cast<long>(maxTimeMs));
	return ok;
}

int PPMqbClient::ConsumeMessage(Envelope & rEnv, long timeoutMs)
{
	rEnv.Z();
	int    ok = -1;
	SString temp_buf;
	SString val_buf;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		amqp_envelope_t envelope;
		struct timeval tv;
		if(timeoutMs >= 0) {
			tv.tv_sec = timeoutMs / 1000;
			tv.tv_usec = (timeoutMs % 1000) * 1000;
		}
		amqp_rpc_reply_t r = amqp_consume_message(GetNativeConnHandle(P_Conn), &envelope, (timeoutMs >= 0) ? &tv : 0, 0);
		if(r.reply_type == AMQP_RESPONSE_NORMAL) {
			rEnv.ChannelN = envelope.channel;
			rEnv.DeliveryTag = envelope.delivery_tag;
			SETFLAG(rEnv.Flags, rEnv.fRedelivered, envelope.redelivered);
			AmpqBytesToSString(envelope.consumer_tag, rEnv.ConsumerTag);
			AmpqBytesToSString(envelope.exchange, rEnv.Exchange);
			AmpqBytesToSString(envelope.routing_key, rEnv.RoutingKey);
			rEnv.Msg.Props.Flags = envelope.message.properties._flags;
			rEnv.Msg.Props.DeliveryMode = envelope.message.properties.delivery_mode;
			rEnv.Msg.Props.Priority = envelope.message.properties.priority;
			rEnv.Msg.Props.TimeStamp.SetTimeT(static_cast<time_t>(envelope.message.properties.timestamp));
			AmpqBytesToSString(envelope.message.properties.content_type, temp_buf);
			{
				SFileFormat ft;
				if(ft.IdentifyMime(temp_buf))
					rEnv.Msg.Props.ContentType = ft;
			}
			AmpqBytesToSString(envelope.message.properties.content_encoding, temp_buf);
			rEnv.Msg.Props.Encoding = SFileFormat::IdentifyMimeType(temp_buf);
			// @todo amqp_table_t headers --> rEnv.Msg.Props.Headers
			{
				const amqp_table_t & r_tbl = envelope.message.properties.headers;
				if(r_tbl.num_entries && r_tbl.entries) {
					for(int i = 0; i < r_tbl.num_entries; i++) {
						const amqp_table_entry_t & r_te = r_tbl.entries[i];
						AmpqBytesToSString(r_te.key, temp_buf);
						val_buf.Z();
						switch(r_te.value.kind) {
							case AMQP_FIELD_KIND_BOOLEAN: val_buf.Cat(STextConst::GetBool(r_te.value.value.boolean)); break;
							case AMQP_FIELD_KIND_I8:  val_buf.Cat(r_te.value.value.i8); break;
							case AMQP_FIELD_KIND_U8:  val_buf.Cat(r_te.value.value.u8); break;
							case AMQP_FIELD_KIND_I16: val_buf.Cat(r_te.value.value.i16); break;
							case AMQP_FIELD_KIND_U16: val_buf.Cat(r_te.value.value.u16); break;
							case AMQP_FIELD_KIND_I32: val_buf.Cat(r_te.value.value.i32); break;
							case AMQP_FIELD_KIND_U32: val_buf.Cat(r_te.value.value.u32); break;
							case AMQP_FIELD_KIND_I64: val_buf.Cat(r_te.value.value.i64); break;
							case AMQP_FIELD_KIND_U64: val_buf.Cat(r_te.value.value.u64); break;
							case AMQP_FIELD_KIND_F32: val_buf.Cat(r_te.value.value.f32, MKSFMTD(0,  8, 0)); break;
							case AMQP_FIELD_KIND_F64: val_buf.Cat(r_te.value.value.f64, MKSFMTD(0, 10, 0)); break;
							case AMQP_FIELD_KIND_DECIMAL: break; // @todo
							case AMQP_FIELD_KIND_UTF8: AmpqBytesToSString(r_te.value.value.bytes, val_buf); break;
							case AMQP_FIELD_KIND_ARRAY: break; // @todo
							case AMQP_FIELD_KIND_TIMESTAMP: break; // @todo
							case AMQP_FIELD_KIND_TABLE: break; // @todo
							case AMQP_FIELD_KIND_VOID: break; // @todo
							case AMQP_FIELD_KIND_BYTES: break; // @todo
						}
						rEnv.Msg.Props.Headers.Add(temp_buf, val_buf);
					}
				}
			}
			AmpqBytesToSString(envelope.message.properties.correlation_id, rEnv.Msg.Props.CorrelationId);
			AmpqBytesToSString(envelope.message.properties.reply_to, rEnv.Msg.Props.ReplyTo);
			AmpqBytesToSString(envelope.message.properties.expiration, rEnv.Msg.Props.Expiration);
			AmpqBytesToSString(envelope.message.properties.message_id, rEnv.Msg.Props.MessageId);
			AmpqBytesToSString(envelope.message.properties.type, rEnv.Msg.Props.Type);
			AmpqBytesToSString(envelope.message.properties.user_id, rEnv.Msg.Props.UserId);
			AmpqBytesToSString(envelope.message.properties.app_id, rEnv.Msg.Props.AppId);
			AmpqBytesToSString(envelope.message.properties.cluster_id, rEnv.Msg.Props.ClusterId);
			rEnv.Msg.Body.Write(envelope.message.body.bytes, envelope.message.body.len);
			ok = 1;
		}
		else if(r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
			if(r.library_error == AMQP_STATUS_TIMEOUT) {
				PPSetError(PPERR_MQBC_CONSUMETIMEOUT, timeoutMs); // @v11.1.6
				ok = -1;
			}
			else {
				THROW_SL(SlCheckAmqpError(r.library_error, Host));
			}
		}
		amqp_destroy_envelope(&envelope);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::Ack(uint64 deliveryTag, long flags /*mqofMultiple*/)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		int pr = amqp_basic_ack(GetNativeConnHandle(P_Conn), ChannelN, deliveryTag, BIN(flags & mqofMultiple));
		THROW(pr >= AMQP_STATUS_OK);
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::Reject(uint64 deliveryTag, long flags /*mqofRequeue*/)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		int pr = amqp_basic_reject(GetNativeConnHandle(P_Conn), ChannelN, deliveryTag, BIN(flags & mqofRequeue));
		THROW(pr >= AMQP_STATUS_OK);
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::DeclarePredefinedExchanges()
{
	int    ok = -1;
	struct ReservedExchangeEntry {
		const  char * P_Name;
		long   Type;
		long   Flags;
	};
	static const ReservedExchangeEntry ree_list[] = {
		{ "papyrusdbx", exgtDirect, 0 },
		{ "papyrusposprotocol", exgtDirect, 0 },
		{ "papyrusstyloview", exgtDirect, 0 },
		{ "papyrusrpc", exgtDirect, 0 },
		{ "papyrusrpcreply", exgtDirect, 0 },
		{ "styloqrpc", exgtDirect, 0 },
	};
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		SString fmt_buf;
		SString msg_buf;
		SString temp_buf;
		for(uint i = 0; i < SIZEOFARRAY(ree_list); i++) {
			const ReservedExchangeEntry & r_ree = ree_list[i];
			temp_buf.Z().Cat(r_ree.P_Name).CatDiv(';', 2).CatEq("type", r_ree.Type).CatDiv(';', 2).CatEq("flags", r_ree.Flags);
			msg_buf.Z();
			if(ExchangeDeclare(r_ree.P_Name, r_ree.Type, r_ree.Flags)) {
				PPLoadText(PPTXT_MQB_PREDEFEXCHREGISTERED, fmt_buf);
				msg_buf.Printf(fmt_buf, temp_buf.cptr());
			}
			else {
				PPLoadText(PPTXT_MQB_PREDEFEXCHREGFAULT, fmt_buf);
				msg_buf.Printf(fmt_buf, temp_buf.cptr());
			}
			if(msg_buf.NotEmpty())
				PPLogMessage(PPFILNAM_INFO_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_DBINFO|LOGMSGF_USER);
		}
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::ApplyRoutingParamEntry(const RoutingParamEntry & rP)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	if(rP.QueueName.NotEmpty()) {
		//if(rP.RtRsrv != rtrsrvRpcReply && !(rP.PreprocessFlags & RoutingParamEntry::ppfSkipQueueDeclaration)) { // ������� ���� ��������������� �����������
			THROW(QueueDeclare(rP.QueueName, rP.QueueFlags));
			if(rP.ExchangeName.NotEmpty()) {
				THROW(ExchangeDeclare(rP.ExchangeName, rP.ExchangeType, rP.ExchangeFlags));
				THROW(QueueBind(rP.QueueName, rP.ExchangeName, rP.RoutingKey));
			}
		//}
		/*if(rP.RtRsrv == rtrsrvRpc) {
			if(rP.RpcReplyQueueName.NotEmpty()) {
				THROW(QueueDeclare(rP.RpcReplyQueueName, rP.RpcReplyQueueFlags));
				if(rP.RpcReplyExchangeName.NotEmpty()) {
					//THROW(ExchangeDeclare(rP.RpcReplyExchangeName, rP.RpcReplyExchangeType, rP.RpcReplyExchangeFlags));
					THROW(QueueBind(rP.RpcReplyQueueName, rP.RpcReplyExchangeName, rP.RpcReplyRoutingKey));
				}
			}
		}*/
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPMqbClient::QueueDeclare(const char * pQueue, long queueFlags)
{
	int    ok = 1;
	THROW_PP(!isempty(pQueue), PPERR_MQBC_EMPTYQUEUENAME);
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_queue_declare_ok_t * p_qdo = amqp_queue_declare(GetNativeConnHandle(P_Conn), ChannelN, queue, 
			BIN(queueFlags & mqofPassive), BIN(queueFlags & mqofDurable), BIN(queueFlags & mqofExclusive), BIN(queueFlags & mqofAutoDelete), amqp_empty_table);
		THROW(VerifyRpcReply());
		// @v11.3.10 {
		if(queueFlags & mqofAutoDelete) {
			AutoDeleteDeclaredQueues.add(pQueue);
		}
		// } @v11.3.10 
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::ExchangeDeclare(const char * pExchange, int type /* exgtXXX */, long exchangeFlags)
{
	int    ok = 1;
	THROW_PP(P_Conn, PPERR_MQBC_NOTINITED);
	{
		const char * p_exchange_type = 0;
		switch(type) {
			case exgtDirect: p_exchange_type = "direct"; break;
			case exgtFanout: p_exchange_type = "fanout"; break;
			case exgtTopic: p_exchange_type = "topic"; break;
			case exgtHeaders: p_exchange_type = "headers"; break;
			case exgtDefault: break;
		}
		amqp_bytes_t exchange = amqp_cstring_bytes(pExchange);
		amqp_bytes_t type = amqp_cstring_bytes(p_exchange_type);
		amqp_exchange_declare_ok_t * p_ok = amqp_exchange_declare(GetNativeConnHandle(P_Conn), ChannelN, exchange, type,
			BIN(exchangeFlags & mqofPassive), BIN(exchangeFlags & mqofDurable), BIN(exchangeFlags & mqofAutoDelete), 
			BIN(exchangeFlags & mqofInternal), amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::QueueBind(const char * pQueue, const char * pExchange, const char * pRoutingKey)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_bytes_t exchange = amqp_cstring_bytes(pExchange);
		amqp_bytes_t routing_key = amqp_cstring_bytes(pRoutingKey);
		amqp_queue_bind_ok_t * p_ok = amqp_queue_bind(GetNativeConnHandle(P_Conn), ChannelN, queue, exchange, routing_key, amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::QueueUnbind(const char * pQueue, const char * pExchange, const char * pRoutingKey)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_bytes_t exchange = amqp_cstring_bytes(pExchange);
		amqp_bytes_t routing_key = amqp_cstring_bytes(pRoutingKey);
		amqp_queue_unbind_ok_t * p_ok = amqp_queue_unbind(GetNativeConnHandle(P_Conn), ChannelN, queue, exchange, routing_key, amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int PPMqbClient::QueueDelete(const char * pQueue, long queueFlags)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		boolint if_unused = 0;
		boolint if_empty = 0;
		amqp_queue_delete_ok_t * p_ok = amqp_queue_delete(GetNativeConnHandle(P_Conn), ChannelN, queue, if_unused, if_empty);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

/*static*/int PPMqbClient::InitClient(PPMqbClient & rC, const PPMqbClient::InitParam & rP)
{
	int    ok = 1;
	// @v11.2.12 {
	SString host(rP.Host);
	InetUrl url(host);
	const  int prot = url.GetProtocol();
	THROW(oneof3(prot, 0, InetUrl::protAMQP, InetUrl::protAMQPS));
	if(prot) {
		url.SetProtocol(0);
		url.Compose(0, host);
	}
	// } @v11.2.12 
	THROW(rC.Connect(host, NZOR(rP.Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
	THROW(rC.Login(rP));
	rC.Host = rP.Host; // @v11.2.12
	rC.Port = rP.Port; // @v11.2.12
	if(rP.ConsumeParamList.getCount()) {
		for(uint i = 0; i < rP.ConsumeParamList.getCount(); i++) {
			const RoutingParamEntry * p_entry = rP.ConsumeParamList.at(i);
			if(p_entry) {
				THROW(rC.ApplyRoutingParamEntry(*p_entry));
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPMqbClient::SetupInitParam(PPMqbClient::InitParam & rP, PPAlbatrossConfig & rACfg, const char * pVHost, SString * pDomain)
{
	int    ok = 1;
	SString data_domain;
	SString temp_buf;
	{
		rACfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
		rP.Host = temp_buf;
		rACfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
		rP.Auth = temp_buf;
		rACfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
		rP.Secret = temp_buf;
		rACfg.GetExtStrData(ALBATROSEXSTR_MQC_DATADOMAIN, data_domain);
		rP.Method = 1;
	}
	if(!isempty(pVHost)) {
		rP.VHost = pVHost;
	}
	THROW_PP(!pDomain || data_domain.NotEmpty(), PPERR_GLOBALDATADOMAINUNDEF);
	CATCHZOK
	ASSIGN_PTR(pDomain, data_domain);
	return ok;
}

/*static*/int PPMqbClient::SetupInitParam(PPMqbClient::InitParam & rP, const char * pVHost, SString * pDomain)
{
	int    ok = 1;
	PPAlbatrossConfig acfg;
	THROW(DS.FetchAlbatrosConfig(&acfg) > 0);
	THROW(SetupInitParam(rP, acfg, pVHost, pDomain));
	CATCHZOK
	return ok;
}

/*static*/int PPMqbClient::InitClient(PPMqbClient & rC, SString * pDomain)
{
	int    ok = 1;
	PPMqbClient::InitParam lp;
	THROW(SetupInitParam(lp, 0/*vhost: will be default*/, pDomain));
	THROW(PPMqbClient::InitClient(rC, lp));
	//THROW(rC.Connect(lp.Host, NZOR(lp.Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
	THROW(rC.Connect(lp.Host, NZOR(lp.Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
	THROW(rC.Login(lp));
	CATCHZOK
	return ok;
}

/*static*/PPMqbClient * PPMqbClient::CreateInstance(const PPMqbClient::InitParam & rP)
{
	PPMqbClient * p_cli = 0;
	THROW_PP(rP.Host.NotEmpty()/*&& rP.ConsumeParamList.getCount()*/, PPERR_MQBC_HOSTEMPTY);
	THROW_SL(p_cli = new PPMqbClient);
	THROW(PPMqbClient::InitClient(*p_cli, rP));
	for(uint i = 0; i < rP.ConsumeParamList.getCount(); i++) {
		/*const*/PPMqbClient::RoutingParamEntry * p_rpe = rP.ConsumeParamList.at(i);
		p_cli->Consume(p_rpe->QueueName, &p_rpe->ConsumeTag.Z(), 0);
	}
	CATCH
		ZDELETE(p_cli);
	ENDCATCH
	return p_cli;
}

// @v10.6.1 (moved to PPConst) static const uint32 MqbEventResponder_Signature = 0xB4C7E6F1;

MqbEventResponder::MqbEventResponder() : Signature(PPConst::Signature_MqbEventResponder), AdvCookie_Msg(0), P_Cli(0)
{
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evMqbMessage;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = MqbEventResponder::AdviseCallback;
		DS.Advise(&AdvCookie_Msg, &adv_blk);
	}
}

MqbEventResponder::~MqbEventResponder()
{
	DS.Unadvise(AdvCookie_Msg);
	delete P_Cli;
	Signature = 0;
}
	
bool MqbEventResponder::IsConsistent() const { return (Signature == PPConst::Signature_MqbEventResponder); }

MqbEventResponder::Command::Command() : Cmd(cmdNone), IdVal(0)
{
}

int MqbEventResponder::ParseCommand(const char * pCmdText, Command & rCmd) const
{
	static const SIntToSymbTabEntry cmd_prefix_list[] = {
		{ cmdGetGlobalAccountList, "GetGlobalAccountList" },
		{ cmdVerifyGlobalAccount, "VerifyGlobalAccount" } // VerifyGlobalAccount GUA_ID MIME64_HASH
	};
	int    ok = 0;
	const size_t cmd_len = sstrlen(pCmdText);
	PPTextCommandBlock cmd_blk;
	size_t p = 0;
	if(cmd_blk.GetWord(pCmdText, &p)) {
		int cmd_id = SIntToSymbTab_GetId(cmd_prefix_list, SIZEOFARRAY(cmd_prefix_list), cmd_blk.Term);
		if(cmd_id) {
			rCmd.Cmd = cmd_id;
			if(cmd_id == cmdGetGlobalAccountList) {
				ok = 1;
			}
			else if(cmd_id == cmdVerifyGlobalAccount) {
				if(cmd_blk.GetWord(pCmdText, &p)) {
					rCmd.IdVal = cmd_blk.Term.ToLong();
					if(cmd_blk.GetWord(pCmdText, &p)) {
						rCmd.Hash = cmd_blk.Term;
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

/*static*/int MqbEventResponder::AdviseCallback(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
{
	int    ok = 1;
	SString temp_buf;
	int compulsory_response_flag = 0;
	if(kind == PPAdviseBlock::evMqbMessage) {
		MqbEventResponder * p_self = static_cast<MqbEventResponder *>(procExtPtr);
		PPMqbClient::Envelope * p_env = (p_self && pEv && pEv->P_MqbEnv) ? pEv->P_MqbEnv : 0;
		if(p_env) {
			if(pEv->Action == PPEVNT_MQB_MESSAGE) {
				{
					temp_buf.Z().Cat("Message has been consumed").CatDiv(':', 2).
						CatEq("Timestamp", p_env->Msg.Props.TimeStamp, DATF_ISO8601, 0).
						Space().CatEq("Exchange", p_env->Exchange).
						Space().CatEq("RoutingKey", p_env->RoutingKey).
						Space().CatEq("ReplyTo", p_env->Msg.Props.DeliveryMode);
					PPLogMessage(PPFILNAM_INFO_LOG, temp_buf, LOGMSGF_COMP|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO|LOGMSGF_SLSSESSGUID);
				}
				p_env->RoutingKey;
				int rt = p_env->IsReservedRoute(0);
				if(rt) {
					const SBuffer & r_body = p_env->Msg.Body;
					if(rt == PPMqbClient::rtrsrvPapyrusDbx) {
						ObjTransmMqProps otmp;
						if(r_body.GetAvailableSize() && otmp.GetFromMqbMessage(p_env->Msg.Props) > 0) {
							if(otmp.FileName.NotEmpty() && otmp.SrcDbDivID && otmp.DestDbDivID && otmp.DestDbDivID == LConfig.DBDiv) {
								SString dest_path;
								PPGetFilePath(PPPATH_IN, otmp.FileName, dest_path);
								int   wr_success = 1;
								{
									SFile f_out(dest_path, SFile::mWrite|SFile::mBinary);
									if(!f_out.IsValid())
										wr_success = 0;
									else if(!f_out.Write(r_body.constptr(), r_body.GetAvailableSize()))
										wr_success = 0;
								}
								if(wr_success) {
									ObjReceiveParam param;
									param.Flags = (ObjReceiveParam::fNonInteractive|ObjReceiveParam::fCommitQueue|ObjReceiveParam::fDisableLogWindow);
									param.SsOnlyFileNames.add(dest_path);
									PPObjectTransmit::ReceivePackets(&param);
								}
							}
						}
					}
					else if(rt == PPMqbClient::rtrsrvPapyrusPosProtocol) {
					}
					else if(rt == PPMqbClient::rtrsrvStyloView) {
					}
					else if(rt == PPMqbClient::rtrsrvRpc) {
						SString result_buf;						
						SString login;
						int   result_ok = 0;
						if(r_body.GetAvailableSize()) {
							temp_buf.Z().CatN(static_cast<const char *>(r_body.GetBuf(r_body.GetRdOffs())), r_body.GetAvailableSize());
							Command cmd;						
							if(p_self->ParseCommand(temp_buf, cmd)) {
								if(cmd.Cmd == cmdGetGlobalAccountList) {
									PPObjGlobalUserAcc gua_obj;
									PPGlobalUserAccPacket gua_pack;
									PPGlobalUserAcc gua_rec;
									StrAssocArray obj_name_list;
									for(SEnum en = gua_obj.Enum(0); en.Next(&gua_rec) > 0;) {
										if(gua_rec.ServiceIdent==PPGLS_UNDEF) {
											if(gua_obj.GetPacket(gua_rec.ID, &gua_pack)>0) {
												if(gua_pack.TagL.GetItemStr(PPTAG_GUA_LOGIN, login.Z())>0) {
													obj_name_list.Add(gua_rec.ID, login.Transf(CTRANSF_INNER_TO_UTF8));
												}
												else {
													obj_name_list.Add(gua_rec.ID, (login = gua_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
												}
											}
										}
									}
									PPExportDL600DataToBuffer("StrAssocArray", &obj_name_list, cpUTF8, result_buf);
									result_ok = 1;
								}
								//@erik v10.5.9 {
								else if(cmd.Cmd == cmdVerifyGlobalAccount) {
									compulsory_response_flag = 1;
									PPObjGlobalUserAcc gua_obj;
									PPGlobalUserAccPacket gua_pack;
									StrAssocArray obj_gua_guid_list;
									SString password;
									if(gua_obj.GetPacket(cmd.IdVal, &gua_pack) > 0){
										if(gua_pack.TagL.GetItemStr(PPTAG_GUA_SECRET, password) <= 0) {
											Reference::Decrypt(Reference::crymRef2, gua_pack.Rec.Password, sizeof(gua_pack.Rec.Password), password);
										}
										if(gua_pack.TagL.GetItemStr(PPTAG_GUA_LOGIN, login) <= 0) {
											if(gua_pack.Rec.Name != "")
												login = gua_pack.Rec.Name;
										}
										if(login.NotEmpty() && password.NotEmpty()){
											//char hash[128];
											char cmd_hash[128];
											size_t hash_len = 0;
											//PTR32(hash)[0] = 0;
											cmd_hash[0] = 0;
											temp_buf.Z().Cat(login).Colon().Cat(password);
											binary160 hash = SlHash::Sha1(0, temp_buf, temp_buf.Len());
											temp_buf.Z().Cat(cmd.Hash);
											hash_len = 0;
											Base64_Decode(temp_buf, temp_buf.Len(), cmd_hash, &hash_len);
											if(!memcmp(cmd_hash, &hash, hash_len)){												
												temp_buf.Z();
												if(gua_pack.TagL.GetItemStr(PPTAG_GUA_GUID, temp_buf)>0) {
													result_buf = temp_buf;
													result_ok = 1;
												}
											}
										}
									}									
								}
								// } @erik v10.5.9
							}
						}
						if(result_ok) {
							ResponseByAdviseCallback(result_buf, p_env, p_self, temp_buf.Z());
						}
						// @erik v10.6.0{
						else {
							if(compulsory_response_flag) {
								result_buf = "Error";
								ResponseByAdviseCallback(result_buf, p_env, p_self, temp_buf.Z());
							}
						}
						// } @erik v10.6.0 
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}

// @erik v10.5.12 {
int MqbEventResponder::ResponseByAdviseCallback(const SString & rResponseMsg, const PPMqbClient::Envelope * pEnv, MqbEventResponder * pSelf, SString &rDomainBuf)
{
	int    ok = 1;
	if(!pSelf->P_Cli) {
		pSelf->P_Cli = new PPMqbClient;
		if(!PPMqbClient::InitClient(*pSelf->P_Cli, &rDomainBuf))
			ZDELETE(pSelf->P_Cli);
	}
	if(pSelf->P_Cli) {
		PPMqbClient::RoutingParamEntry rpe;
		PPMqbClient::MessageProperties props;
		THROW(rpe.SetupRpcReply(*pEnv));
		props.ContentType = SFileFormat::Xml;
		props.Encoding = SEncodingFormat::Unkn;
		props.Priority = 0;
		props.TimeStamp = getcurdatetime_();
		props.CorrelationId = rpe.CorrelationId;
		THROW(pSelf->P_Cli->ApplyRoutingParamEntry(rpe));
		THROW(pSelf->P_Cli->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, rResponseMsg.cptr(), rResponseMsg.Len()));
		ZDELETE(pSelf->P_Cli); // @erik v10.6.2
	}
	CATCHZOK
	return ok;
}
//  } @erik

class TestMqcDialog : public TDialog {
	static int AdviseProc(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
	{
		TestMqcDialog * p_self = static_cast<TestMqcDialog *>(procExtPtr);
		if(p_self && p_self->IsConsistent()) {
			if(p_self->P_Mqbc) {
				SString temp_buf;
				PPMqbClient::Envelope env;
				uint iteridx = 0;
				int cmr = p_self->P_Mqbc->ConsumeMessage(env, 25);
				if(cmr > 0) {
					temp_buf = env.ConsumerTag;
					p_self->P_Mqbc->Cancel(env.ConsumerTag, 0);
					p_self->setCtrlString(CTL_TESTMQC_RESULT, temp_buf);
				}
			}
		}
		return 1;
	}
	long  AdvCookie;
	PPMqbClient * P_Mqbc;
	SString DataDomain;
public:
	TestMqcDialog() : TDialog(DLG_TESTMQC), P_Mqbc(0), AdvCookie(0)
	{
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evQuartz;
		adv_blk.ProcExtPtr = this;
		adv_blk.Proc = AdviseProc;
		DS.Advise(&AdvCookie, &adv_blk);
	}
	~TestMqcDialog()
	{
		DS.Unadvise(AdvCookie);
		ZDELETE(P_Mqbc);
	}
private:
	DECL_HANDLE_EVENT
	{
		if(event.isCmd(cmOK)) {
			if(Run() >= 0)
				clearEvent(event);
		}
		TDialog::handleEvent(event);
	}
	int Run()
	{
		int    ok = 1;
		SString cmd_buf;
		getCtrlString(CTL_TESTMQC_CMD, cmd_buf);
		if(cmd_buf.NotEmptyS()) {
			SString temp_buf;
			(temp_buf = "cmd").Space().Cat(cmd_buf);
			setCtrlString(CTL_TESTMQC_RESULT, temp_buf);
			if(!P_Mqbc) {
				P_Mqbc = new PPMqbClient;
				if(P_Mqbc) {
					if(!PPMqbClient::InitClient(*P_Mqbc, &DataDomain)) {
						ZDELETE(P_Mqbc);
					}
				}
			}
			if(P_Mqbc) {
				PPMqbClient::RoutingParamEntry rpe;
				PPMqbClient::RoutingParamEntry rpe_reply;
				PPMqbClient::MessageProperties props;
				props.ContentType = SFileFormat::TxtUtf8;
				props.Encoding = SEncodingFormat::Unkn;
				props.Priority = 5;
				props.TimeStamp = getcurdatetime_();
				rpe.SetupReserved(PPMqbClient::rtrsrvRpc, DataDomain, 0, 0, &rpe_reply);
				THROW(P_Mqbc->ApplyRoutingParamEntry(rpe));
				THROW(P_Mqbc->ApplyRoutingParamEntry(rpe_reply));
				props.CorrelationId = rpe.CorrelationId;
				props.ReplyTo = rpe_reply.QueueName;
				THROW(P_Mqbc->Publish(rpe.ExchangeName, rpe.RoutingKey, &props, cmd_buf, cmd_buf.Len()));
				{
					// GetGlobalAccountList
					SString consume_tag;
					THROW(P_Mqbc->Consume(rpe_reply.QueueName, &consume_tag, 0));
				}
			}
		}
		else
			ok = -1;
		CATCH
			ok = 0;
		ENDCATCH
		return ok;
	}
};

int TestMqc()
{
	int    ok = -1;
	TestMqcDialog * dlg = new TestMqcDialog;
	if(CheckDialogPtr(&dlg)) {
		ExecViewAndDestroy(dlg);
		dlg = 0;
	}
	return ok;
}
//
//
//
#if 0 // {

static const char * P_TestQueueName = "test-papyrus-queue";

class TestMqcThread : public PPThread {
public:
	TestMqcThread(const PPMqbClient::InitParam & rP) : PPThread(kUnknown, "", 0), MqbcParam(rP)
	{
	}
	virtual void Startup()
	{
		PPThread::Startup();
		SignalStartup();
	}
protected:
	int    Helper_InitRun(PPMqbClient & rCli)
	{
		int    ok = 1;
		THROW(PPMqbClient::InitClient(rCli, MqbcParam));
		CATCHZOK
		return ok;
	}
	PPMqbClient::InitParam MqbcParam;
};

class TestMqcProducer : public TestMqcThread {
public:
	TestMqcProducer(const PPMqbClient::InitParam & rP, const char * pQueueName) : TestMqcThread(rP), QueueName(pQueueName)
	{
	}
	virtual void Run()
	{
		const char * p_exchange_name = "papyrus-exchange-fanout-test";
		int    ok = 1;
		PPMqbClient mqc;
		SString log_file_name;
		PPGetFilePath(PPPATH_LOG, "TestMqcProducer.log", log_file_name);
		if(Helper_InitRun(mqc)) {
			int    stop = 0;
			SString temp_buf;
			SString msg_buf;
			PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("Helper_InitRun"), LOGMSGF_TIME|LOGMSGF_DBINFO);
			{
				THROW(mqc.QueueDeclare(QueueName, 0));
				PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("QueueDeclare"), LOGMSGF_TIME|LOGMSGF_DBINFO);
				THROW(mqc.ExchangeDeclare(p_exchange_name, mqc.exgtFanout, 0));
				PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("ExchangeDeclare"), LOGMSGF_TIME|LOGMSGF_DBINFO);
				THROW(mqc.QueueBind(QueueName, p_exchange_name, ""));
				PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("QueueBind"), LOGMSGF_TIME|LOGMSGF_DBINFO);
			}
			SDelay(1000);
			do {
				ulong random_id = SLS.GetTLA().Rg.GetUniformInt(1000000);
				(temp_buf = "This is a some stupid message").CatChar('-').Cat(random_id);
				//THROW(mqc.Publish("", P_TestQueueName, temp_buf.cptr(), temp_buf.Len()));
				PPMqbClient::MessageProperties props;
				if(mqc.Publish(p_exchange_name, /*P_TestQueueName*/"", 0 /*props*/, temp_buf.cptr(), temp_buf.Len())) {
					PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("Publish"), LOGMSGF_TIME|LOGMSGF_DBINFO);
					SDelay(100);
				}
				else {
					PPLogMessage(log_file_name, msg_buf.Z().Cat("ERR").CatDiv(':', 2).Cat("Publish"), LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
			} while(!stop);
		}
		CATCHZOK
	}
	const SString QueueName;
};

class TestMqcConsumer : public TestMqcThread {
public:
	TestMqcConsumer(const PPMqbClient::InitParam & rP, const char * pQueueName) : TestMqcThread(rP), QueueName(pQueueName)
	{
	}
	virtual void Run()
	{
		int    ok = 1;
		PPMqbClient mqc;
		SString log_file_name;
		PPGetFilePath(PPPATH_LOG, "TestMqcConsumer.log", log_file_name);
		if(Helper_InitRun(mqc)) {
			int    stop = 0;
			SString consume_tag;
			SString msg_buf;
			PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("Helper_InitRun"), LOGMSGF_TIME|LOGMSGF_DBINFO);
			THROW(mqc.QueueDeclare(QueueName, 0));
			PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("QueueDeclare"), LOGMSGF_TIME|LOGMSGF_DBINFO);
			THROW(mqc.Consume(QueueName, &consume_tag, 0));
			PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("Consume"), LOGMSGF_TIME|LOGMSGF_DBINFO);
			SDelay(1500);
			do {
				PPMqbClient::Envelope env;
				int cmr = mqc.ConsumeMessage(env, 10);
				if(cmr > 0) {
					PPLogMessage(log_file_name, msg_buf.Z().Cat("OK").CatDiv(':', 2).Cat("ConsumeMessage"), LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
				else if(cmr == 0) {
					PPLogMessage(log_file_name, msg_buf.Z().Cat("ERR").CatDiv(':', 2).Cat("ConsumeMessage"), LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
			} while(!stop);
		}
		CATCHZOK
	}
	const SString QueueName;
};

int TestMqc()
{
	int    ok = 1;
	const char * p_exchange_name = "papyrus-exchange-fanout-test";
	SString temp_buf;
	PPMqbClient::InitParam lp;
	//lp.Host = "192.168.0.39";
	//lp.Method = 1;
	//lp.Auth = "Admin";
	//lp.Secret = "123";
	if(0) {
		const char * p_queue_name = "papyrus-test-queue";
		PPMqbClient::InitParam mqbcp;
		if(PPMqbClient::SetupInitParam(mqbcp, 0)) {
			TestMqcProducer * p_thr_mqc_producer = new TestMqcProducer(mqbcp, p_queue_name);
			p_thr_mqc_producer->Start(1);
			TestMqcConsumer * p_thr_mqc_consumer = new TestMqcConsumer(mqbcp, p_queue_name);
		}
	}
	{
		PPMqbClient mqc;
		PPAlbatrossConfig acfg;
		THROW(DS.FetchAlbatrosConfig(&acfg) > 0);
		{
			acfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
			lp.Host = temp_buf;
			acfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
			lp.Auth = temp_buf;
			acfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
			lp.Secret = temp_buf;
			//acfg.GetExtStrData(ALBATROSEXSTR_MQC_DATADOMAIN, temp_buf);
			lp.Method = 1;
		}
		THROW(PPMqbClient::InitClient(mqc, lp));
		{
			THROW(mqc.QueueDeclare(P_TestQueueName, 0));
			THROW(mqc.ExchangeDeclare(p_exchange_name, mqc.exgtFanout, 0));
			THROW(mqc.QueueBind(P_TestQueueName, p_exchange_name, ""));
		}
		{
			ulong random_id = SLS.GetTLA().Rg.GetUniformInt(1000000);
			(temp_buf = "This is a some stupid message").CatChar('-').Cat(random_id);
			//THROW(mqc.Publish("", P_TestQueueName, temp_buf.cptr(), temp_buf.Len()));
			PPMqbClient::MessageProperties props;
			THROW(mqc.Publish(p_exchange_name, /*P_TestQueueName*/"", 0 /*props*/, temp_buf.cptr(), temp_buf.Len()));
		}
	}
	{
		PPMqbClient mqc;
		THROW(PPMqbClient::InitClient(mqc, lp));
		{
			THROW(mqc.QueueDeclare(P_TestQueueName, 0));
		}
		{
			THROW(mqc.Consume(P_TestQueueName, "", 0));
			{
				PPMqbClient::Envelope env;
				int cmr = mqc.ConsumeMessage(env, 5000);
			}
		}
	}
	CATCHZOK
	return ok;
}
#endif // } 0