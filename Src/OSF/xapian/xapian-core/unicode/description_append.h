/** @file
 *  @brief Append a string to an object description, escaping invalid UTF-8
 */
/* Copyright (C) 2013 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_DESCRIPTION_APPEND_H
#define XAPIAN_INCLUDED_DESCRIPTION_APPEND_H

//#include <string>
void description_append(std::string & desc, const std::string & s);

#endif
