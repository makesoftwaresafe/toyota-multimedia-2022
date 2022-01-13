/*****************************************************************************
-  \file : iap2_over_carplay_plugin.c
-  \version : $Id: iap2_over_carplay_plugin.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for iAP over CarPlay communication
-  i.e., for CarPlay over WiFi, the iAP messages will be sent as part of CarPlay message.
-  \component :
-  \author : Manavalan Veeramani / RBEI / manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2016 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include "iap2_over_carplay_plugin.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "iap2_dlt_log.h"

LOCAL inline void iAP2OverCarPlayCloseFd(S32* fd)
{
    if(*fd >= 0)
    {
        close(*fd);
        *fd = -1;
    }
}

LOCAL S32 iAP2OverCarPlayClosePlugin(void *iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*)iPodHdl;

    iAP2OverCarPlayCloseFd(&devinfo->socket_fd);

    return rc;
}

LOCAL S32 iAP2OverCarPlayGetProperty(void *iPodHdl, IPOD_IAP2_DATACOM_PROPERTY *property)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*)iPodHdl;

    if( (devinfo != NULL) && (property != NULL) )
    {
        property->maxSize = IPOD_IAP2_USBHOST_MAX_SEND_BYTES;
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, property = %p", devinfo, property);
    }

    return rc;
}

LOCAL S32 iAP2OverCarPlayCloseMsgHandling(void *iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    (void)iPodHdl;

    return rc;
}

LOCAL S32 iAP2OverCarPlayGetFDs(void *iPodHdl, IPOD_IAP2_DATACOM_FD *getFDs, S32 *fd_count)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*)iPodHdl;

    if(devinfo != NULL)
    {
        *fd_count = 1;
        if(getFDs != NULL)
        {
            getFDs[0].event = POLLIN;
            getFDs[0].fd    = devinfo->socket_fd;
        }
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }

    return rc;
}

LOCAL S32 iAP2OverCarPlayLibUSBHandleEvent(void *iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc  = IPOD_DATACOM_SUCCESS;
    S32 ret = IPOD_DATACOM_ERROR;

    IAP2_OVER_CARPLAY_DEV_INFO* devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*)iPodHdl;

    if ( (devinfo != NULL) && (msgBuffer != NULL) )
    {
        if(flags == devinfo->socket_fd)
        {
            ret = read(devinfo->socket_fd, (char*)msgBuffer, buffer_size);
            if (ret < 0)
            {
                if(errno == EAGAIN)
                {
                    /* Andreas: if read returns EAGAIN, resource temporarily available try again */
                    rc = 0;
                }
                else
                {
                    rc = ret;
                }
                IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "read ret =  %d | errno: %d %s", ret, errno, strerror(errno));
            }
            else if(ret == 0)//EOF other side closed
            {
                IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_INFO, "EOF: read ret =  %d", ret);
                rc = IPOD_DATACOM_NOT_CONNECTED;
            }

            else
            {
                rc = ret;
            }
        }
    }
    else
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

LOCAL S32 iAP2OverCarPlaySendtoCarPlayd(void *iPodHdl, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    S32 err = IPOD_DATACOM_SUCCESS;
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*) iPodHdl;

    /* for compiler warnings */
    flags = flags;

    if ( (devinfo != NULL) && (iPod_msg != NULL) && (msgLenTotal > 0) )
    {
        if (devinfo->socket_fd >= 0)
        {
            rc = (S32)send(devinfo->socket_fd, (void*) iPod_msg, (size_t) msgLenTotal, 0);
            if (rc < 0)
            {
                /* get errno in case send() return with an error */
                err = errno;
                IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "unexpected error occurred, errno=%d, (%s)", err, strerror(err));
                rc = IPOD_DATACOM_ERROR;
            }
        }
        else
        {
            /* fd should not be less than zero.
             * this is only possible in case close was called before. */
            IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "write_fd=%d is not valid", devinfo->socket_fd);
            rc = IPOD_DATACOM_ERROR;
        }
    }
    else /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, iPod_msg = %p, msgLenTotal = %d", devinfo, iPod_msg, msgLenTotal);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

LOCAL S32 iAP2OverCarPlayReceiveMessage(void *iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    (void)iPodHdl;
    (void)buffer_size;
    (void)msgBuffer;
    (void)flags;

    return rc;
}

LOCAL S32 iAP2OverCarPlayOpenPlugin(void *iPodHdl, const U8 *device_name, S32 flags, S32 mode)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    struct sockaddr_un address;
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = (IAP2_OVER_CARPLAY_DEV_INFO*)iPodHdl;

    (void)mode;
    (void)flags;

    if(NULL != devinfo)
    {
        devinfo->socket_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
        if(devinfo->socket_fd < 0)
        {
            IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "Error in creating socket errno = %d, (%s)", errno, strerror(errno));
            rc = IPOD_DATACOM_ERROR;
        }
        else
        {
            IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_DEBUG, "Socket created successfully fd = %d", devinfo->socket_fd);
        }
        if(rc == IPOD_DATACOM_SUCCESS)
        {
            memset(&address, 0, sizeof(struct sockaddr_un));

            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, IAP2_OVER_CARPLAY, sizeof(address.sun_path));
            address.sun_path[sizeof(address.sun_path)-1] = '\0';

            /* PRQA: Lint Message 64: Lint describes which the second parameter is "__CONST_SOCKADDR_ARG"
            but according to spec of connect, the type of second parameter is "const struct sockaddr *". so this cast is correct */
            if(connect(devinfo->socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) /*lint !e64 */
            {
                IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "connect() failed errno = %d, (%s)", errno, strerror(errno));
                rc = IPOD_DATACOM_ERROR;
            }
        }
    }
    else
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, device_name = %p", devinfo, device_name);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

void* iAP2OverCarPlayComInit(IPOD_IAP2_DATACOM_FUNC_TABLE *data_com_function)
{
    IAP2_OVER_CARPLAY_DEV_INFO *devinfo = NULL;

    data_com_function->ioctl    = NULL;
    data_com_function->open     = &iAP2OverCarPlayOpenPlugin;
    data_com_function->read     = &iAP2OverCarPlayReceiveMessage;
    data_com_function->write    = &iAP2OverCarPlaySendtoCarPlayd;
    data_com_function->hdlevent = &iAP2OverCarPlayLibUSBHandleEvent;
    data_com_function->getfds   = &iAP2OverCarPlayGetFDs;
    data_com_function->abort    = &iAP2OverCarPlayCloseMsgHandling;
    data_com_function->property = &iAP2OverCarPlayGetProperty;
    data_com_function->close    = &iAP2OverCarPlayClosePlugin;

    devinfo = calloc(1, sizeof(IAP2_OVER_CARPLAY_DEV_INFO) );
    if (devinfo == NULL)
    {
        IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }
    IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo = %p", devinfo);

    return devinfo;
}

S32 iAP2OverCarPlayComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE *data_com_function, void *iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    (void)data_com_function;
    (void)iPodHdl;

    IAP2OVERCARPLAYPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}
