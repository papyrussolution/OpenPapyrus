// SNET.H
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2012, 2013, 2014, 2015, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#ifndef __SNET_H
#define __SNET_H

#include <slib.h>
//
//
//
struct MACAddr : public TSBinary <6> { // @persistent @size=6
	enum {
		fmtDivColon = 0x0001, // Разделитель разрядов ':'. Если этот флаг не установлен и не установлен так же fmtPlain, то разделитель разрядов '-'
		fmtPlain    = 0x0002, // Не разделять разряды 
		fmtLower    = 0x8000, // Флаг, предписывающий использовать буквы [a-f] в строчном регистре
	};
	MACAddr();
	MACAddr(const MACAddr & rS);
	//MACAddr & FASTCALL operator = (const MACAddr & rS);
	//bool   FASTCALL operator == (const MACAddr & rS) const;
	//MACAddr & Z();
	//bool   IsEmpty() const;
	SString & FASTCALL ToStr(long fmt, SString & rBuf) const;
	bool   FromStr(const char * pStr);
	//
	// Descr: сравнивает MAC-адреса this и s.
	//   Сравнение осуществляется побайтно, начиная с байта Addr[0]
	// Returns:
	//   0 - адреса this и s эквивалентны
	//  <0 - адрес this меньше адреса s
	//  >0 - адрес this больше адреса s
	//
	int    FASTCALL Cmp(const MACAddr & s) const;
};

class MACAddrArray : public TSVector <MACAddr> {
public:
	MACAddrArray();
	int    addUnique(const MACAddr &);
};

int GetFirstMACAddr(MACAddr *);
int GetMACAddrList(MACAddrArray *);
//
//
//
class S_IPAddr : public binary128 { // @persistent @size=16 // @v12.0.0 @construction
public:
	S_IPAddr();
	bool    IsIp4() const;
	bool    IsIp6() const;
	SString & ToStr(long fmt, SString & rBuf) const;
	bool    FromStr(const char * pStr);
};
//
//
//
class InetAddr {
public:
	enum {
		fmtAddr = 0x0001,
		fmtHost = 0x0002,
		fmtPort = 0x0004
	};
	static ulong IPToULong(const char * pIP);
	static void  ULongToIP(ulong ip, SString & rIP);
	static int GetNameByAddr(const char * pIP, SString & aHost);
	InetAddr();
	InetAddr(const InetAddr & rS);
	InetAddr & FASTCALL operator = (const InetAddr & rS);
	void   FASTCALL Copy(const InetAddr & rS);
	operator ulong() const { return V4; }
	InetAddr & Z();
	bool   FASTCALL IsEq(const InetAddr & rS) const;
	bool   FASTCALL operator == (const InetAddr & rS) const;
	bool   FASTCALL operator != (const InetAddr & rS) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	bool   IsEmpty() const;
	int    GetPort() const { return Port; }
	const SString & GetHostName() const { return HostName; }
	int    Set(ulong addr, int port = 0);
	int    Set(const char * pHostName, int port = 0);
	int    Set(const sockaddr_in *);
	int    SetPort_(int port);
	sockaddr * Get(sockaddr_in *) const;
	SString & ToStr(long flags /* InetAddr::fmtXXX */, SString & rBuf) const;
	int    FromStr(const char *);
private:
	uint32 V4;
	int    Port;
	SString HostName;
};
//
//
//
class InetUrl : public InetAddr {
public:
	enum { // @persistent
		protUnkn      =  0,
		protHttp      =  1,  // http
		protHttps     =  2,  // https
		protFtp       =  3,  // ftp
		protGopher    =  4,  // gopher
		protMailto    =  5,  // mailto
		protNews      =  6,
		protNntp      =  7,
		protIrc       =  8,
		protProspero  =  9,
		protTelnet    = 10,
		protWais      = 11,
		protXmpp      = 12,
		protFile      = 13,
		protData      = 14,
		protSvn       = 15,
		protSocks4    = 16,
		protSocks5    = 17,
		protSMTP      = 18, // Протокол отправки почты
		protSMTPS     = 19, // Протокол отправки почты (SSL)
		protPOP3      = 20, // Протокол получения почты
		protPOP3S     = 21, // Протокол получения почты (SSL)
		protIMAP      = 22, // Internet Message Access Protocol
		protIMAPS     = 23, // Internet Message Access Protocol (SSL)
		protFtps      = 24, // ftps
		protTFtp      = 25,
		protDict      = 26,
		protSSH       = 27,
		protSMB       = 28,
		protSMBS      = 29,
		protRTSP      = 30,
		protRTMP      = 31,
		protRTMPT     = 32,
		protRTMPS     = 33,
		protLDAP      = 34,
		protLDAPS     = 35,
		protMailFrom  = 36, // Фиктивный протокол (не имеющий соответствия в стандартах). Применяется
			// для внутреннего представления описания параметров приема данных из почтовых сообщений.
		prot_p_PapyrusServer = 37, // @v10.2.3 private-протокол системы Papyrus
		protAMQP      = 38, // @v10.5.3
		protAMQPS     = 39, // @v10.5.3
		prot_p_MYSQL  = 40, // @v10.9.2 private: идентифицирует url доступа к серверу MySQL
		prot_p_SQLITE = 41, // @v10.9.2 private: идентифицирует url доступа к базе данных SQLite
		prot_p_ORACLE = 42, // @v10.9.2 private: идентифицирует url доступа к серверу базы данных ORACLE
		protGit       = 43, // @v11.1.11 
	};
	//
	// Descr: Компоненты URL
	//
	enum { // @persistent
		cScheme = 1, // Схема (протокол) (http)://
		cUserName,   // Имя доступа
		cPassword,   // Пароль доступа
		cHost,       // Наименование хоста https://(twitter.com)
		cPort,       // ip-port http://192.168.0.100:(8080)
		cPath,       // Путь   http://192.168.0.100:8080(/index)?param=value
		cQuery,      // Запрос http://192.168.0.100:8080/index(?param=value)
		cRef         // Ссылка http://192.168.0.100:8080/index#(15)
	};
	enum {
		stError    = 0x80000000,
		stEmpty    = 0x40000000,
		stAll      = 0xffffffff,
		stScheme   = (1 << (cScheme-1)),
		stUserName = (1 << (cUserName-1)),
		stPassword = (1 << (cPassword-1)),
		stHost     = (1 << (cHost-1)),
		stPort     = (1 << (cPort-1)),
		stPath     = (1 << (cPath-1)),
		stQuery    = (1 << (cQuery-1)),
		stRef      = (1 << (cRef-1))
	};
	static const char * FASTCALL GetSchemeMnem(int);
	static int FASTCALL GetSchemeId(const char * pSchemeMnem);
	static int FASTCALL GetDefProtocolPort(int protocol);
	static int FASTCALL ValidateComponent(int c);
	static SString & FASTCALL Mk(int protocol, const char * pHost, const char * pPath);
	static SString & FASTCALL MkHttp(const char * pHost, const char * pPath) { return Mk(protHttp, pHost, pPath); }
	static SString & FASTCALL MkHttps(const char * pHost, const char * pPath) { return Mk(protHttps, pHost, pPath); }

	explicit InetUrl(const char * pUrl = 0);
	InetUrl(const InetUrl & rS);
	InetUrl & FASTCALL operator = (const InetUrl & rS);
	void   FASTCALL Copy(const InetUrl & rS);
	InetUrl & Z();
	long   GetState() const { return State; }
	bool   IsValid() const;
	bool   IsEmpty() const;
	int    Parse(const char * pUrl);
	int    GetComponent(int c, int urlDecode, SString & rBuf) const;
	int    SetComponent(int c, const char * pBuf);
	int    GetProtocol() const { return Protocol; }
	int    SetProtocol(int protocol);
	int    GetQueryParam(const char * pParam, int urlDecode, SString & rBuf) const;
	int    SetQueryParam(const char * pParam, const char * pValue);
	//
	// Descr: Формирует url-строку из компонентов, установленных данном экземпляре.
	// ARG(flags IN): Набор битовых флагов, соответствующих компонентам, которые должны быть внесены в строку.
	//   Значение может быть комбинацией флагов stXXX. При этом flags==stError эквивалентно flags==stEmpty,
	//   flags==stEmpty просто обнуляет буфер rBuf, stAll - предписывает вставить в строку все доступные
	//   компоненты. flags == 0 эквивалентно flags==stAll.
	//   Для того, чтобы опустить один или несколько компонентов можно передать значение (stAll & ~(stXXX))
	//   Например: (stAll & ~(stUserName|stPassword)).
	// ARG(rBuf OUT): Буфер, в котором формируется url.
	// Returns: комбинация флагов stXXX, соответствующая набору компонентов, внесенному в буфер rBuf.
	//
	int    Compose(long flags, SString & rBuf) const;
private:
	int    Protocol;
	StrAssocArray TermList;
	SString Org;
	long   State;
};

int GetFirstHostByMACAddr(const MACAddr * pItem, InetAddr * pAddr);
//
//
//
class TcpSocket {
public:
	enum {
		mRead = 0,
		mWrite
	};
	enum SslMode {
		sslmNone = 0,
		sslmClient = 1
	};

	explicit TcpSocket(int timeout = 0, int maxConn = SOMAXCONN);
	~TcpSocket();
	bool   IsValid() const;
	operator SOCKET () const { return S; }
	bool   FASTCALL IsEq(const TcpSocket & rS) const { return (S == rS.S); } // @v11.0.9
	//
	// Descr:
	//
	int    CheckErrorStatus(); // @>>::getsockopt(S, SOL_SOCKET, SO_ERROR,...)
	int    GetTimeout() const;
	int    FASTCALL SetTimeout(int timeout);
	//
	// Descr: Копирует сокет this в rDest и сбрасывает значение
	//   this->S. Таким образом, деструктор this не закроет сокет,
	//   переданный в rDest.
	// Remark: Если rDest.IsValid() != 0, то функция возвращает ошибку и ничего
	//   не делает, поскольку в этом случае мы рискуем потерять без корректного
	//   разрушения экземпляр сокета.
	// Returns:
	//   >0 - перемещение валидного сокета завершилось успешно
	//   <0 - копирование сокета завершилось успешно, однако this имел
	//        инвалидное значение S (this->IsValid() == 0).
	//   0  - ошибка.
	//
	int    MoveToS(TcpSocket & rDest, int force = 0); // @debug force = 0
	int    CopyS(TcpSocket & rDest);
	int    Connect(const InetAddr &);
	int    Connect(SslMode sslm, const InetAddr &);
	int    Bind(const InetAddr &);
	int    GetSockName(InetAddr * pAddr, int peer);
	int    Disconnect();
	//
	// Descr: Определяет состояние сокета (через вызов ::select).
	// ARG(mode IN): Один из вариантов: TcpSocket::mRead or TcpSocket::mWrite.
	//   Если mode == TcpSocket::mRead, то определяется состояние сокета на чтение,
	//   в противном случае (mode != TcpSocket::mRead) - на запись.
	// ARG(timeout IN): Таймаут ожидания в миллисекундах.
	//   Если timeout < 0, то используется внутреннее значение Timeout.
	//   Если результирующее значение 0, то функция ::select получает не нулевой указатель
	//   не TIMEVAL, в котором все поля обнулены.
	// ARG(pAvailableSize OUT): @#{vptr0} Количество доступных для чтения (mode==TcpSocket::mRead)
	//   или для записи в сокете.
	// Returns:
	//   >0 - сокет готов для приема или отправки данных
	//   0  - ошибка
	//   <0 - истекло время ожидания timeout (или TcpSocket::Timeout, если timeout<0)
	//        В этом случае переменная SLibError получает код SLERR_SOCK_TIMEOUT
	// Note: Обратите внимание на то, что смысл кодов возврата не совпадает со
	//   смыслов кода возврата функции ::select
	//
	int    Select(int mode /* TcpSocket::mXXX */, int timeout = -1, size_t * pAvailableSize = 0);
		// @>>::select
	int    Listen(); // @>>::listen
	int    Accept(TcpSocket &, InetAddr &); // @>>::accept
	//
	// Descr: Читает из сокета данные в буфер pBuf размер которого ограничен
	//   величиной bufLen. По указателю pRcvdSize присваивается количество
	//   прочитанных байт.
	//
	int    Recv(void * pBuf, size_t bufLen, size_t * pRcvdSize); // @>>::recv
	//
	// Descr: Читает из сокета блок данных длиной size в буфер pBuf.
	//   Отличается от функции Recv тем, что читает в цикле до тех пор, пока не
	//   получит требуемое количество байт, либо ошибку, либо очередное
	//   считывние не вернет 0 байт.
	//
	int    RecvBlock(void * pBuf, size_t size, size_t * pRcvdSize);
	//
	// Descr: Записывает в сокет данные из буфера pBuf в количестве dataLen байт.
	//   По указателю pSendedSize возвращается количество действительно переданных
	//   данных.
	//
	int    Send(const void * pBuf, size_t dataLen, size_t * pSendedSize); // @>>::send
	int    RecvBuf(SBuffer & rBuf, size_t frameSize, size_t * pRcvdSize);
	//
	// Descr: Считывает из сокета данные в буфер rBuf до тех пор, пока не
	//   встретится терминальная последовательность pTerminator.
	//   Если pTerminator == 0 || strlen(pTerminator) == 0, то вызывает RecvBuf(rBuf, 0, pRcvdSize)
	//
	int    RecvUntil(SBuffer & rBuf, const char * pTerminator, size_t * pRcvdSize);
	int    SendBuf(SBuffer & rBuf, size_t * pSendedSize);
	int    GetStat(long * pRdCount, long * pWrCount);
#ifdef SLTEST_RUNNING // SLTEST_RUNNING {
	int    BreakSocket() // Прервать связь без отсоединения //
	{
		Reset();
		return 1;
	}
#endif  // } SLTEST_RUNNING
private:
	static size_t DefaultReadFrame;  // Размер кванта считывания из сокета по умолчанию //
	static size_t DefaultWriteFrame; // Размер кванта записи в сокет //

	int    Init(SOCKET s);
	void   Reset();
	int    Helper_Connect(SslMode sslm, const InetAddr & rAddr);
	int    FASTCALL Helper_Recv(void * pBuf, size_t size);

	struct Stat {
		long   RdCount;
		long   WrCount;
		size_t RcvBufSize; // Внутренний размер буфера чтения сокета (извлекается вызовом getsockopt)
		size_t SndBufSize; // Внутренний размер буфера записи сокета (извлекается вызовом getsockopt)
	};
	struct SslBlock {
		SslBlock();
		~SslBlock();
		bool   operator !() const { return !IsValid(); }
		bool   IsValid() const { return (P_Ctx && P_S); }
		int    Init();
		int    Shutdown();
		int    Connect(SOCKET s);
		int    Accept();
		int    Select(int mode /* TcpSocket::mXXX */, int timeout, size_t * pAvailableSize);
		int    Read(void * buf, int len);
		int    Write(const void * buf, int len);

		enum {
			stError = 0x0001
		};
	private:
		long   State;
		int    LastError;
		void * P_Ctx;
        void * P_S;
	};
	SOCKET S;
	SslBlock * P_Ssl;
	int    Timeout;
	int    MaxConn;
	int    LastSockErr; // Последний код ошибки, полученный вызовом TcpSocket::CheckErrorStatus()
	Stat   StatData;
	STempBuffer InBuf;
	STempBuffer OutBuf;
};
//
// Descr: Структура, унифицирующая адрес прослушивания (listening) ip-сервера.
//
struct IpServerListeningEntry { // @v11.9.1 
	bool   Parse(const char * pText);
	InetAddr A;
};
//
//
//
class TcpServer : private TcpSocket {
public:
	explicit TcpServer(const /*InetAddr & rAddr*/IpServerListeningEntry & rSle);
	virtual ~TcpServer();
	int    Run();
	virtual int ExecSession(TcpSocket & rSock, InetAddr & rAddr);
		// @<<TcpServer::Run
private:
	//InetAddr Addr;
	IpServerListeningEntry Sle;
};
//
//
//
class SMailClient {
public:
	enum {
		authtNoAuth = 0,
		authtPlain,
		authtLogin,
		authtCramMD5,
		authtPOP3
	};
	enum {
		stConnected = 0x0001,
		stLoggedIn  = 0x0002
	};
	struct Capability {
		Capability() : SmtpMaxSize(0)
		{
		}
		Capability & Z()
		{
			SmtpMaxSize = 0;
			SmtpServerName.Z();
			SmtpAuthTypeList.clear();
			return *this;
		}
        int64  SmtpMaxSize;
        SString SmtpServerName;
        LongArray SmtpAuthTypeList;
	};

	static SString & FASTCALL Pop3_SkipReplyStatus(SString & rBuf);

	SMailClient();
	~SMailClient();
	int    Connect(InetUrl & rUrl, int timeout = -1);
	int    Disconnect();
	const  Capability & GetCapability() const { return C; }
	int    Auth(int alg, const char * pName, const char * pPassword);
	//
	// Descr: Считывает из сокета одну или более строк.
	//   Первая строка всегда считывается в буфер rBuf. Если pTail != 0, то
	//   остальные строке, если имеются в сокете, считываются как последовательные элементы
	//   pTail.
	// Note: Функция отрезает терминальные символы перевода каретки (\xD\xA) с конца считанных строк.
	//
	int    ReadLine(SString & rBuf);
	int    CheckReply(const SString & rReplyBuf, int onlyValidCode = 0);
	//
	// Descr: Записывает в сокет строку pBuf. Если параметр pReply != 0, то
	//   сразу после успешной записи считывает первую строку ответа сервера в pReply.
	// Note: Строка pBuf не должна иметь терминального перевода каретки (\xD\xA). Функция
	//   WriteLine самостоятельно добавляет терминатор к строке аргумента.
	//
	int    WriteLine(const char * pBuf, SString * pReply);
	int    WriteBlock(const void * pData, size_t dataSize);

	int    Pop3_GetStat(long * pCount, long * pSize);
	int    Pop3_GetMsgSize(long msgN, long * pSize);
	int    Pop3_DeleteMsg(long msgN);
private:
	long   State;
	InetUrl Url;
	Capability C;
	TcpSocket So;
	SBuffer RdBuf;
	SString WrLineBuf;
	SMailClient * P_Pop3AuthSession;
};
//
//
//
class SHttpProtocol {
public:
	enum  {
		reqUnkn = 0,
		reqPost,           // "POST"
		reqGet,            // "GET"
		regOptions,        // "OPTIONS"
		reqHead,           // "HEAD"
		reqPut,            // "PUT"
		reqDelete,         // "DELETE"
		reqTrace,          // "TRACE"
		reqConnect         // "CONNECT"
	};
	enum {
		hdrNone = 0,
		//
 		// General Header Fields
		//
 		// There are a few header fields which have general applicability for both
 		// request and response messages, but which do not apply to the entity being
 		// transferred. These header fields apply only to the message being transmitted. (see RFC2616)
		//
		hdrCacheControl,   // "Cache-Control"
		hdrConnection,     // "Connection"
		hdrDate,           // "Date"
		hdrPragma,         // "Pragma"
		hdrTrailer,        // "Trailer"
		hdrTransferEnc,    // "Transfer-Encoding"
		hdrUpgrade,        // "Upgrade"
		hdrVia,            // "Via"
		hdrWarning,        // "Warning"
		//
 		// Entity Header Fields
 		//   Entity-header fields define metainformation about the entity-body or, if no
 		//   body is present, about the resource identified by the request. Some of this
 		//   metainformation is OPTIONAL; some might be REQUIRED by portions of this
 		//   specification. (see RFC2616 7.1)
		//
		hdrAllow,          // "Allow"
		hdrContentEnc,     // "Content-Encoding"
		hdrContentLang,    // "Content-Language"
		hdrContentLen,     // "Content-Length"
		hdrContentLoc,     // "Content-Location"
		hdrContentMD5,     // "Content-MD5"
		hdrContentRange,   // "Content-Range"
		hdrContentType,    // "Content-Type"
		hdrExpires,        // "Expires"
		hdrLastModif,      // "Last-Modified"
		//
		// Response Header Fields
		//   The response-header fields allow the server to pass additional information
		//   about the response which cannot be placed in the Status-Line. These header
		//   fields give information about the server and about further access to the
		//   resource identified by the Request-URI. (see RFC2616)
		//
		hdrAcceptRanges,   // "Accept-Ranges"
		hdrAge,            // "Age"
		hdrExtTag,         // "ETag"
		hdrLoc,            // "Location"
		hdrAuthent,        // "Proxy-Authenticate"
		hdrRetryAfter,     // "Retry-After"
		hdrServer,         // "Server"
		hdrVary,           // "Vary"
		hdrWwwAuthent,     // "WWW-Authenticate"
		//
		// Request Header Fields
		//   The request-header fields allow the client to pass additional information
		//   about the request, and about the client itself, to the server. These fields
		//   act as request modifiers, with semantics equivalent to the parameters on a
		//   programming language method invocation (see RFC2616).
		//
		hdrAccept,           // "Accept"
		hdrAcceptCharset,    // "Accept-Charset"
		hdrAcceptEnc,        // "Accept-Encoding"
		hdrAcceptLang,       // "Accept-Language"
		hdrAuthorization,    // "Authorization"
		hdrExpect,           // "Expect"
		hdrFrom,             // "From"
		hdrHost,             // "Host"
		hdrIfMatch,          // "If-Match"
		hdrIfModifSince,     // "If-Modified-Since"
		hdrIfNonMatch,       // "If-None-Match"
		hdrIfRange,          // "If-Range"
		hdrIfUnmodifSince,   // "If-Unmodified-Since"
		hdrMaxForwards,      // "Max-Forwards"
		hdrProxiAuth,        // "Proxy-Authorization"
		hdrRange,            // "Range"
		hdrReferer,          // "Referer"
		hdrTransferExt,      // "TE"
		hdrUserAgent,        // "User-Agent"
		//
		hdrSoapAction,       // "SoapAction"
		hdrAuthToken,        // authorization-token
		hdrAuthSecret,       // authorization-secret
		hdrXOriginRequestId, // X-Origin-Request-Id
		hdrXTimestamp,       // X-Timestamp
		hdrXApiKey,          // @v11.9.11 X-API-KEY 
	};
	enum AuthType {
		authtUnkn = 0,
		authtBasic
	};
	struct Auth {
		int    Type; // authtXXX
		SString Login;
		SString Password;
	};

	static bool FASTCALL GetHeaderTitle(int hdr, SString & rTitle);
	static int  FASTCALL GetHeaderId(const char * pTitle);
	static bool SetHeaderField(StrStrAssocArray & rFldList, int titleId, const char * pValue);
	static bool IsThereHeaderField(const StrStrAssocArray & rFldList, int titleId, SString * pValue);
	static uint FASTCALL PutHeaderFieldsIntoString(const StrStrAssocArray & rFldList, SString & rBuf);
	static int  FASTCALL ParseAuth(const char * pAuthParam, Auth & rResult);
};
//
//
//
typedef void (*MailCallbackProc)(const IterCounter & bytesCounter, const IterCounter & msgCounter);

struct SMailMessage : SStrGroup {
	enum {
		fldFrom = 1,
		fldTo,
		fldSubj,
		fldBoundary,
		fldText,
		fldCc,    // Адреса для копий письма
		fldMailer //
	};
	enum {
		fPpyOrder  = 0x0001, // Заказ Albatros
		fPpyObject = 0x0002, // Данные передачи между разделами
		fMultipart = 0x0004, //
		fPpyCharry = 0x0008, // Объекты Charry
		fFrontol   = 0x0010  // Файл с данными с кассового модуля Фронтол
	};
	struct ContentDispositionBlock {
		static int FASTCALL GetTypeName(int t, SString & rBuf);
		static int FASTCALL IdentifyType(const char * pTypeName);

		ContentDispositionBlock();
		void   Destroy();

		enum {
			tUnkn = 0,
			tAttachment, // attachment
			tInline, // inline
			tFormData, // form-data
			tSignal, // signal
			tAlert, // alert
			tIcon, // icon
			tRender, // render
			tRecipientListHistory, // recipient-list-history
			tSession, // session
			tAIB, // aib
			tEarlySession, // early-session
			tRecipientList, // recipient-list
			tNotification, // notification
			tByReference, // by-reference
			tInfoPackage, // info-package
			tRecordingSession // recording-session
		};
		int    Type; // ContentDispositionBlock::tXXX
		uint   NameP;
		uint   FileNameP;
		uint64 Size;
		LDATETIME ModifDtm; // modification-date
		LDATETIME CrDtm;    // creation-date
		LDATETIME RdDtm;    // read-date
	};
	struct ContentTypeBlock {
		ContentTypeBlock();
		void   Destroy();
		uint   MimeP;
		uint   TypeP;
		uint   NameP;
		uint   BoundaryP; // Позиция идентификатора (применяется ко всем внутренним элементам 1-го уровня,
			// если внутренний элемент имеет собственное вложение, то его BoundaryP != 0).
		SCodepageIdent Cp;
	};
	struct Boundary {
		Boundary();
		void   Destroy();
		uint   GetAttachmentCount() const;
		const  Boundary * FASTCALL GetAttachmentByIndex(uint idx /*0..*/) const;

		ContentTypeBlock Ct;
		ContentDispositionBlock Cd;
		int    ContentTransfEnc; // SFileFormat::cteXXX Content-Transfer-Encoding:
		uint   ContentIdP;    // "Content-ID"
		uint   ContentDescrP; // "Content-Description"
		uint   LineNo_Start;  // @debug
		uint   LineNo_Finish; // @debug
		uint   OuterFileNameP;
		Boundary * P_Parent;  // @notowned
		SBuffer Data;
		TSCollection <Boundary> Children;
	private:
		const SMailMessage::Boundary * FASTCALL Helper_GetAttachmentByIndex(int & rIdx /*0..*/) const;
	};
	SMailMessage();
	~SMailMessage();
	void   Init();
	int    IsPpyData() const;
	// @v10.0.0 SString & MakeBoundaryCode(SString & rBuf) const;
	//
	// Parameters:
	//     start: 0 - pure boundary, 1 - start boundary, 2 - finish boundary
	// Returns:
	//     pBuf - on success, 0 - on error
	//
	SString & GetBoundary(int start, SString & rBuf) const;
	int    AttachFile(const char * pFilePath);
	//
	// Descr: Вставляет в письмо inline-содержание. Содержание вставляется как внутренняя область
	//   Boundary pB. Если pB == 0, то как внутренняя область Boundary верхнего уровня.
	// Returns:
	//   0 - error
	//   !0 - указатель на добавленную Boundary
	//
	Boundary * AttachContent(Boundary * pB, int format, SCodepageIdent cp, const void * pData, size_t dataSize);
	//
	// Descr: Вставляет в письмо прикрепленный файл. Файл вставляется как внутренняя область
	//   Boundary pB. Если pB == 0, то как внутренняя область Boundary верхнего уровня.
	// Returns:
	//   0 - error
	//   !0 - указатель на добавленную Boundary
	//
	Boundary * AttachFile(Boundary * pB, int format, const char * pFilePath);
	int    EnumAttach(uint *, SString & rFileName, SString & rFullPath);
	int    SetField(int fldId, const char *);
	int    FASTCALL IsField(int fldId) const;
	SString & GetField(int fldId, SString & rBuf) const;
	int    CmpField(int fldId, const char * pStr, size_t len = 0) const;
	//
	// Descr: Определяет, является ли адрес pEmail адресом отправителя сообщения.
	//   Сравнение осуществляется строго по формальному адресу с исключением описательной части.
	//   Например IsFrom("nemo@gmail.com") даст положительный результат на адрес "Капитан Немо <nemo@gmail.com>"
	//
	int    IsFrom(const char * pEmail) const;
	int    IsSubj(const char * pSubj, int substr) const;
	// @v10.0.0 int    PutToFile(SFile & rF);
	int    ReadFromFile(SFile & rF);
	int    DebugOutput(SString & rBuf) const;
	uint   GetAttachmentCount() const;
	int    SaveAttachmentTo(uint attIdx, const char * pDestPath, SString * pResultFileName) const;
	const  SMailMessage::Boundary * FASTCALL GetAttachmentByIndex(uint attIdx) const;
	int    GetAttachmentFileName(const SMailMessage::Boundary * pB, SString & rFileName) const;
	int    PreprocessEmailAddrString(const SString & rSrc, SString & rResult, StringSet * pSs) const;

	long   Flags;
	long   Size;

	struct WriterBlock {
		WriterBlock(const SMailMessage & rMsg);
		~WriterBlock();
		int    Read(size_t maxChunkSize, SBuffer & rBuf);

		enum {
			phsUndef = 0,
			phsHeader,
			phsBoundaryHeader,
			phsBoundaryBody,
			phsStop
		};
		int    Phase;
		const SMailMessage & R_Msg;
		const Boundary * P_Cb; // Текущая Boundary
		uint32 RdDataOff; // Используется на фазе phsBoundaryBody - смещение, с которого следует читать следующую порцию данных
		SFile * P_InStream;
	};
private:
	struct ParserBlock {
		ParserBlock();
		void   Destroy();
		enum {
			stHeader = 0,
			stBody,
			stMimePartHeader,
			stMimePartBody,
		};
		int    State;
		uint   LineNo;
		Boundary * P_B; // @notowned
	};

	static int IsFieldHeader(const SString & rLineBuf, const char * pHeader, SString & rValue);
	SString & PutField(const char * pFld, const char * pVal, SString & rBuf);
	int    ProcessInputLine(ParserBlock & rBlk, const SString & rLineBuf);
	Boundary * FASTCALL SearchBoundary(const SString & rIdent);
	Boundary * Helper_SearchBoundary(const SString & rIdent, Boundary * pParent);
	const Boundary * FASTCALL SearchBoundary(const Boundary * pB) const;
	const Boundary * FASTCALL Helper_SearchBoundary(const Boundary * pB, const Boundary * pParent) const;
	Boundary * Helper_CreateBoundary(SMailMessage::Boundary * pParent, int format);
	int    DebugOutput_Boundary(const Boundary & rB, uint tab, SString & rBuf) const;

	char   Zero[8];

	struct HdrFldPositions {
		uint   FromP;
		uint   ToP;
		uint   CcP;
		uint   SubjP;
		uint   MailerP;
		uint   MsgIdP; // Message-ID:
		uint   UserAgentP;
		uint   OrganizationP;
		uint   ReturnPathP;
		uint   DeliveredToP;
		uint   ReplyToP;
		uint   ContentLangP; // Content-Language:
		uint   AcceptLangP;  // Accept-Language:
		uint   XOrgIpP;      // x-originating-ip
		LDATETIME Dtm; // Date:
		SVerT  MimeVer;
	};
	HdrFldPositions HFP;
	Boundary B; // Заголовочный список тел сообщения
	TSVector <uint> AttachPosL;
	TSVector <uint> ReceivedChainL; // Received:
};
//
//
//
class ScURL {
public:
	class HttpForm {
	public:
		friend class ScURL;

		enum {
			tCopyName = 1,
			tPtrName,
			tCopyContents,
			tPtrContents,
			tContentsLength,
			tFileContent,
			tFile,
			tContentType,
			tFileName,
			tBuffer,
			tBufferPtr,
			tBufferLength,
			tStream,
			tArray,
			tContentHeader
		};
		HttpForm(ScURL & rC);
		~HttpForm();
        //int    Add(int tagId, void * pData);
        int    AddContentFile(const char * pFileName, const char * pContentType, const char * pContentName);
		const  SString & GetContentType() const { return ContentType; }
	private:
		ScURL & R_C;
		SString ContentType; // @v12.2.6
		void * FH;
		void * LH;
		void * P_CMime; // @v12.2.6 curl_mime
	};
    ScURL();
    ~ScURL();
	bool   operator !() const { return (H == 0); }
	void * GetHandle() { return H; }
	enum {
		authServer = 1
	};
	int    SetAuth(int auth, const char * pUser, const char * pPassword);
	void   SetLogFileName(const char * pFileName);
	int    SetupDefaultSslOptions(const char * pCertFilePath, int sslVer /* SSystem::sslXXX */, const StringSet * pSsCipherList);
	int    SetupSslCert(const char * pCertFile, const char * pKeyCertFile);

	enum {
		mfDontVerifySslPeer = 0x0001, // Устанавливает опцию CURLOPT_SSL_VERIFYPEER в FALSE
		mfTcpKeepAlive      = 0x0002, // Устанавливает опцию CURLOPT_TCP_KEEPALIVE в TRUE
		mfNoProgerss        = 0x0004, // Устанавливает опцию CURLOPT_NOPROGRESS в TRUE
		mfVerbose           = 0x0008  // Подробный вывод в файл журнала (требуется предварительный вызов SetLogFileName())
	};

    int    HttpPost(const char * pUrl, int mflags, HttpForm & rForm, SFile * pReplyStream);
    int    HttpPost(const char * pUrl, int mflags, const StrStrAssocArray * pFields, SFile * pReplyStream);
	int    HttpPost(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, HttpForm & rForm, SFile * pReplyStream);
	int    HttpPost(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const char * pBody, SFile * pReplyStream);
	int    HttpPut(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, const char * pBody, SFile * pReplyStream);
    int    HttpGet(const char * pUrl, int mflags, SFile * pReplyStream);
    int    HttpGet(const char * pUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream);
	int    HttpGet(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream);
	int    HttpDelete(const char * pUrl, int mflags, SFile * pReplyStream);
	int    HttpPatch(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHdr, const StrStrAssocArray * pFields, SFile * pReplyStream);
	int    HttpPatch(const InetUrl & rUrl, int mflags, const StrStrAssocArray * pHdr, const char * pBody, SFile * pReplyStream);
	int    FtpList(const InetUrl & rUrl, int mflags, SFileEntryPool & rPool);
	//
	// Descr: Отправляет локальный файл pLocalFile на ftp-сервер по url заданному параметром rUrl.
	//   rUrl не должен содержать имени файла: конечное имя файла, с которым локальный файл будет скопирован на ftp
	//   определяется либо параметром pDestFileName, либо, если pDestFileName пустой, именем локального файла pLocalFile.
	//   pDestFileName, если задан, то должен быть в кодировке utf-8.
	//
	int    FtpPut(const InetUrl & rUrl, int mflags, const char * pLocalFile, const char * pDestFileNameUtf8, SDataMoveProgressInfo * pProgress);
	int    FtpGet(const InetUrl & rUrl, int mflags, const char * pLocalFile, SString * pResultFileName, SDataMoveProgressInfo * pProgress);
	int    FtpDelete(const InetUrl & rUrl, int mflags);
	int    FtpChangeDir(const InetUrl & rUrl, int mflags);
	int    FtpCreateDir(const InetUrl & rUrl, int mflags);
	int    FtpDeleteDir(const InetUrl & rUrl, int mflags);
	int    Pop3List(const InetUrl & rUrl, int mflags, LAssocArray & rList); // LIST
	int    Pop3Top(const InetUrl & rUrl, int mflags, uint msgN, uint maxLines, SMailMessage & rMsg); // TOP
	int    Pop3Get(const InetUrl & rUrl, int mflags, uint msgN, SMailMessage & rMsg, SDataMoveProgressInfo * pProgress);  // RETR
	int    Pop3Delete(const InetUrl & rUrl, int mflags, uint msgN); // DELE
	int    SmtpSend(const InetUrl & rUrl, int mflags, const SMailMessage & rMsg);
	static size_t CbWrite(char * pBuffer, size_t size, size_t nmemb, void * pExtra);
private:
	static int    ComposeFieldList(const StrStrAssocArray * pFields, SString & rBuf, uint * pCount);
	static void * ComposeHeaderList(const StrStrAssocArray * pHttpHeaderFields);
	static size_t CbRead(char * pBuffer, size_t size, size_t nitems, void * pExtra);
	//@erikTEMp/*static size_t CbWrite(char * pBuffer, size_t size, size_t nmemb, void * pExtra);*/
	static int    CbProgress(void * extraPtr, int64 dltotal, int64 dlnow, int64 ultotal, int64 ulnow);
	static int _GlobalInitDone;
	int    FASTCALL SetError(int errCode);
	int    SetupCbRead(SFile * pF);
	int    SetupCbWrite(SFile * pF);
	int    SetupCbProgress(SDataMoveProgressInfo * pProgress);
	void   CleanCbRW();
	int    SetCommonOptions(int mflags, int bufferSize, const char * pUserAgent);
	int    SetHttpFormOptions(HttpForm & rF);
	int    Execute();

	struct InnerUrlInfo {
		SString User;
		SString Password;
		SString Path;
	};
	//
	// Descr: Функция осуществляет препроцессинг структуры InetUrl результаты
	//   которого заносит в специализированный блок InetUrlInfo.
	//   Исходная структура может быть изменена функцией для корректного использования
	//   методами cURL.
	// ARG(rUrl    IN/OUT): Структура для препроцессинга
	// ARG(defaultProt IN): Если в rUrl нет информации о протоколе, то defaultProt (если не 0)
	//   будет использован для уточнения.
	//   Кроме того, если defaultProt == InetUrl::protFtp или InetUrl::protHttp, то
	//   значение протокола, извлеченного из rUrl проверяется на принадлежность соответствующему
	//   семейству протоколов (FTP, FTPS, TFTP), (HTTP, HTTPS).
	// ARG(rInfo   OUT): Блок со специфической информацией об rUrl, используемый
	//   методами данного класса.
	//
	int    PrepareURL(InetUrl & rUrl, int defaultProt, InnerUrlInfo & rInfo);

	SFile  NullWrF; // Файл-заглушка для записи того, что не важно
	void * H;
	SString LogFileName;
	SFile * P_LogF;
};
//
// Descr: Класс реализующий максимально простой интерфейс для копирования файла с одного URL на другой.
//
class SUniformFileTransmParam : public SStrGroup {
public:
	SUniformFileTransmParam();
	int    Run(SDataMoveProgressProc pf, void * extraPtr);

	enum {
		fRenameExistantFiles = 0x0001,
		fDeleteAfter = 0x0002
	};
	long   Flags;
	int    Format; // SFileFormat::XXX
	uint   Pop3TopMaxLines; // @v9.9.9 Количество строк, которые следует извлекать командой POP3 TOP для анализа сообщения.
		// Если Pop3TopMaxLines == 0 или > 1000, то извлекается весь заголовок сообщения.
		// Экспериментально установлено, что явно заданное количество строк ускоряет время выполнения
		// функции по сравнению с 0.
	SString SrcPath;
	SString DestPath;
	SString AccsName;
	SString AccsPassword;
	SString Reply; // @out
	//
	// @construction
	// Descr: Стрктура результата копирования одного файла.
	//   Поля с суффиксом 'P' представляют позицию строки во внутреннме пуле экземпляра.
	//
	struct ResultItem {
		ResultItem();
		uint   SrcPathP;  // Путь исходного файла
		uint   DestPathP; // Путь результирующего файла
		uint   SrcMimeP;  // MIME исходного файла
		uint   DestMimeP; // MIME результирующего файла
		uint   RmvSrcCookieP;  // Специальная строка, которая может быть использована как ключ для последующего удаления исходного файла
		uint   RmvDestCookieP; // Специальная строка, которая может быть использована как ключ для последующего удаления результирующего файла
		uint64 Size; // Размер скопированного файла
	};
	TSVector <ResultItem> ResultList;
};
//
//
//
#if 0 // {

class SFtpClient {
public:
	enum {
		stConnected = 0x0001
	};
	SFtpClient();
	~SFtpClient();
	int    Connect(const char *);
	void   Disconnect();
	int    PutFile(const char * pLocSrc, const char * pFtpDst);
	int    GetFile(const char * pFtpSrc, const char * pLocDst);

    class Enum : public SEnum::Imp {
	public:
		Enum(const char * pWildcard);
		virtual ~Enum();
        virtual int Next(void * pDirEntry);
	private:
        void * P;
    };
	int    DeleteFile(const char * pFtpFile);
	int    RenameFile(const char * pFtpFileName, const char * pNewFtpFileName);
private:
	long   State;
    SString Login;
	SString Password;
	void * H_Conn;
	void * H_Sess;
	//HINTERNET HInternet;
    //HINTERNET HFtpSession;
	//HINTERNET HFtpFind;
};

#endif // } 0

#endif // __SNET_H
