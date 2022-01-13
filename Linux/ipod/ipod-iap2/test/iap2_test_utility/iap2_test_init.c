#include "iap2_test_init.h"
#include "iap2_test_gstreamer.h"
#include "iap2_test_utility.h"
#include <sys/poll.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include "iap2_dlt_log.h"


S32 g_rc = IAP2_OK;
int g_Quit = 0;
iap2TestAppleDevice_t *gp_iap2TestDevice;
BOOL g_EAtesting = FALSE;

void SetGlobPtr(iap2TestAppleDevice_t *iap2TestAppleDevice)
{
    gp_iap2TestDevice = iap2TestAppleDevice;
}
void iap2SetTestStateError(BOOL value)
{
    gp_iap2TestDevice->testStates[ERROR] = value;
}
BOOL iap2GetTestStateError(void)
{
    return gp_iap2TestDevice->testStates[ERROR];
}
void iap2SetTestState(iap2TestStates_t setState, BOOL value)
{
    gp_iap2TestDevice->testStates[setState] = value;
}
BOOL iap2GetTestState(iap2TestStates_t getState)
{
    return gp_iap2TestDevice->testStates[getState];
}

void iap2SetTestDeviceState(iAP2DeviceState_t testDeviceState)
{
    gp_iap2TestDevice->testDeviceState = testDeviceState;
}
iAP2DeviceState_t iap2GetTestDeviceState(void)
{
    return gp_iap2TestDevice->testDeviceState;
}
void iap2SetiOS8testing(BOOL value)
{
    gp_iap2TestDevice->iap2TestiOS8 = value;
}
BOOL iap2GetiOS8testing(void)
{
    return gp_iap2TestDevice->iap2TestiOS8;
}
void iap2SetGlobalQuit(BOOL value)
{
    g_Quit = value;
}
BOOL iap2GetGlobalQuit(void)
{
    return g_Quit;
}
usbConnectStateType_t iap2ConnectionType(void)
{
    return gp_iap2TestDevice->type;
}
void iap2SetEAtesting(BOOL value)
{
    g_EAtesting = value;
}
BOOL iap2GetEAtesting(void)
{
    return g_EAtesting;
}
BOOL iap2InHostMode(void)
{
    return gp_iap2TestDevice->iap2USBHostMode;
}

int iap2InitDev(iAP2InitParam_t* iAP2InitParameter)
{
    S32 rc = IAP2_CTL_ERROR;
    U16 numDev = 0;

    /* detect connected Apple Device and get number of detected devices */
    rc = iap2DetectDevice();
    if(rc > 0){
        printf(" NumDevices = %d \n",rc);
        /* store number of detected devices */
        numDev = (U16)rc;
        rc = IAP2_OK;
    } else{
        printf(" iap2DetectDevice:  Error | rc = %d\n", rc);
    }

    if((rc == IAP2_OK) && (numDev > 0))
    {
        /* Currently, the iAP2 Smoketest supports only 1 Apple device.
         * Use as 1st argument 0.
         */
        /* get structure with serial nr., product name, vendor and product ID */
        rc = iap2GetUdevDevice(0, &gp_iap2TestDevice->udevDevice);
        if(rc != IAP2_OK){
            printf(" iap2GetUdevDevice(0):  rc  = %d \n", rc);
        } else{
            gp_iap2TestDevice->HIDComponentIdentifier = IAP2_HID_COMP_IDENTIFIER;
        }
        /* Switch Off & On USB Vbus power so as to reset the
         * connection of Apple device.
         */

        //iap2USBSwitchOffOn(gp_iap2TestDevice->udevDevice.udevPath);
        //sleep(1);

        /* Disabling this Vbus reset since it is delaying the apple device
         * to be detected in the user space . Mere reset is done and does not have any other impact.
         */

    }
    if(rc == IAP2_OK)
    {
    S32 len = 0;

    /* copy serial number into the iAP2 init parameter structure */
    len = (S32)strnlen((const char*)gp_iap2TestDevice->udevDevice.serialNum, MAX_STRING_LEN);
    strncpy((char*)iAP2InitParameter->iAP2DeviceId,
            (const char*)gp_iap2TestDevice->udevDevice.serialNum, len+1);
    }

    return rc;
}
void iap2USBSwitchOffOn(U8* udevPath)
{
    usbConnectStateType_t type;
    iAP2USBRoleSwitchInfo info;
    udcParamInfo_t udcParam;
    memset(&udcParam, 0, sizeof(udcParamInfo_t));
    /* Identify the USB Connection type of Apple device */
    type = getConnectStateType((char*)udevPath,&udcParam);
    gp_iap2TestDevice->type = type;
    switch(type)
    {
        case UNWIRED_HUB_CONNECTED:
        {
            printf("\n Device Connected to Unwired Hub \n");
            switchBridgePower(udcParam.pUtBridgePath, udcParam.pDevicePortNum, FALSE);
            sleep(1);
            switchBridgePower(udcParam.pUtBridgePath, udcParam.pDevicePortNum, TRUE);
            /* Delay introduced to make the device available in user space after reset*/
            sleep(1);
            break;
        }
        case OTG_CONNECTED:
        {
            printf("\n Device Connected to Native OTG port \n");
            iap2FindVbus(&info);
            iAP2USBPower_Switch(info.vbusPower, "off");
            sleep(1);
            iAP2USBPower_Switch(info.vbusPower, "on");
            /* Delay introduced to make the device available in user space after reset*/
            sleep(1);
            break;
        }
        default:
        {
            printf("\n WARNING: Apple device is not connected to OTG Port \n");
        }
    }

}


void iap2DeinitDev(iAP2InitParam_t* iAP2InitParameter)
{
    /* free allocated memory of the iap2UdevDevice_t structure */
    iap2FreeUdevDevice(0, &gp_iap2TestDevice->udevDevice);
    /* free allocated memory during device detection */
    iap2FreeDetectDevice();

    /* de-initialize the iAP2InitParam_t structure */
    iap2ResetInitialParameter(iAP2InitParameter);
}

int iap2GetArguments(int argc, const char** argv, iap2UserConfig_t *iap2UserConfig)
{
    int rc = 0;
    int app_user_idx;
    int ffs_group_idx;

    /* set default settings */
    iap2UserConfig->iAP2AuthenticationType = iAP2AUTHI2C;
    iap2UserConfig->iAP2TransportType      = iAP2USBDEVICEMODE;
    iap2UserConfig->iap2iOSintheCar        = FALSE;
    iap2UserConfig->iap2EANativeTransport  = FALSE;

    iap2UserConfig->iap2EAPSupported       = FALSE;
    iap2UserConfig->iap2UsbOtgGPIOPower    = NULL;
    iap2UserConfig->SupportediOSAppCount   = 0;

    /* initalize (don't set user or group) */
    iap2UserConfig->app_thread_user = 0;
    iap2UserConfig->app_thread_prim_group = 0;
    iap2UserConfig->ffs_group = 0;

    /* start with empty list of groups */
    iap2UserConfig->app_thread_groups_cnt = 0;
    iap2UserConfig->app_thread_groups = NULL;

    app_user_idx = 4;
    ffs_group_idx = 5;


    /* check input parameters */
    if(argc >= 4)
    {
        /* - run the selected test in USB Device mode or USB Host mode */
        if(0 == strncmp(argv[1], "host", 4))
        {
            iap2UserConfig->iAP2TransportType = iAP2USBHOSTMODE;
            printf("Host Mode selected. '%s' '%s'\n", argv[1], argv[2]);
        }
        else if(0 == strncmp(argv[1], "gadget", 6))
        {
            iap2UserConfig->iAP2TransportType = iAP2USBDEVICEMODE;
            printf("Device Mode selected. '%s' '%s' '%s'\n", argv[1], argv[2], argv[3]);
        }
        else if(0 == strncmp(argv[1], "multihost", 9))
        {
            iap2UserConfig->iAP2TransportType = iAP2MULTIHOSTMODE;
            printf("Multi Host Mode selected. '%s' '%s' '%s'\n", argv[1], argv[2], argv[3]);
        }
        else
        {
            rc = -1;
            printf("Wrong Mode selected. '%s' \n", argv[1]);
        }

        if(rc == 0)
        {
            /* set GPIO for USB power */
            if (0 == strncmp(argv[2], "sd", 2))
            {
                iap2UserConfig->iap2UsbOtgGPIOPower = (VP)IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD;
            }
            else if (0 == strncmp(argv[2], "ai", 2))
            {
                iap2UserConfig->iap2UsbOtgGPIOPower = (VP)IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI;
            }
            else
            {
                rc = -1;
                printf("Wrong board type selected. '%s' \n", argv[2]);
            }
        }

        if(rc == 0)
        {
            if (0 == strncmp(argv[3], "dipo", 4))
            {
                /* --- host mode with DiPO support--- */
                iap2UserConfig->iap2iOSintheCar = TRUE;

                printf("dipo selected. \n");
            }
            else if (0 == strncmp(argv[3], "nopo", 4))
            {
                /* --- without DiPO --- */
                iap2UserConfig->iap2iOSintheCar = FALSE;
                if(argc == 5)
                {
                    if(0 == strncmp(argv[4], "ios8", 4))
                    {
                        app_user_idx++;
                        ffs_group_idx++;
                        iap2SetiOS8testing(TRUE);
                        printf("iOS8 Testing selected. \n");
                    }
                }
            }
            else if (0 == strncmp(argv[3], "ea", 3))
            {
                app_user_idx++;
                ffs_group_idx++;

                /* --- EA test --- */
                iap2SetEAtesting(TRUE);

                if ((0 == strncmp(argv[4], "np", 2))
                    &&(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EAP Session --- */
                    iap2UserConfig->iap2EAPSupported = TRUE;
                    /* --- EA Native Transport --- */
                    iap2UserConfig->iap2EANativeTransport = TRUE;

                    iap2UserConfig->SupportediOSAppCount = 2;
                }
                else if ((0 == strncmp(argv[4], "p", 1))
                      &&(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EAP Session --- */
                    iap2UserConfig->iap2EAPSupported = TRUE;

                    iap2UserConfig->SupportediOSAppCount = 1;
                }
                else if ((0 == strncmp(argv[4], "p", 1))
                        &&(iap2UserConfig->iAP2TransportType == iAP2USBDEVICEMODE))
                {
                    /* --- EAP Session --- */
                    iap2UserConfig->iap2EAPSupported = TRUE;

                    iap2UserConfig->SupportediOSAppCount = 1;
                }
                else if ((0 == strncmp(argv[4], "nn", 2))
                         &&(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EA Native Transport --- */
                    iap2UserConfig->iap2EANativeTransport = TRUE;

                    iap2UserConfig->SupportediOSAppCount = 2;
                }
                else if ((0 == strncmp(argv[4], "n", 1))
                         &&(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EA Native Transport --- */
                    iap2UserConfig->iap2EANativeTransport = TRUE;

                    iap2UserConfig->SupportediOSAppCount = 1;
                }
                else
                {
                    rc = -1;
                    printf("Wrong EA test type or TransportType selected. '%s' '%s' \n", argv[4], argv[1]);
                }
                if(argc == 6)
                {
                    if(0 == strncmp(argv[5], "ios8", 4))
                    {
                        app_user_idx++;
                        ffs_group_idx++;
                        iap2SetiOS8testing(TRUE);
                        printf("iOS8 Testing selected. \n");
                    }
                }
            }
            else
            {
                rc = -1;
                printf("Wrong test type selected. '%s' \n", argv[3]);
            }
        }

        if(argc > app_user_idx)
        {
            /* parse user for app and group for ffs */
            struct passwd *app_user;

            app_user = getpwnam(argv[app_user_idx]);
            if (!app_user) {
                printf("Unknown user %s\n", argv[app_user_idx]);
                rc = -1;
            } else {
                iap2UserConfig->app_thread_user = app_user->pw_uid;
                iap2UserConfig->app_thread_prim_group = app_user->pw_gid;

                getgrouplist(app_user->pw_name, app_user->pw_gid, NULL, &iap2UserConfig->app_thread_groups_cnt);

                iap2UserConfig->app_thread_groups = malloc(iap2UserConfig->app_thread_groups_cnt * sizeof (gid_t));
                if (!iap2UserConfig->app_thread_groups) {
                    printf ("No memory for supplementary group list\n");
                    rc = -1;
                } else {
                    getgrouplist(app_user->pw_name, app_user->pw_gid, iap2UserConfig->app_thread_groups, &iap2UserConfig->app_thread_groups_cnt);
                }
            }
        }

        if(argc > ffs_group_idx)
        {
            struct group *ffs_group;
            ffs_group = getgrnam(argv[ffs_group_idx]);
            if (!ffs_group) {
                printf("Unknown group %s\n", argv[ffs_group_idx]);
                rc = -1;
            } else {
                iap2UserConfig->ffs_group = ffs_group->gr_gid;
            }
        }
    }
    else if(argc == 1)
    {
        /* if now arguments, then:
         * - run the iAP2 Smoketest in USB Device mode
         * - run standard media library tests */

        /* --- device mode --- */
        iap2UserConfig->iAP2TransportType = iAP2USBDEVICEMODE;
        iap2UserConfig->iap2iOSintheCar   = FALSE;
        iap2UserConfig->iap2EANativeTransport = FALSE;

        iap2UserConfig->iap2EAPSupported = FALSE;
        iap2UserConfig->iap2UsbOtgGPIOPower = NULL;
        iap2UserConfig->SupportediOSAppCount = 0;
        iap2SetEAtesting(FALSE);
        printf("Device Mode selected.\n");
    }
    else
    {
        printf("Wrong number of arguments %d \n", argc);
        rc = -1;
    }
    if ((iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE)||(iap2UserConfig->iAP2TransportType == iAP2MULTIHOSTMODE)){
    gp_iap2TestDevice->iap2USBHostMode = TRUE;
    }else{
    gp_iap2TestDevice->iap2USBHostMode = FALSE;
    }
    return rc;
}

void signalHandlerAppProcess(int signo)
{
    printf(" %u ms  Signal %d received\n", iap2CurrTimeMs(), signo);
    iap2SetTestStateError(TRUE);
    iap2SetUtilityQuit(TRUE);
    iap2SetGlobalQuit(TRUE);

#if IAP2_GST_AUDIO_STREAM
    /* stop gstreamer playback */
    iap2SetGstState(IAP2_GSTREAMER_STATE_STOP);
    iap2SleepMs(1000);
    /* leave gstreamer while-loop */
    iap2SetExitGstThread(TRUE);
    iap2SetGstState(IAP2_GSTREAMER_STATE_DEINITIALIZE);
#endif // #if IAP2_GST_AUDIO_STREAM
}

S32 iap2AppSendSwitchMsg (int cmd, int socket_fd, const U8* udevPath, const iAP2InitParam_t* iAP2InitParameter) {
    S32 rc = IAP2_OK;
    struct pollfd pollfds[1];
    iAP2_host_device_switch_cmd_msg_t* cmd_msg;
    iAP2_host_device_switch_result_msg_t* result_msg;
    int res;

    cmd_msg = malloc (sizeof(iAP2_host_device_switch_cmd_msg_t));
    result_msg = malloc (sizeof(iAP2_host_device_switch_result_msg_t));
    if (!cmd_msg || !result_msg) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2AppSendSwitchMsg : no memory for msgs\n" );
        printf(" %u ms  iap2AppSendSwitchMsg : no memory for msgs\n", iap2CurrTimeMs());
        if (cmd_msg) free(cmd_msg);
        if (result_msg) free(result_msg);
        return IAP2_CTL_ERROR;
    }

    pollfds[0].fd = socket_fd;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR;

    cmd_msg->cmd = cmd;
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->udev_path, udevPath );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.vendorId, STR_ACC_INFO_VENDOR_ID );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.productId, STR_ACC_INFO_PRODUCT_ID );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.manufacturer, STR_ACC_INFO_MANUFACTURER );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.name, STR_ACC_INFO_NAME );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.serial, STR_ACC_INFO_SERIAL_NUM );
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.bcdDevice, STR_ACC_INFO_BCD_DEVICE);
    IAP2APPSENDSWITCHMSG_STRING_CP (cmd_msg->iap2GadgetParams.gadget_fs.qmult, STR_ACC_INFO_QMULT);
    cmd_msg->iAP2iOSintheCar = iAP2InitParameter->p_iAP2AccessoryConfig->iAP2iOSintheCar;

    if (rc == IAP2_OK) {
//        printf (" %u ms  Sending switch msg to root process : cmd %d\n", iap2CurrTimeMs(), cmd_msg->cmd);
        rc = write(socket_fd, cmd_msg, sizeof(iAP2_host_device_switch_cmd_msg_t));
        if(rc < 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "Writing to socket_fd failed in iap2AppSendSwitchMsg");
        }
        else
        {
            rc = IAP2_OK;
        }
        pollfds[0].revents = 0;

        res = poll ( pollfds, 1, 20000 ); /* wait with 20sec timeout */
        if (( res <= 0 ) || (pollfds[0].revents != POLLIN)) {
            printf(" %u ms  receiving ack failed res:0x%x revents:0x%x\n", iap2CurrTimeMs(), res, pollfds[0].revents);
            rc = IAP2_CTL_ERROR;
        } else {
            res = read (socket_fd, result_msg, sizeof (iAP2_host_device_switch_result_msg_t));
            if (res != sizeof (iAP2_host_device_switch_result_msg_t)) {
                printf(" %u ms  receiving ack with unexpected length %d\n", iap2CurrTimeMs(), res);
                rc = IAP2_CTL_ERROR;
            } else {
                rc = result_msg->result;
            }
        }
    }

    free(cmd_msg);
    free(result_msg);

    return rc;
}

void host_device_switch_process_main (int socket_fd, iap2UserConfig_t *iap2UserConfig) {
    iAP2USBRoleSwitchInfo info;
    char udev_path_of_info[IAP2_HOST_DEVICE_SWITCH_UDEV_PATH_MAX_LEN];

    S32 rc;
    int res;
    struct udev *udev;
    struct udev_device *dev;
    const char* tmp_dev_attr;
    struct pollfd pollfds[1];
    iAP2_host_device_switch_cmd_msg_t* cmd_msg;
    iAP2_host_device_switch_result_msg_t* result_msg;
    char *mount_fs_param;

    udcParamInfo_t udcParam;
    memset(&udcParam, 0, sizeof(udcParamInfo_t));

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

#if defined(IPOD_ARCH_X86_64)
    /* [temporary workaround] setting of default mode (to host mode) */
    iap2SwitchOTGInitialize();
#endif /* #ifndef IPOD_ARCH_ARM */

    IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process started\n" );
    printf(" %u ms  host-device-switching process started\n", iap2CurrTimeMs());

    udev = udev_new();
    if (!udev) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process could not register udev\n" );
        printf(" %u ms  host-device-switching process could not register udev\n", iap2CurrTimeMs());
        return;
    }

    cmd_msg = malloc (sizeof(iAP2_host_device_switch_cmd_msg_t));
    result_msg = malloc (sizeof(iAP2_host_device_switch_result_msg_t));
    if (!cmd_msg || !result_msg) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : no memory for msgs\n" );
        printf(" %u ms  host-device-switching process : no memory for msgs\n", iap2CurrTimeMs());
        if (cmd_msg) free(cmd_msg);
        if (result_msg) free(result_msg);
        return;
    }

    memset(&info, 0, sizeof(info));
    memset(udev_path_of_info, 0, sizeof(udev_path_of_info));

    pollfds[0].fd = socket_fd;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR;

    while (1) {
        pollfds[0].revents = 0;

        res = poll (pollfds, 1, -1);
        if (res <= 0) {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : poll returned with error\n" );
            printf(" %u ms  host-device-switching process : poll returned with error\n", iap2CurrTimeMs());
            break;
        }

        if (pollfds[0].revents & ~(POLLIN | POLLHUP)) {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : poll returned unexpected events (revents 0x%x)\n", pollfds[0].revents);
            printf(" %u ms  host-device-switching process : poll returned unexpected events (revents 0x%x)\n", iap2CurrTimeMs(), pollfds[0].revents);
            break;
        }
        if (pollfds[0].revents & POLLHUP) {
            /* other side is closed, so we can close too */
            break;
        }

        rc = IAP2_OK;

        res = read(socket_fd, cmd_msg, sizeof(iAP2_host_device_switch_cmd_msg_t));
        if (res != sizeof(iAP2_host_device_switch_cmd_msg_t)) {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : read msg has unexpected length %d - expected %zu\n", res, sizeof(iAP2_host_device_switch_cmd_msg_t));
            printf(" %u ms  host-device-switching process : read msg has unexpected length %d - expected %zu\n", iap2CurrTimeMs(), res, sizeof(iAP2_host_device_switch_cmd_msg_t));
            rc = IAP2_CTL_ERROR;
        }

        if (rc == IAP2_OK) {
            IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : new command received %d - udevPath %s\n", cmd_msg->cmd, cmd_msg->udev_path );
//            printf(" %u ms  host-device-switching process : new command received %d - udevPath %s\n", iap2CurrTimeMs(), cmd_msg->cmd, cmd_msg->udev_path );

            if (cmd_msg->cmd == IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST) {
                /* --- switch to host mode --- */

                /* fill info and store udev path belonging to info */

                /* depreciated.
                 * use vbusPower. */
                info.powerGPIO = -1;

                dev = udev_device_new_from_syspath (udev, cmd_msg->udev_path);
                if (dev == NULL) {
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : failed to find udevice for %s, Error:%d(%s)\n", cmd_msg->udev_path, errno, strerror(errno));
                    printf(" %u ms  host-device-switching process : failed to find udevice for %s, Error:%d(%s)\n\n", iap2CurrTimeMs(), cmd_msg->udev_path, errno, strerror(errno));
                    rc = IAP2_CTL_ERROR;
                }

                if (rc == IAP2_OK) {
                    /* FIXME check if the device is the only device connected to the port
                     * if not switching the otg mode might cause problems for the other device
                     */

                    /* FIXME when supporting multiple otg ports we need to find the correct vbus
                     * by searching the parent usb port of the device
                     */
                    rc = iap2FindVbus(&info);
                    IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : iap2FindVbus  = %d | %s\n", rc, info.vbusPower );
                    printf(" %u ms  iap2FindVbus  = %d | %s\n", iap2CurrTimeMs(), rc, info.vbusPower);
                    /* if we are not on im6-qi, we might don't have vbus */
                    if (rc != IAP2_OK) {
                        IAP2TESTDLTLOG(DLT_LOG_INFO, "No vbus available");
                        /* continue until we know that we not directly connected to USB OTG port */
                        rc = IAP2_OK;
                    }
                    /* vendorId and productId of an Apple device */
                    tmp_dev_attr = udev_device_get_sysattr_value(dev, "idVendor");
                    if (tmp_dev_attr != NULL) {
                        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : idVendor = %s\n", tmp_dev_attr );
//                        printf(" %u ms  host-device-switching process : idVendor = %s\n", iap2CurrTimeMs(), tmp_dev_attr );
                        info.vendorId = strtol (tmp_dev_attr, NULL, 16);
                    } else {
                        rc = IAP2_CTL_ERROR;
                    }
                    tmp_dev_attr = udev_device_get_sysattr_value(dev, "idProduct");
                    if (tmp_dev_attr != NULL) {
                        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : idProduct = %s\n", tmp_dev_attr );
//                        printf(" %u ms  host-device-switching process : idProduct = %s\n", iap2CurrTimeMs(), tmp_dev_attr );
                        info.productId = strtol (tmp_dev_attr, NULL, 16);
                    } else {
                        rc = IAP2_CTL_ERROR;
                    }
                    info.serialNumber = udev_device_get_sysattr_value(dev, "serial");
                    if (info.serialNumber != NULL) {
                        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : serial = %s\n", info.serialNumber );
//                        printf(" %u ms  host-device-switching process : serial = %s\n", iap2CurrTimeMs(), info.serialNumber );
                    } else {
                        rc = IAP2_CTL_ERROR;
                    }

                    /* check if vendorId of an Apple device
                     * FIXME later we might check the product id too
                     */
                    if (info.vendorId != 0x05ac) {
                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : device at %s is no apple device\n vendor id is 0x%x, product id is 0x%x", cmd_msg->udev_path, info.vendorId, info.productId );
                        printf(" %u ms  host-device-switching process : device at %s is no apple device\n vendor id is 0x%x, product id is 0x%x\n", iap2CurrTimeMs(), cmd_msg->udev_path, info.vendorId, info.productId );
                        rc = IAP2_CTL_ERROR;
                    }

                    info.otgGlob = iap2GetSwtichOtgGlobPath();
                    if(cmd_msg->iAP2iOSintheCar == TRUE){
                        info.mode = IAP2_USB_ROLE_SWITCH_WITH_DIGITAL_IPOD_OUT;
                    } else{
                        info.mode = IAP2_USB_ROLE_SWITCH_WITHOUT_DIGITAL_IPOD_OUT;
                    }

                    /* store udev path belonging to filled info */
                    strcpy(udev_path_of_info, cmd_msg->udev_path);
                }
                if (rc == IAP2_OK)
                {
                    if(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE)
                        rc = iap2SwitchToHostMode(&info, &udcParam);
                    else
                        rc = iap2SwitchToMultiHostMode(&info, &udcParam);
                }
                if (rc == IAP2_OK) {
                    if(udcParam.pUdcDevice != NULL)
                    {
                        if(iap2UserConfig->UdcDeviceName != NULL)
                        {
                            strcpy((char*)iap2UserConfig->UdcDeviceName, (const char*)udcParam.pUdcDevice);
                        }
                    }
                    IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : iap2SwitchToHostMode = %d, pUdcDevice = %s\n", rc, iap2UserConfig->UdcDeviceName );
//                    printf(" %u ms  host-device-switching process : iap2SwitchToHostMode = %d\n", iap2CurrTimeMs(), rc);
                }
                if(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE)
                {
                if (IAP2_OK == rc){
                    if (iap2IsKernel314() || iap2IsKernel4xx())
                    {
                        /* kernel module libcomposite depends on module configfs */
                        rc = iap2LoadModule(NULL, CONFIGFS_MODULE_NAME, strlen(CONFIGFS_MODULE_NAME));
                    }
#ifdef IAP2_USE_CONFIGFS
                    if(IAP2_OK == rc) {
                        mount_fs_param = NULL;
                        res = mount("config", "/sys/kernel/config", "configfs", MS_NOEXEC, mount_fs_param);
                        if (res != 0){
                            rc = IAP2_CTL_ERROR;
                        }
                        printf(" %u ms  mount() res = %d\n", iap2CurrTimeMs(), res);

                        /* after mount we need to set the mode again */
                        res = chmod("/sys/kernel/config", 0750);
                        if (res != 0){
                            rc = IAP2_CTL_ERROR;
                        }
                        printf(" %u ms  chmod() res = %d\n", iap2CurrTimeMs(), res);
                    }
#endif
                    if(IAP2_OK == rc) {
                        if(IAP2_OK != iap2LoadModule(NULL, LIBCOMPOSITE_MODULE_NAME, strlen(LIBCOMPOSITE_MODULE_NAME))) {
                            rc = IAP2_CTL_ERROR;
                        }
                    }
#ifndef IAP2_USE_CONFIGFS
                    char mount_fs_param_buf[MAX_STRING_LEN];

                    if ((iap2IsKernel314() || iap2IsKernel4xx()) &&
                         (IAP2_OK == rc) )
                    {
                        if(IAP2_OK == rc) {
                            /* kernel module g_ffs depends on module usb_f_fs */
                            if(IAP2_OK != iap2LoadModule(NULL, USB_F_FS_MODULE_NAME, strlen(USB_F_FS_MODULE_NAME))) {
                                rc = IAP2_CTL_ERROR;
                            }
                        }
                        if(IAP2_OK == rc) {
                            /* kernel module g_ffs depends on module u_ether */
                            if(IAP2_OK != iap2LoadModule(NULL, U_ETHER_MODULE_NAME, strlen(U_ETHER_MODULE_NAME))) {
                                rc = IAP2_CTL_ERROR;
                            }
                        }
                        if(IAP2_OK == rc) {
                            /* kernel module g_ffs depends on module usb_f_ncm */
                            if(IAP2_OK != iap2LoadModule(NULL, USB_F_NCM_MODULE_NAME, strlen(USB_F_NCM_MODULE_NAME))) {
                                rc = IAP2_CTL_ERROR;
                            }
                        }
                        if(IAP2_OK == rc) {
                        if(IAP2_OK != iap2LoadModule(&(cmd_msg->iap2GadgetParams), GADGET_FFS_MODULE_NAME, strlen(GADGET_FFS_MODULE_NAME))) {
                                rc = IAP2_CTL_ERROR;
                            }
                        }
                    }
                    if(IAP2_OK == rc) {
                        res = mkdir(FUNCTION_FS_PATH, 0750);
                        if (res != 0){
                            rc = IAP2_CTL_ERROR;
                        }

                        mount_fs_param = NULL;
                        if (iap2UserConfig->ffs_group != 0) {
                            snprintf(mount_fs_param_buf, MAX_STRING_LEN, "mode=0660,gid=%d", iap2UserConfig->ffs_group);
                            mount_fs_param = mount_fs_param_buf;
                        }
                        res = mount(FUNCTION_FS_NAME, FUNCTION_FS_PATH, FUNCTION_FS_TYPE, MS_NOEXEC, mount_fs_param);
                        if (res != 0){
                            rc = IAP2_CTL_ERROR;
                        }

                        /* after mount we need to set the mode again */
                        res = chmod(FUNCTION_FS_PATH, 0750);
                        if (res != 0){
                            rc = IAP2_CTL_ERROR;
                        }
                    }
#endif
                }
                }
            } else if (cmd_msg->cmd == IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE) {

                /* FIXME for supporting multiple ones we need to have a list of infos
                 * and search for correct one
                 */
                if (strcmp(udev_path_of_info, cmd_msg->udev_path)) {
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : udev path of device to be switched to device mode (%s) does not match udev path of device switched to host mode previously (%s)\n", cmd_msg->udev_path, udev_path_of_info);
                    printf(" %u ms  host-device-switching process : udev path of device to be switched to device mode (%s) does not match udev path of device switched to host mode previously (%s)\n", iap2CurrTimeMs(), cmd_msg->udev_path, udev_path_of_info);
                    rc = IAP2_CTL_ERROR;
                } else {
                    iap2SleepMs(1000);
                    if(iap2UserConfig->iAP2TransportType == iAP2USBHOSTMODE){
#ifdef IAP2_USE_CONFIGFS
                    umount("/sys/kernel/config");
                    rmdir("/sys/kernel/config");

                    if( (iap2IsKernel314() || iap2IsKernel4xx()) && (iap2UserConfig->iap2iOSintheCar == FALSE) )
                    {
                        iap2UnloadModule(USB_F_UAC2);
                        if(iap2IsKernel414() == TRUE)
                        {
                            /*SWGIII-27741
                             * From Kernel 4.14, some functionality originally belonging to usb_f_uac2.ko has
                             * been moved to a newly created module u_audio.ko, so the test has to unload u_audio.ko*/
                            iap2UnloadModule(U_AUDIO);
                        }
                    }

                    if( (iap2IsKernel314() || iap2IsKernel4xx()) && (iap2UserConfig->iap2iOSintheCar == TRUE) )
                    {
                        iap2UnloadModule(USB_F_NCM_MODULE_NAME);
                        iap2UnloadModule(U_ETHER_MODULE_NAME);
                    }
#endif
                    umount(FUNCTION_FS_PATH);
                    rmdir(FUNCTION_FS_PATH);
#ifndef IAP2_USE_CONFIGFS
                    iap2UnloadModule(GADGET_FFS_MODULE_NAME);
                    if (iap2IsKernel314() || iap2IsKernel4xx())
                    {
                        iap2UnloadModule(USB_F_NCM_MODULE_NAME);
                        iap2UnloadModule(U_ETHER_MODULE_NAME);
                    }
#endif
                    if (iap2IsKernel314() || iap2IsKernel4xx())
                    {
                    iap2UnloadModule(USB_F_FS_MODULE_NAME);
                    }
                    iap2UnloadModule(LIBCOMPOSITE_MODULE_NAME);
                    if (iap2IsKernel314() || iap2IsKernel4xx())
                    {
                    iap2UnloadModule(CONFIGFS_MODULE_NAME);
                    }
                    iap2SleepMs(1000);
                    rc = iap2SwitchToDeviceMode(&info, &udcParam);
                    IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : Switch USB OTG to Host  = %d", rc );
//                    printf(" %u ms  host-device-switching process : Switch USB OTG to Host  = %d\n", iap2CurrTimeMs(), rc);
                    }
                    else
                    {
                        rc = iap2TerminateMultiHostMode(&info);
                        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : MultiHost session closed  = %d", rc );
                    }
                    if(NULL != info.vbusPower)
                    {
                        free(info.vbusPower);
                    }

                    /* clear info */
                    memset(&info, 0, sizeof(info));
                    memset(udev_path_of_info, 0, sizeof(udev_path_of_info));
                }
            } else {
                IAP2TESTDLTLOG(DLT_LOG_ERROR, "host-device-switching process : read msg with unexpected cmd %d\n", cmd_msg->cmd );
                printf(" %u ms  host-device-switching process : read msg with unexpected cmd %d\n", iap2CurrTimeMs(), cmd_msg->cmd);
                rc = IAP2_CTL_ERROR;
            }
        }

        result_msg->result = rc;
        IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process : result of cmd is %d\n", result_msg->result);
//        printf(" %u ms  host-device-switching process : result of cmd is %d\n", iap2CurrTimeMs(), result_msg->result);
        rc = write (socket_fd, result_msg, sizeof(iAP2_host_device_switch_result_msg_t));
        if(rc < 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "Writing to socket_fd failed in host_device_switch_process_main");
        }
        else
        {
            rc = IAP2_OK;
        }
    }

   IAP2TESTDLTLOG(DLT_LOG_INFO, "host-device-switching process shutting down\n" );
   printf(" %u ms  host-device-switching process shutting down\n", iap2CurrTimeMs());

    udev_unref(udev);
    free(cmd_msg);
    free(result_msg);
}
