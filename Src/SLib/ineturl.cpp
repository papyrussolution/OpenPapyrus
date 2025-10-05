// INETURL.CPP
// Copyright (c) A.Sobolev 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

/*static*/ulong InetAddr::IPToULong(const char * pIP)
{
	ulong  addr = 0;
	SString buf;
	StringSet ss('.', pIP);
	THROW(ss.getCount() == 4);
	for(uint i = 0, j = 4; j > 0 && ss.get(&i, buf); j--) {
		const ulong elem = buf.ToLong();
		THROW(elem >= 0 && elem <= 255);
		addr |= (elem << (8 * (j - 1)));
	}
	CATCH
		addr = 0;
	ENDCATCH
	return addr;
}

/*static*/void InetAddr::ULongToIP(ulong ip, SString & rIP)
{
	rIP.Z();
	for(uint i = 4; i > 0; i--) {
		rIP.Cat((ip >> (8 * (i - 1))) & 0x000000FF);
		if(i != 1)
			rIP.Dot();
	}
}

InetAddr::InetAddr() : V4(0), Port(0)
{
}

InetAddr::InetAddr(const InetAddr & rS)
{
	Copy(rS);
}

InetAddr & FASTCALL InetAddr::operator = (const InetAddr & rS)
{
	Copy(rS);
	return *this;
}

void FASTCALL InetAddr::Copy(const InetAddr & rS)
{
	V4 = rS.V4;
	HostName = rS.HostName;
	Port = rS.Port;
}

InetAddr & InetAddr::Z()
{
	V4 = 0;
	HostName.Z();
	Port = 0;
	return *this;
}

bool   FASTCALL InetAddr::IsEq(const InetAddr & rS) const { return (V4 == rS.V4 && Port == rS.Port && HostName == rS.HostName); }
bool   FASTCALL InetAddr::operator == (const InetAddr & rS) const { return IsEq(rS); }
bool   FASTCALL InetAddr::operator != (const InetAddr & rS) const { return !IsEq(rS); }
bool   InetAddr::IsEmpty() const { return (V4 == 0 && HostName.IsEmpty()); }
int    InetAddr::Set(const sockaddr_in * pAddr) { return Set(pAddr->sin_addr.s_addr, pAddr->sin_port); }

int InetAddr::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, V4, rBuf));
	THROW(pSCtx->Serialize(dir, HostName, rBuf));
	THROW(pSCtx->Serialize(dir, Port, rBuf));
	CATCHZOK
	return ok;
}

SString & InetAddr::ToStr(long flags, SString & rBuf) const
{
	rBuf.Z();
	if(flags & fmtAddr || !(flags & (fmtAddr | fmtHost)))
		rBuf.Cat(V4 & 0xff).Dot().Cat((V4 >> 8) & 0xff).Dot().Cat((V4 >> 16) & 0xff).Dot().Cat((V4 >> 24) & 0xff);
	else if(flags & fmtHost)
		rBuf = HostName;
	if(!rBuf.NotEmptyS()) {
		if(HostName.NotEmpty())
			rBuf = HostName;
		else if(V4)
			rBuf.Cat(V4 & 0xff).Dot().Cat((V4 >> 8) & 0xff).Dot().Cat((V4 >> 16) & 0xff).Dot().Cat((V4 >> 24) & 0xff);
	}
	if(flags & fmtPort && Port > 0)
		rBuf.Colon().Cat(Port);
	return rBuf;
}

int InetAddr::FromStr(const char * pStr)
{
	int    ok = 1;
	if(pStr) {
		SString buf(pStr);
		SString addr_buf, port_buf;
		buf.Divide(':', addr_buf, port_buf);
		if(addr_buf.NotEmptyS())
			ok = Set(addr_buf, port_buf.ToLong());
		else {
			Port = port_buf.ToLong();
			ok = -1;
		}
	}
	else {
		V4 = 0;
		HostName.Z();
		Port = 0;
		ok = -1;
	}
	return ok;
}

int InetAddr::SetPort_(int port)
{
	const int preserve_port = Port;
	Port = port;
	return preserve_port;
}

sockaddr * InetAddr::Get(sockaddr_in * pAddr) const
{
	sockaddr_in addr;
	MEMSZERO(addr);
	addr.sin_family = AF_INET;
	addr.sin_port   = htons((ushort)Port);
	addr.sin_addr.s_addr = V4;
	ASSIGN_PTR(pAddr, addr);
	return reinterpret_cast<sockaddr *>(pAddr);
}

int InetAddr::Set(ulong addr, int port)
{
	V4 = addr;
	Port = port;
	return 1;
}

/*static*/int InetAddr::GetNameByAddr(const char * pIP, SString & aHost)
{
	int    ok = -1;
	if(pIP) {
		char ip[16], host[64];
		STRNSCPY(ip, pIP);
		struct sockaddr_in sin;
		sin.sin_family   = AF_INET;
		sin.sin_port     = htons(0);
		sin.sin_addr.S_un.S_addr = inet_addr(ip);
		memzero(host, sizeof(host));
		if(getnameinfo(reinterpret_cast<const sockaddr *>(&sin), sizeof(sin), host, sizeof(host), 0, 0, 0) == 0) {
			aHost = host;
			ok = 1;
		}
	}
	return ok;
}

int InetAddr::Set(const char * pHostName, int port)
{
	int    ok = 0;
	Port = port;
	if(pHostName) {
		/*
			typedef struct addrinfo {
				int ai_flags;
				int ai_family;
				int ai_socktype;
				int ai_protocol;
				size_t ai_addrlen;
				char * ai_canonname;
				struct sockaddr* ai_addr;
				struct addrinfo* ai_next;
			} addrinfo;
		 */
		addrinfo * p_ai = 0;
		addrinfo hint;
		MEMSZERO(hint);
		hint.ai_flags = AI_CANONNAME;
		hint.ai_family = PF_INET;
		hint.ai_protocol = 0;
		hint.ai_socktype = SOCK_STREAM;
		int    r = getaddrinfo(pHostName, 0, &hint, &p_ai);
		if(!r) {
			for(addrinfo * p = p_ai; p != 0; p = p->ai_next) {
				HostName = p->ai_canonname;
				V4 = reinterpret_cast<const sockaddr_in *>(p->ai_addr)->sin_addr.s_addr;
				ok = 1;
				break;
			}
		}
		freeaddrinfo(p_ai);
		if(!ok) {
			SString added_msg;
			added_msg.Cat(pHostName);
			if(port)
				added_msg.CatDiv(':', 0).Cat(port);
			SLS.SetError(SLERR_SOCK_HOSTRESLVFAULT, added_msg);
		}
	}
	return ok;
}
//
//
//
/*static*/int FASTCALL InetUrl::GetDefProtocolPort(int protocol)
{
	int    port = 0;
	switch(protocol) {
		case protHttp:     port = 80; break;
		case protHttps:    port = 443; break;
		case protFtp:      port = 21; break; // @v11.0.7 @fix sic! 20-->21
		case protGopher:   port = 70; break;
		case protSvn:      port = 3690; break;
		case protSMTP:     port = 25; break;
		case protSMTPS:    port = 465; break;
		case protPOP3:     port = 110; break;
		case protPOP3S:    port = 995; break;
		case protIMAP:     port = 143; break;
		case protIMAPS:    port = 993; break;
		case protFtps:     port = 990; break;
		case protTFtp:     port = 69; break;
		case protDict:     port = 2628; break;
		case protSSH:      port = 22; break;
		case protSMB:      port = 445; break;
		case protSMBS:     port = 445; break;
		case protRTSP:     port = 554; break;
		case protRTMP:     port = 1935; break;
		case protRTMPT:    port = 80; break;
		case protRTMPS:    port = 443; break;
		case protLDAP:     port = 389; break;
		case protLDAPS:    port = 636; break;
		case protAMQP:     port = 5672; break; // @v10.5.3
		case protAMQPS:    port = 5671; break; // @v10.5.3
		case prot_p_PapyrusServer: port = 28015; break;
		case prot_p_MYSQL: port = 3306; break; // @v10.9.2
	}
	return port;
}

/*static*/SString & FASTCALL InetUrl::Mk(int protocol, const char * pHost, const char * pPath)
{
	assert(!isempty(pHost));
	const char * p_scheme = protocol ? GetSchemeMnem(protocol) : "";
	SString & r_buf = SLS.AcquireRvlStr();
	if(p_scheme)
		r_buf.Cat(p_scheme).Colon().CatCharN('/', 2);
	r_buf.Cat(pHost);
	if(!isempty(pPath)) {
		if(pPath[0] != '/')
			r_buf.SetLastDSlash();
		r_buf.Cat(pPath);
	}
	return r_buf;
}

/*static*/int FASTCALL InetUrl::ValidateComponent(int c)
{
	return oneof8(c, cScheme, cUserName, cPassword, cHost, cPort, cPath, cQuery, cRef) ? 1 : SLS.SetError(SLERR_INVPARAM);
}

InetUrl::InetUrl(const char * pUrl) : InetAddr()
{
	Z();
	Parse(pUrl);
}

InetUrl::InetUrl(const InetUrl & rS)
{
	Copy(rS);
}

InetUrl & FASTCALL InetUrl::operator = (const InetUrl & rS)
{
	Copy(rS);
	return *this;
}

void FASTCALL InetUrl::Copy(const InetUrl & rS)
{
	InetAddr::Copy(rS);
	Protocol = rS.Protocol;
	TermList = rS.TermList;
	Org = rS.Org;
	State = rS.State;
}

bool InetUrl::IsValid() const { return !(State & stError); }
bool InetUrl::IsEmpty() const { return LOGIC(State & stEmpty); }

InetUrl & InetUrl::Z()
{
	InetAddr::Z();
	State = stEmpty;
	TermList.Z();
	Org.Z();
	Protocol = protUnkn;
	return *this;
}

#if 0 // @v11.1.11 {
static const char * SchemeMnem[] = {
	"",          // #0
	"http",      // #1
	"https",     // #2
	"ftp",       // #3
	"gopher",    // #4
	"mailto",    // #5
	"news",      // #6
	"nntp",      // #7
	"irc",       // #8
	"prospero",  // #9
	"telnet",  // #10
	"wais",    // #11
	"xmpp",    // #12
	"file",    // #13
	"data",    // #14
	"svn",     // #15
	"socks4",  // #16
	"socks5",  // #17
	"smtp",    // #18
	"smtps",   // #19
	"pop3",    // #20
	"pop3s",   // #21
	"imap",    // #22
	"imaps",   // #23
	"ftps",    // #24
	"tftp",    // #25
	"dict",    // #26
	"ssh",     // #27
	"smb",     // #28
	"smbs",    // #29
	"rtsp",    // #30
	"rtmp",    // #31
	"rtmpt",   // #32
	"rtmps",   // #33
	"ldap",    // #34
	"ldaps",   // #35
	"mailfrom", // #36 fixion
	"",         // #37 private PapyrusServer
	"amqp",     // #38
	"amqps",    // #39
	"mysql",    // #40 private
	"sqlite",   // #41 private
	"oracle",   // #42 private
	"git",      // #43 @v11.1.11
};
#endif // } 0 @v11.1.11

static const SIntToSymbTabEntry InetUrlSchemeList[] = {
	{ InetUrl::protUnkn,         "" },          // #0
	{ InetUrl::protHttp,         "http" },      // #1
	{ InetUrl::protHttps,        "https" },     // #2
	{ InetUrl::protFtp,          "ftp" },       // #3
	{ InetUrl::protGopher,		 "gopher" },    // #4
	{ InetUrl::protMailto,		 "mailto" },    // #5
	{ InetUrl::protNews,		 "news" },      // #6
	{ InetUrl::protNntp,         "nntp" },      // #7
	{ InetUrl::protIrc,          "irc" },       // #8
	{ InetUrl::protProspero,     "prospero" },  // #9
	{ InetUrl::protTelnet,		 "telnet" },  // #10
	{ InetUrl::protWais,		 "wais" },    // #11
	{ InetUrl::protXmpp,		 "xmpp" },    // #12
	{ InetUrl::protFile,         "file" },    // #13
	{ InetUrl::protData,         "data" },    // #14
	{ InetUrl::protSvn,          "svn" },     // #15
	{ InetUrl::protSocks4,		 "socks4" },  // #16
	{ InetUrl::protSocks5,		 "socks5" },  // #17
	{ InetUrl::protSMTP,		 "smtp" },    // #18
	{ InetUrl::protSMTPS,		 "smtps" },   // #19
	{ InetUrl::protPOP3,		 "pop3" },    // #20
	{ InetUrl::protPOP3S,		 "pop3s" },   // #21
	{ InetUrl::protIMAP,		 "imap" },    // #22
	{ InetUrl::protIMAPS,		 "imaps" },   // #23
	{ InetUrl::protFtps,		 "ftps" },    // #24
	{ InetUrl::protTFtp,		 "tftp" },    // #25
	{ InetUrl::protDict,		 "dict" },    // #26
	{ InetUrl::protSSH,          "ssh" },     // #27
	{ InetUrl::protSMB,          "smb" },     // #28
	{ InetUrl::protSMBS,         "smbs" },    // #29
	{ InetUrl::protRTSP,         "rtsp" },    // #30
	{ InetUrl::protRTMP,         "rtmp" },    // #31
	{ InetUrl::protRTMPT,        "rtmpt" },   // #32
	{ InetUrl::protRTMPS,        "rtmps" },   // #33
	{ InetUrl::protLDAP,         "ldap" },    // #34
	{ InetUrl::protLDAPS,        "ldaps" },   // #35
	{ InetUrl::protMailFrom,     "mailfrom" }, // #36 fixion
	{ InetUrl::prot_p_PapyrusServer, "" },         // #37 private PapyrusServer
	{ InetUrl::protAMQP,         "amqp" },     // #38
	{ InetUrl::protAMQPS,        "amqps" },    // #39
	{ InetUrl::prot_p_MYSQL,     "mysql" },    // #40 private // @v10.9.2
	{ InetUrl::prot_p_SQLITE,    "sqlite" },   // #41 private // @v10.9.2
	{ InetUrl::prot_p_ORACLE,    "oracle" },   // #42 private // @v10.9.2
	{ InetUrl::protGit,          "git" },      // #43 // @v11.1.11
};

/*static*/const char * FASTCALL InetUrl::GetSchemeMnem(int schemeId)
{
	// @v11.1.11 return (schemeId >= 0 && schemeId < SIZEOFARRAY(SchemeMnem)) ? SchemeMnem[schemeId] : SchemeMnem[0];
	return SIntToSymbTab_GetSymbPtr(InetUrlSchemeList, SIZEOFARRAY(InetUrlSchemeList), schemeId); // @v11.1.11
}

/*static*/int FASTCALL InetUrl::GetSchemeId(const char * pSchemeMnem)
{
	/* @v11.1.11 for(uint i = 0; i < SIZEOFARRAY(SchemeMnem); i++) {
		if(sstreqi_ascii(pSchemeMnem, SchemeMnem[i]))
			return static_cast<int>(i);
	}
	return protUnkn;*/
	return SIntToSymbTab_GetId(InetUrlSchemeList, SIZEOFARRAY(InetUrlSchemeList), pSchemeMnem); // @v11.1.11
}

int InetUrl::Parse(const char * pUrl)
{
	// http://<host>:<port>/<context>
	// <схема>://<логин>:<пароль>@<хост>:<порт>/<URL-путь>?<параметры>#<якорь>
	//
	// Divisors: :, :/, ://, :\, :\\, /, \, ?, #, @

	int    ok = 1;
	SString _url(pUrl);
	Z();
	_url.Strip();
	if(!_url.IsLegalUtf8()) {
		State |= stError;
		ok = SLS.SetError(SLERR_URL_NOTUTF8, _url.Trim(64)); // @v11.8.10
	}
	else {
		while(oneof2(_url.C(0), ' ', '\t'))
			_url.ShiftLeft();
		if(_url.C(0) == '\"') {
			_url.ShiftLeft();
			while(oneof2(_url.C(0), ' ', '\t'))
				_url.ShiftLeft();
			size_t qp = 0;
			if(_url.SearchChar('\"', &qp))
				_url.Trim(qp).Strip();
		}
		if(_url.NotEmptyS()) {
			SString temp_buf;
			SString left_buf, right_buf;
			int    _done = 0;
			{
				//
				// Сначала разберем специальные случаи, которые не обрабатываются функцией UriParseUri
				// Note:
				//   Этот блок требует значительных уточнений, в том числе касательно очень специальных случаев
				//   вроде \\.\COM1
				//
				STokenRecognizer tr;
				SNaturalTokenArray nta;
				SNaturalTokenStat nts;
				//uint tokn = 0;
				//tr.Run(reinterpret_cast<const uchar *>(pCode), sstrlen(pCode), nta, &nts);
				//if(nts.Seq & SNTOKSEQ_ASCII && nts.Len >= 25) ok = 100000;
				tr.Run(_url, nta, &nts);
				if(nta.Has(SNTOK_IP4)) {
					Protocol = 0;
					TermList.Add(cHost, _url);
					_done = 1;									
				}
				else if(_url.IsEqiAscii("localhost")) {
					Protocol = 0;
					TermList.Add(cHost, _url);
					_done = 1;					
				}
				else if(!_url.Search("://", 0, 0, 0) && _url.Divide(':', left_buf, right_buf) > 0) {
					if(left_buf.Len() == 1 && isasciialpha(left_buf.C(0))) {
						// Путь файловой системы
						Protocol = GetSchemeId("file");
						TermList.Add(cScheme, "file", 1);
						TermList.Add(cPath, _url, 1);
						State &= ~stEmpty;
						_done = 1;
					}
					else if(right_buf.IsDec()) {
						tr.Run(left_buf.ucptr(), left_buf.Len(), nta, &nts);
						if(nta.Has(SNTOK_IP4)) {
							Protocol = 0;
							TermList.Add(cHost, left_buf);
							TermList.Add(cPort, right_buf);
							_done = 1;					
						}
						else if(left_buf.IsEqiAscii("localhost")) {
							Protocol = 0;
							TermList.Add(cHost, left_buf);
							TermList.Add(cPort, right_buf);
							_done = 1;					
						}
					}
				}
				if(!_done) {
					if(_url[1] == ':' && isasciialpha(_url[0])) {
						// Путь файловой системы
						Protocol = GetSchemeId("file");
						TermList.Add(cScheme, "file", 1);
						TermList.Add(cPath, _url, 1);
						State &= ~stEmpty;
						_done = 1;
					}
					else if(isdirslash(_url[0]) && isdirslash(_url[1])) {
						// Путь файловой системы
						Protocol = GetSchemeId("file");
						TermList.Add(cScheme, "file", 1);
						TermList.Add(cPath, _url, 1);
						State &= ~stEmpty;
						_done = 1;
					}
					else if(isdirslash(_url[0])) { // Сомнительный случай, но пути типа "/abc/catalog/x.bin" реальность
						// Путь файловой системы
						Protocol = GetSchemeId("file");
						TermList.Add(cScheme, "file", 1);
						TermList.Add(cPath, _url, 1);
						State &= ~stEmpty;
						_done = 1;
					}
					else if(_url.HasPrefixIAscii("file:") && isdirslash(_url[5]) && isdirslash(_url[6]) && isdirslash(_url[7])) {
						// Путь файловой системы, начиная с _url[8]
						Protocol = GetSchemeId("file");
						TermList.Add(cScheme, "file", 1);
						TermList.Add(cPath, _url+8, 1);
						State &= ~stEmpty;
						_done = 1;
					}
				}
			}
			if(!_done) {
				UriUri uri;
				UriParserState state(&uri);
				if(UriParseUri(&state, pUrl)) {
					temp_buf.CopyFromN(uri.Scheme.P_First, uri.Scheme.Len());
					if(temp_buf.NotEmpty()) {
						Protocol = GetSchemeId(temp_buf);
						TermList.Add(cScheme, temp_buf, 1);
					}
					temp_buf.CopyFromN(uri.UserInfo.P_First, uri.UserInfo.Len());
					if(temp_buf.NotEmpty()) {
						temp_buf.Divide(':', left_buf, right_buf);
						if(left_buf.NotEmptyS())
							TermList.Add(cUserName, left_buf, 1);
						if(right_buf.NotEmptyS())
							TermList.Add(cPassword, right_buf, 1);
					}
					temp_buf.CopyFromN(uri.HostText.P_First, uri.HostText.Len());
					if(temp_buf.NotEmpty())
						TermList.Add(cHost, temp_buf, 1);
					temp_buf.CopyFromN(uri.PortText.P_First, uri.PortText.Len());
					if(temp_buf.NotEmpty())
						TermList.Add(cPort, temp_buf, 1);
					//temp_buf = uri.pathHead ? uri.pathHead->text.first : 0;
					{
						temp_buf.Z();
						for(UriUri::PathSegment * p_pseg = uri.pathHead; p_pseg; p_pseg = p_pseg->next) {
							if(temp_buf.NotEmpty())
								temp_buf.Slash();
							temp_buf.CatN(p_pseg->text.P_First, p_pseg->text.Len());
						}
						if(temp_buf.NotEmpty())
							TermList.Add(cPath, temp_buf, 1);
					}
					temp_buf.CopyFromN(uri.query.P_First, uri.query.Len());
					if(temp_buf.NotEmpty())
						TermList.Add(cQuery, temp_buf, 1);
					temp_buf.CopyFromN(uri.fragment.P_First, uri.fragment.Len());
					if(temp_buf.NotEmpty())
						TermList.Add(cRef, temp_buf, 1);
					State &= ~stEmpty;
				}
				else {
					State |= stError;
					ok = 0;
				}
				uri.Destroy();
			}
		}
		else {
			State |= stEmpty;
			SLS.SetError(SLERR_URL_EMPTY); // @v11.8.10
			ok = -1;
		}
	}
	return ok;
}

int InetUrl::SetProtocol(int protocol)
{
	int    preserve_val = Protocol;
	Protocol = protocol;
	const char * p_sh = GetSchemeMnem(Protocol);
	SetComponent(cScheme, p_sh);
	return preserve_val;
}

int InetUrl::GetComponent(int c, int urlDecode, SString & rBuf) const
{
	rBuf.Z();
	int    ok = BIN(TermList.GetText(c, rBuf) > 0);
	if(ok && urlDecode) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		rBuf.DecodeUrl(r_temp_buf);
		rBuf = r_temp_buf;
	}
	return ok;
}

int InetUrl::SetComponent(int c, const char * pBuf)
{
	int    ok = 0;
	if(ValidateComponent(c)) {
		// Don't set port "0"
		if(c == cPort && pBuf && pBuf[0] == '0' && pBuf[1] == 0)
			pBuf = 0;
		ok = TermList.Add(c, pBuf, 1);
	}
	return ok;
}

int InetUrl::SetQueryParam(const char * pParam, const char * pValue)
{
	int    ok = -1;
	if(!isempty(pParam)) {
		SString temp_buf;
		SString org_query;
		GetComponent(cQuery, /*urlDecode*/0, org_query);
		StringSet ss('&', org_query);
		StringSet ss_new("&");
		SString left, right;
		bool   is_found = false;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			if(temp_buf.NotEmptyS()) {
				if(temp_buf.Divide('=', left, right) > 0 && left.Strip().IsEqiAscii(pParam)) {
					is_found = true;
					if(right == pValue)
						ss_new.add(temp_buf);
					else {
						right = pValue;
						temp_buf.Z().CatEq(left, right);
						ss_new.add(temp_buf);
						ok = 1;
					}
				}
				else
					ss_new.add(temp_buf);
			}
		}
		if(!is_found) {
			left = pParam;
			right = pValue;
			temp_buf.Z().CatEq(left, right);
			ss_new.add(temp_buf);
			ok = 1;
		}
		if(ok > 0)
			SetComponent(cQuery, ss_new.getBuf());
	}
	return ok;
}

int InetUrl::GetQueryParam(const char * pParam, int urlDecode, SString & rBuf) const
{
	int   ok = -1;
	SString temp_buf;
	rBuf.Z();
	if(GetComponent(cQuery, urlDecode, temp_buf)) {
		StringSet ss('&', temp_buf);
		SString left, right;
		for(uint ssp = 0; ok < 0 && ss.get(&ssp, temp_buf);) {
			if(temp_buf.Divide('=', left, right) > 0 && left.Strip().CmpNC(pParam) == 0) {
				rBuf = right.Strip();
				ok = 1;
			}
		}
	}
	return ok;
}

int InetUrl::Compose(long flags, SString & rBuf) const
{
	rBuf.Z();
	int    result = 0;
	const  long _f = (flags == 0) ? stAll : flags;
	if(_f == stEmpty)
		result = 0;
	else {
		SString temp_buf;
		if(_f & stScheme) {
			if(GetComponent(cScheme, 0, temp_buf)) {
				if(sstreqi_ascii(temp_buf, "file")) {
					rBuf.Cat(temp_buf).Colon().CatCharN('/', 2); // @v12.4.1 CatCharN('/', 3)-->CatCharN('/', 2)
				}
				else if(sstreqi_ascii(temp_buf, "mailto"))
					rBuf.Cat(temp_buf).Colon();
				else if(sstreqi_ascii(temp_buf, "mailfrom"))
					rBuf.Cat(temp_buf).Colon();
				else
					rBuf.Cat(temp_buf).Colon().CatCharN('/', 2);
				result |= cScheme;
			}
			else if(Protocol) {
				const char * p_scheme = GetSchemeMnem(Protocol);
				if(p_scheme) {
					if(Protocol == protFile) {
						rBuf.Cat(temp_buf).Colon().CatCharN('/', 2); // @v12.4.1 CatCharN('/', 3)-->CatCharN('/', 2)
					}
					else if(Protocol == protMailto)
						rBuf.Cat(temp_buf).Colon();
					else
						rBuf.Cat(p_scheme).Colon().CatCharN('/', 2);
					result |= cScheme;
				}
			}
		}
		if(_f & stUserName && GetComponent(cUserName, 0, temp_buf)) {
			rBuf.Cat(temp_buf);
			result |= cUserName;
		}
		if(_f & stPassword && GetComponent(cPassword, 0, temp_buf)) {
			rBuf.Colon().Cat(temp_buf);
			result |= cPassword;
		}
		if(result & (stUserName|stPassword))
			rBuf.CatChar('@');
		if(_f & stHost && GetComponent(cHost, 0, temp_buf)) {
            rBuf.Cat(temp_buf);
            result |= cHost;
            if(_f & stPort) {
				if(GetComponent(cPort, 0, temp_buf)) {
					rBuf.Colon().Cat(temp_buf);
					result |= cPort;
				}
				else {
					const int local_port = GetPort();
					if(local_port > 0) {
						rBuf.Colon().Cat(local_port);
						result |= cPort;
					}
				}
            }
		}
		if(_f & stPath && GetComponent(cPath, 0, temp_buf)) {
			/* @v12.4.1
			if(temp_buf.C(0) == '/')
				rBuf.RmvLastSlash();
			else
				rBuf.SetLastDSlash();
			rBuf.Cat(temp_buf);
			*/
			// @v12.4.1 {
			SString _path;
			//SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase|SFsPath::npfSlash, _path);
			_path = temp_buf;
			if(rBuf.NotEmpty() && !oneof2(_path.C(0), '/', '\\'))
				rBuf.CatChar('/');
			rBuf.Cat(_path);
			// } @v12.4.1 
			result |= cPath;
		}
		if(_f & stQuery && GetComponent(cQuery, 0, temp_buf)) {
            rBuf.CatChar('?').Cat(temp_buf);
            result |= cQuery;
		}
		if(_f & stRef && GetComponent(cRef, 0, temp_buf)) {
			rBuf.CatChar('#').Cat(temp_buf);
			result |= cRef;
		}
	}
	return result;
}
