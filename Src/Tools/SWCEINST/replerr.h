/****************************************************************************
*                                                                           *
* replerr.h -- Pegasus filter error codes                                   *
*                                                                           *
* Copyright (c) Microsoft Corporation. All rights reserved.		            *
*                                                                           *
****************************************************************************/

#ifndef _REPLERR_
#define _REPLERR_

/*
 * Define how errors are declared
 */
#define CF_DECLARE_ERROR(e)  (0x80040000 | e)
#define PF_DECLARE_ERROR(e)  CF_DECLARE_ERROR(e)

/*
 * Predefined error messages
 */
#define ERROR_ALREADYCONVERTING  CF_DECLARE_ERROR(0x5000)  // conversion is not reentrant
#define ERROR_UNKNOWNCONVERSION  CF_DECLARE_ERROR(0x5001)  // conversion is not recognized by converter dll
#define ERROR_BADFILE            CF_DECLARE_ERROR(0x5002)  // generic error that indicates that the format of a file was not understood

#endif /* !_REPLERR_ */
