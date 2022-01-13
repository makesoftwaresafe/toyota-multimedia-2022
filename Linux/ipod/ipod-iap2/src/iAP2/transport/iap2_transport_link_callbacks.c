/*
 * iap2_transport_callbacks.c
 *
 *  Created on: Jul 8, 2013
 *      Author: ajaykumar.s
 */

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h>
#include "iap2_init.h"
#include "iap2_init_private.h"
#include "iap2_transport.h"
#include "iap2_transport_link_callbacks.h"

#include "iap2_dlt_log.h"
#include "iAP2LinkRunLoop.h"


const U8 kIap2PacketDetectBadAck[]  = { 0xFF, 0x55, 0x0E, 0x00, 0x13, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEB };
U8       kIap2PacketDetectBadAckLen = 18;


/*
 * This callback will actually send the packet out.
 * Callback function to call when packet is ready to be sent.
 */
void iAP2LinkSendPacket_CB (iAP2Link_t* iap2Link, iAP2Packet_t*  packet)
{
    S32                 rc              = IAP2_OK;
    U8*                 txBuffer        = NULL;
    U16                 txLen           = 0;
    iAP2Device_st*      iap2Device      = NULL;
    iAP2Transport_t*    transport       = NULL;
    iAP2LinkRunLoop_t*  linkRunLoop     = NULL;

    linkRunLoop = (iAP2LinkRunLoop_t*)iap2Link->context;
    iap2Device  = (iAP2Device_st*)linkRunLoop->context;
    transport   = &(iap2Device->iAP2Transport);

    if(transport->iAP2TransportHdl != NULL)  /*If valid ipod found */
    {
        txBuffer = iAP2PacketGenerateBuffer(packet);
        txLen    = packet->packetLen;

        rc = iAP2DeviceWrite(transport, txBuffer, txLen);
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
                IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "send packet failed=%d. DevID:%p", rc, iap2Device);
            }
            /* trigger device state callback */
            (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device,
                                                                   iap2Device->iAP2DeviceState,
                                                                   iap2Device->iAP2ContextCallback);
        }
    }
    else
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Invalid iAP2TransportHdl DevID:%p",iap2Device);
    }

}

/*
 *  Callback function to call to send detect byte sequence.
 */
void iAP2LinkSendDetect_CB (iAP2Link_t* iap2Link, BOOL  bBad)
{
    S32                 rc          = IAP2_OK;
    iAP2Device_st*      iap2Device  = NULL;
    iAP2Transport_t*    transport   = NULL;
    iAP2LinkRunLoop_t*  linkRunLoop = NULL;

    linkRunLoop = (iAP2LinkRunLoop_t*)iap2Link->context;
    iap2Device  = (iAP2Device_st*)linkRunLoop->context;
    transport   = &(iap2Device->iAP2Transport);

    if(transport->iAP2TransportHdl != NULL) /*If valid ipod found */
    {
        if (TRUE == bBad)
        {
            /* Send DETECT BAD ACK byte  sequence to the device */
            IAP2TRANSPORTDLTLOG(DLT_LOG_WARN, "iAP1 device detected. DevID:%p",iap2Device);

            /* set device state to iAP2LinkiAP1DeviceDetected and call
             * application iAP2DeviceStateCB callback.
             */
            iap2Device->iAP2DeviceState = iAP2LinkiAP1DeviceDetected;
            (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device,
                                                                   iAP2LinkiAP1DeviceDetected,
                                                                   iap2Device->iAP2ContextCallback);
        }
        else
        {
            /* send DETECT byte sequence to the device */
            IAP2TRANSPORTDLTLOG(DLT_LOG_INFO, "send DETECT byte sequence to device. DevID:%p",iap2Device);

            rc = iAP2DeviceWrite(transport, (U8*)kIap2PacketDetectData, kIap2PacketDetectDataLen);
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
                    IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "send detect byte sequence failed=%d. DevID:%p", rc, iap2Device);
                }
                /* trigger device state callback */
                (*iap2Device->iAP2StackCallbacks.p_iAP2DeviceState_cb)(iap2Device,
                                                                       iap2Device->iAP2DeviceState,
                                                                       iap2Device->iAP2ContextCallback);
            }
        }
    }
    else
    {
        IAP2TRANSPORTDLTLOG(DLT_LOG_ERROR, "Invalid iAP2TransportHdl DevID:%p", iap2Device);
    }
}

