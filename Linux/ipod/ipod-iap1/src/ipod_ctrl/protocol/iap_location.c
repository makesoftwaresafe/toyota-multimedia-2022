#include "iap_location.h"
#include "iap_types.h"
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_transport_message.h"
#include "iap1_dlt_log.h"
#include <stdlib.h>
#include <string.h>


LOCAL S32 iPodMultiRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length);
LOCAL S32 iPodMultiAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length);

/**
 * \addtogroup Location_commands
 */
/*\{*/

/*!
 * \fn iPodRetDevControl(U32 iPodID, IPOD_LOCATION_TYPE, U64 ctrlData)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - The location control type.
 * U64 ctrlData - System control data.
 * \par REPLAY PARAMETERS
 * S32 rc - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is used in response to a GetDevDataControl command received from the iPod.<br>
 * System control support is required for every accessory.<br>
 */
S32 iPodRetDevControl(U32 iPodID, IPOD_LOCATION_TYPE locType, U64 ctlData)
{
    S32 rc = 0;
    U8 msg[] = {IPOD_RET_DEV_CONTROL_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (locType < LOCATION_TYPE_MAX))
    {
        IAP1_LOCATION_LOG(DLT_LOG_VERBOSE, "location type = 0x%02x ctlData = 0x%016llx",locType,ctlData);
        msg[IPOD_POS4] = (U8)locType;
        iPod_convert_to_big64(&msg[IPOD_POS5], ctlData);
        rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p, locType = %d",iPodHndl,locType);
    }
    IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodMultiRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - The location control type.<br>
 * U8 dataType - Location data type<br>
 * U32 locSize - Size of location data.<br>
 * U16 sectCur - Current payload section index<br>
 * U16 sectMax - Maximum payload section index<br>
 * U16 length - Total size of locData. If multiple packets, only in first packet included<br>
 * \par INOUT PARAMETERS
 * U8 locData* - Data of requested locType and dataType.
 * \par REPLAY PARAMETERS
 * S32 rc -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * Helper function of #iPodRetDevData for Multisection Data Transfer.<br>.
 */
LOCAL S32 iPodMultiRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 marker = 0;
    U16 baseLen = IPOD_RET_DEV_DATA_LEN;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (locData != NULL))
    {
        if(sectMax != 0)
        {
            baseLen -= IPOD_TOTAL_SIZE_LEN;
        }

        /* Check Long or Short telegram */
        if(length >= IPOD_SHORT_WRITE_SIZE)
        {
            /* marker byte + length byte */
            marker += IPOD_SEND_LONG_MARKER;
        }
        
        msg = (U8 *)calloc((baseLen + length + marker), sizeof(U8));
        if(msg != NULL)
        {
            IAP1_LOCATION_LOG(DLT_LOG_VERBOSE, "location type = 0x%02x dataType = 0x%02x sectCur = 0x%04x sectMax = 0x%04x",locType,dataType,sectCur,sectMax);
            msg[IPOD_POS0] = IPOD_START_PACKET;
            if(marker == 0)
            {
                /* Short telegram */
                msg[IPOD_POS1] = (U8)(baseLen + length - IPOD_TRANSID_LENGTH);
            }
            else
            {
                /* Long telegram */
                msg[IPOD_POS1] = 0;
                iPod_convert_to_big16(&msg[IPOD_POS2], (U16)(((baseLen + length) -IPOD_TRANSID_LENGTH) + IPOD_SEND_LONG_MARKER));
            }

            msg[IPOD_POS2 + marker] = (U8)IPOD_LINGO_LOCATION;
            msg[IPOD_POS3 + marker] = IPOD_LOCATION_LINGO_RetDevData;
            msg[IPOD_POS4 + marker] = (U8)locType;
            msg[IPOD_POS5 + marker] = dataType;
            iPod_convert_to_big16(&msg[IPOD_POS6 + marker], sectCur);
            iPod_convert_to_big16(&msg[IPOD_POS8 + marker], sectMax);
            if(sectCur == 0)
            {
                /* First msg */
                iPod_convert_to_big32(&msg[IPOD_POS10 + marker], locSize);
                memcpy(&msg[IPOD_POS14 + marker], locData, length);
            }
            else
            {
                /* Greater than second msg */
                memcpy(&msg[IPOD_POS10 + marker], locData, length);
            }

            iPodSetExpectedCmdId(iPodHndl, (U8)IPOD_LOCATION_LINGO_iPodAck, (U8)IPOD_LINGO_LOCATION);
            if(marker == 0)
            {
                rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
            }
            else
            {
                rc = iPodSendLongTelegramNoWaitForACK(iPodHndl, msg);
            }
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_LOCATION_LOG(DLT_LOG_ERROR, "msg is NULL");
        }

        if(msg != NULL)
        {
            free(msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p locData = %p",iPodHndl,locData);
    }

    return rc;
}

/*!
 * \fn iPodRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - The location control type.
 * U8 dataType - Location data type
 * U32 locSize - Size of location data.
 * \par INOUT PARAMETERS
 * U8 locData* - Data of requested locType and dataType.
 * \par REPLAY PARAMETERS
 * S32 rc - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is used in response to GetDevData command reveived from the iPod.<br>
 * This function can sent in the following data<br>
 * \li \c Satellite ephemeris data maximum requeired refresh interval.
 * \li \c Satellite ephemeris data recommended refresh interval.
 */
S32 iPodRetDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize)
{
    S32 rc = IPOD_OK;
    U16 sectMax = 0;
    U16 i = 0;
    U16 length = IPOD_LOCATION_MAX_PAYLOAD - IPOD_RET_DEV_DATA_LEN;
    U16 pos = 0;

    /* Parameter Check */
    if((locData != NULL) && (locSize > 0))
    {
    
        /* Payload Length is over 500 bytes */
        if(locSize > length)
        {
            /* Separate Section number */
            sectMax = (U16)(((U16)locSize - length)/ (length + IPOD_TOTAL_SIZE_LEN));
        }

        for(i = 0; (i <= sectMax) && (rc == IPOD_OK); i++)
        {
            if(locSize < length)
            {
                /* Only one or Last section */
                length = (U16)locSize;
            }

            rc = iPodMultiRetDevData(iPodID, locType, dataType, &locData[pos], locSize, i, sectMax, length);
            IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "iPodMultiRetDevData() returns : rc = %d",rc);
            
            pos += length;
            locSize -= (U32)length;
            if(i == 0)
            {
                length += IPOD_TOTAL_SIZE_LEN;
            }
            i++;
        }
    }
    IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "rc = %d",rc);

    return rc;

}

/*!
 * \fn iPodMultiAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - The location control type.<br>
 * U8 dataType - Location data type<br>
 * U32 locSize - Size of location data.<br>
 * U16 sectCur - Current payload section index<br>
 * U16 sectMax - Maximum payload section index<br>
 * U16 length - Total size of locData. If multiple packets, only in first packet included<br>
 * \par INOUT PARAMETERS
 * U8 locData* - Data of requested locType and dataType.<br>
 * \par REPLAY PARAMETERS
 * S32 rc -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * Helper function of #iPodAsyncDevData for Multisection Data Transfer.<br>.
 */
LOCAL S32 iPodMultiAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U8 *locData, U32 locSize, U16 sectCur, U16 sectMax, U16 length)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 marker = 0;
    U16 baseLen = IPOD_ASYNC_DEV_DATA;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (locData != NULL))
    {
        if(sectMax != 0)
        {
            /* Data is separate */
            baseLen -= IPOD_TOTAL_SIZE_LEN;
        }

        if(length >= IPOD_SHORT_WRITE_SIZE)
        {
            /* Data is Long telegram */
            marker += IPOD_SEND_LONG_MARKER;
        }
        
        msg = (U8 *)calloc((baseLen + length + marker), sizeof(U8));
        if(msg != NULL)
        {
            IAP1_LOCATION_LOG(DLT_LOG_VERBOSE, "location type = 0x%02x dataType = 0x%02x sectCur = 0x%04x sectMax = 0x%04x",locType,dataType,sectCur,sectMax);
            msg[IPOD_POS0] = IPOD_START_PACKET;
            if(marker == 0)
            {
                /* Short telegram */
                msg[IPOD_POS1] = (U8)((baseLen - IPOD_TRANSID_LENGTH) + length);
            }
            else
            {
                /* Long telegram */
                msg[IPOD_POS1] = 0;
                iPod_convert_to_big16(&msg[IPOD_POS2], (((baseLen -IPOD_TRANSID_LENGTH) + IPOD_SEND_LONG_MARKER) + length));
            }

            msg[IPOD_POS2 + marker] = (U8)IPOD_LINGO_LOCATION;
            msg[IPOD_POS3 + marker] = (U8)IPOD_LOCATION_LINGO_AsyncDevData;
            msg[IPOD_POS4 + marker] = (U8)locType;
            msg[IPOD_POS5 + marker] = dataType;
            iPod_convert_to_big16(&msg[IPOD_POS6 + marker], sectCur);
            iPod_convert_to_big16(&msg[IPOD_POS8 + marker], sectMax);
            if(sectCur == 0)
            {
                /* First msg */
                iPod_convert_to_big32(&msg[IPOD_POS10 + marker], locSize);
                memcpy(&msg[IPOD_POS14 + marker], locData, length);
            }
            else
            {
                memcpy(&msg[IPOD_POS10 + marker], locData, length);
            }

            iPodSetExpectedCmdId(iPodHndl, (U8)IPOD_LOCATION_LINGO_iPodAck, (U8)IPOD_LINGO_LOCATION);
            if(marker == 0)
            {
                rc = iPodSendCommand(iPodHndl, msg);
            }
            else
            {
                rc = iPodSendLongTelegram(iPodHndl, msg);
            }

            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_LOCATION_LOG(DLT_LOG_ERROR, "msg is NULL");
        }

        if(msg != NULL)
        {
            free(msg);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p locData = %p",iPodHndl,locData);
    }

    return rc;
}

/*!
 * \fn iPodAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U32 totalSize, U8 *locData)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - The location control type.<br>
 * U8 dataType - Location data type<br>
 * U32 locSize - Size of location data.
 * \par INOUT PARAMETERS
 * U8 locData* - Data of requested locType and dataType.
 * \par REPLAY PARAMETERS
 * S32 rc - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is used for notify the iPod that location data is available.<br>
 * It is valid only if the accessory supports asynchronous notification and enable the notification category.
 */
S32 iPodAsyncDevData(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 dataType, U32 totalSize, U8 *locData)
{
    S32 rc = IPOD_OK;
    U16 length = IPOD_LOCATION_MAX_PAYLOAD - IPOD_ASYNC_DEV_DATA;
    U16 sectMax = 0;
    U16 i = 0;
    U16 pos = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (locData != NULL) && (totalSize > 0))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            /* Check totalSize is over 500 bytes */
            if(totalSize > length)
            {
                sectMax = (U16)(1 + ((U16)totalSize - length)/ (length + IPOD_TOTAL_SIZE_LEN));
            }

            for(i = 0; (i <= sectMax) && (rc == IPOD_OK); i++)
            {
                /* Check no section */
                if(totalSize < length)
                {
                    length = (U16)totalSize;
                }

                rc = iPodMultiAsyncDevData(iPodID, locType, dataType, &locData[pos], totalSize, i, sectMax, length);
                IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "iiPodMultiAsyncDevData() returns : rc = %d",rc);
                pos += length;
                totalSize -= length;
                if(i == 0)
                {
                    length += IPOD_TOTAL_SIZE_LEN;
                }
                i++;
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_LOCATION_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "Error Parameter - iPodHndl = %p locData = %p totalSize = %d",iPodHndl,locData,totalSize);
    }
    IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodRetDevCaps(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 *capsData, U8 size)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_LOCATION_TYPE locType - the capability type<br>
 * U8 *capsData - capabilities data. the contents and length depends on locType<br>
 * U8 size - size of capabilities data<br>
 * \par REPLAY PARAMETERS
 * S32 rc -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is used in response to a GetAccessoryCaps command from Apple device.<br>
 */
S32 iPodRetDevCaps(U32 iPodID, IPOD_LOCATION_TYPE locType, U8 *capsData, U8 size)
{
    S32 rc = 0;
    U8 *msg = NULL;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (capsData != NULL) && (size != 0) && (locType < LOCATION_TYPE_MAX))
    {
        msg = (U8 *)calloc((IPOD_RET_DEV_CAPS_LEN + IPOD_START_LENGTH + size), sizeof(U8));
        if(msg != NULL)
        {
            IAP1_LOCATION_LOG(DLT_LOG_VERBOSE, "location type = 0x%02x caps data = %s",locType,capsData);
            msg[IPOD_POS0] = IPOD_START_PACKET;
            msg[IPOD_POS1] = IPOD_RET_DEV_CAPS_LEN + size;
            msg[IPOD_POS2] = (U8)IPOD_LINGO_LOCATION;
            msg[IPOD_POS3] = IPOD_LOCATION_LINGO_RetDevCaps;
            msg[IPOD_POS4] = (U8)locType;
            memcpy(&msg[IPOD_POS5], capsData, size);
            rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_LOCATION_LOG(DLT_LOG_ERROR, "No Memory- msg is NULL");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p capsData = %p size = %d locType = %d",
                                         iPodHndl,capsData,size,locType);
    }

    if(msg != NULL)
    {
        free(msg);
    }
    IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodDevAck(U32 iPodID, S32 status, U8 cmdIDOrig, const U16 sectCur, U8 multiFlg)
 * \par LOCATION LINGO PROTOCOL VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * S32 status - command status<br>
 * U8 cmdIDOrig - original command ID for which this response is being sent<br>
 * const U16 sectCur - section index for which this response is being sent<br>
 * U8 multiFlg - indicates multisection accessory Ack packet<br>
 * \par REPLAY PARAMETERS
 * S32 rc -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is used in response to a command sent from Apple device.<br>
 */
S32 iPodDevAck(U32 iPodID, S32 status, U8 cmdIDOrig, const U16 sectCur, U8 multiFlg)
{
    S32 rc = 0;
    U8 *msg = NULL;
    U8 msg1[] = {IPOD_DEV_ACK_NO_SEC_CMD};
    U8 msg2[] = {IPOD_DEV_ACK_SEC_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        if(multiFlg != FALSE)
        {
            msg = msg2;
            iPod_convert_to_big16(&msg[IPOD_POS6], sectCur);
        }
        else
        {
            msg = msg1;
        }

        IAP1_LOCATION_LOG(DLT_LOG_VERBOSE, "status = 0x%02x cmdIDOrig = 0x%02x",status,cmdIDOrig);
        msg[IPOD_POS4] = (U8)status;
        msg[IPOD_POS5] = cmdIDOrig;

        rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_LOCATION_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_LOCATION_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}
/*\}*/
