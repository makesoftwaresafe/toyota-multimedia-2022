/*****************************************************************************
-  \file : iap2_utility.c
-  \version : $Id: iap2_utility.c, v Exp $
-  \release : $Name:$
-  Contains the utility function implementation for iAP2 parsing & forming of the messages.
-  \component :
-  \author : Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include "iap2_parameters.h"
#include "iap2_defines.h"
#include "iap2_utility.h"

#include "iap2_dlt_log.h"
#include "iap2_init.h"
#include "iAP2Link.h"
#include "iAP2LinkRunLoop.h"
#include "iap2_session_link_callbacks.h"
#include "iap2_parameter_free.h"

#define IAP2_PAR_LENGTH_SIZE 2
#define IAP2_PAR_ID_SIZE     2

#define IAP2_MSG_START_BYTES         0x40, 0x40
#define IAP2_MSG_START_BYTES_OFFSET  0

#define IAP2_ALIGN_UP(upto, a) ( (((a)+(upto-1) ) / upto) * upto )

S32 iAP2SendMsgToLink(const iAP2Device_t* device, U8* BufferToSend, U16 BufferLength, iAP2SessionType_t sessionType)
{
    S32 rc = IAP2_OK;
    BOOL bContinue;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;
    iAP2LinkRunLoop_t* linkRunLoop = (iAP2LinkRunLoop_t*)thisDevice->p_iAP2AccessoryLink;

    /* iAP2PacketServiceType_t is defined in link/iAP2Link/iAP2Packet.h
     * Values are mapped to the iAP2SessionType_t*/
    U8 Session = iAP2LinkGetSessionForService(linkRunLoop->link, (iAP2PacketServiceType_t)sessionType);

    IAP2DLTCONVERTANDLOG(&iAP2SessionCtxt, DLT_LOG_VERBOSE, "message to be sent to device: %s", BufferToSend, BufferLength);
    iAP2LinkRunLoopQueueSendData(linkRunLoop, BufferToSend, BufferLength, Session, NULL, NULL);
    do
    {
        bContinue = iAP2LinkRunLoopRunOnce(linkRunLoop, NULL);
    }while( (TRUE == bContinue) && (thisDevice->iAP2DeviceState != iAP2NotConnected) );

    return rc;
}

S32 iAP2ServiceSendMsgToLink(const iAP2Device_t* device, U8* BufferToSend, U16 BufferLength, iAP2SessionType_t sessionType)
{
    S32 rc = IAP2_OK;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;
    if(thisDevice->iAP2Service != NULL)
    {
        rc = iAP2ServiceSendMessageToDevice(thisDevice, BufferToSend, BufferLength, sessionType);
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Link interface called without creating iAP2Service object!");
    }

    return rc;
}

S32 iAP2FormandSendCommand(iAP2Device_t* device, iAP2FormCommand_t iAP2FormCommand)
{
    S32 rc = IAP2_OK;
    U16 parameterLength = 0;
    iAP2Device_st* thisDevice = (iAP2Device_st*)device;

    if(iAP2FormCommand.iAP2ParameterStructure == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input DevID:%p", device);
    }
    if( (rc == IAP2_OK) && (iAP2FormCommand.iAP2CalculateParameterLengh != NULL) )
    {
        rc = iAP2FormCommand.iAP2CalculateParameterLengh(iAP2FormCommand.iAP2ParameterStructure, &parameterLength);
        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2CalculateParameterLengh() returns rc:%d", rc);
    }
    if(rc == IAP2_OK)
    {
        U8* msgBuf = NULL;
        U16 msgLength = IAP2_MSG_HEADER_SIZE;
        U16 currentPosition = IAP2_MSG_PARAMETER_OFFSET;

        msgLength += parameterLength;
        msgBuf = calloc(msgLength, sizeof(U8));
        if(msgBuf != NULL)
        {
            iAP2FillMsgBufiAP2Header(&msgBuf, msgLength, iAP2FormCommand.MessageID);
            if(iAP2FormCommand.iAP2FillMessageBuffer != NULL)
            {
                rc = iAP2FormCommand.iAP2FillMessageBuffer(msgBuf, iAP2FormCommand.iAP2ParameterStructure, &currentPosition);
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2FillMessageBuffer() returns rc:%d", rc);
            }
            if(rc == IAP2_OK)
            {
                rc = thisDevice->iAP2Link->send(thisDevice, msgBuf, msgLength, Control);
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2SendControlSessionMessage() returns rc:%d", rc);
            }
            free(msgBuf);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2SESSIONDLTLOG(DLT_LOG_FATAL, "Out of Memory. calloc failed for msgBuf DevID:%p", device);
        }
    }

    return rc;
}

S32 iAP2CreateBufferPool(U8** bufferpoolptr)
{
    S32 rc = IAP2_OK;

    *bufferpoolptr = calloc(IAP2_LINK_MAX_PACKET_SIZE, sizeof(U8));
    if(*bufferpoolptr == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }

    return rc;
}

void iAP2FreeBufferPool(U8* bufferpoolptr)
{
    if(bufferpoolptr != NULL)
    {
        free(bufferpoolptr);
    }
}

void iAP2InitializeBufferPool(U8* bufferpoolptr)
{
    memset(bufferpoolptr, 0, IAP2_LINK_MAX_PACKET_SIZE);
}

S32 iAP2AllocateFromBufferPool(void* dest_ptr, const U8* bufferpoolptr, U8** bufferpoolwrptr, U16* parametercount, U32 parametersize, BOOL isparametertypegroup)
{
    S32 rc = IAP2_ERR_NO_MEM;
    U8** destptr = (U8**)dest_ptr;

    if(isparametertypegroup == TRUE)
    {
        *destptr = calloc(*parametercount, parametersize);
        if(*destptr != NULL)
        {
            rc = IAP2_OK;
        }
    }
    else
    {
        *bufferpoolwrptr = (U8*)IAP2_ALIGN_UP(parametersize, (uintptr_t)(*bufferpoolwrptr));

        U32 sizetoallocate = *parametercount * parametersize;
        if( (*bufferpoolwrptr + sizetoallocate) <= (bufferpoolptr + IAP2_LINK_MAX_PACKET_SIZE) )
        {
            *destptr = *bufferpoolwrptr;
            *bufferpoolwrptr = (*bufferpoolwrptr + sizetoallocate);
            rc = IAP2_OK;
        }
        else
        {
            /* This situation should not arise */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "parametercount = %d, parametersize = %d", *parametercount, parametersize);
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "bufferpoolwrptr = %p, bufferpoolptr = %p, sizetoallocate = %d", *bufferpoolwrptr, bufferpoolptr, sizetoallocate);
        }
    }
    *parametercount = 0;

    return rc;
}

/**
* \fn  iAP2CheckAndSetParameterLength(U16* p_ParameterLength, U16 LengthToSet)
* \par INPUT PARAMETERS
* U16* p_ParameterLength - pointer which contains the current length and which is the destination of the new length.<br>
* U32  LengthToSet - Length of the parameter which has to be stored to p_ParameterLength.<br>
* S32 result -
* \li \c \b #IAP2_OK                        Success, if the length fits the Linklayer package size.
* \li \c \b #IAP2_INVALID_PARAMETER_LENGTH  Error, if new length does not fit into the Linklayer package.
* \par DESCRIPTION
* This function updated the parameter length if the new length fits the maximum LinkLayer package size.
*/
static inline S32 iAP2CheckAndSetParameterLength(U16* p_ParameterLength, U32 LengthToSet)
{
    S32 rc = IAP2_OK;

    /* Check if current parameter length plus new length,
     * fits into the maximum Linklayer package size. */
    if(IAP2_MAX_PARMETER_LENGTH >= (*p_ParameterLength + LengthToSet))
    {
        *p_ParameterLength += LengthToSet;
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid parameter length %d",
            (*p_ParameterLength + LengthToSet));
        rc = IAP2_INVALID_PARAMETER_LENGTH;
    }

    return rc;
}

S32 iAP2CalculateParSpace(iAP2_Type type, const void* parameter, U16* ParameterLength, U32 parametersize)
{
    S32 rc = IAP2_OK;

    if( (type != iAP2_group) && (type != iAP2_none) && (parameter == NULL) )
    {
        rc = IAP2_BAD_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input");
    }

    if(rc == IAP2_OK)
    {
        if( (type == iAP2_utf8) && (parameter != NULL) )
        {
            rc = iAP2CheckAndSetParameterLength(ParameterLength, (strlen((const char*)parameter) + 1));
        }
        else if( (type == iAP2_group) || (type == iAP2_none) )
        {
            /* Parameter Length & ID Size will be added at the end, don't do anything here */
        }
        else
        {
            rc = iAP2CheckAndSetParameterLength(ParameterLength, parametersize);
        }
    }
    if(rc == IAP2_OK)
    {
        *ParameterLength += IAP2_PAR_LENGTH_SIZE + IAP2_PAR_ID_SIZE;
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2CalculateParSpace() ParameterLength = 0x%X, returns rc:%d", *ParameterLength, rc);

    return rc;
}

static inline void iAP2Copy16bit(U8* targetbuffer, U16 datatocopy, U16* position)
{
    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
    datatocopy = IAP2_ADHERE_TO_APPLE_ENDIANESS_16(datatocopy);    /*lint !e160 !e644 */
    memcpy(targetbuffer, &datatocopy, sizeof(U16));
    if(position != NULL)
    {
        *position += sizeof(U16);
    }
}

void iAP2FillMsgBufiAP2Header(U8** iAP2MsgBuf, U16 iAP2MsgLength, U16 iAP2MsgID)
{
    U8 msgStart[] = {IAP2_MSG_START_BYTES};

    memcpy(&( (*iAP2MsgBuf)[IAP2_MSG_START_BYTES_OFFSET] ), msgStart, IAP2_MSG_START_SIZE);
    iAP2Copy16bit(&( (*iAP2MsgBuf)[IAP2_MSG_LENGTH_OFFSET] ), iAP2MsgLength, NULL);
    iAP2Copy16bit(&( (*iAP2MsgBuf)[IAP2_MSG_ID_OFFSET] ),     iAP2MsgID, NULL);
}

void iAP2FillParameterHeader(U8* target, U16 parameterLength, U16 parameterID, U16* position)
{
    iAP2Copy16bit(&target[*position], parameterLength, position);
    iAP2Copy16bit(&target[*position], parameterID,     position);
}

S32 iAP2FillParameter(U8* target, void* source, U16* position, U16 parameterID, iAP2_Type type, iAP2_Type blob_type, U32 parametersize)
{
    S32 rc = IAP2_OK;

    U16 headerSize = IAP2_PAR_LENGTH_SIZE + IAP2_PAR_ID_SIZE;
    U16 parameterLength = 0;

    if(type == iAP2_none)
    {
        rc = iAP2CalculateParSpace(type, source, &parameterLength, parametersize);
        if( (rc == IAP2_OK) && (parameterLength >= headerSize) )
        {
            iAP2Copy16bit(&target[*position], parameterLength, position);
            iAP2Copy16bit(&target[*position], parameterID,     position);
        }
    }
    else if(source != NULL)
    {
        rc = iAP2CalculateParSpace(type, source, &parameterLength, parametersize);
        if( (rc == IAP2_OK) && (parameterLength >= headerSize) )
        {
            iAP2Copy16bit(&target[*position], parameterLength, position);
            iAP2Copy16bit(&target[*position], parameterID,     position);

            switch(type)
            {
                case iAP2_uint8:
                case iAP2_int8:
                case iAP2_enum:
                case iAP2_bool:
                {
                    target[*position] = *((U8*)source);
                    break;
                }

                case iAP2_uint16:
                case iAP2_int16:
                case iAP2_secs16:
                case iAP2_msecs16:
                case iAP2_usecs16:
                {
                    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                    *((U16*)source) = IAP2_ADHERE_TO_APPLE_ENDIANESS_16(*((U16*)source));    /*lint !e160 !e644 */
                    memcpy(&target[*position], source, sizeof(U16));
                    break;
                }

                case iAP2_uint32:
                case iAP2_int32:
                case iAP2_secs32:
                case iAP2_msecs32:
                case iAP2_usecs32:
                {
                    *((U32*)source) = IAP2_ADHERE_TO_APPLE_ENDIANESS_32(*((U32*)source));
                    memcpy(&target[*position], source, sizeof(U32));
                    break;
                }

                case iAP2_uint64:
                case iAP2_int64:
                case iAP2_secs64:
                case iAP2_msecs64:
                case iAP2_usecs64:
                {
                    *((U64*)source) = IAP2_ADHERE_TO_APPLE_ENDIANESS_64(*((U64*)source));
                    memcpy(&target[*position], source, sizeof(U64));
                    break;
                }

                case iAP2_utf8:
                {
                    memcpy(&target[*position], source, (parameterLength - headerSize));
                    break;
                }

                case iAP2_blob:
                {
                    U32 count = 0 ;

                    if( (blob_type == iAP2_uint16) || (blob_type == iAP2_int16) )
                    {
                        U16 ExtractedValue;

                        for(count = 0 ; count < (U32)(parameterLength - headerSize); count += sizeof(U16) )
                        {
                            void* actualvalue = &(((iAP2Blob*)source)->iAP2BlobData[count]);

                            /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                            /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                            ExtractedValue = IAP2_ADHERE_TO_APPLE_ENDIANESS_16(*((U16*)actualvalue) );    /*lint !e160 !e644 */
                            memcpy(&target[*position + count], &ExtractedValue, sizeof(U16));
                        }
                    }
                    else if( (blob_type == iAP2_uint32) || (blob_type == iAP2_int32) )
                    {
                        U32 ExtractedValue;

                        for(count = 0 ; count < (U32)(parameterLength - headerSize); count += sizeof(U32) )
                        {
                            void* actualvalue = &(((iAP2Blob*)source)->iAP2BlobData[count]);

                            ExtractedValue = IAP2_ADHERE_TO_APPLE_ENDIANESS_32(*((U32*)actualvalue) );
                            memcpy(&target[*position + count], &ExtractedValue, sizeof(U32));
                        }
                    }
                    else if( (blob_type == iAP2_uint64) || (blob_type == iAP2_int64) )
                    {
                        U64 ExtractedValue;

                        for(count = 0 ; count < (U32)(parameterLength - headerSize); count += sizeof(U64) )
                        {
                            void* actualvalue = &(((iAP2Blob*)source)->iAP2BlobData[count]);

                            ExtractedValue = IAP2_ADHERE_TO_APPLE_ENDIANESS_64(*((U64*)actualvalue) );
                            memcpy(&target[*position + count], &ExtractedValue, sizeof(U64));
                        }
                    }
                    else
                    {
                        memcpy(&target[*position], ((iAP2Blob*)source)->iAP2BlobData, parameterLength - headerSize);
                    }
                    break;
                }

                case iAP2_none:
                case iAP2_group:
                default:
                {
                    rc = IAP2_BAD_PARAMETER;
                    break;
                }
            }
            if(rc == IAP2_OK)
            {
                *position += (parameterLength - headerSize);
            }
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }
    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2FillParameter() parameter id: %d, parameter type: %d, returns rc: %d", parameterID, type, rc);

    return rc;
}

S32 iAP2GetParam(void* target_buffer, U8* source_buf, U16 buf_size, iAP2_Type type)
{
    S32 rc = IAP2_OK;

    /* Subtract Header Length to Get the size of the parameter data */
    buf_size = buf_size - IAP2_PAR_DATA_OFFSET;

    if(target_buffer != NULL)
    {
        void* sourcebfdataloc = &source_buf[IAP2_PAR_DATA_OFFSET];

        switch(type)
        {
            case iAP2_int8:
            case iAP2_uint8:
            case iAP2_enum:
            case iAP2_bool:
            {
                if( buf_size != sizeof(U8) )
                {
                    rc = IAP2_INVALID_PARAMETER_LENGTH;
                }
                else
                {
                    U8* sourcebuf = (U8*)sourcebfdataloc;

                    *((U8*)target_buffer) = *sourcebuf;
                }
                break;
            }

            case iAP2_int16:
            case iAP2_uint16:
            case iAP2_secs16:
            case iAP2_msecs16:
            case iAP2_usecs16:
            {
                if( buf_size != sizeof(U16) )
                {
                    rc = IAP2_INVALID_PARAMETER_LENGTH;
                }
                else
                {
                    memcpy(target_buffer, sourcebfdataloc, sizeof(U16));

                    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                    *((U16*)target_buffer) = IAP2_ADHERE_TO_HOST_ENDIANESS_16(*((U16*)target_buffer));    /*lint !e160 !e644 */
                }
                break;
            }

            case iAP2_int32:
            case iAP2_uint32:
            case iAP2_secs32:
            case iAP2_msecs32:
            case iAP2_usecs32:
            {
                if( buf_size != sizeof(U32) )
                {
                    rc = IAP2_INVALID_PARAMETER_LENGTH;
                }
                else
                {
                    memcpy(target_buffer, sourcebfdataloc, sizeof(U32));
                    *((U32*)target_buffer) = IAP2_ADHERE_TO_HOST_ENDIANESS_32(*((U32*)target_buffer));
                }
                break;
            }

            case iAP2_int64:
            case iAP2_uint64:
            case iAP2_secs64:
            case iAP2_msecs64:
            case iAP2_usecs64:
            {
                if( buf_size != sizeof(U64) )
                {
                    rc = IAP2_INVALID_PARAMETER_LENGTH;
                }
                else
                {
                    memcpy(target_buffer, sourcebfdataloc, sizeof(U64));
                    *((U64*)target_buffer) = IAP2_ADHERE_TO_HOST_ENDIANESS_64(*((U64*)target_buffer));
                }
                break;
            }

            case iAP2_blob:
            {
                ((iAP2Blob*)target_buffer)->iAP2BlobLength = buf_size;
                ((iAP2Blob*)target_buffer)->iAP2BlobData = (U8*)sourcebfdataloc;
                break;
            }

            case iAP2_utf8:
            {
                *((U8**)target_buffer) =  (U8*)sourcebfdataloc;
                if((*((U8**)target_buffer))[buf_size - 1] != '\0')
                {
                    (*((U8**)target_buffer))[buf_size - 1] = '\0';
                    IAP2SESSIONDLTLOG(DLT_LOG_WARN, "WARNING:utf8 string sent from Device not Null-terminated ");
                }
                break;
            }

            case iAP2_group:
            case iAP2_none:
            default:
            {
                break;
            }
        }
        if(rc == IAP2_INVALID_PARAMETER_LENGTH)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid parameter length");
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE, "parsed %d bytes", buf_size);
        }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input");
    }

    return rc;
}

static inline S32 iAP2AllocateMemory(void* dest_ptr, void* src_ptr, U16 ParameterCount, U32 Size)
{
    S32 rc = IAP2_OK;
    U64** destptr = (U64**)dest_ptr;
    U64*  srcptr  = (U64*)src_ptr;

    *destptr = calloc(ParameterCount, Size);
    if(*destptr == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        U16 count;

        for(count = 0; count < ParameterCount; count++)
        {
            memcpy(&((*destptr)[count]), &srcptr[count], Size);
        }
    }

    return rc;
}

S32 iAP2AllocateandUpdateData(void* dest_ptr, void* src_ptr, U16* dest_count, U16 src_count, iAP2_Type iAP2Type, U32 parametersize)
{
    S32 rc = IAP2_OK;

    switch(iAP2Type)
    {
        case iAP2_int8:
        case iAP2_uint8:
        case iAP2_int16:
        case iAP2_uint16:
        case iAP2_int32:
        case iAP2_uint32:
        case iAP2_int64:
        case iAP2_uint64:
        case iAP2_secs16:
        case iAP2_msecs16:
        case iAP2_usecs16:
        case iAP2_secs32:
        case iAP2_msecs32:
        case iAP2_usecs32:
        case iAP2_secs64:
        case iAP2_msecs64:
        case iAP2_usecs64:
        {
            if(IAP2_OK == iAP2AllocateMemory(dest_ptr, src_ptr, src_count, parametersize) )
            {
                (*dest_count) += src_count;
            }
            break;
        }

        case iAP2_blob:
        {
            iAP2Blob** dest_blob = (iAP2Blob**)dest_ptr;
            U8* src = (U8*)src_ptr;

            *dest_blob = (iAP2Blob*)calloc(1, sizeof(iAP2Blob));
            if(*dest_blob == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                (*dest_blob)->iAP2BlobData = (U8*)calloc(src_count, sizeof(U8));
                if((*dest_blob)->iAP2BlobData == NULL)
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
            if(rc == IAP2_OK)
            {
                memcpy((*dest_blob)->iAP2BlobData, src, src_count);
                (*dest_blob)->iAP2BlobLength = src_count;
                *dest_count = 1;
            }
            break;
        }

        case iAP2_utf8:
        {
            U8*** destptr = (U8***)dest_ptr;
            U8**  srcptr  = (U8**)src_ptr;

            *destptr = (U8**)calloc(src_count, sizeof(U8*));
            if(*destptr == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                U32 StringLength;

                for(*dest_count = 0; ( (*dest_count < src_count) && (rc == IAP2_OK) ); (*dest_count)++)
                {
                    StringLength = strnlen( (const char*)srcptr[*dest_count], STRING_MAX) + IAP2_NULL_CHAR_LEN;
                    (*destptr)[*dest_count] = (U8*)calloc(StringLength, sizeof(U8));
                    if((*destptr)[*dest_count] == NULL)
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                    else
                    {
                        memcpy((*destptr)[*dest_count], srcptr[*dest_count], StringLength);
                        ((*destptr)[*dest_count])[StringLength - IAP2_NULL_CHAR_LEN] = '\0';
                    }
                }
            }
            break;
        }

        case iAP2_bool:
        case iAP2_enum:
        case iAP2_group:
        case iAP2_none:
        default:
        {
            rc = IAP2_BAD_PARAMETER;
            break;
        }
    }

    return rc;
}

static void iAP2FreeBlob(iAP2Blob* p_BlobToFree)
{
    if(p_BlobToFree != NULL)
    {
        if(p_BlobToFree->iAP2BlobData != NULL)
        {
            free(p_BlobToFree->iAP2BlobData);
            p_BlobToFree->iAP2BlobData = NULL;
        }
        p_BlobToFree->iAP2BlobLength = 0;
    }
    else
    {
        IAP2SESSIONDLTLOG(DLT_LOG_WARN, "Invalid Input Parameter");
    }
}

void iAP2FreeiAP2Pointer(void** iAP2PtrtoFree, U16* iAP2ParameterCount, S8* iAP2DbgStr, iAP2_Type iAP2type)
{
    if(*iAP2PtrtoFree != NULL)
    {
        if(iAP2type == iAP2_blob)
        {
            U16 count;
            iAP2Blob** iAP2BlobPtrtoFree = (iAP2Blob**)iAP2PtrtoFree;

            for(count = 0; count < *iAP2ParameterCount; count++)
            {
                iAP2FreeBlob( (iAP2Blob*)&( (*iAP2BlobPtrtoFree)[count] ) );
            }
        }
        else if(iAP2type == iAP2_utf8)
        {
            U16 count;
            U8*** iAP2DoublePtrtoFree = (U8***)iAP2PtrtoFree;

            for(count = 0; count < *iAP2ParameterCount; count++)
            {
                if( (*iAP2DoublePtrtoFree)[count] != NULL )
                {
                    free((*iAP2DoublePtrtoFree)[count]);
                    (*iAP2DoublePtrtoFree)[count] = NULL;
                }
            }
        }
        else
        {
            /* for other types just the below statement if sufficient */
        }
        free(*iAP2PtrtoFree);
        *iAP2PtrtoFree = NULL;
    }
#ifdef IAP2_DLT_ENABLE
    else
    {
        if( (*iAP2ParameterCount != 0) && (iAP2type != iAP2_none) )
        {
            IAP2SESSIONDLTLOG(DLT_LOG_WARN, (const char*)iAP2DbgStr, " is NULL");
        }
    }
#else
    (void)iAP2DbgStr;
#endif
    /* Free the count value */
    *iAP2ParameterCount = 0;
}
