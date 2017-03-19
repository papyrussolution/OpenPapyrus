// SGEO.CPP
// Copyright (c) A.Sobolev 2009, 2010, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

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
