/********************************************************************//**
 * \file iap2_configure_ffs_gadget.h
 *
 * \brief to Configure FFS Gadget
 * \version $ $
 *
 * This header file function declaration required configure the FFS Gadget.
 *
 * \component global definition file
 *
 * \author Manavalan Veeramani/ RBEI / manavalan.veeramani@in.bosch.com
 *
 * \copyright (c) 2010 - 2016 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_CONFIGURE_FFS_GADGET_H
#define IAP2_CONFIGURE_FFS_GADGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <usbg/usbg.h>
#include "iap2_defines.h"

typedef enum
{
    G_FFS_DEFAULT = 0x00,
    G_FFS_BIND    = 0x01,
    G_FFS_ENABLED = 0x10,
    G_FFS_READY   = 0x11
}g_ffs_status_t;

typedef struct
{
    /**
     * \brief nativeTransport
     *
     * defines whether nativeTransport is Enabled or not.
     *
     */
    BOOL nativeTransport;

    /* ep0 file descriptor */
    S32 ep0_fd;

    /**
     * \brief initEndPoint
     *
     * Path in which initEndPoint is available e.g. /dev/ffs/ep0.
     *
     */
    U8* initEndPoint;

    /* status of the ffs gadget creation */
    g_ffs_status_t g_ffs_status;

    /* String which includes all iOS App names.
     * Each iOSAppName must end with '\0' (null termination) */
    U8** iOSAppNames;

    /* Number of iOS App names in param iOSAppNames */
    U32 iOSAppCnt;

    /* Array of the iOS App Identifier */
    U8* iOSAppIdentifier;
}iAP2_ffs_config_t;

/**************************************************************************//**
 * To print libusbg library API result in USB Host mode using ConfigFS.
 *
 * This API must be called after any call to libusbg API's. It will print the result of
 * the libusbg API call in human understandable strings.
 *
 * \param[in] usbg_error - return value of any call to libusbg API
 * \param[in] fn_name    - name of the libusbg API that was called
 *
 * \see
 * \note
 ******************************************************************************/
void iAP2USBHost_PrintLibUsbgResult(const usbg_error rc, const char* fn_name);

/**************************************************************************//**
 * Initialize USB FFS Gadget in USB Host mode using ConfigFS (or) old gadget FFS.
 *
 * This API must be called after iAP2InitializeGadget() if gadgets are created using ConfigFS.
 * It initialize USB FFS Gadget in USB Host mode using ConfigFS. For old gadget FFS method i.e.,
 * when gadgets are not created using ConfigFS, this API will be called internally from the USB Host
 * mode plugin.
 *
 * \param[in, out] iAP2_ffs_config_t - Structure containing information on how the ffs gadget has to be configured.
 * \param[in]      UseConfigFS       - Boolean variable denoting whether ConfigFS is enabled or not.
 * \param[in]      iAP2_gadget       - The value obtained by calling usbg_create_gadget() (only when ConfigFS is enabled, else pass NULL)
 * \param[in]      iAP2_usbg_udc     - The value obtained by calling usbg_get_udc() (only when ConfigFS is enabled, else pass NULL)
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2ConfigureFFSGadget(iAP2_ffs_config_t* iAP2_ffs_config, BOOL UseConfigFS, usbg_gadget *iAP2_gadget, usbg_udc *iAP2_usbg_udc);

#ifdef __cplusplus
}
#endif

#endif
