#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <sys_time_adit.h>
#include <sys/poll.h>

#include "iap2_init.h"

#include "iap2_dlt_log.h"
#include "iap2_transport.h"
#include "iap2_callbacks.h"
#include "iap2_transport_link_callbacks.h"
#include "iap2_session_link_callbacks.h"
#include "iAP2LinkRunLoop.h"
#include "iAP2Time.h"
#include "iAP2TimeImplementation.h"
#include "iAP2Log.h"
#include "iAP2FileTransfer.h"
#include "iap2_service_init.h"

#include "authentication.h"
#include "ipodauth.h"
#include "iap2_init_private.h"
#include "iap2_utility.h"
#include "iap2_parameter_free.h"

static S32 iAP2LinkInit(iAP2Device_t *iap2Device);
static S32 iAP2LinkDeInit(iAP2Device_t *iap2Device);
static void iAP2ConfigLinkSyncPayload(iAP2Device_st *device, iAP2PacketSYNData_t* iAP2LinkSyncParam);
static S32 iAP2SetAccessoryAuthConfig(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2FreeAccessoryAuthConfig(iAP2AccessoryConfig_t* authConfig);
static S32 iAP2InitAuthenticationParam(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeInitAuthenticationParam(iAP2Device_t *iap2Device);
static S32 iAP2RegisterIAP2StackCallbacks(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeRegisterIAP2StackCallbacks(iAP2StackCallbacks_t*  dStack_cb);
static S32 iAP2RegisterControlSessionCallbacks(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeRegisterControlSessionCallbacks(iAP2SessionCallbacks_t*  dCS_cb);
static S32 iAP2RegisterFileTransferSessionCallbacks( iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeRegisterFileTransferSessionCallbacks(iAP2FileTransferCallbacks_t*  dFileTransfer_cb);
static S32 iAP2InitAccessoryIdentificationParam(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeInitAccessoryIdentificationParam(iAP2AccessoryInfo_t* accIdParams);
static S32 iAP2InitAccessoryConfigParam(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2DeInitAccessoryConfigParam(iAP2AccessoryConfig_t *aConfig);
static S32 iAP2InitializePowerUpdates(iAP2Device_t* iap2Device, iAP2InitParam_t* iap2InitParam);
static S32 iAP2RegisterAuthenticationCallbacks(iAP2Authentication_t *iap2AuthCom);
static S32 iAP2DeRegisterAuthenticationCallbacks(iAP2Authentication_t *iap2AuthCom);
static S32 iAP2CreateRunLoopTimerFD(iAP2Device_t* iap2Device);
static S32 iAP2CallRunLoop(iAP2Device_t* iap2Device, void* arg);

IMPORT void iAP2CleanupFileTransferSession(iAP2Device_t* iAP2Device);

iAP2DeviceErrorState_t iAP2GetDeviceErrorState(iAP2Device_t* device, void* context)
{
    iAP2Device_st*  iap2Device  = (iAP2Device_st*)device;
    context = context; /*currently unused*/

    return iap2Device->iAP2DeviceErrState ;
}

S32 iAP2CanceliAP1Support(iAP2Device_t* device)
{
    S32 rc = IAP2_OK;
    /**
     * Send this byte sequence to apple device indicating lack of backward
     * compatibility with iAP1 device.
     */
    const U8 iAP2DetectBadAck[] = { 0xFF, 0x55, 0x0E, 0x00, 0x13, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEB };
    U8       iAP2DetectBadAckLen = 18;

    iAP2Device_st*      iap2Device  = NULL;
    iAP2Transport_t*    transport   = NULL;

    iap2Device = (iAP2Device_st*)device;

    if(NULL != iap2Device)
    {
        transport = &(iap2Device->iAP2Transport);

        rc = iAP2DeviceWrite(transport, (U8*)iAP2DetectBadAck, iAP2DetectBadAckLen );
        if(rc < IAP2_OK)
        {
            if(rc == IAP2_DEV_NOT_CONNECTED)
            {
                iap2Device->iAP2DeviceState = iAP2NotConnected;
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Device disconnected. DevID:%p",iap2Device);
            }
            else /* error IAP2_COMM_ERROR_SEND will be handled here */
            {
                /* Set Device State & Device Error State */
                iap2Device->iAP2DeviceState = iAP2ComError;
                iap2Device->iAP2DeviceErrState = iAP2TransportConnectionFailed;
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "send 'CanceliAP1Support' failed=%d. DevID:%p", rc, iap2Device);
            }
            /* trigger device state callback */
            (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device,
                                                                   iap2Device->iAP2DeviceState,
                                                                   iap2Device->iAP2ContextCallback);
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR,"Send iAP1 cancel failed. DevID:%p",iap2Device);
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR,"Invalid device handle. DevID:%p", iap2Device);
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2CreateRunLoopTimerFD(iAP2Device_t* device)
{
    S32 rc = IAP2_OK;
    iAP2Device_st* iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    iap2Device->iAP2RunLoop.iAP2RunLoopTimerCallback = NULL;
    iap2Device->iAP2RunLoop.iAP2RunLoopTimer = NULL;

    /* make timerfd non-blocking - TFD_NONBLOCK */
    iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if(iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd < 0)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, " rc = %d DevID:%p",iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd, iap2Device);
        rc = IAP2_CTL_ERROR;
    }
    else
    {
        rc = IAP2_OK;
    }

    return rc;
}

static S32 iAP2CallRunLoop(iAP2Device_t* device, void* arg)
{
    S32 rc = IAP2_OK;
    BOOL bContinue;
    iAP2Device_st* iap2Device = NULL;
    iAP2Packet_t* pkt = (iAP2Packet_t*)arg;
    iap2Device = (iAP2Device_st*)device;
    do
    {
        if((pkt == NULL) && (arg != NULL))
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "Call again for sending pending packets, prev arg = %p", arg);
        }
        bContinue = iAP2LinkRunLoopRunOnce(iap2Device->p_iAP2AccessoryLink, pkt);
        pkt = NULL; //Note: packet is deleted inside iAP2LinkHandleReadyPacket
    }while( (TRUE == bContinue) && (iap2Device->iAP2DeviceState != iAP2NotConnected) );

    if(FALSE != bContinue)
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

static S32 iAP2DeRegisterAuthenticationCallbacks(iAP2Authentication_t *iap2AuthCom)
{
    S32 rc = IAP2_OK;

    if(iap2AuthCom != NULL)
    {
        iap2AuthCom->iAP2AuthComFunction.GetCertificate       = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetChallengeData     = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetDeviceID          = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetFirmwareVersion   = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetProtocolVersion   = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetSignature         = NULL;
        iap2AuthCom->iAP2AuthComFunction.GetSignatureData     = NULL;
        iap2AuthCom->iAP2AuthComFunction.Selftest             = NULL;
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2RegisterAuthenticationCallbacks(iAP2Authentication_t *iap2AuthCom)
{
    S32 rc = IAP2_OK;

    if(iap2AuthCom != NULL)
    {
        iap2AuthCom->iAP2AuthComFunction.GetCertificate       = &AuthenticationGetCertificate;
        iap2AuthCom->iAP2AuthComFunction.GetChallengeData     = &AuthenticationGetChallengeData;
        iap2AuthCom->iAP2AuthComFunction.GetDeviceID          = &AuthenticationGetDeviceID;
        iap2AuthCom->iAP2AuthComFunction.GetFirmwareVersion   = &AuthenticationGetFirmwareVersion;
        iap2AuthCom->iAP2AuthComFunction.GetProtocolVersion   = &AuthenticationGetProtocolVersion;
        iap2AuthCom->iAP2AuthComFunction.GetSignature         = &AuthenticationGetSignature;
        iap2AuthCom->iAP2AuthComFunction.GetSignatureData     = &AuthenticationGetSignatureData;
        iap2AuthCom->iAP2AuthComFunction.Selftest             = &AuthenticationSelftest;
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL,"Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}




static S32 iAP2FreeAccessoryAuthConfig(iAP2AccessoryConfig_t* authConfig)
{
    S32 rc = IAP2_OK;

    if(NULL != authConfig)
    {
        if (NULL != authConfig->iAP2AuthDevicename)
        {
            iAP2FreePointer( (void**)&authConfig->iAP2AuthDevicename);
        }

        if (NULL != authConfig->iAP2AuthIoctlRegAddr)
        {
            iAP2FreePointer( (void**)&authConfig->iAP2AuthIoctlRegAddr);
        }

        if (NULL != authConfig->iAP2AuthGPIOReset)
        {
            iAP2FreePointer( (void**)&authConfig->iAP2AuthGPIOReset);
        }

        if (NULL != authConfig->iAP2AuthGPIOReady)
        {
            iAP2FreePointer( (void**)&authConfig->iAP2AuthGPIOReady);
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2SetAccessoryAuthConfig(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32     rc  =   IAP2_OK;

    iAP2Device_st*          iap2Device       = NULL;
    iAP2AccessoryConfig_t*  initAuthConfig   = NULL;

    iap2Device = (iAP2Device_st*)device;

    iap2Device->iAP2Authentication.iAP2AuthenticationType = iap2InitParam->p_iAP2AccessoryConfig->iAP2AuthenticationType;

    initAuthConfig      = iap2InitParam->p_iAP2AccessoryConfig;

    if(NULL != initAuthConfig)
    {
        switch(iap2Device->iAP2Authentication.iAP2AuthenticationType)
        {
            case iAP2AUTHI2C:
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "Attention: This API is depricated. Please use the configuration by the config file");
                /* Authentication configuration via parameters is deprecated so return IAP2_OK */
                break;
            }
            case iAP2AUTHSPI:
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Auth Type:SPI DevID:%p", iap2Device);
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "DeviceName = %s, IoctlRegAddr = %s, GPIORESET = %s, GPIOREADY = %s DevID:%p",
                                                   (U8*)initAuthConfig->iAP2AuthDevicename,
                                                   (U8*)initAuthConfig->iAP2AuthIoctlRegAddr,
                                                   (U8*)initAuthConfig->iAP2AuthGPIOReset,
                                                   (U8*)initAuthConfig->iAP2AuthGPIOReady,
                                                   iap2Device);
                rc = IAP2_BAD_PARAMETER;
                break;
            }
            case iAP2AUTHGENERIC:
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Auth type: GENERIC DevID:%p",iap2Device);
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "DeviceName = %s, IoctlRegAddr = %s , GPIORESET = %s, GPIOREADY = %s, DevID:%p",
                                                  (U8*)initAuthConfig->iAP2AuthDevicename,
                                                  (U8*)initAuthConfig->iAP2AuthIoctlRegAddr,
                                                  (U8*)initAuthConfig->iAP2AuthGPIOReset,
                                                  (U8*)initAuthConfig->iAP2AuthGPIOReady,
                                                  iap2Device);
                rc = IAP2_BAD_PARAMETER;
                break;
            }
            default:
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Auth Type:Unknown DevID:%p", iap2Device);
                rc = IAP2_BAD_PARAMETER;
                break;
            }
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2DeInitAuthenticationParam(iAP2Device_t *device)
{
    S32 rc  = IAP2_CTL_ERROR;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;
    rc = iAP2DeRegisterAuthenticationCallbacks(&(iap2Device->iAP2Authentication));

    if(IAP2_OK == rc)
    {
        rc = iAP2FreeAccessoryAuthConfig(&(iap2Device->iAP2AccessoryConfig));
    }

    if(IAP2_OK != rc)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "FAILED  rc:%d DevID:%p", rc, iap2Device);
    }
    return rc ;
}

static S32 iAP2InitAuthenticationParam(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc  = IAP2_CTL_ERROR;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    rc = iAP2RegisterAuthenticationCallbacks(&(iap2Device->iAP2Authentication));

    if(IAP2_OK == rc)
    {
        rc = iAP2SetAccessoryAuthConfig(iap2Device, iap2InitParam);
    }
    if(IAP2_OK == rc)
    {
        /* PreFetch the Certificate. Its cached inside ipod_authentication
           library for faster access */

        if(NULL != iap2Device->iAP2Authentication.iAP2AuthComFunction.GetCertificate)
        {
            U8 cert_data[IPOD_AUTH_CP_MAX_CERTLENGTH];
            U16 cert_data_len = 0x00;
            iap2Device->iAP2Authentication.iAP2AuthComFunction.GetCertificate(&cert_data_len, cert_data);
        }
    }

    if(IAP2_OK != rc)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "FAILED  rc:%d DevID:%p", rc, iap2Device);
    }
    return rc ;
}

static void iAP2ConfigLinkSyncPayload(iAP2Device_st *device, iAP2PacketSYNData_t* iAP2LinkSyncParam)
{
    /* Set Link Sync parameters */
    switch(device->iAP2Transport.iAP2TransportType)
    {
        case iAP2USBHOSTMODE:
        {
            /* Suggested parameter for USB Host Mode (Full Speed) */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_USB_HOST_MODE_MAX_CUM_ACK;
            break;
        }
        case iAP2MULTIHOSTMODE:
        {
            /* Suggested parameter for USB Multi Host Mode (Full Speed) */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_USB_MULTIHOST_MODE_MAX_CUM_ACK;
            break;
        }
        case iAP2USBDEVICEMODE:
        {
            /* Suggested parameter for USB Device Mode (Full Speed) */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_USB_DEVICE_MODE_MAX_CUM_ACK;
            break;
        }
        case iAP2UART:
        {
            /* Suggested parameter for 57.6 kbps serial transport */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_UART_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_UART_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_UART_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_UART_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_UART_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_UART_MAX_CUM_ACK;
            break;
        }
        case iAP2BLUETOOTH:
        {
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_BT_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_BT_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_BT_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_BT_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_BT_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_BT_MAX_CUM_ACK;
            break;
        }
        case iAP2OVERCARPLAY:
        {
            /* Parameters for iAP2 over CarPlay */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_IAP2_OVER_CPLY_MAX_CUM_ACK;
            break;
        }
        case iAP2GENERICMODE:
        {
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_GENERIC_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_GENERIC_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_GENERIC_RETRANSMIT_TMO;
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_GENERIC_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_GENERIC_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_GENERIC_MAX_CUM_ACK;
            break;
        }
        default:
        {
            /* use default values */
            iAP2LinkSyncParam->maxOutstandingPackets    = IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_OUTSTANDING_PACKETS;
            iAP2LinkSyncParam->maxPacketSize            = IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_PACKET_SIZE;
            iAP2LinkSyncParam->retransmitTimeout        = IAP2_LINK_SYNC_PARAM_DEFAULT_RETRANSMIT_TMO;
            /* TODO:increase cumAckTimeout value, as our packet processing time is more */
            iAP2LinkSyncParam->cumAckTimeout            = (IAP2_LINK_SYNC_PARAM_DEFAULT_RETRANSMIT_TMO / 2);
            iAP2LinkSyncParam->maxRetransmissions       = IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_RETRANSMISSIONS;
            iAP2LinkSyncParam->maxCumAck                = IAP2_LINK_SYNC_PARAM_DEFAULT_MAX_CUM_ACK;
            break;
        }
    }

    IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "MaxRcvdPackteLen is configured to = %d", iAP2LinkSyncParam->maxPacketSize);

    /* set transport type independent values */
    iAP2LinkSyncParam->version                  = 1;    /* Version of the link being established */
    iAP2LinkSyncParam->numSessionInfo           = 1;    /* Control Session by Default */

    /* use LinkSynDefault values */
    iAP2LinkSyncParam->peerMaxOutstandingPackets = 1;   /* Not used for iAP2LinkCreate */
    iAP2LinkSyncParam->peerMaxPacketSize         = 128; /* Not used for iAP2LinkCreate */


    iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].id        = 0x0A;
    iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].type      = kIAP2PacketServiceTypeControl;
    if(device->iAP2AccessoryConfig.ManualLinkConfig == TRUE)
    {
        /* Manually configured by Mediaplayer */
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].version   = device->iAP2AccessoryConfig.LinkConfig_SessionVersion;
    }
    else
    {
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].version   = 2; /* default - used control session v2*/
    }
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "ManualLinkConfig = %s, LinkConfig_SessionVersion = %d", device->iAP2AccessoryConfig.ManualLinkConfig == TRUE ? "TRUE" : "FALSE", device->iAP2AccessoryConfig.LinkConfig_SessionVersion);

    if(device->iAP2AccessoryConfig.iAP2FileXferSupported == TRUE)
    {
        iAP2LinkSyncParam->numSessionInfo++; /* File Transfer Session */
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].id        = 0x0B;
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].type      = kIAP2PacketServiceTypeBuffer;
        if(device->iAP2AccessoryConfig.FileTransferConfig == TRUE)
        {
            iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].version   = device->iAP2AccessoryConfig.FileTransfer_SessionVersion;
        }
        else
        {
            iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].version   = 1;
        }
    }

    if(device->iAP2AccessoryConfig.iAP2EAPSupported == TRUE)
    {
        iAP2LinkSyncParam->numSessionInfo++; /* External Accessory Protocol Session */
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].id        = 0x0C;
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].type      = kIAP2PacketServiceTypeEA;
        iAP2LinkSyncParam->sessionInfo[(iAP2LinkSyncParam->numSessionInfo)-1].version   = 1;
    }

}

static S32 iAP2DeInitAccessoryIdentificationParam(iAP2AccessoryInfo_t* accIdParams)
{
    S32 rc = IAP2_OK;
    U32 i;

    if (NULL != accIdParams)
    {
        if (NULL != accIdParams->iAP2AccessoryName)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryName);
        }

        if (NULL != accIdParams->iAP2AccessoryModelIdentifier)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryModelIdentifier);
        }

        if (NULL != accIdParams->iAP2AccessoryManufacturer)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryManufacturer);
        }

        if (NULL != accIdParams->iAP2AccessorySerialNumber)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessorySerialNumber);
        }
        if (NULL != accIdParams->iAP2AccessoryFirmwareVersion)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryFirmwareVersion);
        }

        if (NULL != accIdParams->iAP2AccessoryHardwareVersion)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryHardwareVersion);
        }

        if (NULL != accIdParams->iAP2AccessoryVendorId)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryVendorId);
        }

        if (NULL != accIdParams->iAP2AccessoryProductId)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryProductId);
        }

        if (NULL != accIdParams->iAP2AccessoryBcdDevice)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2AccessoryBcdDevice);
        }

        if (NULL != accIdParams->iAP2InitEndPoint)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2InitEndPoint);
        }
        
        if (NULL != accIdParams->iAP2CommandsUsedByApplication)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2CommandsUsedByApplication);
        }

        if (NULL != accIdParams->iAP2CallbacksExpectedFromDevice)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2CallbacksExpectedFromDevice);
        }

        for(i = 0; i < accIdParams->iAP2SupportediOSAppCount; i++)
        {
            if(NULL != accIdParams->iAP2iOSAppInfo)
            {
                accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = 0;
                if (NULL != accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName)
                {
                    iAP2FreePointer( (void**)&accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName);
                }
            }
        }
        if(NULL != accIdParams->iAP2iOSAppInfo)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2iOSAppInfo);
            accIdParams->iAP2SupportediOSAppCount = 0;
        }

        if (NULL != accIdParams->iAP2PreferredAppBundleSeedIdentifier)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2PreferredAppBundleSeedIdentifier);
        }

        if (NULL != accIdParams->iAP2CurrentLanguage)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2CurrentLanguage);
        }

        for(i = 0; i < accIdParams->iAP2SupportedLanguageCount; i++)
        {
            if(NULL != accIdParams->iAP2SupportedLanguage)
            {
                if (NULL != accIdParams->iAP2SupportedLanguage[i])
                {
                    iAP2FreePointer( (void**)&accIdParams->iAP2SupportedLanguage[i]);
                }
            }
        }
        if (NULL != accIdParams->iAP2SupportedLanguage)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2SupportedLanguage);
            accIdParams->iAP2SupportedLanguageCount = 0;
        }

        if(accIdParams->iAP2USBDeviceSupportedAudioSampleRate != NULL)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2USBDeviceSupportedAudioSampleRate);
            accIdParams->iAP2USBDeviceSupportedAudioSampleRate_count = 0;
        }

        if(accIdParams->iAP2BluetoothTransportMAC != NULL)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2BluetoothTransportMAC);
        }

        if(accIdParams->iAP2BluetoothTransportComponent != NULL)
        {
            U16 count;

            for(count=0; count<accIdParams->iAP2BluetoothTransportComponent_count;count++)
            {
                iAP2FreeiAP2BluetoothTransportComponent(&accIdParams->iAP2BluetoothTransportComponent[count]);
            }
            iAP2FreePointer( (void**)&accIdParams->iAP2BluetoothTransportComponent);
            accIdParams->iAP2BluetoothTransportComponent_count = 0;
        }

        if(accIdParams->iAP2USBHIDComponent != NULL)
        {
            U16 count;

            for(count = 0; count < accIdParams->iAP2USBHIDComponent_count; count++)
            {
                iAP2FreeiAP2iAP2HIDComponent(&accIdParams->iAP2USBHIDComponent[count]);
            }
            iAP2FreePointer( (void**)&accIdParams->iAP2USBHIDComponent);
            accIdParams->iAP2USBHIDComponent_count = 0;
        }

        if(accIdParams->iAP2VehicleInformationComponent != NULL)
        {
            if(accIdParams->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
            {
                iAP2FreePointer( (void**)&accIdParams->iAP2VehicleInformationComponent->iAP2EngineType);
            }
            accIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count = 0;
            if (accIdParams->iAP2VehicleInformationComponent->iAP2DisplayName != NULL)
            {
                iAP2FreePointer( (void**)&accIdParams->iAP2VehicleInformationComponent->iAP2DisplayName);
            }
            iAP2FreePointer( (void**)&accIdParams->iAP2VehicleInformationComponent);
        }

        if(accIdParams->iAP2VehicleStatusComponent != NULL)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2VehicleStatusComponent);
        }

        if(accIdParams->iAP2LocationInformationComponent != NULL)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2LocationInformationComponent);
        }
        if(accIdParams->iAP2USBHostHIDComponent != NULL)
        {
            U16 count;

            for(count = 0; count < accIdParams->iAP2USBHostHIDComponent_count; count++)
            {
                iAP2FreeiAP2USBHostHIDComponent(&accIdParams->iAP2USBHostHIDComponent[count]);
            }
            iAP2FreePointer( (void**)&accIdParams->iAP2USBHostHIDComponent);
            accIdParams->iAP2USBHIDComponent_count = 0;
        }

        if(accIdParams->iAP2BluetoothHIDComponent != NULL)
        {
            iAP2FreeiAP2BluetoothHIDComponent(accIdParams->iAP2BluetoothHIDComponent);
            iAP2FreePointer( (void**)&accIdParams->iAP2BluetoothHIDComponent);
        }

        if(accIdParams->iAP2WirelessCarPlayTransportComponent != NULL)
        {
            iAP2FreePointer( (void**)&accIdParams->iAP2WirelessCarPlayTransportComponent);
            accIdParams->iAP2WirelessCarPlayTransportComponent_count = 0;
        }

        if(accIdParams->iAP2RouteGuidanceDisplayComponent != NULL)
        {
            U16 count;

            for(count = 0; count < accIdParams->iAP2RouteGuidanceDisplayComponent_count; count++)
            {
                iAP2FreeiAP2RouteGuidanceDisplayComponent(&accIdParams->iAP2RouteGuidanceDisplayComponent[count]);
            }
            iAP2FreePointer( (void**)&accIdParams->iAP2RouteGuidanceDisplayComponent);
            accIdParams->iAP2RouteGuidanceDisplayComponent_count = 0;
        }

    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "FAILED  rc:%d", rc);
    }

    return rc;
}

/* Used to initialize the EA protocol registration information on the Application side.
 * Routing to application callback is decided based on this information.
 * */
static S32 iAP2ServiceInitAccessoryIdentificationParam(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK ;
    U16 i = 0 ;

    iAP2Device_st*       iap2Device        = NULL;
    iAP2AccessoryInfo_t* initAccIdParams   = NULL;
    iAP2AccessoryInfo_t* accIdParams       = NULL;

    iap2Device = (iAP2Device_st*)device;

    initAccIdParams         = iap2InitParam->p_iAP2AccessoryInfo;
    accIdParams             = &(iap2Device->iAP2AccessoryInfo);

    if( (initAccIdParams->iAP2SupportediOSAppCount > 0) )
    {
        accIdParams->iAP2iOSAppInfo = calloc(initAccIdParams->iAP2SupportediOSAppCount, sizeof(iAP2iOSAppInfo_t));
        if(accIdParams->iAP2iOSAppInfo != NULL)
        {
            for(i = 0; ( (i < initAccIdParams->iAP2SupportediOSAppCount) && (rc == IAP2_OK) ); i++)
            {
                rc = iAP2AllocateSPtr(&accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName, initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName);
                if(rc == IAP2_OK)
                {
                    accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                    accIdParams->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay =
                                                        initAccIdParams->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay;
                    accIdParams->iAP2iOSAppInfo[i].iAP2EAPMatchAction   = initAccIdParams->iAP2iOSAppInfo[i].iAP2EAPMatchAction;

                    accIdParams->iAP2iOSAppInfo[i].iAP2EANativeTransport = initAccIdParams->iAP2iOSAppInfo[i].iAP2EANativeTransport;
                }
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if(rc == IAP2_OK)
        {
            accIdParams->iAP2SupportediOSAppCount = initAccIdParams->iAP2SupportediOSAppCount;

            if(initAccIdParams->iAP2PreferredAppBundleSeedIdentifier != NULL)
            {
                rc = iAP2AllocateSPtr(&accIdParams->iAP2PreferredAppBundleSeedIdentifier, initAccIdParams->iAP2PreferredAppBundleSeedIdentifier);
            }
        }
    }
    return rc;
}

static S32 iAP2InitAccessoryIdentificationParam(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK ;
    U16 i = 0 ;

    iAP2Device_st*       iap2Device        = NULL;
    iAP2AccessoryInfo_t* initAccIdParams   = NULL;
    iAP2AccessoryInfo_t* accIdParams       = NULL;

    iap2Device = (iAP2Device_st*)device;

    initAccIdParams         = iap2InitParam->p_iAP2AccessoryInfo;
    accIdParams             = &(iap2Device->iAP2AccessoryInfo);

    if(NULL != initAccIdParams )
    {
        if(initAccIdParams->iAP2AccessoryName != NULL)
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryName, initAccIdParams->iAP2AccessoryName);
        }

        if( (initAccIdParams->iAP2AccessoryModelIdentifier != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryModelIdentifier, initAccIdParams->iAP2AccessoryModelIdentifier);
        }

        if( (initAccIdParams->iAP2AccessoryManufacturer != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryManufacturer, initAccIdParams->iAP2AccessoryManufacturer);
        }

        if( (initAccIdParams->iAP2AccessorySerialNumber != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessorySerialNumber, initAccIdParams->iAP2AccessorySerialNumber);
        }

        if( (initAccIdParams->iAP2AccessoryFirmwareVersion != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryFirmwareVersion, initAccIdParams->iAP2AccessoryFirmwareVersion);
        }

        if( (initAccIdParams->iAP2AccessoryHardwareVersion != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryHardwareVersion, initAccIdParams->iAP2AccessoryHardwareVersion);
        }

        if( (initAccIdParams->iAP2AccessoryVendorId != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryVendorId, initAccIdParams->iAP2AccessoryVendorId);
        }

        if( (initAccIdParams->iAP2AccessoryProductId != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryProductId, initAccIdParams->iAP2AccessoryProductId);
        }

        if( (initAccIdParams->iAP2AccessoryBcdDevice != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2AccessoryBcdDevice, initAccIdParams->iAP2AccessoryBcdDevice);
        }

        if( (initAccIdParams->iAP2InitEndPoint != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2InitEndPoint, initAccIdParams->iAP2InitEndPoint);
        }

        if(initAccIdParams->iAP2ProductPlanUUID != NULL)
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2ProductPlanUUID, initAccIdParams->iAP2ProductPlanUUID);
        }

        if( (initAccIdParams->iAP2CommandsUsedByApplication != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2CommandsUsedByApplication = (U16*)calloc(1, initAccIdParams->iAP2CommandsUsedByApplication_length);
            if(accIdParams->iAP2CommandsUsedByApplication != NULL)
            {
                memcpy((char*)accIdParams->iAP2CommandsUsedByApplication, initAccIdParams->iAP2CommandsUsedByApplication, initAccIdParams->iAP2CommandsUsedByApplication_length);
                accIdParams->iAP2CommandsUsedByApplication_length = initAccIdParams->iAP2CommandsUsedByApplication_length;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (initAccIdParams->iAP2CallbacksExpectedFromDevice != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2CallbacksExpectedFromDevice = (U16*)calloc(1, initAccIdParams->iAP2CallbacksExpectedFromDevice_length);
            if(accIdParams->iAP2CallbacksExpectedFromDevice != NULL)
            {
                memcpy((char*)accIdParams->iAP2CallbacksExpectedFromDevice, initAccIdParams->iAP2CallbacksExpectedFromDevice, initAccIdParams->iAP2CallbacksExpectedFromDevice_length);
                accIdParams->iAP2CallbacksExpectedFromDevice_length = initAccIdParams->iAP2CallbacksExpectedFromDevice_length;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if(rc == IAP2_OK)
        {
            accIdParams->iAP2MaximumCurrentDrawnFromDevice = initAccIdParams->iAP2MaximumCurrentDrawnFromDevice;
        }

        if( (iap2Device->iAP2AccessoryConfig.iAP2EAPSupported == TRUE) ||
            (iap2Device->iAP2AccessoryConfig.iAP2EANativeTransport == TRUE) )
        {
            if( (initAccIdParams->iAP2SupportediOSAppCount == 0) ||
                (initAccIdParams->iAP2iOSAppInfo == NULL) )
            {
                rc = IAP2_BAD_PARAMETER;
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR,
                                    "iAP2EAPSupported / iAP2EANativeTransport is TRUE, but no iOS App(s) configured DevID:%p",
                                     iap2Device);
            }
        }

        if( (initAccIdParams->iAP2SupportediOSAppCount != 0) ||
            (initAccIdParams->iAP2iOSAppInfo != NULL) )
        {
            if( (iap2Device->iAP2AccessoryConfig.iAP2EAPSupported == FALSE) &&
                (iap2Device->iAP2AccessoryConfig.iAP2EANativeTransport == FALSE) )
            {
                rc = IAP2_BAD_PARAMETER;
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR,
                                    "iOS App(s) configured, but iAP2EANativeTransport / iAP2EAPSupported are FALSE DevID:%p",
                                     iap2Device);
            }
        }

        if( (rc == IAP2_OK) && (initAccIdParams->iAP2SupportediOSAppCount > 0) )
        {
            accIdParams->iAP2iOSAppInfo = calloc(initAccIdParams->iAP2SupportediOSAppCount, sizeof(iAP2iOSAppInfo_t));
            if(accIdParams->iAP2iOSAppInfo != NULL)
            {
                for(i = 0; ( (i < initAccIdParams->iAP2SupportediOSAppCount) && (rc == IAP2_OK) ); i++)
                {
                    rc = iAP2AllocateSPtr(&accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName, initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName);
                    if(rc == IAP2_OK)
                    {
                        accIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                        accIdParams->iAP2iOSAppInfo[i].iAP2EAPMatchAction   = initAccIdParams->iAP2iOSAppInfo[i].iAP2EAPMatchAction;

                        accIdParams->iAP2iOSAppInfo[i].iAP2EANativeTransport = initAccIdParams->iAP2iOSAppInfo[i].iAP2EANativeTransport;
                        accIdParams->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay = initAccIdParams->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay;
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                accIdParams->iAP2SupportediOSAppCount = initAccIdParams->iAP2SupportediOSAppCount;

                if(initAccIdParams->iAP2PreferredAppBundleSeedIdentifier != NULL)
                {
                    rc = iAP2AllocateSPtr(&accIdParams->iAP2PreferredAppBundleSeedIdentifier, initAccIdParams->iAP2PreferredAppBundleSeedIdentifier);
                }
            }
        }

        if( (initAccIdParams->iAP2CurrentLanguage != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateSPtr(&accIdParams->iAP2CurrentLanguage, initAccIdParams->iAP2CurrentLanguage);
        }

        if( (initAccIdParams->iAP2SupportedLanguage != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&accIdParams->iAP2SupportedLanguage,
                                           initAccIdParams->iAP2SupportedLanguage,
                                           &accIdParams->iAP2SupportedLanguageCount,
                                           initAccIdParams->iAP2SupportedLanguageCount, iAP2_utf8, 0);
        }

        if( (initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count > 0) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2USBDeviceSupportedAudioSampleRate = (iAP2USBDeviceModeAudioSampleRate*)calloc(initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count,
                                                                                                           sizeof(iAP2USBDeviceModeAudioSampleRate) );
            if(accIdParams->iAP2USBDeviceSupportedAudioSampleRate != NULL)
            {
                memcpy(accIdParams->iAP2USBDeviceSupportedAudioSampleRate,
                       initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate,
                       (initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count * sizeof(iAP2USBDeviceModeAudioSampleRate) ) );
                accIdParams->iAP2USBDeviceSupportedAudioSampleRate_count = initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        /* Deprecated:
         * Used only for not updated platforms/project.
         * Use iAP2BluetoothTransportComponent instead */
        if(initAccIdParams->iAP2BluetoothTransportMAC != NULL)
        {
            rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothTransportMAC,
                                           initAccIdParams->iAP2BluetoothTransportMAC,
                                           &accIdParams->iAP2BluetoothTransportMAC_count,
                                           initAccIdParams->iAP2BluetoothTransportMAC_count, iAP2_uint64, sizeof(U64));
            IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "Used deprecated BluetoothTransport configuration");
        }

        /* iAP2BluetoothTransportMAC is not used, use iAP2BluetoothTransportComponent */
        if((initAccIdParams->iAP2BluetoothTransportMAC == NULL)
           && (initAccIdParams->iAP2BluetoothTransportComponent != NULL) && (rc == IAP2_OK))
        {
            accIdParams->iAP2BluetoothTransportComponent = calloc(initAccIdParams->iAP2BluetoothTransportComponent_count,
                                                                  sizeof(iAP2BluetoothTransportComponent) );
            if(accIdParams->iAP2BluetoothTransportComponent == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                for(i = 0; ( (i < initAccIdParams->iAP2BluetoothTransportComponent_count) && (rc == IAP2_OK) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                   initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                   &accIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                        initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                        &accIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                        1, iAP2_utf8, 0);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                                       initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress->iAP2BlobData,
                                                       &accIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                                       IAP2_BT_MAC_LENGTH, iAP2_blob, 0);
                    }
                    /* As we care only on the count value for parameter type - none */
                    accIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count = initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count;
                }
            }
            if(rc == IAP2_OK)
            {
                accIdParams->iAP2BluetoothTransportComponent_count = initAccIdParams->iAP2BluetoothTransportComponent_count;
            }
        }

        if( (initAccIdParams->iAP2USBHIDComponent != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2USBHIDComponent = (iAP2iAP2HIDComponent*)calloc(initAccIdParams->iAP2USBHIDComponent_count, sizeof(iAP2iAP2HIDComponent));
            if(accIdParams->iAP2USBHIDComponent != NULL)
            {
                for(i=0; ((i<initAccIdParams->iAP2USBHIDComponent_count) && (rc == IAP2_OK)); i++)
                {
                    rc = iAP2AllocateandUpdateData(&accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName,
                                                   initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName,
                                                   &accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName_count,
                                                   initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName_count, iAP2_utf8, 0);
                    if(rc == IAP2_OK)
                    {
                        accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                        if(accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction != NULL)
                        {
                            *(accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction) = *(initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction);
                            accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier,
                                                       initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier,
                                                       &accIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                       initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier_count, iAP2_uint16, sizeof(U16));
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                accIdParams->iAP2USBHIDComponent_count = initAccIdParams->iAP2USBHIDComponent_count;
            }
        }

        if( (initAccIdParams->iAP2VehicleInformationComponent != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2VehicleInformationComponent = (iAP2VehicleInformationComponent_t*)calloc(1, sizeof(iAP2VehicleInformationComponent_t));
            if(accIdParams->iAP2VehicleInformationComponent == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if( (initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType != NULL) &&
                (initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count > 0) &&
                (rc == IAP2_OK) )
            {
                accIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count = initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count;
                accIdParams->iAP2VehicleInformationComponent->iAP2EngineType = (iAP2EngineTypes*)calloc(initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count,
                                                                                                        sizeof(iAP2EngineTypes));
                if(accIdParams->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
                {
                    for(i = 0; i < initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count; i++)
                    {
                        memcpy(&accIdParams->iAP2VehicleInformationComponent->iAP2EngineType[i],
                               &initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType[i],
                               sizeof(iAP2EngineTypes));
                    }
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
            if( (initAccIdParams->iAP2VehicleInformationComponent->iAP2DisplayName != NULL) && (rc == IAP2_OK) )
            {
                rc = iAP2AllocateSPtr(&accIdParams->iAP2VehicleInformationComponent->iAP2DisplayName, initAccIdParams->iAP2VehicleInformationComponent->iAP2DisplayName);
            }
            if( (initAccIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName != NULL) && (rc == IAP2_OK) )
            {
                rc = iAP2AllocateSPtr(&accIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName, initAccIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName);
            }
        }

        if( (initAccIdParams->iAP2VehicleStatusComponent != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2VehicleStatusComponent = (iAP2VehicleStatusComponent_t*)calloc(1, sizeof(iAP2VehicleStatusComponent_t));
            if(accIdParams->iAP2VehicleStatusComponent != NULL)
            {
                memcpy(accIdParams->iAP2VehicleStatusComponent, initAccIdParams->iAP2VehicleStatusComponent, sizeof(iAP2VehicleStatusComponent_t));
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (initAccIdParams->iAP2LocationInformationComponent != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2LocationInformationComponent = (iAP2LocationInformationComponent_t*)calloc(1, sizeof(iAP2LocationInformationComponent_t));
            if(accIdParams->iAP2LocationInformationComponent != NULL)
            {
                memcpy(accIdParams->iAP2LocationInformationComponent, initAccIdParams->iAP2LocationInformationComponent, sizeof(iAP2LocationInformationComponent_t));
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if(rc == IAP2_OK)
        {
            accIdParams->iAP2SupportsiOSintheCar   = initAccIdParams->iAP2SupportsiOSintheCar;
        }

        if( (initAccIdParams->iAP2USBHostHIDComponent != NULL) && (rc == IAP2_OK) )
        {
            accIdParams->iAP2USBHostHIDComponent = (iAP2USBHostHIDComponent*)calloc(initAccIdParams->iAP2USBHostHIDComponent_count, sizeof(iAP2USBHostHIDComponent));
            if(accIdParams->iAP2USBHostHIDComponent != NULL)
            {
                for(i=0; ((i<initAccIdParams->iAP2USBHostHIDComponent_count) && (rc == IAP2_OK)); i++)
                {
                    rc = iAP2AllocateandUpdateData(&accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentName,
                                                   initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentName,
                                                   &accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentName_count,
                                                   1, iAP2_utf8, 0);
                    if(rc == IAP2_OK)
                    {
                        accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                        if(accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction != NULL)
                        {
                            *(accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction) = *(initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction);
                            accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier,
                                                       initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier,
                                                       &accIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&accIdParams->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber ,
                                                       initAccIdParams->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber,
                                                       &accIdParams->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }

            if(rc == IAP2_OK)
            {
                accIdParams->iAP2USBHostHIDComponent_count = initAccIdParams->iAP2USBHostHIDComponent_count;
            }
        }

         if( (initAccIdParams->iAP2BluetoothHIDComponent != NULL) && (rc == IAP2_OK) )
         {
             accIdParams->iAP2BluetoothHIDComponent = (iAP2BluetoothHIDComponent*)calloc(1, sizeof(iAP2BluetoothHIDComponent));
             if(accIdParams->iAP2BluetoothHIDComponent != NULL)
             {
                 rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentName,
                                                initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentName,
                                                &accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentName_count,
                                                1, iAP2_utf8, 0);
                  if(rc == IAP2_OK)
                  {
                      accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                      if(accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction != NULL)
                      {
                          *(accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction) = *(initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction);
                          accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction_count++;
                      }
                      else
                      {
                          rc = IAP2_ERR_NO_MEM;
                      }
                  }
                  if(rc == IAP2_OK)
                  {
                      rc = iAP2AllocateandUpdateData(&accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier,
                                                     initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier,
                                                     &accIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier_count,
                                                     1, iAP2_uint16, sizeof(U16));
                  }
             }
             else
             {
                 rc = IAP2_ERR_NO_MEM;
             }
         }

         if( (initAccIdParams->iAP2WirelessCarPlayTransportComponent != NULL) && (rc == IAP2_OK) )
         {
             accIdParams->iAP2WirelessCarPlayTransportComponent = (iAP2WirelessCarPlayTransportComponent*)calloc(initAccIdParams->iAP2WirelessCarPlayTransportComponent_count, sizeof(iAP2WirelessCarPlayTransportComponent));
             if(accIdParams->iAP2WirelessCarPlayTransportComponent != NULL)
             {
                 for(i=0; ((i<initAccIdParams->iAP2WirelessCarPlayTransportComponent_count) && (rc == IAP2_OK)); i++)
                 {
                     rc = iAP2AllocateandUpdateData(&accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier, initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier,
                                                    &accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier_count, 1, iAP2_uint16, sizeof(U16));
                     if(rc == IAP2_OK)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName, initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName,
                                                        &accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName_count, 1, iAP2_utf8, 0);
                     }

                     if(rc == IAP2_OK)
                     {
                         accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsCarPlay_count++;
                         accIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                     }
                 }
             }
             else
             {
                 rc = IAP2_ERR_NO_MEM;
             }
             if(rc == IAP2_OK)
             {
                 accIdParams->iAP2WirelessCarPlayTransportComponent_count = initAccIdParams->iAP2WirelessCarPlayTransportComponent_count;
             }
         }

         if( (initAccIdParams->iAP2RouteGuidanceDisplayComponent != NULL) && (rc == IAP2_OK) )
         {
             accIdParams->iAP2RouteGuidanceDisplayComponent = (iAP2RouteGuidanceDisplayComponent*)calloc(initAccIdParams->iAP2RouteGuidanceDisplayComponent_count, sizeof(iAP2RouteGuidanceDisplayComponent));
             if(accIdParams->iAP2RouteGuidanceDisplayComponent != NULL)
             {
                 for(i=0; ((i<initAccIdParams->iAP2RouteGuidanceDisplayComponent_count) && (rc == IAP2_OK)); i++)
                 {
                     rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                                    initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                                    &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count,
                                                    1, iAP2_utf8, 0);
                     if(rc == IAP2_OK)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if(rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if(rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if(rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if(rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if(rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if (rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                        initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                        &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count,
                                                        1, iAP2_uint16, sizeof(U16));
                     }
                     if (rc == IAP2_OK && initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count == 1)
                     {
                         rc = iAP2AllocateandUpdateData(&accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                         initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                         &accIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count,
                                                         1, iAP2_uint16, sizeof(U16));
                     }
                 }
             }
             else
             {
                 rc = IAP2_ERR_NO_MEM;
             }
             if(rc == IAP2_OK)
             {
                 accIdParams->iAP2RouteGuidanceDisplayComponent_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent_count;
             }
         }

    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL, "Invalid Parameter DevID:%p", iap2Device);
        rc = IAP2_BAD_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed  rc:%d, DevID:%p", rc, iap2Device);
    }

    return rc;
}

static S32 iAP2DeInitAccessoryConfigParam(iAP2AccessoryConfig_t* aConfig)
{
    S32 rc = IAP2_OK;

    if (NULL != aConfig)
    {
        memset(aConfig, 0, sizeof(iAP2AccessoryConfig_t));
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed  rc:%d", rc);
    }

    return rc;
}


static S32 iAP2InitAccessoryConfigParam(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    /* Initialize power update  : temp */
    rc = iAP2InitializePowerUpdates(iap2Device, iap2InitParam);

    if(rc != IAP2_OK)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed rc:%d DevID:%p", rc, iap2Device);
    }

    return rc;
}

static S32 iAP2DeRegisterControlSessionCallbacks(iAP2SessionCallbacks_t*  dCS_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dCS_cb )
    {
        memset(dCS_cb, 0, sizeof(iAP2SessionCallbacks_t));
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL, "Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }


    return rc;
}

static S32 iAP2RegisterControlSessionCallbacks( iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*          iap2Device  = NULL;
    iAP2SessionCallbacks_t* initCS_cb   = NULL;
    iAP2SessionCallbacks_t* dCS_cb      = NULL;

    iap2Device  = (iAP2Device_st*)device;
    initCS_cb   = iap2InitParam->p_iAP2CSCallbacks;
    dCS_cb      = &(iap2Device->iAP2CSCallbacks);

    if(NULL != initCS_cb )
    {
        memcpy(dCS_cb, initCS_cb, sizeof(iAP2SessionCallbacks_t) );
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }


    return rc;
}

static S32 iAP2DeRegisterFileTransferSessionCallbacks(iAP2FileTransferCallbacks_t*  dFileTransfer_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dFileTransfer_cb )
    {
        memset(dFileTransfer_cb, 0, sizeof(iAP2FileTransferCallbacks_t));
    }

    return rc;
}

static S32 iAP2RegisterFileTransferSessionCallbacks( iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*  iap2Device = NULL;
    iAP2FileTransferCallbacks_t*  initFileTransfer_cb     = NULL;
    iAP2FileTransferCallbacks_t*  dFileTransfer_cb        = NULL;

    iap2Device = (iAP2Device_st*)device;
    initFileTransfer_cb   = iap2InitParam->p_iAP2FileTransferCallbacks;
    dFileTransfer_cb      = &(iap2Device->iAP2FileTransferCallbacks);
    if(NULL != initFileTransfer_cb)
    {
        memcpy(dFileTransfer_cb, initFileTransfer_cb, sizeof(iAP2FileTransferCallbacks_t) );
    }

    return rc;
}

static S32 iAP2DeRegisterEAPSessionCallbacks(iAP2EAPSessionCallbacks_t*  dEAP_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dEAP_cb)
    {
        memset(dEAP_cb, 0, sizeof(iAP2EAPSessionCallbacks_t));
    }

    return rc;
}

static S32 iAP2RegisterEAPSessionCallbacks(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*             iap2Device  = NULL;
    iAP2EAPSessionCallbacks_t* initEAP_cb   = NULL;
    iAP2EAPSessionCallbacks_t* dEAP_cb      = NULL;

    iap2Device = (iAP2Device_st*)device;
    initEAP_cb = iap2InitParam->p_iAP2EAPSessionCallbacks;
    dEAP_cb    = &(iap2Device->iAP2EAPSessionCallbacks);

    if(NULL != initEAP_cb)
    {
        memcpy(dEAP_cb, initEAP_cb, sizeof(iAP2EAPSessionCallbacks_t) );
    }

    return rc;
}

static S32 iAP2DeRegisterMultiEAPSessionCallbacks(iAP2MultiEAPSessionCallbacks_t*  dEAP_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dEAP_cb)
    {
        memset(dEAP_cb, 0, sizeof(iAP2MultiEAPSessionCallbacks_t));
    }

    return rc;
}

static S32 iAP2RegisterMultiEAPSessionCallbacks(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*             iap2Device  = NULL;
    iAP2MultiEAPSessionCallbacks_t* initEAP_cb   = NULL;
    iAP2MultiEAPSessionCallbacks_t* dEAP_cb      = NULL;

    iap2Device = (iAP2Device_st*)device;
    initEAP_cb = iap2InitParam->p_iAP2MultiEAPSessionCallbacks;
    dEAP_cb    = &(iap2Device->iAP2MultiEAPSessionCallbacks);

    if(NULL != initEAP_cb)
    {
        memcpy(dEAP_cb, initEAP_cb, sizeof(iAP2MultiEAPSessionCallbacks_t) );
    }

    return rc;
}

static S32 iAP2DeRegisterEANativeTransportCallbacks(iAP2EANativeTransportCallbacks_t*  dEAnt_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dEAnt_cb)
    {
        memset(dEAnt_cb, 0, sizeof(iAP2EANativeTransportCallbacks_t));
    }

    return rc;
}

static S32 iAP2RegisterEANativeTransportCallbacks(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*             iap2Device  = NULL;
    iAP2EANativeTransportCallbacks_t* initEAnt_cb   = NULL;
    iAP2EANativeTransportCallbacks_t* dEAnt_cb      = NULL;

    iap2Device = (iAP2Device_st*)device;
    initEAnt_cb = iap2InitParam->p_iAP2EANativeTransportCallbacks;
    dEAnt_cb    = &(iap2Device->p_iAP2EANativeTransportCallbacks);

    if(NULL != initEAnt_cb)
    {
        memcpy(dEAnt_cb, initEAnt_cb, sizeof(iAP2EANativeTransportCallbacks_t) );
    }

    return rc;
}

static S32 iAP2DeRegisterIAP2StackCallbacks(iAP2StackCallbacks_t*  dStack_cb)
{
    S32 rc = IAP2_OK;

    if(NULL != dStack_cb )
    {
        memset(dStack_cb, 0, sizeof(*dStack_cb));
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL, "Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2RegisterIAP2StackCallbacks(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;
    iAP2Device_st*  iap2Device = NULL;

    iAP2StackCallbacks_t*  iStack_cb     = NULL;
    iAP2StackCallbacks_t*  dStack_cb     = NULL;

    iap2Device = (iAP2Device_st*)device;
    iStack_cb  = iap2InitParam->p_iAP2StackCallbacks;
    dStack_cb  = &(iap2Device->iAP2StackCallbacks);

    if(NULL != iStack_cb )
    {
        *dStack_cb = *iStack_cb;
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Parameter DevID:%p",iap2Device);
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

static S32 iAP2LinkDeInit(iAP2Device_t *device)
{
    S32             rc          = IAP2_OK;
    iAP2Device_st*  iap2Device  = NULL;

    iap2Device = (iAP2Device_st*)device;

    if(NULL != iap2Device->p_iAP2AccessoryLink)
    {
        /* Check whether an unprocessed/incomplete RecvPacket is present */
        if(iap2Device->iAP2CreateLinkPacket == 0)
        {
            iAP2PacketDelete(iap2Device->p_iAP2LinkPacket); /* Delete such packet */
        }
        /*This will trigger the callback iAP2AccessoryLinkConnected_CB() with bConnected flag set to FALSE*/
        iAP2LinkRunLoopDetached(iap2Device->p_iAP2AccessoryLink);

        iAP2LinkRunLoopDelete(iap2Device->p_iAP2AccessoryLink);

        iap2Device->p_iAP2AccessoryLink = NULL;
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "Invalid iAP2AccesoryLink DevID:%p", iap2Device);
    }

    return rc;
}

static S32 iAP2LinkInit(iAP2Device_t *device)
{
    S32     rc                  = IAP2_CTL_ERROR;
    BOOL    bValidateSYN        = TRUE ;
    U8      maxPacketSentAtOnce = 128;

    iAP2Device_st*        iap2Device = NULL;
    iAP2PacketSYNData_t   syncParam;

    iap2Device = (iAP2Device_st*)device;

    iAP2ConfigLinkSyncPayload(iap2Device, &syncParam);

    iap2Device->p_iAP2AccessoryLink = NULL;
    iap2Device->p_iAP2AccessoryLink = (iAP2LinkRunLoop_t*)iAP2LinkRunLoopCreateAccessory(&syncParam,
                                                                                         (void*)iap2Device,
                                                                                         iAP2LinkSendPacket_CB,
                                                                                         iap2Device->iAP2Link->recv,
                                                                                         iAP2LinkConnected_CB,
                                                                                         iAP2LinkSendDetect_CB,
                                                                                         bValidateSYN,
                                                                                         maxPacketSentAtOnce,
                                                                                         NULL);

    if( NULL != iap2Device->p_iAP2AccessoryLink)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "Create accessory link success. DevID:%p", iap2Device);
        rc = IAP2_OK;
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "Create accessory link failed. DevID:%p", iap2Device);
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

static S32 iAP2InitializePowerUpdates(iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    S32 rc = IAP2_OK;

    iAP2Device_st*         iap2Device    = NULL;
    iAP2AccessoryConfig_t* initPowerConf = NULL;
    iAP2AccessoryConfig_t* accPowerConf  = NULL;

    iap2Device = (iAP2Device_st*)device;
    initPowerConf = iap2InitParam->p_iAP2AccessoryConfig;
    accPowerConf  = &(iap2Device->iAP2AccessoryConfig) ;

    if(NULL != initPowerConf)
    {
        accPowerConf->iAP2AvailableCurrentForDevice                 = initPowerConf->iAP2AvailableCurrentForDevice;
        accPowerConf->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = initPowerConf->iAP2DeviceBatteryShouldChargeIfPowerIsPresent;
        accPowerConf->iAP2MaximumcurrentDrawnFromAccessory          = initPowerConf->iAP2MaximumcurrentDrawnFromAccessory;
        accPowerConf->iAP2DeviceBatteryWillChargeIfPowerIsPresent   = initPowerConf->iAP2DeviceBatteryWillChargeIfPowerIsPresent;
        accPowerConf->iAP2AccessoryPowerMode                        = initPowerConf->iAP2AccessoryPowerMode;
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

S32 iAP2GetPollFDs(iAP2Device_t* device, iAP2GetPollFDs_t* getPollFDs)
{
    S32 rc = IAP2_OK;
    S32 i = 0;
    S32 totalNumFDs = 0;
    iAP2Transport_t* transport = NULL;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    if(iap2Device != NULL)
    {
        transport = &(iap2Device->iAP2Transport);
        if(transport->iAP2TransportHdl != NULL)
        {
            if(iap2Device->iAP2Transport.iAP2PollFds.fds != NULL){
                iAP2FreePointer( (void**)&iap2Device->iAP2Transport.iAP2PollFds.fds);
                iap2Device->iAP2Transport.iAP2PollFds.numFDs = 0;
            }

            rc = transport->iAP2DataComFunction.getfds(transport->iAP2TransportHdl,
                                                       NULL,
                                                       &(iap2Device->iAP2Transport.iAP2PollFds.numFDs));
            if((rc == IAP2_OK) && (iap2Device->iAP2Transport.iAP2PollFds.numFDs > 0))
            {
                totalNumFDs = iap2Device->iAP2Transport.iAP2PollFds.numFDs;
                /* one more for the timerfd */
                totalNumFDs++;
                /* get libusb fds */
                iap2Device->iAP2Transport.iAP2PollFds.fds = (IPOD_IAP2_DATACOM_FD*)malloc(totalNumFDs*sizeof(IPOD_IAP2_DATACOM_FD));
                if(iap2Device->iAP2Transport.iAP2PollFds.fds != NULL)
                {
                    /* get only libusb fds so did not use totalNumFDs */
                    rc = transport->iAP2DataComFunction.getfds(transport->iAP2TransportHdl,
                                                               iap2Device->iAP2Transport.iAP2PollFds.fds,
                                                               &(iap2Device->iAP2Transport.iAP2PollFds.numFDs));
                    if(rc == IAP2_OK)
                    {
                        /* add timerfd */
                        i = iap2Device->iAP2Transport.iAP2PollFds.numFDs;
                        iap2Device->iAP2Transport.iAP2PollFds.fds[i].fd = iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd;
                        iap2Device->iAP2Transport.iAP2PollFds.fds[i].event = POLLIN;
                        iap2Device->iAP2Transport.iAP2PollFds.numFDs = totalNumFDs;
                        memcpy(getPollFDs, &(iap2Device->iAP2Transport.iAP2PollFds), sizeof(iap2Device->iAP2Transport.iAP2PollFds));
                        rc = IAP2_OK;
                    }
                }
                else
                {
                    rc = IAP2_CTL_ERROR;
                }
            }
            else
            {
                rc = IAP2_CTL_ERROR;
            }
        }
        else
        {
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid device DevID:%p", iap2Device);
    }

    return rc;
}

S32 iAP2HandleEvent(iAP2Device_t* device, S32 fd, S16 event)
{
    S32  rc = IAP2_OK;
    BOOL bDetect;
    U64  exp;
    U32 parsedLen = 0;

    iAP2Device_st*     iap2Device  = NULL;
    iAP2Transport_t*   transport   = NULL;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;

    /*Avoid lint warning*/
    event = event;

    iap2Device = (iAP2Device_st*)device;

    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Called  DevID:%p", iap2Device);
    if(fd == iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Called for timer fd DevID:%p", iap2Device);
        /* timerfd was triggered.
         * read the number of expirations that have occurred. */
        rc = read(iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd, &exp, sizeof(U64));
        if (rc < 0)
        {
            /* Read() fail if the timerfd was triggered,
             * but in the meantime disarmed. */
            IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "iAP2RunLoopTimerFd triggered, but read failed. DevID:%p", iap2Device);
        }
        rc = IAP2_OK;

        if((iap2Device->iAP2RunLoop.iAP2RunLoopTimerCallback != NULL)&&
           (iap2Device->iAP2RunLoop.iAP2RunLoopTimer != NULL))
        {
            U32 curTime = iAP2TimeGetCurTimeMs();
            ((iAP2TimeCB_t)iap2Device->iAP2RunLoop.iAP2RunLoopTimerCallback)(iap2Device->iAP2RunLoop.iAP2RunLoopTimer, curTime);
        }
    }
    else
    {
        transport = &(iap2Device->iAP2Transport);
        if(transport->iAP2TransportHdl != NULL)
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Called for read fd DevID:%p", iap2Device);
            /* handle libusb event and receive data */
            rc = transport->iAP2DataComFunction.hdlevent(transport->iAP2TransportHdl,
                                                         kiAP2PacketLenMax,
                                                         iap2Device->iAP2LinkRxBuf, fd);
            if(rc > 0)
            {
                iap2Device->iAP2LinkRxLen = rc;
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "read %d bytes", rc);

                rc = IAP2_OK;

                parsedLen = 0;
                while((rc == IAP2_OK) && parsedLen < iap2Device->iAP2LinkRxLen)
                {
                    linkRunLoop = (iAP2LinkRunLoop_t*)iap2Device->p_iAP2AccessoryLink;
                    if(NULL != linkRunLoop)
                    {
                        if(iap2Device->iAP2CreateLinkPacket == 1)
                        {
                            iap2Device->p_iAP2LinkPacket = (iAP2Packet_t*)iAP2PacketCreateEmptyRecvPacket(linkRunLoop->link);
                            iap2Device->iAP2CreateLinkPacket = 0;
                        }

                        if(iap2Device->p_iAP2LinkPacket == NULL)
                        {
                            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed to get iap2Device->p_iAP2LinkPacket  DevID:%p", iap2Device);
                            rc = IAP2_CTL_ERROR;
                        }
                        else
                        {
                            parsedLen += iAP2PacketParseBuffer(iap2Device->iAP2LinkRxBuf+parsedLen,
                                                                iap2Device->iAP2LinkRxLen-parsedLen,
                                                                iap2Device->p_iAP2LinkPacket,
                                                                ( iAP2LinkGetMaxRecvPayloadSize (linkRunLoop->link) + kIAP2PacketHeaderLen + kIAP2PacketChksumLen ),
                                                                &bDetect, /* Set to TRUE if iAP 1.0/2.0 detect or detect BAD ACK packet */
                                                                NULL,
                                                                NULL);

                            if(TRUE == iAP2PacketIsComplete(iap2Device->p_iAP2LinkPacket))
                            {
                                iap2Device->iAP2CreateLinkPacket = 1;
                                /* instead of calling iAP2LinkRunLoopHandleReadyPacket
                                 * call iAP2LinkRunLoopRunOnce directly, since internally iAP2LinkRunLoopHandleReadyPacket
                                 * only does signaling to call iAP2LinkRunLoopRunOnce
                                 */
                                rc = iAP2CallRunLoop (iap2Device, iap2Device->p_iAP2LinkPacket);  /* returns BOOL */
                            }
                        }
                    }
                    else
                    {
                        rc = IAP2_INVALID_INPUT_PARAMETER;
                        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "linkRunLoop is NULL  DevID:%p", iap2Device);
                    }
                }
            }
            else if(rc == IPOD_DATACOM_NOT_CONNECTED)
            {
                rc = IAP2_DEV_NOT_CONNECTED;
                IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "hdlevent() failed.IPOD_DATACOM_NOT_CONNECTED  DevID:%p", iap2Device);
                iap2Device->iAP2DeviceState = iAP2NotConnected;
                (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2NotConnected, iap2Device->iAP2ContextCallback);
            }
            else if(IAP2_OK > rc)
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "hdlevent(): Unknown Error From Plug-in. rc:%d  DevID:%p",rc, iap2Device);
            }
            else
            {
            }
        }
        else
        {
            rc = IAP2_CTL_ERROR;
        }
    }
    while((iap2Device->iAP2RunLoop.iAP2RunLoopCall == 1)&&(rc == IAP2_OK))
    {
        void* runLoopArg = iap2Device->iAP2RunLoop.iAP2RunLoopCallArg;
        iap2Device->iAP2RunLoop.iAP2RunLoopCall = 0;
        iap2Device->iAP2RunLoop.iAP2RunLoopCallArg = NULL;

        rc = iAP2CallRunLoop (iap2Device, runLoopArg);  /* returns BOOL */
    }

    return rc;
}

static S32 iAP2DeinitDeviceStrucutrePrivate(iAP2Service_t* service, iAP2Device_t* device)
{
    S32  rc  = IAP2_CTL_ERROR;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    if (NULL != iap2Device)
    {
        if(NULL == service) /*Transport is initialized only by iAP2Service & Library Model*/
        {
            /* free and release DataComPlugin */
            rc = iAP2TransportDeInit(&(iap2Device->iAP2Transport));

            /* Free and release information parameters and configuration values */
            rc = iAP2DeInitAuthenticationParam(iap2Device);
            rc = iAP2DeInitAccessoryConfigParam(&iap2Device->iAP2AccessoryConfig);
            rc = iAP2DeInitAccessoryIdentificationParam(&iap2Device->iAP2AccessoryInfo);

            iAP2CleanupFileTransferSession(iap2Device);

            iap2Device->iAP2RunLoop.iAP2RunLoopCall = 0;
            iap2Device->iAP2RunLoop.iAP2RunLoopCallArg = NULL;
        }

        /* de-register callbacks */
        rc = iAP2DeRegisterControlSessionCallbacks(&(iap2Device->iAP2CSCallbacks));
        rc = iAP2DeRegisterFileTransferSessionCallbacks(&(iap2Device->iAP2FileTransferCallbacks));
        rc = iAP2DeRegisterEAPSessionCallbacks(&(iap2Device->iAP2EAPSessionCallbacks));
        rc = iAP2DeRegisterMultiEAPSessionCallbacks(&(iap2Device->iAP2MultiEAPSessionCallbacks));
        rc = iAP2DeRegisterEANativeTransportCallbacks(&(iap2Device->p_iAP2EANativeTransportCallbacks));
        rc = iAP2DeRegisterIAP2StackCallbacks(&(iap2Device->iAP2StackCallbacks));

        /* Free iAP2 Buffer Pool */
        iAP2FreeBufferPool(iap2Device->iAP2BufferPool);

        /* Now we can free the device */
        iAP2FreePointer( (void**)&iap2Device);
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid device DevID:%p", iap2Device);
    }

    IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "rc:%d DevID:%p",rc, iap2Device);
    return rc;
}

/*iAP2Service model - called by applications*/
S32 iAP2ServiceDeInitDeviceStructure(iAP2Service_t* service, iAP2Device_t* device)
{
    return iAP2DeinitDeviceStrucutrePrivate(service, device);
}

/*iAP2 Library model - called by iAP2Service & applications*/
S32 iAP2DeInitDeviceStructure(iAP2Device_t* device)
{
    return iAP2DeinitDeviceStrucutrePrivate(NULL, device);
}

static iAP2Device_t* iAP2InitDeviceStructurePrivate(iAP2Service_t* service, uint32_t deviceId, iAP2InitParam_t* iap2InitParam, iAP2LinkCallbacks_t* linkCB)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2Device_st* iap2Device = NULL;

    if(NULL != iap2InitParam)
    {
        /*
         * Allocate memory for the device structure
         */
        iap2Device = calloc(1, sizeof(iAP2Device_st));
        if(NULL != iap2Device)
        {
            /* Initialize device communication state and error state */
            iap2Device->iAP2DeviceState      = iAP2NotConnected;
            iap2Device->iAP2DeviceErrState   = iAP2NoError;
            iap2Device->iAP2DeviceVerified = 0;

            iap2Device->iAP2RunLoop.iAP2RunLoopCall = 0;
            iap2Device->iAP2RunLoop.iAP2RunLoopCallArg = NULL;

            iap2Device->p_iAP2LinkPacket = NULL;
            iap2Device->iAP2CreateLinkPacket = 1;

            iap2Device->iAP2Link = (iAP2LinkCallbacks_t*)calloc(1, sizeof(iAP2LinkCallbacks_t));
            if(iap2Device->iAP2Link)
            {
                *iap2Device->iAP2Link = *linkCB;
                rc = IAP2_OK;
            }

            iap2Device->iAP2ContextCallback = iap2InitParam->iAP2ContextCallback;

            iap2Device->iAP2DeviceId = deviceId; /*device Id assigned by iAP2 server*/
            iap2Device->iAP2Service = service; /*Applications will send message to iAP2Server*/

            if(service == NULL) /*iAP2Service process & iAP2Library model*/
            {
                /* Initialize transport plug-in */
                rc = iAP2TransportInit(iap2Device, iap2InitParam);

                if(rc == IAP2_OK)
                {
                    /*Initialize authentication details */
                    rc = iAP2InitAuthenticationParam(iap2Device, iap2InitParam);
                }
                if(rc == IAP2_OK)
                {
                    /*Set accessory configuration values */
                    rc = iAP2InitAccessoryConfigParam(iap2Device, iap2InitParam);
                }

                if(rc == IAP2_OK)
                {
                    iap2Device->iAP2AccessoryConfig.ManualLinkConfig            = iap2InitParam->p_iAP2AccessoryConfig->ManualLinkConfig;
                    iap2Device->iAP2AccessoryConfig.LinkConfig_SessionVersion   = iap2InitParam->p_iAP2AccessoryConfig->LinkConfig_SessionVersion;

                    iap2Device->iAP2AccessoryConfig.iAP2EAPSupported      = iap2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported;
                    iap2Device->iAP2AccessoryConfig.iAP2EANativeTransport = iap2InitParam->p_iAP2AccessoryConfig->iAP2EANativeTransport;

                    iap2Device->iAP2AccessoryConfig.FileTransferConfig       = iap2InitParam->p_iAP2AccessoryConfig->FileTransferConfig;
                    iap2Device->iAP2AccessoryConfig.FileTransfer_SessionVersion = iap2InitParam->p_iAP2AccessoryConfig->FileTransfer_SessionVersion;
                }
                if(rc == IAP2_OK)
                {
                    /* Fill Accessory ID Information */
                    rc = iAP2InitAccessoryIdentificationParam(iap2Device, iap2InitParam);
                }

                if(rc == IAP2_OK)
                {
                    iap2Device->iAP2AccessoryConfig.iAP2FileXferSupported = iap2InitParam->p_iAP2AccessoryConfig->iAP2FileXferSupported;

                    if(iap2InitParam->p_iAP2AccessoryConfig->iAP2FileXferRcvAsStream == TRUE)
                    {
                        iap2Device->iAP2AccessoryConfig.iAP2FileXferRcvAsStream = TRUE;
                    }
                    else
                    {
                        iap2Device->iAP2AccessoryConfig.iAP2FileXferRcvAsStream = FALSE;
                    }
                }
            }
            else /*iAP2Service model - called by applications*/
            {
                /* Fill Accessory ID Information */
                rc = iAP2ServiceInitAccessoryIdentificationParam(iap2Device, iap2InitParam);
            }

            if(rc == IAP2_OK)
            {
                /* Initialize Control session callbacks */
                rc = iAP2RegisterControlSessionCallbacks(iap2Device, iap2InitParam);
            }
            if(rc == IAP2_OK)
            {
                /* Initialize Filetransfer session callbacks */
                rc = iAP2RegisterFileTransferSessionCallbacks(iap2Device, iap2InitParam);
            }
            if(rc == IAP2_OK)
            {
                /* Initialize External Accessory Protocol session callbacks */
                rc = iAP2RegisterEAPSessionCallbacks(iap2Device, iap2InitParam);
            }
            if(rc == IAP2_OK)
            {
                /* Initialize External Accessory Protocol session callbacks for Multiple EAP session mode*/
                rc = iAP2RegisterMultiEAPSessionCallbacks(iap2Device, iap2InitParam);
            }
            if(rc == IAP2_OK)
            {
                /* Initialize External Accessory Native Transport callbacks */
                rc = iAP2RegisterEANativeTransportCallbacks(iap2Device, iap2InitParam);
            }
            if(rc == IAP2_OK)
            {
                /* Initialize iap2 stack callbacks */
                rc = iAP2RegisterIAP2StackCallbacks(iap2Device, iap2InitParam);
            }

            if(rc == IAP2_OK)
            {
                /* Allocate iAP2 Buffer Pool */
                rc = iAP2CreateBufferPool(&iap2Device->iAP2BufferPool);
            }

            if(rc != IAP2_OK)
            {
                /* clean-up and release all settings and allocations */
                (void)iAP2DeInitDeviceStructure((iAP2Device_t*)iap2Device);
                /* iap2Device freed at iAP2DeInitDeviceStructure(),
                 * but must be set to NULL to indicate an error to the Application.
                 */
                iap2Device = NULL;
            }
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Out of memory");
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "DevID:%p", iap2Device);
    return (iAP2Device_t*)iap2Device;

}

/* iAP2Service model - called by applications*/
iAP2Device_t* iAP2ServiceInitDeviceStructure(iAP2Service_t* service, uint32_t deviceId, iAP2InitParam_t* iap2InitParam)
{
    iAP2LinkCallbacks_t link_cb;
    link_cb.recv = &iAP2ServiceLinkDataReady_CB;
    link_cb.send = &iAP2ServiceSendMsgToLink;

    return iAP2InitDeviceStructurePrivate(service, deviceId, iap2InitParam, &link_cb);
}

/* iAP2Service process*/
iAP2Device_t* iAP2InitDeviceStructureService(iAP2InitParam_t* iap2InitParam)
{
    iAP2LinkCallbacks_t link_cb;
    link_cb.recv = &iAP2LinkDataReadyService_CB;
    link_cb.send = &iAP2SendMsgToLink;

    return iAP2InitDeviceStructurePrivate(NULL, 0, iap2InitParam, &link_cb);
}

/* iAP2 as Library model*/
iAP2Device_t* iAP2InitDeviceStructure( iAP2InitParam_t* iap2InitParam)
{
    iAP2LinkCallbacks_t link_cb;
    link_cb.recv = &iAP2LinkDataReady_CB;
    link_cb.send = &iAP2SendMsgToLink;

    return iAP2InitDeviceStructurePrivate(NULL, 0, iap2InitParam, &link_cb);
}

S32 iAP2DisconnectDevice(iAP2Device_t* device)
{
    S32 rc = IAP2_OK;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;
    if (NULL != iap2Device) //CR: check may not be required
    {
        /*De-Initialize Link*/
        rc = iAP2LinkDeInit(iap2Device);

        rc = iAP2DeviceClose(&(iap2Device->iAP2Transport));

        /* close timerfd */
        close(iap2Device->iAP2RunLoop.iAP2RunLoopTimerFd);

        /* free iAP2PollFds->fds */
        if(iap2Device->iAP2Transport.iAP2PollFds.fds != NULL)
        {
            iAP2FreePointer( (void**)&iap2Device->iAP2Transport.iAP2PollFds.fds);
            iap2Device->iAP2Transport.iAP2PollFds.numFDs = 0;
        }

        /*reset the device error state and connection state */
        iap2Device->iAP2DeviceState = iAP2NotConnected;
        (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2NotConnected, iap2Device->iAP2ContextCallback);
        iap2Device->iAP2DeviceErrState = iAP2NoError;

        if(rc == IAP2_OK)
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "DisconnectDevice SUCCESS DevID:%p", iap2Device);
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "DisconnectDevice FAILED DevID:%p", iap2Device);
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Device Handle DevID:%p", iap2Device);
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}

S32 iAP2InitDeviceConnection(iAP2Device_t* device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2Device_st*  iap2Device = NULL;

    iap2Device = (iAP2Device_st*)device;

    if (NULL != iap2Device)
    {
        /* set device configuration */
        rc = iAP2DeviceIoCtl(&(iap2Device->iAP2Transport));

        if(rc == IAP2_OK)
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "iAP2DeviceIoCtl SUCCESS DevID:%p", iap2Device);

            /*Open the device and get a device descriptor */
            rc = iAP2DeviceOpen(&(iap2Device->iAP2Transport));
        }

        if(rc == IAP2_OK)
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "iAP2DeviceOpen SUCCESS DevID:%p", iap2Device);

            /* create timerfd and provide file descriptor */
            rc = iAP2CreateRunLoopTimerFD(iap2Device);
        }

        if(rc == IAP2_OK)
        {
            iap2Device->iAP2DeviceState = iAP2TransportConnected;
            (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2TransportConnected, iap2Device->iAP2ContextCallback);

            /*Initialize Link*/
            rc = iAP2LinkInit(iap2Device);

            if(rc == IAP2_OK)
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "iAP2LinkInit SUCCESS DevID:%p", iap2Device);
            }
            else
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2LinkInit FAILED DevID:%p", iap2Device);
                rc = IAP2_CTL_ERROR;
            }
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2DeviceOpen FAILED DevID:%p", iap2Device);

            iap2Device->iAP2DeviceState     = iAP2ComError;
            iap2Device->iAP2DeviceErrState  = iAP2TransportConnectionFailed;

            (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device, iAP2ComError, iap2Device->iAP2ContextCallback);
        }


        if((IAP2_OK == rc ) && (NULL != iap2Device->p_iAP2AccessoryLink))
        {
            /**
             *  This will inform link layer to trigger the iAP2LinkSendDetect_CB()
             *  to send the first DETECT byte sequence.
             */
            iAP2LinkRunLoopAttached(iap2Device->p_iAP2AccessoryLink);
            (void)iAP2CallRunLoop(iap2Device, NULL);

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "InitDeviceConnection SUCCESS DevID:%p", iap2Device);
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "InitDeviceConnection FAILED DevID:%p", iap2Device);
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Device Handle DevID:%p", iap2Device);
        rc = IAP2_BAD_PARAMETER;
    }

    return rc;
}
