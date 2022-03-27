/** @file
 * @brief Convert types to std::string
 */
/* Copyright (C) 2009,2015 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#ifndef XAPIAN_INCLUDED_STR_H
#define XAPIAN_INCLUDED_STR_H

#include <string>

namespace Xapian {
	namespace Internal {
		std::string str(int value); /// Convert int to std::string.
		std::string str(uint value); /// Convert uint to std::string.
		std::string str(long value); /// Convert long to std::string.
		std::string str(ulong value); /// Convert ulong to std::string.
		std::string str(long long value); /// Convert long long to std::string.
		std::string str(uint64 value); /// Convert ulong long to std::string.
		std::string str(double value); /// Convert double to std::string.
		std::string str(const void * value); /// Convert const void * to std::string.

		/** Convert std::string to std::string.
		 *
		 *  This is useful as it allows macros and templates to apply str() to a
		 *  type and have it work if that type is std::string.
		 */
		inline std::string str(const std::string & value) { return value; }
		/// Convert const char * to std::string.
		inline std::string str(const char * value) { return value; }
		/// Convert bool to std::string.
		inline std::string str(bool value) { return std::string(1, '0' | static_cast<char>(value)); }
	}
}

using Xapian::Internal::str;

#endif // XAPIAN_INCLUDED_STR_H
