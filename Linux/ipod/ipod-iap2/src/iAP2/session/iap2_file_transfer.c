#include "iap2_init_private.h"
#include "iap2_file_transfer.h"

#include "iap2_dlt_log.h"
#include "iAP2FileTransfer.h"
#include "iAP2LinkRunLoop.h"
#include <iap2_service_messages.h>

#define IAP2_FILE_XFER_MAX_TRANSFERS    128
#define IAP2_FILE_XFER_ERROR            128

LOCAL void iAP2FileXferSessionDelete(iAP2FileTransferSession_t** p_iAP2FileXferSession, const U8 FileTransferID);
LOCAL void iAP2FileXferSessionFree(iAP2FileTransferSession_t* iAP2FileXferSession);
S32 iAP2FileXferStartOk(iAP2Device_st* this_iAP2Device,U8 ftid, BOOL startOK );
EXPORT void iAP2CleanupFileTransferSession(iAP2Device_t* iAP2Device);
LOCAL void iAP2FileXferInitializeHandle(U8 fileTransferId, iAP2FileTransfer_t* iAP2FileXrHdl, iAP2Device_st* this_iAP2Device);

inline static void iAP2FreePointer(void** iAP2PointerToFree)
{
    if(iAP2PointerToFree != NULL)
    {
        free(*iAP2PointerToFree);
        *iAP2PointerToFree = NULL;
    }
}

/**
 * \addtogroup FileTransferSessionAPIs
 * @{
 */

/***************************************************************************//**
 * Checks whether the File Transfer ID is already present in the File transfer
 * session or not
 *
 * \param  iAP2FileTransfer_list  File Transfer Session List
 * \param  iAP2FileTransferID     File Transfer ID
 *
 * \return IAP2_FILE_XFER_ID_NOT_PRESENT  File transfer ID is not present in the list
 * \return IAP2_OK                        File transfer ID is present in the list
 * \see
 * \note
 *******************************************************************************/
LOCAL S32 iAP2CheckIfFileXferIDPresent(iAP2FileTransferSession_t** iAP2FileTransfer_list, const U8 iAP2FileTransferID)
{
    S32 rc = IAP2_FILE_XFER_ID_NOT_PRESENT;

    while(*iAP2FileTransfer_list != NULL)
    {
        if( (*iAP2FileTransfer_list)->iAP2FileTransferID == iAP2FileTransferID)
        {
            rc = IAP2_OK;
            break;
        }

        *iAP2FileTransfer_list = (*iAP2FileTransfer_list)->NextTransfer;
    }

    return rc;
}

/* File transfer Callbacks */


/***************************************************************************//**
 * Checks if file transfer succeeded or failed
 *
 * \param this_iAP2Device     Device Structure
 * \param iAP2FileXferSession File Transfer Session for which file data is received
 *
 * \return IAP2_OK    On Successful completion of sending success or failure

 * \see
 * \note
 *******************************************************************************/
LOCAL S32 iAP2FileXferSuccessorFail(iAP2Device_st* this_iAP2Device, iAP2FileTransferSession_t*  iAP2FileXferSession)
{
    S32 rc = IAP2_OK;
    struct iAP2FileTransfer_st* fileXfer = NULL;

    if(iAP2FileXferSession != NULL)
    {
        fileXfer = iAP2FileXferSession->iAP2FileXferHdl;

        if(fileXfer != NULL)
        {
            if(fileXfer->totalSize == fileXfer->sentSize)
            {
                /* Send success datagram to Device */
                iAP2FileTransferSuccess(fileXfer);

                if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSuccess_cb != NULL)
                {
                    (*this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSuccess_cb)(this_iAP2Device,
                                                                                             iAP2FileXferSession,
                                                                                             this_iAP2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"File Transfer SUCCESS callback not registered by App. DevID:%p",this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }

            }
            else
            {
                /* Send failure datagram to Device */
                iAP2FileTransferFailure(fileXfer);
                IAP2SESSIONDLTLOG(DLT_LOG_INFO,"File transfer failed. FileID:%d DevID:%p",
                                  iAP2FileXferSession->iAP2FileTransferID, this_iAP2Device);

                if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb != NULL)
                {
                    (*this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb)(this_iAP2Device,
                                                                                             iAP2FileXferSession,
                                                                                             this_iAP2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"File Transfer FAILURE callback not registered by App. DevID:%p",this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
        }
    }

    return rc;
}

/***************************************************************************//**
 * This callback function is called when last datagram is received from device
 * and it send success or failure datagram to device by checking received data
 * length against total data length
 *
 * \param fileXfer  File Transfer Structure which contains data buffer, buffer id,
 *                  session id, total size of the file and so on
 * \param userInfo  additional information
 *
 * \return TRUE     On Successful completion of sending success or failure
 * \return FALSE    Error in file transfer state
 * \see
 * \note
 *******************************************************************************/
BOOL iAP2FiletransferReceive_CB(struct iAP2FileTransfer_st* fileXfer, void* userInfo)
{
    BOOL ret = TRUE;
    S32 rc = IAP2_OK;
    iAP2Device_st* iap2Device = NULL;
    iAP2FileTransferSession_t* iAP2FileXferSession = NULL;

    iap2Device = (iAP2Device_st*)userInfo;

    if( (iap2Device != NULL) && (fileXfer != NULL) )
    {
        iAP2FileXferSession = iap2Device->iAP2FileTransfer_list;

        /*modifies the iAP2FileXferSession to point to file transfer object matching fileTransferID*/
        rc = iAP2CheckIfFileXferIDPresent(&iAP2FileXferSession, fileXfer->bufferID);
        if(rc == IAP2_OK)
        {
            if((fileXfer->state == kiAP2FileTransferStateRecv) || (fileXfer->state == kiAP2FileTransferStateFinishRecv))
            {
                iAP2FileXferSession->iAP2FileXferRxBuf = fileXfer->pBuffer;
                iAP2FileXferSession->iAP2FileXferRxLen = fileXfer->buffSize;

                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataRcvd_cb != NULL)
                {
                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataRcvd_cb)(iap2Device,
                                                                                         iAP2FileXferSession,
                                                                                         iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"File Transfer RECEIVE callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }

                if(fileXfer->state == kiAP2FileTransferStateFinishRecv)
                {
                    rc = iAP2FileXferSuccessorFail(iap2Device, iAP2FileXferSession);
                }
                /* After the return from the callback, reset the  iAP2FileXferRxBuf and
                 * iAP2FileXferRxLen.
                 */
                iAP2FileXferSession->iAP2FileXferRxBuf = NULL;
                iAP2FileXferSession->iAP2FileXferRxLen = 0;
            }
            else if(fileXfer->state == kiAP2FileTransferStateCancelRecv)
            {
                /* Call cancel callback registered by application */
                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb != NULL)
                {

                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb)(iap2Device,
                                                                                       iAP2FileXferSession,
                                                                                       iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"File Transfer CANCEL callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }

            }
            else if(fileXfer->state == kiAP2FileTransferStatePauseRecv)
            {
                /* Call pause callback registered by application */
                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferPause_cb != NULL)
                {
                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferPause_cb)(iap2Device,
                                                                                      iAP2FileXferSession,
                                                                                      iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"File Transfer PAUSE callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }

            }
            else
            {
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Unidentified file transfer state. DevID:%p",iap2Device);
                rc = IAP2_CTL_ERROR;
            }

        }
        else
        {
            /* Reached End of List and Setup datagram for this file ID is not received yet*/
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Received inactive FileID:%d DevID:%p",
                                           fileXfer->bufferID, iap2Device);

            rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, " Invalid input DevID:%p",iap2Device);

        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        ret = FALSE;
    }

    return ret;
}

/***************************************************************************//**
 * This callback is called when device acknowledge the received data
 * by success/failure/Cancel
 *
 * \param fileXfer  File Transfer Structure which contains data buffer, buffer id,
 *                  session id, total size of the file and so on
 * \param userInfo  additional information
 *
 * \return TRUE     On Successful completion of sending success or failure
 * \return FALSE    Error in file transfer state
 * \see
 * \note
 *******************************************************************************/
BOOL iAP2FiletransferTransmit_CB(struct iAP2FileTransfer_st* fileXfer, void* userInfo)
{
    BOOL ret = TRUE;
    S32 rc = IAP2_OK;
    iAP2Device_st* iap2Device = NULL;
    iAP2FileTransferSession_t* iAP2FileXferSession = NULL;

    iap2Device = (iAP2Device_st*)userInfo;

    if( (iap2Device != NULL) && (fileXfer != NULL) )
    {
        iAP2FileXferSession = iap2Device->iAP2FileTransfer_list;

        /*modifies the iAP2FileXferSession to point to file transfer object matching fileTransferID*/
        rc = iAP2CheckIfFileXferIDPresent(&iAP2FileXferSession, fileXfer->bufferID);
        if(rc == IAP2_OK)
        {
            if((fileXfer->state == kiAP2FileTransferStateSend) || (fileXfer->state == kiAP2FileTransferStateFinishSend))
            {
                iAP2FileXferSession->iAP2FileXferRxBuf = fileXfer->pBuffer;
                iAP2FileXferSession->iAP2FileXferRxLen = fileXfer->buffSize;

                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataSent_cb != NULL)
                {
                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataSent_cb)(iap2Device,
                                                                                         iAP2FileXferSession,
                                                                                         iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_WARN,"File Transfer SEND callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }

                if(fileXfer->state == kiAP2FileTransferStateFinishSend)
                {
                    rc = iAP2FileXferSuccessorFail(iap2Device, iAP2FileXferSession);
                }

                /* After the return from the callback, reset the  iAP2FileXferRxBuf and iAP2FileXferRxLen */
                iAP2FileXferSession->iAP2FileXferRxBuf = NULL;
                iAP2FileXferSession->iAP2FileXferRxLen = 0;
            }
            else if(fileXfer->state == kiAP2FileTransferStateCancelSend)
            {
                /* Call cancel callback registered by application */
                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb != NULL)
                {

                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb)(iap2Device,
                                                                                       iAP2FileXferSession,
                                                                                       iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_WARN,"File Transfer CANCEL callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }

            }
            else if(fileXfer->state == kiAP2FileTransferStateWaitStatus)
            {
                IAP2SESSIONDLTLOG(DLT_LOG_INFO,"File is transmitted from the accessory. Waiting for SUCCESS from the Apple device");
            }
            else if(fileXfer->state == kiAP2FileTransferStateFailSend)
            {
                if(iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb != NULL)
                {
                    (*iap2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb)(iap2Device,
                                                                                        iAP2FileXferSession,
                                                                                        iap2Device->iAP2ContextCallback);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_WARN,"File Transfer FAILURE callback not registered by App. DevID:%p",iap2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
            else
            {
                IAP2SESSIONDLTLOG(DLT_LOG_WARN,"Unidentified file transfer state. DevID:%p",iap2Device);
                rc = IAP2_CTL_ERROR;
            }
        }
        else
        {
            /* Reached End of List and Setup datagram for this file ID is not received yet*/
            IAP2SESSIONDLTLOG(DLT_LOG_WARN,"Received inactive FileID:%d DevID:%p",
                                           fileXfer->bufferID, iap2Device);

            rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, " Invalid input DevID:%p",iap2Device);

        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        ret = FALSE;
    }

    return ret;
}

/***************************************************************************//**
 * Creates File transfer object when setup datagram is received from device
 *
 * \param this_iAP2Device  Structure which contains information about the device
 *                         connected to the target.
 * \param t_iAP2FileXferSession  File Transfer session to be added in the list
 *
 * \return IAP2_OK          On Successful completion of creating file transfer
 *                          object.
 * \return IAP2_ERR_NO_MEM  While unable to allocate memory..
 * \see
 * \note
 *******************************************************************************/

LOCAL S32 iAP2FileTransferObjectCreate(iAP2Device_st* this_iAP2Device, iAP2FileTransferSession_t*  t_iAP2FileXferSession)
{
    S32 rc = IAP2_OK;
    U8 session = 0;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2FileTransfer_t* iAP2FileXrHdl = NULL;

    /*
     * As Link layer is configured to allocate its own memory we need not allocate
     * memory for the  iAP2FileXrHdl. But we must call iAP2FileTransferDelete()
     * during de-initialization. iAP2FileTransferCreate() last parameter made
     * NULL.
     */

    linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

    /*Get session ID*/
    session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

    iAP2FileXrHdl = iAP2FileTransferCreate (linkRunLoop->link,
                                            session,
                                            t_iAP2FileXferSession->iAP2FileTransferID,
                                            &iAP2FiletransferReceive_CB,
                                            this_iAP2Device,
                                            this_iAP2Device->iAP2AccessoryConfig.iAP2FileXferRcvAsStream,
                                            NULL);
    if(iAP2FileXrHdl != NULL)
    {
        t_iAP2FileXferSession->iAP2FileXferHdl = iAP2FileXrHdl;
        t_iAP2FileXferSession->NextTransfer = NULL;
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
    }

    return rc;
}

/***************************************************************************//**
 * Returns number of file transfer in progress
 *
 * \param   p_iAP2FileXferSession  File Transfer session List
 *
 * \return  Number of file transfer in progress
 *
 * \see
 * \note
 *******************************************************************************/

LOCAL U8 iAP2FileTransferCount(const iAP2FileTransferSession_t*  p_iAP2FileXferSession)
{
    U8 count = 0;

    while(p_iAP2FileXferSession != NULL)
    {
        count++;
        p_iAP2FileXferSession = p_iAP2FileXferSession->NextTransfer;
    }

    return count;
}

/***************************************************************************//**
 * Deletes elements in the given file transfer session structure from list
 *
 * \param  iAP2FileXferSession  File Transfer session
 * \return none
 * \see
 * \note
 *******************************************************************************/
LOCAL void iAP2FileXferSessionFree(iAP2FileTransferSession_t* iAP2FileXferSession)
{
    if(iAP2FileXferSession->iAP2FileXferHdl != NULL)
    {
        iAP2FreePointer( (void**)&iAP2FileXferSession->iAP2FileXferHdl);
    }

    if(iAP2FileXferSession->iAP2FileXferRxBuf != NULL)
    {
        iAP2FreePointer( (void**)&iAP2FileXferSession->iAP2FileXferRxBuf);
    }
}

/***************************************************************************//**
 * Checks for the file transfer ID in the list and deletes the file transfer
 * structure from list
 *
 * \param p_iAP2FileXferSession  Pointer to File Transfer session list
 * \param FileTransferID         File transfer ID
 *
 * \return None
 *
 * \see
 * \note
 *******************************************************************************/
LOCAL void iAP2FileXferSessionDelete(iAP2FileTransferSession_t** p_iAP2FileXferSession, const U8 FileTransferID)
{
    iAP2FileTransferSession_t* t_iAP2FileXferSession = NULL;
    iAP2FileTransferSession_t* prev_iAP2FileXferSession = NULL;

    if((*p_iAP2FileXferSession) != NULL)
    {
        if((*p_iAP2FileXferSession)->iAP2FileTransferID == FileTransferID)
        {
            iAP2FileTransferSession_t* next_iAP2FileXferSession = (*p_iAP2FileXferSession)->NextTransfer;

            /* First element in the list */
            iAP2FileXferSessionFree(*p_iAP2FileXferSession);
            iAP2FreePointer( (void**)p_iAP2FileXferSession);
            *p_iAP2FileXferSession = next_iAP2FileXferSession;
        }
        else
        {
            t_iAP2FileXferSession = *p_iAP2FileXferSession;

            while( (t_iAP2FileXferSession != NULL) &&
                   (t_iAP2FileXferSession->iAP2FileTransferID != FileTransferID ) )
            {
                prev_iAP2FileXferSession = t_iAP2FileXferSession;
                t_iAP2FileXferSession = t_iAP2FileXferSession->NextTransfer;
            }

            if(t_iAP2FileXferSession  != NULL)
            {
                if(t_iAP2FileXferSession->iAP2FileTransferID == FileTransferID)
                {
                    prev_iAP2FileXferSession->NextTransfer = (t_iAP2FileXferSession)->NextTransfer;
                    iAP2FileXferSessionFree(t_iAP2FileXferSession);
                    iAP2FreePointer( (void**)&t_iAP2FileXferSession);
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"FileID not present in the list:%d",FileTransferID);
                }
            }
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input:p_iAP2FileXferSession ");
    }
}

/***************************************************************************//**
 * Initialise and start the file transfer
 *
 * This function will create the file transfer handle on receiving the setup data-
 * gram and trigger iAP2FileTransferSetup_cb() registered by the application to
 * provide the file details. Based on the return value from the setup callback,
 * this will send start command to apple device to start the file transfer or may
 * cancel the setup.
 *
 * \param this_iAP2Device  Pointer to Device structure
 * \param p_Sourcebuf      data buffer received from device
 * \param p_SourceBufSize  size of p_Sourcebuf
 *
 * \return IAP2_OK                            On Successful completion of sending
 *                                            start datagram to device
 * \return IAP2_FILE_XFER_SETUP_ALREADY_RECVD If duplicate setup datagram received
 * \return IAP2_CTL_ERROR                     Error in received data
 * \return IAP2_FILE_XFER_MAX_XFER_REACHED    If number of file transfer in progress
 *                                            is IAP2_FILE_XFER_MAX_TRANSFERS
 * \see
 * \note
 *******************************************************************************/
LOCAL S32 iAP2FileXferStart(iAP2Device_st* this_iAP2Device, const U8* p_Sourcebuf, const U32 p_SourceBufSize )
{
    BOOL ret = FALSE;
    S32  rc = IAP2_OK;
    U8   Xfercount = 0;
    U8   session =0;
    iAP2LinkRunLoop_t*         linkRunLoop = NULL;
    iAP2FileTransferSession_t* iAP2FileXferSession = NULL;
    iAP2FileTransferSession_t* t_iAP2FileXferSession = NULL;

    linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

    /* Get Session ID. (Set during link negotiation) */
    session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

    if(this_iAP2Device->iAP2FileTransfer_list == NULL)
    {
        /*very first file transfer object. Create the list*/
        this_iAP2Device->iAP2FileTransfer_list = (iAP2FileTransferSession_t*)calloc(1, sizeof(iAP2FileTransferSession_t));
        t_iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list;
    }
    else
    {
        /*File transfer list already present. Get the list-head */
        iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list;

        /*check if the file id is already preset in the list.
         *
         * NOTE : iAP2CheckIfFileXferIDPresent() modifies the iAP2FileXferSession.
         * After return from the function, iAP2FileXferSession may no more point to
         * the list-head
         */
        rc = iAP2CheckIfFileXferIDPresent(&iAP2FileXferSession, p_Sourcebuf[kiAP2FileTransferHdrIdxID]);
        if(rc == IAP2_FILE_XFER_ID_NOT_PRESENT)
        {
            /*Get the list-head*/
            iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list;

            /* Find empty element, to start new transfer */
            while(iAP2FileXferSession != NULL)
            {
                t_iAP2FileXferSession = iAP2FileXferSession;
                iAP2FileXferSession = iAP2FileXferSession->NextTransfer;
            }

            t_iAP2FileXferSession->NextTransfer = (iAP2FileTransferSession_t*)calloc(1, sizeof(iAP2FileTransferSession_t));
            t_iAP2FileXferSession = t_iAP2FileXferSession->NextTransfer;
        }
        else
        {
            rc = IAP2_FILE_XFER_SETUP_ALREADY_RECVD;
        }
    }

    /* New setup datagram received and found a empty place in the list to add the new file transfer*/
    if( t_iAP2FileXferSession != NULL)
    {
        /* Check number of simultaneous file transfer in progress
         * TODO:After finding empty element  we are counting total file transfer.
         * */
        Xfercount = iAP2FileTransferCount(this_iAP2Device->iAP2FileTransfer_list);

        if(Xfercount <= IAP2_FILE_XFER_MAX_TRANSFERS)
        {
            /* Get FileXfer ID for the current transfer before creating the FileXfer object(FileXfer Handle).
             * ref: iAP2FileTransfer.h
             * */
            t_iAP2FileXferSession->iAP2FileTransferID = p_Sourcebuf[kiAP2FileTransferHdrIdxID];

            /* Maximum of 127 simultaneous transfers allowed */
            rc = iAP2FileTransferObjectCreate(this_iAP2Device, t_iAP2FileXferSession);

            if(rc == IAP2_OK)
            {
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"File Handle created for FileID:%d DevID:%p",
                                  p_Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

                /* This will handle setup data only
                 */
                ret = iAP2FileTransferHandleRecv(t_iAP2FileXferSession->iAP2FileXferHdl,
                                                 p_Sourcebuf,
                                                 p_SourceBufSize);

                /* iAP2FileTransferHandleRecv() returns TRUE when the file handle is ready to be
                 * deleted or cleaned. Here  iAP2FileTransferHandleRecv() will only handle
                 * packet type kiAP2FileTransferPacketTypeSetup and it will always return false.
                 *
                 * It would be not necessary to check 'ret'
                 * But, retain the old code ;o)
                 */
                if(ret == FALSE)
                {
                    /* Total size of incoming file is calculated as part of iAP2FileTransferHandleRecv(), and
                     * file transfer handle(fileXfer->totalSize) is updated with size value  */

                    /* Get file size from file transfer handle*/
                    t_iAP2FileXferSession->iAP2FileXferRxLen = ((iAP2FileTransfer_t*)t_iAP2FileXferSession->iAP2FileXferHdl)->totalSize;

                    /* Now call the setup callback registered by application. This is moved here to make sure
                     * file transfer object is created before we trigger the application callback.
                     */
                    if((this_iAP2Device->iAP2FileTransferCallbacks).iAP2FileTransferSetup_cb != NULL)
                    {
                        rc = (this_iAP2Device->iAP2FileTransferCallbacks).iAP2FileTransferSetup_cb(this_iAP2Device,
                                t_iAP2FileXferSession,
                                this_iAP2Device->iAP2ContextCallback);
                    }
                    /*Proceed further only for iap2-library model*/
                    if(this_iAP2Device->iAP2DeviceId == 0)
                    {
                        /* If callback returns OK , It means application is ready to receive the file.
                         * Start the file transfer.
                         */
                        if((rc == IAP2_OK) && (t_iAP2FileXferSession->iAP2FileXferRxLen > 0))
                        {
                            /* Start/Resume the File transfer. Start and Resume are same w.r.t to
                             * Apple provided File transfer code.
                             */
                            iAP2FileTransferResume (t_iAP2FileXferSession->iAP2FileXferHdl);

                            IAP2SESSIONDLTLOG(DLT_LOG_INFO,"Start File transfer sent to Apple device. FileID:%d DevID:%p",
                                              p_Sourcebuf[kiAP2FileTransferHdrIdxID],this_iAP2Device);
                        }
                        else
                        {
                            /* File size is zero or Application is not interested to receive
                             * the file, so cancel the transfer
                             */
                            IAP2SESSIONDLTLOG(DLT_LOG_INFO, "FileID; %d,File size is zero DevID:%p",
                                              p_Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

                            /* We must cleanup the file transfer object here.
                             * We don't need it anymore as file size is zero or Application is not interested
                             * in the file.
                             */
                            iAP2FileTransferDelete (t_iAP2FileXferSession->iAP2FileXferHdl);
                            t_iAP2FileXferSession->iAP2FileXferHdl= NULL;

                            /*Send cancel setup to Apple device */
                            iAP2FileTransferCancelSetup(linkRunLoop->link,
                                                        session,
                                                        p_Sourcebuf[kiAP2FileTransferHdrIdxID]);
                            rc = IAP2_CTL_ERROR;
                        }
                    }
                }
                else
                {
                    /* Should never enter here. If in case it does, set rc to error :o) */
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid return after processing Setup Datagram. DevID:%p",this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Exceeded maximum allowed transfers DevID:%p",this_iAP2Device);
            /* Max file transfer reached, send cancelsetup command to Apple device
             * No file transfer object creation or NO callback to application.
             * Handle internally.
             */
            iAP2FileTransferCancelSetup(linkRunLoop->link,
                                        session,
                                        p_Sourcebuf[kiAP2FileTransferHdrIdxID]);

            rc = IAP2_FILE_XFER_MAX_XFER_REACHED;
        }

        if(rc != IAP2_OK)
        {
            /*Delete file transfer session object from list*/
            iAP2FileXferSessionDelete(&this_iAP2Device->iAP2FileTransfer_list, p_Sourcebuf[kiAP2FileTransferHdrIdxID]);
        }
    }
    else
    {
        /* No empty element found or memory error*/
        rc = IAP2_ERR_NO_MEM;
    }

    return rc;
}

/***************************************************************************//**
 * Signal to send the START command to Apple device
 *
 * This function will trigger to send the START cmd to AD in response to the
 * setup command received before.
 *
 * \param this_iAP2Device  Pointer to Device structure
 *
 * Either OK or NOK to start  (If NOK, then cancel will be sent
 * \see
 * \note
 *******************************************************************************/
S32 iAP2FileXferStartOk(iAP2Device_st* this_iAP2Device, U8 ftid, BOOL startOK )
{
    iAP2LinkRunLoop_t*         linkRunLoop = NULL;
    U8   session =0;
    S32  rc = IAP2_OK;
    iAP2FileTransferSession_t* iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list; /*list head*/
    linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;
    /* Get Session ID. (Set during link negotiation) */
    session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

    /*check if the file id is already preset in the list.
     * NOTE : iAP2CheckIfFileXferIDPresent() modifies the iAP2FileXferSession.
     * After return from the function, iAP2FileXferSession may no more point to
     * the list-head */
    rc = iAP2CheckIfFileXferIDPresent(&iAP2FileXferSession, ftid);
    if(rc == IAP2_FILE_XFER_ID_NOT_PRESENT)
    {
        IAP2SESSIONDLTLOG(DLT_LOG_WARN, "IAP2_FILE_XFER_ID_NOT_PRESENT");
        return IAP2_FILE_XFER_ID_NOT_PRESENT;
    }

    if(startOK == TRUE)
    {
        /* Start/Resume the File transfer. Start and Resume are same w.r.t to
         * Apple provided File transfer code.*/
        iAP2FileTransferResume (iAP2FileXferSession->iAP2FileXferHdl);
        IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE,"Start File transfer sent to Apple device. FileID:%d DevID:%p",
                                                iAP2FileXferSession->iAP2FileTransferID,this_iAP2Device);
    }
    else
    {
        /* File size is zero or Application is not interested to receive
         * the file, so cancel the transfer*/
        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "FileID; %d, Application not interested DevID:%p",
                                    iAP2FileXferSession->iAP2FileTransferID, this_iAP2Device);
        /* We must cleanup the file transfer object here.
         * We don't need it anymore as file size is zero or Application is not interested
         * in the file. */
        iAP2FileTransferDelete (iAP2FileXferSession->iAP2FileXferHdl);
        iAP2FileXferSession->iAP2FileXferHdl= NULL;
        /*Send cancel setup to Apple device */
        iAP2FileTransferCancelSetup(linkRunLoop->link,session,ftid);
        /*Delete file transfer session object from list*/
        iAP2FileXferSessionDelete(&this_iAP2Device->iAP2FileTransfer_list, ftid);
    }

    return rc;
}

/***************************************************************************//**
 * Parses file transfer data received from device and responds accordingly.
 *
 * \param this_iAP2Device  Structure which contains information about the device
 *                         connected to the target.
 * \param Sourcebuf        data buffer from link layer
 * \param SourceBufSize    Size of data buffer from link layer
 *
 * \return IAP2_OK         On Successful completion of handling datagram
 * \return IAP2_CTL_ERROR  If inactive file transfer identifier is received during
 *                         transfer.
 * \see
 * \note
 *******************************************************************************/
S32 iAP2ParseFileTransferMessage(iAP2Device_st* this_iAP2Device, const U8* Sourcebuf, const U32 SourceBufSize)
{
    S32  rc = IAP2_OK;
    BOOL ret = FALSE;
    U8   session = 0;
    BOOL bBuffID = FALSE;

    iAP2LinkRunLoop_t*          linkRunLoop = NULL;
    iAP2FileTransferSession_t*  iAP2FileXferSession = NULL;
    iAP2FileTransfer_t*         iAP2FileXrHdl = NULL;


    if((this_iAP2Device != NULL) && (Sourcebuf != NULL))
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

        /* Get Session ID. (Set during link negotiation) */
        session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

        /* Check if buffer direction is valid */
        bBuffID = iAP2FileTransferValidateBufferID(linkRunLoop->link,
                                                   session,
                                                   Sourcebuf[kiAP2FileTransferHdrIdxID]);
        if( (bBuffID == TRUE) && (SourceBufSize > 0) )
        {
            if((Sourcebuf[kiAP2FileTransferHdrIdxControl] == kiAP2FileTransferPacketTypeSetup) &&
                                            (SourceBufSize >= kiAP2FileTransferHdrSetupBaseLen))
            {
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,
                                  "Received SetupDatagram FileID:%d DevID:%p",
                                  Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

            /* Initialise and start file transfer */
            rc = iAP2FileXferStart(this_iAP2Device, Sourcebuf, SourceBufSize);
            }
            else
            {
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,
                                  "Received Datagram : 0x%2X, FileID:%d DevID:%p",
                                  Sourcebuf[kiAP2FileTransferHdrIdxControl],
                                  Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

                iAP2FileXferSession   = this_iAP2Device->iAP2FileTransfer_list;

                if(iAP2FileXferSession != NULL)
                {
                    rc = iAP2CheckIfFileXferIDPresent(&iAP2FileXferSession, Sourcebuf[kiAP2FileTransferHdrIdxID]);
                    if(rc == IAP2_OK)
                    {
                        iAP2FileXrHdl = (iAP2FileTransfer_t*)iAP2FileXferSession->iAP2FileXferHdl;
                        if(iAP2FileXrHdl != NULL)
                        {
                            ret = iAP2FileTransferHandleRecv(iAP2FileXrHdl, Sourcebuf, SourceBufSize);
                            if(ret == TRUE)
                            {
                                /* Delete and cleanup file transfer handle when iAP2FileTransferHandleRecv()
                                 * returns TRUE  */
                                iAP2FileTransferDelete (iAP2FileXrHdl);
                                iAP2FileXferSession->iAP2FileXferHdl= NULL;

                                /* Delete Current iAP2FileXferSession from File transfer list
                                 * TODO: Check how an node object is deleted. It should not
                                 * break the link list
                                 */
                                iAP2FileXferSessionDelete(&(this_iAP2Device->iAP2FileTransfer_list), iAP2FileXferSession->iAP2FileTransferID);
                            }
                        }
                        else
                        {
                            rc = IAP2_CTL_ERROR;
                        }
                    }
                    else
                    {
                        /* Reached End of List and Setup datagram for this file ID is not received yet*/
                        IAP2SESSIONDLTLOG(DLT_LOG_WARN," Received Inactive FileID:%d DevID:%p",
                                         Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

                        if(Sourcebuf[kiAP2FileTransferHdrIdxControl] != kiAP2FileTransferPacketTypeCancel)
                        {
                            iAP2FileTransferCancelSetup(linkRunLoop->link,
                                                        session,
                                                        Sourcebuf[kiAP2FileTransferHdrIdxID]);
                        }
                        rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
                    }
                }
                else
                {
                    /* No File transfer In Progress */
                    IAP2SESSIONDLTLOG(DLT_LOG_WARN," No File transfer In Progress, Received FileID: %d, Datagram: 0x%.2X DevID:%p",
                                      Sourcebuf[kiAP2FileTransferHdrIdxID], Sourcebuf[kiAP2FileTransferHdrIdxControl], this_iAP2Device);
                }
            }
        }
        else
        {
            /* Received wrong file transfer identifier */
            iAP2FileTransferCancelSetup(linkRunLoop->link,
                                        session,
                                        Sourcebuf[kiAP2FileTransferHdrIdxID]);
            IAP2SESSIONDLTLOG(DLT_LOG_WARN,"Received invalid FileID:%d DevID:%p",
                              Sourcebuf[kiAP2FileTransferHdrIdxID], this_iAP2Device);

            rc = IAP2_FILE_XFER_INVALID_ID;
        }
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid input DevID:%p", this_iAP2Device);
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    return rc;
}

/***************************************************************************//**
 * Parses file transfer data received from device and responds accordingly
 *
 * \param  iAP2Device      Structure which contains information about the device
 *                          connected to the target.
 * \param  Sourcebuf       data buffer from link layer
 * \param  SourceBufSize   Size of data buffer from link layer
 * \return IAP2_OK         On Successful completion of handling datagram
 * \return IAP2_CTL_ERROR  On receiving incomplete header or invalid command
 * \see
 * \note
 *******************************************************************************/
S32 iAP2ServiceParseFileTransferMessage(iAP2Device_t* iAP2Device, U8* Sourcebuf, const U32 SourceBufSize)
{
    iAP2Device_st* this_iAP2Device = (iAP2Device_st*)iAP2Device;
    iAP2FTMessage_t *msg=NULL;
    IAP2FTCmd_t ftcmd;
    void* context=this_iAP2Device->iAP2ContextCallback;
    S32 rc = IAP2_CTL_ERROR;

    if(SourceBufSize < IAP2FTMESSAGE_HDRSIZE)
    {
        IAP2SESSIONDLTLOG(DLT_LOG_WARN,"Incomplete header %d", SourceBufSize);
        return IAP2_CTL_ERROR;
    }
    msg=(iAP2FTMessage_t*)(void*)Sourcebuf;
    ftcmd = msg->ftcmd;
    iAP2FileTransferSession_t  iAP2FileXferSession;
    iAP2FileXferSession.iAP2FileTransferID=msg->ftid;
    iAP2FileXferSession.iAP2FileXferRxBuf=NULL;
    iAP2FileXferSession.iAP2FileXferRxLen=msg->len;
    iAP2FileXferSession.iAP2FileXferHdl=NULL; //not relevant for application
    iAP2FileXferSession.NextTransfer=NULL;//not relevant for application
    switch(ftcmd)
    {
        case FileTransferSetup:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"Received setup len = %llu",iAP2FileXferSession.iAP2FileXferRxLen);
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSetup_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSetup_cb(this_iAP2Device,&iAP2FileXferSession,context);
                if(rc == IAP2_OK)
                {
                    msg->ftcmd=FileTransferStartOK;
                }
                else
                {
                    msg->ftcmd=FileTransferStartNOK;
                }
                msg->deviceid=this_iAP2Device->iAP2DeviceId;
                rc = iAP2ServiceSendMessageToDevice(this_iAP2Device,msg,sizeof(U32)+sizeof(U64)+sizeof(U64)+sizeof(U32),FileTransfer);
            }
            break;
        }
        case FileTransferDataRcvd:
        {
            iAP2FileXferSession.iAP2FileXferRxBuf=msg->buff;
            IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE,"dataRcvd");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataRcvd_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferDataRcvd_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        case FileTransferSuccess:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"Success");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSuccess_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferSuccess_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        case FileTransferFailure:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"failure");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferFailure_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        case FileTransferPause:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"pause");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferPause_cb   != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferPause_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        case FileTransferResume:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_INFO,"resume");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferResume_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferResume_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        case FileTransferCancel:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"cancel");
            if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb != NULL)
            {
                rc = this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferCancel_cb(this_iAP2Device,&iAP2FileXferSession,context);
            }
            break;
        }
        default:
        {
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG,"default");
            break;
        }
    }
    return rc;
}
/***************************************************************************//**
 * This function calls iAP2SendFileTransferCancel API of link layer to send Cancel
 * datagram to device.
 *
 * \param this_iAP2Device  Structure which contains information about the device
 *                         connected to the target.
 * \param FileTransferID   File transfer Identifier for which File transfer has to
 *                         be cancelled
 *
 * \return IAP2_OK         On Successful completion of sending cancel datagram
 *                         to device.
 * \return IAP2_CTL_ERROR  if File transfer structure is NULL..
 * \see
 * \note
 *******************************************************************************/
S32 iAP2CancelFileTransfer(iAP2Device_t* iAP2Device, const U8 FileTransferID)
{
    S32 rc = IAP2_OK;
    U8 session = 0;
    BOOL bBuffID = FALSE;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2FileTransfer_t* iAP2FileXrHdl = NULL;
    iAP2FileTransferSession_t* fileTransferSession = NULL;
    iAP2Device_st* this_iAP2Device = (iAP2Device_st*)iAP2Device;

    if(this_iAP2Device != NULL)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

        /* Get Session ID. Set during link negotiation*/
        session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

        /* Check if buffer direction is valid*/
        bBuffID = iAP2FileTransferValidateBufferID(linkRunLoop->link, session,FileTransferID);
        if(bBuffID == TRUE)
        {
            fileTransferSession = this_iAP2Device->iAP2FileTransfer_list;

            rc = iAP2CheckIfFileXferIDPresent(&fileTransferSession, FileTransferID);
            if(rc  == IAP2_OK)
            {
                iAP2FileXrHdl = (iAP2FileTransfer_t*)fileTransferSession->iAP2FileXferHdl;

                if(iAP2FileXrHdl != NULL)
                {
                    if((iAP2FileXrHdl->state == kiAP2FileTransferStateRecv) ||
                       (iAP2FileXrHdl->state == kiAP2FileTransferStatePauseRecv))
                    {
                        /* Send cancel to Apple device .
                         *
                         * Apple device will respond with a cancel. Cancel response
                         * will be handled in iAP2FiletransferReceive_CB(). FileXfer
                         * handle and session will be cleaned afrer return from
                         * iAP2FiletransferReceive_CB() in iAP2ParseFileTransferMessage()
                         */

                        iAP2FileTransferCancel(iAP2FileXrHdl);
                    }
                    else
                    {
                        IAP2SESSIONDLTLOG(DLT_LOG_INFO,"File transfer can't be cancelled for fileID:%d, State not received DevID:%p",
                                          FileTransferID, this_iAP2Device);
                        rc = IAP2_CTL_ERROR;
                    }
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid iAP2FileXferHdl DevID:%p", this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
            else
            {
                /* Reached End of List and Setup datagram for this fileID is not received yet */
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Received inactive FileID:%d DevID:%p", FileTransferID, this_iAP2Device);
                rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
            }
        }
        else
        {
            rc = IAP2_FILE_XFER_INVALID_ID;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid FileID:%d DevID:%p", FileTransferID, this_iAP2Device);
        }
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid Input DevID:%p", this_iAP2Device);
    }

    return rc;
}
/***************************************************************************//**
 * This function calls iAP2FileTransferPause API of link layer to send pause
 * datagram to device.
 *
 * \param this_iAP2Device  Structure which contains information about the device
 *                         connected to the target.
 * \param FileTransferID   File transfer Identifier for which File transfer has
 *                         to be Paused
 *
 * \return IAP2_OK         On Successful completion of sending pause datagram to
 *                         device.
 * \return IAP2_CTL_ERROR  if File transfer structure is NULL..
 *
 * \see
 * \note
 *******************************************************************************/
S32 iAP2PauseFileTransfer(iAP2Device_t* iAP2Device, const U8 FileTransferID)
{
    S32 rc = IAP2_OK;
    U8  session = 0;
    BOOL bBuffID = FALSE;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2FileTransfer_t* iAP2FileXrHdl = NULL;
    iAP2FileTransferSession_t* fileTransferStruct = NULL;
    iAP2Device_st* this_iAP2Device = (iAP2Device_st*)iAP2Device;

    if(this_iAP2Device != NULL)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

        /* Get Session ID. Set during link negotiation*/
        session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

        /* Check if buffer direction is valid*/
        bBuffID = iAP2FileTransferValidateBufferID(linkRunLoop->link,session,FileTransferID);
        if(bBuffID == TRUE)
        {
            fileTransferStruct = this_iAP2Device->iAP2FileTransfer_list;
            rc = iAP2CheckIfFileXferIDPresent(&fileTransferStruct, FileTransferID);
            if(rc == IAP2_OK)
            {
                iAP2FileXrHdl = (iAP2FileTransfer_t*)fileTransferStruct->iAP2FileXferHdl;
                if(iAP2FileXrHdl != NULL)
                {
                    if(iAP2FileXrHdl->state == kiAP2FileTransferStateRecv)
                    {

                        /* Send file transfer pause command to Apple device .
                         * Response will be handled in iAP2FiletransferReceive_CB()*/
                        iAP2FileTransferPause(iAP2FileXrHdl);
                    }
                    else
                    {
                        IAP2SESSIONDLTLOG(DLT_LOG_INFO," File Transfer can't be paused for fileID:%d,State not received DevID:%p",
                                          FileTransferID, this_iAP2Device);
                        rc = IAP2_CTL_ERROR;
                    }
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid iAP2FileXrHdl DevID:%p", this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
            else
            {
                /* Reached End of List and Setup datagram for this file ID is not received yet*/
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR," Received inactive FileID:%d DevID:%p", FileTransferID, this_iAP2Device);
                rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
            }
        }
        else
        {
            rc = IAP2_FILE_XFER_INVALID_ID;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid fileID:%d DevID:%p", FileTransferID, this_iAP2Device);
        }
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    return rc;
}

/***************************************************************************//**
 * Sends Resume datagram to device
 *
 * \param this_iAP2Device  Structure which contains information about the device
 *                         connected to the target.
 * \param FileTransferID   File transfer Identifier for which File transfer has
 *                         to be resumed
 * \return IAP2_OK         On Successful completion of sending Resume datagram to
 *                         device.
 * \return IAP2_CTL_ERROR  if File transfer structure is NULL..
 * \see
 * \note
 *******************************************************************************/
S32 iAP2ResumeFileTransfer(iAP2Device_t* iAP2Device, const U8 FileTransferID)
{
    S32 rc = IAP2_OK;
    U8  session = 0;
    BOOL bBuffID = FALSE;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2FileTransfer_t*iAP2FileXrHdl = NULL;
    iAP2FileTransferSession_t* fileTransferStruct = NULL;
    iAP2Device_st* this_iAP2Device = (iAP2Device_st*)iAP2Device;


    if(this_iAP2Device != NULL)
    {
        linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;
        session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

        bBuffID = iAP2FileTransferValidateBufferID(linkRunLoop->link,session,FileTransferID);
        if(bBuffID == TRUE)
        {
            fileTransferStruct = this_iAP2Device->iAP2FileTransfer_list;
            rc = iAP2CheckIfFileXferIDPresent(&fileTransferStruct, FileTransferID);
            if(rc == IAP2_OK)
            {
                iAP2FileXrHdl = (iAP2FileTransfer_t*)fileTransferStruct->iAP2FileXferHdl;
                if(iAP2FileXrHdl != NULL)
                {
                    if(iAP2FileXrHdl->state == kiAP2FileTransferStatePauseRecv)
                    {
                        /*Send resume command to Apple device */
                        iAP2FileTransferResume(iAP2FileXrHdl);

                        /* TBD:
                         * We need not trigger the resume callback, because after resume, received
                         * data from Apple device will be handled in  iAP2FiletransferReceive_CB() and
                         * iAP2FileTransferDataRcvd_cb() callback,registered by Application, will be
                         * be triggered to provide resumed artwork data.
                         */
                        if(this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferResume_cb != NULL)
                        {
                            (*this_iAP2Device->iAP2FileTransferCallbacks.iAP2FileTransferResume_cb)(this_iAP2Device,
                                                                                                    fileTransferStruct,
                                                                                                    this_iAP2Device->iAP2ContextCallback);
                        }
                    }
                    else
                    {
                        IAP2SESSIONDLTLOG(DLT_LOG_ERROR," File transfer not paused. Can't resume. DevID:%p", this_iAP2Device);
                        rc = IAP2_CTL_ERROR;
                    }
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid iAP2FileXrHdl DevID:%p", this_iAP2Device);
                    rc = IAP2_CTL_ERROR;
                }
            }
            else
            {
                /* Reached End of List and Setup datagram for this file ID is not received yet*/
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Received inactive FileID:%d DevID:%p", FileTransferID, this_iAP2Device);
                rc = IAP2_FILE_XFER_SETUP_NOT_RECVD;
            }
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR,"Invalid fileID:%d DevID:%p", FileTransferID, this_iAP2Device);
            rc = IAP2_FILE_XFER_INVALID_ID;
        }
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    return rc;
}

/***************************************************************************//**
 * This function is called to deallocate the file transfer structure if file
 * transfer is stopped in between.
 *
 * \param  iAP2FileTransfer_list File Transfer Session List
 * \return None
 * \see
 * \note
 *******************************************************************************/
void iAP2CleanupFileTransferSession(iAP2Device_t* iAP2Device)
{
    iAP2FileTransferSession_t* temp_iAP2FileTransferSession = NULL;
    iAP2FileTransferSession_t* iAP2FileTransferSession_list = ((iAP2Device_st*)iAP2Device)->iAP2FileTransfer_list;

    if(iAP2FileTransferSession_list != NULL)
    {
        temp_iAP2FileTransferSession = iAP2FileTransferSession_list->NextTransfer;
        while(temp_iAP2FileTransferSession != NULL)
        {
            iAP2FileTransferSession_list->NextTransfer = temp_iAP2FileTransferSession->NextTransfer;
            iAP2FileXferSessionFree(temp_iAP2FileTransferSession);
            iAP2FreePointer( (void**)&temp_iAP2FileTransferSession);
            temp_iAP2FileTransferSession = iAP2FileTransferSession_list->NextTransfer;
        }

        IAP2SESSIONDLTLOG(DLT_LOG_INFO,"File Transfer session deleted for FileID:%d DevID:%p",
                          iAP2FileTransferSession_list->iAP2FileTransferID, iAP2Device);

        iAP2FileXferSessionFree(iAP2FileTransferSession_list);
        iAP2FreePointer( (void**)&iAP2FileTransferSession_list);
    }
}
/** @} */


/***************************************************************************//**
 * This function is called to get a file transfer id for sending a file
 * from accessory to Apple Device
 *
 * \param  iAP2Device_st          Structure which contains information about the device
 *                                connected to the target.
 *
 * \return FileTransferID         On successfully finding an unused id,
 *                                return a value between [0..127] both inclusive
 *
 * \return IAP2_FILE_XFER_ERROR   On error
 * \see
 * \note
 *******************************************************************************/
U8 iAP2FiletransferRequestId(iAP2Device_st* this_iAP2Device)
{
    iAP2FileTransferSession_t* iAP2FileXferSession = NULL;
    iAP2FileTransferSession_t* iAP2FileXferSession_last = NULL;

    if(this_iAP2Device->iAP2FileTransfer_list == NULL)
    {
        /* Very first file transfer object. Create the list*/
        this_iAP2Device->iAP2FileTransfer_list = (iAP2FileTransferSession_t*)calloc(1, sizeof(iAP2FileTransferSession_t));
        if(this_iAP2Device->iAP2FileTransfer_list != NULL)
        {
            iAP2FileXferSession_last = this_iAP2Device->iAP2FileTransfer_list;
            iAP2FileXferSession_last->iAP2FileTransferID = 0;
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "No memory available");
            return IAP2_FILE_XFER_ERROR;
        }
    }
    else
    {
        /* File transfer list already present
         * Allocate a new available file transfer id */
        U8 id;
        for(id = 0; id < IAP2_FILE_XFER_MAX_TRANSFERS; id++)
        {
            BOOL unused = TRUE;

            /*Get the list-head for every iteration*/
            iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list;
            iAP2FileXferSession_last = iAP2FileXferSession;

            /* Check whether the ID is already present in the list */
            while(iAP2FileXferSession != NULL)
            {
                if(id == iAP2FileXferSession->iAP2FileTransferID)
                {
                    unused = FALSE;
                    break;
                }
                iAP2FileXferSession_last = iAP2FileXferSession; /* Holds the value of the previous node */
                iAP2FileXferSession = iAP2FileXferSession->NextTransfer;
            }
            if(unused == TRUE)
            {
                iAP2FileXferSession_last->NextTransfer = (iAP2FileTransferSession_t*)calloc(1, sizeof(iAP2FileTransferSession_t));
                if(iAP2FileXferSession_last->NextTransfer != NULL)
                {
                    iAP2FileXferSession_last = iAP2FileXferSession_last->NextTransfer;
                    iAP2FileXferSession_last->iAP2FileTransferID = id;
                    break;
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "No memory available");
                    return IAP2_FILE_XFER_ERROR;
                }
            }
        }
        if(id == IAP2_FILE_XFER_MAX_TRANSFERS)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_WARN, "All file transfer ids are in use now!!");
            return IAP2_FILE_XFER_ERROR;
        }
    }
    return iAP2FileXferSession_last->iAP2FileTransferID;
}


/***************************************************************************//**
 * This function is called to send a file from accessory to Apple Device
 *
 * \param  iAP2Device_st Structure which contains information about the device
 *                        connected to the target.
 * \param  Sourcebuf     Buffer which has to be sent to the Apple Device
 * \param  length        Length of the Sourcebuf
 * \param  this_iAP2FileTransferDatagram
 *                       File transfer setup datagram structure
 * \return None
 * \see
 * \note
 *******************************************************************************/
void iAP2FiletransferSendData(iAP2Device_t* iAP2Device, void* Sourcebuf, U32 length, iAP2FileTransferDatagram_t* this_iAP2FileTransferDatagram)
{
    iAP2Device_st* this_iAP2Device = (iAP2Device_st*)iAP2Device;
    S32 rc = IAP2_OK;
    U8 session = 0;
    iAP2LinkRunLoop_t* linkRunLoop = NULL;
    iAP2FileTransfer_t* iAP2FileXrHdl = NULL;
    U8 fileTransferId = iAP2FiletransferRequestId(this_iAP2Device);

    if(fileTransferId < IAP2_FILE_XFER_MAX_TRANSFERS)
    {
        /*
         * As Link layer is configured to allocate its own memory we need not allocate
         * memory for the  iAP2FileXrHdl. But we must call iAP2FileTransferDelete()
         * during de-initialization. iAP2FileTransferCreate() last parameter made
         * NULL.
         */
        linkRunLoop = (iAP2LinkRunLoop_t*)this_iAP2Device->p_iAP2AccessoryLink;

        /* Get session ID */
        session = iAP2LinkGetSessionForService(linkRunLoop->link, kIAP2PacketServiceTypeBuffer);

        iAP2FileXrHdl = iAP2FileTransferCreate(linkRunLoop->link,
                                               session,
                                               fileTransferId,
                                               &iAP2FiletransferTransmit_CB,
                                               this_iAP2Device,
                                               this_iAP2Device->iAP2AccessoryConfig.iAP2FileXferRcvAsStream,
                                               NULL);

        if(iAP2FileXrHdl != NULL)
        {
            iAP2FileXferInitializeHandle(fileTransferId, iAP2FileXrHdl, this_iAP2Device);

            if(linkRunLoop->link->param.sessionInfo[IAP2_FT_SESSION].version == IAP2_FT_VERSION_TWO)
            {
                iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileType = this_iAP2FileTransferDatagram->iAP2FileType;
                iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FTDatagramParamSize = this_iAP2FileTransferDatagram->iAP2FTDatagramParamSize;

                if(iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileType == kiAP2FileTransferTypeCallStateUpdateVCard)
                {
                    iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileTypeSetupData.iAP2CallUUID = this_iAP2FileTransferDatagram->iAP2FileTypeSetupData.iAP2CallUUID;
                }
                else if(iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileType == kiAP2FileTransferTypeAppDiscoveryIconData)
                {
                    iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileTypeSetupData.iAP2AppBundleID = this_iAP2FileTransferDatagram->iAP2FileTypeSetupData.iAP2AppBundleID;
                }
                else if((iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileType == kiAP2FileTransferTypeMediaLibraryUpdatePlaylistContents)
                        || (iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileType == kiAP2FileTransferTypeMediaItemListMediaLibraryUpdatePlaylistContents))
                {
                    iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileTypeSetupData.iAP2MediaLibraryUpdatePlaylistContents.iAP2LibraryUID = this_iAP2FileTransferDatagram->iAP2FileTypeSetupData.iAP2MediaLibraryUpdatePlaylistContents.iAP2LibraryUID;
                    iAP2FileXrHdl->iAP2FileTransferDatagram.iAP2FileTypeSetupData.iAP2MediaLibraryUpdatePlaylistContents.iAP2PlaylistPID = this_iAP2FileTransferDatagram->iAP2FileTypeSetupData.iAP2MediaLibraryUpdatePlaylistContents.iAP2PlaylistPID;
                }
                else
                {
                    IAP2SESSIONDLTLOG(DLT_LOG_INFO, "File Type does not have File Type Setup Data");
                }
            }
            rc = iAP2FileTransferStart(iAP2FileXrHdl,
                                       Sourcebuf,
                                       length,
                                       length,
                                       &iAP2FiletransferTransmit_CB,
                                       this_iAP2Device,
                                       FALSE,
                                       FALSE);/* Do not release the buffer after the call */
            if(rc == 0) /* iAP2FileTransferStart returns 0 on failure */
            {
                rc = IAP2_CTL_ERROR;
                IAP2SESSIONDLTLOG(DLT_LOG_INFO, "iAP2FileTransferStart failed rc:%d",rc);
            }
            else
            {
                rc = IAP2_OK;
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2FileXrHdl is NULL, rc = %d",rc);
        }
    }
    if(rc != IAP2_OK)
    {
        iAP2FileXferSessionDelete(&this_iAP2Device->iAP2FileTransfer_list, fileTransferId);
    }
}

/*  */
/***************************************************************************//**
 * This function is called to update the FileTransfer Handle in the FileTransfer_Tx_List
 *
 * \param  iAP2Device_st Structure which contains information about the device
 *                        connected to the target.
 * \param  Sourcebuf     Buffer which has to be sent to the Apple Device
 * \param  length        Length of the Sourcebuf
 * \param  this_iAP2FileTransferDatagram
 *                       File transfer setup datagram structure
 * \return None
 * \see
 * \note
 *******************************************************************************/
LOCAL void iAP2FileXferInitializeHandle(U8 fileTransferId, iAP2FileTransfer_t* iAP2FileXrHdl, iAP2Device_st* this_iAP2Device)
{
    iAP2FileTransferSession_t* iAP2FileXferSession = this_iAP2Device->iAP2FileTransfer_list;

    /* Check whether the ID is already present in the list */
    while(iAP2FileXferSession != NULL)
    {
        if(fileTransferId == iAP2FileXferSession->iAP2FileTransferID)
        {
            iAP2FileXferSession->iAP2FileXferHdl = iAP2FileXrHdl;
            break;
        }
        iAP2FileXferSession = iAP2FileXferSession->NextTransfer;
    }
}
