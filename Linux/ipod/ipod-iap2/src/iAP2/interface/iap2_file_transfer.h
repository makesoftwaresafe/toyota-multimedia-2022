/************************************************************************
 * \file: iap2_file_transfer.h
 *
 * \version: $ $
 *
 * This header file declares functions required for file transfer.
 *
 * \component: global definition file
 *
 * \author: Sudha Kuppusamy/Bosch/ sudha.kuppusamy@in.bosch.com
 *
 * \copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/
#ifndef IAP2_FILE_TRANSFER_H
#define IAP2_FILE_TRANSFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init.h"

#define IAP2FTMESSAGE_HDRSIZE (sizeof(iAP2FTMessage_t)-MAX_MESSAGE_SIZE)

struct iAP2FileTransferDatagram_st
{
    union iAP2FileTypeSetupData_u
    {
        /*! Call Unique Identifier of CallStateUpdate */
        U8* iAP2CallUUID;
        U8* iAP2AppBundleID;

        struct iAP2MediaLibraryUpdatePlaylistContents_st
        {
            /*! Playlist Persistent Identifier of MediaLibraryUpdate */
            U64 iAP2PlaylistPID;

            /*! Library UID of MediaLibraryUpdate */
            U8* iAP2LibraryUID;
        }iAP2MediaLibraryUpdatePlaylistContents;
    }iAP2FileTypeSetupData;

    /*! Type of the file that is mentioned in Setup Datagram File Types */
    U16 iAP2FileType;

    /*! Total size of the datagram parameters sent, i.e. net size of the union iAP2FileTypeSetupData_u */
    U16 iAP2FTDatagramParamSize;
};

/* File transfer session datagram object
*
*/
typedef struct iAP2FileTransferDatagram_st iAP2FileTransferDatagram_t;


/**
 * \brief File Transfer session List.
 *
 * Holds information for each file transfer such as FileTransferID,
 * receive buffer and and pointer to file transfer handle.
 */
struct iAP2FileTransferSession_st
{
    /**
     * File transfer Identifier received from device
     */

    U8                  iAP2FileTransferID;
    /**
     * iAP2FileXferRxBuf buffer for session layer to store file transfer data.
     * This will be currently allocated in the session layer based on
     * transfer mode. Application will copy file transfer data from this buffer
     */

    U8*                 iAP2FileXferRxBuf;
    /**
     * Total received length
     */
    U64                 iAP2FileXferRxLen;

    /**
     * Holds information for each file transfer such as file transfer state,
     * transfer mode, File transfer ID, file transfer buffer and so on.
     * It is internal to session layer.
     */
    void*               iAP2FileXferHdl;

    /**
     * Pointer to next file transfer session
     */
    struct iAP2FileTransferSession_st* NextTransfer;
};

  /**
   * File Transfer session object
   */
  typedef struct iAP2FileTransferSession_st iAP2FileTransferSession_t;

/**
 * \addtogroup FileTransferSessionAPIs
 * @{
 */

/**************************************************************************//**
 * Cancel File Transfer.
 *
 * This would send Cancel datagram to device to stop the file transfer. Once
 * the file transfer is stopped, it cannot be resumed. So, iAP2 Library will
 * delete the file transfer session object of the corresponding file
 * transfer ID from File transfer session list
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] FileTransferID File transfer Identifier of the file transfer
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2CancelFileTransfer(iAP2Device_t* this_iAP2Device, U8 FileTransferID);

/**************************************************************************//**
 * Pause File Transfer.
 *
 * This would send Pause datagram to device to pause the file transfer.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] FileTransferID File transfer Identifier of the file transfer
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2PauseFileTransfer(iAP2Device_t* this_iAP2Device, U8 FileTransferID);

/**************************************************************************//**
 * Resume File Transfer.
 *
 * This would send start datagram to device to resume the file transfer which
 * is already paused.
 *
 * \param[in] this_iAP2Device Initialized Device structure
 * \param[in] FileTransferID File transfer Identifier of the file transfer
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2ResumeFileTransfer(iAP2Device_t* this_iAP2Device, U8 FileTransferID);

/***************************************************************************//**
 * Parses file transfer data received from device and responds accordingly
 *
 * \param[in]  this_iAP2Device Structure which contains information about the device
 *                              connected to the target.
 * \param[in]  Sourcebuf       data buffer from link layer
 * \param[in]  SourceBufSize   Size of data buffer from link layer
 * \return     IAP2_OK         On Successful completion of handling datagram
 * \return     IAP2_CTL_ERROR  On receiving incomplete header or invalid command
 * \see
 * \note
 *******************************************************************************/
S32 iAP2ServiceParseFileTransferMessage(iAP2Device_t* this_iAP2Device, U8* Sourcebuf, const U32 SourceBufSize);

/***************************************************************************//**
 * This function is called to send a file from accessory to Apple Device
 *
 * \param[in]  this_iAP2Device Structure which contains information about the device
 *                              connected to the target.
 * \param[in]  Sourcebuf       Buffer which has to be sent to the Apple Device
 * \param[in]  length          Length of the Sourcebuf
 * \param[in]  iAP2FileTransferDatagram
 *                             File transfer setup datagram structure
 * \return None
 * \see
 * \note
 *******************************************************************************/
void iAP2FiletransferSendData(iAP2Device_t* this_iAP2Device, void* Sourcebuf, U32 length, iAP2FileTransferDatagram_t* iAP2FileTransferDatagram);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
