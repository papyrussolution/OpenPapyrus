/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2021 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef colorH
#define colorH

//#include "cppcheck-config.h"

enum class Color {
	Reset      = 0,
	Bold       = 1,
	Dim        = 2,
	FgRed      = 31,
	FgGreen    = 32,
	FgBlue     = 34,
	FgMagenta  = 35,
	FgDefault  = 39,
	BgRed      = 41,
	BgGreen    = 42,
	BgBlue     = 44,
	BgDefault  = 49
};

CPPCHECKLIB std::ostream& operator<<(std::ostream& os, const Color& c);

std::string toString(const Color& c);

#endif
