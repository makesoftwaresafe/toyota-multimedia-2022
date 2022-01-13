/************************************************************************
 * \file: iap2_external_accessory_protocol_session.h
 *
 * \version: $ $
 *
 * This header file declares functions required for file transfer.
 *
 * \component: global definition file
 *
 * \author: Manavalan Veeramani/Bosch/ manavalan.veeramani@in.bosch.com
 *
 * \copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/
#ifndef IAP2_EXTERNAL_ACCESSORY_PROTOCOL_SESSION_H
#define IAP2_EXTERNAL_ACCESSORY_PROTOCOL_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"

/**
 * \addtogroup ExternalAccessoryProtocolSessionAPIs
 * @{
 */

/***************************************************************************//**
 * Send EAP Session message to apple device
 *
 * This API must be called when the Application would like to send Data to the iOS App and
 * should have received iAP2StartExternalAccessoryProtocolSession_cb from the iAP2 Stack for the
 * corresponding iOS App.
 * iOS App should be opened from the Apple Device, then iAP2 Stack will call the
 * iAP2StartExternalAccessoryProtocolSession_cb if it is registered with iAP2 Stack. Then Application
 * can send data to the iOS App.
 *
 * \param[in] device initialized device structure.
 * \param[in] iAP2iOSAppDataToSend Data that has to be sent to iOS App.
 * \param[in] iAP2iOSAppDataLength Length/Size of the data that has to be sent to iOS App.
 * \param[in] iAP2iOSAppIdentifier Identifier of the iOS App to which the data has to be sent.
 *                                 This identifier has to be same as the Identifier mentioned
 *                                 during initialization/declaration of the iOS App.
 * \return A signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2SendEAPSessionMessage(iAP2Device_t* thisDevice, const U8* iAP2iOSAppDataToSend, U32 iAP2iOSAppDataLength, U8 iAP2iOSAppIdentifier);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
