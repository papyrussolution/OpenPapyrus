/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2017 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * __os_cpu_count --
 *	Return the number of CPUs.
 *
 * PUBLIC: uint32 __os_cpu_count __P((void));
 */
uint32 __os_cpu_count()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	return ((uint32)SystemInfo.dwNumberOfProcessors);
}
