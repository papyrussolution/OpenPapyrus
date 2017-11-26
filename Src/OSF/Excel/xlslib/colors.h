/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef COLORS_H
#define COLORS_H

//Added by RLN 111229

// #include "xls_pshpack2.h"

namespace xlslib_core
{
// Colors can be "base" (< 8) or palette (changes with BIFF)
#define COLOR_CODE_BLACK				0x08
// If you use 0x00, Excel won't open the Cell Format dialog box!
#define COLOR_CODE_WHITE				0x09
// If you use 0x01, Excel won't open the Cell Format dialog box!

#define COLOR_CODE_RED					0x0a
#define COLOR_CODE_BRIGHT_GREEN			0x0b
#define COLOR_CODE_BLUE					0x0c
#define COLOR_CODE_YELLOW				0x0d
#define COLOR_CODE_PINK					0x0e
#define COLOR_CODE_TURQUOISE			0x0f

#define COLOR_CODE_DARK_RED				0x10
#define COLOR_CODE_GREEN				0x11
#define COLOR_CODE_DARK_BLUE			0x12
#define COLOR_CODE_DARK_YELLOW			0x13
#define COLOR_CODE_VIOLET				0x14
#define COLOR_CODE_TEAL					0x15
#define COLOR_CODE_GRAY25				0x16
#define COLOR_CODE_GRAY50				0x17

// In Excel2004 on Mac, these represent the lower 16 colors, ordered left to right,
// starting at the top row and moving down
#define COLOR_CODE_PERIWINKLE			0x18
#define COLOR_CODE_DARK_BLUE2			0x19
#define COLOR_CODE_PLUM2				0x1a
#define COLOR_CODE_PINK2				0x1b
#define COLOR_CODE_IVORY				0x1c
#define COLOR_CODE_YELLOW2				0x1d
#define COLOR_CODE_LIGHT_TURQUOISE2		0x1e
#define COLOR_CODE_TURQUOISE2			0x1f
	// ---
#define COLOR_CODE_DARK_PURPLE			0x20
#define COLOR_CODE_VIOLET2				0x21
#define COLOR_CODE_CORAL				0x22
#define COLOR_CODE_DARK_RED2			0x23
#define COLOR_CODE_OCEAN_BLUE			0x24
#define COLOR_CODE_TEAL2				0x25
#define COLOR_CODE_ICE_BLUE				0x26
#define COLOR_CODE_BLUE2				0x27

#define COLOR_CODE_SKY_BLUE				0x28
#define COLOR_CODE_LIGHT_TURQUOISE		0x29
#define COLOR_CODE_LIGHT_GREEN			0x2a
#define COLOR_CODE_LIGHT_YELLOW			0x2b
#define COLOR_CODE_PALEBLUE				0x2c
#define COLOR_CODE_ROSE					0x2d
#define COLOR_CODE_LAVENDER				0x2e
#define COLOR_CODE_TAN					0x2f

#define COLOR_CODE_LIGHT_BLUE			0x30
#define COLOR_CODE_AQUA					0x31
#define COLOR_CODE_LIME					0x32
#define COLOR_CODE_GOLD					0x33
#define COLOR_CODE_LIGHT_ORANGE         0x34
#define COLOR_CODE_ORANGE				0x35
#define COLOR_CODE_BLUE_GRAY			0x36
#define COLOR_CODE_GRAY40				0x37
#define COLOR_CODE_DARK_TEAL			0x38
#define COLOR_CODE_SEA_GREEN			0x39
#define COLOR_CODE_DARK_GREEN			0x3a
#define COLOR_CODE_OLIVE_GREEN			0x3b
#define COLOR_CODE_BROWN				0x3c
#define COLOR_CODE_PLUM					0x3d
#define COLOR_CODE_INDIGO				0x3e
#define COLOR_CODE_GRAY80				0x3f

#define COLOR_CODE_SYS_WIND_FG			0x40
#define COLOR_CODE_SYS_WIND_BG			0x41

	// Good reference: http://www.mvps.org/dmcritchie/excel/colors.htm

	typedef enum {
		ORIG_COLOR_BLACK = 0,	// Well, to get the default fonts etc to use same value as Excel outputs

		// Excel top 40 colors
		CLR_BLACK = 1,  CLR_BROWN,       CLR_OLIVE_GREEN, CLR_DARK_GREEN,      CLR_DARK_TEAL,      CLR_DARK_BLUE,  CLR_INDIGO,     CLR_GRAY80,
		CLR_DARK_RED,   CLR_ORANGE,      CLR_DARK_YELLOW, CLR_GREEN,           CLR_TEAL,           CLR_BLUE,       CLR_BLUE_GRAY,  CLR_GRAY50,
		CLR_RED,        CLR_LITE_ORANGE, CLR_LIME,        CLR_SEA_GREEN,       CLR_AQUA,           CLR_LITE_BLUE,  CLR_VIOLET,     CLR_GRAY40,
		CLR_PINK,       CLR_GOLD,        CLR_YELLOW,      CLR_BRITE_GREEN,     CLR_TURQUOISE,      CLR_SKY_BLUE,   CLR_PLUM,       CLR_GRAY25,
		CLR_ROSE,       CLR_TAN,         CLR_LITE_YELLOW, CLR_LITE_GREEN,      CLR_LITE_TURQUOISE, CLR_PALE_BLUE,  CLR_LAVENDER,   CLR_WHITE,

		// Bottom 16 colors
		CLR_PERIWINKLE, CLR_PLUM2,       CLR_IVORY,       CLR_LITE_TURQUOISE2, CLR_DARK_PURPLE,     CLR_CORAL,     CLR_OCEAN_BLUE, CLR_ICE_BLUE,  
		CLR_DARK_BLUE2, CLR_PINK2,       CLR_YELLOW2,     CLR_TURQUOISE2,      CLR_VIOLET2,         CLR_DARK_RED2, CLR_TEAL2,      CLR_BLUE2,

		CLR_SYS_WIND_FG, CLR_SYS_WIND_BG,

		_NUM_COLOR_NAMES
	} color_name_t;

	struct color_entry_t {
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 nuttin;
	};

	class colors_t { // : public CRecord
	public:
		colors_t();
		~colors_t();
		bool setColor(uint8 r, uint8 g, uint8 b, uint8 idx); // 8 <= idx <= 64
		CUnit* GetData(CDataStorage &datastore) const;
	private:
		colors_t(const colors_t &that);
		colors_t& operator=(const colors_t& right);
	private:
		color_entry_t *colors;
	};
	/*
	 ******************************
	 * CPalette class declaration
	 ******************************
	 */
	class CPalette : public CRecord {
		friend class CDataStorage;
	protected:
		CPalette(CDataStorage &datastore, const color_entry_t *colors);
	private:
		virtual ~CPalette();
	};
}

// #include "xls_poppack.h"

#endif
