/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_playback.c
*
*
***************************************************************************** */
/* -----------------------------------------------------------------------------
 * for the correct program flow, the dynamic heap memory allocation is necessary
 * -----------------------------------------------------------------------------
 */

#include <adit_typedef.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <string.h> /* needed to avoid 'implicit declaration of memcopy ...' compiler warning */

#include "ipodcommon.h"
#include "iap_playback.h"
#include "iap_commands.h"
#include "iap_types.h"
#include "iap_general.h"
#include "iap_util_func.h"
#include "iap_init.h"
#include "iap_transport_message.h"
#include "iap1_dlt_log.h"

/* prototypes for local helper functions ==================================== */
LOCAL void handleTrackCapAndInfo(U32 iPodID, const U8* buf, const IPOD_CB_PLAYING_TRACK_INFO callback);
LOCAL void handleTrackReleaseDate(U32 iPodID, const U8* buf, const IPOD_CB_PLAYING_TRACK_INFO callback);
LOCAL void handleLongResponseString(U32 iPodID, IPOD_TRACK_INFO_TYPE info, const IPOD_CB_PLAYING_TRACK_INFO callback);
LOCAL void handleShortResponseString(U32 iPodID,
                                     IPOD_TRACK_INFO_TYPE info,
                                     U8* buf,
                                     const IPOD_CB_PLAYING_TRACK_INFO callback);
LOCAL void handleTrackArtworkCount(U32 iPodID, const IPOD_CB_PLAYING_TRACK_INFO callback);

/**
 * \addtogroup playback_commands
 */
/*\{*/


/*!
 * \fn iPodPlayToggle(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function toggles between play and pause mode.
 */
S32 iPodPlayToggle(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_TOGGLE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayStop(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function stops the playback.
 */
S32 iPodPlayStop(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_STOP_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodIsInAdvancedMode() returns : rc = %d",rc);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayNextTrack(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function starts the playback of the next track.
 */
S32 iPodPlayNextTrack(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_NEXT_TRACK_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayPrevTrack(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function starts the playback of the previous track.
 */
S32 iPodPlayPrevTrack(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_PREV_TRACK_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayFastForward(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sets the iPod into play fast forward mode.
 */
S32 iPodPlayFastForward(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_FFWD_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayFastBackward(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sets the iPod into play fast backward mode.
 */
S32 iPodPlayFastBackward(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_FBWD_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayNormal(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sets the iPod into normal playback mode.
 */
S32 iPodPlayNormal(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_END_FAST_PLAY_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayNextChapter(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function starts the playback of the next chapter.
 * \note If the current track is not an audiobook or podcast with chapters,
 *       they act like #iPodPlayNextTrack or #iPodPlayPrevTrack.
 */
S32 iPodPlayNextChapter(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_NEXT_CHAPTER_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodPlayPrevChapter(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function starts the playback of the previous chapter.
 * \note If the current track is not an audiobook or podcast with chapters,
 *       they act like #iPodPlayNextTrack or #iPodPlayPrevTrack.
 */
S32 iPodPlayPrevChapter(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_PLAY_PREV_CHAPTER_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetShuffle(U32 iPodID, IPOD_SHUFFLE_MODE shuffleMode, BOOL persistent)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * IPOD_SHUFFLE_MODE shuffleMode - shuffle mode to set
 * BOOL persistent - set Restore-on-exit flag
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function sets the iPod into the shuffle mode indicated by shuffleMode.
 * If the parameter persistent is set, setting made by this API persists even after the iPod is detached.
 */
S32 iPodSetShuffle(U32 iPodID, IPOD_SHUFFLE_MODE shuffleMode, BOOL persistent)
{
    S32 rc = IPOD_ERROR;
    U8 *msg = NULL;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            /* Allocate for command */
            msg = calloc((IPOD_SHUFFLE_REPEAT_BASE_LENGTH + IPOD_START_LENGTH), sizeof(U8));
            if(msg == NULL)
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");
            }
            else
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Shuffle Mode = 0x%02x Restore on exit = %u",shuffleMode,persistent);
                /* Set command of SetShuffle */
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = IPOD_SHUFFLE_REPEAT_BASE_LENGTH;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                msg[IPOD_POS3] = 0;
                msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_SET_SHUFFLE;
                msg[IPOD_POS5] = shuffleMode;
                msg[IPOD_POS6] = (U8)persistent;

                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);

                rc = GetAndSetMsg(iPodHndl, msg);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);

    return rc;
}


/*!
 * \fn iPodShuffleOff(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function turns the shuffle mode off. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodShuffleOff(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_SHUFFLE_OFF_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodShuffleOnSongs(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sets the iPod into the shuffle on songs mode. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodShuffleOnSongs(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_SHUFFLE_ON_SONGS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodShuffleOnAlbums(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function sets the iPod into the shuffle on albums mode. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodShuffleOnAlbums(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_SHUFFLE_ON_ALBUMS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetShuffleMode(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 shuffleMode -
 * \li \c \b #IPOD_SHUFFLE_MODE enumeration of shuffle mode
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
 * This function gets the currently set shuffle mode.
 */
S32 iPodGetShuffleMode(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_SHUFFLE_MODE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_SHUFFLE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            /* Fix the bug the reported by DN. Return always 0 if this command success */
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    rc = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Shuffle Mode = 0x%02x",rc);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetRepeat(U32 iPodID, IPOD_REPEAT_MODE repeatMode, BOOL persistent)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * IPOD_REPEAT_MODE repeatMode - repeat mode to set
 * BOOL persistent - set Restore-on-exit flag
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function sets the iPod into the repeat mode indicated by repeatMode.
 * If the parameter persistent is set, setting made by this API persists even after the iPod is detached.
 */
S32 iPodSetRepeat(U32 iPodID, IPOD_REPEAT_MODE repeatMode, BOOL persistent)
{
    S32 rc = IPOD_ERROR;
    U8 *msg = NULL;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            /* Allocate for command */
            msg = calloc((IPOD_SHUFFLE_REPEAT_BASE_LENGTH + IPOD_START_LENGTH), sizeof(U8));
            if(msg == NULL)
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");
            }
            else
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Repeat Mode = 0x%02x Restore on exit = %u",repeatMode,(U8)persistent);
                /* Set command of SetShuffle */
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = IPOD_SHUFFLE_REPEAT_BASE_LENGTH;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
                msg[IPOD_POS3] = 0;
                msg[IPOD_POS4] = IPOD_EXTENDED_LINGO_SET_REPEAT;
                msg[IPOD_POS5] = repeatMode;
                msg[IPOD_POS6] = (U8)persistent;

                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);

                rc = GetAndSetMsg(iPodHndl, msg);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodRepeatOff(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function turns the repeat mode off. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodRepeatOff(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_REPEAT_OFF_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodRepeatCurrentSong(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function sets the iPod into the repeat current song mode. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodRepeatCurrentSong(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_REPEAT_CURRENT_SONG_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodRepeatAllSongs(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
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
 * This function sets the iPod into the repeat all songs mode. The setting made by this API persists even after the iPod is detached.
 */
S32 iPodRepeatAllSongs(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_REPEAT_ALL_SONGS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetRepeatMode(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 RepeatMode -
 * \li \c \b #IPOD_REPEAT_MODE enumeration of repeat mode
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the currently set repeat mode.
 */
S32 iPodGetRepeatMode(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_REPEAT_MODE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_REPEAT,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            /* Fix the bug the reported by DN. Return always 0 if this command success */
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    rc = (S32)iPodHndl->iAP1Buf[0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Repeat Mode = 0x%02x",rc);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetNumPlayingTracks(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 result - number of tracks or
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
 * This function gets the total number of tracks playing in the iPod playback engine
 */
S32 iPodGetNumPlayingTracks(U32 iPodID)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_NUM_PLAYING_TRACKS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_NUM_PLAYING_TRACKS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                if (rc == IPOD_OK)
                {
                    rc = (S32)iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS0]);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Number of Playing Tracks = %d",rc);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetCurrentPlayingTrack(U32 iPodID, U32 trackIndex)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - index of track to play
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
 * This function sets the index of the track in the Now Playing playlist on the iPod.
 * \note: This command is usable only when the iPod is a playing or paused state.
 * !!!       If the iPod is stopped, this command fails
 */
S32 iPodSetCurrentPlayingTrack(U32 iPodID, U32 trackIndex)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_CUR_PLAYING_TRACK_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "track index = %u",trackIndex);
            /* insert track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], trackIndex);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetAudioBookSpeed(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 result - audiobook playback speed or failure
 * \li \c \b #IPOD_AUDIOBOOK_SPEED enumeration of audiobook speed
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
 * This function returns the currently set playback speed for audio books
 */
S32 iPodGetAudioBookSpeed(U32 iPodID)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_AUDIOBOOK_SPEED_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_AUDIOBOOKSPEED,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            /* Fix the bug the reported by DN. Return always 0 if this command success */
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    rc = (S32)iPodHndl->iAP1Buf[0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Audiobook speed = 0x%02x",rc);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetAudioBookSpeed(U32 iPodID, IPOD_AUDIOBOOK_SPEED speed)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_AUDIOBOOK_SPEED speed - the desired playback speed <br>
 * \par REPLY PARAMETERS
 * S32 ReturnCode
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
 * This function sets the playback speed for the currently playing audio book
 */
S32 iPodSetAudioBookSpeed(U32 iPodID, IPOD_AUDIOBOOK_SPEED speed)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_AUDIOBOOK_SPEED_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "New Audio Book Speed = 0x%02x",speed);
    /* insert audiobook speed into msg array */
    msg[IPOD_POS5] = (U8)speed;

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = GetAndSetMsg(iPodHndl, msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetCurrentPlayingTrackChapter(U32 iPodID, U32 chapterIndex)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 chapterIndex - track chapter to play
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
 * This function sets the currently playing track chapter.
 * \note This command should only be used when the iPod is in a playing or paused state.
 * !!! The command fails if the iPod is stopped or if the track does not contain chapter information.
 */
S32 iPodSetCurrentPlayingTrackChapter(U32 iPodID, U32 chapterIndex)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_CUR_PLAYING_TRACK_CHAPTER_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Chapter Index = %u",chapterIndex);
            /* insert track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], chapterIndex);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == 0)
                {
                    rc = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                }
            }
            else
            {
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error 0x%02x (%s)",rc,iPod_get_error_msg(rc));
                rc = iPod_get_error_code(rc);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodPrepareUIDList(U32 iPodID, U64* uidList, U32 uidCount)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U64* uidList - list of unique track identifier
 * U32 uidCount - number of unique track identifier
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_BAD_PARAMETER The number of unique track identifier is not supported
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function prepare a list of tracks for playback.
 * \note
 * Currently only one UID is supported.
 * With the implementation of the "Multisection Data Transfer" it will support UID lists of any length.
 * The UIDs can be received by iPodGetDBTrackInfo or iPodGetBulkDBTrackInfo.
 * Playback of prepared UID list can be started with iPodPlayPreparedUIDList.
 */
S32 iPodPrepareUIDList(U32 iPodID, U64* uidList, U32 uidCount)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_PREPARE_UID_LIST};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (uidList != NULL))
    {
        /* currently only one UID is supported */
        if (uidCount == 1)
        {
            rc = iPodIsInAdvancedMode(iPodHndl);
            if(rc == IPOD_OK)
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "uidList = %llu",*uidList);
                /* insert track index into msg array */
                iPod_convert_to_big64(&msg[IPOD_POS9], *uidList);

                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);

                rc = iPodSendCommand(iPodHndl, msg);
                if (rc == 0)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                    if (rc == 0)
                    {
                        rc = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "rc = %d",rc);
                    }
                }
                else
                {
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error 0x%02x (%s)",rc,iPod_get_error_msg(rc));
                    rc = iPod_get_error_code(rc);
                }
            }
        }
        else
        {
            /* currently only one UID is supported */
            rc = IPOD_BAD_PARAMETER;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "UID is not 1, currently only one UID is supported");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p uidList = %p",iPodHndl,uidList);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodPlayPreparedUIDList(U32 iPodID, U64 trackUID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U64 trackUID - unique track identifier
 * \par REPLY PARAMETERS
 * S32 ReturnCode -
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function starts the playback of the list of tracks prepared by previous PrepareUIDList.
 */
S32 iPodPlayPreparedUIDList(U32 iPodID, U64 trackUID)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_PLAY_PREPARED_UID_LIST};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track UID = %llu",trackUID);
            /* insert track index into msg array */
            iPod_convert_to_big64(&msg[IPOD_POS6], trackUID);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);

            if (rc == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                if (rc == 0)
                {
                    rc = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                }
            }
            else
            {
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error 0x%02x (%s)",rc,iPod_get_error_msg(rc));
                rc = iPod_get_error_code(rc);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetCurrentPlayingTrackChapterName(U32 iPodID, U32 chapterIndex, U8* chapterName)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 chapterIndex - the track's chapter index
 * \par INOUT PARAMETERS
 * U8* chapterName - pointer to a IPOD_RESPONSE_BUF_SIZE byte result buffer where the chapter name will be stored in
 * as a nullterminated UTF-8 character array
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
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the name of the specified chapter in the currently playing track
 */
S32 iPodGetCurrentPlayingTrackChapterName(U32 iPodID, U32 chapterIndex,
                                          U8* chapterName)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_CUR_PLAY_TRACK_CHAPT_NAME_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (chapterName != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Chapter Index = %u",chapterIndex);
            /* insert trackNr and calculated checksum into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], chapterIndex);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_NAME,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);

            if (rc == IPOD_OK)
            {
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, chapterName);
                chapterName[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "CurrentPlayingTrackChapterName = %s",chapterName);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p chapterName = %p",iPodHndl,chapterName);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetCurrentPlayingTrackIndex(U32 iPodID)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 * S32 result - currently playing track index, or
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
 * This function returns the playback engine index of the currently playing track. This is a global index
 * of the track in the iPod database and does not depend on current database selection
 * \note The returned track index is only valid if there is currently a track playing or paused.
 */
S32 iPodGetCurrentPlayingTrackIndex(U32 iPodID)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_CUR_PLAYING_TRACK_INDEX_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CUR_PLAYINGT_TRACK_INDEX,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == IPOD_OK)
                {
                    rc = (S32)iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "CurrentPlayingTrackIndex = %d",rc);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetCurrentPlayingTrackChapterInfo(U32 iPodID, S32* chapterIndex, S32* chapterCount)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * S32* chapterIndex - currently playing track's chapter index
 * S32* chapterCount - total number of chapters in the track
 * \par REPLY PARAMETERS
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
 * This function returns the chapter information of the currently playing track.
 * The chapter index of the first chapter is always 0x00000000. If the track does
 * not have chapter information, a chapter index of -1 (0xFFFFFFFF) and a chapter
 * count of 0 (0x00000000) are returned.
 * \note The returned track index is valid only, when there is a currently playing or paused track.
 */
S32 iPodGetCurrentPlayingTrackChapterInfo(U32 iPodID, S32* chapterIndex, S32* chapterCount)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_CUR_PLAY_TRACK_CHAPT_INFO_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (chapterIndex != NULL) && (chapterCount != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_INFO,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == IPOD_OK)
                {
                    *chapterIndex = (S32)iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    *chapterCount = (S32)iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS4]);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "CurrentPlayingTrackChapterIndex = %d CurrentPlayingTrackChapterCount = %d",*chapterIndex,*chapterCount);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p chapterIndex = %p chapterCount = %p",iPodHndl,chapterIndex,chapterCount);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32* length, U32* position)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * #IPOD_PLAYER_STATE* state - current iPod state <br>
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
S32 iPodGetPlayStatus(U32 iPodID, IPOD_PLAYER_STATE* state, U32* length, U32* position)
{
    S32 rc          = IPOD_ERROR;
    U8  msg[] = {IPOD_GET_PLAY_STATUS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (state != NULL) && (length != NULL) && (position != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_PLAYSTATUS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == IPOD_OK)
                {
                    *length   = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    *position = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS4]);
                    *state    =  (IPOD_PLAYER_STATE)iPodHndl->iAP1Buf[IPOD_POS8];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, " Track length = %u Track position = %u state = 0x%02x",*length,*position,*state);
                }
            }
        }
        else
        {
            *state       =   IPOD_PLAYER_STATE_ERROR;
            *length      =   0;
            *position    =   0;
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p state =%p length = %p position = %p",
                                            iPodHndl,state,length,position);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetIndexedPlayingTrackTitle(U32 iPodID, U32 currIndex, U8* songTitle)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 index - index of the track for which to get the title
 * \par INOUT PARAMETERS
 * U8* songTitle - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the song title will be stored in as a null-terminated UTF-8 character array.
 * \par REPLY PARAMETERS
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
 * This function returns the title name of the indexed playing track from the iPod
 */
S32 iPodGetIndexedPlayingTrackTitle(U32 iPodID, U32 currIndex, U8* songTitle)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_TITLE_FROM_PL_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (songTitle != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Index of the Track = %u",currIndex);
            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], currIndex);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_TITLE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);

            if (rc == 0)
            {
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, songTitle);
                songTitle[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Playing Track Title = %s",songTitle);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p songTitle = %p",iPodHndl,songTitle);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetIndexedPlayingTrackArtistName(U32 iPodID, U32 currIndex, U8* artistName)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 index - index of the track for which to get the artist name
 * \par INOUT PARAMETERS
 * U8* artistName - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the artist name will be stored in as a null-terminated UTF-8 character array.
 * \par REPLY PARAMETERS
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
 * This function returns the artist name of the indexed playing track.
 */
S32 iPodGetIndexedPlayingTrackArtistName(U32 iPodID, U32 currIndex, U8* artistName)
{
    S32 rc = 0;
    U8 msg[] = {IPOD_GET_ARTIST_FROM_PL_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (artistName != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Index of the Track = %u",currIndex);
            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], currIndex);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_ARTISTNAME,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == 0)
            {
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, artistName);
                artistName[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Playing Track Artist Name = %s",artistName);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p artistName = %p",iPodHndl,artistName);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetIndexedPlayingTrackAlbumName(U32 iPodID, U32 currIndex, U8* albumName)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 index - index of the track for which to get the title
 * \par INOUT PARAMETERS
 * U8* albumName - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the album name will be stored in as a null-terminated UTF-8 character array.
 * \par REPLY PARAMETERS
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
 * This function returns the album name of the indexed playing track.
 */
S32 iPodGetIndexedPlayingTrackAlbumName(U32 iPodID, U32 currIndex, U8* albumName)
{
    S32 rc = 0;
    U8 msg[] = {IPOD_GET_ALBUMNAME_FROM_PL_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (albumName != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Index of the Track = %u",currIndex);
            /* insert index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], currIndex);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACK_ALBUMNAME,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == 0)
            {
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, albumName);
                albumName[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Playing Track Album Name = %s",albumName);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p albumName = %p",iPodHndl,albumName);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetIndexedPlayingTrackGenre(U32 iPodID, U32 trackIndex, U16 chapterIndex, U8* genreName)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.08
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - track index
 * U16 chapterIndex - the tracks chapter index <br>
 * \par INOUT PARAMETERS
 * U8* genreName - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the genre name will be stored in as a null-terminated UTF-8 character array.
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
 * This function gets track genre for the track at the specified index.
 */
S32 iPodGetIndexedPlayingTrackGenre(U32 iPodID, U32 trackIndex,
                                    U16 chapterIndex,
                                    U8* genreName)
{
    S32 rc = 0;
    U8 msg[] = {IPOD_GET_INDEXED_PLAYING_TRACK_INFO_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (genreName != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "trackIndex = %u chapterIndex = %u",trackIndex,chapterIndex);
            /* insert info type into msg array */
            msg[IPOD_POS5] = (U8)IPOD_TRACK_GENRE;

            /* track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS6], trackIndex);
            /* insert chapter index into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS10], chapterIndex);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACKINFO,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                memcpy(genreName, &iPodHndl->iAP1Buf[1], iPodHndl->iAP1MaxPayloadSize);
                genreName[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Playing Track Genre = %s",genreName);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p genreName = %p",iPodHndl,genreName);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetCurrentPlayingTrackChapterPlayStatus(U32 iPodID, U32 chapterIndex, U32* chapterLength, U32* elapsedTime)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.06
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 chapterIndex - the track's chapter index
 * \par INOUT PARAMETERS
 * U32* chapterLength - chapter length in ms <br>
 * U32* elapsedTime - elapsed time in chapter in ms
 * \par REPLY PARAMETERS
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
 * This function returns the chapter playtime status of the currently playing track.
 * The status includes the chapter length and the time elapsed within that chapter.
 */
S32 iPodGetCurrentPlayingTrackChapterPlayStatus(U32 iPodID, U32 chapterIndex, U32* chapterLength, U32* elapsedTime)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_CUR_PLAY_TR_CHAPT_PLAYSTAT_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (chapterLength != NULL) && (elapsedTime != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            *chapterLength  = 0;
            *elapsedTime    = 0;

            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "chapterIndex = %u",chapterIndex);
            /* insert track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], chapterIndex);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_CUR_PLAYING_TRACK_CHAPTER_PLAYSTATUS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == 0)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == 0)
                {
                    *chapterLength  = iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    *elapsedTime = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS4]);
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "chapterLength = %u elapsedTime = %u",*chapterLength,*elapsedTime);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p chapterLength = %p elapsedTime =%p",iPodHndl,chapterLength,elapsedTime);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetPlayStatusChangeNotification(U32 iPodID, IPOD_STATUS_CHANGE_NOTIFICATION mode)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_STATUS_CHANGE_NOTIFICATION mode - possible values <br>
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
 * This function sets the state of play status change notifications from the iPod to the device.
 * Notification of play status can be globally enabled or disabled. If notifications are enabled,
 * the iPod sends a notification command to the device each time the play status changes.
 */
S32 iPodSetPlayStatusChangeNotification(U32 iPodID, IPOD_STATUS_CHANGE_NOTIFICATION mode)
{
    S32 rc = IPOD_ERROR;
    IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION set_mode;

    /* Initialize the structure */
    memset(&set_mode, 0, sizeof(set_mode));
    
    if (mode == 0x01)
    {
        set_mode.basicPlay = 1;
        set_mode.trackIndex = 1;
        set_mode.trackTimeMs = 1;
        set_mode.chapterIndex = 1;
    }
    /* First try whether Apple device supports 4-byte command */
    rc = iPodExtendedSetPlayStatusChangeNotification(iPodID, set_mode);

    /* If 4-byte command failed, send 1-byte command (required for ATS2.2) */
    if(IPOD_OK != rc)
    {
        U8 msg[] = {IPOD_SET_PLAY_STATUS_CHANGE_NOTIFY_CMD};
        IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

        if(iPodHndl != NULL)
        {
            rc = iPodIsInAdvancedMode(iPodHndl);
            if(rc == IPOD_OK)
            {
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Mode of Play status change Notification = 0x%02x",mode);
                /* insert notification mode into msg array */
                msg[IPOD_POS5] = (U8)mode;

                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);

                rc = GetAndSetMsg(iPodHndl, msg);
            }
        }
        else
        {
            rc = IPOD_ERR_PAR;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
        }
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodExtendedSetPlayStatusChangeNotification(U32 iPodID, IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode - possible values <br>
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
 * This function sets the state of extended play status change notifications from the iPod to the device.
 * also, it can get the state from iPod by set of bits.
 */
S32 iPodExtendedSetPlayStatusChangeNotification(U32 iPodID, IPOD_EXTENDED_STATUS_CHANGE_NOTIFICATION mode)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_EXTENDED_SET_PLAYSTATUS_CHANGE_CMD};
    U32 *tmpMode = (U32*)&mode;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "PlayStatusChangeNotification = 0x%08x",*tmpMode);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            iPod_convert_to_big32(&msg[IPOD_POS5], *tmpMode);
            rc = iPodSendCommand(iPodHndl, msg);
        }

        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetIndexedPlayingTrackInfo(U32 iPodID, IPOD_TRACK_INFO_TYPE info, U32 trackIndex, U16 chapterIndex, const IPOD_CB_PLAYING_TRACK_INFO callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.08
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_TRACK_INFO_TYPE info - the desired track info <br>
 * U32 trackIndex - track index <br>
 * U16 chapterIndex - the tracks chapter index <br>
 * IPOD_CB_PLAYING_TRACK_INFO callback - pointer to callback function
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
 * This function gets track information for the track at the specified index.
 * The info parameter specifies the type of information to be returned. The
 * Responses are delivered to the specified callback function. For details
 * on the returned track infos, please refer to the documentation of
 * the callback function (IPOD_CB_PLAYING_TRACK_INFO)
 */
S32 iPodGetIndexedPlayingTrackInfo(U32 iPodID, IPOD_TRACK_INFO_TYPE info,
                                   U32 trackIndex,
                                   U16 chapterIndex,
                                   const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_INDEXED_PLAYING_TRACK_INFO_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (callback != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track info type = 0x%02x Track Index = %u ChapterIndex = %u",info,trackIndex,chapterIndex);
            /* insert info type into msg array */
            msg[IPOD_POS5] = (U8)info;

            /* track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS6], trackIndex);
            /* insert chapter index into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS10], chapterIndex);

            if((info == IPOD_TRACK_DESCRIPTION) || (info == IPOD_TRACK_SONG_LYRICS))
            {
                iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACKINFO,
                             (U8)IPOD_LINGO_EXTENDED_INTERFACE, IPOD_HANDLE_RESPONSE_STRING_COUNT);
            }
            else
            {
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_INDEXED_PLAYING_TRACKINFO,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            }

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                switch (info)
                {
                    case IPOD_TRACK_CAP_INFO:
                    {
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (rc == 0)
                        {
                            handleTrackCapAndInfo(iPodID, iPodHndl->iAP1Buf, callback);
                        }
                        break;
                    }

                    case IPOD_TRACK_PODCAST_NAME:
                    {
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (rc == 0)
                        {
                            handleShortResponseString(iPodID, info, iPodHndl->iAP1Buf, callback);
                        }
                        break;
                    }

                    case IPOD_TRACK_RELEASE_DATE:
                    {
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (rc == 0)
                        {
                            handleTrackReleaseDate(iPodID, iPodHndl->iAP1Buf, callback);
                        }
                        break;
                    }

                    case IPOD_TRACK_DESCRIPTION:
                    {
                        handleLongResponseString(iPodID, info, callback);
                        break;
                    }

                    case IPOD_TRACK_SONG_LYRICS:
                    {
                        handleLongResponseString(iPodID, info, callback);
                        break;
                    }

                    case IPOD_TRACK_GENRE:
                    {
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (rc == 0)
                        {
                            handleShortResponseString(iPodID, info, iPodHndl->iAP1Buf, callback);
                        }
                        break;
                    }

                    case IPOD_TRACK_COMPOSER:
                    {
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (rc == 0)
                        {
                            handleShortResponseString(iPodID, info, iPodHndl->iAP1Buf, callback);
                        }
                        break;
                    }

                    case IPOD_TRACK_ARTWORK_COUNT:
                    {
                        handleTrackArtworkCount(iPodID, callback);
                        break;
                    }

                    default:
                    {
                        rc = IPOD_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Unknown Track Info Type");
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p callback = %p",iPodHndl,callback);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodPlayControl(U32 iPodID, IPOD_PLAY_CONTROL ctrl)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_PLAY_CONTROL mode - type of play control
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
 * This function control the media playback state of the iPod.<br>
 * If the iPod is already in the requested state, the command has no effect and returns successfully.<br>
 */
S32 iPodPlayControl(U32 iPodID, IPOD_PLAY_CONTROL ctrl)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_PLAY_CONTROL_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Media Playback state  = 0x%02x",ctrl);
            msg[IPOD_POS5] = (U8)ctrl;
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}
/*\}*/



/* ========================================================================== */
/* private helper functions                                                   */
/* ========================================================================== */

LOCAL void handleTrackCapAndInfo(U32 iPodID, const U8* buf, const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    U32 capability      = 0;
    U32 trackLength     = 0;
    U16 chapterCount    = 0;

    if((buf != NULL) && (callback != NULL))
    {
        IPOD_TRACK_CAP_INFO_DATA capInfoData;

        capability = iPod_convert_to_little32((U8*)&buf[IPOD_POS1]);
        trackLength = iPod_convert_to_little32((U8*)&buf[IPOD_POS5]);

        chapterCount = iPod_convert_to_little16((U8 *)&buf[IPOD_POS9]);

        capInfoData.capability   =  capability;
        capInfoData.trackLength  =  trackLength;
        capInfoData.chapterCount =  chapterCount;
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Capability bits = 0x%04x Track length in ms = %u Chapter Count = %u",capInfoData.capability,capInfoData.trackLength,capInfoData.chapterCount);

        callback(IPOD_TRACK_CAP_INFO,
                 &capInfoData,
                 NULL,
                 NULL,
                 NULL,
                 iPodID);
    }
}


LOCAL void handleTrackReleaseDate(U32 iPodID, const U8* buf, const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    IPOD_TRACK_RELEASE_DATE_DATA releaseDate;

    if((buf != NULL) && (callback != NULL))
    {
        releaseDate.seconds    =  buf[IPOD_POS1];
        releaseDate.minutes    =  buf[IPOD_POS2];
        releaseDate.hours      =  buf[IPOD_POS3];
        releaseDate.dayOfMonth =  buf[IPOD_POS4];
        releaseDate.month      =  buf[IPOD_POS5];
        releaseDate.weekday    =  buf[IPOD_POS8];

        releaseDate.year = iPod_convert_to_little16((U8 *)&buf[IPOD_POS6]);
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Release Date : seconds = %u minutes = %u hours = %u Day of month = %u Month = %u WeekDay = %u Year = %u",
                       releaseDate.seconds,releaseDate.minutes,releaseDate.hours,releaseDate.dayOfMonth,releaseDate.month,releaseDate.weekday,releaseDate.year);

        callback(IPOD_TRACK_RELEASE_DATE,
                 NULL,
                 &releaseDate,
                 NULL,
                 NULL,
                 iPodID);
    }
}


LOCAL void handleLongResponseString(U32 iPodID, IPOD_TRACK_INFO_TYPE info, const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    U8 hasMorePackets = 0;
    S32 rc = IPOD_OK;
    U8 multiFlg = TRUE;
    U16 count = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl != NULL) && (callback != NULL))
    {
        do
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if (rc == IPOD_OK)
            {
                hasMorePackets = iPodHndl->iAP1Buf[IPOD_POS1];
                callback(info, NULL, NULL, NULL, &iPodHndl->iAP1Buf[IPOD_POS4], iPodID);
            }
            
            if(((hasMorePackets & 0x01) != 1) || (((hasMorePackets & 0x01) == 0x01) && ((hasMorePackets & 0x02) == 0x02)))
            {
                multiFlg = FALSE;
            }
            count++;
        }while((rc == IPOD_OK) && (multiFlg != FALSE) && (count < IPOD_HANDLE_RESPONSE_STRING_COUNT));
        iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_ID_DEFAULT, (U8)IPOD_LINGO_DEFAULT, 0);
    }
}


LOCAL void handleShortResponseString(U32 iPodID,
                                     IPOD_TRACK_INFO_TYPE info,
                                     U8* buf,
                                     const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    if(callback != NULL)
    {
        callback(info, NULL, NULL, NULL, &buf[IPOD_POS1], iPodID);
    }
}


LOCAL void handleTrackArtworkCount(U32 iPodID, const IPOD_CB_PLAYING_TRACK_INFO callback)
{
    S32 responseMsgLen = 0;
    U16 curIdx = 0;
    U16 i = 0;
    U32 resCnt = 0;
    U16 structSize = 0;
    U8* responseMsg = NULL;
    IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData;
    IPOD_TRACK_ARTWORK_COUNT_DATA tmpArtworkCountData;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (callback != NULL))
    {
        responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);

        if ((responseMsgLen + IPOD_POS2) > 0)
        {
            responseMsg = (U8*) malloc((U32)responseMsgLen);

            if (responseMsg != NULL)
            {
                structSize = sizeof(IPOD_TRACK_ARTWORK_COUNT_DATA);
                iPodGetResponseData(iPodHndl, responseMsg);

                resCnt = (U32)(responseMsgLen / IPOD_TRACK_ARTWORK_COUNT_DATA_SIZE);
                artworkCountData = (IPOD_TRACK_ARTWORK_COUNT_DATA*) malloc(resCnt * structSize);

                if (artworkCountData != NULL)
                {
                    for (i = 1; i < responseMsgLen; i += IPOD_TRACK_ARTWORK_COUNT_DATA_SIZE)
                    {
                        tmpArtworkCountData.id = iPod_convert_to_little16(&responseMsg[i]);
                        tmpArtworkCountData.count = iPod_convert_to_little16(&responseMsg[i + IPOD_POS2]);
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Artwork Id = %u Track Artwork count = %u",tmpArtworkCountData.id,tmpArtworkCountData.count);


                        if ((curIdx + (U32)1) == resCnt)
                        {
                            tmpArtworkCountData.isLast = TRUE;
                        }
                        else
                        {
                            tmpArtworkCountData.isLast = FALSE;
                        }

                        if(curIdx <= resCnt)
                        {
                            memcpy(&artworkCountData[curIdx],
                                   &tmpArtworkCountData,
                                   structSize);
                        }

                        curIdx++;
                    }

                    callback(IPOD_TRACK_ARTWORK_COUNT,
                             NULL,
                             NULL,
                             artworkCountData,
                             NULL,
                             iPodID);

                    free(artworkCountData);
                }
                free(responseMsg);
            }
        }
    }
}

