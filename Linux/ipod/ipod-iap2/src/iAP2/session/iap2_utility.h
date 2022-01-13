/************************************************************************
 * \file: iap2_utility.h
 *
 * \version: $ $
 *
 * This header file declares utility items related to iAP2.
 *
 * \component: global definition file
 *
 * \author: Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
 *
 * \copyright: (c) 2010 - 2013 ADIT Corporation
 *
 ***********************************************************************/

#ifndef IAP2_UTILITY_H
#define IAP2_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_init_private.h"
#include <endian.h>

//   4 u64 le64_to_cpup(__le64 *);
//   5 u32 le32_to_cpup(__le32 *);
//   6 u16 le16_to_cpup(__le16 *);

/* Functions to Adhere to Apple Endianess */
#if __BYTE_ORDER == __LITTLE_ENDIAN

#define IAP2_ADHERE_TO_HOST_ENDIANESS_16(x) be16toh(x)
#define IAP2_ADHERE_TO_HOST_ENDIANESS_32(x) be32toh(x)
#define IAP2_ADHERE_TO_HOST_ENDIANESS_64(x) be64toh(x)

#define IAP2_ADHERE_TO_APPLE_ENDIANESS_16(x) htobe16(x)
#define IAP2_ADHERE_TO_APPLE_ENDIANESS_32(x) htobe32(x)
#define IAP2_ADHERE_TO_APPLE_ENDIANESS_64(x) htobe64(x)

#elif __BYTE_ORDER == __BIG_ENDIAN

#define IAP2_ADHERE_TO_HOST_ENDIANESS_16(x) (x)
#define IAP2_ADHERE_TO_HOST_ENDIANESS_32(x) (x)
#define IAP2_ADHERE_TO_HOST_ENDIANESS_64(x) (x)

#define IAP2_ADHERE_TO_APPLE_ENDIANESS_16(x) (x)
#define IAP2_ADHERE_TO_APPLE_ENDIANESS_32(x) (x)
#define IAP2_ADHERE_TO_APPLE_ENDIANESS_64(x) (x)

#else

#error - only big and little endian systems are supported.

#endif

/**
 * iAP2 endianness conversion
 */
#define IPOD_SHIFT_8                     8

/**
 * iAP2 Control session message Header Length
 */
#define IAP2_MSG_HEADER_SIZE             6 //IAP2_MSG_START_SIZE + IAP2_MSG_LENGTH_SIZE + IAP2_MSG_ID_SIZE
/**
 * iAP2 Control session message start bytes length
 */
#define IAP2_MSG_START_SIZE              2
/**
 * iAP2 Control session message length offset
 */
#define IAP2_MSG_LENGTH_OFFSET           2
/**
 * iAP2 Control session message ID offset
 */
#define IAP2_MSG_ID_OFFSET               4
/**
 * iAP2 Control session message parameter offset
 */
#define IAP2_MSG_PARAMETER_OFFSET        6
/**
 * iAP2 Control session message parameter data offset
 */
#define IAP2_PAR_DATA_OFFSET             4
/**
 * iAP2 Control Session message ID offset
 */
#define IAP2_ID_OFFSET                   2
/**
 * iAP2 Link packet structure size.
 * fixed-size 9 byte header + payload checksum
 */
#define IAP2_LINK_PACKET_SIZE            10
/**
 * Maximum iAP2 parameter length which fit into LinkLayer package.
 */
#define IAP2_MAX_PARMETER_LENGTH         (IAP2_LINK_MAX_PACKET_SIZE - (IAP2_LINK_PACKET_SIZE + IAP2_MSG_HEADER_SIZE))
/**
 * iAP2 EAP Session identifier length
 */
#define IAP2_EA_SESSION_IDENTFIER_LENGTH 2

#define IAP2_INITIALIZE_TO_ZERO          0

typedef struct
{
    U16 MessageID;
    const void* iAP2ParameterStructure;
    S32 (*iAP2CalculateParameterLengh)(const void*, U16*);
    S32 (*iAP2FillMessageBuffer)(U8*, const void*, U16*);
}iAP2FormCommand_t;

/**
 * \addtogroup SessionBuffPoolAndUtility
 * @{
 */

/***************************************************************************//**
* Sends any kind of session message to the Link Layer.
*
* \param thisDevice   Structure having information about the device connected to
*                     the target.
* \param BufferToSend Buffer having information about the message that has to be
*                     sent to Apple Device.
* \param BufferLength Length of the Buffer having information about the message
*                     that has to be sent to Apple Device.
* \param Session      Session ID of the message that has to be sent to Apple Device.
*
* \return IAP2_OK  When the message has been sent to Link Layer successfully.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \return IAP2_CTL_ERROR  While unable to send message to Link Layer.
*
* \see
* \note
******************************************************************************/
S32 iAP2SendMsgToLink(const iAP2Device_t* thisDevice, U8* BufferToSend, U16 BufferLength, iAP2SessionType_t Session);

/***************************************************************************//**
* Sends any kind of session message to iAP2Service.
*
* \param thisDevice   Structure having information about the device connected to
*                     the target.
* \param BufferToSend Buffer having information about the message that has to be
*                     sent to Apple Device.
* \param BufferLength Length of the Buffer having information about the message
*                     that has to be sent to Apple Device.
* \param Session      Session ID of the message that has to be sent to Apple Device.
*
* \return IAP2_OK  When the message has been sent to Link Layer successfully.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \return IAP2_CTL_ERROR  While unable to send message to Link Layer.
*
* \see
* \note
******************************************************************************/
S32 iAP2ServiceSendMsgToLink(const iAP2Device_t* thisDevice, U8* BufferToSend, U16 BufferLength, iAP2SessionType_t sessionType);

/***************************************************************************//**
* Calculates the Parameter Length, Fills those parameters, along with header
* information into the buffer that has to be sent to iPod and then sends the
* message to link layer.
*
* \param device          Structure having information about the device connected
*                        to the target.
* \param iAP2FormCommand Information about MessageID, Parameters, Address of Length
*                        Calculation & Fill Buffer functions.
*
* \return IAP2_OK  On Successful completion of forming the message that has to be
*                  sent to Apple Device.
* \return IAP2_INVALID_INPUT_PARAMETER  When the input pointer is NULL.
* \return IAP2_ERR_NO_MEM  While unable to allocate memory.
* \return IAP2_INVALID_PARAMETER_COUNT  When the Parameter Count does not meet the
*                                       criteria as mentioned in the Specification.
*
* \see
* \note
******************************************************************************/
S32 iAP2FormandSendCommand(iAP2Device_t* device, iAP2FormCommand_t iAP2FormCommand);

/* Buffer Pool Wrapper API's */
/**************************************************************************//**
* Allocates memory for the Buffer Pool.
*
* \param bufferpoolptr  In which the Allocated Buffer Pool Memory has to be
*                       stored.
*
* \return IAP2_OK          On Successful completion of creating the Buffer Pool.
* \return IAP2_ERR_NO_MEM  When unable to allocate memory for the Buffer Pool.
*
* \see
* \note
******************************************************************************/
S32 iAP2CreateBufferPool(U8** bufferpoolptr);

/**************************************************************************//**
* Frees the memory allocated for the Buffer Pool.
*
* \param bufferpoolptr  Address of the Buffer Pool, which has to be freed.
* \return None.
*
* \see
* \note
******************************************************************************/

void iAP2FreeBufferPool(U8* bufferpoolptr);

/**************************************************************************//**
* Initializes the Buffer Pool, by initializing to zero.
*
* \param bufferpoolptr Address of the Buffer Pool, which has to be initialized
* \return none
* \see
* \note
******************************************************************************/

void iAP2InitializeBufferPool(U8* bufferpoolptr);

/**************************************************************************//**
* Allocates memory from the buffer pool, stores it to destination and updates
* the write pointer accordingly.
*
* \param dest_ptr        In which memory allocated from Buffer Pool has to be
*                        updated.
* \param bufferpoolptr   Based Location of the Buffer Pool.
* \param bufferpoolwrptr In which the free memory of unallocated buffer pool has
*                        to be updated.
* \param parametercount  variable holding address of number of times the parameter
*                        is occuring in a message.
* \param parametersize   size of the parameter.
*
* \return IAP2_OK        On Successful completion of allocating memory from the
*                        Buffer Pool.
* \return IAP2_ERR_NO_MEM When unable to allocate memory from the Buffer Pool.
*
* \see
* \note
******************************************************************************/

S32 iAP2AllocateFromBufferPool(void* dest_ptr, const U8* bufferpoolptr, U8** bufferpoolwrptr, U16* parametercount, U32 parametersize, BOOL isparametertypegroup);

/**************************************************************************//**
* Evaluates whether input provided by MC is not NULL & updates the length
* required for each parameter based on their type.
*
* \param type             Type of the Parameter.
* \param parameter        Buffer holding the Parameter.
* \param ParameterLength  Parameter Length in which the calculated length will
*                         be updated.
* \param parametersize    Size of the parameter.
*
* \return IAP2_OK             On Successful completion of calculating the space
*                             required.
* \return IAP2_BAD_PARAMETER  When input provided by MC is NULL.
*
* \see
* \note
******************************************************************************/

S32 iAP2CalculateParSpace(iAP2_Type type, const void* parameter, U16* ParameterLength, U32 parametersize);

/**************************************************************************//**
* Updated the Header information (Length & ID) to the Message buffer pointer
* provided.
*
* \param iAP2MsgBuf    Buffer pointer to which the header information has to be
*                      updated.
* \param iAP2MsgLength Length of the iAP2 Message which has to be stored to
*                      iAP2MsgBuf.
* \param iAP2MsgID     ID of the iAP2 Message which has to be stored to iAP2MsgBuf.
*
* \return None
* \see
* \note
******************************************************************************/

void iAP2FillMsgBufiAP2Header(U8** iAP2MsgBuf, U16 iAP2MsgLength, U16 iAP2MsgID);

/**************************************************************************//**
* Fills the parameter headers to the target buffer.
*
* \param target          Target Buffer in which the parameter has to be filled.
* \param parameterLength Length of the parameter.
* \param parameterID     ID of the parameter.
* \param type            Type of the Parameter.
*
* \return None.
*
* \see
* \note
******************************************************************************/

void iAP2FillParameterHeader(U8* target, U16 parameterLength, U16 parameterID, U16* position);

/**************************************************************************//**
* Fills the parameter from the source buffer to the target buffer.
*
* \param target      Target Buffer in which the parameter has to be filled.
* \param source      Source Buffer from which the parameter has to be copied to
*                    the target buffer.
* \param position    Position of Target Buffer, i.e., at which location the
*                    parameter has to be stored.
* \param parameterID ID of the parameter.
* \param type        Type of the Parameter.
*
* \return IAP2_OK             On Successful completion of filling up the target
*                             buffer with the parameters provided.
* \return IAP2_BAD_PARAMETER  When input provided by MC is NULL or when invalid
*                             type is passed.
* \see
* \note
******************************************************************************/

S32 iAP2FillParameter(U8* target, void* source, U16* position, U16 parameterID, iAP2_Type type, iAP2_Type blob_type, U32 parametersize);

/**************************************************************************//**
* Extracts the ID or Length from the buffer provided.
*
* \param  DataLocation  Location from which the ID or Length has to be retrieved.
*
* \return Calculated ID or Length value.
*
* \see
* \note
******************************************************************************/

static inline U16 iAP2GetIDorLength(void* DataLocation)
{
    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
    return IAP2_ADHERE_TO_HOST_ENDIANESS_16(*((U16*)DataLocation));    /*lint !e160 !e644 */
}

/**************************************************************************//**
* Fills the parameter headers to the target buffer.
*
* \param target_buffer  Target Buffer in which the extracted parameter has to
*                       be filled.
* \param source_buf     Pointer to the Source Buffer from which the parameter
*                       has to be extracted.
* \param buf_size       Size of the source buffer.
* \param type           Type of the Parameter.

* \return None
* \see
* \note
******************************************************************************/

S32 iAP2GetParam(void* target_buffer, U8* source_buf, U16 buf_size, iAP2_Type type);

/**************************************************************************//**
* Allocates memory to the destination address & copies the data from the source.
* for the other iAP2Types such as iAP2_bool, iAP2_enum, iAP2_group, iAP2_none it
* returns IAP2_BAD_PARAMETER.
*
* \param dest_ptr   Address to which the memory has to be allocated & the data
*                   to be copied.
* \param src_ptr    Address from which the data has to be copied.
* \param dest_count Variable to which the total data copied to be updated.
* \param src_count  Total times the source data is available.
*
* \return IAP2_OK            On Successful completion of copying the data to
*                            destination from source.
* \return IAP2_ERR_NO_MEM    When unable to allocate the memory.
* \return IAP2_BAD_PARAMETER When utility function could not handle the iAP2_Type
*                            provided.
* \see
* \note
******************************************************************************/

S32 iAP2AllocateandUpdateData(void* dest_ptr, void* src_ptr, U16* dest_count, U16 src_count, iAP2_Type iAP2Type, U32 parametersize);

/**************************************************************************//**
* Frees the memory allocated for the parameters.
*
* \param iAP2PtrtoFree      Parameter which has to be freed.
* \param iAP2ParameterCount Count of the current parameter.
* \param iAP2DbgStr         Debug string that has to be provided to DLT.
* \param iAP2type           Parameter Type.
*
* \return None.
* \see
* \note
******************************************************************************/
void iAP2FreeiAP2Pointer(void** iAP2PtrtoFree, U16* iAP2ParameterCount, S8* iAP2DbgStr, iAP2_Type iAP2type);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
