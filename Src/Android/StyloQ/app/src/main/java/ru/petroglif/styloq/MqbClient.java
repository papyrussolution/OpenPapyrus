// MqbClient.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import static ru.petroglif.styloq.SLib.THROW;
import android.os.SystemClock;
import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.BasicProperties;
import com.rabbitmq.client.BuiltinExchangeType;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.GetResponse;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.StringTokenizer;
import java.util.concurrent.TimeoutException;

public class MqbClient {
	public static int __DefMqbConsumeTimeout = 5000;
	//
	// Descr: Типы зарезервированных параметров маршрутизации
	//
	public static int rtrsrvPapyrusDbx         = 1; // Обмен пакетами синхронизации баз данных
	public static int rtrsrvPapyrusPosProtocol = 2; // Обмен между хостом и автономными кассовыми узлами Papyrus
	public static int rtrsrvStyloView          = 3; // Обмен с системой StyloView
	public static int rtrsrvRpc                = 4; // Короткие запросы
	public static int rtrsrvRpcListener        = (rtrsrvRpc | 0x8000); // Короткие запросы (только слушатель)
	public static int rtrsrvRpcReply           = 5; // Ответы на rtrsrvRpc
	public static int rtrsrvStyloQRpc          = 6; // @v11.0.9 Запросы в рамках проекта Stylo-Q
	public static int rtrsrvStyloQRpcReply     = 7; // @v11.0.9 Ответы на rtrsrvStyloQRpc
	public static int rtrsrvStyloQRpcListener  = (rtrsrvStyloQRpc | 0x8000); // @v11.0.9 Короткие запросы (только слушатель)
	//
	// Descr: Флаги, используемые в различных функциях обмена сообщениями.
	//   Ради унификации все они сведены в общий пул.
	//
	public static int mqofPassive    = 0x0001; // queue exchange
	public static int mqofDurable    = 0x0002; // queue exchange
	public static int mqofExclusive  = 0x0004; // queue exchange consume
	public static int mqofAutoDelete = 0x0008; // queue exchange
	public static int mqofNoLocal    = 0x0010; // consume
	public static int mqofNoAck      = 0x0020; // consume cancel
	public static int mqofInternal   = 0x0040; // exchange
	public static int mqofMultiple   = 0x0080; // ack

	public static class RoutingParamEntry {
		RoutingParamEntry()
		{
			RtRsrv = 0;
			PreprocessFlags = 0;
			QueueFlags = 0;
			ExchangeFlags = 0;
		}
		//
		// ARG(replyEntry OUT): Если != null то инициализируется как блок для получения ответа от контрагента
		//
		public boolean SetupStyloQRpc(final byte [] srcIdent, final byte [] destIdent, final byte [] loclAddendum, RoutingParamEntry replyEntry)
			throws StyloQException
		{
			boolean ok = false;
			try {
				if(SLib.GetLen(srcIdent) > 0 && SLib.GetLen(destIdent) > 0) {
					//Base64.getDecoder().decode(val)
					if(SLib.GetLen(loclAddendum) > 0) {
						ByteArrayOutputStream baos = new ByteArrayOutputStream();
						baos.write(destIdent);
						baos.write(loclAddendum);
						QueueName = Base64.getEncoder().encodeToString(baos.toByteArray());
					}
					else {
						QueueName = Base64.getEncoder().encodeToString(destIdent);
					}
					RtRsrv = rtrsrvRpc;
					RoutingKey = QueueName;
					ExchangeName = "styloqrpc";
					QueueFlags = 0;
					ExchangeType = BuiltinExchangeType.DIRECT;
					ExchangeFlags = 0;
					if(replyEntry != null) {
						replyEntry.QueueName = Base64.getEncoder().encodeToString(srcIdent);
						replyEntry.QueueFlags = mqofAutoDelete;
						replyEntry.ExchangeName = "styloqrpc";
						replyEntry.ExchangeType = BuiltinExchangeType.DIRECT;
						replyEntry.ExchangeFlags = 0;
						replyEntry.RoutingKey = replyEntry.QueueName;
					}
					ok = true;
				}
			} catch(IOException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			}
			return ok;
		}
		int    RtRsrv;
		int    PreprocessFlags; // Флаги, управляющие предварительной обработкой параметров маршрутизации в функции PPMqbClient::ApplyRoutingParamEntry (создание очередей и т.д.)
		int    QueueFlags;
		BuiltinExchangeType ExchangeType; // exgtXXX
		int    ExchangeFlags;
		String QueueName;
		String ExchangeName;
		String RoutingKey;
		//String CorrelationId;
	}
	public static class LoginParam {
		LoginParam(String auth, String secret, String virtualHost)
		{
			Method = 0;
			Auth = auth;
			Secret = secret;
			VHost = virtualHost;
		}
		public final boolean IsEqual(final LoginParam s)
		{
			return (Method == s.Method && Auth.equals(s.Auth) &&
				Secret.equals(s.Secret) && VHost.equals(s.VHost));
		}
		public final String GetVHost(final String defaultVHost)
		{
			return SLib.IsEmpty(VHost) ? (SLib.IsEmpty(defaultVHost) ? "papyrus" : defaultVHost) : VHost;
		}
		int    Method;
		String Auth;
		String Secret;
		String VHost; // default: "papyrus"
	};
	public static class InitParam extends LoginParam {
		//
		// Если GetLen(auth) > 0 то auth и secret имеют приоритет перед параметрами авторизации,
		// включенными в url.
		//
		InitParam(/*String url*/URI uri, String auth, String secret) throws URISyntaxException
		{
			super(null/*"Admin"*/, null/*"CX8U3kM9wTQb"*/, "styloq");
			//URI uri = new URI(url);
			String scheme = uri.getScheme();
			String user_info = uri.getUserInfo();
			String url_auth = "";
			String url_secr = "";
			if(SLib.GetLen(user_info) > 0) {
				StringTokenizer toknzr = new StringTokenizer(user_info, ":");
				if(toknzr.countTokens() == 1)
					url_auth = user_info;
				else if(toknzr.countTokens() == 2) {
					url_auth = toknzr.nextToken();
					url_secr = toknzr.nextToken();
				}
			}
			Host = uri.getHost();
			Port = uri.getPort();
			if(SLib.GetLen(auth) > 0) {
				Auth = auth;
				Secret = secret;
			}
			else {
				Auth = url_auth;
				Secret = url_secr;
			}
		}
		RoutingParamEntry CreateNewConsumeParamEntry()
		{
			if(ConsumeParamList == null)
				ConsumeParamList = new ArrayList<>();
			RoutingParamEntry entry = new RoutingParamEntry();
			ConsumeParamList.add(entry);
			return entry;
		}
		String Host;
		int    Port;
		ArrayList<RoutingParamEntry> ConsumeParamList;
	}
	boolean IsOpened()
	{
		return (Chnnl != null && Chnnl.isOpen());
	}
	boolean QueueDeclare(final String queueName, long queueFlags) throws StyloQException
	{
		boolean ok = true;
		AMQP.Queue.DeclareOk r;
		THROW(SLib.GetLen(queueName) > 0, ppstr2.PPERR_MQBC_EMPTYQUEUENAME);
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		boolean is_durable = (queueFlags & mqofDurable) != 0;
		boolean is_exclusive = (queueFlags & mqofExclusive) != 0;
		boolean is_autodel = (queueFlags & mqofAutoDelete) != 0;
		try {
			r = Chnnl.queueDeclare(queueName, is_durable, is_exclusive, is_autodel, null);
			// @v11.3.10 {
			if(is_autodel) {
				if(AutoDeleteDeclaredQueues == null)
					AutoDeleteDeclaredQueues = new ArrayList<String>();
				AutoDeleteDeclaredQueues.add(queueName);
			}
			// } @v11.3.10
		} catch(IOException exn) {
			ok = false;
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return ok;
	}
	boolean QueueBind(final String queueName, final String exchangeName, final String routingKey) throws StyloQException
	{
		boolean ok = false;
		AMQP.Queue.BindOk r;
		THROW(SLib.GetLen(queueName) > 0, ppstr2.PPERR_MQBC_EMPTYQUEUENAME);
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		try {
			r = Chnnl.queueBind(queueName, exchangeName, routingKey);
			ok = true;
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return ok;
	}
	boolean QueueUnbind(final String queueName, final String exchangeName, final String routingKey) throws StyloQException
	{
		boolean ok = false;
		AMQP.Queue.UnbindOk r;
		THROW(SLib.GetLen(queueName) > 0, ppstr2.PPERR_MQBC_EMPTYQUEUENAME);
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		THROW(IsOpened(), ppstr2.PPERR_MQBC_CHANNELNOTOPENED);
		try {
			r = Chnnl.queueUnbind(queueName, exchangeName, routingKey);
			ok = true;
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return ok;
	}
	boolean QueueDelete(final String queueName) throws StyloQException
	{
		boolean ok = false;
		AMQP.Queue.DeleteOk r;
		THROW(SLib.GetLen(queueName) > 0, ppstr2.PPERR_MQBC_EMPTYQUEUENAME);
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		THROW(IsOpened(), ppstr2.PPERR_MQBC_CHANNELNOTOPENED);
		try {
			boolean if_unused = false;
			boolean if_empty = false;
			r = Chnnl.queueDelete(queueName, if_unused, if_empty);
			ok = true;
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return ok;
	}
	int ApplyRoutingParamEntry(final RoutingParamEntry rpe) throws StyloQException
	{
		int    ok = -1;
		if(rpe != null && SLib.GetLen(rpe.QueueName) > 0) {
			if(QueueDeclare(rpe.QueueName, rpe.QueueFlags)) {
				if(SLib.GetLen(rpe.ExchangeName) > 0) {
					if(QueueBind(rpe.QueueName, rpe.ExchangeName, rpe.RoutingKey))
						ok = 1;
					else
						ok = 0;
				}
				else
					ok = 0;
			}
			else
				ok = 0;
		}
		return ok;
	}
	public MqbClient(final InitParam param) throws StyloQException
	{
		AutoDeleteDeclaredQueues = null;
		ConnFactory = new ConnectionFactory();
		if(param != null) {
			Open(param);
			if(param.ConsumeParamList != null && param.ConsumeParamList.size() > 0) {
				for(int i = 0; i < param.ConsumeParamList.size(); i++) {
					RoutingParamEntry rpe = param.ConsumeParamList.get(i);
					ApplyRoutingParamEntry(rpe);
				}
			}
		}
	}
	public boolean Open(final InitParam param) throws StyloQException
	{
		boolean ok = false;
		Close();
		try {
			ConnFactory.setAutomaticRecoveryEnabled(true);
			ConnFactory.setRequestedHeartbeat(60);
			if(SLib.GetLen(param.Auth) > 0) {
				ConnFactory.setUsername(param.Auth);
				if(SLib.GetLen(param.Secret) > 0)
					ConnFactory.setPassword(param.Secret);
				ConnFactory.setVirtualHost(param.GetVHost(null));
				if(SLib.GetLen(param.Host) > 0) {
					ConnFactory.setHost(param.Host);
					Conn = ConnFactory.newConnection();
					Chnnl = Conn.createChannel();
					ok = true;
				}
			}
		} catch(TimeoutException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_TIMEOUT, exn.getMessage());
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return ok;
	}
	public void Close() /*throws StyloQException*/
	{
		try {
			// @v11.3.10 {
			if(AutoDeleteDeclaredQueues != null) {
				for(int i = 0; i < AutoDeleteDeclaredQueues.size(); i++) {
					String queue_name = AutoDeleteDeclaredQueues.get(i);
					if(SLib.GetLen(queue_name) > 0) {
						try {
							QueueDelete(queue_name);
						} catch(StyloQException exn) {
							;
						}
					}
				}
			}
			// } @v11.3.10
			if(Chnnl != null) {
				Chnnl.close();
				Chnnl = null;
			}
			if(Conn != null) {
				Conn.close();
				Conn = null;
			}
		} catch(TimeoutException exn) {
			//new StyloQException(ppstr2.PPERR_JEXN_TIMEOUT, exn.getMessage());
		} catch(IOException exn) {
			//new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
	}
	public void Publish(String exchangeName, String routingKey, AMQP.BasicProperties props, final byte [] data) throws StyloQException
	{
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		try {
			Chnnl.basicPublish(exchangeName, routingKey, props, data);
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
	}
	public GetResponse ConsumeMessage(String queueName, String correlationId, int timeout) throws StyloQException
	{
		//timeout = 120000; // @debug
		GetResponse resp = null;
		int count = 0;
		final long tm_start = System.currentTimeMillis();
		long tm_end = 0;
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		try {
			int do_sleep_ms = 0;
			do {
				if(do_sleep_ms > 0)
					SystemClock.sleep(do_sleep_ms);
				resp = Chnnl.basicGet(queueName, false);
				if(resp != null && SLib.GetLen(correlationId) > 0) {
					BasicProperties props = resp.getProps();
					String ci = (props != null) ? props.getCorrelationId() : null;
					if(ci == null || !correlationId.equalsIgnoreCase(ci)) {
						//Chnnl.basicAck(resp.getEnvelope().getDeliveryTag(), false);
						Chnnl.basicReject(resp.getEnvelope().getDeliveryTag(), true);
						resp = null;
						do_sleep_ms = 100;
					}
				}
				else
					do_sleep_ms = 100;
				count++;
				tm_end = System.currentTimeMillis();
			} while(resp == null && (timeout > 0 && (tm_end - tm_start) < timeout));
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return resp;
	}
	public void Ack(long deliveryTag) throws StyloQException
	{
		THROW(Chnnl != null, ppstr2.PPERR_MQBC_NOTINITED);
		try {
			Chnnl.basicAck(deliveryTag, false);
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
	}
	private Connection Conn; //= ConnFactory.newConnection();
	private Channel Chnnl;
	private ConnectionFactory ConnFactory;
	private ArrayList <String> AutoDeleteDeclaredQueues;
}
