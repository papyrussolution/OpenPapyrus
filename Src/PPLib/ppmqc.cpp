// PPMQC.CPP
// Copyright (c) A.Sobolev 2019
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

PPMqbClient::InitParam::InitParam() : Port(0)
{
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
	Headers.Clear();
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

SLAPI  PPMqbClient::PPMqbClient() : P_Conn(0), P_Sock(0), Port(0), ChannelN(0)
{
}

SLAPI PPMqbClient::~PPMqbClient()
{
	Disconnect();
}

int SLAPI PPMqbClient::Connect(const char * pHost, int port)
{
	int    ok = 1;
	int    amqp_status = 0;
	Disconnect();
	THROW(P_Conn = amqp_new_connection());
	THROW(P_Sock = amqp_tcp_socket_new(GetNativeConnHandle(P_Conn)));
	amqp_status = amqp_socket_open(static_cast<amqp_socket_t *>(P_Sock), pHost, port);
	THROW(amqp_status == 0);
	CATCH
		Disconnect();
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPMqbClient::Disconnect()
{
	int    ok = -1;
	if(P_Conn) {
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

//static
int FASTCALL PPMqbClient::ProcessAmqpRpcReply(const amqp_rpc_reply_t & rR)
{
	int    ok = 1;
	switch(rR.reply_type) {
		case AMQP_RESPONSE_NORMAL: break;
		case AMQP_RESPONSE_NONE: ok = 0; break;
		case AMQP_RESPONSE_LIBRARY_EXCEPTION: ok = 0; break;
		case AMQP_RESPONSE_SERVER_EXCEPTION: 
			{
				switch(rR.reply.id) {
					case AMQP_CONNECTION_CLOSE_METHOD: ok = 0; break;
					case AMQP_CHANNEL_CLOSE_METHOD: ok = 0; break;
					default: ok = 0; break;
				}
			}
			break;
	}
	return ok;
}

int SLAPI PPMqbClient::VerifyRpcReply()
{
	amqp_rpc_reply_t amqp_reply = amqp_get_rpc_reply(GetNativeConnHandle(P_Conn));
	return ProcessAmqpRpcReply(amqp_reply);
}

int SLAPI PPMqbClient::Login(const LoginParam & rP)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_rpc_reply_t amqp_reply = amqp_login(GetNativeConnHandle(P_Conn), "papyrus", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, rP.Auth.cptr(), rP.Secret.cptr());
		THROW(ProcessAmqpRpcReply(amqp_reply));
		ChannelN = 1;
		amqp_channel_open(GetNativeConnHandle(P_Conn), ChannelN);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPMqbClient::Publish(const char * pExchangeName, const char * pQueue, const MessageProperties * pProps, const void * pData, size_t dataLen)
{
	int    ok = 1;
	amqp_table_entry_t * p_amqp_tbl_entries = 0;
	THROW(P_Conn);
	{
		amqp_basic_properties_t * p_local_props = 0;
		amqp_basic_properties_t local_props;
		amqp_bytes_t exchange = amqp_cstring_bytes(/*"amq.direct"*/pExchangeName);
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
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
					StrStrAssocArray::Item pitem = pProps->Headers.at(pidx);
					p_amqp_tbl_entries[pidx].key = amqp_cstring_bytes(pitem.Key);
					p_amqp_tbl_entries[pidx].value.kind = AMQP_FIELD_KIND_UTF8; // AMQP_FIELD_KIND_BYTES
					p_amqp_tbl_entries[pidx].value.value.bytes = amqp_cstring_bytes(pitem.Val);
				}
				local_props._flags |= AMQP_BASIC_HEADERS_FLAG;
			}
			p_local_props = &local_props;
		}
		int pr = amqp_basic_publish(GetNativeConnHandle(P_Conn), ChannelN, exchange, queue, 0, 0, p_local_props, data);
		THROW(pr == AMQP_STATUS_OK);
	}
	CATCHZOK
	delete [] p_amqp_tbl_entries;
	return ok;
}

int SLAPI PPMqbClient::Consume(const char * pQueue, const char * pConsumerTag, long consumeFlags)
{
	int    ok = 1;
	THROW(P_Conn);
	{
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_bytes_t consumer_tag = amqp_cstring_bytes(pConsumerTag);
		amqp_basic_consume_ok_t * p_bco = amqp_basic_consume(GetNativeConnHandle(P_Conn), ChannelN, queue, consumer_tag,
			BIN(consumeFlags & mqofNoLocal), BIN(consumeFlags & mqofNoAck), BIN(consumeFlags & mqofExclusive), amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

static void FASTCALL AmpqBytesToSString(const amqp_bytes_t & rS, SString & rDest)
{
	rDest.Z();
	if(rS.len && rS.bytes)
		rDest.CatN(static_cast<const char *>(rS.bytes), rS.len);
}

int SLAPI PPMqbClient::ConsumeMessage(Envelope & rEnv, long timeoutMs)
{
	rEnv.Z();
	int    ok = -1;
	SString temp_buf;
	SString val_buf;
	THROW(P_Conn);
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
			rEnv.Msg.Props.TimeStamp.SetTimeT(envelope.message.properties.timestamp);
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
							case AMQP_FIELD_KIND_BOOLEAN: val_buf.Cat(r_te.value.value.boolean ? "true" : "false"); break;
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
				ok = -1;
			}
		}
		amqp_destroy_envelope(&envelope);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPMqbClient::Ack(uint64 deliveryTag, long flags /*mqofMultiple*/)
{
	int    ok = 1;
	THROW(P_Conn);
	{
		int pr = amqp_basic_ack(GetNativeConnHandle(P_Conn), ChannelN, deliveryTag, BIN(flags & mqofMultiple));
		THROW(pr >= AMQP_STATUS_OK);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPMqbClient::QueueDeclare(const char * pQueue, long queueFlags)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_bytes_t queue = amqp_cstring_bytes(pQueue);
		amqp_queue_declare_ok_t * p_qdo = amqp_queue_declare(GetNativeConnHandle(P_Conn), ChannelN, queue, 
			BIN(queueFlags & mqofPassive), BIN(queueFlags & mqofDurable), BIN(queueFlags & mqofExclusive), BIN(queueFlags & mqofAutoDelete), amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPMqbClient::ExchangeDeclare(const char * pExchange, const char * pType, long flags)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_bytes_t exchange = amqp_cstring_bytes(pExchange);
		amqp_bytes_t type = amqp_cstring_bytes(pType);
		amqp_exchange_declare_ok_t * p_ok = amqp_exchange_declare(GetNativeConnHandle(P_Conn), ChannelN, exchange, type,
			BIN(flags & mqofPassive), BIN(flags & mqofDurable), BIN(flags & mqofAutoDelete), BIN(flags & mqofInternal), amqp_empty_table);
		THROW(VerifyRpcReply());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPMqbClient::QueueBind(const char * pQueue, const char * pExchange, const char * pRoutingKey)
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

int SLAPI PPMqbClient::QueueUnbind(const char * pQueue, const char * pExchange, const char * pRoutingKey)
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

//static 
int SLAPI PPMqbClient::InitClient(PPMqbClient & rC, const PPMqbClient::InitParam & rP)
{
	int    ok = 1;
	THROW(rC.Connect(rP.Host, NZOR(rP.Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
	THROW(rC.Login(rP));
	CATCHZOK
	return ok;
}

//static 
int SLAPI PPMqbClient::SetupInitParam(PPMqbClient::InitParam & rP, SString * pDomain)
{
	int    ok = 1;
	SString data_domain;
	SString temp_buf;
	PPAlbatrosConfig acfg;
	THROW(DS.FetchAlbatrosConfig(&acfg) > 0);
	{
		acfg.GetExtStrData(ALBATROSEXSTR_MQC_HOST, temp_buf);
		rP.Host = temp_buf;
		acfg.GetExtStrData(ALBATROSEXSTR_MQC_USER, temp_buf);
		rP.Auth = temp_buf;
		acfg.GetPassword(ALBATROSEXSTR_MQC_SECRET, temp_buf);
		rP.Secret = temp_buf;
		acfg.GetExtStrData(ALBATROSEXSTR_MQC_DATADOMAIN, data_domain);
		rP.Method = 1;
	}
	THROW_PP(!pDomain || data_domain.NotEmpty(), PPERR_GLOBALDATADOMAINUNDEF);
	CATCHZOK
	ASSIGN_PTR(pDomain, data_domain);
	return ok;
}

//static 
int SLAPI PPMqbClient::InitClient(PPMqbClient & rC, SString * pDomain)
{
	int    ok = 1;
	PPMqbClient::InitParam lp;
	THROW(SetupInitParam(lp, pDomain));
	THROW(PPMqbClient::InitClient(rC, lp));
	THROW(rC.Connect(lp.Host, NZOR(lp.Port, InetUrl::GetDefProtocolPort(InetUrl::protAMQP)/*5672*/)));
	THROW(rC.Login(lp));
	CATCHZOK
	return ok;
}

static const char * P_TestQueueName = "test-papyrus-queue";

class TestMqcThread : public PPThread {
public:
	TestMqcThread(const PPMqbClient::InitParam & rP) : PPThread(kUnknown, "", 0), MqbcParam(rP)
	{
	}
	virtual void SLAPI Startup()
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
			PPLogMessage(log_file_name, "OK: Helper_InitRun", LOGMSGF_TIME|LOGMSGF_DBINFO);
			{
				THROW(mqc.QueueDeclare(QueueName, 0));
				PPLogMessage(log_file_name, "OK: QueueDeclare", LOGMSGF_TIME|LOGMSGF_DBINFO);
				THROW(mqc.ExchangeDeclare(p_exchange_name, "fanout", 0));
				PPLogMessage(log_file_name, "OK: ExchangeDeclare", LOGMSGF_TIME|LOGMSGF_DBINFO);
				THROW(mqc.QueueBind(QueueName, p_exchange_name, ""));
				PPLogMessage(log_file_name, "OK: QueueBind", LOGMSGF_TIME|LOGMSGF_DBINFO);
			}
			SDelay(1000);
			do {
				ulong random_id = SLS.GetTLA().Rg.GetUniformInt(1000000);
				(temp_buf = "This is a some stupid message").CatChar('-').Cat(random_id);
				//THROW(mqc.Publish("", P_TestQueueName, temp_buf.cptr(), temp_buf.Len()));
				PPMqbClient::MessageProperties props;
				if(mqc.Publish(p_exchange_name, /*P_TestQueueName*/"", 0 /*props*/, temp_buf.cptr(), temp_buf.Len())) {
					PPLogMessage(log_file_name, "OK: Publish", LOGMSGF_TIME|LOGMSGF_DBINFO);
					SDelay(100);
				}
				else {
					PPLogMessage(log_file_name, "ERR: Publish", LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
			} while(!stop);
		}
		CATCH
			ok = 0;
		ENDCATCH
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
			PPLogMessage(log_file_name, "OK: Helper_InitRun", LOGMSGF_TIME|LOGMSGF_DBINFO);
			THROW(mqc.QueueDeclare(QueueName, 0));
			PPLogMessage(log_file_name, "OK: QueueDeclare", LOGMSGF_TIME|LOGMSGF_DBINFO);
			THROW(mqc.Consume(QueueName, "", 0));
			PPLogMessage(log_file_name, "OK: Consume", LOGMSGF_TIME|LOGMSGF_DBINFO);
			SDelay(1500);
			do {
				PPMqbClient::Envelope env;
				int cmr = mqc.ConsumeMessage(env, 10);
				if(cmr > 0) {
					PPLogMessage(log_file_name, "OK: ConsumeMessage", LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
				else if(cmr == 0) {
					PPLogMessage(log_file_name, "ERR: ConsumeMessage", LOGMSGF_TIME|LOGMSGF_DBINFO);
				}
			} while(!stop);
		}
		CATCH
			ok = 0;
		ENDCATCH
	}
	const SString QueueName;
};

int SLAPI TestMqc()
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
		PPAlbatrosConfig acfg;
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
			THROW(mqc.ExchangeDeclare(p_exchange_name, "fanout", 0));
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
