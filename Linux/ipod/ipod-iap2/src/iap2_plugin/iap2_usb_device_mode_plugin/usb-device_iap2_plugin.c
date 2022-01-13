
#include "iap2_dlt_log.h"
#include "iap2_datacom.h"
#include "iap2_datacom_i.h"


void* iPodiAP2USBDeviceComInit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function)
{

    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_DEV_INFO* devinfo = NULL;

    data_com_function->hdlevent = &iPodiAP2USBDeviceLibUSBHandleEvent;
    data_com_function->getfds = &iPodiAP2USBDeviceLibUSBGetFDs;
    data_com_function->open = &iPodiAP2USBDeviceOpenPlugin;
    data_com_function->close = &iPodiAP2USBDeviceClosePlugin;
    data_com_function->abort = &iPodiAP2CloseMsgHandling;
    data_com_function->write = &iPodiAP2SendtoUSB;
    data_com_function->read = &iPodiAP2ReceiveMessage;
    data_com_function->ioctl = iPodiAP2USBDeviceIoCtl;
    data_com_function->property = &iPodiAP2USBGetProperty;

    /* This can be moved to open */
    devinfo = calloc(1,sizeof(IPOD_IAP2_HID_DEV_INFO));
    if (devinfo != NULL)
    {
        (void)iPodiAP2HIDInit(devinfo);
        rc = iPodiAP2HIDTrnspInit(devinfo);
    }
    else
    {
        rc = IPOD_DATACOM_ERR_NOMEM;
    }
    if(rc != IPOD_DATACOM_SUCCESS)
    {
        free(devinfo);
        devinfo = NULL;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "devinfo = %p", devinfo);

    return devinfo;
}

S32 iPodiAP2USBDeviceComDeinit(IPOD_IAP2_DATACOM_FUNC_TABLE* data_com_function,
                           void* iPodHdl)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_IAP2_HID_DEV_INFO* devinfo = (IPOD_IAP2_HID_DEV_INFO*)iPodHdl;

    if(NULL != devinfo)
    {
        rc = iPodiAP2HIDTrnspDeInit(devinfo);

        if (data_com_function != NULL)
        {
            data_com_function->hdlevent = NULL;
            data_com_function->getfds = NULL;
            data_com_function->open = NULL;
            data_com_function->close = NULL;
            data_com_function->abort = NULL;
            data_com_function->write = NULL;
            data_com_function->read = NULL;
            data_com_function->ioctl = NULL;
            data_com_function->property = NULL;
        }

        memset(devinfo, 0, sizeof(*devinfo));
        free(devinfo);
        devinfo = NULL;

        IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p", devinfo);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}


S32 iPodiAP2USBDeviceLibUSBHandleEvent(void* devinfo, U32 buffer_size, U8 *msgBuffer, S32 pollFD)
{
    S32 rc = IPOD_DATACOM_ERROR;
    /* for compiler warnings */

    if((devinfo != NULL) && (msgBuffer != NULL))
    {
        rc = iPodiAP2HIDLibUsbHandleEvent(devinfo, buffer_size, msgBuffer,pollFD);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}


S32 iPodiAP2USBDeviceLibUSBGetFDs(void* devinfo,
                              IPOD_IAP2_DATACOM_FD* getFDs,
                              S32* fd_count)
{
    S32 rc = IPOD_DATACOM_ERROR;

    if (devinfo != NULL)
    {
        rc = iPodiAP2HIDLibUSBGetFDs(devinfo, (iap2_hid_poll_fd*)getFDs, fd_count);
        if( (fd_count != NULL) && (getFDs != NULL) )
        {
            S32 i;

            for(i = 0; i < *fd_count; i++)
            {
                IAP2USBPLUGINDLTLOG(DLT_LOG_INFO, "getFDs[%d].fd = %d, getFDs[%d].event = %d", i, getFDs[i].fd, i, getFDs[i].event);
            }
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

S32 iPodiAP2USBDeviceOpenPlugin(void* iPodHdl,const U8* device_name, S32 flags, S32 mode)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    flags = flags;
    IPOD_IAP2_HID_DEV_INFO* devinfo = (IPOD_IAP2_HID_DEV_INFO*)iPodHdl;

    /* Initialize the device instance */
    if(NULL != devinfo && NULL != device_name)
    {
        if (devinfo->name[0] == '\0')
        {
            memset(devinfo->name, 0, sizeof(devinfo->name));
            strncpy((VP)devinfo->name, (VP)device_name, sizeof(devinfo->name) - 1);
        }
        else
        {
            if (strncmp((VP)devinfo->name, (VP)device_name, sizeof(devinfo->name)) == 0)
            {
                rc = IPOD_DATACOM_ALREADY_CONNECTED;
            }
        }


        /* else device is already initialized and rc is IPOD_ALREADY_CONNECTED */

        if (rc == IPOD_DATACOM_SUCCESS)
        {
            rc = iPodiAP2HIDOpen(devinfo, device_name, mode);
        }
        if (rc == IPOD_DATACOM_SUCCESS)
        {
            rc = iPodiAP2USBGetReportId_internal(devinfo);
            if(rc != IPOD_DATACOM_SUCCESS)
            {
                /* Close the opened hid device */
                iPodiAP2HIDClose(devinfo);
                /* Clear the Instance */
                memset(devinfo, 0, sizeof(IPOD_IAP2_HID_DEV_INFO));
            }
        }
        else
        {
            /* Clear the Instance */
            memset(devinfo, 0, sizeof(IPOD_IAP2_HID_DEV_INFO));
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, device_name = %p", devinfo, device_name);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

S32 iPodiAP2USBDeviceClosePlugin(void *devinfo)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if (devinfo != NULL)
    {
        iPodiAP2HIDDeInit(devinfo);
        devinfo = NULL;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

S32 iPodiAP2CloseMsgHandling(void *devinfo)
{
    S32 rc = IPOD_DATACOM_ERROR;

    if (devinfo != NULL)
    {
        rc = iPodiAP2HIDClose(devinfo);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo is NULL");
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }
    IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);

    return rc;
}

S32 iPodiAP2USBGetProperty(void *devinfo, IPOD_IAP2_DATACOM_PROPERTY *property)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if ((devinfo != NULL) && (property != NULL))
    {
        rc = iPodiAP2USBGetReportLenMax(devinfo, &property->maxSize);
        if (IPOD_DATACOM_SUCCESS != rc){
            IAP2USBPLUGINDLTLOG(DLT_LOG_DEBUG, "rc = %d", rc);
        }
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, property = %p", devinfo, property);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

S32 iPodiAP2USBDeviceIoCtl(void* devinfo, S32 request, void* argp)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    request = request;

    if ((devinfo != NULL) && (argp != NULL))
    {
        /* no I/O CTL in USB Device Mode Plug-in */
        rc = IPOD_DATACOM_SUCCESS;
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, argp = %p", devinfo, argp);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

S32 iPodiAP2SendtoUSB(void *devinfo, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32  rc                 = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    flags = flags;

    if ((devinfo != NULL)&&(iPod_msg != NULL)&&(msgLenTotal > 0))
    {
        rc = iPodiAP2HIDWriteReport(devinfo, msgLenTotal, iPod_msg);
    }
    else  /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, iPod_msg = %p, msgLenTotal = %d", devinfo, iPod_msg, msgLenTotal);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}

S32 iPodiAP2ReceiveMessage(void *devinfo, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    flags = flags;

    if((devinfo != NULL) && (msgBuffer != NULL))
    {
        rc = iPodiAP2HIDWaitEvent(devinfo, buffer_size, msgBuffer);
    }
    else
    {
        IAP2USBPLUGINDLTLOG(DLT_LOG_ERROR, "devinfo = %p, msgBuffer = %p", devinfo, msgBuffer);
        rc = IPOD_DATACOM_BAD_PARAMETER;
    }

    return rc;
}
