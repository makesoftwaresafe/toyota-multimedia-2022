/*
 * iap2_callbacks.h
 *
 *  Created on: Jul 24, 2013
 *      Author: ajaykumar.s
 */

#ifndef IAP2_CALLBACKS_H_
#define IAP2_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"
#include "iap2_cs_callbacks.h"
#include "iap2_file_transfer.h"


/**************************************************************************//**
 * This callback is called when an iOS App would be opened on the iOS device.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2iOSAppIdentifier is the identifier of the iOS App which was opened.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2StartEANativeTransportCB)(iAP2Device_t* thisDevice, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context);


/**************************************************************************//**
 * This callback is called when an iOS App would be closed on the iOS device.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2iOSAppIdentifier is the identifier of the iOS App which was closed.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2StopEANativeTransportCB)(iAP2Device_t* thisDevice, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context);

struct _iAP2EANativeTransportCallbacks
{
    iAP2StartEANativeTransportCB  p_iAP2StartEANativeTransport_cb;
    iAP2StopEANativeTransportCB   p_iAP2StopEANativeTransport_cb;
};
typedef struct _iAP2EANativeTransportCallbacks iAP2EANativeTransportCallbacks_t;

/**
 * \addtogroup EAPSessionCallbacks
 * @{
 */

/***************************************************************************//**
 * This callback will be called when we receive data from the registered iOS App
 * in the Apple device.
 *
 * \param[in] iap2Device Initialized device structure.
 * \param[in] iAP2iOSAppIdentifier states from which of the registered iOS App we
 *  have received the data.
 * \param[in] iAP2iOSAppDataRxd pointer holding the received data.
 * \param[in] iAP2iOSAppDataLength Length of the received data.
 * \return A signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2iOSAppDataReceivedCallback) (iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8* iAP2iOSAppDataRxd, U16 iAP2iOSAppDataLength, void* context);

/** @} */

struct _iAP2EAPSessionCallbacks
{
    iAP2iOSAppDataReceivedCallback iAP2iOSAppDataReceived_cb;
};
typedef struct _iAP2EAPSessionCallbacks iAP2EAPSessionCallbacks_t;

/**
 * \addtogroup EAPMultiSessionCallbacks
 * @{
 */

/***************************************************************************//**
 * This callback will be called when we receive data from the registered iOS Apps
 * in the Apple device in the case of Multiple EA sessions.
 *
 * \param[in] iap2Device Initialized device structure.
 * \param[in] iAP2iOSAppDataRxd pointer holding the received data.
 * \param[in] iAP2iOSAppDataLength Length of the received data.
 * \return A signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2iOSMultiAppDataReceivedCallback) (iAP2Device_t* iap2Device, U8* iAP2iOSAppDataRxd, U16 iAP2iOSAppDataLength, void* context);

/** @} */

struct _iAP2MultiEAPSessionCallbacks
{
    iAP2iOSMultiAppDataReceivedCallback iAP2iOSMultiAppDataReceived_cb;
};
typedef struct _iAP2MultiEAPSessionCallbacks iAP2MultiEAPSessionCallbacks_t;

/**
 * \addtogroup FileTransferSessionCallbacks
 * @{
 */

/**************************************************************************//**
 * Callback to be called when success datagram is sent to device
 *
 * This would be called when file transfer is success and success datagram is
 * sent to device
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 *
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferSuccessCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * Callback to be called when failure datagram is sent to device
 *
 * This would be called when file transfer fails and failure datagram is
 * sent to device
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferFailureCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * Callback to be called when cancel datagram is sent to device
 *
 * This would be called when cancel datagram is sent to device to stop the file
 * transfer
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 *  file transfer related information
 *
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferCancelCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * Callback to be called when pause datagram is sent to device
 *
 * This would be called when pause datagram is sent to device to pause the file
 * transfer
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 *
 * \return Returns a signed integer value indicating success or failure

 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferPauseCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * Callback to be called when resume datagram is sent to device
 *
 * This would be called when resume datagram is sent to device to resume the
 * file transfer.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 *
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferResumeCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * This callback  would be called when file data is received from device.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferDataRcvdCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * This callback  would be called when file data is sent to device.
 *
 * \param[in] thisDevice Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferDataSentCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/**************************************************************************//**
 * Callback to be called when setup datagram is received from device
 *
 * This would be called when setup datagram is received from device to initiate
 * file transfer.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] iAP2FileXferSession Pointer to file transfer structure that holds
 * file transfer related information
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef S32 (*iAP2FileTransferSetupCallback)(iAP2Device_t* thisDevice, iAP2FileTransferSession_t* iAP2FileXferSession, void* context);

/** @} */

struct _iAP2FileTransferCallbacks
{
    iAP2FileTransferSuccessCallback  iAP2FileTransferSuccess_cb;
    iAP2FileTransferFailureCallback  iAP2FileTransferFailure_cb;
    iAP2FileTransferCancelCallback   iAP2FileTransferCancel_cb;
    iAP2FileTransferPauseCallback    iAP2FileTransferPause_cb;
    iAP2FileTransferResumeCallback   iAP2FileTransferResume_cb;
    iAP2FileTransferDataRcvdCallback iAP2FileTransferDataRcvd_cb;
    iAP2FileTransferDataSentCallback iAP2FileTransferDataSent_cb;
    iAP2FileTransferSetupCallback    iAP2FileTransferSetup_cb;
};
typedef struct _iAP2FileTransferCallbacks iAP2FileTransferCallbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* IAP2_CALLBACKS_H_ */
