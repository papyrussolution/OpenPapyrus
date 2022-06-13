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

// This module handles all formats supported by lcms. There are two flavors, 16 bits and
// floating point. Floating point is supported only in a subset, those formats holding
// float (4 bytes per component) and double (marked as 0 bytes per component
// as special case)

// ---------------------------------------------------------------------------

// This macro return words stored as big endian
#define CHANGE_ENDIAN(w)    (uint16)((uint16)((w)<<8)|((w)>>8))

// These macros handles reversing (negative)
#define REVERSE_FLAVOR_8(x)     ((uint8)(0xff-(x)))
#define REVERSE_FLAVOR_16(x)    ((uint16)(0xffff-(x)))

// * 0xffff / 0xff00 = (255 * 257) / (255 * 256) = 257 / 256
cmsINLINE uint16 FomLabV2ToLabV4(uint16 x)
{
	int a = (x << 8 | x) >> 8; // * 257 / 256
	if(a > 0xffff) return 0xffff;
	return (uint16)a;
}

// * 0xf00 / 0xffff = * 256 / 257
cmsINLINE uint16 FomLabV4ToLabV2(uint16 x)
{
	return (uint16)(((x << 8) + 0x80) / 257);
}

typedef struct {
	uint32 Type;
	uint32 Mask;
	cmsFormatter16 Frm;
} cmsFormatters16;

typedef struct {
	uint32 Type;
	uint32 Mask;
	cmsFormatterFloat Frm;
} cmsFormattersFloat;

#define ANYSPACE        COLORSPACE_SH(31)
#define ANYCHANNELS     CHANNELS_SH(15)
#define ANYEXTRA        EXTRA_SH(7)
#define ANYPLANAR       PLANAR_SH(1)
#define ANYENDIAN       ENDIAN16_SH(1)
#define ANYSWAP         DOSWAP_SH(1)
#define ANYSWAPFIRST    SWAPFIRST_SH(1)
#define ANYFLAVOR       FLAVOR_SH(1)

// Suppress waning about info never being used

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif

// Unpacking routines (16 bits) ----------------------------------------------------------------------------------------

// Does almost everything but is slow
static uint8 * UnrollChunkyBytes(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->InputFormat);
	uint32 DoSwap     = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra      = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint16 v;
	uint32 i;

	if(ExtraFirst) {
		accum += Extra;
	}
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		v = FROM_8_TO_16(*accum);
		v = Reverse ? REVERSE_FLAVOR_16(v) : v;
		wIn[index] = v;
		accum++;
	}
	if(!ExtraFirst) {
		accum += Extra;
	}
	if(Extra == 0 && SwapFirst) {
		uint16 tmp = wIn[0];
		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(uint16));
		wIn[nChan-1] = tmp;
	}
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// Extra channels are just ignored because come in the next planes
static uint8 * UnrollPlanarBytes(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan     = T_CHANNELS(info->InputFormat);
	uint32 DoSwap    = T_DOSWAP(info->InputFormat);
	uint32 SwapFirst = T_SWAPFIRST(info->InputFormat);
	uint32 Reverse   = T_FLAVOR(info->InputFormat);
	uint32 i;
	uint8 * Init = accum;
	if(DoSwap ^ SwapFirst) {
		accum += T_EXTRA(info->InputFormat) * Stride;
	}
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		uint16 v = FROM_8_TO_16(*accum);

		wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;
		accum += Stride;
	}

	return (Init + 1);
}

// Special cases, provided for performance
static uint8 * Unroll4Bytes(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FROM_8_TO_16(*accum); accum++; // C
	wIn[1] = FROM_8_TO_16(*accum); accum++; // M
	wIn[2] = FROM_8_TO_16(*accum); accum++; // Y
	wIn[3] = FROM_8_TO_16(*accum); accum++; // K
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4BytesReverse(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // C
	wIn[1] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // M
	wIn[2] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // Y
	wIn[3] = FROM_8_TO_16(REVERSE_FLAVOR_8(*accum)); accum++; // K
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4BytesSwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[3] = FROM_8_TO_16(*accum); accum++; // K
	wIn[0] = FROM_8_TO_16(*accum); accum++; // C
	wIn[1] = FROM_8_TO_16(*accum); accum++; // M
	wIn[2] = FROM_8_TO_16(*accum); accum++; // Y
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// KYMC
static uint8 * Unroll4BytesSwap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[3] = FROM_8_TO_16(*accum); accum++; // K
	wIn[2] = FROM_8_TO_16(*accum); accum++; // Y
	wIn[1] = FROM_8_TO_16(*accum); accum++; // M
	wIn[0] = FROM_8_TO_16(*accum); accum++; // C
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4BytesSwapSwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[2] = FROM_8_TO_16(*accum); accum++; // K
	wIn[1] = FROM_8_TO_16(*accum); accum++; // Y
	wIn[0] = FROM_8_TO_16(*accum); accum++; // M
	wIn[3] = FROM_8_TO_16(*accum); accum++; // C

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3Bytes(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FROM_8_TO_16(*accum); accum++; // R
	wIn[1] = FROM_8_TO_16(*accum); accum++; // G
	wIn[2] = FROM_8_TO_16(*accum); accum++; // B

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3BytesSkip1Swap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	accum++; // A
	wIn[2] = FROM_8_TO_16(*accum); accum++; // B
	wIn[1] = FROM_8_TO_16(*accum); accum++; // G
	wIn[0] = FROM_8_TO_16(*accum); accum++; // R

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3BytesSkip1SwapSwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[2] = FROM_8_TO_16(*accum); accum++; // B
	wIn[1] = FROM_8_TO_16(*accum); accum++; // G
	wIn[0] = FROM_8_TO_16(*accum); accum++; // R
	accum++; // A

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3BytesSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	accum++; // A
	wIn[0] = FROM_8_TO_16(*accum); accum++; // R
	wIn[1] = FROM_8_TO_16(*accum); accum++; // G
	wIn[2] = FROM_8_TO_16(*accum); accum++; // B

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// BRG
static uint8 * Unroll3BytesSwap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[2] = FROM_8_TO_16(*accum); accum++; // B
	wIn[1] = FROM_8_TO_16(*accum); accum++; // G
	wIn[0] = FROM_8_TO_16(*accum); accum++; // R

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * UnrollLabV2_8(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // L
	wIn[1] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // a
	wIn[2] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // b

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * UnrollALabV2_8(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	accum++; // A
	wIn[0] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // L
	wIn[1] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // a
	wIn[2] = FomLabV2ToLabV4(FROM_8_TO_16(*accum)); accum++; // b

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * UnrollLabV2_16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FomLabV2ToLabV4(*(uint16*)accum); accum += 2;  // L
	wIn[1] = FomLabV2ToLabV4(*(uint16*)accum); accum += 2;  // a
	wIn[2] = FomLabV2ToLabV4(*(uint16*)accum); accum += 2;  // b

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// for duplex
static uint8 * Unroll2Bytes(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = FROM_8_TO_16(*accum); accum++; // ch1
	wIn[1] = FROM_8_TO_16(*accum); accum++; // ch2

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// Monochrome duplicates L into RGB for null-transforms
static uint8 * Unroll1Byte(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++; // L
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1ByteSkip1(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++; // L
	accum += 1;
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1ByteSkip2(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = FROM_8_TO_16(*accum); accum++; // L
	accum += 2;
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1ByteReversed(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = REVERSE_FLAVOR_16(FROM_8_TO_16(*accum)); accum++; // L
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * UnrollAnyWords(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan       = T_CHANNELS(info->InputFormat);
	uint32 SwapEndian  = T_ENDIAN16(info->InputFormat);
	uint32 DoSwap      = T_DOSWAP(info->InputFormat);
	uint32 Reverse     = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst   = T_SWAPFIRST(info->InputFormat);
	uint32 Extra       = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst  = DoSwap ^ SwapFirst;
	uint32 i;

	if(ExtraFirst) {
		accum += Extra * sizeof(uint16);
	}

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		uint16 v = *(uint16*)accum;
		if(SwapEndian)
			v = CHANGE_ENDIAN(v);
		wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;
		accum += sizeof(uint16);
	}
	if(!ExtraFirst) {
		accum += Extra * sizeof(uint16);
	}
	if(Extra == 0 && SwapFirst) {
		uint16 tmp = wIn[0];
		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(uint16));
		wIn[nChan-1] = tmp;
	}
	return accum;
	CXX_UNUSED(Stride);
}

static uint8 * UnrollPlanarWords(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan = T_CHANNELS(info->InputFormat);
	uint32 DoSwap = T_DOSWAP(info->InputFormat);
	uint32 Reverse = T_FLAVOR(info->InputFormat);
	uint32 SwapEndian = T_ENDIAN16(info->InputFormat);
	uint32 i;
	uint8 * Init = accum;

	if(DoSwap) {
		accum += T_EXTRA(info->InputFormat) * Stride;
	}
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		uint16 v = *(uint16*)accum;
		if(SwapEndian)
			v = CHANGE_ENDIAN(v);
		wIn[index] = Reverse ? REVERSE_FLAVOR_16(v) : v;
		accum +=  Stride;
	}
	return (Init + sizeof(uint16));
}

static uint8 * Unroll4Words(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = *(uint16*)accum; accum += 2; // C
	wIn[1] = *(uint16*)accum; accum += 2; // M
	wIn[2] = *(uint16*)accum; accum += 2; // Y
	wIn[3] = *(uint16*)accum; accum += 2; // K
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4WordsReverse(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = REVERSE_FLAVOR_16(*(uint16*)accum); accum += 2; // C
	wIn[1] = REVERSE_FLAVOR_16(*(uint16*)accum); accum += 2; // M
	wIn[2] = REVERSE_FLAVOR_16(*(uint16*)accum); accum += 2; // Y
	wIn[3] = REVERSE_FLAVOR_16(*(uint16*)accum); accum += 2; // K
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4WordsSwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[3] = *(uint16*)accum; accum += 2; // K
	wIn[0] = *(uint16*)accum; accum += 2; // C
	wIn[1] = *(uint16*)accum; accum += 2; // M
	wIn[2] = *(uint16*)accum; accum += 2; // Y
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// KYMC
static uint8 * Unroll4WordsSwap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[3] = *(uint16*)accum; accum += 2; // K
	wIn[2] = *(uint16*)accum; accum += 2; // Y
	wIn[1] = *(uint16*)accum; accum += 2; // M
	wIn[0] = *(uint16*)accum; accum += 2; // C
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll4WordsSwapSwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[2] = *(uint16*)accum; accum += 2; // K
	wIn[1] = *(uint16*)accum; accum += 2; // Y
	wIn[0] = *(uint16*)accum; accum += 2; // M
	wIn[3] = *(uint16*)accum; accum += 2; // C
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3Words(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = *(uint16*)accum; accum += 2; // C R
	wIn[1] = *(uint16*)accum; accum += 2; // M G
	wIn[2] = *(uint16*)accum; accum += 2; // Y B

	return accum;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3WordsSwap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[2] = *(uint16*)accum; accum += 2; // C R
	wIn[1] = *(uint16*)accum; accum += 2; // M G
	wIn[0] = *(uint16*)accum; accum += 2; // Y B
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3WordsSkip1Swap(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	accum += 2; // A
	wIn[2] = *(uint16*)accum; accum += 2; // R
	wIn[1] = *(uint16*)accum; accum += 2; // G
	wIn[0] = *(uint16*)accum; accum += 2; // B
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll3WordsSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	accum += 2; // A
	wIn[0] = *(uint16*)accum; accum += 2; // R
	wIn[1] = *(uint16*)accum; accum += 2; // G
	wIn[2] = *(uint16*)accum; accum += 2; // B
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1Word(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = *(uint16*)accum; accum += 2; // L
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1WordReversed(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = REVERSE_FLAVOR_16(*(uint16*)accum); accum += 2;
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll1WordSkip3(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = wIn[1] = wIn[2] = *(uint16*)accum;
	accum += 8;
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Unroll2Words(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	wIn[0] = *(uint16*)accum; accum += 2; // ch1
	wIn[1] = *(uint16*)accum; accum += 2; // ch2
	return accum;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// This is a conversion of Lab double to 16 bits
static uint8 * UnrollLabDoubleTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	if(T_PLANAR(info->InputFormat)) {
		cmsCIELab Lab;
		uint8 * pos_L;
		uint8 * pos_a;
		uint8 * pos_b;
		pos_L = accum;
		pos_a = accum + Stride;
		pos_b = accum + Stride * 2;
		Lab.L = *(double *)pos_L;
		Lab.a = *(double *)pos_a;
		Lab.b = *(double *)pos_b;
		cmsFloat2LabEncoded(wIn, &Lab);
		return accum + sizeof(double);
	}
	else {
		cmsFloat2LabEncoded(wIn, (cmsCIELab*)accum);
		accum += sizeof(cmsCIELab) + T_EXTRA(info->InputFormat) * sizeof(double);
		return accum;
	}
}

// This is a conversion of Lab float to 16 bits
static uint8 * UnrollLabFloatTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	cmsCIELab Lab;
	if(T_PLANAR(info->InputFormat)) {
		uint8 * pos_L;
		uint8 * pos_a;
		uint8 * pos_b;
		pos_L = accum;
		pos_a = accum + Stride;
		pos_b = accum + Stride * 2;
		Lab.L = *(float *)pos_L;
		Lab.a = *(float *)pos_a;
		Lab.b = *(float *)pos_b;
		cmsFloat2LabEncoded(wIn, &Lab);
		return accum + sizeof(float);
	}
	else {
		Lab.L = ((float *)accum)[0];
		Lab.a = ((float *)accum)[1];
		Lab.b = ((float *)accum)[2];

		cmsFloat2LabEncoded(wIn, &Lab);
		accum += (3 + T_EXTRA(info->InputFormat)) * sizeof(float);
		return accum;
	}
}

// This is a conversion of XYZ double to 16 bits
static uint8 * UnrollXYZDoubleTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	if(T_PLANAR(info->InputFormat)) {
		cmsCIEXYZ XYZ;
		uint8 * pos_X;
		uint8 * pos_Y;
		uint8 * pos_Z;
		pos_X = accum;
		pos_Y = accum + Stride;
		pos_Z = accum + Stride * 2;
		XYZ.X = *(double *)pos_X;
		XYZ.Y = *(double *)pos_Y;
		XYZ.Z = *(double *)pos_Z;
		cmsFloat2XYZEncoded(wIn, &XYZ);
		return accum + sizeof(double);
	}
	else {
		cmsFloat2XYZEncoded(wIn, (cmsCIEXYZ*)accum);
		accum += sizeof(cmsCIEXYZ) + T_EXTRA(info->InputFormat) * sizeof(double);
		return accum;
	}
}

// This is a conversion of XYZ float to 16 bits
static uint8 * UnrollXYZFloatTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	if(T_PLANAR(info->InputFormat)) {
		cmsCIEXYZ XYZ;
		uint8 * pos_X;
		uint8 * pos_Y;
		uint8 * pos_Z;
		pos_X = accum;
		pos_Y = accum + Stride;
		pos_Z = accum + Stride * 2;
		XYZ.X = *(float *)pos_X;
		XYZ.Y = *(float *)pos_Y;
		XYZ.Z = *(float *)pos_Z;
		cmsFloat2XYZEncoded(wIn, &XYZ);
		return accum + sizeof(float);
	}
	else {
		float* Pt = (float *)accum;
		cmsCIEXYZ XYZ;
		XYZ.X = Pt[0];
		XYZ.Y = Pt[1];
		XYZ.Z = Pt[2];
		cmsFloat2XYZEncoded(wIn, &XYZ);
		accum += 3 * sizeof(float) + T_EXTRA(info->InputFormat) * sizeof(float);
		return accum;
	}
}

// Check if space is marked as ink
cmsINLINE boolint IsInkSpace(uint32 Type)
{
	switch(T_COLORSPACE(Type)) {
		case PT_CMY:
		case PT_CMYK:
		case PT_MCH5:
		case PT_MCH6:
		case PT_MCH7:
		case PT_MCH8:
		case PT_MCH9:
		case PT_MCH10:
		case PT_MCH11:
		case PT_MCH12:
		case PT_MCH13:
		case PT_MCH14:
		case PT_MCH15: return TRUE;

		default: return FALSE;
	}
}

// Return the size in bytes of a given formatter
static uint32 PixelSize(uint32 Format)
{
	uint32 fmt_bytes = T_BYTES(Format);
	// For double, the T_BYTES field is zero
	if(fmt_bytes == 0)
		return sizeof(uint64);
	// Otherwise, it is already correct for all formats
	return fmt_bytes;
}

// Inks does come in percentage, remaining cases are between 0..1.0, again to 16 bits
static uint8 * UnrollDoubleTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->InputFormat);
	uint32 DoSwap     = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra      = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	double v;
	uint16 vi;
	uint32 i, start = 0;
	double maximum = IsInkSpace(info->InputFormat) ? 655.35 : 65535.0;

	Stride /= PixelSize(info->InputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		if(Planar)
			v = (float)((double *)accum)[(i + start) * Stride];
		else
			v = (float)((double *)accum)[i + start];

		vi = _cmsQuickSaturateWord(v * maximum);

		if(Reverse)
			vi = REVERSE_FLAVOR_16(vi);
		wIn[index] = vi;
	}
	if(Extra == 0 && SwapFirst) {
		uint16 tmp = wIn[0];
		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(uint16));
		wIn[nChan-1] = tmp;
	}
	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(double);
	else
		return accum + (nChan + Extra) * sizeof(double);
}

static uint8 * UnrollFloatTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan  = T_CHANNELS(info->InputFormat);
	uint32 DoSwap   = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra   = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	float v;
	uint16 vi;
	uint32 i, start = 0;
	double maximum = IsInkSpace(info->InputFormat) ? 655.35 : 65535.0;

	Stride /= PixelSize(info->InputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		if(Planar)
			v = (float)((float *)accum)[(i + start) * Stride];
		else
			v = (float)((float *)accum)[i + start];

		vi = _cmsQuickSaturateWord(v * maximum);

		if(Reverse)
			vi = REVERSE_FLAVOR_16(vi);

		wIn[index] = vi;
	}

	if(Extra == 0 && SwapFirst) {
		uint16 tmp = wIn[0];

		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(uint16));
		wIn[nChan-1] = tmp;
	}

	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(float);
	else
		return accum + (nChan + Extra) * sizeof(float);
}

// For 1 channel, we need to duplicate data (it comes in 0..1.0 range)
static uint8 * UnrollDouble1Chan(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	double * Inks = (double *)accum;
	wIn[0] = wIn[1] = wIn[2] = _cmsQuickSaturateWord(Inks[0] * 65535.0);
	return accum + sizeof(double);
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// For anything going from float
static uint8 * UnrollFloatsToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan  = T_CHANNELS(info->InputFormat);
	uint32 DoSwap   = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra   = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	float v;
	uint32 i, start = 0;
	float maximum = IsInkSpace(info->InputFormat) ? 100.0F : 1.0F;

	Stride /= PixelSize(info->InputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		if(Planar)
			v = (float)((float *)accum)[(i + start) * Stride];
		else
			v = (float)((float *)accum)[i + start];

		v /= maximum;

		wIn[index] = Reverse ? 1 - v : v;
	}

	if(Extra == 0 && SwapFirst) {
		float tmp = wIn[0];

		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(float));
		wIn[nChan-1] = tmp;
	}

	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(float);
	else
		return accum + (nChan + Extra) * sizeof(float);
}

// For anything going from double

static uint8 * UnrollDoublesToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan  = T_CHANNELS(info->InputFormat);
	uint32 DoSwap   = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra   = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	double v;
	uint32 i, start = 0;
	double maximum = IsInkSpace(info->InputFormat) ? 100.0 : 1.0;

	Stride /= PixelSize(info->InputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		if(Planar)
			v = (double)((double *)accum)[(i + start)  * Stride];
		else
			v = (double)((double *)accum)[i + start];

		v /= maximum;

		wIn[index] = (float)(Reverse ? 1.0 - v : v);
	}

	if(Extra == 0 && SwapFirst) {
		float tmp = wIn[0];

		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(float));
		wIn[nChan-1] = tmp;
	}

	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(double);
	else
		return accum + (nChan + Extra) * sizeof(double);
}

// From Lab double to float
static uint8 * UnrollLabDoubleToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	double * Pt = (double *)accum;
	if(T_PLANAR(info->InputFormat)) {
		Stride /= PixelSize(info->InputFormat);
		wIn[0] = (float)(Pt[0] / 100.0);          // from 0..100 to 0..1
		wIn[1] = (float)((Pt[Stride] + 128) / 255.0); // form -128..+127 to 0..1
		wIn[2] = (float)((Pt[Stride*2] + 128) / 255.0);
		return accum + sizeof(double);
	}
	else {
		wIn[0] = (float)(Pt[0] / 100.0);     // from 0..100 to 0..1
		wIn[1] = (float)((Pt[1] + 128) / 255.0); // form -128..+127 to 0..1
		wIn[2] = (float)((Pt[2] + 128) / 255.0);
		accum += sizeof(double)*(3 + T_EXTRA(info->InputFormat));
		return accum;
	}
}

// From Lab double to float
static uint8 * UnrollLabFloatToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	float* Pt = (float *)accum;
	if(T_PLANAR(info->InputFormat)) {
		Stride /= PixelSize(info->InputFormat);

		wIn[0] = (float)(Pt[0] / 100.0);          // from 0..100 to 0..1
		wIn[1] = (float)((Pt[Stride] + 128) / 255.0); // form -128..+127 to 0..1
		wIn[2] = (float)((Pt[Stride*2] + 128) / 255.0);

		return accum + sizeof(float);
	}
	else {
		wIn[0] = (float)(Pt[0] / 100.0);     // from 0..100 to 0..1
		wIn[1] = (float)((Pt[1] + 128) / 255.0); // form -128..+127 to 0..1
		wIn[2] = (float)((Pt[2] + 128) / 255.0);

		accum += sizeof(float)*(3 + T_EXTRA(info->InputFormat));
		return accum;
	}
}

// 1.15 fixed point, that means maximum value is MAX_ENCODEABLE_XYZ (0xFFFF)
static uint8 * UnrollXYZDoubleToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	double * Pt = (double *)accum;
	if(T_PLANAR(info->InputFormat)) {
		Stride /= PixelSize(info->InputFormat);

		wIn[0] = (float)(Pt[0] / MAX_ENCODEABLE_XYZ);
		wIn[1] = (float)(Pt[Stride] / MAX_ENCODEABLE_XYZ);
		wIn[2] = (float)(Pt[Stride*2] / MAX_ENCODEABLE_XYZ);

		return accum + sizeof(double);
	}
	else {
		wIn[0] = (float)(Pt[0] / MAX_ENCODEABLE_XYZ);
		wIn[1] = (float)(Pt[1] / MAX_ENCODEABLE_XYZ);
		wIn[2] = (float)(Pt[2] / MAX_ENCODEABLE_XYZ);

		accum += sizeof(double)*(3 + T_EXTRA(info->InputFormat));
		return accum;
	}
}

static uint8 * UnrollXYZFloatToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	float* Pt = (float *)accum;
	if(T_PLANAR(info->InputFormat)) {
		Stride /= PixelSize(info->InputFormat);
		wIn[0] = (float)(Pt[0] / MAX_ENCODEABLE_XYZ);
		wIn[1] = (float)(Pt[Stride] / MAX_ENCODEABLE_XYZ);
		wIn[2] = (float)(Pt[Stride*2] / MAX_ENCODEABLE_XYZ);

		return accum + sizeof(float);
	}
	else {
		wIn[0] = (float)(Pt[0] / MAX_ENCODEABLE_XYZ);
		wIn[1] = (float)(Pt[1] / MAX_ENCODEABLE_XYZ);
		wIn[2] = (float)(Pt[2] / MAX_ENCODEABLE_XYZ);

		accum += sizeof(float)*(3 + T_EXTRA(info->InputFormat));
		return accum;
	}
}

// Packing routines
// -----------------------------------------------------------------------------------------------------------

// Generic chunky for byte

static uint8 * PackAnyBytes(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan  = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap   = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra   = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint8 * swap1;
	uint8 v = 0;
	uint32 i;

	swap1 = output;

	if(ExtraFirst) {
		output += Extra;
	}

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = FROM_16_TO_8(wOut[index]);

		if(Reverse)
			v = REVERSE_FLAVOR_8(v);

		*output++ = v;
	}

	if(!ExtraFirst) {
		output += Extra;
	}

	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, nChan-1);
		*swap1 = v;
	}

	return output;

	CXX_UNUSED(Stride);
}

static uint8 * PackAnyWords(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan  = T_CHANNELS(info->OutputFormat);
	uint32 SwapEndian = T_ENDIAN16(info->OutputFormat);
	uint32 DoSwap   = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra   = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint16* swap1;
	uint16 v = 0;
	uint32 i;

	swap1 = (uint16*)output;

	if(ExtraFirst) {
		output += Extra * sizeof(uint16);
	}

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = wOut[index];

		if(SwapEndian)
			v = CHANGE_ENDIAN(v);

		if(Reverse)
			v = REVERSE_FLAVOR_16(v);

		*(uint16*)output = v;

		output += sizeof(uint16);
	}

	if(!ExtraFirst) {
		output += Extra * sizeof(uint16);
	}

	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan-1)* sizeof(uint16));
		*swap1 = v;
	}

	return output;

	CXX_UNUSED(Stride);
}

static uint8 * PackPlanarBytes(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan     = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap    = T_DOSWAP(info->OutputFormat);
	uint32 SwapFirst = T_SWAPFIRST(info->OutputFormat);
	uint32 Reverse   = T_FLAVOR(info->OutputFormat);
	uint32 i;
	uint8 * Init = output;

	if(DoSwap ^ SwapFirst) {
		output += T_EXTRA(info->OutputFormat) * Stride;
	}

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		uint8 v = FROM_16_TO_8(wOut[index]);

		*(uint8 *)output = (uint8)(Reverse ? REVERSE_FLAVOR_8(v) : v);
		output += Stride;
	}

	return (Init + 1);

	CXX_UNUSED(Stride);
}

static uint8 * PackPlanarWords(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 SwapEndian = T_ENDIAN16(info->OutputFormat);
	uint32 i;
	uint8 * Init = output;
	uint16 v;

	if(DoSwap) {
		output += T_EXTRA(info->OutputFormat) * Stride;
	}

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = wOut[index];

		if(SwapEndian)
			v = CHANGE_ENDIAN(v);

		if(Reverse)
			v =  REVERSE_FLAVOR_16(v);

		*(uint16*)output = v;
		output += Stride;
	}

	return (Init + sizeof(uint16));
}

// CMYKcm (unrolled for speed)

static uint8 * Pack6Bytes(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[3]);
	*output++ = FROM_16_TO_8(wOut[4]);
	*output++ = FROM_16_TO_8(wOut[5]);

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// KCMYcm

static uint8 * Pack6BytesSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[5]);
	*output++ = FROM_16_TO_8(wOut[4]);
	*output++ = FROM_16_TO_8(wOut[3]);
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// CMYKcm
static uint8 * Pack6Words(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[3];
	output += 2;
	*(uint16*)output = wOut[4];
	output += 2;
	*(uint16*)output = wOut[5];
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// KCMYcm
static uint8 * Pack6WordsSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[5];
	output += 2;
	*(uint16*)output = wOut[4];
	output += 2;
	*(uint16*)output = wOut[3];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4Bytes(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[3]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4BytesReverse(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[0]));
	*output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[1]));
	*output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[2]));
	*output++ = REVERSE_FLAVOR_8(FROM_16_TO_8(wOut[3]));

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4BytesSwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[3]);
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// ABGR
static uint8 * Pack4BytesSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[3]);
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4BytesSwapSwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[3]);

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4Words(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[3];
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack4WordsReverse(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = REVERSE_FLAVOR_16(wOut[0]);
	output += 2;
	*(uint16*)output = REVERSE_FLAVOR_16(wOut[1]);
	output += 2;
	*(uint16*)output = REVERSE_FLAVOR_16(wOut[2]);
	output += 2;
	*(uint16*)output = REVERSE_FLAVOR_16(wOut[3]);
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// ABGR
static uint8 * Pack4WordsSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[3];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// CMYK
static uint8 * Pack4WordsBigEndian(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = CHANGE_ENDIAN(wOut[0]);
	output += 2;
	*(uint16*)output = CHANGE_ENDIAN(wOut[1]);
	output += 2;
	*(uint16*)output = CHANGE_ENDIAN(wOut[2]);
	output += 2;
	*(uint16*)output = CHANGE_ENDIAN(wOut[3]);
	output += 2;

	return output;

	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * PackLabV2_8(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[0]));
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[1]));
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[2]));
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * PackALabV2_8(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[0]));
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[1]));
	*output++ = FROM_16_TO_8(FomLabV4ToLabV2(wOut[2]));
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * PackLabV2_16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = FomLabV4ToLabV2(wOut[0]);
	output += 2;
	*(uint16*)output = FomLabV4ToLabV2(wOut[1]);
	output += 2;
	*(uint16*)output = FomLabV4ToLabV2(wOut[2]);
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3Bytes(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesOptimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = (wOut[0] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[2] & 0xFFU);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesSwapOptimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = (wOut[2] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[0] & 0xFFU);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3Words(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsSwap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsBigEndian(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = CHANGE_ENDIAN(wOut[0]);
	output += 2;
	*(uint16*)output = CHANGE_ENDIAN(wOut[1]);
	output += 2;
	*(uint16*)output = CHANGE_ENDIAN(wOut[2]);
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);
	output++;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1Optimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = (wOut[0] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[2] & 0xFFU);
	output++;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = FROM_16_TO_8(wOut[0]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[2]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1SwapFirstOptimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = (wOut[0] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[2] & 0xFFU);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1Swap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1SwapOptimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = (wOut[2] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[0] & 0xFFU);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1SwapSwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[2]);
	*output++ = FROM_16_TO_8(wOut[1]);
	*output++ = FROM_16_TO_8(wOut[0]);
	output++;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3BytesAndSkip1SwapSwapFirstOptimized(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = (wOut[2] & 0xFFU);
	*output++ = (wOut[1] & 0xFFU);
	*output++ = (wOut[0] & 0xFFU);
	output++;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsAndSkip1(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsAndSkip1Swap(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsAndSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[2];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack3WordsAndSkip1SwapSwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[2];
	output += 2;
	*(uint16*)output = wOut[1];
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1Byte(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1ByteReversed(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(REVERSE_FLAVOR_16(wOut[0]));
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1ByteSkip1(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*output++ = FROM_16_TO_8(wOut[0]);
	output++;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1ByteSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output++;
	*output++ = FROM_16_TO_8(wOut[0]);
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1Word(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1WordReversed(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = REVERSE_FLAVOR_16(wOut[0]);
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1WordBigEndian(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = CHANGE_ENDIAN(wOut[0]);
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1WordSkip1(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	*(uint16*)output = wOut[0];
	output += 4;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

static uint8 * Pack1WordSkip1SwapFirst(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	output += 2;
	*(uint16*)output = wOut[0];
	output += 2;
	return output;
	CXX_UNUSED(info);
	CXX_UNUSED(Stride);
}

// Unencoded Float values -- don't try optimize speed
static uint8 * PackLabDoubleFrom16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	if(T_PLANAR(info->OutputFormat)) {
		cmsCIELab Lab;
		double * Out = (double *)output;
		cmsLabEncoded2Float(&Lab, wOut);
		Out[0]        = Lab.L;
		Out[Stride]   = Lab.a;
		Out[Stride*2] = Lab.b;
		return output + sizeof(double);
	}
	else {
		cmsLabEncoded2Float((cmsCIELab*)output, wOut);
		return output + (sizeof(cmsCIELab) + T_EXTRA(info->OutputFormat) * sizeof(double));
	}
}

static uint8 * PackLabFloatFrom16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	cmsCIELab Lab;
	cmsLabEncoded2Float(&Lab, wOut);
	if(T_PLANAR(info->OutputFormat)) {
		float* Out = (float *)output;
		Stride /= PixelSize(info->OutputFormat);
		Out[0]        = (float)Lab.L;
		Out[Stride]   = (float)Lab.a;
		Out[Stride*2] = (float)Lab.b;
		return output + sizeof(float);
	}
	else {
		((float *)output)[0] = (float)Lab.L;
		((float *)output)[1] = (float)Lab.a;
		((float *)output)[2] = (float)Lab.b;
		return output + (3 + T_EXTRA(info->OutputFormat)) * sizeof(float);
	}
}

static uint8 * PackXYZDoubleFrom16(_cmsTRANSFORM* Info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	if(T_PLANAR(Info->OutputFormat)) {
		cmsCIEXYZ XYZ;
		double * Out = (double *)output;
		cmsXYZEncoded2Float(&XYZ, wOut);
		Stride /= PixelSize(Info->OutputFormat);
		Out[0]        = XYZ.X;
		Out[Stride]   = XYZ.Y;
		Out[Stride*2] = XYZ.Z;
		return output + sizeof(double);
	}
	else {
		cmsXYZEncoded2Float((cmsCIEXYZ*)output, wOut);
		return output + (sizeof(cmsCIEXYZ) + T_EXTRA(Info->OutputFormat) * sizeof(double));
	}
}

static uint8 * PackXYZFloatFrom16(_cmsTRANSFORM* Info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	if(T_PLANAR(Info->OutputFormat)) {
		cmsCIEXYZ XYZ;
		float* Out = (float *)output;
		cmsXYZEncoded2Float(&XYZ, wOut);
		Stride /= PixelSize(Info->OutputFormat);
		Out[0]        = (float)XYZ.X;
		Out[Stride]   = (float)XYZ.Y;
		Out[Stride*2] = (float)XYZ.Z;
		return output + sizeof(float);
	}
	else {
		cmsCIEXYZ XYZ;
		float* Out = (float *)output;
		cmsXYZEncoded2Float(&XYZ, wOut);
		Out[0] = (float)XYZ.X;
		Out[1] = (float)XYZ.Y;
		Out[2] = (float)XYZ.Z;

		return output + (3 * sizeof(float) + T_EXTRA(Info->OutputFormat) * sizeof(float));
	}
}

static uint8 * PackDoubleFrom16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra      = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar     = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	double maximum = IsInkSpace(info->OutputFormat) ? 655.35 : 65535.0;
	double v = 0;
	double * swap1 = (double *)output;
	uint32 i, start = 0;

	Stride /= PixelSize(info->OutputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = (double)wOut[index] / maximum;

		if(Reverse)
			v = maximum - v;

		if(Planar)
			((double *)output)[(i + start)  * Stride] = v;
		else
			((double *)output)[i + start] = v;
	}

	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan-1)* sizeof(double));
		*swap1 = v;
	}

	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(double);
	else
		return output + (nChan + Extra) * sizeof(double);
}

static uint8 * PackFloatFrom16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra      = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar     = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	double maximum = IsInkSpace(info->OutputFormat) ? 655.35 : 65535.0;
	double v = 0;
	float* swap1 = (float *)output;
	uint32 i, start = 0;
	Stride /= PixelSize(info->OutputFormat);
	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		v = (double)wOut[index] / maximum;
		if(Reverse)
			v = maximum - v;
		if(Planar)
			((float *)output)[(i + start) * Stride] = (float)v;
		else
			((float *)output)[i + start] = (float)v;
	}
	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan - 1)* sizeof(float));
		*swap1 = (float)v;
	}

	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(float);
	else
		return output + (nChan + Extra) * sizeof(float);
}

static uint8 * PackFloatsFromFloat(_cmsTRANSFORM * info, float wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap = T_DOSWAP(info->OutputFormat);
	uint32 Reverse = T_FLAVOR(info->OutputFormat);
	uint32 Extra = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	double maximum = IsInkSpace(info->OutputFormat) ? 100.0 : 1.0;
	float* swap1 = (float *)output;
	double v = 0;
	uint32 i, start = 0;

	Stride /= PixelSize(info->OutputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = wOut[index] * maximum;

		if(Reverse)
			v = maximum - v;

		if(Planar)
			((float *)output)[(i + start)* Stride] = (float)v;
		else
			((float *)output)[i + start] = (float)v;
	}

	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan - 1)* sizeof(float));
		*swap1 = (float)v;
	}

	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(float);
	else
		return output + (nChan + Extra) * sizeof(float);
}

static uint8 * PackDoublesFromFloat(_cmsTRANSFORM * info, float wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra      = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar     = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	double maximum = IsInkSpace(info->OutputFormat) ? 100.0 : 1.0;
	double v = 0;
	double * swap1 = (double *)output;
	uint32 i, start = 0;
	Stride /= PixelSize(info->OutputFormat);
	if(ExtraFirst)
		start = Extra;
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		v = wOut[index] * maximum;

		if(Reverse)
			v = maximum - v;

		if(Planar)
			((double *)output)[(i + start) * Stride] = v;
		else
			((double *)output)[i + start] = v;
	}

	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan - 1)* sizeof(double));
		*swap1 = v;
	}

	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(double);
	else
		return output + (nChan + Extra) * sizeof(double);
}

static uint8 * PackLabFloatFromFloat(_cmsTRANSFORM* Info, float wOut[], uint8 * output, uint32 Stride)
{
	float* Out = (float *)output;
	if(T_PLANAR(Info->OutputFormat)) {
		Stride /= PixelSize(Info->OutputFormat);
		Out[0]        = (float)(wOut[0] * 100.0);
		Out[Stride]   = (float)(wOut[1] * 255.0 - 128.0);
		Out[Stride*2] = (float)(wOut[2] * 255.0 - 128.0);
		return output + sizeof(float);
	}
	else {
		Out[0] = (float)(wOut[0] * 100.0);
		Out[1] = (float)(wOut[1] * 255.0 - 128.0);
		Out[2] = (float)(wOut[2] * 255.0 - 128.0);
		return output + (sizeof(float)*3 + T_EXTRA(Info->OutputFormat) * sizeof(float));
	}
}

static uint8 * PackLabDoubleFromFloat(_cmsTRANSFORM* Info, float wOut[], uint8 * output, uint32 Stride)
{
	double * Out = (double *)output;
	if(T_PLANAR(Info->OutputFormat)) {
		Stride /= PixelSize(Info->OutputFormat);

		Out[0]        = (double)(wOut[0] * 100.0);
		Out[Stride]   = (double)(wOut[1] * 255.0 - 128.0);
		Out[Stride*2] = (double)(wOut[2] * 255.0 - 128.0);

		return output + sizeof(double);
	}
	else {
		Out[0] = (double)(wOut[0] * 100.0);
		Out[1] = (double)(wOut[1] * 255.0 - 128.0);
		Out[2] = (double)(wOut[2] * 255.0 - 128.0);

		return output + (sizeof(double)*3 + T_EXTRA(Info->OutputFormat) * sizeof(double));
	}
}

// From 0..1 range to 0..MAX_ENCODEABLE_XYZ
static uint8 * PackXYZFloatFromFloat(_cmsTRANSFORM* Info, float wOut[], uint8 * output, uint32 Stride)
{
	float* Out = (float *)output;
	if(T_PLANAR(Info->OutputFormat)) {
		Stride /= PixelSize(Info->OutputFormat);
		Out[0]        = (float)(wOut[0] * MAX_ENCODEABLE_XYZ);
		Out[Stride]   = (float)(wOut[1] * MAX_ENCODEABLE_XYZ);
		Out[Stride*2] = (float)(wOut[2] * MAX_ENCODEABLE_XYZ);
		return output + sizeof(float);
	}
	else {
		Out[0] = (float)(wOut[0] * MAX_ENCODEABLE_XYZ);
		Out[1] = (float)(wOut[1] * MAX_ENCODEABLE_XYZ);
		Out[2] = (float)(wOut[2] * MAX_ENCODEABLE_XYZ);
		return output + (sizeof(float)*3 + T_EXTRA(Info->OutputFormat) * sizeof(float));
	}
}

// Same, but convert to double
static uint8 * PackXYZDoubleFromFloat(_cmsTRANSFORM* Info, float wOut[], uint8 * output, uint32 Stride)
{
	double * Out = (double *)output;
	if(T_PLANAR(Info->OutputFormat)) {
		Stride /= PixelSize(Info->OutputFormat);
		Out[0]        = (double)(wOut[0] * MAX_ENCODEABLE_XYZ);
		Out[Stride]   = (double)(wOut[1] * MAX_ENCODEABLE_XYZ);
		Out[Stride*2] = (double)(wOut[2] * MAX_ENCODEABLE_XYZ);
		return output + sizeof(double);
	}
	else {
		Out[0] = (double)(wOut[0] * MAX_ENCODEABLE_XYZ);
		Out[1] = (double)(wOut[1] * MAX_ENCODEABLE_XYZ);
		Out[2] = (double)(wOut[2] * MAX_ENCODEABLE_XYZ);
		return output + (sizeof(double)*3 + T_EXTRA(Info->OutputFormat) * sizeof(double));
	}
}

// ----------------------------------------------------------------------------------------------------------------

#ifndef CMS_NO_HALF_SUPPORT

// Decodes an stream of half floats to wIn[] described by input format

static uint8 * UnrollHalfTo16(_cmsTRANSFORM * info, uint16 wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->InputFormat);
	uint32 DoSwap     = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra      = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	float v;
	uint32 i, start = 0;
	float maximum = IsInkSpace(info->InputFormat) ? 655.35F : 65535.0F;

	Stride /= PixelSize(info->OutputFormat);

	if(ExtraFirst)
		start = Extra;

	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;

		if(Planar)
			v = _cmsHalf2Float( ((uint16*)accum)[(i + start) * Stride]);
		else
			v = _cmsHalf2Float( ((uint16*)accum)[i + start]);

		if(Reverse) v = maximum - v;

		wIn[index] = _cmsQuickSaturateWord(v * maximum);
	}

	if(Extra == 0 && SwapFirst) {
		uint16 tmp = wIn[0];

		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(uint16));
		wIn[nChan-1] = tmp;
	}

	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(uint16);
	else
		return accum + (nChan + Extra) * sizeof(uint16);
}

// Decodes an stream of half floats to wIn[] described by input format
static uint8 * UnrollHalfToFloat(_cmsTRANSFORM * info, float wIn[], uint8 * accum, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->InputFormat);
	uint32 DoSwap     = T_DOSWAP(info->InputFormat);
	uint32 Reverse    = T_FLAVOR(info->InputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->InputFormat);
	uint32 Extra      = T_EXTRA(info->InputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	uint32 Planar     = T_PLANAR(info->InputFormat);
	float v;
	uint32 i, start = 0;
	float maximum = IsInkSpace(info->InputFormat) ? 100.0F : 1.0F;
	Stride /= PixelSize(info->OutputFormat);
	if(ExtraFirst)
		start = Extra;
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		if(Planar)
			v =  _cmsHalf2Float( ((uint16*)accum)[(i + start) * Stride]);
		else
			v =  _cmsHalf2Float( ((uint16*)accum)[i + start]);
		v /= maximum;
		wIn[index] = Reverse ? 1 - v : v;
	}
	if(Extra == 0 && SwapFirst) {
		float tmp = wIn[0];
		memmove(&wIn[0], &wIn[1], (nChan-1) * sizeof(float));
		wIn[nChan-1] = tmp;
	}
	if(T_PLANAR(info->InputFormat))
		return accum + sizeof(uint16);
	else
		return accum + (nChan + Extra) * sizeof(uint16);
}

static uint8 * PackHalfFrom16(_cmsTRANSFORM * info, uint16 wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra      = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar     = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	float maximum = IsInkSpace(info->OutputFormat) ? 655.35F : 65535.0F;
	float v = 0;
	uint16* swap1 = (uint16*)output;
	uint32 i, start = 0;
	Stride /= PixelSize(info->OutputFormat);
	if(ExtraFirst)
		start = Extra;
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		v = (float)wOut[index] / maximum;
		if(Reverse)
			v = maximum - v;
		if(Planar)
			((uint16*)output)[(i + start) * Stride] = _cmsFloat2Half(v);
		else
			((uint16*)output)[i + start] = _cmsFloat2Half(v);
	}
	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan - 1)* sizeof(uint16));
		*swap1 = _cmsFloat2Half(v);
	}
	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(uint16);
	else
		return output + (nChan + Extra) * sizeof(uint16);
}

static uint8 * PackHalfFromFloat(_cmsTRANSFORM * info, float wOut[], uint8 * output, uint32 Stride)
{
	uint32 nChan      = T_CHANNELS(info->OutputFormat);
	uint32 DoSwap     = T_DOSWAP(info->OutputFormat);
	uint32 Reverse    = T_FLAVOR(info->OutputFormat);
	uint32 Extra      = T_EXTRA(info->OutputFormat);
	uint32 SwapFirst  = T_SWAPFIRST(info->OutputFormat);
	uint32 Planar     = T_PLANAR(info->OutputFormat);
	uint32 ExtraFirst = DoSwap ^ SwapFirst;
	float maximum = IsInkSpace(info->OutputFormat) ? 100.0F : 1.0F;
	uint16* swap1 = (uint16*)output;
	float v = 0;
	uint32 i, start = 0;
	Stride /= PixelSize(info->OutputFormat);
	if(ExtraFirst)
		start = Extra;
	for(i = 0; i < nChan; i++) {
		uint32 index = DoSwap ? (nChan - i - 1) : i;
		v = wOut[index] * maximum;
		if(Reverse)
			v = maximum - v;
		if(Planar)
			((uint16*)output)[(i + start)* Stride] = _cmsFloat2Half(v);
		else
			((uint16*)output)[i + start] = _cmsFloat2Half(v);
	}
	if(Extra == 0 && SwapFirst) {
		memmove(swap1 + 1, swap1, (nChan - 1)* sizeof(uint16));
		*swap1 = (uint16)_cmsFloat2Half(v);
	}
	if(T_PLANAR(info->OutputFormat))
		return output + sizeof(uint16);
	else
		return output + (nChan + Extra)* sizeof(uint16);
}

#endif

// ----------------------------------------------------------------------------------------------------------------

static const cmsFormatters16 InputFormatters16[] = {
	//    Type                                          Mask                  Function
	//  ----------------------------   ------------------------------------  ----------------------------
	{ TYPE_Lab_DBL,                                 ANYPLANAR|ANYEXTRA,   UnrollLabDoubleTo16},
	{ TYPE_XYZ_DBL,                                 ANYPLANAR|ANYEXTRA,   UnrollXYZDoubleTo16},
	{ TYPE_Lab_FLT,                                 ANYPLANAR|ANYEXTRA,   UnrollLabFloatTo16},
	{ TYPE_XYZ_FLT,                                 ANYPLANAR|ANYEXTRA,   UnrollXYZFloatTo16},
	{ TYPE_GRAY_DBL,                                                 0,   UnrollDouble1Chan},
	{ FLOAT_SH(1)|BYTES_SH(0), ANYCHANNELS|ANYPLANAR|ANYSWAPFIRST|ANYFLAVOR|
	  ANYSWAP|ANYEXTRA|ANYSPACE,   UnrollDoubleTo16},
	{ FLOAT_SH(1)|BYTES_SH(4), ANYCHANNELS|ANYPLANAR|ANYSWAPFIRST|ANYFLAVOR|
	  ANYSWAP|ANYEXTRA|ANYSPACE,   UnrollFloatTo16},
#ifndef CMS_NO_HALF_SUPPORT
	{ FLOAT_SH(1)|BYTES_SH(2), ANYCHANNELS|ANYPLANAR|ANYSWAPFIRST|ANYFLAVOR|
	  ANYEXTRA|ANYSWAP|ANYSPACE,   UnrollHalfTo16},
#endif

	{ CHANNELS_SH(1)|BYTES_SH(1),                              ANYSPACE,  Unroll1Byte},
	{ CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1),                  ANYSPACE,  Unroll1ByteSkip1},
	{ CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(2),                  ANYSPACE,  Unroll1ByteSkip2},
	{ CHANNELS_SH(1)|BYTES_SH(1)|FLAVOR_SH(1),                 ANYSPACE,  Unroll1ByteReversed},
	{ COLORSPACE_SH(PT_MCH2)|CHANNELS_SH(2)|BYTES_SH(1),              0,  Unroll2Bytes},

	{ TYPE_LabV2_8,                                                   0,  UnrollLabV2_8 },
	{ TYPE_ALabV2_8,                                                  0,  UnrollALabV2_8 },
	{ TYPE_LabV2_16,                                                  0,  UnrollLabV2_16 },

	{ CHANNELS_SH(3)|BYTES_SH(1),                              ANYSPACE,  Unroll3Bytes},
	{ CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1),                 ANYSPACE,  Unroll3BytesSwap},
	{ CHANNELS_SH(3)|EXTRA_SH(1)|BYTES_SH(1)|DOSWAP_SH(1),     ANYSPACE,  Unroll3BytesSkip1Swap},
	{ CHANNELS_SH(3)|EXTRA_SH(1)|BYTES_SH(1)|SWAPFIRST_SH(1),  ANYSPACE,  Unroll3BytesSkip1SwapFirst},

	{ CHANNELS_SH(3)|EXTRA_SH(1)|BYTES_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),
	  ANYSPACE,  Unroll3BytesSkip1SwapSwapFirst},

	{ CHANNELS_SH(4)|BYTES_SH(1),                              ANYSPACE,  Unroll4Bytes},
	{ CHANNELS_SH(4)|BYTES_SH(1)|FLAVOR_SH(1),                 ANYSPACE,  Unroll4BytesReverse},
	{ CHANNELS_SH(4)|BYTES_SH(1)|SWAPFIRST_SH(1),              ANYSPACE,  Unroll4BytesSwapFirst},
	{ CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1),                 ANYSPACE,  Unroll4BytesSwap},
	{ CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1), ANYSPACE,  Unroll4BytesSwapSwapFirst},

	{ BYTES_SH(1)|PLANAR_SH(1), ANYFLAVOR|ANYSWAPFIRST|
	  ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, UnrollPlanarBytes},

	{ BYTES_SH(1),    ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|
	  ANYEXTRA|ANYCHANNELS|ANYSPACE, UnrollChunkyBytes},

	{ CHANNELS_SH(1)|BYTES_SH(2),                              ANYSPACE,  Unroll1Word},
	{ CHANNELS_SH(1)|BYTES_SH(2)|FLAVOR_SH(1),                 ANYSPACE,  Unroll1WordReversed},
	{ CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(3),                  ANYSPACE,  Unroll1WordSkip3},

	{ CHANNELS_SH(2)|BYTES_SH(2),                              ANYSPACE,  Unroll2Words},
	{ CHANNELS_SH(3)|BYTES_SH(2),                              ANYSPACE,  Unroll3Words},
	{ CHANNELS_SH(4)|BYTES_SH(2),                              ANYSPACE,  Unroll4Words},

	{ CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1),                 ANYSPACE,  Unroll3WordsSwap},
	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),  ANYSPACE,  Unroll3WordsSkip1SwapFirst},
	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1),     ANYSPACE,  Unroll3WordsSkip1Swap},
	{ CHANNELS_SH(4)|BYTES_SH(2)|FLAVOR_SH(1),                 ANYSPACE,  Unroll4WordsReverse},
	{ CHANNELS_SH(4)|BYTES_SH(2)|SWAPFIRST_SH(1),              ANYSPACE,  Unroll4WordsSwapFirst},
	{ CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1),                 ANYSPACE,  Unroll4WordsSwap},
	{ CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1)|SWAPFIRST_SH(1), ANYSPACE,  Unroll4WordsSwapSwapFirst},

	{ BYTES_SH(2)|PLANAR_SH(1),  ANYFLAVOR|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollPlanarWords},
	{ BYTES_SH(2),  ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE,  UnrollAnyWords},
};

static const cmsFormattersFloat InputFormattersFloat[] = {
	//    Type                                          Mask                  Function
	//  ----------------------------   ------------------------------------  ----------------------------
	{     TYPE_Lab_DBL,                                ANYPLANAR|ANYEXTRA,   UnrollLabDoubleToFloat},
	{     TYPE_Lab_FLT,                                ANYPLANAR|ANYEXTRA,   UnrollLabFloatToFloat},

	{     TYPE_XYZ_DBL,                                ANYPLANAR|ANYEXTRA,   UnrollXYZDoubleToFloat},
	{     TYPE_XYZ_FLT,                                ANYPLANAR|ANYEXTRA,   UnrollXYZFloatToFloat},

	{     FLOAT_SH(1)|BYTES_SH(4), ANYPLANAR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|
	      ANYCHANNELS|ANYSPACE,  UnrollFloatsToFloat},

	{     FLOAT_SH(1)|BYTES_SH(0), ANYPLANAR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|
	      ANYCHANNELS|ANYSPACE,  UnrollDoublesToFloat},
#ifndef CMS_NO_HALF_SUPPORT
	{     FLOAT_SH(1)|BYTES_SH(2), ANYPLANAR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|
	      ANYCHANNELS|ANYSPACE,  UnrollHalfToFloat},
#endif
};

// Bit fields set to one in the mask are not compared
static cmsFormatter _cmsGetStockInputFormatter(uint32 dwInput, uint32 dwFlags)
{
	uint32 i;
	cmsFormatter fr;
	switch(dwFlags) {
		case CMS_PACK_FLAGS_16BITS: {
		    for(i = 0; i < SIZEOFARRAY(InputFormatters16); i++) {
			    const cmsFormatters16 * f = InputFormatters16 + i;
			    if((dwInput & ~f->Mask) == f->Type) {
				    fr.Fmt16 = f->Frm;
				    return fr;
			    }
		    }
	    }
	    break;
		case CMS_PACK_FLAGS_FLOAT: {
		    for(i = 0; i < SIZEOFARRAY(InputFormattersFloat); i++) {
			    const cmsFormattersFloat * f = InputFormattersFloat + i;
			    if((dwInput & ~f->Mask) == f->Type) {
				    fr.FmtFloat = f->Frm;
				    return fr;
			    }
		    }
	    }
	    break;
		default:;
	}
	fr.Fmt16 = NULL;
	return fr;
}

static const cmsFormatters16 OutputFormatters16[] = {
	//    Type                                          Mask                  Function
	//  ----------------------------   ------------------------------------  ----------------------------

	{ TYPE_Lab_DBL,                                      ANYPLANAR|ANYEXTRA,  PackLabDoubleFrom16},
	{ TYPE_XYZ_DBL,                                      ANYPLANAR|ANYEXTRA,  PackXYZDoubleFrom16},

	{ TYPE_Lab_FLT,                                      ANYPLANAR|ANYEXTRA,  PackLabFloatFrom16},
	{ TYPE_XYZ_FLT,                                      ANYPLANAR|ANYEXTRA,  PackXYZFloatFrom16},

	{ FLOAT_SH(1)|BYTES_SH(0),      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|
	  ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,  PackDoubleFrom16},
	{ FLOAT_SH(1)|BYTES_SH(4),      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|
	  ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,  PackFloatFrom16},
#ifndef CMS_NO_HALF_SUPPORT
	{ FLOAT_SH(1)|BYTES_SH(2),      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|
	  ANYCHANNELS|ANYPLANAR|ANYEXTRA|ANYSPACE,  PackHalfFrom16},
#endif

	{ CHANNELS_SH(1)|BYTES_SH(1),                                  ANYSPACE,  Pack1Byte},
	{ CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1),                      ANYSPACE,  Pack1ByteSkip1},
	{ CHANNELS_SH(1)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack1ByteSkip1SwapFirst},

	{ CHANNELS_SH(1)|BYTES_SH(1)|FLAVOR_SH(1),                     ANYSPACE,  Pack1ByteReversed},

	{ TYPE_LabV2_8,                                                       0,  PackLabV2_8 },
	{ TYPE_ALabV2_8,                                                      0,  PackALabV2_8 },
	{ TYPE_LabV2_16,                                                      0,  PackLabV2_16 },

	{ CHANNELS_SH(3)|BYTES_SH(1)|OPTIMIZED_SH(1),                  ANYSPACE,  Pack3BytesOptimized},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|OPTIMIZED_SH(1),      ANYSPACE,  Pack3BytesAndSkip1Optimized},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1)|OPTIMIZED_SH(1),
	  ANYSPACE,  Pack3BytesAndSkip1SwapFirstOptimized},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1)|OPTIMIZED_SH(1),
	  ANYSPACE,  Pack3BytesAndSkip1SwapSwapFirstOptimized},
	{ CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|EXTRA_SH(1)|OPTIMIZED_SH(1),
	  ANYSPACE,  Pack3BytesAndSkip1SwapOptimized},
	{ CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|OPTIMIZED_SH(1),     ANYSPACE,  Pack3BytesSwapOptimized},

	{ CHANNELS_SH(3)|BYTES_SH(1),                                  ANYSPACE,  Pack3Bytes},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1),                      ANYSPACE,  Pack3BytesAndSkip1},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack3BytesAndSkip1SwapFirst},
	{ CHANNELS_SH(3)|BYTES_SH(1)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),
	  ANYSPACE,  Pack3BytesAndSkip1SwapSwapFirst},
	{ CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1)|EXTRA_SH(1),         ANYSPACE,  Pack3BytesAndSkip1Swap},
	{ CHANNELS_SH(3)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack3BytesSwap},
	{ CHANNELS_SH(6)|BYTES_SH(1),                                  ANYSPACE,  Pack6Bytes},
	{ CHANNELS_SH(6)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack6BytesSwap},
	{ CHANNELS_SH(4)|BYTES_SH(1),                                  ANYSPACE,  Pack4Bytes},
	{ CHANNELS_SH(4)|BYTES_SH(1)|FLAVOR_SH(1),                     ANYSPACE,  Pack4BytesReverse},
	{ CHANNELS_SH(4)|BYTES_SH(1)|SWAPFIRST_SH(1),                  ANYSPACE,  Pack4BytesSwapFirst},
	{ CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1),                     ANYSPACE,  Pack4BytesSwap},
	{ CHANNELS_SH(4)|BYTES_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),     ANYSPACE,  Pack4BytesSwapSwapFirst},

	{ BYTES_SH(1),                 ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackAnyBytes},
	{ BYTES_SH(1)|PLANAR_SH(1),    ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackPlanarBytes},

	{ CHANNELS_SH(1)|BYTES_SH(2),                                  ANYSPACE,  Pack1Word},
	{ CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(1),                      ANYSPACE,  Pack1WordSkip1},
	{ CHANNELS_SH(1)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack1WordSkip1SwapFirst},
	{ CHANNELS_SH(1)|BYTES_SH(2)|FLAVOR_SH(1),                     ANYSPACE,  Pack1WordReversed},
	{ CHANNELS_SH(1)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack1WordBigEndian},
	{ CHANNELS_SH(3)|BYTES_SH(2),                                  ANYSPACE,  Pack3Words},
	{ CHANNELS_SH(3)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack3WordsSwap},
	{ CHANNELS_SH(3)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack3WordsBigEndian},
	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1),                      ANYSPACE,  Pack3WordsAndSkip1},
	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1),         ANYSPACE,  Pack3WordsAndSkip1Swap},
	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|SWAPFIRST_SH(1),      ANYSPACE,  Pack3WordsAndSkip1SwapFirst},

	{ CHANNELS_SH(3)|BYTES_SH(2)|EXTRA_SH(1)|DOSWAP_SH(1)|SWAPFIRST_SH(1),
	  ANYSPACE,  Pack3WordsAndSkip1SwapSwapFirst},

	{ CHANNELS_SH(4)|BYTES_SH(2),                                  ANYSPACE,  Pack4Words},
	{ CHANNELS_SH(4)|BYTES_SH(2)|FLAVOR_SH(1),                     ANYSPACE,  Pack4WordsReverse},
	{ CHANNELS_SH(4)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack4WordsSwap},
	{ CHANNELS_SH(4)|BYTES_SH(2)|ENDIAN16_SH(1),                   ANYSPACE,  Pack4WordsBigEndian},

	{ CHANNELS_SH(6)|BYTES_SH(2),                                  ANYSPACE,  Pack6Words},
	{ CHANNELS_SH(6)|BYTES_SH(2)|DOSWAP_SH(1),                     ANYSPACE,  Pack6WordsSwap},

	{ BYTES_SH(2)|PLANAR_SH(1),     ANYFLAVOR|ANYENDIAN|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackPlanarWords},
	{ BYTES_SH(2),                  ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYENDIAN|ANYEXTRA|ANYCHANNELS|ANYSPACE, PackAnyWords}
};

static const cmsFormattersFloat OutputFormattersFloat[] = {
	//    Type                                          Mask                                 Function
	//  ----------------------------   ---------------------------------------------------
	//  ----------------------------
	{     TYPE_Lab_FLT,                                                ANYPLANAR|ANYEXTRA,   PackLabFloatFromFloat},
	{     TYPE_XYZ_FLT,                                                ANYPLANAR|ANYEXTRA,   PackXYZFloatFromFloat},

	{     TYPE_Lab_DBL,                                                ANYPLANAR|ANYEXTRA,   PackLabDoubleFromFloat},
	{     TYPE_XYZ_DBL,                                                ANYPLANAR|ANYEXTRA,   PackXYZDoubleFromFloat},

	{     FLOAT_SH(1)|BYTES_SH(4), ANYPLANAR|
	      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackFloatsFromFloat },
	{     FLOAT_SH(1)|BYTES_SH(0), ANYPLANAR|
	      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackDoublesFromFloat },
#ifndef CMS_NO_HALF_SUPPORT
	{     FLOAT_SH(1)|BYTES_SH(2),
	      ANYFLAVOR|ANYSWAPFIRST|ANYSWAP|ANYEXTRA|ANYCHANNELS|ANYSPACE,   PackHalfFromFloat },
#endif
};

// Bit fields set to one in the mask are not compared
static cmsFormatter _cmsGetStockOutputFormatter(uint32 dwInput, uint32 dwFlags)
{
	uint32 i;
	cmsFormatter fr;
	// Optimization is only a hint
	dwInput &= ~OPTIMIZED_SH(1);
	switch(dwFlags) {
		case CMS_PACK_FLAGS_16BITS: {
		    for(i = 0; i < SIZEOFARRAY(OutputFormatters16); i++) {
			    const cmsFormatters16* f = OutputFormatters16 + i;
			    if((dwInput & ~f->Mask) == f->Type) {
				    fr.Fmt16 = f->Frm;
				    return fr;
			    }
		    }
	    }
	    break;
		case CMS_PACK_FLAGS_FLOAT: {
		    for(i = 0; i < SIZEOFARRAY(OutputFormattersFloat); i++) {
			    const cmsFormattersFloat* f = OutputFormattersFloat + i;
			    if((dwInput & ~f->Mask) == f->Type) {
				    fr.FmtFloat = f->Frm;
				    return fr;
			    }
		    }
	    }
	    break;
		default:;
	}
	fr.Fmt16 = NULL;
	return fr;
}

typedef struct _cms_formatters_factory_list {
	cmsFormatterFactory Factory;
	struct _cms_formatters_factory_list * Next;
} cmsFormattersFactoryList;

_cmsFormattersPluginChunkType _cmsFormattersPluginChunk = { NULL };

// Duplicates the zone of memory used by the plug-in in the new context
static void DupFormatterFactoryList(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	_cmsFormattersPluginChunkType newHead = { NULL };
	cmsFormattersFactoryList*  entry;
	cmsFormattersFactoryList*  Anterior = NULL;
	_cmsFormattersPluginChunkType* head = (_cmsFormattersPluginChunkType*)src->chunks[FormattersPlugin];
	assert(head != NULL);
	// Walk the list copying all nodes
	for(entry = head->FactoryList; entry != NULL; entry = entry->Next) {
		cmsFormattersFactoryList * newEntry = (cmsFormattersFactoryList*)_cmsSubAllocDup(ctx->MemPool, entry, sizeof(cmsFormattersFactoryList));
		if(!newEntry)
			return;
		// We want to keep the linked list order, so this is a little bit tricky
		newEntry->Next = NULL;
		if(Anterior)
			Anterior->Next = newEntry;
		Anterior = newEntry;
		SETIFZQ(newHead.FactoryList, newEntry);
	}
	ctx->chunks[FormattersPlugin] = _cmsSubAllocDup(ctx->MemPool, &newHead, sizeof(_cmsFormattersPluginChunkType));
}

// The interpolation plug-in memory chunk allocator/dup
void _cmsAllocFormattersPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	assert(ctx);
	if(src) {
		// Duplicate the LIST
		DupFormatterFactoryList(ctx, src);
	}
	else {
		static _cmsFormattersPluginChunkType FormattersPluginChunk = { NULL };
		ctx->chunks[FormattersPlugin] = _cmsSubAllocDup(ctx->MemPool, &FormattersPluginChunk, sizeof(_cmsFormattersPluginChunkType));
	}
}

// Formatters management
boolint _cmsRegisterFormattersPlugin(cmsContext ContextID, cmsPluginBase* Data)
{
	_cmsFormattersPluginChunkType* ctx = (_cmsFormattersPluginChunkType*)_cmsContextGetClientChunk(ContextID, FormattersPlugin);
	cmsPluginFormatters* Plugin = (cmsPluginFormatters*)Data;
	cmsFormattersFactoryList* fl;
	// Reset to built-in defaults
	if(!Data) {
		ctx->FactoryList = NULL;
		return TRUE;
	}
	fl = (cmsFormattersFactoryList*)_cmsPluginMalloc(ContextID, sizeof(cmsFormattersFactoryList));
	if(fl == NULL) return FALSE;
	fl->Factory    = Plugin->FormattersFactory;
	fl->Next = ctx->FactoryList;
	ctx->FactoryList = fl;
	return TRUE;
}

cmsFormatter CMSEXPORT _cmsGetFormatter(cmsContext ContextID, uint32 Type, // Specific type, i.e. TYPE_RGB_8
    cmsFormatterDirection Dir, uint32 dwFlags)
{
	_cmsFormattersPluginChunkType* ctx = (_cmsFormattersPluginChunkType*)_cmsContextGetClientChunk(ContextID, FormattersPlugin);
	cmsFormattersFactoryList* f;
	for(f = ctx->FactoryList; f != NULL; f = f->Next) {
		cmsFormatter fn = f->Factory(Type, Dir, dwFlags);
		if(fn.Fmt16 != NULL) return fn;
	}
	// Revert to default
	if(Dir == cmsFormatterInput)
		return _cmsGetStockInputFormatter(Type, dwFlags);
	else
		return _cmsGetStockOutputFormatter(Type, dwFlags);
}

// Return whatever given formatter refers to float values
boolint _cmsFormatterIsFloat(uint32 Type)
{
	return T_LCMS2_FLOAT(Type) ? TRUE : FALSE;
}

// Return whatever given formatter refers to 8 bits
boolint _cmsFormatterIs8bit(uint32 Type)
{
	uint32 Bytes = T_BYTES(Type);
	return (Bytes == 1);
}

// Build a suitable formatter for the colorspace of this profile
uint32 CMSEXPORT cmsFormatterForColorspaceOfProfile(cmsHPROFILE hProfile, uint32 nBytes, boolint lIsFloat)
{
	cmsColorSpaceSignature ColorSpace      = cmsGetColorSpace(hProfile);
	uint32 ColorSpaceBits  = (uint32)_cmsLCMScolorSpace(ColorSpace);
	uint32 nOutputChans    = cmsChannelsOf(ColorSpace);
	uint32 Float   = lIsFloat ? 1U : 0;
	// Create a fake formatter for result
	return FLOAT_SH(Float) | COLORSPACE_SH(ColorSpaceBits) | BYTES_SH(nBytes) | CHANNELS_SH(nOutputChans);
}

// Build a suitable formatter for the colorspace of this profile
uint32 CMSEXPORT cmsFormatterForPCSOfProfile(cmsHPROFILE hProfile, uint32 nBytes, boolint lIsFloat)
{
	cmsColorSpaceSignature ColorSpace = cmsGetPCS(hProfile);
	uint32 ColorSpaceBits = (uint32)_cmsLCMScolorSpace(ColorSpace);
	uint32 nOutputChans = cmsChannelsOf(ColorSpace);
	uint32 Float = lIsFloat ? 1U : 0;
	// Create a fake formatter for result
	return FLOAT_SH(Float) | COLORSPACE_SH(ColorSpaceBits) | BYTES_SH(nBytes) | CHANNELS_SH(nOutputChans);
}
