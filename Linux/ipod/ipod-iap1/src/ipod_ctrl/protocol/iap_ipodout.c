
/**
 * \file: iap_ipodout.c
 *
 */

#include <adit_typedef.h>
#include <string.h>
#include <stdlib.h>
#include "iap_util_func.h"
#include "iap_types.h"
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_general.h"
#include "iap_init.h"
#include "iap1_dlt_log.h"
/**
 * \addtogroup iPodOut_commands
 */
/*\{*/


/*!
 * \fn iPodGetiPodOutOptions(U32 iPodID, U8 options, U32 *optionsBits)
 * \par IPOD OUT
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 options - 0x00 reports all possible and 0x01 reports currently set iPod Out options<br>
 * \par INOUT PARAMETERS
 * U32 *optionsBits  - possible iPod Out options of Apple device<br>
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
 * This function is used to get the capabilities of the Apple device while iPod Out mode.<br>
 */
S32 iPodGetiPodOutOptions(U32 iPodID, U8 options, U32 *optionsBits)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_OUT_OPTIONS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if((iPodHndl != NULL) && (optionsBits != NULL))
    {
        msg[IPOD_POS4] = options;
        IAP1_IPODOUT_LOG(DLT_LOG_VERBOSE, "Enabled options = 0x%02x",options);
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_IPODOUT_LINGO_RetiPodOutOptions, (U8)IPOD_LINGO_OUT);
        rc = iPodSendCommand(iPodHndl, msg);
        if(rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
        
        if(rc == IPOD_OK)
        {
            *optionsBits = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS1]);
            IAP1_IPODOUT_LOG(DLT_LOG_VERBOSE, "Enabled options = 0x%02x optionsBits = 0x%08x",iPodHndl->iAP1Buf[IPOD_POS0],*optionsBits );
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_IPODOUT_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p,optionsBits = %p",iPodHndl,optionsBits);
    }
    IAP1_IPODOUT_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodSetiPodOutOptions(U32 iPodID, U32 optionsBits)
 * \par IPOD OUT
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 optionsBits  - bitmask to enable iPod Out output options of Apple device<br>
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
 * This function is used to set the capabilities of the Apple device while iPod Out mode.<br>
 */
S32 iPodSetiPodOutOptions(U32 iPodID, U32 optionsBits)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_IPOD_OUT_OPTIONS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        IAP1_IPODOUT_LOG(DLT_LOG_DEBUG, "Options bits = 0x%08x",optionsBits );
        iPod_convert_to_big32(&msg[IPOD_POS4], optionsBits);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_IPODOUT_LINGO_iPodAck, (U8)IPOD_LINGO_OUT);
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
        IAP1_IPODOUT_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_IPODOUT_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodDevStateChangeEvent(U32 iPodID, U8 status)
 * \par IPOD OUT
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 status - accessory state transition<br>
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
 * This function is used to tell the Apple device that the state of the Accessory is changing.<br>
 */
S32 iPodDevStateChangeEvent(U32 iPodID, U8 status)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_DEV_STATE_CHANGE_EVENT_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        IAP1_IPODOUT_LOG(DLT_LOG_VERBOSE, "New status = 0x%02x",status );
        msg[IPOD_POS4] = status;

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_IPODOUT_LINGO_iPodAck, (U8)IPOD_LINGO_OUT);
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
        IAP1_IPODOUT_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_IPODOUT_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodDevVideoScreenInfo(U32 iPodID, IPOD_VIDEO_SCREEN_INFO info)
 * \par IPOD OUT
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_VIDEO_SCREEN_INFO info - screen information<br>
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
 * This function is used to tell the Apple device the video screen information of the Accessory.<br>
 */
S32 iPodDevVideoScreenInfo(U32 iPodID, IPOD_VIDEO_SCREEN_INFO info)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_DEV_VIDEO_SCREEN_INFO_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    
    if(iPodHndl != NULL)
    {
        iPod_convert_to_big16(&msg[IPOD_POS4], info.totalWidthInches);
        iPod_convert_to_big16(&msg[IPOD_POS6], info.totalHeightInches);
        iPod_convert_to_big16(&msg[IPOD_POS8], info.totalWidthPixels);
        iPod_convert_to_big16(&msg[IPOD_POS10], info.totalHeightPixels);
        iPod_convert_to_big16(&msg[IPOD_POS12], info.widthPixels);
        iPod_convert_to_big16(&msg[IPOD_POS14], info.heightPixels);
        msg[IPOD_POS16] = info.featuresMask;
        msg[IPOD_POS17] = info.gammaValue;
        IAP1_IPODOUT_LOG(DLT_LOG_VERBOSE, "Total Screen Width(inch) = %u Total Screen Height(inch) = %u Total Screen Width(pixels) = %u Total Screen Height(pixels) = %u",
                                               info.totalWidthInches,info.totalHeightInches,info.totalWidthPixels,info.totalHeightPixels);
        IAP1_IPODOUT_LOG(DLT_LOG_VERBOSE, "iPodOut Screen Width(pixels) = %u iPodOut Screen Height(pixels) = %u feature mask = %u gamma value = %u",
                                            info.widthPixels,info.heightPixels,info.featuresMask,info.gammaValue);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_IPODOUT_LINGO_iPodAck, (U8)IPOD_LINGO_OUT);
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
        IAP1_IPODOUT_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    
    return rc;
}
/*\}*/
