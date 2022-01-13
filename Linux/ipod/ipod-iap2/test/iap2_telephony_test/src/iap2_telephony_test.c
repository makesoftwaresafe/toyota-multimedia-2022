/*
 * iap2_telephony_test.c
 */

/* **********************  includes  ********************** */
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>

#include "iap2_telephony_test.h"
#include "iap2_usb_role_switch.h"
#include <iap2_usb_role_switch.h>
#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include <endian.h>
#include "iap2_dlt_log.h"

iap2UserConfig_t g_iap2UserConfig;
iap2TestAppleDevice_t g_iap2TestDevice;
iap2FileXferBuf g_CoverArtBuf;
S32 g_rc = IAP2_OK;

typedef struct
{
    int socket_fd;
    iAP2InitParam_t iAP2InitParameter;
    U8* udevPath;
} iap2AppThreadInitData_t;


LOCAL S32 application_process_main (int socket_fd);
LOCAL void iap2AppThread(void* exinf);
LOCAL S32 iap2TestStartTelephonyCallStateInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopTelephonyCallStateInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartTelephonyUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopTelephonyUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2Device);

/* **********************  callbacks ********************** */
S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    switch(dState)
    {
        case iAP2NotConnected :
            g_iap2TestDevice.testDeviceState = iAP2NotConnected;
            printf("\t %d ms %p  Device state : iAP2NotConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2TransportConnected :
            g_iap2TestDevice.testDeviceState = iAP2TransportConnected;
            printf("\t %d ms %p  Device state : iAP2TransportConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkConnected:
            g_iap2TestDevice.testDeviceState = iAP2LinkConnected;
            printf("\t %d ms %p  Device state : iAP2LinkConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2AuthenticationPassed :
            g_iap2TestDevice.testDeviceState = iAP2AuthenticationPassed;
            printf("\t %d ms %p  Device state : iAP2AuthenticationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2IdentificationPassed :
            g_iap2TestDevice.testDeviceState = iAP2IdentificationPassed;
            printf("\t %d ms %p  Device state : iAP2IdentificationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2DeviceReady:
            g_iap2TestDevice.testDeviceState = iAP2DeviceReady;
            printf("\t %d ms %p  Device state : iAP2DeviceReady  \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkiAP1DeviceDetected:
            g_iap2TestDevice.testDeviceState = iAP2LinkiAP1DeviceDetected;
            printf("\t %d ms %p  Device state : iAP2LinkiAP1DeviceDetected\n", iap2CurrTimeMs(), iap2Device);
            break;
        default:
            g_iap2TestDevice.testDeviceState = iAP2ComError;
            printf("\t %d ms %p  Device state : unknown %d \n", iap2CurrTimeMs(), iap2Device, dState);
            rc = IAP2_CTL_ERROR;
            break;
    }

    return rc;
}

S32 iap2AuthenticationSucceeded_CB(iAP2Device_t* iap2Device, iAP2AuthenticationSucceededParameter* authParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context         = context;
    authParameter   = authParameter;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AuthenticationSucceeded_CB called ");
    printf("\t %d ms %p  iap2AuthenticationSucceeded_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2AuthenticationFailed_CB(iAP2Device_t* iap2Device, iAP2AuthenticationFailedParameter* authParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    authParameter = authParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AuthenticationFailed_CB called");
    printf("\t %d ms %p  iap2AuthenticationFailed_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationAccepted_CB(iAP2Device_t* iap2Device, iAP2IdentificationAcceptedParameter* idParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationAccepted_CB called");
    printf("\t %d ms %p  iap2IdentificationAccepted_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationRejected_CB(iAP2Device_t* iap2Device, iAP2IdentificationRejectedParameter* idParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationRejected_CB called");
    printf("\t %d ms %p  iap2IdentificationRejected_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2PowerUpdate_CB(iAP2Device_t* iap2Device, iAP2PowerUpdateParameter* powerupdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2PowerUpdate_CB called");

    if (powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory_count != 0)
    {
        printf("\t %d ms %p  iap2PowerUpdate_CB CurrentDrawn: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2AccessoryPowerMode_count != 0)
    {
        printf("\t %d ms %p  iap2PowerUpdate_CB AccessoryPowerMode: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2AccessoryPowerMode));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count != 0)
    {
        printf("\t %d ms %p  iap2PowerUpdate_CB BatteryWillCharge: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent));
        rc = IAP2_OK;
    }

    return rc;
}

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if(iAP2FileXferSession->iAP2FileXferRxLen > 0)
        {
            printf("\t   artwork available. FileID: %d  FileSize: %llu  \n",
                    iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

            g_CoverArtBuf.Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);
            g_CoverArtBuf.CurPos = g_CoverArtBuf.Buffer;
            if(g_CoverArtBuf.Buffer != NULL)
            {
                g_CoverArtBuf.CurReceived = 0;
                g_CoverArtBuf.FileID = iAP2FileXferSession->iAP2FileTransferID;
                g_CoverArtBuf.FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
                rc = IAP2_OK;
            }
            else
            {
                printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            printf("\t   no artwork available. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferDataRcvd_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferDataRcvd_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_CoverArtBuf.CurPos != NULL)
            && (g_CoverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            memcpy(g_CoverArtBuf.CurPos, iAP2FileXferSession->iAP2FileXferRxBuf, iAP2FileXferSession->iAP2FileXferRxLen);
            g_CoverArtBuf.CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
            g_CoverArtBuf.CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;

            iap2SetTestState(FILE_TRANSFER_DATA_RECV,TRUE);
            printf("\t   received artwork data for FileID: %d \n",iAP2FileXferSession->iAP2FileTransferID);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferSuccess_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;
    FILE *fp;
    char fileName[256];

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_CoverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID)
            && (g_CoverArtBuf.CurReceived == g_CoverArtBuf.FileSize) )
        {
            memset(&fileName[0], 0, (sizeof(fileName)));
            sprintf(&fileName[0], "%s%d%s", "/tmp/CoverArt", iAP2FileXferSession->iAP2FileTransferID, ".vcf");

            fp = fopen(&fileName[0], "w");
            if(fp != NULL)
            {
                fwrite(g_CoverArtBuf.Buffer, 1, g_CoverArtBuf.FileSize, fp);
                fclose(fp);

                printf("\t   File Transfer Success!  Please check %s \n", &fileName[0]);
            }

            if(g_CoverArtBuf.Buffer != NULL)
            {
                iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
                g_CoverArtBuf.CurPos = NULL;
                memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
            }

            iap2SetTestState(FILE_TRANSFER_SUCCESS_RECV,TRUE);
        }
        else
        {
            printf("\t   Did not receive all File Transfer data. \n");
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferFailure_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferFailure_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer failed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);

        if(g_CoverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
            g_CoverArtBuf.CurPos = NULL;
            memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_FAILED_RECV,TRUE);

    return rc;
}

S32 iap2FileTransferCancel_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferCancel_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer canceled.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        if(g_CoverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
            g_CoverArtBuf.CurPos = NULL;
            memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_CANCELED_RECV,TRUE);
    return rc;
}

S32 iap2FileTransferPause_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferPause_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer paused.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferResume_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferResume_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer resumed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2TestTelephoneCallStateInfoUpdates_cb(iAP2Device_t* iap2Device, iAP2TelephonyCallStateInformationParameter* theiAP2TelephonyCallStateInformationParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* to fix compiler warning */
    iap2Device = iap2Device;
    context = context;

    if(theiAP2TelephonyCallStateInformationParameter != NULL)
    {
        printf("\n ***************************************************************************************************************************\n");
        printf("\n *\t\t\t\t Telephony Call state Information");
        printf("\n ***************************************************************************************************************************\n");
        if(theiAP2TelephonyCallStateInformationParameter->iAP2CallStatePhoneNumber_count > 0)
        {
            printf("\n\t** Phone number:%s **", *(theiAP2TelephonyCallStateInformationParameter->iAP2CallStatePhoneNumber));
        }
        if(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateCallerName_count > 0)
        {
            printf("\n\t** Caller name: %s **", *(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateCallerName));
        }
        if(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus_count > 0)
        {
            printf("\n\t** Call status: %d **", *(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus));
        }
        if(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection_count > 0)
        {
            printf("\n\t** call state direction: %d **", *(theiAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection));
        }
        if(theiAP2TelephonyCallStateInformationParameter->iAP2UniqueCallID_count > 0)
        {
            printf("\n\t** Unique call Id: %s **", *(theiAP2TelephonyCallStateInformationParameter->iAP2UniqueCallID));
        }
        if(theiAP2TelephonyCallStateInformationParameter->iAP2CallStatevCardFileTransferIdentifier_count > 0)
        {
            printf("\n\t** File transfer ID: %d **", *(theiAP2TelephonyCallStateInformationParameter->iAP2CallStatevCardFileTransferIdentifier));
        }
    }

    return rc;
}

S32 iap2TestTelephoneUpdate_cb(iAP2Device_t* iap2Device, iAP2TelephonyUpdateParameter* theiAP2TelephonyUpdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* to fix compiler warnings */
    iap2Device = iap2Device;
    context = context;

    if(theiAP2TelephonyUpdateParameter != NULL)
    {
        printf("\n ***************************************************************************************************************************\n");
        printf("\n *\t\t\t\t Telephony Update");
        printf("\n ***************************************************************************************************************************\n");
        if(theiAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength_count > 0)
        {
            printf("\n\t** Signal Strength:%d **", *(theiAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength));
        }
        if(theiAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus_count > 0)
        {
            printf("\n\t** Registration status:%d **", *(theiAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus));
        }
        if(theiAP2TelephonyUpdateParameter->iAP2TelephonyAirplaneModeStatus_count > 0)
        {
            printf("\n\t** Airplane mode status:%d **", *(theiAP2TelephonyUpdateParameter->iAP2TelephonyAirplaneModeStatus));
        }
        if(theiAP2TelephonyUpdateParameter->iAP2TelephonyTTYStatus_count > 0)
        {
            printf("\n\t** Telephony TTY status:%d **", *(theiAP2TelephonyUpdateParameter->iAP2TelephonyTTYStatus));
        }
        if(theiAP2TelephonyUpdateParameter->iAP2TelephonyMobileOperator_count > 0)
        {
            printf("\n\t** Mobile operator:%s **", *(theiAP2TelephonyUpdateParameter->iAP2TelephonyMobileOperator));
        }
    }
    iap2SetTestStateError(TRUE);

    return rc;
}
/* **********************  functions ********************** */


void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks)
{
    iap2CSCallbacks->iAP2AuthenticationFailed_cb                     = &iap2AuthenticationFailed_CB;
    iap2CSCallbacks->iAP2AuthenticationSucceeded_cb                 = &iap2AuthenticationSucceeded_CB;
    iap2CSCallbacks->iAP2RequestAuthenticationCertificate_cb        = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationChallengeResponse_cb  = NULL;
    iap2CSCallbacks->iAP2StartIdentification_cb                     = NULL;
    iap2CSCallbacks->iAP2IdentificationAccepted_cb                  = &iap2IdentificationAccepted_CB;
    iap2CSCallbacks->iAP2IdentificationRejected_cb                  = &iap2IdentificationRejected_CB;
    iap2CSCallbacks->iAP2BluetoothConnectionUpdate_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationCertificate_cb         = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationResponse_cb            = NULL;
    iap2CSCallbacks->iAP2DeviceInformationUpdate_cb                 = NULL;
    iap2CSCallbacks->iAP2DeviceLanguageUpdate_cb                    = NULL;
    iap2CSCallbacks->iAP2TelephonyCallStateInformation_cb           = &iap2TestTelephoneCallStateInfoUpdates_cb;
    iap2CSCallbacks->iAP2TelephonyUpdate_cb                         = &iap2TestTelephoneUpdate_cb;
    iap2CSCallbacks->iAP2StartVehicleStatusUpdates_cb               = NULL;
    iap2CSCallbacks->iAP2StopVehicleStatusUpdates_cb                = NULL;
    iap2CSCallbacks->iAP2AssistiveTouchInformation_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceHIDReport_cb                         = NULL;
    iap2CSCallbacks->iAP2StartLocationInformation_cb                = NULL;
    iap2CSCallbacks->iAP2StopLocationInformation_cb                 = NULL;
    iap2CSCallbacks->iAP2MediaLibraryInformation_cb                 = NULL;
    iap2CSCallbacks->iAP2MediaLibraryUpdate_cb                      = NULL;
    iap2CSCallbacks->iAP2NowPlayingUpdateParameter_cb               = NULL;
    iap2CSCallbacks->iAP2PowerUpdate_cb                             = &iap2PowerUpdate_CB;
    iap2CSCallbacks->iAP2USBDeviceModeAudioInformation_cb           = NULL;
    iap2CSCallbacks->iAP2VoiceOverUpdate_cb                         = NULL;
    iap2CSCallbacks->iAP2WiFiInformation_cb                         = NULL;
}

void iap2InitFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks)
{
    iAP2FileTransferCallbacks->iAP2FileTransferSuccess_cb           = iap2FileTransferSuccess_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferFailure_cb           = iap2FileTransferFailure_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferCancel_cb            = iap2FileTransferCancel_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferPause_cb             = iap2FileTransferPause_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferResume_cb            = iap2FileTransferResume_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferDataRcvd_cb          = iap2FileTransferDataRcvd_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferSetup_cb             = iap2FileTransferSetup_CB;
}

S32 iap2HdlComThreadPollMqEvent_CB(iAP2Device_t* iap2Device, S32 mqFD, S32 mqFdSendAck, BOOL* b_endComThread)
{
    S32 rc = IAP2_OK;
    char recvBuf[TEST_MQ_MAX_SIZE];

    char ack_mq_cmd = MQ_CMD_ACK;

    iap2ComThreadMq_t *mq_st = NULL;

    /* receive message from AppThread */
    rc = iap2RecvMq(mqFD, &recvBuf[0], TEST_MQ_MAX_SIZE);
    if(rc > 0)
    {
        mq_st = (iap2ComThreadMq_t*)recvBuf;
        if(mq_st == NULL){
            ack_mq_cmd = MQ_CMD_ERROR;
            rc = IAP2_CTL_ERROR;
            printf("\t %u ms  iap2HdlComThreadPollMqEvent():  mq_st is NULL \n", iap2CurrTimeMs());
        } else{
            switch(mq_st->mq_cmd)
            {
                case MQ_CMD_EXIT_IAP2_COM_THREAD:
                {
                    printf(" %u ms  iap2HdlComThreadPollMqEvent():  leave poll thread\n", iap2CurrTimeMs());
                    *b_endComThread = TRUE;
                    break;
                }
                case MQ_CMD_START_TELEPHONY_CALLSTATE_INFO:
                {
                    /* send Start TelephonyCallStateInfo to device */
                    rc = iAP2StartTelephonyCallStateInformation(iap2Device, (iAP2StartTelephonyCallStateInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StartTelephonyCallStateInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_START_TELEPHONY_UPDATE:
                {
                    /* send Start TelephonyUpdates to device */
                    rc = iAP2StartTelephonyUpdates(iap2Device, (iAP2StartTelephonyUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iap2TestStartTelephonyUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_STOP_TELEPHONY_CALLSTATE_INFO:
                {
                    /* send Stop TelephonyCallStateInfo to device */
                    rc = iAP2StopTelephonyCallStateInformation(iap2Device, (iAP2StopTelephonyCallStateInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StopTelephonyCallStateInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_STOP_TELEPHONY_UPDATES:
                {
                    /* send StopTelephonyUpdates to device */
                    rc = iAP2StopTelephonyUpdates(iap2Device, (iAP2StopTelephonyUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iap2TestStopTelephonyUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_CANCEL_IAP1_SUPPORT:
                {
                    /* send CanceliAP1Support */
                    rc = iAP2CanceliAP1Support(iap2Device);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2CanceliAP1Support failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                default:
                {
                    ack_mq_cmd = MQ_CMD_ERROR;
                    rc = IAP2_CTL_ERROR;
                    break;
                }
            }/* switch */
        }
    }
    else
    {
        ack_mq_cmd = MQ_CMD_ERROR;
        rc = IAP2_CTL_ERROR;
    }

    if(mqFdSendAck > 0){
        (void)iap2SendMq(mqFdSendAck, &ack_mq_cmd, sizeof(ack_mq_cmd));
    }

    return rc;
}

LOCAL S32 iap2TestStartTelephonyCallStateInfo(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartTelephonyCallStateInformationParameter theiAP2StartTelephonyCallStateInformationParameter;

    /* Temporary fix for compiler warnings */
    iap2Device = iap2Device;

    memset(&theiAP2StartTelephonyCallStateInformationParameter, 0, sizeof(iAP2StartTelephonyCallStateInformationParameter));
    theiAP2StartTelephonyCallStateInformationParameter.iAP2CallStatevCardFileTransferIdentifier_count++;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                            MQ_CMD_START_TELEPHONY_CALLSTATE_INFO,
                            &theiAP2StartTelephonyCallStateInformationParameter, sizeof(theiAP2StartTelephonyCallStateInformationParameter));

    return rc;
}

LOCAL S32 iap2TestStopTelephonyCallStateInfo(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;

    iAP2StopTelephonyCallStateInformationParameter theiAP2StopTelephonyCallStateInformationParameter;

    /* Temporary fix for compiler warnings */
    iap2Device = iap2Device;

    memset(&theiAP2StopTelephonyCallStateInformationParameter, 0, sizeof(iAP2StopTelephonyCallStateInformationParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                            MQ_CMD_STOP_TELEPHONY_CALLSTATE_INFO,
                            &theiAP2StopTelephonyCallStateInformationParameter, sizeof(theiAP2StopTelephonyCallStateInformationParameter));

    return rc;
}

LOCAL S32 iap2TestStartTelephonyUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartTelephonyUpdatesParameter theiAP2StartTelephonyUpdatesParameter;

    /* Temporary fix for compiler warnings */
    iap2Device = iap2Device;

    memset(&theiAP2StartTelephonyUpdatesParameter, 0, sizeof(iAP2StartTelephonyUpdatesParameter));
    theiAP2StartTelephonyUpdatesParameter.iAP2TelephonySignalStrength_count++;
    theiAP2StartTelephonyUpdatesParameter.iAP2TelephonyRegistrationStatus_count++;
    theiAP2StartTelephonyUpdatesParameter.iAP2TelephonyAirplaneModeStatus_count++;
    theiAP2StartTelephonyUpdatesParameter.iAP2TelephonyTTYStatus_count++;
    theiAP2StartTelephonyUpdatesParameter.iAP2TelephonyMobileOperator_count++;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                            MQ_CMD_START_TELEPHONY_UPDATE,
                            &theiAP2StartTelephonyUpdatesParameter, sizeof(theiAP2StartTelephonyUpdatesParameter));

    return rc;
}
LOCAL S32 iap2TestStopTelephonyUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;

    iAP2StopTelephonyUpdatesParameter theiAP2StopTelephonyUpdatesParameter;

    /* Temporary fix for compiler warnings */
    iap2Device = iap2Device;

    memset(&theiAP2StopTelephonyUpdatesParameter, 0, sizeof(iAP2StopTelephonyUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                            MQ_CMD_STOP_TELEPHONY_UPDATES,
                            &theiAP2StopTelephonyUpdatesParameter, sizeof(theiAP2StopTelephonyUpdatesParameter));

    return rc;
}

/* send Bad DETECT Ack to device to cancel iAP1 support */
LOCAL S32 iap2TestCanceliAP1Support(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                        MQ_CMD_CANCEL_IAP1_SUPPORT, NULL, 0);

    return rc;
}

LOCAL S32 iap2TestStopPollThread(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd, MQ_CMD_EXIT_IAP2_COM_THREAD, NULL, 0);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2device)
{
    S32 rc = IAP2_OK;

    if(iap2GetTestStateError() != TRUE)
    {

        if(iap2GetTestStateError() != TRUE)
        {
            printf("\n %d ms Start telephony updates()\n", iap2CurrTimeMs());
            rc = iap2TestStartTelephonyCallStateInfo(mq_fd, iap2device);
            printf("\n %d ms iap2TestStartTelephonyUpdates() \n", iap2CurrTimeMs());
            rc  = iap2TestStartTelephonyUpdates(mq_fd, iap2device);
            printf("\nMake a call to the connected device within 30 seconds to test telephony feature\n");
            iap2SleepMs(30000);
        }

          /* check if Apple device was disconnected */
          if(g_iap2TestDevice.testDeviceState == iAP2NotConnected){
              printf("\n %d ms  Apple device was disconnected! \n", iap2CurrTimeMs());
          }
          else
          {
              /* stop telephony call state info update*/
              printf("\n %d ms  iap2TestStopTelephonyCallStateInfo() \n", iap2CurrTimeMs());
              rc = iap2TestStopTelephonyCallStateInfo(mq_fd, iap2device);
              /* stop telephony update */
              printf("\n %d ms iap2TestStopTelephonyUpdates() \n", iap2CurrTimeMs());
              rc = iap2TestStopTelephonyUpdates(mq_fd, iap2device);
          }
          iap2SleepMs(1000);
    }
    return rc;
}

/* application thread */
void iap2AppThread(void* exinf)
{
    S32 rc = IAP2_OK;

    TEST_THREAD_ID threadID = 0;
    char threadName[8];
    void* status;
    mqd_t mq_fd = -1;

    U32 retry_count = 0;

    iap2AppThreadInitData_t* iAP2AppThreadInitData = (iap2AppThreadInitData_t*)exinf;
    iAP2InitParam_t* iAP2InitParameter = &(iAP2AppThreadInitData->iAP2InitParameter);
    iAP2Device_t* iap2device = NULL;
    int socket_fd = iAP2AppThreadInitData->socket_fd;

    rc = iap2CreateMq(&mq_fd, TEST_MQ_NAME, O_CREAT | O_RDWR);
    if(rc != IAP2_OK)
    {
        printf("  create mq %s failed %d \n", TEST_MQ_NAME, rc);
    }

    if(rc == IAP2_OK)
    {
        rc = iap2CreateMq(&g_iap2TestDevice.mqAppTskFd, TEST_MQ_NAME_APP_TSK, O_CREAT | O_RDWR);
        if(rc != IAP2_OK)
        {
            printf("  create mq %s failed %d \n", TEST_MQ_NAME_APP_TSK, rc);
        }
    }

    /**
     * create internal device structure and initialize
     * with iAP2InitParameter provided by application.
     */
    iap2device = iAP2InitDeviceStructure(iAP2InitParameter);
    printf(" %d ms  iAP2InitDeviceStructure = %p \n", iap2CurrTimeMs(), iap2device);
    IAP2TESTDLTLOG(DLT_LOG_ERROR, "iAP2InitDeviceStructure rc = %p ", iap2device);

    if(NULL != iap2device)
    {
        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }

        if(rc == IAP2_OK)
        {
            rc = iAP2InitDeviceConnection(iap2device);
            printf(" %d ms  iAP2InitDeviceConnection:   rc  = %d \n", iap2CurrTimeMs(), rc);
            IAP2TESTDLTLOG(DLT_LOG_DEBUG,"iAP2InitDeviceConnection rc = %d ",rc);
        }

        if(rc == IAP2_OK)
        {
            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iCom", 1);

            /* -----------  start polling thread  ----------- */
            threadID = iap2CreateThread(iap2ComThread, &threadName[0], iap2device);
            if(iap2VerifyThreadId(threadID) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
        }

        if(rc == IAP2_OK)
        {
            iap2SetTestState(RUNNING, TRUE);
            /* wait until device is attached */
            while( (g_iap2TestDevice.testDeviceState != iAP2DeviceReady) && (g_iap2TestDevice.testDeviceState != iAP2LinkiAP1DeviceDetected)
                    && (g_iap2TestDevice.testDeviceState != iAP2ComError) && (retry_count < 300) )
            {
                if(g_iap2TestDevice.testDeviceState == iAP2NotConnected)
                {
                    rc = IAP2_DEV_NOT_CONNECTED;
                    break;
                }
                iap2SleepMs(50);
                retry_count++;
            }
            if(g_iap2TestDevice.testDeviceState == iAP2DeviceReady)
            {
                printf(" %d ms  device attached  retry: %d \n", iap2CurrTimeMs(), retry_count);
                rc = IAP2_OK;
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2LinkiAP1DeviceDetected)
            {
                printf(" %d ms  iAP1 device detected  retry: %d \n", iap2CurrTimeMs(), retry_count);

                printf(" iAP1 not supported. send Bad DETECT Ack to device \n");
                rc = iap2TestCanceliAP1Support(mq_fd, iap2device);

                iap2SetTestStateError(TRUE);
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2ComError)
            {
                printf("%d ms Error in Authentication and Identification. \n", iap2CurrTimeMs());
                iap2SetTestStateError(TRUE);
            }
            else
            {
                printf(" %d ms  device not attached [state: %d | retry: %d] \n\n", iap2CurrTimeMs(), g_iap2TestDevice.testDeviceState, retry_count);
                iap2SetTestStateError(TRUE);
            }
        }

        if(rc == IAP2_OK)
        {
            rc= iap2StartTest(mq_fd,iap2device);
        }
        if(rc != IAP2_OK)
        {
            g_rc = rc;
        }

        /* exit pollThread */
        if(threadID != 0)
        {
            (void)iap2TestStopPollThread(mq_fd, iap2device);
        }
        iap2SetTestState(STOPPED, TRUE);
        iap2SetTestState(RUNNING, FALSE);

        /* -----------  clean thread and mq  ----------- */
        if(threadID != 0)
        {
           rc = pthread_join(threadID, &status);
        }
        if(mq_fd > 0)
        {
            rc = mq_close(mq_fd);
            rc = mq_unlink(TEST_MQ_NAME);
        }
        if(g_iap2TestDevice.mqAppTskFd > 0)
        {
            rc = mq_close(g_iap2TestDevice.mqAppTskFd);
            rc = mq_unlink(TEST_MQ_NAME_APP_TSK);
        }

        rc = iAP2DisconnectDevice(iap2device);
        printf(" %d ms  iAP2DisconnectDevice:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DisconnectDevice = %d", rc);

        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }

        /* de-initialize the device structure */
        rc = iAP2DeInitDeviceStructure(iap2device);
        printf(" %d ms  iAP2DeInitDeviceStructure:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DeInitDeviceStructure = %d ", rc);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        g_rc = rc;
    }

    printf(" %d ms:  exit iap2AppThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}

LOCAL S32 application_process_main (int socket_fd) {
    S32 rc = IAP2_CTL_ERROR;

    TEST_THREAD_ID AppTskID = 0;
    char threadName[8];
    void* status;

    iap2AppThreadInitData_t iAP2AppThreadInitData;
    iAP2InitParam_t *iAP2InitParameter = &iAP2AppThreadInitData.iAP2InitParameter;

    iAP2AppThreadInitData.socket_fd = socket_fd;

    signal(SIGINT, signalHandlerAppProcess);
    signal(SIGQUIT, signalHandlerAppProcess);

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

    memset(iAP2InitParameter, 0, sizeof(iAP2InitParam_t) );

    rc = iap2InitDev(iAP2InitParameter);

    if(rc == IAP2_OK)
    {
        /* Application provided Data */
        rc = iap2SetInitialParameter(iAP2InitParameter, g_iap2UserConfig);
        printf(" iap2SetInitialParameter:  rc  = %d \n",rc);
    }
    if(rc == IAP2_OK)
    {
            /* copy pointer to udev path of testing device */
            iAP2AppThreadInitData.udevPath = g_iap2TestDevice.udevDevice.udevPath;

            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iApp", 1);

            /* -----------  start application thread  ----------- */
            AppTskID = iap2CreateThread(iap2AppThread, &threadName[0], &iAP2AppThreadInitData);
            if(iap2VerifyThreadId(AppTskID) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
        /* -----------  clean thread  ----------- */
        if(AppTskID != 0)
        {
            (void)pthread_join(AppTskID, &status);
        }
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2Udev failed with %d \n", rc);
        g_rc = rc;
    }
    iap2DeinitDev(iAP2InitParameter);

    printf("\n");
    if(g_rc == IAP2_OK)
    {
        printf("iap2_telephony_test                 PASS       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 TELEPHONY TEST PASS *****");
    }
    else
    {
        printf("iap2_telephony_test                 FAIL       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 TELEPHONY TEST FAIL *****");
    }

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return g_rc;
}


S32 main(int argc, const char** argv)
{
    S32 rc;
    int socket_fds[2];
    static const int host_device_switch_process = 0;
    static const int application_process = 1;
    pid_t host_device_switch_process_pid;

    memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
    memset(&g_iap2UserConfig, 0, sizeof(g_iap2UserConfig));

    SetGlobPtr(&g_iap2TestDevice);

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iap2UserConfig Structure.
     */
    rc = iap2GetArguments(argc, argv, &g_iap2UserConfig);
    if (rc == IAP2_OK) {
        socketpair(PF_LOCAL, SOCK_STREAM, 0, socket_fds);

        host_device_switch_process_pid = fork();
        if (host_device_switch_process_pid == 0) {
            close(socket_fds[application_process]);

            host_device_switch_process_main(socket_fds[host_device_switch_process], &g_iap2UserConfig);
            close(socket_fds[host_device_switch_process]);
            rc = 0;
        } else {
            close(socket_fds[host_device_switch_process]);

            if (g_iap2UserConfig.app_thread_user != 0) {
                printf ("Setting app thread user %d, group %d and %d supplementary groups\n", g_iap2UserConfig.app_thread_user, g_iap2UserConfig.app_thread_prim_group, g_iap2UserConfig.app_thread_groups_cnt);

                setgroups( g_iap2UserConfig.app_thread_groups_cnt, g_iap2UserConfig.app_thread_groups );
                if(setgid( g_iap2UserConfig.app_thread_prim_group ) < 0 )
                {
                    printf("setgid() failed %s\n", strerror(errno));
                }
                if(setuid( g_iap2UserConfig.app_thread_user ) < 0)
                {
                    printf("setuid() failed %s\n", strerror(errno));
                }

                /* don't ask why */
                prctl (PR_SET_DUMPABLE, 1);
            }

            rc = application_process_main(socket_fds[application_process]);
            close(socket_fds[application_process]);
        }
    }

    iap2TestFreePtr( (void**)&g_iap2UserConfig.app_thread_groups);

    return rc;
}
