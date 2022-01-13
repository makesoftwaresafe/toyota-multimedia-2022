/************************************************************************//**
*\file : iap2_session_link_callbacks.h
* Header file contains call back functions registered with link layer to get
* notified about event such as data receive or link connection.
*\version : $Id: iap2_session_link_callbacks.h, v Exp $
*\release : $Name:$
*\component :
*\author : ajaykumar.sahoo@in.bosch.com
*
*\copyright (c) 2010 - 2013 Advanced Driver Information Technology.
*          This code is developed by Advanced Driver Information Technology.
*          Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
*          All rights reserved.
*****************************************************************************/


#ifndef IAP2_SESSION_LINK_CALLBACKS_H_
#define IAP2_SESSION_LINK_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "iAP2Link.h"

/**
 * \addtogroup SessionLinkCallbacks
 * @{
 */

/**************************************************************************//**
* Callback function triggered  when link connection is UP/DOWN.
*
* \param iap2Link   Pointer to link object created during link initialisation
* \param bConnected Status of connection. TRUE or FALSE.
*
* \return None
* \see
* \note
******************************************************************************/
void iAP2LinkConnected_CB (iAP2Link_t* iap2Link, BOOL bConnected);


/**************************************************************************//**
* Callback function to call when received data is ready.
*
* Callback function should return as quickly as possible while guaranteeing
* that the data will not be lost... ie. if it can queue the data and can
* guarantee that the data in the queue will eventually be serviced, it can
* return TRUE and if the queue if full, it would return FALSE.
*
* If callback function returns TRUE, the callback function is responsible
* for cleaning up the data buffer.
*
* If FALSE is returned, the packet should not be ACK'd.
*
* \param iap2Link Pointer to link object created during link initialisation
* \param data     pointer to data buffer received from Apple device
* \param dataLen  Length of data received from Apple device
* \param session  Type of session the data belongs to.
*
* \return  TRUE if data is processed and can be discarded.
* \return  FALSE if data cannot be processed at this time.
* \see
* \note
********************************************************************************/
BOOL iAP2LinkDataReady_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session);

/**************************************************************************//**
* Callback function to call when received data is ready in iAP2Service.
*
* Callback function should return as quickly as possible while guaranteeing
* that the data will not be lost... ie. if it can queue the data and can
* guarantee that the data in the queue will eventually be serviced, it can
* return TRUE and if the queue if full, it would return FALSE.
*
* If callback function returns TRUE, the callback function is responsible
* for cleaning up the data buffer.
*
* If FALSE is returned, the packet should not be ACK'd.
*
* \param iap2Link Pointer to link object created during link initialisation
* \param data     pointer to data buffer received from Apple device
* \param dataLen  Length of data received from Apple device
* \param session  Type of session the data belongs to.
*
* \return  TRUE if data is processed and can be discarded.
* \return  FALSE if data cannot be processed at this time.
* \see
* \note
********************************************************************************/
BOOL iAP2LinkDataReadyService_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session);

/*!Callback function to call when data is received on Application(iAP2Library) from iAP2Service*/
BOOL iAP2ServiceLinkDataReady_CB (iAP2Link_t* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* IAP2_LINK_CALLBACKS_H_ */
