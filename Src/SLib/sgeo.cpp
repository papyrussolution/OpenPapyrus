// SGEO.CPP
// Copyright (c) A.Sobolev 2009, 2010, 2016, 2017, 2018, 2019, 2020, 2021
//
#include <slib-internal.h>
#pragma hdrstop

/*static*/uint32 FASTCALL SZIndex2::Combine(uint16 x, uint16 y)
{
	const uint32 xdw = static_cast<uint32>(x);
	const uint32 ydw = static_cast<uint32>(y);
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

/*static*/uint64 FASTCALL SZIndex2::Combine(uint32 x, uint32 y)
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

#define GIS_EPSILON 0.0000001

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
	rBuf.Z();
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

SGeoPosLL::SGeoPosLL() : Lat(0.0), Lon(0.0)
{
}

SGeoPosLL::SGeoPosLL(double lat, double lon) : Lat(lat), Lon(lon)
{
}

int FASTCALL SGeoPosLL::operator == (const SGeoPosLL & s) const { return BIN(Cmp(s) == 0); }
int FASTCALL SGeoPosLL::operator != (const SGeoPosLL & s) const { return BIN(Cmp(s) != 0); }

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

int SGeoPosLL::IsValid() const { return IsGeoPosValid(Lat, Lon); }
SString & FASTCALL SGeoPosLL::ToStr(SString & rBuf) const { return GeoPosToStr(Lat, Lon, rBuf); }
int FASTCALL SGeoPosLL::FromStr(const char * pStr) { return GeoPosFromStr(pStr, Lat, Lon); }
//
//
//
static const double IntGeoCoordScale = 10000000.0;

SGeoPosLL_Int::SGeoPosLL_Int() : Lat(0), Lon(0)
{
}

SGeoPosLL_Int::SGeoPosLL_Int(const SGeoPosLL_Int & rS) : Lat(rS.Lat), Lon(rS.Lon)
{
}

SGeoPosLL_Int::SGeoPosLL_Int(const SGeoPosLL & rS)
{
    Set(rS.Lat, rS.Lon);
}

SGeoPosLL_Int::SGeoPosLL_Int(double lat, double lon)
{
	Set(lat, lon);
}

double SGeoPosLL_Int::GetLat() const { return Lat ? (((double)Lat) / IntGeoCoordScale) : 0.0; }
double SGeoPosLL_Int::GetLon() const { return Lon ? (((double)Lon) / IntGeoCoordScale) : 0.0; }
long SGeoPosLL_Int::GetIntLat() const { return Lat; }
long SGeoPosLL_Int::GetIntLon() const { return Lon; }
int FASTCALL SGeoPosLL_Int::operator == (const SGeoPosLL_Int & rS) const { return BIN(Cmp(rS) == 0); }
int FASTCALL SGeoPosLL_Int::operator != (const SGeoPosLL_Int & rS) const { return BIN(Cmp(rS) != 0); }
int FASTCALL SGeoPosLL_Int::operator == (const SGeoPosLL & rS) const { return BIN(Cmp(rS) == 0); }
int FASTCALL SGeoPosLL_Int::operator != (const SGeoPosLL & rS) const { return BIN(Cmp(rS) != 0); }

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
    Lat = R0i(lat * IntGeoCoordScale);
    Lon = R0i(lon * IntGeoCoordScale);
	return IsValid();
}

int SGeoPosLL_Int::SetInt(long lat, long lon)
{
	Lat = lat;
	Lon = lon;
	return IsValid();
}

int SGeoPosLL_Int::IsValid() const { return IsGeoPosValid(GetLat(), GetLon()); }
SString & FASTCALL SGeoPosLL_Int::ToStr(SString & rBuf) const { return GeoPosToStr(GetLat(), GetLon(), rBuf); }

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
SGeoGridTab::Finder::Finder(const SGeoGridTab & rTab) : R_Tab(rTab), LastPosLat(0), LastPosLon(0)
{
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

void SGeoGridTab::Finder::GetIdx(const SGeoPosLL_Int & rC, uint & rIdxLat, uint & rIdxLon)
{
	rIdxLat = GetIdxLat(rC.GetIntLat());
	rIdxLon = GetIdxLon(rC.GetIntLon());
}

uint32 FASTCALL SGeoGridTab::Finder::GetZIdx32(const SGeoPosLL_Int & rC)
{
	uint idx_lat = 0;
	uint idx_lon = 0;
	GetIdx(rC, idx_lat, idx_lon);
	return SZIndex2::Combine(static_cast<uint16>(idx_lat & 0x0000ffff), static_cast<uint16>(idx_lon & 0x0000ffff));
}

uint64 FASTCALL SGeoGridTab::Finder::GetZIdx64(const SGeoPosLL_Int & rC)
{
	uint idx_lat = 0;
	uint idx_lon = 0;
	GetIdx(rC, idx_lat, idx_lon);
	return SZIndex2::Combine((uint32)idx_lat, (uint32)idx_lon);
}
//
SGeoGridTab::SGeoGridTab(uint dim) : Dim(dim), SrcCountLat(0), SrcCountLon(0)
{
	assert(dim >= 4 && dim <= 32);
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

void SGeoGridTab::SetSrcCountLat(uint64 c)
{
	SrcCountLat = c;
}

void SGeoGridTab::SetSrcCountLon(uint64 c)
{
	SrcCountLon = c;
}

uint SGeoGridTab::GetDim() const
{
	return Dim;
}

uint SGeoGridTab::GetDensityLat() const
{
	return (uint)(SrcCountLat / (1ULL << Dim));
}

uint SGeoGridTab::GetDensityLon() const
{
	return (uint)(SrcCountLon / (1ULL << Dim));
}

int FASTCALL SGeoGridTab::AddThresholdLat(long coord)
{
	assert(coord >= -900000000 && coord <= +900000000);
	if(LatIdx.getCount() == (1UL << Dim)) {
		return LatIdx.at(LatIdx.getCount()-1) = coord;
	}
	else {
		assert(LatIdx.getCount() < (1UL << Dim));
		return LatIdx.add(coord);
	}
}

int FASTCALL SGeoGridTab::AddThresholdLon(long coord)
{
	assert(coord >= -1800000000 && coord <= +1800000000);
	if(LonIdx.getCount() == (1UL << Dim)) {
		return LonIdx.at(LonIdx.getCount()-1) = coord;
	}
	else {
		assert(LonIdx.getCount() < (1UL << Dim));
		return LonIdx.add(coord);
	}
}

uint SGeoGridTab::GetCountLat() const
{
	return LatIdx.getCount();
}

uint SGeoGridTab::GetCountLon() const
{
	return LonIdx.getCount();
}

int SGeoGridTab::Save(const char * pFileName)
{
    int   ok = 1;
    SString line_buf;
    SFile f_out(pFileName, SFile::mWrite);
    THROW(f_out.IsValid());
    THROW(f_out.WriteLine(line_buf.Z().CatBrackStr("pgcg-header").CR()));
	THROW(f_out.WriteLine(line_buf.Z().CatEq("dim", Dim).CR()));
	THROW(f_out.WriteLine(line_buf.Z().CatEq("srccount-lat", (int64)SrcCountLat).CR()));
	THROW(f_out.WriteLine(line_buf.Z().CatEq("srccount-lon", (int64)SrcCountLon).CR()));
	THROW(f_out.WriteLine(line_buf.Z().CatEq("gridcount-lat", LatIdx.getCount()).CR()));
	THROW(f_out.WriteLine(line_buf.Z().CatEq("gridcount-lon", LonIdx.getCount()).CR()));
	THROW(f_out.WriteLine(line_buf.Z().CR()));
	{
		THROW(f_out.WriteLine(line_buf.Z().CatBrackStr("pgcg-lat").CR()));
		for(uint i = 0; i < LatIdx.getCount(); i++) {
			THROW(f_out.WriteLine(line_buf.Z().Cat(LatIdx.get(i)).CR()));
		}
		THROW(f_out.WriteLine(line_buf.Z().CR()));
	}
	{
		THROW(f_out.WriteLine(line_buf.Z().CatBrackStr("pgcg-lon").CR()));
		for(uint i = 0; i < LonIdx.getCount(); i++) {
			THROW(f_out.WriteLine(line_buf.Z().Cat(LonIdx.get(i)).CR()));
		}
		THROW(f_out.WriteLine(line_buf.Z().CR()));
	}
    CATCHZOK
    return ok;
}

int SGeoGridTab::Load(const char * pFileName)
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
                THROW(line_buf.SearchChar(']', &rb_pos)); // Ошибка в формате файла geogridtag
				assert(rb_pos > 0);
				line_buf.Sub(1, rb_pos-1, temp_buf);
				if(temp_buf.IsEqiAscii("pgcg-header")) {
					THROW(zone == 0);
					zone = 1;
				}
				else if(temp_buf.IsEqiAscii("pgcg-lat")) {
					THROW(zone != 2);
					zone = 2;
				}
				else if(temp_buf.IsEqiAscii("pgcg-lon")) {
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
                        if(left_buf.IsEqiAscii("dim")) {
							Dim = (uint)right_buf.ToLong();
                            THROW(Dim >= 4 && Dim <= 32);
                        }
                        else if(left_buf.IsEqiAscii("srccount-lat")) {
                            SrcCountLat = right_buf.ToInt64();
                            THROW(SrcCountLat > 0 && SrcCountLat < 20000000000LL);
                        }
                        else if(left_buf.IsEqiAscii("srccount-lon")) {
                            SrcCountLon = right_buf.ToInt64();
                            THROW(SrcCountLon > 0 && SrcCountLon < 20000000000LL);
                        }
                        else if(left_buf.IsEqiAscii("gridcount-lat")) {
							hdr_count_lat = (uint)right_buf.ToLong();
                        }
                        else if(left_buf.IsEqiAscii("gridcount-lon")) {
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
/*static*/const double SGeo::A_WGS84 = 6378137.0; // Радиус Земли в метрах (согласно WGS84)
/*static*/const double SGeo::Flattening_WGS84 = 1.0 / 298.257223563; // Фактор приплюснотости Земли: ƒ = (a − b)/a (согласно WGS84).

#define GEOGRAPHICLIB_GEODESIC_ORDER 6
#define nA1   GEOGRAPHICLIB_GEODESIC_ORDER
#define nC1   GEOGRAPHICLIB_GEODESIC_ORDER
#define nC1p  GEOGRAPHICLIB_GEODESIC_ORDER
#define nA2   GEOGRAPHICLIB_GEODESIC_ORDER
#define nC2   GEOGRAPHICLIB_GEODESIC_ORDER
#define nA3   GEOGRAPHICLIB_GEODESIC_ORDER
#define nA3x  nA3
#define nC3   GEOGRAPHICLIB_GEODESIC_ORDER
#define nC3x  ((nC3 * (nC3 - 1)) / 2)
#define nC4   GEOGRAPHICLIB_GEODESIC_ORDER
#define nC4x  ((nC4 * (nC4 + 1)) / 2)
#define nC    (GEOGRAPHICLIB_GEODESIC_ORDER + 1)

static double AngNormalize(double x)
{
	x = fmod(x, 360.0);
	return (x < -180.0) ? (x + 360.0) : ((x < 180.0) ? x : (x - 360.0));
}

static double log1px(double x)
{
	volatile double y = 1 + x;
	volatile double z = y - 1;
	// Here's the explanation for this magic: y = 1 + z, exactly, and z
	// approx x, thus log(y)/z (which is nearly constant near z = 0) returns
	// a good approximation to the true log(1 + x)/x.  The multiplication x * (log(y)/z) introduces little additional error. */
	return (z == 0.0) ? x : x * log(y) / z;
}

static double atanhx(double x)
{
	double y = fabs(x); // Enforce odd parity
	y = log1px(2 * y/(1 - y))/2;
	return (x < 0.0) ? -y : y;
}

static double atan2dx(double y, double x)
{
	// In order to minimize round-off errors, this function rearranges the
	// arguments so that result of atan2 is in the range [-pi/4, pi/4] before
	// converting it to degrees and mapping the result to the correct quadrant.
	int    q = 0;
	if(fabs(y) > fabs(x)) {
		Exchange(&x, &y);
		q = 2;
	}
	if(x < 0) {
		x = -x;
		++q;
	}
	{
		// here x >= 0 and x >= abs(y), so angle is in [-pi/4, pi/4]
		double ang = atan2(y, x) / SMathConst::PiDiv180;
		switch(q) {
			// Note that atan2d(-0.0, 1.0) will return -0.  However, we expect that
			// atan2d will not be called with y = -0.  If need be, include
			//
			// case 0: ang = 0 + ang; break;
			//
			case 1: ang = ((y > 0.0) ? 180.0 : -180.0) - ang; break;
			case 2: ang =  90 - ang; break;
			case 3: ang = -90 + ang; break;
		}
		return ang;
	}
}

static double copysignx(double x, double y)
{
	return fabs(x) * (y < 0 || (y == 0 && 1/y < 0) ? -1 : 1);
}

/*static double hypotx(double x, double y)
{
	return sqrt(x * x + y * y);
}*/

double SGeo::SinCosSeries(int sinp, const SGeo::SinCosPair & rSC/*double sinx, double cosx*/, const double c[], int n)
{
	// Evaluate
	// y = sinp ? sum(c[i] * sin( 2*i    * x), i, 1, n) : sum(c[i] * cos((2*i+1) * x), i, 0, n-1)
	// using Clenshaw summation.  N.B. c[0] is unused for sin series
	// Approx operation count = (n + 5) mult and (2 * n + 2) add
	double ar, y0, y1;
	c += (n + sinp);              /* Point to one beyond last element */
	ar = 2 * (rSC.C - rSC.S) * (rSC.C + rSC.S); /* 2 * cos(2 * x) */
	y0 = n & 1 ? *--c : 0; y1 = 0;          /* accumulators for sum */
	// Now n is even
	n /= 2;
	while(n--) {
		// Unroll loop x 2, so accumulators return to their original role
		y1 = ar * y0 - y1 + *--c;
		y0 = ar * y1 - y0 + *--c;
	}
	return sinp ? (2 * rSC.S * rSC.C * y0) /* sin(2 * x) * y0 */ : (rSC.C * (y0 - y1)); /* cos(x) * (y0 - y1) */
}

static double AngRound(double x)
{
	if(x == 0.0)
		return 0.0;
	else {
		const double z = 1.0/16.0;
		volatile double y = fabs(x);
		// The compiler mustn't "simplify" z - (z - y) to y
		y = (y < z) ? (z - (z - y)) : y;
		return (x < 0) ? -y : y;
	}
}

static double sumx(double u, double v, double * t)
{
	volatile double s = u + v;
	volatile double up = s - v;
	volatile double vpp = s - up;
	up -= u;
	vpp -= v;
	ASSIGN_PTR(t, -(up + vpp));
	// error-free sum:
	// u + v =       s      + t
	//   = round(u + v) + t
	return s;
}

static double AngDiff(double x, double y, double * e)
{
	double t;
	double d = -AngNormalize(sumx(AngNormalize(x), AngNormalize(-y), &t));
	// Here y - x = d - t (mod 360), exactly, where d is in (-180,180] and
	// abs(t) <= eps (eps = 2^-45 for doubles).  The only case where the
	// addition of t takes the result outside the range (-180,180] is d = 180
	// and t < 0.  The case, d = -180 + eps, t = eps, can't happen, since
	// sum would have returned the exact result in such a case (i.e., given t = 0).
	return sumx(d == 180 && t < 0 ? -180 : d, -t, e);
}

static double LatFix(double x)
{
	return (fabs(x) > 90.0) ? fgetnan() : x;
}

static double cbrtx(double x)
{
	double y = pow(fabs(x), 1.0/3.0); // Return the real cube root
	return (x < 0.0) ? -y : y;
}

static double polyval(int N, const double p[], double x)
{
	double y = (N < 0) ? 0 : *p++;
	while(--N >= 0)
		y = y * x + *p++;
	return y;
}
//
// The scale factor A1-1 = mean value of (d/dsigma)I1 - 1
//
double A1m1f(double eps)
{
	static const double coeff[] = {
		// (1-eps)*A1-1, polynomial in eps2 of order 3
		1, 4, 64, 0, 256,
	};
	int m = nA1/2;
	double t = polyval(m, coeff, SQ(eps)) / coeff[m + 1];
	return (t + eps) / (1 - eps);
}
//
// The coefficients C1p[l] in the Fourier expansion of B1p
//
void C1pf(double eps, double c[])
{
	static const double coeff[] = {
		/* C1p[1]/eps^1, polynomial in eps2 of order 2 */
		205, -432, 768, 1536,
		/* C1p[2]/eps^2, polynomial in eps2 of order 2 */
		4005, -4736, 3840, 12288,
		/* C1p[3]/eps^3, polynomial in eps2 of order 1 */
		-225, 116, 384,
		/* C1p[4]/eps^4, polynomial in eps2 of order 1 */
		-7173, 2695, 7680,
		/* C1p[5]/eps^5, polynomial in eps2 of order 0 */
		3467, 7680,
		/* C1p[6]/eps^6, polynomial in eps2 of order 0 */
		38081, 61440,
	};
	double eps2 = SQ(eps);
	double d = eps;
	int o = 0, l;
	for(l = 1; l <= nC1p; ++l) { /* l is index of C1p[l] */
		int m = (nC1p - l) / 2;     /* order of polynomial in eps^2 */
		c[l] = d * polyval(m, coeff + o, eps2) / coeff[o + m + 1];
		o += m + 2;
		d *= eps;
	}
}
//
// The coefficients C2[l] in the Fourier expansion of B2
//
void C2f(double eps, double c[])
{
	static const double coeff[] = {
		/* C2[1]/eps^1, polynomial in eps2 of order 2 */
		1, 2, 16, 32,
		/* C2[2]/eps^2, polynomial in eps2 of order 2 */
		35, 64, 384, 2048,
		/* C2[3]/eps^3, polynomial in eps2 of order 1 */
		15, 80, 768,
		/* C2[4]/eps^4, polynomial in eps2 of order 1 */
		7, 35, 512,
		/* C2[5]/eps^5, polynomial in eps2 of order 0 */
		63, 1280,
		/* C2[6]/eps^6, polynomial in eps2 of order 0 */
		77, 2048,
	};
	double eps2 = SQ(eps);
	double d = eps;
	int o = 0, l;
	for(l = 1; l <= nC2; ++l) { /* l is index of C2[l] */
		int m = (nC2 - l) / 2;     /* order of polynomial in eps^2 */
		c[l] = d * polyval(m, coeff + o, eps2) / coeff[o + m + 1];
		o += m + 2;
		d *= eps;
	}
}
//
// The scale factor A2-1 = mean value of (d/dsigma)I2 - 1
//
double A2m1f(double eps)
{
	static const double coeff[] = {
		// (eps+1)*A2-1, polynomial in eps2 of order 3
		-11, -28, -192, 0, 256,
	};
	int m = nA2/2;
	double t = polyval(m, coeff, SQ(eps)) / coeff[m + 1];
	return (t - eps) / (1 + eps);
}

//double A3f(const SGeo::Geodesic * g, double eps)
double SGeo::Geodesic::A3f_(double eps) const
{
	// Evaluate A3
	return polyval(nA3 - 1, A3x, eps);
}

//void C3f(const SGeo::Geodesic * g, double eps, double c[])
void SGeo::Geodesic::C3f_(double eps, double c[]) const
{
	// Evaluate C3 coeffs
	// Elements c[1] thru c[nC3 - 1] are set
	double mult = 1;
	int o = 0, l;
	for(l = 1; l < nC3; ++l) {   /* l is index of C3[l] */
		int m = nC3 - l - 1;        /* order of polynomial in eps */
		mult *= eps;
		c[l] = mult * polyval(m, C3x + o, eps);
		o += m + 1;
	}
}

//void C4f(const SGeo::Geodesic * g, double eps, double c[])
void SGeo::Geodesic::C4f_(double eps, double c[]) const
{
	// Evaluate C4 coeffs
	// Elements c[0] thru c[nC4 - 1] are set
	double mult = 1;
	int o = 0/*, l*/;
	for(uint i = 0; i < nC4; ++i) { // i is index of C4[l]
		const int m = nC4 - (int)i - 1; // order of polynomial in eps
		c[i] = mult * polyval(m, C4x + o, eps);
		o += (m + 1);
		mult *= eps;
	}
}
//
// The coefficients C1[l] in the Fourier expansion of B1
//
void C1f(double eps, double c[])
{
	static const double coeff[] = {
		/* C1[1]/eps^1, polynomial in eps2 of order 2 */
		-1, 6, -16, 32,
		/* C1[2]/eps^2, polynomial in eps2 of order 2 */
		-9, 64, -128, 2048,
		/* C1[3]/eps^3, polynomial in eps2 of order 1 */
		9, -16, 768,
		/* C1[4]/eps^4, polynomial in eps2 of order 1 */
		3, -5, 512,
		/* C1[5]/eps^5, polynomial in eps2 of order 0 */
		-7, 1280,
		/* C1[6]/eps^6, polynomial in eps2 of order 0 */
		-7, 2048,
	};
	double eps2 = SQ(eps);
	double d = eps;
	int o = 0, l;
	for(l = 1; l <= nC1; ++l) { // l is index of C1p[l]
		int m = (nC1 - l) / 2; // order of polynomial in eps^2
		c[l] = d * polyval(m, coeff + o, eps2) / coeff[o + m + 1];
		o += m + 2;
		d *= eps;
	}
}
//
// The scale factor A3 = mean value of (d/dsigma)I3
//
//void A3coeff(SGeo::Geodesic * g)
void SGeo::Geodesic::A3coeff_()
{
	static const double coeff[] = {
		/* A3, coeff of eps^5, polynomial in n of order 0 */
		-3, 128,
		/* A3, coeff of eps^4, polynomial in n of order 1 */
		-2, -3, 64,
		/* A3, coeff of eps^3, polynomial in n of order 2 */
		-1, -3, -1, 16,
		/* A3, coeff of eps^2, polynomial in n of order 2 */
		3, -1, -2, 8,
		/* A3, coeff of eps^1, polynomial in n of order 1 */
		1, -1, 2,
		/* A3, coeff of eps^0, polynomial in n of order 0 */
		1, 1,
	};
	int o = 0, k = 0, j;
	for(j = nA3 - 1; j >= 0; --j) { // coeff of eps^j
		int m = ((nA3 - j - 1) < j) ? (nA3 - j - 1) : j; // order of polynomial in n
		A3x[k++] = polyval(m, coeff + o, N) / coeff[o + m + 1];
		o += m + 2;
	}
}
//
// The coefficients C3[l] in the Fourier expansion of B3
//
//void C3coeff(SGeo::Geodesic * g)
void SGeo::Geodesic::C3coeff_()
{
	static const double coeff[] = {
		/* C3[1], coeff of eps^5, polynomial in n of order 0 */
		3, 128,
		/* C3[1], coeff of eps^4, polynomial in n of order 1 */
		2, 5, 128,
		/* C3[1], coeff of eps^3, polynomial in n of order 2 */
		-1, 3, 3, 64,
		/* C3[1], coeff of eps^2, polynomial in n of order 2 */
		-1, 0, 1, 8,
		/* C3[1], coeff of eps^1, polynomial in n of order 1 */
		-1, 1, 4,
		/* C3[2], coeff of eps^5, polynomial in n of order 0 */
		5, 256,
		/* C3[2], coeff of eps^4, polynomial in n of order 1 */
		1, 3, 128,
		/* C3[2], coeff of eps^3, polynomial in n of order 2 */
		-3, -2, 3, 64,
		/* C3[2], coeff of eps^2, polynomial in n of order 2 */
		1, -3, 2, 32,
		/* C3[3], coeff of eps^5, polynomial in n of order 0 */
		7, 512,
		/* C3[3], coeff of eps^4, polynomial in n of order 1 */
		-10, 9, 384,
		/* C3[3], coeff of eps^3, polynomial in n of order 2 */
		5, -9, 5, 192,
		/* C3[4], coeff of eps^5, polynomial in n of order 0 */
		7, 512,
		/* C3[4], coeff of eps^4, polynomial in n of order 1 */
		-14, 7, 512,
		/* C3[5], coeff of eps^5, polynomial in n of order 0 */
		21, 2560,
	};
	int o = 0, k = 0, l, j;
	for(l = 1; l < nC3; ++l) { // l is index of C3[l]
		for(j = nC3 - 1; j >= l; --j) { // coeff of eps^j
			int m = ((nC3 - j - 1) < j) ? (nC3 - j - 1) : j; // order of polynomial in n
			C3x[k++] = polyval(m, coeff + o, N) / coeff[o + m + 1];
			o += m + 2;
		}
	}
}
//
// The coefficients C4[l] in the Fourier expansion of I4
//
void SGeo::Geodesic::C4coeff_(/*SGeo::Geodesic * g*/)
//void C4coeff(SGeo::Geodesic * g)
{
	static const double coeff[] = {
		/* C4[0], coeff of eps^5, polynomial in n of order 0 */
		97, 15015,
		/* C4[0], coeff of eps^4, polynomial in n of order 1 */
		1088, 156, 45045,
		/* C4[0], coeff of eps^3, polynomial in n of order 2 */
		-224, -4784, 1573, 45045,
		/* C4[0], coeff of eps^2, polynomial in n of order 3 */
		-10656, 14144, -4576, -858, 45045,
		/* C4[0], coeff of eps^1, polynomial in n of order 4 */
		64, 624, -4576, 6864, -3003, 15015,
		/* C4[0], coeff of eps^0, polynomial in n of order 5 */
		100, 208, 572, 3432, -12012, 30030, 45045,
		/* C4[1], coeff of eps^5, polynomial in n of order 0 */
		1, 9009,
		/* C4[1], coeff of eps^4, polynomial in n of order 1 */
		-2944, 468, 135135,
		/* C4[1], coeff of eps^3, polynomial in n of order 2 */
		5792, 1040, -1287, 135135,
		/* C4[1], coeff of eps^2, polynomial in n of order 3 */
		5952, -11648, 9152, -2574, 135135,
		/* C4[1], coeff of eps^1, polynomial in n of order 4 */
		-64, -624, 4576, -6864, 3003, 135135,
		/* C4[2], coeff of eps^5, polynomial in n of order 0 */
		8, 10725,
		/* C4[2], coeff of eps^4, polynomial in n of order 1 */
		1856, -936, 225225,
		/* C4[2], coeff of eps^3, polynomial in n of order 2 */
		-8448, 4992, -1144, 225225,
		/* C4[2], coeff of eps^2, polynomial in n of order 3 */
		-1440, 4160, -4576, 1716, 225225,
		/* C4[3], coeff of eps^5, polynomial in n of order 0 */
		-136, 63063,
		/* C4[3], coeff of eps^4, polynomial in n of order 1 */
		1024, -208, 105105,
		/* C4[3], coeff of eps^3, polynomial in n of order 2 */
		3584, -3328, 1144, 315315,
		/* C4[4], coeff of eps^5, polynomial in n of order 0 */
		-128, 135135,
		/* C4[4], coeff of eps^4, polynomial in n of order 1 */
		-2560, 832, 405405,
		/* C4[5], coeff of eps^5, polynomial in n of order 0 */
		128, 99099,
	};
	int o = 0, k = 0, l, j;
	for(l = 0; l < nC4; ++l) {        /* l is index of C4[l] */
		for(j = nC4 - 1; j >= l; --j) { /* coeff of eps^j */
			int m = nC4 - j - 1;           /* order of polynomial in n */
			C4x[k++] = polyval(m, coeff + o, N) / coeff[o + m + 1];
			o += m + 2;
		}
	}
}

void SGeo::Geodesic::Init(double _a, double _f)
{
	const double _tol2 = sqrt(SMathConst::Epsilon);

	A = _a;
	F = _f;
	F1 = 1 - F;
	E2 = F * (2 - F);
	Ep2 = E2 / SQ(F1); //  e2 / (1 - e2)
	N = F / (2.0 - F);
	B = A * F1;
	C2 = (SQ(A) + SQ(B) * ((E2 == 0) ? 1.0 : ((E2 > 0.0) ? atanhx(sqrt(E2)) : atan(sqrt(-E2))) / sqrt(fabs(E2)))) / 2.0; // authalic radius squared
	//
	// The sig12 threshold for "really short".  Using the auxiliary sphere
	// solution with dnm computed at (bet1 + bet2) / 2, the relative error in the
	// azimuth consistency check is sig12^2 * abs(f) * min(1, 1-f/2) / 2.  (Error
	// measured for 1/100 < b/a < 100 and abs(f) >= 1/1000.  For a given f and
	// sig12, the max error occurs for lines near the pole.  If the old rule for
	// computing dnm = (dn1 + dn2)/2 is used, then the error increases by a
	// factor of 2.)  Setting this equal to epsilon gives sig12 = etol2.  Here
	// 0.1 is a safety factor (error decreased by 100) and max(0.001, abs(f))
	// stops etol2 getting too large in the nearly spherical case.
	//
	Etol2 = 0.1 * _tol2 / sqrt(smax(0.001, fabs(F)) * smin(1.0, 1 - F/2) / 2 );
	A3coeff_();
	C3coeff_();
	C4coeff_();
}

SGeo::Geodesic::Geodesic(double _a, double _f)
{
	Init(_a, _f);
}

SGeo::Geodesic::Geodesic()
{
	Init(SGeo::A_WGS84, SGeo::Flattening_WGS84);
}

SGeo::SinCosPair::SinCosPair() : S(0.0), C(0.0)
{
}

SGeo::SinCosPair::SinCosPair(const SGeo::SinCosPair & rS) : S(rS.S), C(rS.C)
{
}

SGeo::SinCosPair::SinCosPair(double angle)
{
	SetRad(angle);
}

SGeo::SinCosPair::SinCosPair(double _sin, double _cos) : S(_sin), C(_cos)
{
}

SGeo::SinCosPair & FASTCALL SGeo::SinCosPair::operator = (const SGeo::SinCosPair & rS)
{
	S = rS.S;
	C = rS.C;
	return *this;
}

void SGeo::SinCosPair::SetRad(double angle)
{
	S = sin(angle);
	C = cos(angle);
}

void SGeo::SinCosPair::Set_SinCosDX(double x)
{
	// In order to minimize round-off errors, this function exactly reduces
	// the argument to the range [-45, 45] before converting it to radians.
	double r = fmod(x, 360.0);
	int    q = ffloori(r / 90.0 + 0.5);
	r -= 90 * q;
	// now abs(r) <= 45
	r *= SMathConst::PiDiv180;
	// Possibly could call the gnu extension sincos
	//double s = sin(r);
	//double c = cos(r);
	SGeo::SinCosPair sc_(r);
	switch((uint)q & 3U) {
		case 0U: 
			S = sc_.S;
			C = sc_.C;
			break;
		case 1U: 
			S = sc_.C; 
			C = 0.0 - sc_.S; 
			break;
		case 2U: 
			S = 0.0 - sc_.S; 
			C = 0.0 - sc_.C; 
			break;
		default: // case 3U
			S = 0.0 - sc_.C; 
			C = sc_.S; 
			break; 
	}
}

SGeo::SinCosPair & SGeo::SinCosPair::Sum_(const SinCosPair & r1, const SinCosPair & r2)
{
	S = r1.S * r2.C + r1.C * r2.S;
	C = r1.C * r2.C - r1.S * r2.S;
	return *this;
}

double SGeo::SinCosPair::ATan2() const
{
	return atan2(S, C);
}

void SGeo::SinCosPair::Norm2()
{
	const double r = hypot(S, C);
	S /= r;
	C /= r;
}

SGeo::SGeo() :
	_RealMin(SMathConst::Min),
	_Tiny(sqrt(_RealMin)),
	_Tol0(SMathConst::Epsilon),
	_Tol1(200.0 * _Tol0),
	_Tol2(sqrt(_Tol0)),
	_Tolb(_Tol0 * _Tol2), // Check on bisection interval
	_MaxIt1(20),
	_MaxIt2(_MaxIt1 + DBL_MANT_DIG + 10),
	G()
{
}

void SGeo::LineInit_Int(SGeo::GeodesicLine * pLine, const SGeoPosLL & rP1, double azi1, const SinCosPair & rAlp1, uint caps)
{
	pLine->A = G.A;
	pLine->F = G.F;
	pLine->B = G.B;
	pLine->C2 = G.C2;
	pLine->F1 = G.F1;
	// If caps is 0 assume the standard direct calculation
	pLine->Caps = (caps ? caps : GEOD_DISTANCE_IN | GEOD_LONGITUDE) | /* always allow latitude and azimuth and unrolling of longitude */ GEOD_LATITUDE | GEOD_AZIMUTH | GEOD_LONG_UNROLL;

	pLine->P1.Lat = LatFix(rP1.Lat);
	pLine->P1.Lon = rP1.Lon;
	pLine->Azi1 = azi1;
	pLine->Alp1 = rAlp1;
	{
		//double cbet1;
		//double sbet1;
		SinCosPair sc_bet1;
		double eps;
		sc_bet1.Set_SinCosDX(AngRound(pLine->P1.Lat)); 
		sc_bet1.S *= pLine->F1;
		// Ensure cbet1 = +epsilon at poles
		sc_bet1.Norm2();
		sc_bet1.C = smax(_Tiny, sc_bet1.C);
		pLine->Dn1 = sqrt(1 + G.Ep2 * SQ(sc_bet1.S));
		// Evaluate alp0 from sin(alp1) * cos(bet1) = sin(alp0),
		//- pLine->Alp0.S = pLine->Alp1.S * sc_bet1.C; // alp0 in [0, pi/2 - |bet1|] 
		// Alt: calp0 = hypot(sbet1, calp1 * cbet1).  The following
		// is slightly better (consider the case salp1 = 0).
		//- pLine->Alp0.C = hypot(pLine->Alp1.C, pLine->Alp1.S * sc_bet1.S);
		pLine->Alp0.Set(pLine->Alp1.S * sc_bet1.C, hypot(pLine->Alp1.C, pLine->Alp1.S * sc_bet1.S)); // alp0 in [0, pi/2 - |bet1|] 
		// Evaluate sig with tan(bet1) = tan(sig1) * cos(alp1).
		// sig = 0 is nearest northward crossing of equator.
		// With bet1 = 0, alp1 = pi/2, we have sig1 = 0 (equatorial line).
		// With bet1 =  pi/2, alp1 = -pi, sig1 =  pi/2
		// With bet1 = -pi/2, alp1 =  0 , sig1 = -pi/2
		// Evaluate omg1 with tan(omg1) = sin(alp0) * tan(sig1).
		// With alp0 in (0, pi/2], quadrants for sig and omg coincide.
		// No atan2(0,0) ambiguity at poles since cbet1 = +epsilon.
		// With alp0 = 0, omg1 = 0 for alp1 = 0, omg1 = pi for alp1 = pi.
		pLine->Sig1.S = sc_bet1.S; 
		pLine->Omg1.S = pLine->Alp0.S * sc_bet1.S;
		pLine->Sig1.C = pLine->Omg1.C = (sc_bet1.S != 0 || pLine->Alp1.C != 0) ? (sc_bet1.C * pLine->Alp1.C) : 1.0;
		pLine->Sig1.Norm2(); // sig1 in (-pi, pi] 
		// norm2(somg1, comg1); -- don't need to normalize!
		pLine->K2 = SQ(pLine->Alp0.C) * G.Ep2;
		eps = pLine->K2 / (2 * (1 + sqrt(1 + pLine->K2)) + pLine->K2);
		if(pLine->Caps & CAP_C1) {
			pLine->A1m1 = A1m1f(eps);
			C1f(eps, pLine->C1a);
			pLine->B11 = SinCosSeries(TRUE, pLine->Sig1, pLine->C1a, nC1);
			{
				//const double s = sin(pLine->B11); 
				//const double c = cos(pLine->B11);
				//const SinCosPair sc_(pLine->B11);
				// tau1 = sig1 + B11
				pLine->Tau1.Sum_(pLine->Sig1, SinCosPair(pLine->B11));
				//
				// Not necessary because C1pa reverts C1a B11 = -SinCosSeries(TRUE, stau1, ctau1, C1pa, nC1p);
			}
		}
		if(pLine->Caps & CAP_C1p)
			C1pf(eps, pLine->C1pa);
		if(pLine->Caps & CAP_C2) {
			pLine->A2m1 = A2m1f(eps);
			C2f(eps, pLine->C2a);
			pLine->B21 = SinCosSeries(TRUE, pLine->Sig1, pLine->C2a, nC2);
		}
		if(pLine->Caps & CAP_C3) {
			G.C3f_(eps, pLine->C3a);
			pLine->A3c = -pLine->F * pLine->Alp0.S * G.A3f_(eps);
			pLine->B31 = SinCosSeries(TRUE, pLine->Sig1, pLine->C3a, nC3-1);
		}
		if(pLine->Caps & CAP_C4) {
			G.C4f_(eps, pLine->C4a);
			// Multiplier = a^2 * e^2 * cos(alpha0) * sin(alpha0)
			pLine->A4 = SQ(pLine->A) * pLine->Alp0.C * pLine->Alp0.S * G.E2;
			pLine->B41 = SinCosSeries(FALSE, pLine->Sig1, pLine->C4a, nC4);
		}
		pLine->A13 = pLine->S13 = fgetnan();
	}
}

void SGeo::LineInit(GeodesicLine * pLine, /*const SGeo::Geodesic & rG,*/const SGeoPosLL & rP1, double azi1, uint caps)
{
	azi1 = AngNormalize(azi1);
	SinCosPair sc_alp1;
	// Guard against underflow in salp0
	sc_alp1.Set_SinCosDX(AngRound(azi1));
	LineInit_Int(pLine, rP1, azi1, sc_alp1/*sc_alp1.S, sc_alp1.C*/, caps);
}

double SGeo::GenPosition(const SGeo::GeodesicLine * pLine, uint flags, double s12_a12,
	SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12)
{
	SGeoPosLL pt2;
	double azi2 = 0;
	double s12 = 0;
	double m12 = 0;
	double M12 = 0;
	double M21 = 0;
	double S12 = 0;
	// Avoid warning about uninitialized B12
	double sig12;
	SinCosPair sc_sig12;
	double B12 = 0.0;
	double AB1 = 0.0;
	double omg12;
	double lam12;
	double lon12;
	SinCosPair sc_sig2;
	SinCosPair sc_bet2;
	SinCosPair sc_omg2;
	SinCosPair sc_alp2;
	double dn2;
	uint   outmask =
		//(plat2 ? GEOD_LATITUDE : 0U) | (plon2 ? GEOD_LONGITUDE : 0U) |
		(pP2 ? (GEOD_LATITUDE|GEOD_LONGITUDE) : 0U) |
		(pazi2 ? GEOD_AZIMUTH : 0U) | (ps12 ? GEOD_DISTANCE : 0U) | (pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) |
		(pS12 ? GEOD_AREA : 0U);
	outmask &= (pLine->Caps & OUT_ALL);
	if(!(TRUE /*Init()*/ && (flags & GEOD_ARCMODE || (pLine->Caps & (GEOD_DISTANCE_IN & OUT_ALL)))))
		return fgetnan(); // Uninitialized or impossible distance calculation requested
	if(flags & GEOD_ARCMODE) {
		// Interpret s12_a12 as spherical arc length
		sig12 = s12_a12 * SMathConst::PiDiv180;
		sc_sig12.Set_SinCosDX(s12_a12);
	}
	else {
		// Interpret s12_a12 as distance
		double tau12 = s12_a12 / (pLine->B * (1 + pLine->A1m1));
		SinCosPair sc_(tau12);
		{
			// tau2 = tau1 + tau12
			SinCosPair tau_sum;
			tau_sum.Sum_(pLine->Tau1, sc_);
			B12 = -SinCosSeries(TRUE, tau_sum/*pLine->Tau1.S * sc_.C + pLine->Tau1.C * sc_.S, pLine->Tau1.C * sc_.C - pLine->Tau1.S * sc_.S*/, pLine->C1pa, nC1p);
		}
		sig12 = tau12 - (B12 - pLine->B11);
		sc_sig12.SetRad(sig12); 
		if(fabs(pLine->F) > 0.01) {
			/* Reverted distance series is inaccurate for |f| > 1/100, so correct
			* sig12 with 1 Newton iteration.  The following table shows the
			* approximate maximum error for a = WGS_a() and various f relative to
			* GeodesicExact.
			*     erri = the error in the inverse solution (nm)
			*     errd = the error in the direct solution (series only) (nm)
			*     errda = the error in the direct solution (series + 1 Newton) (nm)
			*
			*       f     erri  errd errda
			*     -1/5    12e6 1.2e9  69e6
			*     -1/10  123e3  12e6 765e3
			*     -1/20   1110 108e3  7155
			*     -1/50  18.63 200.9 27.12
			*     -1/100 18.63 23.78 23.37
			*     -1/150 18.63 21.05 20.26
			*      1/150 22.35 24.73 25.83
			*      1/100 22.35 25.03 25.31
			*      1/50  29.80 231.9 30.44
			*      1/20   5376 146e3  10e3
			*      1/10  829e3  22e6 1.5e6
			*      1/5   157e6 3.8e9 280e6 */
			double serr;
			//sc_sig2.S = pLine->Sig1.S * sc_sig12.C + pLine->Sig1.C * sc_sig12.S;
			//sc_sig2.C = pLine->Sig1.C * sc_sig12.C - pLine->Sig1.S * sc_sig12.S;
			sc_sig2.Sum_(pLine->Sig1, sc_sig12);
			B12 = SinCosSeries(TRUE, sc_sig2, pLine->C1a, nC1);
			serr = (1 + pLine->A1m1) * (sig12 + (B12 - pLine->B11)) - s12_a12 / pLine->B;
			sig12 = sig12 - serr / sqrt(1 + pLine->K2 * SQ(sc_sig2.S));
			sc_sig12.SetRad(sig12);
			// Update B12 below
		}
	}
	// sig2 = sig1 + sig12
	//sc_sig2.S = pLine->Sig1.S * sc_sig12.C + pLine->Sig1.C * sc_sig12.S;
	//sc_sig2.C = pLine->Sig1.C * sc_sig12.C - pLine->Sig1.S * sc_sig12.S;
	sc_sig2.Sum_(pLine->Sig1, sc_sig12);
	dn2 = sqrt(1 + pLine->K2 * SQ(sc_sig2.S));
	if(outmask & (GEOD_DISTANCE | GEOD_REDUCEDLENGTH | GEOD_GEODESICSCALE)) {
		if(flags & GEOD_ARCMODE || fabs(pLine->F) > 0.01)
			B12 = SinCosSeries(TRUE, sc_sig2, pLine->C1a, nC1);
		AB1 = (1 + pLine->A1m1) * (B12 - pLine->B11);
	}
	// sin(bet2) = cos(alp0) * sin(sig2)
	//- sc_bet2.S = pLine->Alp0.C * sc_sig2.S;
	// Alt: cbet2 = hypot(csig2, salp0 * ssig2);
	//- sc_bet2.C = hypot(pLine->Alp0.S, pLine->Alp0.C * sc_sig2.C);
	sc_bet2.Set(pLine->Alp0.C * sc_sig2.S, hypot(pLine->Alp0.S, pLine->Alp0.C * sc_sig2.C));
	if(sc_bet2.C == 0) {
	    // I.e., salp0 = 0, csig2 = 0.  Break the degeneracy in this case
		sc_bet2.C = sc_sig2.C = _Tiny;
	}
	// tan(alp0) = cos(sig2)*tan(alp2)
	//- sc_alp2.S = pLine->Alp0.S;
	//- sc_alp2.C = pLine->Alp0.C * sc_sig2.C; // No need to normalize 
	sc_alp2.Set(pLine->Alp0.S, pLine->Alp0.C * sc_sig2.C); // No need to normalize 
	if(outmask & GEOD_DISTANCE)
		s12 = (flags & GEOD_ARCMODE) ? (pLine->B * ((1 + pLine->A1m1) * sig12 + AB1)) : s12_a12;
	if(outmask & GEOD_LONGITUDE) {
		double E = copysignx(1, pLine->Alp0.S); // east or west going?
		// tan(omg2) = sin(alp0) * tan(sig2)
		//- sc_omg2.S = pLine->Alp0.S * sc_sig2.S;
		//- sc_omg2.C = sc_sig2.C; // No need to normalize
		sc_omg2.Set(pLine->Alp0.S * sc_sig2.S, sc_sig2.C); // No need to normalize
		// omg12 = omg2 - omg1
		omg12 = (flags & GEOD_LONG_UNROLL) ? (E * (sig12 - (sc_sig2.ATan2() - pLine->Sig1.ATan2()) +
			(atan2(E * sc_omg2.S, sc_omg2.C) - atan2(E * pLine->Omg1.S, pLine->Omg1.C)))) : atan2(sc_omg2.S * pLine->Omg1.C - sc_omg2.C * pLine->Omg1.S, sc_omg2.C * pLine->Omg1.C + sc_omg2.S * pLine->Omg1.S);
		lam12 = omg12 + pLine->A3c * (sig12 + (SinCosSeries(TRUE, sc_sig2, pLine->C3a, nC3-1) - pLine->B31));
		lon12 = lam12 / SMathConst::PiDiv180;
		pt2.Lon = (flags & GEOD_LONG_UNROLL) ? (pLine->P1.Lon + lon12) : (AngNormalize(AngNormalize(pLine->P1.Lon) + AngNormalize(lon12)));
	}
	if(outmask & GEOD_LATITUDE)
		pt2.Lat = atan2dx(sc_bet2.S, pLine->F1 * sc_bet2.C);
	if(outmask & GEOD_AZIMUTH)
		azi2 = atan2dx(sc_alp2.S, sc_alp2.C);
	if(outmask & (GEOD_REDUCEDLENGTH | GEOD_GEODESICSCALE)) {
		const double B22 = SinCosSeries(TRUE, sc_sig2, pLine->C2a, nC2);
		const double AB2 = (1 + pLine->A2m1) * (B22 - pLine->B21);
		const double J12 = (pLine->A1m1 - pLine->A2m1) * sig12 + (AB1 - AB2);
		if(outmask & GEOD_REDUCEDLENGTH) {
			// Add parens around (csig1 * ssig2) and (ssig1 * csig2) to ensure
			// accurate cancellation in the case of coincident points.
			m12 = pLine->B * ((dn2 * (pLine->Sig1.C * sc_sig2.S) - pLine->Dn1 * (pLine->Sig1.S * sc_sig2.C)) - pLine->Sig1.C * sc_sig2.C * J12);
		}
		if(outmask & GEOD_GEODESICSCALE) {
			const double t = pLine->K2 * (sc_sig2.S - pLine->Sig1.S) * (sc_sig2.S + pLine->Sig1.S) / (pLine->Dn1 + dn2);
			M12 = sc_sig12.C + (t * sc_sig2.S - sc_sig2.C * J12) * pLine->Sig1.S / pLine->Dn1;
			M21 = sc_sig12.C - (t * pLine->Sig1.S - pLine->Sig1.C * J12) * sc_sig2.S /  dn2;
		}
	}
	if(outmask & GEOD_AREA) {
		double B42 = SinCosSeries(FALSE, sc_sig2, pLine->C4a, nC4);
		SinCosPair sc_alp12;
		if(pLine->Alp0.C == 0 || pLine->Alp0.S == 0) {
			// alp12 = alp2 - alp1, used in atan2 so no need to normalize
			//- sc_alp12.S = sc_alp2.S * pLine->Alp1.C - sc_alp2.C * pLine->Alp1.S;
			//- sc_alp12.C = sc_alp2.C * pLine->Alp1.C + sc_alp2.S * pLine->Alp1.S;
			sc_alp12.Set(sc_alp2.S * pLine->Alp1.C - sc_alp2.C * pLine->Alp1.S, sc_alp2.C * pLine->Alp1.C + sc_alp2.S * pLine->Alp1.S);
		}
		else {
			// 
			// tan(alp) = tan(alp0) * sec(sig)
			// tan(alp2-alp1) = (tan(alp2) -tan(alp1)) / (tan(alp2)*tan(alp1)+1) = calp0 * salp0 * (csig1-csig2) / (salp0^2 + calp0^2 * csig1*csig2)
			// If csig12 > 0, write
			//   csig1 - csig2 = ssig12 * (csig1 * ssig12 / (1 + csig12) + ssig1)
			// else
			//   csig1 - csig2 = csig1 * (1 - csig12) + ssig12 * ssig1
			// No need to normalize 
			// 
			sc_alp12.S = pLine->Alp0.C * pLine->Alp0.S * ((sc_sig12.C <= 0) ? (pLine->Sig1.C * (1 - sc_sig12.C) + sc_sig12.S * pLine->Sig1.S) : (sc_sig12.S * (pLine->Sig1.C * sc_sig12.S / (1 + sc_sig12.C) + pLine->Sig1.S)));
			sc_alp12.C = SQ(pLine->Alp0.S) + SQ(pLine->Alp0.C) * pLine->Sig1.C * sc_sig2.C;
		}
		S12 = pLine->C2 * sc_alp12.ATan2() + pLine->A4 * (B42 - pLine->B41);
	}
	if(outmask & (GEOD_LATITUDE|GEOD_LONGITUDE))
		ASSIGN_PTR(pP2, pt2);
	if(outmask & GEOD_AZIMUTH)
		*pazi2 = azi2;
	if(outmask & GEOD_DISTANCE)
		*ps12 = s12;
	if(outmask & GEOD_REDUCEDLENGTH)
		*pm12 = m12;
	if(outmask & GEOD_GEODESICSCALE) {
		ASSIGN_PTR(pM12, M12);
		ASSIGN_PTR(pM21, M21);
	}
	if(outmask & GEOD_AREA)
		*pS12 = S12;
	return (flags & GEOD_ARCMODE) ? s12_a12 : (sig12 / SMathConst::PiDiv180);
}

double SGeo::Direct(/*const Geodesic & rG,*/const SGeoPosLL & rP1, double azi1, uint flags, double s12_a12,
	SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12)
{
	GeodesicLine l;
	uint outmask = // (plat2 ? GEOD_LATITUDE : 0U) | (plon2 ? GEOD_LONGITUDE : 0U) |
		(pP2 ? (GEOD_LATITUDE|GEOD_LONGITUDE) : 0) |
		(pazi2 ? GEOD_AZIMUTH : 0U) | (ps12 ? GEOD_DISTANCE : 0U) |
		(pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) | (pS12 ? GEOD_AREA : 0U);
	// Automatically supply GEOD_DISTANCE_IN if necessary
	LineInit(&l, rP1, azi1, outmask | (flags & GEOD_ARCMODE ? GEOD_NONE : GEOD_DISTANCE_IN));
	return GenPosition(&l, flags, s12_a12, pP2, pazi2, ps12, pm12, pM12, pM21, pS12);
}


//void   Lengths(double eps, double sig12, const SinCosPair & rSig1/*double ssig1, double csig1*/, double dn1,
// const SinCosPair & rSig2/*double ssig2, double csig2*/, double dn2, double cbet1, double cbet2, double* ps12b, double* pm12b, double* pm0, double* pM12, double* pM21,
// /* Scratch area of the right size */ double Ca[]);

void SGeo::Lengths(double eps, double sig12, const SinCosPair & rSig1/*double ssig1, double csig1*/, double dn1,
	const SinCosPair & rSig2/*double ssig2, double csig2*/, double dn2, double cbet1, double cbet2, double * ps12b, double * pm12b, double * pm0, double * pM12, double * pM21,
	/* Scratch area of the right size */ double Ca[])
{
	double m0 = 0;
	double J12 = 0;
	double A1 = 0;
	double A2 = 0;
	double Cb[nC];
	// Return m12b = (reduced length)/b; also calculate s12b = distance/b,
	// and m0 = coefficient of secular term in expression for reduced length.
	const int redlp = (pm12b || pm0 || pM12 || pM21); // bool
	if(ps12b || redlp) {
		A1 = A1m1f(eps);
		C1f(eps, Ca);
		if(redlp) {
			A2 = A2m1f(eps);
			C2f(eps, Cb);
			m0 = A1 - A2;
			A2 = 1 + A2;
		}
		A1 = 1 + A1;
	}
	if(ps12b) {
		double B1 = SinCosSeries(TRUE, rSig2, Ca, nC1) - SinCosSeries(TRUE, rSig1, Ca, nC1);
		// Missing a factor of b
		*ps12b = A1 * (sig12 + B1);
		if(redlp) {
			double B2 = SinCosSeries(TRUE, rSig2, Cb, nC2) - SinCosSeries(TRUE, rSig1, Cb, nC2);
			J12 = m0 * sig12 + (A1 * B1 - A2 * B2);
		}
	}
	else if(redlp) {
		// Assume here that nC1 >= nC2
		for(int l = 1; l <= nC2; ++l)
			Cb[l] = A1 * Ca[l] - A2 * Cb[l];
		J12 = m0 * sig12 + (SinCosSeries(TRUE, rSig2, Cb, nC2) - SinCosSeries(TRUE, rSig1, Cb, nC2));
	}
	ASSIGN_PTR(pm0, m0);
	if(pm12b) {
		// Missing a factor of b.
		// Add parens around (csig1 * ssig2) and (ssig1 * csig2) to ensure
		// accurate cancellation in the case of coincident points.
		*pm12b = dn2 * (rSig1.C * rSig2.S) - dn1 * (rSig1.S * rSig2.C) - rSig1.C * rSig2.C * J12;
	}
	if(pM12 || pM21) {
		double csig12 = rSig1.C * rSig2.C + rSig1.S * rSig2.S;
		double t = G.Ep2 * (cbet1 - cbet2) * (cbet1 + cbet2) / (dn1 + dn2);
		if(pM12)
			*pM12 = csig12 + (t * rSig2.S - rSig2.C * J12) * rSig1.S / dn1;
		if(pM21)
			*pM21 = csig12 - (t * rSig1.S - rSig1.C * J12) * rSig2.S / dn2;
	}
}

double Astroid(double x, double y)
{
	// Solve k^4+2*k^3-(x^2+y^2-1)*k^2-2*y^2*k-y^2 = 0 for positive root k.
	// This solution is adapted from Geocentric::Reverse.
	double k;
	double p = SQ(x);
	double q = SQ(y);
	double r = (p + q - 1) / 6;
	if(!(q == 0 && r <= 0)) {
		// Avoid possible division by zero when r = 0 by multiplying equations for s and t by r^3 and r, resp.
		double S = p * q / 4; // S = r^3 * s
		double r2 = SQ(r);
		double r3 = r * r2;
		// The discriminant of the quadratic equation for T3.  This is zero on
		// the evolute curve p^(1/3)+q^(1/3) = 1
		double disc = S * (S + 2 * r3);
		double u = r;
		double v, uv, w;
		if(disc >= 0) {
			double T3 = S + r3;
			double T;
			//
			// Pick the sign on the sqrt to maximize abs(T3).  This minimizes loss
			// of precision due to cancellation.  The result is unchanged because
			// of the way the T is used in definition of u.
			//
			T3 += (T3 < 0) ? -sqrt(disc) : sqrt(disc); // T3 = (r * t)^3
			// N.B. cbrtx always returns the double root.  cbrtx(-8) = -2.
			T = cbrtx(T3); // T = r * t
			// T can be zero; but then r2 / T -> 0.
			u += T + ((T != 0) ? (r2 / T) : 0);
		}
		else {
			// T is complex, but the way u is defined the result is double.
			double ang = atan2(sqrt(-disc), -(S + r3));
			// There are three possible cube roots.  We choose the root which
			// avoids cancellation.  Note that disc < 0 implies that r < 0.
			u += 2 * r * cos(ang / 3);
		}
		v = sqrt(SQ(u) + q); // guaranteed positive
		// Avoid loss of accuracy when u < 0.
		uv = (u < 0) ? (q / (v - u)) : (u + v); // u+v, guaranteed positive 
		w = (uv - q) / (2 * v); // positive? 
		// Rearrange expression for k to avoid loss of accuracy due to
		// subtraction.  Division by 0 not possible because uv > 0, w >= 0.
		k = uv / (sqrt(uv + SQ(w)) + w); // guaranteed positive
	}
	else { // q == 0 && r <= 0
		// y = 0 with |x| <= 1.  Handle this case directly.
		// for y small, positive root is k = abs(y)/sqrt(1-x^2)
		k = 0;
	}
	return k;
}

double SGeo::InverseStart(const SinCosPair & rScBet1, double dn1, const SinCosPair & rScBet2, double dn2, double lam12, const SinCosPair & rLam12,
	SinCosPair * pAlp1, /* Only updated if return val >= 0 */ SinCosPair * pAlp2, /* Only updated for short lines */ double* pdnm, /* Scratch area of the right size */ double Ca[])
{
	SinCosPair sc_alp1;
	SinCosPair sc_alp2;
	double dnm = 0;
	// Return a starting point for Newton's method in salp1 and calp1 (function
	// value is -1).  If Newton's method doesn't need to be used, return also
	// salp2 and calp2 and function value is sig12.
	double sig12 = -1; // Return value
	// bet12 = bet2 - bet1 in [0, pi); bet12a = bet2 + bet1 in (-pi, 0]
	const double sbet12 = rScBet2.S * rScBet1.C - rScBet2.C * rScBet1.S;
	const double cbet12 = rScBet2.C * rScBet1.C + rScBet2.S * rScBet1.S;
	const int    shortline = BIN(cbet12 >= 0 && (sbet12 < 0.5) && ((rScBet2.C * lam12) < 0.5)); // bool
	double somg12, comg12, ssig12, csig12;
#if defined(__GNUC__) && __GNUC__ == 4 && (__GNUC_MINOR__ < 6 || defined(__MINGW32__))
	// Volatile declaration needed to fix inverse cases
	// 88.202499451857 0 -88.202499451857 179.981022032992859592
	// 89.262080389218 0 -89.262080389218 179.992207982775375662
	// 89.333123580033 0 -89.333123580032997687 179.99295812360148422
	// which otherwise fail with g++ 4.4.4 x86 -O3 (Linux)
	// and g++ 4.4.0 (mingw) and g++ 4.6.1 (tdm mingw).
	double sbet12a;
	{
		volatile double xx1 = sbet2 * cbet1;
		volatile double xx2 = cbet2 * sbet1;
		sbet12a = xx1 + xx2;
	}
#else
	const double sbet12a = rScBet2.S * rScBet1.C + rScBet2.C * rScBet1.S;
#endif
	if(shortline) {
		double sbetm2 = SQ(rScBet1.S + rScBet2.S), omg12;
		// sin((bet1+bet2)/2)^2 =  (sbet1 + sbet2)^2 / ((sbet1 + sbet2)^2 + (cbet1 + cbet2)^2)
		sbetm2 /= sbetm2 + SQ(rScBet1.C + rScBet2.C);
		dnm = sqrt(1 + G.Ep2 * sbetm2);
		omg12 = lam12 / (G.F1 * dnm);
		somg12 = sin(omg12);
		comg12 = cos(omg12);
	}
	else {
		somg12 = rLam12.S;
		comg12 = rLam12.C;
	}
	sc_alp1.S = rScBet2.C * somg12;
	sc_alp1.C = (comg12 >= 0) ? (sbet12 + rScBet2.C * rScBet1.S * SQ(somg12) / (1 + comg12)) : (sbet12a - rScBet2.C * rScBet1.S * SQ(somg12) / (1 - comg12));
	ssig12 = hypot(sc_alp1.S, sc_alp1.C);
	csig12 = rScBet1.S * rScBet2.S + rScBet1.C * rScBet2.C * comg12;
	if(shortline && ssig12 < G.Etol2) {
		// really short lines
		sc_alp2.S = rScBet1.C * somg12;
		sc_alp2.C = sbet12 - rScBet1.C * rScBet2.S * (comg12 >= 0 ? SQ(somg12) / (1 + comg12) : 1 - comg12);
		sc_alp2.Norm2();
		// Set return value
		sig12 = atan2(ssig12, csig12);
	}
	else if((fabs(G.N) > 0.1) || /* No astroid calc if too eccentric */ csig12 >= 0 || ssig12 >= (6 * fabs(G.N) * SMathConst::Pi * SQ(rScBet1.C))) {
		// Nothing to do, zeroth order spherical approximation is OK
	}
	else {
		// Scale lam12 and bet2 to x, y coordinate system where antipodal point
		// is at origin and singular point is at y = 0, x = -1.
		double y, lamscale, betscale;
		// Volatile declaration needed to fix inverse case
		// 56.320923501171 0 -56.320923501171 179.664747671772880215
		// which otherwise fails with g++ 4.4.4 x86 -O3
		volatile double x;
		const double lam12x = atan2(-rLam12.S, -rLam12.C); // lam12 - pi
		if(G.F >= 0.0) { // In fact f == 0 does not get here
			// x = dlong, y = dlat
			{
				double k2 = SQ(rScBet1.S) * G.Ep2;
				double eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
				lamscale = G.F * rScBet1.C * G.A3f_(eps) * SMathConst::Pi;
			}
			betscale = lamscale * rScBet1.C;
			x = lam12x / lamscale;
			y = sbet12a / betscale;
		}
		else { // f < 0
			// x = dlat, y = dlong
			double cbet12a = rScBet2.C * rScBet1.C - rScBet2.S * rScBet1.S;
			double bet12a = atan2(sbet12a, cbet12a);
			double m12b;
			double m0;
			{
				// In the case of lon12 = 180, this repeats a calculation made in Inverse
				SinCosPair temp_sc_bet1(rScBet1.S, -rScBet1.C);
				Lengths(G.N, SMathConst::Pi + bet12a, temp_sc_bet1, dn1, rScBet2, dn2, rScBet1.C, rScBet2.C, 0, &m12b, &m0, 0, 0, Ca);
			}
			x = -1 + m12b / (rScBet1.C * rScBet2.C * m0 * SMathConst::Pi);
			betscale = (x < -0.01) ? (sbet12a / x) : (-G.F * SQ(rScBet1.C) * SMathConst::Pi);
			lamscale = betscale / rScBet1.C;
			y = lam12x / lamscale;
		}
		if(y > -_Tol1 && x > -1 - (1000.0 * _Tol2/*xthresh*/)) {
			// strip near cut
			if(G.F >= 0.0) {
				sc_alp1.S = smin(1.0, -x);
				sc_alp1.C = -sqrt(1 - SQ(sc_alp1.S));
			}
			else {
				sc_alp1.C = smax(((x > -_Tol1) ? 0.0 : -1.0), x);
				sc_alp1.S = sqrt(1.0 - SQ(sc_alp1.C));
			}
		}
		else {
			// Estimate alp1, by solving the astroid problem.
			//
			// Could estimate alpha1 = theta + pi/2, directly, i.e.,
			//   calp1 = y/k; salp1 = -x/(1+k);  for f >= 0
			//   calp1 = x/(1+k); salp1 = -y/k;  for f < 0 (need to check)
			//
			// However, it's better to estimate omg12 from astroid and use
			// spherical formula to compute alp1.  This reduces the mean number of
			// Newton iterations for astroid cases from 2.24 (min 0, max 6) to 2.12
			// (min 0 max 5).  The changes in the number of iterations are as follows:
			//
			// change percent
			//    1       5
			//    0      78
			//   -1      16
			//   -2       0.6
			//   -3       0.04
			//   -4       0.002
			//
			// The histogram of iterations is (m = number of iterations estimating
			// alp1 directly, n = number of iterations estimating via omg12, total
			// number of trials = 148605):
			//
			// iter    m      n
			//    0   148    186
			//    1 13046  13845
			//    2 93315 102225
			//    3 36189  32341
			//    4  5396      7
			//    5   455      1
			//    6    56      0
			//
			// Because omg12 is near pi, estimate work with omg12a = pi - omg12
			//
			const double k = Astroid(x, y);
			const double omg12a = lamscale * ((G.F >= 0.0) ? -x * k/(1 + k) : -y * (1 + k)/k);
			somg12 = sin(omg12a); comg12 = -cos(omg12a);
			// Update spherical estimate of alp1 using omg12 instead of lam12
			sc_alp1.S = rScBet2.C * somg12;
			sc_alp1.C = sbet12a - rScBet2.C * rScBet1.S * SQ(somg12) / (1 - comg12);
		}
	}
	// Sanity check on starting guess.  Backwards check allows NaN through.
	if(!(sc_alp1.S <= 0))
		sc_alp1.Norm2();
	else
		sc_alp1.Set(1.0, 0.0);
	*pAlp1 = sc_alp1;
	if(shortline)
		*pdnm = dnm;
	if(sig12 >= 0)
		*pAlp2 = sc_alp2;
	return sig12;
}

double SGeo::Lambda12(const SinCosPair & rScBet1, double dn1, const SinCosPair & rScBet2, double dn2,
	SinCosPair scAlp1, double slam120, double clam120, double* psalp2, double* pcalp2, double* psig12,
	double* pssig1, double* pcsig1, double* pssig2, double* pcsig2, double* peps, double* psomg12, double* pcomg12,
	int/*bool*/ diffp, double* pdlam12, /* Scratch area of the right size */ double Ca[])
{
	double salp2 = 0;
	double calp2 = 0;
	double sig12 = 0;
	SinCosPair sc_sig1;
	SinCosPair sc_sig2;
	double eps = 0;
	double somg12 = 0;
	double comg12 = 0;
	double dlam12 = 0;
	double salp0;
	double calp0;
	double somg1;
	double comg1;
	double somg2;
	double comg2;
	double lam12;
	double B312, eta, k2;
	if(rScBet1.S == 0 && scAlp1.C == 0) {
		// Break degeneracy of equatorial line.  This case has already been handled.
		scAlp1.C = -_Tiny;
	}
	// sin(alp1) * cos(bet1) = sin(alp0)
	salp0 = scAlp1.S * rScBet1.C;
	calp0 = hypot(scAlp1.C, scAlp1.S * rScBet1.S); // calp0 > 0 

	// tan(bet1) = tan(sig1) * cos(alp1)
	// tan(omg1) = sin(alp0) * tan(sig1) = tan(omg1)=tan(alp1)*sin(bet1)
	sc_sig1.S = rScBet1.S; 
	somg1 = salp0 * rScBet1.S;
	sc_sig1.C = comg1 = scAlp1.C * rScBet1.C;
	sc_sig1.Norm2();
	// norm2(&somg1, &comg1); -- don't need to normalize!

	// Enforce symmetries in the case abs(bet2) = -bet1.  Need to be careful
	// about this case, since this can yield singularities in the Newton iteration.
	// sin(alp2) * cos(bet2) = sin(alp0)
	salp2 = (rScBet2.C != rScBet1.C) ? (salp0 / rScBet2.C) : scAlp1.S;
	// calp2 = sqrt(1 - sq(salp2)) = sqrt(sq(calp0) - sq(sbet2)) / cbet2
	// and subst for calp0 and rearrange to give (choose positive sqrt
	// to give alp2 in [0, pi/2]).
	calp2 = (rScBet2.C != rScBet1.C || fabs(rScBet2.S) != -rScBet1.S) ? sqrt(SQ(scAlp1.C * rScBet1.C) + ((rScBet1.C < -rScBet1.S) ? (rScBet2.C - rScBet1.C) * (rScBet1.C + rScBet2.C) : (rScBet1.S - rScBet2.S) * (rScBet1.S + rScBet2.S))) / rScBet2.C : fabs(scAlp1.C);
	// tan(bet2) = tan(sig2) * cos(alp2)
	// tan(omg2) = sin(alp0) * tan(sig2)
	sc_sig2.S = rScBet2.S; 
	somg2 = salp0 * rScBet2.S;
	sc_sig2.C = comg2 = calp2 * rScBet2.C;
	sc_sig2.Norm2();
	// norm2(&somg2, &comg2); -- don't need to normalize!

	// sig12 = sig2 - sig1, limit to [0, pi]
	sig12 = atan2(smax(0.0, sc_sig1.C * sc_sig2.S - sc_sig1.S * sc_sig2.C), sc_sig1.C * sc_sig2.C + sc_sig1.S * sc_sig2.S);

	// omg12 = omg2 - omg1, limit to [0, pi]
	somg12 = smax(0.0, comg1 * somg2 - somg1 * comg2);
	comg12 = comg1 * comg2 + somg1 * somg2;
	// eta = omg12 - lam120
	eta = atan2(somg12 * clam120 - comg12 * slam120,
	comg12 * clam120 + somg12 * slam120);
	k2 = SQ(calp0) * G.Ep2;
	eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
	G.C3f_(eps, Ca);
	B312 = (SinCosSeries(TRUE, sc_sig2, Ca, nC3-1) - SinCosSeries(TRUE, sc_sig1, Ca, nC3-1));
	lam12 = eta - G.F * G.A3f_(eps) * salp0 * (sig12 + B312);
	if(diffp) {
		if(calp2 == 0)
			dlam12 = -2 * G.F1 * dn1 / rScBet1.S;
		else {
			Lengths(eps, sig12, sc_sig1, dn1, sc_sig2, dn2, rScBet1.C, rScBet2.C, 0, &dlam12, 0, 0, 0, Ca);
			dlam12 *= G.F1 / (calp2 * rScBet2.C);
		}
	}
	*psalp2 = salp2;
	*pcalp2 = calp2;
	*psig12 = sig12;
	*pssig1 = sc_sig1.S;
	*pcsig1 = sc_sig1.C;
	*pssig2 = sc_sig2.S;
	*pcsig2 = sc_sig2.C;
	*peps = eps;
	*psomg12 = somg12;
	*pcomg12 = comg12;
	if(diffp)
		*pdlam12 = dlam12;
	return lam12;
}

double SGeo::Inverse_Int(SGeoPosLL & rP1, SGeoPosLL & rP2, double * ps12, 
	SinCosPair * pAlp1, SinCosPair * pAlp2, double* pm12, double* pM12, double* pM21, double* pS12)
{
	double s12 = 0;
	double m12 = 0;
	double M12 = 0;
	double M21 = 0;
	double S12 = 0;
	double lon12s;
	int    latsign;
	int    swapp;
	SinCosPair sc_bet1;
	SinCosPair sc_bet2;
	double s12x = 0;
	double m12x = 0;
	double dn1;
	double dn2;
	double lam12;
	SinCosPair sc_lam12;
	double a12 = 0;
	double sig12;
	SinCosPair sc_alp1;
	SinCosPair sc_alp2;
	double Ca[nC];
	int meridian; // bool
	// somg12 > 1 marks that it needs to be calculated
	double omg12 = 0.0;
	SinCosPair sc_omg12(2.0, 0.0);
	uint   outmask = (ps12 ? GEOD_DISTANCE : 0U) | (pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) | (pS12 ? GEOD_AREA : 0U);
	outmask &= OUT_ALL;
	// Compute longitude difference (AngDiff does this carefully).  Result is
	// in [-180, 180] but -180 is only for west-going geodesics.  180 is for
	// east-going and meridional geodesics.
	double lon12 = AngDiff(rP1.Lon, rP2.Lon, &lon12s);
	// Make longitude difference positive.
	int    lonsign = (lon12 >= 0) ? 1 : -1;
	// If very close to being on the same half-meridian, then make it so.
	lon12 = lonsign * AngRound(lon12);
	lon12s = AngRound((180 - lon12) - lonsign * lon12s);
	lam12 = lon12 * SMathConst::PiDiv180;
	if(lon12 > 90) {
		sc_lam12.Set_SinCosDX(lon12s);
		sc_lam12.C = -sc_lam12.C;
	}
	else
		sc_lam12.Set_SinCosDX(lon12);
	// If really close to the equator, treat as on equator.
	rP1.Lat = AngRound(LatFix(rP1.Lat));
	rP2.Lat = AngRound(LatFix(rP2.Lat));
	//
	// Swap points so that point with higher (abs) latitude is point 1
	// If one latitude is a nan, then it becomes lat1.
	//
	swapp = (fabs(rP1.Lat) < fabs(rP2.Lat)) ? -1 : 1;
	if(swapp < 0) {
		lonsign *= -1;
		Exchange(&rP1.Lat, &rP2.Lat);
	}
	// Make lat1 <= 0
	latsign = (rP1.Lat < 0) ? 1 : -1;
	rP1.Lat *= latsign;
	rP2.Lat *= latsign;
	// Now we have
	//      0 <= lon12 <= 180
	//      -90 <= lat1 <= 0
	//      lat1 <= lat2 <= -lat1
	//
	// longsign, swapp, latsign register the transformation to bring the
	// coordinates to this canonical form.  In all cases, 1 means no change was
	// made.  We make these transformations so that there are few cases to
	// check, e.g., on verifying quadrants in atan2.  In addition, this
	// enforces some symmetries in the results returned.
	sc_bet1.Set_SinCosDX(rP1.Lat);
	sc_bet1.S *= G.F1;
	// Ensure cbet1 = +epsilon at poles
	sc_bet1.Norm2();
	sc_bet1.C = smax(_Tiny, sc_bet1.C);
	sc_bet2.Set_SinCosDX(rP2.Lat);
	sc_bet2.S *= G.F1;
	// Ensure cbet2 = +epsilon at poles
	sc_bet2.Norm2();
	sc_bet2.C = smax(_Tiny, sc_bet2.C);
	//
	// If cbet1 < -sbet1, then cbet2 - cbet1 is a sensitive measure of the
	// |bet1| - |bet2|.  Alternatively (cbet1 >= -sbet1), abs(sbet2) + sbet1 is
	// a better measure.  This logic is used in assigning calp2 in Lambda12.
	// Sometimes these quantities vanish and in that case we force bet2 = +/-
	// bet1 exactly.  An example where is is necessary is the inverse problem
	// 48.522876735459 0 -48.52287673545898293 179.599720456223079643
	// which failed with Visual Studio 10 (Release and Debug)
	//
	if(sc_bet1.C < -sc_bet1.S) {
		if(sc_bet2.C == sc_bet1.C)
			sc_bet2.S = (sc_bet2.S < 0) ? sc_bet1.S : -sc_bet1.S;
	}
	else {
		if(fabs(sc_bet2.S) == -sc_bet1.S)
			sc_bet2.C = sc_bet1.C;
	}
	dn1 = sqrt(1.0 + G.Ep2 * SQ(sc_bet1.S));
	dn2 = sqrt(1.0 + G.Ep2 * SQ(sc_bet2.S));
	meridian = (rP1.Lat == -90 || sc_lam12.S == 0);
	if(meridian) {
		// Endpoints are on a single full meridian, so the geodesic might lie on a meridian.
		sc_alp1 = sc_lam12; // Head to the target longitude
		//- sc_alp2.C = 1.0;
		//- sc_alp2.S = 0.0; // At the target we're heading north
		sc_alp2.Set(0.0, 1.0); // At the target we're heading north
		// tan(bet) = tan(sig) * cos(alp)
		SinCosPair sc_sig1(sc_bet1.S, sc_alp1.C * sc_bet1.C);
		SinCosPair sc_sig2(sc_bet2.S, sc_alp2.C * sc_bet2.C);
		// sig12 = sig2 - sig1
		sig12 = atan2(smax(0.0, sc_sig1.C * sc_sig2.S - sc_sig1.S * sc_sig2.C), sc_sig1.C * sc_sig2.C + sc_sig1.S * sc_sig2.S);
		Lengths(G.N, sig12, sc_sig1, dn1, sc_sig2, dn2, sc_bet1.C, sc_bet2.C, &s12x, &m12x, 0, outmask & GEOD_GEODESICSCALE ? &M12 : 0, outmask & GEOD_GEODESICSCALE ? &M21 : 0, Ca);
		// Add the check for sig12 since zero length geodesics might yield m12 < 0.  Test case was
		//
		//    echo 20.001 0 20.001 0 | GeodSolve -i
		//
		// In fact, we will have sig12 > pi/2 for meridional geodesic which is not a shortest path.
		//
		if(sig12 < 1 || m12x >= 0) {
			// Need at least 2, to handle 90 0 90 180
			if(sig12 < (3 * _Tiny))
				sig12 = m12x = s12x = 0;
			m12x *= G.B;
			s12x *= G.B;
			a12 = sig12 / SMathConst::PiDiv180;
		}
		else // m12 < 0, i.e., prolate and too close to anti-podal
			meridian = FALSE;
	}
	if(!meridian && sc_bet1.S == 0 && /* and sbet2 == 0 */ /* Mimic the way Lambda12 works with calp1 = 0 */ (G.F <= 0.0 || lon12s >= (G.F * 180.0))) {
		// Geodesic runs along equator
		//- sc_alp1.C = sc_alp2.C = 0.0;
		//- sc_alp1.S = sc_alp2.S = 1.0;
		sc_alp1.Set(1.0, 0.0);
		sc_alp2.Set(1.0, 0.0);
		s12x = G.A * lam12;
		sig12 = omg12 = lam12 / G.F1;
		m12x = G.B * sin(sig12);
		if(outmask & GEOD_GEODESICSCALE)
			M12 = M21 = cos(sig12);
		a12 = lon12 / G.F1;
	}
	else if(!meridian) {
		//
		// Now point1 and point2 belong within a hemisphere bounded by a
		// meridian and geodesic is neither meridional or equatorial.
		//
		// Figure a starting point for Newton's method
		//
		double dnm = 0.0;
		sig12 = InverseStart(sc_bet1, dn1, sc_bet2, dn2, lam12, sc_lam12, &sc_alp1, &sc_alp2, &dnm, Ca);
		if(sig12 >= 0) {
			// Short lines (InverseStart sets salp2, calp2, dnm)
			s12x = sig12 * G.B * dnm;
			m12x = SQ(dnm) * G.B * sin(sig12 / dnm);
			if(outmask & GEOD_GEODESICSCALE)
				M12 = M21 = cos(sig12 / dnm);
			a12 = sig12 / SMathConst::PiDiv180;
			omg12 = lam12 / (G.F1 * dnm);
		}
		else {
			//
			// Newton's method.  This is a straightforward solution of f(alp1) = lambda12(alp1) - lam12 = 0 with one wrinkle.
			// f(alp) has exactly one root in the interval (0, pi) and its derivative is positive at the
			// root.  Thus f(alp) is positive for alp > alp1 and negative for alp < alp1.
			// During the course of the iteration, a range (alp1a, alp1b) is
			// maintained which brackets the root and with each evaluation of f(alp) the range is shrunk,
			// if possible.  Newton's method is restarted whenever the derivative of f is negative (because the new
			// value of alp1 is then further from the solution) or if the new
			// estimate of alp1 lies outside (0,pi); in this case, the new starting guess is taken to be (alp1a + alp1b) / 2.
			//
			SinCosPair sc_sig1;
			SinCosPair sc_sig2;
			double eps = 0;
			uint   numit = 0;
			// Bracketing range
			double salp1a = _Tiny;
			double calp1a = 1.0;
			double salp1b = _Tiny;
			double calp1b = -1.0;
			int    tripn; // bool
			int    tripb; // bool
			for(tripn = FALSE, tripb = FALSE; numit < _MaxIt2; ++numit) {
				// the WGS84 test set: mean = 1.47, sd = 1.25, max = 16
				// WGS84 and random input: mean = 2.85, sd = 0.60
				double dv = 0,
				v = Lambda12(sc_bet1, dn1, sc_bet2, dn2, sc_alp1, sc_lam12.S, sc_lam12.C,
					&sc_alp2.S, &sc_alp2.C, &sig12, &sc_sig1.S, &sc_sig1.C, &sc_sig2.S, &sc_sig2.C, &eps, &sc_omg12.S, &sc_omg12.C, numit < _MaxIt1, &dv, Ca);
				// 2 * tol0 is approximately 1 ulp for a number in [0, pi].
				// Reversed test to allow escape with NaNs
				if(tripb || !(fabs(v) >= (tripn ? 8 : 1) * _Tol0))
					break;
				// Update bracketing values
				if(v > 0 && (numit > _MaxIt1 || sc_alp1.C/sc_alp1.S > calp1b/salp1b)) {
					salp1b = sc_alp1.S;
					calp1b = sc_alp1.C;
				}
				else if(v < 0 && (numit > _MaxIt1 || sc_alp1.C/sc_alp1.S < calp1a/salp1a)) {
					salp1a = sc_alp1.S;
					calp1a = sc_alp1.C;
				}
				if(numit < _MaxIt1 && dv > 0) {
					const double dalp1 = -v/dv;
					const SinCosPair sc_dalp1(dalp1);
					const double nsalp1 = sc_alp1.S * sc_dalp1.C + sc_alp1.C * sc_dalp1.S;
					if(nsalp1 > 0 && fabs(dalp1) < SMathConst::Pi/*pi*/) {
						//- sc_alp1.C = sc_alp1.C * sc_dalp1.C - sc_alp1.S * sc_dalp1.S;
						//- sc_alp1.S = nsalp1;
						sc_alp1.Set(nsalp1, sc_alp1.C * sc_dalp1.C - sc_alp1.S * sc_dalp1.S);
						sc_alp1.Norm2();
						// In some regimes we don't get quadratic convergence because
						// slope -> 0.  So use convergence conditions based on epsilon
						// instead of sqrt(epsilon).
						tripn = fabs(v) <= 16 * _Tol0;
						continue;
					}
				}
				//
				// Either dv was not postive or updated value was outside legal
				// range.  Use the midpoint of the bracket as the next estimate.
				// This mechanism is not needed for the WGS84 ellipsoid, but it does
				// catch problems with more eccentric ellipsoids.  Its efficacy is
				// such for the WGS84 test set with the starting guess set to alp1 = 90deg:
				// the WGS84 test set: mean = 5.21, sd = 3.93, max = 24 WGS84 and random input: mean = 4.74, sd = 0.99
				//
				//- sc_alp1.S = (salp1a + salp1b)/2.0;
				//- sc_alp1.C = (calp1a + calp1b)/2.0;
				sc_alp1.Set((salp1a + salp1b)/2.0, (calp1a + calp1b)/2.0);
				sc_alp1.Norm2();
				tripn = FALSE;
				tripb = (fabs(salp1a - sc_alp1.S) + (calp1a - sc_alp1.C) < _Tolb || fabs(sc_alp1.S - salp1b) + (sc_alp1.C - calp1b) < _Tolb);
			}
			Lengths(eps, sig12, sc_sig1, dn1, sc_sig2, dn2, sc_bet1.C, sc_bet2.C, &s12x, &m12x, 0,
				outmask & GEOD_GEODESICSCALE ? &M12 : 0, outmask & GEOD_GEODESICSCALE ? &M21 : 0, Ca);
			m12x *= G.B;
			s12x *= G.B;
			a12 = sig12 / SMathConst::PiDiv180;
		}
	}
	if(outmask & GEOD_DISTANCE)
		s12 = 0.0 + s12x; // Convert -0 to 0
	if(outmask & GEOD_REDUCEDLENGTH)
		m12 = 0.0 + m12x; // Convert -0 to 0
	if(outmask & GEOD_AREA) {
		// From Lambda12: sin(alp1) * cos(bet1) = sin(alp0)
		double salp0 = sc_alp1.S * sc_bet1.C;
		double calp0 = hypot(sc_alp1.C, sc_alp1.S * sc_bet1.S); // calp0 > 0
		double alp12;
		if(calp0 != 0 && salp0 != 0) {
			// From Lambda12: tan(bet) = tan(sig) * cos(alp)
			SinCosPair sc_sig1(sc_bet1.S, sc_alp1.C * sc_bet1.C);
			SinCosPair sc_sig2(sc_bet2.S, sc_alp2.C * sc_bet2.C);
			double k2 = SQ(calp0) * G.Ep2;
			double eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
			// Multiplier = a^2 * e^2 * cos(alpha0) * sin(alpha0).
			double A4 = SQ(G.A) * calp0 * salp0 * G.E2;
			double B41;
			double B42;
			sc_sig1.Norm2();
			sc_sig2.Norm2();
			G.C4f_(eps, Ca);
			B41 = SinCosSeries(FALSE, sc_sig1, Ca, nC4);
			B42 = SinCosSeries(FALSE, sc_sig2, Ca, nC4);
			S12 = A4 * (B42 - B41);
		}
		else // Avoid problems with indeterminate sig1, sig2 on equator
			S12 = 0;
		if(!meridian) {
			if(sc_omg12.S > 1.0)
				sc_omg12.SetRad(omg12);
			else
				sc_omg12.Norm2();
		}
		if(!meridian && /* omg12 < 3/4 * pi */ (sc_omg12.C > -0.7071) && /* Long difference not too big */ (sc_bet2.S - sc_bet1.S) < 1.75) { // Lat difference not too big
			// Use tan(Gamma/2) = tan(omg12/2) * (tan(bet1/2)+tan(bet2/2))/(1+tan(bet1/2)*tan(bet2/2))
			// with tan(x/2) = sin(x)/(1+cos(x))
			double domg12 = 1.0 + sc_omg12.C;
			double dbet1 = 1.0 + sc_bet1.C;
			double dbet2 = 1.0 + sc_bet2.C;
			alp12 = 2 * atan2(sc_omg12.S * (sc_bet1.S * dbet2 + sc_bet2.S * dbet1), domg12 * (sc_bet1.S * sc_bet2.S + dbet1 * dbet2));
		}
		else {
			// alp12 = alp2 - alp1, used in atan2 so no need to normalize
			double salp12 = sc_alp2.S * sc_alp1.C - sc_alp2.C * sc_alp1.S;
			double calp12 = sc_alp2.C * sc_alp1.C + sc_alp2.S * sc_alp1.S;
			// The right thing appears to happen if alp1 = +/-180 and alp2 = 0, viz
			// salp12 = -0 and alp12 = -180.  However this depends on the sign
			// being attached to 0 correctly.  The following ensures the correct behavior.
			if(salp12 == 0 && calp12 < 0) {
				salp12 = _Tiny * sc_alp1.C;
				calp12 = -1;
			}
			alp12 = atan2(salp12, calp12);
		}
		S12 += G.C2 * alp12;
		S12 *= swapp * lonsign * latsign;
		// Convert -0 to 0
		S12 += 0;
	}
	// Convert calp, salp to azimuth accounting for lonsign, swapp, latsign.
	if(swapp < 0) {
		Exchange(&sc_alp1.S, &sc_alp2.S);
		Exchange(&sc_alp1.C, &sc_alp2.C);
		if(outmask & GEOD_GEODESICSCALE)
			Exchange(&M12, &M21);
	}
	sc_alp1.S *= swapp * lonsign;
	sc_alp1.C *= swapp * latsign;
	sc_alp2.S *= swapp * lonsign;
	sc_alp2.C *= swapp * latsign;
	ASSIGN_PTR(pAlp1, sc_alp1);
	ASSIGN_PTR(pAlp2, sc_alp2);
	if(outmask & GEOD_DISTANCE)
		*ps12 = s12;
	if(outmask & GEOD_REDUCEDLENGTH)
		*pm12 = m12;
	if(outmask & GEOD_GEODESICSCALE) {
		ASSIGN_PTR(pM12, M12);
		ASSIGN_PTR(pM21, M21);
	}
	if(outmask & GEOD_AREA)
		*pS12 = S12;
	// Returned value in [0, 180]
	return a12;
}

double SGeo::Inverse(const SGeoPosLL & rP1, const SGeoPosLL & rP2, double * ps12, double * pazi1, double * pazi2, double * pm12, double * pM12, double * pM21, double * pS12)
{
	/*real geod_geninverse(const struct geod_geodesic* g, real lat1, real lon1, real lat2, real lon2,
		real* ps12, real* pazi1, real* pazi2, real* pm12, real* pM12, real* pM21, real* pS12) {*/
	SinCosPair sc_alp1;
	SinCosPair sc_alp2;
	SGeoPosLL p1 = rP1;
	SGeoPosLL p2 = rP2;
	double a12 = Inverse_Int(p1, p2, ps12, &sc_alp1, &sc_alp2, pm12, pM12, pM21, pS12);
	ASSIGN_PTR(pazi1, atan2dx(sc_alp1.S, sc_alp1.C));
	ASSIGN_PTR(pazi2, atan2dx(sc_alp2.S, sc_alp2.C));
	return a12;
}
//
// @TEST {
//`
#if SLTEST_RUNNING // {
/*

    latitude at point 1, lat1 (degrees, exact)
    longitude at point 1, lon1 (degrees, always 0)
    azimuth at point 1, azi1 (clockwise from north in degrees, exact)
    latitude at point 2, lat2 (degrees, accurate to 10−18 deg)
    longitude at point 2, lon2 (degrees, accurate to 10−18 deg)
    azimuth at point 2, azi2 (degrees, accurate to 10−18 deg)
    geodesic distance from point 1 to point 2, s12 (meters, exact)
    arc distance on the auxiliary sphere, a12 (degrees, accurate to 10−18 deg)
    reduced length of the geodesic, m12 (meters, accurate to 0.1 pm)
    the area under the geodesic, S12 (m2, accurate to 1 mm2)

*/

struct GeodTestRecord {
	GeodTestRecord()
	{
		Azi1 = 0.0;
		Azi2 = 0.0;
		GeodDistance = 0.0;
		ArcDistance = 0.0;
		M12 = 0.0;
		S12 = 0.0;
	}
	int    FASTCALL IsEqual(const GeodTestRecord & rS) const
	{
		int    ok = 1;
		THROW(P1 == rS.P1);
		THROW(P2 == rS.P2);
		THROW(feqeps(Azi1, rS.Azi1, 1E-7));
		THROW(feqeps(Azi2, rS.Azi2, 1E-7));
		THROW(feqeps(GeodDistance, rS.GeodDistance, 1E-4));
		THROW(feqeps(ArcDistance, rS.ArcDistance, 1E-7));
		THROW(feqeps(M12, rS.M12, 1E-4));
		THROW(feqeps(S12, rS.S12, 6));
		CATCHZOK
		return ok;
	}
    SGeoPosLL P1;        // Point 1 (degrees, exact). Lon always 0.
    double Azi1;         // Azimuth at point 1 (clockwise from north in degrees, exact)
    SGeoPosLL P2;        // Point 2 (degrees, exact).
    double Azi2;         // Azimuth at point 2 (degrees, accurate to 10−18 deg)
    double GeodDistance; // Geodesic distance [P1, P2] (meters, exact)
    double ArcDistance;  // Arc distance [P1, P2] (degrees, accurate to 10−18 deg)
	double M12;          // reduced length of the geodesic, m12 (meters, accurate to 0.1 pm)
	double S12;          // the area under the geodesic, S12 (m2, accurate to 1 mm2)
};

SLTEST_R(SGeo)
{
	SString in_file_name(MakeInputFilePath("GeodTest.dat"));
	SGeo   sg;
	uint   line_no = 0;
	SString line_buf;
	SString temp_buf;
	{
		StringSet ss;
		SFile f_in(in_file_name, SFile::mRead);
		THROW(SLTEST_CHECK_NZ(f_in.IsValid()));
		while(f_in.ReadLine(line_buf)) {
			line_no++;
			volatile int  _test_result = 0; // @debug
			GeodTestRecord rec;
			line_buf.Chomp();
			ss.clear();
            line_buf.Tokenize(" \t", ss);
            uint   fld_count = 0;
            for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				fld_count++;
				switch(fld_count) {
					case 1: rec.P1.Lat = temp_buf.ToReal(); break;
					case 2: rec.P1.Lon = temp_buf.ToReal(); break;
					case 3: rec.Azi1 = temp_buf.ToReal(); break;
					case 4: rec.P2.Lat = temp_buf.ToReal(); break;
					case 5: rec.P2.Lon = temp_buf.ToReal(); break;
					case 6: rec.Azi2 = temp_buf.ToReal(); break;
					case 7: rec.GeodDistance = temp_buf.ToReal(); break;
					case 8: rec.ArcDistance = temp_buf.ToReal(); break;
					case 9: rec.M12 = temp_buf.ToReal(); break;
					case 10: rec.S12 = temp_buf.ToReal(); break;
				}
            }
			SLTEST_CHECK_EQ(fld_count, 10);
            if(fld_count == 10) {
				{
					GeodTestRecord test_rec;
					double _m12 = 0.0;
					double _m21 = 0.0;
					test_rec.ArcDistance = sg.Direct(rec.P1, rec.Azi1, 0, rec.GeodDistance, &test_rec.P2, &test_rec.Azi2, &test_rec.GeodDistance, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.Azi1 = rec.Azi1;
					//SLTEST_CHECK_NZ(test_rec.IsEqual(rec));

					//SLTEST_CHECK_NZ(test_rec.P1 == rec.P1);
					SLTEST_CHECK_NZ(test_rec.P2 == rec.P2);
					//SLTEST_CHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLTEST_CHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLTEST_CHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLTEST_CHECK_EQ_TOL(test_rec.S12, rec.S12, 10);

					test_rec.ArcDistance = sg.Direct(rec.P1, rec.Azi1, sg.GEOD_ARCMODE, rec.ArcDistance, &test_rec.P2, &test_rec.Azi2, &test_rec.GeodDistance, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.Azi1 = rec.Azi1;
					//SLTEST_CHECK_NZ(test_rec.IsEqual(rec));

					//SLTEST_CHECK_NZ(test_rec.P1 == rec.P1);
					SLTEST_CHECK_NZ(test_rec.P2 == rec.P2);
					//SLTEST_CHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLTEST_CHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLTEST_CHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLTEST_CHECK_EQ_TOL(test_rec.S12, rec.S12, 10);
				}
				{
					GeodTestRecord test_rec;
					double _m12 = 0.0;
					double _m21 = 0.0;
					test_rec.ArcDistance = sg.Inverse(rec.P1, rec.P2, &test_rec.GeodDistance, &test_rec.Azi1, &test_rec.Azi2, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.P2 = rec.P2;
					//SLTEST_CHECK_NZ(test_rec.IsEqual(rec));

					//SLTEST_CHECK_NZ(test_rec.P1 == rec.P1);
					//SLTEST_CHECK_NZ(test_rec.P2 == rec.P2);
					SLTEST_CHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLTEST_CHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLTEST_CHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLTEST_CHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLTEST_CHECK_EQ_TOL(test_rec.S12, rec.S12, 10);
				}
            }
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
//
// } @TEST
//
