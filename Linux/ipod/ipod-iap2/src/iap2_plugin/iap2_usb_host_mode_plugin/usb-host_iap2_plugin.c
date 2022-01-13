/*****************************************************************************
-  \file : usb-host_iap2_plugin.c
-  \version : $Id: usb-host_iap2_plugin.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for USB Host mode communication
-  i.e., the Accessory acts as gadget and the iOS device acts as host.
-  \component :
-  \author : Manavalan Veeramani / RBEI / manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mount.h>
#include <linux/usb/functionfs.h>
#include <pthread_adit.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>    /* For mode constants */
#include <sys/ioctl.h>
#include <errno.h>

#include "iap2_dlt_log.h"
#include "iap2_host_datacom.h"

/*
 * PRQA: Lint Message 754: parent structure size and address are used in write()
 *       call in  ep0_init() function.
 */
/*lint -esym(754,header,code,str1,str2,lang0)*/

LOCAL inline S32 iAP2AllocateandAppendString(U8** Dest, const char* Source, const char appendstr)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if( (Dest != NULL) && (Source != NULL) )
    {
        U32 Length;

        Length = strnlen(Source, IPOD_IAP2_USBHOST_STRING_MAX) + 2;
        *Dest = calloc(Length , sizeof(U8) );
        if(*Dest == NULL)
        {
            rc = IPOD_DATACOM_ERR_NOMEM;
        }
        else
        {
            snprintf( (char*)*Dest, Length, "%s%c", Source, appendstr);
        }
    }

    return rc;
}

LOCAL inline void iAP2FreePointer(void** input_ptr)
{
    if(*input_ptr != NULL)
    {
        free(*input_ptr);
        *input_ptr = NULL;
    }
}

LOCAL inline void iAP2CloseFd(S32* fd)
{
    if(*fd >= 0)
    {
        close(*fd);
        *fd = -1;
    }
}

/**
 * This function close the running threads.
 *
 * \param void* iPodHdl - device info structure
 */
LOCAL S32 iAP2USBHostClosePlugin(void* iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if(devinfo != NULL)
    {
        /* close all open file descriptors (endpoint, mq, etc.) */
        iAP2CloseFd(&devinfo->ep0_fd);
        iAP2CloseFd(&devinfo->write_fd);
        iAP2CloseFd(&devinfo->read_fd);

        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

LOCAL S32 iAP2USBHostGetProperty(void* iPodHdl, IPOD_IAP2_DATACOM_PROPERTY *property)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if( (devinfo != NULL) && (property != NULL) )
    {
        property->maxSize = IPOD_IAP2_USBHOST_MAX_SEND_BYTES;
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, property = %p", devinfo, property);
    }

    return rc;
}

/**
 * This function abort the current communication
 * and close the open file descriptors.
 *
 * \param void* iPodHdl - device info structure
 */
LOCAL S32 iAP2USBHostCloseMsgHandling(void* iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    (void)iPodHdl;

    return rc;
}

LOCAL S32 iAP2USBHostLibUSBGetFDs(void* iPodHdl, IPOD_IAP2_DATACOM_FD* getFDs, S32* fd_count)
{
    S32 rc = IPOD_DATACOM_BAD_PARAMETER;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if(devinfo != NULL)
    {
        *fd_count = 2;
        if(getFDs != NULL)
        {
            getFDs[0].event = POLLIN;
            getFDs[0].fd    = devinfo->read_fd;
            getFDs[1].event = POLLIN;
            getFDs[1].fd    = devinfo->ep0_fd;
        }
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }

    return rc;
}

LOCAL inline char getEndpoint(char code)
{
    return (char)( (code & 0xF0) >> 4);
}

LOCAL inline char getEvent(char code)
{
    return (char)(code & 0x0F);
}

S32 iAP2USBHostHandleEnable_EAP(IPOD_IAP2_HID_HOSTDEV_INFO* devinfo, const struct usb_functionfs_event *event)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if (devinfo->ioctl_config.nativeTransport == TRUE)
    {
        char interfaceNumber = 0;
        char startStop = 0;
        int i = 0;

        interfaceNumber = getEndpoint(event->code);
        startStop = getEvent(event->code);

        /* Cast interface number to integer */
        i = (int) interfaceNumber;
        if (i > 0)
        {
            /* Normally, the alternate interfaces starting from 1.
             * But, the array of alternate interfaces begin at index 0. */
            i--;
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "interface number not greater than 0 i = %d", i);
        }
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "interfaceNumber = %d, startStop = %d", interfaceNumber, startStop);

        if (startStop == 1)
        {
            if (devinfo->EAcallbacks.p_iAP2StartAlternateIf_cb != NULL)
            {
                devinfo->EAcallbacks.p_iAP2StartAlternateIf_cb(devinfo->ioctl_config.iap2Device,
                        devinfo->alternate_interface[i].iOSAppIdentifier, devinfo->alternate_interface[i].inEndpoint,
                        devinfo->alternate_interface[i].outEndpoint, devinfo->ioctl_config.context);
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "p_iAP2StartAlternateIf_cb is NULL! ");
            }
        }
        else if (startStop == 0)
        {
            if (devinfo->EAcallbacks.p_iAP2StopAlternateIf_cb != NULL)
            {
                devinfo->EAcallbacks.p_iAP2StopAlternateIf_cb(devinfo->ioctl_config.iap2Device,
                        devinfo->alternate_interface[i].iOSAppIdentifier, devinfo->alternate_interface[i].inEndpoint,
                        devinfo->alternate_interface[i].outEndpoint, devinfo->ioctl_config.context);
            }
            else
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "p_iAP2StopAlternateIf_cb is NULL! ");
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "unknown code 0x%X ", event->code);
            rc = IPOD_DATACOM_ERROR;
        }
    }

    return rc;
}

LOCAL S32 EP0_ConsumeEvent(IPOD_IAP2_HID_HOSTDEV_INFO* devinfo, struct usb_functionfs_event *event)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    char *const ffs_event_names[] = {
        [FUNCTIONFS_BIND]       = "BIND",
        [FUNCTIONFS_UNBIND]     = "UNBIND",
        [FUNCTIONFS_ENABLE]     = "ENABLE",
        [FUNCTIONFS_DISABLE]    = "DISABLE",
        [FUNCTIONFS_SETUP]      = "SETUP",
        [FUNCTIONFS_SUSPEND]    = "SUSPEND",
        [FUNCTIONFS_RESUME]     = "RESUME",
    };

    switch (event->type)
    {
        case FUNCTIONFS_BIND:
        case FUNCTIONFS_UNBIND:
        case FUNCTIONFS_SETUP:
        case FUNCTIONFS_RESUME:
        {
            break;
        }
        case FUNCTIONFS_ENABLE:
        {
            rc = iAP2USBHostHandleEnable_EAP(devinfo, event);
            break;
        }
        case FUNCTIONFS_SUSPEND:
        case FUNCTIONFS_DISABLE:
        {
            rc = IPOD_DATACOM_NOT_CONNECTED;
            break;
        }
        default:
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "ERROR: Unknown event type = %d", event->type);
            rc = IPOD_DATACOM_ERROR;
            break;
        }
    }
    if(rc != IPOD_DATACOM_ERROR)
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_INFO, "event %s consumed", ffs_event_names[event->type]);
    }

    return rc;
}

LOCAL S32 iAP2USBHostLibUSBHandleEvent(void* iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    S32 ret = IPOD_DATACOM_ERROR;
    IPOD_IAP2_HID_HOSTDEV_INFO* devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if ( (devinfo != NULL) && (msgBuffer != NULL) )
    {
        if(flags == devinfo->read_fd)
        {
            ret = read(devinfo->read_fd, (char*)msgBuffer, buffer_size);
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
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "read(ep2) ret =  %d | errno: %d %s", ret, errno, strerror(errno));
            }
            else
            {
                rc = ret;
            }
        }
        else if (flags == devinfo->ep0_fd)
        {
            ret = read(devinfo->ep0_fd, (char*)msgBuffer, buffer_size);
            if (ret > 0)
            {
                struct usb_functionfs_event *event = (struct usb_functionfs_event *)((void*)msgBuffer);

                while( (ret >= (S32) sizeof(struct usb_functionfs_event) ) &&
                       (rc == IPOD_DATACOM_SUCCESS) )
                {
                    rc = EP0_ConsumeEvent(devinfo, event);
                    ret -= sizeof(struct usb_functionfs_event);
                    if (ret > 0)
                    {
                        /* If there are more events to be read move to next event in msgBuffer */
                        event += 1;
                    }
                }
                if ( (ret < (S32) sizeof(struct usb_functionfs_event) ) && (ret != 0) )
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "length is less than the sizeof the event structure len=%d", ret);
                    ret = 0;
                }
            }
            else
            {
                if(errno == EAGAIN)
                {
                    /* Andreas: if read returns EAGAIN, resource temporarily available try again */
                    rc = 0;
                }
                else
                {
                    rc = IPOD_DATACOM_ERROR;
                }
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "read(ep0) ret =  %d | errno: %d %s", ret, errno, strerror(errno));
            }
        }
        else
        {
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Unknown file descriptor=%d", flags);
            rc = IPOD_DATACOM_BAD_PARAMETER;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }


    return rc;
}

LOCAL S32 iAP2USBHostSendtoUSB(void* iPodHdl, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    S32 err = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*) iPodHdl;

    /* for compiler warnings */
    flags = flags;

    if ( (devinfo != NULL) && (iPod_msg != NULL) && (msgLenTotal > 0) )
    {
        if (devinfo->write_fd >= 0)
        {
            rc = (S32) write(devinfo->write_fd, (void*) iPod_msg, (size_t) msgLenTotal);
            if (rc < 0)
            {
                /* get errno in case write() return with an erro */
                err = errno;
                /* write() return EAGAIN if the iOS device is (temporary) not available. */
                if (err == EAGAIN)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, " write=%d, errno=EAGAIN", rc);
                    /* we assume that the iOS device is no longer connected. */
                    rc = IPOD_DATACOM_NOT_CONNECTED;
                }
                else if (err == ETIME)
                { /* SWGIII-9274 */
                    /* if write_fd was open with flag O_NONBLOCK and return with ETIME,
                     * and the patch in the g_ffs which writes with a timeout,
                     * we assume that we could not write to the device */
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, " write=%d, errno=ETIME", rc);
                    /* return IPOD_DATACOM_ERR_ABORT because write request was aborted by timer in g_ffs */
                    rc = IPOD_DATACOM_ERR_ABORT;
                }
                else
                {
                    /* another unexpected error occurred. */
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "unexpected error occurred, errno=%d", err);
                    rc = IPOD_DATACOM_ERROR;
                }
            }
        }
        else
        {
            /* write_fd should not less than zero.
             * this is only possible in case close was called before. */
            IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "write_fd=%d is not valid", devinfo->write_fd);
            rc = IPOD_DATACOM_ERROR;
        }
    }
    else /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, iPod_msg = %p, msgLenTotal = %d", devinfo, iPod_msg, msgLenTotal);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

LOCAL S32 iAP2USBHostReceiveMessage(void* iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    iPodHdl     = iPodHdl;
    buffer_size = buffer_size;
    msgBuffer   = msgBuffer;
    flags       = flags;

    return rc;
}

LOCAL S32 iAP2USBHostOpenPlugin(void *iPodHdl, const U8* device_name, S32 flags, S32 mode)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    /* for compiler warnings */
    flags = flags;
    mode = mode;

    if( (NULL != devinfo) && (NULL != device_name) )
    {
        if (devinfo->name[0] == '\0')
        {
            strncpy( (VP)devinfo->name, (VP)device_name, sizeof(devinfo->name) - 1);
        }
        else
        {
            if (strncmp( (VP)devinfo->name, (VP)device_name, sizeof(devinfo->name) ) == 0)
            {
                rc = IPOD_DATACOM_ALREADY_CONNECTED;
            }
        }
        if( (rc == IPOD_DATACOM_SUCCESS) &&
            (devinfo->ioctl_config.useConfigFS == FALSE) )
        {
            rc = iAP2ConfigureFFSGadget(&devinfo->iAP2_ffs_config, FALSE, NULL, NULL);
            if(rc == IPOD_DATACOM_SUCCESS)
            {
                devinfo->ep0_fd = devinfo->iAP2_ffs_config.ep0_fd;
            }
        }
        if(rc == IPOD_DATACOM_SUCCESS)
        {
            if(devinfo->ioctl_config.useConfigFS == TRUE)
            {
                devinfo->ep0_fd   = open( (char*)devinfo->ioctl_config.initEndPoint, O_NONBLOCK | O_RDWR | O_CLOEXEC);
            }
            if(devinfo->ep0_fd  < 0)
            {
				IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,
									"devinfo->ep0_fd = %d,error %d %s",
									devinfo->ep0_fd, errno, strerror(errno));
            }
            devinfo->write_fd = open( (const char*)devinfo->EndPointSink, O_NONBLOCK | O_RDWR | O_CLOEXEC);
            if(devinfo->write_fd  < 0)
            {
				IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,
									"devinfo->write_fd = %d,error %d %s",
									devinfo->write_fd,errno, strerror(errno));
            }
            devinfo->read_fd  = open( (const char*)devinfo->EndPointSource, O_NONBLOCK | O_RDWR | O_CLOEXEC);

            if( (devinfo->ep0_fd  < 0)   ||
                (devinfo->write_fd < 0)  ||
                (devinfo->read_fd  < 0) )
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR,
                                    "devinfo->ep0_fd = %d, devinfo->write_fd = %d, devinfo->read_fd = %d, rc = %d error %d %s",
                                    devinfo->ep0_fd, devinfo->write_fd, devinfo->read_fd, rc,errno,strerror(errno));
                rc = IPOD_DATACOM_ERROR;
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, device_name = %p", devinfo, device_name);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    if (rc != IPOD_DATACOM_SUCCESS)
    {
        (void)iAP2USBHostCloseMsgHandling(iPodHdl);
        (void)iAP2USBHostClosePlugin(iPodHdl);
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

LOCAL void iAP2USBHostFreeIoCtlConfig(IPOD_IAP2_HID_HOSTDEV_INFO *devinfo)
{
    U16 i = 0;

    devinfo->ioctl_config.digitaliPodOut    = FALSE;
    devinfo->ioctl_config.nativeTransport   = FALSE;
    devinfo->ioctl_config.iap2Device        = NULL;
    devinfo->ioctl_config.context           = NULL;

    iAP2FreePointer( (void**)&devinfo->ioctl_config.initEndPoint);
    iAP2FreePointer( (void**)&devinfo->EndPointSink);
    iAP2FreePointer( (void**)&devinfo->EndPointSource);

    iAP2FreePointer( (void**)&devinfo->iAP2_ffs_config.initEndPoint);

    for(i=0; i < devinfo->iAP2_ffs_config.iOSAppCnt; i++)
    {
        iAP2FreePointer( (void**)&devinfo->iAP2_ffs_config.iOSAppNames[i]);
    }
    iAP2FreePointer( (void**)&devinfo->iAP2_ffs_config.iOSAppNames);
    memset(&devinfo->iAP2_ffs_config, 0, sizeof(iAP2_ffs_config_t) );
}

LOCAL S32 iAP2USBHostInitIoCtlConfig(IPOD_IAP2_HID_HOSTDEV_INFO *devinfo, IPOD_IAP2_DATACOM_IOCTL_CONFIG* config)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    U16 i = 0;
    U32 offset = 0;
    S32 len = 0;
    S32 lenEpAddr = 0;
    char EpAddr[IPOD_IAP2_USBHOST_STRING_MAX];
    memset(&EpAddr[0], 0, IPOD_IAP2_USBHOST_STRING_MAX);

    if( (NULL != devinfo) && (NULL != config) )
    {
        devinfo->ioctl_config.digitaliPodOut  = config->digitaliPodOut;
        devinfo->ioctl_config.nativeTransport = config->nativeTransport;
        devinfo->ioctl_config.initEndPoint    = strdup((const char*)config->initEndPoint);
        devinfo->iAP2_ffs_config.initEndPoint = (U8*)strdup((const char*)config->initEndPoint);
        devinfo->ioctl_config.useConfigFS = config->useConfigFS;

        /* get path length for default path */
        lenEpAddr = (S32)strnlen( (const char*)config->initEndPoint,IPOD_IAP2_USBHOST_STRING_MAX);

        /* copy default path e.g. /dev/ffs/ep */
        strncpy(EpAddr, (const char*)config->initEndPoint, lenEpAddr-1);

        rc = iAP2AllocateandAppendString(&devinfo->EndPointSink, EpAddr, '1');
        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo->EndPointSink = %s", devinfo->EndPointSink);

        if(rc == IPOD_DATACOM_SUCCESS)
        {
            rc = iAP2AllocateandAppendString(&devinfo->EndPointSource, EpAddr, '2');
        }
        if(rc == IPOD_DATACOM_SUCCESS)
        {
            devinfo->iAP2_ffs_config.iOSAppCnt = config->iOSAppCnt;
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "iOSAppCnt = %d", devinfo->iAP2_ffs_config.iOSAppCnt);
            if(devinfo->iAP2_ffs_config.iOSAppCnt > 0)
            {
                devinfo->iAP2_ffs_config.nativeTransport = TRUE;
                devinfo->iAP2_ffs_config.iOSAppIdentifier = calloc(devinfo->iAP2_ffs_config.iOSAppCnt, sizeof(U8));
                devinfo->iAP2_ffs_config.iOSAppNames = calloc(devinfo->iAP2_ffs_config.iOSAppCnt, sizeof(U8*));
                if( (devinfo->iAP2_ffs_config.iOSAppNames == NULL) ||
                    (devinfo->iAP2_ffs_config.iOSAppIdentifier == NULL) )
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "Error in allocating memory for iOSAppNames or iOSAppIdentifier");
                    rc = IPOD_DATACOM_ERR_NOMEM;
                }
                else
                {
                    offset = 0;
                    for(i=0; i<devinfo->iAP2_ffs_config.iOSAppCnt; i++)
                    {
                        devinfo->iAP2_ffs_config.iOSAppNames[i] = (U8*)strndup((const char*)&config->iOSAppNames[offset], IPOD_IAP2_USBHOST_STRING_MAX);
                        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "iOSAppNames = %s", devinfo->iAP2_ffs_config.iOSAppNames[i]);
                        len = (S32)strnlen(&config->iOSAppNames[offset], IPOD_IAP2_USBHOST_STRING_MAX);
                        offset += len + 1;
                        devinfo->iAP2_ffs_config.iOSAppIdentifier[i] = config->iOSAppIdentifier[i];
                    }
                }
            }
            if(config->iOSAppCnt > 0)
            {
                devinfo->alternate_interface = calloc(config->iOSAppCnt, sizeof(alternate_interface_t) );
                if(NULL == devinfo->alternate_interface)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo->alternate_interface is NULL");
                    rc = IPOD_DATACOM_ERR_NOMEM;
                }
                else
                {
                    /* - Extended Interface Sets (for apps with support for EA native transport) */
                    for (i = 0; i < config->iOSAppCnt; i++)
                    {
                        devinfo->alternate_interface[i].interfaceNumber  = i + 1;
                        devinfo->alternate_interface[i].inEndpoint       = ( ((i + 2) * 2) - 1);
                        devinfo->alternate_interface[i].outEndpoint      = ( (i + 2) * 2);
                        devinfo->alternate_interface[i].iOSAppIdentifier = config->iOSAppIdentifier[i];
                    }

                }
            }
            devinfo->ioctl_config.iap2Device = config->iap2Device;
            devinfo->ioctl_config.context    = config->context;
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo or config is NULL, devinfo = %p, config = %p", devinfo, config);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

static S32 iAP2USBHostIoCtl(void* iPodHdl, S32 request, void* argp)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_HOSTDEV_INFO *devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if( (devinfo != NULL) && (argp != NULL) )
    {
        switch(request)
        {
            case IPOD_IAP2_DATACOM_IOCTL_SET_CONFIG:
            {
                rc = iAP2USBHostInitIoCtlConfig(devinfo, (IPOD_IAP2_DATACOM_IOCTL_CONFIG*)argp);
                if(IPOD_DATACOM_SUCCESS != rc)
                {
                    IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "iAP2USBHostInitIoCtlConfig() returns, rc = %d", rc);
                    iAP2USBHostFreeIoCtlConfig(devinfo);
                }

                break;
            }

            case IPOD_IAP2_DATACOM_IOCTL_SET_CB:
            {
                memcpy( &(devinfo->EAcallbacks), (IPOD_IAP2_DATACOM_ALTERNATE_IF_CB*)argp, sizeof(IPOD_IAP2_DATACOM_ALTERNATE_IF_CB) );
                break;
            }

            default:
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "request = %d", request);
                rc = IPOD_DATACOM_ERROR;
                break;
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, argp = %p", devinfo, argp);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

void* iPodiAP2USBHostComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function)
{
    IPOD_IAP2_HID_HOSTDEV_INFO* devinfo = NULL;

    data_com_function->ioctl    = &iAP2USBHostIoCtl;
    data_com_function->open     = &iAP2USBHostOpenPlugin;
    data_com_function->read     = &iAP2USBHostReceiveMessage;
    data_com_function->write    = &iAP2USBHostSendtoUSB;
    data_com_function->hdlevent = &iAP2USBHostLibUSBHandleEvent;
    data_com_function->getfds   = &iAP2USBHostLibUSBGetFDs;
    data_com_function->abort    = &iAP2USBHostCloseMsgHandling;
    data_com_function->property = &iAP2USBHostGetProperty;
    data_com_function->close    = &iAP2USBHostClosePlugin;

    devinfo = calloc(1, sizeof(IPOD_IAP2_HID_HOSTDEV_INFO) );
    if (devinfo != NULL)
    {
        devinfo->ioctl_config.digitaliPodOut    = FALSE;
        devinfo->ioctl_config.nativeTransport   = FALSE;
        devinfo->ep0_fd                         = -1;
        devinfo->write_fd                       = -1;
        devinfo->read_fd                        = -1;
        memset(&devinfo->iAP2_ffs_config, 0, sizeof(iAP2_ffs_config_t));
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo = %p", devinfo);

    return devinfo;
}

S32 iPodiAP2USBHostComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function, void* iPodHdl )
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_HOSTDEV_INFO* devinfo = (IPOD_IAP2_HID_HOSTDEV_INFO*)iPodHdl;

    if (data_com_function != NULL)
    {
        memset(data_com_function, 0, sizeof(IPOD_IAP2_DATACOM_FUNC_TABLE) );
    }

    if (devinfo != NULL)
    {
        iAP2USBHostFreeIoCtlConfig(devinfo);

        memset(&devinfo->EAcallbacks, 0, sizeof(IPOD_IAP2_DATACOM_ALTERNATE_IF_CB) );

        iAP2FreePointer( (void**)&devinfo->EndPointSink);
        iAP2FreePointer( (void**)&devinfo->EndPointSource);
        iAP2FreePointer( (void**)&devinfo->alternate_interface);
        iAP2FreePointer( (void**)&devinfo);
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}
