// SGEO.CPP
// Copyright (c) A.Sobolev 2009, 2010, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#define GIS_EPSILON 0.000001

SGeoPosLL::SGeoPosLL()
{
	Latitude = Longitude = 0.0;
}

SGeoPosLL::SGeoPosLL(double lat, double lng)
{
	Latitude = lat;
	Longitude = lng;
}

int SGeoPosLL::operator == (const SGeoPosLL & s) const
{
	return BIN(Cmp(s) == 0);
}

int SGeoPosLL::operator != (const SGeoPosLL & s) const
{
	return BIN(Cmp(s) != 0);
}

int SGeoPosLL::Cmp(const SGeoPosLL & s) const
{
	double d = (Latitude - s.Latitude);
	if(fabs(d) > GIS_EPSILON) {
		if(d < 0.0)
			return -1;
		else
			return 1;
	}
	else {
		d = (Longitude - s.Longitude);
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
	int    ok = 1;
	if(Latitude < -90. || Latitude > 90.) {
		SLS.SetError(SLERR_INVGEOLATITUDE);
		ok = 0;
	}
	else if(Longitude < -180. || Longitude > 180.) {
		SLS.SetError(SLERR_INVGEOLONGITUDE);
		ok = 0;
	}
	return ok;
}

SString & SGeoPosLL::ToStr(SString & rBuf) const
{
	rBuf = 0;
	if(Latitude != 0.0 || Longitude != 0.0) {
		rBuf.Cat(Latitude, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatDiv(',', 2).
			Cat(Longitude, MKSFMTD(0, 6, NMBF_NOTRAILZ));
	}
	return rBuf;
}

int SGeoPosLL::FromStr(const char * pStr)
{
	int    ok = -1;
	SString nmb;
	SStrScan scan(pStr);
	if(scan.GetNumber(nmb)) {
		Latitude = nmb.ToReal();
		scan.Skip(SStrScan::wsSpace | SStrScan::wsTab | SStrScan::wsComma | SStrScan::wsSemicol);
		if(scan.GetNumber(nmb)) {
			Longitude = nmb.ToReal();
			ok = Valid();
		}
		else
			Longitude = 0.0;
	}
	else {
		Latitude = 0.0;
		Longitude = 0.0;
	}
	return ok;
}
