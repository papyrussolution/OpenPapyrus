/******************************************************************
 *  $Id: alloc.h,v 1.1 2004/10/29 09:28:25 snowdrop Exp $
 *
 * CSOAP Project:  A http client/server library in C
 * Copyright (C) 2003-2004  Ferhat Ayaz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 * 
 * Email: ferhatayaz@yahoo.com
 ******************************************************************/
#ifndef UTILS_ALLOC_H
#define UTILS_ALLOC_H

#ifndef MEM_DEBUG
#error MEM_DEBUG not defined! don not include this file!
#endif

#include <stdlib.h>

#define malloc(size) (_mem_alloc(__FUNCTION__, __FILE__, __LINE__,size))
#define free(p) _mem_free(p)

void* _mem_alloc(const char* func, const char* file, int line, size_t size);
void _mem_free(void *p);
void _mem_report();


#endif

