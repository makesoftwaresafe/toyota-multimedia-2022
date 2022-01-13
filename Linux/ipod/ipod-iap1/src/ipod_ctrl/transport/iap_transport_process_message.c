#include <adit_typedef.h>
#include <stdio.h>

#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */

#include <string.h>

#include "iap_init.h"
#include "iap_callback.h"
#include "iap_transport_authentication.h"
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_devconf.h"
#include "iap_util_func.h"
#include "iap_transport_message.h"
#include "iap_transport_process_message.h"
#include "iap_transport_configuration.h"
#include "iap_general.h"
#include "iap_storage.h"
#include "iap_location.h"
#include "iap1_dlt_log.h"

#include "iap_digitalaudio.h"

#ifndef IPOD_FAKE_AUTHENTICATE
IMPORT U16 g_iPodCertTransID;
IMPORT U8* g_iPodAuthenticationBuffer;
IMPORT U16 g_iPodAuthenticationBufferLength;
IMPORT IPOD_SEM_ID g_semWorkerId;
#endif  /* IPOD_FAKE_AUTHENTICATE */


void iPodProcessGeneralLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    S32 ercd = 0;

    if (MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_GetDevAuthenticationInfo)
    {
        iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_CERTIFICATE);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetDevAuthenticationInfo");
    }
    else if (MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_RequestIdentify)
    {
        iPodReidentify(iPodHndl);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :RequestIdentify");
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_AckDevAuthenticationStatus) && 
             (MessageHeaderInfo.telegramErrorCode != IPOD_OK))
    {
        iPodReidentify(iPodHndl);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :AckDevAuthenticationStatus");
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_AckDevAuthenticationStatus) &&
             (MessageHeaderInfo.telegramErrorCode == IPOD_OK))
    {
        iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_PREFERENCE);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :AckDevAuthenticationStatus");
    }
#ifndef IPOD_FAKE_AUTHENTICATE
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_GetDevAuthenticationSignature) &&
             (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetDevAuthenticationSignature");
        ercd = (S32)iPodOSWaitSemaphore(g_semWorkerId, IPOD_TIME_OUT);
        if (IPOD_OK == ercd)
        {
            g_iPodAuthenticationBuffer = (U8*) calloc(MessageHeaderInfo.telegramLen, sizeof(U8));
        
            if (g_iPodAuthenticationBuffer != NULL)
            {
                memcpy(g_iPodAuthenticationBuffer,
                       MessageHeaderInfo.iPodResponseBuffer,
                       MessageHeaderInfo.telegramLen);
                g_iPodAuthenticationBufferLength = MessageHeaderInfo.telegramLen;
                g_iPodCertTransID = MessageHeaderInfo.transID;
                iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_SIGNATURE);
            }
            else
            {
                iPodWorkerExeCB(iPodHndl, IPOD_ERR_NOMEM);
            }
        }
    }
#else /* IPOD_FAKE_AUTHENTICATE */
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_AckDevAuthenticationInfo)
             && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :AckDevAuthenticationInfo");
        if(MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0] == 0x00)
        {
            iPodHndl->isAPIReady = TRUE;
            iPodWorkerExeCB(iPodHndl, IPOD_OK);
        }
    }
#endif /* #ifdef IPOD_FAKE_AUTHENTICATE */
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_NotifyiPodStateChange)
             && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :AckDevAuthenticationInfo");
        IPOD_STATE_CHANGE newState = (IPOD_STATE_CHANGE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0];
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "NewState = 0x%02x",newState);
        iPodExecuteCBNotifyStateChange(iPodHndl, newState);
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_GetAccessoryInfo)
             && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetAccessoryInfo");

        /* ipod is hung up when ipod_ctrl reply during mode changing of ipod */
        /* JIRA[SWGII-4800]*/
        /* if now cannot transmit a message, it must reply */
        ercd = iPodWaitForModeSwitch(iPodHndl);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPodWaitForModeSwitch() returns : ercd = %d",ercd);
        if ( ercd == 0 )
        {
            /* iPod control must answer with a general lingo 0x28 command (RetAccessoryInfo)*/
            iPodRetAccessoryInfo(iPodHndl, MessageHeaderInfo.iPodResponseBuffer);
            ercd = iPodSigModeSwitchSemaphore(iPodHndl);
        }
        else
        {
            /* if iPod is changing the mode, this command send after finish the mode change */
            iPodSetAccInfo(&MessageHeaderInfo, iPodHndl);
        }
    }
    else if (MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_RetiPodAuthenticationInfo)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :RetiPodAuthenticationInfo");
        static U8  iPodCertData[IPOD_AUTH_CP_MAX_CERTLENGTH] = {0};
        static U16 iPodCertLength = 0;
        U8 length    = (U8)(MessageHeaderInfo.telegramLen - IPOD_GENERAL_CERT_INDEX_LEN);
        U8  maxIndex = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS3];
        U8  curIndex = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];

        if((IPOD_AUTH_CP_MAX_CERTLENGTH - iPodCertLength) >= length)
        {
            memcpy(&iPodCertData[iPodCertLength], &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS4], length);
            iPodCertLength += length;
        }

        if(curIndex == maxIndex)
        {
            CertificateInfoType *certData;
            U16 transID = iPodHndl->rcvMsgInfo.accTransID;
            iPodHndl->rcvMsgInfo.accTransID = transID - 1;
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_RetiPodAuthenticationInfo, (U8)IPOD_LINGO_GENERAL);
            iPodHndl->rcvMsgInfo.accTransID = transID;
            MessageHeaderInfo.telegramLen = sizeof(CertificateInfoType);

            /* set certificate data information to iPodResponseBuffer */
            certData = (CertificateInfoType *)((void *)MessageHeaderInfo.iPodResponseBuffer);  /* fixed lint findings */
            certData->cert_len =  iPodCertLength;
            certData->cert_data =  iPodCertData;
            iPodCertLength = 0;
        }
        else if(curIndex > maxIndex)
        {
            MessageHeaderInfo.telegramLen = 0;
        }
        else
        {
            /* For QAC warnning*/
        }
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_iPod_Notification)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :iPod_Notification");
        IPOD_RCVMSGINFO    *rcvMsgInfo = &iPodHndl->rcvMsgInfo;
        IPOD_NOTIFY_TYPE   type = (IPOD_NOTIFY_TYPE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0];
        IPOD_NOTIFY_STATUS status = {0};
        U16                commandID = 0;
        U64                BTProfileSupport = 0;
        U16                transactionID = 0;
        
        switch(type)
        {
        case IPOD_EVT_FLOW_CTRL:
            status.waitMs = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
            transactionID = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS5]);
            /* block sending of messages for requested time */
            rcvMsgInfo->flowControlWait = status.waitMs;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "FLow control Changed waitMs = %d transactionID = %d",status.waitMs,transactionID);
            /* if command which caused overflow is waiting for reply, release waiting command with error */
            if ((rcvMsgInfo->waitingCmdCnt != 0) && (rcvMsgInfo->expectedTransID == transactionID))
            {
                /* Signal that message was received with an error */
                MessageHeaderInfo.telegramLingo = rcvMsgInfo->expectedLingo;
                MessageHeaderInfo.telegramCmdId = rcvMsgInfo->expectedCmdId;
                MessageHeaderInfo.ackCmdId = rcvMsgInfo->sendCmdId;
                MessageHeaderInfo.transID = rcvMsgInfo->expectedTransID;
                MessageHeaderInfo.telegramErrorCode = IPOD_ACKERR_CMD_FAILED;
            }
            break;
        case IPOD_EVT_RADIO_TAG_STATUS:
        case IPOD_EVT_CAMERA_STATUS:
            status.notifyStatus = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Camera Status Changed to %d",status.notifyStatus);
            break;
        case IPOD_EVT_CHARGING:
            if(IPOD_OK == MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1])
            {
                status.availableCurrent = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Apple Device current sink is limit to %d",status.availableCurrent);

            }
            break;
        case IPOD_EVT_DB_CHANGED:
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Database content on Apple device is changed");
            break;
        case IPOD_EVT_NOW_FOCUS_APP:
            status.focusApp = (U8 *)calloc(strlen((VP)&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]) + 1, sizeof(U8));
            if(status.focusApp != NULL)
            {
                memcpy(status.focusApp, &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1], strlen((VP)&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]) + 1);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Current focus app is changed to %s",status.focusApp);
            }
            break;
        case IPOD_EVT_SESSION:
            status.sessionId = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Current session Id is changed to %d",status.sessionId);
            break;
        case IPOD_EVT_CMD_COMPLETE:
            status.commandComplete[0] = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
            commandID = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2]);
            memcpy(&status.commandComplete[1], (U8*)&commandID, sizeof(commandID));
            status.commandComplete[3] = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS4];
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Command is complete : Lingo Id = 0x%02x",status.commandComplete[0]);
            break;
        case IPOD_EVT_IPOD_OUT:
            status.notifyStatus = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Change in iPod Out mode to = %d",status.notifyStatus);
            break;
        case IPOD_EVT_BT_STATUS:
            memcpy(&status.BTStatus[0], &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1], IPOD_BT_MAC_ADDRESS_LEN);
            BTProfileSupport = iPod_convert_to_little64(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS7]);
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "BTProfileSupport = %llu",BTProfileSupport);
            memcpy(&status.BTStatus[6], (U8*)&BTProfileSupport, sizeof(BTProfileSupport));
            break;
        case IPOD_EVT_APP_DISPLAY_NAME:
            status.focusApp = (U8 *)calloc(strlen((VP)&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]) + 1, sizeof(U8));
            if(status.focusApp != NULL)
            {
                memcpy(status.focusApp, &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1], strlen((VP)&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]) + 1);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Display Name of currently active app = %s",status.focusApp);
            }
            break;
        case IPOD_EVT_ASSIST_TOUCH:
            status.notifyStatus = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Status of assistive touch is = %d",status.notifyStatus);
            break;
        default:
            break;
        }
            
        iPodExecuteCBNotification(iPodHndl, type, status);
        
        if(((type == IPOD_EVT_NOW_FOCUS_APP) || (type == IPOD_EVT_APP_DISPLAY_NAME))
            && (status.focusApp != NULL))
        {
            free(status.focusApp);
        }
    
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_OpenDataSession)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :OpenDataSession");
        U8 protocolIndex = (U8)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];
        U16 sessionId = 0;
        sessionId = iPod_convert_to_little16(MessageHeaderInfo.iPodResponseBuffer);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Session Id = %d",sessionId);
        iPodExecuteCBOpenDataSession(iPodHndl, protocolIndex, sessionId);
        iPodSessionDevAck(iPodHndl, IPOD_GENERAL_LINGO_OpenDataSession, IPOD_OK);
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_CloseDataSession)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :CloseDataSession");
        U16 sessionId = iPod_convert_to_little16(MessageHeaderInfo.iPodResponseBuffer);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Session Id = %d",sessionId);
        iPodExecuteCBCloseDataSession(iPodHndl, sessionId);
        iPodSessionDevAck(iPodHndl, IPOD_GENERAL_LINGO_CloseDataSession, IPOD_OK);
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_iPodDataTransfer)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :iPodDataTransfer");
        U16 length = 0;
        U8 *data = NULL;
        U16 sessionId = 0;
        
        length = MessageHeaderInfo.telegramLen - 2;
        data = (U8 *)&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];
        sessionId = iPod_convert_to_little16(MessageHeaderInfo.iPodResponseBuffer);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Session Id = %d",sessionId);
        ercd = iPodExecuteCBiPodDataTransfer(iPodHndl, sessionId, data, length);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodExecuteCBiPodDataTransfer() returns : ercd = %d",ercd);
        if(ercd == IPOD_OK)
        {
            iPodSessionDevAck(iPodHndl, IPOD_GENERAL_LINGO_iPodDataTransfer, IPOD_OK);
        }
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_GENERAL_LINGO_iPodSetAccStatusNotification)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :iPodSetAccStatusNotification");
        U32 statusMask = 0;
        statusMask = iPod_convert_to_little32(MessageHeaderInfo.iPodResponseBuffer);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPodSetAccStatusNotification = 0x%08x",statusMask);
        ercd = iPodExecuteCBSetAccStatusNotification(iPodHndl, statusMask);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodExecuteCBSetAccStatusNotification() returns : ercd = %d",ercd);
    }
    else
    {
        /* For QAC warnnning */
    }
}

void iPodProcessDisplayRemoteLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    if ((MessageHeaderInfo.telegramCmdId == IPOD_DISPLAY_LINGO_REMOTE_EVENT_NOTIFICATION) &&
        (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :REMOTE_EVENT_NOTIFICATION");
        IPOD_STATE_INFO_TYPE   eventNum = (IPOD_STATE_INFO_TYPE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0];
        IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData;
        memset(&eventData, 0, sizeof(eventData));

        switch(eventNum)
        {
            case IPOD_STATE_INFO_TRACK_POSITION_MS:
            {
                eventData.trackPosMS = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Position of track is changed to(in ms) = %u",eventData.trackPosMS);
                break;
            }
            case IPOD_STATE_INFO_TRACK_INDEX:
            {
                eventData.trackIndex = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Index of Playing track = %u",eventData.trackIndex);
                break;
            }
            case IPOD_STATE_INFO_CHAPTER_INFO:
            {
                eventData.chapterInfo.trackIndex = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                eventData.chapterInfo.chapterCount = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS5]);
                eventData.chapterInfo.chapterIndex = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS7]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Current Chapter Info : trackindex = %u chapter count = %u chapter index = %u",eventData.chapterInfo.trackIndex,
                                 eventData.chapterInfo.chapterCount,eventData.chapterInfo.chapterIndex);
                break;
            }
            case IPOD_STATE_INFO_PLAY_STATUS:
            {
                eventData.playStatus = (IPOD_PLAY_STATUS_VALUES)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Play status of apple device = 0x%02x",eventData.playStatus);
                break;
            }
            case IPOD_STATE_INFO_MUTE_VOLUME:
            {
                eventData.muteUiVol.muteState = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                eventData.muteUiVol.uiVol = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "mute state = %d volume level = %d",eventData.muteUiVol.muteState,eventData.muteUiVol.uiVol);
                break;
            }
            case IPOD_STATE_INFO_POWER_BATTERY:
                eventData.powerBattery.powerState = (IPOD_POWER_BATTERY_STATE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                eventData.powerBattery.batteryLevel = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Power state = 0x%02x volume battery level = %d",eventData.powerBattery.powerState,eventData.powerBattery.batteryLevel);
                break;
            case IPOD_STATE_INFO_EQUALIZER_STATE:
            {
                eventData.eqState = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Equalizer state is changed to = %u",eventData.eqState);
                break;
            }
            case IPOD_STATE_INFO_SHUFFLE:
            {
                eventData.shuffle = (IPOD_SHUFFLE_MODE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Shuffle state is changed to = 0x%02x",eventData.shuffle);
                break;
            }
            case IPOD_STATE_INFO_REPEAT:
            {
                eventData.repeat = (IPOD_REPEAT_MODE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Repeat state is changed to = 0x%02x",eventData.repeat);
                break;
            }
            case IPOD_STATE_INFO_DATE_TIME:
            {
                eventData.currDateTime.year = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                eventData.currDateTime.month = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS3];
                eventData.currDateTime.day = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS4];
                eventData.currDateTime.hour = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS5];
                eventData.currDateTime.min = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS6];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Setting of date and time year = %d month = %d day = %d hour = %d min = %d",
                         eventData.currDateTime.year,eventData.currDateTime.month,eventData.currDateTime.day,eventData.currDateTime.hour,eventData.currDateTime.min);
                break;
            }
            case IPOD_STATE_INFO_BACKLIGHT:
            {
                eventData.backlight = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Level of backlight setting is %d",eventData.backlight);
                break;
            }
            case IPOD_STATE_INFO_HOLD_SWITCH:
            {
                eventData.holdSwitch = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Status of hold switch is %d",eventData.holdSwitch);
                break;
            }
            case IPOD_STATE_INFO_SOUND_CHECK:
            {
                eventData.soundCheck = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Status of sound check is %d",eventData.holdSwitch);
                break;
            }
            case IPOD_STATE_INFO_AUDIOBOOK_SPEED:
            {
                eventData.audiobook = (IPOD_AUDIOBOOK_SPEED)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Audiobook playback speed is 0x%02x",eventData.audiobook);
                break;
            }
            case IPOD_STATE_INFO_TRACK_POSITION_S:
            {
                eventData.trackPosSec =  iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Position of track (in s) %d",eventData.trackPosSec);
                break;
            }
            case IPOD_STATE_INFO_MUTE_EXTENDED_VOLUME:
            {
                eventData.muteUiAbsoluteVol.muteState = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                eventData.muteUiAbsoluteVol.uiVol = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2];
                eventData.muteUiAbsoluteVol.absVol = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS3];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "mute state = %d volume = %d absolute volume = %d",eventData.muteUiAbsoluteVol.muteState,
                                                   eventData.muteUiAbsoluteVol.uiVol,eventData.muteUiAbsoluteVol.absVol);
                break;
            }
            case IPOD_STATE_INFO_TRACK_CAPABILITIES:
            {
                U32 *tmpData = (U32*)&eventData.trackCaps;
                *tmpData = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Track capabilities is set to %d",*tmpData);
                break;
            }
            case IPOD_STATE_INFO_NUM_OF_TRACKS_IN_PLAYLIST:
            {
                eventData.playEngineContent = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Number of tracks in new playlist is %u",eventData.playEngineContent);
                break;
            }
            default:
            {
                break;
            }
        }
        iPodExecuteCBRemoteEventNotify(iPodHndl, eventNum, eventData);
    }
    else
    {
        /* For QAC warnning */
    }
}

void iPodProcessExtendedLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    if ((MessageHeaderInfo.telegramCmdId == IPOD_EXTENDED_LINGO_PLAYSTATUSCHANGENOTIFICATION) &&
        (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
         IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :PLAYSTATUSCHANGENOTIFICATION");
        U64 position = 0;
        IPOD_CHANGED_PLAY_STATUS status;

        status      =   (IPOD_CHANGED_PLAY_STATUS)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0];
        switch(status)
        {
            case IPOD_STATUS_PLAYBACK_STOPPED:
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Stopped");
                break;
            case IPOD_STATUS_TRACK_CHANGED:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Trackindex changed to %llu",position);
                break;
            case IPOD_STATUS_FWD_SEEK_STOP:
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Forward seek stop");
                break;
            case IPOD_STATUS_BWD_SEEK_STOP:
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Backward seek stop");
                break;
            case IPOD_STATUS_TRACK_POSITION:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Track position is changed to %llu",position);
                break;
            case IPOD_STATUS_CHAPTER_CHANGED:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback Chapter Index is changed to %llu",position);
                break;
            case IPOD_STATUS_PLAYBACK_EXTENDED:
                position = (U64)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Playback status is %llu",position);
                break;
            case IPOD_STATUS_TRACK_TIME_OFFSET:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Track time offset is %llu",position);
                break;
            case IPOD_STATUS_CHAPTER_MS_OFFSET:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Chapter time offset(in ms) is %llu",position);
                break;
            case IPOD_STATUS_CHAPTER_SEC_OFFSET:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Chapter time offset(in s) is %llu",position);
                break;
            case IPOD_STATUS_TRACK_UID:
                position = (U64)iPod_convert_to_little64(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Track UID is %llu",position);
                break;
            case IPOD_STATUS_TRACK_PLAYBACK_MODE:
                position = (U64)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Track Playback Mode is %llu",position);
                break;
            case IPOD_STATUS_TRACK_LYRICS_READY:
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Status of lyrics for currently playing track are available for download");
                break;
            case IPOD_STATUS_TRACK_CAPABILITIES_CHANGED:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod track capabilities has been changed to %llu",position);
                break;
            case IPOD_STATUS_PLAYBACK_CONTENT_CHANGED:
                position = (U64)iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Number of tracks in new playlist has been changed to %llu",position);
                break;
            default :
                break;
        }

        iPodExecuteCBNotify(iPodHndl, status, (U64)position);
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_EXTENDED_LINGO_RET_TRACKARTWORKDATA)
             && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        iPodExecuteCBGetTrackArtworkData(iPodHndl, MessageHeaderInfo.iPodResponseBuffer, MessageHeaderInfo.telegramLen);
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_EXTENDED_LINGO_RET_TYPE_OF_TRACKARTWORKDATA)
             && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        iPodExecuteCBGetTrackArtworkDataEx(iPodHndl, MessageHeaderInfo.iPodResponseBuffer, MessageHeaderInfo.telegramLen);
    }
    else
    {
        /* For QAC warnning */
    }
}

void iPodProcessAudioLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    if (MessageHeaderInfo.telegramCmdId == IPOD_AUDIO_LINGO_GetAccSamplerateCaps)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetAccSamplerateCaps");
        iPodUSBRetAccSampleCaps(iPodHndl->id);
    }
    else if ((MessageHeaderInfo.telegramCmdId == IPOD_AUDIO_LINGO_NewiPodTrackInfo)
         && (MessageHeaderInfo.iPodResponseBuffer != NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :NewiPodTrackInfo");
        iPodExecuteCBNewTrackInfo(iPodHndl, MessageHeaderInfo.iPodResponseBuffer);
    }
    else
    {
        /* For QAC warnning */
    }
}

void iPodProcessStorageLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    if((MessageHeaderInfo.telegramCmdId >= IPOD_STORAGE_LINGO_MIN_SUPPORTED_CMDID) && 
      (MessageHeaderInfo.telegramCmdId <= IPOD_STORAGE_LINGO_MAX_SUPPORTED_CMDID))
    {
        (void)iPodDeviceACK(iPodHndl->id, (U8)MessageHeaderInfo.telegramCmdId, (U8)IPOD_ACKERR_BAD_PARAM);
    }
    else if(MessageHeaderInfo.telegramCmdId == IPOD_STORAGE_LINGO_GET_DEVICE_CAPS)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GET_DEVICE_CAPS");
        (void)iPodRetDeviceCaps(iPodHndl->id, IPOD_STORAGE_CURRENT_SUPPORT_MAJOR, IPOD_STORAGE_CURRENT_SUPPORT_MINOR);
    }
    else
    {
    }
}

void iPodProcessLocationLingoCommand(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType MessageHeaderInfo)
{
    S32 rc = IPOD_OK;
    IPOD_Cfg *dcInfo = NULL;
    IPOD_LOCATION_TYPE locType;
    U8 dataType = 0;
    static U8 *locData = NULL;
    static U32 dataPos = 0;
    static U32 totalSize = 0;
    dcInfo = iPodGetDevInfo();
    locType = (IPOD_LOCATION_TYPE)MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0];
    
    if(locType < LOCATION_TYPE_MAX)
    {
        switch(MessageHeaderInfo.telegramCmdId)
        {
            case IPOD_LOCATION_LINGO_GetDevCaps :
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetDevCaps");
                U8 capsData[IPOD_LOCATION_CAPS_MAX_SIZE] = {0};
                U8 size = 0;
                if(locType == LOCATION_TYPE_SYS_CAPS)
                {
                    U64 systemCaps = (U64)IPOD_LOCATION_SYSCAPS_MASK;
                    U64 locationCaps = (U64)IPOD_LOCATION_LOCCAPS_MASK;
                    locationCaps = locationCaps | (U64)(dcInfo[IPOD_DC_ASSIST_SUPPORT].para.val << IPOD_SHIFT_2);
                    capsData[IPOD_POS0] = IPOD_LOCATION_SUPPORT_MAJ_VER;
                    capsData[IPOD_POS1] = IPOD_LOCATION_SUPPORT_MINOR_VER;
                    iPod_convert_to_big64(&capsData[IPOD_POS2], systemCaps);
                    iPod_convert_to_big64(&capsData[IPOD_POS10], locationCaps);
                    size = IPOD_LOCATION_SYS_CAPS_MAX_LEN;
                }
                else if(locType == LOCATION_TYPE_GPS_CAPS)
                {
                    U64 gpsCaps = IPOD_LOCATION_SUPPORT_NMEA;
                    iPod_convert_to_big64(&capsData[IPOD_POS0], gpsCaps);
                    size = IPOD_LOCATION_GPS_SIZE;
                }
                else
                {
                    iPod_convert_to_big32(capsData,((U32 *)(VP)dcInfo[IPOD_DC_ASSIST_CAPS].para.p_val)[IPOD_POS0]);
                    iPod_convert_to_big32(&capsData[IPOD_POS4],((U32 *)(VP)dcInfo[IPOD_DC_ASSIST_CAPS].para.p_val)[IPOD_POS1]);
                    size = IPOD_LOCATION_ASSIST_SIZE;
                }

                rc = iPodRetDevCaps(iPodHndl->id, locType, capsData, size);
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodRetDevCaps() returns : rc = %d",rc);
                break;
            }

            case IPOD_LOCATION_LINGO_GetDevControl:
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetDevControl");
                rc = iPodExecuteCBLocation(iPodHndl, IPOD_GET_DEV_CTRL_CMD, locType, 0, NULL, 0);
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodExecuteCBLocation() returns : rc = %d",rc);
                break;
            }

            case IPOD_LOCATION_LINGO_SetDevControl:
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :SetDevControl");
                U8 ctrlData[IPOD_LOCATION_DEVCONTROL_SIZE] = {0};
                memcpy(ctrlData, &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1], IPOD_LOCATION_DEVCONTROL_SIZE);
                rc = iPodExecuteCBLocation(iPodHndl, IPOD_SET_DEV_CTRL_CMD, locType, 0, ctrlData, IPOD_LOCATION_DEVCONTROL_SIZE);
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodExecuteCBLocation() returns : rc = %d",rc);
                if(rc == IPOD_OK)
                {
                    rc = iPodDevAck(iPodHndl->id, IPOD_OK, (U8)MessageHeaderInfo.telegramCmdId, 0, 0);
                }

                break;
            }

            case IPOD_LOCATION_LINGO_GetDevData:
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :GetDevData");
                U8 locDevData[IPOD_LOCATION_DEVDATA_SIZE] = {0};
                U32 size = 0;
                if(locType == LOCATION_TYPE_ASSIST_CAPS)
                {
                    dataType = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                    if(dataType == IPOD_LOCATION_DATA_TYPE_MAX_REFRESH)
                    {
                        iPod_convert_to_big32(locDevData, (U32)dcInfo[IPOD_DC_SATEL_MAX_REFRESH].para.val);
                        size = IPOD_LOCATION_DEVDATA_SIZE;
                    }
                    else if(dataType == IPOD_LOCATION_DATA_TYPE_RECOM_REFRESH)
                    {
                        iPod_convert_to_big32(locDevData, (U32)dcInfo[IPOD_DC_SATEL_RECOM_REFRESH].para.val);
                        size = IPOD_LOCATION_DEVDATA_SIZE;
                    }
                    else
                    {
                        rc = IPOD_BAD_PARAMETER;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter dataType = %d",dataType);

                    }
                }
                else
                {
                    rc = IPOD_BAD_PARAMETER;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter locTpye = %d",locType);
                }
                
                if(rc == IPOD_OK)
                {
                    rc = iPodRetDevData(iPodHndl->id, locType, dataType, locDevData, size);
                }
                break;
            }

            case IPOD_LOCATION_LINGO_SetDevData:
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :SetDevData");
                U16 sectCur = 0;
                U16 sectMax = 0;
                U8 locDataPos = IPOD_POS6;
                U16 dataLen = 0;
                
                dataType = MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1];
                sectCur = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS2]);
                sectMax = iPod_convert_to_little16(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS4]);
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "dataType = 0x%02x sectCur = 0x%04x sectMax = 0x%04x",dataType,sectCur,sectMax);
                if(sectCur == 0)
                {
                    totalSize = iPod_convert_to_little32(&MessageHeaderInfo.iPodResponseBuffer[IPOD_POS6]);
                    locDataPos += IPOD_LOCATION_TOTALSIZE_LEN;
                    if((locData == NULL) && (totalSize > 0))
                    {
                        locData = (U8 *)calloc(totalSize, sizeof(U8));
                    }
                    else
                    {
                        rc = IPOD_OUT_OF_RESOURCES;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod out of resources");
                    }
                }

                if(locData != NULL)
                {
                    if(rc == IPOD_OK)
                    {
                        dataLen = MessageHeaderInfo.telegramLen - locDataPos;
                        if((totalSize - dataPos) >= dataLen)
                        {
                            memcpy(&locData[dataPos], &MessageHeaderInfo.iPodResponseBuffer[IPOD_POS10], dataLen);
                            dataPos += dataLen;
                        }
                        if(sectCur < sectMax)
                        {
                            rc = iPodDevAck(iPodHndl->id, IPOD_OK, (U8)MessageHeaderInfo.telegramCmdId, sectCur, 1);
                        }
                        else
                        {
                            rc = iPodExecuteCBLocation(iPodHndl, IPOD_SET_DEV_DATA_CMD, locType, dataType, locData, totalSize);
                            if((sectCur == 0) && (sectMax == 0))
                            {
                                rc = iPodDevAck(iPodHndl->id, rc, (U8)MessageHeaderInfo.telegramCmdId, 0, 0);
                            }
                            else
                            {
                                rc = iPodDevAck(iPodHndl->id, rc, (U8)MessageHeaderInfo.telegramCmdId, sectCur, 1);
                            }
                            /* todo */
                            dataPos = 0;
                            totalSize = 0;
                        }
                    }
                    free(locData);
                    locData = NULL;

                }
                else
                {
                    rc = IPOD_OUT_OF_RESOURCES;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod out of resources");
                }
                break;
            }
            case IPOD_LOCATION_LINGO_iPodAck:
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Command received from Apple Device :iPodAck");
                if(MessageHeaderInfo.iPodResponseBuffer[IPOD_POS0] != 0)
                {
                    rc = iPodExecuteCBLocation(iPodHndl, IPOD_IPOD_ACK_CMD, (IPOD_LOCATION_TYPE)0,
                                     MessageHeaderInfo.iPodResponseBuffer[IPOD_POS1], MessageHeaderInfo.iPodResponseBuffer, 1);
                }
                break;
            }
            default: 
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter MessageHeaderInfo.telegramCmdId = %d",MessageHeaderInfo.telegramCmdId);
                break;
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter locType = %d",locType);
    }

    if((rc != IPOD_OK) && (rc < IPOD_NOT_CONNECTED))
    {
        rc += 90;
        rc = iPodDevAck(iPodHndl->id, rc, (U8)MessageHeaderInfo.telegramCmdId, 0, 0);
    }

}

