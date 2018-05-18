/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005, 2017 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * isalpha --
 *
 * PUBLIC: #ifndef HAVE_ISALPHA
 * PUBLIC: int isalpha __P((int));
 * PUBLIC: #endif
 */
int isalpha(int c)
{
	// Depends on ASCII-like character ordering.
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ? 1 : 0);
}
