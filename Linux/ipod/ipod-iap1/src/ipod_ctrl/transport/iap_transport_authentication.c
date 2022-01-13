
#include <adit_typedef.h>
#include <authentication.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <string.h>

#include "ipodcommon.h"
#include "iap_transport_os.h"
#include "iap_init.h"
#include "iap_util_func.h"
#include "iap_general.h"
#include "iap_commands.h"
#include "iap_callback.h"
#include "iap_transport_authentication.h"
#include "iap_transport_process_message.h"
#include "iap_transport_os.h"
#include "iap1_dlt_log.h"

EXPORT IPOD_SEM_ID g_semWorkerId     = 0;
EXPORT U16 g_iPodCertTransID = 0;
EXPORT U8* g_iPodAuthenticationBuffer = NULL; /* buffer for the authenti-   *
                                               * cation data from iPod      */
EXPORT U16 g_iPodAuthenticationBufferLength = 0;
                                               
LOCAL IPOD_TASK_ID g_iPodUSBWorkerTask_id = 0;
LOCAL IPOD_FLG_ID g_flgWorkerID  = {0, 0};       /* eventflag to start autentication in worker task        */
LOCAL U16 g_task_stop_flag       = 0;
LOCAL IPOD_INSTANCE* gp_iPodWorking = NULL;
    
LOCAL S32 iPodIdentifyDeviceLingos(IPOD_INSTANCE* iPodHndl);
LOCAL S32 iPodStartIDPS(void);
LOCAL S32 iPodSetFIDTokenValues(void);
LOCAL S32 iPodEndIDPS(U8 endStatus);
LOCAL S32 iPodFIDTokenResult(U8 *buf, U8 **tokenValue, U16 *tokenLen);
LOCAL S32 iPodGetAllLingoVersion(void);
LOCAL void iPodDeleteLingoVersion(void);
LOCAL S32 iPodGetAllOptions(void);
LOCAL void iPodDeleteLingoOption(void);
LOCAL S32 iPodSetIdentifyPreference(void);

LOCAL S32 iPodSetFIDIdentify(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDAccCaps(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDAccInfo(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDPref(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDSDKProtocol(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDBundleSeed(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDMicrophoneCaps(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL S32 iPodSetFIDMetadataToken(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokennum);
LOCAL S32 iPodSetFIDScreenInfoToken(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum);
LOCAL void iPodFreeFIDTokenValue(U8 **tokenValue);

LOCAL S32 iPodRetSignatureData(U16 SignatureDataLength, const U8 *SignatureData);
LOCAL S32 iPodSendAuthenticationCertificate(U8 MajorVersion, U8 MinorVersion, U16 CertLength, const U8  *CertData);

LOCAL void iPodStartIdentification(S32 flg);
LOCAL void iPodGetCertificate(void);
LOCAL void iPodGenerateSignature(void);


LOCAL void iPodStartIdentification(S32 flg)
{
    IPOD_Cfg *dcInfo = iPodGetDevInfo();
    IPOD_TRANSPORT* transport = &gp_iPodWorking->transport;
    U32 tmout        = (U32)dcInfo[IPOD_DC_WAIT_TMO].para.val;
    U8 count         = 0;
    U8 oldiPod = 0;
    S32 err                  =   IPOD_OK;

    if (gp_iPodWorking != NULL)
    {
        transport->transportMaxPayloadSize = IPOD_TRANSPORT_DEFAULT_MAX_PAYLOAD_SIZE;
        iPodDeleteLingoVersion();
        iPodDeleteLingoOption();
        /* JIRA[SWGII-4851] */
        /* if iPod is disconnected and re-connected, Transaction  ID must be 0 */
        /* otherwise transaction ID must be increased even when iPod identify again */
        if((flg & IPOD_WORKER_IDENTIFY) == IPOD_WORKER_IDENTIFY)
        {
            iPodSetStartIDPS(gp_iPodWorking, TRUE);
            oldiPod = 0;
            dcInfo[IPOD_DC_WAIT_TMO].para.val = IPOD_IDPS_WAIT_TMO;
        }
        else if((flg & IPOD_WORKER_REQUESTIDENTIFY) == IPOD_WORKER_REQUESTIDENTIFY)
        {
            /* Dont reset the Transaction IDs*/
            gp_iPodWorking->rcvMsgInfo.startIDPS = TRUE;
            gp_iPodWorking->rcvMsgInfo.supportCancelCmd = TRUE;
            oldiPod = 0;
            dcInfo[IPOD_DC_WAIT_TMO].para.val = IPOD_IDPS_WAIT_TMO;
        }
        else
        {
            if(gp_iPodWorking->rcvMsgInfo.startIDPS == FALSE)
            {
                oldiPod = 1;
            }
        }
                
        if(oldiPod == 0)
        {
            do
            {
                err = iPodStartIDPS();
                IAP1_IDPSF_LOG(DLT_LOG_INFO, "iPodStartIDPS() returns = %d",err);
                if(err == IPOD_OK)
                {
                    (void)iPodRequestTransportMaxPacketSize(gp_iPodWorking->id, &transport->transportMaxPayloadSize);

                    err = iPodGetAllOptions();
                    if (IPOD_OK != err)
                    {
                        err = iPodGetAllLingoVersion();
                    }
                    if (err == IPOD_OK)
                    {
                        dcInfo[IPOD_DC_WAIT_TMO].para.val = (S32)tmout;
                        err = iPodSetFIDTokenValues();
                    }
                    IAP1_IDPSF_LOG(DLT_LOG_INFO, "iPodSetFIDTokenValues() returns = %d",err);
                    if(err == IPOD_OK)
                    {
                        err = iPodEndIDPS(0);
                    }
                    else
                    {
                        err = iPodEndIDPS(1);
                    }
                    IAP1_IDPSF_LOG(DLT_LOG_INFO, "iPodEndIDPS() returns = %d",err);
                }
                count++;
            }while((err != IPOD_OK) && (err != IPOD_NOT_CONNECTED) 
                   && (err != IPOD_BAD_PARAMETER) && (count < IPOD_IDPS_RETRY_COUNT) && (err != IPOD_IDPS_ERR_TMOUT));

            dcInfo[IPOD_DC_WAIT_TMO].para.val = (S32)tmout;
                    
            if((err != IPOD_OK) && (err != IPOD_NOT_CONNECTED))
            {
                iPodSetStartIDPS(gp_iPodWorking, FALSE);
                gp_iPodWorking->rcvMsgInfo.iPodTransID = 0;
                gp_iPodWorking->rcvMsgInfo.accTransID = 0;
                err = iPodIdentifyDeviceLingos(gp_iPodWorking);
            }
        }
        else
        {
            err = iPodIdentifyDeviceLingos(gp_iPodWorking);
        }
                
        if((err != IPOD_OK) && (err != IPOD_NOT_CONNECTED))
        {
            iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
        }
    }
}

LOCAL void iPodGetCertificate(void)
{
    static U8  CertData[IPOD_AUTH_CP_MAX_CERTLENGTH] = {0};
    U16 CertLength = 0;
    U8  count = 0;
    S32 err = IPOD_OK;
    U8 MajorVer = 0;
    U8 MinorVer = 0;

    err = AuthenticationGetProtocolVersion(&MajorVer, &MinorVer);
    if (err != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, " Failed to get Authentication protocol version = %d", err);
    }
    else
    {
        for (count = 0;
             ((CertLength == 0) || (CertLength > IPOD_AUTH_CP_MAX_CERTLENGTH)) && (count <= IPOD_USB_MAX_TRY_COUNT);
             count++)
        {
            AuthenticationGetCertificate(&CertLength, CertData);
        }

        if ((CertLength > 0) && (CertLength <= IPOD_AUTH_CP_MAX_CERTLENGTH) && (count <= IPOD_USB_MAX_TRY_COUNT))
        {
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, " Certificate data length = %d",CertLength);
            err = iPodSendAuthenticationCertificate(MajorVer, MinorVer, CertLength, CertData);
            if((err != IPOD_OK) && (err != IPOD_NOT_CONNECTED))
            {
                iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
            }
        }
        else if (count > IPOD_USB_MAX_TRY_COUNT)
        {
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, " Exceeds_system_limits count = %d",count);
            iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
        }
        else
        {
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, " Authentication_Certification_Length_incorrect! err = %d",err);
            iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
        }
    }
}

LOCAL void iPodGenerateSignature(void)
{
    U8  SignatureData[IPOD_AUTH_CP_SIGN_DATA_SIZE]   =  {0};
    U16 SignatureDataLength  =   0;
    S32 err                  =   IPOD_OK;

    err = AuthenticationGetSignatureData(g_iPodAuthenticationBuffer,
                                         g_iPodAuthenticationBufferLength,
                                         &SignatureDataLength,
                                         SignatureData);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, " AuthenticationGetSignatureData() returns : err = %d",err);
    if(err == IPOD_OK)
    {
        if((g_task_stop_flag & IPOD_STOP_AUTHENTICATE) == 0)
        {
            if (SignatureDataLength > 0)
            {
                err = iPodRetSignatureData(SignatureDataLength, SignatureData);
                if((err != IPOD_OK) && (err != IPOD_NOT_CONNECTED))
                {
                    iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
                }
            }
            else
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "SignatureDataLength = %d",SignatureDataLength);
                iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_APPLE_CP);
            }
        }
    }
    else
    {
        iPodWorkerExeCB(gp_iPodWorking, err);
    }

    free(g_iPodAuthenticationBuffer);
    g_iPodAuthenticationBuffer = NULL;
    g_iPodAuthenticationBufferLength = 0;
}


/*!
 * \fn iPodWorkerTask
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function gets signature data from iPod.
 */
void iPodWorkerTask(void* exinf)
{
    S32 err = IPOD_OK;
    S32 flg = 0;

    exinf = exinf;

    while ((g_task_stop_flag & IPOD_STOP_WORKERTASK) == 0)
    {
        /* Change the check to Flag for switch audio out put Feb/19/2010 */
        flg = iPodWorkerWaitForEvent(IPOD_TMO_FEVR);
        if ((flg > IPOD_OK)&&(gp_iPodWorking != NULL))
        {
            /* Add multi flag check Feb/19/2010 */
            /* To start identification, Identify command is sent to iPod */
            /* either StartIDPS or IdentifyDeviceLingoes (Step 1) */
            if( (((flg & IPOD_WORKER_IDENTIFY) == IPOD_WORKER_IDENTIFY) ||
                 ((flg & IPOD_WORKER_REIDENTIFY) == IPOD_WORKER_REIDENTIFY) ||
                 ((flg & IPOD_WORKER_REQUESTIDENTIFY) == IPOD_WORKER_REQUESTIDENTIFY)) &&
                ((flg & IPOD_STOP_WORKERTASK) != IPOD_STOP_WORKERTASK))
            {
                iPodStartIdentification(flg);
            }
            
            /* JIRA[SWGII-4802] */
            /* RetDevAuthentication must wait an ack. */
            /* iPod requested certificate, get from CP and send to iPod (Step 2) */
            if(((flg & IPOD_WORKER_CERTIFICATE) == IPOD_WORKER_CERTIFICATE) &&
               ((flg & IPOD_STOP_WORKERTASK) != IPOD_STOP_WORKERTASK))
            {
                iPodGetCertificate();
            }
                
            /* iPod requested signature, generate on CP and send to iPod (Step 3) */
            if(((flg & IPOD_WORKER_SIGNATURE) == IPOD_WORKER_SIGNATURE) &&
               ((flg & IPOD_STOP_WORKERTASK) != IPOD_STOP_WORKERTASK))
            {
                iPodGenerateSignature();
            }
                
            /* IdentifyDeviceLingoes case: need to set preferences after Signature finished. */
            /* Special case only for older iPods (Step 4) */
            if(((flg & IPOD_WORKER_PREFERENCE) == IPOD_WORKER_PREFERENCE) &&
               ((flg & IPOD_STOP_WORKERTASK) != IPOD_STOP_WORKERTASK))
            {
                if(gp_iPodWorking->rcvMsgInfo.startIDPS == FALSE)
                {
                    err = iPodSetIdentifyPreference();
                }

#ifndef IPOD_FAKE_AUTHENTICATE
                if(err == IPOD_OK)
                {
                    gp_iPodWorking->isAPIReady = TRUE;
                    iPodWorkerExeCB(gp_iPodWorking, IPOD_OK);
                }
#endif /* IPOD_FAKE_AUTHENTICATE */
                else if(err != IPOD_NOT_CONNECTED)
                {
                    iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
                }
            }
        }
        else
        {
            iPodOSSleep(IPOD_DELAY_100MS);
        }
        if ((flg > IPOD_OK) && ((flg & IPOD_STOP_WORKERTASK) == IPOD_STOP_WORKERTASK))
        {
            g_task_stop_flag |= IPOD_STOP_WORKERTASK;
        }
        (void)iPodOSSignalSemaphore(g_semWorkerId);
    }

    if (g_iPodAuthenticationBuffer != NULL)
    {
        free(g_iPodAuthenticationBuffer);
        g_iPodAuthenticationBuffer = NULL;
        g_iPodAuthenticationBufferLength = 0;
    }

    iPodOSExitTask(g_iPodUSBWorkerTask_id);     /* delete task */
}


void iPodWorkerStopAuthentication(IPOD_INSTANCE* iPodHndl)
{
    if ((g_task_stop_flag != 0) && (gp_iPodWorking == iPodHndl))
    {
        g_task_stop_flag |= IPOD_STOP_AUTHENTICATE;
    }
    (S32)iPodOSWaitSemaphore(g_semWorkerId, IPOD_TIME_OUT);
}

void iPodWorkerResume(void)
{
    (void)iPodOSSignalSemaphore(g_semWorkerId);
}

U8 iPodGetiPodDetected(void)
{
    return gp_iPodWorking->detected;
}


LOCAL S32 iPodRetSignatureData(U16  SignatureDataLength, const U8  *SignatureData)
{
    S32 rc = IPOD_OK;
    U8 Send_Buffer[IPOD_USB_REPORT_LEN] = {0};
    
    Send_Buffer[IPOD_POS0] = IPOD_START_OF_PACKET;
    Send_Buffer[IPOD_POS1] = (U8)(SignatureDataLength + 1 + 1);
    Send_Buffer[IPOD_POS2] = 0x00;
    Send_Buffer[IPOD_POS3] = IPOD_GENERAL_LINGO_RetDevAuthenticationSignature;

    if(SignatureData != NULL)
    {
        if((IPOD_USB_REPORT_LEN - IPOD_POS4) >= SignatureDataLength)
        {
            memcpy(&Send_Buffer[IPOD_POS4],
                   SignatureData,
                   SignatureDataLength);

            rc = iPodSendSignature(gp_iPodWorking, Send_Buffer, g_iPodCertTransID);
        }
        else
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter - SignatureDataLength = %d",SignatureDataLength);
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "SignatureData is NULL");
    }
    
    return rc;
}

/*!
 * \fn iPodWorkerWaitForEvent
 * \par INPUT PARAMETERS
 *  none
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_ERR_OK         Normal completion
 * \li \c \b IPOD_ERR_ID         Invalid ID number (semid is invalid or cannot be used)
 * \li \c \b IPOD_ERR_NOEXS      Object does not exist (the semaphore specified in semid
 *                               does not exist)
 * \li \c \b IPOD_ERR_PAR        Parameter error (timout . (12), cnt . 0)
 * \li \c \b IPOD_ERR_DLT        The object being waited for was deleted (the specified
 *                               semaphore was deleted while waiting)
 * \li \c \b IPOD_ERR_RLWAI      Wait state released (tk rel wai received in wait state)
 * \li \c \b IPOD_ERR_DISWAI     Wait released by wait disabled state
 * \li \c \b IPOD_ERR_TMOUT      Polling failed or timeout
 * \li \c \b IPOD_ERR_CTX        Context error (issued from task-independent portion or
 *                               in dispatch disabled state)
 * \li \c \b IPOD_TASK_STOPPED   All tasks were stopped
 * \par DESCRIPTION
 * The worker function waits for the semaphore.
 */
S32 iPodWorkerWaitForEvent(S32 timeout)
{
    S32 err             = IPOD_OK;
    U32 flg             = 0;

    /* Remove global valiable and use flag ptern for switch Audio output */
    /* Task Stop Flag recieve in same flagID. Feb/19/2010 */
    err = iPodOSWaitFlag( g_flgWorkerID, 
                          IPOD_WORKER_WAIT_FLAG, 
                          0,
                          &flg, 
                          timeout,
                          IPOD_FLG_ONE_WAY );

    if (err == IPOD_OK)
    {
        err = (S32)flg;
    }
    return err;
}

void iPodWorkerSetForEvent(IPOD_INSTANCE* iPodHndl, U32 flg)
{
    S32 rc = IPOD_OK;

    if (iPodHndl != NULL)
    {
        if (IPOD_WORKER_SIGNATURE != flg)
        {
            rc = (S32)iPodOSWaitSemaphore(g_semWorkerId, IPOD_TIME_OUT);
        }
        if (rc == IPOD_OK)
        {
            gp_iPodWorking = iPodHndl;
        }
    }

    if ((iPodHndl == NULL) || ((iPodHndl == gp_iPodWorking) && (rc == IPOD_OK)))
    {
        iPodOSSetFlag(g_flgWorkerID, flg, IPOD_FLG_ONE_WAY);
    }
}

void iPodWorkerSecondIdentify(void)
{
    iPodOSSetFlag(g_flgWorkerID, IPOD_WORKER_SECOND_IDENTIFY, IPOD_FLG_ONE_WAY);
}

void iPodWorkerExeCB(IPOD_INSTANCE* iPodHndl, S32 err)
{
    iPodExecuteCBUSBAttach(iPodHndl, err);
    g_task_stop_flag = 0;
}


void iPodWorkerIdentify(IPOD_INSTANCE* iPodHndl, BOOL isRequestidentify)
{
    g_task_stop_flag &= (U16)(~IPOD_STOP_AUTHENTICATE);
    if(isRequestidentify==TRUE)
    {
        iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_REQUESTIDENTIFY);
    }
    else
    {
        iPodWorkerSetForEvent(iPodHndl, IPOD_WORKER_IDENTIFY);
    }
}


/*!
 * \fn iPodCreateWorkerTask
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * \li \c \b Task ID     Completed successfully
 \li \c \b IPOD_ERROR  Processing failed
 * \par DESCRIPTION
 * This function creates the worker task.
 */
S32 iPodCreateWorkerTask(void)
{
    S32 rc  = IPOD_OK;
    IPOD_Cfg *dcInfo = iPodGetDevInfo();

    rc = iPodOSCreateFlag(dcInfo[IPOD_DC_FLG_WORKER].para.p_val, &g_flgWorkerID, IPOD_FLG_ONE_WAY);
    if ((rc == IPOD_OK) && (g_semWorkerId == 0))
    {
        rc = iPodOSCreateSemaphore(dcInfo[IPOD_DC_SEM_AUTHENTICATION].para.p_val, &g_semWorkerId);
        if(IPOD_ERROR == rc)
        {
            (void)iPodOSDeleteFlag(g_flgWorkerID, dcInfo[IPOD_DC_FLG_WORKER].para.p_val, IPOD_FLG_ONE_WAY);
        }
    }

    if (rc == IPOD_OK)
    {
        g_task_stop_flag = 0; /* initialize state */
        g_iPodUSBWorkerTask_id = iPodOSCreateTask(iPodWorkerTask,
                                                  dcInfo[IPOD_DC_AUDIOWORKER].para.p_val,
                                                  dcInfo[IPOD_DC_WORKER_PRIO].para.val,
                                                  (dcInfo[IPOD_DC_WORKER_STKSZ].para.val+dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val),
                                                  dcInfo[IPOD_DC_WORKER_LCID].para.val, NULL);

        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "IPOD_DC_MAX_PAYLOAD_SIZE: val = %d", dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val);

        if(iPodOSVerifyTaskID(g_iPodUSBWorkerTask_id) != IPOD_OK)
        {
            iPodOSClearTaskID(&g_iPodUSBWorkerTask_id);
            iPodDeleteWorkerTask();
            rc = IPOD_ERROR;        /* Error */
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "g_iPodUSBWorkerTask_id is zero");
        }
    }
    if (rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}


void iPodDeleteWorkerTask(void)
{
    S32 rc  = IPOD_OK;
    IPOD_Cfg *dcInfo = iPodGetDevInfo();
    
    if (IPOD_OK == iPodOSVerifyTaskID(g_iPodUSBWorkerTask_id))
    {
        iPodWorkerSetForEvent(NULL, IPOD_STOP_WORKERTASK);
        (void)iPodOSJoinTask(g_iPodUSBWorkerTask_id);
        iPodOSClearTaskID(&g_iPodUSBWorkerTask_id);
    }
    
    rc = iPodOSDeleteFlag(g_flgWorkerID, dcInfo[IPOD_DC_FLG_WORKER].para.p_val, IPOD_FLG_ONE_WAY);
    if (rc == IPOD_OK)
    {
        memset(&g_flgWorkerID, -1, sizeof(g_flgWorkerID));
    }

    rc = iPodOSDeleteSemaphore(g_semWorkerId, dcInfo[IPOD_DC_SEM_AUTHENTICATION].para.p_val);
    if(rc == IPOD_OK)
    {
        g_semWorkerId = 0;
    }
}


LOCAL S32 iPodIdentifyDeviceLingos(IPOD_INSTANCE* iPodHndl)
{
    S32 result = IPOD_ERROR;
    IPOD_TRANSPORT* transport = &gp_iPodWorking->transport;
    IPOD_Cfg* dcInfo = iPodGetDevInfo();
    U8 msg[] = {IPOD_IDENTIFY_DEVICE_LINGO_CMD};
    U8 msg2[] = {IPOD_IDENTIFY_GENERAL_ONLY_CMD};
    U8 count = 0;
    
    if(gp_iPodWorking->detected == TRUE)
    {    
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_IdentifyDeviceLingoes, (U8)IPOD_LINGO_GENERAL);
        result = iPodSendCommand(iPodHndl, msg2);
        if(result == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        }
    }
    else
    {
        result = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }

    if(result == IPOD_OK)
    {
        result = IPOD_ERR_TMOUT;
        for (count = 0; (count < 5) && (result == IPOD_ERR_TMOUT); count++)
        {
            /* This is waiting to start second Identify after finish the AccessoryInfo communication. */
            result = iPodWorkerWaitForEvent(IPOD_DELAY_1000MS);
            if(result > 0)
            {
                if((result & IPOD_WORKER_SECOND_IDENTIFY) == IPOD_WORKER_SECOND_IDENTIFY)
                {
                    (void)iPodRequestTransportMaxPacketSize(gp_iPodWorking->id, &transport->transportMaxPayloadSize);
                    result = iPodGetAllLingoVersion();
                    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodGetAllLingoVersion() returns : result = %d", result);
                }
                else
                {
                    iPodWorkerSetForEvent(iPodHndl, (U32)result);
                }
            }
            else if(result != IPOD_ERR_TMOUT)
            {
                result = IPOD_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodWorkerWaitForEvent() returns : result = %d", result);
            }
        }
    }
    
    if (result == IPOD_OK)
    {
        S32 i = 0;
        IPOD_OPTIONS* options = iPodHndl->transport.options;
        
        for (i = 0; i < IPOD_LINGO_MAX; i++)
        {
            if (options[i].rc == IPOD_OK)
            {
                if ((options[i].version.major_ver < options[i].supported.major_ver) || 
                    ((options[i].version.major_ver == options[i].supported.major_ver) && (options[i].version.minor_ver < options[i].supported.minor_ver)))
                {
                    result = IPOD_ERR_UNSUP_DEV;
                    IAP1_TRANSPORT_LOG(DLT_LOG_WARN, "Unsupported Version");
                }

                if ((result == IPOD_OK) && ((IPOD_LINGO)i == IPOD_LINGO_DIGITAL_AUDIO))
                {
                    /* Fixed bug of No support Digital Audio Lingo */
                    /* Add CR of DN (set the devconf value) */
                    if (iPodHndl->audioSwitch == IPOD_SWITCH_RESET)
                    {
                        if (dcInfo[IPOD_DC_AUDIO].para.val != 0)
                        {
                            msg[IPOD_POS6] |= (U8)IPOD_DIGITAL_LINGO_BITMASK;
                        }
                    }
                    else
                    {
                        if (iPodHndl->audioSwitch == IPOD_SWITCH_DIGITAL)
                        {
                            msg[IPOD_POS6] |= (U8)IPOD_DIGITAL_LINGO_BITMASK;
                        }
                    }
                }

                if ((result == IPOD_OK) && ((IPOD_LINGO)i == IPOD_LINGO_STORAGE))
                {
                    if (dcInfo[IPOD_DC_STORAGE].para.val != 0)
                    {
                        msg[IPOD_POS6] |= (U8)IPOD_STORAGE_LINGO_BITMASK;
                    }
                }
                if ((result == IPOD_OK) && ((IPOD_LINGO)i == IPOD_LINGO_DISPLAY_REMOTE))
                {
                    if (dcInfo[IPOD_DC_DISPLAY_REMOTE].para.val != 0)
                    {
                        msg[IPOD_POS7] |= (U8)IPOD_DIGITAL_REMOTE_LINGO_BITMASK;
                    }
                }
            }
        }

        if (options[IPOD_LINGO_GENERAL].rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_IdentifyDeviceLingoes, (U8)IPOD_LINGO_GENERAL);
            result = iPodSendCommand(iPodHndl, msg);
            IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns:result = %d",result);

            if (result == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodWaitAndGetResponseFixedSize() returns:result = %d",result);
            }
        }
    }
    if(result == IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "result = %d",result);
    }

    return result;
}


LOCAL S32 iPodGetAllLingoVersion(void)
{
    S32 rc = IPOD_ERROR;
    S32 i = 0;
    IPOD_OPTIONS* options = gp_iPodWorking->transport.options;

    for (i = 0; i < IPOD_LINGO_MAX; i++)
    {
        /* initialize with error to identify that the LINGO might not be supported by device */
        options[i].rc = IPOD_ERROR;
        if (options[i].supported.major_ver != 0)
        {
            options[i].rc = iPodGetLingoProtocolVersion_internal(gp_iPodWorking,
                                                                 (IPOD_LINGO)i, &options[i].version.major_ver, &options[i].version.minor_ver );
            if (options[i].rc == IPOD_OK)
            {
                rc = IPOD_OK;
            }
        }
    }

    return rc;

}

LOCAL void iPodDeleteLingoVersion(void)
{
    if(gp_iPodWorking != NULL)
    {
        memset(&gp_iPodWorking->transport.options->version, 0, sizeof(gp_iPodWorking->transport.options->version));
    }
}

LOCAL S32 iPodGetAllOptions(void)
{
    IPOD_Cfg *dcInfo = iPodGetDevInfo();
    S32 rc = IPOD_ERROR;

    if(dcInfo != NULL)
    {
        IPOD_OPTIONS* options = gp_iPodWorking->transport.options;

        if(dcInfo[IPOD_DC_GENERAL].para.val != 0)
        {
            options[IPOD_LINGO_GENERAL].rc = 
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_GENERAL, 
                                                    &options[IPOD_LINGO_GENERAL].iPodOptions);
            rc = options[IPOD_LINGO_GENERAL].rc;
        }
        
        if((dcInfo[IPOD_DC_EXTEND].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_EXTENDED_INTERFACE].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_EXTENDED_INTERFACE, 
                                                    &options[IPOD_LINGO_EXTENDED_INTERFACE].iPodOptions);
        }
        
        if((dcInfo[IPOD_DC_SIMPLE].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_SIMPLE_REMOTE].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_SIMPLE_REMOTE, 
                                                    &options[IPOD_LINGO_SIMPLE_REMOTE].iPodOptions);
        }
        
        if((dcInfo[IPOD_DC_AUDIO].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_DIGITAL_AUDIO].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_DIGITAL_AUDIO, 
                                                    &options[IPOD_LINGO_DIGITAL_AUDIO].iPodOptions);
        }
        
        if((dcInfo[IPOD_DC_STORAGE].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_STORAGE].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_STORAGE,
                                                    &options[IPOD_LINGO_STORAGE].iPodOptions);
        }
        
        if((dcInfo[IPOD_DC_LOCATION].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_LOCATION].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_LOCATION, 
                                                    &options[IPOD_LINGO_LOCATION].iPodOptions);
        }
        
        if((dcInfo[IPOD_DC_IPODOUT].para.val != 0)&&(IPOD_OK == options[IPOD_LINGO_GENERAL].rc))
        {
            options[IPOD_LINGO_OUT].rc =
                iPodGetiPodOptionsForLingo_internal(gp_iPodWorking, IPOD_LINGO_OUT, 
                                                    &options[IPOD_LINGO_OUT].iPodOptions);
        }
    }
    return rc;
}

LOCAL void iPodDeleteLingoOption(void)
{
    if(gp_iPodWorking != NULL)
    {
        memset(&gp_iPodWorking->transport.options->iPodOptions, 0, sizeof(gp_iPodWorking->transport.options->iPodOptions));
    }
}

LOCAL S32 iPodSetIdentifyPreference(void)
{
    S32 rc = IPOD_OK;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();
    U8 count = 0;
    U8 restore = IPOD_FID_PREFER_RESTORE;
    IPOD_PREFERENCE_SETTING_ID set = {(IPOD_VOUT_SETTING_SETTING_ID)0};

    if(dcInfo != NULL)
    {
        for(count = 0; (count < dcInfo[IPOD_DC_PREFERENCE].count) && (rc == IPOD_OK); count ++)
        {
            if(((S32 *)(VP)dcInfo[IPOD_DC_PREF_SET_FLG].para.p_val)[count] != 0)
            {
                switch((IPOD_PREFERENCE_CLASS_ID)count)
                {
                case IPOD_VOUT_SETTING :
                    set.videoOutSetting = (IPOD_VOUT_SETTING_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VSCREEN_CFG :
                    set.screenCfg = (IPOD_VSCREEN_CFG_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VSIG_FORMAT :
                    set.signalFormat = (IPOD_VSIG_FORMAT_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VLINE_OUT_USAGE :
                    /* Add CR of DN (set the devconf value) */
                    if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_LINE_OUT)
                    {
                        set.lineOut = IPOD_VLINE_OUT_USAGE_USED;
                    }
                    else if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_DIGITAL)
                    {
                        set.lineOut = IPOD_VLINE_OUT_USAGE_NOT_USED;
                    }
                    else
                    {
                        set.lineOut = (IPOD_VLINE_OUT_USAGE_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    }
                    break;
                    
                case IPOD_VOUT_CONNECT :
                    set.videoOutConnection = (IPOD_VOUT_CONNECT_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VCLOSED_CAP :
                    set.closedCaptioning = (IPOD_VCLOSED_CAP_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VASP_RATIO :
                    set.aspectRatio = (IPOD_VASP_RATIO_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VSUBTITLES :
                    set.subTitles = (IPOD_VSUBTITLES_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                case IPOD_VALT_AUD_CHANNEL :
                    set.audioChannel = (IPOD_VALT_AUD_CHANNEL_SETTING_ID)((U8)((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                    break;
                    
                default :
                    rc = IPOD_BAD_PARAMETER;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter - Unknown iPod preference ClassId");

                    break;
                }

                
                if(rc == IPOD_OK)
                {
                    rc = iPodSetiPodPreferences_internal(gp_iPodWorking, (IPOD_PREFERENCE_CLASS_ID)count, set, restore);
                    /* Older iPod model does not support some class ID */
                    /* No support Class ID pass */
                    if(rc == IPOD_BAD_PARAMETER)
                    {
                        rc = IPOD_OK;
                    }
                }
            }
        }
    }

    return rc;
}

LOCAL S32 iPodStartIDPS()
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_START_IDPS_CMD};
    
    if (gp_iPodWorking->detected == TRUE)
    {
        /* TID enable */
        iPodSetExpectedCmdId(gp_iPodWorking, (U16)IPOD_GENERAL_LINGO_StartIDPS, (U8)IPOD_LINGO_GENERAL);
    
        rc = iPodSendCommand(gp_iPodWorking, msg);
        if(rc == IPOD_OK)
        {
            memset(gp_iPodWorking->iAP1Buf, 0, gp_iPodWorking->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(gp_iPodWorking, gp_iPodWorking->iAP1Buf);
        }
    }
    else
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }

    return rc;
}

LOCAL S32 iPodEndIDPS(U8 endStatus)
{
    S32 rc = 0;
    U8 msg[] = {IPOD_END_IDPS_CMD};
    if(gp_iPodWorking->detected == TRUE)
    {
        iPodSetExpectedCmdId(gp_iPodWorking, (U16)IPOD_GENERAL_LINGO_IDPS_Status, (U8)IPOD_LINGO_GENERAL);
        msg[IPOD_POS4] = endStatus;
        rc = iPodSendCommand(gp_iPodWorking, msg);
        if(rc == IPOD_OK)
        {
            memset(gp_iPodWorking->iAP1Buf, 0, gp_iPodWorking->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(gp_iPodWorking, gp_iPodWorking->iAP1Buf);
            if(rc == IPOD_OK)
            {
                rc = (S32)gp_iPodWorking->iAP1Buf[0];
            }
        }
    }
    else
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }
    
    return rc;
}

LOCAL S32 iPodSetFIDTokenValues()
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 *tokenValue[IPOD_TOKEN_MAX_SIZE] = {0};
    U16 tokenLen[IPOD_TOKEN_MAX_SIZE] = {0};
    U16 tokenMaxLen       = IPOD_TOKEN_BASE_LEN;
    U8 tokenNum     = 0;
    IPOD_Cfg *dcInfo = iPodGetDevInfo();
    U8 i = 0;
    U16 total = IPOD_TOKEN_BASE_LEN;
    U8 largePac = 0;
    IPOD_OPTIONS* options = gp_iPodWorking->transport.options;

    S32 (*fidFunc[IPOD_TOKEN_MAX_SIZE])(const IPOD_Cfg *dcInfo, U8 **tokenValue, 
                                        U16 *tokenLen, U8 *tokenNum) = 
        { iPodSetFIDIdentify, iPodSetFIDAccCaps, iPodSetFIDAccInfo, iPodSetFIDPref,
          iPodSetFIDSDKProtocol, iPodSetFIDBundleSeed, iPodSetFIDMicrophoneCaps, NULL, NULL};

    fidFunc[IPOD_POS8] = iPodSetFIDMetadataToken;
    if((options[IPOD_LINGO_OUT].iPodOptions & 1) == 1)
    {
        fidFunc[IPOD_POS7] = iPodSetFIDScreenInfoToken;
    }
    
        for(i = 0; (i < IPOD_TOKEN_MAX_SIZE) && (rc == IPOD_OK); i++)
        {
            if(fidFunc[i] != NULL)
            {
                /* Set FID token */
                rc = (*fidFunc[i])(dcInfo, &tokenValue[i], &tokenLen[i], &tokenNum);
                if(rc == IPOD_OK)
                {
                    tokenMaxLen += tokenLen[i];
                }
            }
        }

    if(rc == IPOD_OK)
    {
        if(tokenMaxLen < IPOD_SHORT_WRITE_SIZE)
        {
            /* Small Packet Format */
            msg = (U8 *)calloc(tokenMaxLen, sizeof(U8));
        }
        else
        {
            /* Large Packet Format */
            tokenMaxLen += IPOD_SEND_LONG_MARKER;
            msg = (U8 *)calloc(tokenMaxLen, sizeof(U8));
        }
    }

    if(msg != NULL)
    {
        msg[IPOD_POS0] = IPOD_START_PACKET;
        if(tokenMaxLen < IPOD_SHORT_WRITE_SIZE)
        {
            /* Set Small Packet Format "length"*/
            msg[IPOD_POS1] = (U8)(tokenMaxLen - IPOD_START_LENGTH);
        }
        else
        {
            /* Set Large Packet Format "length marker", "length" */
            msg[IPOD_POS1] = 0x00;
            iPod_convert_to_big16(&msg[IPOD_POS2], (tokenMaxLen - (IPOD_START_LENGTH + IPOD_SEND_LONG_MARKER)));
            largePac += IPOD_SEND_LONG_MARKER;
            total += largePac;
        }
        msg[largePac + IPOD_POS2] = 0x00;
        msg[largePac + IPOD_POS3] = IPOD_GENERAL_LINGO_SetFIDTokenValues;
        msg[largePac + IPOD_POS4] = tokenNum;
        
        for(i = 0; i < IPOD_TOKEN_MAX_SIZE; i++)
        {
            if(tokenValue[i] != NULL)
            {
                if(tokenMaxLen >= (total + tokenLen[i]))
                {
                    /* Copy each Token */
                    memcpy(&msg[total], tokenValue[i], tokenLen[i]);
                    total += tokenLen[i];
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory for token");
                }
            }
        }
        if(gp_iPodWorking->detected == TRUE)
        {
            if(rc == IPOD_OK)
            {
                iPodSetExpectedCmdId(gp_iPodWorking, (U16)IPOD_GENERAL_LINGO_RetFIDTokenValueAcks, (U8)IPOD_LINGO_GENERAL);

                if(tokenMaxLen < IPOD_SHORT_WRITE_SIZE)
                {
                    /* Small Packet send command */
                    rc = iPodSendCommand(gp_iPodWorking, msg);
                }
                else
                {
                    /* Large Packet send command */
                    rc = iPodSendLongTelegram(gp_iPodWorking, msg);
                }
                if(rc == IPOD_OK)
                {
                    memset(gp_iPodWorking->iAP1Buf, 0, gp_iPodWorking->iAP1MaxPayloadSize);
                    rc = iPodFIDTokenResult(gp_iPodWorking->iAP1Buf, tokenValue, tokenLen);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }

    iPodFreeFIDTokenValue(tokenValue);
    if(msg != NULL)
    {
        free(msg);
    }
    return rc;
}

LOCAL S32 iPodFIDTokenResult(U8 *buf, U8 **tokenValue, U16 *tokenLen)
{
    S32 rc = IPOD_OK;
    U8 status = 0;
    U8 i = 0;
    U8 count = 0;
    U8 tokenOKcount = 0;
    U8 tokenId = 0;
    U8 tokenFlg = FALSE;
    U8 retNumToken = 0;
    U8 dataPos = 1;
    
    rc = iPodWaitAndGetResponseFixedSize(gp_iPodWorking, buf);
    if(rc == IPOD_OK)
    {
        retNumToken = buf[IPOD_POS0];
        for(count = 0; (count < retNumToken) && (retNumToken > 0); count++)
        {
            /* Search the same Token ID */
            for(i = 0, tokenFlg = FALSE; (i < IPOD_TOKEN_MAX_SIZE) && (tokenFlg == FALSE) && (rc == IPOD_OK); i++)
            {
                if(tokenValue[i] != NULL)
                {
                    /* Check Token ID */
                    if((tokenValue[i])[IPOD_POS1] == buf[dataPos + IPOD_POS1]
                       && ((tokenValue[i])[IPOD_POS2] == buf[dataPos + IPOD_POS2]))
                    {
                        /* Found the same TokenID*/
                        tokenId = i;
                        tokenFlg = TRUE;
                    }
                }
            }
            
            /* Same TokenID exists */
            if(tokenFlg != FALSE)
            {
                /* Set ack status */
                status = buf[dataPos + IPOD_POS3];
                
                /* TokenID not have type */
                if(buf[dataPos] == IPOD_TOKEN_NO_TYPE)
                {
                    /* Ack status is OK */
                    if(status == IPOD_OK)
                    {
                        /* Set the TokenID Length + length byte */
                        dataPos += buf[dataPos] + 1;
                        tokenOKcount++;
                    }
                    else if(status == IPOD_IDPS_TOKEN_VALUE_FAILED)
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod IDPS Token Value Failed");
                        /* SetFIDAccCaps Token ID is 1 */
                        if(tokenId == 1)
                        {
                            /* the reason for the error is probably the CancelCmd support */
                            gp_iPodWorking->rcvMsgInfo.supportCancelCmd = FALSE;
                        }
                    }
                    else if(status == IPOD_IDPS_TOKEN_NOT_SUPPORTED)
                    {
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod IDPS Token Not Supported");
                        dataPos += buf[dataPos] + 1;
                        tokenOKcount++;
                    }
                    else if(status == IPOD_IDPS_OPTIONAL_TOKEN_FAILED)
                    {
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod IDPS Optional Token Failed");
                        dataPos += buf[dataPos] + 1;
                        tokenOKcount++;
                    }
                    else
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - status = 0x%02x",status);
                    }
                }
                /* TokenID has few types */
                else if(buf[dataPos] == IPOD_TOKEN_HAS_TYPE)
                {
                    U16 j = 0;
                    U8 classFlg = FALSE;
                    /*Search the same type */
                    for(j = 0; (j < tokenLen[tokenId]) && (classFlg == FALSE) && (rc == IPOD_OK); j++)
                    {
                        /* Hit the type */
                        if((tokenValue[tokenId])[j + IPOD_POS3] == buf[dataPos + IPOD_POS4])
                        {
                            if(status == IPOD_OK)
                            {
                                /*Set the Class Length + length byte */
                                dataPos += buf[dataPos] + 1;
                                classFlg = TRUE;
                                tokenOKcount++;
                            }
                            else if(status == IPOD_IDPS_TOKEN_NOT_SUPPORTED)
                            {
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod IDPS Token Not Supported");
                                dataPos += buf[dataPos] + 1;
                                classFlg = TRUE;
                                tokenOKcount++;
                            }
                            else if(status == IPOD_IDPS_OPTIONAL_TOKEN_FAILED)
                            {
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod IDPS Optional Token Failed");
                                dataPos += buf[dataPos] + 1;
                                classFlg = TRUE;
                                tokenOKcount++;
                            }
                            else
                            {
                                rc = IPOD_ERROR;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - status = 0x%02x",status);
                            }
                        }
                        /* Serch next type */
                        j += (tokenValue[tokenId])[j];
                    }
                }
                else
                {
                    rc = IPOD_BAD_PARAMETER;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter buf[dataPos] = %d",buf[dataPos]);
                }
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter tokenFlg is FALSE");
            }
        }
    
        if((tokenOKcount != retNumToken) && (rc == IPOD_OK))
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - tokenOKcount = %d",tokenOKcount);
        }
    }
    
    return rc;
}


LOCAL S32 iPodSetFIDIdentify(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    U8 identifyLen = IPOD_FID_ID_LEN;
    U8 numLingoes = 0;
    U8 lingoes[IPOD_FID_OPTIONAL_LINGO] = {0};
    U32 devID = 0;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_ID_INFOBYTE};
    S8 pos = 0;
    
    /* Parameter check */
    if((dcInfo != NULL) && (*tokenValue == NULL) && (length != NULL) && (tokenNum != NULL))
    {
        /* General Lingo support */
        if(dcInfo[IPOD_DC_GENERAL].para.val != 0)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_GENERAL;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports General Lingo");
            numLingoes++;
        }
        
        /* Simple Lingo support */
        if(dcInfo[IPOD_DC_SIMPLE].para.val != 0)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_SIMPLE_REMOTE;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Simple Lingo");
            numLingoes++;
        }

        /* Display Remote Lingo support */
        if(dcInfo[IPOD_DC_DISPLAY_REMOTE].para.val != 0)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_DISPLAY_REMOTE;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Display Remote Lingo");
            numLingoes++;
        }        

        /* Extend Lingo support */
        if(dcInfo[IPOD_DC_EXTEND].para.val != 0)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_EXTENDED_INTERFACE;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Extended Interface Lingo");
            numLingoes++;
        }
        
        /* Digital Audio Lingo support */
        if((dcInfo[IPOD_DC_AUDIO].para.val != 0) && (gp_iPodWorking->audioSwitch == IPOD_SWITCH_RESET))
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_DIGITAL_AUDIO;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Digital Audio Lingo");
            numLingoes++;
        }
        /* Add CR of DN (set the devconf value) */
        else if(gp_iPodWorking->audioSwitch == IPOD_SWITCH_DIGITAL)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_DIGITAL_AUDIO;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Digital Audio Lingo");
            numLingoes++;
        }
        /* Storage Lingo support */
        if(dcInfo[IPOD_DC_STORAGE].para.val != 0)
        {
            lingoes[numLingoes] = (U8)IPOD_LINGO_STORAGE;
            IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Storage Lingo");
            numLingoes++;
        }

        /* iPodout Lingo support */
        if(dcInfo[IPOD_DC_IPODOUT].para.val != 0)
        {
            if(gp_iPodWorking->transport.options[IPOD_LINGO_OUT].rc == IPOD_OK)
            {
                lingoes[numLingoes] = (U8)IPOD_LINGO_OUT;
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports iPodout Lingo");
                numLingoes++;
            }
        }
        
        /* Location Lingo support */
        if(dcInfo[IPOD_DC_LOCATION].para.val != 0)
        {
            if(gp_iPodWorking->transport.options[IPOD_LINGO_LOCATION].rc == IPOD_OK)
            {
                lingoes[numLingoes] = (U8)IPOD_LINGO_LOCATION;
                IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Supports Location Lingo");
                numLingoes++;
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p *tokenValue = %p length = %p tokenNum = %p",
                                      dcInfo,*tokenValue,length,tokenNum);
    }
    
    if(rc == IPOD_OK)
    {
        rc = AuthenticationGetDeviceID(&devID);
    }
    if(rc == IPOD_OK)
    {
        *tokenValue = (U8 *)calloc((identifyLen + numLingoes), sizeof(U8));
        if(*tokenValue != NULL)
        {
            /* Set identify token */
            (*tokenValue)[IPOD_POS0] = identifyLen + numLingoes - IPOD_FID_LENGTH_BYTE;
            (*tokenValue)[IPOD_POS1] = infoByte[IPOD_POS0];
            (*tokenValue)[IPOD_POS2] = infoByte[IPOD_POS1];
            (*tokenValue)[IPOD_POS3] = numLingoes;
            memcpy(&((*tokenValue)[IPOD_POS4]), lingoes, numLingoes);
            (*tokenValue)[IPOD_POS4 + numLingoes] = 0x00;
            (*tokenValue)[IPOD_POS5 + numLingoes] = 0x00;
            (*tokenValue)[IPOD_POS6 + numLingoes] = 0x00;
            (*tokenValue)[IPOD_POS7 + numLingoes] = IPOD_FID_AUTHENTICATE_OPTION;
            iPod_convert_to_big32(&((*tokenValue)[IPOD_POS8 + numLingoes + pos]), devID);
            if(length != NULL)
            {
                /* Set Identify total length */
                *length = identifyLen + numLingoes;
            }
            if(tokenNum != NULL)
            {
                /* Increment FID total token */
                *tokenNum = *tokenNum + 1;
            }
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
        }
    }

    return rc;
}

LOCAL S32 iPodSetFIDAccCaps(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    U8 tokenLen = IPOD_FID_ACCCAPS_LEN;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_ACCCAPS_INFOBYTE};
    U64 accCaps = 0;
    U64 temp_acc_caps = 0;
    
    /* Parameter check */
    if((dcInfo != NULL) && (*tokenValue == NULL) && (length != NULL) && (tokenNum != NULL))
    {
        accCaps = ((((U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[0]) << IPOD_ACC_CAPS_BITSHIFT) | (U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[1]);
        *tokenValue = (U8 *)calloc(tokenLen, sizeof(U8));
        if(*tokenValue != NULL)
        {
            (*tokenValue)[IPOD_POS0] = tokenLen - IPOD_FID_LENGTH_BYTE;
            (*tokenValue)[IPOD_POS1] = infoByte[IPOD_POS0];
            (*tokenValue)[IPOD_POS2] = infoByte[IPOD_POS1];
            /* Add CR of DN (set the devconf value) */
            if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_LINE_OUT)
            {
                /* disable digital audio, enable line-out */
                temp_acc_caps = ((accCaps & IPOD_ACC_CAPS_NO_DIGITAL_AUDIO)|IPOD_ACC_CAPS_LINE_OUT);
            }
            else if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_DIGITAL)
            {
                /* disable line-out, enable digital audio */
                temp_acc_caps = ((accCaps & IPOD_ACC_CAPS_NO_LINE_OUT)|IPOD_ACC_CAPS_DIGITAL_AUDIO);
            }
            else
            {
                /* use devconf value, in case of RESET option */
                temp_acc_caps = accCaps;
            }
            
            if((gp_iPodWorking->transport.options[IPOD_LINGO_GENERAL].iPodOptions
                & IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP) != IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP)
            {
                temp_acc_caps &= IPOD_ACC_CAPS_IOS_SUPPORT;
            }

            /* Currently there is no possibility to check the support of multi-packet resp. CancelCmd.
             * Therefore, disable the CancelCmd support if first IDPS fails */
            if(((accCaps & IPOD_CAPS_CANCEL_SUPPORT) == IPOD_CAPS_CANCEL_SUPPORT)
                && (gp_iPodWorking->rcvMsgInfo.supportCancelCmd != TRUE))
            {
                /* disable support of CancelCmd */
                temp_acc_caps &= IPOD_ACC_CAPS_CANCEL_SUPPORT;
            }

            iPod_convert_to_big64(&((*tokenValue)[IPOD_POS3]), temp_acc_caps);
            *length = tokenLen;
            *tokenNum = *tokenNum + 1;
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p *tokenValue = %p length = %p tokenNum = %p",
                                      dcInfo,*tokenValue,length,tokenNum);
    }
    return rc;
}

LOCAL S32 iPodSetFIDAccInfo(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    U16 tokenLen[FID_ACC_MAX] = {0};
    U16 totalLen = 0;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_ACCINFO_INFOBYTE};
    U8 accInfoNum = 0;
    U8 count = 0;
    U16 size = IPOD_RESPONSE_BUF_SIZE;
    U16 tokenLenTotal = 0;
    void* tokenData[FID_ACC_MAX] = {NULL};

    /* Parameter Check */
    if((dcInfo != NULL) && (*tokenValue == NULL) && (length != NULL) && (tokenNum != NULL))
    {
        tokenData[FID_ACC_NAME] = (VP)dcInfo[IPOD_DC_ACCINFO_NAME].para.p_val;
        tokenData[FID_ACC_FW_VER] = (VP)dcInfo[IPOD_DC_FW_VER].para.p_val;
        tokenData[FID_ACC_HW_VER] = (VP)dcInfo[IPOD_DC_HW_VER].para.p_val;
        tokenData[FID_ACC_MAN] = (VP)dcInfo[IPOD_DC_MAN].para.p_val;
        tokenData[FID_ACC_MODEL] = (VP)dcInfo[IPOD_DC_MODEL].para.p_val;
        tokenData[FID_ACC_SERIAL] = (VP)dcInfo[IPOD_DC_SERIAL].para.p_val;
        if(dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val != 0)
        {
            size = (U16)dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val;
        }
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "IPOD_DC_MAX_PAYLOAD_SIZE: val = 0x%X", dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "size = 0x%X", size);
        tokenData[FID_ACC_INCOMING] = (VP)&size;
        tokenData[FID_ACC_STATUS] = (VP)&dcInfo[IPOD_DC_ACC_STATUS].para.val;
        tokenData[FID_ACC_RF] = (VP)&dcInfo[IPOD_DC_RF_CERTIFICATIONS].para.val;

        for(count = 0; count < (U8)FID_ACC_MAX; count ++)
        {
            if(((U8)((S32 *)(VP)dcInfo[IPOD_DC_ACCINFO_FLG].para.p_val)[count]) != 0)
            {
                if(tokenData[count] != NULL)
                {
                    /* AccInfo is string */
                    if((count == (U8)FID_ACC_NAME) || (count == (U8)FID_ACC_MAN) || 
                       (count == (U8)FID_ACC_MODEL) || (count == (U8)FID_ACC_SERIAL))
                    {
                        tokenLen[count] = (U16)strlen(tokenData[count]) + IPOD_FID_STR_NULL_BYTE;
                    }
                    /* Acc Info is incoming */
                    else if(count == (U8)FID_ACC_INCOMING)
                    {
                        tokenLen[count] = IPOD_FID_INCOMING_LEN;
                    }
                    /* Acc Info is number */
                    else  if((count == (U8)FID_ACC_FW_VER) || (count == (U8)FID_ACC_HW_VER))
                    {
                        tokenLen[count] = IPOD_FID_VERSION_LEN;
                    }
                    else if((count == (U8)FID_ACC_STATUS) || (count == FID_ACC_RF))
                    {
                        tokenLen[count] = sizeof(U32);
                    }
                    
                    /* Increment Accessory Info Token */
                    accInfoNum++;
                    totalLen += tokenLen[count];
                }
                
            }
        }
        (*tokenNum) += accInfoNum;
        totalLen += (accInfoNum * (IPOD_FID_ACCINFO_LEN + IPOD_FID_LENGTH_BYTE));
        *tokenValue = (U8 *)calloc(totalLen, sizeof(U8));
        tokenLenTotal = totalLen;
        if(*tokenValue != NULL)
        {
            totalLen = 0;
            for(count = 0; count < (U8)FID_ACC_MAX; count ++)
            {
                if(((U8)((S32 *)(VP)dcInfo[IPOD_DC_ACCINFO_FLG].para.p_val)[count]) != 0)
                {
                    if(tokenData[count] != NULL)
                    {
                        (*tokenValue)[totalLen] = (U8)(tokenLen[count] + IPOD_FID_ACCINFO_LEN);
                        (*tokenValue)[totalLen + IPOD_POS1] = infoByte[IPOD_POS0];
                        (*tokenValue)[totalLen + IPOD_POS2] = infoByte[IPOD_POS1];
                        (*tokenValue)[totalLen + IPOD_POS3] = count;
                        if(count == (U8)FID_ACC_INCOMING)
                        {
                            iPod_convert_to_big16(&((*tokenValue)[totalLen + IPOD_POS4]), (*(U16 *)tokenData[count]));
                        }
                        else  if((count == (U8)FID_ACC_FW_VER) || (count == (U8)FID_ACC_HW_VER))
                        {
                            (*tokenValue)[totalLen + IPOD_POS4] = (U8)((U32 *)tokenData[count])[IPOD_POS0];
                            (*tokenValue)[totalLen + IPOD_POS5] = (U8)((U32 *)tokenData[count])[IPOD_POS1];
                            (*tokenValue)[totalLen + IPOD_POS6] = (U8)((U32 *)tokenData[count])[IPOD_POS2];
                        }
                        else if((count == (U8)FID_ACC_STATUS) || (count == FID_ACC_RF))
                        {
                            iPod_convert_to_big32(&((*tokenValue)[totalLen + IPOD_POS4]), (*(U32 *)tokenData[count]));
                        }
                        else
                        {
                            if(totalLen + IPOD_POS4 + tokenLen[count] <= tokenLenTotal)
                            {
                                memcpy(&((*tokenValue)[totalLen + IPOD_POS4]), tokenData[count], tokenLen[count]);
                            }
                            else
                            {
                                rc = IPOD_BAD_PARAMETER;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter totalLen = %d tokenLen[count] = %d",totalLen,tokenLen[count]);
                            }
                        }
                        totalLen += (U16)(tokenLen[count] + IPOD_FID_ACCINFO_LEN + IPOD_FID_LENGTH_BYTE);
                    }
                }
            }
            *length = totalLen;
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod bad parameter - dcInfo = %p *tokenValue = %p length = %p tokenNum = %p",
                                      dcInfo,*tokenValue,length,tokenNum);
    }
    return rc;
}

LOCAL S32 iPodSetFIDPref(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    U16 count = 0;
    U8 tempTokenNum = 0;
    U8 prefLen = IPOD_FID_PREFER_LEN;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_PREFER_INFOBYTE};
    U8 totalRange = 0;

    /*Paramter check */
    if((dcInfo != NULL) && (*tokenValue == NULL) && (length != NULL) && (tokenNum != NULL))
    {
        /* Check enable preference */
        for(count = 0; count < dcInfo[IPOD_DC_PREFERENCE].count; count++)
        {
            if((U16)(((U32 *)(VP)dcInfo[IPOD_DC_PREF_SET_FLG].para.p_val)[count]) != 0)
            {
                tempTokenNum++;
            }
        }
        
        if(tempTokenNum != 0)
        {
            U8 maxLen = 0;
            maxLen = prefLen * tempTokenNum;
            *length = maxLen;
            *tokenValue = (U8 *)calloc(maxLen, sizeof(U8));
            if(*tokenValue != NULL)
            {
                for(count = 0; count < dcInfo[IPOD_DC_PREFERENCE].count; count ++)
                {
                    /* Check enable preference */
                    if((U8)(((S32 *)(VP)dcInfo[IPOD_DC_PREF_SET_FLG].para.p_val)[count]) != 0)
                    {
                        (*tokenValue)[totalRange + IPOD_POS0] = prefLen - IPOD_FID_LENGTH_BYTE;
                        (*tokenValue)[totalRange + IPOD_POS1] = infoByte[IPOD_POS0];
                        (*tokenValue)[totalRange + IPOD_POS2] = infoByte[IPOD_POS1];
                        (*tokenValue)[totalRange + IPOD_POS3] = (U8)count;
                        (*tokenValue)[totalRange + IPOD_POS4] = (U8)(((U32 *)(VP)dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPod Preference = 0x%02x",(U8)(dcInfo[IPOD_DC_PREFERENCE].para.p_val)[count]);
                        (*tokenValue)[totalRange + IPOD_POS5] = IPOD_FID_PREFER_RESTORE;
                        if((IPOD_PREFERENCE_CLASS_ID)count == IPOD_VLINE_OUT_USAGE) /* needs special handling for case of iPodSwitchAudio */
                        {
                            if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_LINE_OUT)
                            {
                                /* over-write conf. setting to enable line-out */
                                (*tokenValue)[totalRange + IPOD_POS4] = (U8)IPOD_VLINE_OUT_USAGE_USED;
                            }
                            else if(gp_iPodWorking->audioSwitch == (U8)IPOD_SWITCH_DIGITAL)
                            {
                                /* over-write conf. setting to disable line-out */
                                (*tokenValue)[totalRange + IPOD_POS4] = (U8)IPOD_VLINE_OUT_USAGE_NOT_USED;
                            }
                        }
                        totalRange += prefLen;
                    }
                }
            }

            *tokenNum += tempTokenNum;
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p *tokenValue = %p length = %p tokenNum = %p",
                                              dcInfo,*tokenValue,length,tokenNum);
    }
    return rc;
}

LOCAL S32 iPodSetFIDSDKProtocol(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    IMPORT IPOD_IOS_INSTANCE* gp_iPodiOSInst;
    S32 rc = IPOD_OK;
    U16 totalLen = 0;
    U16 totalRange = 0;
    U8 protocolLen[IPOD_CFG_STR_MAX] = {0};
    U16 i = 0;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_SDK_INFOBYTE};
    U8 protocolIndex = 1;
    U8 deviceID = 0;
    BOOL setApp = FALSE;
    IPOD_OPTIONS* options = gp_iPodWorking->transport.options;
    U64 accCaps = 0;
    
    /* Check the parameter */
    if((dcInfo != NULL) && (tokenValue != NULL) && (length != NULL) && (tokenNum != NULL))
    {
        accCaps = ((((U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[0]) << IPOD_ACC_CAPS_BITSHIFT) | (U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[1]);
        if ((accCaps & IPOD_CAPS_IOS_SUPPORT) != 0)
        {
            if(gp_iPodiOSInst != NULL)
            {
                for(i = 0; i < dcInfo[IPOD_DC_INST_COUNT].para.val; i++)
                {
                    if(strcmp((VP)gp_iPodWorking->name, (VP)gp_iPodiOSInst[i].name) == 0)
                    {
                        if ((options[IPOD_LINGO_GENERAL].iPodOptions & IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP) 
                            == IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP)
                        {
                            deviceID = (U8)i;
                            setApp = TRUE;
                        }
                    }
                }

                if(setApp != FALSE)
                {
                    /* Get the each length of devconf string */
                    for(i = 0; i < gp_iPodiOSInst[deviceID].numApps; i++)
                    {
                        protocolLen[i] = ((U8)strlen((VP)gp_iPodiOSInst[deviceID].appInfo[i].protocol) + IPOD_FID_STR_NULL_LEN);
                        totalLen += protocolLen[i] + IPOD_FID_SDK_LEN;
                    }

                    if(gp_iPodiOSInst[deviceID].numApps > 0)
                    {
                        if(*tokenValue == NULL)
                        {
                            *length += totalLen;
                            *tokenValue = (U8 *)calloc(totalLen, sizeof(U8));
                            if(*tokenValue != NULL)
                            {
                                for(i = 0; (i < gp_iPodiOSInst[deviceID].numApps) && (rc == IPOD_OK); i++)
                                {
                                    if(totalLen >= (totalRange + IPOD_POS4 + protocolLen[i]))
                                    {
                                        (*tokenValue)[totalRange + IPOD_POS0] = protocolLen[i] + IPOD_FID_SDK_LEN - 1;
                                        (*tokenValue)[totalRange + IPOD_POS1] = infoByte[IPOD_POS0];
                                        (*tokenValue)[totalRange + IPOD_POS2] = infoByte[IPOD_POS1];
                                        (*tokenValue)[totalRange + IPOD_POS3] = protocolIndex;
                                        memcpy(&(*tokenValue)[totalRange + IPOD_POS4], gp_iPodiOSInst[deviceID].appInfo[i].protocol, protocolLen[i]);
                                        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Protocol Index = %u Protocol String = %s",protocolIndex,gp_iPodiOSInst[deviceID].appInfo[i].protocol);
                                        totalRange += protocolLen[i] + IPOD_FID_SDK_LEN;
                                        protocolIndex++;
                                        *tokenNum = *tokenNum + 1;
                                    }
                                    else
                                    {
                                        rc = IPOD_ERR_NOMEM;
                                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory totalLen is less than required");
                                    }
                                }
                            }
                            else
                            {
                                rc = IPOD_ERR_NOMEM;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
                            }
                        }
                        else
                        {
                            rc = IPOD_BAD_PARAMETER;
                            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter *tokenValue = %p",*tokenValue);
                        }
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p tokenValue = %p length = %p tokenNum = %p",
                                              dcInfo,tokenValue,length,tokenNum);
    }
    
    return rc;
}

/* This is not used. It may have to inmplement in the future . */
LOCAL S32 iPodSetFIDBundleSeed(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    IMPORT IPOD_IOS_INSTANCE *gp_iPodiOSInst;
    S32 rc = IPOD_OK;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_BUNDLE_INFOBYTE};
    U16 totalLen = 0;
    U16 totalRange = 0;
    U16 i = 0;
    U8 bundleLen[IPOD_CFG_STR_MAX] = {0};
    U8 deviceID = 0;
    BOOL setApp = FALSE;
    IPOD_OPTIONS* options = gp_iPodWorking->transport.options;
    U64 accCaps = 0;
    
    /* Check the parameter */
    if((dcInfo != NULL) && (tokenValue != NULL) && (length != NULL) && (tokenNum != NULL))
    {
        accCaps = ((((U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[0]) << IPOD_ACC_CAPS_BITSHIFT) | (U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[1]);
        if ((accCaps & IPOD_CAPS_IOS_SUPPORT) != 0)
        {
            if(gp_iPodiOSInst != NULL)
            {
                for(i = 0; i < dcInfo[IPOD_DC_INST_COUNT].para.val; i++)
                {
                    if(strcmp((VP)gp_iPodWorking->name, (VP)gp_iPodiOSInst[i].name) == 0)
                    {
                        if ((options[IPOD_LINGO_GENERAL].iPodOptions & IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP) 
                            == IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP)
                        {
                            deviceID = (U8)i;
                            setApp = TRUE;
                        }
                    }
                }

                if(setApp != FALSE)
                {
                    for(i = 0; i < gp_iPodiOSInst[deviceID].numApps; i++)
                    {
                        bundleLen[i] = (U8)(strlen((VP)gp_iPodiOSInst[deviceID].appInfo[i].bundle) + IPOD_FID_STR_NULL_LEN);
                        totalLen += bundleLen[i] + IPOD_FID_BUNDLE_LEN;
                    }

                    if(gp_iPodiOSInst[deviceID].numApps > 0)
                    {
                        if(*tokenValue == NULL)
                        {
                            *length += totalLen;
                            *tokenValue = (U8 *)calloc(totalLen, sizeof(U8));
                            if(*tokenValue != NULL)
                            {
                                for(i = 0; (i < gp_iPodiOSInst[deviceID].numApps) && (rc == IPOD_OK); i++)
                                {
                                    if(totalLen >= (totalRange + IPOD_POS3 + bundleLen[i]))
                                    {
                                        (*tokenValue)[totalRange + IPOD_POS0] = bundleLen[i] + IPOD_FID_BUNDLE_LEN - 1;
                                        (*tokenValue)[totalRange + IPOD_POS1] = infoByte[IPOD_POS0];
                                        (*tokenValue)[totalRange + IPOD_POS2] = infoByte[IPOD_POS1];
                                        memcpy(&((*tokenValue)[totalRange + IPOD_POS3]), (VP)gp_iPodiOSInst[deviceID].appInfo[i].bundle, bundleLen[i]);
                                        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "BundleSeed ID = %s",gp_iPodiOSInst[deviceID].appInfo[i].bundle);
                                        totalRange += bundleLen[i] + IPOD_FID_BUNDLE_LEN;
                                        *tokenNum = *tokenNum + 1;
                                    }
                                    else
                                    {
                                        rc = IPOD_ERR_NOMEM;
                                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory totalLen is less than required");
                                    }
                                }
                            }
                            else
                            {
                                rc = IPOD_ERR_NOMEM;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
                            }
                        }
                        else
                        {
                            rc = IPOD_BAD_PARAMETER;
                            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter *tokenValue = %p",*tokenValue);
                        }
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p tokenValue = %p length = %p tokenNum = %p",
                                              dcInfo,tokenValue,length,tokenNum);
    }

    return rc;
}


LOCAL S32 iPodSetFIDMetadataToken(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    IMPORT IPOD_IOS_INSTANCE *gp_iPodiOSInst;
    S32 rc = IPOD_OK;
    U8 infoByte[IPOD_FID_INFO_BYTE_LEN] = {IPOD_FID_METADATA_TOKEN};
    U8 len = IPOD_FID_METADATA_LEN;
    U8 totalLen = 0;
    U8 count = 0;
    U8 totalRange = 0;
    U8 protocolIndex = 1;
    U16 metaSize = 0;
    U8 deviceID = 0;
    U8 setApp = FALSE;
    IPOD_OPTIONS* options = gp_iPodWorking->transport.options;
    U64 accCaps = 0;

    if((dcInfo != NULL) && (tokenValue != NULL) && (length != NULL) && (tokenNum != NULL))
    {
        accCaps = ((((U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[0]) << IPOD_ACC_CAPS_BITSHIFT) | (U64)(U32)((S32 *)(VP)dcInfo[IPOD_DC_ACC_CAPS_BIT].para.p_val)[1]);
        if ((accCaps & IPOD_CAPS_IOS_SUPPORT) != 0)
        {
            if(*tokenValue == NULL)
            {
                if(gp_iPodiOSInst != NULL)
                {
                    for(count = 0; count < dcInfo[IPOD_DC_INST_COUNT].para.val; count++)
                    {
                        if(strcmp((VP)gp_iPodWorking->name, (VP)gp_iPodiOSInst[count].name) == 0)
                        {
                            if ((options[IPOD_LINGO_GENERAL].iPodOptions & 
                                 IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP) 
                                == IPOD_GENERAL_LINGO_OPTIONS_BITMASK_IOS_APP)
                            {
                            deviceID = count;
                            metaSize = len * gp_iPodiOSInst[deviceID].numApps;
                            setApp = TRUE;
                            }
                        }
                    }

                    if(setApp != FALSE)
                    {
                        if(gp_iPodiOSInst[deviceID].numApps > 0)
                        {
                            *tokenValue = (U8 *)calloc(metaSize, sizeof(U8));
                            if(*tokenValue != NULL)
                            {
                                for(count = 0; count < gp_iPodiOSInst[deviceID].numApps; count++)
                                {
                                    (*tokenValue)[totalRange + IPOD_POS0] = IPOD_FID_METADATA_LEN - 1;
                                    (*tokenValue)[totalRange + IPOD_POS1] = infoByte[IPOD_POS0];
                                    (*tokenValue)[totalRange + IPOD_POS2] = infoByte[IPOD_POS1];
                                    (*tokenValue)[totalRange + IPOD_POS3] = protocolIndex;
                                    (*tokenValue)[totalRange + IPOD_POS4] = gp_iPodiOSInst[deviceID].appInfo[count].metaData;
                                    IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, " protocal index = %u metaData = 0x%02x",protocolIndex,gp_iPodiOSInst[deviceID].appInfo[count].metaData);
                                    totalLen += len;
                                    totalRange += len;
                                    protocolIndex++;
                                    *tokenNum = *tokenNum + 1;
                                }
                                *length += totalLen;
                            }
                            else
                            {
                                rc = IPOD_ERR_NOMEM;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
                            }
                        }
                    }
                }
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter * tokenValue = %p",tokenValue);
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p tokenValue = %p length = %p tokenNum = %p",
                                              dcInfo,tokenValue,length,tokenNum);
    }
    
    return rc;
}

LOCAL S32 iPodSetFIDScreenInfoToken(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    U8 infoByte[] = {IPOD_FID_SCREEN_TOKEN};
    if((dcInfo != NULL) && (tokenValue != NULL) && (length != NULL) && (tokenNum != NULL))
    {
        if(*tokenValue == NULL)
        {
            *tokenValue = (U8 *)calloc(IPOD_FID_SCREEN_LEN + 1, sizeof(U8));
            if(*tokenValue != NULL)
            {
                 IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Width(inch) = %d Height(inch) = %d Totalwidth(pxl) = %d Totalheight(pxl) = %d",
                                     dcInfo[IPOD_DC_TOTAL_WIDTH_INCHES].para.val,dcInfo[IPOD_DC_TOTAL_HEIGHT_INCHES].para.val,
                                     dcInfo[IPOD_DC_TOTAL_WIDTH_PIXELS].para.val,dcInfo[IPOD_DC_TOTAL_HEIGHT_PIXELS].para.val);
                 IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "Width(pix) = %d Height(pix) = %d Feature mask = %d Gamma Value = %d",
                                                      dcInfo[IPOD_DC_TOTAL_WIDTH_PIXELS].para.val,dcInfo[IPOD_DC_TOTAL_HEIGHT_PIXELS].para.val,
                                                      dcInfo[IPOD_DC_TOTAL_FEATURES_MASK].para.val,dcInfo[IPOD_DC_TOTAL_GAMMA_VALUE].para.val);
                (*tokenValue)[IPOD_POS0] = IPOD_FID_SCREEN_LEN;
                (*tokenValue)[IPOD_POS1] = infoByte[IPOD_POS0];
                (*tokenValue)[IPOD_POS2] = infoByte[IPOD_POS1];
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS3], (U16)dcInfo[IPOD_DC_TOTAL_WIDTH_INCHES].para.val);
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS5], (U16)dcInfo[IPOD_DC_TOTAL_HEIGHT_INCHES].para.val);
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS7], (U16)dcInfo[IPOD_DC_TOTAL_WIDTH_PIXELS].para.val);
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS9], (U16)dcInfo[IPOD_DC_TOTAL_HEIGHT_PIXELS].para.val);
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS11], (U16)dcInfo[IPOD_DC_WIDTH_PIXELS].para.val);
                iPod_convert_to_big16(&(*tokenValue)[IPOD_POS13], (U16)dcInfo[IPOD_DC_HEIGHT_PIXELS].para.val);
                (*tokenValue)[IPOD_POS15] = (U8)dcInfo[IPOD_DC_TOTAL_FEATURES_MASK].para.val;
                (*tokenValue)[IPOD_POS16] = (U8)dcInfo[IPOD_DC_TOTAL_GAMMA_VALUE].para.val;
                *length += IPOD_FID_SCREEN_LEN + 1;
                *tokenNum = *tokenNum + 1;
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter *tokenValue = %p",*tokenValue);
            }
        }
        else
        {
            rc = IPOD_ERR_NOMEM;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory *tokenValue is NULL");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter - dcInfo = %p tokenValue = %p length = %p tokenNum = %p",
                                              dcInfo,tokenValue,length,tokenNum);
    }
    
    return rc;
}

/* This is not used. It may have to inmplement in the future . */
LOCAL S32 iPodSetFIDMicrophoneCaps(const IPOD_Cfg *dcInfo, U8 **tokenValue, U16 *length, U8 *tokenNum)
{
    S32 rc = IPOD_OK;
    dcInfo = dcInfo;
    tokenValue = tokenValue;
    length = length;
    tokenNum = tokenNum;

    return rc;
}

LOCAL void iPodFreeFIDTokenValue(U8 **tokenValue)
{
    U8 count = 0;

    for(count = 0; count < IPOD_TOKEN_MAX_SIZE; count++)
    {
        /* Fixed buf of CPU exception */
        if(tokenValue[count] != NULL)
        {
            free(tokenValue[count]);
        }
    }
}

LOCAL S32 iPodSendAuthenticationCertificate(U8 MajorVersion, U8 MinorVersion, U16  CertLength, const U8  *CertData)
{
    S32 rc = IPOD_OK;
    U8 Send_Buffer[IPOD_USB_REPORT_LEN] =  {0};
    U8 MaxSection       =   0;
    U8 SectionCount     =   0;
    U16 PacketLength    =   0;

    if (CertData != NULL)
    {
        MaxSection = (U8)(CertLength / IPOD_USB_REPORT_MSG_LEN);

        if ((CertLength % IPOD_USB_REPORT_MSG_LEN) == 0)
        {
            MaxSection--;
        }

        for (SectionCount = 0; (SectionCount <= MaxSection) && (rc == IPOD_OK); SectionCount++)
        {
            memset(Send_Buffer, 0, IPOD_USB_REPORT_LEN);

            Send_Buffer[IPOD_POS0] = IPOD_START_OF_PACKET;
            Send_Buffer[IPOD_POS2] = IPOD_GENERAL_LINGO;
            Send_Buffer[IPOD_POS3] = IPOD_GENERAL_LINGO_RetDevAuthenticationInfo;
            Send_Buffer[IPOD_POS4] = MajorVersion;
            Send_Buffer[IPOD_POS5] = MinorVersion;
            Send_Buffer[IPOD_POS7] = MaxSection;

            if (SectionCount < MaxSection)
            {
                PacketLength = IPOD_USB_REPORT_MSG_LEN;
                iPodSetExpectedCmdId(gp_iPodWorking, (U16)IPOD_GENERAL_LINGO_RetDevAuthenticationInfo, (U8)IPOD_LINGO_GENERAL);
            }
            else
            {
                PacketLength = CertLength - (SectionCount * IPOD_USB_REPORT_MSG_LEN);
                iPodSetExpectedCmdId(gp_iPodWorking, (U16)IPOD_GENERAL_LINGO_AckDevAuthenticationInfo, (U8)IPOD_LINGO_GENERAL);
            }

            Send_Buffer[IPOD_POS1] = (U8)(PacketLength + IPOD_CMD_SEND_CERT_HEAD_LEN);
            Send_Buffer[IPOD_POS6] = SectionCount;

            if(IPOD_USB_REPORT_LEN >= (IPOD_POS8 + PacketLength))
            {
                memcpy(&Send_Buffer[IPOD_POS8],
                       &CertData[SectionCount * IPOD_USB_REPORT_MSG_LEN],
                       PacketLength);
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory USB_REPORT_LEN is less than required");
            }

            /* JIRA[SWGII-4802] */
            /* RetDevAuthentication must wait an ack. */
            if(rc == IPOD_OK)
            {
                rc = iPodSendCommandCertificate(gp_iPodWorking, Send_Buffer);
            }
            
            if (rc == IPOD_OK)
            {
                memset(gp_iPodWorking->iAP1Buf, 0, gp_iPodWorking->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(gp_iPodWorking, gp_iPodWorking->iAP1Buf);
            }

            /* check if final autentication ack is pass */
            if ((SectionCount >= MaxSection) && (rc == IPOD_OK) && (gp_iPodWorking->iAP1Buf[IPOD_POS0] != 0x00))
            {
                iPodWorkerExeCB(gp_iPodWorking, IPOD_ERR_AUTHENTICATION);
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "certData is NULL");
    }
    
    return rc;
}
