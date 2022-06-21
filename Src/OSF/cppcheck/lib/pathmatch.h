/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PATHMATCH_H
#define PATHMATCH_H

//#include "cppcheck-config.h"

/// @addtogroup CLI
/// @{
/**
 * @brief Simple path matching for ignoring paths in CLI.
 */
class CPPCHECKLIB PathMatch {
public:
	/**
	 * The constructor.
	 * @param excludedPaths List of masks.
	 * @param caseSensitive Match the case of the characters when
	 *   matching paths?
	 */
	explicit PathMatch(const std::vector<std::string> &excludedPaths, bool caseSensitive = true);

	/**
	 * @brief Match path against list of masks.
	 * @param path Path to match.
	 * @return true if any of the masks match the path, false otherwise.
	 */
	bool match(const std::string &path) const;

protected:

	/**
	 * @brief Remove filename part from the path.
	 * @param path Path to edit.
	 * @return path without filename part.
	 */
	static std::string removeFilename(const std::string &path);

private:
	std::vector<std::string> mExcludedPaths;
	bool mCaseSensitive;
	std::vector<std::string> mWorkingDirectory;
};

/// @}
#endif // PATHMATCH_H
