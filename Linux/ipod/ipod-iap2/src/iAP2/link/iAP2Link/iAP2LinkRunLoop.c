/*
 *	File: iAP2LinkRunLoop.c
 *	Package: iAP2Link
 *	Abstract: n/a 
 *
 *	Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple
 * 	Inc. ("Apple"), in your capacity as a current, and in good standing,
 *	Licensee in the MFi Licensing Program. Use of this Apple software is
 *	governed by and subject to the terms and conditions of your MFi License,
 *	including, but not limited to, the restrictions specified in the provision
 *	entitled “Public Software”, and is further subject to your agreement to
 *	the following additional terms, and your agreement that the use,
 *	installation, modification or redistribution of this Apple software
 * 	constitutes acceptance of these additional terms. If you do not agree with
 * 	these additional terms, please do not use, install, modify or redistribute
 *	this Apple software.
 *
 *	In consideration of your agreement to abide by the following terms, and
 *	subject to these terms, Apple grants you a personal, non-exclusive
 *	license, under Apple's copyrights in this original Apple software (the
 *	"Apple Software"), to use, reproduce, and modify the Apple Software in
 *	source form, and to use, reproduce, modify, and redistribute the Apple
 *	Software, with or without modifications, in binary form. While you may not
 *	redistribute the Apple Software in source form, should you redistribute
 *	the Apple Software in binary form, in its entirety and without
 *	modifications, you must retain this notice and the following text and
 *	disclaimers in all such redistributions of the Apple Software. Neither the
 *	name, trademarks, service marks, or logos of Apple Inc. may be used to
 *	endorse or promote products derived from the Apple Software without
 *	specific prior written permission from Apple. Except as expressly stated
 *	in this notice, no other rights or licenses, express or implied, are
 *	granted by Apple herein, including but not limited to any patent rights
 *	that may be infringed by your derivative works or by other works in which
 *	the Apple Software may be incorporated.
 *	
 *	The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES
 *	NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
 *	IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A
 *	PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION
 *	ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *	IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
 *	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *	MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND
 *	WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT
 *	LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY
 *	OF SUCH DAMAGE.
 *
 *	Copyright (C) 2012 Apple Inc. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <iAP2LinkConfig.h>
#include <iAP2Time.h>
#include <iAP2Log.h>

#include "iAP2LinkRunLoop.h"
#include "iAP2Packet.h"
#include "iAP2Link.h"
#include "iap2_init_private.h"


#if DEBUG
#define iAP2LINKRUNLOOP_DEBUG 1
#define iAP2LINKRUNLOOP_DEBUG_PACKET 1
#endif


static void _iAP2LinkSignalSendBuffCallback (struct iAP2Link_st* link)
{
    iAP2LinkRunLoop_t* linkRunLoop = (iAP2LinkRunLoop_t*) link->context;
    iAP2LinkRunLoopSignal (linkRunLoop, NULL);
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopGetLinkBuffSize
**
**  Input:
**      synParam:   SYN parameters to base buff size required on.
**
**  Output:
**      None
**
**  Return:
**      uint32_t    minimum size of buff required for proper operation based
**
**
*****************************************************************
*/
uint32_t iAP2LinkRunLoopGetBuffSize (uint8_t maxPacketSentAtOnce)
{
#if iAP2_LINK_ALLOW_MALLOC != 0
    uint32_t result = sizeof(iAP2LinkRunLoop_t) + iAP2_LINK_RUNLOOP_BUFFER_OFFSET;
#else
    uint32_t result = sizeof(iAP2LinkRunLoop_t);
#endif
    result += iAP2LinkGetBuffSize (maxPacketSentAtOnce);
    return result;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopCreate
**  iAP2LinkRunLoopCreateAccessory (type = kiAP2LinkTypeAccessory)
**  iAP2LinkRunLoopCreateDevice (type = kiAP2LinkTypeDevice)
**
**  Input:
**      type:           Indicates whether this is for Accessory or Device.
**      context:            Context info to store with the link.
**      synParam:       SYN packet parameters
**      sendPacketCB:       Callback function to call when packet is ready to be sent.
**                              This callback will actually send the packet out.
**      recvDataCB:         Callback function to call when received data is ready.
**      connectedCB:        Callback function to call when link connection is UP/DOWN.
**      sendDetectCB:       Callback function to call to send detect byte sequence.
**      bValidateSYN        Flag indicating whether to check for valid SYN param.
**                              (for debugging/testing purpose)
**      linkRLBuffer        Pre-allocated buffer to use for storing iAP2LinkRunLoop_t
**                              and other related structures.
**
**  Output:
**      Passed in linkRLBuffer is initiliazed.
**
**  Return:
**      iAP2LinkRunLoop_t*  pointer to new allocated iAP2LinkRunLoop_t structure
**                          or the passed in linkRLBuffer.
**
*****************************************************************
*/
iAP2LinkRunLoop_t* iAP2LinkRunLoopCreateAccessory (iAP2PacketSYNData_t*       synParam,
                                                   void*                      context,
                                                   iAP2LinkSendPacketCB_t     sendPacketCB,
                                                   iAP2LinkDataReadyCB_t      recvDataCB,
                                                   iAP2LinkConnectedCB_t      connectedCB,
                                                   iAP2LinkSendDetectCB_t     sendDetectCB,
                                                   BOOL                       bValidateSYN,
                                                   uint8_t                    maxPacketSentAtOnce,
                                                   uint8_t*                   linkRLBuffer)
{
    return iAP2LinkRunLoopCreate (kiAP2LinkTypeAccessory,
                                  context,
                                  synParam,
                                  sendPacketCB,
                                  NULL,
                                  recvDataCB,
                                  connectedCB,
                                  sendDetectCB,
                                  bValidateSYN,
                                  maxPacketSentAtOnce,
                                  linkRLBuffer);
}


iAP2LinkRunLoop_t* iAP2LinkRunLoopCreateDevice (iAP2PacketSYNData_t*       synParam,
                                                void*                      context,
                                                iAP2LinkSendPacketCB_t     sendPacketCB,
                                                iAP2LinkSendPacketWaitCB_t sendPacketWaitCB,
                                                iAP2LinkDataReadyCB_t      recvDataCB,
                                                iAP2LinkConnectedCB_t      connectedCB,
                                                iAP2LinkSendDetectCB_t     sendDetectCB,
                                                BOOL                       bValidateSYN,
                                                uint8_t                    maxPacketSentAtOnce,
                                                uint8_t*                   linkRLBuffer)
{
    return iAP2LinkRunLoopCreate (kiAP2LinkTypeDevice,
                                  context,
                                  synParam,
                                  sendPacketCB,
                                  sendPacketWaitCB,
                                  recvDataCB,
                                  connectedCB,
                                  sendDetectCB,
                                  bValidateSYN,
                                  maxPacketSentAtOnce,
                                  linkRLBuffer);
}


iAP2LinkRunLoop_t* iAP2LinkRunLoopCreate (iAP2LinkType_t              type,
                                          void*                       context,
                                          iAP2PacketSYNData_t*        synParam,
                                          iAP2LinkSendPacketCB_t      sendPacketCB,
                                          iAP2LinkSendPacketWaitCB_t  sendPacketWaitCB,
                                          iAP2LinkDataReadyCB_t       recvDataCB,
                                          iAP2LinkConnectedCB_t       connectedCB,
                                          iAP2LinkSendDetectCB_t      sendDetectCB,
                                          BOOL                        bValidateSYN,
                                          uint8_t                     maxPacketSentAtOnce,
                                          uint8_t*                    linkRLBuffer)
{
    iAP2LinkRunLoop_t* linkRunLoop;
    uint8_t*           linkRLBufferNext;
#if iAP2_LINK_ALLOW_MALLOC != 0
    if (NULL == linkRLBuffer)
    {
        uint8_t* buff = malloc (iAP2LinkRunLoopGetBuffSize (maxPacketSentAtOnce));
        linkRunLoop               = (iAP2LinkRunLoop_t*) buff;
        linkRLBufferNext          = buff;
        linkRunLoop->linkRLBuffer = buff;
    }
    else
    {
        linkRunLoop               = (iAP2LinkRunLoop_t*) (linkRLBuffer + iAP2_LINK_RUNLOOP_BUFFER_OFFSET);
        linkRLBufferNext          = linkRLBuffer + iAP2_LINK_RUNLOOP_BUFFER_OFFSET;
        linkRunLoop->linkRLBuffer = linkRLBuffer;
    }
#else
    assert (linkRLBuffer);
    linkRunLoop               = (iAP2LinkRunLoop_t*) linkRLBuffer;
    linkRLBufferNext          = linkRLBuffer;
    linkRunLoop->linkRLBuffer = linkRLBuffer;
#endif
    assert(linkRunLoop);
    linkRLBufferNext += sizeof(iAP2LinkRunLoop_t);
#if iAP2LINKRUNLOOP_DEBUG
    iAP2LogDbg("%s:%d Create %s side link\n",
               __FILE__, __LINE__,
               (type == kiAP2LinkTypeDevice ? "Device" : "Accessory"));
#endif
    linkRunLoop->context        = context;
    linkRunLoop->otherData      = NULL;
    linkRunLoop->shuttingDown   = FALSE;
    linkRunLoop->eventMask      = 0;

    linkRunLoop->link = iAP2LinkCreate (type,
                                        linkRunLoop,
                                        synParam,
                                        sendPacketCB,
                                        sendPacketWaitCB,
                                        recvDataCB,
                                        connectedCB,
                                        sendDetectCB,
                                        _iAP2LinkSignalSendBuffCallback,
                                        bValidateSYN,
                                        maxPacketSentAtOnce,
                                        linkRLBufferNext);
    linkRLBufferNext += iAP2LinkGetBuffSize (maxPacketSentAtOnce);
    assert (linkRunLoop->link);

    linkRunLoop->linkRLBufferNext = linkRLBufferNext;
#if iAP2_LINK_USE_LINKRUNLOOP
    iAP2LinkSetUseiAP2LinkRunLoop (linkRunLoop->link);
#endif

    iAP2LinkStart (linkRunLoop->link);

    iAP2LinkRunLoopInitImplementation (linkRunLoop);

    return linkRunLoop;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopDelete
**
**  Input:
**      linkRunLoop:    linkRunLoop structure to delete
**
**  Output:
**      None
**
**  Return:
**      None
**
*****************************************************************
*/
void iAP2LinkRunLoopDelete (iAP2LinkRunLoop_t* linkRunLoop)
{
    if (linkRunLoop != NULL)
    {
        linkRunLoop->shuttingDown = TRUE;
        iAP2LinkRunLoopSignal (linkRunLoop, NULL);
        iAP2LinkRunLoopCleanupImplementation (linkRunLoop);
        iAP2LinkDelete (linkRunLoop->link);
        linkRunLoop->link = NULL;
#if iAP2_LINK_ALLOW_MALLOC != 0
        if ((uint8_t*)linkRunLoop == linkRunLoop->linkRLBuffer)
        {
            free ((uint8_t*)linkRunLoop);
        }
#endif
    }
    else
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopRunOnce
**
**  Input:
**      linkRunLoop:    linkRunLoop structure to delete
**      arg:            argument for the call (unused)
**
**  Output:
**      None
**
**  Return:
**      BOOL    TRUE if more left, else FALSE
**
**  Note: This call is used to process link layer tasks such as processing
**          system events (attach/detach/suspend), received packets, packets
**          to send, etc.
**
*****************************************************************
*/
BOOL iAP2LinkRunLoopRunOnce (iAP2LinkRunLoop_t* linkRunLoop, void* arg)
{
    int i;
    iAP2Packet_t* packet = (iAP2Packet_t*) arg;
    BOOL bContinue = FALSE;
    if (linkRunLoop)
    {
        uint32_t eventMask = iAP2LinkRunLoopGetResetEventMask (linkRunLoop);

        assert (linkRunLoop->link);

        if (eventMask & kiAP2LinkRunLoopEventMaskAttach)
        {
            iAP2LinkAttached (linkRunLoop->link);
        }
        if (eventMask & kiAP2LinkRunLoopEventMaskDetach)
        {
            iAP2LinkDetached (linkRunLoop->link);
        }
        if (eventMask & kiAP2LinkRunLoopEventMaskSuspend)
        {
            iAP2LinkHandleSuspend (linkRunLoop->link);
        }

        /* Handle received packets */
        if (packet)
        {
            iAP2LinkHandleReadyPacket (linkRunLoop->link, packet);
        }

        /*
        ** Check timers
        ** There are three timers: WaitDETECT, SendACK, WaitACK
        */
        if (eventMask & kiAP2LinkRunLoopEventMaskTimeouts)
        {
            uint32_t curTime = iAP2TimeGetCurTimeMs();
            if (eventMask & kiAP2LinkRunLoopEventMaskWaitDetectTimeout)
            {
                iAP2LinkHandleWaitDetectTimeoutLink (linkRunLoop->link, curTime);
            }
            if (eventMask & kiAP2LinkRunLoopEventMaskWaitACKTimeout)
            {
                iAP2LinkHandleWaitACKTimeoutLink (linkRunLoop->link, curTime);
            }
            if (eventMask & kiAP2LinkRunLoopEventMaskSendACKTimeout)
            {
                iAP2LinkHandleSendACKTimeoutLink (linkRunLoop->link, curTime);
            }
        }

        /* Check if anything needs to be sent out. */
        iAP2LinkProcessSendBuff (linkRunLoop->link);
        for (i = 0; i < kIAP2PacketServiceTypeCount; ++i)
        {
            if (iAP2ListArrayGetCount(linkRunLoop->link->sessSendPckList[i]) > 0)
            {
                bContinue = TRUE;
                break;
            }
        }
    }
    else
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
    return bContinue;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopRun
**
**  Input:
**      linkRunLoop:    linkRunLoop structure to delete
**
**  Output:
**      None
**
**  Return:
**      BOOL    TRUE if existed without error (RunLoop finished), else FALSE
**
**  Note: This call is used when running the link layer control loop from
**          a dedicated thread with support for support for signal/wait.
**
*****************************************************************
*/
BOOL iAP2LinkRunLoopRun (iAP2LinkRunLoop_t* linkRunLoop)
{
    BOOL status = TRUE;
    if (linkRunLoop)
    {
        while ( status && ! linkRunLoop->shuttingDown )
        {
            if ( ! iAP2LinkRunLoopProtectedCall (linkRunLoop,
                                                 NULL,
                                                 iAP2LinkRunLoopRunOnce) )
            {
                if ( ! iAP2LinkRunLoopWait (linkRunLoop))
                {
                    status = FALSE;
                }
            }
        }
    }
    else
    {
        status = FALSE;
    }
    return status;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopTimeout
**
**  Input:
**      linkRunLoop:    linkRunLoop structure to delete
**      timeoutID:      expired timeoutID
**      timeoutType:    expired timeout type
**      curTime:        current time for timeout
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   Should be called via iAP2LinkRunLoopProtectedCall
**
*****************************************************************
*/
void iAP2LinkRunLoopTimeout (iAP2LinkRunLoop_t* linkRunLoop,
                             uint16_t           timeoutID,
                             uint8_t            timeoutType,
                             uint32_t           curTime)
{
    if (linkRunLoop)
    {
        switch (timeoutType)
        {
            case kiAP2LinkEventWaitDetectTimeout:
                iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskWaitDetectTimeout);
                iAP2LinkRunLoopSignal (linkRunLoop, NULL);
                break;
            case kiAP2LinkEventWaitACKTimeout:
                iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskWaitACKTimeout);
                iAP2LinkRunLoopSignal (linkRunLoop, NULL);
                break;
            case kiAP2LinkEventSendACKTimeout:
                iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskSendACKTimeout);
                iAP2LinkRunLoopSignal (linkRunLoop, NULL);
                break;
        }
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopAttached
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note: Notify link that the transport has been established.
**
*****************************************************************
*/
void iAP2LinkRunLoopAttached (iAP2LinkRunLoop_t* linkRunLoop)
{
    if (linkRunLoop)
    {
        iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskAttach);
        iAP2LinkRunLoopSignal (linkRunLoop, NULL);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopDetached
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note: Notify link that the transport has been torn down.
**
*****************************************************************
*/
void iAP2LinkRunLoopDetached (iAP2LinkRunLoop_t* linkRunLoop)
{
    if (linkRunLoop)
    {
        iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskDetach);
        iAP2LinkRunLoopSignal (linkRunLoop, NULL);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopHandleSuspend
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      None
**
*****************************************************************
*/
void iAP2LinkRunLoopHandleSuspend (struct iAP2LinkRunLoop_st* linkRunLoop)
{
    if (linkRunLoop)
    {
        /* This needs to be called right away */
        iAP2LinkRunLoopSetEventMaskBit (linkRunLoop, kiAP2LinkRunLoopEventMaskSuspend);
        iAP2LinkRunLoopRunOnce (linkRunLoop, NULL);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopHandleReadyPacket
**
**  Input:
**      linkRunLoop:    link structure
**      packet:         packet to handle: consider packet belongs to link layer.
**                                        caller should not delete the packet.
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note: Process a parsed packet and generate appropriate event.
**        This is called after packet parsing finishes.
**        Should be called via iAP2LinkRunLoopProtectedCall
**
*****************************************************************
*/
void iAP2LinkRunLoopHandleReadyPacket (iAP2LinkRunLoop_t* linkRunLoop,
                                       void*              packetArg)
{
    iAP2Packet_t* packet = (iAP2Packet_t*) packetArg;
    if (linkRunLoop)
    {
        iAP2LinkRunLoopSignal (linkRunLoop, packet);
    }
    else
    {
        iAP2PacketDelete (packet);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopQueueSendData
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**      payload:        payload data buffer to send
**      payloadLen:     size of the payload data buffer
**      session:        sessionID
**      context:        context to use when calling callback on data buffer send
**      callback:       callback to call when data buffer has been sent
**
**  Output:
**      linkRunLoop:    linkRunLoop structure is updated to reflect state after
**                          processing payload data.
**
**  Return:
**      None
**
**  Note: Queue data for sending. Must cause a DataToSend event to be generated
**          for the data to actually get sent out.
**
*****************************************************************
*/
void iAP2LinkRunLoopQueueSendData (iAP2LinkRunLoop_t*     linkRunLoop,
                                   const uint8_t*         payload,
                                   uint32_t               payloadLen,
                                   uint8_t                session,
                                   void*                  context,
                                   iAP2LinkDataSentCB_t   callback)
{
    if (linkRunLoop)
    {
        iAP2LinkQueueSendData (linkRunLoop->link,
                               payload,
                               payloadLen,
                               session,
                               context,
                               callback);
    }
    else
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopDebugPrint
**
**  Input:
**      linkRunLoop:    linkRunLoop to print out info for.
**      file:           file called from
**      line:           line number called from
**      tag:            additional info to display with packet info
**      bDebug:         whether to print as debug with data or regular stats printout
**
**  Output:
**      None
**
**  Return:
**      None
**
*****************************************************************
*/
void iAP2LinkRunLoopDebugPrint (iAP2LinkRunLoop_t* linkRunLoop,
                                const char*        file,
                                int                line,
                                const char*        tag,
                                BOOL               bDebug)
{
#if DEBUG
    iAP2LogDbg("%s:%d linkRunLoop=%p: context=%p link=%p\n",
               file, line, linkRunLoop, linkRunLoop->context, linkRunLoop->link);
    iAP2LinkDebugPrintLink (linkRunLoop->link,
                            file,
                            line,
                            tag,
                            bDebug);
#endif /* DEBUG */
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopInitImplementation
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:
**      Implementation specific initialization.
**      Implementation specific data can be stored in iAP2LinkRunLoop_t.otherData
**      TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**
*****************************************************************
*/
void iAP2LinkRunLoopInitImplementation (iAP2LinkRunLoop_t* linkRunLoop)
{
    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopCleanupImplementation
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   Implementation specific cleanup.
**          TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**
**
*****************************************************************
*/
void iAP2LinkRunLoopCleanupImplementation (iAP2LinkRunLoop_t* linkRunLoop)
{
    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopWait
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      BOOL    TRUE if successfully waited and got signal, else FALSE
**
**  Note:   Waits for a signal (via iAP2LinkRunLoopSignal) before continuing.
**          TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**
**          ADIT implemented this as a stub function as per the usage note for
**          single control-loop in  iAP2LinkRunLoop.h.
**
**          In the current implementation it always returns TRUE
**
*****************************************************************
*/
BOOL iAP2LinkRunLoopWait (iAP2LinkRunLoop_t* linkRunLoop)
{
    BOOL status = TRUE;

    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }

    return status;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopSignal
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**      arg:            argument to call back iAP2LinkRunLoopRunOnce with.
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   Signals RunLoop to continue processing (wait is via iAP2LinkRunLoopWait.
**          Should be implemented to call back into iAP2LinkRunLoopRunOnce under protection
**          of iAP2LinkRunLoopProtectedCall.
**          TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**
*****************************************************************
*/
void iAP2LinkRunLoopSignal (iAP2LinkRunLoop_t* linkRunLoop, void* arg)
{
    iAP2Device_st* device = (iAP2Device_st*)linkRunLoop->context;

    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
    device->iAP2RunLoop.iAP2RunLoopCall = 1;
    device->iAP2RunLoop.iAP2RunLoopCallArg = arg;

}


/*
*****************************************************************
**
**  iAP2LinkRunLoopSetEventMaskBit
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**      bit:            eventMask bit to set (iAP2LinkRunLoopEventMask_t)
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**                If access to evenMask needs to be protected must make
**                sure operation on linkRunLoop->eventMask is atomic.
**
*****************************************************************
*/
void iAP2LinkRunLoopSetEventMaskBit (iAP2LinkRunLoop_t*         linkRunLoop,
                                     iAP2LinkRunLoopEventMask_t bit)
{
    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
    else
    {
        linkRunLoop->eventMask |= bit;
    }
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopGetResetEventMask
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**
**  Output:
**      None
**
**  Return:
**      uint32_t    bit mask of events currently pending (iAP2LinkRunLoopEventMask_t)
**
**  Note:   Retrieve the eventMask and reset it to kiAP2LinkRunLoopEventMaskNone.
**          TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**                If access to evenMask needs to be protected must make
**                sure operation on linkRunLoop->eventMask is atomic.
**
*****************************************************************
*/
uint32_t iAP2LinkRunLoopGetResetEventMask (iAP2LinkRunLoop_t* linkRunLoop)
{
    uint32_t event_mask = kiAP2LinkRunLoopEventMaskNone;

    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
    else
    {
        event_mask = linkRunLoop->eventMask;
        linkRunLoop->eventMask = kiAP2LinkRunLoopEventMaskNone;
    }
    return event_mask;
}


/*
*****************************************************************
**
**  iAP2LinkRunLoopProtectedCall
**
**  Input:
**      linkRunLoop:    linkRunLoop structure
**      arg:            argument for use when calling func
**      func:           function to call
**
**  Output:
**      None
**
**  Return:
**      BOOL    return value of function called with protection.
**
**  Note:   Call the indicated function with simultaneous call protection.
**          TODO: Needs to be implemented by the user of iAP2LinkRunLoop
**
**          In the current implementation it returns TRUE always.
**
*****************************************************************
*/
BOOL iAP2LinkRunLoopProtectedCall (iAP2LinkRunLoop_t* linkRunLoop,
                                   void* arg,
                                   BOOL (*func)(iAP2LinkRunLoop_t* linkRunLoop, void* arg))
{
    BOOL status = TRUE;

    if (!linkRunLoop)
    {
        iAP2LogError("%s:%d NULL linkRunLoop!\n", __FILE__, __LINE__);
    }
    return status;
}
