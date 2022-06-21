/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef clangimportH
#define clangimportH

class Tokenizer;

namespace clangimport {
	void parseClangAstDump(Tokenizer * tokenizer, std::istream &f);
}

#endif
