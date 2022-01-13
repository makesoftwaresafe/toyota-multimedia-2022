/********************************************************************//**
 * \file iap2_defines.h
 * \brief iAP2 Control session defines
 *
 * \version $ $
 *
 * This header file has the Macro's definition for the return values of
 * functions used in iAP2 Stack.
 *
 * \component global definition file
 *
 * \author Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
 *
 * \copyright (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_DEFINES_H
#define IAP2_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adit_typedef.h"

/**
 * iAP2 Return values
 */
#define IAP2_OK                               0
#define IAP2_CTL_ERROR                       -1
#define IAP2_BAD_PARAMETER                   -2
#define IAP2_INVALID_INPUT_PARAMETER         -3
#define IAP2_INVALID_START_OF_BYTES          -4
#define IAP2_UNKNOWN_MSG_ID                  -5
#define IAP2_ERROR_INVALID_MESSAGE           -6
#define IAP2_UNKNOWN_PARAMETER_ID            -7
#define IAP2_INVALID_PARAMETER_LENGTH        -8
#define IAP2_INVALID_PARAMETER_COUNT         -9
#define IAP2_UNKNOWN_SUB_PARAMETER_ID        -10
#define IAP2_ERR_NO_MEM                      -11
#define IAP2_INVALID_EAP_SESSION_IDENTIFIER  -12
#define IAP2_INVALID_EAP_IDENTIFIER          -13
/*! Connected Apple device does not support USB role switch.
 * Only returned in case of USB Host mode.
 * If IAP2_ERR_USB_ROLE_SWITCH_UNSUP will be returned,
 * the Application shall use USB Device mode instead USB Host mode.
 */
#define IAP2_ERR_USB_ROLE_SWITCH_UNSUP       -14
/*! USB role switch process failed.
 * Only returned in case of USB Host mode.
 * If IAP2_ERR_USB_ROLE_SWITCH_FAILED will be returned,
 * the Application shall use USB Device mode instead USB Host mode.
 * A retry of USB role switch a valid option.
 */
#define IAP2_ERR_USB_ROLE_SWITCH_FAILED      -15
#define IAP2_FILE_XFER_SETUP_NOT_RECVD       -16
#define IAP2_FILE_XFER_SETUP_ALREADY_RECVD   -17
#define IAP2_FILE_XFER_INVALID_ID            -18
#define IAP2_FILE_XFER_MAX_XFER_REACHED      -19
#define IAP2_FILE_XFER_ID_NOT_PRESENT        -20
#define IAP2_DEV_NOT_CONNECTED               -21
#define IAP2_SIZE_EXCEED_MAX_PAYLOAD_SIZE    -22
#define IAP2_COMM_ERROR_SEND                 -23
#define IAP2_ERROR_REQD_DATA_FIELD_MISSING   -24

/**
 * \brief Blob structure
 */
typedef struct
{
    /**
     * \brief Array of ordered byte values
     */
    U8* iAP2BlobData;
    /**
     * \brief Size of iAP2BlobData
     */
    U16 iAP2BlobLength;
} iAP2Blob;

#ifdef __cplusplus
}
#endif

#endif
