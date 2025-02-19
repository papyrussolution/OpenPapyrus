/*
 * Copyright 2016 Nu-book Inc.
 * Copyright 2016 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop

namespace ZXing::QRCode {
/**
 * See ISO 18004:2006 Annex D.
 * Element i represents the raw version bits that specify version i + 7
 */
static const int VERSION_DECODE_INFO[] = {
	0x07C94, 0x085BC, 0x09A99, 0x0A4D3, 0x0BBF6,
	0x0C762, 0x0D847, 0x0E60D, 0x0F928, 0x10B78,
	0x1145D, 0x12A17, 0x13532, 0x149A6, 0x15683,
	0x168C9, 0x177EC, 0x18EC4, 0x191E1, 0x1AFAB,
	0x1B08E, 0x1CC1A, 0x1D33F, 0x1ED75, 0x1F250,
	0x209D5, 0x216F0, 0x228BA, 0x2379F, 0x24B0B,
	0x2542E, 0x26A64, 0x27541, 0x28C69
};

const Version* Version::Model2(int number)
{
	/**
	 * See ISO 18004:2006 6.5.1 Table 9
	 */
	static const Version allVersions[] = {
		{1, {}, {
			 7,  1, 19, 0, 0,
			 10, 1, 16, 0, 0,
			 13, 1, 13, 0, 0,
			 17, 1, 9, 0, 0
		 }},
		{2, {6, 18}, {
			 10, 1, 34, 0, 0,
			 16, 1, 28, 0, 0,
			 22, 1, 22, 0, 0,
			 28, 1, 16, 0, 0,
		 }},
		{3, {6, 22}, {
			 15, 1, 55, 0, 0,
			 26, 1, 44, 0, 0,
			 18, 2, 17, 0, 0,
			 22, 2, 13, 0, 0,
		 }},
		{4, {6, 26}, {
			 20, 1, 80, 0, 0,
			 18, 2, 32, 0, 0,
			 26, 2, 24, 0, 0,
			 16, 4, 9, 0, 0,
		 }},
		{5, {6, 30}, {
			 26, 1, 108, 0, 0,
			 24, 2, 43, 0, 0,
			 18, 2, 15, 2, 16,
			 22, 2, 11, 2, 12,
		 }},
		{6, {6, 34}, {
			 18, 2, 68, 0, 0,
			 16, 4, 27, 0, 0,
			 24, 4, 19, 0, 0,
			 28, 4, 15, 0, 0,
		 }},
		{7, {6, 22, 38}, {
			 20, 2, 78, 0, 0,
			 18, 4, 31, 0, 0,
			 18, 2, 14, 4, 15,
			 26, 4, 13, 1, 14,
		 }},
		{8, {6, 24, 42}, {
			 24, 2, 97, 0, 0,
			 22, 2, 38, 2, 39,
			 22, 4, 18, 2, 19,
			 26, 4, 14, 2, 15,
		 }},
		{9, {6, 26, 46}, {
			 30, 2, 116, 0, 0,
			 22, 3, 36, 2, 37,
			 20, 4, 16, 4, 17,
			 24, 4, 12, 4, 13,
		 }},
		{10, {6, 28, 50}, {
			 18, 2, 68, 2, 69,
			 26, 4, 43, 1, 44,
			 24, 6, 19, 2, 20,
			 28, 6, 15, 2, 16,
		 }},
		{11, {6, 30, 54}, {
			 20, 4, 81, 0, 0,
			 30, 1, 50, 4, 51,
			 28, 4, 22, 4, 23,
			 24, 3, 12, 8, 13,
		 }},
		{12, {6, 32, 58}, {
			 24, 2, 92, 2, 93,
			 22, 6, 36, 2, 37,
			 26, 4, 20, 6, 21,
			 28, 7, 14, 4, 15,
		 }},
		{13, {6, 34, 62}, {
			 26, 4, 107, 0, 0,
			 22, 8, 37, 1, 38,
			 24, 8, 20, 4, 21,
			 22, 12, 11, 4, 12,
		 }},
		{14, {6, 26, 46, 66}, {
			 30, 3, 115, 1, 116,
			 24, 4, 40, 5, 41,
			 20, 11, 16, 5, 17,
			 24, 11, 12, 5, 13,
		 }},
		{15, {6, 26, 48, 70}, {
			 22, 5, 87, 1, 88,
			 24, 5, 41, 5, 42,
			 30, 5, 24, 7, 25,
			 24, 11, 12, 7, 13,
		 }},
		{16, {6, 26, 50, 74}, {
			 24, 5, 98, 1, 99,
			 28, 7, 45, 3, 46,
			 24, 15, 19, 2, 20,
			 30, 3, 15, 13, 16,
		 }},
		{17, {6, 30, 54, 78}, {
			 28, 1, 107, 5, 108,
			 28, 10, 46, 1, 47,
			 28, 1, 22, 15, 23,
			 28, 2, 14, 17, 15,
		 }},
		{18, {6, 30, 56, 82}, {
			 30, 5, 120, 1, 121,
			 26, 9, 43, 4, 44,
			 28, 17, 22, 1, 23,
			 28, 2, 14, 19, 15,
		 }},
		{19, {6, 30, 58, 86}, {
			 28, 3, 113, 4, 114,
			 26, 3, 44, 11, 45,
			 26, 17, 21, 4, 22,
			 26, 9, 13, 16, 14,
		 }},
		{20, {6, 34, 62, 90}, {
			 28, 3, 107, 5, 108,
			 26, 3, 41, 13, 42,
			 30, 15, 24, 5, 25,
			 28, 15, 15, 10, 16,
		 }},
		{21, {6, 28, 50, 72, 94}, {
			 28, 4, 116, 4, 117,
			 26, 17, 42, 0, 0,
			 28, 17, 22, 6, 23,
			 30, 19, 16, 6, 17,
		 }},
		{22, {6, 26, 50, 74, 98}, {
			 28, 2, 111, 7, 112,
			 28, 17, 46, 0, 0,
			 30, 7, 24, 16, 25,
			 24, 34, 13, 0, 0,
		 }},
		{23, {6, 30, 54, 78, 102}, {
			 30, 4, 121, 5, 122,
			 28, 4, 47, 14, 48,
			 30, 11, 24, 14, 25,
			 30, 16, 15, 14, 16,
		 }},
		{24, {6, 28, 54, 80, 106}, {
			 30, 6, 117, 4, 118,
			 28, 6, 45, 14, 46,
			 30, 11, 24, 16, 25,
			 30, 30, 16, 2, 17,
		 }},
		{25, {6, 32, 58, 84, 110}, {
			 26, 8, 106, 4, 107,
			 28, 8, 47, 13, 48,
			 30, 7, 24, 22, 25,
			 30, 22, 15, 13, 16,
		 }},
		{26, {6, 30, 58, 86, 114}, {
			 28, 10, 114, 2, 115,
			 28, 19, 46, 4, 47,
			 28, 28, 22, 6, 23,
			 30, 33, 16, 4, 17,
		 }},
		{27, {6, 34, 62, 90, 118}, {
			 30, 8, 122, 4, 123,
			 28, 22, 45, 3, 46,
			 30, 8, 23, 26, 24,
			 30, 12, 15, 28, 16,
		 }},
		{28, {6, 26, 50, 74, 98, 122}, {
			 30, 3, 117, 10, 118,
			 28, 3, 45, 23, 46,
			 30, 4, 24, 31, 25,
			 30, 11, 15, 31, 16,
		 }},
		{29, {6, 30, 54, 78, 102, 126}, {
			 30, 7, 116, 7, 117,
			 28, 21, 45, 7, 46,
			 30, 1, 23, 37, 24,
			 30, 19, 15, 26, 16,
		 }},
		{30, {6, 26, 52, 78, 104, 130}, {
			 30, 5, 115, 10, 116,
			 28, 19, 47, 10, 48,
			 30, 15, 24, 25, 25,
			 30, 23, 15, 25, 16,
		 }},
		{31, {6, 30, 56, 82, 108, 134}, {
			 30, 13, 115, 3, 116,
			 28, 2, 46, 29, 47,
			 30, 42, 24, 1, 25,
			 30, 23, 15, 28, 16,
		 }},
		{32, {6, 34, 60, 86, 112, 138}, {
			 30, 17, 115, 0, 0,
			 28, 10, 46, 23, 47,
			 30, 10, 24, 35, 25,
			 30, 19, 15, 35, 16,
		 }},
		{33, {6, 30, 58, 86, 114, 142}, {
			 30, 17, 115, 1, 116,
			 28, 14, 46, 21, 47,
			 30, 29, 24, 19, 25,
			 30, 11, 15, 46, 16,
		 }},
		{34, {6, 34, 62, 90, 118, 146}, {
			 30, 13, 115, 6, 116,
			 28, 14, 46, 23, 47,
			 30, 44, 24, 7, 25,
			 30, 59, 16, 1, 17,
		 }},
		{35, {6, 30, 54, 78, 102, 126, 150}, {
			 30, 12, 121, 7, 122,
			 28, 12, 47, 26, 48,
			 30, 39, 24, 14, 25,
			 30, 22, 15, 41, 16,
		 }},
		{36, {6, 24, 50, 76, 102, 128, 154}, {
			 30, 6, 121, 14, 122,
			 28, 6, 47, 34, 48,
			 30, 46, 24, 10, 25,
			 30, 2, 15, 64, 16,
		 }},
		{37, {6, 28, 54, 80, 106, 132, 158}, {
			 30, 17, 122, 4, 123,
			 28, 29, 46, 14, 47,
			 30, 49, 24, 10, 25,
			 30, 24, 15, 46, 16,
		 }},
		{38, {6, 32, 58, 84, 110, 136, 162}, {
			 30, 4, 122, 18, 123,
			 28, 13, 46, 32, 47,
			 30, 48, 24, 14, 25,
			 30, 42, 15, 32, 16,
		 }},
		{39, {6, 26, 54, 82, 110, 138, 166}, {
			 30, 20, 117, 4, 118,
			 28, 40, 47, 7, 48,
			 30, 43, 24, 22, 25,
			 30, 10, 15, 67, 16,
		 }},
		{40, {6, 30, 58, 86, 114, 142, 170}, {
			 30, 19, 118, 6, 119,
			 28, 18, 47, 31, 48,
			 30, 34, 24, 34, 25,
			 30, 20, 15, 61, 16
		 }},
	};
	return (number < 1 || number > 40) ? nullptr : (allVersions + number - 1);
}

const Version* Version::Micro(int number)
{
	/**
	 * See ISO 18004:2006 6.5.1 Table 9
	 */
	static const Version allVersions[] = {
		{1, {2, 1, 3, 0, 0}},
		{2, {5, 1, 5, 0, 0, 6, 1, 4, 0, 0}},
		{3, {6, 1, 11, 0, 0, 8, 1, 9, 0, 0}},
		{4, {8, 1, 16, 0, 0, 10, 1, 14, 0, 0, 14, 1, 10, 0, 0}}
	};
	return (number < 1 || number > 4) ? nullptr : (allVersions + number - 1);
}

const Version * Version::rMQR(int number)
{
	/**
	 * See ISO/IEC 23941:2022 Annex D, Table D.1 - Column coordinates of centre module of alignment patterns
	 * See ISO/IEC 23941:2022 7.5.1, Table 8 - Error correction characteristics for rMQR
	 */
	static const Version allVersions[] = {
		// Version number, alignment pattern centres, `ECBlocks`
		{ 1, {21}, { // R7x43
			// 4 `ECBlocks`, one for each `ecLevel` - rMQR only uses M & H but using 2 dummies to keep `ecLevel` index same as QR Code
			// Each begins with no. of error correction codewords divided by no. of error correction blocks, followed by 2 `ECBlock`s
			// Each `ECBlock` begins with no. of error correction blocks followed by no. of data codewords per block
			 0, 0,  0, 0,  0, // L (dummy) - also used to differentiate rMQR from Model2 in `Version::Version()`
			 7, 1,  6, 0,  0, // M
			 0, 0,  0, 0,  0, // Q (dummy)
			10, 1,  3, 0,  0, // H
			}},
		{ 2, {19, 39}, { // R7x59
			 0, 0,  0, 0,  0,
			 9, 1, 12, 0,  0,
			 0, 0,  0, 0,  0,
			14, 1,  7, 0,  0,
			}},
		{ 3, {25, 51}, { // R7x77
			 0, 0,  0, 0,  0,
			12, 1, 20, 0,  0,
			 0, 0,  0, 0,  0,
			22, 1, 10, 0,  0,
			}},
		{ 4, {23, 49, 75}, { // R7x99
			 0, 0,  0, 0,  0,
			16, 1, 28, 0,  0,
			 0, 0,  0, 0,  0,
			30, 1, 14, 0,  0,
			}},
		{ 5, {27, 55, 83, 111}, { // R7x139
			 0, 0,  0, 0,  0,
			24, 1, 44, 0,  0,
			 0, 0,  0, 0,  0,
			22, 2, 12, 0,  0,
			}},
		{ 6, {21}, { // R9x43
			 0, 0,  0, 0,  0,
			 9, 1, 12, 0,  0,
			 0, 0,  0, 0,  0,
			14, 1,  7, 0,  0,
			}},
		{ 7, {19, 39}, { // R9x59
			 0, 0,  0, 0,  0,
			12, 1, 21, 0,  0,
			 0, 0,  0, 0,  0,
			22, 1, 11, 0,  0,
			}},
		{ 8, {25, 51}, { // R9x77
			 0, 0,  0, 0,  0,
			18, 1, 31, 0,  0,
			 0, 0,  0, 0,  0,
			16, 1,  8, 1,  9,
			}},
		{ 9, {23, 49, 75}, { // R9x99
			 0, 0,  0, 0,  0,
			24, 1, 42, 0,  0,
			 0, 0,  0, 0,  0,
			22, 2, 11, 0,  0,
			}},
		{10, {27, 55, 83, 111}, { // R9x139
			 0, 0,  0, 0,  0,
			18, 1, 31, 1, 32,
			 0, 0,  0, 0,  0,
			22, 3, 11, 0,  0,
			}},
		{11, {}, { // R11x27
			 0, 0,  0, 0,  0,
			 8, 1,  7, 0,  0,
			 0, 0,  0, 0,  0,
			10, 1,  5, 0,  0,
			}},
		{12, {21}, { // R11x43
			 0, 0,  0, 0,  0,
			12, 1, 19, 0,  0,
			 0, 0,  0, 0,  0,
			20, 1, 11, 0,  0,
			}},
		{13, {19, 39}, { // R11x59
			 0, 0,  0, 0,  0,
			16, 1, 31, 0,  0,
			 0, 0,  0, 0,  0,
			16, 1,  7, 1,  8,
			}},
		{14, {25, 51}, { // R11x77
			 0, 0,  0, 0,  0,
			24, 1, 43, 0,  0,
			 0, 0,  0, 0,  0,
			22, 1, 11, 1, 12,
			}},
		{15, {23, 49, 75}, { // R11x99
			 0, 0,  0, 0,  0,
			16, 1, 28, 1, 29,
			 0, 0,  0, 0,  0,
			30, 1, 14, 1, 15,
			}},
		{16, {27, 55, 83, 111}, { // R11x139
			 0, 0,  0, 0,  0,
			24, 2, 42, 0,  0,
			 0, 0,  0, 0,  0,
			30, 3, 14, 0,  0,
			}},
		{17, {}, { // R13x27
			 0, 0,  0, 0,  0,
			 9, 1, 12, 0,  0,
			 0, 0,  0, 0,  0,
			14, 1,  7, 0,  0,
			}},
		{18, {21}, { // R13x43
			 0, 0,  0, 0,  0,
			14, 1, 27, 0,  0,
			 0, 0,  0, 0,  0,
			28, 1, 13, 0,  0,
			}},
		{19, {19, 39}, { // R13x59
			 0, 0,  0, 0,  0,
			22, 1, 38, 0,  0,
			 0, 0,  0, 0,  0,
			20, 2, 10, 0,  0,
			}},
		{20, {25, 51}, { // R13x77
			 0, 0,  0, 0,  0,
			16, 1, 26, 1, 27,
			 0, 0,  0, 0,  0,
			28, 1, 14, 1, 15,
			}},
		{21, {23, 49, 75}, { // R13x99
			 0, 0,  0, 0,  0,
			20, 1, 36, 1, 37,
			 0, 0,  0, 0,  0,
			26, 1, 11, 2, 12,
			}},
		{22, {27, 55, 83, 111}, { // R13x139
			 0, 0,  0, 0,  0,
			20, 2, 35, 1, 36,
			 0, 0,  0, 0,  0,
			28, 2, 13, 2, 14,
			}},
		{23, {21}, { // R15x43
			 0, 0,  0, 0,  0,
			18, 1, 33, 0,  0,
			 0, 0,  0, 0,  0,
			18, 1,  7, 1,  8,
			}},
		{24, {19, 39}, { // R15x59
			 0, 0,  0, 0,  0,
			26, 1, 48, 0,  0,
			 0, 0,  0, 0,  0,
			24, 2, 13, 0,  0,
			}},
		{25, {25, 51}, { // R15x77
			 0, 0,  0, 0,  0,
			18, 1, 33, 1, 34,
			 0, 0,  0, 0,  0,
			24, 2, 10, 1, 11,
			}},
		{26, {23, 49, 75}, { // R15x99
			 0, 0,  0, 0,  0,
			24, 2, 44, 0,  0,
			 0, 0,  0, 0,  0,
			22, 4, 12, 0,  0,
			}},
		{27, {27, 55, 83, 111}, { // R15x139
			 0, 0,  0, 0,  0,
			24, 2, 42, 1, 43,
			 0, 0,  0, 0,  0,
			26, 1, 13, 4, 14,
			}},
		{28, {21}, { // R17x43
			 0, 0,  0, 0,  0,
			22, 1, 39, 0,  0,
			 0, 0,  0, 0,  0,
			20, 1, 10, 1, 11,
			}},
		{29, {19, 39}, { // R17x59
			 0, 0,  0, 0,  0,
			16, 2, 28, 0,  0,
			 0, 0,  0, 0,  0,
			30, 2, 14, 0,  0,
			}},
		{30, {25, 51}, { // R17x77
			 0, 0,  0, 0,  0,
			22, 2, 39, 0,  0,
			 0, 0,  0, 0,  0,
			28, 1, 12, 2, 13,
			}},
		{31, {23, 49, 75}, { // R17x99
			 0, 0,  0, 0,  0,
			20, 2, 33, 1, 34,
			 0, 0,  0, 0,  0,
			26, 4, 14, 0,  0,
			}},
		{32, {27, 55, 83, 111}, { // R17x139
			 0, 0,  0, 0,  0,
			20, 4, 38, 0,  0,
			 0, 0,  0, 0,  0,
			26, 2, 12, 4, 13,
			}},
	};
	return (number < 1 || number > Size(allVersions)) ? nullptr : (allVersions + number - 1);
}

static const PointI dimsVersionRMQR[32] {
			  {43,  7}, {59,  7}, {77,  7}, {99,  7}, {139,  7},
			  {43,  9}, {59,  9}, {77,  9}, {99,  9}, {139,  9},
	{27, 11}, {43, 11}, {59, 11}, {77, 11}, {99, 11}, {139, 11},
	{27, 13}, {43, 13}, {59, 13}, {77, 13}, {99, 13}, {139, 13},
			  {43, 15}, {59, 15}, {77, 15}, {99, 15}, {139, 15},
			  {43, 17}, {59, 17}, {77, 17}, {99, 17}, {139, 17},
};

static int getVersionRMQR(const BitMatrix& bitMatrix)
{
	const int width = bitMatrix.width();
	const int height = bitMatrix.height();
	if(width != height && (width & 1) && (height & 1) && width >= 27 && width <= 139 && height >= 7 && height <= 17)
		for(int i = 0; i < Size(dimsVersionRMQR); i++)
			if(width == dimsVersionRMQR[i].x && height == dimsVersionRMQR[i].y)
				return i;
	return -1;
}

PointI Version::DimensionOfVersionRMQR(int versionNumber)
{
	if(versionNumber < 1 || versionNumber > Size(dimsVersionRMQR))
		return {0, 0};
	return dimsVersionRMQR[versionNumber - 1];
}

const Version * Version::Model1(int number)
{
	/**
	 * See ISO 18004:2000 M.4.2 Table M.2
	 * See ISO 18004:2000 M.5 Table M.4
	 */
	static const Version allVersions[] = {
		{1, {
			 7, 1, 19, 0, 0,
			 10, 1, 16, 0, 0,
			 13, 1, 13, 0, 0,
			 17, 1, 9, 0, 0
		 }},
		{2, {
			 10, 1, 36, 0, 0,
			 16, 1, 30, 0, 0,
			 22, 1, 24, 0, 0,
			 30, 1, 16, 0, 0,
		 }},
		{3, {
			 15, 1, 57, 0, 0,
			 28, 1, 44, 0, 0,
			 36, 1, 36, 0, 0,
			 48, 1, 24, 0, 0,
		 }},
		{4, {
			 20, 1, 80, 0, 0,
			 40, 1, 60, 0, 0,
			 50, 1, 50, 0, 0,
			 66, 1, 34, 0, 0,
		 }},
		{5, {
			 26, 1, 108, 0, 0,
			 52, 1, 82, 0, 0,
			 66, 1, 68, 0, 0,
			 88, 2, 46, 0, 0,
		 }},
		{6, {
			 34, 1, 136, 0, 0,
			 63, 2, 106, 0, 0,
			 84, 2, 86, 0, 0,
			 112, 2, 58, 0, 0,
		 }},
		{7, {
			 42, 1, 170, 0, 0,
			 80, 2, 132, 0, 0,
			 104, 2, 108, 0, 0,
			 138, 3, 72, 0, 0,
		 }},
		{8, {
			 48, 2, 208, 0, 0,
			 96, 2, 160, 0, 0,
			 128, 2, 128, 0, 0,
			 168, 3, 87, 0, 0,
		 }},
		{9, {
			 60, 2, 246, 0, 0,
			 120, 2, 186, 0, 0,
			 150, 3, 156, 0, 0,
			 204, 3, 102, 0, 0,
		 }},
		{10, {
			 68, 2, 290, 0, 0,
			 136, 2, 222, 0, 0,
			 174, 3, 183, 0, 0,
			 232, 4, 124, 0, 0,
		 }},
		{11, {
			 80, 2, 336, 0, 0,
			 160, 4, 256, 0, 0,
			 208, 4, 208, 0, 0,
			 270, 5, 145, 0, 0,
		 }},
		{12, {
			 92, 2, 384, 0, 0,
			 184, 4, 292, 0, 0,
			 232, 4, 244, 0, 0,
			 310, 5, 165, 0, 0,
		 }},
		{13, {
			 108, 3, 432, 0, 0,
			 208, 4, 332, 0, 0,
			 264, 4, 276, 0, 0,
			 348, 6, 192, 0, 0,
		 }},
		{14, {
			 120, 3, 489, 0, 0,
			 240, 4, 368, 0, 0,
			 300, 5, 310, 0, 0,
			 396, 6, 210, 0, 0,
		 }},
	};
	return (number < 1 || number > 14) ? nullptr : (allVersions + number - 1);
}

Version::Version(int versionNumber, std::initializer_list<int> alignmentPatternCenters, const std::array<ECBlocks, 4>& ecBlocks) : 
	_versionNumber(versionNumber), _alignmentPatternCenters(alignmentPatternCenters), _ecBlocks(ecBlocks),
	_type(ecBlocks[0].codewordsPerBlock == 0 ? Type::rMQR : Type::Model2)
{
	_totalCodewords = ecBlocks[1].totalDataCodewords(); // Use 1 (M) as 0 dummy for rMQR
}

Version::Version(int versionNumber, const std::array<ECBlocks, 4>& ecBlocks) : _versionNumber(versionNumber),
	_ecBlocks(ecBlocks), _type(ecBlocks[0].codewordsPerBlock < 7 || ecBlocks[0].codewordsPerBlock == 8 ? Type::Micro : Type::Model1)
{
	_totalCodewords = ecBlocks[0].totalDataCodewords();
}

bool Version::HasMicroSize(const BitMatrix& bitMatrix)
{
	const int size = bitMatrix.height();
	return (size >= 11 && size <= 17 && (size % 2) == 1);
}

bool Version::HasRMQRSize(const BitMatrix& bitMatrix) { return getVersionRMQR(bitMatrix) != -1; }

bool Version::HasValidSize(const BitMatrix& bitMatrix)
{
	int size = bitMatrix.height();
	if(bitMatrix.width() != size)
		return HasRMQRSize(bitMatrix);
	else
		return HasMicroSize(bitMatrix) || (size >= 21 && size <= 177 && (size % 4) == 1);
}

int Version::Number(const BitMatrix& bitMatrix)
{
	if(bitMatrix.width() != bitMatrix.height())
		return getVersionRMQR(bitMatrix) + 1;
	if(!HasValidSize(bitMatrix))
		return 0;
	bool isMicro = HasMicroSize(bitMatrix);
	return (bitMatrix.height() - DimensionOffset(isMicro)) / DimensionStep(isMicro);
}

const Version* Version::DecodeVersionInformation(int versionBitsA, int versionBitsB)
{
	int bestDifference = std::numeric_limits<int>::max();
	int bestVersion = 0;
	int i = 0;
	for(int targetVersion : VERSION_DECODE_INFO) {
		for(int bits : {versionBitsA, versionBitsB}) {
			int bitsDifference = /*BitHacks::CountBitsSet*/SBits::Cpop(static_cast<uint32>(bits ^ targetVersion));
			if(bitsDifference < bestDifference) {
				bestVersion = i + 7;
				bestDifference = bitsDifference;
			}
		}
		if(bestDifference == 0)
			break;
		++i;
	}
	// We can tolerate up to 3 bits of error since no two version info codewords will
	// differ in less than 8 bits.
	if(bestDifference <= 3)
		return Model2(bestVersion);
	// If we didn't find a close enough match, fail
	return nullptr;
}
/**
* See ISO 18004:2006 Annex E
*/
BitMatrix Version::buildFunctionPattern() const
{
	if(isRMQR()) {
		const PointI dimension = Version::DimensionOfVersionRMQR(_versionNumber);
		BitMatrix bitMatrix(dimension.x, dimension.y);
		// Set edge timing patterns
		bitMatrix.setRegion(0, 0, dimension.x, 1); // Top
		bitMatrix.setRegion(0, dimension.y - 1, dimension.x, 1); // Bottom
		bitMatrix.setRegion(0, 1, 1, dimension.y - 2); // Left
		bitMatrix.setRegion(dimension.x - 1, 1, 1, dimension.y - 2); // Right
		// Set vertical timing and alignment patterns
		const size_t max = _alignmentPatternCenters.size(); // Same as vertical timing column
		for(size_t x = 0; x < max; ++x) {
			const int cx = _alignmentPatternCenters[x];
			bitMatrix.setRegion(cx - 1, 1, 3, 2); // Top alignment pattern
			bitMatrix.setRegion(cx - 1, dimension.y - 3, 3, 2); // Bottom alignment pattern
			bitMatrix.setRegion(cx, 3, 1, dimension.y - 6); // Vertical timing pattern
		}
		// Top left finder pattern + separator
		bitMatrix.setRegion(1, 1, 8 - 1, 8 - 1 - (dimension.y == 7)); // R7 finder bottom flush with edge
		// Top left format
		bitMatrix.setRegion(8, 1, 3, 5);
		bitMatrix.setRegion(11, 1, 1, 3);
		bitMatrix.setRegion(dimension.x - 5, dimension.y - 5, 5 - 1, 5 - 1); // Bottom right finder subpattern
		// Bottom right format
		bitMatrix.setRegion(dimension.x - 8, dimension.y - 6, 3, 5);
		bitMatrix.setRegion(dimension.x - 5, dimension.y - 6, 3, 1);
		bitMatrix.set(dimension.x - 2, 1); // Top right corner finder
		if(dimension.y > 9) {
			bitMatrix.set(1, dimension.y - 2); // Bottom left corner finder
		}
		return bitMatrix;
	}
	else {
		const int dimension = this->dimension();
		BitMatrix bitMatrix(dimension, dimension);
		// Top left finder pattern + separator + format
		bitMatrix.setRegion(0, 0, 9, 9);
		if(!isMicro()) {
			bitMatrix.setRegion(dimension - 8, 0, 8, 9); // Top right finder pattern + separator + format
			bitMatrix.setRegion(0, dimension - 8, 9, 8); // Bottom left finder pattern + separator + format
			// Alignment patterns
			const size_t max = _alignmentPatternCenters.size();
			for(size_t x = 0; x < max; ++x) {
				const int i = _alignmentPatternCenters[x] - 2;
				for(size_t y = 0; y < max; ++y) {
					if((x == 0 && (y == 0 || y == max - 1)) || (x == max - 1 && y == 0)) {
						continue; // No alignment patterns near the three finder patterns
					}
					bitMatrix.setRegion(_alignmentPatternCenters[y] - 2, i, 5, 5);
				}
			}
			bitMatrix.setRegion(6, 9, 1, dimension - 17); // Vertical timing pattern
			bitMatrix.setRegion(9, 6, dimension - 17, 1); // Horizontal timing pattern
			if(_versionNumber > 6) {
				bitMatrix.setRegion(dimension - 11, 0, 3, 6); // Version info, top right
				bitMatrix.setRegion(0, dimension - 11, 6, 3); // Version info, bottom left
			}
		} 
		else {
			bitMatrix.setRegion(9, 0, dimension - 9, 1); // Vertical timing pattern
			bitMatrix.setRegion(0, 9, 1, dimension - 9); // Horizontal timing pattern
		}
		return bitMatrix;
	}
}

} // namespace ZXing::QRCode
