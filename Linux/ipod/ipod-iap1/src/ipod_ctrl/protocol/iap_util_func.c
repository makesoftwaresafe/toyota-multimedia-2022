/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */
/**************************************************************************
 *	\File : iap_util_func.c
 *
 ************************************************************************/

#include <stdlib.h>
#include "iap_util_func.h"
#include "ipodcommon.h"
#include "iap_general.h"
#include "iap1_dlt_log.h"

/*************************************************************************
 * \Func:  iPod_get_error_code
 *************************************************************************/
S32 iPod_get_error_code(S32 error)
{

    S32 err = IPOD_OK;

    switch(error)
    {
        case IPOD_ACKERR_UNKNOWN_DB_CATEGORY:
                  err = IPOD_UNKNOWN_DATABASE_CATEGORY;
                  break;
        case IPOD_ACKERR_CMD_FAILED:
                  err = IPOD_COMMAND_FAILED;
                  break;
        case IPOD_ACKERR_OUT_OF_RESOURCES:
                  err = IPOD_OUT_OF_RESOURCES;
                  break;
        case IPOD_ACKERR_BAD_PARAM:
                  err = IPOD_BAD_PARAMETER;
                  break;
        case IPOD_ACKERR_UNKNOWN_ID:
                  err = IPOD_UNKNOWN_ID;
                  break;
        case IPOD_ACKERR_ACK_PENDING:
                  err = error;
                  break;
        case IPOD_ACKERR_ACC_NOT_AUTHENTICATED:
                  err = IPOD_ACCESSORY_NOT_AUTHENTICATED;
                  break;
        case IPOD_ACKERR_BAD_AUTHENTICATION_VERSION:
                  err = IPOD_BAD_AUTHENTICATION_VERSION;
                  break;
        case IPOD_ACKERR_POWER_MODE_FAILED:
                  err = IPOD_ACC_POWER_MODE_FAILED;
                  break;
        case IPOD_ACKERR_CERT_INVALID:
                  err = IPOD_CERTIFICATE_INVALID;
                  break;
        case IPOD_ACKERR_CERT_PERMISSIONS_INVALID:
                  err = IPOD_CERTIFICATE_PERMISSIONS_INVALID;
                  break;
        case IPOD_ACKERR_FILE_IS_IN_USE:
                  err = IPOD_FILE_IS_IN_USE;
                  break;
        case IPOD_ACKERR_INVALID_FILE_HANDLE:
                  err = IPOD_INVALID_FILE_HANDLE;
                  break;
        case IPOD_ACKERR_DIR_NOT_EMPTY:
                  err = IPOD_DIR_NOT_EMPTY;
                  break;
        case IPOD_ACKERR_OPERATION_TMOUT:
                  err = IPOD_OPERATION_TMOUT;
                  break;
        case IPOD_ACKERR_UNAVAILABLE_MODE:
                  err = IPOD_UNAVAILABLE_MODE;
                  break;
        case IPOD_ACKERR_INVALID_ACC_RESID_VALUE:
                  err = IPOD_INVALID_ACC_RESID_VALUE;
                  break;
        case IPOD_ACKERR_IPOD_MAX_CONNECT:
                  err = IPOD_MAX_CONNECT;
                  break;
        case IPOD_ACKERR_SESSION_WRITE_FAILURE:
                  err = IPOD_SESSION_WRITE_FAILURE;
                  break;
        case IPOD_ACKERR_IPOD_NOT_CONNECTED:
                  err = IPOD_NOT_CONNECTED;
                  break;
        default:
                  err = error;
                  break;
    }
    return err;
}

U8* iPod_get_error_msg(S32 error)
{

    U8 *msg = (U8*)"iPod OK";
    switch(error)
    {
        case IPOD_ACKERR_UNKNOWN_DB_CATEGORY:
                  msg = (U8*)"Unknown DB category";
                  break;
        case IPOD_ACKERR_CMD_FAILED:
                  msg = (U8*)"Command Failed";
                  break;
        case IPOD_ACKERR_OUT_OF_RESOURCES:
                  msg = (U8*)"Out of resources";
                  break;
        case IPOD_ACKERR_BAD_PARAM:
                  msg = (U8*)"Bad Parameter";
                  break;
        case IPOD_ACKERR_UNKNOWN_ID:
                  msg = (U8*)"Unknown Id";
                  break;
        case IPOD_ACKERR_ACK_PENDING:
                  msg = (U8*)"Acknowledgement Pending";
                  break;
        case IPOD_ACKERR_ACC_NOT_AUTHENTICATED:
                  msg = (U8*)"Accessory Not Authenticated";
                  break;
        case IPOD_ACKERR_BAD_AUTHENTICATION_VERSION:
                  msg = (U8*)"Bad Authentication Version";
                  break;
        case IPOD_ACKERR_POWER_MODE_FAILED:
                  msg = (U8*)"Power Mode Failed";
                  break;
        case IPOD_ACKERR_CERT_INVALID:
                  msg = (U8*)"Invalid Certificate";
                  break;
        case IPOD_ACKERR_CERT_PERMISSIONS_INVALID:
                  msg = (U8*)"Invalid Certificate Permissions";
                  break;
        case IPOD_ACKERR_FILE_IS_IN_USE:
                  msg = (U8*)"File is in Use";
                  break;
        case IPOD_ACKERR_INVALID_FILE_HANDLE:
                  msg = (U8*)"Invalid File Handle";
                  break;
        case IPOD_ACKERR_DIR_NOT_EMPTY:
                  msg = (U8*)"Directory Not empty";
                  break;
        case IPOD_ACKERR_OPERATION_TMOUT:
                  msg = (U8*)"Operation Timeout";
                  break;
        case IPOD_ACKERR_UNAVAILABLE_MODE:
                  msg = (U8*)"Unavailable Mode";
                  break;
        case IPOD_ACKERR_INVALID_ACC_RESID_VALUE:
                  msg = (U8*)"Invalid Accessory Resid Value";
                  break;
        case IPOD_ACKERR_IPOD_MAX_CONNECT:
                  msg = (U8*)"Maximum Connected";
                  break;
        case IPOD_ACKERR_SESSION_WRITE_FAILURE:
                  msg = (U8*)"Session write failure";
                  break;
        case IPOD_ACKERR_IPOD_NOT_CONNECTED:
                  msg = (U8*)"iPod Not Connected";
                  break;
        default:
                  msg = (U8*)"Unknown Error";
                  break;
    }
    return msg;
}

/*!
 * \fn iPod_convert_to_little16(U8 *buf)
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 16 bits data.<br>
 * \par REPLY PARAMETERS
 * S32 little - Value of little endian 
 * \par DESCRIPTION
 * This function convert the 16 bits big endian to little endian.
 */
U16 iPod_convert_to_little16(const U8 *buf)
{
    U16 little = 0;
    little = ((((U16)buf[0]) << IPOD_SHIFT_8) | (U16)buf[1]);
    
    return little;
}

/*!
 * \fn iPod_convert_to_little32(U8 *buf)
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 32 bits data.<br>
 * \par REPLY PARAMETERS
 * S32 little - Value of little endian 
 * \par DESCRIPTION
 * This function convert the 32 bits big endian to little endian.
 */
U32 iPod_convert_to_little32(const U8 *buf)
{
    U32 little = 0;
    
    little  = (((U32)buf[IPOD_POS0]) << IPOD_SHIFT_24);
    little |= (((U32)buf[IPOD_POS1]) << IPOD_SHIFT_16);
    little |= (((U32)buf[IPOD_POS2]) << IPOD_SHIFT_8);
    little |= (U32)buf[IPOD_POS3];

    return little;
}

/*!
 * \fn iPod_convert_to_little64(U8 *buf)
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 64 bits data.<br>
 * \par REPLY PARAMETERS
 * S32 little - Value of little endian 
 * \par DESCRIPTION
 * This function convert the 64 bits big endian to little endian.
 */
U64 iPod_convert_to_little64(const U8 *buf)
{
    U64 little = 0;
    little = (((U64)buf[IPOD_POS0]) << IPOD_SHIFT_56);
    little |= (((U64)buf[IPOD_POS1]) << IPOD_SHIFT_48);
    little |= (((U64)buf[IPOD_POS2]) << IPOD_SHIFT_40);
    little |= (((U64)buf[IPOD_POS3]) << IPOD_SHIFT_32);
    little |= (((U64)buf[IPOD_POS4]) << IPOD_SHIFT_24);
    little |= (((U64)buf[IPOD_POS5]) << IPOD_SHIFT_16);
    little |= (((U64)buf[IPOD_POS6]) << IPOD_SHIFT_8);
    little |= (U64)buf[IPOD_POS7];
    return little;
}

/*!
 * \fn iPod_convert_to_big16(U8 *msg, U16 data)
 * \par INPUT PARAMETERS
 * U16 data - This is data of 16 bits little endian
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 16 bits big endian.<br>
 * \par DESCRIPTION
 * This function convert the 16 bits little endian to big endian.
 */
void iPod_convert_to_big16(U8 *msg, U16 data)
{
/*check*/
    msg[IPOD_POS0] = (U8)((data & IPOD_BITS_8_15) >> IPOD_SHIFT_8);
    msg[IPOD_POS1] = (U8)(data & IPOD_BITS_0_7);
}

/*!
 * \fn iPod_convert_to_big32(U8 *msg, U32 data)
 * \par INPUT PARAMETERS
 * U16 data - This is data of 32 bits little endian
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 32 bits big endian.<br>
 * \par DESCRIPTION
 * This function convert the 32 bits little endian to big endian.
 */
void iPod_convert_to_big32(U8 *msg, U32 data)
{
    
    msg[IPOD_POS0] = (U8)((data & IPOD_BITS_24_31) >> IPOD_SHIFT_24);
    msg[IPOD_POS1] = (U8)((data & IPOD_BITS_16_23) >> IPOD_SHIFT_16);
    msg[IPOD_POS2] = (U8)((data & IPOD_BITS_8_15) >> IPOD_SHIFT_8);
    msg[IPOD_POS3] = (U8)(data & IPOD_BITS_0_7);
}

/*!
 * \fn iPod_convert_to_big64(U8 *msg, U64 data)
 * \par INPUT PARAMETERS
 * U16 data - This is data of 64 bits little endian
 * \par INOUT PARAMETERS
 * U8 buf - This is a pointer of 64 bits big endian.<br>
 * \par DESCRIPTION
 * This function convert the 64 bits little endian to big endian.
 */
void iPod_convert_to_big64(U8 *msg, U64 data)
{
    
    msg[IPOD_POS0] = (U8)((data & IPOD_BITS_56_63) >> IPOD_SHIFT_56);
    msg[IPOD_POS1] = (U8)((data & IPOD_BITS_48_55) >> IPOD_SHIFT_48);
    msg[IPOD_POS2] = (U8)((data & IPOD_BITS_40_47) >> IPOD_SHIFT_40);
    msg[IPOD_POS3] = (U8)((data & IPOD_BITS_32_39) >> IPOD_SHIFT_32);
    msg[IPOD_POS4] = (U8)((data & IPOD_BITS_24_31) >> IPOD_SHIFT_24);
    msg[IPOD_POS5] = (U8)((data & IPOD_BITS_16_23) >> IPOD_SHIFT_16);
    msg[IPOD_POS6] = (U8)((data & IPOD_BITS_8_15) >> IPOD_SHIFT_8);
    msg[IPOD_POS7] = (U8)(data & IPOD_BITS_0_7);
}

/*!
 * \fn iPod_convert_macaddr_to_bigendian(U8 *msg, U8 *data)
 * \par INPUT PARAMETERS
 * U8* data - This is data of 6 bytes of Macaddress in little endian format.
 * \par INOUT PARAMETERS
 * U8* msg - This is a pointer to hold 6 bytes of big endian format.<br>
 * \par DESCRIPTION
 * This function convert the 6 bytes of little endian to big endian.
 */
void iPod_convert_macaddr_to_bigendian(U8 *msg, U8 *data)
{
    msg[IPOD_POS0] = data[IPOD_POS5];
    msg[IPOD_POS1] = data[IPOD_POS4];
    msg[IPOD_POS2] = data[IPOD_POS3];
    msg[IPOD_POS3] = data[IPOD_POS2];
    msg[IPOD_POS4] = data[IPOD_POS1];
    msg[IPOD_POS5] = data[IPOD_POS0];
}

S32 GetAndSetMsg(IPOD_INSTANCE* iPodHndl, U8 *msg)
{
    S32 rc = IPOD_OK;

    if(msg != NULL)
    {
        rc = iPodSendCommand(iPodHndl, msg);
        IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "iPodSendCommand() returns:rc = %d",rc);
    }
    else
    {
        rc = IPOD_ERR_NOMEM;
        IAP1_GENERAL_LOG(DLT_LOG_ERROR, "No Memory msg is NULL");

    }

    if (rc == IPOD_OK)
    {
        memset(iPodHndl->iAP1Buf, 0, iPodHndl->iAP1MaxPayloadSize);
        rc = iPodWaitAndGetResponseFixedSize(iPodHndl, iPodHndl->iAP1Buf);
        IAP1_GENERAL_LOG(DLT_LOG_DEBUG, "iPodWaitAndGetResponseFixedSize() returns:rc = %d",rc);
    }

    return rc;
}
