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

#ifndef  LEPTONICA_PTRA_H
#define  LEPTONICA_PTRA_H

/*!
 * \file ptra.h
 *
 * <pre>
 *  Contains the following structs:
 *      struct L_Ptra
 *      struct L_Ptraa
 *
 *  Contains definitions for:
 *      L_Ptra compaction flags for removal
 *      L_Ptra shifting flags for insert
 *      L_Ptraa accessor flags
 * </pre>
 */

/* Bound on max initial ptra size */
LEPT_DLL extern const uint32 MaxInitPtraSize;

/*------------------------------------------------------------------------*
*                     Generic Ptr Array Structs                          *
*------------------------------------------------------------------------*/

/*! Generic pointer array */
struct L_Ptra {
	int32 nalloc;      /*!< size of allocated ptr array         */
	int32 imax;      /*!< greatest valid index                */
	int32 nactual;      /*!< actual number of stored elements    */
	void           ** array; /*!< ptr array                           */
};

typedef struct L_Ptra L_PTRA;

/*! Array of generic pointer arrays */
struct L_Ptraa {
	int32 nalloc;      /*!< size of allocated ptr array         */
	struct L_Ptra  ** ptra; /*!< array of ptra                       */
};

typedef struct L_Ptraa L_PTRAA;

/*------------------------------------------------------------------------*
*          Accessor and modifier flags for L_Ptra and L_Ptraa            *
*------------------------------------------------------------------------*/

/*! Ptra Removal */
enum {
	L_NO_COMPACTION = 1,    /*!< null the pointer only                */
	L_COMPACTION = 2        /*!< compact the array                    */
};

/*! Ptra Insertion */
enum {
	L_AUTO_DOWNSHIFT = 0, /*!< choose based on number of holes        */
	L_MIN_DOWNSHIFT = 1,  /*!< downshifts min # of ptrs below insert  */
	L_FULL_DOWNSHIFT = 2  /*!< downshifts all ptrs below insert       */
};

/*! Ptraa Accessor */
enum {
	L_HANDLE_ONLY = 0, /*!< ptr to L_Ptra; caller can inspect only    */
	L_REMOVE = 1       /*!< caller owns; destroy or save in L_Ptraa   */
};

#endif  /* LEPTONICA_PTRA_H */
