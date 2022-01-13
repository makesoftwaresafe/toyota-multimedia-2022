#include "iap2_load_test_filetransfer_performance.h"

IMPORT iap2TestAppleDevice_t g_iap2TestDevice;

#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE

typedef struct iAP2FileXferPerform
{
    U8  iAP2FileTransferID;
    U32 iAP2StartTimeMs;
    struct iAP2FileXferPerform* next;
}iAP2FileXferPerf;

iAP2FileXferPerf* iAP2FileXferPerformance = NULL;

LOCAL S32 iap2IdentifyFileTransferIDPresent(iAP2FileXferPerf* iAP2FileXfer, U8 iAP2FileTransferID)
{
    S32 rc = IAP2_OK;

    while(iAP2FileXfer != NULL)
    {
        if(iAP2FileXfer->iAP2FileTransferID == iAP2FileTransferID)
        {
            rc = IAP2_CTL_ERROR;
            printf("\nFileTransferID already present enter a different number");
            break;
        }
        else
        {
            iAP2FileXfer = iAP2FileXfer->next;
        }
    }

    return rc;
}

LOCAL S32 iap2AllocateAndAppendFileTransferID(iAP2FileXferPerf** iAP2FileXfer, U8 iAP2FileTransferID)
{
    S32 rc = IAP2_OK;

    *iAP2FileXfer = calloc(1, sizeof(iAP2FileXferPerf) );
    if(*iAP2FileXfer == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        (*iAP2FileXfer)->iAP2FileTransferID = iAP2FileTransferID;
        (*iAP2FileXfer)->iAP2StartTimeMs = iap2CurrTimeMs();
        printf("\n\nFileTransferID = %d, StartTime (ms) = %d\n\n", iAP2FileTransferID, (*iAP2FileXfer)->iAP2StartTimeMs);
    }

    return rc;
}

LOCAL S32 iap2AppendFileTransferID(iAP2FileXferPerf** iAP2FileXfer, U8 iAP2FileTransferID)
{
    S32 rc = IAP2_OK;
    iAP2FileXferPerf* iAP2FileXferWr = *iAP2FileXfer;

    while(iAP2FileXferWr->next != NULL)
    {
        iAP2FileXferWr = iAP2FileXferWr->next;
    }

    iAP2FileXferWr->next = calloc(1, sizeof(iAP2FileXferPerf) );
    if(iAP2FileXferWr->next == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        iAP2FileXferWr->next->iAP2FileTransferID = iAP2FileTransferID;
        iAP2FileXferWr->next->iAP2StartTimeMs = iap2CurrTimeMs();
        printf("\n\nFileTransferID = %d, StartTime (ms) = %d\n\n", iAP2FileTransferID, iAP2FileXferWr->next->iAP2StartTimeMs);
    }

    return rc;
}

LOCAL S32 iap2InsertFileTransferID(iAP2FileXferPerf** iAP2FileXfer, U8 iAP2FileTransferID)
{
    S32 rc = IAP2_OK;

    if(iAP2FileXfer == NULL)
    {
        rc = IAP2_CTL_ERROR;
    }
    else if(*iAP2FileXfer == NULL)
    {
        rc = iap2AllocateAndAppendFileTransferID(iAP2FileXfer, iAP2FileTransferID);
    }
    else
    {
        rc = iap2IdentifyFileTransferIDPresent(*iAP2FileXfer, iAP2FileTransferID);
        if(rc == IAP2_OK)
        {
            rc = iap2AppendFileTransferID(iAP2FileXfer, iAP2FileTransferID);
        }
    }

    return rc;
}

LOCAL S32 iap2DeleteFileTransferID(iAP2FileXferPerf** iAP2FileXfer, U8 iAP2FileTransferID)
{
    S32 rc = IAP2_CTL_ERROR;

    if(iAP2FileXfer != NULL)
    {
        if(*iAP2FileXfer != NULL)
        {
            iAP2FileXferPerf* iAP2FileXferToFree;
            iAP2FileXferPerf* iAP2PrevFileXfer = *iAP2FileXfer;

            if( (*iAP2FileXfer)->iAP2FileTransferID == iAP2FileTransferID)
            {
                iAP2FileXferToFree = *iAP2FileXfer;
                *iAP2FileXfer = (*iAP2FileXfer)->next;
                free(iAP2FileXferToFree);
                rc = IAP2_OK;
            }
            else
            {
                iAP2FileXferToFree = (*iAP2FileXfer)->next;
                while(iAP2FileXferToFree != NULL)
                {
                    if(iAP2FileXferToFree->iAP2FileTransferID == iAP2FileTransferID)
                    {
                        iAP2PrevFileXfer->next = iAP2FileXferToFree->next;
                        free(iAP2FileXferToFree);
                        rc = IAP2_OK;
                        break;
                    }
                    else
                    {
                        iAP2PrevFileXfer = iAP2FileXferToFree;
                        iAP2FileXferToFree = iAP2FileXferToFree->next;
                    }
                }
            }
        }
    }
    else
    {
        printf("\nFileTransfer is NULL");
    }

    return rc;
}

#endif /* IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE */

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if(iAP2FileXferSession->iAP2FileXferRxLen > 0)
        {
            printf("\t   artwork available. FileID: %d  FileSize: %llu  \n",
                    iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

            g_iap2TestDevice.coverArtBuf.Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);
            g_iap2TestDevice.coverArtBuf.CurPos = g_iap2TestDevice.coverArtBuf.Buffer;
            if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
            {
                g_iap2TestDevice.coverArtBuf.CurReceived = 0;
                g_iap2TestDevice.coverArtBuf.FileID = iAP2FileXferSession->iAP2FileTransferID;
                g_iap2TestDevice.coverArtBuf.FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
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

#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE
        rc = iap2InsertFileTransferID(&iAP2FileXferPerformance, iAP2FileXferSession->iAP2FileTransferID);
#endif

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

    printf("\t %u ms %p  iap2FileTransferDataRcvd_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_iap2TestDevice.coverArtBuf.CurPos != NULL)
            && (g_iap2TestDevice.coverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            memcpy(g_iap2TestDevice.coverArtBuf.CurPos, iAP2FileXferSession->iAP2FileXferRxBuf, iAP2FileXferSession->iAP2FileXferRxLen);
            g_iap2TestDevice.coverArtBuf.CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
            g_iap2TestDevice.coverArtBuf.CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;

            iap2SetTestState(FILE_TRANSFER_DATA_RECV, TRUE);
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
    char fileName[MAX_STRING_LEN];

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_iap2TestDevice.coverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID)
            && (g_iap2TestDevice.coverArtBuf.CurReceived == g_iap2TestDevice.coverArtBuf.FileSize) )
        {

#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE
            iAP2FileXferPerf* iAP2FileXfer = iAP2FileXferPerformance;

            while(iAP2FileXfer != NULL)
            {
                if(iAP2FileXfer->iAP2FileTransferID == iAP2FileXferSession->iAP2FileTransferID)
                {
                    rc = IAP2_CTL_ERROR;
                    printf("\n\nFileTransferID = %d, FileSize (bytes) = %lld, Time Taken (msec) = %d\n\n", iAP2FileXfer->iAP2FileTransferID, g_iap2TestDevice.coverArtBuf.FileSize, (iap2CurrTimeMs() - iAP2FileXfer->iAP2StartTimeMs));
                    break;
                }
                else
                {
                    iAP2FileXfer = iAP2FileXfer->next;
                }
            }
            rc = iap2DeleteFileTransferID(&iAP2FileXferPerformance, iAP2FileXferSession->iAP2FileTransferID);
#endif

            memset(&fileName[0], 0, (sizeof(fileName)));
            sprintf(&fileName[0], "%s%d%s", "/tmp/CoverArt", iAP2FileXferSession->iAP2FileTransferID, ".jpg");

            fp = fopen(&fileName[0], "w");
            if(fp != NULL)
            {
                fwrite(g_iap2TestDevice.coverArtBuf.Buffer, 1, g_iap2TestDevice.coverArtBuf.FileSize, fp);
                fclose(fp);

                printf("\t   File Transfer Success!  Please check %s \n", &fileName[0]);
            }

            if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
            {
                free(g_iap2TestDevice.coverArtBuf.Buffer);
                g_iap2TestDevice.coverArtBuf.Buffer = NULL;
                g_iap2TestDevice.coverArtBuf.CurPos = NULL;
                memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
            }

            iap2SetTestState(FILE_TRANSFER_SUCCESS_RECV, TRUE);
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

    printf("\t %u ms %p  iap2FileTransferFailure_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer failed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);

#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE
        rc = iap2DeleteFileTransferID(&iAP2FileXferPerformance, iAP2FileXferSession->iAP2FileTransferID);
#endif

        if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
        {
            free(g_iap2TestDevice.coverArtBuf.Buffer);
            g_iap2TestDevice.coverArtBuf.Buffer = NULL;
            g_iap2TestDevice.coverArtBuf.CurPos = NULL;
            memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_FAILED_RECV, TRUE);
    return rc;
}

S32 iap2FileTransferCancel_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferCancel_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer canceled.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
        {
            free(g_iap2TestDevice.coverArtBuf.Buffer);
            g_iap2TestDevice.coverArtBuf.Buffer = NULL;
            g_iap2TestDevice.coverArtBuf.CurPos = NULL;
            memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
        }

#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE
        if(rc == IAP2_OK)
        {
            rc = iap2DeleteFileTransferID(&iAP2FileXferPerformance, iAP2FileXferSession->iAP2FileTransferID);
        }
#endif

    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_CANCELED_RECV, TRUE);
    return rc;
}

S32 iap2FileTransferPause_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferPause_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer paused.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_PAUSED_RECV, TRUE);
    return rc;
}

S32 iap2FileTransferResume_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferResume_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer resumed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_RESUMED_RECV, TRUE);
    return rc;
}

