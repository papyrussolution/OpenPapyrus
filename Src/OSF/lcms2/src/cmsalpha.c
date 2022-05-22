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
//
// Alpha copy
//
// This macro return words stored as big endian
#define CHANGE_ENDIAN(w)    (uint16)((uint16)((w)<<8)|((w)>>8))

// Floor to byte, taking care of saturation
cmsINLINE uint8 _cmsQuickSaturateByte(double d)
{
	d += 0.5;
	if(d <= 0)
		return 0;
	if(d >= 255.0) 
		return 255;
	return (uint8)_cmsQuickFloorWord(d);
}

// Return the size in bytes of a given formatter
static uint32 trueBytesSize(uint32 Format)
{
	uint32 fmt_bytes = T_BYTES(Format);
	// For double, the T_BYTES field returns zero
	if(fmt_bytes == 0)
		return sizeof(double);
	// Otherwise, it is already correct for all formats
	return fmt_bytes;
}

// Several format converters

typedef void (* cmsFormatterAlphaFn)(void * dst, const void * src);

// From 8

static void copy8(void * dst, const void * src)
{
	memmove(dst, src, 1);
}

static void from8to16(void * dst, const void * src)
{
	uint8 n = *(uint8 *)src;
	*(uint16*)dst = FROM_8_TO_16(n);
}

static void from8to16SE(void * dst, const void * src)
{
	uint8 n = *(uint8 *)src;
	*(uint16*)dst = CHANGE_ENDIAN(FROM_8_TO_16(n));
}

static void from8toFLT(void * dst, const void * src)
{
	*(float *)dst = (*(uint8 *)src) / 255.0f;
}

static void from8toDBL(void * dst, const void * src)
{
	*(double *)dst = (*(uint8 *)src) / 255.0;
}

static void from8toHLF(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = (*(uint8 *)src) / 255.0f;
	*(uint16*)dst = _cmsFloat2Half(n);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

// From 16
static void from16to8(void * dst, const void * src)
{
	uint16 n = *(uint16*)src;
	*(uint8 *)dst = FROM_16_TO_8(n);
}

static void from16SEto8(void * dst, const void * src)
{
	uint16 n = *(uint16*)src;
	*(uint8 *)dst = FROM_16_TO_8(CHANGE_ENDIAN(n));
}

static void copy16(void * dst, const void * src)
{
	memmove(dst, src, 2);
}

static void from16to16(void * dst, const void * src)
{
	uint16 n = *(uint16*)src;
	*(uint16*)dst = CHANGE_ENDIAN(n);
}

static void from16toFLT(void * dst, const void * src)
{
	*(float *)dst = (*(uint16*)src) / 65535.0f;
}

static void from16SEtoFLT(void * dst, const void * src)
{
	*(float *)dst = (CHANGE_ENDIAN(*(uint16*)src)) / 65535.0f;
}

static void from16toDBL(void * dst, const void * src)
{
	*(double *)dst = (*(uint16*)src) / 65535.0f;
}

static void from16SEtoDBL(void * dst, const void * src)
{
	*(double *)dst = (CHANGE_ENDIAN(*(uint16*)src)) / 65535.0f;
}

static void from16toHLF(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = (*(uint16*)src) / 65535.0f;
	*(uint16*)dst = _cmsFloat2Half(n);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void from16SEtoHLF(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = (CHANGE_ENDIAN(*(uint16*)src)) / 65535.0f;
	*(uint16*)dst = _cmsFloat2Half(n);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

// From Float
static void fromFLTto8(void * dst, const void * src)
{
	float n = *(float *)src;
	*(uint8 *)dst = _cmsQuickSaturateByte(n * 255.0f);
}

static void fromFLTto16(void * dst, const void * src)
{
	float n = *(float *)src;
	*(uint16*)dst = _cmsQuickSaturateWord(n * 65535.0f);
}

static void fromFLTto16SE(void * dst, const void * src)
{
	float n = *(float *)src;
	uint16 i = _cmsQuickSaturateWord(n * 65535.0f);
	*(uint16*)dst = CHANGE_ENDIAN(i);
}

static void copy32(void * dst, const void * src)
{
	memmove(dst, src, sizeof(float));
}

static void fromFLTtoDBL(void * dst, const void * src)
{
	float n = *(float *)src;
	*(double *)dst = (double)n;
}

static void fromFLTtoHLF(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = *(float *)src;
	*(uint16*)dst = _cmsFloat2Half(n);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

// From HALF
static void fromHLFto8(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = _cmsHalf2Float(*(uint16*)src);
	*(uint8 *)dst = _cmsQuickSaturateByte(n * 255.0f);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void fromHLFto16(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = _cmsHalf2Float(*(uint16*)src);
	*(uint16*)dst = _cmsQuickSaturateWord(n * 65535.0f);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void fromHLFto16SE(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = _cmsHalf2Float(*(uint16*)src);
	uint16 i = _cmsQuickSaturateWord(n * 65535.0f);
	*(uint16*)dst = CHANGE_ENDIAN(i);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void fromHLFtoFLT(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	*(float *)dst = _cmsHalf2Float(*(uint16*)src);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void fromHLFtoDBL(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	*(double *)dst = (double)_cmsHalf2Float(*(uint16*)src);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

// From double
static void fromDBLto8(void * dst, const void * src)
{
	double n = *(double *)src;
	*(uint8 *)dst = _cmsQuickSaturateByte(n * 255.0);
}

static void fromDBLto16(void * dst, const void * src)
{
	double n = *(double *)src;
	*(uint16*)dst = _cmsQuickSaturateWord(n * 65535.0f);
}

static void fromDBLto16SE(void * dst, const void * src)
{
	double n = *(double *)src;
	uint16 i = _cmsQuickSaturateWord(n * 65535.0f);
	*(uint16*)dst = CHANGE_ENDIAN(i);
}

static void fromDBLtoFLT(void * dst, const void * src)
{
	double n = *(double *)src;
	*(float *)dst = (float)n;
}

static void fromDBLtoHLF(void * dst, const void * src)
{
#ifndef CMS_NO_HALF_SUPPORT
	float n = (float) *(double *)src;
	*(uint16*)dst = _cmsFloat2Half(n);
#else
	CXX_UNUSED(dst);
	CXX_UNUSED(src);
#endif
}

static void copy64(void * dst, const void * src)
{
	memmove(dst, src, sizeof(double));
}

// Returns the position (x or y) of the formatter in the table of functions
static int FormatterPos(uint32 frm)
{
	uint32 b = T_BYTES(frm);
	if(b == 0 && T_LCMS2_FLOAT(frm))
		return 5; // DBL
#ifndef CMS_NO_HALF_SUPPORT
	if(b == 2 && T_LCMS2_FLOAT(frm))
		return 3; // HLF
#endif
	if(b == 4 && T_LCMS2_FLOAT(frm))
		return 4; // FLT
	if(b == 2 && !T_LCMS2_FLOAT(frm)) {
		if(T_ENDIAN16(frm))
			return 2; // 16SE
		else
			return 1; // 16
	}
	if(b == 1 && !T_LCMS2_FLOAT(frm))
		return 0; // 8
	return -1; // not recognized
}

// Obtains an alpha-to-alpha function formatter
static cmsFormatterAlphaFn _cmsGetFormatterAlpha(cmsContext id, uint32 in, uint32 out)
{
	static cmsFormatterAlphaFn FormattersAlpha[6][6] = {
		/* from 8 */ { copy8,       from8to16,   from8to16SE,   from8toHLF,   from8toFLT,    from8toDBL    },
		/* from 16*/ { from16to8,   copy16,      from16to16,    from16toHLF,  from16toFLT,   from16toDBL   },
		/* from 16SE*/ { from16SEto8, from16to16,  copy16,        from16SEtoHLF, from16SEtoFLT, from16SEtoDBL },
		/* from HLF*/ { fromHLFto8,  fromHLFto16, fromHLFto16SE, copy16,       fromHLFtoFLT,  fromHLFtoDBL  },
		/* from FLT*/ { fromFLTto8,  fromFLTto16, fromFLTto16SE, fromFLTtoHLF, copy32,        fromFLTtoDBL  },
		/* from DBL*/ { fromDBLto8,  fromDBLto16, fromDBLto16SE, fromDBLtoHLF, fromDBLtoFLT,  copy64 }
	};
	int in_n  = FormatterPos(in);
	int out_n = FormatterPos(out);
	if(in_n < 0 || out_n < 0 || in_n > 5 || out_n > 5) {
		cmsSignalError(id, cmsERROR_UNKNOWN_EXTENSION, "Unrecognized alpha channel width");
		return NULL;
	}
	return FormattersAlpha[in_n][out_n];
}

// This function computes the distance from each component to the next one in bytes.
static void ComputeIncrementsForChunky(uint32 Format, uint32 ComponentStartingOrder[], uint32 ComponentPointerIncrements[])
{
	uint32 channels[cmsMAXCHANNELS];
	uint32 extra = T_EXTRA(Format);
	uint32 nchannels = T_CHANNELS(Format);
	uint32 total_chans = nchannels + extra;
	uint32 i;
	uint32 channelSize = trueBytesSize(Format);
	uint32 pixelSize = channelSize * total_chans;
	// Sanity check
	if(total_chans <= 0 || total_chans >= cmsMAXCHANNELS)
		return;
	memzero(channels, sizeof(channels));
	// Separation is independent of starting point and only depends on channel size
	for(i = 0; i < extra; i++)
		ComponentPointerIncrements[i] = pixelSize;
	// Handle do swap
	for(i = 0; i < total_chans; i++) {
		if(T_DOSWAP(Format)) {
			channels[i] = total_chans - i - 1;
		}
		else {
			channels[i] = i;
		}
	}
	// Handle swap first (ROL of positions), example CMYK -> KCMY | 0123 -> 3012
	if(T_SWAPFIRST(Format) && total_chans > 1) {
		uint32 tmp = channels[0];
		for(i = 0; i < total_chans-1; i++)
			channels[i] = channels[i+1];
		channels[total_chans - 1] = tmp;
	}
	// Handle size
	if(channelSize > 1)
		for(i = 0; i < total_chans; i++) {
			channels[i] *= channelSize;
		}
	for(i = 0; i < extra; i++)
		ComponentStartingOrder[i] = channels[i + nchannels];
}

//  On planar configurations, the distance is the stride added to any non-negative
static void ComputeIncrementsForPlanar(uint32 Format, uint32 BytesPerPlane, uint32 ComponentStartingOrder[], uint32 ComponentPointerIncrements[])
{
	uint32 channels[cmsMAXCHANNELS];
	uint32 extra = T_EXTRA(Format);
	uint32 nchannels = T_CHANNELS(Format);
	uint32 total_chans = nchannels + extra;
	uint32 i;
	uint32 channelSize = trueBytesSize(Format);
	// Sanity check
	if(total_chans <= 0 || total_chans >= cmsMAXCHANNELS)
		return;
	memzero(channels, sizeof(channels));
	// Separation is independent of starting point and only depends on channel size
	for(i = 0; i < extra; i++)
		ComponentPointerIncrements[i] = channelSize;
	// Handle do swap
	for(i = 0; i < total_chans; i++) {
		if(T_DOSWAP(Format)) {
			channels[i] = total_chans - i - 1;
		}
		else {
			channels[i] = i;
		}
	}
	// Handle swap first (ROL of positions), example CMYK -> KCMY | 0123 -> 3012
	if(T_SWAPFIRST(Format) && total_chans > 0) {
		uint32 tmp = channels[0];
		for(i = 0; i < total_chans - 1; i++)
			channels[i] = channels[i+1];
		channels[total_chans - 1] = tmp;
	}
	// Handle size
	for(i = 0; i < total_chans; i++) {
		channels[i] *= BytesPerPlane;
	}
	for(i = 0; i < extra; i++)
		ComponentStartingOrder[i] = channels[i + nchannels];
}

// Dispatcher por chunky and planar RGB
static void  ComputeComponentIncrements(uint32 Format, uint32 BytesPerPlane, uint32 ComponentStartingOrder[], uint32 ComponentPointerIncrements[])
{
	if(T_PLANAR(Format)) {
		ComputeIncrementsForPlanar(Format,  BytesPerPlane, ComponentStartingOrder, ComponentPointerIncrements);
	}
	else {
		ComputeIncrementsForChunky(Format,  ComponentStartingOrder, ComponentPointerIncrements);
	}
}

// Handles extra channels copying alpha if requested by the flags
void _cmsHandleExtraChannels(_cmsTRANSFORM* p, const void * in, void * out, uint32 PixelsPerLine, uint32 LineCount, const cmsStride* Stride)
{
	uint32 i, j, k;
	uint32 nExtra;
	uint32 SourceStartingOrder[cmsMAXCHANNELS];
	uint32 SourceIncrements[cmsMAXCHANNELS];
	uint32 DestStartingOrder[cmsMAXCHANNELS];
	uint32 DestIncrements[cmsMAXCHANNELS];
	cmsFormatterAlphaFn copyValueFn;
	// Make sure we need some copy
	if(!(p->dwOriginalFlags & cmsFLAGS_COPY_ALPHA))
		return;
	// Exit early if in-place color-management is occurring - no need to copy extra channels to themselves.
	if(p->InputFormat == p->OutputFormat && in == out)
		return;
	// Make sure we have same number of alpha channels. If not, just return as this should be checked at transform
	// creation time.
	nExtra = T_EXTRA(p->InputFormat);
	if(nExtra != T_EXTRA(p->OutputFormat))
		return;
	// Anything to do?
	if(nExtra == 0)
		return;
	// Compute the increments
	ComputeComponentIncrements(p->InputFormat, Stride->BytesPerPlaneIn, SourceStartingOrder, SourceIncrements);
	ComputeComponentIncrements(p->OutputFormat, Stride->BytesPerPlaneOut, DestStartingOrder, DestIncrements);
	// Check for conversions 8, 16, half, float, dbl
	copyValueFn = _cmsGetFormatterAlpha(p->ContextID, p->InputFormat, p->OutputFormat);
	if(copyValueFn == NULL)
		return;
	if(nExtra == 1) { // Optimized routine for copying a single extra channel quickly
		uint8 * SourcePtr;
		uint8 * DestPtr;
		uint32 SourceStrideIncrement = 0;
		uint32 DestStrideIncrement = 0;
		// The loop itself
		for(i = 0; i < LineCount; i++) {
			// Prepare pointers for the loop
			SourcePtr = (uint8 *)in + SourceStartingOrder[0] + SourceStrideIncrement;
			DestPtr = (uint8 *)out + DestStartingOrder[0] + DestStrideIncrement;
			for(j = 0; j < PixelsPerLine; j++) {
				copyValueFn(DestPtr, SourcePtr);
				SourcePtr += SourceIncrements[0];
				DestPtr += DestIncrements[0];
			}
			SourceStrideIncrement += Stride->BytesPerLineIn;
			DestStrideIncrement += Stride->BytesPerLineOut;
		}
	}
	else { // General case with more than one extra channel
		uint8 * SourcePtr[cmsMAXCHANNELS];
		uint8 * DestPtr[cmsMAXCHANNELS];
		uint32 SourceStrideIncrements[cmsMAXCHANNELS];
		uint32 DestStrideIncrements[cmsMAXCHANNELS];
		memzero(SourceStrideIncrements, sizeof(SourceStrideIncrements));
		memzero(DestStrideIncrements, sizeof(DestStrideIncrements));
		// The loop itself
		for(i = 0; i < LineCount; i++) {
			// Prepare pointers for the loop
			for(j = 0; j < nExtra; j++) {
				SourcePtr[j] = (uint8 *)in + SourceStartingOrder[j] + SourceStrideIncrements[j];
				DestPtr[j] = (uint8 *)out + DestStartingOrder[j] + DestStrideIncrements[j];
			}
			for(j = 0; j < PixelsPerLine; j++) {
				for(k = 0; k < nExtra; k++) {
					copyValueFn(DestPtr[k], SourcePtr[k]);
					SourcePtr[k] += SourceIncrements[k];
					DestPtr[k] += DestIncrements[k];
				}
			}
			for(j = 0; j < nExtra; j++) {
				SourceStrideIncrements[j] += Stride->BytesPerLineIn;
				DestStrideIncrements[j] += Stride->BytesPerLineOut;
			}
		}
	}
}
