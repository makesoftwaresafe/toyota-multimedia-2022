/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_simpleremote.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include "iap_simpleremote.h"
#include "iap_commands.h"
#include "iap_types.h"
#include "iap_general.h"
#include "ipodcommon.h"
#include "iap_util_func.h"
#include <string.h>
#include <stdlib.h>
#include "iap1_dlt_log.h"
/**
 * \addtogroup SimpleRemoteLingoCommands
 */
/*\{*/

/*!
 * \fn iPodPlayPause(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 * S32 errcd - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed

 * \par DESCRIPTION
 * This function toggles between play and pause mode.<br>
 * \note This API is deprecated and should not be used.
 */
S32 iPodPlayPause(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_PLAY_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */



    return errcd;
}


/*!
 * \fn iPodNextTitle(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function is used to jump to the next track.
 */
S32 iPodNextTitle(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_NEXT_TITLE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}


/*!
 * \fn iPodPreviousTitle(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function is used to jump to the previous track.
 */
S32 iPodPreviousTitle(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_PREV_TITLE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}


/*!
 * \fn iPodNextAlbum(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function is used to jump to the next album
 */
S32 iPodNextAlbum(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_NEXT_ALBUM_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}


/*!
 * \fn iPodPreviousAlbum(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function is used to jump to the previous album
 */
S32 iPodPreviousAlbum(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_PREV_ALBUM_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}


/*!
 * \fn iPodVolumeUp(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function increases the volume.
 */
S32 iPodVolumeUp(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_VOLUME_UP_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                           /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}


/*!
 * \fn iPodVolumeDown(U32 iPodID)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 *  S32 errcd - 0 if successfull, <0 otherwise
 * (according to the error codes defined in "tk/errno.h")
 * \par DESCRIPTION
 * This function decreases the volume.
 */
S32 iPodVolumeDown(U32 iPodID)
{
    S32 errcd = IPOD_ERROR;
    U8 msg[]  = {IPOD_VOLUME_DOWN_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    errcd = iPodSendCommand(iPodHndl, msg);
    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns errcd = %d",errcd);
    iPodButtonRelease(iPodID);    /* To avoid that "iPodButtonRelease()" is not invoked, */
                            /* this function call is made generally without replying */
                            /* to the error code of "iPodSendCommand(iPodHndl, ...)" */
                            /* This way is chosen because the comands of the simple remote */
                            /* API are not able to pass iPod error messages to the caller */
                            /* because the iPod does not respond with errors messages if these */
                            /* functions are called. Errors coming up here are just errors relying */
                            /* to memory allocation and/or tk-calls. */

    return errcd;
}

/*!
 * \fn iPodOutButtonStatus(U32 iPodID, U8 source, U32 statusBits)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 source - sourc<br>
 * U32 statusBits - bitmask representing each button that is currently pressed<br>
 * \par REPLY PARAMETERS
 * S32 errcd -
 * \li \c \b #IPOD_OK               Completed successfully
 * \li \c \b #IPOD_ERROR            Command failed
 * \li \c \b #IPOD_ERR_PAR          Bad parameter
 * \par DESCRIPTION
 * This function notifies that the user has pushed one or more accessory buttons
 * intended to control the Apple device.
 * \warning
 * <b> The Apple device must be in iPod Out mode.</b>
 */
S32 iPodOutButtonStatus(U32 iPodID, U8 source, U32 statusBits)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_OUT_BUTTON_STATUS_CMD};
    U8 length = 4;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        if(statusBits > 0x000000FF && statusBits <= 0x0000FFFF)
        {
            length += 1;
        }
        else if(statusBits > 0x0000FFFF && statusBits <= 0x00FFFFFF)
        {
            length += 2;
        }
        else if(statusBits > 0x00FFFFFF && statusBits <= 0xFFFFFFFF)
        {
            length += 3;
        }
        IAP1_SIMPLE_LOG(DLT_LOG_VERBOSE, "button source = 0x%02x",source);
        msg[IPOD_POS1] = length;
        msg[IPOD_POS4] = source;

        memcpy(&msg[IPOD_POS5], &statusBits, sizeof(U32));
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_SIMPLE_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_SIMPLE_LINGO);
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
        IAP1_SIMPLE_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    
    return rc;
}

/*!
 * \fn iPodRotationInputStatus(U32 iPodID, IPOD_ROTATION_INFO rotation)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_ROTATION_INFO rotation - rotation input status<br>
 * \par REPLY PARAMETERS
 * S32 errcd -
 * \li \c \b #IPOD_OK               Completed successfully
 * \li \c \b #IPOD_ERROR            Command failed
 * \li \c \b #IPOD_ERR_PAR          Bad parameter
 * \par DESCRIPTION
 * This function notifies that the user is acting on a rotating device
 * intended to control the Apple device.
 * \warning
 * <b> The Apple device must be in iPod Out mode. </b>
 */
S32 iPodRotationInputStatus(U32 iPodID, IPOD_ROTATION_INFO rotation)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_ROTATION_INPUT_STATUS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        IAP1_SIMPLE_LOG(DLT_LOG_VERBOSE, "No of ms since start of current user action = %u Rotation : Source = 0x%02x Controller Type = 0x%02x Direction = 0x%02x Action = 0x%02x Type = 0x%02x Degrees Moved = %u Degrees Total = %u",
                        rotation.durationMs,rotation.source,rotation.controllerType,rotation.direction,rotation.action,rotation.type,rotation.moved,rotation.total);
        iPod_convert_to_big32(&msg[IPOD_POS4], rotation.durationMs);
        msg[IPOD_POS8] = rotation.source;
        msg[IPOD_POS9] = rotation.controllerType;
        msg[IPOD_POS10] = rotation.direction;
        msg[IPOD_POS11] = rotation.action;
        msg[IPOD_POS12] = rotation.type;
        iPod_convert_to_big16(&msg[IPOD_POS13], rotation.moved);
        iPod_convert_to_big16(&msg[IPOD_POS15], rotation.total);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_SIMPLE_LINGO_ACKNOWLEDGE,
                                     (U8)IPOD_SIMPLE_LINGO);
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
        IAP1_SIMPLE_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    
    return rc;
}

/*!
 * \fn iPodAccessibilityEvent(U32 iPodID, IPOD_ACC_EVENT_TYPE type, IPOD_ACC_EVENT_DATA data)
 * \par SIMPLE REMOTE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_ACC_EVENT_TYPE type - identifies the type of event<br>
 * #IPOD_ACC_EVENT_DATA data - eventData parameter, based on the event type<br>
 * \par REPLY PARAMETERS
 * S32 errcd -
 * \li \c \b #IPOD_OK               Completed successfully
 * \li \c \b #IPOD_ERROR            Command failed
 * \li \c \b #IPOD_BAD_PARAMETER    Bad parameter

 * \par DESCRIPTION
 * This function notifies that the user has generated an interface event.
 */
S32 iPodAccessibilityEvent(U32 iPodID, IPOD_ACC_EVENT_TYPE type, IPOD_ACC_EVENT_DATA data)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U16 length = IPOD_ACC_EVENT_BASE_LEN;
    U8 marker = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl == NULL) || (type > IPOD_CREATE_TOUCH_EVENT) || ((type == IPOD_SEND_TEXT) && (data.text == NULL)))
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_SIMPLE_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p type = %d data.next = %p",iPodHndl,type,data.text);
    }
    else
    {
        IAP1_SIMPLE_LOG(DLT_LOG_VERBOSE, "Accessory event Type = 0x%02x",type);
        switch(type)
        {
            case IPOD_MOVE_POINT : length += 4;
                break;
            case IPOD_SCROLL_POINT : length += 4;
                break;
            case IPOD_SEND_TEXT : length += 4 + data.textLen;
                break;
            case IPOD_CREATE_TOUCH_EVENT : length += 5;
                break;
            default: break;
        }
        
        if(length > IPOD_SHORT_WRITE_SIZE)
        {
            marker += 2;
            length += marker;
        }
        
        msg = (U8 *)calloc(length + IPOD_START_LENGTH, sizeof(U8));
        if(msg != NULL)
        {
            msg[IPOD_POS0] = IPOD_START_PACKET;
            if(marker == 0)
            {
                msg[IPOD_POS1] = (U8)length;
            }
            else
            {
                msg[IPOD_POS1] = 0;
                iPod_convert_to_big16(&msg[IPOD_POS2], length);
            }
            msg[IPOD_POS2 + marker] = IPOD_SIMPLE_LINGO;
            msg[IPOD_POS3 + marker] = 0x13;
            msg[IPOD_POS4 + marker] = (U8)type;
            
            switch(type)
            {
                case IPOD_MOVE_POINT : 
                case IPOD_SCROLL_POINT :
                case IPOD_SEND_TEXT :
                case IPOD_CREATE_TOUCH_EVENT :
                    iPod_convert_to_big16(&msg[IPOD_POS5 + marker], data.x);
                    iPod_convert_to_big16(&msg[IPOD_POS7 + marker], data.y);
                    if(type == IPOD_SEND_TEXT)
                    {
                        msg[IPOD_POS8 + marker] = data.type;
                    }
                    else if(type == IPOD_CREATE_TOUCH_EVENT)
                    {
                        if(((length + IPOD_START_LENGTH) - ((marker + IPOD_POS8) + data.textLen)) >= 0)
                        {
                            memcpy(&msg[IPOD_POS8 + marker], data.text, data.textLen);
                        }
                        else
                        {
                            rc = IPOD_ERROR;
                            IAP1_SIMPLE_LOG(DLT_LOG_ERROR, "Error length = %d marker = %d data.textLen = %d",length,marker,data.textLen);
                        }
                    }
                    break;
                default: break;
            }
            
            if(rc == IPOD_OK)
            {
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_SIMPLE_LINGO_ACKNOWLEDGE,
                                 (U8)IPOD_SIMPLE_LINGO);
                if(marker == 0)
                {
                    rc = iPodSendCommand(iPodHndl, msg);
                    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns : rc = %d",rc);
                }
                else
                {
                    rc = iPodSendLongTelegram(iPodHndl, msg);
                    IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodSendLongTelegram() returns : rc = %d",rc);
                }
            }
            
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_SIMPLE_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");
        }
    }

    if( (rc == IPOD_OK) && (iPodHndl != NULL) )
    {
        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        IAP1_SIMPLE_LOG(DLT_LOG_DEBUG, "iPodWaitAndGetResponseFixedSize() returns rc = %d",rc);
    }
    
    if(msg != NULL)
    {
        free(msg);
    }
    
    return rc;
}

/*\}*/

LOCAL S32 iPodButtonRelease(U32 iPodID)
{
    U8 msg[]  = {IPOD_BUTTON_RELEASE_CMD};

    return iPodSendCommand(iPodGetHandle(iPodID), msg);
}
