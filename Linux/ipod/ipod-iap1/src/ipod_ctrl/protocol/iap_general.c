/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_general.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include <string.h> /* needed to avoid 'implicit declaration of memcopy ...' compiler warning */
#include <stdlib.h>
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_general.h"
#include "iap_types.h"
#include "iap_devconf.h"
#include "iap_util_func.h"
#include "iap_init.h"
#include "iap_transport_authentication.h"
#include "iap_transport_message.h"
#include "iap_transport_os.h"
#include "iap_transport_configuration.h"
#include "iap_transport_process_message.h"
#include "iap_callback.h"
#include "iap1_dlt_log.h"

/* Bluetooth autopairing status notification length - 36-byte fields */
#define IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN    36
#define IPOD_POS34                                      34
#define IPOD_POS35                                      35

/**
 * \addtogroup GeneralLingoCommands
 */
/*\{*/

/*!
 * \fn iPodEnterExtendedInterfaceMode(U32 iPodID)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
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
 * This function sets the iPod into the extended interface mode.
 * In this mode it is not possible to operate the iPod via the iPod's built-in
 * button controls. The Extended Interface lingo commands can be used only in this mode.<br>
 * This API can take several seconds to return, since the iPod can give a command to wait for the mode switch. 
 * Please do not use this command from iPod Control callback APIs.
 */
S32 iPodEnterExtendedInterfaceMode(U32 iPodID)
{
    S32 result = IPOD_ERROR;

    /* EnterExtendedInterfaceMode command is deprecated, first try SetUIMode */
    result = iPodSetUIMode(iPodID, 0x01);

    /* If SetUIMode fails, try EnterExtendedInterfaceMode */
    if(IPOD_OK != result)
    {
#ifndef IPOD_FAST_MODE_SWITCH
        U32 timeout = 0;
#endif /* IPOD_FAST_MODE_SWITCH */
        S32 err = IPOD_ERROR;
        U8 msg[] = {IPOD_ENTER_EXTENDED_INTERFACE_MODE_CMD};
        IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
        
        if(iPodHndl != NULL)
        {
            if(iPodHndl->isAPIReady != FALSE)
            {
                err = iPodWaitForModeSwitchSemaphore(iPodHndl);
                if(err == IPOD_OK)
                {
                    iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
                    
                    result = iPodSendCommand(iPodHndl, msg);
                    if (result == IPOD_OK)
                    {
                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                        if (result == IPOD_OK)
                        {
                            if (iPodHndl->iAP1Buf[0] == IPOD_CMD_ACK_PENDING)
                            {
#ifndef IPOD_FAST_MODE_SWITCH
#ifdef IPOD_FAST_MODE_SWITCH_500MS
                                timeout = IPOD_MODE_CHANGE_WAIT_500MS;
#else
                                timeout = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS2]);
#endif /* IPOD_FAST_MODE_SWITCH_500MS */
                            
                                iPodOSSleep(IPOD_MODE_CHANGE_WAIT_500MS);
#endif /* IPOD_FAST_MODE_SWITCH */

                                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                if (result == IPOD_OK)
                                {
                                    if(iPodHndl->iAP1Buf[0] != IPOD_OK)
                                    {
                                        result = iPod_get_error_code((S32)iPodHndl->iAP1Buf[0]);
                                        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error = 0x%02x (%s)",iPodHndl->iAP1Buf[0],iPod_get_error_msg((S32)iPodHndl->iAP1Buf[0]));
                                    }
                                }
                            }
                            else
                            {
                                result =iPod_get_error_code((S32)iPodHndl->iAP1Buf[0]);
                                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error = 0x%02x (%s)",iPodHndl->iAP1Buf[0],iPod_get_error_msg((S32)iPodHndl->iAP1Buf[0]));
                            }
                        }
                    }

                    err = iPodSigModeSwitchSemaphore(iPodHndl);
                    if(err != IPOD_OK)
                    {
                        result = IPOD_ERR_DISWAI;
                        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodSigModeSwitchSemaphore() returns : err = %d",err);
                    }
                
#ifndef IPOD_FAST_MODE_SWITCH
                
                    if(result == IPOD_OK)
                    {
                        if(timeout > 500)
                        {
                            iPodOSSleep(timeout - IPOD_MODE_CHANGE_WAIT_500MS);
                        }
                    }
#endif /* IPOD_FAST_MODE_SWITCH */
                
                }
                else
                {
                    result = err;
                    IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodWaitForModeSwitchSemaphore() returns : err = %d",err);
                }
            }
            else
            {
                result = IPOD_NOT_CONNECTED;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
            }
        }
        else
        {
            result = IPOD_ERR_PAR;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
        }
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodEnterSimpleMode(U32 iPodID)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
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
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function forces the iPod to exit the Extended Interface Mode.
 * If the iPod is already in the Standard UI Mode, it immediately returns
 * a General Lingo ACK command packet, notifying the user that the command
 * was successful.<br>
 * This API can take several seconds to return, since the iPod can give a command to wait for the mode switch.
 * Please do not use this command from iPod Control callback APIs.
 */
S32 iPodEnterSimpleMode(U32 iPodID)
{
    S32 result = IPOD_ERROR;

    /* EnterExtendedInterfaceMode command is deprecated, exit might become too - first try SetUIMode */
    result = iPodSetUIMode(iPodID, 0x00);

    /* If SetUIMode fails, try EnterExtendedInterfaceMode */
    if(IPOD_OK != result)
    {
#ifndef IPOD_FAST_MODE_SWITCH
        U32 timeout =   0;
#endif /* IPOD_FAST_MODE_SWITCH */
        S32 err  =  IPOD_ERROR;
        U8 msg[] = {IPOD_ENTER_SIMPLE_INTERFACE_MODE_CMD};
        IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

        if(iPodHndl != NULL)
        {
            if(iPodHndl->isAPIReady != FALSE)
            {
                err = iPodWaitForModeSwitchSemaphore(iPodHndl);
                if(err == IPOD_OK)
                {
                    iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
                    result = iPodSendCommand(iPodHndl, msg);
                    if (result == IPOD_OK)
                    {
                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                        if (result == IPOD_OK)
                        {
                            if (iPodHndl->iAP1Buf[0] == IPOD_CMD_ACK_PENDING)
                            {
#ifndef IPOD_FAST_MODE_SWITCH
#ifdef IPOD_FAST_MODE_SWITCH_500MS
                                timeout = IPOD_MODE_CHANGE_WAIT_500MS;
#else
                                timeout = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS2]);
#endif /* IPOD_FAST_MODE_SWITCH_500MS */

                                iPodOSSleep(IPOD_MODE_CHANGE_WAIT_500MS);
#endif /* IPOD_FAST_MODE_SWITCH */
                                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);

                                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                if (result == IPOD_OK)
                                {
                                    if(iPodHndl->iAP1Buf[0] != IPOD_OK)
                                    {
                                        result = iPod_get_error_code((S32)iPodHndl->iAP1Buf[0]);
                                        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error = 0x%02x (%s)",iPodHndl->iAP1Buf[0],iPod_get_error_msg((S32)iPodHndl->iAP1Buf[0]));
                                    }
                                }
                            }
                            else
                            {
                                result = iPod_get_error_code((S32)iPodHndl->iAP1Buf[0]);
                                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error = 0x%02x (%s)",iPodHndl->iAP1Buf[0],iPod_get_error_msg((S32)iPodHndl->iAP1Buf[0]));
                            }
                        }
                    }

                    err = iPodSigModeSwitchSemaphore(iPodHndl);
                    if(err != IPOD_OK)
                    {
                        result = IPOD_ERR_DISWAI;
                        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodSigModeSwitchSemaphore() returns : err = %d",err);
                    }
            
#ifndef IPOD_FAST_MODE_SWITCH

                    if(result == IPOD_OK)
                    {
                        if(timeout > 500)
                        {
                            iPodOSSleep(timeout - IPOD_MODE_CHANGE_WAIT_500MS);
                        }
                    }
#endif /* IPOD_FAST_MODE_SWITCH */

                }
                else
                {
                    result = err;
                    IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodWaitForModeSwitchSemaphore() returns : err = %d",err);
                }
            }
            else
            {
                result = IPOD_NOT_CONNECTED;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
            }
        }
        else
        {
            result = IPOD_ERR_PAR;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
        }
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodGetRemoteUIMode(U32 iPodID)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b 1 : Extended interface mode
 * \li \c \b 0 : Simple mode
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function returns the currently set iPod mode.
 */
S32 iPodGetRemoteUIMode(U32 iPodID)
{
    return iPodGetRemoteUIMode_internal(iPodGetHandle(iPodID));
}


S32 iPodGetRemoteUIMode_internal(IPOD_INSTANCE* iPodHndl)
{
    S32 result = IPOD_ERROR;
    U8 msg[] = {IPOD_GET_REMOTE_UI_MODE_CMD};

    if(iPodHndl != NULL)
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturnRemoteUIMode,
                                  (U8)IPOD_LINGO_GENERAL);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == IPOD_OK)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Mode = 0x%02x",result);
                }
            }
        }
        else
        {
            result = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    return result;
}


/*!
 * \fn iPodGetModelNum(U32 iPodID, U8* modelString)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U8* modelString - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the model string will be stored in as a null-terminated UTF-8 character array.
 * \par REPLY PARAMETERS
 * S32 modelId - specifies the model id of the attached iPods or
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function returns the iPods model id and the model string. That information can be used to find out
 * the exact iPod type. e.g generation, storage size, color
 * For detailed information please refer to "iPod Accessory Protocol Interface Specification R39" Table 2-27,
 * Table 2-28 and Table 2-29
 */
S32 iPodGetModelNum(U32 iPodID, U8* modelString)
{
    S32 rc      =  IPOD_ERROR;
    U8 msg[] = {IPOD_GET_MODEL_NUMBER_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (modelString != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturniPodModelNum, (U8)IPOD_LINGO_GENERAL);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (rc == IPOD_OK)
                {
                    rc = (S32)iPod_convert_to_little32(iPodHndl->iAP1Buf);
                    strncpy((VP)modelString, (VP)&iPodHndl->iAP1Buf[IPOD_POS4], (iPodHndl->iAP1MaxPayloadSize - IPOD_POS5));
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Model Number = %s",modelString);
                    modelString[iPodHndl->iAP1MaxPayloadSize - 1] = '\0';
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error parameter iPodHndl = %p,modelString = %p",iPodHndl,modelString);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetLingoProtocolVersion(U32 iPodID, IPOD_LINGO lingo, U8* majorVer, U8* minorVer)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LINGO lingo - the lingo id for which to get the protocol version <br>
 * \par INOUT PARAMETERS
 * U8* majorVer - the protocols major version number <br>
 * U8* minorVer - the protocols minor version number
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
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function retrieves version information for the specified lingo.
 */
S32 iPodGetLingoProtocolVersion(U32 iPodID, IPOD_LINGO lingo,
                                U8* majorVer,
                                U8* minorVer)
{
    return iPodGetLingoProtocolVersion_internal(iPodGetHandle(iPodID), lingo, majorVer, minorVer);
}


S32 iPodGetLingoProtocolVersion_internal(IPOD_INSTANCE* iPodHndl, IPOD_LINGO lingo,
                                         U8* majorVer, U8* minorVer)
{
    U8 msg[]    = {IPOD_GET_LINGO_PROTOCOL_VERSION};
    S32 result  = -1;

    if((iPodHndl != NULL) && (majorVer != NULL) && (minorVer != NULL))
    {
        IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Requested Lingo Id = 0x%02x",lingo);
        /* insert lingo into msg array */
        msg[IPOD_POS4] = (U8)lingo;

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturnLingoProtocolVersion,
                              (U8)IPOD_LINGO_GENERAL);

        result = iPodSendCommand(iPodHndl, msg);
        if (result == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if (result == IPOD_OK)
            {
                *majorVer = iPodHndl->iAP1Buf[IPOD_POS1];
                *minorVer = iPodHndl->iAP1Buf[IPOD_POS2];
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "LingoProtocalVersion is %u.%u",*majorVer,*minorVer);
            }
        }

        if(result != IPOD_OK)
        {
            *majorVer = 0;
            *minorVer = 0;
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error parameter; iPodHndl = %p, majorVer = %p, minorVer = %p",iPodHndl,majorVer,minorVer);
    }

    return result;
}


/*!
 * \fn iPodGetSoftwareVersion(U32 iPodID, U8* majorVer, U8* minorVer, U8* revisionVer)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U8* majorVer - iPod major version number <br>
 * U8* minorVer - iPod minor version number <br>
 * U8* revisionVer - iPod revision number
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
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function retrieves the iPod's software version information.
 */
S32 iPodGetSoftwareVersion(U32 iPodID,
                           U8* majorVer,
                           U8* minorVer,
                           U8* revisionVer)
{
    U8 msg[]    = {IPOD_GET_SOFTWARE_VERSION_CMD};
    S32 result  = -1;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (majorVer != NULL) && (minorVer != NULL) && (revisionVer != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturniPodSoftwareVersion,
                                     (U8)IPOD_LINGO_GENERAL);

            result = iPodSendCommand(iPodHndl, msg);
            if (result == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if (result == IPOD_OK)
                {
                    *majorVer    =  iPodHndl->iAP1Buf[IPOD_POS0];
                    *minorVer    =  iPodHndl->iAP1Buf[IPOD_POS1];
                    *revisionVer =  iPodHndl->iAP1Buf[IPOD_POS2];
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Software version of Apple Device is %u.%u.%u",*majorVer,*minorVer,*revisionVer);
                }
            }

            if(result != IPOD_OK)
            {
                    *majorVer    =  0;
                    *minorVer    =  0;
                    *revisionVer =  0;
            }
        }
        else
        {
            result = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error parameter; iPodHndl = %p, majorVer = %p, minorVer = %p, revisionVer = %p",
                                          iPodHndl,majorVer,minorVer,revisionVer);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodGetSerialNumber(U32 iPodID, U8* serialNumber)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U8* serialNumber - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the serial number string will be stored in as a null-terminated UTF-8 character array.
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
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function retrieves the serial number string of the iPod.
 */
S32 iPodGetSerialNumber(U32 iPodID, U8* serialNumber)
{
    U8 msg[] = {IPOD_GET_SERIAL_NUMBER_CMD};
    S32 result = -1;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (serialNumber != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturniPodSerialNum,
                                     (U8)IPOD_LINGO_GENERAL);
            result = iPodSendCommand(iPodHndl, msg);
            if (result == IPOD_OK)
            {
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, serialNumber);
                serialNumber[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "serialNumber of Device = %s",serialNumber);
            }
            else
            {
                serialNumber[0] = '\0';
            }
        }
        else
        {
            result = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error parameter iPodHndl = %p, serialNumber = %p",iPodHndl,serialNumber);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodGetIPodName(U32 iPodID, U8* iPodName)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U8* iPodName - pointer to a IPOD_RESPONSE_BUF_SIZE byte buffer where the iPod name will be stored in as a null-terminated UTF-8 character array.
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
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function returns the name of the user's iPod or "iPod" if the iPod name is undefined.
 */
S32 iPodGetIPodName(U32 iPodID, U8* iPodName)
{
    S32 rc = -1;
    U8 msg[] = {IPOD_GET_NAME_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (iPodName != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ReturniPodName,
                                     (U8)IPOD_LINGO_GENERAL);

            rc = iPodSendCommand(iPodHndl, msg);
            if (rc == IPOD_OK)
            {
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodName);
                iPodName[iPodHndl->iAP1MaxPayloadSize-1] = '\0';
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Name of Device = %s",iPodName);
            }
            else
            {
                iPodName[0] = '\0';
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error parameter iPodHndl = %p, iPodName = %p",iPodHndl,iPodName);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

#ifndef IPOD_FAKE_AUTHENTICATE
/*!
 * \fn iPodAuthenticateiPod(U32 iPodID)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK iPod authentication successful
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function can be used to confirm that the attached device is actually an iPod.
 * The purpose is to recognize devices pretending to be an iPod or iPods which have a modified firmware, e.g. using Rockbox or iPodLinux.<br>
 * \note Authentication version 2.00 is the only version supported by iPods that can be authenticated by devices.
 */
S32 iPodAuthenticateiPod(U32 iPodID)
{
    S32 rc = IPOD_OK;
    U8 msg1[] = {IPOD_GET_AUTH_INFO_CMD};
    U8 msg2[] = {IPOD_ACK_IPOD_AUTH_INFO_CMD};
    U8 *msg3 = NULL;
    U8 msg4[] = {IPOD_ACK_IPOD_AUTH_SIG_CMD};
    U8 *iPodCertData = NULL;
    U16 iPodCertLen = 0;
    S32 temprc = IPOD_OK;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl == NULL)
    {
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
        return IPOD_ERR_PAR;
    }
    else
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            /* Send GetiPodAutenticationInfo */
            rc = iPodSendCommand(iPodHndl, msg1);
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod not connected");
        }
    }

    if(rc == IPOD_OK)
    {
        /* Receive RetiPodAuthenticationInfo */
        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        if(rc == IPOD_OK)
        {
            rc = AuthenticationInit();
            if(rc == IPOD_OK)
            {
                CertificateInfoType *certData;
                certData = (CertificateInfoType *)((void*)iPodHndl->iAP1Buf);
                iPodCertLen = certData->cert_len;
                iPodCertData = certData->cert_data;
                
                /* Check the Certificate Data */
                rc = AuthenticationSetCertificate(iPodCertLen, iPodCertData);
                temprc = rc;
            }
            
            if(rc != IPOD_OK)
            {
                /* Authentication status is fail */
                msg2[IPOD_POS4] = 1;
            }
            /* Send AckiPodAuthenticationInfo */
            rc = iPodSendCommandNoWaitForACK(iPodHndl, msg2);
            if(temprc != IPOD_OK)
            {
                rc = temprc;
            }
        }
    }

    if(rc == IPOD_OK)
    {
        U16 chalDataLen = 0;
        U8  chalData[IPOD_AUTH_CP_CHALLENGE_DATA_SIZE] = {0};

        /* Get challenge data from Apple CP */
        rc = AuthenticationGetChallengeData(&chalDataLen, chalData);
        IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "AuthenticationGetChallengeData()returns : rc = %d",rc);
        if(rc == IPOD_OK)
        {
            msg3 = (U8 *)calloc(5 + chalDataLen, sizeof(U8));
            if(msg3 != NULL)
            {
                msg3[IPOD_POS0] = IPOD_START_PACKET;
                msg3[IPOD_POS1] = IPOD_GET_AUTH_LEN + (U8)chalDataLen;
                msg3[IPOD_POS2] = 0x00;
                msg3[IPOD_POS3] = (U8)IPOD_GENERAL_LINGO_GetiPodAuthenticationSignature;
                memcpy(&msg3[IPOD_POS4], chalData, chalDataLen);
                msg3[IPOD_POS4 + chalDataLen] = IPOD_SIG_RETRY_COUNT;
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetiPodAuthenticationSignature, (U8)IPOD_LINGO_GENERAL);
                /* Send GetiPodAuthenticationSignature */
                rc = iPodSendCommand(iPodHndl, msg3);
                free(msg3);
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "No Memory msg3 is NULL");
            }
        }
    }

    if(rc == IPOD_OK)
    {
        U16 sigDataLen = 0;
        U8 sigData[IPOD_AUTH_CP_SIGN_DATA_SIZE] = {0};
        /* Recieve RetiPodAuthenticationSignature */
        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        if(rc == IPOD_OK)
        {
            sigDataLen = iPodHndl->pickupInfo.pickup_len;
            if(sigDataLen <= IPOD_AUTH_CP_SIGN_DATA_SIZE)
            {
                memcpy(sigData, iPodHndl->iAP1Buf, sigDataLen);
                /* Check challenge data */
                rc = AuthenticationGetSignature(sigDataLen, sigData);
                IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "AuthenticationGetSignature()returns : rc = %d",rc);
                temprc = rc;
                if(rc != IPOD_OK)
                {
                    /* Authentication status is fail */
                    msg4[IPOD_POS4] = 1;
                }
                /* AckiPodAuthenticationStatus */
                rc = iPodSendCommandNoWaitForACK(iPodHndl, msg4);
                if(temprc != IPOD_OK)
                {
                    rc = temprc;
                }
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "No Memory for storing sigData");
            }
        }
    }
    
    (void)AuthenticationDeinit();

    return rc;
}
#endif /* IPOD_FAKE_AUTHENTICATE */
/*!
 * \fn iPodGetiPodOptions(U32 iPodID, IPOD_OPTIONS_BIT *optionBits)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.05
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * #IPOD_OPTIONS_BIT optionBits* - iPod supports options
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED This commands is not supported by protocol version
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function gets the options of an attached iPod.
 * \note The primary way to obtain information about the capabilities of an attached iPod
 * is to use #iPodGetiPodOptionsForLingo<br>
 * Currenty this function is used for check the support line-out or video output.
 */ 
S32 iPodGetiPodOptions(U32 iPodID, IPOD_OPTIONS_BIT *optionBits)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_OPTIONS_CMD};
    U64 *tmpOption = (U64*) optionBits;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (optionBits != NULL))
    {
        if(iPodHndl->isAPIReady == FALSE)
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
        if(IPOD_OK == rc)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetiPodOptions, (U8)IPOD_LINGO_GENERAL);
            rc = iPodSendCommand(iPodHndl, msg);
        }

        if(IPOD_OK == rc)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if(IPOD_OK == rc)
            {
                *tmpOption = (U64)iPod_convert_to_little64(iPodHndl->iAP1Buf);
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, " iPod options returned by Device is 0x%016llx",*tmpOption);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, optionBits = %p",iPodHndl,optionBits);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID *settingId)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.05
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_PREFERENCE_CLASS_ID classId - request preference class id
 * \par INOUT PARAMETERS
 * #IPOD_PREFERENCE_SETTING_ID settingId* - setting id of specified preference class id
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed!
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_TMOUT operation occurred time out
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED This commands is not supported by protocol version.
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error - The other error code
 * \par DESCRIPTION
 * This function gets the setting id of specified preference class id.<br>
 * Class Id 0x03 is available only if the iPod supports line-out usage.
 */
S32 iPodGetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID *settingId)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_PREFERENCES_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl == NULL) || (settingId == NULL))
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p,settingId = %p ",iPodHndl,settingId);
    }
    else
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetiPodPreferences, (U8)IPOD_LINGO_GENERAL);
        msg[IPOD_POS4] = (U8)classId;
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if((rc == IPOD_OK) && (iPodHndl->iAP1Buf[IPOD_POS0] == (U8)classId))
            {
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "preference class Id = 0x%02x",classId);
                switch(classId)
                {
                    case IPOD_VOUT_SETTING    : settingId->videoOutSetting = (IPOD_VOUT_SETTING_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VSCREEN_CFG     : settingId->screenCfg = (IPOD_VSCREEN_CFG_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VSIG_FORMAT     : settingId->signalFormat = (IPOD_VSIG_FORMAT_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VLINE_OUT_USAGE : settingId->lineOut= (IPOD_VLINE_OUT_USAGE_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VOUT_CONNECT    : settingId->videoOutConnection = (IPOD_VOUT_CONNECT_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VCLOSED_CAP     : settingId->closedCaptioning = (IPOD_VCLOSED_CAP_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VASP_RATIO      : settingId->aspectRatio = (IPOD_VASP_RATIO_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VSUBTITLES      : settingId->subTitles = (IPOD_VSUBTITLES_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    case IPOD_VALT_AUD_CHANNEL: settingId->audioChannel = (IPOD_VALT_AUD_CHANNEL_SETTING_ID)iPodHndl->iAP1Buf[IPOD_POS1];
                                                break;
                    default                   : rc = IPOD_BAD_PARAMETER;
                                                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Invalid classId");
                                                break;
                }
             }
        }
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodSetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID settingId, U8 restore)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.05
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_PREFERENCE_CLASS_ID - is ID for setting<br>
 * #IPOD_PREFERENCE_SETTING_ID- is set ID<br>
 * U8 restore - is dicide whether restore the original setting when iPod is discconected.0x00 is not restore and 0x01 is restore.
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
 * This function set a specific preference.
 * If restore is set to 0x01, iPod restores the original setting for this preference when the iPod is disconnected.
 * If restore is set to 0x00, it does not perform the restore.
 * \note Video preferences must be set after the authentication process has begun and the iPod ctrl send the attach callback(IPOD_OK).<br>
 * The iPod line-out state is always restored.On some iPods, the video-out state is also restored.<br>
 */
S32 iPodSetiPodPreferences(U32 iPodID, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID settingId, U8 restore)
{
    return iPodSetiPodPreferences_internal(iPodGetHandle(iPodID), classId, settingId, restore);
}

S32 iPodSetiPodPreferences_internal(IPOD_INSTANCE* iPodHndl, IPOD_PREFERENCE_CLASS_ID classId, IPOD_PREFERENCE_SETTING_ID settingId, U8 restore)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_IPOD_PREFERENCES_CMD};

    msg[IPOD_POS4] = (U8)classId;
    msg[IPOD_POS6] = restore;

    if(iPodHndl != NULL)
    {
        IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "preference class Id = 0x%02x restore on exit = %u",classId,restore);
        switch(classId)
        {
            case IPOD_VOUT_SETTING    : msg[IPOD_POS5] = (U8)settingId.videoOutSetting;
                                        break;
            case IPOD_VSCREEN_CFG     : msg[IPOD_POS5] = (U8)settingId.screenCfg;
                                        break;
            case IPOD_VSIG_FORMAT     : msg[IPOD_POS5] = (U8)settingId.signalFormat;
                                        break;
            case IPOD_VLINE_OUT_USAGE : msg[IPOD_POS5] = (U8)settingId.lineOut;
                                        break;
            case IPOD_VOUT_CONNECT    : msg[IPOD_POS5] = (U8)settingId.videoOutConnection;
                                        break;
            case IPOD_VCLOSED_CAP     : msg[IPOD_POS5] = (U8)settingId.closedCaptioning;
                                        break;
            case IPOD_VASP_RATIO      : msg[IPOD_POS5] = (U8)settingId.aspectRatio;
                                        break;
            case IPOD_VSUBTITLES      : msg[IPOD_POS5] = (U8)settingId.subTitles;
                                        break;
            case IPOD_VALT_AUD_CHANNEL: msg[IPOD_POS5] = (U8)settingId.audioChannel;
                                        break;
            default                   : rc = IPOD_BAD_PARAMETER;
                                        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Invalid classId");
                                        break;
        }

        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_SetiPodPreferences, (U8)IPOD_LINGO_GENERAL);
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
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    return rc;
}

/*!
 * \fn iPodSetEventNotification(U32 iPodID, U64 mask)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U64 mask - is event notification mask.<br>
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
 * This function sets event notification mask. After execution application should use 
 * iPodGetEventNotification to confirm desired notifications were enabled, 
 * as unsupported notifications will not be enabled.<br>
 * bits 0-1 are reserved (must be 0),<br>
 * bit 2 means Flow control, must be set to 1!<br>
 * bit 3 means Radio tagging status,<br>
 * bit 4 means camera status,<br>
 * bit 5 means Charging info,<br>
 * bits 6-8 are reserved (must be 0),<br>
 * bit 9 means Database changed, must be set to 1!<br>
 * bit 10 means NowPlayingApplicationBundleName,<br>
 * bit 11 means SessionSpaceAvailable,<br>
 * bit 12 is reserved (must be 0),<br>
 * bit 13 means Command completed,<br>
 * bit 14 is reserved (must be 0),<br>
 * bit 15 means iPod Out mode status,<br>
 * bit 16 is reserved (must be 0),<br>
 * bit 17 means BT connection status,<br>
 * bit 18 is reserved (must be 0),<br>
 * bit 19 means NowPlayingApplicationDisplayName,<br>
 * bit 20 means AssistiveTouch status,<br>
 * bits 21-63 are reserved (must be 0).<br>
 */
S32 iPodSetEventNotification(U32 iPodID, U64 mask)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_EVENT_NOTIFICATION};
    U64 setMask = 0;
    U64 supportedMask = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        rc = iPodGetSupportedEventNotification(iPodID, &supportedMask);
        IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "iPodGetSupportedEventNotification() returns : rc = %d",rc);
        if (IPOD_OK == rc)
        {
            setMask = mask & supportedMask; /* Set only supported notifications */
        }
        else
        {
            setMask = mask & IPOD_MIN_NOTIFICATION_MASK;
        }
        rc = IPOD_OK;
        IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "set event notification mask = 0x%016llx",setMask);
        iPod_convert_to_big64(&msg[IPOD_POS4], setMask);

        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
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
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetiPodOptionsForLingo(U32 iPodID, IPOD_LINGO lingo, U64 *optionBits)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LINGO lingo - is specific Command Lingo.<br>
 * \par INOUT PARAMETERS
 * U64 *optionBits - is supported Lingo option.
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
 * This function gets iPod Options.
 * if Lingo ID 0, bit 0 means Line out usage, bit 1 means video output, 
 * bit 2 means NTSC video signal format, and so on.<br>
 */
S32 iPodGetiPodOptionsForLingo(U32 iPodID, IPOD_LINGO lingo, U64 *optionBits)
{
    return iPodGetiPodOptionsForLingo_internal(iPodGetHandle(iPodID), lingo, optionBits);
}

S32 iPodGetiPodOptionsForLingo_internal(IPOD_INSTANCE* iPodHndl, IPOD_LINGO lingo, U64 *optionBits)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_OPTIONS_FOR_LINGO};

    msg[IPOD_POS4] = (U8)lingo;
    
    if((iPodHndl != NULL) && (optionBits != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetiPodOptionsForLingo, (U8)IPOD_LINGO_GENERAL);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            if(rc == IPOD_OK)
            {
                if((U8)lingo == iPodHndl->iAP1Buf[IPOD_POS0])
                {
                    *optionBits = iPod_convert_to_little64(&iPodHndl->iAP1Buf[IPOD_POS1]);
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, " iPod option returned by Device for lingo Id 0x%02x is 0x%016llx",lingo,*optionBits);
                }
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, optionBits = %p",iPodHndl,optionBits);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "lingo Id 0x%02x,rc = %d",lingo,rc);
    return rc;
}

/*!
 * \fn iPodGetEventNotification(U32 iPodID, U64 *eventMask)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U64 *eventMask - is currently enabled notification.
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
 * This function gets the event notification.<br>
 * bits 0-1 are reserved (must be 0),<br>
 * bit 2 means Flow control, must be set to 1!<br>
 * bit 3 means Radio tagging status,<br>
 * bit 4 means camera status,<br>
 * bit 5 means Charging info,<br>
 * bits 6-8 are reserved (must be 0),<br>
 * bit 9 means Database changed,<br>
 * bit 10 means NowPlayingApplicationBundleName,<br>
 * bit 11 means SessionSpaceAvailable,<br>
 * bit 12 is reserved (must be 0),<br>
 * bit 13 means Command completed,<br>
 * bit 14 is reserved (must be 0),<br>
 * bit 15 means iPod Out mode status,<br>
 * bit 16 is reserved (must be 0),<br>
 * bit 17 means BT connection status,<br>
 * bit 18 is reserved (must be 0),<br>
 * bit 19 means NowPlayingApplicationDisplayName,<br>
 * bit 20 means AssistiveTouch status,<br>
 * bits 21-63 are reserved (must be 0).<br>
 */
S32 iPodGetEventNotification(U32 iPodID, U64 *eventMask)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_EVENT_NOTIFICATION};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (eventMask != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetEventNotification, (U8)IPOD_LINGO_GENERAL);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    *eventMask = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, " EventNotification = 0x%016llx",*eventMask);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, eventMask = %p",iPodHndl,eventMask);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetSupportedEventNotification(U32 iPodID, U64 *eventMask)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U64 *eventMask - is supported event by iPod.
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
 * This function gets iPod's support for notifications.<br>
 * bits 0-1 are reserved (must be 0),<br>
 * bit 2 means Flow control, must be set to 1!<br>
 * bit 3 means Radio tagging status,<br>
 * bit 4 means camera status,<br>
 * bit 5 means Charging info,<br>
 * bits 6-8 are reserved (must be 0),<br>
 * bit 9 means Database changed,<br>
 * bit 10 means NowPlayingApplicationBundleName,<br>
 * bit 11 means SessionSpaceAvailable,<br>
 * bit 12 is reserved (must be 0),<br>
 * bit 13 means Command completed,<br>
 * bit 14 is reserved (must be 0),<br>
 * bit 15 means iPod Out mode status,<br>
 * bit 16 is reserved (must be 0),<br>
 * bit 17 means BT connection status,<br>
 * bit 18 is reserved (must be 0),<br>
 * bit 19 means NowPlayingApplicationDisplayName,<br>
 * bit 20 means AssistiveTouch status,<br>
 * bits 21-63 are reserved (must be 0).<br>
 */
S32 iPodGetSupportedEventNotification(U32 iPodID, U64 *eventMask)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SUPPORTED_EVENT_NOTIFICATION};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (eventMask != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_GetSupportedEventNotification, (U8)IPOD_LINGO_GENERAL);
            rc = iPodSendCommand(iPodHndl, msg);
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    *eventMask = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, " SupportedEventNotification = 0x%016llx",*eventMask);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, eventMask = %p",iPodHndl,eventMask);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodRetAccStatusNotification(U32 iPodID, U32 statusMask)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U32 statusMask - is needed status notification.
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
 * Status mask sets supported event in accessory.
 */
S32 iPodRetAccStatusNotification(U32 iPodID, U32 statusMask)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_RET_ACC_STATUS_NOTIFICATION};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Status Notification mask = %u",statusMask);
    iPod_convert_to_big32(&msg[IPOD_POS4], statusMask);
    rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
    return rc;
}
/*!
 * \fn iPodAccStatusNotification(U32 iPodID, IPOD_ACC_STATUS_TYPE type, IPOD_ACC_STATUS_PARAM *status, U16 len)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_ACC_STATUS_TYPE type - This is type to notify status<br>
 * U8 *param - This is current status of indicated status type.<br>
 * U8 paramLen - This is length of param of parameter <br>
 * \par INOUT PARAMETERS
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
 * This function is used to notify the status to iPod.<br>
 * Notify status is decided by previously iPodSetAccussoryStatusNotification.<br>
 * This function should not send too frequently and must not send no more often than once every 500ms.<br>
 * Immediately after calling a #iPodRetAccStatusNotification function, the accessory must send an initial
 * #iPodAccStatusNotification command for each notification. <br>
 */
S32 iPodAccStatusNotification(U32 iPodID, IPOD_ACC_STATUS_TYPE type, IPOD_ACC_STATUS_PARAM *status, U16 len)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U32 btcount = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (status != NULL))
    {
        msg = (U8 *)calloc((IPOD_ACC_STATUS_BASE_LEN +IPOD_START_LENGTH + len), sizeof(U8));
        if(msg != NULL)
        {
            msg[IPOD_POS0] = IPOD_START_PACKET;
            msg[IPOD_POS1] = (U8)(IPOD_ACC_STATUS_BASE_LEN + len);
            msg[IPOD_POS2] = (U8)IPOD_LINGO_GENERAL;
            msg[IPOD_POS3] = IPOD_GENERAL_LINGO_iPodAccStatusNotification;
            msg[IPOD_POS4] = (U8)type;
            
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Status Type = 0x%02x",type);
            if(type == IPOD_ACC_STATUS_FAULT)
            {
                msg[IPOD_POS5] = status->fault.type;
                msg[IPOD_POS6] = status->fault.condition;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory: fault type = 0x%02x fault condition = 0x%02x",status->fault.type,status->fault.condition);
            }
            else if(type == IPOD_ACC_STATUS_BT_AUTO_PAIRING_CONNECTION_STATUS)
            {
                for(btcount = 0; btcount < status->btautopairing_count; btcount++)
                {
                    iPod_convert_to_big32(&msg[IPOD_POS5  + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ], status->btautopairing[btcount].devtype);
                    iPod_convert_to_big32(&msg[IPOD_POS9  + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ], status->btautopairing[btcount].devstate);
                    iPod_convert_to_big32(&msg[IPOD_POS13 + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ], status->btautopairing[btcount].devclass);
                    msg[IPOD_POS17 + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ] = status->btautopairing[btcount].pairingtype;
                    memcpy(&msg[IPOD_POS19 + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ], status->btautopairing[btcount].pincode, 16);
                    msg[IPOD_POS34 + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ] = '\0';
                    iPod_convert_macaddr_to_bigendian(&msg[IPOD_POS35 + (btcount * IPOD_BT_AUTO_PAIRING_STATUS_NOTIFICATION_LEN) ], status->btautopairing[btcount].macaddress);
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Device type = 0x%08x Device state = 0x%08x Device class = %u ",
                                                     status->btautopairing[btcount].devtype,status->btautopairing[btcount].devstate,status->btautopairing[btcount].devclass);
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Pairing type = 0x%08x PIN code = %s MAC address = %s",
                                                     status->btautopairing[btcount].pairingtype,status->btautopairing[btcount].pincode,status->btautopairing[btcount].macaddress);
                }
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Bad Parameter - Unknown Accessory Status");
            }
            rc = iPodSendCommand(iPodHndl, msg);
            free(msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, status = %p",iPodHndl,status);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodCancelCommand(U32 iPodID, IPOD_CANCEL_COMMAND_TYPE command)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
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
 * This function is used to cancel the multiple data transfer.<br>
 * It can be used by setting bit 19 in the IDPS Accessory capabilities.<br>
 * This function may be used to cancel only the below commands.<br>
 * - iPodGetTrackArtworkData(Extended Lingo command).<br>
 */
S32 iPodCancelCommand(U32 iPodID, IPOD_CANCEL_COMMAND_TYPE command)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_CANCEL_COMMAND};
    U8 lingoId = 0;
    U16 transId = 0;
    U16 commandId = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            if(command == IPOD_REMOTE_ARTWORK_DATA)
            {
                lingoId = IPOD_LINGO_DISPLAY_REMOTE;
                /* cancel GetTrackArtworkData */
                commandId = 0x18;
            }
            else
            {
                lingoId = IPOD_LINGO_EXTENDED_INTERFACE;
                if(command == IPOD_RETRIEVE_CATEGORIZED_DB_RECORDS)
                {
                    /* cancel RetrieveCategorizedDatabaseRecords */
                    commandId = 0x1A;
                }
                else
                {
                    /* cancel GetTrackArtworkData */
                    commandId = 0x10;
                }

            }
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Lingo Id = 0x%02x Command Id = 0x%04x Transaction Id = 0x%04x",lingoId,commandId,transId);
            msg[IPOD_POS4] = lingoId;
            transId = iPodHndl->artwork.transID;
            iPod_convert_to_big16(&msg[IPOD_POS5], commandId);
            iPod_convert_to_big16(&msg[IPOD_POS7], transId);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
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
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;

}

/*!
 * \fn iPodRequestAppLaunch(U32 iPodID, U8 *bundleId, U8 length)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 *bundleId - Null-terminated the Application ID.
 * U8 length - Length of bundleId
 * \par INOUT PARAMETERS
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
 * This function is used that launch the specific application.<br>
 * If ack status is IPOD_OK, launch is possible. but it does not guarantee that the
 * application is running at that time.
 */
S32 iPodRequestAppLaunch(U32 iPodID, U8 *bundleId, U8 length)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U16 maxLen = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    maxLen = IPOD_START_LENGTH + IPOD_REQ_APP_LAUNCH + length;
    if((bundleId != NULL) && (iPodHndl != NULL) && (length != 0))
    {
        if(bundleId[length - 1] != '\0')
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Bad Parameter Last Character of Bundle ID = %c, it should be null",bundleId[length - 1]);
        }
        
        if(maxLen >= IPOD_SHORT_WRITE_SIZE)
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Bad Parameter- maxLen = %d, should be greater than 256",maxLen);
        }
        
        if(rc == IPOD_OK)
        {
            if(iPodHndl->isAPIReady != FALSE)
            {
                msg = (U8 *)calloc(IPOD_START_LENGTH + IPOD_REQ_APP_LAUNCH + length, sizeof(U8));
                if(msg != NULL)
                {
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "BundleId of App to be launched = %s",bundleId);
                    msg[IPOD_POS0] = IPOD_START_PACKET;
                    msg[IPOD_POS1] = IPOD_REQ_APP_LAUNCH + length;
                    msg[IPOD_POS2] = (U8)IPOD_LINGO_GENERAL;
                    msg[IPOD_POS3] = IPOD_GENERAL_LINGO_iPodRequestAppLaunch;
                    msg[IPOD_POS4] = 0x00; /* reserved value according to spec R44 */
                    msg[IPOD_POS5] = 0x02; /* reserved value according to spec R44 for iOS 5.X devices */
                    msg[IPOD_POS6] = 0x00; /* reserved value according to spec R44 */
                    memcpy(&msg[IPOD_POS7], bundleId, length);
                    
                    iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
                    rc = iPodSendCommand(iPodHndl, msg);
                    if(rc == IPOD_OK)
                    {
                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                    }
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_GENERAL_LOG(DLT_LOG_ERROR, "msg is NULL");
                }
                if(msg != NULL)
                {
                    free(msg);
                }
            }
            else
            {
                rc = IPOD_NOT_CONNECTED;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter- bundleId = %p iPodHndl = %p, length = %d",bundleId,iPodHndl,length);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetNowPlayingFocusApp(U32 iPodID, U8 *focusApp, U16 length)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U8 *focusApp - Current focus Application Bundle ID string.
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
 * This function is used that know current now playing focus.
 */
S32 iPodGetNowPlayingFocusApp(U32 iPodID, U8 *focusApp, U16 length)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_NOW_PLAYING_FOCUS_APP};
    U8 size = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((focusApp != NULL) && (iPodHndl != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_iPodRetNowPlayingFocusApp, (U8)IPOD_LINGO_GENERAL);
            rc = iPodSendCommand(iPodHndl, msg);
            IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns : rc = %d",rc);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    size = (U8)(strlen((VP)iPodHndl->iAP1Buf) + 1);
                    if(size < length)
                    {
                        memcpy(focusApp, iPodHndl->iAP1Buf, size);
                    }
                    else
                    {
                        memcpy(focusApp, iPodHndl->iAP1Buf, length);
                        focusApp[length - 1] = '\0';
                    }
                    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Now Playing focus app = %s",focusApp);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, focusApp = %p",iPodHndl,focusApp);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodRequestTransportMaxPacketSize(U32 iPodID, U16 *size)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * U16 *size - This is maximum packet size of transport.
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
 * This function is used that accessory knows the iPod to determine the maximum allowable payload size per packet.
 */
S32 iPodRequestTransportMaxPacketSize(U32 iPodID, U16 *size)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_REQ_TRANSPORT_MAX_PACKET_SIZE};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((size != NULL) && (iPodHndl != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_iPodRetTransportMaxPacketSize, (U8)IPOD_GENERAL_LINGO);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            *size = iPod_convert_to_little16(iPodHndl->iAP1Buf);
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Maximum payload size returned by Apple Device is size = %d",*size);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, size = %p",iPodHndl,size);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodDevDataTransfer(U32 iPodID, U16 sessionId, U8 *data, U16 dataLen)
 * \par GENERAL LINGO PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U16 sessionId - This is used to communicate with application.<br>
 * U8 *data - This send to application.<br>
 * U16 dataLen - length of data.<br>
 * \par INOUT PARAMETERS
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
 * This function is used that accessory communicate to application.<br>
 * Accessory must not send another this function until the previous one finish.<br>
 * If accessory received iPodNotification for flow control, accessory must not send any commands during the specified waiting time.<br>
 * The accessory must not try to send data with payload length greater than 500 bytes.<br>
 * If this function uses when session ID is closed or doesn't exist, iPod reply UnKnown Category Status.
 * 
 */
S32 iPodDevDataTransfer(U32 iPodID, U16 sessionId, U8 *data, U16 dataLen)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U16 totalLen = 0;
    U16 msgLen = 0;
    U8 markerLen = 0;
    U8 idps = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((data != NULL) && (iPodHndl != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            msgLen = IPOD_DEV_DATA_TRANSFER_LEN + dataLen;
            totalLen = msgLen + IPOD_TRANSID_LENGTH + IPOD_CHECKSUM_BYTE_LEN;

            idps = iPodHndl->rcvMsgInfo.startIDPS;
            /* totalLen shorter than 256 */
            if(((totalLen < IPOD_SHORT_WRITE_SIZE) && (idps == FALSE))
                || ((totalLen + 2 < IPOD_SHORT_WRITE_SIZE) && (idps != FALSE)))
            {
                msg = (U8 *)calloc(msgLen + IPOD_START_LENGTH, sizeof(U8));
            }
            /* totalLen greater than 256 and shorter than 500 */
            else
            {
                msg = (U8 *)calloc(msgLen + IPOD_SEND_LONG_MARKER + IPOD_START_LENGTH, sizeof(U8));
                markerLen = IPOD_SEND_LONG_MARKER;
            }
            
            if(msg != NULL)
            {
                msg[IPOD_POS0] = IPOD_START_PACKET;
                if(markerLen == IPOD_SEND_LONG_MARKER)
                {
                    msg[IPOD_POS1] = 0x00;
                    iPod_convert_to_big16(&msg[IPOD_POS2], msgLen);
                }
                else
                {
                    msg[IPOD_POS1] = (U8)msgLen;
                }
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Session Id = %d",sessionId);
                msg[markerLen + IPOD_POS2] = (U8)IPOD_LINGO_GENERAL;
                msg[markerLen + IPOD_POS3] = (U8)IPOD_GENERAL_LINGO_iPodDevDataTransfer;
                iPod_convert_to_big16(&msg[markerLen + IPOD_POS4], sessionId);
                if((msgLen + markerLen + IPOD_START_LENGTH) >= (markerLen + IPOD_POS6 + dataLen)) 
                {
                    /* PRQA: Lint Message 669: The allocated memory size was checked and found to be okay. */
                    memcpy(&msg[markerLen + IPOD_POS6], data, dataLen); /*lint !e669 */
                }
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_GENERAL_LOG(DLT_LOG_ERROR, "No Memory - msg is NULL");
            }
            
            if(rc == IPOD_OK)
            {
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
                if(markerLen == IPOD_SEND_LONG_MARKER)
                {
                    rc = iPodSendLongTelegram(iPodHndl, msg);
                    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "iPodSendLongTelegram() returns : rc = %d",rc);
                }
                else
                {
                    rc = iPodSendCommand(iPodHndl, msg);
                }
            }
            
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
            
            if(msg != NULL)
            {
                free(msg);
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, data = %p",iPodHndl,data);
    }
    
    return rc;
}

S32 iPodGetUIMode(U32 iPodID, U8 *mode)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GETUI_MODE};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl != NULL) && (mode != NULL))
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_iPodRetUIMode, (U8)IPOD_LINGO_GENERAL);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
          rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            *mode = iPodHndl->iAP1Buf[IPOD_POS0];
            IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "mode = 0x%02x",*mode);
        }
    }
    else
    {
      rc = IPOD_ERR_PAR;
      IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, mode = %p",iPodHndl,mode);
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

S32 iPodSetUIMode(U32 iPodID, U8 mode)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SETUI_MODE};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "mode = 0x%02x",mode);
        msg[IPOD_POS4] = mode;
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
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
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*\}*/

/* ========================================================================== */
/* private helper functions                                                   */
/* ========================================================================== */

S32 iPodIsInAdvancedMode(IPOD_INSTANCE* iPodHndl)
{
    S32 rc    = IPOD_OK;
#ifdef IPOD_MODE_CHECK
    U8  count   =   0;
#endif /* IPOD_MODE_CHECK*/

    if(iPodHndl != NULL)
    {
        if (iPodHndl->isAPIReady == FALSE)  //aditj
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

#ifdef IPOD_MODE_CHECK
    if(rc == IPOD_OK)
    {
        do
        {
            rc = iPodGetRemoteUIMode_internal(iPodHndl);
            count++;
        }while ((rc < 1) && (rc != IPOD_NOT_CONNECTED) && (count <= IPOD_MAX_TRY_COUNT));
    }
    if (rc > 0)
    {
        rc = IPOD_OK;
    }
    else if(rc == 0)
    {
        rc = IPOD_ERR_ONLY_IN_ADVANCEDMODE;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "Operation is only available in advanced mede");
    }
    else
    {
    }
#endif /* IPOD_MODE_CHECK */

    return rc;
}
/*!
 * \fn iPodRetAccessoryInfo(IPOD_INSTANCE* iPodHndl, const U8* iPodData)
 * \par INPUT PARAMETERS
 * U8* iPodData - pointer to the buffer containing the data from the iPod's getAccessoryInfo request
 * \par INOUT PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function is called from the iPodReaderTask and sends the retAccessoryInfo command to the iPod.
 */
void iPodRetAccessoryInfo(IPOD_INSTANCE* iPodHndl, const U8* iPodData)
{
    IPOD_ACCESSORY_INFO_TYPES infoType;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();
    infoType = (IPOD_ACCESSORY_INFO_TYPES) iPodData[IPOD_POS0];

    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory info type = 0x%02x",infoType);
    switch(infoType)
    {
        case IPOD_ACC_INFO_CAPABILITIES:
        {
            /* JIRA[SWGII-4801] */
            /* AccessoryInfo notify kind of information same as IDPS */
            U8 i = 0;
            U8 msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE0_CMD};
            U32 caps = IPOD_ACC_BIT_INFO_CAPABILITIES;
            msg[IPOD_POS4] = (U8)IPOD_ACC_INFO_CAPABILITIES;
            for(i = 0; i < IPOD_RET_ACCINFO_MAX; i++)
            {
                if(((U8)((S32 *)(VP)dcInfo[IPOD_DC_ACCINFO_FLG].para.p_val)[i]) != 0)
                {
                    caps = caps | (U32)(1 << i);
                }
            }
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory info capabilities = 0x%04x",caps);
            iPod_convert_to_big32(&msg[IPOD_POS5], caps);

            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_NAME:
        {
            U8  msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE1678_CMD};
            U8 len = (U8)strlen((VP)dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val) + 1;
            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_NAME;

            /* insert accessory name and payload length into msg array */
            msg[IPOD_POS1] = len + IPOD_ACC_INFO_BASE_LENGTH;

            /* it is limited as 64bytes by apple spec, therefore size must be checked */
            if(len <= (sizeof(msg) - IPOD_POS5))
            {
                memcpy(&msg[IPOD_POS5], dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val, len);
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Name = %s",dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val);
            }
            else
            {
                msg[IPOD_POS1] -= len -1;
                msg[IPOD_POS5] = '\0';
            }
            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
            
        }

        case IPOD_ACC_MIN_SUPP_IPOD_FIRMWARE_VER:
        {
            U8 msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE2_CMD};

            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_MIN_SUPP_IPOD_FIRMWARE_VER;

            msg[IPOD_POS5] = iPodData[IPOD_POS1];
            msg[IPOD_POS6] = iPodData[IPOD_POS2];
            msg[IPOD_POS7] = iPodData[IPOD_POS3];
            msg[IPOD_POS8] = iPodData[IPOD_POS4];

            msg[IPOD_POS9] = (U8)IPOD_SUP_FW_VER_MAJ;
            msg[IPOD_POS10] = (U8)IPOD_SUP_FW_VER_MIN;
            msg[IPOD_POS11] = (U8)IPOD_SUP_FW_VER_REV;

            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Minimum Supported iPod firmware version = %u.%u.%u",IPOD_SUP_FW_VER_MAJ,IPOD_SUP_FW_VER_MIN,IPOD_SUP_FW_VER_REV);
            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_MIN_SUPP_LINGO_VER:
        {
            U8 msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE45_CMD};
            IPOD_LINGO requestedLingo;

            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_MIN_SUPP_LINGO_VER;

            requestedLingo = (IPOD_LINGO) iPodData[IPOD_POS1];

            msg[IPOD_POS5] = (U8)requestedLingo;

            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Lingo Id = 0x%02x",requestedLingo);
            switch(requestedLingo)
            {
              case IPOD_LINGO_GENERAL:
              {
                msg[IPOD_POS6] = IPOD_SUP_GENERALLINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_GENERALLINGO_MIN;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Major Protocol Version = %u Minor Protocol Version = %u",IPOD_SUP_GENERALLINGO_MAJ,IPOD_SUP_GENERALLINGO_MIN);

                break;
              }
              case IPOD_LINGO_EXTENDED_INTERFACE:
              {
                msg[IPOD_POS6] = IPOD_SUP_EXTENDEDLINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_EXTENDEDLINGO_MIN;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Major Protocol Version = %u Minor Protocol Version = %u",IPOD_SUP_EXTENDEDLINGO_MAJ,IPOD_SUP_EXTENDEDLINGO_MIN);

                break;
              }
              case IPOD_LINGO_DIGITAL_AUDIO:
              {
                msg[IPOD_POS6] = IPOD_SUP_AUDIOLINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_AUDIOLINGO_MIN;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Major Protocol Version = %u Minor Protocol Version = %u",IPOD_SUP_AUDIOLINGO_MAJ,IPOD_SUP_AUDIOLINGO_MIN);

                break;
              }
              case IPOD_LINGO_STORAGE:
              {
                msg[IPOD_POS6] = IPOD_SUP_STORAGELINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_STORAGELINGO_MIN;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Major Protocol Version = %u Minor Protocol Version = %u",IPOD_SUP_STORAGELINGO_MAJ,IPOD_SUP_STORAGELINGO_MIN);


                break;
              }
              case IPOD_LINGO_LOCATION:
              {
                msg[IPOD_POS6] = IPOD_SUP_LOCATIONLINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_LOCATIONLINGO_MIN;
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Major Protocol Version = %u Minor Protocol Version = %u",IPOD_SUP_LOCATIONLINGO_MAJ,IPOD_SUP_LOCATIONLINGO_MIN);
              
                break;
              }
              default:
              {
                msg[IPOD_POS6] = IPOD_SUP_DEFAULTLINGO_MAJ;
                msg[IPOD_POS7] = IPOD_SUP_DEFAULTLINGO_MIN;

                break;
              }
            }

            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_FIRMWARE_VERSION:
        {
            U8 msg_cmp[] = {IPOD_RET_ACCESSORY_INFO_TYPE45_CMD};
            LOCAL U8 msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE45_CMD};
            if(memcmp(msg, msg_cmp, sizeof(msg_cmp)) == 0)
            {
                /* insert accessory info type into msg array */
                msg[IPOD_POS4] = (U8)IPOD_ACC_FIRMWARE_VERSION;

                /* insert major-, minor- and revison version number into msg array */
                msg[IPOD_POS5] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[IPOD_POS0];
                msg[IPOD_POS6] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[IPOD_POS1];
                msg[IPOD_POS7] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_FW_VER].para.p_val)[IPOD_POS2];
                IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Firmware Version = %u.%u.%u",msg[IPOD_POS5],msg[IPOD_POS6],msg[IPOD_POS7]);
            }

            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_HARDWARE_VERSION:
        {
            U8 msg [] = {IPOD_RET_ACCESSORY_INFO_TYPE45_CMD};
            
            msg[IPOD_POS4] = (U8)IPOD_ACC_HARDWARE_VERSION;
            msg[IPOD_POS5] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[IPOD_POS0];
            msg[IPOD_POS6] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[IPOD_POS1];
            msg[IPOD_POS7] = (U8)((S32 *)(VP)dcInfo[IPOD_DC_HW_VER].para.p_val)[IPOD_POS2];
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Hardware Version = %u.%u.%u",msg[IPOD_POS5],msg[IPOD_POS6],msg[IPOD_POS7]);
            iPodSendCommandNoWaitForACK(iPodHndl, msg);
            break;
        }

        case IPOD_ACC_MANUFACTURER:
        {
            U8  msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE1678_CMD};
            U8 len = (U8)strlen((VP)dcInfo[IPOD_DC_MAN].para.p_val) + 1;
            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_MANUFACTURER;

            /* insert accessory name and payload length into msg array */
            msg[IPOD_POS1] = len + IPOD_ACC_INFO_BASE_LENGTH;

            /* it is limited as 64bytes by apple spec, therefore size must be checked */
            if(len <= sizeof(msg) - IPOD_POS5)
            {
                memcpy(&msg[IPOD_POS5], dcInfo[IPOD_DC_MAN].para.p_val, len);
            }
            else
            {
                msg[IPOD_POS1] -= len -1;
                msg[IPOD_POS5] = '\0';
            }
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Manufacturer = %s",dcInfo[IPOD_DC_MAN].para.p_val);

            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_MODEL_NUMBER:
        {
            U8  msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE1678_CMD};
            U8 len = (U8)strlen((VP)dcInfo[IPOD_DC_MODEL].para.p_val) + 1;
            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_MODEL_NUMBER;

            /* insert accessory name and payload length into msg array */
            msg[IPOD_POS1] = len + IPOD_ACC_INFO_BASE_LENGTH;

            /* it is limited as 64bytes by apple spec, therefore size must be checked */
            if(len <= sizeof(msg) - IPOD_POS5)
            {
                memcpy(&msg[IPOD_POS5], dcInfo[IPOD_DC_MODEL].para.p_val, len);
            }
            else
            {
                msg[IPOD_POS1] -= len -1;
                msg[IPOD_POS5] = '\0';
            }
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Model Number = %s",dcInfo[IPOD_DC_MODEL].para.p_val);

            iPodSendCommandNoWaitForACK(iPodHndl, msg);

            break;
        }

        case IPOD_ACC_SERIAL_NUMBER:
        {
            U8  msg[] = {IPOD_RET_ACCESSORY_INFO_TYPE1678_CMD};
            U8 len = (U8)strlen((VP)dcInfo[IPOD_DC_SERIAL].para.p_val) + 1;
            /* insert accessory info type into msg array */
            msg[IPOD_POS4] = (U8)IPOD_ACC_SERIAL_NUMBER;

            /* insert accessory name and payload length into msg array */
            msg[IPOD_POS1] = len + IPOD_ACC_INFO_BASE_LENGTH;

            /* it is limited as 64bytes by apple spec, therefore size must be checked */
            if(len <= sizeof(msg) - IPOD_POS5)
            {
                memcpy(&msg[IPOD_POS5], dcInfo[IPOD_DC_SERIAL].para.p_val, len);
            }
            else
            {
                msg[IPOD_POS1] -= len -1;
                msg[IPOD_POS5] = '\0';
            }
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Serial Number = %s",dcInfo[IPOD_DC_SERIAL].para.p_val);

            iPodSendCommandNoWaitForACK(iPodHndl, msg);
            
            break;
        }

        case IPOD_ACC_INCOMING_MAX_PAYLOAD_SIZE:
        {
            U8  msg[]       = {IPOD_RET_ACCESSORY_INFO_TYPE9_CMD};

            /* insert max payload len into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS5], iPodHndl->iAP1MaxPayloadSize);

            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Maximum Incoming Payload Size = %d",iPodHndl->iAP1MaxPayloadSize);
            iPodSendCommandNoWaitForACK(iPodHndl, msg);
            /* JIRA[SWGII-4803] */
            /* This is set to notice the end of accessoryInfo */
            /* IdentifyDeviceLingoes is waiting for this flag. This flag is ignored in workertask */
            iPodWorkerSecondIdentify();
            break;
        }
        
        case IPOD_ACC_STATUS_TYPES_SUPPORTED:
        {
            U8 msg[] = {IPOD_RET_ACCESSORY_INFO_TYPEB_CMD};
            
            iPod_convert_to_big32(&msg[IPOD_POS5], (U32)dcInfo[IPOD_DC_ACC_STATUS].para.val);
            IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "Accessory Status Types = 0x%02x",dcInfo[IPOD_DC_ACC_STATUS].para.val);
            iPodSendCommandNoWaitForACK(iPodHndl, msg);
            break;
        }
        
        default:
        {
            IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "Unknown Accessory info type");
            break;
        }
    }
}

S32 iPodSessionDevAck(IPOD_INSTANCE* iPodHndl, U8 cmdId, U8 status)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SESSION_DEV_ACK};
    
    msg[IPOD_POS4] = status;
    msg[IPOD_POS5] = cmdId;
    IAP1_GENERAL_LOG(DLT_LOG_VERBOSE, "status = 0x%02x command Id = 0x%02x",status,cmdId);
    rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
    
    return rc;
}
