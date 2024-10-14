// MACADDR.CPP
// Copyright (c) A.Sobolev 2005, 2008, 2010, 2011, 2014, 2016, 2019, 2020, 2022, 2023, 2024
//
#include <slib-internal.h>
#pragma hdrstop

typedef DWORD (WINAPI * PT_GetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
typedef DWORD (WINAPI * PT_SendARP)(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr, PULONG PhyAddrLen);

class ProcPool_IpHlpApi {
public:
	static int PtLoad();
	static int PtRelease();
	static PT_GetAdaptersInfo GetAdaptersInfo;
	static PT_SendARP SendARP;
private:
	static SDynLibrary * P_Lib;
};

PT_GetAdaptersInfo ProcPool_IpHlpApi::GetAdaptersInfo = 0;
PT_SendARP ProcPool_IpHlpApi::SendARP = 0;
SDynLibrary * ProcPool_IpHlpApi::P_Lib = 0;

static DWORD WINAPI pt_stub()
{
	return -1;
}

int ProcPool_IpHlpApi::PtLoad()
{
	int    ok = -1;
	SETIFZ(P_Lib, new SDynLibrary("iphlpapi.dll"));
	if(P_Lib->IsValid()) {
		if(!GetAdaptersInfo) {
			#define LOAD_PROC(proc) proc = (PT_##proc)P_Lib->GetProcAddr(#proc, 0); if(!proc) proc = (PT_##proc)pt_stub
			LOAD_PROC(GetAdaptersInfo);
			LOAD_PROC(SendARP);
			#undef LOAD_PROC
			ok = 1;
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	return ok;
}

int ProcPool_IpHlpApi::PtRelease()
{
	int    ok = -1;
	if(P_Lib) {
		#define UNLOAD_PROC(proc) proc = 0
		UNLOAD_PROC(GetAdaptersInfo);
		UNLOAD_PROC(SendARP);
		#undef LOAD_PROC
		ZDELETE(P_Lib);
		ok = 1;
	}
	return ok;
}

MACAddr::MACAddr() : TSBinary<6>()
{
}

MACAddr::MACAddr(const MACAddr & rS) : TSBinary<6>(rS)
{
}

/*MACAddr & FASTCALL MACAddr::operator = (const MACAddr & rS)
{
	memcpy(Addr, rS.Addr, sizeof(Addr));
	return *this;
}*/
//bool FASTCALL MACAddr::operator == (const MACAddr & rS) const { return memcmp(Addr, rS.Addr, sizeof(Addr)) == 0; }
/*MACAddr & MACAddr::Z()
{
	memzero(Addr, sizeof(Addr));
	return *this;
}*/
//bool MACAddr::IsEmpty() const { return ismemzero(Addr, sizeof(Addr)); }

SString & FASTCALL MACAddr::ToStr(long fmt, SString & rBuf) const
{
	rBuf.Z();
	for(size_t i = 0; i < sizeof(D); i++) {
		if(i && !(fmt & fmtPlain)) {
			rBuf.CatChar((fmt & fmtDivColon) ? ':' : '-');
		}
		if(fmt & fmtLower)
			rBuf.CatHex(D[i]);
		else
			rBuf.CatHexUpper(D[i]);
	}
	return rBuf;
}

bool MACAddr::FromStr(const char * pStr) // @v12.0.1
{
	Z();
	bool   ok = true;
	if(!isempty(pStr)) {
		const  char * p = pStr;
		uint   t = 0;
		char   temp_buf[128];
		while(*p) {
			char   c = *p;
			if(!oneof2(c, ' ', '\t')) {
				int    is_valid_ch = 0;
				if(ishex(c)) {
					temp_buf[t++] = c;
					is_valid_ch = 1;
				}
				else if(t == 32)
					break;
				else if(oneof3(c, '-', ':', ' ')) // Допускаем что разделителями могут быть пробелы
					is_valid_ch = 1;
				if(t > (sizeof(temp_buf)-4))
					break;
			}
			p++;
		}
		if(t == 12) {
			uint8 * p_data = D;
			for(size_t i = 0; i < 6; i++)
				p_data[i] = hextobyte(temp_buf + (i << 1));
		}
		else
			ok = SLS.SetError(SLERR_INVMACADDRSTR, pStr);
	}
	else
		ok = false;
	return ok;
}

int FASTCALL MACAddr::Cmp(const MACAddr & s) const
{
	for(size_t i = 0; i < sizeof(D); i++)
		if(D[i] < s.D[i])
			return -1;
		else if(D[i] > s.D[i])
			return 1;
	return 0;
}

MACAddrArray::MACAddrArray() : TSVector <MACAddr> ()
{
}

int MACAddrArray::addUnique(const MACAddr & rItem)
{
	MACAddr * p_item;
	int    found = 0;
	for(uint i = 0; !found && enumItems(&i, (void **)&p_item);)
		if(rItem.Cmp(*p_item) == 0)
			found = 1;
	return found ? -1 : (insert(&rItem) ? 1 : 0);
}

static int GetMacByIP(const SString & rIP, MACAddr * pMAC)
{
	int    ok = -1;
	if(pMAC) {
		ULONG size = 0;
		ULONG mac[2];
		MEMSZERO(mac);
		ProcPool_IpHlpApi::PtLoad();
		if(ProcPool_IpHlpApi::SendARP(inet_addr(rIP), 0, mac, &(size = 6)) == NO_ERROR) {
			memcpy(pMAC->D, mac, sizeof(pMAC->D));
			ok = 1;
		}
	}
	return ok;
}

#define RES_NET    0L
#define RES_DOMAIN 1L
#define RES_HOST   2L

int GetIPAddrList(int Level, LPNETRESOURCE lpNet, TSCollection <InetAddr> * pAddrs)
{
	int    ok = 1;
	DWORD  size = 0, count = 0, i = 0, j = 0;
    LPNETRESOURCE p_res = NULL;
    HANDLE handle = NULL;
    DWORD  status = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, lpNet, &handle);
    if(status == NO_ERROR) {
        count = 1000;
        size = sizeof(NETRESOURCE) * count;
        p_res = static_cast<LPNETRESOURCE>(SAlloc::M(size));
        if(p_res) {
            status = WNetEnumResource(handle, &count, p_res, &size);
            if(status == NO_ERROR) {
                WNetCloseEnum(handle);
                handle = NULL;
                for(i = 0; i < count; i++) {
                    if(Level == RES_HOST && !isempty(p_res[i].lpRemoteName)) { // @unicodeproblem
                        if(p_res[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
							if(pAddrs) {
								const char * p = SUcSwitch(p_res[i].lpRemoteName); // @unicodeproblem
								InetAddr * p_addr = new InetAddr;
								p++;
								p++;
								p_addr->Set(p);
								pAddrs->insert(p_addr);
							}
					}
					if(Level < RES_HOST)
						ok = GetIPAddrList(Level + 1, p_res + i, pAddrs); // @recursion
				}
			}
		}
	}
	else
		ok = -1;
    if(handle)
		WNetCloseEnum(handle);
	SAlloc::F(p_res);
	return ok;
}

static int Helper_GetFirstHostByMacAddr(int Level, LPNETRESOURCE lpNet, const MACAddr * pMAC, InetAddr * pAddr)
{
	int    ok = -1;
	DWORD  i = 0, j = 0;
	DWORD  count = 1000;
	DWORD  size = sizeof(NETRESOURCE) * count;
	SString ip;
    LPNETRESOURCE p_res = NULL;
    HANDLE handle = NULL;
	THROW(WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, lpNet, &handle) == NO_ERROR);
    THROW(p_res = static_cast<LPNETRESOURCE>(SAlloc::M(size)));
    THROW(WNetEnumResource(handle, &count, p_res, &size) == NO_ERROR); // @todo в данном месте происходит ошибка на 2-ой итерации рекурсии
    WNetCloseEnum(handle);
    handle = NULL;
    for(i = 0; ok < 0 && i < count; i++) {
        if(Level == RES_HOST && !isempty(p_res[i].lpRemoteName) && p_res[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) { // @unicodeproblem
			MACAddr ma;
			InetAddr addr;
			addr.Set(SUcSwitch(p_res[i].lpRemoteName+2)); // @unicodeproblem
			if(GetMacByIP(addr.ToStr(InetAddr::fmtAddr, ip), &ma) > 0 && ma.Cmp(*pMAC) == 0) {
				ASSIGN_PTR(pAddr, addr);
				ok = 1;
			}
		}
		if(Level < RES_HOST)
			THROW(ok = Helper_GetFirstHostByMacAddr(Level+1, p_res+i, pMAC, pAddr)); // @recursion
	}
	CATCHZOK
    if(handle)
		WNetCloseEnum(handle);
	SAlloc::F(p_res);
	return ok;
}
//
// @todo Кэшировать список адресов. Иначе скорость выполнения функции ниже всякой критики.
//
int GetFirstHostByMACAddr(const MACAddr * pMAC, InetAddr * pAddr)
{
	return Helper_GetFirstHostByMacAddr(RES_NET, 0, pMAC, pAddr);
	/*
	int    ok = -1;
	TSCollection <InetAddr> ary;
	if(GetIPAddrList(RES_NET, NULL, &ary) > 0) {
		SString ip;
		for(uint i = 0; ok == -1 && i < ary.getCount(); i++) {
			MACAddr ma;
			InetAddr * p_addr = ary.at(i);
			if(GetMacByIP(p_addr->ToStr(InetAddr::fmtAddr, ip), &ma) > 0 && ma.Cmp(*pMAC) == 0) {
				ASSIGN_PTR(pAddr, *p_addr);
				ok = 1;
			}
		}
	}
	return ok;
	*/
}

/*
int GetFirstHostByMACAddr(const MACAddr * pItem, InetAddr * pAddr)
{
	int    ok = 0;
	IP_ADAPTER_INFO * p_info = 0;
	ULONG  size = 0;
	if(pItem && pAddr) {
		DWORD  err = GetAdaptersInfo(0, &size);
		if(!err || err == ERROR_BUFFER_OVERFLOW) {
			p_info = (IP_ADAPTER_INFO *)SAlloc::C(size, 1);
			err = GetAdaptersInfo(p_info, &size);
			if(err == 0)
				for(IP_ADAPTER_INFO * p_iter = p_info; p_iter; p_iter = p_iter->Next) {
					if(p_iter->Type == MIB_IF_TYPE_ETHERNET) {
						MACAddr ma;
						if(p_iter->AddressLength == sizeof(ma.Addr)) {
							ma.Init();
							memcpy(ma.Addr, p_iter->Address, sizeof(ma.Addr));
							if(ma.Cmp(*pItem) == 0) {
								char ip[16], host[64];
								STRNSCPY(ip, p_iter->IpAddressList.IpAddress.String);
								struct sockaddr_in sin;
								sin.sin_family   = AF_INET;
								sin.sin_port     = htons(0);
								sin.sin_addr.S_un.S_addr = inet_addr(ip);
								memzero(host, sizeof(host));
								getnameinfo((sockaddr *)&sin, sizeof(sin), host, sizeof(host), 0, 0, 0);

								// HOSTENT * p_hent = gethostbyaddr(ip, strlen(ip), AF_INET);
								// if(p_hent)
								//	STRNSCPY(host, p_hent->h_name);

								pAddr->Set(host);
								ok = 1;
								break;
							}
						}
					}
				}
		}
		SAlloc::F(p_info);
	}
	return ok;
}
*/
int GetMACAddrList(MACAddrArray * pList)
{
	int    ok = 0;
	IP_ADAPTER_INFO * p_info = 0;
	ULONG  size = 0;
	ProcPool_IpHlpApi::PtLoad();
	DWORD  err = ProcPool_IpHlpApi::GetAdaptersInfo(0, &size);
	if(!err || err == ERROR_BUFFER_OVERFLOW) {
		p_info = static_cast<IP_ADAPTER_INFO *>(SAlloc::C(size, 1));
		err = ProcPool_IpHlpApi::GetAdaptersInfo(p_info, &size);
		if(err == 0) {
			for(IP_ADAPTER_INFO * p_iter = p_info; p_iter; p_iter = p_iter->Next) {
				if(oneof2(p_iter->Type, MIB_IF_TYPE_ETHERNET, IF_TYPE_IEEE80211)) { // @v12.0.12 IF_TYPE_IEEE80211
					MACAddr ma;
					if(p_iter->AddressLength == sizeof(ma.D)) {
						memcpy(ma.D, p_iter->Address, sizeof(ma.D));
						pList->insert(&ma);
						ok = 1;
					}
				}
			}
		}
	}
	SAlloc::F(p_info);
	return ok;
}

int GetFirstMACAddr(MACAddr * pAddr)
{
	int    ok = 0;
	MACAddrArray ma_list;
	CALLPTRMEMB(pAddr, Z());
	if(GetMACAddrList(&ma_list)) {
		ASSIGN_PTR(pAddr, ma_list.at(0));
		ok = 1;
	}
	return ok;
}
//
//
//
S_IPAddr::S_IPAddr() : binary128()
{
}
	
bool S_IPAddr::IsIp4() const
{
	return IsZero() ? false : ismemzero(D+4, sizeof(binary128)-4);
}
	
bool S_IPAddr::IsIp6() const
{
	return IsZero() ? false : !ismemzero(D+4, sizeof(binary128)-4);
}
	
/*
char * ossl_ipaddr_to_asc(uchar * p, int len)
{
	//
	// 40 is enough space for the longest IPv6 address + nul terminator byte
	// XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX\0
	//
	char buf[40], * out;
	int i = 0, remain = 0, bytes = 0;
	switch(len) {
		case 4: // IPv4
		    BIO_snprintf(buf, sizeof(buf), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
		    break;
		case 16: // IPv6
		    for(out = buf, i = 8, remain = sizeof(buf); i-- > 0 && bytes >= 0; remain -= bytes, out += bytes) {
			    const char * p_template = (i > 0 ? "%X:" : "%X");
			    bytes = BIO_snprintf(out, remain, p_template, p[0] << 8 | p[1]);
			    p += 2;
		    }
		    break;
		default:
		    BIO_snprintf(buf, sizeof(buf), "<invalid length=%d>", len);
		    break;
	}
	return OPENSSL_strdup(buf);
}
*/ 

SString & S_IPAddr::ToStr(long fmt, SString & rBuf) const
{
	rBuf.Z();
	if(IsZero()) {
		rBuf.Cat("0.0.0.0");
	}
	else {
		if(IsIp4()) {
			rBuf.Cat(D[0]).Dot().Cat(D[1]).Dot().Cat(D[2]).Dot().Cat(D[3]);
		}
		else if(IsIp6()) {
			for(uint i = 0; i < sizeof(D)/2; i++) {
				if(i)
					rBuf.Colon();
				rBuf.CatHex(*reinterpret_cast<const uint16 *>(D+(i<<1)));
			}
		}
		else {
			constexpr int somethint_went_wrong_with_SIpAddr_ToStr = 0;
			assert(somethint_went_wrong_with_SIpAddr_ToStr);
		}
	}
	return rBuf;
}

bool S_IPAddr::FromStr(const char * pStr) // @construction
{
	bool    ok = true;
	THROW(!isempty(pStr));
	while(oneof2(pStr[0], ' ', '\t'))
		pStr++;
	THROW(pStr[0]);
	{
		SString temp_buf(pStr);
		SString src_buf;
		bool is_sq_par = false;
		if(temp_buf.C(0) == '[') {
			size_t ep = 0;
			THROW(temp_buf.SearchCharPos(1, ']', &ep) && ep > 2);
			temp_buf.Sub(1, ep-1, src_buf);
			is_sq_par = true;
		}
		else
			src_buf = temp_buf;
		{
			uint dot_pos = 0;
			uint colon_pos = 0;
			const char * p_dot = src_buf.SearchChar('.', &dot_pos);
			const char * p_colon = src_buf.SearchChar(':', &colon_pos);
			if(p_dot) {
				THROW(!is_sq_par);
				if(!p_colon || colon_pos > dot_pos) {
					StringSet ss('.', src_buf);
					uint _v[4];
					for(uint ssp = 0, _c = 0; ss.get(&ssp, temp_buf); _c++) {
						THROW(_c < SIZEOFARRAY(_v));
						THROW(temp_buf.IsDec());
						{
							const uint idx = _c; //(SIZEOFARRAY(_v) - _c - 1);
							assert(idx >= 0 && idx <= 4); // @paranoic
							_v[idx] = temp_buf.ToULong();
							THROW(_v[idx] < 256);
						}
					}
					D[0] = static_cast<uint8>(_v[0]);
					D[1] = static_cast<uint8>(_v[1]);
					D[2] = static_cast<uint8>(_v[2]);
					D[3] = static_cast<uint8>(_v[3]);
				}
			}
			else if(p_colon) {
				if(!p_dot || dot_pos > colon_pos) {
					constexpr uint __end_break_pos_value = 1000;
					uint break_pos = 0;
					if(src_buf.HasPrefix("::")) {
						break_pos = 1;
						src_buf.ShiftLeft(2);
					}
					else if(src_buf.HasSuffix("::")) {
						break_pos = __end_break_pos_value; // special-value
						src_buf.Trim(src_buf.Len()-2);
					}
					StringSet ss(':', src_buf);
					TSVector <uint> _v;
					uint _c = 0;
					const uint ss_count = ss.getCount();
					for(uint ssp = 0; ss.get(&ssp, temp_buf); _c++) {
						THROW(_c < 8);
						if(temp_buf.IsEmpty()) {
							THROW(break_pos == 0); 
							SETIFZ(break_pos, _c+1);
						}
						else {
							THROW(temp_buf.Len() <= 4 && temp_buf.IsHex());
							uint v = _texttohex32(temp_buf.cptr(), temp_buf.Len());
							_v.insert(&v);
						}
					}
					THROW(_c > 0);
					if(break_pos) {
						const uint zero = 0U;
						const uint vc = _v.getCount();
						if(break_pos == __end_break_pos_value) {
							for(uint i = 0; i < (8 - vc); i++) 
								_v.insert(&zero);
						}
						else {
							for(uint i = 0; i < (8 - vc); i++) 
								_v.atInsert(break_pos-1, &zero);
						}
					}
					THROW(_v.getCount() == 8);
					{
						for(uint i = 0; i < _v.getCount(); i++) {
							const uint32 v = _v.at(i);
							assert(v < (1 << 16));
							*reinterpret_cast<uint16 *>(D + (i<<1)) = static_cast<uint16>(v);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
