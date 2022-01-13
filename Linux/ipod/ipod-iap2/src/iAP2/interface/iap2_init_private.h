/*
 * iap2_init_private.h
 *
 *  Created on: Sep 3, 2013
 *      Author: ajaykumar.s
 */

#ifndef IAP2_INIT_PRIVATE_H_
#define IAP2_INIT_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"
#include "iap2_service_init.h"

/**
 * Transport type dependent Link Sync parameters
 */
#define IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_OUTSTANDING_PACKETS            0x05
#define IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_PACKET_SIZE                    65535
#define IAP2_LINK_SYNC_PARAM_DEFAULT_RETRANSMIT_TMO                     2000
#define IAP2_LINK_SYNC_PARAM_DEFAULT_CUM_ACK_TMO                        22
#define IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_RETRANSMISSIONS                30
#define IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_CUM_ACK                        5
/* Suggested parameter for USB Host Mode (Full Speed) */
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_OUTSTANDING_PACKETS      0x05
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_PACKET_SIZE              65535
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_RETRANSMIT_TMO               2000
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_CUM_ACK_TMO                  22
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_RETRANSMISSIONS          30
#define IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_CUM_ACK                  5
/* Suggested parameter for USB Multi Host Mode (Full Speed) */
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_OUTSTANDING_PACKETS 0x05
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_PACKET_SIZE         65535
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_RETRANSMIT_TMO          2000
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_CUM_ACK_TMO             22
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_RETRANSMISSIONS     30
#define IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_CUM_ACK             5
/* Suggested parameter for USB Device Mode (Full Speed) */
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_OUTSTANDING_PACKETS    0x05
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_PACKET_SIZE            65535
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_RETRANSMIT_TMO             2000
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_CUM_ACK_TMO                22
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_RETRANSMISSIONS        30
#define IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_CUM_ACK                5
/* Suggested parameter for Bluetooth */
#define IAP2_LINK_SYNC_PARAM_BT_MAX_OUTSTANDING_PACKETS                 0x01    /* Suggested value by Apple 5 */
#define IAP2_LINK_SYNC_PARAM_BT_MAX_PACKET_SIZE                         2048
#define IAP2_LINK_SYNC_PARAM_BT_RETRANSMIT_TMO                          1500
#define IAP2_LINK_SYNC_PARAM_BT_CUM_ACK_TMO                             73
#define IAP2_LINK_SYNC_PARAM_BT_MAX_RETRANSMISSIONS                     30
#define IAP2_LINK_SYNC_PARAM_BT_MAX_CUM_ACK                             1       /* Suggested value by Apple 1 */
/* Suggested parameter for 57.6 kbps serial transport */
#define IAP2_LINK_SYNC_PARAM_UART_MAX_OUTSTANDING_PACKETS               0x04
#define IAP2_LINK_SYNC_PARAM_UART_MAX_PACKET_SIZE                       256
#define IAP2_LINK_SYNC_PARAM_UART_RETRANSMIT_TMO                        1000
#define IAP2_LINK_SYNC_PARAM_UART_CUM_ACK_TMO                           134
#define IAP2_LINK_SYNC_PARAM_UART_MAX_RETRANSMISSIONS                   30
#define IAP2_LINK_SYNC_PARAM_UART_MAX_CUM_ACK                           2
/* Suggested parameter for generic transport */
#define IAP2_LINK_SYNC_PARAM_GENERIC_MAX_OUTSTANDING_PACKETS            0x01
#define IAP2_LINK_SYNC_PARAM_GENERIC_MAX_PACKET_SIZE                    128
#define IAP2_LINK_SYNC_PARAM_GENERIC_RETRANSMIT_TMO                     1000
#define IAP2_LINK_SYNC_PARAM_GENERIC_CUM_ACK_TMO                        10
#define IAP2_LINK_SYNC_PARAM_GENERIC_MAX_RETRANSMISSIONS                30
#define IAP2_LINK_SYNC_PARAM_GENERIC_MAX_CUM_ACK                        0
/* Suggested parameter for iAP2 over CarPlay */
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_OUTSTANDING_PACKETS     0x05
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_PACKET_SIZE             65535
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_RETRANSMIT_TMO              2000
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_CUM_ACK_TMO                 22
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_RETRANSMISSIONS         30
#define IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_CUM_ACK                 5

#define IAP2_AUTH_AND_ID_PASSED                                         2

struct iAP2LinkCallbacks;
struct iAP2Link_st;

typedef BOOL (*iAP2LinkReadyCB_t) (struct iAP2Link_st* iap2Link, uint8_t* data, uint32_t dataLen, uint8_t session);
typedef S32  (*iAP2SendMsgToLinkCB_t)(const iAP2Device_t* thisDevice, U8* BufferToSend, U16 BufferLength, iAP2SessionType_t sessionType);

typedef struct iAP2LinkCallbacks
{
    iAP2LinkReadyCB_t recv;
    iAP2SendMsgToLinkCB_t send;
} iAP2LinkCallbacks_t;

/**
 * \brief The device structure
 *
 * Holds all necessary information for a connected device. Such as Transport
 * connection details, authentication details, accessory identification
 * information, accessory configuration details, Session call backs, Generic
 * protocol call backs, Device state information, Device error state information
 * etc... .
 */

typedef struct
{
    void*                       iAP2ContextCallback;

    /*! Accessory transport details */
    iAP2Transport_t             iAP2Transport;

    /*! Accessory authentication  details */
    iAP2Authentication_t        iAP2Authentication;

    /*! Accessory configuration details */
    iAP2AccessoryConfig_t       iAP2AccessoryConfig;

    /*! Accessory Identification information details */
    iAP2AccessoryInfo_t         iAP2AccessoryInfo;

    /*! Callbacks registered for iAP2 session message  */
    iAP2SessionCallbacks_t      iAP2CSCallbacks;

    /*! Callbacks for iAP2 file transfer session  events */
    iAP2FileTransferCallbacks_t iAP2FileTransferCallbacks;

    /*! Callbacks for iAP2 ExternalAccessoryProtocol session  events */
    iAP2EAPSessionCallbacks_t   iAP2EAPSessionCallbacks;

    /*! Callbacks for iAP2 ExternalAccessoryProtocol session  events while using multiple EA sessions */
    iAP2MultiEAPSessionCallbacks_t   iAP2MultiEAPSessionCallbacks;

    /*! Callbacks for iAP2 ExternalAccessory Native Transport  events */
    iAP2EANativeTransportCallbacks_t  p_iAP2EANativeTransportCallbacks;

    /*! Callbacks registered for iAP2 protocol stack generic event */
    iAP2StackCallbacks_t        iAP2StackCallbacks;

    /*! Currently not used : TBD */
    sem_t                       iAP2LinkSem;

    /*! Pointer to iAP2 Link handle*/
    void*                       p_iAP2AccessoryLink;

    /*! Pointer to link packet */
    void*                       p_iAP2LinkPacket;

    /*! Whether to create link packet */
    U8                          iAP2CreateLinkPacket;

    /*! iAP1 / iAP2 :TBD   */
    BOOL                        iAP2DeviceDetect;

    /*! Receive buffer */
    U8                          iAP2LinkRxBuf[RXBUF_SIZE_MAX];

    /*! Receive data length */
    U32                         iAP2LinkRxLen;

    /*! Device states       */
    iAP2DeviceState_t           iAP2DeviceState;

    /*! Device error states */
    iAP2DeviceErrorState_t      iAP2DeviceErrState;

    /*! RunLoop handle      */
    iAP2RunLoop_t               iAP2RunLoop;

    /*! file transfer session object */
    iAP2FileTransferSession_t   *iAP2FileTransfer_list;

    /*! Current iOS App Identifier sent by Apple Device */
    U8                          iAP2CurrentiOSAppIdentifier;

    /*! Matching EAP Session Identifier for the iOS App Identifier provided by application */
    U16                         iAP2MatchingEAPSessionIdentifier;

    /*! Buffer Pool that holds the memory for the iAP2 Parameter extracted from Apple Device */
    U8*                         iAP2BufferPool;

    /*! Verifies whether both Authentication and Identification is done.
     *
     * \brief when control session version 2 is used, the authentication and
     * identification request may come in any order.iAP2DeviceVerified increments
     * itself when either of them is done successfully .
     */
    S32                         iAP2DeviceVerified;

    /*! iAP2Service structure to send messages from Applications to iAP2Server
     *
     * \brief Applications will send iAP2 messages to iAP2 service. iAP2 service
     * then forwards the messages to corresponding Device.
     *
     * Attention: This will be initialized only by iAP2Service based applications.
     */
    iAP2Service_t*              iAP2Service;

    /*! iAP2DeviceId - unique identifier for the connected device.
     *
     *\brief DeviceId used for identifying the origin of the message in the Application.
     *
     * Attention: This will be set only by iAP2Service based applications.
     */
    U32                         iAP2DeviceId;

    /*! iAP2LinkCB - Link interface to be used based on context
     *
     *\brief DeviceId used for identifying the origin of the message in the Application.
     *
     * Attention: This will be set only by iAP2Service based applications.
     */
    struct iAP2LinkCallbacks*    iAP2Link;
} iAP2Device_st;

#ifdef __cplusplus
}
#endif

#endif /* IAP2_INIT_PRIVATE_H_ */
