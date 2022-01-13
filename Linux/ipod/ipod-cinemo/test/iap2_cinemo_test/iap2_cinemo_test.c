/************************************************************************
 * \file: iap2_cinemo_test.c
 *
 * \version: $ $
 *
 * \component: ipod-cinemo
 *
 * \author: abirami.murugesan@in.bosch.com
 *
 * \brief This is the implementation of the smoketest for Cinemo Application
 *
 * \The sequence followed : iOS device is switched to Host mode and target to gadget mode, Gadget configuration is done,
 *
 * \cinemo sample App is executed and then the reverse role switch operation is performed.
 *
/*
 * This software has been developed by Advanced Driver Information Technology.
 * Copyright(c) 2019-2020 Advanced Driver Information Technology GmbH,
 * Advanced Driver Information Technology Corporation, Robert Bosch GmbH,
 * Robert Bosch Car Multimedia GmbH and DENSO Corporation.
 * All rights reserved.
 *
 ***********************************************************************/

#include "iap2_cinemo_test.h"

static U8 serialNum[DEV_DETECT_CFG_STRING_MAX];
static U8 UdcDeviceName[DEV_DETECT_CFG_STRING_MAX];

LOCAL inline void iap2FreePtr(void** iap2PtrToFree)
{
    if (*iap2PtrToFree != NULL)
    {
        free(*iap2PtrToFree);
        *iap2PtrToFree = NULL;
    }
}

U32 iap2CurrTimeValToMs(struct timeval* currTime)
{
    return (U32)(currTime->tv_sec * 1000) + (U32)(currTime->tv_usec / 1000);
}

U32 iap2CurrTimeMshost(void)
{
    U32 timeMs;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    timeMs = iap2CurrTimeValToMs(&tp);

    return timeMs;
}

LOCAL void iap2DeInitGadgetConfiguration(iAP2_usbg_config_t* usb_gadget_configuration)
{
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2GadgetName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFS_InstanceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryModelIdentifier);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryManufacturer);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessorySerialNumber);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryVendorId);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryProductId);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryBcdDevice);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2ConfigFS_MountLocation);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2UdcDeviceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2UAC2_InstanceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2_UAC2_Attrs);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFSConfig.initEndPoint);
}

LOCAL S32 iap2InitGadgetConfiguration(iAP2_usbg_config_t* usb_gadget_configuration, U8 DeviceInstance)
{
    S32 rc = IAP2_OK;

    U8  AccessoryName[]                 = {IAP2_ACC_INFO_NAME};
    U8  ModelIdentifier[]               = {IAP2_ACC_INFO_MODEL_IDENTIFIER};
    U8  Manufacturer[]                  = {IAP2_ACC_INFO_MANUFACTURER};
    U8  SerialNumber[]                  = {IAP2_ACC_INFO_SERIAL_NUM};
    U8  FirmwareVersion[]               = {IAP2_ACC_INFO_FW_VER};
    U8  HardwareVersion[]               = {IAP2_ACC_INFO_HW_VER};
    U8  idVendor[]                      = {IAP2_ACC_INFO_VENDOR_ID};
    U8  idProduct[]                     = {IAP2_ACC_INFO_PRODUCT_ID};
    U8  bcdDevice[]                     = {IAP2_ACC_INFO_BCD_DEVICE};
    U8  initEndpoint[]                  = {IAP2_ACC_INFO_EP_INIT};
    U8  CurrentLanguage[]               = {"en"};
    U16 SupportedLanguageCnt            = 3;
    U8  SupportedLanguage[][3]          = {"en", "de", "fr"};
    U8  ProductPlanUUID[]               = {IAP2_ACC_INFO_PPUUID};

    U8 iAP2GadgetName[STRING_MAX - 1]         = {"iAP_Interface_"};
    U8 iAP2FFS_InstanceName[STRING_MAX - 5]   = {"ffs_"};

    (void)snprintf((char*)&iAP2GadgetName[strlen((const char*)iAP2GadgetName)],
                   ( sizeof(iAP2GadgetName) - strlen((const char*)iAP2GadgetName) ),
                   "%d",
                   DeviceInstance);
    (void)snprintf((char*)&iAP2FFS_InstanceName[strlen((const char*)iAP2FFS_InstanceName)],
                   ( sizeof(iAP2FFS_InstanceName) - strlen((const char*)iAP2FFS_InstanceName) ),
                   "%d",
                   DeviceInstance);
    usb_gadget_configuration->iAP2GadgetName                = (U8*)strndup((const char*)iAP2GadgetName, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2FFS_InstanceName          = (U8*)strndup((const char*)iAP2FFS_InstanceName, STRING_MAX - 4);
    usb_gadget_configuration->iAP2AccessoryName             = (U8*)strndup((const char*)AccessoryName, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryModelIdentifier  = (U8*)strndup((const char*)ModelIdentifier, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryManufacturer     = (U8*)strndup((const char*)Manufacturer, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessorySerialNumber     = (U8*)strndup((const char*)SerialNumber, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryVendorId         = (U8*)strndup((const char*)idVendor, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryProductId        = (U8*)strndup((const char*)idProduct, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryBcdDevice        = (U8*)strndup((const char*)bcdDevice, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2ConfigFS_MountLocation    = (U8*)strndup((const char*)IAP2_CONFIGFS_MOUNT_LOCATION, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2UdcDeviceName             = (U8*)strndup((const char*)UdcDeviceName, (STRING_MAX - 1) );

    if ( (usb_gadget_configuration->iAP2GadgetName == NULL)                  ||
        (usb_gadget_configuration->iAP2FFS_InstanceName == NULL)            ||
        (usb_gadget_configuration->iAP2AccessoryName == NULL)               ||
        (usb_gadget_configuration->iAP2AccessoryModelIdentifier == NULL)    ||
        (usb_gadget_configuration->iAP2AccessoryManufacturer == NULL)       ||
        (usb_gadget_configuration->iAP2AccessorySerialNumber == NULL)       ||
        (usb_gadget_configuration->iAP2AccessoryVendorId == NULL)           ||
        (usb_gadget_configuration->iAP2AccessoryProductId == NULL)          ||
        (usb_gadget_configuration->iAP2AccessoryBcdDevice == NULL)          ||
        (usb_gadget_configuration->iAP2ConfigFS_MountLocation == NULL)      ||
        (usb_gadget_configuration->iAP2UdcDeviceName == NULL) )
    {
        rc = IAP2_ERR_NO_MEM;
        printf("\n %u ms  iap2InitGadgetConfiguration() - Not Enough Memory \n", iap2CurrTimeMshost());
    }
    if (rc == IAP2_OK)
    {
        U8 iAP2UAC2_InstanceName[STRING_MAX - 6]  = {"uac2_"};

        (void)snprintf((char*)&iAP2UAC2_InstanceName[strlen((const char*)iAP2UAC2_InstanceName)],
                           ( sizeof(iAP2UAC2_InstanceName) - strlen((const char*)iAP2UAC2_InstanceName) ),
                           "%d",
                           DeviceInstance);
        usb_gadget_configuration->iAP2UAC2_InstanceName = (U8*)strndup((const char*)iAP2UAC2_InstanceName, STRING_MAX - 5);
        usb_gadget_configuration->iAP2_UAC2_Attrs = calloc(1, sizeof(usbg_f_uac2_attrs) );
        if ( (usb_gadget_configuration->iAP2_UAC2_Attrs == NULL) ||
            (usb_gadget_configuration->iAP2UAC2_InstanceName == NULL) )
        {
            rc = IAP2_ERR_NO_MEM;
            printf("\n %u ms  iap2InitGadgetConfiguration() - Not Enough Memory \n", iap2CurrTimeMshost());
        }
        else
        {
            usb_gadget_configuration->iAP2_UAC2_Attrs->c_chmask     = 3;
            usb_gadget_configuration->iAP2_UAC2_Attrs->c_srate_def  = 44100;
            usb_gadget_configuration->iAP2_UAC2_Attrs->c_ssize      = 2;
            usb_gadget_configuration->iAP2_UAC2_Attrs->delay_tout   = 80;
            usb_gadget_configuration->iAP2_UAC2_Attrs->p_chmask     = 0;
            usb_gadget_configuration->iAP2_UAC2_Attrs->p_srate_def  = 44100;
            usb_gadget_configuration->iAP2_UAC2_Attrs->p_ssize      = 2;
            usb_gadget_configuration->iAP2_UAC2_Attrs->c_srate      = c_srate;
            usb_gadget_configuration->iAP2_UAC2_Attrs->p_srate      = p_srate;
        }
    }
    printf("\n %u ms  iap2InitGadgetConfiguration() - returns rc = %d \n", iap2CurrTimeMshost(), rc);
    return rc;
}

S32 iap2CheckForDevice(struct udev *udev)
{
    S32 ret = -1;

    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    const char *idVendor = NULL;
    const char *idProduct = NULL;
    const char *serial = NULL;
    const char* product = NULL;

    int len = 0;

    if (udev != NULL)
    {
        /* Create a list of the devices in the 'hidraw' subsystem. */
        enumerate = udev_enumerate_new(udev);
        if (enumerate == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate is NULL");
            exit(1);
        }
        ret = udev_enumerate_add_match_subsystem(enumerate, (const char *)IPOD_USB_MONITOR_DEVTYPE);
        if (ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_subsystem failed: ret = %d", ret);
            exit(1);
        }
        /* search only for Apple devices */
        ret = udev_enumerate_add_match_sysattr(enumerate, IPOD_SYSATTR_IDVENDOR, IPOD_APPLE_VENDORID);
        if (ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_sysattr failed: ret = %d", ret);
            exit(1);
        }
        ret = udev_enumerate_scan_devices(enumerate);
        if (ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_scan_devices failed: ret = %d", ret);
            exit(1);
        }
        devices = udev_enumerate_get_list_entry(enumerate);
        if (devices == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "devices is NULL (list_entry)");
            /* no specified Apple device available */
            ret = -1;
        }
        else
        {
            /* For each item enumerated, print out its information.
               udev_list_entry_foreach is a macro which expands to
               a loop. The loop will be executed for each member in
               devices, setting dev_list_entry to a list entry
               which contains the device's path in /sys. */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path;

                /* Get the filename of the /sys entry for the device
                   and create a udev_device object (dev) representing it */
                path = udev_list_entry_get_name(dev_list_entry);
                if (path == NULL)
                {
                    /* Get the path of device failed*/
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "path of device failed");
                }
                else
                {
                    dev = udev_device_new_from_syspath(udev, path);
                    if (dev != NULL)
                    {
                        /* usb_device_get_devnode() returns the path to the device node
                           itself in /dev. */
//                         printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

                        /* From here, we can call get_sysattr_value() for each file
                           in the device's /sys entry. The strings passed into these
                           functions (idProduct, idVendor, serial, etc.) correspond
                           directly to the files in the directory which represents
                           the USB device. Note that USB strings are Unicode, UCS2
                           encoded, but the strings returned from
                           udev_device_get_sysattr_value() are UTF-8 encoded. */
                        idVendor = udev_device_get_sysattr_value(dev,"idVendor");
                        idProduct = udev_device_get_sysattr_value(dev, "idProduct");
                        serial = udev_device_get_sysattr_value(dev, "serial");
                        product = udev_device_get_sysattr_value(dev, "product");

//                        printf("VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, serial);

                        if ( (idVendor != NULL) && (idProduct != NULL) && (serial != NULL) && (product != NULL) )
                        {
                            /* Vendor ID equal to Apple Vendor ID and Product ID equal to Apple Product ID */
                            if ((strncmp((const char *)idVendor, (const char *)IPOD_APPLE_VENDORID, DEV_DETECT_VENDOR_MAX_LEN) == 0) &&
                               (strncmp((const char *)idProduct, (const char *)IPOD_APPLE_IDPRODUCT_MIN, 2) == 0))
                            {
                                printf(" %s found %s with serial-nr: %s\n", __FUNCTION__, product, serial);

                                len = strnlen(serial,DEV_DETECT_CFG_STRING_MAX);
                                memcpy(serialNum,serial, len);
                                serialNum[len] = '\0';

                                len = strnlen(path,DEV_DETECT_CFG_STRING_MAX);

                                if (ret == 0)
                                {
                                    IAP2TESTDLTLOG(DLT_LOG_INFO, "Device details: VID: %s  PID: %s SERIAL %s ", idVendor, idProduct, serialNum );
                                    ++ret;
                                }
                            }
                        }
                        else
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_get_sysattr_value(idVendor or idProduct or serial) is NULL");
                        }

                        udev_device_unref(dev);
                    }
                    else
                    {
                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_new_from_syspath failed");
                    }
                }
            }
        }
        /* Free the enumerator object */
        udev_enumerate_unref(enumerate);
    }

    return ret;
}

/* return the number of detected Apple devices */
S32 iap2DetectDevice(void)
{
    S32 ret = -1;
    int retryCount = 0;

    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *monitor;
    S32 udevfd = -1;
    const char *action = NULL;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Can't create udev");
        exit(1);
    }
    else
    {
        /* check if iPod is already connected */
        ret = iap2CheckForDevice(udev);
        /* no iPod available. wait for connecting iPod */
        if (ret <= 0)
        {
            /* Link the monitor to "udev" devices */
            monitor = udev_monitor_new_from_netlink(udev, (const char *)IPOD_USB_MONITOR_LINK);
            if (monitor != NULL)
            {
                /* Filter only for get the "hiddev" devices */
                ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, (const char *)IPOD_USB_MONITOR_DEVTYPE, IPOD_USB_FILTER_TYPE);
                if (ret == 0)
                {
                    /* Start receiving */
                    ret = udev_monitor_enable_receiving(monitor);
                    if (ret == 0)
                    {
                        /* Get the file descriptor for the monitor. This fd is used by select() */
                        udevfd = udev_monitor_get_fd(monitor);
                        ret = -1;
                        IAP2TESTDLTLOG(DLT_LOG_WARN, "Please connect iPod!");
                        while ((ret == -1) && (retryCount < IPOD_USB_SELECT_RETRY_CNT))
                        {
                            retryCount++;

                            fd_set fds;
                            struct timeval tv;

                            FD_ZERO(&fds);
                            FD_SET(udevfd, &fds);
                            tv.tv_sec = 5;
                            tv.tv_usec = 0;

                            ret = select(udevfd+1, &fds, NULL, NULL, &tv);
                            /* Check if our file descriptor has received data. */
                            if (ret > 0 && FD_ISSET(udevfd, &fds))
                            {
                                /* Make the call to receive the device.
                                   select() ensured that this will not block. */
                                dev = udev_monitor_receive_device(monitor);
                                if (dev)
                                {
                                    action = udev_device_get_action(dev);
                                    if (strncmp((const char *)action, IPOD_USB_ACTION_ADD, 4) == 0)
                                    {
                                        ret = 0;
                                        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Action is %s -> ret = %d", action, ret);
                                    }
                                    else
                                    {
                                        ret = -1;
                                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Action is %s -> ret = %d", action, ret);
                                    }

                                    udev_device_unref(dev);
                                }
                                else
                                {
                                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod ");
                                }
                            }
                            else if (ret < 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: select() failed");
                            }
                            else
                            {
                                /* this occurs if an iPod is already connected */
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod found");
                                ret = -1;
                            }
                            IAP2TESTDLTLOG(DLT_LOG_WARN, "retry %d", retryCount);
                        }

                        if (ret == -1)
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: max retries %d | ret = %d",  retryCount, ret);
                        }
                        else /* IPOD_OK */
                        {
                            /* get Serial-Nr. VendorID and ProductID of newly connected iPod */
                            ret = iap2CheckForDevice(udev);
                            /* no iPod available. wait for connecting iPod */
                            if (ret <= 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: enumerate connected iPod failed | ret = %d", ret);
                            }
                        }
                    }
                }
                udev_monitor_unref(monitor);
            }
            else
            {
                IAP2TESTDLTLOG(DLT_LOG_ERROR, "monitor NULL");
                exit(1);
            }
        }
    }

    udev_unref(udev);

    return ret;
}

/***************************************************************************//**
 * Performs Cinemo Sample test execution in Host mode
 *
 * \Pre-requisites :
 *
 * \The Cinemo sdk should have been installed in the path : /home/root/
 *
 * \The file cinemo_options.xml should contain the required changes to use ADIT specific libraries rather than Cinemo's libraries.
 *
 * \The following operations are done :
 *
 * \Setup the environment variables required for Cinemo Engine
 *
 * \Change the role of Apple phone to Host and that of target to gadget
 *
 * \Create the gadgets required for iAP2 Host mode operation
 *
 * \Execute the Cinemo App
 *
 * \Reverse the roles of target and Apple device
 *
 ******************************************************************************/

S32 main(int argc, const char** argv)
{
    (void)argc;
    (void)argv;

    S32 rc = IAP2_OK, res = IAP2_OK;
    iAP2_usbg_config_t usb_gadget_configuration1 = {0};
    iAP2USBRoleSwitchInfo info;
    udcParamInfo_t udcParam;

    memset(&info, 0, sizeof(info));
    memset(&udcParam, 0, sizeof(udcParamInfo_t));

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

    rc = setenv("CINEMO_PLUGIN_INIT", CINEMO_PLUGIN_INIT_PATH, 1);
    if (rc == IAP2_OK)
    {
        rc = setenv("LD_LIBRARY_PATH", CINEMO_LIBRARY_PATH, 1);
        if (rc == IAP2_OK)
        {
            printf("\n %u ms CINEMO_PLUGIN_INIT %s", iap2CurrTimeMshost(), getenv("CINEMO_PLUGIN_INIT"));
            printf("\n %u ms LD_LIBRARY_PATH %s", iap2CurrTimeMshost(), getenv("LD_LIBRARY_PATH"));
        }
        else
        {
            printf("\n %u ms Set Environment variable function failed", iap2CurrTimeMshost());
        }
    }
    else
    {
        printf("\n %u ms Set Environment variable function failed", iap2CurrTimeMshost());
    }

    rc = iap2FindVbus(&info);
    IAP2TESTDLTLOG(DLT_LOG_INFO, "iap2FindVbus  = %d | %s\n", rc, info.vbusPower );
    printf(" %u ms  iap2FindVbus  = %d | %s \n", iap2CurrTimeMshost(), rc, info.vbusPower);
    if (rc != IAP2_OK)
    {
        IAP2TESTDLTLOG(DLT_LOG_INFO, "No vbus available");
        rc = IAP2_OK;
    }

    rc = iap2DetectDevice();
    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Number of Apple device %d",rc);

    if (rc < 0)
    {
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, " Apple device not found %d",rc);
        return rc;
    }

    info.otgGlob = iap2GetSwtichOtgGlobPath();
    info.mode = IAP2_USB_ROLE_SWITCH_WITHOUT_DIGITAL_IPOD_OUT;
    info.productId = IPOD_APPLE_IDPRODUCT;
    info.vendorId = IPOD_APPLE_IDVENDOR;
    info.serialNumber = (const char*)strndup(serialNum, strlen(serialNum));

    if (NULL == info.serialNumber)
    {
        IAP2TESTDLTLOG(DLT_LOG_WARN, "Memory allocation failed for Serial Number");
    }

    rc = iap2SwitchToHostMode(&info, &udcParam);

    if (udcParam.pUdcDevice != NULL)
    {
        int len = strnlen(udcParam.pUdcDevice,DEV_DETECT_CFG_STRING_MAX);
        memcpy(UdcDeviceName,udcParam.pUdcDevice, len);
        if (len < DEV_DETECT_CFG_STRING_MAX)
        {
            UdcDeviceName[len] = '\0';
        }
        else
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "Length of pUdcDevice %d %s", len, UdcDeviceName);
        }
    }

    if (rc == IAP2_OK)
    {
        if (iap2IsKernel314() || iap2IsKernel4xx())
        {
            if (IAP2_OK != iap2LoadModule(NULL, CONFIGFS_MODULE_NAME, strlen(CONFIGFS_MODULE_NAME)))
            {
                rc = IAP2_CTL_ERROR;
            }
        }
        if (rc == IAP2_OK)
        {
            res = mount("config", "/sys/kernel/config", "configfs", MS_NOEXEC, NULL);
            if (res != 0)
            {
                rc = IAP2_CTL_ERROR;
            }
            printf(" %u ms  mount() res = %d\n", iap2CurrTimeMshost(), res);

            res = chmod("/sys/kernel/config", 0750);
            if (res != 0)
            {
                rc = IAP2_CTL_ERROR;
            }
            printf(" %u ms  chmod() res = %d\n", iap2CurrTimeMshost(), res);

            if (IAP2_OK != iap2LoadModule(NULL, LIBCOMPOSITE_MODULE_NAME, strlen(LIBCOMPOSITE_MODULE_NAME)))
            {
                rc = IAP2_CTL_ERROR;
            }
        }
    }

    if (rc == IAP2_OK)
    {
        rc = iap2InitGadgetConfiguration(&usb_gadget_configuration1,1);
        printf(" %u ms  iap2InitGadgetConfiguration = %d\n", iap2CurrTimeMshost(), rc);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "iap2InitGadgetConfiguration rc = %d ", rc);
    }

    if (rc == IAP2_OK)
    {
        rc = iAP2InitializeGadget(&usb_gadget_configuration1);
        printf(" %u ms  iAP2InitializeGadget = %d\n", iap2CurrTimeMshost(), rc);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "iAP2InitializeGadget rc = %d ", rc);
    }

    if (rc  == IAP2_OK)
    {
        rc = mkdir("/dev/ffs", 0750);
        if ( (rc != 0) && (rc != EEXIST) )
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("mkdir(/dev/ffs)  rc = %d\n", rc);
    }
    if (rc  == IAP2_OK)
    {
        char options[30] = {0};
        snprintf(options, sizeof(options), "uid=%d,gid=%d",0,1);
        rc = mount("ffs_1", "/dev/ffs", "functionfs", MS_NOEXEC, options);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("mount(ffs_1)  rc = %d, errno=%d, %s\n", rc, errno, strerror(errno));
    }

    if (rc  == IAP2_OK)
    {
        /* after mount we need to set the mode again */
        rc = chmod("/dev/ffs", 0777);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("chmod(/dev/ffs)  rc = %d\n", rc);
    }

    if (rc  == IAP2_OK)
    {
        /* after mount we need to set the mode again */
        rc = chmod("/dev/ffs/ep0", 0777);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("chmod(/dev/ffs/ep0)  rc = %d\n", rc);
    }

    if (rc  == IAP2_OK)
    {
        U8 initEndPoint[] = {"/dev/ffs/ep0"};

        usb_gadget_configuration1.iAP2FFSConfig.initEndPoint = (U8*)strdup((const char*)initEndPoint);
        rc = iAP2InitializeFFSGadget(&usb_gadget_configuration1);
        printf("iAP2ConfigureFFSGadget rc = %d\n", rc);
    }

    if (rc == IAP2_OK)
    {
        rc = system("modprobe i2c-dev");
        rc = system("~/sdk/usr/bin/SampleIAP -d -u iap://ADITIAP:/dev/ffs/:ADITAUD:hw:UAC2Gadget,0");

        rc = iAP2DeInitializeGadget();
        printf(" %u ms  iAP2DeInitializeGadget:   rc  = %d \n", iap2CurrTimeMshost(), rc);
        iap2DeInitGadgetConfiguration(&usb_gadget_configuration1);
        printf(" %u ms  iAP2DeInitialized Gadget1\n", iap2CurrTimeMshost() );

        umount("/sys/kernel/config");
        rmdir("/sys/kernel/config");
    }

    if (rc == IAP2_OK)
    {
        if ( (iap2IsKernel314() || iap2IsKernel4xx()))
        {
            iap2UnloadModule(USB_F_UAC2);
            if (iap2IsKernel414() == TRUE)
            {
                iap2UnloadModule(U_AUDIO);
            }
        }

        umount(FUNCTION_FS_PATH);
        rmdir(FUNCTION_FS_PATH);

        if (iap2IsKernel314() || iap2IsKernel4xx())
        {
        iap2UnloadModule(USB_F_FS_MODULE_NAME);
        }
        iap2UnloadModule(LIBCOMPOSITE_MODULE_NAME);
        if (iap2IsKernel314() || iap2IsKernel4xx())
        {
            iap2UnloadModule(CONFIGFS_MODULE_NAME);
        }

        rc = iap2SwitchToDeviceMode(&info, &udcParam);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : Switch USB OTG to Host  = %d", rc );

        if (NULL != info.vbusPower)
        {
            free(info.vbusPower);
        }
    }

    if (NULL != info.serialNumber)
    {
        free(info.serialNumber);
    }

    memset(&info, 0, sizeof(info));
    memset(&udcParam, 0, sizeof(udcParamInfo_t));

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return rc;
}
