// SGEO.CPP
// Copyright (c) A.Sobolev 2009, 2010, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

//static
uint32 FASTCALL SZIndex2::Combine(uint16 x, uint16 y)
{
	const uint32 xdw = (uint32)x;
	const uint32 ydw = (uint32)y;
	const uint32 result =
	((xdw & 0x0001))       | ((ydw & 0x0001) <<  1) |
	((xdw & 0x0002) <<  1) | ((ydw & 0x0002) <<  2) |
	((xdw & 0x0004) <<  2) | ((ydw & 0x0004) <<  3) |
	((xdw & 0x0008) <<  3) | ((ydw & 0x0008) <<  4) |

	((xdw & 0x0010) <<  4) | ((ydw & 0x0010) <<  5) |
	((xdw & 0x0020) <<  5) | ((ydw & 0x0020) <<  6) |
	((xdw & 0x0040) <<  6) | ((ydw & 0x0040) <<  7) |
	((xdw & 0x0080) <<  7) | ((ydw & 0x0080) <<  8) |

	((xdw & 0x0100) <<  8) | ((ydw & 0x0100) <<  9) |
	((xdw & 0x0200) <<  9) | ((ydw & 0x0200) << 10) |
	((xdw & 0x0400) << 10) | ((ydw & 0x0400) << 11) |
	((xdw & 0x0800) << 11) | ((ydw & 0x0800) << 12) |

	((xdw & 0x1000) << 12) | ((ydw & 0x1000) << 13) |
	((xdw & 0x2000) << 13) | ((ydw & 0x2000) << 14) |
	((xdw & 0x4000) << 14) | ((ydw & 0x4000) << 15) |
	((xdw & 0x8000) << 15) | ((ydw & 0x8000) << 16);
	return result;
}

//static
uint64 FASTCALL SZIndex2::Combine(uint32 x, uint32 y)
{
	const uint64 dw_lo =
	((x & 0x0001))       | ((y & 0x0001) <<  1) |
	((x & 0x0002) <<  1) | ((y & 0x0002) <<  2) |
	((x & 0x0004) <<  2) | ((y & 0x0004) <<  3) |
	((x & 0x0008) <<  3) | ((y & 0x0008) <<  4) |

	((x & 0x0010) <<  4) | ((y & 0x0010) <<  5) |
	((x & 0x0020) <<  5) | ((y & 0x0020) <<  6) |
	((x & 0x0040) <<  6) | ((y & 0x0040) <<  7) |
	((x & 0x0080) <<  7) | ((y & 0x0080) <<  8) |

	((x & 0x0100) <<  8) | ((y & 0x0100) <<  9) |
	((x & 0x0200) <<  9) | ((y & 0x0200) << 10) |
	((x & 0x0400) << 10) | ((y & 0x0400) << 11) |
	((x & 0x0800) << 11) | ((y & 0x0800) << 12) |

	((x & 0x1000) << 12) | ((y & 0x1000) << 13) |
	((x & 0x2000) << 13) | ((y & 0x2000) << 14) |
	((x & 0x4000) << 14) | ((y & 0x4000) << 15) |
	((x & 0x8000) << 15) | ((y & 0x8000) << 16);

	const uint32 xh = (x >> 16);
	const uint32 yh = (y >> 16);
	const uint64 dw_hi =
	((xh & 0x0001))       | ((yh & 0x0001) <<  1) |
	((xh & 0x0002) <<  1) | ((yh & 0x0002) <<  2) |
	((xh & 0x0004) <<  2) | ((yh & 0x0004) <<  3) |
	((xh & 0x0008) <<  3) | ((yh & 0x0008) <<  4) |

	((xh & 0x0010) <<  4) | ((yh & 0x0010) <<  5) |
	((xh & 0x0020) <<  5) | ((yh & 0x0020) <<  6) |
	((xh & 0x0040) <<  6) | ((yh & 0x0040) <<  7) |
	((xh & 0x0080) <<  7) | ((yh & 0x0080) <<  8) |

	((xh & 0x0100) <<  8) | ((yh & 0x0100) <<  9) |
	((xh & 0x0200) <<  9) | ((yh & 0x0200) << 10) |
	((xh & 0x0400) << 10) | ((yh & 0x0400) << 11) |
	((xh & 0x0800) << 11) | ((yh & 0x0800) << 12) |

	((xh & 0x1000) << 12) | ((yh & 0x1000) << 13) |
	((xh & 0x2000) << 13) | ((yh & 0x2000) << 14) |
	((xh & 0x4000) << 14) | ((yh & 0x4000) << 15) |
	((xh & 0x8000) << 15) | ((yh & 0x8000) << 16);

	return ((dw_hi << 32) | dw_lo);
}

#define GIS_EPSILON 0.000001

static int IsGeoPosValid(double lat, double lon)
{
	int    ok = 1;
	if(lat < -90. || lat > 90.) {
		SLS.SetError(SLERR_INVGEOLATITUDE);
		ok = 0;
	}
	else if(lon < -180. || lon > 180.) {
		SLS.SetError(SLERR_INVGEOLONGITUDE);
		ok = 0;
	}
	return ok;
}

static SString & GeoPosToStr(double lat, double lon, SString & rBuf)
{
	rBuf = 0;
	if(lat != 0.0 || lon != 0.0) {
		rBuf.Cat(lat, MKSFMTD(0, 7, NMBF_NOTRAILZ)).CatDiv(',', 2).Cat(lon, MKSFMTD(0, 7, NMBF_NOTRAILZ));
	}
	return rBuf;
}

static int GeoPosFromStr(const char * pStr, double & rLat, double & rLon)
{
	int    ok = -1;
	SString nmb;
	SStrScan scan(pStr);
	if(scan.GetNumber(nmb)) {
		rLat = nmb.ToReal();
		scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsComma|SStrScan::wsSemicol);
		if(scan.GetNumber(nmb)) {
			rLon = nmb.ToReal();
			ok = IsGeoPosValid(rLat, rLon);
		}
		else
			rLon = 0.0;
	}
	else {
		rLat = 0.0;
		rLon = 0.0;
	}
	return ok;
}

SGeoPosLL::SGeoPosLL()
{
	Lat = Lon = 0.0;
}

SGeoPosLL::SGeoPosLL(double lat, double lon)
{
	Lat = lat;
	Lon = lon;
}

int FASTCALL SGeoPosLL::operator == (const SGeoPosLL & s) const
{
	return BIN(Cmp(s) == 0);
}

int FASTCALL SGeoPosLL::operator != (const SGeoPosLL & s) const
{
	return BIN(Cmp(s) != 0);
}

int FASTCALL SGeoPosLL::Cmp(const SGeoPosLL & s) const
{
	double d = (Lat - s.Lat);
	if(fabs(d) > GIS_EPSILON) {
		if(d < 0.0)
			return -1;
		else
			return 1;
	}
	else {
		d = (Lon - s.Lon);
		if(fabs(d) > GIS_EPSILON) {
			if(d < 0.0)
				return -1;
			else
				return 1;
		}
		else
			return 0;
	}
}

int SGeoPosLL::Valid() const
{
	return IsGeoPosValid(Lat, Lon);
}

SString & FASTCALL SGeoPosLL::ToStr(SString & rBuf) const
{
	return GeoPosToStr(Lat, Lon, rBuf);
}

int FASTCALL SGeoPosLL::FromStr(const char * pStr)
{
	return GeoPosFromStr(pStr, Lat, Lon);
}
//
//
//
static const double IntGeoCoordScale = 10000000.0;

SGeoPosLL_Int::SGeoPosLL_Int()
{
	Lat = 0;
	Lon = 0;
}

SGeoPosLL_Int::SGeoPosLL_Int(const SGeoPosLL_Int & rS)
{
	Lat = rS.Lat;
	Lon = rS.Lon;
}

SGeoPosLL_Int::SGeoPosLL_Int(const SGeoPosLL & rS)
{
    Set(rS.Lat, rS.Lon);
}

SGeoPosLL_Int::SGeoPosLL_Int(double lat, double lon)
{
	Set(lat, lon);
}

double SGeoPosLL_Int::GetLat() const
{
	return Lat ? (((double)Lat) / IntGeoCoordScale) : 0.0;
}

double SGeoPosLL_Int::GetLon() const
{
	return Lon ? (((double)Lon) / IntGeoCoordScale) : 0.0;
}

long SGeoPosLL_Int::GetIntLat() const
{
	return Lat;
}

long SGeoPosLL_Int::GetIntLon() const
{
	return Lon;
}

int FASTCALL SGeoPosLL_Int::operator == (const SGeoPosLL_Int & rS) const
{
	return BIN(Cmp(rS) == 0);
}

int FASTCALL SGeoPosLL_Int::operator != (const SGeoPosLL_Int & rS) const
{
	return BIN(Cmp(rS) != 0);
}

int FASTCALL SGeoPosLL_Int::operator == (const SGeoPosLL & rS) const
{
	return BIN(Cmp(rS) == 0);
}

int FASTCALL SGeoPosLL_Int::operator != (const SGeoPosLL & rS) const
{
	return BIN(Cmp(rS) != 0);
}

int FASTCALL SGeoPosLL_Int::Cmp(const SGeoPosLL_Int & rS) const
{
	if(Lat != rS.Lat) {
		if(Lat < rS.Lat)
			return -1;
		else
			return 1;
	}
	else if(Lon != rS.Lon) {
		if(Lon < rS.Lon)
			return -1;
		else
			return 1;
	}
	else
		return 0;
}

int FASTCALL SGeoPosLL_Int::Cmp(const SGeoPosLL & rS) const
{
	double d = (GetLat() - rS.Lat);
	if(fabs(d) > GIS_EPSILON) {
		if(d < 0.0)
			return -1;
		else
			return 1;
	}
	else {
		d = (GetLon() - rS.Lon);
		if(fabs(d) > GIS_EPSILON) {
			if(d < 0.0)
				return -1;
			else
				return 1;
		}
		else
			return 0;
	}
}

int SGeoPosLL_Int::Set(double lat, double lon)
{
    Lat = (long)R0(lat * IntGeoCoordScale);
    Lon = (long)R0(lon * IntGeoCoordScale);
	return Valid();
}

int SGeoPosLL_Int::SetInt(long lat, long lon)
{
	Lat = lat;
	Lon = lon;
	return Valid();
}

int SGeoPosLL_Int::Valid() const
{
	return IsGeoPosValid(GetLat(), GetLon());
}

SString & FASTCALL SGeoPosLL_Int::ToStr(SString & rBuf) const
{
	return GeoPosToStr(GetLat(), GetLon(), rBuf);
}

int FASTCALL SGeoPosLL_Int::FromStr(const char * pStr)
{
	double lat = 0.0;
	double lon = 0.0;
	int    ok = GeoPosFromStr(pStr, lat, lon);
	Set(lat, lon);
	return ok;
}
//
//
//
SLAPI  SGeoGridTab::Finder::Finder(const SGeoGridTab & rTab) : R_Tab(rTab)
{
	LastPosLat = 0;
	LastPosLon = 0;
}

uint FASTCALL SGeoGridTab::Finder::GetIdxLat(long c)
{
	uint   idx = 0;
	const long last_b = R_Tab.LatIdx.get(LastPosLat);
	if(last_b >= c && (LastPosLat == 0 || R_Tab.LatIdx.get(LastPosLat-1) < c)) {
		idx = LastPosLat;
	}
	else {
		//
		// @todo Можно еще больше оптимизировать поиск - мы знаем некую граничную точку LastPosLat
		// и результат ее сравнения с заданной координатой c. Следовательно можно искать только
		// слева или справа от LastPosLat.
		//
		if(!R_Tab.LatIdx.bsearch(c, &idx)) {
			const long b = R_Tab.LatIdx.get(idx);
			if(b < c)
				idx++;
		}
		LastPosLat = idx;
	}
	return idx; 
}

uint FASTCALL SGeoGridTab::Finder::GetIdxLon(long c)
{
	uint   idx = 0;
	const long last_b = R_Tab.LonIdx.get(LastPosLon);
	if(last_b >= c && (LastPosLon == 0 || R_Tab.LatIdx.get(LastPosLon-1) < c)) {
		idx = LastPosLon;
	}
	else {
		//
		// @todo Можно еще больше оптимизировать поиск - мы знаем некую граничную точку LastPosLon
		// и результат ее сравнения с заданной координатой c. Следовательно можно искать только
		// слева или справа от LastPosLon.
		//
		if(!R_Tab.LonIdx.bsearch(c, &idx)) {
			const long b = R_Tab.LonIdx.get(idx);
			if(b < c)
				idx++;
		}
		LastPosLon = idx;
	}
	return idx; 
}

int SLAPI SGeoGridTab::Finder::GetIdx(const SGeoPosLL_Int & rC, uint & rIdxLat, uint & rIdxLon)
{
	rIdxLat = GetIdxLat(rC.GetIntLat());
	rIdxLon = GetIdxLon(rC.GetIntLon());
	return 1;
}

uint32 FASTCALL SGeoGridTab::Finder::GetZIdx32(const SGeoPosLL_Int & rC)
{
	uint idx_lat = 0;
	uint idx_lon = 0;
	GetIdx(rC, idx_lat, idx_lon);
	return SZIndex2::Combine((uint16)(idx_lat & 0x0000ffff), (uint16)(idx_lon & 0x0000ffff));
}

uint64 FASTCALL SGeoGridTab::Finder::GetZIdx64(const SGeoPosLL_Int & rC)
{
	uint idx_lat = 0;
	uint idx_lon = 0;
	GetIdx(rC, idx_lat, idx_lon);
	return SZIndex2::Combine((uint32)idx_lat, (uint32)idx_lon);
}
//
SLAPI SGeoGridTab::SGeoGridTab(uint dim)
{
	assert(dim >= 4 && dim <= 32);
	Dim = dim;
	SrcCountLat = 0;
	SrcCountLon = 0;
}

int FASTCALL SGeoGridTab::IsEqual(const SGeoGridTab & rS) const
{
	//
	// При сравнении SrcCountLat и SrcCountLon не учитываем поскольку
	// эти поля не влияют на результат использования таблицы (важны только при построении
	// и для справочных целей.
	//
	int    yes = 1;
    if(Dim != rS.Dim)
		yes = 0;
	else if(LatIdx != rS.LatIdx)
		yes = 0;
	else if(LonIdx != rS.LonIdx)
		yes = 0;
	return yes;
}

int FASTCALL SGeoGridTab::operator == (const SGeoGridTab & rS) const
{
	return IsEqual(rS);
}

int FASTCALL SGeoGridTab::operator != (const SGeoGridTab & rS) const
{
	return !IsEqual(rS);
}

void SLAPI SGeoGridTab::SetSrcCountLat(uint64 c)
{
	SrcCountLat = c;
}

void SLAPI SGeoGridTab::SetSrcCountLon(uint64 c)
{
	SrcCountLon = c;
}

uint SLAPI SGeoGridTab::GetDim() const
{
	return Dim;
}

uint SLAPI SGeoGridTab::GetDensityLat() const
{
	return (uint)(SrcCountLat / (1ULL << Dim));
}

uint SLAPI SGeoGridTab::GetDensityLon() const
{
	return (uint)(SrcCountLon / (1ULL << Dim));
}

int FASTCALL SGeoGridTab::AddThresholdLat(long coord)
{
	assert(LatIdx.getCount() < (1UL << Dim));
	assert(coord >= -900000000 && coord <= +900000000);
	return LatIdx.add(coord);
}

int FASTCALL SGeoGridTab::AddThresholdLon(long coord)
{
	assert(LonIdx.getCount() < (1UL << Dim));
	assert(coord >= -1800000000 && coord <= +1800000000);
	return LonIdx.add(coord);
}

uint SLAPI SGeoGridTab::GetCountLat() const
{
	return LatIdx.getCount();
}

uint SLAPI SGeoGridTab::GetCountLon() const
{
	return LonIdx.getCount();
}

int SLAPI SGeoGridTab::Save(const char * pFileName)
{
    int   ok = 1;
    SString line_buf;
    SFile f_out(pFileName, SFile::mWrite);
    THROW(f_out.IsValid());
    THROW(f_out.WriteLine((line_buf = 0).CatBrackStr("pgcg-header").CR()));
	THROW(f_out.WriteLine((line_buf = 0).CatEq("dim", Dim).CR()));
	THROW(f_out.WriteLine((line_buf = 0).CatEq("srccount-lat", (int64)SrcCountLat).CR()));
	THROW(f_out.WriteLine((line_buf = 0).CatEq("srccount-lon", (int64)SrcCountLon).CR()));
	THROW(f_out.WriteLine((line_buf = 0).CatEq("gridcount-lat", LatIdx.getCount()).CR()));
	THROW(f_out.WriteLine((line_buf = 0).CatEq("gridcount-lon", LonIdx.getCount()).CR()));
	THROW(f_out.WriteLine((line_buf = 0).CR()));
	{
		THROW(f_out.WriteLine((line_buf = 0).CatBrackStr("pgcg-lat").CR()));
		for(uint i = 0; i < LatIdx.getCount(); i++) {
			THROW(f_out.WriteLine((line_buf = 0).Cat(LatIdx.get(i)).CR()));
		}
		THROW(f_out.WriteLine((line_buf = 0).CR()));
	}
	{
		THROW(f_out.WriteLine((line_buf = 0).CatBrackStr("pgcg-lon").CR()));
		for(uint i = 0; i < LonIdx.getCount(); i++) {
			THROW(f_out.WriteLine((line_buf = 0).Cat(LonIdx.get(i)).CR()));
		}
		THROW(f_out.WriteLine((line_buf = 0).CR()));
	}
    CATCHZOK
    return ok;
}

int SLAPI SGeoGridTab::Load(const char * pFileName)
{
    Dim = 0;
    SrcCountLat = 0;
    SrcCountLon = 0;
    LatIdx.clear();
    LonIdx.clear();

	int    ok = 1;
	int    zone = 0; // 1 - header, 2 - latitude, 3 - longitude
    SString line_buf;
    SString temp_buf;
    SString left_buf, right_buf;
    uint   hdr_count_lat = 0;
    uint   hdr_count_lon = 0;
    SFile f_in(pFileName, SFile::mRead);
	while(f_in.ReadLine(line_buf)) {
        line_buf.Chomp();
        if(line_buf.NotEmptyS()) {
            if(line_buf.C(0) == '[') {
				size_t rb_pos = 0;
                THROW(line_buf.StrChr(']', &rb_pos)); // Ошибка в формате файла geogridtag
				assert(rb_pos > 0);
				line_buf.Sub(1, rb_pos-1, temp_buf);
				if(temp_buf.CmpNC("pgcg-header") == 0) {
					THROW(zone == 0);
					zone = 1;
				}
				else if(temp_buf.CmpNC("pgcg-lat") == 0) {
					THROW(zone != 2);
					zone = 2;
				}
				else if(temp_buf.CmpNC("pgcg-lon") == 0) {
					THROW(zone != 3);
					zone = 3;
				}
				else {
					CALLEXCEPT(); // Не известная зона в файле geogridtab
				}
            }
            else {
				THROW(oneof3(zone, 1, 2, 3)); // Не верный формат файла geogridtab
                if(zone == 1) {
					if(line_buf.Divide('=', left_buf, right_buf) > 0) {
                        left_buf.Strip();
                        right_buf.Strip();
                        if(left_buf.CmpNC("dim") == 0) {
							Dim = (uint)right_buf.ToLong();
                            THROW(Dim >= 4 && Dim <= 32);
                        }
                        else if(left_buf.CmpNC("srccount-lat") == 0) {
                            SrcCountLat = right_buf.ToInt64();
                            THROW(SrcCountLat > 0 && SrcCountLat < 20000000000LL);
                        }
                        else if(left_buf.CmpNC("srccount-lon") == 0) {
                            SrcCountLon = right_buf.ToInt64();
                            THROW(SrcCountLon > 0 && SrcCountLon < 20000000000LL);
                        }
                        else if(left_buf.CmpNC("gridcount-lat") == 0) {
							hdr_count_lat = (uint)right_buf.ToLong();
                        }
                        else if(left_buf.CmpNC("gridcount-lon") == 0) {
							hdr_count_lon = (uint)right_buf.ToLong();
                        }
					}
                }
                else if(oneof2(zone, 2, 3)) {
                    const long threshold = line_buf.ToLong();
                    if(zone == 2) {
						THROW(threshold >= -900000000L && threshold <= 900000000L);
						THROW(LatIdx.add(threshold));
                    }
                    else if(zone == 3) {
						THROW(threshold >= -1800000000L && threshold <= 1800000000L);
						THROW(LonIdx.add(threshold));
                    }
                }
            }
        }
	}
	CATCHZOK
	return ok;
}
//
//
//
#if 0 // @construction {
#if !defined(GEOGRAPHICLIB_GEODESICEXACT_ORDER)
	//
	// The order of the expansions used by GeodesicExact.
	//
	#define GEOGRAPHICLIB_GEODESICEXACT_ORDER 30
#endif
//
//
//
class EllipticFunction {
private:
	//
	// Max depth required for sncndn.  Probably 5 is enough.
	//
	enum {
		num_ = 13
	};
	double _k2;
	double _kp2;
	double _alpha2;
	double _alphap2;
	double _eps;
	double _Kc;
	double _Ec;
	double _Dc;
	double _Pic;
	double _Gc;
	double _Hc;
public:
	EllipticFunction(double k2 = 0, double alpha2 = 0)
	{
		Reset(k2, alpha2);
	}
	EllipticFunction(double k2, double alpha2, double kp2, double alphap2)
	{
		Reset(k2, alpha2, kp2, alphap2);
	}
	void Reset(double k2 = 0, double alpha2 = 0)
	{
		Reset(k2, alpha2, 1 - k2, 1 - alpha2);
	}
	void Reset(double k2, double alpha2, double kp2, double alphap2);
	double k2() const
	{
		return _k2;
	}
	double kp2() const
	{
		return _kp2;
	}
	double alpha2() const { return _alpha2; }
	double alphap2() const { return _alphap2; }
	double K() const { return _Kc; }
	double E() const { return _Ec; }
	double D() const { return _Dc; }
	double KE() const { return _k2 * _Dc; }
	double Pi() const { return _Pic; }
	double G() const { return _Gc; }
	double H() const { return _Hc; }
	double F(double phi) const;
	double E(double phi) const;
	double Ed(double ang) const;
	double Einv(double x) const;
	double Pi(double phi) const;
	double D(double phi) const;
	double G(double phi) const;
	double H(double phi) const;
	double F(double sn, double cn, double dn) const;
	double E(double sn, double cn, double dn) const;
	double Pi(double sn, double cn, double dn) const;
	double D(double sn, double cn, double dn) const;
	double G(double sn, double cn, double dn) const;
	double H(double sn, double cn, double dn) const;
	double deltaF(double sn, double cn, double dn) const;
	double deltaE(double sn, double cn, double dn) const;
	double deltaEinv(double stau, double ctau) const;
	double deltaPi(double sn, double cn, double dn) const;
	double deltaD(double sn, double cn, double dn) const;
	double deltaG(double sn, double cn, double dn) const;
	double deltaH(double sn, double cn, double dn) const;
	void sncndn(double x, double & sn, double & cn, double & dn) const;
	double Delta(double sn, double cn) const
	{
		return sqrt((_k2 < 0) ? (1 - _k2 * sn*sn) : (_kp2 + _k2 * cn*cn));
	}
	static double RF(double x, double y, double z);
	static double RF(double x, double y);
	static double RC(double x, double y);
	static double RG(double x, double y, double z);
	static double RG(double x, double y);
	static double RJ(double x, double y, double z, double p);
	static double RD(double x, double y, double z);
};

class GeodesicExact {
private:
	//typedef double double;
	friend class GeodesicLineExact;
	static const int nC4_ = GEOGRAPHICLIB_GEODESICEXACT_ORDER;
	static const int nC4x_ = (nC4_ * (nC4_ + 1)) / 2;
	static const unsigned maxit1_ = 20;
	unsigned maxit2_;
	double tiny_;
	double tol0_;
	double tol1_;
	double tol2_;
	double tolb_;
	double xthresh_;

	enum captype {
		CAP_NONE = 0U,
		CAP_E    = 1U<<0,
		// Skip 1U<<1 for compatibility with Geodesic (not required)
		CAP_D    = 1U<<2,
		CAP_H    = 1U<<3,
		CAP_C4   = 1U<<4,
		CAP_ALL  = 0x1FU,
		CAP_MASK = CAP_ALL,
		OUT_ALL  = 0x7F80U,
		OUT_MASK = 0xFF80U,       // Includes LONG_UNROLL
	};

	static double CosSeries(double sinx, double cosx, const double c[], int n);
	static double Astroid(double x, double y);

	double _a, _f, _f1, _e2, _ep2, _n, _b, _c2, _etol2;
	double _C4x[nC4x_];

	void Lengths(const EllipticFunction & E, double sig12,
		double ssig1, double csig1, double dn1, double ssig2, double csig2, double dn2,
		double cbet1, double cbet2, unsigned outmask, double & s12s, double & m12a, double & m0, double & M12, double & M21) const;
	double InverseStart(EllipticFunction & E, double sbet1, double cbet1, double dn1,
		double sbet2, double cbet2, double dn2, double lam12, double slam12, double clam12,
		double & salp1, double & calp1, double & salp2, double & calp2, double & dnm) const;
	double Lambda12(double sbet1, double cbet1, double dn1, double sbet2, double cbet2, double dn2,
		double salp1, double calp1, double slam120, double clam120, double & salp2, double & calp2, double & sig12,
		double & ssig1, double & csig1, double & ssig2, double & csig2, EllipticFunction & E, double & domg12, bool diffp, double & dlam12) const;
	double GenInverse(double lat1, double lon1, double lat2, double lon2, unsigned outmask, double & s12,
		double & salp1, double & calp1, double & salp2, double & calp2, double & m12, double & M12, double & M21, double & S12) const;
	void C4coeff();
	void C4f(double k2, double c[]) const;
	static double reale(long long hi, long long lo)
	{
		return ldexp(double(hi), 52) + lo;
	}
public:
	enum mask {
		NONE          = 0U,
		LATITUDE      = 1U<<7  | CAP_NONE,
		LONGITUDE     = 1U<<8  | CAP_H,
		AZIMUTH       = 1U<<9  | CAP_NONE,
		DISTANCE      = 1U<<10 | CAP_E,
		DISTANCE_IN   = 1U<<11 | CAP_E,
		REDUCEDLENGTH = 1U<<12 | CAP_D,
		GEODESICSCALE = 1U<<13 | CAP_D,
		AREA          = 1U<<14 | CAP_C4,
		LONG_UNROLL   = 1U<<15,
		ALL           = OUT_ALL| CAP_ALL,
	};
	GeodesicExact(double a, double f);
	double Direct(double lat1, double lon1, double azi1, double s12,
	double & lat2, double & lon2, double & azi2,
	double & m12, double & M12, double & M21, double & S12) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE | LONGITUDE | AZIMUTH | REDUCEDLENGTH | GEODESICSCALE | AREA, lat2, lon2, azi2, t, m12, M12, M21, S12);
	}
	double Direct(double lat1, double lon1, double azi1, double s12, double & lat2, double & lon2) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE | LONGITUDE, lat2, lon2, t, t, t, t, t, t);
	}
	double Direct(double lat1, double lon1, double azi1, double s12,
	double & lat2, double & lon2, double & azi2) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE | LONGITUDE | AZIMUTH, lat2, lon2, azi2, t, t, t, t, t);
	}
	double Direct(double lat1, double lon1, double azi1, double s12, double & lat2, double & lon2, double & azi2, double & m12) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE | LONGITUDE | AZIMUTH | REDUCEDLENGTH, lat2, lon2, azi2, t, m12, t, t, t);
	}
	double Direct(double lat1, double lon1, double azi1, double s12, double & lat2, double & lon2, double & azi2, double & M12, double & M21) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE | LONGITUDE | AZIMUTH | GEODESICSCALE, lat2, lon2, azi2, t, t, M12, M21, t);
	}
	double Direct(double lat1, double lon1, double azi1, double s12, double & lat2, double & lon2, double & azi2, double & m12, double & M12, double & M21) const
	{
		double t;
		return GenDirect(lat1, lon1, azi1, false, s12, LATITUDE|LONGITUDE|AZIMUTH|REDUCEDLENGTH|GEODESICSCALE, lat2, lon2, azi2, t, m12, M12, M21, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2, double & azi2, double & s12,
		double & m12, double & M12, double & M21, double & S12) const
	{
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE|LONGITUDE|AZIMUTH|DISTANCE|REDUCEDLENGTH|GEODESICSCALE|AREA, lat2, lon2, azi2, s12, m12, M12, M21, S12);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE|LONGITUDE, lat2, lon2, t, t, t, t, t, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2, double & azi2) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE|LONGITUDE|AZIMUTH, lat2, lon2, azi2, t, t, t, t, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2, double & azi2, double & s12) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE|LONGITUDE|AZIMUTH|DISTANCE, lat2, lon2, azi2, s12, t, t, t, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2, double & azi2, double & s12, double & m12) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE|LONGITUDE|AZIMUTH|DISTANCE|REDUCEDLENGTH, lat2, lon2, azi2, s12, m12, t, t, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12,
		double & lat2, double & lon2, double & azi2, double & s12, double & M12, double & M21) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | GEODESICSCALE, lat2, lon2, azi2, s12, t, M12, M21, t);
	}
	void ArcDirect(double lat1, double lon1, double azi1, double a12, double & lat2, double & lon2, double & azi2, double & s12, double & m12, double & M12, double & M21) const
	{
		double t;
		GenDirect(lat1, lon1, azi1, true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | REDUCEDLENGTH | GEODESICSCALE, lat2, lon2, azi2, s12, m12, M12, M21, t);
	}
	double GeodesicExact::GenDirect(double lat1, double lon1, double azi1, bool arcmode, double s12_a12, uint outmask,
		double & rLat2, double & rLon2, double & rAzi2, double & r_s12, double & r_m12, double & r_M12, double & r_M21, double & r_S12) const;
	double Inverse(double lat1, double lon1, double lat2, double lon2,
		double & s12, double & azi1, double & azi2, double & m12, double & M12, double & M21, double & S12) const
	{
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE | AZIMUTH | REDUCEDLENGTH | GEODESICSCALE | AREA, s12, azi1, azi2, m12, M12, M21, S12);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2, double & s12) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE, s12, t, t, t, t, t, t);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2, double & azi1, double & azi2) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, AZIMUTH, t, azi1, azi2, t, t, t, t);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2, double & s12, double & azi1, double & azi2) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE | AZIMUTH, s12, azi1, azi2, t, t, t, t);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2,
		double & s12, double & azi1, double & azi2, double & m12) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE | AZIMUTH | REDUCEDLENGTH, s12, azi1, azi2, m12, t, t, t);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2,
		double & s12, double & azi1, double & azi2, double & M12, double & M21) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE | AZIMUTH | GEODESICSCALE, s12, azi1, azi2, t, M12, M21, t);
	}
	double Inverse(double lat1, double lon1, double lat2, double lon2,
	double & s12, double & azi1, double & azi2, double & m12, double & M12, double & M21) const
	{
		double t;
		return GenInverse(lat1, lon1, lat2, lon2, DISTANCE | AZIMUTH | REDUCEDLENGTH | GEODESICSCALE, s12, azi1, azi2, m12, M12, M21, t);
	}
	double GenInverse(double lat1, double lon1, double lat2, double lon2, unsigned outmask,
		double & s12, double & azi1, double & azi2, double & m12, double & M12, double & M21, double & S12) const;
	GeodesicLineExact Line(double lat1, double lon1, double azi1, unsigned caps = ALL) const;
	GeodesicLineExact InverseLine(double lat1, double lon1, double lat2, double lon2, unsigned caps = ALL) const;
	GeodesicLineExact DirectLine(double lat1, double lon1, double azi1, double s12, unsigned caps = ALL) const;
	GeodesicLineExact ArcDirectLine(double lat1, double lon1, double azi1, double a12, unsigned caps = ALL) const;
	GeodesicLineExact GenDirectLine(double lat1, double lon1, double azi1, bool arcmode, double s12_a12, unsigned caps = ALL) const;
	double MajorRadius() const
	{
		return _a;
	}
	double Flattening() const
	{
		return _f;
	}
	double EllipsoidArea() const
	{
		return 4 * SMathConst::Pi * _c2;
	}
	static const GeodesicExact& WGS84();
};

class GeodesicLineExact {
private:
	friend class GeodesicExact;
	static const int nC4_ = GeodesicExact::nC4_;

	double tiny_;
	double _lat1;
	double _lon1;
	double _azi1;
	double _a;
	double _f;
	double _b;
	double _c2;
	double _f1;
	double _e2;
	double _salp0;
	double _calp0;
	double _k2;
	double _salp1;
	double _calp1;
	double _ssig1;
	double _csig1;
	double _dn1;
	double _stau1;
	double _ctau1;
	double _somg1;
	double _comg1;
	double _cchi1;
	double _A4;
	double _B41;
	double _E0;
	double _D0;
	double _H0;
	double _E1;
	double _D1;
	double _H1;
	double _a13;
	double _s13;
	double _C4a[nC4_];            // all the elements of _C4a are used
	EllipticFunction _E;
	unsigned _caps;

	void LineInit(const GeodesicExact& g, double lat1, double lon1, double azi1, double salp1, double calp1, unsigned caps);
	GeodesicLineExact(const GeodesicExact& g, double lat1, double lon1, double azi1, double salp1, double calp1, unsigned caps, bool arcmode, double s13_a13);

	enum captype {
		CAP_NONE = GeodesicExact::CAP_NONE,
		CAP_E    = GeodesicExact::CAP_E,
		CAP_D    = GeodesicExact::CAP_D,
		CAP_H    = GeodesicExact::CAP_H,
		CAP_C4   = GeodesicExact::CAP_C4,
		CAP_ALL  = GeodesicExact::CAP_ALL,
		CAP_MASK = GeodesicExact::CAP_MASK,
		OUT_ALL  = GeodesicExact::OUT_ALL,
		OUT_MASK = GeodesicExact::OUT_MASK,
	};
public:
	enum mask {
		NONE          = GeodesicExact::NONE,
		LATITUDE      = GeodesicExact::LATITUDE,
		LONGITUDE     = GeodesicExact::LONGITUDE,
		AZIMUTH       = GeodesicExact::AZIMUTH,
		DISTANCE      = GeodesicExact::DISTANCE,
		DISTANCE_IN   = GeodesicExact::DISTANCE_IN,
		REDUCEDLENGTH = GeodesicExact::REDUCEDLENGTH,
		GEODESICSCALE = GeodesicExact::GEODESICSCALE,
		AREA          = GeodesicExact::AREA,
		LONG_UNROLL   = GeodesicExact::LONG_UNROLL,
		ALL           = GeodesicExact::ALL,
	};
	GeodesicLineExact(const GeodesicExact& g, double lat1, double lon1, double azi1, unsigned caps = ALL);
	GeodesicLineExact() : _caps(0U)
	{
	}
	double Position(double s12, double & lat2, double & lon2, double & azi2, double & m12, double & M12, double & M21, double & S12) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE | AZIMUTH | REDUCEDLENGTH | GEODESICSCALE | AREA, lat2, lon2, azi2, t, m12, M12, M21, S12);
	}
	double Position(double s12, double & lat2, double & lon2) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE, lat2, lon2, t, t, t, t, t, t);
	}
	double Position(double s12, double & lat2, double & lon2, double & azi2) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE | AZIMUTH, lat2, lon2, azi2, t, t, t, t, t);
	}
	double Position(double s12, double & lat2, double & lon2, double & azi2, double & m12) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE | AZIMUTH | REDUCEDLENGTH,
		lat2, lon2, azi2, t, m12, t, t, t);
	}
	double Position(double s12, double & lat2, double & lon2, double & azi2, double & M12, double & M21) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE | AZIMUTH | GEODESICSCALE, lat2, lon2, azi2, t, t, M12, M21, t);
	}
	double Position(double s12, double & lat2, double & lon2, double & azi2, double & m12, double & M12, double & M21) const
	{
		double t;
		return GenPosition(false, s12, LATITUDE | LONGITUDE | AZIMUTH | REDUCEDLENGTH | GEODESICSCALE, lat2, lon2, azi2, t, m12, M12, M21, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2, double & s12, double & m12, double & M12, double & M21, double & S12) const
	{
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | REDUCEDLENGTH | GEODESICSCALE | AREA, lat2, lon2, azi2, s12, m12, M12, M21, S12);
	}
	void ArcPosition(double a12, double & lat2, double & lon2)	const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE, lat2, lon2, t, t, t, t, t, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2) const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH, lat2, lon2, azi2, t, t, t, t, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2, double & s12) const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE, lat2, lon2, azi2, s12, t, t, t, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2, double & s12, double & m12) const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | REDUCEDLENGTH, lat2, lon2, azi2, s12, m12, t, t, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2, double & s12, double & M12, double & M21) const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | GEODESICSCALE, lat2, lon2, azi2, s12, t, M12, M21, t);
	}
	void ArcPosition(double a12, double & lat2, double & lon2, double & azi2,
		double & s12, double & m12, double & M12, double & M21) const
	{
		double t;
		GenPosition(true, a12, LATITUDE | LONGITUDE | AZIMUTH | DISTANCE | REDUCEDLENGTH | GEODESICSCALE, lat2, lon2, azi2, s12, m12, M12, M21, t);
	}
	double GenPosition(bool arcmode, double s12_a12, unsigned outmask,
	double & lat2, double & lon2, double & azi2,
	double & s12, double & m12, double & M12, double & M21,
	double & S12) const;
	void SetDistance(double s13);
	void SetArc(double a13);
	void GenSetDistance(bool arcmode, double s13_a13);
	bool Init() const
	{
		return _caps != 0U;
	}
	double Latitude() const
	{
		return Init() ? _lat1 : fgetnan();
	}
	double Longitude() const
	{
		return Init() ? _lon1 : fgetnan();
	}
	double Azimuth() const
	{
		return Init() ? _azi1 : fgetnan();
	}
	void Azimuth(double & sazi1, double & cazi1) const
	{
		if(Init()) {
			sazi1 = _salp1; cazi1 = _calp1;
		}
	}
	double EquatorialAzimuth() const;
	void   EquatorialAzimuth(double & sazi0, double & cazi0) const
	{
		if(Init()) {
			sazi0 = _salp0; cazi0 = _calp0;
		}
	}
	double EquatorialArc() const
	{
		return Init() ? atan2(_ssig1, _csig1) / SMathConst::PiDiv180 : fgetnan();
	}
	double MajorRadius() const
	{
		return Init() ? _a : fgetnan();
	}
	double Flattening() const
	{
		return Init() ? _f : fgetnan();
	}
	unsigned Capabilities() const
	{
		return _caps;
	}
	bool Capabilities(unsigned testcaps) const
	{
		testcaps &= OUT_ALL;
		return (_caps & testcaps) == testcaps;
	}
	double GenDistance(bool arcmode) const
	{
		return Init() ? (arcmode ? _a13 : _s13) : fgetnan();
	}
	double Distance() const { return GenDistance(false); }
	double Arc() const { return GenDistance(true); }
};

class SGeodesic {
public:
	//
	// Evaluate the atan2 function with the result in degrees
	//
	static double atan2d(double y, double x)
	{
		// In order to minimize round-off errors, this function rearranges the
		// arguments so that result of atan2 is in the range [-pi/4, pi/4] before
		// converting it to degrees and mapping the result to the correct
		// quadrant.
		//using std::atan2;
		//using std::abs;
		int q = 0;
		if(fabs(y) > fabs(x)) {
			Exchange(&x, &y);
			q = 2;
		}
		if(x < 0.0) {
			x = -x;
			++q;
		}
		// here x >= 0 and x >= abs(y), so angle is in [-pi/4, pi/4]
		double ang = atan2(y, x) / SMathConst::PiDiv180;
		switch(q) {
			// Note that atan2d(-0.0, 1.0) will return -0.  However, we expect that
			// atan2d will not be called with y = -0.  If need be, include
			//
			//   case 0: ang = 0 + ang; break;
			//
			// and handle mpfr as in AngRound.
			case 1: ang = ((y >= 0.0) ? 180.0 : -180.0) - ang; break;
			case 2: ang =  90.0 - ang; break;
			case 3: ang = -90.0 + ang; break;
		}
		return ang;
	}
	//
	// ARG(x IN): Угол в градусах
	//
    static void SinCosD(double x, double & rSinx, double & rCosx)
	{
		// In order to minimize round-off errors, this function exactly reduces
		// the argument to the range [-45, 45] before converting it to radians.
		//using std::sin; using std::cos;
		double r;
		int    q;
#if GEOGRAPHICLIB_CXX11_MATH && GEOGRAPHICLIB_PRECISION <= 3 && !defined(__GNUC__)
		// Disable for gcc because of bug in glibc version < 2.22, see
		//   https://sourceware.org/bugzilla/show_bug.cgi?id=17569
		// Once this fix is widely deployed, should insert a runtime test for the
		// glibc version number.  For example
		//   #include <gnu/libc-version.h>
		//   std::string version(gnu_get_libc_version()); => "2.22"
		//using std::remquo;
		r = remquo(x, T(90), &q);
#else
		//using std::fmod;
		//using std::floor;
		r = fmod(x, 360.0);
		q = int(floor(r / 90.0 + 0.5));
		r -= 90.0 * q;
#endif
		// now abs(r) <= 45
		r *= SMathConst::PiDiv180;
		// Possibly could call the gnu extension sincos
		double s = sin(r);
		double c = cos(r);
#if defined(_MSC_VER) && _MSC_VER < 1900
		// Before version 14 (2015), Visual Studio had problems dealing
		// with -0.0.  Specifically
		//   VC 10,11,12 and 32-bit compile: fmod(-0.0, 360.0) -> +0.0
		//   VC 12       and 64-bit compile:  sin(-0.0)        -> +0.0
		if(x == 0.0)
			s = x;
#endif
		switch(unsigned(q) & 3U) {
			case 0U:
				rSinx =  s;
				rCosx =  c;
				break;
			case 1U:
				rSinx =  c;
				rCosx = -s;
				break;
			case 2U:
				rSinx = -s;
				rCosx = -c;
				break;
			default:
				rSinx = -c;
				rCosx =  s;
				break; // case 3U
		}
		// Set sign of 0 results.  -0 only produced for sin(-0)
		if(x) {
			rSinx += 0.0;
			rCosx += 0.0;
		}
    }
	static double AngleNormalize(double degAngle)
	{
		double y = fmod(degAngle, 360.0);
#if defined(_MSC_VER) && _MSC_VER < 1900
		// Before version 14 (2015), Visual Studio had problems dealing
		// with -0.0.  Specifically
		//   VC 10,11,12 and 32-bit compile: fmod(-0.0, 360.0) -> +0.0
		if(x == 0.0)
			y = x;
#endif
		return (y <= -180.0) ? (y + 360.0) : ((y <= 180.0) ? y : (y - 360.0));
	}
    static double AngleRound(double x)
	{
		//using std::abs;
		static const double z = 1.0/16.0;
		if(x == 0.0)
			return 0;
		else {
			volatile double y = fabs(x);
			// The compiler mustn't "simplify" z - (z - y) to y
			y = (y < z) ? (z - (z - y)) : y;
			return (x < 0.0) ? -y : y;
		}
	}
};

double GeodesicExact::GenDirect(double lat1, double lon1, double azi1, bool arcmode, double s12_a12, uint outmask,
	double & rLat2, double & rLon2, double & rAzi2, double & r_s12, double & r_m12, double & r_M12, double & r_M21, double & r_S12) const
{
	// Automatically supply DISTANCE_IN if necessary
	if(!arcmode)
		outmask |= GeodesicExact::DISTANCE_IN;
	return GeodesicLineExact(*this, lat1, lon1, azi1, outmask).GenPosition(arcmode, s12_a12, outmask, rLat2, rLon2, rAzi2, r_s12, r_m12, r_M12, r_M21, r_S12);
}

GeodesicLineExact::GeodesicLineExact(const GeodesicExact& g, double lat1, double lon1, double azi1, unsigned caps)
{
	azi1 = SGeodesic::AngleNormalize(azi1);
	double salp1, calp1;
	// Guard against underflow in salp0.  Also -0 is converted to +0.
	SGeodesic::SinCosD(SGeodesic::AngleRound(azi1), salp1, calp1);
	LineInit(g, lat1, lon1, azi1, salp1, calp1, caps);
}

double GeodesicLineExact::EquatorialAzimuth() const
{
	return Init() ? SGeodesic::atan2d(_salp0, _calp0) : fgetnan();
}
#endif // } 0 @construction
