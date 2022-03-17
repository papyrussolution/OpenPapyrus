/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 *====================================================================*/

#ifdef _WIN32
#ifndef  LEPTONICA_LEPTWIN_H
#define  LEPTONICA_LEPTWIN_H

#include "allheaders.h"
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

LEPT_DLL extern HBITMAP pixGetWindowsHBITMAP( PIX *pixs );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* LEPTONICA_LEPTWIN_H */
#endif /* _WIN32 */
