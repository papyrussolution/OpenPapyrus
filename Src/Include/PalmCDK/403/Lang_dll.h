/*****************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.  
 * All rights reserved.
 *
 ****************************************************************************/
#ifndef __LANGUAGE_RESOURCE_DLL_H_
#define __LANGUAGE_RESOURCE_DLL_H_
#define DLL_TYPE_NETSYNC    1
#define DLL_TYPE_HOTSYNC    0

#define LANGUAGE_ENGLISH    0x0010
#define LANGUAGE_FRENCH     0x0020
#define LANGUAGE_GERMAN     0x0040
#define LANGUAGE_SPANISH    0x0080
#define LANGUAGE_ITALIAN    0x0100
#define LANGUAGE_DUTCH      0x0200
#define LANGUAGE_JAPANESE   0x1000

#define LANG_DLL_API __declspec(dllexport)

// conduit C API

extern "C" {

LANG_DLL_API  DWORD GetDLLVersion();
typedef  DWORD (*RESOURCEDLLVERSIONFN) ();

}

#endif
