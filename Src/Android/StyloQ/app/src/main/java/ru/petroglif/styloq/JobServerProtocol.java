// JobServerProtocol.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import static ru.petroglif.styloq.SLib.THROW;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.spec.SecretKeySpec;

public class JobServerProtocol {
	public static final byte CurrentProtocolVer = 1;
	//
	// Descr: Типы данных, используемые в поле Header::Type (только для реплик,
	//   команды применяют поле Header::Type для идентификации собственно команды).
	//
	public static final int htGeneric       = 0;
	public static final int htGenericText   = 1;
	public static final int htStrAssocArray = 2;
	public static final int htFile          = 3; // Файл (с предваряющим заголовком TransmitFileBlock)
	//
	// Descr: Флаги заголовка реплики.
	//
	public static final int hfAck		= 0x0001; // Простая реплика, подтверждающая обработку сервером
	// полученной команды.
	public static final int hfRepError	= 0x0002; // Реплика сигнализирует об ошибке.
	// После инициализации чтения реплики функцией StartReading, если
	// реплика сигнализирует об ошибке, то возвращаемая функцией
	// структура заголовка будет содержать этот флаг.
	public static final int hfInformer	= 0x0004; // Реплика информирует клиента о ходе выолнения процесса.
	// Если тип данных равен htGenericText, то после заголовка следует
	// текстовая информация о ходе процесса.
	public static final int hfCmd		= 0x0008; // Пакет является командой (в противном случае - репликой)
	public static final int hfSlString	= 0x0010; // Пакет представлен строкой, завершенной переводом каретки.
	// Это флаг соответствует неструктурированному пакету, по этому
	// в заголовке структурированного ответа не может быть такого типа.
	public static final int hfMore      = 0x0020; // Пакет не полностью передал данные. Требуется дополнительный
	// запрос на продолжение передачи. Пока этот флаг используется для передачи больших файлов порциями.
	// Замечания по следующим флагам, регламентирующим сжатие и шифрование сообщения:
	// -- если hfEncrypted установлен, то тело сообщения, следующее за заголовком зашифровано. Алгоритм
	//    и ключ шифрования определяется вне протокола. То есть, сообщение не несет никакой информации
	//    об этом.
	// -- предполагается, сжатие будет работать по различным алгоритмам. Для начала мы ввели самый
	//    популярный вариант - ZLIB. Не должно быть ситуации при которой заголово содержит более одного
	//    флага с признаком сжатия.
	// -- если к сообщению одновременно должны быть применены и сжатие и шифрование, то исходное сообщение
	//    сначала сжимается, затем шифруется.
	// -- поле заголовка DataLen содержит размер сжатого и зашифрованного пакета (не исходного)
	// -- если необходимо включить в пакет хэш для контроля целостности, то это должно быть реализовано
	//    на уровне использования протокола. Данная спецификация не имеет механизма хранения и вычисления хэша.
	//
	public static final int hfEncrypted  = 0x0040; // @v11.0.10 Данные пакета (не включая заголовок) зашифрованы
	public static final int hfComprZ     = 0x0080; // @v11.0.10 Данные пакета (не включая заголовок) сжаты методом ZLIB
	//
	// Descr: Типы передач
	//
	public static final int TT_GENERIC                    = 0;
	public static final int TT_OBJIMAMGE                  = 1;
	public static final int TT_WORKBOOKCONTENT            = 2;
	//
	// Descr: Команды сервера
	//
	public static final int PPSCMD_HELLO                  = 10001;
	public static final int PPSCMD_LOGIN                  = 10002;
	public static final int PPSCMD_LOGOUT                 = 10003;
	public static final int PPSCMD_QUIT                   = 10004;
	public static final int PPSCMD_SPII                   = 10005;
	public static final int PPSCMD_STYLOBHT               = 10006;
	public static final int PPSCMD_GETBIZSCORES           = 10007;
	public static final int PPSCMD_STYLOBHTII             = 10008;
	public static final int PPSCMD_GETLASTERRMSG          = 10011;
	public static final int PPSCMD_SOBLK                  = 10019;
	public static final int PPSCMD_CHECKGLOBALCREDENTIAL  = 10020;
	public static final int PPSCMD_GETSERVERSTAT          = 10021;
	public static final int PPSCMD_STOPTHREAD             = 10022;
	public static final int PPSCMD_CREATEVIEW             = 10025;
	public static final int PPSCMD_DESTROYVIEW            = 10026;
	public static final int PPSCMD_HSH                    = 10027;
	public static final int PPSCMD_SUSPEND                = 10028;
	public static final int PPSCMD_RESUME                 = 10029;
	public static final int PPSCMD_CONFIG                 = 10030;
	public static final int PPSCMD_REFRESHVIEW            = 10031;
	public static final int PPSCMD_RFIDPRCSSR             = 10032;
	public static final int PPSCMD_GETTDDO                = 10033;
	public static final int PPSCMD_GETIMAGE               = 10034;
	public static final int PPSCMD_GETNEXTFILEPART        = 10035;
	public static final int PPSCMD_ACKFILE                = 10036;
	public static final int PPSCMD_CANCELFILE             = 10037;
	public static final int PPSCMD_EXECVIEWNF             = 10038;
	public static final int PPSCMD_PREPAREPALMOUTDATA     = 10039;
	public static final int PPSCMD_PREPAREPALMINDATA      = 10040;
	public static final int PPSCMD_GETFILE                = 10041;
	public static final int PPSCMD_PUTFILE                = 10042;
	public static final int PPSCMD_PUTNEXTFILEPART        = 10043;
	public static final int PPSCMD_PROCESSPALMXMLDATA     = 10044;
	public static final int PPSCMD_SETGLOBALUSER          = 10045;
	public static final int PPSCMD_GETGLOBALUSER          = 10046;
	public static final int PPSCMD_SETOBJECTTAG           = 10047;
	public static final int PPSCMD_GETOBJECTTAG           = 10048;
	public static final int PPSCMD_SETIMAGEMIME           = 10049;
	public static final int PPSCMD_SETIMAGE               = 10050;
	public static final int PPSCMD_PING                  = 10051; // Тестовая команда, для проверки работоспособности сервера и клиента. Может сопровождаться дополнительными параметрами при специфических тестах.
	public static final int PPSCMD_GTACHECKIN            = 10052; // Регистрация тарифицируемой транзакции по глобальной учетной записи
	public static final int PPSCMD_EXPTARIFFTA           = 10053; // Установить флаг - тарифицируемая транзакци
	public static final int PPSCMD_SENDSMS               = 10054; // Отправка SMS
	public static final int PPSCMD_POS_INIT              = 10055; //
	public static final int PPSCMD_POS_RELEASE           = 10056; // CPOSRELEASE id
	public static final int PPSCMD_POS_GETCCHECKLIST     = 10058; // CPOSGETCCHECKLIST
	public static final int PPSCMD_POS_GETCCHECKLNCOUNT  = 10060; // CPOSGETCCHECKLNCOUNT
	public static final int PPSCMD_POS_ADDCCHECKLINE     = 10061; // CPOSADDCCHECKLINE
	public static final int PPSCMD_POS_RMVCCHECKLINE     = 10062; // CPOSRMVCCHECKLINE
	public static final int PPSCMD_POS_CLEARCCHECK       = 10063; // CPOSCLEARCHECK
	public static final int PPSCMD_POS_RMVCCHECK         = 10064; // CPOSRMVCCHECK
	public static final int PPSCMD_POS_PRINTCCHECK       = 10065; // CPOSPRINTCHECK
	public static final int PPSCMD_POS_GETCONFIG         = 10066; // CPOSGETCONFIG
	public static final int PPSCMD_POS_SETCONFIG         = 10067; // CPOSSETCONFIG
	public static final int PPSCMD_POS_GETSTATE          = 10068; // CPOSGETSTATE
	public static final int PPSCMD_POS_SELECTCCHECK      = 10069; //
	public static final int PPSCMD_POS_SUSPENDCCHECK     = 10070; //
	public static final int PPSCMD_POS_SELECTCTABLE      = 10071;
	public static final int PPSCMD_POS_GETCPOSRIGHTS     = 10072;
	public static final int PPSCMD_POS_DECRYPTAUTHDATA   = 10074;
	public static final int PPSCMD_POS_GETGOODSPRICE     = 10075; // CPOSGETGOODSPRICE
	public static final int PPSCMD_POS_PRINTCCHECKLOCAL  = 10076; // CPOSPRINTCHECKLOCAL
	public static final int PPSCMD_POS_GETCURRENTSTATE   = 10077; // CPOSGETCURRENTSTATE
	public static final int PPSCMD_POS_RESETCURRENTLINE  = 10078; // CPOSRESETCURRENTLINE
	public static final int PPSCMD_POS_PROCESSBARCODE    = 10079; // CPOSPROCESSBARCODE
	public static final int PPSCMD_POS_GETCTABLELIST     = 10080; // CPOSGETCTABLELIST
	public static final int PPSCMD_POS_CPOSSETCCROWQUEUE = 10081; // CPOSSETCCROWQUEUE
	public static final int PPSCMD_POS_GETMODIFLIST      = 10082; // CPOSGETMODIFLIST goodsID
	public static final int PPSCMD_RESETCACHE            = 10101; // RESETCACHE
	public static final int PPSCMD_GETDISPLAYINFO        = 10102;
	public static final int PPSCMD_GETWORKBOOKCONTENT    = 10103;
	public static final int PPSCMD_SETTXTCMDTERM         = 10104; //
	public static final int PPSCMD_GETKEYWORDSEQ         = 10105; //
	public static final int PPSCMD_REGISTERSTYLO         = 10106; //
	public static final int PPSCMD_SETWORKBOOKCONTENT    = 10107; //
	public static final int PPSCMD_QUERYNATURALTOKEN     = 10108; //
	public static final int PPSCMD_GETARTICLEBYPERSON    = 10109; //
	public static final int PPSCMD_GETPERSONBYARTICLE    = 10110; //
	public static final int PPSCMD_LOGLOCKSTACK          = 10111; // Отладочная команда приводящая к выводу стека блокировок всех потоков в журнал debug.log
	public static final int PPSCMD_SETTIMESERIES         = 10112; // @v10.2.3
	public static final int PPSCMD_GETREQQUOTES          = 10113; // @v10.2.4
	public static final int PPSCMD_SETTIMESERIESPROP     = 10114; // @v10.2.5
	public static final int PPSCMD_SETTIMESERIESSTKENV   = 10115; // @v10.2.10
	public static final int PPSCMD_TIMESERIESTANOTIFY    = 10116; // @v10.3.11
	public static final int PPSCMD_GETCOMMONMQSCONFIG    = 10117; // @v10.5.9
	public static final int PPSCMD_SQ_ACQUAINTANCE       = 10118; // @v11.0.10 Инициирующее сообщение от клиента сервису для установки контакта. Клиент еще не "знаком" с сервисом.
	public static final int PPSCMD_SQ_SESSION            = 10119; // @v11.0.10
	public static final int PPSCMD_SQ_SRPREGISTER        = 10120; // @v11.0.10 Регистрация по SRP-протоколу
	public static final int PPSCMD_SQ_SRPAUTH            = 10121; // @v11.0.10 Авторизация по SRP-протоколу
	public static final int PPSCMD_SQ_SRPAUTH_S2         = 10122; // @v11.0.11 Авторизация по SRP-протоколу (the second phase)
	public static final int PPSCMD_SQ_SRPAUTH_ACK        = 10123; // @v11.0.11 Авторизация по SRP-протоколу (завершающее сообщение от клиента серверу об Успешности авторизации)
	public static final int PPSCMD_SQ_COMMAND            = 10124; // @v11.0.11 Собственно команда в рамках протокола Stylo-Q
	public static final int PPSCMD_TEST                  = 11000;

	public static class Header {
		Header Z()
		{
			Zero = 0;
			ProtocolVer = 1;
			Padding = 0;
			DataLenWithHdr = 0;
			Type = 0;
			Flags = 0;
			return this;
		}
		public static final int ZERO_SIZE = 2;
		public static final int SIZE = 16;    // Размер заголовка
		public short Zero;
		public byte ProtocolVer;
		public byte Padding; // Если данные шифруются, то в это поле вносится размер "набивки" (padding) до величины блока шифрования
		public int DataLenWithHdr; // Полный размер пакета (включая и этот заголовок). Это - размер передаваемый транспортом. Следовательно,
			// при блочном шифровании эффективные данные имеют размер (DataLen-Padding)
		public int Type;  // Для команды: идент команды, для ответа: тип данных.
		public int Flags; // @flags
	}
	public static void UnpackHeader(final byte[] buf, Header header)
	{
		header.Zero = (short)((buf[0] & 0xff) | ((buf[1] & 0xff) << 8));
		header.ProtocolVer = buf[2];
		header.Padding = buf[3];
		header.DataLenWithHdr = (buf[4] & 0xff) | ((buf[5] & 0xff) << 8) |
				((buf[6] & 0xff) << 16) | ((buf[7] & 0xff) << 24);
		header.Type = (buf[8] & 0xff) | ((buf[9] & 0xff) << 8) |
				((buf[10] & 0xff) << 16) | ((buf[11] & 0xff) << 24);
		header.Flags = (buf[12] & 0xff) | ((buf[13] & 0xff) << 8) |
				((buf[14] & 0xff) << 16) | ((buf[15] & 0xff) << 24);
		//DataLen = header.DataLenWithHdr - Header.SIZE;
		//DataType = BINARY_DATA;
	}
	public static byte[] PackHeader(final Header header)
	{
		//byte buf[] = new byte[Header.SIZE + ((Data == null) ? 0 : Data.length)];
		byte buf[] = new byte[Header.SIZE];
		// header.Zero
		buf[0] = (byte)header.Zero;
		buf[1] = (byte)(header.Zero >>> 8);
		// header.ProtocolVer
		buf[2] = header.ProtocolVer;
		buf[3] = header.Padding;
		// header.DataLenWithHdr
		buf[4] = (byte)header.DataLenWithHdr;
		buf[5] = (byte)(header.DataLenWithHdr >>> 8);
		buf[6] = (byte)(header.DataLenWithHdr >>> 16);
		buf[7] = (byte)(header.DataLenWithHdr >>> 24);
		// header.Type
		buf[8] = (byte)header.Type;
		buf[9] = (byte)(header.Type >>> 8);
		buf[10] = (byte)(header.Type >>> 16);
		buf[11] = (byte)(header.Type >>> 24);
		// header.Flags
		buf[12] = (byte)header.Flags;
		buf[13] = (byte)(header.Flags >>> 8);
		buf[14] = (byte)(header.Flags >>> 16);
		buf[15] = (byte)(header.Flags >>> 24);
		/*if(Data != null)
			System.arraycopy(Data, 0, buf, Header.SIZE, Data.length);*/
		return buf;
	}
	public static class Frame {
		Frame()
		{
			H = new Header();
			D = null;
			State = 0;
		}
		Frame Z()
		{
			H.Z();
			D = null;
			State = 0;
			return this;
		}
		public boolean CheckRepError()
		{
			if((H.Flags & hfRepError) != 0) {
				//PPSetError(PPERR_JOBSRVCLI_ERR, ErrText);
				return false;
			}
			else
				return true;
		}

		protected static final int stStructured = 0x0001;
		protected static final int stReading    = 0x0002;

		Header H;
		byte [] D; // Буфер в котором находятся "сырые" данные, полученные из источника,
			// либо которые будут отправлены получателю (с заголовком, шифрованием, сжатием и т.д.)
		int    State; // @transient
	}
	public static class StyloQFrame extends Frame {
		StyloQFrame()
		{
			super();
			P = new SecretTagPool();
		}
		StyloQFrame Z()
		{
			super.Z();
			P.Z();
			return this;
		}
		public boolean CheckRepError(StyloQApp appCtx)
		{
			boolean result = super.CheckRepError();
			if(!result) {
				if(appCtx != null) {
					byte[] err_code_raw = P.Get(SecretTagPool.tagErrorCode);
					byte[] err_code_text_raw = P.Get(SecretTagPool.tagReplyStatusText);
					String err_text = (SLib.GetLen(err_code_text_raw) > 0) ? new String(err_code_text_raw) : null;
					if(SLib.GetLen(err_code_raw) == 4) {
						int reply_err_code = SLib.BytesToInt(err_code_raw, 0);
						appCtx.SetLastError(reply_err_code, err_text);
					}
					else
						appCtx.SetLastError(ppstr2.PPERR_SQ_SVCREPLYFAULT, err_text);
				}
			}
			return result;
		}

		public static final int psubtypeForward = 0;
		public static final int psubtypeForwardError = 1;
		public static final int psubtypeReplyOk      = 2;
		public static final int psubtypeReplyError   = 3;
		public static final int psubtypeIntermediateReply = 4; // @v11.2.12 Промежуточный ответ сервера с просьбой подождать либо отчет о ходе выполнения процесса

		public boolean StartWriting(int cmdId, int subtype)
		{
			boolean ok = true;
			super.Z();
			H.ProtocolVer = CurrentProtocolVer;
			H.Type = cmdId;
			H.DataLenWithHdr = Header.SIZE;
			if(subtype == psubtypeForwardError)
				H.Flags |= hfRepError;
			else if(subtype == psubtypeReplyOk)
				H.Flags |= hfAck;
			else if(subtype == psubtypeReplyError)
				H.Flags |= (hfAck|hfRepError);
			D = PackHeader(H);
			//THROW_SL(Write(&H, sizeof(H)));
			State |= stStructured;
			State &= ~stReading;
			return ok;
		}
		public void FinishWriting(final byte [] cryptoKey) throws StyloQException
		{
			try {
				boolean encrypted = false;
				byte padding = 0; // Размер "набивки" буфера до кратного блоку шифрования //
				THROW((State & stStructured) != 0 && SLib.GetLen(D) == H.SIZE, ppstr2.PPERR_SQPROT_WRUNSTRUCTURED);
				byte[] plain_useful_data = P.Serialize();
				byte[] finish_useful_data = null;
				if(SLib.GetLen(cryptoKey) > 0) {
					MessageDigest digest = MessageDigest.getInstance("MD5");
					digest.update(cryptoKey);
					byte[] keydigest = digest.digest();
					if(SLib.GetLen(keydigest) == 16) {
						SecretKeySpec skeyspec = new SecretKeySpec(keydigest, "AES");
						Cipher cipher = Cipher.getInstance("AES/ECB/PKCS5Padding");
						cipher.init(Cipher.ENCRYPT_MODE, skeyspec);
						byte[] cipher_result = cipher.doFinal(plain_useful_data);
						if(SLib.GetLen(cipher_result) > plain_useful_data.length) {
							int ip = cipher_result.length - plain_useful_data.length;
							THROW(ip < 128, ppstr2.PPERR_SQPROT_WRINNERSTATE);
							finish_useful_data = cipher_result;
							padding = (byte) ip;
							encrypted = true;
						}
					}
				}
				else {
					finish_useful_data = plain_useful_data;
				}
				ByteArrayOutputStream baos = new ByteArrayOutputStream();
				H.DataLenWithHdr = Header.SIZE + finish_useful_data.length;
				H.Padding = padding;
				if(encrypted)
					H.Flags |= hfEncrypted;
				else
					H.Flags &= ~hfEncrypted;
				baos.write(PackHeader(H));
				baos.write(finish_useful_data);
				D = baos.toByteArray();
			} catch(IOException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			} catch(NoSuchAlgorithmException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
			} catch(InvalidKeyException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_INVCRYPTOKEY, exn.getMessage());
			} catch(NoSuchPaddingException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_NOSUCHPADDING, exn.getMessage());
			} catch(BadPaddingException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_BADPADDING, exn.getMessage());
			} catch(IllegalBlockSizeException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_ILLEGALBLOCKSIZE, exn.getMessage());
			}
			/*
				int    ok = 1;
				bool   encrypted = false;
				uint8  padding = 0; // Размер "набивки" буфера до кратного блоку шифрования //
				assert(State & stStructured);
				assert(GetWrOffs() == sizeof(H));
				THROW(State & stStructured);
				if(pCryptoKey) {
					const binary128 key_md5 = SlHash::Md5(0, pCryptoKey->PtrC(), pCryptoKey->Len());
					SlCrypto cryp(SlCrypto::algAes, SlCrypto::kbl128, SlCrypto::algmodEcb);
					SlCrypto::Key crypkey;
					const SlCrypto::CipherProperties & r_cp = cryp.GetCipherProperties();
					P.Pack();
					STempBuffer cbuf2(P.GetDataLen() + 64); // 64 ensurance
					size_t cryp_size = 0;
					THROW_SL(cbuf2.IsValid());
					THROW_SL(cryp.SetupKey(crypkey, &key_md5, sizeof(key_md5), 0, 0));
					{
						const size_t raw_data_len = P.GetDataLen();
						size_t padded_size = r_cp.BlockSize ? (raw_data_len ? ((((raw_data_len-1) / r_cp.BlockSize) + 1) * r_cp.BlockSize) : r_cp.BlockSize) : raw_data_len;
						assert(padded_size >= raw_data_len && (padded_size - raw_data_len) < 256);
						THROW(padded_size >= raw_data_len && (padded_size - raw_data_len) < 256);
						padding = static_cast<uint8>(padded_size - raw_data_len);
						THROW_SL(P.Ensure(padded_size));
						THROW_SL(cryp.Encrypt_(&crypkey, P.GetPtr(), padded_size, cbuf2.vptr(), cbuf2.GetSize(), &cryp_size));
						THROW_SL(SBuffer::Write(cbuf2.vcptr(), cryp_size));
						encrypted = true;
					}
				}
				else {
					THROW(P.Serialize(+1, *this, 0));
				}
				{
					Header * p_hdr = static_cast<Header *>(SBuffer::Ptr(SBuffer::GetRdOffs()));
					p_hdr->DataLen = static_cast<int32>(SBuffer::GetAvailableSize());
					SETFLAG(p_hdr->Flags, hfEncrypted, encrypted);
					p_hdr->Padding = padding;
				}
				CATCHZOK
				return ok;
			 */
		}
		public void Read(final byte [] src, final byte [] cryptoKey) throws StyloQException
		{
			Z();
			try {
				ByteArrayInputStream bais = new ByteArrayInputStream(src);
				THROW(bais.available() >= Header.SIZE, ppstr2.PPERR_SQPROT_RDHDRSIZE);
				//Header hdr = new Header();
				if(H == null)
					H = new Header();
				byte[] temp_buf = new byte[Header.SIZE];
				int actual_size = bais.read(temp_buf, 0, Header.SIZE);
				THROW(actual_size == Header.SIZE, ppstr2.PPERR_SQPROT_RDHDRSIZE);
				UnpackHeader(temp_buf, H);
				THROW(H.ProtocolVer == CurrentProtocolVer, ppstr2.PPERR_SQPROT_RDVERSION);
				final int src_size = bais.available();
				THROW(H.DataLenWithHdr == (src_size + Header.SIZE), ppstr2.PPERR_SQPROT_INVHDR);
				THROW((H.Flags & hfEncrypted) != 0 || H.Padding == 0, ppstr2.PPERR_SQPROT_INVHDR);
				if(src_size > 0) {
					temp_buf = new byte[src_size];
					actual_size = bais.read(temp_buf, 0, src_size);
					THROW(actual_size == src_size, ppstr2.PPERR_SQPROT_INVHDR);
					if((H.Flags & hfEncrypted) != 0) {
						// @debug {
						if(SLib.GetLen(cryptoKey) <= 0) {
							THROW(SLib.GetLen(cryptoKey) > 0, ppstr2.PPERR_SQPROT_NOCRYPTOKEY);
						}
						// } @debug
						THROW(SLib.GetLen(cryptoKey) > 0, ppstr2.PPERR_SQPROT_NOCRYPTOKEY);
						MessageDigest digest = MessageDigest.getInstance("MD5");
						digest.update(cryptoKey);
						byte[] keydigest = digest.digest();
						THROW(SLib.GetLen(keydigest) == 16, ppstr2.PPERR_SQPROT_DECRYPTFAULT);
						SecretKeySpec skeyspec = new SecretKeySpec(keydigest, "AES");
						Cipher cipher = Cipher.getInstance("AES/ECB/PKCS5Padding");
						cipher.init(Cipher.DECRYPT_MODE, skeyspec);
						byte[] cipher_result = cipher.doFinal(temp_buf);
						THROW(cipher_result.length > H.Padding, ppstr2.PPERR_SQPROT_DECRYPTFAULT);
						if(H.Padding == 0) {
							P.Unserialize(cipher_result);
						}
						else {
							THROW(H.Padding > 0, ppstr2.PPERR_SQPROT_DECRYPTFAULT); // Иначе - белиберда в данных. Не может такого быть!
							int real_len = cipher_result.length - H.Padding;
							temp_buf = new byte[real_len];
							System.arraycopy(cipher_result, 0, temp_buf, 0, real_len);
							P.Unserialize(temp_buf);
						}
					}
					else {
						P.Unserialize(temp_buf);
					}
				}
			} catch(NoSuchAlgorithmException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
			} catch(InvalidKeyException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_INVCRYPTOKEY, exn.getMessage());
			} catch(NoSuchPaddingException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_NOSUCHPADDING, exn.getMessage());
			} catch(BadPaddingException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_BADPADDING, exn.getMessage());
			} catch(IllegalBlockSizeException exn) {
				new StyloQException(ppstr2.PPERR_JEXN_ILLEGALBLOCKSIZE, exn.getMessage());
			}
		}
		SecretTagPool P;
	}
}
