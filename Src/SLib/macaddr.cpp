// MACADDR.CPP
// Copyright (c) A.Sobolev 2005, 2008, 2010, 2011, 2014, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <snet.h>
#include <iphlpapi.h>
#include <Ws2tcpip.h>

// iphlpapi.dll

// GetAdaptersInfo
// SendARP

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

void SLAPI MACAddr::Init()
{
	memzero(Addr, sizeof(Addr));
}

int  SLAPI MACAddr::IsEmpty() const
{
	for(size_t i = 0; i < sizeof(Addr); i++)
		if(Addr[i] != 0)
			return 0;
	return 1;
}

SString & FASTCALL MACAddr::ToStr(SString & rBuf) const
{
	rBuf.Z();
	for(size_t i = 0; i < sizeof(Addr); i++) {
		char   item_buf[64];
		sprintf(item_buf, "%.2X", Addr[i]);
		if(i)
			rBuf.CatChar('-');
		rBuf.Cat(item_buf);
	}
	return rBuf;
}

int FASTCALL MACAddr::Cmp(const MACAddr & s) const
{
	for(size_t i = 0; i < sizeof(Addr); i++)
		if(Addr[i] < s.Addr[i])
			return -1;
		else if(Addr[i] > s.Addr[i])
			return 1;
	return 0;
}

SLAPI MACAddrArray::MACAddrArray() : TSVector <MACAddr> () // @v9.8.4 TSArray-->TSVector
{
}

int SLAPI MACAddrArray::addUnique(const MACAddr & rItem)
{
	MACAddr * p_item;
	int    found = 0;
	for(uint i = 0; !found && enumItems(&i, (void **)&p_item);)
		if(rItem.Cmp(*p_item) == 0)
			found = 1;
	return found ? -1 : (insert(&rItem) ? 1 : 0);
}

int GetMacByIP(SString &rIP, MACAddr * pMAC)
{
	int    ok = -1;
	if(pMAC) {
		ULONG size = 0;
		ULONG mac[2];
		MEMSZERO(mac);
		ProcPool_IpHlpApi::PtLoad();
		if(ProcPool_IpHlpApi::SendARP(inet_addr(rIP), 0, mac, &(size = 6)) == NO_ERROR) {
			memcpy(pMAC->Addr, mac, sizeof(pMAC->Addr));
			ok = 1;
		}
	}
	return ok;
}

typedef TSCollection <InetAddr> InetAddrArray;

#define RES_NET    0L
#define RES_DOMAIN 1L
#define RES_HOST   2L

int GetIPAddrList(int Level, LPNETRESOURCE lpNet, InetAddrArray * pAddrs)
{
	int    ok = 1;
	DWORD  size = 0, count = 0, i = 0, j = 0;
    LPNETRESOURCE p_res = NULL;
    HANDLE handle = NULL;
    DWORD  status = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, lpNet, &handle);
    if(status == NO_ERROR) {
        count = 1000;
        size = sizeof(NETRESOURCE) * count;
        p_res = (LPNETRESOURCE)new char[size];
        if(p_res) {
            status = WNetEnumResource(handle, &count, (LPVOID)p_res, &size);
            if(status == NO_ERROR) {
                WNetCloseEnum(handle);
                handle = NULL;
                for(i = 0; i < count; i++) {
                    if(Level == RES_HOST && *strip(p_res[i].lpRemoteName)) { // @unicodeproblem
                        if(p_res[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
							if(pAddrs) {
								char * p = p_res[i].lpRemoteName; // @unicodeproblem
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
	delete p_res;
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
    THROW(p_res = (LPNETRESOURCE)new char[size]);
    THROW(WNetEnumResource(handle, &count, (LPVOID)p_res, &size) == NO_ERROR); // @todo в данном месте происходит ошибка на 2-ой итерации рекурсии
    WNetCloseEnum(handle);
    handle = NULL;
    for(i = 0; ok < 0 && i < count; i++) {
        if(Level == RES_HOST && *strip(p_res[i].lpRemoteName) && p_res[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) { // @unicodeproblem
			MACAddr ma;
			InetAddr addr;
			addr.Set(p_res[i].lpRemoteName+2); // @unicodeproblem
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
	delete p_res;
	return ok;
}
//
// @todo Кэшировать список адресов. Иначе скорость выполнения функции ниже всякой критики.
//
int SLAPI GetFirstHostByMACAddr(MACAddr * pMAC, InetAddr * pAddr)
{
	return Helper_GetFirstHostByMacAddr(RES_NET, 0, pMAC, pAddr);
	/*
	int    ok = -1;
	InetAddrArray ary;
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
int SLAPI GetFirstHostByMACAddr(MACAddr * pItem, InetAddr * pAddr)
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
								sin.sin_family           = AF_INET;
								sin.sin_port             = htons(0);
								sin.sin_addr.S_un.S_addr = inet_addr(ip);
								memzero(host, sizeof(host));
								getnameinfo((sockaddr*)&sin, sizeof(sin), host, sizeof(host), 0, 0, 0);

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

int SLAPI GetMACAddrList(MACAddrArray * pList)
{
	int    ok = 0;
	IP_ADAPTER_INFO * p_info = 0;
	ULONG  size = 0;
	ProcPool_IpHlpApi::PtLoad();
	DWORD  err = ProcPool_IpHlpApi::GetAdaptersInfo(0, &size);
	if(!err || err == ERROR_BUFFER_OVERFLOW) {
		p_info = (IP_ADAPTER_INFO *)SAlloc::C(size, 1);
		err = ProcPool_IpHlpApi::GetAdaptersInfo(p_info, &size);
		if(err == 0)
			for(IP_ADAPTER_INFO * p_iter = p_info; p_iter; p_iter = p_iter->Next) {
				if(p_iter->Type == MIB_IF_TYPE_ETHERNET) {
					MACAddr ma;
					if(p_iter->AddressLength == sizeof(ma.Addr)) {
						ma.Init();
						memcpy(ma.Addr, p_iter->Address, sizeof(ma.Addr));
						pList->insert(&ma);
						ok = 1;
					}
				}
			}
	}
	SAlloc::F(p_info);
	return ok;
}

int SLAPI GetFirstMACAddr(MACAddr * pAddr)
{
	int    ok = 0;
	MACAddrArray ma_list;
	CALLPTRMEMB(pAddr, Init());
	if(GetMACAddrList(&ma_list)) {
		ASSIGN_PTR(pAddr, ma_list.at(0));
		ok = 1;
	}
	return ok;
}

