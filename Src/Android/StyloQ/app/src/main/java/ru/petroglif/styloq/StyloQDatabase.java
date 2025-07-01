// StyloQDatabase.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import static java.lang.Math.abs;
import static ru.petroglif.styloq.SLib.PPOBJ_STYLOQBINDERY;
import static ru.petroglif.styloq.SLib.THROW;
import static ru.petroglif.styloq.SLib.THROW_SL;

import android.content.Context;
import android.database.SQLException;
import android.os.Build;

import androidx.annotation.RequiresApi;

import org.checkerframework.checker.nullness.qual.NonNull;
import org.json.JSONException;
import org.json.JSONObject;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Comparator;
import java.util.UUID;
import java.util.Vector;

//import static ru.petroglif.styloq.StyloQDatabase.SecStoragePacket.kSession;

public class StyloQDatabase extends Database {
	StyloQDatabase(StyloQApp ctx) throws StyloQException
	{
		super(ctx, "StyloQDatabase", 1);
	}
	static class SecStoragePacket {
		public static final int kUndef = 0;
		public static final int kNativeService = 1; // Собственная идентификация. Используется для любого узла, включая клиентские, которые никогда не будут сервисами //
		public static final int kForeignService = 2;
		public static final int kClient = 3;
		public static final int kSession = 4;
		public static final int kFace = 5; // Параметры лика, которые могут быть переданы серверу для ассоциации с нашим клиентским аккаунтом
		public static final int kDocIncoming = 6; // Входящие документы
		public static final int kDocOutcoming = 7; // Исходящие документы
		public static final int kCounter = 8; // @v11.2.10 Специальная единственная запись для хранения текущего счетчика (документов и т.д.)
		public static final int kNotification_before90v = 9; // @v11.5.9  Документ извещения. Главным образом, предполагаются извещения от сервисов к клиентам. Но, вероятно,
			// будут возможны и извещения в обратном направлении (клиент о чем-то информирует сервис).
		public static final int kCurrentState = 10; // @v11.7.0 Документ текущего состояния клиента.
		//
		// Descr: Флаги записи таблицы данных StyloQ bindery (StyloQSec::Flags)
		//
		public static final int styloqfMediator = 0x0001; // Запись соответствует kForeignService-медиатору. Флаг устанавливается/снимается при создании или обновлении
		// записи после получения соответствующей информации от сервиса-медиатора
		//public static final int styloqfDocFinished        = 0x0002; // @v11.3.12 Для документа: цикл обработки для документа завершен. Не может содержать флаги (styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv|styloqfDocDraft)
		//public static final int styloqfDocWaitForOrdrsp   = 0x0004; // @v11.3.12 Для документа заказа: ожидает подтверждения заказа. Не может содержать флаги (styloqfDocFinished|styloqfDocDraft)
		//public static final int styloqfDocWaitForDesadv   = 0x0008; // @v11.3.12 Для документа заказа: ожидает документа отгрузки. Не может содержать флаги (styloqfDocFinished|styloqfDocDraft)
		//public static final int styloqfDocDraft           = 0x0010; // @v11.3.12 Для документа: драфт-версия. Не может содержать флаги (styloqfDocFinished|styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv)
		//public static final int styloqfDocTransmission    = 0x0020; // @v11.4.0  Для документа: технический флаг, устанавливаемый перед отправкой документа контрагенту и снимаемый после того, как
		// контрагент подтвердил получение. Необходим для управления документами, передача которых не завершилась.
		//public static final int styloqfDocCancelledByCli  = 0x0040; // @v11.4.0 Для документа: документ отменен клиентом
		//public static final int styloqfDocCancelledBySvc  = 0x0080; // @v11.4.0 Для документа: документ отменен сервисом
		//public static final int styloqfDocStatusFlagsMask = (styloqfDocFinished|styloqfDocWaitForOrdrsp|styloqfDocWaitForDesadv|styloqfDocDraft|styloqfDocTransmission);
		public static final int styloqfDocStatusFlags = (0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40); // 6 bits используются для кодирования статуса документа
		public static final int styloqfDocTransmission = 0x0080; // @v11.4.0  Для документа: технический флаг, устанавливаемый перед отправкой документа контрагенту и снимаемый после того, как
		// контрагент подтвердил получение. Необходим для управления документами, передача которых не завершилась.
		public static final int styloqfPassive = 0x0100; // @v11.4.6 Флаг для kForeignService. Означает, что сервис пассивен (относительно клиента) и не должен отображаться в регулярном списке у клиента.
		public static final int styloqfUnprocessedDoc_ = 0x0200; // @v11.5.0
		public static final int styloqfAutoObjMatching = 0x0400; // @v11.5.6 Объект (обычно, персоналия), соответствующий клиентской записи, был создан автоматически.
		// Флаг необходим для дифференцированного изменения записи объекта в зависимости от того, пришел он изначально от клиента или же существует
		// в базе данных серсиса самостоятельно (матчинг с клиентом был осуществлен вручную).
		public static final int styloqfProcessed = 0x0800; // @v11.5.10 Запись обработана. Флаг изначально введен для пометки записей типа kNotification как прочитанных.
		// В дальнейшем будет, вероятно, применяться и для других типов записей.
		//
		//
		// Статусы документов клиент-->сервис
		//
		public static final int styloqdocstUNDEF = 0; // неопределенный статус. Фактически, недопустимое состояние.
		public static final int styloqdocstDRAFT = 1; // драфт. Находится на стадии формирования на стороне эмитента
		public static final int styloqdocstWAITFORAPPROREXEC = 2; // ждет одобрения или исполнения от акцептора
		public static final int styloqdocstAPPROVED = 3; // одобрен акцептором
		public static final int styloqdocstCORRECTED = 4; // скорректирован акцептором (от акцептора поступает корректирующий документ, который привязывается к оригиналу)
		public static final int styloqdocstCORRECTIONACCEPTED = 5; // корректировка акцептора принята эмитентом
		public static final int styloqdocstCORRECTIONREJECTED = 6; // корректировка акцептора отклонена эмитентом (документ полностью отменяется и цикл документа завершается)
		public static final int styloqdocstREJECTED = 7; // отклонен акцептором (цикл документа завершается)
		public static final int styloqdocstMODIFIED = 8; // изменен эмитентом (от эмитента поступает измененная версия документа, которая привязывается к оригиналу)
		public static final int styloqdocstCANCELLED = 9; // отменен эмитентом (цикл документа завершается). Переход в это состояние возможен с ограничениями.
		public static final int styloqdocstEXECUTED = 10; // исполнен акцептором
		public static final int styloqdocstEXECUTIONACCEPTED = 11; // подтверждение от эмитента исполнения документа акцептором (цикл документа завершается)
		public static final int styloqdocstEXECUTIONCORRECTED = 12; // корректировка от эмитента исполнения документа акцептором (от эмитента поступает документ согласования)
		public static final int styloqdocstEXECORRECTIONACCEPTED = 13; // согласие акцептора с документом согласования эмитента
		public static final int styloqdocstEXECORRECTIONREJECTED = 14; // отказ акцептора от документа согласования эмитента - тупиковая ситуация, которая должна быть
		// разрешена посредством дополнительных механизмов (escrow счета, полный возврат с отменой платежей и т.д.)
		public static final int styloqdocstFINISHED_SUCC = 15; // Финальное состояние документа: завершен как учтенный и отработанный.
		public static final int styloqdocstFINISHED_FAIL = 16; // Финальное состояние документа: завершен как отмененный.
		public static final int styloqdocstCANCELLEDDRAFT = 17; // @v11.4.1 Драфт отмененый эмитентом. Переход в это состояние возможен только после styloqdocstDRAFT || styloqdocstUNDEF.
		public static final int styloqdocstINCOMINGMOD = 18; // @v11.4.9 Входящий (по отношению к клиенту) документ, над которым осуществлена частичная модификация
		public static final int styloqdocstINCOMINGMODACCEPTED = 19; // @v11.4.9 Входящий (по отношению к клиенту) документ, над которым осуществлена частичная модификация, которая, в свою очередь, акцептирована сервисом.
		public static final int styloqdocstPARTIALLYEXECUTED   = 20; // @v11.6.8 частично исполнен акцептором
		//
		public static final int doctypUndef           = 0;
		public static final int doctypCommandList     = 1;
		public static final int doctypOrderPrereq     = 2; // Предопределенный формат данных, подготовленных для формирования заказа на клиентской стороне
		public static final int doctypReport          = 3; // @v11.2.10 Отчеты в формате DL600 export
		public static final int doctypGeneric         = 4; // @v11.2.11 Общий тип для документов, чьи характеристики определяются видом операции (что-то вроде Bill в Papyrus'е)
		public static final int doctypIndexingContent = 5; // @v11.3.4 Документ, содержащий данные для индексации медиатором
		public static final int doctypIndoorSvcPrereq = 6; // @v11.4.5 Предопределенный формат данных, подготовленных для формирования данных для обслуживания внутри помещения сервиса (INDOOR)
		public static final int doctypIncomingList    = 7; // @v11.4.8
		public static final int doctypDebtList        = 8; // @v11.5.4 Реестр долговых документов по контрагентам. Специфичный документ: на клиентской стороне хранится единый реестр по всем
			// контрагентам. При этом запрос сервису отправляется по одному контрагенту, а ответ (корректный) встраивается в общий реестр.
		public static final int doctypCurrentState    = 9; // @v11.7.0 Внутренний документ, сохраняющий состояние и, возможно, какие-то конфигурационные параметры.
			// Применим только для клиентской базы данных.
		//
		SecTable.Rec Rec;
		SecretTagPool Pool;

		SecStoragePacket(int recKind) throws StyloQException
		{
			Rec = new SecTable.Rec();
			Rec.Kind = recKind;
			Pool = new SecretTagPool();
		}
		SecStoragePacket(SecTable.Rec rec) throws StyloQException
		{
			Rec = rec;
			Pool = new SecretTagPool();
			Pool.Unserialize(Rec.VT);
		}
		static boolean IsDocKind(int kind)
		{
			return (kind == kDocIncoming || kind == kDocOutcoming || kind == kCurrentState); // @v11.7.0 (|| kind == kCurrentState)
		}
		boolean IsValid()
		{
			boolean ok = true;
			try {
				THROW_SL(Rec.Kind == kNativeService || Rec.Kind == kForeignService || IsDocKind(Rec.Kind) ||
						Rec.Kind == kClient || Rec.Kind == kSession || Rec.Kind == kFace || Rec.Kind == kCounter, 0);
				THROW_SL((Rec.Kind == kNativeService || Rec.Kind == kForeignService) || (Rec.Flags & styloqfMediator) == 0, 0);
				THROW_SL(IsDocKind(Rec.Kind) || ((Rec.Flags >> 1) & styloqfDocStatusFlags) == 0, 0);
				if(IsDocKind(Rec.Kind)) {
					//THROW_SL((Rec.Flags & styloqfDocFinished) == 0 || (Rec.Flags & (styloqfDocWaitForOrdrsp | styloqfDocWaitForDesadv | styloqfDocDraft)) == 0, 0);
					//THROW_SL((Rec.Flags & styloqfDocWaitForOrdrsp) == 0 || (Rec.Flags & (styloqfDocFinished | styloqfDocDraft)) == 0, 0);
					//THROW_SL((Rec.Flags & styloqfDocWaitForDesadv) == 0 || (Rec.Flags & (styloqfDocFinished | styloqfDocDraft)) == 0, 0);
					//THROW_SL((Rec.Flags & styloqfDocDraft) == 0 || (Rec.Flags & (styloqfDocFinished | styloqfDocWaitForOrdrsp | styloqfDocWaitForDesadv)) == 0, 0);
				}
			} catch(StyloQException exn) {
				ok = false;
			}
			return ok;
		}
		private boolean PreprocessBeforeStoring(final SecStoragePacket exPack) throws StyloQException
		{
			boolean ok = false;
			Rec.VT = Pool.Serialize();
			if(Rec.VT != null) {
				if(Rec.Kind == kFace) {
					if(SLib.GetLen(Rec.BI) == 0 || SLib.IsBytesZero(Rec.BI)) {
						if(exPack != null && SLib.GetLen(exPack.Rec.BI) > 0 && !SLib.IsBytesZero(exPack.Rec.BI))
							Rec.BI = exPack.Rec.BI;
						else {
							UUID u = UUID.randomUUID(); // randomUUID
							ByteBuffer tbb = ByteBuffer.allocate(16);
							tbb.putLong(u.getMostSignificantBits());
							tbb.putLong(u.getLeastSignificantBits());
							Rec.BI = tbb.array();
						}
					}
				}
				ok = true;
			}
			return ok;
		}
		public StyloQFace GetFace()
		{
			StyloQFace result = null;
			byte[] svc_face = Pool.Get(SecretTagPool.tagFace);
			if(svc_face != null) {
				result = new StyloQFace();
				if(!result.FromJson(new String(svc_face)))
					result = null;
			}
			return result;
		}
		public String GetSvcName(StyloQFace outerFaceInstance)
		{
			String result = "";
			StyloQFace face = (outerFaceInstance != null) ? outerFaceInstance : GetFace();
			if(face != null) {
				result = face.GetSimpleText(0);
			}
			if(SLib.GetLen(result) <= 0) {
				result = Base64.getEncoder().encodeToString(Rec.BI);
			}
			return result;
		}
		public StyloQCommand.List GetCommandList()
		{
			StyloQCommand.List result = null;
			if(Rec.DocType == doctypCommandList) {
				byte[] rawdata = Pool.Get(SecretTagPool.tagRawData);
				if(SLib.GetLen(rawdata) > 0) {
					String json_tex = new String(rawdata);
					if(SLib.GetLen(json_tex) > 0)
						result = StyloQCommand.FromJson(json_tex);
				}
			}
			return result;
		}
	}
	static class NotificationStoragePacket {
		NotificationStoragePacket() throws StyloQException
		{
			Rec = new NotificationTable2.Rec();
			Pool = new SecretTagPool();
		}
		NotificationStoragePacket(NotificationTable2.Rec rec) throws StyloQException
		{
			Rec = rec;
			Pool = new SecretTagPool();
			Pool.Unserialize(Rec.VT);
		}
		private boolean PreprocessBeforeStoring() throws StyloQException
		{
			boolean ok = false;
			Rec.VT = Pool.Serialize();
			if(Rec.VT != null) {
				ok = true;
			}
			return ok;
		}
		NotificationTable2.Rec Rec;
		SecretTagPool Pool;
	}

	@RequiresApi(api = Build.VERSION_CODES.O)
	long SetupPeerInstance() throws StyloQException
	{
		long id = 0;
		boolean test_result = false;
		SecStoragePacket pack = GetOwnPeerEntry();
		if(pack == null) {
			//
			// Сгенерировать:
			//   -- собственное большое случайное число (SSecretTagPool::tagPrimaryRN)
			//   -- GUID для дополнения SRN при генерации публичного идентификатора по идентификатору сервиса (SSecretTagPool::tagAG)
			//   -- Автономный фейковый идентификатор сервиса, для генерации собственного публичного идентификатора, не привязанного к сервису (SSecretTagPool::tagFPI)
			//
			final int primary_rn_bits_width = 1024;
			pack = new SecStoragePacket(SecStoragePacket.kNativeService);
			BigInteger rn = SLib.GenerateRandomBigNumber(primary_rn_bits_width);
			pack.Pool.Put(SecretTagPool.tagPrimaryRN, rn.toByteArray());
			rn = SLib.GenerateRandomBigNumber(160);
			final byte[] fpi = rn.toByteArray();
			pack.Pool.Put(SecretTagPool.tagFPI, fpi);
			{
				UUID u = UUID.randomUUID(); // randomUUID
				ByteBuffer tbb = ByteBuffer.allocate(16);
				tbb.putLong(u.getMostSignificantBits());
				tbb.putLong(u.getLeastSignificantBits());
				pack.Pool.Put(SecretTagPool.tagAG, tbb.array());
				//
				StyloQInterchange.GeneratePublicIdent(pack.Pool, fpi, SecretTagPool.tagSvcIdent, 0, pack.Pool);
				byte[] pi = pack.Pool.Get(SecretTagPool.tagSvcIdent);
				assert (pi != null && pi.length == 20);
				if(pi != null && pi.length == 20) {
					pack.Rec.BI = pi;
					id = PutPeerEntry(0, pack, true);
				}
			}
		}
		else {
			id = pack.Rec.ID;
		}
		if(id > 0) {
			//
			// Тестирование результатов
			//
			int test_tag_list[] = {SecretTagPool.tagPrimaryRN, SecretTagPool.tagSvcIdent, SecretTagPool.tagAG, SecretTagPool.tagFPI};
			{
				SecStoragePacket test_pack = GetPeerEntry(id);
				if(test_pack != null) {
					test_result = true;
					for(int i = 0; i < test_tag_list.length; i++) {
						int _tag = test_tag_list[i];
						byte[] c1 = pack.Pool.Get(_tag);
						byte[] c2 = test_pack.Pool.Get(_tag);
						if(c1 != null && c2 != null) {
							if(SLib.AreByteArraysEqual(c1, c2)) {
								if(_tag == SecretTagPool.tagSvcIdent) {
									if(SLib.AreByteArraysEqual(c1, test_pack.Rec.BI))
										;
									else
										test_result = false;
								}
							}
							else
								test_result = false;
						}
						else
							test_result = false;
					}
				}
			}
			if(test_result) {
				SecStoragePacket test_pack = GetOwnPeerEntry();
				if(test_pack != null) {
					test_result = true;
					for(int i = 0; i < test_tag_list.length; i++) {
						int _tag = test_tag_list[i];
						byte[] c1 = pack.Pool.Get(_tag);
						byte[] c2 = test_pack.Pool.Get(_tag);
						if(c1 != null && c2 != null) {
							if(SLib.AreByteArraysEqual(c1, c2)) {
								;
							}
							else
								test_result = false;
						}
						else
							test_result = false;
					}
				}
			}
			if(test_result) {
				//
				// Здесь проверяем инвариантность формирования клиентского идентификатора и секрета по одному и тому же
				// идентификатору сервиса.
				//
				final String svc_ident_mine = "Wn7M3JuxUaDpiCHlWiIStn+YYkQ="; // pft
				SecretTagPool last_test_pool = null;
				byte[] svc_ident_test = Base64.getDecoder().decode(svc_ident_mine);
				for(int gi = 0; gi < 10; gi++) {
					SecretTagPool test_pool = new SecretTagPool();
					StyloQInterchange.GeneratePublicIdent(pack.Pool, svc_ident_test, SecretTagPool.tagClientIdent, StyloQInterchange.gcisfMakeSecret, test_pool);
					if(gi > 0) {
						byte[] bch_test = test_pool.Get(SecretTagPool.tagClientIdent);
						byte[] last_bch_test = last_test_pool.Get(SecretTagPool.tagClientIdent);
						if(bch_test.length > 0 && last_bch_test.length > 0 && SLib.AreByteArraysEqual(bch_test, last_bch_test)) {
							bch_test = test_pool.Get(SecretTagPool.tagSecret);
							last_bch_test = last_test_pool.Get(SecretTagPool.tagSecret);
							if(bch_test.length > 0 && last_bch_test.length > 0 && SLib.AreByteArraysEqual(bch_test, last_bch_test)) {
								;
							}
							else
								test_result = false;
						}
						else
							test_result = false;
					}
					last_test_pool = test_pool;
				}
			}
		}
		return id;
	}
	SysJournalTable.Rec GetLastEvent(int action, long extraVal)
	{
		SysJournalTable.Rec result = null;
		if(action > 0) {
			try {
				Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
				if(tbl != null) {
					final String tn = tbl.GetName();
					String query = "SELECT * FROM " + tn + " WHERE Action=" + action;
					if(extraVal > 0)
						query = query + " and Extra=" + extraVal;
					query = query + " order by TimeStamp desc limit 1";
					android.database.Cursor cur = GetHandle().rawQuery(query, null);
					if(cur != null && cur.moveToFirst()) {
						result = new SysJournalTable.Rec();
						result.Init();
						result.Set(cur);
					}
				}
			} catch(StyloQException exn) {
				result = null;
			}
		}
		return result;
	}
	SysJournalTable.Rec GetLastObjEvent(int action, int objType, long objID)
	{
		SysJournalTable.Rec result = null;
		if(action > 0 && objType > 0 && objID > 0) {
			try {
				Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
				if(tbl != null) {
					final String tn = tbl.GetName();
					String query = "SELECT * FROM " + tn + " WHERE Action=" + action + " AND ObjType=" + objType + " AND ObjID=" + objID;
					query = query + " order by TimeStamp desc limit 1";
					android.database.Cursor cur = GetHandle().rawQuery(query, null);
					if(cur != null && cur.moveToFirst()) {
						result = new SysJournalTable.Rec();
						result.Init();
						result.Set(cur);
					}
				}
			} catch(StyloQException exn) {
				result = null;
			}
		}
		return result;
	}
	ArrayList <Integer> GetObjListByEventSince(int objType, ArrayList <Integer> actList, SLib.LDATETIME since)
	{
		ArrayList <Integer> result = null;
		if(objType > 0 && !SLib.LDATETIME.IsEmpty(since)) {
			long since_ms = since.ToEpochMilliseconds();
			try {
				Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
				if(tbl != null) {
					final String tn = tbl.GetName();
					String query = "SELECT * FROM " + tn + " WHERE TimeStamp>" + since_ms + " AND ObjType=" + objType;
					android.database.Cursor cur = GetHandle().rawQuery(query, null);
					if(cur != null && cur.moveToFirst()) {
						do {
							SysJournalTable.Rec rec = new SysJournalTable.Rec();
							rec.Init();
							rec.Set(cur);
							if(SLib.GetCount(actList) == 0 || actList.contains(rec.Action)) {
								if(result == null) {
									result = new ArrayList<Integer>();
									result.add((int)rec.ObjID);
								}
								else if(!result.contains((int)rec.ObjID))
									result.add((int)rec.ObjID);
							}
						} while(cur.moveToNext());
					}
				}
			} catch(StyloQException exn) {
				result = null;
			}
		}
		return result;
	}
	boolean LogEvent(int action, int obj, long id, long extData, boolean useTa)
	{
		boolean ok = false;
		long result_id = 0;
		SysJournalTable.Rec rec = null;
		Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
		if(tbl != null) {
			try {
				Transaction tra = new Transaction(this, useTa);
				rec = new SysJournalTable.Rec();
				rec.TimeStamp = System.currentTimeMillis();
				rec.Action = action;
				rec.ObjType = obj;
				rec.ObjID = id;
				rec.Extra = extData;
				result_id = tbl.Insert(rec);
				if(result_id > 0)
					ok = tra.Commit();
				else
					tra.Abort();
			} catch(StyloQException exn) {
				//exn.printStackTrace();
			}
		}
		return ok;
	}
	public long PutPeerEntry(long id, SecStoragePacket pack, boolean useTa) throws StyloQException
	{
		long result_id = 0;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			if(id == 0) { // insert packet
				if(pack.PreprocessBeforeStoring(null)) {
					Transaction tra = new Transaction(this, useTa);
					result_id = tbl.Insert(pack.Rec);
					if(result_id > 0)
						LogEvent(SLib.PPACN_OBJADD, PPOBJ_STYLOQBINDERY, result_id, 0, false);
					tra.Commit();
				}
			}
			else { // update packet
				SecStoragePacket ex_pack = GetPeerEntry(id);
				if(ex_pack != null) {
					if(pack == null) {
						Transaction tra = new Transaction(this, useTa);
						if(RemoveRec(tbl, id) > 0) {
							LogEvent(SLib.PPACN_OBJRMV, PPOBJ_STYLOQBINDERY, id, 0, false);
							if(tra.Commit())
								result_id = id;
						}
						else
							tra.Abort();
					}
					else {
						if(pack.PreprocessBeforeStoring(ex_pack)) {
							Transaction tra = new Transaction(this, useTa);
							if(UpdateRec(tbl, id, pack.Rec) > 0) {
								LogEvent(SLib.PPACN_OBJUPD, PPOBJ_STYLOQBINDERY, id, 0, false);
								if(tra.Commit())
									result_id = id;
							}
							else
								tra.Abort();
						}
					}
				}
			}
		}
		return result_id;
	}
	public SecStoragePacket GetPeerEntry(long id) throws StyloQException
	{
		SecStoragePacket result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT * FROM " + tn + " WHERE id=" + id;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				SecTable.Rec rec = new SecTable.Rec();
				rec.Init();
				rec.Set(cur);
				result = new SecStoragePacket(rec);
				if(result.Pool == null) {
					result = null;
				}
			}
		}
		return result;
	}
	public SecStoragePacket GetOwnPeerEntry() throws StyloQException
	{
		SecStoragePacket result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT * FROM " + tn + " WHERE kind=" + SecStoragePacket.kNativeService;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				SecTable.Rec rec = new SecTable.Rec();
				rec.Init();
				rec.Set(cur);
				result = new SecStoragePacket(rec);
				if(result.Pool == null) {
					result = null;
				}
			}
		}
		return result;
	}
	public SecStoragePacket GetForeignSvcCommandList(byte [] svcIdent) throws StyloQException
	{
		SecStoragePacket result = null;
		ArrayList<Long> doc_id_list = GetDocIdListByType(-1, SecStoragePacket.doctypCommandList, 0, svcIdent);
		if(doc_id_list != null && doc_id_list.size() > 0) {
			// Здесь надо бы что-то сделать если (doc_id_list.size() > 1) - это сбойная ситуация //
			long doc_id = doc_id_list.get(0);
			result = GetPeerEntry(doc_id);
		}
		return result;
	}
	public boolean DeleteForeignSvc(long id) throws StyloQException
	{
		boolean ok = false;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			SecStoragePacket pack = GetPeerEntry(id);
			if(pack != null && pack.Rec.Kind == SecStoragePacket.kForeignService) {
				Transaction tra = new Transaction(this,true);
				try {
					final String tn = tbl.GetName();
					//public static final int kSession        = 4;
					//public static final int kDocIncoming    = 6; // Входящие документы
					//public static final int kDocOutcominig  = 7; // Исходящие документы
					final int[] kind_list_to_delete = {SecStoragePacket.kSession, SecStoragePacket.kDocIncoming, SecStoragePacket.kDocOutcoming};
					for(int i = 0; i < kind_list_to_delete.length; i++) {
						int kind_to_delete = kind_list_to_delete[i];
						String query = "DELETE FROM " + tn + " WHERE CorrespondID=" + id + " and kind=" + kind_to_delete;
						GetHandle().execSQL(query);
					}
					{
						String query = "DELETE FROM " + tn + " WHERE id=" + id;
						GetHandle().execSQL(query);
					}
					ok = tra.Commit();
				} catch(SQLException exn) {
					ok = false;
					tra.Abort();
					throw new StyloQException(ppstr2.PPERR_JEXN_SQL, exn.getMessage());
				}
			}
		}
		return ok;
	}
	public ArrayList<Long> GetMediatorIdList() throws StyloQException
	{
		ArrayList<Long> result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT ID, Kind, Flags, BI, TimeStamp FROM " + tn + " WHERE kind=" + SecStoragePacket.kForeignService;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				do {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					if(rec.Kind == SecStoragePacket.kForeignService && (rec.Flags & SecStoragePacket.styloqfMediator) != 0) {
						if(result == null) {
							result = new ArrayList<Long>();
						}
						result.add(new Long(rec.ID));
					}
				} while(cur.moveToNext());
			}
		}
		return result;
	}
	public ArrayList<Long> GetForeignSvcIdList(boolean skipDups, boolean skipMediators) throws StyloQException
	{
		ArrayList<Long> result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			class InnerEntry {
				InnerEntry(long id, byte [] bi, long timeStamp)
				{
					ID = id;
					BI = bi;
					TimeStamp = timeStamp;
				}
				long ID;
				byte [] BI;
				long TimeStamp;
			}
			ArrayList <InnerEntry> inner_list = skipDups ? new ArrayList<InnerEntry>() : null;
			final String tn = tbl.GetName();
			String query = "SELECT ID, Kind, BI, TimeStamp FROM " + tn + " WHERE kind=" + SecStoragePacket.kForeignService;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				do {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					if(rec.Kind == SecStoragePacket.kForeignService) {
						if(result == null) {
							result = new ArrayList<Long>();
						}
						result.add(new Long(rec.ID));
						if(inner_list != null)
							inner_list.add(new InnerEntry(rec.ID, rec.BI, rec.TimeStamp));
					}
				} while(cur.moveToNext());
				if(inner_list != null) {
					Vector<Long> id_list_to_remove = null;
					// @v11.4.6 {
					if(skipMediators) {
						String sf = null;
						String sr = null;
						int i = inner_list.size();
						if(i > 0) do {
							i--;
							InnerEntry inner_entry = inner_list.get(i);
							SecStoragePacket en = GetPeerEntry(inner_entry.ID);
							if(en != null) {
								byte[] cfg_bytes = en.Pool.Get(SecretTagPool.tagConfig);
								if(SLib.GetLen(cfg_bytes) > 0) {
									StyloQConfig svc_cfg = new StyloQConfig();
									if(svc_cfg.FromJson(new String(cfg_bytes))) {
										sf = svc_cfg.Get(StyloQConfig.tagFeatures);
										sr = svc_cfg.Get(StyloQConfig.tagRole);
										if(SLib.GetLen(sf) > 0) {
											int _sf = Integer.valueOf(sf);
											if((_sf & StyloQConfig.featrfMediator) != 0) {
												if(id_list_to_remove == null)
													id_list_to_remove = new Vector<Long>();
												id_list_to_remove.add(inner_list.get(i).ID);
											}
										}
									}
								}
							}
						} while(i > 0);
					}
					// } @v11.4.6
					if(skipDups) {
						int i = inner_list.size();
						if(i > 0) do {
							i--;
							InnerEntry inner_entry1 = inner_list.get(i);
							if(id_list_to_remove == null || !id_list_to_remove.contains(inner_entry1.ID)) {
								int j = i;
								if(j > 0) do {
									j--;
									InnerEntry inner_entry2 = inner_list.get(j);
									if(id_list_to_remove == null || !id_list_to_remove.contains(inner_entry2.ID)) {
										if(SLib.AreByteArraysEqual(inner_entry2.BI, inner_entry1.BI)) {
											int idx_to_remove = -1;
											SecStoragePacket en1 = GetPeerEntry(inner_entry1.ID);
											SecStoragePacket en2 = GetPeerEntry(inner_entry2.ID);
											byte[] face1 = en1.Pool.Get(SecretTagPool.tagFace);
											byte[] face2 = en2.Pool.Get(SecretTagPool.tagFace);
											byte[] cfg1 = en1.Pool.Get(SecretTagPool.tagConfig);
											byte[] cfg2 = en2.Pool.Get(SecretTagPool.tagConfig);
											if(face1 != null) {
												if(face2 == null)
													idx_to_remove = j;
												else {
													if(inner_entry1.TimeStamp > inner_entry2.TimeStamp)
														idx_to_remove = j;
													else if(inner_entry1.TimeStamp < inner_entry2.TimeStamp)
														idx_to_remove = i;
												}
											}
											else if(face2 != null)
												idx_to_remove = i;
											if(idx_to_remove >= 0) {
												if(id_list_to_remove == null)
													id_list_to_remove = new Vector<Long>();
												id_list_to_remove.add(inner_list.get(idx_to_remove).ID);
											}
										}
									}
								} while(j > 0);
							}
						} while(i > 0);
					}
					if(id_list_to_remove != null) {
						id_list_to_remove.sort(new Comparator<Long>() {
							@Override public int compare(Long lh, Long rh)
							{
								return (lh < rh) ? +1 : ((lh > rh) ? -1 : 0);
							}
						});
						for(int k = 0; k < id_list_to_remove.size(); k++) {
							long id_to_remove = id_list_to_remove.get(k);
							result.removeIf(id -> id == id_to_remove);
						}
					}
				}
			}
		}
		return result;
	}
	public String MakeTextHashForForeignService(SecStoragePacket svcPack, int len) throws StyloQException
	{
		String result = null;
		try {
			if(len > 0 && len <= 20 && svcPack != null) {
				byte [] cli_ident = svcPack.Pool.Get(SecretTagPool.tagClientIdent);
				byte [] svc_ident = svcPack.Pool.Get(SecretTagPool.tagSvcIdent);
				if(SLib.GetLen(cli_ident) > 0 && SLib.GetLen(svc_ident) > 0) {
					MessageDigest digest = MessageDigest.getInstance("SHA-1");
					digest.update(svc_ident);
					digest.update(cli_ident);
					byte[] hash = digest.digest();
					if(SLib.GetLen(hash) > 0) {
						result = "";
						for(int i = 0; result.length() < len && i < hash.length; i++) {
							result += (char) ('A' + ((255 + hash[i]) % 26));
						}
					}
				}
			}
		} catch(NoSuchAlgorithmException exn) {
			result = null;
			throw new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		}
		return result;
	}
	public String MakeDocumentCode(byte [] svcIdent) throws StyloQException
	{
		String result = "";
		long c = GetNewCounter();
		if(SLib.GetLen(svcIdent) > 0) {
			SecStoragePacket svc_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, svcIdent);
			result = MakeTextHashForForeignService(svc_pack, 6);
		}
		{
			if(SLib.GetLen(result) > 0)
				result += "-";
			else
				result = "";
			String cs = Long.toString(c);
			final int min_dig = 5;
			if(cs.length() < min_dig) {
				for(int i = 0; i < (min_dig - cs.length()); i++)
					result += "0";
			}
			result += cs;
		}
		return result;
	}
	public byte [] MakeDocumentStorageIdent(byte [] svcIdent, UUID cmdUuid)
	{
		byte [] result = null;
		if(SLib.GetLen(svcIdent) > 0) {
			if(cmdUuid != null) {
				try {
					MessageDigest digest = MessageDigest.getInstance("SHA-1");
					digest.update(svcIdent);
					{
						ByteBuffer bb = ByteBuffer.wrap(new byte[16]);
						bb.putLong(cmdUuid.getMostSignificantBits());
						bb.putLong(cmdUuid.getLeastSignificantBits());
						digest.update(bb.array());
					}
					result = digest.digest();
					assert (result.length == 20);
				} catch(NoSuchAlgorithmException exn) {
					((StyloQApp) Ctx).SetLastError(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
				}
			}
			else
				result = svcIdent;
		}
		return result;
	}
	private SecStoragePacket InitDocumentPacket(int kind, int docType, long correspondId, byte [] ident, int expiryPeriodSec, SecretTagPool pool) throws StyloQException
	{
		SecStoragePacket result = new SecStoragePacket(kind);
		result.Rec.DocType = docType;
		result.Rec.TimeStamp = System.currentTimeMillis();
		result.Rec.CorrespondID = correspondId;
		result.Rec.BI = ident;
		result.Rec.Expiration = StyloQInterchange.EvaluateExpiryTime(expiryPeriodSec);
		result.Pool = pool;
		return result;
	}
	//
	// Descr: Высокоуровневая процедура, реализующая обновление статусов (и, возможно, иных атрибутов) документов
	//   после получения информации от сервиса.
	//
	public int AcceptDocumentRequestList(ArrayList <StyloQInterchange.DocumentRequestEntry> docReqList)
	{
		int    result = 0;
		int    upd_count = 0; // Количество измененных документов
		int    err_count = 0;
		if(docReqList != null && docReqList.size() > 0) {
			Transaction tra = new Transaction(this, true);
			for(int i = 0; i < docReqList.size(); i++) {
				StyloQInterchange.DocumentRequestEntry dre = docReqList.get(i);
				if(dre != null && dre.DocID > 0) {
					try {
						SecStoragePacket pack = GetPeerEntry(dre.DocID);
						final int preserve_flags = pack.Rec.Flags;
						boolean local_set_result = false;
						if((pack.Rec.Flags & SecStoragePacket.styloqfDocTransmission) != 0) {
							pack.Rec.Flags &= ~SecStoragePacket.styloqfDocTransmission;
							local_set_result = true;
						}
						if(dre.AfterTransmitStatus != 0)
							local_set_result = pack.Rec.SetDocStatus(dre.AfterTransmitStatus);
						if(local_set_result && preserve_flags != pack.Rec.Flags) {
							long local_result_id = PutPeerEntry(dre.DocID, pack, false);
							if(local_result_id == dre.DocID) {
								dre.DbAcceptStatus = StyloQInterchange.DocumentRequestEntry.AcceptionResult.Successed;
								upd_count++;
							}
							else if(local_result_id == 0) {
								dre.DbAcceptStatus = StyloQInterchange.DocumentRequestEntry.AcceptionResult.Error;
								err_count++;
							}
							else {
								dre.DbAcceptStatus = StyloQInterchange.DocumentRequestEntry.AcceptionResult.Error;
								err_count++;
								assert(local_result_id != dre.DocID); // В этом случае все плохо - у нас где-то тяжелая ошибка
							}
						}
						else
							dre.DbAcceptStatus = StyloQInterchange.DocumentRequestEntry.AcceptionResult.Skipped;
					} catch(StyloQException exn) {
						;
					}
				}
			}
			tra.Commit();
		}
		if(upd_count > 0)
			result = (err_count > 0) ? 2 : 1;
		else
			result = (err_count > 0) ? 0 : -1;
		return result;
	}
	private long Helper_PutDocument_Singleton(int direction, int packKind, int docType, long correspondId, byte [] ident, int docExpiry, SecretTagPool pool) throws StyloQException
	{
		long   result_id = 0;
		SecStoragePacket pack = InitDocumentPacket(packKind, docType, correspondId, ident, docExpiry, pool);
		{
			Transaction tra = new Transaction(this, true);
			// Документ такого типа может быть только один в комбинации {direction; rIdent}
			ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, 0, ident);
			if(ex_id_list != null) {
				for(int i = 0; i < ex_id_list.size(); i++) {
					long _id_to_remove = ex_id_list.get(i);
					PutPeerEntry(_id_to_remove, null, false); // @throw
				}
			}
			result_id = PutPeerEntry(0, pack, false);
			tra.Commit();
		}
		return result_id;
	}
	//
	// Note: Если direction == 0, то вид пакета определяется по параметру docType
	//
	public long PutDocument(int direction, int docType, int docFlags, byte [] ident, long correspondId, SecretTagPool pool) throws StyloQException
	{
		long   result_id = 0;
		if(SLib.GetLen(ident) > 0) {
			//SecStoragePacket cli_pack = SearchGlobalIdentEntry(SecStoragePacket.kClient, ident);
			//SecStoragePacket svc_pack = (cli_pack != null) ? null : SearchGlobalIdentEntry(SecStoragePacket.kForeignService, ident);
			//if(cli_pack != null || svc_pack != null) {
			//
			// Генеральный способ передачи метаинформации о документе - блок DocDeclaration.
			// Однако, в некоторых случаях метаданные документа могут находится непосредственно
			// в нем. Соответственно, сначала мы пытаемся найти и разобрать декларацию (tagDocDeclaration) и, если
			// ее нет, то пытаемся получить метаданные прямо из документа (tagRawData)
			//
			String raw_doc_type = null;
			long doc_time = 0;
			int doc_expiry = 0;
			if(correspondId > 0) {
				SecStoragePacket corr_pack = GetPeerEntry(correspondId);
				THROW(corr_pack != null, ppstr2.PPERR_SQ_CORRESPONDOBJNFOUND, Long.toString(correspondId));
				THROW(corr_pack.Rec.Kind == SecStoragePacket.kClient || corr_pack.Rec.Kind == SecStoragePacket.kForeignService, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
			}
			int pack_kind = 0;
			if(docType == SecStoragePacket.doctypCurrentState)
				pack_kind = SecStoragePacket.kCurrentState;
			else if(direction > 0)
				pack_kind = SecStoragePacket.kDocOutcoming;
			else if(direction < 0)
				pack_kind = SecStoragePacket.kDocIncoming;
			if(pool == null) { // delete doc
				Transaction tra = new Transaction(this, true);
				ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, correspondId, ident);
				if(ex_id_list != null && ex_id_list.size() > 0) {
					for(int i = 0; i < ex_id_list.size(); i++) {
						long local_id = ex_id_list.get(i);
						result_id = PutPeerEntry(local_id, null, false); // @throw
					}
				}
				else
					result_id = -1;
				tra.Commit();
			}
			else {
				JSONObject js_document = null; // generic document inserted into raw_doc
				StyloQCommand.DocDeclaration doc_decl = null;
				JSONObject js_raw_doc = pool.GetJsonObject(SecretTagPool.tagRawData);
				byte[] raw_doc_decl = pool.Get(SecretTagPool.tagDocDeclaration);
				if(SLib.GetLen(raw_doc_decl) > 0) {
					doc_decl = new StyloQCommand.DocDeclaration();
					if(doc_decl.FromJson(new String(raw_doc_decl))) {
						raw_doc_type = doc_decl.Type;
						doc_time = doc_decl.Time;
						doc_expiry = doc_decl.ResultExpiryTimeSec;
					}
				}
				if(js_raw_doc != null) {
					if(doc_decl == null) {
						raw_doc_type = js_raw_doc.optString("doctype", "");
						doc_time = js_raw_doc.optLong("time", 0);
						doc_expiry = js_raw_doc.optInt("expir_time_sec", 0);
					}
					js_document = js_raw_doc.optJSONObject("document");
					if(js_document == null)
						js_document = js_raw_doc;
					if(raw_doc_type != null) {
						if(docType == SecStoragePacket.doctypGeneric) {
							if(js_document != null) {
								long _ex_doc_id_from_json = (direction > 0) ? js_document.optLong("ID", 0) : 0;
								long _ex_doc_id_from_db = 0;
								SecStoragePacket pack = InitDocumentPacket(pack_kind, docType, correspondId, ident, doc_expiry, pool);
								pack.Rec.Flags |= (docFlags & (SecStoragePacket.styloqfDocStatusFlags | SecStoragePacket.styloqfDocTransmission));
								{
									Transaction tra = new Transaction(this, true);
									// Документ такого типа может быть только один в комбинации {direction; rIdent}
									ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, correspondId, ident);
									if(ex_id_list != null) {
										for(int i = 0; i < ex_id_list.size(); i++) {
											long local_id = ex_id_list.get(i);
											// Последний (из гипотетически нескольких) встретившийся документ считаем нашим, остальные - удаляем
											if(i == ex_id_list.size() - 1)
												_ex_doc_id_from_db = local_id;
											else
												PutPeerEntry(local_id, null, false); // @throw
										}
									}
									result_id = PutPeerEntry(_ex_doc_id_from_db, pack, false);
									tra.Commit();
								}
							}
						}
						else if(docType == SecStoragePacket.doctypCurrentState) { // @v11.7.0
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypDebtList) { // @v11.5.5
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypCommandList && raw_doc_type.equalsIgnoreCase("commandlist")) {
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypReport && raw_doc_type.equalsIgnoreCase("view")) {
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypOrderPrereq && raw_doc_type.equalsIgnoreCase("orderprereq")) {
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypIndoorSvcPrereq && raw_doc_type.equalsIgnoreCase("indoorsvcprereq")) { // @v11.4.5
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
						else if(docType == SecStoragePacket.doctypIncomingList &&
								raw_doc_type.equalsIgnoreCase("incominglistorder") ||
								raw_doc_type.equalsIgnoreCase("incominglistccheck")/*@v11.4.8*/ ||
								raw_doc_type.equalsIgnoreCase("incominglisttsess")/*@v11.6.5*/) {
							result_id = Helper_PutDocument_Singleton(direction, pack_kind, docType, correspondId, ident, doc_expiry, pool);
						}
					}
				}
			}
		}
		return result_id;
	}
	public SecStoragePacket FindRecentIncomingModDoc(int docType, long correspondId, byte [] ident, UUID orgCmdUuid, ArrayList <UUID> possibleDocUuidList)
	{
		final int _direction =  SecStoragePacket.kDocIncoming;
		SecStoragePacket result = null;
		if(possibleDocUuidList != null && possibleDocUuidList.size() > 0) {
			try {
				Database.Table tbl = CreateTable("SecTable");
				if(tbl != null) {
					final String tn = tbl.GetName();
					SecTable.Rec last_suitable_rec = null;
					String query = "SELECT * FROM " + tn + " WHERE docType=" + docType;
					if(SLib.GetLen(ident) > 0) {
						query += " and BI=x'" + SLib.ByteArrayToHexString(ident) + "'";
					}
					query += " and kind=" + _direction;
					if(correspondId > 0)
						query += " and CorrespondID=" + correspondId;
					query += " and Flags=" + (SecStoragePacket.styloqdocstINCOMINGMOD << 1); // @fixme Опасный критерий: пока закладываемся на то, что с этим флагом ничто более не комбинируется.
					query += " order by Kind, BI, TimeStamp"; // #idx1
					android.database.Cursor cur = GetHandle().rawQuery(query, null);
					if(cur != null && cur.moveToFirst()) {
						do {
							SecStoragePacket current_pack = null;
							SecTable.Rec rec = new SecTable.Rec();
							rec.Init();
							rec.Set(cur);
							if(rec.Kind == _direction) {
								boolean is_suitable = false;
								JSONObject js_doc = null;

								current_pack = new SecStoragePacket(rec);
								if(current_pack.Pool != null) {
									byte[] raw_data = current_pack.Pool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_data) > 0) {
										String txt_raw_data = new String(raw_data);
										if(SLib.GetLen(txt_raw_data) > 0) {
											try {
												js_doc = new JSONObject(txt_raw_data);
												if(js_doc != null) {
													if(orgCmdUuid != null) {
														UUID local_uuid = SLib.strtouuid(js_doc.optString("orgcmduuid", null));
														if(local_uuid != null && local_uuid.compareTo(orgCmdUuid) == 0)
															is_suitable = true;
													}
													else
														is_suitable = true;
													if(is_suitable && possibleDocUuidList != null) {
														UUID local_uuid = SLib.strtouuid(js_doc.optString("uuid", null));
														is_suitable = false;
														if(local_uuid != null) {
															for(int uididx = 0; !is_suitable && uididx < possibleDocUuidList.size(); uididx++) {
																if(possibleDocUuidList.get(uididx).compareTo(local_uuid) == 0)
																	is_suitable = true;
															}
														}
													}
												}
											} catch(JSONException exn) {
												;
											}
										}
									}
								}
								if(is_suitable) {
									if(result == null || result.Rec.TimeStamp < rec.TimeStamp) {
										if(current_pack != null)
											result = current_pack;
										else
											result = new SecStoragePacket(rec);
									}
								}
							}
						} while(cur.moveToNext());
					}
				}
			} catch(StyloQException exn) {
				result = null;
			}
		}
		return result;
	}
	public SecStoragePacket FindRecentDraftDoc(int docType, long correspondId, byte [] ident, UUID orgCmdUuid)
	{
		final int _direction =  SecStoragePacket.kDocOutcoming;
		SecStoragePacket result = null;
		try {
			Database.Table tbl = CreateTable("SecTable");
			if(tbl != null) {
				final String tn = tbl.GetName();
				SecTable.Rec last_suitable_rec = null;
				String query = "SELECT * FROM " + tn + " WHERE docType=" + docType;
				if(SLib.GetLen(ident) > 0) {
					query += " and BI=x'" + SLib.ByteArrayToHexString(ident) + "'";
				}
				query += " and kind=" + _direction;
				if(correspondId > 0)
					query += " and CorrespondID=" + correspondId;
				query += " and Flags=" + (SecStoragePacket.styloqdocstDRAFT << 1); // @fixme Опасный критерий: пока закладываемся на то, что с этим флагом ничто более не комбинируется.
				query += " order by Kind, BI, TimeStamp"; // #idx1
				android.database.Cursor cur = GetHandle().rawQuery(query, null);
				if(cur != null && cur.moveToFirst()) {
					do {
						SecStoragePacket current_pack = null;
						SecTable.Rec rec = new SecTable.Rec();
						rec.Init();
						rec.Set(cur);
						if(rec.Kind == _direction) {
							boolean is_suitable = false;
							if(orgCmdUuid == null)
								is_suitable = true;
							else {
								current_pack = new SecStoragePacket(rec);
								if(current_pack.Pool != null) {
									byte[] raw_data = current_pack.Pool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_data) > 0) {
										String txt_raw_data = new String(raw_data);
										if(SLib.GetLen(txt_raw_data) > 0) {
											try {
												JSONObject js = new JSONObject(txt_raw_data);
												if(js != null) {
													UUID local_uuid = SLib.strtouuid(js.optString("orgcmduuid", null));
													if(local_uuid != null && local_uuid.compareTo(orgCmdUuid) == 0)
														is_suitable = true;
												}
											} catch(JSONException exn) {
												;
											}
										}
									}
								}
							}
							if(is_suitable) {
								if(result == null || result.Rec.TimeStamp < rec.TimeStamp) {
									if(current_pack != null)
										result = current_pack;
									else
										result = new SecStoragePacket(rec);
								}
							}
						}
					} while(cur.moveToNext());
				}
			}
		} catch(StyloQException exn) {
			result = null;
		}
		return result;
	}
	public ArrayList<Long> GetDocIdListByType(int direction, int docType, long correspondId, byte [] ident) throws StyloQException
	{
		//return GetDocIdListByType(direction, docType, correspondId, null, ident);
		return Helper_GetDocIdListByType(direction, docType, correspondId, null, ident, false);
	}
	//
	// ARG(direction IN): <0 - incoming, >0 - outcoming, 0 - no matter
	//
	public ArrayList<Long> GetDocIdListByType(int direction, int docType, long correspondId, ArrayList<Integer> statusList, byte [] ident) throws StyloQException
	{
		return Helper_GetDocIdListByType(direction, docType, correspondId, statusList, ident, false);
	}
	public long GetRecentDocIdByType(int direction, int docType, long correspondId, ArrayList<Integer> statusList, byte [] ident) throws StyloQException
	{
		ArrayList<Long> list = Helper_GetDocIdListByType(direction, docType, correspondId, statusList, ident, true);
		int _c = SLib.GetCount(list);
		assert(_c == 0 || _c == 1);
		return (_c == 0) ? 0 : list.get(0);
	}
	//
	// ARG(direction IN): <0 - incoming, >0 - outcoming, 0 - no matter
	//
	private ArrayList<Long> Helper_GetDocIdListByType(int direction, int docType, long correspondId, ArrayList<Integer> statusList, byte [] ident, boolean recentOnly) throws StyloQException
	{
		ArrayList<Long> result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			long recent_timestamp = 0; // for recentOnly
			long recent_id = 0; // for recentOnly
			final String tn = tbl.GetName();
			String query = "SELECT ID, Kind, Flags FROM " + tn + " WHERE docType=" + docType;
			if(SLib.GetLen(ident) > 0) {
				query += " and BI=x'" + SLib.ByteArrayToHexString(ident) + "'";
			}
			if(docType == SecStoragePacket.doctypCurrentState) // @v11.7.0
				query += " and kind=" + SecStoragePacket.kCurrentState;
			else if(direction > 0)
				query += " and kind=" + SecStoragePacket.kDocOutcoming;
			else if(direction < 0)
				query += " and kind=" + SecStoragePacket.kDocIncoming;
			else
				query += " and (kind=" + SecStoragePacket.kDocOutcoming + " or kind=" + SecStoragePacket.kDocIncoming + ")";
			if(correspondId > 0)
				query += " and CorrespondID=" + correspondId;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				do {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					boolean is_suitable = true;
					if(!SecStoragePacket.IsDocKind(rec.Kind))
						is_suitable = false;
					else {
						if(statusList != null && !statusList.contains(new Integer(rec.GetDocStatus()))) {
							is_suitable = false;
						}
					}
					if(is_suitable) {
						if(recentOnly) {
							if(recent_id == 0 || rec.TimeStamp > recent_timestamp) {
								recent_id = rec.ID;
								recent_timestamp = rec.TimeStamp;
							}
						}
						else {
							if(result == null)
								result = new ArrayList<Long>();
							result.add(new Long(rec.ID));
						}
					}
				} while(cur.moveToNext());
			}
			if(recentOnly) {
				assert(result == null);
				if(recent_id > 0) {
					result = new ArrayList<Long>();
					result.add(new Long(recent_id));
				}
			}
		}
		return result;
	}
	public long GetNewCounter()
	{
		// {706FD772-DC3F-452C-93A1-579E01F3E8D1}
		final UUID uid = SLib.strtouuid("706FD772-DC3F-452C-93A1-579E01F3E8D1");
		long result = 0;
		try {
			Database.Table tbl = CreateTable("SecTable");
			if(tbl != null) {
				Transaction tra = new Transaction(this, true);
				final String tn = tbl.GetName();
				String query = "SELECT * FROM " + tn + " WHERE kind=" + SecStoragePacket.kCounter;
				android.database.Cursor cur = GetHandle().rawQuery(query, null);
				if(cur != null && cur.moveToFirst()) {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					rec.TimeStamp = System.currentTimeMillis();
					rec.Counter++;
					if(UpdateRec(tbl, rec.ID, rec) > 0) {
						result = rec.Counter;
					}
					else {
						SLib.LOG_e("GetNewCounter: UpdateRec(tbl, rec.ID, rec)");
					}
				}
				else {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					{
						ByteBuffer tbb = ByteBuffer.allocate(16);
						tbb.putLong(uid.getMostSignificantBits());
						tbb.putLong(uid.getLeastSignificantBits());
						rec.BI = tbb.array();
					}
					rec.Kind = SecStoragePacket.kCounter;
					rec.TimeStamp = System.currentTimeMillis();
					rec.Counter = 1;
					long new_id = InsertRec(tbl, rec);
					if(new_id > 0) {
						result = rec.Counter;
					}
				}
				if(result > 0)
					tra.Commit();
				else
					tra.Abort();
			}
			else {
				SLib.LOG_e("GetNewCounter: tbl != null");
			}
		} catch(StyloQException exn) {
			SLib.LOG_e("GetNewCounter: StyloQException " + exn.getMessage());
			result = 0;
		}
		return result;
	}
	public  StyloQCurrentState GetCurrentState(final byte [] svcIdent, UUID orgCmdUuid)
	{
		StyloQCurrentState result = null;
		if(SLib.GetLen(svcIdent) > 0 && orgCmdUuid != null) {
			try {
				StyloQDatabase.SecStoragePacket svc_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, svcIdent);
				if(svc_pack != null) {
					final byte[] doc_ident = MakeDocumentStorageIdent(svcIdent, orgCmdUuid);
					long id = GetRecentDocIdByType(0, SecStoragePacket.doctypCurrentState, svc_pack.Rec.ID, null, doc_ident);
					SecStoragePacket doc_packet = (id > 0) ? GetPeerEntry(id) : null;
					if(doc_packet != null) {
						JSONObject js_obj = doc_packet.Pool.GetJsonObject(SecretTagPool.tagRawData);
						if(js_obj != null) {
							result = new StyloQCurrentState(svcIdent, orgCmdUuid);
							if(!result.FromJsonObj(js_obj))
								result = null;
							else {
								assert(SLib.AreByteArraysEqual(result.SvcIdent, svcIdent) && SLib.AreUUIDsEqual(result.OrgCmdUuid, orgCmdUuid));
								if(!SLib.AreByteArraysEqual(result.SvcIdent, svcIdent) || !SLib.AreUUIDsEqual(result.OrgCmdUuid, orgCmdUuid))
									result = null;
							}
						}
					}
				}
			} catch(StyloQException exn) {
				result = null;
			}
		}
		return result;
	}
	public  long StoreCurrentState(StyloQCurrentState cs)
	{
		long result = 0;
		if(cs != null && SLib.GetLen(cs.SvcIdent) > 0 && cs.OrgCmdUuid != null) {
			try {
				JSONObject js_cs = cs.ToJsonObj();
				if(js_cs != null) {
					String js_cs_text = js_cs.toString();
					if(SLib.GetLen(js_cs_text) > 0) {
						StyloQDatabase.SecStoragePacket svc_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, cs.SvcIdent);
						if(svc_pack != null) {
							SecretTagPool doc_pool = null;
							if(!cs.IsEmpty()) {
								doc_pool = new SecretTagPool();
								SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
								doc_pool.Put(SecretTagPool.tagRawData, js_cs_text.getBytes(StandardCharsets.UTF_8), ds);
							}
							byte[] doc_ident = MakeDocumentStorageIdent(cs.SvcIdent, cs.OrgCmdUuid);
							result = PutDocument(0/**/, SecStoragePacket.doctypCurrentState, 0, doc_ident, svc_pack.Rec.ID, doc_pool);
						}
					}
				}
			} catch(StyloQException exn) {
				result = 0;
			}
		}
		return result;
	}
	public  StyloQFace GetDefaultFace(ArrayList<StyloQFace> outerFaceList, boolean useTa)
	{
		StyloQFace result = null;
		try {
			ArrayList<StyloQFace> face_list = (outerFaceList != null) ? outerFaceList : GetFaceList();
			if(SLib.GetCount(face_list) > 0) {
				final int vtypes[] = { StyloQFace.vAnonymous, StyloQFace.vArbitrary, StyloQFace.vVerifiable };
				for(int i = 0; result == null && i < vtypes.length; i++) {
					for(StyloQFace iter : face_list) {
						int v = iter.GetVerifiability();
						if(v == vtypes[i]) {
							result = iter;
							break;
						}
					}
				}
			}
			if(result == null) { // Если не удалось найти ни одного лика, то создаем анонимный лик по умолчанию
				StyloQFace default_face = new StyloQFace();
				default_face.Set(StyloQFace.tagCommonName, 0, "Anonym-" + UUID.randomUUID().toString());
				default_face.SetVerifiability(StyloQFace.vAnonymous);
				String jstext = default_face.ToJson();
				if(SLib.GetLen(jstext) > 0) {
					SecStoragePacket sp = new SecStoragePacket(SecStoragePacket.kFace);
					sp.Pool.Put(SecretTagPool.tagSelfyFace, jstext.getBytes(StandardCharsets.UTF_8));
					long new_id = PutPeerEntry(sp.Rec.ID, sp, useTa);
					if(new_id > 0)
						result = GetFace(new_id, SecretTagPool.tagSelfyFace, null);
				}
			}
		} catch(StyloQException exn) {
			result = null;
		}
		return result;
	}
	private StyloQFace Implement_GetFace(SecStoragePacket pack, int tag, StyloQFace exItem)
	{
		StyloQFace result = null;
		if(pack != null && pack.Rec.Kind == SecStoragePacket.kFace && pack.Pool != null) {
			byte [] raw_face_data = pack.Pool.Get(/*SecretTagPool.tagFace*/tag);
			if(raw_face_data != null) {
				String text_face_data = new String(raw_face_data);
				if(text_face_data != null) {
					if(exItem != null) {
						exItem.ID = pack.Rec.ID;
						exItem.BI = pack.Rec.BI;
						if(exItem.FromJson(text_face_data))
							result = exItem;
					}
					else {
						StyloQFace face_entry = new StyloQFace();
						face_entry.ID = pack.Rec.ID;
						face_entry.BI = pack.Rec.BI;
						if(face_entry.FromJson(text_face_data))
							result = face_entry;
					}
				}
			}
		}
		return result;
	}
	public StyloQFace GetFace(long id, int tag, StyloQFace exItem) throws StyloQException
	{
		return Implement_GetFace(GetPeerEntry(id), tag, exItem);
	}
	public @NonNull ArrayList <StyloQFace> GetFaceList() throws StyloQException
	{
		ArrayList <StyloQFace> result = new ArrayList<StyloQFace>();
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT * FROM " + tn + " WHERE kind=" + SecStoragePacket.kFace;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				do {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					StyloQFace face_entry = Implement_GetFace(new SecStoragePacket(rec), SecretTagPool.tagSelfyFace,null);
					if(face_entry != null)
						result.add(face_entry);
				} while(cur.moveToNext());
			}
		}
		return result;
	}
	public boolean IsThereFaceRefs(final byte [] faceIdent)
	{
		boolean result = false;
		if(SLib.GetLen(faceIdent) > 0) {
			try {
				SecStoragePacket pack = GetOwnPeerEntry();
				if(pack != null) {
					byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
					if(SLib.GetLen(cfg_bytes) > 0) {
						String cfg_json = new String(cfg_bytes);
						StyloQConfig cfg = new StyloQConfig();
						cfg.FromJson(cfg_json);
						String face_hex = cfg.Get(StyloQConfig.tagDefFace);
						byte[] def_face_ref = (SLib.GetLen(face_hex) > 0) ? Base64.getDecoder().decode(face_hex) : null;
						if(SLib.AreByteArraysEqual(def_face_ref, faceIdent)) {
							result = true;
						}
					}
				}
				if(!result) {
					ArrayList <Long> svc_id_list = GetForeignSvcIdList(false, false);
					if(svc_id_list != null && svc_id_list.size() > 0) {
						for(int i = 0; !result && i < svc_id_list.size(); i++) {
							long svc_id = svc_id_list.get(i);
							StyloQDatabase.SecStoragePacket cur_entry = GetPeerEntry(svc_id);
							if(cur_entry != null) {
								byte[] face_ref = cur_entry.Pool.Get(SecretTagPool.tagAssignedFaceRef);
								if(SLib.AreByteArraysEqual(face_ref, faceIdent)) {
									result = true;
								}
							}
						}
					}
				}
			} catch(StyloQException exn) {

			}
		}
		return result;
	}
	public SecStoragePacket SearchGlobalIdentEntry(int kind, final byte [] ident) throws StyloQException
	{
		SecStoragePacket result = null;
		if(SLib.GetLen(ident) > 0) {
			Database.Table tbl = CreateTable("SecTable");
			if(tbl != null) {
				final String tn = tbl.GetName();
				String query = "SELECT * FROM " + tn + " WHERE kind=" + kind + " and bi=x'" + SLib.ByteArrayToHexString(ident) + "'";
				android.database.Cursor cur = GetHandle().rawQuery(query, null);
				if(cur != null && cur.moveToFirst()) {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					result = new SecStoragePacket(rec);
					if(result.Pool == null) {
						result = null;
					}
				}
			}
		}
		return result;
	}
	public ArrayList <StyloQInterchange.SvcNotification> GetNotifivationList(long svcID, SLib.LDATETIME since, boolean unprocessedOnly)
	{
		return Helper_GetNotifivationList_new(svcID, since, unprocessedOnly, false);
	}
	public boolean IsThereUnprocessedNotifications(long svcID, SLib.LDATETIME since)
	{
		ArrayList <StyloQInterchange.SvcNotification> list = Helper_GetNotifivationList_new(svcID, since, true, true);
		return (list != null && list.size() > 0);
	}
	public boolean IsThereAnyNotifications(long svcID, SLib.LDATETIME since)
	{
		ArrayList <StyloQInterchange.SvcNotification> list = Helper_GetNotifivationList_new(svcID, since, false, true);
		return (list != null && list.size() > 0);
	}
	private ArrayList <StyloQInterchange.SvcNotification> Helper_GetNotifivationList_before90v(long svcID, SLib.LDATETIME since, boolean unprocessedOnly, boolean single)
	{
		ArrayList <StyloQInterchange.SvcNotification> result = null;
		try {
			Database.Table tbl = CreateTable("SecTable");
			if(tbl != null) {
				final String tn = tbl.GetName();
				long since_epoch_ms = SLib.LDATETIME.IsEmpty(since) ? 0 : since.ToEpochMilliseconds();
				String query = "SELECT * FROM " + tn + " WHERE kind=" + SecStoragePacket.kNotification_before90v;
				if(svcID > 0) {
					query += " " + "and CorrespondID=" + svcID;
				}
				if(since_epoch_ms > 0) {
					query += " " + "and TimeStamp>" + since_epoch_ms;
				}
				android.database.Cursor cur = GetHandle().rawQuery(query, null);
				if(cur != null && cur.moveToFirst()) {
					do {
						SecTable.Rec rec = new SecTable.Rec();
						rec.Init();
						rec.Set(cur);
						if(!unprocessedOnly || (rec.Flags & SecStoragePacket.styloqfProcessed) == 0) {
							SecStoragePacket pack = new SecStoragePacket(rec);
							JSONObject js_obj = (pack.Pool != null) ? pack.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
							if(js_obj != null) {
								StyloQInterchange.SvcNotification item = new StyloQInterchange.SvcNotification();
								if(item.FromJsonObj(js_obj)) {
									if(result == null)
										result = new ArrayList<StyloQInterchange.SvcNotification>();
									item.InternalID = rec.ID;
									item.SvcID = rec.CorrespondID;
									item.Processed = ((rec.Flags & SecStoragePacket.styloqfProcessed) != 0);
									result.add(item);
									if(single)
										break;
								}
							}
						}
					} while(cur.moveToNext());
				}
			}
		} catch(StyloQException exn) {
			result = null;
		}
		return result;
	}
	private ArrayList <StyloQInterchange.SvcNotification> Helper_GetNotifivationList_new(long svcID, SLib.LDATETIME since, boolean unprocessedOnly, boolean single)
	{
		ArrayList <StyloQInterchange.SvcNotification> result = null;
		try {
			Database.Table tbl = CreateTable("NotificationTable2");
			if(tbl != null) {
				final String tn = tbl.GetName();
				long since_epoch_ms = SLib.LDATETIME.IsEmpty(since) ? 0 : since.ToEpochMilliseconds();
				String query = "SELECT * FROM " + tn; // + " WHERE kind=" + SecStoragePacket.kNotification_before90v;
				if(svcID > 0) {
					query += " " + "where SvcID=" + svcID + " " + "and TimeStamp> + since_epoch_ms";
				}
				if(since_epoch_ms > 0) {
					query += " " + "where TimeStamp>" + since_epoch_ms;
				}
				android.database.Cursor cur = GetHandle().rawQuery(query, null);
				if(cur != null && cur.moveToFirst()) {
					do {
						NotificationTable2.Rec rec = new NotificationTable2.Rec();
						rec.Init();
						rec.Set(cur);
						if(!unprocessedOnly || (rec.Flags & SecStoragePacket.styloqfProcessed) == 0) {
							NotificationStoragePacket pack = new NotificationStoragePacket(rec);
							JSONObject js_obj = (pack.Pool != null) ? pack.Pool.GetJsonObject(SecretTagPool.tagRawData) : null;
							if(js_obj != null) {
								StyloQInterchange.SvcNotification item = new StyloQInterchange.SvcNotification();
								if(item.FromJsonObj(js_obj)) {
									if(result == null)
										result = new ArrayList<StyloQInterchange.SvcNotification>();
									item.InternalID = rec.ID;
									// @diag {
									if(!SLib.AreByteArraysEqual(rec.Ident, item.Ident_before90v)) {

									}
									// } @diag
									item.Ident_before90v = rec.Ident;
									item.SvcID = rec.SvcID;
									item.Processed = ((rec.Flags & SecStoragePacket.styloqfProcessed) != 0);
									result.add(item);
									if(single)
										break;
								}
							}
						}
					} while(cur.moveToNext());
				}
			}
		} catch(StyloQException exn) {
			result = null;
		}
		return result;
	}
	/*public boolean RegisterNotificationAsSeen_beforev90v(long id, boolean useTa)
	{
		boolean result = false;
		if(id > 0) {
			try {
				SecStoragePacket pack = GetPeerEntry(id);
				if(pack != null && pack.Rec.Kind == SecStoragePacket.kNotification_before90v && (pack.Rec.Flags & SecStoragePacket.styloqfProcessed) == 0) {
					pack.Rec.Flags |= SecStoragePacket.styloqfProcessed;
					long result_id = PutPeerEntry(id, pack, useTa);
					if(result_id == id)
						result = true;
				}
			} catch(StyloQException exn) {
				;
			}
		}
		return result;
	}*/
	public boolean RegisterNotificationAsSeen_new(long id, boolean useTa)
	{
		boolean result = false;
		if(id > 0) {
			try {
				NotificationStoragePacket pack = GetNotificationEntry(id);
				if(pack != null && (pack.Rec.Flags & SecStoragePacket.styloqfProcessed) == 0) {
					pack.Rec.Flags |= SecStoragePacket.styloqfProcessed;
					long result_id = PutNotificationEntry(id, pack, useTa);
					if(abs(result_id) == id)
						result = true;
				}
			} catch(StyloQException exn) {
				;
			}
		}
		return result;
	}
	//
	// Returns:
	//   0 - error
	//   >0 - идентификатор созданной записи уведомления
	//   <0 - отрицательное значение идентификатор уже существующей записи уведомления
	//
	/*public long StoreNotification_before90v(final byte [] svcIdent, StyloQInterchange.SvcNotification item, boolean useTa) throws StyloQException
	{
		long    result_id = 0;
		final int svcidlen = SLib.GetLen(svcIdent);
		if(item != null && SLib.GetLen(item.Ident_before90v) > 0 && svcidlen > 0) {
			long correspind_id = 0;
			Transaction tra = new Transaction(this, useTa);
			SecStoragePacket correspond_pack = null;
			correspond_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, svcIdent);
			if(correspond_pack != null && correspond_pack.Rec.Kind == SecStoragePacket.kForeignService) {
				final long correspond_id = correspond_pack.Rec.ID;
				SecStoragePacket ex_pack = SearchGlobalIdentEntry(SecStoragePacket.kNotification_before90v, item.Ident_before90v);
				if(ex_pack != null) {
					result_id = -ex_pack.Rec.ID; // Запись найдена: возвращает отрицательное значение идентификатора
				}
				else {
					JSONObject js_obj = item.ToJsonObj(false);
					if(js_obj != null) {
						SecStoragePacket pack = new SecStoragePacket(SecStoragePacket.kNotification_before90v);
						pack.Rec.ID = 0;
						pack.Rec.BI = item.Ident_before90v;
						pack.Rec.TimeStamp = item.EventOrgTime.ToEpochMilliseconds();
						pack.Rec.CorrespondID = correspond_id;
						pack.Rec.Expiration = 0;
						{
							String js_obj_text = js_obj.toString();
							if(SLib.GetLen(js_obj_text) > 0) {
								pack.Pool.Put(SecretTagPool.tagRawData, js_obj_text.getBytes(StandardCharsets.UTF_8));
								result_id = PutPeerEntry(0, pack, false);
							}
							else {
								; // @todo @err
							}
						}
					}
					else {
						; // @todo @err
					}
				}
			}
			tra.Commit();
		}
		return result_id;
	}*/
	public long StoreNotification_new(StyloQInterchange.SvcNotification item, boolean useTa) throws StyloQException
	{
		long    result_id = 0;
		//final int svcidlen = SLib.GetLen(svcIdent);
		if(item != null && SLib.GetLen(item.Ident_before90v) > 0 && item.SvcID > 0) {
			Transaction tra = new Transaction(this, useTa);
			SecStoragePacket correspond_pack = null;
			//correspond_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, svcIdent);
			correspond_pack = GetPeerEntry(item.SvcID);
			if(correspond_pack != null && correspond_pack.Rec.Kind == SecStoragePacket.kForeignService) {
				//final long correspond_id = correspond_pack.Rec.ID;
				//NotificationStoragePacket ex_pack = SearchGlobalIdentEntry(SecStoragePacket.kNotification_before90v, item.Ident);
				NotificationStoragePacket ex_pack = GetNotificationEntry(item.InternalID);
				if(ex_pack != null) {
					result_id = -ex_pack.Rec.ID; // Запись найдена: возвращает отрицательное значение идентификатора
				}
				else {
					JSONObject js_obj = item.ToJsonObj(false);
					if(js_obj != null) {
						NotificationStoragePacket pack = new NotificationStoragePacket();
						pack.Rec.ID = 0;
						//pack.Rec.BI = item.Ident;
						pack.Rec.Ident = item.Ident_before90v;
						if(item.EventOrgTime != null)
							pack.Rec.TimeStamp = item.EventOrgTime.ToEpochMilliseconds();
						else if(item.EventIssueTime != null)
							pack.Rec.TimeStamp = item.EventIssueTime.ToEpochMilliseconds();
						else if(item.ObjNominalTime != null)
							pack.Rec.TimeStamp = item.ObjNominalTime.ToEpochMilliseconds();
						pack.Rec.SvcID = item.SvcID;
						pack.Rec.Expiration = 0;
						{
							String js_obj_text = js_obj.toString();
							if(SLib.GetLen(js_obj_text) > 0) {
								pack.Pool.Put(SecretTagPool.tagRawData, js_obj_text.getBytes(StandardCharsets.UTF_8));
								result_id = PutNotificationEntry(0, pack, false);
							}
							else {
								; // @todo @err
							}
						}
					}
					else {
						; // @todo @err
					}
				}
			}
			tra.Commit();
		}
		return result_id;
	}
	public long StoreSession(long id, SecStoragePacket pack, boolean useTa) throws StyloQException
	{
		long result_id = 0;
		try {
			long correspond_id = 0;
			SecStoragePacket org_pack;
			SecStoragePacket correspond_pack = null;
			Transaction tra = new Transaction(this, useTa);
			if(pack != null) {
				int correspond_type = 0;
				if(pack.Pool.Get(SecretTagPool.tagSessionPublicKey) != null &&
						pack.Pool.Get(SecretTagPool.tagSessionPrivateKey) != null &&
						pack.Pool.Get(SecretTagPool.tagSessionSecret) != null) {
					byte[] cli_ident = pack.Pool.Get(SecretTagPool.tagClientIdent);
					byte[] svc_ident = pack.Pool.Get(SecretTagPool.tagSvcIdent);
					final int cliidlen = SLib.GetLen(cli_ident);
					final int svcidlen = SLib.GetLen(svc_ident);
					if((cliidlen > 0 || svcidlen > 0) && (cliidlen * svcidlen) == 0 && cliidlen == 20 || svcidlen == 20) {
						if(cliidlen > 0) {
							// должна существовать запись соответствующего клиента
							correspond_pack = SearchGlobalIdentEntry(SecStoragePacket.kClient, cli_ident);
							correspond_type = SecStoragePacket.kClient;
						}
						else if(svcidlen > 0) {
							// должна существовать запись соответствующего сервиса
							correspond_pack = SearchGlobalIdentEntry(SecStoragePacket.kForeignService, svc_ident);
							correspond_type = SecStoragePacket.kForeignService;
						}
					}
					else {
						// @error Не должно быть так, что заданы одновременно и клиентский и сервисных идентификаторы
					}
					if(correspond_pack != null) {
						correspond_id = correspond_pack.Rec.ID;
						if(correspond_pack.Rec.Kind == correspond_type) { // Корреспондирующий пакет определенного вида должен быть обязательно
							byte[] other_sess_public = pack.Pool.Get(SecretTagPool.tagSessionPublicKeyOther);
							if(other_sess_public != null) {
								MessageDigest digest = MessageDigest.getInstance("SHA-1");
								digest.update(other_sess_public);
								byte[] bi = digest.digest();
								pack.Rec.Kind = SecStoragePacket.kSession;
								pack.Rec.ID = 0;
								pack.Rec.CorrespondID = correspond_pack.Rec.ID;
								pack.Rec.BI = bi;
								if(id == 0) {
									// create session
									long new_id = 0;
									if(correspond_pack.Rec.CorrespondID != 0) {
										SecStoragePacket prev_sess_pack = GetPeerEntry(correspond_pack.Rec.CorrespondID);
										if(prev_sess_pack != null) {
											if(prev_sess_pack.Rec.Kind != StyloQDatabase.SecStoragePacket.kSession) {
												// @problem Нарушена логическая непротиворечивость данных - основная запись ссылается через CorrespondID
												// на запись, не являющуюся сессией.
												// Тем не менее, мы удалим ту запись, поскольку будем менять ссылку correspond_pack.Rec.CorrespondID
											}
											correspond_pack.Rec.CorrespondID = PutPeerEntry(correspond_pack.Rec.CorrespondID, null, false);
										}
									}
									new_id = PutPeerEntry(new_id, pack, false);
									correspond_pack.Rec.CorrespondID = new_id;
									assert (correspond_id > 0);
									correspond_id = PutPeerEntry(correspond_id, correspond_pack, false);
									result_id = new_id;
								}
								else {
									// update session by id
									org_pack = GetPeerEntry(id);
									//
									// Проверка идентичности некоторых параметров изменяемой записи
									//
									if(org_pack.Rec.Kind == pack.Rec.Kind && org_pack.Rec.CorrespondID == correspond_id) {
										byte[] temp_chunk = null;
										if(cliidlen > 0) {
											temp_chunk = org_pack.Pool.Get(SecretTagPool.tagClientIdent);
											if(temp_chunk == null || !SLib.AreByteArraysEqual(temp_chunk, cli_ident)) {
												; // @error
											}
										}
										else if(svcidlen > 0) {
											temp_chunk = org_pack.Pool.Get(SecretTagPool.tagSvcIdent);
											if(temp_chunk != null || !SLib.AreByteArraysEqual(temp_chunk, svc_ident)) {
												; // @error
											}
										}
										pack.Rec.ID = org_pack.Rec.ID;
										result_id = PutPeerEntry(id, pack, false);
									}
									else {
										// @error
									}

								}
							}
						}
						else {
							// @error
						}
					}
				}
			}
			else { // pack == null
				if(id > 0) { // @error invalid arguments
					// remove session by id
					org_pack = GetPeerEntry(id);
					correspond_id = org_pack.Rec.CorrespondID;
					if(correspond_id > 0) {
						correspond_pack = GetPeerEntry(correspond_id);
						if(correspond_pack != null) {
							if(correspond_pack.Rec.CorrespondID == id) {
								// Обнуляем ссылку на удаляемую запись в коррерспондирующем пакете
								correspond_pack.Rec.CorrespondID = 0;
								PutPeerEntry(correspond_id, correspond_pack, false);
							}
							else {
								// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
							}
						}
						else {
							// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
						}
					}
					PutPeerEntry(id, null, false);
				}
			}
			tra.Commit();
		} catch(NoSuchAlgorithmException exn) {
			throw new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		}
		return result_id;
	}
	public NotificationStoragePacket GetNotificationEntry(long id) throws StyloQException
	{
		NotificationStoragePacket result = null;
		Database.Table tbl = CreateTable("NotificationTable2");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT * FROM " + tn + " WHERE id=" + id;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				NotificationTable2.Rec rec = new NotificationTable2.Rec();
				rec.Init();
				rec.Set(cur);
				result = new NotificationStoragePacket(rec);
				if(result.Pool == null) {
					result = null;
				}
			}
		}
		return result;
	}
	public NotificationStoragePacket GetNotificationEntryByIdent(final byte [] ident) throws StyloQException
	{
		NotificationStoragePacket result = null;
		Database.Table tbl = CreateTable("NotificationTable2");
		if(tbl != null && SLib.GetLen(ident) > 0) {
			final String tn = tbl.GetName();
			String query = "SELECT * FROM " + tn + " WHERE ident=x'" + SLib.ByteArrayToHexString(ident) + "'";
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				NotificationTable2.Rec rec = new NotificationTable2.Rec();
				rec.Init();
				rec.Set(cur);
				result = new NotificationStoragePacket(rec);
				if(result.Pool == null) {
					result = null;
				}
			}
		}
		return result;
	}
	public long PutNotificationEntry(long id, NotificationStoragePacket pack, boolean useTa) throws StyloQException
	{
		long result_id = 0;
		Database.Table tbl = CreateTable("NotificationTable2");
		if(tbl != null) {
			if(id == 0) { // insert packet
				if(pack != null) {
					if(pack.PreprocessBeforeStoring()) {
						if(SLib.GetLen(pack.Rec.Ident) > 0) {
							Transaction tra = new Transaction(this, useTa);
							NotificationStoragePacket ex_pack = GetNotificationEntryByIdent(pack.Rec.Ident);
							if(ex_pack != null) {
								// Точно такое извещение уже есть в бд: мы ничего не меняем, просто
								// возвращаем ид существующей записи (с минусом).
								// Извещения генерируются сервисом и могут приходить несколько
								// одинаковых (уникальный идент формирует сервис).
								// Дабы не перегружать базу данных просто ничего не будем делать
								// если пришло точно такое же извещение.
								result_id = -ex_pack.Rec.ID;
							}
							else {
								result_id = tbl.Insert(pack.Rec);
								//if(result_id > 0)
								//	LogEvent(SLib.PPACN_OBJADD, PPOBJ_STYLOQBINDERY, result_id, 0, false);
							}
							tra.Commit();
						}
						else {
							// notification без ident'а нельзя сохранять!
							// @todo @err
						}
					}
				}
			}
			else { // update packet
				NotificationStoragePacket ex_pack = GetNotificationEntry(id);
				if(ex_pack != null) {
					if(pack == null) {
						Transaction tra = new Transaction(this, useTa);
						if(RemoveRec(tbl, id) > 0) {
							//LogEvent(SLib.PPACN_OBJRMV, PPOBJ_STYLOQBINDERY, id, 0, false);
							if(tra.Commit())
								result_id = id;
						}
						else
							tra.Abort();
					}
					else {
						if(pack.PreprocessBeforeStoring()) {
							if(SLib.AreByteArraysEqual(pack.Rec.Ident, ex_pack.Rec.Ident)) {
								Transaction tra = new Transaction(this, useTa);
								if(UpdateRec(tbl, id, pack.Rec) > 0) {
									//LogEvent(SLib.PPACN_OBJUPD, PPOBJ_STYLOQBINDERY, id, 0, false);
									if(tra.Commit())
										result_id = id;
								}
								else
									tra.Abort();
							}
							else {
								// Что-то не так! Клиент просит изменить пакет извещения с заданным
								// базовым идентификатором (Rec.ID), но при этом хэш (Rec.Ident)
								// нового извещения не совпадает с таковым в существующей записи.
								// @todo @err
							}
						}
					}
				}
			}
		}
		return result_id;
	}
	// @test-table
	public static class TestTable extends Table {
		public final static String TBL_NAME = TestTable.class.getSimpleName();
		public static final String CREATE_SQL = "CREATE TABLE IF NOT EXISTS " + TBL_NAME + " (" +
				"ID INTEGER PRIMARY KEY," +
				"I32F INTEGER(4)," +
				"I64F INTEGER(8)," +
				"DTF  INTEGER(8)," + // DateTime
				"FixLenBlobF BLOB," +
				"FixLenTextF TEXT," +
				"VarLenTextF TEXT," +
				"VarLenBlobF BLOB);" +
				"CREATE UNIQUE INDEX idxSecKey0 ON Sec (ID);" +
				"CREATE INDEX idxSecKey1 ON Sec (I32F);" +
				"CREATE INDEX idxSecKey2 ON Sec (I64F);" +
				"CREATE INDEX idxSecKey3 ON Sec (FixLenBlobF);" +
				"CREATE INDEX idxSecKey4 ON Sec (FixLenTextF);" +
				"CREATE INDEX idxSecKey5 ON Sec (VarLenTextF);" +
				"CREATE INDEX idxSecKey5 ON Sec (DTF);";
		static class Rec extends Record {
			public long ID; // @INT(8)
			public int  I32F; // @INT(4)
			public long I64F;
			public long DTF;
			public byte [] FixLenBlobF; // @BLOB
			public String FixLenTextF;
			public String VarLenTextF;
			public byte [] VarLenBlobF;
			public Rec() throws StyloQException
			{
				super();
			}
		}
		public TestTable()
		{
			super();
		}
		public TestTable(Context ctx)
		{
			super(ctx, TBL_NAME);
		}
	}
	public static class SysJournalTable extends Table {
		public final static String TBL_NAME = SysJournalTable.class.getSimpleName();
		public static final String CREATE_SQL = "CREATE TABLE IF NOT EXISTS " + TBL_NAME + " (" +
			"ID INTEGER PRIMARY KEY," +
			"TimeStamp INTEGER(8)," +
			"Action INTEGER(4)," +
			"ObjType INTEGER(4)," +
			"ObjID INTEGER(8)," +
			"Extra INTEGER(8));" +
			"CREATE UNIQUE INDEX idxSysJournalKey0 ON " + TBL_NAME + " (ID);" +
			"CREATE UNIQUE INDEX idxSysJournalKey1 ON " + TBL_NAME + " (TimeStamp);" +
			"CREATE UNIQUE INDEX idxSysJournalKey2 ON " + TBL_NAME + " (ObjType, ObjID, TimeStamp);";
		static class Rec extends Record {
			public long ID;
			public long TimeStamp;
			public int  Action;
			public int  ObjType;
			public long ObjID;
			public long Extra;
			public Rec() throws StyloQException
			{
				super();
			}
		}
		public SysJournalTable()
		{
			super();
		}
		public SysJournalTable(Context ctx)
		{
			super(ctx, TBL_NAME);
		}
	}
	public static class SecTable extends Table {
		public final static String TBL_NAME = SecTable.class.getSimpleName();
		public static final String CREATE_SQL = "CREATE TABLE IF NOT EXISTS " + TBL_NAME + " (" +
			"ID INTEGER PRIMARY KEY," +
			"Kind INTEGER(4)," +
			"CorrespondID INTEGER(8)," +
			"BI BLOB," +
			"Expiration INTEGER(8)," +
			"DocType INTEGER(4)," +
			"TimeStamp INTEGER(8)," +
			"Flags INTEGER(4)," +  // @v11.2.10
			"Counter INTEGER(8)," + // @v11.2.10
			"VT BLOB);" +
			"CREATE UNIQUE INDEX idxSecKey0 ON " + TBL_NAME + " (ID);" +
			"CREATE UNIQUE INDEX idxSecKey1 ON " + TBL_NAME + " (Kind, BI, TimeStamp);" +
			"CREATE UNIQUE INDEX idxSecKey3 ON " + TBL_NAME + " (CorrespondID, BI) where (CorrespondID > 0);" +
			"CREATE UNIQUE INDEX idxSecKey4 ON " + TBL_NAME + " (DocType, BI, TimeStamp) where (DocType > 0);";
		static class Rec extends Record {
			public long   ID;
			public int    Kind;
			public long   CorrespondID;
			public byte [] BI;
			public long   Expiration; // epoch time, seconds
			public int    DocType; // Тип документа (for oneof2(Kind, 6, 7))
			public long   TimeStamp;
			public int    Flags;   // @v11.2.10
			public long   Counter; // @v11.2.10 Для записей вида kCounter
			public byte [] VT;
			public Rec() throws StyloQException
			{
				super();
			}
			static int GetDocStatus(int flags) { return ((flags & SecStoragePacket.styloqfDocStatusFlags) >> 1); }
			int GetDocStatus()
			{
				return SecStoragePacket.IsDocKind(Kind) ? GetDocStatus(Flags) : 0;
			}
			boolean SetDocStatus(int styloqDocStatus)
			{
				boolean ok = true;
				try {
					THROW(SecStoragePacket.IsDocKind(Kind), 0);
					THROW(((styloqDocStatus << 1) & ~SecStoragePacket.styloqfDocStatusFlags) == 0, 0); // Проверяем чтоб за пределами битовой зоны статусов ничего не было.
					// @v11.8.0 {
					// Предотвращаем установку статуса draft или undef если документ имеет уже сформированный "боевой" статус
					boolean done = false;
					int preserve_status = GetDocStatus();
					if((styloqDocStatus == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT || styloqDocStatus == StyloQDatabase.SecStoragePacket.styloqdocstUNDEF) &&
							preserve_status > StyloQDatabase.SecStoragePacket.styloqdocstDRAFT) {
						done = true;
					}
					// } @v11.8.0
					if(!done) {
						Flags &= ~SecStoragePacket.styloqfDocStatusFlags;
						Flags |= ((styloqDocStatus << 1) & SecStoragePacket.styloqfDocStatusFlags);
					}
				} catch(StyloQException exn) {
					ok = false;
				}
				return ok;
			}
		}
		public SecTable()
		{
			super();
		}
		public SecTable(Context ctx)
		{
			super(ctx, TBL_NAME);
		}
	}
	//
	// Descr: Ошибочная версия таблицы уведомлений (без поля Ident). Заменена на NotificationTable2
	//
	public static class NotificationTable extends Table {
		public final static String TBL_NAME = NotificationTable.class.getSimpleName();
		public static final String CREATE_SQL = "CREATE TABLE IF NOT EXISTS " + TBL_NAME + " (" +
			"ID INTEGER PRIMARY KEY," +
			"SvcID INTEGER(8)," + // ->SecTable.ID
			"Expiration INTEGER(8)," +
			"TimeStamp INTEGER(8)," +
			"Flags INTEGER(4)," +
			"Counter INTEGER(8)," +
			"VT BLOB);" +
			"CREATE UNIQUE INDEX idxSecKey0 ON " + TBL_NAME + " (ID);" +
			"CREATE UNIQUE INDEX idxSecKey1 ON " + TBL_NAME + " (TimeStamp);" +
			"CREATE UNIQUE INDEX idxSecKey3 ON " + TBL_NAME + " (SvcID, TimeStamp) where (SvcID > 0);";
		static class Rec extends Record {
			public long   ID;
			public long   SvcID;
			public long   Expiration; // epoch time, seconds
			public long   TimeStamp;
			public int    Flags;   // @v11.2.10
			public long   Counter; // @v11.2.10 Для записей вида kCounter
			public byte [] VT;
			public Rec() throws StyloQException
			{
				super();
			}
		}
		public NotificationTable()
		{
			super();
		}
		public NotificationTable(Context ctx)
		{
			super(ctx, TBL_NAME);
		}
	}
	//
	// @v11.7.5
	// Descr: Определение таблицы уведомлений.
	//   До версии 11.7.5 уведомления хранились в общей таблице SecTable, однако
	//   из-за значительного числа уведомлений и неадаптированности индексов
	//   возникали тяжелые задержки в работе приложения. В связи с чем решено
	//   уведомления переместить в отдельную таблицу.
	// @v11.7.6 Note: Я, дурень такой, забыл включить в таблицу важнейшее поле Ident.
	//   из-за этого события сплош дублируются.
	//
	public static class NotificationTable2 extends Table {
		public final static String TBL_NAME = NotificationTable2.class.getSimpleName();
		public static final String CREATE_SQL = "CREATE TABLE IF NOT EXISTS " + TBL_NAME + " (" +
			"ID INTEGER PRIMARY KEY," +
			"Ident BLOB," + // @v11.7.6 Композитный идентификатор события, инициируемый сервисом
			"SvcID INTEGER(8)," + // ->SecTable.ID
			"Expiration INTEGER(8)," +
			"TimeStamp INTEGER(8)," +
			"Flags INTEGER(4)," +
			"Counter INTEGER(8)," +
			"VT BLOB);" +
			"CREATE UNIQUE INDEX idxSecKey0 ON " + TBL_NAME + " (ID);" +
			"CREATE UNIQUE INDEX idxSecKey1 ON " + TBL_NAME + " (TimeStamp);" +
			"CREATE UNIQUE INDEX idxSecKey3 ON " + TBL_NAME + " (SvcID, TimeStamp) where (SvcID > 0);" +
			"CREATE UNIQUE INDEX idxSecKey4 ON " + TBL_NAME + " (Ident);"; // @v11.7.6
		static class Rec extends Record {
			public long   ID;
			public byte [] Ident; // @v11.7.6
			public long   SvcID;
			public long   Expiration; // epoch time, seconds
			public long   TimeStamp;
			public int    Flags;   // @v11.2.10
			public long   Counter; // @v11.2.10 Для записей вида kCounter
			public byte [] VT;
			public Rec() throws StyloQException
			{
				super();
			}
		}
		public NotificationTable2()
		{
			super();
		}
		public NotificationTable2(Context ctx)
		{
			super(ctx, TBL_NAME);
		}
	}
	int Upgrade(int curVer, int prevVer) throws StyloQException
	{
		int  ok = -1;
		try {
			if(curVer >= 92/*debug*/ && prevVer < 92/*debug*/) {
				if(prevVer >= 90)
					DropTable("NotificationTable");
			}
			if(curVer >= 92/*debug*/ && prevVer < 90/*debug*/) {
				final String s_tn = "SecTable";
				final String n_tn = "NotificationTable2";
				Database.Table t_sec = CreateTable("SecTable");
				Database.Table t_n = CreateTable("NotificationTable2");
				ArrayList <StyloQInterchange.SvcNotification> nlist = Helper_GetNotifivationList_before90v(0, null, false, false);
				if(SLib.GetCount(nlist) > 0) {
					Transaction tra = new Transaction(this, true);
					for(StyloQInterchange.SvcNotification nitem : nlist) {
						StoreNotification_new(nitem, false);
						PutPeerEntry(nitem.InternalID, null, false);
					}
					tra.Commit();
				}
			}
			if(curVer == 2 && prevVer <= 2) {
				// alter table SecTable
				//sqlite_master
				//sqlite_schema
				{
					final String tn = "SecTable";
					String query = "SELECT sql FROM sqlite_master WHERE name = '" + tn + "';";
					android.database.Cursor cur = GetHandle().rawQuery(query, null);
					if(cur != null && cur.moveToFirst()) {
						boolean fld_exists_flags = false;
						boolean fld_exists_counter = false;
						boolean fld_exists_expiration = false; // @debug
						/*
							CREATE TABLE sqlite_schema(
							  type text,
							  name text,
							  tbl_name text,
							  rootpage integer,
							  sql text
							);
						 */
						int ccount = cur.getColumnCount();
						for(int cidx = 0; cidx < ccount; cidx++) {
							String cn = cur.getColumnName(cidx);
							if(cn.equalsIgnoreCase("sql")) {
								String sqls = cur.getString(cidx);
								if(sqls.indexOf("Flags", 0) >= 0) {
									fld_exists_flags = true;
								}
								if(sqls.indexOf("Counter", 0) >= 0) {
									fld_exists_counter = true;
								}
								if(sqls.indexOf("Expiration", 0) >= 0) { // @debug
									fld_exists_expiration = true;
								}
							}
						}
						if(!fld_exists_flags || !fld_exists_counter) {
							if(!fld_exists_flags) {
								GetHandle().execSQL("alter table " + tn + " add column Flags INTEGER(4);");
							}
							if(!fld_exists_counter) {
								GetHandle().execSQL("alter table " + tn + " add column Counter INTEGER(8);");
							}
							ok = 1;
						}
					}
				}
			}
		} catch(SQLException exn) {
			ok = 0;
			throw new StyloQException(ppstr2.PPERR_JEXN_SQL, exn.getMessage());
		}
		return ok;
	}
}
