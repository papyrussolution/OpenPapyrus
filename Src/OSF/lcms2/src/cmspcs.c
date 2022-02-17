//
//  Little Color Management System
//  Copyright (c) 1998-2020 Marti Maria Saguer
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
#include "lcms2_internal.h"
#pragma hdrstop

//      inter PCS conversions XYZ <-> CIE L* a* b*
/*


       CIE 15:2004 CIELab is defined as:

       L* = 116*f(Y/Yn) - 16                     0 <= L* <= 100
       a* = 500*[f(X/Xn) - f(Y/Yn)]
       b* = 200*[f(Y/Yn) - f(Z/Zn)]

       and

              f(t) = t^(1/3)                     1 >= t >  (24/116)^3
                     (841/108)*t + (16/116)      0 <= t <= (24/116)^3


       Reverse transform is:

       X = Xn*[a* / 500 + (L* + 16) / 116] ^ 3   if(X/Xn) > (24/116)
 = Xn*(a* / 500 + L* / 116) / 7.787      if(X/Xn) <= (24/116)



       PCS in Lab2 is encoded as:

              8 bit Lab PCS:

                     L*      0..100 into a 0..ff byte.
                     a*      t + 128 range is -128.0  +127.0
                     b*

             16 bit Lab PCS:

                     L*     0..100  into a 0..ff00 word.
                     a*     t + 128  range is  -128.0  +127.9961
                     b*



   Interchange Space   Component     Actual Range        Encoded Range
   CIE XYZ             X             0 -> 1.99997        0x0000 -> 0xffff
   CIE XYZ             Y             0 -> 1.99997        0x0000 -> 0xffff
   CIE XYZ             Z             0 -> 1.99997        0x0000 -> 0xffff

   Version 2,3
   -----------

   CIELAB (16 bit)     L*            0 -> 100.0          0x0000 -> 0xff00
   CIELAB (16 bit)     a*            -128.0 -> +127.996  0x0000 -> 0x8000 -> 0xffff
   CIELAB (16 bit)     b*            -128.0 -> +127.996  0x0000 -> 0x8000 -> 0xffff


   Version 4
   ---------

   CIELAB (16 bit)     L*            0 -> 100.0          0x0000 -> 0xffff
   CIELAB (16 bit)     a*            -128.0 -> +127      0x0000 -> 0x8080 -> 0xffff
   CIELAB (16 bit)     b*            -128.0 -> +127      0x0000 -> 0x8080 -> 0xffff

 */

// Conversions
void CMSEXPORT cmsXYZ2xyY(cmsCIExyY* Dest, const cmsCIEXYZ* Source)
{
	double ISum;

	ISum = 1./(Source->X + Source->Y + Source->Z);

	Dest->x = (Source->X) * ISum;
	Dest->y = (Source->Y) * ISum;
	Dest->Y = Source->Y;
}

void CMSEXPORT cmsxyY2XYZ(cmsCIEXYZ* Dest, const cmsCIExyY* Source)
{
	Dest->X = (Source->x / Source->y) * Source->Y;
	Dest->Y = Source->Y;
	Dest->Z = ((1 - Source->x - Source->y) / Source->y) * Source->Y;
}

/*
       The break point (24/116)^3 = (6/29)^3 is a very small amount of tristimulus
       primary (0.008856).  Generally, this only happens for
       nearly ideal blacks and for some orange / amber colors in transmission mode.
       For example, the Z value of the orange turn indicator lamp lens on an
       automobile will often be below this value.  But the Z does not
       contribute to the perceived color directly.
 */

static double f(double t)
{
	const double Limit = (24.0/116.0) * (24.0/116.0) * (24.0/116.0);
	if(t <= Limit)
		return (841.0/108.0) * t + (16.0/116.0);
	else
		return pow(t, 1.0/3.0);
}

static double f_1(double t)
{
	const double Limit = (24.0/116.0);
	if(t <= Limit) {
		return (108.0/841.0) * (t - (16.0/116.0));
	}
	return t * t * t;
}

// Standard XYZ to Lab. it can handle negative XZY numbers in some cases
void CMSEXPORT cmsXYZ2Lab(const cmsCIEXYZ* WhitePoint, cmsCIELab* Lab, const cmsCIEXYZ* xyz)
{
	double fx, fy, fz;
	if(WhitePoint == NULL)
		WhitePoint = cmsD50_XYZ();
	fx = f(xyz->X / WhitePoint->X);
	fy = f(xyz->Y / WhitePoint->Y);
	fz = f(xyz->Z / WhitePoint->Z);
	Lab->L = 116.0*fy - 16.0;
	Lab->a = 500.0*(fx - fy);
	Lab->b = 200.0*(fy - fz);
}

// Standard XYZ to Lab. It can return negative XYZ in some cases
void CMSEXPORT cmsLab2XYZ(const cmsCIEXYZ* WhitePoint, cmsCIEXYZ* xyz,  const cmsCIELab* Lab)
{
	double x, y, z;
	if(WhitePoint == NULL)
		WhitePoint = cmsD50_XYZ();
	y = (Lab->L + 16.0) / 116.0;
	x = y + 0.002 * Lab->a;
	z = y - 0.005 * Lab->b;

	xyz->X = f_1(x) * WhitePoint->X;
	xyz->Y = f_1(y) * WhitePoint->Y;
	xyz->Z = f_1(z) * WhitePoint->Z;
}

static double L2float2(uint16 v)
{
	return (double)v / 652.800;
}

// the a/b part
static double ab2float2(uint16 v)
{
	return ((double)v / 256.0) - 128.0;
}

static uint16 L2Fix2(double L)
{
	return _cmsQuickSaturateWord(L *  652.8);
}

static uint16 ab2Fix2(double ab)
{
	return _cmsQuickSaturateWord((ab + 128.0) * 256.0);
}

static double L2float4(uint16 v)
{
	return (double)v / 655.35;
}

// the a/b part
static double ab2float4(uint16 v)
{
	return ((double)v / 257.0) - 128.0;
}

void CMSEXPORT cmsLabEncoded2FloatV2(cmsCIELab* Lab, const uint16 wLab[3])
{
	Lab->L = L2float2(wLab[0]);
	Lab->a = ab2float2(wLab[1]);
	Lab->b = ab2float2(wLab[2]);
}

void CMSEXPORT cmsLabEncoded2Float(cmsCIELab* Lab, const uint16 wLab[3])
{
	Lab->L = L2float4(wLab[0]);
	Lab->a = ab2float4(wLab[1]);
	Lab->b = ab2float4(wLab[2]);
}

static double Clamp_L_doubleV2(double L)
{
	const double L_max = (double)(0xFFFF * 100.0) / 0xFF00;
	if(L < 0) L = 0;
	if(L > L_max) L = L_max;
	return L;
}

static double Clamp_ab_doubleV2(double ab)
{
	if(ab < MIN_ENCODEABLE_ab2) ab = MIN_ENCODEABLE_ab2;
	if(ab > MAX_ENCODEABLE_ab2) ab = MAX_ENCODEABLE_ab2;
	return ab;
}

void CMSEXPORT cmsFloat2LabEncodedV2(uint16 wLab[3], const cmsCIELab* fLab)
{
	cmsCIELab Lab;
	Lab.L = Clamp_L_doubleV2(fLab->L);
	Lab.a = Clamp_ab_doubleV2(fLab->a);
	Lab.b = Clamp_ab_doubleV2(fLab->b);
	wLab[0] = L2Fix2(Lab.L);
	wLab[1] = ab2Fix2(Lab.a);
	wLab[2] = ab2Fix2(Lab.b);
}

static double Clamp_L_doubleV4(double L)
{
	if(L < 0) L = 0;
	if(L > 100.0) L = 100.0;

	return L;
}

static double Clamp_ab_doubleV4(double ab)
{
	if(ab < MIN_ENCODEABLE_ab4) ab = MIN_ENCODEABLE_ab4;
	if(ab > MAX_ENCODEABLE_ab4) ab = MAX_ENCODEABLE_ab4;

	return ab;
}

static uint16 L2Fix4(double L)
{
	return _cmsQuickSaturateWord(L *  655.35);
}

static uint16 ab2Fix4(double ab)
{
	return _cmsQuickSaturateWord((ab + 128.0) * 257.0);
}

void CMSEXPORT cmsFloat2LabEncoded(uint16 wLab[3], const cmsCIELab* fLab)
{
	cmsCIELab Lab;
	Lab.L = Clamp_L_doubleV4(fLab->L);
	Lab.a = Clamp_ab_doubleV4(fLab->a);
	Lab.b = Clamp_ab_doubleV4(fLab->b);
	wLab[0] = L2Fix4(Lab.L);
	wLab[1] = ab2Fix4(Lab.a);
	wLab[2] = ab2Fix4(Lab.b);
}

// Auxiliary: convert to Radians
static double RADIANS(double deg) { return (deg * SMathConst::PiDiv180); }

// Auxiliary: atan2 but operating in degrees and returning 0 if a==b==0
static double atan2deg(double a, double b)
{
	double h;
	if(a == 0 && b == 0)
		h   = 0;
	else
		h = atan2(a, b);
	h *= (180.0 / SMathConst::Pi);
	while(h > 360.0)
		h -= 360.0;
	while(h < 0)
		h += 360.0;
	return h;
}

// Auxiliary: Square
static double Sqr(double v)
{
	return v *  v;
}

// From cylindrical coordinates. No check is performed, then negative values are allowed
void CMSEXPORT cmsLab2LCh(cmsCIELCh* LCh, const cmsCIELab* Lab)
{
	LCh->L = Lab->L;
	LCh->C = pow(Sqr(Lab->a) + Sqr(Lab->b), 0.5);
	LCh->h = atan2deg(Lab->b, Lab->a);
}

// To cylindrical coordinates. No check is performed, then negative values are allowed
void CMSEXPORT cmsLCh2Lab(cmsCIELab* Lab, const cmsCIELCh* LCh)
{
	double h = (LCh->h * SMathConst::PiDiv180);
	Lab->L = LCh->L;
	Lab->a = LCh->C * cos(h);
	Lab->b = LCh->C * sin(h);
}

// In XYZ All 3 components are encoded using 1.15 fixed point
static uint16 XYZ2Fix(double d)
{
	return _cmsQuickSaturateWord(d * 32768.0);
}

void CMSEXPORT cmsFloat2XYZEncoded(uint16 XYZ[3], const cmsCIEXYZ* fXYZ)
{
	cmsCIEXYZ xyz;
	xyz.X = fXYZ->X;
	xyz.Y = fXYZ->Y;
	xyz.Z = fXYZ->Z;
	// Clamp to encodeable values.
	if(xyz.Y <= 0) {
		xyz.X = 0;
		xyz.Y = 0;
		xyz.Z = 0;
	}

	if(xyz.X > MAX_ENCODEABLE_XYZ)
		xyz.X = MAX_ENCODEABLE_XYZ;

	if(xyz.X < 0)
		xyz.X = 0;

	if(xyz.Y > MAX_ENCODEABLE_XYZ)
		xyz.Y = MAX_ENCODEABLE_XYZ;

	if(xyz.Y < 0)
		xyz.Y = 0;

	if(xyz.Z > MAX_ENCODEABLE_XYZ)
		xyz.Z = MAX_ENCODEABLE_XYZ;

	if(xyz.Z < 0)
		xyz.Z = 0;

	XYZ[0] = XYZ2Fix(xyz.X);
	XYZ[1] = XYZ2Fix(xyz.Y);
	XYZ[2] = XYZ2Fix(xyz.Z);
}

//  To convert from Fixed 1.15 point to double
static double XYZ2float(uint16 v)
{
	// From 1.15 to 15.16
	cmsS15Fixed16Number fix32 = v << 1;
	// From fixed 15.16 to double
	return _cms15Fixed16toDouble(fix32);
}

void CMSEXPORT cmsXYZEncoded2Float(cmsCIEXYZ* fXYZ, const uint16 XYZ[3])
{
	fXYZ->X = XYZ2float(XYZ[0]);
	fXYZ->Y = XYZ2float(XYZ[1]);
	fXYZ->Z = XYZ2float(XYZ[2]);
}

// Returns dE on two Lab values
double CMSEXPORT cmsDeltaE(const cmsCIELab* Lab1, const cmsCIELab* Lab2)
{
	double dL = fabs(Lab1->L - Lab2->L);
	double da = fabs(Lab1->a - Lab2->a);
	double db = fabs(Lab1->b - Lab2->b);
	return pow(Sqr(dL) + Sqr(da) + Sqr(db), 0.5);
}

// Return the CIE94 Delta E
double CMSEXPORT cmsCIE94DeltaE(const cmsCIELab* Lab1, const cmsCIELab* Lab2)
{
	cmsCIELCh LCh1, LCh2;
	double dE, dL, dC, dh, dhsq;
	double c12, sc, sh;
	dL = fabs(Lab1->L - Lab2->L);
	cmsLab2LCh(&LCh1, Lab1);
	cmsLab2LCh(&LCh2, Lab2);
	dC  = fabs(LCh1.C - LCh2.C);
	dE  = cmsDeltaE(Lab1, Lab2);
	dhsq = Sqr(dE) - Sqr(dL) - Sqr(dC);
	if(dhsq < 0)
		dh = 0;
	else
		dh = pow(dhsq, 0.5);

	c12 = sqrt(LCh1.C * LCh2.C);

	sc = 1.0 + (0.048 * c12);
	sh = 1.0 + (0.014 * c12);

	return sqrt(Sqr(dL)  + Sqr(dC) / Sqr(sc) + Sqr(dh) / Sqr(sh));
}

// Auxiliary
static double ComputeLBFD(const cmsCIELab* Lab)
{
	double yt;
	if(Lab->L > 7.996969)
		yt = (Sqr((Lab->L+16)/116)*((Lab->L+16)/116))*100;
	else
		yt = 100 * (Lab->L / 903.3);
	return (54.6 * (M_LOG10E * (log(yt + 1.5))) - 9.6);
}

// bfd - gets BFD(1:1) difference between Lab1, Lab2
double CMSEXPORT cmsBFDdeltaE(const cmsCIELab* Lab1, const cmsCIELab* Lab2)
{
	double lbfd1, lbfd2, AveC, Aveh, dE, deltaL, deltaC, deltah, dc, t, g, dh, rh, rc, rt, bfd;
	cmsCIELCh LCh1, LCh2;
	lbfd1 = ComputeLBFD(Lab1);
	lbfd2 = ComputeLBFD(Lab2);
	deltaL = lbfd2 - lbfd1;
	cmsLab2LCh(&LCh1, Lab1);
	cmsLab2LCh(&LCh2, Lab2);
	deltaC = LCh2.C - LCh1.C;
	AveC = (LCh1.C+LCh2.C)/2;
	Aveh = (LCh1.h+LCh2.h)/2;
	dE = cmsDeltaE(Lab1, Lab2);
	if(Sqr(dE)>(Sqr(Lab2->L-Lab1->L)+Sqr(deltaC)))
		deltah = sqrt(Sqr(dE)-Sqr(Lab2->L-Lab1->L)-Sqr(deltaC));
	else
		deltah = 0;

	dc   = 0.035 * AveC / (1 + 0.00365 * AveC)+0.521;
	g    = sqrt(Sqr(Sqr(AveC))/(Sqr(Sqr(AveC))+14000));
	t    = 0.627+(0.055*cos((Aveh-254)/(180/SMathConst::Pi))- 0.040*cos((2*Aveh-136)/(180/SMathConst::Pi)) + 0.070*cos((3*Aveh-31)/(180/SMathConst::Pi)) +
	    0.049*cos((4*Aveh+114)/(180/SMathConst::Pi)) - 0.015*cos((5*Aveh-103)/(180/SMathConst::Pi)));
	dh    = dc*(g*t+1-g);
	rh    = -0.260*cos((Aveh-308)/(180/SMathConst::Pi)) - 0.379*cos((2*Aveh-160)/(180/SMathConst::Pi)) - 0.636*cos((3*Aveh+254)/(180/SMathConst::Pi)) +
	    0.226*cos((4*Aveh+140)/(180/SMathConst::Pi)) - 0.194*cos((5*Aveh+280)/(180/SMathConst::Pi));
	rc = sqrt((AveC*AveC*AveC*AveC*AveC*AveC)/((AveC*AveC*AveC*AveC*AveC*AveC)+70000000));
	rt = rh*rc;
	bfd = sqrt(Sqr(deltaL)+Sqr(deltaC/dc)+Sqr(deltah/dh)+(rt*(deltaC/dc)*(deltah/dh)));
	return bfd;
}

//  cmc - CMC(l:c) difference between Lab1, Lab2
double CMSEXPORT cmsCMCdeltaE(const cmsCIELab* Lab1, const cmsCIELab* Lab2, double l, double c)
{
	double dE, dL, dC, dh, sl, sc, sh, t, f, cmc;
	cmsCIELCh LCh1, LCh2;
	if(Lab1->L == 0 && Lab2->L == 0) 
		return 0;
	cmsLab2LCh(&LCh1, Lab1);
	cmsLab2LCh(&LCh2, Lab2);
	dL = Lab2->L-Lab1->L;
	dC = LCh2.C-LCh1.C;
	dE = cmsDeltaE(Lab1, Lab2);
	if(Sqr(dE)>(Sqr(dL)+Sqr(dC)))
		dh = sqrt(Sqr(dE)-Sqr(dL)-Sqr(dC));
	else
		dh = 0;
	if((LCh1.h > 164) && (LCh1.h < 345))
		t = 0.56 + fabs(0.2 * cos(((LCh1.h + 168)/(180/SMathConst::Pi))));
	else
		t = 0.36 + fabs(0.4 * cos(((LCh1.h + 35 )/(180/SMathConst::Pi))));
	sc  = 0.0638   * LCh1.C / (1 + 0.0131  * LCh1.C) + 0.638;
	sl  = 0.040975 * Lab1->L /(1 + 0.01765 * Lab1->L);
	if(Lab1->L<16)
		sl = 0.511;
	f   = sqrt((LCh1.C * LCh1.C * LCh1.C * LCh1.C)/((LCh1.C * LCh1.C * LCh1.C * LCh1.C)+1900));
	sh  = sc*(t*f+1-f);
	cmc = sqrt(Sqr(dL/(l*sl))+Sqr(dC/(c*sc))+Sqr(dh/sh));
	return cmc;
}

// dE2000 The weightings KL, KC and KH can be modified to reflect the relative
// importance of lightness, chroma and hue in different industrial applications
double CMSEXPORT cmsCIE2000DeltaE(const cmsCIELab* Lab1, const cmsCIELab* Lab2, double Kl, double Kc, double Kh)
{
	double L1  = Lab1->L;
	double a1  = Lab1->a;
	double b1  = Lab1->b;
	double C   = sqrt(Sqr(a1) + Sqr(b1));
	double Ls = Lab2->L;
	double as = Lab2->a;
	double bs = Lab2->b;
	double Cs = sqrt(Sqr(as) + Sqr(bs));
	double G = 0.5 * ( 1 - sqrt(pow((C + Cs) / 2, 7.0) / (pow((C + Cs) / 2, 7.0) + pow(25.0, 7.0) ) ));
	double a_p = (1 + G ) * a1;
	double b_p = b1;
	double C_p = sqrt(Sqr(a_p) + Sqr(b_p));
	double h_p = atan2deg(b_p, a_p);

	double a_ps = (1 + G) * as;
	double b_ps = bs;
	double C_ps = sqrt(Sqr(a_ps) + Sqr(b_ps));
	double h_ps = atan2deg(b_ps, a_ps);

	double meanC_p = (C_p + C_ps) / 2;

	double hps_plus_hp  = h_ps + h_p;
	double hps_minus_hp = h_ps - h_p;

	double meanh_p = fabs(hps_minus_hp) <= 180.000001 ? (hps_plus_hp)/2 :
	    (hps_plus_hp) < 360 ? (hps_plus_hp + 360)/2 :
	    (hps_plus_hp - 360)/2;

	double delta_h = (hps_minus_hp) <= -180.000001 ?  (hps_minus_hp + 360) :
	    (hps_minus_hp) > 180 ? (hps_minus_hp - 360) :
	    (hps_minus_hp);
	double delta_L = (Ls - L1);
	double delta_C = (C_ps - C_p );

	double delta_H = 2 * sqrt(C_ps*C_p) * sin(RADIANS(delta_h) / 2);

	double T = 1 - 0.17 * cos(RADIANS(meanh_p-30))
	    + 0.24 * cos(RADIANS(2*meanh_p))
	    + 0.32 * cos(RADIANS(3*meanh_p + 6))
	    - 0.2  * cos(RADIANS(4*meanh_p - 63));

	double Sl = 1 + (0.015 * Sqr((Ls + L1) /2- 50) )/ sqrt(20 + Sqr( (Ls+L1)/2 - 50));

	double Sc = 1 + 0.045 * (C_p + C_ps)/2;
	double Sh = 1 + 0.015 * ((C_ps + C_p)/2) * T;

	double delta_ro = 30 * exp(-Sqr(((meanh_p - 275 ) / 25)));

	double Rc = 2 * sqrt(( pow(meanC_p, 7.0) )/( pow(meanC_p, 7.0) + pow(25.0, 7.0)));

	double Rt = -sin(2 * RADIANS(delta_ro)) * Rc;

	double deltaE00 = sqrt(Sqr(delta_L /(Sl * Kl)) +
		Sqr(delta_C/(Sc * Kc))  +
		Sqr(delta_H/(Sh * Kh))  +
		Rt*(delta_C/(Sc * Kc)) * (delta_H / (Sh * Kh)));

	return deltaE00;
}

// This function returns a number of gridpoints to be used as LUT table. It assumes same number
// of gripdpoints in all dimensions. Flags may override the choice.
cmsUInt32Number _cmsReasonableGridpointsByColorspace(cmsColorSpaceSignature Colorspace, cmsUInt32Number dwFlags)
{
	cmsUInt32Number nChannels;

	// Already specified?
	if(dwFlags & 0x00FF0000) {
		// Yes, grab'em
		return (dwFlags >> 16) & 0xFF;
	}

	nChannels = cmsChannelsOf(Colorspace);

	// HighResPrecalc is maximum resolution
	if(dwFlags & cmsFLAGS_HIGHRESPRECALC) {
		if(nChannels > 4)
			return 7; // 7 for Hifi

		if(nChannels == 4) // 23 for CMYK
			return 23;

		return 49; // 49 for RGB and others
	}

	// LowResPrecal is lower resolution
	if(dwFlags & cmsFLAGS_LOWRESPRECALC) {
		if(nChannels > 4)
			return 6; // 6 for more than 4 channels

		if(nChannels == 1)
			return 33; // For monochrome

		return 17;      // 17 for remaining
	}

	// Default values
	if(nChannels > 4)
		return 7;       // 7 for Hifi

	if(nChannels == 4)
		return 17;      // 17 for CMYK

	return 33;              // 33 for RGB
}

boolint _cmsEndPointsBySpace(cmsColorSpaceSignature Space, uint16 ** White, uint16 ** Black, cmsUInt32Number * nOutputs)
{
	// Only most common spaces
	static uint16 RGBblack[4]  = { 0, 0, 0 };
	static uint16 RGBwhite[4]  = { 0xffff, 0xffff, 0xffff };
	static uint16 CMYKblack[4] = { 0xffff, 0xffff, 0xffff, 0xffff };  // 400% of ink
	static uint16 CMYKwhite[4] = { 0, 0, 0, 0 };
	static uint16 LABblack[4]  = { 0, 0x8080, 0x8080 };              // V4 Lab encoding
	static uint16 LABwhite[4]  = { 0xFFFF, 0x8080, 0x8080 };
	static uint16 CMYblack[4]  = { 0xffff, 0xffff, 0xffff };
	static uint16 CMYwhite[4]  = { 0, 0, 0 };
	static uint16 Grayblack[4] = { 0 };
	static uint16 GrayWhite[4] = { 0xffff };
	switch(Space) {
		case cmsSigGrayData: if(White) *White = GrayWhite;
		    if(Black) *Black = Grayblack;
		    if(nOutputs) *nOutputs = 1;
		    return TRUE;

		case cmsSigRgbData:  if(White) *White = RGBwhite;
		    if(Black) *Black = RGBblack;
		    if(nOutputs) *nOutputs = 3;
		    return TRUE;

		case cmsSigLabData:  if(White) *White = LABwhite;
		    if(Black) *Black = LABblack;
		    if(nOutputs) *nOutputs = 3;
		    return TRUE;

		case cmsSigCmykData: if(White) *White = CMYKwhite;
		    if(Black) *Black = CMYKblack;
		    if(nOutputs) *nOutputs = 4;
		    return TRUE;

		case cmsSigCmyData:  if(White) *White = CMYwhite;
		    if(Black) *Black = CMYblack;
		    if(nOutputs) *nOutputs = 3;
		    return TRUE;

		default:;
	}

	return FALSE;
}

// Several utilities -------------------------------------------------------

// Translate from our colorspace to ICC representation

cmsColorSpaceSignature CMSEXPORT _cmsICCcolorSpace(int OurNotation)
{
	switch(OurNotation) {
		case 1:
		case PT_GRAY: return cmsSigGrayData;

		case 2:
		case PT_RGB:  return cmsSigRgbData;

		case PT_CMY:  return cmsSigCmyData;
		case PT_CMYK: return cmsSigCmykData;
		case PT_YCbCr: return cmsSigYCbCrData;
		case PT_YUV:  return cmsSigLuvData;
		case PT_XYZ:  return cmsSigXYZData;

		case PT_LabV2:
		case PT_Lab:  return cmsSigLabData;

		case PT_YUVK: return cmsSigLuvKData;
		case PT_HSV:  return cmsSigHsvData;
		case PT_HLS:  return cmsSigHlsData;
		case PT_Yxy:  return cmsSigYxyData;

		case PT_MCH1: return cmsSigMCH1Data;
		case PT_MCH2: return cmsSigMCH2Data;
		case PT_MCH3: return cmsSigMCH3Data;
		case PT_MCH4: return cmsSigMCH4Data;
		case PT_MCH5: return cmsSigMCH5Data;
		case PT_MCH6: return cmsSigMCH6Data;
		case PT_MCH7: return cmsSigMCH7Data;
		case PT_MCH8: return cmsSigMCH8Data;

		case PT_MCH9:  return cmsSigMCH9Data;
		case PT_MCH10: return cmsSigMCHAData;
		case PT_MCH11: return cmsSigMCHBData;
		case PT_MCH12: return cmsSigMCHCData;
		case PT_MCH13: return cmsSigMCHDData;
		case PT_MCH14: return cmsSigMCHEData;
		case PT_MCH15: return cmsSigMCHFData;

		default:  return (cmsColorSpaceSignature)0;
	}
}

int CMSEXPORT _cmsLCMScolorSpace(cmsColorSpaceSignature ProfileSpace)
{
	switch(ProfileSpace) {
		case cmsSigGrayData: return PT_GRAY;
		case cmsSigRgbData:  return PT_RGB;
		case cmsSigCmyData:  return PT_CMY;
		case cmsSigCmykData: return PT_CMYK;
		case cmsSigYCbCrData: return PT_YCbCr;
		case cmsSigLuvData:  return PT_YUV;
		case cmsSigXYZData:  return PT_XYZ;
		case cmsSigLabData:  return PT_Lab;
		case cmsSigLuvKData: return PT_YUVK;
		case cmsSigHsvData:  return PT_HSV;
		case cmsSigHlsData:  return PT_HLS;
		case cmsSigYxyData:  return PT_Yxy;
		case cmsSig1colorData:
		case cmsSigMCH1Data: return PT_MCH1;
		case cmsSig2colorData:
		case cmsSigMCH2Data: return PT_MCH2;
		case cmsSig3colorData:
		case cmsSigMCH3Data: return PT_MCH3;
		case cmsSig4colorData:
		case cmsSigMCH4Data: return PT_MCH4;
		case cmsSig5colorData:
		case cmsSigMCH5Data: return PT_MCH5;
		case cmsSig6colorData:
		case cmsSigMCH6Data: return PT_MCH6;
		case cmsSigMCH7Data:
		case cmsSig7colorData: return PT_MCH7;
		case cmsSigMCH8Data:
		case cmsSig8colorData: return PT_MCH8;
		case cmsSigMCH9Data:
		case cmsSig9colorData: return PT_MCH9;
		case cmsSigMCHAData:
		case cmsSig10colorData: return PT_MCH10;
		case cmsSigMCHBData:
		case cmsSig11colorData: return PT_MCH11;
		case cmsSigMCHCData:
		case cmsSig12colorData: return PT_MCH12;
		case cmsSigMCHDData:
		case cmsSig13colorData: return PT_MCH13;
		case cmsSigMCHEData:
		case cmsSig14colorData: return PT_MCH14;
		case cmsSigMCHFData:
		case cmsSig15colorData: return PT_MCH15;
		default:  return (cmsColorSpaceSignature)0;
	}
}

cmsUInt32Number CMSEXPORT cmsChannelsOf(cmsColorSpaceSignature ColorSpace)
{
	switch(ColorSpace) {
		case cmsSigMCH1Data:
		case cmsSig1colorData:
		case cmsSigGrayData: return 1;
		case cmsSigMCH2Data:
		case cmsSig2colorData:  return 2;
		case cmsSigXYZData:
		case cmsSigLabData:
		case cmsSigLuvData:
		case cmsSigYCbCrData:
		case cmsSigYxyData:
		case cmsSigRgbData:
		case cmsSigHsvData:
		case cmsSigHlsData:
		case cmsSigCmyData:
		case cmsSigMCH3Data:
		case cmsSig3colorData:  return 3;
		case cmsSigLuvKData:
		case cmsSigCmykData:
		case cmsSigMCH4Data:
		case cmsSig4colorData:  return 4;
		case cmsSigMCH5Data:
		case cmsSig5colorData:  return 5;
		case cmsSigMCH6Data:
		case cmsSig6colorData:  return 6;
		case cmsSigMCH7Data:
		case cmsSig7colorData:  return 7;
		case cmsSigMCH8Data:
		case cmsSig8colorData:  return 8;
		case cmsSigMCH9Data:
		case cmsSig9colorData:  return 9;
		case cmsSigMCHAData:
		case cmsSig10colorData: return 10;
		case cmsSigMCHBData:
		case cmsSig11colorData: return 11;
		case cmsSigMCHCData:
		case cmsSig12colorData: return 12;
		case cmsSigMCHDData:
		case cmsSig13colorData: return 13;
		case cmsSigMCHEData:
		case cmsSig14colorData: return 14;
		case cmsSigMCHFData:
		case cmsSig15colorData: return 15;
		default: return 3;
	}
}
