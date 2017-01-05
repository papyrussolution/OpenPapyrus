
#ifndef DTILICENSE_H
#define DTILICENSE_H
/*************************************************************************
 **
 **  Copyright 1998-2001 Pervasive Software Inc. All Rights Reserved
 **
 *************************************************************************/
/*************************************************************************
 *   DTILICENSE.H
 *
 *
 *   This header prototypes the License Manager functions
 *   of the Pervasive Distributed Tuning Interface.
 *
 *     The following functions are found in this file:
 *
 *       PvListLicenses()
 *       PvDeleteLicense()
 *       PvAddLicense()
 *
 *************************************************************************/

#include "dticonst.h"

// the size for dti buffer where the size of str buffer is stored
#define CHAR_SIZE_BUFFER_SIZE 2

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BTI_API PvListLicenses (BTI_LONG  hConnection, BTI_CHAR_PTR licenses, BTI_ULONG bufSize)
 * Description: returnes a list of comma separated licenses from the specified connection.
 *
 * Returns:         P_OK - on sucess , P_E_FAIL  if the call fails
 * Parameters:
 *              IN: hConnection - the connection of interest
 *          IN/OUT: bufSize - the size available for the returned buffer
 *             OUT: licenses - a character buffer contaning a comma separated
 *                  list of license keys
 * Note: the failre can not be related to license manager problem.  It will return
 *       P_OK even if there are no licenses.
 *
 */
BTI_API PvListLicenses (BTI_LONG  hConnection, BTI_CHAR_PTR licenses, BTI_ULONG *bufSize);

/*
 * BTI_API PvDeleteLicense (BTI_LONG  hConnection, BTI_CHAR_PTR licenses)
 * Description: removes the corresponding license from the computer specified with
 *              the connection.
 *
 * Returns: P_OK - on sucess,
 *          P_E_FAIL  if the call fails with a general error
 *          PS_E_LIC_NOT_FOUND - the license is not installed
 *          PS_E_LIC_INVALID - the license is invalid
 * Parameters:
 *              IN: hConnection - the connection of interest
 *                  license - the license key to be removed
 */
BTI_API PvDeleteLicense (BTI_LONG  hConnection, BTI_CHAR_PTR license);


/*
 * BTI_API PvAddLicense (BTI_LONG  hConnection, BTI_CHAR_PTR licenses)
 * Description: installes the specified license from the computer specified with
 *              the connection.
 *
 * Returns: P_OK - on sucess,
 *          P_E_FAIL  if the call fails with a general error
 *          PS_E_LIC_ALREADY_INSTALLED - the license is installed
 *          PS_E_LIC_INVALID - the license is invalid
 * Parameters:
 *              IN: hConnection - the connection of interest
 *                  license - the license key to be removed
 */
BTI_API PvAddLicense (BTI_LONG  hConnection, BTI_CHAR_PTR license);



#ifdef __cplusplus
}
#endif

#endif // DTILICENSE_H