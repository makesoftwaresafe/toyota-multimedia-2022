
/**
 * \file: iap_storage.c
 *
 */

#include <adit_typedef.h>
#include <string.h>
#include <stdlib.h>
#include "iap_storage.h"
#include "iap_util_func.h"
#include "iap_types.h"
#include "ipodcommon.h"
#include "iap_commands.h"
#include "iap_general.h"
#include "iap_init.h"
#include "iap_transport_message.h"
#include "iap1_dlt_log.h"


/**
 * \addtogroup Storage_commands
 */
/*\{*/

/*!
 * \fn iPodGetiPodCaps(U32 iPodID, IPOD_STORAGE_CAPS *storageCaps)
 * \par STORAGE LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * #IPOD_STORAGE_CAPS storageCaps* - the storage information
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the capability of storage.
 * When this function failed, inout parameter doesn't know what there is entering.
 * The iPod tells the information about the iPod's storage capabilities in the following.
 * \li \c \b totalSpace - is the amount of storage on the iPod in bytes.
 * \li \c \b maxFileSize - is the largest possible size in bytes.
 * \li \c \b maxWriteSize - is the largest amount of data in bytes.
 * \li \c \b majorVersion - is the version number of the Storage Lingo Protocol.
 * \li \c \b minorVersion - is the version number of the Storage Lingo Protocol.<br>
 * \note iPod touch 2G(OS2.0)does not work.
 */
S32 iPodGetiPodCaps(U32 iPodID, IPOD_STORAGE_CAPS *storageCaps)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_CAPS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl == NULL) || (storageCaps == NULL))
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPodHndl = %p storageCaps = %p",iPodHndl,storageCaps);
    }
    else
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_STORAGE_LINGO_RET_IPOD_CAPS,
                                     (U8)IPOD_LINGO_STORAGE);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    /* Set the iPod capabilities */
                    storageCaps->totalSpace = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    storageCaps->maxFileSize = iPod_convert_to_little32(&iPodHndl->iAP1Buf[IPOD_POS8]);
                    storageCaps->maxWriteSize = iPod_convert_to_little16(&iPodHndl->iAP1Buf[IPOD_POS12]);
                    storageCaps->majorVersion = iPodHndl->iAP1Buf[IPOD_POS20];
                    storageCaps->minorVersion = iPodHndl->iAP1Buf[IPOD_POS21];
                    IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "totalSpace = %llu maxFileSize = %u maxWriteSIze = %u majorVersion = %u minorVersion = %u",
                    storageCaps->totalSpace,storageCaps->maxFileSize,storageCaps->maxWriteSize,storageCaps->majorVersion,storageCaps->minorVersion);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodWriteiPodFileData(U32 iPodID, U32 offset, U8 handle, const U8* data, U16 length)
 * \par STORAGE LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 offset - the file at which to begin writing<br>
 * U8 handle - is a unique file identifier<br>
 * U16 length - is a data length
 * \par INOUT PARAMETERS
 * U8 data* - is the data to be written to the file
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function writes a block of data to a file.
 * The file must have been previously opened for writing.
 * All data must be written sequentially.<br>
 * The amount of data transferred must also be within the bounds set by the iPod's capabilities.<br>
 * If write size exceeds the iPod's capabilities, this function will fail.
 */
S32 iPodWriteiPodFileData(U32 iPodID, U32 offset, U8 handle, const U8* data, U16 length)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U16 totalLength = IPOD_WRITE_BASE_LENGTH + length;
    U8 idps = FALSE;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (data != NULL) && (length > 0))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "offset = %u handle = %u",offset,handle);
            msg = (U8 *)calloc((totalLength + IPOD_START_LENGTH + IPOD_WRITE_SIZE_LONG), sizeof(U8));
            if(msg == NULL)
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_STORAGE_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");
            }
            else
            {
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_STORAGE_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_STORAGE);
                
                idps = iPodHndl->rcvMsgInfo.startIDPS;
                /* Check whether length is greater than 256 */
                if(((totalLength < IPOD_SHORT_WRITE_SIZE) && (idps == FALSE)) || 
                    (((totalLength + IPOD_TRANSID_LENGTH) < IPOD_SHORT_WRITE_SIZE) && (idps != FALSE)))
                {
                    msg[IPOD_POS1] = (U8)totalLength;
                    msg[IPOD_POS2] = (U8)IPOD_LINGO_STORAGE;
                    msg[IPOD_POS3] = IPOD_STORAGE_LINGO_WRITE_IPOD_FILE_DATA;
                    iPod_convert_to_big32(&msg[IPOD_POS4], offset);
                    msg[IPOD_POS8] = handle;
                    memcpy(&msg[IPOD_POS9], data, length);
                    rc = iPodSendCommand(iPodHndl, msg);
                }
                else
                {
                    msg[IPOD_POS1] = 0;
                    iPod_convert_to_big16(&msg[IPOD_POS2], totalLength);
                    msg[IPOD_POS4] = (U8)IPOD_LINGO_STORAGE;
                    msg[IPOD_POS5] = IPOD_STORAGE_LINGO_WRITE_IPOD_FILE_DATA;
                    iPod_convert_to_big32(&msg[IPOD_POS6], offset);
                    msg[IPOD_POS10] = handle;
                    memcpy(&msg[IPOD_POS11], data,length);
                    rc = iPodSendLongTelegram(iPodHndl, msg);
                }

                if(rc == IPOD_OK)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                }

                free(msg);
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");

        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p data = %p length = %d",iPodHndl,data,length);
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodCloseiPodFile(U32 iPodID, U8 handle)
 * \par STORAGE LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 handle - is a unique file identifire
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function closes a file and releases its handle.<br>
 * The handle is invalid for further use after this call.<br>
 * iPod may assign the handle later to represent a different file.
 */
S32 iPodCloseiPodFile(U32 iPodID, U8 handle)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_CLOSE_IPOD_FILE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            if(handle != IPOD_NOT_APPLICABLE)
            {
                IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "file descriptor of iPod File to be closed = %d",handle);
                msg[IPOD_POS4] = handle;
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_STORAGE_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_STORAGE);
                rc = iPodSendCommand(iPodHndl, msg);
                if(rc == IPOD_OK)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                }
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_STORAGE_LOG(DLT_LOG_ERROR, "Bad Parameter handle = %d",handle);
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetiPodFreeSpace(U32 iPodID, U64 *freeSpace)
 * \par STORAGE LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * U64 freeSpace* - is amout of the iPod's free space in bytes.
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the current amount of free space in its storage system.
 * When this function failed, inout parameter doesn't know what there is entering.
 */
S32 iPodGetiPodFreeSpace(U32 iPodID, U64 *freeSpace)
{
    S32 rc = IPOD_OK;
    U8 msg[] = {IPOD_GET_IPOD_FREESPACE_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (freeSpace != NULL))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_STORAGE_LINGO_RET_IPOD_FREE_SPACE,
                                     (U8)IPOD_LINGO_STORAGE);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                if(rc == IPOD_OK)
                {
                    *freeSpace = iPod_convert_to_little64(iPodHndl->iAP1Buf);
                    IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "freespace = %llu",*freeSpace);
                }
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "Bad parameter - iPodHndl = %p freeSpace = %p",iPodHndl,freeSpace);
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodOpeniPodFeatureFile(U32 iPodID, IPOD_FEATURE_TYPE featureType, IPOD_FILE_OPTIONS_MASK *bitMask, const U8* fileData, U8 fileSize, U8 *fileHandle)
 * \par STORAGE LINGO PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_FEATURE_TYPE featureType - is current possible featureType<br>
 * #IPOD_FILE_OPTIONS_MASK bitMask* - is current possible fileOptionsMask<br>
 * U8 fileData* - This data is appended to the end of the file when the file is closed. maximum 128 bytes<br>
 * U8 fileDataSize - size of file data to be appecded to the file when it is closed.
 * \par INOUT PARAMETERS
 * U8 fileHandle* - is feature file handle
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function open a feature file on the iPod.<br>
 * If this command is failed and fileHandle is NOT NULL, fileHandle is 0xFF.<br>
 * If bitmask bit0 is 1, fileSize must be NON zero .<br>
 * If bitmask bit0 is 0, fileSize must be zero.<br>
 * If bitmask bit1 is 1, append the XML ipodInfo element to the file. When this bit3 set, bit1 must be set.<br>
 * If bitmask bit3 is 1, insert an XML Signature element to the file.
 */
S32 iPodOpeniPodFeatureFile(U32 iPodID, IPOD_FEATURE_TYPE featureType, IPOD_FILE_OPTIONS_MASK *bitMask, const U8* fileData, U8 fileSize, U8* fileHandle)
{
    S32 rc = IPOD_OK;
    U8 *msg = NULL;
    U8 baseLength = IPOD_OPEN_BASE_LENGTH;
    U32 mask = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (fileHandle != NULL) && (bitMask != NULL) && (fileSize <= IPOD_MAX_FILE_SIZE))
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            memcpy(&mask, bitMask, sizeof(U32));
            IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "Feature Type = 0x%02x File Options Mask = 0x%04x",featureType,mask);
            if(mask != 0)
            {
                baseLength += IPOD_FILE_OPTION_LENGTH;
            }
            msg = (U8 *)calloc((baseLength + fileSize + IPOD_START_LENGTH), sizeof(U8));
            if(msg != NULL)
            {
                /* Set message data */
                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_STORAGE_LINGO_IPOD_RET_IPOD_FILE_HANDLE, (U8)IPOD_LINGO_STORAGE);
                msg[IPOD_POS0] = IPOD_START_OF_PACKET;
                msg[IPOD_POS1] = baseLength + fileSize;
                msg[IPOD_POS2] = (U8)IPOD_LINGO_STORAGE;
                msg[IPOD_POS3] = IPOD_STORAGE_LINGO_OPEN_IPOD_FEATURE_FILE;
                msg[IPOD_POS4] = (U8)featureType;
                /* if mask not set, do not set mask value in command */
                if(mask != 0)
                {
                    iPod_convert_to_big32(&msg[IPOD_POS5], mask);
                    /* Check the bit to write the fileData */
                    if((msg[IPOD_POS8] & IPOD_POS1) == TRUE)
                    {
                        /* Check the parameter to write the fileData */
                        if((fileData != NULL) && (fileSize > 0))
                        {
                            memcpy(&msg[IPOD_POS9], fileData, fileSize);
                        }
                        else
                        {
                            rc = IPOD_BAD_PARAMETER;
                            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "Bad parameter - fileData = %p fileSize = %d",fileData,fileSize);
                        }
                    }
                }
                if(rc == IPOD_OK)
                {
                    rc = iPodSendCommand(iPodHndl, msg);
                }

                if(rc == IPOD_OK)
                {
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                    if(rc == IPOD_OK)
                    {
                        *fileHandle = iPodHndl->iAP1Buf[IPOD_POS0];
                    }
                    else
                    {
                        *fileHandle = IPOD_NOT_APPLICABLE;
                    }
                }

                free(msg);
            }
            else
            {
                rc = IPOD_ERR_NOMEM;
                IAP1_STORAGE_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");
                *fileHandle = IPOD_NOT_APPLICABLE;
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p fileHandle = %p bitMask = %p fileSize = %d",
                                              iPodHndl,fileHandle,bitMask,fileSize);
        if(fileHandle != NULL)
        {
            *fileHandle = IPOD_NOT_APPLICABLE;
        }
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}
/*\}*/

/*!
 * \fn iPodDeviceACK(U32 iPodID, U8 command, U8 status)
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 command - This is command for ack.<br>
 * U8 status - This is ack status for command.
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function reply command of iPod.
 */
S32 iPodDeviceACK(U32 iPodID, U8 command, U8 status)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_DEVICE_ACK_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            IAP1_STORAGE_LOG(DLT_LOG_VERBOSE, "status = 0x%02x command = %u",status,command);
            msg[IPOD_POS4] = status;
            msg[IPOD_POS5] = command;
            rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }

    return rc;
}

/*!
 * \fn iPodRetDeviceCaps(U32 iPodID, U8 major, U8 minor)
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8 major - This is major version of device.<br>
 * U8 minor - This is minor version of device.
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Completed successfully
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Wait released by wait disabled state
 * \li \c \b #IPOD_ERR_TMOUT timeout error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function sends the device storage capabilities to iPod.
 */
S32 iPodRetDeviceCaps(U32 iPodID, U8 major_ver, U8 minor_ver)
{
    S32 rc = IPOD_ERROR;
    U8 msg[] = {IPOD_RET_DEVICE_CAPS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl != NULL)
    {
        if(iPodHndl->isAPIReady != FALSE)
        {
            if(major_ver == 0)
            {
                major_ver = IPOD_DEFAULT_MAJOR;
            }

            if(minor_ver == 0)
            {
                minor_ver = IPOD_DEFAULT_MINOR;
            }

            IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "major_ver = %d minor_ver = %d",major_ver,minor_ver);
            msg[IPOD_POS24] = major_ver;
            msg[IPOD_POS25] = minor_ver;
            rc = iPodSendCommandNoWaitForACK(iPodHndl, msg);
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_STORAGE_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
    IAP1_STORAGE_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}
