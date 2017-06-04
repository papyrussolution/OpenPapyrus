// SNET.H
// Copyright (c) A.Sobolev 2005, 2007, 2010, 2012, 2013, 2014, 2015
//
#ifndef __SNET_H
#define __SNET_H

#include <slib.h>
#ifdef _WIN32_WINNT
	#undef _WIN32_WINNT
#endif
#include <ws2tcpip.h>
//
//
//
struct MACAddr { // size=6
	void   SLAPI Init();
	int    SLAPI IsEmpty() const;
	SString & FASTCALL ToStr(SString & rBuf) const;
	//
	// Descr: сравнивает MAC-адреса this и s.
	//   Сравнение осуществляется побайтно, начиная с байта Addr[0]
	// Returns:
	//   0 - адреса this и s эквивалентны
	//  <0 - адрес this меньше адреса s
	//  >0 - адрес this больше адреса s
	//
	int    FASTCALL Cmp(const MACAddr & s) const;
	uint8  Addr[6];
};

class MACAddrArray : public TSArray <MACAddr> {
public:
	SLAPI  MACAddrArray();
	int    SLAPI addUnique(const MACAddr &);
};

int SLAPI GetFirstMACAddr(MACAddr *);
int SLAPI GetMACAddrList(MACAddrArray *);
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
	static ulong SLAPI IPToULong(const char * pIP);
	static int SLAPI ULongToIP(ulong ip, SString & rIP);
	static int SLAPI GetNameByAddr(const char * pIP, SString & aHost);

	SLAPI  InetAddr();
	SLAPI  operator ulong() const { return V4; }
	InetAddr & SLAPI Clear();
	int    FASTCALL IsEqual(const InetAddr & rS) const;
	int    FASTCALL operator == (const InetAddr & rS) const;
	int    FASTCALL operator != (const InetAddr & rS) const;
	int    SLAPI Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx);
	int    SLAPI IsEmpty() const;
	int    SLAPI GetPort() const { return Port; }
	const SString & SLAPI GetHostName() const { return HostName; }
	int    SLAPI Set(ulong addr, int port = 0);
	int    SLAPI Set(const char * pHostName, int port = 0);
	int    SLAPI Set(const sockaddr_in *);
	int    SLAPI SetPort(int port);
	sockaddr * SLAPI Get(sockaddr_in *) const;
	SString & SLAPI ToStr(long flags /* InetAddr::fmtXXX */, SString & rBuf) const;
	int    SLAPI FromStr(const char *);
private:
	uint32 V4;
	SString HostName;
	int    Port;
};

int SLAPI GetFirstHostByMACAddr(MACAddr * pItem, InetAddr * pAddr);
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

	SLAPI  TcpSocket(int timeout = 0, int maxConn = SOMAXCONN);
	SLAPI ~TcpSocket();
	int    SLAPI IsValid() const;
	operator SOCKET () const
	{
		return S;
	}
	//
	// Descr:
	//
	int    SLAPI CheckErrorStatus(); // @>>::getsockopt(S, SOL_SOCKET, SO_ERROR,...)
	int    SLAPI GetTimeout() const;
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
	int    SLAPI MoveToS(TcpSocket & rDest, int force = 0); // @debug force = 0
	int    SLAPI CopyS(TcpSocket & rDest);
	int    SLAPI Connect(const InetAddr &);
	int    SLAPI Connect(SslMode sslm, const InetAddr &);
	int    SLAPI Bind(const InetAddr &);
	int    SLAPI GetSockName(InetAddr * pAddr, int peer);
	int    SLAPI Disconnect();
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
	int    SLAPI Select(int mode /* TcpSocket::mXXX */, int timeout = -1, size_t * pAvailableSize = 0);
		// @>>::select
	int    SLAPI Listen(); // @>>::listen
	int    SLAPI Accept(TcpSocket *, InetAddr *); // @>>::accept
	//
	// Descr: Читает из сокета данные в буфер pBuf размер которого ограничен
	//   величиной bufLen. По указателю pRcvdSize присваивается количество
	//   прочитанных байт.
	//
	int    SLAPI Recv(void * pBuf, size_t bufLen, size_t * pRcvdSize); // @>>::recv
	//
	// Descr: Читает из сокета блок данных длиной size в буфер pBuf.
	//   Отличается от функции Recv тем, что читает в цикле до тех пор, пока не
	//   получит требуемое количество байт, либо ошибку, либо очередное
	//   считывние не вернет 0 байт.
	//
	int    SLAPI RecvBlock(void * pBuf, size_t size, size_t * pRcvdSize);
	//
	// Descr: Записывает в сокет данные из буфера pBuf в количестве dataLen байт.
	//   По указателю pSendedSize возвращается количество действительно переданных
	//   данных.
	//
	int    SLAPI Send(const void * pBuf, size_t dataLen, size_t * pSendedSize); // @>>::send
	int    SLAPI RecvBuf(SBuffer & rBuf, size_t frameSize, size_t * pRcvdSize);
	//
	// Descr: Считывает из сокета данные в буфер rBuf до тех пор, пока не
	//   встретится терминальная последовательность pTerminator.
	//   Если pTerminator == 0 || strlen(pTerminator) == 0, то вызывает RecvBuf(rBuf, 0, pRcvdSize)
	//
	int    SLAPI RecvUntil(SBuffer & rBuf, const char * pTerminator, size_t * pRcvdSize);
	int    SLAPI SendBuf(SBuffer & rBuf, size_t * pSendedSize);
	int    SLAPI GetStat(long * pRdCount, long * pWrCount);
#ifdef SLTEST_RUNNING // SLTEST_RUNNING {
	int    SLAPI BreakSocket() // Прервать связь без отсоединения //
	{
		Reset();
		return 1;
	}
#endif  // } SLTEST_RUNNING
private:
	static size_t DefaultReadFrame;  // Размер кванта считывания из сокета по умолчанию //
	static size_t DefaultWriteFrame; // Размер кванта записи в сокет //

	int    SLAPI Init(SOCKET s);
	void   SLAPI Reset();
	int    SLAPI Helper_Connect(SslMode sslm, const InetAddr & rAddr);
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

		int    operator !() const
		{
			return !IsValid();
		}
		int    IsValid() const
		{
			return (P_Ctx && P_S);
		}
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
//
//
class TcpServer : private TcpSocket {
public:
	SLAPI  TcpServer(const InetAddr & rAddr);
	virtual SLAPI ~TcpServer();
	int    SLAPI Run();
	virtual int SLAPI ExecSession(TcpSocket & rSock, InetAddr & rAddr);
		// @<<TcpServer::Run
private:
	InetAddr Addr;
};
//
//
//
class MailSession {
public:
	MailSession(SOCKET s, struct sockaddr_in r);
	//
	// Create a session to a remote host and port. This function reads a timeout value from the ArgvMap class
	// and does a nonblocking connect to support this timeout. It should be noted that nonblocking connects
	// suffer from bad portability problems, so look here if you see weird problems on new platforms
	//
	MailSession(const char * pRemote, int port, int timeout = 0);
	MailSession(u_long ip, int port, int timeout = 0);
	~MailSession();
	int    getLine(SString & rBuf);
	int    haveLine(); //!< returns true if a line is available
	int    putBuffer(const void * pBuf, size_t bufLen);
	int    putLine(const char *); //!< Write a line to the remote
	int    timeoutRead(int s, char *buf, size_t len, size_t * pRdBytes);
	int    isError() const { return Err; }
	int    close(); //!< close and disconnect the connection
	void   setTimeout(int seconds);
private:
	int    doConnect(u_long ip, int port);
	int    init();

	int    Err;
	size_t RealBufSize;
	char * P_Buf;
	size_t BufSize;
	size_t RdOffs;
	size_t WrOffs;
	SOCKET clisock;
	struct sockaddr_in remote;
	int    Timeout;
};
//
//
//
class InetUrl : public InetAddr {
public:
	enum { // @persistent
		protUnkn     =  0,
		protHttp     =  1,  // http
		protHttps    =  2,  // https
		protFtp      =  3,  // ftp
		protGopher   =  4,  // gopher
		protMailto   =  5,  // mailto
		protNews     =  6,
		protNntp     =  7,
		protIrc      =  8,
		protProspero =  9,
		protTelnet   = 10,
		protWais     = 11,
		protXmpp     = 12,
		protFile     = 13,
		protData     = 14,
		protSvn      = 15,
		protSocks4   = 16,
		protSocks5   = 17,
		protSMTP     = 18, // Протокол отправки почты
		protSMTPS    = 19, // Протокол отправки почты (SSL)
		protPOP3     = 20, // Протокол получения почты
		protPOP3S    = 21, // Протокол получения почты (SSL)
		protIMAP     = 22, // Internet Message Access Protocol
		protIMAPS    = 23, // Internet Message Access Protocol (SSL)
		protFtps     = 24, // ftps
		protTFtp     = 25,
		protDict     = 26,
		protSSH      = 27,
		protSMB      = 28,
		protSMBS     = 29,
		protRTSP     = 30,
		protRTMP     = 31,
		protRTMPT    = 32,
		protRTMPS    = 33,
		protLDAP     = 34,
		protLDAPS    = 35
	};
	enum {
		cScheme = 1,
		cUserName,
		cPassword,
		cHost,
		cPort,
		cPath,
		cQuery,
		cRef
	};
	enum {
		stError    = 0x80000000,
		stEmpty    = 0x40000000,
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

	InetUrl(const char * pUrl = 0);
	InetUrl & Clear();
	long   GetState() const;
	int    Valid() const;
	int    IsEmpty() const;
	int    Parse(const char * pUrl);
	int    GetComponent(int c, SString & rBuf) const;
	int    GetProtocol() const;
	int    SetProtocol(int protocol);
private:
	int    Protocol;
	StrAssocArray TermList;
	SString Org;
	long   State;
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
		Capability()
		{
			SmtpMaxSize = 0;
		}
		Capability & Reset()
		{
			SmtpMaxSize = 0;
			SmtpServerName = 0;
			SmtpAuthTypeList.clear();
			return *this;
		}
        int64  SmtpMaxSize;
        SString SmtpServerName;
        LongArray SmtpAuthTypeList;
	};

	static SString & FASTCALL Pop3_SkipReplyStatus(SString & rBuf);

	SLAPI  SMailClient();
	SLAPI ~SMailClient();
	int    SLAPI Connect(InetUrl & rUrl, int timeout = -1);
	int    SLAPI Disconnect();
	const  Capability & SLAPI GetCapability() const
	{
		return C;
	}
	int    SLAPI Auth(int alg, const char * pName, const char * pPassword);
	//
	// Descr: Считывает из сокета одну или более строк.
	//   Первая строка всегда считывается в буфер rBuf. Если pTail != 0, то
	//   остальные строке, если имеются в сокете, считываются как последовательные элементы
	//   pTail.
	// Note: Функция отрезает терминальные символы перевода каретки (\xD\xA) с конца считанных строк.
	//
	int    SLAPI ReadLine(SString & rBuf);
	int    SLAPI CheckReply(const SString & rReplyBuf, int onlyValidCode = 0);
	//
	// Descr: Записывает в сокет строку pBuf. Если параметр pReply != 0, то
	//   сразу после успешной записи считывает первую строку ответа сервера в pReply.
	// Note: Строка pBuf не должна иметь терминального перевода каретки (\xD\xA). Функция
	//   WriteLine самостоятельно добавляет терминатор к строке аргумента.
	//
	int    SLAPI WriteLine(const char * pBuf, SString * pReply);
	int    SLAPI WriteBlock(const void * pData, size_t dataSize);

	int    SLAPI Pop3_GetStat(long * pCount, long * pSize);
	int    SLAPI Pop3_GetMsgSize(long msgN, long * pSize);
	int    SLAPI Pop3_DeleteMsg(long msgN);
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
class SProxiAuthParam {
public:
	struct Entry {
		Entry();
		int32  Protocol; // InetUrl::protXXX
		int32  Mode;     // SProxiAuthParam::kXXX
		long   Flags;
		InetAddr Addr;
		SString  UserName;
		SString  Password;
	};

	SProxiAuthParam();
	SProxiAuthParam & Clear();
	int    SetEntry(SProxiAuthParam::Entry & rEntry);
	int    GetEntry(int protocol, SProxiAuthParam::Entry & rEntry) const;
	int    ToStr(long fmt, SString & rBuf) const;
	int    FromStr(long fmt, const char * pStr);

	enum {
		kNone = 0,
		kAuto,
		kSys,
		kManual
	};
	int32  Ver;
private:
	TSCollection <Entry> List;
};
//
//
//
enum SHtmlTag {

};
//
//
//
class SHttpClient {
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
		//
 		// Entity Header Fields
		//
 		// Entity-header fields define metainformation about the entity-body or, if no
 		// body is present, about the resource identified by the request. Some of this
 		// metainformation is OPTIONAL; some might be REQUIRED by portions of this
 		// specification. (see RFC2616 7.1)
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
		//
		// The response-header fields allow the server to pass additional information
		// about the response which cannot be placed in the Status-Line. These header
		// fields give information about the server and about further access to the
		// resource identified by the Request-URI. (see RFC2616)
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
		//
		// The request-header fields allow the client to pass additional information
		// about the request, and about the client itself, to the server. These fields
		// act as request modifiers, with semantics equivalent to the parameters on a
		// programming language method invocation (see RFC2616).
		//
		hdrAccept,         // "Accept"
		hdrAcceptCharset,  // "Accept-Charset"
		hdrAcceptEnc,      // "Accept-Encoding"
		hdrAcceptLang,     // "Accept-Language"
		hdrAuthorization,  // "Authorization"
		hdrExpect,         // "Expect"
		hdrFrom,           // "From"
		hdrHost,           // "Host"
		hdrIfMatch,        // "If-Match"
		hdrIfModifSince,   // "If-Modified-Since"
		hdrIfNonMatch,     // "If-None-Match"
		hdrIfRange,        // "If-Range"
		hdrIfUnmodifSince, // "If-Unmodified-Since"
		hdrMaxForwards,    // "Max-Forwards"
		hdrProxiAuth,      // "Proxy-Authorization"
		hdrRange,          // "Range"
		hdrReferer,        // "Referer"
		hdrTransferExt,    // "TE"
		hdrUserAgent,      // "User-Agent"
		//
		//
		//
		hdrSoapAction      // "SoapAction"
	};

	static int GetHeaderTitle(int hdr, SString & rTitle);
	static int GetHeaderId(const char * pTitle);

	struct Attachment {
	};
	struct Response : public SBuffer {
		Response();
		Response & Reset();

		enum {
			transftypUndef = 0,         // Undefined
			transftypContentLength = 1, // The stream cares about Content-length
			transftypChunked,           // The stream sends/receives chunked data
			transftypConnectionClose,   // The stream sends/receives data until connection is closed
			transftypFile,              // This transfer style will be used by MIME support and for debug purposes
		};
		uint16 HttpVerMj;
		uint16 HttpVerMn;
		int    ErrCode;
		int    TransferType;
		size_t ContentLength;
		long   State;
		SString Descr;
		StrAssocArray Header;
		Attachment Attch;
	};

	SHttpClient();
	~SHttpClient();
	int    SetHeader(int hdrTag, const char * pValue);
	void   ClearHeader();
	int    TolkToServer(int method, const char * pUrl);
	int    ReadResponse(Response & rRsp);
private:
	uint16 HttpVerMj;
	uint16 HttpVerMn;
	StrAssocArray Header;
	TcpSocket S;
	SBuffer Buffer;
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
		HttpForm();
		~HttpForm();
        //int    Add(int tagId, void * pData);
        int    AddContentFile(const char * pFileName, const char * pContentType, const char * pContentName);
	private:
		void * FH;
		void * LH;
	};
    ScURL();
    ~ScURL();
	int    operator !() const
	{
		return (H == 0);
	}
	enum {
		mfDontVerifySslPeer = 0x0001 // Устанавливает опцию CURLOPT_SSL_VERIFYPEER в FALSE
	};
    int    HttpPost(const char * pUrl, int mflags, HttpForm & rForm, SFile * pReplyStream);
    int    HttpPost(const char * pUrl, int mflags, const StrStrAssocArray * pFields, SFile * pReplyStream);
    int    HttpGet(const char * pUrl, int mflags, SFile * pReplyStream);
    int    HttpGet(const char * pUrl, int mflags, const StrStrAssocArray * pHttpHeaderFields, SFile * pReplyStream);
	int    HttpDelete(const char * pUrl, int mflags, SFile * pReplyStream);
private:
	static size_t CbRead(char * pBuffer, size_t size, size_t nitems, void * pExtra);
	static size_t CbWrite(char * pBuffer, size_t size, size_t nmemb, void * pExtra);
	static int _GlobalInitDone;

	int    FASTCALL SetError(int errCode);
	int    SetupCbRead(SFile * pF);
	int    SetupCbWrite(SFile * pF);

	void * H;
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

    class Enum : public SEnumImp {
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
