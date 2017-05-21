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
//static
const double SGeo::A_WGS84 = 6378137.0; // Радиус Земли в метрах (согласно WGS84)
//static
const double SGeo::Flattening_WGS84 = 1.0 / 298.257223563; // Фактор приплюснотости Земли: ƒ = (a − b)/a (согласно WGS84).

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
	int q = 0;
	double ang;
	if(fabs(y) > fabs(x)) {
		Exchange(&x, &y);
		q = 2;
	}
	if(x < 0) {
		x = -x;
		++q;
	}
	// here x >= 0 and x >= abs(y), so angle is in [-pi/4, pi/4]
	ang = atan2(y, x) / SMathConst::PiDiv180;
	switch(q) {
		// Note that atan2d(-0.0, 1.0) will return -0.  However, we expect that
		// atan2d will not be called with y = -0.  If need be, include
		//
		// case 0: ang = 0 + ang; break;
		//
		case 1: ang = (y > 0 ? 180 : -180) - ang; break;
		case 2: ang =  90 - ang; break;
		case 3: ang = -90 + ang; break;
	}
	return ang;
}

static double copysignx(double x, double y)
{
	return fabs(x) * (y < 0 || (y == 0 && 1/y < 0) ? -1 : 1);
}

/*static double hypotx(double x, double y)
{
	return sqrt(x * x + y * y);
}*/

static void sincosdx(double x, double * sinx, double * cosx)
{
	// In order to minimize round-off errors, this function exactly reduces
	// the argument to the range [-45, 45] before converting it to radians.
	double r = fmod(x, 360.0);
	int    q = (int)(floor(r / 90 + 0.5));
	r -= 90 * q;
	// now abs(r) <= 45
	r *= SMathConst::PiDiv180;
	// Possibly could call the gnu extension sincos
	double s = sin(r);
	double c = cos(r);
	switch((unsigned)q & 3U) {
		case 0U: *sinx =     s; *cosx =     c; break;
		case 1U: *sinx =     c; *cosx = 0 - s; break;
		case 2U: *sinx = 0 - s; *cosx = 0 - c; break;
		default: *sinx = 0 - c; *cosx =     s; break; // case 3U
	}
}

static double SinCosSeries(int sinp, double sinx, double cosx, const double c[], int n)
{
	// Evaluate
	// y = sinp ? sum(c[i] * sin( 2*i    * x), i, 1, n) : sum(c[i] * cos((2*i+1) * x), i, 0, n-1)
	// using Clenshaw summation.  N.B. c[0] is unused for sin series
	// Approx operation count = (n + 5) mult and (2 * n + 2) add
	double ar, y0, y1;
	c += (n + sinp);              /* Point to one beyond last element */
	ar = 2 * (cosx - sinx) * (cosx + sinx); /* 2 * cos(2 * x) */
	y0 = n & 1 ? *--c : 0; y1 = 0;          /* accumulators for sum */
	// Now n is even
	n /= 2;
	while(n--) {
		// Unroll loop x 2, so accumulators return to their original role
		y1 = ar * y0 - y1 + *--c;
		y0 = ar * y1 - y0 + *--c;
	}
	return sinp ? 2 * sinx * cosx * y0 /* sin(2 * x) * y0 */ : cosx * (y0 - y1); /* cos(x) * (y0 - y1) */
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
	if(t)
		*t = -(up + vpp);
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

static void norm2(double * sinx, double * cosx)
{
	double r = hypot(*sinx, *cosx);
	*sinx /= r;
	*cosx /= r;
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

SGeo::SGeo() :
	_RealMin(MINDOUBLE),
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

void SGeo::LineInit_Int(SGeo::GeodesicLine * pLine, const SGeoPosLL & rP1, double azi1, double salp1, double calp1, uint caps)
{
	double cbet1;
	double sbet1;
	double eps;
	pLine->a = G.A;
	pLine->f = G.F;
	pLine->b = G.B;
	pLine->c2 = G.C2;
	pLine->f1 = G.F1;
	// If caps is 0 assume the standard direct calculation
	pLine->caps = (caps ? caps : GEOD_DISTANCE_IN | GEOD_LONGITUDE) | /* always allow latitude and azimuth and unrolling of longitude */ GEOD_LATITUDE | GEOD_AZIMUTH | GEOD_LONG_UNROLL;

	pLine->P1.Lat = LatFix(rP1.Lat);
	pLine->P1.Lon = rP1.Lon;
	pLine->azi1 = azi1;
	pLine->salp1 = salp1;
	pLine->calp1 = calp1;

	sincosdx(AngRound(pLine->P1.Lat), &sbet1, &cbet1); sbet1 *= pLine->f1;
	// Ensure cbet1 = +epsilon at poles
	norm2(&sbet1, &cbet1); 
	cbet1 = smax(_Tiny, cbet1);
	pLine->dn1 = sqrt(1 + G.Ep2 * SQ(sbet1));
	// Evaluate alp0 from sin(alp1) * cos(bet1) = sin(alp0),
	pLine->salp0 = pLine->salp1 * cbet1; /* alp0 in [0, pi/2 - |bet1|] */
	// Alt: calp0 = hypot(sbet1, calp1 * cbet1).  The following
	// is slightly better (consider the case salp1 = 0).
	pLine->calp0 = hypot(pLine->calp1, pLine->salp1 * sbet1);
	// Evaluate sig with tan(bet1) = tan(sig1) * cos(alp1).
	// sig = 0 is nearest northward crossing of equator.
	// With bet1 = 0, alp1 = pi/2, we have sig1 = 0 (equatorial line).
	// With bet1 =  pi/2, alp1 = -pi, sig1 =  pi/2
	// With bet1 = -pi/2, alp1 =  0 , sig1 = -pi/2
	// Evaluate omg1 with tan(omg1) = sin(alp0) * tan(sig1).
	// With alp0 in (0, pi/2], quadrants for sig and omg coincide.
	// No atan2(0,0) ambiguity at poles since cbet1 = +epsilon.
	// With alp0 = 0, omg1 = 0 for alp1 = 0, omg1 = pi for alp1 = pi.
	pLine->ssig1 = sbet1; pLine->somg1 = pLine->salp0 * sbet1;
	pLine->csig1 = pLine->comg1 = sbet1 != 0 || pLine->calp1 != 0 ? cbet1 * pLine->calp1 : 1;
	norm2(&pLine->ssig1, &pLine->csig1); /* sig1 in (-pi, pi] */
	// norm2(somg1, comg1); -- don't need to normalize!
	pLine->k2 = SQ(pLine->calp0) * G.Ep2;
	eps = pLine->k2 / (2 * (1 + sqrt(1 + pLine->k2)) + pLine->k2);
	if(pLine->caps & CAP_C1) {
		double s, c;
		pLine->A1m1 = A1m1f(eps);
		C1f(eps, pLine->C1a);
		pLine->B11 = SinCosSeries(TRUE, pLine->ssig1, pLine->csig1, pLine->C1a, nC1);
		s = sin(pLine->B11); c = cos(pLine->B11);
		// tau1 = sig1 + B11
		pLine->stau1 = pLine->ssig1 * c + pLine->csig1 * s;
		pLine->ctau1 = pLine->csig1 * c - pLine->ssig1 * s;
		// Not necessary because C1pa reverts C1a B11 = -SinCosSeries(TRUE, stau1, ctau1, C1pa, nC1p);
	}
	if(pLine->caps & CAP_C1p)
		C1pf(eps, pLine->C1pa);
	if(pLine->caps & CAP_C2) {
		pLine->A2m1 = A2m1f(eps);
		C2f(eps, pLine->C2a);
		pLine->B21 = SinCosSeries(TRUE, pLine->ssig1, pLine->csig1, pLine->C2a, nC2);
	}
	if(pLine->caps & CAP_C3) {
		G.C3f_(eps, pLine->C3a);
		pLine->A3c = -pLine->f * pLine->salp0 * G.A3f_(eps);
		pLine->B31 = SinCosSeries(TRUE, pLine->ssig1, pLine->csig1, pLine->C3a, nC3-1);
	}
	if(pLine->caps & CAP_C4) {
		G.C4f_(eps, pLine->C4a);
		// Multiplier = a^2 * e^2 * cos(alpha0) * sin(alpha0)
		pLine->A4 = SQ(pLine->a) * pLine->calp0 * pLine->salp0 * G.E2;
		pLine->B41 = SinCosSeries(FALSE, pLine->ssig1, pLine->csig1, pLine->C4a, nC4);
	}
	pLine->a13 = pLine->s13 = fgetnan();
}

void SGeo::LineInit(GeodesicLine * pLine, /*const SGeo::Geodesic & rG,*/const SGeoPosLL & rP1, double azi1, uint caps)
{
	azi1 = AngNormalize(azi1);
	double salp1, calp1;
	// Guard against underflow in salp0
	sincosdx(AngRound(azi1), &salp1, &calp1);
	LineInit_Int(pLine, /*rG,*/rP1, azi1, salp1, calp1, caps);
}

double SGeo::GenPosition(const SGeo::GeodesicLine * pLine, uint flags, double s12_a12,
	SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12)
{
	double lat2 = 0, lon2 = 0, azi2 = 0, s12 = 0;
	double m12 = 0, M12 = 0, M21 = 0, S12 = 0;
	// Avoid warning about uninitialized B12
	double sig12;
	double ssig12;
	double csig12;
	double B12 = 0.0;
	double AB1 = 0.0;
	double omg12;
	double lam12;
	double lon12;
	double ssig2;
	double csig2;
	double sbet2;
	double cbet2;
	double somg2;
	double comg2;
	double salp2;
	double calp2;
	double dn2;
	uint   outmask =
		//(plat2 ? GEOD_LATITUDE : 0U) | (plon2 ? GEOD_LONGITUDE : 0U) |
		(pP2 ? (GEOD_LATITUDE|GEOD_LONGITUDE) : 0U) |
		(pazi2 ? GEOD_AZIMUTH : 0U) | (ps12 ? GEOD_DISTANCE : 0U) | (pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) |
		(pS12 ? GEOD_AREA : 0U);
	outmask &= pLine->caps & OUT_ALL;
	if(!(TRUE /*Init()*/ && (flags & GEOD_ARCMODE || (pLine->caps & (GEOD_DISTANCE_IN & OUT_ALL)))))
		return fgetnan(); // Uninitialized or impossible distance calculation requested
	if(flags & GEOD_ARCMODE) {
		// Interpret s12_a12 as spherical arc length
		sig12 = s12_a12 * SMathConst::PiDiv180;
		sincosdx(s12_a12, &ssig12, &csig12);
	}
	else {
		// Interpret s12_a12 as distance
		double tau12 = s12_a12 / (pLine->b * (1 + pLine->A1m1));
		double s = sin(tau12);
		double c = cos(tau12);
		// tau2 = tau1 + tau12
		B12 = - SinCosSeries(TRUE, pLine->stau1 * c + pLine->ctau1 * s, pLine->ctau1 * c - pLine->stau1 * s, pLine->C1pa, nC1p);
		sig12 = tau12 - (B12 - pLine->B11);
		ssig12 = sin(sig12); csig12 = cos(sig12);
		if(fabs(pLine->f) > 0.01) {
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
			ssig2 = pLine->ssig1 * csig12 + pLine->csig1 * ssig12;
			csig2 = pLine->csig1 * csig12 - pLine->ssig1 * ssig12;
			B12 = SinCosSeries(TRUE, ssig2, csig2, pLine->C1a, nC1);
			serr = (1 + pLine->A1m1) * (sig12 + (B12 - pLine->B11)) - s12_a12 / pLine->b;
			sig12 = sig12 - serr / sqrt(1 + pLine->k2 * SQ(ssig2));
			ssig12 = sin(sig12); csig12 = cos(sig12);
			// Update B12 below
		}
	}
	// sig2 = sig1 + sig12
	ssig2 = pLine->ssig1 * csig12 + pLine->csig1 * ssig12;
	csig2 = pLine->csig1 * csig12 - pLine->ssig1 * ssig12;
	dn2 = sqrt(1 + pLine->k2 * SQ(ssig2));
	if(outmask & (GEOD_DISTANCE | GEOD_REDUCEDLENGTH | GEOD_GEODESICSCALE)) {
		if(flags & GEOD_ARCMODE || fabs(pLine->f) > 0.01)
			B12 = SinCosSeries(TRUE, ssig2, csig2, pLine->C1a, nC1);
		AB1 = (1 + pLine->A1m1) * (B12 - pLine->B11);
	}
	// sin(bet2) = cos(alp0) * sin(sig2)
	sbet2 = pLine->calp0 * ssig2;
	// Alt: cbet2 = hypot(csig2, salp0 * ssig2);
	cbet2 = hypot(pLine->salp0, pLine->calp0 * csig2);
	if(cbet2 == 0)
	    // I.e., salp0 = 0, csig2 = 0.  Break the degeneracy in this case
		cbet2 = csig2 = _Tiny;
	// tan(alp0) = cos(sig2)*tan(alp2)
	salp2 = pLine->salp0; 
	calp2 = pLine->calp0 * csig2; /* No need to normalize */
	if(outmask & GEOD_DISTANCE)
		s12 = flags & GEOD_ARCMODE ? pLine->b * ((1 + pLine->A1m1) * sig12 + AB1) : s12_a12;
	if(outmask & GEOD_LONGITUDE) {
		double E = copysignx(1, pLine->salp0); // east or west going?
		// tan(omg2) = sin(alp0) * tan(sig2)
		somg2 = pLine->salp0 * ssig2; 
		comg2 = csig2;  /* No need to normalize */
		// omg12 = omg2 - omg1
		omg12 = flags & GEOD_LONG_UNROLL ? E * (sig12 - (atan2(ssig2, csig2) - atan2(pLine->ssig1, pLine->csig1)) +
			(atan2(E * somg2, comg2) - atan2(E * pLine->somg1, pLine->comg1))) : atan2(somg2 * pLine->comg1 - comg2 * pLine->somg1, comg2 * pLine->comg1 + somg2 * pLine->somg1);
		lam12 = omg12 + pLine->A3c * (sig12 + (SinCosSeries(TRUE, ssig2, csig2, pLine->C3a, nC3-1) - pLine->B31));
		lon12 = lam12 / SMathConst::PiDiv180;
		lon2 = flags & GEOD_LONG_UNROLL ? pLine->P1.Lon + lon12 : AngNormalize(AngNormalize(pLine->P1.Lon) + AngNormalize(lon12));
	}
	if(outmask & GEOD_LATITUDE)
		lat2 = atan2dx(sbet2, pLine->f1 * cbet2);
	if(outmask & GEOD_AZIMUTH)
		azi2 = atan2dx(salp2, calp2);
	if(outmask & (GEOD_REDUCEDLENGTH | GEOD_GEODESICSCALE)) {
		double B22 = SinCosSeries(TRUE, ssig2, csig2, pLine->C2a, nC2);
		double AB2 = (1 + pLine->A2m1) * (B22 - pLine->B21);
		double J12 = (pLine->A1m1 - pLine->A2m1) * sig12 + (AB1 - AB2);
		if(outmask & GEOD_REDUCEDLENGTH) {
			// Add parens around (csig1 * ssig2) and (ssig1 * csig2) to ensure
			// accurate cancellation in the case of coincident points.
			m12 = pLine->b * ((dn2 * (pLine->csig1 * ssig2) - pLine->dn1 * (pLine->ssig1 * csig2)) - pLine->csig1 * csig2 * J12);
		}
		if(outmask & GEOD_GEODESICSCALE) {
			const double t = pLine->k2 * (ssig2 - pLine->ssig1) * (ssig2 + pLine->ssig1) / (pLine->dn1 + dn2);
			M12 = csig12 + (t *  ssig2 -  csig2 * J12) * pLine->ssig1 / pLine->dn1;
			M21 = csig12 - (t * pLine->ssig1 - pLine->csig1 * J12) *  ssig2 /  dn2;
		}
	}
	if(outmask & GEOD_AREA) {
		double B42 = SinCosSeries(FALSE, ssig2, csig2, pLine->C4a, nC4);
		double salp12;
		double calp12;
		if(pLine->calp0 == 0 || pLine->salp0 == 0) {
			// alp12 = alp2 - alp1, used in atan2 so no need to normalize
			salp12 = salp2 * pLine->calp1 - calp2 * pLine->salp1;
			calp12 = calp2 * pLine->calp1 + salp2 * pLine->salp1;
		}
		else {
			/* tan(alp) = tan(alp0) * sec(sig)
			* tan(alp2-alp1) = (tan(alp2) -tan(alp1)) / (tan(alp2)*tan(alp1)+1)
			* = calp0 * salp0 * (csig1-csig2) / (salp0^2 + calp0^2 * csig1*csig2)
			* If csig12 > 0, write
			*   csig1 - csig2 = ssig12 * (csig1 * ssig12 / (1 + csig12) + ssig1)
			* else
			*   csig1 - csig2 = csig1 * (1 - csig12) + ssig12 * ssig1
			* No need to normalize */
			salp12 = pLine->calp0 * pLine->salp0 * (csig12 <= 0 ? pLine->csig1 * (1 - csig12) + ssig12 * pLine->ssig1 : ssig12 * (pLine->csig1 * ssig12 / (1 + csig12) + pLine->ssig1));
			calp12 = SQ(pLine->salp0) + SQ(pLine->calp0) * pLine->csig1 * csig2;
		}
		S12 = pLine->c2 * atan2(salp12, calp12) + pLine->A4 * (B42 - pLine->B41);
	}
	/*
	if(outmask & GEOD_LATITUDE)
		*plat2 = lat2;
	if(outmask & GEOD_LONGITUDE)
		*plon2 = lon2;
	*/
	if(pP2) {
		pP2->Lat = lat2;
		pP2->Lon = lon2;
	}
	if(outmask & GEOD_AZIMUTH)
		*pazi2 = azi2;
	if(outmask & GEOD_DISTANCE)
		*ps12 = s12;
	if(outmask & GEOD_REDUCEDLENGTH)
		*pm12 = m12;
	if(outmask & GEOD_GEODESICSCALE) {
		if(pM12) *pM12 = M12;
		if(pM21) *pM21 = M21;
	}
	if(outmask & GEOD_AREA)
		*pS12 = S12;
	return flags & GEOD_ARCMODE ? s12_a12 : sig12 / SMathConst::PiDiv180;
}

#if 0 // {
static void Init()
{
	if(!init) {
#if defined(__DBL_MANT_DIG__)
		digits = __DBL_MANT_DIG__;
#else
		digits = 53;
#endif
#if defined(__DBL_EPSILON__)
		epsilon = __DBL_EPSILON__;
#else
		epsilon = pow(0.5, digits - 1);
#endif
#if defined(__DBL_MIN__)
		realmin = __DBL_MIN__;
#else
		realmin = pow(0.5, 1022);
#endif
#if defined(M_PI)
		pi = M_PI;
#else
		pi = atan2(0.0, -1.0);
#endif
		maxit1 = 20;
		maxit2 = maxit1 + digits + 10;
		tiny = sqrt(realmin);
		tol0 = epsilon;
		// Increase multiplier in defn of tol1 from 100 to 200 to fix inverse case
		// 52.784459512564 0 -52.784459512563990912 179.634407464943777557
		// which otherwise failed for Visual Studio 10 (Release and Debug)
		tol1 = 200 * tol0;
		tol2 = sqrt(tol0);
		// Check on bisection interval
		tolb = tol0 * tol2;
		xthresh = 1000 * tol2;
		degree = pi/180;
		NaN = sqrt(-1.0);
		init = 1;
	}
}
#endif // } 0

double SGeo::Direct(/*const Geodesic & rG,*/const SGeoPosLL & rP1, double azi1, uint flags, double s12_a12,
	SGeoPosLL * pP2, double * pazi2, double * ps12, double * pm12, double * pM12, double * pM21, double * pS12)
{
	GeodesicLine l;
	uint outmask = // (plat2 ? GEOD_LATITUDE : 0U) | (plon2 ? GEOD_LONGITUDE : 0U) |
		(pP2 ? (GEOD_LATITUDE|GEOD_LONGITUDE) : 0) |
		(pazi2 ? GEOD_AZIMUTH : 0U) | (ps12 ? GEOD_DISTANCE : 0U) |
		(pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) | (pS12 ? GEOD_AREA : 0U);
	// Automatically supply GEOD_DISTANCE_IN if necessary
	LineInit(&l, /*rG,*/rP1, azi1, outmask | (flags & GEOD_ARCMODE ? GEOD_NONE : GEOD_DISTANCE_IN));
	return GenPosition(&l, flags, s12_a12, pP2, pazi2, ps12, pm12, pM12, pM21, pS12);
}

void SGeo::Lengths(/*const Geodesic & rG,*/double eps, double sig12, double ssig1, double csig1, double dn1,
	double ssig2, double csig2, double dn2, double cbet1, double cbet2, double* ps12b, double* pm12b, double* pm0, double* pM12, double* pM21,
	/* Scratch area of the right size */ double Ca[])
{
	double m0 = 0, J12 = 0, A1 = 0, A2 = 0;
	double Cb[nC];
	// Return m12b = (reduced length)/b; also calculate s12b = distance/b,
	// and m0 = coefficient of secular term in expression for reduced length.
	int redlp = pm12b || pm0 || pM12 || pM21; // bool
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
		double B1 = SinCosSeries(TRUE, ssig2, csig2, Ca, nC1) - SinCosSeries(TRUE, ssig1, csig1, Ca, nC1);
		// Missing a factor of b
		*ps12b = A1 * (sig12 + B1);
		if(redlp) {
			double B2 = SinCosSeries(TRUE, ssig2, csig2, Cb, nC2) - SinCosSeries(TRUE, ssig1, csig1, Cb, nC2);
			J12 = m0 * sig12 + (A1 * B1 - A2 * B2);
		}
	}
	else if(redlp) {
		// Assume here that nC1 >= nC2
		for(int l = 1; l <= nC2; ++l)
			Cb[l] = A1 * Ca[l] - A2 * Cb[l];
		J12 = m0 * sig12 + (SinCosSeries(TRUE, ssig2, csig2, Cb, nC2) - SinCosSeries(TRUE, ssig1, csig1, Cb, nC2));
	}
	ASSIGN_PTR(pm0, m0);
	if(pm12b) {
		// Missing a factor of b.
		// Add parens around (csig1 * ssig2) and (ssig1 * csig2) to ensure
		// accurate cancellation in the case of coincident points.
		*pm12b = dn2 * (csig1 * ssig2) - dn1 * (ssig1 * csig2) - csig1 * csig2 * J12;
	}
	if(pM12 || pM21) {
		double csig12 = csig1 * csig2 + ssig1 * ssig2;
		double t = G.Ep2 * (cbet1 - cbet2) * (cbet1 + cbet2) / (dn1 + dn2);
		if(pM12)
			*pM12 = csig12 + (t * ssig2 - csig2 * J12) * ssig1 / dn1;
		if(pM21)
			*pM21 = csig12 - (t * ssig1 - csig1 * J12) * ssig2 / dn2;
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
		uv = u < 0 ? q / (v - u) : u + v; /* u+v, guaranteed positive */
		w = (uv - q) / (2 * v);           /* positive? */
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

double SGeo::InverseStart(/*const Geodesic & rG,*/double sbet1, double cbet1, double dn1,
	double sbet2, double cbet2, double dn2, double lam12, double slam12, double clam12,
	double* psalp1, double* pcalp1, /* Only updated if return val >= 0 */ double* psalp2, double* pcalp2,
	/* Only updated for short lines */ double* pdnm, /* Scratch area of the right size */ double Ca[])
{
	double salp1 = 0;
	double calp1 = 0;
	double salp2 = 0;
	double calp2 = 0;
	double dnm = 0;
	// Return a starting point for Newton's method in salp1 and calp1 (function
	// value is -1).  If Newton's method doesn't need to be used, return also
	// salp2 and calp2 and function value is sig12.
	double sig12 = -1; // Return value
	// bet12 = bet2 - bet1 in [0, pi); bet12a = bet2 + bet1 in (-pi, 0]
	const double sbet12 = sbet2 * cbet1 - cbet2 * sbet1;
	const double cbet12 = cbet2 * cbet1 + sbet2 * sbet1;
	const int    shortline = BIN(cbet12 >= 0 && (sbet12 < 0.5) && ((cbet2 * lam12) < 0.5)); // bool
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
	const double sbet12a = sbet2 * cbet1 + cbet2 * sbet1;
#endif
	if(shortline) {
		double sbetm2 = SQ(sbet1 + sbet2), omg12;
		// sin((bet1+bet2)/2)^2 =  (sbet1 + sbet2)^2 / ((sbet1 + sbet2)^2 + (cbet1 + cbet2)^2)
		sbetm2 /= sbetm2 + SQ(cbet1 + cbet2);
		dnm = sqrt(1 + G.Ep2 * sbetm2);
		omg12 = lam12 / (G.F1 * dnm);
		somg12 = sin(omg12); 
		comg12 = cos(omg12);
	}
	else {
		somg12 = slam12; 
		comg12 = clam12;
	}
	salp1 = cbet2 * somg12;
	calp1 = comg12 >= 0 ? sbet12 + cbet2 * sbet1 * SQ(somg12) / (1 + comg12) : sbet12a - cbet2 * sbet1 * SQ(somg12) / (1 - comg12);
	ssig12 = hypot(salp1, calp1);
	csig12 = sbet1 * sbet2 + cbet1 * cbet2 * comg12;
	if(shortline && ssig12 < G.Etol2) {
		// really short lines
		salp2 = cbet1 * somg12;
		calp2 = sbet12 - cbet1 * sbet2 * (comg12 >= 0 ? SQ(somg12) / (1 + comg12) : 1 - comg12);
		norm2(&salp2, &calp2);
		// Set return value
		sig12 = atan2(ssig12, csig12);
	}
	else if((fabs(G.N) > 0.1) || /* No astroid calc if too eccentric */ csig12 >= 0 || ssig12 >= (6 * fabs(G.N) * SMathConst::Pi * SQ(cbet1))) {
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
		const double lam12x = atan2(-slam12, -clam12); // lam12 - pi
		if(G.F >= 0.0) { // In fact f == 0 does not get here
			// x = dlong, y = dlat
			{
				double k2 = SQ(sbet1) * G.Ep2;
				double eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
				lamscale = G.F * cbet1 * G.A3f_(eps) * SMathConst::Pi;
			}
			betscale = lamscale * cbet1;
			x = lam12x / lamscale;
			y = sbet12a / betscale;
		}
		else { // f < 0
			// x = dlat, y = dlong
			double cbet12a = cbet2 * cbet1 - sbet2 * sbet1;
			double bet12a = atan2(sbet12a, cbet12a);
			double m12b, m0;
			// In the case of lon12 = 180, this repeats a calculation made in Inverse
			Lengths(/*rG,*/G.N, SMathConst::Pi + bet12a, sbet1, -cbet1, dn1, sbet2, cbet2, dn2, cbet1, cbet2, 0, &m12b, &m0, 0, 0, Ca);
			x = -1 + m12b / (cbet1 * cbet2 * m0 * SMathConst::Pi);
			betscale = (x < -0.01) ? (sbet12a / x) : (-G.F * SQ(cbet1) * SMathConst::Pi);
			lamscale = betscale / cbet1;
			y = lam12x / lamscale;
		}
		if(y > -_Tol1 && x > -1 - (1000.0 * _Tol2/*xthresh*/)) {
			// strip near cut
			if(G.F >= 0.0) {
				salp1 = smin(1.0, -x); 
				calp1 = -sqrt(1 - SQ(salp1));
			}
			else {
				calp1 = smax(((x > -_Tol1) ? 0.0 : -1.0), x);
				salp1 = sqrt(1.0 - SQ(calp1));
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
			salp1 = cbet2 * somg12;
			calp1 = sbet12a - cbet2 * sbet1 * SQ(somg12) / (1 - comg12);
		}
	}
	// Sanity check on starting guess.  Backwards check allows NaN through.
	if(!(salp1 <= 0))
		norm2(&salp1, &calp1);
	else {
		salp1 = 1; calp1 = 0;
	}
	*psalp1 = salp1;
	*pcalp1 = calp1;
	if(shortline)
		*pdnm = dnm;
	if(sig12 >= 0) {
		*psalp2 = salp2;
		*pcalp2 = calp2;
	}
	return sig12;
}

double SGeo::Lambda12(/*const SGeo::Geodesic & rG,*/double sbet1, double cbet1, double dn1, double sbet2, double cbet2, double dn2,
	double salp1, double calp1, double slam120, double clam120, double* psalp2, double* pcalp2, double* psig12,
	double* pssig1, double* pcsig1, double* pssig2, double* pcsig2, double* peps, double* psomg12, double* pcomg12,
	int/*bool*/ diffp, double* pdlam12, /* Scratch area of the right size */ double Ca[])
{
	double salp2 = 0;
	double calp2 = 0;
	double sig12 = 0;
	double ssig1 = 0;
	double csig1 = 0;
	double ssig2 = 0;
	double csig2 = 0;
	double eps = 0;
	double somg12 = 0;
	double comg12 = 0;
	double dlam12 = 0;
	double salp0, calp0;
	double somg1, comg1, somg2, comg2, lam12;
	double B312, eta, k2;
	if(sbet1 == 0 && calp1 == 0)
		// Break degeneracy of equatorial line.  This case has already been handled.
		calp1 = -_Tiny;
	// sin(alp1) * cos(bet1) = sin(alp0)
	salp0 = salp1 * cbet1;
	calp0 = hypot(calp1, salp1 * sbet1); /* calp0 > 0 */

	// tan(bet1) = tan(sig1) * cos(alp1)
	// tan(omg1) = sin(alp0) * tan(sig1) = tan(omg1)=tan(alp1)*sin(bet1)
	ssig1 = sbet1; somg1 = salp0 * sbet1;
	csig1 = comg1 = calp1 * cbet1;
	norm2(&ssig1, &csig1);
	// norm2(&somg1, &comg1); -- don't need to normalize!

	// Enforce symmetries in the case abs(bet2) = -bet1.  Need to be careful
	// about this case, since this can yield singularities in the Newton iteration.
	// sin(alp2) * cos(bet2) = sin(alp0)
	salp2 = cbet2 != cbet1 ? salp0 / cbet2 : salp1;
	// calp2 = sqrt(1 - sq(salp2)) = sqrt(sq(calp0) - sq(sbet2)) / cbet2
	// and subst for calp0 and rearrange to give (choose positive sqrt
	// to give alp2 in [0, pi/2]).
	calp2 = cbet2 != cbet1 || fabs(sbet2) != -sbet1 ? sqrt(SQ(calp1 * cbet1) + (cbet1 < -sbet1 ? (cbet2 - cbet1) * (cbet1 + cbet2) : (sbet1 - sbet2) * (sbet1 + sbet2))) / cbet2 : fabs(calp1);
	// tan(bet2) = tan(sig2) * cos(alp2)
	// tan(omg2) = sin(alp0) * tan(sig2)
	ssig2 = sbet2; somg2 = salp0 * sbet2;
	csig2 = comg2 = calp2 * cbet2;
	norm2(&ssig2, &csig2);
	// norm2(&somg2, &comg2); -- don't need to normalize!

	// sig12 = sig2 - sig1, limit to [0, pi]
	sig12 = atan2(smax(0.0, csig1 * ssig2 - ssig1 * csig2),
	csig1 * csig2 + ssig1 * ssig2);

	// omg12 = omg2 - omg1, limit to [0, pi]
	somg12 = smax(0.0, comg1 * somg2 - somg1 * comg2);
	comg12 = comg1 * comg2 + somg1 * somg2;
	// eta = omg12 - lam120
	eta = atan2(somg12 * clam120 - comg12 * slam120,
	comg12 * clam120 + somg12 * slam120);
	k2 = SQ(calp0) * G.Ep2;
	eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
	G.C3f_(eps, Ca);
	B312 = (SinCosSeries(TRUE, ssig2, csig2, Ca, nC3-1) - SinCosSeries(TRUE, ssig1, csig1, Ca, nC3-1));
	lam12 = eta - G.F * G.A3f_(eps) * salp0 * (sig12 + B312);
	if(diffp) {
		if(calp2 == 0)
			dlam12 = - 2 * G.F1 * dn1 / sbet1;
		else {
			Lengths(/*rG,*/eps, sig12, ssig1, csig1, dn1, ssig2, csig2, dn2, cbet1, cbet2, 0, &dlam12, 0, 0, 0, Ca);
			dlam12 *= G.F1 / (calp2 * cbet2);
		}
	}
	*psalp2 = salp2;
	*pcalp2 = calp2;
	*psig12 = sig12;
	*pssig1 = ssig1;
	*pcsig1 = csig1;
	*pssig2 = ssig2;
	*pcsig2 = csig2;
	*peps = eps;
	*psomg12 = somg12;
	*pcomg12 = comg12;
	if(diffp)
		*pdlam12 = dlam12;
	return lam12;
}

double SGeo::Inverse_Int(/*const Geodesic & rG,*/SGeoPosLL & rP1, SGeoPosLL & rP2,
	double * ps12, double * psalp1, double * pcalp1, double * psalp2, double* pcalp2, double* pm12, double* pM12, double* pM21, double* pS12)
{
	double s12 = 0;
	double m12 = 0;
	double M12 = 0;
	double M21 = 0;
	double S12 = 0;
	double lon12s;
	int    latsign;
	int    swapp;
	double sbet1;
	double cbet1;
	double sbet2;
	double cbet2;
	double s12x = 0;
	double m12x = 0;
	double dn1;
	double dn2;
	double lam12;
	double slam12;
	double clam12;
	double a12 = 0;
	double sig12;
	double calp1 = 0;
	double salp1 = 0;
	double calp2 = 0;
	double salp2 = 0;
	double Ca[nC];
	int meridian; // bool
	// somg12 > 1 marks that it needs to be calculated
	double omg12 = 0, somg12 = 2, comg12 = 0;
	unsigned outmask = (ps12 ? GEOD_DISTANCE : 0U) | (pm12 ? GEOD_REDUCEDLENGTH : 0U) | (pM12 || pM21 ? GEOD_GEODESICSCALE : 0U) | (pS12 ? GEOD_AREA : 0U);
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
		sincosdx(lon12s, &slam12, &clam12);
		clam12 = -clam12;
	}
	else
		sincosdx(lon12, &slam12, &clam12);
	// If really close to the equator, treat as on equator.
	rP1.Lat = AngRound(LatFix(rP1.Lat));
	rP2.Lat = AngRound(LatFix(rP2.Lat));
	// Swap points so that point with higher (abs) latitude is point 1
	// If one latitude is a nan, then it becomes lat1.
	swapp = fabs(rP1.Lat) < fabs(rP2.Lat) ? -1 : 1;
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
	sincosdx(rP1.Lat, &sbet1, &cbet1);
	sbet1 *= G.F1;
	// Ensure cbet1 = +epsilon at poles
	norm2(&sbet1, &cbet1);
	cbet1 = smax(_Tiny, cbet1);
	sincosdx(rP2.Lat, &sbet2, &cbet2);
	sbet2 *= G.F1;
	// Ensure cbet2 = +epsilon at poles
	norm2(&sbet2, &cbet2);
	cbet2 = smax(_Tiny, cbet2);
	//
	// If cbet1 < -sbet1, then cbet2 - cbet1 is a sensitive measure of the
	// |bet1| - |bet2|.  Alternatively (cbet1 >= -sbet1), abs(sbet2) + sbet1 is
	// a better measure.  This logic is used in assigning calp2 in Lambda12.
	// Sometimes these quantities vanish and in that case we force bet2 = +/-
	// bet1 exactly.  An example where is is necessary is the inverse problem
	// 48.522876735459 0 -48.52287673545898293 179.599720456223079643
	// which failed with Visual Studio 10 (Release and Debug)
	//
	if(cbet1 < -sbet1) {
		if(cbet2 == cbet1)
			sbet2 = (sbet2 < 0) ? sbet1 : -sbet1;
	}
	else {
		if(fabs(sbet2) == -sbet1)
			cbet2 = cbet1;
	}
	dn1 = sqrt(1 + G.Ep2 * SQ(sbet1));
	dn2 = sqrt(1 + G.Ep2 * SQ(sbet2));
	meridian = (rP1.Lat == -90 || slam12 == 0);
	if(meridian) {
		// Endpoints are on a single full meridian, so the geodesic might lie on a meridian.
		double ssig1;
		double csig1;
		double ssig2;
		double csig2;
		calp1 = clam12; 
		salp1 = slam12; // Head to the target longitude 
		calp2 = 1; 
		salp2 = 0; // At the target we're heading north
		// tan(bet) = tan(sig) * cos(alp)
		ssig1 = sbet1; 
		csig1 = calp1 * cbet1;
		ssig2 = sbet2; 
		csig2 = calp2 * cbet2;
		// sig12 = sig2 - sig1
		sig12 = atan2(smax(0.0, csig1 * ssig2 - ssig1 * csig2), csig1 * csig2 + ssig1 * ssig2);
		Lengths(/*rG,*/G.N, sig12, ssig1, csig1, dn1, ssig2, csig2, dn2, cbet1, cbet2, &s12x, &m12x, 0, outmask & GEOD_GEODESICSCALE ? &M12 : 0, outmask & GEOD_GEODESICSCALE ? &M21 : 0, Ca);
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
	if(!meridian && sbet1 == 0 && /* and sbet2 == 0 */ /* Mimic the way Lambda12 works with calp1 = 0 */ (G.F <= 0.0 || lon12s >= (G.F * 180.0))) {
		// Geodesic runs along equator
		calp1 = calp2 = 0; 
		salp1 = salp2 = 1;
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
		sig12 = InverseStart(/*rG,*/sbet1, cbet1, dn1, sbet2, cbet2, dn2, lam12, slam12, clam12, &salp1, &calp1, &salp2, &calp2, &dnm, Ca);
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
			double ssig1 = 0;
			double csig1 = 0;
			double ssig2 = 0;
			double csig2 = 0;
			double eps = 0;
			uint   numit = 0;
			// Bracketing range
			double salp1a = _Tiny;
			double calp1a = 1;
			double salp1b = _Tiny;
			double calp1b = -1;
			int    tripn, tripb; // bool
			for(tripn = FALSE, tripb = FALSE; numit < _MaxIt2; ++numit) {
				// the WGS84 test set: mean = 1.47, sd = 1.25, max = 16
				// WGS84 and random input: mean = 2.85, sd = 0.60
				double dv = 0,
				v = Lambda12(/*rG,*/sbet1, cbet1, dn1, sbet2, cbet2, dn2, salp1, calp1, slam12, clam12,
					&salp2, &calp2, &sig12, &ssig1, &csig1, &ssig2, &csig2, &eps, &somg12, &comg12, numit < _MaxIt1, &dv, Ca);
				// 2 * tol0 is approximately 1 ulp for a number in [0, pi].
				// Reversed test to allow escape with NaNs
				if(tripb || !(fabs(v) >= (tripn ? 8 : 1) * _Tol0))
					break;
				// Update bracketing values
				if(v > 0 && (numit > _MaxIt1 || calp1/salp1 > calp1b/salp1b)) {
					salp1b = salp1;
					calp1b = calp1;
				}
				else if(v < 0 && (numit > _MaxIt1 || calp1/salp1 < calp1a/salp1a)) {
					salp1a = salp1;
					calp1a = calp1;
				}
				if(numit < _MaxIt1 && dv > 0) {
					const double dalp1 = -v/dv;
					const double sdalp1 = sin(dalp1);
					const double cdalp1 = cos(dalp1);
					const double nsalp1 = salp1 * cdalp1 + calp1 * sdalp1;
					if(nsalp1 > 0 && fabs(dalp1) < SMathConst::Pi/*pi*/) {
						calp1 = calp1 * cdalp1 - salp1 * sdalp1;
						salp1 = nsalp1;
						norm2(&salp1, &calp1);
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
				salp1 = (salp1a + salp1b)/2;
				calp1 = (calp1a + calp1b)/2;
				norm2(&salp1, &calp1);
				tripn = FALSE;
				tripb = (fabs(salp1a - salp1) + (calp1a - calp1) < _Tolb || fabs(salp1 - salp1b) + (calp1 - calp1b) < _Tolb);
			}
			Lengths(/*rG,*/eps, sig12, ssig1, csig1, dn1, ssig2, csig2, dn2, cbet1, cbet2, &s12x, &m12x, 0,
				outmask & GEOD_GEODESICSCALE ? &M12 : 0, outmask & GEOD_GEODESICSCALE ? &M21 : 0, Ca);
			m12x *= G.B;
			s12x *= G.B;
			a12 = sig12 / SMathConst::PiDiv180;
		}
	}
	if(outmask & GEOD_DISTANCE)
		s12 = 0 + s12x; // Convert -0 to 0
	if(outmask & GEOD_REDUCEDLENGTH)
		m12 = 0 + m12x; // Convert -0 to 0
	if(outmask & GEOD_AREA) {
		// From Lambda12: sin(alp1) * cos(bet1) = sin(alp0)
		double salp0 = salp1 * cbet1;
		double calp0 = hypot(calp1, salp1 * sbet1); // calp0 > 0
		double alp12;
		if(calp0 != 0 && salp0 != 0) {
			// From Lambda12: tan(bet) = tan(sig) * cos(alp)
			double ssig1 = sbet1;
			double csig1 = calp1 * cbet1;
			double ssig2 = sbet2;
			double csig2 = calp2 * cbet2;
			double k2 = SQ(calp0) * G.Ep2;
			double eps = k2 / (2 * (1 + sqrt(1 + k2)) + k2);
			// Multiplier = a^2 * e^2 * cos(alpha0) * sin(alpha0).
			double A4 = SQ(G.A) * calp0 * salp0 * G.E2;
			double B41, B42;
			norm2(&ssig1, &csig1);
			norm2(&ssig2, &csig2);
			G.C4f_(eps, Ca);
			B41 = SinCosSeries(FALSE, ssig1, csig1, Ca, nC4);
			B42 = SinCosSeries(FALSE, ssig2, csig2, Ca, nC4);
			S12 = A4 * (B42 - B41);
		}
		else // Avoid problems with indeterminate sig1, sig2 on equator
			S12 = 0;
		if(!meridian) {
			if(somg12 > 1) {
				somg12 = sin(omg12);
				comg12 = cos(omg12);
			}
			else
				norm2(&somg12, &comg12);
		}
		if(!meridian && /* omg12 < 3/4 * pi */ (comg12 > -0.7071) && /* Long difference not too big */ (sbet2 - sbet1) < 1.75) { // Lat difference not too big
			// Use tan(Gamma/2) = tan(omg12/2) * (tan(bet1/2)+tan(bet2/2))/(1+tan(bet1/2)*tan(bet2/2))
			// with tan(x/2) = sin(x)/(1+cos(x))
			double domg12 = 1 + comg12;
			double dbet1 = 1 + cbet1;
			double dbet2 = 1 + cbet2;
			alp12 = 2 * atan2(somg12 * (sbet1 * dbet2 + sbet2 * dbet1), domg12 * (sbet1 * sbet2 + dbet1 * dbet2));
		}
		else {
			// alp12 = alp2 - alp1, used in atan2 so no need to normalize
			double salp12 = salp2 * calp1 - calp2 * salp1;
			double calp12 = calp2 * calp1 + salp2 * salp1;
			// The right thing appears to happen if alp1 = +/-180 and alp2 = 0, viz
			// salp12 = -0 and alp12 = -180.  However this depends on the sign
			// being attached to 0 correctly.  The following ensures the correct behavior.
			if(salp12 == 0 && calp12 < 0) {
				salp12 = _Tiny * calp1;
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
		Exchange(&salp1, &salp2);
		Exchange(&calp1, &calp2);
		if(outmask & GEOD_GEODESICSCALE)
			Exchange(&M12, &M21);
	}
	salp1 *= swapp * lonsign;
	calp1 *= swapp * latsign;
	salp2 *= swapp * lonsign;
	calp2 *= swapp * latsign;
	ASSIGN_PTR(psalp1, salp1);
	ASSIGN_PTR(pcalp1, calp1);
	ASSIGN_PTR(psalp2, salp2);
	ASSIGN_PTR(pcalp2, calp2);
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


double SGeo::Inverse(/*const Geodesic & rG,*/const SGeoPosLL & rP1, /*real lat2, real lon2*/const SGeoPosLL & rP2,
	double * ps12, double * pazi1, double * pazi2, double * pm12, double * pM12, double * pM21, double * pS12)
{
	/*real geod_geninverse(const struct geod_geodesic* g, real lat1, real lon1, real lat2, real lon2,
		real* ps12, real* pazi1, real* pazi2, real* pm12, real* pM12, real* pM21, real* pS12) {*/
	double salp1, calp1, salp2, calp2;
	SGeoPosLL p1 = rP1;
	SGeoPosLL p2 = rP2;
	double a12 = Inverse_Int(/*rG,*//*lat1, lon1, lat2, lon2*/p1, p2, ps12, &salp1, &calp1, &salp2, &calp2, pm12, pM12, pM21, pS12);
	if(pazi1)
		*pazi1 = atan2dx(salp1, calp1);
	if(pazi2)
		*pazi2 = atan2dx(salp2, calp2);
	return a12;
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
