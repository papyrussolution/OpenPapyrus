// StyloQInterchange.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import static ru.petroglif.styloq.JobServerProtocol.PPSCMD_SQ_SESSION;
import static ru.petroglif.styloq.SLib.PPOBJ_STYLOQBINDERY;
import static ru.petroglif.styloq.SLib.THROW;

import android.net.Uri;
import android.util.Log;

import com.rabbitmq.client.AMQP;
import com.rabbitmq.client.BuiltinExchangeType;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.Envelope;
import com.rabbitmq.client.GetResponse;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.AlgorithmParameters;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.KeyManagementException;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.interfaces.ECPrivateKey;
import java.security.interfaces.ECPublicKey;
import java.security.spec.ECGenParameterSpec;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;
import java.security.spec.ECPrivateKeySpec;
import java.security.spec.ECPublicKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.InvalidParameterSpecException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Collections;
import java.util.StringTokenizer;
import java.util.UUID;

import javax.crypto.KeyAgreement;

public class StyloQInterchange {
	private ConnectionFactory ConnFactory = new ConnectionFactory();
	StyloQApp Ctx;

	public static long EvaluateExpiryTime(int expiryPeriodSec)
	{
		long result = 0;
		if(expiryPeriodSec > 0) {
			final long now_ = System.currentTimeMillis();
			result = (now_ / 1000) + expiryPeriodSec;
		}
		return result;
	}
	public static boolean IsExpired(long expiration)
	{
		boolean result = false;
		if(expiration <= 0)
			result = true;
		else {
			final long now_ = System.currentTimeMillis();
			// Эксперсс-проверка на валидность expiration (из-за ошибки на начальной фазе разработки)
			if(expiration > now_)
				expiration = 0;
			if(expiration <= 0 || expiration <= (now_ / 1000))
				result = true;
		}
		return result;
	}
	public StyloQInterchange(StyloQApp appCtx)
	{
		Ctx = appCtx;
	}

	public static class ThreadEngine_DocStatusPoll implements Runnable {
		private StyloQApp AppCtx;
		ThreadEngine_DocStatusPoll(StyloQApp appCtx)
		{
			AppCtx = appCtx;
		}
		@Override public void run() { DocStatusPoll(AppCtx); }
	}
	//
	// Descr: Функция опроса сервисов о статусах документов.
	//   Находит все документы по всем сервисам, для которых необходима информация о
	//   статусе у сервиса и отправляет запросы сервисам для выяснения статусов этих документов.
	// Note: Функция предназначена для выполнения из планировщика заданий и должна вызываться
	//   строго в отдельном потоке.
	//
	public static void DocStatusPoll(StyloQApp appCtx)
	{
		final String cmd_symb = "requestdocumentstatuslist";
		try {
			if(appCtx != null) {
				StyloQDatabase db = appCtx.GetDB();
				if(db != null) {
					ArrayList<Long> svc_id_list = db.GetForeignSvcIdList(true);
					if(svc_id_list != null) {
						for(int svcidx = 0; svcidx < svc_id_list.size(); svcidx++) {
							final long svc_id = svc_id_list.get(svcidx);
							StyloQDatabase.SecStoragePacket svc_pack = db.GetPeerEntry(svc_id);
							final byte [] svc_ident = (svc_pack != null) ? svc_pack.Pool.Get(SecretTagPool.tagSvcIdent) : null;
							if(SLib.GetLen(svc_ident) > 0) {
								ArrayList <Integer> doc_status_list = new ArrayList<>();
								doc_status_list.add(StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC);
								ArrayList<Long> doc_id_list = db.GetDocIdListByType(+1, StyloQDatabase.SecStoragePacket.doctypGeneric,
										svc_id, doc_status_list, null);
								if(doc_id_list != null && doc_id_list.size() > 0) {
									JSONObject js_req = new JSONObject();
									try {
										js_req.put("cmd", cmd_symb);
										JSONArray js_list = null;
										for(int docidx = 0; docidx < doc_id_list.size(); docidx++) {
											final long doc_id = doc_id_list.get(docidx);
											StyloQDatabase.SecStoragePacket doc_pack = db.GetPeerEntry(doc_id);
											if(doc_pack != null) {
												final  int doc_status = doc_pack.Rec.GetDocStatus();
												byte[] raw_doc = doc_pack.Pool.Get(SecretTagPool.tagRawData);
												if(SLib.GetLen(raw_doc) > 0) {
													String json_doc = new String(raw_doc);
													Document local_doc = new Document();
													if(local_doc.FromJson(json_doc) && local_doc.H != null) {
														// Теоретически, в json-теле документа должен быть svcident и его надо сравнить с svc_ident
														// сервиса, с которым мы на этой итерации работаем, но, кажется, не всегда в теле есть svcident :(
														//byte [] doc_svc_ident = local_doc.H.SvcIdent;
														// if(SLib.AreByteArraysEqual(local_doc.H.SvcIdent, svc_ident)) {
															if(local_doc.H.Uuid != null) {
																JSONObject js_doc_item = new JSONObject();
																if(local_doc.H.OrgCmdUuid != null)
																	js_doc_item.put("orgcmduuid", local_doc.H.OrgCmdUuid.toString());
																js_doc_item.put("uuid", local_doc.H.Uuid.toString());
																js_doc_item.put("cid", local_doc.H.ID);
																js_doc_item.put("cst", doc_status);
																if(js_list == null)
																	js_list = new JSONArray();
																js_list.put(js_doc_item);
															}
														//}
													}
												}
											}
										}
										if(js_list != null) {
											js_req.put("list", js_list);
											//
											// Для этой командны нам нужен фейковый экземпляр orgCmdItem
											// ради того, чтобы при получении ответа от сервиса
											// результат не передавался бы в основной поток, но обрабатывался бы 'on the spot'
											// в потоке обмена данными.
											//
											StyloQCommand.Item fake_org_cmd = new StyloQCommand.Item();
											fake_org_cmd.Name = cmd_symb;
											RequestBlock rblk = new RequestBlock(svc_pack, js_req, fake_org_cmd);
											DoSvcRequest(appCtx, rblk);
										}
									} catch(JSONException exn) {
										;
									}
								}
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			;
		}
	}
	private static boolean AcceptDocStatusPollResult(StyloQApp appCtx, JSONObject jsReply) throws StyloQException
	{
		boolean ok = false;
		if(jsReply != null) {
			JSONArray js_list = jsReply.optJSONArray("list");
			if(js_list != null) {
				StyloQDatabase db = appCtx.GetDB();
				for(int lidx = 0; lidx < js_list.length(); lidx++) {
					JSONObject js_item = js_list.optJSONObject(lidx);
					if(js_item != null) {
						String doc_uuid_text = js_item.optString("uuid", null);
						long cid = js_item.optLong("cid", 0);
						long sid = js_item.optLong("sid", 0);
						int sst = js_item.optInt("sst", 0);
						UUID doc_uuid = UUID.fromString(doc_uuid_text);
						if(cid > 0 && doc_uuid != null) {
							StyloQDatabase.SecStoragePacket pack = db.GetPeerEntry(cid);
							if(pack != null && StyloQDatabase.SecStoragePacket.IsDocKind(pack.Rec.Kind)) {
								int ex_status = StyloQDatabase.SecTable.Rec.GetDocStatus(pack.Rec.Flags);
								if(sst != 0 && sst != ex_status) {
									if(Document.ValidateStatusTransition(ex_status, sst)) {
										pack.Rec.SetDocStatus(sst);
										long temp_id = db.PutPeerEntry(cid, pack, true);
										if(temp_id == cid)
											ok = true;
									}
								}
							}
						}
					}
				}
			}
		}
		return ok;
	}
	public static final String EC_PARAM_NAME = "prime256v1"; /*"secp256r1"*/
	public static final int gcisfMakeSecret = 0x0001;
	public static class Invitation {
		public static final int capRegistrationAllowed = 0x0001;
		public static final int capVerifiableFaceOnly  = 0x0002;
		public static final int capAnonymFaceDisabled  = 0x0004;

		int Capabilities; // capXXX
		byte [] SvcIdent;
		byte [] LoclAddendum; // @v11.2.3 Специальное дополнение, индицирующее локальный сервер (то есть, ассоциированный с конкретной машиной или сеансом)
		String AccessPoint;
		String CommandJson;
	}
	int GetNominalSessionLifeTimeSec()
	{
		return 3600;
	}
	public static Invitation AcceptInvitation(String invData) throws StyloQException
	{
		Invitation result = null;
		try {
			THROW(SLib.GetLen(invData) > 0, ppstr2.PPERR_SQ_INVITATPARSEFAULT_EMPTY);
			StringTokenizer toknzr = new StringTokenizer(invData, "&");
			int tokn = 0;
			while(toknzr.hasMoreTokens()) {
				String tok = toknzr.nextToken();
				if(tok == null)
					break;
				else {
					tokn++;
					if(tokn == 1) {
						THROW(tok.charAt(0) == 'A', ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
						tok = tok.substring(1);
						if(result == null)
							result = new Invitation();
						result.SvcIdent = Base64.getDecoder().decode(tok);
					}
					else if(tokn == 2) { // @v11.2.3 Locl addendum
						result.LoclAddendum = (SLib.GetLen(tok) > 0) ? Base64.getDecoder().decode(tok) : null;
					}
					else if(tokn == 3) { // Capabilities // @v11.2.3 2-->3
						byte[] cap_buf = Base64.getDecoder().decode(tok);
						THROW(cap_buf != null && cap_buf.length == 4, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
						if(result == null)
							result = new Invitation();
						result.Capabilities = SLib.BytesToInt(cap_buf, 0);
					}
					else if(tokn == 4) { // access point // @v11.2.3 3-->4
						byte[] acsp_buf = Base64.getDecoder().decode(tok);
						if(acsp_buf != null) {
							String acsp_text = new String(acsp_buf);
							URI uri = new URI(acsp_text);
							THROW(SLib.GetLen(uri.getScheme()) > 0 && SLib.GetLen(uri.getHost()) > 0, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
							if(result == null)
								result = new Invitation();
							result.AccessPoint = acsp_text;
						}
					}
					else if(tokn == 5) { // command // @v11.2.3 4-->5
						byte[] json_buf = Base64.getDecoder().decode(tok);
						THROW(SLib.GetLen(json_buf) > 0, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
						String json_text = new String(json_buf);
						THROW(SLib.GetLen(json_text) > 0, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
						JSONObject jsobj = new JSONObject(json_text);
						THROW(jsobj != null, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
						if(result == null)
							result = new Invitation();
						result.CommandJson = json_text;
					}
					else {
						THROW(false, ppstr2.PPERR_SQ_INVITATPARSEFAULT, invData);
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		} catch(URISyntaxException exn) {
			result = null;
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
		return result;
	}
	public static class RoundTripBlock {
		RoundTripBlock(DoInterchangeParam param)
		{
			Other = new SecretTagPool();
			Sess = new SecretTagPool();
			StP = null;
			SelfyFace = null;
			OtherFace = null;
			InnerSvcID = 0;
			InnerSessID = 0;
			InnerCliID = 0;
			State = 0;
			MqbAuth = param.MqbAuth;
			MqbSecret = param.MqbSecret;
			Mqbc = null;
			if(SLib.GetLen(param.SvcIdent) > 0)
				Other.Put(SecretTagPool.tagSvcIdent, param.SvcIdent);
			if(SLib.GetLen(param.LoclAddendum) > 0) // @v11.2.3
				Other.Put(SecretTagPool.tagSvcLoclAddendum, param.LoclAddendum);
			if(SLib.GetLen(param.AccsPoint) > 0)
				Other.Put(SecretTagPool.tagSvcAccessPoint, param.AccsPoint.getBytes());
		}
		void Close()
		{
			if(Mqbc != null && Mqbc.IsOpened()) {
				if(MqbRpeReply != null) {
					if(SLib.GetLen(MqbRpeReply.QueueName) > 0 && SLib.GetLen(MqbRpeReply.ExchangeName) > 0 &&
							SLib.GetLen(MqbRpeReply.RoutingKey) > 0) {
						try {
							Mqbc.QueueUnbind(MqbRpeReply.QueueName, MqbRpeReply.ExchangeName, MqbRpeReply.RoutingKey);
						} catch(StyloQException exn) {
							;
						}
					}
				}
				Mqbc.Close();
			}
		}
		SecretTagPool Other; // Блок параметров сервиса, к которому осуществляется запрос
		SecretTagPool Sess;  // Блок параметров сессии
		StyloQFace SelfyFace; // Наш лик для представления перед сервисом
		StyloQFace OtherFace; // Лик сервиса
		StyloQDatabase.SecStoragePacket StP; // Блок собственных данных, извлеченных из хранилища
		long InnerSvcID;
		long InnerSessID;
		long InnerCliID;
		int State;
		UUID Uuid; // Для клиента. Инициализируется в методе InitRoundTripBlock() на основании
			// значения SSecretTagPool::tagSvcAccessPoint в Other.
		URI Url; // Для клиента. Инициализируется в методе InitRoundTripBlock() на основании
			// значения SSecretTagPool::tagSvcAccessPoint в Other.
		String MqbAuth;
		String MqbSecret;
		MqbClient Mqbc; // Экземпляр клиента MQ уже "заряженный" на прослушку регулярной очереди от визави
		MqbClient.RoutingParamEntry MqbRpe; // Параметры маршрутизации при использовании брокера MQ
		MqbClient.RoutingParamEntry MqbRpeReply;
	}
	public StyloQFace GetFaceForTransmission(StyloQDatabase db, final byte [] svcIdent, StyloQDatabase.SecStoragePacket ownPeerEntry, boolean force) throws StyloQException
	{
		StyloQFace result = null;
		if(SLib.GetLen(svcIdent) > 0) {
			StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svcIdent);
			if(svc_pack != null) {
				StyloQFace selected_face = null; //SelectFaceForSvc(db, svcIdent, ownPeerEntry);
				final long svc_id = svc_pack.Rec.ID;
				// Параметр svcPool не обязательно извлечен из базы данных - вполне возможно, что
				// это - урезанный пул, пришедший от сервиса.
				//
				// Пока эта функция - почти заглушка. Возвращает первый встретившийся лик
				//
				ArrayList<StyloQFace> face_list = db.GetFaceList();
				if(face_list != null) {
					byte[] face_ident = null;
					face_ident = svc_pack.Pool.Get(SecretTagPool.tagAssignedFaceRef);
					StyloQFace assigned_result = null;
					StyloQFace cfg_default_result = null;
					StyloQFace default_result = null;
					byte[] cfg_def_face_ident = null;
					if(ownPeerEntry != null) {
						final byte[] cfg_bytes = ownPeerEntry.Pool.Get(SecretTagPool.tagPrivateConfig);
						if(SLib.GetLen(cfg_bytes) > 0) {
							String cfg_json = new String(cfg_bytes);
							StyloQConfig private_config = new StyloQConfig();
							private_config.FromJson(cfg_json);
							String def_face_ref_hex = private_config.Get(StyloQConfig.tagDefFace);
							if(SLib.GetLen(def_face_ref_hex) > 0)
								cfg_def_face_ident = Base64.getDecoder().decode(def_face_ref_hex);
						}
					}
					for(int i = 0; assigned_result == null && i < face_list.size(); i++) {
						StyloQFace face_item = face_list.get(i);
						if(SLib.GetLen(face_ident) > 0 && SLib.AreByteArraysEqual(face_ident, face_item.BI))
							assigned_result = face_item;
						else if(SLib.GetLen(cfg_def_face_ident) > 0 && SLib.AreByteArraysEqual(cfg_def_face_ident, face_item.BI))
							cfg_default_result = face_item;
						else if(default_result == null)
							default_result = face_item;
					}
					if(assigned_result != null)
						selected_face = assigned_result;
					else if(cfg_default_result != null)
						selected_face = cfg_default_result;
					else if(default_result != null)
						selected_face = default_result;
				}
				if(selected_face == null) {
					StyloQFace default_face = new StyloQFace();
					String anonym_name = db.MakeTextHashForForeignService(svc_pack, 8);
					//String anonym_name_before1139 = "Anonym-" + UUID.randomUUID().toString();
					default_face.Set(StyloQFace.tagCommonName, 0, anonym_name);
					String jstext = default_face.ToJson();
					if(SLib.GetLen(jstext) > 0) {
						StyloQDatabase.SecStoragePacket sp = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kFace);
						sp.Pool.Put(SecretTagPool.tagSelfyFace, jstext.getBytes(StandardCharsets.UTF_8));
						long new_id = db.PutPeerEntry(sp.Rec.ID, sp, true);
						if(new_id > 0)
							selected_face = db.GetFace(new_id, SecretTagPool.tagSelfyFace, null);
					}
				}
				if(selected_face != null) {
					if(force) {
						result = selected_face;
					}
					else {
						long face_id = selected_face.ID;
						StyloQDatabase.SysJournalTable.Rec ev_last_set = db.GetLastObjEvent(SLib.PPACN_STYLOQFACETRANSMITTED, PPOBJ_STYLOQBINDERY, svc_id);
						if(ev_last_set == null || ev_last_set.Extra != face_id)
							result = selected_face;
						else {
							StyloQDatabase.SysJournalTable.Rec ev_last_face_upd = (face_id > 0) ? db.GetLastObjEvent(SLib.PPACN_OBJUPD, PPOBJ_STYLOQBINDERY, face_id) : null;
							if(ev_last_face_upd != null && (ev_last_set.TimeStamp <= ev_last_face_upd.TimeStamp))
								result = selected_face;
						}
					}
				}
			}
		}
		return result;
	}
	public StyloQFace SelectFaceForSvc(StyloQDatabase db, final byte [] svcIdent, StyloQDatabase.SecStoragePacket ownPeerEntry) throws StyloQException
	{
		// Параметр svcPool не обязательно извлечен из базы данных - вполне возможно, что
		// это - урезанный пул, пришедший от сервиса.
		//
		// Пока эта функция - почти заглушка. Возвращает первый встретившийся лик
		//
		StyloQFace result = null;
		ArrayList<StyloQFace> face_list = db.GetFaceList();
		if(face_list != null) {
			byte [] face_ident = null;
			if(SLib.GetLen(svcIdent) > 0) {
				StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svcIdent);
				if(svc_pack != null)
					face_ident = svc_pack.Pool.Get(SecretTagPool.tagAssignedFaceRef);
			}
			StyloQFace assigned_result = null;
			StyloQFace cfg_default_result = null;
			StyloQFace default_result = null;
			byte [] cfg_def_face_ident = null;
			if(ownPeerEntry != null) {
				final byte[] cfg_bytes = ownPeerEntry.Pool.Get(SecretTagPool.tagPrivateConfig);
				if(SLib.GetLen(cfg_bytes) > 0) {
					String cfg_json = new String(cfg_bytes);
					StyloQConfig private_config = new StyloQConfig();
					private_config.FromJson(cfg_json);
					String def_face_ref_hex = private_config.Get(StyloQConfig.tagDefFace);
					if(SLib.GetLen(def_face_ref_hex) > 0)
						cfg_def_face_ident = Base64.getDecoder().decode(def_face_ref_hex);
				}
			}
			for(int i = 0; assigned_result == null && i < face_list.size(); i++) {
				StyloQFace face_item = face_list.get(i);
				if(SLib.GetLen(face_ident) > 0 && SLib.AreByteArraysEqual(face_ident, face_item.BI))
					assigned_result = face_item;
				else if(SLib.GetLen(cfg_def_face_ident) > 0 && SLib.AreByteArraysEqual(cfg_def_face_ident, face_item.BI))
					cfg_default_result = face_item;
				else if(default_result == null)
					default_result = face_item;
			}
			if(assigned_result != null)
				result = assigned_result;
			else if(cfg_default_result != null)
				result = cfg_default_result;
			else if(default_result != null)
				result = default_result;
		}
		if(result == null) {
			StyloQFace default_face = new StyloQFace();
			default_face.Set(StyloQFace.tagCommonName, 0, "Anonym-" + UUID.randomUUID().toString());
			String jstext = default_face.ToJson();
			if(SLib.GetLen(jstext) > 0) {
				StyloQDatabase.SecStoragePacket sp = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kFace);
				sp.Pool.Put(SecretTagPool.tagSelfyFace, jstext.getBytes(StandardCharsets.UTF_8));
				long new_id = db.PutPeerEntry(sp.Rec.ID, sp, true);
				if(new_id > 0)
					result = db.GetFace(new_id, SecretTagPool.tagSelfyFace, null);
			}
		}
		return result;
	}
	public void InitRoundTripBlock(StyloQDatabase db, RoundTripBlock rtb) throws StyloQException
	{
		rtb.StP = db.GetOwnPeerEntry();
		try {
			if(rtb.StP != null) {
				boolean do_generate_public_ident = true;
				byte[] svc_ident = rtb.Other.Get(SecretTagPool.tagSvcIdent);
				THROW(svc_ident != null, ppstr2.PPERR_SQ_UNDEFSVCID);
				{
					//rtb.Url;
					byte[] svc_acsp = rtb.Other.Get(SecretTagPool.tagSvcAccessPoint);
					THROW(SLib.GetLen(svc_acsp) > 0, ppstr2.PPERR_SQ_UNDEFSVCACCSPOINT);
					String url_buf = new String(svc_acsp);
					rtb.Url = new URI(url_buf);
					THROW(rtb.Url.getHost().length() > 0, ppstr2.PPERR_SQ_UNDEFSVCACCSPOINTHOST, url_buf);
					int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
					THROW(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps ||
							uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQP, ppstr2.PPERR_SQ_INVSVCACCSPOINTPROT, url_buf);
				}
				StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svc_ident);
				if(svc_pack != null) {
					THROW(svc_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
					rtb.InnerSvcID = svc_pack.Rec.ID;
					if(svc_pack.Rec.CorrespondID > 0) { // Для этого сервиса есть сохраненная сессия
						StyloQDatabase.SecStoragePacket corr_pack = db.GetPeerEntry(svc_pack.Rec.CorrespondID);
						THROW(corr_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kSession, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
						//byte [] sess_secret = rtb.Sess.Get(SecretTagPool.tagSecret);
						if(/*SLib.GetLen(sess_secret) > 0 &&*/!StyloQInterchange.IsExpired(corr_pack.Rec.Expiration)) {
							byte[] temp_bch = svc_pack.Pool.Get(SecretTagPool.tagClientIdent);
							rtb.Sess.Put(SecretTagPool.tagClientIdent, temp_bch);
							int cid_list[] = {SecretTagPool.tagSessionPrivateKey, SecretTagPool.tagSessionPublicKey,
									SecretTagPool.tagSessionSecret, SecretTagPool.tagSvcIdent, SecretTagPool.tagSessionPublicKeyOther};
							rtb.Sess.CopyFrom(corr_pack.Pool, cid_list, true);
							rtb.InnerSessID = corr_pack.Rec.ID;
							do_generate_public_ident = false;
						}
					}
				}
				if(do_generate_public_ident) {
					GeneratePublicIdent(rtb.StP.Pool, svc_ident, SecretTagPool.tagClientIdent, gcisfMakeSecret, rtb.Sess);
					if(rtb.Other != null) {
						// @debug Для отладки копируем параметры верификации (на своей стороне повторим серверную верификацию)
						//int cid_list[] = {SecretTagPool.tagSrpVerifier, SecretTagPool.tagSrpVerifierSalt};
						//rtb.Other.CopyFrom(svc_pack.Pool, cid_list, false);
					}
				}
				{
					rtb.Uuid = UUID.randomUUID();
					Log.d("rtb.Uuid = UUID.randomUUID()", rtb.Uuid.toString());
					// Включение этого идентификатора в общий пул сомнительно. Дело в том, что
					// этот идентификатор обрабатывается на ранней фазе получения сообщения стороной диалога
					// когда общий пакет сообщения еще не распакован (не расшифрован и т.д.)
					ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
					bb.putLong(rtb.Uuid.getMostSignificantBits());
					bb.putLong(rtb.Uuid.getLeastSignificantBits());
					rtb.Sess.Put(SecretTagPool.tagRoundTripIdent, bb.array()); // ?
				}
			}
		} catch(URISyntaxException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
	}
	public boolean Session_ClientRequest(StyloQInterchange.RoundTripBlock rtb) throws StyloQException
	{
		boolean ok = false;
		MqbClient mqbc = null;
		try {
			byte[] svc_acsp = rtb.Other.Get(SecretTagPool.tagSvcAccessPoint);
			byte[] svc_ident = rtb.Other.Get(SecretTagPool.tagSvcIdent);
			byte[] sess_pub_key = rtb.Sess.Get(SecretTagPool.tagSessionPublicKey);
			byte[] other_sess_public = rtb.Sess.Get(SecretTagPool.tagSessionPublicKeyOther);
			byte[] own_ident = rtb.Sess.Get(SecretTagPool.tagClientIdent);
			//THROW(SLib.GetLen(svc_acsp) > 0, ppstr2.PPERR_SQ_UNDEFSVCACCSPOINT);
			THROW(SLib.GetLen(svc_ident) > 0, ppstr2.PPERR_SQ_UNDEFSVCID);
			THROW(SLib.GetLen(sess_pub_key) > 0, ppstr2.PPERR_SQ_UNDEFSESSPUBKEY);
			THROW(SLib.GetLen(other_sess_public) > 0, ppstr2.PPERR_SQ_UNDEFOTHERPUBKEY);
			THROW(SLib.GetLen(own_ident) > 0, ppstr2.PPERR_SQ_UNDEFCLIID);
			//String url_buf = new String(svc_acsp);
			//URI uri = new URI(url_buf);
			//String scheme = uri.getScheme();
			//if(scheme.equalsIgnoreCase("amqp") || scheme.equalsIgnoreCase("amqps")) {
			//
			JobServerProtocol.StyloQFrame tp = new JobServerProtocol.StyloQFrame();
			tp.StartWriting(JobServerProtocol.PPSCMD_SQ_SESSION, JobServerProtocol.StyloQFrame.psubtypeForward);
			tp.P.Put(SecretTagPool.tagSessionPublicKey, sess_pub_key);
			tp.P.Put(SecretTagPool.tagClientIdent, own_ident);
			tp.P.Put(SecretTagPool.tagSvcIdent, svc_ident);
			tp.FinishWriting(null);
			int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
			if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
				if(SendHttpQuery(rtb, tp, null) > 0) {
					//ok = 1;
					if(!tp.CheckRepError(Ctx)) {
						;  // @todo error
					}
					else {
						if(tp.H.Type == PPSCMD_SQ_SESSION && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
							// Ключ шифрования сессии есть и у нас и у сервиса!
							//THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
							// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
							ok = true; // SUCCESS!
						}
					}
				}
			}
			else if((uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) || (uriprot == SLib.uripprotUnkn && SLib.GetLen(rtb.MqbAuth) > 0)) {
				MqbClient.InitParam mqip = new MqbClient.InitParam(rtb.Url, rtb.MqbAuth, rtb.MqbSecret);
				// (done by MqbClient.InitParam) mqip.Host = uri.getHost();
				// (done by MqbClient.InitParam) mqip.Port = uri.getPort();
				{
					MqbClient.RoutingParamEntry rpe = new MqbClient.RoutingParamEntry(); // маршрут до очереди, в которой сервис ждет новых клиентов
					MqbClient.RoutingParamEntry reply_rpe = mqip.CreateNewConsumeParamEntry();
					rpe.SetupStyloQRpc(sess_pub_key, svc_ident, rtb.Other.Get(SecretTagPool.tagSvcLoclAddendum), reply_rpe);
					mqbc = new MqbClient(mqip);
					final int cmto = MqbClient.__DefMqbConsumeTimeout;
					// @v11.3.1 AMQP.BasicProperties props = new AMQP.BasicProperties.Builder().expiration(Integer.toString(cmto)).build();
					// @v11.3.1 mqbc.Publish(rpe.ExchangeName, rpe.RoutingKey, props, tp.D);
					PublishMqb(mqbc, rtb, rpe, tp.D); // @v11.3.1
					// @v11.3.1 GetResponse resp = mqbc.ConsumeMessage(reply_rpe.QueueName, null, cmto);
					GetResponse resp = ConsumeMqb(mqbc, rtb, reply_rpe.QueueName, cmto); // @v11.3.1
					THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOSESSQUERY, Integer.toString(cmto));
					// @v11.3.1 Envelope env = resp.getEnvelope();
					// @v11.3.1 mqbc.Ack(env.getDeliveryTag());
					tp.Read(resp.getBody(), null);
					if(!tp.CheckRepError(Ctx)) {
						;  // @todo error
					}
					else if(tp.H.Type == PPSCMD_SQ_SESSION && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
						// Ключ шифрования сессии есть и у нас и у сервиса!
						//THROW(rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, 0));
						MqbClient.RoutingParamEntry rpe_regular = new MqbClient.RoutingParamEntry(); // маршрут до очереди, в которой сервис будет с нами общаться
						rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, null, reply_rpe);
						// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
						SetRoundTripBlockReplyValues(rtb, mqbc, rpe_regular, reply_rpe);
						ok = true; // SUCCESS!
					}
				}
			}
		} catch(URISyntaxException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
		finally {
			if(mqbc != null && !ok) {
				mqbc.Close();
			}
		}
		return ok;
	}
	public void ExtractSessionFromPacket(final StyloQDatabase.SecStoragePacket pack, SecretTagPool sessCtx) throws StyloQException
	{
		THROW(pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kSession, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
		int cid_list[] = {SecretTagPool.tagSessionPrivateKey, SecretTagPool.tagSessionPublicKey, SecretTagPool.tagSessionSecret};
		sessCtx.CopyFrom(pack.Pool, cid_list, true);
	}
	//
	// Descr: Статусы, возвращаемые функцией FetchSessionKeys
	//
	public static final int fsksError = 0;          // Ошибка
	public static final int fsksNewEntry = 1;       // Запись с идентификатором rForeignIdent не найдена
	public static final int fsksNewSession = 2;     // Запись с идентификатором rForeignIdent найдена, но требуется инициация новой сессии
	public static final int fsksSessionById = 3;    // Найдена сессия по идентификатору rForeignIdent
	public static final int fsksSessionByCliId = 4; // Найдена сессия, соответствующая клиентскому ид rForeignIdent
	public static final int fsksSessionBySvcId = 5; // Найдена сессия, соответствующая серверному ид rForeignIdent

	public int FetchSessionKeys(int kind, StyloQDatabase db, SecretTagPool sessCtx, final byte[] foreignIdent) throws StyloQException
	{
		int status = fsksError;
		//boolean do_generate_keys = true;
		StyloQDatabase.SecStoragePacket pack = db.SearchGlobalIdentEntry(kind, foreignIdent);
		if(pack != null) {
			if(pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kSession) {
				ExtractSessionFromPacket(pack, sessCtx);
				status = fsksSessionById;
			}
			else if(pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
				if(pack.Rec.CorrespondID > 0) {
					StyloQDatabase.SecStoragePacket corr_pack = db.GetPeerEntry(pack.Rec.CorrespondID);
					if(corr_pack != null) {
						THROW(corr_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kSession, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
						ExtractSessionFromPacket(corr_pack, sessCtx); // @v11.2.10 @fix pack-->corr_pack
						status = fsksSessionBySvcId;
					}
					else
						status = fsksNewSession;
				}
				else
					status = fsksNewSession;
			}
			else if(pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kClient) {
				if(pack.Rec.CorrespondID > 0) {
					StyloQDatabase.SecStoragePacket corr_pack = db.GetPeerEntry(pack.Rec.CorrespondID);
					if(corr_pack != null) {
						THROW(corr_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kSession, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
						ExtractSessionFromPacket(pack, sessCtx);
						status = fsksSessionByCliId;
					}
					else
						status = fsksNewSession;
				}
				else
					status = fsksNewSession;
			}
			else {
				new StyloQException(ppstr2.PPERR_SQ_WRONGDBITEMKIND, "");
			}
		}
		else
			status = fsksNewEntry;
		return status;
	}
	public void KexGenerageSecret(SecretTagPool sessCtx, final SecretTagPool otherCtx) throws StyloQException
	{
		try {
			byte[] other_public = otherCtx.Get(SecretTagPool.tagSessionPublicKey);
			//byte [] my_public = sessCtx.Get(SecretTagPool.tagSessionPublicKey);
			byte[] my_private = sessCtx.Get(SecretTagPool.tagSessionPrivateKey);
			byte[] shared_secret = null;
			THROW(other_public != null, ppstr2.PPERR_SQ_UNDEFOTHERPUBKEY);
			THROW(my_private != null, ppstr2.PPERR_SQ_UNDEFPRIVKEY);
			final int ecptdimsize = 32;
			int opl_other = other_public.length;
			//int opl_my = my_public.length;
			String other_pub_mime64 = Base64.getEncoder().encodeToString(other_public);
			THROW(opl_other == (1 + ecptdimsize * 2) && other_public[0] == 4,
					ppstr2.PPERR_SQ_INVOTHERPUBKEY, other_pub_mime64);
			/*else if(opl_my != (1+ecptdimsize*2) || my_public[0] != 4) {
				ok = false;
			}*/
			byte[] op_x = new byte[ecptdimsize + 1];
			byte[] op_y = new byte[ecptdimsize + 1];
			byte[] mp_x = new byte[ecptdimsize + 1];
			byte[] mp_y = new byte[ecptdimsize + 1];
			ByteArrayInputStream bais = new ByteArrayInputStream(other_public);
			int first_byte = bais.read();
			THROW(bais.read(op_x, 1, ecptdimsize) == ecptdimsize && bais.read(op_y, 1, ecptdimsize) == ecptdimsize, ppstr2.PPERR_SQ_INVOTHERPUBKEY, other_pub_mime64);
			/*bais = new ByteArrayInputStream(my_public);
			first_byte = bais.read();
			if(bais.read(mp_x, 1, ecptdimsize) != ecptdimsize || bais.read(mp_y, 1, ecptdimsize) != ecptdimsize) {
				ok = false;
			}
			else*/
			{
				AlgorithmParameters params = AlgorithmParameters.getInstance("EC");
				params.init(new ECGenParameterSpec(EC_PARAM_NAME));
				ECParameterSpec ec_params = params.getParameterSpec(ECParameterSpec.class);
				//
				ECPoint ecp_pub_other = new ECPoint(new BigInteger(op_x), new BigInteger(op_y));
				//ECPoint ecp_pub_my = new ECPoint(new BigInteger(mp_x), new BigInteger(mp_y));
				//ECParameterSpec ecps = new ECParameterSpec()
				KeyFactory kf = KeyFactory.getInstance("EC");
				ECPrivateKey _myprvk = (ECPrivateKey) kf.generatePrivate(new ECPrivateKeySpec(new BigInteger(my_private), ec_params));
				//ECPublicKey _mypubk = (ECPublicKey)kf.generatePublic(new ECPublicKeySpec(ecp_pub_my, ec_params));
				ECPublicKey _othrpubk = (ECPublicKey) kf.generatePublic(new ECPublicKeySpec(ecp_pub_other, ec_params));
				//
				KeyAgreement ka = KeyAgreement.getInstance("ECDH");
				ka.init(_myprvk/*kp.getPrivate()*/);
				ka.doPhase(_othrpubk, true);
				shared_secret = ka.generateSecret();
				sessCtx.Put(SecretTagPool.tagSessionSecret, shared_secret);
				//
			}
		} catch(InvalidKeySpecException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_INVCRYPTOKEYSPEC, exn.getMessage());
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		} catch(InvalidKeyException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_INVCRYPTOKEY, exn.getMessage());
		} catch(InvalidParameterSpecException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_INVPARAMSPEC, exn.getMessage());
		}
	}
	public boolean KexGenerateKeys(SecretTagPool sessCtx) throws StyloQException
	{
		// @debug Provider[] provider_list = Security.getProviders();
		// NID_X9_62_prime256v1
		boolean ok = true;
		try {
			byte[] _pub2 = null;
			byte[] _prv2 = null;
			KeyPairGenerator g = KeyPairGenerator.getInstance("EC", "AndroidOpenSSL");
			ECGenParameterSpec ecsp = new ECGenParameterSpec(EC_PARAM_NAME); // ? analog of OpenSSL NID_X9_62_prime256v1
			//
			do {
				g.initialize(ecsp);
				KeyPair kp = g.genKeyPair();
				ECPrivateKey k_priv_key = (ECPrivateKey) kp.getPrivate(); // bignum
				ECPublicKey k_pub_key = (ECPublicKey) kp.getPublic(); // ec_point
				//
				{
					ECPoint public_ecpt = k_pub_key.getW();
					BigInteger pub_aff_x = public_ecpt.getAffineX();
					BigInteger pub_aff_y = public_ecpt.getAffineY();
					//byte[] pub_bytes_aff_x = pub_aff_x.toByteArray();
					//byte[] pub_bytes_aff_y = pub_aff_y.toByteArray();
					ByteArrayOutputStream baos = new ByteArrayOutputStream();
					baos.write(0x4);
					baos.write(pub_aff_x.toByteArray());
					baos.write(pub_aff_y.toByteArray());
					_pub2 = baos.toByteArray();
				}
				_prv2 = k_priv_key.getS().toByteArray();
			} while(_pub2.length != 65 || _prv2.length != 32);
			//
			//byte[] _priv = k_priv_key.getEncoded();
			//byte[] _pub = k_pub_key.getEncoded();
			if(_pub2 != null && _prv2 != null) {
				sessCtx.Put(SecretTagPool.tagSessionPrivateKey, _prv2);
				sessCtx.Put(SecretTagPool.tagSessionPublicKey, _pub2);
			}
			else {
				ok = false;
			}
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		} catch(NoSuchProviderException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHPROVIDER, exn.getMessage());
		} catch(InvalidAlgorithmParameterException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_INVALGPARAM, exn.getMessage());
		}
		return ok;
	}
	int SendHttpQuery(RoundTripBlock rtb, JobServerProtocol.StyloQFrame pack, byte [] cryptoKey) throws StyloQException
	{
		int    ok = 0;
		boolean test_ping = false; // Признак того, что команда является тестовым пингом сервера
		int    uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
		assert(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps);
		HttpURLConnection conn = null;
		Uri.Builder ub = new Uri.Builder();
		String _path = rtb.Url.getPath();
		if(_path.startsWith("/"))
			_path = _path.substring(1);
		if(SLib.GetLen(_path) == 0) { // @temporary
			_path = "styloq";
		}
		String _auth = rtb.Url.getAuthority();
		ub.scheme(rtb.Url.getScheme()).encodedAuthority(_auth).
			appendPath(_path).appendQueryParameter("rtsid", rtb.Uuid.toString());
		try {
			String content_buf = null;
			if(pack.H.Type == JobServerProtocol.PPSCMD_PING) {
				content_buf = "test-request-for-connection-checking";
				test_ping = true;
			}
			else
				content_buf = Base64.getEncoder().encodeToString(pack.D);
			URL nu = new URL(ub.build().toString());
			conn = (HttpURLConnection)nu.openConnection();
			conn.setDoInput(true);
			conn.setDoOutput(true);
			conn.setUseCaches(false);
			conn.setConnectTimeout(2000); // @?
			conn.setReadTimeout(60000); // @?
			conn.setRequestMethod("POST");
			conn.setRequestProperty("Content-Type", "application/octet-stream");
			conn.setRequestProperty("Content-Length", Integer.toString(content_buf.length()));
			conn.setRequestProperty("Accept-Encoding", "application/octet-stream");
			conn.setRequestProperty("connection", "close"); // Если этого нет, то следующее соединение не будет работать!
			conn.connect();

			OutputStream os = conn.getOutputStream();

			//OutputStream out = new DataOutputStream();
			os.write(content_buf.getBytes());
			os.flush();
			os.close();
			//
			String http_resp = conn.getResponseMessage();
			int http_result = conn.getResponseCode();
			//
			//Map<String, List<String>> ret_hf = conn.getHeaderFields();
			//
			String ret_str = "";
			{
				byte[] ret_raw = new byte[1024];
				InputStream is = conn.getInputStream();
				int b = is.read();
				int raw_count = 0;
				if(b != -1) do {
					if(raw_count >= ret_raw.length) {
						ret_str = ret_str + new String(ret_raw, 0, raw_count);
						raw_count = 0;
					}
					if(b != 0) {
						ret_raw[raw_count] = (byte) b;
						raw_count++;
					}
					b = is.read();
				} while(b != -1);
				if(raw_count > 0)
					ret_str = ret_str + new String(ret_raw, 0, raw_count);
				//InputStreamReader isr = new InputStreamReader(is);
				//BufferedReader br = new BufferedReader(isr);
				//String next_line = br.readLine();
				//while(SLib.GetLen(next_line) > 0) {
				//	ret_buf = ret_buf + next_line;
				//	next_line = br.readLine();
				//}
				//br.close();
				//isr.close();
				is.close();
			}
			//
			if(test_ping) {
				if(ret_str.equalsIgnoreCase("your-test-request-is-accepted"))
					ok = 1;
			}
			else if(ret_str.equalsIgnoreCase("(empty)")) {
				ok = 0;
			}
			else {
				byte[] ret_binary = Base64.getDecoder().decode(ret_str);
				pack.Read(ret_binary, cryptoKey);
				ok = 1;
			}
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		finally {
			conn.disconnect();
			conn = null;
		}
		/*
		SString temp_buf;
		ScURL  c;
		SString content_buf;
		SBuffer reply_buf;
		SFile wr_stream(reply_buf, SFile::mWrite);
		StrStrAssocArray hdr_flds;
		THROW(oneof2(rB.Url.GetProtocol(), InetUrl::protHttp, InetUrl::protHttps));
		rB.Url.SetQueryParam("rtsid", SLS.AcquireRvlStr().Cat(rB.Uuid, S_GUID::fmtIDL|S_GUID::fmtPlain|S_GUID::fmtLower));
		content_buf.Z().EncodeMime64(rPack.constptr(), rPack.GetAvailableSize());
		SFileFormat::GetMime(SFileFormat::Unkn, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentLen, temp_buf.Z().Cat(content_buf.Len()));
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, temp_buf);
		SFileFormat::GetContentTransferEncName(P_Cb->ContentTransfEncSFileFormat::cteBase64, temp_buf);
		THROW_SL(c.HttpPost(rB.Url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, content_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			THROW(p_ack_buf); // @todo error
			temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
			THROW(rPack.ReadMime64(temp_buf, pCryptoKey) > 0);
			ok = 1;
		}
		CATCHZOK
		*/
		return ok;
	}
	void SetRoundTripBlockReplyValues(RoundTripBlock rtb, MqbClient mqbc, MqbClient.RoutingParamEntry rpe, MqbClient.RoutingParamEntry rpeReply) throws StyloQException
	{
		THROW(mqbc != null && SLib.GetLen(rpe.ExchangeName) > 0 && SLib.GetLen(rpe.RoutingKey) > 0, ppstr2.PPERR_SQ_INVALIDMQBVALUES);
		rtb.Mqbc = mqbc;
		rtb.MqbRpe = rpe;
		rtb.MqbRpeReply = rpeReply;
	}
	private boolean PublishMqb(MqbClient mqbc, RoundTripBlock rtb, MqbClient.RoutingParamEntry rpe, byte [] data) throws StyloQException
	{
		boolean result = false;
		if(mqbc != null && rtb != null && rpe != null && SLib.GetLen(data) > 0) {
			final int cmto = MqbClient.__DefMqbConsumeTimeout;
			AMQP.BasicProperties.Builder bpb = new AMQP.BasicProperties.Builder();
			bpb.expiration(Integer.toString(cmto));
			if(rtb.Uuid != null)
				bpb.correlationId(rtb.Uuid.toString());
			bpb.expiration("120000"); // @v11.3.1
			AMQP.BasicProperties props = bpb.build();
			mqbc.Publish(rpe.ExchangeName, rpe.RoutingKey, props, data);
			result = true;
		}
		return result;
	}
	private GetResponse ConsumeMqb(MqbClient mqbc, RoundTripBlock rtb, String queueName, int cmto) throws StyloQException
	{
		GetResponse result = null;
		if(mqbc != null && SLib.GetLen(queueName) > 0) {
			String correlation_id = (rtb != null && rtb.Uuid != null) ? rtb.Uuid.toString() : null;
			result = mqbc.ConsumeMessage(queueName, correlation_id, cmto);
			if(result != null) {
				Envelope env = result.getEnvelope();
				if(env != null)
					mqbc.Ack(env.getDeliveryTag());
			}
		}
		return result;
	}
	public long Registration_ClientRequest(StyloQDatabase db, StyloQInterchange.RoundTripBlock rtb) throws StyloQException
	{
		long   new_id = 0;
		int    ok = 0;
		long   transmitted_face_id = 0L;
		boolean face_tramsitted = false;
		byte[] cli_ident = rtb.Sess.Get(SecretTagPool.tagClientIdent);
		byte[] cli_secret = rtb.Sess.Get(SecretTagPool.tagSecret);
		byte[] sess_secret = rtb.Sess.Get(SecretTagPool.tagSessionSecret);
		byte[] svc_ident = rtb.Other.Get(SecretTagPool.tagSvcIdent);
		String js_selfy_face = null;
		THROW(SLib.GetLen(cli_ident) > 0, ppstr2.PPERR_SQ_UNDEFCLIID);
		THROW(SLib.GetLen(svc_ident) > 0, ppstr2.PPERR_SQ_UNDEFSVCID);
		THROW(SLib.GetLen(cli_secret) > 0, ppstr2.PPERR_SQ_UNDEFSESSSECRET_INNER);
		THROW(SLib.GetLen(sess_secret) > 0, ppstr2.PPERR_SQ_UNDEFSESSSECRET);
		JobServerProtocol.StyloQFrame tp = new JobServerProtocol.StyloQFrame();
		//byte [] __s;
		//byte [] __v;
		final int _cmd_id = JobServerProtocol.PPSCMD_SQ_SRPREGISTER;
		String user_name_text = Base64.getEncoder().encodeToString(cli_ident);
		SRP.SaltedVerificationKey svk = SRP.CreateSaltedVerificationKey(SRP.HashAlgorithm.SRP_SHA1, SRP.NGType.SRP_NG_8192,
			user_name_text, cli_secret, null, null);
		tp.StartWriting(_cmd_id, JobServerProtocol.StyloQFrame.psubtypeForward);
		tp.P.Put(SecretTagPool.tagClientIdent, cli_ident);
		{
			// Вставляем наш лик для представления перед сервисом
			if(rtb.SelfyFace == null) {
				//rtb.SelfyFace = SelectFaceForSvc(db, svc_ident, rtb.StP);
				rtb.SelfyFace = GetFaceForTransmission(db, svc_ident, rtb.StP, true);
			}
			if(rtb.SelfyFace != null) {
				js_selfy_face = rtb.SelfyFace.ToJson();
				if(js_selfy_face != null) {
					tp.P.Put(SecretTagPool.tagFace, js_selfy_face.getBytes());
					transmitted_face_id = rtb.SelfyFace.ID;
				}
			}
		}
		tp.P.Put(SecretTagPool.tagSrpVerifier, svk.V);
		tp.P.Put(SecretTagPool.tagSrpVerifierSalt, svk.Salt);
		//tp.P.Put(SecretTagPool.tagSecret, cli_secret); // @debug do remove after debugging
		tp.FinishWriting(sess_secret);

		int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
		if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
			if(SendHttpQuery(rtb, tp, sess_secret) > 0) {
				ok = 1;
			}
		}
		else if(uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) {
			THROW(rtb.Mqbc != null && rtb.MqbRpe != null && rtb.MqbRpeReply != null, ppstr2.PPERR_SQ_INVALIDMQBVALUES);
			final int cmto = MqbClient.__DefMqbConsumeTimeout;
			// @v11.3.1 AMQP.BasicProperties props = new AMQP.BasicProperties.Builder().expiration(Integer.toString(cmto)).build();
			// @v11.3.1 rtb.Mqbc.Publish(rtb.MqbRpe.ExchangeName, rtb.MqbRpe.RoutingKey, props, tp.D);
			PublishMqb(rtb.Mqbc, rtb, rtb.MqbRpe, tp.D); // @v11.3.1
			// @v11.3.1 GetResponse resp = rtb.Mqbc.ConsumeMessage(rtb.MqbRpeReply.QueueName, null, cmto);
			GetResponse resp = ConsumeMqb(rtb.Mqbc, rtb, rtb.MqbRpeReply.QueueName, cmto); // @v11.3.1
			THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOREGQUERY, Integer.toString(cmto));
			// @v11.3.1 Envelope env = resp.getEnvelope();
			// @v11.3.1 rtb.Mqbc.Ack(env.getDeliveryTag());
			tp.Read(resp.getBody(), sess_secret);
			if(tp.H.Type == _cmd_id && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
				if(!tp.CheckRepError(Ctx)) {
					; // @todo сообщить об ошибке вызывающей функции
				}
				else {
					/*
					// @debug Тестируем SRP-верификацию {
					boolean debug_test_is_ok = false;
					{
						String debug_user_name_text = Base64.getEncoder().encodeToString(cli_ident);
						SRP.User debug_usr = new SRP.User(SRP.HashAlgorithm.SRP_SHA1, SRP.NGType.SRP_NG_8192,
								debug_user_name_text, cli_secret, null, null);
						byte [] debug__a = debug_usr.StartAuthentication();
						SRP.Verifier debug_ver = new SRP.Verifier(SRP.HashAlgorithm.SRP_SHA1, SRP.NGType.SRP_NG_8192,
								null, null, debug_user_name_text, svk.Salt, svk.V, debug__a);
						if(SLib.GetLen(debug_ver.GetBytesB()) > 0) {
							byte [] debug__m = debug_usr.ProcessChallenge(svk.GetSalt(), debug_ver.GetBytesB());
							if(debug__m != null) {
								byte [] debug__hamk = debug_ver.VerifySession(debug__m);
								if(debug__hamk != null) {
									if(debug_usr.VerifySession(debug__hamk)) {
										debug_test_is_ok = true;
									}
								}
							}
						}
					}
					// } @debug Тестируем SRP-верификацию
					 */
					ok = 1;
				}
			}
		}
		if(ok > 0) {
			if(transmitted_face_id > 0)
				face_tramsitted = true;
			long ex_id = 0;
			StyloQDatabase.SecStoragePacket stp = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svc_ident);
			if(stp != null) {
				assert(stp.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService);
				ex_id = stp.Rec.ID;
			}
			else {
				stp = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kForeignService);
				stp.Rec.BI = svc_ident;
			}
			stp.Pool.Put(SecretTagPool.tagSvcIdent, svc_ident);
			stp.Pool.Put(SecretTagPool.tagClientIdent, cli_ident);
			stp.Pool.Put(SecretTagPool.tagSrpVerifier, svk.V);
			stp.Pool.Put(SecretTagPool.tagSrpVerifierSalt, svk.Salt);
			//stp.Pool.Put(SecretTagPool.tagSecret, cli_secret); // @debug do remove after debugging
			{
				byte[] other_face_bytes = tp.P.Get(SecretTagPool.tagFace);
				if(SLib.GetLen(other_face_bytes) > 0) {
					StyloQFace other_face = new StyloQFace();
					String js_face = new String(other_face_bytes);
					if(other_face.FromJson(js_face)) {
						stp.Pool.Put(SecretTagPool.tagFace, other_face_bytes);
						rtb.OtherFace = other_face;
					}
				}
				if(js_selfy_face != null)
					stp.Pool.Put(SecretTagPool.tagSelfyFace, js_selfy_face.getBytes());
			}
			{
				byte[] other_cfg_bytes = tp.P.Get(SecretTagPool.tagConfig);
				if(SLib.GetLen(other_cfg_bytes) > 0) {
					StyloQConfig other_cfg = new StyloQConfig();
					String js_cfg = new String(other_cfg_bytes);
					if(other_cfg.FromJson(js_cfg))
						stp.Pool.Put(SecretTagPool.tagConfig, other_cfg_bytes);
				}
			}
			{
				Database.Transaction tra = new Database.Transaction(db, true);
				new_id = db.PutPeerEntry(ex_id, stp, false);
				if(new_id > 0) {
					if(face_tramsitted) {
						db.LogEvent(SLib.PPACN_STYLOQFACETRANSMITTED, PPOBJ_STYLOQBINDERY, new_id, transmitted_face_id, false);
					}
					tra.Commit();
				}
				else
					tra.Abort();
			}
		}
		return new_id;
	}
	public boolean KexClientRequest(StyloQDatabase db, StyloQInterchange.RoundTripBlock rtb) throws StyloQException
	{
		//
		// Инициирующий запрос: устанавливает ключи шифрования для дальнейшего диалога с сервисом.
		//
		boolean ok = false;
		try {
			byte[] svc_acsp = rtb.Other.Get(SecretTagPool.tagSvcAccessPoint);
			byte[] svc_ident = rtb.Other.Get(SecretTagPool.tagSvcIdent);
			THROW(SLib.GetLen(svc_acsp) > 0, ppstr2.PPERR_SQ_UNDEFSVCACCSPOINT);
			THROW(SLib.GetLen(svc_ident) > 0, ppstr2.PPERR_SQ_UNDEFSVCID);
			//String url_buf = new String(svc_acsp);
			//URI uri = new URI(url_buf);
			//
			int fsks = FetchSessionKeys(StyloQDatabase.SecStoragePacket.kForeignService, db, rtb.Sess, svc_ident);
			if(fsks != fsksError) {
				//
				KexGenerateKeys(rtb.Sess);
				byte[] own_ident = rtb.Sess.Get(SecretTagPool.tagClientIdent);
				byte[] sess_pub_key = rtb.Sess.Get(SecretTagPool.tagSessionPublicKey);
				//
				JobServerProtocol.StyloQFrame tp = new JobServerProtocol.StyloQFrame();
				{
					tp.StartWriting(JobServerProtocol.PPSCMD_SQ_ACQUAINTANCE, JobServerProtocol.StyloQFrame.psubtypeForward);
					tp.P.Put(SecretTagPool.tagSvcIdent, svc_ident);
					tp.P.Put(SecretTagPool.tagClientIdent, own_ident);
					tp.P.Put(SecretTagPool.tagSessionPublicKey, sess_pub_key);
					tp.FinishWriting(null);
				}
				//
				int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
				if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
					if(SendHttpQuery(rtb, tp, null) > 0) {
						byte[] other_sess_public = tp.P.Get(SecretTagPool.tagSessionPublicKey);
						rtb.Other.Put(SecretTagPool.tagSessionPublicKey, other_sess_public);
						KexGenerageSecret(rtb.Sess, rtb.Other);
						// Теперь ключ шифрования сессии есть и у нас и у сервиса!
						ok = true;
					}
				}
				else if(uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) {
					MqbClient.RoutingParamEntry rpe = null;
					MqbClient.InitParam mqip = new MqbClient.InitParam(rtb.Url, rtb.MqbAuth, rtb.MqbSecret);
					// (done by MqbClient.InitParam) mqip.Host = uri.getHost();
					// (done by MqbClient.InitParam) mqip.Port = uri.getPort();
					{
						rpe = new MqbClient.RoutingParamEntry();
						MqbClient.RoutingParamEntry reply_rpe = mqip.CreateNewConsumeParamEntry();
						rpe.SetupStyloQRpc(sess_pub_key, svc_ident, rtb.Other.Get(SecretTagPool.tagSvcLoclAddendum), reply_rpe);
						MqbClient mqbc = new MqbClient(mqip);
						final int cmto = MqbClient.__DefMqbConsumeTimeout;
						// @v11.3.1 AMQP.BasicProperties props = new AMQP.BasicProperties.Builder().expiration(Integer.toString(cmto)).build();
						// @v11.3.1 mqbc.Publish(rpe.ExchangeName, rpe.RoutingKey, props, tp.D);
						PublishMqb(mqbc, rtb, rpe, tp.D); // @v11.3.1
						// @v11.3.1 GetResponse resp = mqbc.ConsumeMessage(reply_rpe.QueueName, null, cmto);
						GetResponse resp = ConsumeMqb(mqbc, rtb, reply_rpe.QueueName, cmto); // @v11.3.1
						THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOKEXQUERY, Integer.toString(cmto));
						// @v11.3.1 Envelope env = resp.getEnvelope();
						// @v11.3.1 mqbc.Ack(env.getDeliveryTag());
						byte[] body = resp.getBody();
						tp.Read(body, null);
						byte[] other_sess_public = tp.P.Get(SecretTagPool.tagSessionPublicKey);
						rtb.Other.Put(SecretTagPool.tagSessionPublicKey, other_sess_public);
						KexGenerageSecret(rtb.Sess, rtb.Other);
						// Теперь ключ шифрования сессии есть и у нас и у сервиса!
						MqbClient.RoutingParamEntry rpe_regular = new MqbClient.RoutingParamEntry();
						rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, null, reply_rpe);
						// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
						SetRoundTripBlockReplyValues(rtb, mqbc, rpe_regular, reply_rpe);
						ok = true; // SUCCESS!
					}
				}
			}
		} catch(URISyntaxException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
		return ok;
	}
	private JobServerProtocol.StyloQFrame CreateSrpPacket_Cli_Auth(SRP.User usr, final byte [] svcIdent, final byte [] cliIdent,
		final byte [] sessPubKey, final byte [] sessSecret) throws StyloQException
	{
		JobServerProtocol.StyloQFrame result = new JobServerProtocol.StyloQFrame();
		byte[] __a = usr.StartAuthentication();
		// User -> Host: (ident, __a)
		result.StartWriting(JobServerProtocol.PPSCMD_SQ_SRPAUTH, 0);
		//if(kex_generated) {
			//assert(sess_pub_key.Len()); // Мы его только что сгенерировали (see above)
			result.P.Put(SecretTagPool.tagSessionPublicKey, sessPubKey);
		//}
		result.P.Put(SecretTagPool.tagSrpA, __a);
		result.P.Put(SecretTagPool.tagClientIdent, cliIdent);
		result.P.Put(SecretTagPool.tagSvcIdent, svcIdent);
		//assert(!p_sess_secret || p_sess_secret->Len());
		//assert(p_sess_secret || tp.P.Get(SSecretTagPool::tagSessionPublicKey, 0));
		result.FinishWriting(sessSecret);
		return result;
	}
	private JobServerProtocol.StyloQFrame CreateSrpPacket_Cli_Auth2(final byte [] __m, final byte [] cliIdent, boolean rSrpProtocolFault) throws StyloQException
	{
		JobServerProtocol.StyloQFrame result = new JobServerProtocol.StyloQFrame();
		if(SLib.GetLen(__m) <= 0) { // @error User SRP-6a safety check violation
			rSrpProtocolFault = true;
			result.StartWriting(JobServerProtocol.PPSCMD_SQ_SRPAUTH_S2, JobServerProtocol.StyloQFrame.psubtypeForwardError);
			// Здесь текст и код ошибки нужны
		}
		else {
			// User -> Host: (bytes_M)
			result.StartWriting(JobServerProtocol.PPSCMD_SQ_SRPAUTH_S2, 0);
			result.P.Put(SecretTagPool.tagSrpM, __m);
			result.P.Put(SecretTagPool.tagClientIdent, cliIdent);
			result.FinishWriting(null); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
		}
		return result;
	}
	private JobServerProtocol.StyloQFrame CreateSrpPacket_Cli_HAMK(SRP.User usr, final byte [] hamk, boolean rSrpProtocolFault) throws StyloQException
	{
		JobServerProtocol.StyloQFrame result = null;
		if(SLib.GetLen(hamk) > 0) {
			result = new JobServerProtocol.StyloQFrame();
			if(!usr.VerifySession(hamk)) { // @error Server authentication failed
				rSrpProtocolFault = true;
				result.StartWriting(JobServerProtocol.PPSCMD_SQ_SRPAUTH_ACK, JobServerProtocol.StyloQFrame.psubtypeForwardError);
				// Здесь текст и код ошибки нужны
			}
			else {
				final int sess_expiry_period = GetNominalSessionLifeTimeSec();
				result.StartWriting(JobServerProtocol.PPSCMD_SQ_SRPAUTH_ACK, JobServerProtocol.StyloQFrame.psubtypeForward);
				byte[] bytes = ByteBuffer.allocate(4).putInt(sess_expiry_period).array();
				result.P.Put(SecretTagPool.tagSessionExpirPeriodSec, bytes);
			}
			result.FinishWriting(null); // несмотря на то, что у нас есть теперь ключ шифрования, этот roundtrip завершаем без шифровки пакетов
		}
		return result;
	}
	public boolean Verification_ClientRequest(StyloQDatabase db, StyloQInterchange.RoundTripBlock rtb) throws StyloQException
	{
		//
		// Это - инициирующий запрос и одновременно с авторизацией устанавливает ключи обмена.
		// Все фазы SRP-авторизации осуществляются без шифрования!
		//
		boolean ok = false;
		MqbClient mqbc = null;
		try {
			int svc_session_expiry_period = 0;
			int cli_session_expiry_period = 0;
			byte[] temp_chunk = null;
			byte[] other_sess_public = null;
			byte[] _sess_secret = null;
			//boolean kex_generated = false;
			boolean srp_protocol_fault = false; // Если !0 то возникла ошибка в верификации. Такие ошибки обрабатываются специальным образом.
			byte[] svc_ident = rtb.Other.Get(SecretTagPool.tagSvcIdent);
			byte[] sess_secret = rtb.Sess.Get(SecretTagPool.tagSessionSecret);
			byte[] sess_pub_key = null;
			if(sess_secret == null) {
				KexGenerateKeys(rtb.Sess);
				sess_pub_key = rtb.Sess.Get(SecretTagPool.tagSessionPublicKey);
				THROW(SLib.GetLen(sess_pub_key) > 0, ppstr2.PPERR_SQ_UNDEFSESSPUBKEY_INNER);
				//kex_generated = true;
			}
			byte[] cli_ident = rtb.Sess.Get(SecretTagPool.tagClientIdent);
			byte[] cli_secret = rtb.Sess.Get(SecretTagPool.tagSecret);
			final String cli_ident_text = Base64.getEncoder().encodeToString(cli_ident);
			THROW(SLib.GetLen(cli_ident) > 0, ppstr2.PPERR_SQ_UNDEFCLIID);
			THROW(SLib.GetLen(cli_secret) > 0, ppstr2.PPERR_SQ_UNDEFSESSSECRET_INNER);
			//
			SRP.User usr = new SRP.User(SRP.HashAlgorithm.SRP_SHA1, SRP.NGType.SRP_NG_8192, cli_ident_text, cli_secret, null, null);
			JobServerProtocol.StyloQFrame tp = CreateSrpPacket_Cli_Auth(usr, svc_ident, cli_ident, sess_pub_key, sess_secret);
			//
			int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
			if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
				if(SendHttpQuery(rtb, tp, sess_secret) > 0) {
					if(!tp.CheckRepError(Ctx)) {
						// Сервис вернул ошибку: можно уходить - верификация не пройдена
					}
					else if(tp.H.Type == JobServerProtocol.PPSCMD_SQ_SRPAUTH && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
						other_sess_public = tp.P.Get(SecretTagPool.tagSessionPublicKey);
						KexGenerageSecret(rtb.Sess, tp.P);
						_sess_secret = rtb.Sess.Get(SecretTagPool.tagSessionSecret);
						// Теперь ключ шифрования сессии есть и у нас и у сервиса!
						byte[] __b = tp.P.Get(SecretTagPool.tagSrpB);
						byte[] srp_s = tp.P.Get(SecretTagPool.tagSrpVerifierSalt);
						byte[] __m = usr.ProcessChallenge(srp_s, __b);
						tp = CreateSrpPacket_Cli_Auth2(__m, cli_ident, srp_protocol_fault); // User -> Host: (bytes_M)
						if(!srp_protocol_fault) {
							if(SendHttpQuery(rtb, tp, sess_secret) > 0) {
								if(!tp.CheckRepError(Ctx)) {
									// Сервис вернул ошибку: можно уходить - верификация не пройдена
								}
								else if(tp.H.Type == JobServerProtocol.PPSCMD_SQ_SRPAUTH_S2 && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
									byte[] __hamk = tp.P.Get(SecretTagPool.tagSrpHAMK);
									tp = CreateSrpPacket_Cli_HAMK(usr, __hamk, srp_protocol_fault);
									if(tp != null) {
										if(SendHttpQuery(rtb, tp, sess_secret) > 0) {
											// Это было завершающее сообщение. Если все OK то сервис ждет от нас команды, если нет, то можно уходить - свадьбы не будет
											if(!srp_protocol_fault) {
												temp_chunk= tp.P.Get(SecretTagPool.tagSessionExpirPeriodSec);
												if(SLib.GetLen(temp_chunk) == 4)
													svc_session_expiry_period = SLib.BytesToInt(temp_chunk, 0);
												ok = true;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			else if(uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) {
				MqbClient.RoutingParamEntry rpe_init = null;
				MqbClient.RoutingParamEntry rpe_regular = null;
				MqbClient.RoutingParamEntry reply_rpe = null;
				//String n_hex = new String("");
				//String g_hex = new String("");
				if(rtb.Mqbc != null && rtb.MqbRpe != null) {
					mqbc = rtb.Mqbc;
					rpe_init = rtb.MqbRpe;
				}
				if(mqbc == null) {
					//byte[] svc_acsp = rtb.Other.Get(SecretTagPool.tagSvcAccessPoint);
					//if(svc_acsp != null) {
					MqbClient.InitParam mqip = new MqbClient.InitParam(rtb.Url, rtb.MqbAuth, rtb.MqbSecret);
					// (done by MqbClient.InitParam) mqip.Host = uri.getHost();
					// (done by MqbClient.InitParam) mqip.Port = uri.getPort();
					rpe_init = new MqbClient.RoutingParamEntry();
					reply_rpe = mqip.CreateNewConsumeParamEntry();
					rpe_init.SetupStyloQRpc(sess_pub_key, svc_ident, rtb.Other.Get(SecretTagPool.tagSvcLoclAddendum), reply_rpe);
					mqbc = new MqbClient(mqip);
					//}
				}
				if(mqbc != null) {
					{
						final int cmto1 = MqbClient.__DefMqbConsumeTimeout;
						PublishMqb(mqbc, rtb, rpe_init, tp.D); // @v11.3.1
						//debug_ver.VerifySession(static_cast<const uchar *>(__m.PtrC()), &p_bytes_HAMK);
						GetResponse resp = ConsumeMqb(mqbc, rtb, reply_rpe.QueueName, cmto1); // @v11.3.1
						THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOSRPAUTH1, Integer.toString(cmto1));
						tp.Read(resp.getBody(), sess_secret);
					}
					if(!tp.CheckRepError(Ctx)) {
						// Сервис вернул ошибку: можно уходить - верификация не пройдена
					}
					else if(tp.H.Type == JobServerProtocol.PPSCMD_SQ_SRPAUTH && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
						other_sess_public = tp.P.Get(SecretTagPool.tagSessionPublicKey);
						KexGenerageSecret(rtb.Sess, tp.P);
						_sess_secret = rtb.Sess.Get(SecretTagPool.tagSessionSecret);
						// Теперь ключ шифрования сессии есть и у нас и у сервиса!
						byte[] __b = tp.P.Get(SecretTagPool.tagSrpB);
						byte[] srp_s = tp.P.Get(SecretTagPool.tagSrpVerifierSalt);
						byte[] __m = usr.ProcessChallenge(srp_s, __b);
						rpe_regular = new MqbClient.RoutingParamEntry();
						rpe_regular.SetupStyloQRpc(sess_pub_key, other_sess_public, null, reply_rpe);
						tp = CreateSrpPacket_Cli_Auth2(__m, cli_ident, srp_protocol_fault); // User -> Host: (bytes_M)
						if(!srp_protocol_fault) {
							final int cmto2 = MqbClient.__DefMqbConsumeTimeout;
							PublishMqb(mqbc, rtb, rpe_regular, tp.D); // @v11.3.1
							GetResponse resp = ConsumeMqb(mqbc, rtb, reply_rpe.QueueName, cmto2); // @v11.3.1
							THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOSRPAUTH2, Integer.toString(cmto2));
							tp.Read(resp.getBody(), sess_secret);
							if(!tp.CheckRepError(Ctx)) {
								// Сервис вернул ошибку: можно уходить - верификация не пройдена
							}
							else if(tp.H.Type == JobServerProtocol.PPSCMD_SQ_SRPAUTH_S2 && (tp.H.Flags & JobServerProtocol.hfAck) != 0) {
								byte[] __hamk = tp.P.Get(SecretTagPool.tagSrpHAMK);
								tp = CreateSrpPacket_Cli_HAMK(usr, __hamk, srp_protocol_fault);
								if(tp != null) {
									PublishMqb(mqbc, rtb, rpe_regular, tp.D); // @v11.3.1
									resp = ConsumeMqb(mqbc, rtb, reply_rpe.QueueName, cmto2); // @v11.3.1
									THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOSRPHAMK, Integer.toString(cmto2));
									tp.Read(resp.getBody(), sess_secret);
									if(!tp.CheckRepError(Ctx)) {
										// Сервис вернул ошибку: так, вообще-то, быть не может - это должно было быть завершающее подтверждение
									}
									else if(!srp_protocol_fault) {
										temp_chunk = tp.P.Get(SecretTagPool.tagSessionExpirPeriodSec);
										if(SLib.GetLen(temp_chunk) == 4)
											svc_session_expiry_period = SLib.BytesToInt(temp_chunk, 0);
										// Устанавливаем факторы RoundTrimBlock, которые нам понадобяться при последующих обменах
										SetRoundTripBlockReplyValues(rtb, mqbc, rpe_regular, reply_rpe);
										ok = true;
										// Это было завершающее сообщение. Если все OK то сервис ждет от нас команды, если нет, то можно уходить - свадьбы не будет
									}
								}
							}
						}
					}
				}
			}
			if(ok) {
				long sess_id = 0;
				StyloQDatabase.SecStoragePacket sess_pack = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kSession);
				// Теперь надо сохранить параметры сессии дабы в следующий раз не проделывать столь сложную процедуру
				//
				// Проверки assert'ами (не THROW) реализуются из-за того, что не должно возникнуть ситуации, когда мы
				// попали в этот участок кода с невыполненными условиями (то есть при необходимости THROW должны были быть вызваны выше).
				//assert(rB.Sess.Get(SSecretTagPool::tagSessionSecret, &temp_chunk));
				//assert(temp_chunk == _sess_secret);
				//assert(sess_pub_key.Len());
				//assert(other_sess_public.Len());
				//assert(rB.Sess.Get(SSecretTagPool::tagSessionPrivateKey, 0));
				sess_pack.Pool.Put(SecretTagPool.tagSessionPublicKey, sess_pub_key);
				{
					temp_chunk = rtb.Sess.Get(SecretTagPool.tagSessionPrivateKey);
					sess_pack.Pool.Put(SecretTagPool.tagSessionPrivateKey, temp_chunk);
				}
				sess_pack.Pool.Put(SecretTagPool.tagSessionPublicKeyOther, other_sess_public);
				sess_pack.Pool.Put(SecretTagPool.tagSessionSecret, _sess_secret);
				sess_pack.Pool.Put(SecretTagPool.tagSvcIdent, svc_ident);
				if(cli_session_expiry_period > 0 || svc_session_expiry_period > 0) {
					int sep = 0;
					if(cli_session_expiry_period <= 0 || svc_session_expiry_period <= 0)
						sep = Math.max(cli_session_expiry_period, svc_session_expiry_period);
					else
						sep = Math.min(cli_session_expiry_period, svc_session_expiry_period);
					sess_pack.Rec.Expiration = EvaluateExpiryTime(sep);
				}
				sess_id = db.StoreSession(0, sess_pack, true);
				if(sess_id < 0)
					ok = false;
			}
		} catch(URISyntaxException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
		return ok;
	}
	SecretTagPool Command_ClientRequest(RoundTripBlock rtb, String cmdJson, SecretTagPool additionalItemsToSend) throws StyloQException
	{
		boolean ok = false;
		SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
		byte[] sess_secret = rtb.Sess.Get(SecretTagPool.tagSessionSecret);
		THROW(SLib.GetLen(sess_secret) > 0, ppstr2.PPERR_SQ_UNDEFSESSSECRET_INNER);
		THROW(SLib.GetLen(cmdJson) > 0, ppstr2.PPERR_SQ_UNDEFSVCCOMMAND);
		JobServerProtocol.StyloQFrame tp = new JobServerProtocol.StyloQFrame();
		tp.StartWriting(JobServerProtocol.PPSCMD_SQ_COMMAND, JobServerProtocol.StyloQFrame.psubtypeForward);
		tp.P.Put(SecretTagPool.tagRawData, cmdJson.getBytes(StandardCharsets.UTF_8), ds);
		if(additionalItemsToSend != null && additionalItemsToSend.L != null) {
			for(int i = 0; i < additionalItemsToSend.L.size(); i++) {
				SecretTagPool.Entry entry = additionalItemsToSend.L.get(i);
				if(entry != null && SLib.GetLen(entry.Data) > 0 && entry.I != SecretTagPool.tagRawData)
					tp.P.Put(entry.I, entry.Data);
			}
		}
		tp.FinishWriting(sess_secret);
		int uriprot = SLib.GetUriSchemeId(rtb.Url.getScheme());
		if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
			THROW(SendHttpQuery(rtb, tp, sess_secret) != 0, ppstr2.PPERR_SQ_NOSVCRESPTOCMD, "");
		}
		else {
			boolean more = false;
			int wait_more_ms = 0;
			int poll_interval_ms = 0;
			THROW(rtb.Mqbc != null && rtb.MqbRpe != null && rtb.MqbRpeReply != null, ppstr2.PPERR_SQ_INVALIDMQBVALUES);
			final int cmto = MqbClient.__DefMqbConsumeTimeout;
			// @v11.3.1 AMQP.BasicProperties props = new AMQP.BasicProperties.Builder().expiration(Integer.toString(cmto)).build();
			// @v11.3.1 rtb.Mqbc.Publish(rtb.MqbRpe.ExchangeName, rtb.MqbRpe.RoutingKey, props, tp.D);
			PublishMqb(rtb.Mqbc, rtb, rtb.MqbRpe, tp.D); // @v11.3.1
			do {
				more = false;
				// @v11.3.1 GetResponse resp = rtb.Mqbc.ConsumeMessage(rtb.MqbRpeReply.QueueName, null, (wait_more_ms > 0) ? wait_more_ms : cmto);
				GetResponse resp = ConsumeMqb(rtb.Mqbc, rtb, rtb.MqbRpeReply.QueueName, (wait_more_ms > 0) ? wait_more_ms : cmto); // @v11.3.1
				THROW(resp != null, ppstr2.PPERR_SQ_NOSVCRESPTOCMD, Integer.toString(cmto));
				// @v11.3.1 Envelope env = resp.getEnvelope();
				// @v11.3.1 rtb.Mqbc.Ack(env.getDeliveryTag());
				tp.Read(resp.getBody(), sess_secret);
				//
				if((tp.H.Flags & JobServerProtocol.hfInformer) != 0) {
					JSONObject jsobj = tp.P.GetJsonObject(SecretTagPool.tagRawData);
					if(jsobj != null) {
						wait_more_ms = jsobj.optInt("waitms", 0);
						poll_interval_ms = jsobj.optInt("pollintervalms", 0);
						more = true;
					}
				}
			} while(more);
		}
		return tp.P;
		/*if(!tp.CheckRepError()) {
			// @todo error
			return null;
		}
		else {
			return tp.P;
		}*/
	}
	public static void GeneratePublicIdent(final SecretTagPool ownPool, byte[] svcIdent, int resultIdentTag, int flags, SecretTagPool resultPool) throws StyloQException
	{
		assert (resultIdentTag == SecretTagPool.tagSvcIdent || resultIdentTag == SecretTagPool.tagClientIdent);
		try {
			byte[] prmrn = ownPool.Get(SecretTagPool.tagPrimaryRN);
			byte[] ag = ownPool.Get(SecretTagPool.tagAG);
			THROW(SLib.GetLen(prmrn) > 0, ppstr2.PPERR_SQ_UNDEFINNERPRNTAG);
			THROW(SLib.GetLen(ag) > 0, ppstr2.PPERR_SQ_UNDEFINNERAGTAG);
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			baos.write(prmrn);
			baos.write(svcIdent);
			baos.write(ag);
			MessageDigest digest = MessageDigest.getInstance("SHA-1");
			digest.update(baos.toByteArray());
			byte[] hash = digest.digest();
			assert (hash.length == 20);
			resultPool.Put(resultIdentTag, hash);
			if((flags & gcisfMakeSecret) != 0) {
				digest.reset();
				baos.reset();
				baos.write(ag);
				baos.write(svcIdent);
				digest.update(baos.toByteArray());
				byte[] secret_hash = digest.digest();
				assert (secret_hash.length == 20);
				resultPool.Put(SecretTagPool.tagSecret, secret_hash);
			}
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
	}
	//
	// Descr: Структура, используемая для синхронизации атрибутов документа при обмене с сервисом.
	//    На текущий момент (@v11.3.12) очень приблизительная и потребует значительного пересмотра.
	//
	public static class DocumentRequestEntry {
		enum AcceptionResult {
			Unseen,     // не акцептировалось
			Skipped,    // пропущено: функция AcceptDocumentRequestList видела этот элемент, но посчитала что ничего даже пытаться делать не надо
			Successed,  // успешно акцептировано
			Unchanged,  // состояние документа в базе данных уже соответствует тому, что запрошено этим элементом
			Error       // ошибка при попытке акцепта
		}
		DocumentRequestEntry()
		{
			DocUUID = null;
			DocID = 0;
			RemoteDocID = 0;
			AfterTransmitStatus = 0;
			RemoteStatus = 0;
			DbAcceptStatus = AcceptionResult.Unseen;
		}
		UUID   DocUUID;
		long   DocID;               // Идентификатор документа в локальной базе данных
		long   RemoteDocID;         // Идентификатор документа у контрагента
		int    AfterTransmitStatus; // Статус, который должен быть установлен у документа после успешной передачи
		int    RemoteStatus;        // Статус документа у контрагента
		// ...
		AcceptionResult DbAcceptStatus; // Статус акцепта локальной базой данных данных этой структуры
	}
	public static class RequestBlock {
		RequestBlock(StyloQDatabase.SecStoragePacket svcPack, JSONObject jsQuery, StyloQCommand.Item orgCmdItem)
		{
			SvcPack = svcPack;
			Doc = null;
			JsCmd = jsQuery;
			OrgCmdItem = orgCmdItem;
			RetrActivity = null;
			DocReqList = null;
		}
		StyloQDatabase.SecStoragePacket SvcPack;
		Document Doc;
		JSONObject JsCmd;
		StyloQCommand.Item OrgCmdItem;
		SLib.SlActivity RetrActivity;
		ArrayList <DocumentRequestEntry> DocReqList; // Массив структур для синхронизации состояний документов с сервисом
	}
	static class DoInterchangeParam {
		DoInterchangeParam(byte [] svcIdent)
		{
			SvcIdent = svcIdent;
			LoclAddendum = null;
			SvcCapabilities = 0;
			OriginalCmdItem = null;
			RetrActivity_ = null;
			AccsPoint = null;
			MqbAuth = null;
			MqbSecret = null;
			CommandJson = null;
			DocReqList = null;
		}
		byte [] SvcIdent;
		byte [] LoclAddendum;
		int    SvcCapabilities;
		StyloQCommand.Item OriginalCmdItem; // Если обмен осуществляется с целью исполнения команды,
			// предоставленной сервисом, то это поле содержи ссылку на элемент команды.
			// После выполнения команды ссылка попадет в блок отображения результатов для
			// того, чтобы тот мог обработать результат каким-либо специфическим образом.
		SLib.SlActivity RetrActivity_; // activity в которую необходимо вернуть результат исполнения //
		String AccsPoint;
		String MqbAuth;
		String MqbSecret;
		String CommandJson;
		ArrayList <DocumentRequestEntry> DocReqList; // Массив структур для синхронизации состояний документов с сервисом
	}
	private static int QueryConfigAndSetFaceIfNeeded(StyloQApp appCtx, StyloQInterchange ic, StyloQInterchange.RoundTripBlock rtb) throws StyloQException
	{
		int    result = -1;
		if(rtb != null && rtb.InnerSvcID > 0) {
			StyloQDatabase dbs = appCtx.GetDB();
			if(dbs != null) {
				StyloQDatabase.SecStoragePacket svc_pack = dbs.GetPeerEntry(rtb.InnerSvcID);
				if(svc_pack != null && svc_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
					//
					long transmitted_face_id = 0;
					boolean my_face_tramsitted = false;
					StyloQFace my_face_to_transmit = null;
					JSONObject jsobj_my_face_to_transmit = null;
					final byte [] svc_ident = svc_pack.Pool.Get(SecretTagPool.tagSvcIdent);
					if(SLib.GetLen(svc_ident) > 0) {
						// Вставляем наш лик для представления перед сервисом
						my_face_to_transmit = ic.GetFaceForTransmission(dbs, svc_ident, rtb.StP, false);
						if(my_face_to_transmit != null) {
							jsobj_my_face_to_transmit = my_face_to_transmit.ToJsonObj();
							if(jsobj_my_face_to_transmit != null) {
								//tp.P.Put(SecretTagPool.tagFace, js_selfy_face.getBytes());
								transmitted_face_id = my_face_to_transmit.ID;
							}
						}
					}
					//
					byte [] face_bytes = svc_pack.Pool.Get(SecretTagPool.tagFace);
					byte [] cfg_bytes = svc_pack.Pool.Get(SecretTagPool.tagConfig);
					boolean do_query_cfg = true;
					boolean do_query_face = true;
					SecretTagPool additional_pool_to_send = null;
					if(SLib.GetLen(face_bytes) > 0) {
						StyloQFace current_face = new StyloQFace();
						if(current_face.FromJson(new String(face_bytes))) {
							String ees_text = current_face.Get(StyloQFace.tagExpiryEpochSec, 0);
							if(SLib.GetLen(ees_text) > 0 && !IsExpired(Long.parseLong(ees_text)))
								do_query_face = false;
						}
					}
					if(SLib.GetLen(cfg_bytes) > 0) {
						StyloQConfig current_cfg = new StyloQConfig();
						if(current_cfg.FromJson(new String(cfg_bytes))) {
							String ees_text = current_cfg.Get(StyloQConfig.tagExpiryEpochSec);
							if(SLib.GetLen(ees_text) > 0 && !IsExpired(Long.parseLong(ees_text)))
								do_query_cfg = false;
						}
					}
					if(do_query_face || do_query_cfg) {
						JSONObject js_query = new JSONObject();
						if(jsobj_my_face_to_transmit != null) {
							String jstxt_my_face_to_transmit = jsobj_my_face_to_transmit.toString();
							if(SLib.GetLen(jstxt_my_face_to_transmit) > 0) {
								SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
								additional_pool_to_send = new SecretTagPool();
								additional_pool_to_send.Put(SecretTagPool.tagFace, jstxt_my_face_to_transmit.getBytes(), ds);
								my_face_tramsitted = true;
							}
						}
						try {
							js_query.put("cmd", "GetConfig");
							String jsq_text = js_query.toString();
							if(jsq_text != null) {
								SecretTagPool reply = ic.Command_ClientRequest(rtb, jsq_text, additional_pool_to_send);
								if(reply != null) {
									boolean do_update_svc_pack = false;
									face_bytes = reply.Get(SecretTagPool.tagFace);
									cfg_bytes = reply.Get(SecretTagPool.tagConfig);
									if(SLib.GetLen(face_bytes) > 0) {
										StyloQFace other_face = new StyloQFace();
										if(other_face.FromJson(new String(face_bytes))) {
											// Необходимо модифицировать оригинальный face установкой
											// фактического времени истечения срока действия //
											String ep_text = other_face.Get(StyloQFace.tagExpiryPeriodSec, 0);
											if(SLib.GetLen(ep_text) > 0) {
												int ep = Integer.parseInt(ep_text);
												if(ep > 0) {
													long ees = EvaluateExpiryTime(ep);
													if(ees > 0)
														other_face.Set(StyloQFace.tagExpiryEpochSec, 0, Long.toString(ees));
												}
											}
											face_bytes = other_face.ToJson().getBytes(StandardCharsets.UTF_8);
											svc_pack.Pool.Put(SecretTagPool.tagFace, face_bytes);
											rtb.OtherFace = other_face;
											do_update_svc_pack = true;
										}
									}
									if(SLib.GetLen(cfg_bytes) > 0) {
										StyloQConfig cfg = new StyloQConfig();
										if(cfg.FromJson(new String(cfg_bytes))) {
											// Необходимо модифицировать оригинальную кофигурацию установкой
											// фактического времени истечения срока действия //
											String ep_text = cfg.Get(StyloQConfig.tagExpiryPeriodSec);
											if(SLib.GetLen(ep_text) > 0) {
												int ep = Integer.parseInt(ep_text);
												if(ep > 0) {
													final long ees = EvaluateExpiryTime(ep);
													if(ees > 0)
														cfg.Set(StyloQConfig.tagExpiryEpochSec, Long.toString(ees));
												}
											}
											cfg_bytes = cfg.ToJson().getBytes(StandardCharsets.UTF_8);
											svc_pack.Pool.Put(SecretTagPool.tagConfig, cfg_bytes); // !
											do_update_svc_pack = true;
										}
									}
									if(do_update_svc_pack) {
										Database.Transaction tra = new Database.Transaction(dbs,true);
										final long id = dbs.PutPeerEntry(rtb.InnerSvcID, svc_pack, false);
										if(my_face_tramsitted && transmitted_face_id > 0) {
											dbs.LogEvent(SLib.PPACN_STYLOQFACETRANSMITTED, SLib.PPOBJ_STYLOQBINDERY, id, transmitted_face_id, false);
										}
										if(id == rtb.InnerSvcID) {
											tra.Commit();
											result = 1;
										}
										else
											tra.Abort();
									}
									else if(my_face_tramsitted && transmitted_face_id > 0) {
										dbs.LogEvent(SLib.PPACN_STYLOQFACETRANSMITTED, SLib.PPOBJ_STYLOQBINDERY, rtb.InnerSvcID, transmitted_face_id, true);
									}
								}
							}
						} catch(JSONException exn) {
							result = 0;
							new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
						}
					}
					else if(jsobj_my_face_to_transmit != null) {
						// Автономная передача собственного лика сервису
						JSONObject js_query = new JSONObject();
						try {
							js_query.put("cmd", "setface");
							String jsq_text = js_query.toString();
							if(jsq_text != null) {
								String jstxt_my_face_to_transmit = jsobj_my_face_to_transmit.toString();
								if(SLib.GetLen(jstxt_my_face_to_transmit) > 0) {
									SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
									additional_pool_to_send = new SecretTagPool();
									additional_pool_to_send.Put(SecretTagPool.tagFace, jstxt_my_face_to_transmit.getBytes(), ds);
									my_face_tramsitted = true;
									//
									SecretTagPool reply = ic.Command_ClientRequest(rtb, jsq_text, additional_pool_to_send);
									if(reply != null) {
										if(my_face_tramsitted && transmitted_face_id > 0) {
											dbs.LogEvent(SLib.PPACN_STYLOQFACETRANSMITTED, SLib.PPOBJ_STYLOQBINDERY, rtb.InnerSvcID, transmitted_face_id, true);
										}
									}
								}
								//js_query.put("face", jsobj_my_face_to_transmit);
							}
						} catch(JSONException exn) {
							result = 0;
							new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
						}
					}
				}
			}
		}
		return result;
	}
	//
	// Descr: Процедура реализующая базовый механизм запроса к сервису.
	//
	private static StyloQApp.InterchangeResult Helper_DoInterchange2(StyloQApp appCtx, DoInterchangeParam param)
	{
		StyloQApp.InterchangeResult result = null; //new InterchangeResult();
		boolean debug_mark = false;
		StyloQInterchange.RoundTripBlock rtb = null;
		try {
			StyloQDatabase dbs = appCtx.GetDB();
			THROW(param != null, 0);
			THROW(SLib.GetLen(param.SvcIdent) > 0, 0);
			THROW(SLib.GetLen(param.AccsPoint) > 0, 0);
			THROW(dbs != null, 0); // @internal error
			//long own_peer_id = dbs.SetupPeerInstance();
			//if(own_peer_id > 0) {
			boolean do_req_cmd = false;
			//final int test_count = 100; // @debug
			//int   test_count_ok = 0; // @debug
			StyloQInterchange ic = new StyloQInterchange(appCtx);
			//const char * p_svc_ident_mime = "Lkekoviu1J2nw1O7/R66LYvpAtA="; // pft
			//ConnFactory.setUri("amqp://Admin:CX8U3kM9wTQb@213.166.70.221/styloq");
			//byte[] svc_ident = Base64.getDecoder().decode("Lkekoviu1J2nw1O7/R66LYvpAtA=");
			// String accs_point = "amqp://213.166.70.221";
			rtb = new StyloQInterchange.RoundTripBlock(param);
			ic.InitRoundTripBlock(dbs, rtb);
			/*{
				//
				// Тестовый участок для отработки соединения с сервером
				//
				for(int i = 0; i < test_count; i++) {
					JobServerProtocol.StyloQFrame tp = new JobServerProtocol.StyloQFrame();
					{
						tp.StartWriting(JobServerProtocol.PPSCMD_PING, JobServerProtocol.StyloQFrame.psubtypeForward);
						tp.P.Put(SecretTagPool.tagSvcIdent, param.SvcIdent);
						tp.FinishWriting(null);
					}
					if(ic.SendHttpQuery(rtb, tp, null) > 0) {
						test_count_ok++;
					}
				}
			}*/
			boolean is_current_sess_valid = false;
			if(rtb.InnerSessID > 0) {
				if(ic.Session_ClientRequest(rtb)) {
					QueryConfigAndSetFaceIfNeeded(appCtx, ic, rtb);
					do_req_cmd = true;
					is_current_sess_valid = true;
				}
				else
					result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent, "SessionRequest", null);
			}
			if(!is_current_sess_valid) {
				boolean do_registration = false;
				// Либо нет сохраненной сессии, либо при попытке соединится с ней что-то пошло не так
				if(rtb.InnerSvcID > 0) {
					if(ic.Verification_ClientRequest(dbs, rtb)) {
						QueryConfigAndSetFaceIfNeeded(appCtx, ic, rtb);
						do_req_cmd = true;
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent, "Verification", null);
					}
					else {
						int reply_err_code = appCtx.GetLastError();
						if(reply_err_code == ppstr2.PPERR_SQ_CLIRECORDFAULT || reply_err_code == ppstr2.PPERR_SQ_ENTRYIDENTNFOUND) {
							// Если мы получили от сервиса ошибку нарушения целостности записи
							// то попытаемся зарегистрироваться снова
							do_registration = true;
						}
						else
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent, "Verification", null);
					}
				}
				else
					do_registration = true;
				if(do_registration) {
					if(ic.KexClientRequest(dbs, rtb)) {
						long svc_id = ic.Registration_ClientRequest(dbs, rtb);
						if(svc_id > 0) {
							do_req_cmd = true;
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent,"RegistrationClientRequest", new Long(svc_id));
						}
						else
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent,"RegistrationClientRequest", null);
					}
					else
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent,"KexClientRequest", null);
				}
			}
			if(do_req_cmd && SLib.GetLen(param.CommandJson) > 0) {
				SecretTagPool rpool = ic.Command_ClientRequest(rtb, param.CommandJson, null);
				if(rpool != null) {
					StyloQDatabase.SecStoragePacket svc_pack = dbs.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, param.SvcIdent);
					long svc_id = (svc_pack != null) ? svc_pack.Rec.ID : 0;
					JSONObject jsobj = new JSONObject(param.CommandJson);
					//Object ro = (jsobj != null) ? jsobj.get("cmd") : null;
					String org_cmd_text = (jsobj != null) ? jsobj.optString("cmd", "") : "";
					if(org_cmd_text.equalsIgnoreCase("GetCommandList")) {
						// Специальный случай: получение списка команд сервиса - мы сразу сохраняем
						// этот список в реестре объектов StyloQ
						byte[] doc_ident = dbs.MakeDocumentStorageIdent(param.SvcIdent, null);
						long new_doc_id = dbs.PutDocument(-1, StyloQDatabase.SecStoragePacket.doctypCommandList, 0, doc_ident, svc_id, rpool);
						if(new_doc_id > 0)
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent, "UpdateCommandList", param.SvcIdent);
						else
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent,"UpdateCommandList", param.SvcIdent);
					}
					else if(org_cmd_text.equalsIgnoreCase("GetForeignConfig")) {
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent, "GetForeignConfig", null);
						result.SvcReply = rpool;
					}
					else if(org_cmd_text.equalsIgnoreCase("getblob")) {
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent, "getblob", null);
						result.SvcReply = rpool;
					}
					else {
						//Object subj = null;
						StyloQCommand.DocReference doc_ref = null;
						if(param.OriginalCmdItem != null) {
							byte[] raw_doc_data = rpool.Get(SecretTagPool.tagRawData);
							byte[] raw_doc_decl = rpool.Get(SecretTagPool.tagDocDeclaration);
							long new_doc_id = 0;
							if(SLib.GetLen(raw_doc_data) > 0) {
								if(org_cmd_text.equalsIgnoreCase("PostDocument")) {
									if(param.DocReqList != null && param.DocReqList.size() > 0) {
										String text_doc_data = new String(raw_doc_data);
										if(SLib.GetLen(text_doc_data) > 0) {
											JSONObject js_doc_data = new JSONObject(text_doc_data);
											if(js_doc_data != null) {
												String rep_txt_uuid = js_doc_data.optString("document-uuid", null);
												long rep_id = js_doc_data.optLong("document-id", 0);
												if(SLib.GetLen(rep_txt_uuid) > 0 && rep_id > 0) {
													UUID remote_doc_uuid = UUID.fromString(rep_txt_uuid);
													if(remote_doc_uuid != null) {
														for(int drlidx = 0; drlidx < param.DocReqList.size(); drlidx++) {
															DocumentRequestEntry dre = param.DocReqList.get(drlidx);
															if(dre != null && dre.DocUUID.compareTo(remote_doc_uuid) == 0)
																dre.RemoteDocID = rep_id;
														}
														dbs.AcceptDocumentRequestList(param.DocReqList);
													}
												}
											}
										}
									}
								}
								else if(SLib.GetLen(raw_doc_decl) > 0) {
									if(param.OriginalCmdItem.BaseCmdId == StyloQCommand.sqbcRsrvOrderPrereq) {
										byte[] doc_ident = dbs.MakeDocumentStorageIdent(param.SvcIdent, param.OriginalCmdItem.Uuid);
										new_doc_id = dbs.PutDocument(-1, StyloQDatabase.SecStoragePacket.doctypOrderPrereq, 0, doc_ident, svc_id, rpool);
									}
									else if(param.OriginalCmdItem.BaseCmdId == StyloQCommand.sqbcReport) {
										byte[] doc_ident = dbs.MakeDocumentStorageIdent(param.SvcIdent, param.OriginalCmdItem.Uuid);
										if(doc_ident != null)
											new_doc_id = dbs.PutDocument(-1, StyloQDatabase.SecStoragePacket.doctypReport, 0, doc_ident, svc_id, rpool);
									}
								}
								else {

								}
							}
							if(new_doc_id > 0) {
								doc_ref = new StyloQCommand.DocReference();
								doc_ref.ID = new_doc_id;
								doc_ref.Decl = new StyloQCommand.DocDeclaration();
								doc_ref.Decl.FromJson(new String(raw_doc_decl));
								result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param, doc_ref);
							}
							else
								result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param, rpool);
						}
						else if(param.RetrActivity_ != null)
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param, /*doc_ref*/rpool);
						else
							result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, param.SvcIdent, "Command", /*doc_ref*/rpool);
					}
				}
				else {
					if(param.OriginalCmdItem != null || param.RetrActivity_ != null)
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param, /*reply_pool*/null);
					else
						result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, param.SvcIdent, "Command", /*reply_pool*/null);
				}
			}
			//}
		} catch(StyloQException exn) {
			if(param.RetrActivity_ != null)
				result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.EXCEPTION, param, exn.GetMessage(appCtx));
			else
				result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.EXCEPTION, param, exn.GetMessage(appCtx));
		} catch(JSONException exn) {
			StyloQException stq_exn = new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
			String exn_msg = stq_exn.GetMessage(appCtx);
			result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.EXCEPTION, param, exn_msg);
		} finally {
			if(rtb != null) {
				rtb.Close();
				rtb = null;
			}
		}
		return result;
	}
	//
	// Descr: Запрашивает у произвольно выбранного медиатора blob с сигнатурой blobSignature.
	//
	public static byte [] QueryBlob(StyloQApp appCtx, String blobSignature)
	{
		byte [] result = null;
		if(appCtx != null && SLib.GetLen(blobSignature) > 0) {
			ArrayList<StyloQApp.IgnitionServerEntry> isl = appCtx.GetMediatorList();
			if(isl != null && isl.size() > 0) {
				Collections.shuffle(isl);
				for(int i = 0; result == null && i < isl.size(); i++) {
					StyloQApp.IgnitionServerEntry ise = isl.get(0);
					if(ise != null) {
						DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(ise.SvcIdent);
						inner_param.AccsPoint = ise.Url;
						try {
							JSONObject js_query = new JSONObject();
							js_query.put("cmd", "getblob");
							js_query.put("signature", blobSignature);
							inner_param.CommandJson = js_query.toString();
							StyloQApp.InterchangeResult inner_result = Helper_DoInterchange2(appCtx, inner_param);
							if(inner_result != null && inner_result.ResultTag == StyloQApp.SvcQueryResult.SUCCESS && inner_result.SvcReply != null) {
								byte [] reply_raw_data = inner_result.SvcReply.Get(SecretTagPool.tagRawData);
								if(SLib.GetLen(reply_raw_data) > 0) {
									String json_text = new String(reply_raw_data);
									if(SLib.GetLen(json_text) > 0) {
										JSONObject js_reply = new JSONObject(json_text);
										result = inner_result.SvcReply.Get(SecretTagPool.tagBlob);
										//String content_base64 = js_reply.optString("content", null);
										//if(SLib.GetLen(content_base64) > 0) {
										//	result = Base64.getDecoder().decode(content_base64);
										//}
									}
								}
							}
						} catch(JSONException exn) {
							;
						}

					}
				}
			}
		}
		return result;
	}
	private static StyloQConfig QuerySvcConfig(StyloQApp appCtx, byte [] svcIdent)
	{
		StyloQConfig result = null;
		try {
			ArrayList<StyloQApp.IgnitionServerEntry> isl = appCtx.GetIgnitionServerList();
			if(SLib.GetLen(svcIdent) > 0 && isl != null && isl.size() > 0) {
				String svc_ident_hex = Base64.getEncoder().encodeToString(svcIdent);
				for(int i = 0; result == null && i < isl.size(); i++) {
					StyloQApp.IgnitionServerEntry ise = isl.get(i);
					DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(ise.SvcIdent);
					inner_param.AccsPoint = ise.Url;
					{
						JSONObject js_query = new JSONObject();
						js_query.put("cmd", "GetForeignConfig");
						js_query.put("foreignsvcident", svc_ident_hex);
						inner_param.CommandJson = js_query.toString();
					}
					StyloQApp.InterchangeResult inner_result = Helper_DoInterchange2(appCtx, inner_param);
					if(inner_result != null && inner_result.ResultTag == StyloQApp.SvcQueryResult.SUCCESS && inner_result.SvcReply != null) {
						byte[] reply_raw_data = inner_result.SvcReply.Get(SecretTagPool.tagRawData);
						if(SLib.GetLen(reply_raw_data) > 0) {
							String json_text = new String(reply_raw_data);
							if(SLib.GetLen(json_text) > 0) {
								JSONObject js_reply = new JSONObject(json_text);
								Object cfg_js_obj = js_reply.opt("config");
								if(cfg_js_obj != null && cfg_js_obj instanceof JSONObject) {
									result = new StyloQConfig();
									if(!result.FromJsonObj((JSONObject)cfg_js_obj))
										result = null;
								}
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	//
	// Descr: Процедура осуществляющая полный цикл обращения к сервису, в том числе,
	//   если точка доступа сервиса не определена в param, то функция пытается
	//   получить конфигурацию сервиса от медиатора.
	// Attention: Процедура должна вызываться только в отдельном потоке
	//
	private static void Helper_DoInterchange(StyloQApp appCtx, DoInterchangeParam param)
	{
		boolean debug_mark = false;
		boolean treat_result_on_the_spot = false;
		StyloQInterchange.RoundTripBlock rtb = null;
		try {
			THROW(param != null, 0);
			THROW(SLib.GetLen(param.SvcIdent) > 0, 0);
			if(param.OriginalCmdItem != null) {
				if(param.OriginalCmdItem.Uuid == null && param.OriginalCmdItem.Name.equalsIgnoreCase("requestdocumentstatuslist")) {
					//
					// Это - вариант неинтерактивной команды, результат которой должен быть обработан
					// непосредственно здесь (в этой функции) без передачи в основной поток приложения.
					//
					treat_result_on_the_spot = true;
				}
			}
			StyloQDatabase dbs = appCtx.GetDB();
			//THROW(SLib.GetLen(param.AccsPoint) > 0, 0);
			THROW(dbs != null, 0); // @internal error
			String svc_ident_hex = Base64.getEncoder().encodeToString(param.SvcIdent); // @debug
			long own_peer_id = dbs.SetupPeerInstance();
			if(own_peer_id > 0) {
				//
				// Если в параметрах не задан адрес сервера (param.AccsPoint),
				// то действуем по следующему плану:
				// 1. Если у нас уже есть запись сервиса, то извлекаем из нее точку доступа.
				// 2. Если на предыдущем шаге нас ждало разочарование, то обращаемся к медиаторам
				//    за информацией о конфигурации сервиса.
				//
				if(SLib.GetLen(param.AccsPoint) <= 0) {
					StyloQConfig svc_cfg = QuerySvcConfig(appCtx, param.SvcIdent);
					if(svc_cfg != null) {
						param.AccsPoint = svc_cfg.Get(StyloQConfig.tagUrl);
						param.MqbAuth = svc_cfg.Get(StyloQConfig.tagMqbAuth);
						param.MqbSecret = svc_cfg.Get(StyloQConfig.tagMqbSecret);
					}
				}
				if(SLib.GetLen(param.AccsPoint) <= 0) {
					StyloQDatabase.SecStoragePacket svc_pack = dbs.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, param.SvcIdent);
					if(svc_pack != null) {
						byte [] raw_svc_cfg = svc_pack.Pool.Get(SecretTagPool.tagConfig);
						if(SLib.GetLen(raw_svc_cfg) > 0) {
							StyloQConfig svc_cfg = new StyloQConfig();
							String js_cfg = new String(raw_svc_cfg);
							if(svc_cfg.FromJson(js_cfg)) {
								param.AccsPoint = svc_cfg.Get(StyloQConfig.tagUrl);
								if(SLib.GetLen(param.AccsPoint) > 0) {
									param.MqbAuth = svc_cfg.Get(StyloQConfig.tagMqbAuth);
									param.MqbSecret = svc_cfg.Get(StyloQConfig.tagMqbSecret);
								}
							}
						}
					}
				}
				if(SLib.GetLen(param.AccsPoint) > 0) {
					StyloQApp.InterchangeResult inner_result = Helper_DoInterchange2(appCtx, param);
					// Правильно будет трактовать inner_result == null как внутреннюю ошибку.
					if(inner_result != null) {
						if(treat_result_on_the_spot) {
							if(param.OriginalCmdItem.Uuid == null && param.OriginalCmdItem.Name.equalsIgnoreCase("requestdocumentstatuslist")) {
								if(inner_result.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
									if(inner_result.InfoReply != null && inner_result.InfoReply instanceof SecretTagPool) {
										SecretTagPool rp = (SecretTagPool)inner_result.InfoReply;
										byte [] rawdata = rp.Get(SecretTagPool.tagRawData);
										String text_reply = (SLib.GetLen(rawdata) > 0) ? new String(rawdata) : null;
										if(SLib.GetLen(text_reply) > 0) {
											AcceptDocStatusPollResult(appCtx, new JSONObject(text_reply));
										}
									}
								}
							}
						}
						else {
							if(inner_result.ResultTag != StyloQApp.SvcQueryResult.UNDEF && inner_result.InfoReply == null)
								inner_result.InfoReply = inner_result.SvcReply;
							//StyloQApp.SvcReplySubject srsub = new StyloQApp.SvcReplySubject(param.SvcIdent,
							// inner_result.TextSubj, inner_result.OriginalCmdItem, inner_result.RetrActivity, inner_result.GetErrMsg());
							appCtx.SendSvcReplyToMainThread(/*srsub*/inner_result, inner_result.InfoReply);
						}
					}
				}
				else {
					throw new StyloQException(ppstr2.PPERR_SQ_SVCACCSPTREQFAULT, "");
				}
			}
		} catch(StyloQException exn) {
			if(treat_result_on_the_spot) {
				;
			}
			else {
				StyloQApp.InterchangeResult inner_result = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.EXCEPTION, param, exn.GetMessage(appCtx));
				inner_result.RetrActivity = param.RetrActivity_;
				//StyloQApp.SvcReplySubject srsub = new StyloQApp.SvcReplySubject(param.SvcIdent, null,
				//	param.OriginalCmdItem, param.RetrActivity_, exn.GetMessage(appCtx));
				appCtx.SendSvcReplyToMainThread(/*StyloQApp.SvcQueryResult.EXCEPTION,*/inner_result, null);
			}
		} catch(JSONException exn) {
			;
		}
		finally {
			if(rtb != null) {
				rtb.Close();
				rtb = null;
			}
		}
	}
	public static void RunClientInterchange(StyloQApp appCtx, DoInterchangeParam param)
	{
		class ThreadEngine_ClientInterchange implements Runnable {
			ThreadEngine_ClientInterchange()
			{
			}
			@Override public void run()
			{
				Helper_DoInterchange(appCtx, param);
			}
		}
		boolean ok = true;
		Thread thr = new Thread(new ThreadEngine_ClientInterchange());
		thr.start();
	}
	public static boolean DoSvcRequest(StyloQApp appCtx, RequestBlock blk)
	{
		boolean ok = false;
		if(blk != null && blk.SvcPack != null) {
			String acspt_url = null;
			String acspt_mqbauth = null;
			String acspt_mqbsecr = null;
			StyloQConfig cfg = null;
			byte[] svc_ident = blk.SvcPack.Pool.Get(SecretTagPool.tagSvcIdent);
			byte[] svc_cfg_bytes = blk.SvcPack.Pool.Get(SecretTagPool.tagConfig);
			if(SLib.GetLen(svc_cfg_bytes) > 0) {
				cfg = new StyloQConfig();
				String js_cfg = new String(svc_cfg_bytes);
				if(cfg.FromJson(js_cfg)) {
					acspt_url = cfg.Get(StyloQConfig.tagUrl);
					acspt_mqbauth = cfg.Get(StyloQConfig.tagMqbAuth);
					acspt_mqbsecr = cfg.Get(StyloQConfig.tagMqbSecret);
				}
			}
			if(SLib.GetLen(acspt_url) > 0) {
				if(acspt_url.indexOf("://") < 0) {
					if(SLib.GetLen(acspt_mqbauth) > 0)
						acspt_url = "amqp://" + acspt_url;
					else
						acspt_url = "http://" + acspt_url;
				}
				try {
					URI uri = new URI(acspt_url);
					int uriprot = SLib.GetUriSchemeId(uri.getScheme());
					if(uriprot == SLib.uripprotUnkn) {
						if(SLib.GetLen(acspt_mqbauth) > 0) {
							acspt_url = "amqp://" + acspt_url;
							uri = new URI(acspt_url);
							uriprot = SLib.GetUriSchemeId(uri.getScheme());
						}
						else {
							acspt_url = "http://" + acspt_url;
							uri = new URI(acspt_url);
							uriprot = SLib.GetUriSchemeId(uri.getScheme());
						}
					}
					if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps || uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) {
						DoInterchangeParam param = new DoInterchangeParam(svc_ident);
						param.AccsPoint = acspt_url;
						param.MqbAuth = acspt_mqbauth;
						param.MqbSecret = acspt_mqbsecr;
						param.OriginalCmdItem = blk.OrgCmdItem;
						if(blk.JsCmd != null)
							param.CommandJson = blk.JsCmd.toString();
						param.RetrActivity_ = blk.RetrActivity; // @v11.3.10
						param.DocReqList = blk.DocReqList; // @v11.3.12
						RunClientInterchange(appCtx, param);
						ok = true;
					}
				} catch(URISyntaxException exn) {
					ok = false;
					new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
				}
			}
		}
		return ok;
	}
	public void TestAmq() throws StyloQException
	{
		try {
			//ConnectionFactory factory = new ConnectionFactory();
			ConnFactory.setAutomaticRecoveryEnabled(false);
			//ConnFactory.setUsername("Admin");
			//ConnFactory.setPassword("CX8U3kM9wTQb");
			//ConnFactory.setVirtualHost("styloq");
			//ConnFactory.setHost("213.166.70.221");
			ConnFactory.setUri("amqp://Admin:CX8U3kM9wTQb@213.166.70.221/styloq");
			Thread thr = new Thread(new Runnable() {
				@Override public void run()
				{
					try {
						Connection conn = ConnFactory.newConnection();
						Channel channel = conn.createChannel();
						//
						// exchange: styloqrpc
						String test_message = "Hello, i'm Stylo-Q mobile client!";
						String exchange_name = "styloqrpc";
						String routing_key = ""; // must be
						String queue_name = "StyloQ-mobile-test"; // must be
						//channel.exchangeDeclare(exchange_name, "direct", true);
						//String queueName = channel.queueDeclare().getQueue();
						channel.exchangeDeclare(exchange_name, BuiltinExchangeType.DIRECT);
						channel.queueDeclare(queue_name, false, false, false, null);
						channel.queueBind(queue_name, exchange_name, routing_key);
						channel.basicPublish(exchange_name, routing_key, null, test_message.getBytes());
						//
						channel.close();
						conn.close();
					} catch(Exception exn) {
						Log.d("", exn.toString());
					}
				}
			});
			thr.start();
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		} catch(KeyManagementException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_CRYPTOKEYMANAGEMENT, exn.getMessage());
		} catch(URISyntaxException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
		}
		//ConnFactory.setPort(portNumber);
		//Connection conn = ConnFactory.newConnection();
		//Channel channel = conn.createChannel();
		//
		// exchange: styloqrpc
		//String test_message = "Hello, i'm Stylo-Q mobile client!";
		//String exchange_name = "styloqrpc";
		//String routing_key = ""; // must be
		//String queue_name = "StyloQ-mobile-test"; // must be
		//channel.exchangeDeclare(exchange_name, "direct", true);
		//String queueName = channel.queueDeclare().getQueue();
		//channel.queueDeclare(queue_name, false, false, false, null);
		//channel.queueBind(queue_name, exchange_name, routing_key);
		//channel.basicPublish(exchange_name, routing_key, null, test_message.getBytes());
		//
		//channel.close();
		//conn.close();
	}
}
