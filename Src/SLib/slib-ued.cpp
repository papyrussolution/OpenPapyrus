// slib-ued.cpp
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <ued.h>

ued_t::ued_t() : I(0ULL)
{
}

ued_t::ued_t(uint64 v) : I(v)
{
}

ued_t::ued_t(const ued_t & rS) : I(rS.I) 
{
}

ued_t & ued_t::Z()
{
	I = 0ULL;
	return *this;
}

ued_t FASTCALL ued_t::operator = (ued_t s)
{
	I = s.I;
	return *this;
}

ued_t FASTCALL ued_t::operator = (uint64 v)
{
	I = v;
	return *this;
}

/*static*/bool UED::GetRawValue(uint64 ued, uint64 * pRawValue)
{
	bool   ok = true;
	uint64 raw_value = 0ULL;
	const  uint bits = GetRawDataBits(ued);
	assert(oneof4(bits, 56, 48, 32, 0));
	if(bits == 32)
		raw_value = LoDWord(ued);	
	else if(bits == 48)
		raw_value = ued & 0x0000ffffffffffffULL;
	else if(bits == 56)
		raw_value = ued & 0x0000ffffffffffffULL;
	else
		ok = false;
	ASSIGN_PTR(pRawValue, raw_value);
	return ok;
}

/*static*/bool UED::GetRawValue32(uint64 ued, uint32 * pRawValue)
{
	bool   ok = true;
	uint32 raw_value = 0UL;
	const  uint  bits = GetRawDataBits(ued);
	assert(oneof4(bits, 56, 48, 32, 0));
	if(bits == 32) {
		raw_value = LoDWord(ued);
		assert(raw_value == GetRawValue32(ued));
	}
	else
		ok = false;
	ASSIGN_PTR(pRawValue, raw_value);
	return ok;
}

/*static*/uint64 UED::ApplyMetaToRawValue(uint64 meta, uint64 rawValue)
{
	uint64 result = 0ULL;
	const  uint bits = GetMetaRawDataBits(meta);
	assert(bits < 64);
	THROW(bits > 0);
	THROW((64 - SBits::Clz(rawValue)) <= bits)
	{
		const uint32 meta_lodw = LoDWord(meta);
		if(meta_lodw & 0x80000000)
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00ffffffffffffffULL);
		else if(meta_lodw & 0x40000000)
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x0000ffffffffffffULL);
		else
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00000000ffffffffULL);
	}
	CATCH
		result = 0ULL;
	ENDCATCH
	return result;
}

/*static*/uint64 UED::ApplyMetaToRawValue32(uint64 meta, uint32 rawValue)
{
	uint64 result = 0ULL;
	const  uint bits = GetMetaRawDataBits(meta);
	assert(bits < 64);
	THROW(bits > 0);
	if(rawValue != 0) {
		const uint _clz_ = SBits::Clz(rawValue);
		THROW((64 - (_clz_+32)) <= bits); // @???
	}
	{
		const uint32 meta_lodw = LoDWord(meta);
		THROW(!(meta_lodw & 0x80000000)); // @todo @err (not applicable)
		THROW(!(meta_lodw & 0x40000000)); // @todo @err (not applicable)
		result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00000000ffffffffULL);
	}
	CATCH
		result = 0ULL;
	ENDCATCH
	return result;
}

/*static*/uint64 UED::SetRaw_Int(uint64 meta, int64 val)
{
	uint64 ued = 0;
	const  uint bits = GetMetaRawDataBits(meta);
	int64  aval = abs(val);
	uint64 raw_val  = 0ULL;
	if(oneof2(bits, 48, 56)) {
		if(val != 0) {
			const uint _clz_ = SBits::Clz(static_cast<uint64>(aval));
			THROW((64 - _clz_ - 1) <= bits) // 1 - sign-bit
			if(val < 0) {
				raw_val = 1ULL << (bits-1);			
			}
			raw_val |= aval;
		}
		ued = ApplyMetaToRawValue(meta, raw_val);
	}
	CATCH
		ued = 0;
	ENDCATCH
	return ued;
}

/*static*/bool GetRaw_Int(uint64 ued, int64 & rVal)
{
	bool ok = false;
	// @todo
	return ok;
}

/*static*/uint64 UED::SetRaw_MacAddr(const MACAddr & rVal)
{
	static_assert(sizeof(rVal) == 6);
	uint64 ival = 0;
	memcpy(&ival, &rVal, sizeof(rVal));
	assert(PTR8(&ival)[6] == 0 && PTR8(&ival)[7] == 0);
	return SetRaw_Int(UED_META_MACADDR, ival);
}

/*static*/bool UED::GetRaw_MacAddr(uint64 ued, MACAddr & rVal)
{
	bool ok = false;
	uint64 raw_val = 0;
	const uint64 meta = GetMeta(ued);
	if(meta == UED_META_MACADDR && GetRawValue(ued, &raw_val)) {
		assert(PTR8(&raw_val)[6] == 0 && PTR8(&raw_val)[7] == 0);
		memcpy(&rVal, &raw_val, sizeof(rVal));
		ok = true;
	}
	return ok;
}
//
// Descr: Кодирует real-число в диапазоне [0..upp] в целочисленное представление
//   шириной bits бит, гранулярностью granulation.
//   Пояснение по поводу гранулярности: идея заключается в том, чтобы естественно выглядящие 
//   real-значения кодировались без потери точности. Например, угол 71deg должен после 
//   кодирования/декодирования оставаться таким же. 
//   Not a static for the testing purpose
//
uint64 UedEncodeRange(uint64 upp, uint granulation, uint bits, double value)
{
	assert(value >= 0.0 && value <= static_cast<double>(upp));
	assert(bits >= 8 && bits <= 64);
	// @v12.5.9 {
	if(bits == 64) {
		assert(granulation < MAXUINT64);
	}
	else 
	// } @v12.5.9 
	{
		assert(granulation < ((1ULL << bits) - 1));
	}
	uint64 result = 0;
	const uint64 ued_width = ((1ULL << bits) - 1) / granulation * granulation;
	result = static_cast<uint64>((ued_width / upp) * value);
	return result;
}

// Not a static for the testing purpose
bool UedDecodeRange(uint64 v, uint64 upp, uint granulation, uint bits, double * pResult)
{
	const uint64 ued_width = ((1ULL << bits) - 1) / granulation * granulation;
	*pResult = v * static_cast<double>(upp) / static_cast<double>(ued_width);
	return true;
}

/*static*/uint64 UED::SetRaw_UXControlIdent(SObjID ctlId) // @v12.5.9
{
	uint64 result = 0;
	constexpr uint64 meta = UED_META_UXCONTROL;
	if(meta != 0ULL && checkirange(ctlId.Obj, 0L, static_cast<long>(MAXINT16)) && checkirange(ctlId.Id, 0L, static_cast<long>(MAXINT16))) {
		uint64 raw_value = (ctlId.Obj << 16) | ctlId.Id;
		result = ApplyMetaToRawValue(meta, raw_value);
	}
	return result;
}

/*static*/bool UED::GetRaw_UXControlIdent(uint64 ued, SObjID & rCtlId) // @v12.5.9
{
	rCtlId.Z();
	bool   ok = false;
	uint64 meta = GetMeta(ued);
	uint64 raw_value = 0;
	if(GetRawValue(ued, &raw_value)) {
		const  long container_id = static_cast<long>((raw_value & _FFFF32) >> 16);
		const  long item_id = static_cast<long>(raw_value & _FFFF16);
		if(meta != 0ULL && checkirange(container_id, 0L, static_cast<long>(MAXINT16)) && checkirange(item_id, 0L, static_cast<long>(MAXINT16))) {
			rCtlId.Set(container_id, item_id);
			ok = true;
		}
	}
	return ok;
}

/*static*/uint64 UED::SetRaw_SphDir(const SphericalDirection & rV)
{
	uint64 result = 0;
	if(rV.IsValid()) {
		constexpr uint _bits = 28;
		uint64 _pa = UedEncodeRange(180, 36000, _bits, rV.PolarAngle);
		uint64 _a = UedEncodeRange(360, 36000, _bits, rV.Azimuth);
		uint64 raw = (_pa << _bits) | _a;
		result = ApplyMetaToRawValue(UED_META_SPHERDIR, raw);
	}
	return result;
}

/*static*/bool UED::GetRaw_SphDir(uint64 ued, SphericalDirection & rV)
{
	bool   ok = false;
	if(BelongsToMeta(ued, UED_META_SPHERDIR)) {
		constexpr uint _bits = 28;
		uint64 _pa = ((ued >> _bits) & 0xfffffffULL);
		uint64 _a = (ued & 0xfffffffULL);
		UedDecodeRange(_pa, 180, 36000, _bits, &rV.PolarAngle);
		UedDecodeRange(_a, 360, 36000, _bits, &rV.Azimuth);
		ok = rV.IsValid();
	}
	return ok;
}

/*static*/uint64 UED::SetRaw_GeoLoc(const SGeoPosLL & rGeoPos)
{
	uint64 result = 0;
	if(rGeoPos.IsValid()) {
		constexpr uint _bits = 28;
		uint64 _lat = UedEncodeRange(180, 36000, _bits, rGeoPos.Lat + 90.0);
		uint64 _lon = UedEncodeRange(360, 36000, _bits, rGeoPos.Lon + 180.0);
		uint64 raw = (_lat << _bits) | _lon;
		result = ApplyMetaToRawValue(UED_META_GEOLOC, raw);
	}
	return result;
}

/*static*/bool UED::GetRaw_GeoLoc(uint64 ued, SGeoPosLL & rGeoPos)
{
	bool   ok = false;
	if(BelongsToMeta(ued, UED_META_GEOLOC)) {
		constexpr uint _bits = 28;
		uint64 _lat = ((ued >> _bits) & 0xfffffffULL);
		uint64 _lon = (ued & 0xfffffffULL);
		UedDecodeRange(_lat, 180, 36000, _bits, &rGeoPos.Lat);
		UedDecodeRange(_lon, 360, 36000, _bits, &rGeoPos.Lon);
		rGeoPos.Lat -= 90.0;
		rGeoPos.Lon -= 180.0;
		ok = true;
	}
	return ok;
}

#if(_MSC_VER >= 1900) // {
/*static*/uint64 UED::Helper_SetRaw_PlanarAngleDeg(uint64 meta, double deg)
{
	uint64 result = 0;
	const uint bits = GetMetaRawDataBits(meta);
	if(bits) {
		constexpr uint64 _divisor = 360ULL * 3600ULL;
		const uint64 mask = ~(~0ULL << (bits-2));
		const uint64 sign_mask = 1ULL << (bits-1);
		const bool  minus = (deg < 0);
		const uint64 maxv = (mask / _divisor) * _divisor;
		assert((maxv % 360ULL) == 0ULL);
		double _deg = fmod(deg, 360.0);
		assert(_deg > -360.0 && _deg < 360.0);
		_deg = fabs(_deg);
		uint64 raw = (uint64)(_deg * (maxv / 360ULL));
		result = ApplyMetaToRawValue(meta, minus ? (raw | sign_mask) : raw);
	}
	return result;
}

/*static*/bool UED::Helper_GetRaw_PlanarAngleDeg(uint64 meta, uint64 ued, double & rDeg)
{
	bool   ok = false;
	const  uint bits = GetMetaRawDataBits(meta);
	if(bits && BelongsToMeta(ued, meta)) {
		constexpr uint64 _divisor = 360ULL * 3600ULL;
		const uint64 mask = ~(~0ULL << (bits-2));
		const uint64 sign_mask = 1ULL << (bits-1);
		const uint64 maxv = (mask / _divisor) * _divisor;
		assert((maxv % 360ULL) == 0ULL);
		rDeg = static_cast<double>(ued & mask) / (maxv / 360ULL);
		assert(rDeg > -360.0 && rDeg < 360.0);
		if(ued & sign_mask)
			rDeg = -rDeg;
		ok = true;
	}
	return ok;
}

/*static*/uint64 UED::Helper_SetRaw_DecimalString(uint64 meta, const char * pT, uint flagsBits, uint flags)
{
	assert(flagsBits <= 8 && (!flags || ((sizeof(flags) << 3) - SBits::Clz(flags)) <= flagsBits));
	uint64 result = 0;
	const uint bits = GetMetaRawDataBits(meta);
	if(bits) {
		if(flagsBits <= 8 && (!flags || ((sizeof(flags) << 3) - SBits::Clz(flags)) <= flagsBits)) {
			const size_t tlen = sstrlen(pT);
			if(tlen) {
				bool   all_dec = true;
				for(uint i = 0; all_dec && i < tlen; i++) {
					if(!isdec(pT[i]))
						all_dec = false;
				}
				if(all_dec) {
					uint64 val = _texttodec64(pT, static_cast<uint>(tlen));
					uint   val_bits = val ? ((sizeof(val) << 3) - SBits::Clz(val)) : 0;
					if((val_bits + flagsBits) <= bits) {
						uint64 raw = (static_cast<uint64>(flags) << (bits - flagsBits)) | val;
						result = ApplyMetaToRawValue(meta, raw);
					}
				}
			}
		}
	}
	return result;
}

/*static*/bool UED::Helper_GetRaw_DecimalString(uint64 meta, uint64 ued, SString & rT, uint flagsBits, uint * pFlags)
{
	rT.Z();
	assert(flagsBits <= 8);
	bool ok = false;
	uint flags = 0;
	const  uint bits = GetMetaRawDataBits(meta);
	if(bits && BelongsToMeta(ued, meta)) {
		if(flagsBits <= 8) {
			uint64 raw = 0;
			if(GetRawValue(ued, &raw)) {
				const uint value_bits = (bits - flagsBits);
				flags = static_cast<uint>(raw >> value_bits);
				// @v12.4.7 uint64 val = raw & (_FFFF64 << (64 - bits) >> (64 - bits));
				uint64 val = raw & (_FFFF64 << (64 - value_bits) >> (64 - value_bits)); // @v12.4.7 
				rT.Cat(val);
				ok = true;
			}
		}
	}
	ASSIGN_PTR(pFlags, flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_Ru_INN(const char * pT, bool forceBadCD)
{
	// 10 or 12 десятичных знаков (40bits) + flags 4 bits
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		const size_t len = sstrlen(pT);
		tr.Run(reinterpret_cast<const uchar *>(pT), static_cast<uint>(len), nta, 0);
		const float p = nta.Has(SNTOK_RU_INN);
		if(p > 0.0f) {
			uint    spc_flags = 0;
			if(len == 10)
				spc_flags |= UED_SPCF_INN_010;
			else if(len == 12)
				spc_flags |= UED_SPCF_INN_012;
			assert(oneof2(len, 10, 12));
			if(spc_flags) {
				if(p < 0.1f) {
					if(forceBadCD) {
						spc_flags |= UED_SPCF_INN_BADCD;
						result = Helper_SetRaw_DecimalString(UED_META_RU_INN, pT, 4, spc_flags);
					}
				}
				else {
					result = Helper_SetRaw_DecimalString(UED_META_RU_INN, pT, 4, spc_flags);
				}
			}
		}
	}
	return result;
}

/*static*/bool UED::GetRaw_Ru_INN(uint64 ued, SString & rT, uint * pSpcFlags)
{
	uint   flags = 0;
	bool   ok = Helper_GetRaw_DecimalString(UED_META_RU_INN, ued, rT, 4, &flags);
	if(ok) {
		const  uint _pre_len = rT.Len32();
		if(flags & UED_SPCF_INN_010 && _pre_len < 10) {
			rT.PadLeft(10 - _pre_len, '0');
		}
		else if(flags & UED_SPCF_INN_012 && _pre_len < 12) {
			rT.PadLeft(12 - _pre_len, '0');
		}
	}
	ASSIGN_PTR(pSpcFlags, flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_Ru_KPP(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), static_cast<uint32>(sstrlen(pT)), nta, 0);
		if(nta.Has(SNTOK_RU_KPP))
			result = Helper_SetRaw_DecimalString(UED_META_RU_KPP, pT, 4, 0);
	}
	return result;
}
	
/*static*/bool UED::GetRaw_Ru_KPP(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_RU_KPP, ued, rT, 4, &flags);
	if(ok) {
		const  uint _pre_len = rT.Len32();
		if(_pre_len < 9) {
			rT.PadLeft(9 - _pre_len, '0');
		}
	}
	return ok;
}

/*static*/uint64 UED::SetRaw_Ru_SNILS(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_RU_SNILS))
			result = Helper_SetRaw_DecimalString(UED_META_RU_SNILS, pT, 4, 0);
	}
	return result;
}
	
/*static*/bool UED::GetRaw_Ru_SNILS(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_RU_SNILS, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_Ar_DNI(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_AR_DNI))
			result = Helper_SetRaw_DecimalString(UED_META_AR_DNI, pT, 4, 0);
	}
	return result;
}

/*static*/bool   UED::GetRaw_Ar_DNI(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_AR_DNI, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_Cl_RUT(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_CL_RUT))
			result = Helper_SetRaw_DecimalString(UED_META_CL_RUT, pT, 4, 0);
	}
	return result;
}

/*static*/bool   UED::GetRaw_Cl_RUT(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_CL_RUT, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_EAN13(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_EAN13))
			result = Helper_SetRaw_DecimalString(UED_META_BARCODE_EAN13, pT, 4, 0);
	}
	return result;
}

/*static*/bool UED::GetRaw_EAN13(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_BARCODE_EAN13, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_EAN8(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_EAN8))
			result = Helper_SetRaw_DecimalString(UED_META_BARCODE_EAN8, pT, 4, 0);
	}
	return result;
}

/*static*/bool   UED::GetRaw_EAN8(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_BARCODE_EAN8, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_UPCA(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		// @todo Различие между upca и upce необходимо задать флагами
		if(nta.Has(SNTOK_UPCA))
			result = Helper_SetRaw_DecimalString(UED_META_BARCODE_UPC, pT, 4, 0);
		else if(nta.Has(SNTOK_UPCE))
			result = Helper_SetRaw_DecimalString(UED_META_BARCODE_UPC, pT, 4, 0);
	}
	return result;
}

/*static*/bool UED::GetRaw_UPCA(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_BARCODE_UPC, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_GLN(const char * pT)
{
	uint64 result = 0;
	if(!isempty(pT)) {
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		tr.Run(reinterpret_cast<const uchar *>(pT), sstrlen32(pT), nta, 0);
		if(nta.Has(SNTOK_EAN13))
			result = Helper_SetRaw_DecimalString(UED_META_GLN, pT, 4, 0);
	}
	return result;
}

/*static*/bool UED::GetRaw_GLN(uint64 ued, SString & rT)
{
	uint flags = 0;
	bool ok = Helper_GetRaw_DecimalString(UED_META_GLN, ued, rT, 4, &flags);
	return ok;
}

/*static*/uint64 UED::SetRaw_PlanarAngleDeg(double deg) { return Helper_SetRaw_PlanarAngleDeg(UED_META_PLANARANGLE, deg); }
/*static*/bool UED::GetRaw_PlanarAngleDeg(uint64 ued, double & rDeg) { return Helper_GetRaw_PlanarAngleDeg(UED_META_PLANARANGLE, ued, rDeg); }

/*static*/uint64 UED::SetRaw_Color(const SColor & rC)
{
	union {
		uint8  Bytes[4];
		uint32 U;
	} rv;
	rv.Bytes[0] = rC.B;
	rv.Bytes[1] = rC.G;
	rv.Bytes[2] = rC.R;
	rv.Bytes[3] = rC.Alpha;
	return ApplyMetaToRawValue32(UED_META_COLORRGB, rv.U);
}

/*static*/bool UED::GetRaw_Color(uint64 ued, SColor & rC)
{
	bool   ok = false;
	union {
		uint8  Bytes[4];
		uint32 U;
	} rv;
	if(GetRawValue32(ued, &rv.U)) {
		rC.Set(rv.Bytes[2], rv.Bytes[1], rv.Bytes[0]);
		rC.SetAlpha(rv.Bytes[3]);
		ok = true;
	}
	return ok;
}
// } @construction
#endif // } (_MSC_VER >= 1900)

static int SetTimeZoneOffsetSec(uint64 * pRaw, uint bits, int offs)
{
	int    ok = 1;
	const  bool sign = (offs < 0);
	if(sign)
		offs = -offs;
	if(offs > (14 * 3600))
		ok = 0;
	else if((offs % (15 * 60)) != 0)
		ok = 0;
	else {
		int _qhr_offs = offs / (15 * 60); 
		const uint64 mask = (~(~0ULL << 7)) << (bits-7);
		if((*pRaw & mask) != 0ULL)
			ok = 0;
		else {
			assert((_qhr_offs & ~0x7f) == 0);
			if(sign)
				_qhr_offs |= 0x40;
			*pRaw |= static_cast<uint64>(_qhr_offs) << (bits - 7);
		}
	}
	return ok;
}

static int GetTimeZoneOffsetSec(uint64 * pRaw, uint bits, int * pOffs)
{
	int    ok = 1;
	int _qhr_offs = static_cast<int>(*pRaw >> (bits - 7)); // зональное смещение в четверть-часовых интервалах
	const bool sign = LOGIC(_qhr_offs & 0x40);
	_qhr_offs &= ~0x40;
	int  tz_offs_sc = _qhr_offs * (15 * 60);
	if(tz_offs_sc > (14 * 3600))
		ok = 0; // @err
	else {
		if(sign)
			tz_offs_sc = -tz_offs_sc;
		//if(SUniTime_Internal::ValidateTimeZone(_qhr_offs * ))
		const uint64 mask = (~(~0ULL << 7)) << (bits-7);
		uint64 raw2 = *pRaw & ~mask;
		*pRaw = raw2;
		ASSIGN_PTR(pOffs, tz_offs_sc);
	}
	return ok;
}

/*static*/uint64 UED::_SetRaw_Time(uint64 meta, const SUniTime_Internal & rT)
{
	uint64   result = 0;
	uint64   raw = 0;
	THROW(IsMetaId(meta));
	const uint bits = GetMetaRawDataBits(meta);
	THROW(bits);
	THROW(rT.IsValid());
	switch(meta) {
		case UED_META_DAYTIME_MS: // @v11.9.9
			raw = (rT.Hr * 3600000) + (rT.Mn * 60000) + (rT.Sc * 1000) + rT.MSc;
			break;
		case UED_META_TIME_MSEC:
			THROW(rT.GetTime100ns(&raw));
			raw /= 10000ULL;
			break;
		case UED_META_TIME_SEC:
			THROW(rT.GetTime100ns(&raw));
			raw /= 10000000ULL;
			break;
		case UED_META_TIME_MIN:
			THROW(rT.GetTime100ns(&raw));
			raw /= (60 * 10000000ULL);
			break;
		case UED_META_TIME_HR:
			THROW(rT.GetTime100ns(&raw));
			raw /= (60 * 60 * 10000000ULL);
			break;
		case UED_META_TIME_TZMSEC:
			THROW(rT.GetTime100ns(&raw));
			raw /= 10000ULL;
			if(rT.TimeZoneSc != SUniTime_Internal::Undef_TimeZone) {
				THROW(SetTimeZoneOffsetSec(&raw, bits, rT.TimeZoneSc));
			}
			break;
		case UED_META_TIME_TZSEC:
			THROW(rT.GetTime100ns(&raw));
			raw /= 10000000ULL;
			if(rT.TimeZoneSc != SUniTime_Internal::Undef_TimeZone) {
				THROW(SetTimeZoneOffsetSec(&raw, bits, rT.TimeZoneSc));
			}
			break;
		case UED_META_TIME_TZMIN:
			THROW(rT.GetTime100ns(&raw));
			raw /= (60 * 10000000ULL);
			if(rT.TimeZoneSc != SUniTime_Internal::Undef_TimeZone) {
				THROW(SetTimeZoneOffsetSec(&raw, bits, rT.TimeZoneSc));
			}
			break;
		case UED_META_TIME_TZHR:
			THROW(rT.GetTime100ns(&raw));
			raw /= (60 * 60 * 10000000ULL);
			if(rT.TimeZoneSc != SUniTime_Internal::Undef_TimeZone) {
				THROW(SetTimeZoneOffsetSec(&raw, bits, rT.TimeZoneSc));
			}
			break;
		case UED_META_DATE_DAY:
			raw = rT.GetSdnGregorian();
			break;
		case UED_META_DATE_MON:
			raw = SUniDate_Internal::GregorianToSdn(rT.Y, rT.M, 2);
			break;
		case UED_META_DATE_QUART:
			raw = SUniDate_Internal::GregorianToSdn(rT.Y, (((rT.M-1) / 3) * 3) + 1, 2);
			break;
		case UED_META_DATE_SMYR:
			raw = SUniDate_Internal::GregorianToSdn(rT.Y, (((rT.M-1) / 6) * 6) + 1, 2);
			break;
		case UED_META_DATE_YR:
			raw = rT.Y;
			break;
		case UED_META_DATE_DAYBC:
			CALLEXCEPT(); // @todo @err(unsupported)
			break;
		default:
			CALLEXCEPT(); // @todo @err
			break;
	}
	result = ApplyMetaToRawValue(meta, raw);
	CATCH
		result = 0;
	ENDCATCH
	return result;
}

/*static*/bool  UED::_GetRaw_Time(uint64 ued, SUniTime_Internal & rT)
{
	rT.Z();
	bool   ok = true;
	uint64 raw = 0;
	const  uint64 meta = GetMeta(ued);	
	const  uint   bits = GetRawDataBits(ued);
	THROW(GetRawValue(ued, &raw));
	switch(meta) {
		case UED_META_DAYTIME_MS: // @v11.9.9
			{
				uint32 msecs = static_cast<uint32>(raw);
				uint32 seconds = static_cast<int>(msecs / 1000);
				msecs = msecs % 1000;
				uint32 minutes = seconds / 60;
				seconds = seconds % 60;
				uint32 hours = minutes / 60;
				minutes = minutes % 60;
				rT.Hr = hours;
				rT.Mn = minutes;
				rT.Sc = seconds;
				rT.MSc = msecs;
			}
			break;
		case UED_META_TIME_MSEC:
			rT.SetTime100ns(raw * 10000);
			break;
		case UED_META_TIME_SEC:
			rT.SetTime100ns(raw * 10000000);
			break;
		case UED_META_TIME_MIN:
			rT.SetTime100ns(raw * 10000000 * 60);
			break;
		case UED_META_TIME_HR:
			rT.SetTime100ns(raw * 10000000 * 60 * 60);
			break;
		case UED_META_TIME_TZMSEC:
			{
				int  tz_offs_sc = 0;
				THROW(GetTimeZoneOffsetSec(&raw, bits, &tz_offs_sc));
				rT.SetTime100ns(raw * 10000);
				rT.TimeZoneSc = tz_offs_sc;
			}
			break;
		case UED_META_TIME_TZSEC:
			{
				int  tz_offs_sc = 0;
				THROW(GetTimeZoneOffsetSec(&raw, bits, &tz_offs_sc));
				rT.SetTime100ns(raw * 10000000);
				rT.TimeZoneSc = tz_offs_sc;
			}
			break;
		case UED_META_TIME_TZMIN:
			{
				int  tz_offs_sc = 0;
				THROW(GetTimeZoneOffsetSec(&raw, bits, &tz_offs_sc));
				rT.SetTime100ns(raw * 10000000 * 60);
				rT.TimeZoneSc = tz_offs_sc;
			}
			break;
		case UED_META_TIME_TZHR:
			{
				int  tz_offs_sc = 0;
				THROW(GetTimeZoneOffsetSec(&raw, bits, &tz_offs_sc));
				rT.SetTime100ns(raw * 10000000 * 60 * 60);
				rT.TimeZoneSc = tz_offs_sc;
			}
			break;
		case UED_META_DATE_DAY:
			rT.SetSdnGregorian(LoDWord(raw));
			break;
		case UED_META_DATE_MON:
			rT.SetSdnGregorian(LoDWord(raw));
			rT.D = 2;
			break;
		case UED_META_DATE_QUART:
			rT.SetSdnGregorian(LoDWord(raw));
			rT.M = (((rT.M-1) / 3) * 3) + 1;
			rT.D = 2;
			break;
		case UED_META_DATE_SMYR:
			rT.SetSdnGregorian(LoDWord(raw));
			rT.M = (((rT.M-1) / 6) * 6) + 1;
			rT.D = 2;
			break;
		case UED_META_DATE_YR:
			rT.Y = static_cast<uint>(raw);
			rT.M = 1;
			rT.D = 2;
			break;
		case UED_META_DATE_DAYBC:
			break;
		default:
			CALLEXCEPT(); // @todo @err
			break;
	}
	THROW(rT.IsValid());
	CATCHZOK
	return ok;
}

/*static*/bool   UED::_GetRaw_RangeDate(uint64 ued, DateRange & rT)
{
	bool   ok = false;
	return ok;
}

/*static*/uint64 UED::_SetRaw_RangeDate(const DateRange & rT)
{
	uint64 result = 0;
	return result;
}
