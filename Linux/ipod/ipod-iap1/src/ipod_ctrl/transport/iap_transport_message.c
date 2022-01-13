/* -----------------------------------------------------------------------------
 * An invalid character is automatically inserted by cvs when the file is
 * commited. We can not do anything about it.
 * -----------------------------------------------------------------------------
 */

#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */

#include <string.h>

#include <signal.h>

#include "iap_transport_authentication.h"
#include "ipodcommon.h"
#include "iap_callback.h"
#include "iap1_dlt_log.h"
#include "iap_commands.h"
#include "iap_general.h"
#include "iap_init.h"
#include "iap_util_func.h"
#include "iap_devconf.h"
#include "iap_transport_message.h"
//#include "iap_transport_dependent.h"
#include "iap_transport_process_message.h"
#include "iap_transport_configuration.h"

EXPORT IPOD_INSTANCE*   gp_iPodInst     = NULL;
EXPORT IPOD_IOS_INSTANCE* gp_iPodiOSInst = NULL;
EXPORT IPOD_SEM_ID      g_semSenderId   = 0;    /* semaphore to handle send commands */
IPOD_DATACOM_FUNC_TABLE g_data_com_function_table[IPOD_MAX_CONNECT_TYPE];

LOCAL void delete_all_buffers(IPOD_INSTANCE* iPodHndl);
LOCAL S32  create_all_semaphores(IPOD_INSTANCE* iPodHndl);
LOCAL S32  delete_all_semaphores(IPOD_INSTANCE* iPodHndl);
LOCAL S32  create_eventflag(IPOD_INSTANCE* iPodHndl);
LOCAL S32  delete_eventflag(IPOD_INSTANCE* iPodHndl);
LOCAL U8   iPodCalcChecksum(const U8* msg);
LOCAL U8   iPodCalcChecksumLongTelegram(const U8* msg);
LOCAL S32  iPodGetFreeInstance(U8* devicename);

LOCAL void iPodGetiAPInfo(MessageHeaderInfoType *MessageHeaderInfo, const U8 *header, U16 *dataStartPos);
LOCAL void iPodGetiAPAckInfo(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType *MessageHeaderInfo, const U8 *header);
LOCAL void iPodGetIDPSInfo(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType *MessageHeaderInfo, U16 *dataStartPos);
LOCAL void iPodAddTransID(IPOD_INSTANCE* iPodHndl, U8 *buf, IPOD_SENDMSGINFO* sendInfo);

LOCAL void iPodReaderTask(void* exinf);

LOCAL S32  iPodWaitForResponse(IPOD_INSTANCE* iPodHndl);
LOCAL void iPodGetTelegramHeaderInfo(MessageHeaderInfoType *MessageHeaderInfo, IPOD_INSTANCE* iPodHndl);

/* end internal functions */


/**
 * \addtogroup TransportLayer_commands
 */
/*\{*/

void iPodSetStartIDPS(IPOD_INSTANCE* iPodHndl, U8 idps)
{
    if(iPodHndl != NULL)
    {
        if(idps != FALSE)
        {
            iPodHndl->rcvMsgInfo.iPodTransID = 0;
            iPodHndl->rcvMsgInfo.accTransID = 0;
            iPodHndl->rcvMsgInfo.expectedTransID = 0;
        }
        
        iPodHndl->rcvMsgInfo.startIDPS = idps;
        if(idps == TRUE)
        {
            /* set TRUE to support CancelCmd for new authentication */
            iPodHndl->rcvMsgInfo.supportCancelCmd = TRUE;
        }
    }
}

S32 iPodCheckConnected(U8* devicename)
{
    S32 rc = IPOD_OK;
    S32 i  = 0;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    rc = iPodWaitForSenderSemaphore();
    if (IPOD_OK != rc)
    {
        rc = IPOD_ERR_TMOUT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - Timeout");
    }
    if ((NULL != gp_iPodInst)&&(IPOD_OK == rc))
    {
        for (i = 0; (i < dcInfo[IPOD_DC_INST_COUNT].para.val) && (rc == IPOD_OK); i++)
        {
            if (strncmp((VP)gp_iPodInst[i].name, (VP)devicename, sizeof(gp_iPodInst[i].name)) == 0)
            {
                rc = IPOD_ALREADY_CONNECTED;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Already Connected");
            }
        }
    }
    if (IPOD_ERR_TMOUT != rc)
    {
        (void)iPodSigSenderSemaphore();
    }

    return rc;
}


LOCAL S32 iPodGetFreeInstance(U8* devicename)
{
    S32 rc = IPOD_OK;
    S32 i  = 0;
    S32 emptyField = IPOD_ERROR;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    rc = iPodWaitForSenderSemaphore();
    if (IPOD_OK != rc)
    {
        rc = IPOD_ERR_TMOUT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - Timeout");
    }
    if ((NULL != gp_iPodInst)&&(rc == IPOD_OK))
    {
        for (i = 0; (i < dcInfo[IPOD_DC_INST_COUNT].para.val) && (rc == IPOD_OK); i++)
        {
            if ((gp_iPodInst[i].name[0] == '\0')&&(emptyField == IPOD_ERROR))
            {
                emptyField = i;
            }
            else
            {
                if (strncmp((VP)gp_iPodInst[i].name, (VP)devicename, sizeof(gp_iPodInst[i].name)) == 0)
                {
                    rc = IPOD_ALREADY_CONNECTED;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Already Connected");
                }
            }
        }
    }

    /* There is an empty field and this device was not initialized yet */
    if((emptyField != IPOD_ERROR)&&(rc == IPOD_OK))
    {
        IPOD_INSTANCE*  iPod      = &gp_iPodInst[emptyField];

        memset(iPod, 0, sizeof(*iPod));
        strncpy((VP)iPod->name, (VP)devicename, sizeof(iPod->name) - 1);
        iPodOSClearTaskID(&(iPod->readerTaskId));

        rc = emptyField + 1;
    }
    /* The device was not initialized yet but there is no free field left */
    else if ((emptyField == IPOD_ERROR)&&(rc == IPOD_OK))
    {
        rc = IPOD_ERR_NOMEM;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "The device was not initialized yet but there is no free field left");
    }
    /* else device is already initialized and rc is IPOD_ALREADY_CONNECTED */

    if (IPOD_ERR_TMOUT != rc)
    {
        (void)iPodSigSenderSemaphore();
    }

    return rc;
}

IPOD_INSTANCE* iPodGetHandle(U32 iPodID)
{
    IPOD_INSTANCE* rc = NULL;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    if ((iPodID > 0) && (iPodID <= (U32)dcInfo[IPOD_DC_INST_COUNT].para.val))
    {
        rc = &gp_iPodInst[iPodID-1];
    }

    return rc;
}

S32 iPodTrnspInitPlugins(void)
{
    S32 rc = IPOD_OK;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

#ifdef IPOD_USB_HOST_PLUGIN
    rc = iPodUSBHostComInit(&g_data_com_function_table[IPOD_USB_HOST_CONNECT], (U32)dcInfo[IPOD_DC_INST_COUNT].para.val);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodUSBHostComInit() returns : rc = %d",rc);
#endif
#ifdef IPOD_BT_PLUGIN
    if (IPOD_OK == rc)
    {
        rc = iPodBTComInit(&g_data_com_function_table[IPOD_BT_CONNECT], (U32)dcInfo[IPOD_DC_INST_COUNT].para.val);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodBTComInit() returns : rc = %d",rc);
    }
    else
    {
#ifdef IPOD_USB_HOST_PLUGIN
        (void)iPodUSBHostComDeinit(&g_data_com_function_table[IPOD_USB_HOST_CONNECT]);
#endif
    }
#endif
#ifdef IPOD_UART_PLUGIN
    if (IPOD_OK == rc)
    {
        rc = iPodUARTComInit(&g_data_com_function_table[IPOD_UART_CONNECT], (U32)dcInfo[IPOD_DC_INST_COUNT].para.val);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodUARTComInit() returns : rc = %d",rc);
    }
    else
    {
#ifdef IPOD_USB_HOST_PLUGIN
        (void)iPodUSBHostComDeinit(&g_data_com_function_table[IPOD_USB_HOST_CONNECT]);
#endif
#ifdef IPOD_BT_PLUGIN
        (void)iPodBTComDeinit(&g_data_com_function_table[IPOD_BT_CONNECT]);
#endif
    }
#endif
    return rc;
}

S32 iPodTrnspDeinitPlugins(void)
{
    S32 rc = IPOD_OK;

#ifdef IPOD_USB_HOST_PLUGIN
    rc = iPodUSBHostComDeinit(&g_data_com_function_table[IPOD_USB_HOST_CONNECT]);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodUSBHostComDeinit() returns : rc = %d",rc);
#endif
#ifdef IPOD_BT_PLUGIN
    rc = iPodBTComDeinit(&g_data_com_function_table[IPOD_BT_CONNECT]);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodBTComInit() returns : rc = %d",rc);
#endif
#ifdef IPOD_UART_PLUGIN
    rc = iPodUARTComDeinit(&g_data_com_function_table[IPOD_UART_CONNECT]);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iPodUARTComInit() returns : rc = %d",rc);
#endif

    return rc;
}

S32 iPodTrnspOpenPlugin(IPOD_INSTANCE* iPodHndl, IPOD_CONNECTION_TYPE connection)
{
    S32 rc = IPOD_OK;
    IPOD_TRANSPORT* transport = &iPodHndl->transport;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = &transport->data_com_functions;

    switch(connection)
    {
    case IPOD_USB_HOST_CONNECT:
#ifndef IPOD_USB_HOST_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "USB Host - Unsupported Device");
#endif
        break;
    case IPOD_USB_FUNC_CONNECT:
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "USB Function - Unsupported Device");
        break;
    case IPOD_BT_CONNECT:
#ifndef IPOD_BT_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bluetooth - Unsupported Device");
#endif
        break;
    case IPOD_UART_CONNECT:
#ifndef IPOD_UART_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "UART - Unsupported Device");
#endif
        break;
    default:
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Unsupported Device");
        break;
    }

    if (rc == IPOD_OK)
    {
        /* initialize the functions pointers */
        data_com_function->open = g_data_com_function_table[connection].open;
        data_com_function->close = g_data_com_function_table[connection].close;
        data_com_function->abort = g_data_com_function_table[connection].abort;
        data_com_function->write = g_data_com_function_table[connection].write;
        data_com_function->read = g_data_com_function_table[connection].read;
        data_com_function->ioctl = g_data_com_function_table[connection].ioctl;
        data_com_function->property = g_data_com_function_table[connection].property;

        transport->iPodHndl = iPodHndl;
        if(data_com_function->open != NULL)
        {
            rc = data_com_function->open(iPodHndl->name, IPOD_DATACOM_FLAGS_RDWR, IPOD_DATACOM_MODE_BLOCKING);
            if (rc >= 0)
            {
                transport->devInfo = rc;
                rc = IPOD_OK;
            }
            else
            {
                transport->devInfo = IPOD_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->open () returns rc = %d",rc);
            }
        }
        else
        {
            transport->devInfo = IPOD_ERROR;
            rc = IPOD_BAD_PARAMETER;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->open is NULL");
        }
    }

    return rc;
}

S32 iPodTrnspClosePlugin(IPOD_INSTANCE* iPodHndl)
{
    S32 rc = IPOD_OK;
    IPOD_TRANSPORT* transport = &iPodHndl->transport;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = &transport->data_com_functions;

    switch(iPodHndl->connection)
    {
    case IPOD_USB_HOST_CONNECT:
#ifndef IPOD_USB_HOST_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "USB Host - Unsupported Device");
#endif
        break;
    case IPOD_USB_FUNC_CONNECT:
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "USB Function - Unsupported Device");
        break;
    case IPOD_BT_CONNECT:
#ifndef IPOD_BT_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bluetooth - Unsupported Device");
#endif
        break;
    case IPOD_UART_CONNECT:
#ifndef IPOD_UART_PLUGIN
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "UART - Unsupported Device");
#endif
        break;
    default:
        rc = IPOD_ERR_UNSUP_DEV;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Unsupported Device");
        break;
    }

    if ((rc == IPOD_OK)&&(transport->devInfo != IPOD_ERROR))
    {
        if (data_com_function->abort != NULL)
        {
            (void)data_com_function->abort(transport->devInfo);
        }
        if (data_com_function->close != NULL)
        {
            rc = data_com_function->close(transport->devInfo);
        }
    }

    if (rc == IPOD_OK)
    {
        transport->devInfo = IPOD_ERROR;

        /* uninitialize the functions pointers */
        data_com_function->open = NULL;
        data_com_function->close = NULL;
        data_com_function->abort = NULL;
        data_com_function->write = NULL;
        data_com_function->read = NULL;
        data_com_function->ioctl = NULL;
        data_com_function->property = NULL;

        transport->iPodHndl = NULL;
    }

    return rc;
}

/*!
 * \fn iPodTransportInitConnection
 * \par INPUT PARAMETERS
 * U8* devicename - NULL to use devconf setting or name of device where the iPod is connected (e.g. /dev/usb/hiddev0 for Linux)
 * IPOD_CONNECTION_TYPE connection
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_OK                Completed successfully
 * \li \c \b IPOD_ERR_ID            dd is invalid or not open
 * \li \c \b IPOD_ALREADY_CONNECTED iPod is already connected
 * \li \c \b IPOD_ERR_OACV          Open mode is invalid (write not permitted)
 * \li \c \b IPOD_ERR_RONLY         Read-only device
 * \li \c \b IPOD_ERR_LIMIT         Number of requests exceeds the limit
 * \li \c \b IPOD_ERR_TMOUT         Busy processing other requests
 * \li \c \b IPOD_ERR_ABORT         Processing aborted
 * \li \c \b IPOD_ERR_NOMEM         message buffer couldn't be created
 * \li \c \b IPOD_ERR_NOEXS         Object does not exist (the message buffer or
 *                                  task specified in mbfid does not exist)
 * \par DESCRIPTION
 * This function initializes the Transport Layer connection.
 */
S32 iPodTransportInitConnection(U8* devicename, IPOD_CONNECTION_TYPE connection)
{
    S32 rc      = IPOD_OK;
    S32 iPodID  = 0;
    IPOD_INSTANCE* iPod = NULL;
    IPOD_TRANSPORT* transport = NULL;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = NULL;
    IPOD_Cfg *dcInfo = iPodGetDevInfo();

    if (IPOD_NO_CONNECT != connection)
    {
        iPodID = iPodGetFreeInstance(devicename);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPodGetFreeInstance() returns iPodID = %d",iPodID);
    }

    if (0 < iPodID)
    {
        iPod = &gp_iPodInst[iPodID-1];
        iPod->rcvMsgInfo.sendCmdId         =  IPOD_ID_DEFAULT;
        iPod->rcvMsgInfo.expectedCmdId     =  IPOD_ID_DEFAULT;
        iPod->rcvMsgInfo.expectedLingo     =  IPOD_LINGO_DEFAULT;

        iPod->connection  = connection;
        iPod->id          = iPodID;

        iPod->iPodRetAccInfoData = NULL;
        iPod->iPodRetAccInfoFlg = 0;

        iPod->iAP1MaxPayloadSize = (U16)dcInfo[IPOD_DC_MAX_PAYLOAD_SIZE].para.val;
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "iAP1MaxPayloadSize = 0x%X", iPod->iAP1MaxPayloadSize);

        /* If connection type is USB, allocate the buffer for one packet data */
        if((iPod->connection == IPOD_USB_HOST_CONNECT) ||
            (iPod->connection == IPOD_USB_FUNC_CONNECT) ||
            (iPod->connection == IPOD_BT_CONNECT))
        {
            if(iPod->packet == NULL)
            {
                iPod->packet  = calloc(iPod->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5 + IPOD_USB_LINK_CTRL_BYTE_SIZE, sizeof(U8));
                iPod->iAP1Buf = calloc(iPod->iAP1MaxPayloadSize, sizeof(U8));
                if( (iPod->packet == NULL) || (iPod->iAP1Buf == NULL) )
                {
                    rc = IPOD_ERR_NOMEM;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory iPod->packet / iPod->iAP1Buf is NULL");
                }
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter iPod->packet = %p",iPod->packet);
            }
        }

        transport = &iPod->transport;
        data_com_function = &transport->data_com_functions;

        transport->transportMaxPayloadSize = IPOD_TRANSPORT_DEFAULT_MAX_PAYLOAD_SIZE;

        /* initilize supported LINGOS */
        memset(transport->options, 0, sizeof(transport->options));
        transport->options[IPOD_LINGO_GENERAL].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_GENERAL].supported.minor_ver = IPOD_LINGO_GENERAL_MIN_SUPPORT;
        transport->options[IPOD_LINGO_SIMPLE_REMOTE].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_SIMPLE_REMOTE].supported.minor_ver = IPOD_LINGO_SIMPLE_MIN_SUPPORT;
        transport->options[IPOD_LINGO_DISPLAY_REMOTE].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_DISPLAY_REMOTE].supported.minor_ver = IPOD_LINGO_DISPLAY_MIN_SUPPORT;
        transport->options[IPOD_LINGO_EXTENDED_INTERFACE].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_EXTENDED_INTERFACE].supported.minor_ver = IPOD_LINGO_EXTENDED_MIN_SUPPORT;
        transport->options[IPOD_LINGO_DIGITAL_AUDIO].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_DIGITAL_AUDIO].supported.minor_ver = IPOD_LINGO_DIGITAL_MIN_SUPPORT;
        transport->options[IPOD_LINGO_STORAGE].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_STORAGE].supported.minor_ver = IPOD_LINGO_STORAGE_MIN_SUPPORT;
        transport->options[IPOD_LINGO_OUT].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_OUT].supported.minor_ver = IPOD_LINGO_OUT_MIN_SUPPORT;
        transport->options[IPOD_LINGO_LOCATION].supported.major_ver = IPOD_LINGO_SUPPORT_MAJ_VERSION1;
        transport->options[IPOD_LINGO_LOCATION].supported.minor_ver = IPOD_LINGO_LOCATION_MIN_SUPPORT;

        
        if(rc == IPOD_OK)
        {
            rc = iPodWaitForSenderSemaphore(); /* avoid overlap with disconnect because it frees memory that is accessed here */
            if (IPOD_OK == rc)
            {
                rc = iPodTrnspOpenPlugin(iPod, iPod->connection);

                iPodSigSenderSemaphore(); /* avoid overlap with disconnect because it frees memory that is accessed here */
            }
        }
        
        if (IPOD_OK == rc)
        {
            iPod->isConnected = TRUE;

            rc = iPodCreateReaderTask(iPod);
            if (IPOD_OK != rc)
            {
                if (data_com_function->abort != NULL)
                {
                    data_com_function->abort(transport->devInfo);
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Connection Aborted");
                }
                if (data_com_function->close != NULL)
                {
                    data_com_function->close(transport->devInfo);
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Connection Closed");
                }
            }
        }

        if (IPOD_OK != rc)
        {
            iPod->id           = 0;
            iPodOSClearTaskID(&(iPod->readerTaskId));
            iPod->name[0]      = '\0';

            /* return error */
            iPodID = rc;
        }
    } /* (0 < iPodID) */

    return iPodID;
}


/*!
 * \fn iPodTransportDisconnect
 * \par INPUT PARAMETERS
 * U8 stop_flag
 * \par REPLY PARAMETERS
 * none
 * \par DESCRIPTION
 * This function terminates all tasks.
 */
void iPodTransportDisconnect(U32 iPodID)
{
    IPOD_INSTANCE* iPod = NULL;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    if ((iPodID > 0) && (iPodID <= (U32)dcInfo[IPOD_DC_INST_COUNT].para.val) &&
        (NULL != gp_iPodInst))
    {
        iPod = &gp_iPodInst[iPodID-1];
    }

    if (NULL != iPod)
    {
        IPOD_TRANSPORT* transport = &iPod->transport;
        IPOD_DATACOM_FUNC_TABLE* data_com_function = &transport->data_com_functions;

        /* stops the reader task */
        if (data_com_function->abort != NULL)
        {
            (void)data_com_function->abort(transport->devInfo);
        }

        if ((IPOD_OK == iPodOSVerifyTaskID(iPod->readerTaskId)) && (TRUE == iPod->isConnected))
        {
            (void)iPodOSJoinTask(iPod->readerTaskId);
            iPod->isConnected = FALSE;
            iPodOSClearTaskID(&(iPod->readerTaskId));
        }
        
        iPod->name[0] = '\0';
        
        if(iPod->packet != NULL)
        {
            free(iPod->packet);
            iPod->packet = NULL;
        }
        if(iPod->iAP1Buf != NULL)
        {
            free(iPod->iAP1Buf);
            iPod->iAP1Buf = NULL;
        }

        if(iPod->iPodRetAccInfoData != NULL)
        {
            free(iPod->iPodRetAccInfoData);
            iPod->iPodRetAccInfoData = NULL;
        }
    } /* NULL != iPod */
}

/*!
 * \fn iPodSetExpectedCmdId
 * \par INPUT PARAMETERS
 * \li \c \b U16 cmdId
 * \li \c \b U8  lingo
 * \par REPLY PARAMETERS
 * none
 * \par DESCRIPTION
 * This function sets the expected command ID.
 */
void iPodSetExpectedCmdId(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8  lingo)
{
    iPodSetExpectedCmdIdEx(iPodHndl, cmdId, lingo, 1);
}


/*!
 * \fn iPodSetExpectedCmdIdEx
 * \par INPUT PARAMETERS
 * \li \c \b U16 cmdId
 * \li \c \b U8  lingo
 * \li \c \b U32 cnt
 * \par REPLY PARAMETERS
 * none
 * \par DESCRIPTION
 * This function sets the expected command ID.
 */
void iPodSetExpectedCmdIdEx(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8  lingo, U32 cnt)
{
    S32 rc = IPOD_OK;

    if(iPodHndl != NULL)
    {
        rc = iPodWaitForSenderSemaphore();
        if (IPOD_OK != rc)
        {
            rc = IPOD_ERR_TMOUT;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - Timeout");
        }

        if ((FALSE != iPodHndl->detected)&&(IPOD_OK == rc))
        {
            iPodHndl->rcvMsgInfo.expectedLingo =  lingo;
            iPodHndl->rcvMsgInfo.expectedCmdId =  cmdId;
            iPodHndl->rcvMsgInfo.waitingCmdCnt =  (S32)cnt;
            if((iPodHndl->rcvMsgInfo.startIDPS != FALSE) && (cnt != 0))
            {
                iPodHndl->rcvMsgInfo.expectedTransID = iPodHndl->rcvMsgInfo.accTransID;
            }
            if (cnt == 0)
            {
                iPodHndl->rcvMsgInfo.sendCmdId  = IPOD_ID_DEFAULT;
            }
        }
        if (IPOD_ERR_TMOUT != rc)
        {
            (void)iPodSigSenderSemaphore();
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodHndl is NULL");
    }
}


/*!
 * \fn iPodIncreaseExpectedCmdIdEx
 * \par INPUT PARAMETERS
 * \li \c \b U16 cmdId
 * \li \c \b U8  lingo
 * \li \c \b U32 cnt
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_OK                Normal completion
 * \li \c \b IPOD_ERROR             lingo and command ID does not match
 * \li \c \b IPOD_NOT_CONNECTED     iPod is not connected
 * \li \c \b IPOD_ERR_TMOUT         Polling failed or timeout
 * \par DESCRIPTION
 * This function increase the count of expected callbacks for a specified command ID.
 */
S32 iPodIncreaseExpectedCmdIdEx(IPOD_INSTANCE* iPodHndl, U16 cmdId, U8  lingo, U32 cnt)
{
    S32 rc = IPOD_OK;

    rc = iPodWaitForSenderSemaphore();
    if (IPOD_OK == rc)
    {
        if (FALSE != iPodHndl->detected)
        {
            if((iPodHndl->rcvMsgInfo.expectedLingo == lingo) && (iPodHndl->rcvMsgInfo.expectedCmdId == cmdId))
            {
                iPodHndl->rcvMsgInfo.waitingCmdCnt += (S32)cnt;
            }
            else
            {
                rc = IPOD_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error iPodHndl->rcvMsgInfo.expectedLingo = %d iPodHndl->rcvMsgInfo.expectedCmdId = %d"
                                            ,iPodHndl->rcvMsgInfo.expectedLingo,iPodHndl->rcvMsgInfo.expectedCmdId);
            }
        }
        else
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
        (void)iPodSigSenderSemaphore();
    }
    if (rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodWaitForSenderSemaphore() returns rc = %d",rc);
    }

    return rc;
}


/*!
 * \fn iPodGetValuesOfLenCmdInfo
 * \par INPUT PARAMETERS
 * \li \c \b  U32  len
 * \par OUTPUT PARAMETERS
 * \li \c \b  U16 *telegramLen
 * \li \c \b  U16 *telegramCmdId
 * \li \c \b  U8  *telegramLingo
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 *  none
 * \par DESCRIPTION
 * This function gets values of command info.
 */
void iPodGetValuesOfLenCmdInfo(IPOD_INSTANCE* iPodHndl, 
                               U16 *telegramLen,
                               U16 *telegramCmdId,
                               U8  *telegramLingo)
{
    *telegramLen    =   iPodHndl->pickupInfo.pickup_len;
    *telegramCmdId  =   iPodHndl->pickupInfo.cmdId;
    *telegramLingo  =   iPodHndl->pickupInfo.lingo;
}


/*!
 * \fn iPodWaitForResponse
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_OK                Normal completion
 * \li \c \b IPOD_ERROR             OS error occurred
 * \li \c \b IPOD_NOT_CONNECTED     iPod is not connected
 * \li \c \b IPOD_ERR_TMOUT         Polling failed or timeout
 * \par DESCRIPTION
 * This function waits for the response.
 */
LOCAL S32 iPodWaitForResponse(IPOD_INSTANCE* iPodHndl)
{
    S32  err             =   IPOD_OK;
    S32  rc              =   IPOD_OK;
    U32  flg             =   0;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    if (iPodHndl->detected == FALSE)
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }
    else
    {
        err = iPodOSSetFlag(iPodHndl->flgReaderHandShakeID, IPOD_READ_HANDSHAKE_READ, IPOD_FLG_TWO_WAY);
        if (err == IPOD_OK)
        {
            err = iPodOSWaitFlag( iPodHndl->flgReaderHandShakeID, 
                                  IPOD_READ_HANDSHAKE_CB, 
                                  0,
                                  &flg, 
                                  (S32)(dcInfo[IPOD_DC_WAIT_TMO].para.val * dcInfo[IPOD_DC_READ_RETRIES].para.val), IPOD_FLG_TWO_WAY);
        }

        if (IPOD_OK != err)
        {
            iPodSetExpectedCmdIdEx(iPodHndl, (U16)IPOD_LINGO_DEFAULT, (U8)IPOD_ID_DEFAULT, 0);

            (void) iPodOSWaitFlag( iPodHndl->flgReaderHandShakeID, 
                                   IPOD_READ_HANDSHAKE_CB, 
                                   0,
                                   &flg, 
                                   IPOD_WAIT_READ_RETRY_TIME, IPOD_FLG_TWO_WAY);
            
            (void) iPodOSWaitFlag( iPodHndl->flgReaderHandShakeID, 
                                   IPOD_READ_HANDSHAKE_READ,
                                   0,
                                   &flg, 
                                   IPOD_WAIT_FAILED_READ_TIME, IPOD_FLG_TWO_WAY);
        }

        /* convert OS error code to iPod error code */
        if (IPOD_OK != err)
        {
            rc = IPOD_ERROR;
        }
    }
    if (rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

S32 iPodWaitForModeSwitchSemaphore(IPOD_INSTANCE* iPodHndl)
{
    S32 err = iPodOSWaitSemaphore(iPodHndl->semModeSwitchId, IPOD_MODESWITCH_TIME_OUT);

    /* convert t-kernel error code to ipod error code */
    if (err != IPOD_OK)
    {
        err = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodOSWaitSemaphore() returns err = %d",err);
    }

    return err;

}

S32 iPodWaitForModeSwitch(IPOD_INSTANCE* iPodHndl)
{
    /* only test whether semaphore is available, do not wait here */
    return (S32)iPodOSWaitSemaphore(iPodHndl->semModeSwitchId, 0);
}


S32 iPodWaitForSenderSemaphore(void)
{
    S32 err = iPodOSWaitSemaphore(g_semSenderId, IPOD_MODESWITCH_TIME_OUT);

    /* convert t-kernel error code to ipod error code */
    if (err != IPOD_OK)
    {
        err = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodOSWaitSemaphore() returns err = %d",err);
    }

    return err;

}

S32 iPodSigModeSwitchSemaphore(IPOD_INSTANCE* iPodHndl)
{
    return iPodOSSignalSemaphore(iPodHndl->semModeSwitchId);
}

S32 iPodSigSenderSemaphore(void)
{
    return iPodOSSignalSemaphore(g_semSenderId);
}



/*!
 * \fn iPodWaitAndGetResponseFixedSize
 * \par INPUT PARAMETERS
 *  none
 * \par OUTPUT PARAMETERS
 * \li \c \b  U8 *data
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_ERR_OK      Normal completion
 * \li \c \b IPOD_ERROR       Function failed
 * \par DESCRIPTION
 * This function waits and get the response fixed size.
 */
S32 iPodWaitAndGetResponseFixedSize(IPOD_INSTANCE* iPodHndl, U8 *data)
{
    S32 rc = IPOD_ERROR;
    S32 ercd = IPOD_OK;
    IPOD_PICKUPMSGINFO* pickupInfo = &iPodHndl->pickupInfo;

    if (FALSE != iPodHndl->detected)
    {
        rc = iPodWaitForResponse(iPodHndl);
    }
    else
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }
    if (rc == IPOD_OK)
    {
        /* avoid overlap with Send operation because it accesses to memory that was freed here */
        ercd = iPodWaitForSenderSemaphore();
        if (ercd == IPOD_OK)
        {
            if (pickupInfo->error == IPOD_OK)
            {
                if (pickupInfo->pickup_len < iPodHndl->iAP1MaxPayloadSize)
                {
                    if ((data != NULL)
                     && (pickupInfo->pickup_message != NULL)
                     && (pickupInfo->pickup_len > 0))
                    {
                        memcpy(data,
                               pickupInfo->pickup_message,
                               pickupInfo->pickup_len);
                    }
                }
                else
                {
                    if ((data != NULL)
                     && (pickupInfo->pickup_message != NULL))
                    {
                        memcpy(data,
                               pickupInfo->pickup_message,
                               iPodHndl->iAP1MaxPayloadSize);
                    }
                }

                if (pickupInfo->pickup_message != NULL)
                {
                    free(pickupInfo->pickup_message);
                    pickupInfo->pickup_message = NULL;
                }
                rc = (S32)pickupInfo->error;
            }
            else
            {
                rc = iPod_get_error_code((S32)pickupInfo->error);
                IAP1_TRANSPORT_LOG(DLT_LOG_WARN, "Error 0x%02x (%s)",iPodHndl->pickupInfo.error,iPod_get_error_msg((S32)iPodHndl->pickupInfo.error));
                pickupInfo->pickup_len = 0;
                if (pickupInfo->pickup_message != NULL)
                {
                    free(pickupInfo->pickup_message);
                    pickupInfo->pickup_message = NULL;
                }
            }

            ercd = iPodSigSenderSemaphore();
        }
    }

    return rc;
}


/*!
 * \fn iPodWaitAndGetValuesOfLenCmdInfo
 * \par INPUT PARAMETERS
 *  none
 * \par OUTPUT PARAMETERS
 * \li \c \b  U16  *telegramLen
 * \li \c \b  U16  *telegramCmdId
 * \li \c \b  U8   *telegramLingo
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b IPOD_ERR_OK      Normal completion
 * \li \c \b IPOD_ERROR       Function failed
 * \par DESCRIPTION
 * This function waits and gets the command infos.
 */
S32 iPodWaitAndGetValuesOfLenCmdInfo(IPOD_INSTANCE* iPodHndl,
                                     U16  *telegramLen,
                                     U16  *telegramCmdId,
                                     U8   *telegramLingo)
{
    S32 rc = IPOD_OK;
    S32 ercd = IPOD_OK;

    if (FALSE != iPodHndl->detected)
    {
        rc = iPodWaitForResponse(iPodHndl);
    }
    else
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }
    if (rc == IPOD_OK)
    {
        /* avoid overlap with Send operation because it accesses to memory that was freed here */
        ercd = iPodWaitForSenderSemaphore();
        if (ercd == IPOD_OK)
        {
            *telegramLen    = iPodHndl->pickupInfo.pickup_len;
            *telegramCmdId  = iPodHndl->pickupInfo.cmdId;
            *telegramLingo  = iPodHndl->pickupInfo.lingo;

            ercd = iPodSigSenderSemaphore();
        }
    }
    else
    {
        *telegramLen    = 0;
        *telegramCmdId  = 0;
        *telegramLingo  = 0;
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    }

    return rc;
}


/*!
 * \fn iPodWaitAndGetResponseLength
 * \par INPUT PARAMETERS
 *  none
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b Response length or
 * \li \c \b IPOD_ERR_OK      Normal completion
 * \li \c \b IPOD_ERROR       Function failed
 * \par DESCRIPTION
 * This function waits and gets the command infos.
 */
S32 iPodWaitAndGetResponseLength(IPOD_INSTANCE* iPodHndl)
{
    S32 rc = IPOD_OK;
    S32 ercd = IPOD_OK;

    if (FALSE != iPodHndl->detected)
    {
        rc = iPodWaitForResponse(iPodHndl);
    }
    else
    {
        rc = IPOD_NOT_CONNECTED;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
    }
    if (rc == IPOD_OK)
    {
        /* avoid overlap with Send operation because it accesses to memory that was freed here */
        ercd = iPodWaitForSenderSemaphore();
        if (ercd == IPOD_OK)
        {
            if (iPodHndl->pickupInfo.error == IPOD_OK)
            {
                rc = (S32)iPodHndl->pickupInfo.pickup_len;
            }
            else
            {
                rc = iPod_get_error_code((S32)iPodHndl->pickupInfo.error);
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error 0x%02x (%s)",iPodHndl->pickupInfo.error,iPod_get_error_msg((S32)iPodHndl->pickupInfo.error));
            }

            ercd = iPodSigSenderSemaphore();
        }
    }
    else
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "rc = %d",rc);
    }

    return rc;
}


/*!
 * \fn iPodGetResponseData
 * \par INPUT PARAMETERS
 * \par OUTPUT PARAMETERS
 * \li \c \b U8* data
 * \par REPLY PARAMETERS
 * \par DESCRIPTION
 * This function gets the response data.
 */
void iPodGetResponseData(IPOD_INSTANCE* iPodHndl, U8* data)
{
    S32 ercd = IPOD_OK;
    if ((iPodHndl != NULL) && (data != NULL))
    {
        /* avoid overlap with Send operation because it accesses to memory that was freed here */
        ercd = iPodWaitForSenderSemaphore();
        if (ercd == IPOD_OK)
        {
            if (FALSE != iPodHndl->detected)
            {
                memcpy(data, iPodHndl->pickupInfo.pickup_message, iPodHndl->pickupInfo.pickup_len);
            }
            ercd = iPodSigSenderSemaphore();
        }
    }
}


/*!
 * \fn iPodCreateReaderTask
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 ReturnCode
 * \li \c \b Task ID     Completed successfully
 * \li \c \b IPOD_ERROR  Processing failed
 * \par DESCRIPTION
 * This function creates the reader task.
 */
S32 iPodCreateReaderTask(IPOD_INSTANCE* iPod)
{
    S32 rc = IPOD_OK;
    IPOD_TASK_ID tskId = 0;
    const IPOD_Cfg *dcInfo  = (const IPOD_Cfg *)iPodGetDevInfo();
   
    rc = create_all_semaphores(iPod);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "create_all_semaphores() returns rc = %d",rc);
    if (rc == IPOD_OK)
    {
        rc = create_eventflag(iPod);
        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "create_eventflag() returns rc = %d",rc);
    }

    iPod->iPodRetAccInfoData = calloc(iPod->iAP1MaxPayloadSize, sizeof(U8) );
    if(iPod->iPodRetAccInfoData == NULL)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error in allocating memory for iPodRetAccInfoData");
        rc = IPOD_ERROR;        /* Error */
    }
    else
    {
        tskId = iPodOSCreateTask(iPodReaderTask, dcInfo[IPOD_DC_AUDIOREADER].para.p_val,
                                 dcInfo[IPOD_DC_READER_PRIO].para.val,
                                 (dcInfo[IPOD_DC_READER_STKSZ].para.val+iPod->iAP1MaxPayloadSize),
                                 dcInfo[IPOD_DC_READER_LCID].para.val, iPod);

        if(iPodOSVerifyTaskID(tskId) == IPOD_OK)
        {
            iPod->readerTaskId = tskId;
        }
        else
        {
            iPodOSClearTaskID(&(iPod->readerTaskId));
            rc = IPOD_ERROR;        /* Error */
        }
    }
    if (rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

S32 iPodReadSerialData( IPOD_TRANSPORT *transport, S32 devInfo, U32 readLen, U8* buf, S32 flags )
{
    S32 rc = IPOD_OK;
    IPOD_DATACOM_FUNC_TABLE *data_com_function = NULL;
    
    if((transport == NULL) || (buf == NULL ))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter transport = %p buf = %p",transport,buf);
        return IPOD_BAD_PARAMETER;
    }
    
    data_com_function = &transport->data_com_functions;
    
    if(data_com_function->read == NULL)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter data_com_function->read = %p",data_com_function->read);
        return IPOD_BAD_PARAMETER;
    }
    
    /* Read data until readLen */
    rc = data_com_function->read( devInfo, readLen, buf, flags );
    if(rc >= 0)
    {
        /* Read data length is expected length */
        if((U32)rc == readLen)
        {
            rc = IPOD_OK;
        }
        else
        {
            /* Something data is lost */
            rc = IPOD_ERR_DATA_LOST;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Data Last");
        }
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->read() returns rc = %d",rc);
    }
    
    return rc;
}


S32 iPodReceiveDataFromUSB(IPOD_INSTANCE *iPodHndl, U32 bufSize, U8 *buf)
{
    S32 rc = IPOD_OK;
    U8                hasMorePacket = FALSE;
    S32               linkByte      = 0;
    IPOD_TRANSPORT* transport = NULL;
    U32 recvSize = 0;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = NULL;
    
    if((iPodHndl == NULL) || (buf == NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodHndl = %p buf = %p",iPodHndl,buf);
        return IPOD_BAD_PARAMETER;
    }
    
    if(iPodHndl->packet == NULL)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPodHndl->packet is NULL");
        return IPOD_BAD_PARAMETER;
    }
    
    transport = &iPodHndl->transport;
    data_com_function = &transport->data_com_functions;
    
    if(data_com_function->read == NULL)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->read is NULL");
        return IPOD_BAD_PARAMETER;
    }
    
    /* Fixed for JIRA[SWGII-3906] */
    /* msgBuffer has been deleted  */
    memset(buf, 0, bufSize*sizeof(U8));
    
    do
    {
        /* This receive expects to read the one packet */
        rc = data_com_function->read(transport->devInfo, iPodHndl->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5 + IPOD_USB_LINK_CTRL_BYTE_SIZE, iPodHndl->packet, 0);
        if(rc > 0)
        {
            /* Set link byte */
            linkByte = iPodHndl->packet[IPOD_POS0];
            /* Remove linkbyte length */
            rc--;
            
            if((recvSize + rc) <= bufSize)
            {
                memcpy(&buf[recvSize], &iPodHndl->packet[IPOD_POS1], rc);
                recvSize += rc;
                rc = IPOD_OK;
            }
            else
            {
                memcpy(&buf[recvSize], &iPodHndl->packet[IPOD_POS1], bufSize - recvSize);
                recvSize = bufSize;
                rc = IPOD_OK;
            }
            
            /* Link Control Byte - only one HID report */
            if (linkByte== IPOD_LINK_CTRL_ONLY_ONE)
            {
                hasMorePacket = FALSE;
            }
            /* Link Control Byte - first HID report, more to follow */
            else if (linkByte == IPOD_LINK_CTRL_FRST_FOLLOW)
            {
                hasMorePacket = TRUE;
            }
            /* Link Control Byte - middle HID report, more to follow */
            else if( linkByte == IPOD_LINK_CTRL_MIDDLE)
            {
                hasMorePacket = TRUE;
            }
            /* Link Control Byte - last HID report, no more HID report */
            else if(linkByte == IPOD_LINK_CTRL_LAST)
            {
                hasMorePacket = FALSE;
            }
            /* Link Control Byte - unknown HID report */
            else if(linkByte < 0)
            {
                rc = linkByte;
            }
            else
            {
                rc = IPOD_DATACOM_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Datacom Error linkByte = %d",linkByte);
            }
        }
        
    } while ((rc >= IPOD_DATACOM_SUCCESS) && (hasMorePacket != FALSE));

    return rc;
    
}



S32 iPodReceiveDataFromSerial(IPOD_INSTANCE *iPodHndl, U32 bufSize, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_TRANSPORT* transport = NULL;
    U16 readLen = 0;
    U32 dataPos = 0;
    
    if((iPodHndl == NULL) || (buf == NULL))
    {
        /* NULL pointer parameter */
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter - iPodHndl = %p buf = %p",iPodHndl,buf);
        return IPOD_BAD_PARAMETER;
    }
    
    transport = &iPodHndl->transport;
    
    /* Read Start Byte */
    rc = iPodReadSerialData( transport, transport->devInfo, IPOD_READ_SIZE_1, &buf[IPOD_POS0], 0 );
    if( rc == IPOD_OK )
    {
        /* Check whether first data is 0x55 */
        if(buf[IPOD_POS0] == IPOD_START_OF_PACKET)
        {
            /* Read length */
            rc = iPodReadSerialData( transport, transport->devInfo, IPOD_READ_SIZE_1, &buf[IPOD_POS1], 0 );
            if( rc == IPOD_OK ){
                /* length byte is needed more 2bytes */
                if(buf[IPOD_POS1] == 0x00)
                {
                    /* Read long data length */
                    rc = iPodReadSerialData( transport, transport->devInfo, IPOD_READ_SIZE_2, &buf[IPOD_POS2], 0 );
                    if( rc == IPOD_OK )
                    {
                        readLen = iPod_convert_to_little16(&buf[IPOD_POS2]);
                        dataPos = IPOD_POS4;
                    }
                }
                else
                {
                    /* Length is 1 byte length */
                    readLen = (U16)buf[IPOD_POS1];
                    dataPos = IPOD_POS2;
                }
                
                /* Add the check sum byte */
                readLen ++;
            }
        }
        else
        {
            rc = IPOD_ERR_DATA_LOST;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Data Lost");
        }
    }
    
    /* Read Data */
    if(rc == IPOD_OK)
    {
        /* Length check */
        if((readLen + dataPos <= bufSize) && (readLen <= iPodHndl->iAP1MaxPayloadSize))
        {
            rc = iPodReadSerialData( transport, transport->devInfo, readLen, &buf[dataPos], 0 );
        }
        else
        {
            rc = IPOD_ERR_DATA_LOST;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Data Lost");
        }
    }
    
    return rc;
}

    
/* ========================================================================== */
/*  Reader Task                                                            */
/* ========================================================================== */
LOCAL void iPodReaderTask(void* exinf)
{
    S32 ercd                = IPOD_OK;
    U32 flg                 = 1;
    S32 detectedDevice      = 0;
    IPOD_INSTANCE* iPodHndl = (IPOD_INSTANCE*)exinf;
    IPOD_PICKUPMSGINFO *pickupInfo = &iPodHndl->pickupInfo;
    IPOD_RCVMSGINFO    *rcvMsgInfo = &iPodHndl->rcvMsgInfo;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    struct sigaction ignore  = {.sa_handler = SIG_IGN, .sa_flags = SA_RESTART};

    /* Ignore "broken pipe" signals. */
    ercd = sigaction(SIGPIPE, &ignore, NULL);

    MessageHeaderInfoType MessageHeaderInfo;
    
    /* Initialize the structure */
    memset(&MessageHeaderInfo, 0, sizeof(MessageHeaderInfo));
    
    iPodHndl->detected = FALSE;
    MessageHeaderInfo.iPodResponseBuffer = NULL;

    if (IPOD_ERROR != ercd)
    {
        MessageHeaderInfo.msgBuffer = (void*) calloc(iPodHndl->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5, 1);
    }

    if(MessageHeaderInfo.msgBuffer == NULL)
    {
        iPodWorkerExeCB(iPodHndl, IPOD_ERR_NOMEM);
        detectedDevice = IPOD_ERR_NOMEM;
    }

    if(detectedDevice == IPOD_OK)
    {
        iPodWorkerExeCB(iPodHndl, IPOD_DETECTED);

        iPodHndl->detected = TRUE; /* refer in WorkerTask */
        iPodWorkerIdentify(iPodHndl,FALSE);
    }

    while (iPodHndl->detected == TRUE)
    {
        if((iPodHndl->connection == IPOD_USB_HOST_CONNECT) ||
            (iPodHndl->connection == IPOD_USB_FUNC_CONNECT))
        {
            ercd = iPodReceiveDataFromUSB(iPodHndl, iPodHndl->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5, MessageHeaderInfo.msgBuffer);
        }
        else
        {
            ercd = iPodReceiveDataFromSerial(iPodHndl, iPodHndl->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5, MessageHeaderInfo.msgBuffer);
        }
        
        if( ercd == IPOD_OK )
        {
            MessageHeaderInfo.telegramType = IPOD_MESSAGE;

            iPodGetTelegramHeaderInfo(&MessageHeaderInfo, iPodHndl);

            if (MessageHeaderInfo.telegramLingo == IPOD_GENERAL_LINGO)
            {
                iPodProcessGeneralLingoCommand(iPodHndl, MessageHeaderInfo);
            }
            if(MessageHeaderInfo.telegramLingo == IPOD_DISPLAY_REMOTE_LINGO)
            {
                iPodProcessDisplayRemoteLingoCommand(iPodHndl, MessageHeaderInfo);
            }
            if (MessageHeaderInfo.telegramLingo == IPOD_EXTENDED_LINGO)
            {
                iPodProcessExtendedLingoCommand(iPodHndl, MessageHeaderInfo);
            }

            if (MessageHeaderInfo.telegramLingo == IPOD_AUDIO_LINGO)
            {
                iPodProcessAudioLingoCommand(iPodHndl, MessageHeaderInfo);
            }

            if(MessageHeaderInfo.telegramLingo == IPOD_STORAGE_LINGO)
            {
                iPodProcessStorageLingoCommand(iPodHndl, MessageHeaderInfo);
            }

            if(MessageHeaderInfo.telegramLingo == IPOD_LOCATION_LINGO)
            {
                iPodProcessLocationLingoCommand(iPodHndl, MessageHeaderInfo);
            }

            /* JIRA[SWGII-4886] */
            /* iPodResponseBuffer will be NULL if length is 0, this is okay. If length is not 0, the Buffer must not be NULL either */
            if ((MessageHeaderInfo.iPodResponseBuffer != NULL) || (MessageHeaderInfo.telegramLen == 0))
            {
                if ((iPodHndl->detected == TRUE)
                     && (rcvMsgInfo->waitingCmdCnt > 0)
                     && (((MessageHeaderInfo.telegramLingo == rcvMsgInfo->expectedLingo)
                          && (MessageHeaderInfo.telegramCmdId == rcvMsgInfo->expectedCmdId))
                         || ((MessageHeaderInfo.telegramLingo == rcvMsgInfo->expectedLingo)
                             && (MessageHeaderInfo.ackCmdId == rcvMsgInfo->sendCmdId))))
                {
                    ercd = iPodOSWaitFlag( iPodHndl->flgReaderHandShakeID,
                                           IPOD_READ_HANDSHAKE_READ,
                                           0,
                                           &flg,
                                           (S32)dcInfo[IPOD_DC_WAIT_TMO].para.val, IPOD_FLG_TWO_WAY );
                    if (ercd == IPOD_OK)
                    {
                        pickupInfo->cmdId = MessageHeaderInfo.telegramCmdId;
                        pickupInfo->lingo = MessageHeaderInfo.telegramLingo;
                        pickupInfo->error = MessageHeaderInfo.telegramErrorCode;

                        if (pickupInfo->pickup_message != NULL)
                        {
                            free(pickupInfo->pickup_message);
                            pickupInfo->pickup_message = NULL;
                        }
    
                        if ((MessageHeaderInfo.telegramErrorCode == IPOD_OK)
                            && ((MessageHeaderInfo.telegramLen > 0) && (MessageHeaderInfo.telegramLen <= iPodHndl->iAP1MaxPayloadSize))
                            && ((MessageHeaderInfo.transID == rcvMsgInfo->expectedTransID)
                                || (iPodHndl->rcvMsgInfo.startIDPS == FALSE)))
                        {
                            if (MessageHeaderInfo.telegramLen > 0)
                            {
                                pickupInfo->pickup_message = calloc(MessageHeaderInfo.telegramLen, sizeof(U8));
                            }
                        
                            if ((pickupInfo->pickup_message!=NULL) && (MessageHeaderInfo.iPodResponseBuffer!=NULL))
                            {
                                memcpy(pickupInfo->pickup_message,
                                       MessageHeaderInfo.iPodResponseBuffer,
                                       MessageHeaderInfo.telegramLen);

                                pickupInfo->pickup_len = MessageHeaderInfo.telegramLen;
                            }
                        }
                        else
                        {
                            if (MessageHeaderInfo.telegramLen == 0)
                            {
                                pickupInfo->pickup_len = 0;
                            }
                            if ((MessageHeaderInfo.transID != rcvMsgInfo->expectedTransID)
                                && (iPodHndl->rcvMsgInfo.startIDPS == TRUE))
                            {
                                pickupInfo->error = IPOD_ACKERR_UNKNOWN_ID;
                            }
                        }
                        
                        /* JIRA[SWGII-4800] */
                        /* iPod ctrl must reply retaccessoryInfo. */
                        /* when iPod is changing the mode, ipod ctrl reply retaccessoryInfo after finish the mode changing.*/
                        if ((MessageHeaderInfo.telegramLingo == (U8)IPOD_LINGO_GENERAL) &&
                            (MessageHeaderInfo.telegramCmdId == (U16)IPOD_GENERAL_LINGO_ACK) &&
                            (MessageHeaderInfo.iPodResponseBuffer[0] == (U8)IPOD_OK) &&
                            ( (MessageHeaderInfo.ackCmdId == IPOD_GENERAL_LINGO_EnterRemoteUIMode) ||
                              (MessageHeaderInfo.ackCmdId == IPOD_GENERAL_LINGO_ExitRemoteUIMode)))
                        {
                            if(iPodHndl->iPodRetAccInfoFlg == 1)
                            {
                                iPodRetAccessoryInfo(iPodHndl, iPodHndl->iPodRetAccInfoData);

                                memset(iPodHndl->iPodRetAccInfoData, 0, iPodHndl->iAP1MaxPayloadSize);
                                iPodHndl->iPodRetAccInfoFlg++;
                            }
                        }

                        if(rcvMsgInfo->waitingCmdCnt > 0)
                        {
                            rcvMsgInfo->waitingCmdCnt--;
                        }
                        if(rcvMsgInfo->waitingCmdCnt == 0)
                        {
                            rcvMsgInfo->expectedLingo = IPOD_LINGO_DEFAULT;
                            rcvMsgInfo->expectedCmdId = IPOD_ID_DEFAULT;
                            rcvMsgInfo->sendCmdId      = IPOD_ID_DEFAULT;
                            MessageHeaderInfo.ackCmdId = IPOD_ACKID_DEFAULT;
                        }

                        /* if asked to wait, next command should be an ack */
                        if ((MessageHeaderInfo.telegramLingo == (U8)IPOD_LINGO_GENERAL) &&
                            (MessageHeaderInfo.telegramCmdId == (U16)IPOD_GENERAL_LINGO_ACK) &&
                            (MessageHeaderInfo.iPodResponseBuffer[0] == (U8)IPOD_ACK_WAIT_FOR_ACK))
                        {
                            iPodSetExpectedCmdId(iPodHndl, (U16)IPOD_GENERAL_LINGO_ACK, (U8)IPOD_LINGO_GENERAL);
                            rcvMsgInfo->expectedTransID--;
                        }

                        ercd = iPodOSSetFlag(iPodHndl->flgReaderHandShakeID, IPOD_READ_HANDSHAKE_CB, IPOD_FLG_TWO_WAY);

                        /* JIRA[SWGII-4800] */
                        /* syncronized with Mode change commane. */
                        if(iPodHndl->iPodRetAccInfoFlg == 2)
                        {
                            iPodWaitForModeSwitchSemaphore(iPodHndl);
                            iPodSigModeSwitchSemaphore(iPodHndl);
                            iPodHndl->iPodRetAccInfoFlg = 0;
                        }
                    }
                    else
                    {
                        IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Can't read USB. error = %d",ercd);
                    }
                }
                else
                {
                    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Lost Message in Reader-Task Lingo ID= 0x%04x Command ID  = 0x%04x",MessageHeaderInfo.telegramLingo,MessageHeaderInfo.telegramCmdId);
                }
            }
        }
        else /* Error case */
        {
            if(ercd == IPOD_ERR_DATA_LOST)
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Data Lost");
                /* Some data are lost. Received data is ignored. */
                MessageHeaderInfo.telegramType = IPOD_MESSAGE_DEFECT;
            }
            else
            {
                MessageHeaderInfo.telegramType = IPOD_RCV_ERROR;
            }
            
            MessageHeaderInfo.telegramLen        =   0;
            MessageHeaderInfo.telegramCmdId      =   IPOD_DEFAULT_CMD_ID;
            MessageHeaderInfo.telegramLingo      =   IPOD_DEFAULT_LINGO;
            MessageHeaderInfo.telegramErrorCode  =   IPOD_DEFAULT_ERROR;
        }
        
        if(MessageHeaderInfo.telegramType != IPOD_MESSAGE)
        {
            IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Telegram Cmd ID = 0x%04x",MessageHeaderInfo.telegramCmdId);
            if (MessageHeaderInfo.telegramType == IPOD_RCV_ERROR)
            {
                iPodHndl->isAPIReady = FALSE;
                iPodHndl->detected = FALSE;
                iPodWorkerStopAuthentication(iPodHndl);
                ercd = iPodWaitForSenderSemaphore(); /* avoid overlap with Send operation because it accesses to memory that is freed here */
                if (ercd == IPOD_OK)
                {
                    iPodHndl->iPodRetAccInfoFlg = 0;
                    memset(iPodHndl->iPodRetAccInfoData, 0, iPodHndl->iAP1MaxPayloadSize);

                    (void)iPodTrnspClosePlugin(iPodHndl);
                    iPodFreeArtworkData(iPodHndl);
                    iPodSetStartIDPS(iPodHndl, FALSE);
                    if (rcvMsgInfo->waitingCmdCnt != 0)
                    {
                        pickupInfo->error = IPOD_ACKERR_IPOD_NOT_CONNECTED;

                        rcvMsgInfo->expectedLingo = IPOD_LINGO_DEFAULT;
                        rcvMsgInfo->expectedCmdId = IPOD_ID_DEFAULT;
                        rcvMsgInfo->sendCmdId      = IPOD_ID_DEFAULT;
                        rcvMsgInfo->waitingCmdCnt = 0;
                        iPodOSSetFlag(iPodHndl->flgReaderHandShakeID, IPOD_READ_HANDSHAKE_CB, IPOD_FLG_TWO_WAY);
                    }

                    iPodExecuteCBUSBDetach(iPodHndl);

                    /* close and reopen usb device to stop possible waiting driver accesses */
                    ercd = iPodSigSenderSemaphore(); /* avoid overlap with Send operation because it accesses to memory that was freed here */
                    IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPodSigSenderSemaphore() returns : ercd = %d", ercd);
                }
            }
        }
    }

    ercd = iPodWaitForSenderSemaphore();
    IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "iPodWaitForSenderSemaphore() returns : ercd = %d", ercd);
    if (ercd == IPOD_OK)
    {
        if(MessageHeaderInfo.msgBuffer != NULL)
        {
            free(MessageHeaderInfo.msgBuffer);
            MessageHeaderInfo.msgBuffer = NULL;
        }
        
        if(MessageHeaderInfo.iPodResponseBuffer != NULL)
        {
            free(MessageHeaderInfo.iPodResponseBuffer);
            MessageHeaderInfo.iPodResponseBuffer = NULL;
        }

        delete_all_buffers(iPodHndl);
        delete_eventflag(iPodHndl);
        delete_all_semaphores(iPodHndl);

        ercd = iPodSigSenderSemaphore();
    }
    iPodWorkerResume();

    iPodHndl->id = 0;

    iPodOSExitTask(iPodHndl->readerTaskId);     /* delete task */
}

/****************************************************************************
 * internal functions                                                       *
 ****************************************************************************/
LOCAL void delete_all_buffers(IPOD_INSTANCE* iPodHndl)
{
    IPOD_PICKUPMSGINFO *pickupInfo = &iPodHndl->pickupInfo;

    if (pickupInfo->pickup_message != NULL)
    {
        free(pickupInfo->pickup_message);
        pickupInfo->pickup_message = NULL;
    }
    pickupInfo->pickup_len = 0;
    pickupInfo->cmdId = IPOD_ID_DEFAULT;
    pickupInfo->lingo = IPOD_LINGO_DEFAULT;
    pickupInfo->error = IPOD_ACKERR_IPOD_NOT_CONNECTED;
    pickupInfo->transID = 0;
}

LOCAL S32 create_all_semaphores(IPOD_INSTANCE* iPodHndl)
{
    S32 rc = IPOD_OK;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    if (iPodHndl->semModeSwitchId == 0)
    {
        rc = iPodOSCreateSemaphore(dcInfo[IPOD_DC_SEM_MODESWITCH].para.p_val, &iPodHndl->semModeSwitchId);
    }

    return rc;
}

LOCAL S32 delete_all_semaphores(IPOD_INSTANCE* iPodHndl)
{
    S32 rc = IPOD_OK;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();

    rc = iPodOSDeleteSemaphore(iPodHndl->semModeSwitchId, dcInfo[IPOD_DC_SEM_MODESWITCH].para.p_val);
    if(rc == IPOD_OK)
    {
        iPodHndl->semModeSwitchId = 0;
    }

    return rc;
}

LOCAL S32 create_eventflag(IPOD_INSTANCE* iPodHndl)
{
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();
    return iPodOSCreateFlag(dcInfo[IPOD_DC_FLG_READER].para.p_val, &iPodHndl->flgReaderHandShakeID, IPOD_FLG_TWO_WAY);
}

LOCAL S32 delete_eventflag(IPOD_INSTANCE* iPodHndl)
{
    S32 rc = IPOD_OK;
    const IPOD_Cfg *dcInfo = (const IPOD_Cfg *)iPodGetDevInfo();
    
    /* JIRA[SWGII-3259] */
    rc = iPodOSDeleteFlag(iPodHndl->flgReaderHandShakeID, dcInfo[IPOD_DC_FLG_READER].para.p_val, IPOD_FLG_TWO_WAY);
    if (rc == IPOD_OK)
    {
        memset(&iPodHndl->flgReaderHandShakeID, -1, sizeof(iPodHndl->flgReaderHandShakeID));
    }

    return rc;
}

LOCAL U8 iPodCalcChecksum(const U8* msg)
{
    U32 i = 0;
    U8 checksum = 0x00;
    U32 sum = msg[IPOD_POS1];


    for (i = 0; i < msg[IPOD_POS1]; i++)
    {
        sum += msg[IPOD_START_OF_MSG_POS + i];
    }

    checksum = (U8)((CHECKSUM_BASE - sum) & IPOD_CHKSUM_MASK);

    return checksum;
}


LOCAL U8 iPodCalcChecksumLongTelegram(const U8* msg)
{
    U16 i           = 0;
    U8  checksum    = 0x00;
    U32 sum         = msg[IPOD_POS2];
    U16 msgLen      = msg[IPOD_POS2];

    msgLen  =   (U16)(msgLen << IPOD_SHIFT_8);
    msgLen |=   (U16)msg[IPOD_POS3];
    sum    +=   (U32)msg[IPOD_POS3];

    for (i = 0; i < msgLen; i++)
    {
        sum += msg[IPOD_POS4 + i];
    }

    checksum = (U8)((IPOD_CHKSUM_BASE - sum) & IPOD_CHKSUM_MASK);

    return checksum;
}

LOCAL void iPodAddTransID(IPOD_INSTANCE* iPodHndl, U8 *buf, IPOD_SENDMSGINFO* sendInfo)
{
    U8 transID[2] = {0};
    U16 comLen = 0;

    if((sendInfo->waitFlg != IPOD_NO_WAIT_ACK) && (sendInfo->waitFlg != IPOD_WAIT_ACK_REPLY))
    {
        sendInfo->transID = iPodHndl->rcvMsgInfo.accTransID;
        iPodHndl->rcvMsgInfo.accTransID = sendInfo->transID + 1;
    }
    else
    {
        sendInfo->transID = iPodHndl->rcvMsgInfo.iPodTransID;
    }

    iPod_convert_to_big16(transID, sendInfo->transID);
    if((sendInfo->waitFlg == IPOD_WAIT_ACK_LONG) || (sendInfo->waitFlg == IPOD_NO_WAIT_ACK_LONG))
    {
        buf[IPOD_POS1] = 0;
        sendInfo->checksum = sendInfo->checksum + buf[IPOD_POS2] + buf[IPOD_POS3];
        comLen = iPod_convert_to_little16(&buf[IPOD_POS2]);
        comLen += IPOD_TRANSID_LENGTH;
        iPod_convert_to_big16(&buf[IPOD_POS2], comLen);
        sendInfo->checksum = sendInfo->checksum - (buf[IPOD_POS2] + buf[IPOD_POS3]);
    }
    else
    {
        buf[IPOD_POS1] += IPOD_TRANSID_LENGTH;
        sendInfo->checksum -= IPOD_TRANSID_LENGTH;
    }
    sendInfo->msgLen += IPOD_TRANSID_LENGTH;
    sendInfo->msgLenTotal += IPOD_TRANSID_LENGTH;
    sendInfo->checksum = sendInfo->checksum - (transID[IPOD_POS0] + transID[IPOD_POS1]);

}

S32 iPodSendPacket(IPOD_INSTANCE* iPodHndl, U32 totalLen, const U8* iPod_msg)
{
    S32 res = IPOD_DATACOM_BAD_PARAMETER;
    U32                i                     = 0;
    U32                dataLen               = totalLen;
    U8                *msg                   = NULL;
    U32                hasPartlyFilledReport = TRUE;
    U32                numFullReports        = 0;
    U32                writeLen              = 0;
    IPOD_TRANSPORT* transport = NULL;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = NULL;
    IPOD_DATACOM_PROPERTY usbProperty = {0};
    U32 maxLen = 0;
    
    if((iPodHndl == NULL) || (iPod_msg == NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad parameter iPodHndl = %p iPod_msg = %p",iPodHndl,iPod_msg);
        return IPOD_BAD_PARAMETER;
    }
    
    transport = &iPodHndl->transport;
    data_com_function = &transport->data_com_functions;
    if((data_com_function->write == NULL) || (data_com_function->property == NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->write = %p data_com_function->property = %p",data_com_function->write,data_com_function->property);
        return IPOD_BAD_PARAMETER;
    }

    res = data_com_function->property(transport->devInfo, &usbProperty);
    if (IPOD_DATACOM_SUCCESS == res)
    {
        maxLen = usbProperty.maxSize;
        /* maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE because one
           byte of the report must be used for the Link Control
           Byte */
        if(maxLen > IPOD_USB_LINK_CTRL_BYTE_SIZE)
        {
            numFullReports = ((U16)totalLen / (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE));
            
            if ((totalLen % (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE)) == 0)
            {
                hasPartlyFilledReport = FALSE;
            }
                    
            msg = (U8 *)calloc(maxLen + IPOD_USB_HEADER_SIZE, sizeof(U8));
            if (msg != NULL)
            {
                /* JIRA[SWGII-4227] */
                /* This loop is counted by i and maximum size is numFullReports(U16) */
                /* Therefore, This loop never occur infinite loop. */
                for (i = 0; (((i < numFullReports ) && (hasPartlyFilledReport == FALSE)) || 
                     ((i <= numFullReports) && (hasPartlyFilledReport != FALSE))) &&
                     (res == IPOD_DATACOM_SUCCESS); i++)
                {
                    memset(msg, 0, maxLen + IPOD_USB_HEADER_SIZE);

                    /* check special cases */
                    if ((i == 0) && (numFullReports >= 1))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_FRST_FOLLOW; /* link control byte ??? */
                    }
                    else if ((i >= 1) && (numFullReports > i))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_MIDDLE; /* link control byte ???    */
                    }
                    else if ((i >= 1) && (i == numFullReports))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_LAST;   /* link control byte ???    */
                    }
                    else
                    {
                        msg[IPOD_POS0]  =  IPOD_LINK_CTRL_ONLY_ONE;
                    }
                
                    if ((i == (numFullReports - 1)) && (hasPartlyFilledReport == FALSE))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_LAST;
                    }
                          
                    if((i == 0) && (numFullReports == 1) && (hasPartlyFilledReport == FALSE))
                    {
                        msg[IPOD_POS0] = IPOD_LINK_CTRL_ONLY_ONE;
                    }
                                
                    /* If data length is bigger than maxLen,
                     * it should be set maxLen,
                     * otherwise should be set data length
                     */
                    if(dataLen > (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE))
                    {
                        writeLen = maxLen;
                        dataLen  = dataLen - (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE);
                    }
                    else
                    {
                        writeLen = dataLen + IPOD_USB_LINK_CTRL_BYTE_SIZE;
                    }
                      
                    if(maxLen >= writeLen)
                    {
                        /* TODO: why memcpy instead of pointer to iPod_msg[ xxx ] ? */
                        memcpy(&msg[IPOD_POS1], &iPod_msg[i * (maxLen - IPOD_USB_LINK_CTRL_BYTE_SIZE)], (writeLen - IPOD_USB_LINK_CTRL_BYTE_SIZE));
                        res = data_com_function->write(transport->devInfo, writeLen, msg, 0);
                        if(0 > res)
                        {
                            iPodOSCancelPoint();
                            if(res != IPOD_DATACOM_NOT_CONNECTED)
                            {
                                res = IPOD_DATACOM_ERROR;
                                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Datacom error res = %d",res);
                            }
                        }
                        else if((U32)res == writeLen)
                        {
                            /* write all data completed */
                            res = IPOD_DATACOM_SUCCESS;
                        }
                        else
                        {
                            /* could not write all data */
                            iPodOSCancelPoint();
                            res = IPOD_DATACOM_ERROR;
                            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->write() could not write all data");
                        }
                    }
                    else
                    {
                        res = IPOD_DATACOM_ERR_NOMEM;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory maxLen is less than writeLen");
                    }
                }
                free(msg);
            }
            else
            {
                res = IPOD_DATACOM_ERR_NOMEM;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No Memory - msg is NULL");
            }
        }
        else
        {
            res = IPOD_DATACOM_ERROR;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "maxLen is not greater than USB link Control Byte size");
        }
    }
    else
    {
        res = IPOD_DATACOM_ERR_NOMEM;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->property() fails because of insufficient memory");
    }

    return res;
}

S32 iPodSendSerial(IPOD_INSTANCE* iPodHndl, U32 totalLen, const U8* iPod_msg)
{
    S32 res = IPOD_DATACOM_BAD_PARAMETER;
    IPOD_TRANSPORT* transport = NULL;
    IPOD_DATACOM_FUNC_TABLE* data_com_function = NULL;
    IPOD_DATACOM_PROPERTY usbProperty = {0};
    U32 writeLen = 0;
    U32 dataLen = 0;
    
    if((iPodHndl == NULL) || (iPod_msg == NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p iPod_msg = %p",iPodHndl,iPod_msg);
        return IPOD_BAD_PARAMETER;
    }
    
    transport = &iPodHndl->transport;
    data_com_function = &transport->data_com_functions;
    if((data_com_function->write == NULL) || (data_com_function->property == NULL))
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "data_com_function->write = %p data_com_function->property = %p",data_com_function->write,data_com_function->property);
        return IPOD_BAD_PARAMETER;
    }

    res = data_com_function->property(transport->devInfo, &usbProperty);
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "ata_com_function->property() returns res = %d",res);
    if (IPOD_DATACOM_SUCCESS == res)
    {
        do
        {
            /* Requested length larger than maximam length */
            if(usbProperty.maxSize < totalLen - writeLen)
            {
                dataLen = usbProperty.maxSize;
            }
            else
            {
              dataLen = totalLen - writeLen;
            }
                    
            res = data_com_function->write(transport->devInfo, dataLen, &iPod_msg[writeLen], 0);
            if(res > 0)
            {
              writeLen += res;
              res = IPOD_OK;
            }
            else
            {
                res = IPOD_ERR_WRITE_FAIL;
            }
        } while((res == IPOD_OK) && (writeLen < totalLen));
    }
    
    IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "res = %d",res);

    return res;

}

S32 iPodSendMessage(VP iPod, U8 *buf, VP sendMsg)
{
    IPOD_INSTANCE* iPodHndl = iPod;
    IPOD_SENDMSGINFO *sendInfo = sendMsg;
    S32  rc                 = IPOD_OK;
    S32  err                = IPOD_OK;
    U8* iPod_msg            = NULL;
    U8 transLen             = 0;

    err = iPodWaitForSenderSemaphore();
    if (IPOD_OK != err)
    {
        err = IPOD_ERR_TMOUT;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - Timeout");
    }
    if ((iPodHndl != NULL)&&(sendInfo != NULL)&&(buf != NULL)&&(IPOD_OK == err))
    {
        if(iPodHndl->detected == FALSE)
        {
            rc = IPOD_NOT_CONNECTED;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod Not Connected");
        }
        else
        {
            if (0 != iPodHndl->rcvMsgInfo.flowControlWait)
            {
                iPodOSSleep(iPodHndl->rcvMsgInfo.flowControlWait);
                iPodHndl->rcvMsgInfo.flowControlWait = 0;
            }
            if (iPodHndl->rcvMsgInfo.startIDPS != FALSE)
            {
                iPodAddTransID(iPodHndl, (U8 *)buf, sendInfo);
            }
            /* The following block is intended to set the command ID to wait for in case of ACK,
               as well as determine the position to insert the transaction ID (transLen) */
            if((sendInfo->waitFlg == IPOD_WAIT_ACK_SHORT) || (sendInfo->waitFlg == IPOD_WAIT_ACK_REPLY))
            {
                /* check if telegram is extended interface mode telegram */
                if (buf[IPOD_POS2] == IPOD_EXTENDED_LINGO)
                {
                    iPodHndl->rcvMsgInfo.sendCmdId = iPod_convert_to_little16((U8 *)&buf[IPOD_POS3]);
                    transLen = IPOD_TRANSLEN_WITH_EXTEND_ACK_SHORT;
                }
                else
                {
                    iPodHndl->rcvMsgInfo.sendCmdId = buf[IPOD_POS3];
                    transLen = IPOD_TRANSLEN_WITH_ACK_SHORT;
                }
            }
            else if(sendInfo->waitFlg == IPOD_WAIT_ACK_LONG)
            {
                if(buf[IPOD_POS4] == IPOD_EXTENDED_LINGO)
                {
                    iPodHndl->rcvMsgInfo.sendCmdId = iPod_convert_to_little16((U8 *)&buf[IPOD_POS5]);
                    transLen = IPOD_TRANSLEN_WITH_ACK_LONG;
                }
                else
                {
                    iPodHndl->rcvMsgInfo.sendCmdId = buf[IPOD_POS5];
                    transLen = IPOD_TRANSLEN_WITH_ACK_LONG - 1;
                }
            }
            else if(sendInfo->waitFlg == IPOD_NO_WAIT_ACK)
            {
                transLen = IPOD_TRANSLEN_WITH_NO_ACK_SHORT;
            }
            else if(sendInfo->waitFlg == IPOD_NO_WAIT_ACK_LONG)
            {
                transLen = IPOD_TRANSLEN_WITH_NO_ACK_LONG;
            }
            else
            {
            }

            /* add reportID to start of message needs two more bytes */
            iPod_msg = (U8*) calloc(sendInfo->msgLenTotal, sizeof(U8));
            if(iPod_msg != NULL)
            {
                if(iPodHndl->rcvMsgInfo.startIDPS != FALSE)
                {
                    if(sendInfo->msgLenTotal >= transLen)
                    {
                        memcpy(iPod_msg, buf, transLen);
                    }
                    else
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "sendInfo->msgLenTotal is less than transLen");
                    }

                    if(((sendInfo->msgLenTotal - transLen) >= IPOD_TRANSID_LENGTH) && (rc == IPOD_OK))
                    {
                        iPod_convert_to_big16(&iPod_msg[transLen], sendInfo->transID);
                    }
                    else
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "No length available for inserting Trans Id");
                    }


                    if((transLen + IPOD_TRANSID_LENGTH > 0) && (rc == IPOD_OK) && (sendInfo->msgLenTotal >= sendInfo->msgLen ))
                    {
                        /* PRQA: Lint Message 662: The allocated memory size was checked and found to be okay */
                        /*lint -save -e662 */
                        memcpy(&iPod_msg[transLen + IPOD_TRANSID_LENGTH], &buf[transLen],
                               (sendInfo->msgLen - (transLen + IPOD_TRANSID_LENGTH)));
                        /*lint -restore */
                    }
                    else
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error transLen = %d msgLenTotal = %d msgLen = %d",transLen,sendInfo->msgLenTotal,sendInfo->msgLen);
                    }

                    /* remove the length of transID */
                    buf[IPOD_POS1] -= IPOD_TRANSID_LENGTH;
                }
                else
                {
                    if(sendInfo->msgLenTotal >= sendInfo->msgLen)
                    {
                        memcpy(iPod_msg, buf, sendInfo->msgLen);
                    }
                    else
                    {
                        rc = IPOD_ERROR;
                        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "sendInfo->msgLenTotalis less than sendInfo->msgLen");
                    }
                }

                if(sendInfo->msgLenTotal > sendInfo->msgLen)
                {
                    iPod_msg[sendInfo->msgLen] = sendInfo->checksum;
                }
                else
                {
                    rc = IPOD_ERROR;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "sendInfo->msgLenTotalis less than or equal to sendInfo->msgLen");
                }

                if(rc == IPOD_OK)
                {
                    if((iPodHndl->connection == IPOD_USB_HOST_CONNECT) ||
                        (iPodHndl->connection == IPOD_USB_FUNC_CONNECT))
                    {
                        rc = iPodSendPacket(iPodHndl, sendInfo->msgLenTotal, iPod_msg);
                    }
                    else
                    {
                        rc = iPodSendSerial(iPodHndl, sendInfo->msgLenTotal, iPod_msg);
                    }
                }
                else
                {
                    rc = IPOD_ERROR;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error rc = %d",rc);
                }
                free(iPod_msg);
            }
            else
            {
                rc = IPOD_BAD_PARAMETER;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter iPod_msg is NULL");
            }
        }
    }
    else  /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter iPodHndl = %p buf = %p sendInfo = %p err = %d",iPodHndl,buf,sendInfo,err);
    }

    /* release semaphore */
    if (IPOD_ERR_TMOUT != err)
    {
        err = iPodSigSenderSemaphore();
    }
    if(err != IPOD_OK)
    {
        if(rc != IPOD_NOT_CONNECTED)
        {
            rc = IPOD_ERR_DISWAI;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Wait released by wait disabled state");
        }
    }

    return rc;
}


S32 iPodSendCommand(IPOD_INSTANCE* iPodHndl, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;

    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));

    if(buf != NULL)
    {
        sendInfo.msgLen = buf[IPOD_POS1] + (1 + 1); /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = (sendInfo.msgLen + IPOD_CMD_HEADER_LEN);
        sendInfo.checksum = iPodCalcChecksum(buf);
        sendInfo.waitFlg = IPOD_WAIT_ACK_SHORT;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    if(rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

/* This API is used only to return Certificate data to the iPod */
S32 iPodSendCommandCertificate(IPOD_INSTANCE* iPodHndl, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;

    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));
    
    if(buf != NULL)
    {
        sendInfo.msgLen = buf[IPOD_POS1] + (1 + 1); /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = (sendInfo.msgLen + IPOD_CMD_HEADER_LEN);
        sendInfo.checksum = iPodCalcChecksum(buf);
        sendInfo.waitFlg = IPOD_WAIT_ACK_REPLY;
        /* In case of sending the certificate, it is a reply to an iPod Command, so the Transaction ID sent by the iPod must be used */
        iPodHndl->rcvMsgInfo.expectedTransID = iPodHndl->rcvMsgInfo.iPodTransID;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    if(rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

S32 iPodSendSignature(IPOD_INSTANCE* iPodHndl, U8 *buf, U16 transID)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;
    
    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));
    
    
    if(buf != NULL)
    {
        sendInfo.msgLen = buf[IPOD_POS1] + (1 + 1); /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = (sendInfo.msgLen + IPOD_CMD_HEADER_LEN);
        sendInfo.checksum = iPodCalcChecksum(buf);
        sendInfo.waitFlg = IPOD_NO_WAIT_ACK;
        sendInfo.transID = transID;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - buf is NULL");
    }
    if(rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

S32 iPodSendCommandNoWaitForACK(IPOD_INSTANCE* iPodHndl, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;
    
    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));
    
    
    if(buf != NULL)
    {
        sendInfo.msgLen = buf[IPOD_POS1] + (1 + 1); /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = (sendInfo.msgLen + IPOD_CMD_HEADER_LEN);
        sendInfo.checksum = iPodCalcChecksum(buf);
        sendInfo.waitFlg = IPOD_NO_WAIT_ACK;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - buf is NULL");
    }
    if(rc != IPOD_OK)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "rc = %d",rc);
    }

    return rc;
}

S32 iPodSendLongTelegram(IPOD_INSTANCE* iPodHndl, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;

    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));
    
    
    if(buf != NULL)
    {
        sendInfo.msgLen = iPod_convert_to_little16((U8 *) &buf[IPOD_POS2]) + 4; /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = sendInfo.msgLen + IPOD_CMD_HEADER_LEN;
        sendInfo.checksum = iPodCalcChecksumLongTelegram(buf);
        sendInfo.waitFlg = IPOD_WAIT_ACK_LONG;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - buf is NULL");
    }

    return rc;
}

S32 iPodSendLongTelegramNoWaitForACK(IPOD_INSTANCE* iPodHndl, U8 *buf)
{
    S32 rc = IPOD_OK;
    IPOD_SENDMSGINFO sendInfo;

    /* Initialize the structure */
    memset(&sendInfo, 0, sizeof(sendInfo));
    
    if(buf != NULL)
    {
        sendInfo.msgLen = iPod_convert_to_little16((U8 *) &buf[IPOD_POS2]) + 4; /* start byte(0x55) and length bytes*/
        sendInfo.msgLenTotal = sendInfo.msgLen + IPOD_CMD_HEADER_LEN;
        sendInfo.checksum = iPodCalcChecksumLongTelegram(buf);
        sendInfo.waitFlg = IPOD_NO_WAIT_ACK_LONG;
        rc = iPodSendMessage(iPodHndl, buf, &sendInfo);
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - buf is NULL");
    }

    return rc;
}

LOCAL void iPodGetiAPInfo(MessageHeaderInfoType *MessageHeaderInfo, const U8 *header, U16 *dataStartPos)
{
    if (header[IPOD_POS1] == 0x00)      /* check for Payload length marker */
    {
        /* large telegram format */
        /* set telegram lingo */
        MessageHeaderInfo->telegramLingo = header[IPOD_POS4];

        if (MessageHeaderInfo->telegramLingo == IPOD_EXTENDED_LINGO)   /* check if telegram is extended interface mode telegram */
        {
            /* set telegram cmd id */
            MessageHeaderInfo->telegramCmdId = (((U16)header[IPOD_POS5] << IPOD_SHIFT_8) | 
                                                (U16)header[IPOD_POS6]);
            /* set telegram length */
            MessageHeaderInfo->telegramLen = (((U16)header[IPOD_POS2] << IPOD_SHIFT_8) |
                                              (U16)header[IPOD_POS3]);
            /* we only want the message length, without the lingo and command bytes */
            MessageHeaderInfo->telegramLen    -=   LENGTH_LINGO_COMMAND_BYTES_3;
            *dataStartPos = IPOD_POS7;
        }
        else
        {
            /* command byte (only one for general lingo commands) */
            /* set telegram cmd id */
            MessageHeaderInfo->telegramCmdId = header[IPOD_POS5];
            /* set telegram length */
            MessageHeaderInfo->telegramLen = (((U16)header[IPOD_POS2] << IPOD_SHIFT_8) |
                                              (U16)header[IPOD_POS3]);

            /* we only want the message length, without the lingo and command byte */
            MessageHeaderInfo->telegramLen -= LENGTH_LINGO_COMMAND_BYTES_2;
            *dataStartPos = IPOD_POS6;
        }
    }
    else
    {
        /* small telegram format */
        /* set telegram lingo */
        MessageHeaderInfo->telegramLingo = header[IPOD_POS2];

        /* check if telegram is extended interface mode telegram */
        if (MessageHeaderInfo->telegramLingo == IPOD_EXTENDED_LINGO)
        {
            /* set telegram cmd id */
            MessageHeaderInfo->telegramCmdId = (((U16)header[IPOD_POS3] << IPOD_SHIFT_8) |
                                                (U16)header[IPOD_POS4]); 
            /* set telegram length */
            MessageHeaderInfo->telegramLen = (U16)header[IPOD_POS1] - LENGTH_LINGO_COMMAND_BYTES_3;    /* we only want the message length, without the lingo and command bytes */
            *dataStartPos = IPOD_POS5;
        }
        else
        {
            /* set telegram cmd id */
            MessageHeaderInfo->telegramCmdId = header[IPOD_POS3];

            /* set telegram length */
            MessageHeaderInfo->telegramLen = (U16)header[IPOD_POS1] - LENGTH_LINGO_COMMAND_BYTES_2;    /* we only want the message length, without the lingo and command byte */

            /* command result status */
            /* set error code */
            

            
            *dataStartPos = IPOD_POS4;
        }
    }
}

LOCAL void iPodGetiAPAckInfo(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType *MessageHeaderInfo, const U8 *header)
{
    /* General Lingo Ack Command ID and Command result status */
    if (MessageHeaderInfo->telegramLingo == IPOD_GENERAL_LINGO)
    {
        if(MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_ACK)
        {
            MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5];
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
            if(MessageHeaderInfo->telegramErrorCode == IPOD_ACK_WAIT_FOR_ACK)
            {
                MessageHeaderInfo->telegramErrorCode = 0;
            }
            
            if((MessageHeaderInfo->ackCmdId == IPOD_GENERAL_LINGO_StartIDPS) && (MessageHeaderInfo->telegramErrorCode == IPOD_ACKERR_BAD_PARAM))
            {
                /* No support IDPS */
                iPodHndl->rcvMsgInfo.iPodTransID = 0;
                iPodHndl->rcvMsgInfo.accTransID = 0;
                iPodSetStartIDPS(iPodHndl, FALSE);
                MessageHeaderInfo->transID = 0;
            }
        }
        else if (MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_AckDevAuthenticationStatus)
        {
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
        }
        else
        {
            /* For QAC warning*/
        }
    }

    /* Extended Lingo Ack Command ID and Command result status */
    if ((MessageHeaderInfo->telegramLingo == IPOD_EXTENDED_LINGO) && 
        (MessageHeaderInfo->telegramCmdId == IPOD_EXTENDED_LINGO_ACKNOWLEDGE))
    {
        MessageHeaderInfo->ackCmdId = (((U16)header[IPOD_POS6] << IPOD_SHIFT_8) | 
                                       (U16)header[IPOD_POS7]);
        MessageHeaderInfo->telegramErrorCode = header[IPOD_POS5];
    }

    /* Storage Lingo Ack Command ID and Command result status */
    if ((MessageHeaderInfo->telegramLingo == IPOD_STORAGE_LINGO) && 
        (MessageHeaderInfo->telegramCmdId == IPOD_STORAGE_LINGO_ACKNOWLEDGE))
    {
        MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5];
        MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
    }
    
    /* Simple Lingo Ack Command ID and Command result status */
    if ((MessageHeaderInfo->telegramLingo == IPOD_SIMPLE_LINGO) && 
        (MessageHeaderInfo->telegramCmdId == IPOD_SIMPLE_LINGO_ACKNOWLEDGE))
    {
        MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5];
        MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
    }
    
    /* Display Remote Lingo Ack Command ID and Command result status */
    if ((MessageHeaderInfo->telegramLingo == IPOD_DISPLAY_REMOTE_LINGO) && 
        (MessageHeaderInfo->telegramCmdId == IPOD_DISPLAY_LINGO_ACKNOWLEDGE))
    {
        MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5];
        MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
    }
    
    /* iPodOut Lingo Ack Command ID and Command result status */
    if ((MessageHeaderInfo->telegramLingo == IPOD_IPODOUT_LINGO) && 
        (MessageHeaderInfo->telegramCmdId == IPOD_IPODOUT_LINGO_iPodAck))
    {
        MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5];
        MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4];
    }
}

LOCAL void iPodGetTelegramHeaderInfo(MessageHeaderInfoType *MessageHeaderInfo, IPOD_INSTANCE* iPodHndl)
{
    U8 *header  = NULL;
    U16 dataStartPos;
    
    MessageHeaderInfo->telegramErrorCode = IPOD_OK;
    header = MessageHeaderInfo->msgBuffer;


    if ((MessageHeaderInfo->telegramType == IPOD_MESSAGE)
        && (header[IPOD_POS0] == IPOD_START_OF_PACKET))  /* 0x55 is packet start marker      */
    {
        /* Get Telegram Lingo, command ID and data length */
        iPodGetiAPInfo(MessageHeaderInfo, header, &dataStartPos);
        
        /* Get iPod Ack command ID */
        
        if(((iPodHndl->rcvMsgInfo.startIDPS != FALSE) && ((MessageHeaderInfo->telegramCmdId != IPOD_GENERAL_LINGO_ACK) || (MessageHeaderInfo->telegramLen != 0x02)
                                                          || (MessageHeaderInfo->telegramLingo != IPOD_GENERAL_LINGO))) || 
           ((MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_ACK) && (MessageHeaderInfo->telegramLen == IPOD_ACKERR_BAD_PARAM)
            && (MessageHeaderInfo->telegramLingo == IPOD_GENERAL_LINGO)))
        {
            iPodGetIDPSInfo(iPodHndl, MessageHeaderInfo, &dataStartPos);
        }
        else
        {
            iPodGetiAPAckInfo(iPodHndl, MessageHeaderInfo, header);
        }

        /* delete the buffer of the previous telegram data */
        if (MessageHeaderInfo->iPodResponseBuffer != NULL)
        {
            free(MessageHeaderInfo->iPodResponseBuffer);
            MessageHeaderInfo->iPodResponseBuffer = NULL;
        }

        /* telegrams with telegramLen 0 will be ignored */
        if (MessageHeaderInfo->telegramLen > 0)
        {
            /* create a buffer to hold the new telegram data */
            if(MessageHeaderInfo->telegramLen < ((iPodHndl->iAP1MaxPayloadSize + IPOD_HEADER_SIZE5) - dataStartPos))
            {
                MessageHeaderInfo->iPodResponseBuffer = (U8*) calloc(MessageHeaderInfo->telegramLen, sizeof(U8));
                if (MessageHeaderInfo->iPodResponseBuffer != NULL)
                {
                    memcpy(MessageHeaderInfo->iPodResponseBuffer,
                           &(MessageHeaderInfo->msgBuffer[dataStartPos]),
                           MessageHeaderInfo->telegramLen);
                }
            }
            else
            {
                /* buffer size shall never less than telegram length */
                MessageHeaderInfo->telegramType      = IPOD_MESSAGE_DEFECT;
                MessageHeaderInfo->telegramLen       = 0;
                MessageHeaderInfo->telegramCmdId     = IPOD_DEFAULT_CMD_ID;
                MessageHeaderInfo->telegramLingo     = IPOD_DEFAULT_LINGO;
                MessageHeaderInfo->telegramErrorCode = IPOD_DEFAULT_ERROR;
            }
        }
    }
}

LOCAL void iPodGetIDPSInfo(IPOD_INSTANCE* iPodHndl, MessageHeaderInfoType *MessageHeaderInfo, U16 *dataStartPos)
{
    U8 *header = MessageHeaderInfo->msgBuffer;

    /* CmdID is StartIDPS or ack length is different from previous ack lenght or CmdID is not Ack */
    if(!((MessageHeaderInfo->ackCmdId == IPOD_GENERAL_LINGO_StartIDPS) 
         && (header[IPOD_POS1] == IPOD_PREV_ACK_LENGTH)
         && (MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_ACK))
       && !((MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_RequestIdentify)
            && (MessageHeaderInfo->telegramLingo == IPOD_GENERAL_LINGO)))
    {
        /* Set transID */
        MessageHeaderInfo->transID = iPod_convert_to_little16((U8*)&(MessageHeaderInfo->msgBuffer[*dataStartPos]));
        /* Decrease transID length */
        MessageHeaderInfo->telegramLen -= IPOD_TRANSID_LENGTH;
        *dataStartPos += IPOD_TRANSID_LENGTH;
        
        /* Support IDPS and General Lingo Ack */
        if (MessageHeaderInfo->telegramLingo == IPOD_GENERAL_LINGO)
        {
            if(MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_ACK)
            {
                MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
                MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH];
                if(MessageHeaderInfo->telegramErrorCode == IPOD_ACK_WAIT_FOR_ACK)
                {
                    MessageHeaderInfo->telegramErrorCode = 0;
                }
                
            }
            /* CmdID is Ack DevAuthentiation */
            else if (MessageHeaderInfo->telegramCmdId == IPOD_GENERAL_LINGO_AckDevAuthenticationStatus)
            {
                MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH];
            }
            else
            {
                /* For QAC warning*/
            }
        }

        /* Support IDPS and Extended Lingo Ack */
        if ((MessageHeaderInfo->telegramLingo == IPOD_EXTENDED_LINGO) &&
            (MessageHeaderInfo->telegramCmdId == IPOD_EXTENDED_LINGO_ACKNOWLEDGE))
        {
            MessageHeaderInfo->ackCmdId = (((U16)header[IPOD_POS6 + IPOD_TRANSID_LENGTH] << IPOD_SHIFT_8) | 
                                           (U16)header[IPOD_POS7 + IPOD_TRANSID_LENGTH]);
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
        }

        /* Support IDPS and Storage Lingo Ack */
        if ((MessageHeaderInfo->telegramLingo == IPOD_STORAGE_LINGO) &&
            (MessageHeaderInfo->telegramCmdId == IPOD_STORAGE_LINGO_ACKNOWLEDGE))
        {
            MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH]; 
        }
        
        /* Support IDPS and Simple Lingo Ack */
        if ((MessageHeaderInfo->telegramLingo == IPOD_SIMPLE_LINGO) &&
            (MessageHeaderInfo->telegramCmdId == IPOD_SIMPLE_LINGO_ACKNOWLEDGE))
        {
            MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH]; 
        }
        /* Display Remote Lingo Ack Command ID and Command result status */
        if ((MessageHeaderInfo->telegramLingo == IPOD_DISPLAY_REMOTE_LINGO) && 
            (MessageHeaderInfo->telegramCmdId == IPOD_DISPLAY_LINGO_ACKNOWLEDGE))
        {
            MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH];
        }
        /* Support IDPS and iPodOut Lingo Ack */
        if ((MessageHeaderInfo->telegramLingo == IPOD_IPODOUT_LINGO) &&
            (MessageHeaderInfo->telegramCmdId == IPOD_IPODOUT_LINGO_iPodAck))
        {
            MessageHeaderInfo->ackCmdId = (U16)header[IPOD_POS5 + IPOD_TRANSID_LENGTH];
            MessageHeaderInfo->telegramErrorCode = header[IPOD_POS4 + IPOD_TRANSID_LENGTH]; 
        }
        

        iPodHndl->rcvMsgInfo.iPodTransID = MessageHeaderInfo->transID;
    }
    else
    {
        /* No support IDPS */

        if((MessageHeaderInfo->telegramCmdId != IPOD_GENERAL_LINGO_RequestIdentify)
                &&  (MessageHeaderInfo->telegramLingo != IPOD_GENERAL_LINGO))
        {
            /*Don't reset TransID for RequestIdentify (R46 does not specify anything on this)*/
            iPodHndl->rcvMsgInfo.iPodTransID = 0;
            iPodHndl->rcvMsgInfo.accTransID = 0;
            MessageHeaderInfo->transID = 0;
        }
        iPodSetStartIDPS(iPodHndl, FALSE);
    }
}

void iPodSetAccInfo(MessageHeaderInfoType *info, IPOD_INSTANCE* iPodHndl)
{
    if(info->telegramLen < iPodHndl->iAP1MaxPayloadSize)
    {
        memcpy(iPodHndl->iPodRetAccInfoData, info->iPodResponseBuffer, info->telegramLen);
        iPodHndl->iPodRetAccInfoFlg = 1;
    }
}
/*!
 * \fn iPodGetMaxPayloadSize
 * \par INPUT PARAMETERS
 * U32 iPodID
 * \par REPLY PARAMETERS
 * U16 MaximumPayloadSize
 * \par DESCRIPTION
 * This function returns the Maximum Payload Size.
 */
U16 iPodGetMaxPayloadSize(U32 iPodID)
{
    U16 iAP1MaxPayloadSize = 0;
    IPOD_INSTANCE *iPodHndl = iPodGetHandle(iPodID);

    if(iPodHndl == NULL)
    {
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Invalid handle");
    }
    else
    {
        iAP1MaxPayloadSize = iPodHndl->iAP1MaxPayloadSize;
    }

    return iAP1MaxPayloadSize;
}

