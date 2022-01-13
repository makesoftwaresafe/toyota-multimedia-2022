/************************************************************************
 * @file: iap2_message_response.h
 * @brief List of response messages
 *
 * @version: $ $
 *
 * This header file declares functions required for sending response to the messages sent by Apple Devices.
 *
 * @component: global definition file
 *
 * @author: Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
 *
 * @copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_MESSAGE_RESPONSE_H
#define IAP2_MESSAGE_RESPONSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"
#include "iap2_init_private.h"


#define IAP2_VEHICLE_INFORMATION_COMP_ID        0x0001
#define IAP2_VEHICLE_STATUS_COMP_ID             0x0001
#define IAP2_LOCATION_INFORMATION_COMP_ID       0x0001

/**
 * \addtogroup SessionMessageResponse
 * @{
 */
/**************************************************************************//**
* Generates response for the iAP2RequestAuthenticationCertificate message from
* Apple Device.
*
* \param theiAP2RequestAuthenticationCertificateParameter  Structure having
*            information related to iAP2RequestAuthenticationCertificateParameter
*            for which the response has to be sent.
* \param theiAP2AuthenticationCertificateParameter      Structure in which the
*            response for RequestAuthenticationCertificate Message from Apple,
*            has to be stored.
* \param theiAP2AuthenticationCertificateSerialNumberParameter Structure in which the
*            response for RequestAuthenticationCertificate Message from Apple,
*            has to be stored if the serial number is requested.
*
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2RequestAuthenticationCertificate sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER   When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2RequestAuthenticationCertificateResponse(iAP2RequestAuthenticationCertificateParameter theiAP2RequestAuthenticationCertificateParameter, iAP2AuthenticationCertificateParameter* theiAP2AuthenticationCertificateParameter, iAP2AccessoryAuthenticationSerialNumberParameter* theiAP2AuthenticationCertificateSerialNumberParameter);

/**************************************************************************//***
* Generates response for the iAP2RequestAuthenticationChallengeResponse message
* from Apple Device.
*
* \param theiAP2RequestAuthenticationChallengeResponseParameter  Structure having
*       information related to iAP2RequestAuthenticationChallengeResponseParameter
*          for which the response has to be sent.
* \param theiAP2AuthenticationResponseParameter  Structure in which the response
*          for RequestAuthenticationChallengeResponse Message from Apple, has to
*          be stored.
*
* \return IAP2_OK   On Successful completion of Generating Response to the message
*                   iAP2RequestAuthenticationChallengeResponse sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2RequestAuthenticationChallengeResponseResponse(iAP2RequestAuthenticationChallengeResponseParameter theiAP2RequestAuthenticationChallengeResponseParameter, iAP2AuthenticationResponseParameter* theiAP2AuthenticationResponseParameter);

/**************************************************************************//**
* Generates response for the iAP2StartIdentification message from Apple Device.
*
* \param this_iAP2Device Structure having information about the device connected
*                        to the target.
* \param theiAP2IdentificationInformationParameter       Structure in which the
*                        response  for StartIdentification Message from Apple,
*                        has to be stored.
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2StartIdentification sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2StartIdentificationResponse(iAP2Device_st* this_iAP2Device, iAP2IdentificationInformationParameter* theiAP2IdentificationInformationParameter);


/**************************************************************************//**
* Generates response for the iAP2IdentificationAccepted message from Apple Device.
*
* \param this_iAP2Device Structure having information about the device connected
*                        to the target.
* \param theiAP2PowerSourceUpdateParameter Structure in which the response for
*                                          IdentificationAccepted Message from
*                                          Apple, has to be stored.
* \return IAP2_OK   On Successful completion of Generating Response to the message
*                   iAP2IdentificationAccepted sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2PowerSourceUpdate(iAP2Device_st* this_iAP2Device, iAP2PowerSourceUpdateParameter* theiAP2PowerSourceUpdateParameter);


/**************************************************************************//**
* Generates response for the iAP2IdentificationAccepted message from Apple Device.
*
* \param this_iAP2Device  Structure having information about the device
*                         connected to the target.
* \param theiAP2StartPowerUpdatesParameter  Structure in which the response for
*                                           IdentificationAccepted Message from
*                                           Apple, has to be stored.
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2IdentificationAccepted sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2StartPowerUpdates(iAP2Device_st* this_iAP2Device, iAP2StartPowerUpdatesParameter* theiAP2StartPowerUpdatesParameter);

/**************************************************************************//**
* Generates response for the iAP2DeviceAuthenticationCertificate message from
* Apple Device.
*
* \param theiAP2DeviceAuthenticationCertificateParameter       Structure having
*           information related to iAP2DeviceAuthenticationCertificateParameter
*           for which the response has to be sent.
* \param theiAP2RequestDeviceAuthenticationChallengeResponseParameter  Structure
*              in which the response for DeviceAuthenticationCertificate Message
*              from Apple, has to be stored.
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2DeviceAuthenticationCertificate sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2DeviceAuthenticationCertificateResponse(iAP2DeviceAuthenticationCertificateParameter theiAP2DeviceAuthenticationCertificateParameter, iAP2RequestDeviceAuthenticationChallengeResponseParameter* theiAP2RequestDeviceAuthenticationChallengeResponseParameter);

/**************************************************************************//**
* Generates response for the iAP2DeviceAuthenticationResponse message from Apple
* Device.
*
* \param  heiAP2DeviceAuthenticationResponseParameter          Structure having
*              information related to iAP2DeviceAuthenticationResponseParameter
*              for which the response has to be sent.
* \param  theiAP2DeviceAuthenticationSucceededParameter       Structure in which
*              the response for DeviceAuthenticationResponse Message from Apple,
*              has to be stored.
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2DeviceAuthenticationResponse sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2DeviceAuthenticationResponseResponse(iAP2DeviceAuthenticationResponseParameter theiAP2DeviceAuthenticationResponseParameter, iAP2DeviceAuthenticationSucceededParameter* theiAP2DeviceAuthenticationSucceededParameter);

/**************************************************************************//**
* Generates response for the iAP2PowerUpdate message from Apple Device.
*
* \param theiAP2PowerUpdateParameter    Structure having information related to
*                iAP2PowerUpdateParameter for which the response has to be sent.
* \param theiAP2StopPowerUpdatesParameter Structure in which the response for
*                              PowerUpdate Message from Apple, has to be stored.
* \return IAP2_OK  On Successful completion of Generating Response to the message
*                  iAP2PowerUpdate sent by Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \see
* \note
******************************************************************************/
S32 iAP2GenerateiAP2PowerUpdateResponse(iAP2PowerUpdateParameter theiAP2PowerUpdateParameter, iAP2StopPowerUpdatesParameter* theiAP2StopPowerUpdatesParameter);

/**************************************************************************//**
* Replaces IdentificationInformation parameters based on the Identification
* Rejeceted message from Apple Device.
*
* \param this_iAP2Device    Structure having information about identification
*                           information parameters which has to be replaced.
* \param theiAP2IdentificationRejectedParameter Structure in which the rejection
*                              informion from apple device are stored.
* \return IAP2_OK  On Successful completion of replacing identification information
*                  based on the rejection message from Apple device.
* \return IAP2_CTL_ERROR  When not able to replace identification parameters for
*                         the rejection message.
* \return IAP2_ERR_NO_MEM  When not able to allocate memory.
* \see
* \note
******************************************************************************/
S32 iAP2ReplaceRejectedIdentificationInformationParameter(iAP2Device_st* this_iAP2Device, iAP2IdentificationRejectedParameter theiAP2IdentificationRejectedParameter);

/** @} */
#ifdef __cplusplus
}
#endif

#endif
