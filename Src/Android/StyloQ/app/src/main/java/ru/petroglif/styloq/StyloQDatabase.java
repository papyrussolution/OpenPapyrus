// StyloQDatabase.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import static ru.petroglif.styloq.SLib.THROW;
import android.content.Context;
import android.database.SQLException;
import android.os.Build;
import androidx.annotation.RequiresApi;
import org.checkerframework.checker.nullness.qual.NonNull;
import org.json.JSONException;
import org.json.JSONObject;
import java.math.BigInteger;
import java.nio.ByteBuffer;
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
		//
		//
		public static final int kUndef          = 0;
		public static final int kNativeService  = 1; // Собственная идентификация. Используется для любого узла, включая клиентские, которые никогда не будут сервисами //
		public static final int kForeignService = 2;
		public static final int kClient         = 3;
		public static final int kSession        = 4;
		public static final int kFace           = 5; // Параметры лика, которые могут быть переданы серверу для ассоциации с нашим клиентским аккаунтом
		public static final int kDocIncoming    = 6; // Входящие документы
		public static final int kDocOutcominig  = 7; // Исходящие документы
		public static final int kCounter        = 8; // @v11.2.10 Специальная единственная запись для хранения текущего счетчика (документов и т.д.)
		//
		// Descr: Флаги записи таблицы данных StyloQ bindery (StyloQSec::Flags)
		//
		public static final int styloqfMediator = 0x0001; // Запись соответствует kForeignService-медиатору. Флаг устанавливается/снимается при создании или обновлении
			// записи после получения соответствующей информации от сервиса-медиатора
		//
		public static final int doctypUndef       = 0;
		public static final int doctypCommandList = 1;
		public static final int doctypOrderPrereq = 2; // Предопределенный формат данных, подготовленных для формирования заказа на клиентской стороне
		public static final int doctypReport      = 3; // @v11.2.10 Отчеты в формате DL600 export
		public static final int doctypGeneric     = 4; // @v11.2.11 Общий тип для документов, чьи характеристики определяются видом операции (что-то вроде Bill в Papyrus'е)
		public static final int doctypIndexingContent = 5;  // @v11.3.4 Документ, содержащий данные для индексации медиатором
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
				byte [] rawdata = Pool.Get(SecretTagPool.tagRawData);
				if(SLib.GetLen(rawdata) > 0) {
					String json_tex = new String(rawdata);
					if(SLib.GetLen(json_tex) > 0)
						result = StyloQCommand.FromJson(json_tex);
				}
			}
			return result;
		}
	}
	@RequiresApi(api = Build.VERSION_CODES.O)
	long SetupPeerInstance() throws StyloQException
	{
		long  id = 0;
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
					id = PutPeerEntry(0, pack);
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
		try {
			Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
			if(tbl != null) {
				final String tn = tbl.GetName();
				String query = "SELECT * FROM " + tn + " WHERE Action=" + action;
				if(extraVal > 0)
					query = query + " and Extra="+extraVal;
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
		return result;
	}
	boolean LogEvent(int action, int obj, long id, long extData, int use_ta)
	{
		boolean ok = false;
		long result_id = 0;
		SysJournalTable.Rec rec = null;
		Database.Table tbl = CreateTable(SysJournalTable.TBL_NAME);
		if(tbl != null) {
			try {
				rec = new SysJournalTable.Rec();
				rec.TimeStamp = System.currentTimeMillis();
				rec.Action = action;
				rec.ObjType = obj;
				rec.ObjID = id;
				rec.Extra = extData;
				StartTransaction();
				result_id = tbl.Insert(rec);
				if(result_id > 0) {
					CommitWork();
					ok = true;
				}
				else
					RollbackWork();
			} catch(StyloQException exn) {
				//exn.printStackTrace();
			}
		}
		return ok;
	}
	public long PutPeerEntry(long id, SecStoragePacket pack) throws StyloQException
	{
		long result_id = 0;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			if(id == 0) { // insert packet
				if(pack.PreprocessBeforeStoring(null)) {
					StartTransaction();
					result_id = tbl.Insert(pack.Rec);
					CommitWork();
				}
			}
			else { // update packet
				SecStoragePacket ex_pack = GetPeerEntry(id);
				if(ex_pack != null) {
					if(pack == null) {
						StartTransaction();
						if(RemoveRec(tbl, id) > 0) {
							CommitWork();
							result_id = id;
						}
						else
							RollbackWork();
					}
					else {
						if(pack.PreprocessBeforeStoring(ex_pack)) {
							StartTransaction();
							if(UpdateRec(tbl, id, pack.Rec) > 0) {
								CommitWork();
								result_id = id;
							}
							else
								RollbackWork();
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
				try {
					final String tn = tbl.GetName();
					StartTransaction();
					//public static final int kSession        = 4;
					//public static final int kDocIncoming    = 6; // Входящие документы
					//public static final int kDocOutcominig  = 7; // Исходящие документы
					final int[] kind_list_to_delete = {SecStoragePacket.kSession, SecStoragePacket.kDocIncoming, SecStoragePacket.kDocOutcominig};
					for(int i = 0; i < kind_list_to_delete.length; i++) {
						int kind_to_delete = kind_list_to_delete[i];
						String query = "DELETE FROM " + tn + " WHERE CorrespondID=" + id + " and kind=" + kind_to_delete;
						GetHandle().execSQL(query);
					}
					{
						String query = "DELETE FROM " + tn + " WHERE id=" + id;
						GetHandle().execSQL(query);
					}
					CommitWork();
					ok = true;
				} catch(SQLException exn) {
					ok = false;
					RollbackWork();
					new StyloQException(ppstr2.PPERR_JEXN_SQL, exn.getMessage());
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
	public ArrayList<Long> GetForeignSvcIdList(boolean skipDups) throws StyloQException
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
				if(skipDups && inner_list != null) {
					Vector <Long> id_list_to_remove = null;
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
	public String MakeTextHashForForeignService(SecStoragePacket svcPack, int len)
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
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
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
	public long PutDocument(int direction, int docType, byte [] ident, long correspondId, SecretTagPool pool) throws StyloQException
	{
		long   result_id = 0;
		try {
			if(SLib.GetLen(ident) > 0) {
				SecStoragePacket pack = null;
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
				long   doc_time = 0;
				int    doc_expiry = 0;
				JSONObject js_document = null; // generic document inserted into raw_doc
				StyloQCommand.DocDeclaration doc_decl = null;
				byte [] raw_doc = pool.Get(SecretTagPool.tagRawData);
				byte [] raw_doc_decl = pool.Get(SecretTagPool.tagDocDeclaration);
				if(correspondId > 0) {
					SecStoragePacket corr_pack = GetPeerEntry(correspondId);
					THROW(corr_pack != null, ppstr2.PPERR_SQ_CORRESPONDOBJNFOUND, Long.toString(correspondId));
					THROW(corr_pack.Rec.Kind == SecStoragePacket.kClient || corr_pack.Rec.Kind == SecStoragePacket.kForeignService, ppstr2.PPERR_SQ_WRONGDBITEMKIND);
				}
				if(SLib.GetLen(raw_doc_decl) > 0) {
					doc_decl = new StyloQCommand.DocDeclaration();
					if(doc_decl.FromJson(new String(raw_doc_decl))) {
						raw_doc_type = doc_decl.Type;
						doc_time = doc_decl.Time;
						doc_expiry = doc_decl.ResultExpiryTimeSec;
					}
				}
				if(SLib.GetLen(raw_doc) > 0) {
					{
						JSONObject jsobj = new JSONObject(new String(raw_doc));
						if(jsobj != null) {
							if(doc_decl == null) {
								raw_doc_type = jsobj.optString("doctype", "");
								doc_time = jsobj.optLong("time", 0);
								doc_expiry = jsobj.optInt("expir_time_sec", 0);
							}
							js_document = jsobj.optJSONObject("document");
						}
						if(js_document == null)
							js_document = jsobj;
					}
					int pack_kind = 0;
					if(direction > 0)
						pack_kind = SecStoragePacket.kDocOutcominig;
					else if(direction < 0)
						pack_kind = SecStoragePacket.kDocIncoming;
					if(raw_doc_type != null) {
						if(docType == SecStoragePacket.doctypGeneric) {
							if(js_document != null) {
								long _ex_doc_id_from_json = (direction > 0) ? js_document.optLong("ID", 0) : 0;
								long _ex_doc_id_from_db = 0;
								pack = InitDocumentPacket(pack_kind, docType, correspondId, ident, doc_expiry, pool);
								{
									StartTransaction();
									// Документ такого типа может быть только один в комбинации {direction; rIdent}
									ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, correspondId, ident);
									if(ex_id_list != null) {
										for(int i = 0; i < ex_id_list.size(); i++) {
											long local_id = ex_id_list.get(i);
											if(_ex_doc_id_from_json > 0 && local_id == _ex_doc_id_from_json)
												_ex_doc_id_from_db = local_id;
											else
												PutPeerEntry(local_id, null); // @throw
										}
									}
									result_id = PutPeerEntry(_ex_doc_id_from_db, pack);
									CommitWork();
								}
							}
						}
						else if(docType == SecStoragePacket.doctypCommandList && raw_doc_type.equalsIgnoreCase("commandlist")) {
							pack = InitDocumentPacket(pack_kind, docType, correspondId, ident, doc_expiry, pool);
							{
								StartTransaction();
								// Документ такого типа может быть только один в комбинации {direction; rIdent}
								ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, 0, ident);
								if(ex_id_list != null) {
									for(int i = 0; i < ex_id_list.size(); i++) {
										long _id_to_remove = ex_id_list.get(i);
										PutPeerEntry(_id_to_remove, null); // @throw
									}
								}
								result_id = PutPeerEntry(0, pack);
								CommitWork();
							}
						}
						else if(docType == SecStoragePacket.doctypReport && raw_doc_type.equalsIgnoreCase("view")) {
							pack = InitDocumentPacket(pack_kind, docType, correspondId, ident, doc_expiry, pool);
							{
								StartTransaction();
								// Документ такого типа может быть только один в комбинации {direction; rIdent}
								ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, 0, ident);
								if(ex_id_list != null) {
									for(int i = 0; i < ex_id_list.size(); i++) {
										long _id_to_remove = ex_id_list.get(i);
										PutPeerEntry(_id_to_remove, null); // @throw
									}
								}
								result_id = PutPeerEntry(0, pack);
								CommitWork();
							}
						}
						else if(docType == SecStoragePacket.doctypOrderPrereq && raw_doc_type.equalsIgnoreCase("orderprereq")) {
							pack = InitDocumentPacket(pack_kind, docType, correspondId, ident, doc_expiry, pool);
							{
								StartTransaction();
								// Документ такого типа может быть только один в комбинации {direction; rIdent}
								ArrayList<Long> ex_id_list = GetDocIdListByType(direction, docType, 0, ident);
								if(ex_id_list != null) {
									for(int i = 0; i < ex_id_list.size(); i++) {
										long _id_to_remove = ex_id_list.get(i);
										PutPeerEntry(_id_to_remove, null); // @throw
									}
								}
								result_id = PutPeerEntry(0, pack);
								CommitWork();
							}
						}
					}
				}
			}
			//}
		} catch(JSONException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return result_id;
	}
	//
	// ARG(direction IN): <0 - incoming, >0 - outcoming, 0 - no matter
	//
	public ArrayList<Long> GetDocIdListByType(int direction, int docType, long correspondId, byte [] ident) throws StyloQException
	{
		ArrayList<Long> result = null;
		Database.Table tbl = CreateTable("SecTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			String query = "SELECT ID, Kind FROM " + tn + " WHERE docType=" + docType;
			if(SLib.GetLen(ident) > 0) {
				query += " and BI=x'" + SLib.ByteArrayToHexString(ident) + "'";
			}
			if(direction > 0)
				query += " and kind=" + SecStoragePacket.kDocOutcominig;
			else if(direction < 0)
				query += " and kind=" + SecStoragePacket.kDocIncoming;
			else
				query += " and (kind=" + SecStoragePacket.kDocOutcominig + " or kind=" + SecStoragePacket.kDocIncoming + ")";
			if(correspondId > 0)
				query += " and CorrespondID=" + correspondId;
			android.database.Cursor cur = GetHandle().rawQuery(query, null);
			if(cur != null && cur.moveToFirst()) {
				do {
					SecTable.Rec rec = new SecTable.Rec();
					rec.Init();
					rec.Set(cur);
					if(rec.Kind == SecStoragePacket.kDocIncoming || rec.Kind == SecStoragePacket.kDocOutcominig) {
						if(result == null)
							result = new ArrayList<Long>();
						result.add(new Long(rec.ID));
					}
				} while(cur.moveToNext());
			}
		}
		return result;
	}
	public long GetNewCounter()
	{
		// {706FD772-DC3F-452C-93A1-579E01F3E8D1}
		final UUID uid = UUID.fromString("706FD772-DC3F-452C-93A1-579E01F3E8D1");
		long result = 0;
		try {
			Database.Table tbl = CreateTable("SecTable");
			if(tbl != null) {
				StartTransaction();
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
					CommitWork();
				else
					RollbackWork();
			}
		} catch(StyloQException exn) {
			result = 0;
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
	public long StoreSession(long id, SecStoragePacket pack, int use_ta) throws StyloQException
	{
		long result_id = 0;
		try {
			long correspind_id = 0;
			SecStoragePacket org_pack;
			SecStoragePacket correspond_pack = null;
			StartTransaction();
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
						correspind_id = correspond_pack.Rec.ID;
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
											correspond_pack.Rec.CorrespondID = PutPeerEntry(correspond_pack.Rec.CorrespondID, null);
										}
									}
									new_id = PutPeerEntry(new_id, pack);
									correspond_pack.Rec.CorrespondID = new_id;
									assert (correspind_id > 0);
									correspind_id = PutPeerEntry(correspind_id, correspond_pack);
									result_id = new_id;
								}
								else {
									// update session by id
									org_pack = GetPeerEntry(id);
									//
									// Проверка идентичности некоторых параметров изменяемой записи
									//
									if(org_pack.Rec.Kind == pack.Rec.Kind && org_pack.Rec.CorrespondID == correspind_id) {
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
										result_id = PutPeerEntry(id, pack);
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
					correspind_id = org_pack.Rec.CorrespondID;
					if(correspind_id > 0) {
						correspond_pack = GetPeerEntry(correspind_id);
						if(correspond_pack != null) {
							if(correspond_pack.Rec.CorrespondID == id) {
								// Обнуляем ссылку на удаляемую запись в коррерспондирующем пакете
								correspond_pack.Rec.CorrespondID = 0;
								PutPeerEntry(correspind_id, correspond_pack);
							}
							else {
								// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
							}
						}
						else {
							// @problem (логическая целостность таблицы нарушена, но прерывать исполнение нельзя - мы все равно удаляем запись)
						}
					}
					PutPeerEntry(id, null);
				}
			}
			CommitWork();
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
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
	int Upgrade(int curVer, int prevVer)
	{
		int  ok = -1;
		try {
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
			new StyloQException(ppstr2.PPERR_JEXN_SQL, exn.getMessage());
		}
		return ok;
	}
}
