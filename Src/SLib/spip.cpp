// SPIP.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2009, 2010, 2012, 2015, 2016, 2017, 2018, 2019
// Program Interface Paradigm
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

//uuid(52D5E7CA-F613-4333-A04E-125DE29D715F)
//uuid(52D5E7CAF6134333A04E125DE29D715F)

S_GUID_Base & FASTCALL S_GUID_Base::Init(REFIID rIID)
{
	memcpy(Data, &rIID, sizeof(Data));
	return *this;
}

S_GUID_Base::operator GUID & ()
	{ return *(GUID *)Data; }
int FASTCALL S_GUID_Base::operator == (const S_GUID_Base & s) const
	{ return BIN(memcmp(Data, s.Data, sizeof(Data)) == 0); }
int FASTCALL S_GUID_Base::operator != (const S_GUID_Base & s) const
	{ return BIN(memcmp(Data, s.Data, sizeof(Data)) != 0); }
int S_GUID_Base::IsZero() const
	{ return BIN(Data[0] == 0 && Data[1] == 0 && Data[2] == 0 && Data[3] == 0); }

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

SString & S_GUID_Base::ToStr(long fmt__, SString & rBuf) const
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
	}
	return rBuf;
}

int FASTCALL S_GUID_Base::FromStr(const char * pBuf)
{
	int    ok = 1;
	const  char * p = pBuf;
	uint   t = 0;
	char   temp_buf[64];
	if(p) {
		while(*p) {
			char   c = *p;
			if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
				temp_buf[t++] = c;
			if(c == ')' || t > (sizeof(temp_buf)-4))
				break;
			p++;
		}
	}
	if(t == 32) {
		size_t i;
		uint8 * p_data = reinterpret_cast<uint8 *>(Data);
		for(i = 0; i < 16; i++)
			p_data[i] = hextobyte(temp_buf + (i << 1));
		for(i = 0; i < 4; i++)
			*reinterpret_cast<uint16 *>(p_data+i*2) = swapw(*PTR16(p_data+i*2));
		*reinterpret_cast<uint32 *>(p_data) = swapdw(*PTR32(p_data));
	}
	else {
		memzero(Data, sizeof(Data));
		SLS.SetError(SLERR_INVGUIDSTR, pBuf);
		ok = 0;
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

S_GUID::S_GUID(const S_GUID_Base & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
}

S_GUID::S_GUID(const S_GUID & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
}

S_GUID & FASTCALL S_GUID::operator = (const S_GUID_Base & rS)
{
	memcpy(Data, rS.Data, sizeof(Data));
	return *this;
}
//
//
//
SLAPI SVerT::SVerT(int j, int n, int r)
{
	Set(j, n, r);
}

SVerT::operator uint32() const
{
	return ((static_cast<uint32>(V) << 16) | R);
}

void SLAPI SVerT::Set(uint32 n)
{
	V = static_cast<uint16>(n >> 16);
	R = static_cast<uint16>(n & 0x0000ffff);
}

int SLAPI SVerT::Get(int * pJ, int * pN, int * pR) const
{
	int j = static_cast<int>(V >> 8);
	int n = static_cast<int>(V & 0x00ff);
	int r = static_cast<int>(R);
	ASSIGN_PTR(pJ, j);
	ASSIGN_PTR(pN, n);
	ASSIGN_PTR(pR, r);
	return 1;
}

void SLAPI SVerT::Set(int j, int n, int r)
{
	V = (((uint16)j) << 8) | ((uint16)n);
	R = (uint16)r;
}

int SLAPI SVerT::IsLt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	if(V < v2.V)
		return 1;
	if(V == v2.V && R < v2.R)
		return 1;
	return 0;
}

int SLAPI SVerT::IsGt(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	if(V > v2.V)
		return 1;
	if(V == v2.V && R > v2.R)
		return 1;
	return 0;
}

int SLAPI SVerT::IsEq(int j, int n, int r) const
{
	SVerT v2(j, n, r);
	return (V == v2.V && R == v2.R) ? 1 : 0;
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

SString FASTCALL SVerT::ToStr(SString & rBuf) const
{
	int    j, n, r;
	Get(&j, &n, &r);
	return rBuf.Z().CatDotTriplet(j, n, r);
}

int FASTCALL SVerT::FromStr(const char * pStr)
{
	int    ok = 0;
	int    j = 0, n = 0, r = 0;
	SString temp_buf;
	SStrScan scan(pStr);
	Set(0, 0, 0);
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
	return ok;
}

int SLAPI SVerT::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW(pCtx->Serialize(dir, V, rBuf));
	THROW(pCtx->Serialize(dir, R, rBuf));
	CATCHZOK
	return ok;
}
