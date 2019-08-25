// PPMQC.CPP
// Copyright (c) A.Sobolev 2019
//
#include <pp.h>
#pragma hdrstop
#include <..\OSF\rabbitmq-c\amqp.h>

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
	amqp_rpc_reply_t amqp_reply;
	amqp_reply = amqp_get_rpc_reply(GetNativeConnHandle(P_Conn));
	return ProcessAmqpRpcReply(amqp_reply);
}

int SLAPI PPMqbClient::Login(const LoginParam & rP)
{
	int    ok = 1;
	if(P_Conn) {
		amqp_rpc_reply_t amqp_reply;
		amqp_reply = amqp_login(GetNativeConnHandle(P_Conn), "papyrus", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, rP.Auth.cptr(), rP.Secret.cptr());
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
			if(pProps->ContentType) {
				SString & r_rb = SLS.AcquireRvlStr();
				SFileFormat::GetMime(pProps->ContentType, r_rb);
				local_props.content_type = amqp_cstring_bytes(r_rb);
			}
			if(pProps->Encoding) {
				SString & r_rb = SLS.AcquireRvlStr();
				SFileFormat::GetContentTransferEncName(pProps->Encoding, r_rb);
				local_props.content_encoding = amqp_cstring_bytes(r_rb);
			}
			local_props._flags = pProps->Flags;
			local_props.timestamp = pProps->TimeStamp.GetTimeT();
			local_props.delivery_mode = inrangeordefault(pProps->DeliveryMode, 1U, 2U, 1U);
			local_props.priority = inrangeordefault(pProps->Priority, 0U, 9U, 0U);
			local_props.correlation_id = amqp_cstring_bytes(pProps->CorrelationId);
			local_props.reply_to = amqp_cstring_bytes(pProps->ReplyTo);
			local_props.expiration = amqp_cstring_bytes(pProps->Expiration);
			local_props.message_id = amqp_cstring_bytes(pProps->MessageId);
			local_props.type = amqp_cstring_bytes(pProps->Type);
			local_props.user_id = amqp_cstring_bytes(pProps->UserId);
			local_props.app_id = amqp_cstring_bytes(pProps->AppId);
			local_props.cluster_id = amqp_cstring_bytes(pProps->ClusterId);
			// @todo local_props.headers
			p_local_props = &local_props;
		}
		int pr = amqp_basic_publish(GetNativeConnHandle(P_Conn), ChannelN, exchange, queue, 0, 0, p_local_props, data);
		THROW(pr == AMQP_STATUS_OK);
	}
	CATCHZOK
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

static void AmpqBytesToSString(const amqp_bytes_t & rS, SString & rDest)
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
			rEnv.Msg.Props.ContentType = SFileFormat::IdentifyMimeType(temp_buf);
			AmpqBytesToSString(envelope.message.properties.content_encoding, temp_buf);
			rEnv.Msg.Props.Encoding = SFileFormat::IdentifyMimeType(temp_buf);
			// @todo amqp_table_t headers --> rEnv.Msg.Props.Headers
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

static const char * P_TestQueueName = "test-papyrus-queue";

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
