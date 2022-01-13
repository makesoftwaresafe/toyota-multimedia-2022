/**
* \file: iap_simpleremote.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include "iap_displayremote.h"
#include "iap_commands.h"
#include "iap_types.h"
#include "iap_general.h"
#include "iap_callback.h"
#include "ipodcommon.h"
#include "iap_util_func.h"
#include "iap1_dlt_log.h"
#include <string.h>
#include <stdlib.h>


/**
 * \addtogroup DisplayRemoteLingoCommands
 */
/*\{*/


/*!
 * \fn S32 iPodSetRemoteEventNotification(U32 iPodID, U32 eventMask)
 * \par DISPLAY REMOTE PROTOCOL VERSION
 * 1.02
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 eventMask - is event notification mask.<br>
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function sets event notification mask.
 * bit 0 means track time position in ms,<br>
 * bit 1 means track playback index,<br>
 * bit 2 means chapter index,<br>
 * bit 3 means play status (play, pause, stop, FF and RW),<br>
 * bit 4 means mute/ UI volume,<br>
 * bit 5 means Power/battery,<br>
 * bit 6 means equalizer setting,<br>
 * bit 7 means shuffle setting,<br>
 * bit 8 means repeat setting,<br>
 * bit 9 means date and time setting,<br>
 * bit 10 means alarm setting (must be 0),<br>
 * bit 11 means backlight level,<br>
 * bit 12 means hold switch state,<br>
 * bit 13 means sound check state,<br>
 * bit 14 means audiobook speed,<br>
 * bit 15 means track time position in ms,<br>
 * bit 16 means mute/UI/absolute volume,<br>
 * bit 17 means track capabilities,<br>
 * bit 18 means playback engine contents,<br>
 * bits 19-31 are reserved (must be 0).<br>
 */
S32 iPodSetRemoteEventNotification(U32 iPodID, U32 eventMask)

{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_REMOTE_EVENT_NOTIFICATION};

    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Remote event mask = 0x%08x",eventMask);
        iPod_convert_to_big32(&msg[IPOD_POS4], eventMask);

        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_ACKNOWLEDGE,
                                            (U8)IPOD_DISPLAY_REMOTE_LINGO);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn S32 iPodSetTrackPosition(U32 iPodID, U32 trackPosition)
 * \par DISPLAY REMOTE PROTOCOL VERSION
 * 1.02
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackPosition - Track position in ms to go to
 * \par REPLY PARAMETERS
 * S32 errcd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed

 * \par DESCRIPTION
 * This function makes the iPod jump to the specified position (in ms)
 * of the currently playing track.
 */

S32 iPodSetTrackPosition(U32 iPodID, U32 trackPosition)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_IPOD_STATE_INFO};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Track position in ms = %u",trackPosition);
        msg[IPOD_POS4] = IPOD_STATE_INFO_TRACK_POSITION_MS;
        iPod_convert_to_big32(&msg[IPOD_POS5], trackPosition);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_DISPLAY_REMOTE_LINGO);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetCurrentEQProfileIndex(U32 iPodID, U32 *index)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * U32 *index - This is pointer for set the current equlizer index.
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function can get the current equalizer index number of iPod.<br>
 */
S32 iPodGetCurrentEQProfileIndex(U32 iPodID, U32 *profileIndex)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_CURRENT_EQ_INDEX};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
        
    if((iPodHndl != NULL) && (profileIndex != NULL))
    {
        /* Set the Ack of GetCurrentEQProfileIndex. */
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RET_CUR_EQ_INDEX, (U8)IPOD_DISPLAY_REMOTE_LINGO);
        
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            /* Convert the Profile Index */
            *profileIndex = iPod_convert_to_little32(iPodHndl->iAP1Buf);
            IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Current Equalizer Index = %u",*profileIndex);
        }
    }
    else
    {
        /* Pointer is NULL */
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p profileIndex = %p",iPodHndl,profileIndex);
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetCurrentEQProfileIndex(U32 iPodID, U32 index, U8 restore)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 index - This is equlizer index to set to iPod.
 * U8 restore - Restore bit. If this value sets to 0, original setting is discarded and new setting is stored.<br>
 * If this value sets to 1, original setting is restored when iPod is disconnected.
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sets the equalizer index to iPod.<br>
 * An equalizer index of 0x0 tells to iPod to disable the equalizer.
 */
S32 iPodSetCurrentEQProfileIndex(U32 iPodID, U32 profileIndex, U8 restore)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_CURRENT_EQ_INDEX};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_ACKNOWLEDGE, (U8)IPOD_DISPLAY_REMOTE_LINGO);
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Current Equalizer index %u restore on exit = %u",profileIndex,restore);

        /* Convert the index to big endian */
        iPod_convert_to_big32(&msg[IPOD_POS4], profileIndex);
        /* Set restore */
        msg[IPOD_POS8] = restore;

        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "iPodHdl is NULL" );
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetNumEQProfiles(U32 iPodID, U32 *profileCount)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * U32 *profileCount - This is pointer for set the number of equlizer.
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function can get the total number of equlizer that iPod has.
 */
S32 iPodGetNumEQProfiles(U32 iPodID, U32 *profileCount)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_NUM_EQ_PROFILES};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl != NULL) && (profileCount != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RET_NUM_EQ_PROFILES, (U8)IPOD_DISPLAY_REMOTE_LINGO);
        
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            /* Convert the total count of equalizer to little endian */
            *profileCount = iPod_convert_to_little32(iPodHndl->iAP1Buf);
            IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Equalizer profile count = %u",*profileCount);
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p profileCount = %p",iPodHndl,profileCount);
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetIndexedEQProfileName(U32 iPodID, U32 profileIndex, U8 *name, U32 size)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 profileIndex - equalizer index
 * U32 size - Size of name pointer
 * \par INOUT PARAMETERS
 * U8 *name - Pointer for set the equalizer name
 * U32 *profileCount - This is pointer for set the number of equlizer.
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function can get the name of equlizer that indicated with equalizer index.
 */
S32 iPodGetIndexedEQProfileName(U32 iPodID, U32 profileIndex, U8 *name, U32 size)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_INDEXED_EQ_NAME};
    U32 length = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl != NULL) && (name != NULL) && (size > 0))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RET_INDEXED_EQ_NAME, (U8)IPOD_DISPLAY_REMOTE_LINGO);
        
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Equalizer Index %u",profileIndex);
        /* Conver the index to big endian */
        iPod_convert_to_big32(&msg[IPOD_POS4], profileIndex);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            /* length of received name */
            length = strlen((VP)iPodHndl->iAP1Buf) + 1;
            if(length <= size)
            {
                /* Name copies to iPodHndl->iAP1Buf until last */
                memcpy(name, iPodHndl->iAP1Buf, length);
            }
            else
            {
                /* Name copies to iPodHndl->iAP1Buf until length of size and last bytes set the NULL */
                memcpy(name, iPodHndl->iAP1Buf, size);
                name[size - 1] = '\0';
            }
            IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Indexed EQ ProfileName is %s",name);
        }
    
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter : iPodHndl = %p,name = %p,size = %d",iPodHndl,name,size);
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetDRArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, U16* resultCount)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.04
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * #IPOD_ARTWORK_FORMAT *resultBuf - Pointer for set artwork format
 * U16 *resultCount - Pointer for set the number of format.
 * U32 *profileCount - This is pointer for set the number of equlizer.
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function can get the artwork format that iPod has.
 */
S32 iPodGetDRArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf,
                          U16* resultCount)
{
    S32 i               =  0;
    S32 responseMsgLen  =  0;
    U16 curIdx          =  0;
    U8* responseMsg     =  NULL;
    U8  msg[]           = {IPOD_GET_DR_ARTWORK_FORMATS_CMD};
    S32 rc              =  IPOD_OK;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    

    if((iPodHndl == NULL) || (resultBuf == NULL) || (resultCount == NULL))
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl= %p,resultBuf = %p resultCount = %p",iPodHndl,resultBuf,resultCount );
    }
    else
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RET_ARTWORK_FORMATS,
                                 (U8)IPOD_DISPLAY_REMOTE_LINGO);
        rc = iPodSendCommand(iPodHndl, msg);
        if (rc == IPOD_OK)
        {
            /* Get the length of response data */
            responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);
            if (responseMsgLen > IPOD_POS6)
            {
                responseMsg = (U8*) malloc((U32)responseMsgLen);
                if (responseMsg != NULL)
                {
                    /* Get the response data */
                    iPodGetResponseData(iPodHndl, responseMsg);

                    /* Check the number of format */
                    *resultCount = (U16)(responseMsgLen / IPOD_ARTWORK_FORMAT_SIZE);
                    IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Number of formats = %d",*resultCount);

                    for (i = 0; i < responseMsgLen; i += IPOD_ARTWORK_FORMAT_SIZE)
                    {
                        resultBuf[curIdx].formatID = iPod_convert_to_little16(&responseMsg[i]);
                        resultBuf[curIdx].imageWidth = iPod_convert_to_little16(&responseMsg[i + IPOD_POS3]);
                        resultBuf[curIdx].imageHeight = iPod_convert_to_little16(&responseMsg[i + IPOD_POS5]);
                        resultBuf[curIdx].pixelFormat = responseMsg[i + IPOD_POS2];
                        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "formatID = %u, imageWidth = %u, imageHeight = %u pixelFormat = 0x%02x",
                                                resultBuf[curIdx].formatID,resultBuf[curIdx].imageWidth,resultBuf[curIdx].imageHeight,resultBuf[curIdx].pixelFormat);
                        curIdx++;
                    }
                    free(responseMsg);
                    rc = 0;
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "No Memory responseMsg is NULL" );
                }
            }
            else
            {
                *resultCount = 0;
                rc = IPOD_OK;
            }
        }

        if(rc != IPOD_OK)
        {
           *resultCount = 0;
            resultBuf   = 0;
        }
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetDRTrackArtworkData(U32 iPodID, U32 trackIndex, U16 formatId, U32 timeOffset, const IPOD_CB_GET_ARTWORK callback)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.04
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - Value of track index
 * U16 formatId - ID of artwork fromat.
 * U32 timeOffset - Time that iPod display the artwork.
 * const #IPOD_CB_GET_ARTWORK callback - Artwork data can get by this callback function.
 * \par INOUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function can get the artwork data that track has.
 */
S32 iPodGetDRTrackArtworkData(U32 iPodID, U32 trackIndex,
                              U16 formatId,
                              U32 timeOffset,
                              const IPOD_CB_GET_ARTWORK callback)
{
    S32 rc = -1;
    U8  msg[] = {IPOD_GET_DR_TRACK_ARTWORK_DATA_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (callback != NULL))
    {
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "trackIndex = %u formatId = %u timeOffset in ms = %u",trackIndex,formatId,timeOffset);
        /* track index into msg array */
        iPod_convert_to_big32(&msg[IPOD_POS4], trackIndex);
        /* insert format id into msg array */
        iPod_convert_to_big16(&msg[IPOD_POS8], formatId);
        /* insert time offstet into msg array */
        iPod_convert_to_big32(&msg[IPOD_POS10], timeOffset);
        /* save the pointer to the callback function */
        iPodRegisterCBTrackArtworkData(callback);
    
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RETARTWORKDATA, (U8)IPOD_DISPLAY_REMOTE_LINGO);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p callback = %p",iPodHndl,callback);
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetDRTrackArtworkTimes(U32 iPodID, U32 trackIndex, U16 formatId, U16 artworkIndex, 
                                  U16 artworkCount, U16 *resultCount, U32 *buffer)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.04
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - specifies which track from the Playback Engine is to be selected <br>
 * S16 formatId - specifies which type of artwork is desired.  <br>
 * U16 artworkIndex - specifies at which index to begin searching for artwork <br>
 * U16 artworkCount - specifies the max. number of times to be returned. A value of -1 (0xFFFF)  <br>
 *                    indicates that there is no preferred limit.
 * \par INOUT PARAMETERS
 * U16* resultCount - number of U32 artwork count values in the buffer <br>
 * U32* buffer - pointer to result buffer. The size of the buffer must be at least the size of artworkCount.
 * The max. needed buffer size for all items can be 65536 bytes.
 * \par REPLY PARAMETERS
 * S32 return -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the list of artwork time locations for a track. The user can get the number of artworks
 * beforehand by using iPodGetIndexedPlayingTrackInfo with IPOD_TRACK_INFO_TYPE = IPOD_TRACK_ARTWORK_COUNT.
 * the paramter trackIndex specifies which track from the Playback Engine is to be selected. The parameter
 * formatID indicates which type of artwork is desired. Please refer to "iPod Extended Interface Specification" page 37.
 * The artworkIndex parameter specifies at which index to begin searching for artwork. A value of 0 indicates
 * that the iPod should start with the first available artwork.
 * artworkCount specifies the maximum number of times (artwork locations) to be returned.
 */
S32 iPodGetDRTrackArtworkTimes(U32 iPodID, U32 trackIndex,
                               U16 formatId,
                               U16 artworkIndex,
                               U16 artworkCount,
                               U16 *resultCount,
                               U32 *buffer)
{
    U32 i               =   0;
    U32 responseMsgLen  =   0;
    U16 curIdx          =   0;
    S32 rc              =   IPOD_OK;
    U8* responseMsg     =   NULL;
    U8  msg[]           =  {IPOD_GET_DR_TRACK_ARTWORK_TIMES_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    

    if((iPodHndl != NULL) && (resultCount != NULL) && (buffer != NULL))
    {
        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "trackIndex = %u formatId = %u artworkIndex = %u artworkCount = %u",trackIndex,formatId,artworkIndex,artworkCount);
        /* track index into msg array */
        iPod_convert_to_big32(&msg[IPOD_POS4], trackIndex);
        /* insert format id into msg array */
        iPod_convert_to_big16(&msg[IPOD_POS8], formatId);
        /* insert time offstet into msg array */
        iPod_convert_to_big32(&msg[IPOD_POS10], artworkIndex);

        /* insert artwork count into msg array */
        iPod_convert_to_big16(&msg[IPOD_POS12], artworkCount);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RETARTWORKTIME,
                                 (U8)IPOD_DISPLAY_REMOTE_LINGO);
        rc = iPodSendCommand(iPodHndl, msg);
        if (rc == 0)
        {
            rc = iPodWaitAndGetResponseLength(iPodHndl);
            if(rc >= IPOD_OK)
            {
                responseMsgLen = (U32)rc;
                if (responseMsgLen >= IPOD_POS4)
                {
                    responseMsg = (U8*) malloc((U32)responseMsgLen);

                    if (responseMsg != NULL)
                    {
                        iPodGetResponseData(iPodHndl, responseMsg);

                        *resultCount = (U16)(responseMsgLen / sizeof(U32));
                        IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "number of artwork count values in the buffer = %d",*resultCount);

                        for (i = 0; i < responseMsgLen; i += sizeof(U32))
                        {
                            buffer[curIdx] = iPod_convert_to_little32(&responseMsg[i + IPOD_POS0]);
                            IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "Time offset in milliseconds = %u",buffer[curIdx]);
                            curIdx++;
                        }

                        free(responseMsg);
                        rc = IPOD_OK;
                    }
                    else
                    {
                        rc = IPOD_ERR_NOMEM;
                        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "No Memory responseMsg is NULL" );
                    }
                }
                else
                {
                    rc = IPOD_OK;
                }
            }

        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl= %p,buffer = %p resultCount = %p",iPodHndl,buffer,resultCount );
    }

    if((rc != IPOD_OK) && (resultCount != NULL))
    {
        *resultCount = 0;
    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}



/*!
 * \fn iPodGetDRPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32 *trackIndex, U32* length, U32* position)
 * \par DISPLAY REMOTE LINGO PROTOCOL VERSION
 * 1.04
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * #IPOD_PLAYER_STATE* state - current iPod state <br>
 * U32* trackIndex - current playing track index.
 * U32* length - track length in ms <br>
 * U32* position - track position in ms <br>
 * \par REPLY PARAMETERS
 * S32 returnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the current iPod playback status.
 */
S32 iPodGetDRPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32 *trackIndex, U32* length, U32* position)
{
    S32 rc          = IPOD_ERROR;
    U8  msg[] = {IPOD_GET_DR_PLAY_STATUS};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (state != NULL) && (trackIndex != NULL)
        && (length != NULL) && (position != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_DISPLAY_LINGO_RETPLAYSTATUS,
                                 (U8)IPOD_DISPLAY_REMOTE_LINGO);

        rc = iPodSendCommand(iPodHndl, msg);
        if (rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if (rc == IPOD_OK)
            {
                *state = (IPOD_PLAYER_STATE)iPodHndl->iAP1Buf[IPOD_POS0];
                *trackIndex = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
                *length = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS5]);
                *position = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS9]);
                IAP1_DISPLAY_LOG(DLT_LOG_VERBOSE, "state = 0x%02x,Index of track index = %u,length of track = 0x%08x,position of track = 0x%08x",
                                                       *state,*trackIndex,*length,*position);
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_DISPLAY_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p,state = %p,trackIndex = %p,length = %p,position = %p",
                                               iPodHndl,state,trackIndex,length,position);

    }
    IAP1_DISPLAY_LOG(DLT_LOG_DEBUG, "rc = %d",rc);

    return rc;
}

/*\}*/
