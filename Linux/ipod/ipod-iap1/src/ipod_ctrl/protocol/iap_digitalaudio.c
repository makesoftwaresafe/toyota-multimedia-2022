/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_digitalaudio.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include "iap_digitalaudio.h"
#include "iap_types.h"
#include "iap_general.h"
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_util_func.h"
#include "iap_init.h"
#include "iap1_dlt_log.h"
#include <string.h>

/**
 * \addtogroup DigitalAudioLingoCommands
 */
/*\{*/

/*!
 * \fn iPodAccAck(U32 iPodID, IPOD_ACC_ACK_STATUS status)
 * \par DIGITAL_AUDIO_PROTOCOL_VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_ACC_ACK_STATUS status - the status id
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function sends the AccAck command to acknowledge the receipt of a command
 * from the iPod. An AccAck command should be sent only for commands whose
 * documentation specifies that an AccAck command is needed.
 * \note In the case where the application does the audio streaming,
 * the application must call this API after switching the sample rate in case the iPodNewTrackInfo command required a sample rate change.
 */
void iPodAccAck(U32 iPodID, IPOD_ACC_ACK_STATUS status)
{
    U8 msg[] = {IPOD_SEND_ACC_ACK_CMD};

    msg[IPOD_POS4]  =   (U8)status;
    IAP1_DIGITAL_LOG(DLT_LOG_VERBOSE, "Accessory acknowledgment status = 0x%02x",status);
    iPodSendCommandNoWaitForACK(iPodGetHandle(iPodID), msg);
}

/*!
 * \fn iPodAccAckDevice(IPOD_ACC_ACK_STATUS status, U32 iPodID)
 * \par DIGITAL_AUDIO_PROTOCOL_VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * #IPOD_ACC_ACK_STATUS status - the status id
 * U32 iPodID - ID of the iPod to send the ACK to
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function sends the AccAck command to acknowledge the receipt of a command
 * from the iPod. An AccAck command should be sent only for commands whose
 * documentation specifies that an AccAck command is needed.
 * \note In the case where the application does the audio streaming,
 * the application must call this API after switching the sample rate in case the iPodNewTrackInfo command required a sample rate change.
 */
void iPodAccAckDevice(IPOD_ACC_ACK_STATUS status, U32 iPodID)
{
    U8 msg[] = {IPOD_SEND_ACC_ACK_CMD};
    IPOD_INSTANCE* iPodInstance = iPodGetHandle(iPodID);

    if(iPodInstance != NULL)
    {
        msg[IPOD_POS4]  =   (U8)status;
        IAP1_DIGITAL_LOG(DLT_LOG_VERBOSE, "Accessory acknowledgment status = 0x%02x",status);
        iPodSendCommandNoWaitForACK(iPodInstance, msg);
    }
}

/*!
 * \fn iPodUSBRetAccSampleCaps(U32 iPodID)
 * \par DIGITAL_AUDIO_PROTOCOL_VERSION
 * 1.00
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function sends the list of sample rate it supports.
 * \note This command is used by iPod Control internally. Under normal circumstances there is no reason
 * for the application to call this API. There is also no need to add any further sample rates
 * to the list of supported sample rates, as the iPod will internally convert any other sample rates to match one
 * of the supported (32kHz, 44.1kHz or 48 kHz). It is not allowed to remove any of those three supported sample rates
 * as the iPod specification clearly states that these sample rates are mandatory.
 */
void iPodUSBRetAccSampleCaps(U32 iPodID)
{
    U8 sampleRate32K[IPOD_POS4] = IPOD_USB_SAMPLERATE_32K;
    U8 sampleRate44K[IPOD_POS4] = IPOD_USB_SAMPLERATE_44K;
    U8 sampleRate48K[IPOD_POS4] = IPOD_USB_SAMPLERATE_48K;
    U8 Send_Buffer[IPOD_USB_REPORT_LEN] = IPOD_USB_SEND_BUFFER;

    Send_Buffer[IPOD_POS0] = IPOD_START_OF_PACKET;
    Send_Buffer[IPOD_POS1] = IPOD_AUDIO_ACC_SAMPLE_RATE_LEN;
    Send_Buffer[IPOD_POS2] = IPOD_AUDIO_LINGO;
    Send_Buffer[IPOD_POS3] = IPOD_AUDIO_ACC_SAMPLE_RATE_CMD;

    memcpy(&Send_Buffer[IPOD_POS4],
            sampleRate32K,
            IPOD_POS4*sizeof(U8));

    memcpy(&Send_Buffer[IPOD_POS8],
           sampleRate44K,
           IPOD_POS4*sizeof(U8));

    memcpy(&Send_Buffer[IPOD_POS12],
           sampleRate48K,
           IPOD_POS4*sizeof(U8));

    iPodSendCommandNoWaitForACK(iPodGetHandle(iPodID), Send_Buffer);
}

/*!
 * \fn iPodSetVideoDelay(U32 iPodID, U32 delay)
 * \par DIGITAL_AUDIO_PROTOCOL_VERSION
 * 1.03
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 delay - delay time in milliseconds
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Compleated successfully
 * \li \c \b #IPOD_COMMAND_FAILED command failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function is occurred Video delay for synchronize with audio
 * \note This API is only available beginning with Audio lingo version 1.03. As a result you should
 * make sure to check the return code, which will tell you whether the iPod is able to process this
 * command.
 */
S32 iPodSetVideoDelay(U32 iPodID, U32 delay)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_SET_VIDEO_DELAY_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl->isAPIReady != FALSE)
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_IPOD_ACK, (U8)IPOD_LINGO_DIGITAL_AUDIO);
        IAP1_DIGITAL_LOG(DLT_LOG_VERBOSE, "Delay in ms = %u",delay);
        iPod_convert_to_big32(&msg[IPOD_POS4], delay);
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
        IAP1_DIGITAL_LOG(DLT_LOG_ERROR, "IPOD NOT CONNECTED");
    }
    

    return rc;

}
/*\}*/
