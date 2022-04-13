/** @file
 * @brief Helper functions for safe*.h
 */
/* Copyright (C) 2007 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop
#ifdef __WIN32__
#include "safewindows.h"

// Used by safeunistd.h:
void xapian_sleep_milliseconds(uint millisecs)
{
	Sleep(millisecs);
}

#endif
