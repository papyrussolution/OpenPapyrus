// SPIP.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2012, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// Program Interface Paradigm
//
#include <slib-internal.h>
#pragma hdrstop
#include <sddl.h>

//uuid(52D5E7CA-F613-4333-A04E-125DE29D715F)
//uuid(52D5E7CAF6134333A04E125DE29D715F)

const S_GUID ZEROGUID;

S_GUID_Base & FASTCALL S_GUID_Base::Init(REFIID rIID)
{
	memcpy(Data, &rIID, sizeof(Data));
	return *this;
}

S_GUID_Base::operator const GUID & () const { return *(const GUID *)Data; }
bool FASTCALL S_GUID_Base::operator == (const S_GUID_Base & s) const { return (memcmp(Data, s.Data, sizeof(Data)) == 0); }
bool FASTCALL S_GUID_Base::operator != (const S_GUID_Base & s) const { return (memcmp(Data, s.Data, sizeof(Data)) != 0); }
bool S_GUID_Base::IsZero() const { return (Data[0] == 0 && Data[1] == 0 && Data[2] == 0 && Data[3] == 0); }

S_GUID_Base & S_GUID_Base::Z()
{
	memzero(Data, sizeof(Data));
	return *this;
}

static char * FASTCALL format_uuid_part(const uint8 * pData, size_t numBytes, int useLower, char * pBuf)
{
	if(useLower)
		switch(numBytes) {
			case 2: sprintf(pBuf, "%04x", *reinterpret_cast<const uint16 *>(pData)); break;
			case 4: sprintf(pBuf, "%08x", *reinterpret_cast<const uint32 *>(pData)); break;
			case 1: sprintf(pBuf, "%02x", *pData); break;
		}
	else
		switch(numBytes) {
			case 2:	sprintf(pBuf, "%04X", *reinterpret_cast<const uint16 *>(pData)); break;
			case 4: sprintf(pBuf, "%08X", *reinterpret_cast<const uint32 *>(pData)); break;
			case 1: sprintf(pBuf, "%02X", *pData); break;
		}
	return pBuf;
}

/* @construction
static void GUID_to_InnerSUID27(const S_GUID_Base & rS, SString & rBuf)
{
	rBuf.Z();
	const char * p_base = "ABCDEFGHIJKLMNOPQRSTUV0123456789";
	const char * p_tail_size_base = "WXYZ";
	char   temp_buf[64];
	uint32 c;
	uint i = 0;
	for(; i < (sizeof(rS) * 8) / 5; i++) {
		c = getbits(&rS, sizeof(rS), (i*5), 5); assert(c < 32); temp_buf[i] = p_base[c];
	}
	const uint tail_size = ((sizeof(rS) * 8) % 5);
	assert(tail_size == 3);
	c = getbits(&rS, sizeof(rS), (i*5), tail_size); assert(c < 32); 
	temp_buf[i++] = p_tail_size_base[tail_size-1];
	temp_buf[i++] = p_base[c];
	temp_buf[i] = 0;
	assert(i == 27);
	assert(i == sstrlen(temp_buf));
	//
	S_GUID debug_guid;
	{
		// @debug
		uint32 debug_buf[64];
		memzero(debug_buf, sizeof(debug_buf));
		uint j = 0;
		for(; j < 25; j++) {
			uint v = temp_buf[j];
			assert((v >= 'A' && v <= 'V') || (v >= '0' && v <= '9'));
			c = (v >= 'A' && v <= 'V') ? (v - 'A') : (v - '0' + ('V'-'A'+1));
			assert(c < 32);
			if(c & 0x01)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+0);
			if(c & 0x02)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+1);
			if(c & 0x04)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+2);
			if(c & 0x08)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+3);
			if(c & 0x10)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+4);
		}
		assert(temp_buf[j++] == 'Y');
		{
			uint v = temp_buf[j];
			assert((v >= 'A' && v <= 'V') || (v >= '0' && v <= '9'));
			c = (v >= 'A' && v <= 'V') ? (v - 'A') : (v - '0' + ('V'-'A'+1));
			assert(c < 32);
			if(c & 0x01)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+0);
			if(c & 0x02)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+1);
			if(c & 0x04)
				setbit32(debug_buf, sizeof(debug_buf), (j * 5)+2);
		}
		memcpy(&debug_guid, debug_buf, sizeof(debug_guid));
	}
	rBuf = temp_buf;
}*/

SString & S_GUID_Base::ToStr(uint fmt__, SString & rBuf) const
{
	char   temp_buf[64];
	uint   i;
	const  int use_lower = BIN(fmt__ & fmtLower);
	const uint8 * p_data = reinterpret_cast<const uint8 *>(Data);
	rBuf.Z();
	switch(fmt__ & ~fmtLower) {
		case fmtIDL:
			rBuf.Cat(format_uuid_part(p_data, 4, use_lower, temp_buf)).CatChar('-');
			p_data += 4;
			rBuf.Cat(format_uuid_part(p_data, 2, use_lower, temp_buf)).CatChar('-');
			p_data += 2;
			rBuf.Cat(format_uuid_part(p_data, 2, use_lower, temp_buf)).CatChar('-');
			p_data += 2;
			for(i = 0; i < 2; i++) {
				rBuf.Cat(format_uuid_part(p_data, 1, use_lower, temp_buf));
				p_data++;
			}
			rBuf.CatChar('-');
			for(i = 0; i < 6; i++) {
				rBuf.Cat(format_uuid_part(p_data, 1, use_lower, temp_buf));
				p_data++;
			}
			break;
		case fmtPlain:
			rBuf.Cat(format_uuid_part(p_data, 4, use_lower, temp_buf));
			p_data += 4;
			rBuf.Cat(format_uuid_part(p_data, 2, use_lower, temp_buf));
			p_data += 2;
			rBuf.Cat(format_uuid_part(p_data, 2, use_lower, temp_buf));
			p_data += 2;
			for(i = 0; i < 2; i++) {
				rBuf.Cat(format_uuid_part(p_data, 1, use_lower, temp_buf));
				p_data++;
			}
			for(i = 0; i < 6; i++) {
				rBuf.Cat(format_uuid_part(p_data, 1, use_lower, temp_buf));
				p_data++;
			}
			break;
		case fmtC:
			{
				const char * ox = "0x";
				rBuf.Cat(ox).Cat(format_uuid_part(p_data, 4, use_lower, temp_buf)).Comma();
				p_data += 4;
				rBuf.Cat(ox).Cat(format_uuid_part(p_data, 2, use_lower, temp_buf)).Comma();
				p_data += 2;
				rBuf.Cat(ox).Cat(format_uuid_part(p_data, 2, use_lower, temp_buf)).Comma();
				p_data += 2;
				for(uint i = 0; i < 8; i++) {
					rBuf.Cat(ox).Cat(format_uuid_part(p_data, 1, use_lower, temp_buf));
					if(i < 7)
						rBuf.Comma();
					p_data++;
				}
			}
			break;
		// @construction case fmtSUID27: GUID_to_InnerSUID27(*this, rBuf); break;
	}
	return rBuf;
}

int FASTCALL S_GUID_Base::FromStr(const char * pBuf)
{
	int    ok = 1;
	const  char * p = pBuf;
	uint   t = 0;
	char   temp_buf[128];
	if(p) {
		int    start_ch = 0;
		while(*p) {
			char   c = *p;
			if(!oneof2(c, ' ', '\t')) {
				if(oneof2(c, '{', '(')) {
					p++;
					c = *p;
					start_ch = c;
				}
				int    is_valid_ch = 0;
				if(ishex(c)) {
					temp_buf[t++] = c;
					is_valid_ch = 1;
				}
				else if(t == 32)
					break;
				else if(oneof2(c, '-', ' ')) // Допускаем что разделителями могут быть пробелы
					is_valid_ch = 1;
				else if(c == '}' && start_ch == '{')
					break;
				else if(c == ')' && start_ch == '(')
					break;
				if(t > (sizeof(temp_buf)-4))
					break;
			}
			p++;
		}
	}
	if(t == 32) {
		size_t i;
		uint8 * p_data = reinterpret_cast<uint8 *>(Data);
		for(i = 0; i < 16; i++)
			p_data[i] = hextobyte(temp_buf + (i << 1));
		/*
		for(i = 0; i < 4; i++) {
			*reinterpret_cast<uint16 *>(p_data+i*2) = swapw(*PTR16(p_data+i*2));
		}
		*reinterpret_cast<uint32 *>(p_data) = swapdw(*PTR32(p_data));
		*/
		*reinterpret_cast<uint32 *>(p_data) = SMem::BSwap(*PTR32(p_data));
		*reinterpret_cast<uint16 *>(p_data+sizeof(uint32)) = SMem::BSwap(*PTR16(p_data+sizeof(uint32)));
		*reinterpret_cast<uint16 *>(p_data+sizeof(uint32)+sizeof(uint16)) = SMem::BSwap(*PTR16(p_data+sizeof(uint32)+sizeof(uint16)));
	}
	else {
		memzero(Data, sizeof(Data));
		ok = SLS.SetError(SLERR_INVGUIDSTR, pBuf);
	}
	return ok;
}

int S_GUID_Base::Generate()
{
#ifdef _DEBUG
	GUID guid;
	if(CoCreateGuid(&guid) == S_OK) {
		Init(guid);
		S_GUID_Base test;
		SString s_buf;
		ToStr(fmtIDL, s_buf);
		test.FromStr(s_buf);
		assert(test == *this);
		return 1;
	}
	else
		return 0;
#else
	return BIN(CoCreateGuid((GUID *)Data) == S_OK);
#endif
}

S_GUID::S_GUID()
{
	Z();
}

S_GUID::S_GUID(SCtrGenerate g)
{
	Generate();
}

S_GUID::S_GUID(const S_GUID_Base & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
}

S_GUID::S_GUID(const S_GUID & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
}

S_GUID::S_GUID(const GUID & /*REFIID*/ rS) // @v11.7.4
{
	memcpy(Data, &rS, sizeof(Data));
}

S_GUID::S_GUID(const char * pStr)
{
	FromStr(pStr);
}

S_GUID & FASTCALL S_GUID::operator = (const S_GUID_Base & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
	return *this;
}
//
//
//
S_WinSID::S_WinSID() : SBinaryChunk()
{
}

S_WinSID::S_WinSID(const void * pSid) : SBinaryChunk()
{
	FromPSID(pSid);
}

bool S_WinSID::IsEmpty() const { return SBinaryChunk::Len() == 0; }

S_WinSID & S_WinSID::Z()
{
	SBinaryChunk::Z();
	return *this;
}

bool S_WinSID::FromPSID(const void * pSid)
{
	bool    ok = false;
	if(pSid) {
		if(IsValidSid(const_cast<void *>(pSid))) {
			Put(pSid, GetLengthSid(const_cast<void *>(pSid)));
			ok = true;
		}
	}
	return ok;
}

SString & S_WinSID::ToStr(uint format, SString & rBuf) const
{
	rBuf.Z();
	if(Len()) {
		SID * p_sid = static_cast<SID *>(const_cast<void *>(SBinaryChunk::PtrC()));
		bool done = false;
		if(format & fmtFriendly) {
			// Получаем имя trustee
			wchar_t * p_trustee_name = NULL;
			wchar_t * p_domain_name = NULL;
			DWORD  trustee_size = 0;
			DWORD  domain_size = 0;
			SID_NAME_USE sid_type;
			LookupAccountSidW(NULL, p_sid, p_trustee_name, &trustee_size, p_domain_name, &domain_size, &sid_type);
			if(trustee_size > 0) {
				p_trustee_name = new wchar_t[trustee_size];
				p_domain_name = new wchar_t[domain_size];
				if(LookupAccountSidW(NULL, p_sid, p_trustee_name, &trustee_size, p_domain_name, &domain_size, &sid_type)) {
					//p_entry->Trustee = std::wstring(p_domain_name) + L"\\" + std::wstring(p_trustee_name);
					SString temp_buf;
					temp_buf.Z().CopyUtf8FromUnicode(p_domain_name, sstrlen(p_domain_name), 1);
					rBuf.Cat(temp_buf).SetLastSlash();
					temp_buf.Z().CopyUtf8FromUnicode(p_trustee_name, sstrlen(p_trustee_name), 1);
					rBuf.Cat(temp_buf);
				}
				delete [] p_trustee_name;
				delete [] p_domain_name;
				done = true;
			} 
		}
		if(!done) {
			char * p_text = 0;
			if(::ConvertSidToStringSidA(p_sid, &p_text)) {
				rBuf = p_text;
				::LocalFree(p_text);
				done = true;
			}
		}
	}
	return rBuf;
}

bool S_WinSID::FromStr(const char * pText)
{
	bool   ok = false;
	return ok;
}

const  void * S_WinSID::GetPtrC() const { return SBinaryChunk::PtrC(); }
void * S_WinSID::GetPtr() { return SBinaryChunk::Ptr(); } // dangerous!
//
//
//
SVerT::SVerT() : V(0), R(0)
{
}

SVerT::SVerT(int major, int minor)
{
	Set(major, minor, 0);
}

SVerT::SVerT(int j, int n, int r)
{
	Set(j, n, r);
}

SVerT & SVerT::Z()
{
	V = R = 0;
	return *this;
}

SVerT::operator uint32() const
{
	return ((static_cast<uint32>(V) << 16) | R);
}

void SVerT::Set(uint32 n)
{
	V = static_cast<uint16>(n >> 16);
	R = static_cast<uint16>(n & 0x0000ffff);
}

int SVerT::GetMajor() const { return static_cast<int>(V >> 8); }
int SVerT::GetMinor() const { return static_cast<int>(V & 0x00ff); }
int SVerT::GetRevision() const { return static_cast<int>(R); }

int SVerT::Get(int * pJ, int * pN, int * pR) const
{
	int j = static_cast<int>(V >> 8);
	int n = static_cast<int>(V & 0x00ff);
	int r = static_cast<int>(R);
	ASSIGN_PTR(pJ, j);
	ASSIGN_PTR(pN, n);
	ASSIGN_PTR(pR, r);
	return 1;
}

void SVerT::Set(int j, int n, int r)
{
	V = (((uint16)j) << 8) | ((uint16)n);
	R = (uint16)r;
}

bool SVerT::IsLt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return (V < v2.V || (V == v2.V && R < v2.R));
}

bool SVerT::IsGt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return ((V > v2.V) || (V == v2.V && R > v2.R));
}

bool SVerT::IsGe(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return ((V > v2.V) || (V == v2.V && R >= v2.R));
}

bool SVerT::IsEq(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return (V == v2.V && R == v2.R);
}

int FASTCALL SVerT::Cmp(const SVerT * pVer) const
{
	int ok = 0;
	if(pVer) {
		if(V > pVer->V)
			ok = 1;
		else if(V < pVer->V)
			ok = -1;
		else if(R > pVer->R)
			ok = 1;
		else if(R < pVer->R)
			ok = -1;
		else
			ok = 0;
	}
	return ok;
}

SString & FASTCALL SVerT::ToStr(SString & rBuf) const
{
	int    j, n, r;
	Get(&j, &n, &r);
	return rBuf.Z().CatDotTriplet(j, n, r);
}

SString & SVerT::ToStr(uint fmt, SString & rBuf) const
{	
	rBuf.Z();
	int    j, n, r;
	Get(&j, &n, &r);
	rBuf.Cat(j);
	if(fmt & fmtMinorPadZero2)
		rBuf.Dot().CatLongZ(n, 2);
	else
		rBuf.Dot().Cat(n);
	if(!(fmt & fmtOmitZeroRelease))
		rBuf.Dot().Cat(r);
	return rBuf;
}

int FASTCALL SVerT::FromStr(const char * pStr)
{
	int    ok = 0;
	Z();
	if(!isempty(pStr)) {
		int    j = 0;
		int    n = 0;
		int    r = 0;
		SString temp_buf;
		SStrScan scan(pStr);
		if(scan.Skip().GetDigits(temp_buf)) {
			j = (int)temp_buf.ToLong();
			if(scan.Skip()[0] == '.') {
				scan.Incr();
				if(scan.Skip().GetDigits(temp_buf)) {
					n = (int)temp_buf.ToLong();
					if(scan.Skip()[0] == '.') {
						scan.Incr();
						if(scan.Skip().GetDigits(temp_buf)) {
							r = (int)temp_buf.ToLong();
							Set(j, n, r);
							ok = 3;
						}
					}
					else {
						Set(j, n, 0);
						ok = 2;
					}
				}
			}
		}
	}
	assert(ok != 0 || IsEmpty()); // @v12.2.12
	return ok;
}

int SVerT::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, V, rBuf));
	THROW(pCtx->Serialize(dir, R, rBuf));
	CATCHZOK
	return ok;
}
//
//
//
bool SObjID_Base::IsZero() const { return (Obj == 0 && Id == 0); }

SObjID_Base & SObjID_Base::Z()
{
	Obj = 0;
	Id = 0;
	return *this;
}

SObjID_Base SObjID_Base::Set(int32 objType, int32 objID)
{
	Obj = objType;
	Id = objID;
	return *this;
}

bool SObjID_Base::IsEq(int32 objType, int32 objID) const { return (Obj == objType && Id == objID); }
bool FASTCALL SObjID_Base::operator == (SObjID_Base s) const { return (Obj == s.Obj && Id == s.Id); }
bool FASTCALL SObjID_Base::operator != (SObjID_Base s) const { return (Obj != s.Obj || Id != s.Id); }


double SObjID_Base::ToDouble() const { return ((Obj * 4.0E+9) + static_cast<double>(Id)); }

bool SObjID_Base::FromDouble(double oid)
{
	Id = static_cast<long>(fmod(fabs(oid), 4.0E+9));
	Obj = static_cast<long>(fabs(oid) / 4.0E+9);
	return true;
}

SObjID_Base::operator double() const { return ToDouble(); }

SObjID_Base & FASTCALL SObjID_Base::operator = (double oid)
{
	FromDouble(oid);
	return *this;
}

/* @construction
SString & FASTCALL SObjID_Base::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(!DS.GetObjectTypeSymb(Obj, rBuf)) 
		rBuf.Cat("Obj").CatChar('[').Cat(Obj).CatChar(']');
	if(Id)
		rBuf.CatChar('(').Cat(Id).CatChar(')');
	return rBuf;
}

int FASTCALL SObjID_Base::FromStr(const char * pStr)
{
	int    ok = 0;
	SStrScan scan(pStr);
	SString temp_buf;
	scan.Skip();
	if(scan.GetIdent(temp_buf)) {
		long   extra = 0;
		PPID   lval = DS.GetObjectTypeBySymb(temp_buf, &extra);
		if(lval) {
			Obj = lval;
			ok = 1;
			scan.Skip();
			if(scan[0] == '(') {
				scan.Incr();
				if(scan.GetNumber(temp_buf)) {
					lval = temp_buf.ToLong();
					scan.Skip();
					if(scan[0] == ')') {
						Id = lval;
						ok = 2;
					}
				}
			}
		}
	}
	return ok;
}*/

SObjID::SObjID()
{
	Obj = 0;
	Id = 0;
}

SObjID::SObjID(const SObjID_Base & rS)
{
	Obj = rS.Obj;
	Id = rS.Id;
}

SObjID::SObjID(int32 objType, int32 objID)
{
	Obj = objType;
	Id = objID;
}

SObjID & FASTCALL SObjID::operator = (const SObjID_Base & rS)
{
	Obj = rS.Obj;
	Id = rS.Id;
	return *this;
}

int SObjID::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx) // @v11.7.0
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Obj, rBuf));
	THROW(pSCtx->Serialize(dir, Id, rBuf));
	CATCHZOK
	return ok;	
}
//
//
//
WinSecurityAttrs::WinSecurityAttrs(uint flags) : Flags(flags & fInheritHandle), P_SD(0)
{
}

WinSecurityAttrs::~WinSecurityAttrs()
{
	DestroySD();
}

int WinSecurityAttrs::MakeResult(void * pSA)
{
	int    ok = 1;
	if(pSA) {
		SECURITY_ATTRIBUTES * p_sa = static_cast<SECURITY_ATTRIBUTES *>(pSA);
		p_sa->nLength = sizeof(SECURITY_ATTRIBUTES);
		p_sa->lpSecurityDescriptor = P_SD;
		p_sa->bInheritHandle = BIN(Flags & fInheritHandle);
	}
	else {
		ok = -1;
	}
	return ok;
}

void WinSecurityAttrs::DestroySD()
{
	if(P_SD) {
		::LocalFree(P_SD);
		P_SD = 0;
	}
}

int WinSecurityAttrs::InitSD()
{
	int    ok = 1;
	DestroySD();
	P_SD = static_cast<SECURITY_DESCRIPTOR *>(::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));
	if(P_SD) { 
 		if(InitializeSecurityDescriptor(P_SD, SECURITY_DESCRIPTOR_REVISION)) {  
			ok = 1;
		} 
	}
	if(!ok) {
		DestroySD();
	}
	return ok;
}

int WinSecurityAttrs::SetSD_DACL(void * pACL, bool daclDefaulted)
{
	int    ok = 1;
	if(P_SD || InitSD()) {
		if(!::SetSecurityDescriptorDacl(static_cast<SECURITY_DESCRIPTOR *>(P_SD), TRUE, static_cast<ACL *>(pACL), daclDefaulted))
			ok = 0;
	}
	else {
		ok = 0;
	}
	return ok;
}

int WinSecurityAttrs::ResetSD_DACL()
{
	int    ok = 1;
	if(P_SD || InitSD()) {
		if(!::SetSecurityDescriptorDacl(static_cast<SECURITY_DESCRIPTOR *>(P_SD), FALSE, 0, FALSE))
			ok = 0;
	}
	else {
		ok = 0;
	}
	return ok;
}

static const SIntToSymbTabEntry WinPermissionFlags[] = {
	{ GENERIC_ALL, "GENERIC_ALL" },
	{ GENERIC_READ, "GENERIC_READ" },
	{ GENERIC_WRITE, "GENERIC_WRITE"},
	{ GENERIC_EXECUTE, "GENERIC_EXECUTE"},
	{ FILE_READ_DATA, "FILE_READ_DATA"},
	{ FILE_WRITE_DATA, "FILE_WRITE_DATA"},
	{ FILE_APPEND_DATA, "FILE_APPEND_DATA"},
	{ FILE_READ_EA, "FILE_READ_EA"},
	{ FILE_WRITE_EA, "FILE_WRITE_EA"},
	{ FILE_EXECUTE, "FILE_EXECUTE"},
	{ FILE_READ_ATTRIBUTES, "FILE_READ_ATTRIBUTES"},
	{ FILE_WRITE_ATTRIBUTES, "FILE_WRITE_ATTRIBUTES"},
	{ DELETE, "DELETE"},
	{ READ_CONTROL, "READ_CONTROL"},
	{ WRITE_DAC, "WRITE_DAC"},
	{ WRITE_OWNER, "WRITE_OWNER"},
	{ SYNCHRONIZE, "SYNCHRONIZE"},
};

/*static*/void WinSecurityAttrs::GetPermissionFlagsMnemonic(uint flags, SString & rBuf)
{
	rBuf.Z();
	if(!flags) {
		rBuf.Cat("(empty)");
	}
	else {
		for(uint i = 0; i < 32; i++) {
			if(flags & (1<<i)) {
				const char * p = SIntToSymbTab_GetSymbPtr(WinPermissionFlags, SIZEOFARRAY(WinPermissionFlags), (1<<i));
				if(rBuf.NotEmpty())
					rBuf.Space();
				if(p) {
					rBuf.Cat(p);
				}
				else
					rBuf.Cat("undef").CatChar('(').CatHex(1<<i).CatChar(')');
			}
		}
	}
}

#include <AccCtrl.h>

/*static*/uint WinSecurityAttrs::GetAclInfo(void * pACL, TSCollection <AceEntry> & rList)
{
	uint    result = 0;
	if(pACL) {
		// Получаем информацию о ACL
		ACL_SIZE_INFORMATION acl_size;
		if(::GetAclInformation(static_cast<ACL *>(pACL), &acl_size, sizeof(acl_size), AclSizeInformation)) {
			for(uint i = 0; i < acl_size.AceCount; i++) {
				void * p_ace = NULL;
				if(::GetAce(static_cast<ACL *>(pACL), i, &p_ace)) {
					ACE_HEADER * p_ace_header = (ACE_HEADER *)p_ace;
					// Обрабатываем разные типы ACE
					if(p_ace_header->AceType == ACCESS_ALLOWED_ACE_TYPE) {
						AceEntry * p_entry = rList.CreateNewItem();
						ACCESS_ALLOWED_ACE * p_allowed_ace = (ACCESS_ALLOWED_ACE *)p_ace;
						// Получаем имя trustee
						S_WinSID wsid(&p_allowed_ace->SidStart);
						wsid.ToStr(S_WinSID::fmtSystem, p_entry->TrusteeRaw);
						wsid.ToStr(S_WinSID::fmtFriendly, p_entry->Trustee);
						// Получаем маску прав
						//WinSecurityAttrs::GetPermissionFlagsMnemonic(uint flags, SString & rBuf)
						//aceInfo.permissions = GetPermissionString(pAllowedAce->Mask);
						p_entry->Permissions = p_allowed_ace->Mask;
						p_entry->AccessMode = GRANT_ACCESS;
						p_entry->Inheritance = p_allowed_ace->Header.AceFlags & (OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE|INHERIT_ONLY_ACE);
					}
				}
			}
			//::LocalFree(pSD);
		}
	}
	return result;
}