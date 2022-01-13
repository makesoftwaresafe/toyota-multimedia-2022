/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**
* \file: iap_display.c
*
*
***************************************************************************** */

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <string.h> /* needed to avoid 'implicit declaration of memcopy ...' compiler warning */
#include <arpa/inet.h>

#include "iap_init.h"
#include "ipodcommon.h"
#include "iap_display.h"
#include "iap_commands.h"
#include "iap_types.h"
#include "iap_general.h"
#include "iap_callback.h"
#include "iap_transport_message.h"
#include "iap_util_func.h"
#include "iap_callback.h"
#include "iap1_dlt_log.h"

/**
 * \addtogroup Display_commands
 */
/*\{*/


/*!
 * \fn iPodGetArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf, U16* resultCount)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.10
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * #IPOD_ARTWORK_FORMAT resultBuffer* - pointer to an IPOD_ARTWORK_FORMAT structure where the result will be stored in.
 * U16 resultCount* - number of IPOD_ARTWORK_FORMAT structs in the result buffer
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_COMMANDS_NOT_SUPPORTED Protocol version is not supported this function
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the list of supported artwork formats on the iPod. It is required to
 * use it, if you wish to transfer Album Artwork from the iPod. Each iPod model may support a
 * different range of formats, so it is advisable to check it with each iPod.<br>
 * Based on the resolution and pixel format, choose a format which best suits your
 * output format and use the formatID of that format when you call #iPodGetTrackArtworkData.
 * \note Please make sure that enough memory was allocated for the resultBuffer. There may be more than
 * one artwork format supported by the iPod, so it is advisable to allocate 256 Bytes of memory.
 * Each artwork format is described in a 7-byte structure.
 * Because of alignment, 32 artwork formats fits into the allocated result buffer.
 */
S32 iPodGetArtworkFormats(U32 iPodID, IPOD_ARTWORK_FORMAT* resultBuf,
                          U16* resultCount)
{
    U16 i               =  0;
    S32 responseMsgLen  =  0;
    U16 curIdx          =  0;
    U8* responseMsg     =  NULL;
    U8  msg[]           = {IPOD_GET_ARTWORK_FORMATS_CMD};
    S32 rc              =  IPOD_OK;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((resultBuf == NULL) || (resultCount == NULL))
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter: resultBuf = %p,resultCount = %p ",resultBuf,resultCount);
    }
    else
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_ARTWORKFORMATS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
        }
        if (rc == IPOD_OK)
        {
            responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);

            if (responseMsgLen > IPOD_POS6)
            {
                responseMsg = (U8*) malloc((U32)responseMsgLen);
                if (responseMsg != NULL)
                {
                    iPodGetResponseData(iPodHndl, responseMsg);

                    if(responseMsgLen > (IPOD_MAX_ARTWORK_FORMATS * IPOD_ARTWORK_FORMAT_SIZE))
                    {
                        responseMsgLen = (S32)(IPOD_MAX_ARTWORK_FORMATS * IPOD_ARTWORK_FORMAT_SIZE);
                    }

                    *resultCount = (U16)(responseMsgLen / IPOD_ARTWORK_FORMAT_SIZE);

                    for (i = 0; i < responseMsgLen; i += IPOD_ARTWORK_FORMAT_SIZE)
                    {
                        resultBuf[curIdx].formatID = iPod_convert_to_little16(&responseMsg[i]);
                        resultBuf[curIdx].imageWidth = iPod_convert_to_little16(&responseMsg[i + IPOD_POS3]);
                        resultBuf[curIdx].imageHeight = iPod_convert_to_little16(&responseMsg[i + IPOD_POS5]);
                        resultBuf[curIdx].pixelFormat = responseMsg[i + IPOD_POS2];
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "formatID = %u, imageWidth = %u, imageHeight = %u pixelFormat = 0x%02x",
                                             resultBuf[curIdx].formatID,resultBuf[curIdx].imageWidth,resultBuf[curIdx].imageHeight,resultBuf[curIdx].pixelFormat);
                        curIdx++;
                    }
                    free(responseMsg);
                    rc = 0;
                }
                else
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory responseMsg is NULL");
                }
            }
            else
            {
                *resultCount = 0;
                rc = IPOD_OK;
                IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "responseMsgLen = %d",responseMsgLen);
            }
        }

        if(rc != IPOD_OK)
        {
           *resultCount = 0;
            resultBuf   = 0;
        }
    }

    return rc;
}


/*!
 * \fn iPodGetMonoDisplayImageLimits(U32 iPodID, U16* width, U16* height, U8* pixelFormat)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device
 * \par INOUT PARAMETERS
 * S16* width - max. width in pixels <br>
 * S16* height - max. height in pixels <br>
 * U8* pixelFormat - display pixel format <br>
 * Monochrome, 2bits per pixel = 0x01 <br>
 * RGB 565 color little-endian, 16pp = 0x02 <br>
 * RGB 565 color big-endian, 16pp = 0x03 <br>
 * \par REPLY PARAMETERS
 * S32 result 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the limiting characteristics of the monochrome image that can be sent to the
 * iPod for display while it is connected to the device.
 */
S32 iPodGetMonoDisplayImageLimits(U32 iPodID, U16* width,
                                  U16* height,
                                  U8* pixelFormat)
{
    S32 rc     =  IPOD_OK;
    U8  msg[]  =  {IPOD_GET_MONO_DISPLAY_LIMITS_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((width == NULL) || (height == NULL) || (pixelFormat == NULL))
    {
            rc = IPOD_BAD_PARAMETER;
            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter width = %p, height = %p, pixelFormat = %p",width,height,pixelFormat);
    }
    else
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if(rc == IPOD_OK)
        {
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_MONO_DISPLAY_IMAGE_LIMITS,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
        }
        if (rc == IPOD_OK)
        {
            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
            rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);


            if (rc == IPOD_OK)
            {
                *width = iPod_convert_to_little16(&iPodHndl->iAP1Buf[IPOD_POS0]);
                *height = iPod_convert_to_little16(&iPodHndl->iAP1Buf[IPOD_POS2]);
                *pixelFormat = iPodHndl->iAP1Buf[IPOD_POS4];
                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "width = %u, height = %u, pixelFormat = 0x%02x",*width,*height,*pixelFormat);
            }
        }

        if (rc != 0)
        {
            *width        =  0;
            *height       =  0;
            *pixelFormat  =  0;
        }
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetColorDisplayImageLimits(U32 iPodID, IPOD_DISPLAY_IMAGE_LIMITS* resultBuf, U16* resultCount)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.09
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * \par INOUT PARAMETERS
 * #IPOD_DISPLAY_IMAGE_LIMITS *resultBuf - pointer to a buffer where the results will be stored in <br>
 * U16* resultCount - number of IPOD_DISPLAY_IMAGE_LIMITS structs in the result buffer
 * \par REPLY PARAMETERS
 * S32 ReturnCode - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the limiting characteristics of the color image that can be sent to the
 * iPod for display while it is connected to the device. Since the number of IPOD_DISPLAY_IMAGE_LIMITS
 * returned by the iPod is not limited, the correct way to use this function is by the following sequence:
 * <br>- Call iPodGetColorDisplayImageLimits with resultBuf == NULL to get just the resultCount.
 * <br>- Allocate memory for resultCount * sizeof(IPOD_DISPLAY_IMAGE_LIMITS).
 * <br>- Call iPodGetColorDisplayImageLimits again with resultBuf pointing to the allocated memory area from
 * the second step.
 */
S32 iPodGetColorDisplayImageLimits(U32 iPodID, IPOD_DISPLAY_IMAGE_LIMITS* resultBuf,
                                   U16* resultCount)
{
    U16 i               =   0;
    S32 responseMsgLen  =   0;
    U16 curIdx          =   0;
    U8* responseMsg     =   NULL;
    U8  msg[]           =   {IPOD_GET_COLOR_DISPLAY_LIMITS_CMD};
    S32 rc              =   IPOD_OK;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    rc = iPodIsInAdvancedMode(iPodHndl);
    if(rc == IPOD_OK)
    {
        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_COLOR_DISPLAY_IMAGE_LIMITS,
                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
        rc = iPodSendCommand(iPodHndl, msg);
    }

    if(resultCount != NULL)
    {
        if (rc == IPOD_OK)
        {
            responseMsgLen = iPodWaitAndGetResponseLength(iPodHndl);
            if (responseMsgLen > IPOD_POS4)
            {
                responseMsg = (U8*) malloc((U32)responseMsgLen);

                if (responseMsg != NULL)
                {
                    iPodGetResponseData(iPodHndl, responseMsg);
                    *resultCount = (U16)(responseMsgLen / IPOD_DISPLAY_IMG_LIMIT_SIZE);
                    if(resultBuf != NULL)
                    {
                        for (i = 0; i < responseMsgLen; i += IPOD_DISPLAY_IMG_LIMIT_SIZE)
                        {
                            resultBuf[curIdx].width = iPod_convert_to_little16(&responseMsg[i]);
                            resultBuf[curIdx].height = iPod_convert_to_little16(&responseMsg[i + IPOD_POS2]);
                            resultBuf[curIdx].pixelFormat = responseMsg[i + IPOD_POS4];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "width = %u, height = %u, pixelFormat = 0x%02x",resultBuf[curIdx].width,
                                                                   resultBuf[curIdx].height,resultBuf[curIdx].pixelFormat);
                            curIdx++;
                        }
                    }
                    
                    rc = IPOD_OK;
                    free(responseMsg);
                 }
                 else
                 {
                     rc = IPOD_ERR_NOMEM;
                     IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory responseMsg is NULL");
                 }
            }
            else
            {
                rc = responseMsgLen;
            }
        }

        if(rc != IPOD_OK)
        {
           *resultCount  =  0;
            resultBuf    =  0;
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter resultCount is NULL");
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetDisplayImage(U32 iPodID, const U8* image, IPOD_IMAGE_TYPE imageType)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * const U8* image - the name of the image including the path. e.g. /host/images/aditLogo.img<br>
 * #IPOD_IMAGE_TYPE imageType - the type of image to upload possible values
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function uploads a new image to the iPod. The new image replaces the extended interface mode's
 * standard "connected" image.
 * Calls made to SetDisplayImage more frequently than every 15 seconds will return a successful ACK command,
 * but the bitmap will not be displayed on the iPod's screen. Hence use of the SetDisplayImage command
 * should be limited to drawing one bitmap image per accessory connect.
 */

S32 iPodSetDisplayImage(U32 iPodID, const U8* image,
                        IPOD_IMAGE_TYPE imageType)
{
    S32 result          =   IPOD_OK;
    U8  msg1[]          =   {IPOD_SET_DISPLAY_IMAGE_DESCR_CMD};
    U8  msg2[]          =   {IPOD_SET_DISPLAY_IMAGE_DATA_CMD};
    U8* imageDataBuf    =   NULL;
    FILE* pic           =   NULL;
    U32 imageWidth      =   0;
    U32 imageHeight     =   0;
    U32 rowSize         =   0;
    U32 stuffBytes      =   0;
    U16 rowCounter      =   1;
    U8  c;
    U64 temp = 0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    size_t nread = 0;

    if((iPodHndl != NULL) && (image != NULL))
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            pic = fopen((VP)image,
                        IPOD_IMG_UPLOAD_FILE_OPEN_MODE);

            if (pic != NULL)
            {
                nread = fread(&imageWidth,
                      IPOD_IMG_UPLOAD_WIDTH_SIZE,
                      IPOD_READ_DATA_NUMBER,
                      pic);
                if(nread == IPOD_READ_DATA_NUMBER)
                {
                    /* Change the network byte order to host byte order */
                    imageWidth = ntohl(imageWidth);
                    /* width must be less than U16 */
                    if(imageWidth > U16_MAX)
                {
                    result = IPOD_ERROR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - imageWidth = %d, it should not be > U16",imageWidth);
                }
                }
                else
                {
                    result = IPOD_ERROR;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - nread= %zu, it should be 1", nread);
                }

                if (result == IPOD_OK)
                {
                    nread = fread(&imageHeight,
                          IPOD_IMG_UPLOAD_HEIGHT_SIZE,
                          IPOD_READ_DATA_NUMBER,
                          pic);
                    if(nread == IPOD_READ_DATA_NUMBER)
                    {
                        /* Change the network byte order to host byte order */
                        imageHeight = ntohl(imageHeight);
                        if(imageHeight > U16_MAX)
                        {
                            result = IPOD_ERROR;
                            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - imageHeight = %d, it should not be > U16",imageHeight);
                        }
                    }
                    else
                    {
                        result = IPOD_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - nread= %zu, it should be 1",nread);
                    }
                }
                if (result == IPOD_OK)
                {
                    nread = fread(&temp, IPOD_BITS_PER_BYTE, IPOD_READ_DATA_NUMBER, pic);
                    if (nread != IPOD_READ_DATA_NUMBER)
                    {
                        result = IPOD_ERROR;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - nread= %zu, it should be 1",nread);
                    }
                }
                if (result == IPOD_OK)
                {
                    stuffBytes = ((imageWidth * IPOD_IMAGE_BYTE_ALIGN) % IPOD_IMG_UPLOAD_BYTE_ALIGN);
                    rowSize = ((imageWidth * IPOD_IMAGE_BYTE_ALIGN) + stuffBytes);

                    /* 1. setup and send the descriptor ============================= */
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Display Pixel format = 0x%02x imageWidth = %u, imageHeight = %u, rowSize = %u",
                                                                                   imageType,imageWidth,imageHeight,rowSize);

                    /* insert pixel format into msg array */
                    msg1[IPOD_POS9] = (U8)imageType;

                    /* insert image width into msg */
                    iPod_convert_to_big16(&msg1[IPOD_POS10], (U16)imageWidth);
                    /* insert image height into msg */
                    iPod_convert_to_big16(&msg1[IPOD_POS12], (U16)imageHeight);
                    /* insert row size (stride) into msg */
                    iPod_convert_to_big32(&msg1[IPOD_POS14], rowSize);

                    iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                             (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                    iPodSendLongTelegram(iPodHndl, msg1);
                    memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                    result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);//iPodGetResponseMsg(iPodHndl->iAP1Buf);
                    if (result == IPOD_OK)
                    {
                        /* 2. setup and send the image data ============================= */
                        imageDataBuf = (U8*) malloc(rowSize + sizeof(msg2));
                        if (imageDataBuf != NULL)
                        {
                            for (c = 0; c < imageHeight; c++)
                            {
                                memset(imageDataBuf, 0, rowSize + sizeof(msg2));

                                /* The max. payload length is limited to 500 bytes, so we have to split */
                                /* the telegram for long rows. Lets take 240 pixel (480 bytes) as max.  */
                                /* pixel per telegram package                                           */

                                if (rowSize <= IPOD_MAX_BYTES_PER_ROW_BLOCK)
                                {
                                    /* insert payload length into msg */
                                    iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + rowSize));
                                    /* insert telegram index into msg */
                                    iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                                    rowCounter++;

                                    /* copy header and image data */
                                    memcpy(imageDataBuf,
                                           msg2,
                                           sizeof(msg2));

                                    nread = fread(&imageDataBuf[sizeof(msg2)],
                                          1,
                                          (imageWidth * IPOD_IMAGE_BYTE_ALIGN),
                                          pic);
                                    if (nread != (imageWidth * IPOD_IMAGE_BYTE_ALIGN))
                                    {
                                        result = IPOD_ERROR;
                                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - nread = %zu",nread);
                                    }
                                    else
                                    {
                                        iPodSetExpectedCmdId(iPodHndl, (U8)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                                 (U16)IPOD_LINGO_EXTENDED_INTERFACE);
                                        iPodSendLongTelegram(iPodHndl, imageDataBuf);

                                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                    }

                                    if (result == 0)
                                    {
                                        result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    /* handle rows longer than 240 pixel ... */

                                    U32 numFullRowBlocks = 0;
                                    U32 numRowBlocks = 0;
                                    U32 lastRowBlockSize = 0;
                                    U32 n = 0;
                                    U32 tmpRowSize = IPOD_MAX_BYTES_PER_ROW_BLOCK;

                                    numFullRowBlocks = (U32) (rowSize / IPOD_MAX_BYTES_PER_ROW_BLOCK);

                                    if ((rowSize % IPOD_MAX_BYTES_PER_ROW_BLOCK) != 0)
                                    {
                                        numRowBlocks = (numFullRowBlocks + 1);
                                    }

                                    lastRowBlockSize = (rowSize - (IPOD_MAX_BYTES_PER_ROW_BLOCK * numFullRowBlocks));

                                    for (n = 1; n <= numRowBlocks; n++)
                                    {
                                        /* adjust size for the last block */
                                        if (n == numRowBlocks)
                                        {
                                            tmpRowSize = lastRowBlockSize;
                                        }

                                        /* insert payload length into msg */
                                        /* 5 additional bytes because of Lingo, Command and Descriptor index */
                                        iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + tmpRowSize));

                                        /* insert telegram index into msg */
                                        iPod_convert_to_big16(&msg2[IPOD_POS7], rowCounter);
                                        rowCounter++;

                                        /* copy header and image data */
                                        memcpy(imageDataBuf,
                                               msg2,
                                               sizeof(msg2));

                                        nread = fread(&imageDataBuf[sizeof(msg2)],
                                              1,
                                              tmpRowSize,
                                              pic);
                                        if(nread != tmpRowSize)
                                        {
                                            result = IPOD_ERROR;
                                            IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error - nread = %zu",nread);
                                        }
                                        else
                                        {
                                            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                                            iPodSendLongTelegram(iPodHndl, imageDataBuf);

                                            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                                            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
                                        }

                                        if (result == 0)
                                        {
                                            result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                }

                                if (result != 0)
                                {
                                    break;
                                }
                            }

                            free(imageDataBuf);
                        }
                    }
                }
                fclose(pic);
            }
        }
    }
    else
    {
        result = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter: iPodHndl = %p, image = %p",iPodHndl,image);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}

/*!
 * \fn iPodGetTrackArtworkData(U32 iPodID, U32 trackIndex, U16 formatId, U32 timeOffset, const IPOD_CB_GET_ARTWORK callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.10
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - specifies which track from the Playback Engine is to be selected <br>
 * S16 formatId - specifies which type of artwork is desired. <br>
 * U32 timeOffset - specifies the time offset from track in ms <br>
 * #IPOD_CB_GET_ARTWORK callback - pointer to callback function
 * \par INOUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the artwork data specified by trackIndex, formatId and timeOffset.
 * Depending on the size of the artwork data, the specified callback function might be called
 * multiple times. The values for trackIndex, formatId and timeOffset must have valid values.
 * Otherwise the function will fail. The valid values for formatID and timeOffset
 * can be obtained by the functions #iPodGetArtworkFormats.
 */
S32 iPodGetTrackArtworkData(U32 iPodID,
                            U32 trackIndex,
                            U16 formatId,
                            U32 timeOffset,
                            const IPOD_CB_GET_ARTWORK callback)
{
    S32 rc = -1;
    U8  msg[] = {IPOD_GET_TRACK_ARTWORK_DATA_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (callback != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if (rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "trackIndex = %d formatId = %d timeOffset = %d Transaction Id = %u",trackIndex,formatId,timeOffset,iPodHndl->rcvMsgInfo.accTransID);
            /* save Artwork Transaktion ID */
            iPodHndl->artwork.transID = iPodHndl->rcvMsgInfo.accTransID;
            /* track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], trackIndex);
            /* insert format id into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS9], formatId);
            /* insert time offstet into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS11], timeOffset);
            /* save the pointer to the callback function */
            iPodRegisterCBTrackArtworkData(callback);

            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_TRACKARTWORKDATA, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p, callback = %p",iPodHndl,callback);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetTypeOfTrackArtworkData(U32 iPodID, IPOD_TRACK_TYPE type,U32 trackIndex, U16 formatId, U32 timeOffset, const IPOD_CB_GET_ARTWORK callback)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.10
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_TRACK_TYPE type - track identifier type<br>
 * U32 trackIndex - specifies which track from the Playback Engine is to be selected <br>
 * S16 formatId - specifies which type of artwork is desired. <br>
 * U32 timeOffset - specifies the time offset from track in ms <br>
 * #IPOD_CB_GET_ARTWORK callback - pointer to callback function
 * \par INOUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function returns the artwork data for a given track specified by trackIndex, formatId and timeOffset.
 * Depending on the track identifier type, the trackIndex its the UID of the track,
 * or its index in the playback engine or the index in the database list.
 * Depending on the size of the artwork data, the specified callback function might be called
 * multiple times. The values for trackIndex, formatId and timeOffset must have valid values.
 * Otherwise the function will fail. The valid values for formatID and timeOffset
 * can be obtained by the functions #iPodGetArtworkFormats.
 */
S32 iPodGetTypeOfTrackArtworkData(U32 iPodID,
                                  IPOD_TRACK_TYPE type, U64 trackIndex,
                                  U16 formatId,
                                  U32 timeOffset,
                                  const IPOD_CB_GET_ARTWORK callback)
{
    S32 rc = -1;
    U8  msg[] = {IPOD_GET_TYPE_OF_TRACK_ARTWORK_DATA_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    U8 msgLen = 14;
    
    if((iPodHndl != NULL) && (callback != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if (rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "track type = 0x%02x,track identifier = %llu formatId = %u timeOffset = %u",type,trackIndex,formatId,timeOffset);
            /* save Artwork Transaktion ID */
            iPodHndl->artwork.transID = iPodHndl->rcvMsgInfo.accTransID;

            if(type == TYPE_UID)
            {
                msg[IPOD_POS1] = msgLen + 4;
                msg[IPOD_POS5] = 0x00;
                /* track index into msg array */
                iPod_convert_to_big64(&msg[IPOD_POS6], trackIndex);
                /* insert format id into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS14], formatId);
                /* insert time offstet into msg array */
                iPod_convert_to_big32(&msg[IPOD_POS16], timeOffset);
            }
            else
            {
                msg[IPOD_POS1] = msgLen;
                if(type == TYPE_PB)
                {
                    msg[IPOD_POS5] = 0x01;
                }
                else
                {
                    msg[IPOD_POS5] = 0x02;
                }

                /* track index into msg array */
                iPod_convert_to_big32(&msg[IPOD_POS6], trackIndex);
                /* insert format id into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS10], formatId);
                /* insert time offstet into msg array */
                iPod_convert_to_big32(&msg[IPOD_POS12], timeOffset);
            }
            

            /* save the pointer to the callback function */
            iPodRegisterCBTrackArtworkData(callback);
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_TYPE_OF_TRACKARTWORKDATA, (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            if(rc == IPOD_OK)
            {
                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p,callback = %p",iPodHndl,callback);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodGetTrackArtworkTimes(U32 iPodID, U32 trackIndex, U16 formatId, U16 artworkIndex, 
                                U16 artworkCount, U16 *resultCount, U32 *buffer)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.10
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U32 trackIndex - specifies which track from the Playback Engine is to be selected <br>
 * S16 formatId - specifies which format of artwork is desired.  <br>
 * U16 artworkIndex - specifies at which index to begin searching for artwork <br>
 * U16 artworkCount - specifies the max. number of times to be returned. A value of -1 (0xFFFF)  <br>
 *                    indicates that there is no preferred limit.
 * \par INOUT PARAMETERS
 * U16* resultCount - number of U32 artwork count values in the buffer <br>
 * U32* buffer - pointer to result buffer. The size of the buffer must be at least the size of artworkCount.
 * The max. needed buffer size for all items can be 65536 bytes.
 * \par REPLY PARAMETERS
 * S32 return -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the list of artwork time locations for a track. The user can get the number of artworks
 * beforehand by using iPodGetIndexedPlayingTrackInfo with IPOD_TRACK_INFO_TYPE = IPOD_TRACK_ARTWORK_COUNT.
 * The parameter trackIndex specifies which track from the Playback Engine is to be selected. The parameter
 * formatID indicates which type of artwork is desired. Please refer to "iPod Extended Interface Specification" page 37.
 * The artworkIndex parameter specifies at which index to begin searching for artwork. A value of 0 indicates
 * that the iPod should start with the first available artwork. A valid value for formatID
 * can be obtained by the function #iPodGetArtworkFormats.
 * artworkCount specifies the maximum number of times (artwork locations) to be returned.
 */
S32 iPodGetTrackArtworkTimes(U32 iPodID,
                             U32  trackIndex,
                             U16  formatId,
                             U16  artworkIndex,
                             U16  artworkCount,
                             U16 *resultCount,
                             U32 *buffer)
{
    U16 i               =   0;
    U32 responseMsgLen  =   0;
    U16 curIdx          =   0;
    S32 rc              =  -1;
    U8* responseMsg     =   NULL;
    U8  msg[]           =  {IPOD_GET_TRACK_ARTWORK_TIMES_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (resultCount != NULL) && (buffer != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if (rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "track identifier = %u formatId = %u artworkIndex = %u artworkCount = %u",trackIndex,formatId,artworkIndex,artworkCount);
            /* track index into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS5], trackIndex);
            /* insert format id into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS9], formatId);
            /* insert time offstet into msg array */
            iPod_convert_to_big32(&msg[IPOD_POS11], artworkIndex);

            /* insert artwork count into msg array */
            iPod_convert_to_big16(&msg[IPOD_POS13], artworkCount);


            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_TRACK_ARTWORKTIMES,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);

            if (rc == 0)
            {
                rc = iPodWaitAndGetResponseLength(iPodHndl);
                if (rc > IPOD_POS3)
                {
                    responseMsgLen = (U32)rc;
                    responseMsg = (U8*) malloc((U32)responseMsgLen);

                    if (responseMsg != NULL)
                    {
                        iPodGetResponseData(iPodHndl, responseMsg);

                        *resultCount = (U16)(responseMsgLen / sizeof(U32));
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "number of U32 artwork count values in the buffer = %d",*resultCount);

                        for (i = 0; i < responseMsgLen; i += sizeof(U32))
                        {
                            buffer[curIdx] = iPod_convert_to_little32(&responseMsg[i + IPOD_POS0]);
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Time offset in milliseconds = %d",buffer[curIdx]);
                            curIdx++;
                        }

                        free(responseMsg);
                        rc = 0;
                    }
                    else
                    {
                        rc = IPOD_ERR_NOMEM;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "No Memory responseMsg is NULL");
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p resultCount = %p buffer = %p",iPodHndl,resultCount,buffer);
    }

    if((rc != IPOD_OK) && (resultCount != NULL))
    {
        *resultCount = 0;
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}

/*!
 * \fn iPodGetTypeOfTrackArtworkTimes(U32 iPodID, IPOD_TRACK_TYPE type, U32 trackIndex, U16 formatId,
 *                              U16 artworkIndex, U16 artworkCount, U16 *resultCount, U32 *buffer)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.10
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * #IPOD_TRACK_TYPE type - track identifier type<br>
 * U32 trackIndex - specifies which track from the Playback Engine is to be selected <br>
 * S16 formatId - specifies which format of artwork is desired.  <br>
 * U16 artworkIndex - specifies at which index to begin searching for artwork <br>
 * U16 artworkCount - specifies the max. number of times to be returned. A value of -1 (0xFFFF)  <br>
 *                    indicates that there is no preferred limit.
 * \par INOUT PARAMETERS
 * U16* resultCount - number of U32 artwork count values in the buffer <br>
 * U32* buffer - pointer to result buffer. The size of the buffer must be at least the size of artworkCount.
 * The max. needed buffer size for all items can be 65536 bytes.
 * \par REPLY PARAMETERS
 * S32 return -
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function gets the list of artwork time locations for a track. The user can get the number of artworks
 * beforehand by using iPodGetIndexedPlayingTrackInfo with IPOD_TRACK_INFO_TYPE = IPOD_TRACK_ARTWORK_COUNT.
 * Depending on the value passed in IPOD_TRACK_TYPE, the parameter trackIndex specifies the tracks UID,
 * or its index in the playback engine or its index in the database list.
 * The parameter formatID indicates which type of artwork is desired.
 * Please refer to "iPod Extended Interface Specification" page 37.
 * The artworkIndex parameter specifies at which index to begin searching for artwork. A value of 0 indicates
 * that the iPod should start with the first available artwork. A valid value for formatID
 * can be obtained by the function #iPodGetArtworkFormats.
 * artworkCount specifies the maximum number of times (artwork locations) to be returned.
 */
S32 iPodGetTypeOfTrackArtworkTimes(U32 iPodID,
                                   IPOD_TRACK_TYPE type, 
                                   U32  trackIndex,
                                   U16  formatId,
                                   U16  artworkIndex,
                                   U16  artworkCount,
                                   U16 *resultCount,
                                   U32 *buffer)
{
    U16 i               =   0;
    U32 responseMsgLen  =   0;
    U16 curIdx          =   0;
    S32 rc              =  -1;
    U8* responseMsg     =   NULL;
    U8  msg[]           =  {IPOD_GET_TYPE_OF_TRACK_ARTWORK_TIMES_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);
    U8 msgLen = 14;

    if((iPodHndl != NULL) && (resultCount != NULL) && (buffer != NULL))
    {
        rc = iPodIsInAdvancedMode(iPodHndl);
        if (rc == IPOD_OK)
        {
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Track Identifier type = 0x%02x Track identifier = %u format Id = %u artwork index = %u artwork count = %u",
                                                        type,trackIndex,formatId,artworkIndex,artworkCount);
            if(type == TYPE_UID)
            {
                msg[IPOD_POS1] = msgLen + 4;
                msg[IPOD_POS5] = 0x00;
                /* track index into msg array */
                iPod_convert_to_big64(&msg[IPOD_POS6], trackIndex);
                /* insert format id into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS14], formatId);
                /* insert time offstet into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS16], artworkIndex);
                iPod_convert_to_big16(&msg[IPOD_POS18], artworkCount);
                
            }
            else
            {
                msg[IPOD_POS1] = msgLen;
                if(type == TYPE_PB)
                {
                    msg[IPOD_POS5] = 0x01;
                }
                else
                {
                    msg[IPOD_POS5] = 0x02;
                }
                
                /* track index into msg array */
                iPod_convert_to_big32(&msg[IPOD_POS6], trackIndex);
                /* insert format id into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS10], formatId);
                /* insert time offstet into msg array */
                iPod_convert_to_big16(&msg[IPOD_POS12], artworkIndex);
                iPod_convert_to_big16(&msg[IPOD_POS14], artworkCount);
            }
            
            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_RET_TYPE_OF_TRACK_ARTWORKTIMES,
                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
            rc = iPodSendCommand(iPodHndl, msg);
            IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns rc = %d",rc);

            if (rc == 0)
            {
                rc = iPodWaitAndGetResponseLength(iPodHndl);

                if (rc > IPOD_POS5)
                {
                    responseMsgLen = (U32)rc;
                    responseMsg = (U8*) malloc((U32)responseMsgLen);

                    if (responseMsg != NULL)
                    {
                        iPodGetResponseData(iPodHndl, responseMsg);
                        if(responseMsg[IPOD_POS0] == 0x00)
                        {
                            responseMsgLen -= 9;
                        }
                        else
                        {
                            responseMsgLen -= 5;
                        }

                        *resultCount = (U16)(responseMsgLen / sizeof(U32));
                        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "number of U32 artwork count values in the buffer = %d",*resultCount);

                        for (i = 0; i < responseMsgLen; i += sizeof(U32))
                        {
                            buffer[curIdx] = iPod_convert_to_little32(&responseMsg[i + IPOD_POS5]);
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "Time offset in milliseconds = %d",buffer[curIdx]);
                            curIdx++;
                        }

                        free(responseMsg);
                        rc = 0;
                    }
                    else
                    {
                        rc = IPOD_ERR_NOMEM;
                        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "responseMsg is NULL");
                    }
                }
            }
        }
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p resultCount = %p buffer = %p",iPodHndl,resultCount,buffer);
    }

    if((rc != IPOD_OK) && (resultCount != NULL))
    {
        *resultCount = 0;
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    return rc;
}


/*!
 * \fn iPodSetDisplayImageBMP(U32 iPodID, const U8* bmpImage)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8* image - the name of the image including the path. e.g. /host/images/aditLogo.bmp
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function uploads a bitmap image to the iPod. The new image replaces the extended interface mode's
 * standard "connected" image. The bitmap image must be a "top-down"-bitmap (if not the bitmap has to be
 * vertically flipped and saved before uploading). The bitmap format must be 24bit/uncompressed.
 * Calls made to SetDisplayImageBMP more frequently than every 15 seconds will return a successful ACK command,
 * but the bitmap will not be displayed on the iPod's screen. Hence use of the SetDisplayImage command
 * should be limited to drawing one bitmap image per accessory connect.
 */
S32 iPodSetDisplayImageBMP(U32 iPodID, const U8* bmpImage)
{
    S32 result          =  -1;
    FILE* bmpFile       =   NULL;
    U32 imageWidth      =   0;
    U32 imageHeight     =   0;
    U16* rgb565Buffer   =   NULL;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (bmpImage != NULL))
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "iPodIsInAdvancedMode() returns : result = %d",result);
        if (result == IPOD_OK)
        {
            bmpFile = fopen((VP)bmpImage,
                            IPOD_IMG_UPLOAD_FILE_OPEN_MODE);

            if (bmpFile != NULL)
            {
                iPodReadAndConvertBMPFile(bmpFile,
                                          &rgb565Buffer,
                                          &imageWidth,
                                          &imageHeight);
                fclose(bmpFile);

                if (rgb565Buffer != NULL)
                {
                    result = setImage(iPodID, imageWidth, imageHeight, rgb565Buffer, IPOD_IMAGE_RGB565_LE);
                    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "setImage() returns : result = %d",result);
                    free(rgb565Buffer);
                }
                else
                {
                    result = IPOD_COMMAND_FAILED;
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPod Command Failed");
                }
            }
        }
    }
    else
    {
        result = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p,bmpImage = %p",iPodHndl,bmpImage);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}


/*!
 * \fn iPodSetDisplayImageMemory(U32 iPodID, const U8* bmpImage)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 * U32 iPodID - ID of the Apple device<br>
 * U8* image - the image location in memory, as a pointer (image data must be in BMP format)
 * \par REPLY PARAMETERS
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function uploads a bitmap image to the iPod. The new image replaces the extended interface mode's
 * standard "connected" image. The bitmap image must be a "top-down"-bitmap (if not the bitmap has to be
 * vertically flipped and saved before uploading). The bitmap format must be 24bit/uncompressed.
 * Calls made to SetDisplayImageBMP more frequently than every 15 seconds will return a successful ACK command,
 * but the bitmap will not be displayed on the iPod's screen. Hence use of the SetDisplayImage command
 * should be limited to drawing one bitmap image per accessory connect.
 */
S32 iPodSetDisplayImageMemory(U32 iPodID, const U8* bmpImage)
{
    S32 result          =  -1;
    U32 imageWidth      =   0;
    U32 imageHeight     =   0;
    U16* rgb565Buffer   =   NULL;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (bmpImage != NULL))
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            iPodReadAndConvertBMPMemory(bmpImage,
                                        &rgb565Buffer,
                                        &imageWidth,
                                        &imageHeight);

            if (rgb565Buffer != NULL)
            {
                result = setImage(iPodID, imageWidth, imageHeight, rgb565Buffer, IPOD_IMAGE_RGB565_LE);
                IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "setImage() returns : result = %d",result);
                free(rgb565Buffer);
            }
            else
            {
                result = IPOD_COMMAND_FAILED;
                IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "iPod Command Failed");
            }
        }
    }
    else
    {
        result = IPOD_BAD_PARAMETER;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p,bmpImage = %p",iPodHndl,bmpImage);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}

/*!
 * \fn iPodSetDisplayImageBMPStoredMonochrom(U32 iPodID, const U8*bmpImage)
 * \par EXTENDED INTERFACE PROTOCOL VERSION
 * 1.01
 * \par INPUT PARAMETERS
 *  U8* image - the pointer, where that bmp is stored
 * \par REPLY PARAMETERS
 * S32 return code - 
 * \li \c \b #IPOD_OK Command success
 * \li \c \b #IPOD_ERROR Command failed
 * \li \c \b #IPOD_ERR_GET_RESPONSE_MSG_FAILED Getting response message failed
 * \li \c \b #IPOD_ERR_ONLY_IN_ADVANCEDMODE iPod is not advanced mode
 * \li \c \b #IPOD_ERR_NOMEM Insufficient memory
 * \li \c \b #IPOD_NOT_CONNECTED iPod is not connected
 * \li \c \b #IPOD_ERR_DISWAI Semaphore Error
 * \li \ref AckErrorCode group of Error code of iPod Ack
 * \li \c \b error The other error code
 * \par DESCRIPTION
 * This function uploads a bitmap image to the iPod. The new image replaces the
 * extended interface mode's standard "connected" image. The bitmap image must
 * be a "top-down"-bitmap (if not the bitmap has to be  vertically flipped
 * and saved before uploading). The bitmap format must be 2bit/uncompressed.
 * Calls made to SetDisplayImageBMPStoredMonochrom more frequently than every
 * 15 seconds will return a successful ACK command,but the bitmap will not be
 * displayed on the iPod's screen. Hence use of the
 * SetDisplayImageBMPStoredMonochrom command should be limited to drawing one
 * bitmap image per accessory connect.
 */
S32 iPodSetDisplayImageBMPStoredMonochrom(U32 iPodID, const U8* bmpImage)
{
    S32 result          =  -1;
    U8  msg1[]          =  {IPOD_SET_DISPLAY_IMAGE_DESCR_CMD}; /* descritor telegram */
    U8  msg2[]          =  {IPOD_SET_DISPLAY_IMAGE_DATA_CMD};  /* data telegram */
    U8* imageDataBuf    =   NULL;
    U16 imageWidth      =   0;
    U16 imageHeight     =   0;
    U32 rowSize         =   0;
    U32 stuffBytes      =   0;
    U16 rowCounter      =   1;
    U8  heightcounter   =   0;
    U32 bufPos          =   0;
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if ((iPodHndl != NULL) && (bmpImage != NULL))/* bmpImage represents the pointer on the image.
                                                    The application of files is not possible here */
    {
        result = iPodIsInAdvancedMode(iPodHndl);
        if (result == IPOD_OK)
        {
            imageWidth  = (U16)bmpImage[IPOD_BMP_HEADER_POS_18];
            imageHeight = (U16)bmpImage[IPOD_BMP_HEADER_POS_22];
            stuffBytes = ((imageWidth * IPOD_IMAGE_BYTE_ALIGN)
                                      % IPOD_IMG_UPLOAD_BYTE_ALIGN);
            rowSize = ((imageWidth * IPOD_IMAGE_BYTE_ALIGN) + stuffBytes);

            /* 1. setup and send the descriptor telegram */
            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "imageWidth = %u, imageHeight = %u, rowSize = %u",imageWidth,imageHeight,rowSize);
            /***** insert pixel format into msg array *****/
            msg1[IPOD_POS9] = (U8)IPOD_IMAGE_MONO;
            /* insert image width into msg */
            iPod_convert_to_big16(&msg1[IPOD_POS10], imageWidth);
            /* insert image height into msg */
            iPod_convert_to_big16(&msg1[IPOD_POS12], imageHeight);
            /* insert row size (stride) into msg */
            iPod_convert_to_big32(&msg1[IPOD_POS14], rowSize);
            iPodSendLongTelegram(iPodHndl, msg1);

            /***** 2. setup and send the image data telegram *****/
            imageDataBuf = (U8*) malloc(rowSize + sizeof(msg2));
            if (imageDataBuf != NULL)
            {
                for (heightcounter = 0; heightcounter < imageHeight; heightcounter++)
                {
                    memset(imageDataBuf, 0, rowSize + sizeof(msg2));

                    /* The max. payload length is limited to 500 bytes, so we have to split */
                    /* the telegram for long rows. Lets take 240 pixel (480 bytes) as max.  */
                    /* pixel per telegram package                                           */

                    if (rowSize <= IPOD_MAX_BYTES_PER_ROW_BLOCK)
                    {
                        /* insert payload length into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + rowSize));
                        /* insert telegram index into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                        rowCounter++;

                        /* copy header and image data */
                        memcpy(imageDataBuf,
                               msg2,
                               sizeof(msg2));
                        if(rowSize >= (U32)(imageWidth * IPOD_IMAGE_BYTE_ALIGN))
                        {
                            memcpy(&imageDataBuf[sizeof(msg2)],
                                   &bmpImage[bufPos],
                                   (imageWidth * IPOD_IMAGE_BYTE_ALIGN));
                        }

                        bufPos += imageWidth;

                        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                        iPodSendLongTelegram(iPodHndl, imageDataBuf);

                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);


                        if (result == 0)
                        {
                            result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        /* handle rows longer than 240 pixel ... */

                        U32 numFullRowBlocks = 0;
                        U32 numRowBlocks = 0;
                        U32 lastRowBlockSize = 0;
                        U32 n = 0;
                        U32 tmpRowSize = IPOD_MAX_BYTES_PER_ROW_BLOCK;

                        numFullRowBlocks = (U32) (rowSize / IPOD_MAX_BYTES_PER_ROW_BLOCK);

                        if ((rowSize % IPOD_MAX_BYTES_PER_ROW_BLOCK) != 0)
                        {
                            numRowBlocks = (numFullRowBlocks + 1);
                        }

                        lastRowBlockSize = (rowSize - (IPOD_MAX_BYTES_PER_ROW_BLOCK * numFullRowBlocks));

                        for (n = 1; n < numRowBlocks; n++)
                        {
                            /* insert payload length into msg */
                            // 5 additional bytes because of Lingo, Command and Descriptor index
                            iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + tmpRowSize));

                            /* insert telegram index into msg */
                            iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                            rowCounter++;

                            /* copy header and image data */
                            memcpy(imageDataBuf,
                                   msg2,
                                   sizeof(msg2));

                            if(rowSize >= tmpRowSize)
                            {
                                memcpy(&imageDataBuf[sizeof(msg2)],
                                       &bmpImage[bufPos],
                                       tmpRowSize);
                            }

                            bufPos += tmpRowSize/IPOD_IMAGE_BYTE_ALIGN;

                            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                            iPodSendLongTelegram(iPodHndl, imageDataBuf);

                            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                            if (result == 0)
                            {
                                result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                            }
                            else
                            {
                                break;
                            }
                        }
                        /* adjust size for the last block */
                        tmpRowSize = lastRowBlockSize;

                        /* insert payload length into msg */
                        // 5 additional bytes because of Lingo, Command and Descriptor index
                        iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + tmpRowSize));

                        /* insert telegram index into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                        rowCounter++;

                        /* copy header and image data */
                        memcpy(imageDataBuf,
                               msg2,
                               sizeof(msg2));

                        if(rowSize >= tmpRowSize)
                        {
                            memcpy(&imageDataBuf[sizeof(msg2)],
                                   &bmpImage[bufPos],
                                   tmpRowSize);
                        }

                        bufPos += tmpRowSize/IPOD_IMAGE_BYTE_ALIGN;

                        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                        iPodSendLongTelegram(iPodHndl, imageDataBuf);

                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (result == 0)
                        {
                            result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                         (U8)IPOD_LINGO_EXTENDED_INTERFACE);

                memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                if (result == 0)
                {
                    result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                    IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                }

                free(imageDataBuf);
            }
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p,bmpImage = %p",iPodHndl,bmpImage);
    }
    IAP1_EXTENDED_LOG(DLT_LOG_DEBUG, "result = %d",result);
    return result;
}

/*\}*/


/* ========================================================================== */
/* private helper functions                                                   */
/* ========================================================================== */

/* Reads a U32 from a file */
U32 iPodReadU32(FILE *file)
{
    U32 l;
    size_t ret = 0;

    ret = fread(&l,
                sizeof(U32),
                1,
                file);
    if(ret == 0)
    {
        IAP1_EXTENDED_LOG(DLT_LOG_WARN, "Failed to read");
    }

    return l;
}


/* Reads a U16 from a file */
U16 iPodReadU16(FILE *file)
{
    U16 s;
    size_t ret = 0;

    ret = fread(&s,
                sizeof(U16),
                1,
                file);
    if(ret == 0)
    {
        IAP1_EXTENDED_LOG(DLT_LOG_WARN, "Failed to read");
    }

    return s;
}


/* Reads a S32 from a file */
S32 iPodReadS32(FILE *file)
{
    S32 l;
    size_t ret = 0;

    ret = fread(&l,
                sizeof(S32),
                1,
                file);
    if(ret == 0)
    {
        IAP1_EXTENDED_LOG(DLT_LOG_WARN, "Failed to read");
    }

    return l;
}


/* Reads a S16 from a file */
S16 iPodReadS16(FILE *file)
{
    S16 s;
    size_t ret = 0;

    ret = fread(&s,
                sizeof(S16),
                1,
                file);
    if(ret == 0)
    {
        IAP1_EXTENDED_LOG(DLT_LOG_WARN, "Failed to read");
    }

    return s;
}


/* Converts R, G and B values to a RGB565 pixel */
void iPodRGBToRGB565(U8 r,
                     U8 g,
                     U8 b,
                     U16 *rgb565)
{
    U16 temp = 0;
    temp = ((U16)(r >> IPOD_BITSHIFT_3) << IPOD_BITSHIFT_11);
    temp |= (U16)( g >> IPOD_BITSHIFT_2) << IPOD_BITSHIFT_5;
    temp |= (U16)( b >> IPOD_BITSHIFT_3) ;

    *rgb565 = temp;
}

/* reads a bmp file. The image format must be 24bit/uncompressed */

S32 setImage(U32 iPodID, U32 imageWidth, U32 imageHeight, U16 *image, IPOD_IMAGE_TYPE imageType)
{
    S32 result          =  -1;
    U8* imageDataBuf = NULL;
    U32 stuffBytes = 0;
    U16 rowSize = 0;
    U32 heightcounter = 0;
    U16 rowCounter      =   1;
    U32 bufPos          =   0;
    U8  msg1[] = {IPOD_SET_DISPLAY_IMAGE_DESCR_CMD};
    U8  msg2[] = {IPOD_SET_DISPLAY_IMAGE_DATA_CMD};
    IPOD_INSTANCE* iPodHndl = iPodGetHandle(iPodID);

    if((iPodHndl != NULL) && (image != NULL))
    {
        stuffBytes = ((imageWidth * IPOD_IMAGE_BYTE_ALIGN) % IPOD_IMG_UPLOAD_BYTE_ALIGN);
        rowSize = (U16)((imageWidth * IPOD_IMAGE_BYTE_ALIGN) + stuffBytes);

        /* 1. setup and send the descriptor ============================= */
        IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "imageType = 0x%02x imageWidth = %u imageHeight = %u rowSize = %u",imageType,imageWidth,imageHeight,rowSize);

        /* insert pixel format into msg array */
        msg1[IPOD_POS9] = (U8)imageType;
        /* insert image width into msg */
        iPod_convert_to_big16(&msg1[IPOD_POS10], (U16)imageWidth);
        /* insert image height into msg */
        iPod_convert_to_big16(&msg1[IPOD_POS12], (U16)imageHeight);
        /* insert row size (stride) into msg */
        iPod_convert_to_big32(&msg1[IPOD_POS14], rowSize);

        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
        iPodSendLongTelegram(iPodHndl, msg1);
        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);//iPodGetResponseMsg(iPodHndl->iAP1Buf);

        if(result == IPOD_OK)
        {
            /* 2. setup and send the image data ============================= */
            imageDataBuf = (U8*) malloc(rowSize + sizeof(msg2));
            if (imageDataBuf != NULL)
            {
                for (heightcounter = 0; heightcounter < imageHeight; heightcounter++)
                {
                    memset(imageDataBuf, 0, rowSize + sizeof(msg2));

                    /* The max. payload length is limited to 500 bytes, so we have to split */
                    /* the telegram for long rows. Lets take 240 pixel (480 bytes) as max.  */
                    /* pixel per telegram package                                           */

                    if (rowSize <= IPOD_MAX_BYTES_PER_ROW_BLOCK)
                    {
                        /* insert payload length into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + rowSize));
                        /* insert telegram index into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                        rowCounter++;

                        /* copy header and image data */
                        memcpy(imageDataBuf,
                               msg2,
                               sizeof(msg2));

                        if(rowSize >= (imageWidth * IPOD_IMAGE_BYTE_ALIGN))
                        {
                            memcpy(&imageDataBuf[sizeof(msg2)],
                                   &image[bufPos],
                                   (imageWidth * IPOD_IMAGE_BYTE_ALIGN));
                        }

                        bufPos += imageWidth;

                        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                        iPodSendLongTelegram(iPodHndl, imageDataBuf);

                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (result == 0)
                        {
                            result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        /* handle rows longer than 240 pixel ... */

                        U32 numFullRowBlocks = 0;
                        U32 numRowBlocks = 0;
                        U32 lastRowBlockSize = 0;
                        U32 n = 0;
                        U32 tmpRowSize = IPOD_MAX_BYTES_PER_ROW_BLOCK;

                        numFullRowBlocks = (U32) (rowSize / IPOD_MAX_BYTES_PER_ROW_BLOCK);

                        if ((rowSize % IPOD_MAX_BYTES_PER_ROW_BLOCK) != 0)
                        {
                            numRowBlocks = (numFullRowBlocks + 1);
                        }

                        lastRowBlockSize = (rowSize - (IPOD_MAX_BYTES_PER_ROW_BLOCK * numFullRowBlocks));

                        for (n = 1; n < numRowBlocks; n++)
                        {
                            /* insert payload length into msg */
                            // 5 additional bytes because of Lingo, Command and Descriptor index
                            iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + tmpRowSize));

                            /* insert telegram index into msg */
                            iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                            rowCounter++;

                            /* copy header and image data */
                            memcpy(imageDataBuf,
                                   msg2,
                                   sizeof(msg2));

                            if(rowSize >= tmpRowSize)
                            {
                                memcpy(&imageDataBuf[sizeof(msg2)],
                                       &image[bufPos],
                                       tmpRowSize);
                            }

                            bufPos += tmpRowSize/IPOD_IMAGE_BYTE_ALIGN;

                            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                     (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                            iPodSendLongTelegram(iPodHndl, imageDataBuf);

                            memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                            result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                            if (result == 0)
                            {
                                result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                                IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                            }
                            else
                            {
                                break;
                            }
                        }
                        /* adjust size for the last block */
                        tmpRowSize = lastRowBlockSize;

                        /* insert payload length into msg */
                        // 5 additional bytes because of Lingo, Command and Descriptor index
                        iPod_convert_to_big16(&msg2[IPOD_POS2], (U16)(IPOD_IMG_UPLOAD_ADDITIONAL_BYTES + tmpRowSize));

                        /* insert telegram index into msg */
                        iPod_convert_to_big16(&msg2[IPOD_POS7], (U16)rowCounter);
                        rowCounter++;

                        /* copy header and image data */
                        memcpy(imageDataBuf,
                               msg2,
                               sizeof(msg2));

                        if(rowSize >= tmpRowSize)
                        {
                            memcpy(&imageDataBuf[sizeof(msg2)],
                                   &image[bufPos],
                                   tmpRowSize);
                        }

                        bufPos += tmpRowSize/IPOD_IMAGE_BYTE_ALIGN;

                        iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_EXTENDED_LINGO_ACKNOWLEDGE,
                                                 (U8)IPOD_LINGO_EXTENDED_INTERFACE);
                        iPodSendLongTelegram(iPodHndl, imageDataBuf);

                        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
                        result = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);

                        if (result == 0)
                        {
                            result = (S32)iPodHndl->iAP1Buf[IPOD_POS0];
                            IAP1_EXTENDED_LOG(DLT_LOG_VERBOSE, "result = %d",result);
                        }
                        else
                        {
                            break;
                        }
                    }
                }/* for-loop */
                free(imageDataBuf);
            }/* if(imageDataBuf != NULL) */
        }
    }
    else
    {
        result = IPOD_ERR_PAR;
        IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "Error Parameter iPodHndl = %p,image = %p",iPodHndl,image);
    }
    return result;

}

void iPodReadAndConvertBMPFile(FILE* file,
                               U16** rgb565Buffer,
                               U32*  width,
                               U32*  height)
{
    U32 imageWidth      =   0;
    S32 imageHeight     =   0;
    U16 bitsPerPixel    =   0;
    U32 compression     =   0;
    U32 startPixelData  =   0;
    U8  r               =   0;
    U8  g               =   0;
    U8  b               =   0;
    U32 j               =   0;
    S32 h               =   0;
    U32 w               =   0;
    U32 stuffBytes      =   0;
    U8 *tmpBuffer = NULL;
    size_t nread = 0;

    /* read width and height of bmp image */
    fseek(file,
          IPOD_BMP_HEADER_POS_18,
          SEEK_SET);

    imageWidth  = iPodReadU32(file);
    imageHeight = iPodReadS32(file);

    /* test if it is a "top-down"-Bitmap */
    if (imageHeight < 0)
    {
        imageHeight     =  -1 * imageHeight;
    }

    *width = imageWidth;
    *height = (U32)imageHeight;

    /* read bits per pixel and compression of bmp image */
    fseek(file,
          IPOD_BMP_HEADER_POS_28,
          SEEK_SET);

    bitsPerPixel = iPodReadU16(file);
    compression  = iPodReadU32(file);

    /* check if bmp image is in right format */
    if ((bitsPerPixel == IPOD_BMP_BIT_PER_PIXEL) && (compression == 0))
    {
        /* bitmap might contain stuff bytes */
        stuffBytes = (imageWidth % IPOD_IMG_UPLOAD_BYTE_ALIGN);

        /* allocate buffer for rgb565 pixel data */
        *rgb565Buffer = (U16*)(calloc((U32)(imageWidth * (U32)imageHeight), sizeof(U16)));
        tmpBuffer = (U8 *)(calloc(((imageWidth * IPOD_RGB_NUMBER) + stuffBytes), sizeof(U8)));

        if ((*rgb565Buffer != NULL) && (tmpBuffer != NULL))
        {
            /* read the startadress of the pixel data */
            fseek(file,
                  IPOD_BMP_POINTER_TO_PIXEL_DATA,
                  SEEK_SET);

            startPixelData = iPodReadU32(file);

            /* read the pixel data */
            fseek(file,
                  (long)startPixelData,
                  SEEK_SET);

            for (h = 0; h < imageHeight; h++)
            {
                nread = fread(tmpBuffer, ((imageWidth * IPOD_RGB_NUMBER) + stuffBytes), IPOD_READ_DATA_NUMBER, file);
                if((nread != IPOD_READ_DATA_NUMBER) && (ferror(file) != 0))
                {
                    IAP1_EXTENDED_LOG(DLT_LOG_ERROR, "fread() for reading a BMP file returns = %zu", nread);
                    clearerr(file);
                    /* free buffer and set him to NULL indicates an error */
                    free(*rgb565Buffer);
                    *rgb565Buffer = NULL;
                    break;
                }
                for (w = 0; w < (imageWidth * IPOD_RGB_NUMBER);
                     w += IPOD_BITSHIFT_3)
                {
                    b = tmpBuffer[w];
                    g = tmpBuffer[w + IPOD_POS1];
                    r = tmpBuffer[w + IPOD_POS2];
                    iPodRGBToRGB565(r, g, b, &(*rgb565Buffer)[j++]);
                }
            }
            free(tmpBuffer);
        }
        else
        {
            if(*rgb565Buffer != NULL)
            {
                free(*rgb565Buffer);
                *rgb565Buffer = NULL;
            }
            
            if(tmpBuffer != NULL)
            {
                free(tmpBuffer);
            }
        }
    }
}


void iPodReadAndConvertBMPMemory(const U8* file,
                                 U16** rgb565Buffer,
                                 U32*  width,
                                 U32*  height)
{
    U32 imageWidth      =   0;
    S32 imageHeight     =   0;
    U16 bitsPerPixel    =   0;
    U32 compression     =   0;
    U32 startPixelData  =   0;
    U8  r               =   0;
    U8  g               =   0;
    U8  b               =   0;
    U32 j               =   0;
    S32 h               =   0;
    U32 w               =   0;
    U32 stuffBytes      =   0;
    U8 *tmpBuffer = NULL;
    U32 pos = 0;
    
    /* read width and height of bmp image */
    memcpy(&imageWidth, &file[18], sizeof(imageWidth));
    memcpy(&imageHeight, &file[22], sizeof(imageHeight));
    /* test if it is a "top-down"-Bitmap */
    if (imageHeight < 0)
    {
        imageHeight     =  -1 * imageHeight;
    }

    *width = imageWidth;
    *height = (U32)imageHeight;

    /* read bits per pixel and compression of bmp image */
    memcpy(&bitsPerPixel, &file[28], sizeof(bitsPerPixel));
    memcpy(&compression, &file[30], sizeof(compression));
    
    /* check if bmp image is in right format */
    if ((bitsPerPixel == IPOD_BMP_BIT_PER_PIXEL) && (compression == 0))
    {
        /* bitmap might contain stuff bytes */
        stuffBytes = (imageWidth % IPOD_IMG_UPLOAD_BYTE_ALIGN);

        /* allocate buffer for rgb565 pixel data */
        *rgb565Buffer = (U16*)(calloc((U32)(imageWidth * (U32)imageHeight), sizeof(U16)));
        tmpBuffer = (U8 *)(calloc(((imageWidth * IPOD_RGB_NUMBER) + stuffBytes), sizeof(U8)));

        if ((*rgb565Buffer != NULL) && (tmpBuffer != NULL))
        {
            /* read the startadress of the pixel data */
            memcpy(&startPixelData, &file[10], sizeof(startPixelData));
            
            /* read the pixel data */
            for (h = 0; h < imageHeight; h++)
            {
                memcpy(tmpBuffer, &file[startPixelData + pos], (imageWidth * IPOD_RGB_NUMBER) + stuffBytes);
                pos += (imageWidth * IPOD_RGB_NUMBER) + stuffBytes;
                for (w = 0; w < (imageWidth * IPOD_RGB_NUMBER);
                     w += IPOD_BITSHIFT_3)
                {
                    b = tmpBuffer[w];
                    g = tmpBuffer[w + IPOD_POS1];
                    r = tmpBuffer[w + IPOD_POS2];
                    iPodRGBToRGB565(r, g, b, &(*rgb565Buffer)[j++]);
                }
            }
            free(tmpBuffer);
        }
        else
        {
            if(*rgb565Buffer != NULL)
            {
                free(*rgb565Buffer);
                *rgb565Buffer = NULL;
            }
            
            if(tmpBuffer != NULL)
            {
                free(tmpBuffer);
            }
        }
    }
}
