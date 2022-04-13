// SecretTagPool.java
// Copyright (c) A.Sobolev 2021
//
package ru.petroglif.styloq;

import static ru.petroglif.styloq.SLib.THROW_SL;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.DataFormatException;
import java.util.zip.Deflater;
import java.util.zip.Inflater;

public class SecretTagPool {
	public final static int tagSessionPrivateKey       =  1;  // Приватный ключ сессии обмена данными
	public final static int tagSessionPublicKey  	   =  2;  // Публичный ключ сессии обмена данными
	public final static int tagSessionSecret     	   =  3;  // Секрет, сгенерированный после обмена публичными ключами с контрагентом
	public final static int tagSvcAppellation    	   =  4;  //
	public final static int tagSvcIdent          	   =  5;  //
	public final static int tagSecret            	   =  7;  //
	public final static int tagSvcAccessPoint    	   =  8;  // URL
	public final static int tagSvcCommand        	   =  9;  // Заголовок сервисной команды
	public final static int tagSvcCommandParam   	   = 10;  // Параметры сервисной команды
	public final static int tagSvcCapabities     	   = 11;  // Опции возможностей сервиса
	public final static int tagClientIdent       	   = 12;  // Идентификатор клиента
	public final static int tagSelfyFace               = 13;  // Собственный лик (json. see StyloQFace)
	public final static int tagSrpVerifierSalt   	   = 14;  //
	public final static int tagSrpVerifier       	   = 15;  //
	public final static int tagSrpA              	   = 16;  // user-->host tagPublicIdent:tagSrpA
	public final static int tagSrpB                    = 17;  // host-->user tagSrpVerifierSalt:tagSrpB
	public final static int tagSrpM                    = 18;  // user-->host tagSrpM
	public final static int tagSrpHAMK                 = 19;  // host-->user tagSrpHAMK
	public final static int tagPrimaryRN               = 20;  // Первичное очень большое случайное число, используемое для генерации уникальных для данной инсталляции величин
	public final static int tagAG                      = 21;  // Дополнительное значение, используемое при генерации публичных идентификаторов
	public final static int tagFPI                     = 22;  // Фейковый публичный сервисный идентификатор, для генерации собственного публичного идентификатора
	public final static int tagRawData                 = 23;  // "Сырые" данные, прикрепленные к пакету (для передачи по сети)
	public final static int tagReplyStatus             = 24;  // int32   Статус ответа. 0 - OK, !0 - error_code
	public final static int tagReplyStatusText         = 25;  // zstring Текстовое описание статуса ответа. Если tagReplyStatus == 0, то текста может не быть либо это какое-либо
		// приветствие или иной малозначительный текст.
	public final static int tagSessionExpirPeriodSec   = 26;  // uint32  Период истечения срока действия сессии в секундах.
	public final static int tagSessionPublicKeyOther   = 27;  // Публичный ключ сессии обмена данными противоположной стороны (в случае, если необходимо хранить
		// в одном пуле и свой и чужой публичные ключи, свой идентифицируется как tagSessionPublicKey, чужой - tagSessionPublicKeyOther
	public final static int tagFace             	   = 28;  // Параметры представления контрагента (json. see StyloQFace). Собственный лик хранится по тегу tagSelfyFace
	public final static int tagConfig                  = 29;  // json Конфигурация
	public final static int tagPrivateConfig           = 30;  // @reserve json Приватная конфигурация //
	public final static int tagRoundTripIdent          = 31;  // @v11.2.0 Идентификатор сеанса обмена. Генерируется на стороне инициатора обмена (обычно, клиент)
		// и применяется для сопоставления запросов потокам при асинхронном режиме. Полагаю, в большинстве случаев это будет просто GUID
	public final static int tagDocDeclaration          = 32;  // @v11.2.0 JSON-декларация документа, включенного с тегом tagRawData
	public final static int tagSvcLoclAddendum         = 33;  // @v11.2.3 Дополнительный идентификатор сервиса, определяющий локальный характер обслуживаемого запроса.
		// Участвует в формировании символа очереди в случае MQB-обмена.
	public final static int tagAssignedFaceRef         = 34;  // @v11.2.4 Ссылка на собственный лик, ассоциированная с записью сервиса. Используется в StyloQ
		// на стороне клиента в записи сервиса для того, чтобы сообщить сервису приемлемый вариант лика.
	public final static int tagErrorCode               = 35;  // @v11.2.10 int32 Код ошибки (ответ)
	public final static int tagBlob                    = 36;  // @v11.3.8  BLOB передаваемый с пакетом. По тегу tagRawData хранятся мета-данные этого blob'а в json-формате
	//
	static final int MAGIC = 0x5E4F7D1A;
	static final int MAX_ARRAY_SIZE = Integer.MAX_VALUE - 8;
	static class H { // Заголовок всего пула
		static int SizeOf() { return (4 + 4); }
		H(int f)
		{
			Magic = MAGIC;
			Flags = f;
		}
		H(final byte [] buffer)
		{
			Magic = 0;
			if(buffer.length >= SizeOf()) {
				Magic = (buffer[0] & 0xff) | ((buffer[1] & 0xff) << 8) | ((buffer[2] & 0xff) << 16) | ((buffer[3] & 0xff) << 24);
				Flags = (buffer[4] & 0xff) | ((buffer[5] & 0xff) << 8) | ((buffer[6] & 0xff) << 16) | ((buffer[7] & 0xff) << 24);
			}
			if(Magic != MAGIC) {
				Magic = 0;
				Flags = 0;
			}
		}
		byte [] toByteArray()
		{
			return new byte[] {
				(byte)(Magic & 0xff), (byte)((Magic >> 8) & 0xff), (byte)((Magic >> 16) & 0xff), (byte)((Magic >> 24) & 0xff),
				(byte)(Flags & 0xff), (byte)((Flags >> 8) & 0xff), (byte)((Flags >> 16) & 0xff), (byte)((Flags >> 24) & 0xff),
			};
		}
		int Magic;
		int Flags;
	};
	static class BH { // Заголовок элемента данных
		static int SizeOf() { return (4 + 4); }
		BH(int i, int s)
		{
			I = i;
			S = s;
		}
		BH(final byte [] buffer)
		{
			if(buffer.length >= SizeOf()) {
				I = (buffer[0] & 0xff) | ((buffer[1] & 0xff) << 8) | ((buffer[2] & 0xff) << 16) | ((buffer[3] & 0xff) << 24);
				S = (buffer[4] & 0xff) | ((buffer[5] & 0xff) << 8) | ((buffer[6] & 0xff) << 16) | ((buffer[7] & 0xff) << 24);
			}
			else {
				I = 0;
				S = 0;
			}
		}
		byte [] toByteArray()
		{
			return new byte[] {
				(byte)((I >> 0) & 0xff), (byte)((I >> 8) & 0xff), (byte)((I >> 16) & 0xff), (byte)((I >> 24) & 0xff),
				(byte)((S >> 0) & 0xff), (byte)((S >> 8) & 0xff), (byte)((S >> 16) & 0xff), (byte)((S >> 24) & 0xff),
			};
		}
		int I;
		int S;
	};
	class Entry {
		Entry(int id, byte [] data)
		{
			I = id;
			Data = data;
		}
		int   I;
		byte [] Data;
	}
	List<Entry> L;
	SecretTagPool()
	{
		L = new ArrayList<Entry>();
	}
	SecretTagPool Z()
	{
		L.clear();
		return this;
	}
	//
	// Descr: Дескриптор стратегии сжатия больших отрезков при внесении в пул.
	//   На начальном этапе применяется только алгоритм сжатия zlib.
	//   Если размер отрезка, добавляемого в пул равен или превышает MinChunkSizeToCompress,
	//   то отрезок сжимается и впереди добавляется сигнатура Ssc_CompressionSignature
	//
	public static class DeflateStrategy {
		DeflateStrategy()
		{
			MinChunkSizeToCompress = 0;
			CompressMethod = 0;
		}
		DeflateStrategy(int minChunkSize)
		{
			MinChunkSizeToCompress = minChunkSize;
			CompressMethod = 0;
		}
		int MinChunkSizeToCompress;
		int CompressMethod; // Reserved.
	};

	int Put(int id, final byte [] chunk, DeflateStrategy ds)
	{
		int result = 0;
		try {
			if(ds != null && chunk.length >= ds.MinChunkSizeToCompress) {
				// SLib.Ssc_CompressionSignature
				ByteArrayOutputStream baos = new ByteArrayOutputStream();
				Deflater compresser = new Deflater();
				compresser.setInput(chunk);
				compresser.finish();
				byte[] output = new byte[chunk.length];
				int compressed_size = compresser.deflate(output);
				{
					byte[] prefix_bytes = SLib.LongToBytes(SLib.Ssc_CompressionSignature);
					baos.write(prefix_bytes);
				}
				if(compressed_size > 0)
					baos.write(output, 0, compressed_size);
				compresser.end();
				result = Put(id, baos.toByteArray());
			}
			else
				result = Put(id, chunk);
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			result = 0;
		}
		return result;
	}
	int Put(int id, final byte [] chunk)
	{
		int    ok = -1;
		if(id > 0) {
			boolean do_remove = (chunk == null || chunk.length == 0);
			for(int i = 0; i < L.size(); i++) {
				if(L.get(i).I == id) {
					if(do_remove) {
						L.remove(i);
						ok = 1;
					}
					else {
						L.get(i).Data = chunk;
						ok = 1;
					}
					break;
				}
			}
			if(ok <= 0 && !do_remove) {
				L.add(new Entry(id, chunk));
				ok = 1;
			}
		}
		return ok;
	}
	JSONObject GetJsonObject(int id)
	{
		JSONObject result = null;
		byte [] raw_data = Get(id);
		if(SLib.GetLen(raw_data) > 0) {
			String js_text = new String(raw_data);
			try {
				result = new JSONObject(js_text);
			} catch(JSONException exn) {
				result = null;
			}
		}
		return result;
	}
	byte [] Get(int id)
	{
		byte [] result = null;
		try {
			if(id > 0) {
				for(int i = 0; i < L.size(); i++) {
					if(L.get(i).I == id) {
						byte[] data = L.get(i).Data;
						if(data.length >= Long.BYTES && SLib.BytesToLong(data, 0) == SLib.Ssc_CompressionSignature) {
							Inflater decompresser = new Inflater();
							decompresser.setInput(data, Long.BYTES, data.length - Long.BYTES);
							byte[] temp_buf = new byte[1024*8];
							int result_length = 0;
							ByteArrayOutputStream baos = new ByteArrayOutputStream();
							while(!decompresser.finished()) {
								result_length = decompresser.inflate(temp_buf);
								if(result_length > 0)
									baos.write(temp_buf, 0, result_length);
							}
							decompresser.end();
							result = baos.toByteArray();
						}
						else
							result = data;
					}
				}
			}
		} catch(DataFormatException exn) {
			result = null;
		}
		return result;
	}
	int CopyFrom(final SecretTagPool srcPool, final int [] idList, boolean errOnAbsenseAny) throws StyloQException
	{
		int ok = -1;
		if(idList.length > 0) {
			if(errOnAbsenseAny) {
				//
				// Требование наличия всех идентификаторов проверяем предварительно дабы не ломать содержимое this в случае ошибки
				//
				for(int i = 0; i < idList.length; i++) {
					final int id = idList[i];
					THROW_SL(srcPool.Get(id) != null, SLib.SLERR_BINSET_SRCIDNFOUND);
				}
			}
			{
				for(int i = 0; ok != 0 && i < idList.length; i++) {
					final int id = idList[i];
					byte [] temp_bch = srcPool.Get(id);
					if(temp_bch != null) {
						if(Put(id, temp_bch) > 0)
							ok = 1;
					}
				}
			}
		}
		return ok;
	}
	byte [] Serialize() throws StyloQException
	{
		byte [] result = null;
		try {
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			H hdr = new H(0);
			baos.write(hdr.toByteArray());
			for(int i = 0; i < L.size(); i++) {
				final Entry entry = L.get(i);
				BH bh = new BH(entry.I, entry.Data.length);
				baos.write(bh.toByteArray());
				baos.write(entry.Data);
			}
			result = baos.toByteArray();
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return result;
	}
	public void Unserialize(final byte [] buffer) throws StyloQException
	{
		int offs = 0;
		L.clear();
		ByteArrayInputStream bais = new ByteArrayInputStream(buffer);
		THROW_SL(bais.available() >= H.SizeOf(), SLib.SLERR_SBUFRDSIZE);
		byte [] temp_buf = new byte[H.SizeOf()];
		int real_size = bais.read(temp_buf, 0, temp_buf.length);
		THROW_SL(real_size == temp_buf.length, SLib.SLERR_SBUFRDSIZE);
		offs += real_size;
		H hdr = new H(buffer);
		THROW_SL(hdr.Magic == MAGIC, SLib.SLERR_BINSET_UNSRLZ_SIGNATURE);
		while(bais.available() >= BH.SizeOf()) {
			temp_buf = new byte[BH.SizeOf()];
			real_size = bais.read(temp_buf, 0, temp_buf.length);
			THROW_SL(real_size == temp_buf.length, SLib.SLERR_SBUFRDSIZE);
			offs += real_size;
			BH bh = new BH(temp_buf);
			THROW_SL(bh.I > 0 && bh.S > 0, SLib.SLERR_BINSET_UNSRLZ_ITEMHDR);
			temp_buf = new byte[bh.S];
			real_size = bais.read(temp_buf, 0, temp_buf.length);
			THROW_SL(real_size == temp_buf.length, SLib.SLERR_BINSET_UNSRLZ_ITEMHDR);
			Put(bh.I, temp_buf);
			offs += real_size;
		}
	}
}
