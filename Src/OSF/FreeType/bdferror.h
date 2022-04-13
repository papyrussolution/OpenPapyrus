/*
 * Copyright 2001, 2002, 2012 Francesco Zappa Nardelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
/**************************************************************************
 *
 * This file is used to define the BDF error enumeration constants.
 *
 */

#ifndef BDFERROR_H_
#define BDFERROR_H_

#include <freetype/ftmoderr.h>

#undef FTERRORS_H_

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  BDF_Err_
#define FT_ERR_BASE    FT_Mod_Err_BDF

#include <freetype/fterrors.h>

#endif /* BDFERROR_H_ */

/* END */
