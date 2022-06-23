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

#ifndef  LEPTONICA_STRINGCODE_H
#define  LEPTONICA_STRINGCODE_H

/*!
 * \file stringcode.h
 *
 *     Data structure to hold accumulating generated code for storing
 *     and extracting serializable leptonica objects (e.g., pixa, recog).
 *
 *     Also a flag for selecting a string from the L_GenAssoc struct
 *     in stringcode.
 */

struct L_StrCode {
	int32 fileno;   /*!< index for function and output file names   */
	int32 ifunc;   /*!< index into struct currently being stored   */
	SARRAY       * function; /*!< store case code for extraction             */
	SARRAY       * data; /*!< store base64 encoded data as strings       */
	SARRAY       * descr; /*!< store line in description table            */
	int32 n;   /*!< number of data strings                     */
};

typedef struct L_StrCode L_STRCODE;

/*! Select string in stringcode for a specific serializable data type */
/*! Stringcode Select */
enum {
	L_STR_TYPE = 0,  /*!< typedef for the data type                      */
	L_STR_NAME = 1,  /*!< name of the data type                          */
	L_STR_READER = 2, /*!< reader to get the data type from file          */
	L_STR_MEMREADER = 3 /*!< reader to get the compressed string in memory  */
};

#endif  /* LEPTONICA_STRINGCODE_H */
