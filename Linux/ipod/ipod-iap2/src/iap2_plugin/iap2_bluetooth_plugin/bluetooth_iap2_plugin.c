/*****************************************************************************
-  \file : bluetooth_iap2_plugin.c
-  \version : $Id: usb-bluetooth_iap2_plugin.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for BT communication
-  \component :
-  \author : Manavalan Veeramani / RBEI / manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2015 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/
#include "bluetooth_iap2_plugin.h"
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <errno.h>
#include <poll.h>

#include "iap2_dlt_log.h"

LOCAL inline void iAP2FreePointer(void** ptr2_memfree)
{
    if(*ptr2_memfree != NULL)
    {
        free(*ptr2_memfree);
        *ptr2_memfree = NULL;
    }
}

LOCAL inline S32 iAP2AllocateandCopyString(U8** Dest, const U8* Source)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if( (Dest != NULL) && (Source != NULL) )
    {
        U32 Length;

        Length = strnlen((const char*)Source, IPOD_IAP2_BT_MAC_STRING_MAX) + 1;
        *Dest = calloc(Length , sizeof(U8) );
        if(*Dest == NULL)
        {
            rc = IPOD_DATACOM_ERR_NOMEM;
        }
        else
        {
            strncpy((char*)*Dest, (const char*)Source, Length);
        }
    }

    return rc;
}

LOCAL S32 iAP2BTClosePlugin(void* iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IAP2_BT_INFO *devinfo = (IAP2_BT_INFO*)iPodHdl;

    if(devinfo != NULL)
    {
        iAP2FreePointer( (void**)&devinfo->iAP2DeviceMAC );
        if(devinfo->iAP2BTSocket != -1)
        {
            close(devinfo->iAP2BTSocket);
            devinfo->iAP2BTSocket = -1;
        }
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

LOCAL S32 iAP2BTAbort(void* iPodHdl)
{
    (void)iPodHdl;

    return IPOD_DATACOM_SUCCESS;
}

LOCAL S32 iAP2BTHandleEvent(void* iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IAP2_BT_INFO* devinfo = (IAP2_BT_INFO*)iPodHdl;

    if ( (devinfo != NULL) && (msgBuffer != NULL) )
    {
        if(flags == devinfo->iAP2BTSocket)
        {
            rc = recv(devinfo->iAP2BTSocket, (char*)msgBuffer, buffer_size, 0);
            if (rc == -1)
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "Error in reading data errno: %d %s", errno, strerror(errno));
            }
        }
        else
        {
            IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "Unknown file descriptor=%d, iAP2BTSocket = %d", flags, devinfo->iAP2BTSocket);
        }
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
    }

    return rc;
}

LOCAL S32 iAP2BTGetFDs(void* iPodHdl, IPOD_IAP2_DATACOM_FD* getFDs, S32* fd_count)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IAP2_BT_INFO *devinfo = (IAP2_BT_INFO*)iPodHdl;

    if(devinfo != NULL)
    {
        *fd_count = 1;
        if(getFDs != NULL)
        {
            getFDs[0].event = POLLIN;
            getFDs[0].fd    = devinfo->iAP2BTSocket;
        }
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }

    return rc;
}

LOCAL S32 iAP2BTSend(void* iPodHdl, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32 rc = IPOD_DATACOM_ERROR;
    IAP2_BT_INFO *devinfo = (IAP2_BT_INFO*) iPodHdl;

    /* for compiler warnings */
    (void)flags;

    if ( (devinfo != NULL) && (iPod_msg != NULL) && (msgLenTotal > 0) )
    {
        if (devinfo->iAP2BTSocket != -1)
        {
            rc = send(devinfo->iAP2BTSocket, iPod_msg, (size_t)msgLenTotal, 0);
            if (rc == -1)
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, " send failure, errno = %d (%s)", errno, strerror(errno));
            }
            else
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, " sent data rc = %d", rc);
            }
        }
        else
        {
            IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "iAP2BTSocket=%d is not valid", devinfo->iAP2BTSocket);
        }
    }
    else /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, iPod_msg = %p, msgLenTotal = %d", devinfo, iPod_msg, msgLenTotal);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

LOCAL S32 iAP2BTReceiveMessage(void* iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    /* for compiler warnings */
    (void)iPodHdl;
    (void)buffer_size;
    (void)msgBuffer;
    (void)flags;

    return IPOD_DATACOM_SUCCESS;
}

LOCAL S32 iAP2BTOpenPlugin(void *iPodHdl, const U8* device_name, S32 flags, S32 mode)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IAP2_BT_INFO *devinfo = (IAP2_BT_INFO*)iPodHdl;

    /* for compiler warnings */
    (void)flags;
    (void)mode;

    if( (NULL != devinfo) && (NULL != device_name) )
    {
        // allocate a socket
        devinfo->iAP2BTSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

        if(devinfo->iAP2BTSocket == -1)
        {
            IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "Error in Creating the Socket, errno = %d (%s)", errno, strerror(errno));
            rc = IPOD_DATACOM_ERROR;
        }
        else
        {
            struct sockaddr_rc addr_new = { 0 };

            IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "Allocated Socket Successfully");

            // set the connection parameters (who to connect to)
            addr_new.rc_family  = AF_BLUETOOTH;
            addr_new.rc_channel = (uint8_t) 1;
            str2ba((char*)device_name, &addr_new.rc_bdaddr );

            // connect to server
            /* PRQA: Lint Message 64: Lint describes which the second parameter is "__CONST_SOCKADDR_ARG"
            but according to spec of connect, the type of second parameter is "const struct sockaddr *". so this cast is correct */
            rc = connect(devinfo->iAP2BTSocket, (struct sockaddr *)&addr_new, sizeof(addr_new)); /*lint !e64 */
            if(rc == -1)
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "Error in creating connection, errno = %d (%s)", errno, strerror(errno));
            }
            else
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "connected through BT Successfully");
            }
        }
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, device_name = %p", devinfo, device_name);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    if (rc != IPOD_DATACOM_SUCCESS)
    {
        (void)iAP2BTClosePlugin(iPodHdl);
    }
    IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

LOCAL S32 iAP2BTIoCtl(void* iPodHdl, S32 request, void* argp)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IAP2_BT_INFO *devinfo = (IAP2_BT_INFO*)iPodHdl;

    if( (devinfo != NULL) && (argp != NULL) )
    {
        switch(request)
        {
            case IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG:
            {
                IPOD_IAP2_DATACOM_IOCTL_CONFIG* config = (IPOD_IAP2_DATACOM_IOCTL_CONFIG*)argp;

                if(config->DeviceMAC == NULL)
                {
                    rc = IPOD_DATACOM_BAD_PARAMETER;
                    IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "DeviceMAC is NULL");
                }
                else
                {
                    rc = iAP2AllocateandCopyString(&devinfo->iAP2DeviceMAC, config->DeviceMAC);
                }
                break;
            }

            default:
            {
                IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "request = %d", request);
                rc = IPOD_DATACOM_ERROR;
                break;
            }
        }
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, argp = %p", devinfo, argp);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

void* iPodiAP2BTComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function)
{
    IAP2_BT_INFO* devinfo = NULL;

    data_com_function->ioctl    = &iAP2BTIoCtl;
    data_com_function->open     = &iAP2BTOpenPlugin;
    data_com_function->read     = &iAP2BTReceiveMessage;
    data_com_function->write    = &iAP2BTSend;
    data_com_function->getfds   = &iAP2BTGetFDs;
    data_com_function->hdlevent = &iAP2BTHandleEvent;
    data_com_function->property = NULL;
    data_com_function->abort    = &iAP2BTAbort;
    data_com_function->close    = &iAP2BTClosePlugin;

    devinfo = calloc(1, sizeof(IAP2_BT_INFO) );
    if (devinfo != NULL)
    {
    }
    else
    {
        IAP2BTPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }
    IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo = %p", devinfo);

    return devinfo;
}

S32 iPodiAP2BTComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function, void* iPodHdl )
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IAP2_BT_INFO* devinfo = (IAP2_BT_INFO*)iPodHdl;

    if (data_com_function != NULL)
    {
        memset(data_com_function, 0, sizeof(IPOD_IAP2_DATACOM_FUNC_TABLE) );
    }

    if (devinfo != NULL)
    {
        iAP2FreePointer( (void**)&devinfo);
    }
    IAP2BTPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}
