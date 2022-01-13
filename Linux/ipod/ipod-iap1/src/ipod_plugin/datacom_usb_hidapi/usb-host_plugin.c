
#include "iPodDataCom.h"
#include "iPodDataCom_i.h"
#include "iap1_dlt_log.h"

LOCAL S32 iPodUSBHostGetFreeInstance(const U8* devicename);

S32 g_num_devices = 0;
IPOD_HID_DEVINFO *g_iPodUSBHostInst = NULL;

S32 iPodUSBHostComInit(IPOD_DATACOM_FUNC_TABLE* data_com_function, U32 num_devices)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    U32 i = 0;

    data_com_function->open = &iPodUSBHostOpenPlugin;
    data_com_function->close = &iPodUSBHostClosePlugin;
    data_com_function->abort = &iPodCloseMsgHandling;
    data_com_function->write = &iPodSendtoUSB;
    data_com_function->read = &iPodReceiveMessage;
    data_com_function->ioctl = NULL;
    data_com_function->property = &iPodUSBGetProperty;

    g_iPodUSBHostInst = calloc(num_devices, sizeof(IPOD_HID_DEVINFO));
    if (g_iPodUSBHostInst != NULL)
    {
        g_num_devices = num_devices;
        for (i = 0; i < num_devices; i++)
        {
            (void)iPodHIDInit(&g_iPodUSBHostInst[i]);
        }
        rc = iPodHIDTrnspInit(num_devices);
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "iPodHIDTrnspInit() returns rc = %d",rc);
    }
    else
    {
        rc = IPOD_DATACOM_ERR_NOMEM;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iPod Datacom error g_iPodUSBHostInst is NULL");
    }

    return rc;
}

S32 iPodUSBHostComDeinit(IPOD_DATACOM_FUNC_TABLE* data_com_function)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    iPodHIDTrnspDeInit();

    if (data_com_function != NULL)
    {
        data_com_function->open = NULL;
        data_com_function->close = NULL;
        data_com_function->abort = NULL;
        data_com_function->write = NULL;
        data_com_function->read = NULL;
        data_com_function->ioctl = NULL;
        data_com_function->property = NULL;
    }

    if (g_iPodUSBHostInst != NULL)
    {
        free(g_iPodUSBHostInst);
        g_iPodUSBHostInst = NULL;
    }

    return rc;
}

S32 iPodUSBHostOpenPlugin(const U8* device_name, S32 flags, S32 mode)
{
    S32 fd = IPOD_DATACOM_ERROR;
    S32 rc = IPOD_DATACOM_SUCCESS;

    /* for compiler warnings */
    flags = flags;

    if (device_name != NULL)
    {
        if (device_name[0] != '\0')
        {
            fd = iPodUSBHostGetFreeInstance(device_name);
            IAP1_USBP_LOG(DLT_LOG_VERBOSE, "iPodUSBHostGetFreeInstance() returns fd = %d",fd);
        }
    }

    if (fd >= 0)
    {
        rc = iPodHIDOpen(&g_iPodUSBHostInst[fd], device_name, mode);
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "iPodHIDOpen() returns rc = %d",rc);

        if (rc == IPOD_DATACOM_SUCCESS)
        {
            rc = iPodUSBGetReportId_internal(&g_iPodUSBHostInst[fd]);
            if(rc != IPOD_DATACOM_SUCCESS)
            {
                /* Close the opened hid device */
                iPodHIDClose(&g_iPodUSBHostInst[fd]);
                /* Clear the Instance */
                memset(&g_iPodUSBHostInst[fd], 0, sizeof(g_iPodUSBHostInst[fd]));
            }
        }
        else
        {
            /* Clear the Instance */
            memset(&g_iPodUSBHostInst[fd], 0, sizeof(g_iPodUSBHostInst[fd]));
        }
    }
    if (rc == IPOD_DATACOM_SUCCESS)
    {
        rc = fd;
    }
    return rc;
}

S32 iPodUSBHostClosePlugin(S32 fd)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_HID_DEVINFO *devinfo = NULL;

    if ((g_iPodUSBHostInst != NULL)&&(fd >= 0)&&(fd < g_num_devices))
    {
        devinfo = &g_iPodUSBHostInst[fd];
    }

    if (devinfo != NULL)
    {
        iPodHIDDeInit(devinfo);
        memset(&g_iPodUSBHostInst[fd], 0, sizeof(g_iPodUSBHostInst[fd]));
    }

    return rc;
}

S32 iPodCloseMsgHandling(S32 fd)
{
    S32 err = IPOD_DATACOM_ERROR;
    IPOD_HID_DEVINFO *devinfo = NULL;

    if ((g_iPodUSBHostInst != NULL)&&(fd >= 0)&&(fd < g_num_devices))
    {
        devinfo = &g_iPodUSBHostInst[fd];
    }

    if (devinfo != NULL)
    {
        err = iPodHIDClose(devinfo);
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "iPodHIDClose() returns err = %d",err);
    }

    return err;
}

S32 iPodUSBGetProperty(S32 fd, IPOD_DATACOM_PROPERTY *property)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_HID_DEVINFO *devinfo = NULL;

    if ((g_iPodUSBHostInst != NULL)&&(fd >= 0)&&(fd < g_num_devices) && (property != NULL))
    {
        devinfo = &g_iPodUSBHostInst[fd];
        rc = iPodUSBGetReportLenMax(devinfo, &property->maxSize);
    }
    
    return rc;
}

S32 iPodSendtoUSB(S32 fd, U32 msgLenTotal, const U8 *iPod_msg, S32 flags)
{
    S32  rc                 = IPOD_DATACOM_SUCCESS;
    IPOD_HID_DEVINFO *devinfo = NULL;

    /* for compiler warnings */
    flags = flags;

    if ((g_iPodUSBHostInst != NULL)&&(fd >= 0)&&(fd < g_num_devices))
    {
        devinfo = &g_iPodUSBHostInst[fd];
    }
    
    if ((devinfo != NULL)&&(iPod_msg != NULL)&&(msgLenTotal > 0))
    {
        rc = iPodHIDWriteReport(devinfo, msgLenTotal, iPod_msg);
    }
    else  /* iPodHndl == NULL or buf == NULL or sendInfo == NULL */
    {
        rc = IPOD_DATACOM_BAD_PARAMETER;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "devinfo = %p iPod_msg = %p msgLenTotal = %d", devinfo,iPod_msg,msgLenTotal);
    }

    return rc;
}

S32 iPodReceiveMessage(S32 fd, U32 buffer_size, U8 *msgBuffer, S32 flags)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    IPOD_HID_DEVINFO *devinfo = NULL;

    /* for compiler warnings */
    flags = flags;
    if ((g_iPodUSBHostInst != NULL)&&(fd >= 0)&&(fd < g_num_devices))
    {
        devinfo = &g_iPodUSBHostInst[fd];
    }

    if((devinfo != NULL) && (msgBuffer != NULL))
    {
        rc = iPodHIDWaitEvent(devinfo, buffer_size, msgBuffer);
    }
    return rc;
}

LOCAL S32 iPodUSBHostGetFreeInstance(const U8* devicename)
{
    S32 rc = IPOD_DATACOM_SUCCESS;
    S32 i  = 0;
    S32 emptyField = IPOD_DATACOM_ERROR;

    if ((NULL != g_iPodUSBHostInst)&&(devicename != NULL))
    {
        for (i = 0; (i < g_num_devices) && (rc == IPOD_DATACOM_SUCCESS); i++)
        {
            if ((g_iPodUSBHostInst[i].name[0] == '\0')&&(emptyField == IPOD_DATACOM_ERROR))
            {
                emptyField = i;
            }
            else
            {
                if (strncmp((VP)g_iPodUSBHostInst[i].name, (VP)devicename, sizeof(g_iPodUSBHostInst[i].name)) == 0)
                {
                    rc = IPOD_DATACOM_ALREADY_CONNECTED;
                    IAP1_USBP_LOG(DLT_LOG_DEBUG, "Device is already connected");
                }
            }
        }
    }

    /* There is an empty field and this device was not initialized yet */
    if((emptyField != IPOD_DATACOM_ERROR)&&(rc == IPOD_DATACOM_SUCCESS) && (devicename != NULL))
    {
        IPOD_HID_DEVINFO* devinfo = &g_iPodUSBHostInst[emptyField];

        memset(devinfo, 0, sizeof(*devinfo));
        strncpy((VP)devinfo->name, (VP)devicename, sizeof(devinfo->name) - 1);

        rc = emptyField;
    }
    /* The device was not initialized yet but there is no free field left */
    else if ((emptyField == IPOD_DATACOM_ERROR)&&(rc == IPOD_DATACOM_SUCCESS))
    {
        rc = IPOD_DATACOM_ERR_NOMEM;
        IAP1_USBP_LOG(DLT_LOG_ERROR, "The device was not initialized yet but there is no free field left");
    }
    /* else device is already initialized and rc is IPOD_ALREADY_CONNECTED */

    return rc;
}


