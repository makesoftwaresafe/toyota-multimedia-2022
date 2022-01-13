/************************************************************************//**
*\file : iap2_transport_link_callbacks.h
* Header file contains call back functions registered with link layer to
* to send data out to Apple device
*\version : $Id: iap2_transport_link_callbacks.h, v Exp $
*\release : $Name:$
*\component :
*\author : ajaykumar.sahoo@in.bosch.com
*
*\copyright (c) 2010 - 2013 Advanced Driver Information Technology.
*          This code is developed by Advanced Driver Information Technology.
*          Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
*          All rights reserved.
*****************************************************************************/
#include "iAP2Link.h"
#include "iAP2Packet.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IAP2_TRANSPORT_LINK_CALLBACKS_H_
#define IAP2_TRANSPORT_LINK_CALLBACKS_H_

/**
 * \addtogroup TransportLinkCallbacks
 * @{
 */

/**************************************************************************//**
 * Callback function to call when packet is ready to be sent.This callback will
 * actually send the packet out.
 *
 * \param  iap2Link Pointer to link object created during link initialisation
 * \param  packet   Packet to be sent to Apple device
 *
 * \return None.
 * \see
 * \note
 ********************************************************************************/
void iAP2LinkSendPacket_CB (iAP2Link_t* iap2Link, iAP2Packet_t*  packet);

/**************************************************************************//**
 * Callback function to call to send DETECT byte sequence to Apple device.
 *
 * \param  iap2Link Pointer to link object created during link initialisation
 * \param  bBad     If TRUE send DETECT byte sequence { 0xFF, 0x55, 0x02, 0x00,
 *                  0xEE, 0x10 }. If False send DETECT BAD ACK byte sequence
 *                  { 0xFF, 0x55, 0x04, 0x00, 0x02, 0x04, 0xEE, 0x08 }.
 *
 * \return None.
 * \see
 * \note
 ********************************************************************************/
void iAP2LinkSendDetect_CB (iAP2Link_t* iap2Link, BOOL  bBad);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* IAP2_TRANSPORT_LINK_CALLBACKS_H_ */
